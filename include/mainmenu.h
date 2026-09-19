#ifndef HOMM3_MAINMENU_H
#define HOMM3_MAINMENU_H

#include "window.h"

class message;

struct TMainMenuButtonRect {
    short m_x;
    short m_y;
    short m_width;
    short m_height;
};
SIZE(TMainMenuButtonRect, 0x8);

// DC gives bShowCDMessage@68 and RolloverWidget@72. Retail's 8-byte-larger
// heroWindow moves them to +0x4c/+0x50; the constructor stores +0x4c and
// oldmain's two stack instances independently prove the 0x54 total size.
class TMainMenu : public heroWindow {
public:
    enum EGameCommandIDs {
        NEW_GAME_ID = 101,
        LOAD_GAME_ID,
        HIGH_SCORE_ID,
        CREDITS_ID,
        QUIT_ID,
        SAVE_GAME_ID,
        RESTART_ID,
        MAIN_MENU_ID
    };

    enum EOtherWidgetIDs {
        BACKGROUND_ID = 200,
        TITLE_ID,
        VERSION_ID,
        ROLLOVER_ID
    };

    enum { NWIDGETS = 10 };

    TMainMenu();
    virtual ~TMainMenu();
    void doModal();

    friend int mainMenuHandler(message& msg);

    unsigned char m_showCdMessage;

private:
    // The preceding byte field and following four-byte field establish
    // this alignment gap; the reference layout retains the same boundary.
    char m_paddingBeforeRolloverWidget[3];
    widget* m_rolloverWidget;
};
SIZE(TMainMenu, 0x54);

// SetupCDDrive's result. Dreamcast kb.cpp's static SetupCDRom and retail
// oldmain both dispatch on this value before the front-end is opened.
extern int g_cdDriveNumber;

#endif  /* HOMM3_MAINMENU_H */
