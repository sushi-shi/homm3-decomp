// resrce.h - original CodeView owner of resource and its inline accessors.
#ifndef HOMM3_RESRCE_H
#define HOMM3_RESRCE_H

#include "domains.h"
#include "va.h"

// Dreamcast EResourceType (NB11 enum records), spelled as Dreamcast does.
// A value catalog: the Dreamcast sprite-type interfaces (GetSpriteType,
// GetSpriteTypeName, GetNumSeqs) carry it as an int.
H3_ENUM_VALUES_BEGIN(EResourceType)
    RType_invalid = -1,
    RType_null = 0,
    // Complete's common missing-resource reporter maps switch value 1 to
    // the literal "data"; NH3API's cross-build enum independently names it.
    RType_data = 1,
    // Bitmap816; Dreamcast also spells this value RType_bitmap8.
    RType_bitmap = 16,
    // Both retail text-resource constructors pass 2 to the base ctor.
    RType_text = 2,
    // Bitmap24Bit's retail constructors pass 0x11 to resource::resource.
    RType_bitmap24 = 17,
    // The remaining bitmap and sprite values are the Dreamcast roster.
    // Retail's RemapGraphics/SaturateGraphics jump tables independently
    // prove the values they dispatch (18, 66..71 and 73).
    RType_bitmap16 = 18,
    RType_bitmap565 = 19,
    RType_bitmap555 = 20,
    RType_bitmap1555 = 21,
    // Byte-proven by sample::sample (0x566da0 pushes 0x20 into the base
    // ctor).
    RType_sfx = 32,
    RType_midi = 48,
    RType_sprite = 64,
    RType_spritedef = 65,
    RType_creature = 66,
    RType_advobj = 67,
    RType_hero = 68,
    RType_tileset = 69,
    RType_pointer = 70,
    RType_interface = 71,
    RType_spriteframe = 72,
    RType_combat_hero = 73,
    // Complete's GetNumSeqs jump table has explicit zero-return arms for the
    // otherwise unnamed five-value gap before advmask. Dreamcast's enum omits
    // them, so these reserved spellings are honest PC-only domain names.
    RType_reserved_74 = 74,
    RType_reserved_75 = 75,
    RType_reserved_76 = 76,
    RType_reserved_77 = 77,
    RType_reserved_78 = 78,
    RType_advmask = 79,
    // Byte-proven by font::font (0x4b5070 pushes 0x50 into the base ctor).
    RType_font = 80,
    RType_palette = 96
H3_ENUM_VALUES_END(EResourceType)

// PROVEN layout (retail ctor 0x558720): vptr, Name char[13]@4 (12-char
// strncpy + forced NUL at +0x10), resType@0x14, ReferenceCount@0x18 -
// the Dreamcast roster verbatim, size 0x1c. Retail vtable 0x640ffc:
//   slot 0  0x558770  scalar deleting destructor (uncarved entry)
//   slot 1  0x55d0f0  Dispose - the base cache-removal body
//   slot 2  _purecall - the resource-size query; concrete derived bodies
//                     return their fixed extent plus owned data bytes
// Complete adds the Dispose/GetSize virtual interface. Pinned DC resource
// types 0x1037/0x1dc9 (field lists 0x1860/0x1dca) have only destructor
// virtuals at slot 0, and ordinary AddRef/Release; neither added method is
// present. DC's full derived-class records also lack these overrides.
// Bitmap816's zBufferDraw starts at DC slot 1, shifted to retail slot 3,
// independently confirming the two inserted resource slots. The exact
// Windows-only filters cite each retained body's retail vtable slot.
class resource {
public:
    resource(const char* newName, EResourceType newType);
    virtual ~resource();  // slot 0

    DC_ADDRESS(0x0d9f94, 0x4)
    EResourceType get_resType() const { return resType; }

    DC_ADDRESS(0x074060, 0x6)
    const char* get_Name() const { return Name; }

    // E:\gamedcs\resrce.h:36
    DC_ADDRESS(0x122af0, 0x8)
    int AddRef() { return ++ReferenceCount; }

    DC_ADDRESS(0x122af8, 0x10)
    int Release()
    {
        if (ReferenceCount > 0)
            --ReferenceCount;
        return ReferenceCount;
    }
    // DC resource::GetReferenceCount is public const; retail disposal
    // callers test the reference-count field through this inline boundary.
    // Retail dispose paths test this reference-count field.
    // The DC declaration survives, but no body source location does.
    // Header ownership is provisional; no source order is claimed.
    // @dc-declaration-only: 0x185c
    int GetReferenceCount() const { return ReferenceCount; }

private:
    char Name[13];
    EResourceType resType;
    int ReferenceCount;

public:
    virtual void dispose();  // slot 1, base body 0x55d0f0
    virtual unsigned int getSize() const = 0;  // slot 2, pure at the base
};
SIZE(resource, 28);

#endif  /* HOMM3_RESRCE_H */
