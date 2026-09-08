"""Independent packed-state and ordered-helper controls for treasure reset."""
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


class RmgGroupResetTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-group-reset-family.py")
        self.source = (self.root / self.module.SOURCE).read_text()

    def test_60_reset_states_preserve_the_two_coordinate_helper_boundary(self):
        payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                       axes=self.module.make_axes(self.source))
        with tempfile.TemporaryDirectory(prefix="rmg-group-reset-manifest-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual(len(axes[0].options), 60)
        self.assertEqual(source_families.render(originals, axes, (0,)), originals)
        sources = [source_families.render(originals, axes, (i,))[self.module.SOURCE] for i in range(60)]
        self.assertEqual(len(set(sources)), 60)
        for source in sources:
            axis = self.module.make_axes(source)[0]
            self.assertEqual(axis["find"], axis["options"][0]["replace"])
            self.assertEqual(len(axis["options"]), 60)
            self.assertEqual(source.count("TRmgMapItem* type_random_map::getMapItem(int x, int y)"), 1)
        current = self.module.helpers().definition(self.source, self.module.FUNCTION)
        historical = self.source.replace(current, self.module.BASELINE)
        with self.assertRaisesRegex(ValueError, "review the current"):
            self.module.make_axes(historical.replace("m_map.getMapItem(0, 0);", "m_map.getMapItem(0, 0, 0);"))
        # Overloads are selected by their complete parameter text, not by
        # whichever body happens to be first after moving the real definition.
        helper = self.module.helpers()
        with self.assertRaisesRegex(ValueError, "expected one definition"):
            helper.definition(self.source, "type_random_map::getMapItem")
        self.assertIn("y * m_mapWidth + x", helper.definition(
            self.source, "type_random_map::getMapItem", parameters="int x, int y"))
        self.assertIn("point.m_z", helper.definition(
            self.source, "type_random_map::getMapItem", parameters="TRmgMapPosition point"))

    def test_reset_frontier_retains_ten_parents_and_49_fresh_bindings(self):
        current = self.module.helpers().definition(self.source, self.module.FUNCTION)
        source = self.source.replace(current, self.module.BASELINE)
        forms = list(self.module.forms())
        parents = [("parent" + str(i), source.replace(self.module.BASELINE, forms[index][1]))
                   for i, index in enumerate((1, 4, 7, 10, 14, 22, 30, 39, 48, 57))]
        options = self.module.make_parent_axes(source, parents)[0]["options"]
        self.assertEqual(len(options), 60)
        self.assertEqual(len({option["replace"] for option in options}), 60)
        self.assertEqual(options[0]["replace"], self.module.BASELINE)
        for label, text in parents:
            body = self.module.helpers().definition(text, self.module.FUNCTION)
            self.assertIn(dict(name=label, replace=body), options)
        self.assertTrue({option["replace"] for option in options} <= self.module.admitted_bodies())
        self.assertTrue(any("reference" in option["name"] for option in options))
        self.assertTrue(any("last_first" in option["name"] for option in options))

    def test_lifetime_frontier_uses_real_origin_dimensions_and_cursor(self):
        current = self.module.helpers().definition(self.source, self.module.FUNCTION)
        source = self.source.replace(current, self.module.BASELINE)
        forms = list(self.module.forms())
        parents = []
        for index in (3, 12, 21, 30, 39):
            for name in ("both_references_entry", "outline_reference"):
                body = dict(self.module.refinements(forms[index][1]))[name]
                parents.append((str(index) + "+" + name, source.replace(self.module.BASELINE, body)))
        options = self.module.make_parent_axes(source, parents, lifetime=True)[0]["options"]
        self.assertEqual(len(options), 60)
        self.assertEqual(len({option["replace"] for option in options}), 60)
        for label, text in parents:
            body = self.module.helpers().definition(text, self.module.FUNCTION)
            self.assertIn(dict(name=label, replace=body), options)
        self.assertTrue({option["replace"] for option in options} <= self.module.admitted_bodies())
        self.assertTrue(any("origin_" in option["name"] for option in options))
        self.assertTrue(any("dimension_" in option["name"] for option in options))
        self.assertTrue(any("postincrement_" in option["name"] for option in options))

    @unittest.skipUnless(shutil.which("g++"), "reset contract oracle needs g++")
    def test_all_reset_states_against_independent_masks_and_helper_trace(self):
        header = (self.root / "include/rmg.h").read_text()
        template = (self.root / "scripts/experiments/rmg-group-reset-oracle.cpp").read_text()

        def block(start):
            at = header.index(start)
            return header[at:header.index("\n};", at) + 3]

        def fields(text, names):
            declarations = []
            for name in names:
                found = [line.split("//")[0].strip() for line in text.splitlines()
                         if re.search(r"\b" + name + r";", line.split("//")[0])]
                self.assertEqual(len(found), 1, name)
                declarations.append(found[0])
            return "\n".join(declarations)

        helper = self.module.helpers()
        types = [block("struct " + name + " {") for name in (
            "TRmgVector", "TPoint", "TRmgMapPosition", "TRmgZoneBounds", "TRmgMovementCost",
            "TRmgZoneCellState", "TRmgGroundTile", "TRmgGroundTileData", "TRmgConnectionDecoration")]
        definitions = []
        for name, parameters, hook in (
                ("TRmgMapItem::clear", None, "    recordClear(this);\n"),
                ("TRmgMapItem::setTerrain", None, "    recordTerrain(this, terrain, frame, flipX, flipY);\n"),
                ("type_random_map::clear", None,
                 "    g_events.push_back(Event(0, 0, g_objects->size(), g_outline->size(), *g_hasGuard, *g_ready));\n"),
                ("type_random_map::getMapItem", "int x, int y", "    g_events.push_back(Event(2, 0, x, y));\n")):
            body = helper.definition(self.source, name, parameters=parameters)
            definitions.append(body.replace("\n{\n", "\n{\n" + hook, 1))
        methods = dict(self.module.forms())
        methods["current"] = helper.definition(self.source, self.module.FUNCTION)
        count_first = methods["count_then_item+objects_clear+outline_clear"]
        for label, body in self.module.lifetime_refinements(count_first):
            if label.startswith("dimension_copies_"):
                methods["count_before_lookup+" + label] = body
        if os.environ.get("HOMM3_GROUP_RESET_MANIFEST"):
            _, originals, axes = source_families.load_manifest(Path(os.environ["HOMM3_GROUP_RESET_MANIFEST"]), self.root)
            for index in range(60):
                source = source_families.render(originals, axes, (index,))[self.module.SOURCE]
                methods["manifest_" + str(index)] = helper.definition(source, self.module.FUNCTION)
        programs, checks = [], []

        def candidate(label, body):
            programs.append("struct " + label + " : ResetRoot { void reset(); };\n"
                            + body.replace("TRmgTreasureGroup::", label + "::"))

        for index, body in enumerate(dict.fromkeys(methods.values())):
            label = "Candidate" + str(index)
            candidate(label, body)
            checks.append('if (!check<' + label + '>()) { std::fprintf(stderr, "failed reset state '
                          + str(index) + '\\n"); return 1; }')
        controls = (
            ("MissingObjects", "    m_objects.erase(m_objects.begin(), m_objects.end());\n", ""),
            ("MissingOutline", "    m_outline.erase(m_outline.begin(), m_outline.end());\n", ""),
            ("MissingMapClear", "    m_map.clear();\n", ""),
            ("WrongOverload", "m_map.getMapItem(0, 0)", "m_map.getMapItem(0, 0, 0)"),
            ("WrongOrigin", "m_map.getMapItem(0, 0)", "m_map.getMapItem(1, 0)"),
            ("MissingGuard", "    m_hasGuard = 0;\n", ""),
            ("MissingReady", "    m_ready = 0;\n", ""),
            ("EarlyFlags", "    m_map.clear();\n    m_hasGuard = 0;\n    m_ready = 0;",
             "    m_hasGuard = 0;\n    m_ready = 0;\n    m_map.clear();"),
            ("WrongTerrain", "setTerrain(eTerrainDirt, 0, 0, 0)", "setTerrain(eTerrainWater, 0, 0, 0)"),
            ("WrongPlaneCount", "m_map.m_mapWidth * m_map.m_mapHeight;",
             "m_map.m_mapWidth * m_map.m_mapHeight * m_map.m_numberLevels;"),
            ("MissingLastCell", "while (count--)", "while (count-- > 1)"),
            ("ExtraCell", "m_map.m_mapWidth * m_map.m_mapHeight;", "m_map.m_mapWidth * m_map.m_mapHeight + 1;"),
        )
        for label, before, after in controls:
            self.assertIn(before, self.module.BASELINE)
            candidate(label, self.module.BASELINE.replace(before, after))
            checks.append('if (check<' + label + '>()) { std::fprintf(stderr, "missed ' + label + '\\n"); return 2; }')
        program = template
        for marker, replacement in (
                ("VALUE_TYPES", "\n".join(types)),
                ("CELL_FIELDS", fields(block("struct TRmgMapItem {"), (
                    "m_objects", "m_previousTile", "m_movement", "m_zoneState", "m_tile", "m_tileData", "m_connection"))),
                ("MAP_FIELDS", fields(block("class type_random_map :"), (
                    "m_ownsMapItems", "m_mapItems", "m_mapWidth", "m_mapHeight", "m_numberLevels"))),
                ("GROUP_FIELDS", fields(block("struct TRmgTreasureGroup {"), (
                    "m_map", "m_bounds", "m_objects", "m_outline", "m_hasGuard", "m_guardPosition", "m_position", "m_ready"))),
                ("HELPERS", "\n".join(definitions)), ("CANDIDATES", "\n".join(programs)), ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", replacement)
        self.assertNotIn("// @", program)
        with tempfile.TemporaryDirectory(prefix="rmg-group-reset-oracle-") as raw:
            path = Path(raw) / "oracle.cpp"
            path.write_text(program)
            executable = Path(raw) / "oracle"
            compiled = subprocess.run(["g++", "-std=c++98", "-O1", "-I", str(self.root / "include"),
                                       str(path), "-o", str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(compiled.returncode, 0, compiled.stderr[-10000:])
            checked = subprocess.run([str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)


if __name__ == "__main__":
    unittest.main()
