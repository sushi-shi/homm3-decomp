// Host-only contract oracle. Actual value types, field declarations, byte
// accessors and source bodies are inserted by test_rmg_group_place.py.
// Opaque map helpers are controlled call boundaries, not game replacements.
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
struct type_object {
    // @OBJECT_FIELDS@
    TRmgMapPosition getPosition() const;
};
struct TRmgMapItem {
    TRmgGroundTile m_tile;
    TRmgGroundTileData m_tileData;
    std::vector<type_object*> m_objects;
    // @PREDICATES@
};
struct TRmgTownSlot { // @SLOT_FIELDS@
};
struct TRmgZone { // @ZONE_FIELDS@
};
struct Fixture;
struct type_random_map {
    int m_mapWidth, m_mapHeight;
    TRmgMapItem* m_mapItems;
    std::vector<TRmgMapItem> m_cells;
    Fixture* m_fixture;
    int m_identity;
    void record(int x, int y, int z);
    // @SCALAR_LOOKUP@
    TRmgMapItem* getMapItem(TRmgMapPosition point);
    unsigned char isPlacementBlocked(TRmgObjectPropertiesRef*, TRmgMapPosition, int, unsigned char);
    unsigned char hasConnectedOutline(const std::vector<TPoint>&, TRmgMapPosition,
        unsigned char, TRmgZone*, unsigned char);
};
struct TRmgTreasureGroup { // @GROUP_FIELDS@
};
static unsigned char g_traits[232][16];
static const unsigned char (*g_adventureObjectLandBlocked)[16] = g_traits;
// @VALUE_HELPERS@
// @DIRECTIONS@
struct PlaceRoot { type_random_map m_map; };
// @CANDIDATES@

static void event(std::vector<int>& trace, int kind, int x, int y, int z,
    int a = 0, int b = 0, int c = 0) {
    trace.push_back(kind); trace.push_back(x); trace.push_back(y); trace.push_back(z);
    trace.push_back(a); trace.push_back(b); trace.push_back(c);
}
// Flat flags do not use the game's bitfield layout or predicate helpers.
enum { ROAD = 1, GATE = 2, ENTRANCE = 4, OUTLINE = 8 };
struct Cell {
    int m_flags, m_terrain;
    bool m_monsterFirst;
    Cell() : m_flags(ROAD | GATE | OUTLINE), m_terrain(eTerrainDirt), m_monsterFirst(false) {}
};
struct Scenario {
    int m_width, m_height, m_offsetX, m_offsetY, m_level, m_water;
    int m_count, m_blockedAt, m_blockedValue, m_outlineValue, m_traitsTwo;
    bool m_restricted, m_guard, m_append, m_mutate;
    int m_guardX, m_guardY, m_left, m_top, m_right, m_bottom;
    std::vector<Cell> m_source, m_destination;
    Scenario() : m_width(11), m_height(10), m_offsetX(0), m_offsetY(0), m_level(1), m_water(0),
        m_count(2), m_blockedAt(-1), m_blockedValue(0), m_outlineValue(1), m_traitsTwo(7),
        m_restricted(false), m_guard(false), m_append(false), m_mutate(false),
        m_guardX(4), m_guardY(4), m_left(3), m_top(3), m_right(6), m_bottom(6),
        m_source(81), m_destination(220) {}
    int index(int x, int y, int z) const { return (z * m_height + y) * m_width + x; }
};
static const int g_dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
static const int g_dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const TAdventureObjectType g_kinds[3] = {RESOURCE, ARTIFACT, MINE};
static const int g_originX[3] = {2, 4, 5}, g_originY[3] = {3, 4, 5};

struct Fixture {
    TRmgTreasureGroup m_group;
    TRmgTownSlot m_slot;
    TRmgZone m_zone;
    TObjectType m_prototypes[5];
    TRmgObjectPropertiesRef m_properties[5];
    type_object m_objects[5];
    std::vector<int> m_trace;
    const Scenario* m_scenario;
    int m_blockedCalls;
    bool m_argumentsOkay;
};

void type_random_map::record(int x, int y, int z) {
    if (x < 0 || x >= m_mapWidth || y < 0 || y >= m_mapHeight || z < 0 || z > 1
        || m_fixture->m_trace.size() > 7000) throw std::runtime_error("invalid map query");
    event(m_fixture->m_trace, m_identity, x, y, z);
}
unsigned char type_random_map::isPlacementBlocked(TRmgObjectPropertiesRef* properties,
    TRmgMapPosition position, int zoneIndex, unsigned char rejectBorder) {
    Fixture& f = *m_fixture;
    int object = -1;
    for (int i = 0; i < 3; ++i) if (properties == &f.m_properties[i]) object = i;
    event(f.m_trace, 2, position.m_x, position.m_y, position.m_z, object, zoneIndex, rejectBorder);
    const int call = f.m_blockedCalls++;
    if (call == 0) {
        if (f.m_scenario->m_append) f.m_group.m_objects.push_back(&f.m_objects[2]);
        if (f.m_scenario->m_mutate) {
            f.m_slot.m_zoneIndex = 999;
            f.m_group.m_bounds.m_minimumX = f.m_group.m_bounds.m_maximumX = 8;
            f.m_group.m_bounds.m_minimumY = f.m_group.m_bounds.m_maximumY = 8;
        }
    }
    return call == f.m_scenario->m_blockedAt ? f.m_scenario->m_blockedValue : 0;
}
unsigned char type_random_map::hasConnectedOutline(const std::vector<TPoint>& outline,
    TRmgMapPosition position, unsigned char allowEntrances, TRmgZone* zone, unsigned char requireGate) {
    Fixture& f = *m_fixture;
    f.m_argumentsOkay = f.m_argumentsOkay && &outline == &f.m_group.m_outline && zone == &f.m_zone;
    event(f.m_trace, 3, position.m_x, position.m_y, position.m_z, allowEntrances,
        zone == &f.m_zone, requireGate);
    return f.m_scenario->m_outlineValue;
}

static void fill(type_random_map& map, Fixture& f, const std::vector<Cell>& cells,
    int width, int height, int identity) {
    map.m_mapWidth = width; map.m_mapHeight = height;
    map.m_cells.clear(); map.m_cells.resize(cells.size()); map.m_mapItems = &map.m_cells[0];
    map.m_fixture = &f; map.m_identity = identity;
    for (unsigned i = 0; i < cells.size(); ++i) {
        TRmgMapItem& cell = map.m_cells[i];
        std::memset(&cell.m_tile, 0, sizeof cell.m_tile);
        std::memset(&cell.m_tileData, 0, sizeof cell.m_tileData);
        cell.m_tile.m_landType = cells[i].m_terrain;
        cell.m_tileData.m_roadPassable = (cells[i].m_flags & ROAD) != 0;
        cell.m_tileData.m_subterraneanGate = (cells[i].m_flags & GATE) != 0;
        cell.m_tileData.m_roadEntrance = (cells[i].m_flags & ENTRANCE) != 0;
        cell.m_tileData.m_placementOutline = (cells[i].m_flags & OUTLINE) != 0;
        cell.m_tileData.m_borderObject = i & 1;
        cell.m_tileData.m_zoneBoundary = (i >> 1) & 1;
        const int first = cells[i].m_monsterFirst ? 3 : 4;
        cell.m_objects.push_back(&f.m_objects[first]);
        cell.m_objects.push_back(&f.m_objects[7 - first]);
    }
}
static bool unchanged(const type_random_map& map, const std::vector<TRmgMapItem>& old) {
    if (map.m_cells.size() != old.size() || map.m_mapItems != &map.m_cells[0]) return false;
    for (unsigned i = 0; i < old.size(); ++i) {
        if (std::memcmp(&map.m_cells[i].m_tile, &old[i].m_tile, sizeof(TRmgGroundTile))
            || std::memcmp(&map.m_cells[i].m_tileData, &old[i].m_tileData, sizeof(TRmgGroundTileData))
            || map.m_cells[i].m_objects != old[i].m_objects) return false;
    }
    return true;
}

static unsigned char expected(const Scenario& s, std::vector<int>& trace) {
    const int count = s.m_count + int(s.m_append);
    for (int i = 0; i < count; ++i) {
        const int object = i < s.m_count ? i : 2;
        event(trace, 2, g_originX[object] + object + s.m_offsetX,
            g_originY[object] + object + s.m_offsetY, s.m_level, object, 37, 1);
        if (i == s.m_blockedAt && s.m_blockedValue) return 0;
    }
    if (s.m_guard) {
        const int x = s.m_guardX + s.m_offsetX, y = s.m_guardY + s.m_offsetY;
        if (x <= 0 || x >= s.m_width - 1 || y <= 0 || y >= s.m_height - 1) return 0;
        // Column-major ordinal establishes query order independently of the
        // candidate's nested loop declarations.
        for (int cell = 0; cell < 9; ++cell) {
            const int column = x - 1 + cell / 3, row = y - 1 + cell % 3;
            event(trace, 1, column, row, s.m_level);
            const Cell& value = s.m_destination[s.index(column, row, s.m_level)];
            if ((value.m_flags & ENTRANCE) && value.m_monsterFirst) return 0;
        }
    }
    int usableSource = 0, usableDestination = 0, inside = 0;
    const int last = s.m_append ? 2 : s.m_count - 1;
    const int entranceX = g_originX[last], entranceY = g_originY[last];
    for (int d = 0; d < 8; ++d) {
        const Cell& source = s.m_source[(entranceY + g_dy[d]) * 9 + entranceX + g_dx[d]];
        if ((source.m_flags & 15) == (ROAD | GATE | OUTLINE) && source.m_terrain != eTerrainRock)
            usableSource |= 1 << d;
        const int x = entranceX + g_dx[d] + s.m_offsetX, y = entranceY + g_dy[d] + s.m_offsetY;
        if (x >= 0 && x < s.m_width && y >= 0 && y < s.m_height) {
            inside |= 1 << d;
            const Cell& destination = s.m_destination[s.index(x, y, s.m_level)];
            if ((destination.m_flags & (ROAD | GATE | ENTRANCE)) == (ROAD | GATE)
                && destination.m_terrain != eTerrainRock
                && (destination.m_terrain == eTerrainWater) == bool(s.m_water))
                usableDestination |= 1 << d;
        }
    }
    const int range = s.m_restricted ? 14 : 255;
    const int open = usableSource & usableDestination & range;
    int firstOpen = 8;
    for (int d = 0; d < 8; ++d) if (open & (1 << d)) { firstOpen = d; break; }
    for (int d = 0; d < 8 && d <= firstOpen; ++d) if (range & (1 << d)) {
        event(trace, 0, entranceX + g_dx[d], entranceY + g_dy[d], 0);
        if (usableSource & inside & (1 << d))
            event(trace, 1, entranceX + g_dx[d] + s.m_offsetX, entranceY + g_dy[d] + s.m_offsetY, s.m_level);
    }
    if (!open) return 0;
    int requiredTraits = (1 << s.m_count) - 1;
    if (s.m_append) requiredTraits |= 4;
    const int allow = !s.m_guard && (s.m_traitsTwo & requiredTraits) == requiredTraits;
    event(trace, 3, s.m_offsetX, s.m_offsetY, s.m_level, allow, 1, 1);
    if (!s.m_outlineValue) return 0;
    const int width = s.m_right - s.m_left, height = s.m_bottom - s.m_top;
    if (width > 0 && height > 0) for (int i = 0; i < width * height; ++i) {
        const int column = s.m_left + i % width, row = s.m_top + i / width;
        event(trace, 0, column, row, 0);
        const int x = column + s.m_offsetX, y = row + s.m_offsetY;
        if (!(s.m_source[row * 9 + column].m_flags & GATE) && x < s.m_width && y < s.m_height) {
            event(trace, 1, x, y, s.m_level);
            if (s.m_destination[s.index(x, y, s.m_level)].m_flags & ENTRANCE) return 0;
        }
    }
    return 1;
}

template<class Candidate> static bool one(const Scenario& s) {
    Candidate candidate;
    Fixture f;
    f.m_scenario = &s; f.m_blockedCalls = 0; f.m_argumentsOkay = true;
    f.m_slot.m_zoneIndex = 37; f.m_zone.m_slot = &f.m_slot;
    f.m_zone.m_terrain = s.m_water ? eTerrainWater : eTerrainDirt;
    f.m_group.m_bounds.m_minimumX = s.m_left; f.m_group.m_bounds.m_minimumY = s.m_top;
    f.m_group.m_bounds.m_maximumX = s.m_right; f.m_group.m_bounds.m_maximumY = s.m_bottom;
    f.m_group.m_hasGuard = s.m_guard ? 255 : 0;
    f.m_group.m_guardPosition = TPoint(s.m_guardX, s.m_guardY);
    f.m_group.m_outline.push_back(TPoint(3, 4)); f.m_group.m_outline.push_back(TPoint(4, 5));
    std::memset(g_traits, 0, sizeof g_traits);
    for (int i = 0; i < 5; ++i) {
        f.m_prototypes[i].m_objectType = i < 3 ? g_kinds[i] : (i == 3 ? MONSTER : TOWN);
        f.m_prototypes[i].m_triggerCell.m_x = f.m_prototypes[i].m_triggerCell.m_y = i;
        f.m_properties[i].m_prototype = &f.m_prototypes[i]; f.m_objects[i].m_properties = &f.m_properties[i];
        f.m_objects[i].m_position = TRmgMapPosition(i < 3 ? g_originX[i] + i : 4 + i,
            i < 3 ? g_originY[i] + i : 4 + i, 1 - s.m_level);
        if (i < 3) {
            g_traits[g_kinds[i]][1] = s.m_restricted ? 0 : 255;
            g_traits[g_kinds[i]][2] = (s.m_traitsTwo & (1 << i)) ? 2 : 0;
            if (i < s.m_count) f.m_group.m_objects.push_back(&f.m_objects[i]);
        }
    }
    fill(f.m_group.m_map, f, s.m_source, 9, 9, 0);
    fill(candidate.m_map, f, s.m_destination, s.m_width, s.m_height, 1);
    const std::vector<TRmgMapItem> oldSource = f.m_group.m_map.m_cells, oldDestination = candidate.m_map.m_cells;
    const std::vector<TPoint> oldOutline = f.m_group.m_outline;
    unsigned char oldTraits[232][16]; std::memcpy(oldTraits, g_traits, sizeof g_traits);
    std::vector<int> trace;
    const unsigned char result = expected(s, trace);
    try {
        if (candidate.canPlaceTreasureGroup(&f.m_group, TRmgMapPosition(s.m_offsetX, s.m_offsetY, s.m_level), &f.m_zone)
            != result || f.m_trace != trace || !f.m_argumentsOkay) return false;
    } catch (const std::exception&) { return false; }
    if (!unchanged(f.m_group.m_map, oldSource) || !unchanged(candidate.m_map, oldDestination)
        || f.m_group.m_outline != oldOutline || std::memcmp(oldTraits, g_traits, sizeof g_traits)) return false;
    const bool mutated = s.m_mutate && f.m_blockedCalls;
    if (f.m_slot.m_zoneIndex != (mutated ? 999 : 37)
        || f.m_group.m_bounds.m_minimumX != (mutated ? 8 : s.m_left)
        || f.m_group.m_bounds.m_minimumY != (mutated ? 8 : s.m_top)
        || f.m_group.m_bounds.m_maximumX != (mutated ? 8 : s.m_right)
        || f.m_group.m_bounds.m_maximumY != (mutated ? 8 : s.m_bottom)
        || f.m_group.m_objects.size() != s.m_count + unsigned(s.m_append && f.m_blockedCalls)) return false;
    for (unsigned i = 0; i < f.m_group.m_objects.size(); ++i)
        if (f.m_group.m_objects[i] != &f.m_objects[i < unsigned(s.m_count) ? i : 2]) return false;
    for (int i = 0; i < 3; ++i) {
        if (f.m_objects[i].m_properties != &f.m_properties[i] || f.m_properties[i].m_prototype != &f.m_prototypes[i]
            || f.m_objects[i].m_position.m_x != g_originX[i] + i
            || f.m_objects[i].m_position.m_y != g_originY[i] + i || f.m_objects[i].m_position.m_z != 1 - s.m_level
            || f.m_prototypes[i].m_objectType != g_kinds[i]
            || f.m_prototypes[i].m_triggerCell.m_x != i || f.m_prototypes[i].m_triggerCell.m_y != i) return false;
    }
    if (f.m_zone.m_slot != &f.m_slot || f.m_zone.m_terrain != (s.m_water ? eTerrainWater : eTerrainDirt)
        || f.m_group.m_hasGuard != (s.m_guard ? 255 : 0)
        || f.m_group.m_guardPosition != TPoint(s.m_guardX, s.m_guardY)
        || f.m_group.m_map.m_mapWidth != 9 || f.m_group.m_map.m_mapHeight != 9
        || candidate.m_map.m_mapWidth != s.m_width || candidate.m_map.m_mapHeight != s.m_height) return false;
    return true;
}

template<class Candidate> static bool check() {
    for (int level = 0; level < 2; ++level) for (int water = 0; water < 2; ++water)
    for (int restricted = 0; restricted < 2; ++restricted) for (int mask = 0; mask < 256; ++mask) {
        Scenario s; s.m_level = level; s.m_water = water; s.m_restricted = restricted;
        for (unsigned i = 0; i < s.m_destination.size(); ++i)
            s.m_destination[i].m_terrain = water ? eTerrainWater : eTerrainDirt;
        for (int d = 0; d < 8; ++d) if (!(mask & (1 << d)))
            s.m_destination[s.index(4 + g_dx[d], 4 + g_dy[d], level)].m_flags &= ~ROAD;
        if (!one<Candidate>(s)) return false;
    }
    // Each independent source/destination veto, with exactly one eligible
    // direction, proves all predicates rather than just an aggregate mask.
    for (int d = 0; d < 8; ++d) for (int reason = 0; reason < 12; ++reason) {
        Scenario s;
        for (int n = 0; n < 8; ++n) if (n != d) s.m_source[(4 + g_dy[n]) * 9 + 4 + g_dx[n]].m_flags &= ~OUTLINE;
        Cell& source = s.m_source[(4 + g_dy[d]) * 9 + 4 + g_dx[d]];
        Cell& destination = s.m_destination[s.index(4 + g_dx[d], 4 + g_dy[d], s.m_level)];
        switch (reason) {
        case 1: source.m_flags &= ~GATE; break;
        case 2: source.m_flags &= ~ROAD; break;
        case 3: source.m_terrain = eTerrainRock; break;
        case 4: source.m_flags |= ENTRANCE; break;
        case 5: source.m_flags &= ~OUTLINE; break;
        case 6: destination.m_flags &= ~GATE; break;
        case 7: destination.m_flags &= ~ROAD; break;
        case 8: destination.m_terrain = eTerrainRock; break;
        case 9: destination.m_flags |= ENTRANCE; break;
        case 10: destination.m_terrain = eTerrainWater; break;
        case 11: source.m_terrain = eTerrainWater; break;
        }
        if (!one<Candidate>(s)) return false;
    }
    for (int guard = 0; guard < 2; ++guard) for (int bits = 0; bits < 8; ++bits)
    for (int control = 0; control < 8; ++control) {
        Scenario s; s.m_guard = guard; s.m_traitsTwo = bits;
        s.m_append = (control & 1) != 0; s.m_mutate = (control & 2) != 0;
        s.m_outlineValue = (control & 4) ? 255 : 0;
        s.m_source[3 * 9 + 3].m_flags &= ~GATE;
        if (!one<Candidate>(s)) return false;
    }
    for (int blocked = 0; blocked < 3; ++blocked) for (int value = 0; value < 3; ++value) {
        Scenario s; s.m_append = true; s.m_blockedAt = blocked;
        s.m_blockedValue = value == 2 ? 255 : value; s.m_mutate = true;
        if (!one<Candidate>(s)) return false;
    }
    for (int cell = 0; cell < 9; ++cell) for (int first = 0; first < 2; ++first)
    for (int level = 0; level < 2; ++level) {
        Scenario s; s.m_guard = true; s.m_level = level;
        Cell& destination = s.m_destination[s.index(3 + cell / 3, 3 + cell % 3, level)];
        destination.m_flags |= ENTRANCE; destination.m_monsterFirst = first;
        if (!one<Candidate>(s)) return false;
    }
    for (int x = 0; x < 11; ++x) for (int y = 0; y < 10; ++y) {
        Scenario s; s.m_guard = true; s.m_guardX = x; s.m_guardY = y;
        if (!one<Candidate>(s)) return false;
    }
    for (int cell = 0; cell < 9; ++cell) for (int gate = 0; gate < 2; ++gate) {
        Scenario s; const int x = 3 + cell % 3, y = 3 + cell / 3;
        if (!gate) s.m_source[y * 9 + x].m_flags &= ~GATE;
        s.m_destination[s.index(x, y, s.m_level)].m_flags |= ENTRANCE;
        if (!one<Candidate>(s)) return false;
    }
    for (int x = -5; x <= 8; ++x) for (int y = -5; y <= 8; ++y) {
        Scenario s; s.m_offsetX = x; s.m_offsetY = y;
        // Occupied bounds use only upper clipping in retail. Keep lower
        // translated cells valid; an empty range is also a real input.
        s.m_left = x < 0 ? -x : 0; s.m_top = y < 0 ? -y : 0;
        s.m_right = s.m_bottom = 8;
        for (unsigned i = 0; i < s.m_source.size(); ++i) if (i % 3 == 0) s.m_source[i].m_flags &= ~GATE;
        if (!one<Candidate>(s)) return false;
    }
    Scenario empty; empty.m_left = empty.m_right; empty.m_top = empty.m_bottom;
    if (!one<Candidate>(empty)) return false;
    for (int append = 0; append < 2; ++append) for (int restricted = 0; restricted < 2; ++restricted) {
        Scenario single; single.m_count = 1; single.m_append = append; single.m_restricted = restricted;
        if (!one<Candidate>(single)) return false;
    }
    return true;
}

int main() {
    // @CHECKS@
    return 0;
}
