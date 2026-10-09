// GUIGameObject.h - the editor's view of a map object (GUIGameObject.cpp;
// Loki h3maped). Declared so far only as far as the map windows need it.
// TGUIGameObject has TGameObject as a virtual base: h3maped reaches the
// object type through the virtual base pointer at +4 after the vtable
// pointer (the map edit window's 0x46c71e), and the windows find it by
// dynamic_cast from the layer's TGameObject. The virtuals are in Loki's
// order (__vt_14TGUIGameObject); the map edit window calls draw through
// slot 1, hitTest through slot 5 and isAnimated through slot 7. The
// Windows drawing functions take the device context and the 16-bit back
// buffer where Loki's take a GdkGC and a GdkImage.
#ifndef HOMM3_EDITOR_GUIGAMEOBJECT_H
#define HOMM3_EDITOR_GUIGAMEOBJECT_H

#include "editor/stdafx.h"
#include "editor/GameObject.h"
#include "editor/Tile.h"

class T16bppDIBSection;

class TGUIGameObject : public virtual TGameObject {
public:
    class TEditContext;

    TGUIGameObject(const TObjectType& objType);

    virtual bool edit(TEditContext* pEditContext, unsigned int objID);
    virtual void draw(unsigned int frameNum, CDC* pDC, T16bppDIBSection* pDestBmp, int x, int y,
                      TZoom zoom) const;
    virtual void drawShadow(unsigned int frameNum, CDC* pDC, T16bppDIBSection* pDestBmp, int x, int y,
                            TZoom zoom) const;
    virtual void drawCell(unsigned int frameNum, unsigned int cellX, unsigned int cellY, CDC* pDC,
                          T16bppDIBSection* pDestBmp, int x, int y, TZoom zoom) const;
    virtual void drawCellShadow(unsigned int frameNum, unsigned int cellX, unsigned int cellY, CDC* pDC,
                                T16bppDIBSection* pDestBmp, int x, int y, TZoom zoom) const;
    virtual bool hitTest(unsigned int frameNum, int x, int y, TZoom zoom) const;
    virtual TColor miniMapColor(TTerrainType terrainType) const;
    virtual bool isAnimated() const;
    virtual bool isOwnable() const;
};

#endif  /* HOMM3_EDITOR_GUIGAMEOBJECT_H */
