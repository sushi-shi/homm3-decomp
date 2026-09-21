// 1 functions in link order.
#include <string.h>

#include "basemgr.h"

#include "va.h"
// #include "basemgr.h"

VA(0x0044d530, 0x45)  // dc 0x50a28
baseManager::baseManager()
    : m_nextManager(0),
      m_prevManager(0)
{
    m_priority = -1;
    m_id = -1;
    m_status = 0;
    strcpy(m_mgrName, "Unknown");
}
