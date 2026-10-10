// SpellScrollPropsGeneralPage.cpp - the general page of the spell scroll
// property sheet (h3maped 0x4b9580..0x4b9bb2; GOG only). It shares the
// artifact general page's texts and help; a Restoration of Erathia map
// keeps 300 characters of the message.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/MapEditorText.h"
#include "editor/ObjectSpecializations.h"
#include "editor/SpellScrollPropsGeneralPage.h"

VA(0x004b9754, 0xd2)
TSpellScrollPropsGeneralPage::TSpellScrollPropsGeneralPage(TSpellScroll* pSpellScroll, EGameVersion mapVersion)
    : CPropertyPage(TSpellScrollPropsGeneralPage::IDD),
      _m_pSpellScroll(pSpellScroll),
      _m_mapVersion(mapVersion),
      _m_bModified(false)
{
    _m_typeName = _T("");
    _m_message = _T("");
    m_psp.pszTitle = m_strCaption = kGeneralPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
    m_nIDHelp = IDD_ARTIFACT_PROPS_GENERAL;
}

VA_COMPGEN(0x004b9826, 0x1c, SCALAR_DELETING_DTOR, TSpellScrollPropsGeneralPage)

VA(0x004b9842, 0x6e)
TSpellScrollPropsGeneralPage::~TSpellScrollPropsGeneralPage()
{
}

VA(0x004b98b0, 0x54)
void TSpellScrollPropsGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_SCROLL_SPELL_COMBO, _m_spellCombo);
    DDX_Control(pDX, IDC_MESSAGE_EDIT, _m_messageEdit);
    DDX_Text(pDX, IDC_ARTIFACT_TYPE_STATIC, _m_typeName);
    DDX_Text(pDX, IDC_MESSAGE_EDIT, _m_message);
}

VA(0x004b9904, 0x6)
BEGIN_MESSAGE_MAP(TSpellScrollPropsGeneralPage, CPropertyPage)
END_MESSAGE_MAP()

VA(0x004b990a, 0x1c7)
BOOL TSpellScrollPropsGeneralPage::OnInitDialog()
{
    GetDlgItem(IDC_TYPE_STATIC)->SetWindowText(SArtifactPropsGeneralPageText::kTypeStaticStr);
    GetDlgItem(IDC_MESSAGE_STATIC)->SetWindowText(SArtifactPropsGeneralPageText::kMessageStaticStr);
    GetDlgItem(IDC_NOTE_STATIC)->SetWindowText(SArtifactPropsGeneralPageText::kNoteStaticStr);
    GetDlgItem(IDC_SPELL_STATIC)->SetWindowText(SArtifactPropsGeneralPageText::kSpellStaticStr);
    _m_bModified = false;
    _m_typeName = _m_pSpellScroll->getTypeName().c_str();
    _m_message = _m_pSpellScroll->getMessage().c_str();
    _m_message.Replace("\n", "\r\n");
    _m_spell = _m_pSpellScroll->getSpell();
    CPropertyPage::OnInitDialog();
    if (_m_mapVersion == GAME_VERSION_ROE)
        _m_messageEdit.LimitText(TTreasure::s_kMaxMessageLen);
    for (unsigned int spell = 0; spell < kNumSpells; spell++) {
        if (!(akSpellTraits[spell].m_flags & 0x2000) && akSpellTraits[spell].m_schoolBits != 0)
            _m_spellCombo.SetItemData(_m_spellCombo.AddString(akSpellTraits[spell].m_name), spell);
    }
    int index = 0;
    while (_m_spellCombo.GetItemData(index) != _m_spell)
        index++;
    _m_spellCombo.SetCurSel(index);
    return TRUE;
}

VA(0x004b9ad1, 0xe1)
void TSpellScrollPropsGeneralPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_message.Replace("\r\n", "\n");
    _m_spell = ESpellId(_m_spellCombo.GetItemData(_m_spellCombo.GetCurSel()));
    _m_bModified = _m_bModified || std::string(_m_message) != _m_pSpellScroll->getMessage()
                   || _m_spell != _m_pSpellScroll->getSpell();
}
