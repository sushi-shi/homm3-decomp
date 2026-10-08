// SelectArtifactDlg.cpp - the artifact picker (h3maped 0x4b7dcf..0x4b8063).
// RoE maps offer 127 artifacts, AB maps 129 and SoD maps all 144. The
// implicit destructor has no body here: /OPT:ICF folded it onto an
// identical one at 0x4099d6.
#include "editor/stdafx.h"

#include "va.h"
#include "artifact.h"
#include "editor/MapEditorText.h"
#include "editor/SelectArtifactDlg.h"

VA(0x004b7deb, 0x74)
TSelectArtifactDlg::TSelectArtifactDlg(CWnd* pParent, EGameVersion mapVersion, int artifact)
    : CDialog(TSelectArtifactDlg::IDD, pParent),
      _m_mapVersion(mapVersion),
      _m_artifact(artifact)
{
}

VA_COMPGEN(0x004b7e5f, 0x1c, SCALAR_DELETING_DTOR, TSelectArtifactDlg)

VA(0x004b7e7b, 0x2b)
void TSelectArtifactDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_ARTIFACT_LIST, _m_artifactList);
    DDX_Control(pDX, IDOK, _m_okButton);
}

VA(0x004b7ea6, 0x6)
BEGIN_MESSAGE_MAP(TSelectArtifactDlg, CDialog)
    ON_WM_CREATE()
    ON_LBN_SELCHANGE(IDC_ARTIFACT_LIST, OnSelChangeArtifactList)
    ON_LBN_SELCANCEL(IDC_ARTIFACT_LIST, OnSelCancelArtifactList)
    ON_LBN_DBLCLK(IDC_ARTIFACT_LIST, OnDblClkArtifactList)
END_MESSAGE_MAP()

VA(0x004b7eac, 0x25)
int TSelectArtifactDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(kSelectArtifactCaptionStr);
    return 0;
}

VA(0x004b7ed1, 0x11d)
BOOL TSelectArtifactDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    CDialog::OnInitDialog();
    int numArtifacts;
    if (_m_mapVersion >= GAME_VERSION_SOD)
        numArtifacts = 144;
    else
        numArtifacts = _m_mapVersion >= GAME_VERSION_AB ? 129 : 127;
    for (int artifact = 0; artifact < numArtifacts; artifact++) {
        if (!akArtifactTraits[artifact].m_disabled
            && !(akArtifactTraits[artifact].m_class & ArtifactClassSpecial)) {
            int index = _m_artifactList.AddString(akArtifactTraits[artifact].m_name);
            _m_artifactList.SetItemData(index, artifact);
        }
    }
    if (_m_artifact != -1) {
        int index = 0;
        while (int(_m_artifactList.GetItemData(index)) != _m_artifact)
            index++;
        _m_artifactList.SetCurSel(index);
        _m_artifactList.SetTopIndex(index);
    } else {
        _m_okButton.EnableWindow(FALSE);
    }
    return TRUE;
}

VA(0x004b7fee, 0x33)
void TSelectArtifactDlg::OnOK()
{
    CDialog::OnOK();
    _m_artifact = _m_artifactList.GetItemData(_m_artifactList.GetCurSel());
}

VA(0x004b8021, 0xe)
void TSelectArtifactDlg::OnSelChangeArtifactList()
{
    _m_okButton.EnableWindow(TRUE);
}

VA(0x004b802f, 0xe)
void TSelectArtifactDlg::OnSelCancelArtifactList()
{
    _m_okButton.EnableWindow(FALSE);
}

VA(0x004b803d, 0x26)
void TSelectArtifactDlg::OnDblClkArtifactList()
{
    if (_m_artifactList.GetCurSel() != LB_ERR)
        OnOK();
}
