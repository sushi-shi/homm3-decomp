#ifndef HOMM3_ADVENTUREMAPWINDOW_H
#define HOMM3_ADVENTUREMAPWINDOW_H

#include "va.h"

#include "advmgr.h"

// E:\gamedcs\AdventureMapWindow.h:238
DC_ADDRESS(0x0bd0a0, 0x14)
inline void TAdventureMapWindow::setBackgroundAnimation(bool enable)
{
    m_animateInBackground = enable;
}

class message;

void sendChat(const char* chat, int toWho);
// Retail .bss 0x69954c, the network-session latch (remote.h owns the
// canonical declaration; same reason as above).
extern int g_remoteOn;

#endif  /* HOMM3_ADVENTUREMAPWINDOW_H */
