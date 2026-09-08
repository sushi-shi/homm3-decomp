// Actual value types, arithmetic, packed flags and map accessor are imported.
// Reference propagation uses a flat fixed point seeded by geometric adjacency,
// not the candidate's vector stack or direction-table traversal.
#include <vector>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
namespace std {
template<class T> const T& _cpp_min(const T& a, const T& b) { return b < a ? b : a; }
template<class T> const T& _cpp_max(const T& a, const T& b) { return a < b ? b : a; }
}
// @TYPES@
// @DIRECTIONS@
struct TRmgMapItem { unsigned m_guard; TRmgZoneCellState m_zoneState; TRmgGroundTileData m_tileData;
    // @BOUNDARY_QUERY@
};
struct type_random_map { int m_mapWidth, m_mapHeight; TRmgMapItem* m_mapItems;
    // @ACCESSOR@
};
struct TRmgTownSlot { int m_zoneIndex; };
struct TRmgZone {
    TRmgTownSlot* m_slot;
    TRmgMapPosition m_levelPosition;
    std::vector<TPoint> m_boundary;
    int m_boundaryRoughness;
    TRmgMapPosition getLevelPosition() const;
};
// @HELPERS@
struct Edge { int x, y, previousX, previousY, zone, level, roughness; };
struct Root {
    type_random_map m_map;
    std::vector<Edge> m_edges;
    void drawIslandBoundary(TPoint point, TPoint previous, int zone, int level, int roughness) {
        Edge e = {point.m_x, point.m_y, previous.m_x, previous.m_y, zone, level, roughness};
        m_edges.push_back(e);
        // Opaque drawing changes the same visitation bit that the later fill
        // consumes. Scripted cells make an early fill observably incorrect;
        // this fixture does not claim to reproduce drawIslandBoundary itself.
        int size = m_map.m_mapWidth * m_map.m_mapHeight;
        int index = int(m_edges.size() * 3) % size;
        m_map.m_mapItems[level * size + index].m_tileData.m_zoneBoundary = 1;
    }
};
static unsigned bits(const TRmgGroundTileData& tile) { unsigned value; std::memcpy(&value, &tile, 4); return value; }
static int zoneId(const TRmgZoneCellState& state) {
    unsigned value; std::memcpy(&value, &state, 4);
    int zone = (value >> 16) & 255; return zone < 128 ? zone : zone - 256;
}
static void reference(std::vector<TRmgMapItem>& cells, int w, int h, const TRmgZone& zone) {
    int size = w * h;
    std::vector<bool> filled(size, false);
    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < size; ++i) {
            int x = i % w, y = i / w, cell = 1 + zone.m_levelPosition.m_z * size + i;
            if (filled[i] || (bits(cells[cell].m_tileData) & (1u << 28)) || zoneId(cells[cell].m_zoneState) != zone.m_slot->m_zoneIndex)
                continue;
            bool reachable = std::abs(x - zone.m_levelPosition.m_x) + std::abs(y - zone.m_levelPosition.m_y) == 1;
            for (int j = 0; !reachable && j < size; ++j)
                reachable = filled[j] && std::abs(x - j % w) + std::abs(y - j / w) == 1;
            if (reachable) { filled[i] = true; changed = true; }
        }
    }
    for (int i = 0; i < size; ++i) if (filled[i]) {
        int cell = 1 + zone.m_levelPosition.m_z * size + i;
        unsigned value = bits(cells[cell].m_tileData) | (1u << 28);
        std::memcpy(&cells[cell].m_tileData, &value, 4);
    }
}
static TPoint inset(TPoint input, TRmgMapPosition center) {
    int x = center.m_x - input.m_x, y = center.m_y - input.m_y;
    int square = x * x + y * y, length = 0;
    while ((length + 1) * (length + 1) <= square) ++length;
    if (length) {
        int displacement = length / 4;
        if (displacement < 4) displacement = 4;
        if (displacement > length / 2) displacement = length / 2;
        input.m_x += x * displacement / length;
        input.m_y += y * displacement / length;
    }
    return input;
}
template<class T> static bool check(bool caller) {
    for (int scenario = 0; scenario < 1600; ++scenario) {
        int w = 1 + scenario % 7, h = 1 + scenario / 7 % 6;
        int zones[] = {-128, -1, 0, 3, 127, 128};
        TRmgTownSlot slot; slot.m_zoneIndex = zones[scenario / 5 % 6];
        TRmgZone zone; zone.m_slot = &slot;
        zone.m_levelPosition = TRmgMapPosition(scenario / 11 % (w + 2) - 1, scenario / 13 % (h + 2) - 1, scenario % 2);
        zone.m_boundaryRoughness = scenario % 15 - 5;
        for (int i = 0; i < 1 + scenario % 6; ++i)
            zone.m_boundary.push_back(TPoint((i * 3 + scenario) % (w + 4) - 2, (i * 5 + scenario) % (h + 4) - 2));
        TRmgZone saved = zone;
        std::vector<TRmgMapItem> cells(w * h * 2 + 2);
        for (unsigned i = 0; i < cells.size(); ++i) {
            cells[i].m_guard = 0x13570000u + i;
            unsigned state = (i + scenario + 1) * 0x2345671u, data = (i + 3) * 0x1765432u;
            std::memcpy(&cells[i].m_zoneState, &state, 4);
            std::memcpy(&cells[i].m_tileData, &data, 4);
            cells[i].m_zoneState.m_zone = scenario % 3 || i % 3 ? slot.m_zoneIndex : 2;
            cells[i].m_tileData.m_zoneBoundary = scenario % 4 == 0 && i % 4 == 0;
        }
        std::vector<TRmgMapItem> expected = cells;
        if (caller) {
            for (unsigned i = 1; i <= zone.m_boundary.size(); ++i) {
                int cell = 1 + zone.m_levelPosition.m_z * w * h + int(i * 3) % (w * h);
                unsigned value = bits(expected[cell].m_tileData) | (1u << 28);
                std::memcpy(&expected[cell].m_tileData, &value, 4);
            }
        }
        reference(expected, w, h, zone);
        T candidate; candidate.m_map.m_mapWidth = w; candidate.m_map.m_mapHeight = h; candidate.m_map.m_mapItems = &cells[1];
        if (caller) candidate.insetIslandZone(&zone); else candidate.fillIslandInterior(&zone);
        if (std::memcmp(&cells[0], &expected[0], cells.size() * sizeof(cells[0]))) return false;
        if (std::memcmp(&zone.m_levelPosition, &saved.m_levelPosition, sizeof(zone.m_levelPosition)) || zone.m_slot != saved.m_slot
            || zone.m_boundaryRoughness != saved.m_boundaryRoughness || zone.m_boundary.size() != saved.m_boundary.size()) return false;
        for (unsigned i = 0; i < zone.m_boundary.size(); ++i)
            if (!(zone.m_boundary[i] == saved.m_boundary[i])) return false;
        if (caller) {
            if (candidate.m_edges.size() != zone.m_boundary.size()) return false;
            TPoint previous = inset(zone.m_boundary[0], saved.m_levelPosition);
            for (unsigned i = 0; i < candidate.m_edges.size(); ++i) {
                TPoint point = inset(zone.m_boundary[zone.m_boundary.size() - 1 - i], saved.m_levelPosition);
                Edge& e = candidate.m_edges[i];
                if (e.x != point.m_x || e.y != point.m_y || e.previousX != previous.m_x || e.previousY != previous.m_y
                    || e.zone != slot.m_zoneIndex || e.level != saved.m_levelPosition.m_z || e.roughness != zone.m_boundaryRoughness / 2) return false;
                previous = point;
            }
        } else if (!candidate.m_edges.empty()) return false;
    }
    return true;
}
// @CANDIDATES@
int main() {
    // @CHECKS@
    return 0;
}
