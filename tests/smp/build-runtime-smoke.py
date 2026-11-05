#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Build a static ARM smoke tool using this checkout's existing OpenWrt toolchain."""
from pathlib import Path
import hashlib,json,os,struct,subprocess
ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'tests/smp/runtime-smoke.c'
compilers=list((ROOT/'openwrt/staging_dir').glob('toolchain-arm_cortex-a7*/bin/arm-openwrt-linux-muslgnueabi-gcc'))
if len(compilers)!=1:
    raise SystemExit('Build the project OpenWrt toolchain first; exactly one Cortex-A7 compiler is required.')
cc=compilers[0];output=ROOT/'artifacts/diagnostics/t153-smp-smoke'
output.parent.mkdir(parents=True,exist_ok=True)
command=[str(cc),'-std=c11','-O2','-g0','-mcpu=cortex-a7','-mfpu=neon-vfpv4','-mfloat-abi=hard',
         '-Wall','-Wextra','-Werror','-pthread','-static',str(SOURCE),'-o',str(output)]
env=dict(os.environ,STAGING_DIR=str(ROOT/'openwrt/staging_dir'),LC_ALL='C',SOURCE_DATE_EPOCH='1782864000')
subprocess.run(command,check=True,env=env)
strip_command=[str(cc)[:-3]+'strip','--strip-unneeded',str(output)]
subprocess.run(strip_command,check=True,env=env)
data=output.read_bytes()
assert data[:7]==b'\x7fELF\x01\x01\x01','expected ELF32 little-endian'
assert struct.unpack_from('<HH',data,16)==(2,40),'expected ARM executable'
flags=struct.unpack_from('<I',data,36)[0];assert flags&0x400,'expected hard-float ABI'
phoff=struct.unpack_from('<I',data,28)[0];phsize,phcount=struct.unpack_from('<HH',data,42)
assert all(struct.unpack_from('<I',data,phoff+i*phsize)[0]!=3 for i in range(phcount)),'dynamic PT_INTERP is forbidden'
report={'status':'compiled and ELF-checked; target execution is separate','source':'tests/smp/runtime-smoke.c',
        'source_sha256':hashlib.sha256(SOURCE.read_bytes()).hexdigest(),'compiler':subprocess.check_output([str(cc),'--version'],text=True).splitlines()[0],
        'command':command,'strip_command':strip_command,'build_script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'output':str(output.relative_to(ROOT)),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),
        'elf':'ELF32 ARM, EABI hard-float, no PT_INTERP','hardware_executed_by_builder':False}
(output.parent/'t153-smp-smoke.manifest.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
