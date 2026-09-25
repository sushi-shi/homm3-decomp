// Both DC command routines below retain army::GetName as the source boundary,
// while the original CreatureType.h body supplies its nested GetArmyName
// expansion.  Omitting that header-inline body loses 16 retail CFG blocks.
#include "va.h"

#include <stdio.h>

#include "command.h"

#include "cmbtmgr.h"
#include "combatcontrolsubwindow.h"
#include "combatoptionswindow.h"
#include "combatresultswindow.h"
#include "combatwindow.h"
#include "creaturetype.h"
#include "drawing.h"
#include "findpath.h"
#include "game.h"
#include "hero.h"
#include "herospec.h"
#include "inputmgr.h"
#include "kb.h"
#include "kbwin.h"
#include "misc.h"
#include "mousemgr.h"
#include "prefs.h"
#include "remote.h"
#include "soundmgr.h"
#include "textresource.h"
#include "widget.h"
#include "winmgr.h"

// Retail table initializers, in the layouts used by their named consumers.
DATA(0x0069773c) int g_combatControlNetPos[2];

// The remaining pending-action rungs, byte-proven by ProcessNextAction's
// twelve-entry switch. Kept as command-local integral constants instead of
// enlarging EAIOrder: this TU has a measured VC6 enum-population wall.
static const int g_combatActionCastHeroSpell = 1;
static const int g_combatActionMove = 2;
static const int g_combatActionDefend = 3;
static const int g_combatActionRetreat = 4;
static const int g_combatActionSurrender = 5;
static const int g_combatActionWait = 8;
static const int g_combatActionAttackWall = 9;
static const int g_combatActionCastCreatureSpell = 10;
static const int g_combatActionFirstAid = 11;

// E:\gamedcs\command.cpp:63
// Dreamcast CodeView names this private nullary member and its two static
// TWallTargetId arrays. Retail fixes the Complete-build fourth tower target,
// the x86 unsigned-char return ABI, and every branch below. The first scan
// rejects a siege with no live target. Trained Ballistics then aims at the
// keep when the action is automatic; otherwise a basic catapult chooses a
// random weakest wall, while the remaining cases prefer surviving towers.
// Every successful arm writes the catapult order and its target combat hex.
// Residual (99.8804%): all 59 blocks, 42 branches, five returns and four
// calls are exact. The only six unpaired masked slots are one EAX/ECX scratch
// swap while loading the keep's wall section and strength. why-reg finds the
// callee-saved bindings and IL order identical and classifies this as a C1
// caller-saved choice. Naming the wall as int or TWallSection, using the
// canonical getWallStrength helper, nesting the two tests, and swapping the
// count/skill declaration order are byte-flat; moving the wall lifetime
// earlier costs 36 rows, while naming `skill == 0` costs 37. Keep the direct
// DC-shaped access rather than forcing a register with synthetic state.
// Mac code0+0x81d04 retains validWallTarget(WALL_TARGET_3) at the keep check.
// Keep that source call even though VC6 emits a longer boolean test here.
// A chosen-target result guards the fallback wall/tower selection and
// removes the shared-order goto at 99.8804%. Bool, unsigned-char and int
// forms are neutral, as are two single-pass selection scopes. Copying the
// final order stores and return into the keep arm scores 95.9569%. Preserve
// the named calls and their conditional random draw.
VA(0x00473c00, 0x29F)  // anchor-callee: Main's only automate callee w/ Random discriminator + order-map, dc 0x6af98
unsigned char combatManager::automateCatapult()
{
    DATA(0x0063d54c) static const TWallTargetId walls[4] = {
        WALL_TARGET_1, WALL_TARGET_2, WALL_TARGET_4, WALL_TARGET_5
    };

    army* currentArmy = getCurrentArmy();
    if (currentArmy->m_creatureType != CREATURE_CATAPULT)
        return 0;

    if (m_fortificationLevel == COMBAT_FORTIFICATION_NONE) {
        m_nextAction = AI_ORDER_NONE;
        return 1;
    }

    TWallTargetId target;
    for (target = WALL_TARGET_0; target < WALL_TARGET_COUNT;
            target = TWallTargetId(target + 1)) {
        if (validWallTarget(target))
            break;
    }
    if (target == WALL_TARGET_COUNT) {
        m_nextAction = AI_ORDER_NONE;
        return 1;
    }

    long count;
    long skill = currentArmy->getController()->getSecondarySkill(
        eSecSkillSiegeBallistics);
    bool targetChosen = 0;
    if (static_cast<const combatManager*>(this)->isQuickCombat()
            || isComputerAction(getCurrentArmy())) {
        if (skill > 0 && validWallTarget(WALL_TARGET_3)) {
            target = WALL_TARGET_3;
            targetChosen = 1;
        }
    } else if (skill > 0) {
        return 0;
    }

    if (!targetChosen) {
        count = 0;
        { for (long i = 0; i < 4; i++) {
                if (getWallStrength(walls[i]) > 0)
                    count++;
            }
        }

        if (count > 0 && (skill == 0 || count == sizeof(walls) / sizeof(walls[0]))) {
            long weakest = 100;
            count = 0;
            { for (long i = 0; i < 4; i++) {
                    long strength = getWallStrength(walls[i]);
                    if (strength <= 0 || strength > weakest)
                        continue;
                    if (strength < weakest)
                        count = 0;
                    count++;
                    weakest = strength;
                }
            }

            // Dreamcast and Mac retain sRandom here. Complete binds its
            // identical body to the shared random implementation.
            long choice = sRandom(1, count);
            long index = 0;
            for (; index < 4; index++) {
                long strength = getWallStrength(walls[index]);
                if (strength == weakest && --choice == 0)
                    break;
            }
            target = walls[index];
        } else {
            DATA(0x00670198) static TWallTargetId towers[4] = {
                WALL_TARGET_3, WALL_TARGET_7, WALL_TARGET_0, WALL_TARGET_6
            };

            long index;
            for (index = 0; index < 4; index++) {
                if (validWallTarget(towers[index]))
                    break;
            }
            if (index < 4) {
                target = towers[index];
            } else {
                for (target = WALL_TARGET_0; target < WALL_TARGET_COUNT;
                        target = TWallTargetId(target + 1)) {
                    if (validWallTarget(target))
                        break;
                }
            }
        }
    }

    m_nextAction = 9;
    m_nextActionGridIndex = s_wallTargets[target].m_targetHex;
    m_nextActionExtra = -1;
    return 1;
}

// E:\gamedcs\command.cpp:193. The DC signature and sole local identify
// the current-stack scan; retail fixes the target filters and command tuple.
// The final two stores follow DC's separate statement groups: select and store
// the target grid first, then clear field_40. Retail's Complete-only pointer
// overload of is_computer_action is kept because its call relocation proves it.
VA(0x00473ea0, 0x196)  // anchor-callee: Main's other automate callee (no-Random sibling) + order-map, dc 0x6b12c
unsigned char combatManager::automateFirstAidTent()
{
    const army* currentArmy = getCurrentArmy();
    int side = currentArmy->getControllingSide();

    if (currentArmy->m_creatureType != CREATURE_FIRST_AID_TENT)
        return 0;

    int bestIndex = -1;
    int bestDamage = 0;
    for (int i = 0; i < m_numArmies[side]; ++i) {
        army* target = &m_armies[side][i];
        if (target->is(creatureSiegeWeapon | creatureImmobilized))
            continue;
        if (target->m_topCreatureDamage == 0)
            continue;

        if (target->m_creatureType == CREATURE_WIGHT
                || target->m_creatureType == CREATURE_WIGHT + 1) {
            if (target->m_topCreatureDamage < bestDamage)
                continue;
        } else if (target->m_topCreatureDamage < bestDamage)
            continue;

        bestIndex = i;
        bestDamage = target->m_topCreatureDamage;
    }

    if (bestIndex < 0) {
        m_nextAction = 3;
        return 1;
    }

    if (!static_cast<const combatManager*>(this)->isQuickCombat()
            && !isComputerAction(getCurrentArmy())) {
        if (currentArmy->getController()->getSecondarySkill(
                eSecSkillFirstAid) > 0)
            return 0;
    }

    m_nextAction = 11;
    m_nextActionGridIndex = m_armies[side][bestIndex].m_gridIndex;
    m_nextActionExtra = -1;
    return 1;
}

VA(0x00474040, 0x8C)  // dc 0x6b268
void combatManager::doAnimations()
{
    if (static_cast<const combatManager*>(this)->isQuickCombat())
        return;

    if (GameTime::isPast(g_timers[0])) {
        pollSound();
        long interval = static_cast<long>(
            g_combatSpeedFactors[g_config.m_combatSpeed] * 100.0f);
        g_timers[0] =
            GameTime::nextFrameTime(g_timers[0], interval);
    }

    if (GameTime::isPast(g_timers[8])
            && !g_processingCombatAction) {
        g_processingCombatAction = 1;
        cycleCombatScreen();
        g_processingCombatAction = 0;
    }
}

// Mac 0:0x82384 retains this helper immediately before main and calls it
// from main, processCombatMsg, and resetRound. Dreamcast's older source has
// the equivalent statements at those call sites.
void combatManager::finishCreaturePlacement()
{
    if (!m_creaturePlacement)
        return;
    m_creaturePlacement = 0;
    m_actingSide = 1;
    m_actingSlot = 0;
    m_turnNumber = 0;
    if (!static_cast<const combatManager*>(this)->isQuickCombat())
        m_combatWindow->endPlacementPhase();
    resetRound();
}

// E:\gamedcs\command.cpp:291
VA(0x004740d0, 0x5AB)  // anchor-vtable combatManager slot02 + dispatcher: calls automate_catapult/first_aid + ProcessCombatMsg/CheckWin/ResetRound, dc 0x6b318
int combatManager::main(message& msg)
{
    int result = 1;
    doAnimations();

    unsigned char automaticTurn = 0;
    if (!static_cast<const combatManager*>(this)->isQuickCombat()
            && m_thisNetHasControl && (m_autoCombatOn || g_goSolo)) {
        if (static_cast<const combatManager*>(this)->isQuickCombat()
                || isComputerAction(getCurrentArmy())) {
            while (msg.m_id != MESSAGE_KEY_DOWN
                    && msg.m_id != MESSAGE_LEFT_BUTTON_DOWN
                    && msg.m_id != MESSAGE_LEFT_BUTTON_UP
                    && msg.m_id != MESSAGE_RIGHT_BUTTON_DOWN
                    && msg.m_id != MESSAGE_RIGHT_BUTTON_UP
                    && msg.m_id != MESSAGE_NONE)
                msg = g_inputManager->getEvent();

            if (msg.m_id != MESSAGE_NONE) {
                g_inputManager->flush();
                m_autoCombatOn = 0;
                getControl();
            }
        }
    }

    if (!m_creaturePlacement && m_nextAction == 0 && m_thisNetHasControl) {
        automaticTurn = automateFirstAidTent();

        automaticTurn |= automateTower() | automateCatapult();
    }

    if (checkWin(&msg))
        return MESSAGE_DISPATCH_FORWARD;

    if (!static_cast<const combatManager*>(this)->isQuickCombat()
            && !automaticTurn) {
        // The retail command header retains the Dreamcast two-argument
        // prototype even though remote.cpp's Complete wrapper ignores the
        // compression out-parameter.
        CNetMsg* getRemoteData(unsigned char removeFromQueue,
                               unsigned char* wasCompressed);
        void receiveChat(char* chat, int fromWho);

        CNetMsg* netMsg = getRemoteData(1, 0);
        if (netMsg) {
            CMessageKill killMsg(netMsg);
            switch (netMsg->m_subType) {
            case RS_COMBAT_MAIN: {
                CCombatMainMsg* combatMsg =
                    static_cast<CCombatMainMsg*>(netMsg);
                m_nextAction = combatMsg->m_nextAction;
                m_nextActionExtra = combatMsg->m_nextActionExtra;
                m_nextActionGridIndex = combatMsg->m_nextActionGridIndex;
                m_nextActionGridIndex2 = combatMsg->m_nextActionGridIndex2;
                g_logFile.log(
                    DATA_COMPGEN(0x00670218, receivedCombatActionLog,
                                 "Received action [%d]--> [%d,%d,%d]"),
                    m_nextAction, m_nextActionExtra, m_nextActionGridIndex, m_nextActionGridIndex2);
                sRand(combatMsg->m_seed);
                goto process_action;
            }

            case RS_CHAT_MSG:
                receiveChat(static_cast<CChatMsg*>(netMsg)->m_text,
                            netMsg->m_from);
                break;

            case RS_COMBAT_END_PLACEMENT:
                finishCreaturePlacement();
                nextArmy(1);
                break;

            case RS_PLAYER_DROPPED:
                if (handleCombatPlayerDrop(
                        static_cast<CPlayerDropMsg*>(netMsg)->m_dpid, &msg)) {
                    killMsg.setMessage(0);
                    m_netMsgHandlerPause->setAbortPopupMsg(netMsg);
                    return MESSAGE_DISPATCH_FORWARD;
                }
                break;
            }
        }

        if (!m_thisNetHasControl)
            return processCombatMsg(msg);
    }

    if (m_thisNetHasControl) {
        army* currentArmy = getCurrentArmy();
        if (currentArmy->m_spellInfluence[59]) {
            currentArmy->goBerserk();
            if (checkWin(&msg))
                return MESSAGE_DISPATCH_FORWARD;
        }
    }

process_action:
    if (m_nextAction == 0) {
        if (isComputerAction()) {
            checkGetAIMove();
        } else {
            result = processCombatMsg(msg);
        }
    }

    if (m_nextAction != 0)
        result = processNextAction(msg, automaticTurn);

    return result;
}

// E:\gamedcs\command.cpp:475
// Build the twelve legal approach records around the selected target hex.
// Dreamcast proves the local names and the two header-inline army helpers;
// retail fixes the Complete grid layout, validation order and tie-breaking.
// Residual: 90.8771%, with all 39 branches and the return agreeing; VC6 keeps
// `this` in EDI instead of EBX, adding one reload block (60 vs 59). 64 D1 and
// 256 D2 ordinary-source trees were exhausted. Dreamcast lines 519 and 532
// retain OffsetToFront calls. Restoring those canonical header-inline calls
// is byte-flat under VC6; the third facing adjustment has no such attribution.
VA(0x00474690, 0x36B)  // anchor-callee: CanFit/SeedCombatPosition/GetSpeed + order-map, dc 0x6b66c
void combatManager::setCombatDirections(int hex)
{
    if (static_cast<const combatManager*>(this)->isQuickCombat()
            || isComputerAction(getCurrentArmy()))
        return;

    unsigned char secondIsValid;
    long attackAngle;
    long firstHex;
    int targetIndex;
    int targetGroup;
    army* currentArmy;
    unsigned char firstIsValid;
    long closest;
    long secondHex;

    currentArmy = getCurrentArmy();
    int oldSide = currentArmy->m_side;
    int oldSlot = currentArmy->m_slot;
    currentArmy->m_side = -1;
    currentArmy->m_slot = -1;

    g_searchArray->seedCombatPosition(currentArmy, m_currentSide,
                                     currentArmy->getSpeed(), 0, -1);

    { for (long i = 0; i < COMBAT_ATTACK_ANGLE_COUNT; i++) {
            m_combatDirections[0][i] = 0;
            m_combatDirections[1][i] = -1;
        }
    }

    for (attackAngle = COMBAT_ATTACK_ANGLE_0;
         attackAngle < COMBAT_ATTACK_ANGLE_COUNT; attackAngle++) {
        targetIndex = attackAngle / 2;
        firstHex = m_adjacentCells[hex][targetIndex];
        if (!validHex(firstHex))
            continue;
        firstIsValid =
            m_cells[firstHex].m_validMove
            && currentArmy->canFit(firstHex, 0, 0)
            && !g_searchArray->isMoat(firstHex);
        if (firstIsValid && currentArmy->is(creatureDoubleWide)
                && g_searchArray->isMoat(
                    firstHex + currentArmy->offsetToFront(-1)))
            firstIsValid = 0;

        targetGroup = (targetIndex + 3) % 6;
        if (firstIsValid) {
            m_combatDirections[1][attackAngle] = firstHex;
            m_combatDirections[0][attackAngle] = targetGroup + 7;
        }

        if (!currentArmy->is(creatureDoubleWide))
            continue;

        secondHex = firstHex - currentArmy->offsetToFront(-1);
        secondIsValid =
            m_cells[secondHex].m_validMove
            && currentArmy->canFit(secondHex, 0, 0)
            && !g_searchArray->isMoat(secondHex);
        if (secondIsValid && currentArmy->is(creatureDoubleWide)
                && g_searchArray->isMoat(
                    secondHex + (currentArmy->m_facing ? 1 : -1)))
            secondIsValid = 0;

        if (!firstIsValid && !secondIsValid)
            continue;

        m_combatDirections[0][attackAngle] = targetGroup + 7;
        if (targetIndex == COMBAT_DIRECTION_1
                || targetIndex == COMBAT_DIRECTION_4) {
            if (secondIsValid)
                m_combatDirections[1][attackAngle] = secondHex;
            continue;
        }

        if ((currentArmy->m_facing == 0) == (targetIndex <= 2)) {
            std::swap(firstHex, secondHex);
            std::swap(firstIsValid, secondIsValid);
        }

        targetGroup = targetIndex >= 2 && targetIndex <= 3 ? 13 : 14;
        if (firstIsValid && (!secondIsValid
                || (attackAngle != COMBAT_ATTACK_ANGLE_5
                    && attackAngle != COMBAT_ATTACK_ANGLE_6
                    && attackAngle != COMBAT_ATTACK_ANGLE_0
                    && attackAngle != COMBAT_ATTACK_ANGLE_11))) {
            m_combatDirections[1][attackAngle] = firstHex;
        } else {
            m_combatDirections[1][attackAngle] = secondHex;
            m_combatDirections[0][attackAngle] = targetGroup;
        }
    }

    { for (long i = 0; i < COMBAT_ATTACK_ANGLE_COUNT; i++) {
            if (m_combatDirections[0][i])
                continue;

            closest = COMBAT_ATTACK_ANGLE_COUNT;
            for (long j = 0; j < COMBAT_ATTACK_ANGLE_COUNT; j++) {
                if (i == j)
                    continue;
                long distance = abs(i - j);
                if (distance > 6)
                    distance = 6 - distance;
                if (m_combatDirections[0][j] && distance < closest) {
                    closest = distance;
                    m_combatDirections[0][i] = m_combatDirections[0][j];
                    m_combatDirections[1][i] = m_combatDirections[1][j];
                }
            }
        }
    }

    currentArmy->m_side = oldSide;
    currentArmy->m_slot = oldSlot;
}

VA_COMPGEN(0x0047a670, 0x11, TREE_BEGIN, int_set)

// Original: combatManager::HighlightHex; command.cpp:860, dc 0x6bde0.
void combatManager::highlightHex(int hex)
{
    updateMouseGrid(hex, 0);
    drawFrame(1, 0, 0, 0, 1, 0);
}

// Original: combatManager::HighlightHex; command.cpp:866, dc 0x6be08.
void combatManager::highlightHex(int x, int y)
{
    highlightHex(getGridIndex(x, y));
}

// Original: combatManager::ValidAttackHex; command.cpp:888, dc 0x6be74.
int combatManager::validAttackHex(int hex)
{
    if (hex < 0)
        return -1;
    for (int direction = 0; direction < COMBAT_ATTACK_ANGLE_COUNT; ++direction) {
        if (m_combatDirections[1][direction] == hex)
            return direction;
    }
    return -1;
}

// E:\gamedcs\command.cpp:907. The DC line table proves this helper boundary
// and its two-comparison body. Mac retains the ordinary helper; Complete
// expands its call into ProcessCombatMsg.

int combatManager::getPointer(int inCombatCommand, int /* iHexIndex */)
{
    if (inCombatCommand == COMBAT_COMMAND_VIEW_OTHER_HERO
            || inCombatCommand == COMBAT_COMMAND_VIEW_TOWERS)
        return 5;
    return inCombatCommand;
}

// DC class function types 0x1fd5/0x4c8e declare a void member with three
// int arguments, without a procedure or inline source row. Complete adds
// an unsigned-byte "pointer changed" result consumed by ProcessCombatMsg.
// Its retained body belongs to command.cpp's retail band; no DC header
// definition or inline qualifier is inferred from that declaration.
// The standalone retail body is independently fixed by its exact field graph:
// convert the mouse/hex tuple into one of the twelve SetCombatDirections
// slots, cache that slot's destination hex, and select its combat cursor frame
// only when the frame changes.
// Preserve the separate quick-combat and computer-action checks in one
// failure scope. Both do/while(0) and for(;;) remove the early join at
// unchanged 97.6434%; the full cursor body remains after those checks.
// Positive/negative nested-body guards instead score 85.8951%, and the
// earlier direct zero return loses 3.9161 points. The canonical nullary
// isComputerAction wrapper does not recover the required separate checks.
// Remaining differences are the ESI/EDI register binding, with the same
// retail CFG and calls; this is not a claim of complete byte equality.
VA(0x00474a00, 0x198)  // anchor-fields combatDirections/field_132d8 + SetPointer, dc member type 0x4c8e
unsigned char combatManager::checkSetMouseDirection(int x, int y, int hex)
{
    int direction;
    float slope;

    do {
        if (static_cast<const combatManager*>(this)->isQuickCombat())
            break;
        if (isComputerAction(getCurrentArmy()))
            break;
        int xDifference = x - (hex % 17) * 44 - 14;
        int row = hex / 17;
        if (!(row & 1))
            xDifference -= 22;
        xDifference -= 22;
        int yDifference = y - row * 42 - 112;

        direction = 0;
        if (xDifference < 0) {
            if (yDifference < 0)
                direction = 9;
            else
                direction = 6;
        } else if (yDifference >= 0) {
            direction = 3;
        }

        if (abs(yDifference) == 0)
            slope = 100.0f;
        else {
            slope = static_cast<float>(abs(xDifference));
            slope = slope / abs(yDifference);
        }

        if (direction != COMBAT_ATTACK_ANGLE_0
                && direction != COMBAT_ATTACK_ANGLE_6) {
            if (slope < 0.58)
                direction += 2;
            else if (slope < 1.73)
                direction++;
        } else {
            if (slope > 1.73)
                direction += 2;
            else if (slope > 0.58)
                direction++;
        }

        m_lastMoveToIndex = m_combatDirections[1][direction];
        if (m_combatDirections[0][direction] == m_lastAttackCursor)
            return 0;

        m_lastAttackCursor = m_combatDirections[0][direction];
        g_mouseManager->setPointer(m_combatDirections[0][direction],
                                   mouseManager::COMBAT_SET);
        return 1;
    } while (0);
    return 0;
}

// E:\gamedcs\command.cpp:928, dc 0x6bebc.
VA(0x00474ba0, 0x4A)  // anchor-callee IsQuickCombat + current-army forwarding, dc 0x6bebc
unsigned char combatManager::isComputerAction()
{
    if (static_cast<const combatManager*>(this)->isQuickCombat())
        return 1;
    return isComputerAction(getCurrentArmy());
}

// Complete adds this one-argument policy overload. CodeView's full class
// field lists (method types 0x4354, 0x4ca2 and 0x6700) declare only the
// nullary method above; that DC body obtains its stack through
// get_current_army before applying the policy. Retail retains both entries:
// the adapter calls this worker at 0x474bf0, whose `ret 4` and reads at
// stack offsets +0x34, +0x288 and +0xf4 prove the explicit army argument.

// THE OPTIONS ARE PREFERENCE FIELDS, NOT STANDALONE GLOBALS. All four
// dwords this body reads land inside configStruct (retail .bss
// 0x698758, claimed in misc.cpp): combatCatapult (+0x44),
// combatBallista (+0x48), combatFirstAidTent (+0x4c) and
// combatAutoCreatures (+0x3c) - the same five war-machine flags
// SetDefaultCombatOptions initialises, which is what fixes the DEFAULT
// arm's field as combatAutoCreatures rather than a catch-all.

// The byte at 0x691209 is soundmgr's gbUnk691209. Nothing here
// contradicts that TU's reading: the address is an ordinal-named byte
// flag with two independent readers, and both do nothing but test it
// non-zero (`mov al, byte [0x691209]; test al, al`). The sound guard's
// `field_84 || gbUnk691209` and this body's `gbUnk691209 && field_132b4`
// are both consistent with a single global "an automated/attract mode is
// running" latch; neither reader constrains the other, so the name
// stays soundmgr's.

// CASE ORDER IS THE SOURCE'S, not the case values'. The jump table at
// the tail maps 0x91..0x95 onto four blocks, and those blocks are
// EMITTED in the order ballista/arrow-tower, catapult, first-aid tent,
// default - so the switch was written with the artillery pair first.
// Each of the three machine arms repeats the same three guards
// verbatim; retail duplicates them rather than factoring, and the two
// `return 1` epilogues (one shared, one tail-duplicated at the end of
// each arm) are what that longhand costs.

// Only the artillery arm null-checks the owner. That asymmetry is
// retail's, not a modelling gap: the catapult and first-aid arms
// dereference get_owner's result unguarded.

// Use the controlling player, which can change when a stack is hypnotized.
VA(0x00474bf0, 0x188)  // anchor-global + retained nullary caller, retail-only overload
unsigned char combatManager::isComputerAction(const army* currentArmy)
{
    if (static_cast<const combatManager*>(this)->isQuickCombat())
        return 1;

    hero* owner = currentArmy->getController();
    switch (currentArmy->m_creatureType) {
    case CREATURE_BALLISTA:
    case CREATURE_ARROW_TOWER:
        if (m_creaturePlacement)
            return 0;
        if (m_autoCombatOn && g_config.m_combatBallista)
            return 1;
        if (owner == 0)
            return 1;
        if (owner->m_skillLevel[20] == 0)
            return 1;
        if (g_goSolo && m_thisNetHasControl)
            return 1;
        break;
    case CREATURE_CATAPULT:
        if (m_creaturePlacement)
            return 0;
        if (m_autoCombatOn && g_config.m_combatCatapult)
            return 1;
        if (owner->m_skillLevel[eSecSkillSiegeBallistics] == 0)
            return 1;
        if (g_goSolo && m_thisNetHasControl)
            return 1;
        break;
    case CREATURE_FIRST_AID_TENT:
        if (m_creaturePlacement)
            return 0;
        if (m_autoCombatOn && g_config.m_combatFirstAidTent)
            return 1;
        if (owner->m_skillLevel[27] == 0)
            return 1;
        if (g_goSolo && m_thisNetHasControl)
            return 1;
        break;
    default:
        if (m_autoCombatOn && g_config.m_combatAutoCreatures)
            return 1;
        if (g_goSolo && m_thisNetHasControl)
            return 1;
        break;
    }

    int side = currentArmy->getControllingSide();
    int player = m_playerIds[side];
    return player == -1 || !g_game->isHuman(player);
}

// E:\gamedcs\command.cpp:1001. The retail identity is closed by the
// command.obj order slot immediately after is_computer_action, both direct
// calls from Main, the reference-parameter decorated ABI, and the obstacle
// diagnostic literal in Dreamcast's line-1348 statement group.
// CENSUS 2026-09-05 (93.0970): the two cross-jumped arms are NAMED, and the
// whole residual reduces to ONE cause upstream of them.
// Positional alignment of the two call streams pairs our six `IsHuman` arms
// against retail's four.  Ours sit after ViewSpells+InitiateSpell (+0x224),
// after the first NormalDialog (+0x2c0), after the second NormalDialog
// (+0x48a), after SetupGridForArmy+DrawFrame twice (+0xd0b, +0xe82) and after
// SetPointer+ViewArmy (+0x10ce).  Retail has the LAST FOUR of those and not
// the first and third: at the InitiateSpell arm it emits
// `mov ecx,[ebx+0x132c0] / mov eax,[ebx+4*ecx+0x54a8] / jmp <the first
// NormalDialog arm's own test>` - a cross-jump into the sibling arm's tail
// starting at that arm's `test eax,eax`.
// WHY OURS CANNOT MERGE THERE, and it is not a spelling of the arms: retail
// tests those zeros with `test eax,eax` while we compare against a ZERO CACHED
// IN ESI (`cmp dword ptr [ebx+0x132b4], esi`), and we cache 1 in EDI as well
// (`mov edi,1` at fn+0x1d, against retail's `cmp ecx,1` immediate).  The two
// arms therefore have tails that are byte-identical to each other but carry an
// extra live-register dependency, and C2 declines the merge.  Census: our
// `push esi` 20 / `cmp ..,esi` 39 against retail 12 / 27.
// AND THE CONSTANT CACHE IS DOWNSTREAM OF THE FRAME, WHOSE 0x14 IS NOW
// FULLY ACCOUNTED FOR - it is ONE overlay decision, not missing locals.
// Retail reserves 0x60 and lays out three non-overlapping regions:
// [-0x60,-0x40) the hidden return temporary PeekEvent fills, [-0x40,-0x20)
// `msgTemp` (32 B), [-0x20,-0xc) the network arm's `CEndPlacementPhaseMsg
// placementMsg` (0x14 B), with mouseX/mouseY at -0xc/-0x8.  We reserve 0x4c
// because C2 OVERLAID msgTemp onto placementMsg: our msgTemp sits at -0x2c
// and spans -0x2c..-0xc, straight across placementMsg's -0x20..-0xc, and the
// two never live at once (one is the WIDGET arm, the other MOUSE_MOVE).  So
// retail's msgTemp must be LIVE across the network arm and ours is not, which
// is a use of msgTemp our reconstruction does not have - not a missing
// declaration.  Every ebp slot below -0x20 and the three `lea`s match
// one-for-one otherwise, and the class sizes are confirmed by retail's own
// `mov [ebp-0x14],0x14` (sizeof CEndPlacementPhaseMsg) and by the 8-dword
// `rep movsd` into msgTemp.
// MEASURED AND REJECTED here: declaring msgTemp above mouseX/mouseY is
// byte-flat (93.0970 to the digit, frame still 0x4c), so declaration order is
// not the lever - only a real second use of msgTemp can be.
VA(0x00474d80, 0x114D)  // exhaustive command order-map + callers + literal/call graph, dc 0x6c070
int combatManager::processCombatMsg(message& msg)
{
    int mouseX = msg.m_mouseX;
    int mouseY = msg.m_mouseY;
    // Dreamcast places this sole named local in the function frame before
    // the dispatch switch. Retail's PeekEvent path retains the same
    // whole-function lifetime even though the default construction folds out.
    message msgTemp;

    switch (msg.m_id) {
    case MESSAGE_WIDGET:
        if (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT) {
            if (msg.m_codeX == widget::WIDGET_SELECT
                    || msg.m_codeX == widget::WIDGET_RIGHT_SELECT) {
                if (msg.m_codeY == 0 || msg.m_codeY == 1)
                    rightClick(m_lastCellIndex);
                else
                    m_combatWindow->processRightSelect(&msg);
            }
            break;
        }

        switch (msg.m_codeX) {
        case widget::WIDGET_SELECT:
            if (m_thisNetHasControl && msg.m_codeY >= 0 && msg.m_codeY <= 1) {
                m_combatWindow->m_heroSubWindows[0]->unShow();
                m_combatWindow->m_heroSubWindows[1]->unShow();
                m_combatWindow->m_creatureSubWindows[0]->unShow();
                m_combatWindow->m_creatureSubWindows[1]->unShow();
                m_combatWindow->m_creatureSubWindows[2]->unShow();
                m_combatWindow->m_creatureSubWindows[3]->unShow();
                drawFrame(1, 0, 0, 0, 1, 0);
                doCommand(m_combatCommand);
            }
            break;

        case widget::WIDGET_DESELECT:
            if (!m_thisNetHasControl)
                break;

            switch (msg.m_codeY) {
            case TCombatWindow::COMBAT_LEFT_COMMAND_3_ID:
                m_autoCombatOn = !m_autoCombatOn;
                if (m_autoCombatOn)
                    getControl();
                if (m_autoCombatOn
                        && (static_cast<const combatManager*>(this)->isQuickCombat()
                            || isComputerAction(getCurrentArmy()))) {
                    m_combatWindow->m_controlSubWindow->disableAllButtons();
                }
                break;

            case TCombatWindow::COMBAT_RIGHT_COMMAND_2_ID:
                m_nextAction = 3;
                break;

            case TCombatWindow::COMBAT_LEFT_COMMAND_2_ID:
                combatSystemOptions();
                break;

            case TCombatWindow::COMBAT_RIGHT_COMMAND_0_ID:
                if (!m_heroes[m_currentSide]) {
                    normalDialog((*g_generalText)[GENERAL_TEXT_COMBAT_NO_HERO_FOR_SPELL],
                                 1, -1, -1, -1, 0,
                                 -1, 0, -1, 0, -1, 0);
                } else {
                    initiateSpell(viewSpells(), 0);
                    resetMouse();
                }
                break;

            case TCombatWindow::COMBAT_LEFT_COMMAND_1_ID:
                normalDialog((*g_generalText)[GENERAL_TEXT_COMBAT_RETREAT_PROMPT],
                             2, -1, -1, -1, 0,
                             -1, 0, -1, 0, -1, 0);
                if (g_windowManager->m_dialogReturn == DIALOG_RETURN_ACCEPT)
                    m_nextAction = 4;
                resetMouse();
                break;

            case TCombatWindow::COMBAT_LEFT_COMMAND_0_ID:
                if (doSurrender()) {
                    if (g_game->m_players[m_playerIds[m_currentSide]].m_resources[6]
                            < g_surrenderCost) {
                        normalDialog((*g_generalText)[GENERAL_TEXT_COMBAT_NOT_ENOUGH_GOLD],
                                     1, -1, -1, -1, 0,
                                     -1, 0, -1, 0, -1, 0);
                    } else {
                        m_nextAction = 5;
                        m_nextActionExtra = g_surrenderCost;
                    }
                }
                resetMouse();
                break;

            case TCombatWindow::COMBAT_RIGHT_COMMAND_1_ID:
            case TCombatWindow::COMBAT_PLACEMENT_COMMAND_0_ID:
                m_nextAction = 8;
                break;

            case TCombatWindow::COMBAT_PLACEMENT_COMMAND_1_ID:
                m_lastMovedArmy = 0;
                if (g_remoteOn) {
                    // The Dreamcast NB11 stream gives placementMsg its own
                    // nested lexical scope inside the network arm.
                    {
                        CEndPlacementPhaseMsg placementMsg;
                        transmitRemoteData(
                            &placementMsg,
                            g_combatControlNetPos[1 - m_currentSide],
                            false, true);
                    }
                }
                finishCreaturePlacement();
                nextArmy(1);
                m_backgroundDrawn = 0;
                drawFrame(1, 0, 0, 0, 1, 0);
                break;
            }
            break;
        }
        break;

    case MESSAGE_MOUSE_MOVE: {
        unsigned char pointerChanged = 0;
        if ((m_autoCombatOn || g_goSolo)
                && (static_cast<const combatManager*>(this)->isQuickCombat()
                    || isComputerAction(getCurrentArmy())))
            break;

        msgTemp = g_inputManager->peekEvent();
        if (msgTemp.m_id == MESSAGE_MOUSE_MOVE)
            break;

        int gridIndex = getGridIndex(mouseX, mouseY);
        updateMouseGrid(gridIndex, 0);

        if (!inCombatArea(mouseX, mouseY)) {
            turnOffHighlighter(1);

            m_combatWindow->m_heroSubWindows[0]->unShow();
            m_combatWindow->m_heroSubWindows[1]->unShow();
            m_combatWindow->m_creatureSubWindows[0]->unShow();
            m_combatWindow->m_creatureSubWindows[1]->unShow();
            m_combatWindow->m_creatureSubWindows[2]->unShow();
            m_combatWindow->m_creatureSubWindows[3]->unShow();
            g_windowManager->convertToHover(msg);
            g_mouseManager->setPointer(6, mouseManager::COMBAT_SET);
            m_lastCellIndex = -1;
            m_lastCommand = -99;
            return MESSAGE_DISPATCH_CONSUME;
        }

            if (gridIndex == m_lastCellIndex) {
            if (gridIndex != -1 && m_combatCommand == COMBAT_COMMAND_ATTACK)
                pointerChanged = checkSetMouseDirection(mouseX, mouseY,
                                                        gridIndex);
            } else {
            m_combatWindow->m_heroSubWindows[0]->unShow();
            m_combatWindow->m_heroSubWindows[1]->unShow();
            m_combatWindow->m_creatureSubWindows[0]->unShow();
            m_combatWindow->m_creatureSubWindows[1]->unShow();
            m_combatWindow->m_creatureSubWindows[2]->unShow();
            m_combatWindow->m_creatureSubWindows[3]->unShow();

            if (gridIndex == COMBAT_HEX_DEFENDER_HERO) {
                if (m_heroes[1] && g_config.m_combatArmyInfoLevel) {
                    m_combatWindow->m_heroSubWindows[1]->update(
                        *m_heroes[1], m_heroes[0],
                        m_magicTerrain
                            == COMBAT_SPELL_RESTRICTION_NO_CREATURE_SPELLS);
                    m_combatWindow->m_heroSubWindows[1]->show();
                }
            } else if (gridIndex == COMBAT_HEX_ATTACKER_HERO) {
                if (m_heroes[0] && g_config.m_combatArmyInfoLevel) {
                    m_combatWindow->m_heroSubWindows[0]->update(
                        *m_heroes[0], m_heroes[1],
                        m_magicTerrain
                            == COMBAT_SPELL_RESTRICTION_NO_CREATURE_SPELLS);
                    m_combatWindow->m_heroSubWindows[0]->show();
                }
            } else if (validHex(gridIndex)) {
                if (m_cells[gridIndex].hasArmy()) {
                    army* stack = m_cells[gridIndex].getArmy();
                    hero* owner = stack->getOwner();
                    if (g_config.m_combatArmyInfoLevel
                            == TCombatOptionsWindow::
                                CREATURE_INFO_LEVEL_VERBOSE) {
                        if (stack->getOwningSide() == 0) {
                            m_combatWindow->m_creatureSubWindows[0]->update(*stack,
                                                                        owner);
                            m_combatWindow->m_creatureSubWindows[0]->show();
                        } else if (stack->getOwningSide() == 1) {
                            m_combatWindow->m_creatureSubWindows[1]->update(*stack,
                                                                        owner);
                            m_combatWindow->m_creatureSubWindows[1]->show();
                        }
                    } else if (g_config.m_combatArmyInfoLevel
                               == TCombatOptionsWindow::
                                   CREATURE_INFO_LEVEL_COMPACT) {
                        if (stack->getOwningSide() == 0) {
                            m_combatWindow->m_creatureSubWindows[2]->update(*stack,
                                                                        owner);
                            m_combatWindow->m_creatureSubWindows[2]->show();
                        } else if (stack->getOwningSide() == 1) {
                            m_combatWindow->m_creatureSubWindows[3]->update(*stack,
                                                                        owner);
                            m_combatWindow->m_creatureSubWindows[3]->show();
                        }
                    }
                } else if (m_debugShowBlockedHexes
                           && (m_cells[gridIndex].m_attributes & hexcell::blocked)
                           && m_cells[gridIndex].m_obstacleIndex != -1) {
                    TObstacle& obstacle =
                        m_obstacles[m_cells[gridIndex].m_obstacleIndex];
                    sprintf(g_text,
                            "Obstacle name: %s, owner: %d, visible:%s",
                            obstacle.m_shape->m_spriteName, obstacle.m_owner,
                            obstacle.m_isVisible ? "true" : "false");
                    m_combatWindow->combatMessage(g_text, 0, 0);
                    m_lastCellIndex = gridIndex;
                    return MESSAGE_DISPATCH_CONSUME;
                }
            }

            if (gridIndex != m_lastCellIndex)
                checkChangeHighlighter(gridIndex);

            m_lastCellIndex = gridIndex;
            m_lastCommand = -99;
            m_combatCommand = getCommand(gridIndex);
            m_lastAttackCursor = 6;
            if (m_combatCommand == COMBAT_COMMAND_ATTACK) {
                setCombatDirections(gridIndex);
                pointerChanged = checkSetMouseDirection(mouseX, mouseY,
                                                        gridIndex);
            } else if (m_combatCommand == COMBAT_COMMAND_CREATURE_SPELL) {
                g_mouseManager->setPointer(0, mouseManager::SPELL_SET);
            } else {
                g_mouseManager->setPointer(
                    getPointer(m_combatCommand, gridIndex),
                    mouseManager::COMBAT_SET);
            }
            }

            if (m_combatCommand != m_lastCommand
                    || (m_combatCommand == COMBAT_COMMAND_ATTACK && pointerChanged)) {
                m_lastCommand = m_combatCommand;
                combatMessage(m_combatCommand);
            }
        break;
    }

    case MESSAGE_KEY_DOWN:
        switch (msg.m_codeX) {
        case KEYCODE_F5:
            setCombatViewArmy(
                (g_config.m_combatArmyInfoLevel + 1) % 3);
            break;

        case KEYCODE_F6:
            setCombatGrid(!g_config.m_showCombatGrid,
                          g_config.m_showCombatMouseHex,
                          g_config.m_combatShadeLevel, 1);
            break;

        case KEYCODE_F7:
            setCombatGrid(g_config.m_showCombatGrid,
                          !g_config.m_showCombatMouseHex,
                          g_config.m_combatShadeLevel, 1);
            break;

        case KEYCODE_F8:
            setCombatGrid(g_config.m_showCombatGrid,
                          g_config.m_showCombatMouseHex,
                          !g_config.m_combatShadeLevel, 1);
            break;

        case KEYCODE_KP_MINUS:
            m_combatWindow->scrollRollover(-1);
            break;

        case KEYCODE_KP_2:
            m_combatWindow->scrollRollover(1);
            break;

        case KEYCODE_F:
            if (m_creaturePlacement)
                break;
            if ((m_autoCombatOn || g_goSolo)
                    && (static_cast<const combatManager*>(this)->isQuickCombat()
                        || isComputerAction(getCurrentArmy())))
                break;
            {
                army* currentArmy = getCurrentArmy();
                if (currentArmy->m_creatureType == CREATURE_FAERIE_DRAGON
                        && currentArmy->m_monInfo.m_hasSpell) {
                    initiateSpell(currentArmy->m_faerieDragonSpell, 1);
                    if (m_nextAction == 1)
                        m_nextAction = 10;
                }
            }
            break;

        case KEYCODE_T:
            if (m_creaturePlacement)
                break;
            m_combatWindow->m_heroSubWindows[0]->unShow();
            m_combatWindow->m_heroSubWindows[1]->unShow();
            m_combatWindow->m_creatureSubWindows[0]->unShow();
            m_combatWindow->m_creatureSubWindows[1]->unShow();
            m_combatWindow->m_creatureSubWindows[2]->unShow();
            m_combatWindow->m_creatureSubWindows[3]->unShow();
            g_mouseManager->setPointer(6, mouseManager::COMBAT_SET);
            viewArmy(getCurrentArmy(), 0);
            resetMouse();
            break;
        }
        break;
    }

    return MESSAGE_DISPATCH_CONSUME;
}

// E:\gamedcs\command.cpp:1746. The DC statement table supplies the
// round/stack/obstacle loop spine; Complete's retail body inserts the
// placement-phase handoff in front and widens the stack rows to the retail
// 0x548-byte army layout. The identity and extent are independently fixed by
// the unique whole-body retail map and the exhaustive command order-map.
// RESIDUAL (97.48472%): all 21 branches and both returns are exact. The 24
// register-visible differences are confined to the placement message and the
// inlined TurnOffHighlighter path; why-reg v2 classifies them as C1 front-end
// scratch-pseudo ordering with the same callee-saved bindings. Named/value/
// pointer remote-position locals, reversed endpoint spelling, and an explicit
// highlighter expansion were measured and were byte-identical or worse. The
// DC statement row independently requires the TurnOffHighlighter call here.
// Original: combatManager::GetHexXY; command.cpp:1638, dc 0x6ce2c.
void combatManager::getHexXY(int hex, int& x, int& y)
{
    x = (m_cells[hex].m_hexUlx + m_cells[hex].m_hexBrx) / 2;
    y = (m_cells[hex].m_hexUly + m_cells[hex].m_hexBry) / 2;
}

VA(0x00475ed0, 0x32F)  // unique retail body + order-map, dc 0x6d060
void combatManager::resetRound()
{
    m_turnNumber++;
    if (m_creaturePlacement
            && (static_cast<const combatManager*>(this)->isQuickCombat()
                || isComputerAction(getCurrentArmy()))
            && (!m_anyActionTaken || m_turnNumber >= 3)) {
        finishCreaturePlacement();

        if (g_remoteOn) {
            CEndPlacementPhaseMsg msg;
            transmitRemoteData(&msg,
                               g_combatControlNetPos[1 - m_currentSide],
                               false, true);
        }
        return;
    }

    turnOffHighlighter(1);

    m_anyActionTaken = 0;
    m_inSecondPhase = 0;
    memset(m_creatureIsDead, 0, sizeof(m_creatureIsDead));
    m_someCreaturesVanish = 0;
    m_castleAttackDone = 0;

    for (int side = 0; side < 2; side++) {
        m_dohPlayedThisRound[side] = 0;
        m_yeahPlayedThisRound[side] = 0;
        m_playDoh[side] = 0;
        m_playYeah[side] = 0;
        m_spellsCast[side] = 0;
        m_turnSinceLastEnchanter[side]++;
        for (int slot = 0; slot < 20; slot++) {
            army* stack = &m_armies[side][slot];
            // DC 1788 and retail +0x237 test creatureType at +0x34;
            // gridIndex is the distinct field at +0x38.
            if (stack->m_creatureType != CREATURE_NONE)
                stack->resetRound();
        }
    }

    if (m_someCreaturesVanish)
        makeCreaturesVanish();

    for (TObstacle* obstacle = m_obstacles.begin();
            obstacle != m_obstacles.end(); ++obstacle) {
        if (obstacle->m_duration > 0) {
            obstacle->m_duration--;
            if (obstacle->m_duration == 0) {
                removeObstacle(obstacle - m_obstacles.begin());
                if (obstacle->m_dispelEffect != -1)
                    spellEffect(obstacle->m_dispelEffect, obstacle->m_hex, 100, 0);
            }
        }
    }

    if (!m_creaturePlacement
            && !static_cast<const combatManager*>(this)->isQuickCombat()) {
        m_combatWindow->combatMessage(
            (*g_generalText)[GENERAL_TEXT_COMBAT_ROUND], 1, 0);
    }
    m_lastMovedArmy = 0;
}

VA(0x00476200, 0xE1)  // dc 0x6d2c8
void combatManager::autoResolveCombat()
{
    // ai_combat.cpp's free function is __fastcall under the shared /Gr
    // profile. Keep its one-use declaration at block scope: a file-scope
    // declarator crosses command.obj's measured GetCommand handle wall.
    void aiAutoCombat(hero* attackingHero, hero* defendingHero,
                        armyGroup& attackingArmy, armyGroup& defendingArmy,
                        const town* defendingTown, NewmapCell* cell);

    armyGroup localArmies[2];
    int side;
    for (side = 0; side < 2; side++)
        localArmies[side] = *m_armyGroups[side];

    aiAutoCombat(m_heroes[0], m_heroes[1], localArmies[0], localArmies[1],
                   m_defendingTown, m_combatCell);

    for (side = 0; side < 2; side++) {
        for (int slot = 0; slot < m_numArmies[side]; slot++) {
            army* stack = &m_armies[side][slot];
            if (stack->m_numTroops > 0
                    && !stack->is(creatureSiegeWeapon)
                    && stack->m_originalIndex >= 0) {
                stack->m_numTroops =
                    localArmies[side].m_numTroops[stack->m_originalIndex];
                if (stack->m_numTroops == 0)
                    stack->m_monInfo.m_attributes |= creatureImmobilized;
            }
        }
    }
}

VA(0x004762f0, 0xF6)  // dc 0x6d430
int combatManager::checkWin(message* msg)
{
    if (static_cast<const combatManager*>(this)->isQuickCombat()
            && m_turnNumber > 30)
        autoResolveCombat();

    if (!combatIsOver())
        return 0;

    m_battleOver = 1;
    m_winner = -1;
    if (isWinner(m_currentSide))
        m_winner = m_currentSide;
    if (isWinner(1 - m_currentSide)) {
        if (m_winner == -1)
            m_winner = 1 - m_currentSide;
        else
            m_winner = -1;
    }

    if (m_sideRetreated[0] || m_sideRetreated[1])
        g_combatRetreated = 1;
    if (m_sideSurrendered[0] || m_sideSurrendered[1])
        g_combatSurrendered = 1;

    doVictory(m_winner);
    msg->m_id = MESSAGE_EXECUTIVE;
    msg->m_codeX = EXECUTIVE_COMMAND_TERMINATE_LOOP;
    return 1;
}

VA(0x004763f0, 0x4D)  // dc 0x6d508
unsigned char combatManager::isOutsidePlacementBoundry(int group, int index)
{
    if (group == 0)
        return index % COMBAT_GRID_ROW_STRIDE
            > m_placementBoundaryDepth * 2 + 1;
    return index % COMBAT_GRID_ROW_STRIDE
        < m_placementBoundaryDepth * 2 + 15;
}

VA(0x00476440, 0x50)  // dc 0x6d548
unsigned char combatManager::validWallTarget(TWallTargetId wall)
{
    if ((wall == WALL_TARGET_0 || wall == WALL_TARGET_6)
        && m_fortificationLevel < COMBAT_FORTIFICATION_CASTLE)
        return 0;
    if (wall == WALL_TARGET_7
        && m_fortificationLevel < COMBAT_FORTIFICATION_CITADEL)
        return 0;
    return m_wallStrength[s_wallTargets[wall].m_wall] > 0;
}

// E:\gamedcs\command.cpp:1964
// THE LADDER, in retail's own test order. -1 (no hex) answers 0. A
// network game in which this side is not under interactive control
// (field_132b4 clear) short-circuits the whole thing and answers the
// bare hover pair 5/6. The two hero panels answer 4 for the panel that
// belongs to the side on turn and 21 for the other, and 0 when that
// side has no hero at all. Then the two castle structures GetGridIndex
// answers with pseudo-hexes for, then the real grid.

// VALID_WALL_TARGET IS INLINED THREE TIMES, and its expansion is what
// gives this body its two odd-looking shapes. (1) The fortification
// guard is hoisted out ahead of the pseudo-hex test in the 255 and 254
// arms - `cmp [this+0x132f4], 3` then `cmp newIndex, 0xff` - because
// the same member load serves both arms (retail loads it ONCE at
// 0x4764f6 and compares it against 3 and then 2), and inside each arm
// the redundant `< CASTLE` / `< CITADEL` half of the inlined predicate
// is folded away by the dominating test. (2) In the wall LOOP the
// expansion keeps both halves, reading field_132f4 out of the ebx it
// parked the value in, which is why the loop body carries the
// `i == 0 || i == 6` / `i == 7` tier tests verbatim.

// TWO JUMP-THREADED COMPARES look like duplicated source and are not.
// Retail tests `targetSide == currentSide` twice (0x476ff1 and
// 0x47701f) and `bCreaturePlacement` twice (0x476a05 and 0x476a30); in
// both pairs the FIRST test's failure edge jumps past the second test
// entirely, which is exactly what VC6 does to two consecutive ifs on
// the same condition. Writing them as one if with the arms merged
// produces a different graph.

// THE WALL LOOP breaks on the first row whose hex matches, whether or
// not that row is a legal target - retail's failure edges out of the
// inlined valid_wall_target all land on the loop EXIT, not on the
// increment.

// SEEDCOMBATPOSITION's `limit` argument is army+0xc4, the speed field
// of the embedded traits record; its `in_placement_phase` is pushed as
// a whole register whose low byte only is loaded, the ordinary
// unsigned char argument form.
// Dreamcast line rows 1972/2077 retain hexcell::HasArmy, 2033 retains
// ValidHex/InInvisibleColumn, and 2080/2181 retain army::get_owning_side.
// Using those existing helpers moves current VC6 similarity 92.57143%
// to 92.53571%; Mac's reviewed span still has all 13 direct calls aligned.

VA(0x00476490, 0x52A)  // anchor-global, dc 0x6d58c
int combatManager::getCommand(int newIndex)
{
    if (newIndex == -1)
        return COMBAT_COMMAND_NONE;

    if (g_remoteOn && m_thisNetHasControl == 0)
        return !m_cells[newIndex].hasArmy() ? COMBAT_COMMAND_HOVER
                                            : COMBAT_COMMAND_VIEW_ARMY;

    if (newIndex == COMBAT_HEX_DEFENDER_HERO) {
        if (m_heroes[1] == 0)
            return COMBAT_COMMAND_NONE;
        return m_currentSide == 1 ? COMBAT_COMMAND_SPELL_BOOK
                                : COMBAT_COMMAND_VIEW_OTHER_HERO;
    }

    if (newIndex == COMBAT_HEX_ATTACKER_HERO) {
        if (m_heroes[0] == 0)
            return COMBAT_COMMAND_NONE;
        return m_currentSide == 0 ? COMBAT_COMMAND_SPELL_BOOK
                                : COMBAT_COMMAND_VIEW_OTHER_HERO;
    }

    army* currentArmy = getCurrentArmy();

    if (m_fortificationLevel >= COMBAT_FORTIFICATION_CASTLE
            && newIndex == COMBAT_HEX_UPPER_TOWER) {
        if (currentArmy->is(creatureCatapult) && m_currentSide == 0
                && !m_creaturePlacement
                && validWallTarget(WALL_TARGET_0)) {
            currentArmy->m_slot = COMBAT_HEX_UPPER_TOWER;
            currentArmy->m_side = -1;
            return COMBAT_COMMAND_BOMBARD_WALL;
        }
        return COMBAT_COMMAND_VIEW_TOWERS;
    }

    if (m_fortificationLevel >= COMBAT_FORTIFICATION_CITADEL
            && newIndex == COMBAT_HEX_KEEP) {
        if (currentArmy->is(creatureCatapult) && m_currentSide == 0
                && !m_creaturePlacement
                && validWallTarget(WALL_TARGET_7)) {
            currentArmy->m_slot = COMBAT_HEX_KEEP;
            currentArmy->m_side = -1;
            return COMBAT_COMMAND_BOMBARD_WALL;
        }
        return COMBAT_COMMAND_VIEW_TOWERS;
    }

    if (!validHex(newIndex) || inInvisibleColumn(newIndex))
        return COMBAT_COMMAND_NONE;

    currentArmy->m_side = -1;
    currentArmy->m_slot = -1;

    if (m_cells[newIndex].hasArmy()
            && currentArmy->m_creatureType != CREATURE_CATAPULT) {
        army* target = m_cells[newIndex].getArmy();
        long targetSide = target->getOwningSide();

        if (m_creaturePlacement)
            return COMBAT_COMMAND_VIEW_ARMY;
        if (target == currentArmy)
            return COMBAT_COMMAND_VIEW_ARMY;
        if (currentArmy->m_creatureType == CREATURE_FAERIE_DRAGON
                && currentArmy->canCastSpell(newIndex))
            return COMBAT_COMMAND_CREATURE_SPELL;
        if (targetSide == m_currentSide && currentArmy->canCastSpell(newIndex))
            return COMBAT_COMMAND_CREATURE_SPELL;
        if (targetSide == m_currentSide
                && currentArmy->m_creatureType == CREATURE_FIRST_AID_TENT
                && target->m_topCreatureDamage > 0
                && !target->is(creatureSiegeWeapon))
            return COMBAT_COMMAND_FIRST_AID;
        if (targetSide == m_currentSide)
            return COMBAT_COMMAND_VIEW_ARMY;

        currentArmy->m_side = m_cells[newIndex].m_armySide;
        currentArmy->m_slot = m_cells[newIndex].m_armySlot;

        if (currentArmy->canShoot(0)) {
            if (currentArmy->m_creatureType == CREATURE_ARROW_TOWER)
                return COMBAT_COMMAND_SHOOT;
            if (!shotIsThroughWall(currentArmy, currentArmy->m_gridIndex,
                                   newIndex)
                    && !shotIsNotOptimal(currentArmy, target))
                return COMBAT_COMMAND_SHOOT;
            return COMBAT_COMMAND_SHOOT_PENALTY;
        }
        if (currentArmy->validPath(newIndex, 0))
            return currentArmy->m_creatureType == CREATURE_BALLISTA
                ? COMBAT_COMMAND_SHOOT : COMBAT_COMMAND_ATTACK;

        currentArmy->m_side = -1;
        currentArmy->m_slot = -1;
        return COMBAT_COMMAND_NONE;
    }

    if (currentArmy->is(creatureCatapult)
            && m_fortificationLevel > COMBAT_FORTIFICATION_NONE
            && m_currentSide == 0
            && !m_creaturePlacement) {
        // The counter is TWallTargetId-typed rather than a long with a
        // cast at the call: the value IS a wall-target id everywhere it
        // is used, both as wallTargets' subscript and as
        // valid_wall_target's argument. Codegen is identical - retail's
        // own counter is a plain dword in ecx.
        for (TWallTargetId wall = WALL_TARGET_0; wall < WALL_TARGET_COUNT;
                wall = TWallTargetId(wall + 1)) {
            if (newIndex == s_wallTargets[wall].m_targetHex) {
                if (validWallTarget(wall)) {
                    currentArmy->m_side = -1;
                    currentArmy->m_slot = newIndex;
                    return COMBAT_COMMAND_BOMBARD_WALL;
                }
                break;
            }
        }
    }

    if (!m_creaturePlacement && currentArmy->canCastResurrect(newIndex))
        return COMBAT_COMMAND_CREATURE_SPELL;

    if (!m_creaturePlacement
            || !isOutsidePlacementBoundry(currentArmy->getOwningSide(),
                                             newIndex)) {
        g_searchArray->seedCombatPosition(currentArmy, m_currentSide,
                                          currentArmy->m_monInfo.m_speed,
                                          m_creaturePlacement, -1);
        if (m_cells[newIndex].m_validMove || m_cells[newIndex].m_frontMove)
            return (currentArmy->is(creatureFlyingArmy)) ? COMBAT_COMMAND_FLY
                                            : COMBAT_COMMAND_WALK;
    }

    if (newIndex == s_wallTargets[WALL_TARGET_6].m_targetHex
            && m_fortificationLevel == COMBAT_FORTIFICATION_CASTLE)
        return COMBAT_COMMAND_VIEW_TOWERS;
    return COMBAT_COMMAND_NONE;
}

// Mac retains separate ViewCastleBallista calls for wall target 7 and the
// full-castle targets 0/6 at 0:0x84bf8 and 0:0x84ca8.
VA(0x004769c0, 0x207)  // dc 0x6d988
int combatManager::rightClick(int newIndex)
{
    if (newIndex == COMBAT_HEX_DEFENDER_HERO) {
        if (m_heroes[1]) {
            m_combatWindow->m_heroSubWindows[1]->update(*m_heroes[1], m_heroes[0],
                                                    m_magicTerrain == COMBAT_SPELL_RESTRICTION_NO_CREATURE_SPELLS);
            m_combatWindow->m_heroSubWindows[1]->show();
            g_windowManager->doQuickView(0);
            if (g_config.m_combatArmyInfoLevel)
                return 0;
            m_combatWindow->m_heroSubWindows[1]->unShow();
        }
        return 0;
    }

    if (newIndex == COMBAT_HEX_ATTACKER_HERO) {
        if (m_heroes[0]) {
            m_combatWindow->m_heroSubWindows[0]->update(*m_heroes[0], m_heroes[1],
                                                    m_magicTerrain == COMBAT_SPELL_RESTRICTION_NO_CREATURE_SPELLS);
            m_combatWindow->m_heroSubWindows[0]->show();
            g_windowManager->doQuickView(0);
            if (g_config.m_combatArmyInfoLevel)
                return 0;
            m_combatWindow->m_heroSubWindows[0]->unShow();
        }
        return 0;
    }

    if (newIndex == s_wallTargets[7].m_targetHex) {
        viewCastleBallista(1);
        return 0;
    }

    if (newIndex >= 0 && newIndex < COMBAT_GRID_CELLS
            && m_cells[newIndex].m_armySide >= 0) {
        g_mouseManager->setPointer(6, mouseManager::COMBAT_SET);
        viewArmy(m_cells[newIndex].getArmy(), 1);
        resetMouse();
        return 0;
    }
    if (m_fortificationLevel == COMBAT_FORTIFICATION_CASTLE
        && (newIndex == s_wallTargets[0].m_targetHex
            || newIndex == s_wallTargets[6].m_targetHex))
        viewCastleBallista(1);
    return 0;
}

// THE FIVE ORDER CASES all write the same (what, where, how) triple -
// field_3c the order code, field_44 the target hex and field_40 the
// companion hex - and only the melee case has a real second hex
// (field_132d8); the rest park -1 there. Case 1/2's extra arm is the
// wide-stack shift: when the destination cell's field_4b marks it as a
// tail hex, the anchor moves one column against the stack's facing.

VA(0x00476bd0, 0x402)  // dc 0x6db78
void combatManager::doCommand(int command)
{
    army* currentArmy = getCurrentArmy();

    switch (command) {
    case COMBAT_COMMAND_WALK:
    case COMBAT_COMMAND_FLY:
        m_nextAction = 2;
        m_nextActionGridIndex = m_lastCellIndex;
        if (currentArmy->is(creatureDoubleWide) && m_cells[m_lastCellIndex].m_frontMove)
            m_nextActionGridIndex = m_lastCellIndex - (currentArmy->m_facing ? 1 : -1);
        m_nextActionExtra = -1;
        break;

    case COMBAT_COMMAND_SHOOT:
    case COMBAT_COMMAND_SHOOT_PENALTY:
        m_nextAction = 7;
        m_nextActionGridIndex = m_lastCellIndex;
        m_nextActionExtra = -1;
        break;

    case COMBAT_COMMAND_CREATURE_SPELL:
        m_nextAction = 10;
        m_nextActionGridIndex = m_lastCellIndex;
        m_nextActionExtra = -1;
        break;

    case COMBAT_COMMAND_ATTACK:
        m_nextActionGridIndex = m_lastCellIndex;
        m_nextAction = 6;
        m_nextActionExtra = m_lastMoveToIndex;
        break;

    case COMBAT_COMMAND_SPELL_BOOK:
        if (m_creaturePlacement)
            break;
        {
            int spell = viewSpells();
            if (spell == -1)
                break;
            if (m_spellsCast[m_currentSide] && !m_debugNoSpellLimit) {
                normalDialog(g_generalText->getText(
                                 GENERAL_TEXT_COMBAT_SPELL_ALREADY_CAST),
                             1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
                break;
            }
            m_combatWindow->m_heroSubWindows[0]->unShow();
            m_combatWindow->m_heroSubWindows[1]->unShow();
            m_combatWindow->m_creatureSubWindows[0]->unShow();
            m_combatWindow->m_creatureSubWindows[1]->unShow();
            m_combatWindow->m_creatureSubWindows[2]->unShow();
            m_combatWindow->m_creatureSubWindows[3]->unShow();
            g_mouseManager->setPointer(6, mouseManager::COMBAT_SET);
            initiateSpell(spell, 0);
            resetMouse();
        }
        break;

    case COMBAT_COMMAND_VIEW_TOWERS:
        g_mouseManager->setPointer(6, mouseManager::COMBAT_SET);
        viewCastleBallista(0);
        resetMouse();
        break;

    case COMBAT_COMMAND_VIEW_ARMY:
        g_mouseManager->setPointer(6, mouseManager::COMBAT_SET);
        if (m_lastCellIndex < 0 || m_lastCellIndex >= COMBAT_GRID_CELLS)
            break;
        viewArmy(m_cells[m_lastCellIndex].getArmy(), 0);
        resetMouse();
        break;

    case COMBAT_COMMAND_BOMBARD_WALL:
        m_nextAction = 9;
        m_nextActionGridIndex = m_lastCellIndex;
        m_nextActionExtra = -1;
        break;

    case COMBAT_COMMAND_FIRST_AID:
        m_nextAction = 11;
        m_nextActionGridIndex = m_lastCellIndex;
        m_nextActionExtra = -1;
        break;
    }
}

VA(0x00476fe0, 0x2C4)  // dc 0x6de24
void combatManager::showEagleEye(int winningGroup, int dialogTimeout)
{
    hero* winner = m_heroes[winningGroup];
    if (!winner)
        return;

    std::string msg;
    std::vector<type_dialog_resource> rewards;
    type_dialog_resource reward;

    std::set<SpellID>::iterator x =
        m_eagleEyeData[winningGroup].begin();
    while (x != m_eagleEyeData[winningGroup].end()) {
        SpellID spell = *x;
        x++;
        reward.m_resource = VICTORY_DIALOG_SPELL_ROW;
        reward.m_qualifier = spell;
        if (rewards.size() == 0) {
            // General text 222 is the "<hero> learns <spell>" opener;
            // its enum name describes those two arguments.
            msg = formatString(g_generalText->getText(GENERAL_TEXT_EAGLE_EYE_LEARNS_SPELL_FORMAT), winner->m_name,
                                g_spellTraits[spell].m_name);
        } else {
            if (x == m_eagleEyeData[winningGroup].end()
                || rewards.size() == VICTORY_DIALOG_PAGE_SIZE - 1)
                msg += g_generalText->getText(GENERAL_TEXT_LIST_AND);
            else
                msg += DATA_COMPGEN(0x0066032c, listSeparator, ", ");
            msg += g_spellTraits[spell].m_name;
        }
        rewards.push_back(reward);
        if (rewards.size() == VICTORY_DIALOG_PAGE_SIZE
            || x == m_eagleEyeData[winningGroup].end()) {
            msg += DATA_COMPGEN(0x006603ec, saveExtensionDot, ".");
            launchSample(
                formatString(DATA_COMPGEN(0x00670268, pickupSampleFormat,
                                           "pickup%02d.82M"),
                              sRandom(1, 7))
                    .c_str(),
                -1, 3);
            extendedDialog(msg.c_str(), rewards, -1, -1, dialogTimeout);
            rewards.clear();
        }
    }
}

VA(0x004772b0, 0x1BF)  // dc 0x6e0d8
void combatManager::showLootedArtifacts(
    std::vector<type_artifact>& lootedArtifacts, int dialogTimeout)
{
    type_artifact* artifact = lootedArtifacts.begin();
    std::string msg;
    std::vector<type_dialog_resource> rewards;
    type_dialog_resource reward;

    while (artifact != lootedArtifacts.end()) {
        reward.m_resource = VICTORY_DIALOG_ARTIFACT_ROW;
        reward.m_qualifier =
            (static_cast<unsigned short>(artifact->m_extra) << 16)
            | static_cast<unsigned short>(artifact->m_artifactId);
        rewards.push_back(reward);
        artifact++;
        if (artifact == lootedArtifacts.end()
            || rewards.size() == VICTORY_DIALOG_PAGE_SIZE) {
            launchSample(
                formatString(DATA_COMPGEN(0x00670268, pickupSampleFormat,
                                           "pickup%02d.82M"),
                              sRandom(1, 7))
                    .c_str(),
                -1, 3);
            extendedDialog(g_generalText->getText(GENERAL_TEXT_COMBAT_CAPTURED_ARTIFACT), rewards, -1, -1,
                            dialogTimeout);
            rewards.clear();
        }
    }
}

// DC command.cpp:2624 assigns min directly to the defender's mana. Keeping
// that expression recovers the retail operand homes; separate mana/cap
// snapshots leave 99.8293% despite an otherwise identical call sequence.
// Complete darkens the screen after freeArmies; the older port's earlier
// text drawing and extra results-dialog fade/surface setup are absent.
VA(0x00477470, 0x58C)  // dc 0x6e1c8
void combatManager::doVictory(int winningGroup)
{
    int lastAliveSideIndex = 1 - winningGroup;
    if (lastAliveSideIndex == LAST_ALIVE_ATTACKER) {
        if (m_heroes[0])
            m_heroes[0]->removeArtifact(ARTIFACT_HOLY_GRAIL);
    } else if (lastAliveSideIndex == LAST_ALIVE_DEFENDER) {
        if (m_heroes[1])
            m_heroes[1]->removeArtifact(ARTIFACT_HOLY_GRAIL);
    } else if (lastAliveSideIndex == LAST_ALIVE_NEITHER) {
        if (m_heroes[0])
            m_heroes[0]->removeArtifact(ARTIFACT_HOLY_GRAIL);
        if (m_heroes[1])
            m_heroes[1]->removeArtifact(ARTIFACT_HOLY_GRAIL);
    }

    for (int side = 0; side < 2; side++) {
        int survivingTroops = 0;
        int lastAliveSlot = -1;
        for (int slot = 0; slot < 20; slot++) {
            army* stack = &m_armies[side][slot];
            if (stack->m_creatureType != CREATURE_NONE && stack->m_numTroops > 0) {
                lastAliveSlot = slot;
                if (stack->m_numTroopsBattleResurrected > 0)
                    stack->m_numTroops -= stack->m_numTroopsBattleResurrected;
                if (stack->m_numTroops < 0)
                    stack->m_numTroops = 0;
                survivingTroops += stack->m_numTroops;
            }
        }
        if (survivingTroops == 0 && lastAliveSlot != -1)
            m_armies[side][lastAliveSlot].m_numTroops = 1;
    }

    // Necromancy. The raise is per DEAD STACK, capped at that stack's own
    // losses, and the hit-point term is the SMALLER of the dead
    // creature's and the raised creature's - so a necromancer never
    // profits from killing something tougher than a skeleton.
    m_raisedCreatureCount = 0;
    if (winningGroup != -1 && m_heroes[winningGroup]) {
        float necromancyFactor = m_heroes[winningGroup]->getNecromancyFactor(1);
        if (necromancyFactor > 0.0f) {
            m_raisedCreatureType =
                m_heroes[winningGroup]->getNecromancyCreature();
            int raisedHitPoints =
                g_creatureTypeTraits[m_raisedCreatureType].m_hitPoints;
            unsigned char anythingDied = 0;
            for (int slot = 0; slot < 20; slot++) {
                army* stack = &m_armies[lastAliveSideIndex][slot];
                if (stack->m_creatureType == CREATURE_NONE)
                    continue;
                int killed = stack->m_origNumTroops - stack->m_numTroops;
                if (killed <= 0)
                    continue;
                anythingDied = 1;
                int hitPoints =
                    g_creatureTypeTraits[stack->m_creatureType].m_hitPoints;
                if (hitPoints > raisedHitPoints)
                    hitPoints = raisedHitPoints;
                int raised = static_cast<int>(hitPoints * killed
                                              * necromancyFactor
                                              / raisedHitPoints);
                if (raised > killed)
                    raised = killed;
                m_raisedCreatureCount += raised;
            }
            if (anythingDied && m_raisedCreatureCount < 1)
                m_raisedCreatureCount = 1;
        }
    }

    updateArmyGroup(0);
    updateArmyGroup(1);
    if (m_heroes[1]) {
        m_heroes[1]->setPrimarySkill(0, m_originalAttackSkill);
        m_heroes[1]->setPrimarySkill(1, m_originalDefenseSkill);
        m_heroes[1]->setPrimarySkill(2, m_originalPowerSkill);
        if (m_defendingTown) {
            m_heroes[1]->m_mana = min(m_originalMana, m_heroes[1]->m_mana);
        }
    }

    int experience = 0;
    std::vector<type_artifact> lootedArtifacts;
    if (winningGroup != -1) {
        if (m_heroes[winningGroup]) {
            calculateGainedExperience(winningGroup, &experience);
            // The LOCAL human's experience is awarded later, beside the
            // results window; a remote or AI winner gets it here.
            if (!m_sideIsLocalHuman[winningGroup])
                m_heroes[winningGroup]->giveExperience(experience, 1, 1);
            raiseSkeletons(winningGroup);
            learnSpellFromEagleEye(winningGroup);
            lootDeadHero(winningGroup, lootedArtifacts);
        }
        if (m_heroes[winningGroup])
            m_heroes[winningGroup]->applyBattleWinTemps();
        if (m_heroes[lastAliveSideIndex])
            m_heroes[lastAliveSideIndex]->applyBattleLossTemps();
    } else {
        if (m_heroes[0])
            m_heroes[0]->applyBattleLossTemps();
        if (m_heroes[1])
            m_heroes[1]->applyBattleLossTemps();
    }

    freeArmies();
    if (!static_cast<const combatManager*>(this)->isQuickCombat())
        m_combatWindow->combatMessage("", 0, 0);
    g_mouseManager->m_noChangePointer = 0;
    g_mouseManager->setPointer(6, mouseManager::COMBAT_SET);
    g_mouseManager->showPointer(false);
    if (!static_cast<const combatManager*>(this)->isQuickCombat()) {
        g_windowManager->m_screenBitmap->darken(0, 0, 800, 600);
        g_windowManager->updateScreen(0, 0, 800, 600);
    }

    int dialogtimeout = 15000;
    if (g_remoteOn && !g_thisNetGotAdventureControl) {
        g_dialogDeadline = GameTime::get() + dialogtimeout;
    } else {
        g_dialogDeadline = 0;
        dialogtimeout = 0;
    }
    if (g_goSolo)
        g_dialogDeadline = GameTime::get() + 2000;

    if (winningGroup != -1 && m_playerIds[winningGroup] != -1
        && g_game->isLocalHuman(m_playerIds[winningGroup])) {
        {
            TCombatResultsWindow resultsWindow(
                m_heroes[0], m_heroes[1], winningGroup, winningGroup,
                m_defendingTown != 0, experience);
            g_soundManager->startMP3(
                g_combatResultMusic[g_combatResult], 1, 1);
            resultsWindow.doModal();
        }
        if (m_heroes[winningGroup]) {
            m_heroes[winningGroup]->giveExperience(
                experience, !m_sideIsLocalHuman[winningGroup], 1);
            showEagleEye(winningGroup, dialogtimeout);
            showLootedArtifacts(lootedArtifacts, dialogtimeout);
        }
    } else if (!g_goSolo) {
        TCombatResultsWindow resultsWindow(
            m_heroes[0], m_heroes[1], lastAliveSideIndex, winningGroup,
            m_defendingTown != 0, 0);
        g_soundManager->startMP3(
            g_combatResultMusic[g_combatResult], 1, 1);
        resultsWindow.doModal();
    }

    g_dialogDeadline = 0;
}

VA(0x00477a00, 0xB2)  // dc 0x6e898
long combatManager::getSurrenderCost()
{
    long cost = 0;
    int side = m_currentSide;

    for (int slot = 0; slot < 20; ++slot) {
        army* currentArmy = &m_armies[side][slot];
        if (currentArmy->isActive()
            && !currentArmy->is(creatureSummoned)
            && currentArmy->m_numTroops
                > currentArmy->m_numTroopsBattleResurrected) {
            cost += (currentArmy->m_numTroops
                     - currentArmy->m_numTroopsBattleResurrected)
                  * g_creatureTypeTraits[currentArmy->m_creatureType].m_cost[6];
        }
    }

    return static_cast<long>(m_heroes[side]->getSurrenderCostFactor()
                             * static_cast<float>(cost / 2));
}

// E:\gamedcs\command.cpp:2800. Keep the Dreamcast-proven helper boundary:
// Complete expands this sole caller into ProcessCombatMsg and therefore has
// no standalone retail body. Complete also omits the older port's FullUpdate
// after the modal dialog, as it does in the neighbouring retreat path.

inline int combatManager::doSurrender()
{
    g_surrenderCost = getSurrenderCost();
    sprintf(g_text, g_generalText->getText(GENERAL_TEXT_COMBAT_SURRENDER_OFFER_FORMAT),
            m_heroes[1 - m_currentSide]->m_name, g_surrenderCost);
    normalDialog(g_text, 2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    return g_windowManager->m_dialogReturn == DIALOG_RETURN_ACCEPT;
}

// Complete Mac retains this tower-turn helper at code 0:8627c. Its sole
// caller is main, where Windows expands the decision between first aid and
// catapult automation. An ordinary definition leaves an extra VC6 call;
// this inline definition restores main's 42-call sequence and 77-block CFG.
inline unsigned char combatManager::automateTower()
{
    if (m_fortificationLevel < COMBAT_FORTIFICATION_CITADEL)
        return 0;

    army* currentArmy = getCurrentArmy();
    if (currentArmy->m_creatureType != CREATURE_ARROW_TOWER)
        return 0;

    int wall;
    switch (currentArmy->m_gridIndex) {
    case COMBAT_HEX_LOWER_TOWER:
        wall = 13;
        break;
    case COMBAT_HEX_KEEP:
        wall = 14;
        break;
    case COMBAT_HEX_UPPER_TOWER:
        wall = 5;
        break;
    }

    if (m_wallStrength[wall] == 0) {
        currentArmy->m_monInfo.m_attributes |= creatureImmobilized;
        m_nextAction = 12;
        return 1;
    }
    if (static_cast<const combatManager*>(this)->isQuickCombat()
            || isComputerAction(getCurrentArmy())) {
        unnamed465f20();
        resetMouse();
        return 1;
    }
    return 0;
}

VA(0x00477ac0, 0x95)  // dc 0x6ea10
void combatManager::checkChangeSelector()
{
    if (static_cast<const combatManager*>(this)->isQuickCombat())
        return;

    army* currentArmy = getCurrentArmy();
    if (m_lastMovedArmy == currentArmy)
        return;

    updateGrid(0, 1);
    m_lastMovedArmy = currentArmy;
    if (!currentArmy->is(creatureImmobilized)
            && currentArmy->m_currFrameType != cs_wait) {
        currentArmy->m_currFrameType = cs_wait;
        currentArmy->m_currFrameIndex = 0;
    }
    drawFrame(1, 0, 0, 0, 1, 0);
}

VA(0x00477b60, 0xB6)  // dc 0x6eb18
void combatManager::turnOffSelector(unsigned char drawIt)
{
    if (!m_lastMovedArmy)
        return;

    if (drawIt) {
        resetLimitCreature();
        if (!m_lastMovedArmy->is(creatureImmobilized))
            markCreatureEffect(m_lastMovedArmy->getOwningSide(),
                               m_lastMovedArmy->m_bitIndex);
    }

    m_lastMovedArmy = 0;
    if (drawIt)
        drawFrame(1, 1, 0, 0, 1, 0);
}

VA(0x00477c20, 0x1E4)  // dc 0x6ebbc
void combatManager::checkChangeHighlighter(int currentIndex)
{
    if (static_cast<const combatManager*>(this)->isQuickCombat())
        return;
    if (m_battleOver)
        return;

    army* currentArmy;
    if (validHex(currentIndex) && m_cells[currentIndex].hasArmy())
        currentArmy = m_cells[currentIndex].getArmy();
    else
        currentArmy = 0;

    if (currentArmy && currentArmy->m_gridIndex == m_highlighterIndex)
        return;

    resetLimitCreature();
    if (m_highlighterOn) {
        if (m_cells[m_highlighterIndex].hasArmy())
            markCreatureEffect(m_cells[m_highlighterIndex].m_armySide,
                               m_cells[m_highlighterIndex].m_armySlot);
        m_highlighterOn = 0;
        m_highlighterIndex = -1;
    }

    if (currentArmy) {
        m_highlighterIndex = currentArmy->m_gridIndex;
        m_highlighterOn = 1;
        if (currentArmy->m_stdIcon->isValidSeq(cs_fidget)
                && currentArmy->m_currFrameType != cs_fidget) {
            currentArmy->m_currFrameType = cs_fidget;
            currentArmy->m_currFrameIndex = 0;
        } else if (currentArmy->m_currFrameType != cs_wait) {
            currentArmy->m_currFrameType = cs_wait;
            currentArmy->m_currFrameIndex = 0;
        }
        markCreatureEffect(currentArmy->getOwningSide(), currentArmy->m_bitIndex);
    }

    drawFrame(1, 1, 0, 0, 1, 0);
}

VA(0x00477e10, 0xC2)  // dc 0x6ed18
void combatManager::turnOffHighlighter(unsigned char drawIt)
{
    if (!m_highlighterOn)
        return;

    if (drawIt) {
        resetLimitCreature();
        markCreatureEffect(m_cells[m_highlighterIndex].m_armySide,
                           m_cells[m_highlighterIndex].m_armySlot);
    }

    m_highlighterOn = 0;
    m_highlighterIndex = -1;
    if (drawIt)
        drawFrame(1, 1, 0, 0, 1, 0);
}

// E:\gamedcs\command.cpp:3038. Retail's command-order bracket leaves one
// 997-byte body between TurnOffHighlighter and GetControl, exactly where the
// Dreamcast roster places CheckGetAIMove. The direct call at 0x477f3d lands
// in the Dreamcast AICheckRetreat statement slot. Complete inlines the
// already exact get_surrender_cost body here, while retaining its standalone
// copy at 0x477a00.
// RESIDUAL (94.43355%): the 53-block CFG is exact (49 blocks also have the
// exact instruction count; four are size-only), including all 35 branches
// and five returns. The remaining 110 register-visible slots are confined to
// the two twenty-stack value loops; retail homes one extra four-byte scratch
// and binds the row walk to EDI/ECX where this build uses EDX/EDI. A named
// side is codegen-inert; explicit current-hero locals score 87.19%/80.53%;
// and an earlier IsActive-call probe scored 84.86%. The current source keeps
// IsActive (DC line 3085); the text lookups at 3058/3105 use the underlying
// getText accessor here. FullUpdate after each dialog is DC-only here:
// retail continues directly to the response checks without that redraw.
// Keeping a named army-row base is the best measured natural spelling.
// Countdown sweep 2026-09-06: retail computes ONE `&armies[currentSide][0]
// .numTroops` (edi at fn+0x4b2, disp 0x5518 folded into the lea) and shares
// it between this loop and the inlined get_surrender_cost walk, and homes
// `heroes[currentSide]` at [ebp-0x20] from the enclosing guard; ours
// recomputes both.  Two spellings measured against 94.4335: dropping the
// named row base so both loops spell `&armies[currentSide][slot]` scores
// 94.3892, and the countdown pointer walk
// `army* p = armies[currentSide]; for (int slot = 20; slot--; p++)` is
// BYTE-FLAT.  The wall is the cross-inline CSE of the row base, not the
// loop form.
VA(0x00477ee0, 0x3E5)  // exhaustive command order-map + call graph, dc 0x6ee60
void combatManager::checkGetAIMove()
{
    unsigned char isHuman = 0;
    if (m_playerIds[m_currentSide] >= 0
            && g_game->isHuman(m_playerIds[m_currentSide]))
        isHuman = 1;

    if (!m_creaturePlacement && aiCheckRetreat()) {
        unsigned char proceed = 1;
        if (isHuman) {
            if (m_autoRetreatOn) {
                std::string result = formatString(
                    (*g_generalText)[GENERAL_TEXT_COMBAT_RETREAT_OVERWHELMED_FORMAT], m_heroes[m_currentSide]->m_name);
                normalDialog(result.c_str(), 2, -1, -1, -1, 0, -1, 0,
                             -1, 0, -1, 0);
                if (g_windowManager->m_dialogReturn
                        == DIALOG_RETURN_DECLINE) {
                    proceed = 0;
                    m_autoRetreatOn = proceed;
                }
            } else {
                proceed = 0;
            }
        }

        if (proceed) {
            long combatValue = 0;
            if (m_heroes[1 - m_currentSide] && m_heroes[m_currentSide]) {
                army* currentArmies = m_armies[m_currentSide];
                for (int slot = 0; slot < 20; ++slot) {
                    army* currentArmy = &currentArmies[slot];
                    if (currentArmy->isActive()) {
                        combatValue +=
                            g_creatureTypeTraits[currentArmy->m_creatureType].m_cost[6]
                            * currentArmy->m_numTroops;
                    }
                }

                g_surrenderCost = getSurrenderCost();
                if ((!m_defendingTown || m_currentSide == 0)
                        && g_game->m_players[m_playerIds[m_currentSide]].m_resources[6]
                            >= g_surrenderCost + 2500
                        && combatValue > g_surrenderCost + 2500) {
                    std::string msg;
                    if (isHuman) {
                        msg = formatString((*g_generalText)[GENERAL_TEXT_COMBAT_SAVE_ARMY_PROMPT_FORMAT],
                                            m_heroes[m_currentSide]->m_name,
                                            g_surrenderCost);
                        normalDialog(msg.c_str(), 2, -1, -1, 6,
                                     g_surrenderCost, -1, 0,
                                     -1, 0, -1, 0);
                        if (g_windowManager->m_dialogReturn
                                == DIALOG_RETURN_ACCEPT) {
                            m_nextAction = 5;
                            m_nextActionExtra = g_surrenderCost;
                            return;
                        }
                    }
                }
            }

            m_nextAction = 4;
            return;
        }
    }

    doCompAI(m_currentSide);
}

// E:\gamedcs\command.cpp:3131
VA(0x004782d0, 0x5B5)  // exhaustive command order-map + body, dc 0x6f198
void combatManager::getControl()
{
    m_lastCellIndex = -1;
    m_lastCommand = -99;

    if (!m_autoCombatOn && !g_goSolo)
        g_inputManager->flush();

    if (m_status == STATUS_ACTIVE
            && !static_cast<const combatManager*>(this)->isQuickCombat())
        g_mouseManager->setPointer(6, mouseManager::COMBAT_SET);

    checkChangeSelector();

    // Retail keeps a zero in EDI for the rest of this large state update;
    // spelling the repeated integer zero once gives VC6 that allocation.
    int zero = 0;

    if (g_remoteOn != zero
            && m_playerIds[0] != -1
            && m_playerIds[1] != -1
            && g_game->isHuman(m_playerIds[1])
            && (g_game->isHuman(m_playerIds[0]) || m_playerIds[1] != zero)
            && m_playerIds[m_currentSide] >= zero
            && g_game->isHuman(m_playerIds[m_currentSide])
            && !g_game->isLocalHuman(m_playerIds[m_currentSide]))
        m_thisNetHasControl = zero;
    else
        m_thisNetHasControl = 1;

    if (m_combatWindow && m_combatWindow->m_controlSubWindow) {
        if ((m_autoCombatOn != zero || g_goSolo)
                && isComputerAction()) {
            static_cast<type_combat_sub_window*>(
                m_combatWindow->m_controlSubWindow)->disableAllButtons();
            if (m_autoCombatOn && m_sideIsAi[m_currentSide]) {
                m_combatWindow->widgetClearStatus(
                    0x7d4, 0x1000);
                m_combatWindow->widgetSetStatus(
                    0x7d4, 0x10);
            }
        } else if (m_playerIds[m_currentSide] >= zero
                && g_game->isLocalHuman(m_playerIds[m_currentSide])) {
            m_combatWindow->broadcastMessage(
                MESSAGE_WIDGET, 0x0d,
                0x7d0, m_playerIds[m_currentSide]);

            if (!m_heroes[1 - m_currentSide] || !m_heroes[m_currentSide]
                    || (m_defendingTown && m_currentSide == 1))
                m_combatWindow->widgetSetStatus(
                    0x7d1, 0x1000);
            else
                m_combatWindow->widgetClearStatus(
                    0x7d1, 0x1000);

            if (!m_heroes[m_currentSide]
                    || (m_currentSide == 1 && m_defendingTown
                        && (m_defendingTown->m_type != TOWN_STRONGHOLD
                            || !m_defendingTown->hasBuilding(
                                SPECIAL_BUILDING_ID, true))))
                m_combatWindow->widgetSetStatus(
                    0x7d2, 0x1000);
            else
                m_combatWindow->widgetClearStatus(
                    0x7d2, 0x1000);

            m_combatWindow->widgetClearStatus(
                0x7d3, 0x1000);
            m_combatWindow->widgetClearStatus(
                0x7d4, 0x10);

            if (m_sideIsAi[0] && m_sideIsAi[1])
                m_combatWindow->widgetSetStatus(
                    0x7d4, 0x1000);
            else
                m_combatWindow->widgetClearStatus(
                    0x7d4, 0x1000);

            if (m_creaturePlacement) {
                m_combatWindow->widgetClearStatus(
                    0x8fc, 0x1000);
                m_combatWindow->widgetClearStatus(
                    0x7802, 0x1000);
            } else {
                m_combatWindow->widgetClearStatus(
                    0x7d6, 0x1000);
                m_combatWindow->widgetClearStatus(
                    0x7d7, 0x1000);

                if ((!m_spellsCast[m_currentSide] || m_debugNoSpellLimit)
                        && m_heroes[m_currentSide]
                        && m_heroes[m_currentSide]->isWieldingArtifact(zero)
                        && !m_creaturePlacement)
                    m_combatWindow->widgetClearStatus(
                        0x7d8, 0x1000);
                else
                    m_combatWindow->widgetSetStatus(
                        0x7d8, 0x1000);

                if (isInSecondPhase() || m_creaturePlacement)
                    m_combatWindow->widgetSetStatus(
                        0x7d9, 0x1000);
                else
                    m_combatWindow->widgetClearStatus(
                        0x7d9, 0x1000);

                if (m_creaturePlacement)
                    m_combatWindow->widgetSetStatus(
                        0x7d9, 0x1000);
                else
                    m_combatWindow->widgetClearStatus(
                        0x7da, 0x1000);
            }

            m_combatWindow->drawWindow(
                zero, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
            g_windowManager->updateScreen(zero, 556, 800, 44);
        } else {
            static_cast<type_combat_sub_window*>(
                m_combatWindow->m_controlSubWindow)->disableAllButtons();
        }
    }

    resetMouse();
    m_nextAction = zero;
    doSpellAI();
}

VA(0x00478890, 0x6E)  // dc 0x6f5f4
void combatManager::resetMouse()
{
    if (static_cast<const combatManager*>(this)->isQuickCombat())
        return;

    if (m_thisNetHasControl && m_playerIds[m_currentSide] >= 0
            && g_game->isHuman(m_playerIds[m_currentSide])) {
        m_lastCellIndex = -1;
        if (m_combatWindow)
            m_combatWindow->clearCombatMessages();
        g_inputManager->forceMouseMove();
        return;
    }

    g_mouseManager->setPointer(6, mouseManager::COMBAT_SET);
}

VA(0x00478900, 0x290)  // dc 0x6f664
unsigned char combatManager::processMoveThenAttack(message* msg)
{
    army* currentArmy = getCurrentArmy();
    int oldGridIndex = currentArmy->m_gridIndex;
    int oldFacing = currentArmy->m_facing;

    resetCyclingCreatures();
    currentArmy->m_joustBonus = 0;
    if (m_nextActionExtra != -1 && oldGridIndex != m_nextActionExtra) {
        if (currentArmy->moveTo(m_nextActionExtra, 0)) {
            if (!currentArmy->is(creatureImmobilized))
                currentArmy->attackHex(m_nextActionGridIndex, 0);
        }
    } else {
        if (!currentArmy->is(creatureImmobilized))
            currentArmy->attackHex(m_nextActionGridIndex, 0);
        memset(m_obstacleAttackVisited, 0, COMBAT_GRID_CELLS);
        currentArmy->checkObstacleAttacks(0);
    }

    currentArmy->m_monInfo.m_attributes |= creatureDone;
    currentArmy->m_joustBonus = 0;
    // Mac 0x86ec0..0x86f00 expands isIncapacitated in this return step.
    if (m_nextActionExtra != -1 && oldGridIndex != m_nextActionExtra
            && (currentArmy->m_creatureType == army::ARMY_CREATURE_HARPY
                || currentArmy->m_creatureType
                       == army::ARMY_CREATURE_HARPY_HAG)
            && !currentArmy->is(creatureImmobilized)
            && !currentArmy->isIncapacitated()) {
        currentArmy->moveTo(oldGridIndex, 0);
    }

    if (oldFacing != currentArmy->m_facing
            && !currentArmy->is(creatureImmobilized)) {
        currentArmy->setupAnimation();
        currentArmy->turn(1);
    }

    if (checkWin(msg)) {
        g_processingCombatAction = 0;
        resetMouse();
        return 1;
    }

    checkApplyGoodMorale(m_actingSide, m_actingSlot);
    resetCycleTimers();
    return 0;
}

VA(0x00478b90, 0x1E5)  // dc 0x6f824
void combatManager::processFirstAid(army* currentArmy)
{
    if (validHex(m_nextActionGridIndex)) {
        army* targetArmy = m_cells[m_nextActionGridIndex].getArmy();
        int maximum = sRandom(
            1, static_cast<int>(
                   currentArmy->getController()->getFirstAidFactor()
                   * 100.0f));
        int result = targetArmy->m_topCreatureDamage;
        result = min(maximum, result);
        targetArmy->m_topCreatureDamage -= result;
        currentArmy->m_monInfo.m_attributes |= creatureDone;

        if (!static_cast<const combatManager*>(this)->isQuickCombat()) {
            SAMPLE2 sample = loadPlaySample(
                DATA_COMPGEN(0x00660a94, regenerSampleName,
                             "Regener.wav"));
            std::string text = formatString(
                g_generalText->getText(GENERAL_TEXT_FIRST_AID_HEAL_FORMAT), currentArmy->getName(),
                targetArmy->getName(), result);
            m_combatWindow->combatMessage(text.c_str(), 1, 0);
            spellEffect(eSpellEffectRegeneration, targetArmy, 100, 0);
            waitEndSample(sample, -1);
        }
    }
}

// E:\gamedcs\command.cpp:3461. The DC statement table fixes the local roles,
// switch domain and source calls. Retail independently fixes all twelve
// pending-action arms and shows that VC6 expanded ResetMouse,
// ResetCycleTimers and CheckChangeSelector into this body.
// Keep GetName -> GetArmyName, timer/selector calls, DC3653's max and the
// text-resource getters. Residual 99.7272%: the wait-arm string constructor
// retains _Tidy where retail expands it. Naming the defend-bonus result is
// flat; naming its percentage input is worse. Neither justifies flattening
// max or changing the DC string lifetimes.
// DC3625's extra FullUpdate in the surrender-error arm is absent in retail.
VA(0x00478d80, 0x1054)  // anchor-callee exhaustive + single-fn gap, dc 0x6f984
int combatManager::processNextAction(message& msg, unsigned char automaticTurn)
{
    if (!static_cast<const combatManager*>(this)->isQuickCombat()) {
        m_combatWindow->clearCombatMessages();
        m_combatWindow->m_heroSubWindows[0]->unShow();
        m_combatWindow->m_heroSubWindows[1]->unShow();
        m_combatWindow->m_creatureSubWindows[0]->unShow();
        m_combatWindow->m_creatureSubWindows[1]->unShow();
        m_combatWindow->m_creatureSubWindows[2]->unShow();
        m_combatWindow->m_creatureSubWindows[3]->unShow();
    }

    g_processingCombatAction = 1;
    if (!static_cast<const combatManager*>(this)->isQuickCombat()) {
        if (m_nextAction)
            g_mouseManager->setPointer(6, mouseManager::COMBAT_SET);
        updateMouseGrid(-1, 1);
        memset(m_curDrawGridShade, 0, sizeof(m_curDrawGridShade));
        if (updateGrid(0, 0))
            drawFrame(1, 0, 0, 0, 1, 0);
    }

    if (m_thisNetHasControl && g_remoteOn
            && m_playerIds[0] >= 0 && m_playerIds[1] >= 0
            && g_game->isHuman(m_playerIds[1])
            && g_game->isHuman(m_playerIds[0])) {
        int seed = GameTime::get();
        sRand(seed);
        CCombatMainMsg combatMsg(m_nextAction, m_nextActionExtra, m_nextActionGridIndex, m_nextActionGridIndex2,
                                 seed);
        g_logFile.log(
            DATA_COMPGEN(0x00670278, sendingCombatActionLog,
                         "Sending action [%d]--> [%d,%d,%d]"),
            m_nextAction, m_nextActionExtra, m_nextActionGridIndex, m_nextActionGridIndex2);
        if (!transmitRemoteData(&combatMsg,
                                g_combatControlNetPos[1 - m_currentSide],
                                false, true))
            shutDown(0);
    }

    army* currentArmy = getCurrentArmy();
    int returnValue = 0;

    if (checkWin(&msg)) {
        g_processingCombatAction = 0;
        resetMouse();
        return 1;
    }

    switch (m_nextAction) {
    case g_combatActionCastHeroSpell:
        m_anyActionTaken = 1;
        resetCyclingCreatures();
        // The pending-action payload is an int slot (DC CCombatMainMsg
        // m_nextActionExtra) crossing into CastSpell's DC-proven SpellID.
        castSpell(static_cast<SpellID>(m_nextActionExtra) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */,
                  m_nextActionGridIndex, 0, m_nextActionGridIndex2, 0, 3);
        if (currentArmy->m_numTroops <= 0)
            returnValue = 1;
        resetCycleTimers();
        break;

    case g_combatActionMove:
        m_anyActionTaken = 1;
        resetCyclingCreatures();
        currentArmy->moveTo(m_nextActionGridIndex, 1);
        currentArmy->m_joustBonus = 0;
        currentArmy->m_monInfo.m_attributes |= creatureDone;
        if (checkWin(&msg)) {
            g_processingCombatAction = 0;
            resetMouse();
            return 2;
        }
        checkApplyGoodMorale(m_actingSide, m_actingSlot);
        returnValue = 1;
        resetCycleTimers();
        break;

    case AI_ORDER_SHOOT:
        m_anyActionTaken = 1;
        resetCyclingCreatures();
        currentArmy->attackHex(m_nextActionGridIndex, 1);
        currentArmy->m_monInfo.m_attributes |= creatureDone;
        if (checkWin(&msg)) {
            g_processingCombatAction = 0;
            resetMouse();
            return 2;
        }
        if (currentArmy->m_creatureType != CREATURE_ARROW_TOWER) {
            checkApplyGoodMorale(m_actingSide, m_actingSlot);
            memset(m_obstacleAttackVisited, 0, COMBAT_GRID_CELLS);
            currentArmy->checkObstacleAttacks(0);
        }
        returnValue = 1;
        resetCycleTimers();
        break;

    case g_combatActionCastCreatureSpell:
        m_anyActionTaken = 1;
        resetCyclingCreatures();
        currentArmy->castSpell(m_nextActionGridIndex);
        currentArmy->m_monInfo.m_attributes |= creatureDone;
        checkApplyGoodMorale(m_actingSide, m_actingSlot);
        returnValue = 1;
        resetCycleTimers();
        break;

    case AI_ORDER_MOVE_AND_ATTACK:
        m_anyActionTaken = 1;
        if (processMoveThenAttack(&msg))
            return 2;
        returnValue = 1;
        break;

    case g_combatActionRetreat:
        if ((m_heroes[0] && m_heroes[0]->isWieldingArtifact(125))
                || (m_heroes[1] && m_heroes[1]->isWieldingArtifact(125))) {
            sprintf(g_text, (*g_generalText)[GENERAL_TEXT_SHACKLES_PREVENT_RETREAT_FORMAT],
                    m_heroes[m_currentSide]->m_name);
            normalDialog(g_text, 1, -1, -1, -1, 0, -1, 0,
                         -1, 0, -1, 0);
        } else {
            m_anyActionTaken = 1;
            m_sideRetreated[m_currentSide] = 1;
            resetCycleTimers();
        }
        break;

    case g_combatActionSurrender:
        if ((m_heroes[0] && m_heroes[0]->isWieldingArtifact(125))
                || (m_heroes[1] && m_heroes[1]->isWieldingArtifact(125))) {
            sprintf(g_text, (*g_generalText)[GENERAL_TEXT_SHACKLES_PREVENT_SURRENDER_FORMAT],
                    m_heroes[m_currentSide]->m_name);
            normalDialog(g_text, 1, -1, -1, -1, 0, -1, 0,
                         -1, 0, -1, 0);
        } else {
            m_anyActionTaken = 1;
            m_sideSurrendered[m_currentSide] = 1;
            g_game->m_players[m_playerIds[m_currentSide]].m_resources[6] -= m_nextActionExtra;
            g_game->m_players[m_playerIds[1 - m_currentSide]].m_resources[6] += m_nextActionExtra;
            resetCycleTimers();
        }
        break;

    case g_combatActionDefend:
        if (!currentArmy->is(creatureDone | creatureDefending)) {
            currentArmy->m_monInfo.m_attributes |= creatureDone;
            if (!m_creaturePlacement && !currentArmy->is(creatureSiegeWeapon)) {
                std::string message;
                currentArmy->m_monInfo.m_attributes |= creatureDefending;
                currentArmy->m_defendBonus = max(
                    currentArmy->m_monInfo.m_defenseSkill * 20 / 100, 1);

                if (currentArmy->m_numTroops == 1)
                    message = formatString((*g_generalText)[GENERAL_TEXT_COMBAT_DEFEND_ONE_FORMAT],
                                            currentArmy->getName(),
                                            currentArmy->m_defendBonus);
                else
                    message = formatString((*g_generalText)[GENERAL_TEXT_COMBAT_DEFEND_MANY_FORMAT],
                                            currentArmy->getName(),
                                            currentArmy->m_defendBonus);
                m_combatWindow->combatMessage(message.c_str(), 1, 0);
            } else {
                currentArmy->m_defendBonus = 0;
            }
            currentArmy->m_monInfo.m_defenseSkill += currentArmy->m_defendBonus;
        }
        memset(m_obstacleAttackVisited, 0, COMBAT_GRID_CELLS);
        currentArmy->checkObstacleAttacks(0);
        returnValue = 1;
        break;

    case g_combatActionWait: {
        currentArmy->m_monInfo.m_attributes |= creatureWaiting;
        if (!m_creaturePlacement) {
            std::string message;
            if (currentArmy->m_numTroops == 1)
                message = formatString((*g_generalText)[GENERAL_TEXT_COMBAT_WAIT_ONE_FORMAT],
                                        currentArmy->getName());
            else
                message = formatString((*g_generalText)[GENERAL_TEXT_COMBAT_WAIT_MANY_FORMAT],
                                        currentArmy->getName());
            m_combatWindow->combatMessage(message.c_str(), 1, 0);
        }
        memset(m_obstacleAttackVisited, 0, COMBAT_GRID_CELLS);
        currentArmy->checkObstacleAttacks(0);
        returnValue = 1;
        break;
    }

    case g_combatActionAttackWall:
        resetCyclingCreatures();
        m_anyActionTaken = 1;
        currentArmy->attackWall(m_nextActionGridIndex);
        currentArmy->m_monInfo.m_attributes |= creatureDone;
        checkApplyGoodMorale(m_actingSide, m_actingSlot);
        returnValue = 1;
        resetCycleTimers();
        break;

    case g_combatActionFirstAid:
        processFirstAid(currentArmy);
        returnValue = 1;
        break;

    case AI_ORDER_NONE:
        currentArmy->m_monInfo.m_attributes |= creatureDone;
        returnValue = 1;
        break;
    }

    m_nextAction = 0;
    if (checkWin(&msg)) {
        g_processingCombatAction = 0;
        resetMouse();
        return 2;
    }

    testRaiseDoor();
    if (returnValue) {
        while (!nextArmy(1))
            resetRound();
    }
    checkChangeSelector();
    updateArmyLuckAndMorale();
    g_processingCombatAction = 0;
    resetMouse();
    return 1;
}

VA(0x00479de0, 0x14F)  // dc 0x701b0
void combatManager::resetCyclingCreatures()
{
    int cyclingCreatures = 0;
    for (int side = 0; side < 2; side++) {
        for (int slot = 0; slot < m_numArmies[side]; slot++) {
            army* stack = &m_armies[side][slot];
            if (!stack->is(creatureImmobilized)
                    && stack->m_currFrameType == cs_fidget) {
                cyclingCreatures++;
                markCreatureEffect(side, slot);
            }
        }
    }

    if (cyclingCreatures) {
        computeMaxExtent();
        for (int side = 0; side < 2; side++) {
            for (int slot = 0; slot < m_numArmies[side]; slot++) {
                army* stack = &m_armies[side][slot];
                if (!stack->is(creatureImmobilized)) {
                    stack->m_currFrameType = cs_wait;
                    stack->m_currFrameIndex = 0;
                    stack->m_lastFidgetTime = GameTime::get();
                }
            }
        }
        m_cmbtHeroLastFidgetTime[0] =
            m_cmbtHeroLastFidgetTime[1] = GameTime::get();
        drawFrame(1, 1, 0, 0, 1, 0);
    }
}

VA(0x00479f30, 0x8B)  // dc 0x702bc
void combatManager::resetCycleTimers()
{
    unsigned long now = GameTime::get();
    m_cmbtHeroLastFidgetTime[0] = now;
    m_cmbtHeroLastFidgetTime[1] = now;

    for (int side = 0; side < 2; side++) {
        for (int slot = 0; slot < m_numArmies[side]; slot++) {
            army* stack = &m_armies[side][slot];
            if (stack->m_monFrameInfo.m_fidgetFrequency > 51) {
                stack->m_lastFidgetTime =
                    now + 2 * random(50, stack->m_monFrameInfo.m_fidgetFrequency)
                        - stack->m_monFrameInfo.m_fidgetFrequency;
            }
        }
    }
}

// Original: combatManager::SetCombatViewArmy; command.cpp:3819, dc 0x70398.
// Complete ProcessCombatMsg's F5 arm (0x474d80) expands this preference
// write, frame refresh and WritePrefs sequence.
void combatManager::setCombatViewArmy(int newCombatViewArmy)
{
    g_config.m_combatArmyInfoLevel = newCombatViewArmy;
    drawFrame(1, 0, 0, 0, 1, 0);
    writePrefs();
}

VA(0x00479fc0, 0x131)  // dc 0x703c0
void combatManager::setCombatGrid(int combatShowEntireGrid,
                                  int combatShowMouseHex,
                                  int combatGridLevel,
                                  unsigned char drawItNow)
{
    if (g_config.m_showCombatGrid == combatShowEntireGrid
            && g_config.m_showCombatMouseHex == combatShowMouseHex
            && g_config.m_combatShadeLevel == combatGridLevel)
        return;

    updateMouseGrid(-1, 0);
    g_config.m_showCombatGrid = combatShowEntireGrid;
    g_config.m_showCombatMouseHex = combatShowMouseHex;
    g_config.m_combatShadeLevel = combatGridLevel;

    m_backgroundDrawn = 0;
    if (combatGridLevel)
        setupGridForArmy(getCurrentArmy());
    if (drawItNow)
        drawFrame(1, 0, 0, 0, 1, 0);
    resetMouse();
    writePrefs();
}

// THE FIZZLE TAIL. combatManager::drawbridgeBounds is not
// drawbridge-only: ComputeMaxExtent refreshes the same four dwords and
// AddArmy hands them to the window manager as (left, top,
// right-left+1, bottom-top+1) - which is what fixes their roles as a
// left/top/right/bottom rectangle. The save/redraw/fizzle triple around
// DrawFrame(0,0,0,0,1,0) is what makes a mid-combat summon appear.
VA(0x0047a100, 0x1CD)  // dc 0x70474
army* combatManager::addArmy(int side, int monType, int monQty,
                             int gridIndex, int setAttributes,
                             int fizzleItIn)
{
    long replaced = 0;
    int slot = -1;
    { for (long candidate = 0; candidate < 20; candidate++) {
        const army* stack = &m_armies[side][candidate];
        if (stack->m_creatureType == -1) {
            slot = candidate;
            break;
        }
        if (stack->m_numTroops == 0
                && stack->is(creatureImmobilized) && stack->is(creatureSummoned)) {
            slot = candidate;
            replaced = 1;
            break;
        }
    } }
    if (slot == -1 || m_cells[gridIndex].m_armySide >= 0)
        return 0;

    army* newArmy = &m_armies[side][slot];
    newArmy->init(monType, monQty, m_heroes[side], side, slot, gridIndex,
                  -1);
    newArmy->loadResources();
    newArmy->m_monInfo.m_attributes |= setAttributes;
    if (!replaced)
        m_numArmies[side]++;

    if (fizzleItIn
            && !static_cast<const combatManager*>(this)->isQuickCombat()) {
        resetLimitCreature();
        markCreatureEffect(side, slot);
        computeMaxExtent();
        g_windowManager->saveFizzleSourceX(
            m_drawbridgeBounds.m_minX, m_drawbridgeBounds.m_minY,
            m_drawbridgeBounds.width(),
            m_drawbridgeBounds.height());
        drawFrame(0, 0, 0, 0, 1, 0);
        g_windowManager->fizzleForwardX(
            m_drawbridgeBounds.m_minX, m_drawbridgeBounds.m_minY,
            m_drawbridgeBounds.width(),
            m_drawbridgeBounds.height(), 75);
    }
    return newArmy;
}

VA(0x0047a2d0, 0xA7)  // dc 0x70650
std::string combatManager::getTowerString(TWallSection wall, long archers,
                                             long skill) const
{
    if (m_wallStrength[wall] <= 0) {
        return formatString(
            g_generalText->getText(GENERAL_TEXT_COMBAT_WALL_DESTROYED_FORMAT),
            s_wallTraits[m_defendingTown->m_type][wall].m_name);
    }

    return formatString(
        g_generalText->getText(GENERAL_TEXT_COMBAT_WALL_STATUS_FORMAT),
        s_wallTraits[m_defendingTown->m_type][wall].m_name,
        skill, archers * 2, archers * 3);
}

VA(0x0047a380, 0x180)  // dc 0x70714
void combatManager::viewCastleBallista(int isQuickInfo)
{
    if (m_fortificationLevel < COMBAT_FORTIFICATION_CITADEL)
        return;

    std::string msg;
    int numArchers;
    int skill;
    m_defendingTown->calcNumLevelArchers(&numArchers, &skill);
    msg = getTowerString(eWallSectionMainBuilding,
                           numArchers, skill);
    if (m_fortificationLevel >= COMBAT_FORTIFICATION_CASTLE) {
        msg += getTowerString(eWallSectionUpperTower,
                                (numArchers + 1) / 2, skill);
        msg += getTowerString(eWallSectionLowerTower,
                                (numArchers + 1) / 2, skill);
    }

    normalDialog(msg.c_str(), isQuickInfo ? 4 : 1,
                 -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
}

VA(0x0047a500, 0x164)  // dc 0x70820
unsigned char combatManager::handleCombatPlayerDrop(unsigned long dpid,
                                                      message* msg)
{
    int gamePos = g_game->getGamePosFromDPID(dpid);
    if (gamePos == -1)
        return 0;

    if (gamePos != m_playerIds[0] && gamePos != m_playerIds[1]) {
        handlePlayerDrop(dpid);
        return 0;
    }

    if (gamePos == m_playerIds[0]) {
        normalDialogTimeOut(
            g_generalText->getText(GENERAL_TEXT_COMBAT_LOCAL_PLAYER_DROPPED),
            1, 15000, -1, -1, -1, 0, -1, 0, -1, -1, 0);
        msg->m_id = 0x4000;
        msg->m_codeX = 1;
        return 1;
    }

    normalDialogTimeOut(
        g_generalText->getText(GENERAL_TEXT_COMBAT_REMOTE_PLAYER_DROPPED),
        1, 15000, -1, -1, -1, 0, -1, 0, -1, -1, 0);
    m_sideRetreated[1] = 1;
    resetCycleTimers();
    checkWin(msg);
    return 1;
}
