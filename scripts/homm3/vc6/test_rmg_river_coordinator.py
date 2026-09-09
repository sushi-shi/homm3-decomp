"""Source admission and independently scripted river coordinator checks."""
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


class RmgRiverCoordinatorTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-river-coordinator-family.py")
        self.source = (self.root / self.module.SOURCE).read_text()

    def test_60_distinct_states_roundtrip(self):
        payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=self.module.make_axes(self.source))
        with tempfile.TemporaryDirectory(prefix="rmg-river-coordinator-") as raw:
            path = Path(raw) / "input.json"
            path.write_text(json.dumps(payload))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual(len(axes[0].options), 60)
        self.assertEqual(source_families.render(originals, axes, (0,)), originals)
        states = [source_families.render(originals, axes, (i,))[self.module.SOURCE] for i in range(60)]
        self.assertEqual(len(set(states)), 60)
        for state in states:
            self.assertEqual(len(self.module.make_axes(state)[0]["options"]), 60)
        with self.assertRaisesRegex(ValueError, "review the current"):
            self.module.make_axes(self.source.replace("position.m_x -= 2;", "position.m_x -= 1;"))

    @unittest.skipUnless(shutil.which("g++"), "native river checks require g++")
    def test_states_against_independent_route_trace(self):
        header = (self.root / "include/rmg.h").read_text()
        support = (self.root / "src/rmg_support.cpp").read_text()
        mapcell = (self.root / "include/mapcell.h").read_text()
        object_header = (self.root / "include/advmgr_objects.h").read_text()
        program = (self.root / "scripts/experiments/rmg-river-coordinator-oracle.cpp").read_text()

        def block(marker):
            start = header.index(marker)
            return header[start:header.index("\n};", start) + 3]

        types = [block("struct " + name + " {") for name in ("TRmgVector", "TPoint", "TRmgMapPosition")]
        start = object_header.index("    struct TPoint {")
        prototype_point = object_header[start:object_header.index("\n    };", start) + 7]
        helper_bodies = [self.module.helpers().definition(support, "TRmgMapPosition::TRmgMapPosition"),
            self.module.helpers().definition(self.source, "TRmgMapPosition::operator-=")]
        wheel = re.search(r"\bWATER_WHEEL\s*=\s*(\d+)", mapcell)
        self.assertIsNotNone(wheel)
        program = program.replace("WATER_WHEEL = 109", "WATER_WHEEL = " + wheel.group(1))
        methods, checks = [], []

        def candidate(label, body, positive):
            methods.append("struct " + label + " : RiverRoot { void createRivers(); };\n" + body.replace("type_random_map_generator::", label + "::"))
            checks.append("if (" + ("!" if positive else "") + "check<" + label + ">()) { std::fprintf(stderr, \"failed " + label + "\\n\"); return 1; }")

        for i, (_, body) in enumerate(self.module.forms()):
            candidate("Candidate" + str(i), body, True)
        controls = (
            ("MissingPreparation", "    markRiverObjectTargets();", ""),
            ("PreparationOrder", "    markRiverObjectTargets();\n    markRiverTargets();", "    markRiverTargets();\n    markRiverObjectTargets();"),
            ("Kind", "== WATER_WHEEL", "!= WATER_WHEEL"),
            ("Offset", "position.m_x -= prototype->m_triggerCell.m_x;", "position.m_x -= prototype->m_triggerCell.m_y;"),
            ("Displacement", "position.m_x -= 2;", "position.m_x -= 1;"),
            ("Level", "            createRiverToObject(position);", "            position.m_z = 0;\n            createRiverToObject(position);"),
            ("Progress", "m_progress->advance(1000);", "m_progress->advance(1);"),
            ("CachedPosition", "            position.m_x -= 2;", "            position = object->m_position;\n            position.m_x -= 2;"),
            ("CachedProgress", "            createRiver(position);\n            if (m_progress)\n                m_progress->advance(1000);",
             "            Progress* progress = m_progress;\n            createRiver(position);\n            if (progress)\n                progress->advance(1000);"),
            ("CachedCount", "    for (unsigned int index = 0; index < m_positions.size(); ++index)",
             "    unsigned int count = m_positions.size();\n    for (unsigned int index = 0; index < count; ++index)"),
        )
        for label, before, after in controls:
            self.assertIn(before, self.module.BASELINE)
            candidate("Wrong" + label, self.module.BASELINE.replace(before, after), False)
        for marker, replacement in (("VALUE_TYPES", "\n".join(types)), ("VALUE_HELPERS", "\n".join(helper_bodies)),
                ("PROTOTYPE_POINT", prototype_point),
                ("CANDIDATES", "\n".join(methods)), ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", replacement)
        self.assertNotIn("// @", program)
        with tempfile.TemporaryDirectory(prefix="rmg-river-coordinator-oracle-") as raw:
            path = Path(raw) / "oracle.cpp"
            path.write_text(program)
            executable = Path(raw) / "oracle"
            result = subprocess.run(["g++", "-std=c++98", "-O1", "-fno-elide-constructors", str(path), "-o", str(executable)],
                capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stderr[-10000:])
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
