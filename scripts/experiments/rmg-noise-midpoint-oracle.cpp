#include <vector>
#include <cstdio>
// @TYPES@
typedef void (*Subdivision)(std::vector<TRmgNoiseRegion>&, int, TRmgNoiseRegion, TRmgNoiseMidpoints);

bool same(const TRmgNoiseRegion& a, const TRmgNoiseRegion& b) {
    if (a.m_bounds.m_minimumX != b.m_bounds.m_minimumX
        || a.m_bounds.m_minimumY != b.m_bounds.m_minimumY
        || a.m_bounds.m_maximumX != b.m_bounds.m_maximumX
        || a.m_bounds.m_maximumY != b.m_bounds.m_maximumY
        || a.m_variation != b.m_variation) return false;
    for (int corner = 0; corner < 4; ++corner)
        if (a.m_corners[corner] != b.m_corners[corner]) return false;
    return true;
}

bool check(Subdivision candidate) {
    for (int scenario = 0; scenario < 3136; ++scenario) {
        TRmgNoiseRegion region;
        int x = scenario % 7 - 3, y = scenario / 7 % 7 - 3;
        int width = scenario / 49 % 8, height = scenario / 392 % 8;
        region.m_bounds.m_minimumX = x; region.m_bounds.m_minimumY = y;
        region.m_bounds.m_maximumX = x + width; region.m_bounds.m_maximumY = y + height;
        region.m_variation = scenario % 17 - 8;
        for (int i = 0; i < 4; ++i) region.m_corners[i] = scenario % 31 - i * 13;
        TRmgNoiseMidpoints mids;
        mids.m_minYValue = -53; mids.m_minXValue = 37;
        mids.m_maxYValue = 71; mids.m_maxXValue = -19;
        int center = scenario % 13 - 6;
        // A 3x3 sample lattice, independent of the implementation's repeated
        // copy-and-overwrite sequence, determines all four quadrant corners.
        int samples[3][3] = {
            {region.m_corners[0], mids.m_minXValue, region.m_corners[1]},
            {mids.m_minYValue, center, mids.m_maxYValue},
            {region.m_corners[2], mids.m_maxXValue, region.m_corners[3]}
        };
        int xs[3] = {x, (2 * x + width) / 2, x + width};
        int ys[3] = {y, (2 * y + height) / 2, y + height};
        std::vector<TRmgNoiseRegion> expected(1, region), observed(1, region);
        for (int xi = 1; xi >= 0; --xi)
            for (int yi = 1; yi >= 0; --yi) {
                if (xs[xi] == xs[xi + 1] || ys[yi] == ys[yi + 1]) continue;
                TRmgNoiseRegion part;
                part.m_bounds.m_minimumX = xs[xi]; part.m_bounds.m_maximumX = xs[xi + 1];
                part.m_bounds.m_minimumY = ys[yi]; part.m_bounds.m_maximumY = ys[yi + 1];
                part.m_variation = region.m_variation;
                for (int cx = 0; cx < 2; ++cx)
                    for (int cy = 0; cy < 2; ++cy)
                        part.m_corners[cx * 2 + cy] = samples[xi + cx][yi + cy];
                expected.push_back(part);
            }
        candidate(observed, center, region, mids);
        if (observed.size() != expected.size()) return false;
        for (unsigned int index = 0; index < expected.size(); ++index)
            if (!same(expected[index], observed[index])) return false;
    }
    return true;
}
// @CANDIDATES@
int main() {
    // @CHECKS@
    return 0;
}
