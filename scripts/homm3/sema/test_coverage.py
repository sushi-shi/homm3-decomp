"""Negative controls for independent retail data coverage and PE boundaries."""
import struct
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest

from homm3.sema.coverage import generate, partition, pe_regions, system_claims
from homm3.sema.__main__ import _build_parser


def region(start=100, end=120):
    return dict(name='.data', section='.data', storage='initialized', rva=start, end=end)


def claim(start, size, category='object'):
    return dict(rva=start, size=size, category=category, evidence='test')


def pe_fixture():
    data = bytearray(0x900)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 0x3c, 0x80)
    data[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<H', data, 0x86, 3)
    struct.pack_into('<H', data, 0x94, 224)
    struct.pack_into('<H', data, 0x98, 0x10b)
    for i, (name, virtual, rva, raw, disk) in enumerate([
        (b'.rdata', 0x100, 0x1000, 0x200, 0x400),
        (b'.data', 0x300, 0x2000, 0x200, 0x600),
        (b'.bss', 0x80, 0x3000, 0, 0),
    ]):
        offset = 0x178 + 40 * i
        data[offset:offset + len(name)] = name
        struct.pack_into('<4I', data, offset + 8, virtual, rva, raw, disk)
    return data


class CoverageTest(unittest.TestCase):
    def test_leading_trailing_and_anchored_gaps_remain_unknown(self):
        spans, errors = partition([region()], [claim(105, 5)], [102, 115])
        self.assertFalse(errors)
        self.assertEqual([(s['rva'], s['end'], s['category']) for s in spans],
                         [(100, 102, 'unknown'), (102, 105, 'unknown'),
                          (105, 110, 'object'), (110, 115, 'unknown'),
                          (115, 120, 'unknown')])
        self.assertEqual(sum(s['size'] for s in spans), 20)

    def test_overlaps_count_once_and_are_not_identified_bytes(self):
        spans, errors = partition([region()], [claim(103, 7), claim(108, 5)])
        self.assertFalse(errors)
        self.assertEqual(sum(s['size'] for s in spans), 20)
        self.assertEqual(sum(s['size'] for s in spans if s['category'] == 'overlap'), 2)
        self.assertEqual(next(s['claims'] for s in spans if s['category'] == 'overlap'), [0, 1])

    def test_out_of_bounds_and_nonpositive_extents_stay_visible(self):
        spans, errors = partition([region()], [claim(98, 4), claim(119, 3), claim(110, 0)])
        self.assertEqual(len(errors), 3)
        self.assertEqual(sum(s['size'] for s in spans), 20)

    def test_adjacent_claims_and_provisional_extents(self):
        spans, errors = partition([region()], [claim(100, 10), claim(110, 10, 'provisional')])
        self.assertFalse(errors)
        self.assertEqual([s['category'] for s in spans], ['object', 'provisional'])

    def test_regions_include_raw_tail_zero_fill_and_standalone_bss(self):
        sections, regions, optional = pe_regions(pe_fixture())
        self.assertEqual([(r['rva'], r['end'], r['storage']) for r in regions],
                         [(0x1000, 0x1100, 'initialized'), (0x1100, 0x1200, 'raw-tail'),
                          (0x2000, 0x2200, 'initialized'), (0x2200, 0x2300, 'zero-fill'),
                          (0x3000, 0x3080, 'zero-fill')])
        spans, _ = partition(regions, [])
        self.assertEqual(sum(s['size'] for s in spans), 0x580)
        self.assertTrue(all(s['category'] == 'unknown' for s in spans))
        self.assertEqual(system_claims(pe_fixture(), sections, optional), [])

    def test_truncated_raw_data_is_rejected(self):
        with self.assertRaises(ValueError):
            pe_regions(pe_fixture()[:0x700])

    def test_import_structures_include_terminators_without_string_padding(self):
        data = pe_fixture()
        optional = 0x98
        struct.pack_into('<I', data, optional + 92, 16)
        struct.pack_into('<II', data, optional + 104, 0x1000, 40)
        struct.pack_into('<5I', data, 0x400, 0x1040, 0, 0, 0x1060, 0x1050)
        struct.pack_into('<II', data, 0x440, 0x1070, 0)
        struct.pack_into('<II', data, 0x450, 0x1070, 0)
        data[0x460:0x464] = b'x\0xx'
        data[0x470:0x477] = b'\0\0func\0'
        sections, regions, _ = pe_regions(data)
        claims = system_claims(data, sections, optional)
        self.assertIn(dict(rva=0x1060, size=2, category='system',
                           evidence='PE import DLL name including NUL'), claims)
        self.assertEqual(sum(c['size'] for c in claims), 65)
        spans, errors = partition(regions, claims)
        self.assertFalse(errors)
        self.assertFalse(any(s['category'] == 'overlap' for s in spans))
        struct.pack_into('<II', data, optional + 104, 0x1000, 20)
        with self.assertRaisesRegex(ValueError, 'terminator'):
            system_claims(data, sections, optional)

    def test_cli_can_enforce_full_identification(self):
        args = _build_parser().parse_args(['coverage', '--require-complete', '--output', 'build/coverage'])
        self.assertTrue(args.require_complete)
        self.assertEqual(args.output, 'build/coverage')

    def test_labels_never_establish_extents_and_held_references_survive(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            config = root / 'config/retail'
            config.mkdir(parents=True)
            (config / 'vtables.tsv').write_text('rva\tfunction_count\tclass\n')
            (config / 'data-extents.tsv').write_text('rva\tsize\tcategory\tevidence\n')
            (config / 'relocs.tsv').write_text('site_rva\tkind\n0x2000\tdir32\n')
            (config / 'reloc-evidence.tsv').write_text(
                'site_rva\tvalue\tdisposition\n0x2004\t0x401010\theld\n')
            gen = root / 'build/gen'
            gen.mkdir(parents=True)
            (gen / 'symbol_names.csv').write_text(
                'rva,name,unit,size,kind,provenance\n0x1010,label,u,0x100,data,src-DATA\n')
            data = pe_fixture()
            struct.pack_into('<II', data, 0x600, 0x401010, 0x401010)
            image = SimpleNamespace(data=data, image_base=0x400000, image_end=0x404000)
            report = generate(root, image)
            self.assertEqual(report['total_bytes'], 0x580)
            self.assertEqual(report['bytes_by_category']['unknown'], 0x580)
            self.assertEqual(len(report['references']), 2)
            self.assertEqual(report['references_into_unknown'], 1)
            self.assertEqual(report['unknown_spans_with_incoming_references'], 1)
            self.assertEqual(report['references'][1]['evidence']['disposition'], 'held')
            self.assertIn(dict(site_rva=0x2004, target_rva=0x1010, aligned=True),
                          report['pointer_candidates'])
            self.assertFalse(any(p['site_rva'] == 0x2000 for p in report['pointer_candidates']))
            self.assertIsNone(report['comparison']['verified_pointer_fields'])
