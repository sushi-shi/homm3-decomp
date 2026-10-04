// Build-time exporter only. Generation never links a C++ runtime.
// The includes contain the exact same initializers the game TUs consume.
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include "rmg_line_pattern.h"
#include "tile_direction.h"
#include "rmg_settings.h"
#include "town_type.h"
#include "terrain_type.h"
#include "rmg_terrain_data.h"
#include "adventure_object_data.h"
#include "adventure_object_type.h"
#include "adventure_object_subtype.h"
#include "game_resource.h"
#include "spellschool.h"

#include "creature_traits.h"
#include "spell_traits.h"
#include "hero_traits.h"
#include "artifact_data.h"

// Build-host point adapter: the shared initializer exports scalar offsets only.
struct TPoint {
    int m_x, m_y;
    TPoint(int x, int y) : m_x(x), m_y(y) {}
};
static const TPoint directions[] =
#include "rmg_data/directions.inc"
;

static const TPoint portalBorderOffsets[RMG_PORTAL_BORDER_OFFSET_COUNT] =
#include "rmg_data/portal_border_offsets.inc"
;
static const TPoint shipyardWaterOffsets[RMG_SHIPYARD_WATER_OFFSET_COUNT] =
#include "rmg_data/shipyard_water_offsets.inc"
;

static const int guardThresholdLow[RMG_GUARD_STRENGTH_COUNT] =
#include "rmg_data/guard_threshold_low.inc"
;
static const int guardThresholdHigh[RMG_GUARD_STRENGTH_COUNT] =
#include "rmg_data/guard_threshold_high.inc"
;
static const int guardScaleLow[RMG_GUARD_STRENGTH_COUNT] =
#include "rmg_data/guard_scale_low.inc"
;
static const int guardScaleHigh[RMG_GUARD_STRENGTH_COUNT] =
#include "rmg_data/guard_scale_high.inc"
;

static const THeroTraits heroTraits[] =
#include "rmg_data/hero_traits.inc"
;
static const int disabledArtifacts[] =
#include "rmg_data/disabled_artifacts.inc"
;
static const int artifactSlotColumns[] =
#include "rmg_data/artifact_slot_columns.inc"
;

// Build-host adapters consume the canonical factory argument lists. They
// export values only; target bitset representation and allocator are irrelevant.
template <unsigned N, class... Indices>
std::bitset<N> makeHostMask(unsigned count, Indices... indices)
{
    if (count != sizeof...(indices)) std::abort();
    std::bitset<N> result;
    const unsigned values[] = {static_cast<unsigned>(indices)...};
    for (unsigned index : values) result.set(index);
    return result;
}
template <class... Indices>
std::bitset<144> makeArtifactComponentMask(unsigned count, Indices... indices)
{
    return makeHostMask<144>(count, indices...);
}
template <class... Indices>
std::bitset<19> makeArtifactSlotMask(unsigned count, Indices... indices)
{
    return makeHostMask<19>(count, indices...);
}
static const TCombinationArtifact artifactCombinations[] =
#include "rmg_data/artifact_combinations.inc"
;
static const TArtifactSlotMask artifactSlotMasks[] =
#include "rmg_data/artifact_slot_masks.inc"
;

static const TCreatureTypeTraits creatureTraits[] =
#include "rmg_data/creature_traits.inc"
;
static const SSpellTraits spellTraits[] =
#include "rmg_data/spell_traits.inc"
;

static const TAdvObjectNameRow objectNameRows[] =
#include "rmg_data/object_name_rows.inc"
;
static const int objectDecorationIds[] =
#include "rmg_data/object_decoration_ids.inc"
;
static const int objectClearedIds[] =
#include "rmg_data/object_cleared_ids.inc"
;
static const int objectLandBlockedIds[] =
#include "rmg_data/object_land_blocked_ids.inc"
;
static const int objectNorthIds[] =
#include "rmg_data/object_north_ids.inc"
;

static const TRmgTerrainTransitionEntry rockFrames[] =
#include "rmg_data/rock_frames.inc"
;
static const TRmgTerrainPatternEntry g_rmgLandPatternEntries[] =
#include "rmg_data/land_frames.inc"
;
static const TRmgTerrainPatternEntry g_rmgDirtPatternEntries[] =
#include "rmg_data/dirt_frames.inc"
;
static const TRmgTerrainPatternEntry g_rmgSandPatternEntries[] =
#include "rmg_data/sand_frames.inc"
;
static const TRmgTerrainPatternEntry g_rmgWaterPatternEntries[] =
#include "rmg_data/water_frames.inc"
;

// Host-only adapter for the canonical constructor argument lists.
struct RuleMetadata {
    bool blends, separated;
    int chance;
    unsigned count;
    const TRmgTerrainPatternEntry* entries;
};
static const RuleMetadata g_rmgDirtRule = {
#include "rmg_data/dirt_rule.inc"
};
static const RuleMetadata g_rmgSandRule = {
#include "rmg_data/sand_rule.inc"
};
static const RuleMetadata g_rmgGrassRule = {
#include "rmg_data/grass_rule.inc"
};
static const RuleMetadata g_rmgSnowRule = {
#include "rmg_data/snow_rule.inc"
};
static const RuleMetadata g_rmgSwampRule = {
#include "rmg_data/swamp_rule.inc"
};
static const RuleMetadata g_rmgRoughRule = {
#include "rmg_data/rough_rule.inc"
};
static const RuleMetadata g_rmgSubterraneanRule = {
#include "rmg_data/subterranean_rule.inc"
};
static const RuleMetadata g_rmgLavaRule = {
#include "rmg_data/lava_rule.inc"
};
static const RuleMetadata g_rmgWaterRule = {
#include "rmg_data/water_rule.inc"
};
static const RuleMetadata g_rmgRockRule = {};
static const RuleMetadata* const terrainRules[] =
#include "rmg_data/terrain_rules.inc"
;

template <unsigned long N>
void emitPatternFrames(const char* name, const TRmgTerrainPatternEntry (&frames)[N])
{
    std::printf("pub const %s: [TRmgTerrainPatternEntry; %lu] = [", name, N);
    for (const auto& frame : frames)
        std::printf("TRmgTerrainPatternEntry { m_transition: %u, m_special: %u },", frame.m_transition, frame.m_special);
    std::printf("];\n");
}

template <unsigned long N>
bool isPatternTable(const RuleMetadata& rule, const TRmgTerrainPatternEntry (&frames)[N])
{
    if (rule.entries != frames) return false;
    if (rule.count != N) std::abort();
    return true;
}

void emitTerrainRules()
{
    emitPatternFrames("LAND_FRAMES", g_rmgLandPatternEntries);
    emitPatternFrames("DIRT_FRAMES", g_rmgDirtPatternEntries);
    emitPatternFrames("SAND_FRAMES", g_rmgSandPatternEntries);
    emitPatternFrames("WATER_FRAMES", g_rmgWaterPatternEntries);
    std::printf("pub const ROCK_FRAMES: [TRmgTerrainTransitionEntry; %lu] = [", sizeof(rockFrames)/sizeof(rockFrames[0]));
    for (const auto& frame : rockFrames)
        std::printf("TRmgTerrainTransitionEntry { m_transition: %u, m_flipX: %u, m_flipY: %u },", frame.m_transition, frame.m_flipX, frame.m_flipY);
    std::printf("];\n");
    std::printf("#[derive(Clone, Copy, Debug)] pub enum TerrainRuleData { Pattern { blends: bool, separated: bool, chance: u32, entries: &'static [TRmgTerrainPatternEntry] }, Fixed }\n");
    std::printf("pub const TERRAIN_RULES: [TerrainRuleData; %lu] = [", sizeof(terrainRules)/sizeof(terrainRules[0]));
    for (const auto* rule : terrainRules) {
        if (rule == &g_rmgRockRule) { std::printf("TerrainRuleData::Fixed,"); continue; }
        const char* table = isPatternTable(*rule, g_rmgLandPatternEntries) ? "LAND_FRAMES"
            : isPatternTable(*rule, g_rmgDirtPatternEntries) ? "DIRT_FRAMES"
            : isPatternTable(*rule, g_rmgSandPatternEntries) ? "SAND_FRAMES"
            : isPatternTable(*rule, g_rmgWaterPatternEntries) ? "WATER_FRAMES" : nullptr;
        if (!table) std::abort();
        std::printf("TerrainRuleData::Pattern { blends: %s, separated: %s, chance: %u, entries: &%s },",
            rule->blends ? "true" : "false", rule->separated ? "true" : "false", rule->chance, table);
    }
    std::printf("];\n");
}

static const ERmgLinePattern river[] =
#include "rmg_data/river_patterns.inc"
;
static const ERmgLinePattern road[] =
#include "rmg_data/road_patterns.inc"
;
static const unsigned char reflections[][2] =
#include "rmg_data/line_reflections.inc"
;
static const int neighbours[][2][TILE_DIR_COUNT] =
#include "rmg_data/reflected_neighbours.inc"
;
static const TTownType terrainTowns[][RMG_TERRAIN_TOWN_CHOICE_COUNT] =
#include "rmg_data/terrain_towns.inc"
;
static const TTerrainType nativeTerrain[] =
#include "rmg_data/native_terrain.inc"
;
static const double cosines[] =
#include "rmg_data/radial_cosines.inc"
;
static const double sines[] =
#include "rmg_data/radial_sines.inc"
;

template <unsigned long N>
void emitDoubles(const char* name, const double (&array)[N])
{
    static_assert(sizeof(double) == sizeof(std::uint64_t), "binary64 host required");
    std::printf("pub const %s: [u64; %lu] = [", name, N);
    for (unsigned long index = 0; index < N; ++index) {
        std::uint64_t bits;
        std::memcpy(&bits, &array[index], sizeof(bits));
        std::printf("0x%llx,", static_cast<unsigned long long>(bits));
    }
    std::printf("];\n");
}

template <class T, unsigned long N>
void emitArray(const char* name, const T (&array)[N])
{
    std::printf("pub const %s: [u32; %lu] = [", name, N);
    for (unsigned long index = 0; index < N; ++index)
        std::printf("%u,", static_cast<unsigned>(array[index]));
    std::printf("];\n");
}

template <unsigned long N>
void emitSignedArray(const char* name, const int (&array)[N])
{
    std::printf("pub const %s: [i32; %lu] = [", name, N);
    for (int value : array) std::printf("%d,", value);
    std::printf("];\n");
}

static const int creatureRewardValues[] =
#include "rmg_data/creature_reward_values.inc"
;
static const int keyTentValues[] =
#include "rmg_data/treasure_tent_values.inc"
;
// These adapters export constructor arguments, preserving their source order.
static void emitTreasureDef(int kind, int subtype, int value, int density)
{
    std::printf("TreasureRecipe::Plain(%d,%d,%d,%d),", kind, subtype, value, density);
}
static void emitArtifactDef(int kind, int value)
{
    std::printf("TreasureRecipe::Artifact(%d,%d),", kind, value);
}
static void emitBlackBoxExperienceDef(int value, int experience)
{
    std::printf("TreasureRecipe::Experience(%d,%d),", value, experience);
}
static void emitBlackBoxGoldDef(int value, int gold)
{
    std::printf("TreasureRecipe::Gold(%d,%d),", value, gold);
}
static void emitBlackBoxSpellsDef(int value, int minimumLevel, int maximumLevel, int schoolMask)
{
    std::printf("TreasureRecipe::Spells(%d,%d,%d,%d),", value, minimumLevel, maximumLevel, schoolMask);
}
static void emitPrisonDef(int value, int experience)
{
    std::printf("TreasureRecipe::Prison(%d,%d),", value, experience);
}
static void emitResourceLumpDef(int kind, int subtype, int value, int density)
{
    std::printf("TreasureRecipe::Resource(%d,%d,%d,%d),", kind, subtype, value, density);
}
static void emitScholarDef()
{
    std::printf("TreasureRecipe::Scholar,");
}
static void emitShrineDef(int kind, int value)
{
    std::printf("TreasureRecipe::Shrine(%d,%d),", kind, value);
}
static void emitSpellScrollDef(int level, int value)
{
    std::printf("TreasureRecipe::Scroll(%d,%d),", level, value);
}
static void emitWitchHutDef()
{
    std::printf("TreasureRecipe::WitchHut,");
}
static void emitTreasureRecipes()
{
    std::printf("#[derive(Clone, Copy, Debug, PartialEq, Eq)]\npub enum TreasureRecipe { Plain(i32,i32,i32,i32), Artifact(i32,i32), Experience(i32,i32), Gold(i32,i32), Spells(i32,i32,i32,i32), Prison(i32,i32), Resource(i32,i32,i32,i32), Scholar, Shrine(i32,i32), Scroll(i32,i32), WitchHut, CreatureBoxes, KeyTents, Dwellings, Seers }\n");
    std::printf("pub const TREASURE_RECIPES: &[TreasureRecipe] = &[");
#define RMG_TREASURE(Type, args) emit##Type args;
#define RMG_CREATURE_BOXES() std::printf("TreasureRecipe::CreatureBoxes,");
#define RMG_KEY_TENTS() std::printf("TreasureRecipe::KeyTents,");
#define RMG_MAP_DWELLINGS() std::printf("TreasureRecipe::Dwellings,");
#define RMG_SEER_QUESTS() std::printf("TreasureRecipe::Seers,");
#include "rmg_data/treasure_recipes.inc"
#undef RMG_SEER_QUESTS
#undef RMG_MAP_DWELLINGS
#undef RMG_KEY_TENTS
#undef RMG_CREATURE_BOXES
#undef RMG_TREASURE
    std::printf("];\n#[derive(Clone, Copy, Debug, PartialEq, Eq)]\npub enum SeerRewardRecipe { Experience(i32,i32), Gold(i32,i32) }\npub const SEER_REWARD_RECIPES: &[SeerRewardRecipe] = &[");
#define RMG_SEER_REWARD(Type, value, amount) std::printf("SeerRewardRecipe::" #Type "(%d,%d),", value, amount);
#include "rmg_data/treasure_seer_rewards.inc"
#undef RMG_SEER_REWARD
    std::printf("];\n");
}

int main()
{
    std::printf("pub const DIRECTIONS: [(i32, i32); %lu] = [", sizeof(directions)/sizeof(directions[0]));
    for (const auto& point : directions) std::printf("(%d,%d),", point.m_x, point.m_y);
    std::printf("];\n");
    // The availability bytes are the little-endian retail attributes word.
    // Decode the word explicitly so export does not depend on host endianness.
    std::printf("pub const HERO_AVAILABILITY: [(bool, bool, bool); %lu] = [", sizeof(heroTraits)/sizeof(heroTraits[0]));
    for (const auto& row : heroTraits)
        std::printf("(%s,%s,%s),", row.m_attributes & 0xff ? "true" : "false",
            row.m_attributes & 0xff00 ? "true" : "false", row.m_attributes & 0xff0000 ? "true" : "false");
    std::printf("];\n");
    emitArray("DISABLED_ARTIFACTS", disabledArtifacts);
    emitArray("ARTIFACT_SLOT_COLUMNS", artifactSlotColumns);
    std::printf("pub const COMBINATION_ARTIFACTS: [u32; %lu] = [", sizeof(artifactCombinations)/sizeof(artifactCombinations[0]));
    for (const auto& row : artifactCombinations) std::printf("%u,", static_cast<unsigned>(row.m_artifactId));
    std::printf("];\n");
    std::printf("pub const ARTIFACT_SLOT_MASKS: [u32; %lu] = [", sizeof(artifactSlotMasks)/sizeof(artifactSlotMasks[0]));
    for (const auto& row : artifactSlotMasks) std::printf("%lu,", row.to_ulong());
    std::printf("];\n");
    emitArray("OBJECT_DECORATION_IDS", objectDecorationIds);
    emitArray("OBJECT_CLEARED_IDS", objectClearedIds);
    emitArray("OBJECT_LAND_BLOCKED_IDS", objectLandBlockedIds);
    emitArray("OBJECT_NORTH_IDS", objectNorthIds);
    std::printf("pub const OBJECT_NAME_ROWS: [TAdvObjectNameRow; %lu] = [", sizeof(objectNameRows)/sizeof(objectNameRows[0]));
    for (const auto& row : objectNameRows)
        std::printf("TAdvObjectNameRow { m_objectType: %d, m_nameRow: %d },", row.m_objectType, row.m_nameRow);
    std::printf("];\n");
    std::printf("pub const CREATURE_FACTIONS_AND_LEVELS: [(i32, i32); %lu] = [", sizeof(creatureTraits)/sizeof(creatureTraits[0]));
    for (const auto& row : creatureTraits)
        std::printf("(%d,%d),", row.m_townType, row.m_level);
    std::printf("];\n");
    std::printf("pub const SPELL_FLAGS: [u32; %lu] = [", sizeof(spellTraits)/sizeof(spellTraits[0]));
    for (const auto& row : spellTraits) std::printf("%u,", row.m_flags);
    std::printf("];\n");
    std::printf("pub const SHIPYARD_WATER_OFFSETS: [(i32, i32); %u] = [", RMG_SHIPYARD_WATER_OFFSET_COUNT);
    for (const auto& point : shipyardWaterOffsets) std::printf("(%d,%d),", point.m_x, point.m_y);
    std::printf("];\n");
    std::printf("pub const PORTAL_BORDER_OFFSETS: [(i32, i32); %u] = [", RMG_PORTAL_BORDER_OFFSET_COUNT);
    for (const auto& point : portalBorderOffsets) std::printf("(%d,%d),", point.m_x, point.m_y);
    std::printf("];\n");
    emitTreasureRecipes();
    emitSignedArray("CREATURE_REWARD_VALUES", creatureRewardValues);
    emitSignedArray("KEY_TENT_REWARD_VALUES", keyTentValues);
    emitTerrainRules();
    emitSignedArray("GUARD_THRESHOLD_LOW", guardThresholdLow);
    emitSignedArray("GUARD_THRESHOLD_HIGH", guardThresholdHigh);
    emitSignedArray("GUARD_SCALE_LOW", guardScaleLow);
    emitSignedArray("GUARD_SCALE_HIGH", guardScaleHigh);
    emitArray("RIVER_PATTERNS", river);
    emitArray("ROAD_PATTERNS", road);
    emitArray("NATIVE_TERRAIN", nativeTerrain);
    emitDoubles("RADIAL_COSINE_BITS", cosines);
    emitDoubles("RADIAL_SINE_BITS", sines);
    std::printf("pub const TERRAIN_TOWNS: [[i32; %u]; %u] = [",
                RMG_TERRAIN_TOWN_CHOICE_COUNT, eTerrainWater + 1);
    for (unsigned terrain = 0; terrain <= eTerrainWater; ++terrain) {
        std::printf("[");
        for (unsigned choice = 0; choice < RMG_TERRAIN_TOWN_CHOICE_COUNT; ++choice)
            std::printf("%d,", static_cast<int>(terrainTowns[terrain][choice]));
        std::printf("],");
    }
    std::printf("];\n");
    std::printf("pub const LINE_REFLECTIONS: [[bool; 2]; 4] = [");
    for (unsigned index = 0; index < 4; ++index)
        std::printf("[%s,%s],", reflections[index][0] ? "true" : "false",
                    reflections[index][1] ? "true" : "false");
    std::printf("];\n");
    std::printf("pub const REFLECTED_NEIGHBOURS: [[[usize; %u]; 2]; 2] = [", TILE_DIR_COUNT);
    for (unsigned x = 0; x < 2; ++x) {
        std::printf("[");
        for (unsigned y = 0; y < 2; ++y) {
            std::printf("[");
            for (unsigned direction = 0; direction < TILE_DIR_COUNT; ++direction)
                std::printf("%u,", static_cast<unsigned>(neighbours[x][y][direction]));
            std::printf("],");
        }
        std::printf("],");
    }
    std::printf("];\n");
    return std::ferror(stdout) ? 1 : 0;
}
