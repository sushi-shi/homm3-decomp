// MapSpecsRumorsPage.h - the map specifications' rumors page (Loki
// MapSpecsRumorsPage.cpp): a glade list of rumor names with add, edit,
// remove and remove-all buttons over TEditRumorDlg. Not a CWnd: the map,
// the edited rumors (_m_rumors, the asserts' name), the rumors OnOK
// publishes, the modified flag and a flag OnEditRumorButton sets, then the
// vtable pointer (OnOK, OnInitDialog). The other member names are not
// proven.
#ifndef HOMM3_EDITOR_MAPSPECSRUMORSPAGE_H
#define HOMM3_EDITOR_MAPSPECSRUMORSPAGE_H

#include "editor/stdafx.h"

#include <vector>

#include "editor/GameMap.h"

class TMapSpecsRumorsPage {
public:
    TMapSpecsRumorsPage(const TGameMap& map);
    ~TMapSpecsRumorsPage();

    virtual void OnOK();
    virtual BOOL OnInitDialog();

    void OnAddRumorButton();
    void OnEditRumorButton();
    void OnRemoveRumorButton();
    void OnRemoveAllRumorButton();
    void OnSelChangeRumorListbox();
    void OnSelCancelRumorListbox();
    void OnDblclkRumorListbox();

    const vector<TRumor>& getRumors() const { return _m_newRumors; }
    bool wasModified() const { return _m_bModified; }

private:
    const TGameMap& _m_map;
    vector<TRumor> _m_rumors;
    vector<TRumor> _m_newRumors;
    bool _m_bModified;
    bool _m_bEdited;
};

#endif  /* HOMM3_EDITOR_MAPSPECSRUMORSPAGE_H */
