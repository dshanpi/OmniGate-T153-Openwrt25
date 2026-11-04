#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# Default is an offline plan. --boot loads RAM only; no storage write is issued.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
case "${1:-}" in
    '') [ "$#" -eq 0 ] || exit 2; mode=--plan ;;
    --plan|--boot) [ "$#" -eq 1 ] || exit 2; mode=$1 ;;
    -h|--help)
        printf '%s\n' 'Usage: scripts/boot-fel-installer.sh [--plan|--boot]' \
            'Default: validate artifacts offline. --boot: load RAM via FEL; never writes SD/eMMC.'
        exit 0 ;;
    *) echo 'Expected --plan or --boot' >&2; exit 2 ;;
esac
exec python3 "$root/scripts/package-fel-installer.py" "$mode"
