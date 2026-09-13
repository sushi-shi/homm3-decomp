// creaturetype.h - prototypes of creaturetype.cpp (compiland creaturetype.obj)
#ifndef HOMM3_CREATURETYPE_H
#define HOMM3_CREATURETYPE_H

#include "armygrp.h"

#include "creaturetype_fwd.h"

// Complete extends the Dreamcast creature-name domain through id 0x96.
// GetArmyName's retail range guard proves the inclusive upper bound.
const int g_creatureTypeLast = 0x96;

inline const char* getArmyName(int type, int count)
{
    if (type < 0 || type > g_creatureTypeLast) {
        return DATA_COMPGEN(0x00691210, emptyCreatureName, "");
    } else {
        if (count == 1) {
            return g_creatureTypeTraits[type].m_name;
        } else {
            return g_creatureTypeTraits[type].m_pluralName;
        }
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
