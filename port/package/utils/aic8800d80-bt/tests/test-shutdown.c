// SPDX-License-Identifier: GPL-2.0-or-later
/* Exercise the production shutdown function without Bluetooth hardware.
 * Run from the package directory:
 * cc -std=c11 -O2 -Wall -Wextra -Werror tests/test-shutdown.c \
 *   -Wl,--wrap=socket,--wrap=ioctl,--wrap=close -o /tmp/test-bt-shutdown
 * /tmp/test-bt-shutdown
 * Only shutdown_controller() is executed; helper_main() is never called.
 */
#define main helper_main
#include "../src/aic8800d80-bt.c"
#undef main
#include <assert.h>
#include <stdarg.h>
#include <sys/socket.h>

#define MOCK_FD 73
#define MAX_IOCTL_CALLS 8

static struct {
	int expected_index;
	int socket_error;
	int ioctl_errors[MAX_IOCTL_CALLS];
	size_t expected_ioctl_calls;
	int close_error;
	unsigned int socket_calls;
	size_t ioctl_calls;
	unsigned int close_calls;
	char calls[MAX_IOCTL_CALLS + 3];
	size_t call_count;
} mock;

static void record_call(char call)
{
	assert(mock.call_count + 1 < sizeof(mock.calls));
	mock.calls[mock.call_count++] = call;
	mock.calls[mock.call_count] = '\0';
}

int __wrap_socket(int domain, int type, int protocol)
{
	record_call('S');
	assert(++mock.socket_calls == 1);
	assert(domain == AF_BLUETOOTH);
	assert(type == (SOCK_RAW | SOCK_CLOEXEC));
	assert(protocol == BTPROTO_HCI);
	if (mock.socket_error) {
		errno = mock.socket_error;
		return -1;
	}
	return MOCK_FD;
}

int __wrap_ioctl(int fd, unsigned long request, ...)
{
	va_list ap;
	int index, error;

	record_call('I');
	assert(mock.socket_calls == 1 && !mock.socket_error);
	assert(mock.close_calls == 0);
	assert(fd == MOCK_FD);
	assert(request == HCIDEVDOWN);
	va_start(ap, request);
	index = va_arg(ap, int);
	va_end(ap);
	assert(index == mock.expected_index);
	assert(mock.ioctl_calls < mock.expected_ioctl_calls);
	error = mock.ioctl_errors[mock.ioctl_calls++];
	if (error) {
		errno = error;
		return -1;
	}
	return 0;
}

int __wrap_close(int fd)
{
	record_call('C');
	assert(mock.socket_calls == 1 && !mock.socket_error);
	assert(fd == MOCK_FD);
	assert(++mock.close_calls == 1);
	assert(mock.ioctl_calls == mock.expected_ioctl_calls);
	/* Deliberately clobber errno to check the original ioctl error survives. */
	errno = mock.close_error ? mock.close_error : EDOM;
	return mock.close_error ? -1 : 0;
}

static void run_case(const char *name, int index, int socket_error,
		     const int *ioctl_errors, size_t ioctl_calls,
		     int close_error, int expected_result, int expected_errno,
		     const char *expected_calls)
{
	int result, saved_errno;

	memset(&mock, 0, sizeof(mock));
	assert(ioctl_calls <= MAX_IOCTL_CALLS);
	mock.expected_index = index;
	mock.socket_error = socket_error;
	mock.expected_ioctl_calls = ioctl_calls;
	mock.close_error = close_error;
	if (ioctl_calls)
		memcpy(mock.ioctl_errors, ioctl_errors,
		       ioctl_calls * sizeof(mock.ioctl_errors[0]));
	errno = 0;
	result = shutdown_controller(index);
	saved_errno = errno;
	assert(result == expected_result);
	if (expected_result < 0)
		assert(saved_errno == expected_errno);
	assert(mock.socket_calls == 1);
	assert(mock.ioctl_calls == ioctl_calls);
	assert(mock.close_calls == (socket_error ? 0u : 1u));
	assert(!strcmp(mock.calls, expected_calls));
	printf("PASS %s (calls=%s)\n", name, mock.calls);
}

int main(void)
{
	static const int success[] = { 0 };
	static const int interrupted[] = { EINTR, EINTR, 0 };
	static const int disappeared[] = { ENODEV };
	static const int interrupted_disappeared[] = { EINTR, ENODEV };
	static const int denied[] = { EPERM };
	static const int timed_out[] = { ETIMEDOUT };
	static const int interrupted_failed[] = { EINTR, EIO };

	run_case("hci0 down", 0, 0, success, 1, 0, 0, 0, "SIC");
	run_case("exact nonzero controller index", 7, 0, success, 1, 0,
		 0, 0, "SIC");
	run_case("retry interrupted ioctl only", 31, 0, interrupted, 3, 0,
		 0, 0, "SIIIC");
	run_case("already removed controller", 7, 0, disappeared, 1, 0,
		 0, 0, "SIC");
	run_case("removed during retry", 7, 0, interrupted_disappeared, 2, 0,
		 0, 0, "SIIC");
	run_case("permission failure preserved across close", 7, 0, denied, 1, 0,
		 -1, EPERM, "SIC");
	run_case("timeout preserved despite close failure", 7, 0, timed_out, 1,
		 EIO, -1, ETIMEDOUT, "SIC");
	run_case("non-EINTR failure stops retries", 7, 0, interrupted_failed, 2,
		 0, -1, EIO, "SIIC");
	run_case("socket permission failure has no ioctl or close", 7, EPERM,
		 NULL, 0, 0, -1, EPERM, "S");
	run_case("socket fd exhaustion has no ioctl or close", 7, EMFILE,
		 NULL, 0, 0, -1, EMFILE, "S");
	puts("All shutdown lifecycle tests passed.");
	return 0;
}
