#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
mkdir -p "$root/u-boot-work/host-tests"
cc -O2 -g -Wall -Wextra -Werror -fsanitize=undefined,address \
    -I"$root/tests/ddr" "$root/tests/ddr/test-ddr.c" \
    -o "$root/u-boot-work/host-tests/test-ddr"
"$root/u-boot-work/host-tests/test-ddr"
