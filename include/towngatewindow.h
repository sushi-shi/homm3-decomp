// towngatewindow.h - prototypes of towngatewindow.cpp (compiland towngatewindow.obj)
#ifndef HOMM3_TOWNGATEWINDOW_H
#define HOMM3_TOWNGATEWINDOW_H

#include <vector>
#include "advmgr_popup.h"

static void townGateSliderCallback(int state, heroWindow* parentWindow);

// Before normalization (type): TTownGateWindow.
class TownGateWindow : public CAdvPopup {
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
    TownGateWindow(bool adventureSpell);
    virtual ~TownGateWindow();
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
SIZE(TownGateWindow, 0x7c);

// --- TTownGateWindow ---
// CODEVIEW(E:\gamedcs\towngatewindow.cpp:168, dc 0x1699e8) void TTownGateWindow::UpdateTownLocators();
// CODEVIEW(E:\gamedcs\towngatewindow.cpp:98, dc 0x169c98) void* TTownGateWindow::`scalar deleting destructor'(unsigned __flags);

// --- game ---
// CODEVIEW(E:\gamedcs\game.h:897, dc 0x169c0c) unsigned char game::GetNumAllies(int playerNum);
// CODEVIEW(E:\gamedcs\game.h:1022, dc 0x169c60) const town* game::GetTown(int which);
// CODEVIEW(E:\gamedcs\game.h:1027, dc 0x169c7c) const char* game::GetTownName(int iTownId);

// --- std ---
// CODEVIEW(..\stlport\stl_vector.c:68, dc 0x169ccc) void std::vector<int,std::allocator<int> >::reserve(unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:199, dc 0x169d60) unsigned std::vector<int,std::allocator<int> >::capacity();
// CODEVIEW(..\stlport\stl_vector.h:514, dc 0x169d6c) std::vector<int,std::allocator<int> >::_M_allocate_and_copy(unsigned __n, int* __first, int* __last);

#endif  /* HOMM3_TOWNGATEWINDOW_H */
