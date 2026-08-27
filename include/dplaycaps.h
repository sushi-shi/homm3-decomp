#ifndef HOMM3_DPLAYCAPS_H
#define HOMM3_DPLAYCAPS_H

#include "va.h"

// DirectPlay 6 capability record. Dreamcast publishes the complete layout;
// retail's CDPlay::GetCaps memset fixes the same 0x28-byte extent, and the
// multiplayer session browser reads dwTimeout at +0x24.
struct DPCAPS {
    unsigned long dwSize;             // +0x00
    unsigned long dwFlags;            // +0x04
    unsigned long dwMaxBufferSize;    // +0x08
    unsigned long dwMaxQueueSize;     // +0x0c
    unsigned long dwMaxPlayers;       // +0x10
    unsigned long dwHundredBaud;      // +0x14
    unsigned long dwLatency;          // +0x18
    unsigned long dwMaxLocalPlayers;  // +0x1c
    unsigned long dwHeaderLength;     // +0x20
    unsigned long dwTimeout;          // +0x24
};
SIZE(DPCAPS, 0x28);

#endif  /* HOMM3_DPLAYCAPS_H */
