// BlackBoxPropsSheet.h - the black box property sheet
// (BlackBoxPropsSheet.cpp; GOG only): the general, guardians and contents
// pages over the box itself, which DoModal updates from the pages that
// changed. Layout from the image: the box at 0x88 and the three pages'
// auto_ptrs (0xa8 bytes).
#ifndef HOMM3_EDITOR_BLACKBOXPROPSSHEET_H
#define HOMM3_EDITOR_BLACKBOXPROPSSHEET_H

#include <memory>

#include "gameversion.h"
#include "editor/BlackBoxPropsContentsPage.h"
#include "editor/BlackBoxPropsGeneralPage.h"
#include "editor/TreasurePropsGuardiansPage.h"

class TBlackBox;

class TBlackBoxPropsSheet : public CPropertySheet {
public:
    TBlackBoxPropsSheet(CWnd* pParentWnd, TBlackBox* pBlackBox, EGameVersion mapVersion);
    virtual ~TBlackBoxPropsSheet();

    virtual int DoModal();
    bool wasModified() const;

protected:
    DECLARE_MESSAGE_MAP()

private:
    TBlackBox* _m_pBlackBox;
    std::auto_ptr<TBlackBoxPropsGeneralPage> _m_pGeneralPage;
    std::auto_ptr<TTreasurePropsGuardiansPage> _m_pGuardiansPage;
    std::auto_ptr<TBlackBoxPropsContentsPage> _m_pContentsPage;
};

#endif  /* HOMM3_EDITOR_BLACKBOXPROPSSHEET_H */
