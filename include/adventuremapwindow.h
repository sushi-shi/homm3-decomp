// adventuremapwindow.h - adventuremapwindow.cpp (compiland adventuremapwindow.obj)
#ifndef HOMM3_ADVENTUREMAPWINDOW_H
#define HOMM3_ADVENTUREMAPWINDOW_H

#include "advmgr.h"

// E:\gamedcs\AdventureMapWindow.h:238, dc 0xbd0a0.
inline void TAdventureMapWindow::setBackgroundAnimation(unsigned char enable)
{
    m_animateInBackground = enable;
}

class message;

void sendChat(const char* chat, int toWho);
// Retail .bss 0x69954c, the network-session latch (remote.h owns the
// canonical declaration; same reason as above).
extern int g_networkActive69954c;

#endif  /* HOMM3_ADVENTUREMAPWINDOW_H */
