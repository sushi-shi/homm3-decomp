// MainFrame.cpp - the campaign editor's main frame (h3ccmped
// 0x426920..0x428ab0). CmpEdCmd.txt gives each command its status bar
// prompt (column 0) and its tool tip (column 1), one row per command.
#include "campaign_editor/stdafx.h"

#include <map>
#include <utility>

#include "va.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "textresource.h"
#include "editor/AfxPrivMessages.h"
#include "editor/GDIObjectSelector.h"
#include "editor/MemoryDC.h"
#include "campaign_editor/CampaignEditorText.h"
#include "campaign_editor/MainFrame.h"
#include "campaign_editor/resource.h"

VA(0x00426950, 0x78)
static void setMainMenuText(CMenu* pMenu)
{
    unsigned int index = getFirstMenuItem(pMenu);
    setMenuItemText(pMenu, index, SMainMenuText::kFileStr);
    setMenuItemText(pMenu, index, SMainMenuText::kEditStr);
    setMenuItemText(pMenu, index, SMainMenuText::kHelpStr);
}

VA(0x004269d0, 0x166)
static void setFileMenuText(CMenu* pMenu)
{
    unsigned int index = getFirstMenuItem(pMenu);
    setMenuItemText(pMenu, index, SFileMenuText::kNewStr);
    setMenuItemText(pMenu, index, SFileMenuText::kOpenStr);
    setMenuItemText(pMenu, index, SFileMenuText::kSaveStr);
    setMenuItemText(pMenu, index, SFileMenuText::kSaveAsStr);
    setMenuItemText(pMenu, index, SFileMenuText::kExportTextStr);
    setMenuItemText(pMenu, index, SFileMenuText::kImportTextStr);
    setMenuItemText(pMenu, index, SFileMenuText::kRefreshMapsStr);
    setMenuItemText(pMenu, index, SFileMenuText::kExportMapsStr);
    setMenuItemText(pMenu, index, SFileMenuText::kRecentFilesStr);
    setMenuItemText(pMenu, index, SFileMenuText::kExitStr);
}

VA(0x00426b40, 0x56)
static void setEditMenuText(CMenu* pMenu)
{
    unsigned int index = getFirstMenuItem(pMenu);
    setMenuItemText(pMenu, index, SEditMenuText::kCampaignPropertiesStr);
    setMenuItemText(pMenu, index, SEditMenuText::kScenarioPropertiesStr);
}

VA(0x00426ba0, 0x56)
static void setHelpMenuText(CMenu* pMenu)
{
    unsigned int index = getFirstMenuItem(pMenu);
    setMenuItemText(pMenu, index, SHelpMenuText::kHelpTopicsStr);
    setMenuItemText(pMenu, index, SHelpMenuText::kAboutStr);
}

// A command's row in CmpEdCmd.txt.
struct TCommandRow {
    UINT m_id;
    int m_row;
};

DATA(0x004bcff8) static TCommandRow akCommandRows[] = {
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
    { AFX_IDS_SCSIZE, 12 },
    { AFX_IDS_SCMOVE, 13 },
    { AFX_IDS_SCMINIMIZE, 14 },
    { AFX_IDS_SCMAXIMIZE, 15 },
    { AFX_IDS_SCCLOSE, 16 },
    { AFX_IDS_SCRESTORE, 17 },
    { ID_FILE_REFRESH_SCENARIO_MAPS, 18 },
    { ID_FILE_EXPORT_SCENARIO_MAPS, 19 },
    { ID_CAMPAIGN_PROPERTIES, 20 },
    { ID_SCENARIO_PROPERTIES, 21 },
    { ID_FILE_EXPORT_TEXT, 22 },
    { ID_FILE_IMPORT_TEXT, 23 }
};

// The prompt and the tool tip of each command.
class TCommandTexts : public std::map<UINT, std::pair<const CString, const CString> > {
public:
    TCommandTexts();

private:
    void _add(UINT id, const TSpreadsheetResource* pSpreadsheet, int row);
};

VA(0x00426c00, 0x129)
TCommandTexts::TCommandTexts()
{
    TResourcePtr<TSpreadsheetResource> pSpreadsheet(ResourceManager::GetSpreadsheet("CmpEdCmd.txt"));
    if (pSpreadsheet.get() == NULL)
        throw TRuntimeError();
    for (const TCommandRow* pRow = akCommandRows; pRow < akCommandRows + sizeof(akCommandRows) / sizeof(akCommandRows[0]);
         pRow++)
        _add(pRow->m_id, pSpreadsheet.get(), pRow->m_row);
}

VA(0x00426d30, 0x182)
void TCommandTexts::_add(UINT id, const TSpreadsheetResource* pSpreadsheet, int row)
{
    const TSpreadsheetResource::TStringVector& cells = pSpreadsheet->GetRow(row);
    CString prompt(cells[0]);
    CString toolTip(cells.size() > 1 ? cells[1] : "");
    insert(value_type(id, std::make_pair(prompt, toolTip)));
}

static inline const TCommandTexts& getCommandTexts()
{
    DATA_COMPGEN_GUARD(0x004ce284, commandTextsGuard, commandTexts)
    // Retail loads this static object's address, then tail-calls its destructor.
    VA_COMPGEN(0x004270e0, 0xa, STATIC_DTOR, commandTexts)
    DATA(0x004ce290) static TCommandTexts commandTexts;
    return commandTexts;
}

VA(0x00426fb0, 0x5a)
IMPLEMENT_DYNCREATE(TMainFrame, CFrameWnd)
VA_COMPGEN(0x00427010, 0x6, RUNTIME_CLASS_GETTER, TMainFrame)

VA(0x00427020, 0x6)
BEGIN_MESSAGE_MAP(TMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_GETMINMAXINFO()
    ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
    ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
    ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
    ON_WM_WINDOWPOSCHANGING()
    ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
    ON_MESSAGE(WM_DISPLAYCHANGE, OnDisplayChange)
    ON_WM_SYSCOMMAND()
    ON_WM_QUERYOPEN()
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXT, 0, 0xFFFF, OnToolTipText)
END_MESSAGE_MAP()

VA(0x00427030, 0xab)
TMainFrame::TMainFrame()
    : _m_pView(NULL)
{
    getCommandTexts();
}

// The static cleanup and constructor unwind share this 108-byte body with
// the base map destructor; all fixed bytes and nine relocation targets agree.
VA_COMPGEN(0x004270f0, 0x6c, IMPLICIT_DTOR, TCommandTexts)

VA_COMPGEN(0x00427160, 0x1e, SCALAR_DELETING_DTOR, TMainFrame)

VA(0x00427180, 0x6f)
TMainFrame::~TMainFrame()
{
}

VA(0x004271f0, 0x96)
CSize TMainFrame::_getMaxSize()
{
    CSize size = _m_pView->getMapSize();
    size.cx += 2 * ::GetSystemMetrics(SM_CXFRAME);
    size.cy += 2 * ::GetSystemMetrics(SM_CYFRAME) + ::GetSystemMetrics(SM_CYMENU) + ::GetSystemMetrics(SM_CYCAPTION);
    CRect toolBarRect;
    _m_toolBar.GetWindowRect(&toolBarRect);
    CRect statusBarRect;
    _m_statusBar.GetWindowRect(&statusBarRect);
    size.cy += toolBarRect.Height() + statusBarRect.Height();
    return size;
}

// Retail retains the metric for both pane updates. A LONG snapshot before the
// pane query recovers its register and stack layout; after-query snapshots and
// a split declaration/assignment do not. Retail's later metric load remains.
VA(0x00427290, 0x30d)
int TMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    DATA(0x004c82fc) static UINT indicators[] = { ID_SEPARATOR, ID_SEPARATOR };
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;
    m_strTitle = kAppTitleStr;
    SetWindowText(m_strTitle);
    CMenu* pMenu = GetMenu();
    setMainMenuText(pMenu);
    setFileMenuText(pMenu->GetSubMenu(0));
    setEditMenuText(pMenu->GetSubMenu(1));
    setHelpMenuText(pMenu->GetSubMenu(2));
    if (!_m_toolBar.Create(this) || !_m_toolBar.LoadToolBar(IDR_MAINFRAME))
        return -1;
    if (!_m_statusBar.Create(this) || !_m_statusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT)))
        return -1;
    _m_toolBar.SetBarStyle(_m_toolBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
    RecalcLayout();
    CRect windowRect;
    GetWindowRect(&windowRect);
    CSize maxSize = _getMaxSize();
    if (windowRect.Width() > maxSize.cx || windowRect.Height() > maxSize.cy) {
        if (windowRect.Width() > maxSize.cx)
            windowRect.right = windowRect.left + maxSize.cx;
        if (windowRect.Height() > maxSize.cy)
            windowRect.bottom = windowRect.top + maxSize.cy;
        CWnd* pParent = GetParent();
        if (pParent != NULL)
            pParent->ScreenToClient(&windowRect);
        MoveWindow(&windowRect);
    }
    CClientDC dc(NULL);
    TGDIObjectSelector<CFont> selector(&dc, _m_statusBar.GetFont());
    TEXTMETRIC textMetric;
    dc.GetTextMetrics(&textMetric);
    const LONG averageCharWidth = textMetric.tmAveCharWidth;
    UINT id;
    UINT style;
    int width;
    _m_statusBar.GetPaneInfo(0, id, style, width);
    _m_statusBar.SetPaneInfo(0, id, style, min(width, 35 * averageCharWidth));
    _m_statusBar.SetPaneInfo(1, 0, 0, 30 * averageCharWidth);
    return 0;
}

VA(0x004275a0, 0x13)
BOOL TMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CFrameWnd::PreCreateWindow(cs))
        return FALSE;
    return TRUE;
}

VA(0x004275c0, 0x16)
void TMainFrame::setScenarioName(const CString& name)
{
    _m_statusBar.SetPaneText(1, name);
}

VA(0x004275e0, 0x4f)
BOOL TMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
    CWnd* pView = CreateView(pContext);
    if (pView == NULL)
        return FALSE;
    _m_pView = (TCampaignView*)pView;
    _m_pView->setStatusUI(this);
    ModifyStyleEx(WS_EX_CLIENTEDGE, 0, SWP_FRAMECHANGED);
    return TRUE;
}

VA(0x00427630, 0x61)
void TMainFrame::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    CFrameWnd::OnGetMinMaxInfo(lpMMI);
    if (_m_pView != NULL) {
        CSize maxSize = _getMaxSize();
        lpMMI->ptMaxSize.x = min(lpMMI->ptMaxSize.x, maxSize.cx);
        lpMMI->ptMaxSize.y = min(lpMMI->ptMaxSize.y, maxSize.cy);
        lpMMI->ptMaxTrackSize.x = min(lpMMI->ptMaxTrackSize.x, maxSize.cx);
        lpMMI->ptMaxTrackSize.y = min(lpMMI->ptMaxTrackSize.y, maxSize.cy);
    }
}

VA(0x004276a0, 0x57)
void TMainFrame::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
    CFrameWnd::OnWindowPosChanging(lpwndpos);
    if (!(lpwndpos->flags & SWP_NOSIZE) && lpwndpos->cx > 0 && lpwndpos->cy > 0) {
        CSize maxSize = _getMaxSize();
        lpwndpos->cx = min(lpwndpos->cx, maxSize.cx);
        lpwndpos->cy = min(lpwndpos->cy, maxSize.cy);
    }
}

VA(0x00427700, 0x1e)
LRESULT TMainFrame::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
    SendMessage(WM_COMMAND, MAKEWPARAM(ID_HELP_FINDER, 1));
    return TRUE;
}

VA(0x00427720, 0x35)
LRESULT TMainFrame::OnDisplayChange(WPARAM wParam, LPARAM lParam)
{
    if ((int)wParam < 15 && !IsIconic())
        SendMessage(WM_SYSCOMMAND, SC_MINIMIZE);
    return 0;
}

VA(0x00427760, 0xd4)
void TMainFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
    if (nID == SC_RESTORE || nID == SC_MAXIMIZE) {
        CClientDC dc(NULL);
        if (dc.GetDeviceCaps(BITSPIXEL) * dc.GetDeviceCaps(PLANES) < 15) {
            AfxMessageBox(kDisplayDepthErrorStr);
            return;
        }
    }
    CFrameWnd::OnSysCommand(nID, lParam);
}

VA(0x00427840, 0x48)
BOOL TMainFrame::OnQueryOpen()
{
    CClientDC dc(NULL);
    BOOL result;
    if (dc.GetDeviceCaps(BITSPIXEL) * dc.GetDeviceCaps(PLANES) >= 15)
        result = TRUE;
    else
        result = FALSE;
    return result;
}

VA(0x00427890, 0xc2)
void TMainFrame::GetMessageString(UINT nID, CString& rMessage) const
{
    TCommandTexts::const_iterator it = getCommandTexts().find(nID);
    if (it != getCommandTexts().end())
        rMessage = it->second.first;
}

VA(0x00427960, 0xec)
BOOL TMainFrame::OnToolTipText(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
    TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
    TCommandTexts::const_iterator it = getCommandTexts().find(pNMHDR->idFrom);
    if (it == getCommandTexts().end())
        return FALSE;
    lstrcpyn(pTTT->szText, it->second.second, sizeof(pTTT->szText));
    ::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOOWNERZORDER);
    return TRUE;
}
