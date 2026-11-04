#!/usr/bin/env python3
"""Validate wiring/provider consistency and exercise the no-flash upgrade guard."""
from pathlib import Path
import hashlib,re,subprocess,sys
root=Path(__file__).resolve().parents[1]
owrt=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else root/'openwrt'
subprocess.run(['sh',str(root/'scripts/apply.sh'),str(owrt)],check=True)
src=(owrt/'target/linux/sunxi/base-files/lib/upgrade/platform.sh').read_text()
for fun in ['platform_check_image','platform_copy_config','platform_do_upgrade']:
    code='''board_name() { echo dshanpi,t153mx-omnigate; }
dd() { echo UNEXPECTED_WRITE; return 99; }
mount() { echo UNEXPECTED_WRITE; return 99; }
export_bootdevice() { echo UNEXPECTED_DISK_ACCESS; return 99; }
'''+src+'\n'+fun+' fake-image\n'
    r=subprocess.run(['sh'],input=code,text=True,capture_output=True)
    assert r.returncode==1 and 'UNEXPECTED' not in r.stdout,(fun,r)
    print('PASS upgrade guard:',fun)
files=root/'port/target/linux/sunxi/files'
table=(files/'drivers/pinctrl/sunxi/pinctrl-sun8i-t153.c').read_text()
pinfun={}
for bank,num,body in re.findall(r'SUNXI_PIN\(SUNXI_PINCTRL_PIN\(([A-Z]),\s*(\d+)\)(.*?)(?=SUNXI_PIN\(|\n};)',table,re.S):
    pinfun[f'P{bank}{num}']=set(re.findall(r'SUNXI_FUNCTION\([^,]+,\s*"([^"]+)"',body))
dtsi=(files/'arch/arm/boot/dts/allwinner/sun8i-t153.dtsi').read_text()
for pins,fun in re.findall(r'pins = (.*?);\s*function = "([^"]+)";',dtsi,re.S):
    for pin in re.findall(r'"(P[A-Z]\d+)"',pins):
        assert fun in pinfun.get(pin,set()),(pin,fun,pinfun.get(pin))
print('PASS DT pinmux groups')
ccu=(files/'drivers/clk/sunxi-ng/ccu-sun8i-t153.c').read_text()
for clock in set(re.findall(r'<&ccu (CLK_\w+)',dtsi)):
    assert '['+clock+']' in ccu,clock
print('PASS DT clock providers')
paths=['target/linux/sunxi/modules.mk','version','target/linux/sunxi/base-files/etc/board.d/02_network','target/linux/sunxi/base-files/lib/upgrade/platform.sh','target/linux/sunxi/image/cortexa7.mk','target/linux/sunxi/cortexa7/config-6.12']
a={p:hashlib.sha256((owrt/p).read_bytes()).hexdigest() for p in paths}
subprocess.run(['sh',str(root/'scripts/apply.sh'),str(owrt)],check=True)
b={p:hashlib.sha256((owrt/p).read_bytes()).hexdigest() for p in paths}
assert a==b
print('PASS idempotent application')
network=(owrt/'target/linux/sunxi/base-files/etc/board.d/02_network').read_text()
func=network.split('sunxi_setup_interfaces()',1)[1].split('\nnanopi_r1_get_mac',1)[0]
code='ucidef_set_interfaces_lan_wan() { echo "$1/$2"; }\nucidef_set_interface_lan() { echo "$1"; }\n'
code+='sunxi_setup_interfaces()'+func+'\nsunxi_setup_interfaces dshanpi,t153mx-omnigate\n'
r=subprocess.run(['sh'],input=code,text=True,capture_output=True,check=True)
assert r.stdout.strip()=='eth0/eth1',r
print('PASS eth0 LAN / eth1 WAN defaults')
