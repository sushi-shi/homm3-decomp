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

#include "armygrp.h"
#include "artifact_type.h"
#include "primaryskill.h"
#include "secondaryskill.h"
#include "editor/Army.h"
#include "editor/GameObject.h"
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

// Objects with an owner (+4).
class TPlayableObject : public virtual TGameObject {
public:
    TPlayableObject(const TObjectType& objType, TPlayer owner);
    TPlayableObject(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream) const;

    TPlayer getOwner() const { return _m_owner; }
    void setOwner(TPlayer newOwner) { _m_owner = newOwner; }

private:
    TPlayer _m_owner;
};

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

// A pickable object with an optional message (+4) and guardians (custom
// flag +0x10, army +0x14).
class TTreasure : public virtual TGameObject {
public:
    TTreasure(const TObjectType& objType);
    TTreasure(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void importText(istream* pIStream);
    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const;
    virtual bool hasText() const;
    virtual void exportText(ostream* pOStream) const;

    const string& getMessage() const { return _m_message; }
    void setMessage(const string& newMessage);
    bool getBCustomGuardians() const { return _m_bCustomGuardians; }
    void setBCustomGuardians(bool bCustom) { _m_bCustomGuardians = bCustom; }
    const TArmy& getGuardians() const { return _m_guardians; }
    void setGuardians(const TArmy& newGuardians) { _m_guardians = newGuardians; }

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

// The scholar: what it teaches (+4) and the primary skill (+8), secondary
// skill (+0xc) or spell (+0x10). TRewardType's enumerators are not
// recovered.
class TScholar : public virtual TGameObject {
public:
    enum TRewardType {
    };

    TScholar(const TObjectType& objType);
    TScholar(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const;

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

// A garrison (army +8); its kind is the object type's subtype.
class TGarrison : public TFlaggableObject {
public:
    struct TTypeTraits;

    static void initializeTypeTraitsTable();

    TGarrison(const TObjectType& objType, TPlayer owner);
    TGarrison(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const;
    virtual string getTypeName() const;

    int getType() const { return getExtra(); }
    const TTypeTraits& getTypeTraits() const;
    const TArmy& getArmy() const { return _m_army; }
    void setArmy(const TArmy& newArmy) { _m_army = newArmy; }

private:
    TArmy _m_army;
};

// A sign or ocean bottle with its text (+4).
class TSign : public virtual TGameObject {
public:
    TSign(const TObjectType& objType);
    TSign(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void importText(istream* pIStream);
    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const { return !getText().empty(); }
    virtual bool hasText() const { return true; }
    virtual void exportText(ostream* pOStream) const;

    const string& getText() const { return _m_text; }
    void setText(const string& newText);

private:
    string _m_text;
};

// A shrine and its spell (+4).
class TShrine : public virtual TGameObject {
public:
    TShrine(const TObjectType& objType);
    TShrine(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const;

    unsigned int getSpellLevel() const;
    SpellID getSpell() const { return _m_spell; }
    void setSpell(SpellID newSpell);

private:
    SpellID _m_spell;
};

// A creature generator; its kind is the object type's subtype.
class TGenerator : public TFlaggableObject {
public:
    struct TTypeTraits;

    static void initializeTypeTraitsTables();

    TGenerator(const TObjectType& objType, TPlayer owner);
    TGenerator(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual string getTypeName() const;

    const TTypeTraits& getGeneratorTypeTraits() const;
    int getGenerator1Type() const;
    int getGenerator4Type() const;
};

// A mine; its kind is the object type's subtype.
class TMine : public TFlaggableObject {
public:
    struct TTypeTraits;

    static void initializeTypeTraitsTable();

    TMine(const TObjectType& objType, TPlayer owner);
    TMine(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual string getTypeName() const;
};

// An abandoned mine and the resources it may hold (+8).
class TAbandonedMine : public TMine {
public:
    TAbandonedMine(const TObjectType& objType);
    TAbandonedMine(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const { return _m_abIsPotentialResource != _s_kabDefaultPotentialResource; }

    bool getBIsPotentialResource(TGameResourceType type) const;
    void setBIsPotentialResource(TGameResourceType type, bool bIsPotential);
    unsigned int getNumPotentialResources() const { return _m_abIsPotentialResource.count(); }

private:
    static const bitset<kNumGameResourceTypes> _s_kabDefaultPotentialResource;

    bitset<kNumGameResourceTypes> _m_abIsPotentialResource;
};

// The grail and its dig radius (+4).
class THolyGrail : public virtual TGameObject {
public:
    THolyGrail(const TObjectType& objType);
    THolyGrail(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream) const;
    virtual bool isCustomized() const { return _m_radius != 0; }

    unsigned int getRadius() const { return _m_radius; }
    void setRadius(unsigned int newRadius);

private:
    unsigned int _m_radius;
};

// The creature bank and monolith type tables, filled at start-up by
// cppbridge.cpp's init_objects (neither returns a value).
void InitializeCreatureBankTypeTraitsTable();
void InitializeMonolithTypeTraitsTables();

#endif  /* HOMM3_EDITOR_OBJECTSPECIALIZATIONS_H */
