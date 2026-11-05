# SPDX-License-Identifier: GPL-2.0-only
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=aic8800-sdio
PKG_VERSION:=2026.11.05
PKG_RELEASE:=1
PKG_SOURCE_VERSION:=d9c72efd382ccdbf2f1fdb09d2d4f3cc212edd9d
PKG_SOURCE:=aic8800-$(PKG_SOURCE_VERSION).tar.gz
PKG_SOURCE_URL:=https://codeload.github.com/radxa-pkg/aic8800/tar.gz
PKG_SOURCE_URL_FILE:=$(PKG_SOURCE_VERSION)
PKG_HASH:=46bf2d2202287479c46adeed07696505a16d84b413bedfd13cb6a73c2f7d07b9
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/aic8800-$(PKG_SOURCE_VERSION)
PKG_LICENSE:=GPL-2.0-only
PKG_LICENSE_FILES:=debian/copyright
PKG_BUILD_DEPENDS:=mac80211
PKG_BUILD_PARALLEL:=1
PKG_FLAGS:=nonshared
STAMP_CONFIGURED_DEPENDS := $(STAGING_DIR)/usr/include/mac80211-backport/backport/autoconf.h

include $(INCLUDE_DIR)/package.mk

AIC_DRIVER_DIR:=$(PKG_BUILD_DIR)/src/SDIO/driver_fw/driver/aic8800

define KernelPackage/aic8800-sdio
  SUBMENU:=Wireless Drivers
  TITLE:=AIC8800D80 SDIO Wi-Fi (T153)
  DEPENDS:=+kmod-cfg80211 +kmod-mmc +aic8800d80-firmware +@DRIVER_11N_SUPPORT +@DRIVER_11AC_SUPPORT +@DRIVER_11AX_SUPPORT
  FILES:= \
	$(AIC_DRIVER_DIR)/aic8800_bsp/aic8800_bsp.ko \
	$(AIC_DRIVER_DIR)/aic8800_fdrv/aic8800_fdrv.ko
  AUTOLOAD:=$(call AutoProbe,aic8800_bsp aic8800_fdrv)
endef

define KernelPackage/aic8800-sdio/description
  GPL host driver for the T153 board's AIC8800D80 SDIO radio. Runs on the
  OpenWrt mainline-derived kernel with OpenWrt cfg80211 backports.
  This external driver is not merged into upstream Linux. Radio firmware
  is separately packaged binary device code, not host DDR initialization.
endef

define Package/aic8800d80-firmware
  SECTION:=firmware
  CATEGORY:=Firmware
  TITLE:=AIC8800D80 binary Wi-Fi and Bluetooth device firmware
  LICENSE:=LicenseRef-AIC8800-Firmware
endef

define Package/aic8800d80-firmware/description
  Unmodified device firmware from the pinned Radxa source distribution.
  These opaque binaries execute on the wireless controller. They are not
  source-built firmware and are explicitly separate from the GPL host driver.
  See installed provenance and upstream copyright metadata.
endef

define KernelPackage/rfkill-gpio
  SUBMENU:=Other modules
  TITLE:=Mainline GPIO controlled rfkill switch
  DEPENDS:=+@USE_RFKILL +kmod-rfkill
  KCONFIG:=CONFIG_RFKILL_GPIO
  FILES:=$(LINUX_DIR)/net/rfkill/rfkill-gpio.ko
  AUTOLOAD:=$(call AutoProbe,rfkill-gpio)
endef

NOSTDINC_FLAGS:= \
	-DAIC_CFG80211_6_18 \
	$(KERNEL_NOSTDINC_FLAGS) \
	-I$(STAGING_DIR)/usr/include/mac80211-backport/uapi \
	-I$(STAGING_DIR)/usr/include/mac80211-backport \
	-I$(STAGING_DIR)/usr/include/mac80211/uapi \
	-I$(STAGING_DIR)/usr/include/mac80211 \
	-include backport/autoconf.h \
	-include backport/backport.h

define Build/Compile
	+$(KERNEL_MAKE) $(PKG_JOBS) \
		M="$(AIC_DRIVER_DIR)" \
		NOSTDINC_FLAGS="$(NOSTDINC_FLAGS)" \
		CONFIG_PLATFORM_UBUNTU=y CONFIG_PLATFORM_ALLWINNER=n \
		CONFIG_AIC8800_BTLPM_SUPPORT=n CONFIG_SDIO_BT=n \
		CONFIG_USE_FW_REQUEST=y CONFIG_AIC_FW_PATH="/lib/firmware" \
		CONFIG_SDIO_PWRCTRL=n CONFIG_OOB=n CONFIG_GPIO_WAKEUP=n \
		modules
endef

define Package/aic8800d80-firmware/install
	$(INSTALL_DIR) $(1)/lib/firmware/aic8800D80
	$(CP) $(PKG_BUILD_DIR)/src/SDIO/driver_fw/fw/aic8800D80/* $(1)/lib/firmware/aic8800D80/
	$(INSTALL_DIR) $(1)/usr/share/aic8800d80
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/debian/copyright $(1)/usr/share/aic8800d80/upstream-copyright
	$(INSTALL_DATA) ./files/FIRMWARE-NOTICE.txt ./files/firmware-sha256.json $(1)/usr/share/aic8800d80/
endef

$(eval $(call KernelPackage,aic8800-sdio))
$(eval $(call KernelPackage,rfkill-gpio))
$(eval $(call BuildPackage,aic8800d80-firmware))
