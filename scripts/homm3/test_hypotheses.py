"""Manifest safety and Cartesian expansion without invoking Wine."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm3.hypotheses import parse_manifest, variants


class HypothesisTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.source = self.root / "unit.cpp"
        self.source.write_text("int left = 1;\nint right = 2;\nint tail = 3;\n")
        self.payload = {"schema": 1, "unit": "unit", "function": "symbol", "axes": [
            {"name": "left", "find": "left = 1", "options": [
                {"name": "baseline"}, {"name": "changed", "replace": "left = 4"}]},
            {"name": "right", "find": "right = 2", "options": [
                {"name": "baseline"}, {"name": "changed", "replace": "right = 5",
                 "extra_edits": [{"find": "tail = 3", "replace": "tail = 6"}]}]}]}

    def parse(self):
        path = self.root / "batch.json"
        path.write_text(json.dumps(self.payload))
        with patch("homm3.hypotheses.manifest.by_unit", return_value={
                "unit": {"unit": "unit", "source": "unit.cpp"}}):
            return parse_manifest(path, root=self.root)

    def test_product_keeps_atomic_edits_and_source(self):
        original = self.source.read_bytes()
        *_, body, axes = self.parse()
        states = variants(body, axes)
        self.assertEqual(len(states), 4)
        self.assertIn(b"left = 4", states[-1].source)
        self.assertIn(b"right = 5", states[-1].source)
        self.assertIn(b"tail = 6", states[-1].source)
        self.assertEqual(original, self.source.read_bytes())

    def test_rejects_stale_and_ambiguous_spans(self):
        for value in ("missing", "int "):
            self.payload["axes"][0]["find"] = value
            with self.assertRaisesRegex(ValueError, "expected 1"):
                self.parse()

    def test_rejects_cross_axis_extra_edit_overlap(self):
        self.payload["axes"][1]["options"][1]["extra_edits"] = [
            {"find": "left = 1", "replace": "left = 7"}]
        with self.assertRaisesRegex(ValueError, "overlap"):
            self.parse()

    def test_identical_sources_compile_once(self):
        self.payload["axes"][0]["options"].append({"name": "same_baseline"})
        *_, body, axes = self.parse()
        self.assertEqual(len(variants(body, axes)), 4)


if __name__ == "__main__":
    unittest.main()
