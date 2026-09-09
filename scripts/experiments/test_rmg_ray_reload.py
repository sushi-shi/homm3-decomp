"""Ray reload variants against a closed-form lattice oracle."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from homm3.vc6.test_rmg_families import generator
from homm3.vc6 import source_families


class RayReloadTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[2]
        self.module = generator("generate-rmg-ray-reload-family.py")
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
    def test_lattice_oracle(self):
        header = (self.root / "include/rmg.h").read_text()
        def block(name):
            start = header.index("struct " + name + " {")
            return header[start:header.index("\n};", start) + 3]
        types = block("TRmgVector") + "\n" + block("TPoint")
        methods, checks = [], []
        def candidate(name, body, good):
            methods.append("struct " + name + " : Root { TPoint traceBranchEnd(TPoint from, TPoint toward, int level); };\n" + body.replace("type_random_map::", name + "::"))
            checks.append("if (" + ("!" if good else "") + "check<" + name + ">()) { std::fprintf(stderr, \"failed " + name + "\\n\"); return 1; }")
        options = self.module.make_axes(self.source)[0]["options"]
        for index, option in enumerate(options):
            candidate("Candidate" + str(index), option["replace"], True)
        recurrence = generator("generate-rmg-ray-recurrence-family.py").make_axes(self.source)[0]["options"]
        self.assertEqual(len(recurrence), 60)
        for index, option in enumerate(recurrence):
            candidate("Recurrence" + str(index), option["replace"], True)
        baseline = options[0]["replace"]
        for name, before, after in (
            ("EarlyGate", "steps > 2", "steps > 1"),
            ("LateGate", "steps > 2", "steps > 3"),
            ("Level", "nearby.m_z = level;", "nearby.m_z = 0;"),
            ("Boundary", "from.m_x < 1", "from.m_x < 2")):
            self.assertIn(before, baseline)
            candidate("Wrong" + name, baseline.replace(before, after), False)
        program = (self.root / "scripts/experiments/rmg-ray-reload-oracle.cpp").read_text()
        for marker, value in (("TYPES", types), ("CANDIDATES", "\n".join(methods)), ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", value)
        with tempfile.TemporaryDirectory(prefix="rmg-ray-reload-") as raw:
            source = Path(raw) / "oracle.cpp"
            source.write_text(program)
            exe = Path(raw) / "oracle"
            result = subprocess.run(["g++", "-std=c++98", "-O1", str(source), "-o", str(exe)], capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stderr[-4000:])
            result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
