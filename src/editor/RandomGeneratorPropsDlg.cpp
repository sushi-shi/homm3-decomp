// RandomGeneratorPropsDlg.cpp - the properties dialogs of the random
// dwellings (h3maped 0x4afcfb..0x4b23cf; GOG only). The owner, level and
// alignment child dialogs each edit their part of the copy; a dwelling of
// random alignment either draws from the checked town types (at least one
// stays checked) or takes the type of a random town, listed nearest first
// from the dwelling's entrance, its own level's towns before the other's.
#include "editor/stdafx.h"

#include <math.h>
#include <stdio.h>

#include <algorithm>
#include <bitset>
#include <memory>
#include <vector>

#include "exceptions.h"
#include "va.h"
#include "editor/Clamp.h"
#include "editor/Digits.h"
#include "editor/FormattedString.h"
#include "editor/GameMap.h"
#include "editor/Generator.h"
#include "editor/MapEditorText.h"
#include "editor/Player.h"
#include "editor/RandomGeneratorPropsDlg.h"
#include "editor/Town.h"

namespace {

// The dwelling's owner.
class TRandomGeneratorOwnerDlg : public CDialog {
public:
    TRandomGeneratorOwnerDlg(const TFlaggableObject* pOldObject, TFlaggableObject* pNewObject);

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_RANDOM_GENERATOR_OWNER };

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    const TFlaggableObject* _m_pOldObject;
    TFlaggableObject* _m_pNewObject;
    bool _m_bModified;
    CComboBox _m_ownerCombo;
};

// The lowest and highest level the dwelling may take, shown from 1.
class TRandomGeneratorLevelDlg : public CDialog {
public:
    TRandomGeneratorLevelDlg(const TAbstractRandomlyLeveledGenerator* pOldGenerator,
                             TAbstractRandomlyLeveledGenerator* pNewGenerator);

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_RANDOM_GENERATOR_LEVEL };

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnKillFocusMinLevelEdit();
    afx_msg void OnKillFocusMaxLevelEdit();
    DECLARE_MESSAGE_MAP()

private:
    const TAbstractRandomlyLeveledGenerator* _m_pOldGenerator;
    TAbstractRandomlyLeveledGenerator* _m_pNewGenerator;
    bool _m_bModified;
    CEdit _m_minLevelEdit;
    CSpinButtonCtrl _m_minLevelSpin;
    CEdit _m_maxLevelEdit;
    CSpinButtonCtrl _m_maxLevelSpin;
};

// Orders town references by their distance from a location, the towns on
// the location's level first.
class TTownDistanceLess {
public:
    TTownDistanceLess(const TGameMap* pMap, const TTilePoint& loc, bool bSecondLayer)
        : _m_pMap(pMap), _m_loc(loc), _m_bSecondLayer(bSecondLayer) {}

    bool operator()(const TMapObjectRef& lhs, const TMapObjectRef& rhs) const;

private:
    const TGameMap* _m_pMap;
    TTilePoint _m_loc;
    bool _m_bSecondLayer;
};

// The town types the dwelling's alignment is drawn from, or the random
// town whose type it takes.
class TRandomGeneratorAlignmentDlg : public CDialog {
public:
    TRandomGeneratorAlignmentDlg(const TGameMap* pOldMap, TGameMap* pNewMap, bool bSecondLayer, unsigned int objID);

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_RANDOM_GENERATOR_ALIGNMENT };

    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnCastleCheck();
    afx_msg void OnRampartCheck();
    afx_msg void OnTowerCheck();
    afx_msg void OnInfernoCheck();
    afx_msg void OnNecropolisCheck();
    afx_msg void OnDungeonCheck();
    afx_msg void OnStrongholdCheck();
    afx_msg void OnFortressCheck();
    afx_msg void OnConfluxCheck();
    afx_msg void OnAlignmentRadio();
    DECLARE_MESSAGE_MAP()

private:
    void _onAlignmentCheck(int townType);
    const TGameMap& _getNewMap() const { return *_m_pNewMap; }
    const TAbstractRandomlyAlignedGenerator* _getOldGenerator() const;
    TAbstractRandomlyAlignedGenerator* _getNewGenerator();

    const TGameMap* _m_pOldMap;
    TGameMap* _m_pNewMap;
    bool _m_bSecondLayer;
    TMapLayerObjectID _m_objectID;
    const TAbstractRandomlyAlignedGenerator* _m_pOldGenerator;
    TAbstractRandomlyAlignedGenerator* _m_pNewGenerator;
    bool _m_bModified;
    std::vector<TMapObjectRef> _m_aTownRef;
    int _m_sameAsTown;
    std::bitset<kNumTownTypes> _m_alignments;
    CButton _m_aAlignmentCheck[kNumTownTypes];
    CButton _m_sameAsTownRadio;
    CListBox _m_townList;
};

}

VA(0x004afcfb, 0x53)
TRandomGeneratorOwnerDlg::TRandomGeneratorOwnerDlg(const TFlaggableObject* pOldObject, TFlaggableObject* pNewObject)
    : _m_pOldObject(pOldObject),
      _m_pNewObject(pNewObject),
      _m_bModified(false)
{
}

VA_COMPGEN(0x004afd4e, 0x1c, SCALAR_DELETING_DTOR, TRandomGeneratorOwnerDlg)
VA_COMPGEN(0x004afd6a, 0x35, IMPLICIT_DTOR, TRandomGeneratorOwnerDlg)

VA(0x004afd9f, 0x15)
void TRandomGeneratorOwnerDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_OWNER_COMBO, _m_ownerCombo);
}

VA(0x004afdb4, 0xc8)
BOOL TRandomGeneratorOwnerDlg::OnInitDialog()
{
    GetDlgItem(IDC_OWNER_COMBO_STATIC)->SetWindowText(SRandomGeneratorPropsDlgText::kOwnerStaticStr);
    _m_bModified = false;
    CDialog::OnInitDialog();
    _m_ownerCombo.SetItemData(_m_ownerCombo.AddString(kSelNoneStr), ePlayerNone);
    for (unsigned int player = 0; player < kNumPlayers; player++)
        _m_ownerCombo.SetItemData(_m_ownerCombo.AddString(akPlayerTraits[player].m_pName), player);
    TPlayer owner = _m_pNewObject->getOwner();
    int index = 0;
    while (TPlayer(_m_ownerCombo.GetItemData(index)) != owner)
        index++;
    _m_ownerCombo.SetCurSel(index);
    return TRUE;
}

VA(0x004afe7c, 0x53)
void TRandomGeneratorOwnerDlg::OnOK()
{
    CDialog::OnOK();
    _m_pNewObject->setOwner(TPlayer(_m_ownerCombo.GetItemData(_m_ownerCombo.GetCurSel())));
    _m_bModified = _m_bModified || _m_pNewObject->getOwner() != _m_pOldObject->getOwner();
}

VA(0x004afecf, 0x6)
BEGIN_MESSAGE_MAP(TRandomGeneratorLevelDlg, CDialog)
    ON_EN_KILLFOCUS(IDC_MIN_LEVEL_EDIT, OnKillFocusMinLevelEdit)
    ON_EN_KILLFOCUS(IDC_MAX_LEVEL_EDIT, OnKillFocusMaxLevelEdit)
END_MESSAGE_MAP()

VA(0x004afed5, 0x94)
TRandomGeneratorLevelDlg::TRandomGeneratorLevelDlg(const TAbstractRandomlyLeveledGenerator* pOldGenerator,
                                                   TAbstractRandomlyLeveledGenerator* pNewGenerator)
    : _m_pOldGenerator(pOldGenerator),
      _m_pNewGenerator(pNewGenerator)
{
}

VA_COMPGEN(0x004aff69, 0x1c, SCALAR_DELETING_DTOR, TRandomGeneratorLevelDlg)
VA_COMPGEN(0x004aff85, 0x65, IMPLICIT_DTOR, TRandomGeneratorLevelDlg)

VA(0x004affea, 0x52)
void TRandomGeneratorLevelDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_MIN_LEVEL_EDIT, _m_minLevelEdit);
    DDX_Control(pDX, IDC_MIN_LEVEL_SPIN, _m_minLevelSpin);
    DDX_Control(pDX, IDC_MAX_LEVEL_EDIT, _m_maxLevelEdit);
    DDX_Control(pDX, IDC_MAX_LEVEL_SPIN, _m_maxLevelSpin);
}

VA(0x004b003c, 0x14f)
BOOL TRandomGeneratorLevelDlg::OnInitDialog()
{
    GetDlgItem(IDC_CREATURE_LEVEL_STATIC)->SetWindowText(SRandomGeneratorPropsDlgText::kCreatureLevelStaticStr);
    GetDlgItem(IDC_MIN_LEVEL_STATIC)->SetWindowText(SRandomGeneratorPropsDlgText::kMinimumStaticStr);
    GetDlgItem(IDC_MAX_LEVEL_STATIC)->SetWindowText(SRandomGeneratorPropsDlgText::kMaximumStaticStr);
    _m_bModified = false;
    CDialog::OnInitDialog();
    _m_minLevelEdit.LimitText(TDigits<TAbstractRandomlyLeveledGenerator::s_kNumLevels>::getDigits());
    _m_minLevelSpin.SetRange(1, _m_pNewGenerator->getMaxLevel() + 1);
    _m_minLevelEdit.SetWindowText(TFormattedString("%d", _m_pNewGenerator->getMinLevel() + 1));
    _m_maxLevelEdit.LimitText(TDigits<TAbstractRandomlyLeveledGenerator::s_kNumLevels>::getDigits());
    _m_maxLevelSpin.SetRange(_m_pNewGenerator->getMinLevel() + 1, TAbstractRandomlyLeveledGenerator::s_kNumLevels);
    _m_maxLevelEdit.SetWindowText(TFormattedString("%d", _m_pNewGenerator->getMaxLevel() + 1));
    return TRUE;
}

VA(0x004b018b, 0x11e)
void TRandomGeneratorLevelDlg::OnOK()
{
    CDialog::OnOK();
    CString text;
    int minLevel = 0;
    _m_minLevelEdit.GetWindowText(text);
    sscanf(text, "%d", &minLevel);
    minLevel--;
    if (minLevel < 0 || minLevel >= TAbstractRandomlyLeveledGenerator::s_kNumLevels)
        minLevel = clamp(0, minLevel, TAbstractRandomlyLeveledGenerator::s_kNumLevels - 1);
    int maxLevel = 0;
    _m_maxLevelEdit.GetWindowText(text);
    sscanf(text, "%d", &maxLevel);
    maxLevel--;
    if (maxLevel < minLevel || maxLevel >= TAbstractRandomlyLeveledGenerator::s_kNumLevels)
        maxLevel = clamp(minLevel, maxLevel, TAbstractRandomlyLeveledGenerator::s_kNumLevels - 1);
    _m_pNewGenerator->setMinLevel(minLevel);
    _m_pNewGenerator->setMaxLevel(maxLevel);
    _m_bModified = _m_bModified || _m_pNewGenerator->getMinLevel() != _m_pOldGenerator->getMinLevel()
                   || _m_pNewGenerator->getMaxLevel() != _m_pOldGenerator->getMaxLevel();
}

VA(0x004b02a9, 0xea)
void TRandomGeneratorLevelDlg::OnKillFocusMinLevelEdit()
{
    CString text;
    int maxLevel = 0;
    _m_maxLevelEdit.GetWindowText(text);
    sscanf(text, "%d", &maxLevel);
    maxLevel--;
    int minLevel = 0;
    _m_minLevelEdit.GetWindowText(text);
    sscanf(text, "%d", &minLevel);
    minLevel--;
    if (minLevel < 0 || minLevel > maxLevel) {
        minLevel = clamp(0, minLevel, maxLevel);
        text.Format("%d", minLevel + 1);
        _m_minLevelEdit.SetWindowText(text);
    }
    _m_maxLevelSpin.SetRange(minLevel + 1, TAbstractRandomlyLeveledGenerator::s_kNumLevels);
}

VA(0x004b0393, 0xf0)
void TRandomGeneratorLevelDlg::OnKillFocusMaxLevelEdit()
{
    CString text;
    int minLevel = 0;
    _m_minLevelEdit.GetWindowText(text);
    sscanf(text, "%d", &minLevel);
    minLevel--;
    int maxLevel = 0;
    _m_maxLevelEdit.GetWindowText(text);
    sscanf(text, "%d", &maxLevel);
    maxLevel--;
    if (maxLevel < minLevel || maxLevel >= TAbstractRandomlyLeveledGenerator::s_kNumLevels) {
        maxLevel = clamp(minLevel, maxLevel, TAbstractRandomlyLeveledGenerator::s_kNumLevels - 1);
        text.Format("%d", maxLevel + 1);
        _m_maxLevelEdit.SetWindowText(text);
    }
    _m_minLevelSpin.SetRange(1, maxLevel + 1);
}

VA(0x004b0483, 0x10b)
bool TTownDistanceLess::operator()(const TMapObjectRef& lhs, const TMapObjectRef& rhs) const
{
    bool bLhsOnLevel = lhs.getBSecondLayer() == _m_bSecondLayer;
    bool bRhsOnLevel = rhs.getBSecondLayer() == _m_bSecondLayer;
    if (bLhsOnLevel != bRhsOnLevel)
        return bLhsOnLevel;
    const TGameMap::TLayer& layer = _m_pMap->getLayer(lhs.getBSecondLayer());
    TTilePoint lhsDelta = layer.getObjectLoc(lhs.getObjectID()) - layer.getObject(lhs.getObjectID()).getTriggerLoc() - _m_loc;
    int lhsDistanceSquared = lhsDelta.x() * lhsDelta.x() + lhsDelta.y() * lhsDelta.y();
    unsigned int lhsDistance = unsigned(sqrt(double(lhsDistanceSquared)) + 0.5);
    TTilePoint rhsDelta = layer.getObjectLoc(rhs.getObjectID()) - layer.getObject(rhs.getObjectID()).getTriggerLoc() - _m_loc;
    int rhsDistanceSquared = rhsDelta.x() * rhsDelta.x() + rhsDelta.y() * rhsDelta.y();
    unsigned int rhsDistance = unsigned(sqrt(double(rhsDistanceSquared)) + 0.5);
    return lhsDistance < rhsDistance;
}

VA(0x004b058e, 0x8)
void TRandomGeneratorAlignmentDlg::OnCastleCheck()
{
    _onAlignmentCheck(TOWN_CASTLE);
}

VA(0x004b0596, 0x8)
void TRandomGeneratorAlignmentDlg::OnRampartCheck()
{
    _onAlignmentCheck(TOWN_RAMPART);
}

VA(0x004b059e, 0x8)
void TRandomGeneratorAlignmentDlg::OnTowerCheck()
{
    _onAlignmentCheck(TOWN_TOWER);
}

VA(0x004b05a6, 0x8)
void TRandomGeneratorAlignmentDlg::OnInfernoCheck()
{
    _onAlignmentCheck(TOWN_INFERNO);
}

VA(0x004b05ae, 0x8)
void TRandomGeneratorAlignmentDlg::OnNecropolisCheck()
{
    _onAlignmentCheck(TOWN_NECROPOLIS);
}

VA(0x004b05b6, 0x8)
void TRandomGeneratorAlignmentDlg::OnDungeonCheck()
{
    _onAlignmentCheck(TOWN_DUNGEON);
}

VA(0x004b05be, 0x8)
void TRandomGeneratorAlignmentDlg::OnStrongholdCheck()
{
    _onAlignmentCheck(TOWN_STRONGHOLD);
}

VA(0x004b05c6, 0x8)
void TRandomGeneratorAlignmentDlg::OnFortressCheck()
{
    _onAlignmentCheck(TOWN_FORTRESS);
}

VA(0x004b05ce, 0x8)
void TRandomGeneratorAlignmentDlg::OnConfluxCheck()
{
    _onAlignmentCheck(TOWN_CONFLUX);
}

VA(0x004b05d6, 0x6)
BEGIN_MESSAGE_MAP(TRandomGeneratorAlignmentDlg, CDialog)
    ON_BN_CLICKED(IDC_ANY_ALIGNMENT_RADIO, OnAlignmentRadio)
    ON_BN_CLICKED(IDC_SAME_AS_TOWN_RADIO, OnAlignmentRadio)
    ON_BN_CLICKED(IDC_ALIGNMENT_CASTLE_CHECK, OnCastleCheck)
    ON_BN_CLICKED(IDC_ALIGNMENT_RAMPART_CHECK, OnRampartCheck)
    ON_BN_CLICKED(IDC_ALIGNMENT_TOWER_CHECK, OnTowerCheck)
    ON_BN_CLICKED(IDC_ALIGNMENT_INFERNO_CHECK, OnInfernoCheck)
    ON_BN_CLICKED(IDC_ALIGNMENT_NECROPOLIS_CHECK, OnNecropolisCheck)
    ON_BN_CLICKED(IDC_ALIGNMENT_DUNGEON_CHECK, OnDungeonCheck)
    ON_BN_CLICKED(IDC_ALIGNMENT_STRONGHOLD_CHECK, OnStrongholdCheck)
    ON_BN_CLICKED(IDC_ALIGNMENT_FORTRESS_CHECK, OnFortressCheck)
    ON_BN_CLICKED(IDC_ALIGNMENT_CONFLUX_CHECK, OnConfluxCheck)
END_MESSAGE_MAP()

VA(0x004b05dc, 0x1ed)
TRandomGeneratorAlignmentDlg::TRandomGeneratorAlignmentDlg(const TGameMap* pOldMap, TGameMap* pNewMap,
                                                           bool bSecondLayer, unsigned int objID)
    : _m_pOldMap(pOldMap),
      _m_pNewMap(pNewMap),
      _m_bSecondLayer(bSecondLayer),
      _m_objectID(objID),
      _m_pOldGenerator(_getOldGenerator()),
      _m_pNewGenerator(_getNewGenerator()),
      _m_bModified(false)
{
    unsigned int numLayers = _getNewMap().isTwoLayer() ? 2 : 1;
    for (unsigned int layerNum = 0; layerNum < numLayers; layerNum++) {
        const TGameMap::TLayer* pLayer = _getNewMap().getPLayer(layerNum);
        for (TGameMap::TLayer::TObjectIDIter iter = pLayer->objectIDBegin(); iter != pLayer->objectIDEnd(); ++iter) {
            const TTown* pTown = dynamic_cast<const TTown*>(pLayer->getPObject(*iter));
            if (pTown != NULL && pTown->getType() == RANDOM_TOWN)
                _m_aTownRef.push_back(TMapObjectRef(layerNum != 0, *iter));
        }
    }
    TTilePoint loc = _getNewMap().getLayer(_m_bSecondLayer).getObjectLoc(objID)
                     - _m_pNewGenerator->getTriggerLoc();
    std::sort(_m_aTownRef.begin(), _m_aTownRef.end(), TTownDistanceLess(_m_pNewMap, loc, _m_bSecondLayer));
}

VA_COMPGEN(0x004b07c9, 0x1c, SCALAR_DELETING_DTOR, TRandomGeneratorAlignmentDlg)
VA_COMPGEN(0x004b07e5, 0x6f, IMPLICIT_DTOR, TRandomGeneratorAlignmentDlg)

VA(0x004b0854, 0x7a)
void TRandomGeneratorAlignmentDlg::_onAlignmentCheck(int townType)
{
    _m_alignments.set(townType, _m_aAlignmentCheck[townType].GetCheck() != 0);
    if (_m_alignments.count() == 0) {
        townType = (townType + 1) % kNumTownTypes;
        _m_aAlignmentCheck[townType].SetCheck(1);
        _onAlignmentCheck(townType);
    }
}

VA(0x004b08ce, 0x39)
const TAbstractRandomlyAlignedGenerator* TRandomGeneratorAlignmentDlg::_getOldGenerator() const
{
    return dynamic_cast<const TAbstractRandomlyAlignedGenerator*>(
        _m_pOldMap->getLayer(_m_bSecondLayer).getPObject(_m_objectID));
}

VA(0x004b0907, 0x39)
TAbstractRandomlyAlignedGenerator* TRandomGeneratorAlignmentDlg::_getNewGenerator()
{
    return dynamic_cast<TAbstractRandomlyAlignedGenerator*>(_m_pNewMap->getLayer(_m_bSecondLayer).getPObject(_m_objectID));
}

VA(0x004b0940, 0x69)
void TRandomGeneratorAlignmentDlg::DoDataExchange(CDataExchange* pDX)
{
    DATA(0x00540e48) static const int akAlignmentCheckID[kNumTownTypes] = {
        IDC_ALIGNMENT_CASTLE_CHECK,     IDC_ALIGNMENT_RAMPART_CHECK, IDC_ALIGNMENT_TOWER_CHECK,
        IDC_ALIGNMENT_INFERNO_CHECK,    IDC_ALIGNMENT_NECROPOLIS_CHECK, IDC_ALIGNMENT_DUNGEON_CHECK,
        IDC_ALIGNMENT_STRONGHOLD_CHECK, IDC_ALIGNMENT_FORTRESS_CHECK, IDC_ALIGNMENT_CONFLUX_CHECK
    };
    DDX_Radio(pDX, IDC_ANY_ALIGNMENT_RADIO, _m_sameAsTown);
    for (int townType = 0; townType < kNumTownTypes; townType++)
        DDX_Control(pDX, akAlignmentCheckID[townType], _m_aAlignmentCheck[townType]);
    DDX_Control(pDX, IDC_SAME_AS_TOWN_RADIO, _m_sameAsTownRadio);
    DDX_Control(pDX, IDC_TOWN_LIST, _m_townList);
}

VA(0x004b09a9, 0x383)
BOOL TRandomGeneratorAlignmentDlg::OnInitDialog()
{
    GetDlgItem(IDC_ALIGNMENT_STATIC)->SetWindowText(SRandomGeneratorPropsDlgText::kAlignmentStaticStr);
    GetDlgItem(IDC_ANY_ALIGNMENT_RADIO)->SetWindowText(SRandomGeneratorPropsDlgText::kSelectOneOfRadioStr);
    GetDlgItem(IDC_SAME_AS_TOWN_RADIO)->SetWindowText(SRandomGeneratorPropsDlgText::kSameAsRadioStr);
    GetDlgItem(IDC_ALIGNMENT_CASTLE_CHECK)->SetWindowText(SRandomGeneratorPropsDlgText::kCastleCheckStr);
    GetDlgItem(IDC_ALIGNMENT_RAMPART_CHECK)->SetWindowText(SRandomGeneratorPropsDlgText::kRampartCheckStr);
    GetDlgItem(IDC_ALIGNMENT_TOWER_CHECK)->SetWindowText(SRandomGeneratorPropsDlgText::kTowerCheckStr);
    GetDlgItem(IDC_ALIGNMENT_INFERNO_CHECK)->SetWindowText(SRandomGeneratorPropsDlgText::kInfernoCheckStr);
    GetDlgItem(IDC_ALIGNMENT_NECROPOLIS_CHECK)->SetWindowText(SRandomGeneratorPropsDlgText::kNecropolisCheckStr);
    GetDlgItem(IDC_ALIGNMENT_DUNGEON_CHECK)->SetWindowText(SRandomGeneratorPropsDlgText::kDungeonCheckStr);
    GetDlgItem(IDC_ALIGNMENT_STRONGHOLD_CHECK)->SetWindowText(SRandomGeneratorPropsDlgText::kStrongholdCheckStr);
    GetDlgItem(IDC_ALIGNMENT_FORTRESS_CHECK)->SetWindowText(SRandomGeneratorPropsDlgText::kFortressCheckStr);
    GetDlgItem(IDC_ALIGNMENT_CONFLUX_CHECK)->SetWindowText(SRandomGeneratorPropsDlgText::kConfluxCheckStr);
    _m_bModified = false;
    _m_sameAsTown = _m_pNewGenerator->getTownLinkID() != TLinkableObject::s_kNoLinkID;
    _m_alignments = _m_pNewGenerator->getAlignments();
    CDialog::OnInitDialog();
    for (unsigned int townType = 0; townType < kNumTownTypes; townType++)
        _m_aAlignmentCheck[townType].SetCheck(_m_alignments[townType]);
    for (unsigned int i = 0; i < _m_aTownRef.size(); i++) {
        const TMapObjectRef& ref = _m_aTownRef[i];
        const TGameMap::TLayer& layer = _getNewMap().getLayer(ref.getBSecondLayer());
        const TTown* pTown = dynamic_cast<const TTown*>(layer.getPObject(ref.getObjectID()));
        TTilePoint loc = layer.getObjectLoc(ref.getObjectID()) - pTown->getTriggerLoc();
        TFormattedString name(kObjectAtLocationFmtStr, pTown->getTownTypeTraits().m_pName, loc.x(), loc.y(),
                              ref.getBSecondLayer());
        _m_townList.SetItemData(_m_townList.AddString(name), i);
    }
    if (_m_sameAsTown) {
        for (int townType = 0; townType < kNumTownTypes; townType++)
            _m_aAlignmentCheck[townType].EnableWindow(FALSE);
        int index;
        for (index = 0;; index++) {
            const TMapObjectRef& ref = _m_aTownRef[_m_townList.GetItemData(index)];
            const TTown* pTown =
                dynamic_cast<const TTown*>(_getNewMap().getLayer(ref.getBSecondLayer()).getPObject(ref.getObjectID()));
            if (pTown->getLinkID() == _m_pNewGenerator->getTownLinkID())
                break;
        }
        _m_townList.SetCurSel(index);
    } else {
        _m_townList.EnableWindow(FALSE);
        if (_m_aTownRef.empty())
            _m_sameAsTownRadio.EnableWindow(FALSE);
    }
    return TRUE;
}

VA(0x004b0d2c, 0x11e)
void TRandomGeneratorAlignmentDlg::OnOK()
{
    CDialog::OnOK();
    if (_m_sameAsTown) {
        const TMapObjectRef& townRef = _m_aTownRef[_m_townList.GetItemData(_m_townList.GetCurSel())];
        if (_m_pNewGenerator->getTownLinkID() == TLinkableObject::s_kNoLinkID) {
            _m_pNewMap->linkGeneratorToTown(TMapObjectRef(_m_bSecondLayer, _m_objectID), townRef);
        } else if (_m_pNewMap->getLinkableObjectRef(_m_pNewGenerator->getTownLinkID()) != townRef) {
            _m_pNewMap->unlinkGenerator(TMapObjectRef(_m_bSecondLayer, _m_objectID));
            _m_pNewMap->linkGeneratorToTown(TMapObjectRef(_m_bSecondLayer, _m_objectID), townRef);
        }
    } else if (_m_pNewGenerator->getTownLinkID() != TLinkableObject::s_kNoLinkID) {
        _m_pNewMap->unlinkGenerator(TMapObjectRef(_m_bSecondLayer, _m_objectID));
    }
    _m_pNewGenerator->setAlignments(_m_alignments);
    _m_bModified = _m_bModified || _m_pNewGenerator->getTownLinkID() != _m_pOldGenerator->getTownLinkID()
                   || _m_pNewGenerator->getAlignments() != _m_pOldGenerator->getAlignments();
}

VA(0x004b0e4a, 0xe6)
void TRandomGeneratorAlignmentDlg::OnAlignmentRadio()
{
    bool bWasSameAsTown = _m_sameAsTown != 0;
    UpdateData(TRUE);
    if (_m_sameAsTown) {
        if (!bWasSameAsTown) {
            for (unsigned int townType = 0; townType < kNumTownTypes; townType++) {
                if (!_m_aAlignmentCheck[townType].GetCheck()) {
                    _m_aAlignmentCheck[townType].SetCheck(1);
                    _m_alignments.set(townType);
                }
                _m_aAlignmentCheck[townType].EnableWindow(FALSE);
            }
            _m_townList.EnableWindow(TRUE);
            _m_townList.SetCurSel(0);
        }
    } else if (bWasSameAsTown) {
        _m_townList.SetCurSel(-1);
        _m_townList.EnableWindow(FALSE);
        for (int townType = 0; townType < kNumTownTypes; townType++)
            _m_aAlignmentCheck[townType].EnableWindow(TRUE);
    }
}

// The random dwelling's child dialogs.
struct TRandomGeneratorPropsDlg::_TDialogs {
    std::auto_ptr<TRandomGeneratorOwnerDlg> m_pOwnerDlg;
    std::auto_ptr<TRandomGeneratorLevelDlg> m_pLevelDlg;
    std::auto_ptr<TRandomGeneratorAlignmentDlg> m_pAlignmentDlg;
};

VA(0x004b0f30, 0x6)
BEGIN_MESSAGE_MAP(TRandomGeneratorPropsDlg, CDialog)
    ON_WM_CREATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

VA(0x004b0f36, 0x324)
TRandomGeneratorPropsDlg::TRandomGeneratorPropsDlg(CWnd* pParent, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
    : CDialog(TRandomGeneratorPropsDlg::IDD, pParent),
      _m_pMap(pMap),
      _m_pOldGenerator(NULL),
      _m_bModified(false)
{
    _m_pNewMap = std::auto_ptr<TGameMap>(new TGameMap(*_m_pMap));
    if (!_m_pNewMap.get())
        throw TAllocationFailure();
    const TGameMap* pOldMap = _m_pMap;
    _m_pOldGenerator = dynamic_cast<const TRandomGenerator*>(
        pOldMap->getLayer(bSecondLayer).getPObject(objID));
    TRandomGenerator* pNewGenerator = dynamic_cast<TRandomGenerator*>(
        _m_pNewMap->getLayer(bSecondLayer).getPObject(objID));
    _m_pDialogs = std::auto_ptr<_TDialogs>(new _TDialogs);
    if (!_m_pDialogs.get())
        throw TAllocationFailure();
    _m_pDialogs->m_pOwnerDlg =
        std::auto_ptr<TRandomGeneratorOwnerDlg>(new TRandomGeneratorOwnerDlg(_m_pOldGenerator, pNewGenerator));
    if (!_m_pDialogs->m_pOwnerDlg.get())
        throw TAllocationFailure();
    _m_pDialogs->m_pLevelDlg =
        std::auto_ptr<TRandomGeneratorLevelDlg>(new TRandomGeneratorLevelDlg(_m_pOldGenerator, pNewGenerator));
    if (!_m_pDialogs->m_pLevelDlg.get())
        throw TAllocationFailure();
    _m_pDialogs->m_pAlignmentDlg = std::auto_ptr<TRandomGeneratorAlignmentDlg>(
        new TRandomGeneratorAlignmentDlg(_m_pMap, _m_pNewMap.get(), bSecondLayer, objID));
    if (!_m_pDialogs->m_pAlignmentDlg.get())
        throw TAllocationFailure();
}

VA_COMPGEN(0x004b125a, 0x1c, SCALAR_DELETING_DTOR, TRandomGeneratorPropsDlg)

VA(0x004b128c, 0x4a)
TRandomGeneratorPropsDlg::~TRandomGeneratorPropsDlg()
{
}

VA(0x004b12d6, 0xeb)
int TRandomGeneratorPropsDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1
        || !_m_pDialogs->m_pOwnerDlg->Create(TRandomGeneratorOwnerDlg::IDD, this)
        || !_m_pDialogs->m_pLevelDlg->Create(TRandomGeneratorLevelDlg::IDD, this)
        || !_m_pDialogs->m_pAlignmentDlg->Create(TRandomGeneratorAlignmentDlg::IDD, this))
        return -1;
    CString caption;
    caption.Format(kObjectPropertiesCaptionFmtStr, _m_pOldGenerator->getTypeName().c_str());
    SetWindowText(caption);
    return 0;
}

VA(0x004b13c1, 0x2d)
void TRandomGeneratorPropsDlg::OnDestroy()
{
    _m_pDialogs->m_pAlignmentDlg->DestroyWindow();
    _m_pDialogs->m_pLevelDlg->DestroyWindow();
    _m_pDialogs->m_pOwnerDlg->DestroyWindow();
    CDialog::OnDestroy();
}

int TRandomGeneratorPropsDlg::DoModal()
{
    int result = CDialog::DoModal();
    if (_m_bModified)
        *_m_pMap = *_m_pNewMap;
    return result;
}

VA(0x004b13ee, 0x12d)
BOOL TRandomGeneratorPropsDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    _m_bModified = false;
    CDialog::OnInitDialog();
    CRect rect;
    CWnd* pFrame = GetDlgItem(IDC_FRAME);
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    _m_pDialogs->m_pOwnerDlg->SetWindowPos(pFrame, rect.left, rect.top, 0, 0, SWP_NOSIZE);
    pFrame->DestroyWindow();
    pFrame = GetDlgItem(IDC_SECOND_FRAME);
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    _m_pDialogs->m_pLevelDlg->SetWindowPos(pFrame, rect.left, rect.top, 0, 0, SWP_NOSIZE);
    pFrame->DestroyWindow();
    pFrame = GetDlgItem(IDC_THIRD_FRAME);
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    _m_pDialogs->m_pAlignmentDlg->SetWindowPos(pFrame, rect.left, rect.top, 0, 0, SWP_NOSIZE);
    pFrame->DestroyWindow();
    return TRUE;
}

VA(0x004b151b, 0x60)
void TRandomGeneratorPropsDlg::OnOK()
{
    CDialog::OnOK();
    _m_pDialogs->m_pOwnerDlg->OnOK();
    _m_pDialogs->m_pLevelDlg->OnOK();
    _m_pDialogs->m_pAlignmentDlg->OnOK();
    _m_bModified = _m_bModified || _m_pDialogs->m_pOwnerDlg->wasModified() || _m_pDialogs->m_pLevelDlg->wasModified()
                   || _m_pDialogs->m_pAlignmentDlg->wasModified();
}

// The randomly aligned dwelling's child dialogs.
struct TRandomlyAlignedGeneratorPropsDlg::_TDialogs {
    std::auto_ptr<TRandomGeneratorOwnerDlg> m_pOwnerDlg;
    std::auto_ptr<TRandomGeneratorAlignmentDlg> m_pAlignmentDlg;
};

VA(0x004b157b, 0x6)
BEGIN_MESSAGE_MAP(TRandomlyAlignedGeneratorPropsDlg, CDialog)
    ON_WM_CREATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

VA(0x004b1581, 0x28e)
TRandomlyAlignedGeneratorPropsDlg::TRandomlyAlignedGeneratorPropsDlg(CWnd* pParent, TGameMap* pMap,
                                                                     bool bSecondLayer, unsigned int objID)
    : CDialog(TRandomlyAlignedGeneratorPropsDlg::IDD, pParent),
      _m_pMap(pMap),
      _m_pOldGenerator(NULL),
      _m_bModified(false)
{
    _m_pNewMap = std::auto_ptr<TGameMap>(new TGameMap(*_m_pMap));
    if (!_m_pNewMap.get())
        throw TAllocationFailure();
    const TGameMap* pOldMap = _m_pMap;
    _m_pOldGenerator = dynamic_cast<const TRandomlyAlignedGenerator*>(
        pOldMap->getLayer(bSecondLayer).getPObject(objID));
    TRandomlyAlignedGenerator* pNewGenerator = dynamic_cast<TRandomlyAlignedGenerator*>(
        _m_pNewMap->getLayer(bSecondLayer).getPObject(objID));
    _m_pDialogs = std::auto_ptr<_TDialogs>(new _TDialogs);
    if (!_m_pDialogs.get())
        throw TAllocationFailure();
    _m_pDialogs->m_pOwnerDlg =
        std::auto_ptr<TRandomGeneratorOwnerDlg>(new TRandomGeneratorOwnerDlg(_m_pOldGenerator, pNewGenerator));
    if (!_m_pDialogs->m_pOwnerDlg.get())
        throw TAllocationFailure();
    _m_pDialogs->m_pAlignmentDlg = std::auto_ptr<TRandomGeneratorAlignmentDlg>(
        new TRandomGeneratorAlignmentDlg(_m_pMap, _m_pNewMap.get(), bSecondLayer, objID));
    if (!_m_pDialogs->m_pAlignmentDlg.get())
        throw TAllocationFailure();
}

VA_COMPGEN(0x004b180f, 0x1c, SCALAR_DELETING_DTOR, TRandomlyAlignedGeneratorPropsDlg)

VA(0x004b182b, 0x4a)
TRandomlyAlignedGeneratorPropsDlg::~TRandomlyAlignedGeneratorPropsDlg()
{
}

VA(0x004b1875, 0xd0)
int TRandomlyAlignedGeneratorPropsDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1
        || !_m_pDialogs->m_pOwnerDlg->Create(TRandomGeneratorOwnerDlg::IDD, this)
        || !_m_pDialogs->m_pAlignmentDlg->Create(TRandomGeneratorAlignmentDlg::IDD, this))
        return -1;
    CString caption;
    caption.Format(kObjectPropertiesCaptionFmtStr, _m_pOldGenerator->getTypeName().c_str());
    SetWindowText(caption);
    return 0;
}

VA(0x004b1945, 0x22)
void TRandomlyAlignedGeneratorPropsDlg::OnDestroy()
{
    _m_pDialogs->m_pAlignmentDlg->DestroyWindow();
    _m_pDialogs->m_pOwnerDlg->DestroyWindow();
    CDialog::OnDestroy();
}

VA(0x004b1967, 0x21)
int TRandomlyAlignedGeneratorPropsDlg::DoModal()
{
    int result = CDialog::DoModal();
    if (_m_bModified)
        *_m_pMap = *_m_pNewMap;
    return result;
}

VA(0x004b1988, 0xec)
BOOL TRandomlyAlignedGeneratorPropsDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    _m_bModified = false;
    CDialog::OnInitDialog();
    CRect rect;
    CWnd* pFrame = GetDlgItem(IDC_FRAME);
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    _m_pDialogs->m_pOwnerDlg->SetWindowPos(pFrame, rect.left, rect.top, 0, 0, SWP_NOSIZE);
    pFrame->DestroyWindow();
    pFrame = GetDlgItem(IDC_SECOND_FRAME);
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    _m_pDialogs->m_pAlignmentDlg->SetWindowPos(pFrame, rect.left, rect.top, 0, 0, SWP_NOSIZE);
    pFrame->DestroyWindow();
    return TRUE;
}

VA(0x004b1a74, 0x4a)
void TRandomlyAlignedGeneratorPropsDlg::OnOK()
{
    CDialog::OnOK();
    _m_pDialogs->m_pOwnerDlg->OnOK();
    _m_pDialogs->m_pAlignmentDlg->OnOK();
    _m_bModified = _m_bModified || _m_pDialogs->m_pOwnerDlg->wasModified()
                   || _m_pDialogs->m_pAlignmentDlg->wasModified();
}

// The randomly leveled dwelling's child dialogs.
struct TRandomlyLeveledGeneratorPropsDlg::_TDialogs {
    std::auto_ptr<TRandomGeneratorOwnerDlg> m_pOwnerDlg;
    std::auto_ptr<TRandomGeneratorLevelDlg> m_pLevelDlg;
};

VA(0x004b1abe, 0x6)
BEGIN_MESSAGE_MAP(TRandomlyLeveledGeneratorPropsDlg, CDialog)
    ON_WM_CREATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

VA(0x004b1ac4, 0x26c)
TRandomlyLeveledGeneratorPropsDlg::TRandomlyLeveledGeneratorPropsDlg(CWnd* pParent,
                                                                     TRandomlyLeveledGenerator* pGenerator)
    : CDialog(TRandomlyLeveledGeneratorPropsDlg::IDD, pParent),
      _m_pGenerator(pGenerator),
      _m_bModified(false)
{
    _m_pNewGenerator = std::auto_ptr<TRandomlyLeveledGenerator>(
        dynamic_cast<TRandomlyLeveledGenerator*>(_m_pGenerator->clone().release()));
    if (!_m_pNewGenerator.get())
        throw TAllocationFailure();
    _m_pDialogs = std::auto_ptr<_TDialogs>(new _TDialogs);
    if (!_m_pDialogs.get())
        throw TAllocationFailure();
    _m_pDialogs->m_pOwnerDlg = std::auto_ptr<TRandomGeneratorOwnerDlg>(
        new TRandomGeneratorOwnerDlg(_m_pGenerator, _m_pNewGenerator.get()));
    if (!_m_pDialogs->m_pOwnerDlg.get())
        throw TAllocationFailure();
    _m_pDialogs->m_pLevelDlg = std::auto_ptr<TRandomGeneratorLevelDlg>(
        new TRandomGeneratorLevelDlg(_m_pGenerator, _m_pNewGenerator.get()));
    if (!_m_pDialogs->m_pLevelDlg.get())
        throw TAllocationFailure();
}

VA_COMPGEN(0x004b1d42, 0x1c, SCALAR_DELETING_DTOR, TRandomlyLeveledGeneratorPropsDlg)

VA(0x004b1d5e, 0x4a)
TRandomlyLeveledGeneratorPropsDlg::~TRandomlyLeveledGeneratorPropsDlg()
{
}

VA(0x004b1da8, 0xd0)
int TRandomlyLeveledGeneratorPropsDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1
        || !_m_pDialogs->m_pOwnerDlg->Create(TRandomGeneratorOwnerDlg::IDD, this)
        || !_m_pDialogs->m_pLevelDlg->Create(TRandomGeneratorLevelDlg::IDD, this))
        return -1;
    CString caption;
    caption.Format(kObjectPropertiesCaptionFmtStr, _m_pGenerator->getTypeName().c_str());
    SetWindowText(caption);
    return 0;
}

VA(0x004b1e78, 0x22)
void TRandomlyLeveledGeneratorPropsDlg::OnDestroy()
{
    _m_pDialogs->m_pLevelDlg->DestroyWindow();
    _m_pDialogs->m_pOwnerDlg->DestroyWindow();
    CDialog::OnDestroy();
}

VA(0x004b1e9a, 0x21)
int TRandomlyLeveledGeneratorPropsDlg::DoModal()
{
    int result = CDialog::DoModal();
    if (_m_bModified)
        *_m_pGenerator = *_m_pNewGenerator;
    return result;
}

VA(0x004b1f90, 0xec)
BOOL TRandomlyLeveledGeneratorPropsDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    _m_bModified = false;
    CDialog::OnInitDialog();
    CRect rect;
    CWnd* pFrame = GetDlgItem(IDC_FRAME);
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    _m_pDialogs->m_pOwnerDlg->SetWindowPos(pFrame, rect.left, rect.top, 0, 0, SWP_NOSIZE);
    pFrame->DestroyWindow();
    pFrame = GetDlgItem(IDC_SECOND_FRAME);
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    _m_pDialogs->m_pLevelDlg->SetWindowPos(pFrame, rect.left, rect.top, 0, 0, SWP_NOSIZE);
    pFrame->DestroyWindow();
    return TRUE;
}

VA(0x004b207c, 0x4a)
void TRandomlyLeveledGeneratorPropsDlg::OnOK()
{
    CDialog::OnOK();
    _m_pDialogs->m_pOwnerDlg->OnOK();
    _m_pDialogs->m_pLevelDlg->OnOK();
    _m_bModified = _m_bModified || _m_pDialogs->m_pOwnerDlg->wasModified() || _m_pDialogs->m_pLevelDlg->wasModified();
}
