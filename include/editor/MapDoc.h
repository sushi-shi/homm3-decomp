// MapDoc.h - the map document (MapDoc.cpp; Loki h3maped object 57). The
// Windows document is an MFC CDocument (DECLARE_DYNCREATE, 0x14c bytes) and
// the map's TGameMap::TClient (vtable 0x5397ec at +0x50). It owns the map
// and its undo queue, runs the terrain, river, road and erase operations
// through its nested adapters, loads and saves .h3m files (gzip-compressed
// raw streams with the format version first), imports and exports the
// text form, generates random maps, and autosaves to the temporary
// directory while a named mutex says that no other editor owns the file.
//
// Views are notified through UpdateAllViews with the address of a
// TNotification. Each map in the undo queue keeps the revision number it
// had; views follow the revisions to keep their own per-map state.
//
// Layout from the constructors: the map at +0x54, the current revision and
// the last one handed out, the undo queue (a Dinkumware deque at +0x60),
// its current and saved indices, the new-map parameters at +0x9c, the
// operation adapters at +0xb4..+0xc4, the autosave path, mutex, thread and
// close event at +0xcc..+0x140, and the time of the last save.
#ifndef HOMM3_EDITOR_MAPDOC_H
#define HOMM3_EDITOR_MAPDOC_H

#include <deque>
#include <memory>
#include <utility>

#include <afxmt.h>

#include "gameversion.h"
#include "rmg_request.h"
#include "editor/GameMap.h"

class TGameObject;

// The newest map file version (Shadow of Death's); a file of another
// version is rejected with it.
const int kMapFileVersion = 28;

class TMapDocLoadFailure : public CException {
    DECLARE_DYNAMIC(TMapDocLoadFailure)

public:
    TMapDocLoadFailure() : CException(TRUE) {}
};

class TMapDocInvalidFileVersion : public TMapDocLoadFailure {
    DECLARE_DYNAMIC(TMapDocInvalidFileVersion)

public:
    TMapDocInvalidFileVersion(int version, int expectedVersion)
        : _m_version(version),
          _m_expectedVersion(expectedVersion)
    {
    }

    int getVersion() const { return _m_version; }
    int getExpectedVersion() const { return _m_expectedVersion; }

private:
    int _m_version;
    int _m_expectedVersion;
};

class TMapDoc : public CDocument, private TGameMap::TClient {
    DECLARE_DYNCREATE(TMapDoc)

public:
    // The random map generator's settings (six words, copied into the
    // generator's request).
    struct TRandomMapParams {
        int m_humanPlayerCount;
        int m_humanTeamCount;
        int m_computerPlayerCount;
        int m_computerTeamCount;
        ERmgWaterContent m_waterContent;
        int m_monsterStrength;
    };

    // The next new map's version, size and levels, and the random map
    // settings when the map is to be generated.
    struct TNewMapParams {
        TNewMapParams() {}
        TNewMapParams(EGameVersion version, TGameMap::TSize size, bool bTwoLayer,
                      std::auto_ptr<TRandomMapParams> pRandomMapParams)
            : m_version(version),
              m_size(size),
              m_bTwoLayer(bTwoLayer),
              m_pRandomMapParams(pRandomMapParams)
        {
        }
        TNewMapParams(const TNewMapParams& other);

        EGameVersion m_version;
        TGameMap::TSize m_size;
        bool m_bTwoLayer;
        std::auto_ptr<TRandomMapParams> m_pRandomMapParams;
    };

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
            eObjectRemoved,
            eBackup,
            eDiscardBackup,
            eUndo,
            eRedo
        };

        TType m_type;
        union {
            const TUpdateParams* m_pUpdateParams;
            const TObjectRemovedParams* m_pObjectRemovedParams;
            unsigned int m_revision;
        };
    };

    static const TNewMapParams s_kDefaultNewMapParams;

    static void autosaveAll();
    static void setAutosaveInterval(unsigned int interval);
    static void setSpecialTileFrequency(unsigned int newFrequency);
    static unsigned int getSpecialTileFrequency() { return _s_specialTileFrequency; }

    TMapDoc(CDocTemplate* pTemplate);
    virtual ~TMapDoc();
    virtual BOOL IsModified() { return m_bModified; }
    virtual void SetModifiedFlag(BOOL bModified = TRUE) { m_bModified = bModified; }
    virtual BOOL OnNewDocument();
    virtual void Serialize(CArchive& ar);
    virtual void DeleteContents();
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
    virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
    virtual void ReportSaveLoadException(LPCTSTR lpszPathName, CException* e, BOOL bSaving,
                                         UINT nIDPDefault);
    virtual BOOL SaveModified();

    void undo();
    void redo();
    void backupMap();

    TMapLayerObjectID placeObject(bool bSecondLayer, std::auto_ptr<TGameObject> pObj, const TTilePoint& loc);
    void removeObject(bool bSecondLayer, unsigned int objID);
    void floatObject(bool bSecondLayer, unsigned int objID);
    void unfloatObject(bool bSecondLayer, unsigned int x, unsigned int y);
    void onObjectPropertiesChanged(bool bSecondLayer, unsigned int objID);

    void startTerrainPlacementOp(bool bSecondLayer, TTerrainType terrainType);
    void endTerrainPlacementOp();
    void terrainFill(unsigned int left, unsigned int top, unsigned int width, unsigned int height);
    void startRiverPlacementOp(bool bSecondLayer, int riverType, unsigned int x, unsigned int y);
    void endRiverPlacementOp();
    void placeRiver(unsigned int x, unsigned int y);
    void startRoadPlacementOp(bool bSecondLayer, int roadType, unsigned int x, unsigned int y);
    void endRoadPlacementOp();
    void placeRoad(unsigned int x, unsigned int y);
    void startEraseOp(bool bSecondLayer);
    void endEraseOp();
    void erase(unsigned int left, unsigned int top, unsigned int width, unsigned int height);
    void startRiverEraseOp(bool bSecondLayer);
    void endRiverEraseOp();
    void eraseRiver(unsigned int x, unsigned int y);
    void startRoadEraseOp(bool bSecondLayer);
    void endRoadEraseOp();
    void eraseRoad(unsigned int x, unsigned int y);

    void promptForNewMapParams() { _m_bPromptForNewMapParams = true; }
    void setNewMapParams(const TNewMapParams& params) { _m_newMapParams = params; }
    TGameMap* getPMap() { return _m_pMap; }
    unsigned int getRevision() const { return _m_revision; }
    bool canUndo() const { return _m_currentIndex > 0; }
    bool canRedo() const { return _m_currentIndex < _m_undoQueue.size(); }

protected:
    TMapDoc();

    afx_msg void OnFileExportText();
    afx_msg void OnFileImportText();
    afx_msg void OnToolsRepaintMap();
    afx_msg void OnFileBatchConvert();
    DECLARE_MESSAGE_MAP()

private:
    class _TTerrainPlacementOp;
    class _TRiverPlacementOp;
    class _TRiverEraseOp;
    class _TRoadPlacementOp;
    class _TRoadEraseOp;
    friend class _TTerrainPlacementOp;
    friend class _TRiverPlacementOp;
    friend class _TRiverEraseOp;
    friend class _TRoadPlacementOp;
    friend class _TRoadEraseOp;

    typedef std::pair<unsigned int, TGameMap> _TUndoEntry;

    enum { s_kMaxUndoQueueSize = 32 };

    static unsigned int _s_autosaveInterval;
    static unsigned int _s_specialTileFrequency;

    static UINT _autosaveThreadProc(LPVOID pParam);

    virtual void onMapObjectRemoved(bool bSecondLayer, TMapLayerObjectID objID);

    std::auto_ptr<TGameMap> _readMap(std::streambuf* pStreamBuf);
    bool _generateRandomMap();
    void _onTerrainTypeChanged(bool bSecondLayer, unsigned int x, unsigned int y, TTerrainType oldType);
    // The operations' updates reach the views unless the map is being
    // created (Loki's TTerrainPlacementOpClient and line-operation client
    // callbacks).
    void _onTerrainUpdated(bool bSecondLayer, unsigned int left, unsigned int top, unsigned int width,
                           unsigned int height)
    {
        if (!_m_bCreatingMap)
            _sendUpdate(bSecondLayer, left, top, width, height);
    }
    void _onRiversUpdated(bool bSecondLayer, unsigned int left, unsigned int top, unsigned int width,
                          unsigned int height)
    {
        if (!_m_bCreatingMap)
            _sendUpdate(bSecondLayer, left, top, width, height);
    }
    void _onRoadsUpdated(bool bSecondLayer, unsigned int left, unsigned int top, unsigned int width,
                         unsigned int height)
    {
        if (!_m_bCreatingMap)
            _sendUpdate(bSecondLayer, left, top, width, height);
    }
    void _sendUpdate(bool bSecondLayer, unsigned int left, unsigned int top, unsigned int width,
                     unsigned int height);
    int _autosave();

    TGameMap* _m_pMap;
    unsigned int _m_revision;
    unsigned int _m_lastRevision;
    std::deque<_TUndoEntry> _m_undoQueue;
    unsigned int _m_currentIndex;
    unsigned int _m_savedIndex;
    bool _m_bPromptForNewMapParams;
    TNewMapParams _m_newMapParams;
    bool _m_bOfferAutosave;
    bool _m_bCreatingMap;
    _TTerrainPlacementOp* _m_pTerrainPlacementOp;
    _TRiverPlacementOp* _m_pRiverPlacementOp;
    _TRiverEraseOp* _m_pRiverEraseOp;
    _TRoadPlacementOp* _m_pRoadPlacementOp;
    _TRoadEraseOp* _m_pRoadEraseOp;
    bool _m_bEraseUnderground;
    bool _m_bOwnsAutosave;
    CString _m_autosavePathName;
    CMutex _m_autosaveMutex;
    CWinThread _m_autosaveThread;
    CEvent _m_closeEvent;
    DWORD _m_lastSaveTime;
};

#endif  /* HOMM3_EDITOR_MAPDOC_H */
