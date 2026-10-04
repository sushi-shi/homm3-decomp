// Build-time exporter only. Generation never links a C++ runtime.
// The includes contain the exact same initializers the game TUs consume.
#include <cstdio>
#include <cstdint>
#include <cstring>
#include "rmg_line_pattern.h"
#include "tile_direction.h"
#include "rmg_settings.h"
#include "town_type.h"
#include "terrain_type.h"

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
