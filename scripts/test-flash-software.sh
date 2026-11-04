#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# Offline only. No real block devices or USB devices are accessed by these tests.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"
python3 tests/flash/test-disk-image.py
python3 tests/flash/test_emmc_installer.py
python3 tests/flash/test-fel-installer.py
u-boot-work/sunxi-tools-660d7e08edd025c8f84ab8eba3d8a903a5b81ae0/test-header-swap
cc -g -Wall -Wextra -Werror -Dmain=mmc_info_main -c port/installer/mmc-info.c -o logs/mmc-info-test.o
cc -g -Wall -Wextra -Werror tests/flash/test-mmc-info.c logs/mmc-info-test.o \
    -Wl,--wrap=open,--wrap=fstat,--wrap=ioctl,--wrap=close -o logs/mmc-info-test
logs/mmc-info-test
# These execute the actual ARM functions via Unicorn, without device MMIO.
.venv/bin/python tests/flash/test-uboot-boot-source.py
.venv/bin/python tests/ddr/test-uboot-dt.py
python3 scripts/package-disk-image.py --verify-only
python3 scripts/package-fel-installer.py --verify-only
scripts/boot-fel-installer.sh
