"""Check the actual native object-type read and its short-read commit boundary.

This is a portable behavioral oracle, not an x86 layout or VC6 verdict. It
uses the real enum, abstract-file interface and production read/commit block.
Only valid enum encodings are tested; malformed map validation is unchanged.
"""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class ObjectTypeReadOwnerTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_native_read_preserves_short_read_commit_boundary(self):
        root = Path(__file__).resolve().parents[3]
        source = (root / "src/mapcell.cpp").read_text()
        start = source.index("int NewfullMap::readObjectType(TAbstractFile* infile,")
        body = source[start:source.index("\n}\n", start)]
        start = body.index("    TAdventureObjectType objectTypeRead;")
        assignment = "    tempObjectType->m_objectType = objectTypeRead;"
        end = body.index(assignment, start) + len(assignment)
        block = body[start:end]
        header = (root / "include/mapcell.h").read_text()
        start = header.index("enum TAdventureObjectType {")
        enumeration = header[start:header.index("\n};", start) + 3]
        variants = [
            ("Actual", block, True),
            ("DirectField", block.replace("&objectTypeRead,", "&tempObjectType->m_objectType,")
             .replace(assignment, ""), False),
            ("ShortWidth", block.replace("sizeof(objectTypeRead)", "sizeof(short)"), False),
            ("RejectFullRead", block.replace(" < ", " <= "), False),
            ("WrongValue", block.replace(assignment, "    tempObjectType->m_objectType = NOTHING;"), False),
            ("MissingCommit", block.replace(assignment, ""), False),
        ]
        fixture = r"""
struct CObjectType {
    unsigned before;
    TAdventureObjectType m_objectType;
    unsigned after;
};
class ScriptFile : public TAbstractFile {
public:
    unsigned char payload[4];
    int reported, calls;
    bool validWidth;
    ScriptFile(TAdventureObjectType type, int result)
        : reported(result), calls(0), validWidth(true) {
        std::memcpy(payload, &type, sizeof(payload));
    }
    int read(void* data, int size) {
        ++calls;
        if (size != 4) {
            validWidth = false;
            return 0;
        }
        std::memcpy(data, payload, reported);
        return reported;
    }
    int write(const void*, int) { return 0; }
};
int readType(TAbstractFile* infile, CObjectType* tempObjectType) {
// @BLOCK@
    return 1;
}
bool check() {
    if (sizeof(TAdventureObjectType) != 4) return false;
    const TAdventureObjectType values[] = {
        NOTHING, HERO, MONSTER, SHIPYARD, BLACK_MARKET, ROCKLANDS
    };
    for (unsigned i = 0; i != sizeof(values) / sizeof(values[0]); ++i)
    for (int reported = 0; reported <= 4; ++reported) {
        CObjectType record = {0x12345678, TOWN, 0x76543210};
        TAdventureObjectType initial = record.m_objectType;
        ScriptFile file(values[i], reported);
        int result = readType(&file, &record);
        if (file.calls != 1 || !file.validWidth || record.before != 0x12345678
            || record.after != 0x76543210) return false;
        if (reported < 4) {
            if (result != -1 || std::memcmp(&record.m_objectType, &initial,
                                           sizeof(initial)) != 0) return false;
        } else if (result != 1 || record.m_objectType != values[i]) return false;
    }
    return true;
}
"""
        programs, checks = [], []
        for name, candidate, expected in variants:
            if not expected:
                self.assertNotEqual(candidate, block, name)
            programs.append("namespace " + name + " {\n"
                            + fixture.replace("// @BLOCK@", candidate) + "\n}")
            checks.append("if (" + name + "::check() != " + str(expected).lower()
                          + ') { std::fputs("' + name + '\\n", stderr); return 1; }')
        program = ('#include <cstdio>\n#include <cstring>\n#include "abstractfile.h"\n'
                   + enumeration + "\n" + "\n".join(programs)
                   + "\nint main() {\n" + "\n".join(checks) + "\n}\n")
        with tempfile.TemporaryDirectory(prefix="object-type-read-owner-") as directory:
            source_path = Path(directory) / "oracle.cpp"
            source_path.write_text(program)
            executable = Path(directory) / "oracle"
            for optimization in ("-O0", "-O2"):
                result = subprocess.run(["g++", "-std=c++98", optimization,
                                         "-I", str(root / "include"), str(source_path),
                                         "-o", str(executable)], capture_output=True,
                                        text=True, timeout=180)
                self.assertEqual(result.returncode, 0, result.stderr[-6000:])
                result = subprocess.run([str(executable)], capture_output=True,
                                        text=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
