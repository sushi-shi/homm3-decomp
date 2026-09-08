// Host-only independent rectangle perimeter and query-trace checks. Actual
// value classes, tile bitfields, predicates, point addition, lookup and source
// methods are inserted by test_rmg_treasure_outline.py, never into VC6 input.
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <vector>
enum { eTerrainRock = 9 };
// @VALUE_TYPES@
struct TRmgMapItem {
    TRmgGroundTile m_tile;
    TRmgGroundTileData m_tileData;
    // @PREDICATES@
};
struct type_random_map {
    int m_mapWidth, m_mapHeight;
    TRmgMapItem* m_mapItems;
    std::vector<TRmgMapItem> m_cells;
    std::vector<int> m_queries;
    void record(int x, int y, int z) {
        if (x < 0 || x >= m_mapWidth || y < 0 || y >= m_mapHeight || z != 0
            || m_queries.size() >= 10000) throw std::runtime_error("invalid lookup or unbounded walk");
        m_queries.push_back(x); m_queries.push_back(y); m_queries.push_back(z);
    }
    // @SCALAR_LOOKUP@
};
// @VALUE_HELPERS@
// @DIRECTIONS@
struct OutlineRoot {
    type_random_map m_map;
    std::vector<TPoint> m_outline;
};
// @CANDIDATES@

static void initialize(OutlineRoot& root, int width, int height, int flags,
    int left, int top, int right, int bottom) {
    type_random_map& map = root.m_map;
    map.m_mapWidth = width; map.m_mapHeight = height;
    map.m_cells.resize(width * height);
    map.m_mapItems = map.m_cells.empty() ? 0 : &map.m_cells[0];
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
        TRmgMapItem& cell = map.m_cells[y * width + x];
        cell.m_tile.m_landType = 0;
        cell.m_tileData.m_roadEntrance = 0;
        cell.m_tileData.m_roadPassable = 1;
        cell.m_tileData.m_subterraneanGate = 1;
        // Exercise the other high bits without changing the four predicates.
        cell.m_tileData.m_zoneBoundary = (x + y) & 1;
        cell.m_tileData.m_borderObject = (x * 3 + y) & 1;
        if (x >= left && x <= right && y >= top && y <= bottom) {
            cell.m_tileData.m_roadEntrance = (flags & 1) != 0;
            cell.m_tileData.m_roadPassable = (flags & 2) == 0;
            cell.m_tile.m_landType = (flags & 4) ? eTerrainRock : 0;
            cell.m_tileData.m_subterraneanGate = (flags & 8) == 0;
        }
    }
}

static std::vector<TPoint> rectangle(int left, int top, int right, int bottom) {
    // Expanded-by-one clockwise perimeter, starting above the first blocked
    // cell. Construct each straight edge independently, without the candidate's
    // direction search or tile predicate. Do not repeat the closing vertex.
    std::vector<TPoint> points;
    for (int x = left; x <= right + 1; ++x) points.push_back(TPoint(x, top - 1));
    for (int y = top; y <= bottom + 1; ++y) points.push_back(TPoint(right + 1, y));
    for (int x = right; x >= left - 1; --x) points.push_back(TPoint(x, bottom + 1));
    for (int y = bottom; y >= top - 1; --y) points.push_back(TPoint(left - 1, y));
    return points;
}

template<class Candidate> static bool check() {
    try {
        for (int width = 0; width <= 5; ++width) for (int height = 0; height <= 4; ++height) {
            Baseline baseline; Candidate actual;
            initialize(baseline, width, height, 0, 0, 0, -1, -1);
            initialize(actual, width, height, 0, 0, 0, -1, -1);
            baseline.traceOutline(); actual.traceOutline();
            std::vector<TPoint> expected;
            if (width > 0 && height == 0) {
                expected.push_back(TPoint(0, -1)); expected.push_back(TPoint(1, -1));
                expected.push_back(TPoint(1, 0)); expected.push_back(TPoint(0, 0));
            }
            if (actual.m_outline != expected || baseline.m_outline != expected
                || actual.m_map.m_queries != baseline.m_map.m_queries) return false;
            // Cache guard must do no map queries, even for degenerate maps.
            actual.m_outline.push_back(TPoint(77, -19));
            expected = actual.m_outline;
            actual.m_map.m_queries.clear(); actual.traceOutline();
            if (actual.m_outline != expected || !actual.m_map.m_queries.empty()) return false;
        }
        for (int width = 1; width <= 5; ++width) for (int height = 1; height <= 4; ++height)
        for (int left = 0; left < width; ++left) for (int right = left; right < width; ++right)
        for (int top = 0; top < height; ++top) for (int bottom = top; bottom < height; ++bottom)
        for (int flags = 1; flags < 16; ++flags) {
            Baseline baseline; Candidate actual;
            initialize(baseline, width, height, flags, left, top, right, bottom);
            initialize(actual, width, height, flags, left, top, right, bottom);
            baseline.traceOutline(); actual.traceOutline();
            const std::vector<TPoint> expected = rectangle(left, top, right, bottom);
            if (actual.m_outline != expected || baseline.m_outline != expected
                || actual.m_map.m_queries != baseline.m_map.m_queries) return false;
        }
    } catch (const std::exception&) { return false; }
    return true;
}
int main() {
    // @CHECKS@
    std::puts("treasure-outline states preserve rectangle perimeters, flags, scan/query order and degenerate/cache behavior");
}
