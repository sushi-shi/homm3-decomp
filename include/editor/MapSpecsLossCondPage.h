// MapSpecsLossCondPage.h - the special loss condition page of the map
// specifications sheet (MapSpecsLossCondPage.cpp; Loki h3maped object 71).
// A radio per condition type shows that type's child dialog in the page's
// frame; each child dialog (a TLossConditionDlg) builds its condition. The
// page visits the map's condition to select its type and fill its dialog.
// Layout from the image: the property page, the visitor at 0x8c, the
// chosen type, the map before and during the sheet, whether the map has
// towns and heroes, the modified flag, the child dialogs and a pointer to
// each, then an unused condition (0xbc bytes).
#ifndef HOMM3_EDITOR_MAPSPECSLOSSCONDPAGE_H
#define HOMM3_EDITOR_MAPSPECSLOSSCONDPAGE_H

#include <memory>
#include <vector>

#include "editor/GameMap.h"
#include "editor/MapObjectRef.h"
#include "editor/VictoryCondition.h"
#include "editor/resource.h"

// A child dialog of the loss condition page: the page presses its OK and
// takes the condition it built (RTTI TLossConditionDlg; its OnOK is the
// dialog's, made public, h3maped 0x413ff7).
class TLossConditionDlg : public CDialog {
public:
    virtual void OnOK() { CDialog::OnOK(); }
    virtual std::auto_ptr<TLossCondition> getLossCondition() const = 0;
};

class TMapSpecsLossCondPage : public CPropertyPage, private TLossCondition::TVisitor {
public:
    TMapSpecsLossCondPage(const TGameMap& oldMap, TGameMap& newMap, const std::vector<TMapObjectRef>& townsOnMap,
                          const std::vector<TMapObjectRef>& heroesOnMap);
    virtual ~TMapSpecsLossCondPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_MAP_SPECS_LOSS_COND };
    int _m_lossConditionType;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnDestroy();
    afx_msg void OnLossConditionRadio();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    DECLARE_MESSAGE_MAP()

private:
    enum {
        _s_kNone,
        _s_kLoseTown,
        _s_kLoseHero,
        _s_kTimeExpires,
        _s_kNumLossConditionTypes
    };

    struct _TDialogs;

    virtual void visit(const TLCLoseTown& lc);
    virtual void visit(const TLCLoseHero& lc);
    virtual void visit(const TLCTimeExpires& lc);

    void _setLossConditionType(int lossConditionType);

    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    bool _m_bTownsOnMap;
    bool _m_bHeroesOnMap;
    bool _m_bModified;
    _TDialogs* _m_pDialogs;
    TLossConditionDlg* _m_apDialog[_s_kNumLossConditionTypes];
    std::auto_ptr<TLossCondition> _m_pLossCondition;
};

#endif  /* HOMM3_EDITOR_MAPSPECSLOSSCONDPAGE_H */
