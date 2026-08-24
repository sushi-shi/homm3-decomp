// combatwindowchatedit.h - combatwindow.cpp's private chat-edit subclass.
#ifndef HOMM3_COMBATWINDOWCHATEDIT_H
#define HOMM3_COMBATWINDOWCHATEDIT_H

// remote.h normally hides CChatEdit from consumers because declaring its
// large virtual family perturbs VC6's handle population.  combatwindow.cpp is
// the one other retail compiland that derives from it.
#define HOMM3_CHAT_EDIT_DECLS
#include "remote.h"
#undef HOMM3_CHAT_EDIT_DECLS

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
    virtual ~CCombatChatEdit();
    virtual int OnKeyPress(message* msg);              // slot 15
    virtual void UpdateScreen();                       // slot 19
    virtual int OnEscape(message msg);                 // slot 21
    virtual void SendChat(const char* text, int toWho); // slot 24
};
SIZE(CCombatChatEdit, 0x74);

#endif  /* HOMM3_COMBATWINDOWCHATEDIT_H */
