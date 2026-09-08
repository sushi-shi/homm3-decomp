"""Semantic and manifest controls for Complete-only treasure-group fitting."""
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


class RmgGroupFitTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-group-fit-family.py")
        self.source = (self.root / self.module.SOURCE).read_text()

    def test_all_60_manifest_states_rebase_and_reject_changed_rules(self):
        payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                       axes=self.module.make_axes(self.source))
        with tempfile.TemporaryDirectory(prefix="rmg-group-fit-manifest-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual(len(axes[0].options), 60)
        self.assertEqual(source_families.render(originals, axes, (0,)), originals)
        texts = [source_families.render(originals, axes, (i,))[self.module.SOURCE] for i in range(60)]
        self.assertEqual(len(set(texts)), 60)
        for text in texts:
            rebased = self.module.make_axes(text)[0]
            self.assertEqual(len(rebased["options"]), 60)
            self.assertEqual(rebased["find"], rebased["options"][0]["replace"])
        with self.assertRaisesRegex(ValueError, "review current"):
            self.module.make_axes(self.source.replace("objectType != MONSTER", "objectType == MONSTER"))

    def test_frontier_keeps_ten_parents_and_fills_60_with_tested_successors(self):
        original = self.module.helpers().definition(self.source, self.module.FUNCTION)
        # Representative diverse parents: do not depend on an external score
        # artifact or assume that their relative ranking survives a new TU.
        coordinates = ((1, 4), (4, 4), (2, 4), (3, 4), (5, 4),
                       (2, 0), (2, 1), (3, 0), (3, 1), (2, 2))
        parents = [("parent" + str(i), self.source.replace(original, self.module.body(origin, query, 0)))
                   for i, (origin, query) in enumerate(coordinates)]
        axes = self.module.make_parent_axes(self.source, parents)
        options = axes[0]["options"]
        self.assertEqual(len(options), 60)
        self.assertEqual(len({option["replace"] for option in options}), 60)
        self.assertEqual(options[0]["replace"], original)
        for label, text in parents:
            self.assertIn(dict(name=label, replace=self.module.helpers().definition(text, self.module.FUNCTION)), options)
        self.assertEqual(len(options) - 1 - len(parents), 49)
        semantic = {text for _, text in self.module.semantic_forms()}
        self.assertEqual(len(semantic), 198)
        self.assertTrue({option["replace"] for option in options} <= semantic)
        payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes)
        with tempfile.TemporaryDirectory(prefix="rmg-group-fit-frontier-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            _, originals, parsed = source_families.load_manifest(path, self.root)
        self.assertEqual(source_families.render(originals, parsed, (0,)), originals)

    @unittest.skipUnless(shutil.which("g++"), "group-fit oracle needs g++")
    def test_parents_and_all_successors_against_mask_oracle_and_negative_controls(self):
        header = (self.root / "include/rmg.h").read_text()
        template = (self.root / "scripts/experiments/rmg-group-fit-oracle.cpp").read_text()

        def block(text, start):
            at = text.index(start)
            return text[at:text.index("\n};", at) + 3]

        def fields(text, names):
            # Project actual field declarations, without asserting host layout
            # or including unrelated construction/destruction in this caller test.
            lines = []
            for name in names:
                matches = [line.split("//")[0].strip() for line in text.splitlines()
                           if re.search(r"\b" + name + r";", line.split("//")[0])]
                self.assertEqual(len(matches), 1, name)
                lines.append(matches[0])
            return "\n".join(lines)

        types = [block(header, "struct " + name + " {") for name in
                 ("TRmgVector", "TPoint", "TRmgMapPosition", "TRmgGroundTile", "TRmgGroundTileData")]
        types.append(block(header, "enum ERmgDirectionLimits {"))
        predicates = []
        for name in ("isRoadEntrance", "hasBorderObject"):
            at = header.index("    unsigned char " + name + "() const")
            predicates.append(header[at:header.index("\n    }", at) + 6])
        at = header.index("    inline TRmgMapItem* getMapItem(int x, int y, int z)")
        lookup = header[at:header.index("\n    }", at) + 6]
        lookup = lookup.replace("    {\n", "    {\n        record(x, y, z);\n", 1)
        at = self.source.index("TPoint operator+(TPoint point, TRmgVector offset)\n")
        addition = self.source[at:self.source.index("\n}", at) + 2]
        original = self.module.helpers().definition(self.source, self.module.FUNCTION)
        programs, checks = [], []

        def candidate(label, body):
            programs.append("struct " + label + " : FitRoot { unsigned char canFitObject("
                            "TRmgObjectPropertiesRef*, TRmgMapPosition); };\n"
                            + body.replace("TRmgTreasureGroup::", label + "::"))

        methods = dict(self.module.semantic_forms())
        methods["current"] = original
        # Before a later-stage VC6 population, optionally compile every exact
        # manifest state through this same independent behavioral oracle.
        # No scores, flags, templates or candidate-provided tests are trusted.
        manifest = os.environ.get("HOMM3_GROUP_FIT_MANIFEST")
        if manifest:
            _, originals, axes = source_families.load_manifest(Path(manifest), self.root)
            self.assertEqual(len(axes), 1)
            self.assertEqual(len(axes[0].options), 60)
            self.assertEqual(set(originals), {self.module.SOURCE})
            for i in range(60):
                rendered = source_families.render(originals, axes, (i,))[self.module.SOURCE]
                methods["manifest" + str(i)] = self.module.helpers().definition(rendered, self.module.FUNCTION)
        for index, body in enumerate(dict.fromkeys(methods.values())):
            label = "Candidate" + str(index)
            candidate(label, body)
            checks.append('if (!check<' + label + '>()) { std::fprintf(stderr, "failed state ' + str(index) + '\\n"); return 1; }')
        for label, before, after in (
                ("WrongRearRange", "int direction = 5;", "int direction = 6;"),
                ("WrongTrait", "[neighborType][2]", "[neighborType][1]"),
                ("WrongBorderPolicy", "properties, position, -1, 1", "properties, position, -1, 0"),
                ("WrongNeighborLevel", "nearby.m_x, nearby.m_y, 0", "nearby.m_x, nearby.m_y, position.m_z"),
                ("WrongRock", "item->m_tile.m_landType != eTerrainRock", "true"),
                ("WrongClosed", "if (direction == RMG_DIRECTION_COUNT)", "if (direction > RMG_DIRECTION_COUNT)")):
            control = self.module.body(0, 0, 0)
            self.assertIn(before, control)
            candidate(label, control.replace(before, after))
            checks.append("if (check<" + label + ">()) return 2;")
        prototype = block((self.root / "include/advmgr_objects.h").read_text(), "struct TObjectType {")
        point_start = prototype.index("    struct TPoint {")
        prototype_point = prototype[point_start:prototype.index("\n    };", point_start) + 7]
        program = template
        for marker, replacement in (("VALUE_TYPES", "\n".join(types)),
                ("OBJECT_ENUM", block((self.root / "include/mapcell.h").read_text(), "enum TAdventureObjectType {")),
                ("PROTOTYPE_FIELDS", fields(prototype, ("m_objectType", "m_triggerCell"))),
                ("PROTOTYPE_POINT", prototype_point),
                ("PROPERTY_FIELDS", fields(block(header, "struct TRmgObjectPropertiesRef {"), ("m_prototype",))),
                ("OBJECT_FIELDS", fields(block(header, "class type_object {"), ("m_properties",))),
                ("PREDICATES", "\n".join(predicates)), ("SCALAR_LOOKUP", lookup), ("VALUE_HELPERS", addition),
                ("DIRECTIONS", block(self.source, "TPoint g_rmgDirections[8] = {")),
                ("CANDIDATES", "\n".join(programs)), ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", replacement)
        self.assertNotIn("// @", program)
        with tempfile.TemporaryDirectory(prefix="rmg-group-fit-oracle-") as raw:
            path = Path(raw) / "oracle.cpp"
            path.write_text(program)
            executable = Path(raw) / "oracle"
            compiled = subprocess.run(["g++", "-std=c++98", "-O1", "-fno-elide-constructors", "-I", str(self.root / "include"),
                                       str(path), "-o", str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(compiled.returncode, 0, compiled.stderr[-10000:])
            checked = subprocess.run([str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)


if __name__ == "__main__":
    unittest.main()
