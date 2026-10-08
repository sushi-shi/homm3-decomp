// ToolkitBase.h - the toolkit panes' bases (Loki ToolkitBase.cpp).
// A toolkit is a CWnd over its glade widget; the erase, road and river
// toolkits own one TToolkitBaseToolBar each. The constructors keep only
// the widget: the unsigned arguments (41, 193, 211 and 234 for the erase,
// road, river and terrain toolkits; 111/112/37 and 1 for their tool bars)
// are not stored, and their meaning and names are not recovered. Neither
// class declares a destructor (their vtables hold OnCaptureChanged alone).
// A tool bar is 20 bytes (the toolkits' operator new): two words follow
// CWnd that its constructor leaves unset; they are named after the two
// arguments they presumably once kept. The file name and those member
// names are not proven.
#ifndef HOMM3_EDITOR_TOOLKITBASE_H
#define HOMM3_EDITOR_TOOLKITBASE_H

#include "editor/stdafx.h"

class TToolkitBase : public CWnd {
public:
    TToolkitBase(unsigned int id, GtkWidget* thisWidget);
};

class TToolkitBaseToolBar : public CWnd {
public:
    TToolkitBaseToolBar(GtkWidget* thisWidget, unsigned int firstID, unsigned int numTools = 1);

private:
    unsigned int _m_firstID;
    unsigned int _m_numTools;
};

#endif  /* HOMM3_EDITOR_TOOLKITBASE_H */
