// ScenarioPropsProloguePage.h - the prologue and epilogue pages of the
// scenario properties sheet (ScenarioPropsProloguePage.cpp): whether the
// scenario has one, its movie (with a preview), its music and its text.
// One dialog template serves both; each page reads and writes its own part
// of the scenario. Layout from the image: the controls from 0x8c, the
// scenario before and during the sheet at 0x26c and 0x270, the campaign
// version at 0x274, the modified and initialized flags at 0x278 and 0x279
// and the preview bitmap at 0x27c.
#ifndef HOMM3_CAMPAIGN_EDITOR_SCENARIOPROPSPROLOGUEPAGE_H
#define HOMM3_CAMPAIGN_EDITOR_SCENARIOPROPSPROLOGUEPAGE_H

#include <memory>

#include "campaign_editor/Campaign.h"
#include "campaign_editor/resource.h"

class TScenarioPropsProloguePageBase : public CPropertyPage {
public:
    TScenarioPropsProloguePageBase(const TScenario& oldScenario, TScenario& newScenario, int campaignVersion,
                                   const CString& caption, UINT nIDHelp);
    virtual ~TScenarioPropsProloguePageBase();

    bool wasModified() const { return _m_bModified; }
    // Follows the scenario's map: no map, nothing to edit.
    void update();

    enum { IDD = IDD_SCENARIO_PROPS_PROLOGUE };
    CStatic _m_messageStatic;
    CStatic _m_musicStatic;
    CStatic _m_movieStatic;
    CEdit _m_messageEdit;
    CListBox _m_musicList;
    CComboBox _m_movieCombo;
    CStatic _m_previewStatic;
    CButton _m_hasPrologueCheck;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnDestroy();
    afx_msg void OnHasPrologueCheck();
    afx_msg void OnSelChangeMovieCombo();
    afx_msg void OnEnable(BOOL bEnable);
    DECLARE_MESSAGE_MAP()

    virtual CString getCheckText() const = 0;
    virtual const TScenarioPrologue* getPrologue(const TScenario& scenario) const = 0;
    virtual void setPrologue(TScenario& scenario, std::auto_ptr<TScenarioPrologue> pPrologue) const = 0;
    virtual void removePrologue(TScenario& scenario) const = 0;

private:
    void _showPrologue(const TScenarioPrologue* pPrologue);
    void _getPrologue(std::auto_ptr<TScenarioPrologue>& pPrologue);

    const TScenario& _m_oldScenario;
    TScenario& _m_newScenario;
    int _m_campaignVersion;
    bool _m_bModified;
    bool _m_bInitialized;
    CBitmap _m_previewBitmap;
};

class TScenarioPropsProloguePage : public TScenarioPropsProloguePageBase {
public:
    TScenarioPropsProloguePage(const TScenario& oldScenario, TScenario& newScenario, int campaignVersion);

protected:
    virtual CString getCheckText() const;
    virtual const TScenarioPrologue* getPrologue(const TScenario& scenario) const;
    virtual void setPrologue(TScenario& scenario, std::auto_ptr<TScenarioPrologue> pPrologue) const;
    virtual void removePrologue(TScenario& scenario) const;
};

class TScenarioPropsEpiloguePage : public TScenarioPropsProloguePageBase {
public:
    TScenarioPropsEpiloguePage(const TScenario& oldScenario, TScenario& newScenario, int campaignVersion);

protected:
    virtual CString getCheckText() const;
    virtual const TScenarioPrologue* getPrologue(const TScenario& scenario) const;
    virtual void setPrologue(TScenario& scenario, std::auto_ptr<TScenarioPrologue> pPrologue) const;
    virtual void removePrologue(TScenario& scenario) const;
};

#endif  /* HOMM3_CAMPAIGN_EDITOR_SCENARIOPROPSPROLOGUEPAGE_H */
