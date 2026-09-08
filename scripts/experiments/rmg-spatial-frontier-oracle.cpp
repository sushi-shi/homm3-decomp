// Host-only controls. Actual value types, helpers and candidate methods are
// inserted by test_rmg_spatial_frontiers.py; this file never enters VC6 input.
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
using std::sqrt;
namespace std {
template<class T> const T& _cpp_min(const T& a, const T& b) { return b < a ? b : a; }
template<class T> const T& _cpp_max(const T& a, const T& b) { return a < b ? b : a; }
}

// @VALUE_TYPES@

struct TRmgTownSlot { int m_zoneIndex, m_size; };
struct TRmgZone {
    TRmgMapPosition m_levelPosition;
    TRmgTownSlot* m_slot;
    int m_boundaryRoughness;
    TRmgMapPosition getLevelPosition() const;
};
struct TRmgMapItem {
    struct ZoneState { int m_zone; } m_zoneState;
    struct Connection { int m_present; } m_connection;
    struct TileData { int m_zoneBoundary, m_borderObject, m_subterraneanGate; } m_tileData;
    bool operator==(const TRmgMapItem& other) const {
        return m_zoneState.m_zone == other.m_zoneState.m_zone
            && m_connection.m_present == other.m_connection.m_present
            && m_tileData.m_zoneBoundary == other.m_tileData.m_zoneBoundary
            && m_tileData.m_borderObject == other.m_tileData.m_borderObject
            && m_tileData.m_subterraneanGate == other.m_tileData.m_subterraneanGate;
    }
};
struct type_random_map {
    int m_mapWidth, m_mapHeight, m_numberLevels;
    TRmgMapItem* m_mapItems;
    std::vector<TRmgMapItem> m_cells;
    std::vector<int> m_queries;
    void record(int x, int y, int z) {
        assert(x >= 0 && x < m_mapWidth && y >= 0 && y < m_mapHeight);
        assert(z >= 0 && z < m_numberLevels && m_queries.size() < 30000);
        m_queries.push_back(x); m_queries.push_back(y); m_queries.push_back(z);
    }
    // @SCALAR_LOOKUP@
    TRmgMapItem* getMapItem(TRmgMapPosition point);
};

// @VALUE_HELPERS@

struct SpatialRoot {
    type_random_map m_map;
    std::vector<TRmgZone*> m_zones;
};
static unsigned int g_spatialRandom;
static unsigned int g_spatialDraws;
static int spatialRandom() {
    assert(++g_spatialDraws < 10000);
    g_spatialRandom = g_spatialRandom * 214013u + 2531011u;
    return (g_spatialRandom >> 16) & 0x7fff;
}
#define rand spatialRandom

// @CANDIDATES@

struct ZoneFixture {
    TRmgTownSlot m_slots[4];
    TRmgZone m_zones[4];
    int m_outputs[4];
};
static void initializeZones(SpatialRoot& root, ZoneFixture& fixture, int count, int seed) {
    for (int i = 0; i < 4; ++i) {
        fixture.m_outputs[i] = 700 + i;
        fixture.m_slots[i].m_zoneIndex = i;
        fixture.m_slots[i].m_size = 1 + (seed + i * 7) % 23;
        fixture.m_zones[i].m_slot = &fixture.m_slots[i];
        fixture.m_zones[i].m_levelPosition = TRmgMapPosition(
            (seed * 19 + i * 13) % 61 - 30, (seed * 7 + i * 23) % 61 - 30, i & 1);
        fixture.m_zones[i].m_boundaryRoughness = 3;
        if (i < count) root.m_zones.push_back(&fixture.m_zones[i]);
    }
}
static int& output(ZoneFixture& fixture, int index) {
    if (index < 4) return fixture.m_outputs[index];
    if (index == 4) return fixture.m_zones[0].m_levelPosition.m_x;
    if (index == 5) return fixture.m_zones[0].m_levelPosition.m_y;
    return fixture.m_slots[0].m_size;
}
template<class Candidate> static bool checkBounds() {
    for (int count = 0; count <= 4; count += 2) {
        for (int seed = 0; seed < 3; ++seed) {
            for (int alias = 0; alias < 300; ++alias) {
                Baseline expected; Candidate actual;
                ZoneFixture a, b;
                initializeZones(expected, a, count, seed);
                initializeZones(actual, b, count, seed);
                // Exhaust all 4^4 output-reference aliases, then include aliases
                // to input fields that later loop iterations can observe.
                int value = alias < 256 ? alias : (alias - 256) * 53;
                int radix = alias < 256 ? 4 : 7;
                int indices[4];
                for (int i = 0; i < 4; ++i) { indices[i] = value % radix; value /= radix; }
                expected.getInitialZoneBounds(output(a, indices[0]), output(a, indices[1]),
                    output(a, indices[2]), output(a, indices[3]));
                actual.getInitialZoneBounds(output(b, indices[0]), output(b, indices[1]),
                    output(b, indices[2]), output(b, indices[3]));
                for (int i = 0; i < 7; ++i) if (output(a, i) != output(b, i)) return false;
            }
        }
    }
    return true;
}
static void initializeMap(type_random_map& map, int width, int height, int seed) {
    map.m_mapWidth = width; map.m_mapHeight = height; map.m_numberLevels = 2;
    map.m_cells.resize(width * height * 2);
    map.m_mapItems = &map.m_cells[0];
    map.m_queries.clear();
    for (unsigned int i = 0; i < map.m_cells.size(); ++i) {
        TRmgMapItem& cell = map.m_cells[i];
        cell.m_zoneState.m_zone = (i + seed) % 3;
        cell.m_connection.m_present = (i + seed) % 5 == 0;
        cell.m_tileData.m_zoneBoundary = (i + seed) % 7 == 0;
        cell.m_tileData.m_borderObject = (i + seed) % 3 != 0;
        cell.m_tileData.m_subterraneanGate = (i + seed) % 5 != 0;
    }
}
template<class Candidate> static bool checkEdges(bool junction) {
    const int widths[] = {1, 2, 5, 8};
    const int heights[] = {1, 3, 4, 7};
    for (int shape = 0; shape < 4; ++shape) {
        int width = widths[shape], height = heights[shape];
        for (int scenario = 0; scenario < 24; ++scenario) {
            Baseline expected; Candidate actual;
            initializeMap(expected.m_map, width, height, scenario);
            initializeMap(actual.m_map, width, height, scenario);
            TPoint from((scenario * 7) % (width + 4) - 2, (scenario * 5) % (height + 4) - 2);
            TPoint to((scenario * 11 + 2) % (width + 4) - 2, (scenario * 13 + 1) % (height + 4) - 2);
            if (scenario % 4 == 0) to = from;
            else if (scenario % 4 == 1) to.m_x = from.m_x;
            else if (scenario % 4 == 2) to.m_y = from.m_y;
            int level = scenario & 1, zoneIndex = scenario % 3, roughness = 1 + scenario % 4;
            TRmgTownSlot slot; slot.m_zoneIndex = zoneIndex; slot.m_size = 1;
            TRmgZone zone; zone.m_slot = &slot; zone.m_levelPosition = TRmgMapPosition(0, 0, level);
            zone.m_boundaryRoughness = roughness;
            g_spatialRandom = scenario + 1; g_spatialDraws = 0;
            if (junction) expected.connectJunctionEntrance(from, to, &zone);
            else expected.drawIslandBoundary(from, to, zoneIndex, level, roughness);
            unsigned int random = g_spatialRandom, draws = g_spatialDraws;
            g_spatialRandom = scenario + 1; g_spatialDraws = 0;
            if (junction) actual.connectJunctionEntrance(from, to, &zone);
            else actual.drawIslandBoundary(from, to, zoneIndex, level, roughness);
            if (random != g_spatialRandom || draws != g_spatialDraws
                || expected.m_map.m_queries != actual.m_map.m_queries
                || expected.m_map.m_cells != actual.m_map.m_cells) return false;
        }
    }
    return true;
}
template<class Candidate> static bool check() {
    return checkBounds<Candidate>() && checkEdges<Candidate>(false) && checkEdges<Candidate>(true);
}
int main() {
    // @CHECKS@
    std::puts("spatial source states preserve aliased bounds, ordered lookups, cell flags and RNG draws");
}
