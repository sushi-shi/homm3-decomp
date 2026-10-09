// MapSpecsSpellsPage.cpp - the spells page of the map specifications
// sheet (h3maped 0x4765ed..0x476c1b; GOG only). Only school spells are
// listed; before Shadow of Death every spell is allowed and the list is
// read-only. Clearing a level's last allowed spell allows the next one of
// that level in the list.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/GameMap.h"
#include "editor/MapEditorText.h"
#include "editor/MapSpecsSpellsPage.h"

VA(0x004767c1, 0x9d)
TMapSpecsSpellsPage::TMapSpecsSpellsPage(const TGameMap& oldMap, TGameMap& newMap)
    : CPropertyPage(TMapSpecsSpellsPage::IDD),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_bModified(false)
{
    m_psp.pszTitle = m_strCaption = SMapSpecsSpellsPageText::kCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x0047685e, 0x1c, SCALAR_DELETING_DTOR, TMapSpecsSpellsPage)

VA(0x0047687a, 0x3e)
TMapSpecsSpellsPage::~TMapSpecsSpellsPage()
{
}

VA(0x004768b8, 0x18)
void TMapSpecsSpellsPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_AVAILABLE_SPELLS_LIST, _m_spellsList);
}

VA(0x004768d0, 0x6)
BEGIN_MESSAGE_MAP(TMapSpecsSpellsPage, CPropertyPage)
    ON_CLBN_CHKCHANGE(IDC_AVAILABLE_SPELLS_LIST, OnCheckChangeSpellsList)
END_MESSAGE_MAP()

VA(0x004768d6, 0x19e)
BOOL TMapSpecsSpellsPage::OnInitDialog()
{
    GetDlgItem(IDC_AVAILABLE_SPELLS_STATIC)->SetWindowText(SMapSpecsSpellsPageText::kAvailableSpellsStaticStr);
    _m_bModified = false;
    _m_disabledSpells = _m_newMap.getDisabledSpells();
    for (unsigned int level = 0; level < s_kNumSpellLevels; level++)
        _m_aNumAllowedSpells[level] = 0;
    CPropertyPage::OnInitDialog();
    CRect rect;
    _m_spellsList.GetClientRect(&rect);
    _m_spellsList.SetColumnWidth(rect.Width() / 2);
    int index;
    for (int spell = 0; spell < kNumSpells; spell++) {
        if (!(akSpellTraits[spell].m_flags & 0x2000) && akSpellTraits[spell].m_schoolBits != 0) {
            index = _m_spellsList.AddString(akSpellTraits[spell].m_name);
            _m_spellsList.SetItemData(index, spell);
        }
    }
    if (_m_newMap.getVersion() >= GAME_VERSION_SOD) {
        int count = _m_spellsList.GetCount();
        for (index = 0; index < count; index++) {
            unsigned int spell = _m_spellsList.GetItemData(index);
            if (!_m_disabledSpells[spell]) {
                _m_spellsList.SetCheck(index, 1);
                ++_m_aNumAllowedSpells[akSpellTraits[spell].m_level - 1];
            }
        }
    } else {
        int count = _m_spellsList.GetCount();
        for (index = 0; index < count; index++) {
            _m_spellsList.SetCheck(index, 1);
            _m_spellsList.Enable(index, FALSE);
        }
    }
    return TRUE;
}

VA(0x00476a74, 0x54)
void TMapSpecsSpellsPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_newMap.setDisabledSpells(_m_disabledSpells);
    _m_bModified = _m_bModified || _m_newMap.getDisabledSpells() != _m_oldMap.getDisabledSpells();
}

VA(0x00476ac8, 0x153)
void TMapSpecsSpellsPage::OnCheckChangeSpellsList()
{
    int curSel = _m_spellsList.GetCurSel();
    unsigned int spell = _m_spellsList.GetItemData(curSel);
    if (_m_spellsList.GetCheck(curSel)) {
        if (_m_disabledSpells[spell]) {
            _m_disabledSpells.set(spell, false);
            ++_m_aNumAllowedSpells[akSpellTraits[spell].m_level - 1];
        }
    } else if (!_m_disabledSpells[spell]) {
        _m_disabledSpells.set(spell, true);
        int level = akSpellTraits[spell].m_level;
        if (--_m_aNumAllowedSpells[level - 1] == 0) {
            int count = _m_spellsList.GetCount();
            int index = curSel;
            unsigned int otherSpell;
            do {
                if (++index >= count)
                    index = 0;
                otherSpell = _m_spellsList.GetItemData(index);
            } while (akSpellTraits[otherSpell].m_level != level);
            _m_spellsList.SetCheck(index, 1);
            _m_disabledSpells.set(otherSpell, false);
            ++_m_aNumAllowedSpells[level - 1];
        }
    }
}
