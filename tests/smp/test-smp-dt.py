#!/usr/bin/env python3
"""Execute the actual opt-in board fixup on hostile/legacy DT handoffs."""
from pathlib import Path
import ctypes, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as td:
    td=Path(td)
    subprocess.run(['cc','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror','-fsanitize=undefined',
                    '-DT153_SMP_DT_HOST_TEST',str(ROOT/'port/uboot/files/board/dshanpi/t153/smp-dt.c'),
                    '-lfdt','-o',str(td/'fixup.so')],check=True)
    lib=ctypes.CDLL(str(td/'fixup.so'));lib.t153_smp_fixup_dt.argtypes=[ctypes.c_void_p]
    for invalid in (False, True):
        cpus=''.join(f'cpu@{i} {{ device_type="cpu"; compatible="arm,cortex-a7"; reg=<{i if not invalid or i != 3 else 4}>; status="disabled"; }};' for i in range(4))
        src='/dts-v1/; / { #address-cells=<1>; #size-cells=<1>; cpus { #address-cells=<1>; #size-cells=<0>; '+cpus+' }; psci { compatible="arm,psci-1.0", "arm,psci-0.2"; method="hvc"; cpu_off=<1>; cpu_suspend=<2>; migrate=<3>; }; };'
        (td/'in.dts').write_text(src)
        subprocess.run(['dtc','-q','-I','dts','-O','dtb','-p','8192','-o',str(td/'in.dtb'),str(td/'in.dts')],check=True)
        data=(td/'in.dtb').read_bytes();buf=ctypes.create_string_buffer(data,len(data))
        ret=lib.t153_smp_fixup_dt(buf)
        if invalid:
            assert ret<0, 'out-of-range CPU must not be advertised';continue
        assert ret==0
        first=buf.raw
        assert lib.t153_smp_fixup_dt(buf)==0
        assert buf.raw==first, 'fixup must not duplicate SRAM reservations or properties'
        (td/'out.dtb').write_bytes(buf.raw)
        def get(path,prop,kind='s'):
            return subprocess.check_output(['fdtget','-t'+kind,str(td/'out.dtb'),path,prop],text=True).strip()
        assert get('/psci','compatible')=='arm,psci'
        assert get('/psci','method')=='smc'
        assert get('/psci','cpu_on','x')=='95c1ba60'
        for absent in ('cpu_off','cpu_suspend','migrate'):
            assert subprocess.run(['fdtget',str(td/'out.dtb'),'/psci',absent],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode!=0
        for i in range(4):
            assert get('/cpus/cpu@'+str(i),'status')=='okay'
            assert get('/cpus/cpu@'+str(i),'enable-method')=='psci'
    image_fdt=(ROOT/'u-boot-work/u-boot-2026.07/boot/image-fdt.c').read_text()
    assert image_fdt.index('arch_fixup_fdt(blob)') < image_fdt.index('ft_board_setup(blob, gd->bd)')
print('PASS: exact PSCI0.1 CPU_ON-only DT; four CPUs; SRAM reservation/idempotence; invalid CPU rejected; callback ordering')
