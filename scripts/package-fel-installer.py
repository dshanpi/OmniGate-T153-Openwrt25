#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Package/verify a FEL RAM installer. --boot loads RAM only; it never writes media."""
import argparse
import gzip
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shlex
import stat
import struct
import subprocess
import sys
import zlib

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / 'artifacts'
CPIO_NAME = 't153-fel-installer.cpio'
SCRIPT_NAME = 't153-fel-installer.scr'
MANIFEST_NAME = 't153-fel-installer-manifest.json'
DISK_MANIFEST = 't153-disk-manifest.json'
DISK_GZIP = 't153-openwrt25-source-disk.img.gz'
UBOOT_NAME = 't153-uboot-2026.07-source-fel.bin'
HELPER_NAME = 't153-mmc-info'
FEL_REL = 'u-boot-work/sunxi-tools-660d7e08edd025c8f84ab8eba3d8a903a5b81ae0/sunxi-fel'
LOADS = {'fit': 0x50000000, 'cpio': 0x54000000, 'script': 0x4f100000}
EPOCH = 1782864000
BOARD = 'dshanpi,t153mx-omnigate'


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def file_hash(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def descriptor(path):
    return {'file': path.name, 'bytes': path.stat().st_size, 'sha256': file_hash(path)}


def check_descriptor(desc):
    name = desc['file']
    require(isinstance(name, str) and Path(name).name == name and name not in ('.', '..'), 'unsafe artifact filename')
    path = ART / name
    require(path.is_file() and not path.is_symlink(), 'missing or symlink artifact: ' + name)
    require(path.stat().st_size == desc['bytes'], 'artifact size mismatch: ' + name)
    require(file_hash(path) == desc['sha256'], 'artifact SHA256 mismatch: ' + name)
    return path


def newc(entries):
    """Only ordinary files and directories, owned by root, with deterministic metadata."""
    out = bytearray()
    for ino, (name, mode, data) in enumerate(entries + [('TRAILER!!!', 0, b'')], 1):
        require(name == 'TRAILER!!!' or (re.fullmatch(r't153(?:/[a-zA-Z0-9_.-]+)?', name) and all(part not in ('.', '..') for part in name.split('/'))), 'unsafe cpio path')
        encoded = name.encode('ascii') + b'\0'
        fields = (ino, mode, 0, 0, 2 if stat.S_ISDIR(mode) else 1, EPOCH,
                  len(data), 0, 0, 0, 0, len(encoded), 0)
        out += b'070701' + ''.join('%08x' % x for x in fields).encode('ascii')
        out += encoded
        out += bytes((-len(out)) % 4)
        out += data
        out += bytes((-len(out)) % 4)
    out += bytes((-len(out)) % 512)
    return bytes(out)


def parse_cpio(data):
    files = {}
    offset = 0
    while offset + 110 <= len(data):
        require(data[offset:offset+6] == b'070701', 'newc magic')
        fields = [int(data[offset+6+i*8:offset+14+i*8], 16) for i in range(13)]
        _, mode, uid, gid, _, _, size, _, _, rmaj, rmin, namesize, checksum = fields
        require(namesize > 1 and offset+110+namesize <= len(data), 'cpio name bounds')
        offset += 110
        encoded = data[offset:offset+namesize]
        require(encoded[-1:] == b'\0' and b'\0' not in encoded[:-1], 'cpio name terminator')
        name = encoded[:-1].decode('ascii')
        offset = (offset + namesize + 3) & ~3
        require(offset + size <= len(data), 'cpio data bounds')
        contents = data[offset:offset+size]
        offset = (offset + size + 3) & ~3
        if name == 'TRAILER!!!':
            require(size == 0 and not any(data[offset:]), 'cpio trailing bytes')
            return files
        require(re.fullmatch(r't153(?:/[a-zA-Z0-9_.-]+)?', name) is not None and all(part not in ('.', '..') for part in name.split('/')), 'cpio path outside /t153')
        require(name not in files, 'duplicate cpio path')
        require(uid == gid == rmaj == rmin == checksum == 0, 'cpio ownership/device/checksum')
        require(stat.S_ISREG(mode) or stat.S_ISDIR(mode), 'cpio disallows links and devices')
        require(not (mode & 0o7000), 'cpio privileged mode')
        require(not stat.S_ISDIR(mode) or size == 0, 'cpio directory payload')
        files[name] = (mode, contents)
    raise ValueError('cpio missing trailer')


def disk_info():
    path = ART / DISK_MANIFEST
    info = json.loads(path.read_text())
    require(info['schema_version'] == 1 and info['board'] == BOARD, 'disk manifest board/schema')
    require(info['image']['file'] == 't153-openwrt25-source-disk.img', 'raw disk name')
    require(info['compressed_image']['file'] == DISK_GZIP, 'compressed disk name')
    require(info['image']['bytes'] == 306184192, 'raw disk size')
    require(info['disk_signature'] == '15302512' and info['root_partuuid'] == '15302512-02', 'disk identity')
    path = check_descriptor(info['compressed_image'])
    digest = hashlib.sha256()
    size = 0
    with gzip.open(path, 'rb') as stream:
        while chunk := stream.read(1024 * 1024):
            size += len(chunk)
            require(size <= 306184192, 'oversized decompressed disk')
            digest.update(chunk)
    require(size == info['image']['bytes'], 'decompressed disk size')
    require(digest.hexdigest() == info['image']['sha256'], 'decompressed disk SHA256')
    return info


def verify_fit(path):
    spec = importlib.util.spec_from_file_location('t153_artifact_verify', ROOT / 'scripts/verify-artifacts.py')
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    tree = mod.fdt_nodes(path.read_bytes())
    cfg = tree['/configurations/' + mod.text(tree['/configurations']['default'])]
    require('ramdisk' not in cfg, 'rescue FIT must use external raw cpio')
    kernel = '/images/' + mod.text(cfg['kernel'])
    dt = '/images/' + mod.text(cfg['fdt'])
    require(mod.text(tree[kernel]['arch']) == 'arm', 'FIT architecture')
    require(mod.text(tree[kernel]['compression']) == 'gzip', 'FIT kernel compression')
    require(struct.unpack('>I', tree[kernel]['load'])[0] == 0x48008000, 'FIT kernel load address')
    for base in (kernel, dt):
        payload = tree[base]['data']
        algos = set()
        for name, props in tree.items():
            if name.startswith(base + '/hash'):
                algo = mod.text(props['algo'])
                actual = struct.pack('>I', zlib.crc32(payload)) if algo == 'crc32' else hashlib.new(algo, payload).digest()
                require(actual == props['value'], 'FIT payload hash: ' + name)
                algos.add(algo)
        require(bool(algos & {'sha1', 'sha256'}) and 'crc32' in algos, 'FIT digest/crc32 required; whole FIT is separately SHA256 checked')
    board = mod.fdt_nodes(tree[dt]['data'])
    require((BOARD + '\0').encode() in board['/']['compatible'], 'FIT board')
    zimage = gzip.decompress(tree[kernel]['data'])
    require(struct.unpack_from('<I', zimage, 0x24)[0] == 0x016f2818, 'FIT zImage magic')
    require(len(zimage) < 0x2000000 and path.stat().st_size < 0x2000000, 'FIT memory budget')
    return zimage


def kernel_support(fit):
    dirs = list((ROOT / 'openwrt/build_dir').glob('target-*/linux-sunxi_cortexa7/linux-6.12.113'))
    require(len(dirs) == 1, 'one Linux build directory required')
    linux = dirs[0]
    config = (linux / '.config').read_text()
    require('CONFIG_BLK_DEV_INITRD=y\n' in config, 'kernel needs external initrd support')
    require('CONFIG_INITRAMFS_FORCE=y\n' not in config, 'kernel must not ignore external initrd')
    require('CONFIG_INITRAMFS_COMPRESSION_NONE=y\n' in config, 'expected uncompressed embedded initramfs')
    require(verify_fit(fit) == (linux.parent / 'zImage-initramfs').read_bytes(), 'FIT is not this build initramfs kernel')
    embedded = (linux / 'usr/initramfs_data.cpio').read_bytes()
    spec = importlib.util.spec_from_file_location('t153_artifact_verify', ROOT / 'scripts/verify-artifacts.py')
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    require(b'export INITRAMFS=1' in mod.cpio_entries(embedded).get('init', b''), 'embedded OpenWrt RAM init')
    return {'external_initrd': True, 'force_builtin_initramfs': False,
            'external_cpio_compression': 'none', 'config_sha256': file_hash(linux / '.config'),
            'zimage_sha256': file_hash(linux.parent / 'zImage-initramfs')}


def script_text(size):
    return ('echo T153 full-source FEL RAM installer - no storage write performed\n'
            'setenv bootargs console=ttyS0,115200 earlycon=uart8250,mmio32,0x02600000,115200n8,24000000 '
            'loglevel=8 ignore_loglevel clk_ignore_unused rdinit=/init\n'
            'setenv initrd_high 0xffffffff\n'
            'setenv autostart yes\n'
            f'bootm 0x50000000 0x54000000:{size:x}\n'
            'echo RAM installer boot failed; staying in U-Boot\n')


def verify_script(data, expected):
    require(len(data) >= 72, 'script legacy header size')
    magic, crc, _, size, _, _, payload_crc = struct.unpack_from('>7I', data)
    require(magic == 0x27051956 and size == len(data) - 64, 'script legacy magic/size')
    require(data[29:32] == bytes((2, 6, 0)), 'script ARM/script/uncompressed type')
    header = bytearray(data[:64]); header[4:8] = bytes(4)
    require(zlib.crc32(header) == crc and zlib.crc32(data[64:]) == payload_crc, 'script CRC')
    length, end = struct.unpack_from('>II', data, 64)
    require(end == 0 and length == len(expected.encode()) and data[72:72+length] == expected.encode(), 'script exact commands')
    require(not any(data[72+length:]), 'script padding')


def package():
    info = disk_info()
    fits = list(ART.glob('*dshanpi_t153mx-omnigate-initramfs-kernel.bin'))
    require(len(fits) == 1, 'one initramfs FIT required')
    fit = fits[0]
    support = kernel_support(fit)
    installer = ROOT / 'scripts/t153-install-emmc.sh'
    require(installer.is_file(), 'missing eMMC installer')
    helper = (ART / HELPER_NAME).read_bytes()
    require(helper[:7] == b'\x7fELF\x01\x01\x01' and struct.unpack_from('<H', helper, 18)[0] == 40, 'MMC helper must be ELF32 ARM')
    phoff = struct.unpack_from('<I', helper, 28)[0]
    phsize, phnum = struct.unpack_from('<HH', helper, 42)
    require(all(struct.unpack_from('<I', helper, phoff + n*phsize)[0] not in (2, 3) for n in range(phnum)), 'MMC helper must be static, with no interpreter')
    raw_sha = info['image']['sha256']
    readme = f'''T153 full-source FEL RAM installer (hardware untested)\n
This directory was loaded into RAM through BootROM FEL. No storage has been written.
The DDR4/SPL and U-Boot are source builds. This is not Tina FES or PhoenixSuit.

The compressed disk image expands to {info['image']['bytes']} bytes. The installer
streams it, checks SHA256 before writing and reads the written bytes back.
Expected raw image SHA256: {raw_sha}

First inspect the eMMC controller path/type/CID and use the actual device name:
  for d in /sys/block/mmcblk*; do echo "$d"; readlink -f "$d/device"; cat "$d/device/type" "$d/device/cid"; done

Read-only preflight (replace DEVICE and CID with values you have checked):
  sh /t153/install-emmc.sh --image /t153/disk.img.gz --sha256 {raw_sha} --device /dev/DEVICE --expect-cid CID

To install, repeat that same command with --write. This overwrites the eMMC user
area image range. The boot partitions are not changed. After successful readback,
remove other boot media and power-cycle independently to validate cold boot.
No successful hardware boot or burn is claimed by this package.
'''
    entries = [('t153', stat.S_IFDIR | 0o755, b''),
               ('t153/disk.img.gz', stat.S_IFREG | 0o644, (ART / DISK_GZIP).read_bytes()),
               ('t153/install-emmc.sh', stat.S_IFREG | 0o755, installer.read_bytes()),
               ('t153/t153-mmc-info', stat.S_IFREG | 0o755, helper),
               ('t153/disk.sha256', stat.S_IFREG | 0o644, (raw_sha + '  disk.img\n').encode()),
               ('t153/manifest.json', stat.S_IFREG | 0o644, (ART / DISK_MANIFEST).read_bytes()),
               ('t153/README.txt', stat.S_IFREG | 0o644, readme.encode())]
    cpio = newc(entries)
    require(len(cpio) < 0x4000000, 'external cpio exceeds 64 MiB RAM window')
    (ART / CPIO_NAME).write_bytes(cpio)
    text = script_text(len(cpio))
    script_source = ART / 't153-fel-installer.cmd'
    script_source.write_text(text)
    env = dict(os.environ, SOURCE_DATE_EPOCH=str(EPOCH))
    subprocess.run([str(ROOT / 'u-boot-work/build-t153-source/tools/mkimage'), '-A', 'arm', '-O', 'linux',
                    '-T', 'script', '-C', 'none', '-n', 'T153 FEL RAM installer', '-d', str(script_source),
                    str(ART / SCRIPT_NAME)], env=env, check=True)
    files = {key: descriptor(ART / name) for key, name in
             [('uboot', UBOOT_NAME), ('fit', fit.name), ('cpio', CPIO_NAME),
              ('script', SCRIPT_NAME), ('script_source', script_source.name), ('disk_manifest', DISK_MANIFEST), ('mmc_info', HELPER_NAME)]}
    manifest = {'schema_version': 1, 'board': BOARD, 'hardware_verified': False,
                'protocol': 'BootROM FEL; no FES service', 'media_written_by_host': False,
                'load_addresses': {key: hex(value) for key, value in LOADS.items()},
                'kernel_support': support, 'raw_disk_sha256': raw_sha,
                'files': files, 'cpio_entries': {name: {'mode': oct(mode), 'bytes': len(data), 'sha256': sha(data)}
                                               for name, mode, data in entries},
                'host_tool': {'path': FEL_REL, 'sha256': file_hash(ROOT / FEL_REL)},
                'limitations': ['hardware DDR/FEL/MMC/network/cold-boot validation pending',
                                'CPU0 only; PSCI/SMP not implemented', 'no USB gadget or FES protocol service']}
    (ART / MANIFEST_NAME).write_text(json.dumps(manifest, indent=2) + '\n')
    verify_bundle()
    print('PASS: packaged RAM-only FEL installer; hardware not tested')


def verify_bundle():
    manifest = json.loads((ART / MANIFEST_NAME).read_text())
    require(manifest['schema_version'] == 1 and manifest['board'] == BOARD, 'installer manifest schema/board')
    require(manifest['media_written_by_host'] is False, 'RAM-only host contract')
    require(manifest['load_addresses'] == {key: hex(value) for key, value in LOADS.items()}, 'fixed RAM layout')
    files = manifest['files']
    require(set(files) == {'uboot', 'fit', 'cpio', 'script', 'script_source', 'disk_manifest', 'mmc_info'}, 'manifest file set')
    for key, expected in [('uboot', UBOOT_NAME), ('cpio', CPIO_NAME), ('script', SCRIPT_NAME),
                          ('script_source', 't153-fel-installer.cmd'), ('disk_manifest', DISK_MANIFEST), ('mmc_info', HELPER_NAME)]:
        require(files[key]['file'] == expected, 'unexpected artifact role: ' + key)
    require(files['fit']['file'].endswith('dshanpi_t153mx-omnigate-initramfs-kernel.bin'), 'expected initramfs FIT filename')
    paths = {key: check_descriptor(desc) for key, desc in files.items()}
    info = disk_info()
    require(manifest['raw_disk_sha256'] == info['image']['sha256'], 'disk digest disagreement')
    verify_fit(paths['fit'])
    support = manifest['kernel_support']
    require(support['external_initrd'] and not support['force_builtin_initramfs'] and
            support['external_cpio_compression'] == 'none', 'kernel external initrd requirements')
    cpio = paths['cpio'].read_bytes()
    require(len(cpio) < 0x4000000, 'cpio memory window')
    unpacked = parse_cpio(cpio)
    expected_names = {'t153', 't153/disk.img.gz', 't153/install-emmc.sh', 't153/disk.sha256',
                      't153/manifest.json', 't153/README.txt', 't153/t153-mmc-info'}
    require(set(unpacked) == expected_names, 'unexpected external cpio entries')
    require(set(manifest['cpio_entries']) == expected_names, 'cpio manifest entries')
    for name, (mode, content) in unpacked.items():
        meta = manifest['cpio_entries'][name]
        require(meta == {'mode': oct(mode), 'bytes': len(content), 'sha256': sha(content)}, 'cpio metadata: ' + name)
        expected_mode = (stat.S_IFDIR | 0o755) if name == 't153' else stat.S_IFREG | (0o755 if name.endswith('.sh') or name == 't153/t153-mmc-info' else 0o644)
        require(mode == expected_mode, 'cpio exact permissions: ' + name)
    require(unpacked['t153/t153-mmc-info'][1] == paths['mmc_info'].read_bytes(), 'bundled MMC helper')
    require(unpacked['t153/disk.img.gz'][1] == (ART / DISK_GZIP).read_bytes(), 'bundled disk gzip')
    require(unpacked['t153/manifest.json'][1] == (ART / DISK_MANIFEST).read_bytes(), 'bundled disk manifest')
    require(unpacked['t153/disk.sha256'][1] == (info['image']['sha256'] + '  disk.img\n').encode(), 'bundled raw disk checksum')
    require(unpacked['t153/install-emmc.sh'][1] == (ROOT / 'scripts/t153-install-emmc.sh').read_bytes(), 'installer changed since packaging')
    expected_script = script_text(len(cpio))
    require(paths['script_source'].read_text() == expected_script, 'script source mismatch')
    verify_script(paths['script'].read_bytes(), expected_script)
    require(paths['script'].stat().st_size < 0x10000, 'script memory window')
    tool = manifest['host_tool']
    require(tool['path'] == FEL_REL and file_hash(ROOT / FEL_REL) == tool['sha256'], 'host FEL tool changed since packaging')
    # The source bundle includes eGON SPL followed by legacy U-Boot proper.
    boot = paths['uboot'].read_bytes()
    require(boot[4:12] == b'eGON.BT0' and boot[20:23] == b'SPL', 'source SPL header')
    length = struct.unpack_from('<I', boot, 16)[0]
    require(0x8000 <= length <= 0x18000 and length % 8192 == 0, 'source SPL length')
    spl = bytearray(boot[:length]); expected_sum = struct.unpack_from('<I', spl, 12)[0]
    struct.pack_into('<I', spl, 12, 0x5f0a6c39)
    require((sum(struct.unpack('<%dI' % (length//4), spl)) & 0xffffffff) == expected_sum, 'eGON checksum')
    proper = boot[length:]
    require(len(proper) >= 64, 'proper header')
    magic, hcrc, _, size, load, entry, dcrc = struct.unpack_from('>7I', proper)
    require(magic == 0x27051956 and size == len(proper)-64 and load == entry == 0x4a000000, 'proper size/address')
    header = bytearray(proper[:64]); header[4:8] = bytes(4)
    require(zlib.crc32(header) == hcrc and zlib.crc32(proper[64:]) == dcrc, 'proper CRC')
    disk_bootarea = boot[:length] + bytes(0x20000 - 0x2000 - length) + proper
    require(info['bootarea']['bytes'] == len(disk_bootarea) and info['bootarea']['sha256'] == sha(disk_bootarea), 'disk boot area differs from current source FEL U-Boot; repackage disk first')
    return manifest, paths


def selected_device(listing):
    lines = [line for line in listing.splitlines() if line.strip()]
    matches = [re.fullmatch(r'USB device (\d{3}):(\d{3})\s+Allwinner\s+T153(?:\s+.*)?', line) for line in lines]
    require(len(matches) == 1 and matches[0] is not None, 'exactly one FEL device, identified as T153, is required')
    return '%d:%d' % tuple(int(x) for x in matches[0].groups())


def check_version(version):
    require(re.fullmatch(r'AWUSBFEX soc=00001922\(T153\) [0-9a-f]{8} ver=[0-9a-f]{4} [0-9a-f]{2} [0-9a-f]{2} scratchpad=[0-9a-f]{8} [0-9a-f]{8} [0-9a-f]{8}\s*', version) is not None,
            'BootROM version is not T153 SoC 0x1922')


def boot_command(paths, device):
    return [str(ROOT / FEL_REL), '--dev', device, '--progress', 'uboot', str(paths['uboot']),
            'write', hex(LOADS['fit']), str(paths['fit']),
            'write', hex(LOADS['cpio']), str(paths['cpio']),
            'write', hex(LOADS['script']), str(paths['script'])]


def host_plan(boot=False):
    _, paths = verify_bundle()
    print('PASS: installer artifacts, compressed/raw disk hashes, cpio and script verified.')
    print('RAM load only: FEL -> source DDR4/SPL -> U-Boot -> OpenWrt installer.')
    print('No eMMC/SD write runs automatically. UART0: 115200 8N1.')
    if not boot:
        print('Offline plan; USB has not been opened. Use --boot when the board is in native FEL.')
        print(shlex.join(boot_command(paths, 'BUS:DEVICE')))
        return
    tool = str(ROOT / FEL_REL)
    listing = subprocess.run([tool, '--list'], check=True, capture_output=True, text=True, timeout=30).stdout
    device = selected_device(listing)
    version = subprocess.run([tool, '--dev', device, 'version'], check=True, capture_output=True, text=True, timeout=30).stdout
    check_version(version)
    print('Selected FEL device ' + device + ': T153 0x1922. Loading RAM only.', flush=True)
    subprocess.run(boot_command(paths, device), check=True)
    print('RAM upload completed. Observe UART; Linux boot and DDR operation still require validation.')
    print('In the RAM system, read /t153/README.txt. No persistent storage was written by this host tool.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--plan', action='store_true', help='verify and print a plan; do not open USB')
    mode.add_argument('--verify-only', action='store_true', help='verify all generated bundle files; no USB')
    mode.add_argument('--boot', action='store_true', help='load and boot RAM installer through FEL; no media write')
    args = parser.parse_args()
    if args.plan or args.boot:
        host_plan(args.boot)
    elif args.verify_only:
        verify_bundle()
        print('PASS: FEL installer bundle verified offline')
    else:
        package()


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, KeyError, EOFError, subprocess.SubprocessError) as exc:
        print('FAIL: ' + str(exc), file=sys.stderr)
        sys.exit(1)
