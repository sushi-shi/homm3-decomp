"""Recovered island fill and its formerly omitted caller operation."""
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


class IslandFillTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-island-fill-family.py")
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

    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_fixed_point_reference_and_caller_integration(self):
        header = (self.root / "include/rmg.h").read_text()
        support = (self.root / "src/rmg_support.cpp").read_text()
        def block(name):
            start = header.index("struct " + name + " {")
            return header[start:header.index("\n};", start) + 3]
        types = "\n".join(block(n) for n in ("TRmgVector", "TPoint", "TRmgMapPosition", "TRmgZoneCellState", "TRmgGroundTileData"))
        helper = "\n".join(self.module.helpers().definition(support, n) for n in ("TRmgMapPosition::TRmgMapPosition", "TRmgVector::length"))
        helper += "\n" + "\n".join(self.module.helpers().definition(self.source, n) for n in (
            "TRmgMapPosition::operator+=", "TRmgZone::getLevelPosition", "TRmgVector::operator*", "TRmgVector::operator/"))
        caller = self.module.helpers().definition(self.source, "type_random_map_generator::insetIslandZone")
        self.assertTrue(caller.endswith("    fillIslandInterior(zone);\n}"))
        methods, checks = [], []
        def candidate(name, body, good, call=caller):
            methods.append("struct " + name + " : Root { void fillIslandInterior(TRmgZone*); void insetIslandZone(TRmgZone*); };\n"
                + body.replace("type_random_map_generator::", name + "::") + "\n" + call.replace("type_random_map_generator::", name + "::"))
            condition = ("!check<" + name + ">(false) || !check<" + name + ">(true)") if good else ("check<" + name + ">(true)")
            checks.append('if (' + condition + ') { std::fprintf(stderr, "failed ' + name + '\\n"); return 1; }')
        for i, (_, body) in enumerate(self.module.forms()):
            candidate("Candidate" + str(i), body, True)
        forms = list(self.module.forms())
        original = self.module.helpers().definition(self.source, self.module.FUNCTION)
        candidate("Adopted", original, True)
        parents = [(str(i), self.source.replace(original, forms[i][1])) for i in (53, 52, 51, 15, 16, 17, 4, 1, 36, 40)]
        frontier = self.module.make_parent_axes(self.source, parents)
        self.assertEqual(len(frontier[0]["options"]), 60)
        for i, option in enumerate(frontier[0]["options"]):
            candidate("Frontier" + str(i), option["replace"], True)
        for name, before, after in (
            ("Diagonal", "direction += 2", "++direction"),
            ("Zone", "!= zoneIndex", "== zoneIndex"),
            ("Blocked", "item->isZoneBoundary()", "item->isZoneBoundary() || item->m_tileData.m_roadEntrance"),
            ("Level", "next.m_y, next.m_z", "next.m_y, 0"),
            ("Extent", "next.m_x >= m_map.m_mapWidth", "next.m_x >= m_map.m_mapWidth - 1"),
            ("Seed", "pending.push_back(position);", "")):
            self.assertIn(before, self.module.BASELINE)
            candidate("Wrong" + name, self.module.BASELINE.replace(before, after), False)
        candidate("WrongMissingFill", self.module.BASELINE, False, caller.replace("    fillIslandInterior(zone);\n", ""))
        candidate("WrongEarlyFill", self.module.BASELINE, False, caller.replace("    fillIslandInterior(zone);\n", "").replace("    while (count--)", "    fillIslandInterior(zone);\n    while (count--)"))
        # Scripted edge writes in the fixture make the early-fill control
        # observable without replacing the recovered fill's implementation.
        program = (self.root / "scripts/experiments/rmg-island-fill-oracle.cpp").read_text()
        direction = re.search(r"TPoint g_rmgDirections\[[^]]+\] = \{.*?\n};", self.source, re.S)[0].replace("RMG_DIRECTION_COUNT", "8")
        for marker, value in (("TYPES", types), ("HELPERS", helper), ("DIRECTIONS", direction),
                ("BOUNDARY_QUERY", re.search(r"unsigned char isZoneBoundary\(\) const\s*\{[^}]+}", header)[0]),
                ("ACCESSOR", re.search(r"inline TRmgMapItem\* getMapItem\(int x, int y, int z\)\s*\{[^}]+}", header)[0]),
                ("CANDIDATES", "\n".join(methods)), ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", value)
        with tempfile.TemporaryDirectory(prefix="rmg-island-fill-oracle-") as raw:
            path = Path(raw) / "oracle.cpp"; path.write_text(program)
            executable = Path(raw) / "oracle"
            result = subprocess.run(["g++", "-std=c++98", "-O1", "-fno-elide-constructors", str(path), "-o", str(executable)], capture_output=True, text=True, timeout=180)
            self.assertEqual(result.returncode, 0, result.stderr[-5000:])
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=180)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
