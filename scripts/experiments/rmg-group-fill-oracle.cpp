// Native contract oracle, not a byte target. Only opaque object creation,
// placement and virtual lifecycle boundaries are mocked. The reference is a
// phase/budget state machine; it never invokes a generated fill body.
#include <vector>
#include <cstdio>
#include <climits>
#include <stdint.h>

// @VALUE_TYPES@
// @LIMITS@
struct TObjectType {
    struct TPoint { int m_x; int m_y; };
    struct TImageInfo { TPoint m_objectSize; };
    TImageInfo m_imageInfo;
    // @ACCESSORS@
};
struct TRmgObjectPropertiesRef { TObjectType* m_prototype; };
struct TRmgZone {};
struct State;
static State* g_state;
class type_object {
public:
    TRmgObjectPropertiesRef* m_properties;
    int m_id;
    virtual void unknownOperation();
    virtual ~type_object();
};
class type_random_map {
public:
    int m_mapWidth;
    int m_mapHeight;
    void addObject(type_object*, TRmgMapPosition);
};
struct TRmgTreasureGroup {
    type_random_map m_map;
    std::vector<type_object*> m_objects;
    unsigned char tryAddObject(type_object*);
    void updateBounds();
};
struct FillRoot {
    type_object* createTreasureObject(TRmgZone*, int, int, int*,
        unsigned char, unsigned char, unsigned char, TRmgMapPosition);
};
// @VALUE_HELPERS@

struct Scenario {
    int m_value, m_width, m_height, m_objectWidth, m_objectHeight;
    int m_cost[24], m_exists[24], m_fit[24];
    int m_mapAdjustment, m_fitAdjustment;
    unsigned char m_alternate;
};
struct State {
    Scenario m_scenario;
    TRmgZone m_zone;
    TRmgTreasureGroup m_group;
    TObjectType m_prototype;
    TRmgObjectPropertiesRef m_properties;
    std::vector<type_object*> m_pool;
    std::vector<int> m_events;
    int m_creation, m_fits, *m_outValue;
    bool m_bad;
    State(const Scenario& scenario) : m_scenario(scenario), m_creation(0),
        m_fits(0), m_outValue(0), m_bad(false) {
        m_group.m_map.m_mapWidth = 117;
        m_group.m_map.m_mapHeight = 239;
        m_properties.m_prototype = &m_prototype;
        m_prototype.m_imageInfo.m_objectSize.m_x = 17;
        m_prototype.m_imageInfo.m_objectSize.m_y = 29;
        // A pre-existing object is not part of this fill's ownership phase.
        m_group.m_objects.push_back(0);
    }
    ~State() {
        g_state = 0;
        for (unsigned i = 0; i < m_pool.size(); ++i)
            delete m_pool[i];
    }
};
static void event(std::vector<int>& events, int code, int a = 0, int b = 0,
                  int c = 0, int d = 0, int e = 0, int f = 0, int h = 0) {
    int data[] = {code, a, b, c, d, e, f, h};
    events.insert(events.end(), data, data + 8);
}
static int center(int dimension, int objectDimension) {
    uint32_t sum = uint32_t(dimension) + uint32_t(objectDimension);
    return int(sum / 2);
}
type_object* FillRoot::createTreasureObject(TRmgZone* zone, int minimum,
    int maximum, int* outValue, unsigned char primary, unsigned char terrain,
    unsigned char compact, TRmgMapPosition position) {
    State& state = *g_state;
    if (zone != &state.m_zone || !outValue || (state.m_outValue && state.m_outValue != outValue))
        state.m_bad = true;
    state.m_outValue = outValue;
    event(state.m_events, 1, minimum, maximum, primary, terrain, compact,
          position.m_x, position.m_y);
    event(state.m_events, 2, position.m_z);
    int index = state.m_creation++;
    if (index >= 24) return 0;
    *outValue = state.m_scenario.m_cost[index];
    state.m_group.m_map.m_mapWidth = state.m_scenario.m_width;
    state.m_group.m_map.m_mapHeight = state.m_scenario.m_height;
    state.m_prototype.m_imageInfo.m_objectSize.m_x = state.m_scenario.m_objectWidth;
    state.m_prototype.m_imageInfo.m_objectSize.m_y = state.m_scenario.m_objectHeight;
    if (!state.m_scenario.m_exists[index]) return 0;
    type_object* object = new type_object;
    object->m_properties = &state.m_properties;
    object->m_id = int(state.m_pool.size());
    state.m_pool.push_back(object);
    return object;
}
void type_random_map::addObject(type_object* object, TRmgMapPosition position) {
    State& state = *g_state;
    if (this != &state.m_group.m_map || state.m_group.m_objects.size() != 2 ||
        state.m_group.m_objects[0] != 0 || state.m_group.m_objects.back() != object)
        state.m_bad = true;
    event(state.m_events, 3, object->m_id, position.m_x, position.m_y, position.m_z);
    *state.m_outValue += state.m_scenario.m_mapAdjustment;
}
unsigned char TRmgTreasureGroup::tryAddObject(type_object* object) {
    State& state = *g_state;
    if (this != &state.m_group) state.m_bad = true;
    bool accepted = state.m_fits < 24 && state.m_scenario.m_fit[state.m_fits];
    ++state.m_fits;
    event(state.m_events, 4, object->m_id, accepted);
    *state.m_outValue += state.m_scenario.m_fitAdjustment;
    if (accepted) m_objects.push_back(object);
    return accepted;
}
void type_object::unknownOperation() { event(g_state->m_events, 5, m_id); }
type_object::~type_object() {
    if (g_state) {
        event(g_state->m_events, 6, m_id);
        g_state->m_pool[m_id] = 0;
    }
}
void TRmgTreasureGroup::updateBounds() {
    if (this != &g_state->m_group) g_state->m_bad = true;
    event(g_state->m_events, 7);
}

struct Expected {
    std::vector<int> m_events, m_objects;
    int m_total;
};
static Expected reference(const Scenario& scenario) {
    Expected result;
    result.m_total = 0;
    result.m_objects.push_back(-1);
    bool primary = true;
    int remainingCreations = 3, remainingFits = 3, remainder = scenario.m_value;
    int nextId = 0, creation = 0, fit = 0;
    // Each step consumes exactly one scripted creation. A successful primary
    // enters accumulation; a failed fit resets creation budget but consumes
    // fit budget. A successful fit resets both budgets for the next remainder.
    for (;;) {
        int maximum = primary ? scenario.m_value : 5 * remainder / 4;
        event(result.m_events, 1, remainder / 4, maximum, primary, 1,
              scenario.m_alternate, -1, -1);
        event(result.m_events, 2, -1);
        bool exists = creation < 24 && scenario.m_exists[creation];
        int cost = creation < 24 ? scenario.m_cost[creation] : 0;
        ++creation;
        if (!exists) {
            if (--remainingCreations) continue;
            if (primary) return result;
            break;
        }
        int id = nextId++;
        if (primary) {
            result.m_objects.push_back(id);
            event(result.m_events, 3, id, center(scenario.m_width, scenario.m_objectWidth),
                  center(scenario.m_height, scenario.m_objectHeight), 0);
            result.m_total = cost + scenario.m_mapAdjustment;
            primary = false;
        } else {
            bool accepted = fit < 24 && scenario.m_fit[fit];
            ++fit;
            event(result.m_events, 4, id, accepted);
            if (!accepted) {
                event(result.m_events, 5, id);
                event(result.m_events, 6, id);
                if (!--remainingFits) break;
                remainingCreations = 3;
                continue;
            }
            result.m_objects.push_back(id);
            result.m_total += cost + scenario.m_fitAdjustment;
        }
        if (result.m_total >= scenario.m_value) break;
        remainder = scenario.m_value - result.m_total;
        if (remainder < 1500 && remainder < result.m_total / 2) break;
        remainingCreations = remainingFits = 3;
    }
    event(result.m_events, 7);
    return result;
}
static Scenario scenarioFor(int seed) {
    Scenario scenario;
    const int values[] = {-17, 0, 1, 1499, 1500, 2999, 3000, 4499, 4500, 6000, 18000};
    const int dimensions[] = {0, 1, 7, 14, -3, INT_MAX, INT_MIN};
    scenario.m_value = values[seed % 11];
    scenario.m_width = dimensions[(seed / 11) % 7];
    scenario.m_height = dimensions[(seed / 3 + 2) % 7];
    scenario.m_objectWidth = dimensions[(seed / 7 + 1) % 7];
    scenario.m_objectHeight = dimensions[(seed / 5 + 3) % 7];
    scenario.m_alternate = (seed % 3 == 0) ? 0 : (seed % 3 == 1 ? 1 : 255);
    scenario.m_mapAdjustment = (seed % 5 - 2) * 101;
    scenario.m_fitAdjustment = (seed % 7 - 3) * 79;
    unsigned random = unsigned(seed + 1);
    for (int i = 0; i < 24; ++i) {
        random = random * 1664525u + 1013904223u;
        scenario.m_cost[i] = int((random >> 8) % 8) * 500 - 500;
        scenario.m_exists[i] = seed % 13 ? int((random >> 20) % 4 != 0) : 0;
        scenario.m_fit[i] = seed % 17 ? int((random >> 25) % 3 != 0) : 0;
    }
    return scenario;
}
template<class Candidate> bool check() {
    for (int seed = 0; seed < 1540; ++seed) {
        Scenario scenario = scenarioFor(seed);
        Expected expected = reference(scenario);
        State state(scenario);
        g_state = &state;
        Candidate candidate;
        int total = candidate.fillTreasureGroup(&state.m_zone, &state.m_group,
                                               scenario.m_alternate, scenario.m_value);
        std::vector<int> objects;
        for (unsigned i = 0; i < state.m_group.m_objects.size(); ++i)
            objects.push_back(state.m_group.m_objects[i] ? state.m_group.m_objects[i]->m_id : -1);
        if (state.m_bad || total != expected.m_total || state.m_events != expected.m_events ||
            objects != expected.m_objects) return false;
    }
    return true;
}
// @CANDIDATES@
int main() {
    // @CHECKS@
    return 0;
}
