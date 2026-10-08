// resrce.h - original CodeView owner of resource and its inline accessors.
#ifndef HOMM3_RESRCE_H
#define HOMM3_RESRCE_H

#include "va.h"

// Dreamcast-attested resource-type domain; values unattested - grow as
// consumers prove them.
enum EResourceType {
    RESOURCE_TYPE_INVALID = -1,
    RESOURCE_TYPE_NONE = 0,
    // Complete's common missing-resource reporter maps switch value 1 to
    // the literal "data"; NH3API's cross-build enum independently names it.
    RESOURCE_TYPE_DATA = 1,
    // Bitmap816 / Dreamcast RType_bitmap and RType_bitmap8.
    RESOURCE_TYPE_BITMAP = 16,
    // Both retail text-resource constructors pass 2 to the base ctor.
    RESOURCE_TYPE_TEXT = 2,
    // Bitmap24Bit's retail constructors pass 0x11 to resource::resource.
    RESOURCE_TYPE_BITMAP24 = 17,
    // The remaining bitmap and sprite values are the Dreamcast EResourceType
    // roster verbatim. Retail's RemapGraphics/SaturateGraphics jump tables
    // independently prove the values they dispatch (18, 66..71 and 73).
    RESOURCE_TYPE_BITMAP16 = 18,
    RESOURCE_TYPE_BITMAP565 = 19,
    RESOURCE_TYPE_BITMAP555 = 20,
    RESOURCE_TYPE_BITMAP1555 = 21,
    // Byte-proven by sample::sample (0x566da0 pushes 0x20 into the base
    // ctor); the value's DC name is RType_sfx (NB11 enum records), respelled to this file's convention.
    RESOURCE_TYPE_SFX = 32,
    RESOURCE_TYPE_MIDI = 48,
    RESOURCE_TYPE_SPRITE = 64,
    RESOURCE_TYPE_SPRITE_DEFINITION = 65,
    RESOURCE_TYPE_CREATURE = 66,
    RESOURCE_TYPE_ADVENTURE_OBJECT = 67,
    RESOURCE_TYPE_HERO = 68,
    RESOURCE_TYPE_TILESET = 69,
    RESOURCE_TYPE_POINTER = 70,
    RESOURCE_TYPE_INTERFACE = 71,
    RESOURCE_TYPE_SPRITE_FRAME = 72,
    RESOURCE_TYPE_COMBAT_HERO = 73,
    // Complete's GetNumSeqs jump table has explicit zero-return arms for the
    // otherwise unnamed five-value gap before advmask. Dreamcast's enum omits
    // them, so these reserved spellings are honest PC-only domain names.
    RESOURCE_TYPE_RESERVED_74 = 74,
    RESOURCE_TYPE_RESERVED_75 = 75,
    RESOURCE_TYPE_RESERVED_76 = 76,
    RESOURCE_TYPE_RESERVED_77 = 77,
    RESOURCE_TYPE_RESERVED_78 = 78,
    RESOURCE_TYPE_ADVENTURE_MASK = 79,
    // Byte-proven by font::font (0x4b5070 pushes 0x50 into the base ctor).
    RESOURCE_TYPE_FONT = 80,
    RESOURCE_TYPE_PALETTE = 96
};

// Loki h3maped (object 49, RoE source): Name char[13]@0 (12-char strncpy
// + forced NUL), resType@0x10, ReferenceCount@0x14 and g++ 2.95's vptr
// last at 0x18 - the Dreamcast field roster verbatim. The vtable holds only
// the destructor; Complete's Dispose/GetSize slots are later additions.
// The accessors are in-class inlines (Dreamcast resrce.h:36); the 2.95.2
// release compiler emits them as strong .text globals in resource.o, the
// unit that defines the vtable, after the constructor and destructor.
class resource {
public:
    resource(const char* name, EResourceType type);
    virtual ~resource();

    EResourceType get_resType() const { return resType; }
    const char* get_Name() const { return Name; }
    int AddRef() { return ++ReferenceCount; }
    int Release()
    {
        if (ReferenceCount > 0)
            --ReferenceCount;
        return ReferenceCount;
    }
    int GetReferenceCount() const { return ReferenceCount; }

private:
    char Name[13];
    EResourceType resType;
    int ReferenceCount;
};

#endif  /* HOMM3_RESRCE_H */
