"""Bounded actual-body oracle for tactical mass damage and summoning.

The four extracted helper bodies run against separate battle/choice owners.
Mocked damage, mastery and summon calls expose operands, order and early
mutation. Native tests check semantics, not VC6 ABI/layout or /Ob2 decisions.
--source permits checking a reproduced candidate before adopting it.
"""

import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


SOURCE = Path(__file__).resolve().parents[2] / "src/ai_tactical.cpp"


def function(source, signature):
    start = source.index(signature)
    return source[start:source.index("\n}\n", start) + 2]


class TacticalMassTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "requires native C++ compiler")
    def test_actual_helpers_and_negative_controls(self):
        source = SOURCE.read_text()
        bodies = "\n".join(function(source, signature) for signature in (
            "long type_AI_spellcaster::getGroupDamageValue(",
            "long type_AI_spellcaster::getMassDamageEffect(",
            "void type_AI_spellcaster::considerMassDamage(",
            "void type_AI_spellcaster::considerSummon(",
        ))
        self.assertNotIn("@stub", bodies)
        self.assertIn("type_spell_choice& choice) const", bodies)
        variants = [
            ("Actual", bodies, True),
            ("ForwardGroup", bodies.replace("[group][count]", "[group][g_combatManager->m_numArmies[group] - 1 - count]"), False),
            ("WrongEnemySide", bodies.replace("m_enemySide, m_enemyHero", "m_side, m_ourHero"), False),
            ("MissingFriendlyClamp", bodies.replace("    if (friendlyDamage < 0)\n        friendlyDamage = 0;\n", ""), False),
            ("ReversedLossComparison", bodies.replace("<= static_cast<float>(friendlyDamage)", ">= static_cast<float>(friendlyDamage)"), False),
            ("WrongMassMastery", bodies.replace("+ choice.getMasteryValue()", "+ choice.m_mastery"), False),
            ("WrongMassSign", bodies.replace("return enemyDamage - friendlyDamage;", "return enemyDamage + friendlyDamage;"), False),
            ("PrematureSummonStore", bodies.replace("    if (m_winLikely)\n", "    choice.m_castNow = 1;\n    if (m_winLikely)\n"), False),
            ("MissingSummonGuard", bodies.replace("    if (!g_combatManager->ableToSummonElemental(choice.m_spell, m_side))\n        return;\n", ""), False),
            ("WrongSummonValue", bodies.replace("g_creatureTypeTraits[summoned].m_aiValue * power", "1000 * power"), False),
        ]
        fixture = r"""
typedef int SpellID;
typedef int TCreatureType;
struct Event {
    long what, a, b, c, d;
    Event(long w, long x=0, long y=0, long z=0, long q=0) : what(w),a(x),b(y),c(z),d(q) {}
    bool operator==(const Event& e) const { return what==e.what && a==e.a && b==e.b && c==e.c && d==e.d; }
};
std::vector<Event> calls;
struct Traits { long m_powerFactor, m_masteryBonus[4]; } g_spellTraits[80];
struct CreatureTraits { long m_aiValue; } g_creatureTypeTraits[4];
struct type_spell_choice {
    SpellID m_spell; int m_mastery; long m_power, m_value, m_target;
    unsigned char m_castNow;
    long getMasteryValue() const {
        calls.push_back(Event(0,m_spell,m_mastery,m_castNow));
        return g_spellTraits[m_spell].m_masteryBonus[m_mastery];
    }
};
type_spell_choice* activeChoice;
struct hero { long id; } heroes[2];
struct army { long id, damage; };
struct Combat {
    long m_numArmies[2]; army m_armies[2][7]; bool allowed;
    bool ableToSummonElemental(SpellID spell, long side) const {
        calls.push_back(Event(2,spell,side,activeChoice->m_castNow)); return allowed;
    }
} combat;
Combat* g_combatManager = &combat;
TCreatureType getElementalType(SpellID spell) {
    calls.push_back(Event(3,spell,activeChoice->m_castNow)); return spell%4;
}
struct Estimate { long m_friendlyCombatValue, m_enemyCombatValue; bool m_killsOnly; };
class type_AI_spellcaster {
public:
    long m_side, m_enemySide; hero* m_ourHero; hero* m_enemyHero;
    Estimate m_estimate; bool m_winLikely;
    long getDamageValue(SpellID spell, long base, const hero* h, const army* target) const {
        calls.push_back(Event(1,spell,base,h->id,target->id)); return target->damage + base*h->id;
    }
    long getGroupDamageValue(SpellID spell,long damage,long group,hero* h) const;
    long getMassDamageEffect(long enemy,long friendly) const;
    void considerMassDamage(type_spell_choice& choice) const;
    void considerSummon(type_spell_choice& choice) const;
};
// @BODIES@
long effect(long enemy, long friendly, long ours, long theirs) {
    const long loss = friendly>0 ? friendly : 0;
    if (enemy<=0 || enemy<=loss || loss>=ours) return 0;
    const float enemyFraction=static_cast<float>(enemy)/static_cast<float>(theirs);
    const float ourFraction=static_cast<float>(loss)/static_cast<float>(ours);
    return enemyFraction<=ourFraction ? 0 : enemy-loss;
}
type_spell_choice initialChoice(int spell, int mastery, long power) {
    type_spell_choice c; c.m_spell=spell; c.m_mastery=mastery; c.m_power=power;
    c.m_value=-319; c.m_target=77; c.m_castNow=9; return c;
}
bool check() {
    for(int s=0;s<80;++s) {
        g_spellTraits[s].m_powerFactor=s%5+1;
        for(int m=0;m<4;++m) g_spellTraits[s].m_masteryBonus[m]=m*7+s%3;
    }
    for(int t=0;t<4;++t) g_creatureTypeTraits[t].m_aiValue=80+t*113;
    heroes[0].id=7; heroes[1].id=13;
    type_AI_spellcaster caster;
    caster.m_ourHero=&heroes[0]; caster.m_enemyHero=&heroes[1];
    caster.m_winLikely=false; caster.m_estimate.m_killsOnly=false;
    const long damages[]={-3,0,1,5,99,100,101,1000,16777215,16777216,16777217};
    const long totals[]={0,1,100,1000,16777216};
    for(unsigned e=0;e<sizeof(damages)/sizeof(*damages);++e)
    for(unsigned f=0;f<sizeof(damages)/sizeof(*damages);++f)
    for(unsigned a=0;a<sizeof(totals)/sizeof(*totals);++a)
    for(unsigned b=0;b<sizeof(totals)/sizeof(*totals);++b) {
        caster.m_estimate.m_friendlyCombatValue=totals[a];
        caster.m_estimate.m_enemyCombatValue=totals[b];
        if(caster.getMassDamageEffect(damages[e],damages[f]) != effect(damages[e],damages[f],totals[a],totals[b])) return false;
        if(caster.m_estimate.m_friendlyCombatValue!=totals[a] || caster.m_estimate.m_enemyCombatValue!=totals[b]) return false;
    }
    const int massSpells[]={14,23,26};
    for(int side=0;side<2;++side)
    for(int n=0;n<8;++n) for(int k=0;k<8;++k)
    for(int mastery=0;mastery<4;++mastery) for(int power=0;power<4;++power)
    for(int s=0;s<3;++s) {
        caster.m_side=side; caster.m_enemySide=1-side;
        caster.m_estimate.m_friendlyCombatValue=3000;
        caster.m_estimate.m_enemyCombatValue=1000;
        combat.m_numArmies[side]=n; combat.m_numArmies[1-side]=k;
        for(int group=0;group<2;++group) for(int i=0;i<7;++i) {
            combat.m_armies[group][i].id=group*100+i;
            combat.m_armies[group][i].damage=i%3==0 ? -5 : 7+group*29+i;
        }
        type_spell_choice choice=initialChoice(massSpells[s],mastery,power);
        activeChoice=&choice; calls.clear(); caster.considerMassDamage(choice);
        const long base=g_spellTraits[massSpells[s]].m_powerFactor*power+g_spellTraits[massSpells[s]].m_masteryBonus[mastery];
        std::vector<Event> expected; expected.push_back(Event(0,massSpells[s],mastery,9));
        long damage[2]={0,0};
        for(int pass=0;pass<2;++pass) {
            int group=pass==0 ? 1-side : side; const hero* h=pass==0 ? &heroes[1] : &heroes[0];
            for(int j=combat.m_numArmies[group];j>0;--j) {
                const army& target=combat.m_armies[group][j-1];
                expected.push_back(Event(1,massSpells[s],base,h->id,target.id));
                damage[pass]+=target.damage+base*h->id;
            }
        }
        if(calls!=expected || choice.m_value!=effect(damage[0],damage[1],3000,1000) || choice.m_castNow!=1 || choice.m_target!=77) return false;
        if(choice.m_spell!=massSpells[s] || choice.m_mastery!=mastery || choice.m_power!=power) return false;
        if(combat.m_numArmies[side]!=n || combat.m_numArmies[1-side]!=k) return false;
    }
    const long powers[]={0,1,3,12};
    for(int side=0;side<2;++side) for(int win=0;win<2;++win)
    for(int allow=0;allow<2;++allow) for(int kills=0;kills<2;++kills)
    for(int spell=66;spell<70;++spell) for(int mastery=0;mastery<4;++mastery)
    for(int p=0;p<4;++p) {
        caster.m_side=side; caster.m_winLikely=win!=0; combat.allowed=allow!=0;
        caster.m_estimate.m_killsOnly=kills!=0;
        type_spell_choice choice=initialChoice(spell,mastery,powers[p]); activeChoice=&choice;
        calls.clear(); caster.considerSummon(choice);
        std::vector<Event> expected; long value=-319; unsigned char cast=9;
        if(!win) {
            expected.push_back(Event(2,spell,side,9));
            if(allow) {
                expected.push_back(Event(0,spell,mastery,9));
                if(!kills) expected.push_back(Event(3,spell,9));
                value=g_spellTraits[spell].m_masteryBonus[mastery]*powers[p]*(kills ? 1000 : g_creatureTypeTraits[spell%4].m_aiValue);
                cast=1;
            }
        }
        if(calls!=expected || choice.m_value!=value || choice.m_castNow!=cast || choice.m_target!=77) return false;
        if(choice.m_spell!=spell || choice.m_mastery!=mastery || choice.m_power!=powers[p]) return false;
    }
    return true;
}
"""
        programs, checks = [], []
        for name, candidate, expected in variants:
            if not expected:
                self.assertNotEqual(candidate, bodies, name)
            programs.append("namespace " + name + " {\n" + fixture.replace("// @BODIES@", candidate) + "\n}")
            checks.append("if (" + name + "::check() != " + str(expected).lower()
                          + ') { std::fputs("' + name + '\\n", stderr); return 1; }')
        program = "#include <cstdio>\n#include <vector>\n" + "\n".join(programs)
        program += "\nint main() {\n" + "\n".join(checks) + "\n}\n"
        with tempfile.TemporaryDirectory(prefix="tactical-mass-boundaries-") as directory:
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
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=SOURCE)
    args = parser.parse_args()
    SOURCE = args.source
    unittest.main(argv=[__file__])
