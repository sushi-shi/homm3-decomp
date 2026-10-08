// MapSpecsLossCondPage.h - the map specifications' loss condition page
// (Loki MapSpecsLossCondPage.cpp): none, losing a town, losing a hero or a
// time limit, each choice filling the loss_cond_combo glade combo. The page
// is privately a TLossCondition::TVisitor (__rtti_class marks the base
// private; its vtable pointer comes first, then the
// destructor and the three visits, then OnOK and OnInitDialog). Layout from
// the image: the chosen condition type, the map, the towns and heroes on
// the map (_m_aTownsOnMap, _m_aHeroesOnMap) each with an is-not-empty flag,
// the modified flag, the edited condition, the chosen town and hero, the
// time limit in days and the combo item data. The names the asserts do not
// give, and the 1000-entry size of the item data, are not proven.
#ifndef HOMM3_EDITOR_MAPSPECSLOSSCONDPAGE_H
#define HOMM3_EDITOR_MAPSPECSLOSSCONDPAGE_H

#include "editor/stdafx.h"

#include <memory>
#include <vector>

#include "editor/VictoryCondition.h"
#include "editor/MapObjectRef.h"

class TGameMap;

class TMapSpecsLossCondPage : private TLossCondition::TVisitor {
public:
    TMapSpecsLossCondPage(const TGameMap& map, const vector<TMapObjectRef>& townsOnMap,
                          const vector<TMapObjectRef>& heroesOnMap);
    virtual ~TMapSpecsLossCondPage();

    virtual void OnOK();
    virtual BOOL OnInitDialog();

    void OnLoseTownToggled();
    void OnLoseNullToggled();
    void OnLoseHeroToggled();
    void OnLoseTimeToggled();

    const TLossCondition* getPLossCondition() const { return _m_pLossCondition.get(); }
    bool wasModified() const { return _m_bModified; }

    virtual void visit(const TLCLoseTown& lc) {}
    virtual void visit(const TLCLoseHero& lc) {}
    virtual void visit(const TLCTimeExpires& lc) {}

private:
    void LoseNullInit();
    auto_ptr<TLossCondition> LoseTownCondition() const;
    auto_ptr<TLossCondition> LoseNullCondition() const;
    BOOL LoseTownInit();
    void LoseTownOnOK();
    auto_ptr<TLossCondition> LoseHeroCondition() const;
    BOOL LoseHeroInit();
    void LoseHeroOnOK();
    auto_ptr<TLossCondition> LoseTimeCondition() const;
    BOOL LoseTimeInit();
    void LoseTimeOnOK();

    int _m_lossConditionType;
    const TGameMap& _m_map;
    const vector<TMapObjectRef>& _m_aTownsOnMap;
    bool _m_bTownsOnMap;
    const vector<TMapObjectRef>& _m_aHeroesOnMap;
    bool _m_bHeroesOnMap;
    bool _m_bModified;
    auto_ptr<TLossCondition> _m_pLossCondition;
    TMapObjectRef _m_townRef;
    TMapObjectRef _m_heroRef;
    unsigned int _m_numDays;
    int _m_itemData[1000];
};

#endif  /* HOMM3_EDITOR_MAPSPECSLOSSCONDPAGE_H */
