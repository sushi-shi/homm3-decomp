// adventuremapwindow.h - prototypes of adventuremapwindow.cpp (compiland adventuremapwindow.obj)
#ifndef HOMM3_ADVENTUREMAPWINDOW_H
#define HOMM3_ADVENTUREMAPWINDOW_H

#include "advmgr.h"

// E:\gamedcs\AdventureMapWindow.h:238, dc 0xbd0a0.
inline void TAdventureMapWindow::setBackgroundAnimation(unsigned char enable)
{
    m_animateInBackground = enable;
}

class message;

void sendChat(const char* chat, int toWho);
// Retail .bss 0x69954c, the network-session latch (remote.h owns the
// canonical declaration; same reason as above).
extern int g_networkActive69954c;

// --- globals ---
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:63, dc 0x3b0) void CheckAdvCheatCode(std::basic_string<char,std::char_traits<char>,std::allocator<char>& chatString);

// --- CAdventurMapChatEdit ---
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:257, dc 0x3274) void CAdventurMapChatEdit::CAdventurMapChatEdit(int textWidgetX, int textWidgetY, int textWidgetWidth, int textWidgetHeight, int textStringSize, char* textString, char* textFontName, int colorIndex, font::EJustify justification, char* backgroundIconName, int backgroundFrame, int textWidgetId, int textWidgetStyle, int iReadType, int textInsetX, int textInsetY);
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:261, dc 0x330c) void CAdventurMapChatEdit::SendChat(const char* sChat, int toWho);
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:311, dc 0x3414) void* CAdventurMapChatEdit::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:311, dc 0x3448) void CAdventurMapChatEdit::~CAdventurMapChatEdit();

// --- CHeroWindowEx ---
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:1445, dc 0x34e0) void CHeroWindowEx::~CHeroWindowEx();

// --- TAdvMenu ---
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:1273, dc 0x1238) void TAdvMenu::SetAdvWinButtonPalette(int id, int player);
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:1307, dc 0x1284) void TAdvMenu::TAdvMenu();
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:1448, dc 0x198c) void TAdvMenu::InitAdvMenu();
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:1475, dc 0x1a40) void TAdvMenu::~TAdvMenu();
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:1486, dc 0x1ab8) int TAdvMenu::WindowHandler(message* msg);
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:1706, dc 0x2108) void TAdvMenu::UpdateHeroLocator(int iWhich, unsigned char drawWinSect, unsigned char updateFlag);
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:1782, dc 0x23bc) void TAdvMenu::UpdateHeroLocators(int top, unsigned char drawWin, unsigned char updateFlag);
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:1842, dc 0x250c) void TAdvMenu::UpdateTownLocator(int i, unsigned char drawWinSect, unsigned char updateFlag);
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:1885, dc 0x2668) void TAdvMenu::UpdateTownLocators(int top, unsigned char drawWin, unsigned char updateFlag);
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:1942, dc 0x27e0) void TAdvMenu::HighlightLocators(unsigned char update);
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:1978, dc 0x28c0) void TAdvMenu::DoHeroKnob(unsigned char up);
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:1996, dc 0x2958) void TAdvMenu::DoTownKnob(unsigned char up);
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:2014, dc 0x29bc) unsigned char TAdvMenu::SetElevationToggleImage(int level);
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:2038, dc 0x2a28) void TAdvMenu::UpdateSpellButton(const hero* this_hero);
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:2056, dc 0x2a74) void TAdvMenu::SetSleepImage(int image);
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:2082, dc 0x2b3c) void TAdvMenu::UpdateSleepButton(const hero* this_hero);
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:2099, dc 0x2b7c) void TAdvMenu::UpdateQuestLogButton(unsigned char update);
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:2132, dc 0x2c30) void TAdvMenu::CheckDimNextHeroBut();
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:1445, dc 0x3494) void* TAdvMenu::`scalar deleting destructor'(unsigned __flags);

// --- TAdventureMapWindow ---
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:52, dc 0x370) void TAdventureMapWindow::SleepAllWidgets(unsigned char put_to_sleep);
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:505, dc 0xbf0) void TAdventureMapWindow::animate_bottom_view(unsigned char in_background);
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:1216, dc 0x118c) void TAdventureMapWindow::SetSleepImage();
// CODEVIEW(E:\gamedcs\adventuremapwindow.cpp:486, dc 0x3460) void* TAdventureMapWindow::`scalar deleting destructor'(unsigned __flags);

#endif  /* HOMM3_ADVENTUREMAPWINDOW_H */
