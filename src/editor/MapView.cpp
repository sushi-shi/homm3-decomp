// MapView.cpp - the map view (h3maped 0x47ff04..0x486b74; Loki h3maped
// object 53). The view creates the map frame, the mini map and the toolkit
// window over the document's map, turns their events into document
// operations, opens the objects' property dialogs and carries the Tools,
// Players, View and Edit menu handlers. Release drops Loki's asserts.
//
// GOG's view adds the obstacle tool (the mask, its brushes and its undo
// history across the document's revisions), a generic properties dialog
// for objects without properties, the map validation dialog and the find
// dialog's patterns. Loki's _animate is gone (animate calls the frames
// itself); the other per-view setters keep their int results for
// mem_fun1.
#include "editor/stdafx.h"

#include <algorithm>
#include <functional>
#include <set>
#include <string>

#include "va.h"
#include "gamecontext.h"
#include "objnames.h"
#include "rmg_request.h"
#include "retailobjecttype.h"
#include "editor/AbandonedMinePropsDlg.h"
#include "editor/ArtifactPropsSheet.h"
#include "editor/BlackBoxPropsSheet.h"
#include "editor/EventPropsSheet.h"
#include "editor/FlaggablePropsDlg.h"
#include "editor/FormattedString.h"
#include "editor/GameMap.h"
#include "editor/GarrisonPropsDlg.h"
#include "editor/Hero.h"
#include "editor/HeroPlaceholderPropsDlg.h"
#include "editor/HeroPropsSheet.h"
#include "editor/HolyGrailPropsDlg.h"
#include "editor/MapDoc.h"
#include "editor/MapEditorText.h"
#include "editor/MapFrameWnd.h"
#include "editor/MapSpecsSheet.h"
#include "editor/MapValidation.h"
#include "editor/MapView.h"
#include "editor/MiniMapWnd.h"
#include "editor/MonsterPropsSheet.h"
#include "editor/ObjectSpecializations.h"
#include "editor/random_object_placer.h"
#include "editor/resource.h"
#include "editor/ResourcePropsSheet.h"
#include "editor/ScholarPropsDlg.h"
#include "editor/ShrinePropsDlg.h"
#include "editor/SignPropsDlg.h"
#include "editor/SpellScrollPropsSheet.h"
#include "editor/TownPropsSheet.h"
#include "editor/WitchHutPropsDlg.h"

namespace {

// Asks nothing of an object without properties: the caption names the
// object, the text says it has none.
class TObjectPropsDlg : public CDialog {
public:
    TObjectPropsDlg(CWnd* pParent, const TGameObject* pObject)
        : CDialog(IDD_OBJECT_PROPS, pParent), _m_pObject(pObject) {}

protected:
    virtual BOOL OnInitDialog();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    DECLARE_MESSAGE_MAP()

private:
    const TGameObject* _m_pObject;
};

// Shows the validation's notes.
class TMapValidationDlg : public CDialog {
public:
    TMapValidationDlg(CWnd* pParent, const CString& problems)
        : CDialog(IDD_MAP_VALIDATION, pParent), _m_problems(problems)
    {
        _m_problems.Replace("\n", "\r\n");
    }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    DECLARE_MESSAGE_MAP()

private:
    CString _m_problems;
    CEdit _m_problemsEdit;
};

// One layer of the map and the obstacle mask as the random object
// placer's map.
class TObstaclePlacerMap : public t_random_object_map {
public:
    TObstaclePlacerMap(TGameMap* pMap, const TGameMapMask* pMask, bool bSecondLayer);

    virtual bool getFirstObject(unsigned int* pObjID);
    virtual bool getNextObject(unsigned int* pObjID);
    virtual const TObjectType& getObjectType(unsigned int objID);
    virtual TTilePoint getObjectLoc(unsigned int objID);
    virtual int getTerrain(unsigned int x, unsigned int y);
    virtual bool isClear(unsigned int x, unsigned int y);
    virtual bool isObstacleArea(unsigned int x, unsigned int y);
    virtual void placeObject(const TObjectType& objType, TTilePoint loc);
    virtual void eraseObject(unsigned int objID);

private:
    TGameMap* _m_pMap;
    const TGameMapMask* _m_pMask;
    bool _m_bSecondLayer;
};

DATA(0x005a1e80) std::set<TMapView*> allMapViews;

VA(0x00480112, 0x6)
BEGIN_MESSAGE_MAP(TObjectPropsDlg, CDialog)
    ON_WM_CREATE()
END_MESSAGE_MAP()

VA(0x00480118, 0x58)
BOOL TObjectPropsDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_NO_PROPERTIES_STATIC)->SetWindowText(kNoPropertiesStr);
    CDialog::OnInitDialog();
    return TRUE;
}

VA(0x00480170, 0x8f)
int TObjectPropsDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    CString caption;
    caption.Format(kObjectPropertiesCaptionFmtStr, _m_pObject->getTypeName().c_str());
    SetWindowText(caption);
    return 0;
}

VA(0x004801ff, 0x6)
BEGIN_MESSAGE_MAP(TMapValidationDlg, CDialog)
    ON_WM_CREATE()
END_MESSAGE_MAP()

VA(0x00480205, 0x25)
int TMapValidationDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(kMapValidationCaptionStr);
    return 0;
}

VA(0x0048022a, 0x15)
void TMapValidationDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_PROBLEMS_EDIT, _m_problemsEdit);
}

VA(0x0048023f, 0x6d)
BOOL TMapValidationDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_PROBLEMS_STATIC)->SetWindowText(kProblemsStaticStr);
    CDialog::OnInitDialog();
    _m_problemsEdit.SetFocus();
    _m_problemsEdit.SetWindowText(_m_problems);
    return FALSE;
}

VA(0x004802ac, 0x3e)
TObstaclePlacerMap::TObstaclePlacerMap(TGameMap* pMap, const TGameMapMask* pMask, bool bSecondLayer)
    : t_random_object_map(pMap->getWidth(), pMap->getHeight()),
      _m_pMap(pMap),
      _m_pMask(pMask),
      _m_bSecondLayer(bSecondLayer)
{
}

VA(0x004802ea, 0x31)
bool TObstaclePlacerMap::getFirstObject(unsigned int* pObjID)
{
    TMapLayerObjectID objID = static_cast<const TGameMap*>(_m_pMap)->getLayer(_m_bSecondLayer).getFirstObjectID();
    if (objID == TGameMap::TLayer::s_kInvalidObjID)
        return false;
    *pObjID = objID;
    return true;
}

VA(0x0048031b, 0x37)
bool TObstaclePlacerMap::getNextObject(unsigned int* pObjID)
{
    TMapLayerObjectID objID = static_cast<const TGameMap*>(_m_pMap)->getLayer(_m_bSecondLayer).getNextObjectID(*pObjID);
    if (objID == TGameMap::TLayer::s_kInvalidObjID)
        return false;
    *pObjID = objID;
    return true;
}

VA(0x00480352, 0x29)
const TObjectType& TObstaclePlacerMap::getObjectType(unsigned int objID)
{
    return static_cast<const TGameMap*>(_m_pMap)->getLayer(_m_bSecondLayer).getObject(objID).getObjectType();
}

VA(0x0048037b, 0x3b)
TTilePoint TObstaclePlacerMap::getObjectLoc(unsigned int objID)
{
    return static_cast<const TGameMap*>(_m_pMap)->getLayer(_m_bSecondLayer).getObjectLoc(objID);
}

VA(0x004803b6, 0x2c)
int TObstaclePlacerMap::getTerrain(unsigned int x, unsigned int y)
{
    return static_cast<const TGameMap*>(_m_pMap)->getLayer(_m_bSecondLayer).getCell(x, y).getTerrainType();
}

VA(0x004803e2, 0x1d)
bool TObstaclePlacerMap::isClear(unsigned int x, unsigned int y)
{
    return _m_pMask->getTileState(x, y, _m_bSecondLayer) == TGameMapMask::eTileClear;
}

VA(0x004803ff, 0x1e)
bool TObstaclePlacerMap::isObstacleArea(unsigned int x, unsigned int y)
{
    return _m_pMask->getTileState(x, y, _m_bSecondLayer) == TGameMapMask::eTileObstacle;
}

VA(0x0048041d, 0x5f)
void TObstaclePlacerMap::placeObject(const TObjectType& objType, TTilePoint loc)
{
    std::auto_ptr<TGameObject> pObj = _m_pMap->createObject(objType, ePlayerNone);
    _m_pMap->insertObject(_m_bSecondLayer, pObj, loc);
}

VA(0x0048047c, 0x13)
void TObstaclePlacerMap::eraseObject(unsigned int objID)
{
    _m_pMap->eraseObject(_m_bSecondLayer, objID);
}

}  // namespace

DATA(0x005a1eb8) TZoom TMapView::_s_zoom;
DATA(0x005a1ebc) bool TMapView::_s_bViewGrid;
DATA(0x005a1ebd) bool TMapView::_s_bViewPassability;

VA(0x0048048f, 0x35)
IMPLEMENT_DYNCREATE(TMapView, CView)

VA(0x004804ca, 0x6)
BEGIN_MESSAGE_MAP(TMapView, CView)
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_COMMAND(ID_TOOLS_ERASE, OnToolsErase)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_ERASE, OnUpdateToolsErase)
    ON_COMMAND(ID_TOOLS_ERASE_1X1, OnToolsErase1x1)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_ERASE_1X1, OnUpdateToolsErase1x1)
    ON_COMMAND(ID_TOOLS_ERASE_2X2, OnToolsErase2x2)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_ERASE_2X2, OnUpdateToolsErase2x2)
    ON_COMMAND(ID_TOOLS_ERASE_4X4, OnToolsErase4x4)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_ERASE_4X4, OnUpdateToolsErase4x4)
    ON_COMMAND(ID_TOOLS_ERASE_FILL, OnToolsEraseFill)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_ERASE_FILL, OnUpdateToolsEraseFill)
    ON_COMMAND(ID_TOOLS_MAP_SPECIFICATIONS, OnToolsMapSpecifications)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_MAP_SPECIFICATIONS, OnUpdateToolsMapSpecifications)
    ON_COMMAND(ID_TOOLS_OBJECTS_ARTIFACTS, OnToolsObjectsArtifacts)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBJECTS_ARTIFACTS, OnUpdateToolsObjectsArtifacts)
    ON_COMMAND(ID_TOOLS_OBJECTS_DIRT, OnToolsObjectsDirt)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBJECTS_DIRT, OnUpdateToolsObjectsDirt)
    ON_COMMAND(ID_TOOLS_OBJECTS_GRASS, OnToolsObjectsGrass)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBJECTS_GRASS, OnUpdateToolsObjectsGrass)
    ON_COMMAND(ID_TOOLS_OBJECTS_HEROES, OnToolsObjectsHeroes)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBJECTS_HEROES, OnUpdateToolsObjectsHeroes)
    ON_COMMAND(ID_TOOLS_OBJECTS_LAVA, OnToolsObjectsLava)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBJECTS_LAVA, OnUpdateToolsObjectsLava)
    ON_COMMAND(ID_TOOLS_OBJECTS_MONSTERS, OnToolsObjectsMonsters)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBJECTS_MONSTERS, OnUpdateToolsObjectsMonsters)
    ON_COMMAND(ID_TOOLS_OBJECTS_ROUGH, OnToolsObjectsRough)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBJECTS_ROUGH, OnUpdateToolsObjectsRough)
    ON_COMMAND(ID_TOOLS_OBJECTS_SAND, OnToolsObjectsSand)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBJECTS_SAND, OnUpdateToolsObjectsSand)
    ON_COMMAND(ID_TOOLS_OBJECTS_SNOW, OnToolsObjectsSnow)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBJECTS_SNOW, OnUpdateToolsObjectsSnow)
    ON_COMMAND(ID_TOOLS_OBJECTS_SUBTERRANEAN, OnToolsObjectsSubterranean)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBJECTS_SUBTERRANEAN, OnUpdateToolsObjectsSubterranean)
    ON_COMMAND(ID_TOOLS_OBJECTS_SWAMP, OnToolsObjectsSwamp)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBJECTS_SWAMP, OnUpdateToolsObjectsSwamp)
    ON_COMMAND(ID_TOOLS_OBJECTS_TOWNS, OnToolsObjectsTowns)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBJECTS_TOWNS, OnUpdateToolsObjectsTowns)
    ON_COMMAND(ID_TOOLS_OBJECTS_TREASURES, OnToolsObjectsTreasures)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBJECTS_TREASURES, OnUpdateToolsObjectsTreasures)
    ON_COMMAND(ID_TOOLS_OBJECTS_WATER, OnToolsObjectsWater)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBJECTS_WATER, OnUpdateToolsObjectsWater)
    ON_COMMAND(ID_TOOLS_RIVERS, OnToolsRivers)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_RIVERS, OnUpdateToolsRivers)
    ON_COMMAND(ID_TOOLS_RIVERS_CLEAR, OnToolsRiversClear)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_RIVERS_CLEAR, OnUpdateToolsRiversClear)
    ON_COMMAND(ID_TOOLS_RIVERS_ICY, OnToolsRiversIcy)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_RIVERS_ICY, OnUpdateToolsRiversIcy)
    ON_COMMAND(ID_TOOLS_RIVERS_LAVA, OnToolsRiversLava)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_RIVERS_LAVA, OnUpdateToolsRiversLava)
    ON_COMMAND(ID_TOOLS_RIVERS_MUDDY, OnToolsRiversMuddy)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_RIVERS_MUDDY, OnUpdateToolsRiversMuddy)
    ON_COMMAND(ID_TOOLS_ROADS, OnToolsRoads)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_ROADS, OnUpdateToolsRoads)
    ON_COMMAND(ID_TOOLS_ROADS_COBBLESTONE, OnToolsRoadsCobblestone)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_ROADS_COBBLESTONE, OnUpdateToolsRoadsCobblestone)
    ON_COMMAND(ID_TOOLS_ROADS_DIRT, OnToolsRoadsDirt)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_ROADS_DIRT, OnUpdateToolsRoadsDirt)
    ON_COMMAND(ID_TOOLS_ROADS_GRAVEL, OnToolsRoadsGravel)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_ROADS_GRAVEL, OnUpdateToolsRoadsGravel)
    ON_COMMAND(ID_TOOLS_TERRAIN, OnToolsTerrain)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_TERRAIN, OnUpdateToolsTerrain)
    ON_COMMAND(ID_TOOLS_TERRAIN_1X1, OnToolsTerrain1x1)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_TERRAIN_1X1, OnUpdateToolsTerrain1x1)
    ON_COMMAND(ID_TOOLS_TERRAIN_2X2, OnToolsTerrain2x2)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_TERRAIN_2X2, OnUpdateToolsTerrain2x2)
    ON_COMMAND(ID_TOOLS_TERRAIN_4X4, OnToolsTerrain4x4)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_TERRAIN_4X4, OnUpdateToolsTerrain4x4)
    ON_COMMAND(ID_TOOLS_TERRAIN_FILL, OnToolsTerrainFill)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_TERRAIN_FILL, OnUpdateToolsTerrainFill)
    ON_COMMAND(ID_TOOLS_TERRAIN_DIRT, OnToolsTerrainDirt)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_TERRAIN_DIRT, OnUpdateToolsTerrainDirt)
    ON_COMMAND(ID_TOOLS_TERRAIN_GRASS, OnToolsTerrainGrass)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_TERRAIN_GRASS, OnUpdateToolsTerrainGrass)
    ON_COMMAND(ID_TOOLS_TERRAIN_LAVA, OnToolsTerrainLava)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_TERRAIN_LAVA, OnUpdateToolsTerrainLava)
    ON_COMMAND(ID_TOOLS_TERRAIN_ROUGH, OnToolsTerrainRough)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_TERRAIN_ROUGH, OnUpdateToolsTerrainRough)
    ON_COMMAND(ID_TOOLS_TERRAIN_SAND, OnToolsTerrainSand)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_TERRAIN_SAND, OnUpdateToolsTerrainSand)
    ON_COMMAND(ID_TOOLS_TERRAIN_SNOW, OnToolsTerrainSnow)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_TERRAIN_SNOW, OnUpdateToolsTerrainSnow)
    ON_COMMAND(ID_TOOLS_TERRAIN_SUBTERRANEAN, OnToolsTerrainSubterranean)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_TERRAIN_SUBTERRANEAN, OnUpdateToolsTerrainSubterranean)
    ON_COMMAND(ID_TOOLS_TERRAIN_SWAMP, OnToolsTerrainSwamp)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_TERRAIN_SWAMP, OnUpdateToolsTerrainSwamp)
    ON_COMMAND(ID_TOOLS_TERRAIN_WATER, OnToolsTerrainWater)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_TERRAIN_WATER, OnUpdateToolsTerrainWater)
    ON_COMMAND(ID_VIEW_UNDERGROUND, OnViewUnderground)
    ON_UPDATE_COMMAND_UI(ID_VIEW_UNDERGROUND, OnUpdateViewUnderground)
    ON_COMMAND(ID_PLAYERS_NONE, OnPlayersNone)
    ON_UPDATE_COMMAND_UI(ID_PLAYERS_NONE, OnUpdatePlayersNone)
    ON_COMMAND(ID_PLAYERS_PLAYER1, OnPlayersPlayer1)
    ON_UPDATE_COMMAND_UI(ID_PLAYERS_PLAYER1, OnUpdatePlayersPlayer1)
    ON_COMMAND(ID_PLAYERS_PLAYER2, OnPlayersPlayer2)
    ON_UPDATE_COMMAND_UI(ID_PLAYERS_PLAYER2, OnUpdatePlayersPlayer2)
    ON_COMMAND(ID_PLAYERS_PLAYER3, OnPlayersPlayer3)
    ON_UPDATE_COMMAND_UI(ID_PLAYERS_PLAYER3, OnUpdatePlayersPlayer3)
    ON_COMMAND(ID_PLAYERS_PLAYER4, OnPlayersPlayer4)
    ON_UPDATE_COMMAND_UI(ID_PLAYERS_PLAYER4, OnUpdatePlayersPlayer4)
    ON_COMMAND(ID_PLAYERS_PLAYER5, OnPlayersPlayer5)
    ON_UPDATE_COMMAND_UI(ID_PLAYERS_PLAYER5, OnUpdatePlayersPlayer5)
    ON_COMMAND(ID_PLAYERS_PLAYER6, OnPlayersPlayer6)
    ON_UPDATE_COMMAND_UI(ID_PLAYERS_PLAYER6, OnUpdatePlayersPlayer6)
    ON_COMMAND(ID_PLAYERS_PLAYER7, OnPlayersPlayer7)
    ON_UPDATE_COMMAND_UI(ID_PLAYERS_PLAYER7, OnUpdatePlayersPlayer7)
    ON_COMMAND(ID_PLAYERS_PLAYER8, OnPlayersPlayer8)
    ON_UPDATE_COMMAND_UI(ID_PLAYERS_PLAYER8, OnUpdatePlayersPlayer8)
    ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
    ON_COMMAND(ID_TOOLS_TERRAIN_ROCK, OnToolsTerrainRock)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_TERRAIN_ROCK, OnUpdateToolsTerrainRock)
    ON_COMMAND(ID_TOOLS_OBJECTS_ALL_TERRAIN, OnToolsObjectsAllTerrain)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBJECTS_ALL_TERRAIN, OnUpdateToolsObjectsAllTerrain)
    ON_WM_SETFOCUS()
    ON_COMMAND(ID_TOOLS_BRUSH_1X1, OnToolsBrush1x1)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_BRUSH_1X1, OnUpdateToolsBrush1x1)
    ON_COMMAND(ID_TOOLS_BRUSH_2X2, OnToolsBrush2x2)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_BRUSH_2X2, OnUpdateToolsBrush2x2)
    ON_COMMAND(ID_TOOLS_BRUSH_4X4, OnToolsBrush4x4)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_BRUSH_4X4, OnUpdateToolsBrush4x4)
    ON_COMMAND(ID_TOOLS_BRUSH_FILL, OnToolsBrushFill)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_BRUSH_FILL, OnUpdateToolsBrushFill)
    ON_COMMAND(ID_TOOLS_VALIDATE_MAP, OnToolsValidateMap)
    ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
    ON_COMMAND(ID_EDIT_FIND, OnEditFind)
    ON_COMMAND(ID_EDIT_FIND_NEXT, OnEditFindNext)
    ON_UPDATE_COMMAND_UI(ID_EDIT_FIND_NEXT, OnUpdateEditFindNext)
    ON_COMMAND(ID_EDIT_FIND_PREV, OnEditFindPrev)
    ON_UPDATE_COMMAND_UI(ID_EDIT_FIND_PREV, OnUpdateEditFindNext)
    ON_UPDATE_COMMAND_UI(ID_EDIT_FIND, OnUpdateEditFind)
    ON_COMMAND(ID_TOOLS_OBSTACLES, OnToolsObstacles)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBSTACLES, OnUpdateToolsObstacles)
    ON_COMMAND(ID_TOOLS_OBSTACLES_ERASE_1X1, OnToolsObstaclesErase1x1)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBSTACLES_ERASE_1X1, OnUpdateToolsObstaclesErase1x1)
    ON_COMMAND(ID_TOOLS_OBSTACLES_ERASE_2X2, OnToolsObstaclesErase2x2)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBSTACLES_ERASE_2X2, OnUpdateToolsObstaclesErase2x2)
    ON_COMMAND(ID_TOOLS_OBSTACLES_ERASE_4X4, OnToolsObstaclesErase4x4)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBSTACLES_ERASE_4X4, OnUpdateToolsObstaclesErase4x4)
    ON_COMMAND(ID_TOOLS_OBSTACLES_ERASE_FILL, OnToolsObstaclesEraseFill)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBSTACLES_ERASE_FILL, OnUpdateToolsObstaclesEraseFill)
    ON_COMMAND(ID_TOOLS_OBSTACLES_BORDERED_AREA_1X1, OnToolsObstaclesBorderedArea1x1)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBSTACLES_BORDERED_AREA_1X1, OnUpdateToolsObstaclesBorderedArea1x1)
    ON_COMMAND(ID_TOOLS_OBSTACLES_BORDERED_AREA_2X2, OnToolsObstaclesBorderedArea2x2)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBSTACLES_BORDERED_AREA_2X2, OnUpdateToolsObstaclesBorderedArea2x2)
    ON_COMMAND(ID_TOOLS_OBSTACLES_BORDERED_AREA_4X4, OnToolsObstaclesBorderedArea4x4)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBSTACLES_BORDERED_AREA_4X4, OnUpdateToolsObstaclesBorderedArea4x4)
    ON_COMMAND(ID_TOOLS_OBSTACLES_BORDERED_AREA_FILL, OnToolsObstaclesBorderedAreaFill)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBSTACLES_BORDERED_AREA_FILL, OnUpdateToolsObstaclesBorderedAreaFill)
    ON_COMMAND(ID_TOOLS_OBSTACLES_AREA_1X1, OnToolsObstaclesArea1x1)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBSTACLES_AREA_1X1, OnUpdateToolsObstaclesArea1x1)
    ON_COMMAND(ID_TOOLS_OBSTACLES_AREA_2X2, OnToolsObstaclesArea2x2)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBSTACLES_AREA_2X2, OnUpdateToolsObstaclesArea2x2)
    ON_COMMAND(ID_TOOLS_OBSTACLES_AREA_4X4, OnToolsObstaclesArea4x4)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBSTACLES_AREA_4X4, OnUpdateToolsObstaclesArea4x4)
    ON_COMMAND(ID_TOOLS_OBSTACLES_AREA_FILL, OnToolsObstaclesAreaFill)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBSTACLES_AREA_FILL, OnUpdateToolsObstaclesAreaFill)
    ON_COMMAND(ID_TOOLS_OBSTACLES_PLACE, OnToolsObstaclesPlace)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBSTACLES_PLACE, OnUpdateToolsObstaclesPlace)
    ON_COMMAND(ID_TOOLS_OBSTACLES_BORDERED_ERASE_1X1, OnToolsObstaclesBorderedErase1x1)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBSTACLES_BORDERED_ERASE_1X1, OnUpdateToolsObstaclesBorderedErase1x1)
    ON_COMMAND(ID_TOOLS_OBSTACLES_BORDERED_ERASE_2X2, OnToolsObstaclesBorderedErase2x2)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBSTACLES_BORDERED_ERASE_2X2, OnUpdateToolsObstaclesBorderedErase2x2)
    ON_COMMAND(ID_TOOLS_OBSTACLES_BORDERED_ERASE_4X4, OnToolsObstaclesBorderedErase4x4)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBSTACLES_BORDERED_ERASE_4X4, OnUpdateToolsObstaclesBorderedErase4x4)
    ON_COMMAND(ID_TOOLS_OBSTACLES_BORDERED_ERASE_FILL, OnToolsObstaclesBorderedEraseFill)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_OBSTACLES_BORDERED_ERASE_FILL, OnUpdateToolsObstaclesBorderedEraseFill)
    ON_COMMAND(ID_TOOLS_RIVERS_ERASE, OnToolsRiversErase)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_RIVERS_ERASE, OnUpdateToolsRiversErase)
    ON_COMMAND(ID_TOOLS_ROADS_ERASE, OnToolsRoadsErase)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_ROADS_ERASE, OnUpdateToolsRoadsErase)
END_MESSAGE_MAP()

VA(0x004804d0, 0x12b)
TMapView::TMapView()
    : _m_pStatusUI(NULL),
      _m_pMapFrameWnd(NULL),
      _m_pMiniMapWnd(NULL),
      _m_pToolkitWnd(NULL),
      _m_revision(0),
      _m_obstacleMaskHistoryPos(0),
      _m_viewPos(0, 0),
      _m_bViewUnderground(false),
      _m_mode(_eModeStartup),
      _m_objectSlot(eSlotDirt),
      _m_brush(_eBrush2x2),
      _m_terrainType(eTerrainDirt),
      _m_riverType(1),
      _m_roadType(1),
      _m_eraseBrush(_eBrush2x2),
      _m_obstacleBrush(_eObstacleBrushBorderedArea2x2),
      _m_currentPlayer(ePlayerNone),
      _m_pFloatingObj(NULL),
      _m_floatingObjX(0),
      _m_floatingObjY(0),
      _m_lastFindType(-1)
{
    allMapViews.insert(this);
}

VA_COMPGEN(0x004805fb, 0x1c, SCALAR_DELETING_DTOR, TMapView)

VA(0x0048061c, 0x90)
TMapView::~TMapView()
{
    allMapViews.erase(this);
}

VA(0x004806ac, 0x77)
BOOL TMapView::PreCreateWindow(CREATESTRUCT& cs)
{
    DATA_COMPGEN_GUARD(0x005a1e7c, mapViewClassNameGuard, className)
    VA_COMPGEN(0x00480723, 0xa, STATIC_DTOR, className)
    DATA(0x005a1e78) static CString className;
    if (className.IsEmpty())
        className = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW));
    cs.lpszClass = className;
    cs.style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN;
    return CView::PreCreateWindow(cs);
}

VA(0x0048072d, 0x3a)
void TMapView::animate(unsigned int frameNum, bool bForce)
{
    for (std::set<TMapView*>::const_iterator it = allMapViews.begin(); it != allMapViews.end(); ++it)
        (*it)->_m_pMapFrameWnd->animate(frameNum, bForce);
}

VA(0x00480767, 0x2f)
void TMapView::resetAnimation()
{
    std::for_each(allMapViews.begin(), allMapViews.end(), std::mem_fun(&TMapView::_resetAnimation));
}

VA(0x00480796, 0x48)
void TMapView::setZoom(TZoom zoom)
{
    std::for_each(allMapViews.begin(), allMapViews.end(), std::bind2nd(std::mem_fun1(&TMapView::_setZoom), zoom));
    _s_zoom = zoom;
}

VA(0x004807de, 0x48)
void TMapView::setViewGrid(bool bViewGrid)
{
    std::for_each(allMapViews.begin(), allMapViews.end(),
                  std::bind2nd(std::mem_fun1(&TMapView::_setViewGrid), bViewGrid));
    _s_bViewGrid = bViewGrid;
}

VA(0x00480826, 0x48)
void TMapView::setViewPassability(bool bViewPassability)
{
    std::for_each(allMapViews.begin(), allMapViews.end(),
                  std::bind2nd(std::mem_fun1(&TMapView::_setViewPassability), bViewPassability));
    _s_bViewPassability = bViewPassability;
}

VA(0x0048086e, 0xa)
void TMapView::setStatusUI(TStatusUI* pStatusUI)
{
    _m_pStatusUI = pStatusUI;
}

VA(0x00480878, 0x4)
TMapDoc* TMapView::getPDocument()
{
    return (TMapDoc*)m_pDocument;
}

VA(0x0048087c, 0x5a)
bool TMapView::onToolkitCanCreateObject(TToolkitWnd* pToolkitWnd, const TObjectType& objType)
{
    if (!g_gameContextFeatures[g_videoGameState].test(GAME_VERSION_AB)
        && (objType.getType() == HERO && THero::s_akClassTraits[objType.getExtra()].m_townType == TOWN_CONFLUX
            || objType.getType() == TOWN && objType.getExtra() == TOWN_CONFLUX))
        return false;
    return getPDocument()->getPMap()->canCreate(objType, _m_currentPlayer);
}

VA(0x004808d6, 0x244)
void TMapView::onToolkitGrabObject(TToolkitWnd* pToolkitWnd, const TObjectType& objType)
{
    if (!g_gameContextFeatures[g_videoGameState].test(GAME_VERSION_AB)
        && (objType.getType() == HERO && THero::s_akClassTraits[objType.getExtra()].m_townType == TOWN_CONFLUX
            || objType.getType() == TOWN && objType.getExtra() == TOWN_CONFLUX)) {
        MessageBeep(-1);
        MessageBox(kObjectNeedsABInstalledStr);
        return;
    }
    try {
        _m_pNewFloatingObj = getPDocument()->getPMap()->createObject(objType, _m_currentPlayer);
    } catch (const TCreateObjFailureTooManyInstancesOfTypeOnMap& e) {
        MessageBeep(-1);
        MessageBox(TFormattedString(kMaxObjectsOfTypeFmtStr, e.getCap(), akAdvObjectTypeTraits[e.getType()].m_name));
        return;
    } catch (const TCreateObjFailureNoAvailableHeroesInClass&) {
        MessageBeep(-1);
        MessageBox(kNoMoreClassHeroesStr);
        return;
    } catch (const TCreateObjFailureNoOwnerForHero&) {
        MessageBeep(-1);
        MessageBox(kSelectPlayerBeforePlacingHeroStr);
        return;
    } catch (const TCreateObjFailureTooManyHeroesForPlayer&) {
        MessageBeep(-1);
        MessageBox(TFormattedString(kMaxHeroesPerPlayerFmtStr, 8));
        return;
    } catch (const TCreateObjFailureHolyGrailAlreadyPlaced&) {
        MessageBeep(-1);
        MessageBox(kGrailAlreadyPlacedStr);
        return;
    } catch (const TCreateObjFailureNotSupportedByReleaseVersion& e) {
        MessageBeep(-1);
        if (e.getRequiredVersion() == GAME_VERSION_AB)
            MessageBox(kObjectNeedsABOrSoDStr);
        else
            MessageBox(kObjectNeedsSoDStr);
        return;
    } catch (const TCreateObjectFailure& e) {
        MessageBeep(-1);
        MessageBox(e.what());
        return;
    }
    _m_pFloatingObj = dynamic_cast<const TGUIGameObject*>(_m_pNewFloatingObj.get());
    _m_pMapFrameWnd->grabObject(_m_pFloatingObj);
}

VA(0x00480b1a, 0x32)
void TMapView::onMoveMapViewRect(TMapViewingWnd* pWnd, const CPoint& pos)
{
    _m_viewPos = pos;
    if (_m_pMapFrameWnd != NULL)
        _m_pMapFrameWnd->moveViewRect(pos);
    if (_m_pMiniMapWnd != NULL)
        _m_pMiniMapWnd->moveViewRect(pos);
}

VA(0x00480b4c, 0x13)
void TMapView::onSizeMapViewRect(TMapViewingWnd* pWnd, const CSize& size)
{
    if (_m_pMiniMapWnd != NULL)
        _m_pMiniMapWnd->sizeViewRect(size);
}

// /OPT:ICF folds the two fill-rectangle notifications onto this empty body,
// with the map document's repaint operation's and the map specifications'
// adapters' empty slots.
VA(0x00480b5f, 0x3)
void TMapView::onEditCursorTilePosChanged(TMapEditingWnd* pWnd, const CPoint& pos)
{
}

VA(0x00480b62, 0x8d)
void TMapView::onEditObjectSelected(TMapEditingWnd* pWnd, unsigned int objID)
{
    if (objID != TGameMap::TLayer::s_kInvalidObjID) {
        const TGameMap* pMap = getPDocument()->getPMap();
        const TGameObject& object = pMap->getLayer(_m_bViewUnderground).getObject(objID);
        _m_pStatusUI->setObjectName(object.getTypeName().c_str());
    } else
        _m_pStatusUI->setObjectName("");
}

VA(0x00480bef, 0xbb)
void TMapView::onEditEditObject(TMapEditingWnd* pWnd, unsigned int objID)
{
    TGameMap map(*getPDocument()->getPMap());
    TGameMap::TLayer& layer = map.getLayer(_m_bViewUnderground);
    TGUIGameObject* pObject = dynamic_cast<TGUIGameObject*>(layer.getPObject(objID));
    if (pObject->edit(this, &map, _m_bViewUnderground, objID)) {
        getPDocument()->backupMap();
        *getPDocument()->getPMap() = map;
        getPDocument()->onObjectPropertiesChanged(_m_bViewUnderground, objID);
    }
}

VA(0x00480caa, 0x1f)
void TMapView::onEditDeleteObject(TMapEditingWnd* pWnd, unsigned int objID)
{
    getPDocument()->backupMap();
    getPDocument()->removeObject(_m_bViewUnderground, objID);
}

VA(0x00480cc9, 0x8e)
void TMapView::onEditGrabObject(TMapEditingWnd* pWnd, unsigned int objID)
{
    getPDocument()->backupMap();
    const TGameMap::TLayer& layer = getPDocument()->getPMap()->getLayer(_m_bViewUnderground);
    const TGameObject* pObject = layer.getPObject(objID);
    _m_pFloatingObj = dynamic_cast<const TGUIGameObject*>(pObject);
    TTilePoint loc = layer.getObjectLoc(objID);
    _m_floatingObjX = loc.x();
    _m_floatingObjY = loc.y();
    getPDocument()->floatObject(_m_bViewUnderground, objID);
    _m_pMapFrameWnd->grabObject(_m_pFloatingObj);
}

VA(0x00480d57, 0x9c)
void TMapView::onEditCopyObject(TMapEditingWnd* pWnd, unsigned int objID)
{
    const TGameMap* pMap = getPDocument()->getPMap();
    _m_pNewFloatingObj = pMap->getLayer(_m_bViewUnderground).getObject(objID).clone();
    if (_m_pNewFloatingObj.get() == NULL)
        throw TAllocationFailure();
    _m_pFloatingObj = dynamic_cast<const TGUIGameObject*>(_m_pNewFloatingObj.get());
    _m_pMapFrameWnd->grabObject(_m_pFloatingObj);
}

VA(0x00480df3, 0x1f5)
void TMapView::onEditPlaceObject(TMapEditingWnd* pWnd, const TGUIGameObject& obj, unsigned int x, unsigned int y)
{
    if (_m_pNewFloatingObj.get() == NULL) {
        try {
            getPDocument()->unfloatObject(_m_bViewUnderground, x, y);
        } catch (const TPlaceObjFailurePlacementNotOnMap&) {
            getPDocument()->getPMap()->removeFloatingObject(_m_bViewUnderground);
        } catch (const TPlaceObjectFailure& e) {
            if (dynamic_cast<const TPlaceObjFailureHolyGrailTooCloseToEdge*>(&e) != NULL) {
                CString msg;
                msg.Format(kGrailPlacedTooCloseToEdgeFmtStr, 9);
                MessageBeep(-1);
                MessageBox(msg);
            } else if (dynamic_cast<const TPlaceObjFailureInvalidPlacement*>(&e) != NULL)
                MessageBeep(-1);
            try {
                getPDocument()->unfloatObject(_m_bViewUnderground, _m_floatingObjX, _m_floatingObjY);
            } catch (const TPlaceObjectFailure&) {
                getPDocument()->getPMap()->removeFloatingObject(_m_bViewUnderground);
            }
        }
    } else {
        getPDocument()->backupMap();
        try {
            _placeObjectWithExceptionHandlingImpl(_m_pNewFloatingObj, x, y);
        } catch (const TPlaceObjFailurePlacementNotOnMap&) {
        } catch (const TPlaceObjFailureHolyGrailTooCloseToEdge&) {
            CString msg;
            msg.Format(kGrailPlacedTooCloseToEdgeFmtStr, 9);
            MessageBeep(-1);
            MessageBox(msg);
        } catch (const TPlaceObjFailureInvalidPlacement&) {
            MessageBeep(-1);
        }
        _m_pNewFloatingObj = std::auto_ptr<TGameObject>();
    }
    _m_pFloatingObj = NULL;
}

VA(0x00480fe8, 0x4a)
void TMapView::onEditDiscardObject(TMapEditingWnd* pWnd, const TGUIGameObject& obj)
{
    if (_m_pNewFloatingObj.get() == NULL)
        getPDocument()->getPMap()->removeFloatingObject(_m_bViewUnderground);
    else
        _m_pNewFloatingObj = std::auto_ptr<TGameObject>();
    _m_pFloatingObj = NULL;
}

VA(0x00481032, 0x164)
void TMapView::onEditPasteObject(TMapEditingWnd* pWnd, const TGameObject& obj, const CRect& rect)
{
    getPDocument()->backupMap();
    unsigned int minX = rect.left + obj.getWidth() - 1;
    unsigned int minY = rect.top + obj.getHeight() - 1;
    unsigned int maxX = minX + rect.Width() - 1;
    unsigned int maxY = minY + rect.Height() - 1;
    unsigned int x = minX;
    unsigned int y = minY;
    try {
        std::auto_ptr<TGameObject> pClone = obj.clone();
        _placeObjectWithExceptionHandlingImpl(pClone, x, y);
        return;
    } catch (const TPlaceObjFailureInvalidPlacement&) {
    }
    for (;;) {
        if (y + 1 < maxY && x > minX) {
            ++y;
            --x;
        } else {
            x = x + y - minY + 1;
            if (x < maxX)
                y = minY;
            else {
                y = minY + x - (maxX - 1);
                if (y >= maxY)
                    break;
                x = maxX - 1;
            }
        }
        try {
            std::auto_ptr<TGameObject> pClone = obj.clone();
            _placeObjectImpl(pClone, x, y);
            return;
        } catch (const TPlaceObjFailureInvalidPlacement&) {
        }
    }
    MessageBeep(-1);
    MessageBox(kNowhereToPasteStr);
}

VA(0x00481196, 0x116)
void TMapView::onEditBrushBeginDrag(TMapEditingWnd* pWnd, const CRect& rect)
{
    TMapDoc* pDoc = getPDocument();
    switch (_m_mode) {
    case _eModeTerrain:
        pDoc->startTerrainPlacementOp(_m_bViewUnderground, _m_terrainType);
        pDoc->terrainFill(rect.left, rect.top, rect.Width(), rect.Height());
        break;
    case _eModeRiver:
        if (_m_riverType != 0)
            pDoc->startRiverPlacementOp(_m_bViewUnderground, _m_riverType, rect.left, rect.top);
        else {
            pDoc->startRiverEraseOp(_m_bViewUnderground);
            pDoc->eraseRiver(rect.left, rect.top);
        }
        break;
    case _eModeRoad:
        if (_m_roadType != 0)
            pDoc->startRoadPlacementOp(_m_bViewUnderground, _m_roadType, rect.left, rect.top);
        else {
            pDoc->startRoadEraseOp(_m_bViewUnderground);
            pDoc->eraseRoad(rect.left, rect.top);
        }
        break;
    case _eModeErase:
        pDoc->startEraseOp(_m_bViewUnderground);
        pDoc->erase(rect.left, rect.top, rect.Width(), rect.Height());
        break;
    case _eModeObstacles:
        _backupObstacleMask();
        onEditBrushDrag(pWnd, rect);
        break;
    }
}

VA(0x004812ac, 0x8d)
void TMapView::onEditBrushEndDrag(TMapEditingWnd* pWnd)
{
    switch (_m_mode) {
    case _eModeTerrain: {
        CWaitCursor wait;
        getPDocument()->endTerrainPlacementOp();
    }
        break;
    case _eModeRiver:
        if (_m_riverType != 0)
            getPDocument()->endRiverPlacementOp();
        else
            getPDocument()->endRiverEraseOp();
        break;
    case _eModeRoad:
        if (_m_roadType != 0)
            getPDocument()->endRoadPlacementOp();
        else
            getPDocument()->endRoadEraseOp();
        break;
    case _eModeErase:
        getPDocument()->endEraseOp();
        break;
    }
}

VA(0x00481339, 0x129)
void TMapView::onEditBrushDrag(TMapEditingWnd* pWnd, const CRect& rect)
{
    switch (_m_mode) {
    case _eModeTerrain:
        getPDocument()->terrainFill(rect.left, rect.top, rect.Width(), rect.Height());
        break;
    case _eModeRiver:
        if (_m_riverType != 0)
            getPDocument()->placeRiver(rect.left, rect.top);
        else
            getPDocument()->eraseRiver(rect.left, rect.top);
        break;
    case _eModeRoad:
        if (_m_roadType != 0)
            getPDocument()->placeRoad(rect.left, rect.top);
        else
            getPDocument()->eraseRoad(rect.left, rect.top);
        break;
    case _eModeErase:
        getPDocument()->erase(rect.left, rect.top, rect.Width(), rect.Height());
        break;
    case _eModeObstacles:
        switch (_m_obstacleBrush) {
        case _eObstacleBrushArea1x1:
        case _eObstacleBrushArea2x2:
        case _eObstacleBrushArea4x4:
            _paintObstacleArea(rect);
            break;
        case _eObstacleBrushBorderedArea1x1:
        case _eObstacleBrushBorderedArea2x2:
        case _eObstacleBrushBorderedArea4x4:
            _paintBorderedObstacleArea(rect);
            break;
        case _eObstacleBrushErase1x1:
        case _eObstacleBrushErase2x2:
        case _eObstacleBrushErase4x4:
            _eraseObstacleArea(rect);
            break;
        case _eObstacleBrushBorderedErase1x1:
        case _eObstacleBrushBorderedErase2x2:
        case _eObstacleBrushBorderedErase4x4:
            _eraseBorderedObstacleArea(rect);
            break;
        }
        break;
    }
}

void TMapView::onEditFillRectAnchor(TMapEditingWnd* pWnd, const CRect& rect)
{
}

void TMapView::onEditFillRectDrag(TMapEditingWnd* pWnd, const CRect& rect)
{
}

VA(0x00481462, 0x14a)
void TMapView::onEditFillRectEndDrag(TMapEditingWnd* pWnd, const CRect& rect)
{
    switch (_m_mode) {
    case _eModeTerrain: {
        CWaitCursor wait;
        getPDocument()->startTerrainPlacementOp(_m_bViewUnderground, _m_terrainType);
        getPDocument()->terrainFill(rect.left, rect.top, rect.Width(), rect.Height());
        getPDocument()->endTerrainPlacementOp();
    }
        break;
    case _eModeErase: {
        CWaitCursor wait;
        getPDocument()->startEraseOp(_m_bViewUnderground);
        getPDocument()->erase(rect.left, rect.top, rect.Width(), rect.Height());
        getPDocument()->endEraseOp();
    }
        break;
    case _eModeObstacles: {
        CWaitCursor wait;
        _backupObstacleMask();
        switch (_m_obstacleBrush) {
        case _eObstacleBrushAreaFill:
            _paintObstacleArea(rect);
            break;
        case _eObstacleBrushBorderedAreaFill:
            _paintBorderedObstacleArea(rect);
            break;
        case _eObstacleBrushEraseFill:
            _eraseObstacleArea(rect);
            break;
        case _eObstacleBrushBorderedEraseFill:
            _eraseBorderedObstacleArea(rect);
            break;
        }
    }
        break;
    }
}

VA(0x004815ac, 0x53)
bool TMapView::onEditProperties(TGameObject* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    TObjectPropsDlg dlg(this, pObj);
    dlg.DoModal();
    return false;
}

VA(0x004815ff, 0x5e)
bool TMapView::onEditProperties(THeroPlaceholder* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    THeroPlaceholderPropsDlg dlg(this, pMap, bSecondLayer, objID);
    dlg.DoModal();
    return dlg.wasModified();
}

VA(0x0048165d, 0x67)
bool TMapView::onEditProperties(TNonRandomHero* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    TNonRandomHeroPropsSheet sheet(this, pMap, bSecondLayer, objID, true);
    sheet.DoModal();
    return sheet.wasModified();
}

VA(0x004816c4, 0x67)
bool TMapView::onEditProperties(TRandomHero* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    TRandomHeroPropsSheet sheet(this, pMap, bSecondLayer, objID, true);
    sheet.DoModal();
    return sheet.wasModified();
}

VA(0x0048172b, 0x65)
bool TMapView::onEditProperties(TPrison* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    TPrisonPropsSheet sheet(this, pMap, bSecondLayer, objID);
    sheet.DoModal();
    return sheet.wasModified();
}

VA(0x00481790, 0x65)
bool TMapView::onEditProperties(TTown* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    TTownPropsSheet sheet(this, pMap, bSecondLayer, objID);
    sheet.DoModal();
    return sheet.wasModified();
}

VA(0x004817f5, 0x96)
bool TMapView::onEditProperties(TEvent* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    TPlayerMask availablePlayers;
    for (unsigned int player = 0; player < kNumPlayers; ++player)
        availablePlayers[player] = pMap->isPlayerPresent(TPlayer(player));
    TEventPropsSheet sheet(this, pObj, availablePlayers, pMap->getVersion());
    sheet.DoModal();
    return sheet.wasModified();
}

VA(0x0048188b, 0x6c)
bool TMapView::onEditProperties(TMonster* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    TMonsterPropsSheet sheet(this, pObj, pMap->getVersion());
    sheet.DoModal();
    return sheet.wasModified();
}

VA(0x004818f7, 0x87)
bool TMapView::onEditProperties(TFlaggableObject* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    if ((pObj->getType() == MINE || pObj->getType() == ABANDONED_MINE) && pObj->getExtra() >= 7)
        return onEditProperties(static_cast<TGameObject*>(pObj), pMap, bSecondLayer, objID);
    TFlaggablePropsDlg dlg(this, pObj);
    dlg.DoModal();
    return dlg.wasModified();
}

VA(0x0048197e, 0x58)
bool TMapView::onEditProperties(TAbandonedMine* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    TAbandonedMinePropsDlg dlg(this, pObj);
    dlg.DoModal();
    return dlg.wasModified();
}

VA(0x004819d6, 0x62)
bool TMapView::onEditProperties(TGarrison* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    TGarrisonPropsDlg dlg(this, pObj, pMap->getVersion());
    dlg.DoModal();
    return dlg.wasModified();
}

VA(0x00481a38, 0x55)
bool TMapView::onEditProperties(TSign* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    TSignPropsDlg dlg(this, pObj);
    dlg.DoModal();
    return dlg.wasModified();
}

VA(0x00481ad4, 0x6c)
bool TMapView::onEditProperties(TGameArtifact* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    TArtifactPropsSheet sheet(this, pObj, pMap->getVersion());
    sheet.DoModal();
    return sheet.wasModified();
}

VA(0x00481b40, 0x6c)
bool TMapView::onEditProperties(TSpellScroll* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    TSpellScrollPropsSheet sheet(this, pObj, pMap->getVersion());
    sheet.DoModal();
    return sheet.wasModified();
}

VA(0x00481bac, 0x6c)
bool TMapView::onEditProperties(TGameResource* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    TResourcePropsSheet sheet(this, pObj, pMap->getVersion());
    sheet.DoModal();
    return sheet.wasModified();
}

VA(0x00481c18, 0x6c)
bool TMapView::onEditProperties(TBlackBox* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    TBlackBoxPropsSheet sheet(this, pObj, pMap->getVersion());
    sheet.DoModal();
    return sheet.wasModified();
}

VA(0x00481c84, 0x55)
bool TMapView::onEditProperties(TScholar* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    TScholarPropsDlg dlg(this, pObj);
    dlg.DoModal();
    return dlg.wasModified();
}

VA(0x00481d8d, 0x55)
bool TMapView::onEditProperties(THolyGrail* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    THolyGrailPropsDlg dlg(this, pObj);
    dlg.DoModal();
    return dlg.wasModified();
}

VA(0x00481de2, 0x55)
bool TMapView::onEditProperties(TShrine* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    TShrinePropsDlg dlg(this, pObj);
    dlg.DoModal();
    return dlg.wasModified();
}

VA(0x00481fd5, 0x71)
bool TMapView::onEditProperties(TWitchHut* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
{
    TWitchHutPropsDlg dlg(this, pObj, pMap->getVersion());
    dlg.DoModal();
    return dlg.wasModified();
}

VA(0x0048207b, 0x3b)
void TMapView::_deleteAll()
{
    delete _m_pToolkitWnd;
    _m_pToolkitWnd = NULL;
    delete _m_pMiniMapWnd;
    _m_pMiniMapWnd = NULL;
    delete _m_pMapFrameWnd;
    _m_pMapFrameWnd = NULL;
}

VA(0x004820b6, 0x6c)
void TMapView::_placeObjectImpl(std::auto_ptr<TGameObject> pObj, unsigned int x, unsigned int y)
{
    TMapLayerObjectID objID = getPDocument()->placeObject(_m_bViewUnderground, pObj, TTilePoint(x, y));
    _m_pMapFrameWnd->selectObject(objID);
}

VA(0x00482122, 0x165)
void TMapView::_placeObjectWithExceptionHandlingImpl(std::auto_ptr<TGameObject> pObj, unsigned int x,
                                                     unsigned int y)
{
    try {
        _placeObjectImpl(pObj, x, y);
    } catch (const TPlaceObjFailureTooManyInstancesOfTypeOnMap& e) {
        CString msg;
        msg.Format(kMaxObjectsOfTypeFmtStr, e.getCap(), akAdvObjectTypeTraits[e.getType()].m_name);
        MessageBeep(-1);
        MessageBox(msg);
    } catch (const TPlaceObjFailureNoAvailableHeroesInClass&) {
        MessageBeep(-1);
        MessageBox(kNoMoreClassHeroesStr);
    } catch (const TPlaceObjFailureTooManyHeroesForPlayer&) {
        CString msg;
        msg.Format(kMaxHeroesPerPlayerFmtStr, 8);
        MessageBeep(-1);
        MessageBox(msg);
    } catch (const TPlaceObjFailureHolyGrailAlreadyPlaced&) {
        MessageBeep(-1);
        MessageBox(kGrailAlreadyPlacedStr);
    } catch (const TPlaceObjFailureNotSupportedByReleaseVersion& e) {
        MessageBeep(-1);
        MessageBox(kObjectNeedsABOrSoDStr);
    }
}

VA(0x00482287, 0x8a)
void TMapView::_layout(int cx, int cy)
{
    CRect miniMapRect;
    _m_pMiniMapWnd->GetWindowRect(&miniMapRect);
    CSize miniMapSize = miniMapRect.Size();
    CSize toolkitSize = _m_pToolkitWnd->getMinSize();
    int paneWidth = max(miniMapSize.cx, toolkitSize.cx);
    _m_pMiniMapWnd->MoveWindow(cx - paneWidth, 0, 0, 0);
    _m_pMapFrameWnd->MoveWindow(0, 0, cx - paneWidth - 1, cy);
    _m_pToolkitWnd->MoveWindow(cx - paneWidth, miniMapSize.cy + 1, paneWidth, cy - miniMapSize.cy - 1);
}

VA(0x00482311, 0xb)
int TMapView::_resetAnimation()
{
    _m_pMapFrameWnd->resetAnimation();
    return 0;
}

VA(0x0048231c, 0x11)
int TMapView::_setZoom(TZoom zoom)
{
    _m_pMapFrameWnd->setZoom(zoom);
    return 0;
}

VA(0x0048232d, 0x11)
int TMapView::_setViewGrid(bool bViewGrid)
{
    _m_pMapFrameWnd->showGrid(bViewGrid);
    return 0;
}

VA(0x0048233e, 0x11)
int TMapView::_setViewPassability(bool bViewPassability)
{
    _m_pMapFrameWnd->showPassability(bViewPassability);
    return 0;
}

VA(0x0048234f, 0x99)
void TMapView::_setViewUnderground(bool bViewUnderground)
{
    const TGameMap* pMap = getPDocument()->getPMap();
    _m_bViewUnderground = bViewUnderground;
    _m_pMapFrameWnd->MoveWindow(0, 0, 0, 0);
    _m_pMapFrameWnd->setMapLayer(pMap, _m_pObstacleMask.get(), _m_bViewUnderground);
    _m_pMapFrameWnd->moveViewRect(_m_viewPos);
    _m_pMiniMapWnd->setMapLayer(pMap, _m_pObstacleMask.get(), _m_bViewUnderground);
    _m_pMiniMapWnd->moveViewRect(_m_viewPos);
    CRect rect;
    GetClientRect(&rect);
    if (rect.Width() > 0 && rect.Height() > 0)
        _layout(rect.Width(), rect.Height());
}

VA(0x004823e8, 0x184)
void TMapView::_setMode(_TMode newMode)
{
    if (newMode == _m_mode)
        return;
    switch (_m_mode) {
    case _eModeObjects:
        _m_pStatusUI->hideObjectName();
        break;
    case _eModeObstacles: {
        _m_obstacleMaskHistory.clear();
        _m_obstacleMaskHistoryPos = 0;
        _m_obstacleMaskHistories.clear();
        bool bHadTiles = _m_pObstacleMask->hasTiles(_m_bViewUnderground);
        _m_pObstacleMask->clear();
        if (bHadTiles) {
            const TGameMap* pMap = getPDocument()->getPMap();
            _m_pMapFrameWnd->update(CRect(0, 0, pMap->getWidth(), pMap->getHeight()));
            _m_pMiniMapWnd->update(CRect(0, 0, pMap->getWidth(), pMap->getHeight()));
        }
    }
        break;
    }
    _m_mode = newMode;
    switch (_m_mode) {
    case _eModeTerrain:
        _m_pToolkitWnd->showTerrainToolkit();
        _realizeBrush(_m_brush);
        break;
    case _eModeRiver:
        _m_pToolkitWnd->showRiverToolkit();
        _m_pMapFrameWnd->brushMode(CSize(1, 1));
        break;
    case _eModeRoad:
        _m_pToolkitWnd->showRoadToolkit();
        _m_pMapFrameWnd->brushMode(CSize(1, 1));
        break;
    case _eModeErase:
        _m_pToolkitWnd->showEraseToolkit();
        _realizeBrush(_m_eraseBrush);
        break;
    case _eModeObjects:
        _m_pStatusUI->showObjectName();
        _m_pToolkitWnd->showObjectPalette(_m_objectSlot);
        _m_pMapFrameWnd->selectionMode();
        break;
    case _eModeObstacles:
        _m_pToolkitWnd->showObstacleToolkit();
        _realizeObstacleBrush();
        break;
    }
}

VA(0x0048256c, 0x28)
void TMapView::_setCurrentPlayer(TPlayer player)
{
    _m_currentPlayer = player;
    _m_pToolkitWnd->setPlayer(_m_currentPlayer);
    _m_pStatusUI->setCurPlayer(_m_currentPlayer);
}

VA(0x00482594, 0x56)
void TMapView::_realizeBrush(_TBrush brush)
{
    switch (brush) {
    case _eBrush1x1:
        _m_pMapFrameWnd->brushMode(CSize(1, 1));
        break;
    case _eBrush2x2:
        _m_pMapFrameWnd->brushMode(CSize(2, 2));
        break;
    case _eBrush4x4:
        _m_pMapFrameWnd->brushMode(CSize(4, 4));
        break;
    case _eBrushFill:
        _m_pMapFrameWnd->fillMode();
        break;
    }
}

VA(0x004825ea, 0x95)
void TMapView::_realizeObstacleBrush()
{
    switch (_m_obstacleBrush) {
    case _eObstacleBrushArea1x1:
    case _eObstacleBrushBorderedArea1x1:
    case _eObstacleBrushErase1x1:
    case _eObstacleBrushBorderedErase1x1:
        _m_pMapFrameWnd->brushMode(CSize(1, 1));
        break;
    case _eObstacleBrushArea2x2:
    case _eObstacleBrushBorderedArea2x2:
    case _eObstacleBrushErase2x2:
    case _eObstacleBrushBorderedErase2x2:
        _m_pMapFrameWnd->brushMode(CSize(2, 2));
        break;
    case _eObstacleBrushArea4x4:
    case _eObstacleBrushBorderedArea4x4:
    case _eObstacleBrushErase4x4:
    case _eObstacleBrushBorderedErase4x4:
        _m_pMapFrameWnd->brushMode(CSize(4, 4));
        break;
    case _eObstacleBrushAreaFill:
    case _eObstacleBrushBorderedAreaFill:
    case _eObstacleBrushEraseFill:
    case _eObstacleBrushBorderedEraseFill:
        _m_pMapFrameWnd->fillMode();
        break;
    }
}

VA(0x0048267f, 0x42)
void TMapView::_backupObstacleMask()
{
    if (_m_obstacleMaskHistoryPos < _m_obstacleMaskHistory.size())
        _m_obstacleMaskHistory.erase(_m_obstacleMaskHistory.begin() + _m_obstacleMaskHistoryPos,
                                     _m_obstacleMaskHistory.end());
    _m_obstacleMaskHistory.push_back(*_m_pObstacleMask);
    ++_m_obstacleMaskHistoryPos;
}

VA(0x004826c1, 0x52)
void TMapView::_paintObstacleArea(const CRect& rect)
{
    for (int y = rect.top; y < rect.bottom; ++y)
        for (int x = rect.left; x < rect.right; ++x)
            _m_pObstacleMask->setTileState(x, y, _m_bViewUnderground, TGameMapMask::eTileObstacle);
    if (_m_pMapFrameWnd != NULL) {
        _m_pMapFrameWnd->update(rect);
        _m_pMiniMapWnd->update(rect);
    }
}

VA(0x00482713, 0x210)
void TMapView::_paintBorderedObstacleArea(const CRect& rect)
{
    const TGameMap* pMap = getPDocument()->getPMap();
    int width = pMap->getWidth();
    int height = pMap->getHeight();
    int x;
    int y;
    if (rect.top >= 1) {
        for (int x = max(rect.left - 1, 0), xEnd = min(rect.right + 1, width), y = rect.top - 1; x < xEnd; ++x) {
            if (_m_pObstacleMask->getTileState(x, y, _m_bViewUnderground) == TGameMapMask::eTileNone)
                _m_pObstacleMask->setTileState(x, y, _m_bViewUnderground, TGameMapMask::eTileClear);
        }
    }
    if (rect.bottom < height) {
        for (int x = max(rect.left - 1, 0), xEnd = min(rect.right + 1, width), y = rect.bottom; x < xEnd; ++x) {
            if (_m_pObstacleMask->getTileState(x, y, _m_bViewUnderground) == TGameMapMask::eTileNone)
                _m_pObstacleMask->setTileState(x, y, _m_bViewUnderground, TGameMapMask::eTileClear);
        }
    }
    if (rect.left >= 1) {
        x = rect.left - 1;
        for (y = rect.top; y < rect.bottom; ++y) {
            if (_m_pObstacleMask->getTileState(x, y, _m_bViewUnderground) == TGameMapMask::eTileNone)
                _m_pObstacleMask->setTileState(x, y, _m_bViewUnderground, TGameMapMask::eTileClear);
        }
    }
    if (rect.right < width) {
        x = rect.right;
        for (y = rect.top; y < rect.bottom; ++y) {
            if (_m_pObstacleMask->getTileState(x, y, _m_bViewUnderground) == TGameMapMask::eTileNone)
                _m_pObstacleMask->setTileState(x, y, _m_bViewUnderground, TGameMapMask::eTileClear);
        }
    }
    for (y = rect.top; y < rect.bottom; ++y)
        for (x = rect.left; x < rect.right; ++x)
            _m_pObstacleMask->setTileState(x, y, _m_bViewUnderground, TGameMapMask::eTileObstacle);
    if (_m_pMapFrameWnd != NULL) {
        CRect updateRect(max(0, rect.left - 2), max(0, rect.top - 2), min(width, rect.right + 2),
                         min(height, rect.bottom + 2));
        _m_pMapFrameWnd->update(updateRect);
        _m_pMiniMapWnd->update(updateRect);
    }
}

VA(0x00482923, 0x5f)
void TMapView::_eraseObstacleArea(const CRect& rect)
{
    int x;
    int y;
    for (y = rect.top; y < rect.bottom; ++y)
        for (x = rect.left; x < rect.right; ++x)
            _m_pObstacleMask->setTileState(x, y, _m_bViewUnderground, TGameMapMask::eTileNone);
    if (_m_pMapFrameWnd != NULL) {
        _m_pMapFrameWnd->update(rect);
        _m_pMiniMapWnd->update(rect);
    }
}

VA(0x00482982, 0x214)
void TMapView::_eraseBorderedObstacleArea(const CRect& rect)
{
    const TGameMap* pMap = getPDocument()->getPMap();
    int width = pMap->getWidth();
    int height = pMap->getHeight();
    int x;
    int y;
    if (rect.top >= 1) {
        for (int x = max(rect.left - 1, 0), xEnd = min(rect.right + 1, width), y = rect.top - 1; x < xEnd; ++x) {
            if (_m_pObstacleMask->getTileState(x, y, _m_bViewUnderground) == TGameMapMask::eTileObstacle)
                _m_pObstacleMask->setTileState(x, y, _m_bViewUnderground, TGameMapMask::eTileClear);
        }
    }
    if (rect.bottom < height) {
        for (int x = max(rect.left - 1, 0), xEnd = min(rect.right + 1, width), y = rect.bottom; x < xEnd; ++x) {
            if (_m_pObstacleMask->getTileState(x, y, _m_bViewUnderground) == TGameMapMask::eTileObstacle)
                _m_pObstacleMask->setTileState(x, y, _m_bViewUnderground, TGameMapMask::eTileClear);
        }
    }
    if (rect.left >= 1) {
        x = rect.left - 1;
        for (y = rect.top; y < rect.bottom; ++y) {
            if (_m_pObstacleMask->getTileState(x, y, _m_bViewUnderground) == TGameMapMask::eTileObstacle)
                _m_pObstacleMask->setTileState(x, y, _m_bViewUnderground, TGameMapMask::eTileClear);
        }
    }
    if (rect.right < width) {
        x = rect.right;
        for (y = rect.top; y < rect.bottom; ++y) {
            if (_m_pObstacleMask->getTileState(x, y, _m_bViewUnderground) == TGameMapMask::eTileObstacle)
                _m_pObstacleMask->setTileState(x, y, _m_bViewUnderground, TGameMapMask::eTileClear);
        }
    }
    for (y = rect.top; y < rect.bottom; ++y)
        for (x = rect.left; x < rect.right; ++x)
            _m_pObstacleMask->setTileState(x, y, _m_bViewUnderground, TGameMapMask::eTileNone);
    if (_m_pMapFrameWnd != NULL) {
        CRect updateRect(max(0, rect.left - 2), max(0, rect.top - 2), min(width, rect.right + 2),
                         min(height, rect.bottom + 2));
        _m_pMapFrameWnd->update(updateRect);
        _m_pMiniMapWnd->update(updateRect);
    }
}

VA(0x00482b96, 0x3f)
BOOL TMapView::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
    if (_m_pMapFrameWnd != NULL && _m_pMapFrameWnd->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
        return TRUE;
    return CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

VA(0x00482bd5, 0x2e5)
void TMapView::OnInitialUpdate()
{
    _m_bViewUnderground = false;
    const TGameMap* pMap = getPDocument()->getPMap();
    _m_pStatusUI->setCurPlayer(_m_currentPlayer);
    _m_pStatusUI->setMapSize(pMap->getWidth(), pMap->getHeight(), pMap->isTwoLayer());
    _m_pObstacleMask = std::auto_ptr<TGameMapMask>(
        new TGameMapMask(pMap->getWidth(), pMap->getHeight(), pMap->isTwoLayer()));
    if (_m_pObstacleMask.get() == NULL)
        throw TAllocationFailure();
    _m_revision = getPDocument()->getRevision();
    if (_m_pMapFrameWnd == NULL) {
        if ((_m_pMapFrameWnd = new TMapFrameWnd(this, this, 28, pMap, _m_pObstacleMask.get(), false, _s_zoom,
                                                _s_bViewGrid, _s_bViewPassability)) == NULL)
            throw TAllocationFailure();
        try {
            if (GetFocus() == this)
                _m_pMapFrameWnd->SetFocus();
            if ((_m_pMiniMapWnd = new TMiniMapWnd(this, this, pMap, _m_pObstacleMask.get(), false)) == NULL)
                throw TAllocationFailure();
            if ((_m_pToolkitWnd = new TToolkitWnd(this, this)) == NULL)
                throw TAllocationFailure();
        } catch (...) {
            _deleteAll();
            throw;
        }
        _setMode(_eModeTerrain);
    } else {
        _m_pMapFrameWnd->MoveWindow(0, 0, 0, 0);
        _m_pMapFrameWnd->setMapLayer(pMap, _m_pObstacleMask.get(), false);
        _m_pMiniMapWnd->setMapLayer(pMap, _m_pObstacleMask.get(), false);
    }
    _m_viewPos = CPoint(0, 0);
    CRect rect;
    GetClientRect(&rect);
    if (rect.Width() > 0 && rect.Height() > 0)
        _layout(rect.Width(), rect.Height());
    CView::OnInitialUpdate();
}

VA(0x00482eba, 0x335)
void TMapView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
    if (_m_pMapFrameWnd == NULL)
        return;
    const TMapDoc::TNotification* pNotification = (const TMapDoc::TNotification*)lHint;
    if (pNotification == NULL || pNotification->m_type == TMapDoc::TNotification::eUpdate) {
        CRect rect;
        if (pNotification != NULL) {
            if (pNotification->m_pUpdateParams->m_bSecondLayer != _m_bViewUnderground)
                return;
            rect = pNotification->m_pUpdateParams->m_rect;
        } else {
            const TGameMap* pMap = getPDocument()->getPMap();
            rect = CRect(0, 0, pMap->getWidth(), pMap->getHeight());
        }
        _m_pMapFrameWnd->update(rect);
        _m_pMiniMapWnd->update(rect);
    } else if (pNotification->m_type == TMapDoc::TNotification::eUndo
               || pNotification->m_type == TMapDoc::TNotification::eRedo) {
        if (_m_mode == _eModeObstacles) {
            _m_obstacleMaskHistory.insert(_m_obstacleMaskHistory.begin() + _m_obstacleMaskHistoryPos,
                                          *_m_pObstacleMask);
            _m_obstacleMaskHistories.insert(
                _TObstacleMaskHistoryMap::value_type(_m_revision, _m_obstacleMaskHistory));
            _m_obstacleMaskHistory.clear();
            _m_obstacleMaskHistoryPos = 0;
            _m_revision = pNotification->m_revision;
            _TObstacleMaskHistoryMap::iterator it = _m_obstacleMaskHistories.find(_m_revision);
            if (it != _m_obstacleMaskHistories.end()) {
                _m_obstacleMaskHistory = it->second;
                _m_obstacleMaskHistories.erase(it);
                if (pNotification->m_type == TMapDoc::TNotification::eUndo) {
                    *_m_pObstacleMask = _m_obstacleMaskHistory.back();
                    _m_obstacleMaskHistory.pop_back();
                    _m_obstacleMaskHistoryPos = _m_obstacleMaskHistory.size();
                } else {
                    *_m_pObstacleMask = _m_obstacleMaskHistory.front();
                    _m_obstacleMaskHistory.erase(_m_obstacleMaskHistory.begin());
                }
            } else
                _m_pObstacleMask->clear();
        } else
            _m_revision = pNotification->m_revision;
        if (_m_bViewUnderground && !getPDocument()->getPMap()->isTwoLayer())
            _setViewUnderground(false);
        _m_pMapFrameWnd->onUndo();
        const TGameMap* pMap = getPDocument()->getPMap();
        _m_pMiniMapWnd->update(CRect(0, 0, pMap->getWidth(), pMap->getHeight()));
    } else if (pNotification->m_type == TMapDoc::TNotification::eBackup) {
        if (_m_mode == _eModeObstacles) {
            _m_obstacleMaskHistory.insert(_m_obstacleMaskHistory.begin() + _m_obstacleMaskHistoryPos,
                                          *_m_pObstacleMask);
            _m_obstacleMaskHistories.insert(
                _TObstacleMaskHistoryMap::value_type(_m_revision, _m_obstacleMaskHistory));
            _m_obstacleMaskHistory.clear();
            _m_obstacleMaskHistoryPos = 0;
        }
        _m_revision = pNotification->m_revision;
    } else if (pNotification->m_type == TMapDoc::TNotification::eDiscardBackup) {
        if (_m_mode == _eModeObstacles) {
            _TObstacleMaskHistoryMap::iterator it = _m_obstacleMaskHistories.find(_m_revision);
            if (it != _m_obstacleMaskHistories.end())
                _m_obstacleMaskHistories.erase(it);
        }
    } else if (pNotification->m_type == TMapDoc::TNotification::eObjectRemoved) {
        if (pNotification->m_pObjectRemovedParams->m_bSecondLayer == _m_bViewUnderground)
            _m_pMapFrameWnd->onObjectRemoved(pNotification->m_pObjectRemovedParams->m_objID);
    } else if (pNotification->m_type == TMapDoc::TNotification::eClear) {
        _m_pMapFrameWnd->clearMap();
        _m_pMiniMapWnd->clearMap();
        _m_pObstacleMask = std::auto_ptr<TGameMapMask>();
        _m_obstacleMaskHistory.clear();
        _m_obstacleMaskHistoryPos = 0;
        _m_obstacleMaskHistories.clear();
    }
}

void TMapView::OnDraw(CDC* pDC)
{
}

VA(0x004831f7, 0x17)
void TMapView::OnDestroy()
{
    if (_m_pMapFrameWnd != NULL)
        _deleteAll();
    CView::OnDestroy();
}

VA(0x0048320e, 0x2e)
void TMapView::OnSize(UINT nType, int cx, int cy)
{
    CView::OnSize(nType, cx, cy);
    if (cx > 0 && cy > 0 && _m_pMapFrameWnd != NULL)
        _layout(cx, cy);
}

VA(0x0048323c, 0x18)
void TMapView::OnSetFocus(CWnd* pOldWnd)
{
    if (_m_pMapFrameWnd != NULL)
        _m_pMapFrameWnd->SetFocus();
    else
        CView::OnSetFocus(pOldWnd);
}

VA(0x00483254, 0x2d)
void TMapView::OnToolsBrush1x1()
{
    _TBrush* pBrush;
    if (_m_mode == _eModeTerrain)
        pBrush = &_m_brush;
    else if (_m_mode == _eModeErase)
        pBrush = &_m_eraseBrush;
    else
        return;
    if (*pBrush != _eBrush1x1) {
        *pBrush = _eBrush1x1;
        _realizeBrush(_eBrush1x1);
    }
}

VA(0x00483281, 0x36)
void TMapView::OnUpdateToolsBrush1x1(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeTerrain && _m_brush == _eBrush1x1
                     || _m_mode == _eModeErase && _m_eraseBrush == _eBrush1x1);
}

VA(0x004832b7, 0x2d)
void TMapView::OnToolsBrush2x2()
{
    _TBrush* pBrush;
    if (_m_mode == _eModeTerrain)
        pBrush = &_m_brush;
    else if (_m_mode == _eModeErase)
        pBrush = &_m_eraseBrush;
    else
        return;
    if (*pBrush != _eBrush2x2) {
        *pBrush = _eBrush2x2;
        _realizeBrush(_eBrush2x2);
    }
}

VA(0x004832e4, 0x35)
void TMapView::OnUpdateToolsBrush2x2(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeTerrain && _m_brush == _eBrush2x2
                     || _m_mode == _eModeErase && _m_eraseBrush == _eBrush2x2);
}

VA(0x00483319, 0x2e)
void TMapView::OnToolsBrush4x4()
{
    _TBrush* pBrush;
    if (_m_mode == _eModeTerrain)
        pBrush = &_m_brush;
    else if (_m_mode == _eModeErase)
        pBrush = &_m_eraseBrush;
    else
        return;
    if (*pBrush != _eBrush4x4) {
        *pBrush = _eBrush4x4;
        _realizeBrush(_eBrush4x4);
    }
}

VA(0x00483347, 0x36)
void TMapView::OnUpdateToolsBrush4x4(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeTerrain && _m_brush == _eBrush4x4
                     || _m_mode == _eModeErase && _m_eraseBrush == _eBrush4x4);
}

VA(0x0048337d, 0x2e)
void TMapView::OnToolsBrushFill()
{
    _TBrush* pBrush;
    if (_m_mode == _eModeTerrain)
        pBrush = &_m_brush;
    else if (_m_mode == _eModeErase)
        pBrush = &_m_eraseBrush;
    else
        return;
    if (*pBrush != _eBrushFill) {
        *pBrush = _eBrushFill;
        _realizeBrush(_eBrushFill);
    }
}

VA(0x004833ab, 0x36)
void TMapView::OnUpdateToolsBrushFill(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeTerrain && _m_brush == _eBrushFill
                     || _m_mode == _eModeErase && _m_eraseBrush == _eBrushFill);
}

VA(0x004833e1, 0x8)
void TMapView::OnToolsErase()
{
    _setMode(_eModeErase);
}

VA(0x004833e9, 0x1d)
void TMapView::OnUpdateToolsErase(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeErase);
}

VA(0x00483406, 0x2b)
void TMapView::OnToolsErase1x1()
{
    OnToolsErase();
    _setEraseBrush(_eBrush1x1);
}

VA(0x00483431, 0x23)
void TMapView::OnUpdateToolsErase1x1(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeErase && _m_eraseBrush == _eBrush1x1);
}

VA(0x00483454, 0x2c)
void TMapView::OnToolsErase2x2()
{
    OnToolsErase();
    _setEraseBrush(_eBrush2x2);
}

VA(0x00483480, 0x23)
void TMapView::OnUpdateToolsErase2x2(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeErase && _m_eraseBrush == _eBrush2x2);
}

VA(0x004834a3, 0x2c)
void TMapView::OnToolsErase4x4()
{
    OnToolsErase();
    _setEraseBrush(_eBrush4x4);
}

VA(0x004834cf, 0x26)
void TMapView::OnUpdateToolsErase4x4(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeErase && _m_eraseBrush == _eBrush4x4);
}

VA(0x004834f5, 0x2c)
void TMapView::OnToolsEraseFill()
{
    OnToolsErase();
    _setEraseBrush(_eBrushFill);
}

VA(0x00483521, 0x26)
void TMapView::OnUpdateToolsEraseFill(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeErase && _m_eraseBrush == _eBrushFill);
}

VA(0x00483547, 0x110)
void TMapView::OnToolsMapSpecifications()
{
    TGameMap* pMap = getPDocument()->getPMap();
    TGameMap map(*pMap);
    TMapSpecsSheet sheet(this, &map, TMapDoc::getSpecialTileFrequency());
    sheet.DoModal();
    if (sheet.wasModified()) {
        getPDocument()->backupMap();
        *pMap = map;
        getPDocument()->SetModifiedFlag();
        if (pMap->isTwoLayer()) {
            if (!_m_pObstacleMask->isTwoLayer())
                _m_pObstacleMask->addSecondLayer();
        } else if (_m_pObstacleMask->isTwoLayer())
            _m_pObstacleMask->removeSecondLayer();
        if (_m_bViewUnderground && !getPDocument()->getPMap()->isTwoLayer())
            _setViewUnderground(false);
    }
}

VA(0x00483657, 0xd)
void TMapView::OnUpdateToolsMapSpecifications(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(TRUE);
}

VA(0x00483664, 0x1e)
void TMapView::OnToolsObjectsAllTerrain()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotAllTerrain);
}

VA(0x00483682, 0x26)
void TMapView::OnUpdateToolsObjectsAllTerrain(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObjects && _m_objectSlot == eSlotAllTerrain);
}

VA(0x004836a8, 0x1e)
void TMapView::OnToolsObjectsArtifacts()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotArtifacts);
}

VA(0x004836c6, 0x26)
void TMapView::OnUpdateToolsObjectsArtifacts(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObjects && _m_objectSlot == eSlotArtifacts);
}

VA(0x004836ec, 0x1d)
void TMapView::OnToolsObjectsDirt()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotDirt);
}

VA(0x00483709, 0x23)
void TMapView::OnUpdateToolsObjectsDirt(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObjects && _m_objectSlot == eSlotDirt);
}

VA(0x0048372c, 0x1e)
void TMapView::OnToolsObjectsGrass()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotGrass);
}

VA(0x0048374a, 0x26)
void TMapView::OnUpdateToolsObjectsGrass(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObjects && _m_objectSlot == eSlotGrass);
}

VA(0x00483770, 0x1e)
void TMapView::OnToolsObjectsHeroes()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotHeroes);
}

VA(0x0048378e, 0x26)
void TMapView::OnUpdateToolsObjectsHeroes(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObjects && _m_objectSlot == eSlotHeroes);
}

VA(0x004837b4, 0x1e)
void TMapView::OnToolsObjectsLava()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotLava);
}

VA(0x004837d2, 0x26)
void TMapView::OnUpdateToolsObjectsLava(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObjects && _m_objectSlot == eSlotLava);
}

VA(0x004837f8, 0x1e)
void TMapView::OnToolsObjectsMonsters()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotMonsters);
}

VA(0x00483816, 0x26)
void TMapView::OnUpdateToolsObjectsMonsters(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObjects && _m_objectSlot == eSlotMonsters);
}

VA(0x0048383c, 0x20)
void TMapView::OnToolsObjectsRough()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotRough);
}

VA(0x0048385c, 0x26)
void TMapView::OnUpdateToolsObjectsRough(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObjects && _m_objectSlot == eSlotRough);
}

VA(0x00483882, 0x1e)
void TMapView::OnToolsObjectsSand()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotSand);
}

VA(0x004838a0, 0x23)
void TMapView::OnUpdateToolsObjectsSand(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObjects && _m_objectSlot == eSlotSand);
}

VA(0x004838c3, 0x1e)
void TMapView::OnToolsObjectsSnow()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotSnow);
}

VA(0x004838e1, 0x26)
void TMapView::OnUpdateToolsObjectsSnow(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObjects && _m_objectSlot == eSlotSnow);
}

VA(0x00483907, 0x1e)
void TMapView::OnToolsObjectsSubterranean()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotSubterranean);
}

VA(0x00483925, 0x26)
void TMapView::OnUpdateToolsObjectsSubterranean(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObjects && _m_objectSlot == eSlotSubterranean);
}

VA(0x0048394b, 0x1e)
void TMapView::OnToolsObjectsSwamp()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotSwamp);
}

VA(0x00483969, 0x26)
void TMapView::OnUpdateToolsObjectsSwamp(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObjects && _m_objectSlot == eSlotSwamp);
}

VA(0x0048398f, 0x1e)
void TMapView::OnToolsObjectsTowns()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotTowns);
}

VA(0x004839ad, 0x26)
void TMapView::OnUpdateToolsObjectsTowns(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObjects && _m_objectSlot == eSlotTowns);
}

VA(0x004839d3, 0x1e)
void TMapView::OnToolsObjectsTreasures()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotTreasures);
}

VA(0x004839f1, 0x26)
void TMapView::OnUpdateToolsObjectsTreasures(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObjects && _m_objectSlot == eSlotTreasures);
}

VA(0x00483a17, 0x1e)
void TMapView::OnToolsObjectsWater()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotWater);
}

VA(0x00483a35, 0x26)
void TMapView::OnUpdateToolsObjectsWater(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObjects && _m_objectSlot == eSlotWater);
}

VA(0x00483a5b, 0x8)
void TMapView::OnToolsRivers()
{
    _setMode(_eModeRiver);
}

VA(0x00483a63, 0x1d)
void TMapView::OnUpdateToolsRivers(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeRiver);
}

VA(0x00483a80, 0x16)
void TMapView::OnToolsRiversClear()
{
    OnToolsRivers();
    _m_riverType = 1;
}

VA(0x00483a96, 0x23)
void TMapView::OnUpdateToolsRiversClear(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeRiver && _m_riverType == 1);
}

VA(0x00483ab9, 0x16)
void TMapView::OnToolsRiversIcy()
{
    OnToolsRivers();
    _m_riverType = 2;
}

VA(0x00483acf, 0x26)
void TMapView::OnUpdateToolsRiversIcy(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeRiver && _m_riverType == 2);
}

VA(0x00483af5, 0x16)
void TMapView::OnToolsRiversLava()
{
    OnToolsRivers();
    _m_riverType = 4;
}

VA(0x00483b0b, 0x26)
void TMapView::OnUpdateToolsRiversLava(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeRiver && _m_riverType == 4);
}

VA(0x00483b31, 0x16)
void TMapView::OnToolsRiversMuddy()
{
    OnToolsRivers();
    _m_riverType = 3;
}

VA(0x00483b47, 0x26)
void TMapView::OnUpdateToolsRiversMuddy(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeRiver && _m_riverType == 3);
}

VA(0x00483b6d, 0x8)
void TMapView::OnToolsRoads()
{
    _setMode(_eModeRoad);
}

VA(0x00483b75, 0x1d)
void TMapView::OnUpdateToolsRoads(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeRoad);
}

VA(0x00483b92, 0x16)
void TMapView::OnToolsRoadsCobblestone()
{
    OnToolsRoads();
    _m_roadType = 3;
}

VA(0x00483ba8, 0x26)
void TMapView::OnUpdateToolsRoadsCobblestone(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeRoad && _m_roadType == 3);
}

VA(0x00483bce, 0x16)
void TMapView::OnToolsRoadsDirt()
{
    OnToolsRoads();
    _m_roadType = 1;
}

VA(0x00483be4, 0x23)
void TMapView::OnUpdateToolsRoadsDirt(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeRoad && _m_roadType == 1);
}

VA(0x00483c07, 0x16)
void TMapView::OnToolsRoadsGravel()
{
    OnToolsRoads();
    _m_roadType = 2;
}

VA(0x00483c1d, 0x26)
void TMapView::OnUpdateToolsRoadsGravel(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeRoad && _m_roadType == 2);
}

VA(0x00483c43, 0x8)
void TMapView::OnToolsTerrain()
{
    _setMode(_eModeTerrain);
}

VA(0x00483c4b, 0x1d)
void TMapView::OnUpdateToolsTerrain(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeTerrain);
}

VA(0x00483c68, 0x2b)
void TMapView::OnToolsTerrain1x1()
{
    OnToolsTerrain();
    _setTerrainBrush(_eBrush1x1);
}

VA(0x00483c93, 0x23)
void TMapView::OnUpdateToolsTerrain1x1(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeTerrain && _m_brush == _eBrush1x1);
}

VA(0x00483cb6, 0x30)
void TMapView::OnToolsTerrain2x2()
{
    OnToolsTerrain();
    _setTerrainBrush(_eBrush2x2);
}

VA(0x00483ce6, 0x22)
void TMapView::OnUpdateToolsTerrain2x2(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeTerrain && _m_brush == _eBrush2x2);
}

VA(0x00483d08, 0x2c)
void TMapView::OnToolsTerrain4x4()
{
    OnToolsTerrain();
    _setTerrainBrush(_eBrush4x4);
}

VA(0x00483d34, 0x23)
void TMapView::OnUpdateToolsTerrain4x4(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeTerrain && _m_brush == _eBrush4x4);
}

VA(0x00483d57, 0x2c)
void TMapView::OnToolsTerrainFill()
{
    OnToolsTerrain();
    _setTerrainBrush(_eBrushFill);
}

VA(0x00483d83, 0x23)
void TMapView::OnUpdateToolsTerrainFill(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeTerrain && _m_brush == _eBrushFill);
}

VA(0x00483da6, 0x13)
void TMapView::OnToolsTerrainDirt()
{
    OnToolsTerrain();
    _m_terrainType = eTerrainDirt;
}

VA(0x00483db9, 0x23)
void TMapView::OnUpdateToolsTerrainDirt(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeTerrain && _m_terrainType == eTerrainDirt);
}

VA(0x00483ddc, 0x16)
void TMapView::OnToolsTerrainGrass()
{
    OnToolsTerrain();
    _m_terrainType = eTerrainGrass;
}

VA(0x00483df2, 0x23)
void TMapView::OnUpdateToolsTerrainGrass(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeTerrain && _m_terrainType == eTerrainGrass);
}

VA(0x00483e15, 0x16)
void TMapView::OnToolsTerrainLava()
{
    OnToolsTerrain();
    _m_terrainType = eTerrainLava;
}

VA(0x00483e2b, 0x23)
void TMapView::OnUpdateToolsTerrainLava(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeTerrain && _m_terrainType == eTerrainLava);
}

VA(0x00483e4e, 0x16)
void TMapView::OnToolsTerrainRough()
{
    OnToolsTerrain();
    _m_terrainType = eTerrainRough;
}

VA(0x00483e64, 0x23)
void TMapView::OnUpdateToolsTerrainRough(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeTerrain && _m_terrainType == eTerrainRough);
}

VA(0x00483e87, 0x16)
void TMapView::OnToolsTerrainSand()
{
    OnToolsTerrain();
    _m_terrainType = eTerrainSand;
}

VA(0x00483e9d, 0x22)
void TMapView::OnUpdateToolsTerrainSand(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeTerrain && _m_terrainType == eTerrainSand);
}

VA(0x00483ebf, 0x16)
void TMapView::OnToolsTerrainSnow()
{
    OnToolsTerrain();
    _m_terrainType = eTerrainSnow;
}

VA(0x00483ed5, 0x23)
void TMapView::OnUpdateToolsTerrainSnow(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeTerrain && _m_terrainType == eTerrainSnow);
}

VA(0x00483ef8, 0x16)
void TMapView::OnToolsTerrainSubterranean()
{
    OnToolsTerrain();
    _m_terrainType = eTerrainSubterranean;
}

VA(0x00483f0e, 0x23)
void TMapView::OnUpdateToolsTerrainSubterranean(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeTerrain && _m_terrainType == eTerrainSubterranean);
}

VA(0x00483f31, 0x16)
void TMapView::OnToolsTerrainSwamp()
{
    OnToolsTerrain();
    _m_terrainType = eTerrainSwamp;
}

VA(0x00483f47, 0x23)
void TMapView::OnUpdateToolsTerrainSwamp(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeTerrain && _m_terrainType == eTerrainSwamp);
}

VA(0x00483f6a, 0x16)
void TMapView::OnToolsTerrainWater()
{
    OnToolsTerrain();
    _m_terrainType = eTerrainWater;
}

VA(0x00483f80, 0x23)
void TMapView::OnUpdateToolsTerrainWater(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeTerrain && _m_terrainType == eTerrainWater);
}

VA(0x00483fa3, 0x16)
void TMapView::OnToolsTerrainRock()
{
    OnToolsTerrain();
    _m_terrainType = eTerrainRock;
}

VA(0x00483fb9, 0x23)
void TMapView::OnUpdateToolsTerrainRock(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeTerrain && _m_terrainType == eTerrainRock);
}

VA(0x00483fdc, 0x26)
void TMapView::OnViewUnderground()
{
    if (getPDocument()->getPMap()->isTwoLayer())
        _setViewUnderground(!_m_bViewUnderground);
}

VA(0x00484002, 0x3e)
void TMapView::OnUpdateViewUnderground(CCmdUI* pCmdUI)
{
    if (getPDocument()->getPMap()->isTwoLayer())
        pCmdUI->SetCheck(_m_bViewUnderground);
    else {
        pCmdUI->SetCheck(0);
        pCmdUI->Enable(FALSE);
    }
}

VA(0x00484040, 0x8)
void TMapView::OnPlayersNone()
{
    _setCurrentPlayer(ePlayerNone);
}

VA(0x00484048, 0x1d)
void TMapView::OnUpdatePlayersNone(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_currentPlayer == ePlayerNone);
}

VA(0x00484065, 0x8)
void TMapView::OnPlayersPlayer1()
{
    _setCurrentPlayer(TPlayer(0));
}

VA(0x0048406d, 0x1c)
void TMapView::OnUpdatePlayersPlayer1(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_currentPlayer == TPlayer(0));
}

VA(0x00484089, 0x8)
void TMapView::OnPlayersPlayer2()
{
    _setCurrentPlayer(TPlayer(1));
}

VA(0x00484091, 0x1d)
void TMapView::OnUpdatePlayersPlayer2(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_currentPlayer == TPlayer(1));
}

VA(0x004840ae, 0x8)
void TMapView::OnPlayersPlayer3()
{
    _setCurrentPlayer(TPlayer(2));
}

VA(0x004840b6, 0x1d)
void TMapView::OnUpdatePlayersPlayer3(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_currentPlayer == TPlayer(2));
}

VA(0x004840d3, 0x8)
void TMapView::OnPlayersPlayer4()
{
    _setCurrentPlayer(TPlayer(3));
}

VA(0x004840db, 0x1d)
void TMapView::OnUpdatePlayersPlayer4(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_currentPlayer == TPlayer(3));
}

VA(0x004840f8, 0x8)
void TMapView::OnPlayersPlayer5()
{
    _setCurrentPlayer(TPlayer(4));
}

VA(0x00484100, 0x1d)
void TMapView::OnUpdatePlayersPlayer5(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_currentPlayer == TPlayer(4));
}

VA(0x0048411d, 0x8)
void TMapView::OnPlayersPlayer6()
{
    _setCurrentPlayer(TPlayer(5));
}

VA(0x00484125, 0x1d)
void TMapView::OnUpdatePlayersPlayer6(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_currentPlayer == TPlayer(5));
}

VA(0x00484142, 0x8)
void TMapView::OnPlayersPlayer7()
{
    _setCurrentPlayer(TPlayer(6));
}

VA(0x0048414a, 0x1d)
void TMapView::OnUpdatePlayersPlayer7(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_currentPlayer == TPlayer(6));
}

VA(0x00484167, 0x8)
void TMapView::OnPlayersPlayer8()
{
    _setCurrentPlayer(TPlayer(7));
}

VA(0x0048416f, 0x1d)
void TMapView::OnUpdatePlayersPlayer8(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_currentPlayer == TPlayer(7));
}

VA(0x0048418c, 0xd0)
void TMapView::OnEditUndo()
{
    if (_m_mode == _eModeObstacles && _m_obstacleMaskHistoryPos > 0) {
        CWaitCursor wait;
        --_m_obstacleMaskHistoryPos;
        std::swap(*_m_pObstacleMask, _m_obstacleMaskHistory[_m_obstacleMaskHistoryPos]);
        if (_m_pMapFrameWnd != NULL) {
            const TGameMap* pMap = getPDocument()->getPMap();
            _m_pMapFrameWnd->onUndo();
            _m_pMiniMapWnd->update(CRect(0, 0, pMap->getWidth(), pMap->getHeight()));
        }
    } else if (getPDocument()->canUndo()) {
        CWaitCursor wait;
        getPDocument()->undo();
    }
}

VA(0x0048425c, 0x2a)
void TMapView::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(_m_mode == _eModeObstacles && _m_obstacleMaskHistoryPos > 0 || getPDocument()->canUndo());
}

VA(0x00484286, 0xee)
void TMapView::OnEditRedo()
{
    if (_m_mode == _eModeObstacles && _m_obstacleMaskHistoryPos < _m_obstacleMaskHistory.size()) {
        CWaitCursor wait;
        std::swap(*_m_pObstacleMask, _m_obstacleMaskHistory[_m_obstacleMaskHistoryPos++]);
        if (_m_pMapFrameWnd != NULL) {
            const TGameMap* pMap = getPDocument()->getPMap();
            _m_pMapFrameWnd->onUndo();
            _m_pMiniMapWnd->update(CRect(0, 0, pMap->getWidth(), pMap->getHeight()));
        }
    } else if (getPDocument()->canRedo()) {
        CWaitCursor wait;
        getPDocument()->redo();
    }
}

VA(0x00484374, 0x45)
void TMapView::OnUpdateEditRedo(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(_m_mode == _eModeObstacles && _m_obstacleMaskHistoryPos < _m_obstacleMaskHistory.size()
                   || getPDocument()->canRedo());
}

VA(0x004843b9, 0x96)
void TMapView::OnToolsValidateMap()
{
    TMapValidationFunc validate(*getPDocument()->getPMap());
    TMapValidationDlg dlg(this, validate());
    dlg.DoModal();
}

VA(0x004845e2, 0x1c)
void TMapView::OnUpdateEditFind(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(_m_mode == _eModeObjects);
}

VA(0x004847c3, 0x25)
void TMapView::OnUpdateEditFindNext(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(_m_mode == _eModeObjects && _m_lastFindType != -1);
}

VA(0x004849ad, 0x8)
void TMapView::OnToolsObstacles()
{
    _setMode(_eModeObstacles);
}

VA(0x004849b5, 0x1d)
void TMapView::OnUpdateToolsObstacles(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObstacles);
}

VA(0x004849d2, 0x2b)
void TMapView::OnToolsObstaclesErase1x1()
{
    OnToolsObstacles();
    _setObstacleBrush(_eObstacleBrushErase1x1);
}

VA(0x004849fd, 0x26)
void TMapView::OnUpdateToolsObstaclesErase1x1(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObstacles && _m_obstacleBrush == _eObstacleBrushErase1x1);
}

VA(0x00484a23, 0x2b)
void TMapView::OnToolsObstaclesErase2x2()
{
    OnToolsObstacles();
    _setObstacleBrush(_eObstacleBrushErase2x2);
}

VA(0x00484a4e, 0x26)
void TMapView::OnUpdateToolsObstaclesErase2x2(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObstacles && _m_obstacleBrush == _eObstacleBrushErase2x2);
}

VA(0x00484a74, 0x2b)
void TMapView::OnToolsObstaclesErase4x4()
{
    OnToolsObstacles();
    _setObstacleBrush(_eObstacleBrushErase4x4);
}

VA(0x00484a9f, 0x26)
void TMapView::OnUpdateToolsObstaclesErase4x4(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObstacles && _m_obstacleBrush == _eObstacleBrushErase4x4);
}

VA(0x00484ac5, 0x2b)
void TMapView::OnToolsObstaclesEraseFill()
{
    OnToolsObstacles();
    _setObstacleBrush(_eObstacleBrushEraseFill);
}

VA(0x00484af0, 0x26)
void TMapView::OnUpdateToolsObstaclesEraseFill(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObstacles && _m_obstacleBrush == _eObstacleBrushEraseFill);
}

VA(0x00484b16, 0x2b)
void TMapView::OnToolsObstaclesBorderedArea1x1()
{
    OnToolsObstacles();
    _setObstacleBrush(_eObstacleBrushBorderedArea1x1);
}

VA(0x00484b41, 0x26)
void TMapView::OnUpdateToolsObstaclesBorderedArea1x1(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObstacles && _m_obstacleBrush == _eObstacleBrushBorderedArea1x1);
}

VA(0x00484b67, 0x2b)
void TMapView::OnToolsObstaclesBorderedArea2x2()
{
    OnToolsObstacles();
    _setObstacleBrush(_eObstacleBrushBorderedArea2x2);
}

VA(0x00484b92, 0x26)
void TMapView::OnUpdateToolsObstaclesBorderedArea2x2(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObstacles && _m_obstacleBrush == _eObstacleBrushBorderedArea2x2);
}

VA(0x00484bb8, 0x2f)
void TMapView::OnToolsObstaclesBorderedArea4x4()
{
    OnToolsObstacles();
    _setObstacleBrush(_eObstacleBrushBorderedArea4x4);
}

VA(0x00484be7, 0x26)
void TMapView::OnUpdateToolsObstaclesBorderedArea4x4(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObstacles && _m_obstacleBrush == _eObstacleBrushBorderedArea4x4);
}

VA(0x00484c0d, 0x2b)
void TMapView::OnToolsObstaclesBorderedAreaFill()
{
    OnToolsObstacles();
    _setObstacleBrush(_eObstacleBrushBorderedAreaFill);
}

VA(0x00484c38, 0x26)
void TMapView::OnUpdateToolsObstaclesBorderedAreaFill(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObstacles && _m_obstacleBrush == _eObstacleBrushBorderedAreaFill);
}

VA(0x00484c5e, 0x2e)
void TMapView::OnToolsObstaclesArea1x1()
{
    OnToolsObstacles();
    _setObstacleBrush(_eObstacleBrushArea1x1);
}

VA(0x00484c8c, 0x23)
void TMapView::OnUpdateToolsObstaclesArea1x1(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObstacles && _m_obstacleBrush == _eObstacleBrushArea1x1);
}

VA(0x00484caf, 0x2b)
void TMapView::OnToolsObstaclesArea2x2()
{
    OnToolsObstacles();
    _setObstacleBrush(_eObstacleBrushArea2x2);
}

VA(0x00484cda, 0x23)
void TMapView::OnUpdateToolsObstaclesArea2x2(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObstacles && _m_obstacleBrush == _eObstacleBrushArea2x2);
}

VA(0x00484cfd, 0x2b)
void TMapView::OnToolsObstaclesArea4x4()
{
    OnToolsObstacles();
    _setObstacleBrush(_eObstacleBrushArea4x4);
}

VA(0x00484d28, 0x26)
void TMapView::OnUpdateToolsObstaclesArea4x4(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObstacles && _m_obstacleBrush == _eObstacleBrushArea4x4);
}

VA(0x00484d4e, 0x2b)
void TMapView::OnToolsObstaclesAreaFill()
{
    OnToolsObstacles();
    _setObstacleBrush(_eObstacleBrushAreaFill);
}

VA(0x00484d79, 0x26)
void TMapView::OnUpdateToolsObstaclesAreaFill(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObstacles && _m_obstacleBrush == _eObstacleBrushAreaFill);
}

VA(0x00484d9f, 0x187)
void TMapView::OnToolsObstaclesPlace()
{
    if (_m_pObstacleMask.get() == NULL)
        return;
    TMapDoc* pDoc = getPDocument();
    TGameMap* pMap = pDoc->getPMap();
    if (_m_mode != _eModeObstacles
        || !_m_pObstacleMask->hasTiles(false) && !(pMap->isTwoLayer() && _m_pObstacleMask->hasTiles(true)))
        return;
    CWaitCursor wait;
    pDoc->backupMap();
    ERmgMapVersion mapVersion;
    switch (pMap->getVersion()) {
    case GAME_VERSION_ROE:
        mapVersion = RMG_MAP_RESTORATION_OF_ERATHIA;
        break;
    case GAME_VERSION_AB:
        mapVersion = RMG_MAP_ARMAGEDDONS_BLADE;
        break;
    case GAME_VERSION_SOD:
        mapVersion = RMG_MAP_SHADOW_OF_DEATH;
        break;
    }
    if (_m_pObstacleMask->hasTiles(false)) {
        TObstaclePlacerMap placerMap(pMap, _m_pObstacleMask.get(), false);
        placeRandomObjects(&placerMap, true, NULL, mapVersion);
    }
    if (pMap->isTwoLayer() && _m_pObstacleMask->hasTiles(true)) {
        TObstaclePlacerMap placerMap(pMap, _m_pObstacleMask.get(), true);
        placeRandomObjects(&placerMap, true, NULL, mapVersion);
    }
    _m_pObstacleMask->clear();
    _m_pMapFrameWnd->update(CRect(0, 0, pMap->getWidth(), pMap->getHeight()));
    _m_pMiniMapWnd->update(CRect(0, 0, pMap->getWidth(), pMap->getHeight()));
}

VA(0x00484f26, 0x54)
void TMapView::OnUpdateToolsObstaclesPlace(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(_m_mode == _eModeObstacles && _m_pObstacleMask.get() != NULL
                   && (_m_pObstacleMask->hasTiles(false)
                       || getPDocument()->getPMap()->isTwoLayer() && _m_pObstacleMask->hasTiles(true)));
}

VA(0x00484f7a, 0x2b)
void TMapView::OnToolsObstaclesBorderedErase1x1()
{
    OnToolsObstacles();
    _setObstacleBrush(_eObstacleBrushBorderedErase1x1);
}

VA(0x00484fa5, 0x26)
void TMapView::OnUpdateToolsObstaclesBorderedErase1x1(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObstacles && _m_obstacleBrush == _eObstacleBrushBorderedErase1x1);
}

VA(0x00484fcb, 0x2b)
void TMapView::OnToolsObstaclesBorderedErase2x2()
{
    OnToolsObstacles();
    _setObstacleBrush(_eObstacleBrushBorderedErase2x2);
}

VA(0x00484ff6, 0x26)
void TMapView::OnUpdateToolsObstaclesBorderedErase2x2(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObstacles && _m_obstacleBrush == _eObstacleBrushBorderedErase2x2);
}

VA(0x0048501c, 0x2b)
void TMapView::OnToolsObstaclesBorderedErase4x4()
{
    OnToolsObstacles();
    _setObstacleBrush(_eObstacleBrushBorderedErase4x4);
}

VA(0x00485047, 0x26)
void TMapView::OnUpdateToolsObstaclesBorderedErase4x4(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObstacles && _m_obstacleBrush == _eObstacleBrushBorderedErase4x4);
}

VA(0x0048506d, 0x2b)
void TMapView::OnToolsObstaclesBorderedEraseFill()
{
    OnToolsObstacles();
    _setObstacleBrush(_eObstacleBrushBorderedEraseFill);
}

VA(0x00485098, 0x26)
void TMapView::OnUpdateToolsObstaclesBorderedEraseFill(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeObstacles && _m_obstacleBrush == _eObstacleBrushBorderedEraseFill);
}

VA(0x004850be, 0x13)
void TMapView::OnToolsRiversErase()
{
    OnToolsRivers();
    _m_riverType = 0;
}

VA(0x004850d1, 0x23)
void TMapView::OnUpdateToolsRiversErase(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeRiver && _m_riverType == 0);
}

VA(0x004850f4, 0x13)
void TMapView::OnToolsRoadsErase()
{
    OnToolsRoads();
    _m_roadType = 0;
}

VA(0x00485107, 0x23)
void TMapView::OnUpdateToolsRoadsErase(CCmdUI* pCmdUI)
{
    pCmdUI->SetRadio(_m_mode == _eModeRoad && _m_roadType == 0);
}
