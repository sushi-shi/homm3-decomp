"""Behavioral checks for recovered retail spell-immunity switch routing."""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class SpellWorkChanceTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_actual_body_and_wrong_routing_negative_controls(self):
        root = Path(__file__).resolve().parents[3]
        header = (root / "include/armygrp.h").read_text()
        helper_start = header.index("inline unsigned char isMindSpell(")
        helper = header[helper_start:header.index("\n}", helper_start) + 2]
        enums = []
        for name in ("TCreatureType", "ESpellId", "EArtifactId"):
            start = header.index("enum " + name + " {")
            enums.append(header[start:header.index("\n};", start) + 3])
        source = (root / "src/armygrp.cpp").read_text()
        start = source.index("float getSpellWorkChance(")
        body = source[start:source.index("\n}", start) + 2]
        bad_mind = body.replace("if ((attrs & 0x400)\n                    ||",
                                "if (!(attrs & 0x400)\n                    &&")
        bad_resistance = body.replace(
            "            if (targetHero)\n                chance -=",
            "            if (targetHero && targetArmyType == CREATURE_DWARF)\n"
            "                chance -=")
        bad_dragon = body.replace("case CREATURE_BLACK_DRAGON:",
                                  "case CREATURE_DIAMOND_GOLEM:")
        variants = (("Actual", body, True), ("WrongMind", bad_mind, False),
                    ("DwarfOnlyResistance", bad_resistance, False),
                    ("WrongDragon", bad_dragon, False))
        candidates, checks = [], []
        for name, implementation, valid in variants:
            if not valid:
                self.assertNotEqual(implementation, body, name)
            candidates.append("namespace " + name + " {\n" + helper + "\n"
                              + implementation + "\n}")
            checks.append("if (check(" + name + "::getSpellWorkChance) != "
                          + str(valid).lower() + ") { std::puts(\"" + name
                          + " failed\"); return 1; }")
        program = (root / "scripts/experiments/spell-work-chance-oracle.cpp").read_text()
        for marker, value in (("ENUMS", "\n".join(enums)),
                              ("CANDIDATES", "\n".join(candidates)),
                              ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", value)
        with tempfile.TemporaryDirectory(prefix="spell-work-chance-") as directory:
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
