// SPDX-License-Identifier: GPL-2.0-only
/*
 * Allwinner T153 GMAC-211 glue for upstream stmmac.
 * Register definitions derived from Allwinner's GPL-2.0 dwmac-sunxi driver.
 * Copyright (c) 2020-2024 Allwinner Technology Co., Ltd.
 */
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include "stmmac_platform.h"

#define T153_RGMII_EN	BIT(2)
#define T153_RMII_EN	BIT(13)
#define T153_TX_DELAY_LO	GENMASK(12, 10)
#define T153_TX_DELAY_HI	GENMASK(17, 16)
#define T153_RX_DELAY	GENMASK(9, 5)

struct t153_dwmac {
	void __iomem *syscfg;
	struct clk_bulk_data bus_clks[2];
	struct clk *phy_clk;
	struct reset_control *ahb_reset;
	u32 config;
};

static int t153_dwmac_init(struct platform_device *pdev, void *priv)
{
	struct t153_dwmac *dwmac = priv;
	int ret;

	ret = reset_control_deassert(dwmac->ahb_reset);
	if (ret)
		return ret;
	ret = clk_bulk_prepare_enable(ARRAY_SIZE(dwmac->bus_clks), dwmac->bus_clks);
	if (ret)
		goto assert_reset;
	ret = clk_prepare_enable(dwmac->phy_clk);
	if (ret)
		goto disable_bus;
	writel(dwmac->config, dwmac->syscfg);
	return 0;

disable_bus:
	clk_bulk_disable_unprepare(ARRAY_SIZE(dwmac->bus_clks), dwmac->bus_clks);
assert_reset:
	reset_control_assert(dwmac->ahb_reset);
	return ret;
}

static void t153_dwmac_exit(struct platform_device *pdev, void *priv)
{
	struct t153_dwmac *dwmac = priv;

	writel(0, dwmac->syscfg);
	clk_disable_unprepare(dwmac->phy_clk);
	clk_bulk_disable_unprepare(ARRAY_SIZE(dwmac->bus_clks), dwmac->bus_clks);
	reset_control_assert(dwmac->ahb_reset);
}

static int t153_dwmac_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct plat_stmmacenet_data *plat;
	struct stmmac_resources res;
	struct t153_dwmac *dwmac;
	u32 tx_delay = 0, rx_delay = 0;
	int ret;

	dwmac = devm_kzalloc(dev, sizeof(*dwmac), GFP_KERNEL);
	if (!dwmac)
		return -ENOMEM;
	ret = stmmac_get_platform_resources(pdev, &res);
	if (ret)
		return ret;
	plat = devm_stmmac_probe_config_dt(pdev, res.mac);
	if (IS_ERR(plat))
		return PTR_ERR(plat);
	dwmac->syscfg = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(dwmac->syscfg))
		return PTR_ERR(dwmac->syscfg);
	/*
	 * The DT helper already acquired this reset as shared.  Keep its
	 * deassert/assert pair in init/exit so the configuration registers are
	 * accessible before stmmac probes, including on resume.  Do not let the
	 * core take a second deassert reference to the same reset.
	 */
	dwmac->ahb_reset = plat->stmmac_ahb_rst;
	plat->stmmac_ahb_rst = NULL;
	dwmac->bus_clks[0].id = "ahb-sw";
	dwmac->bus_clks[1].id = "mbus-sw";
	ret = devm_clk_bulk_get(dev, ARRAY_SIZE(dwmac->bus_clks), dwmac->bus_clks);
	if (ret)
		return dev_err_probe(dev, ret, "bus configuration clocks\n");
	if (of_property_read_bool(np, "allwinner,provide-phy-clock")) {
		dwmac->phy_clk = devm_clk_get(dev, "phy");
		if (IS_ERR(dwmac->phy_clk))
			return dev_err_probe(dev, PTR_ERR(dwmac->phy_clk), "PHY clock\n");
	}

	of_property_read_u32(np, "allwinner,tx-delay-taps", &tx_delay);
	of_property_read_u32(np, "allwinner,rx-delay-taps", &rx_delay);
	if (tx_delay > 31 || rx_delay > 31)
		return dev_err_probe(dev, -EINVAL, "delay taps must be 0..31\n");
	switch (plat->mac_interface) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		dwmac->config = T153_RGMII_EN |
			(of_property_read_bool(np, "allwinner,external-rgmii-clock") ? 1 : 2);
		break;
	case PHY_INTERFACE_MODE_RMII:
		dwmac->config = T153_RMII_EN;
		break;
	default:
		return dev_err_probe(dev, -EINVAL, "unsupported PHY interface\n");
	}
	dwmac->config |= FIELD_PREP(T153_TX_DELAY_LO, tx_delay & 7) |
			 FIELD_PREP(T153_TX_DELAY_HI, tx_delay >> 3) |
			 FIELD_PREP(T153_RX_DELAY, rx_delay);
	plat->bsp_priv = dwmac;
	plat->init = t153_dwmac_init;
	plat->exit = t153_dwmac_exit;
	plat->host_dma_width = 32;
	plat->flags |= STMMAC_FLAG_SPH_DISABLE;
	plat->has_gmac4 = 1;
	plat->has_gmac = 0;
	plat->pmt = 1;
	/* The vendor hardware reference specifies the 200 MHz CSR clock. */
	plat->clk_csr = 4;
	return devm_stmmac_pltfr_probe(pdev, plat, &res);
}

static const struct of_device_id t153_dwmac_match[] = {
	{ .compatible = "allwinner,sun8i-t153-dwmac" },
	{ }
};
MODULE_DEVICE_TABLE(of, t153_dwmac_match);
static struct platform_driver t153_dwmac_driver = {
	.probe = t153_dwmac_probe,
	.driver = {
		.name = "dwmac-sun8i-t153",
		.of_match_table = t153_dwmac_match,
		.pm = &stmmac_pltfr_pm_ops,
	},
};
module_platform_driver(t153_dwmac_driver);
MODULE_DESCRIPTION("Allwinner T153 DWMAC glue");
MODULE_LICENSE("GPL");
