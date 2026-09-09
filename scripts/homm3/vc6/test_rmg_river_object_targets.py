"""River-object target families preserve clipping and the packed presence bit."""
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


class RmgRiverObjectTargetTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-river-object-target-family.py")
        self.source = (self.root / self.module.SOURCE).read_text()

    def test_60_distinct_states_rebase(self):
        payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=self.module.make_axes(self.source))
        with tempfile.TemporaryDirectory(prefix="rmg-river-object-target-") as raw:
            path = Path(raw) / "input.json"; path.write_text(json.dumps(payload))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual(len(axes[0].options), 60)
        self.assertEqual(source_families.render(originals, axes, (0,)), originals)
        states = [source_families.render(originals, axes, (i,))[self.module.SOURCE] for i in range(60)]
        self.assertEqual(len(set(states)), 60)
        for source in states:
            self.assertEqual(len(self.module.make_axes(source)[0]["options"]), 60)
        with self.assertRaisesRegex(ValueError, "review the current"):
            self.module.make_axes(self.source.replace("prototype->m_subtype == GEMS", "prototype->m_subtype != GEMS"))

    @unittest.skipUnless(shutil.which("g++"), "native bitfield checks require g++")
    def test_all_states_preserve_clipping_and_other_bits(self):
        header = (self.root / "include/rmg.h").read_text()
        support = (self.root / "src/rmg_support.cpp").read_text()
        object_header = (self.root / "include/advmgr_objects.h").read_text()
        constants_source = (self.root / "include/mapcell.h").read_text() + (self.root / "include/town.h").read_text()
        program = (self.root / "scripts/experiments/rmg-river-object-target-oracle.cpp").read_text()

        def block(name):
            start = header.index("struct " + name + " {")
            return header[start:header.index("\n};", start) + 3]

        constants = []
        for name in ("TERRAIN_MOUNTAIN", "TERRAIN_LAKE", "MINE", "GEMS"):
            match = re.search(r"\b" + name + r"\s*=\s*(\d+)", constants_source)
            self.assertIsNotNone(match)
            constants.append(name + " = " + match.group(1))
        start = object_header.index("    struct TPoint {")
        prototype_point = object_header[start:object_header.index("\n    };", start) + 7]
        scalar = re.search(r"inline TRmgMapItem\* getMapItem\(int x, int y, int z\)\s*\{[^}]+}", header)
        self.assertIsNotNone(scalar)
        methods, checks = [], []

        def candidate(label, body, positive):
            methods.append("struct " + label + " : TargetRoot { void markRiverObjectTargets(); };\n" + body.replace("type_random_map_generator::", label + "::"))
            checks.append("if (" + ("!" if positive else "") + "check<" + label + ">()) { std::fprintf(stderr, \"failed " + label + "\\n\"); return 1; }")

        for i, (_, body) in enumerate(self.module.forms()):
            candidate("Candidate" + str(i), body, True)
        controls = (
            ("Kind", "prototype->m_objectType == TERRAIN_MOUNTAIN", "prototype->m_objectType == 1"),
            ("Subtype", "prototype->m_subtype == GEMS", "prototype->m_subtype != GEMS"),
            ("Trigger", "if (prototype->m_hasTrigger)", "if (!prototype->m_hasTrigger)"),
            ("HalfSign", "static_cast<unsigned int>(prototype->m_imageInfo.m_objectSize.m_x)", "prototype->m_imageInfo.m_objectSize.m_x"),
            ("HalfRound", "m_objectSize.m_x) / 2", "m_objectSize.m_x) / 3"),
            ("Dimensions", "offsetY = static_cast<unsigned int>(prototype->m_imageInfo.m_objectSize.m_y)", "offsetY = static_cast<unsigned int>(prototype->m_imageInfo.m_objectSize.m_x)"),
            ("TriggerY", "offsetY = prototype->m_triggerCell.m_y;", "offsetY = prototype->m_triggerCell.m_x;"),
            ("XBoundary", "position.m_x < m_map.m_mapWidth", "position.m_x <= m_map.m_mapWidth"),
            ("YBoundary", "position.m_y < m_map.m_mapHeight", "position.m_y <= m_map.m_mapHeight"),
            ("Level", "position.m_x, position.m_y, position.m_z", "position.m_x, position.m_y, 0"),
            ("Bit", "m_tileData.m_hasRiver = 1", "m_tileData.m_riverTarget = 1"),
            ("Progress", "m_progress->advance(1000);", "m_progress->advance(1);"),
        )
        for label, before, after in controls:
            self.assertIn(before, self.module.BASELINE)
            candidate("Wrong" + label, self.module.BASELINE.replace(before, after), False)
        for marker, replacement in (("CONSTANTS", "enum { " + ", ".join(constants) + " };"),
                ("VALUE_TYPES", "\n".join(block(name) for name in ("TRmgVector", "TPoint", "TRmgMapPosition"))),
                ("TILE_DATA", block("TRmgGroundTileData")), ("PROTOTYPE_POINT", prototype_point),
                ("SCALAR_ACCESSOR", scalar.group()),
                ("POSITION_ACCESSOR", self.module.helpers().definition(self.source, "type_random_map::getMapItem", parameters="TRmgMapPosition point")),
                ("VALUE_HELPERS", self.module.helpers().definition(support, "TRmgMapPosition::TRmgMapPosition")),
                ("CANDIDATES", "\n".join(methods)), ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", replacement)
        self.assertNotIn("// @", program)
        with tempfile.TemporaryDirectory(prefix="rmg-river-object-target-oracle-") as raw:
            path = Path(raw) / "oracle.cpp"; path.write_text(program)
            executable = Path(raw) / "oracle"
            result = subprocess.run(["g++", "-std=c++98", "-O1", str(path), "-o", str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stderr[-10000:])
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
