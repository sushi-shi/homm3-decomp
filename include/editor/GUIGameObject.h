// GUIGameObject.h - the editor's view of a map object (Loki
// GUIGameObject.cpp). Declared so far only as far as the window classes
// need it: TEditContext is the property-editing interface TMapView
// implements, one pure overload per object class, in the vtable order of
// __vt_8TMapView.Q214TGUIGameObject12TEditContext.
//
// TGUIGameObject is a virtual TGameObject base (its constructor takes the
// in-charge flag; the mini map reaches it by dynamic_cast from the layer's
// TGameObject). Its layout is the virtual base pointer, a byte the
// constructor fills from rand() (getAnimOffset), then its own vtable
// pointer; the virtuals are in the order of __vt_14TGUIGameObject, with the
// signatures of their __PRETTY_FUNCTION__ texts and the parameter names of
// their asserts. The animation byte's name is not proven.
//
// TGUIGameObjectFactory is the map's object factory: each create function
// places the matching GUI object (GUIGameObject.cpp's anonymous classes).
#ifndef HOMM3_EDITOR_GUIGAMEOBJECT_H
#define HOMM3_EDITOR_GUIGAMEOBJECT_H

#include "editor/stdafx.h"
#include "editor/GameMap.h"
#include "editor/GameObject.h"
#include "editor/Tile.h"
#include "terrain.h"

class TNonRandomHero;
class TRandomHero;
class TPrison;
class TTown;
class TEvent;
class TMonster;
class TFlaggableObject;
class TAbandonedMine;
class TGarrison;
class TSign;
class TGameArtifact;
class TSpellScroll;
class TGameResource;
class TBlackBox;
class TScholar;
class TSeersHut;
class THolyGrail;
class TShrine;

class TGUIGameObject : public virtual TGameObject {
public:
    class TEditContext {
    public:
        virtual bool onEditProperties(TGameObject* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TNonRandomHero* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TRandomHero* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TPrison* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TTown* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TEvent* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TMonster* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TFlaggableObject* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TAbandonedMine* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TGarrison* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TSign* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TGameArtifact* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TSpellScroll* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TGameResource* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TBlackBox* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TScholar* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TSeersHut* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(THolyGrail* pObj, unsigned int objID) = 0;
        virtual bool onEditProperties(TShrine* pObj, unsigned int objID) = 0;
    };

    TGUIGameObject(const TObjectType& objType);

    virtual bool edit(TEditContext* pEditContext, unsigned int objID);
    virtual void draw(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp, int x, int y,
                      TZoom zoom) const;
    virtual void drawShadow(unsigned int frameNum, GdkGC* gc, GdkImage* pDestBmp, int x, int y,
                            TZoom zoom) const;
    virtual void drawCell(unsigned int frameNum, unsigned int cellX, unsigned int cellY, GdkGC* gc,
                          GdkImage* pDestBmp, int x, int y, TZoom zoom) const;
    virtual void drawCellShadow(unsigned int frameNum, unsigned int cellX, unsigned int cellY,
                                GdkGC* gc, GdkImage* pDestBmp, int x, int y, TZoom zoom) const;
    virtual bool hitTest(unsigned int frameNum, int x, int y, TZoom zoom) const;
    virtual TColor miniMapColor(TTerrainType terrainType) const;
    virtual bool isAnimated() const;
    virtual bool isOwnable() const;

    unsigned int getAnimOffset() const { return _m_animOffset; }

private:
    ubyte _m_animOffset;
};

class TGUIGameObjectFactory : public TGameMap::TObjectFactory {
public:
    virtual TGenericObject* createGenericObject(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TGenericObject* createGenericObject(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TNonRandomHero* createNonRandomHero(const TObjectType& objType, TPlayer owner, unsigned int protoNum,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TNonRandomHero* createNonRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TRandomHero* createRandomHero(const TObjectType& objType, TPlayer owner,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TRandomHero* createRandomHero(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TPrison* createPrison(const TObjectType& objType, THeroClass heroClass, unsigned int protoNum,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TPrison* createPrison(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TTown* createTown(const TObjectType& objType, TPlayer owner,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TTown* createTown(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TEvent* createEvent(const TObjectType& objType, void* (*pfnAllocator)(unsigned int)) const;
    virtual TEvent* createEvent(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TMonster* createMonster(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TMonster* createMonster(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TSign* createSign(const TObjectType& objType, void* (*pfnAllocator)(unsigned int)) const;
    virtual TSign* createSign(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TFlaggableObject* createFlaggable(const TObjectType& objType, TPlayer owner,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TFlaggableObject* createFlaggable(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TMine* createMine(const TObjectType& objType, TPlayer owner,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TMine* createMine(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TAbandonedMine* createAbandonedMine(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TAbandonedMine* createAbandonedMine(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TGenerator* createGenerator(const TObjectType& objType, TPlayer owner,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TGenerator* createGenerator(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TGarrison* createGarrison(const TObjectType& objType, TPlayer owner,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TGarrison* createGarrison(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TGameArtifact* createArtifact(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TGameArtifact* createArtifact(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TSpellScroll* createSpellScroll(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TSpellScroll* createSpellScroll(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TGameResource* createResource(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TGameResource* createResource(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TBlackBox* createBlackBox(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TBlackBox* createBlackBox(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TScholar* createScholar(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TScholar* createScholar(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TSeersHut* createSeersHut(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TSeersHut* createSeersHut(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual THolyGrail* createHolyGrail(const TObjectType& objType,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual THolyGrail* createHolyGrail(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
    virtual TShrine* createShrine(const TObjectType& objType, void* (*pfnAllocator)(unsigned int)) const;
    virtual TShrine* createShrine(const TObjectType& objType, TRawIStream* pIStream, int version,
        void* (*pfnAllocator)(unsigned int)) const;
};

#endif  /* HOMM3_EDITOR_GUIGAMEOBJECT_H */
