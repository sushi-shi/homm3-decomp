// Native harness for the authored function. Helpers are controlled inputs;
// this checks text composition, not the independent GetLuck calculation.
// Retail 0x44c1c0 stores GetLuck at EBP+14h, subtracts the Clover Field
// bonus from EBP+10h, tests EBP+14h for the Halfling floor, then subtracts
// EBP+14h from EBP+10h. These separate homes define the oracle below.
#include <string>
#include <cstdio>
#include <cstdarg>

// @ENUMS@

static const int EXTRA_0_ID = 21;
struct CreatureTraits { int m_townType; };
static CreatureTraits g_creatureTypeTraits[151];
struct ArtifactTraits { const char* m_name; };
static ArtifactTraits g_artifactTraits[144];
struct Game { int m_f1f698; };
static Game game;
static Game* g_game = &game;
static const char* g_cursedGroundLuckText = "cursed;";
static const char* g_hourglassLuckFormat = "hourglass:%s;";
static const char* g_cloverFieldLuckText = "clover;";
static const char* g_enemyCreatureStatFormat = "enemy:%s;";
static const char* g_otherStatModifiersFormat = "other:%d;";

static std::string formatString(const char* format, ...)
{
    char text[128];
    va_list args;
    va_start(args, format);
    vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    return text;
}
static const char* getArmyName(int, int) { return "creatures"; }
static const char* getBuildingName(int, int) { return "building"; }
struct hero {
    bool hourglass;
    bool isWieldingArtifact(int) const { return hourglass; }
    std::string getLuckDescription() const { return "hero;"; }
};
struct town {
    char m_type;
    bool hasBuilding(int, int) const { return false; }
};
struct armyGroup {
    int baseLuck;
    mutable int luckCalls;
    int getLuck(const hero*, const town*, const hero*, const armyGroup*,
                int, int) const { ++luckCalls; return baseLuck; }
    bool isMember(TCreatureType) const { return false; }
    std::string getLuckDescription(TCreatureType, int, const hero*,
        const town*, const hero*, const armyGroup*, int) const;
    std::string wrongAccumulator(TCreatureType, int, const hero*,
        const town*, const hero*, const armyGroup*, int) const;
};

// @CANDIDATES@

typedef std::string (armyGroup::*Description)(TCreatureType, int, const hero*,
    const town*, const hero*, const armyGroup*, int) const;

static bool check(Description description)
{
    const TCreatureType creatures[] = { CREATURE_GRIFFIN, CREATURE_HALFLING,
        CREATURE_AIR_ELEMENTAL, CREATURE_EARTH_ELEMENTAL,
        CREATURE_FIRE_ELEMENTAL, CREATURE_WATER_ELEMENTAL };
    const int terrains[] = { MAGIC_TERRAIN_COAST, MAGIC_TERRAIN_CLOVER_FIELD,
                            MAGIC_TERRAIN_CURSED_GROUND };
    armyGroup group;
    // Include out-of-range town values, both expansion modes, every excluded
    // elemental, and values on both sides of the Halfling floor. Requested
    // luck is independent of the helper's result, as in the retail ABI.
    for (int type = -1; type <= 9; ++type)
    for (int expansion = 0; expansion != 2; ++expansion)
    for (unsigned c = 0; c < sizeof(creatures)/sizeof(*creatures); ++c)
    for (unsigned t = 0; t < sizeof(terrains)/sizeof(*terrains); ++t)
    for (int base = -2; base <= 3; ++base)
    for (int requested = -2; requested <= 5; ++requested) {
        group.baseLuck = base;
        group.luckCalls = 0;
        game.m_f1f698 = expansion;
        g_creatureTypeTraits[creatures[c]].m_townType = type;
        std::string expected;
        if (terrains[t] == MAGIC_TERRAIN_CURSED_GROUND) {
            expected = "cursed;";
        } else {
            const int terrainBonus = terrains[t] == MAGIC_TERRAIN_CLOVER_FIELD
                && type >= 6 && type <= 8 && (expansion || c < 2) ? 2 : 0;
            const bool floor = c == 1 && base < 1;
            const int accountedLuck = terrainBonus + (floor ? 1 : base);
            if (terrainBonus) expected += "clover;";
            if (floor) expected += "creatures are always lucky";
            if (requested != accountedLuck)
                expected += formatString("other:%d;", requested - accountedLuck);
        }
        const std::string actual = (group.*description)(creatures[c], requested,
            0, 0, 0, 0, terrains[t]);
        if (actual != expected || group.luckCalls !=
                (terrains[t] == MAGIC_TERRAIN_CURSED_GROUND ? 0 : 1))
            return false;
    }
    return true;
}

int main()
{
    if (!check(&armyGroup::getLuckDescription)) {
        std::puts("Authored luck description disagrees with retail accumulator");
        return 1;
    }
    if (check(&armyGroup::wrongAccumulator)) {
        std::puts("Wrong-accumulator negative control unexpectedly passed");
        return 1;
    }
    std::puts("19008 luck/terrain combinations and negative control passed");
    return 0;
}
