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
#include "editor/ObjectSpecializations.h"
#include "editor/Player.h"
#include "editor/RawStream.h"
#include "editor/RefCountingPtr.h"

// Complete's heroes: the map's hero tables hold one row per THeroID.
typedef int THeroID;
enum { kNumHeroes = 156 };

// A hero's definition: what the map specifications let a map customize
// per hero. A copy-on-write handle (4 bytes; the map keeps 156 of them and
// the hero traits rows one each) to its 0x54-byte implementation. Equality
// (0x44a77a) compares every member; the map saves a hero only where it
// differs from the game's. The members' offsets come from that comparison,
// the setters (0x44a582..0x44a724, each a no-op on an equal value), the hero
// settings writer (0x428892) and the available-heroes writer (0x423b20).
class THeroPrototype {
public:
    // The hero's secondary skills; it reads itself from a map (0x44a2a6).
    class TSecondarySkills : public std::map<TSecondarySkill, TSkillMastery> {
    public:
        void read(TRawIStream* pIStream, int version);
    };

    // The worn artifact per slot (none: -1) and the backpack, a multiset
    // (0x5c; equality 0x428ac9; constructed from a map by 0x426c7a).
    class TArtifactContainer {
    public:
        TArtifactContainer(TRawIStream* pIStream, int version) : _m_aSlot(ARTIFACT_NONE)
        {
            read(pIStream, version);
        }

        void read(TRawIStream* pIStream, int version);

        TArtifact getSlot(TArtifactSlot slot) const { return _m_aSlot[slot]; }
        const std::multiset<TArtifact>& getBackpack() const { return _m_backpack; }

        friend bool operator==(const TArtifactContainer& lhs, const TArtifactContainer& rhs);

    private:
        TArray<TArtifact, kNumArtifactSlots + 1> _m_aSlot;
        std::multiset<TArtifact> _m_backpack;
    };

    // The spells the hero knows (constructed from a map by 0x426cde).
    class TSpells : public std::bitset<kNumSpells> {
    public:
        TSpells(TRawIStream* pIStream, int version) { read(pIStream, version); }

        void read(TRawIStream* pIStream, int version);
    };

    // The four primary skills, a byte each in a map (0x44a4f3).
    class TPrimarySkills : public TArray<int, kNumPrimarySkills> {
    public:
        TPrimarySkills(TRawIStream* pIStream, int version);
    };

    void setBiography(const std::string& newBiography);
    void setSecondarySkills(const TSecondarySkills& newSecondarySkills);
    void setArtifacts(const TArtifactContainer& newArtifacts);
    void setSpells(const std::bitset<kNumSpells>& newSpells);
    void setPrimarySkills(const TArray<int, kNumPrimarySkills>& newPrimarySkills);
    void setSex(int newSex);
    void setExperience(int newExperience);
    void setAvailability(const TPlayerMask& newAvailability);

    const TPlayerMask& getAvailability() const { return _m_pImpl->m_availability; }

    friend bool operator==(const THeroPrototype& lhs, const THeroPrototype& rhs);

private:
    struct _TImpl {
        std::string m_name;
        std::string m_biography;
        TRefCountingPtr<TSecondarySkills> m_pSecondarySkills;
        TRefCountingPtr<TArtifactContainer> m_pArtifacts;
        std::bitset<kNumSpells> m_spells;
        TArray<int, kNumPrimarySkills> m_aPrimarySkill;
        int m_sex;
        int m_portrait;
        int m_experience;
        TPlayerMask m_availability;
    };

    TRefCountingPtr<_TImpl> _m_pImpl;
};

// What heroes and hero placeholders share: an owner and the hero they
// stand for (RTTI TBasicHero: its vtable, then the playable base; slot 0
// answers no hero, 0x44ad60).
class TBasicHero : public TPlayableObject {
public:
    TBasicHero(const TObjectType& objType, TPlayer owner);

    virtual THeroID getHeroID() const { return -1; }
};

// A placeholder for a hero the campaign carries over: a hero id, or none
// and the power rank of the hero to place (h3maped's constructor 0x44ad64
// sets rank 1 and no hero; the copy constructor 0x440342).
class THeroPlaceholder : public TBasicHero {
public:
    THeroPlaceholder(const TObjectType& objType, TPlayer owner);

    virtual THeroID getHeroID() const { return _m_heroID; }
    void setHeroID(THeroID newHeroID);

private:
    unsigned int _m_powerRank;
    THeroID _m_heroID;
};

// A hero on the map: linkable (+0) and a basic hero (+0xc).
class THero : public TLinkableObject, public TBasicHero {
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
