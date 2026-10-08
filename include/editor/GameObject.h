// GameObject.h - the base of every object on the map (GameObject.cpp;
// Loki h3maped object 13). A game object refers to its interned
// TObjectType: the class keeps one map entry per distinct type with a
// reference count, and each object holds an iterator to its entry. The
// vtable order is Loki's (__vt_11TGameObject): destructor, importText,
// clone, write, getTypeName, isCustomized, hasText, exportText; the props
// dialogs' captions call getTypeName through slot 4.
//
// The iterator follows the vtable pointer: h3maped's constructObjectHeightMap
// (0x41e84b) reaches the TObjectType at the entry's node + 0xc through +4.
#ifndef HOMM3_EDITOR_GAMEOBJECT_H
#define HOMM3_EDITOR_GAMEOBJECT_H

#include <iosfwd>
#include <map>
#include <memory>
#include <string>

#include "objecttype.h"

class TRawOStream;

class TGameObject {
public:
    TGameObject(const TGameObject& other);
    TGameObject(const TObjectType& objType);
    virtual ~TGameObject() = 0;
    TGameObject& operator=(const TGameObject& other);

    virtual void importText(std::istream* pIStream) {}
    // Windows returns the clone in an auto_ptr (h3maped 0x42a75a calls
    // slot 2 with a result slot and no allocator); Loki passes one.
    virtual std::auto_ptr<TGameObject> clone() const = 0;
    // The map format version follows the stream (h3maped 0x41f217 passes 2).
    virtual void write(TRawOStream* pOStream, int version) const = 0;
    virtual std::string getTypeName() const;
    virtual bool isCustomized() const { return false; }
    virtual bool hasText() const { return false; }
    virtual void exportText(std::ostream* pOStream) const {}

    const TObjectType& getObjectType() const { return _m_objectTypeIter->first; }
    bool getBCellPlaced(unsigned int x, unsigned int y) const
    {
        return _m_objectTypeIter->first.getBCellPlaced(x, y);
    }
    bool getBCellPassable(unsigned int x, unsigned int y) const
    {
        return _m_objectTypeIter->first.getBCellPassable(x, y);
    }
    bool getBCellShadow(unsigned int x, unsigned int y) const
    {
        return _m_objectTypeIter->first.getBCellShadow(x, y);
    }
    bool getBCellTrigger(unsigned int x, unsigned int y) const
    {
        return _m_objectTypeIter->first.getBCellTrigger(x, y);
    }
    const TObjectType::TPoint& getTriggerLoc() const { return _m_objectTypeIter->first.getTriggerLoc(); }
    bool getBUnderlay() const { return _m_objectTypeIter->first.getBUnderlay() != 0; }
    unsigned int getWidth() const { return _m_objectTypeIter->first.getWidth(); }
    unsigned int getHeight() const { return _m_objectTypeIter->first.getHeight(); }

private:
    typedef std::map<TObjectType, unsigned int, std::less<TObjectType> > _TObjectTypeMap;

    _TObjectTypeMap::iterator _m_objectTypeIter;
};

// An object type's identity in the map format (h3maped 0x490944).
TRawOStream& operator<<(TRawOStream& stream, const TObjectType& objType);

#endif  /* HOMM3_EDITOR_GAMEOBJECT_H */
