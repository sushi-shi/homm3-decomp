// ScenarioPropsCrossoverPage.h - the crossover page of the scenario
// properties sheet (ScenarioPropsCrossoverPage.cpp): what the heroes keep
// from the previous scenario, and check lists of the creatures and
// artifacts they keep. Layout from the image: the five retain checks from
// 0x8c, the scenario before and during the sheet at 0x1b8 and 0x1bc, the
// campaign version at 0x1c0, the modified and initialized flags at 0x1c4
// and 0x1c5, and the check lists at 0x1c8 and 0x20c.
#ifndef HOMM3_CAMPAIGN_EDITOR_SCENARIOPROPSCROSSOVERPAGE_H
#define HOMM3_CAMPAIGN_EDITOR_SCENARIOPROPSCROSSOVERPAGE_H

#include "campaign_editor/Campaign.h"
#include "campaign_editor/resource.h"

class TScenarioPropsCrossoverPage : public CPropertyPage {
public:
    TScenarioPropsCrossoverPage(const TScenario& oldScenario, TScenario& newScenario, int campaignVersion);
    virtual ~TScenarioPropsCrossoverPage();

    bool wasModified() const { return _m_bModified; }
    // Shows the scenario's crossover again (its map may have changed).
    void update();

    enum { IDD = IDD_SCENARIO_PROPS_CROSSOVER };
    CButton _m_artifactsCheck;
    CButton _m_spellsCheck;
    CButton _m_secondarySkillsCheck;
    CButton _m_primarySkillsCheck;
    CButton _m_experienceCheck;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnAllButton();
    afx_msg void OnNoneButton();
    afx_msg void OnDestroy();
    afx_msg void OnEnable(BOOL bEnable);
    DECLARE_MESSAGE_MAP()

private:
    void _setCrossover(const TScenarioCrossover& crossover);
    void _getCrossover(TScenarioCrossover& crossover);

    const TScenario& _m_oldScenario;
    TScenario& _m_newScenario;
    int _m_campaignVersion;
    bool _m_bModified;
    bool _m_bInitialized;
    CCheckListBox _m_creatureList;
    CCheckListBox _m_artifactList;
};

#endif  /* HOMM3_CAMPAIGN_EDITOR_SCENARIOPROPSCROSSOVERPAGE_H */
