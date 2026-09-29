import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace

from homm3.retail_labels.data import _uncached_declarations


class LocalDataTypesTest(unittest.TestCase):
    def parse(self, source, header=None):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = root / 'probe.cpp'
            path.write_text('#define DATA(x) __attribute__((annotate("data:" #x)))\n'
                            + source)
            if header is not None:
                (root / 'dependency.h').write_text(header)
            profiles = SimpleNamespace(
                project=SimpleNamespace(root=root, image=SimpleNamespace(image_base=0x400000)),
                for_source=lambda _: ['-x', 'c++', '-std=c++98',
                                       '--target=i686-pc-windows-msvc', '-fms-extensions'])
            return _uncached_declarations(path, profiles)

    def test_unrelated_body_error_keeps_clean_local_type(self):
        facts, errors = self.parse('''
void broken() { missing(); }
void clean() { DATA(0x00401000) static int values[3] = { 1, 2, 3 }; }
''')
        self.assertEqual(facts[0x1000]['size'], 12)
        self.assertEqual(facts[0x1000]['type'], 'int[3]')
        self.assertTrue(any('missing' in error for error in errors))

    def test_error_in_own_body_withholds_its_declaration_only(self):
        facts, errors = self.parse('''
void broken() { DATA(0x00401000) static int values[3] = { 1, 2, 3 }; missing(); }
void clean() { DATA(0x00402000) static short values[2] = { 4, 5 }; }
''')
        self.assertNotIn(0x1000, facts)
        self.assertEqual(facts[0x2000]['size'], 4)
        self.assertTrue(errors)

    def test_header_error_does_not_certify_local_types(self):
        facts, errors = self.parse('''
#include "dependency.h"
void clean() { DATA(0x00401000) static int values[3] = { 1, 2, 3 }; }
''', 'struct Broken { Unknown value; };\n')
        self.assertFalse(facts)
        self.assertTrue(errors)

    def test_signature_error_does_not_certify_local_types(self):
        facts, errors = self.parse('''
void broken(Unknown value) {}
void clean() { DATA(0x00401000) static int values[3] = { 1, 2, 3 }; }
''')
        self.assertFalse(facts)
        self.assertTrue(errors)

    def test_clean_declaration_remains_source_owned(self):
        facts, errors = self.parse('''
void clean() { DATA(0x00401000) static unsigned char values[5] = { 1 }; }
''')
        self.assertFalse(errors)
        self.assertEqual(facts[0x1000]['size'], 5)
        self.assertTrue(facts[0x1000]['source'].startswith('probe.cpp:'))

    def test_static_in_anonymous_namespace_takes_cl_c_name(self):
        # cl 12 names `static` data in a source file's anonymous namespace
        # `_name` (retail rmg.obj); without `static` the scope stays mangled.
        facts, errors = self.parse('''
namespace {
DATA(0x00401000) static const int table[4] = { 2, 0, 3, 1 };
DATA(0x00402000) int plain[2] = { 1, 2 };
}
namespace named { DATA(0x00403000) static int inner = 1; }
DATA(0x00404000) static int fileStatic = 1;
''')
        self.assertFalse(errors)
        self.assertEqual(facts[0x1000]['name'], '_table')
        self.assertIn('?A0x', facts[0x2000]['name'])
        self.assertNotEqual(facts[0x3000]['name'], '_inner')
        self.assertEqual(facts[0x4000]['name'], '_fileStatic')


class StaticGuardExtentTest(unittest.TestCase):
    def test_adjacent_local_static_guards_own_one_byte_each(self):
        from homm3.retail_labels.source import scan_file
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'probe.cpp'
            path.write_text("""
DATA_COMPGEN_GUARD(0x00698b98, firstGuard, first)
DATA_COMPGEN_GUARD(0x00698b99, secondGuard, second)
DATA_COMPGEN_GUARD(0x00698b9a, thirdGuard, third)
""")
            rows = scan_file(path, set())
        self.assertEqual([(r['rva'], r['size']) for r in rows],
                         [(0x298b98, 1), (0x298b99, 1), (0x298b9a, 1)])


if __name__ == '__main__':
    unittest.main()
