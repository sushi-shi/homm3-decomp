// Reduced behavioral fixture, not an ABI or instruction-scheduling oracle.
#include <vector>
#include <cstdio>
#include <cstdlib>
// @TYPES@
struct TRmgMapPosition {
    int m_x, m_y, m_z;
    TRmgMapPosition() {}
    TRmgMapPosition(int x, int y, int z) : m_x(x), m_y(y), m_z(z) {}
};
struct TRmgMapItem {
    bool gate;
    bool hasSubterraneanGate() const { return gate; }
};
struct Root {
    int m_mapWidth, m_mapHeight;
    std::vector<TRmgMapItem> tiles;
    Root() : m_mapWidth(7), m_mapHeight(7), tiles(98) {}
    TRmgMapItem* getMapItem(TRmgMapPosition position) {
        return &tiles[(position.m_z * m_mapHeight + position.m_y) * m_mapWidth + position.m_x];
    }
};

TPoint lattice(TPoint start, int dx, int dy, int count) {
    int x = std::abs(dx), y = std::abs(dy);
    int major = x > y ? x : y, minor = x > y ? y : x;
    int diagonals = major ? (major / 2 + count * minor) / major : count;
    return TPoint(start.m_x + (dx > 0 ? 1 : -1) * (x > y ? count : diagonals),
                  start.m_y + (dy > 0 ? 1 : -1) * (x > y ? diagonals : count));
}

TPoint reference(Root& map, TPoint start, TPoint toward, int level) {
    int dx = toward.m_x - start.m_x, dy = toward.m_y - start.m_y;
    for (int step = 1; step < 50; ++step) {
        TPoint point = lattice(start, dx, dy, step);
        TPoint previous = lattice(start, dx, dy, step - 1);
        if (point.m_x < 1 || point.m_x >= map.m_mapWidth - 1
            || point.m_y < 1 || point.m_y >= map.m_mapHeight - 1)
            return previous;
        if (step > 2) {
            for (int x = point.m_x - 1; x <= point.m_x + 1; ++x)
                for (int y = point.m_y - 1; y <= point.m_y + 1; ++y)
                    if (map.getMapItem(TRmgMapPosition(x, y, level))->gate)
                        return previous;
        }
    }
    std::abort();
}

template<class T> bool check() {
    T map;
    for (int pattern = 0; pattern < 7; ++pattern) {
        for (int i = 0; i < 98; ++i)
            map.tiles[i].gate = pattern && ((i * 13 + pattern * 11) % (pattern + 3) == 0);
        for (int level = 0; level < 2; ++level)
            for (int x = 1; x < 6; ++x)
                for (int y = 1; y < 6; ++y)
                    for (int dx = -3; dx <= 3; ++dx)
                        for (int dy = -3; dy <= 3; ++dy) {
                            TPoint start(x, y), toward(x + dx, y + dy);
                            if (map.traceBranchEnd(start, toward, level) != reference(map, start, toward, level))
                                return false;
                        }
    }
    return true;
}
// @CANDIDATES@
int main() {
    // @CHECKS@
    return 0;
}
