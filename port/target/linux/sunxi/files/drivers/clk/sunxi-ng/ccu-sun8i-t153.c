// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2020-2024 Allwinner Technology Co., Ltd.
 * T153 core/I/O clock tables, adapted to the upstream sunxi-ng framework.
 * CPU/DRAM frequency and voltage remain under boot firmware control.
 */
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <dt-bindings/clock/sun8i-t153-ccu.h>
#include <dt-bindings/reset/sun8i-t153-ccu.h>
#include "ccu_common.h"
#include "ccu_reset.h"
#include "ccu_div.h"
#include "ccu_gate.h"
#include "ccu_mult.h"

static struct ccu_mult pll_peri0_clk = {

	.lock			= BIT(28),

	.enable = BIT(31) | BIT(29) | BIT(27),
	.mult			= _SUNXI_CCU_MULT_OFFSET_MIN_MAX(8, 8, 1, 52, 105),



	.common			= {
		.reg		= 0x00A0,
		.hw.init	= CLK_HW_INIT("pll-peri0", "dcxo24M",
					&ccu_mult_ops,
					CLK_SET_RATE_UNGATE |
					CLK_IS_CRITICAL |
					0),
	},
};

static SUNXI_CCU_M(pll_peri0_2x_clk, "pll-peri0-2x",
			"pll-peri0", 0x00a0,
			16, 3, CLK_IS_CRITICAL | 0);

static SUNXI_CCU_M(pll_peri0_800m_clk, "pll-peri0-800m",
			"pll-peri0", 0x00a0,
			20, 3, CLK_IS_CRITICAL | 0);

static SUNXI_CCU_M(pll_peri0_480m_clk, "pll-peri0-480m",
			"pll-peri0", 0x00a0,
			2, 3, CLK_IS_CRITICAL | 0);

static CLK_FIXED_FACTOR(pll_peri0_600m_clk, "pll-peri0-600m",
			"pll-peri0-2x", 2, 1, CLK_IS_CRITICAL | 0);

static CLK_FIXED_FACTOR(pll_peri0_400m_clk, "pll-peri0-400m",
			"pll-peri0-2x", 3, 1, CLK_IS_CRITICAL | 0);

static CLK_FIXED_FACTOR(pll_peri0_300m_clk, "pll-peri0-300m",
			"pll-peri0-600m", 2, 1, CLK_IS_CRITICAL | 0);

static CLK_FIXED_FACTOR(pll_peri0_200m_clk, "pll-peri0-200m",
			"pll-peri0-400m", 2, 1, CLK_IS_CRITICAL | 0);

static CLK_FIXED_FACTOR(pll_peri0_160m_clk, "pll-peri0-160m",
			"pll-peri0-480m", 3, 1, CLK_IS_CRITICAL | 0);

static CLK_FIXED_FACTOR(pll_peri0_150m_clk, "pll-peri0-150m",
			"pll-peri0-300m", 2, 1, CLK_IS_CRITICAL | 0);

static struct ccu_mult pll_peri1_clk = {

	.lock			= BIT(28),

	.enable = BIT(31) | BIT(29) | BIT(27),
	.mult			= _SUNXI_CCU_MULT_OFFSET_MIN_MAX(8, 8, 1, 52, 105),



	.common			= {
		.reg		= 0x00C0,
		.hw.init	= CLK_HW_INIT("pll-peri1", "dcxo24M",
					&ccu_mult_ops,
					CLK_SET_RATE_UNGATE | CLK_IS_CRITICAL),
	},
};

static SUNXI_CCU_M(pll_peri1_2x_clk, "pll-peri1-2x",
			"pll-peri1", 0x00c0,
			16, 3, 0);

static SUNXI_CCU_M(pll_peri1_800m_clk, "pll-peri1-800m",
			"pll-peri1", 0x00c0,
			20, 3, 0);

static SUNXI_CCU_M(pll_peri1_480m_clk, "pll-peri1-480m",
			"pll-peri1", 0x00c0,
			2, 3, 0);

static CLK_FIXED_FACTOR(pll_peri1_600m_clk, "pll-peri1-600m",
			"pll-peri1-2x", 2, 1, 0);

static CLK_FIXED_FACTOR(pll_peri1_400m_clk, "pll-peri1-400m",
			"pll-peri1-2x", 3, 1, 0);

static CLK_FIXED_FACTOR(pll_peri1_300m_clk, "pll-peri1-300m",
			"pll-peri1-600m", 2, 1, 0);

static CLK_FIXED_FACTOR(pll_peri1_200m_clk, "pll-peri1-200m",
			"pll-peri1-400m", 2, 1, 0);

static CLK_FIXED_FACTOR(pll_peri1_160m_clk, "pll-peri1-160m",
			"pll-peri1-480m", 3, 1, 0);

static CLK_FIXED_FACTOR(pll_peri1_150m_clk, "pll-peri1-150m",
			"pll-peri1-300m", 2, 1, 0);

static const char * const ahb_parents[] = { "dcxo24M", "ext-32k", "rc_16m", "pll-peri0-600m" };

static SUNXI_CCU_M_WITH_MUX(ahb_clk, "ahb", ahb_parents,
			0x0500, 0, 5, 24, 2, 0);

static const char * const apb0_parents[] = { "dcxo24M", "ext-32k", "rc_16m", "pll-peri0-600m" };

static SUNXI_CCU_M_WITH_MUX(apb0_clk, "apb0", apb0_parents,
			0x0510, 0, 5, 24, 2, 0);

static const char * const apb1_parents[] = { "dcxo24M", "ext-32k", "rc_16m", "pll-peri0-600m" };

static SUNXI_CCU_M_WITH_MUX(apb1_clk, "apb1", apb1_parents,
			0x0518, 0, 5, 24, 2, 0);

static const char * const apb_uart_parents[] = { "dcxo24M", "ext-32k", "rc_16m", "pll-peri0-600m", "pll-peri0-480m" };

static SUNXI_CCU_M_WITH_MUX(apb_uart_clk, "apb-uart", apb_uart_parents,
			0x0538, 0, 5, 24, 3, 0);

static SUNXI_CCU_GATE(usb2p0_sys_ahb_sw_cfg_bus_clk, "usb2p0-sys-ahb-sw-cfg-bus",
			"dcxo24M",
			0x05C0, BIT(16), 0);

static SUNXI_CCU_GATE(gmac1_ahb_sw_cfg_bus_clk, "gmac1-ahb-sw-cfg-bus",
			"dcxo24M",
			0x05C0, BIT(14), 0);

static SUNXI_CCU_GATE(gmac0_ahb_sw_cfg_bus_clk, "gmac0-ahb-sw-cfg-bus",
			"dcxo24M",
			0x05C0, BIT(13), 0);

static SUNXI_CCU_GATE(smhc3_ahb_sw_cfg_bus_clk, "smhc3-ahb-sw-cfg-bus",
			"dcxo24M",
			0x05C0, BIT(8), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE(smhc2_ahb_sw_cfg_bus_clk, "smhc2-ahb-sw-cfg-bus",
			"dcxo24M",
			0x05C0, BIT(7), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE(smhc1_ahb_sw_cfg_bus_clk, "smhc1-ahb-sw-cfg-bus",
			"dcxo24M",
			0x05C0, BIT(6), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE(smhc0_ahb_sw_cfg_bus_clk, "smhc0-ahb-sw-cfg-bus",
			"dcxo24M",
			0x05C0, BIT(5), CLK_IS_CRITICAL);

static SUNXI_CCU_GATE(can_mbus_cfg_clk, "can-mbus-cfg", "dcxo24M",
			0x05E4, BIT(19), 0);
static SUNXI_CCU_GATE(can_mbus_gate_clk, "can-mbus-gate", "can-mbus-cfg",
			0x05E0, BIT(17), 0);

static SUNXI_CCU_GATE(gmac1_mbus_bus_clk, "gmac1-mbus-bus",
			"dcxo24M",
			0x05E0, BIT(12), 0);

static SUNXI_CCU_GATE(gmac0_mbus_bus_clk, "gmac0-mbus-bus",
			"dcxo24M",
			0x05E0, BIT(11), 0);

static SUNXI_CCU_GATE(gmac1_mbus_sw_cfg_bus_clk, "gmac1-mbus-sw-cfg-bus",
			"dcxo24M",
			0x05E4, BIT(13), 0);

static SUNXI_CCU_GATE(gmac0_mbus_sw_cfg_bus_clk, "gmac0-mbus-sw-cfg-bus",
			"dcxo24M",
			0x05E4, BIT(12), 0);

static SUNXI_CCU_GATE(gmac_mbus_sw_cfg_bus_clk, "gmac-mbus-sw-cfg-bus",
			"dcxo24M",
			0x05E4, BIT(11), CLK_IS_CRITICAL);

static const char * const smhc0_parents[] = { "dcxo24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-400m", "pll-peri1-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(smhc0_m_clk, "smhc0-m",
			smhc0_parents, 0x0D00, 0, 5, 24, 3, BIT(31), 0);
static SUNXI_CCU_M(smhc0_clk, "smhc0", "smhc0-m",
			0x0D00, 8, 5, CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(smhc0_ahb_bus_clk, "smhc0-ahb-bus",
			"dcxo24M",
			0x0D0C, BIT(0), 0);

static const char * const smhc1_parents[] = { "dcxo24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-400m", "pll-peri1-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(smhc1_m_clk, "smhc1-m",
			smhc1_parents, 0x0D10, 0, 5, 24, 3, BIT(31), 0);
static SUNXI_CCU_M(smhc1_clk, "smhc1", "smhc1-m",
			0x0D10, 8, 5, CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(smhc1_ahb_bus_clk, "smhc1-ahb-bus",
			"dcxo24M",
			0x0D1C, BIT(0), 0);

static const char * const smhc2_parents[] = { "dcxo24M", "pll-peri0-800m", "pll-peri0-600m", "pll-peri1-800m", "pll-peri1-600m" };

static SUNXI_CCU_M_WITH_MUX_GATE(smhc2_m_clk, "smhc2-m",
			smhc2_parents, 0x0D20, 0, 5, 24, 3, BIT(31), 0);
static SUNXI_CCU_M(smhc2_clk, "smhc2", "smhc2-m",
			0x0D20, 8, 5, CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(smhc2_ahb_bus_clk, "smhc2-ahb-bus",
			"dcxo24M",
			0x0D2C, BIT(0), 0);

static const char * const smhc3_parents[] = { "dcxo24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-400m", "pll-peri1-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(smhc3_m_clk, "smhc3-m",
			smhc3_parents, 0x0D30, 0, 5, 24, 3, BIT(31), 0);
static SUNXI_CCU_M(smhc3_clk, "smhc3", "smhc3-m",
			0x0D30, 8, 5, CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(smhc3_ahb_bus_clk, "smhc3-ahb-bus",
			"dcxo24M",
			0x0D3C, BIT(0), 0);

static SUNXI_CCU_GATE(uart0_apb_bus_clk, "uart0-apb-bus",
			"apb-uart",
			0x0E00, BIT(0), 0);

static SUNXI_CCU_GATE(uart1_apb_bus_clk, "uart1-apb-bus",
			"apb-uart",
			0x0E04, BIT(0), 0);

static SUNXI_CCU_GATE(uart2_apb_bus_clk, "uart2-apb-bus",
			"apb-uart",
			0x0E08, BIT(0), 0);

static SUNXI_CCU_GATE(uart3_apb_bus_clk, "uart3-apb-bus",
			"apb-uart",
			0x0E0C, BIT(0), 0);

static SUNXI_CCU_GATE(uart4_apb_bus_clk, "uart4-apb-bus",
			"apb-uart",
			0x0E10, BIT(0), 0);

static SUNXI_CCU_GATE(uart5_apb_bus_clk, "uart5-apb-bus",
			"apb-uart",
			0x0E14, BIT(0), 0);

static SUNXI_CCU_GATE(uart6_apb_bus_clk, "uart6-apb-bus",
			"apb-uart",
			0x0E18, BIT(0), 0);

static SUNXI_CCU_GATE(uart7_apb_bus_clk, "uart7-apb-bus",
			"apb-uart",
			0x0E20, BIT(0), 0);

static SUNXI_CCU_GATE(uart8_apb_bus_clk, "uart8-apb-bus",
			"apb-uart",
			0x0E24, BIT(0), 0);

static SUNXI_CCU_GATE(uart9_apb_bus_clk, "uart9-apb-bus",
			"apb-uart",
			0x0E28, BIT(0), 0);

static SUNXI_CCU_GATE(twi0_apb_bus_clk, "twi0-apb-bus",
			"apb1",
			0x0E80, BIT(0), 0);

static SUNXI_CCU_GATE(twi1_apb_bus_clk, "twi1-apb-bus",
			"apb1",
			0x0E84, BIT(0), 0);

static SUNXI_CCU_GATE(twi2_apb_bus_clk, "twi2-apb-bus",
			"apb1",
			0x0E88, BIT(0), 0);

static SUNXI_CCU_GATE(twi3_apb_bus_clk, "twi3-apb-bus",
			"apb1",
			0x0E8C, BIT(0), 0);

static SUNXI_CCU_GATE(twi4_apb_bus_clk, "twi4-apb-bus",
			"apb1",
			0x0E90, BIT(0), 0);

static SUNXI_CCU_GATE(twi5_apb_bus_clk, "twi5-apb-bus",
			"apb1",
			0x0E94, BIT(0), 0);

static SUNXI_CCU_M_WITH_GATE(can0_clk, "can0",
			"pll-peri0-400m", 0x0F80,
			0, 5,	/* M */
			BIT(31), 0); /* gate */

static SUNXI_CCU_GATE(can0_bus_clk, "can0-bus",
			"dcxo24M",
			0x0F84, BIT(0), 0);

static SUNXI_CCU_M_WITH_GATE(can1_clk, "can1",
			"pll-peri0-400m", 0x0F88,
			0, 5,		/* M */
			BIT(31), 0); /* gate */

static SUNXI_CCU_GATE(can1_bus_clk, "can1-bus",
			"dcxo24M",
			0x0F8C, BIT(0), 0);

static SUNXI_CCU_GATE(usb0_bus_clk, "usb0-bus",
			"dcxo24M",
			0x1300, BIT(31), 0);

static SUNXI_CCU_GATE(usb0_dev_ahb_bus_clk, "usb0-dev-ahb-bus",
			"dcxo24M",
			0x1304, BIT(8), 0);

static SUNXI_CCU_GATE(usb0_ehci_ahb_bus_clk, "usb0-ehci-ahb-bus",
			"dcxo24M",
			0x1304, BIT(4), 0);

static SUNXI_CCU_GATE(usb0_ohci_ahb_bus_clk, "usb0-ohci-ahb-bus",
			"dcxo24M",
			0x1304, BIT(0), 0);

static SUNXI_CCU_GATE(usb1_bus_clk, "usb1-bus",
			"dcxo24M",
			0x1308, BIT(31), 0);

static SUNXI_CCU_GATE(usb1_ehci_ahb_bus_clk, "usb1-ehci-ahb-bus",
			"dcxo24M",
			0x130C, BIT(4), 0);

static SUNXI_CCU_GATE(usb1_ohci_ahb_bus_clk, "usb1-ohci-ahb-bus",
			"dcxo24M",
			0x130C, BIT(0), 0);

static SUNXI_CCU_GATE(usb2p0_sys_phy_ref_bus_clk, "usb2p0-sys-phy-ref-bus",
			"dcxo24M",
			0x1340, BIT(31), 0);

static SUNXI_CCU_GATE(usb2p0_sys_ahb_bus_clk, "usb2p0-sys-ahb-bus",
			"dcxo24M",
			0x1344, BIT(0), 0);

static SUNXI_CCU_M_WITH_GATE(gmac0_phy_clk, "gmac0-phy",
			"pll-peri0-150m", 0x1400, 0,
			5, BIT(31), 0);

static const char * const gmac0_ptp_ref_parents[] = { "dcxo24M", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(gmac0_ptp_ref_clk, "gmac0-ptp-ref",
			gmac0_ptp_ref_parents, 0x1404,
			0, 5,  /* M */
			24, 3,  /* mux */
			BIT(31),	/* gate */
			0);

static SUNXI_CCU_GATE(gmac0_ahb_bus_clk, "gmac0-ahb-bus",
			"dcxo24M",
			0x140C, BIT(0), 0);

static SUNXI_CCU_M_WITH_GATE(gmac1_phy_clk, "gmac1-phy",
			"pll-peri0-150m", 0x1410, 0,
			5, BIT(31), 0);

static const char * const gmac1_ptp_ref_parents[] = { "dcxo24M", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(gmac1_ptp_ref_clk, "gmac1-ptp-ref",
			gmac1_ptp_ref_parents, 0x1414,
			0, 5,  /* M */
			24, 3,  /* mux */
			BIT(31),	/* gate */
			0);

static SUNXI_CCU_GATE(gmac1_ahb_bus_clk, "gmac1-ahb-bus",
			"dcxo24M",
			0x141C, BIT(0), 0);

static CLK_FIXED_FACTOR(gpio_60m_clk, "gpio-60m",
			"pll-peri0-600m", 10, 1, 0);

static struct ccu_reset_map t153_resets[] = {
	[RST_BUS_PLL_SSC_RSTN]		= { 0x0354, BIT(30) },
	[RST_BUS_DMA0]			= { 0x0704, BIT(16) },
	[RST_BUS_DMA1]			= { 0x070c, BIT(16) },
	[RST_BUS_SPINLOCK]		= { 0x0724, BIT(16) },
	[RST_BUS_MSGBOX_CPUX]		= { 0x0744, BIT(16) },
	[RST_BUS_MSGBOX_CORE0]		= { 0x074c, BIT(16) },
	[RST_BUS_MSGBOX_CORE1]		= { 0x0754, BIT(16) },
	[RST_BUS_MSGBOX_CORE2]		= { 0x075c, BIT(16) },
	[RST_BUS_MSGBOX_CORE3]		= { 0x0764, BIT(16) },
	[RST_BUS_MSGBOX_RV]		= { 0x076c, BIT(16) },
	[RST_BUS_PWM2]			= { 0x0794, BIT(16) },
	[RST_BUS_DCU]			= { 0x07a4, BIT(16) },
	[RST_BUS_DAP]			= { 0x07ac, BIT(16) },
	[RST_BUS_PWMCS0]		= { 0x07c4, BIT(16) },
	[RST_BUS_PWMCS1]		= { 0x07cc, BIT(16) },
	[RST_BUS_TIMER0]		= { 0x0850, BIT(16) },
	[RST_BUS_TIMER0_RV]		= { 0x0870, BIT(16) },
	[RST_BUS_DE0]			= { 0x0a04, BIT(16) },
	[RST_BUS_G2D]			= { 0x0a44, BIT(16) },
	[RST_BUS_CE_SY]			= { 0x0ac4, BIT(17) },
	[RST_BUS_RV_SY]			= { 0x0b94, BIT(17) },
	[RST_BUS_RV_CORE]		= { 0x0b94, BIT(16) },
	[RST_BUS_RV_CFG]		= { 0x0b9c, BIT(16) },
	[RST_BUS_DRAMC]			= { 0x0c0c, BIT(16) },
	[RST_BUS_SMHC0]			= { 0x0d0c, BIT(16) },
	[RST_BUS_SMHC1]			= { 0x0d1c, BIT(16) },
	[RST_BUS_SMHC2]			= { 0x0d2c, BIT(16) },
	[RST_BUS_SMHC3]			= { 0x0d3c, BIT(16) },
	[RST_BUS_UART0]			= { 0x0e00, BIT(16) },
	[RST_BUS_UART1]			= { 0x0e04, BIT(16) },
	[RST_BUS_UART2]			= { 0x0e08, BIT(16) },
	[RST_BUS_UART3]			= { 0x0e0c, BIT(16) },
	[RST_BUS_UART4]			= { 0x0e10, BIT(16) },
	[RST_BUS_UART5]			= { 0x0e14, BIT(16) },
	[RST_BUS_UART6]			= { 0x0e18, BIT(16) },
	[RST_BUS_UART7]			= { 0x0e20, BIT(16) },
	[RST_BUS_UART8]			= { 0x0e24, BIT(16) },
	[RST_BUS_UART9]			= { 0x0e28, BIT(16) },
	[RST_BUS_TWI0]			= { 0x0e80, BIT(16) },
	[RST_BUS_TWI1]			= { 0x0e84, BIT(16) },
	[RST_BUS_TWI2]			= { 0x0e88, BIT(16) },
	[RST_BUS_TWI3]			= { 0x0e8c, BIT(16) },
	[RST_BUS_TWI4]			= { 0x0e90, BIT(16) },
	[RST_BUS_TWI5]			= { 0x0e94, BIT(16) },
	[RST_BUS_SPI0]			= { 0x0f04, BIT(16) },
	[RST_BUS_SPI1]			= { 0x0f0c, BIT(16) },
	[RST_BUS_SPI2]			= { 0x0f14, BIT(16) },
	[RST_BUS_SPIF]			= { 0x0f1c, BIT(16) },
	[RST_BUS_SPI3]			= { 0x0f24, BIT(16) },
	[RST_BUS_CAN0]			= { 0x0f84, BIT(16) },
	[RST_BUS_CAN1]			= { 0x0f8c, BIT(16) },
	[RST_BUS_CAN_SYS]		= { 0x0fbc, BIT(16) },
	[RST_BUS_GPADC0]		= { 0x0fc4, BIT(16) },
	[RST_BUS_GPADC1]		= { 0x0fcc, BIT(16) },
	[RST_BUS_GPADC2]		= { 0x0fd4, BIT(16) },
	[RST_BUS_GPADC3]		= { 0x0fdc, BIT(16) },
	[RST_BUS_TSENSO]		= { 0x0fe4, BIT(16) },
	[RST_BUS_IR_RX0]		= { 0x1004, BIT(16) },
	[RST_BUS_IR_TX]			= { 0x100c, BIT(16) },
	[RST_BUS_TPADC]			= { 0x1034, BIT(16) },
	[RST_BUS_LBC]			= { 0x104c, BIT(16) },
	[RST_BUS_IR_RX1]		= { 0x1104, BIT(16) },
	[RST_BUS_IR_RX2]		= { 0x110c, BIT(16) },
	[RST_BUS_IR_RX3]		= { 0x1114, BIT(16) },
	[RST_BUS_I2S0]			= { 0x120c, BIT(16) },
	[RST_BUS_I2S1]			= { 0x121c, BIT(16) },
	[RST_BUS_I2S2]			= { 0x122c, BIT(16) },
	[RST_BUS_OWA0]			= { 0x128c, BIT(16) },
	[RST_BUS_DMIC]			= { 0x12cc, BIT(16) },
	[RST_BUS_AUDIOCODEC0]		= { 0x12ec, BIT(16) },
	[RST_USB_0_DEV]			= { 0x1304, BIT(24) },
	[RST_USB_0_EHCI]		= { 0x1304, BIT(20) },
	[RST_USB_0_OHCI]		= { 0x1304, BIT(16) },
	[RST_USB_1_EHCI]		= { 0x130c, BIT(20) },
	[RST_USB_1_OHCI]		= { 0x130c, BIT(16) },
	[RST_USB_2P0_SY]		= { 0x1344, BIT(16) },
	[RST_BUS_GMAC0_TOP_AHB]		= { 0x140c, BIT(18) },
	[RST_BUS_GMAC0_AXI]		= { 0x140c, BIT(17) },
	[RST_BUS_GMAC0_AHB]		= { 0x140c, BIT(16) },
	[RST_BUS_GMAC1_TOP_AHB]		= { 0x141c, BIT(18) },
	[RST_BUS_GMAC1_AXI]		= { 0x141c, BIT(17) },
	[RST_BUS_GMAC1_AHB]		= { 0x141c, BIT(16) },
	[RST_BUS_GMAC2_TOP_AHB]		= { 0x142c, BIT(18) },
	[RST_BUS_GMAC2_AXI]		= { 0x142c, BIT(17) },
	[RST_BUS_GMAC2_AHB]		= { 0x142c, BIT(16) },
	[RST_BUS_TCON_LCD0]		= { 0x1504, BIT(16) },
	[RST_BUS_LVDS0]			= { 0x1544, BIT(16) },
	[RST_BUS_MIPI_DSI0]		= { 0x1584, BIT(16) },
	[RST_BUS_VO0_REG]		= { 0x16c4, BIT(16) },
	[RST_BUS_VIDEO_OUT0]		= { 0x16e4, BIT(16) },
	[RST_BUS_LEDC]			= { 0x1704, BIT(16) },
	[RST_BUS_VIDEO_IN]		= { 0x1884, BIT(16) },
	[RST_BUS_RV_AXIMON]		= { 0x1c00, BIT(16) },
	[RST_BUS_DCU_AHBMON]		= { 0x1c04, BIT(17) },
	[RST_BUS_CPU_SYS_AHBMON]	= { 0x1c04, BIT(16) },
};
/* Internal stages also need registration: ccu_clks only assigns MMIO and
 * the common lock. Keep these indices outside the existing DT clock ABI.
 */
enum {
	CLK_SMHC0_M_INTERNAL = CLK_NUMBER,
	CLK_SMHC1_M_INTERNAL,
	CLK_SMHC2_M_INTERNAL,
	CLK_SMHC3_M_INTERNAL,
	CLK_CAN_MBUS_CFG_INTERNAL,
	CLK_T153_HW_NUMBER,
};
static struct clk_hw_onecell_data t153_hw_clks = {
	.hws = {
		[CLK_SMHC0_M_INTERNAL] = &smhc0_m_clk.common.hw,
		[CLK_SMHC1_M_INTERNAL] = &smhc1_m_clk.common.hw,
		[CLK_SMHC2_M_INTERNAL] = &smhc2_m_clk.common.hw,
		[CLK_SMHC3_M_INTERNAL] = &smhc3_m_clk.common.hw,
		[CLK_CAN_MBUS_CFG_INTERNAL] = &can_mbus_cfg_clk.common.hw,
		[CLK_PLL_PERI0] = &pll_peri0_clk.common.hw,
		[CLK_PLL_PERI0_2X] = &pll_peri0_2x_clk.common.hw,
		[CLK_PLL_PERI0_800M] = &pll_peri0_800m_clk.common.hw,
		[CLK_PLL_PERI0_480M] = &pll_peri0_480m_clk.common.hw,
		[CLK_PLL_PERI0_600M] = &pll_peri0_600m_clk.hw,
		[CLK_PLL_PERI0_400M] = &pll_peri0_400m_clk.hw,
		[CLK_PLL_PERI0_300M] = &pll_peri0_300m_clk.hw,
		[CLK_PLL_PERI0_200M] = &pll_peri0_200m_clk.hw,
		[CLK_PLL_PERI0_160M] = &pll_peri0_160m_clk.hw,
		[CLK_PLL_PERI0_150M] = &pll_peri0_150m_clk.hw,
		[CLK_PLL_PERI1] = &pll_peri1_clk.common.hw,
		[CLK_PLL_PERI1_2X] = &pll_peri1_2x_clk.common.hw,
		[CLK_PLL_PERI1_800M] = &pll_peri1_800m_clk.common.hw,
		[CLK_PLL_PERI1_480M] = &pll_peri1_480m_clk.common.hw,
		[CLK_PLL_PERI1_600M] = &pll_peri1_600m_clk.hw,
		[CLK_PLL_PERI1_400M] = &pll_peri1_400m_clk.hw,
		[CLK_PLL_PERI1_300M] = &pll_peri1_300m_clk.hw,
		[CLK_PLL_PERI1_200M] = &pll_peri1_200m_clk.hw,
		[CLK_PLL_PERI1_160M] = &pll_peri1_160m_clk.hw,
		[CLK_PLL_PERI1_150M] = &pll_peri1_150m_clk.hw,
		[CLK_AHB] = &ahb_clk.common.hw,
		[CLK_APB0] = &apb0_clk.common.hw,
		[CLK_APB1] = &apb1_clk.common.hw,
		[CLK_APB_UART] = &apb_uart_clk.common.hw,
		[CLK_BUS_USB2P0_SYS_AHB_SW_CFG] = &usb2p0_sys_ahb_sw_cfg_bus_clk.common.hw,
		[CLK_BUS_GMAC1_AHB_SW_CFG] = &gmac1_ahb_sw_cfg_bus_clk.common.hw,
		[CLK_BUS_GMAC0_AHB_SW_CFG] = &gmac0_ahb_sw_cfg_bus_clk.common.hw,
		[CLK_BUS_SMHC3_AHB_SW_CFG] = &smhc3_ahb_sw_cfg_bus_clk.common.hw,
		[CLK_BUS_SMHC2_AHB_SW_CFG] = &smhc2_ahb_sw_cfg_bus_clk.common.hw,
		[CLK_BUS_SMHC1_AHB_SW_CFG] = &smhc1_ahb_sw_cfg_bus_clk.common.hw,
		[CLK_BUS_SMHC0_AHB_SW_CFG] = &smhc0_ahb_sw_cfg_bus_clk.common.hw,
		[CLK_MBUS_CAN_GATE] = &can_mbus_gate_clk.common.hw,
		[CLK_MBUS_BUS_GMAC1] = &gmac1_mbus_bus_clk.common.hw,
		[CLK_MBUS_BUS_GMAC0] = &gmac0_mbus_bus_clk.common.hw,
		[CLK_MBUS_BUS_GMAC1_SW_CFG] = &gmac1_mbus_sw_cfg_bus_clk.common.hw,
		[CLK_MBUS_BUS_GMAC0_SW_CFG] = &gmac0_mbus_sw_cfg_bus_clk.common.hw,
		[CLK_MBUS_BUS_GMAC_SW_CFG] = &gmac_mbus_sw_cfg_bus_clk.common.hw,
		[CLK_SMHC0] = &smhc0_clk.common.hw,
		[CLK_BUS_SMHC0_AHB] = &smhc0_ahb_bus_clk.common.hw,
		[CLK_SMHC1] = &smhc1_clk.common.hw,
		[CLK_BUS_SMHC1_AHB] = &smhc1_ahb_bus_clk.common.hw,
		[CLK_SMHC2] = &smhc2_clk.common.hw,
		[CLK_BUS_SMHC2_AHB] = &smhc2_ahb_bus_clk.common.hw,
		[CLK_SMHC3] = &smhc3_clk.common.hw,
		[CLK_BUS_SMHC3_AHB] = &smhc3_ahb_bus_clk.common.hw,
		[CLK_BUS_UART0_APB] = &uart0_apb_bus_clk.common.hw,
		[CLK_BUS_UART1_APB] = &uart1_apb_bus_clk.common.hw,
		[CLK_BUS_UART2_APB] = &uart2_apb_bus_clk.common.hw,
		[CLK_BUS_UART3_APB] = &uart3_apb_bus_clk.common.hw,
		[CLK_BUS_UART4_APB] = &uart4_apb_bus_clk.common.hw,
		[CLK_BUS_UART5_APB] = &uart5_apb_bus_clk.common.hw,
		[CLK_BUS_UART6_APB] = &uart6_apb_bus_clk.common.hw,
		[CLK_BUS_UART7_APB] = &uart7_apb_bus_clk.common.hw,
		[CLK_BUS_UART8_APB] = &uart8_apb_bus_clk.common.hw,
		[CLK_BUS_UART9_APB] = &uart9_apb_bus_clk.common.hw,
		[CLK_BUS_TWI0_APB] = &twi0_apb_bus_clk.common.hw,
		[CLK_BUS_TWI1_APB] = &twi1_apb_bus_clk.common.hw,
		[CLK_BUS_TWI2_APB] = &twi2_apb_bus_clk.common.hw,
		[CLK_BUS_TWI3_APB] = &twi3_apb_bus_clk.common.hw,
		[CLK_BUS_TWI4_APB] = &twi4_apb_bus_clk.common.hw,
		[CLK_BUS_TWI5_APB] = &twi5_apb_bus_clk.common.hw,
		[CLK_CAN0] = &can0_clk.common.hw,
		[CLK_BUS_CAN0] = &can0_bus_clk.common.hw,
		[CLK_CAN1] = &can1_clk.common.hw,
		[CLK_BUS_CAN1] = &can1_bus_clk.common.hw,
		[CLK_BUS_USB0] = &usb0_bus_clk.common.hw,
		[CLK_BUS_USB0_DEV_AHB] = &usb0_dev_ahb_bus_clk.common.hw,
		[CLK_BUS_USB0_EHCI_AHB] = &usb0_ehci_ahb_bus_clk.common.hw,
		[CLK_BUS_USB0_OHCI_AHB] = &usb0_ohci_ahb_bus_clk.common.hw,
		[CLK_BUS_USB1] = &usb1_bus_clk.common.hw,
		[CLK_BUS_USB1_EHCI_AHB] = &usb1_ehci_ahb_bus_clk.common.hw,
		[CLK_BUS_USB1_OHCI_AHB] = &usb1_ohci_ahb_bus_clk.common.hw,
		[CLK_BUS_USB2P0_SYS_PHY_REF] = &usb2p0_sys_phy_ref_bus_clk.common.hw,
		[CLK_BUS_USB2P0_SYS_AHB] = &usb2p0_sys_ahb_bus_clk.common.hw,
		[CLK_GMAC0_PHY] = &gmac0_phy_clk.common.hw,
		[CLK_GMAC0_PTP_REF] = &gmac0_ptp_ref_clk.common.hw,
		[CLK_BUS_GMAC0_AHB] = &gmac0_ahb_bus_clk.common.hw,
		[CLK_GMAC1_PHY] = &gmac1_phy_clk.common.hw,
		[CLK_GMAC1_PTP_REF] = &gmac1_ptp_ref_clk.common.hw,
		[CLK_BUS_GMAC1_AHB] = &gmac1_ahb_bus_clk.common.hw,
		[CLK_GPIO_60M] = &gpio_60m_clk.hw,
	},
	.num = CLK_T153_HW_NUMBER,
};
static struct ccu_common *t153_clks[] = {
	&ahb_clk.common,
	&apb0_clk.common,
	&apb1_clk.common,
	&apb_uart_clk.common,
	&can0_bus_clk.common,
	&can0_clk.common,
	&can1_bus_clk.common,
	&can1_clk.common,
	&can_mbus_cfg_clk.common,
	&can_mbus_gate_clk.common,
	&gmac0_ahb_bus_clk.common,
	&gmac0_ahb_sw_cfg_bus_clk.common,
	&gmac0_mbus_bus_clk.common,
	&gmac0_mbus_sw_cfg_bus_clk.common,
	&gmac0_phy_clk.common,
	&gmac0_ptp_ref_clk.common,
	&gmac1_ahb_bus_clk.common,
	&gmac1_ahb_sw_cfg_bus_clk.common,
	&gmac1_mbus_bus_clk.common,
	&gmac1_mbus_sw_cfg_bus_clk.common,
	&gmac1_phy_clk.common,
	&gmac1_ptp_ref_clk.common,
	&gmac_mbus_sw_cfg_bus_clk.common,
	&pll_peri0_2x_clk.common,
	&pll_peri0_480m_clk.common,
	&pll_peri0_800m_clk.common,
	&pll_peri0_clk.common,
	&pll_peri1_2x_clk.common,
	&pll_peri1_480m_clk.common,
	&pll_peri1_800m_clk.common,
	&pll_peri1_clk.common,
	&smhc0_ahb_bus_clk.common,
	&smhc0_ahb_sw_cfg_bus_clk.common,
	&smhc0_clk.common,
	&smhc0_m_clk.common,
	&smhc1_ahb_bus_clk.common,
	&smhc1_ahb_sw_cfg_bus_clk.common,
	&smhc1_clk.common,
	&smhc1_m_clk.common,
	&smhc2_ahb_bus_clk.common,
	&smhc2_ahb_sw_cfg_bus_clk.common,
	&smhc2_clk.common,
	&smhc2_m_clk.common,
	&smhc3_ahb_bus_clk.common,
	&smhc3_ahb_sw_cfg_bus_clk.common,
	&smhc3_clk.common,
	&smhc3_m_clk.common,
	&twi0_apb_bus_clk.common,
	&twi1_apb_bus_clk.common,
	&twi2_apb_bus_clk.common,
	&twi3_apb_bus_clk.common,
	&twi4_apb_bus_clk.common,
	&twi5_apb_bus_clk.common,
	&uart0_apb_bus_clk.common,
	&uart1_apb_bus_clk.common,
	&uart2_apb_bus_clk.common,
	&uart3_apb_bus_clk.common,
	&uart4_apb_bus_clk.common,
	&uart5_apb_bus_clk.common,
	&uart6_apb_bus_clk.common,
	&uart7_apb_bus_clk.common,
	&uart8_apb_bus_clk.common,
	&uart9_apb_bus_clk.common,
	&usb0_bus_clk.common,
	&usb0_dev_ahb_bus_clk.common,
	&usb0_ehci_ahb_bus_clk.common,
	&usb0_ohci_ahb_bus_clk.common,
	&usb1_bus_clk.common,
	&usb1_ehci_ahb_bus_clk.common,
	&usb1_ohci_ahb_bus_clk.common,
	&usb2p0_sys_ahb_bus_clk.common,
	&usb2p0_sys_ahb_sw_cfg_bus_clk.common,
	&usb2p0_sys_phy_ref_bus_clk.common,
};
static const struct sunxi_ccu_desc t153_ccu_desc = {
	.ccu_clks = t153_clks,
	.num_ccu_clks = ARRAY_SIZE(t153_clks),
	.hw_clks = &t153_hw_clks,
	.resets = t153_resets,
	.num_resets = ARRAY_SIZE(t153_resets),
};
static int t153_ccu_probe(struct platform_device *pdev)
{
	void __iomem *reg;

	reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);
	return devm_sunxi_ccu_probe(&pdev->dev, reg, &t153_ccu_desc);
}
static const struct of_device_id t153_ccu_ids[] = {
	{ .compatible = "allwinner,sun8i-t153-ccu" },
	{ }
};
MODULE_DEVICE_TABLE(of, t153_ccu_ids);
static struct platform_driver t153_ccu_driver = {
	.probe = t153_ccu_probe,
	.driver = {
		.name = "sun8i-t153-ccu",
		.of_match_table = t153_ccu_ids,
	},
};
builtin_platform_driver(t153_ccu_driver);
MODULE_DESCRIPTION("Allwinner T153 core and I/O clocks");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS(SUNXI_CCU);
