// BlackBoxPropsContentsPage.h - the contents page of the black box and
// event property sheets (BlackBoxPropsContentsPage.cpp; GOG only, Loki's
// port has no counterpart): a category combo over one child dialog per
// kind of content - experience, mana, morale, luck, resources, primary
// skills, secondary skills, artifacts, spells and creatures. Layout from
// the image: the category combo at 0x8c, the shown category at 0xc8, the
// dialogs (allocated together) at 0xcc and each category's dialog at 0xd0.
#ifndef HOMM3_EDITOR_BLACKBOXPROPSCONTENTSPAGE_H
#define HOMM3_EDITOR_BLACKBOXPROPSCONTENTSPAGE_H

#include <vector>

#include "gameversion.h"
#include "editor/BlackBox.h"
#include "editor/resource.h"

class TBlackBoxPropsContentsPage : public CPropertyPage {
public:
    enum { s_kNumCategories = 10 };

    TBlackBoxPropsContentsPage(const TBlackBox* pBlackBox, EGameVersion mapVersion, UINT nIDHelp);
    virtual ~TBlackBoxPropsContentsPage();

    int getExperienceBonus() const;
    int getManaBonus() const;
    int getMoraleBonus() const;
    int getLuckBonus() const;
    const TResourceQuantities& getResourceQuantities() const;
    const TPrimarySkillBonuses& getPrimarySkillBonuses() const;
    const std::vector<TSecondarySkillRecord>& getSecondarySkills() const;
    const std::vector<TArtifact>& getArtifacts() const;
    const std::vector<SpellID>& getSpells() const;
    const std::vector<TCreatureStack>& getCreatureStacks() const;
    bool wasModified() const;

    enum { IDD = IDD_BLACK_BOX_PROPS_CONTENTS };
    CComboBox _m_categoryCombo;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    afx_msg void OnSelChangeCategoryCombo();
    DECLARE_MESSAGE_MAP()

private:
    struct _TDialogs;

    unsigned int _m_category;
    _TDialogs* _m_pDialogs;
    CDialog* _m_apCategoryDlg[s_kNumCategories];
};

#endif  /* HOMM3_EDITOR_BLACKBOXPROPSCONTENTSPAGE_H */
