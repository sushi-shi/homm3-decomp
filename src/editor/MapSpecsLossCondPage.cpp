// MapSpecsLossCondPage.cpp - the special loss condition page of the map
// specifications sheet (h3maped 0x471b84..0x472d8b; Loki h3maped object
// 71). The page creates a child dialog per condition type in its frame and
// shows the chosen type's: none, losing a town or a hero (each listed as
// "name at (x, y)") or a time limit. Its type radios, data exchange and type
// switch fold with the victory condition page's (0x47c4bb, 0x47be7c,
// 0x47bc8b), as do the town and hero dialogs' selection and OK.
#include "editor/stdafx.h"

#include "adventureobjecttype.h"
#include "objnames.h"
#include "retailobjecttype.h"
#include "va.h"
#include "editor/DialogTemplate.h"
#include "editor/GameObject.h"
#include "editor/Hero.h"
#include "editor/MapEditorText.h"
#include "editor/MapSpecsLossCondPage.h"
#include "editor/Town.h"

// The time limits in days: 2..7 days, 1..7 weeks, 2..12 months.
DATA(0x0053a5b8)
static const unsigned int akTimeLimitEntryInfo[] = {
    2, 3, 4, 5, 6, 7, 14, 21, 28, 35, 42, 49, 56, 84, 112, 140, 168, 196, 224, 252, 280, 308, 336
};

namespace {

enum { kNumTimeLimitEntries = sizeof(akTimeLimitEntryInfo) / sizeof(akTimeLimitEntryInfo[0]) };

inline const char* getTimeLimitEntryName(unsigned int i);

// No loss condition: an empty dialog.
class TNullLossConditionDlg : public TLossConditionDlg {
public:
    BOOL Create(CWnd* pParentWnd)
    {
        TEmptyDialogTemplate dialogTemplate;
        return CreateIndirect(&dialogTemplate, pParentWnd);
    }

    virtual std::auto_ptr<TLossCondition> getLossCondition() const
    {
        return std::auto_ptr<TLossCondition>(NULL);
    }
};

// Losing a town: a combo of the map's towns.
class TLoseTownDlg : public TLossConditionDlg {
public:
    TLoseTownDlg(const TGameMap& map, const std::vector<TMapObjectRef>& townsOnMap)
        : _m_map(map), _m_aTownsOnMap(townsOnMap) {}

    virtual void OnOK();
    virtual std::auto_ptr<TLossCondition> getLossCondition() const;
    void setLossCondition(const TLCLoseTown& lc);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    const TGameMap& _m_map;
    const std::vector<TMapObjectRef>& _m_aTownsOnMap;
    CComboBox _m_townCombo;
    TMapObjectRef _m_townRef;
};

// Losing a hero: a combo of the map's heroes, those in towns included.
class TLoseHeroDlg : public TLossConditionDlg {
public:
    TLoseHeroDlg(const TGameMap& map, const std::vector<TMapObjectRef>& heroesOnMap)
        : _m_map(map), _m_aHeroesOnMap(heroesOnMap) {}

    virtual void OnOK();
    virtual std::auto_ptr<TLossCondition> getLossCondition() const;
    void setLossCondition(const TLCLoseHero& lc);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    const TGameMap& _m_map;
    const std::vector<TMapObjectRef>& _m_aHeroesOnMap;
    CComboBox _m_heroCombo;
    TMapObjectRef _m_heroRef;
};

// A time limit: a combo of the limits.
class TTimeExpiresDlg : public TLossConditionDlg {
public:
    virtual void OnOK();
    virtual std::auto_ptr<TLossCondition> getLossCondition() const;
    void setLossCondition(const TLCTimeExpires& lc);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:
    CComboBox _m_timeLimitCombo;
    unsigned int _m_numDays;
};

}

VA(0x00471b84, 0xa5)
std::auto_ptr<TLossCondition> TLoseTownDlg::getLossCondition() const
{
    std::auto_ptr<TLossCondition> pLossCondition(new TLCLoseTown(_m_townRef));
    if (!pLossCondition.get())
        throw TAllocationFailure();
    return pLossCondition;
}

VA(0x00471c29, 0x15)
void TLoseTownDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_TOWN_COMBO, _m_townCombo);
}

VA(0x00471c3e, 0x190)
BOOL TLoseTownDlg::OnInitDialog()
{
    GetDlgItem(IDC_TOWN_STATIC)->SetWindowText(SMapSpecsLossCondPageText::kTownStaticStr);
    TLossConditionDlg::OnInitDialog();
    if (_m_aTownsOnMap.size() > 0) {
        for (unsigned int i = 0; i < _m_aTownsOnMap.size(); i++) {
            const TTown* pTown = dynamic_cast<const TTown*>(_m_map.getPObject(_m_aTownsOnMap[i]));
            TTilePoint loc = _m_map.getObjectLoc(_m_aTownsOnMap[i]) - pTown->getTriggerLoc();
            CString name;
            name.Format(kObjectAtLocationFmtStr, pTown->getTownTypeTraits().m_pName, loc.x(), loc.y(),
                        _m_aTownsOnMap[i].getBSecondLayer());
            int index = _m_townCombo.AddString(name);
            _m_townCombo.SetItemData(index, i);
        }
        _m_townCombo.SetCurSel(0);
    }
    return TRUE;
}

VA(0x00471dce, 0x54)
void TLoseTownDlg::setLossCondition(const TLCLoseTown& lc)
{
    int index;
    for (index = 0; _m_aTownsOnMap[_m_townCombo.GetItemData(index)] != lc.getTownRef(); index++)
        ;
    _m_townCombo.SetCurSel(index);
}

VA(0x00471e22, 0xa5)
std::auto_ptr<TLossCondition> TLoseHeroDlg::getLossCondition() const
{
    std::auto_ptr<TLossCondition> pLossCondition(new TLCLoseHero(_m_heroRef));
    if (!pLossCondition.get())
        throw TAllocationFailure();
    return pLossCondition;
}

VA(0x00471ec7, 0x15)
void TLoseHeroDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_HERO_COMBO, _m_heroCombo);
}

VA(0x00471edc, 0x2f7)
BOOL TLoseHeroDlg::OnInitDialog()
{
    GetDlgItem(IDC_HERO_STATIC)->SetWindowText(SMapSpecsLossCondPageText::kHeroStaticStr);
    TLossConditionDlg::OnInitDialog();
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

void TLoseHeroDlg::setLossCondition(const TLCLoseHero& lc)
{
    int index;
    for (index = 0; _m_aHeroesOnMap[_m_heroCombo.GetItemData(index)] != lc.getHeroRef(); index++)
        ;
    _m_heroCombo.SetCurSel(index);
}

VA(0x004721d3, 0x4c)
void TLoseHeroDlg::OnOK()
{
    TLossConditionDlg::OnOK();
    _m_heroRef = _m_aHeroesOnMap[_m_heroCombo.GetItemData(_m_heroCombo.GetCurSel())];
}

void TLoseTownDlg::OnOK()
{
    TLossConditionDlg::OnOK();
    _m_townRef = _m_aTownsOnMap[_m_townCombo.GetItemData(_m_townCombo.GetCurSel())];
}

VA(0x0047221f, 0x5a)
void TTimeExpiresDlg::setLossCondition(const TLCTimeExpires& lc)
{
    int index;
    for (index = 0; akTimeLimitEntryInfo[_m_timeLimitCombo.GetItemData(index)] != lc.getNumDays(); index++)
        ;
    _m_timeLimitCombo.SetCurSel(index);
}

VA(0x00472279, 0x96)
std::auto_ptr<TLossCondition> TTimeExpiresDlg::getLossCondition() const
{
    std::auto_ptr<TLossCondition> pLossCondition(new TLCTimeExpires(_m_numDays));
    if (!pLossCondition.get())
        throw TAllocationFailure();
    return pLossCondition;
}

VA(0x0047230f, 0x15)
void TTimeExpiresDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_TIME_LIMIT_COMBO, _m_timeLimitCombo);
}

VA(0x00472324, 0x67)
BOOL TTimeExpiresDlg::OnInitDialog()
{
    GetDlgItem(IDC_TIME_LIMIT_STATIC)->SetWindowText(SMapSpecsLossCondPageText::kTimeLimitStaticStr);
    TLossConditionDlg::OnInitDialog();
    for (unsigned int i = 0; i < kNumTimeLimitEntries; i++) {
        int index = _m_timeLimitCombo.AddString(getTimeLimitEntryName(i));
        _m_timeLimitCombo.SetItemData(index, i);
    }
    _m_timeLimitCombo.SetCurSel(0);
    return TRUE;
}

namespace {

VA(0x0047238b, 0xf5)  // after its first user
inline const char* getTimeLimitEntryName(unsigned int i)
{
    const char* const akName[kNumTimeLimitEntries] = {
        k2DaysStr, k3DaysStr, k4DaysStr, k5DaysStr, k6DaysStr, k1WeekStr, k2WeeksStr, k3WeeksStr,
        k4WeeksStr, k5WeeksStr, k6WeeksStr, k7WeeksStr, k2MonthsStr, k3MonthsStr, k4MonthsStr,
        k5MonthsStr, k6MonthsStr, k7MonthsStr, k8MonthsStr, k9MonthsStr, k10MonthsStr, k11MonthsStr,
        k12MonthsStr
    };
    return akName[i];
}

}

VA(0x00472480, 0x3a)
void TTimeExpiresDlg::OnOK()
{
    TLossConditionDlg::OnOK();
    _m_numDays = akTimeLimitEntryInfo[_m_timeLimitCombo.GetItemData(_m_timeLimitCombo.GetCurSel())];
}

// The page's child dialogs, one per condition type.
struct TMapSpecsLossCondPage::_TDialogs {
    _TDialogs(const TGameMap& map, const std::vector<TMapObjectRef>& townsOnMap,
              const std::vector<TMapObjectRef>& heroesOnMap)
        : m_loseTownDlg(map, townsOnMap), m_loseHeroDlg(map, heroesOnMap) {}

    TNullLossConditionDlg m_nullDlg;
    TLoseTownDlg m_loseTownDlg;
    TLoseHeroDlg m_loseHeroDlg;
    TTimeExpiresDlg m_timeExpiresDlg;
};

VA(0x004724ba, 0x164)
TMapSpecsLossCondPage::TMapSpecsLossCondPage(const TGameMap& oldMap, TGameMap& newMap,
                                             const std::vector<TMapObjectRef>& townsOnMap,
                                             const std::vector<TMapObjectRef>& heroesOnMap)
    : CPropertyPage(TMapSpecsLossCondPage::IDD),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_bTownsOnMap(townsOnMap.size() > 0),
      _m_bHeroesOnMap(heroesOnMap.size() > 0),
      _m_bModified(false),
      _m_pDialogs(NULL)
{
    _m_lossConditionType = -1;
    m_psp.pszTitle = m_strCaption = kSpecialLossConditionPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
    _m_pDialogs = new _TDialogs(_m_newMap, townsOnMap, heroesOnMap);
    if (_m_pDialogs == NULL)
        throw TAllocationFailure();
    _m_apDialog[_s_kNone] = &_m_pDialogs->m_nullDlg;
    _m_apDialog[_s_kLoseTown] = &_m_pDialogs->m_loseTownDlg;
    _m_apDialog[_s_kLoseHero] = &_m_pDialogs->m_loseHeroDlg;
    _m_apDialog[_s_kTimeExpires] = &_m_pDialogs->m_timeExpiresDlg;
}

VA_COMPGEN(0x0047289b, 0x1c, SCALAR_DELETING_DTOR, TMapSpecsLossCondPage)

VA(0x004728b7, 0x7b)
TMapSpecsLossCondPage::~TMapSpecsLossCondPage()
{
    delete _m_pDialogs;
}

VA(0x004729bf, 0x23)
void TMapSpecsLossCondPage::visit(const TLCLoseTown& lc)
{
    _setLossConditionType(_s_kLoseTown);
    _m_pDialogs->m_loseTownDlg.setLossCondition(lc);
}

VA(0x004729e2, 0x26)
void TMapSpecsLossCondPage::visit(const TLCLoseHero& lc)
{
    _setLossConditionType(_s_kLoseHero);
    _m_pDialogs->m_loseHeroDlg.setLossCondition(lc);
}

VA(0x00472a08, 0x26)
void TMapSpecsLossCondPage::visit(const TLCTimeExpires& lc)
{
    _setLossConditionType(_s_kTimeExpires);
    _m_pDialogs->m_timeExpiresDlg.setLossCondition(lc);
}

void TMapSpecsLossCondPage::_setLossConditionType(int lossConditionType)
{
    _m_apDialog[_m_lossConditionType]->ShowWindow(SW_HIDE);
    _m_lossConditionType = lossConditionType;
    UpdateData(FALSE);
    _m_apDialog[_m_lossConditionType]->ShowWindow(SW_SHOW);
}

void TMapSpecsLossCondPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Radio(pDX, IDC_NONE_RADIO, _m_lossConditionType);
}

VA(0x00472a2e, 0x6)
BEGIN_MESSAGE_MAP(TMapSpecsLossCondPage, CPropertyPage)
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_NONE_RADIO, OnLossConditionRadio)
    ON_BN_CLICKED(IDC_LOSE_TOWN_RADIO, OnLossConditionRadio)
    ON_BN_CLICKED(IDC_LOSE_HERO_RADIO, OnLossConditionRadio)
    ON_BN_CLICKED(IDC_TIME_EXPIRES_RADIO, OnLossConditionRadio)
    ON_WM_CREATE()
END_MESSAGE_MAP()

VA(0x00472a34, 0x8f)
int TMapSpecsLossCondPage::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CPropertyPage::OnCreate(lpCreateStruct) == -1 || !_m_pDialogs->m_nullDlg.Create(this)
        || !_m_pDialogs->m_loseTownDlg.Create(IDD_LOSE_TOWN, this)
        || !_m_pDialogs->m_loseHeroDlg.Create(IDD_LOSE_HERO, this)
        || !_m_pDialogs->m_timeExpiresDlg.Create(IDD_TIME_EXPIRES, this))
        return -1;
    return 0;
}

VA(0x00472ac3, 0x50)
void TMapSpecsLossCondPage::OnDestroy()
{
    _m_pDialogs->m_timeExpiresDlg.DestroyWindow();
    _m_pDialogs->m_loseHeroDlg.DestroyWindow();
    _m_pDialogs->m_loseTownDlg.DestroyWindow();
    _m_pDialogs->m_nullDlg.DestroyWindow();
    CPropertyPage::OnDestroy();
}

void TMapSpecsLossCondPage::OnLossConditionRadio()
{
    _m_apDialog[_m_lossConditionType]->ShowWindow(SW_HIDE);
    UpdateData(TRUE);
    _m_apDialog[_m_lossConditionType]->ShowWindow(SW_SHOW);
}

VA(0x00472b13, 0x16e)
BOOL TMapSpecsLossCondPage::OnInitDialog()
{
    GetDlgItem(IDC_SELECT_LOSS_CONDITION_STATIC)->SetWindowText(SMapSpecsLossCondPageText::kSelectLossConditionStaticStr);
    GetDlgItem(IDC_NONE_RADIO)->SetWindowText(SMapSpecsLossCondPageText::kNoneRadioStr);
    GetDlgItem(IDC_LOSE_TOWN_RADIO)->SetWindowText(SMapSpecsLossCondPageText::kLoseTownRadioStr);
    GetDlgItem(IDC_LOSE_HERO_RADIO)->SetWindowText(SMapSpecsLossCondPageText::kLoseHeroRadioStr);
    GetDlgItem(IDC_TIME_EXPIRES_RADIO)->SetWindowText(SMapSpecsLossCondPageText::kTimeExpiresRadioStr);
    _m_bModified = false;
    _m_lossConditionType = _s_kNone;
    const TLossCondition* pLossCondition = _m_newMap.getPLossCondition();
    CPropertyPage::OnInitDialog();
    if (!_m_bTownsOnMap)
        GetDlgItem(IDC_LOSE_TOWN_RADIO)->EnableWindow(FALSE);
    if (!_m_bHeroesOnMap)
        GetDlgItem(IDC_LOSE_HERO_RADIO)->EnableWindow(FALSE);
    CWnd* pFrame = GetDlgItem(IDC_FRAME);
    CRect rect;
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    const CWnd* pInsertAfter = pFrame;
    for (int type = 0; type < _s_kNumLossConditionTypes; type++) {
        _m_apDialog[type]->SetWindowPos(pInsertAfter, rect.left, rect.top, 0, 0, SWP_NOSIZE);
        pInsertAfter = _m_apDialog[type];
    }
    pFrame->DestroyWindow();
    _m_apDialog[_s_kNone]->ShowWindow(SW_SHOW);
    if (pLossCondition != NULL)
        pLossCondition->accept(this);
    return TRUE;
}

VA(0x00472c81, 0x10a)
void TMapSpecsLossCondPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_apDialog[_m_lossConditionType]->OnOK();
    std::auto_ptr<TLossCondition> pLossCondition(_m_apDialog[_m_lossConditionType]->getLossCondition().release());
    _m_newMap.setLossCondition(pLossCondition);
    _m_bModified = _m_bModified
                   || (_m_newMap.getPLossCondition() != NULL) != (_m_oldMap.getPLossCondition() != NULL)
                   || (_m_newMap.getPLossCondition() != NULL
                       && !TLossCondition::equivalent(*_m_newMap.getPLossCondition(), *_m_oldMap.getPLossCondition()));
}
