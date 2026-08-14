// ai_spellvalue.h - E:\gamedcs\ai_spellvalue.h, the AI spell-appraisal
// helper type.  HAND-OWNED: every member here is Dreamcast-attested and
// every offset is retail-byte-proven by the philai.cpp bodies below.
#ifndef HOMM3_AI_SPELLVALUE_H
#define HOMM3_AI_SPELLVALUE_H

#include <vector>
#include "armygrp.h"

class hero;

// Dreamcast type_creature_value (12 B): type at 0, value at 4, amount at
// 8.  Retail's get_enchantment_value indexes the vector with a 12-byte
// stride and reads +0 as the creature type and +4 as the value, which
// pins both the size and the first two fields; the constructor's
// `mov word ptr [..], cx` store proves the third is the 16-bit count.
// POD on purpose - AI_get_spell_value's inlined ~vector<> is a bare
// operator delete with no element loop, so the element type must have
// no destructor.
struct type_creature_value {
    TCreatureType type;
    long value;
    short amount;
};

// get_best_spell_value refuses to appraise any spell whose
// SSpellTraits::level is above 2 while the caster wields artifact 0x53 -
// i.e. the artifact that shuts level 3+ magic off. DC 83 is
// eArtifactRecantersCloak and NH3API agrees; ai_combat.cpp reaches the
// same identification from cast_spell and keeps its own TU-local
// constant, for the reason its comment gives - adding enumerators to
// armygrp.h/artifact.h measurably perturbs unrelated VC6 units through
// the shared type environment. This header is philai-only, so the name
// stays here rather than in the artifact domain's owner header; unifying
// the two spellings is a separate, measured decision.
const int ARTIFACT_RECANTERS_CLOAK = 0x53;

// SSpellTraits::field_c carries an AI VALUE-CLASS field in bits 15..20:
// get_raw_spell_value switches on `field_c & 0x1f8000` and dispatches one
// appraisal per value.  The names are role-derived from the arm each
// value selects (byte-proven at 0x5273d0); the values themselves are
// retail immediates.  AI_get_spell_value's own 0x38000 mask is exactly
// the three damage classes, which is why it gets its own name.
// ai_combat.cpp carries TU-local twins of the same six bits
// (AI_SPELL_DIRECT_DAMAGE .. AI_SPELL_CLASS_MASK) reached from
// cast_spell; the values agree bit for bit and the two rosters stay
// separate for the include-set reason its comment records.
enum ESpellValueClass {
    SPELL_VALUE_DAMAGE = 0x8000,
    SPELL_VALUE_DAMAGE_ONCE = 0x10000,
    SPELL_VALUE_MASS_DAMAGE = 0x20000,
    SPELL_VALUE_ENCHANTMENT = 0x40000,
    SPELL_VALUE_SUMMONING = 0x80000,
    SPELL_VALUE_SPECIAL = 0x100000,
    SPELL_VALUE_ANY_DAMAGE = 0x38000,
    SPELL_VALUE_CLASS_MASK = 0x1f8000
};

// Retail layout (36 B): the five Dreamcast scalars in DC order, then the
// creature-value vector.  DC reports 32 because its STLport vector is
// three pointers; VC6's Dinkumware vector carries the allocator byte
// first, so the retail object runs to 0x24 - which the constructor
// proves by storing the allocator at +0x14 and the three pointers at
// +0x18/+0x1c/+0x20.
class type_spellvalue {
public:
    type_spellvalue(const hero* new_hero);

    // ai_spellvalue.h:84 in the Dreamcast roster - the guard
    // AI_get_spell_value applies before appraising anything.
    unsigned char can_cast_spells() const { return power > 0; }

    long get_raw_spell_value(SpellID spell) const;
    long get_best_spell_value(long bits) const;

protected:
    // `mastery` is the Dreamcast TSkillMastery; that enum has no
    // retail-proven spelling in this tree yet, and hero::get_spell_level
    // - the only producer at every call site - already returns int, so
    // the parameter stays int and needs no cast.
    long get_damage_spell_value(SpellID spell, int mastery,
                                long times_castable, long combat_value) const;
    long get_mass_damage_spell_value(SpellID spell, int mastery,
                                     long times_castable) const;
    long get_enchantment_value(SpellID spell, int mastery,
                               long times_castable) const;

    const hero* our_hero;   // +0x00
    long stack_value;       // +0x04
    long power;             // +0x08
    long duration;          // +0x0c
    long mana;              // +0x10
    std::vector<type_creature_value> list;  // +0x14
};
SIZE(type_spellvalue, 0x24);

#endif  /* HOMM3_AI_SPELLVALUE_H */
