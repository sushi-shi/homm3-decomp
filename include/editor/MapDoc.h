// MapDoc.h - the map document (Loki MapDoc.cpp): the map, its undo
// history and the placement operations, with the MFC document surface
// (OnNewDocument, Serialize, OnOpenDocument, ...). Methods as the image
// declares them; return types from __PRETTY_FUNCTION__ texts or the retail
// bodies; the virtuals in vtable order (TGameMap::TClient's
// onMapObjectRemoved, the destructor, OnNewDocument, Serialize,
// DeleteContents, OnOpenDocument, ReportSaveLoadException, OnSaveDocument,
// SaveModified). The bases' order follows the thunk deltas (TClient at +0,
// the terrain, river and road clients at +4, +8, +12). Data members are not
// declared yet (cppbridge.cpp allocates 0x7c bytes); getPMap reads +0x1c.
// The load failures are thrown by pointer; their inline bodies (linkonce,
// owned by MapDoc.cpp) are not written yet.
#ifndef HOMM3_EDITOR_MAPDOC_H
#define HOMM3_EDITOR_MAPDOC_H

#include <exception>

#include "editor/GameMap.h"
#include "editor/TerrainPlacement.h"
#include "editor/RiverPlacement.h"
#include "editor/RoadPlacement.h"

class TGameObject;
class TMapView;

class TMapDocLoadFailure : public exception {
public:
    TMapDocLoadFailure(bool bLoading);
    virtual ~TMapDocLoadFailure();
};

class TMapDocInvalidFileVersion : public TMapDocLoadFailure {
public:
    TMapDocInvalidFileVersion(int version, int expectedVersion, bool bLoading);
    virtual ~TMapDocInvalidFileVersion();
};

class TMapDoc : public TGameMap::TClient,
                public TTerrainPlacementOpClient,
                public TRiverPlacementOpClient,
                public TRoadPlacementOpClient {
public:
    static void setAutosaveInterval(unsigned int interval);
    void setNewMapParams(TGameMap::TSize size, bool bTwoLayer);
    static void setSpecialTileFrequency(unsigned int frequency);
    static unsigned int getSpecialTileFrequency();

    TMapDoc();
    TMapDoc(void* pParent, TMapView* pView);

    void onMapObjectRemoved(bool bUnderground, TMapLayerObjectID objID);
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
    bool canUndo() const;
    bool canRedo() const;
    void backupMap();
    void backupMap(const TGameMap& map);

    TMapLayerObjectID placeObject(bool bUnderground, const TGameObject& obj,
                                  unsigned int x, unsigned int y);
    void removeObject(bool bUnderground, unsigned int objID);
    void floatObject(bool bUnderground, unsigned int objID);
    void unfloatObject(bool bUnderground, unsigned int x, unsigned int y);
    void onObjectPropertiesChanged(bool bUnderground, unsigned int objID);

    void startTerrainPlacementOp(bool bUnderground, TTerrainType terrainType);
    void endTerrainPlacementOp();
    void terrainFill(unsigned int left, unsigned int top, unsigned int width, unsigned int height);
    void startRiverPlacementOp(bool bUnderground, TRiverType riverType,
                               unsigned int x, unsigned int y);
    void endRiverPlacementOp();
    void placeRiver(unsigned int x, unsigned int y);
    void startRoadPlacementOp(bool bUnderground, TRoadType roadType,
                              unsigned int x, unsigned int y);
    void endRoadPlacementOp();
    void placeRoad(unsigned int x, unsigned int y);
    void startEraseOp(bool bUnderground);
    void endEraseOp();
    void erase(unsigned int left, unsigned int top, unsigned int width, unsigned int height);

    void onTerrainTypeChanged(bool bUnderground, unsigned int x, unsigned int y,
                              TTerrainType newType);
    void onTerrainUpdated(bool bUnderground, unsigned int left, unsigned int top,
                          unsigned int width, unsigned int height);
    void onRiversUpdated(bool bUnderground, unsigned int left, unsigned int top,
                         unsigned int width, unsigned int height);
    void onPlacingRiver(bool bUnderground, unsigned int x, unsigned int y);
    void onRoadsUpdated(bool bUnderground, unsigned int left, unsigned int top,
                        unsigned int width, unsigned int height);
    void onPlacingRoad(bool bUnderground, unsigned int x, unsigned int y);

    void OnFileExportText(char* pathName);
    void OnFileImportText(char* pathName);
    void OnToolsRepaintMap();
    void OnFileBatchConvert();
    void promptForNewMapParams();

    TGameMap* getPMap();
    bool IsModified();
    void SetModifiedFlag();
    void SetModifiedFlag(bool bModified);
    void UpdateAllViews(void* pSender, unsigned long hint);
    void UpdateAllViews(void* pSender);

private:
    void _sendUpdate(bool bUnderground, unsigned int left, unsigned int top,
                     unsigned int width, unsigned int height);
};

#endif  /* HOMM3_EDITOR_MAPDOC_H */
