// Actual point/vector types and operators are imported by the test.
// Reduced ring fixture checks behavior, not retail ABI or allocator state.
#include <vector>
#include <cstdio>
// @TYPES@
// @HELPERS@
struct TRmgZone {};
struct TRmgBoundaryVertex {
    TPoint m_sitePosition;
    TRmgZone* m_zone;
    TRmgBoundaryVertex* m_twin;
    TRmgBoundaryVertex* m_next;
    unsigned char m_positionComputed;
    TPoint m_position;
};
struct Root { std::vector<TRmgBoundaryVertex*> m_edges; };
template<class T> bool check() {
    int checked = 0;
    for (int scenario = 0; scenario < 3000; ++scenario) {
        int ax = scenario % 11 - 5, ay = scenario / 11 % 9 - 4;
        int bx = scenario / 7 % 13 - 6, by = scenario / 13 % 7 - 3;
        int cx = scenario / 3 % 9 - 4, cy = scenario / 5 % 11 - 5;
        int dx = bx - ax, dy = by - ay;
        int sx = cx - bx, sy = cy - by;
        int tx = ax - cx, ty = ay - cy;
        int numerator = sx * tx + sy * ty;
        int denominator = -dy * tx + dx * ty;
        if (!denominator) continue;
        // Deliberately preserve the two integer truncations, unlike the
        // floating-point circumcenter formula which is not this contract.
        TPoint expected(ax + (dx + (-dy * numerator) / denominator) / 2,
                        ay + (dy + (dx * numerator) / denominator) / 2);
        TPoint sites[3] = {TPoint(ax, ay), TPoint(bx, by), TPoint(cx, cy)};
        TRmgBoundaryVertex edges[6]; TRmgZone zone;
        T candidate;
        for (int i = 0; i < 3; ++i) {
            edges[i].m_sitePosition = sites[i];
            edges[i + 3].m_sitePosition = sites[(i + 1) % 3];
            edges[i].m_twin = &edges[i + 3];
            edges[i + 3].m_twin = &edges[i];
            edges[i].m_next = &edges[(i + 2) % 3 + 3];
            edges[i + 3].m_next = &edges[(i + 1) % 3];
            edges[i].m_zone = scenario % 3 ? &zone : 0;
            edges[i + 3].m_zone = 0;
            edges[i].m_positionComputed = scenario % 5 == 0;
            edges[i + 3].m_positionComputed = 0;
            edges[i].m_position = TPoint(-99, 71);
            edges[i + 3].m_position = TPoint(-77, 93);
            candidate.m_edges.push_back(&edges[i]);
        }
        bool active = scenario % 3 && scenario % 5;
        candidate.buildVertices();
        for (int i = 0; i < 3; ++i) {
            if (edges[i].m_position != (active ? expected : TPoint(-99, 71))) return false;
            if (edges[i].m_positionComputed != (active || scenario % 5 == 0)) return false;
            if (edges[i + 3].m_position != TPoint(-77, 93) || edges[i + 3].m_positionComputed) return false;
            if (edges[i].m_sitePosition != sites[i] || edges[i].m_twin != &edges[i + 3]) return false;
        }
        ++checked;
    }
    return checked > 1000;
}
// @CANDIDATES@
int main() {
    // @CHECKS@
    return 0;
}
