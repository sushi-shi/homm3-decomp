// GameObject.h - the base of every object on the map (Loki h3maped
// GameObject.cpp). A game object refers to its interned TObjectType: the
// class keeps one map entry per distinct type with a reference count, and
// each object holds an iterator to its entry (the only data member; the
// vtable pointer follows it at +4). The destructor is pure and defined;
// clone and write are pure. The vtable order is retail's (__vt_11TGameObject):
// destructor, importText, clone, write, getTypeName, isCustomized, hasText,
// exportText. The inline members are emitted with the vtable in
// GameObject.cpp. The iterator member's name is not recorded;
// _s_objectTypeMap is the image's symbol.
#ifndef HOMM3_EDITOR_GAMEOBJECT_H
#define HOMM3_EDITOR_GAMEOBJECT_H

#include <exception>
#include <map>
#include <string>

#include "objecttype.h"

class istream;
class ostream;
class TRawOStream;

class TGameObject {
public:
    class TImportTextFailure : public exception {
    };

    TGameObject(const TGameObject& other);
    TGameObject(const TObjectType& objType);
    virtual ~TGameObject() = 0;
    TGameObject& operator=(const TGameObject& other);

    virtual void importText(istream* pIStream) {}
    virtual TGameObject* clone(void* (*pfnAllocator)(unsigned int)) const = 0;
    virtual void write(TRawOStream* pOStream) const = 0;
    virtual string getTypeName() const;
    virtual bool isCustomized() const { return false; }
    virtual bool hasText() const { return false; }
    virtual void exportText(ostream* pOStream) const {}

    const TObjectType& getObjectType() const { return _m_objectTypeIter->first; }
    const string& getImageName() const { return _m_objectTypeIter->first.getImageName(); }
    unsigned int getImageNum() const { return _m_objectTypeIter->first.getImageNum(); }
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
    const TTerrainMask& getTerrainMask() const { return _m_objectTypeIter->first.getTerrainMask(); }
    const TTerrainMask& getRecommendedTerrainMask() const
    {
        return _m_objectTypeIter->first.getRecommendedTerrainMask();
    }
    int getType() const { return _m_objectTypeIter->first.getType(); }
    int getExtra() const { return _m_objectTypeIter->first.getExtra(); }
    TSlotCategory getSlotCategory() const { return _m_objectTypeIter->first.getSlotCategory(); }
    bool getBUnderlay() const { return _m_objectTypeIter->first.getBUnderlay(); }
    unsigned int getWidth() const { return _m_objectTypeIter->first.getWidth(); }
    unsigned int getHeight() const { return _m_objectTypeIter->first.getHeight(); }
    bool hasTrigger() const { return _m_objectTypeIter->first.hasTrigger(); }
    const TPoint<unsigned int>& getTriggerLoc() const { return _m_objectTypeIter->first.getTriggerLoc(); }

private:
    typedef map<TObjectType, unsigned int, less<TObjectType> > _TObjectTypeMap;

    static _TObjectTypeMap _s_objectTypeMap;

    _TObjectTypeMap::iterator _m_objectTypeIter;
};

#endif  /* HOMM3_EDITOR_GAMEOBJECT_H */
