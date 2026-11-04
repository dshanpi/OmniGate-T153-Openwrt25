#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Extract six hash-pinned RAM loader inputs from a user-supplied factory image.

Reproduces only the IMAGEWTY header metadata, six selected inputs and alignment.
No vendor executable or USB access. Vendor FES1/U-Boot payloads remain binaries.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('fes_package',ROOT/'scripts/package-fes-ddr933.py')
fes=importlib.util.module_from_spec(spec)
spec.loader.exec_module(fes)


def extract(factory_path,manifest,layout):
    fes.require(factory_path.is_file() and not factory_path.is_symlink(),'factory image must be a regular file')
    with factory_path.open('rb') as stream:
        digest=hashlib.file_digest(stream,'sha256').hexdigest()
        fes.require(digest==layout['factory_image_sha256'],'factory image SHA256 mismatch')
        stream.seek(0)
        header=stream.read(1024)
        fes.require(header[:8]==b'IMAGEWTY' and struct.unpack_from('<I',header,8)[0]==0x300,'factory IMAGEWTY format')
        factory_size=factory_path.stat().st_size
        fes.require(struct.unpack_from('<I',header,24)[0]==factory_size,'factory image length mismatch')
        count=struct.unpack_from('<I',header,60)[0]
        fes.require(0<count<256 and (count+1)*1024<=factory_size,'invalid factory header count')
        wanted={(e['maintype'],e['subtype']):e for e in manifest['entries']}
        payloads={}
        for i in range(count):
            stream.seek((i+1)*1024)
            item=stream.read(1024)
            fes.require(struct.unpack_from('<II',item)==(256,1024),'invalid factory entry header')
            key=(item[8:16].decode('ascii').rstrip('\0 '),item[16:32].decode('ascii').rstrip('\0 '))
            if key not in wanted:
                continue
            fes.require(key not in payloads,'duplicate selected factory entry')
            stored,hi1,length,hi2,offset=struct.unpack_from('<5I',item,292)
            fes.require(hi1==hi2==0 and 0<length<=stored and offset%1024==0 and
                        (count+1)*1024<=offset<=factory_size-stored,'invalid selected payload range')
            stream.seek(offset)
            payload=stream.read(length)
            fes.require(len(payload)==length and fes.sha(payload)==wanted[key]['sha256'],'selected factory payload SHA256 mismatch')
            payloads[key]=payload
    fes.require(payloads.keys()==wanted.keys(),'factory image lacks required six RAM-only roles')
    headers=b''.join(bytes.fromhex(h) for h in layout['headers_hex'])
    fes.require(len(headers)==layout['header_bytes']==7168 and fes.sha(headers)==layout['header_sha256'],'header metadata mismatch')
    output=bytearray([layout['padding_byte']])*layout['image_bytes']
    output[:len(headers)]=headers
    for i,e in enumerate(manifest['entries']):
        h=(i+1)*1024
        stored,hi1,length,hi2,offset=struct.unpack_from('<5I',headers,h+292)
        data=payloads[(e['maintype'],e['subtype'])]
        fes.require(length==len(data) and offset+stored<=len(output),'RAM payload/layout mismatch')
        output[offset:offset+length]=data
    fes.require(fes.sha(output)==manifest['expected_output_sha256']==layout['stock_sha256'],'reconstructed stock SHA256 mismatch')
    entries=fes.image_entries(output)
    for actual,wanted in zip(entries,manifest['entries']):
        fes.require(actual['sha256']==wanted['sha256']==wanted['packed_sha256'],'reconstructed payload mismatch')
        actual.update(role=wanted['role'],flash_to_media=False)
    return bytes(output),{'status':'PASS: pure Python factory extraction and exact stock RAM-loader reproduction',
                          'factory_sha256':digest,'factory_bytes':factory_size,'bytes':len(output),'sha256':fes.sha(output),
                          'purpose':'ram-fes-bootstrap','flash_payload':False,'vendor_ddr_binary':True,
                          'license':'LicenseRef-NOASSERTION','entries':entries,'vendor_host_tools_used':False}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--factory-image',type=Path,help='original hash-pinned T153 OmniGate factory IMAGEWTY')
    parser.add_argument('--sdk-root',type=Path,default=Path(os.environ.get('SDK_ROOT',ROOT.parent)))
    parser.add_argument('--output-dir',type=Path,default=ROOT/'artifacts')
    parser.add_argument('--compare',type=Path,help='optional existing stock image for byte comparison')
    args=parser.parse_args()
    factory=args.factory_image or args.sdk_root/'out/t153_linux_omnigate_uart0-default.img'
    manifest=json.loads((ROOT/'configs/fes-stock-loader.json').read_text())
    layout=json.loads((ROOT/'configs/fes-imagewty-layout.json').read_text())
    output,report=extract(factory,manifest,layout)
    if args.compare:
        fes.require(fes.read_regular(args.compare)==output,'stock reference byte comparison failed')
    destination=args.output_dir/'t153-fes-ram-loader.img'
    fes.require(destination.resolve()!=factory.resolve(),'output must not replace factory input')
    if args.compare:
        fes.require(destination.resolve()!=args.compare.resolve(),'isolated output must not overwrite comparison input')
    fes.atomic_write(destination,output)
    fes.atomic_write(args.output_dir/'t153-fes-stock-extraction.json',(json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps({'artifact':str(destination),'bytes':len(output),'sha256':report['sha256'],'vendor_host_tools_used':False}))


if __name__=='__main__':
    try:
        main()
    except (ValueError,OSError,KeyError,IndexError,struct.error) as error:
        raise SystemExit('Stock FES extraction failed: '+str(error))
