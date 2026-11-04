// SPDX-License-Identifier: GPL-2.0-only
/*
 * Allwinner T153 AW PHY v1.01, host mode.
 * Register sequences derived from Allwinner's GPL-2.0 sunxi-awphy-plat.c
 * and sunxi-hci.c, Copyright (c) 2020-2024 Allwinner Technology Co., Ltd.
 * The USB0 gadget/OTG switch is intentionally not implemented here.
 */
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/reset.h>

#define T153_PHY_CTRL(n)	(0x10 + 0x20 * (n))
#define T153_PHY_RST(n)	(0x28 + 0x20 * (n))
#define T153_SIDDQ	BIT(3)
#define T153_RESET_N	BIT(0)
#define T153_MAP_SEL	BIT(0)
/* OHCI bulk-out bypass, INCR8, INCR4, INCR alignment, UTMI. */
#define T153_HOST_BYPASS	(BIT(15) | BIT(10) | BIT(9) | BIT(8) | BIT(0))

struct t153_usb_phy;
struct t153_usb_port {
	struct t153_usb_phy *parent;
	struct phy *phy;
	void __iomem *pmu;
	unsigned int index;
};

struct t153_usb_phy {
	void __iomem *regs;
	void __iomem *top;
	struct clk_bulk_data clocks[3];
	struct reset_control *reset;
	struct mutex lock;
	struct t153_usb_port ports[2];
};

static int t153_usb_init(struct phy *phy)
{
	struct t153_usb_port *port = phy_get_drvdata(phy);
	struct t153_usb_phy *priv = port->parent;
	u32 val;
	int ret;

	ret = clk_bulk_prepare_enable(ARRAY_SIZE(priv->clocks), priv->clocks);
	if (ret)
		return ret;
	ret = reset_control_deassert(priv->reset);
	if (ret) {
		clk_bulk_disable_unprepare(ARRAY_SIZE(priv->clocks), priv->clocks);
		return ret;
	}
	mutex_lock(&priv->lock);
	val = readl(priv->regs + T153_PHY_CTRL(port->index));
	writel(val & ~T153_SIDDQ, priv->regs + T153_PHY_CTRL(port->index));
	val = readl(priv->regs + T153_PHY_RST(port->index));
	writel(val | T153_RESET_N, priv->regs + T153_PHY_RST(port->index));
	val = readl(priv->top + 0x08);
	writel(val | T153_MAP_SEL, priv->top + 0x08);
	val = readl(port->pmu);
	writel(val | T153_HOST_BYPASS, port->pmu);
	mutex_unlock(&priv->lock);
	return 0;
}

static int t153_usb_exit(struct phy *phy)
{
	struct t153_usb_port *port = phy_get_drvdata(phy);
	struct t153_usb_phy *priv = port->parent;
	u32 val;

	mutex_lock(&priv->lock);
	val = readl(port->pmu);
	writel(val & ~T153_HOST_BYPASS, port->pmu);
	val = readl(priv->regs + T153_PHY_RST(port->index));
	writel(val & ~T153_RESET_N, priv->regs + T153_PHY_RST(port->index));
	val = readl(priv->regs + T153_PHY_CTRL(port->index));
	writel(val | T153_SIDDQ, priv->regs + T153_PHY_CTRL(port->index));
	mutex_unlock(&priv->lock);
	reset_control_assert(priv->reset);
	clk_bulk_disable_unprepare(ARRAY_SIZE(priv->clocks), priv->clocks);
	return 0;
}

static int t153_usb_set_mode(struct phy *phy, enum phy_mode mode, int submode)
{
	if (mode != PHY_MODE_USB_HOST && mode != PHY_MODE_USB_HOST_LS &&
	    mode != PHY_MODE_USB_HOST_FS && mode != PHY_MODE_USB_HOST_HS)
		return -EOPNOTSUPP;
	return 0;
}

static const struct phy_ops t153_usb_ops = {
	.init = t153_usb_init,
	.exit = t153_usb_exit,
	.set_mode = t153_usb_set_mode,
	.owner = THIS_MODULE,
};

static struct phy *t153_usb_xlate(struct device *dev, const struct of_phandle_args *args)
{
	struct t153_usb_phy *priv = dev_get_drvdata(dev);

	if (args->args_count != 1 || args->args[0] >= ARRAY_SIZE(priv->ports))
		return ERR_PTR(-EINVAL);
	/* Port 0 is wired as a device port on OmniGate; do not switch its role. */
	if (!args->args[0])
		return ERR_PTR(-EOPNOTSUPP);
	return priv->ports[args->args[0]].phy;
}

static int t153_usb_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phy_provider *provider;
	struct t153_usb_phy *priv;
	struct t153_usb_port *port;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	mutex_init(&priv->lock);
	priv->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(priv->regs))
		return PTR_ERR(priv->regs);
	priv->top = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(priv->top))
		return PTR_ERR(priv->top);
	priv->clocks[0].id = "bus";
	priv->clocks[1].id = "bus-cfg";
	priv->clocks[2].id = "ref";
	ret = devm_clk_bulk_get(dev, ARRAY_SIZE(priv->clocks), priv->clocks);
	if (ret)
		return dev_err_probe(dev, ret, "PHY clocks\n");
	priv->reset = devm_reset_control_get_shared(dev, NULL);
	if (IS_ERR(priv->reset))
		return dev_err_probe(dev, PTR_ERR(priv->reset), "USB subsystem reset\n");
	port = &priv->ports[1];
	port->parent = priv;
	port->index = 1;
	port->pmu = devm_platform_ioremap_resource(pdev, 2);
	if (IS_ERR(port->pmu))
		return PTR_ERR(port->pmu);
	port->phy = devm_phy_create(dev, NULL, &t153_usb_ops);
	if (IS_ERR(port->phy))
		return PTR_ERR(port->phy);
	phy_set_drvdata(port->phy, port);
	platform_set_drvdata(pdev, priv);
	provider = devm_of_phy_provider_register(dev, t153_usb_xlate);
	return PTR_ERR_OR_ZERO(provider);
}

static const struct of_device_id t153_usb_match[] = {
	{ .compatible = "allwinner,sun8i-t153-usb-phy" },
	{ }
};
MODULE_DEVICE_TABLE(of, t153_usb_match);
static struct platform_driver t153_usb_driver = {
	.probe = t153_usb_probe,
	.driver = {
		.name = "phy-sun8i-t153-usb",
		.of_match_table = t153_usb_match,
	},
};
module_platform_driver(t153_usb_driver);
MODULE_DESCRIPTION("Allwinner T153 USB2 host PHY");
MODULE_LICENSE("GPL");
