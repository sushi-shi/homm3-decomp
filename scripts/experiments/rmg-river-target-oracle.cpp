// Native behavioral oracle. Imported coordinate types and constructors are
// retained. The coastal helper is scripted; a phase machine derives expected
// scan calls independently, then an edge predicate derives expected tile bits.
#include <vector>
#include <cstdio>
#include <cstring>
#include <stdint.h>
// @CONSTANTS@
// @VALUE_TYPES@
// @VALUE_HELPERS@
// @TILE_TYPES@
struct TRmgMapItem { unsigned m_guard; TRmgGroundTile m_tile; TRmgGroundTileData m_tileData; };
struct type_random_map {
    int m_mapWidth, m_mapHeight, m_numberLevels;
    TRmgMapItem* m_mapItems;
    // @ACCESSOR@
};
struct Progress { int m_id; void advance(int); };
struct TargetRoot {
    type_random_map m_map;
    Progress* m_progress;
    void markRiverCoastTarget(TRmgMapPosition, int);
};
struct Scenario { int m_width, m_height, m_levels, m_mutate, m_progress, m_seed; };
static uint32_t initial(int i, int seed) { return uint32_t(i + 1) * 0x13579bdu ^ uint32_t(seed * 73); }
static int land(int i, int seed) { return (i + seed) % 3 ? eTerrainWater : (i + seed) % 8; }
static void event(std::vector<int>& trace, int code, int x = 0, int y = 0, int z = 0, int direction = 0) {
    int data[] = {code, x, y, z, direction}; trace.insert(trace.end(), data, data + 5);
}
template<class T> static uint32_t word(const T& object) {
    uint32_t result; std::memcpy(&result, &object, sizeof(result)); return result;
}
struct Expected {
    int m_width, m_height, m_progress;
    std::vector<uint32_t> m_tiles, m_data;
    std::vector<int> m_trace;
};
static Expected reference(const Scenario& input) {
    Expected result;
    result.m_width = input.m_width; result.m_height = input.m_height; result.m_progress = input.m_progress;
    int size = input.m_width * input.m_height * (input.m_levels ? input.m_levels : 1);
    for (int i = 0; i < size + input.m_width + 2; ++i) {
        result.m_tiles.push_back((initial(i, input.m_seed) & ~uint32_t(63)) | uint32_t(land(i - 1, input.m_seed) & 63));
        result.m_data.push_back(initial(i, input.m_seed + 7));
    }
    int x = 0, y = 0, z = 0, cursor = 1, calls = 0, phase = 0;
    bool finished = false;
    while (!finished) {
        switch (phase) {
        case 0:
            if (z >= input.m_levels) { finished = true; break; }
            y = 0; phase = 1; break;
        case 1:
            if (y >= result.m_height) { ++z; phase = 0; break; }
            x = 0; phase = 2; break;
        case 2:
            if (x >= result.m_width) { ++y; phase = 1; break; }
            if ((result.m_tiles[cursor] & 63u) == unsigned(eTerrainWater)) {
                // The water test occurs once; all four directions still run
                // if an opaque call changes this or a later cell's terrain.
                int directions[] = {0, 2, 4, 6};
                for (int d = 0; d < 4; ++d) {
                    event(result.m_trace, 1, x, y, z, directions[d]);
                    ++calls;
                    if (input.m_mutate) {
                        if (calls == 1) {
                            result.m_width = input.m_width > 1 ? input.m_width - 1 : 1;
                            result.m_height = input.m_height > 1 ? input.m_height - 1 : 1;
                        }
                        int index = 1 + (calls * 5 + 1) % size;
                        result.m_tiles[index] = (result.m_tiles[index] & ~uint32_t(63)) | uint32_t(calls % 2 ? eTerrainWater : 0);
                        result.m_progress = calls % 3;
                    }
                }
            }
            ++cursor; ++x; break;
        }
    }
    int plane = result.m_width * result.m_height;
    for (int i = 0; i < plane * input.m_levels; ++i) {
        int local = i % plane, row = local / result.m_width, col = local % result.m_width;
        if (col == 0 || row == 0 || col == result.m_width - 1 || row == result.m_height - 1)
            result.m_data[i + 1] |= uint32_t(1) << 30;
    }
    if (result.m_progress) event(result.m_trace, 2, result.m_progress, 1000);
    return result;
}
struct State {
    Scenario m_scenario;
    Expected m_expected;
    TargetRoot* m_root;
    std::vector<TRmgMapItem> m_cells;
    std::vector<int> m_trace;
    Progress m_progress[2];
    int m_calls;
    bool m_bad;
    State(const Scenario& scenario, TargetRoot* root) : m_scenario(scenario), m_expected(reference(scenario)),
        m_root(root), m_cells(m_expected.m_data.size()), m_calls(0), m_bad(false) {
        m_progress[0].m_id = 1; m_progress[1].m_id = 2;
        root->m_progress = scenario.m_progress ? &m_progress[scenario.m_progress - 1] : 0;
        root->m_map.m_mapWidth = scenario.m_width; root->m_map.m_mapHeight = scenario.m_height;
        root->m_map.m_numberLevels = scenario.m_levels; root->m_map.m_mapItems = &m_cells[1];
        for (unsigned i = 0; i < m_cells.size(); ++i) {
            uint32_t tile = initial(i, scenario.m_seed), data = initial(i, scenario.m_seed + 7);
            std::memcpy(&m_cells[i].m_tile, &tile, sizeof(tile));
            std::memcpy(&m_cells[i].m_tileData, &data, sizeof(data));
            m_cells[i].m_tile.m_landType = land(int(i) - 1, scenario.m_seed);
            m_cells[i].m_guard = 0xabc000u + i;
        }
    }
    bool equalCells() const {
        for (unsigned i = 0; i < m_cells.size(); ++i)
            if (word(m_cells[i].m_tile) != m_expected.m_tiles[i] || word(m_cells[i].m_tileData) != m_expected.m_data[i] ||
                m_cells[i].m_guard != 0xabc000u + i) return false;
        return true;
    }
};
static State* g_state;
void TargetRoot::markRiverCoastTarget(TRmgMapPosition position, int direction) {
    State& s = *g_state;
    if (this != s.m_root) s.m_bad = true;
    event(s.m_trace, 1, position.m_x, position.m_y, position.m_z, direction);
    ++s.m_calls;
    if (s.m_scenario.m_mutate) {
        if (s.m_calls == 1) {
            m_map.m_mapWidth = s.m_scenario.m_width > 1 ? s.m_scenario.m_width - 1 : 1;
            m_map.m_mapHeight = s.m_scenario.m_height > 1 ? s.m_scenario.m_height - 1 : 1;
        }
        int size = s.m_scenario.m_width * s.m_scenario.m_height * (s.m_scenario.m_levels ? s.m_scenario.m_levels : 1);
        int index = 1 + (s.m_calls * 5 + 1) % size;
        s.m_cells[index].m_tile.m_landType = s.m_calls % 2 ? eTerrainWater : 0;
        m_progress = s.m_calls % 3 ? &s.m_progress[s.m_calls % 3 - 1] : 0;
    }
}
void Progress::advance(int amount) {
    if (this != g_state->m_root->m_progress || !g_state->equalCells()) g_state->m_bad = true;
    event(g_state->m_trace, 2, m_id, amount);
}
// @CANDIDATES@
template<class Candidate> bool check() {
    for (int n = 0; n < 512; ++n) {
        Scenario scenario;
        scenario.m_width = 1 + n % 7; scenario.m_height = 1 + (n / 3) % 5;
        scenario.m_levels = n % 3; scenario.m_mutate = (n >> 2) & 1;
        scenario.m_progress = (n / 7) % 3; scenario.m_seed = n;
        Candidate root;
        State state(scenario, &root); g_state = &state;
        root.markRiverTargets();
        if (state.m_bad || !state.equalCells() || state.m_trace != state.m_expected.m_trace ||
            root.m_map.m_mapWidth != state.m_expected.m_width || root.m_map.m_mapHeight != state.m_expected.m_height)
            return false;
    }
    return true;
}
int main() {
    if (sizeof(TRmgGroundTile) != 4 || sizeof(TRmgGroundTileData) != 4) return 3;
    // @CHECKS@
    return 0;
}
