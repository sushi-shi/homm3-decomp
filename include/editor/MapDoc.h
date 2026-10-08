// MapDoc.h - the map document (Loki MapDoc.cpp): the map, its undo
// history and the placement operations, with the MFC document surface
// (OnNewDocument, Serialize, OnOpenDocument, ...). Methods as the image
// declares them; return types from __PRETTY_FUNCTION__ texts or the retail
// bodies; the virtuals in vtable order (TGameMap::TClient's
// onMapObjectRemoved, the destructor, OnNewDocument, Serialize,
// DeleteContents, OnOpenDocument, ReportSaveLoadException, OnSaveDocument,
// SaveModified). The bases' order follows the thunk deltas (TClient at +0,
// the terrain, river and road clients at +4, +8, +12).
//
// The data members (0x7c bytes, as cppbridge.cpp allocates) follow the
// constructor's initializers and their users; the assert texts name
// _m_pMap, _m_undoQueue, _m_currentIndex and the five operation pointers.
// The undo queue is a deque<TGameMap>; the new-map parameters are copied
// from s_kDefaultParams (8 bytes: the size and the two-layer flag, as
// setNewMapParams stores them). The other member names, the parameters'
// type name and the last word (never touched by MapDoc.o) are not proven.
// The load failures are thrown by pointer; their in-class bodies are the
// linkonce copies MapDoc.o owns. Views are notified through UpdateAllViews
// with the address of a TNotification (TMapView::OnUpdate names
// m_pUpdateParams); the kinds' and the removal parameters' names are not
// proven. The in-class members after the virtuals are strong in MapDoc.o,
// in declaration order.
#ifndef HOMM3_EDITOR_MAPDOC_H
#define HOMM3_EDITOR_MAPDOC_H

#include <deque>
#include <exception>
#include <memory>

// The map file format version MapDoc.cpp reads and writes. Declared before
// GameMap.h: MapDoc.o and cppbridge.o emit it ahead of every constant that
// header brings in.
const int kMapFileVersion = 14;

#include "editor/GameMap.h"
#include "editor/TerrainPlacement.h"
#include "editor/RiverPlacement.h"
#include "editor/RoadPlacement.h"
#include "editor/MapView.h"

class TGameObject;
class TMapView;
class TTerrainPlacementOp;
class TRiverPlacementOp;
class TRiverEraseOp;
class TRoadPlacementOp;
class TRoadEraseOp;

class TMapDocLoadFailure : public exception {
public:
    TMapDocLoadFailure(bool bLoading) {}
};

class TMapDocInvalidFileVersion : public TMapDocLoadFailure {
public:
    TMapDocInvalidFileVersion(int version, int expectedVersion, bool bLoading)
        : TMapDocLoadFailure(bLoading),
          _m_version(version),
          _m_expectedVersion(expectedVersion)
    {
    }

private:
    int _m_version;
    int _m_expectedVersion;
};

class TMapDoc : private TGameMap::TClient,
                public TTerrainPlacementOpClient,
                public TRiverPlacementOpClient,
                public TRoadPlacementOpClient {
public:
    struct TUpdateParams {
        TUpdateParams(const CRect& rect, bool bSecondLayer) : m_rect(rect), m_bSecondLayer(bSecondLayer) {}

        CRect m_rect;
        bool m_bSecondLayer;
    };

    struct TObjectRemovedParams {
        bool m_bSecondLayer;
        TMapLayerObjectID m_objID;
    };

    struct TNotification {
        enum TType {
            eUpdate,
            eClear,
            eReplace,
            eObjectRemoved
        };

        TType m_type;
        union {
            const TUpdateParams* m_pUpdateParams;
            const TObjectRemovedParams* m_pObjectRemovedParams;
        };
    };

    static const unsigned int s_kMaxSpecialTileFrequency = 8;

    static void setAutosaveInterval(unsigned int interval);
    void setNewMapParams(TGameMap::TSize size, bool bTwoLayer);
    static void setSpecialTileFrequency(unsigned int newFrequency);
    static unsigned int getSpecialTileFrequency() { return _s_specialTileFrequency; }

    TMapDoc();
    TMapDoc(void* pParent, TMapView* pView);

    void onMapObjectRemoved(bool bSecondLayer, TMapLayerObjectID objID);
    virtual ~TMapDoc();
    virtual BOOL OnNewDocument();
    virtual void Serialize(char* pathName, bool bStoring);
    virtual void DeleteContents();
    virtual BOOL OnOpenDocument(char* pathName);
    virtual void ReportSaveLoadException(char* pathName);
    virtual bool OnSaveDocument(char* pathName);
    virtual bool SaveModified();

    void undo();
    void redo();
    void backupMap();
    void backupMap(const TGameMap& map);

    TMapLayerObjectID placeObject(bool bSecondLayer, const TGameObject& obj,
                                  unsigned int x, unsigned int y);
    void removeObject(bool bSecondLayer, unsigned int objID);
    void floatObject(bool bSecondLayer, unsigned int objID);
    void unfloatObject(bool bSecondLayer, unsigned int x, unsigned int y);
    void onObjectPropertiesChanged(bool bSecondLayer, unsigned int objID);

    void startTerrainPlacementOp(bool bSecondLayer, TTerrainType terrainType);
    void endTerrainPlacementOp();
    void terrainFill(unsigned int left, unsigned int top, unsigned int width, unsigned int height);
    void startRiverPlacementOp(bool bSecondLayer, TRiverType riverType,
                               unsigned int x, unsigned int y);
    void endRiverPlacementOp();
    void placeRiver(unsigned int x, unsigned int y);
    void startRoadPlacementOp(bool bSecondLayer, TRoadType roadType,
                              unsigned int x, unsigned int y);
    void endRoadPlacementOp();
    void placeRoad(unsigned int x, unsigned int y);
    void startEraseOp(bool bSecondLayer);
    void endEraseOp();
    void erase(unsigned int left, unsigned int top, unsigned int width, unsigned int height);

    void onTerrainTypeChanged(bool bSecondLayer, unsigned int x, unsigned int y,
                              TTerrainType newType);
    void onTerrainUpdated(bool bSecondLayer, unsigned int left, unsigned int top,
                          unsigned int width, unsigned int height);
    void onRiversUpdated(bool bSecondLayer, unsigned int left, unsigned int top,
                         unsigned int width, unsigned int height);
    void onPlacingRiver(bool bSecondLayer, unsigned int x, unsigned int y);
    void onRoadsUpdated(bool bSecondLayer, unsigned int left, unsigned int top,
                        unsigned int width, unsigned int height);
    void onPlacingRoad(bool bSecondLayer, unsigned int x, unsigned int y);

    void OnFileExportText(char* pathName);
    void OnFileImportText(char* pathName);
    void OnToolsRepaintMap();
    void OnFileBatchConvert();
    void promptForNewMapParams() { _m_bPromptForNewMapParams = true; }

    TGameMap* getPMap() { return _m_pMap; }
    bool canUndo() const { return _m_currentIndex != 0; }
    bool canRedo() const { return _m_currentIndex < _m_undoQueue.size(); }
    bool IsModified() { return m_bModified; }
    void SetModifiedFlag() { m_bModified = true; }
    void SetModifiedFlag(bool bModified) { m_bModified = bModified; }
    void UpdateAllViews(void* pSender, unsigned long hint) { m_pView->OnUpdate(this, hint); }
    void UpdateAllViews(void* pSender) { m_pView->OnUpdate(this, 0); }

private:
    struct TNewMapParams {
        TGameMap::TSize m_size;
        bool m_bTwoLayer;
    };

    static const TNewMapParams s_kDefaultParams;
    static unsigned int _s_autosaveInterval;
    static unsigned int _s_specialTileFrequency;

    void _sendUpdate(bool bSecondLayer, unsigned int left, unsigned int top,
                     unsigned int width, unsigned int height);

protected:
    bool m_bModified;
    TMapView* m_pView;

private:
    auto_ptr<TGameMap::TObjectFactory> _m_pObjectFactory;
    TGameMap* _m_pMap;
    deque<TGameMap> _m_undoQueue;
    unsigned int _m_currentIndex;
    unsigned int _m_savedIndex;
    bool _m_bPromptForNewMapParams;
    TNewMapParams _m_newMapParams;
    bool _m_bFlag5c;
    bool _m_bCreatingMap;
    TTerrainPlacementOp* _m_pTerrainPlacementOp;
    TRiverPlacementOp* _m_pRiverPlacementOp;
    TRiverEraseOp* _m_pRiverEraseOp;
    TRoadPlacementOp* _m_pRoadPlacementOp;
    TRoadEraseOp* _m_pRoadEraseOp;
    bool _m_bEraseUnderground;
    unsigned int _m_unknown78;
};

#endif  /* HOMM3_EDITOR_MAPDOC_H */
