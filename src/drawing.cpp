#include "va.h"

#include <string.h>

#include "drawing.h"

#include "ai_tactical.h"
#include "bitmap16.h"
#include "bitmap816.h"
#include "combatcontrolsubwindow.h"
#include "combatwindow.h"
#include "command.h"
#include "creaturetype.h"
#include "csprite.h"
#include "findpath.h"
#include "game.h"
#include "kb.h"
#include "kbwin.h"
#include "misc.h"
#include "palette.h"
#include "prefs.h"
#include "remote.h"
#include "resourcemanager.h"
#include "soundmgr.h"
#include "textresource.h"
#include "textwdgt.h"
#include "town.h"
#include "widget.h"
#include "winmgr.h"

// DC attests combatManager::CombatAreaLimits; the retail address and all four
// dword lanes are proven by ResetLimitCreature and thirteen other readers.
DATA(0x006aace8) TDrawbridgeBounds g_combatAreaLimits;

// UpdateGrid's private "the grid bitmap has been posted" latch. It is
// cleared when the caller says the clean battlefield was reposted and set
// after the complete visible-grid pass. No other retail body references it.
DATA(0x006969d4) int g_combatGridPosted6969d4;

// NH3API's name crosses to this retail datum by bijective instruction-
// operand correspondence, not by its contradicted address. Retail accesses
// the flag as a full dword in this wrapper and the command-processing sites.
DATA(0x006989ec) int g_processingCombatAction;

// ARITY SCREEN over the located rows, run 2026-08-14 before reconstructing
// anything here. `ret N` against the Dreamcast parameter count:

//   0x4937d0 ret 4    SetupGridForArmy(const army*)          1  OK
//   0x493930 ret 8    UpdateGrid(int, int)                   2  OK
//   0x493cf0 ret      DrawBackground()                       0  OK
//   0x494440 ret 0x18 DrawFrame(6)                           6  OK
//   0x495650 ret      DrawCreatureAndHeroSubwindows()        0  OK (claimed)
//   0x495bf0 ret      ComputeMaxExtent()                     0  OK
//   0x495f50 ret 0x20 ComputeExtent(8)                       8  OK
//   0x4960d0 ret      CycleCombatScreen()                    0  OK
//   0x494c20 ret 8    DrawWallAt(int, int)                   2  OK
//   0x494f40 ret 0xc  DrawOccupant(int, int, int)            3  OK
//   0x495090 ret 0x20 DrawArcher(7)                          7  +1

// 0x495090's extra argument is decoded: a byte that selects table index 0
// vs 0x60 in the 16-bit table at [0x6aacb0 + 0x1c], i.e. a colour row.
// Its only retail caller is the siege-wall/archer pass at 0x494c20, matching
// the Dreamcast DrawWallAt -> DrawArcher edge and settling this row as
// DrawArcher rather than neighbouring DrawCreatureAlpha. Complete therefore
// added the trailing colour selector. The body forwards the other seven
// arguments verbatim to ComputeExtent (0x495f50) plus the +0x13d2c limit
// byte as SaveBiggestExtent, and the screen puts ComputeExtent at eight,
// which is what makes the forwarding legible.

// Every row here reads the combatManager limit band at +0x13d2c..+0x13d44
// - a byte at +0x13d2c, then dwords at +0x13d30/+0x13d34 (two enables) and
// +0x13d38/+0x13d3c/+0x13d40/+0x13d44 used as clip bounds compared in the
// order (>= , <= , > , <) against SLimitData's four dwords at +0/+4/+8/+0xc.
// cmbtmgr.h wants that band modelled before this TU is opened.

// E:\gamedcs\drawing.cpp:47. Dreamcast retains this file-static helper;
// Complete's larger creature roster is expanded into CombatMessage, but the
// ValidHex guard, controlling-side accessor and four shared cases prove that
// the helper boundary survived at source level.
static void getCreatureSpellMessage(char* buffer,
                                       const army* currentArmy,
                                       long currentHex)
{
    if (!combatManager::validHex(currentHex)) {
        buffer[0] = 0;
        return;
    }

    int currentSide = currentArmy->getControllingSide();
    army* targetArmy;
    switch (currentArmy->m_creatureType) {
    case CREATURE_ARCHANGEL:
        targetArmy = g_combatManager->findResurrectionTarget(
            currentSide, currentHex, 1);
        sprintf(buffer, (*g_generalText)[300], targetArmy->getName());
        break;
    case CREATURE_MASTER_GENIE:
        targetArmy = g_combatManager->m_cells[currentHex].getArmy();
        sprintf(buffer, (*g_generalText)[302], targetArmy->getName());
        break;
    case army::ARMY_CREATURE_PIT_LORD:
        targetArmy = g_combatManager->findDemonicResurrectionTarget(
            currentSide, currentHex);
        sprintf(buffer, (*g_generalText)[301], targetArmy->getName());
        break;
    // Complete adds the null-target message absent from DC line 79. Its
    // retail expansion places format 27 before the named format 28; both
    // a negative if/else and an early switch break reproduce that ordering.
    case CREATURE_FAERIE_DRAGON:
        targetArmy = g_combatManager->m_cells[currentHex].getArmy();
        if (!targetArmy) {
            sprintf(buffer, (*g_generalText)[27],
                    g_spellTraits[currentArmy->m_faerieDragonSpell].m_name);
        } else {
            sprintf(buffer, (*g_generalText)[28],
                    g_spellTraits[currentArmy->m_faerieDragonSpell].m_name,
                    targetArmy->getName());
        }
        break;
    case CREATURE_STORM_ELEMENTAL:
        targetArmy = g_combatManager->m_cells[currentHex].getArmy();
        sprintf(buffer, (*g_generalText)[28],
                g_spellTraits[SPELL_PROTECTION_FROM_AIR].m_name,
                targetArmy->getName());
        break;
    case CREATURE_ICE_ELEMENTAL:
        targetArmy = g_combatManager->m_cells[currentHex].getArmy();
        sprintf(buffer, (*g_generalText)[28],
                g_spellTraits[SPELL_PROTECTION_FROM_WATER].m_name,
                targetArmy->getName());
        break;
    case CREATURE_ENERGY_ELEMENTAL:
        targetArmy = g_combatManager->m_cells[currentHex].getArmy();
        sprintf(buffer, (*g_generalText)[28],
                g_spellTraits[SPELL_PROTECTION_FROM_FIRE].m_name,
                targetArmy->getName());
        break;
    case CREATURE_MAGMA_ELEMENTAL:
        targetArmy = g_combatManager->m_cells[currentHex].getArmy();
        sprintf(buffer, (*g_generalText)[28],
                g_spellTraits[SPELL_PROTECTION_FROM_EARTH].m_name,
                targetArmy->getName());
        break;
    case CREATURE_OGRE_MAGE:
        targetArmy = g_combatManager->m_cells[currentHex].getArmy();
        sprintf(buffer, (*g_generalText)[28],
                g_spellTraits[SPELL_BLOODLUST].m_name,
                targetArmy->getName());
        break;
    }
}

static std::string formatRounded(long amount, long high)
{
    if (high < 1000)
        return formatString(
            DATA_COMPGEN(0x00660a1c, roundedDamageIntegerFormat, "%d"),
            amount);

    if (high < 10000) {
        long rounded = (amount + 50) / 100;
        long whole = rounded / 10;
        long fraction = rounded % 10;
        if (fraction == 0) {
            return formatString(
                DATA_COMPGEN(0x00660cb0, roundedDamageThousandsFormat,
                             "%dk"),
                whole);
        }
        return formatString(
            DATA_COMPGEN(0x006772e4, roundedDamageDecimalThousandsFormat,
                         "%d.%dk"),
            whole, fraction);
    }

    return formatString(
        DATA_COMPGEN(0x00660cb0, roundedDamageLargeThousandsFormat, "%dk"),
        (amount + 500) / 1000);
}

static std::string getEstimatedDamage(const army* currentArmy,
                                        army* targetArmy,
                                        unsigned char ranged,
                                        long distance)
{
    long low = currentArmy->m_monInfo.m_damageLowBound * currentArmy->m_numTroops;
    long high = currentArmy->m_monInfo.m_damageHighBound * currentArmy->m_numTroops;
    std::string result;

    if (currentArmy->m_spellInfluence[41] || currentArmy->m_spellInfluence[42]) {
        low = high = currentArmy->computeBaseDamage(1);
    } else if (currentArmy->m_creatureType == CREATURE_BALLISTA) {
        hero* controller = currentArmy->getController();
        low *= controller->getPrimarySkill(0) + 1;
        high *= controller->getPrimarySkill(0) + 1;
    }

    low = currentArmy->adjustDamage(targetArmy, low, ranged, 1,
                                     distance, 0);
    high = currentArmy->adjustDamage(targetArmy, high, ranged, 1,
                                      distance, 0);
    if (low == high) {
        result = formatRounded(high, high);
    } else {
        result = formatString(
            DATA_COMPGEN(0x006772dc, estimatedDamageRangeFormat,
                         "%s-%s"),
            formatRounded(low, high).c_str(),
            formatRounded(high, high).c_str());
    }
    return result;
}

VA(0x004922f0, 0x54C)  // dc 0x8354c
bool combatManager::showCreatureSpellError(
    char* buffer, const army* currentArmy)
{
    if (validHex(m_lastCellIndex) && (currentArmy->m_creatureType == CREATURE_ARCHANGEL
            || currentArmy->m_creatureType == army::ARMY_CREATURE_PIT_LORD
            || currentArmy->m_creatureType == CREATURE_OGRE_MAGE)) {
        hexcell* cell;
        army* targetArmy;
        int i;

        cell = &m_cells[m_lastCellIndex];
        targetArmy = cell->getArmy();
        if (!targetArmy) {
            if (cell->m_attributes & 2) {
                return false;
            }
            for (i = cell->m_bodiesInHex - 1; i >= 0; i--) {
                int deadSide = cell->m_deadArmySide[i];
                int deadSlot = cell->m_deadArmySlot[i];
                if (deadSide == m_currentSide) {
                    targetArmy = &m_armies[deadSide][deadSlot];
                    break;
                }
            }
        }
        if (!targetArmy) {
            return false;
        }
        if (targetArmy->getOwningSide() != m_currentSide) {
            return false;
        }
        if ((targetArmy->is(creatureImmobilized))
                && currentArmy->m_creatureType == CREATURE_OGRE_MAGE) {
            return false;
        }
        if (!(targetArmy->is(creatureImmobilized))
                && currentArmy->m_creatureType == army::ARMY_CREATURE_PIT_LORD) {
            return false;
        }

        if (!currentArmy->m_monInfo.m_hasSpell) {
            if (currentArmy->m_numTroops == 1) {
                sprintf(buffer, (*g_generalText)[697],
                        currentArmy->getName());
            } else {
                sprintf(buffer, (*g_generalText)[698],
                        currentArmy->getName());
            }
            return true;
        }

        if (m_magicTerrain == COMBAT_SPELL_RESTRICTION_NO_CREATURE_SPELLS) {
            strcpy(buffer, (*g_generalText)[699]);
            return true;
        }
        if (m_onAntiMagicGarrison) {
            strcpy(buffer, (*g_generalText)[700]);
            return true;
        }
        for (i = 0; i < 2; i++) {
            if (m_heroes[i]
                    && m_heroes[i]->isWieldingArtifact(
                        ARTIFACT_ORB_OF_INHIBITION)) {
                sprintf(buffer, (*g_generalText)[701],
                        g_artifactTraits[ARTIFACT_ORB_OF_INHIBITION].m_name);
                return true;
            }
        }

        switch (currentArmy->m_creatureType) {
        case CREATURE_OGRE_MAGE: {
            if (getSpellWorkChance(SPELL_BLOODLUST,
                                      targetArmy->m_creatureType, 0, 0) > 0.0f) {
                break;
            }
            sprintf(buffer, (*g_generalText)[181], targetArmy->getName(2),
                    g_spellTraits[SPELL_BLOODLUST].m_name);
            return true;
        }

        case army::ARMY_CREATURE_PIT_LORD: {
            if (!(targetArmy->is(creatureAlive))) {
                sprintf(buffer, (*g_generalText)[702],
                        getArmyName(army::ARMY_CREATURE_DEMON, 2));
                return true;
            }
            if (currentArmy->getResurrectionSize(targetArmy) > 0) {
                break;
            }
            if (targetArmy->m_numTroops == 1) {
                sprintf(buffer, (*g_generalText)[703], targetArmy->getName(),
                        getArmyName(army::ARMY_CREATURE_DEMON, 2));
            } else {
                sprintf(buffer, (*g_generalText)[704], targetArmy->getName(),
                        getArmyName(army::ARMY_CREATURE_DEMON, 2));
            }
            return true;
        }

        case CREATURE_ARCHANGEL: {
            if (targetArmy->m_origNumTroops <= targetArmy->m_numTroops) {
                break;
            }
            if (!(targetArmy->is(creatureAlive))) {
                strcpy(buffer, (*g_generalText)[705]);
                return true;
            }
            if (currentArmy->getResurrectionSize(targetArmy) > 0) {
                break;
            }
            if (currentArmy->m_numTroops == 1) {
                sprintf(buffer, (*g_generalText)[706], currentArmy->getName(),
                        targetArmy->getName());
            } else {
                sprintf(buffer, (*g_generalText)[707], currentArmy->getName(),
                        targetArmy->getName());
            }
            return true;
        }
        }

    }
    return false;
}

// E:\gamedcs\drawing.cpp:326, dc 0x838f0
VA(0x00492840, 0xB8E)  // retail CFG/calls + DC source shape, dc 0x838f0
void combatManager::combatMessage(int command)
{
    if (!m_combatShowIt
            || static_cast<const combatManager*>(this)->isQuickCombat())
        return;

    army* currentArmy = getCurrentArmy();
    army* targetArmy = 0;
    // TCombatWindow's native bool parameter receives this flag directly.
    bool priority = false;
    if (currentArmy->m_side >= 0 && currentArmy->m_slot >= 0)
        targetArmy = &m_armies[currentArmy->m_side][currentArmy->m_slot];

    long distance;
    switch (command) {
    case COMBAT_COMMAND_NONE:
        if (currentArmy->is(creatureShootingArmy) && currentArmy->m_monInfo.m_numShots == 0
                && targetArmy)
            strcpy(g_text, (*g_generalText)[299]);
        else
            strcpy(g_text, "");
        break;

    case COMBAT_COMMAND_WALK:
        sprintf(g_text, (*g_generalText)[295], currentArmy->getName());
        break;

    case COMBAT_COMMAND_FLY:
        sprintf(g_text, (*g_generalText)[296], currentArmy->getName());
        break;

    case COMBAT_COMMAND_ATTACK:
        distance = getDistance(currentArmy->m_gridIndex, m_lastMoveToIndex);
        if (g_unnamed698758.m_combatArmyInfoLevel) {
            sprintf(g_text, (*g_generalText)[37], targetArmy->getName(),
                    getEstimatedDamage(currentArmy, targetArmy, 0,
                                         distance).c_str());
        } else {
            sprintf(g_text, (*g_generalText)[221], targetArmy->getName());
        }
        priority = 1;
        break;

    case COMBAT_COMMAND_SHOOT:
    case COMBAT_COMMAND_SHOOT_PENALTY: {
        distance = getDistance(currentArmy->m_gridIndex,
                                targetArmy->m_gridIndex);
        long targetHits = targetArmy->getTotalHitPoints(0);
        long currentHits = currentArmy->getTotalHitPoints(0);
        long expectedDamage = aiGetAttackDamage(*(currentArmy), currentHits, *(targetArmy), 1, distance);
        if (!g_unnamed698758.m_combatArmyInfoLevel) {
            sprintf(g_text, (*g_generalText)[221], targetArmy->getName());
        } else if (currentArmy->m_monInfo.m_numShots == 1) {
            sprintf(g_text, (*g_generalText)[38], targetArmy->getName(),
                    getEstimatedDamage(currentArmy, targetArmy, 1,
                                         distance).c_str());
        } else {
            // Retail loads text row 297 (table displacement 0x4a4).
            sprintf(g_text, (*g_generalText)[297], targetArmy->getName(),
                    currentArmy->m_monInfo.m_numShots,
                    getEstimatedDamage(currentArmy, targetArmy, 1,
                                         distance).c_str());
        }
        priority = 1;
        break;
    }

    case COMBAT_COMMAND_SPELL_BOOK:
        strcpy(g_text, (*g_generalText)[418]);
        break;

    case COMBAT_COMMAND_VIEW_OTHER_HERO:
        strcpy(g_text, (*g_generalText)[419]);
        break;

    case COMBAT_COMMAND_VIEW_TOWERS:
        strcpy(g_text, (*g_generalText)[157]);
        break;

    case COMBAT_COMMAND_VIEW_ARMY:
        if (validHex(m_lastCellIndex)) {
            army* viewedArmy = m_cells[m_lastCellIndex].getArmy();
            if (viewedArmy)
                sprintf(g_text, (*g_generalText)[298],
                        viewedArmy->getName(1));
            else
                g_text[0] = 0;
        }
        break;

    case COMBAT_COMMAND_BOMBARD_WALL: {
        int wall;
        for (wall = 0; wall < WALL_TARGET_COUNT; wall++) {
            if (currentArmy->m_slot == s_wallTargets[wall].m_targetHex) {
                sprintf(g_text, (*g_generalText)[221],
                        s_wallTraits[m_defendingTown->m_type]
                                    [s_wallTargets[wall].m_wall].m_name);
                break;
            }
        }
        break;
    }

    case COMBAT_COMMAND_CREATURE_SPELL:
        getCreatureSpellMessage(g_text, currentArmy, m_lastCellIndex);
        break;

    case COMBAT_COMMAND_FIRST_AID:
        // DC drawing.cpp:445 reads manager +0x12984, the same side used
        // by get_current_army. Retail reuses actingSide * 21 from that
        // earlier lookup; reading currentArmy->m_side adds another product.
        sprintf(g_text, (*g_generalText)[420],
                m_armies[m_actingSide]
                    [m_cells[m_lastCellIndex].m_armySlot].getName());
        break;

    default:
        g_text[0] = 0;
        break;
    }

    if (command == COMBAT_COMMAND_NONE
            || command == COMBAT_COMMAND_WALK
            || command == COMBAT_COMMAND_FLY
            || command == COMBAT_COMMAND_VIEW_ARMY) {
        if (showCreatureSpellError(g_text, currentArmy))
            priority = 1;
    }
    m_combatWindow->combatMessage(g_text, 0, priority);
}

VA(0x004933d0, 0x27A)  // dc 0x833b4
static std::string getEstimatedDamage(const army* currentArmy,
                                        army* targetArmy,
                                        unsigned char ranged,
                                        long distance);

VA(0x00493650, 0xBD)  // dc 0x832d8
static std::string formatRounded(long amount, long high);

VA(0x00493710, 0x63)  // dc 0x83db0
void combatManager::resetLimitCreature()
{
    memset(m_creatureEffect, 0, sizeof m_creatureEffect);
    m_heroEffect[0] = 0;
    m_heroEffect[1] = 0;
    m_flagEffect[0] = 0;
    m_flagEffect[1] = 0;
    memset(m_archerEffect, 0, sizeof m_archerEffect);
    m_drawbridgeBounds = g_combatAreaLimits;
}

VA(0x00493780, 0x44)
void combatManager::updateCombatArea()
{
    if (!static_cast<const combatManager*>(this)->isQuickCombat()
            && m_combatShowIt) {
        g_windowManager->updateScreen(
            g_combatDrawLimits694f18.m_minX,
            g_combatDrawLimits694f18.m_minY,
            g_combatDrawLimits694f18.m_maxX
                - g_combatDrawLimits694f18.m_minX + 1,
            g_combatDrawLimits694f18.m_maxY
                - g_combatDrawLimits694f18.m_minY + 1);
    }
}

// Windows fixed-viewport extent helpers are defined once in cmbtmgr.h;
// see their platform evidence comment.
// The coordinate ScrollTo facade below retains its drawing.cpp ownership.

// Original: combatManager::ScrollTo; drawing.cpp:666, dc 0x841d4.
unsigned char combatManager::scrollTo(int x, int y, unsigned char draw,
    unsigned char doscrollX, unsigned char doscrollY)
{
    return scrollTo(SLimitData(x, y, x + 1, y + 1), draw, doscrollX, doscrollY);
}

// E:\gamedcs\drawing.cpp:679, dc 0x84248. ScrollTo's rectangle overload
// constructs SLimitData(x, y, x + width, y + height) at line 680 and delegates
// to the extent overload. SpellEffect calls it at line 2653; the
// fixed PC viewport folds the false result into UpdateCombatArea's path.
bool combatManager::scrollTo(int x, int y, int width, int height, bool draw,
                             bool doscrollX, bool doscrollY)
{
    return scrollTo(SLimitData(x, y, x + width, y + height),
                    draw, doscrollX, doscrollY);
}

#if 0  // @carcass

// DC FullUpdate/UpdateCombatArea(x,y,w,h), ScrollCombatArea, ScrollToPixel
// and Rescale operate on the removed ca_scroll_x/y origin and translated
// destination (8,32). Complete's retained UpdateCombatArea0x493780 and
// DrawFrame0x494440 submit the fixed client rectangle via UpdateScreen0x602bd0;
// the two scroll-origin members are absent before the retained archer records.
// Preserve the ordinary ScrollTo facades used by source calls, while these
// exact translated-viewport interfaces are reviewed in dc_only.tsv.

// E:\gamedcs\drawing.cpp:689
// RETAIL_LOCATED(0x004937d0, 0x155): not reconstructed; dc-callgraph unique, dc 0x842a8
void combatManager::setupGridForArmy(const army* thisArmy)
{
    // @stub
}

// E:\gamedcs\drawing.cpp:755
// RETAIL_LIVE(0x00493930, 0x3c0): reconstructed below; anchor-global, dc 0x84420
int combatManager::updateGrid(int bPostGridIsClean, int bSetupGrid)
{
    // @stub
}

// E:\gamedcs\drawing.cpp:919
// RETAIL_LOCATED(0x00493cf0, 0x1ab): not reconstructed; dc-callgraph unique, dc 0x847dc
void combatManager::drawBackground()
{
    // @stub
}

// E:\gamedcs\drawing.cpp:982
// RETAIL_LIVE(0x00493ea0, 0x4ca): reconstructed below; dc order/callgraph,
// dc 0x849c4; Complete adds a trailing force-refresh flag
void combatManager::updateMouseGrid(int iNewMouseGridIndex,
                                    std::vector<long>& hexes,
                                    unsigned char forceUpdate)
{
    // @stub
}

// E:\gamedcs\drawing.cpp:1141
// RETAIL_LOCATED(0x00494440, 0x7d5): not reconstructed; anchor-global, dc 0x84e2c
void combatManager::drawFrame(unsigned char update, unsigned char bLimitCreatureEffect, unsigned char bLimitDraw, int iDelay, unsigned char bRefreshBackground, unsigned char bDoDelayTil)
{
    // @stub
}

// E:\gamedcs\drawing.cpp:1426
// RETAIL_LIVE(0x00494c20, 0x31c): reconstructed below; callee-set, dc 0x85478
void combatManager::drawWallAt(int hex_index, int dx)
{
    // @stub
}

// E:\gamedcs\drawing.cpp:1602
// RETAIL_LIVE(0x00494f40, 0x147): reconstructed below; callee-set, dc 0x85844
void combatManager::drawOccupant(int index, int iDrawPriority, int bNumBoxOnly)
{
    // @stub
}

// E:\gamedcs\drawing.cpp:1661
// RETAIL_LIVE(0x00495090, 0x114): reconstructed below; dc caller edge, dc 0x85978
int combatManager::drawArcher(const CSprite* sprite, int sequence, int frame, int x, int y, SLimitData* psLimitData, unsigned char isFlipped)
{
    // @stub
}

// E:\gamedcs\drawing.cpp:1699
// RETAIL_LIVE(0x004951b0, 0xfd): reconstructed below; caller-edge, dc 0x85a48
int combatManager::drawCreature(const CSprite* sprite, int sequence, int frame, int x, int y, SLimitData* psLimitData, int id, unsigned char isFlipped, int iColor)
{
    // @stub
}

// E:\gamedcs\drawing.cpp:1772
// RETAIL_LIVE(0x004952b0, 0xfb): reconstructed below; caller-edge, dc 0x85c2c
int combatManager::drawCombatHero(const CSprite* sprite, int sequence, int frame, int x, int y, SLimitData* psLimitData, unsigned char isFlipped)
{
    // @stub
}

// E:\gamedcs\drawing.cpp:1804
// RETAIL_LIVE(0x004953b0, 0x144): reconstructed below; caller-edge, dc 0x85d00
int combatManager::drawSpellEffect(const CSprite* sprite, int frame, int x, int y, unsigned char isFlipped, unsigned char isAlpha)
{
    // @stub
}

// E:\gamedcs\drawing.cpp:1836
// RETAIL_LIVE(0x00495500, 0x142): reconstructed below; caller-edge, dc 0x85e3c
int combatManager::drawSpriteObject(const CSprite* sprite, int frame, int x, int y, unsigned char isFlipped)
{
    // @stub
}

#endif  // @carcass

// E:\gamedcs\drawing.cpp:689
VA(0x004937d0, 0x155)  // dc 0x842a8
void combatManager::setupGridForArmy(const army* thisArmy)
{
    if (isQuickCombat())
        return;
    if (thisArmy->m_creatureType == army::ARMY_CREATURE_ARROW_TOWER)
        return;
    if (!g_unnamed698758.m_showCombatGrid && !m_creaturePlacement)
        return;

    thisArmy->getAttackMask(thisArmy->m_gridIndex, 2, -1);
    memset(m_curDrawGridShade, 0, sizeof(m_curDrawGridShade));

    int side = thisArmy->getControllingSide();
    g_searchArray->seedCombatPosition(thisArmy, side, thisArmy->getSpeed(),
                                      m_creaturePlacement, -1);

    for (int i = 0; i < COMBAT_GRID_CELLS; i++) {
        if (i == thisArmy->m_gridIndex) {
            m_curDrawGridShade[i] = 1;
        } else if (thisArmy->is(creatureDoubleWide)
                   && i == thisArmy->m_gridIndex
                       + thisArmy->offsetToFront(-1)) {
            m_curDrawGridShade[i] = 1;
        } else if (m_cells[i].m_validMove || m_cells[i].m_frontMove) {
            if (m_cells[i].hasArmy()) {
                if (m_cells[i].getArmy()->getOwningSide()
                        != thisArmy->getControllingSide())
                    m_curDrawGridShade[i] = 1;
            } else
                m_curDrawGridShade[i] = 3;
        }
    }
}

VA(0x00493930, 0x3c0)  // dc 0x84420
int combatManager::updateGrid(int postGridIsClean, int setupGrid)
{
    if (isQuickCombat())
        return 0;

    army* currentArmy = getCurrentArmy();
    if (setupGrid) {
        if (isComputerAction()
                || !m_sideIsLocalHuman[currentArmy->getControllingSide()])
            memset(m_curDrawGridShade, 0, sizeof(m_curDrawGridShade));
        else
            setupGridForArmy(currentArmy);
    }

    if (postGridIsClean)
        g_combatGridPosted6969d4 = 0;

    if (!g_unnamed698758.m_combatShadeLevel && !m_creaturePlacement
            && !g_unnamed698758.m_showCombatGrid && !m_debugShowBlockedHexes)
        return 0;

    unsigned char update = 0;

    if (m_debugShowBlockedHexes) {
        for (int i = 0; i < COMBAT_GRID_CELLS; i++) {
            if (m_cells[i].m_attributes & 2) {
                m_combatShadowBitmap->draw(
                    0, 0, 45, 52, m_saveScreenPostGrid,
                    m_cells[i].m_hexUlx, m_cells[i].m_hexUly, true);
                update = 1;
            }
        }
    }

    if (g_unnamed698758.m_combatShadeLevel || m_creaturePlacement) {
        unsigned char newGrid = 0;
        unsigned char oldGrid = 0;
        unsigned char changed = 0;
        int i;
        for (i = 0; i < COMBAT_GRID_CELLS; i++) {
            if (m_lastDrawGridShade[i] != m_curDrawGridShade[i]) {
                changed = 1;
                break;
            }
        }

        for (i = 0; i < COMBAT_GRID_CELLS; i++) {
            if (m_curDrawGridShade[i]) {
                newGrid = 1;
                break;
            }
        }

        for (i = 0; i < COMBAT_GRID_CELLS; i++) {
            if (m_lastDrawGridShade[i]) {
                oldGrid = 1;
                break;
            }
        }

        if (!postGridIsClean) {
            if (!changed)
                return 0;

            if (oldGrid) {
                SLimitData updateLimits =
                    *static_cast<const SLimitData*>(static_cast<const void*>(
                        &g_combatAreaLimits));
                for (i = 0; i < COMBAT_GRID_CELLS; i++) {
                    if (m_lastDrawGridShade[i] != m_curDrawGridShade[i]
                            || m_curDrawGridShade[i]) {
                        updateLimits.include(m_cells[i].limits());
                    }
                }

                updateLimits.clip(g_combatGridAreaLimits);

                m_saveScreenPreGrid->draw(
                    updateLimits.m_minX - 58,
                    updateLimits.m_minY - 86,
                    updateLimits.width(), updateLimits.height(),
                    m_saveScreenPostGrid, updateLimits.m_minX, updateLimits.m_minY,
                    false);
                update = 1;
            }
        }

        if (newGrid) {
            for (i = 0; i < COMBAT_GRID_CELLS; i++) {
                if (m_curDrawGridShade[i]) {
                    m_saveScreenPostGrid->darken(
                        m_cells[i].m_hexUlx, m_cells[i].m_hexUly, 45, 52,
                        m_combatShadowBitmap, 0, 0);
                    update = 1;
                }
            }
        }
    }

    if (g_unnamed698758.m_showCombatGrid
            && (!g_combatGridPosted6969d4 || update)) {
        for (int i = 0; i < COMBAT_GRID_CELLS; i++) {
            if (!inInvisibleColumn(i)) {
                m_combatCellGridBitmap->draw(
                    0, 0, m_combatCellGridBitmap->getWidth(),
                    m_combatCellGridBitmap->getHeight(), m_saveScreenPostGrid,
                    m_cells[i].m_hexUlx, m_cells[i].m_hexUly, true);
            }
        }
        update = 1;
        g_combatGridPosted6969d4 = 1;
    }

    memcpy(m_lastDrawGridShade, m_curDrawGridShade, sizeof(m_lastDrawGridShade));
    return update;
}

VA(0x00493cf0, 0x1ab)  // dc 0x847dc
void combatManager::drawBackground()
{
    if (static_cast<const combatManager*>(this)->isQuickCombat())
        return;
    if (m_backgroundDrawn)
        return;

    ResourceManager::getBackdrop(m_backgroundName, m_saveScreenPostGrid);

    int index = m_largeObstacleId;
    if (index >= 0) {
        const SElevationOverlay* overlay = &s_elevationOverlay[index];
        Bitmap816* bitmap = ResourceManager::getBitmap816(
            overlay->m_fileName);
        bitmap->draw(0, 0, bitmap->getWidth(), bitmap->getHeight(), m_saveScreenPostGrid,
                     overlay->m_x, overlay->m_y, true);
        bitmap->dispose();
    }

    if (m_fortificationLevel > COMBAT_FORTIFICATION_NONE && m_moatOn) {
        TWallTraits* traits =
            &s_wallTraits[m_defendingTown->m_type][WALL_TRAITS_ROW_MOAT];
        Bitmap816* bitmap = m_combatIcons[WALL_TRAITS_ROW_MOAT][0];
        if (bitmap) {
            bitmap->draw(
                0, 0, bitmap->getWidth(), bitmap->getHeight(), m_saveScreenPostGrid,
                traits->m_x, traits->m_y, true);
        }

        traits = &s_wallTraits[m_defendingTown->m_type]
                              [WALL_TRAITS_ROW_MOAT + 1];
        bitmap = m_combatIcons[WALL_TRAITS_ROW_MOAT + 1][0];
        if (bitmap) {
            bitmap->draw(
                0, 0, bitmap->getWidth(), bitmap->getHeight(), m_saveScreenPostGrid,
                traits->m_x, traits->m_y, true);
        }
    }

    m_saveScreenPostGrid->draw(0x3a, 0x56, 0x2ab, 0x1d8,
                     m_saveScreenPreGrid, 0, 0, false);
    updateGrid(1, 0);
    m_saveScreenPostGrid->draw(0, 0, 800, 556,
                     g_windowManager->m_screenBitmap, 0, 0, false);
    m_backgroundDrawn = 1;
}

// E:\gamedcs\drawing.cpp:982
VA(0x00493ea0, 0x4ca)  // dc 0x849c4
void combatManager::updateMouseGrid(int newMouseGridIndex,
                                    std::vector<long>& hexes,
                                    unsigned char forceUpdate)
{
    DATA(0x006772d8)
    static int lastMouseGridIndex = -1;
    DATA_COMPGEN_GUARD(0x006969b0, oldMouseGridHexesGuard, oldHexes)
    VA_COMPGEN(0x00494370, 0x20, STATIC_DTOR, oldHexes)
    DATA(0x006969b8)
    static std::vector<long> oldHexes;

    if (m_battleOver
            || static_cast<const combatManager*>(this)->isQuickCombat()
            || !g_unnamed698758.m_showCombatMouseHex)
        return;
    if (newMouseGridIndex == lastMouseGridIndex && !forceUpdate)
        return;

    int i;
    int copyHeight;
    for (i = 0; i < oldHexes.size(); ++i) {
        hexcell& cell = m_cells[oldHexes[i]];
        copyHeight = 52;
        if (cell.m_hexUly > 504)
            copyHeight = 556 - cell.m_hexUly;

        m_combatMouseBackground->draw(
            cell.m_backgroundOffset * 45, 0, 45, copyHeight,
            m_saveScreenPostGrid, cell.m_hexUlx, cell.m_hexUly, false);
        cell.m_backgroundOffset = -1;
    }

    {
        unsigned char offsetUsed[19];
        memset(offsetUsed, 0, sizeof(offsetUsed));
        int offset = 0;
        for (i = 0; i < hexes.size(); ++i) {
            hexcell& cell = m_cells[hexes[i]];
            while (offsetUsed[offset] && offset < 19)
                ++offset;

            cell.m_backgroundOffset = offset;
            offsetUsed[offset] = 1;
            copyHeight = 52;
            if (cell.m_hexUly > 504)
                copyHeight = 556 - cell.m_hexUly;

            m_saveScreenPostGrid->draw(
                cell.m_hexUlx, cell.m_hexUly, 45, copyHeight,
                m_combatMouseBackground, offset * 45, 0, false);
        }
    }

    for (i = 0; i < hexes.size(); ++i) {
        hexcell& cell = m_cells[hexes[i]];
        m_saveScreenPostGrid->darken(cell.m_hexUlx, cell.m_hexUly, 45, 52,
                           m_combatShadowBitmap, 0, 0);
    }

    SLimitData& extent =
        *static_cast<SLimitData*>(static_cast<void*>(&m_drawbridgeBounds));
    const SLimitData& combatDrawLimits =
        *static_cast<const SLimitData*>(static_cast<const void*>(
            &g_combatDrawLimits694f18));
    SLimitData saveExtent = extent;
    int saveLimitToExtent = m_limitToExtent;
    m_drawbridgeBounds = g_combatAreaLimits;
    m_limitToExtent = 1;

    for (i = 0; i < oldHexes.size(); ++i) {
        const hexcell& cell = m_cells[oldHexes[i]];
        extent.include(SLimitData(cell.m_hexUlx, cell.m_hexUly,
                                  cell.m_hexUlx + 44,
                                  cell.m_hexUly + 51));
    }
    for (i = 0; i < hexes.size(); ++i) {
        const hexcell& cell = m_cells[hexes[i]];
        extent.include(SLimitData(cell.m_hexUlx, cell.m_hexUly,
                                  cell.m_hexUlx + 44,
                                  cell.m_hexUly + 51));
    }

    extent.clip(combatDrawLimits);
    m_saveScreenPostGrid->draw(
        m_drawbridgeBounds.m_minX, m_drawbridgeBounds.m_minY,
        extent.width(), extent.height(), g_windowManager->m_screenBitmap,
        m_drawbridgeBounds.m_minX, m_drawbridgeBounds.m_minY, false);
    drawFrame(0, 0, 0, 0, 1, 0);
    updateCombatArea(extent);

    extent = saveExtent;
    m_limitToExtent = saveLimitToExtent;
    lastMouseGridIndex = newMouseGridIndex;

    oldHexes.clear();
    for (i = 0; i < hexes.size(); ++i)
        oldHexes.push_back(hexes[i]);
}

VA(0x00494390, 0xA7)  // dc 0x84dac
void combatManager::updateMouseGrid(int newMouseGridIndex,
                                    int allowDuringAction)
{
    if (g_processingCombatAction && !allowDuringAction)
        return;

    std::vector<long> hexes;
    if (newMouseGridIndex >= 0
            && newMouseGridIndex < COMBAT_GRID_HEX_COUNT
            && newMouseGridIndex % COMBAT_GRID_COLUMN_COUNT != 0
            && newMouseGridIndex % COMBAT_GRID_COLUMN_COUNT
                   != COMBAT_GRID_RIGHT_BORDER_COLUMN) {
        hexes.push_back(newMouseGridIndex);
    } else {
        newMouseGridIndex = -1;
    }

    updateMouseGrid(newMouseGridIndex, hexes, 0);
}

// Complete keeps the DC six-stage battlefield compositor. The first pass
// draws underlay obstacles, the eight-priority pass interleaves walls, corpses,
// ordinary obstacles and living stacks, and the tail posts either the full
// combat area or the accumulated creature-effect extent. Dreamcast's line
// table preserves the three small helper boundaries which retail /Ob2 folds
// into these walks.
// DC1165 groups the three chat reset assignments. The source chain
// limitCreatureEffect = limitDraw = limitToExtent = 0 reproduces the
// retail B7 store/test schedule and raises 99.9453 to 99.9807. The 48-state
// reset/step/local-order family emits seven objects and reproduces all seven;
// independent field-first stores give the same winning object. Preserve the
// chain because DC also converts the int zero before copying the byte flags.
// Changing the column-step operand order or the three local declarations
// does not move its remaining priority-loop reload schedule. All 28 exact
// siblings survive. Reversing the canonical GetHexIndex sum was byte-flat
// across drawing, cmbtmgr and spells (two states, one reproduced object).
// DrawObstacle, DrawObstacleAt, DrawDeadOccupants and both GetHexIndex source
// calls remain canonical. GetHexIndex is a static header member; the audit's
// unmatched call-name lead does not denote an absent source call.
// The reused row (and side in the two later animation helpers) now has an
// explicit enclosing declaration, preserving VC6's old for-scope lifetime.
// This is byte-flat and allows Clang to check all five recorded local types.
// The last priority-loop reload exchange is recovered by sharing one column
// counter between the underlay and priority walks, reinitialized at each
// loop. Six counter-lifetime states produce two reproduced objects: shared
// columns reach 100%, separate columns leave 99.9807%, independently of
// the priority declaration's placement. The five named DC locals remain.
// DC1205/1207 and1303/1305/1307 prove both walks, but do not name their
// counters, so sharing is a retail-tested lifetime hypothesis.
// DrawFrame's DC public symbol ends _N00H00: five native Boolean flags.
// E:\gamedcs\drawing.cpp:1141
VA(0x00494440, 0x7d5)  // anchor-global + retail arity, dc 0x84e2c
void combatManager::drawFrame(bool update,
                              bool limitCreatureEffect,
                              bool limitDraw, int delay,
                              bool refreshBackground,
                              bool doDelayTil)
{
    if (m_battleOver
            || static_cast<const combatManager*>(this)->isQuickCombat()
            || !m_combatShowIt)
        return;

    SLimitData tempLimits;

    if (limitCreatureEffect) {
        computeMaxExtent();
        m_limitToExtent = 1;
    }

    if (g_chatMan.chatChanged()) {
        limitCreatureEffect = limitDraw = m_limitToExtent = 0;
        if (m_backgroundDrawn) {
            m_saveScreenPostGrid->draw(
                m_combatWindow->m_chatWidget->m_x,
                m_combatWindow->m_chatWidget->m_y,
                m_combatWindow->m_chatWidget->m_width,
                m_combatWindow->m_chatWidget->m_height,
                g_windowManager->m_screenBitmap,
                m_combatWindow->m_chatWidget->m_x,
                m_combatWindow->m_chatWidget->m_y, false);
        } else {
            drawBackground();
        }
    } else if (refreshBackground) {
        if (m_backgroundDrawn) {
            if (limitCreatureEffect || limitDraw || m_limitToExtent) {
                m_saveScreenPostGrid->draw(
                    m_drawbridgeBounds.m_minX, m_drawbridgeBounds.m_minY,
                    m_drawbridgeBounds.width(), m_drawbridgeBounds.height(),
                    g_windowManager->m_screenBitmap,
                    m_drawbridgeBounds.m_minX, m_drawbridgeBounds.m_minY, false);
            } else {
                m_saveScreenPostGrid->draw(0, 0, 800, 556,
                                 g_windowManager->m_screenBitmap,
                                 0, 0, false);
            }
        } else {
            drawBackground();
        }
    }

    int row;
    int column;
    for (row = 0; row < 11; row++) {
        for (column = 1; column < COMBAT_GRID_LAST_COLUMN; column++) {
            hexcell& cell = m_cells[getHexIndex(column, row)];
            if (cell.m_attributes & 1) {
                TObstacle& obstacle = m_obstacles[cell.m_obstacleIndex];
                if (obstacle.m_shape->m_underlay) {
                    if (obstacle.m_isVisible
                            || (obstacle.m_owner == m_currentSide
                                && m_sideIsLocalHuman[m_currentSide])
                            || (obstacle.m_owner != m_currentSide
                                && !m_sideIsLocalHuman[m_currentSide])
                            || m_debugShowHiddenObjects) {
                        drawObstacle(cell);
                    }
                }
            }
        }
    }

    if (m_fortificationLevel > COMBAT_FORTIFICATION_NONE) {
        const TWallTraits& traits =
            s_wallTraits[m_defendingTown->m_type][eWallSectionBackWall];
        drawObject(m_combatIcons[eWallSectionBackWall][0],
                   traits.m_x, traits.m_y);
    }

    if (m_heroes[0]) {
        drawCombatHero(m_heroFlagSprites[0], 0, m_cmbtHeroFlagFrame[0],
                       29, 20,
                       &m_cmbtHeroFlagLimitData[0], 0);
        drawCombatHero(m_creatureSprites[0], m_cmbtHeroFrameType[0], m_cmbtHeroFrameIndex[0],
                       -43, -19,
                       &m_cmbtHeroLimitData[0], 0);
    }
    if (m_heroes[1]) {
        drawCombatHero(m_heroFlagSprites[1], 0, m_cmbtHeroFlagFrame[1],
                       755, 20,
                       &m_cmbtHeroFlagLimitData[1], 0);
        drawCombatHero(m_creatureSprites[1], m_cmbtHeroFrameType[1], m_cmbtHeroFrameIndex[1],
                       693, -19,
                       &m_cmbtHeroLimitData[1], 1);
    }

    if (m_fortificationLevel > COMBAT_FORTIFICATION_NONE)
        drawWallAt(255, 1);

    for (row = 0; row < 11; row++) {
        if (m_fortificationLevel > COMBAT_FORTIFICATION_NONE
                && row == COMBAT_GATE_ROW
                && m_drawbridgeState != DRAWBRIDGE_UP) {
            const TWallTraits& traits =
                s_wallTraits[m_defendingTown->m_type][eWallSectionDoor];
            drawObject(m_combatIcons[eWallSectionDoor][m_drawbridgeState],
                       traits.m_x, traits.m_y);
        }

        int xStart;
        int xChange;
        int xStop;
        if (m_fortificationLevel > COMBAT_FORTIFICATION_NONE && row >= 6) {
            xStart = COMBAT_GRID_LAST_COLUMN;
            xStop = -1;
            xChange = -1;
        } else {
            xStart = 0;
            xStop = COMBAT_GRID_ROW_STRIDE;
            xChange = 1;
        }

        for (int priority = 0; priority <= COMBAT_DRAW_PRIORITY_SINGLE_PASS;
                priority++) {
            for (column = xStart; column != xStop;
                    column += xChange) {
                const int hexIndex = getHexIndex(column, row);

                if (m_fortificationLevel > COMBAT_FORTIFICATION_NONE
                        && priority == COMBAT_DRAW_PRIORITY_WALL) {
                    drawWallAt(hexIndex, xChange);
                } else if (priority == COMBAT_DRAW_PRIORITY_CORPSE) {
                    drawDeadOccupants(hexIndex);
                } else if (priority == COMBAT_DRAW_PRIORITY_OBSTACLE) {
                    drawObstacleAt(hexIndex);
                }

                if (m_cells[hexIndex].hasArmy())
                    drawOccupant(hexIndex, priority, 0);
            }
        }

        if (m_fortificationLevel > COMBAT_FORTIFICATION_NONE
                && row == COMBAT_GATE_ROW
                && m_drawbridgeState == DRAWBRIDGE_DOWN
                && m_combatIcons[eWallSectionDoorRope][1]) {
            const TWallTraits& traits =
                s_wallTraits[m_defendingTown->m_type][eWallSectionDoorRope];
            drawObject(m_combatIcons[eWallSectionDoorRope][1],
                       traits.m_x, traits.m_y);
        }
        pollSound();
    }

    if (m_fortificationLevel > COMBAT_FORTIFICATION_NONE) {
        drawWallAt(200, -1);
        if (m_fortificationLevel > COMBAT_FORTIFICATION_NONE)
            drawWallAt(251, -1);
    }

    m_combatWindow->drawChatText(0);
    if (g_unnamed698758.m_combatArmyInfoLevel)
        drawCreatureAndHeroSubwindows();

    if (doDelayTil) {
        GameTime::delayTil(g_timers[0]);
        g_timers[0] = GameTime::nextFrameTime(
            g_timers[0],
            static_cast<long>(
                delay
                * g_combatSpeedFactors[g_unnamed698758.m_combatSpeed]));
    }

    if (update) {
        if (!limitCreatureEffect && !limitDraw) {
            updateCombatArea();
            return;
        }

        SLimitData& bounds = m_drawbridgeBounds;
        const SLimitData& combatDrawLimits = g_combatDrawLimits694f18;
        bounds.clip(combatDrawLimits);
        updateCombatArea(bounds);
    }

    if (limitCreatureEffect || limitDraw)
        m_limitToExtent = 0;
}

// Dreamcast drawing.cpp:1399. Complete's /Ob2 build folds this helper into
// DrawFrame, but the source boundary and statement grouping remain positive
// CodeView evidence.

void combatManager::drawObstacleAt(int hexIndex)
{
    hexcell& cell = m_cells[hexIndex];
    if (cell.m_attributes & 1) {
        TObstacle& obstacle = m_obstacles[cell.m_obstacleIndex];
        if (!obstacle.m_shape->m_underlay) {
            if (obstacle.m_isVisible
                    || (obstacle.m_owner == m_currentSide
                        && m_sideIsLocalHuman[m_currentSide])
                    || (obstacle.m_owner != m_currentSide
                        && !m_sideIsLocalHuman[m_currentSide])
                    || m_debugShowHiddenObjects)
                drawObstacle(cell);
        }
    }
}

VA(0x00494c20, 0x31c)  // dc 0x85478
void combatManager::drawWallAt(int hexIndex, int dx)
{
    const TWallTraits* const wtTable = s_wallTraits[m_defendingTown->m_type];

    for (int wall = eWallSectionDoor; wall < kNumWallSections; wall++) {
        const TWallTraits& traits = wtTable[wall];
        int wallHex = traits.m_hex;
        Bitmap816* image = m_combatIcons[wall][m_wallStanding[wall]];
        if (wallHex == -1 || !image)
            continue;

        if (!(wall == eWallSectionMainBuilding
                || wall == eWallSectionMainBuildingCover
                || wall == eWallSectionLowerTower
                || wall == eWallSectionLowerTowerCover
                || wall == eWallSectionUpperTower
                || wall == eWallSectionUpperTowerCover
                || wall == eWallSectionGate)) {
            if (hexIndex
                    == wallHex + dx * COMBAT_GRID_ROW_STRIDE
                        - rowIsOdd(gridY(wallHex))
                    || (hexIndex == wallHex && !gridY(wallHex))) {
                const int sw = m_cells[wallHex].m_hexUlx - traits.m_x;
                if (sw > 0)
                    drawWall(image, 0, 0, sw, image->getHeight(),
                             traits.m_x, traits.m_y);
            } else if (hexIndex
                    == wallHex - dx * COMBAT_GRID_ROW_STRIDE
                        - rowIsOdd(gridY(wallHex)) + 1) {
                const hexcell& cell = m_cells[wallHex];
                const int sw = image->getWidth() - cell.m_hexUlx + traits.m_x
                               - COMBAT_WALL_HEX_WIDTH;
                if (sw > 0) {
                    const int sx = cell.m_hexUlx - traits.m_x
                                   + COMBAT_WALL_HEX_WIDTH;
                    drawWall(image, sx, 0, sw, image->getHeight(),
                             cell.m_hexUlx + COMBAT_WALL_HEX_WIDTH, traits.m_y);
                }
            }

            if (hexIndex == wallHex) {
                int width = COMBAT_WALL_HEX_WIDTH;
                const hexcell& cell = m_cells[wallHex];
                int sourceX = cell.m_hexUlx - traits.m_x;
                int destX = cell.m_hexUlx;
                if (sourceX < 0) {
                    width += sourceX;
                    destX = traits.m_x;
                    sourceX = 0;
                }
                int remainingWidth = image->getWidth() - sourceX;
                if (width > remainingWidth)
                    width = remainingWidth;
                drawWall(image, sourceX, 0, width, image->getHeight(),
                         destX, traits.m_y);
            }
        }
        else {
            if (hexIndex != wallHex)
                continue;

            if (wall == eWallSectionMainBuildingCover
                    || wall == eWallSectionLowerTowerCover
                    || wall == eWallSectionUpperTowerCover) {
                TArcher* archer;
                if (wall == eWallSectionMainBuildingCover)
                    archer = &m_archers[0];
                else if (wall == eWallSectionLowerTowerCover)
                    archer = &m_archers[1];
                else
                    archer = &m_archers[2];

                if (archer->m_sprite) {
                    int drawX;
                    if (!archer->m_facing) {
                        drawX = archer->m_x - archer->m_sprite->getWidth();
                        drawX += COMBAT_ARCHER_X_BIAS;
                        if (g_creatureTypeTraits[archer->m_creatureType].m_attributes
                                & COMBAT_ARCHER_DOUBLE_WIDE_ATTRIBUTE)
                            drawX += COMBAT_WALL_HEX_WIDTH;
                        if (archer->m_creatureType == CREATURE_MEDUSA)
                            drawX -= 5;
                    } else {
                        drawX = archer->m_x - COMBAT_ARCHER_X_BIAS;
                        if (g_creatureTypeTraits[archer->m_creatureType].m_attributes
                                & COMBAT_ARCHER_DOUBLE_WIDE_ATTRIBUTE)
                            drawX -= COMBAT_WALL_HEX_WIDTH;
                        if (archer->m_creatureType == CREATURE_MEDUSA)
                            drawX += 5;
                    }
                    int drawY = archer->m_y - COMBAT_ARCHER_Y_BIAS;

                    drawArcher(
                        archer->m_sprite, archer->m_sequence, archer->m_frame,
                        drawX, drawY, 0,
                        !archer->m_facing,
                        archer->m_sequence == COMBAT_ARCHER_ACTIVE_SEQUENCE
                            && m_actingSide == COMBAT_ARCHER_DEFENDING_SIDE
                            && m_actingSlot == archer->m_armySlot);
                }
            }

            drawWall(image, 0, 0, image->getWidth(), image->getHeight(),
                     traits.m_x, traits.m_y);
        }
    }
}

// Dreamcast drawing.cpp:1581. Complete's /Ob2 build folds the corpse walk
// into DrawFrame; retaining the helper keeps the original local lifetime and
// statement boundary visible to the compiler. Ordinary auto-inlining retains
// all exact callers; no explicit inline keyword is needed or evidenced.

void combatManager::drawDeadOccupants(int index)
{
    hexcell& cell = m_cells[index];
    for (int body = 0; body < cell.m_bodiesInHex; body++) {
        army* dead = cell.getDeadArmy(body);
        if (cell.m_deadPartOfDouble[body] != dead->m_facing)
            dead->drawToBuffer(cell.m_refX, cell.m_refY, 0);
    }
}

VA(0x00494f40, 0x147)  // dc 0x85844
void combatManager::drawOccupant(int index, int drawPriority,
                                 int numBoxOnly)
{
    if (!validHex(index))
        return;

    const hexcell& hex = m_cells[index];
    int row = gridY(index);
    army* occupant = hex.getArmy();
    if (drawPriority != COMBAT_DRAW_PRIORITY_ANY
            && drawPriority != occupant->m_drawPriority)
        return;
    if (occupant->m_letsPretendImNotHere)
        return;
    if (hex.m_partOfDouble == occupant->m_facing)
        return;

    occupant->drawToBuffer(hex.m_refX, hex.m_refY, numBoxOnly);
    if (!m_moatOn)
        return;
    if (row == COMBAT_GATE_ROW && m_drawbridgeState != DRAWBRIDGE_UP)
        return;
    if (drawPriority == COMBAT_DRAW_PRIORITY_SINGLE_PASS)
        return;

    if (index == g_moatHexes[row]
            || (m_moatIsWide && index == g_innerMoatHexes[row]))
        drawMoatOverlay(index);

    if (occupant->is(creatureDoubleWide)) {
        int front = index + occupant->offsetToFront(-1);
        if (front == g_moatHexes[row]
                || (m_moatIsWide && front == g_innerMoatHexes[row]))
            drawMoatOverlay(front);
    }

    occupant->drawToBuffer(hex.m_refX, hex.m_refY, 1);
}

VA(0x00495090, 0x114)  // dc 0x85978
int combatManager::drawArcher(const CSprite* sprite, int sequence, int frame,
                              int x, int y, SLimitData* limits,
                              bool isFlipped,
                              unsigned char colorRow)
{
    SLimitData computedLimits;
    if (!limits)
        limits = &computedLimits;

    if (m_saveBiggestExtent || m_limitToExtent) {
        computeExtent(sprite, sequence, frame, x, y, limits, isFlipped,
                      m_saveBiggestExtent);
        if (m_computeExtentOnly)
            return 0;
    }

    if (m_limitToExtent) {
        if (!limits->intersects(m_drawbridgeBounds))
            return 0;
    }

    unsigned int paletteIndex = 0;
    if (colorRow)
        paletteIndex = 96;
    Bitmap16Bit* screen = g_windowManager->m_screenBitmap;
    sprite->drawCreature(
        sequence, frame, 0, 0, sprite->getWidth(), 232, screen,
        x, y, isFlipped, g_systemPalette->m_data[paletteIndex]);
    return 1;
}

VA(0x004951b0, 0xfd)  // dc 0x85a48
int combatManager::drawCreature(const CSprite* sprite, int sequence, int frame,
                                int x, int y, SLimitData* limits, int id,
                                bool isFlipped, int color)
{
    SLimitData computedLimits;
    if (!limits)
        limits = &computedLimits;

    if (m_saveBiggestExtent || m_limitToExtent) {
        computeExtent(sprite, sequence, frame, x, y, limits, isFlipped,
                      m_saveBiggestExtent);
        if (m_computeExtentOnly)
            return 0;
    }

    if (m_limitToExtent) {
        if (!limits->intersects(m_drawbridgeBounds))
            return 0;
    }

    Bitmap16Bit* screen = g_windowManager->m_screenBitmap;
    sprite->drawCreature(
        sequence, frame, 0, 0, sprite->getWidth(), sprite->getHeight(), screen,
        x, y, isFlipped, color);
    return 1;
}

// Original: combatManager::DrawCreatureAlpha; drawing.cpp:1738, dc 0x85b50.
int combatManager::drawCreatureAlpha(const CSprite* sprite, int sequence,
    int frame, int x, int y, SLimitData* limits, bool isFlipped, int color)
{
    SLimitData computedLimits;
    if (!limits)
        limits = &computedLimits;
    if (m_saveBiggestExtent || m_limitToExtent) {
        computeExtent(sprite, sequence, frame, x, y, limits, isFlipped,
                      m_saveBiggestExtent);
        if (m_computeExtentOnly)
            return 0;
    }
    if (m_limitToExtent) {
        if (!limits->intersects(m_drawbridgeBounds))
            return 0;
    }
    sprite->drawCreatureAlpha(sequence, frame, 0, 0,
        sprite->getWidth(), sprite->getHeight(), g_windowManager->m_screenBitmap,
        x, y, isFlipped, static_cast<unsigned short>(color));
    return 1;
}

VA(0x004952b0, 0xfb)  // dc 0x85c2c
int combatManager::drawCombatHero(const CSprite* sprite, int sequence,
                                  int frame, int x, int y,
                                  SLimitData* limits,
                                  bool isFlipped)
{
    SLimitData computedLimits;
    if (!limits)
        limits = &computedLimits;

    if (m_saveBiggestExtent || m_limitToExtent) {
        computeExtent(sprite, sequence, frame, x, y, limits, isFlipped,
                      m_saveBiggestExtent);
        if (m_computeExtentOnly)
            return 0;
    }

    if (m_limitToExtent) {
        if (!limits->intersects(m_drawbridgeBounds))
            return 0;
    }

    Bitmap16Bit* screen = g_windowManager->m_screenBitmap;
    sprite->drawCombatHero(
        sequence, frame, 0, 0, sprite->getWidth(), sprite->getHeight(), screen,
        x, y, isFlipped);
    return 1;
}

VA(0x004953b0, 0x144)  // dc 0x85d00
int combatManager::drawSpellEffect(const CSprite* sprite, int frame,
                                   int x, int y,
                                   bool isFlipped,
                                   bool isAlpha)
{
    SLimitData limits(x, y, x + sprite->getWidth() - 1,
                      y + sprite->getHeight() - 1);
    limits.clip(g_combatDrawLimits694f18);

    // DC drawing.cpp:1809 calls ScrollTo before extent accumulation.
    // Complete expands the fixed-viewport helper without emitted code.
    scrollTo(limits, true, true, true);

    if (m_saveBiggestExtent) {
        m_drawbridgeBounds.include(limits);
    }

    if (m_computeExtentOnly)
        return 0;

    if (m_limitToExtent) {
        if (!limits.intersects(m_drawbridgeBounds))
            return 0;
    }

    Bitmap16Bit* screen = g_windowManager->m_screenBitmap;
    sprite->drawSpellEffect(
        0, frame, 0, 0, sprite->getWidth(), limits.m_maxY - y + 1,
        screen, x, y, isFlipped, isAlpha);
    return 1;
}

VA(0x00495500, 0x142)  // dc 0x85e3c
int combatManager::drawSpriteObject(const CSprite* sprite, int frame,
                                    int x, int y,
                                    bool isFlipped)
{
    SLimitData limits(x, y, x + sprite->getWidth() - 1,
                      y + sprite->getHeight() - 1);

    limits.clip(g_combatDrawLimits694f18);

    if (m_saveBiggestExtent) {
        m_drawbridgeBounds.include(limits);
    }

    if (m_computeExtentOnly)
        return 0;

    if (m_limitToExtent) {
        if (!limits.intersects(m_drawbridgeBounds))
            return 0;
    }

    Bitmap16Bit* screen = g_windowManager->m_screenBitmap;
    sprite->draw(
        0, frame, 0, 0, sprite->getWidth(), limits.m_maxY - y + 1,
        screen, x, y, isFlipped, true);
    return 1;
}

VA(0x00495650, 0xD9)  // dc 0x85f1c
int combatManager::drawCreatureAndHeroSubwindows()
{
    if (m_combatWindow->m_heroSubWindows[0]->isShown())
        m_combatWindow->m_heroSubWindows[0]->draw(
            0, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
    if (m_combatWindow->m_heroSubWindows[1]->isShown())
        m_combatWindow->m_heroSubWindows[1]->draw(
            0, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
    // Complete adds four creature-panel tests at 0x49569b..0x495710.
    // DC 0x85f1c calls only the two hero-panel IsShown accessors; neither
    // its procedure roster nor its (forward-only) creature-panel type
    // proves the reconstruction's additional creature accessor.
    if (m_combatWindow->m_creatureSubWindows[0]->m_shown)
        m_combatWindow->m_creatureSubWindows[0]->draw(
            0, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
    if (m_combatWindow->m_creatureSubWindows[1]->m_shown)
        m_combatWindow->m_creatureSubWindows[1]->draw(
            0, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
    if (m_combatWindow->m_creatureSubWindows[2]->m_shown)
        m_combatWindow->m_creatureSubWindows[2]->draw(
            0, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
    if (m_combatWindow->m_creatureSubWindows[3]->m_shown)
        m_combatWindow->m_creatureSubWindows[3]->draw(
            0, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
    return 1;
}

VA(0x00495730, 0x73)  // dc 0x85f70
int combatManager::drawObstacle(const hexcell& cell)
{
    TObstacle& obstacle = m_obstacles[cell.m_obstacleIndex];
    int yOffset = 42 * (obstacle.m_shape->m_minRow - 1);
    return drawSpriteObject(
        obstacle.m_sprite,
        m_obstacleAnimationFrame % obstacle.m_sprite->getNumFrames(0),
        cell.m_hexUlx, cell.m_hexUly - yOffset, 0);
}

VA(0x004957b0, 0x125)  // dc 0x85fd4
int combatManager::drawWall(const Bitmap816* image, int x, int y,
                            int width, int height, int dx, int dy)
{
    SLimitData limits(dx, dy, dx + width - 1, dy + height - 1);

    limits.clip(g_combatDrawLimits694f18);

    if (m_saveBiggestExtent) {
        m_drawbridgeBounds.include(limits);
    }

    if (m_computeExtentOnly)
        return 0;

    if (m_limitToExtent) {
        if (!limits.intersects(m_drawbridgeBounds))
            return 0;
    }

    image->draw(x, y, width, limits.m_maxY - dy + 1,
                g_windowManager->m_screenBitmap, dx, dy, true);
    return 1;
}

VA(0x004958e0, 0x129)  // dc 0x86098
int combatManager::drawObject(const Bitmap816* image, int x, int y)
{
    SLimitData limits(x, y,
                      x + image->getWidth() - 1,
                      y + image->getHeight() - 1);

    limits.clip(g_combatDrawLimits694f18);

    if (m_saveBiggestExtent) {
        m_drawbridgeBounds.include(limits);
    }

    if (m_computeExtentOnly)
        return 0;

    if (m_limitToExtent) {
        if (!limits.intersects(m_drawbridgeBounds))
            return 0;
    }

    image->draw(0, 0, image->getWidth(), limits.m_maxY - y + 1,
                g_windowManager->m_screenBitmap, x, y, true);
    return 1;
}

VA(0x00495a10, 0x1d7)  // dc 0x861cc
int combatManager::drawMoatOverlay(int index)
{
    const hexcell& cell = m_cells[index];
    const TWallTraits& traits =
        s_wallTraits[m_defendingTown->m_type][WALL_TRAITS_ROW_MOAT];
    SLimitData moatExtent(cell.m_hexUlx, cell.m_hexUly + 36,
                           cell.m_hexUlx + 43, cell.m_hexUly + 41);
    moatExtent.clip(g_combatDrawLimits694f18);

    Bitmap816* image = m_combatIcons[WALL_TRAITS_ROW_MOAT][0];
    if (!image)
        return 0;

    SLimitData imageExtent(
        traits.m_x, traits.m_y,
        traits.m_x + image->getWidth() - 1,
        traits.m_y + image->getHeight() - 1);
    moatExtent.clip(imageExtent);
    if (moatExtent.isEmpty())
        return 0;

    if (m_saveBiggestExtent)
        m_drawbridgeBounds.include(moatExtent);
    if (m_computeExtentOnly)
        return 0;
    if (m_limitToExtent && !moatExtent.intersects(m_drawbridgeBounds))
        return 0;

    int sourceX;
    if (traits.m_x < moatExtent.m_minX)
        sourceX = moatExtent.m_minX - traits.m_x;
    else {
        sourceX = 0;
        moatExtent.m_minX = traits.m_x;
    }
    int sourceY;
    if (traits.m_y < moatExtent.m_minY)
        sourceY = moatExtent.m_minY - traits.m_y;
    else {
        sourceY = 0;
        moatExtent.m_minY = traits.m_y;
    }

    image->draw(sourceX, sourceY,
                moatExtent.width(), moatExtent.height(),
                g_windowManager->m_screenBitmap,
                moatExtent.m_minX, moatExtent.m_minY, true);
    return 1;
}

VA(0x00495bf0, 0x35e)  // dc 0x86380
void combatManager::computeMaxExtent()
{
    int side;
    for (side = 0; side < 2; side++) {
        for (int slot = 0; slot < 20; slot++) {
            if (m_creatureEffect[side][slot]) {
                m_saveBiggestExtent = 1;
                m_computeExtentOnly = 1;
                m_armies[side][slot].drawToBuffer(
                    m_cells[m_armies[side][slot].m_gridIndex].m_refX,
                    m_cells[m_armies[side][slot].m_gridIndex].m_refY, 0);
                m_computeExtentOnly = 0;
                m_saveBiggestExtent = 0;
            }
        }
    }

    for (side = 0; side < 2; side++) {
        if (m_heroEffect[side] || m_flagEffect[side]) {
            computeExtent(m_creatureSprites[side], m_cmbtHeroFrameType[side],
                          m_cmbtHeroFrameIndex[side], side ? 693 : -43, -19,
                          &m_cmbtHeroLimitData[side], side != 0, 1);
            computeExtent(m_heroFlagSprites[side], 0,
                          m_cmbtHeroFlagFrame[side],
                          side ? 755 : 29, 20,
                          &m_cmbtHeroFlagLimitData[side], 0, 1);
        }
    }

    if (m_obstacles.size()) {
        for (TObstacle* obstacle = m_obstacles.begin();
             obstacle != m_obstacles.end(); obstacle++) {
            CSprite* sprite = obstacle->m_sprite;
            if (sprite && sprite->getNumFrames(0) > 1) {
                int yOffset = 42 * (obstacle->m_shape->m_minRow - 1);
                computeExtent(
                    sprite, 0, m_obstacleAnimationFrame % sprite->getNumFrames(0),
                    m_cells[obstacle->m_hex].m_hexUlx,
                    m_cells[obstacle->m_hex].m_hexUly - yOffset,
                    0, 0, 1);
            }
        }
    }

    for (int archerIndex = 0; archerIndex < 3; archerIndex++) {
        if (m_archerEffect[archerIndex] && m_archers[archerIndex].m_sprite) {
            m_saveBiggestExtent = 1;
            m_computeExtentOnly = 1;
            TArcher& archer = m_archers[archerIndex];
            int drawX;
            if (!archer.m_facing) {
                drawX = archer.m_x - archer.m_sprite->getWidth()
                         + COMBAT_ARCHER_X_BIAS;
                if (g_creatureTypeTraits[archer.m_creatureType].m_attributes
                        & COMBAT_ARCHER_DOUBLE_WIDE_ATTRIBUTE)
                    drawX += COMBAT_WALL_HEX_WIDTH;
                if (archer.m_creatureType == CREATURE_MEDUSA)
                    drawX -= 5;
            } else {
                drawX = archer.m_x - COMBAT_ARCHER_X_BIAS;
                if (g_creatureTypeTraits[archer.m_creatureType].m_attributes
                        & COMBAT_ARCHER_DOUBLE_WIDE_ATTRIBUTE)
                    drawX -= COMBAT_WALL_HEX_WIDTH;
                if (archer.m_creatureType == CREATURE_MEDUSA)
                    drawX += 5;
            }
            drawArcher(archer.m_sprite, archer.m_sequence, archer.m_frame,
                       drawX, archer.m_y - COMBAT_ARCHER_Y_BIAS, 0,
                       !archer.m_facing, 0);
            m_computeExtentOnly = 0;
            m_saveBiggestExtent = 0;
        }
    }

    m_drawbridgeBounds.clip(g_combatDrawLimits694f18);
}

VA(0x00495f50, 0x17c)  // dc 0x866ac
void combatManager::computeExtent(const CSprite* sprite, int sequence,
                                  int frame, int x, int y,
                                  SLimitData* limits, int isFlipped,
                                  bool saveBiggestExtent)
{
    SLimitData dummy;
    if (!limits)
        limits = &dummy;

    if (isFlipped) {
        limits->m_minX = x + sprite->getWidth()
            - sprite->getCroppedX(sequence, frame)
            - sprite->getCroppedWidth(sequence, frame);
        limits->m_maxX = x + sprite->getWidth()
            - sprite->getCroppedX(sequence, frame) - 1;
    } else {
        limits->m_minX = x + sprite->getCroppedX(sequence, frame);
        limits->m_maxX = x + sprite->getCroppedX(sequence, frame)
            + sprite->getCroppedWidth(sequence, frame) - 1;
    }
    limits->m_minY = y + sprite->getCroppedY(sequence, frame);
    limits->m_maxY = y + sprite->getCroppedY(sequence, frame)
        + sprite->getCroppedHeight(sequence, frame) - 1;

    limits->clip(g_combatDrawLimits694f18);
    if (saveBiggestExtent)
        m_drawbridgeBounds.include(*limits);
}

VA(0x004960d0, 0x76a)  // dc 0x867bc
void combatManager::cycleCombatScreen()
{
    resetLimitCreature();

    int side;
    for (side = 0; side < 2; side++) {
        if (m_heroFlagSprites[side] && m_heroes[side]) {
            m_cmbtHeroFlagFrame[side] =
                (m_cmbtHeroFlagFrame[side] + 1)
                    % m_heroFlagSprites[side]->getNumFrames(0);
            m_flagEffect[side] = 1;
        }
    }

    int selectorGroup;
    int selectorIndex;
    if (m_lastMovedArmy) {
        selectorGroup = m_lastMovedArmy->m_combatSide;
        selectorIndex = m_lastMovedArmy->m_bitIndex;
    } else {
        selectorGroup = -1;
        selectorIndex = -1;
    }

    int highlighterGroup;
    int highlighterIndex;
    if (m_highlighterOn) {
        highlighterGroup = m_cells[m_highlighterIndex].m_armySide;
        highlighterIndex = m_cells[m_highlighterIndex].m_armySlot;
    } else {
        highlighterGroup = -1;
        highlighterIndex = -1;
    }

    unsigned char cyclingCreatures = 0;
    unsigned char cycleMonster[2][20];
    memset(cycleMonster, 0, sizeof(cycleMonster));
    for (side = 0; side < 2; side++) {
        for (int slot = 0; slot < m_numArmies[side]; slot++) {
            army* stack = &m_armies[side][slot];
            if (!(stack->is(creatureImmobilized))
                    && !stack->isIncapacitated()
                    && stack->m_creatureType != army::ARMY_CREATURE_ARROW_TOWER
                    && (stack->m_currFrameType == cs_fidget
                        || (stack->m_currFrameType == cs_wait
                            && stack->m_stdIcon->getNumFrames(cs_fidget) > 0
                            && GameTime::elapsedSince(stack->m_lastFidgetTime)
                                > stack->m_monFrameInfo.m_fidgetFrequency))) {
                markCreatureEffect(side, slot);
                cyclingCreatures = 1;
                cycleMonster[side][slot] = 1;
            }
            if (stack->isInAreaHighlight())
                markCreatureEffect(side, slot);
        }
    }

    if (selectorGroup != -1 && selectorIndex != -1)
        markCreatureEffect(selectorGroup, selectorIndex);
    if (highlighterGroup != -1 && highlighterIndex != -1)
        markCreatureEffect(highlighterGroup, highlighterIndex);

    int nextCmbtHeroFrameType[2];
    for (side = 0; side < 2; side++) {
        nextCmbtHeroFrameType[side] = -1;
        if (!m_creatureSprites[side])
            continue;

        if (m_cmbtHeroFrameType[side] == COMBAT_HERO_FRAME_EVENT_2
                || m_cmbtHeroFrameType[side] == COMBAT_HERO_FRAME_EVENT_3
                || m_cmbtHeroFrameType[side] == COMBAT_HERO_FRAME_FIDGET) {
            m_heroEffect[side] = 1;
        } else if (m_cmbtHeroFrameType[side] == COMBAT_HERO_FRAME_IDLE
                   && !m_dohPlayedThisRound[side] && m_playDoh[side]) {
            int player = m_playerIds[m_currentSide];
            if (player != -1 && g_game->isLocalHuman(player)) {
                m_playYeah[side] = 0;
                m_playDoh[side] = 0;
                m_dohPlayedThisRound[side] = 1;
                if (m_creatureSprites[side]->getNumFrames(
                        COMBAT_HERO_FRAME_EVENT_2) > 0) {
                    nextCmbtHeroFrameType[side] =
                        COMBAT_HERO_FRAME_EVENT_2;
                    m_heroEffect[side] = 1;
                }
            } else {
                m_playYeah[side] = 0;
                m_playDoh[side] = 0;
            }
        } else if (m_cmbtHeroFrameType[side] == COMBAT_HERO_FRAME_IDLE
                   && !m_yeahPlayedThisRound[side] && m_playYeah[side]) {
            int player = m_playerIds[m_currentSide];
            if (player != -1 && g_game->isLocalHuman(player)) {
                m_playYeah[side] = 0;
                m_playDoh[side] = 0;
                m_yeahPlayedThisRound[side] = 1;
                if (m_creatureSprites[side]->getNumFrames(
                        COMBAT_HERO_FRAME_EVENT_3) > 0) {
                    m_heroEffect[side] = 1;
                    nextCmbtHeroFrameType[side] =
                        COMBAT_HERO_FRAME_EVENT_3;
                }
            } else {
                m_playYeah[side] = 0;
                m_playDoh[side] = 0;
            }
        } else if (m_cmbtHeroFrameType[side] == COMBAT_HERO_FRAME_IDLE
                   && GameTime::elapsedSince(
                          m_cmbtHeroLastFidgetTime[side]) > 4500) {
            m_heroEffect[side] = 1;
            nextCmbtHeroFrameType[side] = COMBAT_HERO_FRAME_FIDGET;
        }
    }

    computeMaxExtent();

    if (cyclingCreatures) {
        for (side = 0; side < 2; side++) {
            for (int slot = 0; slot < m_numArmies[side]; slot++) {
                if (!cycleMonster[side][slot])
                    continue;

                army* stack = &m_armies[side][slot];
                if (stack->m_currFrameType == cs_wait) {
                    stack->m_currFrameType = cs_fidget;
                    stack->m_currFrameIndex = 0;
                    continue;
                }

                if (stack->m_monFrameInfo.m_fidgetFrequency
                        && safeRandom(0, 100) >= 8)
                    stack->m_currFrameIndex++;
                if (stack->m_currFrameIndex
                        >= stack->m_stdIcon->getNumFrames(cs_fidget)) {
                    stack->m_currFrameType = cs_wait;
                    stack->m_currFrameIndex = 0;
                    stack->m_lastFidgetTime = GameTime::get();
                    if (stack->m_monFrameInfo.m_fidgetFrequency > 0) {
                        stack->m_lastFidgetTime = static_cast<unsigned long>(
                            stack->m_lastFidgetTime
                            + (safeRandom(
                                   0, stack->m_monFrameInfo.m_fidgetFrequency) * 0.5
                               - stack->m_monFrameInfo.m_fidgetFrequency * 0.25));
                    }
                }
            }
        }
    }

    for (side = 0; side < 2; side++) {
        if (!m_heroEffect[side])
            continue;
        if (nextCmbtHeroFrameType[side] != -1) {
            m_cmbtHeroFrameType[side] = nextCmbtHeroFrameType[side];
            m_cmbtHeroFrameIndex[side] = 0;
        } else {
            m_cmbtHeroFrameIndex[side]++;
            if (m_cmbtHeroFrameIndex[side]
                    >= m_creatureSprites[side]->getNumFrames(
                           m_cmbtHeroFrameType[side])) {
                m_cmbtHeroFrameType[side] = COMBAT_HERO_FRAME_IDLE;
                m_cmbtHeroFrameIndex[side] = 0;
                m_cmbtHeroLastFidgetTime[side] = GameTime::get();
            }
        }
    }

    m_obstacleAnimationFrame++;
    g_systemPalette->cycle(96, 103, -1);
    g_systemPalette->cycle(112, 119, -1);
    drawFrame(1, 1, 0, 0, 1, 0);
    GameTime::get();
    g_timers[8] =
        GameTime::nextFrameTime(g_timers[8], 100);
}

VA(0x00496840, 0x1c5)  // dc 0x86ea0
void combatManager::spellEffect(int effect, army* targetArmy, int delay,
                                bool doWince)
{
    if (static_cast<const combatManager*>(this)->isQuickCombat())
        return;
    if (effect == -1)
        return;
    if (effect < 0)
        return;
    if (effect >= 83)
        return;
    if (!g_spellEffectTraits[effect].m_name)
        return;

    if (targetArmy->m_currFrameType == cs_wince)
        doWince = 0;

    if (loadSpellEffect(effect))
        targetArmy->m_showPowEffect = 1;

    int frame = 0;
    if (doWince) {
        targetArmy->m_currFrameType = cs_wince;
        while (frame < targetArmy->m_stdIcon->getNumFrames(cs_wince)) {
            targetArmy->m_currFrameIndex = frame;
            if (frame < m_powSprite->getNumFrames(cs_walk))
                m_powFrameIndex = frame;
            else
                m_powFrameIndex = m_powSprite->getNumFrames(cs_walk);
            drawFrame(1, 0, 0, 100, 1, 1);
            frame++;
        }
        targetArmy->m_currFrameType = cs_wait;
        targetArmy->m_currFrameIndex = 0;
        if (frame >= m_powSprite->getNumFrames(cs_walk))
            drawFrame(1, 0, 0, 0, 1, 0);
    }

    playImmEffect(g_spellEffectTraits[effect].m_immName, 1);
    while (frame < m_powSprite->getNumFrames(cs_walk)) {
        m_powFrameIndex = frame;
        drawFrame(1, 0, 0, delay, 1, 1);
        frame++;
    }
    targetArmy->m_showPowEffect = 0;
    drawFrame(1, 0, 0, delay, 1, 1);
}

VA(0x00496a10, 0x23d)  // dc 0x8703c
void combatManager::spellEffect(int effect, int hex, int delay,
                                bool leaveLastFrame)
{
    if (static_cast<const combatManager*>(this)->isQuickCombat())
        return;
    if (effect == -1)
        return;
    if (effect < 0)
        return;
    if (effect >= 83)
        return;

    TSpellEffectTraits traits = g_spellEffectTraits[effect];
    if (!traits.m_name)
        return;

    loadSpellEffect(effect);

    int x;
    int y;
    switch (traits.m_flags & 0xf) {
    case SPELL_EFFECT_PLACE_OVERHEAD:
        x = m_cells[hex].m_refX - m_powSprite->getWidth() / 2;
        y = m_cells[hex].m_hexUly - m_powSprite->getHeight() + 52;
        break;
    case SPELL_EFFECT_PLACE_CENTERED:
        x = m_cells[hex].m_refX - m_powSprite->getWidth() / 2;
        y = m_cells[hex].m_refY - m_powSprite->getHeight() / 2 - 37;
        break;
    case SPELL_EFFECT_PLACE_HEX:
        x = m_cells[hex].m_hexUlx;
        y = m_cells[hex].m_hexUly - m_powSprite->getHeight() + 52;
        break;
    }

    playImmEffect(g_spellEffectTraits[effect].m_immName, 1);
    for (int frame = 0; frame < m_powSprite->getNumFrames(cs_walk); frame++) {
        drawFrame(0, 0, 0, delay, 1, 1);

        Bitmap16Bit* screen = g_windowManager->m_screenBitmap;
        m_powSprite->drawSpellEffect(
            0, frame, 0, 0, m_powSprite->getWidth(), m_powSprite->getHeight(),
            screen, x, y, false, (traits.m_flags >> 8) & 1);
        // DC drawing.cpp:2650 uses the CSprite bitmap overload; 2653/2654
        // update only when ScrollTo did not redraw the viewport.
        if (!scrollTo(x, y, m_powSprite->getWidth(), m_powSprite->getHeight(),
                      true, true, true))
            updateCombatArea();
    }

    if (!leaveLastFrame)
        drawFrame(1, 0, 0, delay, 1, 1);
}

#if 0  // @carcass

// E:\gamedcs\drawing.cpp:2093
// RETAIL_LOCATED(0x00495bf0, 0x35e): not reconstructed; anchor-global, dc 0x86380
void combatManager::computeMaxExtent()
{
    // @stub
}

// E:\gamedcs\drawing.cpp:2214
// RETAIL_LOCATED(0x00495f50, 0x17c): not reconstructed; dc-bracket forced, dc 0x866ac
void combatManager::computeExtent(const CSprite* sprite, int sequence, int frame, int x, int y, SLimitData* psLimitData, int isFlipped, unsigned char SaveBiggestExtent)
{
    // @stub
}

// E:\gamedcs\drawing.cpp:2257
// RETAIL_LOCATED(0x004960d0, 0x76a): not reconstructed; anchor-global, dc 0x867bc
void combatManager::cycleCombatScreen()
{
    // @stub
}

#endif  // @carcass
