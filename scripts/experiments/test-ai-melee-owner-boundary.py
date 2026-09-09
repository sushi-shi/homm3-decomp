"""Bounded native semantic oracle for the recovered AI accessor/helper.

Extract the actual getTotal and doGeneralMelee bodies. Mock final-value,
kill and damage operations to verify read/call order and damage arguments;
keep vector cardinality independent of the combat-value field. This checks
source semantics, not VC6 ABI/layout or floating-point code generation.
"""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class MeleeOwnerTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_actual_bodies_and_negative_controls(self):
        root = Path(__file__).resolve().parents[2]
        header = (root / "include/ai_combat.h").read_text()
        source = (root / "src/ai_combat.cpp").read_text()
        start = header.index("    long getTotal() const")
        accessor = header[start:header.index("}", start) + 1]
        start = source.index("void type_AI_combat_data::doGeneralMelee(")
        helper = source[start:source.index("\n}\n", start) + 3]
        self.assertIn("std::vector<type_monster_data> m_creatures;", header)
        self.assertNotIn("struct type_monster_vector :", header)
        actual = accessor + "\n// @HELPER@\n" + helper
        variants = [
            ("Actual", actual, True),
            ("WrongTotalOwner", actual.replace("return m_totalCombatValue;", "return m_creatures.size();"), False),
            ("MissingZeroGuard", actual.replace("    if (target == 0.0)\n        return;\n", ""), False),
            ("WrongTieVictim", actual.replace("if (attacker > target)", "if (attacker >= target)"), False),
            ("MissingLossBias", actual.replace(" + 0.05", ""), False),
            ("WrongDamageOperand", actual.replace("ratio * target", "ratio * attacker"), False),
        ]
        fixture = r"""
struct type_monster_data { int value; };
struct Event {
    int who, action; long value;
    Event(int w, int a, long v) : who(w), action(a), value(v) {}
    bool operator==(const Event& e) const {
        return who == e.who && action == e.action && value == e.value;
    }
};
std::vector<Event> events;
class type_AI_combat_data {
public:
    std::vector<type_monster_data> m_creatures;
    long m_totalCombatValue, finalValue;
    int who;
    type_AI_combat_data(int w, long v) : m_totalCombatValue(v), finalValue(v), who(w) {}
    long getFinalMeleeValue() const { events.push_back(Event(who, 0, finalValue)); return finalValue; }
    void kill() { events.push_back(Event(who, 1, 0)); m_totalCombatValue = 0; }
    void inflictDamage(long value, long speed) {
        events.push_back(Event(who, 2, value));
        if (speed != 0) events.push_back(Event(who, 3, speed));
    }
    void doGeneralMelee(type_AI_combat_data& defender);
// @ACCESSOR@
};
// @BODY@
bool check() {
    const long totals[] = {-37, 0, 1, 99, 1000000};
    const int counts[] = {0, 1, 7, 9};
    for (unsigned t = 0; t != sizeof(totals)/sizeof(*totals); ++t)
    for (unsigned n = 0; n != sizeof(counts)/sizeof(*counts); ++n) {
        type_AI_combat_data value(0, totals[t]);
        value.m_creatures.resize(counts[n]);
        const type_AI_combat_data& view = value;
        if (view.getTotal() != totals[t]) return false;
        type_AI_combat_data copy(value);
        copy.m_creatures.clear();
        if (copy.getTotal() != totals[t] || value.m_creatures.size() != unsigned(counts[n])) return false;
    }
    const long values[] = {0,1,2,19,20,21,99,100,101,1000000,16777215,16777216,16777217};
    for (unsigned a = 0; a != sizeof(values)/sizeof(*values); ++a)
    for (unsigned d = 0; d != sizeof(values)/sizeof(*values); ++d) {
        type_AI_combat_data attacker(0, values[a]), defender(1, values[d]);
        events.clear();
        attacker.doGeneralMelee(defender);
        std::vector<Event> expected;
        expected.push_back(Event(0, 0, values[a]));
        expected.push_back(Event(1, 0, values[d]));
        float left = static_cast<float>(values[a]);
        float right = static_cast<float>(values[d]);
        if (left != 0 && right != 0) {
            const bool weWin = left > right;
            const float winner = weWin ? left : right;
            const float loser = weWin ? right : left;
            const float ratio = loser / winner + 0.05;
            expected.push_back(Event(weWin ? 1 : 0, 1, 0));
            expected.push_back(Event(weWin ? 0 : 1, 2, static_cast<long>(ratio * loser)));
        }
        if (events != expected) return false;
    }
    return true;
}
"""
        programs, checks = [], []
        for name, candidate, expected in variants:
            if not expected:
                self.assertNotEqual(candidate, actual, name)
            candidate_accessor, candidate_helper = candidate.split("\n// @HELPER@\n")
            programs.append("namespace " + name + " {\n" + fixture.replace("// @ACCESSOR@", candidate_accessor)
                            .replace("// @BODY@", candidate_helper) + "\n}")
            checks.append("if (" + name + "::check() != " + str(expected).lower()
                          + ') { std::fputs("' + name + '\\n", stderr); return 1; }')
        program = "#include <cstdio>\n#include <vector>\n" + "\n".join(programs)
        program += "\nint main() {\n" + "\n".join(checks) + "\n}\n"
        with tempfile.TemporaryDirectory(prefix="ai-melee-boundary-") as directory:
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
