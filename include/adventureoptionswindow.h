// adventureoptionswindow.h - prototypes of adventureoptionswindow.cpp (compiland adventureoptionswindow.obj)
#ifndef HOMM3_ADVENTUREOPTIONSWINDOW_H
#define HOMM3_ADVENTUREOPTIONSWINDOW_H

#include <va.h>
#include "advmgr_popup.h"

class TextWidget;

// Dreamcast supplies the sole derived member name and offset (+0x58 on its
// 0x58-byte CAdvPopup). Retail's proven 0x60 base shifts the pointer to +0x60;
// advManager::DoAdventureOptions allocates exactly 0x64 bytes on its stack.
// Before normalization (type): TAdventureOptionsWindow.
class AdventureOptionsWindow : public CAdvPopup {
public:
    enum WidgetIDs {
        VIEW_WORLD_ID = 1,
        VIEW_PUZZLE_ID = 2,
        VIEW_SCENARIO_ID = 3,
        DIG_ID = 4,
        REPLAY_ID = 5,
        ADVENTURE_OPTION_BACKGROUND_ID = 200,
        ADVENTURE_OPTION_ROLLOVER_ID = 201,
        ADVENTURE_OPTION_ACCEPT_ID = 0x7802
    };

// Before normalization (type): AdventureOptionsWindow::EHotkeys.
    enum Hotkeys {
        ADVENTURE_OPTION_VIEW_HOTKEY = 47,
        ADVENTURE_OPTION_PUZZLE_HOTKEY = 25,
        ADVENTURE_OPTION_DIG_HOTKEY = 32,
        ADVENTURE_OPTION_INFO_HOTKEY = 23,
        ADVENTURE_OPTION_TURN_HOTKEY = 19,
        ADVENTURE_OPTION_ACCEPT_HOTKEY_1 = 28,
        ADVENTURE_OPTION_ACCEPT_HOTKEY_2 = 1
    };

    AdventureOptionsWindow();
    virtual ~AdventureOptionsWindow();
    virtual int windowHandler(Message& msg);

private:
    TextWidget* m_rolloverWidget;
    int convertID2HelpID(int id) const;
};
SIZE(AdventureOptionsWindow, 0x64);

// --- TAdventureOptionsWindow ---
// CODEVIEW(E:\gamedcs\adventureoptionswindow.cpp:112, dc 0x51b0) int TAdventureOptionsWindow::convertID2HelpID(int id);
// CODEVIEW(E:\gamedcs\adventureoptionswindow.cpp:101, dc 0x53e0) void* TAdventureOptionsWindow::`scalar deleting destructor'(unsigned __flags);

#endif  /* HOMM3_ADVENTUREOPTIONSWINDOW_H */
