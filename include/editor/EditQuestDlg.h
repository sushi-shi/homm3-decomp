// EditQuestDlg.h - the quest editor a quest location's properties dialog
// opens (EditQuestDlg.cpp; GOG only, Loki's port has no counterpart): one
// radio per quest kind, none first, and the kind's page over the dialog's
// frame. The pages are TQuestTypeDlg's, created modeless from their own
// templates; each tells the dialog when its quest becomes valid or
// invalid, and OK follows the shown page. The dialog visits the quest it
// opens with to fill and show that kind's page.
//
// Layout from the image: the visitor and client bases at 0x5c and 0x60,
// the OK button at 0x64, the checked radio at 0xa0, the quest at 0xa4,
// the map at 0xac, the pages at 0xb0, the radios at 0xb8 and the
// destroyed flag at 0x310.
#ifndef HOMM3_EDITOR_EDITQUESTDLG_H
#define HOMM3_EDITOR_EDITQUESTDLG_H

#include <memory>

#include "editor/Quest.h"
#include "editor/resource.h"

class TGameMap;

// A page of the quest editor: it builds the quest of its kind and reports
// to its client when the quest it would build becomes valid or invalid.
class TQuestTypeDlg : public CDialog {
public:
    class TClient {
    public:
        virtual void onValidityChanged(TQuestTypeDlg* pDlg, bool bValid) = 0;
    };

    TQuestTypeDlg(TClient* pClient, bool bValid) : _m_pClient(pClient), _m_bValid(bValid) {}

    virtual void OnOK() { CDialog::OnOK(); }
    // Creates the page as a child of pParent.
    virtual BOOL create(CWnd* pParent) = 0;
    // Whether the map's version has quests of this kind.
    virtual bool isAvailable() const { return true; }
    virtual std::auto_ptr<TQuest> getQuest() const = 0;

    bool isValid() const { return _m_bValid; }

protected:
    void _setValid(bool bValid);

private:
    TClient* _m_pClient;
    bool _m_bValid;
};

// RTTI: the interface bases are private (attributes 9).
class TEditQuestDlg : public CDialog, private TQuest::TVisitor, private TQuestTypeDlg::TClient {
public:
    enum { s_kNumQuestTypes = 10 };

    TEditQuestDlg(CWnd* pParent, const TQuest* pQuest, const TGameMap* pMap);
    virtual ~TEditQuestDlg();

    // The quest OK left, none for the first radio; the dialog lets go of it.
    std::auto_ptr<TQuest> getQuest();

    enum { IDD = IDD_EDIT_QUEST };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void visit(const TQuestAchieveExperienceLevel& quest);
    virtual void visit(const TQuestAchievePrimarySkillLevel& quest);
    virtual void visit(const TQuestDefeatHero& quest);
    virtual void visit(const TQuestDefeatMonster& quest);
    virtual void visit(const TQuestBringArtifacts& quest);
    virtual void visit(const TQuestBringCreatures& quest);
    virtual void visit(const TQuestBringResources& quest);
    virtual void visit(const TQuestBeASpecificHero& quest);
    virtual void visit(const TQuestBelongToASpecificPlayer& quest);
    virtual void onValidityChanged(TQuestTypeDlg* pDlg, bool bValid);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    afx_msg void OnQuestTypeRadio();
    DECLARE_MESSAGE_MAP()

private:
    struct _TPages;

    void _showPage(int questType);

    CButton _m_okButton;
    int _m_questType;
    std::auto_ptr<TQuest> _m_pQuest;
    const TGameMap* _m_pMap;
    std::auto_ptr<_TPages> _m_pPages;
    CButton _m_aQuestTypeRadio[s_kNumQuestTypes];
    bool _m_bDestroyed;
};

#endif  /* HOMM3_EDITOR_EDITQUESTDLG_H */
