// GUIGameObject.cpp - Loki h3maped object 2: the editor's map objects. Each
// game object class gets a GUI counterpart that draws its sprite (and the
// owner's flag or the hero's flag), hit-tests it, colours it on the mini map
// and hands itself to the edit context; TGUIGameObjectFactory creates them.
// Assert and throw lines come from the retail immediates. The names of the
// clip helper, the town sprite table and the mini map's type list are not
// recorded. The image's type_info functions show the victory and loss
// conditions in this unit, so VictoryCondition.h is included.
#include "editor/stdafx.h"

#include <assert.h>
#include <new>
#include <bitset>
#include <string>

#include "adventureobjecttype.h"
#include "csprite.h"
#include "cspriteframe.h"
#include "exceptions.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "editor/GameMap.h"
#include "editor/GUIGameObject.h"
#include "editor/BlackBox.h"
#include "editor/Colors.h"
#include "editor/Event.h"
#include "editor/Hero.h"
#include "editor/Monster.h"
#include "editor/ObjectSpecializations.h"
#include "editor/ObjectSprites.h"
#include "editor/Town.h"
#include "editor/SeersHut.h"
#include "editor/Tile.h"
#include "editor/VictoryCondition.h"

#define ARRAY_SIZE( a ) ( sizeof( a ) / sizeof( ( a )[ 0 ] ) )

// A 9x9 check mark, one string per pixel row ('!' outline, '.' fill, '-'
// clear), in a 16-entry row table. Nothing in the image reads it; the
// layout is GUIGameObject's whole .data (nine row pointers, seven null
// entries) and the rows' strings precede drawSprite's in .rodata. The
// name is not proven.
static const char* akCheckMarkRows[16] = {
    "-------!-",
    "------!.!",
    "-!---!..!",
    "!.!-!...!",
    "!..!...!-",
    "!.....!--",
    "-!...!---",
    "--!.!----",
    "---!-----"
};

// The clip rectangle a draw call honours: the GC's clip mask, or the whole
// image.
static inline void getClipRect(GdkGC* gc, CRect& clipRect, GdkImage* pImage)
{
    GdkGCValues values;
    gdk_gc_get_values(gc, &values);
    if (values.clip_mask != NULL) {
        gdk_window_get_size(values.clip_mask, &clipRect.right, &clipRect.bottom);
        clipRect.left = values.clip_x_origin;
        clipRect.top = values.clip_y_origin;
        clipRect.right += clipRect.left;
        clipRect.bottom += clipRect.top;
    } else {
        clipRect.left = clipRect.top = 0;
        clipRect.right = pImage->width;
        clipRect.bottom = pImage->height;
    }
}

namespace {

void drawSprite(const CSprite* pSprite, unsigned int frameNum, GdkImage* pDestBmp, int x, int y,
                const CRect& destClipRect, TZoom zoom)
{
#line 119
    assert(pSprite != NULL);
    assert(pDestBmp != NULL);
    assert(destClipRect.right >= destClipRect.left && destClipRect.bottom >= destClipRect.top);
#line 123
    assert(destClipRect.Width() <= pDestBmp->width && destClipRect.Height() <= pDestBmp->height);
    assert(zoom >= 0 && zoom < kNumZooms);
    unsigned int frame = frameNum % pSprite->GetNumFrames(0);
    const TZoomTraits& zoomTraits = akZoomTraits[zoom];
    int width = pSprite->GetWidth() >> zoomTraits.m_scaleShift;
    int height = pSprite->GetHeight() >> zoomTraits.m_scaleShift;
    x -= width - 1;
    y -= height - 1;
    zoomTraits.m_pDrawAdvObj(pSprite, frame, 0, 0, width, height, x - destClipRect.left, y - destClipRect.top,
                                     (uword*)getImgLine(pDestBmp, destClipRect.top) + destClipRect.left,
                                     destClipRect.Width(), destClipRect.Height(), getImgPitch(pDestBmp), 0);
}

void drawSprite(const CSprite* pSprite, unsigned int frameNum, TPlayer player, GdkImage* pDestBmp,
                int x, int y, const CRect& destClipRect, TZoom zoom)
{
#line 157
    assert(pSprite != NULL);
    assert(player >= ePlayerNone && player < kNumPlayers);
    assert(pDestBmp != NULL);
    assert(destClipRect.right >= destClipRect.left && destClipRect.bottom >= destClipRect.top);
#line 162
    assert(destClipRect.Width() <= pDestBmp->width && destClipRect.Height() <= pDestBmp->height);
    assert(zoom >= 0 && zoom < kNumZooms);
    unsigned int frame = frameNum % pSprite->GetNumFrames(0);
    const TZoomTraits& zoomTraits = akZoomTraits[zoom];
    int width = pSprite->GetWidth() >> zoomTraits.m_scaleShift;
    int height = pSprite->GetHeight() >> zoomTraits.m_scaleShift;
    x -= width - 1;
    y -= height - 1;
    zoomTraits.m_pDrawAdvObj(pSprite, frame, 0, 0, width, height, x - destClipRect.left, y - destClipRect.top,
                                     (uword*)getImgLine(pDestBmp, destClipRect.top) + destClipRect.left,
                                     destClipRect.Width(), destClipRect.Height(), getImgPitch(pDestBmp),
                                     akPlayerColor[player + 1]);
}

void drawSpriteShadow(const CSprite* pSprite, unsigned int frameNum, GdkImage* pDestBmp, int x, int y,
                      const CRect& destClipRect, TZoom zoom)
{
#line 196
    assert(pSprite != NULL);
    assert(pDestBmp != NULL);
    assert(destClipRect.right >= destClipRect.left && destClipRect.bottom >= destClipRect.top);
#line 200
    assert(destClipRect.Width() <= pDestBmp->width && destClipRect.Height() <= pDestBmp->height);
    assert(zoom >= 0 && zoom < kNumZooms);
    unsigned int frame = frameNum % pSprite->GetNumFrames(0);
    const TZoomTraits& zoomTraits = akZoomTraits[zoom];
    int width = pSprite->GetWidth() >> zoomTraits.m_scaleShift;
    int height = pSprite->GetHeight() >> zoomTraits.m_scaleShift;
    x -= width - 1;
    y -= height - 1;
    zoomTraits.m_pDrawAdvObjShadow(pSprite, frame, 0, 0, width, height, x - destClipRect.left,
                                   y - destClipRect.top,
                                           (uword*)getImgLine(pDestBmp, destClipRect.top) + destClipRect.left,
                                           destClipRect.Width(), destClipRect.Height(), getImgPitch(pDestBmp));
}

void drawSpriteCell(const CSprite* pSprite, unsigned int frameNum, unsigned int cellX, unsigned int cellY,
                    GdkImage* pDestBmp, int x, int y, const CRect& destClipRect, TZoom zoom)
{
#line 245
    assert(pSprite != NULL);
    assert(pDestBmp != NULL);
    assert(destClipRect.right >= destClipRect.left && destClipRect.bottom >= destClipRect.top);
#line 249
    assert(destClipRect.Width() <= pDestBmp->width && destClipRect.Height() <= pDestBmp->height);
    assert(zoom >= 0 && zoom < kNumZooms);
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
    zoomTraits.m_pDrawAdvObj(pSprite, frame,
                                     srcX, srcY,
                                     cellWidth, cellHeight,
                                     x - destClipRect.left, y - destClipRect.top,
                                     (uword*)getImgLine(pDestBmp, destClipRect.top) + destClipRect.left,
                                     destClipRect.Width(), destClipRect.Height(), getImgPitch(pDestBmp), 0);
}

void drawSpriteCellShadow(const CSprite* pSprite, unsigned int frameNum, unsigned int cellX,
                          unsigned int cellY, GdkImage* pDestBmp, int x, int y, const CRect& destClipRect,
                          TZoom zoom)
{
#line 298
    assert(pSprite != NULL);
    assert(pDestBmp != NULL);
    assert(destClipRect.right >= destClipRect.left && destClipRect.bottom >= destClipRect.top);
#line 302
    assert(destClipRect.Width() <= pDestBmp->width && destClipRect.Height() <= pDestBmp->height);
    assert(zoom >= 0 && zoom < kNumZooms);
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
    zoomTraits.m_pDrawAdvObjShadow(pSprite, frame,
                                           srcX, srcY,
                                           cellWidth, cellHeight,
                                           x - destClipRect.left, y - destClipRect.top,
                                           (uword*)getImgLine(pDestBmp, destClipRect.top) + destClipRect.left,
                                           destClipRect.Width(), destClipRect.Height(), getImgPitch(pDestBmp));
}

void drawSpriteCell(const CSprite* pSprite, unsigned int frameNum, unsigned int cellX, unsigned int cellY,
                    TPlayer player, GdkImage* pDestBmp, int x, int y, const CRect& destClipRect, TZoom zoom)
{
#line 351
    assert(pSprite != NULL);
    assert(player >= ePlayerNone && player < kNumPlayers);
    assert(pDestBmp != NULL);
    assert(destClipRect.right >= destClipRect.left && destClipRect.bottom >= destClipRect.top);
#line 356
    assert(destClipRect.Width() <= pDestBmp->width && destClipRect.Height() <= pDestBmp->height);
    assert(zoom >= 0 && zoom < kNumZooms);
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
    zoomTraits.m_pDrawAdvObj(pSprite, frame,
                                     srcX, srcY,
                                     cellWidth, cellHeight, x - destClipRect.left, y - destClipRect.top,
                                     (uword*)getImgLine(pDestBmp, destClipRect.top) + destClipRect.left,
                                     destClipRect.Width(), destClipRect.Height(), getImgPitch(pDestBmp),
                                     akPlayerColor[player + 1]);
}

bool hitTestSprite(const CSprite* pSprite, unsigned int frameNum, int x, int y, TZoom zoom)
{
#line 395
    assert(pSprite != NULL);
    assert(zoom >= 0 && zoom < kNumZooms);
    unsigned int frame = frameNum % pSprite->GetNumFrames(0);
    const TZoomTraits& zoomTraits = akZoomTraits[zoom];
    int width = pSprite->GetWidth() >> zoomTraits.m_scaleShift;
    int height = pSprite->GetHeight() >> zoomTraits.m_scaleShift;
    const CSpriteFrame* pSpriteFrame = ((CSprite*)pSprite)->GetFrame(0, frame);
#line 406
    assert(pSpriteFrame != NULL);
    unsigned char pixel = pSpriteFrame->GetPixel((width - x - 1) << zoomTraits.m_scaleShift,
                                                 (height - y - 1) << zoomTraits.m_scaleShift);
    return pixel > 4;
}

// A generic object.
class TGUIGenericObject : public TGenericObject, public TGUIGameObject {
public:
    TGUIGenericObject(const TObjectType& objType);
    TGUIGenericObject(const TObjectType& objType, TRawIStream* pIStream, int version);
    virtual ~TGUIGenericObject() {}

    virtual TGameObject* clone(void* (*pfnAllocator)(unsigned int)) const;
};

TGUIGenericObject::TGUIGenericObject(const TObjectType& objType)
    : TGameObject(objType), TGenericObject(objType), TGUIGameObject(objType)
{
}

TGUIGenericObject::TGUIGenericObject(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TGenericObject(objType, pIStream, version), TGUIGameObject(objType)
{
}

TGameObject* TGUIGenericObject::clone(void* (*pfnAllocator)(unsigned int)) const
{
#line 447
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIGenericObject))) TGUIGenericObject(*this);
}

// An object with a game class of its own: drawing goes through the
// sprite hooks, and a customized object would carry a check mark.
class TGUISpecializedObject : public TGUIGameObject {
public:
    TGUISpecializedObject(const TObjectType& objType);

    virtual void draw(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp, int x, int y,
                      TZoom zoom) const;
    virtual void drawCell(unsigned int frameNum, unsigned int cellX, unsigned int cellY, GdkGC* gc,
                          GdkImage* pDestBmp, int x, int y, TZoom zoom) const;
    virtual bool hitTest(unsigned int frameNum, int x, int y, TZoom zoom) const;

protected:
    virtual void drawSpriteImpl(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp, int x, int y,
                                TZoom zoom) const;
    virtual void drawSpriteCellImpl(unsigned int frameNum, unsigned int cellX, unsigned int cellY,
                                    GdkGC* gc, GdkImage* pDestBmp, int x, int y, TZoom zoom) const;
    virtual bool hitTestSpriteImpl(unsigned int frameNum, int x, int y, TZoom zoom) const;
};

TGUISpecializedObject::TGUISpecializedObject(const TObjectType& objType)
    : TGameObject(objType), TGUIGameObject(objType)
{
}

void TGUISpecializedObject::draw(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp, int x, int y,
                                 TZoom zoom) const
{
#line 510
    assert(zoom >= 0 && zoom < kNumZooms);
    drawSpriteImpl(frameNum, gc, pDestBmp, x, y, zoom);
    if (isCustomized())
        g_warning("NO CHECK MARK.");
}

void TGUISpecializedObject::drawCell(unsigned int frameNum, unsigned int cellX, unsigned int cellY,
                                     GdkGC* gc, GdkImage* pDestBmp, int x, int y, TZoom zoom) const
{
#line 544
    assert(zoom >= 0 && zoom < kNumZooms);
    drawSpriteCellImpl(frameNum, cellX, cellY, gc, pDestBmp, x, y, zoom);
    if (cellX == 0 && cellY == 0) {
        if (isCustomized())
            g_warning("NO CHECK MARK.");
    }
}

bool TGUISpecializedObject::hitTest(unsigned int frameNum, int x, int y, TZoom zoom) const
{
    if (hitTestSpriteImpl(frameNum, x, y, zoom))
        return true;
    if (!isCustomized())
        return false;
    g_warning("GUIGameObject:601 (hitTest()) not fully implemented.");
    return false;
}

void TGUISpecializedObject::drawSpriteImpl(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp, int x,
                                           int y, TZoom zoom) const
{
    TGUIGameObject::draw(frameNum, gc, pDestBmp, x, y, zoom);
}

void TGUISpecializedObject::drawSpriteCellImpl(unsigned int frameNum, unsigned int cellX, unsigned int cellY,
                                               GdkGC* gc, GdkImage* pDestBmp, int x, int y, TZoom zoom) const
{
    TGUIGameObject::drawCell(frameNum, cellX, cellY, gc, pDestBmp, x, y, zoom);
}

bool TGUISpecializedObject::hitTestSpriteImpl(unsigned int frameNum, int x, int y, TZoom zoom) const
{
    return TGUIGameObject::hitTest(frameNum, x, y, zoom);
}

// A hero on the map: the hero's sprite and its owner's flag.
class TGUIHeroBase : public TGUISpecializedObject {
public:
    TGUIHeroBase(const TObjectType& objType);

    virtual void drawShadow(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp, int x, int y,
                            TZoom zoom) const;
    virtual void drawCellShadow(unsigned int frameNum, unsigned int cellX, unsigned int cellY,
                                GdkGC* gc, GdkImage* pDestBmp, int x, int y, TZoom zoom) const;
    virtual TColor miniMapColor(TTerrainType terrainType) const;
    virtual bool isAnimated() const;
    virtual bool isOwnable() const { return true; }

protected:
    virtual void drawSpriteImpl(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp, int x, int y,
                                TZoom zoom) const;
    virtual void drawSpriteCellImpl(unsigned int frameNum, unsigned int cellX, unsigned int cellY,
                                    GdkGC* gc, GdkImage* pDestBmp, int x, int y, TZoom zoom) const;
    virtual bool hitTestSpriteImpl(unsigned int frameNum, int x, int y, TZoom zoom) const;

private:
    virtual THero* _getPHero() = 0;
    virtual const THero* _getPHero() const = 0;
};

TGUIHeroBase::TGUIHeroBase(const TObjectType& objType)
    : TGameObject(objType), TGUISpecializedObject(objType)
{
}

void TGUIHeroBase::drawShadow(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp, int x, int y,
                              TZoom zoom) const
{
    TGUIGameObject::drawShadow(frameNum, gc, pDestBmp, x, y, zoom);
    THeroFlagSpritePtr pFlagSprite(_getPHero()->getOwner());
#line 662
    assert(pFlagSprite.get() != NULL);
    CRect clipRect;
    getClipRect(gc, clipRect, pDestBmp);
    drawSpriteShadow(pFlagSprite.get(), getAnimOffset() + frameNum, pDestBmp, x, y, clipRect, zoom);
}

void TGUIHeroBase::drawCellShadow(unsigned int frameNum, unsigned int cellX, unsigned int cellY, GdkGC* gc,
                                  GdkImage* pDestBmp, int x, int y, TZoom zoom) const
{
    TGUIGameObject::drawCellShadow(frameNum, cellX, cellY, gc, pDestBmp, x, y, zoom);
    THeroFlagSpritePtr pFlagSprite(_getPHero()->getOwner());
#line 677
    assert(pFlagSprite.get() != NULL);
    CRect clipRect;
    getClipRect(gc, clipRect, pDestBmp);
    drawSpriteCellShadow(pFlagSprite.get(), getAnimOffset() + frameNum, cellX, cellY, pDestBmp, x, y,
                         clipRect, zoom);
}

void TGUIHeroBase::drawSpriteImpl(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp, int x, int y,
                                  TZoom zoom) const
{
#line 689
    assert(pDestBmp != NULL);
    assert(gc != NULL);
    TGUISpecializedObject::drawSpriteImpl(frameNum, gc, pDestBmp, x, y, zoom);
    THeroFlagSpritePtr pFlagSprite(_getPHero()->getOwner());
#line 695
    assert(pFlagSprite.get() != NULL);
    CRect clipRect;
    getClipRect(gc, clipRect, pDestBmp);
    drawSprite(pFlagSprite.get(), getAnimOffset() + frameNum, pDestBmp, x, y, clipRect, zoom);
}

void TGUIHeroBase::drawSpriteCellImpl(unsigned int frameNum, unsigned int cellX, unsigned int cellY,
                                      GdkGC* gc, GdkImage* pDestBmp, int x, int y, TZoom zoom) const
{
    TGUISpecializedObject::drawSpriteCellImpl(frameNum, cellX, cellY, gc, pDestBmp, x, y, zoom);
    THeroFlagSpritePtr pFlagSprite(_getPHero()->getOwner());
#line 710
    assert(pFlagSprite.get() != NULL);
    CRect clipRect;
    getClipRect(gc, clipRect, pDestBmp);
    drawSpriteCell(pFlagSprite.get(), getAnimOffset() + frameNum, cellX, cellY, pDestBmp, x, y, clipRect,
                   zoom);
}

bool TGUIHeroBase::hitTestSpriteImpl(unsigned int frameNum, int x, int y, TZoom zoom) const
{
    if (TGUISpecializedObject::hitTestSpriteImpl(frameNum, x, y, zoom))
        return true;
    THeroFlagSpritePtr pFlagSprite(_getPHero()->getOwner());
#line 726
    assert(pFlagSprite.get() != NULL);
    return hitTestSprite(pFlagSprite.get(), getAnimOffset() + frameNum, x, y, zoom);
}

TColor TGUIHeroBase::miniMapColor(TTerrainType terrainType) const
{
#line 734
    assert(terrainType >= 0 && terrainType < kNumTerrainTypes);
    TPlayer owner = _getPHero()->getOwner();
#line 737
    assert(owner >= 0 && owner < kNumPlayers);
    return akPlayerColor[owner + 1];
}

bool TGUIHeroBase::isAnimated() const
{
    if (TGUIGameObject::isAnimated())
        return true;
    THeroFlagSpritePtr pFlagSprite(_getPHero()->getOwner());
#line 748
    assert(pFlagSprite.get() != NULL);
    return pFlagSprite->GetNumFrames(0) > 1;
}

class TGUINonRandomHero : public TNonRandomHero, public TGUIHeroBase {
public:
    TGUINonRandomHero(const TObjectType& objType, TPlayer owner, unsigned int protoNum);
    TGUINonRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version);
    virtual ~TGUINonRandomHero() {}

    virtual bool edit(TEditContext* pEditContext, unsigned int objID);
    virtual TGameObject* clone(void* (*pfnAllocator)(unsigned int)) const;

private:
    virtual THero* _getPHero() { return this; }
    virtual const THero* _getPHero() const { return this; }
};

TGUINonRandomHero::TGUINonRandomHero(const TObjectType& objType, TPlayer owner, unsigned int protoNum)
    : TGameObject(objType), TNonRandomHero(objType, owner, protoNum), TGUIHeroBase(objType)
{
}

TGUINonRandomHero::TGUINonRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TNonRandomHero(objType, pIStream, version), TGUIHeroBase(objType)
{
}

bool TGUINonRandomHero::edit(TEditContext* pEditContext, unsigned int objID)
{
#line 791
    assert(pEditContext != NULL);
    return pEditContext->onEditProperties(this, objID);
}

TGameObject* TGUINonRandomHero::clone(void* (*pfnAllocator)(unsigned int)) const
{
#line 798
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUINonRandomHero))) TGUINonRandomHero(*this);
}

class TGUIRandomHero : public TRandomHero, public TGUIHeroBase {
public:
    TGUIRandomHero(const TObjectType& objType, TPlayer owner);
    TGUIRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version);
    virtual ~TGUIRandomHero() {}

    virtual bool edit(TEditContext* pEditContext, unsigned int objID);
    virtual TGameObject* clone(void* (*pfnAllocator)(unsigned int)) const;

private:
    virtual THero* _getPHero() { return this; }
    virtual const THero* _getPHero() const { return this; }
};

TGUIRandomHero::TGUIRandomHero(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), TRandomHero(objType, owner), TGUIHeroBase(objType)
{
}

TGUIRandomHero::TGUIRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TRandomHero(objType, pIStream, version), TGUIHeroBase(objType)
{
}

bool TGUIRandomHero::edit(TEditContext* pEditContext, unsigned int objID)
{
#line 840
    assert(pEditContext != NULL);
    return pEditContext->onEditProperties(this, objID);
}

TGameObject* TGUIRandomHero::clone(void* (*pfnAllocator)(unsigned int)) const
{
#line 847
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIRandomHero))) TGUIRandomHero(*this);
}

// A prison: a hero's sprite without the flag.
class TGUIPrison : public TPrison, public TGUISpecializedObject {
public:
    TGUIPrison(const TObjectType& objType, THeroClass heroClass, unsigned int protoNum);
    TGUIPrison(const TObjectType& objType, TRawIStream* pIStream, int version);
    virtual ~TGUIPrison() {}

    virtual bool edit(TEditContext* pEditContext, unsigned int objID);
    virtual TGameObject* clone(void* (*pfnAllocator)(unsigned int)) const;
};

TGUIPrison::TGUIPrison(const TObjectType& objType, THeroClass heroClass, unsigned int protoNum)
    : TGameObject(objType), TPrison(objType, heroClass, protoNum), TGUISpecializedObject(objType)
{
}

TGUIPrison::TGUIPrison(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TPrison(objType, pIStream, version), TGUISpecializedObject(objType)
{
}

bool TGUIPrison::edit(TEditContext* pEditContext, unsigned int objID)
{
#line 885
    assert(pEditContext != NULL);
    return pEditContext->onEditProperties(this, objID);
}

TGameObject* TGUIPrison::clone(void* (*pfnAllocator)(unsigned int)) const
{
#line 892
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIPrison))) TGUIPrison(*this);
}

// A town: its sprite shows the town, castle or capitol.
class TGUITown : public TTown, public TGUISpecializedObject {
public:
    TGUITown(const TObjectType& objType, TPlayer owner);
    TGUITown(const TObjectType& objType, TRawIStream* pIStream, int version);
    virtual ~TGUITown() {}

    virtual bool edit(TEditContext* pEditContext, unsigned int objID);
    virtual TGameObject* clone(void* (*pfnAllocator)(unsigned int)) const;
    virtual void drawShadow(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp, int x, int y,
                            TZoom zoom) const;
    virtual void drawCellShadow(unsigned int frameNum, unsigned int cellX, unsigned int cellY,
                                GdkGC* gc, GdkImage* pDestBmp, int x, int y, TZoom zoom) const;
    virtual TColor miniMapColor(TTerrainType terrainType) const;
    virtual bool isAnimated() const;
    virtual bool isOwnable() const { return true; }

protected:
    virtual void drawSpriteImpl(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp, int x, int y,
                                TZoom zoom) const;
    virtual void drawSpriteCellImpl(unsigned int frameNum, unsigned int cellX, unsigned int cellY,
                                    GdkGC* gc, GdkImage* pDestBmp, int x, int y, TZoom zoom) const;
    virtual bool hitTestSpriteImpl(unsigned int frameNum, int x, int y, TZoom zoom) const;

private:
    const CSprite* _getPSprite() const;
    bool _hasCapitol() const { return getBuildingStates()[eBuildingCapitol].getBBuilt(); }
    bool _hasCastle() const { return getBuildingStates()[eBuildingFort].getBBuilt(); }
};

TGUITown::TGUITown(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), TTown(objType, owner), TGUISpecializedObject(objType)
{
}

TGUITown::TGUITown(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TTown(objType, pIStream, version), TGUISpecializedObject(objType)
{
}

bool TGUITown::edit(TEditContext* pEditContext, unsigned int objID)
{
#line 946
    assert(pEditContext != NULL);
    return pEditContext->onEditProperties(this, objID);
}

TGameObject* TGUITown::clone(void* (*pfnAllocator)(unsigned int)) const
{
#line 953
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUITown))) TGUITown(*this);
}

void TGUITown::drawShadow(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp, int x, int y,
                          TZoom zoom) const
{
#line 960
    assert(gc != NULL);
    assert(pDestBmp != NULL);
    const CSprite* pSprite = _getPSprite();
#line 964
    assert(pSprite != NULL);
    CRect clipRect;
    getClipRect(gc, clipRect, pDestBmp);
    drawSpriteShadow(pSprite, getAnimOffset() + frameNum, pDestBmp, x, y, clipRect, zoom);
}

void TGUITown::drawCellShadow(unsigned int frameNum, unsigned int cellX, unsigned int cellY, GdkGC* gc,
                              GdkImage* pDestBmp, int x, int y, TZoom zoom) const
{
#line 976
    assert(pDestBmp != NULL);
    assert(gc != NULL);
    const CSprite* pSprite = _getPSprite();
#line 980
    assert(pSprite != NULL);
    CRect clipRect;
    getClipRect(gc, clipRect, pDestBmp);
    drawSpriteCellShadow(pSprite, getAnimOffset() + frameNum, cellX, cellY, pDestBmp, x, y, clipRect, zoom);
}

TColor TGUITown::miniMapColor(TTerrainType terrainType) const
{
#line 993
    assert(terrainType >= 0 && terrainType < kNumTerrainTypes);
    TPlayer owner = getOwner();
#line 996
    assert(owner >= ePlayerNone && owner < kNumPlayers);
    return akPlayerColor[owner + 1];
}

bool TGUITown::isAnimated() const
{
    const CSprite* pSprite = _getPSprite();
#line 1004
    assert(pSprite != NULL);
    return pSprite->GetNumFrames(0) > 1;
}

void TGUITown::drawSpriteImpl(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp, int x, int y,
                              TZoom zoom) const
{
#line 1012
    assert(pDestBmp != NULL);
    assert(gc != NULL);
    const CSprite* pSprite = _getPSprite();
#line 1016
    assert(pSprite != NULL);
    CRect clipRect;
    getClipRect(gc, clipRect, pDestBmp);
    drawSprite(pSprite, getAnimOffset() + frameNum, getOwner(), pDestBmp, x, y, clipRect, zoom);
}

void TGUITown::drawSpriteCellImpl(unsigned int frameNum, unsigned int cellX, unsigned int cellY, GdkGC* gc,
                                  GdkImage* pDestBmp, int x, int y, TZoom zoom) const
{
#line 1028
    assert(pDestBmp != NULL);
    assert(gc != NULL);
    const CSprite* pSprite = _getPSprite();
#line 1032
    assert(pSprite != NULL);
    CRect clipRect;
    getClipRect(gc, clipRect, pDestBmp);
    drawSpriteCell(pSprite, getAnimOffset() + frameNum, cellX, cellY, getOwner(), pDestBmp, x, y, clipRect,
                   zoom);
}

bool TGUITown::hitTestSpriteImpl(unsigned int frameNum, int x, int y, TZoom zoom) const
{
    const CSprite* pSprite = _getPSprite();
#line 1045
    assert(pSprite != NULL);
    return hitTestSprite(pSprite, frameNum, x, y, zoom);
}

const CSprite* TGUITown::_getPSprite() const
{
    struct TSprites {
        TResourcePtr<CSprite> m_pTown;
        TResourcePtr<CSprite> m_pCastle;
        TResourcePtr<CSprite> m_pCapitol;
    };
    static const char* const akSpriteNames[kNumTownTypes + 1][3] = {
        { "avccast0.def", "avccasx0.def", "avccasz0.def" },
        { "avcramp0.def", "avcramx0.def", "avcramz0.def" },
        { "avctowr0.def", "avctowx0.def", "avctowz0.def" },
        { "avcinft0.def", "avcinfx0.def", "avcinfz0.def" },
        { "avcnecr0.def", "avcnecx0.def", "avcnecz0.def" },
        { "avcdung0.def", "avcdunx0.def", "avcdunz0.def" },
        { "avcstro0.def", "avcstrx0.def", "avcstrz0.def" },
        { "avcftrt0.def", "avcftrx0.def", "avcforz0.def" },
        { "avcrand0.def", "avcranx0.def", "avcranz0.def" }
    };
    static TSprites aSprites[kNumTownTypes + 1];

    TTownType townType = getTownType();
    if (_hasCapitol()) {
        if (!aSprites[townType].m_pCapitol.get()) {
            CSprite* pSprite = ResourceManager::GetSprite(akSpriteNames[townType][2]);
            if (!pSprite)
#line 1085
                throw TRuntimeError(__FILE__, __LINE__, string("Unable to load capitol sprite:  \"")
                                    + akSpriteNames[townType][2] + "\".");
            aSprites[townType].m_pCapitol = TResourcePtr<CSprite>(pSprite);
        }
        return aSprites[townType].m_pCapitol.get();
    }
    if (_hasCastle()) {
        if (!aSprites[townType].m_pCastle.get()) {
            CSprite* pSprite = ResourceManager::GetSprite(akSpriteNames[townType][1]);
            if (!pSprite)
#line 1097
                throw TRuntimeError(__FILE__, __LINE__, string("Unable to load castle sprite:  \"")
                                    + akSpriteNames[townType][1] + "\".");
            aSprites[townType].m_pCastle = TResourcePtr<CSprite>(pSprite);
        }
        return aSprites[townType].m_pCastle.get();
    }
    if (!aSprites[townType].m_pTown.get()) {
        CSprite* pSprite = ResourceManager::GetSprite(akSpriteNames[townType][0]);
        if (!pSprite)
#line 1107
            throw TRuntimeError(__FILE__, __LINE__, string("Unable to load town sprite:  \"")
                                + akSpriteNames[townType][0] + "\".");
        aSprites[townType].m_pTown = TResourcePtr<CSprite>(pSprite);
    }
    return aSprites[townType].m_pTown.get();
}

// An object of game class T with nothing to add but its edit dialog.
template <class T>
class TGUIStandardSpecializedObject : public T, public TGUISpecializedObject {
public:
    TGUIStandardSpecializedObject(const TObjectType& objType);
    TGUIStandardSpecializedObject(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual bool edit(TEditContext* pEditContext, unsigned int objID);
    virtual TGameObject* clone(void* (*pfnAllocator)(unsigned int)) const;
};

template <class T>
TGUIStandardSpecializedObject<T>::TGUIStandardSpecializedObject(const TObjectType& objType)
    : TGameObject(objType), T(objType), TGUISpecializedObject(objType)
{
}

template <class T>
TGUIStandardSpecializedObject<T>::TGUIStandardSpecializedObject(const TObjectType& objType,
                                                                TRawIStream* pIStream, int version)
    : TGameObject(objType), T(objType, pIStream, version), TGUISpecializedObject(objType)
{
}

template <class T>
bool TGUIStandardSpecializedObject<T>::edit(TEditContext* pEditContext, unsigned int objID)
{
#line 1151
    assert(pEditContext != NULL);
    return pEditContext->onEditProperties(this, objID);
}

template <class T>
TGameObject* TGUIStandardSpecializedObject<T>::clone(void* (*pfnAllocator)(unsigned int)) const
{
#line 1159
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<T>))) TGUIStandardSpecializedObject<T>(*this);
}

// An object of game class T that a player can own: drawn with its owner's
// colour.
template <class T>
class TGUIOwnableSpecializedObject : public T, public TGUISpecializedObject {
public:
    TGUIOwnableSpecializedObject(const TObjectType& objType, TPlayer owner);
    TGUIOwnableSpecializedObject(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual bool edit(TEditContext* pEditContext, unsigned int objID);
    virtual TGameObject* clone(void* (*pfnAllocator)(unsigned int)) const;
    virtual TColor miniMapColor(TTerrainType terrainType) const;
    virtual bool isOwnable() const;

protected:
    virtual void drawSpriteImpl(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp, int x, int y,
                                TZoom zoom) const;
    virtual void drawSpriteCellImpl(unsigned int frameNum, unsigned int cellX, unsigned int cellY,
                                    GdkGC* gc, GdkImage* pDestBmp, int x, int y, TZoom zoom) const;
};

template <class T>
TGUIOwnableSpecializedObject<T>::TGUIOwnableSpecializedObject(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), T(objType, owner), TGUISpecializedObject(objType)
{
}

template <class T>
TGUIOwnableSpecializedObject<T>::TGUIOwnableSpecializedObject(const TObjectType& objType,
                                                              TRawIStream* pIStream, int version)
    : TGameObject(objType), T(objType, pIStream, version), TGUISpecializedObject(objType)
{
}

template <class T>
bool TGUIOwnableSpecializedObject<T>::edit(TEditContext* pEditContext, unsigned int objID)
{
#line 1222
    assert(pEditContext != NULL);
    return pEditContext->onEditProperties(this, objID);
}

template <class T>
TGameObject* TGUIOwnableSpecializedObject<T>::clone(void* (*pfnAllocator)(unsigned int)) const
{
#line 1230
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIOwnableSpecializedObject<T>))) TGUIOwnableSpecializedObject<T>(*this);
}

template <class T>
TColor TGUIOwnableSpecializedObject<T>::miniMapColor(TTerrainType terrainType) const
{
#line 1239
    assert(terrainType >= 0 && terrainType < kNumTerrainTypes);
    TPlayer owner = getOwner();
#line 1242
    assert(owner >= ePlayerNone && owner < kNumPlayers);
    return akPlayerColor[owner + 1];
}

template <class T>
bool TGUIOwnableSpecializedObject<T>::isOwnable() const
{
    return true;
}

template <class T>
void TGUIOwnableSpecializedObject<T>::drawSpriteImpl(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp,
                                                     int x, int y, TZoom zoom) const
{
#line 1257
    assert(pDestBmp != NULL);
    assert(gc != NULL);
    TObjectSpritePtr pSprite(getObjectType());
#line 1261
    assert(pSprite.get() != NULL);
    CRect clipRect;
    getClipRect(gc, clipRect, pDestBmp);
    drawSprite(pSprite.get(), getAnimOffset() + frameNum, getOwner(), pDestBmp, x, y, clipRect, zoom);
}

template <class T>
void TGUIOwnableSpecializedObject<T>::drawSpriteCellImpl(unsigned int frameNum, unsigned int cellX,
                                                         unsigned int cellY, GdkGC* gc, GdkImage* pDestBmp,
                                                         int x, int y, TZoom zoom) const
{
#line 1283
    assert(pDestBmp != NULL);
    assert(gc != NULL);
    TObjectSpritePtr pSprite(getObjectType());
#line 1286
    assert(pSprite.get() != NULL);
    CRect clipRect;
    getClipRect(gc, clipRect, pDestBmp);
    drawSpriteCell(pSprite.get(), getAnimOffset() + frameNum, cellX, cellY, getOwner(), pDestBmp, x, y,
                   clipRect, zoom);
}

// A creature generator: ownable only when its kind can be flagged.
class TGUIGenerator : public TGUIOwnableSpecializedObject<TGenerator> {
public:
    TGUIGenerator(const TObjectType& objType, TPlayer owner);
    TGUIGenerator(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual bool edit(TEditContext* pEditContext, unsigned int objID);
    virtual TGameObject* clone(void* (*pfnAllocator)(unsigned int)) const;
    virtual void draw(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp, int x, int y,
                      TZoom zoom) const;
    virtual void drawCell(unsigned int frameNum, unsigned int cellX, unsigned int cellY, GdkGC* gc,
                          GdkImage* pDestBmp, int x, int y, TZoom zoom) const;
    virtual TColor miniMapColor(TTerrainType terrainType) const;
    virtual bool isOwnable() const { return _isOwnable(); }

private:
    bool _isOwnable() const { return getGeneratorTypeTraits().m_bFlaggable; }
};

TGUIGenerator::TGUIGenerator(const TObjectType& objType, TPlayer owner)
    : TGameObject(objType), TGUIOwnableSpecializedObject<TGenerator>(objType, owner)
{
}

TGUIGenerator::TGUIGenerator(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TGUIOwnableSpecializedObject<TGenerator>(objType, pIStream, version)
{
}

bool TGUIGenerator::edit(TEditContext* pEditContext, unsigned int objID)
{
    if (_isOwnable())
        return TGUIOwnableSpecializedObject<TGenerator>::edit(pEditContext, objID);
    return TGUIGameObject::edit(pEditContext, objID);
}

TGameObject* TGUIGenerator::clone(void* (*pfnAllocator)(unsigned int)) const
{
#line 1350
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIGenerator))) TGUIGenerator(*this);
}

void TGUIGenerator::draw(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp, int x, int y,
                         TZoom zoom) const
{
    if (_isOwnable())
        TGUIOwnableSpecializedObject<TGenerator>::draw(frameNum, gc, pDestBmp, x, y, zoom);
    else
        TGUISpecializedObject::draw(frameNum, gc, pDestBmp, x, y, zoom);
}

void TGUIGenerator::drawCell(unsigned int frameNum, unsigned int cellX, unsigned int cellY, GdkGC* gc,
                             GdkImage* pDestBmp, int x, int y, TZoom zoom) const
{
    if (_isOwnable())
        TGUIOwnableSpecializedObject<TGenerator>::drawCell(frameNum, cellX, cellY, gc, pDestBmp, x, y, zoom);
    else
        TGUISpecializedObject::drawCell(frameNum, cellX, cellY, gc, pDestBmp, x, y, zoom);
}

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

    virtual TGameObject* clone(void* (*pfnAllocator)(unsigned int)) const;
    virtual TColor miniMapColor(TTerrainType terrainType) const;
    virtual bool isOwnable() const { return true; }
};

TGUIAbandonedMine::TGUIAbandonedMine(const TObjectType& objType)
    : TGameObject(objType), TGUIStandardSpecializedObject<TAbandonedMine>(objType)
{
}

TGUIAbandonedMine::TGUIAbandonedMine(const TObjectType& objType, TRawIStream* pIStream, int version)
    : TGameObject(objType), TGUIStandardSpecializedObject<TAbandonedMine>(objType, pIStream, version)
{
}

TGameObject* TGUIAbandonedMine::clone(void* (*pfnAllocator)(unsigned int)) const
{
#line 1414
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIAbandonedMine))) TGUIAbandonedMine(*this);
}

TColor TGUIAbandonedMine::miniMapColor(TTerrainType terrainType) const
{
#line 1421
    assert(terrainType >= 0 && terrainType < kNumTerrainTypes);
    return akPlayerColor[0];
}

// The object types the mini map draws in the terrain's darker colour.
class TMiniMapTypeFlags : public bitset<MAX_EVENT_TYPE> {
public:
    TMiniMapTypeFlags();
};

TMiniMapTypeFlags::TMiniMapTypeFlags()
{
    static const TAdventureObjectType akTypes[] = {
        TERRAIN_CACTUS, TERRAIN_CRATER, TERRAIN_DEAD_VEGETATION, TERRAIN_FROZEN_LAKE, TERRAIN_LAKE,
        TERRAIN_LAVA_LAKE, TERRAIN_HILL, TERRAIN_MANDRAKE, TERRAIN_MOUND, TERRAIN_MOUNTAIN,
        TERRAIN_OAK_TREE, TERRAIN_PINE_TREE, TERRAIN_SAND_DUNE, TERRAIN_SAND_PIT, TERRAIN_TAR_PIT,
        TERRAIN_STALAGMITE, TERRAIN_STUMP, TERRAIN_TREE, TERRAIN_VOLCANO, TERRAIN_WILLOW_TREE,
        TERRAIN_YUCCA_TREE
    };
    for (unsigned int i = 0; i < ARRAY_SIZE(akTypes); i++)
        (*this)[akTypes[i]] = true;
}
}

TGUIGameObject::TGUIGameObject(const TObjectType& objType)
    : TGameObject(objType)
{
    _m_animOffset = rand();
}

bool TGUIGameObject::edit(TEditContext* pEditContext, unsigned int objID)
{
#line 1483
    assert(pEditContext != NULL);
    bool result = pEditContext->onEditProperties(this, objID);
#line 1485
    assert(!result);
    return result;
}

void TGUIGameObject::draw(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp, int x, int y,
                          TZoom zoom) const
{
#line 1492
    assert(gc != NULL);
    assert(pDestBmp != NULL);
    TObjectSpritePtr pSprite(getObjectType());
#line 1496
    assert(pSprite.get() != NULL);
    CRect clipRect;
    getClipRect(gc, clipRect, pDestBmp);
    drawSprite(pSprite.get(), getAnimOffset() + frameNum, pDestBmp, x, y, clipRect, zoom);
}

void TGUIGameObject::drawShadow(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp, int x, int y,
                                TZoom zoom) const
{
#line 1509
    assert(gc != NULL);
    assert(pDestBmp != NULL);
    TObjectSpritePtr pSprite(getObjectType());
#line 1513
    assert(pSprite.get() != NULL);
    CRect clipRect;
    getClipRect(gc, clipRect, pDestBmp);
    drawSpriteShadow(pSprite.get(), getAnimOffset() + frameNum, pDestBmp, x, y, clipRect, zoom);
}

void TGUIGameObject::drawCell(unsigned int frameNum, unsigned int cellX, unsigned int cellY, GdkGC* gc,
                              GdkImage* pDestBmp, int x, int y, TZoom zoom) const
{
#line 1525
    assert(gc != NULL);
    assert(pDestBmp != NULL);
    TObjectSpritePtr pSprite(getObjectType());
#line 1529
    assert(pSprite.get() != NULL);
    CRect clipRect;
    getClipRect(gc, clipRect, pDestBmp);
    drawSpriteCell(pSprite.get(), getAnimOffset() + frameNum, cellX, cellY, pDestBmp, x, y, clipRect, zoom);
}

void TGUIGameObject::drawCellShadow(unsigned int frameNum, unsigned int cellX, unsigned int cellY, GdkGC* gc,
                                    GdkImage* pDestBmp, int x, int y, TZoom zoom) const
{
#line 1541
    assert(gc != NULL);
    assert(pDestBmp != NULL);
    TObjectSpritePtr pSprite(getObjectType());
#line 1545
    assert(pSprite.get() != NULL);
    CRect clipRect;
    getClipRect(gc, clipRect, pDestBmp);
    drawSpriteCellShadow(pSprite.get(), getAnimOffset() + frameNum, cellX, cellY, pDestBmp, x, y, clipRect,
                         zoom);
}

bool TGUIGameObject::hitTest(unsigned int frameNum, int x, int y, TZoom zoom) const
{
    TObjectSpritePtr pSprite(getObjectType());
#line 1558
    assert(pSprite.get() != NULL);
    return hitTestSprite(pSprite.get(), getAnimOffset() + frameNum, x, y, zoom);
}

TColor TGUIGameObject::miniMapColor(TTerrainType terrainType) const
{
#line 1566
    assert(terrainType >= 0 && terrainType < kNumTerrainTypes);
    static const TMiniMapTypeFlags typeFlags;
    if (typeFlags[getType()])
        return akTerrainColors[terrainType].m_obstacleColor;
    return akTerrainColors[terrainType].m_color;
}

bool TGUIGameObject::isAnimated() const
{
    TObjectSpritePtr pSprite(getObjectType());
#line 1580
    assert(pSprite.get() != NULL);
    return pSprite->GetNumFrames(0) > 1;
}

bool TGUIGameObject::isOwnable() const
{
    return false;
}

TGenericObject* TGUIGameObjectFactory::createGenericObject(const TObjectType& objType,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1597
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIGenericObject))) TGUIGenericObject(objType);
}

TGenericObject* TGUIGameObjectFactory::createGenericObject(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1604
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIGenericObject))) TGUIGenericObject(objType, pIStream, version);
}

TNonRandomHero* TGUIGameObjectFactory::createNonRandomHero(const TObjectType& objType, TPlayer owner, unsigned int protoNum,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1611
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUINonRandomHero))) TGUINonRandomHero(objType, owner, protoNum);
}

TNonRandomHero* TGUIGameObjectFactory::createNonRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1618
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUINonRandomHero))) TGUINonRandomHero(objType, pIStream, version);
}

TRandomHero* TGUIGameObjectFactory::createRandomHero(const TObjectType& objType, TPlayer owner,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1625
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIRandomHero))) TGUIRandomHero(objType, owner);
}

TRandomHero* TGUIGameObjectFactory::createRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1632
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIRandomHero))) TGUIRandomHero(objType, pIStream, version);
}

TPrison* TGUIGameObjectFactory::createPrison(const TObjectType& objType, THeroClass heroClass, unsigned int protoNum,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1639
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIPrison))) TGUIPrison(objType, heroClass, protoNum);
}

TPrison* TGUIGameObjectFactory::createPrison(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1646
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIPrison))) TGUIPrison(objType, pIStream, version);
}

TTown* TGUIGameObjectFactory::createTown(const TObjectType& objType, TPlayer owner,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1653
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUITown))) TGUITown(objType, owner);
}

TTown* TGUIGameObjectFactory::createTown(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1660
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUITown))) TGUITown(objType, pIStream, version);
}

TEvent* TGUIGameObjectFactory::createEvent(const TObjectType& objType,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1667
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TEvent>))) TGUIStandardSpecializedObject<TEvent>(objType);
}

TEvent* TGUIGameObjectFactory::createEvent(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1674
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TEvent>))) TGUIStandardSpecializedObject<TEvent>(objType, pIStream, version);
}

TMonster* TGUIGameObjectFactory::createMonster(const TObjectType& objType,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1681
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TMonster>))) TGUIStandardSpecializedObject<TMonster>(objType);
}

TMonster* TGUIGameObjectFactory::createMonster(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1688
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TMonster>))) TGUIStandardSpecializedObject<TMonster>(objType, pIStream, version);
}

TSign* TGUIGameObjectFactory::createSign(const TObjectType& objType,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1695
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TSign>))) TGUIStandardSpecializedObject<TSign>(objType);
}

TSign* TGUIGameObjectFactory::createSign(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1702
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TSign>))) TGUIStandardSpecializedObject<TSign>(objType, pIStream, version);
}

TFlaggableObject* TGUIGameObjectFactory::createFlaggable(const TObjectType& objType, TPlayer owner,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1709
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIOwnableSpecializedObject<TFlaggableObject>))) TGUIOwnableSpecializedObject<TFlaggableObject>(objType, owner);
}

TFlaggableObject* TGUIGameObjectFactory::createFlaggable(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1716
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIOwnableSpecializedObject<TFlaggableObject>))) TGUIOwnableSpecializedObject<TFlaggableObject>(objType, pIStream, version);
}

TMine* TGUIGameObjectFactory::createMine(const TObjectType& objType, TPlayer owner,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1723
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIOwnableSpecializedObject<TMine>))) TGUIOwnableSpecializedObject<TMine>(objType, owner);
}

TMine* TGUIGameObjectFactory::createMine(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1730
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIOwnableSpecializedObject<TMine>))) TGUIOwnableSpecializedObject<TMine>(objType, pIStream, version);
}

TAbandonedMine* TGUIGameObjectFactory::createAbandonedMine(const TObjectType& objType,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1737
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIAbandonedMine))) TGUIAbandonedMine(objType);
}

TAbandonedMine* TGUIGameObjectFactory::createAbandonedMine(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1744
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIAbandonedMine))) TGUIAbandonedMine(objType, pIStream, version);
}

TGenerator* TGUIGameObjectFactory::createGenerator(const TObjectType& objType, TPlayer owner,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1751
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIGenerator))) TGUIGenerator(objType, owner);
}

TGenerator* TGUIGameObjectFactory::createGenerator(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1758
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIGenerator))) TGUIGenerator(objType, pIStream, version);
}

TGarrison* TGUIGameObjectFactory::createGarrison(const TObjectType& objType, TPlayer owner,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1765
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIOwnableSpecializedObject<TGarrison>))) TGUIOwnableSpecializedObject<TGarrison>(objType, owner);
}

TGarrison* TGUIGameObjectFactory::createGarrison(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1772
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIOwnableSpecializedObject<TGarrison>))) TGUIOwnableSpecializedObject<TGarrison>(objType, pIStream, version);
}

TGameArtifact* TGUIGameObjectFactory::createArtifact(const TObjectType& objType,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1779
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TGameArtifact>))) TGUIStandardSpecializedObject<TGameArtifact>(objType);
}

TGameArtifact* TGUIGameObjectFactory::createArtifact(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1786
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TGameArtifact>))) TGUIStandardSpecializedObject<TGameArtifact>(objType, pIStream, version);
}

TSpellScroll* TGUIGameObjectFactory::createSpellScroll(const TObjectType& objType,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1793
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TSpellScroll>))) TGUIStandardSpecializedObject<TSpellScroll>(objType);
}

TSpellScroll* TGUIGameObjectFactory::createSpellScroll(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1800
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TSpellScroll>))) TGUIStandardSpecializedObject<TSpellScroll>(objType, pIStream, version);
}

TGameResource* TGUIGameObjectFactory::createResource(const TObjectType& objType,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1807
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TGameResource>))) TGUIStandardSpecializedObject<TGameResource>(objType);
}

TGameResource* TGUIGameObjectFactory::createResource(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1814
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TGameResource>))) TGUIStandardSpecializedObject<TGameResource>(objType, pIStream, version);
}

TBlackBox* TGUIGameObjectFactory::createBlackBox(const TObjectType& objType,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1821
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TBlackBox>))) TGUIStandardSpecializedObject<TBlackBox>(objType);
}

TBlackBox* TGUIGameObjectFactory::createBlackBox(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1828
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TBlackBox>))) TGUIStandardSpecializedObject<TBlackBox>(objType, pIStream, version);
}

TScholar* TGUIGameObjectFactory::createScholar(const TObjectType& objType,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1835
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TScholar>))) TGUIStandardSpecializedObject<TScholar>(objType);
}

TScholar* TGUIGameObjectFactory::createScholar(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1842
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TScholar>))) TGUIStandardSpecializedObject<TScholar>(objType, pIStream, version);
}

TSeersHut* TGUIGameObjectFactory::createSeersHut(const TObjectType& objType,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1849
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TSeersHut>))) TGUIStandardSpecializedObject<TSeersHut>(objType);
}

TSeersHut* TGUIGameObjectFactory::createSeersHut(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1856
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TSeersHut>))) TGUIStandardSpecializedObject<TSeersHut>(objType, pIStream, version);
}

THolyGrail* TGUIGameObjectFactory::createHolyGrail(const TObjectType& objType,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1863
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<THolyGrail>))) TGUIStandardSpecializedObject<THolyGrail>(objType);
}

THolyGrail* TGUIGameObjectFactory::createHolyGrail(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1870
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<THolyGrail>))) TGUIStandardSpecializedObject<THolyGrail>(objType, pIStream, version);
}

TShrine* TGUIGameObjectFactory::createShrine(const TObjectType& objType,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1877
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TShrine>))) TGUIStandardSpecializedObject<TShrine>(objType);
}

TShrine* TGUIGameObjectFactory::createShrine(const TObjectType& objType, TRawIStream* pIStream, int version,
    void* (*pfnAllocator)(unsigned int)) const
{
#line 1884
    assert(pfnAllocator != NULL);
    return new (pfnAllocator(sizeof(TGUIStandardSpecializedObject<TShrine>))) TGUIStandardSpecializedObject<TShrine>(objType, pIStream, version);
}
