#ifndef HOMM3_HERO_TRAITS_H
#define HOMM3_HERO_TRAITS_H

#include "hero_class.h"
#include "creature_type.h"

// THeroTraits - the per-hero static-traits record, 92 B stride
// byte-proven by strip::DrawOwner 0x5aa060/0x5aa230-adjacent bodies:
// akHeroTraits[frame] is addressed as frame*23 dwords, and the +0x34
// dword rides a WIDGET_SET_IMAGE message, i.e. an image-name string
// (Dreamcast and NH3API name it m_large_portrait_name). The recovered
// members below follow their consumers and reference layouts; the PC-only
// four-byte slot at +0x3c remains unresolved.
struct THeroTraits {
public:
    int m_sex;  // +0x00 (DC m_sex)
    int m_race;  // +0x04 (DC m_race)
    THeroClass m_heroClass;  // +0x08 (DC m_class)
    int m_firstSkill;  // +0x0c (TSecondarySkill)
    int m_firstSkillLevel;  // +0x10 (TSkillMastery)
    int m_secondSkill;  // +0x14 (TSecondarySkill)
    int m_secondSkillLevel;  // +0x18 (TSkillMastery)
    unsigned char m_startsWithSpellbook;  // +0x1c
    // Dreamcast m_startsWithSpellbook is one byte at +0x1c, followed
    // by m_startingSpell at +0x20. Retail uses the byte flag; the intervening
    // three bytes align the spell ID, despite NH3API widening the flag to bool32.
    char m_paddingBeforeStartingSpell[3];
    int m_startingSpell;  // +0x20 (SpellID)
    TCreatureType m_firstStack;  // +0x24
    TCreatureType m_secondStack;  // +0x28
    TCreatureType m_thirdStack;  // +0x2c
    // UpdateHeroLocator sends this pointer to the portrait widget. Dreamcast
    // independently names the same +0x30 member m_small_portrait_name.
    const char* m_smallPortraitName;  // +0x30 image name for locator portraits
    const char* m_largePortraitName;  // +0x34 image name for WIDGET_SET_IMAGE
    // DC names/types the +0x38 word attributes (authored m_attributes).
    // Complete's RMG constructor
    // 0x537b10 independently reads +0x38/+0x39/+0x3a as original-map
    // availability, expansion-map availability and special-hero exclusion.
    // Preserve the proven word and expose the retail byte layout alongside
    // it; the fourth byte's role remains unknown. These role-derived byte
    // names and the union model are retail-supported, not recovered DC text.
    union {
        unsigned int m_attributes;
        struct {
            unsigned char m_availableInOriginal;
            unsigned char m_availableInExpansion;
            unsigned char m_special;
        } m_availability;
    };                                              // +0x38
    char m_pad3c[4];  // +0x3c retail-only field
    // HeroFn_004D8FB0 strcmp's the live hero name against this pointer.
    // InitializeHeroTraitsTable independently fills it from hotraits.txt.
    const char* m_defaultName;  // +0x40
    // Retail parses columns 1/2, 4/5, and 7/8 into these six dwords;
    // Dreamcast independently names the same three low/high stack pairs.
    int m_firstStackLow;  // +0x44
    int m_firstStackHigh;  // +0x48
    int m_secondStackLow;  // +0x4c
    int m_secondStackHigh;  // +0x50
    int m_thirdStackLow;  // +0x54
    int m_thirdStackHigh;  // +0x58
};

#endif
