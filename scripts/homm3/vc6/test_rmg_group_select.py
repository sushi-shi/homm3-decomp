"""Independent semantic controls for treasure-position candidate selection."""
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


class RmgGroupSelectTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-group-select-family.py")
        self.source = (self.root / self.module.SOURCE).read_text()

    def test_60_states_rebase_without_accepting_changed_selection_rules(self):
        payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                       axes=self.module.make_axes(self.source))
        with tempfile.TemporaryDirectory(prefix="rmg-group-select-manifest-") as raw:
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
        with self.assertRaisesRegex(ValueError, "review the current"):
            self.module.make_axes(self.source.replace("candidates[rand() % candidates.size()]", "candidates.front()"))

    def test_parent_frontier_retains_ten_sources_and_49_new_states(self):
        current = self.module.helpers().definition(self.source, self.module.FUNCTION)
        source = self.source.replace(current, self.module.BASELINE)
        forms = list(self.module.forms())
        parents = [("parent" + str(i), source.replace(self.module.BASELINE, forms[index][1]))
                   for i, index in enumerate((11, 59, 23, 35, 18, 6, 42, 7, 55, 43))]
        options = self.module.make_parent_axes(source, parents)[0]["options"]
        self.assertEqual(len(options), 60)
        self.assertEqual(len({option["replace"] for option in options}), 60)
        self.assertEqual(options[0]["replace"], self.module.BASELINE)
        for label, text in parents:
            body = self.module.helpers().definition(text, self.module.FUNCTION)
            self.assertIn(dict(name=label, replace=body), options)
        self.assertTrue({option["replace"] for option in options} <= self.module.admitted_bodies())

    def test_shape_frontier_preserves_parents_and_ordered_guard_compositions(self):
        current = self.module.helpers().definition(self.source, self.module.FUNCTION)
        source = self.source.replace(current, self.module.BASELINE)
        forms = list(self.module.forms())
        selected = ((11, "bounds_assigned"), (59, None), (23, None), (23, "insert_single"),
                    (35, None), (0, "erase_named_reverse"), (35, "insert_single"),
                    (6, "insert_single"), (6, None), (54, "selection_reference"))
        parents = []
        for i, (index, refinement) in enumerate(selected):
            text = forms[index][1]
            if refinement:
                text = dict(self.module.refinements(text))[refinement]
            parents.append(("parent" + str(i), source.replace(self.module.BASELINE, text)))
        options = self.module.make_parent_axes(source, parents, shape=True)[0]["options"]
        self.assertEqual(len(options), 60)
        self.assertEqual(len({option["replace"] for option in options}), 60)
        for label, text in parents:
            body = self.module.helpers().definition(text, self.module.FUNCTION)
            self.assertIn(dict(name=label, replace=body), options)
        self.assertTrue({option["replace"] for option in options} <= self.module.admitted_bodies())
        self.assertTrue(any("center_point_sum" in option["name"] for option in options))
        self.assertTrue(any("continue_filters" in option["name"] for option in options))

    def test_lifetime_frontier_only_moves_existing_value_declarations(self):
        current = self.module.helpers().definition(self.source, self.module.FUNCTION)
        source = self.source.replace(current, self.module.BASELINE)
        forms = list(self.module.forms())
        parents = []
        for index in (6, 11, 23, 35, 59):
            selected = dict(self.module.refinements(forms[index][1]))["insert_single"]
            for shape in ("center_point_compound", "continue_filters"):
                body = dict(self.module.shape_refinements(selected))[shape]
                parents.append((str(index) + "+" + shape, source.replace(self.module.BASELINE, body)))
        options = self.module.make_parent_axes(source, parents, lifetime=True)[0]["options"]
        self.assertEqual(len(options), 60)
        self.assertEqual(len({option["replace"] for option in options}), 60)
        admitted = {self.module.BASELINE}
        for label, text in [("baseline", source), *parents]:
            body = self.module.helpers().definition(text, self.module.FUNCTION)
            admitted.add(body)
            admitted.update(text for _, text in self.module.lifetime_refinements(body))
            if label != "baseline":
                self.assertIn(dict(name=label, replace=body), options)
        self.assertTrue({option["replace"] for option in options} <= admitted)
        self.assertTrue(any("position_" in option["name"] for option in options))
        self.assertTrue(any("query_" in option["name"] for option in options))

    @unittest.skipUnless(shutil.which("g++"), "selection contract oracle needs g++")
    def test_all_states_against_independent_footprint_and_selection_oracle(self):
        header = (self.root / "include/rmg.h").read_text()
        support = (self.root / "src/rmg_support.cpp").read_text()
        template = (self.root / "scripts/experiments/rmg-group-select-oracle.cpp").read_text()

        def block(text, start):
            at = text.index(start)
            return text[at:text.index("\n};", at) + 3]

        def fields(text, names):
            result = []
            for name in names:
                matches = [line.split("//")[0].strip() for line in text.splitlines()
                           if re.search(r"\b" + name + r";", line.split("//")[0])]
                self.assertEqual(len(matches), 1, name)
                result.append(matches[0])
            return "\n".join(result)

        helper = self.module.helpers()
        types = [block(header, "struct " + name + " {") for name in
                 ("TRmgVector", "TPoint", "TRmgMapPosition", "TRmgZoneBounds", "TRmgZoneCellState")]
        value_helpers = [helper.definition(support, "TRmgMapPosition::TRmgMapPosition")]
        value_helpers += [helper.definition(self.source, name,
                          parameters="TRmgMapPosition point" if name == "type_random_map::getMapItem" else None) for name in
                          ("TRmgZone::getLevelPosition", "type_random_map::getMapItem",
                           "TRmgMapPosition::operator+", "TRmgMapPosition::operator+=")]
        at = self.source.index("TPoint operator+(TPoint point, TRmgVector offset)\n")
        value_helpers.append(self.source[at:self.source.index("\n}", at) + 2])
        at = header.index("    inline TRmgMapItem* getMapItem(int x, int y, int z)")
        lookup = header[at:header.index("\n    }", at) + 6]
        lookup = lookup.replace("    {\n", "    {\n        record(x, y, z);\n", 1)
        methods = dict(self.module.forms())
        methods["current"] = helper.definition(self.source, self.module.FUNCTION)
        if os.environ.get("HOMM3_GROUP_SELECT_MANIFEST"):
            _, originals, axes = source_families.load_manifest(Path(os.environ["HOMM3_GROUP_SELECT_MANIFEST"]), self.root)
            for i in range(60):
                source = source_families.render(originals, axes, (i,))[self.module.SOURCE]
                methods["manifest_" + str(i)] = helper.definition(source, self.module.FUNCTION)
        programs, checks = [], []

        def candidate(label, body):
            programs.append("struct " + label + " : SelectionRoot { unsigned char placeTreasureGroup("
                            "TRmgTreasureGroup*, TRmgZone*, int); };\n"
                            + body.replace("type_random_map_generator::", label + "::"))

        for i, method in enumerate(dict.fromkeys(methods.values())):
            label = "Candidate" + str(i)
            candidate(label, method)
            checks.append('if (!check<' + label + '>()) { std::fprintf(stderr, "failed state ' + str(i) + '\\n"); return 1; }')
        controls = (
            ("MissingClear", "candidates.clear();", ";"),
            ("WrongTie", "m_score >= spacing", "m_score > spacing"),
            ("WrongScoreUpdate", "spacing = item->m_zoneState.m_score;", ";"),
            ("WrongZone", "item->m_zoneState.m_zone == zoneIndex", "item->m_zoneState.m_zone != zoneIndex"),
            ("WrongZoneSnapshot", "m_zone == zoneIndex", "m_zone == zone->m_slot->m_zoneIndex"),
            ("WrongRetest", "&& canPlaceTreasureGroup(group, position, zone))", "&& canPlaceTreasureGroup(group, position, zone)\n"
             "                && item->m_zoneState.m_score >= spacing)"),
            ("WrongCenter", "(groupBounds.m_minimumX + groupBounds.m_maximumX) / 2", "groupBounds.m_maximumX"),
            ("WrongFitGroup", "canPlaceTreasureGroup(group, position, zone)", "canPlaceTreasureGroup(0, position, zone)"),
            ("WrongChoice", "rand() % candidates.size()", "rand() % 1"),
            ("MissingRandom", "rand() % candidates.size()", "0"),
            ("WrongCommitLevel", "commitTreasureGroup(group, position);", "position.m_z = 0;\n    commitTreasureGroup(group, position);"),
            ("WrongOrder", "    for (; position.m_y < bounds.m_maximumY; ++position.m_y) {\n"
             "        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {",
             "    for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {\n"
             "        for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {"),
        )
        for label, before, after in controls:
            self.assertIn(before, self.module.BASELINE)
            candidate(label, self.module.BASELINE.replace(before, after))
            checks.append('if (check<' + label + '>()) { std::fprintf(stderr, "missed ' + label + '\\n"); return 2; }')
        program = template
        for marker, replacement in (("VALUE_TYPES", "\n".join(types)),
                ("SLOT_FIELDS", fields(block(header, "struct TRmgTownSlot {"), ("m_zoneIndex",))),
                ("ZONE_FIELDS", fields(block(header, "struct TRmgZone {"), ("m_slot", "m_bounds", "m_levelPosition"))),
                ("GROUP_FIELDS", fields(block(header, "struct TRmgTreasureGroup {"), ("m_bounds",))),
                ("MAP_FIELDS", fields(block(header, "class type_random_map :"), ("m_mapItems", "m_mapWidth", "m_mapHeight"))),
                ("SCALAR_LOOKUP", lookup), ("VALUE_HELPERS", "\n".join(value_helpers)),
                ("CANDIDATES", "\n".join(programs)), ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", replacement)
        self.assertNotIn("// @", program)
        with tempfile.TemporaryDirectory(prefix="rmg-group-select-oracle-") as raw:
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
