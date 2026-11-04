#!/usr/bin/env python3
"""Offline negative tests for bytes that will become T153 boot media."""
import importlib.util
import os
from pathlib import Path
import struct
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('t153_disk', ROOT / 'scripts/package-disk-image.py')
disk = importlib.util.module_from_spec(spec)
spec.loader.exec_module(disk)


class DiskImageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        profile = os.environ.get('T153_DISK_PROFILE', '512m')
        if profile not in ('512m', '1g'):
            raise ValueError('T153_DISK_PROFILE must be 512m or 1g')
        prefix = 't153-uboot-2026.07-source' + ('-1g' if profile == '1g' else '')
        cls.area = (ROOT / 'artifacts' / (prefix + '-sd-bootarea.bin')).read_bytes()
        cls.build = next((ROOT / 'openwrt/build_dir').glob('target-*/linux-sunxi_cortexa7'))
        cls.fit = (cls.build / 'dshanpi_t153mx-omnigate-kernel.bin').read_bytes()

    def test_layout_contract(self):
        raw = disk.make_mbr()
        self.assertEqual(raw[510:], b'\x55\xaa')
        self.assertEqual(struct.unpack_from('<I', raw, 440)[0], 0x15302512)
        for off, flag, kind, start, size in [(446, 0x80, 0x0e, 8192, 65536),
                                           (462, 0, 0x83, 73728, 524288)]:
            self.assertEqual(raw[off], flag)
            self.assertEqual(raw[off + 4], kind)
            self.assertEqual(struct.unpack_from('<II', raw, off + 8), (start, size))
        disk.verify_mbr(raw, 306184192)

    def test_root_partition_cannot_overlap_boot(self):
        corrupt = bytearray(disk.make_mbr())
        struct.pack_into('<I', corrupt, 462 + 8, 73727)
        with self.assertRaisesRegex(ValueError, 'MBR'):
            disk.verify_mbr(corrupt, disk.DISK_BYTES)

    def test_changed_disk_id_breaks_root_contract(self):
        corrupt = bytearray(disk.make_mbr())
        corrupt[440] ^= 1
        with self.assertRaisesRegex(ValueError, 'MBR'):
            disk.verify_mbr(corrupt, disk.DISK_BYTES)

    def test_truncated_disk_rejected(self):
        with self.assertRaisesRegex(ValueError, 'disk length'):
            disk.verify_mbr(disk.make_mbr(), disk.DISK_BYTES - 512)

    def test_valid_bootarea(self):
        details = disk.verify_bootarea(self.area)
        self.assertEqual(details['spl_offset'], 8192)
        self.assertEqual(details['proper_offset'], 131072)
        self.assertLess(details['bytes'] + 8192, 4 * 1024 * 1024)

    def test_corrupt_spl_rejected(self):
        corrupt = bytearray(self.area)
        corrupt[96] ^= 1
        with self.assertRaisesRegex(ValueError, 'eGON checksum'):
            disk.verify_bootarea(corrupt)

    def test_overlong_spl_rejected(self):
        corrupt = bytearray(self.area)
        struct.pack_into('<I', corrupt, 16, 4 * 1024 * 1024)
        with self.assertRaisesRegex(ValueError, 'SPL length'):
            disk.verify_bootarea(corrupt)

    def test_corrupt_proper_rejected(self):
        corrupt = bytearray(self.area)
        corrupt[disk.UBOOT_OFFSET - disk.SPL_OFFSET + 64] ^= 1
        with self.assertRaisesRegex(ValueError, 'legacy header/data CRC'):
            disk.verify_bootarea(corrupt)

    def test_wrong_proper_load_rejected_even_with_valid_crc(self):
        image = bytearray(self.area[disk.UBOOT_OFFSET - disk.SPL_OFFSET:])
        struct.pack_into('>I', image, 16, 0x40000000)
        image[4:8] = bytes(4)
        struct.pack_into('>I', image, 4, zlib.crc32(image[:64]))
        with self.assertRaisesRegex(ValueError, 'load/entry'):
            disk.verify_legacy(image, 5, 0x4a000000)

    def test_corrupt_fit_rejected(self):
        nodes = disk.verify.fdt_nodes(self.fit)
        payload = nodes['/images/kernel-1']['data']
        off = self.fit.index(payload)
        corrupt = bytearray(self.fit)
        corrupt[off + len(payload) // 2] ^= 1
        with self.assertRaisesRegex(ValueError, 'FIT hash mismatch'):
            disk.fit_details(corrupt)

    def test_normal_fit_has_only_empty_builtin_cpio(self):
        result = disk.verify_persistent_kernel(self.fit, self.build)
        self.assertFalse(result['has_init'])
        self.assertFalse(result['populated_initramfs'])
        self.assertLessEqual(result['builtin_cpio_bytes'], 4096)
        self.assertNotIn('init', result['builtin_cpio_entries'])

    def test_rescue_fit_cannot_be_used_for_persistent_disk(self):
        rescue = next((ROOT / 'openwrt/bin/targets/sunxi/cortexa7').glob('*dshanpi_t153mx-omnigate-initramfs-kernel.bin'))
        with self.assertRaisesRegex(ValueError, 'ordinary zImage'):
            disk.verify_persistent_kernel(rescue.read_bytes(), self.build)


if __name__ == '__main__':
    unittest.main(verbosity=2)
