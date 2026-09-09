// Native caller-contract oracle. Flat-cell selection is independent of the
// candidate's adjusted-bounds loops; VC6 remains the code/layout verdict.
#include <cstdio>
#include <vector>
#include <algorithm>

// @VALUE_TYPES@

struct TRmgMapItem { TRmgZoneCellState m_zoneState; };
struct TRmgTownSlot {
    // @SLOT_FIELDS@
};
struct TRmgZone {
    // @ZONE_FIELDS@
    TRmgMapPosition getLevelPosition() const;
};
struct TRmgTreasureGroup {
    // @GROUP_FIELDS@
};

enum { mapWidth = 24, mapHeight = 20, mapCount = mapWidth * mapHeight * 2 };
struct Point {
    int m_x, m_y, m_z;
    Point(int newX = 0, int newY = 0, int newZ = 0) : m_x(newX), m_y(newY), m_z(newZ) {}
    bool operator==(const Point& other) const { return m_x == other.m_x && m_y == other.m_y && m_z == other.m_z; }
};
struct Event {
    int m_kind;
    Point m_point;
    Event(int newKind, Point newPoint = Point()) : m_kind(newKind), m_point(newPoint) {}
    bool operator==(const Event& other) const { return m_kind == other.m_kind && m_point == other.m_point; }
};
struct FlatCell {
    unsigned m_score;
    int m_zone, m_permitted, m_after;
    FlatCell() : m_score(0), m_zone(0), m_permitted(1), m_after(-1) {}
};
struct Scenario {
    TRmgZoneBounds m_bounds, m_groupBounds;
    Point m_level;
    int m_zoneIndex, m_spacing, m_randomValue;
    bool m_mutateSnapshots;
    std::vector<FlatCell> m_cells;
    Scenario() : m_level(17, 13, 0), m_zoneIndex(37), m_spacing(3), m_randomValue(0),
        m_mutateSnapshots(false), m_cells(mapCount) {
        m_bounds.m_minimumX = 3; m_bounds.m_minimumY = 4;
        m_bounds.m_maximumX = 8; m_bounds.m_maximumY = 8;
        m_groupBounds.m_minimumX = -1; m_groupBounds.m_minimumY = -2;
        m_groupBounds.m_maximumX = 2; m_groupBounds.m_maximumY = 1;
    }
};
struct Expected {
    bool m_result, m_touchedSnapshots;
    std::vector<Event> m_events;
    std::vector<FlatCell> m_cells;
    Expected() : m_result(false), m_touchedSnapshots(false) {}
};

int indexOf(int x, int y, int z) { return (z * mapHeight + y) * mapWidth + x; }
bool inMap(int x, int y, int z) {
    return x >= 0 && x < mapWidth && y >= 0 && y < mapHeight && z >= 0 && z < 2;
}
int midpoint(int low, int high) {
    int sum = low + high;
    return sum < 0 ? -((-sum) / 2) : sum / 2;
}
Expected expectedFor(const Scenario& scenario) {
    Expected expected;
    expected.m_cells = scenario.m_cells;
    std::vector<Point> winners;
    unsigned required = scenario.m_spacing;
    int centerX = midpoint(scenario.m_groupBounds.m_minimumX, scenario.m_groupBounds.m_maximumX);
    int centerY = midpoint(scenario.m_groupBounds.m_minimumY, scenario.m_groupBounds.m_maximumY);
    // Enumerate a containing coordinate domain, then test the translated
    // footprint inequalities directly, instead of reusing adjusted bounds.
    for (int y = -16; y != 33; ++y)
    for (int x = -16; x != 33; ++x) {
        if (x + scenario.m_groupBounds.m_minimumX < scenario.m_bounds.m_minimumX ||
            x + scenario.m_groupBounds.m_maximumX > scenario.m_bounds.m_maximumX ||
            y + scenario.m_groupBounds.m_minimumY < scenario.m_bounds.m_minimumY ||
            y + scenario.m_groupBounds.m_maximumY > scenario.m_bounds.m_maximumY)
            continue;
        int cellX = x + centerX, cellY = y + centerY, z = scenario.m_level.m_z;
        if (!inMap(cellX, cellY, z)) { expected.m_events.push_back(Event(99)); return expected; }
        expected.m_events.push_back(Event(0, Point(cellX, cellY, z)));
        FlatCell& cell = expected.m_cells[indexOf(cellX, cellY, z)];
        if (cell.m_zone != scenario.m_zoneIndex || cell.m_score < required)
            continue;
        expected.m_events.push_back(Event(1, Point(x, y, z)));
        if (cell.m_after >= 0) cell.m_score = unsigned(cell.m_after) & 65535u;
        if (scenario.m_mutateSnapshots) expected.m_touchedSnapshots = true;
        if (!cell.m_permitted) continue;
        // The opaque fit call may lower the cell's score. Retail still adds
        // that candidate: only the greater-than test is repeated after it.
        if (cell.m_score > required) { required = cell.m_score; winners.clear(); }
        winners.push_back(Point(x, y, z));
    }
    if (!winners.empty()) {
        expected.m_result = true;
        expected.m_events.push_back(Event(2));
        expected.m_events.push_back(Event(3, winners[unsigned(scenario.m_randomValue) % winners.size()]));
    }
    return expected;
}

struct SelectionRoot;
SelectionRoot* g_activeSelection;
struct type_random_map {
    // @MAP_FIELDS@
    SelectionRoot* m_owner;
    void record(int x, int y, int z);
    // @SCALAR_LOOKUP@
    TRmgMapItem* getMapItem(TRmgMapPosition position);
};
struct SelectionRoot {
    type_random_map m_map;
    std::vector<TRmgMapItem> m_cells;
    std::vector<Event> m_events;
    const Scenario* m_scenario;
    TRmgZone* m_zone;
    TRmgTreasureGroup* m_group;
    bool m_bad;
    void initialize(const Scenario& scenario, TRmgZone* zone, TRmgTreasureGroup* group) {
        m_scenario = &scenario; m_zone = zone; m_group = group; m_bad = false;
        m_cells.resize(mapCount);
        for (int i = 0; i != mapCount; ++i) {
            m_cells[i].m_zoneState.m_score = scenario.m_cells[i].m_score;
            m_cells[i].m_zoneState.m_zone = scenario.m_cells[i].m_zone;
            m_cells[i].m_zoneState.m_connectionEligibility = -23;
        }
        m_map.m_mapItems = &m_cells[0]; m_map.m_mapWidth = mapWidth;
        m_map.m_mapHeight = mapHeight; m_map.m_owner = this;
        g_activeSelection = this;
    }
    unsigned char canPlaceTreasureGroup(TRmgTreasureGroup* group, TRmgMapPosition position, TRmgZone* zone) {
        m_bad = m_bad || group != m_group || zone != m_zone;
        m_events.push_back(Event(1, Point(position.m_x, position.m_y, position.m_z)));
        int x = position.m_x + midpoint(m_scenario->m_groupBounds.m_minimumX, m_scenario->m_groupBounds.m_maximumX);
        int y = position.m_y + midpoint(m_scenario->m_groupBounds.m_minimumY, m_scenario->m_groupBounds.m_maximumY);
        if (!inMap(x, y, position.m_z)) { m_bad = true; return 0; }
        int index = indexOf(x, y, position.m_z);
        const FlatCell& cell = m_scenario->m_cells[index];
        if (cell.m_after >= 0) m_cells[index].m_zoneState.m_score = cell.m_after;
        if (m_scenario->m_mutateSnapshots) {
            zone->m_bounds.m_maximumX = -99; zone->m_bounds.m_maximumY = -99;
            zone->m_slot->m_zoneIndex = 99; zone->m_levelPosition.m_z = 1 - m_scenario->m_level.m_z;
            group->m_bounds.m_minimumX = 99; group->m_bounds.m_minimumY = 99;
        }
        return cell.m_permitted;
    }
    void commitTreasureGroup(TRmgTreasureGroup* group, TRmgMapPosition position) {
        m_bad = m_bad || group != m_group;
        m_events.push_back(Event(3, Point(position.m_x, position.m_y, position.m_z)));
    }
};
void type_random_map::record(int x, int y, int z) {
    m_owner->m_bad = m_owner->m_bad || !inMap(x, y, z);
    m_owner->m_events.push_back(Event(0, Point(x, y, z)));
}
int oracleRand() {
    g_activeSelection->m_events.push_back(Event(2));
    return g_activeSelection->m_scenario->m_randomValue;
}

// @VALUE_HELPERS@

#define rand oracleRand
// @CANDIDATES@
#undef rand

bool sameBounds(const TRmgZoneBounds& left, const TRmgZoneBounds& right) {
    return left.m_minimumX == right.m_minimumX && left.m_minimumY == right.m_minimumY &&
        left.m_maximumX == right.m_maximumX && left.m_maximumY == right.m_maximumY;
}
std::vector<Scenario> g_scenarios;
std::vector<Expected> g_expected;
template<class Candidate> bool check() {
    for (unsigned trial = 0; trial != g_scenarios.size(); ++trial) {
        const Scenario& scenario = g_scenarios[trial];
        const Expected& expected = g_expected[trial];
        TRmgTownSlot slot; slot.m_zoneIndex = scenario.m_zoneIndex;
        TRmgZone zone; zone.m_slot = &slot; zone.m_bounds = scenario.m_bounds;
        zone.m_levelPosition.m_x = scenario.m_level.m_x; zone.m_levelPosition.m_y = scenario.m_level.m_y;
        zone.m_levelPosition.m_z = scenario.m_level.m_z;
        TRmgTreasureGroup group; group.m_bounds = scenario.m_groupBounds;
        Candidate candidate; candidate.initialize(scenario, &zone, &group);
        unsigned char result = candidate.placeTreasureGroup(&group, &zone, scenario.m_spacing);
        if (bool(result) != expected.m_result || candidate.m_bad || candidate.m_events != expected.m_events)
            return false;
        TRmgZoneBounds finalBounds = scenario.m_bounds, finalGroup = scenario.m_groupBounds;
        if (expected.m_touchedSnapshots) {
            finalBounds.m_maximumX = finalBounds.m_maximumY = -99;
            finalGroup.m_minimumX = finalGroup.m_minimumY = 99;
        }
        if (!sameBounds(zone.m_bounds, finalBounds) || !sameBounds(group.m_bounds, finalGroup) ||
            zone.m_slot != &slot || slot.m_zoneIndex != (expected.m_touchedSnapshots ? 99 : scenario.m_zoneIndex) ||
            zone.m_levelPosition.m_x != scenario.m_level.m_x || zone.m_levelPosition.m_y != scenario.m_level.m_y ||
            zone.m_levelPosition.m_z != (expected.m_touchedSnapshots ? 1 - scenario.m_level.m_z : scenario.m_level.m_z))
            return false;
        for (int i = 0; i != mapCount; ++i)
            if (candidate.m_cells[i].m_zoneState.m_score != expected.m_cells[i].m_score ||
                candidate.m_cells[i].m_zoneState.m_zone != expected.m_cells[i].m_zone ||
                candidate.m_cells[i].m_zoneState.m_connectionEligibility != -23)
                return false;
    }
    return true;
}
void add(const Scenario& scenario) {
    g_scenarios.push_back(scenario);
    g_expected.push_back(expectedFor(scenario));
}
void buildScenarios() {
    // The native contract covers nonnegative spacing. VC6 treats the
    // unsigned 16-bit bitfield comparison as unsigned even for a negative
    // int threshold, unlike host bitfield promotion; retail bytes own that
    // ABI edge. No candidate in this family changes either operand type.
    const int randomValues[] = {0, 1, 2, 7, 32767};
    const int spacings[] = {0, 3, 7, 65535, 65536};
    for (int level = 0; level != 2; ++level)
    for (int pattern = 0; pattern != 10; ++pattern)
    for (int geometry = 0; geometry != 6; ++geometry)
    for (int random = 0; random != 5; ++random) {
        Scenario scenario;
        scenario.m_level.m_z = level; scenario.m_randomValue = randomValues[random];
        scenario.m_spacing = spacings[(pattern + geometry) % 5];
        scenario.m_zoneIndex = geometry == 5 ? -5 : 37;
        scenario.m_mutateSnapshots = pattern == 9;
        if (geometry == 1) { scenario.m_groupBounds.m_minimumX = -4; scenario.m_groupBounds.m_maximumX = -1; }
        if (geometry == 2) { scenario.m_groupBounds.m_minimumY = 2; scenario.m_groupBounds.m_maximumY = 5; }
        if (geometry == 3) { scenario.m_bounds.m_maximumX = scenario.m_bounds.m_minimumX; }
        if (geometry == 4) {
            scenario.m_bounds.m_maximumX = scenario.m_bounds.m_minimumX + 3;
            scenario.m_bounds.m_maximumY = scenario.m_bounds.m_minimumY + 3;
        }
        for (int z = 0; z != 2; ++z)
        for (int y = 0; y != mapHeight; ++y)
        for (int x = 0; x != mapWidth; ++x) {
            FlatCell& cell = scenario.m_cells[indexOf(x, y, z)];
            cell.m_zone = ((x + y + pattern) % 7 == 0) ? 99 : scenario.m_zoneIndex;
            cell.m_score = pattern == 0 ? 3 : pattern == 1 ? 65535 : (x * 3 + y * 7 + z) % 12;
            cell.m_permitted = pattern == 2 ? 0 : pattern == 3 ? 255 : (x + 2 * y + pattern) % 3;
            if (pattern == 4) cell.m_after = 65535;
            if (pattern == 5) cell.m_after = 0;
            if (pattern == 6) cell.m_after = 65536 + x + y;
            if (pattern == 7) cell.m_zone = 99;
            if (pattern == 8) { cell.m_score = 7; cell.m_zone = scenario.m_zoneIndex; }
        }
        add(scenario);
    }
    // All masks over eight tied candidates, with each legal random residue.
    for (int mask = 0; mask != 256; ++mask)
    for (int level = 0; level != 2; ++level)
    for (int random = 0; random != 8; ++random) {
        Scenario scenario;
        scenario.m_groupBounds.m_minimumX = scenario.m_groupBounds.m_minimumY = 0;
        scenario.m_groupBounds.m_maximumX = scenario.m_groupBounds.m_maximumY = 1;
        scenario.m_bounds.m_minimumX = 3; scenario.m_bounds.m_minimumY = 4;
        scenario.m_bounds.m_maximumX = 7; scenario.m_bounds.m_maximumY = 6;
        scenario.m_level.m_z = level; scenario.m_randomValue = random;
        for (int index = 0; index != 8; ++index) {
            FlatCell& cell = scenario.m_cells[indexOf(3 + index % 4, 4 + index / 4, level)];
            cell.m_zone = scenario.m_zoneIndex; cell.m_score = 3;
            cell.m_permitted = (mask & (1 << index)) != 0;
        }
        add(scenario);
    }
}
int main() {
    buildScenarios();
    // @CHECKS@
    return 0;
}
