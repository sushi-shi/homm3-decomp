// Hero.h - the map's heroes (Hero.cpp; Loki h3maped object 15).
//
// Ported so far: the hero definition the map specifications customize and
// the per-class traits table the hero dialogs read. Each class row (0x1c
// bytes; h3maped's constructor 0x44a882) holds the class's object type,
// its town type, its name (copied from the game's class traits by the
// table's initializer 0x44a8b5) and the set of the class's heroes, whose
// first member the hero constructors read (0x44c0ec). Loki's RoE row kept
// a first hero id and a prototype array instead.
#ifndef HOMM3_EDITOR_HERO_H
#define HOMM3_EDITOR_HERO_H

#include <bitset>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "armygrp.h"
#include "artifact.h"
#include "herospec.h"
#include "objecttype.h"
#include "primaryskill.h"
#include "town_type.h"
#include "editor/Array.h"
#include "editor/Player.h"
#include "editor/RefCountingPtr.h"

// Complete's heroes: the map's hero tables hold one row per THeroID.
typedef int THeroID;
enum { kNumHeroes = 156 };

// A hero's definition: what the map specifications let a map customize
// per hero (0x54 bytes behind its handle). Equality (0x44a77a) compares
// every member; the map saves a hero only where it differs from the
// game's. The members' offsets come from that comparison, the hero
// settings writer (0x428892: experience +0x4c, skills, artifacts,
// biography, sex +0x44) and the available-heroes writer (0x423b20:
// portrait +0x48, players +0x50).
class THeroPrototype {
public:
    // The worn artifact per slot and the backpack (0x5c; equality 0x428ac9).
    class TArtifactContainer {
    public:
        TArtifact getSlot(TArtifactSlot slot) const { return _m_aSlot[slot]; }
        const std::vector<TArtifact>& getBackpack() const { return _m_backpack; }

        friend bool operator==(const TArtifactContainer& lhs, const TArtifactContainer& rhs);

    private:
        TArray<TArtifact, kNumArtifactSlots + 1> _m_aSlot;
        std::vector<TArtifact> _m_backpack;
    };

    typedef std::map<TSecondarySkill, TSkillMastery> TSecondarySkills;

    const std::string& getName() const { return _m_name; }
    const std::string& getBiography() const { return _m_biography; }

    friend bool operator==(const THeroPrototype& lhs, const THeroPrototype& rhs);

private:
    std::string _m_name;
    std::string _m_biography;
    TRefCountingPtr<TSecondarySkills> _m_pSecondarySkills;
    TRefCountingPtr<TArtifactContainer> _m_pArtifacts;
    std::bitset<kNumSpells> _m_spells;
    TArray<int, kNumPrimarySkills> _m_aPrimarySkill;
    int _m_sex;
    int _m_portrait;
    int _m_experience;
    TPlayerMask _m_availability;
};

// The handle the map keeps per hero (0x4 bytes; 0x432ac7 assigns it).
typedef TRefCountingPtr<THeroPrototype> TPHeroPrototype;

// Two handles' definitions are equal (h3maped 0x44a77a, cdecl).
bool operator==(const TPHeroPrototype& lhs, const TPHeroPrototype& rhs);

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
