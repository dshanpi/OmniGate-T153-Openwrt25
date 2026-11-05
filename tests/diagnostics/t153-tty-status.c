// SPDX-License-Identifier: GPL-2.0-only
/* Standard TTY ioctl snapshot: no reads/writes of UART data or registers. */
#define _DEFAULT_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/serial.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static void json_string(const char *s)
{
	putchar('"');
	for (; *s; ++s) {
		unsigned char c = (unsigned char)*s;
		if (c == '"' || c == '\\') printf("\\%c", c);
		else if (c < 32) printf("\\u%04x", c);
		else putchar(c);
	}
	putchar('"');
}

static void failure(int err)
{
	printf("{\"ok\":false,\"errno\":%d,\"error\":", err);
	json_string(strerror(err));
	putchar('}');
}

static unsigned long baud(speed_t b)
{
	switch (b) {
	case B9600: return 9600;
	case B19200: return 19200;
	case B38400: return 38400;
	case B57600: return 57600;
	case B115200: return 115200;
	case B230400: return 230400;
	case B460800: return 460800;
	case B921600: return 921600;
	case B1000000: return 1000000;
	case B1500000: return 1500000;
	case B2000000: return 2000000;
	case B3000000: return 3000000;
	default: return 0;
	}
}

int main(int argc, char **argv)
{
	struct serial_struct ser;
	struct serial_icounter_struct cnt;
	struct termios t;
	int fd, value, serial_ok = 0, count_ok = 0;
	if (argc != 2) {
		fprintf(stderr, "Usage: %s /dev/ttyS7\n", argv[0]);
		return 2;
	}
	printf("{\"device\":"); json_string(argv[1]);
	printf(",\"operation\":\"standard-tty-ioctl-snapshot\",\"open\":");
	fd = open(argv[1], O_RDONLY | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
	if (fd < 0) { failure(errno); puts("}"); return 1; }
	printf("{\"ok\":true},\"serial\":");
	memset(&ser, 0, sizeof(ser));
	if (ioctl(fd, TIOCGSERIAL, &ser) < 0) failure(errno);
	else {
		serial_ok = 1;
		printf("{\"ok\":true,\"type\":%d,\"line\":%d,\"irq\":%d,\"flags\":%d,\"xmit_fifo_size\":%d,\"baud_base\":%d,\"custom_divisor\":%d,\"io_type\":%u}",
			ser.type, ser.line, ser.irq, ser.flags, ser.xmit_fifo_size,
			ser.baud_base, ser.custom_divisor, (unsigned int)ser.io_type);
	}
	printf(",\"icount\":");
	memset(&cnt, 0, sizeof(cnt));
	if (ioctl(fd, TIOCGICOUNT, &cnt) < 0) failure(errno);
	else {
		count_ok = 1;
		printf("{\"ok\":true,\"cts\":%d,\"dsr\":%d,\"rng\":%d,\"dcd\":%d,\"rx\":%d,\"tx\":%d,\"frame\":%d,\"overrun\":%d,\"parity\":%d,\"brk\":%d,\"buf_overrun\":%d}",
			cnt.cts, cnt.dsr, cnt.rng, cnt.dcd, cnt.rx, cnt.tx, cnt.frame,
			cnt.overrun, cnt.parity, cnt.brk, cnt.buf_overrun);
	}
	printf(",\"termios\":");
	if (tcgetattr(fd, &t) < 0) failure(errno);
	else printf("{\"ok\":true,\"cflag\":%lu,\"iflag\":%lu,\"ispeed\":%lu,\"ospeed\":%lu,\"rtscts\":%s,\"clocal\":%s}",
		(unsigned long)t.c_cflag, (unsigned long)t.c_iflag,
		baud(cfgetispeed(&t)), baud(cfgetospeed(&t)),
		(t.c_cflag & CRTSCTS) ? "true" : "false", (t.c_cflag & CLOCAL) ? "true" : "false");
	printf(",\"modem\":");
	if (ioctl(fd, TIOCMGET, &value) < 0) failure(errno);
	else printf("{\"ok\":true,\"bits\":%d,\"cts\":%s,\"rts\":%s,\"dtr\":%s}", value,
		(value & TIOCM_CTS) ? "true" : "false", (value & TIOCM_RTS) ? "true" : "false", (value & TIOCM_DTR) ? "true" : "false");
	printf(",\"line_discipline\":");
	if (ioctl(fd, TIOCGETD, &value) < 0) failure(errno);
	else printf("{\"ok\":true,\"value\":%d}", value);
	printf(",\"tx_queued\":");
	if (ioctl(fd, TIOCOUTQ, &value) < 0) failure(errno);
	else printf("{\"ok\":true,\"bytes\":%d}", value);
	puts("}");
	close(fd);
	return serial_ok && count_ok ? 0 : 1;
}
