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

int main()
{
    emitTerrainRules();
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
