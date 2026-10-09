// ObjectSpecializations.h - the map objects that carry properties of
// their own (ObjectSpecializations.cpp; Loki h3maped object 21). Each
// derives virtually from TGameObject: its vbptr comes first, then its own
// members.
//
// Ported so far: the declarations the props dialogs and the map need.
// Layouts follow h3maped's copy constructors; VC6 keeps a vtordisp dword
// before each virtual TGameObject (the copy constructors clear it).
#ifndef HOMM3_EDITOR_OBJECTSPECIALIZATIONS_H
#define HOMM3_EDITOR_OBJECTSPECIALIZATIONS_H

#include <bitset>
#include <string>

#include "editor/GameObject.h"
#include "editor/Player.h"
#include "town_type.h"

class TRawIStream;

class THero;

// A player's object that may own heroes or towns (RTTI TPlayableObject:
// the vbptr, then the owner; copy constructor 0x44055f).
class TPlayableObject : public virtual TGameObject {
public:
    TPlayableObject(const TObjectType& objType, TPlayer owner);

    TPlayer getOwner() const { return _m_owner; }
    void setOwner(TPlayer newOwner) { _m_owner = newOwner; }

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

private:
    unsigned int _m_linkID;
};

// A map object a player can own: the owner follows the vbptr.
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

// Objects that only have a type (RTTI TGenericObject: the vbptr first).
class TGenericObject : public virtual TGameObject {
};

// A mine; its kind is the object type's subtype (RTTI TMine <-
// TFlaggableObject).
class TMine : public TFlaggableObject {
};

// An abandoned mine and the resources it may hold (RTTI TAbandonedMine <-
// TMine).
class TAbandonedMine : public TMine {
};

// A creature generator; its kind is the object type's subtype (RTTI
// TGenerator <- TFlaggableObject).
class TGenerator : public TFlaggableObject {
};

// A garrison and its army (RTTI TGarrison <- TFlaggableObject).
class TGarrison : public TFlaggableObject {
};

// A pickable object with an optional message and guardians (RTTI
// TTreasure: the vbptr first).
class TTreasure : public virtual TGameObject {
};

// An artifact on the map; its artifact is the object type's subtype.
class TGameArtifact : public TTreasure {
};

// A spell scroll.
class TSpellScroll : public TTreasure {
};

// A resource pile; its type is the object type's subtype.
class TGameResource : public TTreasure {
};

// The scholar and what it teaches (RTTI TScholar: the vbptr first).
class TScholar : public virtual TGameObject {
};

// A shrine and its spell (RTTI TShrine: the vbptr first).
class TShrine : public virtual TGameObject {
};

// A witch hut and its skill (RTTI TWitchHut: the vbptr first).
class TWitchHut : public virtual TGameObject {
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

// The Grail's site: it may not lie within nine cells of the map's edge
// (TGameMap's placements throw TPlaceObjFailureHolyGrailTooCloseToEdge).
class THolyGrail : public virtual TGameObject {
public:
    THolyGrail(const TObjectType& objType);
};

// A sign: its message, at most s_kMaxTextLen characters (the sign dialog's
// OnInitDialog limits its edit control to 150).
class TSign : public virtual TGameObject {
public:
    enum { s_kMaxTextLen = 150 };

    TSign(const TObjectType& objType);
    TSign(const TObjectType& objType, TRawIStream* pIStream, int version);

    const std::string& getText() const { return _m_text; }
    void setText(const std::string& newText);

    virtual void importText(std::istream* pIStream);
    virtual void write(TRawOStream* pOStream, int version) const;
    virtual bool isCustomized() const { return !_m_text.empty(); }
    virtual bool hasText() const { return true; }
    virtual void exportText(std::ostream* pOStream) const;

private:
    std::string _m_text;
};

#endif  /* HOMM3_EDITOR_OBJECTSPECIALIZATIONS_H */
