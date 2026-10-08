// TownPropsGarrisonPage.h - the garrison page of the town property sheet
// (Loki TownPropsGarrisonPage.cpp): a customize toggle over a TArmyDlg on
// the tp_ prefixed glade widgets, and the garrison's formation. Layout
// from the image: the toggle widget, the formation radio index (MFC's
// DDX_Radio int), the town, the modified and custom flags, the army
// dialog, the grouped-formation flag, then the vtable pointer (OnOK,
// OnInitDialog). The member names are not proven.
#ifndef HOMM3_EDITOR_TOWNPROPSGARRISONPAGE_H
#define HOMM3_EDITOR_TOWNPROPSGARRISONPAGE_H

#include "editor/stdafx.h"
#include "editor/Army.h"
#include "editor/Town.h"

class TArmyDlg;

class TTownPropsGarrisonPage {
public:
    TTownPropsGarrisonPage(const TTown& town);
    ~TTownPropsGarrisonPage();

    const TArmy& getGarrison() const;
    bool getBCustomGarrison() const { return _m_bCustomGarrison; }
    bool getBGroupedFormation() const { return _m_bGroupedFormation; }
    bool wasModified() const { return _m_bModified; }

    virtual void OnOK();
    virtual BOOL OnInitDialog();
    void OnCustomizeCheck();

private:
    GtkToggleButton* _m_customizeCheck;
    int _m_formation;
    const TTown& _m_town;
    bool _m_bModified;
    bool _m_bCustomGarrison;

public:
    // cppbridge.cpp's glade handlers reach through it.
    TArmyDlg* _m_pArmyDlg;

private:
    bool _m_bGroupedFormation;
};

#endif  /* HOMM3_EDITOR_TOWNPROPSGARRISONPAGE_H */
