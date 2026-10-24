// EditArtifactDlg.cpp - the artifact editor of the hero artifacts page
// (h3maped 0x40d6b1..0x40ddd9; Loki h3maped object 82). The where combo
// lists the backpack (unless full) and the unused slots; the artifact
// combo lists the version's artifacts but the spellbook, the spell scroll
// and the disabled ones that fit the chosen slot (all of them for the
// backpack). OK needs an artifact.
#include "editor/stdafx.h"

#include <algorithm>
#include <functional>

#include "artifact.h"
#include "const_bitset_iterator.h"
#include "va.h"
#include "editor/EditArtifactDlg.h"
#include "editor/MapEditorText.h"

// The backpack-full search reads the borrowed const mask. Const traversal
// matches retail 0x40dd78; mutable proxy iteration adds two cached words.
VA(0x0040d6d5, 0xde)
TEditArtifactDlg::TEditArtifactDlg(CWnd* pParent, const TSlots& unusedSlots,
                                   const TSlotClassCounts& slotClassCounts, bool bBackpackFull,
                                   EGameVersion mapVersion)
    : CDialog(TEditArtifactDlg::IDD, pParent),
      _m_artifact(ARTIFACT_NONE),
      _m_slot(TArtifactSlot(s_kBackpackSlot)),
      _m_unusedSlots(unusedSlots),
      _m_slotClassCounts(slotClassCounts),
      _m_bBackpackFull(bBackpackFull),
      _m_mapVersion(mapVersion)
{
    if (_m_bBackpackFull)
        _m_slot = TArtifactSlot(std::find_if(TConstBitsetIterator<kNumArtifactSlots + 1>(_m_unusedSlots, 0),
                                             TConstBitsetIterator<kNumArtifactSlots + 1>(_m_unusedSlots, s_kBackpackSlot),
                                             std::bind2nd(std::not_equal_to<bool>(), false))
                                    .position());
}

// Folded by /OPT:ICF: the scalar deleting destructor calls an identical
// implicit destructor of another dialog.
VA_COMPGEN(0x0040d7b3, 0x1c, SCALAR_DELETING_DTOR, TEditArtifactDlg)

VA(0x0040d7cf, 0xae)
TEditArtifactDlg::TEditArtifactDlg(CWnd* pParent, const TSlots& unusedSlots,
                                   const TSlotClassCounts& slotClassCounts, bool bBackpackFull,
                                   EGameVersion mapVersion, TArtifact artifact, TArtifactSlot slot)
    : CDialog(TEditArtifactDlg::IDD, pParent),
      _m_artifact(artifact),
      _m_slot(slot),
      _m_unusedSlots(unusedSlots),
      _m_slotClassCounts(slotClassCounts),
      _m_bBackpackFull(bBackpackFull),
      _m_mapVersion(mapVersion)
{
}

VA(0x0040d87d, 0x131)
void TEditArtifactDlg::_setArtifacts()
{
    for (int count = _m_artifactCombo.GetCount(); count-- > 0;)
        _m_artifactCombo.DeleteString(0);
    int numArtifacts;
    if (_m_mapVersion >= GAME_VERSION_SOD)
        numArtifacts = 144;
    else
        numArtifacts = _m_mapVersion >= GAME_VERSION_AB ? 129 : 127;
    for (int artifact = 0; artifact < numArtifacts; artifact++) {
        if (artifact == ARTIFACT_SPELL_SCROLL || artifact == 0)
            continue;
        if (akArtifactTraits[artifact].m_disabled)
            continue;
        if (_m_slot >= s_kBackpackSlot || _isAllowed(TArtifact(artifact), _m_slot))
            _m_artifactCombo.SetItemData(_m_artifactCombo.AddString(akArtifactTraits[artifact].m_name), artifact);
    }
    if (_m_artifact != ARTIFACT_NONE) {
        int count = _m_artifactCombo.GetCount();
        int index;
        for (index = 0; index < count; index++) {
            if (_m_artifactCombo.GetItemData(index) == _m_artifact)
                break;
        }
        if (index < count)
            _m_artifactCombo.SetCurSel(index);
        else {
            _m_artifact = ARTIFACT_NONE;
            _m_okButton.EnableWindow(FALSE);
        }
    }
}

// Whether the artifact may go in the slot: the slot is of its class and,
// for a combination artifact, each component's class keeps an unused slot
// beyond those the worn combination parts take.
// Retail's true exit precedes the outer false exit. The positive slot guard
// reproduces that ordering. Remaining: artifactAllowedInSlot stays out of line
// here while retail expands it to bitset::operator[]; iterator/count register
// homes also differ. An explicit find_if/break scan does not improve this form.
// The /O1 C2 trace vetoes the helper 17/12 (EH allowance spent) because its
// nested operator[] passes its own veto 11/12 with test expanded; retail
// keeps operator[] const (0x405a77) out of line, which would leave the outer
// count under the limit. Dropping the helper's asserts or its named mask
// reference leaves the 17/12 veto unchanged.
VA(0x0040d9ae, 0x11b)
bool TEditArtifactDlg::_isAllowed(TArtifact artifact, TArtifactSlot slot) const
{
    if (artifactAllowedInSlot(artifact, slot)) {
        int combination = akArtifactTraits[artifact].m_comboType;
        if (combination == -1)
            return true;
        TSlotClassCounts slotClassCounts(_m_slotClassCounts);
        const std::bitset<ARTIFACT_COUNT>& components = g_combinationArtifacts[combination].m_components;
        TConstBitsetIterator<ARTIFACT_COUNT> current(components, 0);
        TConstBitsetIterator<ARTIFACT_COUNT> end(components, ARTIFACT_COUNT);
        for (; (current = std::find_if(current, end, std::bind2nd(std::not_equal_to<bool>(), false))) != end;
             ++current) {
            const TArtifactTraits& componentTraits = akArtifactTraits[current.position()];
            unsigned int count = ++slotClassCounts[componentTraits.m_allowableSlotMask];
            if (count > (_m_unusedSlots & g_artifactSlotMasks[componentTraits.m_allowableSlotMask]).count())
                return false;
        }
        return true;
    }
    return false;
}

VA(0x0040dac9, 0x3d)
void TEditArtifactDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDOK, _m_okButton);
    DDX_Control(pDX, IDC_ARTIFACT_COMBO, _m_artifactCombo);
    DDX_Control(pDX, IDC_WHERE_COMBO, _m_whereCombo);
}

VA(0x0040db06, 0x6)
BEGIN_MESSAGE_MAP(TEditArtifactDlg, CDialog)
    ON_CBN_SELCHANGE(IDC_WHERE_COMBO, OnSelChangeWhereCombo)
    ON_CBN_SELCHANGE(IDC_ARTIFACT_COMBO, OnSelChangeArtifactCombo)
    ON_WM_CREATE()
END_MESSAGE_MAP()

VA(0x0040db0c, 0x25)
int TEditArtifactDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(kEditArtifactCaptionStr);
    return 0;
}

VA(0x0040db31, 0x1a6)
BOOL TEditArtifactDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_EQUIP_WHERE_STATIC)->SetWindowText(SEditArtifactDlgText::kEquipWhereStaticStr);
    GetDlgItem(IDC_ARTIFACT_STATIC)->SetWindowText(SEditArtifactDlgText::kArtifactStaticStr);
    CDialog::OnInitDialog();
    if (!_m_bBackpackFull)
        _m_whereCombo.SetItemData(_m_whereCombo.AddString(kBackpackStr), s_kBackpackSlot);
    int numSlots = _m_mapVersion >= GAME_VERSION_SOD ? kNumArtifactSlots + 1 : kNumArtifactSlots;
    for (int slot = 0; slot < numSlots; slot++) {
        if (_m_unusedSlots.test(slot))
            _m_whereCombo.SetItemData(_m_whereCombo.AddString(akArtifactSlotTraits[slot].m_name), slot);
    }
    int index;
    for (index = 0; _m_whereCombo.GetItemData(index) != _m_slot; index++)
        ;
    _m_whereCombo.SetCurSel(index);
    _m_okButton.EnableWindow(_m_artifact != ARTIFACT_NONE);
    _setArtifacts();
    return TRUE;
}

VA(0x0040dcd7, 0x3b)
void TEditArtifactDlg::OnSelChangeWhereCombo()
{
    _m_slot = TArtifactSlot(_m_whereCombo.GetItemData(_m_whereCombo.GetCurSel()));
    _setArtifacts();
}

VA(0x0040dd12, 0x66)
void TEditArtifactDlg::OnSelChangeArtifactCombo()
{
    if (_m_artifactCombo.GetCurSel() >= 0) {
        _m_artifact = TArtifact(_m_artifactCombo.GetItemData(_m_artifactCombo.GetCurSel()));
        _m_okButton.EnableWindow(TRUE);
    } else if (_m_artifact != ARTIFACT_NONE) {
        _m_artifact = ARTIFACT_NONE;
        _m_okButton.EnableWindow(FALSE);
    }
}
