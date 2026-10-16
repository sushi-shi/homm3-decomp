// CampaignEditor.cpp - the campaign editor application (h3ccmped
// 0x403250..0x404ff0): its splash window, document template and about
// box. Startup finds the game's Data directory beside the executable (or
// else uses the current directory), opens the resource manager on it and
// loads the editor's text and trait tables.
#include "campaign_editor/stdafx.h"

#include <stdlib.h>
#include <string.h>

#include "va.h"
#include "autoarrayptr.h"
#include "campaignmap.h"
#include "campaignmusic.h"
#include "herodefs.h"
#include "resourcemanager.h"
#include "spelldefs.h"
#include "editor/FormattedString.h"
#include "editor/GameResource.h"
#include "editor/GDIObjectSelector.h"
#include "editor/Hero.h"
#include "editor/MemoryDC.h"
#include "editor/Player.h"
#include "editor/Town.h"
#include "campaign_editor/CampaignDoc.h"
#include "campaign_editor/CampaignEditor.h"
#include "campaign_editor/CampaignEditorText.h"
#include "campaign_editor/CampaignView.h"
#include "campaign_editor/MainFrame.h"
#include "campaign_editor/resource.h"

bool InitializeArtifactTraitsTable();
unsigned char InitializeCreatureTypeTraitsTable();
unsigned char initializeCampaignMovieTraitsTable();

// The product version from the executable's version resource (US English,
// Unicode or Windows code page), empty without one.
class TProductVersion : public CString {
public:
    TProductVersion();
};

// Retail obtains the destination buffer before loading the queried string.
VA(0x004032b0, 0x13b)
TProductVersion::TProductVersion()
{
    char fileName[MAX_PATH + 1];
    if (::GetModuleFileName(NULL, fileName, sizeof(fileName)) > 0) {
        DWORD handle;
        DWORD size = ::GetFileVersionInfoSize(fileName, &handle);
        if (size > 0) {
            TAutoArrayPtr<char> pVersionInfo(new char[size]);
            if (pVersionInfo.get() != NULL && ::GetFileVersionInfo(fileName, handle, size, pVersionInfo.get())) {
                UINT length;
                char* pValue;
                if (::VerQueryValue(pVersionInfo.get(), "\\StringFileInfo\\040904B0\\ProductVersion", (void**)&pValue,
                                    &length)
                    || ::VerQueryValue(pVersionInfo.get(), "\\StringFileInfo\\040904e4\\ProductVersion",
                                       (void**)&pValue, &length)) {
                    char* buffer = GetBuffer(length);
                    strcpy(buffer, pValue);
                    ReleaseBuffer();
                }
            }
        }
    }
}

// The resource manager over the game's Data directory, open for the
// application's lifetime.
class TResourceManagerSession {
public:
    TResourceManagerSession(const CString& directory);
    ~TResourceManagerSession();
};

VA(0x004033f0, 0x83)
TResourceManagerSession::TResourceManagerSession(const CString& directory)
{
    ResourceManager::SetPath(directory + "Data\\");
    ResourceManager::Open(false, true, NULL);
    ResourceManager::SetPixelFormat(0x7c00, 0x3e0, 0x1f);
}

VA(0x00403480, 0x5)
TResourceManagerSession::~TResourceManagerSession()
{
    ResourceManager::Close();
}

namespace {

// The splash bitmap, centred on the screen while the editor starts.
class TSplashWnd : public CWnd {
public:
    TSplashWnd(LPCTSTR lpszTitle);

protected:
    afx_msg void OnPaint();
    DECLARE_MESSAGE_MAP()

private:
    CBitmap _m_bitmap;
};

VA(0x00403490, 0x6)
BEGIN_MESSAGE_MAP(TSplashWnd, CWnd)
    ON_WM_PAINT()
END_MESSAGE_MAP()

VA(0x004034a0, 0x1df)
TSplashWnd::TSplashWnd(LPCTSTR lpszTitle)
{
    _m_bitmap.LoadBitmap(IDB_SPLASH);
    BITMAP bitmap;
    _m_bitmap.GetBitmap(&bitmap);
    DATA_COMPGEN_GUARD(0x004c6f10, classNameGuard, className)
    VA_COMPGEN(0x00403750, 0xa, STATIC_DTOR, className)
    DATA(0x004c6f0c) static CString className;
    if (className.IsEmpty())
        className = AfxRegisterWndClass(0, ::LoadCursor(NULL, IDC_ARROW));
    if (!CreateEx(0, className, lpszTitle, WS_POPUP | WS_VISIBLE, 0, 0, bitmap.bmWidth, bitmap.bmHeight, NULL, NULL))
        throw TRuntimeError();
    CenterWindow();
    UpdateWindow();
}

VA(0x004037e0, 0x155)
void TSplashWnd::OnPaint()
{
    CPaintDC dc(this);
    BITMAP bitmap;
    _m_bitmap.GetBitmap(&bitmap);
    TMemoryDC memoryDC(&dc);
    TGDIObjectSelector<CBitmap> selector(&memoryDC, &_m_bitmap);
    dc.BitBlt(0, 0, bitmap.bmWidth, bitmap.bmHeight, &memoryDC, 0, 0, SRCCOPY);
}

// The single document template: the editor's own document strings, and
// File New asks for the new campaign's map.
class TCampaignDocTemplate : public CSingleDocTemplate {
public:
    TCampaignDocTemplate();

    virtual BOOL GetDocString(CString& rString, enum DocStringIndex index) const;
    virtual CDocument* OpenDocumentFile(LPCTSTR lpszPathName, BOOL bMakeVisible = TRUE);
};

VA(0x00403c80, 0x26)
TCampaignDocTemplate::TCampaignDocTemplate()
    : CSingleDocTemplate(IDR_MAINFRAME, RUNTIME_CLASS(TCampaignDoc), RUNTIME_CLASS(TMainFrame),
                         RUNTIME_CLASS(TCampaignView))
{
}

VA(0x00403ce0, 0x6d)
BOOL TCampaignDocTemplate::GetDocString(CString& rString, enum DocStringIndex index) const
{
    switch (index) {
    case windowTitle:
        rString = kAppTitleStr;
        return TRUE;
    case filterName:
        rString = kCampaignFileFilterStr;
        return TRUE;
    case regFileTypeName:
        rString = kCampaignFileRegNameStr;
        return TRUE;
    default:
        return CSingleDocTemplate::GetDocString(rString, index);
    }
}

VA(0x00403d50, 0x3c)
CDocument* TCampaignDocTemplate::OpenDocumentFile(LPCTSTR lpszPathName, BOOL bMakeVisible)
{
    if (lpszPathName == NULL && m_pOnlyDoc != NULL)
        ((TCampaignDoc*)m_pOnlyDoc)->setBNewCampaign(true);
    CDocument* pDocument = CSingleDocTemplate::OpenDocumentFile(lpszPathName, bMakeVisible);
    if (pDocument == NULL && lpszPathName != NULL)
        pDocument = CSingleDocTemplate::OpenDocumentFile(NULL, bMakeVisible);
    return pDocument;
}

}  // namespace

class CAboutDlg : public CDialog {
public:
    CAboutDlg();

    enum { IDD = IDD_ABOUTBOX };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    DECLARE_MESSAGE_MAP()
};

VA(0x00403d90, 0x6)
BEGIN_MESSAGE_MAP(TCampaignEditorApp, CWinApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_FILE_NEW, OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

VA(0x00403da0, 0x1f)
TCampaignEditorApp::TCampaignEditorApp()
{
}

DATA(0x004c6e48) TCampaignEditorApp theApp;

// Native passes theApp at 0x4c6e48 to the constructor at 0x403da0.
VA_COMPGEN(0x00403e40, 0xa, STATIC_CTOR, theApp)

// Named handler resources preserve retail's address reloads and shared slots.
VA(0x00403e70, 0x6a2)
BOOL TCampaignEditorApp::InitInstance()
{
    try {
        bool bInstalled = true;
        try {
            ::GetModuleFileName(NULL, _m_directory.GetBuffer(MAX_PATH), MAX_PATH);
            _m_directory.ReleaseBuffer();
            int separator = _m_directory.ReverseFind('\\');
            if (separator == -1)
                throw false;
            _m_directory.Delete(separator + 1, _m_directory.GetLength() - separator - 1);
            WIN32_FIND_DATA findData;
            HANDLE hFind = ::FindFirstFile(_m_directory + "Data", &findData);
            if (hFind == INVALID_HANDLE_VALUE)
                throw false;
            ::FindClose(hFind);
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                throw false;
        } catch (bool) {
            _fullpath(_m_directory.GetBuffer(MAX_PATH), ".\\", MAX_PATH);
            _m_directory.ReleaseBuffer();
            bInstalled = false;
        }
        DATA_COMPGEN_GUARD(0x004c6e42, resourceManagerGuard, resourceManager)
        VA_COMPGEN(0x00404530, 0xa, STATIC_DTOR, resourceManager)
        DATA(0x004c6e41) static TResourceManagerSession resourceManager(_m_directory);
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
                AfxMessageBox(kDisplayDepthErrorStr);
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
        if (!initializeCampaignMapTraitsTable())
            throw TRuntimeError();
        if (!initializeCampaignMusicTable())
            throw TRuntimeError();
        if (!initializeCampaignMovieTraitsTable())
            throw TRuntimeError();
        InitializePlayerTraitsTable();
        InitializeArtifactTraitsTable();
        InitializeCreatureTypeTraitsTable();
        InitializeSpellTraitsTable();
        InitializePrimarySkillTraitsTable();
        InitializeSSkillTraitsTable();
        InitializeSkillMasteryTraitsTable();
        InitializeHeroClassTraitsTable();
        InitializeHeroTraitsTable();
        InitializeTownTypeTraitsTable();
        InitializeGameResourceTypeTraitsTable();
        AfxEnableControlContainer();
        Enable3dControlsStatic();
        if (m_pszRegistryKey != NULL)
            free((void*)m_pszRegistryKey);
        m_pszRegistryKey = _tcsdup("New World Computing");
        if (m_pszProfileName != NULL)
            free((void*)m_pszProfileName);
        m_pszProfileName = _tcsdup("Heroes of Might and Magic\xae III Campaign Editor");
        LoadStdProfileSettings(4);
        AddDocTemplate(new TCampaignDocTemplate);
        if (bInstalled) {
            EnableShellOpen();
            RegisterShellFileTypes(FALSE);
        }
        ParseCommandLine(cmdInfo);
        if (!ProcessShellCommand(cmdInfo))
            return FALSE;
        m_pMainWnd->ShowWindow(SW_SHOW);
        m_pMainWnd->UpdateWindow();
        m_pMainWnd->DragAcceptFiles();
        return TRUE;
    } catch (const exception& e) {
        CString fatalError((LPCTSTR)IDS_FATAL_ERROR);
        AfxMessageBox(fatalError + "\n\n" + e.what(), MB_ICONSTOP);
    } catch (CException* e) {
        TCHAR message[256];
        if (!e->GetErrorMessage(message, sizeof(message)))
            throw;
        CString fatalError((LPCTSTR)IDS_FATAL_ERROR);
        AfxMessageBox(fatalError + "\n\n" + message, MB_ICONSTOP);
    }
    return FALSE;
}

VA(0x004046c0, 0x16)
CAboutDlg::CAboutDlg()
    : CDialog(CAboutDlg::IDD)
{
}

// Native CAboutDlg vtable 0x487b18 selects this override at slot 33;
// the pinned MFC CDialog vtable names that slot as DoDataExchange.
VA(0x004046e0, 0x3)
void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

VA(0x004046f0, 0x6)
BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    ON_WM_CREATE()
END_MESSAGE_MAP()

VA(0x00404700, 0x28)
int CAboutDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(kAboutCaptionStr);
    return 0;
}

VA(0x00404730, 0x12a)
BOOL CAboutDlg::OnInitDialog()
{
    DATA_COMPGEN_GUARD(0x004c6e40, aboutGuard, buildStr)
    DATA(0x004c6e3c) static const char* const buildStr = kReleaseBuildStr;
    VA_COMPGEN(0x004048b0, 0xa, STATIC_DTOR, version)
    VA_COMPGEN(0x004048c0, 0x5, IMPLICIT_DTOR, TProductVersion)
    DATA(0x004c6e38) static TProductVersion version;
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDC_ABOUT_TITLE_STATIC)->SetWindowText(kAppTitleStr);
    GetDlgItem(IDC_ABOUT_VERSION_STATIC)->SetWindowText(TFormattedString(kVersionFmtStr, (LPCTSTR)version));
    GetDlgItem(IDC_ABOUT_BUILD_STATIC)->SetWindowText(buildStr);
    GetDlgItem(IDC_ABOUT_COPYRIGHT_STATIC)->SetWindowText(kCopyrightStr);
    CDialog::OnInitDialog();
    return TRUE;
}

VA(0x004048d0, 0x52)
void TCampaignEditorApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

// Both catch handlers reload the resource CString address after construction;
// named resources preserve that access and the three-string cleanup order.
VA(0x00404930, 0x174)
int TCampaignEditorApp::Run()
{
    int exitCode = 0;
    try {
        exitCode = CWinApp::Run();
    } catch (const exception& e) {
        CString fatalError((LPCTSTR)IDS_FATAL_ERROR);
        m_pMainWnd->MessageBox(fatalError + "\n\n" + e.what(), NULL, MB_ICONSTOP);
    } catch (CException* e) {
        TCHAR message[256];
        if (!e->GetErrorMessage(message, sizeof(message)))
            throw;
        CString fatalError((LPCTSTR)IDS_FATAL_ERROR);
        m_pMainWnd->MessageBox(fatalError + "\n\n" + message, NULL, MB_ICONSTOP);
    }
    return exitCode;
}

VA(0x00404ab0, 0x20)
LRESULT TCampaignEditorApp::ProcessWndProcException(CException* e, const MSG* pMsg)
{
    throw e;
}

VA(0x00404ad0, 0x3d)
void TCampaignEditorApp::OnFileNew()
{
    if (m_pMainWnd == NULL)
        ::SetCurrentDirectory(_m_directory + "Maps");
    CWinApp::OnFileNew();
}

VA(0x00404b10, 0x9b)
CDocument* TCampaignEditorApp::OpenDocumentFile(LPCTSTR lpszFileName)
{
    CDocument* pDocument = CWinApp::OpenDocumentFile(lpszFileName);
    if (pDocument != NULL && !pDocument->GetPathName().IsEmpty()) {
        CString directory(pDocument->GetPathName());
        int separator = directory.ReverseFind('\\');
        if (separator != -1) {
            directory.Delete(separator + 1, directory.GetLength() - separator - 1);
            ::SetCurrentDirectory(directory);
        }
    }
    return pDocument;
}
