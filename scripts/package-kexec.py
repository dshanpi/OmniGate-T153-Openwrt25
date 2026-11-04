#!/usr/bin/env python3
"""Package the ordinary OpenWrt initramfs for standard ARM kexec; never boot it.

Only local regular artifact files are written. No initramfs scripts are added,
no persistent media is touched, and the kexec executable is never invoked here.
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
import struct
import subprocess
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('verify', ROOT / 'scripts/verify-artifacts.py')
verify = importlib.util.module_from_spec(spec)
spec.loader.exec_module(verify)
need = verify.require
BASE = 0x48000000
SIZE = 0x18000000
END = BASE + SIZE
BOOTARGS = ('console=ttyS0,115200 '
            'earlycon=uart8250,mmio32,0x02600000,115200n8,24000000 '
            'clk_ignore_unused')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def only(paths, description):
    paths = list(paths)
    need(len(paths) == 1, description + ': expected one file')
    need(paths[0].is_file() and not paths[0].is_symlink(), description + ': regular file')
    return paths[0]


def fit_payload(fit):
    nodes = verify.fdt_nodes(fit)
    default = verify.text(nodes['/configurations']['default'])
    config = nodes['/configurations/' + default]
    names = ['/images/' + verify.text(config[key]) for key in ('kernel', 'fdt')]
    for path in names:
        payload = nodes[path]['data']
        count = 0
        for child, props in nodes.items():
            if child.startswith(path + '/hash'):
                algorithm = verify.text(props['algo'])
                actual = (struct.pack('>I', zlib.crc32(payload) & 0xffffffff)
                          if algorithm == 'crc32' else hashlib.new(algorithm, payload).digest())
                need(actual == props['value'], 'FIT hash ' + child)
                count += 1
        need(count >= 2, 'FIT hash coverage ' + path)
    kernel = nodes[names[0]]
    need(verify.text(kernel['arch']) == 'arm' and verify.text(kernel['os']) == 'linux', 'ARM Linux FIT')
    need(verify.text(kernel['compression']) == 'gzip', 'FIT wrapper gzip')
    need(kernel['load'] == kernel['entry'] == struct.pack('>I', BASE + 0x8000), 'FIT kernel load/entry')
    zimage = gzip.decompress(kernel['data'])
    need(len(zimage) > 0x30 and struct.unpack_from('<I', zimage, 0x24)[0] == 0x016f2818, 'ARM zImage')
    return zimage, nodes[names[1]]['data']


def patched_dtb(original, output):
    nodes = verify.fdt_nodes(original)
    need(b'dshanpi,t153mx-omnigate\0' in nodes['/']['compatible'], 'board compatible')
    need(nodes['/soc/watchdog@2050000']['compatible'] == b'allwinner,sun8i-t153-wdt\0', 'new watchdog driver')
    mode = nodes['/soc/reboot-mode@7090108']
    need(mode['reg'] == struct.pack('>II', 0x07090108, 4), 'RTC data[2] resource')
    need(mode['mode-normal'] == struct.pack('>I', 0), 'normal reboot clears request')
    need(mode['mode-efex'] == struct.pack('>I', 0x5aa5a55a), 'FEL reboot magic')
    need(nodes['/soc/ethernet@4510000']['phy-mode'] == b'rgmii-rxid\0', 'validated WAN RX delay')
    output.write_bytes(original)
    tool = shutil.which('fdtput')
    need(tool is not None, 'host fdtput is required')

    def edit(*args):
        subprocess.run([tool, *args[:1], str(output), *args[1:]], check=True, capture_output=True)

    edit('-r', '/psci')
    cpus = [path for path in nodes if path.startswith('/cpus/cpu@') and path.count('/') == 2]
    need(len(cpus) == 4, 'exactly four source CPU nodes')
    for path in cpus:
        reg = struct.unpack('>I', nodes[path]['reg'])[0]
        need(reg in range(4), 'CPU index')
        if reg == 0:
            edit('-d', path, 'enable-method')
            edit('-ts', path, 'status', 'okay')
        else:
            edit('-ts', path, 'status', 'disabled')
    memories = [path for path, props in nodes.items() if props.get('device_type') == b'memory\0']
    need(len(memories) == 1, 'single memory bank')
    edit('-r', memories[0])
    edit('-c', '/memory@48000000')
    edit('-ts', '/memory@48000000', 'device_type', 'memory')
    edit('-tx', '/memory@48000000', 'reg', f'{BASE:x}', f'{SIZE:x}')
    edit('-ts', '/chosen', 'bootargs', BOOTARGS)
    # Never inherit an old external ramdisk or an artificial usable-memory cap.
    for prop in ('linux,initrd-start', 'linux,initrd-end', 'linux,usable-memory-range', 'kaslr-seed'):
        if prop in nodes['/chosen']:
            edit('-d', '/chosen', prop)
    result = verify.fdt_nodes(output.read_bytes())
    need('/psci' not in result, 'no nonexistent PSCI service')
    for path in cpus:
        is_cpu0 = struct.unpack('>I', result[path]['reg'])[0] == 0
        need(result[path]['status'] == (b'okay\0' if is_cpu0 else b'disabled\0'), 'CPU status')
        need(not is_cpu0 or 'enable-method' not in result[path], 'CPU0 has no PSCI enable-method')
    need(result['/memory@48000000']['reg'] == struct.pack('>II', BASE, SIZE), '384MiB safe memory window')
    need(result['/chosen']['bootargs'] == BOOTARGS.encode() + b'\0', 'initramfs boot arguments')
    need(not any(arg.startswith(('root=', 'rootfstype=', 'rootwait', 'rootdelay=', 'init=')) for arg in BOOTARGS.split()), 'no persistent root override')
    for path in ('/soc/watchdog@2050000', '/soc/reboot-mode@7090108', '/soc/ethernet@4510000'):
        need(result[path] == nodes[path], 'preserved new hardware node ' + path)
    return result


def zimage_space(zimage):
    need(struct.unpack_from('<I', zimage, 0x34)[0] == 0x45454545, 'zImage extension header')
    offset = struct.unpack_from('<I', zimage, 0x38)[0]
    while offset + 8 < len(zimage):
        words, tag = struct.unpack_from('<II', zimage, offset)
        if not words:
            break
        size = words * 4
        need(size >= 8 and offset + size < len(zimage), 'zImage extension bounds')
        if tag == 0x5a534c4b:
            need(size >= 16, 'zImage size tag length')
            pointer, bss = struct.unpack_from('<II', zimage, offset + 8)
            need(pointer + 4 <= len(zimage), 'zImage edata pointer')
            edata = struct.unpack_from('<I', zimage, pointer)[0]
            minimum = max(edata + bss, edata + len(zimage) + 0x11000)
            need(minimum <= 0x08000000, '128MiB image-size covers kexec extension requirement')
            return {'edata_bytes': edata, 'bss_bytes': bss, 'minimum_image_size': minimum}
        offset += size
    raise SystemExit('FAIL: zImage kernel-size extension not found')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--openwrt', type=Path, default=ROOT / 'openwrt')
    parser.add_argument('--output', type=Path, default=ROOT / 'artifacts/kexec-native-reboot')
    args = parser.parse_args()
    ow = args.openwrt.resolve()
    target = only(ow.glob('build_dir/target-*/linux-sunxi_cortexa7/linux-6.12.113/.config'), 'kernel config').parent
    build = target.parent
    fit = only((ow / 'bin/targets/sunxi/cortexa7').glob('*dshanpi_t153mx-omnigate-initramfs-kernel.bin'), 'initramfs FIT')
    zimage, dtb = fit_payload(fit.read_bytes())
    need(zimage == (build / 'zImage-initramfs').read_bytes(), 'FIT is actual initramfs zImage')
    # OpenWrt's flat vmlinux-initramfs has ELF notes removed; zImage uses
    # arch/arm/boot/Image, whose note bytes are part of the actual payload.
    raw = (target / 'arch/arm/boot/Image').read_bytes()
    found_raw = False
    offset = -1
    while True:
        offset = zimage.find(b'\xfd7zXZ\x00', offset + 1)
        if offset < 0:
            break
        try:
            decoder = lzma.LZMADecompressor()
            payload = decoder.decompress(zimage[offset:], max_length=128 * 1024 * 1024)
        except lzma.LZMAError:
            continue
        if decoder.eof and payload == raw:
            found_raw = True
            break
    need(found_raw, 'raw initramfs kernel matches actual zImage compressed payload')
    for marker in (b'allwinner,sun8i-t153-reboot-mode', b'allwinner,sun8i-t153-wdt', b'RTC reboot request %08x did not latch'):
        need(marker in raw, 'new reboot driver linked into kernel: ' + marker.decode())
    # Packed sunxi_wdt_reg (7 u8, padding, 3 u32, bool, padding).
    # The final 1 is T153's single-shot restart flag, absent from old kernels.
    variant = bytes.fromhex('0c10141804030100 0000aa16 ff0f0000 3f000000 01000000')
    need(raw.count(variant) == 1, 'actual kernel contains T153 single-shot watchdog variant')
    config = (target / '.config').read_text()
    for symbol in ('KEXEC', 'SUNXI_WATCHDOG', 'REBOOT_MODE', 'SUN8I_T153_REBOOT_MODE'):
        need('\nCONFIG_' + symbol + '=y\n' in '\n' + config, 'config ' + symbol)
    need('CONFIG_CMDLINE=""' in config and 'CONFIG_CMDLINE_FORCE=y' not in config, 'no built-in root/commandline override')
    archive = only((path for path in (target / 'usr').glob('initramfs_data.cpio*') if path.suffix in ('.cpio', '.gz', '.xz', '.lzma')), 'embedded initramfs cpio')
    packed = archive.read_bytes()
    payload = gzip.decompress(packed) if archive.suffix == '.gz' else lzma.decompress(packed) if archive.suffix in ('.xz', '.lzma') else packed
    need((target / 'usr/initramfs_inc_data').read_bytes() in raw, 'cpio is embedded in this kernel')
    files = verify.cpio_entries(payload)
    need(b'export INITRAMFS=1' in files.get('init', b''), 'ordinary OpenWrt initramfs entry')
    # Compare the final stripped package binaries, not source strings alone.
    package_files = {}
    for path, pattern in (
        ('bin/busybox', 'busybox-default/busybox-*/ipkg-*/busybox/bin/busybox'),
        ('sbin/procd', 'procd-default/procd-*/ipkg-*/procd/sbin/procd')):
        candidate = only(build.parent.glob(pattern), 'new package ' + path)
        need(files.get(path) == candidate.read_bytes(), 'cpio contains rebuilt ' + path)
        package_files[path] = {'sha256': sha(files[path]), 'source': str(candidate.relative_to(ROOT))}
    need(b'{"command":"%s"}' in files['bin/busybox'], 'actual BusyBox mode forwarding implementation')
    # Keep a generous decompression/relocation envelope below 0x50000000.
    envelope_end = BASE + 0x8000 + len(raw) + len(zimage) + 0x800000
    need(envelope_end < 0x50000000, '128MiB decompression/relocation envelope')
    tag_space = zimage_space(zimage)
    tool_dir = ROOT / 'artifacts/kexec-tools'
    tool_meta = json.loads((tool_dir / 'manifest.json').read_text())
    tool_lock = json.loads((ROOT / 'configs/kexec-tools.lock.json').read_text())
    tool_data = (tool_dir / 'kexec').read_bytes()
    need(sha(tool_data) == tool_meta['binary']['sha256'], 'locked kexec executable hash')
    for key in ('version', 'source_url', 'source_sha256', 'openwrt_commit'):
        need(tool_meta[key] == tool_lock[key], 'kexec lock ' + key)
    need(tool_data[:7] == b'\x7fELF\x01\x01\x01' and struct.unpack_from('<H', tool_data, 18)[0] == 40, 'kexec ARM32 ELF')
    phoff = struct.unpack_from('<I', tool_data, 28)[0]
    phsize, phnum = struct.unpack_from('<HH', tool_data, 42)
    need(all(struct.unpack_from('<I', tool_data, phoff + i * phsize)[0] not in (2, 3) for i in range(phnum)), 'static kexec has no dynamic/interpreter segment')
    output = args.output.resolve()
    artifacts = (ROOT / 'artifacts').resolve()
    need(output.is_relative_to(artifacts) and output != artifacts, 'output must be a subdirectory of artifacts')
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.kexec-package-', dir=output) as temp:
        stage = Path(temp)
        (stage / 'kexec').write_bytes(tool_data)
        (stage / 'kexec').chmod(0o755)
        (stage / 'COPYING.kexec-tools').write_bytes((tool_dir / 'COPYING').read_bytes())
        (stage / 'kexec-tool-manifest.json').write_bytes((tool_dir / 'manifest.json').read_bytes())
        (stage / 'openwrt-initramfs.zImage').write_bytes(zimage)
        patched_dtb(dtb, stage / 't153-cpu0-384m.dtb')
        manifest = {
            'status': 'software verified; kexec and hardware reboot not yet tested',
            'source_fit': str(fit.relative_to(ROOT)), 'source_fit_sha256': sha(fit.read_bytes()),
            'rootfs': 'unaltered ordinary OpenWrt built-in initramfs; no installer scripts',
            'embedded_userspace': package_files,
            'memory': {'base': hex(BASE), 'size': hex(SIZE), 'end_exclusive': hex(END),
                       'mem_min': hex(BASE), 'mem_max_inclusive': hex(END - 1),
                       'zimage_entry': hex(BASE + 0x8000),
                       'uncompressed_kernel_bytes': len(raw),
                       'checked_decompression_envelope_end': hex(envelope_end)},
            'cpus': 'CPU0 only, no PSCI', 'bootargs': BOOTARGS,
            'single_shot_watchdog_verified': True, 'raw_kernel_sha256': sha(raw),
            'zimage_extension': tag_space,
            'load_argv': ['./kexec', '--debug', '--load', '--mem-min=0x48000000', '--mem-max=0x5fffffff',
                          '--image-size=0x08000000', '--dtb=t153-cpu0-384m.dtb', 'openwrt-initramfs.zImage'],
            'dtb_placement_minimum': '0x50009000; actual page is chosen by standard kexec locate_hole',
            'kexec_tool': {'version': tool_meta['version'], 'sha256': sha(tool_data), 'source_lock': 'configs/kexec-tools.lock.json'},
            'files': {name: {'bytes': (stage / name).stat().st_size, 'sha256': sha((stage / name).read_bytes())}
                      for name in ('openwrt-initramfs.zImage', 't153-cpu0-384m.dtb', 'kexec', 'COPYING.kexec-tools', 'kexec-tool-manifest.json')},
            'execution': 'Load only with the standard kexec tool and these memory bounds; kernel kexec allocates staging pages. Never copy directly to physical addresses. Actual execution is a separate authorized step.'
        }
        (stage / 'LOAD-ONLY.txt').write_text(
            'Standard ARM kexec package; no firmware installer or replacement init.\n'
            'Copy files to RAM, verify SHA256SUMS, and inspect the normal tool output.\n\n'
            'Load only (does not execute the new kernel):\n'
            './kexec --debug --load --mem-min=0x48000000 --mem-max=0x5fffffff '
            '--image-size=0x08000000 --dtb=t153-cpu0-384m.dtb openwrt-initramfs.zImage\n\n'
            'Expected entry: 0x48008000. DTB must be inside the safe RAM bank and '
            'at/above 0x50009000. The loader reserves 128MiB for decompression.\n'
            'No --append/--command-line: keep the DTB ordinary initramfs bootargs.\n'
            'The kexec syscall allocates staging pages; never manually copy into '
            'physical addresses or live kernel memory.\n'
            'Execution is a separate step through the supported tool path, after '
            'normal filesystem sync/quiescing. Loading alone does not shut down '
            'the currently mounted root filesystem. Do not bypass a tool denial.\n')
        (stage / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
        (stage / 'SHA256SUMS').write_text(''.join(f"{info['sha256']}  {name}\n" for name, info in manifest['files'].items()))
        for path in stage.iterdir():
            dest = output / path.name
            need(not dest.is_symlink(), 'refuse output symlink')
            os.replace(path, dest)
    (output / 'STALE-REBUILD-REQUIRED.txt').unlink(missing_ok=True)
    print('PASS: ordinary initramfs, rebuilt userspace, linked reboot drivers, CPU0/no-PSCI/384MiB DTB; ' + str(output))


if __name__ == '__main__':
    main()
