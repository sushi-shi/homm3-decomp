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
    virtual int OnKeyPress(message* msg);                       // slot 15
    virtual unsigned char IgnoreKey(message* msg);              // slot 16
    virtual void UpdateScreen();                                // slot 19
    virtual int OnEnter(message msg);                            // slot 20
    virtual int OnEscape(message msg);                           // slot 21
    virtual int OnFunctionKey(message msg, int toWho);           // slot 22
    virtual bool IsOpen();                                      // slot 23
    virtual void SendChat(const char* text, int toWho) = 0;      // slot 24
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
    virtual int OnKeyPress(message* msg);
    virtual int OnEscape(message msg);
    virtual void SendChatCleanup();
    virtual void Activate();

    unsigned char field_70;
    char pad_71[3];
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
    virtual void SendChat(const char* text, int toWho);
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
    field_70 = 0;
}

#endif  /* HOMM3_CHATEDIT_H */
