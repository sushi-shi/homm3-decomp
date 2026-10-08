#ifndef HOMM3_ARTIFACT_H
#define HOMM3_ARTIFACT_H

#include <bitset>

#include "artifact_type.h"

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
// The artifact classes artraits.txt column 20 names by letter: S(pecial),
// T(reasure), N (minor), J (major), R(elic).
enum TArtifactClass {
    eArtifactClassSpecial = 1,
    eArtifactClassTreasure = 2,
    eArtifactClassMinor = 4,
    eArtifactClassMajor = 8,
    eArtifactClassRelic = 0x10
};

// RoE has 127 artifacts ("id >= 0 && id < kNumArtifacts").
enum {
    kNumArtifacts = 127
};

// Loki's 20-byte RoE record (Artifact.cpp): the allowed equipment slots are
// a bitset<kNumArtifactSlots> filled from artraits.txt columns 2..19.
struct TArtifactTraits {
    const char* m_name;                     // +0x00
    int m_cost;                             // +0x04
    std::bitset<kNumArtifactSlots> m_slots; // +0x08
    TArtifactClass m_class;                 // +0x0c
    const char* m_description;              // +0x10
};

// One slot name per row of artslots.txt.
struct TArtifactSlotTraits {
    const char* m_name;
};

extern const TArtifactSlotTraits* akArtifactSlotTraits;
extern const TArtifactTraits* akArtifactTraits;

bool InitializeArtifactTraitsTable();

// Whether an artifact may be worn in a slot. An inline of the editor's
// Artifact.h (its assert names the header); Hero.cpp owns the image's copy.
bool artifactAllowedInSlot(TArtifact artifact, TArtifactSlot slot);

#endif  /* HOMM3_ARTIFACT_H */
