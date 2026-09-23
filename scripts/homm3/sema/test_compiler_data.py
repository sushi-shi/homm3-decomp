"""Compiler accounting must preserve bytes, uncertain ownership and match scope."""
import json
from pathlib import Path
import struct
import tempfile
import unittest

from homm3.core import tsv
from homm3.sema import compiler_data, data_coverage, vendor_coverage
from homm3.sema.image_coverage import audit_partition, export, partition
from homm3.sema.retail_claims import claim
from homm3.sema.retail_layout import Layout
from homm3.sema.test_retail_layout import fixture


class CompilerDataTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        (self.root/'config/retail').mkdir(parents=True)
        (self.root/'config/retail/vtables.tsv').write_text('rva\tfunction_count\tclass\n0x1080\t2\tWidget\n')
        self.data = fixture()
        self.data[0x178 + 40:0x178 + 48] = b'.text\0\0\0'
        self.data[0x600:0x609] = b'\x55\x8b\xec\x68' + struct.pack('<I', 0x402080) + b'\xc3'
        self.data[0x680:0x68a] = b'\xb8' + struct.pack('<I', 0x401000) + b'\xe9' + struct.pack('<i', 0x2100-0x208a)
        struct.pack_into('<7I', self.data, 0x400, 0x19930520, 1, 0x401040, 0, 0, 0, 0)
        struct.pack_into('<iI', self.data, 0x440, -1, 0)
        struct.pack_into('<2I', self.data, 0x480, 0x402000, 0x402000)
        self.claims = [claim(0x2000, 9, 'function', 'retail function inventory'),
                       claim(0x2080, 10, 'eh-handler-stub', 'validated handler'),
                       claim(0x1000, 28, 'eh-funcinfo', 'validated FuncInfo'),
                       claim(0x1040, 8, 'eh-unwind-map', 'validated unwind map'),
                       claim(0x1080, 8, 'vtable', 'admitted vtable'),
                       claim(0x10a0, 20, 'import-structure', 'PE import descriptor')]
        self.references = [dict(site_rva=0x2004, target_rva=0x2080, admitted=True)]
        self.labels = {0x2000: [dict(name='owner', source='src/owner.cpp')]}

    def collect(self):
        return compiler_data.collect(self.root, Layout(self.data), self.claims, self.labels, self.references)

    def test_owners_follow_decoded_push_and_named_vtable_inventory(self):
        rows, issues = self.collect()
        self.assertFalse(issues)
        self.assertEqual(len(rows), 4)
        for row in rows:
            if row['kind'].startswith('eh-'):
                self.assertEqual(row['owner_rvas'], ['0x2000'])
                self.assertEqual(row['owner_names'], ['owner'])
                self.assertEqual(row['handler_rvas'], ['0x2080'])
            elif row['kind'] == 'vtable':
                self.assertEqual(row['class_name'], 'Widget')
            else:
                self.assertEqual(row['owner_kind'], 'linker')
            self.assertEqual(row['candidate_matched_bytes'], 0)
            self.assertEqual(row['candidate_status'], 'not-compared')

    def test_unadmitted_reference_and_embedded_push_bytes_do_not_prove_owner(self):
        for mutation in ('unadmitted', 'embedded'):
            with self.subTest(mutation=mutation):
                self.setUp()
                if mutation == 'unadmitted':
                    self.references[0]['admitted'] = False
                else:
                    # B8's immediate contains 68; scanning that byte would see
                    # a false handler push, but full instruction decoding does not.
                    self.data[0x600:0x608] = b'\xb8\x68' + struct.pack('<I', 0x402080) + b'\xc3\x90'
                    self.references[0]['site_rva'] = 0x2002
                rows, issues = self.collect()
                self.assertTrue(any(i['kind'] == 'unresolved-owner' for i in issues))
                self.assertTrue(all(not r['owner_rvas'] for r in rows if r['kind'].startswith('eh-')))
                self.assertTrue(all(r['status'] == 'accounted' for r in rows))

    def test_unknown_class_is_not_invented_from_navigation_label(self):
        (self.root/'config/retail/vtables.tsv').write_text('rva\tfunction_count\tclass\n0x1080\t2\t\n')
        self.labels[0x1080] = [dict(name='inventedClass', source='snapshot')]
        rows, issues = self.collect()
        vtable = next(r for r in rows if r['kind'] == 'vtable')
        self.assertEqual(vtable['owner_kind'], 'unresolved')
        self.assertFalse(vtable['class_name'])
        self.assertTrue(any(i['compiler_id'] == vtable['id'] for i in issues))

    def test_vtable_slot_outside_code_is_invalid(self):
        struct.pack_into('<I', self.data, 0x480, 0x401000)
        rows, issues = self.collect()
        self.assertEqual(next(r for r in rows if r['kind'] == 'vtable')['status'], 'invalid')
        self.assertTrue(any(i['kind'] == 'invalid-structure' for i in issues))

    def test_zeroes_without_import_descriptor_evidence_are_not_linker_records(self):
        self.claims[-1]['evidence'] = 'observed zero run'
        rows, _ = self.collect()
        self.assertFalse(any(r['kind'] == 'import-terminator' for r in rows))

    def test_shared_map_retains_both_function_owners(self):
        struct.pack_into('<7I', self.data, 0x420, 0x19930520, 1, 0x401040, 0, 0, 0, 0)
        self.data[0x610:0x619] = b'\x55\x8b\xec\x68' + struct.pack('<I', 0x402090) + b'\xc3'
        self.data[0x690:0x69a] = b'\xb8' + struct.pack('<I', 0x401020) + b'\xe9' + struct.pack('<i', 0x2100-0x209a)
        self.claims.extend([claim(0x1020, 28, 'eh-funcinfo', 'validated FuncInfo'),
                            claim(0x2090, 10, 'eh-handler-stub', 'validated handler'),
                            claim(0x2010, 9, 'function', 'retail function inventory')])
        self.references.append(dict(site_rva=0x2014, target_rva=0x2090, admitted=True))
        rows, _ = self.collect()
        self.assertEqual(next(r for r in rows if r['kind'] == 'eh-unwind-map')['owner_rvas'], ['0x2000', '0x2010'])

    def test_overlay_and_export_do_not_change_match_or_data_denominators(self):
        layout = Layout(self.data)
        structures, issues = self.collect()
        rows = partition(layout, self.claims, self.labels, self.references)
        rows, _, data_summary = data_coverage.overlay(layout, rows, dict(declarations=[], issues=[],
                                                                       analysis_complete=True, fingerprint='test'))
        rows, vendor_summary = vendor_coverage.overlay(layout, rows, [])
        rows, summary = compiler_data.overlay(rows, structures)
        self.assertEqual(summary['newly_accounted_gap_bytes'], 64)
        self.assertEqual(summary['candidate_matched_bytes'], 0)
        self.assertEqual(summary['remaining_unaccounted_bytes'], vendor_summary['unaccounted_gap_bytes']-64)
        self.assertEqual(sum(r['size'] for r in data_coverage.gaps(rows)), data_summary['uncovered_bytes'])
        audit_partition(rows, 'file', len(self.data))
        audit_partition(rows, 'image', layout.image_size)
        report = dict(rows=rows, claims=self.claims, labels={}, references=[], vendor_data=[], vendor_issues=[],
                      compiler_data=structures, compiler_issues=issues, compiler_coverage=summary)
        directory = self.root/'reports'
        export(report, directory)
        self.assertEqual(len(tsv.read(directory/'compiler-data.tsv')[2]), 4)
        self.assertNotIn('compiler_data', json.loads((directory/'summary.json').read_text()))
        self.assertEqual(sum(int(r['size']) for r in tsv.read(directory/'data-unaccounted.tsv')[2]),
                         summary['remaining_unaccounted_bytes'])

    def test_conflicting_retail_claims_cannot_be_accounted_as_compiler_storage(self):
        structures, _ = self.collect()
        row = dict(domain='image', size=28, claims=[2], category='conflict',
                   data_accounting_status='unresolved', data_coverage_status='uncovered')
        rows, summary = compiler_data.overlay([row], structures)
        self.assertEqual(rows[0]['compiler_status'], 'conflict')
        self.assertEqual(summary['newly_accounted_gap_bytes'], 0)
