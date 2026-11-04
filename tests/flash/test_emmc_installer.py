#!/usr/bin/env python3
"""Offline tests only: synthetic disk files and extracted read/validation functions.

The production script contains no path override/test mode. Tests never pass a
real block device, invoke its write path, or alter /sys, /proc, or /dev.
"""
import gzip
import hashlib
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / 'scripts/t153-install-emmc.sh'
RAW_BYTES = 306184192
SOURCE = SCRIPT.read_text()
FUNCTIONS = SOURCE.split('# Production entry point.', 1)[0]
SHELL = ['busybox', 'ash']


def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for data in iter(lambda: f.read(1024 * 1024), b''):
            h.update(data)
    return h.hexdigest()


def header():
    data = bytearray(262144)
    data[510:512] = b'\x55\xaa'
    struct.pack_into('<I', data, 440, 0x15302512)
    data[446] = 0x80
    data[450] = 0x0e
    struct.pack_into('<II', data, 454, 8192, 65536)
    data[466] = 0x83
    struct.pack_into('<II', data, 470, 73728, 524288)
    data[8196:8204] = b'eGON.BT0'
    struct.pack_into('<I', data, 8204, 0x5f0a6c39)
    struct.pack_into('<I', data, 8208, 49152)
    checksum = sum(struct.unpack_from('<12288I', data, 8192)) & 0xffffffff
    struct.pack_into('<I', data, 8204, checksum)
    struct.pack_into('>I', data, 131072, 0x27051956)
    struct.pack_into('>III', data, 131084, 262144, 0x4a000000, 0x4a000000)
    data[131100:131104] = bytes((17, 2, 5, 0))
    return data


class InstallerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix='t153-installer-tests-')
        cls.directory = Path(cls.temp.name)
        cls.raw = cls.directory / 'candidate.img'
        with cls.raw.open('wb') as f:
            f.write(header())
            f.truncate(RAW_BYTES)
        cls.digest = sha(cls.raw)
        cls.gz = cls.directory / 'candidate.img.gz'
        with cls.raw.open('rb') as src, cls.gz.open('wb') as dest:
            with gzip.GzipFile(fileobj=dest, mode='wb', mtime=0) as comp:
                shutil.copyfileobj(src, comp)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def run_function(self, tail, *, image=None, digest=None, functions=None, expect=0):
        with tempfile.TemporaryDirectory(prefix='t153-install.') as tmp:
            Path(tmp, 'header').write_bytes(header())
            env = dict(os.environ, TEST_WORK=tmp,
                       TEST_IMAGE=str(image or self.raw),
                       TEST_SHA=digest or self.digest)
            # Test-only harness selects temp inputs after loading definitions.
            # Production always runs main and has no such variables/interface.
            code = (functions or FUNCTIONS) + '''
WORK=$TEST_WORK
IMAGE=$TEST_IMAGE
EXPECT_SHA=$TEST_SHA
''' + tail
            result = subprocess.run(SHELL + ['-c', code, str(SCRIPT)], env=env,
                                    text=True, capture_output=True, timeout=60)
            self.assertEqual(result.returncode, expect, result.stdout + result.stderr)
            return result

    def test_valid_header(self):
        self.run_function('validate_header')

    def test_header_rejections(self):
        changes = {
            'signature': (510, b'\0'), 'disk_id': (440, b'\0'),
            'boot_flag': (446, b'\0'), 'partition_type': (450, b'\x83'),
            'partition_start': (455, b'\0'), 'partition_size': (459, b'\x01'),
            'root_type': (466, b'\x0e'), 'root_start': (471, b'\0'),
            'root_size': (476, b'\x01'), 'extra_partition': (478, b'\x80'),
            'spl_magic': (8196, b'x'), 'spl_checksum': (8204, b'\0'),
            'uboot_magic': (131072, b'\0'), 'uboot_load': (131088, b'\x42'),
            'uboot_type': (131100, b'\x05'), 'uboot_length': (131084, b'\xff'),
        }
        for name, (offset, value) in changes.items():
            with self.subTest(name=name):
                modified = bytearray(header())
                modified[offset:offset + len(value)] = value
                patch = self.directory / ('bad-' + name)
                patch.write_bytes(modified)
                self.run_function(f'cat "{patch}" >"$WORK/header"; validate_header', expect=1)

    def test_raw_full_preflight(self):
        self.run_function('validate_image')

    def test_gzip_full_preflight(self):
        self.run_function('validate_image', image=self.gz)

    def test_wrong_raw_hash(self):
        self.run_function('validate_image', digest='0' * 64, expect=1)

    def test_corrupt_gzip_footer_fails_even_with_correct_raw_hash(self):
        corrupt = self.directory / 'bad-footer.gz'
        data = bytearray(self.gz.read_bytes())
        data[-8] ^= 1
        corrupt.write_bytes(data)
        result = self.run_function('validate_image', image=corrupt, expect=1)
        self.assertIn('stream failed', result.stderr)

    def test_truncated_gzip(self):
        truncated = self.directory / 'truncated.gz'
        truncated.write_bytes(self.gz.read_bytes()[:-6])
        self.run_function('validate_image', image=truncated, expect=1)

    def test_wrong_size_even_with_matching_hash(self):
        short = self.directory / 'short.img'
        short.write_bytes(header())
        result = self.run_function('validate_image', image=short, digest=sha(short), expect=1)
        self.assertIn('image size must be', result.stderr)

    def test_readback_exact_length(self):
        self.run_function('DEVICE=$TEST_IMAGE; verify_readback')

    def test_readback_corruption(self):
        self.run_function('DEVICE=$TEST_IMAGE; verify_readback', digest='0' * 64, expect=1)

    def test_readback_truncated_source(self):
        short = self.directory / 'short-readback.img'
        short.write_bytes(header())
        self.run_function('DEVICE=$TEST_IMAGE; verify_readback', image=short, expect=1)

    def test_root_mount_guards(self):
        fixture = self.directory / 'proc-fixture'
        fixture.mkdir(exist_ok=True)
        (fixture / 'swaps').write_text('Filename Type Size Used Priority\n')
        # Path replacement exists ONLY in the isolated unit-test source string.
        funcs = FUNCTIONS.replace('/proc/mounts', str(fixture / 'mounts')).replace('/proc/swaps', str(fixture / 'swaps'))
        for name, text, expect in [
            ('ram', 'rootfs / rootfs rw 0 0\nproc /proc proc rw 0 0\n', 0),
            ('ext4', '/dev/root / ext4 rw 0 0\n', 1),
            ('overlay', 'overlayfs:/overlay / overlay rw 0 0\n', 1),
            ('persistent', 'rootfs / rootfs rw 0 0\n/dev/sda1 /mnt ext4 rw 0 0\n', 1),
            ('non_sd_ro', 'rootfs / rootfs rw 0 0\n/dev/sda1 /mnt ext4 ro 0 0\n', 1),
        ]:
            with self.subTest(name=name):
                (fixture / 'mounts').write_text(text)
                self.run_function('validate_mounts', functions=funcs, expect=expect)
        (fixture / 'mounts').write_text('rootfs / rootfs rw 0 0\n')
        (fixture / 'swaps').write_text('Filename Type Size Used Priority\n/dev/zram0 partition 1 0 0\n')
        self.run_function('validate_mounts', functions=funcs, expect=1)

    def test_sysfs_identity_and_use_guards(self):
        base = self.directory / 'state-fixture'
        sys = base / 'sys'
        node = sys / 'mmcblk0'
        card = base / '4022000.mmc/mmc_host/mmc0/mmc0:0001'
        card.mkdir(parents=True, exist_ok=True)
        node.mkdir(parents=True, exist_ok=True)
        (node / 'device').symlink_to(card)
        (node / 'holders').mkdir()
        proc = base / 'proc'
        proc.mkdir()
        defaults = {card / 'type': 'MMC', card / 'cid': 'a' * 32,
                    node / 'removable': '0', node / 'ro': '0',
                    node / 'size': '16777216', node / 'dev': '179:0',
                    proc / 'mounts': 'rootfs / rootfs rw 0 0\n',
                    proc / 'swaps': 'Filename Type Size Used Priority\n',
                    proc / 'mountinfo': ''}
        def reset():
            for path, value in defaults.items():
                path.write_text(value)
        reset()
        funcs = FUNCTIONS.replace('/sys/class/block', str(sys))
        for name in ('mounts', 'swaps', 'self/mountinfo'):
            funcs = funcs.replace('/proc/' + name, str(proc / name.split('/')[-1]))
        tail = 'BASE=mmcblk0; DEVICE=/dev/mmcblk0; EXPECT_CID=' + 'a' * 32 + '; validate_target_state'
        self.run_function(tail, functions=funcs)
        mutations = [(card / 'cid', 'b' * 32), (card / 'type', 'SD'),
                     (node / 'removable', '1'), (node / 'ro', '1'),
                     (node / 'size', '500000'),
                     (proc / 'mountinfo', '24 1 179:0 / /mnt rw - ext4 /dev/alias rw\n')]
        for path, value in mutations:
            with self.subTest(path=str(path)):
                reset()
                path.write_text(value)
                self.run_function(tail, functions=funcs, expect=1)
        reset()
        (node / 'holders/dm-0').touch()
        self.run_function(tail, functions=funcs, expect=1)
        (node / 'holders/dm-0').unlink()
        (node / 'device').unlink()
        wrong = base / '4020000.mmc/mmc_host/mmc1/mmc1:0001'
        wrong.mkdir(parents=True)
        (node / 'device').symlink_to(wrong)
        self.run_function(tail, functions=funcs, expect=1)

    def test_extcsd_boot_configuration_guards(self):
        helper = self.directory / 'mmc-info-fixture'
        # Replace helper fallback in test-only function text, never production.
        funcs = FUNCTIONS.replace('/usr/sbin/t153-mmc-info', str(helper))
        for config, expect in [(0, 0), (56, 0), (8, 1), (16, 1), (1, 1), (255, 1)]:
            with self.subTest(config=config):
                helper.write_text('#!/bin/sh\nprintf "PARTITION_CONFIG=' + str(config) + '\\nBOOT_BUS_CONDITIONS=0\\n"\n')
                helper.chmod(0o700)
                self.run_function('DEVICE=/dev/mmcblk999999; read_boot_configuration', functions=funcs, expect=expect)
        helper.write_text('#!/bin/sh\necho PARTITION_CONFIG=0\necho PARTITION_CONFIG=56\necho BOOT_BUS_CONDITIONS=0\n')
        self.run_function('DEVICE=/dev/mmcblk999999; read_boot_configuration', functions=funcs, expect=1)

    def test_production_has_no_test_bypass(self):
        self.assertNotIn('SYSFS_ROOT', SOURCE)
        self.assertNotIn('PROC_ROOT', SOURCE)
        self.assertNotIn('T153_TEST', SOURCE)
        self.assertTrue(SOURCE.rstrip().endswith('main "$@"'))
        env = dict(os.environ, SYSFS_ROOT=str(self.directory), PROC_ROOT=str(self.directory),
                   T153_TEST_ROOT=str(self.directory), T153_TEST_MODE='1')
        args = ['--write', '--device', '/dev/mmcblk999999', '--expect-cid', '0' * 32,
                '--image', str(self.raw), '--sha256', self.digest]
        before = sha(self.raw)
        result = subprocess.run(SHELL + [str(SCRIPT)] + args, env=env, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn('WRITE_BEGIN', result.stdout)
        self.assertEqual(before, sha(self.raw))
        unknown = subprocess.run(SHELL + [str(SCRIPT), '--test-root', str(self.directory)], capture_output=True, text=True)
        self.assertNotEqual(unknown.returncode, 0)
        self.assertIn('unknown argument', unknown.stderr)

    def test_help_has_no_side_effect(self):
        result = subprocess.run(SHELL + [str(SCRIPT), '--help'], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0)
        self.assertIn('uncompressed', result.stdout)


if __name__ == '__main__':
    unittest.main(verbosity=2)
