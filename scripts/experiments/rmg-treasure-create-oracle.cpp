// Native behavioral oracle, not the VC6 byte verdict. Footprints are valid
// 1..8 by 1..6 with at least one occupied cell; densities are positive and
// scaled values do not overflow. Invalid bit indices/exception ABI are outside
// this host-STL check. Only virtual methods, selector, placement and RNG mock.
#include <vector>
#include <bitset>
#include <cstdio>
#include <stdint.h>
// @VALUE_TYPES@
// @VALUE_HELPERS@
struct CObjectType {
    // @BIT_POSITION@
};
struct TObjectType {
    struct ImageInfo { TPoint m_objectSize; } m_imageInfo;
    std::bitset<48> m_passableMask, m_triggerMask;
    // @ACCESSORS@
};
struct TRmgObjectPropertiesRef { TObjectType* m_prototype; int m_id; };
struct Slot { int m_zoneIndex; };
struct TRmgZone { Slot* m_slot; int m_terrain; int m_objectCountByType[232]; };
struct type_object { int m_id; };
struct CreateRoot;
struct type_treasure_def {
    int m_objectType, m_subtype, m_density, m_id, m_calls;
    virtual unsigned char isTerrainDependent();
    virtual int getValue(TRmgZone*, CreateRoot*);
    virtual type_object* generate(TRmgObjectPropertiesRef*, CreateRoot*, TRmgZone*);
};
struct Map {
    unsigned char isPlacementBlocked(TRmgObjectPropertiesRef*, TRmgMapPosition, int, unsigned char);
};
struct CreateRoot {
    std::vector<type_treasure_def*> m_objectGenerators;
    int m_objectCountByType[232];
    Map m_map;
    TRmgObjectPropertiesRef* selectObjectPrototype(int, int, int);
};
static int g_adventureObjectLandBlocked[232][16];
static int g_rmgMapObjectLimits[232], g_rmgZoneObjectLimits[232];
struct Item {
    int m_value, m_density, m_terrain, m_land, m_water, m_mapCount, m_zoneCount;
    int m_exists, m_blocked, m_factory, m_width, m_height;
    uint64_t m_passable, m_trigger;
};
struct Scenario {
    Item m_items[8];
    int m_count, m_minimum, m_maximum, m_random;
    unsigned char m_primary, m_terrain, m_compact;
    TRmgMapPosition m_position;
};
struct State {
    Scenario m_scenario;
    TRmgZone m_zone;
    Slot m_slot;
    TObjectType m_prototypes[8];
    TRmgObjectPropertiesRef m_properties[8];
    type_treasure_def m_definitions[8];
    type_object m_objects[8];
    CreateRoot* m_root;
    std::vector<int> m_events;
    int m_value;
    bool m_bad;
    State(const Scenario& s, CreateRoot* root) : m_scenario(s), m_root(root), m_value(-765), m_bad(false) {
        m_slot.m_zoneIndex = 17; m_zone.m_slot = &m_slot; m_zone.m_terrain = 6;
        for (int i = 0; i < s.m_count; ++i) {
            const Item& item = s.m_items[i];
            m_definitions[i].m_objectType = i; m_definitions[i].m_subtype = 31 + i;
            m_definitions[i].m_density = item.m_density;
            m_definitions[i].m_id = i; m_definitions[i].m_calls = 0;
            root->m_objectGenerators.push_back(&m_definitions[i]);
            root->m_objectCountByType[i] = item.m_mapCount;
            m_zone.m_objectCountByType[i] = item.m_zoneCount;
            g_rmgMapObjectLimits[i] = 2; g_rmgZoneObjectLimits[i] = 2;
            g_adventureObjectLandBlocked[i][0] = item.m_land;
            g_adventureObjectLandBlocked[i][2] = item.m_water;
            m_prototypes[i].m_imageInfo.m_objectSize.m_x = item.m_width;
            m_prototypes[i].m_imageInfo.m_objectSize.m_y = item.m_height;
            for (int bit = 0; bit < 48; ++bit) {
                m_prototypes[i].m_passableMask[bit] = (item.m_passable >> bit) & 1;
                m_prototypes[i].m_triggerMask[bit] = (item.m_trigger >> bit) & 1;
            }
            m_properties[i].m_prototype = &m_prototypes[i]; m_properties[i].m_id = i;
            m_objects[i].m_id = i;
        }
    }
};
static State* g_state;
static void event(std::vector<int>& events, int code, int id, int a = 0, int b = 0, int c = 0) {
    int fields[] = {code, id, a, b, c};
    events.insert(events.end(), fields, fields + 5);
}
unsigned char type_treasure_def::isTerrainDependent() {
    event(g_state->m_events, 1, m_id);
    return g_state->m_scenario.m_items[m_id].m_terrain;
}
int type_treasure_def::getValue(TRmgZone* zone, CreateRoot* root) {
    if (zone != &g_state->m_zone || root != g_state->m_root) g_state->m_bad = true;
    event(g_state->m_events, 2, m_id, m_calls);
    return g_state->m_scenario.m_items[m_id].m_value + 137 * m_calls++;
}
type_object* type_treasure_def::generate(TRmgObjectPropertiesRef* properties, CreateRoot* root, TRmgZone* zone) {
    if (properties != &g_state->m_properties[m_id] || root != g_state->m_root || zone != &g_state->m_zone)
        g_state->m_bad = true;
    event(g_state->m_events, 5, m_id, g_state->m_value);
    return g_state->m_scenario.m_items[m_id].m_factory ? &g_state->m_objects[m_id] : 0;
}
TRmgObjectPropertiesRef* CreateRoot::selectObjectPrototype(int terrain, int type, int subtype) {
    if (this != g_state->m_root || terrain != 6 || subtype != 31 + type || type < 0 || type >= 8)
        g_state->m_bad = true;
    event(g_state->m_events, 3, type, terrain, subtype);
    return g_state->m_scenario.m_items[type].m_exists ? &g_state->m_properties[type] : 0;
}
unsigned char Map::isPlacementBlocked(TRmgObjectPropertiesRef* p, TRmgMapPosition pos, int zone, unsigned char flag) {
    const TRmgMapPosition& expected = g_state->m_scenario.m_position;
    if (this != &g_state->m_root->m_map || zone != 17 || flag != 1 ||
        pos.m_x != expected.m_x || pos.m_y != expected.m_y || pos.m_z != expected.m_z)
        g_state->m_bad = true;
    event(g_state->m_events, 4, p->m_id, zone, flag);
    return g_state->m_scenario.m_items[p->m_id].m_blocked;
}
static int controlledRandom() {
    event(g_state->m_events, 6, g_state->m_scenario.m_random);
    return g_state->m_scenario.m_random;
}
#define rand controlledRandom
// @CANDIDATES@
#undef rand

static int reference(const Scenario& s, std::vector<int>& trace, int& value) {
    std::vector<int> admitted;
    int best = 0;
    for (int i = 0; i < s.m_count; ++i) {
        const Item& item = s.m_items[i];
        if (!s.m_primary && item.m_land && !item.m_water) continue;
        if (!s.m_terrain) {
            event(trace, 1, i);
            if (item.m_terrain) continue;
        }
        if (item.m_mapCount >= 2 || item.m_zoneCount >= 2) continue;
        event(trace, 2, i, 0);
        if (item.m_value < 0 || item.m_value < s.m_minimum || item.m_value > s.m_maximum) continue;
        event(trace, 3, i, 6, 31 + i);
        if (!item.m_exists) continue;
        if (s.m_position.m_x >= 0) {
            event(trace, 4, i, 17, 1);
            if (item.m_blocked) continue;
        }
        if (s.m_compact) {
            int cells = 0;
            // Row-major flat mask reference, independent of nested candidate loops.
            uint64_t occupied = ~item.m_passable | item.m_trigger;
            for (int bit = 0; bit < 48; ++bit) {
                int cell = 47 - bit;
                if (cell % 8 < item.m_width && cell / 8 < item.m_height && ((occupied >> bit) & 1)) ++cells;
            }
            int score = item.m_value / cells;
            if (score < best * 3 / 4) continue;
            if (best < score * 3 / 4) { admitted.clear(); best = score; }
        }
        admitted.push_back(i);
    }
    if (admitted.empty()) return -1;
    int weight = 0;
    for (unsigned i = 0; i < admitted.size(); ++i) weight += s.m_items[admitted[i]].m_density;
    event(trace, 6, s.m_random);
    int draw = s.m_random % weight, upper = 0, chosen = -1;
    for (unsigned i = 0; i < admitted.size(); ++i) {
        upper += s.m_items[admitted[i]].m_density;
        if (draw < upper) { chosen = admitted[i]; break; }
    }
    event(trace, 2, chosen, 1);
    value = s.m_items[chosen].m_value + 137;
    event(trace, 5, chosen, value);
    return s.m_items[chosen].m_factory ? chosen : -1;
}
static uint32_t g_seed = 89123;
static unsigned next() { g_seed = g_seed * 1664525u + 1013904223u; return g_seed >> 8; }
static Scenario scenario(int n) {
    Scenario s;
    s.m_count = n % 9;
    s.m_primary = n & 1; s.m_terrain = (n >> 1) & 1; s.m_compact = (n >> 2) & 1;
    s.m_minimum = n % 5 ? 0 : 50; s.m_maximum = n % 7 ? 1000 : 100;
    s.m_random = next() % 32768;
    s.m_position.m_x = n % 3 - 1; s.m_position.m_y = n % 5 - 2; s.m_position.m_z = n % 2 - 1;
    for (int i = 0; i < 8; ++i) {
        Item& a = s.m_items[i];
        a.m_value = int(next() % 1200) - 20; a.m_density = 1 + next() % 9;
        a.m_terrain = next() % 3 == 0; a.m_land = next() & 1; a.m_water = next() & 1;
        a.m_mapCount = next() % 3; a.m_zoneCount = next() % 3;
        a.m_exists = next() % 5 != 0; a.m_blocked = next() % 4 == 0; a.m_factory = next() % 5 != 0;
        a.m_width = 1 + next() % 8; a.m_height = 1 + next() % 6;
        a.m_passable = (uint64_t(next()) << 24) | next();
        a.m_passable &= ~(uint64_t(1) << 47);
        a.m_trigger = (uint64_t(next()) << 24) | next() | (uint64_t(1) << 47);
        if (n < 256) {
            a.m_value = (i % 4 + 1) * 40; a.m_mapCount = a.m_zoneCount = 0;
            a.m_exists = a.m_factory = 1; a.m_land = a.m_terrain = a.m_blocked = 0;
            a.m_width = a.m_height = 1;
            if (n < 64) a.m_value = i % 3 == 0 ? s.m_minimum : i % 3 == 1 ? s.m_maximum : 0;
        }
    }
    return s;
}
template<class Candidate> bool check() {
    g_seed = 89123;
    for (int n = 0; n < 2304; ++n) {
        Scenario s = scenario(n);
        Candidate root;
        State state(s, &root); g_state = &state;
        std::vector<int> expected;
        int expectedValue = -765;
        int expectedId = reference(s, expected, expectedValue);
        type_object* got = root.createTreasureObject(&state.m_zone, s.m_minimum, s.m_maximum, &state.m_value,
            s.m_primary, s.m_terrain, s.m_compact, s.m_position);
        if (state.m_bad || (got ? got->m_id : -1) != expectedId || state.m_value != expectedValue || state.m_events != expected)
            return false;
    }
    return true;
}
int main() {
    // @CHECKS@
    return 0;
}
