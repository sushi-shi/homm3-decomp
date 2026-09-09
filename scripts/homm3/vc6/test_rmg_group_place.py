"""Independent caller-contract checks for the 60 group-placement states."""
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


class RmgGroupPlaceTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-group-place-family.py")
        self.source = (self.root / self.module.SOURCE).read_text()

    def historical_source(self):
        # These frontier builders consume checkpoints from before adoption;
        # their strict loader must not reinterpret those scores after an edit.
        # Keep the original control for stage-construction fixtures, while the
        # rebasing and native-contract tests below exercise the actual source.
        current = self.module.helpers().definition(self.source, self.module.FUNCTION)
        return self.source.replace(current, self.module.BASELINE)

    def test_60_states_rebase_but_changed_placement_rules_do_not(self):
        payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                       axes=self.module.make_axes(self.source))
        with tempfile.TemporaryDirectory(prefix="rmg-group-place-manifest-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual(len(axes[0].options), 60)
        self.assertEqual(source_families.render(originals, axes, (0,)), originals)
        states = [source_families.render(originals, axes, (i,))[self.module.SOURCE] for i in range(60)]
        self.assertEqual(len(set(states)), 60)
        for source in states:
            axis = self.module.make_axes(source)[0]
            self.assertEqual(axis["find"], axis["options"][0]["replace"])
        with self.assertRaisesRegex(ValueError, "review current"):
            self.module.make_axes(self.source.replace("direction == lastDirection", "direction > lastDirection"))

    def test_frontier_keeps_ten_parents_and_49_new_states(self):
        self.source = self.historical_source()
        original = self.module.helpers().definition(self.source, self.module.FUNCTION)
        forms = list(self.module.forms())
        parents = [("parent" + str(i), self.source.replace(original, forms[index][1]))
                   for i, index in enumerate((38, 2, 17, 50, 26, 47, 44, 8, 20, 11))]
        axes = self.module.make_parent_axes(self.source, parents)
        options = axes[0]["options"]
        self.assertEqual(len(options), 60)
        self.assertEqual(len({option["replace"] for option in options}), 60)
        self.assertEqual(options[0]["replace"], original)
        for label, source in parents:
            self.assertIn(dict(name=label, replace=self.module.helpers().definition(source, self.module.FUNCTION)), options)
        self.assertTrue({option["replace"] for option in options} <= self.module.admitted_bodies())
        payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes)
        with tempfile.TemporaryDirectory(prefix="rmg-group-place-frontier-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            fixture = Path(raw) / "src/rmg.cpp"
            fixture.parent.mkdir()
            fixture.write_text(self.source)
            _, originals, parsed = source_families.load_manifest(path, Path(raw))
            if (self.root / self.module.SOURCE).read_text() != self.source:
                with self.assertRaisesRegex(ValueError, "anchor must occur exactly once"):
                    source_families.load_manifest(path, self.root)
        self.assertEqual(source_families.render(originals, parsed, (0,)), originals)

    def test_lifetime_frontier_preserves_the_copy_and_join_parents(self):
        self.source = self.historical_source()
        original = self.module.helpers().definition(self.source, self.module.FUNCTION)
        forms = list(self.module.forms())
        selected = ((38, "guard_point_copy"), (2, "guard_point_copy"), (17, "guard_point_copy"),
                    (38, "allow_else_join"), (2, "allow_else_join"), (17, "neighbor_direct"),
                    (2, "neighbor_assigned"), (38, None), (2, "neighbor_direct"), (2, "guard_point_reference"))
        parents = []
        for i, (index, refinement) in enumerate(selected):
            text = forms[index][1]
            if refinement:
                text = dict(self.module.refinements(text))[refinement]
            parents.append(("parent" + str(i), self.source.replace(original, text)))
        options = self.module.make_parent_axes(self.source, parents, lifetime=True)[0]["options"]
        self.assertEqual(len(options), 60)
        self.assertEqual(len({option["replace"] for option in options}), 60)
        self.assertEqual(options[0]["replace"], original)
        for label, source in parents:
            self.assertIn(dict(name=label, replace=self.module.helpers().definition(source, self.module.FUNCTION)), options)
        self.assertTrue({option["replace"] for option in options} <= self.module.admitted_bodies())
        self.assertTrue(any("guard_scan_point" in option["name"] for option in options))
        self.assertTrue(any("guard_const_copy" in option["name"] for option in options))

    def test_frame_frontier_keeps_60_states_and_both_real_lookup_overloads(self):
        self.source = self.historical_source()
        original = self.module.helpers().definition(self.source, self.module.FUNCTION)
        forms = list(self.module.forms())
        parents = []
        for index in (2, 17, 38):
            copied = dict(self.module.refinements(forms[index][1]))["guard_point_copy"]
            for label, text in self.module.lifetime_refinements(copied):
                parents.append((str(index) + "+" + label, self.source.replace(original, text)))
                if len(parents) == 10:
                    break
            if len(parents) == 10:
                break
        options = self.module.make_parent_axes(self.source, parents, frame=True)[0]["options"]
        self.assertEqual(len(options), 60)
        self.assertEqual(len({option["replace"] for option in options}), 60)
        for label, source in parents:
            self.assertIn(dict(name=label, replace=self.module.helpers().definition(source, self.module.FUNCTION)), options)
        self.assertTrue({option["replace"] for option in options} <= self.module.admitted_bodies())
        self.assertTrue(any("getMapItem(point)" in option["replace"] for option in options))
        self.assertTrue(any("getMapItem(point.m_x, point.m_y, point.m_z)" in option["replace"] for option in options))

    def test_shared_frontier_keeps_the_ten_parents_and_only_phase_reuses(self):
        self.source = self.historical_source()
        original = self.module.helpers().definition(self.source, self.module.FUNCTION)
        forms = list(self.module.forms())
        parents = []
        for index in (2, 17, 38):
            copied = dict(self.module.refinements(forms[index][1]))["guard_point_copy"]
            joined = dict(self.module.lifetime_refinements(copied))["allow_else_join"]
            frames = dict(self.module.frame_refinements(joined))
            for shape in ("scan_position_fields", "scan_position_value_query", "scan_point"):
                parents.append((str(index) + "+" + shape, self.source.replace(original, frames[shape])))
        parents.append(("copy_control", self.source.replace(original, copied)))
        options = self.module.make_parent_axes(self.source, parents, shared=True)[0]["options"]
        self.assertEqual(len(options), 60)
        self.assertEqual(len({option["replace"] for option in options}), 60)
        admitted = {original}
        for label, source in parents:
            body = self.module.helpers().definition(source, self.module.FUNCTION)
            self.assertIn(dict(name=label, replace=body), options)
            admitted.add(body)
            admitted.update(text for _, text in self.module.shared_refinements(body))
        admitted.update(text for _, text in self.module.shared_refinements(original))
        self.assertTrue({option["replace"] for option in options} <= admitted)

    @unittest.skipUnless(shutil.which("g++"), "group-placement oracle needs g++")
    def test_all_states_against_flat_cell_oracle_and_negative_controls(self):
        header = (self.root / "include/rmg.h").read_text()
        support = (self.root / "src/rmg_support.cpp").read_text()
        template = (self.root / "scripts/experiments/rmg-group-place-oracle.cpp").read_text()

        def block(text, start):
            at = text.index(start)
            return text[at:text.index("\n};", at) + 3]

        def fields(text, names):
            declarations = []
            for name in names:
                matches = [line.split("//")[0].strip() for line in text.splitlines()
                           if re.search(r"\b" + name + r";", line.split("//")[0])]
                self.assertEqual(len(matches), 1, name)
                declarations.append(matches[0])
            return "\n".join(declarations)

        types = [block(header, "struct " + name + " {") for name in
                 ("TRmgVector", "TPoint", "TRmgMapPosition", "TRmgZoneBounds", "TRmgGroundTile", "TRmgGroundTileData")]
        types.append(block(header, "enum ERmgDirectionLimits {"))
        predicates = []
        for name in ("isRoadEntrance", "hasSubterraneanGate", "isPlacementOutline"):
            at = header.index("    unsigned char " + name + "() const")
            predicates.append(header[at:header.index("\n    }", at) + 6])
        at = header.index("    inline TRmgMapItem* getMapItem(int x, int y, int z)")
        lookup = header[at:header.index("\n    }", at) + 6]
        lookup = lookup.replace("    {\n", "    {\n        record(x, y, z);\n", 1)
        helper = self.module.helpers()
        value_helpers = [helper.definition(support, "TRmgMapPosition::TRmgMapPosition"),
                         helper.definition(self.source, "type_object::getPosition"),
                         helper.definition(self.source, "type_random_map::getMapItem",
                                           parameters="TRmgMapPosition point")]
        # Select the canonical point/vector overload, not another operator+.
        at = self.source.index("TPoint operator+(TPoint point, TRmgVector offset)\n")
        value_helpers.append(self.source[at:self.source.index("\n}", at) + 2])
        prototype = block((self.root / "include/advmgr_objects.h").read_text(), "struct TObjectType {")
        at = prototype.index("    struct TPoint {")
        prototype_point = prototype[at:prototype.index("\n    };", at) + 7]
        methods = dict(self.module.forms())
        methods["current"] = helper.definition(self.source, self.module.FUNCTION)
        for manifest_index, selected in enumerate(filter(None, os.environ.get("HOMM3_GROUP_PLACE_MANIFEST", "").split(":"))):
            path = Path(selected)
            if not path.is_absolute():
                path = self.root / path
            _, originals, axes = source_families.load_manifest(path, self.root)
            for i in range(len(axes[0].options)):
                source = source_families.render(originals, axes, (i,))[self.module.SOURCE]
                methods["manifest_" + str(manifest_index) + "_" + str(i)] = helper.definition(source, self.module.FUNCTION)
        programs, checks = [], []

        def candidate(label, body):
            programs.append("struct " + label + " : PlaceRoot { unsigned char canPlaceTreasureGroup("
                            "TRmgTreasureGroup*, TRmgMapPosition, TRmgZone*); };\n"
                            + body.replace("type_random_map_generator::", label + "::"))

        for i, method in enumerate(dict.fromkeys(methods.values())):
            label = "Candidate" + str(i)
            candidate(label, method)
            checks.append('if (!check<' + label + '>()) { std::fprintf(stderr, "failed state ' + str(i) + '\\n"); return 1; }')
        controls = (
            ("WrongGuardKind", "== MONSTER)", "!= MONSTER)"),
            ("WrongGuardElement", "item->m_objects[0]", "item->m_objects[1]"),
            ("WrongGuardLevel", "getMapItem(x, y, guardPosition.m_z)", "getMapItem(x, y, 1 - guardPosition.m_z)"),
            ("WrongLastObject", "group->m_objects.back()", "group->m_objects.front()"),
            ("WrongRange", "firstDirection = 1;", "firstDirection = 2;"),
            ("WrongWater", "== waterZone", "!= waterZone"),
            ("WrongSourceOutline", "|| !source->isPlacementOutline()", ""),
            ("WrongLevel", "point.m_x, point.m_y, position.m_z", "point.m_x, point.m_y, 0"),
            ("WrongOutlinePolicy", "position, allowEntrances, zone, 1", "position, 1, zone, 1"),
            ("WrongCachedZone", "objectPosition, zoneIndex, 1", "objectPosition, zone->m_slot->m_zoneIndex, 1"),
            ("WrongBoundsSnapshot", "point.m_y = bounds.m_minimumY", "point.m_y = group->m_bounds.m_minimumY"),
            ("WrongOccupied", "if (!source->hasSubterraneanGate())", "if (source->hasSubterraneanGate())"),
            ("WrongGuardOrder", "for (int x = guardPosition.m_x - 1; x <= guardPosition.m_x + 1; ++x) {\n"
                "            for (int y = guardPosition.m_y - 1; y <= guardPosition.m_y + 1; ++y) {",
                "for (int y = guardPosition.m_y - 1; y <= guardPosition.m_y + 1; ++y) {\n"
                "            for (int x = guardPosition.m_x - 1; x <= guardPosition.m_x + 1; ++x) {"),
            ("WrongCachedCount", "    for (unsigned int i = 0; i < group->m_objects.size(); ++i) {",
                "    unsigned int count = group->m_objects.size();\n    for (unsigned int i = 0; i < count; ++i) {"),
        )
        for label, before, after in controls:
            self.assertIn(before, self.module.BASELINE)
            candidate(label, self.module.BASELINE.replace(before, after))
            checks.append("if (check<" + label + ">()) { std::fprintf(stderr, \"missed " + label + "\\n\"); return 2; }")
        program = template
        for marker, replacement in (("VALUE_TYPES", "\n".join(types)),
                ("OBJECT_ENUM", block((self.root / "include/mapcell.h").read_text(), "enum TAdventureObjectType {")),
                ("PROTOTYPE_POINT", prototype_point),
                ("PROTOTYPE_FIELDS", fields(prototype, ("m_objectType", "m_triggerCell"))),
                ("PROPERTY_FIELDS", fields(block(header, "struct TRmgObjectPropertiesRef {"), ("m_prototype",))),
                ("OBJECT_FIELDS", fields(block(header, "class type_object {"), ("m_properties", "m_position"))),
                ("GROUP_FIELDS", fields(block(header, "struct TRmgTreasureGroup {"),
                                        ("m_map", "m_bounds", "m_objects", "m_outline", "m_hasGuard", "m_guardPosition"))),
                ("SLOT_FIELDS", fields(block(header, "struct TRmgTownSlot {"), ("m_zoneIndex",))),
                ("ZONE_FIELDS", fields(block(header, "struct TRmgZone {"), ("m_slot", "m_terrain"))),
                ("PREDICATES", "\n".join(predicates)), ("SCALAR_LOOKUP", lookup),
                ("VALUE_HELPERS", "\n".join(value_helpers)),
                ("DIRECTIONS", block(self.source, "TPoint g_rmgDirections[")),
                ("CANDIDATES", "\n".join(programs)), ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", replacement)
        self.assertNotIn("// @", program)
        with tempfile.TemporaryDirectory(prefix="rmg-group-place-oracle-") as raw:
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
