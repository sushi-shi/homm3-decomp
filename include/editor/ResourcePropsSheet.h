// ResourcePropsSheet.h - the resource property sheet
// (ResourcePropsSheet.cpp; GOG only, TArtifactPropsSheet's twin): the
// general and guardians pages over the resource itself, which DoModal
// updates from the pages that changed. Layout from the image: the resource
// at 0x88 and the two pages' auto_ptrs (0xa0 bytes).
#ifndef HOMM3_EDITOR_RESOURCEPROPSSHEET_H
#define HOMM3_EDITOR_RESOURCEPROPSSHEET_H

#include <memory>

#include "gameversion.h"
#include "editor/ResourcePropsGeneralPage.h"
#include "editor/TreasurePropsGuardiansPage.h"

class TGameResource;

class TResourcePropsSheet : public CPropertySheet {
public:
    TResourcePropsSheet(CWnd* pParentWnd, TGameResource* pResource, EGameVersion mapVersion);
    virtual ~TResourcePropsSheet();

    virtual int DoModal();
    bool wasModified() const;

protected:
    DECLARE_MESSAGE_MAP()

private:
    TGameResource* _m_pResource;
    std::auto_ptr<TResourcePropsGeneralPage> _m_pGeneralPage;
    std::auto_ptr<TTreasurePropsGuardiansPage> _m_pGuardiansPage;
};

#endif  /* HOMM3_EDITOR_RESOURCEPROPSSHEET_H */
