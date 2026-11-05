// SPDX-License-Identifier: GPL-2.0+
/*
 * T153 OmniGate fixed-geometry DDR4 initialization, reconstructed from
 * register-level observations of the board SDK. No vendor object or
 * executable byte array is used. See docs/DDR-SOURCE.txt for provenance,
 * assumptions and the limits of host-side validation.
 *
 * All training executes in SRAM with the data cache disabled. Test areas
 * are disposable: this entry point is ONLY for pre-DDR cold/FEL SPL.
 */
#ifdef T153_DDR_HOST_TEST
#include "ddr-test-io.h"
#else
#include <config.h>
#include <asm/io.h>
#include <asm/barriers.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/bitops.h>
#include <stdio.h>
#include <string.h>
#endif
#include "ddr.h"

#define COM 0x03110000UL
#define CTL 0x03120000UL
#define PHY 0x03130000UL
#define DDR_CLK 0x02002588UL
#define DDR_BGR 0x02002c0cUL
#define RTC_DDR 0x070901f4UL
#define DX(n) (PHY + 0x4000 + (n) * 0x1000)
#define TIMEOUT_US 10000

/* 'keep' is the mask of bits NOT modified, matching reset-independent RMW. */
static void update(unsigned long addr, u32 keep, u32 value)
{
	writel((readl(addr) & keep) | value, addr);
}

static int await(unsigned long addr, u32 mask, u32 value)
{
	unsigned int n;

	for (n = 0; n < TIMEOUT_US; n++) {
		if ((readl(addr) & mask) == value)
			return 0;
		udelay(1);
	}
	printf("DDR timeout: %08lx mask=%08x expected=%08x actual=%08x\n",
	       addr, mask, value, readl(addr));
	return -ETIMEDOUT;
}

static int clock_init(struct t153_ddr_state *s)
{
	u32 pll = s->mhz * (s->mhz < 600 ? 4 : 2);

	update(RTC_DDR, ~0U, BIT(2));
	update(DDR_CLK, ~BIT(31), BIT(27));
	update(DDR_CLK, ~BIT(30), BIT(27));
	udelay(1);
	update(DDR_BGR, ~BIT(0), 0);
	update(DDR_BGR, ~BIT(16), 0);
	udelay(1);
	update(DDR_CLK, 0xf8ffffff, 0x09000000);
	udelay(10);
	update(DDR_BGR, ~0U, BIT(16));
	update(DDR_CLK, ~0U, BIT(30) | BIT(27));
	udelay(10);
	update(DDR_BGR, ~0U, 1);
	update(DDR_CLK, ~0U, BIT(31) | BIT(27));
	update(PHY + 0x20, ~0U, BIT(5));
	update(PHY + 0x30, ~0U, BIT(1));
	update(PHY + 0x202c, ~0U, BIT(1));
	update(PHY + 0x30, ~0U, BIT(16));
	udelay(10);
	/* 24 MHz reference; integer N plus 19-bit fractional multiplier. */
	update(PHY + 4, 0x8000000f,
	       (pll / 24) << 4 | (((pll % 24) << 19) / 24) << 12);
	update(PHY + 0x14, 0xff00ffff, 0xa7000000);
	update(PHY + 0x14, ~0U, 1);
	update(PHY, ~0U, 0xa0000000);
	update(PHY + 0x200, ~0U, 0xa0000000);
	update(PHY, ~0U, BIT(27));
	update(PHY + 0x200, ~0U, BIT(27));
	if (await(PHY, BIT(28), BIT(28)))
		return -ETIMEDOUT;
	update(PHY + 0x1c, 0xfffffcf8, 0x88000000 | (s->mhz < 600));
	update(COM + 0x500, 0xffffff0f, 0x02000030);
	writel(0x8000, COM + 0x0c);
	udelay(10);
	return 0;
}

/* The two supported frequency profiles. Offsets are DDRC DRAMTMG0..14. */
static const u16 timing_offsets[] = {
	0x100, 0x104, 0x108, 0x10c, 0x110, 0x114, 0x118,
	0x120, 0x124, 0x128, 0x12c, 0x130, 0x134, 0x138,
};
static const u32 timing[2][14] = {
	{ 0x0c091009, 0x0002030c, 0x05050309, 0x0000400c,
	  0x04030204, 0x03030302, 0x02020004, 0x01011002,
	  0x00020208, 0x000e0c05, 0x440c021c, 8, 0x0a100002, 4 },
	{ 0x0e111f10, 0x00030417, 0x0507050b, 0x0000400c,
	  0x07030407, 0x05050403, 0x02020005, 0x02021005,
	  0x00020309, 0x000e0c05, 0x440c021c, 8, 0x0a100002, 4 },
};
static const u16 bank_offsets[] = { 0x4000, 0x3000, 0x2000, 0 };

static void controller_timing(struct t153_ddr_state *s)
{
	unsigned int i, j, profile = s->mhz == 933;
	unsigned long bank;

	s->mr[0] = profile ? 0x524 : 0x104;
	s->mr[2] = profile ? 8 : 0;
	for (i = 0; i < 4; i++) {
		bank = CTL + bank_offsets[i];
		for (j = 0; j < 14; j++)
			writel(timing[profile][j], bank + timing_offsets[j]);
		update(CTL + 0xd0, 0x3ffff000, 0x187);
		writel(0x01f20000, CTL + 0xd4);
		update(CTL + 0xd8, 0xffff00f0, 0x1705);
		writel(0, CTL + 0x1b0);
		writel(s->mr[0] << 16 | s->mr[1], bank + 0xdc);
		writel(s->mr[2] << 16 | s->mr[3], bank + 0xe0);
		writel(s->mr[4] << 16 | s->mr[5], bank + 0xe8);
		writel(s->mr[6], bank + 0xec);
		writel(0, CTL + 0x1b0);
		update(bank + 0xf4, 0xfffff00f, 0xccf);
		writel(profile ? 0x028c8406 : 0x02888405, bank + 0x190);
		writel(0x00100202, bank + 0x194);
		writel(profile ? 0xc06 : 0x805, bank + 0x1b4);
		writel(profile ? 0x003800a4 : 0x001d0054, bank + 0x64);
	}
}

static void address_map(void)
{
	/* HIF x16 single-rank address map. SDK para1 0x60fa/0x610a
	 * differs only in ADDRMAP6 row15: disabled (0xf) vs HIF offset 6.
	 * Verified by host execution of the reference address-map routine. */
	static const u32 map[9] = {
		0x1f1f, 0x003f0808, 0x01010100, 0x1f010101, 0x1f1f,
		0x06060606,
		T153_DDR_SIZE_MIB == 1024 ? 0x06060606 : 0x0f060606,
		0x0f0f, 0x3f01,
	};
	unsigned int i;

	writel(0x80000001, COM);
	writel(0x80000001, COM + 4);
	for (i = 0; i < 9; i++)
		writel(map[i], CTL + 0x200 + 4 * i);
}

static int controller_init(struct t153_ddr_state *s)
{
	unsigned int i;

	update(CTL + 0x250, 0xffff00f7, 0x300c);
	update(CTL + 0x254, 0x88ffffff, 0x330000ff);
	update(CTL + 0x270, 0xffff0000, 0x110);
	update(CTL + 0x264, 0x00ff0000, 0x1f000030);
	update(COM + 8, 0xff00ffff, 0x1f0000);
	writel(0, CTL + 0x38);
	/* DDR4, BL8, single rank, half-width, 2T command timing. */
	writel(0xc1041410, CTL);
	writel(0x201, CTL + 0x244);
	for (i = 0; i < 4; i++)
		writel(0x06000400, CTL + bank_offsets[i] + 0x240);
	address_map();
	controller_timing(s);
	update(CTL + 0x1a0, 0xffffff00, 0xc0000003);
	for (i = 0; i < 4; i++)
		update(CTL + bank_offsets[i] + 0x180, ~0U, 0xc0000000);
	update(CTL + 0x60, ~1U, 1);
	writel(0, CTL + 0x320);
	update(CTL + 0x1b0, ~1U, 0);
	writel(1, CTL + 0x320);
	if (await(CTL + 0x324, 1, 1))
		return -ETIMEDOUT;
	update(COM + 0x0c, ~0U, 0x100);
	udelay(5);
	return 0;
}

static u32 packed(const u8 *a)
{
	return a[0] | a[1] << 8 | a[2] << 16 | a[3] << 24;
}

static void latch_delays(void)
{
	update(PHY + 0x2000, ~0U, 4);
	update(PHY + 0x2000, ~4U, 0);
	update(PHY + 0xc4, ~0x80U, 0);
	update(PHY + 0xc4, ~0U, 0x80);
}

static void set_delays(const struct t153_ddr_state *s)
{
	unsigned int lane;
	unsigned long b;

	for (lane = 0; lane < 2; lane++) {
		b = DX(lane);
		writel(packed(s->rdq + lane * 8), b + 0x70);
		writel(packed(s->rdq + lane * 8 + 4), b + 0x74);
		writel(0x20, b + 0x78);
		writel(packed(s->wdq + lane * 8), b + 0x60);
		writel(packed(s->wdq + lane * 8 + 4), b + 0x64);
		writel((s->wdq[lane * 8] & 0x3f) | 0x1c00, b + 0x68);
		update(b + 0x40, 0xfffffe00, s->wdqs);
		update(b + 0x44, 0xfffffe00, s->wr[lane]);
		writel(s->rxen[lane] << 1, b + 0x48);
		update(b + 0x4c, 0xffffffc0, s->rd[lane]);
		update(b + 0x50, 0xffffffc0, s->rd[lane]);
	}
	latch_delays();
}

static void set_read_vref(const struct t153_ddr_state *s)
{
	unsigned int lane;

	for (lane = 0; lane < 4; lane++)
		update(PHY + 0x2010 + lane * 4, 0xf80fff8e,
		       (lane < 2 ? s->vref[lane] : 0x40) << 20 | 0x30);
	udelay(5);
}

static void phy_init(struct t153_ddr_state *s)
{
	/* Board CA wiring, packed as four 8-bit selectors per register. */
	static const u8 remap[28] = {
		11, 10, 0, 14, 5, 2, 13, 8, 20, 4, 19, 24, 21, 6,
		9, 1, 16, 3, 22, 17, 18, 7, 15, 23, 12, 25, 0, 0,
	};
	unsigned int i, j;
	unsigned long b;

	writel(0x01ff2f70, PHY + 0x20);
	udelay(10);
	writel(0x01ff6f70, PHY + 0x20);
	udelay(10);
	update(PHY + 0x30, ~0U, 0x100f7);
	udelay(10);
	update(PHY + 0x202c, ~0U, 2);
	update(PHY + 0x80, ~0U, 2);
	update(PHY + 0x80, 0xffffff0f, 4 << 4);
	update(PHY + 0x2024, 0xbfff1fff, 6 << 13);
	for (i = 0; i < 4; i++)
		update(DX(i) + 0x28, 0xffe0c000, 9 << 16 | 0x3f0);
	update(PHY + 0xc4, ~0U, 0x60);
	update(PHY + 0x3064, 0xfffff801, 0x731);
	udelay(25000);
	set_read_vref(s);
	writel(0x3fffffff, PHY + 0x50);
	for (i = 0; i < 7; i++)
		writel(packed(remap + i * 4), PHY + 0x90 + i * 4);
	for (i = 0; i < 4; i++) {
		b = PHY + 0x3000 + i * 0x400;
		update(b, 0xfcf00fff, 0x021cc000);
		update(b, ~0U, 3);
		update(b + 8, 0xfff00fff, 0x001cc000);
		update(b + 8, ~0U, 3);
		update(b + 4, 0xfff00fff, 0x001cc000);
		update(b + 4, ~0U, 3);
	}
	for (j = 0; j < 4; j++) {
		for (i = 0; i < 4; i++) {
			b = DX(i) + j * 0x400;
			update(b, 0xfcf00fff, 0x02500000 | (i < 2 ? 0xcc000 : 0));
			update(b, ~0U, 3);
			update(b + 4, 0xf8ff00ff, 0x03000000 | (i < 2 ? 0x8000 : 0));
		}
	}
	writel(0x3210, PHY + 0xb0);
	writel(0xf, PHY + 0x54);
	for (i = 0; i < 4; i++) {
		b = PHY + 0x3000 + i * 0x400;
		update(b + 0x24, 0xffffe0ce, 0);
		update(b + 0x20, 0xffff80ff, s->ac << 8);
	}
	set_delays(s);
	update(PHY + 0x202c, ~0U, 0x20);
	udelay(100);
	update(PHY + 0x202c, ~0x20U, 0);
	writel(0x1f7f7, PHY + 0x30);
	udelay(100);
	writel(0x01ff6f70, PHY + 0x20);
	udelay(100);
}

static int mr_write(unsigned int mr, u16 value)
{
	if (await(CTL + 0x18, 1, 0))
		return -ETIMEDOUT;
	writel(value, CTL + 0x14);
	writel(mr << 12 | 0xf0, CTL + 0x10);
	udelay(10);
	writel(mr << 12 | 0x800000f0, CTL + 0x10);
	udelay(10);
	return await(CTL + 0x10, BIT(31), 0);
}

static int set_write_vref(u16 mr6)
{
	if (mr_write(6, mr6 | 0x80))
		return -ETIMEDOUT;
	udelay(10);
	if (mr_write(6, mr6 | 0x80))
		return -ETIMEDOUT;
	udelay(10);
	return mr_write(6, mr6 & ~0x80);
}

static int dfi_init(struct t153_ddr_state *s)
{
	unsigned int i;

	writel(0, CTL + 0x320);
	update(CTL + 0x1b0, ~0U, 1);
	update(CTL + 0x1b0, ~0U, 0x20);
	writel(1, CTL + 0x320);
	if (await(CTL + 0x324, 1, 1) || await(CTL + 0x1bc, 1, 1))
		return -ETIMEDOUT;
	update(PHY + 0x202c, ~0U, 5);
	update(PHY + 0x202c, ~4U, 0);
	update(RTC_DDR, ~0U, 4);
	writel(0, CTL + 0x320);
	update(CTL + 0x1b0, ~0x20U, 0);
	update(CTL + 0x30, ~0x20U, 0);
	writel(1, CTL + 0x320);
	if (await(CTL + 0x324, 1, 1) || await(CTL + 4, 3, 1))
		return -ETIMEDOUT;
	writel(0, CTL + 0x320);
	update(CTL + 0x1b0, ~1U, 0);
	writel(1, CTL + 0x320);
	if (await(CTL + 0x324, 1, 1))
		return -ETIMEDOUT;
	for (i = 0; i < 6; i++)
		if (mr_write(i, s->mr[i]))
			return -ETIMEDOUT;
	if (set_write_vref(s->mr[6]))
		return -ETIMEDOUT;
	writel(0, CTL + 0x320);
	update(CTL + 0x60, ~1U, 0);
	writel(1, CTL + 0x320);
	return await(CTL + 0x324, 1, 1);
}

static int rxen_train(struct t153_ddr_state *s)
{
	unsigned int i, trp = (15 * (s->mhz / 2) + 999) / 1000;
	int ret;
	u32 result;

	update(PHY + 0x20, ~0U, 0x1000);
	update(PHY + 0x30, ~0U, 0x101);
	update(PHY + 0xc4, ~0U, 4);
	if (readl(PHY + 0x80) & 2)
		update(PHY + 0x1030, ~0U, 0x10000);
	update(PHY + 0x1030, 0xfffffff0, 2);
	for (i = 0; i < 5; i++)
		update(PHY + 0x101c + 4 * i, 0xffff0000, s->mr[i]);
	update(PHY + 0x1048, ~0U, trp << 24);
	update(PHY + 0x104c, 0xffff0000, 4 | (trp < 12 ? 12 : trp) << 8);
	update(PHY + 0x1054, 0x00ffffff, 3 << 24);
	update(PHY + 0x1064, 0xffffff00, trp + 1);
	writel(0, PHY + 0x1080);
	update(PHY + 0x107c, 0xffffc0ff, (s->mhz == 933 ? 12 : 8) << 8);
	update(PHY + 0x103c, 0xffff0000, 0x221f2e40);
	update(PHY + 0x1080, 0x0000ffff, 0x00320000);
	update(PHY + 0x80, ~0U, 1);
	writel(0x301, PHY + 0x1034); /* rank 0, byte lanes 0 and 1 */
	update(PHY + 0x1034, ~0U, 0x01100000);
	ret = await(PHY + 0x1034, BIT(31), BIT(31));
	if (!ret && (readl(PHY + 0x1040) & 3))
		ret = -EIO;
	if (!ret) {
		result = readl(PHY + 0x10d0);
		s->rxen[0] = result >> 1;
		s->rxen[1] = result >> 17;
	}
	/* Leave training mode even after a timeout or an error result. */
	update(PHY + 0x1034, 0xfeefffff, 0);
	update(PHY + 0x80, ~1U, 0);
	update(PHY + 0x30, 0xfffffefe, 0);
	update(PHY + 0x1034, 0xfeeff0f0, 0);
	update(PHY + 0x20, ~0x1000U, 0);
	for (i = 0; i < 4; i++)
		update(DX(i) + 0x28, 0xffff80ff, 0x300);
	return ret;
}

static int core_init(struct t153_ddr_state *s)
{
	unsigned int i;
	int ret;

	ret = clock_init(s);
	if (ret)
		return ret;
	ret = controller_init(s);
	if (ret)
		return ret;
	phy_init(s);
	ret = dfi_init(s);
	if (ret)
		return ret;
	for (i = 0; i < 6; i++) {
		ret = rxen_train(s);
		if (!ret)
			break;
		if (ret == -ETIMEDOUT)
			return ret;
	}
	if (ret)
		return ret;
	set_delays(s);
	update(0x03606014, ~0U, BIT(31));
	udelay(10);
	return 0;
}

static void default_state(struct t153_ddr_state *s)
{
	static const struct t153_ddr_state defaults = {
		.mhz = 933, .mr = { 0x520, 0x101, 8, 0, 0, 0x400, 0x81a },
		.rdq = { 0x12, 0x14, 0x14, 0x14, 0x1f, 0x18, 0x1f, 0x18,
			 0x16, 0x13, 0x22, 0x11, 0x12, 0x12, 0x10, 0x11 },
		.wdq = { 0x16, 0x16, 0x16, 0x18, 0x21, 0x1b, 0x21, 0x1e,
			 0x1d, 0x16, 0x23, 0x12, 0x18, 0x12, 0x16, 0x14 },
		.rd = { 0x20, 0x20 }, .wr = { 0x50, 0x50 },
		.vref = { 0x40, 0x40 }, .ac = 0x25, .wdqs = 0x60,
	};

	*s = defaults;
}

#define TEST_WORDS 256
#define TEST0 0x40000000UL
#define TEST1 0x50000000UL

static u32 pattern(unsigned int i, u32 salt)
{
	u32 v = (i ^ salt) * 1664525U + 1013904223U;

	v ^= v >> 13;
	return v ^ BIT(i & 31) ^ (i & 1 ? 0xaaaaaaaa : 0x55555555);
}

static void pattern_write(u32 salt)
{
	unsigned int i;

	for (i = 0; i < TEST_WORDS; i++) {
		writel(pattern(i, salt), TEST0 + i * 4);
		writel(~pattern(i, salt), TEST1 + i * 4);
	}
	dsb();
}

static int pattern_check(u32 mask, u32 salt)
{
	unsigned int i;
	u32 expected;

	for (i = 0; i < TEST_WORDS; i++) {
		expected = pattern(i, salt);
		if (((readl(TEST0 + i * 4) ^ expected) & mask) ||
		    ((readl(TEST1 + i * 4) ^ ~expected) & mask))
			return -EIO;
	}
	return 0;
}

static int rw_check(u32 mask)
{
	pattern_write(0x12345678);
	if (pattern_check(mask, 0x12345678))
		return -EIO;
	pattern_write(0x87654321);
	return pattern_check(mask, 0x87654321);
}

/* Longest contiguous passing interval, including a run at the last sample. */
static unsigned int longest_run(u64 bits, unsigned int count, unsigned int *mid)
{
	unsigned int i, start = 0, best = 0, width;

	for (i = 0; i <= count; i++) {
		if (i < count && (bits & (1ULL << i)))
			continue;
		width = i - start;
		if (width > best) {
			best = width;
			*mid = start + (width - 1) / 2;
		}
		start = i + 1;
	}
	return best;
}

/* Choose the center of an all-pass rectangle, not a single lucky sample. */
static int read_eye(struct t153_ddr_state *s, unsigned int lane)
{
	u64 eye[31], common;
	unsigned int v, d, lo, hi, mid = 0, width, height, margin, score;
	unsigned int best = 0, best_v = 0, best_d = 0;
	u32 mask = 0x00ff00ffU << (lane * 8);

	for (v = 0; v < 31; v++) {
		s->vref[lane] = 4 + 4 * v;
		set_read_vref(s);
		eye[v] = 0;
		for (d = 0; d < 32; d++) {
			s->rd[lane] = d * 2;
			set_delays(s);
			if (!pattern_check(mask, 0))
				eye[v] |= 1ULL << d;
		}
	}
	for (lo = 0; lo < 31; lo++) {
		common = ~0ULL;
		for (hi = lo; hi < 31; hi++) {
			common &= eye[hi];
			width = longest_run(common, 32, &mid);
			height = hi - lo + 1;
			if (width < 4 || height < 3)
				continue;
			margin = width * 2 < height * 4 ? width * 2 : height * 4;
			score = margin * 10000 + width * height;
			if (score > best) {
				best = score;
				best_v = (lo + hi) / 2;
				best_d = mid;
			}
		}
	}
	if (!best) {
		printf("DDR read eye missing/narrow on lane %u\n", lane);
		return -EIO;
	}
	s->vref[lane] = 4 + 4 * best_v;
	s->rd[lane] = 2 * best_d;
	set_read_vref(s);
	set_delays(s);
	printf("DDR lane %u: read Vref=%u delay=%u\n",
	       lane, s->vref[lane], s->rd[lane]);
	return pattern_check(mask, 0);
}

static int write_eye(struct t153_ddr_state *s)
{
	u64 eye[26][2], common[2];
	unsigned int v, d, lane, lo, hi, width[2], mid[2] = { 0, 0 };
	unsigned int height, narrow, margin, score, best = 0, best_v = 0;
	unsigned int best_d[2] = { 0, 0 };
	u8 saved[2] = { s->wr[0], s->wr[1] };
	u16 mr6 = s->mr[6] & ~0x3f;

	/* DDR4 write Vref is rank-wide, so require a shared window for both lanes. */
	for (v = 0; v < 26; v++) {
		if (set_write_vref(mr6 | v * 2))
			return -ETIMEDOUT;
		for (lane = 0; lane < 2; lane++) {
			eye[v][lane] = 0;
			for (d = 0; d < 57; d++) {
				s->wr[lane] = 64 + 2 * d;
				set_delays(s);
				if (!rw_check(0x00ff00ffU << (lane * 8)))
					eye[v][lane] |= 1ULL << d;
			}
			s->wr[lane] = saved[lane];
			set_delays(s);
		}
	}
	for (lo = 0; lo < 26; lo++) {
		common[0] = common[1] = ~0ULL;
		for (hi = lo; hi < 26; hi++) {
			for (lane = 0; lane < 2; lane++) {
				common[lane] &= eye[hi][lane];
				width[lane] = longest_run(common[lane], 57, &mid[lane]);
			}
			narrow = width[0] < width[1] ? width[0] : width[1];
			height = hi - lo + 1;
			if (narrow < 4 || height < 3)
				continue;
			margin = narrow < height ? narrow : height;
			score = margin * 10000 + narrow * height;
			if (score > best) {
				best = score;
				best_v = (lo + hi) / 2;
				best_d[0] = mid[0];
				best_d[1] = mid[1];
			}
		}
	}
	if (!best) {
		puts("DDR write eye missing/narrow across the two lanes\n");
		return -EIO;
	}
	s->mr[6] = mr6 | best_v * 2;
	for (lane = 0; lane < 2; lane++)
		s->wr[lane] = 64 + best_d[lane] * 2;
	if (set_write_vref(s->mr[6]))
		return -ETIMEDOUT;
	set_delays(s);
	printf("DDR write Vref=%u delays=%u,%u\n",
	       s->mr[6] & 0x3f, s->wr[0], s->wr[1]);
	return rw_check(~0U);
}

static int bit_deskew(struct t153_ddr_state *s, int write)
{
	u64 pass;
	u8 *skew = write ? s->wdq : s->rdq;
	unsigned int bit, delay, mid = 0, width;
	u32 mask;

	if (!write) {
		pattern_write(0);
		if (pattern_check(~0U, 0))
			return -EIO;
	}
	for (bit = 0; bit < 16; bit++) {
		pass = 0;
		mask = BIT(bit) | BIT(bit + 16);
		for (delay = 0; delay < 64; delay++) {
			skew[bit] = delay;
			set_delays(s);
			if (!(write ? rw_check(mask) : pattern_check(mask, 0)))
				pass |= 1ULL << delay;
		}
		width = longest_run(pass, 64, &mid);
		if (width < 6) {
			printf("DDR %s DQ%u eye too narrow: %u taps\n",
			       write ? "write" : "read", bit, width);
			return -EIO;
		}
		skew[bit] = mid;
		set_delays(s);
	}
	return write ? rw_check(~0U) : pattern_check(~0U, 0);
}

static int memory_verify(void)
{
	unsigned int bank, i, pass;
	unsigned long b;
	u32 expected;

	/* Address-line alias check covers the configured physical geometry. */
	writel(0x12345678, TEST0);
	for (i = 2; i <= T153_DDR_LAST_ADDRESS_BIT; i++)
		writel(0xa5a50000 | i, TEST0 + (1UL << i));
	dsb();
	if (readl(TEST0) != 0x12345678)
		return -EIO;
	for (i = 2; i <= T153_DDR_LAST_ADDRESS_BIT; i++)
		if (readl(TEST0 + (1UL << i)) != (0xa5a50000 | i))
			return -EIO;
	/* Walking-one/zero patterns plus address-dependent data, in every 64 MiB. */
	for (bank = 0; bank < T153_DDR_SIZE_MIB / 64; bank++) {
		b = TEST0 + bank * 0x04000000UL + 0x10000;
		for (pass = 0; pass < 4; pass++) {
			for (i = 0; i < 16384; i++) {
				expected = pass < 2 ? BIT(i & 31) : pattern(i, b);
				writel(pass & 1 ? ~expected : expected, b + i * 4);
			}
			dsb();
			for (i = 0; i < 16384; i++) {
				expected = pass < 2 ? BIT(i & 31) : pattern(i, b);
				if (pass & 1)
					expected = ~expected;
				if (readl(b + i * 4) != expected) {
					printf("DDR memory mismatch at %08lx\n", b + i * 4);
					return -EIO;
				}
			}
		}
	}
	return 0;
}

int t153_ddr_init(void)
{
	struct t153_ddr_state s;
	unsigned int delay;
	int ret;

	default_state(&s);
	/* Board-specific controller-access setup; supply voltage is fixed by board. */
	update(0x03000160, ~2U, 0);
	writel(0x19220000, 0x03000188);
	update(0x07010254, ~0U, 1);
	udelay(10);
	printf("DDR4 source init: %u MiB physical x16; bootstrap 480 MHz\n",
	       T153_DDR_SIZE_MIB);
	s.mhz = 480;
	s.ac = 0x20;
	for (delay = 0x16; delay <= 0x32; delay += 4) {
		s.rd[0] = s.rd[1] = delay;
		ret = core_init(&s);
		if (ret == -ETIMEDOUT)
			return ret;
		if (!ret && !rw_check(~0U))
			break;
	}
	if (delay > 0x32) {
		puts("DDR low-frequency bootstrap failed\n");
		return -EIO;
	}
	pattern_write(0);
	default_state(&s);
	puts("DDR4: 933 MHz read eye and Vref training\n");
	ret = core_init(&s);
	if (ret)
		return ret;
	if (read_eye(&s, 0) || read_eye(&s, 1))
		return -EIO;
	puts("DDR4: write eye and Vref training\n");
	ret = write_eye(&s);
	if (ret)
		return ret;
	puts("DDR4: per-bit read/write deskew\n");
	if (bit_deskew(&s, 0) || bit_deskew(&s, 1))
		return -EIO;
	/* Reapply the trained state, rerun RXEN, and reject any final mismatch. */
	ret = core_init(&s);
	if (ret)
		return ret;
	puts("DDR4: geometry and memory checks\n");
	ret = memory_verify();
	if (ret)
		return ret;
	puts("DDR4: source initialization and memory checks passed\n");
	return 0;
}
