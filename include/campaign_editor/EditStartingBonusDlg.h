// EditStartingBonusDlg.h - the dialog that edits one bonus of a scenario's
// bonus starting option (EditStartingBonusDlg.cpp). A radio per bonus type
// selects which of the nine type dialogs (one per TScenarioBonusType; the
// classes are the .cpp's own) shows in the frame; OK takes that dialog's
// bonus. The dialog visits the bonus it starts with to select its type.
// Layout from the image: the OK button at 0x64, the type at 0xa0, the bonus
// at 0xa4, the type dialogs at 0xac, the type radios at 0xb4 and the
// inactive flag at 0x294.
#ifndef HOMM3_CAMPAIGN_EDITOR_EDITSTARTINGBONUSDLG_H
#define HOMM3_CAMPAIGN_EDITOR_EDITSTARTINGBONUSDLG_H

#include <memory>

#include "campaign_editor/Campaign.h"
#include "campaign_editor/resource.h"

// One bonus type's controls: a modeless child dialog of the bonus dialog
// that builds a bonus of its type and tells its client whether its choice
// makes one.
class TStartingBonusTypeDlg : public CDialog {
public:
    class TClient {
    public:
        virtual void onBValidChange(TStartingBonusTypeDlg* pDlg, bool bValid) = 0;
    };

    TStartingBonusTypeDlg(TClient* pClient, bool bValid);

    virtual BOOL create(CWnd* pParent) = 0;
    // Whether the scenario's player can take a bonus of the type.
    virtual bool isAvailable() const { return true; }
    virtual std::auto_ptr<TScenarioStartingBonus> getBonus() const = 0;

    bool getBValid() const { return _m_bValid; }

    virtual void OnOK() { CDialog::OnOK(); }

protected:
    virtual void DoDataExchange(CDataExchange* pDX) {}

    void setBValid(bool bValid);

private:
    TClient* _m_pClient;
    bool _m_bValid;
};

class TEditStartingBonusDlg : public CDialog,
                              private TScenarioStartingBonus::TVisitor,
                              private TStartingBonusTypeDlg::TClient {
public:
    // The bonus to edit (a new one when NULL) for the player of the
    // scenario's map, in the campaign's version.
    TEditStartingBonusDlg(CWnd* pParent, const TCampaignScenarioMap* pMap, int player, int campaignVersion,
                          const TScenarioStartingBonus* pBonus);
    virtual ~TEditStartingBonusDlg();

    // Hands the bonus over and starts a new one.
    std::auto_ptr<TScenarioStartingBonus> releaseBonus();

    enum { IDD = IDD_EDIT_STARTING_BONUS };
    CButton _m_okButton;
    int _m_type;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    afx_msg void OnTypeRadio();
    DECLARE_MESSAGE_MAP()

private:
    struct _TTypeDlgs;

    virtual void visit(const TScenarioBonusSpell& bonus);
    virtual void visit(const TScenarioBonusCreature& bonus);
    virtual void visit(const TScenarioBonusBuilding& bonus);
    virtual void visit(const TScenarioBonusArtifact& bonus);
    virtual void visit(const TScenarioBonusSpellScroll& bonus);
    virtual void visit(const TScenarioBonusPrimarySkill& bonus);
    virtual void visit(const TScenarioBonusSecondarySkill& bonus);
    virtual void visit(const TScenarioBonusResource& bonus);

    virtual void onBValidChange(TStartingBonusTypeDlg* pDlg, bool bValid);

    void _setType(int type);

    std::auto_ptr<TScenarioStartingBonus> _m_pBonus;
    std::auto_ptr<_TTypeDlgs> _m_pTypeDlgs;
    CButton _m_aTypeRadios[kNumBonusTypes];
    // Set until OnInitDialog has shown the bonus and again once the dialog
    // is destroyed: the type dialogs' validity then leaves OK alone.
    bool _m_bInactive;
};

#endif  /* HOMM3_CAMPAIGN_EDITOR_EDITSTARTINGBONUSDLG_H */
