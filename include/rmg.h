// Complete-only random-map generator declarations.
#ifndef HOMM3_RMG_H
#define HOMM3_RMG_H

#include <va.h>

// Complete's random-map object factories share this five-dword prefix.  The
// constructor at 0x534160 writes the four fields, while vtable 0x640b64 proves
// three virtual operations: an object factory taking three arguments, a
// two-argument value query, and a parameterless boolean property.  The method
// names remain role descriptions until retail-era source identifies their
// original spelling; their boundaries and arities are retail-byte facts.
class RMGObjectGenerator {
public:
    int objectType;
    int subtype;
    int value;
    int density;

    RMGObjectGenerator(int objectType, int subtype, int value, int density);

    virtual void* Generate(void* owner, int x, int y);
    virtual int GetValue(void* object, void* map);
    virtual unsigned char IsTerrainDependent();
};

SIZE(RMGObjectGenerator, 0x14);

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
