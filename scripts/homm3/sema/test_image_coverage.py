"""Whole-file/image accounting, aliases, evidence confidence and TSV contracts."""
import json
from pathlib import Path
import tempfile
import unittest

from homm3.core import tsv
from homm3.sema.image_coverage import audit_partition, backlog_rows, compatible, export, partition, validate_claims
from homm3.sema.retail_claims import claim
from homm3.sema.retail_layout import Layout
from homm3.sema.test_retail_layout import fixture


class AccountingTest(unittest.TestCase):
    def setUp(self):
        self.layout = Layout(fixture())

    def rows(self, claims=(), labels=None, refs=()):
        return partition(self.layout, list(claims), labels or {}, refs)

    def test_every_byte_in_both_domains_including_gaps_is_unknown_without_evidence(self):
        rows = self.rows()
        for domain, total in [('file', len(self.layout.data)), ('image', self.layout.image_size)]:
            audit_partition(rows, domain, total)
            self.assertEqual(sum(r['size'] for r in rows if r['domain'] == domain), total)
        self.assertTrue(all(r['category'] == 'unknown' for r in rows))
        self.assertTrue(all(r['next_action'] for r in rows))

    def test_aliases_share_extent_and_both_names_and_sources_survive(self):
        claims = [claim(0x1000, 20, 'vtable', 'reviewed', 'object')]
        labels = {0x1000: [dict(name='one', source='src/a.cpp'), dict(name='two', source='src/b.cpp')]}
        rows = self.rows(claims, labels)
        known = [r for r in rows if r['category'] == 'identified']
        self.assertEqual(len(known), 2)
        self.assertTrue(all(r['shared'] for r in known))
        self.assertTrue(all(r['owners'] == ['one', 'two'] for r in known))
        self.assertTrue(all(r['source'] == ['src/a.cpp', 'src/b.cpp'] for r in known))
        self.assertEqual(sum(r['size'] for r in known if r['domain'] == 'file'), 20)

    def test_duplicate_claims_and_reviewed_partial_sharing_are_compatible(self):
        a = claim(0x1000, 20, 'vtable', 'one', 'a')
        b = claim(0x1000, 20, 'vtable', 'two', 'b')
        self.assertTrue(compatible([a, b]))
        b['start'] += 4
        self.assertFalse(compatible([a, b]))
        a['shared_group'] = b['shared_group'] = 'reviewed-shared-array'
        self.assertTrue(compatible([a, b]))
        b['shared_group'] = 'different'
        self.assertFalse(compatible([a, b]))

    def test_contradictory_overlap_is_not_silently_identified(self):
        claims = [claim(0x1000, 20, 'vtable', 'one'), claim(0x1010, 20, 'vtable', 'two')]
        rows = self.rows(claims)
        conflicts = [r for r in rows if r['domain'] == 'image' and r['category'] == 'conflict']
        self.assertEqual(sum(r['size'] for r in conflicts), 4)
        self.assertEqual(conflicts[0]['evidence'], ['one', 'two'])
        audit_partition(rows, 'image', self.layout.image_size)

    def test_labels_and_heuristics_never_establish_extents(self):
        labels = {0x1000: [dict(name='large_candidate_array', source='src/a.cpp')]}
        guesses = [claim(0x1000, 40, 'string-candidate', 'guess', confidence='provisional')]
        rows = self.rows(guesses, labels)
        self.assertFalse(any(r['category'] == 'identified' for r in rows))
        self.assertTrue(any(r['category'] == 'provisional' for r in rows))
        self.assertTrue(all(not r['owners'] for r in rows))
        proven = claim(0x1004, 8, 'object', 'retail')
        rows = self.rows([*guesses, proven], labels)
        self.assertEqual(sum(r['size'] for r in rows if r['domain'] == 'image' and r['category'] == 'identified'), 8)

    def test_extent_outside_storage_rejected_but_zero_fill_permitted(self):
        claims = [claim(0x3000, 4, 'object', 'evidence'), claim(0x2100, 0x200, 'object', 'evidence'),
                  claim(0x2f00, 0x101, 'object', 'evidence'), claim(0x1100, 0, 'object', 'evidence')]
        errors = validate_claims(self.layout, claims)
        self.assertEqual([e['claim'] for e in errors], [2, 3])

    def test_file_only_overlay_claim_is_not_projected_to_rva(self):
        rows = self.rows([claim(0x800, 16, 'certificate', 'PE security directory', domain='file')])
        known = [r for r in rows if r['category'] == 'identified']
        self.assertEqual(len(known), 1)
        self.assertEqual(known[0]['domain'], 'file')
        self.assertIsNone(known[0]['rva'])

    def test_auditor_rejects_hole_overlap_and_wrong_denominator(self):
        rows = self.rows()
        for mutation in ('hole', 'overlap', 'total'):
            changed = [dict(r) for r in rows]
            if mutation == 'hole':
                changed[0]['end'] -= 1
                changed[0]['size'] -= 1
            if mutation == 'overlap':
                changed[1]['start'] -= 1
                changed[1]['size'] += 1
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                audit_partition(changed, 'file', len(self.layout.data) + (mutation == 'total'))

    def test_reference_anchor_does_not_claim_whole_neighbor_span(self):
        refs = [dict(site_rva=0x2000, target_rva=0x1001, admitted=True),
                dict(site_rva=0x2004, target_rva=0x1002, admitted=False)]
        rows = self.rows(refs=refs)
        self.assertEqual(sum(r['incoming_references'] for r in rows if r['domain'] == 'image'), 1)
        self.assertTrue(all(r['category'] == 'unknown' for r in rows))

    def test_backlog_deduplicates_mapped_file_bytes_and_keeps_overlay(self):
        report = dict(rows=self.rows())
        backlog = backlog_rows(report)
        self.assertTrue(any(r['storage'] == 'overlay' for r in backlog))
        self.assertTrue(all(r['domain'] == 'image' or r['rva'] is None for r in backlog))
        self.assertEqual(sum(r['size'] for r in backlog), self.layout.image_size + 0x300)

    def test_tsv_is_standalone_and_escapes_multiline_string_leads(self):
        claims = [claim(0x1000, 8, 'string-candidate', 'guess', 'a\tb\nc', confidence='provisional')]
        report = dict(rows=self.rows(claims), claims=claims, labels={}, references=[], problems=[])
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            export(report, directory)
            for name in ('claims.tsv', 'coverage.tsv'):
                entries = tsv.read(directory / name)[2]
                self.assertTrue(entries)
            entries = tsv.read(directory / 'claims.tsv')[2]
            self.assertEqual(json.loads(entries[0]['owner']), 'a\tb\nc')
            row = next(r for r in tsv.read(directory / 'coverage.tsv')[2] if r['category'] == 'provisional')
            self.assertIn('guess', row['evidence'])
            self.assertTrue(row['next_action'])
