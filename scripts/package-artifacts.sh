#!/bin/sh
# Package only verified software outputs; never access a block device.
set -eu
port_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
owrt=${1:-"$port_dir/openwrt"}
legacy=true
case "${2:-}" in
    '') ;;
    --openwrt-only) legacy=false ;;
    *) echo 'Usage: scripts/package-artifacts.sh [OPENWRT_DIR] [--openwrt-only]' >&2; exit 2 ;;
esac
[ "$#" -le 2 ] || exit 2
owrt=$(CDPATH= cd -- "$owrt" && pwd)
python3 "$port_dir/scripts/verify-artifacts.py" "$owrt"
output="$port_dir/artifacts"
source="$owrt/bin/targets/sunxi/cortexa7"
mkdir -p "$output"
for file in "$source"/*dshanpi_t153mx-omnigate* "$source"/*rootfs.tar.gz "$source"/*.manifest "$source"/config.buildinfo "$source"/feeds.buildinfo "$source"/version.buildinfo; do
    [ -f "$file" ] || continue
    case "$file" in *-targz-rootfs.tar.gz) continue;; esac
    cp -p "$file" "$output/"
done
cp "$port_dir/logs/artifact-validation.json" "$output/validation.json"
cp "$port_dir/docs/BOOT-RAM.txt" "$output/BOOT-RAM.txt"
if [ "$legacy" = true ] && [ -f "$port_dir/u-boot-work/build-t153-ram/u-boot.bin" ]; then
    python3 "$port_dir/scripts/package-uboot.py"
    cp "$port_dir/docs/BOOT-UBOOT-RAM.txt" "$output/BOOT-UBOOT-RAM.txt"
fi
if [ "$legacy" = true ] && [ -f "$port_dir/u-boot-work/build-t153-source/t153-source-spl.bin" ]; then
    python3 "$port_dir/scripts/package-uboot-source.py"
    sh "$port_dir/scripts/build-installer-tools.sh"
    python3 "$port_dir/scripts/package-disk-image.py"
    python3 "$port_dir/scripts/package-fel-installer.py"
    cp "$port_dir/docs/FLASH-SOURCE.txt" "$output/FLASH-SOURCE.txt"
fi
cp "$port_dir/docs/FLASH-SOURCE.txt" "$output/FLASH-SOURCE.txt"
cp "$port_dir/docs/FES-REPRODUCE.txt" "$output/FES-REPRODUCE.txt"
# Source overlay only: no Tina code tree, no build caches, no SSH credentials.
tar -czf "$output/t153-openwrt25-port.tar.gz" \
    --exclude='__pycache__' --exclude='*.pyc' \
    --exclude='tests/diagnostics/t153-phy-loop-packets' \
    --exclude='tests/diagnostics/t153-phy-loop-packets.host' \
    -C "$port_dir" README COPYING LICENSES port scripts configs docs tests host-tools
(cd "$output" && find . -maxdepth 1 -type f ! -name SHA256SUMS -printf '%f\n' | LC_ALL=C sort | xargs -d '\n' sha256sum >SHA256SUMS)
printf '%s\n' "Verified experimental artifacts: $output (this packer does not perform hardware validation)"
