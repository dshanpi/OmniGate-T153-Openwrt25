#!/usr/bin/env python3
"""Build/verify the experimental source-only T153 SD/eMMC user-area image.

Only regular files inside the build tree are written. No block devices, loop
mounts, sudo, board access, or factory flash layout are involved.
"""
from pathlib import Path
import argparse
import gzip
import hashlib
import importlib.util
import json
import lzma
import os
import shutil
import stat
import struct
import subprocess
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / 'artifacts'
SECTOR = 512
DISK_ID = 0x15302512
PARTUUID = '15302512-02'
BOOT_START, BOOT_SECTORS = 8192, 65536
ROOT_START, ROOT_SECTORS = 73728, 524288
DISK_BYTES = (ROOT_START + ROOT_SECTORS) * SECTOR
SPL_OFFSET, UBOOT_OFFSET = 8192, 131072
IMAGE_NAME = 't153-openwrt25-source-disk.img'
MANIFEST_NAME = 't153-disk-manifest.json'
FIT_NAME = 'openwrt-t153.itb'
UBOOT_PREFIX = 't153-uboot-2026.07-source'
BOOT_COMMAND = '''# T153 persistent rootfs; bootdev is 0 (SD) or 1 (eMMC) in U-Boot.
setenv bootargs console=ttyS0,115200 earlycon=uart8250,mmio32,0x02600000,115200n8,24000000 root=PARTUUID=15302512-02 rootfstype=ext4 rootwait rw clk_ignore_unused
if load mmc ${bootdev}:1 ${loadaddr} openwrt-t153.itb; then
    if iminfo ${loadaddr}; then
        bootm ${loadaddr}
    fi
fi
echo T153 persistent boot failed
false
'''

spec = importlib.util.spec_from_file_location('t153_artifact_verify', ROOT / 'scripts/verify-artifacts.py')
verify = importlib.util.module_from_spec(spec)
spec.loader.exec_module(verify)


def need(condition, message):
    if not condition:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def file_sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def run(args, **kwargs):
    result = subprocess.run([str(x) for x in args], text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, **kwargs)
    need(result.returncode == 0, 'command failed: ' + ' '.join(map(str, args)) + '\n' + result.stdout)
    return result.stdout


def regular_file(path):
    path = Path(path)
    need(not path.is_symlink() and stat.S_ISREG(path.stat().st_mode), 'regular file required: ' + str(path))
    return path


def safe_output(path):
    path = Path(path)
    need(path.parent.resolve() == ART.resolve(), 'output must be directly inside artifacts')
    if path.exists() or path.is_symlink():
        regular_file(path)


def make_mbr():
    mbr = bytearray(SECTOR)
    struct.pack_into('<I', mbr, 440, DISK_ID)
    for number, start, sectors, kind in ((1, BOOT_START, BOOT_SECTORS, 0x0e),
                                         (2, ROOT_START, ROOT_SECTORS, 0x83)):
        struct.pack_into('<B3sB3sII', mbr, 446 + (number - 1) * 16,
                         0x80 if number == 1 else 0, b'\xfe\xff\xff', kind,
                         b'\xfe\xff\xff', start, sectors)
    mbr[510:512] = b'\x55\xaa'
    return bytes(mbr)


def verify_mbr(mbr, size):
    need(size == DISK_BYTES, 'wrong disk length')
    need(len(mbr) == SECTOR and mbr == make_mbr(), 'unexpected MBR/layout/signature')
    need(SPL_OFFSET < UBOOT_OFFSET < BOOT_START * SECTOR, 'boot area overlaps partitions')
    need(BOOT_START + BOOT_SECTORS == ROOT_START, 'partition boundary mismatch')


def verify_legacy(image, image_type, load=None):
    need(len(image) >= 64, 'short legacy image')
    magic, hcrc, stamp, size, address, entry, dcrc = struct.unpack_from('>7I', image)
    need(magic == 0x27051956 and len(image) == size + 64, 'legacy header magic/length')
    header = bytearray(image[:64])
    header[4:8] = b'\0' * 4
    need(zlib.crc32(header) == hcrc and zlib.crc32(image[64:]) == dcrc, 'legacy header/data CRC')
    need(image[29] == 2 and image[30] == image_type and image[31] == 0, 'legacy arch/type/compression')
    if load is not None:
        need(address == entry == load and image[28] == 17, 'U-Boot proper load/entry/OS')
    return image[64:]


def verify_bootarea(area):
    need(area[4:12] == b'eGON.BT0', 'SPL eGON magic')
    need(struct.unpack_from('<I', area)[0] == 0xea000016, 'SPL entry branch')
    checksum, length = struct.unpack_from('<II', area, 12)
    need(96 <= length <= 0x18000 and length % 8192 == 0, 'SPL length/alignment')
    need(length < UBOOT_OFFSET - SPL_OFFSET, 'SPL overlaps U-Boot proper')
    spl = bytearray(area[:length])
    need(len(spl) == length, 'truncated SPL')
    struct.pack_into('<I', spl, 12, 0x5f0a6c39)
    need(sum(struct.unpack('<%dI' % (length // 4), spl)) & 0xffffffff == checksum, 'SPL eGON checksum')
    rel = UBOOT_OFFSET - SPL_OFFSET
    need(area[length:rel] == bytes(rel - length), 'unexpected data between SPL and U-Boot')
    verify_legacy(area[rel:], 5, 0x4a000000)
    need(SPL_OFFSET + len(area) < BOOT_START * SECTOR, 'U-Boot overlaps FAT partition')
    return {'spl_bytes': length, 'proper_bytes': len(area) - rel,
            'spl_offset': SPL_OFFSET, 'proper_offset': UBOOT_OFFSET,
            'bytes': len(area), 'sha256': sha(area)}


def fit_details(data):
    nodes = verify.fdt_nodes(data)
    text = verify.text
    config = nodes['/configurations/' + text(nodes['/configurations']['default'])]
    need('ramdisk' not in config, 'persistent FIT must not select a ramdisk')
    kernel_path = '/images/' + text(config['kernel'])
    dt_path = '/images/' + text(config['fdt'])
    kernel = nodes[kernel_path]
    need(text(kernel['type']) == 'kernel' and text(kernel['arch']) == 'arm'
         and text(kernel['os']) == 'linux' and text(kernel['compression']) == 'gzip', 'FIT kernel format')
    need(kernel['load'] == kernel['entry'] == struct.pack('>I', 0x48008000), 'FIT load/entry')
    for path, props in nodes.items():
        need(not (path.startswith('/images/') and props.get('type') == b'ramdisk\0'), 'FIT ramdisk image found')
    for path in (kernel_path, dt_path):
        payload = nodes[path]['data']
        algorithms = set()
        for child, props in nodes.items():
            if child.startswith(path + '/hash'):
                algorithm = text(props['algo'])
                actual = struct.pack('>I', zlib.crc32(payload)) if algorithm == 'crc32' else hashlib.new(algorithm, payload).digest()
                need(actual == props['value'], 'FIT hash mismatch: ' + child)
                algorithms.add(algorithm)
        need({'crc32', 'sha1'} <= algorithms, 'FIT requires crc32 and sha1')
    board = verify.fdt_nodes(nodes[dt_path]['data'])
    need(b'dshanpi,t153mx-omnigate\0' in board['/']['compatible'], 'FIT wrong board')
    need(board['/chosen']['stdout-path'] == b'serial0:115200n8\0', 'kernel console changed')
    zimage = gzip.decompress(kernel['data'])
    need(struct.unpack_from('<I', zimage, 0x24)[0] == 0x016f2818, 'ARM zImage magic')
    return zimage, {'bytes': len(data), 'sha256': sha(data), 'zimage_bytes': len(zimage),
                    'zimage_sha256': sha(zimage), 'load': '0x48008000', 'console': 'ttyS0,115200'}


def verify_persistent_kernel(fit, build):
    zimage, info = fit_details(fit)
    need(zimage == (build / 'zImage').read_bytes(), 'FIT does not match ordinary zImage')
    need(zimage != (build / 'zImage-initramfs').read_bytes(), 'rescue initramfs kernel used for disk boot')
    # Inspect the actual compressed kernel in zImage, not .config left by the
    # later initramfs build. XZ magic also occurs in decompressor code.
    streams = []
    offset = -1
    while True:
        offset = zimage.find(b'\xfd7zXZ\x00', offset + 1)
        if offset < 0:
            break
        try:
            dec = lzma.LZMADecompressor()
            raw = dec.decompress(zimage[offset:], max_length=64 * 1024 * 1024)
            if dec.eof and b'Linux version 6.12.113' in raw:
                streams.append(raw)
        except lzma.LZMAError:
            pass
    need(len(streams) == 1, 'cannot uniquely decode normal zImage XZ kernel')
    raw = streams[0]
    names = {'_text', '__initramfs_start', '__initramfs_size'}
    symbols = {}
    for line in run(['nm', build / 'vmlinux.debug']).splitlines():
        fields = line.split()
        if len(fields) == 3 and fields[2] in names:
            symbols[fields[2]] = int(fields[0], 16)
    need(symbols.keys() == names, 'missing ordinary kernel initramfs symbols')
    begin = symbols['__initramfs_start'] - symbols['_text']
    size_at = symbols['__initramfs_size'] - symbols['_text']
    need(0 <= begin < size_at <= len(raw) - 4, 'initramfs symbol bounds')
    size = struct.unpack_from('<I', raw, size_at)[0]
    need(0 < size <= 4096 and begin + size <= size_at, 'ordinary kernel contains a populated initramfs')
    entries = verify.cpio_entries(raw[begin:begin + size])
    need(set(entries) <= {'dev', 'dev/console', 'root'} and 'init' not in entries,
         'ordinary kernel includes /init or a populated rootfs')
    kconfig = (build / 'linux-6.12.113/.config').read_text()
    for name in ('EXT4_FS', 'MMC', 'MMC_BLOCK', 'MMC_SUNXI', 'SERIAL_8250_CONSOLE'):
        need('CONFIG_' + name + '=y\n' in kconfig, 'persistent boot needs built-in ' + name)
    info.update({'has_init': False, 'populated_initramfs': False,
                 'builtin_cpio_bytes': size, 'builtin_cpio_entries': sorted(entries),
                 'decoded_kernel_bytes': len(raw), 'decoded_kernel_sha256': sha(raw)})
    return info


def copy_region(source, output, offset, length):
    with Path(source).open('rb') as src, Path(output).open('r+b') as dst:
        dst.seek(offset)
        copied = 0
        while chunk := src.read(min(1024 * 1024, length - copied)):
            dst.write(chunk)
            copied += len(chunk)
        need(copied == length and not src.read(1), 'partition source length mismatch')


def partition_to_file(disk, path, offset, length):
    with Path(disk).open('rb') as src, Path(path).open('wb') as dst:
        src.seek(offset)
        remaining = length
        while remaining:
            chunk = src.read(min(1024 * 1024, remaining))
            need(chunk, 'truncated partition')
            dst.write(chunk)
            remaining -= len(chunk)


def verify_disk(path, manifest, temp):
    regular_file(path)
    need(path.stat().st_size == manifest['image']['bytes'] == DISK_BYTES, 'disk image size')
    need(file_sha(path) == manifest['image']['sha256'], 'disk SHA256')
    with path.open('rb') as stream:
        verify_mbr(stream.read(SECTOR), path.stat().st_size)
        stream.seek(SPL_OFFSET)
        bootarea = stream.read(manifest['bootarea']['bytes'])
    need(verify_bootarea(bootarea) == manifest['bootarea'], 'boot area manifest mismatch')
    fat = temp / 'verify-fat.img'
    rootfs = temp / 'verify-rootfs.ext4'
    partition_to_file(path, fat, BOOT_START * SECTOR, BOOT_SECTORS * SECTOR)
    partition_to_file(path, rootfs, ROOT_START * SECTOR, ROOT_SECTORS * SECTOR)
    with fat.open('rb') as stream:
        bpb = stream.read(512)
    need(bpb[54:62] == b'FAT16   ' and bpb[510:512] == b'\x55\xaa', 'FAT16 boot sector')
    need(struct.unpack_from('<H', bpb, 11)[0] == SECTOR, 'FAT sector size')
    need(struct.unpack_from('<I', bpb, 28)[0] == BOOT_START, 'FAT hidden sectors')
    run(['fsck.fat', '-n', fat])
    for name in (FIT_NAME, 'boot.scr'):
        extracted = temp / ('verified-' + name)
        run(['mcopy', '-i', fat, '::/' + name, extracted])
        need(file_sha(extracted) == manifest['boot_files'][name]['sha256'], 'FAT content mismatch: ' + name)
        need(extracted.stat().st_size == manifest['boot_files'][name]['bytes'], 'FAT file size: ' + name)
    builds = list((ROOT / 'openwrt/build_dir').glob('target-*/linux-sunxi_cortexa7'))
    need(len(builds) == 1, 'expected one cortexa7 build directory')
    actual_kernel = verify_persistent_kernel((temp / ('verified-' + FIT_NAME)).read_bytes(), builds[0])
    need(actual_kernel == manifest['kernel'], 'persistent kernel manifest mismatch')
    script = verify_legacy((temp / 'verified-boot.scr').read_bytes(), 6)
    command = BOOT_COMMAND.encode()
    need(script == struct.pack('>II', len(command), 0) + command, 'legacy script command payload')
    need(file_sha(rootfs) == manifest['rootfs']['sha256'], 'rootfs partition mismatch')
    run(['e2fsck', '-f', '-n', rootfs])
    info = run(['debugfs', '-R', 'stat /sbin/init', rootfs])
    need('Inode:' in info, 'persistent rootfs /sbin/init missing')
    return ['MBR signature/partition geometry', 'eGON checksum/branch',
            'U-Boot legacy header/data CRC', 'FAT16 fsck and file hashes',
            'FIT payload CRC32/SHA1', 'boot script CRC/root PARTUUID',
            'ext4 e2fsck -fn and /sbin/init', 'whole image SHA256']


def main():
    global IMAGE_NAME, MANIFEST_NAME, UBOOT_PREFIX
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify-only', action='store_true', help='verify existing disk, gzip and manifest without rebuilding')
    parser.add_argument('--profile', choices=['512m', '1g'], default='512m', help='source DDR profile; 1g uses separate output names')
    args = parser.parse_args()
    if args.profile == '1g':
        IMAGE_NAME = 't153-openwrt25-source-1g-disk.img'
        MANIFEST_NAME = 't153-disk-1g-manifest.json'
        UBOOT_PREFIX = 't153-uboot-2026.07-source-1g'
        validation = json.loads((ART / 'uboot-source-1g-validation.json').read_text())
        need(validation['ddr_size_mib'] == 1024 and validation['linux_usable_mib'] == 384, 'wrong 1GiB profile')
        need(validation['vendor_ddr_object_linked'] is False, 'vendor DDR object in source profile')
        for name, info in validation['outputs'].items():
            path = regular_file(ART / name)
            need(path.stat().st_size == info['bytes'] and file_sha(path) == info['sha256'], '1GiB U-Boot artifact mismatch: ' + name)
    ART.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.disk-package-', dir=ROOT) as work:
        temp = Path(work)
        image_path = ART / IMAGE_NAME
        gzip_path = ART / (IMAGE_NAME + '.gz')
        manifest_path = ART / MANIFEST_NAME
        for path in (image_path, gzip_path, manifest_path):
            safe_output(path)
        if args.verify_only:
            manifest = json.loads(regular_file(manifest_path).read_text())
            need(manifest['image']['file'] == IMAGE_NAME, 'wrong image profile')
            if args.profile == '1g':
                need(manifest['physical_dram_mib'] == 1024 and manifest['linux_usable_mib'] == 384, 'wrong memory profile')
                need(manifest['bootarea']['sha256'] == file_sha(ART / (UBOOT_PREFIX + '-sd-bootarea.bin')), 'wrong 1GiB boot area')
            verify_disk(image_path, manifest, temp)
            need(file_sha(gzip_path) == manifest['compressed_image']['sha256'], 'gzip SHA256')
            digest = hashlib.sha256()
            total = 0
            with gzip.open(gzip_path, 'rb') as stream:
                while chunk := stream.read(1024 * 1024):
                    total += len(chunk)
                    digest.update(chunk)
            need(total == DISK_BYTES and digest.hexdigest() == manifest['image']['sha256'], 'gzip decompressed image mismatch')
            print('PASS: existing disk layout, SPL/U-Boot/FIT, boot script, filesystems, raw/gzip hashes')
            return
        builds = list((ROOT / 'openwrt/build_dir').glob('target-*/linux-sunxi_cortexa7'))
        need(len(builds) == 1, 'expected one cortexa7 build directory')
        build = builds[0]
        fit = (build / 'dshanpi_t153mx-omnigate-kernel.bin').read_bytes()
        fit_info = verify_persistent_kernel(fit, build)
        area = (ART / (UBOOT_PREFIX + '-sd-bootarea.bin')).read_bytes()
        area_info = verify_bootarea(area)
        need(area[:area_info['spl_bytes']] == (ART / (UBOOT_PREFIX + '-spl.bin')).read_bytes(), 'SPL artifacts disagree')
        need(area[UBOOT_OFFSET - SPL_OFFSET:] == (ART / (UBOOT_PREFIX + '.img')).read_bytes(), 'U-Boot artifacts disagree')
        root_archives = list((ROOT / 'openwrt/bin/targets/sunxi/cortexa7').glob('*dshanpi_t153mx-omnigate-ext4-rootfs.ext4.gz'))
        need(len(root_archives) == 1, 'expected one ext4 rootfs')
        rootfs = temp / 'rootfs.ext4'
        with gzip.open(root_archives[0], 'rb') as source, rootfs.open('wb') as output:
            shutil.copyfileobj(source, output)
        need(rootfs.stat().st_size == ROOT_SECTORS * SECTOR, 'rootfs must be exactly 256 MiB')
        original_rootfs_sha = file_sha(rootfs)
        # OpenWrt make_ext4fs may leave unset inode-bitmap padding. Repair only
        # e2fsck's automatically safe metadata classes on this temporary copy;
        # source artifacts are never opened for writing. Preen stops on errors
        # requiring manual decisions, and the final image must pass read-only.
        fsck = subprocess.run(['e2fsck', '-f', '-p', str(rootfs)], text=True,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              env=dict(os.environ, E2FSPROGS_FAKE_TIME='1793712000'))
        need(fsck.returncode in (0, 1), 'ext4 normalization failed: ' + fsck.stdout)
        run(['e2fsck', '-f', '-n', rootfs])
        fit_file = temp / FIT_NAME
        fit_file.write_bytes(fit)
        command_file, script_file = temp / 'boot.cmd', temp / 'boot.scr'
        command_file.write_text(BOOT_COMMAND)
        env = dict(os.environ, SOURCE_DATE_EPOCH='315532800')
        run(['mkimage', '-A', 'arm', '-O', 'linux', '-T', 'script', '-C', 'none',
             '-n', 'T153 OpenWrt persistent boot', '-d', command_file, script_file], env=env)
        for path in (fit_file, script_file):
            os.utime(path, (315532800, 315532800))
        fat = temp / 'boot.fat'
        with fat.open('wb') as stream:
            stream.truncate(BOOT_SECTORS * SECTOR)
        run(['mkfs.fat', '--invariant', '-F', '16', '-h', str(BOOT_START),
             '-n', 'T153BOOT', '-i', '%08x' % DISK_ID, fat])
        for path in (fit_file, script_file):
            run(['mcopy', '-m', '-i', fat, path, '::/' + path.name], env=env)
        candidate = temp / IMAGE_NAME
        with candidate.open('wb') as stream:
            stream.truncate(DISK_BYTES)
            stream.write(make_mbr())
            stream.seek(SPL_OFFSET)
            stream.write(area)
        copy_region(fat, candidate, BOOT_START * SECTOR, BOOT_SECTORS * SECTOR)
        copy_region(rootfs, candidate, ROOT_START * SECTOR, ROOT_SECTORS * SECTOR)
        manifest = {
            'schema_version': 1, 'board': 'dshanpi,t153mx-omnigate',
            'status': 'software image checks passed; hardware NOT tested',
            'hardware_verified': False, 'target_media': ['SD card', 'eMMC user area'],
            'emmc_bootrom_user_offsets_verified': False,
            'emmc_hardware_boot_configuration_changed': False,
            'factory_layout_compatible': False, 'cpu_count': 1,
            'sector_bytes': SECTOR, 'disk_signature': '%08x' % DISK_ID, 'root_partuuid': PARTUUID,
            'image': {'file': IMAGE_NAME, 'bytes': DISK_BYTES, 'sha256': file_sha(candidate)},
            'partitions': [
                {'number': 1, 'type': '0x0e', 'filesystem': 'fat16', 'label': 'T153BOOT',
                 'start_lba': BOOT_START, 'sectors': BOOT_SECTORS, 'partuuid': '15302512-01'},
                {'number': 2, 'type': '0x83', 'filesystem': 'ext4', 'start_lba': ROOT_START,
                 'sectors': ROOT_SECTORS, 'partuuid': PARTUUID}],
            'bootarea': area_info, 'kernel': fit_info,
            'boot_files': {path.name: {'bytes': path.stat().st_size, 'sha256': file_sha(path)}
                           for path in (fit_file, script_file)},
            'boot_script': BOOT_COMMAND,
            'rootfs': {'bytes': rootfs.stat().st_size, 'sha256': file_sha(rootfs),
                       'source_gzip': root_archives[0].name,
                       'source_gzip_sha256': file_sha(root_archives[0]),
                       'source_uncompressed_sha256': original_rootfs_sha,
                       'metadata_normalization': fsck.stdout.strip()},
            'limitations': ['hardware bring-up pending', 'CPU0 only; PSCI/SMP not implemented',
                            'writing overwrites the factory partition table and user-area data',
                            'eMMC BootROM user-area offsets remain hardware-unverified; installer refuses enabled boot0/boot1',
                            'remove SD card before testing eMMC boot; cloned disks share PARTUUID'],
        }
        if args.profile == '1g':
            manifest.update({'source_profile': 'dshanpi_t153_source_1g_defconfig', 'physical_dram_mib': 1024, 'linux_usable_mib': 384, 'ddr_mhz': [480, 933], 'vendor_ddr_object_linked': False})
        manifest['checks'] = verify_disk(candidate, manifest, temp)
        compressed = temp / (IMAGE_NAME + '.gz')
        with candidate.open('rb') as source, compressed.open('wb') as output:
            with gzip.GzipFile(fileobj=output, filename='', mode='wb', compresslevel=9, mtime=0) as zipfile:
                shutil.copyfileobj(source, zipfile)
        manifest['compressed_image'] = {'file': compressed.name, 'bytes': compressed.stat().st_size,
                                        'sha256': file_sha(compressed)}
        # Verify the actual gzip stream before publishing all three outputs.
        digest = hashlib.sha256()
        total = 0
        with gzip.open(compressed, 'rb') as stream:
            while chunk := stream.read(1024 * 1024):
                total += len(chunk)
                digest.update(chunk)
        need(total == DISK_BYTES and digest.hexdigest() == manifest['image']['sha256'], 'compressed image roundtrip')
        os.replace(candidate, image_path)
        os.replace(compressed, gzip_path)
        draft = temp / MANIFEST_NAME
        draft.write_text(json.dumps(manifest, indent=2) + '\n')
        os.replace(draft, manifest_path)
        print(json.dumps({'image': manifest['image'], 'compressed_image': manifest['compressed_image'],
                          'checks': manifest['checks'], 'hardware_verified': False}, indent=2))
        print('PASS: source-only disk image packaged; no device accessed')


if __name__ == '__main__':
    main()
