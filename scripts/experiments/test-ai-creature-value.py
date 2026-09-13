"""Check the authored AI creature record against numeric retail expectations.

Retail initializeCreatures loads unit.value at unit+0x3c for the per-hit
ratio. Changing stack population must change total value, not that ratio.
This reduced fixture exercises the actual record-writing statements; it does
not model VC6 layout, sort, or the hero bonus calculation outside that block.
"""
from pathlib import Path
import argparse
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (args.source or root / 'src/ai_combat.cpp').read_text()
function = source.index('void type_AI_combat_data::initializeCreatures(')
start = source.index('        unit.m_index = i;', function)
end = source.index('        m_totalCombatValue += unit.m_totalValue;', start)
block = source[start:end]

fixture = r'''
#include <cmath>
#include <cstdio>
typedef int TCreatureType;
enum { CREATURE_ARCH_MAGE = 35, const_ranged = 0 };
const unsigned g_ctaNoMeleePenalty = 0x1000;
const unsigned g_ctaDoubleRangedValue = 0x8000;
struct Traits {
    long m_speed, m_hitPoints, m_baseFightValue;
    unsigned m_attributes;
};
struct Army { long m_numTroops[7]; };
struct Monster {
    long m_index, m_type, m_number, m_originalNumber, m_speed;
    long m_value, m_totalValue, m_catagory;
    double m_meleeModifier, m_finalMeleeModifier, m_rangedModifier;
    double m_combatValuePerHit;
};
struct Projection {
    Army* m_currentArmy;
    bool m_wallArcheryPenalty;
    int category;
    long getCatagory(TCreatureType, long) const { return category; }
    Monster project(const Traits& traits, int i, TCreatureType creature,
                    long speedBonus, double hitPoints, double forceModifier,
                    double archeryModifier) {
        Monster unit;
        long speed = traits.m_speed + speedBonus;
        unsigned attributes = traits.m_attributes;
@BLOCK@
        return unit;
    }
};
static bool near(double a, double b) {
    return std::fabs(a-b) < 1e-10;
}
int main() {
    // Integral square roots make these independent expected values exact:
    // (base HP, boosted HP, force, expected value for base fight value 40).
    struct Case { long baseHp; double hp, force; long value; } cases[] = {
        {16, 16, 1.0, 40}, {16, 64, 1.0, 80},
        {16, 144, 0.5, 60}, {25, 100, 1.5, 120}
    };
    const long counts[] = {1, 2, 7, 1000};
    unsigned tested = 0;
    for (const auto& c : cases)
      for (long count : counts)
       for (int category : {0, 3})
        for (unsigned bits = 0; bits != 4; ++bits)
         for (bool wall : {false, true})
          for (int creature : {1, int(CREATURE_ARCH_MAGE)})
           for (int slot = 0; slot != 7; ++slot) {
            Army army = {};
            army.m_numTroops[slot] = count;
            Traits traits = {5, c.baseHp, 40,
                (bits & 1 ? g_ctaNoMeleePenalty : 0)
                | (bits & 2 ? g_ctaDoubleRangedValue : 0)};
            Projection p = {&army, wall, category};
            Monster m = p.project(traits, slot, creature, 3, c.hp, c.force, 0.4);
            const double ranged[4] = {0.4, 0.4, 0.8, 0.8};
            double expectedRanged = category == 0 ? ranged[bits] : 0.0;
            if (wall && creature != CREATURE_ARCH_MAGE) expectedRanged /= 2;
            double expectedMelee = category == 0 && !(bits & 1) ? 0.1 : 0.2;
            double expectedFinal = category == 0 && !(bits & 1) ? 0.7 : 1.0;
            if (m.m_index != slot || m.m_type != creature || m.m_number != count
                || m.m_originalNumber != count || m.m_speed != 8
                || m.m_value != c.value || m.m_totalValue != c.value * count
                || m.m_catagory != category
                || !near(m.m_combatValuePerHit, double(c.value)/c.hp)
                || !near(m.m_meleeModifier, expectedMelee)
                || !near(m.m_finalMeleeModifier, expectedFinal)
                || !near(m.m_rangedModifier, expectedRanged)) return 1;
            ++tested;
           }
    std::printf("%u creature-record cases\n", tested);
}
'''.replace('#include <cstdio>', '#include <cstdio>\n#include <initializer_list>')

variants = [
    ('actual', block, 0),
    ('stack-total-numerator', block.replace(
        'static_cast<double>(unit.m_value)',
        'static_cast<double>(unit.m_totalValue)'), 1),
    ('base-hitpoint-denominator', block.replace(
        '/ hitPoints;', '/ traits.m_hitPoints;'), 1),
    ('missing-population-factor', block.replace(
        'unit.m_value * unit.m_number', 'unit.m_value'), 1),
]
with tempfile.TemporaryDirectory(prefix='ai-creature-value-') as directory:
    directory = Path(directory)
    for name, body, expected in variants:
        if expected and body == block:
            raise AssertionError(f'{name}: negative-control anchor absent')
        cpp = directory / (name + '.cpp')
        binary = directory / name
        cpp.write_text(fixture.replace('@BLOCK@', body))
        subprocess.run(['c++', '-std=c++17', '-O2', str(cpp), '-o', str(binary)],
                       check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        if result.returncode != expected:
            raise AssertionError(f'{name}: got {result.returncode}, expected {expected}')
        print(name + ': ' + (result.stdout.strip() or 'rejected as expected'))
