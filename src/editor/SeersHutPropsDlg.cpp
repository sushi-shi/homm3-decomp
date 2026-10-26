// SeersHutPropsDlg.cpp - the Seer's Hut's properties dialog (h3maped
// 0x4b732f..0x4b7dcf; GOG only): the quest location dialog with the hut's
// reward, described by a visitor of the reward.
#include "editor/stdafx.h"

#include "artifact.h"
#include "exceptions.h"
#include "va.h"
#include "editor/EditRewardDlg.h"
#include "editor/GameMap.h"
#include "editor/GameResource.h"
#include "editor/Hero.h"
#include "editor/MapEditorText.h"
#include "editor/Monster.h"
#include "editor/SeersHutPropsDlg.h"

enum { kSeersHutTextColumn = 1 };

namespace {

// One line of the description: what the reward gives.
class TGetRewardDescFunc : public TSeersHut::TReward::TVisitor {
public:
    CString operator()(const TSeersHut::TReward* pReward);

    virtual void visit(const TSeersHut::TRewardExperience& reward);
    virtual void visit(const TSeersHut::TRewardMana& reward);
    virtual void visit(const TSeersHut::TRewardMorale& reward);
    virtual void visit(const TSeersHut::TRewardLuck& reward);
    virtual void visit(const TSeersHut::TRewardResource& reward);
    virtual void visit(const TSeersHut::TRewardPrimarySkill& reward);
    virtual void visit(const TSeersHut::TRewardSecondarySkill& reward);
    virtual void visit(const TSeersHut::TRewardArtifact& reward);
    virtual void visit(const TSeersHut::TRewardSpell& reward);
    virtual void visit(const TSeersHut::TRewardCreature& reward);

private:
    CString _m_desc;
};

}

VA(0x004b7567, 0x7c)
CString TGetRewardDescFunc::operator()(const TSeersHut::TReward* pReward)
{
    if (pReward != NULL)
        pReward->accept(this);
    else
        _m_desc = kNoRewardStr;
    CString desc(_m_desc);
    _m_desc = "";
    return desc;
}

VA(0x004b75e3, 0x1d)
void TGetRewardDescFunc::visit(const TSeersHut::TRewardExperience& reward)
{
    _m_desc.Format(kRewardExperienceFmtStr, reward.getBonus());
}

VA(0x004b7600, 0x1d)
void TGetRewardDescFunc::visit(const TSeersHut::TRewardMana& reward)
{
    _m_desc.Format(kRewardManaFmtStr, reward.getBonus());
}

VA(0x004b761d, 0x1d)
void TGetRewardDescFunc::visit(const TSeersHut::TRewardMorale& reward)
{
    _m_desc.Format(kRewardMoraleFmtStr, reward.getBonus());
}

VA(0x004b763a, 0x1d)
void TGetRewardDescFunc::visit(const TSeersHut::TRewardLuck& reward)
{
    _m_desc.Format(kRewardLuckFmtStr, reward.getBonus());
}

VA(0x004b7657, 0x29)
void TGetRewardDescFunc::visit(const TSeersHut::TRewardResource& reward)
{
    _m_desc.Format("%d %s", reward.getQuantity(), akGameResourceTypeTraits[reward.getType()].m_name);
}

VA(0x004b7680, 0x29)
void TGetRewardDescFunc::visit(const TSeersHut::TRewardPrimarySkill& reward)
{
    _m_desc.Format("+%d %s", reward.getBonus(), H3_AT(THero::s_akPrimarySkillTraits, reward.getSkill()).m_name);
}

VA(0x004b76a9, 0x33)
void TGetRewardDescFunc::visit(const TSeersHut::TRewardSecondarySkill& reward)
{
    _m_desc.Format("%s %s", THero::s_akSkillMasteryTraits[reward.getMastery() - eMasteryBasic].m_name,
                   THero::s_akSecondarySkillTraits[reward.getSkill()].m_name);
}

VA(0x004b76dc, 0x1e)
void TGetRewardDescFunc::visit(const TSeersHut::TRewardArtifact& reward)
{
    _m_desc = akArtifactTraits[reward.getArtifact()].m_name;
}

VA(0x004b76fa, 0x2d)
void TGetRewardDescFunc::visit(const TSeersHut::TRewardSpell& reward)
{
    _m_desc.Format(kRewardSpellFmtStr, akSpellTraits[reward.getSpell()].m_name);
}

VA(0x004b7727, 0x55)
void TGetRewardDescFunc::visit(const TSeersHut::TRewardCreature& reward)
{
    const TCreatureStack& creatureStack = reward.getCreatureStack();
    TCreatureType type = creatureStack.getCreatureType();
    unsigned int quantity = creatureStack.getQuantity();
    _m_desc.Format("%d %s", quantity, getArmyName(type, quantity));
}

VA(0x004b777c, 0x186)
TSeersHutPropsDlg::TSeersHutPropsDlg(CWnd* pParent, TGameMap* pMap, bool bSecondLayer, unsigned int objID)
    : TQuestLocationPropsDlg(TSeersHutPropsDlg::IDD, pParent, pMap, bSecondLayer, objID, kSeersHutTextColumn),
      _m_pMap(pMap),
      _m_bSecondLayer(bSecondLayer),
      _m_objID(objID),
      _m_pOldHut(_getOldHut()),
      _m_pNewHut(NULL),
      _m_bModified(false)
{
    _m_pNewMap = std::auto_ptr<TGameMap>(new TGameMap(*_m_pMap));
    if (!_m_pNewMap.get())
        throw TAllocationFailure();
    _setNewMap(_m_pNewMap.get());
    _m_pNewHut = dynamic_cast<TSeersHut*>(_m_pNewMap->getLayer(_m_bSecondLayer).getPObject(_m_objID));
}

VA_COMPGEN(0x004b7902, 0x1c, SCALAR_DELETING_DTOR, TSeersHutPropsDlg)

VA(0x004b791e, 0x77)
TSeersHutPropsDlg::~TSeersHutPropsDlg()
{
}

VA(0x004b7995, 0x42)
const TSeersHut* TSeersHutPropsDlg::_getOldHut() const
{
    const TGameMap* pMap = _m_pMap;
    return dynamic_cast<const TSeersHut*>(pMap->getLayer(_m_bSecondLayer).getPObject(_m_objID));
}

VA(0x004b79d7, 0xcf)
void TSeersHutPropsDlg::_onQuestChanged(const TQuest* pNewQuest)
{
    TQuestLocationPropsDlg::_onQuestChanged(pNewQuest);
    if (pNewQuest == NULL) {
        if (_m_pReward.get() != NULL) {
            _m_pReward = std::auto_ptr<TSeersHut::TReward>();
            TGetRewardDescFunc getRewardDesc;
            _m_rewardStatic.SetWindowText(getRewardDesc(NULL));
        }
        _m_rewardStatic.EnableWindow(FALSE);
        _m_editRewardButton.EnableWindow(FALSE);
    } else {
        _m_rewardStatic.EnableWindow(TRUE);
        _m_editRewardButton.EnableWindow(TRUE);
    }
}

VA_COMPGEN(0x004b7aa6, 0x8, IMPLICIT_DTOR, TGetRewardDescFunc)

VA(0x004b7aae, 0x37)
void TSeersHutPropsDlg::DoDataExchange(CDataExchange* pDX)
{
    TQuestLocationPropsDlg::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_QUEST_REWARD_STATIC, _m_rewardStatic);
    DDX_Control(pDX, IDC_EDIT_REWARD_BUTTON, _m_editRewardButton);
}

VA(0x004b7ae5, 0x6)
BEGIN_MESSAGE_MAP(TSeersHutPropsDlg, TQuestLocationPropsDlg)
    ON_BN_CLICKED(IDC_EDIT_REWARD_BUTTON, OnEditRewardButton)
END_MESSAGE_MAP()

VA(0x004b7aeb, 0x2f)
int TSeersHutPropsDlg::DoModal()
{
    int result = CDialog::DoModal();
    if (result == IDOK && _m_bModified)
        *_m_pMap = *_m_pNewMap;
    return result;
}

VA(0x004b7b1a, 0x130)
BOOL TSeersHutPropsDlg::OnInitDialog()
{
    GetDlgItem(IDC_QUEST_REWARD_GROUP)->SetWindowText(SSeersHutPropsDlgText::kRewardStaticStr);
    GetDlgItem(IDC_EDIT_REWARD_BUTTON)->SetWindowText(SSeersHutPropsDlgText::kEditRewardButtonStr);
    _m_bModified = false;
    if (_m_pNewHut->getPReward() != NULL) {
        _m_pReward = _m_pNewHut->getPReward()->clone();
        if (!_m_pReward.get())
            throw TAllocationFailure();
    }
    TQuestLocationPropsDlg::OnInitDialog();
    TGetRewardDescFunc getRewardDesc;
    _m_rewardStatic.SetWindowText(getRewardDesc(_m_pReward.get()));
    if (_m_pNewHut->getPQuest() == NULL) {
        _m_rewardStatic.EnableWindow(FALSE);
        _m_editRewardButton.EnableWindow(FALSE);
    }
    return TRUE;
}

VA(0x004b7c4a, 0xa9)
void TSeersHutPropsDlg::OnOK()
{
    TQuestLocationPropsDlg::OnOK();
    if (_m_pNewHut->getPQuest() != NULL) {
        if (_m_pReward.get() != NULL)
            _m_pNewHut->setReward(_m_pReward);
        else if (_m_pNewHut->getPReward() != NULL)
            _m_pNewHut->clearReward();
    }
    _m_bModified = _m_bModified || TQuestLocationPropsDlg::wasModified()
                   || (_m_pNewHut->getPReward() != NULL) != (_m_pOldHut->getPReward() != NULL)
                   || (_m_pNewHut->getPReward() != NULL
                       && !TSeersHut::TReward::equivalent(*_m_pNewHut->getPReward(), *_m_pOldHut->getPReward()));
}

VA(0x004b7cf3, 0xdc)
void TSeersHutPropsDlg::OnEditRewardButton()
{
    TEditRewardDlg dlg(this, _m_pReward.get(), _m_pNewMap->getVersion());
    if (dlg.DoModal() == IDOK) {
        _m_pReward = dlg.getReward();
        TGetRewardDescFunc getRewardDesc;
        _m_rewardStatic.SetWindowText(getRewardDesc(_m_pReward.get()));
    }
}
