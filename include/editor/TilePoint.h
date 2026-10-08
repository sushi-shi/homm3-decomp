// TilePoint.h - the neighbours of a map tile (Loki TilePoint.cpp).
// The file name is not proven.
#ifndef HOMM3_EDITOR_TILEPOINT_H
#define HOMM3_EDITOR_TILEPOINT_H

#include "editor/Point.h"

// The eight neighbour offsets, clockwise from north (0, -1).
extern const TPoint<int> akAdjOffset[8];

// Clears abDir[i] for each neighbour akAdjOffset[i] that falls off a
// width x height map at (x, y).
void computeAdjacentDirs(unsigned int width, unsigned int height,
                         unsigned int x, unsigned int y, bool (&abDir)[8]);

#endif  /* HOMM3_EDITOR_TILEPOINT_H */
