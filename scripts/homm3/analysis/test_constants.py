"""Constant-index parsing, lookup and literal ranking on synthetic sources."""
from pathlib import Path
import tempfile
import unittest

from homm3.analysis.constants import (
    Entry, Index, evaluate, parse_int, resolve, scan_literals, scan_source)


def _entries(source, path="x.h"):
    raws = scan_source(source, path)
    resolve(raws)
    return {(r.kind, r.scope, r.name): (r.value if r.resolved else None) for r in raws}


class ParseTests(unittest.TestCase):
    def test_integer_literals(self):
        self.assertEqual(parse_int("0x9c"), 156)
        self.assertEqual(parse_int("156u"), 156)
        self.assertEqual(parse_int("010"), 8)
        self.assertEqual(parse_int("0"), 0)
        self.assertIsNone(parse_int("1.5"))
        self.assertIsNone(parse_int("1e3"))

    def test_evaluate_operators_and_names(self):
        names = {"A": 3, "B": 4}.get
        self.assertEqual(evaluate(["(", "A", "+", "1", ")", "*", "B"], names), 16)
        self.assertEqual(evaluate(["1", "<<", "4"], names), 16)
        self.assertEqual(evaluate(["7", "/", "2"], names), 3)
        self.assertIsNone(evaluate(["sizeof", "(", "int", ")"], names))
        self.assertIsNone(evaluate(["Unknown"], names))

    def test_enumerators_implicit_explicit_and_scoped(self):
        found = _entries("""
            enum TTownType { eTownNeutral = -1, TOWN_CASTLE, TOWN_RAMPART,
                             TOWN_CONFLUX = 0x8, // comment = 99
                             TOWN_TYPE_COUNT = TOWN_CONFLUX + 1 };
            class hero {
            public:
                enum { NUM_SPELLS = 70 };
                unsigned char m_inSpellbook[NUM_SPELLS];
            };
            H3_ENUM_BEGIN(TOwnerColor)
                OWNER_RED = 0, OWNER_BLUE
            H3_ENUM_END(TOwnerColor)
            enum class EScoped : unsigned char { FIRST = 2, SECOND };
        """)
        self.assertEqual(found[("enumerator", "TTownType", "TOWN_CASTLE")], 0)
        self.assertEqual(found[("enumerator", "TTownType", "TOWN_RAMPART")], 1)
        self.assertEqual(found[("enumerator", "TTownType", "TOWN_TYPE_COUNT")], 9)
        self.assertEqual(found[("enumerator", "hero", "NUM_SPELLS")], 70)
        self.assertEqual(found[("array-length", "hero", "m_inSpellbook")], 70)
        self.assertEqual(found[("enumerator", "TOwnerColor", "OWNER_BLUE")], 1)
        self.assertEqual(found[("enumerator", "EScoped", "SECOND")], 3)

    def test_defines_consts_and_arrays(self):
        found = _entries("""
            #define NUM_RESOURCES 7
            #define MAKE(x) ((x) + 1)
            #define NAME "text"
            static const int g_bits = 0x90;
            const unsigned long kMask = 1 << 3;
            extern const char* g_names[NUM_RESOURCES];
            int g_grid[4][2 * 3] = { 0 };
            void f(int a[]) { int local[16]; x = table[5]; return g_names[3]; }
        """)
        self.assertEqual(found[("define", "", "NUM_RESOURCES")], 7)
        self.assertNotIn(("define", "", "MAKE"), found)
        self.assertIsNone(found[("define", "", "NAME")])
        self.assertEqual(found[("const", "", "g_bits")], 144)
        self.assertEqual(found[("const", "", "kMask")], 8)
        self.assertEqual(found[("array-length", "", "g_names")], 7)
        self.assertEqual(found[("array-length", "", "g_grid[dim 1]")], 4)
        self.assertEqual(found[("array-length", "", "g_grid[dim 2]")], 6)
        self.assertEqual(found[("array-length", "", "local")], 16)
        self.assertNotIn(("array-length", "", "table"), found)


class IndexTests(unittest.TestCase):
    def setUp(self):
        self.index = Index([
            Entry(83, "kNumSpellEffects", "enumerator", "tree", "a.h:1", "TSpellEffectID"),
            Entry(83, "kNumSpellEffects", "enumerator", "dc", "NB11", "TSpellEffectID"),
            Entry(83, "g_spellEffectTraits", "array-length", "tree", "b.cpp:2"),
            Entry(None, "kUnknown", "const", "tree", "c.h:3"),
        ])

    def test_value_and_name_lookup(self):
        self.assertEqual(len(self.index.values(0x53)), 3)
        self.assertEqual([e.origin for e in self.index.values(83, {"dc"})], ["dc"])
        self.assertEqual(self.index.values(84), [])
        self.assertEqual({e.qualified for e in self.index.names("numspell")},
                         {"TSpellEffectID::kNumSpellEffects"})
        self.assertEqual(self.index.array_lengths()["g_spellEffectTraits"], {83})
        self.assertEqual(self.index.values(83)[2].label(), "array length of g_spellEffectTraits")


class LiteralTests(unittest.TestCase):
    def test_ranking(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "src").mkdir()
            header = root / "src/t.h"
            header.write_text("enum E { kNumThings = 83 };\nextern int g_table[83];\n")
            body = root / "src/t.cpp"
            body.write_text(
                "VA(0x00496840, 0x53)\n"
                "int g_table[83] = { 0 };\n"
                "static const int g_other = 83;\n"
                "void f(int effect) {\n"
                "    if (effect >= 83) return;\n"
                "    g_table[effect] = 83;\n"
                "    while (effect < 1000) { effect++; }\n"
                "}\n")
            raws = scan_source(header.read_text(), "src/t.h") + \
                scan_source(body.read_text(), "src/t.cpp")
            resolve(raws)
            index = Index([Entry(r.value, r.name, r.kind, "tree", r.where, r.scope,
                                 " ".join(r.expression)) for r in raws])
            rows = scan_literals(index, root, [header, body])
            found = {(r.path, r.line): (r.score, r.context) for r in rows}
            self.assertEqual(found[("src/t.cpp", 5)], (3, "bound"))
            self.assertEqual(found[("src/t.cpp", 2)], (2, "dimension"))
            self.assertEqual(found[("src/t.h", 2)], (2, "dimension"))
            self.assertEqual(found[("src/t.cpp", 6)], (1, "value"))
            self.assertNotIn(("src/t.cpp", 1), found)   # annotation
            self.assertNotIn(("src/t.cpp", 3), found)   # definition
            self.assertNotIn(("src/t.cpp", 7), found)   # no candidate
            self.assertIn("kNumThings", {e.name for e in rows[0].candidates})


if __name__ == "__main__":
    unittest.main()
