// Actual coordinate helpers and packed tile fields are imported by the test.
// The reference uses affine sample positions, not the candidate's three walks.
#include <vector>
#include <cstdio>
#include <cstring>
#include <stdint.h>
// @TYPES@
// @HELPERS@
// @DIRECTIONS@
struct TRmgMapItem {
    unsigned m_guard;
    TRmgGroundTile m_tile;
    TRmgGroundTileData m_tileData;
    // @ENTRANCE@
};
struct type_random_map {
    int m_mapWidth, m_mapHeight;
    TRmgMapItem* m_mapItems;
    // @ACCESSOR@
};
struct Root { type_random_map m_map; };
struct Sample { int x, y, water; };
static std::vector<Sample> samples(int x, int y, int d) {
    std::vector<Sample> result;
    int side = (d - 2) & 7;
    for (int i = 0; i < 10; ++i) {
        Sample s;
        int start = (d + (i < 3 ? 2 : 1)) & 7;
        if (i < 6) {
            int n = i % 3;
            s.x = x + g_rmgDirections[start].m_x + n * g_rmgDirections[side].m_x;
            s.y = y + g_rmgDirections[start].m_y + n * g_rmgDirections[side].m_y;
        } else {
            s.x = x + (i - 5) * g_rmgDirections[d].m_x;
            s.y = y + (i - 5) * g_rmgDirections[d].m_y;
        }
        s.water = i < 3;
        result.push_back(s);
    }
    return result;
}
static bool valid(const Sample& s, int w, int h) {
    return s.x >= 0 && s.x <= w && s.y >= 0 && s.y < h;
}
static unsigned word(const TRmgGroundTileData& d) {
    unsigned v; std::memcpy(&v, &d, 4); return v;
}
template<class T> static bool check() {
    for (int scenario = 0; scenario < 1800; ++scenario) {
        int w = 5 + scenario % 7, h = 5 + scenario / 7 % 5;
        int z = scenario % 2, d = (scenario / 2 % 4) * 2;
        int x = scenario % 3 ? w / 2 : (scenario / 13 % (w + 3)) - 1;
        int y = scenario % 3 ? h / 2 : (scenario / 17 % (h + 3)) - 1;
        std::vector<TRmgMapItem> cells(w * h * 2 + w + 4);
        for (unsigned i = 0; i < cells.size(); ++i) {
            cells[i].m_guard = 0x12340000 + i;
            unsigned a = (i + 3) * 0x13579bdU, b = (i + scenario + 1) * 0x2468aceU;
            std::memcpy(&cells[i].m_tile, &a, 4);
            std::memcpy(&cells[i].m_tileData, &b, 4);
            cells[i].m_tile.m_landType = 0;
            cells[i].m_tileData.m_roadEntrance = 0;
        }
        std::vector<Sample> path = samples(x, y, d);
        // Construct complete successes, then fail each of ten terrain tests
        // and each of the seven entrance tests, including the last cell.
        for (int i = 0; i < 10; ++i) if (valid(path[i], w, h)) {
            int index = 1 + (z * h + path[i].y) * w + path[i].x;
            cells[index].m_tile.m_landType = path[i].water ? eTerrainWater : 0;
        }
        int fault = scenario / 8 % 20;
        if (fault < 17) {
            int i = fault < 10 ? fault : fault - 7;
            if (valid(path[i], w, h)) {
                int index = 1 + (z * h + path[i].y) * w + path[i].x;
                if (fault < 10) cells[index].m_tile.m_landType = path[i].water ? 0 : eTerrainWater;
                else cells[index].m_tileData.m_roadEntrance = 1;
            }
        }
        std::vector<TRmgMapItem> expected = cells;
        bool success = true;
        int last = 0;
        for (int i = 0; i < 10; ++i) {
            if (!valid(path[i], w, h)) { success = false; break; }
            last = 1 + (z * h + path[i].y) * w + path[i].x;
            bool water = expected[last].m_tile.m_landType == eTerrainWater;
            if (water != bool(path[i].water) || (!path[i].water && (word(expected[last].m_tileData) & (1u << 22)))) {
                success = false; break;
            }
        }
        if (success) {
            unsigned bits = word(expected[last].m_tileData);
            bits |= (1u << 30) | (1u << (8 + (((d - 4) >> 1) & 3)));
            std::memcpy(&expected[last].m_tileData, &bits, 4);
        }
        T candidate;
        candidate.m_map.m_mapWidth = w; candidate.m_map.m_mapHeight = h;
        candidate.m_map.m_mapItems = &cells[1];
        candidate.markRiverCoastTarget(TRmgMapPosition(x, y, z), d);
        if (std::memcmp(&cells[0], &expected[0], cells.size() * sizeof(cells[0]))) return false;
    }
    return true;
}
// @CANDIDATES@
int main() {
    // @CHECKS@
    return 0;
}
