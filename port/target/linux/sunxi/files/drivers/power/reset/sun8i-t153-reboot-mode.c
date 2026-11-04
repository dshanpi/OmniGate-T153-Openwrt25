// SPDX-License-Identifier: GPL-2.0-only
/* Allwinner T153 RTC data[2] BootROM request, independent of RTC timekeeping.
 * The supplied T153 SDK rtc.c uses 0x5aa5a55a in data[2] for FEL. Data[6]
 * is a separate boot-mode flag and must not be used for this request.
 */
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/reboot-mode.h>
#include <linux/slab.h>

#define T153_RTC_FEL_MAGIC 0x5aa5a55aU

struct t153_reboot_mode {
	struct reboot_mode_driver reboot;
	void __iomem *request;
};

static int t153_reboot_mode_write(struct reboot_mode_driver *reboot,
				 unsigned int magic)
{
	struct t153_reboot_mode *mode =
		container_of(reboot, struct t153_reboot_mode, reboot);
	unsigned int retry;

	if (magic && magic != T153_RTC_FEL_MAGIC)
		return -EINVAL;

	/* The RTC bank is in a different clock domain. Follow the SDK's
	 * write/read-back sequence with a bounded retry instead of an
	 * unbounded loop. This callback runs before device shutdown.
	 */
	for (retry = 0; retry < 1000; retry++) {
		writel(magic, mode->request);
		if (readl(mode->request) == magic)
			return 0;
		udelay(1);
	}

	dev_err(reboot->dev, "RTC reboot request %08x did not latch\n", magic);
	return -ETIMEDOUT;
}

static int t153_reboot_mode_probe(struct platform_device *pdev)
{
	struct t153_reboot_mode *mode;

	mode = devm_kzalloc(&pdev->dev, sizeof(*mode), GFP_KERNEL);
	if (!mode)
		return -ENOMEM;

	mode->request = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(mode->request))
		return PTR_ERR(mode->request);

	mode->reboot.dev = &pdev->dev;
	mode->reboot.write = t153_reboot_mode_write;
	return devm_reboot_mode_register(&pdev->dev, &mode->reboot);
}

static const struct of_device_id t153_reboot_mode_of_match[] = {
	{ .compatible = "allwinner,sun8i-t153-reboot-mode" },
	{ }
};
MODULE_DEVICE_TABLE(of, t153_reboot_mode_of_match);

static struct platform_driver t153_reboot_mode_driver = {
	.probe = t153_reboot_mode_probe,
	.driver = {
		.name = "sun8i-t153-reboot-mode",
		.of_match_table = t153_reboot_mode_of_match,
	},
};
module_platform_driver(t153_reboot_mode_driver);

MODULE_DESCRIPTION("Allwinner T153 RTC BootROM reboot mode");
MODULE_LICENSE("GPL");
