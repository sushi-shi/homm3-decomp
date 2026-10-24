// MainFrame.cpp - the editor's main window (h3maped 0x45948c..0x45beb9;
// GOG only). The frame names its menus from editor.txt (the menu resource
// carries only placeholders), docks the tool bar and the mode bar, sizes
// the status bar's object name, player and map size panes, restores its
// placement and the bars' layout from the registry and saves them when it
// closes. It refuses to restore or maximize below 16-bit colour and
// minimizes itself when the display drops below it.
//
// Every command's status bar prompt and tool tip come from the two columns
// of its row of "editrcmd.txt"; the player commands share one row whose
// prompt formats the player's colour.
#include "editor/stdafx.h"

#include <afxadv.h>
#include <stdio.h>
#include <map>
#include <utility>

#include "va.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "textresource.h"
#include "editor/AfxPrivMessages.h"
#include "editor/FormattedString.h"
#include "editor/GDIObjectSelector.h"
#include "editor/MainFrame.h"
#include "editor/MapEditorText.h"
#include "editor/MemoryDC.h"
#include "editor/Player.h"
#include "editor/resource.h"

namespace {

VA(0x0045948c, 0xc3)
void setMainMenuTexts(CMenu* pMenu)
{
    unsigned int index = getFirstMenuItem(pMenu);
    setMenuItemText(pMenu, index, SMainMenuText::kFileStr);
    setMenuItemText(pMenu, index, SMainMenuText::kEditStr);
    setMenuItemText(pMenu, index, SMainMenuText::kViewStr);
    setMenuItemText(pMenu, index, SMainMenuText::kToolsStr);
    setMenuItemText(pMenu, index, SMainMenuText::kPlayerStr);
    setMenuItemText(pMenu, index, SMainMenuText::kHelpStr);
}

VA(0x0045954f, 0xfd)
void setFileMenuTexts(CMenu* pMenu)
{
    unsigned int index = getFirstMenuItem(pMenu);
    setMenuItemText(pMenu, index, SFileMenuText::kNewStr);
    setMenuItemText(pMenu, index, SFileMenuText::kOpenStr);
    setMenuItemText(pMenu, index, SFileMenuText::kSaveStr);
    setMenuItemText(pMenu, index, SFileMenuText::kSaveAsStr);
    setMenuItemText(pMenu, index, SFileMenuText::kExportTextStr);
    setMenuItemText(pMenu, index, SFileMenuText::kImportTextStr);
    setMenuItemText(pMenu, index, SFileMenuText::kRecentFileStr);
    setMenuItemText(pMenu, index, SFileMenuText::kExitStr);
}

VA(0x0045964c, 0x137)
void setEditMenuTexts(CMenu* pMenu)
{
    unsigned int index = getFirstMenuItem(pMenu);
    setMenuItemText(pMenu, index, SEditMenuText::kUndoStr);
    setMenuItemText(pMenu, index, SEditMenuText::kRedoStr);
    setMenuItemText(pMenu, index, SEditMenuText::kCutStr);
    setMenuItemText(pMenu, index, SEditMenuText::kCopyStr);
    setMenuItemText(pMenu, index, SEditMenuText::kPasteStr);
    setMenuItemText(pMenu, index, SEditMenuText::kDeleteStr);
    setMenuItemText(pMenu, index, SEditMenuText::kFindStr);
    setMenuItemText(pMenu, index, SEditMenuText::kFindNextStr);
    setMenuItemText(pMenu, index, SEditMenuText::kFindPrevStr);
    setMenuItemText(pMenu, index, SEditMenuText::kPropertiesStr);
}

VA(0x00459783, 0x137)
void setViewMenuTexts(CMenu* pMenu)
{
    unsigned int index = getFirstMenuItem(pMenu);
    setMenuItemText(pMenu, index, SViewMenuText::kZoomInStr);
    setMenuItemText(pMenu, index, SViewMenuText::kZoomOutStr);
    setMenuItemText(pMenu, index, SViewMenuText::kUndergroundStr);
    setMenuItemText(pMenu, index, SViewMenuText::kGridStr);
    setMenuItemText(pMenu, index, SViewMenuText::kPassabilityStr);
    setMenuItemText(pMenu, index, SViewMenuText::kObjectAnimationStr);
    setMenuItemText(pMenu, index, SViewMenuText::kTerrainAnimationStr);
    setMenuItemText(pMenu, index, SViewMenuText::kToolbarStr);
    setMenuItemText(pMenu, index, SViewMenuText::kModeBarStr);
    setMenuItemText(pMenu, index, SViewMenuText::kStatusBarStr);
}

VA(0x004598ba, 0x11a)
void setToolsMenuTexts(CMenu* pMenu)
{
    unsigned int index = getFirstMenuItem(pMenu);
    setMenuItemText(pMenu, index, SToolsMenuText::kTerrainStr);
    setMenuItemText(pMenu, index, SToolsMenuText::kRiversStr);
    setMenuItemText(pMenu, index, SToolsMenuText::kRoadsStr);
    setMenuItemText(pMenu, index, SToolsMenuText::kEraseStr);
    setMenuItemText(pMenu, index, SToolsMenuText::kObstaclesStr);
    setMenuItemText(pMenu, index, SToolsMenuText::kObjectsStr);
    setMenuItemText(pMenu, index, SToolsMenuText::kMapSpecificationsStr);
    setMenuItemText(pMenu, index, SToolsMenuText::kValidateMapStr);
    setMenuItemText(pMenu, index, SToolsMenuText::kOptionsStr);
}

VA(0x004599d4, 0x1ab)
void setToolsTerrainMenuTexts(CMenu* pMenu)
{
    unsigned int index = getFirstMenuItem(pMenu);
    setMenuItemText(pMenu, index, SToolsTerrainMenuText::k1x1Str);
    setMenuItemText(pMenu, index, SToolsTerrainMenuText::k2x2Str);
    setMenuItemText(pMenu, index, SToolsTerrainMenuText::k4x4Str);
    setMenuItemText(pMenu, index, SToolsTerrainMenuText::kFillStr);
    setMenuItemText(pMenu, index, SToolsTerrainMenuText::kDirtStr);
    setMenuItemText(pMenu, index, SToolsTerrainMenuText::kSandStr);
    setMenuItemText(pMenu, index, SToolsTerrainMenuText::kGrassStr);
    setMenuItemText(pMenu, index, SToolsTerrainMenuText::kSnowStr);
    setMenuItemText(pMenu, index, SToolsTerrainMenuText::kSwampStr);
    setMenuItemText(pMenu, index, SToolsTerrainMenuText::kRoughStr);
    setMenuItemText(pMenu, index, SToolsTerrainMenuText::kSubterraneanStr);
    setMenuItemText(pMenu, index, SToolsTerrainMenuText::kLavaStr);
    setMenuItemText(pMenu, index, SToolsTerrainMenuText::kWaterStr);
    setMenuItemText(pMenu, index, SToolsTerrainMenuText::kRockStr);
}

VA(0x00459b7f, 0xa6)
void setToolsRiversMenuTexts(CMenu* pMenu)
{
    unsigned int index = getFirstMenuItem(pMenu);
    setMenuItemText(pMenu, index, SToolsRiversMenuText::kClearStr);
    setMenuItemText(pMenu, index, SToolsRiversMenuText::kIcyStr);
    setMenuItemText(pMenu, index, SToolsRiversMenuText::kMuddyStr);
    setMenuItemText(pMenu, index, SToolsRiversMenuText::kLavaStr);
    setMenuItemText(pMenu, index, SToolsRiversMenuText::kEraserStr);
}

VA(0x00459c25, 0x89)
void setToolsRoadsMenuTexts(CMenu* pMenu)
{
    unsigned int index = getFirstMenuItem(pMenu);
    setMenuItemText(pMenu, index, SToolsRoadsMenuText::kDirtStr);
    setMenuItemText(pMenu, index, SToolsRoadsMenuText::kGravelStr);
    setMenuItemText(pMenu, index, SToolsRoadsMenuText::kCobblestoneStr);
    setMenuItemText(pMenu, index, SToolsRoadsMenuText::kEraserStr);
}

VA(0x00459cae, 0x89)
void setToolsEraseMenuTexts(CMenu* pMenu)
{
    unsigned int index = getFirstMenuItem(pMenu);
    setMenuItemText(pMenu, index, SToolsEraseMenuText::k1x1Str);
    setMenuItemText(pMenu, index, SToolsEraseMenuText::k2x2Str);
    setMenuItemText(pMenu, index, SToolsEraseMenuText::k4x4Str);
    setMenuItemText(pMenu, index, SToolsEraseMenuText::kFillStr);
}

VA(0x00459d37, 0x202)
void setToolsBrushMenuTexts(CMenu* pMenu)
{
    unsigned int index = getFirstMenuItem(pMenu);
    setMenuItemText(pMenu, index, SToolsBrushMenuText::k1x1SolidBrushStr);
    setMenuItemText(pMenu, index, SToolsBrushMenuText::k2x2SolidBrushStr);
    setMenuItemText(pMenu, index, SToolsBrushMenuText::k4x4SolidBrushStr);
    setMenuItemText(pMenu, index, SToolsBrushMenuText::kSolidAreaBrushStr);
    setMenuItemText(pMenu, index, SToolsBrushMenuText::k1x1FuzzyBrushStr);
    setMenuItemText(pMenu, index, SToolsBrushMenuText::k2x2FuzzyBrushStr);
    setMenuItemText(pMenu, index, SToolsBrushMenuText::k4x4FuzzyBrushStr);
    setMenuItemText(pMenu, index, SToolsBrushMenuText::kFuzzyAreaBrushStr);
    setMenuItemText(pMenu, index, SToolsBrushMenuText::k1x1SolidEraserStr);
    setMenuItemText(pMenu, index, SToolsBrushMenuText::k2x2SolidEraserStr);
    setMenuItemText(pMenu, index, SToolsBrushMenuText::k4x4SolidEraserStr);
    setMenuItemText(pMenu, index, SToolsBrushMenuText::kSolidAreaEraserStr);
    setMenuItemText(pMenu, index, SToolsBrushMenuText::k1x1FuzzyEraserStr);
    setMenuItemText(pMenu, index, SToolsBrushMenuText::k2x2FuzzyEraserStr);
    setMenuItemText(pMenu, index, SToolsBrushMenuText::k4x4FuzzyEraserStr);
    setMenuItemText(pMenu, index, SToolsBrushMenuText::kFuzzyAreaEraserStr);
    setMenuItemText(pMenu, index, SToolsBrushMenuText::kPlaceObstaclesStr);
}

VA(0x00459f39, 0x1c8)
void setToolsObjectsMenuTexts(CMenu* pMenu)
{
    unsigned int index = getFirstMenuItem(pMenu);
    setMenuItemText(pMenu, index, SToolsObjectsMenuText::kDirtObjectsStr);
    setMenuItemText(pMenu, index, SToolsObjectsMenuText::kSandObjectsStr);
    setMenuItemText(pMenu, index, SToolsObjectsMenuText::kGrassObjectsStr);
    setMenuItemText(pMenu, index, SToolsObjectsMenuText::kSnowObjectsStr);
    setMenuItemText(pMenu, index, SToolsObjectsMenuText::kSwampObjectsStr);
    setMenuItemText(pMenu, index, SToolsObjectsMenuText::kRoughObjectsStr);
    setMenuItemText(pMenu, index, SToolsObjectsMenuText::kSubterraneanObjectsStr);
    setMenuItemText(pMenu, index, SToolsObjectsMenuText::kLavaObjectsStr);
    setMenuItemText(pMenu, index, SToolsObjectsMenuText::kWaterObjectsStr);
    setMenuItemText(pMenu, index, SToolsObjectsMenuText::kAllTerrainObjectsStr);
    setMenuItemText(pMenu, index, SToolsObjectsMenuText::kTownsStr);
    setMenuItemText(pMenu, index, SToolsObjectsMenuText::kMonstersStr);
    setMenuItemText(pMenu, index, SToolsObjectsMenuText::kHeroesStr);
    setMenuItemText(pMenu, index, SToolsObjectsMenuText::kArtifactsStr);
    setMenuItemText(pMenu, index, SToolsObjectsMenuText::kTreasuresStr);
}

VA(0x0045a101, 0x26c)
void setPlayerMenuTexts(CMenu* pMenu)
{
    unsigned int index = getFirstMenuItem(pMenu);
    setMenuItemText(pMenu, index, SPlayerMenuText::kNoneStr);
    setMenuItemText(pMenu, index, TFormattedString(SPlayerMenuText::kPlayer1FmtStr, akPlayerTraits[0].m_pColorName));
    setMenuItemText(pMenu, index, TFormattedString(SPlayerMenuText::kPlayer2FmtStr, akPlayerTraits[1].m_pColorName));
    setMenuItemText(pMenu, index, TFormattedString(SPlayerMenuText::kPlayer3FmtStr, akPlayerTraits[2].m_pColorName));
    setMenuItemText(pMenu, index, TFormattedString(SPlayerMenuText::kPlayer4FmtStr, akPlayerTraits[3].m_pColorName));
    setMenuItemText(pMenu, index, TFormattedString(SPlayerMenuText::kPlayer5FmtStr, akPlayerTraits[4].m_pColorName));
    setMenuItemText(pMenu, index, TFormattedString(SPlayerMenuText::kPlayer6FmtStr, akPlayerTraits[5].m_pColorName));
    setMenuItemText(pMenu, index, TFormattedString(SPlayerMenuText::kPlayer7FmtStr, akPlayerTraits[6].m_pColorName));
    setMenuItemText(pMenu, index, TFormattedString(SPlayerMenuText::kPlayer8FmtStr, akPlayerTraits[7].m_pColorName));
}

VA(0x0045a36d, 0x51)
void setHelpMenuTexts(CMenu* pMenu)
{
    unsigned int index = getFirstMenuItem(pMenu);
    setMenuItemText(pMenu, index, SHelpMenuText::kHelpTopicsStr);
    setMenuItemText(pMenu, index, SHelpMenuText::kAboutMapEditorStr);
}

// A command's status bar prompt and tool tip.
typedef std::pair<const CString, const CString> TCommandTexts;

// Every command's texts by command id, from "editrcmd.txt".
class TCommandTextMap : public std::map<UINT, TCommandTexts> {
public:
    TCommandTextMap();
    ~TCommandTextMap() {}

private:
    void _add(UINT id, const TSpreadsheetResource& spreadsheet, int row);
};

// The row of each command's texts. The player commands share one row.
struct TCommandRow {
    UINT m_id;
    int m_row;
};

enum { kPlayerCommandRow = 70 };

DATA(0x0058b190) TCommandRow aCommandRows[] = {
    { AFX_IDS_IDLEMESSAGE, 2 },
    { ID_FILE_NEW, 3 },
    { ID_FILE_OPEN, 4 },
    { ID_FILE_SAVE, 5 },
    { ID_FILE_SAVE_AS, 6 },
    { ID_APP_ABOUT, 7 },
    { ID_APP_EXIT, 8 },
    { ID_HELP_FINDER, 9 },
    { ID_HELP, 10 },
    { ID_FILE_MRU_FILE1, 11 },
    { ID_FILE_MRU_FILE2, 11 },
    { ID_FILE_MRU_FILE3, 11 },
    { ID_FILE_MRU_FILE4, 11 },
    { ID_FILE_MRU_FILE5, 11 },
    { ID_FILE_MRU_FILE6, 11 },
    { ID_FILE_MRU_FILE7, 11 },
    { ID_FILE_MRU_FILE8, 11 },
    { ID_FILE_MRU_FILE9, 11 },
    { ID_FILE_MRU_FILE10, 11 },
    { ID_FILE_MRU_FILE11, 11 },
    { ID_FILE_MRU_FILE12, 11 },
    { ID_FILE_MRU_FILE13, 11 },
    { ID_FILE_MRU_FILE14, 11 },
    { ID_FILE_MRU_FILE15, 11 },
    { ID_FILE_MRU_FILE16, 11 },
    { ID_EDIT_COPY, 12 },
    { ID_EDIT_CUT, 13 },
    { ID_EDIT_FIND, 14 },
    { ID_EDIT_PASTE, 15 },
    { ID_EDIT_UNDO, 16 },
    { ID_EDIT_REDO, 17 },
    { AFX_IDS_SCSIZE, 18 },
    { AFX_IDS_SCMOVE, 19 },
    { AFX_IDS_SCMINIMIZE, 20 },
    { AFX_IDS_SCMAXIMIZE, 21 },
    { AFX_IDS_SCCLOSE, 22 },
    { AFX_IDS_SCRESTORE, 23 },
    { ID_VIEW_UNDERGROUND, 24 },
    { ID_VIEW_PASSABILITY, 25 },
    { ID_TOOLS_TERRAIN, 26 },
    { ID_TOOLS_RIVERS, 27 },
    { ID_TOOLS_ROADS, 28 },
    { ID_TOOLS_ERASE, 29 },
    { ID_TOOLS_OBJECTS_DIRT, 30 },
    { ID_TOOLS_OBJECTS_SAND, 31 },
    { ID_TOOLS_OBJECTS_GRASS, 32 },
    { ID_TOOLS_OBJECTS_SNOW, 33 },
    { ID_TOOLS_OBJECTS_SWAMP, 34 },
    { ID_TOOLS_OBJECTS_ROUGH, 35 },
    { ID_TOOLS_OBJECTS_SUBTERRANEAN, 36 },
    { ID_TOOLS_OBJECTS_LAVA, 37 },
    { ID_TOOLS_OBJECTS_WATER, 38 },
    { ID_TOOLS_OBJECTS_TOWNS, 39 },
    { ID_TOOLS_OBJECTS_MONSTERS, 40 },
    { ID_TOOLS_OBJECTS_HEROES, 41 },
    { ID_TOOLS_OBJECTS_ARTIFACTS, 42 },
    { ID_TOOLS_OBJECTS_TREASURES, 43 },
    { ID_VIEW_OBJECT_ANIMATION, 44 },
    { ID_TOOLS_MAP_SPECIFICATIONS, 45 },
    { ID_TOOLS_TERRAIN_1X1, 46 },
    { ID_TOOLS_TERRAIN_2X2, 47 },
    { ID_TOOLS_TERRAIN_4X4, 48 },
    { ID_TOOLS_TERRAIN_FILL, 49 },
    { ID_TOOLS_TERRAIN_DIRT, 50 },
    { ID_TOOLS_TERRAIN_SAND, 51 },
    { ID_TOOLS_TERRAIN_GRASS, 52 },
    { ID_TOOLS_TERRAIN_SNOW, 53 },
    { ID_TOOLS_TERRAIN_SWAMP, 54 },
    { ID_TOOLS_TERRAIN_ROUGH, 55 },
    { ID_TOOLS_TERRAIN_SUBTERRANEAN, 56 },
    { ID_TOOLS_TERRAIN_LAVA, 57 },
    { ID_TOOLS_TERRAIN_WATER, 58 },
    { ID_TOOLS_RIVERS_CLEAR, 59 },
    { ID_TOOLS_RIVERS_ICY, 60 },
    { ID_TOOLS_RIVERS_MUDDY, 61 },
    { ID_TOOLS_RIVERS_LAVA, 62 },
    { ID_TOOLS_ROADS_GRAVEL, 63 },
    { ID_TOOLS_ROADS_COBBLESTONE, 64 },
    { ID_TOOLS_ERASE_1X1, 65 },
    { ID_TOOLS_ERASE_2X2, 66 },
    { ID_TOOLS_ERASE_4X4, 67 },
    { ID_TOOLS_ERASE_FILL, 68 },
    { ID_PLAYERS_NONE, 69 },
    { ID_TOOLS_ROADS_DIRT, 71 },
    { ID_TOOLS_TERRAIN_ROCK, 72 },
    { ID_EDIT_DELETE, 73 },
    { ID_EDIT_PROPERTIES, 74 },
    { ID_VIEW_GRID, 75 },
    { ID_TOOLS_OBJECTS_ALL_TERRAIN, 76 },
    { ID_TOOLS_BRUSH_1X1, 77 },
    { ID_TOOLS_BRUSH_2X2, 78 },
    { ID_TOOLS_BRUSH_4X4, 79 },
    { ID_TOOLS_BRUSH_FILL, 80 },
    { ID_TOOLS_VALIDATE_MAP, 81 },
    { ID_VIEW_TERRAIN_ANIMATION, 82 },
    { ID_TOOLS_OPTIONS, 83 },
    { ID_FILE_EXPORT_TEXT, 84 },
    { ID_FILE_IMPORT_TEXT, 85 },
    { ID_HELP_WHATS_THIS, 86 },
    { ID_EDIT_FIND_NEXT, 87 },
    { ID_EDIT_FIND_PREV, 88 },
    { ID_VIEW_ZOOM_OUT, 89 },
    { ID_VIEW_ZOOM_IN, 90 },
    { ID_VIEW_TOOLBAR, 91 },
    { ID_VIEW_STATUS_BAR, 92 },
    { ID_VIEW_REBAR, 93 },
    { ID_TOOLS_OBSTACLES, 94 },
    { ID_TOOLS_OBSTACLES_AREA_1X1, 95 },
    { ID_TOOLS_OBSTACLES_AREA_2X2, 96 },
    { ID_TOOLS_OBSTACLES_AREA_4X4, 97 },
    { ID_TOOLS_OBSTACLES_AREA_FILL, 98 },
    { ID_TOOLS_OBSTACLES_BORDERED_AREA_1X1, 99 },
    { ID_TOOLS_OBSTACLES_BORDERED_AREA_2X2, 100 },
    { ID_TOOLS_OBSTACLES_BORDERED_AREA_4X4, 101 },
    { ID_TOOLS_OBSTACLES_BORDERED_AREA_FILL, 102 },
    { ID_TOOLS_OBSTACLES_ERASE_1X1, 103 },
    { ID_TOOLS_OBSTACLES_ERASE_2X2, 104 },
    { ID_TOOLS_OBSTACLES_ERASE_4X4, 105 },
    { ID_TOOLS_OBSTACLES_ERASE_FILL, 106 },
    { ID_TOOLS_OBSTACLES_BORDERED_ERASE_1X1, 107 },
    { ID_TOOLS_OBSTACLES_BORDERED_ERASE_2X2, 108 },
    { ID_TOOLS_OBSTACLES_BORDERED_ERASE_4X4, 109 },
    { ID_TOOLS_OBSTACLES_BORDERED_ERASE_FILL, 110 },
    { ID_TOOLS_OBSTACLES_PLACE, 111 },
    { ID_TOOLS_RIVERS_ERASE, 112 },
    { ID_TOOLS_ROADS_ERASE, 113 }
};

VA(0x0045a3be, 0x212)
TCommandTextMap::TCommandTextMap()
{
    TResourcePtr<TSpreadsheetResource> pSpreadsheet(ResourceManager::GetSpreadsheet("editrcmd.txt"));
    if (pSpreadsheet.get() == NULL)
        throw TRuntimeError();
    for (const TCommandRow* pRow = aCommandRows; pRow < aCommandRows + sizeof(aCommandRows) / sizeof(aCommandRows[0]);
         ++pRow)
        _add(pRow->m_id, *pSpreadsheet.get(), pRow->m_row);
    const UINT aPlayerCommands[kNumPlayers] = {
        ID_PLAYERS_PLAYER1, ID_PLAYERS_PLAYER2, ID_PLAYERS_PLAYER3, ID_PLAYERS_PLAYER4,
        ID_PLAYERS_PLAYER5, ID_PLAYERS_PLAYER6, ID_PLAYERS_PLAYER7, ID_PLAYERS_PLAYER8
    };
    const TSpreadsheetResource::TStringVector& cells = pSpreadsheet->GetRow(kPlayerCommandRow);
    CString toolTip(cells.size() > 1 ? cells[1] : "");
    const char* promptFormat = cells[0];
    for (unsigned int player = 0; player < kNumPlayers; ++player) {
        TFormattedString prompt(promptFormat, akPlayerTraits[player].m_pColorName);
        insert(value_type(aPlayerCommands[player], make_pair(prompt, toolTip)));
    }
}

// The texts' pair temporaries: make_pair's pair<TFormattedString, CString>,
// its conversion to the mapped pair<const CString, const CString>, and the
// value_type; _add's make_pair builds a pair<CString, CString>.
VA_COMPGEN(0x0045a5d0, 0x35, IMPLICIT_DTOR, pair)
VA_COMPGEN(0x0045a605, 0x35, IMPLICIT_DTOR, pair)
VA_COMPGEN(0x0045a63a, 0x36, IMPLICIT_DTOR, pair)

VA(0x0045a670, 0x141)
void TCommandTextMap::_add(UINT id, const TSpreadsheetResource& spreadsheet, int row)
{
    const TSpreadsheetResource::TStringVector& cells = spreadsheet.GetRow(row);
    CString prompt(cells[0]);
    CString toolTip(cells.size() > 1 ? cells[1] : "");
    insert(value_type(id, make_pair(prompt, toolTip)));
}

VA_COMPGEN(0x0045a7b1, 0x35, IMPLICIT_DTOR, pair)

inline const TCommandTextMap& getCommandTexts();

}  // namespace

VA(0x0045a7e6, 0x35)
IMPLEMENT_DYNCREATE(TMainFrame, CFrameWnd)
VA_COMPGEN(0x0045a81b, 0x6, RUNTIME_CLASS_GETTER, TMainFrame)

VA(0x0045a821, 0x6)
BEGIN_MESSAGE_MAP(TMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_GETMINMAXINFO()
    ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
    ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
    ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
    ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
    ON_MESSAGE(WM_DISPLAYCHANGE, OnDisplayChange)
    ON_WM_SYSCOMMAND()
    ON_WM_QUERYOPEN()
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXT, 0, 0xFFFF, OnToolTipText)
END_MESSAGE_MAP()

// The object name, player and map size panes; the first is the prompt.
DATA(0x0059fcb0) static UINT indicators[] = {
    ID_SEPARATOR,
    ID_SEPARATOR,
    ID_SEPARATOR,
    ID_SEPARATOR
};

VA(0x0045a827, 0x7c)
TMainFrame::TMainFrame()
    : _m_bFirstActivation(true)
{
    getCommandTexts();
}

namespace {

// Retail tests/sets bit 0 at 0x5aa650 around construction of commandTexts.
VA(0x0045a8a3, 0x2b)
inline const TCommandTextMap& getCommandTexts()
{
    DATA_COMPGEN_GUARD(0x005aa650, commandTextsGuard, commandTexts)
    DATA(0x005aa660) static const TCommandTextMap commandTexts;
    return commandTexts;
}

}  // namespace

VA(0x0045a8f9, 0x69)
TMainFrame::~TMainFrame()
{
}

// VC6 probes: reversing the final width-add operands or splitting += was
// byte-flat; a named CSize also retained its EAX/ECX mismatch (99.9941%).
VA(0x0045a962, 0x3e7)
int TMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;
    m_strTitle = kAppTitleStr;
    SetWindowText(m_strTitle);
    CMenu* pMenu = CMenu::FromHandle(::GetMenu(m_hWnd));
    setMainMenuTexts(pMenu);
    setFileMenuTexts(pMenu->GetSubMenu(0));
    setEditMenuTexts(pMenu->GetSubMenu(1));
    setViewMenuTexts(pMenu->GetSubMenu(2));
    CMenu* pToolsMenu = pMenu->GetSubMenu(3);
    setToolsMenuTexts(pToolsMenu);
    setToolsTerrainMenuTexts(pToolsMenu->GetSubMenu(0));
    setToolsRiversMenuTexts(pToolsMenu->GetSubMenu(1));
    setToolsRoadsMenuTexts(pToolsMenu->GetSubMenu(2));
    setToolsEraseMenuTexts(pToolsMenu->GetSubMenu(3));
    setToolsBrushMenuTexts(pToolsMenu->GetSubMenu(4));
    setToolsObjectsMenuTexts(pToolsMenu->GetSubMenu(5));
    setPlayerMenuTexts(pMenu->GetSubMenu(4));
    setHelpMenuTexts(pMenu->GetSubMenu(5));
    if (!_m_toolBar.Create(this) || !_m_toolBar.LoadToolBar(IDR_MAINFRAME))
        return -1;
    if (!_m_modeBar.Create(this, WS_CHILD | WS_VISIBLE | CBRS_TOP, AFX_IDW_REBAR) ||
        !_m_modeBar.LoadToolBar(IDR_MODE_BAR))
        return -1;
    if (!_m_statusBar.Create(this) || !_m_statusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT)))
        return -1;
    _m_toolBar.SetBarStyle(_m_toolBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
    _m_toolBar.SetWindowText(kToolbarCaptionStr);
    _m_modeBar.SetBarStyle(_m_modeBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
    _m_modeBar.SetWindowText(kModeBarCaptionStr);
    _m_toolBar.EnableDocking(CBRS_ALIGN_ANY);
    _m_modeBar.EnableDocking(CBRS_ALIGN_ANY);
    EnableDocking(CBRS_ALIGN_ANY);
    DockControlBar(&_m_toolBar);
    DockControlBar(&_m_modeBar);
    CClientDC dc(NULL);
    TGDIObjectSelector<CFont> fontSelector(&dc, _m_statusBar.GetFont());
    TEXTMETRIC metrics;
    dc.GetTextMetrics(&metrics);
    _m_objectNamePaneWidth = metrics.tmAveCharWidth * 35;
    _m_statusBar.SetPaneInfo(1, 0, SBPS_NOBORDERS, 0);
    int playerWidth = dc.GetTextExtent(kPlayerNoneStr).cx;
    for (unsigned int player = 0; player < kNumPlayers; ++player) {
        int width = dc.GetTextExtent(akPlayerTraits[player].m_pName).cx;
        if (width > playerWidth)
            playerWidth = width;
    }
    _m_statusBar.SetPaneInfo(2, 0, 0, playerWidth + metrics.tmAveCharWidth);
    int digitWidth = 0;
    int widestDigit = 0;
    for (char digit = '0'; digit <= '9'; ++digit) {
        int width = dc.GetTextExtent(&digit, 1).cx;
        if (width > digitWidth) {
            digitWidth = width;
            widestDigit = digit - '0';
        }
    }
    char mapSize[12];
    sprintf(mapSize, "%03dx%03dx%01d", widestDigit * 111, widestDigit * 111, widestDigit);
    int mapSizeWidth = dc.GetTextExtent(mapSize, 9).cx + metrics.tmAveCharWidth;
    _m_statusBar.SetPaneInfo(3, 0, 0, mapSizeWidth);
    return 0;
}

VA(0x0045ad5f, 0x175)
void TMainFrame::ActivateFrame(int nCmdShow)
{
    if (_m_bFirstActivation) {
        _m_bFirstActivation = false;
        CWinApp* pApp = AfxGetApp();
        BYTE* pData;
        UINT size;
        if (pApp->GetProfileBinary("Layout", "WindowPlacement", &pData, &size)) {
            std::auto_ptr<BYTE> apData(pData);
            WINDOWPLACEMENT* pPlacement = reinterpret_cast<WINDOWPLACEMENT*>(pData);
            if (nCmdShow == SW_SHOWNORMAL)
                nCmdShow = pPlacement->showCmd;
            pPlacement->showCmd = SW_HIDE;
            SetWindowPlacement(pPlacement);
        }
        if (pApp->GetProfileBinary("Layout", "ControlBars", &pData, &size)) {
            std::auto_ptr<BYTE> apData(pData);
            CMemFile file(pData, size);
            CArchive archive(&file, CArchive::load);
            CDockState dockState;
            dockState.Serialize(archive);
            archive.Close();
            SetDockState(dockState);
        }
    }
    CFrameWnd::ActivateFrame(nCmdShow);
}

VA(0x0045aed4, 0x145)
BOOL TMainFrame::DestroyWindow()
{
    CWinApp* pApp = AfxGetApp();
    CDockState dockState;
    GetDockState(dockState);
    CMemFile file;
    CArchive archive(&file, CArchive::store);
    dockState.Serialize(archive);
    archive.Close();
    UINT size = file.GetLength();
    BYTE* pData = file.Detach();
    try {
        pApp->WriteProfileBinary("Layout", "ControlBars", pData, size);
    } catch (...) {
        free(pData);
        throw;
    }
    free(pData);
    WINDOWPLACEMENT placement;
    placement.length = sizeof(placement);
    if (GetWindowPlacement(&placement))
        pApp->WriteProfileBinary("Layout", "WindowPlacement", reinterpret_cast<BYTE*>(&placement), sizeof(placement));
    return CFrameWnd::DestroyWindow();
}

VA(0x0045b019, 0x53)
BOOL TMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CFrameWnd::PreCreateWindow(cs))
        return FALSE;
    WNDCLASS wndClass;
    if (cs.lpszClass != NULL && ::GetClassInfo(AfxGetInstanceHandle(), cs.lpszClass, &wndClass))
        cs.lpszClass = AfxRegisterWndClass(wndClass.style & ~(CS_HREDRAW | CS_VREDRAW), wndClass.hCursor,
                                           wndClass.hbrBackground, wndClass.hIcon);
    return TRUE;
}

VA(0x0045b06c, 0x15)
void TMainFrame::showObjectName()
{
    _m_statusBar.SetPaneInfo(1, 0, 0, _m_objectNamePaneWidth);
}

VA(0x0045b081, 0x28)
void TMainFrame::hideObjectName()
{
    _m_statusBar.SetPaneText(1, "");
    _m_statusBar.SetPaneInfo(1, 0, SBPS_NOBORDERS, 0);
}

VA(0x0045b0a9, 0x13)
void TMainFrame::setObjectName(const char* name)
{
    _m_statusBar.SetPaneText(1, name);
}

VA(0x0045b0bc, 0x2b)
void TMainFrame::setCurPlayer(TPlayer player)
{
    _m_statusBar.SetPaneText(2, player != ePlayerNone ? akPlayerTraits[player].m_pName : kPlayerNoneStr);
}

VA(0x0045b0e7, 0x3f)
void TMainFrame::setMapSize(unsigned int width, unsigned int height, bool bTwoLayer)
{
    char text[12];
    sprintf(text, "%dx%dx%d", width, height, bTwoLayer ? 2 : 1);
    _m_statusBar.SetPaneText(3, text);
}

VA(0x0045b126, 0x4c)
BOOL TMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
    if (pContext != NULL && pContext->m_pNewViewClass != NULL) {
        TMapView* pView = static_cast<TMapView*>(CreateView(pContext, AFX_IDW_PANE_FIRST));
        if (pView == NULL)
            return FALSE;
        pView->setStatusUI(this);
        ModifyStyleEx(WS_EX_CLIENTEDGE, 0, SWP_FRAMECHANGED);
    }
    return TRUE;
}

VA(0x0045b172, 0x1b)
LRESULT TMainFrame::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
    SendMessage(WM_COMMAND, MAKEWPARAM(ID_HELP_FINDER, 1));
    return TRUE;
}

VA(0x0045b18d, 0x31)
LRESULT TMainFrame::OnDisplayChange(WPARAM wParam, LPARAM lParam)
{
    if (int(wParam) < 15 && !IsIconic())
        SendMessage(WM_SYSCOMMAND, SC_MINIMIZE);
    return 0;
}

VA(0x0045b1be, 0x86)
void TMainFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
    if (nID == SC_RESTORE || nID == SC_MAXIMIZE) {
        CClientDC dc(NULL);
        if (dc.GetDeviceCaps(BITSPIXEL) < 16) {
            AfxMessageBox(kNeed16BitColorStr);
            return;
        }
    }
    CFrameWnd::OnSysCommand(nID, lParam);
}

VA(0x0045b244, 0x43)
BOOL TMainFrame::OnQueryOpen()
{
    CClientDC dc(NULL);
    int bits = dc.GetDeviceCaps(BITSPIXEL);
    int planes = dc.GetDeviceCaps(PLANES);
    BOOL bOpen = bits * planes >= 15;
    return bOpen;
}

VA(0x0045b287, 0x8)
void TMainFrame::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    CFrameWnd::OnGetMinMaxInfo(lpMMI);
}

VA(0x0045b28f, 0x36)
void TMainFrame::GetMessageString(UINT nID, CString& rMessage) const
{
    TCommandTextMap::const_iterator it = getCommandTexts().find(nID);
    if (it != getCommandTexts().end())
        rMessage = it->second.first;
}

VA(0x0045b2c5, 0x58)
BOOL TMainFrame::OnToolTipText(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
    TOOLTIPTEXT* pText = reinterpret_cast<TOOLTIPTEXT*>(pNMHDR);
    TCommandTextMap::const_iterator it = getCommandTexts().find(pNMHDR->idFrom);
    if (it == getCommandTexts().end())
        return FALSE;
    lstrcpyn(pText->szText, it->second.second, sizeof(pText->szText));
    ::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
                   SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOOWNERZORDER);
    return TRUE;
}
