// SPDX-License-Identifier: GPL-2.0+
/* T153 watchdog v2 reset and BootROM FEL request.
 * Register layout, reset output mask and keys follow the T153 SDK
 * sun8iw22p1.dtsi and sunxi_wdt.c. The RTC request follows rtc.c.
 * No PLL, DDR, MMC, eMMC boot configuration or persistent data is changed.
 */
#include <config.h>
#include <cpu_func.h>
#include <hang.h>
#include <irq_func.h>
#include <stdio.h>
#include <asm/barriers.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/errno.h>
#ifndef CONFIG_XPL_BUILD
#include <command.h>
#endif

#define T153_RTC_FEL_REG		0x07090108UL
#define T153_FEL_MAGIC		0x5aa5a55aU
#define T153_WDT_BASE		0x02050000UL
#define T153_WDT_CTRL		0x0c
#define T153_WDT_CFG		0x10
#define T153_WDT_MODE		0x14
#define T153_WDT_OUT_CFG		0x18
#define T153_WDT_KEY		0x16aa0000U
#define T153_WDT_RELOAD		0x000014afU
#define T153_WDT_OUT_MASK	0x00000fffU
#define T153_WDT_OUT_RESET	0x0000003fU
#define T153_WDT_RESET_MASK	0x00000003U
#define T153_WDT_SYSTEM_RESET	0x00000001U
#define T153_WDT_TIMEOUT_MASK	0x000000f0U
#define T153_WDT_ENABLE		0x00000001U

static int t153_rtc_store(u32 value)
{
	void __iomem *rtc = (void __iomem *)T153_RTC_FEL_REG;
	unsigned int retry;

	for (retry = 0; retry < 1000; retry++) {
		writel(value, rtc);
		dsb();
		isb();
		if (readl(rtc) == value)
			return 0;
		udelay(1);
	}
	return -ETIMEDOUT;
}

static int t153_clear_fel_request(void)
{
	/* This is one full RTC data word, not the index-6 bootmode flag. */
	if (readl((void __iomem *)T153_RTC_FEL_REG) == T153_FEL_MAGIC)
		return t153_rtc_store(0);
	return 0;
}

static int t153_watchdog_prepare(void)
{
	void __iomem *wdt = (void __iomem *)T153_WDT_BASE;
	u32 value;

	/* The SDK node uses the fixed 24 MHz oscillator, without a CCU gate
	 * or reset phandle; the SDK watchdog probe does not enable a clock.
	 * Its probe first stops the watchdog before configuring it.
	 */
	writel(T153_WDT_KEY, wdt + T153_WDT_MODE);
	value = readl(wdt + T153_WDT_OUT_CFG) & ~T153_WDT_OUT_MASK;
	writel(value | T153_WDT_OUT_RESET | T153_WDT_KEY,
	       wdt + T153_WDT_OUT_CFG);
	value = readl(wdt + T153_WDT_CFG) & ~T153_WDT_RESET_MASK;
	writel(value | T153_WDT_SYSTEM_RESET | T153_WDT_KEY,
	       wdt + T153_WDT_CFG);
	dsb();
	if ((readl(wdt + T153_WDT_OUT_CFG) & T153_WDT_OUT_MASK) !=
	    T153_WDT_OUT_RESET ||
	    (readl(wdt + T153_WDT_CFG) & T153_WDT_RESET_MASK) !=
	    T153_WDT_SYSTEM_RESET)
		return -EIO;
	return 0;
}

/* Returns only if register access or the watchdog reset failed. */
static int t153_restart(int fel)
{
	void __iomem *wdt = (void __iomem *)T153_WDT_BASE;
	unsigned int elapsed;
	int ret, interrupts_enabled;
	u32 value;

	if (!fel) {
		ret = t153_clear_fel_request();
		if (ret)
			return ret;
	}
	ret = t153_watchdog_prepare();
	if (ret)
		return ret;
	if (fel) {
		ret = t153_rtc_store(T153_FEL_MAGIC);
		if (ret) {
			t153_clear_fel_request();
			return ret;
		}
	}

	interrupts_enabled = disable_interrupts();
	/* Interval zero is the SDK's shortest watchdog interval (0.5 s). */
	value = readl(wdt + T153_WDT_MODE) & ~T153_WDT_TIMEOUT_MASK;
	writel(value | T153_WDT_ENABLE | T153_WDT_KEY, wdt + T153_WDT_MODE);
	writel(T153_WDT_RELOAD, wdt + T153_WDT_CTRL);
	dsb();
	isb();
	ret = -EIO;
	if ((readl(wdt + T153_WDT_MODE) &
	     (T153_WDT_TIMEOUT_MASK | T153_WDT_ENABLE)) == T153_WDT_ENABLE) {
		/* No watchdog servicing: let hardware reset. A bounded wait keeps
		 * an unvalidated reset path recoverable at the efex command.
		 */
		for (elapsed = 0; elapsed < 2000; elapsed++)
			udelay(1000);
		ret = -ETIMEDOUT;
	}
	writel(T153_WDT_KEY, wdt + T153_WDT_MODE);
	dsb();
	if (fel && t153_clear_fel_request())
		puts("Failed to clear RTC FEL request after reset failure\n");
	if (interrupts_enabled)
		enable_interrupts();
	return ret;
}

void reset_cpu(void)
{
	int ret;

	flush();
	ret = t153_restart(0);
	printf("T153 watchdog reset failed: %d\n", ret);
	flush();
	hang();
}

#ifndef CONFIG_XPL_BUILD
static int do_t153_efex(struct cmd_tbl *cmdtp, int flag, int argc,
		       char *const argv[])
{
	int ret;

	if (argc != 1)
		return CMD_RET_USAGE;
	puts("T153: requesting BootROM FEL via watchdog reset\n");
	flush();
	ret = t153_restart(1);
	printf("T153 FEL reset failed: %d\n", ret);
	return CMD_RET_FAILURE;
}

U_BOOT_CMD(
	efex, 1, 0, do_t153_efex,
	"restart T153 into BootROM USB FEL mode",
	""
);
#endif
