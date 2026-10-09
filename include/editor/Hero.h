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
#include "heroclass.h"
#include "herospec.h"
#include "objecttype.h"
#include "primaryskill.h"
#include "town_type.h"
#include "editor/Army.h"
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
    // The longest name the map text's import keeps (h3maped 0x425267).
    enum { s_kMaxNameLen = 12 };

    // The hero's secondary skills; it reads itself from a map (0x44a2a6).
    class TSecondarySkills : public std::map<TSecondarySkill, TSkillMastery> {
    public:
        void read(TRawIStream* pIStream, int version);
        void write(TRawOStream* pOStream, int version) const;
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
        void write(TRawOStream* pOStream, int version) const;

        TArtifact getSlot(TArtifactSlot slot) const { return _m_aSlot[slot]; }
        const std::multiset<TArtifact>& getBackpack() const { return _m_backpack; }

        friend bool operator==(const TArtifactContainer& lhs, const TArtifactContainer& rhs)
        {
            return lhs._m_aSlot == rhs._m_aSlot && lhs._m_backpack == rhs._m_backpack;
        }
        friend bool operator!=(const TArtifactContainer& lhs, const TArtifactContainer& rhs) { return !(lhs == rhs); }

    private:
        TArray<TArtifact, kNumArtifactSlots + 1> _m_aSlot;
        std::multiset<TArtifact> _m_backpack;
    };

    // The spells the hero knows (constructed from a map by 0x426cde).
    class TSpells : public std::bitset<kNumSpells> {
    public:
        TSpells(TRawIStream* pIStream, int version) { read(pIStream, version); }

        void read(TRawIStream* pIStream, int version);
        void write(TRawOStream* pOStream, int version) const;
    };

    // The four primary skills, a byte each in a map (0x44a4f3).
    class TPrimarySkills : public TArray<int, kNumPrimarySkills> {
    public:
        TPrimarySkills(TRawIStream* pIStream, int version);

        void write(TRawOStream* pOStream, int version) const;
    };

    void setBiography(const std::string& newBiography);
    void setSecondarySkills(const TSecondarySkills& newSecondarySkills);
    void setArtifacts(const TArtifactContainer& newArtifacts);
    void setSpells(const std::bitset<kNumSpells>& newSpells);
    void setPrimarySkills(const TArray<int, kNumPrimarySkills>& newPrimarySkills);
    void setSex(int newSex);
    void setPortrait(int newPortrait);
    void setName(const std::string& newName);
    void setExperience(int newExperience);
    void setAvailability(const TPlayerMask& newAvailability);

    const std::string& getBiography() const { return _m_pImpl->m_biography; }
    const TSecondarySkills& getSecondarySkills() const { return *_m_pImpl->m_pSecondarySkills; }
    const TArtifactContainer& getArtifacts() const { return *_m_pImpl->m_pArtifacts; }
    const TSpells& getSpells() const { return _m_pImpl->m_spells; }
    const TPrimarySkills& getPrimarySkills() const { return _m_pImpl->m_aPrimarySkill; }
    int getSex() const { return _m_pImpl->m_sex; }
    int getExperience() const { return _m_pImpl->m_experience; }
    const std::string& getName() const { return _m_pImpl->m_name; }
    int getPortrait() const { return _m_pImpl->m_portrait; }
    const TPlayerMask& getAvailability() const { return _m_pImpl->m_availability; }

    friend bool operator==(const THeroPrototype& lhs, const THeroPrototype& rhs);

private:
    struct _TImpl {
        std::string m_name;
        std::string m_biography;
        TRefCountingPtr<TSecondarySkills> m_pSecondarySkills;
        TRefCountingPtr<TArtifactContainer> m_pArtifacts;
        TSpells m_spells;
        TPrimarySkills m_aPrimarySkill;
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

// A hero on the map: linkable (+0) and a basic hero (+0xc), then what a
// map may customize (0x110 bytes before the vtordisp). The copy
// constructor 0x440a2b copies the members in this order. The stream
// constructor 0x44b09a reads each customization flag before its member:
// the first eight flags fill byte +0x18 in declaration order and the
// experience flag, added last, opens byte +0x19.
class THero : public TLinkableObject, public TBasicHero {
public:
    // One row per hero (16 bytes): the game's definition, the class, the
    // game versions that have the hero (indexed by the map's version,
    // 0x42373b) and whether the hero is a special one.
    struct TTraits {
        THeroPrototype m_prototype;
        THeroClass m_class;
        std::bitset<3> m_gameVersions;
        bool m_bSpecial;
    };

    struct TClassTraits {
        TClassTraits(const TObjectType& objType, TTownType townType)
            : m_objType(objType), m_townType(townType), m_name(NULL) {}

        const TObjectType& m_objType;
        TTownType m_townType;
        const char* m_name;
        std::set<int> m_heroes;
    };

    // h3maped 0x5857d4: points at the 156 hero rows (one per THeroID).
    static TTraits* s_akTraits;
    // One row per secondary skill: its name (Loki's TSecondarySkillTraits;
    // the hero table loader copies the skill traits' names in, 0x44af1b).
    struct TSecondarySkillTraits {
        const char* m_name;
    };

    // h3maped 0x5857d8: points at the eighteen rows (one per THeroClass).
    static TClassTraits* s_akClassTraits;
    // h3maped 0x5857dc: points at the 28 rows (one per secondary skill).
    static TSecondarySkillTraits* s_akSecondarySkillTraits;

    // The hero's id as the map stores it, a byte (a random hero has none,
    // 0xff, and ignores a new one; 0x44bf12, 0x45726f).
    virtual void setStoredHeroID(unsigned char heroID) = 0;
    virtual unsigned char getStoredHeroID() const = 0;

    bool getBCustomName() const { return _m_bCustomName; }
    bool getBCustomPortrait() const { return _m_bCustomPortrait; }
    bool getBCustomArmy() const { return _m_bCustomArmy; }
    void setBCustomArmy(bool bCustomArmy) { _m_bCustomArmy = bCustomArmy; }
    const std::string& getName() const { return _m_name; }
    int getPortrait() const { return _m_portrait; }
    const TArmy& getArmy() const { return _m_army; }
    // h3maped 0x44b53a.
    void setArmy(const TArmy& newArmy);
    bool getBGroupedFormation() const { return _m_bGroupedFormation; }
    void setBGroupedFormation(bool bGroupedFormation) { _m_bGroupedFormation = bGroupedFormation; }

private:
    bool _m_bCustomName : 1;
    bool _m_bCustomPortrait : 1;
    bool _m_bCustomSecondarySkills : 1;
    bool _m_bCustomArmy : 1;
    bool _m_bCustomArtifacts : 1;
    bool _m_bCustomBiography : 1;
    bool _m_bCustomSpells : 1;
    bool _m_bCustomPrimarySkills : 1;
    bool _m_bCustomExperience : 1;
    std::string _m_name;
    int _m_portrait;
    THeroPrototype::TSecondarySkills _m_secondarySkills;
    TArmy _m_army;
    THeroPrototype::TArtifactContainer _m_artifacts;
    std::string _m_biography;
    std::bitset<kNumSpells> _m_spells;
    TArray<int, kNumPrimarySkills> _m_aPrimarySkill;
    int _m_experience;
    bool _m_bGroupedFormation;
    int _m_patrolRadius;
    int _m_sex;
};

// A hero of the map's choosing (RTTI TRandomHero).
class TRandomHero : public THero {
};

// A specific hero (RTTI TIdentifiedHero; its basic hero's slot 0 answers
// the id at +0x110, 0x44c03e).
class TIdentifiedHero : public THero {
public:
    virtual THeroID getHeroID() const { return _m_heroID; }
    virtual void setStoredHeroID(unsigned char heroID) { setHeroID(heroID); }
    virtual unsigned char getStoredHeroID() const { return _m_heroID; }
    virtual void setHeroID(THeroID newHeroID) { _m_heroID = newHeroID; }

    THeroClass getHeroClass() const { return s_akTraits[_m_heroID].m_class; }

private:
    THeroID _m_heroID;
};

// A specific hero walking the map (RTTI TNonRandomHero).
class TNonRandomHero : public TIdentifiedHero {
};

// A specific hero held in a prison (RTTI TPrison).
class TPrison : public TIdentifiedHero {
};

#endif  /* HOMM3_EDITOR_HERO_H */
