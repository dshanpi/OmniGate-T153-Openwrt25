Native OpenWrt T153 source SPL and U-Boot package
================================================

The package uses include/u-boot.mk and downloads the unmodified U-Boot
2026.07 source archive, verified by OpenWrt's normal PKG_HASH mechanism:
  https://ftp.denx.de/pub/u-boot/u-boot-2026.07.tar.bz2
  SHA256 78e8bfc382fe388f9b55aa1daf8c563522a037779b5d4c349d1415e381f1243e

The package overlay contains all T153 DDR4 initialization, SPL, read-only
MMC loading, reset/FEL handling, and the resident four-core PSCI CPU_ON
implementation. The patch only integrates this isolated ARM target into the
upstream Kconfig and DT build. No original Tina SDK directory, vendor DDR
object, secure-monitor binary, prebuilt U-Boot, external project build script,
or FES loader is consumed during this package build.

Selection
---------
OpenWrt target: sunxi / cortexa7
Device: dshanpi_t153mx-omnigate
Boot package: u-boot-dshanpi_t153mx-omnigate
U-Boot defconfig: dshanpi_t153_source_1g_smp

The normal U-Boot package default selects this variant for the T153 device.
As with other sunxi boot packages, PKG_TARGETS=bin provides staged image
artifacts, not an APK to install into rootfs. Do not add this boot package
to DEVICE_PACKAGES; use the standard BUILD_DEVICES default-selection rule.
For package-only development inside the patched OpenWrt source tree:
  make package/boot/uboot-t153/download V=s
  make package/boot/uboot-t153/compile V=s
Normal top-level make builds it before image generation. Host prerequisites
are those used by OpenWrt's native U-Boot packages, including in-tree DTC.

Image integration contract
--------------------------
Build/InstallDev stages the following under STAGING_DIR_IMAGE:
  dshanpi_t153mx-omnigate-u-boot-with-spl.bin
  dshanpi_t153mx-omnigate-spl.bin
  dshanpi_t153mx-omnigate-u-boot.img
  dshanpi_t153mx-omnigate-uboot-build.json

The combined boot-area file starts with the eGON SPL at file offset zero.
The image recipe inserts the combined file at absolute byte offset 8192
(LBA 16). Inside that file, legacy u-boot.img starts at byte offset 122880
(0x1e000), giving absolute disk offset 131072 (LBA 256), exactly matching the
SPL raw MMC configuration. It must fit before the FAT partition at 4 MiB.
This boot-area file is not a complete disk image and has no partition table.

The native image recipe supplies FAT boot.scr and the kernel FIT. The current
board environment loads boot.scr from partition 1 of the selected boot MMC
first, exposing the device number as bootdev. Generate root=PARTUUID=... for
the filesystem rather than assuming a Linux mmcblk index. FES remains a
separate host flashing workflow; it is not part of the installed boot chain.

Build checks
------------
The package-local pack-bootarea.py validates generated eGON and legacy-image
checksums/load addresses, source DDR linkage, SPL BSS/FDT placement, the FEL
context, selected configuration and monitor SRAM residency/call edges. It
then combines only this build's outputs and writes a hash/layout manifest.
It uses Python standard library and the selected OpenWrt cross-binutils;
there is no dependency on the surrounding adaptation repository.

This profile programs the 1 GiB physical DDR4 device while retaining the already
validated 384 MiB Linux/U-Boot memory window. It enables four Cortex-A7 CPUs via
PSCI 0.1 CPU_ON. CPU_OFF/hotplug, suspend and multi-core kexec remain outside the
implemented monitor interface. U-Boot MMC/filesystem writes and saved
environment are disabled, as in the validated standalone source profile.
