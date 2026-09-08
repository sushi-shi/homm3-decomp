from __future__ import annotations

import json
from pathlib import Path
import random
import tempfile
import unittest

from homm3.vc6.source_families import (
    Axis, Option, identity_symbol, load_manifest, next_population, render, select_elites,
)


class SourceFamiliesTests(unittest.TestCase):
    def test_distributed_starter_matches_current_source(self):
        root = Path(__file__).resolve().parents[3]
        _, originals, axes = load_manifest(
            root / "scripts/experiments/rmg-grid-source-family.json", root)
        self.assertEqual([len(axis.options) for axis in axes], [2, 2, 3, 6])
        self.assertEqual(render(originals, axes, (0, 0, 0, 0)), originals)

    def test_anonymous_scope_identity_preserves_semantics_not_path_nonce(self):
        first = r'?g_directions@?%Z:\tmp\first\rmg.cpp123@@3PAUTPoint@@A'
        repeat = r'?g_directions@?%Z:\tmp\repeat\rmg.cpp456@@3PAUTPoint@@A'
        self.assertEqual(identity_symbol(first), identity_symbol(repeat))
        self.assertNotEqual(identity_symbol(first), identity_symbol(repeat.replace('g_directions', 'g_other')))
        self.assertNotEqual(identity_symbol(first), identity_symbol(repeat.replace('rmg.cpp', 'terrain.cpp')))
        self.assertEqual(identity_symbol('?ordinary@@YAXXZ'), '?ordinary@@YAXXZ')

    def manifest(self, root, payload):
        path = root / "family.json"
        path.write_text(json.dumps(payload))
        return load_manifest(path, root)

    def test_cartesian_exact_edits_and_atomic_extra_edit(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "include").mkdir()
            (root / "include/a.h").write_text("int x; int y;")
            (root / "include/b.h").write_text("int z;")
            payload = {"schema": 1, "source": "include/a.h", "axes": [
                {"name": "x", "find": "int x;", "options": [
                    {"name": "base"}, {"name": "long", "replace": "long x;",
                        "extra_edits": [{"source": "include/b.h", "find": "int z;", "replace": "long z;"}]}]},
                {"name": "y", "find": "int y;", "options": [
                    {"name": "base"}, {"name": "short", "replace": "short y;"}]}]}
            _, originals, axes = self.manifest(root, payload)
            self.assertEqual(render(originals, axes, (0, 0)), originals)
            changed = render(originals, axes, (1, 1))
            self.assertEqual(changed["include/a.h"], "long x; short y;")
            self.assertEqual(changed["include/b.h"], "long z;")
            self.assertEqual((root / "include/a.h").read_text(), originals["include/a.h"])
            payload["axes"][1]["find"] = "int x;"
            with self.assertRaisesRegex(ValueError, "overlapping axes"):
                self.manifest(root, payload)
            payload["axes"][1]["find"] = "absent"
            with self.assertRaisesRegex(ValueError, "exactly once"):
                self.manifest(root, payload)

    def test_vendor_and_escape_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for source in ("vendor/a.h", "../outside.h"):
                with self.assertRaises(ValueError):
                    self.manifest(root, {"schema": 1, "source": source, "axes": [
                        {"name": "a", "find": "x", "options": [{"name": "base"}]}]})

    def test_60_member_family_exhausts_without_repeating(self):
        axes = tuple(Axis(str(n), tuple(Option(str(i), ()) for i in range(n))) for n in (2, 2, 3, 5))
        population = next_population(axes, [], set(), 60, random.Random(1))
        self.assertEqual(len(set(population)), 60)
        self.assertEqual(population[0], (0, 0, 0, 0))
        self.assertEqual(next_population(axes, [], set(population), 60, random.Random(2)), [])

    def test_elites_keep_specialist_not_just_same_object_leaders(self):
        records = [
            {"id": "a", "object_hash": "a", "scores": {"x": 100, "y": 90}},
            {"id": "b", "object_hash": "a", "scores": {"x": 100, "y": 90}},
            {"id": "c", "object_hash": "c", "scores": {"x": 60, "y": 100}},
            {"id": "d", "object_hash": "d", "scores": {"x": 99, "y": 90}},
        ]
        elites = select_elites(records, 2)
        self.assertEqual({row["object_hash"] for row in elites}, {"a", "c"})


if __name__ == "__main__":
    unittest.main()
