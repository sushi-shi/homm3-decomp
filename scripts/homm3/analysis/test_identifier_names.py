import csv
import tempfile
import unittest
from pathlib import Path

from homm3.analysis.identifier_names import (
    collect_carcass_functions,
    confidence_for_function,
    lower_camel,
    apply_carcass_plan,
    rewrite_code_identifiers,
    restore_compgen_owner_spellings,
    refresh_compatibility_macros,
)


class IdentifierNamesTest(unittest.TestCase):
    def test_lower_camel(self):
        self.assertEqual(lower_camel("get_new_hero"), "getNewHero")
        self.assertEqual(lower_camel("clear_AI_values"), "clearAiValues")
        self.assertEqual(lower_camel("get_Name"), "getName")

    def test_confidence(self):
        self.assertEqual(confidence_for_function("get_new_hero"), 9)
        self.assertEqual(confidence_for_function("clear_AI_values"), 7)
        self.assertEqual(confidence_for_function("function_123"), 6)

    def test_carcass_fallback(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "src").mkdir()
            (root / "src" / "game.cpp").write_text(
                "#if 0  // @carcass\n"
                "Thing get_new_hero(HeroClass heroClass)\n"
                "{\n}\n"
                "#endif  // @carcass\n"
            )
            rows = collect_carcass_functions(root)
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0].qualified_name, "get_new_hero")
        self.assertEqual(rows[0].suggested, "getNewHero")
        self.assertEqual(rows[0].strategy, "lexical-review")

    def test_apply_carcass_plan(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "src").mkdir()
            source = root / "src" / "game.cpp"
            source.write_text("Thing get_new_hero(HeroClass heroClass)\n")
            plan = root / "plan.tsv"
            with plan.open("w", newline="") as stream:
                writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
                writer.writerow(("kind", "qualified_name", "current", "suggested",
                                 "strategy", "confidence", "file", "line", "usr"))
                writer.writerow(("function", "get_new_hero", "get_new_hero",
                                 "getNewHero", "lexical-review", 8,
                                 "src/game.cpp", 1, ""))
            self.assertEqual(apply_carcass_plan(root, plan, 8), 0)
            self.assertEqual(
                source.read_text(),
                "// Before normalization (function): get_new_hero.\n"
                "Thing getNewHero(HeroClass heroClass)\n",
            )

    def test_type_rewrite_preserves_comments_literals_and_value_names(self):
        source = (
            "class town {}; // town stays as evidence\n"
            "town* selected = new town;\n"
            "int town = 3;\n"
            "searchArray currentSearch;\n"
            "void* town::`scalar deleting destructor'(unsigned flags);\n"
            "std::pair<int, town> pair;\n"
            "call(first, town, last);\n"
            "draw(town * 2);\n"
            "const char* label = \"town\";\n"
            "TArtifact artifact;\n"
        )
        self.assertEqual(
            rewrite_code_identifiers(
                source, {"town": "Town", "searchArray": "SearchArray",
                         "TArtifact": "Artifact"}, {"town", "searchArray"}),
            "class Town {}; // town stays as evidence\n"
            "Town* selected = new Town;\n"
            "int town = 3;\n"
            "SearchArray currentSearch;\n"
            "void* Town::`scalar deleting destructor'(unsigned flags);\n"
            "std::pair<int, Town> pair;\n"
            "call(first, town, last);\n"
            "draw(town * 2);\n"
            "const char* label = \"town\";\n"
            "Artifact artifact;\n",
        )

    def test_compgen_owner_keeps_recovered_compiler_spelling(self):
        source = (
            "VA_COMPGEN(0x401000, 0x21, SCALAR_DELETING_DTOR, Town)\n"
            "VA_COMPGEN(0x402000, 0x21, VECTOR_DTOR, ResourcePtr)\n"
        )
        self.assertEqual(
            restore_compgen_owner_spellings(
                source, {"town": "Town", "TResourcePtr": "ResourcePtr"}),
            "VA_COMPGEN(0x401000, 0x21, SCALAR_DELETING_DTOR, town)\n"
            "VA_COMPGEN(0x402000, 0x21, VECTOR_DTOR, TResourcePtr)\n",
        )

    def test_compatibility_macros_are_idempotent(self):
        source = (
            "#ifndef ResourcePtr\n#define ResourcePtr ResourcePtr\n#endif\n"
            "#ifndef ResourcePtr\n#define ResourcePtr TResourcePtr\n#endif\n"
            "class ResourcePtr {};\n"
        )
        expected = (
            "#ifndef ResourcePtr\n#define ResourcePtr TResourcePtr\n#endif\n"
            "class ResourcePtr {};\n"
        )
        replacements = {"TResourcePtr": "ResourcePtr"}
        self.assertEqual(refresh_compatibility_macros(source, replacements),
                         expected)
        self.assertEqual(refresh_compatibility_macros(expected, replacements),
                         expected)

    def test_elaborated_type_use_is_not_a_declaration(self):
        source = "void f() {\nclass Town* town = 0;\n}\n"
        self.assertEqual(
            refresh_compatibility_macros(source, {"town": "Town"}), source)


if __name__ == "__main__":
    unittest.main()
