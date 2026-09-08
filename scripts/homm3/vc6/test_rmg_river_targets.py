"""River target scan families preserve dynamic coast calls and packed edges."""
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


class RmgRiverTargetTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-river-target-family.py")
        self.source = (self.root / self.module.SOURCE).read_text()

    def test_60_distinct_states_roundtrip(self):
        payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=self.module.make_axes(self.source))
        with tempfile.TemporaryDirectory(prefix="rmg-river-target-") as raw:
            path = Path(raw) / "input.json"; path.write_text(json.dumps(payload))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual(len(axes[0].options), 60)
        self.assertEqual(source_families.render(originals, axes, (0,)), originals)
        states = [source_families.render(originals, axes, (i,))[self.module.SOURCE] for i in range(60)]
        self.assertEqual(len(set(states)), 60)
        for source in states:
            self.assertEqual(len(self.module.make_axes(source)[0]["options"]), 60)
        with self.assertRaisesRegex(ValueError, "review the current"):
            self.module.make_axes(self.source.replace("direction += 2)", "direction += 1)"))

    @unittest.skipUnless(shutil.which("g++"), "native scan oracle requires g++")
    def test_all_states_against_scan_machine_and_edge_predicate(self):
        header = (self.root / "include/rmg.h").read_text()
        support = (self.root / "src/rmg_support.cpp").read_text()
        terrain = (self.root / "include/terrain_type.h").read_text()
        program = (self.root / "scripts/experiments/rmg-river-target-oracle.cpp").read_text()

        def block(name):
            start = header.index("struct " + name + " {")
            return header[start:header.index("\n};", start) + 3]

        water = re.search(r"\beTerrainWater\s*=\s*(\d+)", terrain)
        scalar = re.search(r"inline TRmgMapItem\* getMapItem\(int x, int y, int z\)\s*\{[^}]+}", header)
        self.assertIsNotNone(water); self.assertIsNotNone(scalar)
        methods, checks = [], []

        def candidate(label, body, positive):
            # VC6's legacy for-scope permits reusing z after the first loop.
            # Host C++98 needs an explicit function-scope declaration; no
            # expression, lifetime-bearing object or game implementation changes.
            if "\n    for (z = 0;" in body:
                body = body.replace("    for (int z = 0;", "    int z;\n    for (z = 0;", 1)
            methods.append("struct " + label + " : TargetRoot { void markRiverTargets(); };\n" + body.replace("type_random_map_generator::", label + "::"))
            checks.append("if (" + ("!" if positive else "") + "check<" + label + ">()) { std::fprintf(stderr, \"failed " + label + "\\n\"); return 1; }")

        for i, (_, body) in enumerate(self.module.forms()):
            candidate("Candidate" + str(i), body, True)
        controls = (
            ("Water", "== eTerrainWater", "!= eTerrainWater"),
            ("Directions", "direction += 2", "direction += 1"),
            ("Coordinates", "TRmgMapPosition(x, y, z)", "TRmgMapPosition(y, x, z)"),
            ("Level", "TRmgMapPosition(x, y, z)", "TRmgMapPosition(x, y, 0)"),
            ("Cursor", "++x, ++item", "++x"),
            ("LeftEdge", "            m_map.getMapItem(0, y, z)->m_tileData.m_riverTarget = 1;", ""),
            ("RightEdge", "            m_map.getMapItem(m_map.m_mapWidth - 1, y, z)->m_tileData.m_riverTarget = 1;", ""),
            ("TopEdge", "            m_map.getMapItem(x, 0, z)->m_tileData.m_riverTarget = 1;", ""),
            ("BottomEdge", "            m_map.getMapItem(x, m_map.m_mapHeight - 1, z)->m_tileData.m_riverTarget = 1;", ""),
            ("PresenceBit", "m_tileData.m_riverTarget = 1", "m_tileData.m_hasRiver = 1"),
            ("Progress", "advance(1000)", "advance(1)"),
        )
        for label, before, after in controls:
            self.assertIn(before, self.module.BASELINE)
            candidate("Wrong" + label, self.module.BASELINE.replace(before, after), False)
        for marker, replacement in (("CONSTANTS", "enum { eTerrainWater = " + water.group(1) + " };"),
                ("VALUE_TYPES", "\n".join(block(name) for name in ("TRmgVector", "TPoint", "TRmgMapPosition"))),
                ("VALUE_HELPERS", self.module.helpers().definition(support, "TRmgMapPosition::TRmgMapPosition")),
                ("TILE_TYPES", "\n".join(block(name) for name in ("TRmgGroundTile", "TRmgGroundTileData"))),
                ("ACCESSOR", scalar.group()), ("CANDIDATES", "\n".join(methods)), ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", replacement)
        self.assertNotIn("// @", program)
        with tempfile.TemporaryDirectory(prefix="rmg-river-target-oracle-") as raw:
            path = Path(raw) / "oracle.cpp"; path.write_text(program)
            executable = Path(raw) / "oracle"
            result = subprocess.run(["g++", "-std=c++98", "-O1", "-fno-elide-constructors", str(path), "-o", str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stderr[-10000:])
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
