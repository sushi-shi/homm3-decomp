// EventPropsSheet.h - the event property sheet (EventPropsSheet.cpp; GOG
// only): the general, guardians and contents pages over the event itself,
// which DoModal updates from the pages that changed. Layout from the
// image: the event at 0x88 and the three pages' auto_ptrs (0xa8 bytes).
#ifndef HOMM3_EDITOR_EVENTPROPSSHEET_H
#define HOMM3_EDITOR_EVENTPROPSSHEET_H

#include <memory>

#include "gameversion.h"
#include "editor/BlackBoxPropsContentsPage.h"
#include "editor/EventPropsGeneralPage.h"
#include "editor/TreasurePropsGuardiansPage.h"

class TEvent;

class TEventPropsSheet : public CPropertySheet {
public:
    TEventPropsSheet(CWnd* pParentWnd, TEvent* pEvent, const TPlayerMask& playersPresent, EGameVersion mapVersion);
    virtual ~TEventPropsSheet();

    virtual int DoModal();
    bool wasModified() const;

protected:
    DECLARE_MESSAGE_MAP()

private:
    TEvent* _m_pEvent;
    std::auto_ptr<TEventPropsGeneralPage> _m_pGeneralPage;
    std::auto_ptr<TTreasurePropsGuardiansPage> _m_pGuardiansPage;
    std::auto_ptr<TBlackBoxPropsContentsPage> _m_pContentsPage;
};

#endif  /* HOMM3_EDITOR_EVENTPROPSSHEET_H */
