// path.cpp - E:\gamedcs\path.cpp (compiland path.obj)
// 9 functions in link order; retail drops GetBestDirection (no carve
// row fits between OppositeDirection and the cinit cluster - the
// combat move choice lives in the ai_* units).
// The shared bounds-check exits are expansions of combatManager::ValidHex,
// attested at Dreamcast path.cpp:31, :52 and :279. Calling the canonical
// inline reproduces retail's merged failure block without source gotos.
// The old labelled guards matched too, but did not establish original gotos.
// Negative control: nesting the successful body under if (validHex(...))
// sinks the failure block and lowers FindPath to 78.8679% and
// GetAdjacentCellIndexNoArmy to 74.1667%; the early-return helper form is exact.
// Second family lever, byte-proven here and worth trying anywhere
// `creatureId & 1` appears (cmbtmgr, ai_tactical, army): retail
// computes the two-hex test as a BYTE-typed value and reuses it -
// `unsigned char twoHex = creatureId & 1;` then `twoHex ? a : b`.
// Writing `(creatureId & 1) ? a : b` twice makes our CL CSE it as a
// DWORD (`and eax,1` / `and esi,0xffffff40`) where retail works in
// AL/CL (`and al,1` / `and cl,0x40`); that one change took
// GetAttackMask 70.44% -> exact with nothing else touched.
#include "terrain.h"
#include <va.h>
#include "army.h"
#include "hexcell.h"
#include "cmbtmgr.h"
#include "findpath.h"

VA(0x005239d0, 0x96)  // dc 0x10c918
int army::findPath(int fpTargetCellIndex, int maxMoves, unsigned char moveUnlimited, unsigned char literalTarget)
{
    if (!combatManager::validHex(fpTargetCellIndex))
        return 0;
    int moves;
    if (!g_combatManager->m_creaturePlacement && !moveUnlimited)
        moves = getSpeed();
    else
        moves = 99;
    if (m_spellInfluence[72])
        moves = 0;
    int group;
    if (m_spellInfluence[60])
        group = 1 - m_combatSide;
    else
        group = m_combatSide;
    return g_searchArray->findCombatPath(this, group, fpTargetCellIndex,
        g_combatManager->m_creaturePlacement, moves, -1);
}

VA(0x00523a70, 0xA8)  // dc 0x10c9a4
unsigned char army::validPath(int destIndex, unsigned char literalTest)
{
    if (!combatManager::validHex(destIndex))
        return 0;
    if (!findPath(destIndex, getSpeed(), 0, literalTest))
        return 0;
    m_pathTarget = destIndex;
    return 1;
}

VA(0x00523b20, 0x89)  // dc 0x10c9ec
unsigned army::getAttackMask(int currIndex, int criteria, int literalTargetIndex) const
{
    int testCellIndex;
    unsigned char twoHex = static_cast<unsigned char>(m_monInfo.m_attributes & 1);
    unsigned bit = 1;
    unsigned mask = twoHex ? 0 : 0xc0;
    int dirs = twoHex ? 8 : 6;
    for (int i = 0; i < dirs; i++) {
        if (!validAttack(currIndex, i, criteria, literalTargetIndex, &testCellIndex))
            mask |= bit;
        bit <<= 1;
    }
    return mask;
}

VA(0x00523bb0, 0x1DF)  // dc 0x10ca6c
int army::validAttack(int currIndex, int direction, int criteria, int literalIndex, int* testCellIndex) const
{
    if (!combatManager::validHex(currIndex))
        return 0;
    int other = currIndex;
    if (m_monInfo.m_attributes & 1) {
        if (direction == COMBAT_DIRECTION_WIDE_UPPER) {
            *testCellIndex = getAdjacentCellIndex(currIndex, m_facing ? 0 : 5);
        } else if (direction == COMBAT_DIRECTION_WIDE_LOWER) {
            *testCellIndex = getAdjacentCellIndex(currIndex, m_facing ? 2 : 3);
        } else {
            switch (m_facing) {
                case FACING_ATTACKER:
                    if (direction >= 3)
                        other = getAdjacentCellIndex(currIndex, 4);
                    break;
                case FACING_DEFENDER:
                    if (direction <= 2)
                        other = getAdjacentCellIndex(currIndex, 1);
                    break;
            }
            if (other == -1)
                return 0;
            *testCellIndex = getAdjacentCellIndex(other, direction);
        }
    } else {
        *testCellIndex = getAdjacentCellIndex(currIndex, direction);
    }
    if (!combatManager::validHex(*testCellIndex))
        return 0;
    if (literalIndex != -1 && *testCellIndex != literalIndex)
        return 0;
    hexcell* hc = &g_combatManager->m_cells[*testCellIndex];
    switch (criteria) {
        case ATTACK_CRITERIA_SELF:
            if (hc->m_armySide == m_side && hc->m_armySlot == m_slot)
                return 1;
            break;
        case ATTACK_CRITERIA_ENEMY:
            if (hc->hasArmy() && isEnemy(hc->getArmy()))
                return 1;
            break;
        case ATTACK_CRITERIA_OCCUPIED:
            if (hc->hasArmy())
                return 1;
            break;
    }
    return 0;
}

VA(0x00523d90, 0x57)  // dc 0x10cbf8
int army::getAdjacentCellIndex(int currIndex, int direction) const
{
    if (!combatManager::validHex(currIndex))
        return -1;
    if (direction == COMBAT_DIRECTION_WIDE_UPPER)
        direction = (m_facing == 1) ? 5 : 0;
    else if (direction == COMBAT_DIRECTION_WIDE_LOWER)
        direction = (m_facing == 1) ? 3 : 2;
    return g_combatManager->m_adjacentCells[currIndex][direction];
}

VA(0x00523df0, 0x86)  // dc 0x10cc80
long army::getAdjacentHex(long hex, long direction) const
{
    if (m_monInfo.m_attributes & 1) {
        if (m_facing == 0) {
            if (direction >= 3)
                hex--;
        } else if ((direction >= 0 && direction <= 2) || direction >= 6) {
            hex++;
        }
    }
    return getAdjacentCellIndex(hex, direction);
}

VA(0x00523e80, 0x3B)  // dc 0x10ccdc
int getAdjacentCellIndexNoArmy(int currIndex, int direction)
{
    if (!combatManager::validHex(currIndex))
        return -1;
    if (direction == COMBAT_DIRECTION_WIDE_UPPER)
        direction = 5;
    else if (direction == COMBAT_DIRECTION_WIDE_LOWER)
        direction = 3;
    return g_combatManager->m_adjacentCells[currIndex][direction];
}

VA(0x00523ec0, 0x1F)  // dc 0x10cd28
int oppositeDirection(int direction)
{
    if (direction < 6)
        return (direction + 3) % 6;
    return direction == COMBAT_DIRECTION_WIDE_UPPER
               ? COMBAT_DIRECTION_WIDE_LOWER : COMBAT_DIRECTION_WIDE_UPPER;
}

#if 0  // @carcass

// E:\gamedcs\path.cpp:480
DC_ONLY(0x10cd50, 0x4A8)
int army::getBestDirection(int currIndex, int destIndex, int currMask)
{
    // @stub
}

#endif  // @carcass
