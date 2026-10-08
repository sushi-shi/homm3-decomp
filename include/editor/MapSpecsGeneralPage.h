// MapSpecsGeneralPage.h - the map specifications' general page (Loki
// MapSpecsGeneralPage.cpp): the map name, description, difficulty and
// two-level flag. Not a CWnd: the image lays out two words the page never
// touches, the dialog-data copies the widgets are filled from (difficulty
// radio, description, name, two-level check), the map, the edited values
// and the modified flag, then the vtable pointer (OnOK, OnApply,
// OnInitDialog). _m_intDifficulty is OnOK's assert name for the edited
// difficulty, which operator!=<TGameMap::TDifficulty> compares with the
// map's. The two-level flags are BOOLs, read with the Windows idiom
// `!= FALSE` (cmpb/setne in the getter and in OnOK's comparison). The other
// member names and the two leading words' types are not proven.
#ifndef HOMM3_EDITOR_MAPSPECSGENERALPAGE_H
#define HOMM3_EDITOR_MAPSPECSGENERALPAGE_H

#include "editor/stdafx.h"

#include <string>

#include "editor/GameMap.h"

class TMapSpecsGeneralPage {
public:
    TMapSpecsGeneralPage(const TGameMap& map);
    ~TMapSpecsGeneralPage();

    virtual void OnOK();
    virtual void OnApply();
    virtual BOOL OnInitDialog();

    bool getBTwoLevelMap() const { return _m_bTwoLevelMap != FALSE; }
    const string& getName() const { return _m_name; }
    const string& getDesc() const { return _m_desc; }
    TGameMap::TDifficulty getDifficulty() const { return _m_intDifficulty; }
    bool wasModified() const { return _m_bModified; }

private:
    int _m_reserved[2];
    int _m_difficultyRadio;
    string _m_descEdit;
    string _m_nameEdit;
    BOOL _m_bTwoLevelMapCheck;
    const TGameMap& _m_map;
    TGameMap::TDifficulty _m_intDifficulty;
    BOOL _m_bTwoLevelMap;
    string _m_name;
    string _m_desc;
    bool _m_bModified;
};

#endif  /* HOMM3_EDITOR_MAPSPECSGENERALPAGE_H */
