// DialogTemplate.h - an in-memory template for a child dialog with no
// controls, menu, class or title (h3maped 0x40e70b, first used by
// EditQuestDlg.cpp): the pages that switch between condition or reward
// dialogs create their empty one from it.
#ifndef HOMM3_EDITOR_DIALOGTEMPLATE_H
#define HOMM3_EDITOR_DIALOGTEMPLATE_H

struct TEmptyDialogTemplate : public DLGTEMPLATE {
    TEmptyDialogTemplate() : m_menu(0), m_windowClass(0), m_title(0)
    {
        style = WS_CHILD | DS_CONTROL;
        dwExtendedStyle = 0;
        cdit = 0;
        x = y = 0;
        cx = cy = 0;
    }

    WORD m_menu;
    WORD m_windowClass;
    WORD m_title;
};

#endif  /* HOMM3_EDITOR_DIALOGTEMPLATE_H */
