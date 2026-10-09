// Monster.h - a wandering monster (Monster.cpp; Loki h3maped). On Windows
// a linkable object (RTTI TMonster <- TLinkableObject <- virtual
// TGameObject).
//
// Ported so far: the class the map's casts name, and the creature and
// quantity the victory condition page lists (the quantity follows the
// linkable object's id at +0xc).
#ifndef HOMM3_EDITOR_MONSTER_H
#define HOMM3_EDITOR_MONSTER_H

#include "armygrp.h"
#include "creaturetype_fwd.h"
#include "editor/ObjectSpecializations.h"

class TMonster : public TLinkableObject {
public:
    TCreatureType getCreatureType() const { return TCreatureType(getExtra()); }
    unsigned int getQuantity() const { return _m_quantity; }

private:
    unsigned int _m_quantity;
};

// The name of a number of creatures of a type, as the game spells it
// (getArmyName, creaturetype.h): nothing past the last type.
inline const char* getArmyName(TCreatureType type, unsigned int count)
{
    if (type < 0 || type > g_creatureTypeLast)
        return "";
    return count == 1 ? akCreatureTypeTraits[type].m_name : akCreatureTypeTraits[type].m_plural_name;
}

#endif  /* HOMM3_EDITOR_MONSTER_H */
