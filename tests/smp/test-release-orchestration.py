#!/usr/bin/env python3
"""Run the real release driver with lightweight build doubles; no target build."""
from pathlib import Path
import hashlib, json, shutil, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[2]
SHELL_STEPS=['scripts/bootstrap.sh','scripts/prepare-source.sh','scripts/build.sh',
 'scripts/test-ddr-source.sh','scripts/build-uboot-source-1g.sh',
 'scripts/build-uboot-source-1g-smp.sh','scripts/package-artifacts.sh']
PY_STEPS=['tests/build/test-feed-cache.py','tests/clock/test-t153-mmc-clock.py',
 'tests/reboot/test-command.py','tests/reboot/test_kernel_reboot.py',
 'scripts/package-uboot-source-1g.py','scripts/package-disk-image.py',
 'tests/flash/test-disk-image.py','tests/smp/test-smp-dt.py',
 'tests/smp/test-fel-request.py','scripts/package-uboot-source-1g-smp.py',
 'scripts/package-disk-smp.py','scripts/prepare-fes-stock.py',
 'scripts/package-fes-ddr933.py','tests/fes/test-portable-loader.py']
def fixture(root):
    (root/'scripts').mkdir()
    shutil.copyfile(ROOT/'scripts/build-release.sh',root/'scripts/build-release.sh')
    for name in SHELL_STEPS:
        p=root/name;p.parent.mkdir(parents=True,exist_ok=True)
        body="#!/bin/sh\nset -eu\nprintf '%s\\n' "+name+' >> calls.txt\n'
        if name=='scripts/build.sh':
            body+='mkdir -p openwrt\nprintf kernel-config > openwrt/.config\n'
        if name in ('scripts/build-uboot-source-1g.sh','scripts/build-uboot-source-1g-smp.sh'):
            profile='1g-smp' if name.endswith('-smp.sh') else '1g'
            body+=f'mkdir -p u-boot-work/build-t153-source-{profile}\nprintf config-{profile} > u-boot-work/build-t153-source-{profile}/.config\n'
        p.write_text(body)
    for name in PY_STEPS:
        p=root/name;p.parent.mkdir(parents=True,exist_ok=True)
        body='from pathlib import Path\nimport json\nwith open("calls.txt","a") as f:f.write('+repr(name+'\n')+')\n'
        if name in ('scripts/package-disk-image.py','scripts/package-disk-smp.py'):
            profile='1g-smp' if name.endswith('-smp.py') else '1g'
            manifest={'cpu_count':4 if profile=='1g-smp' else 1,'image':{'file':f't153-{profile}.img','bytes':4,'sha256':hashlib.sha256(profile.encode()).hexdigest()}}
            body+='Path("artifacts").mkdir(exist_ok=True)\n'
            body+=f'Path("artifacts/t153-disk-{profile}-manifest.json").write_text({json.dumps(json.dumps(manifest))})\n'
        p.write_text(body)
for smp,fes in ((False,False),(True,False),(True,True)):
    with tempfile.TemporaryDirectory() as tmp:
        root=Path(tmp);fixture(root)
        args=['bash','scripts/build-release.sh']+(['--smp'] if smp else [])+(['--with-fes'] if fes else [])
        subprocess.run(args,cwd=root,stdout=subprocess.DEVNULL,check=True)
        logdir=Path((root/'logs/latest-rebuild-path.txt').read_text().strip())
        report=json.loads((logdir/'build-report.json').read_text())
        profile='1g-smp' if smp else '1g'
        assert report['profile']=='source-'+profile and report['cpu_count']==(4 if smp else 1)
        assert report['disk']['file']==f't153-{profile}.img'
        assert report['uboot_config_sha256']==hashlib.sha256(('config-'+profile).encode()).hexdigest()
        assert not report['incremental'] and not report['hardware_validated_by_this_run']
        steps=report['steps'];assert all(v==0 for v in steps.values())
        assert ('smp-dt' in steps)==smp and ('fes-tests' in steps)==fes
        calls=(root/'calls.txt').read_text().splitlines()
        if smp:
            assert calls.index('scripts/package-disk-image.py') < calls.index('scripts/build-uboot-source-1g-smp.sh') < calls.index('scripts/package-disk-smp.py')
        else: assert 'scripts/build-uboot-source-1g-smp.sh' not in calls
with tempfile.TemporaryDirectory() as tmp:
    root=Path(tmp);fixture(root)
    (root/'u-boot-work/build-t153-source-1g-smp').mkdir(parents=True)
    result=subprocess.run(['bash','scripts/build-release.sh','--smp'],cwd=root,capture_output=True,text=True)
    assert result.returncode!=0 and 'unused build-t153-source-1g-smp' in result.stderr
    assert not (root/'calls.txt').exists()
print('PASS: actual driver selects CPU0/SMP artifacts and configs; step ordering; optional FES; clean SMP tree refusal (build steps mocked)')
