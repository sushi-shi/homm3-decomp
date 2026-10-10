// EditStartingHeroOptionDlg.h - the dialog that picks one starting hero
// choice of a scenario's starting options: the hero (among the heroes the
// caller allows, or a random one when allowed) and the player who starts
// with it (among the players whose map part can start with a hero). Layout
// from the image: the player combo at 0x5c, the hero combo at 0x98, the
// scenario at 0xd4, the allowed heroes at 0xd8, the random hero flag at
// 0xec, the choice at 0xf0 and the players at 0xf8.
#ifndef HOMM3_CAMPAIGN_EDITOR_EDITSTARTINGHEROOPTIONDLG_H
#define HOMM3_CAMPAIGN_EDITOR_EDITSTARTINGHEROOPTIONDLG_H

#include <bitset>

#include "editor/Player.h"
#include "campaign_editor/Campaign.h"
#include "campaign_editor/resource.h"

class TEditStartingHeroOptionDlg : public CDialog {
public:
    typedef std::bitset<TCampaignScenarioMap::kNumHeroes> THeroMask;

    // A new choice: the random hero when allowed, else the first allowed
    // hero, for the first player that can start with one.
    TEditStartingHeroOptionDlg(CWnd* pParent, const TScenario* pScenario, const THeroMask& heroes,
                               bool bRandomHeroAllowed);
    // An existing choice, whose hero the list offers besides the allowed ones.
    TEditStartingHeroOptionDlg(CWnd* pParent, const TScenario* pScenario, const THeroMask& heroes,
                               bool bRandomHeroAllowed, const TScenarioOptionsStartingHero::TChoice& choice);

    int getHero() const { return _m_hero; }
    int getPlayer() const { return _m_player; }

    enum { IDD = IDD_EDIT_STARTING_HERO_OPTION };
    CComboBox _m_playerCombo;
    CComboBox _m_heroCombo;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    DECLARE_MESSAGE_MAP()

private:
    void _initPlayers();

    const TScenario* _m_pScenario;
    THeroMask _m_heroes;
    bool _m_bRandomHeroAllowed;
    int _m_hero;
    int _m_player;
    TPlayerMask _m_players;
};

#endif  /* HOMM3_CAMPAIGN_EDITOR_EDITSTARTINGHEROOPTIONDLG_H */
