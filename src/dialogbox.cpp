// dialogbox.cpp - E:\gamedcs\dialogbox.cpp (compiland dialogbox.obj)
// HAND-OWNED after admission; retail bytes are authoritative.
#include <va.h>
#include "border.h"
#include "dialogbox.h"
#include "font.h"
#include "iconwdgt.h"
#include "kb.h"
#include "message.h"
#include "textwdgt.h"
#include "widget.h"
#include "winmgr.h"

// E:\gamedcs\dialogbox.cpp:36
VA(0x0048fdc0, 0x6F)  // Setup call + five-arg heroWindow ctor, dc 0x81748
TDialogBox::TDialogBox(int winX, int winY, int winWidth,
                       int winHeight, unsigned winType)
    : heroWindow(winX, winY, winWidth, winHeight, winType)
{
    setup(winX, winY, winWidth, winHeight);
}

// Retail vtable 0x63db40 slot 0.
VA_COMPGEN(0x0048fe30, 0x21, SCALAR_DELETING_DTOR, TDialogBox)

// E:\gamedcs\dialogbox.cpp:41
VA(0x0048fe60, 0x2A)  // vtable + heroWindow ctor, dc 0x817b0
TDialogBox::TDialogBox(unsigned winType)
    : heroWindow(0, 0, 800, 600, winType)
{
}

// E:\gamedcs\dialogbox.cpp:46
VA(0x0048fe90, 0x6B)  // deleting-dtor callee + widget ownership, dc 0x817f8
TDialogBox::~TDialogBox()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

// E:\gamedcs\dialogbox.cpp:52
VA(0x0048ff00, 0x833)  // vtable slot 9 + tiled-dialog CFG, dc 0x8185c
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

// Retail vtable 0x63db68 slot 0.
VA_COMPGEN(0x00490740, 0x21, SCALAR_DELETING_DTOR, CTextDialog)

// E:\gamedcs\dialogbox.cpp:143
// CodeView dc 0x82068: CV_fldattr_t.compgenx marks this destructor
// as implicit. Its retained retail body performs only base/member teardown.
VA_COMPGEN(0x00490770, 0x6B, IMPLICIT_DTOR, CTextDialog)

// E:\gamedcs\dialogbox.cpp:146
VA(0x004907e0, 0x31)  // vtable + pTextWidget zero, dc 0x81e00
CTextDialog::CTextDialog(unsigned winType)
    : TDialogBox(winType)
{
    m_textWidget = 0;
}

// E:\gamedcs\dialogbox.cpp:151
// Before normalization (locals): cText, pFont.
VA(0x00490820, 0x26B)  // vtable slot 10 + CalcDimensions call, dc 0x81e38
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
        text, currentFont->m_name, font::PRIMARY, -1, 1, 0, 8);
    m_widgets.push_back(m_textWidget);
    addWidget(m_textWidget, -1);
    return 1;
}

// E:\gamedcs\dialogbox.cpp:175
// Before normalization (locals): cText, pFont.
VA(0x00490a90, 0x8C)  // vtable slot 12 + font metric calls, dc 0x81f00
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

// E:\gamedcs\dialogbox.cpp:200
// NOT a helper the message handlers share.  The same five statements appear
// at the tail of four free `*Handler(message&)` functions - CampaignBrief,
// GameTypeWindow, CombatResultsWindow and HiScore - and that is a repeated
// source idiom, not a paste of this body: checked 2026-09-06 (polish lane
// 46) and REFUTED on the ABI.  This is `?ExitDialog@CTextDialog@@QAEHAAV
// message@@@Z`, a thiscall member of a class the handlers have no instance
// of (`CTextDialog : TDialogBox`, while `TCampaignBrief` and
// `TGameTypeWindow` are both `heroWindow`), and retail's inventory has no
// free `?ExitDialog@@YI...` row for them to reach - every other ExitDialog
// is a member of some window class too, and all seven take `message*`
// rather than the `message&` the handlers hold.  So there is no callable
// spelling to restore, and GameTypeWindowHandler (100) is right as written.
VA(0x00490b20, 0x2C)  // anchor-global, dc 0x81f98
int CTextDialog::exitDialog(message& msg)
{
    msg.m_id = MESSAGE_WIDGET;
    g_windowManager->m_dialogReturn = msg.m_codeY;
    msg.m_codeY = widget::WIDGET_END_DIALOG;
    msg.m_codeX = widget::WIDGET_END_DIALOG;
    return MESSAGE_DISPATCH_FORWARD;
}

// E:\gamedcs\dialogbox.cpp:211
// Before normalization (locals): cNewText.
VA(0x00490b50, 0x17)  // vtable slot 11 + textWidget::SetText, dc 0x81fb0
void CTextDialog::updateText(const char* newText)
{
    if (m_textWidget)
        m_textWidget->setText(newText);
}

#if 0  // @carcass: retail has no distinct out-of-line bodies

// The text-taking constructor is likewise absent as a distinct retail row.
DC_ONLY(0x81d8c, 0x74)
void CTextDialog::CTextDialog(const char* cText, font* pFont,
                              unsigned winType)
{
    // @stub
}

#endif  // @carcass
