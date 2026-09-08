"""Admission and independent lifecycle controls for fill source families."""
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


class RmgGroupFillTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-group-fill-family.py")
        self.source = (self.root / self.module.SOURCE).read_text()

    def test_60_distinct_states_roundtrip_and_reject_behavior_changes(self):
        payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                       axes=self.module.make_axes(self.source))
        with tempfile.TemporaryDirectory(prefix="rmg-group-fill-manifest-") as raw:
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
            self.module.make_axes(self.source.replace("total += objectValue;", "total += 1;"))

    def test_frontier_retains_ten_distinct_parents(self):
        current = self.module.helpers().definition(self.source, self.module.FUNCTION)
        source = self.source.replace(current, self.module.BASELINE)
        forms = list(self.module.forms())
        parents = [("parent" + str(i), source.replace(self.module.BASELINE, forms[index][1]))
                   for i, index in enumerate((0, 12, 1, 13, 24, 37, 46, 22, 33, 9))]
        options = self.module.make_parent_axes(source, parents)[0]["options"]
        self.assertEqual(len(options), 60)
        self.assertEqual(len({option["replace"] for option in options}), 60)
        self.assertEqual(options[0]["replace"], self.module.BASELINE)
        for _, text in parents:
            body = self.module.helpers().definition(text, self.module.FUNCTION)
            self.assertIn(body, {option["replace"] for option in options})
        self.assertTrue({option["replace"] for option in options} <= self.module.admitted_bodies())

    def test_centering_frontier_retains_scoped_parents(self):
        current = self.module.helpers().definition(self.source, self.module.FUNCTION)
        source = self.source.replace(current, self.module.BASELINE)
        forms = list(self.module.forms())
        selected = [(index, "initial_object_phase_scope") for index in (0, 12, 1, 13, 24)]
        selected += [(37, "accepted_object_scope"), (0, "map_pointer"),
                     (0, None), (1, "const_prototype"), (1, "map_pointer")]
        parents = []
        for i, (index, refinement) in enumerate(selected):
            body = forms[index][1]
            if refinement:
                body = dict(self.module.refinements(body))[refinement]
            parents.append(("parent" + str(i), source.replace(self.module.BASELINE, body)))
        options = self.module.make_parent_axes(source, parents, centering=True)[0]["options"]
        self.assertEqual(len(options), 60)
        self.assertEqual(len({option["replace"] for option in options}), 60)
        for _, text in parents:
            body = self.module.helpers().definition(text, self.module.FUNCTION)
            self.assertIn(body, {option["replace"] for option in options})
        self.assertTrue({option["replace"] for option in options} <= self.module.admitted_bodies())

    @unittest.skipUnless(shutil.which("g++"), "fill contract oracle needs g++")
    def test_all_states_against_independent_lifecycle_oracle(self):
        header = (self.root / "include/rmg.h").read_text()
        object_header = (self.root / "include/advmgr_objects.h").read_text()
        support = (self.root / "src/rmg_support.cpp").read_text()
        program = (self.root / "scripts/experiments/rmg-group-fill-oracle.cpp").read_text()

        def block(text, marker):
            at = text.index(marker)
            return text[at:text.index("\n};", at) + 3]

        types = [block(header, "struct " + name + " {") for name in
                 ("TRmgVector", "TPoint", "TRmgMapPosition")]
        accessors = [line.strip() for line in object_header.splitlines()
                     if re.match(r"\s*int get(?:Width|Height)\(\) const \{", line)]
        self.assertEqual(len(accessors), 2)
        methods = dict(self.module.forms())
        second = {body for _, body in self.module.forms()}
        second |= {body for parent in tuple(second) for _, body in self.module.refinements(parent)}
        methods.update(("admitted_second_" + str(i), body) for i, body in enumerate(sorted(second)))
        methods.update(self.module.center_refinements(self.module.BASELINE))
        methods["current"] = self.module.helpers().definition(self.source, self.module.FUNCTION)
        if os.environ.get("HOMM3_GROUP_FILL_MANIFEST"):
            _, originals, axes = source_families.load_manifest(Path(os.environ["HOMM3_GROUP_FILL_MANIFEST"]), self.root)
            for i in range(60):
                source = source_families.render(originals, axes, (i,))[self.module.SOURCE]
                methods["manifest_" + str(i)] = self.module.helpers().definition(source, self.module.FUNCTION)
        programs, checks = [], []

        def candidate(label, method):
            programs.append("struct " + label + " : FillRoot { int fillTreasureGroup("
                            "TRmgZone*, TRmgTreasureGroup*, unsigned char, int); };\n"
                            + method.replace("type_random_map_generator::", label + "::"))

        for i, method in enumerate(dict.fromkeys(methods.values())):
            label = "Candidate" + str(i)
            candidate(label, method)
            checks.append('if (!check<' + label + '>()) { std::fprintf(stderr, "failed state ' + str(i) + '\\n"); return 1; }')
        controls = (
            ("WrongPrimary", "1, 1, alternate, position", "0, 1, alternate, position"),
            ("WrongAlternate", "0, 1, alternate, unspecified", "0, 1, 0, unspecified"),
            ("WrongMinimum", "zone, value / 4, value", "zone, value / 2, value"),
            ("WrongMaximum", "5 * remainder / 4", "remainder"),
            ("WrongCenter", "prototype->getWidth()", "prototype->getHeight()"),
            ("WrongLevel", "position.m_z = 0;", "position.m_z = -1;"),
            ("EarlySnapshot", "    group->m_map.addObject(object, position);\n    int total = objectValue;",
             "    int total = objectValue;\n    group->m_map.addObject(object, position);"),
            ("WrongFirstBudget", "attempts < RMG_TREASURE_ATTEMPTS", "attempts < 2"),
            ("WrongCreateBudget", "creationAttempts < RMG_TREASURE_ATTEMPTS", "creationAttempts < 2"),
            ("WrongFitBudget", "++attempts >= RMG_TREASURE_ATTEMPTS", "++attempts >= 2"),
            ("MissingRelease", "nextObject->unknownOperation();", ";"),
            ("MissingBounds", "group->updateBounds();", ";"),
            ("WrongReturn", "return total;", "return value;"),
            ("WrongRemainder", "remainder < RMG_TREASURE_MINIMUM_REMAINDER && remainder < total / 2",
             "remainder < RMG_TREASURE_MINIMUM_REMAINDER || remainder < total / 2"),
        )
        for label, before, after in controls:
            self.assertIn(before, self.module.BASELINE)
            candidate(label, self.module.BASELINE.replace(before, after))
            checks.append('if (check<' + label + '>()) { std::fprintf(stderr, "missed ' + label + '\\n"); return 2; }')
        for marker, replacement in (("VALUE_TYPES", "\n".join(types)),
                ("ACCESSORS", "\n".join(accessors)),
                ("LIMITS", block(header, "enum ERmgTreasurePlacementLimits {")),
                ("VALUE_HELPERS", self.module.helpers().definition(support, "TRmgMapPosition::TRmgMapPosition")),
                ("CANDIDATES", "\n".join(programs)), ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", replacement)
        self.assertNotIn("// @", program)
        with tempfile.TemporaryDirectory(prefix="rmg-group-fill-oracle-") as raw:
            path = Path(raw) / "oracle.cpp"
            path.write_text(program)
            executable = Path(raw) / "oracle"
            compiled = subprocess.run(["g++", "-std=c++98", "-O1", "-fno-elide-constructors",
                                       str(path), "-o", str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(compiled.returncode, 0, compiled.stderr[-10000:])
            checked = subprocess.run([str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)


if __name__ == "__main__":
    unittest.main()
