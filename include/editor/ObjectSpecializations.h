// ObjectSpecializations.h - the specialized map objects (Loki h3maped
// ObjectSpecializations.cpp, object 21).
//
// Every specialization derives *virtually* from TGameObject: retail's
// vtables are __vt_<class>.11TGameObject with thunks at the TGameObject
// offset, the type_info functions call __rtti_class, and every constructor
// takes g++ 2.95's hidden in-charge int (the census's `(int, ...)`). The
// non-virtual part therefore starts with the virtual-base pointer; the
// offsets below are relative to it. The tiny in-class accessors are
// emitted with the vtable in object 21, the asserted setters out of line.
// Member names follow their accessors and the assert text; constructor
// parameter names are not recorded.
#ifndef HOMM3_EDITOR_OBJECTSPECIALIZATIONS_H
#define HOMM3_EDITOR_OBJECTSPECIALIZATIONS_H

#include <bitset>
#include <string>

#include "editor/GameObject.h"
#include "armygrp.h"
#include "artifact_type.h"
#include "primaryskill.h"
#include "secondaryskill.h"
#include "editor/Army.h"
#include "artifact.h"
#include "editor/GameResource.h"
#include "editor/Player.h"

class istream;
class ostream;
class TRawIStream;
class TRawOStream;

// Objects that only have a type (the vtable's own write and type name).
class TGenericObject : public virtual TGameObject {
public:
    TGenericObject(const TObjectType& objType);
    TGenericObject(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream) const;
    virtual string getTypeName() const;
};

// One name per creature bank kind (ObjectSpecializations.cpp's
// aCreatureBankTypeTraitsImp; the palette's tooltip).
enum {
    kNumCreatureBankTypes = 7
};

struct TCreatureBankTypeTraits {
    const char* m_name;
};

extern const TCreatureBankTypeTraits* akCreatureBankTypeTraits;

// One name per two-way and per one-way monolith kind (the object type's
// subtype; ObjectSpecializations.cpp's aMonolithTypeTraitsImp and
// aOneWayMonolithTypeTraitsImp, 12 bytes each). The bounds are the
// validation asserts'; the row type's name is not recorded.
enum {
    kNumMonolithTypes = 3,
    kNumOneWayMonolithTypes = 3
};

struct TMonolithTypeTraits {
    const char* m_name;
};

extern const TMonolithTypeTraits* akMonolithTypeTraits;
extern const TMonolithTypeTraits* akOneWayMonolithTypeTraits;

// Objects that can be flagged by a player (+4).
class TFlaggableObject : public virtual TGameObject {
public:
    TFlaggableObject(const TObjectType& objType, TPlayer owner);
    TFlaggableObject(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream) const;

    TPlayer getOwner() const { return _m_owner; }
    void setOwner(TPlayer newOwner) { _m_owner = newOwner; }

private:
    TPlayer _m_owner;
};

// A mine; its kind is the object type's subtype.
class TMine : public TFlaggableObject {
public:
    // One name per mine kind (the palette's tooltip).
    struct TTypeTraits {
        const char* m_name;
    };

    enum {
        s_kNumMineTypes = 8
    };

    static const TTypeTraits* s_akMineTypeTraits;

    static void initializeTypeTraitsTable();

    TMine(const TObjectType& objType, TPlayer owner = ePlayerNone);
    TMine(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual string getTypeName() const;
};

// An abandoned mine and the resources it may hold (+8).
class TAbandonedMine : public TMine {
public:
    TAbandonedMine(const TObjectType& objType);
    TAbandonedMine(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const { return _m_abPotentialResource != _s_kabDefaultPotentialResource; }

    bool getBIsPotentialResource(TGameResourceType type) const;
    void setBIsPotentialResource(TGameResourceType type, bool bIsPotential);
    unsigned int getNumPotentialResources() const { return _m_abPotentialResource.count(); }

private:
    static const bitset<kNumGameResourceTypes> _s_kabDefaultPotentialResource;

    bitset<kNumGameResourceTypes> _m_abPotentialResource;
};

// A creature generator; its kind is the object type's subtype.
class TGenerator : public TFlaggableObject {
public:
    // 8-byte rows of the two generator tables ("const struct
    // TGenerator::TGeneratorTypeTraits & TGenerator::getGeneratorTypeTraits()
    // const"); the name is the second word. The first is whether the
    // generator can be flagged (the constructors clear the owner when it
    // is not); its name is not recorded.
    struct TGeneratorTypeTraits {
        bool m_bFlaggable;
        const char* m_name;
    };

    enum TGenerator1Type {
    };

    enum TGenerator4Type {
    };

    enum {
        s_kNumGenerator1Types = 59,
        s_kNumGenerator4Types = 2
    };

    static const TGeneratorTypeTraits* s_akGenerator1TypeTraits;
    static const TGeneratorTypeTraits* s_akGenerator4TypeTraits;

    static void initializeTypeTraitsTables();

    TGenerator(const TObjectType& objType, TPlayer owner = ePlayerNone);
    TGenerator(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual string getTypeName() const;

    const TGeneratorTypeTraits& getGeneratorTypeTraits() const;
    TGenerator1Type getGenerator1Type() const;
    TGenerator4Type getGenerator4Type() const;
};

// A garrison (army +8); its kind is the object type's subtype.
class TGarrison : public TFlaggableObject {
public:
    // One name per garrison kind (the palette's tooltip).
    struct TTypeTraits {
        const char* m_name;
    };

    enum {
        s_kNumTypes = 2
    };

    static const TTypeTraits* s_akTypeTraits;

    static void initializeTypeTraitsTable();

    TGarrison(const TObjectType& objType, TPlayer owner = ePlayerNone);
    TGarrison(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const;
    virtual string getTypeName() const;

    int getType() const { return getExtra(); }
    const TTypeTraits& getTypeTraits() const { return s_akTypeTraits[getType()]; }
    const TArmy& getArmy() const { return _m_army; }
    void setArmy(const TArmy& newArmy);

private:
    TArmy _m_army;
};

// Objects with an owner (+4).
class TPlayableObject : public virtual TGameObject {
public:
    TPlayableObject(const TObjectType& objType, TPlayer owner = ePlayerNone);
    TPlayableObject(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream) const;

    TPlayer getOwner() const { return _m_owner; }
    void setOwner(TPlayer newOwner) { _m_owner = newOwner; }

private:
    TPlayer _m_owner;
};

// A pickable object with an optional message (+4) and guardians (custom
// flag +0x10, army +0x14).
class TTreasure : public virtual TGameObject {
public:
    static const unsigned int s_kMaxMessageLen = 300;

    TTreasure(const TObjectType& objType);
    TTreasure(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void importText(istream* pIStream);
    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const;
    virtual bool hasText() const { return !getMessage().empty(); }
    virtual void exportText(ostream* pOStream) const;

    const string& getMessage() const { return _m_message; }
    void setMessage(const string& newMessage);
    bool getBCustomGuardians() const { return _m_bCustomGuardians; }
    void setBCustomGuardians(bool bCustom) { _m_bCustomGuardians = bCustom; }
    const TArmy& getGuardians() const { return _m_guardians; }
    void setGuardians(const TArmy& newGuardians);

protected:
    void read(TRawIStream* pIStream, int version);

private:
    string _m_message;
    bool _m_bCustomGuardians;
    TArmy _m_guardians;
};

// An artifact on the map; its artifact is the object type's subtype.
class TGameArtifact : public TTreasure {
public:
    TGameArtifact(const TObjectType& objType);
    TGameArtifact(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual string getTypeName() const;

    TArtifact getArtifactType() const { return TArtifact(getExtra()); }
};

// A spell scroll (+0x4c).
class TSpellScroll : public TTreasure {
public:
    TSpellScroll(const TObjectType& objType);
    TSpellScroll(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const;

    SpellID getSpell() const { return _m_spell; }
    void setSpell(SpellID newSpell);

private:
    SpellID _m_spell;
};

// A resource pile (quantity +0x4c); its type is the object type's subtype.
class TGameResource : public TTreasure {
public:
    static const unsigned int s_kMaxQuantity = 99999;

    TGameResource(const TObjectType& objType);
    TGameResource(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const;
    virtual string getTypeName() const;

    TGameResourceType getResourceType() const { return TGameResourceType(getExtra()); }
    unsigned int getQuantity() const { return _m_quantity; }
    void setQuantity(unsigned int newQuantity);

private:
    unsigned int _m_quantity;
};

// A sign or ocean bottle with its text (+4).
class TSign : public virtual TGameObject {
public:
    static const unsigned int s_kMaxTextLen = 150;

    TSign(const TObjectType& objType);
    TSign(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void importText(istream* pIStream);
    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const { return !_m_text.empty(); }
    virtual bool hasText() const { return true; }
    virtual void exportText(ostream* pOStream) const;

    const string& getText() const { return _m_text; }
    void setText(const string& newText);

private:
    string _m_text;
};

// The scholar: what it teaches (+4) and the primary skill (+8), secondary
// skill (+0xc) or spell (+0x10). TRewardType's random and count names
// are the asserts'; the other three are named by role.
class TScholar : public virtual TGameObject {
public:
    enum TRewardType {
        eRewardRandom = -1,
        eRewardPrimarySkill = 0,
        eRewardSecondarySkill = 1,
        eRewardSpell = 2
    };

    enum {
        s_kNumRewardTypes = 3
    };

    TScholar(const TObjectType& objType);
    TScholar(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const { return _m_rewardType != eRewardRandom; }

    TRewardType getRewardType() const { return _m_rewardType; }
    void setRewardType(TRewardType newRewardType);
    TPrimarySkill getPrimarySkill() const { return _m_primarySkill; }
    void setPrimarySkill(TPrimarySkill newPrimarySkill);
    TSecondarySkill getSecondarySkill() const { return _m_secondarySkill; }
    void setSecondarySkill(TSecondarySkill newSecondarySkill);
    SpellID getSpell() const { return _m_spell; }
    void setSpell(SpellID newSpell);

private:
    TRewardType _m_rewardType;
    TPrimarySkill _m_primarySkill;
    TSecondarySkill _m_secondarySkill;
    SpellID _m_spell;
};

// The grail and its dig radius (+4).
class THolyGrail : public virtual TGameObject {
public:
    static const unsigned int s_kMaxRadius = 127;

    THolyGrail(const TObjectType& objType);
    THolyGrail(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const { return _m_radius != 0; }

    unsigned int getRadius() const { return _m_radius; }
    void setRadius(unsigned int newRadius);

private:
    unsigned int _m_radius;
};

// A shrine and its spell (+4).
class TShrine : public virtual TGameObject {
public:
    TShrine(const TObjectType& objType);
    TShrine(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const;

    int getSpellLevel() const;
    SpellID getSpell() const { return _m_spell; }
    void setSpell(SpellID newSpell);

private:
    SpellID _m_spell;
};

// The creature bank and monolith type tables, filled at start-up by
// cppbridge.cpp's init_objects (neither returns a value).
void InitializeCreatureBankTypeTraitsTable();
void InitializeMonolithTypeTraitsTables();

#endif  /* HOMM3_EDITOR_OBJECTSPECIALIZATIONS_H */
