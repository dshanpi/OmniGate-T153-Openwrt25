# SPDX-License-Identifier: GPL-2.0-only

define KernelPackage/can-t153
  SUBMENU:=CAN Support
  TITLE:=Allwinner T153 CAN-FD controller
  DEPENDS:=@TARGET_sunxi +kmod-can
  KCONFIG:=CONFIG_CAN_M_CAN \
        CONFIG_CAN_M_CAN_SUN8I_T153 \
        CONFIG_CAN_M_CAN_PCI=n \
        CONFIG_CAN_M_CAN_PLATFORM=n \
        CONFIG_CAN_M_CAN_TCAN4X5X=n
  FILES:=$(LINUX_DIR)/drivers/net/can/m_can/m_can.ko \
        $(LINUX_DIR)/drivers/net/can/m_can/m_can_sun8i_t153.ko
  AUTOLOAD:=$(call AutoProbe,m_can_sun8i_t153)
endef

$(eval $(call KernelPackage,can-t153))
