#!/usr/bin/env python3
"""Validate and package the experimental mainline U-Boot RAM chainloader."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess

root = Path(__file__).resolve().parents[1]
build = root / 'u-boot-work/build-t153-ram'
art = root / 'artifacts'
spec = importlib.util.spec_from_file_location('verify_openwrt', root/'scripts/verify-artifacts.py')
verify = importlib.util.module_from_spec(spec)
spec.loader.exec_module(verify)
require, fdt_nodes, text = verify.require, verify.fdt_nodes, verify.text
cfg = (build/'.config').read_text()
for key, value in {
    'TARGET_DSHANPI_T153_RAM':'y', 'TEXT_BASE':'0x4a000000',
    'CUSTOM_SYS_INIT_SP_ADDR':'0x49f00000', 'SYS_ARCH_TIMER':'y',
    'SKIP_LOWLEVEL_INIT_ONLY':'y', 'BOARD_EARLY_INIT_F':'y',
    'OF_SEPARATE':'y', 'FIT':'y', 'SHA256':'y', 'BOOTM_LINUX':'y',
    'CMD_BOOTM':'y', 'CMD_BOOTZ':'y', 'CMD_IMI':'y', 'CMD_LOADB':'y',
    'CMD_MMC':'y', 'CMD_FAT':'y', 'CMD_EXT4':'y', 'CMD_FS_GENERIC':'y',
    'DM_MMC':'y', 'ENV_IS_NOWHERE':'y', 'BOOTDELAY':'-1',
    'SYS_NS16550_MEM32':'y', 'SYS_BOOTM_LEN':'0x4000000',
}.items():
    require(f'CONFIG_{key}={value}\n' in cfg, 'U-Boot option '+key)
for key in ['SPL', 'ARCH_SUNXI', 'ARMV7_NONSEC', 'ARMV7_PSCI',
            'MMC_WRITE', 'MMC_HW_PARTITIONING', 'CMD_SAVEENV', 'NET',
            'FAT_WRITE', 'EXT4_WRITE', 'USE_BOOTCOMMAND', 'CMD_MEMTEST']:
    require(f'CONFIG_{key}=y\n' not in cfg, 'unsupported U-Boot option '+key)
elf = (build/'u-boot').read_bytes()
require(elf[:7] == b'\x7fELF\x01\x01\x01', 'ELF32 little-endian')
etype, machine = struct.unpack_from('<HH', elf, 16)
entry, phoff = struct.unpack_from('<II', elf, 24)
require((etype, machine, entry) == (2, 40, 0x4a000000), 'ARM ELF entry')
phsize, phnum = struct.unpack_from('<HH', elf, 42)
for i in range(phnum):
    ptype, offset, vaddr, paddr, filesz, memsz = struct.unpack_from('<6I', elf, phoff+i*phsize)
    if ptype == 1:
        require(0x4a000000 <= paddr <= paddr+memsz < 0x4a200000, 'ELF load segment bounds')
binary = (build/'u-boot.bin').read_bytes()
control = (build/'u-boot.dtb').read_bytes()
require(binary == (build/'u-boot-nodtb.bin').read_bytes()+control, 'appended control DTB')
require(len(binary) < 0x200000, 'U-Boot source load size')
require(b'U-Boot 2026.07' in binary, 'actual U-Boot version')
symbols = (build/'u-boot.sym').read_text()
for name in ['board_early_init_f', 'dram_init', 'enable_caches',
             't153_mmc_probe', 't153_mmc_send_cmd', 'do_bootm']:
    require(name in symbols, 'linked function '+name)
require('init_DRAM' not in symbols and 'mctl_phy_init' not in symbols, 'no vendor DRAM object')
board = fdt_nodes(control)
require(b'dshanpi,t153mx-omnigate\0' in board['/']['compatible'], 'control DT board')
require(struct.unpack('>II', board['/memory@48000000']['reg']) == (0x48000000,0x18000000), 'usable RAM')
uart = board['/serial@2600000']
require(struct.unpack('>I', uart['clock-frequency'])[0] == 24000000, 'UART oscillator')
require(struct.unpack('>I', uart['reg-io-width'])[0] == 4, 'UART access width')
for name, addr, width in [('mmc@4020000',0x04020000,4), ('mmc@4022000',0x04022000,8)]:
    node=board['/'+name]
    require(struct.unpack('>II',node['reg']) == (addr,0x1000), 'MMC register base')
    require(struct.unpack('>I',node['bus-width'])[0] == width, 'MMC bus width')
    require(struct.unpack('>I',node['max-frequency'])[0] == 12000000, 'conservative MMC clock')
# This Linux DTB is only used for the factory bootm handoff. U-Boot itself
# uses its appended control DTB. The final OpenWrt FIT carries its own Linux DTB.
linux_dtb = art/'t153-omnigate.dtb'
require(linux_dtb.exists(), 'package verified OpenWrt artifacts first')
linux_nodes = fdt_nodes(linux_dtb.read_bytes())
require(b'dshanpi,t153mx-omnigate\0' in linux_nodes['/']['compatible'], 'handoff board DTB')
shutil.copyfile(linux_dtb, build/'chainload-handoff.dtb')
its = build/'t153-uboot-ram.its'
its.write_text('''/dts-v1/;
/ {
    description = "T153 mainline U-Boot 2026.07 RAM chainload, NO SPL";
    #address-cells = <1>;
    images {
        uboot {
            description = "U-Boot proper via ARM Linux bootm handoff";
            data = /incbin/("u-boot.bin");
            type = "kernel";
            arch = "arm";
            os = "linux";
            compression = "none";
            load = <0x4a000000>;
            entry = <0x4a000000>;
            hash-1 { algo = "sha256"; };
        };
        fdt {
            description = "Handoff DT only; U-Boot uses its appended DT";
            data = /incbin/("chainload-handoff.dtb");
            type = "flat_dt";
            arch = "arm";
            compression = "none";
            hash-1 { algo = "sha256"; };
        };
    };
    configurations {
        default = "conf-t153";
        conf-t153 { kernel = "uboot"; fdt = "fdt"; };
    };
};
''')
env=dict(os.environ, SOURCE_DATE_EPOCH='1782864000')
fit = build/'t153-uboot-ram.itb'
subprocess.run([str(build/'tools/mkimage'), '-f', its.name, fit.name],cwd=build, env=env, check=True)
fit_nodes = fdt_nodes(fit.read_bytes())
for name, payload in [('uboot',binary), ('fdt',linux_dtb.read_bytes())]:
    node = fit_nodes['/images/'+name]
    require(node['data'] == payload, 'chainload FIT actual '+name)
    require(fit_nodes['/images/'+name+'/hash-1']['value'] == hashlib.sha256(payload).digest(), 'chainload FIT SHA256 '+name)
require(struct.unpack('>I',fit_nodes['/images/uboot']['entry'])[0] == entry, 'FIT ELF entry matches')
require(fit_nodes['/images/uboot']['entry'] == fit_nodes['/images/uboot']['load'], 'FIT load matches')
require(text(fit_nodes['/configurations/conf-t153']['kernel']) == 'uboot', 'FIT selected payload')
require(0x4c000000+fit.stat().st_size < 0x4e000000, 'chainloader FIT staging bounds')
# Check the staged OpenWrt FIT is well above kernel decompression and below
# the 16 MiB heap and U-Boot relocation area at the top of the 512 MiB RAM.
fits=list(art.glob('*dshanpi_t153mx-omnigate-initramfs-kernel.bin'))
require(len(fits)==1, 'one packaged OpenWrt FIT')
require(0x50000000+max(fit.stat().st_size,fits[0].stat().st_size) < 0x58000000, 'staging margin')
report = {
    'status':'software checks passed; hardware NOT tested',
    'upstream':'U-Boot 2026.07',
    'archive_url':'https://ftp.denx.de/pub/u-boot/u-boot-2026.07.tar.bz2',
    'archive_sha256':'78e8bfc382fe388f9b55aa1daf8c563522a037779b5d4c349d1415e381f1243e',
    'mode':'RAM chainload U-Boot proper; DDR/monitor inherited from factory boot chain',
    'cold_boot_spl':False,
    'binary_bytes':len(binary), 'fit_bytes':fit.stat().st_size,
    'chainload_fit_staging':'0x4c000000',
    'entry':'0x4a000000', 'initial_stack':'0x49f00000',
    'usable_ram_start':'0x48000000','usable_ram_size':'0x18000000',
    'mmc_max_hz':12000000,'media_write_enabled':False,
    'uboot_network_usb_supported':False,
    'binary_sha256':hashlib.sha256(binary).hexdigest(),
}
for src, name in [(build/'u-boot.bin','t153-uboot-2026.07-ram.bin'),
                  (fit,'t153-uboot-2026.07-ram.itb'),
                  (build/'u-boot.dtb','t153-uboot-2026.07-control.dtb'),
                  (build/'.config','uboot.config')]:
    shutil.copyfile(src, art/name)
(art/'uboot-validation.json').write_text(json.dumps(report,indent=2)+'\n')
(root/'logs/uboot-validation.json').write_text(json.dumps(report,indent=2)+'\n')
(art/'uboot.buildinfo').write_text(json.dumps({
    'source':report['archive_url'],'source_sha256':report['archive_sha256'],
    'compiler':'OpenWrt GCC 14.3.0 / binutils 2.44',
    'build_command':'sh scripts/build-uboot.sh',
    'config':'port/uboot/files/configs/dshanpi_t153_ram_defconfig',
    'port':'port/uboot/files; scripts/apply-uboot.py',
    'scope':report['mode'],
},indent=2)+'\n')
print(json.dumps(report,indent=2))
print('PASS: ARM ELF entry/segments, actual linked code, appended DTB, configuration, FIT SHA256/payloads and RAM ranges')
