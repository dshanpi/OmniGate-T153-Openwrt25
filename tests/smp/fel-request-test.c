// SPDX-License-Identifier: GPL-2.0-only
#define _GNU_SOURCE
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
typedef uint8_t u8;
struct boot_file_head { uint32_t branch; u8 magic[8]; u8 rest[28]; uint32_t boot_media; };
#define BOOT0_MAGIC "eGON.BT0"
static unsigned reads, writes, clear_after, barriers;
static void dsb(void) { ++barriers; }
static void isb(void) { ++barriers; }
static uint32_t rtc;
static jmp_buf escape;
static u8 readb(const void *p) { return *(const u8 *)p; }
static uint32_t readl(const void *p)
{
	assert(p == (void *)0x07090108);
	++reads;
	assert(barriers == writes * 2);
	if (writes && reads > clear_after) rtc = 0;
	return rtc;
}
static void writel(uint32_t value, void *p)
{
	assert(p == (void *)0x07090108 && value == 0);
	++writes;
}
static void hang(void) __attribute__((noreturn));
static void hang(void) { longjmp(escape, 2); }
void t153_enter_fel_fresh(void) __attribute__((noreturn));
void t153_enter_fel_fresh(void) { assert(rtc == 0); longjmp(escape, 1); }
#define T153_FEL_REQUEST_HOST_TEST
#include "../../port/uboot/files/board/dshanpi/t153/fel-request.c"
static void run(const char *magic, unsigned media, uint32_t flag,
		unsigned delay, int expected, unsigned expected_reads)
{
	struct boot_file_head *h = (void *)0x40000;
	memcpy(h->magic, magic, 8); h->boot_media = media;
	rtc = flag; reads = writes = barriers = 0; clear_after = delay;
	int result = setjmp(escape);
	if (!result) t153_handle_fel_request();
	assert(result == expected);
	assert(reads == expected_reads);
	assert(writes == (expected ? expected_reads - 1 : 0));
}
int main(void)
{
	void *p = mmap((void *)0x40000, 4096, PROT_READ|PROT_WRITE,
		MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE, -1, 0);
	assert(p == (void *)0x40000);
	run("eGON.BT0", 0, 0, 0, 0, 1);
	run("eGON.BT0", 2, 0x12345678, 0, 0, 1);
	run("eGON.FEL", 0, T153_EFEX_MAGIC, 0, 0, 0);
	run("badmagic", 2, T153_EFEX_MAGIC, 0, 0, 0);
	run("eGON.BT0", 3, T153_EFEX_MAGIC, 0, 0, 0);
	run("eGON.BT0", 0, T153_EFEX_MAGIC, 0, 1, 2);
	run("eGON.BT0", 2, T153_EFEX_MAGIC, 0, 1, 2);
	run("eGON.BT0", 2, T153_EFEX_MAGIC, 17, 1, 18);
	run("eGON.BT0", 2, T153_EFEX_MAGIC, 1001, 2, 1001);
	/* Same subtraction used by assembly, including low-word wrap. */
	assert((uint32_t)(0x00000020u - 0xfffffff0u) == 48u);
	assert(munmap(p, 4096) == 0);
	puts("PASS: actual FEL request C: media/magic gates, clear-before-jump, delayed clear, bounded failure without jump");
	return 0;
}
