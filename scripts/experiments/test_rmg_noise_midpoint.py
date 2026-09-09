"""Noise midpoint forms preserve quadrant bounds, samples and append order."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from homm3.vc6.test_rmg_families import generator
from homm3.vc6 import source_families


class NoiseMidpointTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[2]
        self.module = generator("generate-rmg-noise-midpoint-family.py")
        self.source = (self.root / "src/rmg.cpp").read_text()

    def test_manifest(self):
        axes = self.module.make_axes(self.source)
        self.assertEqual(len(axes[0]["options"]), 60)
        with tempfile.TemporaryDirectory() as raw:
            path = Path(raw) / "family.json"
            path.write_text(json.dumps(dict(schema=1, units=["rmg"], axes=axes)))
            _, original, parsed = source_families.load_manifest(path, self.root)
        self.assertEqual(source_families.render(original, parsed, (0,)), original)

    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_sample_lattice(self):
        header = (self.root / "include/rmg.h").read_text()
        def block(name):
            start = header.index("struct " + name + " {")
            return header[start:header.index("\n};", start) + 3]
        types = "\n".join(block(name) for name in ("TRmgVector", "TPoint", "TRmgZoneBounds", "TRmgNoiseRegion", "TRmgNoiseMidpoints"))
        methods, checks = [], []
        def candidate(name, body, good):
            methods.append(body.replace("subdivideRmgNoiseRegion", name))
            checks.append("if (" + ("!" if good else "") + "check(" + name + ")) { std::fprintf(stderr, \"failed " + name + "\\n\"); return 1; }")
        options = self.module.make_axes(self.source)[0]["options"]
        for index, option in enumerate(options):
            candidate("candidate" + str(index), option["replace"], True)
        baseline = options[0]["replace"]
        for name, before, after in (
            ("Midpoint", ") / 2;", ") / 3;"),
            ("Sample", "part.m_corners[0] = centerValue;", "part.m_corners[0] = midpoints.m_minXValue;"),
            ("Degenerate", "&& part.m_bounds.m_minimumY", "|| part.m_bounds.m_minimumY"),
            ("Variation", "pending.push_back(part);", "part.m_variation = 12345; pending.push_back(part);")):
            self.assertIn(before, baseline)
            candidate("wrong" + name, baseline.replace(before, after), False)
        program = (self.root / "scripts/experiments/rmg-noise-midpoint-oracle.cpp").read_text()
        for marker, value in (("TYPES", types), ("CANDIDATES", "\n".join(methods)), ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", value)
        with tempfile.TemporaryDirectory(prefix="rmg-noise-midpoint-") as raw:
            source = Path(raw) / "oracle.cpp"
            source.write_text(program)
            exe = Path(raw) / "oracle"
            result = subprocess.run(["g++", "-std=c++98", "-O1", "-fno-elide-constructors", str(source), "-o", str(exe)], capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stderr[-4000:])
            result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
