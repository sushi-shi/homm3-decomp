// dialogbox.h - prototypes of dialogbox.cpp (compiland dialogbox.obj)
#ifndef HOMM3_DIALOGBOX_H
#define HOMM3_DIALOGBOX_H

#include "window.h"

struct Message;
class Font;
class TextWidget;

// DC gives heroWindow as the sole base, followed by beginID/endID at
// +68/+72. Retail's VC6 vector widens heroWindow from 0x44 to 0x4c, so the
// translated fields are +0x4c/+0x50 and the class is 0x54. Retail vtable
// 0x63db40 has ten slots: heroWindow's nine followed by Setup.
// Before normalization (type): TDialogBox.
class DialogBoxWindow : public HeroWindow {
public:
    enum {
        EDGE_SIZE = 64,
        TILE_SIZE = 256
    };

    DialogBoxWindow(int winX, int winY, int winWidth, int winHeight,
               unsigned winType);
    DialogBoxWindow(unsigned winType);
    virtual ~DialogBoxWindow();

    int m_beginId;
    int m_endId;
    virtual unsigned char setup(int winX, int winY,
                                int winWidth, int winHeight);
};
SIZE(DialogBoxWindow, 0x54);

class CTextDialog : public DialogBoxWindow {
public:
    CTextDialog(const char* text, Font* currentFont, unsigned winType);
    CTextDialog(unsigned winType);

    int exitDialog(Message& msg);
    virtual unsigned char setup(const char* text, Font* currentFont);
    virtual void updateText(const char* newText);

protected:
    TextWidget* m_textWidget;
    virtual void calcDimensions(const char* text, Font* currentFont,
                                int& winX, int& winY,
                                int& winWidth, int& winHeight);
};
SIZE(CTextDialog, 0x58);

// --- CTextDialog ---
// CODEVIEW(E:\gamedcs\dialogbox.cpp:139, dc 0x81d8c) void CTextDialog::CTextDialog(const char* cText, font* pFont, unsigned winType);
// CODEVIEW(E:\gamedcs\dialogbox.cpp:143, dc 0x82034) void* CTextDialog::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\dialogbox.cpp:143, dc 0x82068) void CTextDialog::~CTextDialog();

// --- TDialogBox ---
// CODEVIEW(E:\gamedcs\dialogbox.cpp:38, dc 0x82000) void* TDialogBox::`scalar deleting destructor'(unsigned __flags);

#endif  /* HOMM3_DIALOGBOX_H */
