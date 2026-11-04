#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
exec 9>"$root/.uboot-build.lock"
flock -n 9 || { echo 'Another U-Boot build is active' >&2; exit 1; }
src="$root/u-boot-work/u-boot-2026.07"
out="$root/u-boot-work/build-t153-source-1g"
archive="$root/openwrt/dl/u-boot-2026.07.tar.bz2"
sha=78e8bfc382fe388f9b55aa1daf8c563522a037779b5d4c349d1415e381f1243e
if [ ! -f "$archive" ]; then
    mkdir -p "$(dirname -- "$archive")"
    curl -fL --retry 3 https://ftp.denx.de/pub/u-boot/u-boot-2026.07.tar.bz2 -o "$archive.part"
    printf '%s  %s\n' "$sha" "$archive.part" | sha256sum -c -
    mv "$archive.part" "$archive"
fi
printf '%s  %s\n' "$sha" "$archive" | sha256sum -c -
if [ ! -d "$src" ]; then
    mkdir -p "$root/u-boot-work"
    tar -xjf "$root/openwrt/dl/u-boot-2026.07.tar.bz2" -C "$root/u-boot-work"
fi
python3 "$root/scripts/apply-uboot.py" "$src"
if [ -z "${CROSS_COMPILE:-}" ]; then
    set -- "$root"/openwrt/staging_dir/toolchain-arm_cortex-a7*/bin/arm-openwrt-linux-muslgnueabi-gcc
    [ "$#" -eq 1 ] && [ -x "$1" ] || exit 1
    CROSS_COMPILE=${1%gcc}
fi
export CROSS_COMPILE STAGING_DIR="$root/openwrt/staging_dir"
export SOURCE_DATE_EPOCH=1782864000 KBUILD_BUILD_USER=builder KBUILD_BUILD_HOST=t153-mainline
make -C "$src" O="$out" ARCH=arm dshanpi_t153_source_1g_defconfig
make -C "$src" O="$out" ARCH=arm KCFLAGS=-fstack-usage -j"${JOBS:-4}"
"$out/tools/mkimage" -A arm -T sunxi_egon -n sun8i-t153-dshanpi-omnigate-uboot -d "$out/spl/u-boot-spl.bin" "$out/t153-source-1g-spl.bin"
