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

#include "artifact.h"
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
    // The worn artifact per slot and the backpack.
    class TArtifactContainer {
    public:
        TArtifactContainer();

        TArtifact getSlot(TArtifactSlot slot) const { return _m_aSlot[slot]; }
        void setSlot(TArtifactSlot slot, TArtifact newArtifact);
        const vector<TArtifact>& getBackpack() const { return _m_backpack; }
        vector<TArtifact>* getPBackpack() { return &_m_backpack; }
        void setBackpack(const vector<TArtifact>& newBackpack);

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
    void setPortrait(int newPortrait) { _m_portrait = newPortrait; }
    const map<TSecondarySkill, TSkillMastery>& getSecondarySkills() const { return _m_secondarySkills; }
    void setSecondarySkills(const map<TSecondarySkill, TSkillMastery>& newSecondarySkills);
    const TArmy& getArmy() const { return _m_army; }
    void setArmy(const TArmy& newArmy) { _m_army = newArmy; }
    const TArtifactContainer& getArtifacts() const { return _m_artifacts; }
    void setArtifacts(const TArtifactContainer& newArtifacts);

private:
    string _m_name;
    int _m_portrait;
    map<TSecondarySkill, TSkillMastery> _m_secondarySkills;
    TArmy _m_army;
    TArtifactContainer _m_artifacts;
};

// A set of hero classes, the random class included
// ("TSelectHeroClassDlg::TSelectHeroClassDlg(GtkWidget *, const THeroClassMask &)").
typedef bitset<kNumHeroClasses + 1> THeroClassMask;

class THero : public TPlayableObject {
public:
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

    struct TPrimarySkillTraits;
    // One name per row (the hero secondary skills page lists them).
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

    virtual void importText(istream* pIStream);
    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const;
    virtual bool hasText() const { return getBCustomName(); }
    virtual void exportText(ostream* pOStream) const;

    virtual void setProtoNum(unsigned int newProtoNum);
    virtual THeroClass getClass() const = 0;

    unsigned int getProtoNum() const { return _m_protoNum; }
    THeroID getIndivID() const { return THeroID(getClassTraits().m_firstHeroID + _m_protoNum); }
    const TClassTraits& getClassTraits() const;

    const string& getName() const;
    void setName(string newName);
    int getPortrait() const;
    void setPortrait(int newPortrait) { _m_customPrototype.setPortrait(newPortrait); }
    const map<TSecondarySkill, TSkillMastery>& getSecondarySkills() const;
    void setSecondarySkills(const map<TSecondarySkill, TSkillMastery>& newSecondarySkills)
    {
        _m_customPrototype.setSecondarySkills(newSecondarySkills);
    }
    const TArmy& getArmy() const;
    void setArmy(const TArmy& newArmy) { _m_customPrototype.setArmy(newArmy); }
    const THeroPrototype::TArtifactContainer& getArtifacts() const;
    void setArtifacts(const THeroPrototype::TArtifactContainer& newArtifacts)
    {
        _m_customPrototype.setArtifacts(newArtifacts);
    }
    bool getBHasArtifact(TArtifact artifact) const;

    bool getBCustomName() const { return _m_bCustomName; }
    void setBCustomName(bool bCustom) { _m_bCustomName = bCustom; }
    bool getBCustomPortrait() const { return _m_bCustomPortrait; }
    void setBCustomPortrait(bool bCustom) { _m_bCustomPortrait = bCustom; }
    bool getBCustomSecondarySkills() const { return _m_bCustomSecondarySkills; }
    void setBCustomSecondarySkills(bool bCustom) { _m_bCustomSecondarySkills = bCustom; }
    bool getBCustomArmy() const { return _m_bCustomArmy; }
    void setBCustomArmy(bool bCustom) { _m_bCustomArmy = bCustom; }
    bool getBCustomArtifacts() const { return _m_bCustomArtifacts; }
    void setBCustomArtifacts(bool bCustom) { _m_bCustomArtifacts = bCustom; }

    int getExperience() const { return _m_experience; }
    void setExperience(int newExperience);
    bool getBGroupedFormation() const { return _m_bGroupedFormation; }
    void setBGroupedFormation(bool bGrouped) { _m_bGroupedFormation = bGrouped; }
    int getPatrol() const { return _m_patrol; }
    void setPatrol(int newPatrol) { _m_patrol = newPatrol; }

    const string& getDefaultName() const;
    int getDefaultPortrait() const;
    const map<TSecondarySkill, TSkillMastery>& getDefaultSecondarySkills() const;
    const TArmy& getDefaultArmy() const;
    const THeroPrototype::TArtifactContainer& getDefaultArtifacts() const;
    int getDefaultExperience() const { return 0; }
    bool getDefaultBGroupedFormation() const { return false; }
    int getDefaultPatrol() const { return -1; }

    const string& getCustomName() const { return _m_customPrototype.getName(); }
    int getCustomPortrait() const { return _m_customPrototype.getPortrait(); }
    const map<TSecondarySkill, TSkillMastery>& getCustomSecondarySkills() const
    {
        return _m_customPrototype.getSecondarySkills();
    }
    const TArmy& getCustomArmy() const { return _m_customPrototype.getArmy(); }
    const THeroPrototype::TArtifactContainer& getCustomArtifacts() const { return _m_customPrototype.getArtifacts(); }

protected:
    void protectedSetProtoNum(unsigned int newProtoNum) { _m_protoNum = newProtoNum; }

private:
    const THeroPrototype& _getPrototype() const;

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
