// GameObject.h - the base of every object on the map (GameObject.cpp;
// Loki h3maped object 13). A game object refers to its interned
// TObjectType: the class keeps one map entry per distinct type with a
// reference count, and each object holds an iterator to its entry. The
// vtable order is Loki's (__vt_11TGameObject): destructor, importText,
// clone, write, getTypeName, isCustomized, hasText, exportText; the props
// dialogs' captions call getTypeName through slot 4.
//
// Ported so far: the virtual interface the props dialogs call. The data
// member, an iterator into the map keyed by TObjectType, waits for the
// editor's TObjectType: the game's objecttype.h brings mapcell.h, whose
// CObject collides with MFC's.
#ifndef HOMM3_EDITOR_GAMEOBJECT_H
#define HOMM3_EDITOR_GAMEOBJECT_H

#include <iosfwd>
#include <string>

class TRawOStream;
struct TObjectType;

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
};

#endif  /* HOMM3_EDITOR_GAMEOBJECT_H */
