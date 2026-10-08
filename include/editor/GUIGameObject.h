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
// signatures of their __PRETTY_FUNCTION__ texts. The animation byte's name
// and the parameter names other than terrainType are not proven.
#ifndef HOMM3_EDITOR_GUIGAMEOBJECT_H
#define HOMM3_EDITOR_GUIGAMEOBJECT_H

#include "editor/stdafx.h"
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

    virtual bool edit(TEditContext* pContext, unsigned int objID);
    virtual void draw(unsigned int frameNum, GdkGC* gc, GdkImage* image, int x, int y,
                      TZoom zoom) const;
    virtual void drawShadow(unsigned int frameNum, GdkGC* gc, GdkImage* image, int x, int y,
                            TZoom zoom) const;
    virtual void drawCell(unsigned int frameNum, unsigned int cellX, unsigned int cellY, GdkGC* gc,
                          GdkImage* image, int x, int y, TZoom zoom) const;
    virtual void drawCellShadow(unsigned int frameNum, unsigned int cellX, unsigned int cellY,
                                GdkGC* gc, GdkImage* image, int x, int y, TZoom zoom) const;
    virtual bool hitTest(unsigned int frameNum, int x, int y, TZoom zoom) const;
    virtual TColor miniMapColor(TTerrainType terrainType) const;
    virtual bool isAnimated() const;
    virtual bool isOwnable() const;

    unsigned int getAnimOffset() const;

private:
    ubyte _m_animOffset;
};

#endif  /* HOMM3_EDITOR_GUIGAMEOBJECT_H */
