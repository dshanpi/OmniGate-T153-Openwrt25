/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef T153_DDR_H
#define T153_DDR_H
/* Single rank, x16, 10 columns, 4 banks, 2 BG; 15 or 16 row bits.
 * Physical geometry is independent of the intentionally capped Linux RAM.
 */
#ifndef CONFIG_T153_DDR_SIZE_MIB
#define CONFIG_T153_DDR_SIZE_MIB 512
#endif
#if CONFIG_T153_DDR_SIZE_MIB != 512 && CONFIG_T153_DDR_SIZE_MIB != 1024
#error "T153 DDR4 supports only the verified 512/1024 MiB geometries"
#endif
#define T153_DDR_SIZE_MIB CONFIG_T153_DDR_SIZE_MIB
#define T153_DDR_SIZE_BYTES (T153_DDR_SIZE_MIB * 1024UL * 1024UL)
#define T153_DDR_LAST_ADDRESS_BIT (T153_DDR_SIZE_MIB == 1024 ? 29 : 28)
struct t153_ddr_state {
	unsigned int mhz;
	unsigned short mr[7];
	unsigned char rdq[16], wdq[16];
	unsigned char rd[2], wr[2], rxen[2], vref[2];
	unsigned char ac, wdqs;
};
int t153_ddr_init(void);
#endif
