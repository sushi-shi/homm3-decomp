// EditRewardDlg.h - the reward editor a Seer's Hut's properties dialog
// opens (EditRewardDlg.cpp; GOG only, Loki's port has no counterpart): one
// radio per reward kind, none first, and the kind's page over the dialog's
// frame. The pages are created modeless from their own templates; OK
// follows the shown page. The dialog visits the reward it opens with to
// fill and show that kind's page.
//
// Layout from the image: the visitor base at 0x5c, the checked radio at
// 0x60, the reward at 0x64 and the pages at 0x6c.
#ifndef HOMM3_EDITOR_EDITREWARDDLG_H
#define HOMM3_EDITOR_EDITREWARDDLG_H

#include <memory>

#include "gameversion.h"
#include "editor/SeersHut.h"
#include "editor/resource.h"

// RTTI: the interface base is private (attributes 9).
class TEditRewardDlg : public CDialog, private TSeersHut::TReward::TVisitor {
public:
    enum { s_kNumRewardTypes = 11 };

    TEditRewardDlg(CWnd* pParent, const TSeersHut::TReward* pReward, EGameVersion mapVersion);
    virtual ~TEditRewardDlg();

    // The reward OK left, none for the first radio; the dialog lets go of
    // it.
    std::auto_ptr<TSeersHut::TReward> getReward();

    enum { IDD = IDD_EDIT_REWARD };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
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
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    afx_msg void OnRewardTypeRadio();
    DECLARE_MESSAGE_MAP()

private:
    struct _TPages;

    void _showPage(int rewardType);

    int _m_rewardType;
    std::auto_ptr<TSeersHut::TReward> _m_pReward;
    std::auto_ptr<_TPages> _m_pPages;
};

#endif  /* HOMM3_EDITOR_EDITREWARDDLG_H */
