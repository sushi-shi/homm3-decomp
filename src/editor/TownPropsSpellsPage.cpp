// TownPropsSpellsPage.cpp - the spells page of the town property sheet
// (h3maped 0x4c6c3c..0x4c7653; Loki h3maped object 95). A guild level
// holds a fixed number of spells, so once that many must appear the other
// spells of the level cannot be made obligatory. Before Armageddon's Blade
// no spell can be made obligatory, and a spell the town type never offers
// cannot be allowed.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/FormattedString.h"
#include "editor/GameMap.h"
#include "editor/MapEditorText.h"
#include "editor/Town.h"
#include "editor/TownPropsSpellsPage.h"

// Each mage guild level's number of spells.
DATA(0x0054404c)
static const int g_akNumGuildLevelSpells[TTownPropsSpellsPage::s_kNumSpellLevels] = { 5, 4, 3, 2, 1 };

VA(0x004c6e10, 0x102)
TTownPropsSpellsPage::TTownPropsSpellsPage(const TGameMap& oldMap, TGameMap& newMap, bool bSecondLayer, unsigned int objID)
    : CPropertyPage(TTownPropsSpellsPage::IDD),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_bSecondLayer(bSecondLayer),
      _m_objectID(objID),
      _m_pOldTown(_getOldTown()),
      _m_bModified(false)
{
    m_psp.pszTitle = m_strCaption = kSpellsPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x004c6f12, 0x1c, SCALAR_DELETING_DTOR, TTownPropsSpellsPage)

VA(0x004c6f2e, 0x5f)
TTownPropsSpellsPage::~TTownPropsSpellsPage()
{
}

VA(0x004c6f8d, 0x1a2)
void TTownPropsSpellsPage::_fillSpellLists(unsigned int level)
{
    TTown* pNewTown = _getNewTown();
    const TTown::TTypeTraits& typeTraits = pNewTown->getTownTypeTraits();
    TTownType townType = pNewTown->getTownType();
    for (unsigned int spell = 0; spell < kNumSpells; spell++) {
        const TSpellTraits& traits = akSpellTraits[spell];
        if (!(traits.m_flags & 0x2000) && traits.m_schoolBits != 0 && traits.m_level - 1 == level) {
            int index = _m_obligatorySpellsList.AddString(traits.m_name);
            _m_obligatorySpellsList.SetItemData(index, spell);
            if (_m_newMap.getVersion() >= GAME_VERSION_AB) {
                if (_m_obligatorySpells[spell])
                    _m_obligatorySpellsList.SetCheck(index, 1);
                else {
                    _m_obligatorySpellsList.SetCheck(index, 0);
                    if (_m_aNumObligatorySpells[level] == g_akNumGuildLevelSpells[level])
                        _m_obligatorySpellsList.Enable(index, FALSE);
                }
            } else
                _m_obligatorySpellsList.Enable(index, FALSE);
            index = _m_disabledSpellsList.AddString(traits.m_name);
            _m_disabledSpellsList.SetItemData(index, spell);
            if (townType < kNumTownTypes && traits.m_townGetsItChance[townType] <= 0) {
                _m_disabledSpellsList.SetCheck(index, 0);
                _m_disabledSpellsList.Enable(index, FALSE);
            } else
                _m_disabledSpellsList.SetCheck(index, _m_disabledSpells[spell] ? 0 : 1);
        }
    }
}

VA(0x004c712f, 0x42)
const TTown* TTownPropsSpellsPage::_getOldTown() const
{
    const TGameMap::TLayer& layer = _m_oldMap.getLayer(_m_bSecondLayer);
    return dynamic_cast<const TTown*>(layer.getPObject(_m_objectID));
}

VA(0x004c7171, 0x42)
TTown* TTownPropsSpellsPage::_getNewTown()
{
    TGameMap::TLayer& layer = _m_newMap.getLayer(_m_bSecondLayer);
    return dynamic_cast<TTown*>(layer.getPObject(_m_objectID));
}

VA(0x004c71b3, 0x43)
void TTownPropsSpellsPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_SPELL_LEVEL_COMBO, _m_levelCombo);
    DDX_Control(pDX, IDC_OBLIGATORY_SPELLS_LIST, _m_obligatorySpellsList);
    DDX_Control(pDX, IDC_DISABLED_SPELLS_LIST, _m_disabledSpellsList);
}

VA(0x004c71f6, 0x6)
BEGIN_MESSAGE_MAP(TTownPropsSpellsPage, CPropertyPage)
    ON_CBN_SELCHANGE(IDC_SPELL_LEVEL_COMBO, OnSelChangeLevelCombo)
    ON_CLBN_CHKCHANGE(IDC_OBLIGATORY_SPELLS_LIST, OnCheckChangeObligatorySpellsList)
    ON_CLBN_CHKCHANGE(IDC_DISABLED_SPELLS_LIST, OnCheckChangeDisabledSpellsList)
END_MESSAGE_MAP()

VA(0x004c71fc, 0x185)
BOOL TTownPropsSpellsPage::OnInitDialog()
{
    GetDlgItem(IDC_SPELL_LEVEL_STATIC)->SetWindowText(STownPropsSpellsPageText::kSpellLevelStaticStr);
    GetDlgItem(IDC_OBLIGATORY_SPELLS_STATIC)->SetWindowText(STownPropsSpellsPageText::kMustAppearStaticStr);
    GetDlgItem(IDC_DISABLED_SPELLS_STATIC)->SetWindowText(STownPropsSpellsPageText::kMayAppearStaticStr);
    _m_bModified = false;
    TTown* pNewTown = _getNewTown();
    const TTown::TTypeTraits& typeTraits = pNewTown->getTownTypeTraits();
    _m_obligatorySpells = pNewTown->getObligatorySpellsMask();
    _m_disabledSpells = pNewTown->getDisabledSpellsMask();
    unsigned int level;
    for (level = 0; level < s_kNumSpellLevels; level++)
        _m_aNumObligatorySpells[level] = 0;
    for (unsigned int spell = 0; spell < kNumSpells; spell++) {
        if (_m_obligatorySpells[spell])
            ++_m_aNumObligatorySpells[akSpellTraits[spell].m_level - 1];
    }
    CPropertyPage::OnInitDialog();
    for (level = 0; level < s_kNumSpellLevels; level++) {
        if (typeTraits.hasMageGuildLevel(level)) {
            int index = _m_levelCombo.AddString(TFormattedString("%d", level + 1));
            _m_levelCombo.SetItemData(index, level);
        }
    }
    _m_levelCombo.SetCurSel(0);
    _fillSpellLists(_m_levelCombo.GetItemData(0));
    return TRUE;
}

VA(0x004c7381, 0x81)
void TTownPropsSpellsPage::OnOK()
{
    CPropertyPage::OnOK();
    TTown* pNewTown = _getNewTown();
    pNewTown->setObligatorySpellsMask(_m_obligatorySpells);
    pNewTown->setDisabledSpellsMask(_m_disabledSpells);
    _m_bModified = _m_bModified || pNewTown->getObligatorySpellsMask() != _m_pOldTown->getObligatorySpellsMask()
                   || pNewTown->getDisabledSpellsMask() != _m_pOldTown->getDisabledSpellsMask();
}

VA(0x004c7402, 0x52)
void TTownPropsSpellsPage::OnSelChangeLevelCombo()
{
    _m_obligatorySpellsList.ResetContent();
    _m_disabledSpellsList.ResetContent();
    _fillSpellLists(_m_levelCombo.GetItemData(_m_levelCombo.GetCurSel()));
}

VA(0x004c7454, 0x19d)
void TTownPropsSpellsPage::OnCheckChangeObligatorySpellsList()
{
    int curSel = _m_obligatorySpellsList.GetCurSel();
    unsigned int spell = _m_obligatorySpellsList.GetItemData(curSel);
    int level = akSpellTraits[spell].m_level - 1;
    bool bWasObligatory = _m_obligatorySpells[spell];
    bool bObligatory = _m_obligatorySpellsList.GetCheck(curSel) != 0;
    _m_obligatorySpells.set(spell, bObligatory);
    if (bObligatory) {
        if (!bWasObligatory && ++_m_aNumObligatorySpells[level] == g_akNumGuildLevelSpells[level]) {
            int count = _m_obligatorySpellsList.GetCount();
            for (int index = 0; index < count; index++) {
                if (index != curSel && !_m_obligatorySpells[_m_obligatorySpellsList.GetItemData(index)])
                    _m_obligatorySpellsList.Enable(index, FALSE);
            }
        }
    } else if (bWasObligatory && _m_aNumObligatorySpells[level]-- == g_akNumGuildLevelSpells[level]) {
        int count = _m_obligatorySpellsList.GetCount();
        for (int index = 0; index < count; index++) {
            if (index != curSel && !_m_obligatorySpells[_m_obligatorySpellsList.GetItemData(index)])
                _m_obligatorySpellsList.Enable(index, TRUE);
        }
    }
}

VA(0x004c75f1, 0x62)
void TTownPropsSpellsPage::OnCheckChangeDisabledSpellsList()
{
    int curSel = _m_disabledSpellsList.GetCurSel();
    unsigned int spell = _m_disabledSpellsList.GetItemData(curSel);
    _m_disabledSpells[spell] = !_m_disabledSpellsList.GetCheck(curSel);
}
