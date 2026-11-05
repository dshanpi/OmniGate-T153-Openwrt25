// SPDX-License-Identifier: GPL-2.0-or-later
#define main helper_main
#include "../src/aic8800d80-bt.c"
#undef main
#include <assert.h>
#include <sys/socket.h>
#include <sys/wait.h>

static void test_aon(void)
{
	/* Original SDK ARM layout: 3 u32, 12 u16, 7 u32, 14 u16,
	 * 2 u32, 1 u16 and 2 zero tail-padding bytes. */
	static const uint8_t expected[AON_SIZE] = {
		0x00,0xd7,0x18,0x00,0x00,0xf7,0x18,0x00,0x40,0x00,0x00,0x00,
		0x28,0x00,0x90,0x01,0x90,0x01,0x03,0x00,0x02,0x00,0x03,0x00,
		0x02,0x00,0x28,0x00,0x00,0x02,0x14,0x00,0x15,0x00,0x14,0x00,
		0x20,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0xff,0xff,0xff,0xff,
		0x00,0x00,0x00,0x00,0x20,0x4e,0x00,0x00,0x13,0x01,0x00,0x00,
		0x02,0x73,0x06,0x20,0x07,0x00,0x40,0x00,0x48,0x00,0x47,0x00,
		0x20,0x00,0x00,0x02,0xa4,0x01,0x64,0x00,0x64,0x00,0x08,0x00,
		0x18,0x00,0x28,0x00,0x8c,0x00,0x00,0x00,0x40,0x00,0x00,0x00,
		0x20,0x4e,0x00,0x00,0x32,0x00,0x00,0x00
	};
	uint8_t actual[AON_SIZE];
	encode_aon(actual);
	assert(!memcmp(actual, expected, sizeof(expected)));
}

static void test_addresses(void)
{
	uint8_t addr[6];
	assert(parse_address("02:01:02:03:04:05", addr) == 0);
	assert(addr[0] == 2 && addr[5] == 5);
	assert(parse_address("A0:b1:C2:d3:E4:f5", addr) == 0);
	assert(parse_address("00:00:00:00:00:00", addr) == -1);
	assert(parse_address("FF:FF:FF:FF:FF:FF", addr) == -1);
	assert(parse_address("03:01:02:03:04:05", addr) == -1);
	assert(parse_address("02:01:02:03:04:-1", addr) == -1);
	assert(parse_address(" 2:01:02:03:04:05", addr) == -1);
	assert(parse_address("02:01:02:03:04:05x", addr) == -1);
	assert(parse_address("02:01:02:03:04", addr) == -1);
}

static void transaction_test(int mode, int expected_errno)
{
	int sv[2], status, rc, saved;
	pid_t child;
	assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);
	child = fork();
	assert(child >= 0);
	if (!child) {
		uint8_t cmd[4];
		uint8_t event[] = { 4, 0x0e, 4, 1, 0x03, 0x0c, 0 };
		close(sv[0]);
		assert(transfer(sv[1], cmd, sizeof(cmd), 0, monotonic_ms() + 1000) == 0);
		assert(!memcmp(cmd, "\x01\x03\x0c\x00", 4));
		if (mode == 1) {
			event[4] = 0x04; /* unrelated completion must be skipped */
			assert(write(sv[1], event, sizeof(event)) == sizeof(event));
			event[4] = 0x03;
		}
		if (mode == 2)
			event[6] = 0x12;
		if (mode == 3) {
			assert(write(sv[1], event, 5) == 5);
		} else if (mode == 4) {
			(void)delay_ms(COMMAND_TIMEOUT_MS + 100);
		} else if (mode == 5) {
			event[0] = 0x02;
			assert(write(sv[1], event, sizeof(event)) == sizeof(event));
		} else if (mode == 6) {
			const uint8_t rejected[] = { 4, 0x0f, 4, 0x0c, 1, 0x03, 0x0c };
			assert(write(sv[1], rejected, sizeof(rejected)) == sizeof(rejected));
		} else {
			assert(write(sv[1], event, sizeof(event)) == sizeof(event));
		}
		close(sv[1]);
		_exit(0);
	}
	close(sv[1]);
	errno = 0;
	rc = hci_command(sv[0], 0x0c03, NULL, 0);
	saved = errno;
	assert(waitpid(child, &status, 0) == child);
	assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
	assert(expected_errno ? rc == -1 && saved == expected_errno : rc == 0);
	close(sv[0]);
}


static void test_initialization_sequence(void)
{
	static const uint16_t opcodes[] = { 0x0c03, 0xfc70, 0xfc4d, 0xfc50, 0xfc51, 0xfc52 };
	static const uint8_t lengths[] = { 0, 6, 104, 1, 1, 1 };
	const uint8_t address[] = { 2, 0x11, 0x22, 0x33, 0x44, 0x55 };
	int sv[2], status;
	pid_t child;
	assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);
	child = fork();
	assert(child >= 0);
	if (!child) {
		size_t i;
		close(sv[0]);
		for (i = 0; i < sizeof(lengths); ++i) {
			uint8_t header[4], payload[255], aon[AON_SIZE];
			uint8_t response[] = { 4, 0x0e, 4, 1, opcodes[i] & 255, opcodes[i] >> 8, 0 };
			int64_t deadline = monotonic_ms() + 1000;
			assert(transfer(sv[1], header, 4, 0, deadline) == 0);
			assert(header[0] == 1 && header[1] == (opcodes[i] & 255));
			assert(header[2] == opcodes[i] >> 8 && header[3] == lengths[i]);
			assert(transfer(sv[1], payload, lengths[i], 0, deadline) == 0);
			if (i == 1)
				assert(!memcmp(payload, "\x55\x44\x33\x22\x11\x02", 6));
			if (i == 2) {
				encode_aon(aon);
				assert(!memcmp(payload, aon, sizeof(aon)));
			}
			if (i >= 3)
				assert(payload[0] == (i == 4 ? 1 : 0));
			/* Exercise partial serial reads as well as full event delivery. */
			assert(write(sv[1], response, 2) == 2);
			assert(delay_ms(5) == 0);
			assert(write(sv[1], response + 2, sizeof(response) - 2) == sizeof(response) - 2);
		}
		close(sv[1]);
		_exit(0);
	}
	close(sv[1]);
	assert(initialize_controller(sv[0], address) == 0);
	assert(waitpid(child, &status, 0) == child);
	assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
	close(sv[0]);
}

int main(void)
{
	test_aon();
	test_addresses();
	test_initialization_sequence();
	transaction_test(0, 0);
	transaction_test(1, 0);
	transaction_test(2, EREMOTEIO);
	transaction_test(3, EIO);
	transaction_test(4, ETIMEDOUT);
	transaction_test(5, EPROTO);
	transaction_test(6, EREMOTEIO);
	puts("PASS: 104-byte AON payload, address validation, full init sequence and HCI complete/mismatch/error/truncation/timeout/framing/status");
	return 0;
}
