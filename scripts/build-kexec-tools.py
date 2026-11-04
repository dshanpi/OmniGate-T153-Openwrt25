#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Build a pinned, static ARM kexec tool without changing OpenWrt selection/config.

This script never runs kexec or accesses a board. It reuses the existing OpenWrt
cross toolchain and applies the package's hash-locked upstream patches.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tarfile
import urllib.request

ROOT=Path(__file__).resolve().parents[1]


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def need(value,message):
    if not value:
        raise ValueError(message)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--jobs',type=int,default=2)
    parser.add_argument('--output-dir',type=Path,default=ROOT/'artifacts/kexec-tools')
    args=parser.parse_args();need(args.jobs>0,'jobs must be positive')
    lock=json.loads((ROOT/'configs/kexec-tools.lock.json').read_text())
    package=ROOT/'openwrt/package/boot/kexec-tools'
    need(digest(package/'Makefile')==lock['package_makefile_sha256'],'OpenWrt kexec package definition changed')
    for name,wanted in lock['patches'].items():
        need(digest(package/'patches'/name)==wanted,'OpenWrt kexec patch changed: '+name)
    toolchains=list((ROOT/'openwrt/staging_dir').glob('toolchain-arm_cortex-a7*/bin/arm-openwrt-linux-muslgnueabi-gcc'))
    need(len(toolchains)==1,'build the OpenWrt ARM cross toolchain first')
    prefix=str(toolchains[0])[:-3]
    archive=ROOT/'downloads'/('kexec-tools-'+lock['version']+'.tar.xz')
    archive.parent.mkdir(exist_ok=True)
    if not archive.is_file():
        temporary=archive.with_suffix(archive.suffix+'.part')
        urllib.request.urlretrieve(lock['source_url'],temporary)
        need(digest(temporary)==lock['source_sha256'],'download SHA256 mismatch')
        os.replace(temporary,archive)
    need(digest(archive)==lock['source_sha256'],'source SHA256 mismatch')
    work=ROOT/'build/kexec-tools'
    work.mkdir(parents=True,exist_ok=True)
    source=work/('kexec-tools-'+lock['version'])
    marker=source/'.t153-source-sha256'
    if not source.exists():
        with tarfile.open(archive,'r:xz') as tar:
            tar.extractall(work,filter='data')
        marker.write_text(lock['source_sha256']+'\n')
    need(marker.is_file() and marker.read_text().strip()==lock['source_sha256'],'existing source tree lacks pinned marker')
    logs=ROOT/'logs';logs.mkdir(exist_ok=True)
    log=logs/'kexec-tools-build.log'
    commands=[]
    env=os.environ.copy()
    env.update(CC=prefix+'gcc',TARGET_CC=prefix+'gcc',AR=prefix+'ar',LD=prefix+'ld',TARGET_LD=prefix+'ld',
               AS=prefix+'as',STRIP=prefix+'strip',OBJCOPY=prefix+'objcopy',BUILD_CC='cc',
               CFLAGS=lock['cflags'],LDFLAGS=lock['ldflags'],STAGING_DIR=str(ROOT/'openwrt/staging_dir'),
               SOURCE_DATE_EPOCH=str(lock['source_date_epoch']),LC_ALL='C')
    with log.open('w') as output:
        def run(cmd):
            commands.append(cmd);subprocess.run(cmd,cwd=source,env=env,stdout=output,stderr=subprocess.STDOUT,check=True)
        for name in lock['patches']:
            patch=package/'patches'/name
            probe=subprocess.run(['patch','--batch','--forward','--dry-run','-p1','-i',str(patch)],cwd=source,capture_output=True)
            if probe.returncode==0:
                run(['patch','--batch','--forward','-p1','-i',str(patch)])
            else:
                run(['patch','--batch','--reverse','--dry-run','-p1','-i',str(patch)])
        run(['./configure',*lock['configure']])
        run(['make','-j'+str(args.jobs)])
    binary=source/'build/sbin/kexec'
    need(binary.is_file(),'kexec output not found')
    args.output_dir.mkdir(parents=True,exist_ok=True)
    output=args.output_dir/'kexec'
    shutil.copyfile(binary,output)
    subprocess.run([prefix+'strip','--strip-unneeded',str(output)],check=True)
    output.chmod(0o755)
    raw=output.read_bytes()
    need(raw[:7]==b'\x7fELF\x01\x01\x01' and struct.unpack_from('<H',raw,18)[0]==40,'expected ELF32 little-endian ARM')
    readelf=subprocess.check_output([prefix+'readelf','-h','-l','-d','-A',str(output)],text=True)
    need('INTERP' not in readelf and '(NEEDED)' not in readelf,'expected static binary without runtime shared libraries')
    need('hard-float ABI' in readelf and 'VFP registers' in readelf,'expected OpenWrt hard-float ABI')
    (args.output_dir/'elf-validation.txt').write_text(readelf)
    shutil.copyfile(source/'COPYING',args.output_dir/'COPYING')
    manifest={'schema_version':1,'status':'static ARM build and ELF checks passed; tool never executed',
              **lock,'binary':{'file':'kexec','bytes':output.stat().st_size,'sha256':digest(output)},
              'cross_compiler':subprocess.check_output([prefix+'gcc','--version'],text=True).splitlines()[0],
              'compile_commands':commands,'toolchain_reused':True,'openwrt_config_modified':False,
              'board_io':False,'kexec_executed':False,'hardware_validated':False}
    (args.output_dir/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    (args.output_dir/'SHA256SUMS').write_text(''.join(digest(p)+'  '+p.name+'\n' for p in sorted(args.output_dir.iterdir()) if p.is_file() and p.name!='SHA256SUMS'))
    print(json.dumps(manifest['binary'],indent=2))


if __name__=='__main__':
    main()
