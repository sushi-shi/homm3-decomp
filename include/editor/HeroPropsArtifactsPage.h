// HeroPropsArtifactsPage.h - the hero sheet's artifacts page (Loki
// HeroPropsArtifactsPage.cpp): a two-column list of the worn and backpack
// artifacts (row data: the slot in the high word, the artifact in the low
// word), the add/edit/remove buttons and the spellbook toggle. Layout from
// the image: the list selection, the customize toggle state (_m_bCustom),
// the hero, the prototype, its class and number, the unused slots
// (_m_unusedSlots), the backpack size (_m_backpackSize), the edited
// artifacts, the custom flag, the custom artifacts, the modified flag, the
// list's item count (_m_itemCount), then the vtable pointer (OnOK,
// OnInitDialog). Names from the asserts as listed; the others are not
// proven.
#ifndef HOMM3_EDITOR_HEROPROPSARTIFACTSPAGE_H
#define HOMM3_EDITOR_HEROPROPSARTIFACTSPAGE_H

#include "editor/stdafx.h"

#include "editor/Hero.h"
#include "editor/EditArtifactDlg.h"

class THeroPropsArtifactsPage {
public:
    THeroPropsArtifactsPage(const THero& hero);
    ~THeroPropsArtifactsPage();

    void setIdentity(THeroClass newHeroClass, unsigned int newProtoNum);

    virtual void OnOK();
    virtual BOOL OnInitDialog();
    void UpdateData(bool bSaveAndValidate);
    void OnCustomizeCheck();
    void OnAddArtifactButton();
    void OnEditArtifactButton();
    void OnRemoveArtifactButton();
    void OnRemoveAllArtifactButton();

    bool getBCustomArtifacts() const { return _m_bCustomArtifacts; }
    const THeroPrototype::TArtifactContainer& getArtifacts() const { return _m_artifacts; }
    bool wasModified() const { return _m_bModified; }
    void OnListSelect(int row) { _m_curSel = row; }

private:
    void _setArtifacts(const THeroPrototype::TArtifactContainer& artifacts);
    void _retrieveArtifacts(THeroPrototype::TArtifactContainer* pArtifacts);
    void _enableControls();
    void _disableControls();
    int _addItem(TArtifact artifact, TArtifactSlot slot);
    bool _setItem(int item, TArtifact artifact, TArtifactSlot slot);

    int _m_curSel;
    bool _m_bCustom;
    const THero& _m_hero;
    const THeroPrototype* _m_pPrototype;
    THeroClass _m_heroClass;
    unsigned int _m_protoNum;
    TArtifactSlotSet _m_unusedSlots;
    unsigned int _m_backpackSize;
    THeroPrototype::TArtifactContainer _m_editArtifacts;
    bool _m_bCustomArtifacts;
    THeroPrototype::TArtifactContainer _m_artifacts;
    bool _m_bModified;
    int _m_itemCount;
};

#endif  /* HOMM3_EDITOR_HEROPROPSARTIFACTSPAGE_H */
