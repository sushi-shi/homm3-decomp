// ScenarioPropsGeneralPage.cpp - the general page of the scenario
// properties sheet (h3ccmped 0x42aa30..0x42c060). A scenario without a map
// offers only its region color and the import button. Prerequisites are
// the other scenarios with maps that do not already require this one; an
// indirect prerequisite shows checked and disabled.
#include "campaign_editor/stdafx.h"

#include <afxdlgs.h>

#include <string>

#include "campaignmap.h"
#include "va.h"
#include "editor/Player.h"
#include "campaign_editor/Campaign.h"
#include "campaign_editor/CampaignDoc.h"
#include "campaign_editor/CampaignEditorText.h"
#include "campaign_editor/ScenarioPropsGeneralPage.h"

// Asks for a map file to open or save: the map filter's text, then its
// extension after the last line break.
VA(0x0042aa60, 0x262)
static bool promptMapFileName(LPCTSTR name, BOOL bOpenFileDialog, DWORD flags, CString& fileName)
{
    CString filter;
    CString defaultExt;
    CString filterText(kMapFileFilterStr);
    int separator = filterText.ReverseFind('\n');
    defaultExt = filterText.Right(filterText.GetLength() - separator - 2);
    filter = filterText.Left(separator);
    filter += "|*";
    filter += filterText.Right(filterText.GetLength() - separator - 1);
    filter += "|";
    CString allFilter((LPCTSTR)AFX_IDS_ALLFILTER);
    filter += allFilter;
    filter += "|*.*||";
    CFileDialog dlg(bOpenFileDialog, defaultExt, NULL, flags, filter, NULL);
    fileName = name != NULL ? name : "";
    dlg.m_ofn.lpstrFile = fileName.GetBuffer(_MAX_PATH);
    int result = dlg.DoModal();
    fileName.ReleaseBuffer();
    return result == IDOK;
}

VA(0x0042acd0, 0x1af)
TScenarioPropsGeneralPage::TScenarioPropsGeneralPage(TParentSheet* pSheet, TCampaignDoc* pDoc,
                                                     TCampaign* pCampaign, int scenario)
    : CPropertyPage(TScenarioPropsGeneralPage::IDD),
      _m_pSheet(pSheet),
      _m_pDoc(pDoc),
      _m_pCampaign(pCampaign),
      _m_scenario(scenario),
      _m_bModified(false),
      _m_bMapChanged(false)
{
    m_psp.pszTitle = m_strCaption = SScenarioPropsGeneralPageText::kCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x0042ae80, 0x1e, SCALAR_DELETING_DTOR, TScenarioPropsGeneralPage)

VA(0x0042aea0, 0xf5)
TScenarioPropsGeneralPage::~TScenarioPropsGeneralPage()
{
}

VA(0x0042afa0, 0x1c6)
void TScenarioPropsGeneralPage::_fillPrerequisiteList()
{
    _m_prerequisiteList.ResetContent();
    for (unsigned int scenario = 0; scenario < _m_pCampaign->getMapTraits().m_numRegions; scenario++) {
        if (scenario == _m_scenario)
            continue;
        const TCampaignScenarioMap* pMap = _m_pCampaign->getScenario(scenario).getMap();
        if (pMap == NULL || _m_pCampaign->getBPrerequisite(scenario, _m_scenario))
            continue;
        int index = _m_prerequisiteList.AddString(
            (pMap->m_name + " - " + _m_pCampaign->getMapTraits().m_regionTraits[scenario].m_name).c_str());
        _m_prerequisiteList.SetItemData(index, scenario);
    }
}

VA(0x0042b170, 0xbd)
void TScenarioPropsGeneralPage::_checkPrerequisites()
{
    int count = _m_prerequisiteList.GetCount();
    for (int i = 0; i < count; i++) {
        int prerequisite = _m_prerequisiteList.GetItemData(i);
        if (_m_pCampaign->getBDirectPrerequisite(_m_scenario, prerequisite)) {
            _m_prerequisiteList.Enable(i, TRUE);
            _m_prerequisiteList.SetCheck(i, 1);
        } else if (_m_pCampaign->getBPrerequisite(_m_scenario, prerequisite)) {
            _m_prerequisiteList.Enable(i, FALSE);
            _m_prerequisiteList.SetCheck(i, 1);
        } else {
            _m_prerequisiteList.Enable(i, TRUE);
            _m_prerequisiteList.SetCheck(i, 0);
        }
    }
}

VA(0x0042b230, 0xd3)
void TScenarioPropsGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_REGION_COLOR_COMBO, _m_regionColorCombo);
    DDX_Control(pDX, IDC_DIFFICULTY_COMBO, _m_difficultyCombo);
    DDX_Control(pDX, IDC_MESSAGE_EDIT, _m_regionTextEdit);
    DDX_Control(pDX, IDC_EXPORT_BUTTON, _m_exportButton);
    DDX_Control(pDX, IDC_REFRESH_BUTTON, _m_refreshButton);
    DDX_Control(pDX, IDC_IMPORT_BUTTON, _m_importButton);
    DDX_Control(pDX, IDC_REMOVE_BUTTON, _m_removeButton);
    DDX_Control(pDX, IDC_MAP_FILE_STATIC, _m_mapFileStatic);
    DDX_Control(pDX, IDC_SCENARIO_NAME_VALUE_STATIC, _m_scenarioNameStatic);
    DDX_Control(pDX, IDC_REGION_NAME_VALUE_STATIC, _m_regionNameStatic);
    DDX_Control(pDX, IDC_PREREQUISITE_LIST, _m_prerequisiteList);
}

VA(0x0042b310, 0x6)
BEGIN_MESSAGE_MAP(TScenarioPropsGeneralPage, CPropertyPage)
    ON_BN_CLICKED(IDC_IMPORT_BUTTON, OnImportButton)
    ON_BN_CLICKED(IDC_EXPORT_BUTTON, OnExportButton)
    ON_BN_CLICKED(IDC_REMOVE_BUTTON, OnRemoveButton)
    ON_BN_CLICKED(IDC_REFRESH_BUTTON, OnRefreshButton)
    ON_CLBN_CHKCHANGE(IDC_PREREQUISITE_LIST, OnCheckChangePrerequisiteList)
END_MESSAGE_MAP()

VA(0x0042b320, 0x49e)
BOOL TScenarioPropsGeneralPage::OnInitDialog()
{
    GetDlgItem(IDC_REGION_NAME_STATIC)->SetWindowText(SScenarioPropsGeneralPageText::kRegionNameStaticStr);
    GetDlgItem(IDC_REGION_COLOR_STATIC)->SetWindowText(SScenarioPropsGeneralPageText::kRegionColorStaticStr);
    GetDlgItem(IDC_SCENARIO_NAME_STATIC)->SetWindowText(SScenarioPropsGeneralPageText::kScenarioNameStaticStr);
    GetDlgItem(IDC_MAP_FILE_GROUP)->SetWindowText(SScenarioPropsGeneralPageText::kMapFileGroupStr);
    GetDlgItem(IDC_IMPORT_BUTTON)->SetWindowText(SScenarioPropsGeneralPageText::kImportButtonStr);
    GetDlgItem(IDC_REFRESH_BUTTON)->SetWindowText(SScenarioPropsGeneralPageText::kRefreshButtonStr);
    GetDlgItem(IDC_EXPORT_BUTTON)->SetWindowText(SScenarioPropsGeneralPageText::kExportButtonStr);
    GetDlgItem(IDC_REMOVE_BUTTON)->SetWindowText(SScenarioPropsGeneralPageText::kRemoveButtonStr);
    GetDlgItem(IDC_DIFFICULTY_STATIC)->SetWindowText(SScenarioPropsGeneralPageText::kDifficultyStaticStr);
    GetDlgItem(IDC_PREREQUISITES_STATIC)->SetWindowText(SScenarioPropsGeneralPageText::kPrerequisitesStaticStr);
    GetDlgItem(IDC_REGION_TEXT_STATIC)->SetWindowText(SScenarioPropsGeneralPageText::kRegionTextStaticStr);
    _m_bModified = false;
    _m_bMapChanged = false;
    CPropertyPage::OnInitDialog();
    for (unsigned int player = 0; player < kNumPlayers; player++)
        _m_regionColorCombo.SetItemData(_m_regionColorCombo.AddString(akPlayerTraits[player].m_pColorName), player);
    int regionColor = _m_pCampaign->getScenario(_m_scenario).getRegionColor();
    int index = 0;
    while (_m_regionColorCombo.GetItemData(index) != regionColor)
        index++;
    _m_regionColorCombo.SetCurSel(index);
    static const char* const akDifficultyNames[TScenario::kNumDifficulties] = {
        kEasyStr, kNormalStr, kHardStr, kExpertStr, kImpossibleStr
    };
    for (int difficulty = 0; difficulty < TScenario::kNumDifficulties; difficulty++)
        _m_difficultyCombo.SetItemData(_m_difficultyCombo.AddString(akDifficultyNames[difficulty]), difficulty);
    _m_regionTextEdit.LimitText(TScenario::s_kMaxRegionDescLen);
    _m_regionNameStatic.SetWindowText(_m_pCampaign->getMapTraits().m_regionTraits[_m_scenario].m_name);
    if (_m_pCampaign->getScenario(_m_scenario).getMap() != NULL) {
        const TCampaignScenarioMap* pMap = _m_pCampaign->getScenario(_m_scenario).getMap();
        _m_scenarioNameStatic.SetWindowText(pMap->m_name.c_str());
        _m_mapFileStatic.SetWindowText(pMap->m_fileName.c_str());
        _m_importButton.ShowWindow(SW_HIDE);
        _fillPrerequisiteList();
        _checkPrerequisites();
        int difficulty = _m_pCampaign->getScenario(_m_scenario).getDifficulty();
        index = 0;
        while (_m_difficultyCombo.GetItemData(index) != difficulty)
            index++;
        _m_difficultyCombo.SetCurSel(index);
        CString regionDesc(_m_pCampaign->getScenario(_m_scenario).getRegionDesc().c_str());
        regionDesc.Replace("\n", "\r\n");
        _m_regionTextEdit.SetWindowText(regionDesc);
    } else {
        _m_scenarioNameStatic.SetWindowText(kNoneStr);
        _m_mapFileStatic.SetWindowText(kNoneStr);
        _m_refreshButton.ShowWindow(SW_HIDE);
        _m_exportButton.EnableWindow(FALSE);
        _m_removeButton.EnableWindow(FALSE);
        _m_prerequisiteList.EnableWindow(FALSE);
        _m_difficultyCombo.EnableWindow(FALSE);
        _m_regionTextEdit.EnableWindow(FALSE);
    }
    return TRUE;
}

VA(0x0042b7c0, 0x34a)
void TScenarioPropsGeneralPage::OnOK()
{
    CPropertyPage::OnOK();
    const TScenario& scenario = _m_pCampaign->getScenario(_m_scenario);
    _m_pCampaign->modifyScenario(_m_scenario)
        .setRegionColor(_m_regionColorCombo.GetItemData(_m_regionColorCombo.GetCurSel()));
    if (scenario.getMap() != NULL) {
        _m_pCampaign->modifyScenario(_m_scenario)
            .setDifficulty(_m_difficultyCombo.GetItemData(_m_difficultyCombo.GetCurSel()));
        CString regionDesc;
        _m_regionTextEdit.GetWindowText(regionDesc);
        regionDesc.Replace("\r\n", "\n");
        _m_pCampaign->modifyScenario(_m_scenario).setRegionDesc(std::string(regionDesc));
    }
    const TCampaign* pOldCampaign = _m_pDoc->getCampaign();
    const TScenario& oldScenario = pOldCampaign->getScenario(_m_scenario);
    const TScenario& newScenario = _m_pCampaign->getScenario(_m_scenario);
    _m_bModified = _m_bModified || _m_bMapChanged || newScenario.getRegionColor() != oldScenario.getRegionColor();
    if (!_m_bModified && scenario.getMap() != NULL) {
        if (newScenario.getDifficulty() != oldScenario.getDifficulty()
            || newScenario.getRegionDesc() != oldScenario.getRegionDesc()) {
            _m_bModified = true;
        } else {
            for (unsigned int prerequisite = 0; prerequisite < _m_pCampaign->getMapTraits().m_numRegions;
                 prerequisite++) {
                if (prerequisite != _m_scenario
                    && _m_pCampaign->getBDirectPrerequisite(_m_scenario, prerequisite)
                           != pOldCampaign->getBDirectPrerequisite(_m_scenario, prerequisite)) {
                    _m_bModified = true;
                    break;
                }
            }
        }
    }
}

VA(0x0042bb10, 0x1d5)
void TScenarioPropsGeneralPage::OnImportButton()
{
    CString pathName;
    if (promptMapFileName(NULL, TRUE, OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST, pathName)) {
        std::auto_ptr<TCampaignScenarioMap> pMap = _m_pDoc->loadScenarioMap(pathName);
        if (pMap.get() != NULL) {
            _m_bMapChanged = true;
            _m_pSheet->setMap(pMap);
            const TCampaignScenarioMap* pNewMap = _m_pCampaign->getScenario(_m_scenario).getMap();
            _m_scenarioNameStatic.SetWindowText(pNewMap->m_name.c_str());
            _m_mapFileStatic.SetWindowText(pNewMap->m_fileName.c_str());
            _m_importButton.ShowWindow(SW_HIDE);
            _m_refreshButton.ShowWindow(SW_SHOW);
            _m_exportButton.EnableWindow(TRUE);
            _m_removeButton.EnableWindow(TRUE);
            _m_prerequisiteList.EnableWindow(TRUE);
            _fillPrerequisiteList();
            _checkPrerequisites();
            _m_difficultyCombo.EnableWindow(TRUE);
            int index = 0;
            while (_m_difficultyCombo.GetItemData(index) != TScenario::eDifficultyNormal)
                index++;
            _m_difficultyCombo.SetCurSel(index);
            _m_regionTextEdit.EnableWindow(TRUE);
        }
    }
}

VA(0x0042bcf0, 0x146)
void TScenarioPropsGeneralPage::OnRefreshButton()
{
    const TCampaignScenarioMap* pMap = _m_pCampaign->getScenario(_m_scenario).getMap();
    CString pathName;
    if (promptMapFileName(pMap->m_fileName.c_str(), TRUE, OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST,
                          pathName)) {
        std::auto_ptr<TCampaignScenarioMap> pNewMap = _m_pDoc->loadScenarioMap(pathName);
        if (pNewMap.get() != NULL) {
            _m_bMapChanged = true;
            _m_pSheet->setMap(pNewMap);
            pMap = _m_pCampaign->getScenario(_m_scenario).getMap();
            _m_scenarioNameStatic.SetWindowText(pMap->m_name.c_str());
            _m_mapFileStatic.SetWindowText(pMap->m_fileName.c_str());
        }
    }
}

VA(0x0042be40, 0xc1)
void TScenarioPropsGeneralPage::OnExportButton()
{
    const TCampaignScenarioMap* pMap = _m_pCampaign->getScenario(_m_scenario).getMap();
    CString name(pMap->m_fileName.c_str());
    CString pathName;
    if (promptMapFileName(name, FALSE, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST,
                          pathName))
        _m_pDoc->exportScenarioMap(pMap, pathName);
}

VA(0x0042bf10, 0xd3)
void TScenarioPropsGeneralPage::OnRemoveButton()
{
    _m_bMapChanged = true;
    _m_pSheet->removeMap();
    _m_scenarioNameStatic.SetWindowText(kNoneStr);
    _m_mapFileStatic.SetWindowText(kNoneStr);
    _m_refreshButton.ShowWindow(SW_HIDE);
    _m_importButton.ShowWindow(SW_SHOW);
    _m_exportButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
    _m_prerequisiteList.ResetContent();
    _m_prerequisiteList.EnableWindow(FALSE);
    _m_difficultyCombo.SetCurSel(-1);
    _m_difficultyCombo.EnableWindow(FALSE);
    _m_regionTextEdit.SetWindowText("");
    _m_regionTextEdit.EnableWindow(FALSE);
}

VA(0x0042bff0, 0x62)
void TScenarioPropsGeneralPage::OnCheckChangePrerequisiteList()
{
    int index = _m_prerequisiteList.GetCurSel();
    int prerequisite = _m_prerequisiteList.GetItemData(index);
    _m_pCampaign->setBPrerequisite(_m_scenario, prerequisite, _m_prerequisiteList.GetCheck(index) != 0);
    _checkPrerequisites();
}
