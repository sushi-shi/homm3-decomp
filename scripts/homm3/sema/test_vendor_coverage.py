"""Vendor intervals account for gaps without hiding DATA or changing denominators."""
import unittest
import json
from pathlib import Path
import struct
import tempfile

from homm3.core import tsv
from homm3.sema import data_coverage, vendor_coverage
from homm3.sema.image_coverage import partition, audit_partition, export
from homm3.sema.retail_claims import claim
from homm3.sema.retail_layout import Layout
from homm3.sema.test_retail_layout import fixture
from homm3.sema.test_data_coverage import declaration


def contribution(start=0x1008, size=16, status='verified', library='test.lib', index=0):
    return dict(id=index, rva=start, end=start+size, status=status, library=library,
                member='test.obj', evidence=['retail code relocation + COFF bytes'])


class VendorCoverageTest(unittest.TestCase):
    def overlay(self, cs, ds=()):
        layout = Layout(fixture())
        rows = partition(layout, [], {}, [])
        rows, _, _ = data_coverage.overlay(layout, rows, dict(declarations=list(ds), issues=[],
                                                            analysis_complete=True, fingerprint='test'))
        rows, summary = vendor_coverage.overlay(layout, rows, cs)
        audit_partition(rows, 'file', len(layout.data))
        audit_partition(rows, 'image', layout.image_size)
        return rows, summary

    def test_vendor_splits_gap_without_erasing_data_coverage_or_retail_evidence(self):
        rows, summary = self.overlay([contribution()])
        self.assertEqual(summary['vendor_gap_bytes'], 16)
        self.assertEqual(summary['data_gap_bytes'], 0x580)
        self.assertEqual(sum(r['size'] for r in data_coverage.gaps(rows)), 0x580)
        self.assertEqual(sum(r['size'] for r in vendor_coverage.unaccounted(rows)), 0x580 - 16)
        covered = next(r for r in rows if r['domain'] == 'image' and r['start'] == 0x1008)
        self.assertEqual(covered['data_accounting_status'], 'vendor')
        self.assertEqual(covered['category'], 'unknown')

    def test_candidate_conflict_and_rejected_cannot_close_gaps(self):
        for status in ('candidate', 'conflict', 'rejected'):
            _, summary = self.overlay([contribution(status=status)])
            self.assertEqual(summary['vendor_gap_bytes'], 0)
            self.assertEqual(summary['unaccounted_gap_bytes'], 0x580)

    def test_verified_range_does_not_upgrade_overlapping_candidate_library(self):
        rows, summary = self.overlay([contribution(), contribution(status='candidate', library='candidate.lib', index=1)])
        self.assertEqual(summary['vendor_gap_bytes'], 16)
        self.assertNotIn('candidate.lib', summary['by_library'])
        row = next(r for r in rows if r['vendor_status'] == 'verified')
        self.assertEqual(row['vendor_libraries'], ['candidate.lib', 'test.lib'])
        self.assertEqual(row['vendor_verified_libraries'], ['test.lib'])

    def test_vendor_and_declaration_union_without_double_counting(self):
        _, summary = self.overlay([contribution()], [declaration(0x1000, 16)])
        self.assertEqual(summary['vendor_gap_bytes'], 8)
        self.assertEqual(summary['vendor_verified_bytes'], 16)
        self.assertEqual(summary['data_gap_bytes'], 0x580 - 16)

    def test_identical_copies_count_once_but_partial_overlaps_conflict(self):
        a, b = contribution(), contribution(library='other.lib', index=1)
        rows, summary = self.overlay([a, b])
        self.assertEqual(summary['vendor_gap_bytes'], 16)
        self.assertEqual(summary['by_library']['other.lib']['gap_bytes'], 16)
        self.assertTrue(any(r['vendor_libraries'] == ['other.lib', 'test.lib'] for r in rows))
        b['rva'], b['end'] = 0x1010, 0x1020
        rows, summary = self.overlay([a, b])
        self.assertEqual(summary['vendor_conflict_bytes'], 8)
        self.assertEqual(summary['vendor_gap_bytes'], 16)
        issues = vendor_coverage.overlap_issues(rows)
        self.assertEqual([(i['rva'], i['end']) for i in issues], [(0x1010, 0x1018)])

    def test_bss_vendor_data_only_has_image_coverage(self):
        rows, summary = self.overlay([contribution(0x3020, 32)])
        self.assertEqual(summary['vendor_gap_bytes'], 32)
        self.assertFalse(any(r['domain'] == 'file' and r['vendor_status'] == 'verified' for r in rows))

    def test_import_owner_includes_slots_names_and_terminators_but_no_padding(self):
        data = fixture()
        struct.pack_into('<I', data, 0x98 + 92, 16)
        struct.pack_into('<II', data, 0x98 + 104, 0x1000, 40)
        struct.pack_into('<5I', data, 0x400, 0x1040, 0, 0, 0x1060, 0x1050)
        struct.pack_into('<II', data, 0x440, 0x1070, 0)
        struct.pack_into('<II', data, 0x450, 0x1070, 0)
        data[0x460:0x464] = b'x\0xx'
        data[0x470:0x477] = b'\0\0func\0'
        cs = vendor_coverage.import_contributions(Layout(data))
        self.assertTrue(all(c['library'] == 'x' and c['status'] == 'verified' for c in cs))
        self.assertEqual(sum(c['size'] for c in cs), 45)
        self.assertIn((0x1060, 2), [(c['rva'], c['size']) for c in cs])
        self.assertNotIn(0x1014, [c['rva'] for c in cs])  # End descriptor belongs to no DLL.

    def test_export_keeps_full_gap_report_and_reduced_unaccounted_report(self):
        cs = [contribution()]
        rows, summary = self.overlay(cs)
        report = dict(rows=rows, claims=[claim(0x1000, 4, 'object', 'test')], labels={}, references=[],
                      data_declarations=[], data_issues=[], vendor_data=cs, vendor_issues=[],
                      vendor_coverage=summary)
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            export(report, directory)
            gaps = tsv.read(directory/'data-gaps.tsv')[2]
            unaccounted = tsv.read(directory/'data-unaccounted.tsv')[2]
            self.assertEqual(sum(int(r['size']) for r in gaps), 0x580)
            self.assertEqual(sum(int(r['size']) for r in unaccounted), 0x580 - 16)
            self.assertEqual(tsv.read(directory/'vendor-data.tsv')[2][0]['library'], 'test.lib')
            exported_summary = json.loads((directory/'summary.json').read_text())
            self.assertNotIn('vendor_data', exported_summary)
            self.assertEqual(exported_summary['vendor_coverage'], summary)
