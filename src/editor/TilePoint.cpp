// TilePoint.cpp - Loki h3maped object 33: the neighbour offsets and the map
// edge test. Assert lines come from the retail immediates.
#include <assert.h>
#include <algorithm>

#include "editor/TilePoint.h"

const TPoint<int> akAdjOffset[8] = {
    TPoint<int>(0, -1),
    TPoint<int>(1, -1),
    TPoint<int>(1, 0),
    TPoint<int>(1, 1),
    TPoint<int>(0, 1),
    TPoint<int>(-1, 1),
    TPoint<int>(-1, 0),
    TPoint<int>(-1, -1),
};

void computeAdjacentDirs(unsigned int width, unsigned int height,
                         unsigned int x, unsigned int y, bool (&abDir)[8])
{
#line 35
    assert(width > 0);
    assert(height > 0);
    assert(x >= 0 && x < width);
    assert(y >= 0 && y < height);
    assert(abDir != NULL);

    fill(abDir, abDir + 8, true);
    if (y == 0)
        abDir[0] = abDir[7] = abDir[1] = false;
    else if (y == height - 1)
        abDir[4] = abDir[5] = abDir[3] = false;
    if (x == 0)
        abDir[6] = abDir[7] = abDir[5] = false;
    else if (x == width - 1)
        abDir[2] = abDir[1] = abDir[3] = false;
}
