#!/usr/bin/env python3
"""Export the self-contained T153 board patch against the official v25.12.6 tarball.

Maintainer tool only: users apply the resulting patch with patch -p1, then use
normal OpenWrt feeds/menuconfig/make. No generated binaries enter this patch.
"""
from pathlib import Path
import argparse, difflib, hashlib, json, stat, tarfile

ROOT = Path(__file__).resolve().parents[1]
ARCHIVE_SHA256 = 'f5ff8258572a82df960470608ceb025655a42bc94b8e7e2a77a4373db46263d9'
COMMIT = '64900e756efa667774214f9c6b17c113222e01f7'
UPSTREAM_EDITS = {
    'target/linux/sunxi/modules.mk',
    'target/linux/sunxi/image/Makefile',
    'target/linux/sunxi/image/Config.in',
    'target/linux/sunxi/cortexa7/config-6.12',
    'target/linux/sunxi/base-files/etc/board.d/02_network',
    'target/linux/sunxi/base-files/lib/upgrade/platform.sh',
    'version', 'version.date',
}

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('source', type=Path, help='v25.12.6 tree after apply-native.sh')
    ap.add_argument('--archive', type=Path, default=ROOT/'downloads/openwrt-v25.12.6.tar.gz')
    ap.add_argument('--output', type=Path, default=ROOT/'artifacts/openwrt-25.12.6-t153.patch')
    a = ap.parse_args()
    with a.archive.open('rb') as f:
        if hashlib.file_digest(f, 'sha256').hexdigest() != ARCHIVE_SHA256:
            raise ValueError('official source archive does not match recorded SHA256')
    paths = set(UPSTREAM_EDITS)
    for subtree in ('package', 'target'):
        for p in (ROOT/'port'/subtree).rglob('*'):
            if p.is_file():
                if p.is_symlink(): raise ValueError(f'unsupported symlink: {p}')
                paths.add(p.relative_to(ROOT/'port').as_posix())
    original = {}
    with tarfile.open(a.archive, 'r:gz') as tar:
        for member in tar:
            path = member.name.partition('/')[2]
            if path not in paths: continue
            if not member.isfile(): raise ValueError(f'expected source file: {path}')
            original[path] = (tar.extractfile(member).read(), member.mode)
    chunks = []
    files = []
    for path in sorted(paths):
        p = a.source/path
        if p.is_symlink() or not p.is_file(): raise ValueError(f'missing regular file: {p}')
        data = p.read_bytes()
        old, old_mode = original.get(path, (b'', 0))
        mode = 0o100755 if p.stat().st_mode & stat.S_IXUSR else 0o100644
        previous_mode = 0o100755 if old_mode & stat.S_IXUSR else 0o100644
        if old == data and path in original and mode == previous_mode: continue
        text, before = data.decode('utf-8'), old.decode('utf-8')
        if text and not text.endswith('\n'): raise ValueError(f'missing final newline: {path}')
        if before and not before.endswith('\n'): raise ValueError(f'upstream missing final newline: {path}')
        chunks.append(f'diff --git a/{path} b/{path}\n')
        if path not in original:
            chunks.append(f'new file mode {mode:o}\n')
        elif mode != previous_mode:
            chunks.append(f'old mode {previous_mode:o}\nnew mode {mode:o}\n')
        chunks.extend(difflib.unified_diff(before.splitlines(True), text.splitlines(True),
                      fromfile=f'a/{path}' if path in original else '/dev/null', tofile=f'b/{path}'))
        files.append({'path':path, 'mode':f'{mode:o}', 'bytes':len(data),
                      'sha256':hashlib.sha256(data).hexdigest()})
    patch = ''.join(chunks).encode()
    if not patch: raise ValueError('empty patch')
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_bytes(patch)
    manifest = {'upstream_tag':'v25.12.6', 'upstream_commit':COMMIT,
                'source_archive_url':'https://codeload.github.com/openwrt/openwrt/tar.gz/refs/tags/v25.12.6',
                'source_archive_sha256_observed':ARCHIVE_SHA256,
                'patch':{'file':a.output.name, 'bytes':len(patch), 'sha256':hashlib.sha256(patch).hexdigest()},
                'files':files}
    a.output.with_suffix('.manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print(json.dumps(manifest['patch'], indent=2))

if __name__ == '__main__': main()
