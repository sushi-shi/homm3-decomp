"""Centroid source families preserve signed zone filtering and empty zones."""
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


class RecenterTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-recenter-family.py")
        self.source = (self.root / self.module.SOURCE).read_text()

    def test_60_states_roundtrip(self):
        payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=self.module.make_axes(self.source))
        with tempfile.TemporaryDirectory() as raw:
            path = Path(raw) / "input.json"; path.write_text(json.dumps(payload))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual(len(axes[0].options), 60)
        self.assertEqual(source_families.render(originals, axes, (0,)), originals)
        for i in range(60):
            source = source_families.render(originals, axes, (i,))[self.module.SOURCE]
            self.assertEqual(len(self.module.make_axes(source)[0]["options"]), 60)
        with self.assertRaisesRegex(ValueError, "review current"):
            self.module.make_axes(self.source.replace("total.m_x += x", "total.m_x += y"))

    def test_entry_binding_admission(self):
        axes = self.module.entry_binding_axes(self.source)
        payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes)
        with tempfile.TemporaryDirectory() as raw:
            path = Path(raw) / "input.json"
            path.write_text(json.dumps(payload))
            _, originals, loaded = source_families.load_manifest(path, self.root)
        self.assertEqual(len(loaded[0].options), 60)
        self.assertEqual(source_families.render(originals, loaded, (0,)), originals)
        original = self.module.helpers().definition(self.source, self.module.FUNCTION)
        for _, body in self.module.zeroing_refinements(original):
            self.assertEqual(body.count("total.m_z = position.m_z;"), 1)
            self.assertLess(body.index("position = zone->getLevelPosition();"), body.index("total.m_z = position.m_z;"))
        with self.assertRaisesRegex(ValueError, "review recenter entry-binding"):
            self.module.entry_binding_axes(self.source.replace("int count = 0;\n    TRmgMapPosition total;", "int count = 1;\n    TRmgMapPosition total;"))

    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_flat_reference_and_negative_controls(self):
        header = (self.root / "include/rmg.h").read_text()
        support = (self.root / "src/rmg_support.cpp").read_text()
        def block(name):
            start = header.index("struct " + name + " {")
            return header[start:header.index("\n};", start) + 3]
        types = "\n".join(block(n) for n in ("TRmgVector", "TPoint", "TRmgMapPosition", "TRmgZoneBounds", "TRmgZoneCellState"))
        helper = self.module.helpers().definition(support, "TRmgMapPosition::TRmgMapPosition")
        helper += "\n" + "\n".join(self.module.helpers().definition(self.source, "TRmgZone::" + n) for n in ("getLevelPosition", "setLevelPosition"))
        methods, checks = [], []
        def candidate(name, body, good):
            methods.append("struct " + name + " : Root { void recenterZone(TRmgZone*); };\n" + body.replace("type_random_map_generator::", name + "::"))
            checks.append('if (' + ('!' if good else '') + 'check<' + name + '>()) { std::fprintf(stderr, "failed ' + name + '\\n"); return 1; }')
        for i, (_, body) in enumerate(self.module.forms()):
            candidate("Candidate" + str(i), body, True)
        forms = list(self.module.forms())
        original = self.module.helpers().definition(self.source, self.module.FUNCTION)
        parents = []
        for i in (28, 19, 40, 3, 39, 30, 52, 5, 29, 41):
            parents.append((str(i), self.source.replace(original, forms[i][1])))
            for j, (_, body) in enumerate(self.module.refinements(forms[i][1])):
                candidate("Refined" + str(i) + "_" + str(j), body, True)
        self.assertEqual(len(self.module.make_parent_axes(self.source, parents)[0]["options"]), 60)
        second_parents = []
        for number, (i, mutation) in enumerate(((28, "level_sum"), (19, "level_sum"), (52, "level_sum"), (40, "level_sum"),
                (28, None), (19, None), (40, "split_count"), (3, "count_after_bounds"), (39, "count_after_bounds"), (30, "count_after_bounds"))):
            body = forms[i][1]
            if mutation:
                body = dict(self.module.refinements(body))[mutation]
            second_parents.append((str(number), self.source.replace(original, body)))
        third = self.module.make_parent_axes(self.source, second_parents, scheduling=True)
        self.assertEqual(len(third[0]["options"]), 60)
        for i, option in enumerate(third[0]["options"]):
            candidate("Scheduled" + str(i), option["replace"], True)
        balanced = self.module.make_parent_axes(self.source, second_parents, scheduling=True, balanced=True)
        self.assertEqual(len(balanced[0]["options"]), 60)
        labels = "\n".join(option["name"] for option in balanced[0]["options"])
        for family in ("copy", "direct_copy", "assigned_copy", "fields_xyz", "fields_xzy", "fields_yxz", "fields_yzx", "fields_zxy", "fields_zyx"):
            self.assertIn("+" + family, labels)
        for i, option in enumerate(balanced[0]["options"]):
            candidate("Balanced" + str(i), option["replace"], True)
        entries = self.module.entry_binding_axes(self.source)
        self.assertEqual(len(entries[0]["options"]), 60)
        for i, option in enumerate(entries[0]["options"]):
            candidate("EntryBinding" + str(i), option["replace"], True)
        candidate("Authored", original, True)
        if os.environ.get("HOMM3_RECENTER_MANIFEST"):
            manifest = Path(os.environ["HOMM3_RECENTER_MANIFEST"])
            if not manifest.is_absolute():
                manifest = self.root / manifest
            _, originals, axes = source_families.load_manifest(manifest, self.root)
            for i in range(len(axes[0].options)):
                rendered = source_families.render(originals, axes, (i,))[self.module.SOURCE]
                candidate("Manifest" + str(i), self.module.helpers().definition(rendered, self.module.FUNCTION), True)
        baseline = list(self.module.forms())[0][1]
        for name, before, after in (
            ("Zone", "== zoneIndex", "!= zoneIndex"),
            ("Level", "x, y, position.m_z", "x, y, 0"),
            ("X", "total.m_x += x", "total.m_x += y"),
            ("Y", "total.m_y += y", "total.m_y += x"),
            ("Count", "++count", "count += 2"),
            ("Empty", "if (count)", "if (count > 1)"),
            ("KeepLevel", "zone->setLevelPosition(position);", "position.m_z = 0; zone->setLevelPosition(position);")):
            self.assertIn(before, baseline)
            candidate("Wrong" + name, baseline.replace(before, after), False)
        candidate("WrongSlotWrite", baseline.replace("    int count = 0;", "    ++zone->m_slot->m_zoneIndex;\n    int count = 0;"), False)
        program = (self.root / "scripts/experiments/rmg-recenter-oracle.cpp").read_text()
        for marker, value in (("TYPES", types), ("HELPERS", helper),
                ("ACCESSOR", re.search(r"inline TRmgMapItem\* getMapItem\(int x, int y, int z\)\s*\{[^}]+}", header)[0]),
                ("CANDIDATES", "\n".join(methods)), ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", value)
        with tempfile.TemporaryDirectory(prefix="rmg-recenter-oracle-") as raw:
            path = Path(raw) / "oracle.cpp"; path.write_text(program)
            executable = Path(raw) / "oracle"
            result = subprocess.run(["g++", "-std=c++98", "-O1", "-fno-elide-constructors", str(path), "-o", str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stderr[-5000:])
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
