"""Negative controls for the King's Field source-batch adapter."""
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from homm3.vc6 import hypotheses as h


class ManifestTests(unittest.TestCase):
    def parse(self, axes, original=b"int x; int y; int z;"):
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            source = root / "example.cpp"
            source.write_bytes(original)
            manifest = root / "matrix.json"
            manifest.write_text(json.dumps(dict(
                schema=1, unit="example", function="foo", axes=axes)))
            with patch.object(h.manifest, "by_unit", return_value={
                    "example": {"unit": "example", "source": "example.cpp"}}):
                return h.parse_manifest(manifest, root=root)

    def test_cartesian_and_atomic_extra_edits(self):
        data = self.parse([
            dict(name="x", find="int x;", options=[dict(name="base"),
                 dict(name="long", replace="long x;", extra_edits=[
                     dict(find="int z;", replace="long z;")])]),
            dict(name="y", find="int y;", options=[dict(name="base"),
                 dict(name="short", replace="short y;")]),
        ])
        rendered = h.variants(data[4], data[5])
        self.assertEqual(len(rendered), 4)
        self.assertEqual(rendered[3].source, b"long x; short y; long z;")
        self.assertEqual(data[4], b"int x; int y; int z;")

    def test_stale_or_ambiguous_span_rejected(self):
        for find in ("missing", "int"):
            with self.subTest(find=find), self.assertRaisesRegex(ValueError, "occurs"):
                self.parse([dict(name="x", find=find, options=[dict(name="base")])])

    def test_cross_axis_extra_edit_overlap_rejected(self):
        with self.assertRaisesRegex(ValueError, "overlap"):
            self.parse([
                dict(name="x", find="int x;", options=[dict(name="long",
                     extra_edits=[dict(find="int y;", replace="long y;")])]),
                dict(name="y", find="int y;", options=[dict(name="base")]),
            ])

    def test_identical_sources_compile_once(self):
        data = self.parse([dict(name="x", find="int x;", options=[
            dict(name="baseline"), dict(name="same", replace="int x;")])])
        self.assertEqual(len(h.variants(data[4], data[5])), 1)


if __name__ == "__main__":
    unittest.main()
