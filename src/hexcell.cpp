// hexcell.cpp - E:\gamedcs\hexcell.cpp (compiland hexcell.obj)
// 3 functions in link order.
#include "terrain.h"
#include <va.h>
#include "hexcell.h"
#include "cmbtmgr.h"

VA(0x004e7150, 0x20)  // dc 0xd60fc
Hexcell::Hexcell()
{
    int none = -1;
    m_obstacleIndex = none;
    m_attributes = 0;
    m_armySide = none;
    m_armySlot = none;
    m_partOfDouble = none;
    m_bodiesInHex = 0;
    m_mouseShaded = 0;
    m_backgroundOffset = none;
}

VA(0x004e7170, 0x3C)  // dc 0xd6138
Army* Hexcell::getArmy() const
{
    if (m_armySide >= 0)
        return &g_combatManager->m_armies[m_armySide][m_armySlot];
    return 0;
}

VA(0x004e71b0, 0x4D)  // dc 0xd6178
Army* Hexcell::getDeadArmy(int i) const
{
    if (m_deadArmySide[i] < 0)
        return 0;
    return &g_combatManager->m_armies[m_deadArmySide[i]][m_deadArmySlot[i]];
}
