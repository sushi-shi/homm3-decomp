#ifndef HOMM3_ARTIFACT_H
#define HOMM3_ARTIFACT_H

#include "va.h"

#include <bitset>

#include "artifact_type.h"
#include "artifact_data.h"

// Dreamcast's public wearable-position type. Complete adds a nineteenth
// equipped position, but retains the same dword parameter ABI and may pass
// that retail-only ordinal through functions which use this shared type.
enum TArtifactSlot {
    eArtifactSlotHead = 0,
    eArtifactSlotShoulders,
    eArtifactSlotNeck,
    eArtifactSlotRightHand,
    eArtifactSlotLeftHand,
    eArtifactSlotTorso,
    eArtifactSlotRightRing,
    eArtifactSlotLeftRing,
    eArtifactSlotFeet,
    eArtifactSlotMisc1,
    eArtifactSlotMisc2,
    eArtifactSlotMisc3,
    eArtifactSlotMisc4,
    eArtifactSlotWarMachine1,
    eArtifactSlotWarMachine2,
    eArtifactSlotWarMachine3,
    eArtifactSlotWarMachine4,
    eArtifactSlotSpellbook,
    kNumArtifactSlots,
    const_first_artifact_slot = eArtifactSlotHead
};

// The per-artifact traits record. The 32-byte STRIDE is byte-proven by
// hero::IsWieldingArtifact's `shl esi,5` index, and +0x18 by the same
// body: it holds the id of the COMBINATION artifact this piece belongs
// to, -1 when the artifact is not a component of one.
// The DC's own TArtifactTraits (NB11 member records) is only
// 20 bytes - m_name 0, m_cost 4, m_allowableSlotMask 8, m_class 12,
// m_description 16 - i.e. the AB-era record without the Shadow of Death
// combination column. Retail's artraits.txt parser at 0x44cd50 independently
// confirms every DC-era member at the same offset, then writes the four
// Complete-era fields at +0x14..+0x1d. hero::remove_artifact corroborates the
// allowable-slot class, combination indices and spell-list flag.
struct TArtifactTraits {
    // armyGroup::get_luck_description indexes artifact 0x55 at stride
    // 0x20 and passes +0 directly to format_string: the display name.
    const char* m_name;           // +0x00
    int m_cost;                   // +0x04
    // Equipped-slot class. remove_artifact compares this between the
    // combination artifact and each component, then uses it to index the
    // hero's per-slot equipped counts.
    int m_allowableSlotMask;      // +0x08
    int m_artifactClass;          // +0x0c
    const char* m_description;    // +0x10
    // comboType is set on an assembled artifact; targetCombo is set on each
    // component. Both are indices into gCombinationArtifacts, or -1.
    int m_comboType;              // +0x14
    int m_targetCombo;            // +0x18
    unsigned char m_disabled;     // +0x1c
    // Removing this artifact requires rebuilding the available-spell list.
    unsigned char m_givesSpells;      // +0x1d
    // Trailing alignment after the byte at +0x1d, rounding the proven
    // 0x20-byte artifact stride. NH3API explicitly leaves these two bytes unnamed.
    char m_paddingAfterGivesSpells[0x2];
};
SIZE(TArtifactTraits, 32);

// The retail artslots.txt table: 19 display names paired with the first of
// the 15 allowable-slot masks containing that physical equipment slot.
// Its 8-byte stride and both fields are written by 0x44cd50; the public name
// is preserved by the retail symbol at 0x660b64.
struct TArtifactSlotTraits {
    const char* m_name;
    int m_type;
};
SIZE(TArtifactSlotTraits, 8);

SIZE(TCombinationArtifact, 24);
SIZE(TArtifactSlotMask, 4);

// Cinit-owned tables consumed by artifact.cpp's ordinary source body.
DATA(0x00693898)
extern const TArtifactSlotMask g_artifactSlotMasks[15];
DATA(0x006938d8)
extern const TCombinationArtifact g_combinationArtifactTable[12];

// DC akArtifactTraits and akArtifactSlotTraits are references to const arrays
// of 127/18 records. Complete extends those domains to 144/19; its reference
// cells at 0x660b68/0x660b64 point to storage at 0x6939f8/0x694bf8.
// Preserve that reference-to-array interface with the Complete-era bounds.
// The combination table is Complete-only; its inferred pointer interface is
// independent of the two DC declarations. artifact.cpp owns all three tables.
extern const TArtifactTraits (&g_artifactTraits)[144];
extern const TCombinationArtifact* g_combinationArtifacts;
extern const TArtifactSlotTraits (&g_artifactSlotTraits)[19];

// Original: artifactAllowedInSlot; artifact.h:229
// DC233 indexes the artifact's bitset18 with operator[]. Complete replaces
// that inline mask with a slot-class index into bitset19; the primitive
// survives as the initial mask test in hero::heroFn004E2840 (0x4e2840).
// The richer hero member also checks displaced/combination artifacts.
// Retain the const mask reference: it preserves the retail exception-path
// value lifetime. Direct nested indexing expands _Eos instead of retaining
// its call (92.66% caller); this canonical reference form matches100%.
DC_ADDRESS(0x037d88, 0x2c)
inline unsigned char artifactAllowedInSlot(TArtifact artifact, TArtifactSlot slot)
{
    const std::bitset<19>& allowable =
        g_artifactSlotMasks[g_artifactTraits[artifact].m_allowableSlotMask];
    return allowable[slot];
}

// Retail .data 0x6aa9f8, defined by townmgr.cpp and consumed by the AI
// town-entry path. The record itself is completed by hero.h; an extern
// array of unknown bound can retain that single owning declaration here.
struct type_artifact;
extern const type_artifact g_blacksmithArtifacts[];

// Four signed primary-skill deltas per artifact. remove_artifact walks all
// 144 rows when dismantling a combination; the adjacent address is a real
// retail data symbol and is used as the pointer-loop bound.
extern const signed char g_artifactPrimarySkillBonuses[][4];
DATA(0x0063e998)
extern const signed char g_artifactPrimarySkillBonusesEnd[];

#endif  /* HOMM3_ARTIFACT_H */
