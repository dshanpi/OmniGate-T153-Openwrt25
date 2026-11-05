#!/usr/bin/env python3
"""Exercise actual early-SPL C and validate linked placement/call ordering."""
from pathlib import Path
import argparse, subprocess, tempfile, re
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--build-dir',type=Path,default=root/'u-boot-work/build-t153-source-1g-smp')
a=p.parse_args()
with tempfile.TemporaryDirectory() as tmp:
    exe=Path(tmp)/'fel-request-test'
    subprocess.run(['cc','-O2','-Wall','-Wextra','-Werror','-fsanitize=undefined',str(root/'tests/smp/fel-request-test.c'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
objdump=next((root/'openwrt/staging_dir').glob('toolchain-arm_cortex-a7*/bin/arm-openwrt-linux-muslgnueabi-objdump'))
dis=subprocess.check_output([str(objdump),'-d',str(a.build_dir/'spl/u-boot-spl')],text=True)
def fn(name):
    return dis.split('<'+name+'>:',1)[1].split('\n\n',1)[0]
board=fn('board_init_f')
assert board.index('<preloader_console_init>') < board.index('<t153_handle_fel_request>') < board.index('<t153_ddr_init>')
request=fn('t153_handle_fel_request')
assert '<t153_enter_fel_fresh>' in request and '<hang>' in request
fresh=fn('t153_enter_fel_fresh')
assert 'cpsid' in fresh and 'mrrc' in fresh and 'cr14' in fresh
assert re.search(r'mov\s+r0, #32',fresh) and 'bx\tr0' in fresh
assert 'cr12' in fresh and '[r0, #16]' in fresh
assert not re.search(r'\bbl[x]?\s',fresh), 'fresh entry must remain self-contained SRAM assembly'
symbols=(a.build_dir/'spl/u-boot-spl.sym').read_text()
for name in ('t153_handle_fel_request','t153_enter_fel_fresh','t153_fel_context'):
    address=int(next(line.split()[0] for line in symbols.splitlines() if line.split() and line.split()[-1]==name),16)
    assert 0x40060 <= address < 0x58060, name+' must reside in SPL SRAM'
(root/'logs/t153-fel-request-disassembly.txt').write_text(board+'\n'+request+'\n'+fresh)
print('PASS: linked pre-DDR hook; ARM BootROM 0x20; VBAR context restore; self-contained SRAM transition')
