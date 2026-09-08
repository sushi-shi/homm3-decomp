"""Finite semantic controls for the recovered Complete-only terrain selector."""
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


class RmgZoneTerrainTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-zone-terrain-family.py")
        self.source = (self.root / self.module.SOURCE).read_text()

    def test_manifest_rebases_all_60_states_without_accepting_semantic_drift(self):
        payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                       axes=self.module.make_axes(self.source))
        with tempfile.TemporaryDirectory(prefix="rmg-zone-terrain-manifest-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual(len(axes[0].options), 60)
        self.assertEqual(source_families.render(originals, axes, (0,)), originals)
        texts = [source_families.render(originals, axes, (i,))[self.module.SOURCE] for i in range(60)]
        self.assertEqual(len(set(texts)), 60)
        for text in texts:
            rebased = self.module.make_axes(text)[0]
            self.assertEqual(len(rebased["options"]), 60)
            self.assertEqual(rebased["find"], rebased["options"][0]["replace"])
        with self.assertRaisesRegex(ValueError, "review the current"):
            self.module.make_axes(self.source.replace("int selected = rand() % count;", "int selected = rand();"))

    @unittest.skipUnless(shutil.which("g++"), "zone terrain oracle needs g++")
    def test_all_60_source_states_against_explicit_eligible_list_and_negative_controls(self):
        header = (self.root / "include/rmg.h").read_text()
        template = (self.root / "scripts/experiments/rmg-zone-terrain-oracle.cpp").read_text()

        def structure(name):
            start = header.index("struct " + name + " {")
            return header[start:header.index("\n};", start) + 3]

        def fields(name, names):
            # Project only the tested fields, keeping their actual declarations
            # and value types. This fixture makes no x86 layout claim.
            source = structure(name)
            lines = []
            for field in names:
                matches = [line.split("//")[0].strip() for line in source.splitlines()
                           if re.search(r"\b" + field + r"(?:\[\d+\])?;", line.split("//")[0])]
                self.assertEqual(len(matches), 1, (name, field))
                lines.append(matches[0])
            return "\n".join(lines)

        original = self.module.helpers().definition(self.source, self.module.FUNCTION)
        start = self.source.index("static const int g_rmgTownNativeTerrains[9] = {")
        native = self.source[start:self.source.index("\n};", start) + 3]
        programs, checks = [], []

        def candidate(label, text):
            programs.append("struct " + label + " : TerrainRoot { void chooseTerrain(); };\n"
                            + text.replace("TRmgZone::", label + "::"))

        for index, option in enumerate(self.module.make_axes(self.source)[0]["options"]):
            label = "Candidate" + str(index)
            candidate(label, option["replace"])
            checks.append('if (!check<' + label + '>()) { std::fprintf(stderr, "failed state ' + str(index) + '\\n"); return 1; }')
        for label, before, after in (
                ("WrongLevel", "m_levelPosition.m_z == 1", "m_levelPosition.m_z != 0"),
                ("WrongRank", "selected-- <= 0", "selected-- < 0"),
                ("WrongNative", "m_useNativeTerrain && m_alignment != -1", "m_useNativeTerrain && m_alignment > 0"),
                ("WrongDefault", "m_terrain = eTerrainDirt;", "m_terrain = eTerrainLava;")):
            self.assertIn(before, original)
            candidate(label, original.replace(before, after))
            checks.append("if (check<" + label + ">()) return 2;")
        program = template
        for marker, replacement in (("POSITION_TYPE", structure("TRmgMapPosition")),
                ("SLOT_FIELDS", fields("TRmgTownSlot", ("m_useNativeTerrain", "m_allowedTerrain"))),
                ("ZONE_FIELDS", fields("TRmgZone", ("m_slot", "m_alignment", "m_terrain", "m_levelPosition"))),
                ("NATIVE_TABLE", native), ("CANDIDATES", "\n".join(programs)), ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", replacement)
        self.assertNotIn("// @", program)
        with tempfile.TemporaryDirectory(prefix="rmg-zone-terrain-oracle-") as raw:
            path = Path(raw) / "oracle.cpp"
            path.write_text(program)
            executable = Path(raw) / "oracle"
            compiled = subprocess.run(["g++", "-std=c++98", "-O0", "-fno-elide-constructors", "-I", str(self.root / "include"),
                                       str(path), "-o", str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(compiled.returncode, 0, compiled.stderr[-10000:])
            checked = subprocess.run([str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)


if __name__ == "__main__":
    unittest.main()
