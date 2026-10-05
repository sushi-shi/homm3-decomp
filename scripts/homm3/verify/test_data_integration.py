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
        # Clang's `B` storage class on a reference variable is cl 6's `A`.
        self.assertEqual(msvc_names.data(
            '?g_traits@@3AAY0JA@$$CBUTTraits@@B', internal=False,
            decorated=True), '?g_traits@@3AAY0JA@$$CBUTTraits@@A')
        self.assertEqual(msvc_names.data('?g_ptr@@3PBHB', internal=False,
                                         decorated=True), '?g_ptr@@3PBHB')
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

    def test_type_cache_reparses_only_consumers_of_an_edited_header(self):
        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'include').mkdir()
            used = root / 'include/used.h'; used.write_text('int x;')
            other = root / 'include/other.h'; other.write_text('int y;')
            source = root / 'sample.cpp'; source.write_text('#include "used.h"')
            answer = ({123: {'size': 4}}, [])

            def parse(_path, _profiles, reached=None):
                reached.add(str(used))
                return answer

            def run():
                # A fresh command: profiles memoize content hashes per command.
                profiles = SimpleNamespace(project=SimpleNamespace(root=root),
                                           for_source=lambda _path: ['-m32'])
                return data.declarations(source, profiles)
            with patch.object(data, '_uncached_declarations', side_effect=parse) as parsed:
                self.assertEqual(run(), answer)
                self.assertEqual(run(), answer)
                self.assertEqual(parsed.call_count, 1)
                other.write_text('int y2;')  # not read by this TU
                self.assertEqual(run(), answer)
                self.assertEqual(parsed.call_count, 1)
                used.write_text('int x2;')
                self.assertEqual(run(), answer)
                self.assertEqual(parsed.call_count, 2)
                (root / 'include/added.h').write_text('')  # may change resolution
                self.assertEqual(run(), answer)
                self.assertEqual(parsed.call_count, 3)


def literal_object(name, payload, raw=True, section='.data'):
    """A candidate COFF holding one `??_C@` COMDAT at offset 0."""
    return SimpleNamespace(iter_symbols=lambda: [(0, 0, 1)],
                           sym_name=lambda _: name,
                           section_table=[{'size': len(payload), 'name': section}],
                           section_payload=lambda _: payload if raw else b'')


def retail_image(data):
    data = bytes(data)

    def cstring(rva):
        end = data.find(b'\0', rva)
        return data[rva:end] if end >= 0 else None
    return SimpleNamespace(cstring=cstring, off=lambda rva: rva, data=data)


class StringStorageTests(unittest.TestCase):
    def test_literal_identity_is_its_complete_bytes(self):
        # "\0\1\2" must not answer for "" at a retail NUL, and an all-zero
        # COMDAT without raw data is still the one-byte literal "".
        from homm3.delink import data_manifest
        objects = [('a', literal_object('??_C@binary', b'\0\1\2\0')),
                   ('b', literal_object('??_C@empty', b'\0', raw=False))]
        image = bytearray(0x3000)
        image[0x1000:0x1004] = b'\0\1\2\0'
        with patch.object(data_manifest.coffx, 'objects', return_value=objects), \
             patch.object(data_manifest, 'retail', return_value=retail_image(image)), \
             patch.object(data_manifest, '_reloc_data_rvas', return_value=[0x1000, 0x2000]), \
             patch.object(data_manifest, '_classify', return_value='data-initialized'):
            rows, withheld = data_manifest.string_rows(
                votes={'??_C@empty': {0x2000}})
            self.assertEqual(sorted((r['name'], r['rva'], r['size']) for r in rows),
                             [('??_C@binary', 0x1000, 4), ('??_C@empty', 0x2000, 1)])
            self.assertEqual(withheld, [])

    def test_zero_tail_literal_takes_the_candidate_section_storage(self):
        # cl emits "" as an uninitialized .bss COMDAT; at .data's zero raw
        # edge a paired literal takes the storage cl gave it, never .data.
        from homm3.delink import data_manifest
        for section, storage in (('.bss', 'bss'), ('.data', 'data')):
            objects = [('a', literal_object('??_C@empty', b'\0', raw=False, section=section))]
            image = bytearray(0x3000)
            with patch.object(data_manifest.coffx, 'objects', return_value=objects), \
                 patch.object(data_manifest, 'retail', return_value=retail_image(image)), \
                 patch.object(data_manifest, '_reloc_data_rvas', return_value=[0x2000]), \
                 patch.object(data_manifest, '_classify', return_value='data-unprovable-tail'):
                rows, withheld = data_manifest.string_rows(votes={'??_C@empty': {0x2000}})
                self.assertEqual([(r['rva'], r['storage']) for r in rows], [(0x2000, storage)])
                # Without a paired reference the zero run stays unenrolled.
                rows, withheld = data_manifest.string_rows()
                self.assertEqual(rows, [])

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
        candidate = literal_object('??_C@comma', b',\0')
        image = bytearray(0x8000)
        for rva in (0x1004, 0x2002, 0x7000):
            image[rva:rva + 2] = b',\0'
        retail = retail_image(image)
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
        candidate = literal_object('??_C@caption', b'Caption\0')
        image = bytearray(0x3000)
        for rva in (0x1000, 0x2000):
            image[rva:rva + 8] = b'Caption\0'
        retail = retail_image(image)
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
