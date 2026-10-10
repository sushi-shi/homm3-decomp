// EditCrossoverScenarioOptionDlg.h - the dialog that picks one crossover
// choice of a scenario's starting options: the scenario a player's heroes
// come from (among the scenarios the caller allows) and that player (among
// the players the caller allows). Layout from the image: the player combo
// at 0x5c, the scenario combo at 0x98, the campaign at 0xd4, the allowed
// scenarios at 0xd8, the allowed players at 0xe8 and the choice at 0xec.
#ifndef HOMM3_CAMPAIGN_EDITOR_EDITCROSSOVERSCENARIOOPTIONDLG_H
#define HOMM3_CAMPAIGN_EDITOR_EDITCROSSOVERSCENARIOOPTIONDLG_H

#include <vector>

#include "editor/Player.h"
#include "campaign_editor/Campaign.h"
#include "campaign_editor/resource.h"

class TEditCrossoverScenarioOptionDlg : public CDialog {
public:
    // A new choice: the first allowed scenario and player.
    TEditCrossoverScenarioOptionDlg(CWnd* pParent, const TCampaign* pCampaign,
                                    const std::vector<bool>& abScenarioAllowed, const TPlayerMask& players);
    // An existing choice, which the lists offer besides the allowed ones.
    TEditCrossoverScenarioOptionDlg(CWnd* pParent, const TCampaign* pCampaign,
                                    const std::vector<bool>& abScenarioAllowed, const TPlayerMask& players,
                                    const TScenarioOptionsCrossoverScenario::TChoice& choice);

    int getScenario() const { return _m_scenario; }
    int getPlayer() const { return _m_player; }

    enum { IDD = IDD_EDIT_CROSSOVER_SCENARIO_OPTION };
    CComboBox _m_playerCombo;
    CComboBox _m_scenarioCombo;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    DECLARE_MESSAGE_MAP()

private:
    const TCampaign* _m_pCampaign;
    std::vector<bool> _m_abScenarioAllowed;
    TPlayerMask _m_players;
    int _m_scenario;
    int _m_player;
};

#endif  /* HOMM3_CAMPAIGN_EDITOR_EDITCROSSOVERSCENARIOOPTIONDLG_H */
