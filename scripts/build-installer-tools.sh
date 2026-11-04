#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
set -- "$root"/openwrt/staging_dir/toolchain-arm_cortex-a7*/bin/arm-openwrt-linux-muslgnueabi-gcc
[ "$#" -eq 1 ] && [ -x "$1" ] || exit 1
export STAGING_DIR="$root/openwrt/staging_dir"
mkdir -p "$root/artifacts"
"$1" -Os -static -Wall -Wextra -Werror -Wl,--build-id=none -s \
    "$root/port/installer/mmc-info.c" -o "$root/artifacts/t153-mmc-info"
file "$root/artifacts/t153-mmc-info"
