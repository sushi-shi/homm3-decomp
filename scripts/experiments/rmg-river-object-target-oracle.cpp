// Native semantic check, not a layout/byte verdict. Actual point classes,
// bitfields and accessor implementations are imported by the test. The map
// item is a reduced host fixture; its guard and every bitfield word are checked.
#include <vector>
#include <cstdio>
#include <cstring>
#include <stdint.h>
// @CONSTANTS@
// @VALUE_TYPES@
// @TILE_DATA@
struct TRmgMapItem { unsigned m_guard; TRmgGroundTileData m_tileData; };
struct type_random_map {
    int m_mapWidth, m_mapHeight;
    TRmgMapItem* m_mapItems;
    // @SCALAR_ACCESSOR@
    TRmgMapItem* getMapItem(TRmgMapPosition);
};
// @POSITION_ACCESSOR@
// @VALUE_HELPERS@
struct TObjectType {
    // @PROTOTYPE_POINT@
    struct ImageInfo { TPoint m_objectSize; } m_imageInfo;
    TPoint m_triggerCell;
    int m_objectType, m_subtype;
    unsigned char m_hasTrigger;
};
struct TRmgObjectPropertiesRef { TObjectType* m_prototype; };
struct type_object { TRmgObjectPropertiesRef* m_properties; TRmgMapPosition m_position; };
struct Progress { void advance(int); };
struct TargetRoot {
    type_random_map m_map;
    std::vector<type_object*> m_positions;
    Progress* m_progress;
};
struct Item { int m_kind, m_subtype, m_trigger, m_x, m_y, m_z, m_dx, m_dy, m_width, m_height; };
struct Scenario { int m_width, m_height, m_count, m_progress, m_seed; Item m_items[8]; };
static uint32_t initialWord(int index, int seed) { return uint32_t(index + 1) * 0x13579bdu ^ uint32_t(seed * 73); }
static uint32_t word(const TRmgMapItem& item) {
    uint32_t result; std::memcpy(&result, &item.m_tileData, sizeof(result)); return result;
}
static std::vector<uint32_t> reference(const Scenario& s) {
    int count = s.m_width * s.m_height * 2;
    std::vector<uint32_t> result;
    for (int i = 0; i < count + s.m_width + 2; ++i) result.push_back(initialWord(i, s.m_seed));
    for (int i = 0; i < s.m_count; ++i) {
        const Item& item = s.m_items[i];
        bool accepted;
        switch (item.m_kind) {
        case TERRAIN_MOUNTAIN: case TERRAIN_LAKE: accepted = true; break;
        case MINE: accepted = item.m_subtype == GEMS; break;
        default: accepted = false;
        }
        if (!accepted) continue;
        int64_t x = int64_t(item.m_x) - (item.m_trigger ? int64_t(item.m_dx) : int64_t(uint32_t(item.m_width) / 2));
        int64_t y = int64_t(item.m_y) - (item.m_trigger ? int64_t(item.m_dy) : int64_t(uint32_t(item.m_height) / 2));
        if (x < 0 || y < 0 || x >= s.m_width || y >= s.m_height) continue;
        unsigned index = 1 + item.m_z * s.m_width * s.m_height + unsigned(y) * s.m_width + unsigned(x);
        result[index] |= uint32_t(1) << 29;
    }
    return result;
}
struct State {
    TargetRoot* m_root;
    Scenario m_scenario;
    TObjectType m_prototypes[8];
    TRmgObjectPropertiesRef m_properties[8];
    type_object m_objects[8];
    std::vector<TRmgMapItem> m_cells;
    std::vector<uint32_t> m_expected;
    Progress m_progress;
    int m_progressCalls;
    bool m_bad;
    State(const Scenario& input, TargetRoot* root) : m_root(root), m_scenario(input),
        m_cells(input.m_width * input.m_height * 2 + input.m_width + 2), m_expected(reference(input)), m_progressCalls(0), m_bad(false) {
        root->m_map.m_mapWidth = input.m_width; root->m_map.m_mapHeight = input.m_height;
        root->m_map.m_mapItems = &m_cells[1]; root->m_progress = input.m_progress ? &m_progress : 0;
        for (unsigned i = 0; i < m_cells.size(); ++i) {
            uint32_t initial = initialWord(i, input.m_seed);
            std::memcpy(&m_cells[i].m_tileData, &initial, sizeof(initial));
            m_cells[i].m_guard = 0xabc000u + i;
        }
        for (int i = 0; i < input.m_count; ++i) {
            const Item& item = input.m_items[i];
            TObjectType& prototype = m_prototypes[i];
            prototype.m_objectType = item.m_kind; prototype.m_subtype = item.m_subtype;
            prototype.m_hasTrigger = item.m_trigger;
            prototype.m_triggerCell.m_x = item.m_dx; prototype.m_triggerCell.m_y = item.m_dy;
            prototype.m_imageInfo.m_objectSize.m_x = item.m_width; prototype.m_imageInfo.m_objectSize.m_y = item.m_height;
            m_properties[i].m_prototype = &prototype;
            m_objects[i].m_properties = &m_properties[i];
            m_objects[i].m_position.m_x = item.m_x; m_objects[i].m_position.m_y = item.m_y; m_objects[i].m_position.m_z = item.m_z;
            root->m_positions.push_back(&m_objects[i]);
        }
    }
    bool equal() const {
        for (unsigned i = 0; i < m_cells.size(); ++i)
            if (word(m_cells[i]) != m_expected[i] || m_cells[i].m_guard != 0xabc000u + i) return false;
        return true;
    }
};
static State* g_state;
void Progress::advance(int value) {
    if (this != &g_state->m_progress || value != 1000 || !g_state->equal()) g_state->m_bad = true;
    ++g_state->m_progressCalls;
}
// @CANDIDATES@
static Scenario scenario(int n) {
    Scenario s;
    s.m_width = 1 + n % 9; s.m_height = 1 + (n / 3) % 7;
    s.m_count = n % 9; s.m_progress = n & 1; s.m_seed = n;
    int kinds[] = {TERRAIN_MOUNTAIN, TERRAIN_LAKE, MINE, MINE, 109, 1};
    for (int i = 0; i < 8; ++i) {
        Item& a = s.m_items[i];
        a.m_kind = kinds[(n + i) % 6]; a.m_subtype = (n + i) % 2 ? GEMS : 4;
        a.m_trigger = (n >> (i % 5)) & 1;
        a.m_x = (n + i * 3) % (s.m_width + 4) - 1; a.m_y = (n * 3 + i) % (s.m_height + 4) - 1;
        a.m_z = (n + i) & 1; a.m_dx = (n + i) % 7 - 3; a.m_dy = (n + 2 * i) % 9 - 4;
        a.m_width = (n + i) % 8 + 1; a.m_height = (3 * n + i) % 6 + 1;
        if (n % 11 == 0) {
            a.m_width = i % 2 ? -1 : (-2147483647 - 1); a.m_height = n % 2 ? 1 : -1;
            a.m_x = i; a.m_y = i; // keep signed subtraction defined for unsigned half sizes
        }
    }
    return s;
}
template<class Candidate> bool check() {
    for (int n = 0; n < 1536; ++n) {
        Scenario s = scenario(n);
        Candidate root;
        State state(s, &root); g_state = &state;
        root.markRiverObjectTargets();
        if (state.m_bad || !state.equal() || state.m_progressCalls != s.m_progress) return false;
    }
    return true;
}
int main() {
    if (sizeof(TRmgGroundTileData) != sizeof(uint32_t)) return 3;
    // @CHECKS@
    return 0;
}
