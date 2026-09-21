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
    EResourceType getResType() const { return m_resType; }
    const char* getName() const { return m_name; }
    // E:\gamedcs\resrce.h:36, dc 0x122af0
    int addRef() { return ++m_referenceCount; }
    int release()
    {
        if (m_referenceCount > 0)
            --m_referenceCount;
        return m_referenceCount;
    }
    // DC resource::GetReferenceCount is public const; retail disposal
    // callers test the reference-count field through this inline boundary.
    // Retail dispose paths test this reference-count field.
    // The DC declaration survives, but no body source location does.
    // Header ownership is provisional; no source order is claimed.
    // @dc-declaration-only: 0x185c
    int getReferenceCount() const { return m_referenceCount; }

private:
    char m_name[13];
    EResourceType m_resType;
    int m_referenceCount;

public:
    virtual void dispose();  // slot 1, base body 0x55d0f0
    virtual unsigned int getSize() const = 0;  // slot 2, pure at the base
};
SIZE(resource, 28);

#endif  /* HOMM3_RESRCE_H */
