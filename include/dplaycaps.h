#ifndef HOMM3_DPLAYCAPS_H
#define HOMM3_DPLAYCAPS_H

#include "va.h"

// DirectPlay 6 capability record. Dreamcast publishes the complete layout;
// retail's CDPlay::GetCaps memset fixes the same 0x28-byte extent, and the
// multiplayer session browser reads dwTimeout at +0x24.
struct DPCAPS {
    unsigned long m_size;             // +0x00
    unsigned long m_flags;            // +0x04
    unsigned long m_maxBufferSize;    // +0x08
    unsigned long m_maxQueueSize;     // +0x0c
    unsigned long m_maxPlayers;       // +0x10
    unsigned long m_hundredBaud;      // +0x14
    unsigned long m_latency;          // +0x18
    unsigned long m_maxLocalPlayers;  // +0x1c
    unsigned long m_headerLength;     // +0x20
    unsigned long m_timeout;          // +0x24
};
SIZE(DPCAPS, 0x28);

#endif  /* HOMM3_DPLAYCAPS_H */
