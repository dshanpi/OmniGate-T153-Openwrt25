#!/usr/bin/env python3
"""Reject a requested profile whose settings were silently dropped by Kconfig."""
from pathlib import Path
import re, sys

def values(path):
    result={}
    for line in Path(path).read_text().splitlines():
        if re.fullmatch(r'CONFIG_[A-Za-z0-9_+.-]+=.*',line):
            key,value=line.split('=',1);result[key]=value
        elif re.fullmatch(r'# CONFIG_[A-Za-z0-9_+.-]+ is not set',line):
            result[line.split()[1]]='n'
    return result
expected=values(sys.argv[1]);actual=values(sys.argv[2])
missing=[f'{key}: requested {value}, resolved {actual.get(key,"n")}'
         for key,value in expected.items() if actual.get(key,'n')!=value]
if missing:
    raise SystemExit('Profile configuration mismatch:\n'+'\n'.join(missing))
print(f'PASS: all {len(expected)} requested profile settings retained by defconfig')
