// 10 functions in link order.
#include "subwindow.h"

#include "bitmap16.h"
#include "soundmgr.h"
#include "va.h"
#include "widget.h"
#include "window.h"
#include "winmgr.h"

VA(0x005aa340, 0x41)  // dc 0x158d34
TSubWindow::TSubWindow()
    : m_x(0), m_y(0), m_width(0), m_height(0), m_parentWindow(0),
      m_lowId(0xffff), m_highId(0xffff0001), m_background(0)
{
}

VA_COMPGEN(0x005aa390, 0x21, SCALAR_DELETING_DTOR, TSubWindow)

VA(0x005aa3c0, 0x4F)  // dc 0x158dac
TSubWindow::TSubWindow(int inX, int inY, int w, int h, heroWindow* parentWindow)
    : m_x(inX), m_y(inY), m_width(w), m_height(h), m_parentWindow(parentWindow),
      m_lowId(0xffff), m_highId(0xffff0001), m_background(0)
{
}

VA(0x005aa410, 0x60)  // dc 0x158e14
TSubWindow::~TSubWindow()
{
    if (m_background)
        delete m_background;
}

VA(0x005aa470, 0x25)  // dc 0x158e60
void TSubWindow::initialize(int inX, int inY, int w, int h, heroWindow* parentWindow)
{
    m_x = inX;
    m_y = inY;
    m_width = w;
    m_height = h;
    m_parentWindow = parentWindow;
}

VA(0x005aa4a0, 0x45)  // dc 0x158e7c
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

// Original: TSubWindow::RemoveWidget; subwindow.cpp:111, dc 0x158ebc.
void TSubWindow::removeWidget(widget* killWidget)
{
    m_parentWindow->removeWidget(killWidget);
}

VA(0x005aa4f0, 0x63)  // dc 0x158ed0
void TSubWindow::draw(unsigned char update, int lowID, int highID)
{
    if (lowID == WINDOW_ALL_WIDGETS_LOW)
        lowID = m_lowId;
    if (highID == WINDOW_ALL_WIDGETS_HIGH)
        highID = m_highId;
    m_parentWindow->drawWindow(0, lowID, highID);
    if (update) {
        g_windowManager->updateScreen(
            m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y, m_width, m_height);
    }
}

VA(0x005aa560, 0xA1)  // dc 0x158f4c
void TSubWindow::saveBackground()
{
    m_background = new Bitmap16Bit(m_width, m_height);
    pollSound();
    Bitmap16Bit* screen = g_windowManager->m_screenBitmap;
    m_background->grab(screen->getMap(0, 0),
        m_x + m_parentWindow->m_x, m_y + m_parentWindow->m_y,
        screen->getWidth(), screen->getHeight(), screen->getPitch());
    pollSound();
}

VA(0x005aa610, 0x7A)  // dc 0x158fa4
void TSubWindow::restoreBackground()
{
    if (m_background) {
        int drawX = m_x + m_parentWindow->m_x;
        int drawY = m_y + m_parentWindow->m_y;
        Bitmap16Bit* screen = g_windowManager->m_screenBitmap;
        m_background->draw(0, 0, m_background->getWidth(), m_background->getHeight(),
            screen->getMap(0, 0), drawX, drawY,
            screen->getWidth(), screen->getHeight(), screen->getPitch(), false);
        g_windowManager->updateScreen(drawX, drawY, m_width + 1, m_height);
        delete m_background;
        m_background = 0;
    }
}
