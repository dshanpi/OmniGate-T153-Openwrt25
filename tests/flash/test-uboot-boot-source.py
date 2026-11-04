#!/usr/bin/env python3
"""Execute real ARM board_late_init with synthetic BootROM/FEL headers."""
import json, re, struct
from pathlib import Path
from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC
root=Path(__file__).resolve().parents[2]
b=root/'u-boot-work/build-t153-source'
syms={line.split()[-1]:int(line.split()[0],16) for line in (b/'u-boot.sym').read_text().splitlines() if re.match(r'^[0-9a-f]{8} ',line)}
def run(magic=b'eGON.FEL',signature=b'SPL\x02',script=0x4f100000,uenv=0,media=0, stale_fel=False):
    u=Uc(UC_ARCH_ARM,UC_MODE_ARM)
    u.mem_map(0x40000,0x28000);u.mem_map(0x47000000,0x04000000)
    with (b/'u-boot').open('rb') as f:
        for seg in ELFFile(f).iter_segments():
            if seg['p_type']=='PT_LOAD':u.mem_write(seg['p_vaddr'],seg.data())
    h=bytearray(96);h[4:12]=magic;h[20:24]=signature
    struct.pack_into('<II',h,24,script,uenv);h[40]=media
    # Reproduce the thunk's final swap: BootROM bytes restored at 0x40000,
    # the executed SPL header moved to 0x66c00. Cold boot never swaps.
    u.mem_write(0x40000,b'BootROM workspace' + bytes(96-17))
    u.mem_write(0x40000 if magic==b'eGON.BT0' else 0x66c00,bytes(h))
    if stale_fel:
        stale=bytearray(h);stale[4:12]=b'eGON.FEL'
        u.mem_write(0x66c00,bytes(stale))
    env={'fel_scriptaddr':'stale'}
    def string(a):
        data=bytearray()
        while a:
            c=bytes(u.mem_read(a,1))
            if c==b'\0':break
            data.extend(c);a+=1
        return data.decode()
    def hook(u,a,size,data):
        if a in (syms['env_set'],syms['env_set_hex']):
            key=string(u.reg_read(UC_ARM_REG_R0));value=u.reg_read(UC_ARM_REG_R1)
            env[key]=(f'{value:x}' if a==syms['env_set_hex'] else string(value)) if value else None
            u.reg_write(UC_ARM_REG_R0,0);u.reg_write(UC_ARM_REG_PC,u.reg_read(UC_ARM_REG_LR))
    u.hook_add(UC_HOOK_CODE,hook)
    u.reg_write(UC_ARM_REG_SP,0x47300000);u.reg_write(UC_ARM_REG_LR,0x470f0000)
    u.emu_start(syms['board_late_init'],0x470f0000,count=100000)
    assert u.reg_read(UC_ARM_REG_PC)==0x470f0000 and u.reg_read(UC_ARM_REG_R0)==0
    return env
assert run()['fel_scriptaddr']=='4f100000'
for changes in [dict(script=0),dict(script=0x40000),dict(script=0x58000000),dict(uenv=1)]:
    env=run(**changes);assert env['fel_scriptaddr'] is None and env['t153_boot_source']=='fel'
for changes in [dict(signature=b'BAD\x02'),dict(signature=b'SPL\x00'),dict(signature=b'SPL\xff'),dict(magic=b'noheader')]:
    env=run(**changes);assert env['fel_scriptaddr'] is None and env['t153_boot_source']=='unknown'
for media,order in [(0,'0 1'),(2,'1 0')]:
    for stale in [False,True]:
        env=run(magic=b'eGON.BT0',media=media,stale_fel=stale)
        assert env['fel_scriptaddr'] is None and env['t153_boot_source']=='mmc' and env['bootdevs']==order
assert run(magic=b'eGON.BT0',media=0x12)['t153_boot_source']=='unknown'
report={'status':'pass','test':'real ARM board_late_init with T153 FEL thunk swap semantics','cases':14,'hardware_test':False}
(root/'logs/uboot-boot-source-test.json').write_text(json.dumps(report,indent=2)+'\n')
print(report)
