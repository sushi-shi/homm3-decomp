// EditTownEventCreaturesPage.cpp - the creatures page of the town event
// sheet (h3maped 0x417b32..0x4181c2; Loki h3maped object 99): a 0..s_kMax
// growth bonus for each creature generator. Each label's dialog text is a
// format for the generator's base and upgraded creature names.
#include "editor/stdafx.h"

#include <stdio.h>

#include "va.h"
#include "editor/Digits.h"
#include "editor/EditTownEventCreaturesPage.h"
#include "editor/MapEditorText.h"

DATA(0x00534678)
static const int g_aakGeneratorControlIDs[TTown::s_kNumGeneratorTypes][3] = {
    { IDC_GENERATOR1_STATIC, IDC_QTY_EDIT1, IDC_QTY_SPIN1 },
    { IDC_GENERATOR2_STATIC, IDC_QTY_EDIT2, IDC_QTY_SPIN2 },
    { IDC_GENERATOR3_STATIC, IDC_QTY_EDIT3, IDC_QTY_SPIN3 },
    { IDC_GENERATOR4_STATIC, IDC_QTY_EDIT4, IDC_QTY_SPIN4 },
    { IDC_GENERATOR5_STATIC, IDC_QTY_EDIT5, IDC_QTY_SPIN5 },
    { IDC_GENERATOR6_STATIC, IDC_QTY_EDIT6, IDC_QTY_SPIN6 },
    { IDC_GENERATOR7_STATIC, IDC_QTY_EDIT7, IDC_QTY_SPIN7 }
};

VA(0x00417d22, 0xaf)
TEditTownEventCreaturesPage::TEditTownEventCreaturesPage(const TTown::TTimedEvent& event, TTownType townType)
    : CPropertyPage(TEditTownEventCreaturesPage::IDD),
      _m_event(event),
      _m_townType(townType),
      _m_bModified(false)
{
    m_psp.pszTitle = m_strCaption = kCreaturesPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x00417dd1, 0x1c, SCALAR_DELETING_DTOR, TEditTownEventCreaturesPage)
VA_COMPGEN(0x00417ded, 0x55, CLASS_CTOR, _TGeneratorControls)
VA_COMPGEN(0x00417e42, 0x44, IMPLICIT_DTOR, _TGeneratorControls)

VA(0x00417e86, 0x57)
TEditTownEventCreaturesPage::~TEditTownEventCreaturesPage()
{
}

VA(0x00417edd, 0x4c)
void TEditTownEventCreaturesPage::DoDataExchange(CDataExchange* pDX)
{
    for (unsigned int i = 0; i < TTown::s_kNumGeneratorTypes; i++) {
        DDX_Control(pDX, g_aakGeneratorControlIDs[i][0], _m_aGeneratorControls[i].m_nameStatic);
        DDX_Control(pDX, g_aakGeneratorControlIDs[i][1], _m_aGeneratorControls[i].m_bonusEdit);
        DDX_Control(pDX, g_aakGeneratorControlIDs[i][2], _m_aGeneratorControls[i].m_bonusSpin);
    }
}

VA(0x00417f29, 0x6)
BEGIN_MESSAGE_MAP(TEditTownEventCreaturesPage, CPropertyPage)
END_MESSAGE_MAP()

VA(0x00417f2f, 0x1ae)
BOOL TEditTownEventCreaturesPage::OnInitDialog()
{
    GetDlgItem(IDC_GENERATOR_NO_STATIC)->SetWindowText(SEditTownEventCreaturesPageText::kGeneratorNoStaticStr);
    GetDlgItem(IDC_QUANTITY_STATIC)->SetWindowText(SEditTownEventCreaturesPageText::kQuantityStaticStr);
    GetDlgItem(IDC_NOTE_STATIC)->SetWindowText(SEditTownEventCreaturesPageText::kNoteStaticStr);
    _m_bModified = false;
    _m_generatorBonuses = _m_event.getGeneratorBonuses();
    CPropertyPage::OnInitDialog();
    for (unsigned int i = 0; i < TTown::s_kNumGeneratorTypes; i++) {
        CString nameFormat;
        _m_aGeneratorControls[i].m_nameStatic.GetWindowText(nameFormat);
        const TTown::TGeneratorTraits* pTraits = TTown::s_akTypeTraits[_m_townType].m_apGeneratorTraits[i];
        CString name;
        name.Format(nameFormat, pTraits->getBaseCreatureName(), pTraits->getUpgradeCreatureName());
        _m_aGeneratorControls[i].m_nameStatic.SetWindowText(name);
        _m_aGeneratorControls[i].m_bonusEdit.LimitText(TDigits<TTown::TGeneratorBonuses::s_kMax>::getDigits());
        CString bonus;
        bonus.Format("%d", _m_generatorBonuses.get(TTown::TGeneratorType(i)));
        _m_aGeneratorControls[i].m_bonusEdit.SetWindowText(bonus);
        _m_aGeneratorControls[i].m_bonusSpin.SetRange(0, TTown::TGeneratorBonuses::s_kMax);
    }
    return TRUE;
}

VA(0x004180dd, 0xc9)
void TEditTownEventCreaturesPage::OnOK()
{
    CPropertyPage::OnOK();
    for (unsigned int i = 0; i < TTown::s_kNumGeneratorTypes; i++) {
        int bonus = 0;
        CString text;
        _m_aGeneratorControls[i].m_bonusEdit.GetWindowText(text);
        sscanf(text, "%d", &bonus);
        _m_generatorBonuses.set(TTown::TGeneratorType(i), bonus);
    }
    _m_bModified = _m_bModified || _m_generatorBonuses != _m_event.getGeneratorBonuses();
}
