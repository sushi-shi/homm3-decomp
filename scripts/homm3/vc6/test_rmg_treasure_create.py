"""Admission and selection-contract checks for retail treasure creation."""
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


class RmgTreasureCreateTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(__file__).resolve().parents[3]
        self.module = generator("generate-rmg-treasure-create-family.py")
        self.source = (self.root / self.module.SOURCE).read_text()

    def test_distinct_states_roundtrip_and_reject_changed_behavior(self):
        payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=self.module.make_axes(self.source))
        with tempfile.TemporaryDirectory(prefix="rmg-create-manifest-") as raw:
            path = Path(raw) / "input.json"
            path.write_text(json.dumps(payload))
            _, originals, axes = source_families.load_manifest(path, self.root)
        self.assertEqual(len(axes[0].options), 60)
        self.assertEqual(source_families.render(originals, axes, (0,)), originals)
        states = [source_families.render(originals, axes, (i,))[self.module.SOURCE] for i in range(60)]
        self.assertEqual(len(set(states)), 60)
        for source in states:
            self.assertEqual(len(self.module.make_axes(source)[0]["options"]), 60)
        with self.assertRaisesRegex(ValueError, "review the current"):
            self.module.make_axes(self.source.replace("objectValue /= occupied;", "objectValue /= 2;"))

    def test_frontier_retains_ten_parents(self):
        parents = list(self.module.forms())[:10]
        options = self.module.make_parent_axes(self.source, parents)[0]["options"]
        self.assertEqual(len(options), 60)
        bodies = {option["replace"] for option in options}
        self.assertEqual(len(bodies), 60)
        self.assertTrue({body for _, body in parents} <= bodies)

    @unittest.skipUnless(shutil.which("g++"), "native selection oracle requires g++")
    def test_all_states_against_independent_selection_oracle(self):
        header = (self.root / "include/rmg.h").read_text()
        object_header = (self.root / "include/advmgr_objects.h").read_text()
        support = (self.root / "src/rmg_support.cpp").read_text()
        program = (self.root / "scripts/experiments/rmg-treasure-create-oracle.cpp").read_text()

        def block(text, marker):
            start = text.index(marker)
            return text[start:text.index("\n};", start) + 3]

        types = [block(header, "struct " + name + " {") for name in ("TRmgVector", "TPoint", "TRmgMapPosition")]
        accessors = [line.strip() for line in object_header.splitlines()
            if re.match(r"\s*int get(?:Width|Height)\(\) const \{", line)]
        self.assertEqual(len(accessors), 2)
        bit_position = re.search(r"static unsigned(?: int)? getBitPos\([^}]+}", object_header)
        self.assertIsNotNone(bit_position)
        methods, checks = [], []

        def candidate(label, method, positive):
            methods.append("struct " + label + " : CreateRoot { type_object* createTreasureObject(TRmgZone*, int, int, int*, "
                "unsigned char, unsigned char, unsigned char, TRmgMapPosition); };\n"
                + method.replace("type_random_map_generator::", label + "::"))
            checks.append("if (" + ("!" if positive else "") + "check<" + label + ">()) { std::fprintf(stderr, \"failed "
                + label + "\\n\"); return 1; }")

        bodies = [body for _, body in self.module.forms()]
        bodies += [body for parent in tuple(bodies) for _, body in self.module.refinements(parent)]
        if os.environ.get("HOMM3_TREASURE_CREATE_MANIFEST"):
            _, originals, axes = source_families.load_manifest(Path(os.environ["HOMM3_TREASURE_CREATE_MANIFEST"]), self.root)
            for i in range(60):
                bodies.append(self.module.helpers().definition(source_families.render(originals, axes, (i,))[self.module.SOURCE], self.module.FUNCTION))
        for i, method in enumerate(dict.fromkeys(bodies)):
            candidate("Candidate" + str(i), method, True)
        controls = (
            ("Primary", "!primary &&", "primary &&"),
            ("Terrain", "!allowTerrainDependent &&", "allowTerrainDependent &&"),
            ("MapLimit", "m_objectCountByType[objectType] >= g_rmgMapObjectLimits", "m_objectCountByType[objectType] > g_rmgMapObjectLimits"),
            ("Minimum", "objectValue < minimum", "objectValue <= minimum"),
            ("Maximum", "objectValue > maximum", "objectValue >= maximum"),
            ("Position", "position.m_x >= 0", "position.m_y >= 0"),
            ("Footprint", "++occupied;", "occupied += 2;"),
            ("Masks", "|| prototype->m_triggerMask", "&& prototype->m_triggerMask"),
            ("Reset", "candidates.clear();", ";"),
            ("Weights", "totalWeight += definition->m_density;", "totalWeight += 1;"),
            ("Boundary", "if (selected < 0)", "if (selected <= 0)"),
            ("FreshValue", "*value = definition->getValue(zone, this);", "*value = 0;"),
        )
        for label, before, after in controls:
            self.assertIn(before, self.module.BASELINE)
            candidate("Wrong" + label, self.module.BASELINE.replace(before, after), False)
        for marker, replacement in (("VALUE_TYPES", "\n".join(types)),
                ("VALUE_HELPERS", self.module.helpers().definition(support, "TRmgMapPosition::TRmgMapPosition")),
                ("ACCESSORS", "\n".join(accessors)), ("BIT_POSITION", bit_position.group()),
                ("CANDIDATES", "\n".join(methods)), ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", replacement)
        self.assertNotIn("// @", program)
        with tempfile.TemporaryDirectory(prefix="rmg-create-oracle-") as raw:
            path = Path(raw) / "oracle.cpp"
            path.write_text(program)
            executable = Path(raw) / "oracle"
            result = subprocess.run(["g++", "-std=c++98", "-O1", str(path), "-o", str(executable)],
                capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stderr[-10000:])
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
