// 3 functions in link order.
#include "va.h"

#include "hexcell.h"

#include "cmbtmgr.h"
#include "terrain.h"

DC_ADDRESS(0x0d60fc, 0x3a)
VA(0x004e7150, 0x20) MAC_ADDRESS(0x1081cc, 0x2c)
hexcell::hexcell()
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

DC_ADDRESS(0x0d6138, 0x3e)
VA(0x004e7170, 0x3C) MAC_ADDRESS(0x1081f8, 0x40)
army* hexcell::getArmy() const
{
    // Dreamcast hexcell.cpp:39 calls the canonical HexCell.h helper.
    if (hasArmy())
        return &g_combatManager->m_armies[m_armySide][m_armySlot];
    return 0;
}

DC_ADDRESS(0x0d6178, 0x44)
VA(0x004e71b0, 0x4D) MAC_ADDRESS(0x108238, 0x44)
army* hexcell::getDeadArmy(int i) const
{
    if (m_deadArmySide[i] < 0)
        return 0;
    return &g_combatManager->m_armies[m_deadArmySide[i]][m_deadArmySlot[i]];
}
