#ifndef HOMM3_SPELL_TRAITS_H
#define HOMM3_SPELL_TRAITS_H

#include "spellschool.h"
#include "spelleffect_type.h"

// Bootstrap VIEW of the spell-traits record (136-byte stride proven
// by get_spell_work_chance's spell*17*8 indexing at 0x44a4e2): only
// the fields that function reads are modeled; the full roster gets
// its own header when spell work begins in earnest.
struct SSpellTraits {
    int m_karma;              // <= 0 short-circuits to certain-work
    // DC TSpellTraits.m_sample (members.csv TSpellTraits@4) - the WAV
    // this spell plays. army::do_fire_shield (0x4409c0) is the witness:
    // it hands `akSpellTraits[SPELL_FIRE_SHIELD].m_sample` straight to
    // LoadPlaySample as `[akSpellTraits + 29*136 + 4]`. The record's
    // first seven fields are UNSHIFTED against the DC roster (m_flags
    // 12/+0xc, m_name 16/+0x10, m_abbreviated_name 20/+0x14,
    // m_level 24/+0x18 all already agree here), so 4 is 4.

    // +0x08 IS THE DC's m_effect, sliced 2026-08-20: the SPRITE-EFFECT
    // id this spell plays over its target. combatManager::AreaEffect
    // (0x5a4970) is the witness - it loads
    // `[akSpellTraits + spell*136 + 8]` and hands it straight to
    // drawing.obj's hex-taking SpellEffect overload (0x496a10) as the
    // effect argument, exactly as do_fire_shield hands +0x04 to
    // LoadPlaySample.  UNGATED 2026-08-20 by the view audit; both names
    // are byte-proven, so both are declared for everyone.
    const char* m_sample;     // +0x04
    // DC TSpellTraits record 0x1f65: member type 0x1f15, TSpellEffectID.
    TSpellEffectID m_effect;  // +0x08, int-wide enum
    unsigned int m_flags;     // bit 10 gates one immunity family;
                              // bit 12 (byte +0xd & 0x10) blocks the
                              // spell against siege weapons
    // +0x10 is the display name. SetShrineHelpText passes it as the string
    // argument to the central shrine format after indexing this 136-byte row.
    const char* m_name;
    // Dreamcast-attested m_abbreviated_name; retail's loader duplicates
    // sptraits.txt column 1 into this pointer.
    const char* m_abbreviatedName;
    int m_level;                // the dragons' magic-immunity gate
    union {
        TSpellSchool m_school;  // typed consumer view
        unsigned int m_schoolBits;  // loader's OR-accumulator view
    };                        // +0x1c
    // +0x20, the per-mastery MANA COST row: GetManaCost indexes it
    // `[base + 4*(mastery + spell*34) + 0x20]` with the mastery
    // get_spell_level just returned.
    int m_manaCost[4];         // +0x20
    // Spell-power multiplier (NH3API m_power_factor): multiplied by the
    // caster's power in get_resurrection_value (0x423d60),
    // get_mass_damage_value (0x42540a) and ai_tactical's
    // get_damage_spell_value (0x436f60: traits[spell*136 + 0x30]).
    int m_powerFactor;         // +0x30
    // Per-mastery flat bonus row (NH3API m_mastery_bonus):
    // get_damage_spell_value adds [spell*136 + mastery*4 + 0x34].
    int m_masteryBonus[4];     // +0x34
    // +0x44, nine faction weights used by town::initialize_spells.
    int m_townProbability[9];
    // A SECOND per-mastery dword row: get_enchantment_value indexes it
    // as spell*34 + mastery dwords from the table base (0x423cab) =
    // record +0x68 + mastery*4. Distinct from mastery_bonus - both
    // rows are byte-proven by their own consumers. Name provisional.
    int m_masteryValues[4];    // +0x68
    // Complete's four per-mastery descriptions. Dreamcast names this
    // m_description at +0x74 before the added ninth town-probability dword;
    // retail InitializeSpellTraits writes the shifted +0x78 row.
    const char* m_levelDescriptions[4];  // +0x78
};

#endif
