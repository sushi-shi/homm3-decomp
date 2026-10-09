// EditTownEventCreaturesPage.h - the creatures page of the town event
// sheet (EditTownEventCreaturesPage.cpp; Loki h3maped object 99): a
// growth bonus edit and spin for each of the town's seven creature
// generators, labelled with the generator's creatures. Layout from the
// image: the event, town type, modified flag and bonuses from 0x8c, then
// each generator's label, edit and spin from 0xb4 (0x5a0 bytes, the
// sheet's new).
#ifndef HOMM3_EDITOR_EDITTOWNEVENTCREATURESPAGE_H
#define HOMM3_EDITOR_EDITTOWNEVENTCREATURESPAGE_H

#include "editor/resource.h"
#include "editor/Town.h"

class TEditTownEventCreaturesPage : public CPropertyPage {
public:
    TEditTownEventCreaturesPage(const TTown::TTimedEvent& event, TTownType townType);
    virtual ~TEditTownEventCreaturesPage();

    const TTown::TGeneratorBonuses& getGeneratorBonuses() const { return _m_generatorBonuses; }
    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_EDIT_TOWN_EVENT_CREATURES };

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    DECLARE_MESSAGE_MAP()

private:
    struct _TGeneratorControls {
        CStatic m_nameStatic;
        CEdit m_bonusEdit;
        CSpinButtonCtrl m_bonusSpin;
    };

    const TTown::TTimedEvent& _m_event;
    TTownType _m_townType;
    bool _m_bModified;
    TTown::TGeneratorBonuses _m_generatorBonuses;
    _TGeneratorControls _m_aGeneratorControls[TTown::s_kNumGeneratorTypes];
};

#endif  /* HOMM3_EDITOR_EDITTOWNEVENTCREATURESPAGE_H */
