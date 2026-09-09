"""Independent semantic controls for the 60-state group-commit population."""
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


class RmgGroupCommitTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-group-commit-family.py")
        self.source = (self.root / self.module.SOURCE).read_text()

    def test_60_distinct_states_rebase_without_accepting_changed_map_rules(self):
        payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                       axes=self.module.make_axes(self.source))
        with tempfile.TemporaryDirectory(prefix="rmg-group-commit-manifest-") as raw:
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
            self.module.make_axes(self.source.replace("source->m_tileData.m_subterraneanGate = gate;",
                                                     "source->m_tileData.m_subterraneanGate = border;"))

    @unittest.skipUnless(shutil.which("g++"), "group-commit oracle needs g++")
    def test_all_states_against_map_and_call_oracle_with_negative_controls(self):
        header = (self.root / "include/rmg.h").read_text()
        support = (self.root / "src/rmg_support.cpp").read_text()
        template = (self.root / "scripts/experiments/rmg-group-commit-oracle.cpp").read_text()

        def block(start):
            at = header.index(start)
            return header[at:header.index("\n};", at) + 3]

        def field(text, name):
            matches = [line.split("//")[0].strip() for line in text.splitlines()
                       if re.search(r"\b" + name + r";", line.split("//")[0])]
            self.assertEqual(len(matches), 1, name)
            return matches[0]

        types = [block("struct " + name + " {") for name in ("TRmgVector", "TPoint", "TRmgMapPosition",
                 "TRmgZoneBounds", "TRmgGroundTile", "TRmgGroundTileData", "TRmgConnectionDecoration")]
        predicates = []
        for name in ("hasSubterraneanGate", "hasBorderObject", "isRoadEntrance"):
            at = header.index("    unsigned char " + name + "() const")
            predicates.append(header[at:header.index("\n    }", at) + 6])
        at = header.index("    inline TRmgMapItem* getMapItem(int x, int y, int z)")
        lookup = header[at:header.index("\n    }", at) + 6]
        lookup = lookup.replace("    {\n", "    {\n        record(x, y, z);\n", 1)
        helper = self.module.helpers()
        value_helpers = [helper.definition(support, "TRmgMapPosition::TRmgMapPosition"),
                         helper.definition(self.source, "type_object::getPosition"),
                         helper.definition(self.source, "type_random_map::getMapItem",
                                           parameters="TRmgMapPosition point")]
        original = helper.definition(self.source, self.module.FUNCTION)
        programs, checks = [], []

        def candidate(label, body):
            programs.append("struct " + label + " : CommitRoot { void commitTreasureGroup("
                            "TRmgTreasureGroup*, TRmgMapPosition); };\n"
                            + body.replace("type_random_map_generator::", label + "::"))

        methods = dict(self.module.semantic_forms())
        methods["current"] = original
        if os.environ.get("HOMM3_GROUP_COMMIT_MANIFEST"):
            _, originals, axes = source_families.load_manifest(Path(os.environ["HOMM3_GROUP_COMMIT_MANIFEST"]), self.root)
            for index in range(60):
                rendered = source_families.render(originals, axes, (index,))[self.module.SOURCE]
                methods["manifest_" + str(index)] = helper.definition(rendered, self.module.FUNCTION)
        for index, method in enumerate(dict.fromkeys(methods.values())):
            label = "Candidate" + str(index)
            candidate(label, method)
            checks.append('if (!check<' + label + '>()) { std::fprintf(stderr, "failed state ' + str(index) + '\\n"); return 1; }')
        controls = [
            ("WrongClip", "std::_cpp_max<long>(0, -position.m_x)", "std::_cpp_max<long>(0, position.m_x)"),
            ("WrongLevel", "objectPosition.m_z = position.m_z;", "objectPosition.m_z = 0;"),
            ("WrongConnection", "if (!source->m_connection.m_present)", "if (true)"),
            ("WrongSnapshot", "source->m_tileData.m_subterraneanGate = gate;", "source->m_tileData.m_subterraneanGate = border;"),
            ("WrongWater", "destination->m_tile.m_landType != eTerrainWater", "true"),
            ("MissingVirtual", "group->m_objects[objectIndex]->isWritable();", ";"),
        ]
        for label, before, after in controls:
            self.assertIn(before, original)
            candidate(label, original.replace(before, after))
            checks.append("if (check<" + label + ">()) return 2;")
        # Replacing a live vector bound with a cached size loses objects added
        # by the virtual transfer. This is not an admissible search axis.
        before = "    for (unsigned int i = 0; i < group->m_objects.size(); ++i) {"
        after = "    unsigned count = group->m_objects.size();\n    for (unsigned int i = 0; i < count; ++i) {"
        self.assertIn(before, original)
        candidate("WrongCachedSize", original.replace(before, after))
        checks.append("if (check<WrongCachedSize>()) return 2;")
        program = template
        for marker, replacement in (("VALUE_TYPES", "\n".join(types)), ("PREDICATES", "\n".join(predicates)),
                ("OBJECT_POSITION", field(block("class type_object {"), "m_position")),
                ("GROUP_POSITION", field(block("struct TRmgTreasureGroup {"), "m_position")),
                ("SCALAR_LOOKUP", lookup), ("VALUE_HELPERS", "\n".join(value_helpers)),
                ("CANDIDATES", "\n".join(programs)), ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", replacement)
        self.assertNotIn("// @", program)
        with tempfile.TemporaryDirectory(prefix="rmg-group-commit-oracle-") as raw:
            path = Path(raw) / "oracle.cpp"
            path.write_text(program)
            executable = Path(raw) / "oracle"
            compiled = subprocess.run(["g++", "-std=c++98", "-O1", "-fno-elide-constructors", "-I", str(self.root / "include"),
                                       str(path), "-o", str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(compiled.returncode, 0, compiled.stderr[-10000:])
            checked = subprocess.run([str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(checked.returncode, 0, checked.stdout + checked.stderr)

    def test_parent_frontier_keeps_ten_and_admits_only_checked_successors(self):
        helper = self.module.helpers()
        original = helper.definition(self.source, self.module.FUNCTION)
        parents = [(label, self.source.replace(original, text))
                   for label, text in list(self.module.forms())[1:11]]
        axes = self.module.make_parent_axes(self.source, parents)
        options = axes[0]["options"]
        self.assertEqual(len(options), 60)
        self.assertEqual(len({option["replace"] for option in options}), 60)
        self.assertEqual(options[0]["replace"], original)
        for _, text in parents:
            self.assertIn(helper.definition(text, self.module.FUNCTION), [o["replace"] for o in options])
        for option in options:
            self.assertIn(option["replace"], self.module.admitted_bodies())
        self.assertEqual(len(self.module.admitted_bodies()), 420)


if __name__ == "__main__":
    unittest.main()
