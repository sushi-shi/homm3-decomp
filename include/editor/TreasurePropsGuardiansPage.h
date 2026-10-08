// TreasurePropsGuardiansPage.h - the guardians page of the artifact
// property sheet (Loki TreasurePropsGuardiansPage.cpp): a customize toggle
// over a TArmyDlg on the gd_ prefixed glade widgets. Layout from the image:
// the toggle's DDX flag, the treasure, the army dialog, the custom flag, the
// modified flag, the toggle widget, then the vtable pointer (OnOK,
// OnInitDialog). The member names are not proven.
#ifndef HOMM3_EDITOR_TREASUREPROPSGUARDIANSPAGE_H
#define HOMM3_EDITOR_TREASUREPROPSGUARDIANSPAGE_H

#include "editor/stdafx.h"
#include "editor/ObjectSpecializations.h"
#include "editor/Army.h"

class TArmyDlg;

class TTreasurePropsGuardiansPage {
public:
    TTreasurePropsGuardiansPage(const TTreasure& treasure, unsigned int pageNum);
    ~TTreasurePropsGuardiansPage();

    const TArmy& getGuardians() const;
    bool getBCustomGuardians() const { return _m_bCustomGuardians; }
    bool wasModified() const { return _m_bModified; }

    void OnDestroy();
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    void OnCustomizeCheck();

private:
    BOOL _m_bCustomizeCheck;
    const TTreasure& _m_treasure;

public:
    // cppbridge.cpp's glade handlers reach through it.
    TArmyDlg* _m_pArmyDlg;

private:
    bool _m_bCustomGuardians;
    bool _m_bModified;
    GtkToggleButton* _m_customizeCheck;
};

#endif  /* HOMM3_EDITOR_TREASUREPROPSGUARDIANSPAGE_H */
