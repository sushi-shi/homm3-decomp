"""Native value/trace controls for the independent spatial source frontiers."""
import itertools
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


class RmgSpatialFrontierTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-spatial-frontiers.py")
        self.source = (self.root / self.module.SOURCE).read_text()

    def states(self):
        payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                       axes=self.module.make_axes(self.source))
        with tempfile.TemporaryDirectory(prefix="rmg-spatial-manifest-") as raw:
            path = Path(raw) / "manifest.json"
            path.write_text(json.dumps(payload))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual([len(axis.options) for axis in axes], [6, 6, 5])
        self.assertEqual(source_families.render(originals, axes, (0, 0, 0)), originals)
        return [source_families.render(originals, axes, choices)[self.module.SOURCE]
                for choices in itertools.product(range(6), range(6), range(5))]

    def test_three_independent_axes_keep_baseline_and_rebase_all_180_states(self):
        states = self.states()
        self.assertEqual(len(set(states)), 180)
        for text in states:
            axes = self.module.make_axes(text)
            self.assertEqual([len(axis["options"]) for axis in axes], [6, 6, 5])
            for axis in axes:
                self.assertEqual(axis["find"], axis["options"][0]["replace"])
            bounds, island, junction = (axis["find"] for axis in axes)
            self.assertEqual(bounds.count("getLevelPosition()"), 1)
            self.assertEqual(bounds.count("std::_cpp_min<long>"), 2)
            self.assertEqual(bounds.count("std::_cpp_max<long>"), 2)
            self.assertEqual(island.count("pending.insert("), 1)
            self.assertEqual(island.count("pending.push_back("), 2)
            self.assertEqual(junction.count("m_map.getMapItem("), 2)
            self.assertEqual(island.count("rand()"), 1)
            self.assertEqual(junction.count("rand()"), 1)
        for before, after in (
                ("minimumY = 0;", "minimumY = 1;"),
                ("pending.insert(pending.end(), to);", "pending.push_back(to);"),
                ("m_map.getMapItem(column, row, position.m_z);", "m_map.getMapItem(column, row, 0);")):
            with self.assertRaisesRegex(ValueError, "review the current"):
                self.module.make_axes(self.source.replace(before, after))

    @unittest.skipUnless(shutil.which("g++"), "spatial source oracle needs g++")
    def test_all_states_preserve_aliased_outputs_cell_flags_queries_and_random_draws(self):
        helper = self.module.helpers()
        header = (self.root / "include/rmg.h").read_text()
        support = (self.root / "src/rmg_support.cpp").read_text()
        template = (self.root / "scripts/experiments/rmg-spatial-frontier-oracle.cpp").read_text()
        values = []
        for name in ("TRmgVector", "TPoint", "TRmgMapPosition", "TRmgZoneBounds"):
            start = header.index("struct " + name + " {")
            values.append(header[start:header.index("\n};", start) + 3])
        values.append("TRmgVector operator-(TPoint left, TPoint right);")
        value_helpers = [helper.definition(support, name) for name in (
            "TRmgMapPosition::TRmgMapPosition", "TRmgVector::length")]
        value_helpers += [helper.definition(self.source, name) for name in (
            "TRmgZone::getLevelPosition", "type_random_map::getMapItem",
            "TRmgVector::operator*", "TRmgVector::operator/", "operator-")]
        start = header.index("    inline TRmgMapItem* getMapItem(int x, int y, int z)")
        scalar = header[start:header.index("\n    }", start) + 6]
        scalar = scalar.replace("    {\n", "    {\n        record(x, y, z);\n", 1)
        original = [helper.definition(self.source, name) for name in self.module.FUNCTIONS]

        def candidate(label, methods):
            declarations = [text[:text.index("\n{\n")].replace(
                "type_random_map_generator::", "") + ";" for text in methods]
            return ("struct " + label + " : SpatialRoot {\n" + "\n".join(declarations) + "\n};\n"
                    + "\n".join(text.replace("type_random_map_generator::", label + "::") for text in methods))

        programs = [candidate("Baseline", original)]
        checks = []
        # Each axis replaces one complete method. Compile those exact option
        # bodies; repeated scans of the unrelated 9,000-line TU add no coverage.
        options = [axis["options"] for axis in self.module.make_axes(self.source)]
        combinations = list(itertools.product(*options))
        self.assertEqual(len(combinations), 180)
        for index, choices in enumerate(combinations):
            label = "Candidate" + str(index)
            programs.append(candidate(label, [option["replace"] for option in choices]))
            checks.append('if (!check<' + label + '>()) { std::fprintf(stderr, "failed state ' + str(index) + '\\n"); return 1; }')
        negatives = (
            ("WrongBounds", 0, "position.m_x + size + 1", "position.m_x + size + 2"),
            ("WrongEndpoint", 1, "pending.insert(pending.end(), to);", "pending.insert(pending.end(), from);"),
            ("WrongLevel", 2, "m_map.getMapItem(column, row, position.m_z);", "m_map.getMapItem(column, row, 0);"),
        )
        for label, index, before, after in negatives:
            methods = list(original)
            self.assertEqual(methods[index].count(before), 1)
            methods[index] = methods[index].replace(before, after)
            programs.append(candidate(label, methods))
            checks.append("if (check<" + label + ">()) return 2;")
        program = template.replace("// @VALUE_TYPES@", "\n".join(values)).replace(
            "// @VALUE_HELPERS@", "\n".join(value_helpers)).replace(
            "// @SCALAR_LOOKUP@", scalar).replace("// @CANDIDATES@", "\n".join(programs)).replace(
            "// @CHECKS@", "\n".join(checks))
        self.assertNotIn("// @", program)
        with tempfile.TemporaryDirectory(prefix="rmg-spatial-oracle-") as raw:
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
