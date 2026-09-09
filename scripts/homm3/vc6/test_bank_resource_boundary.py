"""Native oracle for the actual bank appraisal and resource-cost overloads.

The fixture tests bounded values, early exits, player selection, reward order
and the recovered call boundaries. Counters are added only to fixture copies
of the two overload entries, never to production or VC6 search candidates.
This is not an ABI/layout, floating-point edge-case or inlining oracle.
"""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class BankResourceBoundaryTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_actual_bodies_and_negative_controls(self):
        root = Path(__file__).resolve().parents[3]
        source = (root / "src/philai.cpp").read_text()

        def body(signature):
            start = source.index(signature + "\n{")
            return source[start:source.index("\n}\n", start) + 3]

        pointer = body("int aiResourceCost(const playerData* player, const int* resources)")
        wrapper = body("int aiResourceCost(long playerId, const int* resources)")
        bank = body("inline long valueOfBank(const hero* currentHero, NewmapCell* cell)")
        pointer = pointer.replace("\n{", "\n{\n    ++resourceCalls;", 1)
        wrapper = wrapper.replace("\n{", "\n{\n    ++wrapperCalls;", 1)
        actual = pointer + wrapper + bank
        variants = [
            ("Actual", actual, True),
            ("WrongEmptyFlag", actual.replace("0x2000000", "0x4000000"), False),
            ("WrongCombatBoundary", actual.replace("value <= -500000000", "value < -500000000"), False),
            ("WrongResourcePlayer", actual.replace("m_players[playerId]", "m_players[0]"), False),
            ("MissingResourceAccumulation", actual.replace(
                "value += aiResourceCost(currentHero->m_owner", "value = aiResourceCost(currentHero->m_owner"), False),
            ("WrongRewardGuard", actual.replace("m_rewardCreatures > 0", "m_rewardCreatures < 0"), False),
            ("WrongArtifactCount", actual.replace("bank.m_artifacts.size()", "(bank.m_artifacts.size() + 1)"), False),
            ("BypassedOwnerWrapper", actual.replace(
                "aiResourceCost(currentHero->m_owner, bank.m_resources)",
                "aiResourceCost(&g_game->m_players[currentHero->m_owner], bank.m_resources)"), False),
        ]
        fixture = r"""
enum { NUM_RESOURCES = 7 };
struct type_AI_player { double m_resourceValue[7]; float m_turnValueOfAvgArtifact; };
struct playerData { type_AI_player m_ai; };
struct game { playerData m_players[4]; };
game gameStorage;
game* g_game = &gameStorage;
playerData* g_currentPlayer;
struct hero { signed char m_owner; };
struct armyGroup {};
struct type_creature_bank {
    armyGroup m_guards;
    int m_resources[7];
    int m_rewardCreature;
    signed char m_rewardCreatures;
    std::vector<int> m_artifacts;
} bankStorage;
struct creatureTraits { int m_aiValue; } g_creatureTypeTraits[3];
int lookupCalls, combatCalls, resourceCalls, wrapperCalls;
bool badArguments;
long combatResult;
const hero* expectedHero;
struct NewmapCell { unsigned m_extraInfo; };
// A real derived fixture object makes the body-only void* round trip valid;
// no fixture declaration is asserted to reproduce the retail union layout.
struct ExtraInfoUnion : NewmapCell {
    type_creature_bank& getCreatureBank() { ++lookupCalls; return bankStorage; }
};
NewmapCell* expectedCell;
long aiValueOfCombat(const hero* attacker, const hero* defender,
                    const armyGroup& guards, const void* town, NewmapCell* cell) {
    ++combatCalls;
    if (lookupCalls != 1 || resourceCalls || wrapperCalls || attacker != expectedHero
        || defender || &guards != &bankStorage.m_guards || town || cell != expectedCell)
        badArguments = true;
    return combatResult;
}
// @BODY@
bool check() {
    const long combats[] = {-500000001, -500000000, -499999999, -100, 0, 37};
    const int rewards[] = {-2, 0, 1, 7};
    for (int owner = 0; owner != 4; ++owner)
    for (int active = 0; active != 4; ++active)
    for (int empty = 0; empty != 2; ++empty)
    for (unsigned combat = 0; combat != 6; ++combat)
    for (unsigned reward = 0; reward != 4; ++reward)
    for (unsigned artifacts = 0; artifacts != 5; ++artifacts)
    for (int creature = 0; creature != 3; ++creature) {
        for (int player = 0; player != 4; ++player) {
            gameStorage.m_players[player].m_ai.m_turnValueOfAvgArtifact = 0.5f + 2.0f * player;
            for (int resource = 0; resource != 7; ++resource)
                gameStorage.m_players[player].m_ai.m_resourceValue[resource] =
                    0.5 * (player + 1) * (resource + 1);
        }
        for (int resource = 0; resource != 7; ++resource)
            bankStorage.m_resources[resource] = resource - 2;
        for (int kind = 0; kind != 3; ++kind)
            g_creatureTypeTraits[kind].m_aiValue = 17 + kind * 13;
        bankStorage.m_rewardCreature = creature;
        bankStorage.m_rewardCreatures = rewards[reward];
        bankStorage.m_artifacts.assign(artifacts, 0);
        const type_creature_bank original = bankStorage;
        hero attacker;
        attacker.m_owner = owner;
        ExtraInfoUnion cell;
        cell.m_extraInfo = empty ? 0x2000000 : 0;
        expectedHero = &attacker;
        expectedCell = &cell;
        g_currentPlayer = &gameStorage.m_players[active];
        lookupCalls = combatCalls = resourceCalls = wrapperCalls = 0;
        badArguments = false;
        combatResult = combats[combat];
        long expected = 0;
        const bool appraise = !empty && combatResult > -500000000;
        if (!empty) {
            expected = combatResult;
            if (appraise) {
                int resourceValue = 0;
                for (int resource = 0; resource != 7; ++resource)
                    resourceValue = int(resourceValue + (resource - 2)
                        * (0.5 * (owner + 1) * (resource + 1)));
                expected += resourceValue;
                if (rewards[reward] > 0)
                    expected += rewards[reward] * (17 + creature * 13);
                const float rewardValue = artifacts * (0.5f + 2.0f * active);
                expected = long(rewardValue + float(expected));
            }
        }
        const long result = valueOfBank(&attacker, &cell);
        if (result != expected || badArguments || lookupCalls != 1
            || combatCalls != !empty || resourceCalls != appraise || wrapperCalls != appraise
            || cell.m_extraInfo != (empty ? 0x2000000u : 0u) || attacker.m_owner != owner
            || bankStorage.m_artifacts != original.m_artifacts
            || bankStorage.m_rewardCreatures != original.m_rewardCreatures
            || bankStorage.m_rewardCreature != original.m_rewardCreature)
            return false;
        for (int resource = 0; resource != 7; ++resource)
            if (bankStorage.m_resources[resource] != original.m_resources[resource])
                return false;
    }
    return true;
}
"""
        programs, checks = [], []
        for name, candidate, expected in variants:
            if not expected:
                self.assertNotEqual(candidate, actual, name)
            programs.append("namespace " + name + " {\n"
                            + fixture.replace("// @BODY@", candidate) + "\n}")
            checks.append("if (" + name + "::check() != " + str(expected).lower()
                          + ') { std::fputs("' + name + '\\n", stderr); return 1; }')
        program = ("#include <cstdio>\n#include <vector>\n" + "\n".join(programs)
                   + "\nint main() {\n" + "\n".join(checks) + "\n}\n")
        with tempfile.TemporaryDirectory(prefix="bank-resource-boundary-") as directory:
            source_path = Path(directory) / "oracle.cpp"
            source_path.write_text(program)
            executable = Path(directory) / "oracle"
            for optimization in ("-O0", "-O2"):
                result = subprocess.run(["g++", "-std=c++98", optimization,
                                         str(source_path), "-o", str(executable)],
                                        capture_output=True, text=True, timeout=180)
                self.assertEqual(result.returncode, 0, result.stderr[-6000:])
                result = subprocess.run([str(executable)], capture_output=True,
                                        text=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
