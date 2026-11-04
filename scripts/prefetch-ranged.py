#!/usr/bin/env python3
"""Fetch a hash-pinned source archive using checked HTTP ranges (optional)."""
import concurrent.futures
import hashlib
import os
from pathlib import Path
import re
import sys
import time
import urllib.request

url,expected,destination=sys.argv[1:4]
dest=Path(destination).resolve()
if dest.exists():
    with dest.open('rb') as f:actual=hashlib.file_digest(f,'sha256').hexdigest()
    if actual==expected:
        print('Already verified:',dest.name,flush=True)
        sys.exit(0)
    raise SystemExit('Refusing to overwrite an existing archive with another hash')

def request(start,end):
    return urllib.request.urlopen(urllib.request.Request(url,headers={'Range':f'bytes={start}-{end}','Accept-Encoding':'identity'}),timeout=40)

with request(0,0) as response:
    match=re.fullmatch(r'bytes 0-0/(\d+)',response.headers.get('Content-Range',''))
    if response.status!=206 or not match:raise SystemExit('Server does not provide exact HTTP ranges')
    total=int(match[1])
    assert len(response.read())==1
work=dest.parent.parent/'prefetch'/dest.name
work.mkdir(parents=True,exist_ok=True)
chunk=1024*1024

def fetch(index):
    start=index*chunk;end=min(start+chunk,total)-1
    path=work/f'{index:05d}'
    if path.exists() and path.stat().st_size==end-start+1:return path
    for attempt in range(4):
        try:
            with request(start,end) as response:
                if response.status!=206 or response.headers.get('Content-Range')!=f'bytes {start}-{end}/{total}':
                    raise ValueError('Range response mismatch')
                data=response.read(end-start+2)
                if len(data)!=end-start+1:raise ValueError('Incorrect range length')
            path.write_bytes(data)
            return path
        except Exception:
            if attempt==3:raise
            time.sleep(attempt+1)

count=(total+chunk-1)//chunk
started=time.monotonic()
print('Fetch:',dest.name,'bytes:',total,'chunks:',count,flush=True)
with concurrent.futures.ThreadPoolExecutor(max_workers=16) as pool:
    futures=[pool.submit(fetch,i) for i in range(count)]
    done=0
    for f in concurrent.futures.as_completed(futures):
        f.result();done+=1
        if done%8==0 or done==count:print(f'{done}/{count}, {time.monotonic()-started:.1f}s',flush=True)
staged=work/'complete'
hashobj=hashlib.sha256()
with staged.open('wb') as out:
    for index in range(count):
        data=(work/f'{index:05d}').read_bytes();hashobj.update(data);out.write(data)
if hashobj.hexdigest()!=expected:raise SystemExit('Archive hash mismatch; not installed')
dest.parent.mkdir(parents=True,exist_ok=True)
try:os.link(staged,dest)
except FileExistsError:
    with dest.open('rb') as f:assert hashlib.file_digest(f,'sha256').hexdigest()==expected
staged.unlink()
for index in range(count):(work/f'{index:05d}').unlink()
work.rmdir()
print('Verified:',dest.name,expected,flush=True)
