#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# Build an SD/eMMC user-area image using the normal OpenWrt host tools.
# This creates a regular file, never writes a device, and does not use FES.
set -eu

[ "$#" -eq 6 ] || {
	echo "Usage: $0 <image> <kernel FIT> <rootfs.ext4> <root MiB> <SPL+U-Boot> <boot script>" >&2
	exit 1
}
output="$1"
kernel="$2"
rootfs="$3"
root_mib="$4"
uboot="$5"
bootscript="$6"

fail() {
	echo "T153 image: $*" >&2
	exit 1
}

# Do not allow a typo in a standalone invocation to select a block device.
[ ! -L "$output" ] && { [ ! -e "$output" ] || [ -f "$output" ]; } || fail "output must be a regular file"
for input in "$kernel" "$rootfs" "$uboot" "$bootscript"; do
	[ -f "$input" ] || fail "missing input: $input"
	[ ! -e "$output" ] || [ ! "$output" -ef "$input" ] || fail "output aliases an input"
done
case "$root_mib" in ''|*[!0-9]*) fail "root filesystem size must be an integer in MiB";; esac
[ "$root_mib" -ge 16 ] && [ "$root_mib" -le 2097115 ] || fail "root filesystem size outside MBR limits"
root_bytes=$((root_mib * 1024 * 1024))
[ "$(stat -c %s "$rootfs")" -eq "$root_bytes" ] || fail "ext4 size differs from configured root partition"
[ "$(od -An -tx1 -j 1080 -N 2 "$rootfs" | tr -d ' \n')" = 53ef ] || fail "root filesystem is not ext4"
[ "$(od -An -tc -j 4 -N 8 "$uboot" | tr -d ' \n')" = eGON.BT0 ] || fail "SPL eGON header missing"
[ "$(od -An -tx1 -j 122880 -N 4 "$uboot" | tr -d ' \n')" = 27051956 ] || fail "U-Boot proper must begin at relative 0x1e000"
[ "$(stat -c %s "$uboot")" -lt 4186112 ] || fail "bootloader overlaps boot partition"
[ "$(od -An -tx1 -N 4 "$kernel" | tr -d ' \n')" = d00dfeed ] || fail "kernel must be a FIT image"

work="$(mktemp -d "${output}.work.XXXXXX")"
trap 'rm -rf "$work"' EXIT
trap 'exit 1' HUP INT TERM
export TZ=UTC

# mkimage consumes SOURCE_DATE_EPOCH. FAT timestamps also need a fixed value.
file_epoch="${SOURCE_DATE_EPOCH:-315532800}"
[ "$file_epoch" -ge 315532800 ] || file_epoch=315532800

# make_ext4fs leaves inode-bitmap padding unset. Normalize a private copy with
# OpenWrt's staged e2fsck; never change the rootfs build artifact. Preen permits
# only automatically repairable metadata, followed by a strict read-only pass.
cp "$rootfs" "$work/root.ext4"
fsck_status=0
E2FSCK_TIME="$file_epoch" E2FSPROGS_FAKE_TIME="$file_epoch" e2fsck -p -f "$work/root.ext4" || fsck_status=$?
case "$fsck_status" in
	0|1) ;;
	*) fail "rootfs metadata normalization failed (e2fsck status $fsck_status)" ;;
esac
E2FSCK_TIME="$file_epoch" E2FSPROGS_FAKE_TIME="$file_epoch" e2fsck -f -n "$work/root.ext4"

cp "$kernel" "$work/openwrt-t153.itb"
mkimage -A arm -O linux -T script -C none -a 0 -e 0 \
	-n 'T153 persistent boot' -d "$bootscript" "$work/boot.scr"
touch -d "@$file_epoch" "$work/boot.scr" "$work/openwrt-t153.itb"
mkfs.fat --invariant -F 16 -S 512 -h 8192 -i 15302512 -n boot \
	-C "$work/boot.fat" 32768
mcopy -m -i "$work/boot.fat" "$work/boot.scr" ::boot.scr
mcopy -m -i "$work/boot.fat" "$work/openwrt-t153.itb" ::openwrt-t153.itb
fsck.fat -n "$work/boot.fat"

# ptgen reserves a track before every partition and cannot express our
# adjacent FAT (4..36 MiB) and rootfs (36 MiB..) layout. Keep the already
# validated layout with a small standard-library MBR writer instead.
python3 - "$work/disk.img" "$root_mib" <<'PY_MBR'
import struct
import sys

root_sectors = int(sys.argv[2]) * 2048
mbr = bytearray(512)
struct.pack_into('<I', mbr, 440, 0x15302512)
for number, start, sectors, kind in ((0, 8192, 65536, 0x0e),
                                    (1, 73728, root_sectors, 0x83)):
    struct.pack_into('<B3sB3sII', mbr, 446 + number * 16,
                     0x80 if number == 0 else 0, b'\xfe\xff\xff', kind,
                     b'\xfe\xff\xff', start, sectors)
mbr[510:512] = b'\x55\xaa'
with open(sys.argv[1], 'wb') as image:
    image.write(mbr)
    image.truncate((73728 + root_sectors) * 512)
PY_MBR

dd if="$uboot" of="$work/disk.img" bs=1024 seek=8 conv=notrunc
# Absolute offsets: SPL 8 KiB, U-Boot proper 128 KiB, FAT 4 MiB, root 36 MiB.
dd if="$work/boot.fat" of="$work/disk.img" bs=1048576 seek=4 conv=notrunc
dd if="$work/root.ext4" of="$work/disk.img" bs=1048576 seek=36 conv=notrunc
[ "$(stat -c %s "$work/disk.img")" -eq "$((root_bytes + 37748736))" ] || fail "incorrect image length"
mv -f "$work/disk.img" "$output"
