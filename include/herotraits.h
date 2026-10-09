// herotraits.h - the static per-hero and per-hero-class traits tables
// (hero.cpp's records; herodefs.cpp defines the hero storage). Split from
// hero.h so the map editor's hero code can read them without the game's
// adventure-object headers.
#ifndef HOMM3_HEROTRAITS_H
#define HOMM3_HEROTRAITS_H

#include "va.h"

#include "armygrp.h"
#include "heroclass.h"
#include "herospec.h"
#include "primaryskill.h"
#include "secondaryskill.h"

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
    THeroClass m_class;  // +0x08 (DC m_class)
    TSecondarySkill m_1stSkill;  // +0x0c
    TSkillMastery m_1stSkillLevel;  // +0x10
    TSecondarySkill m_2ndSkill;  // +0x14
    TSkillMastery m_2ndSkillLevel;  // +0x18
    unsigned char m_startsWithSpellbook;  // +0x1c
    // Dreamcast m_startsWithSpellbook is one byte at +0x1c, followed
    // by m_startingSpell at +0x20. Retail uses the byte flag; the intervening
    // three bytes align the spell ID, despite NH3API widening the flag to bool32.
    char m_paddingBeforeStartingSpell[3];
    int m_startingSpell;  // +0x20 (SpellID)
    TCreatureType m_1stStack;  // +0x24
    TCreatureType m_2ndStack;  // +0x28
    TCreatureType m_3rdStack;  // +0x2c
    // UpdateHeroLocator sends this pointer to the portrait widget. Dreamcast
    // independently names the same +0x30 member m_small_portrait_name.
    const char* m_small_portrait_name;  // +0x30 image name for locator portraits
    const char* m_large_portrait_name;  // +0x34 image name for WIDGET_SET_IMAGE
    // DC names/types the +0x38 word attributes (authored m_attributes).
    // Complete's RMG constructor
    // 0x537b10 independently reads +0x38/+0x39/+0x3a as original-map
    // availability, expansion-map availability and special-hero exclusion.
    // Preserve the proven word and expose the retail byte layout alongside
    // it; the fourth byte's role remains unknown. These role-derived byte
    // names and the union model are retail-supported, not recovered DC text.
    union {
        unsigned int attributes;
        struct {
            unsigned char m_availableInOriginal;
            unsigned char m_availableInExpansion;
            unsigned char m_special;
        } m_availability;
        // The map editor indexes the first two by edition (h3maped
        // 0x41f2e0: `setge` + 0x38 into the record), Restoration of Erathia
        // first.
        unsigned char m_abAvailableIn[2];
    };                                              // +0x38
    char m_pad3c[4];  // +0x3c retail-only field
    // hero::getBiography strcmp's the live hero name against this pointer.
    // InitializeHeroTraitsTable independently fills it from hotraits.txt.
    const char* m_defaultName;  // +0x40
    // Retail parses columns 1/2, 4/5, and 7/8 into these six dwords;
    // Dreamcast independently names the same three low/high stack pairs.
    int m_1stStackLow;  // +0x44
    int m_1stStackHigh;  // +0x48
    int m_2ndStackLow;  // +0x4c
    int m_2ndStackHigh;  // +0x50
    int m_3rdStackLow;  // +0x54
    int m_3rdStackHigh;  // +0x58
};
SIZE(THeroTraits, 0x5c);

// Retail's eighteen 64-byte hero-class rows. The class-name getter reaches
// only the pointer at +4; cursor rendering independently proves the eighteen
// class extent.
struct THeroClassTraits {
public:
    int m_townType;  // +0x00
    const char* m_name;  // +0x04
    float m_aggression;  // +0x08
    signed char m_initialPrimarySkill[kNumPrimarySkills];  // +0x0c
    signed char m_gainPrimarySkillChance[kNumPrimarySkills];  // +0x10
    signed char m_gainPrimarySkillChance10P[kNumPrimarySkills];  // +0x14
    signed char m_gainSecondarySkillChance[kNumSecSkills];  // +0x18
    signed char m_foundInTownType[9];  // +0x34
    // Complete expands foundInTownType to nine bytes at +0x34.
    // NH3API confirms the three trailing alignment bytes and 0x40-byte PC stride.
    char m_paddingAfterTownChances[3];
};
SIZE(THeroClassTraits, 0x40);

extern THeroClassTraits g_heroClassTraits[kNumHeroClasses];
extern const THeroClassTraits (&akHeroClassTraits)[18];

// Retail .data 0x67dce8 (reloc-evidence datum; read by strip::DrawOwner
// as pointer+index). The IDA-lineage mangling
// ?akHeroTraits@@3AAY0KD@$$CBUTHeroTraits@@A types it as an array
// reference. The parser loads the first 156 playable heroes at stride 0x5c.
// Seven additional portrait records follow at 0x67d5e0, ending at 0x67d864
// before the aligned class table at 0x67d868. The complete array has 163 rows;
// the reference cell points to 0x679dd0.
extern THeroTraits g_heroTraitsStorage[163];
extern const THeroTraits (&akHeroTraits)[163];

#endif  /* HOMM3_HEROTRAITS_H */
