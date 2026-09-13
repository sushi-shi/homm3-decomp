// Host-only caller contract, not an x86 layout model. The test inserts actual
// value types, field declarations, bitfields, byte queries and scalar lookup.
// isPlacementBlocked is a controlled boundary, never a replacement game body.
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>
#include "terrain_type.h"
// @VALUE_TYPES@
// @OBJECT_ENUM@
struct TObjectType {
    // @PROTOTYPE_POINT@
    // @PROTOTYPE_FIELDS@
};
struct TRmgObjectPropertiesRef { // @PROPERTY_FIELDS@
};
struct type_object { // @OBJECT_FIELDS@
};
struct TRmgMapItem {
    TRmgGroundTile m_tile;
    TRmgGroundTileData m_tileData;
    std::vector<type_object*> m_objects;
    // @PREDICATES@
};
static unsigned char traits[232][16];
static const unsigned char (*g_adventureObjectLandBlocked)[16] = traits;
struct type_random_map {
    int m_mapWidth, m_mapHeight;
    TRmgMapItem* m_mapItems;
    std::vector<TRmgMapItem> m_cells;
    std::vector<int> m_queries;
    int calls, zone, rejectBorder;
    unsigned char blocked;
    TRmgObjectPropertiesRef* argument;
    TRmgMapPosition placement;
    void record(int x, int y, int z) {
        if (x < 0 || x >= m_mapWidth || y < 0 || y >= m_mapHeight || z < 0 || z > 1
            || m_queries.size() >= 60) throw std::runtime_error("invalid neighbor walk");
        m_queries.push_back(x); m_queries.push_back(y); m_queries.push_back(z);
    }
    // @SCALAR_LOOKUP@
    unsigned char isPlacementBlocked(TRmgObjectPropertiesRef* properties,
        TRmgMapPosition position, int zoneIndex, unsigned char border) {
        ++calls; argument = properties; placement = position;
        zone = zoneIndex; rejectBorder = border;
        return blocked;
    }
};
// @VALUE_HELPERS@
// @DIRECTIONS@
struct FitRoot { type_random_map m_map; };
// @CANDIDATES@

static const TAdventureObjectType neighborKinds[8] = {
    ALTAR_OF_SACRIFICE, ANCHOR_POINT, ARENA, ARTIFACT,
    BLACK_BOX, BLACK_MARKET, BOAT, BORDER_TENT
};
static const TAdventureObjectType ownKinds[3] = {RESOURCE, MONSTER, BORDER_GUARD};
static bool samePosition(const TRmgMapPosition& a, const TRmgMapPosition& b) {
    return a.m_x == b.m_x && a.m_y == b.m_y && a.m_z == b.m_z;
}
static void query(std::vector<int>& trace, int direction, int x, int y) {
    // Independent ordered coordinate table, not the candidate's table lookup.
    const int dx[] = {1, 1, 0, -1, -1, -1, 0, 1};
    const int dy[] = {0, 1, 1, 1, 0, -1, -1, -1};
    trace.push_back(x + dx[direction]); trace.push_back(y + dy[direction]); trace.push_back(0);
}

template<class Candidate> static bool one(Candidate& actual, int entrances,
    int traitOne, int traitTwo, int ownTrait, int kind, unsigned char blocked,
    int roads, int rocks, int borders, int level) {
    const int x = 1 + (entrances & 1), y = 1 + ((entrances >> 1) & 1);
    TObjectType prototype, neighbors[8], opposite;
    TRmgObjectPropertiesRef properties, neighborProperties[8], oppositeProperties;
    type_object objects[8], oppositeObject;
    prototype.m_objectType = ownKinds[kind];
    prototype.m_triggerCell.m_x = (entrances % 5) - 2;
    prototype.m_triggerCell.m_y = 3 - (entrances % 7);
    properties.m_prototype = &prototype;
    opposite.m_objectType = TOWN;
    opposite.m_triggerCell.m_x = opposite.m_triggerCell.m_y = 0;
    oppositeProperties.m_prototype = &opposite; oppositeObject.m_properties = &oppositeProperties;
    std::memset(traits, 0, sizeof traits);
    traits[ownKinds[kind]][1] = ownTrait;
    // A second entrance object has the opposite trait outcome. The first
    // vector element, not the last or an aggregate, determines compatibility.
    traits[TOWN][1] = traits[TOWN][2] = ((traitOne & traitTwo) == 255) ? 0 : 255;
    type_random_map& map = actual.m_map;
    map.m_mapWidth = map.m_mapHeight = 4;
    map.m_cells.clear(); map.m_cells.resize(32); map.m_mapItems = &map.m_cells[0];
    map.m_queries.clear(); map.calls = 0; map.blocked = blocked;
    map.argument = 0; map.zone = map.rejectBorder = 77;
    TRmgMapPosition position;
    position.m_x = x + prototype.m_triggerCell.m_x;
    position.m_y = y + prototype.m_triggerCell.m_y;
    position.m_z = level;
    for (int d = 0; d < 8; ++d) {
        neighbors[d].m_objectType = neighborKinds[d];
        neighbors[d].m_triggerCell.m_x = d; neighbors[d].m_triggerCell.m_y = -d;
        neighborProperties[d].m_prototype = &neighbors[d]; objects[d].m_properties = &neighborProperties[d];
        traits[neighborKinds[d]][1] = (traitOne & (1 << d)) ? 255 : 0;
        traits[neighborKinds[d]][2] = (traitTwo & (1 << d)) ? 2 : 0;
        int at = (y + g_rmgDirections[d].m_y) * 4 + x + g_rmgDirections[d].m_x;
        TRmgMapItem& cell = map.m_cells[at];
        cell.m_tile.m_landType = (rocks & (1 << d)) ? eTerrainRock : eTerrainWater;
        cell.m_tileData.m_roadEntrance = (entrances >> d) & 1;
        cell.m_tileData.m_roadPassable = (roads >> d) & 1;
        cell.m_tileData.m_borderObject = (borders >> d) & 1;
        cell.m_tileData.m_subterraneanGate = d & 1;
        cell.m_tileData.m_zoneBoundary = (d >> 1) & 1;
        cell.m_objects.push_back(&objects[d]); cell.m_objects.push_back(&oppositeObject);
        // Neighbor lookups must remain on the surface even for level 1.
        map.m_cells[at + 16] = cell;
        map.m_cells[at + 16].m_tileData.m_roadEntrance = 1;
        map.m_cells[at + 16].m_tileData.m_roadPassable = 0;
    }
    const std::vector<TRmgMapItem> before = map.m_cells;
    unsigned char beforeTraits[232][16]; std::memcpy(beforeTraits, traits, sizeof traits);
    const TRmgMapPosition beforePosition = position;
    const TObjectType beforePrototype = prototype;
    std::vector<int> expectedQueries;
    int firstRearFailure = 8, firstFrontFailure = 5, firstOpening = 8;
    const int badRear = ownTrait ? 0 : entrances & 224;
    const int badFront = entrances & 31 & ~(traitOne & traitTwo);
    const int openings = roads & ~entrances & ~rocks & ~borders & 255;
    for (int d = 0; d < 8; ++d) {
        if (d >= 5 && firstRearFailure == 8 && (badRear & (1 << d))) firstRearFailure = d;
        if (d < 5 && firstFrontFailure == 5 && (badFront & (1 << d))) firstFrontFailure = d;
        if (firstOpening == 8 && (openings & (1 << d))) firstOpening = d;
    }
    if (!ownTrait) for (int d = 5; d < 8 && d <= firstRearFailure; ++d) query(expectedQueries, d, x, y);
    if (!badRear) for (int d = 0; d < 5 && d <= firstFrontFailure; ++d) query(expectedQueries, d, x, y);
    const bool reachesHelper = !badRear && !badFront;
    if (reachesHelper && !blocked && kind != 0)
        for (int d = 0; d < 8 && d <= firstOpening; ++d) query(expectedQueries, d, x, y);
    const unsigned char expected = reachesHelper && !blocked && (kind == 0 || openings != 0);
    if (actual.canFitObject(&properties, position) != expected || map.m_queries != expectedQueries
        || map.calls != int(reachesHelper) || !samePosition(position, beforePosition)
        || properties.m_prototype != &prototype || prototype.m_objectType != beforePrototype.m_objectType
        || prototype.m_triggerCell.m_x != beforePrototype.m_triggerCell.m_x
        || prototype.m_triggerCell.m_y != beforePrototype.m_triggerCell.m_y
        || std::memcmp(beforeTraits, traits, sizeof traits)) return false;
    if (reachesHelper && (map.argument != &properties || !samePosition(map.placement, position)
        || map.zone != -1 || map.rejectBorder != (kind == 0))) return false;
    if (map.m_mapWidth != 4 || map.m_mapHeight != 4 || map.m_mapItems != &map.m_cells[0]
        || map.m_cells.size() != before.size() || map.blocked != blocked) return false;
    for (unsigned i = 0; i < before.size(); ++i) {
        if (std::memcmp(&before[i].m_tile, &map.m_cells[i].m_tile, sizeof(TRmgGroundTile))
            || std::memcmp(&before[i].m_tileData, &map.m_cells[i].m_tileData, sizeof(TRmgGroundTileData))
            || before[i].m_objects != map.m_cells[i].m_objects) return false;
    }
    for (int d = 0; d < 8; ++d)
        if (objects[d].m_properties != &neighborProperties[d] || neighborProperties[d].m_prototype != &neighbors[d]
            || neighbors[d].m_objectType != neighborKinds[d] || neighbors[d].m_triggerCell.m_x != d
            || neighbors[d].m_triggerCell.m_y != -d) return false;
    return true;
}
template<class Candidate> static bool check() {
    Candidate actual;
    const unsigned char bytes[] = {0, 1, 255};
    try {
        for (int mask = 0; mask < 256; ++mask) for (int traits = 0; traits < 4; ++traits)
        for (int own = 0; own < 3; ++own) for (int kind = 0; kind < 3; ++kind)
        for (int blocked = 0; blocked < 3; ++blocked) for (int level = 0; level < 2; ++level)
            if (!one(actual, mask, (traits & 1) ? 255 : 0, (traits & 2) ? 255 : 0,
                bytes[own], kind, bytes[blocked], 255, 0, 0, level)) return false;
        // Isolate each front object's two traits among otherwise compatible
        // entrances. Also enumerate every open-neighbor subset and all four
        // distinct reasons for closing a neighbor, with both guard types.
        for (int mask = 0; mask < 256; ++mask) {
            for (int d = 0; d < 5; ++d) for (int trait = 0; trait < 2; ++trait)
                if (!one(actual, mask, trait ? 255 : 255 ^ (1 << d), trait ? 255 ^ (1 << d) : 255,
                    255, 0, 0, 255, 0, 0, 1)) return false;
            for (int reason = 0; reason < 4; ++reason) for (int kind = 1; kind < 3; ++kind)
            for (int level = 0; level < 2; ++level)
                if (!one(actual, reason == 0 ? 255 ^ mask : 0, 255, 255, 255, kind, 0,
                    reason == 1 ? mask : 255, reason == 2 ? 255 ^ mask : 0,
                    reason == 3 ? 255 ^ mask : 0, level)) return false;
        }
    } catch (const std::exception&) { return false; }
    return true;
}
int main() {
    // @CHECKS@
    std::puts("group-fit states preserve results, ordered surface queries, unchanged inputs and placement-helper arguments");
}
