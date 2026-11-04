#!/usr/bin/env python3
"""Check and package source-only T153 SPL/U-Boot; never access a board."""
import hashlib, importlib.util, json, re, shutil, struct, subprocess, zlib
from pathlib import Path
root=Path(__file__).resolve().parents[1]
b=root/'u-boot-work/build-t153-source'; art=root/'artifacts'
spec=importlib.util.spec_from_file_location('verify',root/'scripts/verify-artifacts.py')
v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
req=v.require
cfg=(b/'.config').read_text()
for name,value in {
 'TARGET_DSHANPI_T153_SOURCE':'y','SPL':'y','SPL_TEXT_BASE':'0x40060',
 'SPL_STACK':'0x66000','SPL_SYS_MALLOC_F_LEN':'0x4000',
 'SPL_EARLY_BSS':'y','SPL_SYS_ICACHE_OFF':'y','SPL_SYS_DCACHE_OFF':'y',
 'SPL_SKIP_LOWLEVEL_INIT_ONLY':'y','SPL_SYS_MMCSD_RAW_MODE':'y',
 'SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR':'0x100','SPL_LEGACY_IMAGE_CRC_CHECK':'y',
 'SPL_DM_MMC':'y','SPL_DM_SERIAL':'y','OF_BOARD_SETUP':'y',
 'TEXT_BASE':'0x4a000000','ENV_IS_NOWHERE':'y','BOOTDELAY':'3','BOARD_LATE_INIT':'y','CMD_SOURCE':'y',
}.items():req(f'CONFIG_{name}={value}\n' in cfg,'source config '+name)
for name in ('SPL_SHARES_INIT_SP_ADDR','SPL_MMC_WRITE','MMC_WRITE','ARMV7_PSCI',
             'ARMV7_NONSEC','CMD_SAVEENV','FAT_WRITE','EXT4_WRITE','ARCH_SUNXI'):
 req(f'CONFIG_{name}=y\n' not in cfg,'unsupported config '+name)

def elf_check(path,entry,limit):
 e=path.read_bytes();req(e[:7]==b'\x7fELF\x01\x01\x01','ELF32 ARM')
 req(struct.unpack_from('<HH',e,16)==(2,40),'ELF type/machine')
 ep,poff=struct.unpack_from('<II',e,24);req(ep==entry,'ELF entry '+path.name)
 psz,pnum=struct.unpack_from('<HH',e,42)
 for i in range(pnum):
  kind,offset,va,pa,fs,ms=struct.unpack_from('<6I',e,poff+i*psz)
  if kind==1:req(entry<=pa<=pa+ms<=limit,'ELF load bounds')
elf_check(b/'spl/u-boot-spl',0x40060,0x58060)
elf_check(b/'u-boot',0x4a000000,0x4a200000)
sym=(b/'spl/u-boot-spl.sym').read_text(); fullsym=(b/'u-boot.sym').read_text()
for name in ('t153_ddr_init','t153_return_to_fel','t153_fel_context','save_boot_params',
             'spl_boot_device','spl_mmc_load_image','t153_mmc_send_cmd'):
 req(re.search(r'\b'+name+r'\b',sym),'SPL linked source '+name)
req('ft_board_setup' in fullsym,'source kernel DT fixup')
req('board_late_init' in fullsym,'source boot header handoff')
req('CONFIG_BOOTCOMMAND="run t153boot"' in cfg,'source automatic boot command')
for name in ('init_DRAM','mctl_phy_init','libdram'):
 req(name not in sym and name not in (b/'spl/u-boot-spl.map').read_text(),'no SDK '+name)
req('board/dshanpi/t153/ddr.o' in (b/'spl/u-boot-spl.map').read_text(),'source DDR object linked')

def address(name):
 return int(next(line.split()[0] for line in sym.splitlines() if line.split() and line.split()[-1]==name),16)
raw=(b/'spl/u-boot-spl.bin').read_bytes()
nd=(b/'spl/u-boot-spl-nodtb.bin').read_bytes()
pad=(b/'spl/u-boot-spl-pad.bin').read_bytes();dt=(b/'spl/u-boot-spl.dtb').read_bytes()
req(raw==nd+pad+dt,'SPL DT after BSS padding')
req(address('__bss_start')==0x40060+len(nd),'BSS start')
req(address('__bss_end')==0x40060+len(nd)+len(pad),'BSS end before DT')
req(0x40060+len(raw)<0x5e000,'SPL below reserved stack/heap')
req(address('t153_fel_context')<address('__bss_start'),'FEL stash not in BSS')
nodes=v.fdt_nodes(dt)
for path in ('/serial@2600000','/mmc@4020000','/mmc@4022000'):
 req(path in nodes,'SPL DT contains '+path)
spl=(b/'t153-source-spl.bin').read_bytes()
req(spl[4:12]==b'eGON.BT0','eGON magic')
branch,=struct.unpack_from('<I',spl)
req(branch==0xea000016,'eGON branch to SRAM+96')
saved,length=struct.unpack_from('<II',spl,12)
req(length==len(spl) and length%8192==0 and length<=0x18000,'eGON size and padding')
check=bytearray(spl);struct.pack_into('<I',check,12,0x5f0a6c39)
req(sum(struct.unpack('<%dI'%(length//4),check))&0xffffffff==saved,'eGON checksum')
req(spl[96:96+len(raw)]==raw,'eGON payload is this build')
req(spl[20:23]==b'SPL','sunxi FEL header')
nameoff=struct.unpack_from('<I',spl,32)[0]
req(spl[nameoff:].split(b'\0',1)[0]==b'sun8i-t153-dshanpi-omnigate-uboot','eGON board name')
proper=(b/'u-boot.bin').read_bytes();img=(b/'u-boot.img').read_bytes()
req(img[64:]==proper,'legacy payload includes appended control DTB')
magic,hcrc,stamp,size,load,entry,dcrc=struct.unpack_from('>7I',img)
req(magic==0x27051956 and size==len(proper),'legacy image header')
req(load==entry==0x4a000000,'legacy entry/load')
req(img[28:32]==bytes([17,2,5,0]),'U-Boot/ARM/firmware/uncompressed image type')
h=bytearray(img[:64]);h[4:8]=b'\0'*4
req(zlib.crc32(h)==hcrc and zlib.crc32(proper)==dcrc,'legacy header/data CRC')
req(b'CPU0 only' in proper,'single-core limitation visible in binary')
# Standard sunxi-fel looks immediately after max(32 KiB, SPL length).
fel=spl+img
# Card boot uses absolute LBA 256, with SPL at absolute 8 KiB.
sd=spl+bytes(0x20000-0x2000-len(spl))+img
req(sd[0x1e000:]==img,'SD U-Boot at absolute 128 KiB')
art.mkdir(exist_ok=True)
outputs={
 't153-uboot-2026.07-source-spl.bin':spl,
 't153-uboot-2026.07-source.bin':proper,
 't153-uboot-2026.07-source.img':img,
 't153-uboot-2026.07-source-fel.bin':fel,
 't153-uboot-2026.07-source-sd-bootarea.bin':sd,
}
for name,data in outputs.items():(art/name).write_bytes(data)
shutil.copyfile(b/'.config',art/'uboot-source.config')
shutil.copyfile(root/'docs/BOOT-UBOOT-SOURCE.txt',art/'BOOT-UBOOT-SOURCE.txt')
shutil.copyfile(root/'docs/DDR-SOURCE.txt',art/'DDR-SOURCE.txt')
report={
 'status':'software checks passed; hardware NOT tested',
 'upstream':'U-Boot 2026.07 with out-of-tree T153 source port',
 'ddr_source':'port/uboot/files/board/dshanpi/t153/ddr.c',
 'vendor_ddr_object_linked':False,'factory_monitor_required':False,
 'hardware_verified':False,'linux_cpus_enabled':1,'psci_smp_implemented':False,
 'ddr_mhz':[480,933],'ddr_size_mib':512,'ddr_bus_bits':16,'ddr_ranks':1,
 'spl_entry':'0x40060','spl_bss_end':hex(address('__bss_end')),
 'spl_initial_stack':'0x66000','spl_heap_bytes':16384,
 'fel_thunk':'0x66a00','fel_swap':'0x66c00..0x67fff',
 'fel_returned_header':'0x66c00','fel_script_address':'0x4f100000',
 'uboot_entry':'0x4a000000','sd_spl_offset':8192,'sd_uboot_offset':131072,
 'media_write_enabled':False,
 'default_boot':'FEL script or boot.scr from boot MMC first; 3 second console delay',
 'fel_tool_commit':'660d7e08edd025c8f84ab8eba3d8a903a5b81ae0',
 'fel_tool_archive_sha256':'8279709da499dd8100f6f071075ab9cfda0f1b068240fcbee1fe99116b745637',
 'host_model_tests':'scripts/test-ddr-source.sh (ASan/UBSan)',
 'arm_dt_fixup_test':'tests/ddr/test-uboot-dt.py',
 'limitations':['hardware bring-up pending','CPU0 only; PSCI/SMP not implemented',
  'SD/eMMC only; no SPI flash loader','Linux installer and card cold boot require hardware validation'],
 'outputs':{n:{'bytes':len(d),'sha256':hashlib.sha256(d).hexdigest()} for n,d in outputs.items()},
}
(root/'logs/uboot-source-validation.json').write_text(json.dumps(report,indent=2)+'\n')
shutil.copyfile(root/'logs/uboot-source-validation.json',art/'uboot-source-validation.json')
print(json.dumps(report,indent=2))
