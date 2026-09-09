"""Integer circumcenter families retain division order and triangle flags."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


class VoronoiVerticesTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-voronoi-vertices-family.py")
        self.source = (self.root / self.module.SOURCE).read_text()

    def test_manifest(self):
        axes = self.module.make_axes(self.source)
        self.assertEqual(len(axes[0]["options"]), 58)
        with tempfile.TemporaryDirectory() as raw:
            path = Path(raw) / "family.json"
            path.write_text(json.dumps(dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes)))
            _, originals, parsed = source_families.load_manifest(path, self.root)
        self.assertEqual(source_families.render(originals, parsed, (0,)), originals)
        self.assertEqual(len({option["replace"] for option in axes[0]["options"]}), 58)

    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_integer_reference(self):
        header = (self.root / "include/rmg.h").read_text()
        def block(name):
            start = header.index("struct " + name + " {")
            return header[start:header.index("\n};", start) + 3]
        types = block("TRmgVector") + "\n" + block("TPoint")
        return_axes = generator("generate-rmg-voronoi-return-family.py").operator_axes(self.source)
        self.assertEqual([len(axis["options"]) for axis in return_axes], [3] * 5)
        methods, checks = [], []
        def candidate(name, body, good):
            methods.append("struct " + name + " : Root { void buildVertices(); };\n" + body.replace("TRmgVoronoi::", name + "::"))
            checks.append("if (" + ("!" if good else "") + "check<" + name + ">()) { std::fprintf(stderr, \"failed " + name + "\\n\"); return 1; }")
        for index, option in enumerate(self.module.make_axes(self.source)[0]["options"]):
            candidate("Candidate" + str(index), option["replace"], True)
        baseline = self.module.helpers().definition(self.source, self.module.FUNCTION)
        for name, before, after in (
            ("Division", ") / 2;", ") / 3;"),
            ("Flag", "edge->m_positionComputed = 1;", "edge->m_positionComputed = 0;"),
            ("Zone", "edge->m_zone &&", "!edge->m_zone &&"),
            ("Numerator", "secondSide.m_y * thirdSide.m_y", "secondSide.m_y * thirdSide.m_x")):
            self.assertIn(before, baseline)
            candidate("Wrong" + name, baseline.replace(before, after), False)
        program = (self.root / "scripts/experiments/rmg-voronoi-vertices-oracle.cpp").read_text()
        for marker, value in (("TYPES", types), ("CANDIDATES", "\n".join(methods)), ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", value)
        with tempfile.TemporaryDirectory(prefix="rmg-voronoi-vertices-") as raw:
            source = Path(raw) / "oracle.cpp"
            exe = Path(raw) / "oracle"
            for form in range(3):
                helpers = "\n".join(axis["options"][form]["replace"] for axis in return_axes)
                source.write_text(program.replace("// @HELPERS@", helpers))
                result = subprocess.run(["g++", "-std=c++98", "-O1", "-fno-elide-constructors", str(source), "-o", str(exe)], capture_output=True, text=True, timeout=120)
                self.assertEqual(result.returncode, 0, result.stderr[-5000:])
                result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=120)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
