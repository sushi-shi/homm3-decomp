// towngatewindow.h - towngatewindow.cpp (compiland towngatewindow.obj)
#ifndef HOMM3_TOWNGATEWINDOW_H
#define HOMM3_TOWNGATEWINDOW_H

#include <vector>
#include "advmgr_popup.h"

static void townGateSliderCallback(int state, heroWindow* parentWindow);

class TTownGateWindow : public CAdvPopup {
public:
    // Dreamcast CodeView publishes this nested enum in full. Complete's
    // constructor independently uses every value in the same roles.
    enum EWidgetIDs {
        BACKGROUND_ID = 0,
        TITLE_TEXT_ID = 1,
        SELECT_TEXT_ID = 2,
        ICON_ID = 3,
        TOWN_0_ID = 4,
        TOWN_1_ID = 5,
        TOWN_2_ID = 6,
        TOWN_3_ID = 7,
        TOWN_4_ID = 8,
        TOWN_5_ID = 9,
        TOWN_6_ID = 10,
        TOWN_7_ID = 11,
        TOWN_8_ID = 12,
        SELECTOR_ID = 13,
        SLIDER_ID = 14,
        NUM_TOWN_ENTRIES = 9
    };

private:
    std::vector<int> m_towns;
    int m_topTown;
    int m_selectedTown;
    bool m_adventureSpell;

public:
    TTownGateWindow(bool adventureSpell);
    virtual ~TTownGateWindow();
    void addTown(int newTown);

private:
    void updateTownLocator(int i);

public:
    // DC callback 0x169ba8 calls this private method; retail 0x5c2980 agrees.
    friend void townGateSliderCallback(int state, heroWindow* parentWindow);
    void doModal();
    virtual int windowHandler(message& msg);

private:
    void updateTownLocators();
};
SIZE(TTownGateWindow, 0x7c);

#endif  /* HOMM3_TOWNGATEWINDOW_H */
