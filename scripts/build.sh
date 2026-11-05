#!/bin/sh
set -eu
port_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
openwrt_dir=${1:?Usage: build.sh /path/to/openwrt-25.12}
openwrt_dir=$(CDPATH= cd -- "$openwrt_dir" && pwd)
exec 9>"$openwrt_dir/.t153-build.lock"
flock -n 9 || { echo "Another T153 build is running" >&2; exit 1; }
sh "$port_dir/scripts/apply.sh" "$openwrt_dir"
sh "$port_dir/scripts/prepare-feeds.sh" "$openwrt_dir"
cd "$openwrt_dir"
./scripts/feeds update -a
# Install only selected feed packages and their dependencies. Installing the
# entire LuCI feed also exposes unrelated upstream Kconfig dependency cycles.
./scripts/feeds uninstall -a
./scripts/feeds install luci luci-ssl luci-proto-qmi luci-proto-mbim luci-proto-3g usbutils canutils picocom bluez-utils bluez-daemon
if [ -f .config ]; then
    cp -p .config ".config.before-t153.$(date +%Y%m%d%H%M%S)"
fi
cp "$port_dir/configs/t153mx-omnigate.config" .config
make defconfig
python3 "$port_dir/scripts/check-config.py" "$port_dir/configs/t153mx-omnigate.config" .config
make -j"${JOBS:-4}" download
make -j"${JOBS:-4}" V=s
