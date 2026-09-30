// 3 functions in link order.
#include "va.h"

#include "hexcell.h"

#include "cmbtmgr.h"
#include "terrain.h"

VA(0x004e7150, 0x20)
DC_ADDRESS(0x0d60fc, 0x3a)
MAC_ADDRESS(0x1081cc, 0x2c)
hexcell::hexcell()
{
    int none = -1;
    m_obstacleIndex = none;
    m_attributes = 0;
    resetArmy();
    m_bodiesInHex = 0;
    m_mouseShaded = 0;
    m_backgroundOffset = none;
}

VA(0x004e7170, 0x3C)
DC_ADDRESS(0x0d6138, 0x3e)
MAC_ADDRESS(0x1081f8, 0x40)
army* hexcell::getArmy() const
{
    // Dreamcast hexcell.cpp:39 calls the canonical HexCell.h helper.
    if (hasArmy())
        return &g_combatManager->m_armies[m_armySide][m_armySlot];
    return 0;
}

VA(0x004e71b0, 0x4D)
DC_ADDRESS(0x0d6178, 0x44)
MAC_ADDRESS(0x108238, 0x44)
army* hexcell::getDeadArmy(int i) const
{
    if (m_deadArmySide[i] < 0)
        return 0;
    return &g_combatManager->m_armies[m_deadArmySide[i]][m_deadArmySlot[i]];
}

// Project-inferred operations shared by army initialization, movement and
// death. These names/placement do not claim retained native helper symbols.
void hexcell::setArmy(int side, int slot, int partOfDouble)
{
    m_armySide = static_cast<signed char>(side);
    m_armySlot = static_cast<signed char>(slot);
    m_partOfDouble = static_cast<signed char>(partOfDouble);
}

void hexcell::clearArmy()
{
    m_armySide = -1;
    m_armySlot = -1;
}

void hexcell::resetArmy()
{
    clearArmy();
    m_partOfDouble = -1;
}

void hexcell::recordArmyBody()
{
    if (hasArmy()) {
        m_deadArmySide[m_bodiesInHex] = m_armySide;
        m_deadArmySlot[m_bodiesInHex] = m_armySlot;
        m_deadPartOfDouble[m_bodiesInHex] = m_partOfDouble;
        ++m_bodiesInHex;
    }
}
