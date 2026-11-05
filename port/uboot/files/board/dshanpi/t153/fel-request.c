// SPDX-License-Identifier: GPL-2.0+
/* Consume the vendor RTC EFEX request before DDR, from cold SD/eMMC only. */
#ifndef T153_FEL_REQUEST_HOST_TEST
#include <config.h>
#include <stdio.h>
#include <linux/string.h>
#include <hang.h>
#include <sunxi_image.h>
#include <asm/io.h>
#include <asm/barriers.h>
#endif

#define T153_RTC_EFEX_FLAG ((void *)0x07090108)
#define T153_EFEX_MAGIC 0x5aa5a55a
#define T153_EFEX_CLEAR_POLLS 1000

extern void t153_enter_fel_fresh(void) __attribute__((noreturn));

void t153_handle_fel_request(void)
{
	struct boot_file_head *head = (void *)0x40000;
	unsigned int poll;
	u8 media;

	/* The eGON.FEL USB loader must retain its saved SP/LR return path. */
	if (memcmp(head->magic, BOOT0_MAGIC, 8))
		return;
	media = readb(&head->boot_media);
	if (media != 0 && media != 2)
		return;
	if (readl(T153_RTC_EFEX_FLAG) != T153_EFEX_MAGIC)
		return;

	for (poll = 0; poll < T153_EFEX_CLEAR_POLLS; ++poll) {
		writel(0, T153_RTC_EFEX_FLAG);
		dsb();
		isb();
		if (!readl(T153_RTC_EFEX_FLAG)) {
			puts("RTC EFEX request cleared; entering BootROM FEL\n");
			t153_enter_fel_fresh();
		}
	}
	puts("RTC EFEX request could not be cleared; stopping in SRAM\n");
	hang();
}
