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
// differs from the game's. The setters (0x44a544..0x44a747) do nothing on
// an equal value, so an unchanged prototype keeps sharing its copy.
class THeroPrototype {
public:
    // The longest name the map text's import keeps (h3maped 0x425267).
    enum { s_kMaxNameLen = 12 };

    // The hero's secondary skills: a count, then a skill and a mastery byte
    // each.
    class TSecondarySkills : public std::map<TSecondarySkill, TSkillMastery> {
    public:
        void read(TRawIStream* pIStream, int version);
        void write(TRawOStream* pOStream, int version) const;
    };

    // The worn artifact per slot (none: -1) and the backpack, a multiset
    // (0x5c; equality 0x428ac9). From map version 21 an artifact is a
    // short, before it a byte; from version 27 (the edition's Shadow of
    // Death) the misc slot 5 is stored too.
    class TArtifactContainer {
    public:
        TArtifactContainer() : _m_aSlot(ARTIFACT_NONE) {}
        TArtifactContainer(TRawIStream* pIStream, int version) : _m_aSlot(ARTIFACT_NONE)
        {
            read(pIStream, version);
        }

        void read(TRawIStream* pIStream, int version);
        void write(TRawOStream* pOStream, int version) const;

        TArtifact getSlot(TArtifactSlot slot) const { return _m_aSlot[slot]; }
        void setSlot(TArtifactSlot slot, TArtifact artifact) { _m_aSlot[slot] = artifact; }
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
        TSpells() {}
        TSpells(TRawIStream* pIStream, int version) { read(pIStream, version); }

        void read(TRawIStream* pIStream, int version);
        void write(TRawOStream* pOStream, int version) const;
    };

    // The four primary skills, a byte each in a map.
    class TPrimarySkills : public TArray<int, kNumPrimarySkills> {
    public:
        TPrimarySkills() {}
        TPrimarySkills(TRawIStream* pIStream, int version) { read(pIStream, version); }

        TPrimarySkills& operator=(const TArray<int, kNumPrimarySkills>& other)
        {
            TArray<int, kNumPrimarySkills>::operator=(other);
            return *this;
        }

        void read(TRawIStream* pIStream, int version);
        void write(TRawOStream* pOStream, int version) const;
    };

    void setName(const std::string& newName);
    void setBiography(const std::string& newBiography);
    void setSecondarySkills(const TSecondarySkills& newSecondarySkills);
    void setArtifacts(const TArtifactContainer& newArtifacts);
    void setSpells(const TSpells& newSpells);
    void setPrimarySkills(const TArray<int, kNumPrimarySkills>& newPrimarySkills);
    void setSex(int newSex);
    void setPortrait(int newPortrait);
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
    TBasicHero(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual THeroID getHeroID() const;
};

// A placeholder for a hero the campaign carries over: a hero id, or none
// and the power rank of the hero to place (the copy constructor 0x440342).
class THeroPlaceholder : public TBasicHero {
public:
    THeroPlaceholder(const TObjectType& objType, TPlayer owner);
    THeroPlaceholder(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual THeroID getHeroID() const;
    void setHeroID(THeroID newHeroID);

    virtual void write(TRawOStream* pOStream, int version) const;
    virtual bool isCustomized() const;

private:
    unsigned int _m_powerRank;
    THeroID _m_heroID;
};

// A hero on the map: linkable (+0) and a basic hero (+0xc), then what a
// map may customize (0x110 bytes before the vtordisp). The copy
// constructor 0x440a2b copies the members in this order. The reader
// (0x44b09a) reads each customization flag before its member: the first
// eight flags fill byte +0x18 in declaration order and the experience
// flag, added last, opens byte +0x19.
class THero : public TLinkableObject, public TBasicHero {
public:
    // One row per hero (16 bytes): the game's definition, the class, the
    // game versions that have the hero (indexed by the map's version,
    // 0x42373b) and whether the hero is a special one.
    struct TTraits {
        TTraits() : m_class(kNumHeroClasses), m_bSpecial(false) {}

        THeroPrototype m_prototype;
        THeroClass m_class;
        std::bitset<3> m_gameVersions;
        bool m_bSpecial;
    };

    // One row per class (0x1c bytes; constructor 0x44a882): the class's
    // object type, its town type, its name and the set of the class's
    // heroes, whose first member gives a hero of the class its default
    // portrait (0x44c0ec). A last row stands for the random hero.
    struct TClassTraits {
        TClassTraits(const TObjectType& objType, TTownType townType)
            : m_objType(objType), m_townType(townType), m_name(NULL) {}

        const TObjectType& m_objType;
        TTownType m_townType;
        const char* m_name;
        std::set<int> m_heroes;
    };

    // One row per secondary skill: its name (Loki's TSecondarySkillTraits;
    // copied from the game's skill traits, 0x44af1b).
    struct TSecondarySkillTraits {
        const char* m_name;
    };

    // h3maped 0x5857d4: points at the 156 hero rows (one per THeroID).
    static TTraits* s_akTraits;
    // h3maped 0x5857d8: points at the nineteen rows (one per THeroClass and
    // the random hero's).
    static TClassTraits* s_akClassTraits;
    // h3maped 0x5857dc: points at the 28 rows (one per secondary skill).
    static TSecondarySkillTraits* s_akSecondarySkillTraits;
    // One name per row of skilllev.txt, basic mastery first.
    struct TSkillMasteryTraits {
        const char* m_name;
    };

    // h3maped 0x59e66c: points at the three rows.
    static TSkillMasteryTraits* s_akSkillMasteryTraits;

    static void initialize();

    THero(const TObjectType& objType, TPlayer owner);

    // The hero's id as the map stores it, a byte (a random hero has none,
    // 0xff, and ignores a new one; 0x44bf12, 0x45726f).
    virtual void setStoredHeroID(unsigned char heroID) = 0;
    virtual unsigned char getStoredHeroID() const = 0;

    virtual void importText(std::istream* pIStream, EGameVersion version);
    virtual void write(TRawOStream* pOStream, int version) const;
    virtual bool isCustomized() const;
    virtual bool hasText() const { return getBCustomName() || getBCustomBiography(); }
    virtual void exportText(std::ostream* pOStream, EGameVersion version) const;

    bool getBCustomName() const { return _m_bCustomName; }
    bool getBCustomPortrait() const { return _m_bCustomPortrait; }
    bool getBCustomSecondarySkills() const { return _m_bCustomSecondarySkills; }
    void setBCustomSecondarySkills(bool bCustomSecondarySkills) { _m_bCustomSecondarySkills = bCustomSecondarySkills; }
    bool getBCustomArmy() const { return _m_bCustomArmy; }
    void setBCustomArmy(bool bCustomArmy) { _m_bCustomArmy = bCustomArmy; }
    bool getBCustomBiography() const { return _m_bCustomBiography; }
    void setBCustomBiography(bool bCustomBiography) { _m_bCustomBiography = bCustomBiography; }
    const std::string& getName() const { return _m_name; }
    int getPortrait() const { return _m_portrait; }
    const THeroPrototype::TSecondarySkills& getSecondarySkills() const { return _m_secondarySkills; }
    const TArmy& getArmy() const { return _m_army; }
    bool getBCustomSpells() const { return _m_bCustomSpells; }
    void setBCustomSpells(bool bCustomSpells) { _m_bCustomSpells = bCustomSpells; }
    const std::bitset<kNumSpells>& getSpells() const { return _m_spells; }
    bool getBCustomPrimarySkills() const { return _m_bCustomPrimarySkills; }
    void setBCustomPrimarySkills(bool bCustomPrimarySkills) { _m_bCustomPrimarySkills = bCustomPrimarySkills; }
    const TArray<int, kNumPrimarySkills>& getPrimarySkills() const { return _m_aPrimarySkill; }
    const std::string& getBiography() const { return _m_biography; }
    // h3maped 0x44b53a.
    void setArmy(const TArmy& newArmy);
    bool getBGroupedFormation() const { return _m_bGroupedFormation; }
    void setBGroupedFormation(bool bGroupedFormation) { _m_bGroupedFormation = bGroupedFormation; }

    void setName(const std::string& newName);
    void setPortrait(int newPortrait);
    void setSecondarySkills(const THeroPrototype::TSecondarySkills& newSecondarySkills);
    void setArtifacts(const THeroPrototype::TArtifactContainer& newArtifacts);
    void setBiography(const std::string& newBiography);
    void setSpells(const std::bitset<kNumSpells>& newSpells);
    void setPrimarySkills(const TArray<int, kNumPrimarySkills>& newPrimarySkills);
    void setExperience(int newExperience);
    void setSex(int newSex);

    // Whether the hero carries the artifact, worn or in the backpack (only
    // a hero with custom artifacts carries any).
    bool hasArtifact(TArtifact artifact) const;

protected:
    void read(TRawIStream* pIStream, int version);

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
public:
    TRandomHero(const TObjectType& objType, TPlayer owner);
    TRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void setStoredHeroID(unsigned char heroID) {}
    virtual unsigned char getStoredHeroID() const;
};

// A specific hero (RTTI TIdentifiedHero; its basic hero's slot 0 answers
// the id at +0x110, 0x44c03e).
class TIdentifiedHero : public THero {
public:
    TIdentifiedHero(const TObjectType& objType);
    TIdentifiedHero(const TObjectType& objType, TPlayer owner, THeroID heroID);

    virtual THeroID getHeroID() const;
    virtual void setStoredHeroID(unsigned char heroID);
    virtual unsigned char getStoredHeroID() const;
    virtual void setHeroID(THeroID newHeroID);

    THeroClass getHeroClass() const { return s_akTraits[_m_heroID].m_class; }

private:
    THeroID _m_heroID;
};

// A specific hero walking the map (RTTI TNonRandomHero): its portrait is
// its class's first hero's unless the map customizes it, and its type name
// is its class's.
class TNonRandomHero : public TIdentifiedHero {
public:
    TNonRandomHero(const TObjectType& objType, TPlayer owner, THeroID heroID);
    TNonRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual std::string getTypeName() const;
};

// A specific hero held in a prison (RTTI TPrison).
class TPrison : public TIdentifiedHero {
public:
    TPrison(const TObjectType& objType, THeroID heroID);
    TPrison(const TObjectType& objType, TRawIStream* pIStream, int version);
};

#endif  /* HOMM3_EDITOR_HERO_H */
