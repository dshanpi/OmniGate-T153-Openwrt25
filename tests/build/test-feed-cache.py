#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Exercise the actual feed script against isolated archives and a fake curl."""
from pathlib import Path
import hashlib
import io
import os
import shutil
import subprocess
import tarfile
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
REV='1234567890abcdef1234567890abcdef12345678'
NAME='packages-'+REV+'.tar.gz'
CURL='''#!/bin/sh
set -eu
printf '%s\\n' "$*" >> "$TEST_CURL_LOG"
[ "$TEST_CURL_MODE" = copy ] || exit 99
out=''
while [ "$#" -gt 0 ]; do
  if [ "$1" = -o ]; then out=$2; shift 2; else shift; fi
done
cp "$TEST_CURL_SOURCE" "$out"
'''


class FeedCacheTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory(prefix='t153-feed-test-');self.root=Path(self.tmp.name)
        for directory in ['scripts','configs','openwrt','downloads/feeds','mocks']:(self.root/directory).mkdir(parents=True,exist_ok=True)
        shutil.copyfile(ROOT/'scripts/prepare-feeds.sh',self.root/'scripts/prepare-feeds.sh')
        self.original=self.root/'original.tar.gz'
        with tarfile.open(self.original,'w:gz') as tar:
            data=b'pinned fixture content\n';info=tarfile.TarInfo('packages-'+REV+'/README');info.size=len(data);tar.addfile(info,io.BytesIO(data))
        sha=hashlib.sha256(self.original.read_bytes()).hexdigest()
        (self.root/'configs/feeds.lock').write_text('packages '+REV+' '+sha+'\n')
        self.archive=self.root/'downloads/feeds'/NAME
        self.dest=self.root/'feed-sources'/('packages-'+REV)
        (self.root/'mocks/curl').write_text(CURL);(self.root/'mocks/curl').chmod(0o755)
        self.env=os.environ.copy();self.env.pop('FEED_CACHE',None);self.env.pop('FEED_DOWNLOAD_CACHE',None)
        self.env.update(PATH=str(self.root/'mocks')+os.pathsep+self.env['PATH'],TEST_CURL_MODE='deny',TEST_CURL_LOG=str(self.root/'curl.log'),TEST_CURL_SOURCE=str(self.original))

    def tearDown(self):self.tmp.cleanup()

    def run_script(self):
        return subprocess.run(['sh',str(self.root/'scripts/prepare-feeds.sh'),str(self.root/'openwrt')],env=self.env,capture_output=True,text=True)

    def assert_source(self):
        self.assertEqual((self.dest/'README').read_bytes(),b'pinned fixture content\n')
        self.assertEqual((self.dest/'.t153-complete').read_text().strip(),REV)
        self.assertIn(str(self.dest),(self.root/'openwrt/feeds.conf').read_text())

    def test_cached_archive_never_calls_curl(self):
        shutil.copyfile(self.original,self.archive)
        self.assertEqual(self.run_script().returncode,0);self.assert_source();self.assertFalse((self.root/'curl.log').exists())
        self.assertEqual(self.run_script().returncode,0);self.assertFalse((self.root/'curl.log').exists())

    def test_missing_archive_downloads_exact_revision(self):
        self.env['TEST_CURL_MODE']='copy'
        r=self.run_script();self.assertEqual(r.returncode,0,r.stderr);self.assert_source()
        self.assertIn('https://codeload.github.com/openwrt/packages/tar.gz/'+REV,(self.root/'curl.log').read_text())

    def test_bad_cached_hash_is_rejected_without_network(self):
        self.archive.write_bytes(b'corrupted')
        r=self.run_script();self.assertNotEqual(r.returncode,0);self.assertIn('SHA256 mismatch',r.stderr)
        self.assertFalse((self.root/'curl.log').exists());self.assertFalse(self.dest.exists())
        self.assertEqual(self.archive.read_bytes(),b'corrupted')

    def test_bad_download_hash_is_not_published(self):
        bad=self.root/'bad';bad.write_bytes(b'bad download')
        self.env.update(TEST_CURL_MODE='copy',TEST_CURL_SOURCE=str(bad))
        r=self.run_script();self.assertNotEqual(r.returncode,0);self.assertIn('SHA256 mismatch',r.stderr)
        self.assertFalse(self.archive.exists());self.assertFalse(self.dest.exists())

    def test_wrong_marker_is_rejected(self):
        shutil.copyfile(self.original,self.archive);self.dest.mkdir(parents=True)
        (self.dest/'.t153-complete').write_text('wrong\n')
        r=self.run_script();self.assertNotEqual(r.returncode,0);self.assertIn('marker differs',r.stderr)

    def test_verified_legacy_archive_migrates_without_download(self):
        legacy=self.root/'feed-sources'/NAME;legacy.parent.mkdir();shutil.copyfile(self.original,legacy)
        r=self.run_script();self.assertEqual(r.returncode,0,r.stderr);self.assert_source()
        self.assertTrue(self.archive.exists());self.assertEqual(legacy.stat().st_ino,self.archive.stat().st_ino)
        self.assertFalse((self.root/'curl.log').exists())


if __name__=='__main__':unittest.main()
