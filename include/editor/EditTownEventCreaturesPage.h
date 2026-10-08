// EditTownEventCreaturesPage.h - the creatures page of the town event sheet
// (Loki EditTownEventCreaturesPage.cpp): a growth bonus for each of the
// town's seven creature generators, over the edit_town_event_label%d and
// edit_town_event_spin%d glade widgets. Layout from the image: the event,
// the town type, the modified flag, the bonuses, each generator's label and
// spin button, then the vtable pointer (OnOK, OnInitDialog). The member
// names are not proven.
#ifndef HOMM3_EDITOR_EDITTOWNEVENTCREATURESPAGE_H
#define HOMM3_EDITOR_EDITTOWNEVENTCREATURESPAGE_H

#include "editor/stdafx.h"
#include "editor/Town.h"

class TEditTownEventCreaturesPage {
public:
    TEditTownEventCreaturesPage(const TTown::TTimedEvent& event, TTownType townType);
    ~TEditTownEventCreaturesPage();

    virtual void OnOK();
    virtual BOOL OnInitDialog();

    const TTown::TGeneratorBonuses& getGeneratorBonuses() const { return _m_generatorBonuses; }
    bool wasModified() const { return _m_bModified; }

private:
    struct _TGeneratorWidgets {
        GtkLabel* m_pLabel;
        GtkSpinButton* m_pSpin;
    };

    const TTown::TTimedEvent& _m_event;
    TTownType _m_townType;
    bool _m_bModified;
    TTown::TGeneratorBonuses _m_generatorBonuses;
    _TGeneratorWidgets _m_aGeneratorWidgets[TTown::s_kNumGeneratorTypes];
};

#endif  /* HOMM3_EDITOR_EDITTOWNEVENTCREATURESPAGE_H */
