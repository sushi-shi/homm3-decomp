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
    // this on TAB; onEscape/sendChat clear it. Name follows the DC-proven
    // CGameChatEdit::activated counterpart; this class has no full DC type.
    unsigned char m_activated;
    // the activation byte at +0x70, within the retail 0x74-byte allocation.
    char m_paddingAfterActivated[3];

    CCombatChatEdit(int x, int y, int w, int h, int textSize, char* text,
                    char* fontName, font::TColor color,
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
