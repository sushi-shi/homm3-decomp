#include "va.h"

#include "army.h"
#include "cmbtmgr.h"
#include "findpath.h"
#include "hexcell.h"
#include "terrain.h"

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
    // Dreamcast path.cpp:36/40 names these two army header helpers.
    if (getSpellTime(72))
        moves = 0;
    int group = getControllingSide();
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
    unsigned char twoHex = static_cast<unsigned char>(is(creatureDoubleWide));
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
    if (is(creatureDoubleWide)) {
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
    // Dreamcast path.cpp:271 calls OffsetToFront(-1) for the double-wide
    // adjustment; retail expands its facing-dependent +/-1 result.
    if (is(creatureDoubleWide)) {
        if (m_facing == 0) {
            if (direction >= 3)
                hex += offsetToFront(-1);
        } else if ((direction >= 0 && direction <= 2) || direction >= 6) {
            hex += offsetToFront(-1);
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

// Original: army::GetBestDirection; path.cpp:480, dc 0x10cd50.
// The blocked-direction mask is searched in a source-proven priority order:
// vertical targets distinguish odd/even staggered rows, then diagonal and
// horizontal targets prefer the nearest heading. No retained Complete RVA or
// fabricated caller is assigned to this ordinary legacy member.
int army::getBestDirection(int currIndex, int destIndex, int currMask)
{
    if (!combatManager::validHex(currIndex)
        || !combatManager::validHex(destIndex))
        return -1;

    int currX = combatManager::gridX(currIndex);
    int currY = combatManager::gridY(currIndex);
    int destX = combatManager::gridX(destIndex);
    int destY = combatManager::gridY(destIndex);
    int up = 0;
    int down = 0;
    int left = 0;
    int right = 0;
    if (destX > currX)
        right = 1;
    else if (destX != currX)
        left = 1;
    if (destY > currY)
        down = 1;
    else if (destY != currY)
        up = 1;

    if (right == left) {
        if (up == 1) {
            if (currY & 1) {
                if (!(currMask & (1 << COMBAT_DIRECTION_5)))
                    return COMBAT_DIRECTION_5;
                else if (!(currMask & (1 << COMBAT_DIRECTION_0)))
                    return COMBAT_DIRECTION_0;
                else if (!(currMask & (1 << COMBAT_DIRECTION_4)))
                    return COMBAT_DIRECTION_4;
                else if (!(currMask & (1 << COMBAT_DIRECTION_1)))
                    return COMBAT_DIRECTION_1;
                else if (!(currMask & (1 << COMBAT_DIRECTION_3)))
                    return COMBAT_DIRECTION_3;
                else if (!(currMask & (1 << COMBAT_DIRECTION_2)))
                    return COMBAT_DIRECTION_2;
                else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_UPPER)))
                    return COMBAT_DIRECTION_WIDE_UPPER;
                else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_LOWER)))
                    return COMBAT_DIRECTION_WIDE_LOWER;
            } else {
                if (!(currMask & (1 << COMBAT_DIRECTION_0)))
                    return COMBAT_DIRECTION_0;
                else if (!(currMask & (1 << COMBAT_DIRECTION_5)))
                    return COMBAT_DIRECTION_5;
                else if (!(currMask & (1 << COMBAT_DIRECTION_1)))
                    return COMBAT_DIRECTION_1;
                else if (!(currMask & (1 << COMBAT_DIRECTION_4)))
                    return COMBAT_DIRECTION_4;
                else if (!(currMask & (1 << COMBAT_DIRECTION_2)))
                    return COMBAT_DIRECTION_2;
                else if (!(currMask & (1 << COMBAT_DIRECTION_3)))
                    return COMBAT_DIRECTION_3;
                else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_UPPER)))
                    return COMBAT_DIRECTION_WIDE_UPPER;
                else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_LOWER)))
                    return COMBAT_DIRECTION_WIDE_LOWER;
            }
        } else {
            if (currY & 1) {
                if (!(currMask & (1 << COMBAT_DIRECTION_3)))
                    return COMBAT_DIRECTION_3;
                else if (!(currMask & (1 << COMBAT_DIRECTION_2)))
                    return COMBAT_DIRECTION_2;
                else if (!(currMask & (1 << COMBAT_DIRECTION_4)))
                    return COMBAT_DIRECTION_4;
                else if (!(currMask & (1 << COMBAT_DIRECTION_1)))
                    return COMBAT_DIRECTION_1;
                else if (!(currMask & (1 << COMBAT_DIRECTION_5)))
                    return COMBAT_DIRECTION_5;
                else if (!(currMask & (1 << COMBAT_DIRECTION_0)))
                    return COMBAT_DIRECTION_0;
                else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_LOWER)))
                    return COMBAT_DIRECTION_WIDE_LOWER;
                else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_UPPER)))
                    return COMBAT_DIRECTION_WIDE_UPPER;
            } else {
                if (!(currMask & (1 << COMBAT_DIRECTION_2)))
                    return COMBAT_DIRECTION_2;
                else if (!(currMask & (1 << COMBAT_DIRECTION_3)))
                    return COMBAT_DIRECTION_3;
                else if (!(currMask & (1 << COMBAT_DIRECTION_1)))
                    return COMBAT_DIRECTION_1;
                else if (!(currMask & (1 << COMBAT_DIRECTION_4)))
                    return COMBAT_DIRECTION_4;
                else if (!(currMask & (1 << COMBAT_DIRECTION_0)))
                    return COMBAT_DIRECTION_0;
                else if (!(currMask & (1 << COMBAT_DIRECTION_5)))
                    return COMBAT_DIRECTION_5;
                else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_LOWER)))
                    return COMBAT_DIRECTION_WIDE_LOWER;
                else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_UPPER)))
                    return COMBAT_DIRECTION_WIDE_UPPER;
            }
        }
    }

    if (left == 1) {
        if (up == 1) {
            if (!(currMask & (1 << COMBAT_DIRECTION_5)))
                return COMBAT_DIRECTION_5;
            else if (!(currMask & (1 << COMBAT_DIRECTION_4)))
                return COMBAT_DIRECTION_4;
            else if (!(currMask & (1 << COMBAT_DIRECTION_0)))
                return COMBAT_DIRECTION_0;
            else if (!(currMask & (1 << COMBAT_DIRECTION_3)))
                return COMBAT_DIRECTION_3;
            else if (!(currMask & (1 << COMBAT_DIRECTION_1)))
                return COMBAT_DIRECTION_1;
            else if (!(currMask & (1 << COMBAT_DIRECTION_2)))
                return COMBAT_DIRECTION_2;
            else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_UPPER)))
                return COMBAT_DIRECTION_WIDE_UPPER;
            else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_LOWER)))
                return COMBAT_DIRECTION_WIDE_LOWER;
        } else if (down == 1) {
            if (!(currMask & (1 << COMBAT_DIRECTION_3)))
                return COMBAT_DIRECTION_3;
            else if (!(currMask & (1 << COMBAT_DIRECTION_4)))
                return COMBAT_DIRECTION_4;
            else if (!(currMask & (1 << COMBAT_DIRECTION_2)))
                return COMBAT_DIRECTION_2;
            else if (!(currMask & (1 << COMBAT_DIRECTION_5)))
                return COMBAT_DIRECTION_5;
            else if (!(currMask & (1 << COMBAT_DIRECTION_1)))
                return COMBAT_DIRECTION_1;
            else if (!(currMask & (1 << COMBAT_DIRECTION_0)))
                return COMBAT_DIRECTION_0;
            else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_LOWER)))
                return COMBAT_DIRECTION_WIDE_LOWER;
            else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_UPPER)))
                return COMBAT_DIRECTION_WIDE_UPPER;
        } else {
            if (!(currMask & (1 << COMBAT_DIRECTION_4)))
                return COMBAT_DIRECTION_4;
            else if (!(currMask & (1 << COMBAT_DIRECTION_5)))
                return COMBAT_DIRECTION_5;
            else if (!(currMask & (1 << COMBAT_DIRECTION_3)))
                return COMBAT_DIRECTION_3;
            else if (!(currMask & (1 << COMBAT_DIRECTION_0)))
                return COMBAT_DIRECTION_0;
            else if (!(currMask & (1 << COMBAT_DIRECTION_2)))
                return COMBAT_DIRECTION_2;
            else if (!(currMask & (1 << COMBAT_DIRECTION_1)))
                return COMBAT_DIRECTION_1;
            else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_LOWER)))
                return COMBAT_DIRECTION_WIDE_LOWER;
            else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_UPPER)))
                return COMBAT_DIRECTION_WIDE_UPPER;
        }
    } else if (right == 1) {
        if (up == 1) {
            if (!(currMask & (1 << COMBAT_DIRECTION_0)))
                return COMBAT_DIRECTION_0;
            else if (!(currMask & (1 << COMBAT_DIRECTION_1)))
                return COMBAT_DIRECTION_1;
            else if (!(currMask & (1 << COMBAT_DIRECTION_5)))
                return COMBAT_DIRECTION_5;
            else if (!(currMask & (1 << COMBAT_DIRECTION_2)))
                return COMBAT_DIRECTION_2;
            else if (!(currMask & (1 << COMBAT_DIRECTION_4)))
                return COMBAT_DIRECTION_4;
            else if (!(currMask & (1 << COMBAT_DIRECTION_3)))
                return COMBAT_DIRECTION_3;
            else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_UPPER)))
                return COMBAT_DIRECTION_WIDE_UPPER;
            else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_LOWER)))
                return COMBAT_DIRECTION_WIDE_LOWER;
        } else if (down == 1) {
            if (!(currMask & (1 << COMBAT_DIRECTION_2)))
                return COMBAT_DIRECTION_2;
            else if (!(currMask & (1 << COMBAT_DIRECTION_1)))
                return COMBAT_DIRECTION_1;
            else if (!(currMask & (1 << COMBAT_DIRECTION_3)))
                return COMBAT_DIRECTION_3;
            else if (!(currMask & (1 << COMBAT_DIRECTION_0)))
                return COMBAT_DIRECTION_0;
            else if (!(currMask & (1 << COMBAT_DIRECTION_5)))
                return COMBAT_DIRECTION_5;
            else if (!(currMask & (1 << COMBAT_DIRECTION_4)))
                return COMBAT_DIRECTION_4;
            else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_LOWER)))
                return COMBAT_DIRECTION_WIDE_LOWER;
            else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_UPPER)))
                return COMBAT_DIRECTION_WIDE_UPPER;
        } else {
            if (!(currMask & (1 << COMBAT_DIRECTION_1)))
                return COMBAT_DIRECTION_1;
            else if (!(currMask & (1 << COMBAT_DIRECTION_0)))
                return COMBAT_DIRECTION_0;
            else if (!(currMask & (1 << COMBAT_DIRECTION_2)))
                return COMBAT_DIRECTION_2;
            else if (!(currMask & (1 << COMBAT_DIRECTION_5)))
                return COMBAT_DIRECTION_5;
            else if (!(currMask & (1 << COMBAT_DIRECTION_3)))
                return COMBAT_DIRECTION_3;
            else if (!(currMask & (1 << COMBAT_DIRECTION_4)))
                return COMBAT_DIRECTION_4;
            else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_LOWER)))
                return COMBAT_DIRECTION_WIDE_LOWER;
            else if (!(currMask & (1 << COMBAT_DIRECTION_WIDE_UPPER)))
                return COMBAT_DIRECTION_WIDE_UPPER;
        }
    }
    return -1;
}
