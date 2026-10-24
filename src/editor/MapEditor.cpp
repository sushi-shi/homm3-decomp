// MapEditor.cpp - the map editor application ("Map Editor.cpp", h3maped
// 0x45c08d..0x45d627; GOG only). InitInstance finds the editor's directory
// (the one holding Data, else the current one, and then the shell file
// types are not registered), opens the resources, loads the texts and the
// start-up tables in Loki's init_objects order, refuses to run below
// 16-bit colour, and starts the watchdog thread once the frame is up. The
// idle loop autosaves and steps the object and terrain animations; the
// View and Tools handlers keep the options and the views in step. The
// document template, the splash window and the about box are the file's
// own classes.
#include "editor/stdafx.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <exception>
#include <memory>

#include "va.h"
#include "herodefs.h"
#include "log.h"
#include "objnames.h"
#include "resourcemanager.h"
#include "spelldefs.h"
#include "editor/AfxPrivMessages.h"
#include "editor/Army.h"
#include "editor/Colors.h"
#include "editor/FormattedString.h"
#include "editor/GameResource.h"
#include "editor/GDIObjectSelector.h"
#include "editor/Generator.h"
#include "editor/Hero.h"
#include "editor/MainFrame.h"
#include "editor/MapDoc.h"
#include "editor/MapEditor.h"
#include "editor/MapEditorText.h"
#include "editor/MapView.h"
#include "editor/MemoryDC.h"
#include "editor/ObjectSpecializations.h"
#include "editor/ObjectSprites.h"
#include "editor/ObjectTypeTable.h"
#include "editor/Options.h"
#include "editor/OptionsDlg.h"
#include "editor/Player.h"
#include "editor/resource.h"
#include "editor/T16bppBitmap.h"
#include "editor/Tile.h"
#include "editor/Town.h"
#include "editor/WorldDirections.h"

// The game's table initializers, declared where its own start-up calls
// them (kb.cpp).
unsigned char InitializeCreatureTypeTraitsTable();
bool InitializeArtifactTraitsTable();

namespace {

// The editor's product version, from its version resource.
class TProductVersion : public CString {
public:
    TProductVersion();
};

VA(0x0045c08d, 0xf2)
TProductVersion::TProductVersion()
{
    char fileName[MAX_PATH + 1];
    if (GetModuleFileName(NULL, fileName, sizeof(fileName)) > 0) {
        DWORD handle;
        DWORD size = GetFileVersionInfoSize(fileName, &handle);
        if (size > 0) {
            std::auto_ptr<char> pVersionInfo(new char[size]);
            if (pVersionInfo.get() != NULL && GetFileVersionInfo(fileName, handle, size, pVersionInfo.get())) {
                char* pValue;
                UINT length;
                if (VerQueryValue(pVersionInfo.get(), "\\StringFileInfo\\040904B0\\ProductVersion",
                                  reinterpret_cast<void**>(&pValue), &length) ||
                    VerQueryValue(pVersionInfo.get(), "\\StringFileInfo\\040904e4\\ProductVersion",
                                  reinterpret_cast<void**>(&pValue), &length)) {
                    char* pBuffer = GetBuffer(length);
                    strcpy(pBuffer, pValue);
                    ReleaseBuffer();
                }
            }
        }
    }
}

// The resource manager over the editor's Data directory, open for as long
// as the editor runs.
class TResourceManagerSession {
public:
    TResourceManagerSession(const CString& path);
    ~TResourceManagerSession() { ResourceManager::Close(); }
};

VA(0x0045c17f, 0x66)
TResourceManagerSession::TResourceManagerSession(const CString& path)
{
    ResourceManager::SetPath(path + "Data\\");
    ResourceManager::Open(true, true, NULL);
    ResourceManager::SetPixelFormat(T16bppBitmapBase<ubyte>::redMask(), T16bppBitmapBase<ubyte>::greenMask(),
                                    T16bppBitmapBase<ubyte>::blueMask());
}

// The single document template: it names the map files from editor.txt
// and asks for a new map's parameters when no file is opened.
class TMapDocTemplate : public CSingleDocTemplate {
public:
    TMapDocTemplate();

    virtual BOOL GetDocString(CString& rString, enum DocStringIndex index) const;
    virtual CDocument* CreateNewDocument();
    virtual CDocument* OpenDocumentFile(LPCTSTR lpszPathName, BOOL bMakeVisible = TRUE);

private:
    TMapDoc* _m_pDocument;
};

VA(0x0045c1e5, 0x2a)
TMapDocTemplate::TMapDocTemplate()
    : CSingleDocTemplate(IDR_MAINFRAME, RUNTIME_CLASS(TMapDoc), RUNTIME_CLASS(TMainFrame), RUNTIME_CLASS(TMapView)),
      _m_pDocument(NULL)
{
}

VA(0x0045c230, 0x43)
BOOL TMapDocTemplate::GetDocString(CString& rString, enum DocStringIndex index) const
{
    switch (index) {
    case windowTitle:
        rString = kAppTitleStr;
        return TRUE;
    case filterName:
        rString = kMapFileFilterStr;
        return TRUE;
    case regFileTypeName:
        rString = kMapFileRegNameStr;
        return TRUE;
    }
    return CSingleDocTemplate::GetDocString(rString, index);
}

VA(0x0045c273, 0x77)
CDocument* TMapDocTemplate::CreateNewDocument()
{
    _m_pDocument = new TMapDoc(this);
    if (_m_pDocument == NULL)
        throw TAllocationFailure();
    AddDocument(_m_pDocument);
    return _m_pDocument;
}

VA(0x0045c2ea, 0x40)
CDocument* TMapDocTemplate::OpenDocumentFile(LPCTSTR lpszPathName, BOOL bMakeVisible)
{
    if (lpszPathName == NULL && _m_pDocument != NULL)
        _m_pDocument->promptForNewMapParams();
    CDocument* pDocument = CSingleDocTemplate::OpenDocumentFile(lpszPathName, bMakeVisible);
    if (pDocument == NULL && lpszPathName != NULL)
        pDocument = CSingleDocTemplate::OpenDocumentFile(NULL, bMakeVisible);
    return pDocument;
}

// The splash bitmap in a borderless window, centred on the screen while
// the editor starts.
class TSplashWnd : public CWnd {
public:
    TSplashWnd(const char* title);

protected:
    afx_msg void OnPaint();
    DECLARE_MESSAGE_MAP()

private:
    CBitmap _m_bitmap;
};

VA(0x0045c32a, 0x6)
BEGIN_MESSAGE_MAP(TSplashWnd, CWnd)
    ON_WM_PAINT()
END_MESSAGE_MAP()

VA(0x0045c330, 0xfa)
TSplashWnd::TSplashWnd(const char* title)
{
    _m_bitmap.LoadBitmap(IDB_SPLASH);
    BITMAP bitmap;
    _m_bitmap.GetObject(sizeof(bitmap), &bitmap);
    DATA_COMPGEN_GUARD(0x0059fe1c, splashWndClassNameGuard, className)
    VA_COMPGEN(0x0045c42a, 0xa, STATIC_DTOR, className)
    DATA(0x0059fe18) static CString className;
    if (className.IsEmpty())
        className = AfxRegisterWndClass(0, ::LoadCursor(NULL, IDC_ARROW));
    if (!CreateEx(0, className, title, WS_POPUP | WS_VISIBLE, 0, 0, bitmap.bmWidth, bitmap.bmHeight, NULL, NULL))
        throw TRuntimeError();
    CenterWindow();
    UpdateWindow();
}

VA(0x0045c493, 0xad)
void TSplashWnd::OnPaint()
{
    CPaintDC dc(this);
    BITMAP bitmap;
    _m_bitmap.GetObject(sizeof(bitmap), &bitmap);
    TMemoryDC memoryDC(&dc);
    TGDIObjectSelector<CBitmap> bitmapSelector(&memoryDC, &_m_bitmap);
    dc.BitBlt(0, 0, bitmap.bmWidth, bitmap.bmHeight, &memoryDC, 0, 0, SRCCOPY);
}

// The idle loop's animation step and the watchdog's period, in
// milliseconds.
const DWORD kAnimationStep = 180;
const DWORD kWatchdogPeriod = 10000;

}  // namespace

VA(0x0045c750, 0x6)
BEGIN_MESSAGE_MAP(TMapEditorApp, CWinApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_FILE_NEW, OnFileNew)
    ON_COMMAND(ID_VIEW_OBJECT_ANIMATION, OnViewObjectAnimation)
    ON_UPDATE_COMMAND_UI(ID_VIEW_OBJECT_ANIMATION, OnUpdateViewObjectAnimation)
    ON_COMMAND(ID_VIEW_GRID, OnViewGrid)
    ON_UPDATE_COMMAND_UI(ID_VIEW_GRID, OnUpdateViewGrid)
    ON_COMMAND(ID_VIEW_PASSABILITY, OnViewPassability)
    ON_UPDATE_COMMAND_UI(ID_VIEW_PASSABILITY, OnUpdateViewPassability)
    ON_COMMAND(ID_VIEW_TERRAIN_ANIMATION, OnViewTerrainAnimation)
    ON_UPDATE_COMMAND_UI(ID_VIEW_TERRAIN_ANIMATION, OnUpdateViewTerrainAnimation)
    ON_COMMAND(ID_TOOLS_OPTIONS, OnToolsOptions)
    ON_COMMAND(ID_VIEW_ZOOM_OUT, OnViewZoomOut)
    ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOM_OUT, OnUpdateViewZoomOut)
    ON_COMMAND(ID_VIEW_ZOOM_IN, OnViewZoomIn)
    ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOM_IN, OnUpdateViewZoomIn)
    ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

VA(0x0045c756, 0xc)
UINT TMapEditorApp::_watchdogThreadProc(LPVOID pParam)
{
    static_cast<TMapEditorApp*>(pParam)->_watchdog();
    return 0;
}

VA(0x0045c762, 0x73)
TMapEditorApp::TMapEditorApp()
    : _m_bWatchdogStarted(false),
      _m_watchdogThread(_watchdogThreadProc, this),
      _m_quitEvent(FALSE, TRUE)
{
    _m_watchdogThread.m_bAutoDelete = FALSE;
}

VA(0x0045c7f1, 0x82)
TMapEditorApp::~TMapEditorApp()
{
    if (_m_bWatchdogStarted) {
        _m_quitEvent.SetEvent();
        ::WaitForSingleObject(_m_watchdogThread.m_hThread, INFINITE);
    }
}

DATA(0x0059fcd8) TMapEditorApp theApp;

VA_COMPGEN(0x0045c873, 0x16, STATIC_CTOR, theApp)

VA_COMPGEN(0x0045c889, 0xa, STATIC_DTOR, theApp)

VA(0x0045c893, 0x5ab)
BOOL TMapEditorApp::InitInstance()
{
    srand(GetTickCount());
    try {
        bool bRegisterShellFileTypes = true;
        try {
            GetModuleFileName(NULL, _m_path.GetBuffer(MAX_PATH), MAX_PATH + 1);
            _m_path.ReleaseBuffer();
            int index = _m_path.ReverseFind('\\');
            if (index == -1)
                throw false;
            _m_path.Delete(index + 1, _m_path.GetLength() - index - 1);
            WIN32_FIND_DATA findData;
            HANDLE hFind = FindFirstFile(_m_path + "Data", &findData);
            if (hFind == INVALID_HANDLE_VALUE)
                throw false;
            FindClose(hFind);
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                throw false;
        } catch (bool) {
            _fullpath(_m_path.GetBuffer(MAX_PATH), ".\\", MAX_PATH + 1);
            _m_path.ReleaseBuffer();
            bRegisterShellFileTypes = false;
        }
        initLogFile(_m_path);
        DATA_COMPGEN_GUARD(0x0059fcd2, resourceManagerSessionGuard, resourceManagerSession)
        VA_COMPGEN(0x0045ce3e, 0x5, STATIC_DTOR, resourceManagerSession)
        DATA(0x0059fcd1) static TResourceManagerSession resourceManagerSession(_m_path);
        SText::initialize();
        if (m_pszAppName != NULL) {
            free((void*)m_pszAppName);
            m_pszAppName = NULL;
        }
        m_pszAppName = _tcsdup(kAppTitleStr);
        AfxGetModuleState()->m_lpszCurrentAppName = m_pszAppName;
        {
            CClientDC dc(NULL);
            if (dc.GetDeviceCaps(BITSPIXEL) < 16) {
                AfxMessageBox(kNeed16BitColorStr);
                exit(-1);
            }
        }
        CCommandLineInfo cmdInfo;
        ParseCommandLine(cmdInfo);
        std::auto_ptr<TSplashWnd> pSplashWnd;
        if (cmdInfo.m_bShowSplash) {
            pSplashWnd = std::auto_ptr<TSplashWnd>(new TSplashWnd(kAppTitleStr));
            if (pSplashWnd.get() == NULL)
                throw TAllocationFailure();
        }
        loadTilesets();
        initColors();
        initializeObjectSprites();
        InitializeAdvObjectTypeTraitsTable();
        InitializeGameResourceTypeTraitsTable();
        InitializePlayerTraitsTable();
        InitializeCreatureTypeTraitsTable();
        InitializeRandomCreatureTraitsTable();
        InitializeArtifactTraitsTable();
        InitializeSpellTraitsTable();
        InitializeCreatureBankTypeTraitsTable();
        InitializeMonolithTypeTraitsTables();
        InitializeKeymasterTentTypeTraitsTable();
        TLinkableObject::s_nextLinkID = (time(NULL) + GetTickCount()) * 214013 + 2531011;
        TMine::initializeTypeTraitsTable();
        TGarrison::initializeTypeTraitsTable();
        InitializeSSkillTraitsTable();
        InitializeHeroClassTraitsTable();
        InitializeHeroTraitsTable();
        loadObjectTypeTable();
        THero::initialize();
        TTown::initialize();
        TGenerator::initializeTypeTraitsTables();
        TRandomlyAlignedGenerator::initializeTypeTraitsTable();
        TRandomlyLeveledGenerator::initializeTypeTraitsTable();
        InitializeWorldDirectionNamesTable();
        AfxEnableControlContainer();
        Enable3dControlsStatic();
        if (m_pszRegistryKey != NULL)
            free((void*)m_pszRegistryKey);
        m_pszRegistryKey = _tcsdup(_T("New World Computing"));
        if (m_pszProfileName != NULL)
            free((void*)m_pszProfileName);
        m_pszProfileName = _tcsdup(_T("Heroes of Might and Magic\xae III Map Editor"));
        LoadStdProfileSettings();
        SOptions::load();
        TMapView::setZoom(TZoom(SOptions::kZoom));
        TMapView::setViewPassability(SOptions::kPassability);
        TMapView::setViewGrid(SOptions::kGrid);
        TMapDoc::setAutosaveInterval(SOptions::kAutosaveMinutes);
        TMapDoc::setSpecialTileFrequency(SOptions::kSpecialTileFrequency);
        AddDocTemplate(new TMapDocTemplate);
        if (bRegisterShellFileTypes) {
            EnableShellOpen();
            RegisterShellFileTypes(FALSE);
        }
        ParseCommandLine(cmdInfo);
        if (!ProcessShellCommand(cmdInfo))
            return FALSE;
        m_pMainWnd->ShowWindow(SW_SHOW);
        m_pMainWnd->UpdateWindow();
        m_pMainWnd->DragAcceptFiles();
        if (!_m_watchdogThread.CreateThread())
            throw TRuntimeError();
        _m_bWatchdogStarted = true;
        return TRUE;
    } catch (exception& e) {
        CString title(MAKEINTRESOURCE(IDS_FATAL_ERROR));
        AfxMessageBox(title + "\n\n" + e.what(), MB_ICONERROR);
    } catch (CException* e) {
        char message[256];
        if (!e->GetErrorMessage(message, sizeof(message)))
            throw;
        CString title(MAKEINTRESOURCE(IDS_FATAL_ERROR));
        AfxMessageBox(title + "\n\n" + message, MB_ICONERROR);
    }
    return FALSE;
}

// The about box: the title, version, build and copyright from editor.txt
// and the version resource.
class CAboutDlg : public CDialog {
public:
    CAboutDlg() : CDialog(IDD) {}

    enum { IDD = IDD_ABOUTBOX };

protected:
    virtual BOOL OnInitDialog();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    DECLARE_MESSAGE_MAP()
};

VA(0x0045ce43, 0x6)
BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    ON_WM_CREATE()
END_MESSAGE_MAP()

VA(0x0045ce49, 0x25)
int CAboutDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(kAboutBoxCaptionStr);
    return 0;
}

VA(0x0045ce6e, 0x108)
BOOL CAboutDlg::OnInitDialog()
{
    DATA_COMPGEN_GUARD(0x0059fcd0, aboutDlgStaticsGuard, pBuild)
    DATA(0x0059fccc) static const char* const pBuild = kReleaseBuildStr;
    VA_COMPGEN(0x0045cf76, 0xa, STATIC_DTOR, productVersion)
    DATA(0x0059fcc8) static const TProductVersion productVersion;
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDC_ABOUT_TITLE_STATIC)->SetWindowText(kAppTitleStr);
    GetDlgItem(IDC_ABOUT_VERSION_STATIC)->SetWindowText(TFormattedString(kVersionFmtStr, (LPCTSTR)productVersion));
    GetDlgItem(IDC_ABOUT_BUILD_STATIC)->SetWindowText(pBuild);
    GetDlgItem(IDC_ABOUT_COPYRIGHT_STATIC)->SetWindowText(kCopyrightStr);
    CDialog::OnInitDialog();
    return TRUE;
}

VA(0x0045cf80, 0x44)
void TMapEditorApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

VA(0x0045cfc4, 0x166)
int TMapEditorApp::Run()
{
    int result = 0;
    try {
        _m_animationTime = _m_lastIdleTime = GetTickCount();
        result = CWinApp::Run();
    } catch (exception& e) {
        CString title(MAKEINTRESOURCE(IDS_FATAL_ERROR));
        m_pMainWnd->MessageBox(title + "\n\n" + e.what(), NULL, MB_ICONERROR);
    } catch (CException* e) {
        char message[256];
        if (!e->GetErrorMessage(message, sizeof(message)))
            throw;
        CString title(MAKEINTRESOURCE(IDS_FATAL_ERROR));
        m_pMainWnd->MessageBox(title + "\n\n" + message, NULL, MB_ICONERROR);
    }
    return result;
}

VA(0x0045d12a, 0x3c)
void TMapEditorApp::OnFileNew()
{
    if (m_pMainWnd == NULL)
        SetCurrentDirectory(_m_path + "Maps");
    CWinApp::OnFileNew();
}

VA(0x0045d166, 0x7b)
CDocument* TMapEditorApp::OpenDocumentFile(LPCTSTR lpszFileName)
{
    CDocument* pDocument = CWinApp::OpenDocumentFile(lpszFileName);
    if (pDocument != NULL && !pDocument->GetPathName().IsEmpty()) {
        CString directory(pDocument->GetPathName());
        int index = directory.ReverseFind('\\');
        if (index != -1) {
            directory.Delete(index + 1, directory.GetLength() - index - 1);
            SetCurrentDirectory(directory);
        }
    }
    return pDocument;
}

VA(0x0045d1e1, 0x11)
int TMapEditorApp::ExitInstance()
{
    SOptions::save();
    return CWinApp::ExitInstance();
}

VA(0x0045d1f2, 0x4c)
void TMapEditorApp::_watchdog()
{
    while (!_m_quitEvent.Lock(kWatchdogPeriod)) {
        if (GetTickCount() - _m_lastIdleTime >= kWatchdogPeriod)
            ::PostThreadMessage(m_nThreadID, WM_KICKIDLE, 0, 0);
    }
}

VA(0x0045d23e, 0xa9)
BOOL TMapEditorApp::OnIdle(LONG lCount)
{
    DWORD time = GetTickCount();
    _m_lastIdleTime = time;
    CWinApp::OnIdle(lCount);
    TMapDoc::autosaveAll();
    if (!m_pMainWnd->IsIconic()) {
        bool bAnimation = SOptions::kAnimation;
        bool bCycling = SOptions::kCycling;
        if (bAnimation || bCycling) {
            DWORD elapsed = time - _m_animationTime;
            if (elapsed >= kAnimationStep) {
                unsigned int frames = elapsed / kAnimationStep;
                _m_animationTime += frames * kAnimationStep;
                if (bCycling)
                    animateTilesets(frames);
                TMapView::animate(bAnimation ? frames : 0, bCycling);
            }
            return TRUE;
        }
    }
    return FALSE;
}

VA(0x0045d2e7, 0x17)
LRESULT TMapEditorApp::ProcessWndProcException(CException* e, const MSG* pMsg)
{
    throw e;
}

VA(0x0045d2fe, 0x42)
void TMapEditorApp::OnViewObjectAnimation()
{
    bool bAnimation = !SOptions::kAnimation;
    SOptions::setAnimation(bAnimation);
    if (!bAnimation)
        TMapView::resetAnimation();
    else if (!SOptions::kCycling)
        _m_animationTime = GetTickCount();
}

VA(0x0045d340, 0x16)
void TMapEditorApp::OnUpdateViewObjectAnimation(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(SOptions::kAnimation);
}

VA(0x0045d356, 0x3b)
void TMapEditorApp::OnViewTerrainAnimation()
{
    bool bCycling = !SOptions::kCycling;
    SOptions::setCycling(bCycling);
    if (bCycling && !SOptions::kAnimation)
        _m_animationTime = GetTickCount();
}

VA(0x0045d391, 0x16)
void TMapEditorApp::OnUpdateViewTerrainAnimation(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(SOptions::kCycling);
}

VA(0x0045d3a7, 0x26)
void TMapEditorApp::OnViewGrid()
{
    bool bGrid = !SOptions::kGrid;
    SOptions::setGrid(bGrid);
    TMapView::setViewGrid(bGrid);
}

VA(0x0045d3cd, 0x16)
void TMapEditorApp::OnUpdateViewGrid(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(SOptions::kGrid);
}

VA(0x0045d3e3, 0x26)
void TMapEditorApp::OnViewPassability()
{
    bool bPassability = !SOptions::kPassability;
    SOptions::setPassability(bPassability);
    TMapView::setViewPassability(bPassability);
}

VA(0x0045d409, 0x16)
void TMapEditorApp::OnUpdateViewPassability(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(SOptions::kPassability);
}

VA(0x0045d41f, 0xab)
void TMapEditorApp::OnToolsOptions()
{
    TOptionsDlg dlg(m_pMainWnd);
    if (dlg.DoModal() == IDOK) {
        unsigned int minutes = dlg.getAutosaveMinutes();
        if (minutes != SOptions::kAutosaveMinutes) {
            SOptions::setAutosaveMinutes(minutes);
            TMapDoc::setAutosaveInterval(minutes);
        }
        unsigned int frequency = dlg.getSpecialTileFrequency();
        if (frequency != SOptions::kSpecialTileFrequency) {
            SOptions::setSpecialTileFrequency(frequency);
            TMapDoc::setSpecialTileFrequency(frequency);
        }
        if (dlg.getBRepaint())
            m_pMainWnd->SendMessage(WM_COMMAND, MAKEWPARAM(ID_TOOLS_REPAINT_MAP, 1));
    }
}

VA(0x0045d52f, 0x5a)
void TMapEditorApp::OnViewZoomIn()
{
    if (SOptions::kZoom > eZoom100) {
        CWaitCursor waitCursor;
        TZoom zoom = TZoom(SOptions::kZoom - 1);
        SOptions::setZoom(zoom);
        TMapView::setZoom(zoom);
    }
}

// MFC's inline CWaitCursor destructor, kept out of line for the unwind
// funclets that call it; this file is the first to instantiate it.
VA_COMPGEN(0x0045d589, 0xd, IMPLICIT_DTOR, CWaitCursor)

VA(0x0045d596, 0x1b)
void TMapEditorApp::OnUpdateViewZoomIn(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(SOptions::kZoom > eZoom100);
}

VA(0x0045d5b1, 0x5a)
void TMapEditorApp::OnViewZoomOut()
{
    if (SOptions::kZoom < eZoom25) {
        CWaitCursor waitCursor;
        TZoom zoom = TZoom(SOptions::kZoom + 1);
        SOptions::setZoom(zoom);
        TMapView::setZoom(zoom);
    }
}

VA(0x0045d60b, 0x1c)
void TMapEditorApp::OnUpdateViewZoomOut(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(SOptions::kZoom < eZoom25);
}
