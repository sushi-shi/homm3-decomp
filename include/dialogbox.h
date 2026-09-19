#ifndef HOMM3_DIALOGBOX_H
#define HOMM3_DIALOGBOX_H

#include "window.h"

struct message;
class font;
class textWidget;

// DC gives heroWindow as the sole base, followed by beginID/endID at
// +68/+72. Retail's VC6 vector widens heroWindow from 0x44 to 0x4c, so the
// translated fields are +0x4c/+0x50 and the class is 0x54. Retail vtable
// 0x63db40 has ten slots: heroWindow's nine followed by Setup.
class TDialogBox : public heroWindow {
public:
    enum {
        EDGE_SIZE = 64,
        TILE_SIZE = 256
    };

    TDialogBox(int winX, int winY, int winWidth, int winHeight,
               unsigned winType);
    TDialogBox(unsigned winType);
    virtual ~TDialogBox();

    int m_beginId;
    int m_endId;
    virtual unsigned char setup(int winX, int winY,
                                int winWidth, int winHeight);
};
SIZE(TDialogBox, 0x54);

class CTextDialog : public TDialogBox {
public:
    CTextDialog(const char* text, font* currentFont, unsigned winType);
    CTextDialog(unsigned winType);

    int exitDialog(message& msg);
    virtual unsigned char setup(const char* text, font* currentFont);
    virtual void updateText(const char* newText);

protected:
    textWidget* m_textWidget;
    virtual void calcDimensions(const char* text, font* currentFont,
                                int& winX, int& winY,
                                int& winWidth, int& winHeight);
};
SIZE(CTextDialog, 0x58);

#endif  /* HOMM3_DIALOGBOX_H */
