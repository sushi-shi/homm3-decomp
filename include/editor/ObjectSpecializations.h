// ObjectSpecializations.h - the map objects that carry properties of
// their own (ObjectSpecializations.cpp; Loki h3maped object 21). Each
// derives virtually from TGameObject: its vbptr comes first, then its own
// members, then a vtordisp dword before the virtual TGameObject (the
// constructors store it).
//
// The release drops Loki's asserts, so the setters only store. The
// Windows editors read and write by the map's version and edition: the
// stream constructors take the map file's version, the writers the
// edition.
#ifndef HOMM3_EDITOR_OBJECTSPECIALIZATIONS_H
#define HOMM3_EDITOR_OBJECTSPECIALIZATIONS_H

#include <bitset>
#include <string>

#include "va.h"
#include "armygrp.h"
#include "artifact_type.h"
#include "primaryskill.h"
#include "secondaryskill.h"
#include "editor/Army.h"
#include "editor/GameObject.h"
#include "editor/GameResource.h"
#include "editor/Player.h"
#include "town_type.h"

class TRawIStream;
class TRawOStream;

class THero;

// The creature bank, monolith and keymaster's tent name tables, filled at
// start-up from crbanks.txt, monolith.txt and TentColr.txt (the tent
// colours lowercased).
enum {
    kNumCreatureBankTypes = 7,
    kNumMonolithTypes = 8,
    kNumOneWayMonolithTypes = 8,
    kNumKeymasterTentTypes = 8
};

struct TCreatureBankTypeTraits {
    const char* m_name;
};

struct TMonolithTypeTraits {
    const char* m_name;
};

struct TKeymasterTentTypeTraits {
    const char* m_name;
};

extern const TCreatureBankTypeTraits* akCreatureBankTypeTraits;
extern const TMonolithTypeTraits* akMonolithTypeTraits;
extern const TMonolithTypeTraits* akOneWayMonolithTypeTraits;
extern const TKeymasterTentTypeTraits* akKeymasterTentTypeTraits;

void InitializeCreatureBankTypeTraitsTable();
void InitializeMonolithTypeTraitsTables();
void InitializeKeymasterTentTypeTraitsTable();

// Objects that only have a type (RTTI TGenericObject).
class TGenericObject : public virtual TGameObject {
public:
    TGenericObject(const TObjectType& objType);
    TGenericObject(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream, int version) const;
    virtual std::string getTypeName() const;
};

// A map object a player can own: the owner follows the vbptr. A garrison
// before map version 4 and a shipyard before version 10 have no owner on
// disk.
class TFlaggableObject : public virtual TGameObject {
public:
    TFlaggableObject(const TObjectType& objType, TPlayer owner);
    TFlaggableObject(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream, int version) const;

    void setOwner(TPlayer newOwner) { _m_owner = newOwner; }
    TPlayer getOwner() const { return _m_owner; }

private:
    TPlayer _m_owner;
};

// A mine; its kind is the object type's subtype (RTTI TMine <-
// TFlaggableObject).
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

    virtual std::string getTypeName() const;
};

// An abandoned mine and the resources it may hold (RTTI TAbandonedMine <-
// TMine): any but wood by default.
class TAbandonedMine : public TMine {
public:
    TAbandonedMine(const TObjectType& objType);
    TAbandonedMine(const TObjectType& objType, TRawIStream* pIStream, int version);

    bool getBIsPotentialResource(TGameResourceType type) const;
    void setBIsPotentialResource(TGameResourceType type, bool bIsPotential);
    unsigned int getNumPotentialResources() const { return _m_abPotentialResource.count(); }

    virtual void write(TRawOStream* pOStream, int version) const;
    virtual bool isCustomized() const { return _m_abPotentialResource != _s_kabDefaultPotentialResource; }

private:
    static const std::bitset<kNumGameResourceTypes> _s_kabDefaultPotentialResource;

    std::bitset<kNumGameResourceTypes> _m_abPotentialResource;
};

// A creature generator; its kind is the object type's subtype (RTTI
// TGenerator <- TFlaggableObject).
class TGenerator : public TFlaggableObject {
};

// A garrison, its army and whether a visiting hero may take the army
// (RTTI TGarrison <- TFlaggableObject); its kind is the object type's
// subtype.
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

    virtual void write(TRawOStream* pOStream, int version) const;
    virtual std::string getTypeName() const;
    virtual bool isCustomized() const;

    const TArmy& getArmy() const { return _m_army; }
    void setArmy(const TArmy& newArmy);
    bool getBRemovableUnits() const { return _m_bRemovableUnits; }
    void setBRemovableUnits(bool bRemovable) { _m_bRemovableUnits = bRemovable; }

private:
    TArmy _m_army;
    bool _m_bRemovableUnits;
};

// A player's object that may own heroes or towns (RTTI TPlayableObject:
// the vbptr, then the owner; copy constructor 0x44055f).
class TPlayableObject : public virtual TGameObject {
public:
    TPlayableObject(const TObjectType& objType, TPlayer owner = ePlayerNone);
    TPlayableObject(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream, int version) const;

    TPlayer getOwner() const { return _m_owner; }
    void setOwner(TPlayer newOwner) { _m_owner = newOwner; }

protected:
    // The owner byte that heroes and towns read in their own records.
    void read(TRawIStream* pIStream, int version);

private:
    TPlayer _m_owner;
};

// An object a quest or another object can refer to by its link id (RTTI
// TLinkableObject: its vtable, vbptr, then the id; copy constructor
// 0x440c91). Two virtuals yield the linkable object it holds: none here,
// the visiting hero for a town (0x4c2a1d); the map follows them to find
// an id (0x421523).
class TLinkableObject : public virtual TGameObject {
public:
    TLinkableObject(const TObjectType& objType);

    virtual TLinkableObject* getPContainedObject() { return NULL; }
    virtual const TLinkableObject* getPContainedObject() const { return NULL; }

    unsigned int getLinkID() const { return _m_linkID; }
    // A fresh id from the running counter, never the no-link id (h3maped
    // 0x426ffd).
    void assignNewLinkID()
    {
        _m_linkID = s_nextLinkID++;
        if (_m_linkID == s_kNoLinkID)
            _m_linkID = s_nextLinkID++;
    }

    static const unsigned int s_kNoLinkID;
    static unsigned int s_nextLinkID;

    // The link id, from Armageddon's Blade on.
    virtual void write(TRawOStream* pOStream, int version) const;

protected:
    // The link id, in maps from version 15.
    void read(TRawIStream* pIStream, int version);

private:
    unsigned int _m_linkID;
};

// A pickable object with an optional message and guardians (RTTI
// TTreasure: the vbptr, the message, the custom-guardians flag and the
// guardians). A Restoration of Erathia map keeps 300 characters of the
// message.
class TTreasure : public virtual TGameObject {
public:
    enum { s_kMaxMessageLen = 300 };

    TTreasure(const TObjectType& objType);
    TTreasure(const TObjectType& objType, TRawIStream* pIStream, int version);

    const std::string& getMessage() const { return _m_message; }
    void setMessage(const std::string& newMessage) { _m_message = newMessage; }
    bool getBCustomGuardians() const { return _m_bCustomGuardians; }
    void setBCustomGuardians(bool bCustom) { _m_bCustomGuardians = bCustom; }
    const TArmy& getGuardians() const { return _m_guardians; }
    void setGuardians(const TArmy& newGuardians);

    virtual void importText(std::istream* pIStream, EGameVersion version);
    virtual void write(TRawOStream* pOStream, int version) const;
    virtual bool isCustomized() const;
    virtual bool hasText() const { return !getMessage().empty(); }
    virtual void exportText(std::ostream* pOStream, EGameVersion version) const;

protected:
    void read(TRawIStream* pIStream, int version);

private:
    std::string _m_message;
    bool _m_bCustomGuardians;
    TArmy _m_guardians;
};

// An artifact on the map; its artifact is the object type's subtype.
class TGameArtifact : public TTreasure {
public:
    TGameArtifact(const TObjectType& objType);
    TGameArtifact(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual std::string getTypeName() const;

    int getArtifactType() const { return getExtra(); }
};

// A spell scroll and its spell (Magic Arrow by default).
class TSpellScroll : public TTreasure {
public:
    TSpellScroll(const TObjectType& objType);
    TSpellScroll(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream, int version) const;
    virtual bool isCustomized() const;

    ESpellId getSpell() const { return _m_spell; }
    void setSpell(ESpellId newSpell);

private:
    ESpellId _m_spell;
};

// A resource pile and its quantity (0 is random); its type is the object
// type's subtype.
class TGameResource : public TTreasure {
public:
    enum { s_kMaxQuantity = 99999 };

    TGameResource(const TObjectType& objType);
    TGameResource(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream, int version) const;
    virtual std::string getTypeName() const;
    virtual bool isCustomized() const;

    TGameResourceType getResourceType() const { return TGameResourceType(getExtra()); }
    unsigned int getQuantity() const { return _m_quantity; }
    void setQuantity(unsigned int newQuantity) { _m_quantity = newQuantity; }

private:
    unsigned int _m_quantity;
};

// A sign or ocean bottle: its message, at most s_kMaxTextLen characters
// (the sign dialog's OnInitDialog limits its edit control to 150).
class TSign : public virtual TGameObject {
public:
    enum { s_kMaxTextLen = 150 };

    TSign(const TObjectType& objType);
    TSign(const TObjectType& objType, TRawIStream* pIStream, int version);

    const std::string& getText() const { return _m_text; }
    void setText(const std::string& newText);

    virtual void importText(std::istream* pIStream, EGameVersion version);
    virtual void write(TRawOStream* pOStream, int version) const;
    virtual bool isCustomized() const { return !_m_text.empty(); }
    virtual bool hasText() const { return true; }
    virtual void exportText(std::ostream* pOStream, EGameVersion version) const;

private:
    std::string _m_text;
};

// The scholar and what it teaches: a random reward, or a primary skill,
// secondary skill or spell.
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

    TRewardType getRewardType() const { return _m_rewardType; }
    void setRewardType(TRewardType newRewardType);
    TPrimarySkill getPrimarySkill() const { return _m_primarySkill; }
    void setPrimarySkill(TPrimarySkill newPrimarySkill);
    TSecondarySkill getSecondarySkill() const { return _m_secondarySkill; }
    void setSecondarySkill(TSecondarySkill newSecondarySkill);
    ESpellId getSpell() const { return _m_spell; }
    void setSpell(ESpellId newSpell);

    virtual void write(TRawOStream* pOStream, int version) const;
    virtual bool isCustomized() const { return _m_rewardType != eRewardRandom; }

private:
    TRewardType _m_rewardType;
    TPrimarySkill _m_primarySkill;
    TSecondarySkill _m_secondarySkill;
    ESpellId _m_spell;
};

// The Grail's site and its dig radius. It may not lie within nine cells
// of the map's edge (TGameMap's placements throw
// TPlaceObjFailureHolyGrailTooCloseToEdge).
class THolyGrail : public virtual TGameObject {
public:
    enum { s_kMaxRadius = 127 };

    THolyGrail(const TObjectType& objType);
    THolyGrail(const TObjectType& objType, TRawIStream* pIStream, int version);

    unsigned int getRadius() const { return _m_radius; }
    void setRadius(unsigned int newRadius) { _m_radius = newRadius; }

    virtual void write(TRawOStream* pOStream, int version) const;
    virtual bool isCustomized() const { return _m_radius != 0; }

private:
    unsigned int _m_radius;
};

// A shrine and its spell (none is random): a spell of the shrine's level
// that belongs to a school and is not a special one.
class TShrine : public virtual TGameObject {
public:
    TShrine(const TObjectType& objType);
    TShrine(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream, int version) const;
    virtual bool isCustomized() const;

    int getSpellLevel() const;
    ESpellId getSpell() const { return _m_spell; }
    void setSpell(ESpellId newSpell) { _m_spell = newSpell; }

private:
    ESpellId _m_spell;
};

// A witch hut and the secondary skills it may teach: any but leadership
// and necromancy by default (from map version 20).
class TWitchHut : public virtual TGameObject {
public:
    TWitchHut(const TObjectType& objType);
    TWitchHut(const TObjectType& objType, TRawIStream* pIStream, int version);

    const std::bitset<kNumSecSkills>& getPotentialSkills() const { return _m_abPotentialSkill; }
    void setPotentialSkills(const std::bitset<kNumSecSkills>& newSkills);

    virtual void write(TRawOStream* pOStream, int version) const;
    virtual bool isCustomized() const;

private:
    static const std::bitset<kNumSecSkills> _s_kabDefaultPotentialSkill;

    std::bitset<kNumSecSkills> _m_abPotentialSkill;
};

// A random dwelling whose alignment follows a town or a set of town types
// (RTTI TAbstractRandomlyAlignedGenerator: its vtable, vbptr, the town's
// link id, then the alignments; 16 bytes before TRandomlyAlignedGenerator's
// TFlaggableObject). Its two pure virtuals yield the flaggable part (the
// derived dwellings return their TFlaggableObject, 0x43e774). Removing a
// random town unlinks the dwellings that named it (h3maped 0x427fbe).
class TAbstractRandomlyAlignedGenerator : public virtual TGameObject {
public:
    virtual TFlaggableObject* getPFlaggableObject() = 0;
    virtual const TFlaggableObject* getPFlaggableObject() const = 0;

    unsigned int getTownLinkID() const { return _m_townLinkID; }
    void setTownLinkID(unsigned int townLinkID) { _m_townLinkID = townLinkID; }

private:
    unsigned int _m_townLinkID;
    std::bitset<kNumTownTypes> _m_alignments;
};

// A random dwelling whose level is drawn from a range (RTTI
// TAbstractRandomlyLeveledGenerator: its vtable and vbptr; 16 bytes in
// TRandomGenerator, after the alignment part).
class TAbstractRandomlyLeveledGenerator : public virtual TGameObject {
public:
    virtual TFlaggableObject* getPFlaggableObject() = 0;
    virtual const TFlaggableObject* getPFlaggableObject() const = 0;
};

#endif  /* HOMM3_EDITOR_OBJECTSPECIALIZATIONS_H */
