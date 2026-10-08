// Hero.h - heroes on the map (Loki h3maped Hero.cpp, object 15).
//
// THeroPrototype is a hero's identity and starting kit: name (+0),
// portrait (+0xc), secondary skills (+0x10), army (+0x1c) and artifacts
// (+0x54; 0xa8 bytes in all). THero is a TPlayableObject that keeps a
// custom prototype (+0xc) beside the flags saying which parts of it apply
// (+8), the prototype number within its class (+0xb4), experience (+0xb8),
// formation (+0xbc) and patrol radius (+0xc0); its own vtable (setProtoNum,
// pure getClass) follows at +0xc4. The class traits table holds, per hero
// class, the object type, the first hero ID, the town type, the number of
// prototypes and the prototypes themselves (THero::s_akClassTraits,
// asserts name m_objType, m_townType and m_numPrototypes). The field at
// +0xc is the class name: TSelectHeroClassDlg labels its list rows with it
// (the name m_name is not proven).
#ifndef HOMM3_EDITOR_HERO_H
#define HOMM3_EDITOR_HERO_H

#include <bitset>
#include <map>
#include <string>
#include <vector>

#include "artifact_type.h"
#include "herodefs.h"
#include "herospec.h"
#include "secondaryskill.h"
#include "town_type.h"
#include "editor/Army.h"
#include "editor/Array.h"
#include "editor/ObjectSpecializations.h"

class istream;
class ostream;
class TRawIStream;
class TRawOStream;

class THeroPrototype {
public:
    static const unsigned int s_kMaxNameLen = 12;
    static const unsigned int s_kMaxSecSkills = 8;
    static const unsigned int s_kMaxBackpackSize = 64;

    // The worn artifact per slot and the backpack.
    class TArtifactContainer {
    public:
        TArtifactContainer() : _m_aSlot(eArtifactNone) {}

        TArtifact getSlot(TArtifactSlot slot) const { return _m_aSlot[slot]; }
        void setSlot(TArtifactSlot slot, TArtifact artifact);
        const vector<TArtifact>& getBackpack() const { return _m_backpack; }
        vector<TArtifact>* getPBackpack() { return &_m_backpack; }
        void setBackpack(const vector<TArtifact>& newBackpack);

        friend bool operator==(const TArtifactContainer& lhs, const TArtifactContainer& rhs)
        {
            return lhs._m_aSlot == rhs._m_aSlot && lhs._m_backpack == rhs._m_backpack;
        }

    private:
        TArray<TArtifact, kNumArtifactSlots> _m_aSlot;
        vector<TArtifact> _m_backpack;
    };

    THeroPrototype();
    THeroPrototype(const string& name, int portrait, const map<TSecondarySkill, TSkillMastery>& secondarySkills,
                   const TArtifactContainer& artifacts);

    const string& getName() const { return _m_name; }
    void setName(string newName);
    int getPortrait() const { return _m_portrait; }
    void setPortrait(int newPortrait);
    const map<TSecondarySkill, TSkillMastery>& getSecondarySkills() const { return _m_secondarySkills; }
    void setSecondarySkills(const map<TSecondarySkill, TSkillMastery>& newSecondarySkills);
    const TArmy& getArmy() const { return _m_army; }
    void setArmy(const TArmy& newArmy);
    const TArtifactContainer& getArtifacts() const { return _m_artifacts; }
    void setArtifacts(const TArtifactContainer& newArtifacts);

private:
    string _m_name;
    int _m_portrait;
    map<TSecondarySkill, TSkillMastery> _m_secondarySkills;
    TArmy _m_army;
    TArtifactContainer _m_artifacts;
};

inline bool operator!=(const THeroPrototype::TArtifactContainer& lhs, const THeroPrototype::TArtifactContainer& rhs)
{
    return !(lhs == rhs);
}

// A set of hero classes, the random class included
// ("TSelectHeroClassDlg::TSelectHeroClassDlg(GtkWidget *, const THeroClassMask &)").
typedef bitset<kNumHeroClasses + 1> THeroClassMask;

// The in-class members come out of Hero.cpp in declaration order: the
// modifiers, the accessors, then hasText, protectedSetProtoNum and
// _getPrototype.
class THero : public TPlayableObject {
public:
    // "_m_backpackSize < THero::s_kMaxBackpackSize" (HeroPropsArtifactsPage.cpp).
    static const unsigned int s_kMaxBackpackSize = THeroPrototype::s_kMaxBackpackSize;
    static const int s_kMaxSecSkills = THeroPrototype::s_kMaxSecSkills;
    static const int s_kMaxExperience = 99999999;

    struct TClassTraits {
        TClassTraits(const TObjectType& objType, THeroID firstHeroID, TTownType townType)
            : m_objType(objType), m_firstHeroID(firstHeroID), m_townType(townType), m_name(NULL),
              m_numPrototypes(0), m_aPrototype(NULL) {}

        const TObjectType& m_objType;
        THeroID m_firstHeroID;
        TTownType m_townType;
        const char* m_name;
        unsigned int m_numPrototypes;
        THeroPrototype* m_aPrototype;
    };

    // One name per row of priskill.txt, secskill traits and skilllev.txt
    // (the hero pages list them).
    struct TPrimarySkillTraits {
        const char* m_name;
    };
    struct TSecondarySkillTraits {
        const char* m_name;
    };
    struct TSkillMasteryTraits {
        const char* m_name;
    };

    static TClassTraits* s_akClassTraits;
    static TPrimarySkillTraits* s_akPrimarySkillTraits;
    static TSecondarySkillTraits* s_akSecondarySkillTraits;
    static TSkillMasteryTraits* s_akSkillMasteryTraits;

    static void initialize();

    THero(const TObjectType& objType, TPlayer owner, unsigned int protoNum);
    THero(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void setProtoNum(unsigned int newProtoNum);
    void setName(string newName) { _m_customPrototype.setName(newName); }
    void setPortrait(int newPortrait) { _m_customPrototype.setPortrait(newPortrait); }
    void setSecondarySkills(const map<TSecondarySkill, TSkillMastery>& newSecondarySkills)
    {
        _m_customPrototype.setSecondarySkills(newSecondarySkills);
    }
    void setArmy(const TArmy& newArmy) { _m_customPrototype.setArmy(newArmy); }
    void setArtifacts(const THeroPrototype::TArtifactContainer& newArtifacts)
    {
        _m_customPrototype.setArtifacts(newArtifacts);
    }
    void setBCustomName(bool bCustom) { _m_bCustomName = bCustom; }
    void setBCustomPortrait(bool bCustom) { _m_bCustomPortrait = bCustom; }
    void setBCustomSecondarySkills(bool bCustom) { _m_bCustomSecondarySkills = bCustom; }
    void setBCustomArmy(bool bCustom) { _m_bCustomArmy = bCustom; }
    void setBCustomArtifacts(bool bCustom) { _m_bCustomArtifacts = bCustom; }
    void setExperience(int newExperience);
    void setBGroupedFormation(bool bGrouped) { _m_bGroupedFormation = bGrouped; }
    void setPatrol(int newPatrol) { _m_patrol = newPatrol; }

    virtual THeroClass getClass() const = 0;
    unsigned int getProtoNum() const { return _m_protoNum; }
    THeroID getIndivID() const { return THeroID(getClassTraits().m_firstHeroID + _m_protoNum); }
    const TClassTraits& getClassTraits() const { return s_akClassTraits[getClass()]; }
    const string& getName() const { return _m_bCustomName ? getCustomName() : getDefaultName(); }
    int getPortrait() const { return _m_bCustomPortrait ? getCustomPortrait() : getDefaultPortrait(); }
    const map<TSecondarySkill, TSkillMastery>& getSecondarySkills() const
    {
        return _m_bCustomSecondarySkills ? getCustomSecondarySkills() : getDefaultSecondarySkills();
    }
    const TArmy& getArmy() const { return _m_bCustomArmy ? getCustomArmy() : getDefaultArmy(); }
    const THeroPrototype::TArtifactContainer& getArtifacts() const
    {
        return _m_bCustomArtifacts ? getCustomArtifacts() : getDefaultArtifacts();
    }
    bool getBHasArtifact(TArtifact whichArtifact) const;
    bool getBCustomName() const { return _m_bCustomName; }
    bool getBCustomPortrait() const { return _m_bCustomPortrait; }
    bool getBCustomSecondarySkills() const { return _m_bCustomSecondarySkills; }
    bool getBCustomArmy() const { return _m_bCustomArmy; }
    bool getBCustomArtifacts() const { return _m_bCustomArtifacts; }
    int getExperience() const { return _m_experience; }
    bool getBGroupedFormation() const { return _m_bGroupedFormation; }
    int getPatrol() const { return _m_patrol; }

    const string& getDefaultName() const { return _getPrototype().getName(); }
    int getDefaultPortrait() const { return _getPrototype().getPortrait(); }
    const map<TSecondarySkill, TSkillMastery>& getDefaultSecondarySkills() const
    {
        return _getPrototype().getSecondarySkills();
    }
    const TArmy& getDefaultArmy() const { return _getPrototype().getArmy(); }
    const THeroPrototype::TArtifactContainer& getDefaultArtifacts() const { return _getPrototype().getArtifacts(); }
    const string& getCustomName() const { return _m_customPrototype.getName(); }
    int getCustomPortrait() const { return _m_customPrototype.getPortrait(); }
    const map<TSecondarySkill, TSkillMastery>& getCustomSecondarySkills() const
    {
        return _m_customPrototype.getSecondarySkills();
    }
    const TArmy& getCustomArmy() const { return _m_customPrototype.getArmy(); }
    const THeroPrototype::TArtifactContainer& getCustomArtifacts() const { return _m_customPrototype.getArtifacts(); }
    int getDefaultExperience() const { return 0; }
    bool getDefaultBGroupedFormation() const { return false; }
    int getDefaultPatrol() const { return -1; }

    virtual void importText(istream* pIStream);
    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const;
    virtual bool hasText() const { return _m_bCustomName; }
    virtual void exportText(ostream* pOStream) const;

protected:
    void protectedSetProtoNum(unsigned int newProtoNum) { _m_protoNum = newProtoNum; }

private:
    const THeroPrototype& _getPrototype() const { return s_akClassTraits[getClass()].m_aPrototype[_m_protoNum]; }

    bool _m_bCustomName : 1;
    bool _m_bCustomPortrait : 1;
    bool _m_bCustomSecondarySkills : 1;
    bool _m_bCustomArmy : 1;
    bool _m_bCustomArtifacts : 1;
    THeroPrototype _m_customPrototype;
    unsigned int _m_protoNum;
    int _m_experience;
    bool _m_bGroupedFormation;
    int _m_patrol;
};

// A specific hero; its class is the object type's subtype.
class TNonRandomHero : public THero {
public:
    TNonRandomHero(const TObjectType& objType, TPlayer owner, unsigned int protoNum);
    TNonRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual string getTypeName() const;
    virtual THeroClass getClass() const { return THeroClass(getExtra()); }
};

// A random hero (class kNumHeroClasses).
class TRandomHero : public THero {
public:
    TRandomHero(const TObjectType& objType, TPlayer owner);
    TRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual THeroClass getClass() const { return kNumHeroClasses; }
    virtual void setProtoNum() {}
};

// A hero in a prison, of any class (+0xc8).
class TPrison : public THero {
public:
    TPrison(const TObjectType& objType, THeroClass heroClass, unsigned int protoNum);
    TPrison(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual THeroClass getClass() const { return _m_class; }
    void setClass(THeroClass newClass);

private:
    THeroClass _m_class;
};

#endif  /* HOMM3_EDITOR_HERO_H */
