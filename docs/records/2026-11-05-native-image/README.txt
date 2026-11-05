Official v25.12.6 native disk: independent software validation
============================================================
Date: 2026-11-05. No board operation was performed by this validation task.

Native OpenWrt make completed separately under the build coordinator. The
first image exposed make_ext4fs inode-bitmap padding: staged e2fsck -fn
returned 4. The T153 image recipe was corrected to copy root.ext4 to its
private work directory, run staged e2fsck -pf (only status 0 or 1 accepted),
then require -fn status 0 before inserting that copy. Neither the build
root.ext4 nor other OpenWrt boards' image recipes were altered.
Both E2FSCK_TIME and E2FSPROGS_FAKE_TIME are fixed to SOURCE_DATE_EPOCH.

The final native image passes tests/native/verify-image.py:
- Whole disk: 306184192 bytes; MBR, 4 MiB boot start, 32 MiB FAT16,
  256 MiB ext4 at 36 MiB, and exact disk length checked.
- SPL/U-Boot match this native build's staged bytes; eGON/legacy CRC,
  source/four-core build metadata and 8 KiB/128 KiB offsets checked.
- FAT, boot.scr CRC/PARTUUID and epoch, FIT hashes/load/entry and compiled
  DTB checked. FIT uses the ordinary KDIR/zImage.
- Actual kernel banner is Linux 6.12.112. Its decompressed bytes exactly
  match objcopy output from KDIR/vmlinux.debug. ELF symbols locate the
  minimal built-in cpio; it has only dev, dev/console and root, no /init
  or populated root filesystem. Console major/minor is 5:1.
- Final rootfs passes staged e2fsck -fn with status 0. Installed release
  is OpenWrt 25.12.6 / r33311-981c978eff, independently compared against
  root-sunxi along with init/procd, network/upgrade/firewall files and
  the AIC Bluetooth helper/service/config.
- The original root.ext4 SHA remains unchanged. The final partition
  equals an independently normalized private copy. All 3087 changed
  bytes are audited: 3072 inode-bitmap padding bytes, 8 time bytes,
  3 unused-inode count bytes and 4 group-descriptor checksum bytes.
  Unused inode counts 6313 and 8192 are independently derived from the
  bitmaps. File data and inode tables remain byte-for-byte unchanged.

Compressed image: openwrt-sunxi-cortexa7-dshanpi_t153mx-omnigate-ext4-raw.img.gz
Bytes: 16560656
SHA256: aafef7198f0c5f68005471c30132b4a57334ae4d7e08c9ea5fcf9df5a68482ed
Raw disk bytes: 306184192
SHA256: cd57d6970329f4a9255e7020ea3f7dfbc2c13040f7154563cbe43ba0d431a32e
Original root.ext4 SHA256:
  15c082651fb49ac140a2dae3d9126d5934f6757b58af4f401f40221907bde369
Final root partition SHA256:
  1605ab1ce5ef6682993977721a86ae5a72d64bae191f8a92713a187f76400a6c

Files:
  validation.json: exact inputs, hashes, filesystem logs, kernel checks,
                   installed-file hashes and verifier source hash.
  repeat-validation.json: a later native image-recipe invocation using
                         the same existing build inputs was byte-identical.
  repeated-image-generation.log: that repeat's raw tool output.
No toolchain/kernel was rebuilt by the repeat. Temporary copies were removed.

The validated raw image was retained outside the source tree under the
build coordinator's artifacts directory for subsequent FES use. This record
sets hardware_verified=false. It proves no new flashing, runtime four-core
operation, network stability, or cold-boot result; attach separate board
records to the same raw SHA for those claims.
