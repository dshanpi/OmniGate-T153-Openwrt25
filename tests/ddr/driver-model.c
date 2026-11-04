/* SPDX-License-Identifier: GPL-2.0+ */
#define T153_DDR_HOST_TEST
#include "../../port/uboot/files/board/dshanpi/t153/ddr.c"
static u32 (*reader)(unsigned long);
static void (*writer)(u32, unsigned long);
static void (*delay)(unsigned long);
u32 ddr_test_read(unsigned long addr) { return reader(addr); }
void ddr_test_write(u32 value, unsigned long addr) { writer(value, addr); }
void ddr_test_delay(unsigned long usec) { delay(usec); }
void set_callbacks(void *r, void *w, void *d) { reader=r; writer=w; delay=d; }
int run_phase(int phase, int mhz)
{
	struct t153_ddr_state s;
	default_state(&s);
	s.mhz = mhz;
	switch (phase) {
	case 0: return clock_init(&s);
	case 1: controller_timing(&s); return 0;
	case 2: address_map(); return 0;
	case 3: phy_init(&s); return 0;
	case 4: return core_init(&s);
	default: return -EINVAL;
	}
}
