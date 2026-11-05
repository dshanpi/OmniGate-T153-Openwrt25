# SPDX-License-Identifier: GPL-2.0-only
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_VERSION:=2026.07
PKG_RELEASE:=1
PKG_HASH:=78e8bfc382fe388f9b55aa1daf8c563522a037779b5d4c349d1415e381f1243e

UBOOT_USE_INTREE_DTC:=1

include $(INCLUDE_DIR)/u-boot.mk
include $(INCLUDE_DIR)/package.mk

define U-Boot/Default
  BUILD_TARGET:=sunxi
  BUILD_SUBTARGET:=cortexa7
  HIDDEN:=1
endef

define U-Boot/dshanpi_t153mx-omnigate
  NAME:=DshanPI T153MX OmniGate (source DDR4, four Cortex-A7 cores)
  BUILD_DEVICES:=dshanpi_t153mx-omnigate
  UBOOT_CONFIG:=dshanpi_t153_source_1g_smp
  UBOOT_IMAGE:=u-boot-t153-with-spl.bin
endef

UBOOT_TARGETS:=dshanpi_t153mx-omnigate
UBOOT_CONFIGURE_VARS += USE_PRIVATE_LIBGCC=yes
UBOOT_CUSTOMIZE_CONFIG := \
	--disable TOOLS_KWBIMAGE \
	--disable TOOLS_LIBCRYPTO \
	--disable TOOLS_MKEFICAPSULE
UBOOT_MAKE_FLAGS += KBUILD_BUILD_USER=builder KBUILD_BUILD_HOST=OpenWrt

define Build/Prepare
	$(call Build/Prepare/Default)
	$(CP) ./files/. $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(call Build/Compile/U-Boot)
	$(PKG_BUILD_DIR)/tools/mkimage -A arm -T sunxi_egon \
		-n sun8i-t153-dshanpi-omnigate-uboot \
		-d $(PKG_BUILD_DIR)/spl/u-boot-spl.bin $(PKG_BUILD_DIR)/t153-spl.bin
	$(PYTHON) ./tools/pack-bootarea.py --build-dir $(PKG_BUILD_DIR) \
		--cross-prefix $(TARGET_CROSS)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR_IMAGE)
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/$(UBOOT_IMAGE) \
		$(STAGING_DIR_IMAGE)/$(BUILD_DEVICES)-u-boot-with-spl.bin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/t153-spl.bin \
		$(STAGING_DIR_IMAGE)/$(BUILD_DEVICES)-spl.bin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/u-boot.img \
		$(STAGING_DIR_IMAGE)/$(BUILD_DEVICES)-u-boot.img
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/t153-bootarea.json \
		$(STAGING_DIR_IMAGE)/$(BUILD_DEVICES)-uboot-build.json
endef

# The boot area is consumed by target/linux/sunxi/image, not installed in rootfs.
define Package/u-boot/install/default
endef

$(eval $(call BuildPackage/U-Boot))
