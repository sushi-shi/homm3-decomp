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
    virtual TGameObject* clone(void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual void write(TRawOStream* pOStream) const = 0;
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
    bool getBUnderlay() const { return _m_objectTypeIter->first.getBUnderlay() != 0; }
    unsigned int getWidth() const { return _m_objectTypeIter->first.getWidth(); }
    unsigned int getHeight() const { return _m_objectTypeIter->first.getHeight(); }

private:
    typedef std::map<TObjectType, unsigned int, std::less<TObjectType> > _TObjectTypeMap;

    _TObjectTypeMap::iterator _m_objectTypeIter;
};

#endif  /* HOMM3_EDITOR_GAMEOBJECT_H */
