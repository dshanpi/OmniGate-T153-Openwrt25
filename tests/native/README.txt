Native OpenWrt v25.12.6 / T153 image validation
============================================

Wait for the normal OpenWrt build to finish successfully. This verifier reads
its inputs and uses private temporary files; it does not compile, mount a
filesystem, access a block device, modify the source tree, or contact a board.
It uses Python standard library, staged OpenWrt host tools and cross-binutils.

Example from this adaptation repository:
  python3 tests/native/verify-image.py \
    --source /path/to/patched-openwrt-25.12.6 \
    --output /path/to/artifacts/native-image-validation.json \
    --raw-output /path/to/artifacts/openwrt-t153-native-raw.img

--image may select an explicit native *ext4-raw.img.gz. Without it, exactly one
T153 image must exist under bin/targets/sunxi/cortexa7. --rootfs-source supports
an explicitly selected build root.ext4. Outputs must be outside --source.
The retained raw file is written only after all checks pass. An existing raw
output is accepted only when its hash matches; it is never silently replaced.
A failed check returns nonzero and writes a failed JSON report.

Checks bind the disk to the current staging bootarea, ordinary FIT/zImage,
compiled DTB, ordinary vmlinux.debug and root.ext4. Linux must be exactly
6.12.112, and installed OpenWrt must be 25.12.6/r33311-981c978eff. The ordinary
ELF must retain _text/__initramfs_start/__initramfs_size so the actual decoded
kernel's minimal cpio can be inspected; missing symbols are not proof that no
initramfs exists. The rescue kernel and linux/usr build intermediates are not
used to establish the ordinary kernel's contents.

The source make_ext4fs output may have unset inode-bitmap padding. The native
image recipe normalizes only a private copy with staged e2fsck, fixing both
E2FSCK_TIME and E2FSPROGS_FAKE_TIME to the build epoch. Validation records the
original fsck result and SHA, independently normalizes another copy, requires
the final disk partition to match it and pass read-only fsck, and audits every
changed byte. Only bitmap tail bits, fsck times/state and descriptor checksum/
unused-inode bookkeeping can change. The latter counts are independently
recomputed from allocation bitmaps; file data and inode tables cannot change.
Critical installed files, including the Bluetooth helper and OS release files,
are also compared directly with root-sunxi. A hash difference is disclosed,
not described as an unchanged raw rootfs input.

The output always declares hardware_verified=false. Passing is evidence for
software structure/content only; FES write/readback, source boot, four-core
runtime operation and stability need separate hardware records.
