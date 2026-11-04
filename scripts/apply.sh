#!/bin/sh
set -eu
port_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
openwrt_dir=${1:?Usage: apply.sh /path/to/openwrt-25.12}
openwrt_dir=$(CDPATH= cd -- "$openwrt_dir" && pwd)
test -f "$openwrt_dir/target/linux/generic/kernel-6.12"
grep -q '^LINUX_VERSION-6.12 = .113$' "$openwrt_dir/target/linux/generic/kernel-6.12" || {
    echo "This port is validated against Linux 6.12.113; rebase before changing kernel versions." >&2
    exit 1
}
cp -a "$port_dir/port/." "$openwrt_dir/"
python3 - "$openwrt_dir" <<'PY'
from pathlib import Path
import sys
root=Path(sys.argv[1])
if (root/'.upstream-commit').is_file():
    (root/'version').write_text((root/'.upstream-commit').read_text())
def section(path, body):
    p=root/path
    text=p.read_text()
    begin='# BEGIN T153 PORT\n';end='# END T153 PORT\n'
    if begin in text:
        before, rest=text.split(begin,1)
        _, after=rest.split(end,1)
        text=before+after
    p.write_text(text.rstrip()+'\n\n'+begin+body.strip()+'\n'+end)
# Migrate the early standalone packaging prototype into the kernel package.
legacy=root/'package/kernel/t153/Makefile'
if legacy.exists():
    assert 'Allwinner T153 CAN-FD controller' in legacy.read_text()
    legacy.unlink()
section('target/linux/sunxi/modules.mk', 'include $(TOPDIR)/target/linux/sunxi/t153-modules.mk')
# Package scanning caches indirect platform module definitions.
(root/'tmp/info/.packageinfo-kernel_linux').unlink(missing_ok=True)
section('target/linux/sunxi/image/cortexa7.mk', '''
define Device/dshanpi_t153mx-omnigate
  $(call Device/FitImageGzip)
  DEVICE_VENDOR := DshanPI
  DEVICE_MODEL := T153MX OmniGate (experimental)
  SOC := sun8i-t153
  SUNXI_DTS := allwinner/sun8i-t153-dshanpi-omnigate
  SUPPORTED_DEVICES := dshanpi,t153mx-omnigate
  KERNEL_LOADADDR := 0x48008000
  KERNEL_INITRAMFS := kernel-bin | gzip | fit gzip $$(DTS_DIR)/$$(SUNXI_DTS).dtb
  IMAGES := rootfs.ext4.gz
  IMAGE/rootfs.ext4.gz := append-rootfs | gzip
endef
TARGET_DEVICES += dshanpi_t153mx-omnigate
''')
section('target/linux/sunxi/cortexa7/config-6.12', '''
CONFIG_SUN8I_T153_CCU=y
CONFIG_PINCTRL_SUN8I_T153=y
CONFIG_DWMAC_SUN8I_T153=y
CONFIG_REALTEK_PHY=y
CONFIG_SERIAL_8250_DW=y
CONFIG_ARM_PSCI=y
CONFIG_ARM_PSCI_FW=y
CONFIG_PHY_SUN8I_T153_USB=y
CONFIG_SUNXI_WATCHDOG=y
CONFIG_POWER_RESET=y
CONFIG_REBOOT_MODE=y
CONFIG_SUN8I_T153_REBOOT_MODE=y
CONFIG_MMC_PWRSEQ_SIMPLE=y
CONFIG_SERIAL_8250_NR_UARTS=10
CONFIG_SERIAL_8250_RUNTIME_UARTS=10
''')
# Make these edits in place; retain all upstream cases for other boards.
p=root/'target/linux/sunxi/base-files/etc/board.d/02_network'
text=p.read_text()
anchor='\txunlong,orangepi-r1)'
if '\tdshanpi,t153mx-omnigate|' not in text:
    assert anchor in text
    text=text.replace(anchor, '\tdshanpi,t153mx-omnigate|\\\n'+anchor, 1)
    p.write_text(text)
p=root/'target/linux/sunxi/base-files/etc/board.d/02_network'
text=p.read_text()
block='''	dshanpi,t153mx-omnigate)
		# T153 MAC from eMMC CID; block probe order is not a stable identity.
		for mmc_path in /sys/block/mmcblk[0-9]*; do
			[ "$(cat "$mmc_path/device/type" 2>/dev/null)" = MMC ] || continue
			[ -r "$mmc_path/device/cid" ] || continue
			lan_mac=$(macaddr_generate_from_mmc_cid "${mmc_path##*/}")
			wan_mac=$(macaddr_add "$lan_mac" 1)
			break
		done
		;;
'''
if '# T153 MAC from eMMC CID' in text:
    import re
    text,count=re.subn(r'\tdshanpi,t153mx-omnigate\)\n\t\t# T153 MAC from eMMC CID.*?\n\t\t;;\n',lambda m:block,text,count=1,flags=re.S)
    assert count == 1
else:
    anchor='\tcase "$board" in\n\tfriendlyarm,nanopi-r1)'
    assert anchor in text
    text=text.replace(anchor,'\tcase "$board" in\n'+block+'\tfriendlyarm,nanopi-r1)',1)
p.write_text(text)
p=root/'target/linux/sunxi/base-files/etc/board.d/02_network'
text=p.read_text()
anchor='sunxi_setup_interfaces $board\n'
line='[ "$board" = "dshanpi,t153mx-omnigate" ] && ucidef_set_interface lan ipaddr "192.168.153.1" netmask "255.255.255.0"\n'
if line not in text:
    assert anchor in text
    text=text.replace(anchor,anchor+line,1)
    p.write_text(text)
p=root/'target/linux/sunxi/base-files/lib/upgrade/platform.sh'
text=p.read_text()
# Update the old help text before the idempotent guard comparison.
text=text.replace('T153 experimental port: persistent upgrade is not supported. Use the RAM boot image.', 'T153 experimental port: sysupgrade is not supported. Use the whole-disk FES workflow; see docs/FLASH-SOURCE.txt.')
for name in ['platform_check_image', 'platform_copy_config', 'platform_do_upgrade']:
    anchor=name+'() {\n'
    guard='''	# T153 PORT: no qualified installer exists for the factory A/B layout.
	case "$(board_name)" in
	dshanpi,t153mx-omnigate)
		echo "T153 experimental port: sysupgrade is not supported. Use the whole-disk FES workflow; see docs/FLASH-SOURCE.txt." >&2
		return 1
		;;
	esac
'''
    assert anchor in text
    if anchor+guard not in text:
        text=text.replace(anchor, anchor+guard, 1)
p.write_text(text)
PY
printf '%s\n' "T153 port applied to $openwrt_dir (no device was flashed)"
