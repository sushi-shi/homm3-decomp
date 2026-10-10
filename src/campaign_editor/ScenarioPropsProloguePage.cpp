// ScenarioPropsProloguePage.cpp - the prologue and epilogue pages of the
// scenario properties sheet (h3ccmped 0x42c060..0x42d910). The movie
// previews are drawn once from their images and kept.
#include "campaign_editor/stdafx.h"

#include <string>

#include "bitmap16.h"
#include "campaignmovie.h"
#include "campaignmusic.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "va.h"
#include "editor/DIBSection.h"
#include "editor/GDIObjectSelector.h"
#include "campaign_editor/Campaign.h"
#include "campaign_editor/CampaignEditorText.h"
#include "campaign_editor/ScenarioPropsProloguePage.h"

VA(0x0042c090, 0x16a)
TScenarioPropsProloguePageBase::TScenarioPropsProloguePageBase(const TScenario& oldScenario, TScenario& newScenario,
                                                               int campaignVersion, const CString& caption,
                                                               UINT nIDHelp)
    : CPropertyPage(TScenarioPropsProloguePageBase::IDD),
      _m_oldScenario(oldScenario),
      _m_newScenario(newScenario),
      _m_campaignVersion(campaignVersion),
      _m_bModified(false),
      _m_bInitialized(false)
{
    m_psp.pszTitle = m_strCaption = caption;
    m_psp.dwFlags |= PSP_USETITLE;
    m_nIDHelp = nIDHelp;
}

VA(0x0042c220, 0xe6)
TScenarioPropsProloguePageBase::~TScenarioPropsProloguePageBase()
{
}

VA(0x0042c310, 0x28)
void TScenarioPropsProloguePageBase::update()
{
    if (_m_bInitialized)
        EnableWindow(_m_newScenario.getMap() != NULL);
}

VA(0x0042c340, 0x393)
void TScenarioPropsProloguePageBase::_showPrologue(const TScenarioPrologue* pPrologue)
{
    if (pPrologue != NULL) {
        _m_hasPrologueCheck.SetCheck(1);
        _m_movieCombo.EnableWindow(TRUE);
        int index = 0;
        while (_m_movieCombo.GetItemData(index) != pPrologue->m_movie)
            index++;
        _m_movieCombo.SetCurSel(index);
        OnSelChangeMovieCombo();
        _m_musicList.EnableWindow(TRUE);
        index = 0;
        while (_m_musicList.GetItemData(index) != pPrologue->m_music)
            index++;
        _m_musicList.SetCurSel(index);
        _m_musicList.SetTopIndex(index);
        _m_messageEdit.EnableWindow(TRUE);
        CString text(pPrologue->m_text.c_str());
        text.Replace("\n", "\r\n");
        _m_messageEdit.SetWindowText(text);
    } else {
        _m_hasPrologueCheck.SetCheck(0);
        _m_previewStatic.SetBitmap(NULL);
        BITMAP bm;
        _m_previewBitmap.GetBitmap(&bm);
        {
            CDC dc;
            {
                CClientDC clientDC(&_m_previewStatic);
                dc.CreateCompatibleDC(&clientDC);
            }
            TGDIObjectSelector<CBitmap> bitmapSelector(&dc, &_m_previewBitmap);
            {
                CBrush brush(::GetSysColor(COLOR_BTNFACE));
                TGDIObjectSelector<CBrush> brushSelector(&dc, &brush);
                dc.PatBlt(0, 0, bm.bmWidth, bm.bmHeight, PATCOPY);
            }
        }
        _m_previewStatic.SetBitmap(_m_previewBitmap);
        _m_movieCombo.SetCurSel(-1);
        _m_movieCombo.EnableWindow(FALSE);
        _m_musicList.SetCurSel(-1);
        _m_musicList.SetTopIndex(0);
        _m_musicList.EnableWindow(FALSE);
        _m_messageEdit.SetWindowText("");
        _m_messageEdit.EnableWindow(FALSE);
    }
}

VA(0x0042c720, 0x28c)
void TScenarioPropsProloguePageBase::_getPrologue(std::auto_ptr<TScenarioPrologue>& pPrologue)
{
    if (_m_hasPrologueCheck.GetCheck() != 0) {
        int movie = _m_movieCombo.GetItemData(_m_movieCombo.GetCurSel());
        int music = _m_musicList.GetItemData(_m_musicList.GetCurSel());
        CString text;
        _m_messageEdit.GetWindowText(text);
        text.Replace("\r\n", "\n");
        pPrologue = std::auto_ptr<TScenarioPrologue>(new TScenarioPrologue(movie, music, std::string(text)));
        if (pPrologue.get() == NULL)
            throw TAllocationFailure();
    } else {
        pPrologue = std::auto_ptr<TScenarioPrologue>();
    }
}

VA(0x0042c9b0, 0x9d)
void TScenarioPropsProloguePageBase::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_MESSAGE_STATIC, _m_messageStatic);
    DDX_Control(pDX, IDC_MUSIC_LIST_STATIC, _m_musicStatic);
    DDX_Control(pDX, IDC_MOVIE_STATIC, _m_movieStatic);
    DDX_Control(pDX, IDC_MESSAGE_EDIT, _m_messageEdit);
    DDX_Control(pDX, IDC_MUSIC_LIST, _m_musicList);
    DDX_Control(pDX, IDC_MOVIE_COMBO, _m_movieCombo);
    DDX_Control(pDX, IDC_PREVIEW_STATIC, _m_previewStatic);
    DDX_Control(pDX, IDC_HAS_PROLOGUE_CHECK, _m_hasPrologueCheck);
}

VA(0x0042ca50, 0x6)
BEGIN_MESSAGE_MAP(TScenarioPropsProloguePageBase, CPropertyPage)
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_HAS_PROLOGUE_CHECK, OnHasPrologueCheck)
    ON_CBN_SELCHANGE(IDC_MOVIE_COMBO, OnSelChangeMovieCombo)
    ON_WM_ENABLE()
END_MESSAGE_MAP()

VA(0x0042ca60, 0xc)
void TScenarioPropsProloguePageBase::OnDestroy()
{
    _m_bInitialized = false;
    CPropertyPage::OnDestroy();
}

VA(0x0042ca70, 0x65)
void TScenarioPropsProloguePageBase::OnEnable(BOOL bEnable)
{
    if (!bEnable)
        _showPrologue(NULL);
    CPropertyPage::OnEnable(bEnable);
    for (CWnd* pChild = GetWindow(GW_CHILD); pChild != NULL; pChild = pChild->GetWindow(GW_HWNDNEXT))
        pChild->EnableWindow(bEnable);
    if (bEnable)
        _showPrologue(NULL);
}

VA(0x0042cae0, 0x21e)
BOOL TScenarioPropsProloguePageBase::OnInitDialog()
{
    GetDlgItem(IDC_MOVIE_STATIC)->SetWindowText(SScenarioPropsProloguePageText::kMovieStaticStr);
    GetDlgItem(IDC_MUSIC_LIST_STATIC)->SetWindowText(SScenarioPropsProloguePageText::kMusicStaticStr);
    GetDlgItem(IDC_MESSAGE_STATIC)->SetWindowText(SScenarioPropsProloguePageText::kMessageStaticStr);
    _m_bModified = false;
    CPropertyPage::OnInitDialog();
    _m_hasPrologueCheck.SetWindowText(getCheckText());
    CRect rect;
    _m_previewStatic.GetClientRect(&rect);
    {
        CClientDC dc(&_m_previewStatic);
        _m_previewBitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
    }
    _m_previewStatic.SetBitmap(_m_previewBitmap);
    unsigned int numMovies = _m_campaignVersion >= eCampaignVersionShadowOfDeath ? CAMPAIGN_MOVIE_COUNT : 57;
    unsigned int movie;
    for (movie = 0; movie < numMovies; movie++)
        _m_movieCombo.SetItemData(_m_movieCombo.AddString(g_campaignMovieTraits[movie].m_name), movie);
    unsigned int numMusic = _m_campaignVersion >= eCampaignVersionShadowOfDeath ? CAMPAIGN_MUSIC_CUE_COUNT
                                                                                 : CAMPAIGN_MUSIC_CUE_COUNT - 1;
    for (unsigned int music = 0; music < numMusic; music++)
        _m_musicList.SetItemData(_m_musicList.AddString(g_campaignMusicTraits[music].m_track), music);
    _m_bInitialized = true;
    if (_m_newScenario.getMap() != NULL)
        _showPrologue(getPrologue(_m_newScenario));
    else
        EnableWindow(FALSE);
    return TRUE;
}

VA(0x0042cd00, 0x24f)
void TScenarioPropsProloguePageBase::OnOK()
{
    CPropertyPage::OnOK();
    if (_m_newScenario.getMap() != NULL) {
        std::auto_ptr<TScenarioPrologue> pPrologue;
        _getPrologue(pPrologue);
        if (pPrologue.get() != NULL) {
            if (getPrologue(_m_newScenario) == NULL || !(*getPrologue(_m_newScenario) == *pPrologue))
                setPrologue(_m_newScenario, pPrologue);
        } else if (getPrologue(_m_newScenario) != NULL) {
            removePrologue(_m_newScenario);
        }
    }
    _m_bModified = _m_bModified
                   || (getPrologue(_m_oldScenario) != NULL) != (getPrologue(_m_newScenario) != NULL)
                   || getPrologue(_m_newScenario) != NULL
                          && !(*getPrologue(_m_oldScenario) == *getPrologue(_m_newScenario));
}

VA(0x0042cf50, 0xae)
void TScenarioPropsProloguePageBase::OnHasPrologueCheck()
{
    if (_m_hasPrologueCheck.GetCheck()) {
        TScenarioPrologue prologue;
        _showPrologue(&prologue);
    } else {
        _showPrologue(NULL);
    }
}

VA(0x0042d000, 0x465)
void TScenarioPropsProloguePageBase::OnSelChangeMovieCombo()
{
    static std::auto_ptr<T16bppDIBSection> apPreviews[CAMPAIGN_MOVIE_COUNT];
    int movie = _m_movieCombo.GetItemData(_m_movieCombo.GetCurSel());
    std::auto_ptr<T16bppDIBSection>& pPreview = apPreviews[movie];
    if (pPreview.get() == NULL) {
        TResourcePtr<Bitmap16Bit> pImage(ResourceManager::GetBitmap16(g_campaignMovieTraits[movie].m_imageName));
        if (pImage.get() == NULL)
            throw TRuntimeError();
        pPreview = std::auto_ptr<T16bppDIBSection>(new T16bppDIBSection(pImage->GetWidth(), pImage->GetHeight()));
        if (pPreview.get() == NULL)
            throw TAllocationFailure();
        pImage->Draw(0, 0, pImage->GetWidth(), pImage->GetHeight(), pPreview->getPixels(), 0, 0,
                     pPreview->getWidth(), pPreview->getHeight(), pPreview->getPitch(), false);
    }
    _m_previewStatic.SetBitmap(NULL);
    BITMAP bm;
    _m_previewBitmap.GetBitmap(&bm);
    {
        CDC sourceDC;
        CDC destDC;
        {
            CClientDC clientDC(&_m_previewStatic);
            sourceDC.CreateCompatibleDC(&clientDC);
            destDC.CreateCompatibleDC(&clientDC);
        }
        TGDIObjectSelector<CBitmap> sourceSelector(&sourceDC, pPreview.get());
        TGDIObjectSelector<CBitmap> destSelector(&destDC, &_m_previewBitmap);
        int oldMode = destDC.SetStretchBltMode(HALFTONE);
        destDC.StretchBlt(0, 0, bm.bmWidth, bm.bmHeight, &sourceDC, 0, 0, pPreview->getWidth(),
                          pPreview->getHeight(), SRCCOPY);
        destDC.SetStretchBltMode(oldMode);
    }
    _m_previewStatic.SetBitmap(_m_previewBitmap);
}

VA(0x0042d490, 0x80)
TScenarioPropsProloguePage::TScenarioPropsProloguePage(const TScenario& oldScenario, TScenario& newScenario,
                                                       int campaignVersion)
    : TScenarioPropsProloguePageBase(oldScenario, newScenario, campaignVersion, CString(kPrologueStr),
                                     IDD_SCENARIO_PROPS_PROLOGUE)
{
}

VA(0x0042d530, 0x24)
CString TScenarioPropsProloguePage::getCheckText() const
{
    return kHasPrologueStr;
}

VA(0x0042d560, 0xc)
const TScenarioPrologue* TScenarioPropsProloguePage::getPrologue(const TScenario& scenario) const
{
    return scenario.getPrologue();
}

VA(0x0042d570, 0x50)
void TScenarioPropsProloguePage::setPrologue(TScenario& scenario, std::auto_ptr<TScenarioPrologue> pPrologue) const
{
    scenario.setPrologue(pPrologue);
}

VA(0x0042d5c0, 0xc)
void TScenarioPropsProloguePage::removePrologue(TScenario& scenario) const
{
    scenario.removePrologue();
}

VA(0x0042d5d0, 0x80)
TScenarioPropsEpiloguePage::TScenarioPropsEpiloguePage(const TScenario& oldScenario, TScenario& newScenario,
                                                       int campaignVersion)
    : TScenarioPropsProloguePageBase(oldScenario, newScenario, campaignVersion, CString(kEpilogueStr),
                                     IDD_SCENARIO_PROPS_EPILOGUE)
{
}

VA(0x0042d660, 0x24)
CString TScenarioPropsEpiloguePage::getCheckText() const
{
    return kHasEpilogueStr;
}

VA(0x0042d690, 0xc)
const TScenarioPrologue* TScenarioPropsEpiloguePage::getPrologue(const TScenario& scenario) const
{
    return scenario.getEpilogue();
}

VA(0x0042d6a0, 0x50)
void TScenarioPropsEpiloguePage::setPrologue(TScenario& scenario, std::auto_ptr<TScenarioPrologue> pPrologue) const
{
    scenario.setEpilogue(pPrologue);
}

VA(0x0042d6f0, 0xc)
void TScenarioPropsEpiloguePage::removePrologue(TScenario& scenario) const
{
    scenario.removeEpilogue();
}
