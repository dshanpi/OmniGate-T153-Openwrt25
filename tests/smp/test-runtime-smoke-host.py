#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Host-only scheduling/control-flow tests; they do not qualify T153 hardware."""
from pathlib import Path
import json,os,subprocess,tempfile,time
ROOT=Path(__file__).resolve().parents[2]
allowed=sorted(os.sched_getaffinity(0))
if len(allowed)<4:
    raise SystemExit('Need four permitted host CPUs for the host-only check.')
cpus=allowed[:4]
with tempfile.TemporaryDirectory(prefix='t153-smp-host-') as td:
    exe=Path(td)/'runtime-smoke'
    subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-pthread','-fsanitize=undefined',
                    str(ROOT/'tests/smp/runtime-smoke.c'),'-o',str(exe)],check=True)
    runs=[]
    def run(args,expected,limited=False):
        begin=time.monotonic()
        proc=subprocess.run([str(exe),*args],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
                            timeout=6,preexec_fn=(lambda:os.sched_setaffinity(0,{cpus[0]})) if limited else None)
        assert proc.returncode==expected,(args,proc.returncode,proc.stdout)
        runs.append({'arguments':args,'exit':proc.returncode,'seconds':round(time.monotonic()-begin,3),'output':proc.stdout})
        return proc.stdout
    selected=','.join(map(str,cpus))
    output=run(['--cpus',selected,'--duration-ms','300'],0)
    assert 'MODE=explicit-cpus' in output and 'RESULT PASS' in output
    for cpu in cpus:
        assert f'CPU target={cpu} observed={cpu} ' in output
    run(['--cpus',selected,'--duration-ms','100'],1,limited=True)
    run(['--cpus',f'{cpus[0]},{cpus[0]},{cpus[2]},{cpus[3]}'],2)
    run(['--duration-ms','0'],2)
    run(['--duration-ms','3001'],2)
    run(['--cpus',',1,2,3,4'],2)
report={'scope':'host-only control-flow and short concurrent run, not T153 hardware','allowed_host_cpus':allowed,
        'checks':runs,'result':'PASS'}
(ROOT/'logs').mkdir(exist_ok=True)
(ROOT/'logs/smp-runtime-host-tests.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'PASS: {len(runs)} host-only success/invalid-argument/restricted-affinity checks')
