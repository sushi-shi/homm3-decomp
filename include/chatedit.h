// Shared chat-edit hierarchy used by the adventure, combat, swap, and
// multiplayer windows. Dreamcast CodeView proves the source inheritance;
// retail vtables and constructor lowering prove the translated VC6 layout.
#ifndef HOMM3_CHATEDIT_H
#define HOMM3_CHATEDIT_H

#include "textntry.h"

class message;

// Retail vtable 0x640e30. Slots 0..18 are textEntryWidget's exact prefix;
// Dreamcast supplies the seven introduced method names at slots 19..24 and
// proves that this class adds no data (its 0x70-byte extent equals retail's
// textEntryWidget extent). The retail bodies independently confirm the base
// tail offsets: IsOpen reads cursorIndex at +0x58 and the edit actions use
// Text at +0x30.
class CChatEdit : public textEntryWidget {
public:
    CChatEdit(int x, int y, int w, int h, int textSize, char* text,
              char* fontName, font::TColor color,
              font::EJustify justification,
              char* backgroundIcon, int backgroundFrame, int id, int style,
              int readType, int insetX, int insetY);
    virtual ~CChatEdit();
    virtual int onKeyPress(message* msg);                       // slot 15
    virtual unsigned char ignoreKey(message* msg);              // slot 16
    virtual void updateScreen();                                // slot 19
    virtual int onEnter(message msg);                            // slot 20
    virtual int onEscape(message msg);                           // slot 21
    virtual int onFunctionKey(message msg, int toWho);           // slot 22
    virtual bool isOpen();                                      // slot 23
    virtual void sendChat(const char* text, int toWho) = 0;      // slot 24
};

// Dreamcast remote.h proves this intermediate class. Retail constructors for
// both surviving derived editors expand its forwarding ctor into a direct
// CChatEdit call followed by the +0x70 clear.
class CGameChatEdit : public CChatEdit {
public:
    CGameChatEdit(int x, int y, int w, int h, int textSize, char* text,
                  char* fontName, font::TColor color,
                  font::EJustify justification, char* backgroundIcon,
                  int backgroundFrame, int id, int style, int readType,
                  int insetX, int insetY);
    virtual int onKeyPress(message* msg);
    virtual int onEscape(message msg);
    virtual void sendChatCleanup();
    virtual void activate();

    // at +0x70. Retail activate/onEscape/sendChatCleanup set/clear it.
    unsigned char m_activated;
    // has only the activation byte after CChatEdit and size 0x74.
    char m_paddingAfterActivated[3];
};

// Dreamcast adventuremapwindow.cpp proves this final derived editor. Retail's
// adventure-window constructor expands its forwarding constructor through
// CGameChatEdit, then writes this class's vtable after the shared +0x70 clear.
class CAdventurMapChatEdit : public CGameChatEdit {
public:
    CAdventurMapChatEdit(
        int x, int y, int w, int h, int textSize, char* text,
        char* fontName, font::TColor color, font::EJustify justification,
        char* backgroundIcon, int backgroundFrame, int id, int style,
        int readType, int insetX, int insetY);
    virtual void sendChat(const char* text, int toWho);
};

inline CGameChatEdit::CGameChatEdit(
    int x, int y, int w, int h, int textSize, char* text, char* fontName,
    font::TColor color, font::EJustify justification, char* backgroundIcon,
    int backgroundFrame, int id, int style, int readType, int insetX,
    int insetY)
    : CChatEdit(x, y, w, h, textSize, text, fontName, color, justification,
                backgroundIcon, backgroundFrame, id, style, readType,
                insetX, insetY)
{
    m_activated = 0;
}

#endif  /* HOMM3_CHATEDIT_H */
