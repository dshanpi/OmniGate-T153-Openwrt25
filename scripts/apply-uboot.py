#!/usr/bin/env python3
"""Apply the T153 RAM/source-SPL overlay to the pinned upstream U-Boot release."""
import shutil
import sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = Path(sys.argv[1]).resolve()
make = (source / 'Makefile').read_text()
for line in ('VERSION = 2026', 'PATCHLEVEL = 07', 'SUBLEVEL =', 'EXTRAVERSION ='):
    if line not in make.splitlines():
        raise SystemExit('Expected unmodified U-Boot 2026.07 release')
for src in (root / 'port/uboot/files').rglob('*'):
    if src.is_file():
        dst = source / src.relative_to(root / 'port/uboot/files')
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(src, dst)
p = source / 'arch/arm/Kconfig'
s = p.read_text()
if 'config TARGET_DSHANPI_T153_RAM' not in s:
    s = s.replace('config TARGET_VEXPRESS_CA9X4', '''config TARGET_DSHANPI_T153_RAM
    bool "DshanPI T153MX OmniGate (RAM chainload only)"
    select CPU_V7A
    help
      Experimental U-Boot proper for already initialized 512 MiB DRAM.
      Does not include DDR initialization, SPL or a cold-boot image.

config TARGET_VEXPRESS_CA9X4''')
if 'source "board/dshanpi/t153/Kconfig"' not in s:
    s += '\nsource "board/dshanpi/t153/Kconfig"\n'
p.write_text(s)
if 'config TARGET_DSHANPI_T153_SOURCE' not in s:
    s = s.replace('config TARGET_DSHANPI_T153_RAM', 'config TARGET_DSHANPI_T153_SOURCE\n    bool "DshanPI T153MX OmniGate (experimental source-only SPL)"\n    select CPU_V7A\n    select SUPPORT_SPL\n    help\n      Source-only DDR4 training, FEL return and read-only MMC loading.\n      Board bring-up target; not yet validated on hardware.\n\nconfig TARGET_DSHANPI_T153_RAM')
p.write_text(s)
p = source / 'arch/arm/dts/Makefile'
s = p.read_text()
line = 'dtb-$(CONFIG_TARGET_DSHANPI_T153_RAM) += sun8i-t153-dshanpi-omnigate-uboot.dtb'
if line not in s:
    s = line + '\n' + s
p.write_text(s)
print('Applied experimental T153 U-Boot overlay:', source)

line = 'dtb-$(CONFIG_TARGET_DSHANPI_T153_SOURCE) += sun8i-t153-dshanpi-omnigate-uboot.dtb'
if line not in s:
    p.write_text(line+'\n'+s)
