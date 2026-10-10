// MonsterPropsTreasurePage.h - the treasure page of the monster property
// sheet (MonsterPropsTreasurePage.cpp; GOG only): the resources and the
// artifact the monster guards. Layout from the image: the artifact combo
// at 0x8c, the monster at 0xc8, the map's version at 0xcc, one quantity
// with its edit and spin per resource type at 0xd0 (0x7c bytes each), the
// artifact at 0x434 and the modified flag at 0x438 (0x43c bytes, the
// sheet's new).
#ifndef HOMM3_EDITOR_MONSTERPROPSTREASUREPAGE_H
#define HOMM3_EDITOR_MONSTERPROPSTREASUREPAGE_H

#include "artifact_type.h"
#include "gameversion.h"
#include "editor/GameResource.h"
#include "editor/resource.h"

class TMonster;

class TMonsterPropsTreasurePage : public CPropertyPage {
public:
    TMonsterPropsTreasurePage(TMonster* pMonster, EGameVersion mapVersion);
    virtual ~TMonsterPropsTreasurePage();

    bool wasModified() const { return _m_bModified; }
    unsigned int getResourceQuantity(TGameResourceType type) const
    {
        return _m_aResourceControls[type].m_quantity;
    }
    TArtifact getArtifact() const { return _m_artifact; }

    enum { IDD = IDD_MONSTER_PROPS_TREASURE };
    CComboBox _m_artifactCombo;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    DECLARE_MESSAGE_MAP()

private:
    // One resource: its quantity, edit and spin.
    struct _TResourceControls {
        unsigned int m_quantity;
        CEdit m_quantityEdit;
        CSpinButtonCtrl m_quantitySpin;
    };

    bool _areResourceQuantitiesModified() const;

    TMonster* _m_pMonster;
    EGameVersion _m_mapVersion;
    _TResourceControls _m_aResourceControls[kNumGameResourceTypes];
    TArtifact _m_artifact;
    bool _m_bModified;
};

#endif  /* HOMM3_EDITOR_MONSTERPROPSTREASUREPAGE_H */
