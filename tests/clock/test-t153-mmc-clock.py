#!/usr/bin/env python3
"""Check CCU registration coverage and cold-boot MMC oscillator rates.

No board MMIO. This catches the missing internal-clock registrations which made
all four SMHC clocks orphans and caused ccu_mux_helper_determine_rate -> EINVAL.
The rate cases use only the 24MHz crystal, independent of firmware PLL setup.
"""
import json
import re
from pathlib import Path
root = Path(__file__).resolve().parents[2]
src = (root/'port/target/linux/sunxi/files/drivers/clk/sunxi-ng/ccu-sun8i-t153.c').read_text()
common_body = src.split('static struct ccu_common *t153_clks[] = {', 1)[1].split('};', 1)[0]
hw_body = src.split('static struct clk_hw_onecell_data t153_hw_clks = {', 1)[1].split('};', 1)[0]
common = set(re.findall(r'&(\w+)\.common', common_body))
registered = set(re.findall(r'&(\w+)\.common\.hw', hw_body))
assert not (common - registered), ('CCU stage never registered', sorted(common - registered))
for i in range(4):
    assert f'smhc{i}_m_clk' in registered
    assert re.search(rf'SUNXI_CCU_M\(smhc{i}_clk, "smhc{i}", "smhc{i}-m",\s*0x0D{i}0, 8, 5, CLK_SET_RATE_PARENT\)', src)
    assert re.search(rf'SUNXI_CCU_M_WITH_MUX_GATE\(smhc{i}_m_clk, "smhc{i}-m",\s*smhc{i}_parents, 0x0D{i}0, 0, 5, 24, 3, BIT\(31\), 0\)', src)
    assert re.search(rf'smhc{i}_parents\[\] = \{{ "dcxo24M",', src)
# Regression sensitivity: the pre-fix lists missed exactly these five stages.
missing = {f'smhc{i}_m_clk' for i in range(4)} | {'can_mbus_cfg_clk'}
assert common - (registered - missing) == missing

# T153 M and N are both 5-bit LINEAR divisors, not old sunxi power-of-two N.
# New-timing SDR adds a fixed /2 in the controller: request 2*card clock.
def choose(card_hz, parents=(24_000_000,)):
    wanted = card_hz * 2
    choices = [(p // m // n, p, m, n) for p in parents
               for m in range(1, 33) for n in range(1, 33)
               if p // m // n <= wanted]
    module, parent, m, n = max(choices)
    return module // 2, parent, m, n
cases = []
for requested in (100_000, 200_000, 300_000, 400_000):
    actual, parent, m, n = choose(requested)
    assert actual == requested and parent == 24_000_000
    cases.append({'card_hz': requested, 'module_hz': requested * 2,
                  'parent_hz': parent, 'm': m, 'n': n})
assert choose(25_000_000)[0] == 12_000_000  # safe when PLL is unavailable
assert choose(25_000_000, (24_000_000, 600_000_000))[0] == 25_000_000
patch = (root/'port/target/linux/sunxi/patches-6.12/504-mmc-sunxi-add-T153-SMHC2.patch').read_text()
assert '+\t.fixed_divide_by_two = true,' in patch
assert '+\tif (host->cfg->fixed_divide_by_two)\n+\t\tclock <<= 1;' in patch
assert '+\tif (host->cfg->fixed_divide_by_two)\n+\t\trate /= 2;' in patch
report = {'status': 'pass', 'ccu_common_stages': len(common),
          'all_common_stages_registered': True,
          'oscillator_only_identification_rates': cases,
          'oscillator_only_max_card_hz': 12_000_000,
          'pll_600m_max_card_hz': 25_000_000, 'hardware_test': False}
(root/'logs/t153-mmc-clock-check.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(report, indent=2))
