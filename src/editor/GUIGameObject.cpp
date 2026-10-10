// GUIGameObject.cpp - the editor's map objects (h3maped 0x43ee7e..0x449358;
// Loki h3maped object 2). Each game object class gets a GUI counterpart
// that draws its sprite (and the owner's flag, or the hero's flag),
// hit-tests it, colours it on the mini map and hands itself to the edit
// context; TGUIGameObjectFactory creates them. The Windows objects draw
// into a 16-bit DIB section clipped by the device context's clip box, mark
// a customized object with a check mark bitmap, and drop Loki's asserts.
//
// The classes are the RTTI's (anonymous namespace, "C:\Dev\Heroes 3 Exp
// 2\Editor\GUIGameObject.cpp"): Complete calls Loki's TGUIHeroBase
// TGUIBasicHero and adds a hero placeholder, the random generators, the
// quest guard and the witch hut.
#include "editor/stdafx.h"

#include <stdlib.h>
#include <bitset>
#include <memory>

#include "va.h"
#include "adventureobjecttype.h"
#include "csprite.h"
#include "cspriteframe.h"
#include "exceptions.h"
#include "objnames.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "town_type.h"
#include "editor/BlackBox.h"
#include "editor/Colors.h"
#include "editor/DIBSection.h"
#include "editor/Event.h"
#include "editor/GameMap.h"
#include "editor/Generator.h"
#include "editor/GUIGameObject.h"
#include "editor/Hero.h"
#include "editor/MemoryDC.h"
#include "editor/Monster.h"
#include "editor/ObjectSpecializations.h"
#include "editor/ObjectSprites.h"
#include "editor/QuestGuard.h"
#include "editor/resource.h"
#include "editor/SeersHut.h"
#include "editor/Tile.h"
#include "editor/Town.h"

// auto_ptr's transfer: the source gives up ownership and the pointer.
template<class T>
TResourcePtr<T>& TResourcePtr<T>::operator=(const TResourcePtr<T>& rhs)
{
    if (this != &rhs) {
        if (m_ptr != rhs.m_ptr) {
            if (m_owns && m_ptr)
                ResourceManager::Dispose(m_ptr);
            m_owns = rhs.m_owns;
        } else if (rhs.m_owns)
            m_owns = true;
        rhs.m_owns = false;
        m_ptr = rhs.m_ptr;
    }
    return *this;
}

namespace {

// The check mark drawn over a customized object's trigger corner: a
// bitmap resource, the small one at the smallest zoom, and its size.
class TCheckMarkBmp : public CBitmap {
public:
    TCheckMarkBmp(bool bSmall);

    static TCheckMarkBmp& getCheckMark()
    {
        DATA_COMPGEN_GUARD(0x005aa690, checkMarkGuard, checkMark)
        VA_COMPGEN(0x0043fb65, 0xa, STATIC_DTOR, checkMark)
        DATA(0x005aa6a0) static TCheckMarkBmp checkMark(false);
        return checkMark;
    }
    static TCheckMarkBmp& getSmallCheckMark()
    {
        DATA_COMPGEN_GUARD(0x005aa674, smallCheckMarkGuard, smallCheckMark)
        VA_COMPGEN(0x0043fb5b, 0xa, STATIC_DTOR, smallCheckMark)
        DATA(0x005aa680) static TCheckMarkBmp smallCheckMark(true);
        return smallCheckMark;
    }
    static TCheckMarkBmp& get(TZoom zoom) { return zoom == eZoom25 ? getSmallCheckMark() : getCheckMark(); }

    int m_width;
    int m_height;
};

VA(0x0043f20b, 0x80)
TCheckMarkBmp::TCheckMarkBmp(bool bSmall)
{
    BITMAP bitmap;
    if (!LoadBitmap(bSmall ? IDB_CHECK_MARK_SMALL : IDB_CHECK_MARK) || !GetObject(sizeof(bitmap), &bitmap))
        throw TRuntimeError();
    m_width = bitmap.bmWidth;
    m_height = bitmap.bmHeight;
}

VA(0x0043f2f4, 0xa0)
void drawSprite(const CSprite* pSprite, unsigned int frameNum, T16bppDIBSection* pDestBmp, int x, int y,
                const CRect& destClipRect, TZoom zoom)
{
    unsigned int frame = frameNum % pSprite->GetNumFrames(0);
    const TZoomTraits& zoomTraits = akZoomTraits[zoom];
    int width = pSprite->GetWidth() >> zoomTraits.m_scaleShift;
    int height = pSprite->GetHeight() >> zoomTraits.m_scaleShift;
    x -= width - 1;
    y -= height - 1;
    zoomTraits.m_pDrawAdvObj(pSprite, frame, 0, 0, width, height, x - destClipRect.left, y - destClipRect.top,
                             (uword*)((ubyte*)pDestBmp->getPixels() + destClipRect.top * pDestBmp->getPitch())
                                 + destClipRect.left,
                             destClipRect.Width(), destClipRect.Height(), pDestBmp->getPitch(), 0);
}

VA(0x0043f3b8, 0xa7)
void drawSprite(const CSprite* pSprite, unsigned int frameNum, TPlayer player, T16bppDIBSection* pDestBmp, int x,
                int y, const CRect& destClipRect, TZoom zoom)
{
    unsigned int frame = frameNum % pSprite->GetNumFrames(0);
    const TZoomTraits& zoomTraits = akZoomTraits[zoom];
    int width = pSprite->GetWidth() >> zoomTraits.m_scaleShift;
    int height = pSprite->GetHeight() >> zoomTraits.m_scaleShift;
    x -= width - 1;
    y -= height - 1;
    zoomTraits.m_pDrawAdvObj(pSprite, frame, 0, 0, width, height, x - destClipRect.left, y - destClipRect.top,
                             (uword*)((ubyte*)pDestBmp->getPixels() + destClipRect.top * pDestBmp->getPitch())
                                 + destClipRect.left,
                             destClipRect.Width(), destClipRect.Height(), pDestBmp->getPitch(),
                             akPlayerColor[player + 1]);
}

VA(0x0043f45f, 0x9c)
void drawSpriteShadow(const CSprite* pSprite, unsigned int frameNum, T16bppDIBSection* pDestBmp, int x, int y,
                      const CRect& destClipRect, TZoom zoom)
{
    unsigned int frame = frameNum % pSprite->GetNumFrames(0);
    const TZoomTraits& zoomTraits = akZoomTraits[zoom];
    int width = pSprite->GetWidth() >> zoomTraits.m_scaleShift;
    int height = pSprite->GetHeight() >> zoomTraits.m_scaleShift;
    x -= width - 1;
    y -= height - 1;
    zoomTraits.m_pDrawAdvObjShadow(pSprite, frame, 0, 0, width, height, x - destClipRect.left,
                                   y - destClipRect.top,
                                   (uword*)((ubyte*)pDestBmp->getPixels()
                                            + destClipRect.top * pDestBmp->getPitch())
                                       + destClipRect.left,
                                   destClipRect.Width(), destClipRect.Height(), pDestBmp->getPitch());
}

VA(0x0043f4fb, 0xb4)
void drawSpriteCell(const CSprite* pSprite, unsigned int frameNum, unsigned int cellX, unsigned int cellY,
                    T16bppDIBSection* pDestBmp, int x, int y, const CRect& destClipRect, TZoom zoom)
{
    unsigned int frame = frameNum % pSprite->GetNumFrames(0);
    const TZoomTraits& zoomTraits = akZoomTraits[zoom];
    int width = pSprite->GetWidth() >> zoomTraits.m_scaleShift;
    int height = pSprite->GetHeight() >> zoomTraits.m_scaleShift;
    int cellWidth = zoomTraits.m_tileSize;
    int cellHeight = zoomTraits.m_tileSize;
    int srcX = width - (cellX + 1) * cellWidth;
    int srcY = height - (cellY + 1) * cellHeight;
    x -= cellWidth - 1;
    y -= cellHeight - 1;
    zoomTraits.m_pDrawAdvObj(pSprite, frame, srcX, srcY, cellWidth, cellHeight, x - destClipRect.left,
                             y - destClipRect.top,
                             (uword*)((ubyte*)pDestBmp->getPixels() + destClipRect.top * pDestBmp->getPitch())
                                 + destClipRect.left,
                             destClipRect.Width(), destClipRect.Height(), pDestBmp->getPitch(), 0);
}

VA(0x0043f5af, 0xae)
void drawSpriteCellShadow(const CSprite* pSprite, unsigned int frameNum, unsigned int cellX, unsigned int cellY,
                          T16bppDIBSection* pDestBmp, int x, int y, const CRect& destClipRect, TZoom zoom)
{
    unsigned int frame = frameNum % pSprite->GetNumFrames(0);
    const TZoomTraits& zoomTraits = akZoomTraits[zoom];
    int width = pSprite->GetWidth() >> zoomTraits.m_scaleShift;
    int height = pSprite->GetHeight() >> zoomTraits.m_scaleShift;
    int cellWidth = zoomTraits.m_tileSize;
    int cellHeight = zoomTraits.m_tileSize;
    int srcX = width - (cellX + 1) * cellWidth;
    int srcY = height - (cellY + 1) * cellHeight;
    x -= cellWidth - 1;
    y -= cellHeight - 1;
    zoomTraits.m_pDrawAdvObjShadow(pSprite, frame, srcX, srcY, cellWidth, cellHeight, x - destClipRect.left,
                                   y - destClipRect.top,
                                   (uword*)((ubyte*)pDestBmp->getPixels()
                                            + destClipRect.top * pDestBmp->getPitch())
                                       + destClipRect.left,
                                   destClipRect.Width(), destClipRect.Height(), pDestBmp->getPitch());
}

VA(0x0043f65d, 0xba)
void drawSpriteCell(const CSprite* pSprite, unsigned int frameNum, unsigned int cellX, unsigned int cellY,
                    TPlayer player, T16bppDIBSection* pDestBmp, int x, int y, const CRect& destClipRect, TZoom zoom)
{
    unsigned int frame = frameNum % pSprite->GetNumFrames(0);
    const TZoomTraits& zoomTraits = akZoomTraits[zoom];
    int width = pSprite->GetWidth() >> zoomTraits.m_scaleShift;
    int height = pSprite->GetHeight() >> zoomTraits.m_scaleShift;
    int cellWidth = zoomTraits.m_tileSize;
    int cellHeight = zoomTraits.m_tileSize;
    int srcX = width - (cellX + 1) * cellWidth;
    int srcY = height - (cellY + 1) * cellHeight;
    x -= cellWidth - 1;
    y -= cellHeight - 1;
    zoomTraits.m_pDrawAdvObj(pSprite, frame, srcX, srcY, cellWidth, cellHeight, x - destClipRect.left,
                             y - destClipRect.top,
                             (uword*)((ubyte*)pDestBmp->getPixels() + destClipRect.top * pDestBmp->getPitch())
                                 + destClipRect.left,
                             destClipRect.Width(), destClipRect.Height(), pDestBmp->getPitch(),
                             akPlayerColor[player + 1]);
}

VA(0x0043f717, 0x64)
bool hitTestSprite(const CSprite* pSprite, unsigned int frameNum, int x, int y, TZoom zoom)
{
    unsigned int frame = frameNum % pSprite->GetNumFrames(0);
    const TZoomTraits& zoomTraits = akZoomTraits[zoom];
    int width = pSprite->GetWidth() >> zoomTraits.m_scaleShift;
    int height = pSprite->GetHeight() >> zoomTraits.m_scaleShift;
    const CSpriteFrame* pSpriteFrame = ((CSprite*)pSprite)->GetFrame(0, frame);
    unsigned char pixel = pSpriteFrame->GetPixel((width - x - 1) << zoomTraits.m_scaleShift,
                                                 (height - y - 1) << zoomTraits.m_scaleShift);
    return pixel > 4;
}

// A generic object.
class TGUIGenericObject : public TGenericObject, public TGUIGameObject {
public:
    TGUIGenericObject(const TObjectType& objType);
    TGUIGenericObject(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual std::auto_ptr<TGameObject> clone() const;
};

VA(0x0043f77b, 0x90)
TGUIGenericObject::TGUIGenericObject(const TObjectType& objType)
    : TGameObject(objType), TGenericObject(objType), TGUIGameObject(objType)
{
}

VA(0x0043f80b, 0x96)
TGUIGenericObject::TGUIGenericObject(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TGenericObject(objType, pIStream, version), TGUIGameObject(objType)
{
}

VA(0x0043f8a1, 0x64)
std::auto_ptr<TGameObject> TGUIGenericObject::clone() const
{
    return std::auto_ptr<TGameObject>(new TGUIGenericObject(*this));
}

// An object with a game class of its own: drawing goes through the sprite
// hooks, and a customized object carries a check mark.
class TGUISpecializedObject : public TGUIGameObject {
public:
    TGUISpecializedObject(const TObjectType& objType);

    virtual void draw(unsigned int frameNum, CDC* pDC, T16bppDIBSection* pDestBmp, int x, int y,
                      TZoom zoom) const;
    virtual void drawCell(unsigned int frameNum, unsigned int cellX, unsigned int cellY, CDC* pDC,
                          T16bppDIBSection* pDestBmp, int x, int y, TZoom zoom) const;
    virtual bool hitTest(unsigned int frameNum, int x, int y, TZoom zoom) const;

protected:
    virtual void drawSpriteImpl(unsigned int frameNum, CDC* pDC, T16bppDIBSection* pDestBmp, int x, int y,
                                TZoom zoom) const;
    virtual void drawSpriteCellImpl(unsigned int frameNum, unsigned int cellX, unsigned int cellY, CDC* pDC,
                                    T16bppDIBSection* pDestBmp, int x, int y, TZoom zoom) const;
    virtual bool hitTestSpriteImpl(unsigned int frameNum, int x, int y, TZoom zoom) const;
};

VA(0x0043f9fe, 0x6a)
TGUISpecializedObject::TGUISpecializedObject(const TObjectType& objType) : TGameObject(objType), TGUIGameObject(objType)
{
}

VA(0x0043fa93, 0x6e)
void TGUISpecializedObject::draw(unsigned int frameNum, CDC* pDC, T16bppDIBSection* pDestBmp, int x, int y,
                                 TZoom zoom) const
{
    drawSpriteImpl(frameNum, pDC, pDestBmp, x, y, zoom);
    if (isCustomized()) {
        TCheckMarkBmp& checkMark = TCheckMarkBmp::get(zoom);
        drawTransparentBitmap(pDC, &checkMark, x - checkMark.m_width + 1, y - akZoomTraits[zoom].m_tileSize + 1);
    }
}

VA(0x0043fb6f, 0x80)
void TGUISpecializedObject::drawCell(unsigned int frameNum, unsigned int cellX, unsigned int cellY, CDC* pDC,
                                     T16bppDIBSection* pDestBmp, int x, int y, TZoom zoom) const
{
    drawSpriteCellImpl(frameNum, cellX, cellY, pDC, pDestBmp, x, y, zoom);
    if (cellX == 0 && cellY == 0 && isCustomized()) {
        TCheckMarkBmp& checkMark = TCheckMarkBmp::get(zoom);
        drawTransparentBitmap(pDC, &checkMark, x - checkMark.m_width + 1, y - akZoomTraits[zoom].m_tileSize + 1);
    }
}

VA(0x0043fbef, 0x8b)
bool TGUISpecializedObject::hitTest(unsigned int frameNum, int x, int y, TZoom zoom) const
{
    if (hitTestSpriteImpl(frameNum, x, y, zoom))
        return true;
    if (!isCustomized())
        return false;
    TCheckMarkBmp& checkMark = TCheckMarkBmp::get(zoom);
    int bmpX = checkMark.m_width - x - 1;
    int bmpY = akZoomTraits[zoom].m_tileSize - y - 1;
    return bmpX >= 0 && bmpX < checkMark.m_width && bmpY >= 0 && bmpY < checkMark.m_height
           && !isTransparentPixel(&checkMark, bmpX, bmpY);
}

VA(0x0043fc7a, 0x1e)
void TGUISpecializedObject::drawSpriteImpl(unsigned int frameNum, CDC* pDC, T16bppDIBSection* pDestBmp, int x,
                                           int y, TZoom zoom) const
{
    TGUIGameObject::draw(frameNum, pDC, pDestBmp, x, y, zoom);
}

VA(0x0043fc98, 0x24)
void TGUISpecializedObject::drawSpriteCellImpl(unsigned int frameNum, unsigned int cellX, unsigned int cellY,
                                               CDC* pDC, T16bppDIBSection* pDestBmp, int x, int y, TZoom zoom) const
{
    TGUIGameObject::drawCell(frameNum, cellX, cellY, pDC, pDestBmp, x, y, zoom);
}

VA(0x0043fcbc, 0x18)
bool TGUISpecializedObject::hitTestSpriteImpl(unsigned int frameNum, int x, int y, TZoom zoom) const
{
    return TGUIGameObject::hitTest(frameNum, x, y, zoom);
}

// A hero or hero placeholder on the map: its sprite and its owner's flag.
class TGUIBasicHero : public TGUISpecializedObject {
public:
    TGUIBasicHero(const TObjectType& objType);

    virtual void drawShadow(unsigned int frameNum, CDC* pDC, T16bppDIBSection* pDestBmp, int x, int y,
                            TZoom zoom) const;
    virtual void drawCellShadow(unsigned int frameNum, unsigned int cellX, unsigned int cellY, CDC* pDC,
                                T16bppDIBSection* pDestBmp, int x, int y, TZoom zoom) const;
    virtual TColor miniMapColor(TTerrainType terrainType) const;
    virtual bool isAnimated() const;
    virtual bool isOwnable() const { return true; }

protected:
    virtual void drawSpriteImpl(unsigned int frameNum, CDC* pDC, T16bppDIBSection* pDestBmp, int x, int y,
                                TZoom zoom) const;
    virtual void drawSpriteCellImpl(unsigned int frameNum, unsigned int cellX, unsigned int cellY, CDC* pDC,
                                    T16bppDIBSection* pDestBmp, int x, int y, TZoom zoom) const;
    virtual bool hitTestSpriteImpl(unsigned int frameNum, int x, int y, TZoom zoom) const;

private:
    virtual TBasicHero* _getPHero() = 0;
    virtual const TBasicHero* _getPHero() const = 0;
};

VA(0x0043fcd4, 0x6a)
TGUIBasicHero::TGUIBasicHero(const TObjectType& objType) : TGameObject(objType), TGUISpecializedObject(objType)
{
}

VA(0x0043fd3e, 0xa1)
void TGUIBasicHero::drawShadow(unsigned int frameNum, CDC* pDC, T16bppDIBSection* pDestBmp, int x, int y,
                               TZoom zoom) const
{
    TGUIGameObject::drawShadow(frameNum, pDC, pDestBmp, x, y, zoom);
    THeroFlagSpritePtr pFlagSprite(_getPHero()->getOwner());
    CRect clipRect;
    int region = pDC->GetClipBox(&clipRect);
    if (region != ERROR && region != NULLREGION)
        drawSpriteShadow(pFlagSprite.get(), getAnimOffset() + frameNum, pDestBmp, x, y, clipRect, zoom);
}

VA(0x0043fdee, 0xad)
void TGUIBasicHero::drawCellShadow(unsigned int frameNum, unsigned int cellX, unsigned int cellY, CDC* pDC,
                                   T16bppDIBSection* pDestBmp, int x, int y, TZoom zoom) const
{
    TGUIGameObject::drawCellShadow(frameNum, cellX, cellY, pDC, pDestBmp, x, y, zoom);
    THeroFlagSpritePtr pFlagSprite(_getPHero()->getOwner());
    CRect clipRect;
    int region = pDC->GetClipBox(&clipRect);
    if (region != ERROR && region != NULLREGION)
        drawSpriteCellShadow(pFlagSprite.get(), getAnimOffset() + frameNum, cellX, cellY, pDestBmp, x, y,
                             clipRect, zoom);
}

VA(0x0043fe9b, 0xa1)
void TGUIBasicHero::drawSpriteImpl(unsigned int frameNum, CDC* pDC, T16bppDIBSection* pDestBmp, int x, int y,
                                   TZoom zoom) const
{
    TGUISpecializedObject::drawSpriteImpl(frameNum, pDC, pDestBmp, x, y, zoom);
    THeroFlagSpritePtr pFlagSprite(_getPHero()->getOwner());
    CRect clipRect;
    int region = pDC->GetClipBox(&clipRect);
    if (region != ERROR && region != NULLREGION)
        drawSprite(pFlagSprite.get(), getAnimOffset() + frameNum, pDestBmp, x, y, clipRect, zoom);
}

VA(0x0043ff3c, 0xad)
void TGUIBasicHero::drawSpriteCellImpl(unsigned int frameNum, unsigned int cellX, unsigned int cellY, CDC* pDC,
                                       T16bppDIBSection* pDestBmp, int x, int y, TZoom zoom) const
{
    TGUISpecializedObject::drawSpriteCellImpl(frameNum, cellX, cellY, pDC, pDestBmp, x, y, zoom);
    THeroFlagSpritePtr pFlagSprite(_getPHero()->getOwner());
    CRect clipRect;
    int region = pDC->GetClipBox(&clipRect);
    if (region != ERROR && region != NULLREGION)
        drawSpriteCell(pFlagSprite.get(), getAnimOffset() + frameNum, cellX, cellY, pDestBmp, x, y, clipRect,
                       zoom);
}

VA(0x0043ffe9, 0x87)
bool TGUIBasicHero::hitTestSpriteImpl(unsigned int frameNum, int x, int y, TZoom zoom) const
{
    if (TGUISpecializedObject::hitTestSpriteImpl(frameNum, x, y, zoom))
        return true;
    THeroFlagSpritePtr pFlagSprite(_getPHero()->getOwner());
    return hitTestSprite(pFlagSprite.get(), getAnimOffset() + frameNum, x, y, zoom);
}

VA(0x00440070, 0x19)
TColor TGUIBasicHero::miniMapColor(TTerrainType terrainType) const
{
    return akPlayerColor[_getPHero()->getOwner() + 1];
}

VA(0x00440089, 0x54)
bool TGUIBasicHero::isAnimated() const
{
    if (TGUIGameObject::isAnimated())
        return true;
    THeroFlagSpritePtr pFlagSprite(_getPHero()->getOwner());
    return pFlagSprite.get()->GetNumFrames(0) > 1;
}

// A hero placeholder: a campaign's carried-over hero.
class TGUIHeroPlaceholder : public THeroPlaceholder, public TGUIBasicHero {
public:
    TGUIHeroPlaceholder(const TObjectType& objType, TPlayer owner);
    TGUIHeroPlaceholder(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual bool edit(TEditContext* pEditContext, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual std::auto_ptr<TGameObject> clone() const;

private:
    virtual TBasicHero* _getPHero() { return this; }
    virtual const TBasicHero* _getPHero() const { return this; }
};

VA(0x004400dd, 0x98)
TGUIHeroPlaceholder::TGUIHeroPlaceholder(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), THeroPlaceholder(objType, owner), TGUIBasicHero(objType)
{
}

VA(0x00440179, 0x9b)
TGUIHeroPlaceholder::TGUIHeroPlaceholder(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), THeroPlaceholder(objType, pIStream, version), TGUIBasicHero(objType)
{
}

VA(0x00440214, 0x1e)
bool TGUIHeroPlaceholder::edit(TEditContext* pEditContext, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    return pEditContext->onEditProperties(this, pMap, TMapObjectRef(bSecondLayer, objID));
}

VA(0x00440232, 0x64)
std::auto_ptr<TGameObject> TGUIHeroPlaceholder::clone() const
{
    return std::auto_ptr<TGameObject>(new TGUIHeroPlaceholder(*this));
}

class TGUINonRandomHero : public TNonRandomHero, public TGUIBasicHero {
public:
    TGUINonRandomHero(const TObjectType& objType, TPlayer owner, THeroID heroID);
    TGUINonRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual bool edit(TEditContext* pEditContext, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual std::auto_ptr<TGameObject> clone() const;

private:
    virtual TBasicHero* _getPHero() { return this; }
    virtual const TBasicHero* _getPHero() const { return this; }
};

VA(0x004405b6, 0xb5)
TGUINonRandomHero::TGUINonRandomHero(const TObjectType& objType, TPlayer owner, THeroID heroID)
    : TGameObject(objType), TNonRandomHero(objType, owner, heroID), TGUIBasicHero(objType)
{
}

VA(0x004406ec, 0xb5)
TGUINonRandomHero::TGUINonRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TNonRandomHero(objType, pIStream, version), TGUIBasicHero(objType)
{
}

VA(0x004407a1, 0x20)
bool TGUINonRandomHero::edit(TEditContext* pEditContext, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    return pEditContext->onEditProperties(this, pMap, TMapObjectRef(bSecondLayer, objID));
}

VA(0x004407c1, 0x6a)
std::auto_ptr<TGameObject> TGUINonRandomHero::clone() const
{
    return std::auto_ptr<TGameObject>(new TGUINonRandomHero(*this));
}

class TGUIRandomHero : public TRandomHero, public TGUIBasicHero {
public:
    TGUIRandomHero(const TObjectType& objType, TPlayer owner);
    TGUIRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual bool edit(TEditContext* pEditContext, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual std::auto_ptr<TGameObject> clone() const;

private:
    virtual TBasicHero* _getPHero() { return this; }
    virtual const TBasicHero* _getPHero() const { return this; }
};

VA(0x00440d64, 0xb2)
TGUIRandomHero::TGUIRandomHero(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), TRandomHero(objType, owner), TGUIBasicHero(objType)
{
}

VA(0x00440e2e, 0xb5)
TGUIRandomHero::TGUIRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TRandomHero(objType, pIStream, version), TGUIBasicHero(objType)
{
}

VA(0x00440ee3, 0x20)
bool TGUIRandomHero::edit(TEditContext* pEditContext, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    return pEditContext->onEditProperties(this, pMap, TMapObjectRef(bSecondLayer, objID));
}

VA(0x00440f03, 0x6a)
std::auto_ptr<TGameObject> TGUIRandomHero::clone() const
{
    return std::auto_ptr<TGameObject>(new TGUIRandomHero(*this));
}

// A prison: a hero's sprite without the flag.
class TGUIPrison : public TPrison, public TGUISpecializedObject {
public:
    TGUIPrison(const TObjectType& objType, THeroID heroID);
    TGUIPrison(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual bool edit(TEditContext* pEditContext, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual std::auto_ptr<TGameObject> clone() const;
};

VA(0x004410ca, 0xb2)
TGUIPrison::TGUIPrison(const TObjectType& objType, THeroID heroID)
    : TGameObject(objType), TPrison(objType, heroID), TGUISpecializedObject(objType)
{
}

VA(0x0044117c, 0xb5)
TGUIPrison::TGUIPrison(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TPrison(objType, pIStream, version), TGUISpecializedObject(objType)
{
}

VA(0x00441231, 0x20)
bool TGUIPrison::edit(TEditContext* pEditContext, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    return pEditContext->onEditProperties(this, pMap, TMapObjectRef(bSecondLayer, objID));
}

VA(0x00441251, 0x6a)
std::auto_ptr<TGameObject> TGUIPrison::clone() const
{
    return std::auto_ptr<TGameObject>(new TGUIPrison(*this));
}

// A town: its sprite shows the town, the castle or the capitol.
class TGUITown : public TTown, public TGUISpecializedObject {
public:
    TGUITown(const TObjectType& objType, TPlayer owner);
    TGUITown(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual bool edit(TEditContext* pEditContext, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual std::auto_ptr<TGameObject> clone() const;
    virtual void drawShadow(unsigned int frameNum, CDC* pDC, T16bppDIBSection* pDestBmp, int x, int y,
                            TZoom zoom) const;
    virtual void drawCellShadow(unsigned int frameNum, unsigned int cellX, unsigned int cellY, CDC* pDC,
                                T16bppDIBSection* pDestBmp, int x, int y, TZoom zoom) const;
    virtual TColor miniMapColor(TTerrainType terrainType) const;
    virtual bool isAnimated() const;
    virtual bool isOwnable() const { return true; }

protected:
    virtual void drawSpriteImpl(unsigned int frameNum, CDC* pDC, T16bppDIBSection* pDestBmp, int x, int y,
                                TZoom zoom) const;
    virtual void drawSpriteCellImpl(unsigned int frameNum, unsigned int cellX, unsigned int cellY, CDC* pDC,
                                    T16bppDIBSection* pDestBmp, int x, int y, TZoom zoom) const;
    virtual bool hitTestSpriteImpl(unsigned int frameNum, int x, int y, TZoom zoom) const;

private:
    const CSprite* _getPSprite() const;
    bool _hasCapitol() const { return getBuildingStates()[eBuildingCapitol].getBBuilt(); }
    bool _hasCastle() const { return getBuildingStates()[eBuildingFort].getBBuilt(); }
};

VA(0x00441418, 0xab)
TGUITown::TGUITown(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), TTown(objType, owner), TGUISpecializedObject(objType)
{
}

VA(0x004414c3, 0xae)
TGUITown::TGUITown(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TTown(objType, pIStream, version), TGUISpecializedObject(objType)
{
}

VA(0x00441571, 0x20)
bool TGUITown::edit(TEditContext* pEditContext, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    return pEditContext->onEditProperties(this, pMap, TMapObjectRef(bSecondLayer, objID));
}

VA(0x00441591, 0x6a)
std::auto_ptr<TGameObject> TGUITown::clone() const
{
    return std::auto_ptr<TGameObject>(new TGUITown(*this));
}

VA(0x004416ba, 0x53)
void TGUITown::drawShadow(unsigned int frameNum, CDC* pDC, T16bppDIBSection* pDestBmp, int x, int y,
                          TZoom zoom) const
{
    const CSprite* pSprite = _getPSprite();
    CRect clipRect;
    int region = pDC->GetClipBox(&clipRect);
    if (region != ERROR && region != NULLREGION)
        drawSpriteShadow(pSprite, getAnimOffset() + frameNum, pDestBmp, x, y, clipRect, zoom);
}

VA(0x0044170d, 0x59)
void TGUITown::drawCellShadow(unsigned int frameNum, unsigned int cellX, unsigned int cellY, CDC* pDC,
                              T16bppDIBSection* pDestBmp, int x, int y, TZoom zoom) const
{
    const CSprite* pSprite = _getPSprite();
    CRect clipRect;
    int region = pDC->GetClipBox(&clipRect);
    if (region != ERROR && region != NULLREGION)
        drawSpriteCellShadow(pSprite, getAnimOffset() + frameNum, cellX, cellY, pDestBmp, x, y, clipRect, zoom);
}

VA(0x00441766, 0x14)
TColor TGUITown::miniMapColor(TTerrainType terrainType) const
{
    return akPlayerColor[getOwner() + 1];
}

VA(0x0044177a, 0x1f)
bool TGUITown::isAnimated() const
{
    const CSprite* pSprite = _getPSprite();
    return pSprite->GetNumFrames(0) > 1;
}

VA(0x00441799, 0x59)
void TGUITown::drawSpriteImpl(unsigned int frameNum, CDC* pDC, T16bppDIBSection* pDestBmp, int x, int y,
                              TZoom zoom) const
{
    const CSprite* pSprite = _getPSprite();
    CRect clipRect;
    int region = pDC->GetClipBox(&clipRect);
    if (region != ERROR && region != NULLREGION)
        drawSprite(pSprite, getAnimOffset() + frameNum, getOwner(), pDestBmp, x, y, clipRect, zoom);
}

VA(0x004417f2, 0x5f)
void TGUITown::drawSpriteCellImpl(unsigned int frameNum, unsigned int cellX, unsigned int cellY, CDC* pDC,
                                  T16bppDIBSection* pDestBmp, int x, int y, TZoom zoom) const
{
    const CSprite* pSprite = _getPSprite();
    CRect clipRect;
    int region = pDC->GetClipBox(&clipRect);
    if (region != ERROR && region != NULLREGION)
        drawSpriteCell(pSprite, getAnimOffset() + frameNum, cellX, cellY, getOwner(), pDestBmp, x, y, clipRect,
                       zoom);
}

VA(0x00441851, 0x27)
bool TGUITown::hitTestSpriteImpl(unsigned int frameNum, int x, int y, TZoom zoom) const
{
    const CSprite* pSprite = _getPSprite();
    return hitTestSprite(pSprite, frameNum, x, y, zoom);
}

VA(0x00441878, 0x199)
const CSprite* TGUITown::_getPSprite() const
{
    struct TSprites {
        TResourcePtr<CSprite> m_pTown;
        TResourcePtr<CSprite> m_pCastle;
        TResourcePtr<CSprite> m_pCapitol;
    };
    DATA(0x00535818) static const char* const akSpriteNames[kNumTownTypes + 1][3] = {
        { "avccast0.def", "avccasx0.def", "avccasz0.def" },
        { "avcramp0.def", "avcramx0.def", "avcramz0.def" },
        { "avctowr0.def", "avctowx0.def", "avctowz0.def" },
        { "avcinft0.def", "avcinfx0.def", "avcinfz0.def" },
        { "avcnecr0.def", "avcnecx0.def", "avcnecz0.def" },
        { "avcdung0.def", "avcdunx0.def", "avcdunz0.def" },
        { "avcstro0.def", "avcstrx0.def", "avcstrz0.def" },
        { "avcftrt0.def", "avcftrx0.def", "avcforz0.def" },
        { "avchfor0.def", "avchforx.def", "avchforz.def" },
        { "avcrand0.def", "avcranx0.def", "avcranz0.def" }
    };
    DATA_COMPGEN_GUARD(0x0059e638, townSpritesGuard, aSprites)
    VA_COMPGEN(0x00441a55, 0x14, STATIC_DTOR, aSprites)
    DATA(0x0059e548) static TSprites aSprites[kNumTownTypes + 1];

    TTownType townType = getTownType();
    if (_hasCapitol()) {
        if (!aSprites[townType].m_pCapitol.get()) {
            CSprite* pSprite = ResourceManager::GetSprite(akSpriteNames[townType][2]);
            if (!pSprite)
                throw TRuntimeError();
            aSprites[townType].m_pCapitol = TResourcePtr<CSprite>(pSprite);
        }
        return aSprites[townType].m_pCapitol.get();
    }
    if (_hasCastle()) {
        if (!aSprites[townType].m_pCastle.get()) {
            CSprite* pSprite = ResourceManager::GetSprite(akSpriteNames[townType][1]);
            if (!pSprite)
                throw TRuntimeError();
            aSprites[townType].m_pCastle = TResourcePtr<CSprite>(pSprite);
        }
        return aSprites[townType].m_pCastle.get();
    }
    if (!aSprites[townType].m_pTown.get()) {
        CSprite* pSprite = ResourceManager::GetSprite(akSpriteNames[townType][0]);
        if (!pSprite)
            throw TRuntimeError();
        aSprites[townType].m_pTown = TResourcePtr<CSprite>(pSprite);
    }
    return aSprites[townType].m_pTown.get();
}

// An object of game class T with nothing to add but its edit dialog.
template <class T>
class TGUIStandardSpecializedObject : public T, public TGUISpecializedObject {
public:
    TGUIStandardSpecializedObject(const TObjectType& objType) : TGameObject(objType), T(objType),
                                                                TGUISpecializedObject(objType)
    {
    }
    TGUIStandardSpecializedObject(const TObjectType& objType, TRawIStream* pIStream, int version)
        : TGameObject(objType), T(objType, pIStream, version), TGUISpecializedObject(objType)
    {
    }

    virtual bool edit(TEditContext* pEditContext, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
    {
        return pEditContext->onEditProperties(this, pMap, TMapObjectRef(bSecondLayer, objID));
    }
    virtual std::auto_ptr<TGameObject> clone() const
    {
        return std::auto_ptr<TGameObject>(new TGUIStandardSpecializedObject<T>(*this));
    }
};

// An object of game class T that a player can own: drawn and coloured in
// its owner's colour.
template <class T>
class TGUIOwnableSpecializedObject : public T, public TGUISpecializedObject {
public:
    TGUIOwnableSpecializedObject(const TObjectType& objType, TPlayer owner)
        : TGameObject(objType), T(objType, owner), TGUISpecializedObject(objType)
    {
    }
    TGUIOwnableSpecializedObject(const TObjectType& objType, TRawIStream* pIStream, int version)
        : TGameObject(objType), T(objType, pIStream, version), TGUISpecializedObject(objType)
    {
    }

    virtual bool edit(TEditContext* pEditContext, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
    {
        return pEditContext->onEditProperties(this, pMap, TMapObjectRef(bSecondLayer, objID));
    }
    virtual std::auto_ptr<TGameObject> clone() const
    {
        return std::auto_ptr<TGameObject>(new TGUIOwnableSpecializedObject<T>(*this));
    }
    virtual TColor miniMapColor(TTerrainType terrainType) const { return akPlayerColor[getOwner() + 1]; }
    virtual bool isOwnable() const { return true; }

protected:
    virtual void drawSpriteImpl(unsigned int frameNum, CDC* pDC, T16bppDIBSection* pDestBmp, int x, int y,
                                TZoom zoom) const
    {
        TObjectSpritePtr pSprite(getObjectType());
        CRect clipRect;
        int region = pDC->GetClipBox(&clipRect);
        if (region != ERROR && region != NULLREGION)
            drawSprite(pSprite.get(), getAnimOffset() + frameNum, getOwner(), pDestBmp, x, y, clipRect, zoom);
    }
    virtual void drawSpriteCellImpl(unsigned int frameNum, unsigned int cellX, unsigned int cellY, CDC* pDC,
                                    T16bppDIBSection* pDestBmp, int x, int y, TZoom zoom) const
    {
        TObjectSpritePtr pSprite(getObjectType());
        CRect clipRect;
        int region = pDC->GetClipBox(&clipRect);
        if (region != ERROR && region != NULLREGION)
            drawSpriteCell(pSprite.get(), getAnimOffset() + frameNum, cellX, cellY, getOwner(), pDestBmp, x, y,
                           clipRect, zoom);
    }
};

// A creature generator: ownable only when its kind can be flagged.
class TGUIGenerator : public TGUIOwnableSpecializedObject<TGenerator> {
public:
    TGUIGenerator(const TObjectType& objType, TPlayer owner);
    TGUIGenerator(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual bool edit(TEditContext* pEditContext, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual std::auto_ptr<TGameObject> clone() const;
    virtual void draw(unsigned int frameNum, CDC* pDC, T16bppDIBSection* pDestBmp, int x, int y,
                      TZoom zoom) const;
    virtual void drawCell(unsigned int frameNum, unsigned int cellX, unsigned int cellY, CDC* pDC,
                          T16bppDIBSection* pDestBmp, int x, int y, TZoom zoom) const;
    virtual TColor miniMapColor(TTerrainType terrainType) const;
    virtual bool isOwnable() const { return _isOwnable(); }

private:
    bool _isOwnable() const { return getGeneratorTypeTraits().m_bFlaggable; }
};

VA(0x00441a69, 0x81)
TGUIGenerator::TGUIGenerator(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), TGUIOwnableSpecializedObject<TGenerator>(objType, owner)
{
}

VA(0x00441af5, 0x84)
TGUIGenerator::TGUIGenerator(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TGUIOwnableSpecializedObject<TGenerator>(objType, pIStream, version)
{
}

VA(0x00441b79, 0x59)
bool TGUIGenerator::edit(TEditContext* pEditContext, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    if (_isOwnable())
        return TGUIOwnableSpecializedObject<TGenerator>::edit(pEditContext, pMap, bSecondLayer, objID);
    return TGUIGameObject::edit(pEditContext, pMap, bSecondLayer, objID);
}

VA(0x00441bd2, 0x64)
std::auto_ptr<TGameObject> TGUIGenerator::clone() const
{
    return std::auto_ptr<TGameObject>(new TGUIGenerator(*this));
}

VA(0x00441e3f, 0x2f)
void TGUIGenerator::draw(unsigned int frameNum, CDC* pDC, T16bppDIBSection* pDestBmp, int x, int y,
                         TZoom zoom) const
{
    if (_isOwnable())
        TGUIOwnableSpecializedObject<TGenerator>::draw(frameNum, pDC, pDestBmp, x, y, zoom);
    else
        TGUISpecializedObject::draw(frameNum, pDC, pDestBmp, x, y, zoom);
}

VA(0x00441e6e, 0x35)
void TGUIGenerator::drawCell(unsigned int frameNum, unsigned int cellX, unsigned int cellY, CDC* pDC,
                             T16bppDIBSection* pDestBmp, int x, int y, TZoom zoom) const
{
    if (_isOwnable())
        TGUIOwnableSpecializedObject<TGenerator>::drawCell(frameNum, cellX, cellY, pDC, pDestBmp, x, y, zoom);
    else
        TGUISpecializedObject::drawCell(frameNum, cellX, cellY, pDC, pDestBmp, x, y, zoom);
}

VA(0x00441ea3, 0x2f)
TColor TGUIGenerator::miniMapColor(TTerrainType terrainType) const
{
    if (_isOwnable())
        return TGUIOwnableSpecializedObject<TGenerator>::miniMapColor(terrainType);
    return TGUIGameObject::miniMapColor(terrainType);
}

// An abandoned mine: neutral on the mini map.
class TGUIAbandonedMine : public TGUIStandardSpecializedObject<TAbandonedMine> {
public:
    TGUIAbandonedMine(const TObjectType& objType);
    TGUIAbandonedMine(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual std::auto_ptr<TGameObject> clone() const;
    virtual TColor miniMapColor(TTerrainType terrainType) const;
    virtual bool isOwnable() const { return true; }
};

VA(0x00441ed2, 0x7e)
TGUIAbandonedMine::TGUIAbandonedMine(const TObjectType& objType)
    : TGameObject(objType), TGUIStandardSpecializedObject<TAbandonedMine>(objType)
{
}

VA(0x00441f50, 0x84)
TGUIAbandonedMine::TGUIAbandonedMine(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TGUIStandardSpecializedObject<TAbandonedMine>(objType, pIStream, version)
{
}

VA(0x00441fd4, 0x64)
std::auto_ptr<TGameObject> TGUIAbandonedMine::clone() const
{
    return std::auto_ptr<TGameObject>(new TGUIAbandonedMine(*this));
}

VA(0x0044226a, 0xb)
TColor TGUIAbandonedMine::miniMapColor(TTerrainType terrainType) const
{
    return akPlayerColor[0];
}

// The object types the mini map draws in the terrain's obstacle colour:
// Loki's list, then Complete's second set of obstacles (their type names
// are not recorded).
class TMiniMapTypeFlags : public bitset<ADVENTURE_OBJECT_TRAIT_COUNT> {
public:
    TMiniMapTypeFlags();
};

VA(0x00442275, 0x2a)
TMiniMapTypeFlags::TMiniMapTypeFlags()
{
    DATA(0x00535890) static const TAdventureObjectType akTypes[] = {
        TERRAIN_CACTUS, TERRAIN_CRATER, TERRAIN_DEAD_VEGETATION, TERRAIN_FROZEN_LAKE, TERRAIN_LAKE,
        TERRAIN_LAVA_LAKE, TERRAIN_HILL, TERRAIN_MANDRAKE, TERRAIN_MOUND, TERRAIN_MOUNTAIN,
        TERRAIN_OAK_TREE, TERRAIN_PINE_TREE, TERRAIN_SAND_DUNE, TERRAIN_SAND_PIT, TERRAIN_TAR_PIT,
        TERRAIN_STALAGMITE, TERRAIN_STUMP, TERRAIN_TREE, TERRAIN_VOLCANO, TERRAIN_WILLOW_TREE,
        TERRAIN_YUCCA_TREE,
        TAdventureObjectType(167), TAdventureObjectType(169), TAdventureObjectType(170),
        TAdventureObjectType(172), TAdventureObjectType(177), TAdventureObjectType(179),
        TAdventureObjectType(174), TAdventureObjectType(182), TAdventureObjectType(184),
        TAdventureObjectType(185), TAdventureObjectType(186), TAdventureObjectType(188),
        TAdventureObjectType(192), TAdventureObjectType(193), TAdventureObjectType(198),
        TAdventureObjectType(196), TAdventureObjectType(197), TAdventureObjectType(199),
        TAdventureObjectType(202), TAdventureObjectType(203), TAdventureObjectType(204),
        TAdventureObjectType(206), TAdventureObjectType(207), TAdventureObjectType(208),
        TAdventureObjectType(209), TAdventureObjectType(210), TAdventureObjectType(211)
    };
    for (unsigned int i = 0; i < sizeof(akTypes) / sizeof(akTypes[0]); i++)
        (*this)[akTypes[i]] = true;
}

}

VA(0x0044229f, 0x46)
TGUIGameObject::TGUIGameObject(const TObjectType& objType) : TGameObject(objType), _m_animOffset(rand())
{
}

VA(0x00442305, 0x55)
TGUIGameObject::TGUIGameObject(const TGUIGameObject& other) : TGameObject(other), _m_animOffset(rand())
{
}

VA(0x0044235a, 0x2b)
bool TGUIGameObject::edit(TEditContext* pEditContext, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    return pEditContext->onEditProperties(this, pMap, TMapObjectRef(bSecondLayer, objID));
}

VA(0x00442385, 0x88)
void TGUIGameObject::draw(unsigned int frameNum, CDC* pDC, T16bppDIBSection* pDestBmp, int x, int y,
                          TZoom zoom) const
{
    TObjectSpritePtr pSprite(getObjectType());
    CRect clipRect;
    int region = pDC->GetClipBox(&clipRect);
    if (region != ERROR && region != NULLREGION)
        drawSprite(pSprite.get(), getAnimOffset() + frameNum, pDestBmp, x, y, clipRect, zoom);
}

VA(0x0044240d, 0x88)
void TGUIGameObject::drawShadow(unsigned int frameNum, CDC* pDC, T16bppDIBSection* pDestBmp, int x, int y,
                                TZoom zoom) const
{
    TObjectSpritePtr pSprite(getObjectType());
    CRect clipRect;
    int region = pDC->GetClipBox(&clipRect);
    if (region != ERROR && region != NULLREGION)
        drawSpriteShadow(pSprite.get(), getAnimOffset() + frameNum, pDestBmp, x, y, clipRect, zoom);
}

VA(0x00442495, 0x8e)
void TGUIGameObject::drawCell(unsigned int frameNum, unsigned int cellX, unsigned int cellY, CDC* pDC,
                              T16bppDIBSection* pDestBmp, int x, int y, TZoom zoom) const
{
    TObjectSpritePtr pSprite(getObjectType());
    CRect clipRect;
    int region = pDC->GetClipBox(&clipRect);
    if (region != ERROR && region != NULLREGION)
        drawSpriteCell(pSprite.get(), getAnimOffset() + frameNum, cellX, cellY, pDestBmp, x, y, clipRect, zoom);
}

VA(0x00442523, 0x8e)
void TGUIGameObject::drawCellShadow(unsigned int frameNum, unsigned int cellX, unsigned int cellY, CDC* pDC,
                                    T16bppDIBSection* pDestBmp, int x, int y, TZoom zoom) const
{
    TObjectSpritePtr pSprite(getObjectType());
    CRect clipRect;
    int region = pDC->GetClipBox(&clipRect);
    if (region != ERROR && region != NULLREGION)
        drawSpriteCellShadow(pSprite.get(), getAnimOffset() + frameNum, cellX, cellY, pDestBmp, x, y, clipRect,
                             zoom);
}

VA(0x004425b1, 0x6e)
bool TGUIGameObject::hitTest(unsigned int frameNum, int x, int y, TZoom zoom) const
{
    TObjectSpritePtr pSprite(getObjectType());
    return hitTestSprite(pSprite.get(), getAnimOffset() + frameNum, x, y, zoom);
}

VA(0x0044261f, 0x77)
TColor TGUIGameObject::miniMapColor(TTerrainType terrainType) const
{
    DATA_COMPGEN_GUARD(0x0059e540, miniMapTypeFlagsGuard, typeFlags)
    VA_COMPGEN(0x00442696, 0x1, STATIC_DTOR, typeFlags)
    DATA(0x0059e520) static const TMiniMapTypeFlags typeFlags;
    if (typeFlags[getType()])
        return akTerrainColors[terrainType].m_obstacleColor;
    return akTerrainColors[terrainType].m_color;
}

VA(0x00442697, 0x45)
bool TGUIGameObject::isAnimated() const
{
    TObjectSpritePtr pSprite(getObjectType());
    return pSprite.get()->GetNumFrames(0) > 1;
}

bool TGUIGameObject::isOwnable() const
{
    return false;
}

VA(0x004426dc, 0x56)
std::auto_ptr<TGenericObject> TGUIGameObjectFactory::createGenericObject(const TObjectType& objType) const
{
    return std::auto_ptr<TGenericObject>(new TGUIGenericObject(objType));
}

VA(0x00442732, 0x5c)
std::auto_ptr<TGenericObject> TGUIGameObjectFactory::createGenericObject(const TObjectType& objType,
                                                                         TRawIStream* pIStream, int version) const
{
    return std::auto_ptr<TGenericObject>(new TGUIGenericObject(objType, pIStream, version));
}

VA(0x0044278e, 0x52)
std::auto_ptr<THeroPlaceholder> TGUIGameObjectFactory::createHeroPlaceholder(const TObjectType& objType,
                                                                             TPlayer owner) const
{
    return std::auto_ptr<THeroPlaceholder>(new TGUIHeroPlaceholder(objType, owner));
}

VA(0x004427e0, 0x55)
std::auto_ptr<THeroPlaceholder> TGUIGameObjectFactory::createHeroPlaceholder(const TObjectType& objType,
                                                                             TRawIStream* pIStream,
                                                                             int version) const
{
    return std::auto_ptr<THeroPlaceholder>(new TGUIHeroPlaceholder(objType, pIStream, version));
}

VA(0x00442835, 0x58)
std::auto_ptr<TNonRandomHero> TGUIGameObjectFactory::createNonRandomHero(const TObjectType& objType, TPlayer owner,
                                                                         THeroID heroID) const
{
    return std::auto_ptr<TNonRandomHero>(new TGUINonRandomHero(objType, owner, heroID));
}

VA(0x0044288d, 0x58)
std::auto_ptr<TNonRandomHero> TGUIGameObjectFactory::createNonRandomHero(const TObjectType& objType,
                                                                         TRawIStream* pIStream, int version) const
{
    return std::auto_ptr<TNonRandomHero>(new TGUINonRandomHero(objType, pIStream, version));
}

VA(0x004428e5, 0x55)
std::auto_ptr<TRandomHero> TGUIGameObjectFactory::createRandomHero(const TObjectType& objType, TPlayer owner) const
{
    return std::auto_ptr<TRandomHero>(new TGUIRandomHero(objType, owner));
}

VA(0x0044293a, 0x58)
std::auto_ptr<TRandomHero> TGUIGameObjectFactory::createRandomHero(const TObjectType& objType, TRawIStream* pIStream,
                                                                   int version) const
{
    return std::auto_ptr<TRandomHero>(new TGUIRandomHero(objType, pIStream, version));
}

VA(0x00442992, 0x55)
std::auto_ptr<TPrison> TGUIGameObjectFactory::createPrison(const TObjectType& objType, THeroID heroID) const
{
    return std::auto_ptr<TPrison>(new TGUIPrison(objType, heroID));
}

VA(0x004429e7, 0x58)
std::auto_ptr<TPrison> TGUIGameObjectFactory::createPrison(const TObjectType& objType, TRawIStream* pIStream,
                                                           int version) const
{
    return std::auto_ptr<TPrison>(new TGUIPrison(objType, pIStream, version));
}

VA(0x00442a3f, 0x55)
std::auto_ptr<TTown> TGUIGameObjectFactory::createTown(const TObjectType& objType, TPlayer owner) const
{
    return std::auto_ptr<TTown>(new TGUITown(objType, owner));
}

VA(0x00442a94, 0x58)
std::auto_ptr<TTown> TGUIGameObjectFactory::createTown(const TObjectType& objType, TRawIStream* pIStream,
                                                       int version) const
{
    return std::auto_ptr<TTown>(new TGUITown(objType, pIStream, version));
}

VA(0x00442aec, 0x56)
std::auto_ptr<TEvent> TGUIGameObjectFactory::createEvent(const TObjectType& objType) const
{
    return std::auto_ptr<TEvent>(new TGUIStandardSpecializedObject<TEvent>(objType));
}

VA(0x00442b42, 0x5c)
std::auto_ptr<TEvent> TGUIGameObjectFactory::createEvent(const TObjectType& objType, TRawIStream* pIStream,
                                                         int version) const
{
    return std::auto_ptr<TEvent>(new TGUIStandardSpecializedObject<TEvent>(objType, pIStream, version));
}

VA(0x00442b9e, 0x4f)
std::auto_ptr<TMonster> TGUIGameObjectFactory::createMonster(const TObjectType& objType) const
{
    return std::auto_ptr<TMonster>(new TGUIStandardSpecializedObject<TMonster>(objType));
}

VA(0x00442bed, 0x55)
std::auto_ptr<TMonster> TGUIGameObjectFactory::createMonster(const TObjectType& objType, TRawIStream* pIStream,
                                                             int version) const
{
    return std::auto_ptr<TMonster>(new TGUIStandardSpecializedObject<TMonster>(objType, pIStream, version));
}

VA(0x00442c42, 0x56)
std::auto_ptr<TSign> TGUIGameObjectFactory::createSign(const TObjectType& objType) const
{
    return std::auto_ptr<TSign>(new TGUIStandardSpecializedObject<TSign>(objType));
}

VA(0x00442c98, 0x5c)
std::auto_ptr<TSign> TGUIGameObjectFactory::createSign(const TObjectType& objType, TRawIStream* pIStream,
                                                       int version) const
{
    return std::auto_ptr<TSign>(new TGUIStandardSpecializedObject<TSign>(objType, pIStream, version));
}

VA(0x00442cf4, 0x59)
std::auto_ptr<TFlaggableObject> TGUIGameObjectFactory::createFlaggable(const TObjectType& objType,
                                                                       TPlayer owner) const
{
    return std::auto_ptr<TFlaggableObject>(new TGUIOwnableSpecializedObject<TFlaggableObject>(objType, owner));
}

VA(0x00442d4d, 0x5c)
std::auto_ptr<TFlaggableObject> TGUIGameObjectFactory::createFlaggable(const TObjectType& objType,
                                                                       TRawIStream* pIStream, int version) const
{
    return std::auto_ptr<TFlaggableObject>(
        new TGUIOwnableSpecializedObject<TFlaggableObject>(objType, pIStream, version));
}

VA(0x00442da9, 0x59)
std::auto_ptr<TMine> TGUIGameObjectFactory::createMine(const TObjectType& objType, TPlayer owner) const
{
    return std::auto_ptr<TMine>(new TGUIOwnableSpecializedObject<TMine>(objType, owner));
}

VA(0x00442e02, 0x5c)
std::auto_ptr<TMine> TGUIGameObjectFactory::createMine(const TObjectType& objType, TRawIStream* pIStream,
                                                       int version) const
{
    return std::auto_ptr<TMine>(new TGUIOwnableSpecializedObject<TMine>(objType, pIStream, version));
}

VA(0x00442e5e, 0x56)
std::auto_ptr<TAbandonedMine> TGUIGameObjectFactory::createAbandonedMine(const TObjectType& objType) const
{
    return std::auto_ptr<TAbandonedMine>(new TGUIAbandonedMine(objType));
}

VA(0x00442eb4, 0x5c)
std::auto_ptr<TAbandonedMine> TGUIGameObjectFactory::createAbandonedMine(const TObjectType& objType,
                                                                         TRawIStream* pIStream, int version) const
{
    return std::auto_ptr<TAbandonedMine>(new TGUIAbandonedMine(objType, pIStream, version));
}

VA(0x00442f10, 0x59)
std::auto_ptr<TGarrison> TGUIGameObjectFactory::createGarrison(const TObjectType& objType, TPlayer owner) const
{
    return std::auto_ptr<TGarrison>(new TGUIOwnableSpecializedObject<TGarrison>(objType, owner));
}

VA(0x00442f69, 0x5c)
std::auto_ptr<TGarrison> TGUIGameObjectFactory::createGarrison(const TObjectType& objType, TRawIStream* pIStream,
                                                               int version) const
{
    return std::auto_ptr<TGarrison>(new TGUIOwnableSpecializedObject<TGarrison>(objType, pIStream, version));
}

VA(0x00442fc5, 0x56)
std::auto_ptr<TGameArtifact> TGUIGameObjectFactory::createArtifact(const TObjectType& objType) const
{
    return std::auto_ptr<TGameArtifact>(new TGUIStandardSpecializedObject<TGameArtifact>(objType));
}

VA(0x0044301b, 0x5c)
std::auto_ptr<TGameArtifact> TGUIGameObjectFactory::createArtifact(const TObjectType& objType,
                                                                   TRawIStream* pIStream, int version) const
{
    return std::auto_ptr<TGameArtifact>(new TGUIStandardSpecializedObject<TGameArtifact>(objType, pIStream, version));
}

VA(0x00443077, 0x56)
std::auto_ptr<TSpellScroll> TGUIGameObjectFactory::createSpellScroll(const TObjectType& objType) const
{
    return std::auto_ptr<TSpellScroll>(new TGUIStandardSpecializedObject<TSpellScroll>(objType));
}

VA(0x004430cd, 0x5c)
std::auto_ptr<TSpellScroll> TGUIGameObjectFactory::createSpellScroll(const TObjectType& objType,
                                                                     TRawIStream* pIStream, int version) const
{
    return std::auto_ptr<TSpellScroll>(new TGUIStandardSpecializedObject<TSpellScroll>(objType, pIStream, version));
}

VA(0x00443129, 0x56)
std::auto_ptr<TGameResource> TGUIGameObjectFactory::createResource(const TObjectType& objType) const
{
    return std::auto_ptr<TGameResource>(new TGUIStandardSpecializedObject<TGameResource>(objType));
}

VA(0x0044317f, 0x5c)
std::auto_ptr<TGameResource> TGUIGameObjectFactory::createResource(const TObjectType& objType,
                                                                   TRawIStream* pIStream, int version) const
{
    return std::auto_ptr<TGameResource>(new TGUIStandardSpecializedObject<TGameResource>(objType, pIStream, version));
}

VA(0x004431db, 0x56)
std::auto_ptr<TBlackBox> TGUIGameObjectFactory::createBlackBox(const TObjectType& objType) const
{
    return std::auto_ptr<TBlackBox>(new TGUIStandardSpecializedObject<TBlackBox>(objType));
}

VA(0x00443231, 0x5c)
std::auto_ptr<TBlackBox> TGUIGameObjectFactory::createBlackBox(const TObjectType& objType, TRawIStream* pIStream,
                                                               int version) const
{
    return std::auto_ptr<TBlackBox>(new TGUIStandardSpecializedObject<TBlackBox>(objType, pIStream, version));
}

VA(0x0044328d, 0x56)
std::auto_ptr<TScholar> TGUIGameObjectFactory::createScholar(const TObjectType& objType) const
{
    return std::auto_ptr<TScholar>(new TGUIStandardSpecializedObject<TScholar>(objType));
}

VA(0x004432e3, 0x5c)
std::auto_ptr<TScholar> TGUIGameObjectFactory::createScholar(const TObjectType& objType, TRawIStream* pIStream,
                                                             int version) const
{
    return std::auto_ptr<TScholar>(new TGUIStandardSpecializedObject<TScholar>(objType, pIStream, version));
}

VA(0x0044333f, 0x4f)
std::auto_ptr<TSeersHut> TGUIGameObjectFactory::createSeersHut(const TObjectType& objType) const
{
    return std::auto_ptr<TSeersHut>(new TGUIStandardSpecializedObject<TSeersHut>(objType));
}

VA(0x0044338e, 0x55)
std::auto_ptr<TSeersHut> TGUIGameObjectFactory::createSeersHut(const TObjectType& objType, TRawIStream* pIStream,
                                                               int version) const
{
    return std::auto_ptr<TSeersHut>(new TGUIStandardSpecializedObject<TSeersHut>(objType, pIStream, version));
}

VA(0x004433e3, 0x56)
std::auto_ptr<THolyGrail> TGUIGameObjectFactory::createHolyGrail(const TObjectType& objType) const
{
    return std::auto_ptr<THolyGrail>(new TGUIStandardSpecializedObject<THolyGrail>(objType));
}

VA(0x00443439, 0x5c)
std::auto_ptr<THolyGrail> TGUIGameObjectFactory::createHolyGrail(const TObjectType& objType, TRawIStream* pIStream,
                                                                 int version) const
{
    return std::auto_ptr<THolyGrail>(new TGUIStandardSpecializedObject<THolyGrail>(objType, pIStream, version));
}

VA(0x00443495, 0x56)
std::auto_ptr<TShrine> TGUIGameObjectFactory::createShrine(const TObjectType& objType) const
{
    return std::auto_ptr<TShrine>(new TGUIStandardSpecializedObject<TShrine>(objType));
}

VA(0x004434eb, 0x5c)
std::auto_ptr<TShrine> TGUIGameObjectFactory::createShrine(const TObjectType& objType, TRawIStream* pIStream,
                                                           int version) const
{
    return std::auto_ptr<TShrine>(new TGUIStandardSpecializedObject<TShrine>(objType, pIStream, version));
}

VA(0x00443547, 0x59)
std::auto_ptr<TGenerator> TGUIGameObjectFactory::createGenerator(const TObjectType& objType, TPlayer owner) const
{
    return std::auto_ptr<TGenerator>(new TGUIGenerator(objType, owner));
}

VA(0x004435a0, 0x5c)
std::auto_ptr<TGenerator> TGUIGameObjectFactory::createGenerator(const TObjectType& objType, TRawIStream* pIStream,
                                                                 int version) const
{
    return std::auto_ptr<TGenerator>(new TGUIGenerator(objType, pIStream, version));
}

VA(0x004435fc, 0x52)
std::auto_ptr<TRandomlyAlignedGenerator> TGUIGameObjectFactory::createRandomlyAlignedGenerator(
    const TObjectType& objType, TPlayer owner) const
{
    return std::auto_ptr<TRandomlyAlignedGenerator>(
        new TGUIOwnableSpecializedObject<TRandomlyAlignedGenerator>(objType, owner));
}

VA(0x0044364e, 0x55)
std::auto_ptr<TRandomlyAlignedGenerator> TGUIGameObjectFactory::createRandomlyAlignedGenerator(
    const TObjectType& objType, TRawIStream* pIStream, int version) const
{
    return std::auto_ptr<TRandomlyAlignedGenerator>(
        new TGUIOwnableSpecializedObject<TRandomlyAlignedGenerator>(objType, pIStream, version));
}

VA(0x004436a3, 0x52)
std::auto_ptr<TRandomlyLeveledGenerator> TGUIGameObjectFactory::createRandomlyLeveledGenerator(
    const TObjectType& objType, TPlayer owner) const
{
    return std::auto_ptr<TRandomlyLeveledGenerator>(
        new TGUIOwnableSpecializedObject<TRandomlyLeveledGenerator>(objType, owner));
}

VA(0x004436f5, 0x55)
std::auto_ptr<TRandomlyLeveledGenerator> TGUIGameObjectFactory::createRandomlyLeveledGenerator(
    const TObjectType& objType, TRawIStream* pIStream, int version) const
{
    return std::auto_ptr<TRandomlyLeveledGenerator>(
        new TGUIOwnableSpecializedObject<TRandomlyLeveledGenerator>(objType, pIStream, version));
}

VA(0x0044374a, 0x52)
std::auto_ptr<TRandomGenerator> TGUIGameObjectFactory::createRandomGenerator(const TObjectType& objType,
                                                                             TPlayer owner) const
{
    return std::auto_ptr<TRandomGenerator>(new TGUIOwnableSpecializedObject<TRandomGenerator>(objType, owner));
}

VA(0x0044379c, 0x55)
std::auto_ptr<TRandomGenerator> TGUIGameObjectFactory::createRandomGenerator(const TObjectType& objType,
                                                                             TRawIStream* pIStream,
                                                                             int version) const
{
    return std::auto_ptr<TRandomGenerator>(
        new TGUIOwnableSpecializedObject<TRandomGenerator>(objType, pIStream, version));
}

VA(0x004437f1, 0x4f)
std::auto_ptr<TQuestGuard> TGUIGameObjectFactory::createQuestGuard(const TObjectType& objType) const
{
    return std::auto_ptr<TQuestGuard>(new TGUIStandardSpecializedObject<TQuestGuard>(objType));
}

VA(0x00443840, 0x55)
std::auto_ptr<TQuestGuard> TGUIGameObjectFactory::createQuestGuard(const TObjectType& objType,
                                                                   TRawIStream* pIStream, int version) const
{
    return std::auto_ptr<TQuestGuard>(new TGUIStandardSpecializedObject<TQuestGuard>(objType, pIStream, version));
}

VA(0x00443895, 0x56)
std::auto_ptr<TWitchHut> TGUIGameObjectFactory::createWitchHut(const TObjectType& objType) const
{
    return std::auto_ptr<TWitchHut>(new TGUIStandardSpecializedObject<TWitchHut>(objType));
}

VA(0x004438eb, 0x5c)
std::auto_ptr<TWitchHut> TGUIGameObjectFactory::createWitchHut(const TObjectType& objType, TRawIStream* pIStream,
                                                               int version) const
{
    return std::auto_ptr<TWitchHut>(new TGUIStandardSpecializedObject<TWitchHut>(objType, pIStream, version));
}
