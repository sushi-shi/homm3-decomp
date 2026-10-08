// Hero.h - the map's heroes (Hero.cpp; Loki h3maped object 15).
//
// Ported so far: the per-class traits table the hero dialogs read. Each
// class row (0x1c bytes; h3maped's constructor 0x44a882) holds the class's
// object type, its town type, its name (copied from the game's class
// traits by the table's initializer 0x44a8b5) and the set of the class's
// heroes, whose first member the hero constructors read (0x44c0ec).
// Loki's RoE row kept a first hero id and a prototype array instead.
#ifndef HOMM3_EDITOR_HERO_H
#define HOMM3_EDITOR_HERO_H

#include <set>

#include "objecttype.h"
#include "town_type.h"

class THero {
public:
    struct TClassTraits {
        TClassTraits(const TObjectType& objType, TTownType townType)
            : m_objType(objType), m_townType(townType), m_name(NULL) {}

        const TObjectType& m_objType;
        TTownType m_townType;
        const char* m_name;
        std::set<int> m_heroes;
    };

    // h3maped 0x5857d8: points at the eighteen rows (one per THeroClass).
    static TClassTraits* s_akClassTraits;
};

#endif  /* HOMM3_EDITOR_HERO_H */
