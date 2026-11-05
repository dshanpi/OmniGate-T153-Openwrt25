// SPDX-License-Identifier: GPL-2.0+
/* T153 OmniGate U-Boot proper: requires initialized DDR and ARM Linux handoff. */
#include <config.h>
#include <cpu_func.h>
#include <env.h>
#include <sunxi_image.h>
#include <hang.h>
#include <init.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <stdio.h>
#include <linux/libfdt.h>
#include "t153.h"

DECLARE_GLOBAL_DATA_PTR;

void t153_pin(unsigned int bank, unsigned int pin, unsigned int pull)
{
	void *base = (void *)0x03604000 + 0x80 + bank * 0x80;
	unsigned int shift = pin % 8 * 4;

	/* UART0, SD0 and eMMC2 all use function 2 on this board. */
	clrsetbits_le32(base + pin / 8 * 4, 0xf << shift, 2 << shift);
	/* Drive field value 3: the hardware drive field is four bits wide. */
	clrsetbits_le32(base + 0x20 + pin / 8 * 4, 0xf << shift, 3 << shift);
	shift = pin % 16 * 2;
	clrsetbits_le32(base + 0x30 + pin / 16 * 4, 3 << shift, pull << shift);
}

int board_early_init_f(void)
{
	/* Oscillator source, /1; do not change CPU, DDR or peripheral PLLs. */
	clrsetbits_le32((void *)0x02002538, GENMASK(26, 24) | GENMASK(4, 0), 0);
	setbits_le32((void *)0x02002e00, BIT(16) | BIT(0));
	t153_pin(1, 9, 1);
	t153_pin(1, 10, 1);
	return 0;
}

int dram_init(void)
{
	/* Fixed 384 MiB safe window; physical DDR capacity is selected by the
	 * SPL profile. Do not destructively probe RAM from U-Boot proper. */
	gd->ram_size = CFG_SYS_SDRAM_SIZE;
	return 0;
}

int dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = CFG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = CFG_SYS_SDRAM_SIZE;
	return 0;
}

int board_init(void)
{
	gd->bd->bi_boot_params = CFG_SYS_SDRAM_BASE + 0x100;
	return 0;
}

int checkboard(void)
{
#ifdef CONFIG_TARGET_DSHANPI_T153_SOURCE
	puts("Board: T153 OmniGate (experimental source SPL/U-Boot)\n");
	#ifdef CONFIG_T153_PSCI_COLD_SMP
	puts("No vendor DDR/monitor. Four-core PSCI cold-boot profile.\n");
#else
	puts("No vendor DDR/monitor. Linux bring-up is currently CPU0 only.\n");
#endif
#else
	puts("Board: DshanPI T153MX OmniGate (experimental RAM U-Boot)\n");
	puts("DDR and secure monitor inherited; cold-boot SPL is not included.\n");
#endif
	return 0;
}

void enable_caches(void)
{
	icache_enable();
	dcache_enable();
}

#if defined(CONFIG_TARGET_DSHANPI_T153_SOURCE) && !defined(CONFIG_XPL_BUILD)
/* Only sunxi-fel's documented SPL header may request a RAM boot script.
 * Never infer FEL mode from arbitrary DRAM contents, and never fall through
 * to a persistent disk when a requested rescue boot fails.
 */
int board_late_init(void)
{
	const struct boot_file_head *head = (const void *)0x40000;

	env_set("fel_scriptaddr", NULL);
	/* Cold boot has no SRAM swap. Prefer its current, valid header over
	 * any stale FEL scratch data that might survive a warm reset. */
	if (!memcmp(head->magic, BOOT0_MAGIC, 8) &&
	    !memcmp(head->spl_signature, SPL_SIGNATURE, 3)) {
		switch (readb(&head->boot_media)) {
		case 0:
			env_set("bootdevs", "0 1");
			break;
		case 2:
			env_set("bootdevs", "1 0");
			break;
		default:
			env_set("t153_boot_source", "unknown");
			return 0;
		}
		env_set("t153_boot_source", "mmc");
		return 0;
	}
	/* The T153 FEL thunk swaps 0x40000..0x413ff with 0x66c00.
	 * After return, 0x40000 belongs to BootROM again. The host passes
	 * script information through the relocated header at 0x66c00. */
	head = (const void *)0x66c00;
	if (!memcmp(head->magic, "eGON.FEL", 8) &&
	    !memcmp(head->spl_signature, SPL_SIGNATURE, 3) &&
	    head->spl_signature[3] >= SPL_ENV_HEADER_VERSION &&
	    head->spl_signature[3] <= SPL_HEADER_VERSION) {
		env_set("t153_boot_source", "fel");
		if (!readl(&head->fel_uEnv_length) &&
		    readl(&head->fel_script_address) == 0x4f100000)
			env_set_hex("fel_scriptaddr", 0x4f100000);
		return 0;
	}
	env_set("t153_boot_source", "unknown");
	return 0;
}

/* Until the T153 CPU power controller is implemented, do not advertise
 * a factory PSCI service that does not exist in a source-only boot chain.
 */
int ft_board_setup(void *fdt, struct bd_info *bd)
{
#ifdef CONFIG_T153_PSCI_COLD_SMP
	extern int t153_smp_fixup_dt(void *fdt);

	return t153_smp_fixup_dt(fdt);
#else
	int node, cpus, ret;
	const fdt32_t *reg;

	node = fdt_path_offset(fdt, "/psci");
	if (node >= 0) {
		ret = fdt_del_node(fdt, node);
		if (ret)
			return ret;
	}
	cpus = fdt_path_offset(fdt, "/cpus");
	if (cpus < 0)
		return cpus;
	fdt_for_each_subnode(node, fdt, cpus) {
		reg = fdt_getprop(fdt, node, "reg", NULL);
		if (!reg)
			continue;
		if (fdt32_to_cpu(*reg)) {
			ret = fdt_setprop_string(fdt, node, "status", "disabled");
			if (ret)
				return ret;
		} else {
			ret = fdt_delprop(fdt, node, "enable-method");
			if (ret && ret != -FDT_ERR_NOTFOUND)
				return ret;
		}
	}
	puts("Source boot: CPU0 only; T153 PSCI/SMP bring-up is pending.\n");
	return 0;
#endif
}
#endif
