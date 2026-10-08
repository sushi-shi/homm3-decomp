#ifndef HOMM3_SPELLDEFS_H
#define HOMM3_SPELLDEFS_H

#include <vector>

#include "armygrp.h"

// SpellDefs.h of the Loki port (RoE): 80 spell and creature-effect rows
// ("id >= 0 && id < kNumSpellsAndCreatureEffects", a SpellID enumerator).

// Dreamcast SpellDefs.h:345..346: original IsMindSpell.
inline bool IsMindSpell(int spell)
{
    return (akSpellTraits[spell].m_flags & 0x400) != 0;
}

// Spell-class flag roles in SSpellTraits::m_flags. Names are behavior-
// derived; values and mastery thresholds are byte-proven by
// SpellTargetsASingleArmy.
enum ESpellTargetFlags {
    SPELL_TARGET_ALWAYS_SINGLE = 0x10,
    SPELL_TARGET_MASS_AT_ADVANCED = 0x20,
    SPELL_TARGET_MASS_AT_EXPERT = 0x40,
    SPELL_TARGET_MARK_AREA = 0x280
};

bool SpellTargetsASingleArmy(int spell, int sslevel);
bool InitializeSpellTraitsTable();

#endif  /* HOMM3_SPELLDEFS_H */
