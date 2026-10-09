// SpellsDlg.cpp - the spell check list (h3maped 0x4ba27f..0x4ba65e; GOG
// only). Only school spells are listed.
#include "editor/stdafx.h"

#include "exceptions.h"
#include "va.h"
#include "editor/MapEditorText.h"
#include "editor/SpellsDlg.h"

VA(0x004ba29b, 0x96)
TSpellsDlg::TSpellsDlg(CWnd* pParent, EGameVersion mapVersion)
    : CDialog(TSpellsDlg::IDD, pParent),
      _m_mapVersion(mapVersion)
{
    if (!Create(TSpellsDlg::IDD, pParent))
        throw TRuntimeError();
}

VA_COMPGEN(0x004ba331, 0x1c, SCALAR_DELETING_DTOR, TSpellsDlg)
VA_COMPGEN(0x004ba34d, 0x35, IMPLICIT_DTOR, TSpellsDlg)

VA(0x004ba382, 0x6c)
void TSpellsDlg::setSpells(const std::bitset<kNumSpells>& spells)
{
    _m_spells = spells;
    int count = _m_spellsList.GetCount();
    for (int index = 0; index < count; index++)
        _m_spellsList.SetCheck(index, _m_spells[_m_spellsList.GetItemData(index)]);
}

VA(0x004ba3ee, 0x81)
void TSpellsDlg::enableSpells(bool bEnable)
{
    int count;
    int index;
    if (bEnable) {
        if (!_m_bEnabled) {
            _m_bEnabled = true;
            count = _m_spellsList.GetCount();
            for (index = 0; index < count; index++)
                _m_spellsList.Enable(index, TRUE);
        }
    } else if (_m_bEnabled) {
        _m_bEnabled = false;
        count = _m_spellsList.GetCount();
        for (index = 0; index < count; index++)
            _m_spellsList.Enable(index, FALSE);
    }
}

VA(0x004ba46f, 0x15)
void TSpellsDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_SPELLS_LIST, _m_spellsList);
}

VA(0x004ba484, 0x6)
BEGIN_MESSAGE_MAP(TSpellsDlg, CDialog)
    ON_CLBN_CHKCHANGE(IDC_SPELLS_LIST, OnCheckChangeSpellsList)
END_MESSAGE_MAP()

VA(0x004ba48a, 0xc3)
BOOL TSpellsDlg::OnInitDialog()
{
    GetDlgItem(IDC_SPELLS_STATIC)->SetWindowText(SSpellsDlgText::kSpellsStaticStr);
    _m_spells = std::bitset<kNumSpells>();
    CDialog::OnInitDialog();
    CRect rect;
    _m_spellsList.GetClientRect(&rect);
    _m_spellsList.SetColumnWidth(rect.Width() / 2);
    for (int spell = 0; spell < kNumSpells; spell++) {
        if (!(akSpellTraits[spell].m_flags & 0x2000) && akSpellTraits[spell].m_schoolBits != 0)
            _m_spellsList.SetItemData(_m_spellsList.AddString(akSpellTraits[spell].m_name), spell);
    }
    _m_bEnabled = true;
    return TRUE;
}

VA(0x004ba54d, 0x111)
void TSpellsDlg::OnCheckChangeSpellsList()
{
    int curSel = _m_spellsList.GetCurSel();
    unsigned int spell = _m_spellsList.GetItemData(curSel);
    if (_m_mapVersion >= GAME_VERSION_SOD)
        _m_spells[spell] = _m_spellsList.GetCheck(curSel) != 0;
    else if (_m_spellsList.GetCheck(curSel)) {
        std::bitset<kNumSpells>::reference bSpell = _m_spells[spell];
        if (!bSpell) {
            bSpell = true;
            int count = _m_spellsList.GetCount();
            for (int index = 0; index < count; index++) {
                if (index != curSel && _m_spellsList.GetCheck(index)) {
                    unsigned int otherSpell = _m_spellsList.GetItemData(index);
                    _m_spellsList.SetCheck(index, 0);
                    _m_spells[otherSpell] = false;
                    return;
                }
            }
        }
    } else
        _m_spells[spell] = false;
}
