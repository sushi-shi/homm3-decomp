"""Bounded actual-body oracle for horde initialization, not a VC6 ABI test.

Checks both pairs in all nine towns, matches/misses, duplicate dwelling IDs,
normal/upgraded records, bonuses and unchanged dwelling storage. Slot seven
is retained as retail behavior; final-town slot-seven matches are excluded
because their upgrade lookup would exceed the existing flat table.
"""
from pathlib import Path
import json
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / "src/town.cpp").read_text()
start = source.index("void town::initializeHordes()")
actual = source[start:source.index("\n}\n", start) + 2]
header = (root / "include/town.h").read_text()
start = header.index("struct type_horde_effect {")
record = header[start:header.index("\n};", start) + 3]
variants = [("actual", actual, True)]
if len(sys.argv) > 1:
    manifest = json.loads(Path(sys.argv[1]).read_text())
    axis = manifest["axes"][0]
    variants += [(option["name"], option.get("replace", axis["find"]), True)
                 for option in axis["options"]]
for name, before, after in (
    ("skip-last-town", "townType < TOWN_TYPE_COUNT", "townType < TOWN_TYPE_COUNT - 1"),
    ("exclusive-search", "slot <= TOWN_DWELLING_COUNT", "slot < TOWN_DWELLING_COUNT"),
    ("wrong-upgrade", "slot += TOWN_DWELLING_COUNT;", "slot += TOWN_DWELLING_COUNT - 1;"),
    ("wrong-bonus", "= *bonus;", "= *bonus + 1;"),
):
    bad = actual.replace(before, after)
    assert bad != actual, name
    variants.append((name, bad, False))

fixture = r"""
typedef int TCreatureType;
enum { TOWN_TYPE_COUNT = 9, TOWN_DWELLING_COUNT = 7 };
@RECORD@
struct town {
    static type_horde_effect s_constHordeEffects[9][4];
    static void initializeHordes();
};
type_horde_effect town::s_constHordeEffects[9][4];
TCreatureType g_townDwellingCreatures[126];
@BODY@
bool check() {
    for (int first = 0; first < 9; ++first)
    for (int second = 0; second < 9; ++second)
    for (int duplicate = 0; duplicate < 2; ++duplicate)
    for (int bonusCase = 0; bonusCase < 3; ++bonusCase) {
        type_horde_effect expected[9][4];
        int unchanged[126];
        for (int i = 0; i < 126; ++i)
            g_townDwellingCreatures[i] = 1000 + i;
        if (duplicate)
            for (int t = 0; t < 9; ++t)
                g_townDwellingCreatures[t * 14 + 5] = g_townDwellingCreatures[t * 14 + 2];
        for (int t = 0; t < 9; ++t) {
            for (int i = 0; i < 4; ++i) {
                type_horde_effect& e = town::s_constHordeEffects[t][i];
                e.m_creature = -100 - t * 4 - i;
                e.m_bonus = short((bonusCase - 1) * (t * 4 + i + 1));
                e.m_dwelling = short(-200 - t * 4 - i);
            }
            for (int pair = 0; pair < 2; ++pair) {
                int index = pair ? second : first;
                if (index < 8 && !(t == 8 && index == 7))
                    town::s_constHordeEffects[t][pair * 2].m_creature = g_townDwellingCreatures[t * 14 + index];
            }
        }
        std::memcpy(expected, town::s_constHordeEffects, sizeof expected);
        std::memcpy(unchanged, g_townDwellingCreatures, sizeof unchanged);
        for (int t = 0; t < 9; ++t)
            for (int pair = 0; pair < 2; ++pair)
                for (int slot = 0; slot < 8; ++slot)
                    if (expected[t][pair * 2].m_creature == unchanged[t * 14 + slot]) {
                        expected[t][pair * 2].m_dwelling = short(slot);
                        expected[t][pair * 2 + 1].m_creature = unchanged[t * 14 + slot + 7];
                        expected[t][pair * 2 + 1].m_dwelling = short(slot + 7);
                        expected[t][pair * 2 + 1].m_bonus = expected[t][pair * 2].m_bonus;
                        break;
                    }
        town::initializeHordes();
        for (int t = 0; t < 9; ++t)
            for (int i = 0; i < 4; ++i) {
                const type_horde_effect& a = town::s_constHordeEffects[t][i];
                const type_horde_effect& e = expected[t][i];
                if (a.m_creature != e.m_creature || a.m_dwelling != e.m_dwelling || a.m_bonus != e.m_bonus)
                    return false;
            }
        if (std::memcmp(unchanged, g_townDwellingCreatures, sizeof unchanged)) return false;
    }
    return true;
}
"""
program = "#include <cstdio>\n#include <cstring>\n"
checks = []
for index, (name, body, expected) in enumerate(variants):
    namespace = "form" + str(index)
    program += "namespace " + namespace + " {\n" + fixture.replace("@RECORD@", record).replace("@BODY@", body) + "}\n"
    checks.append(f'if ({namespace}::check() != {str(expected).lower()}) {{ std::puts("{name}"); return 1; }}')
program += "int main() {\n" + "\n".join(checks) + "\n}\n"
with tempfile.TemporaryDirectory(prefix="horde-row-oracle-") as directory:
    path = Path(directory) / "oracle.cpp"
    executable = Path(directory) / "oracle"
    path.write_text(program)
    for optimization in ("-O0", "-O2"):
        subprocess.run(["g++", "-std=c++98", optimization, str(path), "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
        print(f"{optimization}: 486 cases per valid form, {len(variants) - 4} forms; four negative controls rejected")
