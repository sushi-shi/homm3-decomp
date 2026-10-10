// ShrinePropsDlg.cpp - the shrine's spell dialog (h3maped
// 0x4b854e..0x4b8b94; GOG only). The list offers the spells of the
// shrine's level that belong to a school and are not special; DoModal
// stores the spell when OK changed it, and a double click on the list is
// an OK. The double click handler folds into TFlaggablePropsDlg's
// (0x41e573) and the implicit destructor into MapView's copy (0x481e37).
#include "editor/stdafx.h"

#include "va.h"
#include "editor/MapEditorText.h"
#include "editor/ObjectSpecializations.h"
#include "editor/ShrinePropsDlg.h"

VA(0x004b8722, 0x79)
TShrinePropsDlg::TShrinePropsDlg(CWnd* pParent, TShrine* pShrine)
    : CDialog(TShrinePropsDlg::IDD, pParent),
      _m_pShrine(pShrine),
      _m_bModified(false)
{
    _m_spellChoice = -1;
}

VA_COMPGEN(0x004b879b, 0x1c, SCALAR_DELETING_DTOR, TShrinePropsDlg)

VA(0x004b87b7, 0x40)
void TShrinePropsDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_SPELL_LIST, _m_spellList);
    DDX_Control(pDX, IDC_SPELL_STATIC, _m_spellGroup);
    DDX_Radio(pDX, IDC_RANDOM_SPELL_RADIO, _m_spellChoice);
}

VA(0x004b87f7, 0x6)
BEGIN_MESSAGE_MAP(TShrinePropsDlg, CDialog)
    ON_WM_CREATE()
    ON_BN_CLICKED(IDC_CUSTOM_SPELL_RADIO, OnCustomSpellRadio)
    ON_BN_CLICKED(IDC_RANDOM_SPELL_RADIO, OnRandomSpellRadio)
    ON_LBN_DBLCLK(IDC_SPELL_LIST, OnDblclkSpellList)
END_MESSAGE_MAP()

VA(0x004b87fd, 0x99)
int TShrinePropsDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    CString caption;
    caption.Format(kObjectPropertiesCaptionFmtStr, _m_pShrine->getTypeName().c_str());
    SetWindowText(caption);
    return 0;
}

VA(0x004b8896, 0x2a)
int TShrinePropsDlg::DoModal()
{
    int result = CDialog::DoModal();
    if (_m_bModified)
        _m_pShrine->setSpell(_m_spell);
    return result;
}

VA(0x004b88c0, 0x1f4)
BOOL TShrinePropsDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_SPELL_STATIC)->SetWindowText(SShrinePropsDlgText::kSpellStaticFmtStr);
    GetDlgItem(IDC_RANDOM_SPELL_RADIO)->SetWindowText(SShrinePropsDlgText::kRandomSpellRadioStr);
    GetDlgItem(IDC_CUSTOM_SPELL_RADIO)->SetWindowText(SShrinePropsDlgText::kCustomSpellRadioStr);
    _m_bModified = false;
    _m_spell = _m_pShrine->getSpell();
    _m_spellChoice = _m_spell != SPELL_NONE;
    CDialog::OnInitDialog();
    int level = _m_pShrine->getSpellLevel();
    CString format;
    _m_spellGroup.GetWindowText(format);
    CString text;
    text.Format(format, level);
    _m_spellGroup.SetWindowText(text);
    for (unsigned int spell = 0; spell < kNumSpells; spell++) {
        if (!(akSpellTraits[spell].m_flags & 0x2000) && akSpellTraits[spell].m_schoolBits != 0
            && akSpellTraits[spell].m_level == level)
            _m_spellList.SetItemData(_m_spellList.AddString(akSpellTraits[spell].m_name), spell);
    }
    if (_m_spellChoice) {
        int index = 0;
        while (_m_spellList.GetItemData(index) != _m_spell)
            index++;
        _m_spellList.SetCurSel(index);
        _m_spellList.SetTopIndex(index);
    } else {
        _m_spellList.EnableWindow(FALSE);
    }
    return TRUE;
}

VA(0x004b8ab4, 0x69)
void TShrinePropsDlg::OnOK()
{
    CDialog::OnOK();
    if (_m_spellChoice)
        _m_spell = ESpellId(_m_spellList.GetItemData(_m_spellList.GetCurSel()));
    else
        _m_spell = SPELL_NONE;
    _m_bModified = _m_bModified || _m_spell != _m_pShrine->getSpell();
}

VA(0x004b8b1d, 0x34)
void TShrinePropsDlg::OnCustomSpellRadio()
{
    if (!_m_spellChoice) {
        _m_spellChoice = 1;
        _m_spellList.EnableWindow(TRUE);
        _m_spellList.SetCurSel(0);
    }
}

VA(0x004b8b51, 0x43)
void TShrinePropsDlg::OnRandomSpellRadio()
{
    if (_m_spellChoice) {
        _m_spellChoice = 0;
        _m_spellList.SetCurSel(-1);
        _m_spellList.SetTopIndex(0);
        _m_spellList.EnableWindow(FALSE);
    }
}

void TShrinePropsDlg::OnDblclkSpellList()
{
    OnOK();
}
