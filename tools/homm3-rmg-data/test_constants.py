"""Contracts for selecting scalar defaults from real Clang constructor ASTs."""
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace
import unittest

from constants import Native, array_count, assignments, integer_expression
from extract import Sources, initializer, text
import clang.cindex as cx


class ConstantExtractionTests(unittest.TestCase):
    def setUp(self):
        self.directory = TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        (self.root / 'src').mkdir()

    def native(self, body):
        (self.root / 'src/rmg.cpp').write_text('''
struct TRmgGenerator { void initializeObjectGenerators(); };
void TRmgGenerator::initializeObjectGenerators() {}
struct TRmgTreasureDef { TRmgTreasureDef(int, int, int, int); };
''' + body)
        profiles = SimpleNamespace(for_source=lambda path: ['-x', 'c++', '-std=c++14'])
        return Native(Sources(self.root, profiles))

    def test_base_default_ignores_constructor_calls_in_body(self):
        native = self.native('''
struct Reward : TRmgTreasureDef {
    Reward() : TRmgTreasureDef(0, 0, 40 + 2, 7 * 3) {
        TRmgTreasureDef unrelated(0, 0, 999, 888);
    }
};
''')
        self.assertEqual(native.base_argument('Reward', 2), '40 + 2')
        self.assertEqual(native.base_argument('Reward', 3), '7 * 3')

    def test_dynamic_argument_cannot_become_a_scalar_default(self):
        native = self.native('''
struct Reward : TRmgTreasureDef {
    Reward(int density) : TRmgTreasureDef(0, 0, 42, density) {}
};
''')
        with self.assertRaisesRegex(ValueError, 'unsupported 32-bit integer constant expression'):
            native.base_argument('Reward', 3)

    def test_host_dependent_defaults_and_explicit_casts_are_rejected(self):
        for expression in ('sizeof(void*)', 'alignof(void*)', '1L + 2L',
                           'static_cast<int>(1)', '(int)1'):
            with self.subTest(expression=expression):
                native = self.native('''
struct Reward : TRmgTreasureDef {
    Reward() : TRmgTreasureDef(0, 0, 42, ''' + expression + ''') {}
};
''')
                with self.assertRaisesRegex(ValueError, 'unsupported 32-bit integer constant expression'):
                    native.base_argument('Reward', 3)

    def test_named_initializer_uses_the_same_host_independence_check(self):
        native = self.native('const int named = sizeof(void*);')
        variable = native.declaration(cx.CursorKind.VAR_DECL, 'named')
        with self.assertRaisesRegex(ValueError, 'unsupported 32-bit integer constant expression'):
            integer_expression(initializer(variable))

    def test_parenthesized_integer_arithmetic_is_preserved(self):
        native = self.native('const unsigned named = (~0u & (1u << 4)) + 2u;')
        variable = native.declaration(cx.CursorKind.VAR_DECL, 'named')
        self.assertEqual(integer_expression(initializer(variable)), '(~0u & (1u << 4)) + 2u')

    def test_ambiguous_constructor_is_rejected(self):
        native = self.native('''
struct Reward : TRmgTreasureDef {
    Reward() : TRmgTreasureDef(0, 0, 42, 3) {}
    Reward(int) : TRmgTreasureDef(0, 0, 42, 4) {}
};
''')
        with self.assertRaisesRegex(ValueError, 'expected one'):
            native.base_argument('Reward', 3)

    def test_assignment_selector_excludes_comparisons_and_nested_operators(self):
        native = self.native('''
void initialize(int* limits, int index) {
    if (limits[index + 1] == 5) limits[index + 1] = 40 + 2;
}
''')
        function = native.declaration(cx.CursorKind.FUNCTION_DECL, 'initialize')
        self.assertEqual([(text(lhs), text(rhs)) for lhs, rhs in assignments(function)],
                         [('limits[index + 1]', '40 + 2')])

    def test_invalid_selected_constructor_is_rejected(self):
        native = self.native('''
struct Reward : TRmgTreasureDef { Reward(); };
Reward::Reward() : TRmgTreasureDef(0, 0, 42, 3) { missing(); }
''')
        with self.assertRaisesRegex(ValueError, 'invalid body'):
            native.base_argument('Reward', 3)

    def test_array_extent_requires_an_actual_bounded_array(self):
        native = self.native('int offsets[2 + 3]; int* pointer; extern int unbounded[];')
        self.assertEqual(array_count(native.declaration(cx.CursorKind.VAR_DECL, 'offsets')), 5)
        for name in ('pointer', 'unbounded'):
            with self.subTest(name=name), self.assertRaisesRegex(ValueError, 'bounded array'):
                array_count(native.declaration(cx.CursorKind.VAR_DECL, name))


if __name__ == '__main__':
    unittest.main()
