#ifndef HOMM3_SPELLDEFS_H
#define HOMM3_SPELLDEFS_H

#include <vector>

#include "armygrp.h"

// Dreamcast SpellDefs.h:345..346, dc 0x4fd34: original IsMindSpell.
// Its header definition and get_spell_work_chance line 505 establish the
// canonical accessor boundary; Complete expands this bit test in the caller.
inline unsigned char isMindSpell(int spell)
{
    return (g_spellTraits[spell].m_flags & 0x400) != 0;
}

// Retail spell-class flag roles in SSpellTraits::field_c. Names are
// behavior-derived; values and mastery thresholds are byte-proven by
// SpellTargetsASingleArmy.
enum ESpellTargetFlags {
    SPELL_TARGET_ALWAYS_SINGLE = 0x10,
    SPELL_TARGET_MASS_AT_ADVANCED = 0x20,
    SPELL_TARGET_MASS_AT_EXPERT = 0x40,
    // mark_area_highlights owns the rollover preview for the two area
    // traits carried together by this mask. Retail tests the pair as one
    // value before also admitting Berserk explicitly.
    SPELL_TARGET_MARK_AREA = 0x280
};

unsigned char spellTargetsASingleArmy(int spell, int sslevel);
unsigned char initializeSpellTraitsTable();

// Mutable implementation storage filled from sptraits.txt. The public
// akSpellTraits pointer/reference cell is at 0x687f58; retail writes this
// adjacent 81*136-byte backing array directly. Name is provisional because
// only the public DC array name survives.
extern SSpellTraits g_spellTraitsImp[81];

#endif  /* HOMM3_SPELLDEFS_H */
