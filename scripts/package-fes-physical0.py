#!/usr/bin/env python3
"""Deprecated physical0 packaging implementation retained only for incident traceability."""
raise SystemExit("DISABLED: physical0 is incompatible with OpenixCLI 0.2.11 raw / LYNX MCP; its host already compensates the stock +40960-sector mapping. Use artifacts/t153-fes-ddr933-ram-loader.img. See docs/FES-RAW-MAPPING-CORRECTION.txt.")
from pathlib import Path
import hashlib,struct,json,shutil,subprocess,importlib.util
root=Path(__file__).resolve().parents[1];orig=root/'fes-work/t153-ddr4-emmc-omnigate-1g';work=root/'fes-work/t153-ddr4-emmc-omnigate-1g-physical0';inputs=work/'input';inputs.mkdir(parents=True,exist_ok=True)
manifest=json.loads((orig/'loader.json').read_text())
for e in manifest['entries']:shutil.copyfile(orig/e['path'],work/e['path'])
old=(orig/'input/u-boot.fex').read_bytes();assert hashlib.sha256(old).hexdigest()=='80bc0bc21d429677ca29d1eb3bc8437980dffc0a57c880a48b70a6b8725151c2'
oldDT=old[0xd32e8:0xd32e8+27192];assert oldDT[:4]==bytes.fromhex('d00dfeed') and struct.unpack_from('>I',oldDT,4)[0]==len(oldDT)
src=root/'fes-work/vendor-physical/u-boot-2023';new=(src/'u-boot-nodtb.bin').read_bytes();assert len(new)==1302832
assert 'CONFIG_SUNXI_SDMMC_LOGICAL_OFFSET=0\n' in (src/'.config').read_text()
newCombined=new+oldDT;(inputs/'u-boot.fex').write_bytes(newCombined)
(work/'stock-control.dtb').write_bytes(oldDT)
shutil.copyfile(inputs/'config.fex',work/'sys_config.bin')
up=root.parent/'tools/pack/pctools/linux/mod_update/update_uboot'
r=subprocess.run([str(up),'-no_merge','input/u-boot.fex','sys_config.bin'],cwd=work,capture_output=True,text=True);print('update_uboot',r.returncode,r.stdout,r.stderr);assert r.returncode==0 and 'error' not in r.stdout.lower()
packed=(inputs/'u-boot.fex').read_bytes();assert oldDT in packed
assert packed[1536:len(newCombined)]==newCombined[1536:]
length=struct.unpack_from('<I',packed,20)[0];assert length==len(packed)
check=bytearray(packed);saved=struct.unpack_from('<I',check,12)[0];struct.pack_into('<I',check,12,0x5f0a6c39);assert sum(struct.unpack('<%dI'%(len(check)//4),check))&0xffffffff==saved
wrappers={}
for label,addr,target in [('read',0x43524a80,0x43524948),('write',0x43524b0c,0x43524a84)]:
 off=addr-0x43500000;word=struct.unpack_from('<I',packed,off)[0];assert word>>24==0xea
 imm=word&0xffffff;imm=imm-(1<<24) if imm&(1<<23) else imm
 actual=addr+8+imm*4;assert actual==target
 wrappers[label]=dict(address=hex(addr),file_offset=hex(off),instruction=hex(word),branch_target=hex(actual),logical_sector_addition=0)
manifest.update(product_name='omnigate-1g-physical0',output_name='t153-ddr4-emmc-omnigate-1g-physical0-loader.bin',expected_output_sha256='0'*64,hardware_validation='software-only',source_revision='T153 original FES/config plus source-built vendor U-Boot CONFIG_SUNXI_SDMMC_LOGICAL_OFFSET=0; stock Jul21 control DT',raw_addressing={'logical_offset_sectors':0,'mapping':'FES normal MMC sector N -> eMMC user physical sector N','boot0_special_writer':'must not be used','wrapper_evidence':wrappers})
for e in manifest['entries']:
 e['sha256']=hashlib.sha256((work/e['path']).read_bytes()).hexdigest();e['packed_sha256']=e['sha256']
(work/'loader.json').write_text(json.dumps(manifest,indent=2)+'\n')
repo=Path('/home/ubuntu/T113-MainLine/tools/allwinner-loader');tool=repo/'tools/allwinner_loader.py';out=work/'dist'
r=subprocess.run(['python3',str(tool),'build','--manifest',str(work/'loader.json'),'--output-dir',str(out)],capture_output=True,text=True);assert r.returncode==1 and 'does not match expected_output_sha256' in r.stderr
spec=importlib.util.spec_from_file_location('loader',tool);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);image=out/manifest['output_name'];info=m.parse_image(image)
for actual,e in zip(info['files'],manifest['entries']):assert actual['sha256']==e['sha256']
manifest['expected_output_sha256']=info['sha256'];(work/'loader.json').write_text(json.dumps(manifest,indent=2)+'\n')
subprocess.run(['python3',str(tool),'build','--manifest',str(work/'loader.json'),'--output-dir',str(out),'--check-reproducible'],check=True)
report=m.verify_image(work/'loader.json',image)
shutil.copyfile(image,root/'artifacts/t153-fes-physical0-ram-loader.img')
shutil.copyfile(out/manifest['output_name'].replace('.bin','.manifest.json'),root/'artifacts/t153-fes-physical0-ram-loader.manifest.json')
record={'status':'software-only; NOT yet hardware booted','raw_mapping':manifest['raw_addressing'],'source_nodtb_sha256':hashlib.sha256(new).hexdigest(),'stock_control_dtb_offset':hex(0xd32e8),'stock_control_dtb_bytes':len(oldDT),'stock_control_dtb_sha256':hashlib.sha256(oldDT).hexdigest(),'combined_code_before_header_update_sha256':hashlib.sha256(newCombined).hexdigest(),'updated_uboot_sha256':hashlib.sha256(packed).hexdigest(),'updated_uboot_bytes':len(packed),'entries':report['files'],'loader_sha256':report['sha256'],'loader_bytes':report['image_size'],'fes_ddr_mhz':1066,'fes_ddr_auto_scan':True,'physical_dram_mib':1024,'boot0_special_writer_allowed':False}
(root/'logs/t153-fes-physical0-loader-audit.json').write_text(json.dumps(record,indent=2)+'\n')
print('READY',root/'artifacts/t153-fes-physical0-ram-loader.img',report['image_size'],report['sha256'])
