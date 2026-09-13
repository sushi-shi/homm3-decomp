"""Semantic and manifest controls for the 60-state treasure-outline search."""
import json
import itertools
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


class RmgTreasureOutlineTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-treasure-outline-family.py")
        self.source = (self.root / self.module.SOURCE).read_text()

    def test_manifest_keeps_60_real_states_and_fails_closed_on_changed_scan(self):
        payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                       axes=self.module.make_axes(self.source))
        with tempfile.TemporaryDirectory(prefix="rmg-treasure-outline-manifest-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual(len(axes), 1)
        self.assertEqual(len(axes[0].options), 60)
        self.assertEqual(source_families.render(originals, axes, (0,)), originals)
        texts = [source_families.render(originals, axes, (i,))[self.module.SOURCE] for i in range(60)]
        self.assertEqual(len(set(texts)), 60)
        for text in texts:
            rebased = self.module.make_axes(text)[0]
            self.assertEqual(len(rebased["options"]), 60)
            self.assertEqual(rebased["find"], rebased["options"][0]["replace"])
        with self.assertRaisesRegex(ValueError, "review the current"):
            self.module.make_axes(self.source.replace("int height = m_map.m_mapHeight;",
                                                     "int height = m_map.m_mapHeight + 1;"))

    def test_parent_and_start_axes_are_disjoint_and_preserve_their_baseline(self):
        options = self.module.make_axes(self.source)[0]["options"]
        original = options[0]["replace"]
        parents = [(option["name"], self.source.replace(original, option["replace"])) for option in options[:10]]
        axes = self.module.make_parent_axes(self.source, parents)
        self.assertEqual([len(axis["options"]) for axis in axes], [10, 6])
        payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes)
        with tempfile.TemporaryDirectory(prefix="rmg-treasure-parent-manifest-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            _, originals, parsed = source_families.load_manifest(path, self.root)
        self.assertEqual(source_families.render(originals, parsed, (0, 0)), originals)
        texts = [source_families.render(originals, parsed, choice)[self.module.SOURCE]
                 for choice in itertools.product(range(10), range(6))]
        self.assertEqual(len(set(texts)), 60)

    @unittest.skipUnless(shutil.which("g++"), "treasure-outline oracle needs g++")
    def test_all_source_states_against_independent_perimeters_and_negative_controls(self):
        helper = self.module.helpers()
        header = (self.root / "include/rmg.h").read_text()
        template = (self.root / "scripts/experiments/rmg-treasure-outline-oracle.cpp").read_text()
        types = []
        for name in ("TRmgVector", "TPoint", "TRmgGroundTile", "TRmgGroundTileData"):
            start = header.index("struct " + name + " {")
            types.append(header[start:header.index("\n};", start) + 3])
        start = header.index("enum ERmgDirectionLimits {")
        types.append(header[start:header.index("\n};", start) + 3])
        predicates = []
        for name in ("isRoadEntrance", "hasSubterraneanGate"):
            start = header.index("    unsigned char " + name + "() const")
            predicates.append(header[start:header.index("\n    }", start) + 6])
        start = header.index("    inline TRmgMapItem* getMapItem(int x, int y, int z)")
        lookup = header[start:header.index("\n    }", start) + 6]
        lookup = lookup.replace("    {\n", "    {\n        record(x, y, z);\n", 1)
        start = self.source.index("TPoint g_rmgDirections[")
        directions = self.source[start:self.source.index("\n};", start) + 3]
        start = self.source.index("TPoint operator+(TPoint point, TRmgVector offset)\n")
        addition = self.source[start:self.source.index("\n}", start) + 2]
        original = helper.definition(self.source, self.module.FUNCTION)

        def candidate(label, body):
            return ("struct " + label + " : OutlineRoot { void traceOutline(); };\n"
                    + body.replace("TRmgTreasureGroup::", label + "::"))

        programs = [candidate("Baseline", original)]
        checks = []
        options = self.module.make_axes(self.source)[0]["options"]
        # Compile the complete 60 x 6 semantic space. A subsequent VC6 search
        # uses only the ten reproduced scan parents x six start lifetimes.
        combinations = itertools.product(options, self.module.walk_starts())
        for index, (option, (_, start)) in enumerate(combinations):
            label = "Candidate" + str(index)
            self.assertEqual(option["replace"].count(self.module.WALK_START), 1)
            programs.append(candidate(label, option["replace"].replace(self.module.WALK_START, start)))
            checks.append('if (!check<' + label + '>()) { std::fprintf(stderr, "failed state ' + str(index) + '\\n"); return 1; }')
        for label, before, after in (
                ("WrongEmpty", "position.m_x == m_map.m_mapWidth", "position.m_y == m_map.m_mapHeight"),
                ("WrongStart", "--position.m_y;", "++position.m_y;"),
                ("WrongPredicate", "item->isRoadEntrance() ||", "false ||"),
                ("WrongLevel", "position.m_x, position.m_y, 0", "position.m_x, position.m_y, 1")):
            self.assertEqual(original.count(before), 1)
            programs.append(candidate(label, original.replace(before, after)))
            checks.append("if (check<" + label + ">()) return 2;")
        program = template
        for marker, replacement in (("VALUE_TYPES", "\n".join(types)), ("PREDICATES", "\n".join(predicates)),
                ("SCALAR_LOOKUP", lookup), ("VALUE_HELPERS", addition), ("DIRECTIONS", directions),
                ("CANDIDATES", "\n".join(programs)), ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", replacement)
        self.assertNotIn("// @", program)
        with tempfile.TemporaryDirectory(prefix="rmg-treasure-outline-oracle-") as raw:
            path = Path(raw) / "oracle.cpp"
            path.write_text(program)
            executable = Path(raw) / "oracle"
            compiled = subprocess.run(["g++", "-std=c++98", "-O0", "-fno-elide-constructors", str(path),
                                       "-o", str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(compiled.returncode, 0, compiled.stderr[-10000:])
            checked = subprocess.run([str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)


if __name__ == "__main__":
    unittest.main()
