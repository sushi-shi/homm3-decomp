from __future__ import annotations

import json
from pathlib import Path
import random
import tempfile
import unittest

from homm3.vc6.source_families import (
    Axis, Option, build_snapshot, identity_symbol, load_manifest,
    next_population, render, projected_max_scores, select_elites,
)


class SourceFamiliesTests(unittest.TestCase):
    def test_anonymous_scope_identity_preserves_semantics_not_path_nonce(self):
        first = r'?g_directions@?%Z:\tmp\first\rmg.cpp123@@3PAUTPoint@@A'
        repeat = r'?g_directions@?%Z:\tmp\repeat\rmg.cpp456@@3PAUTPoint@@A'
        self.assertEqual(identity_symbol(first), identity_symbol(repeat))
        self.assertNotEqual(identity_symbol(first), identity_symbol(repeat.replace('g_directions', 'g_other')))
        self.assertNotEqual(identity_symbol(first), identity_symbol(repeat.replace('rmg.cpp', 'terrain.cpp')))
        self.assertEqual(identity_symbol('?ordinary@@YAXXZ'), '?ordinary@@YAXXZ')

    def test_header_anonymous_scope_identity_preserves_type_and_signature(self):
        first = r'??0TAutoStrPtr@?%Z:\tmp\first\include\autostrptr.h3081621612@@QAE@XZ'
        repeat = r'??0TAutoStrPtr@?%Z:\tmp\repeat\include\autostrptr.h8275597@@QAE@XZ'
        self.assertEqual(identity_symbol(first), identity_symbol(repeat))
        for changed in (repeat.replace('TAutoStrPtr', 'OtherType'),
                        repeat.replace('autostrptr.h', 'other.h'),
                        repeat.replace('QAE@XZ', 'QAE@H@Z')):
            self.assertNotEqual(identity_symbol(first), identity_symbol(changed))

    def test_snapshot_carries_every_tree_a_candidate_compile_resolves(self):
        # A candidate is compiled with HOMM3_DIR set to its own tree, so cc_wrap
        # reads the project specification from there. Dropping config/ out of the
        # snapshot made every run die on the unchanged-source control with
        # FileNotFoundError on <candidate>/config/project.toml.
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "root"
            for name in ("include", "src", "vendor", "config"):
                (root / name).mkdir(parents=True)
            (root / "config/project.toml").write_text("[inputs.retail]\n")
            (root / "src/unit.cpp").write_text("int unit;\n")
            (root / "src/build").mkdir()
            (root / "src/build/stale.obj").write_text("x")
            snapshot = build_snapshot(root, Path(tmp) / "snapshot")
            # Whatever cc_wrap resolves from the candidate root must be reachable.
            self.assertTrue((snapshot / "config/project.toml").is_file())
            self.assertTrue((snapshot / "src/unit.cpp").is_file())
            self.assertTrue((snapshot / "include").is_dir())
            # Read-only trees are shared, edited trees are real copies.
            self.assertTrue((snapshot / "config").is_symlink())
            self.assertTrue((snapshot / "vendor").is_symlink())
            self.assertFalse((snapshot / "src").is_symlink())
            self.assertFalse((snapshot / "src/build").exists())

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

    def test_held_max_does_not_displace_a_better_reconstruction(self):
        from homm3.match.status import MatchRow

        previous = {("u", "target"): MatchRow(90, 90, 90, 1, "target-old"),
                    ("u", "sibling"): MatchRow(100, 100, 100, 2, "same")}
        better = {"id": "better", "object_hash": "a",
                  "scores": {"u|target": 95, "u|sibling": 70}}
        weaker = {"id": "weaker", "object_hash": "b",
                  "scores": {"u|target": 94, "u|sibling": 100}}
        hashes = {("u", "target"): "target-new", ("u", "sibling"): "same"}
        for row in (better, weaker):
            row["max_scores"] = projected_max_scores(row, previous, hashes)
        self.assertEqual(better["max_scores"]["u|sibling"], 100)
        self.assertEqual(select_elites([better, weaker], 1)[0]["id"], "better")
        self.assertEqual(better["scores"]["u|sibling"], 70)
        self.assertEqual(previous[("u", "sibling")].cur, 100)

    def test_proven_source_edit_resets_projected_max_but_unknown_does_not(self):
        from homm3.match.status import MatchRow

        previous = {("u", "f"): MatchRow(90, 100, 100, 1, "old")}
        row = {"scores": {"u|f": 80}}
        self.assertEqual(projected_max_scores(row, previous, {}), {"u|f": 100})
        self.assertEqual(projected_max_scores(row, previous, {("u", "f"): "old"}),
                         {"u|f": 100})
        self.assertEqual(projected_max_scores(row, previous, {("u", "f"): "new"}),
                         {"u|f": 80})

    def test_specialist_ranking_uses_projected_max(self):
        records = [
            {"id": "a", "object_hash": "a", "scores": {"held": 50, "x": 100, "y": 90},
             "max_scores": {"held": 100, "x": 100, "y": 90}},
            {"id": "b", "object_hash": "b", "scores": {"held": 100, "x": 99, "y": 90},
             "max_scores": {"held": 100, "x": 99, "y": 90}},
            {"id": "c", "object_hash": "c", "scores": {"held": 70, "x": 90, "y": 95},
             "max_scores": {"held": 100, "x": 90, "y": 95}},
        ]
        self.assertEqual([row["id"] for row in select_elites(records, 2)], ["a", "c"])


if __name__ == "__main__":
    unittest.main()
