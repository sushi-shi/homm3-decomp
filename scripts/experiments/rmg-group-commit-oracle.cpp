// Host-only behavioral oracle. Actual value classes, map bitfields, predicates,
// coordinate constructor and accessors are inserted by the Python test. The
// virtual boundaries record calls and can append to the live object vector.
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <vector>
#include "terrain_type.h"
namespace std {
template<class T> const T& _cpp_min(const T& a, const T& b) { return b < a ? b : a; }
template<class T> const T& _cpp_max(const T& a, const T& b) { return a < b ? b : a; }
}
// @VALUE_TYPES@
static void event(std::vector<int>& events, int kind, int a, int b, int c, int d) {
    events.push_back(kind); events.push_back(a); events.push_back(b); events.push_back(c); events.push_back(d);
}
struct type_object {
    // @OBJECT_POSITION@
    int m_id, m_writes;
    std::vector<int>* m_events;
    std::vector<type_object*>* m_objects;
    type_object* m_append;
    TRmgMapPosition getPosition() const;
    virtual unsigned char isWritable() {
        event(*m_events, -2, m_id, m_position.m_x, m_position.m_y, m_position.m_z);
        ++m_writes;
        if (m_append) { m_objects->push_back(m_append); m_append = 0; }
        return m_id & 1 ? 255 : 0;
    }
};
struct TRmgMapItem {
    TRmgGroundTile m_tile;
    TRmgGroundTileData m_tileData;
    TRmgConnectionDecoration m_connection;
    // @PREDICATES@
};
struct type_random_map {
    int m_mapWidth, m_mapHeight, m_id;
    TRmgMapItem* m_mapItems;
    std::vector<int>* m_events;
    void record(int x, int y, int z) {
        if (x < 0 || x >= m_mapWidth || y < 0 || y >= m_mapHeight
            || z < 0 || z > (m_id ? 0 : 1) || m_events->size() > 10000)
            throw std::runtime_error("invalid or unbounded map traversal");
        event(*m_events, m_id, x, y, z, 0);
    }
    // @SCALAR_LOOKUP@
    TRmgMapItem* getMapItem(TRmgMapPosition point);
};
struct TRmgTreasureGroup {
    type_random_map m_map;
    std::vector<type_object*> m_objects;
    // @GROUP_POSITION@
};
// @VALUE_HELPERS@
struct CommitRoot {
    type_random_map m_map;
    TRmgTreasureGroup* m_group;
    std::vector<int> m_events;
    type_object* m_append;
    TRmgMapPosition m_expectedOffset;
    virtual void addObject(type_object* object, TRmgMapPosition position) {
        if (m_group->m_position.m_x != m_expectedOffset.m_x
            || m_group->m_position.m_y != m_expectedOffset.m_y
            || m_group->m_position.m_z != m_expectedOffset.m_z)
            throw std::runtime_error("group offset was not saved before transfer");
        event(m_events, -1, object->m_id, position.m_x, position.m_y, position.m_z);
        object->m_position = position;
        if (m_append) { m_group->m_objects.push_back(m_append); m_append = 0; }
    }
};
// @CANDIDATES@

// Independent compact state: g=1, b=2, road=4, entrance=8, connected=16.
struct Cell { int flags, terrain; };
static void initialize(TRmgMapItem& cell, const Cell& value, unsigned salt) {
    cell.m_tile.m_landType = value.terrain;
    cell.m_tile.m_terrainFrame = salt & 63;
    cell.m_tileData.m_subterraneanGate = (value.flags & 1) != 0;
    cell.m_tileData.m_borderObject = (value.flags & 2) != 0;
    cell.m_tileData.m_roadPassable = (value.flags & 4) != 0;
    cell.m_tileData.m_roadEntrance = (value.flags & 8) != 0;
    cell.m_tileData.m_placementOutline = salt & 1;
    cell.m_tileData.m_zoneBoundary = (salt >> 1) & 1;
    cell.m_connection.m_present = (value.flags & 16) != 0;
    cell.m_connection.m_direction = salt & 15;
}
static bool equal(const TRmgMapItem& actual, const Cell& value, unsigned salt) {
    const unsigned bits = actual.m_tileData.m_subterraneanGate
        | (actual.m_tileData.m_borderObject << 1) | (actual.m_tileData.m_roadPassable << 2)
        | (actual.m_tileData.m_roadEntrance << 3) | (actual.m_connection.m_present << 4);
    return bits == unsigned(value.flags) && actual.m_tile.m_landType == value.terrain
        && actual.m_tile.m_terrainFrame == int(salt & 63)
        && actual.m_tileData.m_placementOutline == (salt & 1)
        && actual.m_tileData.m_zoneBoundary == ((salt >> 1) & 1)
        && actual.m_connection.m_direction == (salt & 15);
}
template<class Candidate> static bool one(int width, int height, int groupWidth, int groupHeight,
    int offsetX, int offsetY, int level, bool alias, int sourceFlags, int destinationFlags,
    int sourceTerrain, int destinationTerrain, int seed, int appendMode) {
    Candidate actual;
    TRmgTreasureGroup group;
    actual.m_group = &group;
    actual.m_expectedOffset.m_x = offsetX; actual.m_expectedOffset.m_y = offsetY;
    actual.m_expectedOffset.m_z = level;
    TRmgMapPosition position = actual.m_expectedOffset;
    const int worldCount = width * height * 2;
    const int sourceBase = alias ? 0 : worldCount;
    const int count = worldCount + groupWidth * groupHeight + 1;
    std::vector<Cell> cells(count);
    std::vector<TRmgMapItem> storage(count);
    for (int i = 0; i < count; ++i) {
        cells[i].flags = seed < 0 ? destinationFlags : (i * 7 + seed * 11) & 31;
        cells[i].terrain = seed < 0 ? destinationTerrain : (i + seed) % 3 == 0 ? eTerrainRock : eTerrainDirt;
    }
    if (!alias) for (int i = sourceBase; i < sourceBase + groupWidth * groupHeight; ++i) {
        cells[i].flags = seed < 0 ? sourceFlags : (i * 13 + seed * 3) & 31;
        cells[i].terrain = seed < 0 ? sourceTerrain : (i + seed) % 3 == 0 ? eTerrainWater : eTerrainDirt;
    }
    for (int i = 0; i < count; ++i) initialize(storage[i], cells[i], i);
    actual.m_map.m_mapWidth = width; actual.m_map.m_mapHeight = height;
    actual.m_map.m_mapItems = &storage[0]; actual.m_map.m_id = 0; actual.m_map.m_events = &actual.m_events;
    group.m_map.m_mapWidth = groupWidth; group.m_map.m_mapHeight = groupHeight;
    group.m_map.m_mapItems = &storage[sourceBase]; group.m_map.m_id = 1; group.m_map.m_events = &actual.m_events;
    group.m_position.m_x = group.m_position.m_y = group.m_position.m_z = -99;
    type_object objects[4];
    TRmgMapPosition objectPositions[4];
    int writes[4] = {0, 0, 0, 0};
    for (int i = 0; i < 4; ++i) {
        objects[i].m_position.m_x = i - 1; objects[i].m_position.m_y = 2 - i; objects[i].m_position.m_z = 1 - (i & 1);
        objectPositions[i] = objects[i].m_position;
        objects[i].m_id = i; objects[i].m_writes = 0; objects[i].m_events = &actual.m_events;
        objects[i].m_objects = &group.m_objects; objects[i].m_append = 0;
    }
    const int initialCount = seed < 0 ? 2 : seed % 3;
    std::vector<int> order;
    for (int i = 0; i < initialCount; ++i) { order.push_back(i); group.m_objects.push_back(&objects[i]); }
    actual.m_append = (appendMode & 1) ? &objects[2] : 0;
    objects[0].m_append = (appendMode & 2) ? &objects[3] : 0;
    std::vector<int> expectedEvents;
    for (unsigned i = 0; i < order.size(); ++i) {
        const int id = order[i];
        objectPositions[id].m_x += offsetX; objectPositions[id].m_y += offsetY; objectPositions[id].m_z = level;
        event(expectedEvents, -1, id, objectPositions[id].m_x, objectPositions[id].m_y, level);
        if (i == 0 && (appendMode & 1)) order.push_back(2);
    }
    // Enumerate local cells and independently filter global coordinates.
    // This avoids copying the candidate's min/max clipping expressions.
    for (int y = 0; y < groupHeight; ++y) for (int x = 0; x < groupWidth; ++x) {
        const int dx = x + offsetX, dy = y + offsetY;
        if (dx < 0 || dy < 0 || dx >= width || dy >= height) continue;
        event(expectedEvents, 0, dx, dy, level, 0); event(expectedEvents, 1, x, y, 0, 0);
        const int si = sourceBase + y * groupWidth + x, di = (level * height + dy) * width + dx;
        const Cell source = cells[si], destination = cells[di];
        const bool passableSource = (source.flags & 13) == 4 && source.terrain != eTerrainRock;
        const bool passableDestination = (destination.flags & 12) == 4
            && destination.terrain != eTerrainWater && destination.terrain != eTerrainRock;
        if (passableSource && passableDestination && !(destination.flags & 16)) {
            cells[di].flags &= ~1;
            if (source.flags & 2) cells[di].flags |= 2;
        }
        // The old destination gate wins over its border when both are set.
        // These assignments follow the destination writes even for aliasing.
        if (!(source.flags & 16)) {
            cells[si].flags &= ~3;
            cells[si].flags |= destination.flags & 1 ? 1 : destination.flags & 2;
        }
    }
    for (unsigned i = 0; i < order.size(); ++i) {
        const int id = order[i];
        event(expectedEvents, -2, id, objectPositions[id].m_x, objectPositions[id].m_y, objectPositions[id].m_z);
        ++writes[id];
        if (i == 0 && (appendMode & 2)) order.push_back(3);
    }
    actual.commitTreasureGroup(&group, position);
    if (actual.m_events != expectedEvents || group.m_objects.size() != order.size()
        || group.m_position.m_x != offsetX || group.m_position.m_y != offsetY || group.m_position.m_z != level
        || position.m_x != offsetX || position.m_y != offsetY || position.m_z != level) return false;
    for (int i = 0; i < count; ++i) if (!equal(storage[i], cells[i], i)) return false;
    for (unsigned i = 0; i < order.size(); ++i) if (group.m_objects[i] != &objects[order[i]]) return false;
    for (int i = 0; i < 4; ++i)
        if (objects[i].m_position.m_x != objectPositions[i].m_x || objects[i].m_position.m_y != objectPositions[i].m_y
            || objects[i].m_position.m_z != objectPositions[i].m_z || objects[i].m_writes != writes[i]) return false;
    return actual.m_map.m_mapWidth == width && actual.m_map.m_mapHeight == height
        && group.m_map.m_mapWidth == groupWidth && group.m_map.m_mapHeight == groupHeight
        && actual.m_map.m_mapItems == &storage[0] && group.m_map.m_mapItems == &storage[sourceBase];
}
template<class Candidate> static bool check() {
    const int terrains[] = {eTerrainDirt, eTerrainWater, eTerrainRock};
    const int dimensions[][4] = {{0,0,0,0}, {1,0,3,2}, {0,2,3,2}, {2,2,0,2}, {3,3,2,0}, {1,1,1,1},
        {2,3,3,2}, {4,3,3,2}, {2,2,3,3}, {4,1,1,3}, {1,3,4,2}, {4,3,4,3}};
    try {
        for (int s = 0; s < 32; ++s) for (int d = 0; d < 32; ++d)
        for (int st = 0; st < 3; ++st) for (int dt = 0; dt < 3; ++dt) for (int level = 0; level < 2; ++level)
            if (!one<Candidate>(1, 1, 1, 1, 0, 0, level, false, s, d, terrains[st], terrains[dt], -1, 3)) return false;
        for (int shape = 0; shape < 12; ++shape) for (int x = -4; x <= 4; ++x) for (int y = -3; y <= 3; ++y)
        for (int level = 0; level < 2; ++level) for (int alias = 0; alias < 2; ++alias) for (int mode = 0; mode < 4; ++mode) {
            const int* dim = dimensions[shape];
            const int seed = shape + mode + 20 + x * 2 + y;
            if (!one<Candidate>(dim[0], dim[1], dim[2], dim[3], x, y, level, alias != 0,
                0, 0, 0, 0, seed, mode)) return false;
        }
    } catch (const std::exception&) { return false; }
    return true;
}
int main() {
    // @CHECKS@
    std::puts("group-commit states preserve clipped maps, flag snapshots, aliases, object positions and live virtual-call loops");
}
