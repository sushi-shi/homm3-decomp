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
    obstacleIndex = none;
    attributes = 0;
    armyGroup = none;
    armyIndex = none;
    partOfDouble = none;
    iBodiesInHex = 0;
    mouse_shaded = 0;
    background_offset = none;
}

VA(0x004e7170, 0x3C)
DC_ADDRESS(0x0d6138, 0x3e)
MAC_ADDRESS(0x1081f8, 0x40)
army* hexcell::get_army() const
{
    // Dreamcast hexcell.cpp:39 calls the canonical HexCell.h helper.
    if (HasArmy())
        return &g_combatManager->Armies[armyGroup][armyIndex];
    return 0;
}

VA(0x004e71b0, 0x4D)
DC_ADDRESS(0x0d6178, 0x44)
MAC_ADDRESS(0x108238, 0x44)
army* hexcell::get_dead_army(int i) const
{
    if (deadArmyGroup[i] < 0)
        return 0;
    return &g_combatManager->Armies[deadArmyGroup[i]][deadArmyIndex[i]];
}
