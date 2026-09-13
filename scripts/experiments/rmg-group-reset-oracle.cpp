// Independent reset contract: empty borrowed-pointer containers, clear every
// allocated cell, then repaint only the surface plane. Compile actual helper
// definitions and packed field types; expected words come from the retail masks.
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>
#include "terrain_type.h"

// @VALUE_TYPES@

static int g_objectDestructions;
struct type_object {
    int m_identity;
    ~type_object() { ++g_objectDestructions; }
};
struct Event {
    int m_kind, m_index, m_a, m_b, m_c, m_d;
    Event(int kind, int index, int a = 0, int b = 0, int c = 0, int d = 0)
        : m_kind(kind), m_index(index), m_a(a), m_b(b), m_c(c), m_d(d) {}
    bool operator==(const Event& other) const {
        return m_kind == other.m_kind && m_index == other.m_index
            && m_a == other.m_a && m_b == other.m_b && m_c == other.m_c && m_d == other.m_d;
    }
};
struct TRmgMapItem {
    // @CELL_FIELDS@
    void clear();
    void setTerrain(int, int, unsigned char, unsigned char);
};
static std::vector<Event> g_events;
static TRmgMapItem* g_cells;
static std::vector<type_object*>* g_objects;
static std::vector<TPoint>* g_outline;
static unsigned char* g_hasGuard;
static unsigned char* g_ready;

static void recordClear(TRmgMapItem* item) {
    g_events.push_back(Event(1, item - g_cells));
}
static void recordTerrain(TRmgMapItem* item, int terrain, int frame, int flipX, int flipY) {
    g_events.push_back(Event(3, item - g_cells, terrain, frame, flipX, flipY));
}
struct type_random_map {
    // @MAP_FIELDS@
    void clear();
    TRmgMapItem* getMapItem(int, int);
    // Wrong-overload control remains observable even though z=0 returns the
    // same cell. This is the independently proven ret-8 call-site contract.
    TRmgMapItem* getMapItem(int x, int y, int z) {
        g_events.push_back(Event(4, 0, x, y, z));
        return m_mapItems + (z * m_mapHeight + y) * m_mapWidth + x;
    }
};
struct ResetRoot {
    // @GROUP_FIELDS@
};

// @HELPERS@
// @CANDIDATES@

template<class T> static unsigned word(const T& value) {
    unsigned result = 0;
    if (sizeof(value) != sizeof(result)) return 0xdeadbeefU;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}
template<class T> static void putWord(T& value, unsigned bits) {
    if (sizeof(value) == sizeof(bits)) std::memcpy(&value, &bits, sizeof(bits));
}
static unsigned seed(unsigned index, unsigned salt) {
    return (index * 0x9e3779b9U) ^ (salt * 0x85ebca6bU) ^ 0xa59678e1U;
}
struct Snapshot {
    unsigned m_tile, m_data, m_connection, m_movement, m_zone;
    int m_x, m_y, m_z;
    std::vector<type_object*> m_objects;
    explicit Snapshot(const TRmgMapItem& item)
        : m_tile(word(item.m_tile)), m_data(word(item.m_tileData)),
          m_connection(word(item.m_connection)), m_movement(word(item.m_movement)),
          m_zone(word(item.m_zoneState)), m_x(item.m_previousTile.m_x),
          m_y(item.m_previousTile.m_y), m_z(item.m_previousTile.m_z), m_objects(item.m_objects) {}
    bool same(const TRmgMapItem& item) const {
        return m_tile == word(item.m_tile) && m_data == word(item.m_tileData)
            && m_connection == word(item.m_connection) && m_movement == word(item.m_movement)
            && m_zone == word(item.m_zoneState) && m_x == item.m_previousTile.m_x
            && m_y == item.m_previousTile.m_y && m_z == item.m_previousTile.m_z
            && m_objects == item.m_objects;
    }
};

template<class Candidate> static bool checkCase(int width, int height, int levels,
                                               unsigned salt, unsigned occupancy, unsigned char flag) {
    const int plane = width * height, total = plane * levels;
    std::vector<TRmgMapItem> cells(total + 2);
    type_object objects[3];
    for (int object = 0; object != 3; ++object) objects[object].m_identity = object + 19;
    Candidate group;
    group.m_map.m_mapItems = &cells[1];
    group.m_map.m_mapWidth = width;
    group.m_map.m_mapHeight = height;
    group.m_map.m_numberLevels = levels;
    group.m_map.m_ownsMapItems = 0;
    group.m_bounds.m_minimumX = -8; group.m_bounds.m_minimumY = 3;
    group.m_bounds.m_maximumX = 13; group.m_bounds.m_maximumY = 24;
    group.m_guardPosition.m_x = 7; group.m_guardPosition.m_y = -5;
    group.m_position.m_x = 101; group.m_position.m_y = 103; group.m_position.m_z = 2;
    group.m_hasGuard = flag; group.m_ready = flag ^ 0x5a;
    for (unsigned i = 0; i != occupancy; ++i) {
        group.m_objects.push_back(&objects[i % 3]);
        group.m_outline.push_back(TPoint(3 * i, 5 * i));
    }
    group.m_objects.reserve(11); group.m_outline.reserve(13);
    const size_t objectCapacity = group.m_objects.capacity(), outlineCapacity = group.m_outline.capacity();
    std::vector<Snapshot> before;
    std::vector<size_t> capacities;
    for (int i = 0; i != total + 2; ++i) {
        TRmgMapItem& item = cells[i];
        putWord(item.m_tile, seed(i, salt));
        putWord(item.m_tileData, seed(i + 11, salt));
        putWord(item.m_connection, seed(i + 23, salt));
        putWord(item.m_movement, seed(i + 37, salt));
        putWord(item.m_zoneState, seed(i + 41, salt));
        item.m_previousTile.m_x = i + 7; item.m_previousTile.m_y = -i - 9; item.m_previousTile.m_z = i % 3;
        for (unsigned j = 0; j != (occupancy + i) % 4; ++j) item.m_objects.push_back(&objects[j % 3]);
        item.m_objects.reserve(7);
        before.push_back(Snapshot(item));
        capacities.push_back(item.m_objects.capacity());
    }
    g_cells = &cells[1]; g_objects = &group.m_objects; g_outline = &group.m_outline;
    g_hasGuard = &group.m_hasGuard; g_ready = &group.m_ready;
    g_events.clear();
    const int destructions = g_objectDestructions;
    group.reset();
    if (g_objectDestructions != destructions || !group.m_objects.empty() || !group.m_outline.empty()
        || group.m_objects.capacity() != objectCapacity || group.m_outline.capacity() != outlineCapacity
        || group.m_hasGuard || group.m_ready || group.m_map.m_mapItems != &cells[1]
        || group.m_map.m_mapWidth != width || group.m_map.m_mapHeight != height
        || group.m_map.m_numberLevels != levels || group.m_map.m_ownsMapItems != 0
        || group.m_bounds.m_minimumX != -8 || group.m_bounds.m_minimumY != 3
        || group.m_bounds.m_maximumX != 13 || group.m_bounds.m_maximumY != 24
        || group.m_guardPosition.m_x != 7 || group.m_guardPosition.m_y != -5
        || group.m_position.m_x != 101 || group.m_position.m_y != 103 || group.m_position.m_z != 2)
        return false;
    if (!before.front().same(cells.front()) || !before.back().same(cells.back())) return false;
    std::vector<Event> expected;
    expected.push_back(Event(0, 0, 0, 0, flag, flag ^ 0x5a));
    for (int i = 0; i != total; ++i) expected.push_back(Event(1, i));
    expected.push_back(Event(2, 0, 0, 0));
    for (int i = 0; i != plane; ++i) expected.push_back(Event(3, i, eTerrainDirt, 0, 0, 0));
    if (g_events != expected) return false;
    for (int i = 0; i != total; ++i) {
        const Snapshot& old = before[i + 1];
        const TRmgMapItem& item = cells[i + 1];
        // Retail clear preserves tile bits 30/31 and connection-visited bit
        // 24; it sets road-passable/gate and clears connection presence only.
        const unsigned terrain = i < plane ? 0U : (21U << 6) | 8U;
        if (word(item.m_tile) != ((old.m_tile & 0xc0000000U) | terrain)
            || word(item.m_tileData) != ((old.m_data & 0x01000000U) | 0x0a000000U)
            || word(item.m_connection) != (old.m_connection & 0xfffffffeU)
            || word(item.m_movement) != 0x7fbc7fbcU || word(item.m_zoneState) != 0xffff7fbcU
            || item.m_previousTile.m_x != -1 || item.m_previousTile.m_y != old.m_y
            || item.m_previousTile.m_z != old.m_z || !item.m_objects.empty()
            || item.m_objects.capacity() != capacities[i + 1]) return false;
    }
    return true;
}

template<class Candidate> static bool check() {
    static const int shapes[][3] = {{0, 0, 1}, {0, 4, 2}, {5, 0, 1}, {1, 1, 1},
        {1, 5, 2}, {5, 1, 3}, {3, 4, 1}, {4, 3, 2}, {7, 5, 3}, {16, 16, 1}};
    static const unsigned char flags[] = {0, 1, 255};
    for (unsigned shape = 0; shape != sizeof(shapes) / sizeof(shapes[0]); ++shape)
        for (unsigned salt = 0; salt != 5; ++salt)
            for (unsigned occupancy = 0; occupancy != 4; ++occupancy)
                for (unsigned flag = 0; flag != sizeof(flags); ++flag)
                    if (!checkCase<Candidate>(shapes[shape][0], shapes[shape][1], shapes[shape][2],
                                              salt, occupancy, flags[flag])) return false;
    return true;
}

int main() {
    // @CHECKS@
    return 0;
}
