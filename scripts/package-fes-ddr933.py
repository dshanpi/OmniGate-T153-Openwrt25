#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Reproduce the audited RAM-only FES933 loader without vendor host executables.

The user-supplied stock container contains vendor FES/DDR binaries. This script
only changes dram_clk and its eGON checksum; it does not make those inputs source.
No USB, block devices, SDK writes or external commands are used.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
OUTPUT_NAME = 't153-fes-ddr933-ram-loader.img'
EXPECTED_DIFF = {0x12e98, 0x12e99, 0x3180c, 0x3180d, 0x31838, 0x31839}


def require(test, message):
    if not test:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read_regular(path):
    require(path.is_file() and not path.is_symlink(), f'regular file required: {path}')
    return path.read_bytes()


def image_entries(raw):
    require(len(raw) >= 1024 and raw[:8] == b'IMAGEWTY', 'not an IMAGEWTY container')
    require(struct.unpack_from('<I', raw, 8)[0] == 0x300, 'unsupported container version')
    require(struct.unpack_from('<I', raw, 24)[0] == len(raw), 'container length mismatch')
    count = struct.unpack_from('<I', raw, 60)[0]
    require(count == 6 and len(raw) >= (count + 1) * 1024, 'RAM loader needs six entries')
    entries = []
    end_previous = (count + 1) * 1024
    for index in range(count):
        header = (index + 1) * 1024
        require(struct.unpack_from('<II', raw, header) == (256, 1024), 'invalid entry header')
        main = raw[header+8:header+16].decode('ascii').rstrip('\0 ')
        sub = raw[header+16:header+32].decode('ascii').rstrip('\0 ')
        filename = raw[header+36:header+292].split(b'\0', 1)[0].decode('ascii')
        stored, hi1, length, hi2, offset = struct.unpack_from('<5I', raw, header+292)
        require(hi1 == hi2 == 0 and stored >= length > 0, 'invalid entry length')
        require(offset % 1024 == 0 and end_previous <= offset and offset + stored <= len(raw), 'entry overlap/range')
        payload = raw[offset:offset+length]
        entries.append({'filename': filename, 'maintype': main, 'subtype': sub,
                        'offset': offset, 'length': length, 'sha256': sha(payload)})
        end_previous = offset + stored
    require(len({(e['maintype'], e['subtype']) for e in entries}) == 6, 'duplicate entry role')
    return entries


def field_offset(data, section, key):
    require(len(data) >= 16, 'truncated configuration')
    for i in range(struct.unpack_from('<I', data)[0]):
        off = 16 + 40 * i
        require(off + 40 <= len(data), 'configuration section bounds')
        if data[off:off+32].split(b'\0')[0].decode() != section:
            continue
        count, start = struct.unpack_from('<II', data, off+32)
        for j in range(count):
            pos = start * 4 + j * 40
            require(pos + 40 <= len(data), 'configuration field bounds')
            if data[pos:pos+32].split(b'\0')[0].decode() == key:
                value, kind = struct.unpack_from('<II', data, pos+32)
                require(kind == 0x10001 and value * 4 + 4 <= len(data), 'invalid integer field')
                return value * 4
    raise ValueError(f'missing configuration field {section}/{key}')


def egon_checksum(data, length_offset):
    require(len(data) >= 24 and len(data) % 4 == 0, 'eGON alignment')
    require(struct.unpack_from('<I', data, length_offset)[0] == len(data), 'eGON size mismatch')
    copy = bytearray(data)
    saved = struct.unpack_from('<I', copy, 12)[0]
    struct.pack_into('<I', copy, 12, 0x5f0a6c39)
    calculated = sum(struct.unpack('<%dI' % (len(copy)//4), copy)) & 0xffffffff
    require(saved == calculated, 'eGON checksum mismatch')
    return {'bytes':len(data), 'stored':hex(saved), 'calculated':hex(calculated)}


def generate(stock, manifest, profile, input_dir=None, sdk_root=None):
    require(sha(stock) == manifest['expected_output_sha256'], 'stock loader SHA256 mismatch')
    require(manifest['purpose'] == 'ram-fes-bootstrap' and manifest['flash_payload'] is False,
            'stock manifest must describe a RAM-only loader')
    entries = image_entries(stock)
    expected = manifest['entries']
    require(len(expected) == 6, 'manifest needs six entries')
    payloads = {}
    for actual, wanted in zip(entries, expected):
        require(wanted['flash_to_media'] is False, 'loader entry must never be flashed')
        require((actual['maintype'], actual['subtype']) == (wanted['maintype'], wanted['subtype']), 'unexpected IMAGEWTY role')
        require(actual['sha256'] == wanted['sha256'] == wanted['packed_sha256'], 'payload SHA256 mismatch')
        payload = stock[actual['offset']:actual['offset']+actual['length']]
        if input_dir:
            candidate = (input_dir / wanted['path']).resolve()
            require(candidate.is_relative_to(input_dir.resolve()), 'input path escapes profile')
            require(read_regular(candidate) == payload, 'external input differs from stock loader')
        payloads[wanted['role']] = (actual, payload)
    params = [entry['value'] for entry in profile['parameters']]
    require(len(params) == 32 and params[0] == 933, 'profile must contain exactly 32 DDR933 words')
    if sdk_root:
        source = read_regular(sdk_root/'device/config/chips/t153/configs/omnigate/sys_config.fex').decode()
        section = source.split('[dram_para]',1)[1].split('[',1)[0]
        values = [int(v,0) for _,v in re.findall(r'^\s*(dram_\w+)\s*=\s*(0x[\da-fA-F]+|\d+)',section,re.M)]
        require(values == params, 'SDK DDR profile differs from pinned parameters')
    fes_entry, fes = payloads['fes1']
    cfg_entry, cfg = payloads['sys-config']
    old_params = list(struct.unpack_from('<32I', fes, 56))
    require(old_params[0] == 1066 and old_params[1:] == params[1:], 'stock DDR parameter mismatch')
    before_checksum = egon_checksum(fes, 16)
    clk_at = field_offset(cfg, 'dram_para', 'dram_clk')
    require(clk_at == 0x1298 and struct.unpack_from('<I',cfg,clk_at)[0] == 1066, 'unexpected config clock')
    patched_fes = bytearray(fes)
    struct.pack_into('<I', patched_fes, 56, 933)
    struct.pack_into('<I', patched_fes, 12, 0x5f0a6c39)
    checksum = sum(struct.unpack('<%dI' % (len(patched_fes)//4), patched_fes)) & 0xffffffff
    struct.pack_into('<I', patched_fes, 12, checksum)
    after_checksum = egon_checksum(patched_fes, 16)
    output = bytearray(stock)
    output[fes_entry['offset']:fes_entry['offset']+len(fes)] = patched_fes
    struct.pack_into('<I', output, cfg_entry['offset']+clk_at, 933)
    differences = [{'offset':hex(i),'before':hex(a),'after':hex(b)}
                   for i,(a,b) in enumerate(zip(stock,output)) if a != b]
    require({int(d['offset'],16) for d in differences} == EXPECTED_DIFF, 'unexpected container modifications')
    require(sha(output) == profile['expected_output_sha256'], 'candidate SHA256 differs from audited reference')
    uboot = payloads['uboot'][1]
    egon_checksum(uboot,20)
    for offset in (0x246f4, 0x24784):
        require(struct.unpack_from('<I',uboot,offset)[0] == 0xe2811a0a, 'stock +40960 mapping changed')
    mbr = payloads['mbr-placeholder'][1]
    require(len(mbr) == 65536, 'placeholder MBR size')
    for offset in range(0,len(mbr),16384):
        require(struct.unpack_from('<I',mbr,offset)[0] == zlib.crc32(mbr[offset+4:offset+16384]), 'placeholder MBR CRC')
    updated_entries = image_entries(output)
    for actual,wanted in zip(updated_entries,expected):
        actual.update(role=wanted['role'],flash_to_media=False)
        if wanted['role'] not in ('fes1','sys-config'):
            require(actual['sha256'] == wanted['sha256'], 'unchanged payload modified')
    report = {'schema_version':1,'status':'software byte-for-byte reproduction passed; this invocation performs no hardware IO',
              'purpose':'ram-fes-bootstrap','flash_payload':False,'vendor_ddr_binary':True,
              'filename':OUTPUT_NAME,'bytes':len(output),'sha256':sha(output),
              'stock_sha256':sha(stock),'license':'LicenseRef-NOASSERTION',
              'profile_source':profile['source'],'frequency_mhz':{'before':1066,'after':933},
              'other_31_dram_words_unchanged':True,'stock_mmc_logical_offset_sectors':40960,
              'entire_container_byte_diff':differences,'fes1_checksum_before':before_checksum,
              'fes1_checksum_after':after_checksum,'entries':updated_entries,
              'external_vendor_host_executables_used':False,
              'limits':['Vendor RAM loader inputs remain binary; persistent source-only SPL is separate.',
                        'Exact output matches the second-flash loader; it does not establish the first-bit-error cause.',
                        'Use stock MMC translation with host raw compensation; never use physical0.']}
    return bytes(output), report


def atomic_write(path, data):
    path.parent.mkdir(parents=True,exist_ok=True)
    require(not path.exists() or (path.is_file() and not path.is_symlink()), 'output must be a regular file')
    with tempfile.NamedTemporaryFile(dir=path.parent, prefix='.'+path.name+'.', delete=False) as temp:
        temp.write(data)
        temp_path = Path(temp.name)
    try:
        os.replace(temp_path,path)
    finally:
        temp_path.unlink(missing_ok=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stock-loader',type=Path,default=Path(os.environ.get('FES_STOCK_LOADER', ROOT/'artifacts/t153-fes-ram-loader.img')))
    parser.add_argument('--input-dir',type=Path,default=Path(os.environ['FES_INPUT_DIR']) if 'FES_INPUT_DIR' in os.environ else None,
                        help='optional extracted six-input directory, validated against stock payloads')
    parser.add_argument('--sdk-root',type=Path,default=Path(os.environ['SDK_ROOT']) if 'SDK_ROOT' in os.environ else None,
                        help='optional SDK directory for independent 32-word profile comparison')
    parser.add_argument('--output-dir',type=Path,default=ROOT/'artifacts')
    parser.add_argument('--compare',type=Path,help='also require byte identity with an existing candidate')
    args=parser.parse_args()
    stock=read_regular(args.stock_loader)
    manifest=json.loads((ROOT/'configs/fes-stock-loader.json').read_text())
    profile=json.loads((ROOT/'configs/fes-ddr933-profile.json').read_text())
    output,report=generate(stock,manifest,profile,args.input_dir,args.sdk_root)
    if args.compare:
        require(read_regular(args.compare)==output,'comparison candidate differs')
    destination=args.output_dir/OUTPUT_NAME
    require(destination.resolve()!=args.stock_loader.resolve(),'output must not replace stock input')
    if args.compare:
        require(destination.resolve()!=args.compare.resolve(),'isolated output must not overwrite comparison input')
    atomic_write(destination,output)
    atomic_write(args.output_dir/'t153-fes-ddr933-ram-loader.manifest.json', (json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps({'artifact':str(destination),'bytes':len(output),'sha256':report['sha256'],
                      'changed_bytes':len(report['entire_container_byte_diff']),'vendor_host_tools_used':False}))


if __name__=='__main__':
    try:
        main()
    except (ValueError, OSError, KeyError, IndexError, struct.error) as error:
        raise SystemExit('FES packaging failed: '+str(error))
