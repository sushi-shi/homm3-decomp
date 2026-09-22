"""DATA annotation/type binding, definition accounting and cache invalidation."""
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest

from homm3.analysis import data_declarations as data


HEADER = '#define DATA(a) __attribute__((annotate("data:" #a)))\n'


class ExtractionTest(unittest.TestCase):
    def parse(self, root, source, name='a.cpp'):
        path = root / 'src' / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(HEADER + source)
        return data.parse_unit(dict(root=str(root), source='src/' + name, unit=path.stem,
                                    image_base=0x400000, args=['--target=i686-pc-windows-msvc',
                                    '-fms-extensions', '-std=c++11']))

    def test_complete_incomplete_inferred_arrays_and_reference_storage(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            unit = self.parse(root, '''
DATA(0x401000) extern int declared[8];
DATA(0x401100) extern int unknown[];
DATA(0x401200) int inferred[] = {1,2,3};
DATA(0x401300) extern int (&reference)[8];
void f() { DATA(0x401400) static int local[3]; DATA(0x401500) int automatic; }
''')
            self.assertFalse(unit['errors'])
            facts = {f['name']: f for f in unit['facts']}
            self.assertEqual(facts['declared']['size'], 32)
            self.assertIsNone(facts['unknown']['size'])
            self.assertEqual(facts['inferred']['size'], 12)
            self.assertEqual(facts['reference']['size'], 4)
            self.assertTrue(facts['reference']['reference_cell'])
            self.assertTrue(facts['local']['static_storage'])
            self.assertFalse(facts['automatic']['static_storage'])
            result = data.summarize([unit], data.annotation_sites(root, 0x400000))
            self.assertFalse(any(i['kind'] == 'annotation-unbound' for i in result['issues']))
            self.assertEqual({i['kind'] for i in result['issues']}, {'unknown-size', 'automatic-storage'})

    def test_utf8_before_annotation_uses_byte_offsets(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            unit = self.parse(root, '// café\nDATA(0x401000) int value;')
            result = data.summarize([unit], data.annotation_sites(root, 0x400000))
            self.assertFalse(result['issues'])
            self.assertTrue(result['analysis_complete'])

    def test_packing_and_pointer_width_follow_compiler_layout(self):
        with tempfile.TemporaryDirectory() as directory:
            unit = self.parse(Path(directory), '''
#pragma pack(push, 1)
struct Packed { char a; int b; };
#pragma pack(pop)
DATA(0x401000) Packed value;
DATA(0x401100) int* pointer;
''')
            self.assertEqual([f['size'] for f in unit['facts']], [5, 4])

    def test_unannotated_definition_completes_extern_without_inventing_definition(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            a = self.parse(root, 'DATA(0x401000) extern int values[];', 'a.cpp')
            b = self.parse(root, 'int values[4];', 'b.cpp')
            report = data.summarize([a, b], data.annotation_sites(root, 0x400000))
            row = report['declarations'][0]
            self.assertEqual(row['size'], 16)
            self.assertTrue(row['definition'])
            self.assertEqual(row['definition_units'], ['b'])
            report = data.summarize([a], data.annotation_sites(root, 0x400000))
            self.assertIsNone(report['declarations'][0]['size'])
            self.assertFalse(report['declarations'][0]['definition'])

    def test_repeated_header_site_deduplicates_but_disagreement_is_retained(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'include').mkdir()
            (root / 'include/common.h').write_text(HEADER + 'DATA(0x401000) extern int shared[4];\n')
            a = self.parse(root, '#include "../include/common.h"', 'a.cpp')
            b = self.parse(root, '#include "../include/common.h"', 'b.cpp')
            result = data.summarize([a, b], data.annotation_sites(root, 0x400000))
            self.assertEqual(len(result['declarations']), 1)
            self.assertEqual(result['declarations'][0]['units'], ['a', 'b'])
            b['facts'][0]['size'] = 20
            result = data.summarize([a, b], data.annotation_sites(root, 0x400000))
            self.assertEqual(result['declarations'][0]['status'], 'conflicting-size')
            self.assertIsNone(result['declarations'][0]['size'])

    def test_invalid_parse_never_appears_as_successful_empty_unit(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            unit = self.parse(root, 'DATA(0x401000) missing_type storage;')
            self.assertTrue(unit['errors'])
            result = data.summarize([unit], data.annotation_sites(root, 0x400000))
            self.assertFalse(result['analysis_complete'])
            self.assertEqual({i['kind'] for i in result['issues']}, {'parse-failure', 'annotation-unbound'})

    def test_lexical_scanner_ignores_comments_and_retains_inactive_and_compgen(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'src').mkdir()
            (root / 'src/a.cpp').write_text('''// DATA(0x401000)
const char* s = "DATA(0x401100)";
#if 0
DATA(0x401200) int inactive;
#endif
DATA_COMPGEN(0x401300, string, "a,b");
''')
            sites = data.annotation_sites(root, 0x400000)
            self.assertEqual([s['rva'] for s in sites], [0x1200, 0x1300])
            result = data.summarize([], sites)
            self.assertEqual({i['kind'] for i in result['issues']}, {'annotation-unbound', 'compgen-extent-unbound'})

    def test_fingerprint_changes_with_source_header_profile_and_implementation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            paths = ['src/a.cpp', 'include/a.h', 'include/types.inc', 'vendor/a.h', 'config/project.toml', 'config/units.toml',
                     'scripts/homm3/analysis/data_declarations.py', 'scripts/homm3/retail_labels/source.py',
                     'scripts/homm3/core/compiler_profile.py', 'scripts/homm3/core/clang.py', 'scripts/homm3/core/project.py']
            for name in paths:
                p = root / name
                p.parent.mkdir(parents=True, exist_ok=True)
                p.write_text('original')
            profiles = SimpleNamespace(includes=[root / 'include'])
            previous = data.fingerprint(root, profiles, [])
            for name in paths:
                (root / name).write_text('changed')
                current = data.fingerprint(root, profiles, [])
                self.assertNotEqual(current, previous, name)
                previous = current
            self.assertNotEqual(previous, data.fingerprint(root, profiles, [{'args': ['/Zp1']}]))
