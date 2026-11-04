#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Board-side installer for the fixed T153MX OmniGate user-area disk layout.
# No sysfs/proc/dev path overrides, test mode, vendor loader or automatic reboot.
set -eu
PATH=/usr/sbin:/usr/bin:/sbin:/bin
export PATH LC_ALL=C
unset ENV BASH_ENV CDPATH
umask 077
readonly IMAGE_BYTES=306184192 HEADER_BYTES=262144
readonly BOARD_COMPATIBLE=dshanpi,t153mx-omnigate
WORK= PRODUCER= IMAGE= DEVICE= EXPECT_CID= EXPECT_SHA= WRITE=0

fail() { echo "ERROR: $*" >&2; exit 1; }
info() { echo "T153_INSTALL: $*"; }
usage() {
    cat <<'USAGE'
Usage: t153-install-emmc.sh --device /dev/mmcblkX --expect-cid HEX32 \
       --image /path/t153.img[.gz] --sha256 RAW_IMAGE_SHA256 [--write]
Default: validate only; no device writes. --sha256 always names the uncompressed
292 MiB disk image, including when --image is gzip compressed. Copy/download the
image to RAM, or mount the source SD card read-only. Run from the dedicated RAM
system. --write overwrites the eMMC user area and verifies every written byte.
This tool never changes eMMC boot partitions/configuration or reboots the board.
USAGE
}
cleanup() {
    if [ -n "$PRODUCER" ]; then kill "$PRODUCER" 2>/dev/null || :; wait "$PRODUCER" 2>/dev/null || :; fi
    case "$WORK" in /tmp/t153-install.*) rm -rf -- "$WORK" ;; esac
}
image_stream() {
    if [ "$COMPRESSED" = 1 ]; then gzip -dc -- "$IMAGE"; else cat -- "$IMAGE"; fi
}
# Both sides are checked. POSIX shell pipelines alone hide producer failures.
stream_to() {
    action=$1
    rm -f "$WORK/fifo"
    mkfifo "$WORK/fifo" || fail 'cannot create bounded stream'
    image_stream >"$WORK/fifo" 2>"$WORK/producer.log" &
    PRODUCER=$!
    consumer_ok=0
    case "$action" in
        hash) sha256sum <"$WORK/fifo" >"$WORK/hash" && consumer_ok=1 ;;
        count) wc -c <"$WORK/fifo" >"$WORK/count" && consumer_ok=1 ;;
        header)
            # Drain the rest, so gzip reaches and verifies its CRC/footer.
            { dd bs=1 count="$HEADER_BYTES" 2>"$WORK/dd.log" >"$WORK/header" && cat >/dev/null; } <"$WORK/fifo" && consumer_ok=1 ;;
        write) dd of="$DEVICE" bs=1048576 iflag=fullblock oflag=direct conv=fsync <"$WORK/fifo" 2>"$WORK/dd.log" && consumer_ok=1 ;;
        *) fail 'internal stream action' ;;
    esac
    producer_ok=0
    wait "$PRODUCER" && producer_ok=1
    PRODUCER=
    [ "$producer_ok" = 1 ] && [ "$consumer_ok" = 1 ] || fail "image $action stream failed; no automatic retry"
    rm -f "$WORK/fifo"
}
hex_bytes() { hexdump -v -s "$1" -n "$2" -e '1/1 "%02x"' "$WORK/header"; }
le32() {
    hexdump -v -s "$1" -n 4 -e '4/1 "%u " "\n"' "$WORK/header" |
        awk '{ printf "%.0f\n", $1 + 256*$2 + 65536*$3 + 16777216*$4 }'
}
be32() {
    hexdump -v -s "$1" -n 4 -e '4/1 "%u " "\n"' "$WORK/header" |
        awk '{ printf "%.0f\n", 16777216*$1 + 65536*$2 + 256*$3 + $4 }'
}
validate_header() {
    [ "$(wc -c <"$WORK/header" | tr -d ' ')" = "$HEADER_BYTES" ] || fail 'truncated image header'
    [ "$(hex_bytes 510 2)" = 55aa ] || fail 'MBR signature mismatch'
    [ "$(hex_bytes 440 4)" = 12253015 ] || fail 'T153 disk identifier mismatch'
    [ "$(hex_bytes 446 1)" = 80 ] && [ "$(hex_bytes 462 1)" = 00 ] || fail 'MBR boot flags mismatch'
    [ "$(hex_bytes 450 1)" = 0e ] || fail 'partition 1 must be FAT16 LBA (0x0e)'
    [ "$(le32 454)" = 8192 ] && [ "$(le32 458)" = 65536 ] || fail 'partition 1 layout mismatch'
    [ "$(hex_bytes 466 1)" = 83 ] || fail 'partition 2 must be Linux (0x83)'
    [ "$(le32 470)" = 73728 ] && [ "$(le32 474)" = 524288 ] || fail 'partition 2 layout mismatch'
    [ "$(hex_bytes 478 32)" = 0000000000000000000000000000000000000000000000000000000000000000 ] || fail 'unexpected MBR partitions 3/4'
    [ "$(hex_bytes 8196 8)" = 65474f4e2e425430 ] || fail 'mainline eGON.BT0 SPL missing at 8 KiB'
    spl_length=$(le32 8208)
    [ "$spl_length" -ge 32 ] && [ "$spl_length" -le 122880 ] && [ "$((spl_length % 512))" = 0 ] || fail 'SPL length overlaps U-Boot or is not sector aligned'
    spl_expected=$(le32 8204)
    # T153/ARM is little-endian. eGON checksum replaces word 3 with STAMP_VALUE.
    spl_actual=$(hexdump -v -s 8192 -n "$spl_length" -e '1/4 "%u\n"' "$WORK/header" | awk 'BEGIN { n=0; sum=0 } { for (i=1;i<=NF;i++) { v=(n==3 ? 1594518585 : $i); sum=(sum+v)%4294967296; n++ } } END { printf "%.0f\n", sum }')
    [ "$spl_actual" = "$spl_expected" ] || fail 'eGON SPL checksum mismatch'
    [ "$(hex_bytes 131072 4)" = 27051956 ] || fail 'legacy U-Boot image missing at 128 KiB'
    [ "$(hex_bytes 131088 8)" = 4a0000004a000000 ] || fail 'U-Boot load/entry address mismatch'
    [ "$(hex_bytes 131100 4)" = 11020500 ] || fail 'U-Boot type/architecture/compression mismatch'
    uboot_length=$(be32 131084)
    [ "$uboot_length" -gt 0 ] && [ "$uboot_length" -le 4063168 ] || fail 'U-Boot exceeds reserved boot area'
}
validate_image() {
    [ -f "$IMAGE" ] && [ -r "$IMAGE" ] || fail 'image must be a readable regular file'
    IMAGE_SOURCE=$(readlink -f -- "$IMAGE") || fail 'cannot resolve image path'
    # Pin the input inode without copying a 292 MiB image into limited RAM.
    # Reopening this descriptor through proc starts each full traversal at zero.
    exec 3<"$IMAGE_SOURCE" || fail 'cannot open image'
    IMAGE=/proc/$$/fd/3
    COMPRESSED=0
    # Identify gzip from content, never solely from a filename extension.
    magic=$(hexdump -n 2 -e '1/1 "%02x"' "$IMAGE")
    [ "$magic" != 1f8b ] || COMPRESSED=1
    info 'checking complete image SHA-256 and gzip integrity (if compressed)'
    stream_to hash
    actual=$(awk '{print $1}' "$WORK/hash")
    [ "$actual" = "$EXPECT_SHA" ] || fail 'uncompressed image SHA-256 mismatch'
    stream_to count
    actual=$(tr -d ' \n' <"$WORK/count")
    [ "$actual" = "$IMAGE_BYTES" ] || fail "image size must be $IMAGE_BYTES bytes, got $actual"
    stream_to header
    validate_header
    info "image verified: raw_bytes=$IMAGE_BYTES raw_sha256=$EXPECT_SHA gzip=$COMPRESSED"
}
validate_mounts() {
    awk '$2=="/" { n++; if ($3!="rootfs" && $3!="tmpfs" && $3!="ramfs") bad=1 } END { exit (n!=1 || bad) }' /proc/mounts || fail 'root filesystem must be RAM (rootfs/tmpfs/ramfs)'
    while read -r source point fstype options rest; do
        case "$fstype" in
            rootfs|tmpfs|ramfs|proc|sysfs|devtmpfs|devpts|cgroup|cgroup2|debugfs|tracefs|securityfs|pstore|configfs|mqueue|bpf) continue ;;
        esac
        case ",$options," in *,ro,*) ;; *) fail "persistent/unsupported writable mount: $point ($fstype)" ;; esac
        case "$source" in /dev/mmcblk[0-9]*p[0-9]*) ;; *) fail "only read-only SD source mounts are allowed: $point" ;; esac
        source_base=${source##*/}; source_disk=${source_base%p*}
        [ -r "/sys/class/block/$source_disk/device/type" ] && [ "$(cat "/sys/class/block/$source_disk/device/type")" = SD ] || fail "read-only source is not an SD card: $source"
    done </proc/mounts
    awk 'NR>1 { bad=1 } END { exit bad }' /proc/swaps || fail 'active swap is not allowed'
}
validate_target() {
    [ "$(id -u)" = 0 ] || fail 'run as root in the RAM system'
    [ -r /sys/firmware/devicetree/base/compatible ] || fail 'board device tree unavailable'
    tr '\000' '\n' </sys/firmware/devicetree/base/compatible | awk -v board="$BOARD_COMPATIBLE" '$0==board {found=1} END {exit !found}' || fail 'wrong board compatible'
    case "$DEVICE" in /dev/mmcblk[0-9]*) ;; *) fail 'target must be /dev/mmcblkX' ;; esac
    BASE=${DEVICE##*/}; index=${BASE#mmcblk}
    case "$index" in ''|*[!0-9]*) fail 'target must be a whole mmcblk device, not a partition/boot area' ;; esac
    [ -b "$DEVICE" ] && [ ! -L "$DEVICE" ] || fail 'target is not a direct block device node'
    validate_target_state
}
# Split read-only state checks for offline unit coverage. main always calls the
# physical block-node/board checks above; no runtime path override is accepted.
validate_target_state() {
    TARGET_SYS=/sys/class/block/$BASE
    [ -d "$TARGET_SYS" ] && [ ! -e "$TARGET_SYS/partition" ] || fail 'target is not a whole block device'
    target_path=$(readlink -f "$TARGET_SYS/device") || fail 'cannot resolve eMMC controller'
    case "$target_path" in */4022000.mmc/*|*/mmc@4022000/*) ;; *) fail "wrong eMMC controller: $target_path" ;; esac
    [ "$(cat "$TARGET_SYS/device/type")" = MMC ] || fail 'target is not an eMMC'
    [ "$(cat "$TARGET_SYS/removable")" = 0 ] || fail 'target is removable'
    [ "$(cat "$TARGET_SYS/ro")" = 0 ] || fail 'target is read-only'
    actual_cid=$(tr 'A-F' 'a-f' <"$TARGET_SYS/device/cid")
    [ "$actual_cid" = "$EXPECT_CID" ] || fail 'eMMC CID does not match --expect-cid'
    sectors=$(cat "$TARGET_SYS/size")
    case "$sectors" in ''|*[!0-9]*) fail 'invalid capacity' ;; esac
    [ "$sectors" -ge 598016 ] || fail 'eMMC smaller than disk image'
    validate_mounts
    for node in /sys/class/block/"$BASE"*; do
        [ -r "$node/dev" ] || continue
        devid=$(cat "$node/dev")
        awk -v devid="$devid" '$3==devid {bad=1} END {exit bad}' /proc/self/mountinfo || fail "target or child is mounted: ${node##*/}"
        for holder in "$node"/holders/*; do
            [ ! -e "$holder" ] && [ ! -L "$holder" ] || fail "target or child has an active holder: ${node##*/}"
        done
    done
    info "target verified: device=$DEVICE cid=$actual_cid controller=0x04022000 sectors=$sectors"
}
read_boot_configuration() {
    script_path=$(readlink -f -- "$0") || fail 'cannot resolve installer location'
    helper=${script_path%/*}/t153-mmc-info
    [ -x "$helper" ] || helper=/usr/sbin/t153-mmc-info
    [ -x "$helper" ] || fail 'missing t153-mmc-info beside installer (or /usr/sbin); EXT_CSD must be read before installation'
    "$helper" --device "$DEVICE" >"$WORK/mmc-info" || fail 'cannot read eMMC EXT_CSD'
    # Parse data; never source/evaluate tool output.
    PARTITION_CONFIG=$(awk -F= '$1=="PARTITION_CONFIG" {n++; v=$2} END {if(n!=1 || v!~/^[0-9]+$/ || v>255) exit 1; print v}' "$WORK/mmc-info") || fail 'invalid PARTITION_CONFIG'
    BOOT_BUS_CONDITIONS=$(awk -F= '$1=="BOOT_BUS_CONDITIONS" {n++; v=$2} END {if(n!=1 || v!~/^[0-9]+$/ || v>255) exit 1; print v}' "$WORK/mmc-info") || fail 'invalid BOOT_BUS_CONDITIONS'
    boot_enable=$(((PARTITION_CONFIG >> 3) & 7))
    info "EXT_CSD: PARTITION_CONFIG=$PARTITION_CONFIG BOOT_BUS_CONDITIONS=$BOOT_BUS_CONDITIONS boot_enable=$boot_enable"
    [ "$((PARTITION_CONFIG & 7))" = 0 ] || fail 'eMMC is not exposing the user data partition'
    case "$boot_enable" in 0|7) ;; *) fail 'eMMC hardware boot partition is enabled; user-area image cannot replace that boot chain (configuration left unchanged)' ;; esac
}
verify_readback() {
    rm -f "$WORK/fifo"; mkfifo "$WORK/fifo" || fail 'cannot create readback stream'
    dd if="$DEVICE" bs=1048576 count=292 iflag=direct,fullblock >"$WORK/fifo" 2>"$WORK/readback.log" &
    PRODUCER=$!
    consumer_ok=0; sha256sum <"$WORK/fifo" >"$WORK/readback.sha256" && consumer_ok=1
    producer_ok=0; wait "$PRODUCER" && producer_ok=1; PRODUCER=
    [ "$producer_ok" = 1 ] && [ "$consumer_ok" = 1 ] || fail 'eMMC readback failed; no automatic retry'
    actual=$(awk '{print $1}' "$WORK/readback.sha256")
    [ "$actual" = "$EXPECT_SHA" ] || fail 'eMMC readback SHA-256 mismatch; do not assume the board is bootable'
    rm -f "$WORK/fifo"
}
main() {
    while [ "$#" -gt 0 ]; do
        case "$1" in
            --write) WRITE=1; shift ;;
            --device|--expect-cid|--image|--sha256)
                [ "$#" -ge 2 ] || fail "missing value for $1"
                case "$1" in --device) DEVICE=$2 ;; --expect-cid) EXPECT_CID=$2 ;; --image) IMAGE=$2 ;; --sha256) EXPECT_SHA=$2 ;; esac
                shift 2 ;;
            --help|-h) usage; exit 0 ;;
            *) usage >&2; fail "unknown argument: $1" ;;
        esac
    done
    [ -n "$DEVICE" ] && [ -n "$EXPECT_CID" ] && [ -n "$IMAGE" ] && [ -n "$EXPECT_SHA" ] || { usage >&2; fail 'all four identity/image arguments are required'; }
    EXPECT_CID=$(printf '%s' "$EXPECT_CID" | tr 'A-F' 'a-f'); EXPECT_SHA=$(printf '%s' "$EXPECT_SHA" | tr 'A-F' 'a-f')
    case "$EXPECT_CID" in *[!0-9a-f]*) fail 'CID must be hexadecimal' ;; esac
    case "$EXPECT_SHA" in *[!0-9a-f]*) fail 'SHA-256 must be hexadecimal' ;; esac
    [ "${#EXPECT_CID}" = 32 ] && [ "${#EXPECT_SHA}" = 64 ] || fail 'CID/SHA-256 length mismatch'
    for tool in awk hexdump dd sha256sum gzip wc mktemp mkfifo readlink tr cat sync id; do command -v "$tool" >/dev/null 2>&1 || fail "missing required tool: $tool"; done
    WORK=$(mktemp -d /tmp/t153-install.XXXXXXXX) || fail 'cannot create temporary directory'
    trap cleanup 0; trap 'exit 130' INT; trap 'exit 143' TERM HUP
    validate_target
    read_boot_configuration
    validate_image
    if [ "$WRITE" = 0 ]; then info 'PREFLIGHT_OK writes=0 cold_boot=NOT_TESTED'; exit 0; fi
    # Repeat live identity/mount/configuration guards immediately before writing.
    validate_target; read_boot_configuration
    # A second full hash immediately before destructive access catches changes
    # made in-place since preflight; the pinned descriptor prevents path swaps.
    stream_to hash
    [ "$(awk '{print $1}' "$WORK/hash")" = "$EXPECT_SHA" ] || fail 'image changed after preflight'
    info "WRITE_BEGIN device=$DEVICE bytes=$IMAGE_BYTES; do not disconnect power"
    stream_to write
    sync || fail 'sync failed'
    verify_readback
    sync || fail 'final sync failed'
    info "INSTALL_READBACK_OK device=$DEVICE bytes=$IMAGE_BYTES sha256=$EXPECT_SHA cold_boot=NOT_TESTED"
    info 'Keep the UART log. A separate power-off cold boot is required; this program does not reboot or change eMMC boot configuration.'
    info 'Only the first 292 MiB of the user area were replaced; this is not a secure erase of remaining capacity or hardware boot partitions.'
}
# Production entry point. Offline tests extract function definitions; there is no runtime test bypass.
main "$@"
