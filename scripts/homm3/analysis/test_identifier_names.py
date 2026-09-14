import csv
import tempfile
import unittest
from pathlib import Path

from homm3.analysis.identifier_names import (
    collect_carcass_functions,
    confidence_for_function,
    lower_camel,
    apply_carcass_plan,
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


if __name__ == "__main__":
    unittest.main()
