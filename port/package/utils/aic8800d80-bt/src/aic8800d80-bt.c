// SPDX-License-Identifier: GPL-2.0-or-later
/* AIC8800D80 UART bring-up for T153.
 * Command IDs, AON values and ordering follow the Allwinner BlueZ SDK patch
 * 012-add-aw869b-hciattach-support.patch (luoyuyi, 2021-08-24).
 * This implementation uses bounded I/O and explicit little-endian encoding.
 */
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#ifndef N_HCI
#define N_HCI 15
#endif
#define HCIUARTSETPROTO _IOW('U', 200, int)
#define HCIUARTGETDEVICE _IOR('U', 202, int)
#define HCI_UART_H4 0
/* Linux Bluetooth socket ABI; no libbluetooth runtime dependency. */
#define BTPROTO_HCI 1
#define HCIDEVDOWN _IOW('H', 202, int)
#define COMMAND_TIMEOUT_MS 3000
#define AON_SIZE 104
static volatile sig_atomic_t stopping;

static void signal_stop(int sig)
{
	(void)sig;
	stopping = 1;
}

static int64_t monotonic_ms(void)
{
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
		return -1;
	return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static int wait_fd(int fd, short events, int64_t deadline)
{
	while (!stopping) {
		int64_t now = monotonic_ms();
		struct pollfd pfd = { .fd = fd, .events = events };
		int rc;
		if (now < 0)
			return -1;
		if (now >= deadline) {
			errno = ETIMEDOUT;
			return -1;
		}
		rc = poll(&pfd, 1, (int)(deadline - now));
		if (rc < 0 && errno == EINTR)
			continue;
		if (rc < 0)
			return -1;
		if (rc == 0) {
			errno = ETIMEDOUT;
			return -1;
		}
		if (pfd.revents & events)
			return 0;
		if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
			errno = EIO;
			return -1;
		}
	}
	errno = EINTR;
	return -1;
}

static int transfer(int fd, void *buffer, size_t len, int write_data,
		    int64_t deadline)
{
	uint8_t *p = buffer;
	while (len) {
		ssize_t rc;
		if (wait_fd(fd, write_data ? POLLOUT : POLLIN, deadline) < 0)
			return -1;
		rc = write_data ? write(fd, p, len) : read(fd, p, len);
		if (rc < 0 && (errno == EAGAIN || errno == EINTR))
			continue;
		if (rc < 0)
			return -1;
		if (rc == 0) {
			errno = EIO;
			return -1;
		}
		p += rc;
		len -= (size_t)rc;
	}
	return 0;
}

/* All commands used below return Command Complete with a status byte. */
static int hci_command(int fd, uint16_t opcode, const uint8_t *data, uint8_t len)
{
	uint8_t command[259] = { 0x01, opcode & 0xff, opcode >> 8, len };
	int64_t now = monotonic_ms();
	int64_t deadline;
	unsigned int events = 0;
	if (now < 0)
		return -1;
	deadline = now + COMMAND_TIMEOUT_MS;
	if (len)
		memcpy(command + 4, data, len);
	if (transfer(fd, command, (size_t)len + 4, 1, deadline) < 0)
		return -1;
	while (events++ < 32) {
		uint8_t header[3], payload[255];
		uint16_t returned_opcode;
		if (transfer(fd, header, sizeof(header), 0, deadline) < 0)
			return -1;
		if (header[0] != 0x04) {
			errno = EPROTO;
			return -1;
		}
		if (transfer(fd, payload, header[2], 0, deadline) < 0)
			return -1;
		if (header[1] == 0x0e) {
			if (header[2] < 4) {
				errno = EPROTO;
				return -1;
			}
			returned_opcode = (uint16_t)payload[1] | (uint16_t)payload[2] << 8;
			if (returned_opcode != opcode)
				continue;
			if (payload[3]) {
				fprintf(stderr, "HCI command 0x%04x failed: status 0x%02x\n", opcode, payload[3]);
				errno = EREMOTEIO;
				return -1;
			}
			return 0;
		}
		if (header[1] == 0x0f) {
			if (header[2] != 4) {
				errno = EPROTO;
				return -1;
			}
			returned_opcode = (uint16_t)payload[2] | (uint16_t)payload[3] << 8;
			if (returned_opcode == opcode && payload[0]) {
				fprintf(stderr, "HCI command 0x%04x rejected: status 0x%02x\n", opcode, payload[0]);
				errno = EREMOTEIO;
				return -1;
			}
		}
	}
	errno = EPROTO;
	return -1;
}

static void put16(uint8_t *p, uint16_t v)
{
	p[0] = v & 0xff;
	p[1] = v >> 8;
}

static void put32(uint8_t *p, uint32_t v)
{
	put16(p, (uint16_t)v);
	put16(p + 2, (uint16_t)(v >> 16));
}

static void encode_aon(uint8_t p[AON_SIZE])
{
	static const uint32_t a[] = { 0x18d700, 0x18f700, 64 };
	static const uint16_t b[] = { 40, 400, 400, 3, 2, 3, 2, 40, 512, 20, 21, 20 };
	static const uint32_t c[] = { 32, 8, UINT32_MAX, 0, 20000, 0x113, 0x20067302 };
	static const uint16_t d[] = { 7, 64, 72, 71, 32, 512, 420, 100, 100, 8, 24, 40, 140, 0 };
	size_t i;
	/* Original ARM ABI structure has two bytes of zero tail padding. */
	memset(p, 0, AON_SIZE);
	for (i = 0; i < sizeof(a) / sizeof(a[0]); ++i)
		put32(p + i * 4, a[i]);
	for (i = 0; i < sizeof(b) / sizeof(b[0]); ++i)
		put16(p + 12 + i * 2, b[i]);
	for (i = 0; i < sizeof(c) / sizeof(c[0]); ++i)
		put32(p + 36 + i * 4, c[i]);
	for (i = 0; i < sizeof(d) / sizeof(d[0]); ++i)
		put16(p + 64 + i * 2, d[i]);
	put32(p + 92, 64);
	put32(p + 96, 20000);
	put16(p + 100, 50);
}

static int parse_address(const char *str, uint8_t addr[6])
{
	unsigned int a[6];
	char tail;
	unsigned int i, all_zero = 1, all_ff = 1;
	if (strlen(str) != 17)
		return -1;
	for (i = 0; i < 17; ++i) {
		if ((i % 3 == 2 && str[i] != ':') ||
		    (i % 3 != 2 && !isxdigit((unsigned char)str[i])))
			return -1;
	}
	if (sscanf(str, "%2x:%2x:%2x:%2x:%2x:%2x%c",
		&a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &tail) != 6)
		return -1;
	for (i = 0; i < 6; ++i) {
		addr[i] = (uint8_t)a[i];
		all_zero &= (a[i] == 0);
		all_ff &= (a[i] == 255);
	}
	return (all_zero || all_ff || (addr[0] & 1)) ? -1 : 0;
}

static int derive_address(uint8_t addr[6])
{
	/* No common placeholder is permitted. Use a board-specific identity only.
	 * FNV-1a is an identifier hash, not a security or privacy mechanism. */
	static const char *const paths[] = {
		"/sys/firmware/devicetree/base/serial-number",
		"/sys/devices/soc0/serial_number",
		"/sys/class/block/mmcblk0/device/cid",
		"/sys/class/block/mmcblk1/device/cid",
		"/sys/class/block/mmcblk2/device/cid",
		"/etc/machine-id", NULL
	};
	const char domain[] = "t153-aic8800d80-bt-v1:";
	uint8_t identity[256];
	uint64_t hash = UINT64_C(14695981039346656037);
	ssize_t len = -1;
	size_t i;
	for (i = 0; paths[i]; ++i) {
		int fd = open(paths[i], O_RDONLY | O_CLOEXEC);
		if (fd < 0)
			continue;
		len = read(fd, identity, sizeof(identity));
		close(fd);
		while (len > 0 && (identity[len - 1] == 0 || identity[len - 1] == '\n' || identity[len - 1] == '\r'))
			--len;
		if (len > 0 && (size_t)len < sizeof(identity)) {
			ssize_t j;
			int specific = 0;
			for (j = 0; j < len; ++j)
				if (identity[j] != '0' && identity[j] != '-' && identity[j] != ':' && identity[j] != ' ')
					specific = 1;
			if (specific) {
				fprintf(stderr, "Deriving Bluetooth address from %s\n", paths[i]);
				break;
			}
		}
		len = -1;
	}
	if (len <= 0) {
		fprintf(stderr, "No unique SoC serial, MMC CID or machine-id; configure option address explicitly\n");
		errno = ENODATA;
		return -1;
	}
	for (i = 0; i < sizeof(domain) - 1; ++i) {
		hash ^= (uint8_t)domain[i];
		hash *= UINT64_C(1099511628211);
	}
	for (i = 0; i < (size_t)len; ++i) {
		hash ^= identity[i];
		hash *= UINT64_C(1099511628211);
	}
	for (i = 0; i < 6; ++i)
		addr[i] = (uint8_t)(hash >> (8 * (5 - i)));
	addr[0] = (addr[0] & 0xfc) | 0x02;
	return 0;
}

static int rfkill_set(const char *path, int blocked)
{
	int fd = open(path, O_WRONLY | O_CLOEXEC);
	const char *value = blocked ? "1\n" : "0\n";
	ssize_t rc;
	if (fd < 0)
		return -1;
	do {
		rc = write(fd, value, 2);
	} while (rc < 0 && errno == EINTR);
	if (rc != 2) {
		int saved = rc < 0 ? errno : EIO;
		close(fd);
		errno = saved;
		return -1;
	}
	return close(fd);
}

static int delay_ms(unsigned int ms)
{
	struct timespec t = { .tv_sec = ms / 1000, .tv_nsec = (long)(ms % 1000) * 1000000 };
	while (nanosleep(&t, &t) < 0) {
		if (errno != EINTR || stopping)
			return -1;
	}
	return 0;
}

static int initialize_controller(int fd, const uint8_t addr[6])
{
	uint8_t reversed[6], aon[AON_SIZE];
	const uint8_t active = 0, slave = 1, power_off = 0;
	size_t i;
	for (i = 0; i < 6; ++i)
		reversed[i] = addr[5 - i];
	encode_aon(aon);
	if (hci_command(fd, 0x0c03, NULL, 0) < 0 ||
	    hci_command(fd, 0xfc70, reversed, sizeof(reversed)) < 0 ||
	    hci_command(fd, 0xfc4d, aon, sizeof(aon)) < 0 ||
	    hci_command(fd, 0xfc50, &active, 1) < 0 ||
	    hci_command(fd, 0xfc51, &slave, 1) < 0 ||
	    hci_command(fd, 0xfc52, &power_off, 1) < 0)
		return -1;
	return 0;
}

/* Finish RESET_ON_CLOSE while N_HCI can still transmit and receive. */
static int shutdown_controller(int index)
{
	int sock, result, saved;

	sock = socket(AF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC, BTPROTO_HCI);
	if (sock < 0)
		return -1;
	do {
		result = ioctl(sock, HCIDEVDOWN, index);
	} while (result < 0 && errno == EINTR);
	saved = errno;
	close(sock);
	/* A controller already removed by the kernel needs no further down. */
	if (result < 0 && saved != ENODEV) {
		errno = saved;
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	const char *device = NULL, *rfkill = NULL, *address = "auto";
	uint8_t addr[6];
	struct termios original, configured;
	struct sigaction action;
	int i, fd = -1, rc = EXIT_FAILURE, termios_saved = 0, attached = 0;
	int reset_started = 0, hci_id = -1;
	for (i = 1; i < argc; ++i) {
		if (i + 1 < argc && !strcmp(argv[i], "--device"))
			device = argv[++i];
		else if (i + 1 < argc && !strcmp(argv[i], "--rfkill-soft"))
			rfkill = argv[++i];
		else if (i + 1 < argc && !strcmp(argv[i], "--address"))
			address = argv[++i];
		else {
			fprintf(stderr, "Usage: %s --device /dev/ttyS7 --rfkill-soft /sys/class/rfkill/rfkillN/soft [--address auto|XX:XX:XX:XX:XX:XX]\n", argv[0]);
			return EXIT_FAILURE;
		}
	}
	if (!device || !rfkill) {
		fprintf(stderr, "UART and named Bluetooth rfkill soft path are required\n");
		return EXIT_FAILURE;
	}
	if ((!strcmp(address, "auto") ? derive_address(addr) : parse_address(address, addr)) < 0) {
		fprintf(stderr, "Bluetooth address is missing or invalid\n");
		return EXIT_FAILURE;
	}
	memset(&action, 0, sizeof(action));
	action.sa_handler = signal_stop;
	sigemptyset(&action.sa_mask);
	if (sigaction(SIGINT, &action, NULL) < 0 || sigaction(SIGTERM, &action, NULL) < 0) {
		perror("sigaction");
		return EXIT_FAILURE;
	}
	fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
	if (fd < 0) {
		perror(device);
		goto out;
	}
	if (flock(fd, LOCK_EX | LOCK_NB) < 0 || ioctl(fd, TIOCEXCL) < 0) {
		perror("claim Bluetooth UART");
		goto out;
	}
	if (tcgetattr(fd, &original) < 0) {
		perror("get UART attributes");
		goto out;
	}
	termios_saved = 1;
	configured = original;
	cfmakeraw(&configured);
	configured.c_cflag |= CLOCAL | CREAD | CRTSCTS;
	configured.c_cflag &= ~(CSTOPB | PARENB | CSIZE);
	configured.c_cflag |= CS8;
	configured.c_cc[VMIN] = 1;
	configured.c_cc[VTIME] = 0;
	if (cfsetispeed(&configured, B1500000) < 0 || cfsetospeed(&configured, B1500000) < 0 ||
	    tcsetattr(fd, TCSANOW, &configured) < 0) {
		perror("configure UART 1500000 8N1 RTS/CTS");
		goto out;
	}
	reset_started = 1;
	if (rfkill_set(rfkill, 1) < 0 || delay_ms(100) < 0 ||
	    rfkill_set(rfkill, 0) < 0 || delay_ms(500) < 0 ||
	    tcflush(fd, TCIOFLUSH) < 0) {
		perror("reset Bluetooth via rfkill-gpio");
		goto out;
	}
	if (initialize_controller(fd, addr) < 0) {
		perror("AIC8800D80 vendor initialization");
		goto out;
	}
	if (stopping)
		goto out;
	i = N_HCI;
	if (ioctl(fd, TIOCSETD, &i) < 0) {
		perror("set HCI line discipline");
		goto out;
	}
	attached = 1;
	if (ioctl(fd, HCIUARTSETPROTO, HCI_UART_H4) < 0) {
		perror("attach H4 protocol");
		goto out;
	}
	hci_id = ioctl(fd, HCIUARTGETDEVICE);
	if (hci_id < 0) {
		perror("get registered HCI device");
		goto out;
	}
	fprintf(stderr, "AIC8800D80 attached as hci%d at 1500000 baud, address %02X:%02X:%02X:%02X:%02X:%02X; low power disabled\n",
		hci_id, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
	while (!stopping)
		(void)poll(NULL, 0, 1000);
	rc = EXIT_SUCCESS;
out:
	if (fd >= 0) {
		if (attached) {
			if (hci_id >= 0 && shutdown_controller(hci_id) < 0) {
				perror("power down HCI before UART detach");
				rc = EXIT_FAILURE;
			}
			i = N_TTY;
			if (ioctl(fd, TIOCSETD, &i) < 0)
				perror("restore TTY line discipline");
		}
		if (termios_saved && tcsetattr(fd, TCSANOW, &original) < 0)
			perror("restore UART attributes");
		close(fd);
	}
	if (reset_started && rfkill_set(rfkill, 1) < 0)
		perror("block Bluetooth on exit");
	return rc;
}
