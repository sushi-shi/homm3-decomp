// SpellScrollPropsSheet.h - the spell scroll property sheet
// (SpellScrollPropsSheet.cpp; GOG only, TArtifactPropsSheet's twin): the
// general and guardians pages over the scroll itself, which DoModal
// updates from the pages that changed. Layout from the image: the scroll
// at 0x88 and the two pages' auto_ptrs (0xa0 bytes).
#ifndef HOMM3_EDITOR_SPELLSCROLLPROPSSHEET_H
#define HOMM3_EDITOR_SPELLSCROLLPROPSSHEET_H

#include <memory>

#include "gameversion.h"
#include "editor/SpellScrollPropsGeneralPage.h"
#include "editor/TreasurePropsGuardiansPage.h"

class TSpellScroll;

class TSpellScrollPropsSheet : public CPropertySheet {
public:
    TSpellScrollPropsSheet(CWnd* pParentWnd, TSpellScroll* pSpellScroll, EGameVersion mapVersion);
    virtual ~TSpellScrollPropsSheet();

    virtual int DoModal();
    bool wasModified() const;

protected:
    DECLARE_MESSAGE_MAP()

private:
    TSpellScroll* _m_pSpellScroll;
    std::auto_ptr<TSpellScrollPropsGeneralPage> _m_pGeneralPage;
    std::auto_ptr<TTreasurePropsGuardiansPage> _m_pGuardiansPage;
};

#endif  /* HOMM3_EDITOR_SPELLSCROLLPROPSSHEET_H */
