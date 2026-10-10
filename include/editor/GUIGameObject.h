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
//
// TEditContext is the property-editing interface TMapView implements: one
// pure overload per object class. Loki's order, with the hero placeholder
// after the plain object and Complete's generators, quest guard and witch
// hut at the end; VC6 lists the overloads in reverse (h3maped vtable
// 0x53ecec). Windows edits a copy of the map: edit takes the map, the
// object's layer and id (h3maped 0x480c60 pushes them apart) and hands the
// context the map and the object's reference.
#ifndef HOMM3_EDITOR_GUIGAMEOBJECT_H
#define HOMM3_EDITOR_GUIGAMEOBJECT_H

#include "editor/stdafx.h"
#include <memory>

#include "editor/GameMap.h"
#include "editor/GameObject.h"
#include "editor/MapObjectRef.h"
#include "editor/Tile.h"

class T16bppDIBSection;
class TGameMap;
class THeroPlaceholder;
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
class TRandomGenerator;
class TRandomlyAlignedGenerator;
class TRandomlyLeveledGenerator;
class TQuestGuard;
class TWitchHut;

class TGUIGameObject : public virtual TGameObject {
public:
    class TEditContext {
    public:
        virtual bool onEditProperties(TGameObject* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(THeroPlaceholder* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TNonRandomHero* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TRandomHero* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TPrison* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TTown* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TEvent* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TMonster* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TFlaggableObject* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TAbandonedMine* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TGarrison* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TSign* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TGameArtifact* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TSpellScroll* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TGameResource* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TBlackBox* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TScholar* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TSeersHut* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(THolyGrail* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TShrine* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TRandomGenerator* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TRandomlyAlignedGenerator* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TRandomlyLeveledGenerator* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TQuestGuard* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
        virtual bool onEditProperties(TWitchHut* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID) = 0;
    };

    TGUIGameObject(const TObjectType& objType);
    // A copy draws its own animation phase (h3maped 0x442305).
    TGUIGameObject(const TGUIGameObject& other);

    virtual bool edit(TEditContext* pEditContext, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
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

    unsigned int getAnimOffset() const { return _m_animOffset; }

private:
    ubyte _m_animOffset;
};

// The map's object factory: each create function makes the matching GUI
// object (GUIGameObject.cpp's anonymous classes). The document's own
// factory derives from it (MapDoc.cpp's TTheObjectFactory, whose vtable
// 0x53942c holds these bodies).
class TGUIGameObjectFactory : public TGameMap::TObjectFactory {
public:
    virtual std::auto_ptr<TGenericObject> createGenericObject(const TObjectType& objType) const;
    virtual std::auto_ptr<TGenericObject> createGenericObject(const TObjectType& objType, TRawIStream* pIStream,
                                                              int version) const;
    virtual std::auto_ptr<THeroPlaceholder> createHeroPlaceholder(const TObjectType& objType, TPlayer owner) const;
    virtual std::auto_ptr<THeroPlaceholder> createHeroPlaceholder(const TObjectType& objType, TRawIStream* pIStream,
                                                                  int version) const;
    virtual std::auto_ptr<TNonRandomHero> createNonRandomHero(const TObjectType& objType, TPlayer owner,
                                                              THeroID heroID) const;
    virtual std::auto_ptr<TNonRandomHero> createNonRandomHero(const TObjectType& objType, TRawIStream* pIStream,
                                                              int version) const;
    virtual std::auto_ptr<TRandomHero> createRandomHero(const TObjectType& objType, TPlayer owner) const;
    virtual std::auto_ptr<TRandomHero> createRandomHero(const TObjectType& objType, TRawIStream* pIStream,
                                                        int version) const;
    virtual std::auto_ptr<TPrison> createPrison(const TObjectType& objType, THeroID heroID) const;
    virtual std::auto_ptr<TPrison> createPrison(const TObjectType& objType, TRawIStream* pIStream,
                                                int version) const;
    virtual std::auto_ptr<TTown> createTown(const TObjectType& objType, TPlayer owner) const;
    virtual std::auto_ptr<TTown> createTown(const TObjectType& objType, TRawIStream* pIStream, int version) const;
    virtual std::auto_ptr<TEvent> createEvent(const TObjectType& objType) const;
    virtual std::auto_ptr<TEvent> createEvent(const TObjectType& objType, TRawIStream* pIStream, int version) const;
    virtual std::auto_ptr<TMonster> createMonster(const TObjectType& objType) const;
    virtual std::auto_ptr<TMonster> createMonster(const TObjectType& objType, TRawIStream* pIStream,
                                                  int version) const;
    virtual std::auto_ptr<TSign> createSign(const TObjectType& objType) const;
    virtual std::auto_ptr<TSign> createSign(const TObjectType& objType, TRawIStream* pIStream, int version) const;
    virtual std::auto_ptr<TFlaggableObject> createFlaggable(const TObjectType& objType, TPlayer owner) const;
    virtual std::auto_ptr<TFlaggableObject> createFlaggable(const TObjectType& objType, TRawIStream* pIStream,
                                                            int version) const;
    virtual std::auto_ptr<TMine> createMine(const TObjectType& objType, TPlayer owner) const;
    virtual std::auto_ptr<TMine> createMine(const TObjectType& objType, TRawIStream* pIStream, int version) const;
    virtual std::auto_ptr<TAbandonedMine> createAbandonedMine(const TObjectType& objType) const;
    virtual std::auto_ptr<TAbandonedMine> createAbandonedMine(const TObjectType& objType, TRawIStream* pIStream,
                                                              int version) const;
    virtual std::auto_ptr<TGarrison> createGarrison(const TObjectType& objType, TPlayer owner) const;
    virtual std::auto_ptr<TGarrison> createGarrison(const TObjectType& objType, TRawIStream* pIStream,
                                                    int version) const;
    virtual std::auto_ptr<TGameArtifact> createArtifact(const TObjectType& objType) const;
    virtual std::auto_ptr<TGameArtifact> createArtifact(const TObjectType& objType, TRawIStream* pIStream,
                                                        int version) const;
    virtual std::auto_ptr<TSpellScroll> createSpellScroll(const TObjectType& objType) const;
    virtual std::auto_ptr<TSpellScroll> createSpellScroll(const TObjectType& objType, TRawIStream* pIStream,
                                                          int version) const;
    virtual std::auto_ptr<TGameResource> createResource(const TObjectType& objType) const;
    virtual std::auto_ptr<TGameResource> createResource(const TObjectType& objType, TRawIStream* pIStream,
                                                        int version) const;
    virtual std::auto_ptr<TBlackBox> createBlackBox(const TObjectType& objType) const;
    virtual std::auto_ptr<TBlackBox> createBlackBox(const TObjectType& objType, TRawIStream* pIStream,
                                                    int version) const;
    virtual std::auto_ptr<TScholar> createScholar(const TObjectType& objType) const;
    virtual std::auto_ptr<TScholar> createScholar(const TObjectType& objType, TRawIStream* pIStream,
                                                  int version) const;
    virtual std::auto_ptr<TSeersHut> createSeersHut(const TObjectType& objType) const;
    virtual std::auto_ptr<TSeersHut> createSeersHut(const TObjectType& objType, TRawIStream* pIStream,
                                                    int version) const;
    virtual std::auto_ptr<THolyGrail> createHolyGrail(const TObjectType& objType) const;
    virtual std::auto_ptr<THolyGrail> createHolyGrail(const TObjectType& objType, TRawIStream* pIStream,
                                                      int version) const;
    virtual std::auto_ptr<TShrine> createShrine(const TObjectType& objType) const;
    virtual std::auto_ptr<TShrine> createShrine(const TObjectType& objType, TRawIStream* pIStream,
                                                int version) const;
    virtual std::auto_ptr<TGenerator> createGenerator(const TObjectType& objType, TPlayer owner) const;
    virtual std::auto_ptr<TGenerator> createGenerator(const TObjectType& objType, TRawIStream* pIStream,
                                                      int version) const;
    virtual std::auto_ptr<TRandomlyAlignedGenerator> createRandomlyAlignedGenerator(const TObjectType& objType,
                                                                                    TPlayer owner) const;
    virtual std::auto_ptr<TRandomlyAlignedGenerator> createRandomlyAlignedGenerator(const TObjectType& objType,
        TRawIStream* pIStream, int version) const;
    virtual std::auto_ptr<TRandomlyLeveledGenerator> createRandomlyLeveledGenerator(const TObjectType& objType,
                                                                                    TPlayer owner) const;
    virtual std::auto_ptr<TRandomlyLeveledGenerator> createRandomlyLeveledGenerator(const TObjectType& objType,
        TRawIStream* pIStream, int version) const;
    virtual std::auto_ptr<TRandomGenerator> createRandomGenerator(const TObjectType& objType, TPlayer owner) const;
    virtual std::auto_ptr<TRandomGenerator> createRandomGenerator(const TObjectType& objType, TRawIStream* pIStream,
                                                                  int version) const;
    virtual std::auto_ptr<TQuestGuard> createQuestGuard(const TObjectType& objType) const;
    virtual std::auto_ptr<TQuestGuard> createQuestGuard(const TObjectType& objType, TRawIStream* pIStream,
                                                        int version) const;
    virtual std::auto_ptr<TWitchHut> createWitchHut(const TObjectType& objType) const;
    virtual std::auto_ptr<TWitchHut> createWitchHut(const TObjectType& objType, TRawIStream* pIStream,
                                                    int version) const;
};

#endif  /* HOMM3_EDITOR_GUIGAMEOBJECT_H */
