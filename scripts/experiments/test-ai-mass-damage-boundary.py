"""Bounded semantic oracle for the actual AI mass-damage and damage bodies.

The reverse scan carries takeDamage's capped return to the next creature,
not the uncapped offered sum (DC 0x2ac58:758 and both retail 0x425bd0 arms).
Mock only spell/mastery inputs; use the authored takeDamage implementation.
Small nonnegative values avoid native-long versus VC6-long overflow issues.
This is a source-semantic check, not a VC6 layout or codegen oracle.
"""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class MassDamageTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_actual_bodies_and_negative_controls(self):
        root = Path(__file__).resolve().parents[2]
        source = (root / "src/ai_combat.cpp").read_text()

        def body(signature):
            start = source.index(signature)
            return source[start:source.index("\n}\n", start) + 3]

        damage = body("long type_monster_data::takeDamage(")
        mass = body("void type_AI_combat_data::castMassDamageSpell(")
        self.assertIn("value = m_creatures[i].takeDamage(value);", mass)
        actual = damage + "\n" + mass
        variants = [
            ("Actual", actual, True),
            ("MissingReturnedCarry", actual.replace(
                "value = m_creatures[i].takeDamage(value);\n        m_totalCombatValue -= value;",
                "m_totalCombatValue -= m_creatures[i].takeDamage(value);"), False),
            ("ResetCarry", actual.replace("value += m_creatures[i].getSpellDamage(",
                                          "value = m_creatures[i].getSpellDamage("), False),
            ("ForwardScan", actual.replace("long i = m_creatures.size(); i-- > 0;",
                                           "long i = 0; i < static_cast<long>(m_creatures.size()); ++i"), False),
            ("MissingTotalUpdate", actual.replace("m_totalCombatValue -= value;", ""), False),
            ("WrongHeroes", actual.replace("choice.m_spell, castingHero, m_currentHero, damage",
                                           "choice.m_spell, m_currentHero, castingHero, damage"), False),
            ("WrongDamageReturn", actual.replace("damage = m_totalValue;", "damage = 0;"), False),
        ]
        fixture = r"""
struct hero { int id; };
struct SpellTraits { long m_powerFactor; };
SpellTraits g_spellTraits[3];
bool argumentsOk;
std::vector<int> events;
const hero* expectedCaster;
const hero* expectedDefender;
long expectedDamage;
struct type_spell_choice {
    int m_spell;
    long m_power, mastery;
    long getMasteryValue() const { events.push_back(-1); return mastery; }
};
struct type_monster_data {
    long m_totalValue, m_number, m_value, offer;
    int id;
    long getSpellDamage(int spell, const hero* caster, const hero* defender, long damage) const {
        events.push_back(id);
        if (spell != 2 || caster != expectedCaster || defender != expectedDefender
            || damage != expectedDamage) argumentsOk = false;
        return offer;
    }
    long takeDamage(long damage);
};
class type_AI_combat_data {
public:
    const hero* m_currentHero;
    long m_totalCombatValue;
    std::vector<type_monster_data> m_creatures;
    void castMassDamageSpell(type_spell_choice&, const hero*);
};
// @BODIES@
bool check() {
    hero caster = {1}, defender = {2};
    expectedCaster = &caster; expectedDefender = &defender;
    g_spellTraits[2].m_powerFactor = 7;
    for (int count = 0; count != 8; ++count)
    for (int seed = 0; seed != 41; ++seed) {
        type_AI_combat_data army;
        army.m_currentHero = &defender;
        army.m_totalCombatValue = 0;
        type_spell_choice choice = {2, seed % 5, seed % 11};
        expectedDamage = choice.mastery + 7 * choice.m_power;
        for (int i = 0; i != count; ++i) {
            type_monster_data monster;
            monster.m_value = 1 + (seed + i) % 9;
            monster.m_totalValue = (seed * 19 + i * 7) % 37;
            monster.m_number = (monster.m_totalValue + monster.m_value - 1) / monster.m_value;
            monster.offer = (seed * 5 + i * 13) % (monster.m_totalValue + 1);
            monster.id = i;
            army.m_creatures.push_back(monster);
            army.m_totalCombatValue += monster.m_totalValue;
        }
        std::vector<type_monster_data> expected = army.m_creatures;
        long expectedTotal = army.m_totalCombatValue, carry = 0;
        std::vector<int> expectedEvents;
        expectedEvents.push_back(-1);
        for (int i = count; i-- > 0;) {
            expectedEvents.push_back(i);
            long offered = carry + expected[i].offer;
            carry = offered < expected[i].m_totalValue ? offered : expected[i].m_totalValue;
            expected[i].m_totalValue -= carry;
            expected[i].m_number = (expected[i].m_totalValue + expected[i].m_value - 1) / expected[i].m_value;
            expectedTotal -= carry;
        }
        events.clear(); argumentsOk = true;
        army.castMassDamageSpell(choice, &caster);
        if (!argumentsOk || events != expectedEvents || army.m_totalCombatValue != expectedTotal) return false;
        for (int i = 0; i != count; ++i)
            if (army.m_creatures[i].m_totalValue != expected[i].m_totalValue
                || army.m_creatures[i].m_number != expected[i].m_number) return false;
    }
    return true;
}
"""
        programs, checks = [], []
        for name, candidate, expected in variants:
            if not expected:
                self.assertNotEqual(candidate, actual, name)
            programs.append("namespace " + name + " {\n" + fixture.replace("// @BODIES@", candidate) + "\n}")
            checks.append("if (" + name + "::check() != " + str(expected).lower()
                          + ') { std::fputs("' + name + '\\n", stderr); return 1; }')
        program = "#include <cstdio>\n#include <vector>\n" + "\n".join(programs)
        program += "\nint main() {\n" + "\n".join(checks) + "\n}\n"
        with tempfile.TemporaryDirectory(prefix="ai-mass-boundary-") as directory:
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
