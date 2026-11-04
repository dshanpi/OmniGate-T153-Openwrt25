/* SPDX-License-Identifier: GPL-2.0+ */
/* Software-only register/eye model. This is not a model of analog hardware. */
#define T153_DDR_HOST_TEST
#include "../../port/uboot/files/board/dshanpi/t153/ddr.c"
#include <assert.h>

#define SLOTS (1U << 19)
static struct { u32 addr, value; } cells[SLOTS];
static unsigned long delayed;
static unsigned int fault, memory_writes;
/* 0 pass, 1 PLL, 2 SWSTAT, 3 DFISTAT, 4 CTLSTAT, 5 MR busy,
 * 6 MR command, 7 RXEN timeout, 8 RXEN errors, 9 no read eye,
 * 10 no write eye, 11 address alias, 12 narrow read eye. */

static unsigned int slot(u32 addr)
{
	unsigned int i = (addr * 2654435761U) >> 13;
	while (cells[i].addr && cells[i].addr != addr)
		i = (i + 1) & (SLOTS - 1);
	return i;
}
static u32 get(u32 addr)
{
	return cells[slot(addr)].value;
}
static void put(u32 addr, u32 value)
{
	unsigned int i = slot(addr);
	cells[i].addr = addr;
	cells[i].value = value;
}
static int target_rate(void)
{
	return (get(PHY + 4) & 0x7ffffff0) == 0x600004d0;
}
static u32 read_errors(void)
{
	unsigned int bit, lane, d, v, skew;
	u32 error = 0;

	if (!target_rate())
		return 0;
	for (lane = 0; lane < 2; lane++) {
		d = get(DX(lane) + 0x4c) & 63;
		v = (get(PHY + 0x2010 + lane * 4) >> 20) & 127;
		if (fault == 9 || d < 12 || d > 50 || v < 40 || v > 88 ||
		    (fault == 12 && (v < 64 || v > 68)))
			error |= 0x00ff00ffU << (lane * 8);
		for (bit = 0; bit < 8; bit++) {
			skew = (get(DX(lane) + 0x70 + (bit / 4) * 4) >> ((bit % 4) * 8)) & 255;
			if (skew < 8 || skew > 44)
				error |= BIT(lane * 8 + bit) | BIT(16 + lane * 8 + bit);
		}
	}
	return error;
}
static u32 write_errors(void)
{
	unsigned int bit, lane, d, v, skew;
	u32 error = 0;

	if (!target_rate())
		return 0;
	v = get(0x1000) & 63; /* model-only rank MR6 */
	for (lane = 0; lane < 2; lane++) {
		d = get(DX(lane) + 0x44) & 255;
		if (fault == 10 || d < 64 || d > 144 || v < 12 || v > 40)
			error |= 0x00ff00ffU << (lane * 8);
		for (bit = 0; bit < 8; bit++) {
			skew = (get(DX(lane) + 0x60 + (bit / 4) * 4) >> ((bit % 4) * 8)) & 255;
			if (skew < 8 || skew > 44)
				error |= BIT(lane * 8 + bit) | BIT(16 + lane * 8 + bit);
		}
	}
	return error;
}
u32 ddr_test_read(unsigned long addr)
{
	u32 v;
	if (addr >= 0x40000000 && addr < 0x40000000UL + T153_DDR_SIZE_BYTES) {
		if (fault == 11)
			addr &= ~0x10000000UL;
		if (fault == 13) addr &= ~0x20000000UL;
		return get(addr) ^ read_errors();
	}
	v = get(addr);
	if (addr == PHY && fault != 1 && (v & BIT(27))) v |= BIT(28);
	if (addr == CTL + 0x324) v = fault == 2 ? 0 : 1;
	if (addr == CTL + 0x1bc) v = fault == 3 ? 0 : 1;
	if (addr == CTL + 4) v = fault == 4 ? 0 : 1;
	if (addr == CTL + 0x18) v = fault == 5 ? 1 : 0;
	if (addr == CTL + 0x10 && fault != 6) v &= ~BIT(31);
	if (addr == PHY + 0x1034 && fault != 7 && (v & BIT(24))) v |= BIT(31);
	if (addr == PHY + 0x1040) v = fault == 8 ? 3 : 0;
	if (addr == PHY + 0x10d0) v = 0x00700070;
	return v;
}
void ddr_test_write(u32 value, unsigned long addr)
{
	if (addr >= 0x40000000 && addr < 0x40000000UL + T153_DDR_SIZE_BYTES) {
		memory_writes++;
		value ^= write_errors();
		if (fault == 11) addr &= ~0x10000000UL;
		if (fault == 13) addr &= ~0x20000000UL;
	}
	if (addr == CTL + 0x10 && (value & BIT(31)) && ((value >> 12) & 7) == 6)
		put(0x1000, get(CTL + 0x14));
	put(addr, value);
}
void ddr_test_delay(unsigned long usec) { delayed += usec; }
static void reset_model(unsigned int f)
{
	memset(cells, 0, sizeof(cells));
	fault=f; delayed=0; memory_writes=0;
}
int main(void)
{
	struct t153_ddr_state s;
	unsigned int i, mid = 999;

	assert(longest_run(0, 64, &mid) == 0);
	assert(longest_run(0xf000000000000000ULL, 64, &mid) == 4 && mid == 61);
	assert(longest_run(0xff00ffULL, 64, &mid) == 8 && mid == 3);
	assert(longest_run(~0ULL, 64, &mid) == 64 && mid == 31);
	for (i = 1; i <= 8; i++) {
		reset_model(i); default_state(&s);
		assert(core_init(&s) < 0);
		assert(!memory_writes && delayed < 50000);
	}
	reset_model(0);
	assert(t153_ddr_init() == 0);
	assert(memory_writes > 1000000);
	assert(get(CTL + 0x218) == (T153_DDR_SIZE_MIB == 1024 ? 0x06060606 : 0x0f060606));
	if (T153_DDR_SIZE_MIB == 1024) {
		reset_model(13);
		assert(t153_ddr_init() < 0); /* Detect upper-half alias, not only 512 MiB. */
	}
	for (i = 9; i <= 12; i++) {
		reset_model(i);
		assert(t153_ddr_init() < 0);
	}
	puts("PASS: source DDR algorithm, 8 handshake failures, missing/narrow eyes, alias rejection");
	return 0;
}
