#!/usr/bin/env python3
"""Run the built ARM ft_board_setup against the real OpenWrt DTB in Unicorn."""
import ctypes, importlib.util, json, re
from pathlib import Path
from elftools.elf.elffile import ELFFile
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM,UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_SP,UC_ARM_REG_LR,UC_ARM_REG_PC
root=Path(__file__).resolve().parents[2]
build=root/'u-boot-work/build-t153-source'
e=ELFFile(open(build/'u-boot','rb'))
syms={line.split()[-1]:int(line.split()[0],16) for line in (build/'u-boot.sym').read_text().splitlines() if re.match(r'^[0-9a-f]{8} ',line)}
u=Uc(UC_ARCH_ARM,UC_MODE_ARM);u.mem_map(0x47000000,0x04000000)
for seg in e.iter_segments():
 if seg['p_type']=='PT_LOAD':u.mem_write(seg['p_vaddr'],seg.data())
fdt=ctypes.CDLL('libfdt.so.1')
src=(root/'artifacts/t153-omnigate.dtb').read_bytes();buf=ctypes.create_string_buffer(65536)
assert fdt.fdt_open_into(src,buf,len(buf))==0
u.mem_write(0x47000000,bytes(buf))
def hook(u,addr,size,data):
 if addr in (syms['puts'],syms['printf']):
  u.reg_write(UC_ARM_REG_R0,0);u.reg_write(UC_ARM_REG_PC,u.reg_read(UC_ARM_REG_LR))
u.hook_add(UC_HOOK_CODE,hook)
u.reg_write(UC_ARM_REG_R0,0x47000000);u.reg_write(UC_ARM_REG_R1,0)
u.reg_write(UC_ARM_REG_SP,0x47300000);u.reg_write(UC_ARM_REG_LR,0x470f0000)
u.emu_start(syms['ft_board_setup'],0x470f0000,count=3000000)
assert u.reg_read(UC_ARM_REG_PC)==0x470f0000
assert u.reg_read(UC_ARM_REG_R0)==0
spec=importlib.util.spec_from_file_location('verify',root/'scripts/verify-artifacts.py')
v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
nodes=v.fdt_nodes(bytes(u.mem_read(0x47000000,65536)))
assert '/psci' not in nodes
assert 'enable-method' not in nodes['/cpus/cpu@0']
for i in range(1,4):assert nodes[f'/cpus/cpu@{i}']['status']==b'disabled\0'
for path,node in v.fdt_nodes(src).items():
 if node.get('device_type')==b'memory\0':assert nodes[path]['reg']==node['reg']
report={'status':'pass','test':'actual ARM U-Boot DT fixup, CPU0-only, no factory PSCI','hardware_test':False}
(root/'logs/uboot-source-dt-test.json').write_text(json.dumps(report,indent=2)+'\n')
print(report)
