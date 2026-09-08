"""Coastal lifetime family roundtrip and affine-path behavioral controls."""
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


class RiverCoastTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-river-coast-family.py")
        self.source = (self.root / self.module.SOURCE).read_text()

    def test_60_states_roundtrip(self):
        payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=self.module.make_axes(self.source))
        with tempfile.TemporaryDirectory() as raw:
            path = Path(raw) / "input.json"; path.write_text(json.dumps(payload))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual(len(axes[0].options), 60)
        self.assertEqual(source_families.render(originals, axes, (0,)), originals)
        for _, body in self.module.forms():
            self.assertEqual(len(self.module.make_axes(self.source.replace(self.module.helpers().definition(self.source, self.module.FUNCTION), body))[0]["options"]), 60)

    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_affine_reference_and_negative_controls(self):
        header = (self.root / "include/rmg.h").read_text()
        support = (self.root / "src/rmg_support.cpp").read_text()
        terrain = (self.root / "include/terrain_type.h").read_text()
        def block(name):
            start = header.index("struct " + name + " {")
            return header[start:header.index("\n};", start) + 3]
        types = "\n".join(block(n) for n in ("TRmgVector", "TPoint", "TRmgMapPosition", "TRmgGroundTile", "TRmgGroundTileData"))
        types += "\nenum { eTerrainWater = " + re.search(r"eTerrainWater\s*=\s*(\d+)", terrain)[1] + " };"
        methods, checks = [], []
        def candidate(name, body, good):
            # Lift only the legacy for-int scope for the host compiler.
            if "for (count = 0;" in body and "    int count;" not in body:
                body = body.replace("    for (int count = 0;", "    int count;\n    for (count = 0;", 1)
            methods.append("struct " + name + " : Root { void markRiverCoastTarget(TRmgMapPosition, int); };\n" + body.replace("type_random_map_generator::", name + "::"))
            checks.append('if (' + ('!' if good else '') + 'check<' + name + '>()) { std::fprintf(stderr, "failed ' + name + '\\n"); return 1; }')
        for i, (_, body) in enumerate(self.module.forms()):
            candidate("Candidate" + str(i), body, True)
        # Representatives span the full coordinate/step/counter families and
        # include all ten distinct retained parents of the measured first run.
        forms = list(self.module.forms())
        parents = []
        original = self.module.helpers().definition(self.source, self.module.FUNCTION)
        for i in (0, 7, 16, 22, 33, 45, 46, 48, 54, 55):
            parents.append((str(i), self.source.replace(original, forms[i][1])))
            for j, (_, body) in enumerate(self.module.refinements(forms[i][1])):
                candidate("Refined" + str(i) + "_" + str(j), body, True)
        self.assertEqual(len(self.module.make_parent_axes(self.source, parents)[0]["options"]), 60)
        for name, before, after in (
            ("Water", "!= eTerrainWater", "== eTerrainWater"),
            ("Entrance", "item->isRoadEntrance()", "false"),
            ("Inland", "count < 4", "count < 3"),
            ("Shore", "count < 3", "count < 2"),
            ("Bound", "point.m_x > m_map.m_mapWidth", "point.m_x >= m_map.m_mapWidth"),
            ("Direction", "direction - 4", "direction - 2"),
            ("Target", "m_riverTarget = 1", "m_hasRiver = 1")):
            candidate("Wrong" + name, self.module.baseline().replace(before, after), False)
        direction = re.search(r"TPoint g_rmgDirections\[[^]]+\] = \{.*?\n};", self.source, re.S)[0].replace("RMG_DIRECTION_COUNT", "8")
        helper = self.module.helpers().definition(support, "TRmgMapPosition::TRmgMapPosition")
        helper += "\n" + "\n".join(self.module.helpers().definition(self.source, "TRmgMapPosition::operator" + op) for op in ("+", "+="))
        program = (self.root / "scripts/experiments/rmg-river-coast-oracle.cpp").read_text()
        for marker, value in (("TYPES", types), ("HELPERS", helper), ("DIRECTIONS", direction),
                ("ENTRANCE", re.search(r"unsigned char isRoadEntrance\(\) const\s*\{[^}]+}", header)[0]),
                ("ACCESSOR", re.search(r"inline TRmgMapItem\* getMapItem\(int x, int y, int z\)\s*\{[^}]+}", header)[0]),
                ("CANDIDATES", "\n".join(methods)), ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", value)
        with tempfile.TemporaryDirectory(prefix="rmg-coast-oracle-") as raw:
            path = Path(raw) / "oracle.cpp"; path.write_text(program)
            executable = Path(raw) / "oracle"
            result = subprocess.run(["g++", "-std=c++98", "-O1", "-fno-elide-constructors", str(path), "-o", str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stderr[-5000:])
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
