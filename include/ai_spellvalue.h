// ai_spellvalue.h - E:\gamedcs\ai_spellvalue.h, the AI's per-hero spell
// valuer.
// HAND-OWNED. The Dreamcast roster gives this class its own header of
// exactly this name (cited by set_stack_value at ai.cpp:2457,
// can_cast_spells / set_power at ai_player.cpp:1756/1763 and get_mana /
// set_mana at philai.cpp:914/921), so the type lives here rather than in
// any one consumer's header. Every member below is Dreamcast-attested and
// every offset is retail-byte-proven by the philai.cpp bodies.
#ifndef HOMM3_AI_SPELLVALUE_H
#define HOMM3_AI_SPELLVALUE_H

#include <va.h>
#include <vector>
#include "armygrp.h"
// The value list's element. The Dreamcast puts it in its own
// ai_creature_value.h; this tree models it in ai_player.h, which is where
// the include comes from until someone splits it out. Its `type` is a
// plain int because ai_player.cpp's calculate_reserve indexes the list BY
// creature ordinal and stores the loop counter straight into it;
// philai.cpp bridges the int/TCreatureType crossing at its one call site
// with the bit-preserving inline it already uses for armyGroup's roster.
#include "ai_player.h"

class hero;

// get_best_spell_value refuses to appraise any spell whose
// SSpellTraits::level is above 2 while the caster wields artifact 0x53 -
// i.e. the artifact that shuts level 3+ magic off. DC 83 is
// eArtifactRecantersCloak and NH3API agrees; ai_combat.cpp reaches the
// same identification from cast_spell and keeps its own TU-local
// constant, for the reason its comment gives - adding enumerators to
// armygrp.h/artifact.h measurably perturbs unrelated VC6 units through
// the shared type environment. Unifying the two spellings is a separate,
// measured decision.
// Before normalization: ARTIFACT_RECANTERS_CLOAK.
const int g_artifactRecantersCloak = 0x53;

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
//
// The size is corroborated FRAME-SIDE by a second body:
// combatManager::has_ranged_advantage (0x420a80) builds one as a local at
// [ebp-0x50] under a `sub esp,0x44` frame whose other ten dwords are all
// accounted for, which fixes sizeof at exactly 0x24; the same body pins
// stack_value at +0x04 (it writes the side's total combat value there
// before asking for a spell value) and the vector at +0x14 through the
// compiler-generated destructor it INLINES - `operator delete(+0x18)`
// followed by zero stores to +0x18/+0x1c/+0x20.
class type_spellvalue {
public:
    // Before normalization (locals): new_hero.
    type_spellvalue(const hero* newHero);
    // Non-trivial solely because of list. Keeping the empty body inline
    // preserves the compiler-generated expansion at each use while giving
    // ai.cpp a source spelling for retail's retained COMDAT copy.
    ~type_spellvalue() {}

    // ai_spellvalue.h:84 in the Dreamcast roster - the guard
    // AI_get_spell_value applies before appraising anything.
    // Before normalization (function): type_spellvalue::can_cast_spells.
    unsigned char canCastSpells() const { return m_power > 0; }
    // DC ai_spellvalue.h:114 - the one-store setter, inlined at both
    // type_school_artifact::get_value call sites in retail.
    // Before normalization (function): type_spellvalue::set_power.
    void setPower(long arg) { m_power = arg; }
    // DC ai_spellvalue.h:124 - the one-store setter, inlined at every
    // retail call site (dc 0x27c74 is the 4-byte out-of-line copy).
    // combatManager::do_combat_ai is a located caller: it writes the
    // side's whole combat value here before asking for a spell value.
    // Before normalization (function): type_spellvalue::set_stack_value.
    void setStackValue(long arg) { m_stackValue = arg; }
    // DC ai_spellvalue.h:99/119 - the mana pair (dc 0x114bdc/0x114be0
    // are philai.obj's 4-byte out-of-line copies).  Retail
    // AI_set_hero_bonuses (0x527760) is the byte-proven consumer: it
    // reseeds the valuer from hero::mana and reads the initial pool for
    // the well/spring valuations.
    // Before normalization (function): type_spellvalue::get_mana.
    long getMana() const { return m_mana; }
    // Before normalization (function): type_spellvalue::set_mana.
    void setMana(long arg) { m_mana = arg; }
    // E:\gamedcs\philai.cpp:1699 (dc 0x10fe64) - the what-if probe:
    // bump power/duration/mana, re-ask get_best_spell_value, restore,
    // return the delta against the caller's baseline.  DEFINED in
    // philai.cpp as the DC build does; retail keeps no out-of-line row
    // (AI_set_hero_bonuses expands it at all six probe sites).
    // Before normalization (function): type_spellvalue::get_value_of_increase.
    // Before normalization (locals): base_value, power_change, duration_change, mana_change.
    long getValueOfIncrease(long baseValue, long powerChange,
                               long durationChange, long manaChange);

    // Before normalization (function): type_spellvalue::get_raw_spell_value.
    long getRawSpellValue(SpellID spell) const;
    // Before normalization (function): type_spellvalue::get_best_spell_value.
    long getBestSpellValue(long bits) const;

protected:
    // `mastery` is the Dreamcast TSkillMastery; that enum has no
    // retail-proven spelling in this tree yet, and hero::get_spell_level
    // - the only producer at every call site - already returns int, so
    // the parameter stays int and needs no cast.
    // Before normalization (function): type_spellvalue::get_damage_spell_value.
    long getDamageSpellValue(SpellID spell, int mastery,
                                // Before normalization (locals): times_castable, combat_value.
                                long timesCastable, long combatValue) const;
    // Before normalization (function): type_spellvalue::get_mass_damage_spell_value.
    long getMassDamageSpellValue(SpellID spell, int mastery,
                                     // Before normalization (locals): times_castable.
                                     long timesCastable) const;
    // Before normalization (function): type_spellvalue::get_enchantment_value.
    long getEnchantmentValue(SpellID spell, int mastery,
                               // Before normalization (locals): times_castable.
                               long timesCastable) const;
    // E:\\gamedcs\\philai.cpp:1610. Complete expands this one-call helper
    // into the constructor, but the Dreamcast member boundary and local
    // inventory remain authoritative source-shape evidence.
    // Before normalization (function): type_spellvalue::fill_creature_value_list.
    void fillCreatureValueList();

    // Before normalization: our_hero.
    const hero* m_ourHero;   // +0x00
    // Before normalization: stack_value.
    long m_stackValue;       // +0x04
    // Before normalization: power.
    long m_power;             // +0x08
    // Before normalization: duration.
    long m_duration;          // +0x0c
    // Before normalization: mana.
    long m_mana;              // +0x10
    // Before normalization: list.
    std::vector<type_creature_value> m_list;  // +0x14
};
SIZE(type_spellvalue, 0x24);

#endif  /* HOMM3_AI_SPELLVALUE_H */
