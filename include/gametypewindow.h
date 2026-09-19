#ifndef HOMM3_GAMETYPEWINDOW_H
#define HOMM3_GAMETYPEWINDOW_H

#include "window.h"

class message;

// Packed source table row used by the game-type menu's five button layouts.
// Retail's fixed-address movsx loads prove four consecutive shorts and an
// eight-byte stride.
struct TGameTypeButtonRect {
    short m_x;
    short m_y;
    short m_width;
    short m_height;
};
SIZE(TGameTypeButtonRect, 0x8);

// DC field evidence gives heroWindow as the sole base and RolloverWidget at
// +68.  Retail heroWindow is eight bytes wider because its shipped VC6 vector
// occupies 16 bytes, moving the same derived field to +0x4c and the complete
// object size to 0x50.  The retail constructor stores through +0x4c and the
// handler reads it, independently confirming the translated offset.
class TGameTypeWindow : public heroWindow {
public:
    enum EWidgetIDs {
        SINGLE_ID = 100,
        CAMPAIGN_ID,
        MULTIPLAYER_ID,
        TUTORIAL_ID,
        QUIT_ID,
        NEW_LOAD_ID
    };

    enum { NWIDGETS = 9 };

    TGameTypeWindow(unsigned char loadGameMode);
    virtual ~TGameTypeWindow();
    void doModal();

private:
    void update(unsigned char loadGameMode);
    int m_rolloverWidget;  // +0x4c retail (+68 DC)
};
SIZE(TGameTypeWindow, 0x50);

int gameTypeWindowHandler(message& msg);

// Cross-TU menu-mode latch; oldmain owns the setup and both front-end menus
// consume it.
extern int g_noCdRom;

#endif  /* HOMM3_GAMETYPEWINDOW_H */
