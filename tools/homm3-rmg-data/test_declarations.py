"""Contracts for build-only declarations extracted from real Clang ASTs."""
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace
import unittest

from declarations import Declarations
from extract import Sources


class DeclarationTests(unittest.TestCase):
    def setUp(self):
        self.directory = TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        (self.root / 'src').mkdir()
        (self.root / 'include').mkdir()

    def declarations(self, header):
        (self.root / 'include/types.h').write_text(header)
        (self.root / 'src/input.cpp').write_text('#include "../include/types.h"\nint anchor = 0;')
        sources = Sources(self.root, SimpleNamespace(for_source=lambda path: ['-x', 'c++', '-std=c++14']))
        sources.definition('src/input.cpp', 'anchor')
        declarations = Declarations(sources)
        declarations.index(sources.units['src/input.cpp'][0])
        return declarations

    def test_dependencies_precede_fields_and_preserve_native_enum_evaluation(self):
        declarations = self.declarations('''
typedef unsigned char Byte;
#define NATIVE_VALUE 7
enum Kind { First = NATIVE_VALUE, Second, Alias = First << 2 };
struct Record { Byte bytes[Second]; Kind kind; };
''')
        declarations.emit(declarations.select('Record'))
        result = '\n'.join(declarations.output)
        self.assertIn('First = 7,', result)
        self.assertIn('Second = 8,', result)
        self.assertIn('Alias = 28,', result)
        self.assertNotIn('NATIVE_VALUE', result)
        self.assertLess(result.index('typedef unsigned char Byte'), result.index('struct Record'))
        self.assertLess(result.index('enum Kind'), result.index('struct Record'))
        self.assertIn('Byte bytes[Second];', result)
        self.assertIn(self.root / 'include/types.h', declarations.sources.dependencies)

    def test_record_preserves_field_order_and_anonymous_union_but_omits_methods(self):
        declarations = self.declarations('''
class Record {
public:
    int first;
    union { int integer; unsigned char bytes[4]; };
    short last;
    int get() const { return first; }
};
''')
        declarations.emit(declarations.select('Record'))
        result = '\n'.join(declarations.output)
        self.assertIn('union { int integer; unsigned char bytes[4]; };', result)
        self.assertLess(result.index('int first'), result.index('union'))
        self.assertLess(result.index('union'), result.index('short last'))
        self.assertNotIn('get()', result)
        self.assertNotIn('Record(int', result)

    def test_non_plain_records_and_non_integer_domains_fail(self):
        for source, diagnostic in (
                ('struct Base {}; struct Record : Base { int field; };', 'bases'),
                ('struct Record { int field; virtual int get(); };', 'virtual'),
                ('struct Record { int field; virtual ~Record(); };', 'virtual'),
                ('struct Record { int field; Record(int x) : field(x + 1) {} };', 'constructors'),
                ('enum Record : unsigned char { Small = 1 };', 'enum storage'),
                ('enum class Record { Small = 1 };', 'scoped enum')):
            with self.subTest(source=source):
                declarations = self.declarations(source)
                with self.assertRaisesRegex(ValueError, diagnostic):
                    declarations.emit(declarations.select('Record'))

    def test_unknown_root_does_not_silently_omit_a_declaration(self):
        declarations = self.declarations('struct Present { int field; };')
        with self.assertRaisesRegex(ValueError, 'missing native declaration'):
            declarations.select('Missing')

    def test_integer_macro_is_source_owned_and_unsupported_forms_fail(self):
        declarations = self.declarations('#define COUNT 7\n')
        declarations.macro('COUNT')
        self.assertEqual(declarations.output, ['#define COUNT 7\n'])
        with self.assertRaisesRegex(ValueError, 'missing or ambiguous'):
            declarations.macro('MISSING')
        declarations = self.declarations('#define COUNT (3 + 4)\n')
        with self.assertRaisesRegex(ValueError, 'unsupported'):
            declarations.macro('COUNT')

    def test_anonymous_namespace_domains_are_projected_and_collisions_rejected(self):
        declarations = self.declarations('namespace { enum Kind { Value = 9 }; }')
        declarations.emit(declarations.select('Kind'))
        self.assertIn('Value = 9,', '\n'.join(declarations.output))
        declarations = self.declarations('''
enum Kind { Outer = 1 };
namespace { enum Kind { Inner = 2 }; }
struct Unrelated { int value; };
''')
        declarations.emit(declarations.select('Unrelated'))
        with self.assertRaisesRegex(ValueError, 'ambiguous native declaration'):
            declarations.select('Kind')

    def test_selected_constant_and_dependency_collisions_fail_lazily(self):
        declarations = self.declarations('''
typedef int Number;
struct Record { Number value; };
namespace { typedef short Number; }
enum { Value = 1 };
namespace { enum { Value = 2 }; }
''')
        with self.assertRaisesRegex(ValueError, 'ambiguous native declaration: Number'):
            declarations.emit(declarations.select('Record'))
        with self.assertRaisesRegex(ValueError, 'ambiguous native constant declaration: Value'):
            declarations.constant('Value')

    def test_same_header_origin_with_different_tu_profiles_is_rejected(self):
        (self.root / 'include/types.h').write_text('''
enum Kind { Value = MODE };
#if MODE == 1
#define NUMBER int
#else
#define NUMBER short
#endif
typedef NUMBER Number;
struct Record { Number value; };
''')
        for filename in ('first.cpp', 'second.cpp'):
            (self.root / 'src' / filename).write_text('#include "../include/types.h"\nint anchor = 0;')
        profiles = SimpleNamespace(for_source=lambda path: [
            '-x', 'c++', '-std=c++14', '-DMODE=' + ('1' if path.name == 'first.cpp' else '2')])
        sources = Sources(self.root, profiles)
        declarations = Declarations(sources)
        for filename in ('src/first.cpp', 'src/second.cpp'):
            sources.definition(filename, 'anchor')
            declarations.index(sources.units[filename][0])
        for name in ('Kind', 'Number', 'Record'):
            with self.subTest(name=name), self.assertRaisesRegex(ValueError, 'differs across TU profiles'):
                declarations.select(name)

    def test_request_abi_projection_may_omit_its_constructor(self):
        declarations = self.declarations('struct TRandomMapRequest { int width; TRandomMapRequest(int); };')
        declarations.emit(declarations.select('TRandomMapRequest'))
        result = '\n'.join(declarations.output)
        self.assertIn('int width;', result)
        self.assertNotIn('TRandomMapRequest(int)', result)


if __name__ == '__main__':
    unittest.main()
