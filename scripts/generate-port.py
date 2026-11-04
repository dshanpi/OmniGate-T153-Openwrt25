#!/usr/bin/env python3
"""Developer helper: derive register tables from the supplied, GPL-2.0 BSP.

The resulting port/ files are standalone; a firmware build does not use Tina.
"""
from pathlib import Path
import re
import difflib
import argparse

ROOT = Path(__file__).resolve().parents[1]
REF = ROOT / '.reference'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, default=ROOT / 'port/target/linux/sunxi',
                    help='sunxi overlay output directory (use an isolated directory to audit changes)')
OUT = parser.parse_args().output.resolve()
FILES = OUT / 'files'

def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(data, encoding='utf-8', newline='\n')

def patch(name, changes):
    data = ''
    for path, after in changes.items():
        before = (REF / 'mainline' / path).read_text()
        data += ''.join(difflib.unified_diff(before.splitlines(True), after.splitlines(True),
                                            fromfile='a/'+path, tofile='b/'+path))
    write(OUT / 'patches-6.12' / name, data)

# Preserve the original GPL notice and pin mux table, excluding FPGA-only pins.
pins = (REF / 'bsp/drivers/pinctrl/pinctrl-sun8iw22.c').read_text()
table = pins.split('static const struct sunxi_desc_pin sun8iw22_pins[] = {', 1)[1].split('\n};',1)[0]
table = table.split('#else',1)[1].rsplit('#endif',1)[0]
write(FILES / 'drivers/pinctrl/sunxi/pinctrl-sun8i-t153.c', '''// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2020-2024 Allwinner Technology Co., Ltd.
 * Pin mux data from the T153 SDK; adapted to the upstream pinctrl framework.
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinctrl.h>
#include "pinctrl-sunxi.h"

static const struct sunxi_desc_pin t153_pins[] = {''' + table + '''
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
''')

hpath = 'drivers/pinctrl/sunxi/pinctrl-sunxi.h'
cpath = 'drivers/pinctrl/sunxi/pinctrl-sunxi.c'
h = (REF/'mainline'/hpath).read_text()
c = (REF/'mainline'/cpath).read_text()
h = h.replace('regulators[9]', 'regulators[11]')
h = h.replace('#define PL_BASE', '#define PJ_BASE\t288\n#define PK_BASE\t320\n#define PL_BASE')
h = h.replace('bool\t\t\t\tdisable_strict_mode;', 'bool\t\t\t\tdisable_strict_mode;\n\tbool\t\t\t\tt153_reg_layout;')
h = h.replace('u32\t\t\t\tbank_mem_size;', 'u32\t\t\t\tbank_mem_size;\n\tu32\t\t\t\tbank_base_offset;\n\tu32\t\t\t\tdlevel_regs_offset;')
# T153 interrupts live inside each 0x80-byte bank, after its GPIO registers.
for old,new in [('IRQ_CFG_REG','0xc0'),('IRQ_CTRL_REG','0xd0'),('IRQ_STATUS_REG','0xd4'),('IRQ_DEBOUNCE_REG','0xd8')]:
    h = h.replace('return '+old+' +', 'return (desc->t153_reg_layout ? '+new+' : '+old+') +')
h = h.replace('* IRQ_MEM_SIZE', '* (desc->t153_reg_layout ? 0x80 : IRQ_MEM_SIZE)')
c = c.replace('bank * pctl->bank_mem_size +', 'pctl->bank_base_offset + bank * pctl->bank_mem_size +')
c = c.replace('+ DLEVEL_REGS_OFFSET +', '+ pctl->dlevel_regs_offset +')
needle = '\tpctl->desc = desc;'
assert needle in c
c = c.replace(needle, needle+'\n\tpctl->dlevel_regs_offset = DLEVEL_REGS_OFFSET;')
needle = '\tpctl->irq_array = devm_kcalloc(&pdev->dev,'
assert needle in c
# Override after the legacy and D1 layout defaults have been set.
c = c.replace(needle, '''	if (desc->t153_reg_layout) {
		pctl->bank_base_offset = 0x80;
		pctl->bank_mem_size = 0x80;
		pctl->dlevel_regs_offset = 0x20;
		pctl->dlevel_field_width = 4;
		pctl->pull_regs_offset = 0x30;
	}
''' + needle)
kp='drivers/pinctrl/sunxi/Kconfig'; mp='drivers/pinctrl/sunxi/Makefile'
k=(REF/'mainline'/kp).read_text()+'''\nconfig PINCTRL_SUN8I_T153
	bool "Allwinner T153 pin controller"
	depends on MACH_SUN8I || COMPILE_TEST
	select PINCTRL_SUNXI
'''
m=(REF/'mainline'/mp).read_text()+'\nobj-$(CONFIG_PINCTRL_SUN8I_T153) += pinctrl-sun8i-t153.o\n'
patch('500-pinctrl-sunxi-add-T153-register-layout.patch',{hpath:h,cpath:c,kp:k,mp:m})

# Clock register tables are kept separate from the vendor clock framework.
ccu=(REF/'bsp/drivers/clk/sunxi-ng/ccu-sun8iw22.c').read_text()
declarations = re.findall(r'^static .*?;\n',ccu,re.M|re.S)
wanted=re.compile(r'pll_peri[01]|(?:^|\W)(?:ahb_clk|apb0_clk|apb1_clk|apb_uart_clk|gpio_60m_clk)|(?:^|\W)(?:ahb|apb[01]|apb_uart)_parents|smhc[0123]|uart[0-9]_apb|twi[0-5]_apb|gmac[01]_|gmac_mbus|usb[012]|can[01]_|can_mbus')
selected=[]
for d in declarations:
    first = d.split('=',1)[0] if d.startswith('static struct') or d.startswith('static const') else d.split(',',1)[0]
    if wanted.search(first) and not 'sdm_table' in first and not 'hw_clks' in first and not 'resets' in first:
        selected.append(d)
data='\n'.join(selected)
# The vendor's associated CAN gate is two gates in separate registers.
# Model them as a parent chain so common-clock enable counts stay balanced.
data=re.sub(r'static SUNXI_CCU_GATE_ASSOC\(can_mbus_gate_clk,.*?;\n', '''static SUNXI_CCU_GATE(can_mbus_cfg_clk, "can-mbus-cfg", "dcxo24M",
			0x05E4, BIT(19), 0);
static SUNXI_CCU_GATE(can_mbus_gate_clk, "can-mbus-gate", "can-mbus-cfg",
			0x05E0, BIT(17), 0);
''', data, flags=re.S)
# Infrastructure gates have no separate upstream consumers yet. Keep the
# shared PLL/GMAC fabric and MMC configuration access clocks enabled.
data=re.sub(r'(static SUNXI_CCU_GATE\((?:pll_peri\w*|gmac_mbus\w*|smhc[0123]_ahb_sw_cfg\w*).*?), 0\);',r'\1, CLK_IS_CRITICAL);',data,flags=re.S)
data=data.replace('SUNXI_CLK_IS_COMMON','0')
data=re.sub(r'^\s*\.output\s*=.*\n','\n',data,flags=re.M)
data=re.sub(r'^\s*\.lock_enable\s*=.*\n','\n',data,flags=re.M)
data=re.sub(r'^\s*\.sdm\s*= _SUNXI_CCU_SDM_INFO.*\n','\n',data,flags=re.M)
data=data.replace('_SUNXI_CCU_MULT_MIN_MAX(8, 8, 52, 105)', '_SUNXI_CCU_MULT_OFFSET_MIN_MAX(8, 8, 1, 52, 105)')
# The T153 peripheral PLL has only N, no M field. The upstream NM setter
# writes both fields and cannot represent a missing M divider safely.
# Use the upstream N-only implementation with the same 52..105 limits.
data=data.replace('struct ccu_nm', 'struct ccu_mult').replace('&ccu_nm_ops', '&ccu_mult_ops')
data=re.sub(r'\.n(\s*=)', r'.mult\1', data)
data=re.sub(r'^\s*\.(?:min_rate|max_rate)\s*=.*\n', '\n', data, flags=re.M)

# PLL output and lock detector need to stay on with the enable bit.
data=re.sub(r'\.enable\s*= BIT\(31\),', '.enable = BIT(31) | BIT(29) | BIT(27),', data)
data=data.replace('CLK_SET_RATE_UNGATE |\n\t\t\t\t\t0','CLK_SET_RATE_UNGATE | CLK_IS_CRITICAL')
# These MMC controllers use two linear divisors, not the older 2^P divisor.
def mmc_div(m):
    i,reg=m.group(1),m.group(2)
    return f'''static SUNXI_CCU_M_WITH_MUX_GATE(smhc{i}_m_clk, "smhc{i}-m",
			smhc{i}_parents, {reg}, 0, 5, 24, 3, BIT(31), 0);
static SUNXI_CCU_M(smhc{i}_clk, "smhc{i}", "smhc{i}-m",
			{reg}, 8, 5, CLK_SET_RATE_PARENT);
'''
data=re.sub(r'static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX\(smhc([0123])_clk,.*?parents, (0x[0-9A-Fa-f]+),.*?;\n',mmc_div,data,flags=re.S)
assert 'NO_INDEX' not in data
names=set(re.findall(r'^static (?:SUNXI_CCU_\w+|CLK_FIXED_FACTOR)\((\w+)',data,re.M))
names.update(re.findall(r'static struct ccu_mult (\w+)',data))
common=sorted(x for x in names if re.search(r'(?:SUNXI_CCU_\w+\('+x+r'\b|struct ccu_mult '+x+r'\b)',data))
entries=[]
for key,var,field in re.findall(r'\[(CLK_\w+)\]\s*= &(\w+)\.(common.hw|hw)',ccu):
    if var in names: entries.append(f'\t\t[{key}] = &{var}.{field},')
# ccu_clks assigns MMIO/locks only. Every newly introduced clock stage must
# also be in hw_clks so sunxi_ccu_probe actually registers its clk_hw.
internal = [(f'CLK_SMHC{i}_M_INTERNAL', f'smhc{i}_m_clk') for i in range(4)]
internal.append(('CLK_CAN_MBUS_CFG_INTERNAL', 'can_mbus_cfg_clk'))
registered = set(re.findall(r'&([a-zA-Z0-9_]+)\.', '\n'.join(entries)))
assert names - registered == {var for _, var in internal}, 'unexpected unregistered clock stage'
entries = [f'\t\t[{key}] = &{var}.common.hw,' for key, var in internal] + entries
internal_enum = '''/* Internal stages also need registration: ccu_clks only assigns MMIO and
 * the common lock. Keep these indices outside the existing DT clock ABI.
 */
enum {
\tCLK_SMHC0_M_INTERNAL = CLK_NUMBER,
\tCLK_SMHC1_M_INTERNAL,
\tCLK_SMHC2_M_INTERNAL,
\tCLK_SMHC3_M_INTERNAL,
\tCLK_CAN_MBUS_CFG_INTERNAL,
\tCLK_T153_HW_NUMBER,
};
'''
reset=ccu.split('static struct ccu_reset_map sun8iw22_ccu_resets[] = {',1)[1].split('\n};',1)[0]
# Expose the original register reset map; no reset is asserted until a consumer asks.
body='''// SPDX-License-Identifier: GPL-2.0
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

'''+data+'\nstatic struct ccu_reset_map t153_resets[] = {'+reset+'''\n};
'''+internal_enum+'''static struct clk_hw_onecell_data t153_hw_clks = {
	.hws = {
'''+ '\n'.join(entries)+'''
	},
	.num = CLK_T153_HW_NUMBER,
};
static struct ccu_common *t153_clks[] = {
'''+''.join('\t&'+x+'.common,\n' for x in common)+'''};
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
'''
write(FILES/'drivers/clk/sunxi-ng/ccu-sun8i-t153.c',body)
for kind in ['clock','reset']:
    hdr=(REF/f'bsp/include/dt-bindings/{kind}/sun8iw22-ccu.h').read_text().replace('SUN8IW22','SUN8I_T153')
    if kind=='clock': hdr=hdr.replace('#endif', '#define CLK_NUMBER CLK_MAX_NO\n\n#endif')
    write(FILES/f'include/dt-bindings/{kind}/sun8i-t153-ccu.h',hdr)
kp='drivers/clk/sunxi-ng/Kconfig';mp='drivers/clk/sunxi-ng/Makefile'
k=(REF/'mainline'/kp).read_text()+'''\nconfig SUN8I_T153_CCU
	bool "Support for the Allwinner T153 CCU"
	depends on MACH_SUN8I || COMPILE_TEST
'''
m=(REF/'mainline'/mp).read_text()+'\nobj-$(CONFIG_SUN8I_T153_CCU) += ccu-sun8i-t153.o\n'
patch('501-clk-sunxi-ng-add-T153-core-and-IO-clocks.patch',{kp:k,mp:m})

kp='drivers/net/ethernet/stmicro/stmmac/Kconfig';mp='drivers/net/ethernet/stmicro/stmmac/Makefile'
k=(REF/'mainline'/kp).read_text()+'''\nconfig DWMAC_SUN8I_T153
	tristate "Allwinner T153 GMAC-211 support"
	depends on STMMAC_PLATFORM && (ARCH_SUNXI || COMPILE_TEST)
'''
m=(REF/'mainline'/mp).read_text()+'\nobj-$(CONFIG_DWMAC_SUN8I_T153) += dwmac-sun8i-t153.o\n'
patch('502-net-stmmac-add-T153-glue.patch',{kp:k,mp:m})

mp='arch/arm/mach-sunxi/sunxi.c'
m=(REF/'mainline'/mp).read_text()+'''\n/* T153 has a GIC and an architectural timer; use generic PSCI SMP. */
static const char * const t153_board_dt_compat[] = {
	"allwinner,sun8i-t153",
	NULL,
};

DT_MACHINE_START(T153_DT, "Allwinner T153")
	.dt_compat = t153_board_dt_compat,
MACHINE_END
'''
dp='arch/arm/boot/dts/allwinner/Makefile'
d=(REF/'mainline'/dp).read_text()+'\ndtb-$(CONFIG_MACH_SUN8I) += sun8i-t153-dshanpi-omnigate.dtb\n'
patch('503-ARM-add-T153-machine-and-OmniGate-DTB.patch',{mp:m,dp:d})

mp='drivers/mmc/host/sunxi-mmc.c'
m=(REF/'mainline'/mp).read_text()
needle='\tbool ccu_has_timings_switch;'
assert m.count(needle) == 1
m=m.replace(needle, needle+'''

\t/* T153 v5.3x has a fixed divide-by-two before the card clock. */
\tbool fixed_divide_by_two;

\t/* Some controllers require a literal length, including the segment cap. */
\tbool idma_des_size_literal;''')
needle='\t\tif (data->sg[i].length == max_len)'
assert m.count(needle) == 1
m=m.replace(needle, '''\t\t/* T153 limits segments to 4 KiB but encodes 4096 literally. */
\t\tif (data->sg[i].length == max_len &&
\t\t    !host->cfg->idma_des_size_literal)''')
needle='\t\thost->int_sum |= msk_int;'
assert m.count(needle) == 1
m=m.replace(needle, needle+'''

\t\tif (host->cfg->idma_des_size_literal && mrq->data &&
\t\t    (host->int_sum & SDXC_INTERRUPT_ERROR_BIT)) {
\t\t\tstruct sunxi_idma_des *des = host->sg_cpu;

\t\t\tdev_err_ratelimited(mmc_dev(host->mmc),
\t\t\t\t"cmd %u data error: irq=%#x idma=%#x blocks=%u clock=%u desc0=%#x/%#x/%#x/%#x\\n",
\t\t\t\tmrq->cmd->opcode, host->int_sum, idma_int,
\t\t\t\tmrq->data->blocks, host->mmc->actual_clock,
\t\t\t\tle32_to_cpu(des[0].config),
\t\t\t\tle32_to_cpu(des[0].buf_size),
\t\t\t\tle32_to_cpu(des[0].buf_addr_ptr1),
\t\t\t\tle32_to_cpu(des[0].buf_addr_ptr2));
\t\t}''')
needle='\tif (host->use_new_timings && host->cfg->ccu_has_timings_switch) {'
assert m.count(needle) == 1
m=m.replace(needle, '''\t/* This divider is present in SDR as well as DDR on T153. */
\tif (host->cfg->fixed_divide_by_two)
\t\tclock <<= 1;

'''+needle)
needle='\t/* update card clock rate to account for internal divider */\n\trate /= div;'
assert m.count(needle) == 1
m=m.replace(needle, '''\t/* Account for both the programmable and T153 fixed dividers. */
\trate /= div;
\tif (host->cfg->fixed_divide_by_two)
\t\trate /= 2;''')
m=m.replace('/* New registers introduced in A64 */',
            '#define SDXC_REG_VERS\t\t0x300 /* T153 controller hardware version */\n\n/* New registers introduced in A64 */')
needle='static const struct of_device_id sunxi_mmc_of_match[] = {'
assert needle in m
m=m.replace(needle, '''/* T153 SMHC blocks use at most 4 KiB per descriptor buffer. The address
 * unit is selected from the controller version after its bus clock is on.
 * High-speed modes still need board-specific timing qualification.
 */
static const struct sunxi_mmc_cfg sun8i_t153_mmc_cfg = {
	.fixed_divide_by_two = true,
	.idma_des_size_literal = true,
	.idma_des_size_bits = 12,
	.can_calibrate = true,
	.needs_new_timings = true,
	.mask_data0 = true,
};

'''+needle+'''
	{ .compatible = "allwinner,sun8i-t153-emmc", .data = &sun8i_t153_mmc_cfg },
	{ .compatible = "allwinner,sun8i-t153-mmc", .data = &sun8i_t153_mmc_cfg },''')
needle='\thost->irq = platform_get_irq(pdev, 0);'
assert needle in m
m=m.replace(needle, '''	if (host->cfg == &sun8i_t153_mmc_cfg) {
		struct sunxi_mmc_cfg *cfg;
		u32 version = mmc_readl(host, REG_VERS) & 0xffffff;

		cfg = devm_kmemdup(&pdev->dev, host->cfg, sizeof(*cfg), GFP_KERNEL);
		if (!cfg) {
			ret = -ENOMEM;
			goto error_disable_mmc;
		}
		/* See the v4p6x/v5p3x address-unit rules in the T153 BSP. */
		cfg->idma_des_shift = (version == 0x40502 || version >= 0x50300) ? 2 : 0;
		host->cfg = cfg;
		dev_info(&pdev->dev, "SMHC version %#x, DMA address shift %u\\n",
			 version, cfg->idma_des_shift);
	}

'''+needle)
# The mainline driver advertises DDR automatically; explicitly restrict this
# unqualified board port even if a user adds high-speed properties to its DT.
needle='\t/* TODO: This driver doesn\'t support HS400 mode yet */'
assert needle in m
m=m.replace(needle, '''	if (of_device_is_compatible(pdev->dev.of_node, "allwinner,sun8i-t153-emmc") ||
	    of_device_is_compatible(pdev->dev.of_node, "allwinner,sun8i-t153-mmc")) {
		mmc->caps &= ~(MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED |
			       MMC_CAP_3_3V_DDR | MMC_CAP_1_8V_DDR |
			       MMC_CAP_1_2V_DDR | MMC_CAP_UHS);
		mmc->caps2 &= ~(MMC_CAP2_HS200 | MMC_CAP2_HS400);
		mmc->f_max = min(mmc->f_max, 25000000U);
	}

'''+needle)
patch('504-mmc-sunxi-add-T153-SMHC2.patch',{mp:m})
kp='drivers/phy/allwinner/Kconfig';mp='drivers/phy/allwinner/Makefile'
k=(REF/'mainline'/kp).read_text()+'''\nconfig PHY_SUN8I_T153_USB
	tristate "Allwinner T153 USB2 host PHY"
	depends on ARCH_SUNXI || COMPILE_TEST
	depends on OF && HAS_IOMEM
	select GENERIC_PHY
'''
m=(REF/'mainline'/mp).read_text()+'\nobj-$(CONFIG_PHY_SUN8I_T153_USB) += phy-sun8i-t153-usb.o\n'
patch('505-phy-add-T153-USB2-host-PHY.patch',{kp:k,mp:m})
print(f'Generated {len(names)} clocks and T153 pinctrl tables')
