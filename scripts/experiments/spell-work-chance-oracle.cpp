// Native behavioral fixture for the actual recovered getSpellWorkChance.
// Branch expectations come from retail 0x44a4d0, including its two tables.
#include <cmath>
#include <cstdio>
#include <cstring>

// @ENUMS@
typedef int SpellID;
const unsigned g_ctaUndead = 0x40000;
const unsigned g_ctaSiegeWeapon = 0x40;
const unsigned g_ctaNoMorale = 0x20000;

struct TCreatureTypeTraits {
    unsigned m_attributes;
    int m_damageHighBound;
};
struct SSpellTraits {
    int m_karma, m_level;
    unsigned m_flags, m_school;
};
TCreatureTypeTraits g_creatureTypeTraits[150];
SSpellTraits g_spellTraits[81];

struct hero {
    bool artifacts[140];
    float resistanceFactor;
    int resistanceCalls;
    hero() : resistanceFactor(0.75f), resistanceCalls(0) {
        std::memset(artifacts, 0, sizeof(artifacts));
    }
    unsigned char isWieldingArtifact(int id) { return artifacts[id]; }
    float getMagicResistanceFactor() {
        ++resistanceCalls;
        return resistanceFactor;
    }
};

// @CANDIDATES@

typedef float (*Chance)(SpellID, TCreatureType, const hero*, const hero*);

bool near(float a, float b) { return std::fabs(a - b) < 0.00001f; }

void resetTraits() {
    std::memset(g_spellTraits, 0, sizeof(g_spellTraits));
    for (int i = 0; i < 81; ++i)
        g_spellTraits[i].m_level = 5;
    for (int i = 0; i < 150; ++i) {
        g_creatureTypeTraits[i].m_attributes = 0x10;
        g_creatureTypeTraits[i].m_damageHighBound = 5;
    }
}

bool check(Chance chance) {
    resetTraits();
    hero target;

    // Retail's creature-table default (+0x325) reaches hero resistance.
    // Dwarves set a base chance first; Black/Magic and Arrow Tower reject.
    for (int id = 0; id < 150; ++id) {
        float expected = 0.75f;
        int calls = 1;
        if (id == 0x10 || id == 0x85) expected = 0.55f;
        if (id == 0x11) expected = 0.35f;
        if (id == 0x53 || id == 0x79 || id == 0x95) {
            expected = 0.0f;
            calls = 0;
        }
        target.resistanceCalls = 0;
        if (!near(chance(SPELL_SLOW, static_cast<TCreatureType>(id), 0, &target), expected)
            || target.resistanceCalls != calls)
            return false;
    }

    // Exercise every spell-table entry on a living, non-shooting creature.
    // Dispel bypasses resistance, Destroy Undead/Animate Dead require undead, and the
    // shared +0x15c shooter test rejects Precision and Forgetfulness.
    for (int spell = 0; spell < 81; ++spell) {
        float expected = 0.75f;
        if (spell == 0x23) expected = 1.0f;
        if (spell == 0x19 || spell == 0x27 || spell == 0x2c || spell == 0x3d)
            expected = 0.0f;
        if (!near(chance(spell, CREATURE_GRIFFIN, 0, &target), expected))
            return false;
    }

    // Each pendant's literal argument must stay paired with its spell.
    // A shooting creature allows Forgetfulness to reach that distinction.
    const int pendants[] = {0x64, 0x65, 0x66, 0x67, 0x69, 0x6a, 0x6b};
    const int spells[] = {0x3b, 0x3e, 0x2a, 0x18, 0x3c, 0x11, 0x3d};
    g_creatureTypeTraits[CREATURE_GRIFFIN].m_attributes |= 4;
    for (unsigned i = 0; i < sizeof(pendants) / sizeof(*pendants); ++i) {
        target.artifacts[pendants[i]] = true;
        if (chance(spells[i], CREATURE_GRIFFIN, 0, &target) != 0.0f)
            return false;
        if (!near(chance(SPELL_SLOW, CREATURE_GRIFFIN, 0, &target), 0.75f))
            return false;
        target.artifacts[pendants[i]] = false;
    }

    // Retail +0x2bd branches to zero for the trait independently of hero.
    resetTraits();
    g_spellTraits[SPELL_SLOW].m_flags = 0x400;
    g_creatureTypeTraits[CREATURE_GRIFFIN].m_attributes |= 0x400;
    if (chance(SPELL_SLOW, CREATURE_GRIFFIN, 0, 0) != 0.0f)
        return false;
    g_creatureTypeTraits[CREATURE_GRIFFIN].m_attributes &= ~0x400;
    target.artifacts[ARTIFACT_BADGE_OF_COURAGE] = true;
    if (chance(SPELL_SLOW, CREATURE_GRIFFIN, 0, &target) != 0.0f)
        return false;
    target.artifacts[ARTIFACT_BADGE_OF_COURAGE] = false;

    // Separate retail entries deliberately share only parts of the tests.
    resetTraits();
    g_creatureTypeTraits[CREATURE_GRIFFIN].m_damageHighBound = 0;
    if (!near(chance(SPELL_RESURRECTION, CREATURE_GRIFFIN, 0, &target), 0.75f))
        return false;
    g_creatureTypeTraits[CREATURE_GRIFFIN].m_attributes = 4;
    if (!near(chance(SPELL_PRECISION, CREATURE_GRIFFIN, 0, &target), 0.75f))
        return false;
    g_creatureTypeTraits[CREATURE_GRIFFIN].m_attributes = g_ctaUndead;
    g_creatureTypeTraits[CREATURE_GRIFFIN].m_damageHighBound = 5;
    if (chance(SPELL_BLESS, CREATURE_GRIFFIN, 0, &target) != 0.0f
        || !near(chance(SPELL_FORTUNE, CREATURE_GRIFFIN, 0, &target), 0.75f)
        || !near(chance(SPELL_MISFORTUNE, CREATURE_GRIFFIN, 0, &target), 0.75f)
        || !near(chance(SPELL_SLAYER, CREATURE_GRIFFIN, 0, &target), 0.75f))
        return false;

    resetTraits();
    if (chance(SPELL_STONE, CREATURE_TROGLODYTE, 0, &target) != 0.0f
        || !near(chance(SPELL_STONE, CREATURE_STONE_GARGOYLE, 0, &target), 0.75f)
        || chance(SPELL_POISON, CREATURE_STONE_GARGOYLE, 0, &target) != 0.0f
        || !near(chance(SPELL_POISON, CREATURE_TROGLODYTE, 0, &target), 0.75f))
        return false;
    g_creatureTypeTraits[CREATURE_GRIFFIN].m_attributes = 0;
    if (chance(SPELL_POISON, CREATURE_GRIFFIN, 0, &target) != 0.0f)
        return false;

    // Table +0x4b8 routes Green/Red/Azure together, Gold separately.
    resetTraits();
    const TCreatureType dragons[] = {CREATURE_GREEN_DRAGON, CREATURE_RED_DRAGON,
                                    CREATURE_AZURE_DRAGON, CREATURE_GOLD_DRAGON};
    for (int level = 3; level <= 5; ++level) {
        g_spellTraits[SPELL_SLOW].m_level = level;
        for (int i = 0; i < 4; ++i) {
            float expected = level <= (i == 3 ? 4 : 3) ? 0.0f : 0.75f;
            if (!near(chance(SPELL_SLOW, dragons[i], 0, &target), expected))
                return false;
        }
    }

    // Vulnerability bypasses the resistance stage; early spell gates stay.
    target.artifacts[ARTIFACT_ORB_OF_VULNERABILITY] = true;
    target.resistanceCalls = 0;
    if (chance(SPELL_SLOW, CREATURE_BLACK_DRAGON, 0, &target) != 1.0f
        || target.resistanceCalls != 0
        || chance(SPELL_ANIMATE_DEAD, CREATURE_GRIFFIN, 0, &target) != 0.0f)
        return false;
    target.artifacts[ARTIFACT_ORB_OF_VULNERABILITY] = false;

    target.resistanceFactor = 0.1f;
    if (chance(SPELL_SLOW, CREATURE_DWARF, 0, &target) != 0.0f)
        return false;
    g_spellTraits[SPELL_SLOW].m_karma = 1;
    target.resistanceCalls = 0;
    if (chance(SPELL_SLOW, CREATURE_DWARF, 0, &target) != 1.0f
        || target.resistanceCalls != 1)
        return false;
    return true;
}

int main() {
    // @CHECKS@
    return 0;
}
