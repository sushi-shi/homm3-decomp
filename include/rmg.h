// Complete-only random-map generator declarations.
#ifndef HOMM3_RMG_H
#define HOMM3_RMG_H

// Retail's RMG set cluster stores x/y as consecutive dwords and compares y
// first, then x. The surrounding callers reach it only from random-map
// generation; the Dreamcast build has no corresponding RMG compiland.
struct TPoint {
    int x;
    int y;

    bool operator<(const TPoint& other) const
    {
        return y < other.y || (y == other.y && x < other.x);
    }
};

#endif  // HOMM3_RMG_H
