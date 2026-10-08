"""Source-extraction contracts, using tiny real Clang translation units."""
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace
import unittest

from extract import Sources, expand


class ExtractionTests(unittest.TestCase):
    def setUp(self):
        self.directory = TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)

    def source(self, text):
        (self.root / 'input.cpp').write_text(text)
        return Sources(self.root, SimpleNamespace(for_source=lambda path: ['-x', 'c++', '-std=c++14']))

    def test_nested_initializer_and_constructor_arguments(self):
        source = self.source('''
enum Shape { Corner = 7 };
struct Entry { Shape shape; bool special; };
const Entry frames[2] = {{Corner, true}};
struct Rule { Rule(bool, int, const Entry*); };
Rule rule(false, 2, frames);
''')
        result = expand('// @rmg initializer input.cpp frames\n'
                        '// @rmg arguments input.cpp rule', source)
        self.assertEqual(result, '{{Corner, true}}\nfalse, 2, frames')
        self.assertEqual(expand('Entry exported[@count] =\n// @rmg initializer input.cpp frames', source),
                         'Entry exported[2] =\n{{Corner, true}}')

    def test_local_definitions_require_function_scope(self):
        source = self.source('''
struct Generator { void first(); void second(); };
void Generator::first() { static int offsets[] = {1, 2}; }
void Generator::second() { static int offsets[] = {3, 4}; }
''')
        self.assertEqual(expand('// @rmg initializer input.cpp Generator::second::offsets', source), '{3, 4}')
        with self.assertRaisesRegex(ValueError, 'expected one'):
            expand('// @rmg initializer input.cpp offsets', source)

    def test_errors_in_unrelated_bodies_do_not_hide_valid_data(self):
        source = self.source('int values[] = {42}; void broken() { missing(); }')
        self.assertEqual(expand('// @rmg initializer input.cpp values', source), '{42}')

    def test_error_in_selected_body_rejects_local_initializer(self):
        source = self.source('struct G { void run(); }; void G::run() { int v[] = {42}; missing(); }')
        with self.assertRaisesRegex(ValueError, 'invalid body'):
            expand('// @rmg initializer input.cpp G::run::v', source)

    def test_header_errors_are_fatal_and_headers_are_tracked(self):
        header = self.root / 'values.h'
        header.write_text('enum { VALUE = 19 };')
        code = '#include "values.h"\nint values[] = {VALUE};'
        source = self.source(code)
        self.assertEqual(expand('// @rmg initializer input.cpp values', source), '{VALUE}')
        self.assertIn(header, source.dependencies)
        header.write_text('unknown_type broken;')
        with self.assertRaises(ValueError):
            expand('// @rmg initializer input.cpp values', self.source(code))

    def test_conditional_fixed_recipe_is_not_flattened(self):
        source = self.source('''
struct type_scholar_def {};
struct List { void push_back(type_scholar_def*); };
struct G { List m_objectGenerators; void run(); };
void G::run() { if (true) m_objectGenerators.push_back(new type_scholar_def()); }
''')
        with self.assertRaisesRegex(ValueError, 'conditional'):
            expand('// @rmg recipes input.cpp G::run', source)

    def test_ordered_recipes_keep_dynamic_groups_and_reward_arguments(self):
        code = '''
struct type_scholar_def {};
struct type_black_box_creature_def { type_black_box_creature_def(int); };
struct type_key_tent_def { type_key_tent_def(int, int); };
struct type_map_dwelling_def { type_map_dwelling_def(int); };
struct type_quest_creature_def { type_quest_creature_def(int, int); };
struct type_quest_experience_def { type_quest_experience_def(int, int, int); };
struct type_quest_gold_def { type_quest_gold_def(int, int, int); };
struct List { template<class T> void push_back(T*); };
struct G { List m_objectGenerators; void run(); };
void G::run() {
    m_objectGenerators.push_back(new type_scholar_def());
    for (int i = 2; i--;) m_objectGenerators.push_back(new type_black_box_creature_def(i));
    for (int i = 2; i--;) {
        m_objectGenerators.push_back(new type_key_tent_def(i, 5000));
        m_objectGenerators.push_back(new type_key_tent_def(i, 7500));
    }
    for (int i = 2; i--;) m_objectGenerators.push_back(new type_map_dwelling_def(i));
    for (int p = 0; p < 2; ++p) {
        for (int i = 2; i--;) m_objectGenerators.push_back(new type_quest_creature_def(i, p));
        m_objectGenerators.push_back(new type_quest_experience_def(p, 2000, 5000));
        m_objectGenerators.push_back(new type_quest_gold_def(p, 5333, 10000));
    }
}
'''
        source = self.source(code)
        self.assertEqual(expand('// @rmg recipes input.cpp G::run', source), '\n'.join([
            'emitScholarDef();',
            'std::printf("TreasureRecipe::CreatureBoxes,");',
            'std::printf("TreasureRecipe::KeyTents,");',
            'std::printf("TreasureRecipe::Dwellings,");',
            'std::printf("TreasureRecipe::Seers,");',
        ]))
        loop = 'for (int i = 2; i--;) m_objectGenerators.push_back(new type_black_box_creature_def(i));'
        expected = expand('// @rmg recipes input.cpp G::run', source)
        scoped = self.source(code.replace(loop, '{ int count = 2; ' + loop + ' }'))
        self.assertEqual(expand('// @rmg recipes input.cpp G::run', scoped), expected)
        source = self.source(code)
        self.assertEqual(expand('// @rmg tent_values input.cpp G::run', source), '{5000, 7500}')
        self.assertEqual(expand('// @rmg seer_rewards input.cpp G::run', source),
                         'RMG_SEER_REWARD(Experience, 2000, 5000)\nRMG_SEER_REWARD(Gold, 5333, 10000)')
        creature = 'for (int i = 2; i--;) m_objectGenerators.push_back(new type_quest_creature_def(i, p));'
        reward = 'm_objectGenerators.push_back(new type_quest_experience_def(p, 2000, 5000));'
        swapped = code.replace(creature, 'CREATURE').replace(reward, creature).replace('CREATURE', reward)
        with self.assertRaisesRegex(ValueError, 'must precede'):
            expand('// @rmg recipes input.cpp G::run', self.source(swapped))
        for constructor in ('type_key_tent_def(i, 5000)', 'type_quest_gold_def(p, 5333, 10000)'):
            append = f'm_objectGenerators.push_back(new {constructor});'
            conditional = code.replace(append, f'if (true) {{ {append} }}')
            with self.subTest(constructor=constructor), self.assertRaisesRegex(ValueError, 'unconditional'):
                expand('// @rmg recipes input.cpp G::run', self.source(conditional))


if __name__ == '__main__':
    unittest.main()
