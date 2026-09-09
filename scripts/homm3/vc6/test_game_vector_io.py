"""Semantic controls for the actual typed saved-game vector templates."""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class GameVectorIOTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_actual_helpers_stream_contract_and_negative_controls(self):
        root = Path(__file__).resolve().parents[3]
        game = (root / "src/game.cpp").read_text()
        start = game.index("template <class T>\nbool loadVector(")
        end = game.index("\n}\n", game.index("template <class T>\nbool saveVector(", start)) + 3
        helpers = game[start:end]
        self.assertNotIn("#pragma", helpers)
        self.assertNotIn("__forceinline", helpers)
        self.assertIn("destVector.resize(count);", helpers)
        header = (root / "include/game.h").read_text()
        start = header.index("struct type_university {")
        university = header[start:header.index("\n};", start) + 3]
        town = (root / "src/townmgr.cpp").read_text()
        start = town.index("type_university* type_university::initializeMagicSkills()\n{")
        initializer = town[start:town.index("\n}", start) + 2]
        self.assertNotIn("type_university();", university)
        randomize = game[game.index("void game::randomizeUniversity("):]
        randomize = randomize[:randomize.index("\n}\n")]
        self.assertIn("type_university university;", randomize)
        self.assertNotIn("universitySkillsRecord", randomize)
        university_variants = [
            ("ActualUniversity", university, initializer, True),
            ("BadSchool", university, initializer.replace(
                "m_skills[3] = eSecSkillSchoolOfEarthMagic;",
                "m_skills[3] = eSecSkillSchoolOfFireMagic;"), False),
            ("BadReturn", university, initializer.replace("return this;", "return 0;"), False),
            ("BadDefault", university.replace(
                "TSecondarySkill m_skills[4];",
                "TSecondarySkill m_skills[4];\n"
                "    type_university() { initializeMagicSkills(); }"), initializer, False),
        ]
        university_candidates, university_checks = [], []
        for name, record, body, valid in university_variants:
            if not valid:
                self.assertNotEqual((record, body), (university, initializer), name)
            university_candidates.append("namespace " + name + " {\n" + record + "\n" + body + "\n}")
            university_checks.append("if (checkUniversity<" + name + "::type_university>() != "
                                     + str(valid).lower() + ") return 2;")
        variants = [
            ("Adopted", helpers, True),
            ("BadHeaderWidth", helpers.replace("sizeof(count)", "sizeof(int)"), False),
            ("BadPayloadWidth", helpers.replace("count * sizeof(T)", "count * (sizeof(T) / 2)"), False),
            ("BadResize", helpers.replace("destVector.resize(count);", "destVector.resize(count + 1);"), False),
            ("BadReadFailure", helpers.replace("return false;", "return true;", 1), False),
            ("BadSaveCount", helpers.replace("static_cast<short>(count)", "count"), False),
            ("BadWriteFailure", helpers.replace(
                "if (outfile->write(&count, sizeof(short)) < sizeof(short))\n        return false;",
                "if (outfile->write(&count, sizeof(short)) < sizeof(short))\n        return true;"), False),
        ]
        candidates, checks = [], []
        for name, body, valid in variants:
            if not valid:
                self.assertNotEqual(body, helpers, name)
            candidates.append("namespace " + name + " {\n" + body + """
struct Ops {
    template<class T> static bool load(TAbstractFile* file, std::vector<T>& data) {
        return loadVector(file, data);
    }
    template<class T> static bool save(TAbstractFile* file, std::vector<T>& data) {
        return saveVector(file, data);
    }
};
}
""")
            checks.append("if (check<" + name + "::Ops>() != " + str(valid).lower() + ") return 1;")
        program = (root / "scripts/experiments/game-vector-io-oracle.cpp").read_text()
        for marker, value in (("UNIVERSITY", university), ("UNIVERSITY_INITIALIZER", initializer),
                              ("UNIVERSITY_CANDIDATES", "\n".join(university_candidates)),
                              ("UNIVERSITY_CHECKS", "\n".join(university_checks)),
                              ("CANDIDATES", "\n".join(candidates)), ("CHECKS", "\n".join(checks))):
            program = program.replace("// @" + marker + "@", value)
        with tempfile.TemporaryDirectory(prefix="game-vector-io-") as directory:
            source = Path(directory) / "oracle.cpp"
            source.write_text(program)
            executable = Path(directory) / "oracle"
            result = subprocess.run(["g++", "-std=c++98", "-O1", "-mms-bitfields",
                                     "-fno-elide-constructors", "-I", str(root / "include"),
                                     str(source), "-o", str(executable)],
                                    capture_output=True, text=True, timeout=180)
            self.assertEqual(result.returncode, 0, result.stderr[-6000:])
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
