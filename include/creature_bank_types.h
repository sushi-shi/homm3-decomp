// creature_bank_types.h - shared retail creature-bank record layout.
#ifndef HOMM3_CREATURE_BANK_TYPES_H
#define HOMM3_CREATURE_BANK_TYPES_H

#include <va.h>
#include <vector>
#include "artifact.h"
#include "armygrp.h"

// Dreamcast CodeView names the reward tail and retail independently fixes all
// of its boundaries.  The army is 56 bytes; the seven-resource row occupies
// +0x38..+0x53; RandomizeEvents' implicit local constructor writes the byte at
// +0x5c and zeroes the three vector pointers at +0x60/+0x64/+0x68.  The PC
// vector is four bytes wider than Dreamcast STLport's, which accounts exactly
// for retail's 108-byte stride against the DC record's 104 bytes. Dreamcast's
// field record identifies resources as T_INT4[7], which also makes the row's
// proven AI_resource_cost(const int*) call type-correct. The three alignment
// bytes before artifacts stay implicit so generated copies skip them.
struct type_creature_bank {
    // Before normalization: guards.
    armyGroup m_guards;
    // Before normalization: resources.
    int m_resources[7];
    // Before normalization: reward_creature.
    TCreatureType m_rewardCreature;
    // Before normalization: reward_creatures.
    signed char m_rewardCreatures;
    // Before normalization: artifacts.
    std::vector<TArtifact> m_artifacts;
    // Implicit destructor; CodeView dc 0xbd58c compgenx.
    unsigned char load(void* infile);
};
SIZE(type_creature_bank, 0x6c);

#endif  // HOMM3_CREATURE_BANK_TYPES_H
