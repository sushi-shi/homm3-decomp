// fly.cpp - E:\gamedcs\fly.cpp (compiland fly.obj)
// 7 functions in link order.
#include <va.h>
#include <math.h>
#include "army.h"
#include "cmbtmgr.h"
#include "csprite.h"
#include "drawing.h"
#include "kb.h"
#include "kbwin.h"
#include "prefs.h"
#include "sample.h"
#include "soundmgr.h"
#include "winmgr.h"

// GameTime, glTimers and gCombatAreaLimits all reach this TU through
// their owners' headers: retail's fly.cpp saw GameTime for free (the DC
// roster files GameTime::NextFrameTime at struct.h:438, and CmbtMgr.h
// already drags struct.h in), and this tree simply parks the class in
// kbwin.h instead.

// E:\gamedcs\fly.cpp:35
// Both private find_flyer_attack_cell overloads are called only by
// ValidFlight in the Dreamcast graph. Retail's 761-byte ValidFlight contains
// their work inline and has no separate carved predecessors after the ten
// terrain.h bitset initializers.
#if 0  // @carcass -- inlined away in retail
DC_ONLY(0xa1360, 0x88)
unsigned char army::findFlyerAttackCell(int start, int target)
{
    // @stub
}
#endif

// E:\gamedcs\fly.cpp:58
#if 0  // @carcass -- inlined away in retail
DC_ONLY(0xa13e8, 0x46)
unsigned char army::findFlyerAttackCell(int target)
{
    // @stub
}
#endif

// E:\gamedcs\fly.cpp:76

// RECONSTRUCTED 2026-08-13. The 761 retail bytes are FOUR copies of the
// same six-step adjacency scan, which is exactly the two private DC
// find_flyer_attack_cell member overloads expanded by /Ob2. Their source
// bodies stay member definitions even though retail retained no out-of-line
// copy; /Ob2 decides the emission, not a file-local replacement helper.

// WHAT THE FOUR COPIES VARY. The pair (start, target) walks
// (this hex, enemy hex), (this second hex, enemy hex),
// (this hex, enemy second hex), (this second hex, enemy second hex), each
// guarded by the wide-stack bit of the stack it belongs to - which is why
// the third and fourth copies sit inside one `enemy->creatureId & 1`
// block and the second and fourth inside their own `creatureId & 1`
// tests. Retail computes the adjacency ROW ADDRESS once per target and
// reuses it across the two copies that share a target (`[ebp-8]`), so the
// two hexes are locals, not re-read members.

// side/slot (+0x10/+0x14) are read here as the CHOSEN ATTACK TARGET's
// (side, slot), not as this stack's own identity: retail indexes
// gpCombatManager->armies with them and then scans the cells adjacent to
// the resulting stack, and the -1 pair is the "no target selected" case
// that falls through to the literal reachability test.

// E:\gamedcs\fly.cpp:35
inline bool army::findFlyerAttackCell(int start, int target) const
{
    for (long dir = 0; dir < 6; dir++) {
        long adjacent = g_combatManager->m_adjacentCells[target][dir];
        long hex = adjacent - start + m_gridIndex;
        if (adjacent >= 0 && hex >= 0 && hex < COMBAT_GRID_CELLS
                && combatManager::getDistance(start, adjacent) <= getSpeed()
                && canFit(hex, 0, 0))
            return 1;
    }
    return 0;
}

// E:\gamedcs\fly.cpp:58
inline bool army::findFlyerAttackCell(int target) const
{
    if (findFlyerAttackCell(m_gridIndex, target))
        return 1;
    if (is(1u << 0)
            && findFlyerAttackCell(getSecondGridIndex(), target))
        return 1;
    return 0;
}

VA(0x004b46c0, 0x2F9)  // dc 0xa1430
unsigned char army::validFlight(int destIndex, unsigned char literalTest) const
{
    if (!combatManager::validHex(destIndex))
        return 0;

    if (literalTest || m_side == -1 || m_slot == -1) {
        if (combatManager::getDistance(m_gridIndex, destIndex) > getSpeed()
                && !g_combatManager->m_creaturePlacement)
            return 0;
        if (!canFit(destIndex, 0, 0))
            return 0;
    } else {
        const army* enemy = &g_combatManager->m_armies[m_side][m_slot];
        long enemyHex = enemy->m_gridIndex;
        if (!findFlyerAttackCell(enemyHex)) {
            if (!enemy->is(1u << 0)
                    || !findFlyerAttackCell(
                    enemy->getSecondGridIndex()))
                return 0;
        }
    }
    return 1;
}

VA(0x004b49c0, 0x76)  // dc 0xa1514
int army::flyTo(int destIndex, unsigned char restoreFacing)
{
    if (combatManager::validHex(destIndex)) {
        int oldFacing = m_facing;
        removeAura();
        removeBinding();
        fly(destIndex);
        addAura();
        if (m_facing != oldFacing && restoreFacing)
            this->turn(1);
        playAnimation(2, 1, 0);
        checkObstacleAttacks(0);
        g_combatManager->testRaiseDoor();
    }
    return 0;
}

// RECONSTRUCTED 2026-08-13. The animation is a two-level loop: `steps`
// straight-line hops of iFlightPixelSpan pixels each, and inside every
// hop one pass over the walk sequence's frames. The pixel geometry is
// the combat grid's own - cells[hex].field_00/field_02 are the hex's
// screen x/y, and the flight distance is the plain euclidean one, which
// is what puts a CRT `sqrt` call (not `fsqrt`; /Op keeps the double in
// memory and the call form) at the head of the body.

// THE PACING PAIR is GameTime::DelayTil followed by the struct.h inline
// GameTime::NextFrameTime; see its definition above for why the
// hand-spelled `glTimers[0] += lag` form is ruled out by the bytes.

// combatManager+0x53b0 is one of the three CCombatOwnedObject slots
// cmbtmgr.h models only as "polymorphic, deleted by Close". This body
// calls Bitmap16Bit::Draw on it, so it is really a Bitmap16Bit* - the
// combat back-buffer. The cast is spelled at the call site rather than
// retyped in the shared header, which is a decision for whoever owns
// the cmbtmgr layout, not for fly.obj.

VA(0x004b4a40, 0x44E)  // dc 0xa1590
int army::fly(int destIndex)
{
    unsigned char turn;
    int sourceX = combatManager::gridX(m_gridIndex);
    int destX = combatManager::gridX(destIndex);

    if (sourceX > destX)
        turn = m_facing == FACING_DEFENDER;
    else if (sourceX < destX)
        turn = m_facing == FACING_ATTACKER;
    else
        turn = 0;

    if (g_combatManager->shouldLowerDoor(this, destIndex)) {
        m_currFrameType = 2;
        m_currFrameIndex = 0;
        g_combatManager->drawFrame(1, 0, 0, 0, 1, 0);
        g_combatManager->lowerDoor();
    }

    if (is(1u << 0) && turn)
        destIndex += offsetToFront(-1);

    setupAnimation();
    if (turn)
        this->turn(1);

    int startX = g_combatManager->m_cells[m_gridIndex].m_refX;
    long startY = g_combatManager->m_cells[m_gridIndex].m_refY;
    long spanX = g_combatManager->m_cells[destIndex].m_refX - startX;
    long spanY = g_combatManager->m_cells[destIndex].m_refY - startY;
    int ttlLoops = static_cast<int>(
        sqrt(static_cast<double>(spanX * spanX + spanY * spanY)));
    if (m_monFrameInfo.m_flightPixelSpan > 0)
        ttlLoops = (ttlLoops + m_monFrameInfo.m_flightPixelSpan / 2)
                    / m_monFrameInfo.m_flightPixelSpan;
    else
        ttlLoops = 1;

    float stepX = static_cast<float>(spanX)
                  / static_cast<float>(ttlLoops);
    float stepY = static_cast<float>(spanY)
                  / static_cast<float>(ttlLoops);
    int loop;

    if (!static_cast<const combatManager*>(g_combatManager)->isQuickCombat()) {
        m_isMoving = 1;
        playSample(PRE_WALK_SAMPLE);
        playAnimation(20, -1, 0);
        playSample(WALK_SAMPLE);
        g_combatManager->removeArmyFromGrid(*this);

        m_currFrameType = 0;
        int numFlapFrames = m_stdIcon->getNumFrames(cs_walk);
        float x = static_cast<float>(startX);
        float y = static_cast<float>(startY);
        const int flyperiod = static_cast<long>(
            static_cast<float>(m_monFrameInfo.m_walkCycleTime)
            * g_combatSpeedFactors[g_unnamed698758.m_combatSpeed]
            / static_cast<float>(numFlapFrames));

        { for (loop = 0; loop < ttlLoops; loop++) {
            for (m_currFrameIndex = 0; m_currFrameIndex < numFlapFrames;
                    m_currFrameIndex++) {
                SLimitData ttlExtent = g_combatManager->m_drawbridgeBounds;
                x += stepX / static_cast<float>(numFlapFrames);
                y += stepY / static_cast<float>(numFlapFrames);
                g_combatManager->m_saveScreenPostGrid->draw(
                           g_combatManager->m_drawbridgeBounds.m_minX,
                           g_combatManager->m_drawbridgeBounds.m_minY,
                           g_combatManager->m_drawbridgeBounds.width(),
                           g_combatManager->m_drawbridgeBounds.height(),
                           g_windowManager->m_screenBitmap->getMap(0, 0),
                           g_combatManager->m_drawbridgeBounds.m_minX,
                           g_combatManager->m_drawbridgeBounds.m_minY,
                           g_windowManager->m_screenBitmap->getWidth(),
                           g_windowManager->m_screenBitmap->getHeight(),
                           g_windowManager->m_screenBitmap->getPitch(),
                           false);
                g_combatManager->m_drawbridgeBounds = g_combatAreaLimits;
                g_combatManager->m_saveBiggestExtent = 1;
                drawToBuffer(static_cast<int>(x), static_cast<int>(y), 0);
                g_combatManager->m_saveBiggestExtent = 0;
                bool scrolled = g_combatManager->scrollTo(
                    g_combatManager->m_drawbridgeBounds, true, true, true);
                ttlExtent.include(g_combatManager->m_drawbridgeBounds);
                GameTime::delayTil(g_timers[0]);
                g_timers[0] = GameTime::nextFrameTime(g_timers[0], flyperiod);
                if (!scrolled)
                    g_combatManager->updateCombatArea(ttlExtent);
            }
        } }
    }

    g_combatManager->placeArmyInGrid(*this, destIndex);
    m_gridIndex = destIndex;

    if (!static_cast<const combatManager*>(g_combatManager)->isQuickCombat()) {
        playSample(POST_WALK_SAMPLE);
        g_soundManager->stopSample(m_armySample[WALK_SAMPLE]->m_memSample.m_memSampleHandle);
        playAnimation(21, -1, 0);
        playAnimation(2, 1, 0);
        m_isMoving = 0;
    }

    cancelSpellType(0);
    return 1;
}

VA(0x004b4e90, 0x76)  // dc 0xa19a0
int army::teleportTo(int destIndex, unsigned char restoreFacing)
{
    if (combatManager::validHex(destIndex)) {
        int oldFacing = m_facing;
        removeAura();
        removeBinding();
        teleport(destIndex);
        addAura();
        if (m_facing != oldFacing && restoreFacing)
            this->turn(1);
        playAnimation(2, 1, 0);
        checkObstacleAttacks(0);
        g_combatManager->testRaiseDoor();
    }
    return 0;
}

VA(0x004b4f10, 0x102)  // dc 0xa1a7c
int army::teleport(int destIndex)
{
    unsigned char turn;
    int sourceX = combatManager::gridX(m_gridIndex);
    int destX = combatManager::gridX(destIndex);

    if (sourceX > destX)
        turn = m_facing == FACING_DEFENDER;
    else if (sourceX < destX)
        turn = m_facing == FACING_ATTACKER;
    else
        turn = 0;

    if (is(1u << 0) && turn)
        destIndex += offsetToFront(-1);

    setupAnimation();
    if (turn)
        this->turn(1);

    if (!g_combatManager->isQuickCombat()) {
        playSample(PRE_WALK_SAMPLE);
        playAnimation(20, -1, 0);
    }

    g_combatManager->removeArmyFromGrid(*this);
    g_combatManager->placeArmyInGrid(*this, destIndex);
    m_gridIndex = destIndex;

    if (!g_combatManager->isQuickCombat()) {
        playSample(POST_WALK_SAMPLE);
        playAnimation(21, -1, 0);
    }

    if (!g_combatManager->isQuickCombat())
        playAnimation(2, 1, 0);

    cancelSpellType(0);
    return 1;
}
