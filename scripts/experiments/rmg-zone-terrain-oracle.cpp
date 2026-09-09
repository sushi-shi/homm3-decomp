// Native-only selector oracle. Actual source methods, relevant field
// declarations, terrain constants, coordinate type and table are inserted
// by test_rmg_zone_terrain.py. No instrumentation enters a VC6 snapshot.
#include <cstdio>
#include "terrain_type.h"
struct TPoint;
// @POSITION_TYPE@
struct TRmgTownSlot {
    // @SLOT_FIELDS@
};
struct TerrainRoot {
    // @ZONE_FIELDS@
};
// @NATIVE_TABLE@
static int g_terrainRandomValue;
static int g_terrainRandomDraws;
static int terrainRandom() { ++g_terrainRandomDraws; return g_terrainRandomValue; }
#define rand terrainRandom
// @CANDIDATES@

template<class Candidate> static bool check() {
    const int native[9] = {2, 2, 3, 7, 0, 0, 5, 4, 2}; // pinned 0x6408c8
    const int draws[] = {0, 1, 2, 7, 31, 32767};
    for (int mask = 0; mask < 256; ++mask)
    for (int level = -1; level <= 2; ++level)
    for (int alignment = -1; alignment < 9; ++alignment)
    for (int useNative = 0; useNative < 3; ++useNative)
    for (int draw = 0; draw < 6; ++draw) {
        Candidate actual;
        TRmgTownSlot slot;
        slot.m_useNativeTerrain = useNative == 2 ? 255 : useNative;
        for (int i = 0; i < 8; ++i)
            slot.m_allowedTerrain[i] = (mask & (1 << i)) ? (i & 1 ? 255 : 2) : 0;
        actual.m_slot = &slot;
        actual.m_alignment = alignment;
        actual.m_levelPosition.m_x = 31;
        actual.m_levelPosition.m_y = -5;
        actual.m_levelPosition.m_z = level;
        actual.m_terrain = -123;
        int expected = 0, expectedDraws = 0;
        if (useNative && alignment != -1) expected = native[alignment];
        else {
            int eligible[8], count = 0;
            // Independent rank oracle: build an explicit eligible list,
            // index it once, and never use the candidate's countdown loop.
            for (int terrain = 0; terrain < 8; ++terrain)
                if ((mask & (1 << terrain)) && (terrain != 6 || level == 1))
                    eligible[count++] = terrain;
            if (count) { expected = eligible[draws[draw] % count]; expectedDraws = 1; }
        }
        if (level == 1 && expected != 7) expected = 6;
        g_terrainRandomValue = draws[draw]; g_terrainRandomDraws = 0;
        actual.chooseTerrain();
        if (actual.m_terrain != expected || g_terrainRandomDraws != expectedDraws
            || actual.m_slot != &slot || actual.m_alignment != alignment
            || actual.m_levelPosition.m_x != 31 || actual.m_levelPosition.m_y != -5
            || actual.m_levelPosition.m_z != level
            || slot.m_useNativeTerrain != (useNative == 2 ? 255 : useNative)) return false;
        for (int i = 0; i < 8; ++i)
            if (slot.m_allowedTerrain[i] != ((mask & (1 << i)) ? (i & 1 ? 255 : 2) : 0)) return false;
    }
    return true;
}
int main() {
    // @CHECKS@
    std::puts("terrain selectors preserve all masks, native preferences, exact levels and random draw counts");
}
