// 1 functions in link order.
#include "va.h"

#include <string.h>

#include "basemgr.h"

// #include "basemgr.h"
VA(0x0044d530, 0x45)
DC_ADDRESS(0x050a28, 0x54)
MAC_ADDRESS(0x05bc04, 0x5c)
baseManager::baseManager()
    : m_nextManager(0),
      m_prevManager(0)
{
    m_priority = -1;
    m_id = -1;
    m_status = 0;
    strcpy(m_mgrName, "Unknown");
}
