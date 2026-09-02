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
class type_treasure_def {
public:
    int objectType;
    int subtype;
    int value;
    int density;

    type_treasure_def(int objectType, int subtype, int value, int density);

    virtual void* Generate(void* owner, int x, int y);
    virtual int GetValue(void* object, void* map);
    virtual unsigned char IsTerrainDependent();
};

SIZE(type_treasure_def, 0x14);

// These identities come from the contiguous cross-build vtable roster.  The
// current-image constructor relocations independently fix each table address.
class type_shrine_def : public type_treasure_def {
public:
    type_shrine_def(int objectType, int value);
    virtual void* Generate(void* owner, int x, int y);
};

class type_witch_hut_def : public type_treasure_def {
public:
    type_witch_hut_def();
    virtual void* Generate(void* owner, int x, int y);
};

class type_spell_scroll_def : public type_treasure_def {
public:
    int spellLevel;

    type_spell_scroll_def(int spellLevel, int value);
    virtual void* Generate(void* owner, int x, int y);
};

class type_black_box_creature_def : public type_treasure_def {
public:
    int creatureType;
    int adjustedValue;

    type_black_box_creature_def(int creatureType);
    virtual void* Generate(void* owner, int x, int y);
    virtual int GetValue(void* object, void* map);
};

SIZE(type_shrine_def, 0x14);
SIZE(type_witch_hut_def, 0x14);
SIZE(type_spell_scroll_def, 0x18);
SIZE(type_black_box_creature_def, 0x1c);

// Retail 0x6824e0 is indexed by the creature-traits level dword before
// type_black_box_creature_def divides by that creature's AI value.
DATA(0x006824E0) extern int gRmgCreatureValueByLevel[];

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
