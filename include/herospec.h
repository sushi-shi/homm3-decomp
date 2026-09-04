// herospec.h - the secondary-skill domain and the hero specialty table.
//
// A dedicated domain header shared by Hero.h, Army.h and the AI headers.
// Dreamcast CodeView records one TSkillMastery enum across those interfaces;
// keeping it here prevents the former per-subsystem int/enum twins from
// recreating contradictory source models.
#ifndef HOMM3_HEROSPEC_H
#define HOMM3_HEROSPEC_H

#include <va.h>
#include "secondaryskill.h"

// Five TSecondarySkill values are independently retail-proven here:
// hero.obj's specialty factor getters each read hero::skillLevel at a fixed
// offset and require the specialty record to name the same skill id:
//   Learning      21  0x4e4840 reads +0xde (0xc9 + 21), tests id 0x15
//   Offense       22  0x4e42b0 reads +0xdf (0xc9 + 22), tests id 0x16
//   Defense       23  0x4e4310 reads +0xe0 (0xc9 + 23), tests id 0x17
//   Intelligence  24  0x4e48b0 reads +0xe1 (0xc9 + 24), tests id 0x18
//   First Aid     27  0x4e4920 reads +0xe4 (0xc9 + 27), tests id 0x1b
// Wisdom 7, Ballistics 10 and Eagle Eye 11 agree with that shared ladder.

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
    // Byte-proven by hero::GetMobility (0x4e4990): kind 1 with the
    // subject dword equal to a stack's creature type - or to that
    // creature's UPGRADE - adds one to the stack's speed, i.e. the
    // CREATURE specialty. The subject shares the dword `skill` occupies
    // for kind 0.
    eHeroAbilityCreature = 1,
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
    ,
    // Two more kinds, both byte-proven by hero::HeroFn_004E6120
    // (0x4e6120), and both needed there as CASE LABELS - which is the
    // only reason they exist. Kind 4 shares kind 1's creature-match path
    // but takes the FLAT-bonus arm (+attack/+defense/+damage out of the
    // record's +0x08/+0x0c/+0x10) where kind 1 takes the scaled one;
    // kind 7 skips the creature match entirely and gates on the target's
    // own dragon attribute bit. NH3API spells them
    // SPECIALITY_CREATURE_UNIVERSAL and SPECIALITY_DRAGONS; only the
    // SPELLINGS are borrowed - the values and the behaviour are retail's.
    // Gated with the creature view because hero.obj is the only
    // compiland that names either.
    eHeroAbilityCreatureUniversal = 4,
    // Byte-proven by game::ViewArmy (0x4c6c50), which is the only body
    // in this tree that reaches kind 6: it requires the record's type to
    // be 6, matches the viewed stack against BOTH creature subjects
    // (+0x04 and +0x14) and against each one's UpgradedCreatureType, and
    // on a hit takes the upgrade target out of +0x18. That is the
    // creature-UPGRADE specialty - the hero who upgrades one ladder into
    // another for free.
    eHeroAbilityCreatureUpgrade = 6,
    eHeroAbilityDragons = 7
};

struct THeroSpecificAbility {
    int type;                   // +0x00 - a THeroAbilityKind
    // +0x04 under TWO domains. hero::GetMobility reads it as a CREATURE
    // for kind 1 and hands it to UpgradedCreatureType, which takes a
    // TCreatureType - so that arm needs a real lvalue of that type, not
    // a cast into an enum (a cleanliness floor at zero here). The
    // creature member is spelled with VC6's elaborated forward enum
    // because herospec.h does not include armygrp.h and several TUs
    // (advmgr.cpp, findpath.cpp) reach this header before it.
    union {
        TSecondarySkill skill;       // +0x04 - valid for kind 0
        enum TCreatureType creature; // +0x04 - valid for kind 1
    };
    // +0x08/+0x0c/+0x10, the FLAT creature bonuses kinds 4 and 7 add.
    // Byte-proven by hero::HeroFn_004E6120, which reads them at exactly
    // these displacements off the stride-40 row and adds the third to
    // BOTH damage bounds. Sliced out of the pad only in this view, so
    // findpath.cpp and game.cpp keep their declarator count unchanged;
    // SIZE below is unaffected. NH3API supplies the spellings.
    int creatureAttackBonus;    // +0x08
    int creatureDefenseBonus;   // +0x0c
    int creatureDamageBonus;    // +0x10
    // +0x14 / +0x18, sliced out of the pad 2026-08-20 by game::ViewArmy
    // (0x4c6c50) - the only reader of either. For kind 6 the record
    // names TWO creatures it will upgrade, the one at +0x04 and this
    // second one, and +0x18 is what both become. Elaborated enum for the
    // reason the union above is: herospec.h does not include armygrp.h.
    enum TCreatureType upgradeAlternateSubject;  // +0x14
    enum TCreatureType upgradeResult;            // +0x18
    // +0x1c, the one-line specialty label. Retail's own 17-byte getter at
    // 0x4d7220 is nothing but `return akHeroSpecificAbilities[id].<+0x1c>;`,
    // and THeroScreenWindow::SetupHeroView sprintf's it into widget 0x8b,
    // a 90x18 smalfont one-liner. The LONG description is a second char*
    // at +0x24 (read by THeroScreenWindow::WindowHandler at 0x4dd7af and
    // by 0x51f41e / 0x5b038f / 0x5b050a); it stays inside the trailing pad
    // until a body needs it. WHICH of the DC pair (GetSpecificAbilityText
    // / GetSpecificAbilityTextShort) owns which offset is an INFERENCE -
    // only "the 17-byte retail getter returns +0x1c" is proof, so the
    // name here is role-derived.
    const char* shortText;      // +0x1c
    char pad_20[0x4];
    // +0x24, the LONG description the note above predicted. Landed
    // 2026-08-20 by the consumer it names: THeroScreenWindow::
    // WindowHandler's specialty arm strcpy's exactly this displacement
    // into gText before the describe dialog. Sliced only in hero.obj's
    // view so no other compiland's declarator count moves.
    const char* longText;       // +0x24
};
SIZE(THeroSpecificAbility, 40);

// Owner TU unlocated (the table is filled by
// InitializeHeroSpecificAbilitiesTable, dc 0xca728, which retail did
// not keep as a standalone body); extern only, no DATA claim - the
// gpWindowManager pattern.
extern const THeroSpecificAbility (&akHeroSpecificAbilities)[128];

#endif  /* HOMM3_HEROSPEC_H */
