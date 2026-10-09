// MapSpecsArtifactsPage.h - the artifacts page of the map specifications
// sheet (MapSpecsArtifactsPage.cpp; GOG only): a check list of the
// artifacts the map allows, of those its version has; at least one stays
// allowed. Layout from the image: the map before and during the sheet at
// 0x8c, the modified flag, the disabled artifacts at 0x98 and the list at
// 0xac.
#ifndef HOMM3_EDITOR_MAPSPECSARTIFACTSPAGE_H
#define HOMM3_EDITOR_MAPSPECSARTIFACTSPAGE_H

#include <bitset>

#include "editor/GameMap.h"
#include "editor/resource.h"

class TMapSpecsArtifactsPage : public CPropertyPage {
public:
    // The artifacts each version has, and the class bit of the special
    // artifacts, which the list leaves out.
    enum {
        s_kNumRoEArtifacts = 127,
        s_kNumABArtifacts = 129,
        s_kSpecialClass = 1
    };

    TMapSpecsArtifactsPage(const TGameMap& oldMap, TGameMap& newMap);
    virtual ~TMapSpecsArtifactsPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_MAP_SPECS_ARTIFACTS };

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnCheckChangeArtifactsList();
    DECLARE_MESSAGE_MAP()

private:
    const TGameMap& _m_oldMap;
    TGameMap& _m_newMap;
    bool _m_bModified;
    std::bitset<kNumArtifacts> _m_disabledArtifacts;

public:
    CCheckListBox _m_artifactsList;
};

#endif  /* HOMM3_EDITOR_MAPSPECSARTIFACTSPAGE_H */
