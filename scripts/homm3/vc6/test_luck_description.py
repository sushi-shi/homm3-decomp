"""Check the recovered retail Clover Field accumulator and Halfling interaction."""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class LuckDescriptionTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_actual_body_and_wrong_accumulator_control(self):
        root = Path(__file__).resolve().parents[3]
        enums = []
        for filename, name in (("armygrp.h", "TCreatureType"),
                               ("armygrp.h", "EMagicTerrain"),
                               ("town.h", "TTownType"),
                               ("artifact_type.h", "TArtifact")):
            header = (root / "include" / filename).read_text()
            start = header.index("enum " + name + " {")
            enums.append(header[start:header.index("\n};", start) + 3])
        source = (root / "src/armygrp.cpp").read_text()
        start = source.index("std::string armyGroup::getLuckDescription(")
        body = source[start:source.index("\n}", start) + 2]
        wrong = body.replace("luck -= 2;", "currentLuck -= 2;")
        self.assertNotEqual(wrong, body)
        wrong = wrong.replace("armyGroup::getLuckDescription(",
                              "armyGroup::wrongAccumulator(")
        program = (root / "scripts/experiments/luck-description-oracle.cpp").read_text()
        program = program.replace("// @ENUMS@", "\n".join(enums))
        program = program.replace("// @CANDIDATES@", body + "\n" + wrong)
        with tempfile.TemporaryDirectory(prefix="luck-description-") as directory:
            cpp = Path(directory) / "oracle.cpp"
            cpp.write_text(program)
            executable = Path(directory) / "oracle"
            result = subprocess.run(["g++", "-std=c++98", "-O1", str(cpp),
                                     "-o", str(executable)], capture_output=True,
                                    text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr[-6000:])
            result = subprocess.run([str(executable)], capture_output=True,
                                    text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
