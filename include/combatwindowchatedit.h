// combatwindowchatedit.h - combatwindow.cpp's private chat-edit subclass.
#ifndef HOMM3_COMBATWINDOWCHATEDIT_H
#define HOMM3_COMBATWINDOWCHATEDIT_H

#include <string>

// combatwindow.cpp is the one other retail compiland that derives from
// CChatEdit.
#include "remote.h"

// Retail 0x4721d0 allocates 0x74 bytes, calls CChatEdit's forwarding
// constructor, clears the byte at +0x70, then installs vtable 0x63d4bc.
// Dreamcast supplies the four overrides and their signatures; the retail
// vtable keeps them in CChatEdit's inherited slots 15, 19, 21 and 24.
class CCombatChatEdit : public CChatEdit {
public:
    unsigned char field_70;
    char pad_71[3];

    CCombatChatEdit(int x, int y, int w, int h, int textSize, char* text,
                    char* fontName, font::TColor color,
                    font::EJustify justification, char* backgroundIcon,
                    int backgroundFrame, int id, int style, int readType,
                    int insetX, int insetY);
    virtual int OnKeyPress(message* msg);              // slot 15
    virtual void UpdateScreen();                       // slot 19
    virtual int OnEscape(message msg);                 // slot 21
    virtual void SendChat(const char* text, int toWho); // slot 24
};
SIZE(CCombatChatEdit, 0x74);

// combatwindow.cpp:42, dc 0x69638. The chat editor calls this before sending
// local-game input so combat-only cheat words can be consumed in place.
void CheckCombatCheatCode(std::string& chatString);

#endif  /* HOMM3_COMBATWINDOWCHATEDIT_H */
