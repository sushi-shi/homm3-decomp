// 10 functions in link order.
#include "va.h"

#include "subwindow.h"

#include "bitmap16.h"
#include "soundmgr.h"
#include "widget.h"
#include "window.h"
#include "winmgr.h"

VA(0x005aa340, 0x41)
DC_ADDRESS(0x158d34, 0x78)
MAC_ADDRESS(0x19b838, 0x90)
TSubWindow::TSubWindow()
    : m_x(0), m_y(0), m_width(0), m_height(0), m_parentWindow(0),
      m_lowId(0xffff), m_highId(0xffff0001), m_background(0)
{
}

VA_COMPGEN(0x005aa390, 0x21, SCALAR_DELETING_DTOR, TSubWindow)

VA(0x005aa3c0, 0x4F)
DC_ADDRESS(0x158dac, 0x66)
MAC_ADDRESS(0x19b8c8, 0x94)
TSubWindow::TSubWindow(int inX, int inY, int w, int h, heroWindow* parentWindow)
    : m_x(inX), m_y(inY), m_width(w), m_height(h), m_parentWindow(parentWindow),
      m_lowId(0xffff), m_highId(0xffff0001), m_background(0)
{
}

VA(0x005aa410, 0x60)
DC_ADDRESS(0x158e14, 0x4a)
MAC_ADDRESS(0x19b95c, 0x94)
TSubWindow::~TSubWindow()
{
    if (m_background)
        delete m_background;
}

VA(0x005aa470, 0x25)
DC_ADDRESS(0x158e60, 0x1c)
MAC_ADDRESS(0x19b9f0, 0x18)
void TSubWindow::initialize(int inX, int inY, int w, int h, heroWindow* parentWindow)
{
    m_x = inX;
    m_y = inY;
    m_width = w;
    m_height = h;
    m_parentWindow = parentWindow;
}

VA(0x005aa4a0, 0x45)
DC_ADDRESS(0x158e7c, 0x3e)
MAC_ADDRESS(0x19ba08, 0x6c)
void TSubWindow::addWidget(widget* newWidget, int newPriority)
{
    newWidget->m_x += static_cast<short>(m_x);
    newWidget->m_y += static_cast<short>(m_y);
    if (newWidget->m_id < m_lowId)
        m_lowId = newWidget->m_id;
    if (newWidget->m_id > m_highId)
        m_highId = newWidget->m_id;
    m_parentWindow->addWidget(newWidget, newPriority);
}

// Original: TSubWindow::RemoveWidget; subwindow.cpp:111
DC_ADDRESS(0x158ebc, 0x12)
void TSubWindow::removeWidget(widget* killWidget)
{
    m_parentWindow->removeWidget(killWidget);
}

// Project-inferred registration of the already-owned widget vector. Ordinary
// banners skip null entries; the combat popup constructors only guard the
// flag change and still pass each entry to addWidget.
void TSubWindow::addWidgetsToMessageStream()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
    }
}

// Project-inferred dynamic insertion. The placement bar guards this operation
// itself, while the control bar retains null entries in its owned vector.
void TSubWindow::addOwnedWidget(widget* newWidget)
{
    m_widgets.push_back(newWidget);
    if (newWidget)
        addWidget(newWidget, -1);
}

// Project-inferred terminal operations. Combat information popups only
// delete; replaceable control/bottom-view strips unlink each item immediately
// before deleting it. Neither operation clears the pointer vector.
void TSubWindow::deleteWidgetObjects()
{
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

VA(0x005aa4f0, 0x63)
DC_ADDRESS(0x158ed0, 0x7c)
MAC_ADDRESS(0x19ba74, 0xa4)
void TSubWindow::draw(unsigned char update, int lowID, int highID)
{
    if (lowID == WINDOW_ALL_WIDGETS_LOW)
        lowID = m_lowId;
    if (highID == WINDOW_ALL_WIDGETS_HIGH)
        highID = m_highId;
    m_parentWindow->drawWindow(0, lowID, highID);
    if (update) {
        // DC 0x158f1e selects a full combat refresh for its combat-mode
        // branch. Complete Mac 0x19ba74..0x19bb18 has only this ordinary
        // parent-relative rectangle path, with no combat-state test.
        g_windowManager->updateScreen(
            m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, m_width, m_height);
    }
}

VA(0x005aa560, 0xA1)
DC_ADDRESS(0x158f4c, 0x58)
MAC_ADDRESS(0x19bb18, 0x9c)
void TSubWindow::saveBackground()
{
    m_background = new Bitmap16Bit(m_width, m_height);
    pollSound();
    m_background->grab(g_windowManager->m_screenBitmap,
        m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y);
    pollSound();
}

VA(0x005aa610, 0x7A)
DC_ADDRESS(0x158fa4, 0xf0)
MAC_ADDRESS(0x19bbb4, 0xf8)
void TSubWindow::restoreBackground()
{
    if (m_background) {
        int drawX = m_x + m_parentWindow->m_x;
        int drawY = m_y + m_parentWindow->m_y;
        Bitmap16Bit* screen = g_windowManager->m_screenBitmap;
        // Mac 0x19bbe4..bc44 expands the Bitmap16Bit-destination Draw overload.
        m_background->draw(0, 0, m_background->getWidth(), m_background->getHeight(),
            screen, drawX, drawY, false);
        g_windowManager->updateScreen(drawX, drawY, m_width + 1, m_height);
        delete m_background;
        m_background = 0;
    }
}
