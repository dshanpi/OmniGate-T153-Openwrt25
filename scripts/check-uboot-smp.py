#!/usr/bin/env python3
"""Audit resident ARMv7 monitor ranges and call edges in the linked SMP ELF."""
from pathlib import Path
import argparse,hashlib,json,re,struct,subprocess
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build-dir',type=Path,default=ROOT/'u-boot-work/build-t153-source-1g-smp');a=p.parse_args();b=a.build_dir.resolve()
def need(value,msg):
    if not value:raise ValueError(msg)
e=(b/'u-boot').read_bytes();need(e[:7]==b'\x7fELF\x01\x01\x01','ELF32 little endian')
shoff=struct.unpack_from('<I',e,32)[0];ents,count,names=struct.unpack_from('<HHH',e,46)
headers=[struct.unpack_from('<10I',e,shoff+i*ents) for i in range(count)]
st=headers[names];namestr=e[st[4]:st[4]+st[5]]
sections={namestr[h[0]:].split(b'\0',1)[0].decode():h for h in headers}
ranges={}
for n in ('.secure_text','.secure_data','.secure_stack'):
 h=sections[n];start,size=h[3],h[5]
 need(size>0 and 0x50000<=start<start+size<=0x58000,n+' SRAM bounds')
 ranges[n]={'start':start,'end':start+size,'bytes':size}
need(ranges['.secure_text']['start']==0x50000,'secure entry base')
need(ranges['.secure_stack']['bytes']>=4096,'four monitor stacks')
symbols={line.split()[-1]:int(line.split()[0],16) for line in (b/'u-boot.sym').read_text().splitlines() if len(line.split())>=2 and re.fullmatch('[0-9a-fA-F]+',line.split()[0])}
for n in ('_nonsec_init','_do_nonsec_entry','_psci_vectors','psci_cpu_on','psci_cpu_entry','psci_save','psci_arch_cpu_entry','t153_psci_cpu_entry','psci_get_cpu_id','psci_stack_setup'):
 need(0x50000<=symbols[n]<ranges['.secure_text']['end'],'nonresident monitor function '+n)
need(0x4a000000<=symbols['psci_board_init']<0x4a200000,'boot-only initializer placement')
need(symbols['__secure_stack_end']<=0x58000 and symbols['__secure_stack_end']-symbols['__secure_stack_start']>=4096,'generic per-core stack layout')
spl=(b/'spl/u-boot-spl.sym').read_text()
need(not re.search(r'\b(psci_cpu_on|t153_psci_cpu_entry|t153_smp_fixup_dt)\b',spl),'monitor linked in SPL')
# PSCI dispatch table function pointers must target only resident code.
h=sections['.secure_text'];table=symbols['_psci_table']-h[3]+h[4];functions=[]
for i in range(32):
 ident,fn=struct.unpack_from('<II',e,table+i*8)
 if ident==0:
  need(fn==0,'table terminator');break
 need(0x50000<=fn<ranges['.secure_text']['end'],'PSCI table target outside SRAM')
 functions.append({'id':hex(ident),'target':hex(fn)})
else:raise ValueError('unterminated PSCI table')
objdump=list((ROOT/'openwrt/staging_dir').glob('toolchain-arm_cortex-a7*/bin/arm-openwrt-linux-muslgnueabi-objdump'))
need(len(objdump)==1,'ARM objdump required')
dis=subprocess.check_output([str(objdump[0]),'-d','-j','.secure_text',str(b/'u-boot')],text=True)
for line in dis.splitlines():
 m=re.search(r'\s(b(?:l|lx)?(?:eq|ne|cc|cs|mi|pl|vs|vc|hi|ls|ge|lt|gt|le)?)\s+([0-9a-f]+)\s+<',line)
 if m:need(0x50000<=int(m[2],16)<ranges['.secure_text']['end'],'runtime branch outside resident SRAM: '+line)
 need(not re.search(r'\.word\s+0x[4567][0-9a-f]{7}',line),'literal reference into disposable DRAM: '+line)
rel=subprocess.check_output(['readelf','-rW',str(b/'u-boot')],text=True)
for line in rel.splitlines():
 fields=line.split()
 if fields and re.fullmatch('[0-9a-fA-F]{8}',fields[0]):
  addr=int(fields[0],16);need(not 0x50000<=addr<0x58000,'runtime relocation inside fixed SRAM')
need(not subprocess.check_output(['nm','-u',str(b/'u-boot')],text=True).strip(),'unresolved U-Boot symbols')
report={'status':'linked software checks passed; hardware not tested','elf_sha256':hashlib.sha256(e).hexdigest(),'sram_reservation':{'base':'0x50000','bytes':32768},'sections':ranges,'dispatch_entries':functions,'direct_runtime_branches_resident':True,'no_runtime_secure_relocations':True,'spl_excludes_monitor':True,'hardware_isolation_configured':False,'limitations':['CPU_ON only','CPU_OFF/hotplug/suspend/multi-core kexec unsupported','SRAM residency is not a claim of TrustZone access isolation']}
(ROOT/'logs/t153-smp-linked-validation.json').write_text(json.dumps(report,indent=2)+'\n')
(ROOT/'logs/t153-smp-secure-disassembly.txt').write_text(dis)
print(json.dumps(report,indent=2))
