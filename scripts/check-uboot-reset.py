#!/usr/bin/env python3
"""Offline cross-check T153 reset constants against the SDK; never access hardware."""
from pathlib import Path
import hashlib, json, os, re
root=Path(__file__).resolve().parents[1]
sdk=Path(os.environ.get('SDK_ROOT',str(root.parent)))
overlay=root/'port/uboot/files/board/dshanpi/t153'
source=root/'u-boot-work/u-boot-2026.07/board/dshanpi/t153'
bsp=sdk/'brandy/brandy-2.0/u-boot-bsp'
code=(overlay/'reset.c').read_text()
macros={k:int(v.rstrip('UuLl'),0) for k,v in re.findall(r'^#define\s+(T153_\w+)\s+(0x[0-9a-fA-F]+[UuLl]*)',code,re.M)}
dts=(bsp/'arch/arm/dts/sun8iw22p1.dtsi').read_text()
node=dts.split('watchdog: watchdog@',1)[1].split('};',1)[0]
values={k:int(v,0) for k,v in re.findall(r'(wdt_\w+)\s*=\s*<(0x[0-9a-fA-F]+|\d+)>',node)}
assert macros['T153_WDT_BASE']==int(re.search(r'reg\s*=\s*<(0x[0-9a-fA-F]+)',node).group(1),0)
for macro,key in [('T153_WDT_CTRL','wdt_ctrl'),('T153_WDT_CFG','wdt_cfg'),('T153_WDT_MODE','wdt_mode'),('T153_WDT_OUT_CFG','wdt_ocfg'),('T153_WDT_KEY','wdt_key_val'),('T153_WDT_OUT_MASK','wdt_out_mask'),('T153_WDT_OUT_RESET','wdt_out_val'),('T153_WDT_RESET_MASK','wdt_reset_mask'),('T153_WDT_SYSTEM_RESET','wdt_reset_val')]:
    assert macros[macro]==values[key], (macro,key)
assert macros['T153_WDT_TIMEOUT_MASK']==(0xf << values['wdt_timeout_shift'])
assert macros['T153_WDT_RELOAD']==((0x0a57<<1)|1)
assert macros['T153_WDT_ENABLE']==1
assert 'clocks = <&osc24M>;' in node and 'resets' not in node
rtc=(bsp/'arch/arm/mach-sunxi/rtc.c').read_text()
assert re.search(r'#define EFEX_FLAG\s+\(0x5AA5A55A\)',rtc)
assert re.search(r'#define RTC_FEL_INDEX\s+2\b',rtc)
cpu=(bsp/'arch/arm/include/asm/arch/plat-sun8iw22p1/cpu_autogen.h').read_text()
rtc_base=int(re.search(r'#define SUNXI_RTC_BASE\s+(0x[0-9a-fA-F]+)',cpu).group(1),0)
assert macros['T153_RTC_FEL_REG']==rtc_base+0x100+2*4
assert macros['T153_FEL_MAGIC']==0x5aa5a55a
for filename in ['board.c','Makefile','reset.c']:
    assert (overlay/filename).read_bytes()==(source/filename).read_bytes(), filename
assert 'reset.o' in (overlay/'Makefile').read_text()
assert 'void reset_cpu' not in (overlay/'board.c').read_text()
assert 'efex, 1, 0, do_t153_efex' in code
assert 't153_restart(0)' in code and 't153_restart(1)' in code
assert 'if (!fel)' in code and 'if (fel)' in code
assert 'disable_interrupts()' in code and 'enable_interrupts()' in code
assert 'elapsed < 2000' in code and 'retry < 1000' in code
assert '#ifndef CONFIG_XPL_BUILD' in code
for config in ['dshanpi_t153_source_defconfig','dshanpi_t153_source_1g_defconfig','dshanpi_t153_ram_defconfig']:
    conf=(root/'port/uboot/files/configs'/config).read_text()
    assert 'CONFIG_TARGET_DSHANPI_T153_' in conf
apply=(root/'scripts/apply-uboot.py').read_text()
assert "port/uboot/files" in apply and '.rglob(' in apply
report={'status':'offline SDK/register/overlay checks passed; compiled and hardware validation pending',
        'source':'port/uboot/files/board/dshanpi/t153/reset.c','sha256':hashlib.sha256(code.encode()).hexdigest(),
        'register_constants':{k:hex(v) for k,v in macros.items()},
        'rtc_magic_scope':'Only efex sets it; ordinary reset clears the exact magic; failed efex attempts cleanup.',
        'clock_evidence':'T153 SDK DTS uses osc24M without CCU gate/reset; vendor watchdog probe has no clock/reset API.',
        'fes_magic_consumption':'Stock FES1 main 0x52054 checks RTC via 0x4ae28, then 0x5205a calls 0x4aea0 -> 0x4ae0c to clear RTC index 2 with readback. The 933 MHz candidate preserves this code.',
        'limits':['No hardware access or compilation performed.','RTC backup retention across power-off is not established and is not assumed.','Actual reset, efex USB FEL enumeration and post-FES cold boot remain required.']}
print(json.dumps(report,indent=2))
