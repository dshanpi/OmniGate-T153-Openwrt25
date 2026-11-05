#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Validate this source build and combine its eGON SPL and legacy U-Boot image.

Runs inside the native OpenWrt boot package. It requires only Python's standard
library, this package's build products and the selected cross-binutils. It does
not read an SDK, another source tree, a device or a prebuilt bootloader.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import zlib


def need(ok, message):
    if not ok:
        raise ValueError(message)


def symbols(path):
    result = {}
    for line in path.read_text().splitlines():
        fields = line.split()
        if len(fields) >= 2 and re.fullmatch(r'[0-9a-fA-F]+', fields[0]):
            result[fields[-1]] = int(fields[0], 16)
    return result


def elf(path, entry):
    data = path.read_bytes()
    need(data[:7] == b'\x7fELF\x01\x01\x01', 'ELF32 little endian: ' + path.name)
    need(struct.unpack_from('<HHI', data, 16) == (2, 40, 1), 'ARM executable ELF')
    need(struct.unpack_from('<I', data, 24)[0] == entry, 'ELF entry: ' + path.name)
    return data


def fdt_nodes(data):
    need(len(data) >= 40, 'FDT header')
    magic, total, off, strings, _rsv, version = struct.unpack_from('>6I', data)
    need(magic == 0xd00dfeed and total <= len(data) and version >= 16, 'FDT format')
    need(40 <= off < total and 40 <= strings < total, 'FDT offsets')
    stack, nodes = [], set()
    while off + 4 <= total:
        token = struct.unpack_from('>I', data, off)[0]
        off += 4
        if token == 1:
            end = data.index(0, off, total)
            stack.append(data[off:end].decode())
            nodes.add('/' + '/'.join(stack[1:]))
            off = (end + 4) & ~3
        elif token == 2:
            need(bool(stack), 'balanced FDT nodes')
            stack.pop()
        elif token == 3:
            length = struct.unpack_from('>I', data, off)[0]
            off = (off + 8 + length + 3) & ~3
        elif token == 4:
            continue
        elif token == 9:
            need(not stack, 'closed FDT nodes')
            return nodes
        else:
            raise ValueError('unknown FDT token')
    raise ValueError('unterminated FDT')


def secure_monitor(build, cross):
    data = elf(build / 'u-boot', 0x4a000000)
    shoff = struct.unpack_from('<I', data, 32)[0]
    stride, count, names = struct.unpack_from('<HHH', data, 46)
    headers = [struct.unpack_from('<10I', data, shoff + i * stride) for i in range(count)]
    strings = headers[names]
    strings = data[strings[4]:strings[4] + strings[5]]
    sections = {strings[h[0]:].split(b'\0', 1)[0].decode(): h for h in headers}
    ranges = {}
    for name in ('.secure_text', '.secure_data', '.secure_stack'):
        header = sections[name]
        start, size = header[3], header[5]
        need(size > 0 and 0x50000 <= start < start + size <= 0x58000,
             name + ' must remain in reserved SRAM')
        ranges[name] = {'start': start, 'bytes': size}
    text = sections['.secure_text']
    text_end = text[3] + text[5]
    need(text[3] == 0x50000 and ranges['.secure_stack']['bytes'] >= 4096,
         'monitor entry and four per-core stacks')
    syms = symbols(build / 'u-boot.sym')
    for name in ('_nonsec_init', '_do_nonsec_entry', '_psci_vectors', 'psci_cpu_on',
                 'psci_cpu_entry', 'psci_save', 'psci_arch_cpu_entry',
                 't153_psci_cpu_entry', 'psci_get_cpu_id', 'psci_stack_setup'):
        need(0x50000 <= syms[name] < text_end, 'resident monitor symbol: ' + name)
    need(0x4a000000 <= syms['psci_board_init'] < 0x4a200000, 'boot-only PSCI initializer')
    table = syms['_psci_table'] - text[3] + text[4]
    for i in range(32):
        ident, function = struct.unpack_from('<II', data, table + i * 8)
        if ident == 0:
            need(function == 0, 'PSCI table terminator')
            break
        need(0x50000 <= function < text_end, 'PSCI table points outside SRAM')
    else:
        raise ValueError('unterminated PSCI table')
    dis = subprocess.check_output([cross + 'objdump', '-d', '-j', '.secure_text',
                                   str(build / 'u-boot')], text=True)
    for line in dis.splitlines():
        branch = re.search(r'\s(b(?:l|lx)?(?:eq|ne|cc|cs|mi|pl|vs|vc|hi|ls|ge|lt|gt|le)?)\s+([0-9a-f]+)\s+<', line)
        if branch:
            need(0x50000 <= int(branch[2], 16) < text_end, 'monitor branch leaves SRAM: ' + line)
        need(not re.search(r'\.word\s+0x[4567][0-9a-f]{7}', line),
             'monitor literal references disposable DRAM: ' + line)
    rel = subprocess.check_output([cross + 'readelf', '-rW', str(build / 'u-boot')], text=True)
    for line in rel.splitlines():
        fields = line.split()
        if fields and re.fullmatch(r'[0-9a-fA-F]{8}', fields[0]):
            need(not 0x50000 <= int(fields[0], 16) < 0x58000, 'relocation in fixed SRAM')
    need(not subprocess.check_output([cross + 'nm', '-u', str(build / 'u-boot')], text=True).strip(),
         'unresolved U-Boot symbols')
    return ranges


def build_bootarea(build, cross):
    config = (build / '.config').read_text()
    required = {
        'TARGET_DSHANPI_T153_SOURCE': 'y', 'T153_DDR_SIZE_MIB': '1024',
        'T153_PSCI_COLD_SMP': 'y', 'ARMV7_NONSEC': 'y', 'ARMV7_PSCI': 'y',
        'ARMV7_PSCI_0_1': 'y', 'ARMV7_PSCI_NR_CPUS': '4',
        'ARMV7_SECURE_BASE': '0x50000', 'ARMV7_SECURE_MAX_SIZE': '0x8000',
        'ARM_GIC_BASE_ADDRESS': '0x03400000', 'COUNTER_FREQUENCY': '24000000',
        'ARMV7_SET_CORTEX_SMPEN': 'y', 'SPL': 'y', 'SPL_TEXT_BASE': '0x40060',
        'SPL_STACK': '0x66000', 'SPL_EARLY_BSS': 'y', 'SPL_SYS_MALLOC_F_LEN': '0x4000',
        'SPL_SYS_ICACHE_OFF': 'y', 'SPL_SYS_DCACHE_OFF': 'y',
        'SPL_SYS_MMCSD_RAW_MODE': 'y', 'SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR': '0x100',
        'SPL_LEGACY_IMAGE_CRC_CHECK': 'y', 'SPL_DM_MMC': 'y', 'SPL_DM_SERIAL': 'y',
        'OF_BOARD_SETUP': 'y', 'TEXT_BASE': '0x4a000000', 'ENV_IS_NOWHERE': 'y',
        'BOARD_LATE_INIT': 'y', 'CMD_SOURCE': 'y', 'BOOTCOMMAND': '"run t153boot"',
    }
    for name, value in required.items():
        need(f'CONFIG_{name}={value}\n' in config, 'configuration: ' + name)
    for name in ('SPL_SHARES_INIT_SP_ADDR', 'SPL_MMC_WRITE', 'MMC_WRITE',
                 'ARMV7_PSCI_1_0', 'ARMV7_PSCI_0_2', 'ARMV7_VIRT',
                 'ARMV7_BOOT_SEC_DEFAULT', 'CMD_SAVEENV', 'FAT_WRITE', 'EXT4_WRITE', 'ARCH_SUNXI'):
        need(f'CONFIG_{name}=y\n' not in config, 'unsupported configuration: ' + name)
    elf(build / 'spl/u-boot-spl', 0x40060)
    syms = symbols(build / 'spl/u-boot-spl.sym')
    for name in ('t153_ddr_init', 't153_return_to_fel', 't153_fel_context',
                 't153_handle_fel_request', 't153_enter_fel_fresh',
                 'save_boot_params', 'spl_mmc_load_image', 't153_mmc_send_cmd'):
        need(name in syms, 'missing source SPL function: ' + name)
    for name in ('psci_cpu_on', 't153_psci_cpu_entry', 't153_smp_fixup_dt'):
        need(name not in syms, 'monitor must not be linked in SPL')
    map_text = (build / 'spl/u-boot-spl.map').read_text()
    need('board/dshanpi/t153/ddr.o' in map_text, 'source DDR object missing')
    for name in ('init_DRAM', 'mctl_phy_init', 'libdram'):
        need(name not in map_text, 'vendor DDR object: ' + name)
    raw = (build / 'spl/u-boot-spl.bin').read_bytes()
    nodtb = (build / 'spl/u-boot-spl-nodtb.bin').read_bytes()
    pad = (build / 'spl/u-boot-spl-pad.bin').read_bytes()
    dtb = (build / 'spl/u-boot-spl.dtb').read_bytes()
    need(raw == nodtb + pad + dtb, 'SPL DT must follow BSS padding')
    need(syms['__bss_start'] == 0x40060 + len(nodtb), 'SPL BSS start')
    need(syms['__bss_end'] == 0x40060 + len(nodtb) + len(pad), 'SPL BSS end')
    need(syms['t153_fel_context'] < syms['__bss_start'], 'FEL context must survive BSS clear')
    need(0x40060 + len(raw) < 0x5e000, 'SPL overlaps reserved stack or heap')
    nodes = fdt_nodes(dtb)
    for path in ('/serial@2600000', '/mmc@4020000', '/mmc@4022000'):
        need(path in nodes, 'SPL DT missing ' + path)
    spl = (build / 't153-spl.bin').read_bytes()
    need(spl[4:12] == b'eGON.BT0', 'SPL eGON magic')
    branch, checksum, length = struct.unpack_from('<I8xII', spl)
    need(branch == 0xea000016, 'SPL branch must enter at SRAM + 96')
    need(length == len(spl) and length % 8192 == 0 and length <= 0x18000, 'SPL size or padding')
    check = bytearray(spl)
    struct.pack_into('<I', check, 12, 0x5f0a6c39)
    need(sum(struct.unpack('<%dI' % (length // 4), check)) & 0xffffffff == checksum, 'eGON checksum')
    need(spl[96:96 + len(raw)] == raw and spl[20:23] == b'SPL', 'eGON source payload and FEL header')
    nameoff = struct.unpack_from('<I', spl, 32)[0]
    need(spl[nameoff:].split(b'\0', 1)[0] == b'sun8i-t153-dshanpi-omnigate-uboot', 'eGON board name')
    proper = (build / 'u-boot.bin').read_bytes()
    image = (build / 'u-boot.img').read_bytes()
    magic, hcrc, _stamp, size, load, entry, dcrc = struct.unpack_from('>7I', image)
    need(magic == 0x27051956 and size == len(proper) and image[64:] == proper, 'legacy payload')
    need(load == entry == 0x4a000000 and image[28:32] == bytes([17, 2, 5, 0]), 'legacy load/type')
    header = bytearray(image[:64])
    header[4:8] = bytes(4)
    need(zlib.crc32(header) == hcrc and zlib.crc32(proper) == dcrc, 'legacy CRCs')
    need(b'Four-core PSCI cold-boot profile' in proper, 'SMP profile not visible in image')
    ranges = secure_monitor(build, cross)
    area = spl + bytes(0x1e000 - len(spl)) + image
    need(0x2000 + len(area) < 0x400000, 'boot area overlaps FAT partition at 4 MiB')
    output = build / 'u-boot-t153-with-spl.bin'
    output.write_bytes(area)
    result = {
        'upstream': 'U-Boot 2026.07', 'source_ddr': True, 'vendor_ddr_or_monitor': False,
        'profile': 'dshanpi_t153_source_1g_smp', 'physical_ddr_mib': 1024,
        'usable_dram_mib': 384, 'linux_cpus': 4, 'psci': '0.1 CPU_ON only',
        'disk_spl_offset': 8192, 'disk_uboot_offset': 131072,
        'file_uboot_offset': 122880, 'secure_sram_sections': ranges,
        'image': output.name, 'bytes': len(area), 'sha256': hashlib.sha256(area).hexdigest(),
        'spl_sha256': hashlib.sha256(spl).hexdigest(),
        'uboot_img_sha256': hashlib.sha256(image).hexdigest(),
        'config_sha256': hashlib.sha256(config.encode()).hexdigest(),
    }
    (build / 't153-bootarea.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--cross-prefix', required=True)
    args = parser.parse_args()
    build_bootarea(args.build_dir.resolve(), args.cross_prefix)


if __name__ == '__main__':
    main()
