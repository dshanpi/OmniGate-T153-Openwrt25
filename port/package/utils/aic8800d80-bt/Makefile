include $(TOPDIR)/rules.mk

PKG_NAME:=aic8800d80-bt
PKG_VERSION:=2026.11.05
PKG_RELEASE:=2
PKG_LICENSE:=GPL-2.0-or-later
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/aic8800d80-bt
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=AIC8800D80 UART Bluetooth initialization
  DEPENDS:=@TARGET_sunxi +kmod-aic8800-sdio +aic8800d80-firmware +kmod-hci-uart +kmod-rfkill-gpio
endef

define Package/aic8800d80-bt/description
Initialize the AIC8800D80 Bluetooth controller after its shared SDIO
firmware loader, then attach UART7 to the mainline H4 line discipline.
The initial T153 configuration keeps the device awake; low power is disabled.
endef

define Package/aic8800d80-bt/conffiles
/etc/config/aic8800d80-bt
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/aic8800d80-bt.c $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) -std=c11 -Wall -Wextra -Werror \
		-o $(PKG_BUILD_DIR)/aic8800d80-bt $(PKG_BUILD_DIR)/aic8800d80-bt.c $(TARGET_LDFLAGS)
endef

define Package/aic8800d80-bt/install
	$(INSTALL_DIR) $(1)/usr/sbin $(1)/etc/init.d $(1)/etc/config
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/aic8800d80-bt $(1)/usr/sbin/
	$(INSTALL_BIN) ./files/aic8800d80-bt.init $(1)/etc/init.d/aic8800d80-bt
	$(INSTALL_CONF) ./files/aic8800d80-bt.config $(1)/etc/config/aic8800d80-bt
endef

$(eval $(call BuildPackage,aic8800d80-bt))
