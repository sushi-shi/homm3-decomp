"""Bounded native oracle for the actual bank loader and canonical reader.

Mock the resource interface, not the two production bodies. Check all 44
levels, row/cell order, signed-byte narrowing and failure-path disposal.
This is not a VC6 codegen/layout oracle; the COFF table verifier covers the
two recovered owners and all 66 retail dwords separately.
"""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class BankLevelBoundaryTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_actual_loader_reader_and_negative_controls(self):
        root = Path(__file__).resolve().parents[3]
        source = (root / "src/creature_bank.cpp").read_text()

        def body(signature):
            start = source.index(signature)
            return source[start:source.index("\n}\n", start) + 3]

        helper = body("static void initializeCreatureBankLevel(")
        helper = helper.replace("\n{", "\n{\n    ++readerCalls;", 1)
        loader = body("unsigned char initializeCreatureBankTraits()")
        actual = helper + loader
        variants = [
            ("Actual", actual, True),
            ("WrongInitialRow", actual.replace("int row = 2;", "int row = 3;"), False),
            ("WrongLevelCount", actual.replace("int levelsLeft = 4;", "int levelsLeft = 3;"), False),
            ("WrongChanceColumn", actual.replace("int column = 2;", "int column = 1;"), False),
            ("WrongGuardCountColumn", actual.replace("column += 2;", "column += 1;", 1), False),
            ("MissingGuardClear", actual.replace("traits.m_guards.m_armyTypes[guard] = CREATURE_NONE;",
                                                   "traits.m_guards.m_armyTypes[guard] = CREATURE_CYCLOPS;"), False),
            ("WrongRewardZeroWidth", actual.replace("traits.m_rewardCreatures == 0",
                                                      "atoi(resource[20]) == 0"), False),
            ("MissingFailureDispose", actual.replace("        sheet->dispose();\n", "", 1), False),
        ]
        fixture = r"""
enum TCreatureType {
    CREATURE_NONE = -1, CREATURE_CYCLOPS = 94, CREATURE_DWARF = 16,
    CREATURE_GRIFFIN = 4, CREATURE_IMP = 42, CREATURE_MEDUSA = 76,
    CREATURE_NAGA_SENTINEL = 38, CREATURE_DRAGON_FLY = 105,
    CREATURE_WIGHT = 60, CREATURE_WATER_ELEMENTAL = 115,
    CREATURE_SKELETON = 56, CREATURE_WALKING_DEAD = 58,
    CREATURE_VAMPIRE = 62, CREATURE_GREEN_DRAGON = 26,
    CREATURE_RED_DRAGON = 82, CREATURE_GOLD_DRAGON = 27,
    CREATURE_BLACK_DRAGON = 83, CREATURE_ANGEL = 12, CREATURE_WYVERN = 108
};
enum { CREATURE_BANK_COUNT = 11 };
struct armyGroup {
    TCreatureType m_armyTypes[7]; int m_numTroops[7];
    void initialize() {
        for (int i = 0; i != 7; ++i) {
            m_armyTypes[i] = CREATURE_NONE; m_numTroops[i] = 0;
        }
    }
};
struct type_creature_bank_level {
    armyGroup m_guards; int m_resources[7]; TCreatureType m_rewardCreature;
    signed char m_rewardCreatures, m_chance, m_upgradeChance;
    signed char m_treasureArtifacts, m_minorArtifacts, m_majorArtifacts, m_relicArtifacts;
};
struct type_creature_bank_traits {
    std::string m_name; type_creature_bank_level m_levels[4];
} g_creatureBankTraits[11];
int readerCalls, disposeCalls, rowCount, acquireCalls;
bool missing, badName;
std::vector<int> rowsRead;
std::vector<std::string> cellsParsed;
int atoi(const char* value) {
    cellsParsed.push_back(value); return std::atoi(value);
}
struct TSpreadsheetResource {
    char storage[48][26][24];
    std::vector<char*> rows[48];
    int getNumberOfRows() { return rowCount; }
    const std::vector<char*>& getRow(int row) {
        rowsRead.push_back(row); return rows[row];
    }
    void dispose() { ++disposeCalls; }
} sheetStorage;
struct ResourceManager {
    static TSpreadsheetResource* getSpreadsheet(const char* name) {
        ++acquireCalls; badName = std::strcmp(name, "crbanks.txt") != 0;
        return missing ? 0 : &sheetStorage;
    }
};
// @BODY@
int sample(int pass, int row, int column) {
    const int edge[] = {0, 1, -1, 127, 128, 255, 256, 257, -128, -129, -256};
    return edge[(pass + 3 * row + column) % 11];
}
bool check() {
    const int guards[11][5] = {
        {94,-1,0,0,0}, {16,-1,0,0,0}, {4,-1,0,0,0}, {42,-1,0,0,0},
        {76,-1,0,0,0}, {38,-1,0,0,0}, {105,-1,0,0,0}, {60,-1,0,0,0},
        {115,-1,0,0,0}, {56,58,60,62,-1}, {26,82,27,83,-1}
    };
    const int rewards[] = {-1,-1,12,-1,-1,-1,108,-1,-1,-1,-1};
    const int counts[] = {3,6,8,10};
    const int parseColumns[] = {2,3,5,6,8,10,13,14,15,16,17,18,19,20,22,23,24,25};
    for (int pass = 0; pass != 11; ++pass)
    for (int condition = 0; condition != 4; ++condition) {
        for (int row = 0; row != 48; ++row) {
            sheetStorage.rows[row].clear();
            for (int column = 0; column != 26; ++column) {
                std::sprintf(sheetStorage.storage[row][column], "%d", sample(pass,row,column));
                sheetStorage.rows[row].push_back(sheetStorage.storage[row][column]);
            }
        }
        for (int bank = 0; bank != 11; ++bank) {
            g_creatureBankTraits[bank].m_name = "unloaded";
            for (int level = 0; level != 4; ++level) {
                type_creature_bank_level& out = g_creatureBankTraits[bank].m_levels[level];
                out.m_guards.initialize(); out.m_rewardCreature = CREATURE_ANGEL;
                out.m_rewardCreatures = out.m_chance = out.m_upgradeChance = -17;
                out.m_treasureArtifacts = out.m_minorArtifacts = out.m_majorArtifacts = out.m_relicArtifacts = -17;
                for (int resource = 0; resource != 7; ++resource) out.m_resources[resource] = -17;
            }
        }
        missing = condition == 0; rowCount = condition == 1 ? 12 : condition == 2 ? 13 : 46;
        readerCalls = disposeCalls = acquireCalls = 0; badName = false;
        rowsRead.clear(); cellsParsed.clear();
        const bool success = condition >= 2;
        if (initializeCreatureBankTraits() != success || acquireCalls != 1 || badName
            || disposeCalls != !missing || readerCalls != (success ? 44 : 0)) return false;
        if (!success) {
            if (!rowsRead.empty() || !cellsParsed.empty()) return false;
            for (int bank = 0; bank != 11; ++bank)
                if (g_creatureBankTraits[bank].m_name != "unloaded") return false;
            continue;
        }
        std::vector<int> expectedRows;
        std::vector<std::string> expectedCells;
        for (int bank = 0; bank != 11; ++bank) {
            expectedRows.push_back(2 + 4 * bank);
            if (g_creatureBankTraits[bank].m_name != sheetStorage.storage[2 + 4 * bank][0]) return false;
            for (int level = 0; level != 4; ++level) {
                const int row = 2 + 4 * bank + level;
                expectedRows.push_back(row);
                for (unsigned column = 0; column != sizeof(parseColumns)/sizeof(*parseColumns); ++column)
                    expectedCells.push_back(sheetStorage.storage[row][parseColumns[column]]);
                const type_creature_bank_level& out = g_creatureBankTraits[bank].m_levels[level];
                for (int slot = 0; slot != 7; ++slot) {
                    const int count = slot < 4 ? sample(pass,row,counts[slot]) : 0;
                    int type = slot < 4 ? guards[bank][slot] : -1;
                    if (bank < 9 && slot > 0) type = -1;
                    if (slot > 0 && slot < 4 && count == 0) type = -1;
                    if (out.m_guards.m_numTroops[slot] != count || out.m_guards.m_armyTypes[slot] != type) return false;
                }
                for (int resource = 0; resource != 7; ++resource)
                    if (out.m_resources[resource] != sample(pass,row,13+resource)) return false;
                const signed char rewardCount = sample(pass,row,20);
                if (out.m_rewardCreatures != rewardCount || out.m_rewardCreature != (rewardCount ? rewards[bank] : -1)
                    || out.m_chance != (signed char)sample(pass,row,2)
                    || out.m_upgradeChance != (signed char)sample(pass,row,5)
                    || out.m_treasureArtifacts != (signed char)sample(pass,row,22)
                    || out.m_minorArtifacts != (signed char)sample(pass,row,23)
                    || out.m_majorArtifacts != (signed char)sample(pass,row,24)
                    || out.m_relicArtifacts != (signed char)sample(pass,row,25)) return false;
            }
        }
        if (rowsRead != expectedRows || cellsParsed != expectedCells) return false;
    }
    return true;
}
"""
        programs, checks = [], []
        for name, candidate, expected in variants:
            if not expected:
                self.assertNotEqual(candidate, actual, name)
            programs.append("namespace " + name + " {\n" + fixture.replace("// @BODY@", candidate) + "\n}")
            checks.append("if (" + name + "::check() != " + str(expected).lower()
                          + ') { std::fputs("' + name + '\\n", stderr); return 1; }')
        program = ("#include <cstdio>\n#include <cstdlib>\n#include <cstring>\n#include <string>\n#include <vector>\n"
                   "#define DATA(address)\n#define DATA_COMPGEN(address,name,value) value\n"
                   + "\n".join(programs) + "\nint main() {\n" + "\n".join(checks) + "\n}\n")
        with tempfile.TemporaryDirectory(prefix="bank-level-boundary-") as directory:
            source_path = Path(directory) / "oracle.cpp"
            source_path.write_text(program)
            executable = Path(directory) / "oracle"
            for optimization in ("-O0", "-O2"):
                result = subprocess.run(["g++", "-std=c++98", optimization,
                                         str(source_path), "-o", str(executable)],
                                        capture_output=True, text=True, timeout=180)
                self.assertEqual(result.returncode, 0, result.stderr[-6000:])
                result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
