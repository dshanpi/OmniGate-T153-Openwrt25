#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
commit=660d7e08edd025c8f84ab8eba3d8a903a5b81ae0
archive="$root/downloads/sunxi-tools-t153-660d7e08.tar.gz"
sha=8279709da499dd8100f6f071075ab9cfda0f1b068240fcbee1fe99116b745637
src="$root/u-boot-work/sunxi-tools-$commit"
mkdir -p "$root/downloads" "$root/u-boot-work" "$root/logs"
if [ ! -f "$archive" ]; then
    curl -fL --retry 3 "https://codeload.github.com/dlan17/sunxi-tools/tar.gz/$commit" -o "$archive.part"
    printf '%s  %s\n' "$sha" "$archive.part" | sha256sum -c -
    mv "$archive.part" "$archive"
fi
printf '%s  %s\n' "$sha" "$archive" | sha256sum -c -
[ -d "$src" ] || tar -xzf "$archive" -C "$root/u-boot-work"
python3 - "$src/fel_lib.c" <<'PY'
from pathlib import Path
import sys
p=Path(sys.argv[1]);s=p.read_text()
old='#define USB_TIMEOUT\t10000 /* 10 seconds */'
new='#define USB_TIMEOUT\t180000 /* 180 s: SRAM software DDR eye training */'
assert old in s or new in s, 'Unexpected pinned FEL source'
p.write_text(s.replace(old,new))
PY
patch_file="$root/port/uboot/sunxi-tools/0001-fel-use-swapped-SPL-header-for-boot-info.patch"
if patch --dry-run --batch --forward -d "$src" -p1 <"$patch_file" >/dev/null 2>&1; then
    patch --batch --forward -d "$src" -p1 <"$patch_file"
else
    # Repeated builds must see exactly the already applied patch.
    patch --dry-run --batch --reverse -d "$src" -p1 <"$patch_file" >/dev/null
fi
make -C "$src" -j4 sunxi-fel
# Exercise the real FEL C routines with an SRAM model, with USB unreachable.
cc -std=c99 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200112L -O1 -g \
    -ffunction-sections -fdata-sections -fsanitize=address,undefined \
    -I"$src" -I"$src/include" $(pkg-config --cflags libusb-1.0) \
    "$root/port/uboot/sunxi-tools/test-header-swap.c" "$src/soc_info.c" \
    -Wl,--gc-sections -fsanitize=address,undefined -o "$src/test-header-swap"
"$src/test-header-swap"
sha256sum "$src/sunxi-fel" >"$root/logs/fel-tool.sha256"
"$src/sunxi-fel" --help >/dev/null
printf '%s\n' "Built host tool: $src/sunxi-fel (T153 RFC, no device access)"
