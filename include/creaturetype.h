// creaturetype.h - prototypes of creaturetype.cpp (compiland creaturetype.obj)
#ifndef HOMM3_CREATURETYPE_H
#define HOMM3_CREATURETYPE_H

#include "armygrp.h"

#include "creaturetype_fwd.h"

// Complete extends the Dreamcast creature-name domain through id 0x96.
// GetArmyName's retail range guard proves the inclusive upper bound.
const int g_creatureTypeLast = 0x96;

// E:\gamedcs\CreatureType.h:296. Complete retains the army.obj copy;
// events.cpp also expands this at monsters_flee/join/sell_out, passing a
// literal count so each singular/plural selection folds at its call site.
// DC observes the range guard at299, empty return301, else boundary303 and
// singular/plural selection305. A conditional return inside that else keeps
// the retained 62-byte body exact and recovers the nested name expansions in
// showCreatureSpellError and processNextAction with their canonical wrappers.
// Four forms were checked across all66 header consumers: this form has no
// score regressions; the early-return alternatives change later inline
// decisions in morale/failure-reason callers. Keep the guard's else scope.
VA(0x00440100, 0x3E)  // two-register /Gr ABI + trait lookup, dc 0x1ef94
inline const char* getArmyName(int type, int count)
{
    if (type < 0 || type > g_creatureTypeLast) {
        return DATA_COMPGEN(0x00691210, emptyCreatureName, "");
    } else {
        return count == 1 ? g_creatureTypeTraits[type].m_name
                          : g_creatureTypeTraits[type].m_pluralName;
    }
}

// --- globals ---
// CODEVIEW(E:\gamedcs\creaturetype.cpp:202, dc 0x718dc) TCreatureType GetBaseCreature(TTownType townType, int baseCreatureNbr);

// --- `anonymous namespace' ---
// CODEVIEW(E:\gamedcs\creaturetype.cpp:399, dc 0x71eec) void `anonymous namespace'::TAutoStrPtr::TAutoStrPtr();
// CODEVIEW(E:\gamedcs\creaturetype.cpp:402, dc 0x71ef4) void `anonymous namespace'::TAutoStrPtr::~TAutoStrPtr();
// CODEVIEW(E:\gamedcs\creaturetype.cpp:404, dc 0x71f0c) void `anonymous namespace'::TAutoStrPtr::set(char* pStr);
// CODEVIEW(E:\gamedcs\creaturetype.cpp:406, dc 0x71f10) char* `anonymous namespace'::TAutoStrPtr::get();

#endif  /* HOMM3_CREATURETYPE_H */
