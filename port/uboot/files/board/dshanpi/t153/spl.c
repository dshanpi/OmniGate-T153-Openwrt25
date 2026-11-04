// SPDX-License-Identifier: GPL-2.0+
/* Source-only T153 SPL. SRAM execution until DDR training has succeeded. */
#include <config.h>
#include <cpu_func.h>
#include <hang.h>
#include <init.h>
#include <spl.h>
#include <sunxi_image.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/errno.h>
#include "ddr.h"

DECLARE_GLOBAL_DATA_PTR;

/* Must be initialized data: saved by assembly before the BSS is cleared. */
u32 t153_fel_context[5] __section(".data") = { 0, 0, 0, 0, 0 };
extern void t153_return_to_fel(void) __attribute__((noreturn));

void board_init_f(ulong dummy)
{
	int ret;
	u32 frequency = 24000000;

	board_early_init_f();
	/* ARM generic counter control, using the board's 24 MHz oscillator. */
	setbits_le32((void *)0x08120000, 1);
	asm volatile("mcr p15, 0, %0, c14, c0, 0" : : "r" (frequency));
	timer_init();
	ret = spl_early_init();
	if (ret)
		hang();
	preloader_console_init();
	puts("T153 source SPL: no vendor DDR library\n");
	ret = t153_ddr_init();
	if (ret) {
		printf("DDR failed (%d); stopping in SRAM\n", ret);
		hang();
	}
	gd->ram_size = T153_DDR_SIZE_BYTES;
}

u32 spl_boot_device(void)
{
	struct boot_file_head *head = (void *)0x40000;

	if (memcmp(head->magic, BOOT0_MAGIC, 8))
		return BOOT_DEVICE_BOARD;
	/* Only the standard 8 KiB card location is supported in this image. */
	switch (readb(&head->boot_media)) {
	case 0: return BOOT_DEVICE_MMC1;
	case 2: return BOOT_DEVICE_MMC2;
	default:
		printf("SPL boot medium %u unsupported\n", readb(&head->boot_media));
		hang();
	}
}

u32 spl_mmc_boot_mode(struct mmc *mmc, const u32 boot_device)
{
	return MMCSD_MODE_RAW;
}

static int load_fel(struct spl_image_info *image, struct spl_boot_device *device)
{
	printf("DDR ready; returning to FEL (sp=%08x lr=%08x)\n",
	       t153_fel_context[0], t153_fel_context[1]);
	icache_disable();
	t153_return_to_fel();
}
SPL_LOAD_IMAGE_METHOD("FEL", 0, BOOT_DEVICE_BOARD, load_fel);
