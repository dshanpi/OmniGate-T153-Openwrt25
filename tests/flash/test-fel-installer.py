#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Offline regression tests: never invoke a real FEL tool or access USB."""
import importlib.util
from pathlib import Path
import stat
import struct
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import zlib

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('bundle', ROOT / 'scripts/package-fel-installer.py')
b = importlib.util.module_from_spec(spec)
spec.loader.exec_module(b)


class ArchiveTests(unittest.TestCase):
    def setUp(self):
        self.entries = [('t153', stat.S_IFDIR | 0o755, b''),
                        ('t153/disk.img.gz', stat.S_IFREG | 0o644, b'payload'),
                        ('t153/install-emmc.sh', stat.S_IFREG | 0o755, b'#!/bin/sh\n')]

    def test_roundtrip_deterministic_root_owned(self):
        data = b.newc(self.entries)
        self.assertEqual(data, b.newc(self.entries))
        self.assertEqual(len(data) % 512, 0)
        self.assertEqual(b.parse_cpio(data), {name: (mode, data) for name, mode, data in self.entries})

    def test_no_path_escape_or_init_replacement(self):
        for name in ('/t153/file', '../t153/file', 't153/../../init', 'init', 't153/a/b', 't153/..', 't153/.'):
            with self.subTest(name=name), self.assertRaises(ValueError):
                b.newc([(name, stat.S_IFREG | 0o644, b'')])

    def test_duplicate_and_privileged_file_rejected(self):
        with self.assertRaises(ValueError):
            b.parse_cpio(b.newc(self.entries + [self.entries[1]]))
        with self.assertRaises(ValueError):
            b.parse_cpio(b.newc([('t153/tool', stat.S_IFREG | 0o4755, b'x')]))

    def test_symlinks_devices_and_truncation_rejected(self):
        for kind in (stat.S_IFLNK, stat.S_IFBLK, stat.S_IFCHR):
            with self.subTest(kind=kind), self.assertRaises(ValueError):
                b.parse_cpio(b.newc([('t153/tool', kind | 0o644, b'x')]))
        data = b.newc(self.entries)
        for end in (0, 15, 120, len(data.rstrip(b'\0')) - 20):
            with self.subTest(end=end), self.assertRaises((ValueError, UnicodeDecodeError)):
                b.parse_cpio(data[:end])
        with self.assertRaises(ValueError):
            b.parse_cpio(data + b'bad')


class ArtifactTests(unittest.TestCase):
    def test_missing_corrupt_and_symlink_artifacts_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            art = Path(temporary)
            path = art / 'fixture.bin'
            path.write_bytes(b'known contents')
            good = b.descriptor(path)
            with patch.object(b, 'ART', art):
                self.assertEqual(b.check_descriptor(good), path)
                path.write_bytes(b'wrong contents')
                with self.assertRaises(ValueError):
                    b.check_descriptor(good)
                path.unlink()
                with self.assertRaises(ValueError):
                    b.check_descriptor(good)
                other = art / 'other.bin'
                other.write_bytes(b'known contents')
                path.symlink_to(other)
                with self.assertRaises(ValueError):
                    b.check_descriptor(good)
                with self.assertRaises(ValueError):
                    b.check_descriptor(dict(good, file='../fixture.bin'))


class ScriptTests(unittest.TestCase):
    def image(self, text):
        encoded = text.encode()
        payload = struct.pack('>II', len(encoded), 0) + encoded
        header = bytearray(struct.pack('>7I4B32s', 0x27051956, 0, 0, len(payload), 0, 0,
                                       zlib.crc32(payload), 5, 2, 6, 0, b'T153 test'))
        struct.pack_into('>I', header, 4, zlib.crc32(header))
        return bytes(header) + payload

    def test_commands_and_crc(self):
        script = b.script_text(0x123456)
        self.assertIn('bootm 0x50000000 0x54000000:123456', script)
        self.assertNotIn('mmc write', script)
        self.assertNotIn('--write', script)
        image = self.image(script)
        b.verify_script(image, script)
        altered = bytearray(image); altered[-1] ^= 1
        with self.assertRaises(ValueError):
            b.verify_script(bytes(altered), script)
        with self.assertRaises(ValueError):
            b.verify_script(self.image(script + 'mmc write 0 0 1\n'), script)


class HostTests(unittest.TestCase):
    def setUp(self):
        quiet = patch('builtins.print')
        quiet.start()
        self.addCleanup(quiet.stop)

    GOOD_LIST = 'USB device 001:005   Allwinner T153     12345678:90abcdef:00000000:00000000\n'
    GOOD_VERSION = 'AWUSBFEX soc=00001922(T153) 00000001 ver=0001 44 08 scratchpad=00046e00 00000000 00000000\n'

    def paths(self):
        return {name: Path('/fixture/' + name) for name in ('uboot', 'fit', 'cpio', 'script')}

    def test_unique_t153_selection(self):
        self.assertEqual(b.selected_device(self.GOOD_LIST), '1:5')
        for listing in ('', self.GOOD_LIST * 2, self.GOOD_LIST.replace('T153', 'T113'), 'unexpected\n' + self.GOOD_LIST):
            with self.subTest(listing=listing), self.assertRaises(ValueError):
                b.selected_device(listing)
        b.check_version(self.GOOD_VERSION)
        with self.assertRaises(ValueError):
            b.check_version(self.GOOD_VERSION.replace('00001922', '00001859'))

    def test_default_plan_never_opens_usb(self):
        with patch.object(b, 'verify_bundle', return_value=({}, self.paths())), \
             patch.object(b.subprocess, 'run', side_effect=AssertionError('default opened USB')):
            b.host_plan(False)

    def test_transfer_pinned_device_only_after_preflight(self):
        calls = []
        def run(argv, **kwargs):
            calls.append(argv)
            if argv[-1] == '--list':
                return subprocess.CompletedProcess(argv, 0, self.GOOD_LIST, '')
            if argv[-1] == 'version':
                return subprocess.CompletedProcess(argv, 0, self.GOOD_VERSION, '')
            return subprocess.CompletedProcess(argv, 0)
        with patch.object(b, 'verify_bundle', return_value=({}, self.paths())), patch.object(b.subprocess, 'run', side_effect=run):
            b.host_plan(True)
        self.assertEqual(len(calls), 3)
        self.assertEqual(calls[1][1:3], ['--dev', '1:5'])
        self.assertEqual(calls[2], b.boot_command(self.paths(), '1:5'))
        self.assertEqual(calls[2][-3:], ['write', '0x4f100000', '/fixture/script'])
        self.assertFalse(any('flash' in word or 'nand' in word for word in calls[2]))

    def test_reject_wrong_board_before_ram_transfer(self):
        calls = []
        def run(argv, **kwargs):
            calls.append(argv)
            text = self.GOOD_LIST if argv[-1] == '--list' else self.GOOD_VERSION.replace('00001922', '00001859')
            return subprocess.CompletedProcess(argv, 0, text, '')
        with patch.object(b, 'verify_bundle', return_value=({}, self.paths())), patch.object(b.subprocess, 'run', side_effect=run), self.assertRaises(ValueError):
            b.host_plan(True)
        self.assertEqual(len(calls), 2)

    def test_failed_preflight_never_opens_usb(self):
        with patch.object(b, 'verify_bundle', side_effect=ValueError('corrupt payload')), \
             patch.object(b.subprocess, 'run', side_effect=AssertionError('invalid bundle opened USB')), self.assertRaises(ValueError):
            b.host_plan(True)


if __name__ == '__main__':
    unittest.main()
