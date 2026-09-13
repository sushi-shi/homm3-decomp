// path.h - prototypes of path.cpp (compiland path.obj)
#ifndef HOMM3_PATH_H
#define HOMM3_PATH_H

int oppositeDirection(int direction);
// 0x523e80, claimed in src/path.cpp. Declared here for the same reason:
// spells.obj's MirrorImage walks outward from the source stack through
// it, one direction at a time, looking for a free hex to clone into.
int getAdjacentCellIndexNoArmy(int currIndex, int direction);

// --- army ---
// CODEVIEW(E:\gamedcs\path.cpp:480, dc 0x10cd50) int army::GetBestDirection(int currIndex, int destIndex, int currMask);

#endif  /* HOMM3_PATH_H */
