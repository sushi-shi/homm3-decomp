// MapSpecsVictoryCondPage.cpp - the special victory condition page of the
// map specifications sheet (h3maped 0x478fac..0x47c4f5). The page creates a
// child dialog per condition type in its frame and shows the chosen type's;
// the dialogs list the map's towns, heroes and monsters as "name at
// (x, y)", the artifacts and creatures of the map's version, or the
// resources. The loss condition page's radio, data exchange and type switch
// fold with this page's.
#include "editor/stdafx.h"

#include <stdio.h>

#include "adventureobjecttype.h"
#include "artifact.h"
#include "objnames.h"
#include "retailobjecttype.h"
#include "va.h"
#include "editor/Clamp.h"
#include "editor/DialogTemplate.h"
#include "editor/Digits.h"
#include "editor/GameObject.h"
#include "editor/GameResource.h"
#include "editor/Hero.h"
#include "editor/MapEditorText.h"
#include "editor/MapSpecsVictoryCondPage.h"
#include "editor/Monster.h"
#include "editor/Town.h"

namespace {

enum {
    // The artifacts and creatures each game version offers.
    kNumRoEArtifacts = 127,
    kNumABArtifacts = 129,
    kNumRoECreatures = CREATURE_PIXIE,
    kNumCreatures = CREATURE_CATAPULT,
    kMaxCreatureQuantity = 99999,
    kMaxResourceQuantity = 9999999
};

// The "normal victory too" and "computer players too" checks; a
// condition shows the ones it supports.
class TVictoryConditionGeneralDlg : public CDialog {
public:
    enum { IDD = IDD_VICTORY_CONDITION_GENERAL };

    TVictoryConditionGeneralDlg(bool bShowAllowNormalVictory, bool bShowAppliesToComputer)
        : _m_bShowAllowNormalVictory(bShowAllowNormalVictory), _m_bShowAppliesToComputer(bShowAppliesToComputer) {}

    virtual void OnOK() { CDialog::OnOK(); }
    void setVictoryCondition(const TVictoryCondition& vc);

private:
    bool _m_bShowAllowNormalVictory;
    bool _m_bShowAppliesToComputer;

public:
    int _m_allowNormalVictory;
    int _m_appliesToComputer;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
};

}

// A child dialog of the victory condition page, with its general dialog
// in its frame (RTTI TVictoryConditionDlg).
class TVictoryConditionDlg : public CDialog {
public:
    TVictoryConditionDlg(bool bShowAllowNormalVictory, bool bShowAppliesToComputer, bool bGeneral);
    virtual ~TVictoryConditionDlg();

    virtual void OnOK();
    virtual std::auto_ptr<TVictoryCondition> getVictoryCondition() const = 0;
    void setVictoryCondition(const TVictoryCondition& vc)
    {
        if (_m_pGeneralDlg != NULL)
            _m_pGeneralDlg->setVictoryCondition(vc);
    }

protected:
    virtual BOOL OnInitDialog();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()

    TVictoryConditionGeneralDlg* _m_pGeneralDlg;
};

namespace {

// No victory condition: an empty dialog.
class TNullVictoryConditionDlg : public TVictoryConditionDlg {
public:
    TNullVictoryConditionDlg() : TVictoryConditionDlg(false, false, false) {}

    BOOL Create(CWnd* pParentWnd)
    {
        TEmptyDialogTemplate dialogTemplate;
        return CreateIndirect(&dialogTemplate, pParentWnd);
    }

    virtual std::auto_ptr<TVictoryCondition> getVictoryCondition() const
    {
        return std::auto_ptr<TVictoryCondition>(NULL);
    }
};

// Acquiring an artifact: a combo of the version's artifacts.
class TAquireArtifactDlg : public TVictoryConditionDlg {
public:
    enum { IDD = IDD_AQUIRE_ARTIFACT };

    TAquireArtifactDlg(EGameVersion mapVersion);

    virtual void OnOK();
    virtual std::auto_ptr<TVictoryCondition> getVictoryCondition() const;
    void setVictoryCondition(const TVCAquireArtifact& vc);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    EGameVersion _m_mapVersion;
    CComboBox _m_artifactCombo;
    TArtifact _m_artifact;
};

// Accumulating creatures: a combo of the version's creatures and a
// quantity.
class TAccumulateCreatureDlg : public TVictoryConditionDlg {
public:
    enum { IDD = IDD_ACCUMULATE_CREATURES };

    TAccumulateCreatureDlg(EGameVersion mapVersion);

    virtual void OnOK();
    virtual std::auto_ptr<TVictoryCondition> getVictoryCondition() const;
    void setVictoryCondition(const TVCAccumulateCreature& vc);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnKillFocusQuantityEdit();
    DECLARE_MESSAGE_MAP()

private:
    EGameVersion _m_mapVersion;
    CComboBox _m_creatureCombo;
    CEdit _m_quantityEdit;
    CSpinButtonCtrl _m_quantitySpin;
    TCreatureType _m_creatureType;
    unsigned int _m_quantity;
};

// Accumulating a resource: a combo of the resources and a quantity.
class TAccumulateResourceDlg : public TVictoryConditionDlg {
public:
    enum { IDD = IDD_ACCUMULATE_RESOURCES };

    TAccumulateResourceDlg() : TVictoryConditionDlg(true, true, true) {}

    virtual void OnOK();
    virtual std::auto_ptr<TVictoryCondition> getVictoryCondition() const;
    void setVictoryCondition(const TVCAccumulateResource& vc);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnKillFocusQuantityEdit();
    DECLARE_MESSAGE_MAP()

private:
    CComboBox _m_resourceCombo;
    CEdit _m_quantityEdit;
    CSpinButtonCtrl _m_quantitySpin;
    TGameResourceType _m_resourceType;
    unsigned int _m_quantity;
};

// Upgrading a town: a combo of the map's towns and the hall and castle
// levels.
class TUpgradeTownDlg : public TVictoryConditionDlg {
public:
    enum { IDD = IDD_UPGRADE_TOWN };

    TUpgradeTownDlg(const TGameMap& gameMap, const std::vector<TMapObjectRef>& townsOnMap)
        : TVictoryConditionDlg(true, false, true), _m_map(gameMap), _m_aTownsOnMap(townsOnMap) {}

    virtual void OnOK();
    virtual std::auto_ptr<TVictoryCondition> getVictoryCondition() const;
    void setVictoryCondition(const TVCUpgradeTown& vc);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    const TGameMap& _m_map;
    const std::vector<TMapObjectRef>& _m_aTownsOnMap;
    CComboBox _m_townCombo;
    TMapObjectRef _m_townRef;
    int _m_hallLevel;
    int _m_castleLevel;
};

// Building the grail structure: in any town, or one of the map's.
class TBuildHolyGrailStructDlg : public TVictoryConditionDlg {
public:
    enum { IDD = IDD_BUILD_GRAIL };

    TBuildHolyGrailStructDlg(const TGameMap& gameMap, const std::vector<TMapObjectRef>& townsOnMap)
        : TVictoryConditionDlg(false, false, false), _m_map(gameMap), _m_aTownsOnMap(townsOnMap) {}

    virtual void OnOK();
    virtual std::auto_ptr<TVictoryCondition> getVictoryCondition() const;
    void setVictoryCondition(const TVCBuildHolyGrailStruct& vc);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    const TGameMap& _m_map;
    const std::vector<TMapObjectRef>& _m_aTownsOnMap;
    CComboBox _m_townCombo;
    TMapObjectRef _m_townRef;
};

// Defeating a hero: a combo of the map's heroes, those in towns included.
class TDefeatHeroDlg : public TVictoryConditionDlg {
public:
    enum { IDD = IDD_DEFEAT_HERO };

    TDefeatHeroDlg(const TGameMap& gameMap, const std::vector<TMapObjectRef>& heroesOnMap)
        : TVictoryConditionDlg(false, false, false), _m_map(gameMap), _m_aHeroesOnMap(heroesOnMap) {}

    virtual void OnOK();
    virtual std::auto_ptr<TVictoryCondition> getVictoryCondition() const;
    void setVictoryCondition(const TVCDefeatHero& vc);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    const TGameMap& _m_map;
    const std::vector<TMapObjectRef>& _m_aHeroesOnMap;
    CComboBox _m_heroCombo;
    TMapObjectRef _m_heroRef;
};

// Capturing a town: a combo of the map's towns.
class TCaptureTownDlg : public TVictoryConditionDlg {
public:
    enum { IDD = IDD_CAPTURE_TOWN };

    TCaptureTownDlg(const TGameMap& gameMap, const std::vector<TMapObjectRef>& townsOnMap)
        : TVictoryConditionDlg(true, true, true), _m_map(gameMap), _m_aTownsOnMap(townsOnMap) {}

    virtual void OnOK();
    virtual std::auto_ptr<TVictoryCondition> getVictoryCondition() const;
    void setVictoryCondition(const TVCCaptureTown& vc);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    const TGameMap& _m_map;
    const std::vector<TMapObjectRef>& _m_aTownsOnMap;
    CComboBox _m_townCombo;
    TMapObjectRef _m_townRef;
};

// Defeating a monster: a combo of the map's monsters.
class TDefeatMonsterDlg : public TVictoryConditionDlg {
public:
    enum { IDD = IDD_DEFEAT_MONSTER };

    TDefeatMonsterDlg(const TGameMap& gameMap, const std::vector<TMapObjectRef>& monstersOnMap)
        : TVictoryConditionDlg(true, false, true), _m_map(gameMap), _m_aMonstersOnMap(monstersOnMap) {}

    virtual void OnOK();
    virtual std::auto_ptr<TVictoryCondition> getVictoryCondition() const;
    void setVictoryCondition(const TVCDefeatMonster& vc);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    const TGameMap& _m_map;
    const std::vector<TMapObjectRef>& _m_aMonstersOnMap;
    CComboBox _m_monsterCombo;
    TMapObjectRef _m_monsterRef;
};

// Flagging all creature generators: the general checks only.
class TFlagAllCreatureGeneratorsDlg : public TVictoryConditionDlg {
public:
    enum { IDD = IDD_FLAG_ALL };

    TFlagAllCreatureGeneratorsDlg() : TVictoryConditionDlg(true, true, true) {}

    virtual std::auto_ptr<TVictoryCondition> getVictoryCondition() const;
};

// Flagging all mines: the general checks only.
class TFlagAllMinesDlg : public TVictoryConditionDlg {
public:
    enum { IDD = IDD_FLAG_ALL };

    TFlagAllMinesDlg() : TVictoryConditionDlg(true, true, true) {}

    virtual std::auto_ptr<TVictoryCondition> getVictoryCondition() const;
};

// Transporting an artifact to a town: combos of the version's artifacts
// and of the map's towns.
class TTransportArtifactDlg : public TVictoryConditionDlg {
public:
    enum { IDD = IDD_TRANSPORT_ARTIFACT };

    TTransportArtifactDlg(const TGameMap& gameMap, const std::vector<TMapObjectRef>& townsOnMap, EGameVersion mapVersion);

    virtual void OnOK();
    virtual std::auto_ptr<TVictoryCondition> getVictoryCondition() const;
    void setVictoryCondition(const TVCTransportArtifact& vc);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    const TGameMap& _m_map;
    const std::vector<TMapObjectRef>& _m_aTownsOnMap;
    EGameVersion _m_mapVersion;
    CComboBox _m_artifactCombo;
    CComboBox _m_townCombo;
    TArtifact _m_artifact;
    TMapObjectRef _m_townRef;
};

// Lists the map's towns ("name at (x, y)"), each item's data its index.
VA(0x00478fac, 0x132)
void fillTownCombo(const TGameMap& gameMap, const std::vector<TMapObjectRef>& townsOnMap, CComboBox& combo)
{
    for (unsigned int i = 0; i < townsOnMap.size(); i++) {
        const TTown* pTown = dynamic_cast<const TTown*>(gameMap.getPObject(townsOnMap[i]));
        TTilePoint loc = gameMap.getObjectLoc(townsOnMap[i]) - pTown->getTriggerLoc();
        CString name;
        name.Format(kObjectAtLocationFmtStr, pTown->getTownTypeTraits().m_pName, loc.x(), loc.y(),
                    townsOnMap[i].getBSecondLayer());
        int index = combo.AddString(name);
        combo.SetItemData(index, i);
    }
}

VA(0x004790de, 0x1c)
void TVictoryConditionGeneralDlg::setVictoryCondition(const TVictoryCondition& vc)
{
    _m_allowNormalVictory = vc.getBAllowNormalVictory();
    _m_appliesToComputer = vc.getBAppliesToComputer();
    UpdateData(FALSE);
}

VA(0x004790fa, 0x37)
void TVictoryConditionGeneralDlg::DoDataExchange(CDataExchange* pDX)
{
    if (_m_bShowAllowNormalVictory)
        DDX_Check(pDX, IDC_ALLOW_NORMAL_VICTORY_CHECK, _m_allowNormalVictory);
    if (_m_bShowAppliesToComputer)
        DDX_Check(pDX, IDC_APPLIES_TO_COMPUTER_CHECK, _m_appliesToComputer);
}

VA(0x00479131, 0xfa)
BOOL TVictoryConditionGeneralDlg::OnInitDialog()
{
    GetDlgItem(IDC_ALLOW_NORMAL_VICTORY_CHECK)->SetWindowText(SMapSpecsVictoryCondPageText::kNormalVictoryCheckStr);
    GetDlgItem(IDC_APPLIES_TO_COMPUTER_CHECK)->SetWindowText(SMapSpecsVictoryCondPageText::kComputerAlsoCheckStr);
    _m_allowNormalVictory = 0;
    _m_appliesToComputer = 0;
    CDialog::OnInitDialog();
    if (!_m_bShowAllowNormalVictory) {
        CWnd* pAllowNormalVictoryCheck = GetDlgItem(IDC_ALLOW_NORMAL_VICTORY_CHECK);
        CRect allowNormalVictoryRect;
        pAllowNormalVictoryCheck->GetWindowRect(&allowNormalVictoryRect);
        ScreenToClient(&allowNormalVictoryRect);
        pAllowNormalVictoryCheck->DestroyWindow();
        CWnd* pAppliesToComputerCheck = GetDlgItem(IDC_APPLIES_TO_COMPUTER_CHECK);
        CRect appliesToComputerRect;
        pAppliesToComputerCheck->GetWindowRect(&appliesToComputerRect);
        ScreenToClient(&appliesToComputerRect);
        appliesToComputerRect.OffsetRect(allowNormalVictoryRect.TopLeft() - appliesToComputerRect.TopLeft());
        pAppliesToComputerCheck->MoveWindow(&appliesToComputerRect);
    }
    if (!_m_bShowAppliesToComputer)
        GetDlgItem(IDC_APPLIES_TO_COMPUTER_CHECK)->DestroyWindow();
    return TRUE;
}

}

VA(0x0047922b, 0x6)
BEGIN_MESSAGE_MAP(TVictoryConditionDlg, CDialog)
    ON_WM_CREATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

VA(0x00479231, 0x9e)
TVictoryConditionDlg::TVictoryConditionDlg(bool bShowAllowNormalVictory, bool bShowAppliesToComputer, bool bGeneral)
    : _m_pGeneralDlg(NULL)
{
    if (bGeneral) {
        _m_pGeneralDlg = new TVictoryConditionGeneralDlg(bShowAllowNormalVictory, bShowAppliesToComputer);
        if (_m_pGeneralDlg == NULL)
            throw TAllocationFailure();
    }
}

VA(0x004792cf, 0x1b)
void TVictoryConditionDlg::OnOK()
{
    if (_m_pGeneralDlg != NULL)
        _m_pGeneralDlg->OnOK();
    CDialog::OnOK();
}

VA_COMPGEN(0x004792ea, 0x1c, SCALAR_DELETING_DTOR, TVictoryConditionDlg)

VA(0x00479306, 0x41)
TVictoryConditionDlg::~TVictoryConditionDlg()
{
    delete _m_pGeneralDlg;
}

VA(0x00479347, 0x2e)
int TVictoryConditionDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1
        || (_m_pGeneralDlg != NULL && !_m_pGeneralDlg->Create(TVictoryConditionGeneralDlg::IDD, this)))
        return -1;
    return 0;
}

VA(0x00479375, 0xd)
void TVictoryConditionDlg::OnDestroy()
{
    if (_m_pGeneralDlg != NULL)
        _m_pGeneralDlg->DestroyWindow();
}

VA(0x00479382, 0x76)
BOOL TVictoryConditionDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    CWnd* pFrame = GetDlgItem(IDC_FRAME);
    if (pFrame != NULL) {
        if (_m_pGeneralDlg != NULL) {
            CRect rect;
            pFrame->GetWindowRect(&rect);
            ScreenToClient(&rect);
            _m_pGeneralDlg->SetWindowPos(pFrame, rect.left, rect.top, rect.Width(), rect.Height(), 0);
            _m_pGeneralDlg->ShowWindow(SW_SHOW);
        }
        pFrame->DestroyWindow();
    }
    return TRUE;
}

VA(0x004793f8, 0x4f)
TAquireArtifactDlg::TAquireArtifactDlg(EGameVersion mapVersion)
    : TVictoryConditionDlg(false, true, true), _m_mapVersion(mapVersion)
{
}

VA(0x00479498, 0x65)
void TAquireArtifactDlg::setVictoryCondition(const TVCAquireArtifact& vc)
{
    TVictoryConditionDlg::setVictoryCondition(vc);
    int index;
    for (index = 0; _m_artifactCombo.GetItemData(index) != vc.getArtifact(); index++)
        ;
    _m_artifactCombo.SetCurSel(index);
}

VA(0x004794fd, 0xa8)
std::auto_ptr<TVictoryCondition> TAquireArtifactDlg::getVictoryCondition() const
{
    std::auto_ptr<TVictoryCondition> pVictoryCondition(
        new TVCAquireArtifact(_m_pGeneralDlg->_m_appliesToComputer != 0, _m_artifact));
    if (!pVictoryCondition.get())
        throw TAllocationFailure();
    return pVictoryCondition;
}

VA(0x004795a5, 0x15)
void TAquireArtifactDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_ARTIFACT_COMBO, _m_artifactCombo);
}

VA(0x004795ba, 0x10a)
BOOL TAquireArtifactDlg::OnInitDialog()
{
    GetDlgItem(IDC_ARTIFACT_STATIC)->SetWindowText(SMapSpecsVictoryCondPageText::kArtifactStaticStr);
    TVictoryConditionDlg::OnInitDialog();
    int index = _m_artifactCombo.AddString(akArtifactTraits[ARTIFACT_HOLY_GRAIL].m_name);
    _m_artifactCombo.SetItemData(index, ARTIFACT_HOLY_GRAIL);
    int numArtifacts = _m_mapVersion >= GAME_VERSION_SOD ? ARTIFACT_COUNT
                       : (_m_mapVersion >= GAME_VERSION_AB ? kNumABArtifacts : kNumRoEArtifacts);
    for (int artifact = 0; artifact < numArtifacts; artifact++) {
        if (!akArtifactTraits[artifact].m_disabled && !(akArtifactTraits[artifact].m_class & ArtifactClassSpecial)) {
            index = _m_artifactCombo.AddString(akArtifactTraits[artifact].m_name);
            _m_artifactCombo.SetItemData(index, artifact);
        }
    }
    for (index = 0; _m_artifactCombo.GetItemData(index) != ARTIFACT_HOLY_GRAIL; index++)
        ;
    _m_artifactCombo.SetCurSel(index);
    return TRUE;
}

VA(0x004796c4, 0x39)
void TAquireArtifactDlg::OnOK()
{
    TVictoryConditionDlg::OnOK();
    _m_artifact = TArtifact(_m_artifactCombo.GetItemData(_m_artifactCombo.GetCurSel()));
}

VA(0x004796fd, 0x6)
BEGIN_MESSAGE_MAP(TAccumulateCreatureDlg, TVictoryConditionDlg)
    ON_EN_KILLFOCUS(IDC_QUANTITY_EDIT, OnKillFocusQuantityEdit)
END_MESSAGE_MAP()

VA(0x00479703, 0x7d)
TAccumulateCreatureDlg::TAccumulateCreatureDlg(EGameVersion mapVersion)
    : TVictoryConditionDlg(true, true, true), _m_mapVersion(mapVersion)
{
}

// The creature lists' order: by town (the neutrals last), then level, then
// id.
VA(0x004797f2, 0x80)
static bool isCreatureBefore(TCreatureType lhs, TCreatureType rhs)
{
    TCreatureTypeTraits lhsTraits = akCreatureTypeTraits[lhs];
    TCreatureTypeTraits rhsTraits = akCreatureTypeTraits[rhs];
    int lhsTown = lhsTraits.townType == eTownNeutral ? kNumTownTypes : lhsTraits.townType;
    int rhsTown = rhsTraits.townType == eTownNeutral ? kNumTownTypes : rhsTraits.townType;
    if (lhsTown != rhsTown)
        return lhsTown < rhsTown;
    if (lhsTraits.level != rhsTraits.level)
        return lhsTraits.level < rhsTraits.level;
    return lhs < rhs;
}

// Inserts a creature in the list's order, its item data the creature.
static void addCreature(CComboBox& combo, TCreatureType creature)
{
    int index;
    for (index = combo.GetCount(); index > 0 && isCreatureBefore(creature, TCreatureType(combo.GetItemData(index - 1)));
         index--)
        ;
    combo.InsertString(index, akCreatureTypeTraits[creature].m_name);
    combo.SetItemData(index, creature);
}

VA(0x00479872, 0xb1)
void TAccumulateCreatureDlg::setVictoryCondition(const TVCAccumulateCreature& vc)
{
    TVictoryConditionDlg::setVictoryCondition(vc);
    int index;
    for (index = 0; _m_creatureCombo.GetItemData(index) != vc.getCreatureType(); index++)
        ;
    _m_creatureCombo.SetCurSel(index);
    CString text;
    text.Format("%d", vc.getQuantity());
    _m_quantityEdit.SetWindowText(text);
}

VA(0x00479923, 0xb9)
std::auto_ptr<TVictoryCondition> TAccumulateCreatureDlg::getVictoryCondition() const
{
    std::auto_ptr<TVictoryCondition> pVictoryCondition(
        new TVCAccumulateCreature(_m_pGeneralDlg->_m_allowNormalVictory != 0,
                                  _m_pGeneralDlg->_m_appliesToComputer != 0, _m_creatureType, _m_quantity));
    if (!pVictoryCondition.get())
        throw TAllocationFailure();
    return pVictoryCondition;
}

VA(0x004799dc, 0x40)
void TAccumulateCreatureDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_CREATURE_COMBO, _m_creatureCombo);
    DDX_Control(pDX, IDC_QUANTITY_EDIT, _m_quantityEdit);
    DDX_Control(pDX, IDC_QUANTITY_SPIN, _m_quantitySpin);
}

VA(0x00479a1c, 0xef)
BOOL TAccumulateCreatureDlg::OnInitDialog()
{
    GetDlgItem(IDC_CREATURE_TYPE_STATIC)->SetWindowText(SMapSpecsVictoryCondPageText::kCreatureTypeStaticStr);
    GetDlgItem(IDC_QUANTITY_STATIC)->SetWindowText(SMapSpecsVictoryCondPageText::kQuantityStaticStr);
    TVictoryConditionDlg::OnInitDialog();
    unsigned int numCreatures = _m_mapVersion >= GAME_VERSION_AB ? kNumCreatures : kNumRoECreatures;
    for (unsigned int creature = 0; creature < numCreatures; creature++) {
        if (akCreatureTypeTraits[creature].level >= 0)
            addCreature(_m_creatureCombo, TCreatureType(creature));
    }
    int index;
    for (index = 0; _m_creatureCombo.GetItemData(index) != 0; index++)
        ;
    _m_creatureCombo.SetCurSel(index);
    _m_quantityEdit.SetWindowText("1");
    _m_quantityEdit.LimitText(TDigits<kMaxCreatureQuantity>::getDigits());
    _m_quantitySpin.SetRange(1, UD_MAXVAL);
    return TRUE;
}

VA(0x00479b0b, 0xba)
void TAccumulateCreatureDlg::OnOK()
{
    TVictoryConditionDlg::OnOK();
    _m_creatureType = TCreatureType(_m_creatureCombo.GetItemData(_m_creatureCombo.GetCurSel()));
    int quantity = 0;
    CString text;
    _m_quantityEdit.GetWindowText(text);
    sscanf(text, "%d", &quantity);
    _m_quantity = clamp<int>(1, quantity, kMaxCreatureQuantity);
}

VA(0x00479bc5, 0xa8)
void TAccumulateCreatureDlg::OnKillFocusQuantityEdit()
{
    int quantity = 0;
    CString text;
    _m_quantityEdit.GetWindowText(text);
    sscanf(text, "%d", &quantity);
    if (quantity < 1 || quantity > kMaxCreatureQuantity) {
        quantity = clamp<int>(1, quantity, kMaxCreatureQuantity);
        text.Format("%d", quantity);
        _m_quantityEdit.SetWindowText(text);
    }
}

VA(0x00479c6d, 0x6)
BEGIN_MESSAGE_MAP(TAccumulateResourceDlg, TVictoryConditionDlg)
    ON_EN_KILLFOCUS(IDC_QUANTITY_EDIT, OnKillFocusQuantityEdit)
END_MESSAGE_MAP()

VA(0x00479c73, 0xa8)
void TAccumulateResourceDlg::setVictoryCondition(const TVCAccumulateResource& vc)
{
    TVictoryConditionDlg::setVictoryCondition(vc);
    int index;
    for (index = 0; _m_resourceCombo.GetItemData(index) != vc.getResourceType(); index++)
        ;
    _m_resourceCombo.SetCurSel(index);
    CString text;
    text.Format("%d", vc.getQuantity());
    _m_quantityEdit.SetWindowText(text);
}

VA(0x00479d1b, 0xb9)
std::auto_ptr<TVictoryCondition> TAccumulateResourceDlg::getVictoryCondition() const
{
    std::auto_ptr<TVictoryCondition> pVictoryCondition(
        new TVCAccumulateResource(_m_pGeneralDlg->_m_allowNormalVictory != 0,
                                  _m_pGeneralDlg->_m_appliesToComputer != 0, _m_resourceType, _m_quantity));
    if (!pVictoryCondition.get())
        throw TAllocationFailure();
    return pVictoryCondition;
}

VA(0x00479dd4, 0x40)
void TAccumulateResourceDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_RESOURCE_COMBO, _m_resourceCombo);
    DDX_Control(pDX, IDC_QUANTITY_EDIT, _m_quantityEdit);
    DDX_Control(pDX, IDC_QUANTITY_SPIN, _m_quantitySpin);
}

VA(0x00479e14, 0xd5)
BOOL TAccumulateResourceDlg::OnInitDialog()
{
    GetDlgItem(IDC_RESOURCE_TYPE_STATIC)->SetWindowText(SMapSpecsVictoryCondPageText::kResourceTypeStaticStr);
    GetDlgItem(IDC_QUANTITY_STATIC)->SetWindowText(SMapSpecsVictoryCondPageText::kQuantityStaticStr);
    TVictoryConditionDlg::OnInitDialog();
    for (unsigned int resourceType = 0; resourceType < kNumGameResourceTypes; resourceType++) {
        int index = _m_resourceCombo.AddString(akGameResourceTypeTraits[resourceType].m_name);
        _m_resourceCombo.SetItemData(index, resourceType);
    }
    int index;
    for (index = 0; _m_resourceCombo.GetItemData(index) != eResourceGold; index++)
        ;
    _m_resourceCombo.SetCurSel(index);
    _m_quantityEdit.SetWindowText("1");
    _m_quantityEdit.LimitText(TDigits<kMaxResourceQuantity>::getDigits());
    _m_quantitySpin.SetRange(1, UD_MAXVAL);
    return TRUE;
}

VA(0x00479ee9, 0xb4)
void TAccumulateResourceDlg::OnOK()
{
    TVictoryConditionDlg::OnOK();
    _m_resourceType = TGameResourceType(_m_resourceCombo.GetItemData(_m_resourceCombo.GetCurSel()));
    int quantity = 0;
    CString text;
    _m_quantityEdit.GetWindowText(text);
    sscanf(text, "%d", &quantity);
    _m_quantity = clamp<int>(1, quantity, kMaxResourceQuantity);
}

VA(0x00479f9d, 0xa8)
void TAccumulateResourceDlg::OnKillFocusQuantityEdit()
{
    int quantity = 0;
    CString text;
    _m_quantityEdit.GetWindowText(text);
    sscanf(text, "%d", &quantity);
    if (quantity < 1 || quantity > kMaxResourceQuantity) {
        quantity = clamp<int>(1, quantity, kMaxResourceQuantity);
        text.Format("%d", quantity);
        _m_quantityEdit.SetWindowText(text);
    }
}

VA(0x0047a045, 0x7c)
void TUpgradeTownDlg::setVictoryCondition(const TVCUpgradeTown& vc)
{
    TVictoryConditionDlg::setVictoryCondition(vc);
    int index;
    for (index = 0; _m_aTownsOnMap[_m_townCombo.GetItemData(index)] != vc.getTownRef(); index++)
        ;
    _m_townCombo.SetCurSel(index);
    _m_hallLevel = vc.getHallLevel();
    _m_castleLevel = vc.getCastleLevel();
    UpdateData(FALSE);
}

VA(0x0047a0c1, 0xcb)
std::auto_ptr<TVictoryCondition> TUpgradeTownDlg::getVictoryCondition() const
{
    std::auto_ptr<TVictoryCondition> pVictoryCondition(
        new TVCUpgradeTown(_m_pGeneralDlg->_m_allowNormalVictory != 0, _m_townRef,
                           TVCUpgradeTown::THallLevel(_m_hallLevel), TVCUpgradeTown::TCastleLevel(_m_castleLevel)));
    if (!pVictoryCondition.get())
        throw TAllocationFailure();
    return pVictoryCondition;
}

VA(0x0047a18c, 0x40)
void TUpgradeTownDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_TOWN_COMBO, _m_townCombo);
    DDX_Radio(pDX, IDC_TOWN_HALL_RADIO, _m_hallLevel);
    DDX_Radio(pDX, IDC_FORT_RADIO, _m_castleLevel);
}

VA(0x0047a1cc, 0x13e)
BOOL TUpgradeTownDlg::OnInitDialog()
{
    GetDlgItem(IDC_TOWN_STATIC)->SetWindowText(SMapSpecsVictoryCondPageText::kTownStaticStr);
    GetDlgItem(IDC_HALL_LEVEL_STATIC)->SetWindowText(SMapSpecsVictoryCondPageText::kHallLevelStaticStr);
    GetDlgItem(IDC_TOWN_HALL_RADIO)->SetWindowText(SMapSpecsVictoryCondPageText::kTownHallRadioStr);
    GetDlgItem(IDC_CITY_HALL_RADIO)->SetWindowText(SMapSpecsVictoryCondPageText::kCityHallRadioStr);
    GetDlgItem(IDC_CAPITOL_RADIO)->SetWindowText(SMapSpecsVictoryCondPageText::kCapitolHallRadioStr);
    GetDlgItem(IDC_CASTLE_LEVEL_STATIC)->SetWindowText(SMapSpecsVictoryCondPageText::kCastleLevelStaticStr);
    GetDlgItem(IDC_FORT_RADIO)->SetWindowText(SMapSpecsVictoryCondPageText::kFortCastleRadioStr);
    GetDlgItem(IDC_CITADEL_RADIO)->SetWindowText(SMapSpecsVictoryCondPageText::kCitadelCastleRadioStr);
    GetDlgItem(IDC_CASTLE_RADIO)->SetWindowText(SMapSpecsVictoryCondPageText::kCastleCastleRadioStr);
    _m_hallLevel = 0;
    _m_castleLevel = 0;
    TVictoryConditionDlg::OnInitDialog();
    if (_m_aTownsOnMap.size() > 0) {
        fillTownCombo(_m_map, _m_aTownsOnMap, _m_townCombo);
        _m_townCombo.SetCurSel(0);
    }
    return TRUE;
}

VA(0x0047a30a, 0x4c)
void TUpgradeTownDlg::OnOK()
{
    TVictoryConditionDlg::OnOK();
    _m_townRef = _m_aTownsOnMap[_m_townCombo.GetItemData(_m_townCombo.GetCurSel())];
}

VA(0x0047a356, 0xa1)
void TBuildHolyGrailStructDlg::setVictoryCondition(const TVCBuildHolyGrailStruct& vc)
{
    TVictoryConditionDlg::setVictoryCondition(vc);
    int index;
    for (index = 0;; index++) {
        if (vc.getTownRef() == TMapObjectRef()) {
            if (_m_townCombo.GetItemData(index) >= _m_aTownsOnMap.size())
                break;
        } else if (_m_aTownsOnMap[_m_townCombo.GetItemData(index)] == vc.getTownRef())
            break;
    }
    _m_townCombo.SetCurSel(index);
}

VA(0x0047a3f7, 0xab)
std::auto_ptr<TVictoryCondition> TBuildHolyGrailStructDlg::getVictoryCondition() const
{
    std::auto_ptr<TVictoryCondition> pVictoryCondition(new TVCBuildHolyGrailStruct(_m_townRef));
    if (!pVictoryCondition.get())
        throw TAllocationFailure();
    return pVictoryCondition;
}

VA(0x0047a4a2, 0xa4)
BOOL TBuildHolyGrailStructDlg::OnInitDialog()
{
    GetDlgItem(IDC_TOWN_STATIC)->SetWindowText(SMapSpecsVictoryCondPageText::kTownStaticStr);
    TVictoryConditionDlg::OnInitDialog();
    if (_m_aTownsOnMap.size() > 0) {
        int index = _m_townCombo.AddString(kAnyTownStr);
        _m_townCombo.SetItemData(index, _m_aTownsOnMap.size());
        fillTownCombo(_m_map, _m_aTownsOnMap, _m_townCombo);
        _m_townCombo.SetCurSel(index);
    }
    return TRUE;
}

VA(0x0047a546, 0x71)
void TBuildHolyGrailStructDlg::OnOK()
{
    TVictoryConditionDlg::OnOK();
    unsigned int itemData = _m_townCombo.GetItemData(_m_townCombo.GetCurSel());
    _m_townRef = itemData < _m_aTownsOnMap.size() ? _m_aTownsOnMap[itemData] : TMapObjectRef();
}

VA(0x0047a5b7, 0x61)
void TDefeatHeroDlg::setVictoryCondition(const TVCDefeatHero& vc)
{
    TVictoryConditionDlg::setVictoryCondition(vc);
    int index;
    for (index = 0; _m_aHeroesOnMap[_m_heroCombo.GetItemData(index)] != vc.getHeroRef(); index++)
        ;
    _m_heroCombo.SetCurSel(index);
}

VA(0x0047a618, 0xac)
std::auto_ptr<TVictoryCondition> TDefeatHeroDlg::getVictoryCondition() const
{
    std::auto_ptr<TVictoryCondition> pVictoryCondition(new TVCDefeatHero(_m_heroRef));
    if (!pVictoryCondition.get())
        throw TAllocationFailure();
    return pVictoryCondition;
}

VA(0x0047a6c4, 0x15)
void TDefeatHeroDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_HERO_COMBO, _m_heroCombo);
}

VA(0x0047a6d9, 0x2f7)
BOOL TDefeatHeroDlg::OnInitDialog()
{
    GetDlgItem(IDC_HERO_STATIC)->SetWindowText(SMapSpecsVictoryCondPageText::kHeroStaticStr);
    TVictoryConditionDlg::OnInitDialog();
    if (_m_aHeroesOnMap.size() > 0) {
        for (unsigned int i = 0; i < _m_aHeroesOnMap.size(); i++) {
            const TGameObject* pObject = _m_map.getPObject(_m_aHeroesOnMap[i]);
            TTilePoint loc = _m_map.getObjectLoc(_m_aHeroesOnMap[i]) - pObject->getTriggerLoc();
            const TBasicHero* pHero = dynamic_cast<const TBasicHero*>(pObject);
            if (pHero == NULL)
                pHero = dynamic_cast<const TTown*>(pObject)->getPVisitingHero();
            CString heroName;
            const TIdentifiedHero* pIdentifiedHero = dynamic_cast<const TIdentifiedHero*>(pHero);
            if (pIdentifiedHero != NULL) {
                const std::string& name = pIdentifiedHero->getBCustomName()
                                              ? pIdentifiedHero->getName()
                                              : _m_map.getHeroPrototype(pIdentifiedHero->getHeroID()).getName();
                heroName.Format(kSpecificHeroAndClassFmtStr, name.c_str(),
                                THero::s_akClassTraits[pIdentifiedHero->getHeroClass()].m_name);
            } else {
                const THeroPlaceholder* pPlaceholder = dynamic_cast<const THeroPlaceholder*>(pHero);
                if (pPlaceholder != NULL) {
                    bool bHasHero = pPlaceholder->getHeroID() != -1;
                    if (bHasHero) {
                        THeroID heroID = pPlaceholder->getHeroID();
                        const THero::TClassTraits& classTraits = THero::s_akClassTraits[THero::s_akTraits[heroID].m_class];
                        heroName.Format(kSpecificHeroAndClassFmtStr, _m_map.getHeroPrototype(heroID).getName().c_str(),
                                        classTraits.m_name);
                    } else
                        heroName = akAdvObjectTypeTraits[HERO_PLACEHOLDER].m_name;
                } else {
                    const THero* pRandomHero = static_cast<const THero*>(pHero);
                    if (pRandomHero->getBCustomName())
                        heroName.Format(kSpecificHeroAndClassFmtStr, pRandomHero->getName().c_str(),
                                        akAdvObjectTypeTraits[RANDOM_HERO].m_name);
                    else
                        heroName = akAdvObjectTypeTraits[RANDOM_HERO].m_name;
                }
            }
            CString name;
            name.Format(kObjectAtLocationFmtStr, (LPCTSTR)heroName, loc.x(), loc.y(),
                        _m_aHeroesOnMap[i].getBSecondLayer());
            int index = _m_heroCombo.AddString(name);
            _m_heroCombo.SetItemData(index, i);
        }
        _m_heroCombo.SetCurSel(0);
    }
    return TRUE;
}

void TDefeatHeroDlg::OnOK()
{
    TVictoryConditionDlg::OnOK();
    _m_heroRef = _m_aHeroesOnMap[_m_heroCombo.GetItemData(_m_heroCombo.GetCurSel())];
}

void TCaptureTownDlg::setVictoryCondition(const TVCCaptureTown& vc)
{
    TVictoryConditionDlg::setVictoryCondition(vc);
    int index;
    for (index = 0; _m_aTownsOnMap[_m_townCombo.GetItemData(index)] != vc.getTownRef(); index++)
        ;
    _m_townCombo.SetCurSel(index);
}

VA(0x0047a9d0, 0xbb)
std::auto_ptr<TVictoryCondition> TCaptureTownDlg::getVictoryCondition() const
{
    std::auto_ptr<TVictoryCondition> pVictoryCondition(
        new TVCCaptureTown(_m_pGeneralDlg->_m_allowNormalVictory != 0, _m_pGeneralDlg->_m_appliesToComputer != 0,
                           _m_townRef));
    if (!pVictoryCondition.get())
        throw TAllocationFailure();
    return pVictoryCondition;
}

VA(0x0047aa8b, 0x15)
void TCaptureTownDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_TOWN_COMBO, _m_townCombo);
}

void TBuildHolyGrailStructDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_TOWN_COMBO, _m_townCombo);
}

VA(0x0047aaa0, 0x60)
BOOL TCaptureTownDlg::OnInitDialog()
{
    GetDlgItem(IDC_TOWN_STATIC)->SetWindowText(SMapSpecsVictoryCondPageText::kTownStaticStr);
    TVictoryConditionDlg::OnInitDialog();
    if (_m_aTownsOnMap.size() > 0) {
        fillTownCombo(_m_map, _m_aTownsOnMap, _m_townCombo);
        _m_townCombo.SetCurSel(0);
    }
    return TRUE;
}

void TCaptureTownDlg::OnOK()
{
    TVictoryConditionDlg::OnOK();
    _m_townRef = _m_aTownsOnMap[_m_townCombo.GetItemData(_m_townCombo.GetCurSel())];
}

void TDefeatMonsterDlg::setVictoryCondition(const TVCDefeatMonster& vc)
{
    TVictoryConditionDlg::setVictoryCondition(vc);
    int index;
    for (index = 0; _m_aMonstersOnMap[_m_monsterCombo.GetItemData(index)] != vc.getMonsterRef(); index++)
        ;
    _m_monsterCombo.SetCurSel(index);
}

VA(0x0047ab00, 0xb5)
std::auto_ptr<TVictoryCondition> TDefeatMonsterDlg::getVictoryCondition() const
{
    std::auto_ptr<TVictoryCondition> pVictoryCondition(
        new TVCDefeatMonster(_m_pGeneralDlg->_m_allowNormalVictory != 0, _m_monsterRef));
    if (!pVictoryCondition.get())
        throw TAllocationFailure();
    return pVictoryCondition;
}

VA(0x0047abb5, 0x15)
void TDefeatMonsterDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_MONSTER_COMBO, _m_monsterCombo);
}

VA(0x0047abca, 0x1d5)
BOOL TDefeatMonsterDlg::OnInitDialog()
{
    GetDlgItem(IDC_MONSTER_STATIC)->SetWindowText(SMapSpecsVictoryCondPageText::kMonsterStaticStr);
    TVictoryConditionDlg::OnInitDialog();
    if (_m_aMonstersOnMap.size() > 0) {
        for (unsigned int i = 0; i < _m_aMonstersOnMap.size(); i++) {
            const TMonster* pMonster = dynamic_cast<const TMonster*>(_m_map.getPObject(_m_aMonstersOnMap[i]));
            TTilePoint loc = _m_map.getObjectLoc(_m_aMonstersOnMap[i]) - pMonster->getTriggerLoc();
            const char* monsterName;
            if (pMonster->getType() == MONSTER)
                monsterName = getArmyName(pMonster->getCreatureType(), pMonster->getQuantity());
            else
                monsterName = pMonster->getQuantity() == 1 ? kRandomMonsterStr : kRandomMonstersStr;
            CString name;
            name.Format(kObjectAtLocationFmtStr, monsterName, loc.x(), loc.y(), _m_aMonstersOnMap[i].getBSecondLayer());
            int index = _m_monsterCombo.AddString(name);
            _m_monsterCombo.SetItemData(index, i);
        }
        _m_monsterCombo.SetCurSel(0);
    }
    return TRUE;
}

void TDefeatMonsterDlg::OnOK()
{
    TVictoryConditionDlg::OnOK();
    _m_monsterRef = _m_aMonstersOnMap[_m_monsterCombo.GetItemData(_m_monsterCombo.GetCurSel())];
}

VA(0x0047ad9f, 0xa3)
std::auto_ptr<TVictoryCondition> TFlagAllCreatureGeneratorsDlg::getVictoryCondition() const
{
    std::auto_ptr<TVictoryCondition> pVictoryCondition(new TVCFlagAllCreatureGenerators(
        _m_pGeneralDlg->_m_allowNormalVictory != 0, _m_pGeneralDlg->_m_appliesToComputer != 0));
    if (!pVictoryCondition.get())
        throw TAllocationFailure();
    return pVictoryCondition;
}

VA(0x0047ae42, 0xa3)
std::auto_ptr<TVictoryCondition> TFlagAllMinesDlg::getVictoryCondition() const
{
    std::auto_ptr<TVictoryCondition> pVictoryCondition(
        new TVCFlagAllMines(_m_pGeneralDlg->_m_allowNormalVictory != 0, _m_pGeneralDlg->_m_appliesToComputer != 0));
    if (!pVictoryCondition.get())
        throw TAllocationFailure();
    return pVictoryCondition;
}

VA(0x0047aee5, 0x7e)
TTransportArtifactDlg::TTransportArtifactDlg(const TGameMap& gameMap, const std::vector<TMapObjectRef>& townsOnMap,
                                             EGameVersion mapVersion)
    : TVictoryConditionDlg(false, true, true), _m_map(gameMap), _m_aTownsOnMap(townsOnMap), _m_mapVersion(mapVersion)
{
}

VA(0x0047afc6, 0xaa)
void TTransportArtifactDlg::setVictoryCondition(const TVCTransportArtifact& vc)
{
    TVictoryConditionDlg::setVictoryCondition(vc);
    int index;
    for (index = 0; _m_artifactCombo.GetItemData(index) != vc.getArtifact(); index++)
        ;
    _m_artifactCombo.SetCurSel(index);
    for (index = 0; _m_aTownsOnMap[_m_townCombo.GetItemData(index)] != vc.getTownRef(); index++)
        ;
    _m_townCombo.SetCurSel(index);
}

VA(0x0047b070, 0xc0)
std::auto_ptr<TVictoryCondition> TTransportArtifactDlg::getVictoryCondition() const
{
    std::auto_ptr<TVictoryCondition> pVictoryCondition(
        new TVCTransportArtifact(_m_pGeneralDlg->_m_appliesToComputer != 0, _m_artifact, _m_townRef));
    if (!pVictoryCondition.get())
        throw TAllocationFailure();
    return pVictoryCondition;
}

VA(0x0047b130, 0x2e)
void TTransportArtifactDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_ARTIFACT_COMBO, _m_artifactCombo);
    DDX_Control(pDX, IDC_TOWN_COMBO, _m_townCombo);
}

VA(0x0047b15e, 0x162)
BOOL TTransportArtifactDlg::OnInitDialog()
{
    GetDlgItem(IDC_ARTIFACT_STATIC)->SetWindowText(SMapSpecsVictoryCondPageText::kArtifactStaticStr);
    GetDlgItem(IDC_DESTINATION_STATIC)->SetWindowText(SMapSpecsVictoryCondPageText::kDestinationStaticStr);
    TVictoryConditionDlg::OnInitDialog();
    if (_m_aTownsOnMap.size() > 0) {
        int index = _m_artifactCombo.AddString(akArtifactTraits[ARTIFACT_HOLY_GRAIL].m_name);
        _m_artifactCombo.SetItemData(index, ARTIFACT_HOLY_GRAIL);
        int numArtifacts = _m_mapVersion >= GAME_VERSION_SOD ? ARTIFACT_COUNT
                           : (_m_mapVersion >= GAME_VERSION_AB ? kNumABArtifacts : kNumRoEArtifacts);
        for (int artifact = 0; artifact < numArtifacts; artifact++) {
            if (!akArtifactTraits[artifact].m_disabled && !(akArtifactTraits[artifact].m_class & ArtifactClassSpecial)) {
                index = _m_artifactCombo.AddString(akArtifactTraits[artifact].m_name);
                _m_artifactCombo.SetItemData(index, artifact);
            }
        }
        for (index = 0; _m_artifactCombo.GetItemData(index) != ARTIFACT_HOLY_GRAIL; index++)
            ;
        _m_artifactCombo.SetCurSel(index);
        fillTownCombo(_m_map, _m_aTownsOnMap, _m_townCombo);
        _m_townCombo.SetCurSel(0);
    }
    return TRUE;
}

VA(0x0047b2c0, 0x71)
void TTransportArtifactDlg::OnOK()
{
    TVictoryConditionDlg::OnOK();
    _m_artifact = TArtifact(_m_artifactCombo.GetItemData(_m_artifactCombo.GetCurSel()));
    _m_townRef = _m_aTownsOnMap[_m_townCombo.GetItemData(_m_townCombo.GetCurSel())];
}

// The page's child dialogs, one per condition type.
struct TMapSpecsVictoryCondPage::_TDialogs {
    _TDialogs(const TGameMap& gameMap, const std::vector<TMapObjectRef>& townsOnMap,
              const std::vector<TMapObjectRef>& heroesOnMap, const std::vector<TMapObjectRef>& monstersOnMap)
        : m_aquireArtifactDlg(gameMap.getVersion()),
          m_accumulateCreatureDlg(gameMap.getVersion()),
          m_upgradeTownDlg(gameMap, townsOnMap),
          m_buildHolyGrailStructDlg(gameMap, townsOnMap),
          m_defeatHeroDlg(gameMap, heroesOnMap),
          m_captureTownDlg(gameMap, townsOnMap),
          m_defeatMonsterDlg(gameMap, monstersOnMap),
          m_transportArtifactDlg(gameMap, townsOnMap, gameMap.getVersion()) {}

    TNullVictoryConditionDlg m_nullDlg;
    TAquireArtifactDlg m_aquireArtifactDlg;
    TAccumulateCreatureDlg m_accumulateCreatureDlg;
    TAccumulateResourceDlg m_accumulateResourceDlg;
    TUpgradeTownDlg m_upgradeTownDlg;
    TBuildHolyGrailStructDlg m_buildHolyGrailStructDlg;
    TDefeatHeroDlg m_defeatHeroDlg;
    TCaptureTownDlg m_captureTownDlg;
    TDefeatMonsterDlg m_defeatMonsterDlg;
    TFlagAllCreatureGeneratorsDlg m_flagAllCreatureGeneratorsDlg;
    TFlagAllMinesDlg m_flagAllMinesDlg;
    TTransportArtifactDlg m_transportArtifactDlg;
};

VA(0x0047b331, 0x1dd)
TMapSpecsVictoryCondPage::TMapSpecsVictoryCondPage(const TGameMap& oldMap, TGameMap& newMap,
                                                   const std::vector<TMapObjectRef>& townsOnMap,
                                                   const std::vector<TMapObjectRef>& heroesOnMap,
                                                   const std::vector<TMapObjectRef>& monstersOnMap)
    : CPropertyPage(TMapSpecsVictoryCondPage::IDD),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_bTownsOnMap(townsOnMap.size() > 0),
      _m_bHeroesOnMap(heroesOnMap.size() > 0),
      _m_bMonstersOnMap(monstersOnMap.size() > 0),
      _m_bModified(false),
      _m_pDialogs(NULL)
{
    _m_victoryConditionType = -1;
    m_psp.pszTitle = m_strCaption = kSpecialVictoryConditionPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
    _m_pDialogs = new _TDialogs(_m_newMap, townsOnMap, heroesOnMap, monstersOnMap);
    if (_m_pDialogs == NULL)
        throw TAllocationFailure();
    _m_apDialog[eVCNone + 1] = &_m_pDialogs->m_nullDlg;
    _m_apDialog[eVCAquireArtifact + 1] = &_m_pDialogs->m_aquireArtifactDlg;
    _m_apDialog[eVCAccumulateCreature + 1] = &_m_pDialogs->m_accumulateCreatureDlg;
    _m_apDialog[eVCAccumulateResource + 1] = &_m_pDialogs->m_accumulateResourceDlg;
    _m_apDialog[eVCUpgradeTown + 1] = &_m_pDialogs->m_upgradeTownDlg;
    _m_apDialog[eVCBuildHolyGrailStruct + 1] = &_m_pDialogs->m_buildHolyGrailStructDlg;
    _m_apDialog[eVCDefeatHero + 1] = &_m_pDialogs->m_defeatHeroDlg;
    _m_apDialog[eVCCaptureTown + 1] = &_m_pDialogs->m_captureTownDlg;
    _m_apDialog[eVCDefeatMonster + 1] = &_m_pDialogs->m_defeatMonsterDlg;
    _m_apDialog[eVCFlagAllCreatureGenerators + 1] = &_m_pDialogs->m_flagAllCreatureGeneratorsDlg;
    _m_apDialog[eVCFlagAllMines + 1] = &_m_pDialogs->m_flagAllMinesDlg;
    _m_apDialog[eVCTransportArtifact + 1] = &_m_pDialogs->m_transportArtifactDlg;
}

VA_COMPGEN(0x0047bacc, 0x1c, SCALAR_DELETING_DTOR, TMapSpecsVictoryCondPage)

VA(0x0047bae8, 0x6c)
TMapSpecsVictoryCondPage::~TMapSpecsVictoryCondPage()
{
    delete _m_pDialogs;
}

VA(0x0047bc8b, 0x42)
void TMapSpecsVictoryCondPage::_setVictoryConditionType(int victoryConditionType)
{
    _m_apDialog[_m_victoryConditionType]->ShowWindow(SW_HIDE);
    _m_victoryConditionType = victoryConditionType;
    UpdateData(FALSE);
    _m_apDialog[_m_victoryConditionType]->ShowWindow(SW_SHOW);
}

VA(0x0047bccd, 0x23)
void TMapSpecsVictoryCondPage::visit(const TVCAquireArtifact& vc)
{
    _setVictoryConditionType(eVCAquireArtifact + 1);
    _m_pDialogs->m_aquireArtifactDlg.setVictoryCondition(vc);
}

VA(0x0047bcf0, 0x26)
void TMapSpecsVictoryCondPage::visit(const TVCAccumulateCreature& vc)
{
    _setVictoryConditionType(eVCAccumulateCreature + 1);
    _m_pDialogs->m_accumulateCreatureDlg.setVictoryCondition(vc);
}

VA(0x0047bd16, 0x26)
void TMapSpecsVictoryCondPage::visit(const TVCAccumulateResource& vc)
{
    _setVictoryConditionType(eVCAccumulateResource + 1);
    _m_pDialogs->m_accumulateResourceDlg.setVictoryCondition(vc);
}

VA(0x0047bd3c, 0x26)
void TMapSpecsVictoryCondPage::visit(const TVCUpgradeTown& vc)
{
    _setVictoryConditionType(eVCUpgradeTown + 1);
    _m_pDialogs->m_upgradeTownDlg.setVictoryCondition(vc);
}

VA(0x0047bd62, 0x26)
void TMapSpecsVictoryCondPage::visit(const TVCBuildHolyGrailStruct& vc)
{
    _setVictoryConditionType(eVCBuildHolyGrailStruct + 1);
    _m_pDialogs->m_buildHolyGrailStructDlg.setVictoryCondition(vc);
}

VA(0x0047bd88, 0x26)
void TMapSpecsVictoryCondPage::visit(const TVCDefeatHero& vc)
{
    _setVictoryConditionType(eVCDefeatHero + 1);
    _m_pDialogs->m_defeatHeroDlg.setVictoryCondition(vc);
}

VA(0x0047bdae, 0x26)
void TMapSpecsVictoryCondPage::visit(const TVCCaptureTown& vc)
{
    _setVictoryConditionType(eVCCaptureTown + 1);
    _m_pDialogs->m_captureTownDlg.setVictoryCondition(vc);
}

VA(0x0047bdd4, 0x26)
void TMapSpecsVictoryCondPage::visit(const TVCDefeatMonster& vc)
{
    _setVictoryConditionType(eVCDefeatMonster + 1);
    _m_pDialogs->m_defeatMonsterDlg.setVictoryCondition(vc);
}

VA(0x0047bdfa, 0x2e)
void TMapSpecsVictoryCondPage::visit(const TVCFlagAllCreatureGenerators& vc)
{
    _setVictoryConditionType(eVCFlagAllCreatureGenerators + 1);
    _m_pDialogs->m_flagAllCreatureGeneratorsDlg.setVictoryCondition(vc);
}

VA(0x0047be28, 0x2e)
void TMapSpecsVictoryCondPage::visit(const TVCFlagAllMines& vc)
{
    _setVictoryConditionType(eVCFlagAllMines + 1);
    _m_pDialogs->m_flagAllMinesDlg.setVictoryCondition(vc);
}

VA(0x0047be56, 0x26)
void TMapSpecsVictoryCondPage::visit(const TVCTransportArtifact& vc)
{
    _setVictoryConditionType(eVCTransportArtifact + 1);
    _m_pDialogs->m_transportArtifactDlg.setVictoryCondition(vc);
}

VA(0x0047be7c, 0x18)
void TMapSpecsVictoryCondPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Radio(pDX, IDC_NONE_RADIO, _m_victoryConditionType);
}

VA(0x0047be94, 0x6)
BEGIN_MESSAGE_MAP(TMapSpecsVictoryCondPage, CPropertyPage)
    ON_BN_CLICKED(IDC_NONE_RADIO, OnVictoryConditionRadio)
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_AQUIRE_ARTIFACT_RADIO, OnVictoryConditionRadio)
    ON_BN_CLICKED(IDC_ACCUMULATE_CREATURES_RADIO, OnVictoryConditionRadio)
    ON_BN_CLICKED(IDC_ACCUMULATE_RESOURCES_RADIO, OnVictoryConditionRadio)
    ON_BN_CLICKED(IDC_UPGRADE_TOWN_RADIO, OnVictoryConditionRadio)
    ON_BN_CLICKED(IDC_BUILD_GRAIL_RADIO, OnVictoryConditionRadio)
    ON_BN_CLICKED(IDC_DEFEAT_HERO_RADIO, OnVictoryConditionRadio)
    ON_BN_CLICKED(IDC_CAPTURE_TOWN_RADIO, OnVictoryConditionRadio)
    ON_BN_CLICKED(IDC_DEFEAT_MONSTER_RADIO, OnVictoryConditionRadio)
    ON_BN_CLICKED(IDC_FLAG_GENERATORS_RADIO, OnVictoryConditionRadio)
    ON_BN_CLICKED(IDC_FLAG_MINES_RADIO, OnVictoryConditionRadio)
    ON_BN_CLICKED(IDC_TRANSPORT_ARTIFACT_RADIO, OnVictoryConditionRadio)
    ON_WM_CREATE()
END_MESSAGE_MAP()

VA(0x0047be9a, 0x184)
int TMapSpecsVictoryCondPage::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CPropertyPage::OnCreate(lpCreateStruct) == -1 || !_m_pDialogs->m_nullDlg.Create(this)
        || !_m_pDialogs->m_aquireArtifactDlg.Create(TAquireArtifactDlg::IDD, this)
        || !_m_pDialogs->m_accumulateCreatureDlg.Create(TAccumulateCreatureDlg::IDD, this)
        || !_m_pDialogs->m_accumulateResourceDlg.Create(TAccumulateResourceDlg::IDD, this)
        || !_m_pDialogs->m_upgradeTownDlg.Create(TUpgradeTownDlg::IDD, this)
        || !_m_pDialogs->m_buildHolyGrailStructDlg.Create(TBuildHolyGrailStructDlg::IDD, this)
        || !_m_pDialogs->m_defeatHeroDlg.Create(TDefeatHeroDlg::IDD, this)
        || !_m_pDialogs->m_captureTownDlg.Create(TCaptureTownDlg::IDD, this)
        || !_m_pDialogs->m_defeatMonsterDlg.Create(TDefeatMonsterDlg::IDD, this)
        || !_m_pDialogs->m_flagAllCreatureGeneratorsDlg.Create(TFlagAllCreatureGeneratorsDlg::IDD, this)
        || !_m_pDialogs->m_flagAllMinesDlg.Create(TFlagAllMinesDlg::IDD, this)
        || !_m_pDialogs->m_transportArtifactDlg.Create(TTransportArtifactDlg::IDD, this))
        return -1;
    return 0;
}

VA(0x0047c01e, 0xf8)
void TMapSpecsVictoryCondPage::OnDestroy()
{
    _m_pDialogs->m_transportArtifactDlg.DestroyWindow();
    _m_pDialogs->m_flagAllMinesDlg.DestroyWindow();
    _m_pDialogs->m_flagAllCreatureGeneratorsDlg.DestroyWindow();
    _m_pDialogs->m_defeatMonsterDlg.DestroyWindow();
    _m_pDialogs->m_captureTownDlg.DestroyWindow();
    _m_pDialogs->m_defeatHeroDlg.DestroyWindow();
    _m_pDialogs->m_buildHolyGrailStructDlg.DestroyWindow();
    _m_pDialogs->m_upgradeTownDlg.DestroyWindow();
    _m_pDialogs->m_accumulateResourceDlg.DestroyWindow();
    _m_pDialogs->m_accumulateCreatureDlg.DestroyWindow();
    _m_pDialogs->m_aquireArtifactDlg.DestroyWindow();
    _m_pDialogs->m_nullDlg.DestroyWindow();
    CPropertyPage::OnDestroy();
}

VA(0x0047c116, 0x29b)
BOOL TMapSpecsVictoryCondPage::OnInitDialog()
{
    GetDlgItem(IDC_SELECT_VICTORY_CONDITION_STATIC)
        ->SetWindowText(SMapSpecsVictoryCondPageText::kSelectVictoryConditionStaticStr);
    GetDlgItem(IDC_NONE_RADIO)->SetWindowText(SMapSpecsVictoryCondPageText::kNoneRadioStr);
    GetDlgItem(IDC_AQUIRE_ARTIFACT_RADIO)->SetWindowText(SMapSpecsVictoryCondPageText::kAquireArtifactRadioStr);
    GetDlgItem(IDC_ACCUMULATE_CREATURES_RADIO)->SetWindowText(SMapSpecsVictoryCondPageText::kAccumCreaturesRadioStr);
    GetDlgItem(IDC_ACCUMULATE_RESOURCES_RADIO)->SetWindowText(SMapSpecsVictoryCondPageText::kAccumResourcesRadioStr);
    GetDlgItem(IDC_UPGRADE_TOWN_RADIO)->SetWindowText(SMapSpecsVictoryCondPageText::kUpgradeTownRadioStr);
    GetDlgItem(IDC_BUILD_GRAIL_RADIO)->SetWindowText(SMapSpecsVictoryCondPageText::kBuildGrailRadioStr);
    GetDlgItem(IDC_DEFEAT_HERO_RADIO)->SetWindowText(SMapSpecsVictoryCondPageText::kDefeatHeroRadioStr);
    GetDlgItem(IDC_CAPTURE_TOWN_RADIO)->SetWindowText(SMapSpecsVictoryCondPageText::kCaptureTownRadioStr);
    GetDlgItem(IDC_DEFEAT_MONSTER_RADIO)->SetWindowText(SMapSpecsVictoryCondPageText::kDefeatMonsterRadioStr);
    GetDlgItem(IDC_FLAG_GENERATORS_RADIO)->SetWindowText(SMapSpecsVictoryCondPageText::kFlagGeneratorsRadioStr);
    GetDlgItem(IDC_FLAG_MINES_RADIO)->SetWindowText(SMapSpecsVictoryCondPageText::kFlagMinesRadioStr);
    GetDlgItem(IDC_TRANSPORT_ARTIFACT_RADIO)->SetWindowText(SMapSpecsVictoryCondPageText::kTransportArtifactRadioStr);
    _m_bModified = false;
    _m_victoryConditionType = eVCNone + 1;
    const TVictoryCondition* pVictoryCondition = _m_newMap.getPVictoryCondition();
    CPropertyPage::OnInitDialog();
    if (!_m_bTownsOnMap) {
        GetDlgItem(IDC_UPGRADE_TOWN_RADIO)->EnableWindow(FALSE);
        GetDlgItem(IDC_BUILD_GRAIL_RADIO)->EnableWindow(FALSE);
        GetDlgItem(IDC_CAPTURE_TOWN_RADIO)->EnableWindow(FALSE);
        GetDlgItem(IDC_TRANSPORT_ARTIFACT_RADIO)->EnableWindow(FALSE);
    }
    if (!_m_bHeroesOnMap)
        GetDlgItem(IDC_DEFEAT_HERO_RADIO)->EnableWindow(FALSE);
    if (!_m_bMonstersOnMap)
        GetDlgItem(IDC_DEFEAT_MONSTER_RADIO)->EnableWindow(FALSE);
    CWnd* pFrame = GetDlgItem(IDC_FRAME);
    CRect rect;
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    const CWnd* pInsertAfter = pFrame;
    for (int type = 0; type < _s_kNumVictoryConditionTypes; type++) {
        _m_apDialog[type]->SetWindowPos(pInsertAfter, rect.left, rect.top, 0, 0, SWP_NOSIZE);
        pInsertAfter = _m_apDialog[type];
    }
    pFrame->DestroyWindow();
    _m_apDialog[eVCNone + 1]->ShowWindow(SW_SHOW);
    if (pVictoryCondition != NULL)
        pVictoryCondition->accept(this);
    return TRUE;
}

VA(0x0047c3b1, 0x10a)
void TMapSpecsVictoryCondPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_apDialog[_m_victoryConditionType]->OnOK();
    std::auto_ptr<TVictoryCondition> pVictoryCondition(
        _m_apDialog[_m_victoryConditionType]->getVictoryCondition().release());
    _m_newMap.setVictoryCondition(pVictoryCondition);
    _m_bModified = _m_bModified
                   || (_m_newMap.getPVictoryCondition() != NULL) != (_m_oldMap.getPVictoryCondition() != NULL)
                   || (_m_newMap.getPVictoryCondition() != NULL
                       && !TVictoryCondition::equivalent(*_m_newMap.getPVictoryCondition(),
                                                         *_m_oldMap.getPVictoryCondition()));
}

VA(0x0047c4bb, 0x3a)
void TMapSpecsVictoryCondPage::OnVictoryConditionRadio()
{
    _m_apDialog[_m_victoryConditionType]->ShowWindow(SW_HIDE);
    UpdateData(TRUE);
    _m_apDialog[_m_victoryConditionType]->ShowWindow(SW_SHOW);
}
