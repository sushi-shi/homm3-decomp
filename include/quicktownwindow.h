// quicktownwindow.h - prototypes of quicktownwindow.cpp (compiland quicktownwindow.obj)
#ifndef HOMM3_QUICKTOWNWINDOW_H
#define HOMM3_QUICKTOWNWINDOW_H

#include "window.h"

class Town;
class garrison;
class ArmyGroup;

// Retail .bss 0x6a7a70. This is the quick-view label consumed by the
// garrison constructor; storage ownership remains with the text-loading TU.
DATA(0x006a7a70) extern const char* g_quickViewGarrisonText;

// Both retail constructors initialize heroWindow directly, install vtable
// 0x6406f4, and touch no storage beyond heroWindow's proven 0x4c-byte extent.
// Before normalization (type): TQuickTownWindow.
class QuickTownWindow : public heroWindow {
public:
// Before normalization (type): TQuickTownWindow::TViewLevel.
    enum ViewLevel {
        ViewNone = 0,
        ViewArmyTypes = 1,
        ViewArmySizes = 2,
        ViewAll = 3
    };

    // Dreamcast CodeView EWidgetIDs/NWIDGETS; every value is independently
    // present as a retail widget id in the two constructors.
    enum EWidgetIDs {
        BACKGROUND_ID = 2000,
        PORTRAIT_ID = 2001,
        NAME_ID = 2002,
        HALL_LEVEL_ID = 2003,
        CASTLE_LEVEL_ID = 2004,
        TYPE_LEVEL_NAME_ID = 2005,
        GOLD_PER_DAY_ID = 2006,
        RESOURCE_BONUS_ID = 2007,
        GARRISON_HERO_ID = 2008,
        ARMY_1_SPRITE_ID = 2009
    };

    enum {
        NWIDGETS = 25,
        SINGLE_RESOURCE_BONUS = 1,
        DOUBLE_RESOURCE_BONUS = 2
    };

    QuickTownWindow(const Town* thisTown, ViewLevel viewLevel);
    QuickTownWindow(const garrison* thisGarrison, ViewLevel viewLevel);
    virtual ~QuickTownWindow();
    void center(long newX, long newY);
    void quickWindowWait();
    void initializeArmyDisplay(const ArmyGroup& currentArmyGroup,
                                 ViewLevel viewLevel);
};
SIZE(QuickTownWindow, 0x4c);

// --- TQuickTownWindow ---
// CODEVIEW(E:\gamedcs\quicktownwindow.cpp:39, dc 0x117e48) void TQuickTownWindow::TQuickTownWindow(const town* thisTown, TQuickTownWindow::TViewLevel view_level);
// CODEVIEW(E:\gamedcs\quicktownwindow.cpp:260, dc 0x1187f8) void TQuickTownWindow::QuickWindowWait();
// CODEVIEW(E:\gamedcs\quicktownwindow.cpp:139, dc 0x118848) void* TQuickTownWindow::`scalar deleting destructor'(unsigned __flags);

#endif  /* HOMM3_QUICKTOWNWINDOW_H */
