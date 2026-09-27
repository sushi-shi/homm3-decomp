#include "va.h"

#include "dialogbox.h"

#include "border.h"
#include "font.h"
#include "iconwdgt.h"
#include "kb.h"
#include "message.h"
#include "textwdgt.h"
#include "widget.h"
#include "winmgr.h"

VA(0x0048fdc0, 0x6F) MAC_ADDRESS(0x0a1500, 0x74)  // dc 0x81748
TDialogBox::TDialogBox(int winX, int winY, int winWidth,
                       int winHeight, unsigned winType)
    : heroWindow(winX, winY, winWidth, winHeight, winType)
{
    setup(winX, winY, winWidth, winHeight);
}

VA_COMPGEN(0x0048fe30, 0x21, SCALAR_DELETING_DTOR, TDialogBox)

VA(0x0048fe60, 0x2A) MAC_ADDRESS(0x0a1574, 0x4c)  // dc 0x817b0
TDialogBox::TDialogBox(unsigned winType)
    : heroWindow(0, 0, 800, 600, winType)
{
}

VA(0x0048fe90, 0x6B) MAC_ADDRESS(0x0a15c0, 0xac)  // dc 0x817f8
TDialogBox::~TDialogBox()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

VA(0x0048ff00, 0x833) MAC_ADDRESS(0x0a166c, 0xa48)  // dc 0x8185c
unsigned char TDialogBox::setup(int winX, int winY,
                                int winWidth, int winHeight)
{
    m_x = winX;
    m_y = winY;

    int id = 200;
    int tilesWide = winWidth / TILE_SIZE;
    int tilesHigh = winHeight / TILE_SIZE;
    const int tilesWide2 = (winWidth + EDGE_SIZE - 1) / EDGE_SIZE;
    const int tilesHigh2 = (winHeight + EDGE_SIZE - 1) / EDGE_SIZE;
    m_width = tilesWide2 * EDGE_SIZE;
    m_height = tilesHigh2 * EDGE_SIZE;

    m_widgets.reserve(tilesWide * tilesHigh + tilesWide2 * tilesHigh2);

    int row;
    int column;
    for (row = 0; row < tilesHigh; ++row) {
        for (column = 0; column < tilesWide; ++column) {
            m_widgets.push_back(new bitmapBorder(
                column * TILE_SIZE, row * TILE_SIZE,
                TILE_SIZE, TILE_SIZE, id++, "diboxbck.pcx", 0x800));
        }
    }

    const int moreWidth = m_width - tilesWide * TILE_SIZE;
    if (moreWidth > 0 && tilesHigh > 0) {
        for (row = 0; row < tilesHigh; ++row) {
            m_widgets.push_back(new bitmapBorder(
                tilesWide * TILE_SIZE, row * TILE_SIZE,
                moreWidth, TILE_SIZE, id++, "diboxbck.pcx", 0x800));
        }
    }

    const int moreHeight = m_height - tilesHigh * TILE_SIZE;
    if (moreHeight > 0 && tilesWide > 0) {
        for (column = 0; column < tilesWide; ++column) {
            m_widgets.push_back(new bitmapBorder(
                column * TILE_SIZE, tilesHigh * TILE_SIZE,
                TILE_SIZE, moreHeight, id++, "diboxbck.pcx", 0x800));
        }
    }

    if (moreWidth > 0 && moreHeight > 0) {
        m_widgets.push_back(new bitmapBorder(
            tilesWide * TILE_SIZE, tilesHigh * TILE_SIZE,
            moreWidth, moreHeight, id++, "diboxbck.pcx", 0x800));
    }

    m_beginId = id;

    m_widgets.push_back(new iconWidget(
        0, 0, EDGE_SIZE, EDGE_SIZE, id++, "dialgbox.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        m_width - EDGE_SIZE, 0, EDGE_SIZE, EDGE_SIZE, id++, "dialgbox.def",
        1, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0, m_height - EDGE_SIZE, EDGE_SIZE, EDGE_SIZE, id++, "dialgbox.def",
        2, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        m_width - EDGE_SIZE, m_height - EDGE_SIZE,
        EDGE_SIZE, EDGE_SIZE, id++, "dialgbox.def",
        3, 0, 0, 0, 0x10));

    int edge;
    for (edge = 1; edge < tilesWide2 - 1; ++edge) {
        m_widgets.push_back(new iconWidget(
            edge * EDGE_SIZE, 0, EDGE_SIZE, EDGE_SIZE,
            id++, "dialgbox.def", 6, 0, 0, 0, 0x10));
        m_widgets.push_back(new iconWidget(
            edge * EDGE_SIZE, m_height - EDGE_SIZE,
            EDGE_SIZE, EDGE_SIZE, id++, "dialgbox.def",
            7, 0, 0, 0, 0x10));
    }

    for (edge = 1; edge < tilesHigh2 - 1; ++edge) {
        m_widgets.push_back(new iconWidget(
            0, edge * EDGE_SIZE, EDGE_SIZE, EDGE_SIZE,
            id++, "dialgbox.def", 4, 0, 0, 0, 0x10));
        m_widgets.push_back(new iconWidget(
            m_width - EDGE_SIZE, edge * EDGE_SIZE,
            EDGE_SIZE, EDGE_SIZE, id++, "dialgbox.def",
            5, 0, 0, 0, 0x10));
    }

    m_endId = id - 1;
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }

    return 1;
}

VA_COMPGEN(0x00490740, 0x21, SCALAR_DELETING_DTOR, CTextDialog)

VA_COMPGEN(0x00490770, 0x6B, IMPLICIT_DTOR, CTextDialog) MAC_COMPGEN_ADDRESS(0x0a23a8, 0x60, IMPLICIT_DTOR, CTextDialog)

// Original: CTextDialog::CTextDialog; dialogbox.cpp:139, dc 0x81d8c.
CTextDialog::CTextDialog(const char* text, font* currentFont, unsigned winType)
    : TDialogBox(winType), m_textWidget(0)
{
    setup(text, currentFont);
}

VA(0x004907e0, 0x31) MAC_ADDRESS(0x0a20b4, 0x40)  // dc 0x81e00
CTextDialog::CTextDialog(unsigned winType)
    : TDialogBox(winType)
{
    m_textWidget = 0;
}

VA(0x00490820, 0x26B) MAC_ADDRESS(0x0a20f4, 0x17c)  // dc 0x81e38
unsigned char CTextDialog::setup(const char* text, font* currentFont)
{
    int winX;
    int winY;
    int winWidth;
    int winHeight;

    calcDimensions(text, currentFont, winX, winY, winWidth, winHeight);
    m_x = winX;
    m_y = winY;
    m_width = winWidth;
    m_height = winHeight;
    TDialogBox::setup(winX, winY, winWidth, winHeight);

    m_textWidget = new textWidget(
        20, 40, winWidth - 40, winHeight - 40,
        text, currentFont->getName(), font::PRIMARY, -1, 1, 0, 8);
    m_widgets.push_back(m_textWidget);
    addWidget(m_textWidget, -1);
    return 1;
}

VA(0x00490a90, 0x8C) MAC_ADDRESS(0x0a2270, 0xd4)  // dc 0x81f00
void CTextDialog::calcDimensions(const char* text, font* currentFont,
                                 int& winX, int& winY,
                                 int& winWidth, int& winHeight)
{
    int lines = currentFont->lineLength(text, 344);
    winHeight = currentFont->m_fs.m_height;
    winHeight *= lines;
    winWidth = currentFont->longestLineWidth(text);
    if (winWidth > 344)
        winWidth = currentFont->longestWrappedLineWidth(text, 344);

    winWidth = ((winWidth + EDGE_SIZE - 1) & ~(EDGE_SIZE - 1)) + 40;
    winHeight += 40;
    winX = (800 - winWidth) / 2;
    winY = (600 - winHeight) / 2;
}

VA(0x00490b20, 0x2C) MAC_ADDRESS(0x0a2344, 0x2c)  // dc 0x81f98
int CTextDialog::exitDialog(message& msg)
{
    msg.m_id = MESSAGE_WIDGET;
    g_windowManager->m_dialogReturn = msg.m_codeY;
    msg.m_codeY = widget::WIDGET_END_DIALOG;
    msg.m_codeX = widget::WIDGET_END_DIALOG;
    return MESSAGE_DISPATCH_FORWARD;
}

VA(0x00490b50, 0x17) MAC_ADDRESS(0x0a2370, 0x38)  // dc 0x81fb0
void CTextDialog::updateText(const char* newText)
{
    if (m_textWidget)
        m_textWidget->setText(newText);
}
