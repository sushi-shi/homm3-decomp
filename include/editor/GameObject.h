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

#include <exception>
#include <iosfwd>
#include <map>
#include <memory>
#include <string>

#include "gameversion.h"
#include "objecttype.h"
#include "editor/Point.h"

class TRawIStream;
class TRawOStream;

class TGameObject {
public:
    // importText's failure (the map's importText catches it).
    class TImportTextFailure : public std::exception {
    };

    TGameObject(const TGameObject& other);
    TGameObject(const TObjectType& objType);
    virtual ~TGameObject() = 0;
    TGameObject& operator=(const TGameObject& other);

    // The text import and export pass the map's edition (the map's
    // importText and exportText hand over _m_version; the empty bodies
    // share one `ret 8`, 0x4026bc).
    virtual void importText(std::istream* pIStream, EGameVersion version) {}
    // Windows returns the clone in an auto_ptr (h3maped 0x42a75a calls
    // slot 2 with a result slot and no allocator); Loki passes one.
    virtual std::auto_ptr<TGameObject> clone() const = 0;
    // The map format version follows the stream (h3maped 0x41f217 passes 2).
    virtual void write(TRawOStream* pOStream, int version) const = 0;
    virtual std::string getTypeName() const;
    virtual bool isCustomized() const { return false; }
    virtual bool hasText() const { return false; }
    virtual void exportText(std::ostream* pOStream, EGameVersion version) const {}

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
    bool hasTrigger() const { return _m_objectTypeIter->first.m_hasTrigger != 0; }
    const TObjectType::TPoint& getTriggerLoc() const { return _m_objectTypeIter->first.getTriggerLoc(); }
    const std::bitset<kNumTerrainTypes>& getTerrainMask() const { return _m_objectTypeIter->first._m_terrainMask; }
    TAdventureObjectType getType() const { return _m_objectTypeIter->first.getType(); }
    int getExtra() const { return _m_objectTypeIter->first.getExtra(); }
    bool getBUnderlay() const { return _m_objectTypeIter->first.getBUnderlay() != 0; }
    unsigned int getWidth() const { return _m_objectTypeIter->first.getWidth(); }
    unsigned int getHeight() const { return _m_objectTypeIter->first.getHeight(); }

private:
    typedef std::map<TObjectType, unsigned int, std::less<TObjectType> > _TObjectTypeMap;

    _TObjectTypeMap::iterator _m_objectTypeIter;
};

// An object type's identity in the map format (h3maped 0x490944).
TRawOStream& operator<<(TRawOStream& stream, const TObjectType& objType);
TRawIStream& operator>>(TRawIStream& stream, TObjectType& objType);
// The object types' order, member by member (the less instance h3maped
// keeps, 0x490b77, compares the type, subtype and masks in turn).
bool operator<(const TObjectType& lhs, const TObjectType& rhs);

// A placed object's tile less its type's trigger cell: the location the
// dialogs show (h3maped 0x47344d).
inline const TTilePoint operator-(const TTilePoint& lhs, const TObjectType::TPoint& rhs)
{
    TTilePoint result = lhs;
    result.x(result.x() - rhs.m_x);
    result.y(result.y() - rhs.m_y);
    return result;
}

#endif  /* HOMM3_EDITOR_GAMEOBJECT_H */
