"""HoMM3 input adapters for the Gruntz data model."""
import os
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from homm3.core import msvc_names
from homm3.match import status
from homm3.retail_labels import data


class InputAdapterTests(unittest.TestCase):
    def test_vc6_array_and_file_static_names(self):
        self.assertEqual(msvc_names.data('?table@@3QBHB', internal=False,
                                         decorated=True), '?table@@3QBHB')
        self.assertEqual(msvc_names.data('_g_table', internal=True,
                                         decorated=True), '_g_table')
        self.assertEqual(msvc_names.mask('_local$S123'), '_local$S')
        self.assertNotEqual(msvc_names.mask('$S123'), msvc_names.mask('$S124'))

    def test_score_policy_reset_preserves_history_and_source_hash(self):
        row = status.MatchRow(95, 100, 100, 0x1234, 'tokens1:unchanged')
        old = {('test', 'f'): row}
        reset = status.policy_baseline(old, '# old policy\n')
        self.assertEqual(reset[('test', 'f')],
                         status.MatchRow(None, 0, 100, 0x1234, 'tokens1:unchanged'))
        self.assertEqual(status.policy_baseline(old,
            f'# score_policy={status.SCORE_POLICY}\n'), old)

    def test_type_cache_tracks_content_not_mtime_and_retains_errors(self):
        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / 'sample.cpp'; source.write_text('int x;')
            profiles = SimpleNamespace(project=SimpleNamespace(root=root),
                                       for_source=lambda _path: ['-m32'])
            answer = ({123: {'size': 4}}, ['explicit diagnostic'])
            with patch.object(data, '_uncached_declarations', return_value=answer) as parse:
                self.assertEqual(data.declarations(source, profiles), answer)
                self.assertEqual(data.declarations(source, profiles), answer)
                self.assertEqual(parse.call_count, 1)
                before = source.stat()
                source.write_text('int y;')
                os.utime(source, ns=(before.st_atime_ns, before.st_mtime_ns))
                self.assertEqual(data.declarations(source, profiles), answer)
                self.assertEqual(parse.call_count, 2)


class StringStorageTests(unittest.TestCase):
    def test_typed_storage_excludes_numeric_and_aggregate_interiors(self):
        from homm3.delink import data_manifest
        types = {0x1000: 'const int[2]', 0x2000: 'const ButtonRect[5]',
                 0x3000: 'char[8]', 0x4000: 'const char[8]'}
        bindings = [SimpleNamespace(rva=rva, size=8, channel='src')
                    for rva in [*types, 0x5000]]
        bindings.append(SimpleNamespace(rva=0x6000, size=8, channel='src_data_compgen'))
        types[0x6000] = 'int[2]'
        self.assertEqual(data_manifest.nonliteral_storage(
            SimpleNamespace(data=bindings), types),
            [(0x1000, 0x1008), (0x2000, 0x2008), (0x3000, 0x3008)])
        candidate = SimpleNamespace(iter_symbols=lambda: [(0, 0, 1)],
                                    sym_name=lambda _: '??_C@comma',
                                    cstring=lambda *_: b',')
        retail = SimpleNamespace(cstring=lambda _: b',')
        with patch.object(data_manifest.coffx, 'objects', return_value=[('window', candidate)]), \
             patch.object(data_manifest, 'retail', return_value=retail), \
             patch.object(data_manifest, '_reloc_data_rvas', return_value=[0x1004, 0x2002, 0x7000]), \
             patch.object(data_manifest, '_classify', return_value='data-initialized'):
            rows, withheld = data_manifest.string_rows(nonliteral_ranges=
                data_manifest.nonliteral_storage(SimpleNamespace(data=bindings), types))
            self.assertEqual([(r['rva'], r['size']) for r in rows], [(0x7000, 2)])
            self.assertEqual(withheld, [])

    def test_mutable_array_does_not_collide_with_a_separate_literal(self):
        from homm3.delink import data_manifest
        candidate = SimpleNamespace(iter_symbols=lambda: [(0, 0, 1)],
                                    sym_name=lambda _: '??_C@caption',
                                    cstring=lambda *_: b'Caption')
        retail = SimpleNamespace(cstring=lambda _: b'Caption')
        with patch.object(data_manifest.coffx, 'objects', return_value=[('window', candidate)]), \
             patch.object(data_manifest, 'retail', return_value=retail), \
             patch.object(data_manifest, '_reloc_data_rvas', return_value=[0x1000, 0x2000]), \
             patch.object(data_manifest, '_classify', return_value='data-initialized'):
            rows, withheld = data_manifest.string_rows(nonliteral_ranges=[(0x1000, 0x1008)])
            self.assertEqual([(r['rva'], r['size']) for r in rows], [(0x2000, 8)])
            self.assertEqual(withheld, [])
            # Without a proven mutable owner, duplicate addresses remain ambiguous.
            rows, withheld = data_manifest.string_rows()
            self.assertEqual(rows, [])
            self.assertEqual(len(withheld), 2)


if __name__ == '__main__':
    unittest.main()
