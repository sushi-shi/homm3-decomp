// MapSpecsVictoryCondPage.h - the special victory condition page of the
// map specifications sheet (MapSpecsVictoryCondPage.cpp; Loki names the
// page in its sheet but ships no source). As on the loss condition page, a
// radio per condition type shows that type's child dialog in the page's
// frame and the page visits the map's condition to select it.
// Layout from the image: the property page, the visitor at 0x8c, the
// chosen type, the map before and during the sheet, whether the map has
// towns, heroes and monsters, the modified flag, the child dialogs and a
// pointer to each (0xd4 bytes).
#ifndef HOMM3_EDITOR_MAPSPECSVICTORYCONDPAGE_H
#define HOMM3_EDITOR_MAPSPECSVICTORYCONDPAGE_H

#include <memory>
#include <vector>

#include "editor/GameMap.h"
#include "editor/MapObjectRef.h"
#include "editor/VictoryCondition.h"
#include "editor/resource.h"

// A child dialog of the page (RTTI TVictoryConditionDlg; the file
// defines it).
class TVictoryConditionDlg;

class TMapSpecsVictoryCondPage : public CPropertyPage, private TVictoryCondition::TVisitor {
public:
    TMapSpecsVictoryCondPage(const TGameMap& oldMap, TGameMap& newMap, const std::vector<TMapObjectRef>& townsOnMap,
                             const std::vector<TMapObjectRef>& heroesOnMap,
                             const std::vector<TMapObjectRef>& monstersOnMap);
    virtual ~TMapSpecsVictoryCondPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_MAP_SPECS_VICTORY_COND };
    int _m_victoryConditionType;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnVictoryConditionRadio();
    afx_msg void OnDestroy();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    DECLARE_MESSAGE_MAP()

private:
    // The dialogs in the radios' order: none, then the condition kinds.
    enum {
        _s_kNumVictoryConditionTypes = kNumVictoryConditionTypes + 1
    };

    struct _TDialogs;

    virtual void visit(const TVCAquireArtifact& vc);
    virtual void visit(const TVCAccumulateCreature& vc);
    virtual void visit(const TVCAccumulateResource& vc);
    virtual void visit(const TVCUpgradeTown& vc);
    virtual void visit(const TVCBuildHolyGrailStruct& vc);
    virtual void visit(const TVCDefeatHero& vc);
    virtual void visit(const TVCCaptureTown& vc);
    virtual void visit(const TVCDefeatMonster& vc);
    virtual void visit(const TVCFlagAllCreatureGenerators& vc);
    virtual void visit(const TVCFlagAllMines& vc);
    virtual void visit(const TVCTransportArtifact& vc);

    void _setVictoryConditionType(int victoryConditionType);

    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    bool _m_bTownsOnMap;
    bool _m_bHeroesOnMap;
    bool _m_bMonstersOnMap;
    bool _m_bModified;
    _TDialogs* _m_pDialogs;
    TVictoryConditionDlg* _m_apDialog[_s_kNumVictoryConditionTypes];
};

#endif  /* HOMM3_EDITOR_MAPSPECSVICTORYCONDPAGE_H */
