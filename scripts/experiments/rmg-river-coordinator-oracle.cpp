// Behavioral reference, not a byte target. The coordinate types and their
// operations come from game source; preparation, routing and progress are
// opaque scripted boundaries. All coordinate arithmetic stays in int range.
#include <vector>
#include <cstdio>
// @VALUE_TYPES@
// @VALUE_HELPERS@
enum { WATER_WHEEL = 109 };
struct TObjectType {
    // @PROTOTYPE_POINT@
    int m_objectType;
    TPoint m_triggerCell;
};
struct TRmgObjectPropertiesRef { TObjectType* m_prototype; };
struct type_object { TRmgObjectPropertiesRef* m_properties; TRmgMapPosition m_position; };
struct Progress { int m_id; void advance(int); };
struct RiverRoot {
    std::vector<type_object*> m_positions;
    Progress* m_progress;
    void markRiverObjectTargets();
    void markRiverTargets();
    void createRiverToObject(TRmgMapPosition);
    void createRiver(TRmgMapPosition);
};
struct Scenario {
    int m_count, m_appendPrepared, m_grow, m_progressMode, m_mutate;
    int m_kind[8], m_x[8], m_y[8], m_z[8], m_dx[8], m_dy[8];
};
struct State {
    Scenario m_scenario;
    RiverRoot* m_root;
    TObjectType m_prototypes[8];
    TRmgObjectPropertiesRef m_properties[8];
    type_object m_objects[8];
    Progress m_progress[2];
    std::vector<int> m_events;
    int m_phase, m_callIndex, m_activeId;
    bool m_grown, m_bad;
    State(const Scenario& scenario, RiverRoot* root) : m_scenario(scenario), m_root(root),
        m_phase(0), m_callIndex(0), m_activeId(-1), m_grown(false), m_bad(false) {
        m_progress[0].m_id = 0; m_progress[1].m_id = 1;
        root->m_progress = scenario.m_progressMode ? &m_progress[0] : 0;
        for (int i = 0; i < 8; ++i) {
            m_prototypes[i].m_objectType = scenario.m_kind[i];
            m_prototypes[i].m_triggerCell.m_x = scenario.m_dx[i];
            m_prototypes[i].m_triggerCell.m_y = scenario.m_dy[i];
            m_properties[i].m_prototype = &m_prototypes[i];
            m_objects[i].m_properties = &m_properties[i];
            m_objects[i].m_position.m_x = scenario.m_x[i];
            m_objects[i].m_position.m_y = scenario.m_y[i];
            m_objects[i].m_position.m_z = scenario.m_z[i];
        }
    }
};
static State* g_state;
static void event(std::vector<int>& trace, int code, int x = 0, int y = 0, int z = 0) {
    int fields[] = {code, x, y, z};
    trace.insert(trace.end(), fields, fields + 4);
}
void RiverRoot::markRiverObjectTargets() {
    State& s = *g_state;
    if (this != s.m_root || s.m_phase != 0) s.m_bad = true;
    s.m_phase = 1;
    event(s.m_events, 1);
    for (int i = 0; i < s.m_scenario.m_count; ++i) m_positions.push_back(&s.m_objects[i]);
}
void RiverRoot::markRiverTargets() {
    State& s = *g_state;
    if (this != s.m_root || s.m_phase != 1) s.m_bad = true;
    s.m_phase = 2;
    event(s.m_events, 2);
    if (s.m_scenario.m_appendPrepared) m_positions.push_back(&s.m_objects[6]);
}
void RiverRoot::createRiverToObject(TRmgMapPosition position) {
    State& s = *g_state;
    if (this != s.m_root || s.m_phase != 2) s.m_bad = true;
    s.m_phase = 3;
    event(s.m_events, 3, position.m_x, position.m_y, position.m_z);
    // Discover the next eligible object independently of candidate's index.
    while (s.m_callIndex < int(m_positions.size()) &&
        m_positions[s.m_callIndex]->m_properties->m_prototype->m_objectType != WATER_WHEEL) ++s.m_callIndex;
    if (s.m_callIndex >= int(m_positions.size())) { s.m_bad = true; return; }
    type_object* object = m_positions[s.m_callIndex++];
    s.m_activeId = int(object - s.m_objects);
    if (s.m_scenario.m_mutate) {
        object->m_position.m_x += 51;
        object->m_position.m_y -= 39;
        object->m_position.m_z += 2;
        object->m_properties->m_prototype->m_triggerCell.m_x += 7;
        object->m_properties->m_prototype->m_triggerCell.m_y -= 9;
    }
    m_progress = &s.m_progress[0];
}
void RiverRoot::createRiver(TRmgMapPosition position) {
    State& s = *g_state;
    if (this != s.m_root || s.m_phase != 3) s.m_bad = true;
    s.m_phase = 2;
    event(s.m_events, 4, position.m_x, position.m_y, position.m_z);
    if (s.m_scenario.m_grow && !s.m_grown) {
        m_positions.push_back(&s.m_objects[7]); s.m_grown = true;
    }
    m_progress = s.m_scenario.m_progressMode == 0 ? 0 : &s.m_progress[s.m_scenario.m_progressMode == 2 ? 1 : 0];
}
void Progress::advance(int value) {
    State& s = *g_state;
    if (this != s.m_root->m_progress || s.m_phase != 2) s.m_bad = true;
    event(s.m_events, 5, m_id, value);
}
// @CANDIDATES@

static std::vector<int> reference(const Scenario& s) {
    std::vector<int> trace;
    event(trace, 1); event(trace, 2);
    std::vector<int> objects;
    for (int i = 0; i < s.m_count; ++i) objects.push_back(i);
    if (s.m_appendPrepared) objects.push_back(6);
    bool grown = false;
    for (unsigned index = 0; index < objects.size(); ++index) {
        int id = objects[index];
        if (s.m_kind[id] != WATER_WHEEL) continue;
        int x = s.m_x[id] - s.m_dx[id], y = s.m_y[id] - s.m_dy[id];
        event(trace, 3, x, y, s.m_z[id]);
        // Opaque routing may mutate the source but the second call uses
        // the captured first position, with only x translated by two.
        event(trace, 4, x - 2, y, s.m_z[id]);
        if (s.m_progressMode) event(trace, 5, s.m_progressMode == 2 ? 1 : 0, 1000);
        if (s.m_grow && !grown) { objects.push_back(7); grown = true; }
    }
    return trace;
}
static Scenario scenario(int n) {
    Scenario s;
    s.m_count = n % 7; s.m_appendPrepared = (n >> 1) & 1;
    s.m_grow = (n >> 2) & 1; s.m_progressMode = (n / 3) % 3; s.m_mutate = (n >> 3) & 1;
    for (int i = 0; i < 8; ++i) {
        s.m_kind[i] = ((n >> (i % 6)) & 1) || i == 7 ? WATER_WHEEL : 108;
        s.m_x[i] = (n * 17 + i * 91) % 2001 - 1000;
        s.m_y[i] = (n * 31 + i * 47) % 2001 - 1000;
        s.m_z[i] = (n + i) % 5 - 1;
        s.m_dx[i] = (n + 3 * i) % 31 - 15; s.m_dy[i] = (3 * n + i) % 29 - 14;
    }
    return s;
}
template<class Candidate> bool check() {
    for (int n = 0; n < 1024; ++n) {
        Scenario input = scenario(n);
        Candidate root;
        State state(input, &root); g_state = &state;
        root.createRivers();
        if (state.m_bad || state.m_phase != 2 || state.m_events != reference(input)) return false;
    }
    return true;
}
int main() {
    // @CHECKS@
    return 0;
}
