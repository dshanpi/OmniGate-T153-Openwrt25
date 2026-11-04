#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# Check host commands and optionally prepare the pinned upstream source.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
case "${1:-}" in
    --help|-h) echo 'Usage: scripts/bootstrap.sh [--check-only]'; exit 0 ;;
    --check-only|'') ;;
    *) echo 'Usage: scripts/bootstrap.sh [--check-only]' >&2; exit 2 ;;
esac
[ "$#" -le 1 ] || exit 2
missing=''
for command in cc g++ make patch git curl tar bzip2 xz gzip python3 rsync flock \
    awk flex bison msgfmt unzip zstd dtc mkimage mcopy mkfs.fat fsck.fat \
    e2fsck debugfs pkg-config; do
    command -v "$command" >/dev/null 2>&1 || missing="$missing $command"
done
if [ -n "$missing" ]; then
    echo "Missing host commands:$missing" >&2
    echo 'Install the documented Ubuntu dependencies in docs/BUILD-REPRODUCE.txt.' >&2
    exit 1
fi
python3 -c 'import sys; assert sys.version_info >= (3, 11), "Python 3.11+ required"'
pkg-config --exists libusb-1.0 || {
    echo 'Missing libusb-1.0 development files.' >&2
    exit 1
}
echo 'Host prerequisite command checks passed.'
[ "${1:-}" = --check-only ] || sh "$root/scripts/prepare-source.sh"
