"""Vendor contribution proof must survive byte, address and provenance defects."""
from pathlib import Path
import struct
import hashlib
import json
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from homm3.analysis import vendor_data as vendor
from homm3.build.test_eh_handler_normalization import FixtureSection, _coff, _symbol, _section_aux
from homm3.sema.retail_layout import Layout
from homm3.sema.test_retail_layout import fixture


def obj(sections, symbols):
    payload = bytearray(_coff(tuple(sections), tuple(symbols)))
    for i, section in enumerate(sections):
        flags = 0x20 if section.name.startswith('.text') else 0x80 if section.name == '.bss' else 0x40
        struct.pack_into('<I', payload, 20 + i * 40 + 36, flags)
    return vendor.load_object('test.lib', 'test.obj', 'test.lib', bytes(payload))


def put(data, rva, blob):
    layout = Layout(data)
    offset = layout.raw(rva, len(blob))
    data[offset:offset + len(blob)] = blob


class VendorMatchTest(unittest.TestCase):
    def setup_object(self, *, data_raw=b'0123456789abcdef', addend=2, symbol_offset=4):
        raw = b'abcdefgh' + struct.pack('<i', addend) + b'\xc3'
        candidate = obj([FixtureSection('.text', raw, ((8, 1, 6),)),
                         FixtureSection('.data', data_raw, ())],
                        [_symbol('_func', 0, 1, 0x20, 2), _symbol('_data', symbol_offset, 2, 0, 2)])
        retail = fixture()
        put(retail, 0x1000, raw[:8] + struct.pack('<I', 0x402010 + symbol_offset + addend) + raw[12:])
        put(retail, 0x2010, data_raw)
        return retail, candidate

    def match(self, retail, candidate, seeds=None):
        return vendor.match(Layout(retail), [candidate], seeds if seeds is not None else {('', '_func'): {0x1000}})

    def test_whole_data_section_uses_real_addend_and_symbol_offset(self):
        retail, candidate = self.setup_object()
        rows, _, _ = self.match(retail, candidate)
        self.assertEqual([(r['rva'], r['size'], r['status']) for r in rows], [(0x2010, 16, 'verified')])
        self.assertEqual(len(rows[0]['anchor_path']), 2)
        self.assertIn('_data+0x4', rows[0]['symbols'])

    def test_changed_initializer_is_rejected_and_no_seed_means_no_search(self):
        retail, candidate = self.setup_object()
        self.assertFalse(self.match(retail, candidate, {})[0])
        put(retail, 0x2018, b'!')
        self.assertEqual(self.match(retail, candidate)[0][0]['status'], 'rejected')

    def test_wrong_code_or_relocation_type_cannot_seed_storage(self):
        retail, candidate = self.setup_object()
        put(retail, 0x1000, b'!')
        rows, issues, _ = self.match(retail, candidate)
        self.assertFalse(rows)
        self.assertEqual(issues[0]['kind'], 'unmatched-code-anchor')
        with self.assertRaises(ValueError):
            vendor.compare(Layout(retail), 0x1000, b'x' * 12,
                           [SimpleNamespace(site=0, typ=99)])

    def test_common_size_requires_reference_and_checks_entire_extent(self):
        raw = b'abcdefgh' + bytes(4) + b'\xc3'
        candidate = obj([FixtureSection('.text', raw, ((8, 1, 6),))],
                        [_symbol('_func', 0, 1, 0x20, 2), _symbol('_buffer', 32, 0, 0, 2)])
        retail = fixture()
        put(retail, 0x1000, raw[:8] + struct.pack('<I', 0x403040) + raw[12:])
        rows = self.match(retail, candidate)[0]
        self.assertEqual([(r['rva'], r['size'], r['storage'], r['status']) for r in rows],
                         [(0x3040, 32, 'bss', 'verified')])
        put(retail, 0x1008, struct.pack('<I', 0x403070))
        self.assertEqual(self.match(retail, candidate)[0][0]['status'], 'rejected')

    def test_section_symbol_auxiliary_record_still_resolves_storage(self):
        raw = b'abcdefgh' + bytes(4) + b'\xc3'
        candidate = obj([FixtureSection('.text', raw, ((8, 1, 6),)),
                         FixtureSection('.data', b'12345678', ())],
                        [_symbol('_func', 0, 1, 0x20, 2),
                         _symbol('.data', 0, 2, 0, 3, _section_aux(8, 0))])
        retail = fixture()
        put(retail, 0x1000, raw[:8] + struct.pack('<I', 0x402000) + raw[12:])
        put(retail, 0x2000, b'12345678')
        self.assertEqual(self.match(retail, candidate)[0][0]['status'], 'verified')

    def test_debug_records_are_not_linkable_data_contributions(self):
        raw = b'abcdefgh' + bytes(4) + b'\xc3'
        candidate = obj([FixtureSection('.text', raw, ((8, 1, 6),)),
                         FixtureSection('.debug$F', b'12345678', ())],
                        [_symbol('_func', 0, 1, 0x20, 2), _symbol('_debug', 0, 2, 0, 2)])
        retail = fixture()
        put(retail, 0x1000, raw[:8] + struct.pack('<I', 0x402000) + raw[12:])
        put(retail, 0x2000, b'12345678')
        rows, _, summary = self.match(retail, candidate)
        self.assertFalse(rows)
        self.assertEqual(summary['libraries']['test.lib']['data_contributions'], 0)

    def test_unresolved_pointer_table_is_candidate_not_verified(self):
        raw = b'abcdefgh' + bytes(4) + b'\xc3'
        candidate = obj([FixtureSection('.text', raw, ((8, 1, 6),)),
                         FixtureSection('.data', bytes(4), ((0, 2, 6),))],
                        [_symbol('_func', 0, 1, 0x20, 2), _symbol('_table', 0, 2, 0, 2),
                         _symbol('_missing', 0, 0, 0, 2)])
        retail = fixture()
        put(retail, 0x1000, raw[:8] + struct.pack('<I', 0x402000) + raw[12:])
        put(retail, 0x2000, struct.pack('<I', 0x403000))
        row = self.match(retail, candidate)[0][0]
        self.assertEqual(row['status'], 'candidate')
        self.assertEqual(row['unresolved_relocations'], ['0x0:_missing'])
        seeds = {('', '_func'): {0x1000}, ('', '_missing'): {0x3040}}
        self.assertEqual(self.match(retail, candidate, seeds)[0][0]['status'], 'conflict')

    def test_conflicting_code_placements_cannot_launder_identical_child(self):
        retail, candidate = self.setup_object()
        put(retail, 0x1080, bytes(retail[0x400:0x40d]))
        rows = self.match(retail, candidate, {('', '_func'): {0x1000, 0x1080}})[0]
        self.assertEqual(rows[0]['status'], 'candidate')
        self.assertFalse(rows[0]['anchor_path'])

    def test_rel32_and_dir32nb_preserve_signed_addends(self):
        retail = fixture()
        raw = struct.pack('<i', -4)
        put(retail, 0x1000, struct.pack('<i', 0x2010 - 4 - (0x1000 + 4)))
        self.assertEqual(vendor.relocation_target(Layout(retail), 0x1000, raw,
                         SimpleNamespace(typ=20, site=0)), 0x2010)
        put(retail, 0x1000, struct.pack('<I', 0x2010 - 4))
        self.assertEqual(vendor.relocation_target(Layout(retail), 0x1000, raw,
                         SimpleNamespace(typ=7, site=0)), 0x2010)

    def test_long_archive_names_and_truncated_members(self):
        def member(name, data):
            header = f'{name:<16}{0:<12}{0:<6}{0:<6}{0:<8}{len(data):<10}`\n'.encode()
            return header + data + (b'\n' if len(data) & 1 else b'')
        archive = b'!<arch>\n' + member('//', b'long/library/member.obj\0') + member('/0', b'object')
        entries = list(vendor.archive_members(archive))
        self.assertEqual(entries[0][1:], ('long/library/member.obj', b'object'))
        with self.assertRaises(ValueError):
            list(vendor.archive_members(archive[:-1]))

    def test_zlib_fingerprint_tracks_source_headers_flags_and_compiler(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            paths = ['config/units.toml', 'scripts/homm3/core/cc_wrap.py', 'include/test.h',
                     'vendor/zlib-1.1.3/test.c', 'toolchain/bin/C2.DLL', 'toolchain/include/STDIO.H']
            for p in paths:
                path = root / p
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('initial')
            project = SimpleNamespace(root=root, includes=[root/'include'], toolchain=root/'toolchain')
            key = vendor.zlib_key(project)
            for p in paths:
                (root/p).write_text('changed')
                new = vendor.zlib_key(project)
                self.assertNotEqual(key, new, p)
                key = new

    def test_missing_archives_and_tampered_objects_remain_explicit(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for p, value in {'config/units.toml': '', 'scripts/homm3/core/cc_wrap.py': '',
                             'vendor/zlib-1.1.3/test.c': '',
                             'config/retail/runtime-map.tsv': 'rva\tname\n',
                             'config/retail/zlib-map.tsv': 'rva\tname\tunit\n'}.items():
                path = root / p
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(value)
            project = SimpleNamespace(root=root, includes=[], toolchain=root/'missing',
                                      manifest={'unit': [dict(unit='test', source='vendor/zlib-1.1.3/test.c')]})
            with patch.object(vendor, 'Project', return_value=project):
                objects, _, issues, _ = vendor.inputs(root)
                self.assertFalse(objects)
                self.assertEqual([i['kind'] for i in issues],
                                 ['missing-library', 'missing-library', 'missing-or-stale-objects'])
                directory = root / 'build/gen/vendor-data'
                directory.mkdir(parents=True)
                _, candidate = self.setup_object()
                payload = candidate.coff.data
                (directory/'test.obj').write_bytes(payload)
                (directory/'zlib.json').write_text(json.dumps(dict(fingerprint=vendor.zlib_key(project),
                    objects={'test': hashlib.sha256(payload).hexdigest()})))
                self.assertEqual(len(vendor.inputs(root)[0]), 1)
                (directory/'test.obj').write_bytes(payload + b'corruption')
                objects, _, issues, _ = vendor.inputs(root)
                self.assertFalse(objects)
                self.assertEqual(issues[-1]['kind'], 'missing-or-stale-objects')
