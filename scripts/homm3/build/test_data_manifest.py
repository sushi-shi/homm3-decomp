"""Delinker ownership, exhaustive gaps and fresh candidate identity controls."""
from pathlib import Path
from types import SimpleNamespace
import hashlib
import json
import struct
from unittest.mock import patch
import tempfile
import unittest

from homm3.build import data_manifest as manifest
from homm3.build.canonicalize_data_symbols import load_compgen_data_claims, CoffObject
from homm3.build.test_equivalent_relocation_normalization import _base
from homm3.core import tsv


class DataManifestTest(unittest.TestCase):
    def setUp(self):
        self.layout = SimpleNamespace(sections=[SimpleNamespace(
            name='.rdata', rva=0x1000, mapped_size=32, raw_size=24)])

    def sample(self, unit='a', rva=0x1004, size=4, macro='DATA'):
        key = unit+':4:0'
        candidate = dict(id=key, unit=unit, symbols=['_table'], section_ordinal=4,
                         section_offset=0, physical_size=4, alignment=4, scopes=['external'], storage='rdata')
        binding = dict(id=0, status='bound', candidate_ids=[key], macro=macro,
                       rva=rva, size=size, source='src/a.cpp:4', literal_sha256='', name='_table')
        return dict(candidate_data=[candidate], data_bindings=[binding])

    def test_data_extent_is_delivered_and_does_not_absorb_next_gap(self):
        rows, issues = manifest.definitions(self.sample(size=1), {0x1004: 'data_1004'}, self.layout)
        self.assertFalse(issues)
        self.assertEqual((rows[0]['name'], rows[0]['size']), ('_table', 1))
        self.assertEqual(rows[0]['section_ordinal'], '-')
        ledger = manifest.accounting(self.layout, rows, issues)
        self.assertEqual(sum(r['size'] for r in ledger), 32)
        self.assertEqual(sum(r['size'] for r in ledger if r['status'] == 'gap'), 31)
        self.assertEqual(ledger[-1]['backing'], 'zero-fill')

    def test_conflicting_ranges_are_withheld_and_remain_in_accounting(self):
        report = self.sample()
        other = self.sample('b', rva=0x1006)
        other['data_bindings'][0].update(id=1, name='_other')
        for key in report:
            report[key].extend(other[key])
        rows, issues = manifest.definitions(report, {}, self.layout)
        self.assertEqual(rows, [])
        ledger = manifest.accounting(self.layout, rows, issues)
        self.assertEqual(sum(r['size'] for r in ledger if r['status'] == 'conflict'), 6)
        self.assertEqual(sum(r['size'] for r in ledger), 32)

    def test_shared_vendor_copy_has_one_game_owner(self):
        report = self.sample()
        other = self.sample('vendor_a')
        other['data_bindings'][0]['id'] = 1
        for key in report:
            report[key].extend(other[key])
        rows, issues = manifest.definitions(report, {}, self.layout)
        self.assertFalse(issues)
        self.assertEqual([r['object'] for r in rows], ['a.c'])

    def test_microsoft_vendor_allocation_is_an_ordinary_delinker_definition(self):
        rows, issues = manifest.definitions(self.sample('vendor_msvcrt', macro='VENDOR'), {}, self.layout)
        self.assertFalse(issues)
        self.assertEqual(rows[0]['object'], 'vendor_msvcrt.c')

    def test_same_candidate_cannot_be_given_two_retail_identities(self):
        report = self.sample()
        report['data_bindings'].append(dict(report['data_bindings'][0], id=1, rva=0x1010))
        rows, issues = manifest.definitions(report, {}, self.layout)
        self.assertFalse(rows)
        self.assertEqual(len(issues), 2)

    def test_names_or_sizes_missing_from_candidate_do_not_get_guessed(self):
        report = self.sample()
        report['candidate_data'][0]['section_ordinal'] = 0
        self.assertFalse(manifest.definitions(report, {}, self.layout)[0])
        report = self.sample(rva=0x101f)
        self.assertFalse(manifest.definitions(report, {}, self.layout)[0])

    def test_delivery_gate_rejects_missing_unit_and_truncated_data(self):
        from homm3.build.test_eh_handler_normalization import FixtureSection, _coff, _symbol
        payload = bytearray(_coff((FixtureSection('.data', b'abcd', ()),),
                                 (_symbol('table', 0, 1, 0, 2),)))
        struct.pack_into('<I', payload, 20+36, 0xc0300040)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            objdiff = root/'build/objdiff'
            objdiff.mkdir(parents=True)
            output = objdiff/'a.obj'
            output.write_bytes(payload)
            config = objdiff/'objdiff.json'
            config.write_text(json.dumps(dict(units=[dict(name='a', target_path='a.obj')])))
            manifest_path = root/'manifest.tsv'
            tsv.write(manifest_path, [], ['object', 'rva', 'size'],
                      [dict(object='a.c', rva='0x1000', size=4)])
            with patch.object(manifest.common, 'HOMM3_DIR', root), \
                 patch.object(manifest, 'DATA_OUT', manifest_path), \
                 patch.object(manifest, 'data_names', return_value={0x1000: 'table'}):
                manifest.verify_delivery()
                struct.pack_into('<I', payload, 20+16, 0)
                output.write_bytes(payload)
                with self.assertRaisesRegex(ValueError, 'truncated'):
                    manifest.verify_delivery()
                config.write_text(json.dumps(dict(units=[])))
                with self.assertRaisesRegex(ValueError, 'absent from objdiff'):
                    manifest.verify_delivery()

    def test_stale_binding_is_not_applied_to_a_recompiled_object(self):
        payload = _base()
        symbol = next(s for s in CoffObject(payload).symbols.values() if s.section > 0 and not s.aux_count)
        row = dict(name='table', object='a.c', rva='0x1004', size='1', storage='data', alignment='1',
                   section_ordinal=str(symbol.section), section_offset=hex(symbol.value), scope='external',
                   provenance='delink-definition', symbol=symbol.name,
                   object_sha256=hashlib.sha256(payload).hexdigest())
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'bindings.tsv'
            tsv.write(path, [], list(row), [row])
            self.assertEqual(len(load_compgen_data_claims(path, 'a', payload)), 1)
            changed = bytearray(payload)
            changed[4] ^= 1
            self.assertEqual(load_compgen_data_claims(path, 'a', bytes(changed)), ())


if __name__ == '__main__':
    unittest.main()
