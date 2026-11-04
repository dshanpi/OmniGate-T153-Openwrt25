#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
mkdir -p "$root/u-boot-work/host-tests"
for size_mib in 512 1024; do
    output="$root/u-boot-work/host-tests/test-ddr-${size_mib}"
    cc -O2 -g -Wall -Wextra -Werror -fsanitize=undefined,address \
        -DCONFIG_T153_DDR_SIZE_MIB="$size_mib" \
        -I"$root/tests/ddr" "$root/tests/ddr/test-ddr.c" -o "$output"
    printf 'DDR model: %s MiB\n' "$size_mib"
    "$output"
done
