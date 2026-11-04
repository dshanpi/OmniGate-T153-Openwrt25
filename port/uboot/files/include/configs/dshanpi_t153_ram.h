/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __DSHANPI_T153_RAM_H
#define __DSHANPI_T153_RAM_H

/* Keep the original firmware/monitor and E907's low DRAM untouched. */
#define CFG_SYS_SDRAM_BASE 0x48000000
#define CFG_SYS_SDRAM_SIZE 0x18000000
#define CFG_SYS_HZ_CLOCK 24000000

#ifdef CONFIG_TARGET_DSHANPI_T153_SOURCE
#define T153_SOURCE_ENV \
	"scriptaddr=0x4f100000\0" \
	"bootdevs=0 1\0" \
	"bootdisk=for bootdev in ${bootdevs}; do " \
	"if load mmc ${bootdev}:1 ${scriptaddr} boot.scr; then " \
	"source ${scriptaddr}; fi; done\0" \
	"t153boot=if test \"${t153_boot_source}\" = fel; then " \
	"if test -n \"${fel_scriptaddr}\"; then source ${fel_scriptaddr}; " \
	"else echo FEL mode: waiting for manual RAM boot; fi; " \
	"else if test \"${t153_boot_source}\" = mmc; then run bootdisk; " \
	"else echo Unknown boot source: waiting at prompt; fi; fi\0"
#else
#define T153_SOURCE_ENV ""
#endif

#define CFG_EXTRA_ENV_SETTINGS \
	T153_SOURCE_ENV \
	"stdin=serial\0stdout=serial\0stderr=serial\0" \
	"loadaddr=0x50000000\0" \
	"kernel_addr_r=0x48008000\0" \
	"fdt_addr_r=0x4f000000\0" \
	"ramdisk_addr_r=0x52000000\0" \
	"bootm_low=0x48000000\0bootm_size=0x18000000\0" \
	"bootfile=openwrt-t153-initramfs.itb\0" \
	"bootargs=console=ttyS0,115200 " \
	"earlycon=uart8250,mmio32,0x02600000,115200n8,24000000 " \
	"loglevel=8 ignore_loglevel clk_ignore_unused\0" \
	"bootsd=if load mmc 0:1 ${loadaddr} ${bootfile}; then " \
	"if iminfo ${loadaddr}; then bootm ${loadaddr}; fi; fi\0"
#endif
