// Actual coordinate operations and packed fields are supplied by the runner.
#include <vector>
#include <cstdio>
#include <cstring>
#include "terrain_type.h"
// @TYPES@
// @OFFSETS@
struct TRmgMapItem {
    TRmgGroundTile m_tile;
    TRmgGroundTileData m_tileData;
    // @GATE@
};
struct type_random_map {
    TRmgMapPosition m_size;
    TRmgMapItem* m_mapItems;
    std::vector<int> m_reads;
    bool m_badRead;
    TRmgMapItem m_invalid;
    // @ACCESSOR@
    TRmgMapItem* getMapItem(TRmgMapPosition point);
};
// @HELPERS@
struct Root { type_random_map m_map; };

static const TRmgMapItem& read(const std::vector<TRmgMapItem>& cells,
    int width, int height, int x, int y, int level, std::vector<int>& reads) {
    int index = x + width * y + width * height * level;
    reads.push_back(index);
    return cells[index];
}

static bool reference(const std::vector<TRmgMapItem>& cells, int w, int h,
    TRmgMapPosition p, std::vector<int>& reads) {
    if (p.m_y >= h - 1) return false;
    for (int row = 0; row != 2; ++row) {
        for (int column = -2; column != 1; ++column) {
            const TRmgMapItem& cell = read(cells, w, h, p.m_x + column, p.m_y + row, p.m_z, reads);
            if (cell.m_tile.m_landType == 8) return false;
            if (cell.m_tileData.m_roadEntrance || !cell.m_tileData.m_roadPassable || cell.m_tile.m_landType == 9)
                return false;
        }
    }
    for (int probe = 0; probe != 4; ++probe) {
        int side = probe % 2 ? 1 : -3;
        int x = p.m_x + side;
        if (x < 0 || x >= w) continue;
        const TRmgMapItem& cell = read(cells, w, h, x, p.m_y + probe / 2, p.m_z, reads);
        if (cell.m_tile.m_landType != 8 || !cell.m_tileData.m_subterraneanGate) continue;
        int opposite = p.m_x + (side == -3 ? 1 : -3);
        if (opposite < 0 || opposite >= w) return false;
        return read(cells, w, h, opposite, p.m_y, p.m_z, reads).m_tile.m_landType != 8;
    }
    return false;
}

template<class Candidate> static bool check() {
    int accepted = 0;
    for (int scenario = 0; scenario != 4800; ++scenario) {
        int w = 4 + scenario % 9, h = 2 + scenario / 9 % 7;
        TRmgMapPosition p(2 + scenario / 63 % (w - 2), scenario / 11 % h, scenario % 2);
        std::vector<TRmgMapItem> cells(w * h * 2);
        for (unsigned i = 0; i != cells.size(); ++i) {
            std::memset(&cells[i], 0, sizeof(cells[i]));
            cells[i].m_tile.m_landType = int((i * 31 + scenario * 17) % 64) - 32;
            cells[i].m_tileData.m_roadPassable = (i + scenario) % 3 != 0;
            cells[i].m_tileData.m_roadEntrance = (i + scenario) % 5 == 0;
            cells[i].m_tileData.m_subterraneanGate = (i + scenario) % 2;
        }
        if (scenario % 3 && p.m_y + 1 < h) {
            for (int i = p.m_z * w * h; i < (p.m_z + 1) * w * h; ++i) {
                cells[i].m_tile.m_landType = 0;
                cells[i].m_tileData.m_roadPassable = 1;
                cells[i].m_tileData.m_roadEntrance = 0;
            }
            int probe = scenario / 3 % 4;
            int x = p.m_x + (probe % 2 ? 1 : -3), y = p.m_y + probe / 2;
            if (x >= 0 && x < w) {
                TRmgMapItem& cell = cells[x + w * y + w * h * p.m_z];
                cell.m_tile.m_landType = 8;
                cell.m_tileData.m_subterraneanGate = scenario % 7 != 0;
            }
            if (scenario % 13 == 0)
                cells[p.m_x - 1 + w * p.m_y + w * h * p.m_z].m_tile.m_landType = 9;
            if (scenario % 17 == 0)
                cells[p.m_x + w * (p.m_y + 1) + w * h * p.m_z].m_tileData.m_roadEntrance = 1;
        }
        std::vector<TRmgMapItem> saved = cells;
        std::vector<int> reads;
        bool expected = reference(cells, w, h, p, reads);
        accepted += expected;
        Candidate candidate;
        candidate.m_map.m_size.m_x = w;
        candidate.m_map.m_size.m_y = h;
        candidate.m_map.m_mapItems = &cells[0];
        candidate.m_map.m_badRead = false;
        std::memset(&candidate.m_map.m_invalid, 0, sizeof(candidate.m_map.m_invalid));
        bool actual = candidate.canPlaceShipyard(p) != 0;
        if (actual != expected || candidate.m_map.m_badRead || candidate.m_map.m_reads != reads
            || std::memcmp(&cells[0], &saved[0], cells.size() * sizeof(cells[0]))) return false;
    }
    return accepted > 100;
}
// @CANDIDATES@
int main() {
    // @CHECKS@
    return 0;
}
