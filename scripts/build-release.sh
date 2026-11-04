#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-only
# Reproducible software pipeline only: no USB/device access or Git mutations.
set -euo pipefail
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
jobs=${JOBS:-4}
incremental=false
with_fes=false
factory_image=''
while [ "$#" -gt 0 ]; do
    case "$1" in
        --jobs) [ "$#" -ge 2 ] || exit 2; jobs=$2; shift 2 ;;
        --incremental) incremental=true; shift ;;
        --with-fes) with_fes=true; shift ;;
        --factory-image) [ "$#" -ge 2 ] || exit 2; factory_image=$2; shift 2 ;;
        --help|-h)
            echo 'Usage: scripts/build-release.sh [--jobs N] [--incremental] [--with-fes [--factory-image PATH]]'
            echo 'Default refuses existing target build trees; final acceptance must use this clean mode.'
            echo 'FES uses a user-supplied hash-pinned factory image, or SDK_ROOT/out/t153_linux_omnigate_uart0-default.img.'
            exit 0 ;;
        *) echo "Unknown option: $1" >&2; exit 2 ;;
    esac
done
case "$jobs" in ''|*[!0-9]*|0) echo 'jobs must be a positive integer' >&2; exit 2;; esac
if [ "$incremental" = false ]; then
    for build in "$root/openwrt/build_dir" "$root/openwrt/staging_dir" \
        "$root/openwrt/tmp" "$root/openwrt/bin" "$root/openwrt/.config" \
        "$root/u-boot-work/build-t153-source-1g"; do
        if [ -e "$build" ]; then
            echo "Clean rebuild requires an unused build tree: $build exists." >&2
            echo 'Use a clean source checkout for final acceptance; --incremental is development-only.' >&2
            exit 1
        fi
    done
fi
export JOBS=$jobs
cd "$root"
mkdir -p logs
run_dir=$(mktemp -d "$root/logs/rebuild-$(date -u +%Y%m%dT%H%M%SZ)-XXXXXX")
printf '%s\n' "$run_dir" > logs/latest-rebuild-path.txt
run_step() {
    local name=$1
    shift
    echo "[$name] log: $run_dir/$name.log"
    if "$@" >"$run_dir/$name.log" 2>&1; then
        printf '0\n' >"$run_dir/$name.exit"
    else
        local status=$?
        printf '%s\n' "$status" >"$run_dir/$name.exit"
        tail -40 "$run_dir/$name.log" >&2
        exit "$status"
    fi
}
run_step prerequisites sh scripts/bootstrap.sh --check-only
run_step feed-cache python3 tests/build/test-feed-cache.py
run_step prepare-source sh scripts/prepare-source.sh
run_step openwrt sh scripts/build.sh "$root/openwrt"
run_step ddr-model sh scripts/test-ddr-source.sh
run_step mmc-clock python3 tests/clock/test-t153-mmc-clock.py
run_step reboot-command python3 tests/reboot/test-command.py
run_step reboot-kernel python3 tests/reboot/test_kernel_reboot.py
run_step uboot-source-1g sh scripts/build-uboot-source-1g.sh
run_step package-uboot-source-1g python3 scripts/package-uboot-source-1g.py
run_step package-common sh scripts/package-artifacts.sh "$root/openwrt" --openwrt-only
run_step package-disk-1g python3 scripts/package-disk-image.py --profile 1g
run_step verify-disk-1g python3 scripts/package-disk-image.py --profile 1g --verify-only
run_step disk-regression-1g env T153_DISK_PROFILE=1g python3 tests/flash/test-disk-image.py
if [ "$with_fes" = true ]; then
    fes_args=()
    [ -z "$factory_image" ] || fes_args+=(--factory-image "$factory_image")
    run_step fes-stock python3 scripts/prepare-fes-stock.py "${fes_args[@]}"
    run_step fes-ddr933 python3 scripts/package-fes-ddr933.py
    run_step fes-tests python3 tests/fes/test-portable-loader.py
fi
python3 - "$run_dir" "$incremental" <<'PY'
from pathlib import Path
import hashlib,json,subprocess,sys
root=Path.cwd();output=Path(sys.argv[1]);manifest=json.loads((root/'artifacts/t153-disk-1g-manifest.json').read_text())
r=subprocess.run(['git','rev-parse','HEAD'],capture_output=True,text=True)
report={'status':'software build passed; hardware acceptance is separate','source_commit':r.stdout.strip() if r.returncode==0 else None,
        'incremental':sys.argv[2]=='true','hardware_validated_by_this_run':False,'disk':manifest['image'],
        'config_sha256':hashlib.sha256((root/'openwrt/.config').read_bytes()).hexdigest(),
        'uboot_config_sha256':hashlib.sha256((root/'u-boot-work/build-t153-source-1g/.config').read_bytes()).hexdigest(),
        'steps':{p.stem:int(p.read_text()) for p in sorted(output.glob('*.exit'))}}
(output/'build-report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
PY
# The aggregate packer ran before explicit 1g/FES outputs; update only checksums now.
(cd artifacts && find . -maxdepth 1 -type f ! -name SHA256SUMS -printf '%f\n' | LC_ALL=C sort | xargs -d '\n' sha256sum >SHA256SUMS)
echo "Software pipeline complete. Hardware acceptance still required. Logs: $run_dir"
