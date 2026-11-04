#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Positive reproduction and fail-closed checks for the temporary FES loader."""
from pathlib import Path
import copy
import importlib.util
import json
import os
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('fes',ROOT/'scripts/package-fes-ddr933.py')
fes=importlib.util.module_from_spec(spec);spec.loader.exec_module(fes)


class FesTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        path=Path(os.environ.get('FES_STOCK_LOADER',ROOT/'artifacts/t153-fes-ram-loader.img'))
        if not path.is_file():
            raise unittest.SkipTest('prepare-fes-stock.py requires the user-supplied factory image first')
        cls.stock=path.read_bytes()
        cls.manifest=json.loads((ROOT/'configs/fes-stock-loader.json').read_text())
        cls.profile=json.loads((ROOT/'configs/fes-ddr933-profile.json').read_text())

    def test_reproduces_audited_candidate_exactly(self):
        first,report=fes.generate(self.stock,self.manifest,self.profile)
        second,_=fes.generate(self.stock,self.manifest,self.profile)
        self.assertEqual(first,second)
        self.assertEqual(fes.sha(first),'0d32d52139bc969f7f90665d32203ff07366ed0a31139bda607d89edbcd12873')
        self.assertEqual(len(report['entire_container_byte_diff']),6)
        self.assertTrue(all(e['flash_to_media'] is False for e in report['entries']))

    def test_rejects_wrong_input_before_modification(self):
        modified=bytearray(self.stock);modified[-1]^=1
        with self.assertRaisesRegex(ValueError,'stock loader SHA256'):
            fes.generate(modified,self.manifest,self.profile)

    def test_rejects_persistent_payload_marker(self):
        manifest=copy.deepcopy(self.manifest);manifest['entries'][0]['flash_to_media']=True
        with self.assertRaisesRegex(ValueError,'never be flashed'):
            fes.generate(self.stock,manifest,self.profile)

    def test_rejects_unreviewed_ddr_word(self):
        profile=copy.deepcopy(self.profile);profile['parameters'][1]['value']^=1
        with self.assertRaisesRegex(ValueError,'DDR parameter mismatch'):
            fes.generate(self.stock,self.manifest,profile)

    def test_rejects_external_payload_path_escape(self):
        manifest=copy.deepcopy(self.manifest);manifest['entries'][0]['path']='../unexpected'
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(ValueError,'escapes profile'):
                fes.generate(self.stock,manifest,self.profile,Path(temp))


if __name__=='__main__':
    unittest.main()
