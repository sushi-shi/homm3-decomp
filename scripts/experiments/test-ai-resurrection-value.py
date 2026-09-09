"""Bounded semantic check of actual AI resurrection valuation.

Only mastery, spell-work probability and hero bonus are mocked. The actual
value helper must short-circuit, truncate/convert, divide, cap to missing
creatures and return the corresponding combat value without mutating a stack.
Numbers deliberately fit VC6's signed 32-bit long on the native host too.
"""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class ResurrectionValueTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_actual_body_and_negative_controls(self):
        source = (Path(__file__).resolve().parents[2] / "src/ai_combat.cpp").read_text()
        start = source.index("long type_monster_data::getResurrectionValue(")
        actual = source[start:source.index("\n}\n", start) + 3]
        variants = [
            ("Actual", actual, True),
            ("MissingOriginalGuard", actual.replace(
                "    if (m_originalNumber <= m_number)\n        return 0;\n", ""), False),
            ("WrongChanceGate", actual.replace("== 0.0)", "< 1.0)"), False),
            ("MissingHeroBonus", actual.replace("value += const_cast<hero*>", "value = const_cast<hero*>"), False),
            ("WrongRemainingCount", actual.replace("m_originalNumber - m_number", "m_originalNumber"), False),
            ("WrongCap", actual.replace("min(", "max("), False),
        ]
        fixture = r"""
struct hero {
    int bonus;
    int getHeroSpellBonus(int spell, int level, long value);
};
struct SpellTraits { int m_powerFactor; };
struct CreatureTraits { int m_level; };
SpellTraits g_spellTraits[3];
CreatureTraits g_creatureTypeTraits[2];
std::vector<int> events;
float workChance;
bool argumentsOk;
const hero* expectedHero;
long expectedRaw;
int min(int a, int b) { return a < b ? a : b; }
int max(int a, int b) { return a < b ? b : a; }
int hero::getHeroSpellBonus(int spell, int level, long value) {
    events.push_back(2);
    if (this != expectedHero || spell != 2 || level != 4 || value != expectedRaw) argumentsOk = false;
    return bonus;
}
float getSpellWorkChance(int spell, int creature, const hero* caster, const hero* target) {
    events.push_back(0);
    if (spell != 2 || creature != 1 || caster != expectedHero || target != expectedHero) argumentsOk = false;
    return workChance;
}
struct type_spell_choice {
    int m_spell, m_power, mastery;
    long getMasteryValue() const { events.push_back(1); return mastery; }
};
struct type_monster_data {
    long m_originalNumber, m_number, m_value;
    int m_type;
    double m_combatValuePerHit;
    long getResurrectionValue(type_spell_choice&, const hero*) const;
};
// @BODY@
bool check() {
    const int counts[] = {0,1,3,10}, values[] = {1,7,100};
    const double modifiers[] = {0.25,1.0,1.5,2.0};
    g_spellTraits[2].m_powerFactor = 3;
    g_creatureTypeTraits[1].m_level = 4;
    hero caster = {11};
    for (int original = 0; original != 4; ++original)
    for (int current = 0; current != 4; ++current)
    for (int cost = 0; cost != 3; ++cost)
    for (int modifier = 0; modifier != 4; ++modifier)
    for (int present = 0; present != 2; ++present)
    for (int chance = 0; chance != 3; ++chance)
    for (int power = 0; power != 3; ++power) {
        type_monster_data monster = {counts[original], counts[current], values[cost], 1, modifiers[modifier]};
        type_spell_choice choice = {2, power * 10, power};
        expectedHero = present ? &caster : 0;
        expectedRaw = choice.mastery + 3 * choice.m_power;
        workChance = chance * 0.5f;
        std::vector<int> expectedEvents;
        long expected = 0;
        if (monster.m_originalNumber > monster.m_number) {
            expectedEvents.push_back(0);
            if (chance) {
                expectedEvents.push_back(1);
                if (present) expectedEvents.push_back(2);
                long count = static_cast<long>((expectedRaw + (present ? 11 : 0)) * modifiers[modifier]) / values[cost];
                long missing = counts[original] - counts[current];
                expected = (count < missing ? count : missing) * values[cost];
            }
        }
        events.clear(); argumentsOk = true;
        long result = monster.getResurrectionValue(choice, expectedHero);
        if (result != expected || events != expectedEvents || !argumentsOk
            || monster.m_originalNumber != counts[original] || monster.m_number != counts[current]
            || monster.m_value != values[cost] || monster.m_combatValuePerHit != modifiers[modifier]) return false;
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
        program = "#include <cstdio>\n#include <vector>\n" + "\n".join(programs)
        program += "\nint main() {\n" + "\n".join(checks) + "\n}\n"
        with tempfile.TemporaryDirectory(prefix="ai-resurrection-value-") as directory:
            path = Path(directory) / "oracle.cpp"
            path.write_text(program)
            executable = Path(directory) / "oracle"
            for optimization in ("-O0", "-O2"):
                result = subprocess.run(["g++", "-std=c++98", optimization, str(path), "-o", str(executable)],
                                        capture_output=True, text=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stderr[-6000:])
                result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
