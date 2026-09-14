// combatwindowchatedit.h - combatwindow.cpp's private chat-edit subclass.
#ifndef HOMM3_COMBATWINDOWCHATEDIT_H
#define HOMM3_COMBATWINDOWCHATEDIT_H

#include <string>

// CCombatChatEdit uses the CGameChatEdit base recovered from its vtable.
#include "remote.h"

// Retail 0x4721d0 allocates 0x74 bytes and installs vtable 0x63d4bc.
// CGameChatEdit owns the +0x70 byte and the two inherited chat-control slots.
class CCombatChatEdit : public CGameChatEdit {
public:
    CCombatChatEdit(int x, int y, int w, int h, int textSize, char* text,
                    char* fontName, font::Color color,
                    font::EJustify justification, char* backgroundIcon,
                    int backgroundFrame, int id, int style, int readType,
                    int insetX, int insetY);
    virtual int onKeyPress(message* msg);              // slot 15
    virtual void updateScreen();                       // slot 19
    virtual int onEscape(message msg);                 // slot 21
    virtual void sendChat(const char* text, int toWho); // slot 24
};
SIZE(CCombatChatEdit, 0x74);

// combatwindow.cpp:42, dc 0x69638. The chat editor calls this before sending
// local-game input so combat-only cheat words can be consumed in place.
void checkCombatCheatCode(std::string& chatString);

#endif  /* HOMM3_COMBATWINDOWCHATEDIT_H */
