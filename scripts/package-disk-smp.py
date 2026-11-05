#!/usr/bin/env python3
"""Derive a bounded SMP disk candidate from an independently verified 1GiB disk.

Only its source SPL/U-Boot boot area changes. Kernel, FAT boot files, rootfs,
MBR and partition geometry remain byte-identical. No device access.
"""
from pathlib import Path
import argparse, gzip, importlib.util, json, os, shutil, tempfile
ROOT=Path(__file__).resolve().parents[1];ART=ROOT/'artifacts'
spec=importlib.util.spec_from_file_location('disk',ROOT/'scripts/package-disk-image.py')
disk=importlib.util.module_from_spec(spec);spec.loader.exec_module(disk)
need=disk.need
NAME='t153-openwrt25-source-1g-smp-disk.img';MANIFEST='t153-disk-1g-smp-manifest.json'
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--base-root',type=Path,default=ROOT,help='build root containing the validated CPU0 1GiB disk, kernel ELF and rootfs')
p.add_argument('--verify-only',action='store_true')
a=p.parse_args();base=a.base_root.resolve();disk.ROOT=base
m=json.loads(disk.regular_file(base/'artifacts/t153-disk-1g-manifest.json').read_text())
need(m['physical_dram_mib']==1024 and m['linux_usable_mib']==384 and m['cpu_count']==1,'base profile is not source 1GiB/CPU0')
need(m['vendor_ddr_object_linked'] is False,'base uses vendor DDR')
original=disk.regular_file(base/'artifacts'/m['image']['file'])
validation=json.loads((ART/'uboot-source-1g-smp-validation.json').read_text())
need(validation['linux_cpus_enabled']==4 and validation['psci_version']=='0.1','SMP boot-chain validation missing')
for name,info in validation['outputs'].items():
    path=disk.regular_file(ART/name)
    need(path.stat().st_size==info['bytes'] and disk.file_sha(path)==info['sha256'],'SMP artifact hash '+name)
area=(ART/'t153-uboot-2026.07-source-1g-smp-sd-bootarea.bin').read_bytes()
area_info=disk.verify_bootarea(area)
need(disk.SPL_OFFSET+len(area)<disk.BOOT_START*disk.SECTOR,'boot-area overlaps boot filesystem')
for name in (NAME,NAME+'.gz',MANIFEST):disk.safe_output(ART/name)
with tempfile.TemporaryDirectory(prefix='.smp-disk-',dir=ROOT) as tmp:
    tmp=Path(tmp);check=tmp/'base';check.mkdir()
    disk.verify_disk(original,m,check)
    if a.verify_only:
        result=json.loads((ART/MANIFEST).read_text());check=tmp/'verify';check.mkdir()
        need(result['cpu_count']==4 and result['bootarea']==area_info,'wrong SMP metadata')
        need(result['base_image']['sha256']==m['image']['sha256'],'wrong base image')
        disk.verify_disk(ART/NAME,result,check)
        need(disk.file_sha(ART/(NAME+'.gz'))==result['compressed_image']['sha256'],'gzip hash')
        with gzip.open(ART/(NAME+'.gz'),'rb') as z:
            import hashlib
            digest=hashlib.sha256();total=0
            while chunk:=z.read(1024*1024):digest.update(chunk);total+=len(chunk)
        need(total==disk.DISK_BYTES and digest.hexdigest()==result['image']['sha256'],'gzip content')
        print('PASS: SMP whole-disk, retained base filesystems/FIT, source boot area and gzip hashes')
    else:
        image=tmp/NAME;shutil.copyfile(original,image)
        with image.open('r+b') as f:
            f.seek(disk.SPL_OFFSET);f.write(area)
        # Check every unchanged byte, including the entire FAT/ext4 regions.
        with original.open('rb') as old,image.open('rb') as new:
            need(old.read(disk.SPL_OFFSET)==new.read(disk.SPL_OFFSET),'MBR changed')
            old.seek(disk.SPL_OFFSET+len(area));new.seek(disk.SPL_OFFSET+len(area))
            while chunk:=old.read(1024*1024):need(chunk==new.read(len(chunk)),'byte outside boot-area changed')
        result=dict(m)
        result.update(cpu_count=4,source_profile='dshanpi_t153_source_1g_smp_defconfig',psci_version='0.1',psci_cpu_on=True,psci_cpu_off=False,
                      secure_sram={'base':'0x50000','reserved_bytes':32768},base_image=m['image'],bootarea=area_info,
                      image={'file':NAME,'bytes':image.stat().st_size,'sha256':disk.file_sha(image)},
                      limitations=['hardware validation pending','CPU_ON only; hotplug/suspend/multi-core kexec unsupported','Linux remains 384MiB','whole-disk write replaces factory partition table'])
        check=tmp/'verify';check.mkdir();result['checks']=disk.verify_disk(image,result,check)
        zipped=tmp/(NAME+'.gz')
        with image.open('rb') as src,zipped.open('wb') as dst:
            with gzip.GzipFile(fileobj=dst,filename='',mode='wb',mtime=0,compresslevel=9) as z:shutil.copyfileobj(src,z)
        result['compressed_image']={'file':zipped.name,'bytes':zipped.stat().st_size,'sha256':disk.file_sha(zipped)}
        os.replace(image,ART/NAME);os.replace(zipped,ART/zipped.name)
        (ART/MANIFEST).write_text(json.dumps(result,indent=2)+'\n')
        print(json.dumps({'image':result['image'],'base_image':result['base_image'],'hardware_verified':False},indent=2))
