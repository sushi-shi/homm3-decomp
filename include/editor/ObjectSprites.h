// ObjectSprites.h - shared sprites for object types and hero flags (Loki
// h3maped ObjectSprites.cpp; h3maped 0x48fa1b..0x48fdc9). A sprite pointer
// locks its sprite in the object's shared table on construction (loading
// it on first use, h3maped 0x48fce9 and 0x48fd7c) and unlocks it when it
// goes out of scope (an empty body on Windows, folded at 0x40ed06); the
// tables are released at exit by initializeObjectSprites' function-local
// initializer. The members' names are not recorded.
#ifndef HOMM3_EDITOR_OBJECTSPRITES_H
#define HOMM3_EDITOR_OBJECTSPRITES_H

#include <stddef.h>

#include "editor/Player.h"

class CSprite;
struct TObjectType;

void initializeObjectSprites();

class TObjectSpritePtr {
public:
    TObjectSpritePtr(const TObjectType& objType) : _m_objType(objType)
    {
        _m_pSprite = _lockSprite(_m_objType);
    }
    ~TObjectSpritePtr()
    {
        if (_m_pSprite != NULL)
            _unlockSprite(_m_objType);
    }

    const CSprite* operator->() const { return _m_pSprite; }
    const CSprite* get() const { return _m_pSprite; }

private:
    static const CSprite* _lockSprite(const TObjectType& objType);
    static void _unlockSprite(const TObjectType& objType);

    const TObjectType& _m_objType;
    const CSprite* _m_pSprite;
};

class THeroFlagSpritePtr {
public:
    THeroFlagSpritePtr(TPlayer player) : _m_player(player)
    {
        _m_pSprite = _lockSprite(_m_player);
    }
    ~THeroFlagSpritePtr()
    {
        if (_m_pSprite != NULL)
            _unlockSprite(_m_player);
    }

    const CSprite* operator->() const { return _m_pSprite; }
    const CSprite* get() const { return _m_pSprite; }

private:
    static const CSprite* _lockSprite(TPlayer player);
    static void _unlockSprite(TPlayer player);

    TPlayer _m_player;
    const CSprite* _m_pSprite;
};

#endif  /* HOMM3_EDITOR_OBJECTSPRITES_H */
