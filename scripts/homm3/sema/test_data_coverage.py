"""Missing DATA, shortened arrays, overlap and incomplete-analysis negative controls."""
import unittest

from homm3.sema import data_coverage
from homm3.sema.image_coverage import audit_partition, partition
from homm3.sema.retail_claims import claim
from homm3.sema.retail_layout import Layout
from homm3.sema.test_retail_layout import fixture


def declaration(start, size, name='array', definition=True, **kwargs):
    return dict(id=name, usr=name, rva=start, size=size, end=start + size if size else None,
                name=name, status='sized' if size else 'unknown-size', source='src/test.cpp:1',
                definition=definition, definition_sources=['src/test.cpp:1'] if definition else [],
                size_evidence='clang-i686-msvc-layout', **kwargs)


class CoverageTest(unittest.TestCase):
    def setUp(self):
        self.layout = Layout(fixture())
        # Identified on retail: it MUST still have a DATA gap until source covers it.
        self.rows = partition(self.layout, [claim(0x1000, 0x20, 'vtable', 'retail')], {}, [])

    def overlay(self, declarations=(), issues=(), complete=True):
        snapshot = dict(declarations=list(declarations), issues=list(issues),
                        analysis_complete=complete, fingerprint='test')
        return data_coverage.overlay(self.layout, self.rows, snapshot)

    def test_identified_objects_and_leading_trailing_zero_fill_gaps_remain(self):
        rows, issues, summary = self.overlay()
        self.assertFalse(issues)
        gaps = data_coverage.gaps(rows)
        self.assertTrue(any(r['category'] == 'identified' for r in gaps))
        self.assertTrue(any(r['storage'] == 'zero-fill' for r in gaps))
        self.assertEqual(sum(r['size'] for r in gaps), 0x580)
        self.assertEqual(summary['uncovered_bytes'], 0x580)

    def test_declaration_splits_retail_row_and_shortening_exposes_exact_gap(self):
        for size in (0x20, 0x10):
            rows, _, summary = self.overlay([declaration(0x1000, size)])
            audit_partition(rows, 'file', len(self.layout.data))
            audit_partition(rows, 'image', self.layout.image_size)
            self.assertEqual(summary['declared_bytes'], size)
            gap = next(r for r in data_coverage.gaps(rows) if r['start'] == 0x1000 + size)
            self.assertEqual(gap['end'], 0x1020 if size < 0x20 else 0x1100)
            self.assertEqual(summary['uncovered_bytes'], 0x580 - size)
        _, _, moved = self.overlay([declaration(0x1008, 0x10)])
        self.assertEqual(moved['declared_bytes'], 0x10)

    def test_extern_only_is_declared_but_not_defined(self):
        rows, _, summary = self.overlay([declaration(0x1000, 16, definition=False)])
        self.assertEqual(summary['declared_bytes'], 16)
        self.assertEqual(summary['extern-only_bytes'], 16)
        self.assertEqual(summary.get('defined_bytes', 0), 0)
        self.assertTrue(any(r['definition_status'] == 'extern-only' for r in rows))

    def test_distinct_overlapping_symbols_preserved_and_counted_once(self):
        rows, issues, summary = self.overlay([declaration(0x1000, 20, 'a'), declaration(0x1010, 20, 'b')])
        self.assertEqual(summary['declared_bytes'], 36)
        self.assertEqual(summary['overlap_bytes'], 4)
        overlap = next(r for r in rows if r['data_coverage_status'] == 'overlap')
        self.assertEqual(overlap['declared_owners'], ['a', 'b'])
        self.assertEqual([i['kind'] for i in issues], ['overlapping-declarations'])

    def test_repeated_same_entity_does_not_invent_overlap(self):
        a = declaration(0x1000, 20, 'same')
        b = dict(a, id='second-site', source='include/test.h:1')
        rows, _, summary = self.overlay([a, b])
        self.assertEqual(summary['declared_bytes'], 20)
        self.assertEqual(summary.get('overlap_bytes', 0), 0)
        covered = next(r for r in rows if r['data_coverage_status'] == 'covered')
        self.assertEqual(len(covered['declaration_ids']), 2)

    def test_unknown_invalid_and_conflicting_addresses_cannot_cover_storage(self):
        ds = [declaration(0x1000, None), declaration(0x22fc, 16, 'outside'),
              declaration(0x2000, 4, 'same'), declaration(0x2010, 4, 'same')]
        _, issues, summary = self.overlay(ds)
        self.assertEqual(summary.get('declared_bytes', 0), 0)
        self.assertEqual({i['kind'] for i in issues}, {'out-of-section', 'conflicting-address'})

    def test_skipped_analysis_cannot_be_reported_complete(self):
        rows, _, summary = self.overlay([declaration(0x1000, 4)],
                                        [dict(kind='parse-failure', rva=None, source='src/b.cpp', detail='error')], False)
        self.assertFalse(summary['analysis_complete'])
        self.assertTrue(all(not r['data_analysis_complete'] for r in rows))

    def test_raw_tail_boundary_does_not_drop_an_active_declaration(self):
        rows, _, summary = self.overlay([declaration(0x10fc, 8)])
        self.assertEqual(summary['declared_bytes'], 8)
        covered = [r for r in rows if r['domain'] == 'image' and r['data_coverage_status'] == 'covered']
        self.assertEqual([r['size'] for r in covered], [4, 4])
