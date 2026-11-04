// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2020-2024 Allwinner Technology Co., Ltd.
 * Pin mux data from the T153 SDK; adapted to the upstream pinctrl framework.
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinctrl.h>
#include "pinctrl-sunxi.h"

static const struct sunxi_desc_pin t153_pins[] = {
	/* Pin banks are: A B C D E F G J K */

	/* bank A */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc2"),        /* gpadc2 */
		SUNXI_FUNCTION(0x3, "tp"),            /* tp */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_ready3 */
		SUNXI_FUNCTION(0x7, "lbus_f1"),       /* lbus_lbe2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc2"),        /* gpadc2 */
		SUNXI_FUNCTION(0x3, "tp"),            /* tp */
		SUNXI_FUNCTION(0x6, "lbus_f1"),		  /* lbus_ready2 */
		SUNXI_FUNCTION(0x7, "lbus"),		  /* lbus_dp1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc2"),        /* gpadc2 */
		SUNXI_FUNCTION(0x3, "tp"),            /* tp */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_dp2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc2"),        /* gpadc2 */
		SUNXI_FUNCTION(0x3, "tp"),            /* tp */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_dp3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),        /* gpadc0 */
		SUNXI_FUNCTION(0x4, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),        /* gpadc0 */
		SUNXI_FUNCTION(0x4, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_ale */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),        /* gpadc0 */
		SUNXI_FUNCTION(0x4, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_cs1 */
		SUNXI_FUNCTION(0x7, "pwmcs_i8"),      /* pwmcs_i8 */
		SUNXI_FUNCTION(0x9, "lcd"),           /* lcd */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),        /* gpadc0 */
		SUNXI_FUNCTION(0x4, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_cs0 */
		SUNXI_FUNCTION(0x7, "pwmcs_i9"),      /* pwmcs_i9 */
		SUNXI_FUNCTION(0x9, "lcd"),           /* lcd */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),        /* gpadc0 */
		SUNXI_FUNCTION(0x4, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_dp0 */
		SUNXI_FUNCTION(0x7, "pwmcs_ia"),      /* pwmcs_ia */
		SUNXI_FUNCTION(0x9, "lcd"),           /* lcd */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),        /* gpadc0 */
		SUNXI_FUNCTION(0x4, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus_f1"),          /* lbus_dp1 */
		SUNXI_FUNCTION(0x7, "pwmcs_ib"),      /* pwmcs_ib */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_lbe3 */
		SUNXI_FUNCTION(0x9, "lcd"),           /* lcd */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),        /* gpadc0 */
		SUNXI_FUNCTION(0x4, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_lbe0 */
		SUNXI_FUNCTION(0x7, "pwmcs_ic"),      /* pwmcs_ic */
		SUNXI_FUNCTION(0x9, "lcd"),           /* lcd */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),        /* gpadc0 */
		SUNXI_FUNCTION(0x4, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_lbe1 */
		SUNXI_FUNCTION(0x7, "pwmcs_id"),      /* pwmcs_id */
		SUNXI_FUNCTION(0x9, "lcd"),           /* lcd */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),        /* gpadc0 */
		SUNXI_FUNCTION(0x4, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_ld8 */
		SUNXI_FUNCTION(0x7, "pwmcs_ie"),      /* pwmcs_ie */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc0"),        /* gpadc0 */
		SUNXI_FUNCTION(0x4, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_ld9 */
		SUNXI_FUNCTION(0x7, "pwmcs_if"),      /* pwmcs_if */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc1"),        /* gpadc1 */
		SUNXI_FUNCTION(0x4, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_ld10 */
		SUNXI_FUNCTION(0x7, "uart6"),         /* uart6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc1"),        /* gpadc1 */
		SUNXI_FUNCTION(0x4, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_ld11 */
		SUNXI_FUNCTION(0x7, "uart6"),         /* uart6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc1"),        /* gpadc1 */
		SUNXI_FUNCTION(0x4, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_ld12 */
		SUNXI_FUNCTION(0x7, "twi3"),          /* twi3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc1"),        /* gpadc1 */
		SUNXI_FUNCTION(0x4, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_ld13 */
		SUNXI_FUNCTION(0x7, "twi3"),          /* twi3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc1"),        /* gpadc1 */
		SUNXI_FUNCTION(0x4, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_ld14 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc1"),        /* gpadc1 */
		SUNXI_FUNCTION(0x4, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_ld15 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc1"),        /* gpadc1 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_wait */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 20),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc1"),        /* gpadc1 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_cs3 */
		SUNXI_FUNCTION(0x7, "spi2"),          /* spi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc1"),        /* gpadc1 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_cs2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gpadc1"),        /* gpadc1 */
		SUNXI_FUNCTION(0x5, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "lbus"),          /* lbus_de */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank B */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x3, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x4, "jtag"),          /* jtag */
		SUNXI_FUNCTION(0x5, "pwm0_6"),        /* pwm0_6 */
		SUNXI_FUNCTION(0x7, "rjtag"),         /* rjtag */
		SUNXI_FUNCTION(0x9, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x3, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x4, "jtag"),          /* jtag */
		SUNXI_FUNCTION(0x5, "pwm0_7"),        /* pwm0_7 */
		SUNXI_FUNCTION(0x7, "rjtag"),         /* rjtag */
		SUNXI_FUNCTION(0x9, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x3, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x4, "jtag"),          /* jtag */
		SUNXI_FUNCTION(0x5, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x7, "rjtag"),         /* rjtag */
		SUNXI_FUNCTION(0x9, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x3, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x4, "jtag"),          /* jtag */
		SUNXI_FUNCTION(0x5, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x7, "rjtag"),         /* rjtag */
		SUNXI_FUNCTION(0x9, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x3, "i2s0_mclk"),     /* i2s0_mclk */
		SUNXI_FUNCTION(0x4, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x5, "pwm1_0"),        /* pwm1_0 */
		SUNXI_FUNCTION(0x6, "ir0"),           /* ir0 */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8 */
		SUNXI_FUNCTION(0x9, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x3, "i2s0_bclk"),     /* i2s0_bclk */
		SUNXI_FUNCTION(0x4, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x5, "pwm1_1"),        /* pwm1_1 */
		SUNXI_FUNCTION(0x6, "ir1"),           /* ir1 */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8 */
		SUNXI_FUNCTION(0x9, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "i2s0_lrck"),     /* i2s0_lrck */
		SUNXI_FUNCTION(0x5, "pwm1_2"),        /* pwm1_2 */
		SUNXI_FUNCTION(0x6, "ir"),            /* ir */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8 */
		SUNXI_FUNCTION(0x8, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x9, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "owa"),           /* owa */
		SUNXI_FUNCTION(0x3, "i2s0_dout0"),    /* i2s0_dout0 */
		SUNXI_FUNCTION(0x4, "i2s0_din1"),     /* i2s0_din1 */
		SUNXI_FUNCTION(0x5, "pwm1_3"),        /* pwm1_3 */
		SUNXI_FUNCTION(0x6, "can1"),		  /* can1_tx0 */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8 */
		SUNXI_FUNCTION(0x8, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x9, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "owa"),           /* owa */
		SUNXI_FUNCTION(0x3, "i2s0_din0"),     /* i2s0_din0 */
		SUNXI_FUNCTION(0x4, "i2s0_dout1"),    /* i2s0_dout1 */
		SUNXI_FUNCTION(0x5, "pwm0_0"),        /* pwm0_0 */
		SUNXI_FUNCTION(0x6, "can1"),		  /* can1_rx0 */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x3, "i2s0_din2"),     /* i2s0_din2 */
		SUNXI_FUNCTION(0x4, "i2s0_dout2"),    /* i2s0_dout2 */
		SUNXI_FUNCTION(0x6, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x3, "i2s0_din3"),     /* i2s0_din3 */
		SUNXI_FUNCTION(0x4, "i2s0_dout3"),    /* i2s0_dout3 */
		SUNXI_FUNCTION(0x5, "pwm0_1"),        /* pwm0_1 */
		SUNXI_FUNCTION(0x6, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi5"),          /* twi5 */
		SUNXI_FUNCTION(0x3, "uart7"),         /* uart7 */
		SUNXI_FUNCTION(0x4, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x5, "pwm0_2"),        /* pwm0_2 */
		SUNXI_FUNCTION(0x6, "watchdog"),      /* watchdog */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8 */
		SUNXI_FUNCTION(0x9, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi5"),          /* twi5 */
		SUNXI_FUNCTION(0x3, "uart7"),         /* uart7 */
		SUNXI_FUNCTION(0x4, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x5, "pwm0_3"),        /* pwm0_3 */
		SUNXI_FUNCTION(0x6, "pll"),           /* pll */
		SUNXI_FUNCTION(0x9, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi4"),          /* twi4 */
		SUNXI_FUNCTION(0x3, "uart7"),         /* uart7 */
		SUNXI_FUNCTION(0x4, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x5, "pwm0_4"),        /* pwm0_4 */
		SUNXI_FUNCTION(0x6, "ir2"),           /* ir2 */
		SUNXI_FUNCTION(0x7, "clk"),           /* clk */
		SUNXI_FUNCTION(0x9, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi4"),          /* twi4 */
		SUNXI_FUNCTION(0x3, "uart7"),         /* uart7 */
		SUNXI_FUNCTION(0x4, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x5, "pwm0_5"),        /* pwm0_5 */
		SUNXI_FUNCTION(0x6, "ir3"),           /* ir3 */
		SUNXI_FUNCTION(0x7, "clk"),           /* clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank C */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x3, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x4, "spif0"),         /* spif0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x3, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x4, "spif0"),         /* spif0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x3, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x4, "spif0"),         /* spif0 */
		SUNXI_FUNCTION(0x5, "boot"),          /* boot */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x3, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x4, "spif0"),         /* spif0 */
		SUNXI_FUNCTION(0x5, "boot"),          /* boot */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x3, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x4, "spif0"),         /* spif0 */
		SUNXI_FUNCTION(0x5, "can1"),		  /* can1_tx0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x3, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x4, "spif0"),         /* spif0 */
		SUNXI_FUNCTION(0x5, "can1"),		  /* can1_rx0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x3, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x4, "spif0"),         /* spif0 */
		SUNXI_FUNCTION(0x5, "can0"),		  /* can0_tx0 */
		SUNXI_FUNCTION(0x6, "uart6"),         /* uart6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "spif0"),         /* spif0 */
		SUNXI_FUNCTION(0x5, "can0"),		  /* can0_rx0 */
		SUNXI_FUNCTION(0x6, "uart6"),         /* uart6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "spif0"),         /* spif0 */
		SUNXI_FUNCTION(0x5, "uart5"),         /* uart5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "spif0"),         /* spif0 */
		SUNXI_FUNCTION(0x5, "uart5"),         /* uart5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "spif0"),         /* spif0 */
		SUNXI_FUNCTION(0x5, "uart5"),         /* uart5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x5, "uart5"),         /* uart5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank D */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "pwm0_0"),        /* pwm0_0 */
		SUNXI_FUNCTION(0x6, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x7, "uart9"),         /* uart9 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld20 */
		SUNXI_FUNCTION(0x9, "pwmcs_o0"),      /* pwmcs_o0 */
		SUNXI_FUNCTION(0xb, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "pwm0_1"),        /* pwm0_1 */
		SUNXI_FUNCTION(0x6, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x7, "uart9"),         /* uart9 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld21 */
		SUNXI_FUNCTION(0x9, "pwmcs_o1"),      /* pwmcs_o1 */
		SUNXI_FUNCTION(0xb, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "pwm0_2"),        /* pwm0_2 */
		SUNXI_FUNCTION(0x6, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x7, "uart9"),         /* uart9 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld22 */
		SUNXI_FUNCTION(0x9, "pwmcs_o2"),      /* pwmcs_o2 */
		SUNXI_FUNCTION(0xb, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "pwm0_3"),        /* pwm0_3 */
		SUNXI_FUNCTION(0x6, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x7, "uart9"),         /* uart9 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld23 */
		SUNXI_FUNCTION(0x9, "pwmcs_o3"),      /* pwmcs_o3 */
		SUNXI_FUNCTION(0xb, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "pwm0_4"),        /* pwm0_4 */
		SUNXI_FUNCTION(0x6, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x7, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld24 */
		SUNXI_FUNCTION(0x9, "pwmcs_o4"),      /* pwmcs_o4 */
		SUNXI_FUNCTION(0xb, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "pwm0_5"),        /* pwm0_5 */
		SUNXI_FUNCTION(0x6, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x7, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld25 */
		SUNXI_FUNCTION(0x9, "pwmcs_o5"),      /* pwmcs_o5 */
		SUNXI_FUNCTION(0xa, "lbus_f1"),		  /* lbus_cs3 */
		SUNXI_FUNCTION(0xb, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "pwm0_6"),        /* pwm0_6 */
		SUNXI_FUNCTION(0x6, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x7, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld26 */
		SUNXI_FUNCTION(0x9, "pwmcs_o6"),      /* pwmcs_o6 */
		SUNXI_FUNCTION(0xa, "lbus_f1"),		  /* lbus_cs2 */
		SUNXI_FUNCTION(0xb, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "pwm0_7"),        /* pwm0_7 */
		SUNXI_FUNCTION(0x6, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x7, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld27 */
		SUNXI_FUNCTION(0x9, "pwmcs_o7"),      /* pwmcs_o7 */
		SUNXI_FUNCTION(0xa, "lbus_f1"),		  /* lbus_ready3 */
		SUNXI_FUNCTION(0xb, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "pwm1_0"),        /* pwm1_0 */
		SUNXI_FUNCTION(0x7, "can1"),		  /* can1_tx0 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld28 */
		SUNXI_FUNCTION(0x9, "pwmcs_o8"),      /* pwmcs_o8 */
		SUNXI_FUNCTION(0xa, "lbus_f1"),		  /* lbus_de */
		SUNXI_FUNCTION(0xb, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "pwm1_1"),        /* pwm1_1 */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x7, "can1"),	      /* can1_rx0 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld29 */
		SUNXI_FUNCTION(0x9, "pwmcs_o9"),      /* pwmcs_o9 */
		SUNXI_FUNCTION(0xa, "lbus_f1"),		  /* lbus_ready2 */
		SUNXI_FUNCTION(0xb, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x5, "pwm1_2"),        /* pwm1_2 */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld30 */
		SUNXI_FUNCTION(0x9, "pwmcs_oa"),      /* pwmcs_oa */
		SUNXI_FUNCTION(0xa, "lbus_f1"),		  /* lbus_ready1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x5, "pwm1_3"),        /* pwm1_3 */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld31 */
		SUNXI_FUNCTION(0x9, "pwmcs_ob"),      /* pwmcs_ob */
		SUNXI_FUNCTION(0xa, "lbus_f1"),		  /* lbus_ready0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x5, "pwm1_4"),        /* pwm1_4 */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld0 */
		SUNXI_FUNCTION(0x9, "pwmcs_oc"),      /* pwmcs_oc */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x5, "pwm1_5"),        /* pwm1_5 */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld1 */
		SUNXI_FUNCTION(0x9, "pwmcs_od"),      /* pwmcs_od */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x5, "pwm1_6"),        /* pwm1_6 */
		SUNXI_FUNCTION(0x6, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld2 */
		SUNXI_FUNCTION(0x9, "pwmcs_oe"),      /* pwmcs_oe */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x5, "pwm1_7"),        /* pwm1_7 */
		SUNXI_FUNCTION(0x6, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld3 */
		SUNXI_FUNCTION(0x9, "pwmcs_of"),      /* pwmcs_of */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x5, "pwm2_0"),        /* pwm2_0 */
		SUNXI_FUNCTION(0x6, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld4 */
		SUNXI_FUNCTION(0x9, "pwmcs_i0"),      /* pwmcs_i0 */
		SUNXI_FUNCTION(0xb, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x5, "pwm2_1"),        /* pwm2_1 */
		SUNXI_FUNCTION(0x6, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x7, "uart8"),         /* uart8 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld5 */
		SUNXI_FUNCTION(0x9, "pwmcs_i1"),      /* pwmcs_i1 */
		SUNXI_FUNCTION(0xb, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "twi5"),          /* twi5 */
		SUNXI_FUNCTION(0x5, "pwm2_2"),        /* pwm2_2 */
		SUNXI_FUNCTION(0x7, "can0"),		  /* can0_tx0 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld6 */
		SUNXI_FUNCTION(0x9, "pwmcs_i2"),      /* pwmcs_i2 */
		SUNXI_FUNCTION(0xb, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "twi5"),          /* twi5 */
		SUNXI_FUNCTION(0x5, "pwm2_3"),        /* pwm2_3 */
		SUNXI_FUNCTION(0x7, "can0"),		  /* can0_rx0 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld7 */
		SUNXI_FUNCTION(0x9, "pwmcs_i3"),      /* pwmcs_i3 */
		SUNXI_FUNCTION(0xb, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "pwm2_4"),        /* pwm2_4 */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x5, "twi5"),          /* twi5 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_wr */
		SUNXI_FUNCTION(0x9, "pwmcs_i4"),      /* pwmcs_i4 */
		SUNXI_FUNCTION(0xb, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 20),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),           /* lcd */
		SUNXI_FUNCTION(0x3, "pwm2_5"),        /* pwm2_5 */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x5, "twi5"),          /* twi5 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_oe */
		SUNXI_FUNCTION(0x9, "pwmcs_i5"),      /* pwmcs_i5 */
		SUNXI_FUNCTION(0xb, "rgmii2"),        /* rgmii2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "pwm2_6"),        /* pwm2_6 */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x5, "twi4"),          /* twi4 */
		SUNXI_FUNCTION(0x6, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x7, "csi0"),          /* csi0 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_drq0 */
		SUNXI_FUNCTION(0x9, "pwmcs_i6"),      /* pwmcs_i6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "pwm2_7"),        /* pwm2_7 */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x5, "twi4"),          /* twi4 */
		SUNXI_FUNCTION(0x6, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x7, "csi1"),          /* csi1 */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_drq1 */
		SUNXI_FUNCTION(0x9, "pwmcs_i7"),      /* pwmcs_i7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank E */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsi0"),         /* mcsi0 */
		SUNXI_FUNCTION(0x4, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x5, "clk"),           /* clk */
		SUNXI_FUNCTION(0x7, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x9, "sdc3"),          /* sdc3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x4, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x7, "dmic"),          /* dmic */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x3, "pwm0_3"),        /* pwm0_3 */
		SUNXI_FUNCTION(0x4, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x7, "dmic"),          /* dmic */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x3, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x4, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x5, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x6, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x7, "dmic"),          /* dmic */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x3, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x4, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x5, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x6, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x7, "dmic"),          /* dmic */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsi1"),         /* mcsi1 */
		SUNXI_FUNCTION(0x3, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x4, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x5, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x6, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x9, "sdc3"),          /* sdc3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x3, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x4, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x5, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x6, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x9, "sdc3"),          /* sdc3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x3, "pwm0_2"),        /* pwm0_2 */
		SUNXI_FUNCTION(0x4, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x6, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x9, "sdc3"),          /* sdc3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsi2"),         /* mcsi2 */
		SUNXI_FUNCTION(0x3, "pwm0_4"),        /* pwm0_4 */
		SUNXI_FUNCTION(0x4, "csi0"),          /* csi0 */
		SUNXI_FUNCTION(0x5, "clk"),           /* clk */
		SUNXI_FUNCTION(0x6, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x9, "sdc3"),          /* sdc3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "tcon"),          /* tcon */
		SUNXI_FUNCTION(0x3, "pwm0_5"),        /* pwm0_5 */
		SUNXI_FUNCTION(0x4, "csi1"),          /* csi1 */
		SUNXI_FUNCTION(0x9, "sdc3"),          /* sdc3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank F */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "jtag"),          /* jtag */
		SUNXI_FUNCTION(0x4, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x7, "rjtag"),         /* rjtag */
		SUNXI_FUNCTION(0x8, "ir0"),           /* ir0 */
		SUNXI_FUNCTION(0x9, "lbus"),          /* lbus_drq0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "jtag"),          /* jtag */
		SUNXI_FUNCTION(0x4, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x7, "rjtag"),         /* rjtag */
		SUNXI_FUNCTION(0x8, "ir1"),           /* ir1 */
		SUNXI_FUNCTION(0x9, "lbus"),          /* lbus_drq1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x4, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x6, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x8, "ir2"),           /* ir2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "jtag"),          /* jtag */
		SUNXI_FUNCTION(0x4, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x6, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x7, "rjtag"),         /* rjtag */
		SUNXI_FUNCTION(0x8, "ir3"),           /* ir3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x4, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x6, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x8, "ir"),            /* ir */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "jtag"),          /* jtag */
		SUNXI_FUNCTION(0x4, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x6, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x7, "rjtag"),         /* rjtag */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "dmic"),          /* dmic */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank G */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x4, "uart8"),         /* uart8 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x4, "uart8"),         /* uart8 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x4, "uart8"),         /* uart8 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x4, "uart8"),         /* uart8 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x4, "can0"),		  /* can0_tx0 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x4, "can0"),		  /* can0_rx0 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x3, "twi4"),          /* twi4 */
		SUNXI_FUNCTION(0x4, "uart9"),         /* uart9 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x6, "i2s2_mclk"),     /* i2s2_mclk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x3, "twi4"),          /* twi4 */
		SUNXI_FUNCTION(0x4, "uart9"),         /* uart9 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x6, "i2s2_bclk"),     /* i2s2_bclk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x3, "twi5"),          /* twi5 */
		SUNXI_FUNCTION(0x4, "uart9"),         /* uart9 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x6, "i2s2_lrck"),     /* i2s2_lrck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x3, "twi5"),          /* twi5 */
		SUNXI_FUNCTION(0x4, "uart9"),         /* uart9 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x6, "i2s2_dout0"),    /* i2s2_dout0 */
		SUNXI_FUNCTION(0x7, "i2s2_din1"),     /* i2s2_din1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x3, "i2s1_mclk"),     /* i2s1_mclk */
		SUNXI_FUNCTION(0x4, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x6, "i2s2_din0"),     /* i2s2_din0 */
		SUNXI_FUNCTION(0x7, "i2s2_dout1"),    /* i2s2_dout1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x3, "i2s1_bclk"),     /* i2s1_bclk */
		SUNXI_FUNCTION(0x4, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x3, "i2s1_lrck"),     /* i2s1_lrck */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x3, "i2s1_dout0"),    /* i2s1_dout0 */
		SUNXI_FUNCTION(0x4, "i2s1_din1"),     /* i2s1_din1 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "can1"),		  /* can1_tx0 */
		SUNXI_FUNCTION(0x3, "i2s1_din0"),     /* i2s1_din0 */
		SUNXI_FUNCTION(0x4, "i2s1_dout1"),    /* i2s1_dout1 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_drq0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "can1"),		  /* can1_rx0 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_drq1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank J */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_0"),        /* pwm2_0 */
		SUNXI_FUNCTION(0x3, "i2s2_mclk"),     /* i2s2_mclk */
		SUNXI_FUNCTION(0x4, "twi4"),          /* twi4 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x7, "uart5"),         /* uart5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_1"),        /* pwm2_1 */
		SUNXI_FUNCTION(0x3, "i2s2_bclk"),     /* i2s2_bclk */
		SUNXI_FUNCTION(0x4, "twi4"),          /* twi4 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x7, "uart5"),         /* uart5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_2"),        /* pwm2_2 */
		SUNXI_FUNCTION(0x3, "i2s2_lrck"),     /* i2s2_lrck */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x7, "uart5"),         /* uart5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_3"),        /* pwm2_3 */
		SUNXI_FUNCTION(0x3, "i2s2_dout0"),    /* i2s2_dout0 */
		SUNXI_FUNCTION(0x4, "i2s2_din1"),     /* i2s2_din1 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x7, "uart5"),         /* uart5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_4"),        /* pwm2_4 */
		SUNXI_FUNCTION(0x3, "i2s2_din0"),     /* i2s2_din0 */
		SUNXI_FUNCTION(0x4, "i2s2_dout1"),    /* i2s2_dout1 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x7, "uart7"),         /* uart7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_5"),        /* pwm2_5 */
		SUNXI_FUNCTION(0x3, "i2s2_din2"),     /* i2s2_din2 */
		SUNXI_FUNCTION(0x4, "i2s2_dout2"),    /* i2s2_dout2 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x7, "uart7"),         /* uart7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_6"),        /* pwm2_6 */
		SUNXI_FUNCTION(0x3, "i2s2_din3"),     /* i2s2_din3 */
		SUNXI_FUNCTION(0x4, "i2s2_dout3"),    /* i2s2_dout3 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x7, "uart7"),         /* uart7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_7"),        /* pwm2_7 */
		SUNXI_FUNCTION(0x3, "i2s1_mclk"),     /* i2s1_mclk */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x7, "uart7"),         /* uart7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_8"),        /* pwm2_8 */
		SUNXI_FUNCTION(0x3, "i2s1_bclk"),     /* i2s1_bclk */
		SUNXI_FUNCTION(0x4, "ledc"),          /* ledc */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x7, "uart2"),         /* uart2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_9"),        /* pwm2_9 */
		SUNXI_FUNCTION(0x3, "i2s1_lrck"),     /* i2s1_lrck */
		SUNXI_FUNCTION(0x4, "ir"),            /* ir */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x7, "uart2"),         /* uart2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_10"),       /* pwm2_10 */
		SUNXI_FUNCTION(0x3, "i2s1_dout0"),    /* i2s1_dout0 */
		SUNXI_FUNCTION(0x4, "i2s1_din1"),     /* i2s1_din1 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x7, "uart2"),         /* uart2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_11"),       /* pwm2_11 */
		SUNXI_FUNCTION(0x3, "i2s1_din0"),     /* i2s1_din0 */
		SUNXI_FUNCTION(0x4, "i2s1_dout1"),    /* i2s1_dout1 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x7, "uart2"),         /* uart2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_12"),       /* pwm2_12 */
		SUNXI_FUNCTION(0x3, "can0"),		  /* can0_tx0 */
		SUNXI_FUNCTION(0x4, "ir0"),           /* ir0 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x6, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x7, "uart9"),         /* uart9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2_13"),       /* pwm2_13 */
		SUNXI_FUNCTION(0x3, "can0"),		  /* can0_rx0 */
		SUNXI_FUNCTION(0x4, "ir1"),           /* ir1 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x6, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x7, "uart9"),         /* uart9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "can1"),		  /* can1_tx0 */
		SUNXI_FUNCTION(0x4, "ir2"),           /* ir2 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x6, "twi5"),          /* twi5 */
		SUNXI_FUNCTION(0x7, "uart9"),         /* uart9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "can1"),		  /* can1_rx0 */
		SUNXI_FUNCTION(0x4, "ir3"),           /* ir3 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x6, "twi5"),          /* twi5 */
		SUNXI_FUNCTION(0x7, "uart9"),         /* uart9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank K */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "mcsia"),         /* mcsia */
		SUNXI_FUNCTION(0x5, "uart7"),         /* uart7 */
		SUNXI_FUNCTION(0x6, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x7, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ready1 */
		SUNXI_FUNCTION(0x9, "i2s1_mclk"),     /* i2s1_mclk */
		SUNXI_FUNCTION(0xa, "lbus_f1"),		  /* lbus_drq0 */
		SUNXI_FUNCTION(0xb, "lcd"),	      /* lcd */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "mcsia"),         /* mcsia */
		SUNXI_FUNCTION(0x5, "uart7"),         /* uart7 */
		SUNXI_FUNCTION(0x6, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x7, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ready0 */
		SUNXI_FUNCTION(0x9, "i2s1_bclk"),     /* i2s1_bclk */
		SUNXI_FUNCTION(0xa, "lbus_f1"),		  /* lbus_drq1 */
		SUNXI_FUNCTION(0xb, "lcd"),	      /* lcd */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "mcsia"),         /* mcsia */
		SUNXI_FUNCTION(0x5, "uart7"),         /* uart7 */
		SUNXI_FUNCTION(0x6, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x7, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld19 */
		SUNXI_FUNCTION(0x9, "i2s1_lrck"),     /* i2s1_lrck */
		SUNXI_FUNCTION(0xa, "lbus_f1"),		  /* lbus_lbe3 */
		SUNXI_FUNCTION(0xb, "lcd"),	      /* lcd */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "mcsia"),         /* mcsia */
		SUNXI_FUNCTION(0x5, "uart7"),         /* uart7 */
		SUNXI_FUNCTION(0x6, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x7, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld18 */
		SUNXI_FUNCTION(0x9, "i2s1_dout0"),    /* i2s1_dout0 */
		SUNXI_FUNCTION(0xa, "lbus_f1"),       /* lbus_de */
		SUNXI_FUNCTION(0xb, "lcd"),	      /* lcd */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "mcsia"),         /* mcsia */
		SUNXI_FUNCTION(0x5, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x6, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x7, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld17 */
		SUNXI_FUNCTION(0x9, "i2s1_din0"),     /* i2s1_din0 */
		SUNXI_FUNCTION(0xa, "lbus_f1"),		  /* lbus_cs3 */
		SUNXI_FUNCTION(0xb, "lcd"),	      /* lcd */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "mcsia"),         /* mcsia */
		SUNXI_FUNCTION(0x5, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x6, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x7, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld16 */
		SUNXI_FUNCTION(0xa, "lbus_f1"),		  /* lbus_cs2 */
		SUNXI_FUNCTION(0xb, "lcd"),	      /* lcd */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "mcsib"),         /* mcsib */
		SUNXI_FUNCTION(0x5, "mcsi0"),         /* mcsi0 */
		SUNXI_FUNCTION(0x6, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x7, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld15 */
		SUNXI_FUNCTION(0x9, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0xb, "rgmii0"),	      /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "mcsib"),         /* mcsib */
		SUNXI_FUNCTION(0x5, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x6, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x7, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld14 */
		SUNXI_FUNCTION(0xb, "rgmii0"),	      /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "mcsib"),         /* mcsib */
		SUNXI_FUNCTION(0x5, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x6, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x7, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld13 */
		SUNXI_FUNCTION(0xb, "rgmii0"),	      /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "mcsib"),         /* mcsib */
		SUNXI_FUNCTION(0x5, "mcsi1"),         /* mcsi1 */
		SUNXI_FUNCTION(0x6, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x7, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_ld12 */
		SUNXI_FUNCTION(0x9, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0xb, "rgmii0"),	      /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "mcsib"),         /* mcsib */
		SUNXI_FUNCTION(0x5, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x6, "pwm0_6"),        /* pwm0_6 */
		SUNXI_FUNCTION(0x7, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x8, "lbus_f1"),	      /* lbus_wait */
		SUNXI_FUNCTION(0x9, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0xa, "lbus"),          /* lbus_lbe2 */
		SUNXI_FUNCTION(0xb, "rgmii0"),	      /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "mcsib"),         /* mcsib */
		SUNXI_FUNCTION(0x5, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x6, "pwm0_7"),        /* pwm0_7 */
		SUNXI_FUNCTION(0x7, "ncsi"),          /* ncsi */
		SUNXI_FUNCTION(0x8, "lbus"),          /* lbus_cs2 */
		SUNXI_FUNCTION(0x9, "uart3"),         /* uart3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */

};

static const unsigned int t153_irq_bank_map[] = { 0, 1, 2, 3, 4, 5, 6, 9, 10 };

static const struct sunxi_pinctrl_desc t153_pinctrl_data = {
	.pins = t153_pins,
	.npins = ARRAY_SIZE(t153_pins),
	.irq_banks = ARRAY_SIZE(t153_irq_bank_map),
	.irq_bank_map = t153_irq_bank_map,
	.t153_reg_layout = true,
};

static int t153_pinctrl_probe(struct platform_device *pdev)
{
	return sunxi_pinctrl_init(pdev, &t153_pinctrl_data);
}

static const struct of_device_id t153_pinctrl_match[] = {
	{ .compatible = "allwinner,sun8i-t153-pinctrl" },
	{ }
};
MODULE_DEVICE_TABLE(of, t153_pinctrl_match);

static struct platform_driver t153_pinctrl_driver = {
	.probe = t153_pinctrl_probe,
	.driver = {
		.name = "sun8i-t153-pinctrl",
		.of_match_table = t153_pinctrl_match,
	},
};
builtin_platform_driver(t153_pinctrl_driver);
MODULE_DESCRIPTION("Allwinner T153 pin controller");
MODULE_LICENSE("GPL");
