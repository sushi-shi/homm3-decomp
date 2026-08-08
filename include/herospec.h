// herospec.h - the secondary-skill domain and the hero specialty table.
//
// A DEDICATED header rather than an addition to hero.h. hero.h sits
// inside game.h's include closure, which four compiled TUs pull in, and
// this tree has a byte-proven include-set sensitivity class (see the
// initialize_game_data note in docs/): adding TYPE DEFINITIONS to a
// widely-included header moves unrelated functions with no semantic
// change. Only hero.cpp needs these three declarations today, so they
// live where only hero.cpp sees them. Promoting them into hero.h is a
// measured decision, not a free one.
#ifndef HOMM3_HEROSPEC_H
#define HOMM3_HEROSPEC_H

#include <va.h>

// DC LF_ENUM `TSecondarySkill`, transcribed from
// evidence/dreamcast/enums.csv (31 enumerators; the DC spellings, minus
// the two aliases const_first_secondary_skill = 0 and
// kNumSecSkillsPerHero = 8 that name no distinct skill).
// FIVE of the values are retail-proven on this image, and each one
// lands where the DC roster puts it - which is what promotes the whole
// ladder from a transcription to a model. hero.obj's five specialty
// factor getters each read hero::skillLevel at a fixed offset and then
// require the hero's specialty record to name the SAME skill id:
//   Learning      21  0x4e4840 reads +0xde (0xc9 + 21), tests id 0x15
//   Offense       22  0x4e42b0 reads +0xdf (0xc9 + 22), tests id 0x16
//   Defense       23  0x4e4310 reads +0xe0 (0xc9 + 23), tests id 0x17
//   Intelligence  24  0x4e48b0 reads +0xe1 (0xc9 + 24), tests id 0x18
//   First Aid     27  0x4e4920 reads +0xe4 (0xc9 + 27), tests id 0x1b
// and the three slots hero.h already byte-proves independently -
// Wisdom 7, Ballistics 10, Eagle Eye 11 - agree with the same ladder.
enum TSecondarySkill {
    eSecSkillNone = -1,
    eSecSkillPathfinding = 0,
    eSecSkillArchery = 1,
    eSecSkillLogistics = 2,
    eSecSkillScouting = 3,
    eSecSkillDiplomacy = 4,
    eSecSkillNavigation = 5,
    eSecSkillLeadership = 6,
    eSecSkillWisdom = 7,
    eSecSkillMysticism = 8,
    eSecSkillLuck = 9,
    eSecSkillSiegeBallistics = 10,
    eSecSkillEagleEye = 11,
    eSecSkillNecromancy = 12,
    eSecSkillEstates = 13,
    eSecSkillSchoolOfFireMagic = 14,
    eSecSkillSchoolOfAirMagic = 15,
    eSecSkillSchoolOfWaterMagic = 16,
    eSecSkillSchoolOfEarthMagic = 17,
    eSecSkillMagicScholar = 18,
    eSecSkillBattleTactics = 19,
    eSecSkillBattlefieldBallistics = 20,
    eSecSkillLearning = 21,
    eSecSkillOffense = 22,
    eSecSkillDefense = 23,
    eSecSkillIntelligence = 24,
    eSecSkillSorcery = 25,
    eSecSkillMagicResistance = 26,
    eSecSkillFirstAid = 27,
    kNumSecSkills = 28
};

// DC LF_ENUM `TSkillMastery`. Retail corroborates the extent from the
// data side: every per-skill factor table in hero.obj's .rdata run
// (0x63e9e8 onward) is exactly FOUR floats wide, indexed by the
// skillLevel byte.
enum TSkillMastery {
    eMasteryInvalid = -1,
    eMasteryNone = 0,
    eMasteryBasic = 1,
    eMasteryAdvanced = 2,
    eMasteryExpert = 3,
    kNumMasteries = 4
};

// The hero-specialty record. DC public
// ?akHeroSpecificAbilities@@3AAY0IA@$$CBUTHeroSpecificAbility@@A - a
// REFERENCE to const THeroSpecificAbility[128], which is exactly how
// retail addresses it: `mov edx,[0x679c80]` loads the storage cell and
// then indexes it (the akHeroTraits reference-global precedent).
// The 40-byte stride is byte-proven by the five factor getters'
// `lea r,[id + 4*id]` / `lea r,[base + 8*r]` chain, and both modeled
// fields come from the same bodies: +0x00 is the specialty KIND (all
// five require it to be 0, the "secondary skill" kind) and +0x04 the
// TSecondarySkill it names. The remaining 32 bytes stay a pad.
// The kinds hero.obj's readers actually discriminate. Kept as plain
// ints in the record (retail reads a full dword either way) so a
// consumer can compare `type` against whichever domain the kind
// selects for `skill`.
enum THeroAbilityKind {
    // Byte-proven by all eight specialty getters in hero.obj: each one
    // requires kind 0 and then matches the record's second dword
    // against its OWN TSecondarySkill id.
    eHeroAbilitySecondarySkill = 0,
    // Byte-proven by hero::GetEstatesBonus (0x4e4390): kind 2 with the
    // subject dword equal to 6 - town.h's EGameResource GOLD - is
    // worth a flat +350 gold a day, i.e. the RESOURCE specialty.
    eHeroAbilityResource = 2,
    // Byte-proven by hero::GetHeroSpellBonus (0x4e5ff0): kind 3 with the
    // subject dword equal to the spell being cast selects the whole
    // per-spell bonus switch - the SPELL specialty.
    eHeroAbilitySpell = 3,
    // Proven only by hero::get_combat_speed_bonus (0x4e5aa0), which
    // adds +2 creature speed for kind 5 and does NOT look at the
    // subject dword at all. Role unattested - ORDINAL PLACEHOLDER
    // spelling (the WIDGET_RETURN_32 precedent).
    eHeroAbilityKind5 = 5
};

struct THeroSpecificAbility {
    int type;                   // +0x00 - a THeroAbilityKind
    TSecondarySkill skill;      // +0x04 - valid for kind 0
    char pad_08[0x20];
};
SIZE(THeroSpecificAbility, 40);

// Owner TU unlocated (the table is filled by
// InitializeHeroSpecificAbilitiesTable, dc 0xca728, which retail did
// not keep as a standalone body); extern only, no DATA claim - the
// gpWindowManager pattern.
extern const THeroSpecificAbility (&akHeroSpecificAbilities)[128];

#endif  /* HOMM3_HEROSPEC_H */
