// cmbtmgr.cpp - E:\gamedcs\cmbtmgr.cpp (compiland cmbtmgr.obj)

// LOCATE SWEEP 2026-08-08 (span 0x62760..0x6a45c, 72 carve rows). Each
// bracket between two already-proven claims was order-mapped onto the
// DC roster and every pairing checked against `ret N` (thiscall, so
// ret N == 4*(params-1) against the DC `params` column, which counts
// `this`), the callee graph and the string pool. The high-confidence
// anchors are the ones that carry their own filename: 0x62990 reads
// walls.txt through ResourceManager::GetSpreadsheet
// (LoadWallTraitsTable), 0x63370 loads CCELLGRD.PCX (LoadIcons),
// 0x646d0 builds CMBKBOAT.PCX (GetBackgroundName), 0x64920 plays
// GOODMRLE.WAV and 0x64b40 BADMRLE.WAV (the two CheckApply*Morale
// bodies, both `ret 8` for their three parameters), 0x65330 plays
// MANADRAI.WAV (SetNextArmy). 0x639f0 is settled outright by arity:
// `ret 0x2c` is eleven stack words plus this, and SetupCombat is the
// compiland's only 12-parameter function.

// DC rows proven bodyless in retail: CheckNativeTerrain (0x5e948),
// CombineGroups (0x5f1d0), ComputeDamageModifier (0x6021c, four bytes
// on SH4), ExperienceValueOfStack (0x60220 - single call site in
// CalculateGainedExperience), TestRaiseDoor (0x610e0),
// DoorCanBeLowered (0x63268), and FreeArmies
// (0x5e3d8, whose only caller is Close).

#include <math.h>
#include <stdlib.h>

#include <va.h>
#include "DC_precompiledheaders.h"  // canonical reference selectors
// PowEffect's own surface: its declarator and TSpellEffectID from
// cmbtmgr.h, the five animation-state bytes plus iPostPowSpellToCast
// and bPowSequenceComplete from army.h, the death sequence from
// csprite.h and the Immersion hook from game.h.
#include "advmgr.h"  // advManager::MoreTreesNear, for GetBackgroundName
#include "herospec.h"  // TSecondarySkill, for the skillLevel slot names
#include "bitmap816.h"
                                     // it in InitNonVisualVars' siege arms
#include "cmbtmgr.h"
#include "combatwindow.h"
#include "combatoptionswindow.h"
#include "creaturetype.h" // UpgradedCreatureType, for RaiseSkeletons
#include "csprite.h"  // CSprite::Dispose, for RemoveObstacle
#include "drawing.h"  // gCombatAreaLimits / gCombatSpeedFactors, for the
                      // missile animators
#include "game.h"     // gpGame ruleset gate, for RaiseSkeletons
#include "hero.h"   // hero::IsWieldingArtifact, for ShotIsThroughWall
#include "findpath.h" // searchArray::lower_door, for LowerDoor
#include "kb.h"   // gText, the shared combat-message scratch buffer
#include "kbwin.h"  // bVideoPaused storage, the network-game gate here
#include "inputmgr.h" // gpInputManager, for Open
#include "misc.h"   // TPickANumber, for PlaceAllObstacles
#include "monframeinfo.h" // gMonFrameInfo, the shot table KeepAttack times from
#include "prefs.h"  // the local quick-combat preference
#include "mapcell.h"
#include "resourcemanager.h"
#include "sample.h"   // TResourceHandle<sample>::~TResourceHandle calls resource::Dispose
#include "soundmgr.h" // SAMPLE2 / LoadPlaySample / WaitEndSample
#include "mousemgr.h" // gpMouseManager / SetPointer / ShowPointer, for Open
#include "remote.h"
#include "remotedlg.h" // CNetMsgHandlerPause, the pause handler Open installs
#include "textresource.h"
#include "town.h"   // TTownType, for IsInMoat's Fortress row
#include "viewarmywindow.h"
#include "widget.h"  // WIDGET_DIMMED / WIDGET_UPDATE, for Open
#include "winmgr.h"

VA(0x00462760, 0x127)  // dc 0x5d3e0
CombatManager::CombatManager()
    : m_combatWindow(0)
{
    m_actingSide = 1;
    m_actingSlot = -1;
    m_currentSide = 1;
    m_highlighterIndex = -1;
    m_saveBiggestExtent = 0;
    m_limitToExtent = 0;
    m_computeExtentOnly = 0;
    m_lastMovedArmy = 0;
    m_highlighterOn = 0;
    m_combatCommand = 0;
    m_fortificationLevel = COMBAT_FORTIFICATION_NONE;
    m_combatShowIt = 0;
    m_netMsgHandlerPause = 0;
}

VA_COMPGEN(0x00462890, 0x15, DEFAULT_CTOR_CLOSURE, set)
VA_COMPGEN(0x004628b0, 0x6E, IMPLICIT_DTOR, set)

// The manager constructor takes these callbacks for three 0x24-byte
// TArcher rows. The two canonical resource handles generate both bodies:
// 0x462920 clears +4/+8; 0x462930 releases +8 then +4, including cleanup
// of the second member if the first release throws. CodeView's older
// TArcher uses raw Sprite/Missile pointers and has no written constructor.
VA_COMPGEN(0x00462920, 0x0B, CLASS_CTOR, Archer)
VA_COMPGEN(0x00462930, 0x54, IMPLICIT_DTOR, Archer)

VA(0x00462990, 0x8F)  // dc 0x5d538
unsigned char CombatManager::loadWallTraitsTable()
{
    SpreadsheetResource* sheet = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x0066fec0, wallsSpreadsheetName, "walls.txt"));
    if (!sheet)
        return 0;
    if (sheet->getNumberOfRows() < 179) {
        sheet->dispose();
        return 0;
    }

    int row = 1;
    for (int townType = 0; townType < 9; townType++) {
        row += 2;
        for (int wall = 0; wall < 18; wall++) {
            const SpreadsheetResource::TStringVector& values =
                sheet->getRow(row);
            s_wallTraits[townType][wall].m_name = values[0];
            s_wallTraits[townType][wall].m_hitpoints =
                static_cast<short>(atoi(values[1]));
            row++;
        }
    }
    return 1;
}

// Two asymmetries in the bytes that are real and not transcription
// slips: showCombatMouseHex is saved and cleared UNCONDITIONALLY at the
// top but restored only inside the last !IsQuickCombat() block, and
// gpMouseManager->field_38 is raised unconditionally and lowered only
// there too. A quick combat therefore leaves both changed.

// EH states run 0..4, one per `new`, with -1 between them - the frame
// exists only to run operator delete if a constructor throws, since
// there is no STL and no string anywhere in the body.

VA(0x00462a20, 0x83F)  // dc 0x5d60c
int CombatManager::open(int newPriority)
{
    SAMPLE2 sample;

    g_mouseManager->m_noChangePointer = 1;
    int savedShowMouseHex = g_unnamed698758.m_showCombatMouseHex;
    g_unnamed698758.m_showCombatMouseHex = 0;
    m_combatShowIt = 0;
    g_soundManager->stopAllSamples(1);

    if (!isQuickCombat()) {
        char name[20];
        sprintf(name,
                DATA_COMPGEN(0x0066fee8, battleSampleFormat,
                             "battle%02d.wav"),
                random(1, 8) - 1);
        sample = loadPlaySample(name);
        g_windowManager->fadeScreen(1, 4, 1);
    }

    m_saveScreenPreGrid = new Bitmap16Bit(683, 472);
    m_saveScreenPostGrid = new Bitmap16Bit(800, 600);
    m_combatMouseBackground = new Bitmap16Bit(855, 52);

    loadIcons();
    initializeArchers();
    initNonVisualVars();
    setupAndLoadObstacles();

    memset(m_lastDrawGridShade, 0, COMBAT_GRID_CELLS);
    memset(m_curDrawGridShade, 0, COMBAT_GRID_CELLS);

    m_backgroundDrawn = 0;
    g_combatActive698a18 = m_combatCycleType;
    m_powSprite = 0;
    m_powSpellEffect = -1;

    int leftTactics = m_heroes[0] ? m_heroes[0]->m_skillLevel[19] : 0;
    int rightTactics = m_heroes[1] ? m_heroes[1]->m_skillLevel[19] : 0;
    int tacticsSide = rightTactics > leftTactics;
    m_placementBoundaryDepth = leftTactics - rightTactics;
    m_creaturePlacement = m_placementBoundaryDepth != 0 && !m_isSurrounded;
    if (m_creaturePlacement && !isQuickCombat() && m_sideIsAi[tacticsSide]
            && !(m_heroes[tacticsSide]->m_formation & 2))
        m_creaturePlacement = 0;

    m_turnNumber = 0;
    m_currentSide = 1;
    m_actingSide = 1;
    m_actingSlot = 0;
    g_chatMan.pauseTimeOuts();
    m_combatShowIt = 1;

    if (isQuickCombat()) {
        m_combatWindow = 0;
    } else {
        m_combatWindow = new CombatWindow(m_creaturePlacement);
        if (!m_combatWindow)
            memError();
        g_windowManager->addWindow(m_combatWindow, -1, 1);
    }

    updateArmyLuckAndMorale();
    m_inSecondPhase = 0;
    if (m_creaturePlacement && !isQuickCombat()) {
        m_currentSide = m_placementBoundaryDepth <= 0;
        if (!g_game->isLocalHuman(m_playerIds[m_currentSide])) {
            m_combatWindow->widgetSetStatus(
                0x7d9, Widget::WIDGET_DIMMED | Widget::WIDGET_UPDATE);
            m_combatWindow->widgetSetStatus(
                0x7802, Widget::WIDGET_DIMMED | Widget::WIDGET_UPDATE);
        }
    }

    if (!isQuickCombat()) {
        drawFrame(1, 0, 0, 0, 1, 0);
        m_combatWindow->drawWindow(1, -65535, 65535);
        g_combatStamp698998 = GameTime::get();
        kbChangeMenu(g_gameMenu);
        CheckMenuItem(g_activeMenu, 0xb798, 0);
        CheckMenuItem(g_activeMenu, 0xb79c, 0);
        CheckMenuItem(g_activeMenu, 0xb79b, 0);
        g_windowManager->updateScreen(0, 0, 800, 600);
        g_unnamed698758.m_showCombatMouseHex = savedShowMouseHex;
        g_mouseManager->m_noChangePointer = 0;
        g_mouseManager->setPointer(6, MouseManager::COMBAT_SET);
        g_mouseManager->showPointer(0);
        g_windowManager->fadeScreen(0, 4, 0);
        waitEndSample(sample, 10000);

        char music[100];
        sprintf(music,
                DATA_COMPGEN(0x0066fedc, combatMusicFormat, "combat%02d"),
                random(1, 4));
        g_soundManager->startMP3(music, 0, 1);
    }

    g_combatStamp6989b8 = GameTime::get();
    resetCycleTimers();
    g_inputManager->flush();
    resetMouse();

    m_combatCommand = 0;
    m_priority = newPriority;
    m_id = 0x200;
    m_status = 1;
    strcpy(m_mgrName,
           DATA_COMPGEN(0x0066fecc, combatManagerName, "combatManager"));

    if (m_creaturePlacement
            && g_game->isLocalHuman(m_playerIds[m_currentSide])
            && g_game->m_players[m_playerIds[m_currentSide]]
                   .m_placementHelpEnabled
            && !isQuickCombat()) {
        normalDialogTimeOut(
            g_generalText->getText(GENERAL_TEXT_COMBAT_PLACEMENT_HELP),
            1, 10000, -1, -1, -1, 0, -1, 0, -1, -1, 0);
        g_game->m_players[m_playerIds[m_currentSide]].m_placementHelpEnabled = 0;
    }

    g_chatMan.resumeTimeOuts();
    m_netMsgHandlerPause = new CNetMsgHandlerPause();
    nextArmy(0);
    return 0;
}

VA(0x00463260, 0x105)  // dc 0x5dc70
void CombatManager::close()
{
    if (!isQuickCombat())
        g_windowManager->fadeScreen(1, 4, 1);

    g_combatActive698a18 = 0;
    delete m_saveScreenPreGrid;
    delete m_saveScreenPostGrid;
    delete m_combatMouseBackground;
    if (m_combatWindow)
        g_windowManager->removeWindow(m_combatWindow);
    g_soundManager->stopAllSamples(1);
    freeIcons();
    if (m_combatWindow) {
        delete m_combatWindow;
        m_combatWindow = 0;
    }
    delete m_netMsgHandlerPause;
    m_status = 0;
    m_combatShowIt = 0;
}

// Declared file-locally rather than by pulling border.h/button.h in: an
// extern declaration is include-set inert where a whole header is not.
void setPlayerPaletteColors(unsigned short* pal, int whichPlayer);

VA(0x00463370, 0x18D)  // dc 0x5ddc0
void CombatManager::loadIcons()
{
    m_combatCellGridBitmap = ResourceManager::getBitmap816(
        DATA_COMPGEN(0x0066ff30, combatCellGridBitmapName, "ccellgrd.pcx"));
    m_combatShadowBitmap = ResourceManager::getBitmap816(
        DATA_COMPGEN(0x0066ff20, combatShadowBitmapName, "ccellshd.pcx"));
    m_combatGridBitmap = ResourceManager::getBitmap816(
        DATA_COMPGEN(0x0066ff10, combatGridBitmapName, "CmNumWin.pcx"));

    if (m_fortificationLevel > 0) {
        WallTraits* traits = s_wallTraits[m_defendingTown->m_type];
        for (int wall = 0; wall < 18; wall++) {
            for (int icon = 0; icon < 5; icon++) {
                if ((g_game->m_f1f698 >= 2
                        || m_defendingTown->m_type != TOWN_STRONGHOLD
                        || wall != WALL_TRAITS_ROW_MOAT)
                        && traits[wall].m_filenames[icon] != 0)
                    m_combatIcons[wall][icon] = ResourceManager::getBitmap816(
                        traits[wall].m_filenames[icon]);
                else
                    m_combatIcons[wall][icon] = 0;
            }
        }
    } else {
        memset(m_combatIcons, 0, sizeof(m_combatIcons));
    }

    for (int side = 0; side < 2; side++) {
        m_cmbtHeroFrameType[side] = 0;
        m_cmbtHeroFrameIndex[side] = 0;
        if (m_heroes[side]) {
            m_creatureSprites[side] = ResourceManager::getSprite(
                g_combatHeroSprites[
                    2 * g_heroClasses[m_heroes[side]->m_heroClass].m_townType
                    + g_heroTraits[m_heroes[side]->m_id].m_sex].m_defName);
            m_heroFlagSprites[side] = ResourceManager::getSprite(side == 0
                ? DATA_COMPGEN(0x0066ff04, leftFlagSpriteName, "CmFlagL.def")
                : DATA_COMPGEN(0x0066fef8, rightFlagSpriteName, "CmFlagR.def"));
            setPlayerPaletteColors(m_heroFlagSprites[side]->getPalette(),
                m_playerIds[side]);
        } else {
            m_creatureSprites[side] = 0;
            m_heroFlagSprites[side] = 0;
        }
    }
}

VA(0x00463500, 0xFD)  // dc 0x5dfb4
void CombatManager::freeIcons()
{
    for (int group = 0; group < 18; ++group) {
        for (int icon = 0; icon < 5; ++icon) {
            if (m_combatIcons[group][icon])
                m_combatIcons[group][icon]->dispose();
        }
    }

    for (Obstacle* obstacle = m_obstacles.begin();
            obstacle != m_obstacles.end(); ++obstacle) {
        if (obstacle->m_sprite)
            obstacle->m_sprite->dispose();
    }
    m_obstacles.erase(m_obstacles.begin(), m_obstacles.end());

    for (int side = 0; side < 2; ++side) {
        if (m_creatureSprites[side])
            m_creatureSprites[side]->dispose();
        if (m_heroFlagSprites[side])
            m_heroFlagSprites[side]->dispose();
    }

    loadSpellEffect(-1);
    m_combatGridBitmap->dispose();
    m_combatCellGridBitmap->dispose();
    m_combatShadowBitmap->dispose();
}

// E:\gamedcs\cmbtmgr.cpp:1004
// Deployment. Both halves of every war-machine block are byte-forced:
// which side may have the machine at all, and the hex it lands on.
//   * the catapult is gated on `fortification > 0 && side == 0` and goes
//     into armies[0] with no side term in its address, which is why it is
//     spelled armies[0] here and not armies[side] - retail's own source
//     evidently said so too;
//   * the ballista is refused to a DEFENDER of a fortified town
//     (`!fortification || side != 1`), which is the siege rule;
//   * the three artifact machines' hexes are `side ? A : B` ternaries -
//     retail materialises them with the neg/sbb/and/add idiom that pairs
//     two constants differing by one mask, not with arithmetic on side;
//   * the arrow towers are gated on `side == 1` and address armies[1] the
//     same way the catapult addresses armies[0]. A Citadel installs the
//     keep alone, a Castle adds the two towers with (n+1)/2 shots each -
//     which is exactly what InitializeArchers was already documented to
//     prove from the other end.
// The deployment tables' bounds come from adjacency, not from guessing a
// stride: see their declaration in the header.

// TWO SPELLINGS HERE ARE BYTE-FORCED AND LOOK REDUNDANT IN SOURCE; both
// were measured, and both are retail's:
//   * `group` is a local for GetNumArmies and for Init's two arguments,
//     but the loop's empty-slot test re-reads `armyGroups[side]` from the
//     member. Using the local for the test too costs 99.35 -> 95.90 -
//     retail really does load the pointer once into a frame slot AND
//     re-read the member at the top of each iteration.
//   * the hex lookup is hoisted out of the tight/loose arms into a
//     separate `ordinal`, because retail forms the `7*side` index ONCE
//     after the two arms rejoin; writing the two-level index inside each
//     arm duplicates it and costs 95.90 -> 94.18.

// DREAMCAST LOCALS RESTORED 2026-08-30, byte-flat at 99.35275%. Raw NB11
// places procedure-scope `side` before the outer loop, then records const
// unsigned-char `grouped` and const-int `layout` in the side scope. The
// occupied-slot scope records `hex` followed by an `army& thisArmy`; its
// Init and LoadResources calls are source-visible in the SH4 line/xref
// stream. The prior names (`tight_formation`, `last`) and repeated direct
// array receiver were a score-neutral flattening. Fatal rules now preserve
// these five names, types, declaration scopes/order and receiver boundary.

VA(0x00463600, 0x3D8)  // anchor-callee, dc 0x5e09c
void CombatManager::loadArmies(unsigned char isSurrounded)
{
    int side;
    for (side = 0; side < 2; side++) {
        for (int slot = 0; slot < 20; slot++) {
            m_armies[side][slot].m_numTroops = 0;
            m_armies[side][slot].m_creatureType = CREATURE_NONE;
            m_armies[side][slot].initClean();
        }
        m_numArmies[side] = 0;
        int placed = 0;
        Hero* combatHero = m_heroes[side];
        ArmyGroup* group = m_armyGroups[side];
        const unsigned char grouped =
            combatHero && (combatHero->m_formation & 1) && m_sideIsAi[side];
        const int layout = group->getNumArmies() - 1;
        for (int i = 0; i < ArmyGroup::ARMY_GROUP_SLOT_COUNT; i++) {
            if (m_armyGroups[side]->m_armies[i] == CREATURE_NONE)
                continue;
            int hex;
            Army& thisArmy = m_armies[side][placed];
            if (isSurrounded) {
                hex = g_combatDeploySurroundedHexes63d0e0[side][placed];
            } else {
                int ordinal;
                if (grouped)
                    ordinal = g_combatDeploySlots63d1dc[layout][placed];
                else
                    ordinal = g_combatDeploySlots63d118[layout][placed];
                hex = g_combatDeployHexes63d0a8[side][ordinal];
            }
            thisArmy.init(group->m_armies[i], group->m_numTroops[i], combatHero,
                          side, placed, hex, i);
            thisArmy.loadResources();
            placed++;
        }
        if (combatHero && !isSurrounded) {
            if (m_fortificationLevel > 0 && side == 0) {
                m_armies[0][placed].init(CREATURE_CATAPULT, 1, combatHero,
                                       side, placed, 0x77, -1);
                m_armies[0][placed].loadResources();
                placed++;
            }
            if (combatHero->isWieldingArtifact(ARTIFACT_BALLISTA)) {
                if (!m_fortificationLevel || side != 1) {
                    m_armies[side][placed].init(CREATURE_BALLISTA, 1,
                                              combatHero, side, placed,
                                              side ? 0x43 : 0x33, -1);
                    m_armies[side][placed].loadResources();
                    placed++;
                }
            }
            if (combatHero->isWieldingArtifact(ARTIFACT_FIRST_AID_TENT)) {
                m_armies[side][placed].init(CREATURE_FIRST_AID_TENT, 1,
                                          combatHero, side, placed,
                                          side ? 0xa9 : 0x99, -1);
                m_armies[side][placed].loadResources();
                placed++;
            }
            if (combatHero->isWieldingArtifact(ARTIFACT_AMMO_CART)) {
                m_armies[side][placed].init(CREATURE_AMMO_CART, 1, combatHero,
                                          side, placed, side ? 0x20 : 0x12,
                                          -1);
                m_armies[side][placed].loadResources();
                placed++;
            }
        }
        if (side == 1 && m_fortificationLevel >= COMBAT_FORTIFICATION_CITADEL) {
            int numArchers;
            int archerLevel;
            m_defendingTown->calcNumLevelArchers(&numArchers, &archerLevel);
            m_armies[1][placed].init(CREATURE_ARROW_TOWER, numArchers,
                                   combatHero, side, placed,
                                   COMBAT_HEX_KEEP, -1);
            m_archers[0].m_armySlot = placed;
            placed++;
            if (m_fortificationLevel == COMBAT_FORTIFICATION_CASTLE) {
                numArchers = (numArchers + 1) / 2;
                m_armies[1][placed].init(CREATURE_ARROW_TOWER, numArchers,
                                       combatHero, side, placed,
                                       COMBAT_HEX_UPPER_TOWER, -1);
                m_archers[2].m_armySlot = placed;
                placed++;
                m_armies[1][placed].init(CREATURE_ARROW_TOWER, numArchers,
                                       combatHero, side, placed,
                                       COMBAT_HEX_LOWER_TOWER, -1);
                m_archers[1].m_armySlot = placed;
                placed++;
            }
        }
        m_numArmies[side] = placed;
    }
}

#if 0  // @carcass

// E:\gamedcs\cmbtmgr.cpp:1151
DC_ONLY(0x5e3d8, 0x8C)
void CombatManager::FreeArmies()
{
    // @stub
}

#endif  // @carcass

VA(0x004639e0, 0x0E)
void CombatManager::stopCombatSounds()
{
    g_soundManager->stopAllSamples(1);
}

VA(0x004639f0, 0x270)  // dc 0x5e464
void CombatManager::setupCombat(type_point point, Hero* leftHero, ArmyGroup* leftArmyGroup, long rightPlayer, Town* rightTown, Hero* rightHero, ArmyGroup* rightArmyGroup, int x, int y, int seed, unsigned char isSurrounded)
{
    g_combatSeed66d840 = seed;
    sRand(x * 0x1aed3 + y * 0x28f79 + 0x13ea1);
    m_mapPoint = point;
    m_combatCell = g_advManager->getCell(point);
    m_autoRetreatOn = 1;
    if (leftHero)
        m_playerIds[0] = leftHero->m_owner;
    else
        m_playerIds[0] = -1;
    m_playerIds[1] = rightPlayer;
    m_heroes[0] = leftHero;
    m_armyGroups[0] = leftArmyGroup;
    m_heroes[1] = rightHero;
    m_armyGroups[1] = rightArmyGroup;
    for (int side = 0; side < 2; side++) {
        if (m_playerIds[side] >= 0) {
            m_sideIsAi[side] = g_game->isHuman(m_playerIds[side]);
            m_sideIsLocalHuman[side] = g_game->isLocalHuman(m_playerIds[side]);
            m_hasAngelicAlliance[side] =
                g_game->m_players[m_playerIds[side]].hasGivenArtifact(0x81);
        } else {
            m_sideIsAi[side] = 0;
            m_sideIsLocalHuman[side] = 0;
            m_hasAngelicAlliance[side] = 0;
        }
        m_artifactCast[side] = 1;
        m_spellsCast[side] = 0;
        m_turnSinceLastEnchanter[side] = 30000;
    }
    if (rightTown) {
        if (rightTown->hasBuilding(CASTLE_FORT_ID, false)) {
            m_fortificationLevel = COMBAT_FORTIFICATION_FORT;
            m_moatIsWide = 0;
            m_moatOn = 0;
        } else if (rightTown->hasBuilding(CASTLE_CITADEL_ID, false)) {
            m_fortificationLevel = COMBAT_FORTIFICATION_CITADEL;
            m_moatOn = rightTown->m_type != TOWN_TOWER
                         && (rightTown->m_type != TOWN_STRONGHOLD
                             || g_game->m_f1f698 >= 2);
            m_moatIsWide = rightTown->m_type == TOWN_FORTRESS;
        } else if (rightTown->hasBuilding(CASTLE_CASTLE_ID, false)) {
            m_fortificationLevel = COMBAT_FORTIFICATION_CASTLE;
            m_moatOn = rightTown->m_type != TOWN_TOWER
                         && (rightTown->m_type != TOWN_STRONGHOLD
                             || g_game->m_f1f698 >= 2);
            m_moatIsWide = rightTown->m_type == TOWN_FORTRESS;
        } else {
            m_fortificationLevel = COMBAT_FORTIFICATION_NONE;
            m_moatIsWide = 0;
            m_moatOn = 0;
        }
        m_drawbridgeState = DRAWBRIDGE_UP;
        m_defendingTown = rightTown;
    } else {
        m_fortificationLevel = COMBAT_FORTIFICATION_NONE;
        m_moatIsWide = 0;
        m_moatOn = 0;
        m_defendingTown = 0;
    }
    determineCombatTerrain();
    m_isSurrounded = isSurrounded;
    m_backgroundName = getBackgroundName();
}

// E:\gamedcs\cmbtmgr.cpp:1348
// Everything about the combat that is NOT a screen object: the two
// heroes' cached combat statistics, the town's own siege bonuses, the
// per-side latches, the grid, the armies.

// The earlier survey called the middle of this body a blocker - "a
// three-argument __cdecl call, f(&ret, *root, root), which is a
// Dinkumware set::erase(first, last) returning an iterator by value" -
// and said modelling it was "a bigger piece of work than the rest of
// this function put together". IT IS `clear()`. The three pushes are
// the hidden return slot, `_Left(_Head)` and `_Head`, which is exactly
// what VC6's `void clear() { erase(begin(), end()); }` expands to, and
// eagleEyeData was already backed by std::set<SpellID> in this
// header. The DC roster names the member outright (std::set<SpellID>::
// clear, dc 0x63b98). No new STL surface at all.

// The town arm is a real six-entry JUMP TABLE on `defendingTown->type`,
// not a compare chain, and slot 2 (Necropolis) points at the default -
// VC6 built a dense 2..7 table over five live cases. Stronghold's arm
// FALLS THROUGH into the Fortress arm; that is a case without a break
// in the source, not a shared tail.

// Dreamcast names the four clamp sites as hero::GetPrimarySkill and every
// town statistic award as hero::AdjustPrimarySkill. Their Hero.h bodies
// retain the byte load and high-bound-first clamp / byte add that retail
// expands; a generic int clamp instead widens too early and loses three
// conditional branches.

VA(0x00463c60, 0x43C)  // anchor-callee, dc 0x5e690
void CombatManager::initNonVisualVars()
{
    m_debugNoSpellLimit = 0;
    m_debugShowHiddenObjects = 0;
    m_debugShowBlockedHexes = 0;
    m_autoCombatOn = 0;
    m_battleOver = 0;
    m_anyActionTaken = 0;
    m_castleAttackDone = 0;

    if (m_heroes[1]) {
        m_originalAttackSkill = m_heroes[1]->getPrimarySkill(0);
        m_originalDefenseSkill = m_heroes[1]->getPrimarySkill(1);
        m_originalPowerSkill = m_heroes[1]->getPrimarySkill(2);
        m_originalMana = m_heroes[1]->m_mana;

        if (m_defendingTown) {
            switch (m_defendingTown->m_type) {
            case TOWN_TOWER:
                if (m_defendingTown->hasBuilding(HOLY_GRAIL_ID, 0))
                    m_heroes[1]->m_mana = static_cast<short>(
                        m_heroes[1]->m_mana + 150);
                break;
            case TOWN_INFERNO:
                if (m_defendingTown->hasBuilding(EXTRA_0_ID, 1))
                    m_heroes[1]->adjustPrimarySkill(2, 2);
                break;
            case TOWN_DUNGEON:
                if (m_defendingTown->hasBuilding(HOLY_GRAIL_ID, 0))
                    m_heroes[1]->adjustPrimarySkill(2, 12);
                break;
            case TOWN_STRONGHOLD:
                if (m_defendingTown->hasBuilding(HOLY_GRAIL_ID, 0))
                    m_heroes[1]->adjustPrimarySkill(0, 20);
                break;
            case TOWN_FORTRESS:
                if (m_defendingTown->hasBuilding(EXTRA_0_ID, 1))
                    m_heroes[1]->adjustPrimarySkill(0, 2);
                if (m_defendingTown->hasBuilding(EXTRA_1_ID, 1))
                    m_heroes[1]->adjustPrimarySkill(1, 2);
                if (m_defendingTown->hasBuilding(HOLY_GRAIL_ID, 1)) {
                    m_heroes[1]->adjustPrimarySkill(0, 10);
                    m_heroes[1]->adjustPrimarySkill(1, 10);
                }
                break;
            }
        }
    } else {
        m_originalAttackSkill = 0;
        m_originalDefenseSkill = 0;
        m_originalPowerSkill = 0;
        m_originalMana = 0;
    }

    m_spellPower[0] = m_heroes[0] ? m_heroes[0]->getPrimarySkill(2) : 0;
    m_spellPower[1] = m_heroes[1] ? m_heroes[1]->getPrimarySkill(2) : 0;

    m_cmbtHeroFlagFrame[0] = 0;
    m_cmbtHeroFlagFrame[1] = 3;
    m_dohPlayedThisRound[0] = m_dohPlayedThisRound[1] = 0;
    m_yeahPlayedThisRound[0] = m_yeahPlayedThisRound[1] = 0;
    m_playDoh[0] = m_playDoh[1] = 0;
    m_playYeah[0] = m_playYeah[1] = 0;

    m_eagleEyeData[0].clear();
    m_eagleEyeData[1].clear();

    m_nextAction = 0;
    m_summonedElemental[0] = -1;
    m_summonedElemental[1] = -1;
    m_lastCellIndex = -1;
    m_lastCommand = -99;
    m_currentSide = 1;
    m_actingSide = 1;
    m_actingSlot = 0;
    g_combatFlag6985a3 = 0;
    g_combatFlag697744 = 0;
    m_sideSurrendered[0] = 0;
    m_sideSurrendered[1] = 0;
    m_sideRetreated[0] = 0;
    m_sideRetreated[1] = 0;
    m_winner = 3;
    m_lastMovedArmy = 0;
    turnOffHighlighter(0);

    setupAdjacencyArray();
    generateMap();
    loadArmies(m_isSurrounded);

    for (int side = 0; side < 2; side++) {
        m_onNativeTerrain[side] = 0;
        for (int slot = 0; slot < m_numArmies[side]; slot++) {
            if (m_armies[side][slot].m_onNativeTerrain) {
                m_onNativeTerrain[side] = 1;
                break;
            }
        }
    }
}

#if 0  // @carcass

// E:\gamedcs\cmbtmgr.cpp:1498
DC_ONLY(0x5e948, 0x64)
void CombatManager::CheckNativeTerrain()
{
    // @stub
}

#endif  // @carcass

VA(0x004640a0, 0x144)  // dc 0x5e9ac
void CombatManager::setupAdjacencyArray()
{
    int adjacent;
    for (int hex = 0; hex < COMBAT_GRID_CELLS; hex++) {
        int row = hex / COMBAT_GRID_ROW_STRIDE;
        int column = hex % COMBAT_GRID_ROW_STRIDE;

        for (int direction = 0; direction < COMBAT_DIRECTION_COUNT;
                direction++) {
            if (validHex(hex)
                    && (column == 0
                        || column == COMBAT_GRID_LAST_COLUMN)) {
                m_adjacentCells[hex][direction] = -1;
                if (column == 0) {
                    if (direction >= COMBAT_DIRECTION_3)
                        continue;
                } else if (direction <= COMBAT_DIRECTION_2) {
                    continue;
                }
            }

            switch (direction) {
            case COMBAT_DIRECTION_0:
                adjacent = (row & 1) ? hex - 17 : hex - 16;
                break;
            case COMBAT_DIRECTION_1:
                adjacent = hex + 1;
                break;
            case COMBAT_DIRECTION_2:
                adjacent = (row & 1) ? hex + 17 : hex + 18;
                break;
            case COMBAT_DIRECTION_3:
                adjacent = (row & 1) ? hex + 16 : hex + 17;
                break;
            case COMBAT_DIRECTION_4:
                adjacent = hex - 1;
                break;
            case COMBAT_DIRECTION_5:
                adjacent = (row & 1) ? hex - 18 : hex - 17;
                break;
            }

            if (validHex(adjacent)) {
                int adjacentColumn = adjacent % COMBAT_GRID_ROW_STRIDE;
                if (adjacentColumn != 0
                        && adjacentColumn != COMBAT_GRID_LAST_COLUMN) {
                    m_adjacentCells[hex][direction] =
                        static_cast<short>(adjacent);
                    continue;
                }
            }
            m_adjacentCells[hex][direction] = -1;
        }
    }
}
VA(0x004641f0, 0xDA)  // dc 0x5eb40
void CombatManager::updateArmyGroup(int whichSide)
{
    for (int slot = 0; slot < ArmyGroup::ARMY_GROUP_SLOT_COUNT; slot++) {
        m_armyGroups[whichSide]->m_armies[slot] = CREATURE_NONE;
        m_armyGroups[whichSide]->m_numTroops[slot] = 0;
    }

    for (int index = 0; index < m_numArmies[whichSide]; index++) {
        Army& current = m_armies[whichSide][index];
        if (current.m_numTroops <= 0)
            continue;

        if (current.is(1u << 21))
            continue;
        if (m_playerIds[whichSide] != -1) {
            if (current.is(1u << 22))
                continue;
        }
        if (current.is(1u << 23))
            continue;
        if (current.is(1u << 6))
            continue;
        if (current.m_originalIndex < 0
            || current.m_originalIndex >= ArmyGroup::ARMY_GROUP_SLOT_COUNT)
            continue;

        m_armyGroups[whichSide]->m_armies[current.m_originalIndex] =
            current.m_creatureType;
        m_armyGroups[whichSide]->m_numTroops[current.m_originalIndex] =
            current.m_numTroops;
    }
}

VA(0x004642d0, 0xDC)  // dc 0x5eca8
void CombatManager::generateMap()
{
    int y;
    for (y = 0; y < 11; y++) {
        for (int x = 0; x < 17; x++) {
            Hexcell* cell = &getCell(x, y);
            cell->m_refX = static_cast<short>(
                x * 44 + (rowIsOdd(y) ? 22 : 44) + 14);
            cell->m_refY = static_cast<short>(y * 42 + 128);
            cell->m_hexUlx = static_cast<short>(
                x * 44 + (rowIsOdd(y) ? 0 : 22) + 14);
            cell->m_hexUly = static_cast<short>(y * 42 + 86);
            cell->m_hexBrx = static_cast<short>(cell->m_hexUlx + 44);
            cell->m_hexBry = static_cast<short>(cell->m_hexUly + 42);
            cell->m_fullHexBry = static_cast<short>(cell->m_hexUly + 52);
            cell->m_armySide = -1;
            cell->m_armySlot = -1;
            cell->m_partOfDouble = -1;
            cell->m_obstacleIndex = -1;
            cell->m_attributes = 0;
            cell->m_bodiesInHex = 0;
            cell->m_backgroundOffset = -1;
            cell->m_mouseShaded = 0;
        }
    }
}

VA(0x004643b0, 0x317)  // dc 0x5ed68
void CombatManager::determineCombatTerrain()
{
    m_magicTerrain = -1;
    m_onBoats = 0;
    m_onAntiMagicGarrison = 0;

    int terrain;
    if (m_defendingTown) {
        terrain = m_defendingTown->getNativeTerrain();
    } else if ((m_heroes[0] && (m_heroes[0]->m_flags & 0x40000))
            || (m_heroes[1] && (m_heroes[1]->m_flags & 0x40000))
            || (m_combatCell->getMapObject() == SHIPWRECK
                && m_combatCell->m_isTrigger)
            || (m_combatCell->getMapObject() == DERELICT_SHIP
                && m_combatCell->m_isTrigger)) {
        terrain = eTerrainWater;
        m_onBoats = 1;
    } else if (m_mapPoint.m_z > 0) {
        terrain = COMBAT_TERRAIN_SUBTERRANEAN;
    } else if (m_combatCell->getMapObject() == MINE
            && m_combatCell->m_isTrigger
            && (g_game->m_mines[m_combatCell->getMapExtraInfo()].m_type
                    == COMBAT_MINE_TYPE_6
                || g_game->m_mines[m_combatCell->getMapExtraInfo()].m_type
                    == COMBAT_MINE_TYPE_4
                || g_game->m_mines[m_combatCell->getMapExtraInfo()].m_isAbandoned)) {
        terrain = COMBAT_TERRAIN_SUBTERRANEAN;
    } else if (m_combatCell->m_flags0011 & 0x200) {
        m_magicTerrain = 0;
        terrain = COMBAT_TERRAIN_SAND;
    } else {
        terrain = m_combatCell->m_groundSet;
    }
    m_terrainType = terrain;

    switch (m_combatCell->getSpecialTerrain()) {
    case MAGIC_PLAINS:
        m_magicTerrain = 1;
        break;
    case CURSED_GROUND:
        m_magicTerrain = 2;
        break;
    case HOLY_GROUND:
        m_magicTerrain = 3;
        break;
    case EVIL_FOG:
        m_magicTerrain = 4;
        break;
    case CLOVER_FIELD_2:
        m_magicTerrain = 5;
        break;
    case LUCID_POOLS:
        m_magicTerrain = 6;
        break;
    case FIERY_FIELDS:
        m_magicTerrain = 7;
        break;
    case ROCKLANDS:
        m_magicTerrain = 8;
        break;
    case MAGIC_CLOUDS:
        m_magicTerrain = 9;
        break;
    case GARRISON:
        m_onAntiMagicGarrison = m_combatCell->m_objectIndex == 1;
        break;
    }
}

VA(0x004646d0, 0xC5)  // dc 0x5ef54
const char* CombatManager::getBackgroundName()
{
    const char* background;
    if (m_fortificationLevel > 0) {
        background = g_townCombatBackgrounds[m_defendingTown->m_type];
    } else {
        int magicTerrain = m_magicTerrain;
        if (magicTerrain != -1 && magicTerrain != 0) {
            background = g_magicTerrainCombatBackgrounds[magicTerrain];
        } else {
            unsigned int boatFlag = 0x40000;
            if (m_heroes[0] && (m_heroes[0]->m_flags & boatFlag)
                && m_heroes[1] && (m_heroes[1]->m_flags & boatFlag)) {
                background = DATA_COMPGEN(
                    0x0066ff5c, boatCombatBackground, "CmBkBoat.pcx");
            } else if (m_onBoats) {
                background = DATA_COMPGEN(
                    0x0066ff4c, deckCombatBackground, "CmBkDeck.pcx");
            } else if (magicTerrain == 0) {
                background = DATA_COMPGEN(
                    0x0066ff40, beachCombatBackground, "CmBkBch.pcx");
            } else {
                int nearbyTrees = g_advManager->moreTreesNear(m_mapPoint);
                int combatTerrain = m_terrainType;
                background = g_terrainCombatBackgrounds[combatTerrain]
                    [nearbyTrees];
            }
        }
    }

    m_combatCycleType = 1;
    m_combatFringe = -1;
    return background;
}
VA(0x004647a0, 0x17A)  // dc 0x5f058
int CombatManager::getGridIndex(int x, int y) const
{
    if (g_combatHexLeft694f08 <= x && x <= g_combatHexRight694f10
            && g_combatHexTop694f0c <= y
            && y <= g_combatHexBottom694f14)
        return 252;
    if (g_combatHexLeft694ef0 <= x && x <= g_combatHexRight694ef8
            && g_combatHexTop694ef4 <= y
            && y <= g_combatHexBottom694efc)
        return 253;
    if (g_combatHexLeft694ea8 <= x && x <= g_combatHexRight694eb0
            && g_combatHexTop694eac <= y
            && y <= g_combatHexBottom694eb4)
        return 254;
    if (g_combatHexLeft694ed8 <= x && x <= g_combatHexRight694ee0
            && g_combatHexTop694edc <= y
            && y <= g_combatHexBottom694ee4)
        return 255;

    int px = x - 14;
    int py = y - 86;
    int row = py / 42;
    if ((row & 1) == 0)
        px -= 22;
    int col = px / 44;
    if (px < 0 || px >= 748 || py < 0 || py >= 472)
        return -1;
    int depth = py % 42;
    if (depth < 10) {
        int across = px % 44;
        if (depth < abs(across - 22) / 2) {
            row--;
            if (across < 22) {
                if ((row & 1) == 0)
                    col--;
            } else if ((row & 1) != 0) {
                col++;
            }
        }
    }
    if (row < 0 || row >= 11 || col < 0 || col >= 17)
        return -1;
    return row * 17 + col;
}

#if 0  // @carcass

// E:\gamedcs\cmbtmgr.cpp:1971
DC_ONLY(0x5f1d0, 0xDE)
void CombatManager::CombineGroups(ArmyGroup* src, ArmyGroup* dest)
{
    // @stub
}

#endif  // @carcass

// and the creature-name lookup uses CreatureType.h's canonical helper.
VA(0x00464920, 0x211)  // dc 0x5f2b0
void CombatManager::checkApplyGoodMorale(int group, int index)
{
    if (group < 0)
        return;
    if (index < 0)
        return;
    Army* stack = &m_armies[group][index];
    if (m_creaturePlacement)
        return;
    if (stack->is(1u << 27))
        return;
    if (stack->is(1u << 24))
        return;
    if (!stack->m_numTroops)
        return;
    if (random(1, 24) > stack->getMorale(1))
        return;
    stack->m_monInfo.m_attributes = (stack->m_monInfo.m_attributes & ~0x04000000) | 0x01000000;
    if (!isQuickCombat()) {
        SAMPLE2 sample = loadPlaySample(
            DATA_COMPGEN(0x0066ff6c, goodMoraleSampleName, "GoodMrle.wav"));
        spellEffect(20, stack, 100, 0);
        sprintf(g_text, g_generalText->getText(GENERAL_TEXT_GOOD_MORALE),
            getArmyName(stack->m_creatureType, stack->m_numTroops));
        m_combatWindow->combatMessage(g_text, 1, 0);
        waitEndSample(sample, -1);
    }
    updateGrid(0, 1);
    drawFrame(1, 0, 0, 0, 1, 0);
}

VA(0x00464b40, 0x1FB)  // dc 0x5f3c4
int CombatManager::checkApplyBadMorale(int group, int index)
{
    if (group >= 0 && index >= 0) {
        Army* stack = &m_armies[group][index];
        if (random(1, 12) <= -stack->getMorale(1)) {
            if (m_sideIsAi[group] || random(1, 4) != 1) {
                stack->m_monInfo.m_attributes |= 0x04000000;
                if (!isQuickCombat()) {
                    SAMPLE2 sample = loadPlaySample(DATA_COMPGEN(
                        0x0066ff7c, badMoraleSampleName, "BadMrle.wav"));
                    sprintf(g_text,
                        g_generalText->getText(GENERAL_TEXT_BAD_MORALE),
                        getArmyName(stack->m_creatureType, stack->m_numTroops));
                    m_combatWindow->combatMessage(g_text, 1, 0);
                    spellEffect(30, stack, 100, 1);
                    waitEndSample(sample, -1);
                }
                return 1;
            }
        }
    }
    return 0;
}

VA(0x00464d40, 0x20D)
unsigned char CombatManager::unnamed464d40(Army* selected)
{
    if (selected->is(1u << 17))
        return 0;
    if (selected->m_creatureType == CREATURE_AZURE_DRAGON)
        return 0;

    int actualSide;
    if (selected->m_spellInfluence[60])
        actualSide = 1 - selected->m_combatSide;
    else
        actualSide = selected->m_combatSide;
    int opposingSide = 1 - actualSide;

    int azureDragons = 0;
    Army* stack = m_armies[opposingSide];
    for (int count = m_numArmies[opposingSide]; count--; stack++) {
        if (stack->m_creatureType == CREATURE_AZURE_DRAGON)
            azureDragons += stack->m_numTroops;
    }
    if (!azureDragons)
        return 0;
    if (rand() % 10 > 0)
        return 0;

    selected->m_monInfo.m_attributes |= 1 << 26;
    if (!isQuickCombat()) {
        SAMPLE2 sample = loadPlaySample(DATA_COMPGEN(
            0x0066ff88, fearSampleName, "Fear.wav"));
        sprintf(g_text,
                g_generalText->getText(GENERAL_TEXT_COMBAT_FEAR),
                getArmyName(CREATURE_AZURE_DRAGON, azureDragons),
                getArmyName(selected->m_creatureType, selected->m_numTroops));
        m_combatWindow->combatMessage(g_text, 1, 0);
        spellEffect(15, selected, 100, 1);
        waitEndSample(sample, -1);
    }
    return 1;
}

VA(0x00464f50, 0x123)
unsigned char CombatManager::unnamed464f50(
    const Army* incumbent, const Army* candidate)
{
    if ((incumbent->is(1u << 24)) != (candidate->is(1u << 24)))
        return incumbent->is(1u << 24);

    int incumbentSpecial = incumbent->m_creatureType == CREATURE_ARROW_TOWER;
    int candidateSpecial = candidate->m_creatureType == CREATURE_ARROW_TOWER;
    if (incumbentSpecial != candidateSpecial)
        return incumbentSpecial;

    incumbentSpecial = incumbent->m_creatureType == CREATURE_CATAPULT;
    candidateSpecial = candidate->m_creatureType == CREATURE_CATAPULT;
    if (incumbentSpecial != candidateSpecial)
        return incumbentSpecial;

    if (incumbent->getSpeed() != candidate->getSpeed()) {
        if (m_inSecondPhase)
            return incumbent->getSpeed() < candidate->getSpeed();
        return incumbent->getSpeed() > candidate->getSpeed();
    }
    if (incumbent->m_combatSide != candidate->m_combatSide)
        return incumbent->m_combatSide != m_actingSide;
    return incumbent->m_bitIndex < candidate->m_bitIndex;
}

VA(0x00465080, 0x2A2)  // dc 0x5f518
unsigned char CombatManager::nextArmy(unsigned char checkingForBadMorale)
{
    if (m_actingSlot >= 0 && m_actingSide == 0
        && m_armies[0][m_actingSlot].m_creatureType == CREATURE_CATAPULT) {
        m_actingSide = 1;
        m_actingSlot = 0;
    }
    for (int pass = (m_inSecondPhase != 0) + 1; pass <= 2; pass++) {
        while (1) {
            Army* best = 0;
            for (int side = 0; side < 2; side++) {
                for (int i = 0; i < m_numArmies[side]; i++) {
                    Army* stack = &m_armies[side][i];
                    if (stack->is(1u << 26))
                        continue;
                    if (stack->is(1u << 21))
                        continue;
                    if (stack->is(1u << 25))
                        continue;
                    if (stack->m_resetThisRound && stack->isIncapacitated())
                        continue;
                    if (m_creaturePlacement) {
                        if (!stack->m_monInfo.m_speed)
                            continue;
                    }
                    if (m_creaturePlacement) {
                        if (m_placementBoundaryDepth > 0 && side != 0)
                            continue;
                        if (m_placementBoundaryDepth < 0 && side != 1)
                            continue;
                    }
                    if (stack->m_creatureType == CREATURE_AMMO_CART)
                        continue;
                    if (m_creaturePlacement && (stack->is(1u << 6)))
                        continue;
                    if (best && unnamed464f50(best, stack))
                        continue;
                    best = stack;
                }
            }
            if (best) {
                if (!m_inSecondPhase)
                    best->newTurn();
                if (best->m_spellInfluence[62])
                    continue;
                if (best->m_spellInfluence[70])
                    continue;
                if (best->m_spellInfluence[74])
                    continue;
                if (checkingForBadMorale && !m_creaturePlacement
                    && !m_inSecondPhase) {
                    if (checkApplyBadMorale(best->m_combatSide, best->m_bitIndex))
                        continue;
                    if (unnamed464d40(best))
                        continue;
                }
                setNextArmy(best->m_combatSide, best->m_bitIndex);
                return 1;
            }
            break;
        }
        if (pass == 1) {
            m_inSecondPhase = 1;
            checkingForBadMorale = 0;
            for (int s = 0; s < 2; s++) {
                for (int j = 0; j < m_numArmies[s]; j++)
                    m_armies[s][j].m_monInfo.m_attributes &= ~(1 << 25);
            }
        }
    }
    return 0;
}

// E:\gamedcs\cmbtmgr.cpp:2364

// The body is a start-of-turn hook in two halves. The first is the
// combination-artifact auto-cast: while the acting side's field_54b0
// latch is up and that side HAS a hero, five spells are offered, each
// gated on the spells.obj leaf at 0x5a40d0 and cast through CastSpell
// with a delay. Artifact 0x81 offers one spell and 0x84 offers four,
// and the two spell SETS are what identify both artifacts and both
// ids - Prayer alone is the Angelic Alliance, Slow/Curse/Weakness/
// Misfortune is the Armor of the Damned. The latch is then cleared,
// so the whole block fires once per combat and not once per turn.

// The second half is a three-way compare chain on creatureType, and
// the three ids are the creatures with a start-of-turn ability:
// 0x3d drains two mana off the OTHER side's hero (ManaDrai.wav, the
// wraith), 0x86 rolls the weighted random spell at army 0x447510 (the
// faerie dragon) and 0x88 fires a mass cast gated on a three-round
// counter (the enchanter). Every id is argued on the enumerator in
// army.h rather than here.

// Two shapes the bytes force:
//   * the controlling side is computed TWICE through Dreamcast's
//     Army.h `army::get_controlling_side` boundary, once into the member
//     and once again inside the wraith arm. VC6 expands both calls, as
//     retail requires;
//   * the wraith arm's exits are `break`s, not `return`s: all of them
//     land on the shared two-statement tail (lastMovedArmy = 0 and the
//     command-bar rearm), which is also where bCreaturePlacement and
//     the compare chain's default edge go.

// 75.89 -> 87.51% 2026-08-20, and the lever was the INLINER again, the
// same reading place_obstacle's note records. predict-inline named it
// in one line - retail CALLS basic_string::assign in BOTH message arms
// and our /Ob2 expanded it in the plural arm only - and the scoped
// inline_depth(0) on that one statement is the whole fix. Worth
// stressing that the two arms are spelled IDENTICALLY, so no source
// difference explains why the budget ran out between them and nothing
// but the pragma reaches it. inline_depth(1) and (2) were both
// measured and both leave the expansion in place (75.8883, flat), so
// only 0 works.

// DREAMCAST SHAPE RESTORED 2026-08-30, byte-flat at 99.64975%. Raw NB11
// records `result` as the sole non-optimized local inside the mana-drain
// sample scope. Its xrefs prove two Army.h get_controlling_side calls,
// two Army.h GetName calls and the named GetControl tail. The old source
// used a file-local controlling-side expansion, a direct creature-trait
// helper, `message`, and an address-ordinal command helper solely to retain
// the existing bytes. Enabling the original CreatureType.h inline view and
// restoring all five named source boundaries produces the identical x86.
// Three frozen missing-call rows retire; fatal rules now reject putting any
// of those flattened spellings back.

// Residual (99.6498%, rechecked 2026-09-01): 73 of 74 CFG blocks are exact
// and the remaining block differs by one instruction. The two string
// temporaries' frame slots are
// TRANSPOSED (retail singular -0x34 / plural -0x44; ours -0x44/-0x34)
// because the named local allocates in the named-local region while
// retail's two unnamed temps take creation order - ~10 masked lines of
// pure slot renumber; one `lea edx,[ebp-0x34] / push edx` where retail
// propagates the RVO address still in eax (`push eax`); and the
// unwind-table push addend (0x0 vs 0x8), the unscored prologue class.
// All three are the price of the named-local device and no spelling
// that keeps the dtor out of the pin can avoid it.
VA(0x00465330, 0x4F6)  // anchor-global, dc 0x5f934
void CombatManager::setNextArmy(int group, int index)
{
    Army* stack = &m_armies[group][index];
    m_actingSide = group;
    m_actingSlot = index;
    m_currentSide = stack->getControllingSide();
    if (!m_creaturePlacement) {
        if (m_artifactCast[m_currentSide]) {
            Hero* castingHero = m_heroes[m_currentSide];
            if (castingHero) {
                if (castingHero->isWieldingArtifact(
                        ARTIFACT_ANGELIC_ALLIANCE)) {
                    if (hasValidSpellTarget(SPELL_PRAYER, 3, m_currentSide, 1, 2))
                        castSpell(SPELL_PRAYER, -1, 2, -1, 3, 10);
                }
                if (castingHero->isWieldingArtifact(
                        ARTIFACT_ARMOR_OF_THE_DAMNED)) {
                    if (hasValidSpellTarget(SPELL_SLOW, 3, m_currentSide, 1, 2))
                        castSpell(SPELL_SLOW, -1, 2, -1, 3, 50);
                    if (hasValidSpellTarget(SPELL_CURSE, 3, m_currentSide, 1, 2))
                        castSpell(SPELL_CURSE, -1, 2, -1, 3, 50);
                    if (hasValidSpellTarget(SPELL_WEAKNESS, 3, m_currentSide, 1, 2))
                        castSpell(SPELL_WEAKNESS, -1, 2, -1, 3, 50);
                    if (hasValidSpellTarget(SPELL_MISFORTUNE, 3, m_currentSide, 1, 2))
                        castSpell(SPELL_MISFORTUNE, -1, 2, -1, 3, 50);
                }
            }
            m_artifactCast[m_currentSide] = 0;
        }

        switch (stack->m_creatureType) {
        case Army::ARMY_CREATURE_WRAITH:
            if (m_inSecondPhase)
                break;
            {
                Hero* drained = m_heroes[1 - stack->getControllingSide()];
                if (!drained)
                    break;
                if (drained->m_mana <= 0)
                    break;
                drained->m_mana = static_cast<short>(drained->m_mana - 2);
                if (drained->m_mana < 0)
                    drained->m_mana = 0;
                if (!isQuickCombat()) {
                    SAMPLE2 sample = loadPlaySample(DATA_COMPGEN(
                        0x0066ff94, manaDrainSampleName, "ManaDrai.wav"));
                    std::string result;
                    if (stack->m_numTroops == 1)
                        result = formatString(
                            g_generalText->getText(
                                GENERAL_TEXT_COMBAT_MANA_DRAIN_ONE),
                            stack->getName(),
                            drained->m_name);
                    else {
                        // OVER-INLINE, pinned. Retail CALLS
                        // basic_string::assign in BOTH arms; our /Ob2
                        // expands it in this one only - erase x2 plus
                        // _Grow, _Eos and the _Nullstr compare, which is
                        // the whole 53-vs-44 conditional-branch gap. The
                        // asymmetry is the inline budget running out at
                        // a different point, not a spelling difference:
                        // the two arms are written identically.
                        // SPLIT: the temporary named in its own statement
                        // so the pin misses its destructor -
                        // the dtor runs at the brace, outside the pinned
                        // statement, and stays inline as retail has it.
                        std::string many = formatString(
                            g_generalText->getText(
                                GENERAL_TEXT_COMBAT_MANA_DRAIN_MANY),
                            stack->getName(),
                            drained->m_name);
#pragma inline_depth(0)
                        result.assign(many, 0, std::string::npos);
#pragma inline_depth()
                    }
                    if (m_combatWindow)
                        m_combatWindow->combatMessage(result.c_str(), 1, 0);
                    spellEffect(77, stack, 100, 0);
                    waitEndSample(sample, -1);
                }
            }
            break;
        case Army::ARMY_CREATURE_FAERIE_DRAGON:
            stack->faerieDragonSpell();
            break;
        case Army::ARMY_CREATURE_ENCHANTER:
            if (m_turnSinceLastEnchanter[m_currentSide] > 2 && stack->unnamed447fe0())
                m_turnSinceLastEnchanter[m_currentSide] = 0;
            break;
        }
    }
    m_lastMovedArmy = 0;
    getControl();
}

VA(0x00465830, 0x76)  // dc 0x5fb14
unsigned char CombatManager::combatIsOver() const
{
    for (int side = 0; side < 2; side++) {
        if (m_sideSurrendered[side])
            return 1;
        if (m_sideRetreated[side])
            return 1;
        unsigned char hasArmy = 0;
        for (int slot = 0; slot < 20; slot++) {
            const Army& currentArmy = m_armies[side][slot];
            if (currentArmy.m_creatureType == -1)
                continue;
            if (currentArmy.is(1u << 21))
                continue;
            if (currentArmy.is(1u << 6))
                continue;
            hasArmy = 1;
            break;
        }
        if (!hasArmy)
            return 1;
    }
    return 0;
}

VA(0x004658b0, 0xBC)  // dc 0x5fc00
unsigned char CombatManager::isWinner(int thisSide) const
{
    const int otherSide = 1 - thisSide;
    int other;
    bool noStacks = 1;
    for (int slot = 0; slot < 20; slot++) {
        const Army& a = m_armies[thisSide][slot];
        if (a.m_creatureType == -1)
            continue;
        if (a.is(1u << 22))
            continue;
        if (a.is(1u << 6))
            continue;
        if (!a.is(1u << 21)) {
            noStacks = 0;
            break;
        }
    }
    if (noStacks)
        return 0;
    if (!m_sideSurrendered[otherSide] && !m_sideRetreated[otherSide]) {
        for (other = 0; other < 20; other++) {
            const Army& a = m_armies[otherSide][other];
            if (a.m_creatureType == -1)
                continue;
            if (a.is(1u << 21))
                continue;
            if (!a.is(1u << 6))
                return 0;
        }
    }
    return 1;
}

VA(0x00465970, 0x20)  // dc 0x5fcec
int getTargetWallIndex(int gridIndex)
{
    for (int i = 0; i < 8; i++) {
        if (CombatManager::s_wallTargets[i].m_targetHex == gridIndex)
            return i;
    }
    return -1;
}

VA(0x00465990, 0x140)  // dc 0x5fd10
void CombatManager::damageWall(WallTargetId targetWall, int damage)
{
    if (damage <= 0)
        return;

    int strength;
    strength = m_wallStrength[s_wallTargets[targetWall].m_wall] - damage;
    if (strength < 0)
        strength = 0;

    if (strength == 0) {
        switch (targetWall) {
        case WALL_TARGET_1:
        case WALL_TARGET_2:
        case WALL_TARGET_4:
        case WALL_TARGET_5: {
            int blockedHex = s_wallTargets[targetWall].getBlockedHex();
            m_cells[blockedHex].m_attributes &= ~2;
            break;
        }
        case WALL_TARGET_3:
            m_drawbridgeState = 0;
            break;
        case WALL_TARGET_0: {
            int slot = m_archers[2].m_armySlot;
            m_wallStrength[17] = 0;
            m_wallStanding[17] = 0;
            m_armies[1][slot].m_monInfo.m_attributes |= 1 << 21;
            break;
        }
        case WALL_TARGET_6: {
            int slot = m_archers[1].m_armySlot;
            m_wallStrength[16] = 0;
            m_wallStanding[16] = 0;
            m_armies[1][slot].m_monInfo.m_attributes |= 1 << 21;
            break;
        }
        case WALL_TARGET_7: {
            int slot = m_archers[0].m_armySlot;
            m_wallStrength[15] = 0;
            m_wallStanding[15] = 0;
            m_armies[1][slot].m_monInfo.m_attributes |= 1 << 21;
            break;
        }
        }
    }

    int wallId = s_wallTargets[targetWall].m_wall;
    m_wallStrength[wallId] = strength;
    if (strength == 0)
        m_wallStanding[wallId] = 0;
    else
        m_wallStanding[wallId] = 1;
}

// E:\gamedcs\cmbtmgr.cpp:2603
// One arrow-tower shot, and the survey it replaces got the PROTOTYPE
// right and two of its four callee readings wrong.

// The parameter is NOT combatManager::TArcherID. It is scaled by 169 and
// added to +0x54cc, i.e. `&armies[0][0] + p * sizeof(army)`, with no
// side term at all - retail hardcodes group 0 because an arrow tower is
// always the defender and only ever shoots side 0. The single caller
// (army::range_attack, 0x4401c3) pushes `this->slot`, so the domain is
// 0..20 and the value names the VICTIM, not the tower. The acting tower
// is read separately, from the (actingSide, actingSlot) pair.

// The three-way gridIndex switch has NO default arm - retail leaves the
// archer index uninitialised for any other hex - and its 0xfe -> 0 /
// 0xfb -> 1 / 0xff -> 2 mapping is what proves field_1402c is an array.

// Residual (95.0344%): FLOW-DISTANCE 0 and the call multisets agree
// exactly, 17 against 17, so nothing structural is left. What remains
// is register binding with the schedule already aligned - eax->ebx x11,
// ecx->eax x8 - which is the register-homing family damage_message's
// note also lands in. why-reg's B6/B14 naming knobs are the only route
// the catalog offers and neither is source-addressable here.

// Two shapes the bytes forced and that are worth not re-litigating:
// the damage accumulator is a DOWN-counted loop over a copy of
// numTroops (retail spills the count and steps it with dec/jne, which
// an up-counted `for (i = 0; i < n; i++)` does not produce), and
// `damage * ComputeDefenderDamageReduction(1)` is in THAT operand
// order - retail materialises and spills the int-to-double conversion
// BEFORE the call, which is the left-operand-first shape.
VA(0x00465ad0, 0x443)  // anchor-callee, dc 0x5feac
void CombatManager::keepAttack(int towerPos)
{
    Army* tower = &m_armies[m_actingSide][m_actingSlot];
    int archerIndex;
    switch (tower->m_gridIndex) {
    case COMBAT_HEX_KEEP:
        archerIndex = 0;
        break;
    case COMBAT_HEX_LOWER_TOWER:
        archerIndex = 1;
        break;
    case COMBAT_HEX_UPPER_TOWER:
        archerIndex = 2;
        break;
    }
    Archer* archer = &m_archers[archerIndex];
    const SMonFrameInfo* info = &g_monFrameInfo[archer->m_creatureType];
    Army* target = &m_armies[0][towerPos];

    SAMPLE2 sample;
    int armyDir;
    int delay;
    int startX;
    int startY;
    int missileFrame;

    if (!isQuickCombat()) {
        int destX = target->midX();
        const int destY = target->midY();
        archer->m_facing = destX >= archer->m_x;
        getMissileStartingPosition(archer->m_creatureType, archer->m_x,
                                   archer->m_y, archer->m_facing, destX,
                                   destY, archer->m_shadowSprite,
                                   &startX, &startY, &armyDir,
                                   &missileFrame);
        sprintf(g_text,
                DATA_COMPGEN(0x0066ffa4, towerShotSampleFormat,
                             "%sshot.82m"),
                g_creatureTypeTraits[archer->m_creatureType].m_samplePrefix);
        sample = loadPlaySample(g_text);

        int frames = info->m_attackFrames;
        if (frames <= 0)
            frames = archer->m_sprite->getNumFrames(armyDir);
        archer->m_sequence = armyDir;
        delay = info->m_attackStartCycleTime / frames;

        resetLimitCreature();
        m_archerEffect[archerIndex] = 1;
        for (archer->m_frame = 0; archer->m_frame < frames;
                archer->m_frame++)
            drawFrame(1, 1, 0, delay, 1, 1);
        if (archer->m_frame > 0)
            archer->m_frame--;

        shootMissile(startX, startY, destX, destY, info->m_arrowAngle,
                     archer->m_shadowSprite);
    }

    // The accumulator is a DOWN-counted loop over a copy of numTroops:
    // retail spills the count and steps it with `dec / jne`, which an
    // up-counted `for (i = 0; i < n; i++)` does not produce.
    int damage = 0;
    for (int shot = tower->m_numTroops; shot > 0; shot--)
        damage += random(2, 4);
    damage = static_cast<int>(
        damage * target->computeDefenderDamageReduction(1));
    if (damage <= 0)
        damage = 1;

    int killed = target->damage(damage);
    damageMessage(
        g_generalText->getText(GENERAL_TEXT_COMBAT_ARROW_TOWER_ATTACKER),
        1, damage, target, killed);
    target->cancelSpellType(ARMY_CANCEL_SPELLS_AFTER_DAMAGE);
    powEffect(-1, 1);

    if (!isQuickCombat()) {
        while (archer->m_frame
                < archer->m_sprite->getNumFrames(armyDir) - 1) {
            archer->m_frame++;
            drawFrame(1, 1, 0, delay, 1, 1);
        }
    }
    archer->m_facing = 0;
    archer->m_sequence = cs_wait;
    archer->m_frame = 0;
    drawFrame(1, 0, 0, 0, 1, 0);

    if (!isQuickCombat())
        waitEndSample(sample, -1);
}

VA(0x00465f20, 0xB2)
void CombatManager::unnamed465f20()
{
    int numArchers;
    int archerLevel;
    m_defendingTown->calcNumLevelArchers(&numArchers, &archerLevel);
    if (m_armies[m_actingSide][m_actingSlot].m_gridIndex != COMBAT_HEX_KEEP)
        numArchers = (numArchers + 1) / 2;

    int target = chooseBallistaTarget(0, archerLevel, numArchers * 6 / 2);
    if (target < 0) {
        m_nextAction = AI_ORDER_NONE;
        return;
    }
    m_nextAction = AI_ORDER_SHOOT;
    m_nextActionGridIndex = m_armies[0][target].m_gridIndex;
}

#if 0  // @carcass

// E:\gamedcs\cmbtmgr.cpp:2727
DC_ONLY(0x6021c, 0x4)
float CombatManager::ComputeDamageModifier(int attack, int defense)
{
    // @stub
}

#endif  // @carcass

// Dreamcast cmbtmgr.cpp:2738..2752. Complete expands this ordinary helper
// into CalculateGainedExperience. The helper owns the stack loop, both
// army::Is calls (line 2745), and the defeated-hero bonus (2749/2750).
DC_ONLY(0x60220, 0xF8)
int CombatManager::experienceValueOfStack(int whichGroup)
{
    int total = 0;
    for (int slot = 0; slot < 20; ++slot) {
        const Army& a = m_armies[whichGroup][slot];
        if (a.m_creatureType != -1 && !a.is(1u << 22) && !a.is(1u << 6))
            total += (a.m_origNumTroops - a.m_numTroops)
                * g_creatureTypeTraits[a.m_creatureType].m_hitPoints;
    }
    if (m_heroes[whichGroup])
        total += 500;
    return total;
}

VA(0x00465fe0, 0x29)  // dc 0x60318
void CombatManager::resetHitByCreature()
{
    for (int side = 0; side < 2; side++) {
        for (int slot = 0; slot < 20; slot++)
            m_armies[side][slot].m_hitByCreature = 0;
    }
}

VA(0x00466010, 0x243)  // dc 0x60354
unsigned char CombatManager::placeObstacle(int obstacleId)
{
    const ObstacleInfo* shape = &s_obstacleInfo[obstacleId];
    PickANumber picker(0x12, 0xa8);
    int hex;
    while (1) {
        hex = picker.pick();
        if (hex < 0x12)
            return 0;
        {
            int row = gridY(hex);
            if (shape->m_minRow > row)
                continue;
            int column = gridX(hex);
            if (column == 0)
                continue;
            if (shape->m_width + column > 15)
                continue;
            if (m_cells[hex].m_attributes & 0x3f)
                continue;
            const unsigned char baseRowIsOdd = rowIsOdd(row);
            unsigned char overlap = 0;
            for (int i = 0; i < shape->m_extraHexCount; i++) {
                int cellIndex = shape->m_extraHexOffsets[i] + hex;
                if (baseRowIsOdd && !rowIsOdd(gridY(cellIndex)))
                    cellIndex--;
                int cellColumn = gridX(cellIndex);
                if (cellColumn <= 2 || cellColumn >= 14
                        || (m_cells[cellIndex].m_attributes & 0x3f)) {
                    overlap = 1;
                    break;
                }
            }
            if (overlap)
                continue;

            if (g_game->m_f1f698 < 2 && m_fortificationLevel >= 2
                    && m_defendingTown->m_type == TOWN_STRONGHOLD) {
                int wallColumn = g_castleWallColumns[row];
                if (wallColumn == COMBAT_HEX_GATE)
                    wallColumn = 0x5d;
                if (shape->m_width + hex >= wallColumn - 2)
                    continue;
            }

            Obstacle obstacle;
            obstacle.m_sprite = ResourceManager::getSprite(shape->m_spriteName);
            obstacle.m_shape = shape;
            obstacle.m_hex = static_cast<unsigned char>(hex);
            obstacle.m_owner = -1;
            obstacle.m_isVisible = 1;
            obstacle.m_spellDamage = 0;
            obstacle.m_duration = 0;
            obstacle.m_dispelEffect = -1;
            m_obstacles.push_back(obstacle);
            // Keep placeObstacle out of line at this call site.
            int obstacleSlot = m_obstacles.size() - 1;
#pragma inline_depth(0)
            placeObstacle(&obstacle, obstacleSlot, hex, 2);
#pragma inline_depth()
            return 1;
        }
    }
}

VA_COMPGEN(0x00466260, 0x26, IMPLICIT_DTOR, PickANumber)  // dc 0x63a18

// E:\gamedcs\cmbtmgr.cpp:2859
// Everything the battlefield carries before the armies land: the wall
// hitpoint tables, the castle wall's blocked column, a Tower's mined
// moat, the two-boat naval blockade, and the random scatter of ordinary
// obstacles. Called only from Open.

// Shapes the bytes force, all of them cheap to get wrong:
//   * the naval branch has its OWN epilogue, so it is a real `return`
//     and not a jump into the shared tail every other exit uses;
//   * both obstacle-budget arms tail-merge into ONE Random call site,
//     which is what says a single local is assigned in an if/else
//     rather than two calls written out;
//   * the large-obstacle guard is the De Morgan form
//     `(fort < 2 || type != STRONGHOLD) && Random(1, 100) <= 40` -
//     retail falls THROUGH to Random on `jl` and skips on `je`, and the
//     positive `!(a && b)` spelling emits those two swapped;
//   * the placement loop is `while (placed < budget)` with `placed`
//     pre-set to 0, which VC6 folds to a `test/jle` on budget alone.
//     The inner re-draw is the ROTATED two-Pick shape PlaceLargeObstacle
//     already carries - a leading `Pick()` and a second one at the foot
//     of the reject loop, not a do/while with one site: retail emits two
//     out-of-line Pick calls and a do/while can only ever emit one
//     (89.8722 -> 91.5113 on that rewrite alone). The redundant
//     `id < 0` re-test in front of place_obstacle survives it.

VA(0x00466290, 0x607)  // anchor-callee, dc 0x60538
void CombatManager::setupAndLoadObstacles()
{
    m_obstacleAnimationFrame = 0;
    m_largeObstacleId = -1;
    if (m_isSurrounded)
        return;

    if (m_fortificationLevel > COMBAT_FORTIFICATION_NONE) {
        for (int wall = 0; wall < 18; wall++)
            m_wallStrength[wall] =
                s_wallTraits[m_defendingTown->m_type][wall].m_hitpoints;
        m_wallStrength[17] = 1;
        m_wallStrength[16] = 1;
        m_wallStrength[15] = 1;
        for (int copy = 0; copy < 18; copy++)
            m_wallStanding[copy] = m_wallStrength[copy];

        if (m_fortificationLevel == COMBAT_FORTIFICATION_CASTLE) {
            m_wallStrength[6]++;
            m_wallStrength[8]++;
            m_wallStrength[10]++;
            m_wallStrength[12]++;
        }

        for (int row = 0; row < 11; row++)
            m_cells[g_castleWallColumns[row]].m_attributes |= 2;

        // A Tower's moat is a minefield. Row 5 is the gate hex and is
        // skipped; every other row gets one obstacle whose damage is the
        // greater of the town's own moat figure and what the defending
        // hero's Land Mine would do.
        if (m_fortificationLevel >= COMBAT_FORTIFICATION_CITADEL
                && m_defendingTown->m_type == TOWN_TOWER) {
            for (int row = 0; row < 11; row++) {
                if (row == COMBAT_GATE_ROW)
                    continue;
                int hex = g_moatColumns[row];

                long damage;
                if (g_game->m_f1f698 >= 2) {
                    damage = g_moatDamage[TOWN_TOWER];
                    if (m_heroes[1]) {
                        long cast = computeSpellDamage(
                            SPELL_LAND_MINE, m_spellPower[1],
                            m_heroes[1]->getSpellLevel(SPELL_LAND_MINE,
                                                       m_magicTerrain),
                            0, 0, 0, 0);
                        if (cast > damage)
                            damage = cast;
                    }
                } else if (m_heroes[1]) {
                    damage = computeSpellDamage(
                        SPELL_LAND_MINE, m_spellPower[1],
                        m_heroes[1]->getSpellLevel(SPELL_LAND_MINE,
                                                   m_magicTerrain),
                        0, 0, 0, 0);
                } else {
                    damage = computeSpellDamage(SPELL_LAND_MINE, 3, 0,
                                                0, 0, 0, 0);
                }

                Obstacle newLandmine;
                newLandmine.m_sprite =
                    ResourceManager::getSprite(s_landMineInfo[0].m_spriteName);
                newLandmine.m_shape = &s_landMineInfo[0];
                newLandmine.m_hex = static_cast<unsigned char>(hex);
                newLandmine.m_owner = 1;
                newLandmine.m_isVisible = 0;
                newLandmine.m_spellDamage = damage;
                newLandmine.m_duration = 0;
                newLandmine.m_dispelEffect = 0x3b;
                m_obstacles.insert(m_obstacles.end(), 1, newLandmine);
                int landmineSlot = m_obstacles.size();
                landmineSlot--;
#pragma inline_depth(0)
                placeObstacle(&newLandmine, landmineSlot, hex, 8);
#pragma inline_depth()
            }
        }

        if (g_game->m_f1f698 >= 2)
            return;
        if (m_defendingTown->m_type != TOWN_STRONGHOLD)
            return;
        if (m_fortificationLevel < COMBAT_FORTIFICATION_CITADEL)
            return;
    }

    // Two boats meeting at sea: the hulls block thirty-two hexes and
    // nothing else is placed at all.
    if (m_terrainType == eTerrainWater
            && m_heroes[0] && (m_heroes[0]->m_flags & 0x40000)
            && m_heroes[1] && (m_heroes[1]->m_flags & 0x40000)) {
        for (const int* hex = g_boatBlockedHexes;
                hex < g_boatBlockedHexes + 32; hex++)
            m_cells[*hex].m_attributes |= 2;
        return;
    }

    int budget;
    if (m_fortificationLevel >= COMBAT_FORTIFICATION_CITADEL
            && m_defendingTown->m_type == TOWN_STRONGHOLD)
        budget = random(10, 16);
    else
        budget = random(5, 12);

    unsigned int terrainMask = 0;
    unsigned int specialTerrainMask = 0;
    if (m_magicTerrain != -1)
        specialTerrainMask = 1 << m_magicTerrain;
    else
        terrainMask = 1 << m_terrainType;

    if ((m_fortificationLevel < COMBAT_FORTIFICATION_CITADEL
                || m_defendingTown->m_type != TOWN_STRONGHOLD)
            && random(1, 100) <= 40)
        budget -= placeLargeObstacle(terrainMask, specialTerrainMask) / 2;

    int placed = 0;
    PickANumber obstaclePicker(0, 90);
    while (placed < budget) {
        int obstacleId = obstaclePicker.pick();
        while (obstacleId >= 0
               && !(s_obstacleInfo[obstacleId].m_terrainMask & terrainMask)
               && !(s_obstacleInfo[obstacleId].m_specialTerrainMask
                    & specialTerrainMask))
            obstacleId = obstaclePicker.pick();
        if (obstacleId < 0)
            break;
        if (placeObstacle(obstacleId))
            placed += s_obstacleInfo[obstacleId].m_extraHexCount;
    }
}

VA(0x004668a0, 0x108)  // dc 0x6091c
int CombatManager::placeLargeObstacle(unsigned terrainMask,
                                      unsigned magicTerrainMask)
{
    PickANumber picker(0, 0x21);
    int obstacleId = picker.pick();
    while (obstacleId >= 0) {
        if ((terrainMask & g_largeObstacleTerrainMasks[obstacleId * 34])
                || (magicTerrainMask
                    & g_largeObstacleMagicTerrainMasks[obstacleId * 34])) {
            int count = 0;
            int i = 0;
            const short* hex = &g_largeObstacleHexes[obstacleId * 34];
            for (; i < 25 && *hex != -1; ++i, ++hex) {
                m_cells[*hex].m_attributes |= 2;
                ++count;
            }
            m_largeObstacleId = obstacleId;
            return count;
        }
        obstacleId = picker.pick();
    }
    return 0;
}

VA(0x004669b0, 0xBF)  // dc 0x609d0
void CombatManager::placeObstacle(const CombatManager::Obstacle* obstacle, int id, int hex, unsigned attributes)
{
    const ObstacleInfo* shape = obstacle->m_shape;
    unsigned char rowIsOdd = static_cast<unsigned char>((hex / 0x11) & 1);
    for (int i = 0; i < shape->m_extraHexCount; i++) {
        int cellIndex = shape->m_extraHexOffsets[i] + hex;
        if (rowIsOdd && ((cellIndex / 0x11) & 1) == 0)
            cellIndex--;
        Hexcell& cell = m_cells[cellIndex];
        cell.m_attributes |= attributes;
        cell.m_obstacleIndex = id;
    }
    Hexcell& anchor = m_cells[hex];
    anchor.m_attributes |= 1;
    anchor.m_obstacleIndex = id;
}

VA(0x00466a70, 0xBD)  // dc 0x60a70
void CombatManager::placeAllObstacles()
{
    unsigned int terrainMask = 0;
    unsigned int specialTerrainMask = 0;
    if (m_magicTerrain != -1)
        specialTerrainMask = 1 << m_magicTerrain;
    else
        terrainMask = 1 << m_terrainType;

    PickANumber picker(0, 90);
    for (;;) {
        int obstacleId;
        do {
            obstacleId = picker.pick();
            if (obstacleId < 0)
                break;
        } while (!(s_obstacleInfo[obstacleId].m_terrainMask & terrainMask)
                 && !(s_obstacleInfo[obstacleId].m_specialTerrainMask
                      & specialTerrainMask));
        if (obstacleId < 0)
            break;
        placeObstacle(obstacleId);
    }
}

VA(0x00466b30, 0x11F)  // dc 0x60b20
void CombatManager::removeObstacle(int index)
{
    if (index < 0
            || static_cast<unsigned>(index)
               >= static_cast<unsigned>(m_obstacles.size()))
        return;
    Obstacle* obstacle = &getObstacle(index);
    const ObstacleInfo* shape = obstacle->m_shape;
    unsigned char rowIsOdd =
        static_cast<unsigned char>((obstacle->m_hex / 0x11) & 1);
    for (int i = 0; i < shape->m_extraHexCount; i++) {
        int cellIndex = shape->m_extraHexOffsets[i] + obstacle->m_hex;
        if (rowIsOdd && ((cellIndex / 0x11) & 1) == 0)
            cellIndex--;
        Hexcell& cell = m_cells[cellIndex];
        cell.m_attributes &= ~0x3f;
        cell.m_obstacleIndex = -1;
    }
    Hexcell& anchor = m_cells[obstacle->m_hex];
    anchor.m_attributes &= ~1;
    anchor.m_obstacleIndex = -1;
    obstacle->m_sprite->dispose();
    obstacle->m_sprite = 0;
}

VA(0x00466c50, 0x1A1)  // dc 0x60c0c
void CombatManager::initializeArchers()
{
    Archer* archer = m_archers;
    memset(archer, 0, sizeof(m_archers));
    if (m_fortificationLevel < COMBAT_FORTIFICATION_CITADEL)
        return;

    const SiegeArcherInfo& info = g_siegeArcherInfo[m_defendingTown->m_type];
    ArcherLoadState locals;
    locals.m_spriteName =
        g_creatureTypeTraits[info.m_creatureType].m_spriteName;

    archer->m_creatureType = info.m_creatureType;
    locals.m_sprite = ResourceManager::getSprite(locals.m_spriteName);
    if (archer->m_sprite)
        archer->m_sprite->dispose();
    archer->m_sprite = locals.m_sprite;
    locals.m_sprite = ResourceManager::getSprite(info.m_shadowSpriteName);
    if (archer->m_shadowSprite)
        archer->m_shadowSprite->dispose();
    archer->m_shadowSprite = locals.m_sprite;
    archer->m_x = info.m_positions[0].m_x;
    archer->m_y = info.m_positions[0].m_y;
    archer->m_facing = 0;
    archer->m_sequence = 2;
    archer->m_frame = 0;

    if (m_fortificationLevel != COMBAT_FORTIFICATION_CASTLE)
        return;

    m_archers[1].m_creatureType = info.m_creatureType;
    locals.m_sprite = ResourceManager::getSprite(locals.m_spriteName);
    if (m_archers[1].m_sprite)
        m_archers[1].m_sprite->dispose();
    m_archers[1].m_sprite = locals.m_sprite;
    locals.m_sprite = ResourceManager::getSprite(info.m_shadowSpriteName);
    if (m_archers[1].m_shadowSprite)
        m_archers[1].m_shadowSprite->dispose();
    m_archers[1].m_shadowSprite = locals.m_sprite;
    m_archers[1].m_x = info.m_positions[1].m_x;
    m_archers[1].m_y = info.m_positions[1].m_y;
    m_archers[1].m_facing = 0;
    m_archers[1].m_sequence = 2;
    m_archers[1].m_frame = 0;

    m_archers[2].m_creatureType = info.m_creatureType;
    locals.m_sprite = ResourceManager::getSprite(locals.m_spriteName);
    if (m_archers[2].m_sprite)
        m_archers[2].m_sprite->dispose();
    m_archers[2].m_sprite = locals.m_sprite;
    locals.m_sprite = ResourceManager::getSprite(info.m_shadowSpriteName);
    if (m_archers[2].m_shadowSprite)
        m_archers[2].m_shadowSprite->dispose();
    m_archers[2].m_shadowSprite = locals.m_sprite;
    m_archers[2].m_x = info.m_positions[2].m_x;
    m_archers[2].m_y = info.m_positions[2].m_y;
    m_archers[2].m_facing = 0;
    m_archers[2].m_sequence = 2;
    m_archers[2].m_frame = 0;
}

// E:\gamedcs\cmbtmgr.cpp:3299
// RECONSTRUCTED 2026-08-20. Two walks over the same +0x13438 latch: the
// first raises each marked stack's drawing-effect byte (or, for an arrow
// tower, the one of three keep/tower latches its grid index selects),
// the second clears the stack's hexcell occupancy. Only the drawing half
// is quick-combat gated - the occupancy clear runs either way, which is
// what makes the two IsQuickCombat expansions straddle it.

// Dreamcast explicitly calls MarkCreatureEffect at cmbtmgr.cpp:3312
// (dc 0x60d3a). The inline helper's Complete view contains the retail-only
// arrow-tower switch and expands to the same x86 previously written out
// here: restoring the helper boundary was byte-neutral at 97.37131% and
// the whole marked-stack region remains instruction-exact.

// The 20-per-side byte arrays against armies[2][21] are retail's own
// asymmetry, not a mis-slice: one loop steps the byte arrays by 20 and
// the army index by 21, and ResetLimitCreature memsets exactly 2x20.

// Generated scheduling search (2026-08-27): moving the width capture before
// `y` raises 96.8312 -> 97.3713 while preserving all 27 branches. The 21
// depth-1 variants found that winner; all 218 depth-2 interactions around
// the retained body were flat or worse. Post-helper structure is 38/39
// blocks exact; the extent-capture block alone is 14 versus 16 instructions.
// Retail still captures y and x before computing either extent, reloads y
// for the height subtraction, and consequently assigns the four SaveFizzle
// arguments to a different caller-saved ordering. Enabling drawing.h's
// SLimitData view to spell Width/Height is not a local lever: it changes the
// whole consumer view and makes existing `.values` consumers ill-formed.
// A partial view conversion is therefore rejected rather than retained.
VA(0x00466e00, 0x323)  // anchor-global, dc 0x60ce0
void CombatManager::makeCreaturesVanish()
{
    int x;
    int y;
    int width;
    int height;
    int side;
    int index;
    if (!isQuickCombat()) {
        resetLimitCreature();
        for (side = 0; side < 2; side++) {
            for (index = 0; index < m_numArmies[side]; index++) {
                if (!m_creatureIsDead[side][index])
                    continue;
                markCreatureEffect(side, index);
            }
        }
        computeMaxExtent();
        x = m_drawbridgeBounds.m_minX;
        width = m_drawbridgeBounds.m_maxX - m_drawbridgeBounds.m_minX + 1;
        y = m_drawbridgeBounds.m_minY;
        height = m_drawbridgeBounds.m_maxY - m_drawbridgeBounds.m_minY + 1;
    }

    for (side = 0; side < 2; side++) {
        for (index = 0; index < m_numArmies[side]; index++) {
            if (!m_creatureIsDead[side][index])
                continue;
            const Army& stack = m_armies[side][index];
            m_cells[stack.m_gridIndex].m_armySide = -1;
            m_cells[stack.m_gridIndex].m_armySlot = -1;
            if (stack.is(1u << 0)) {
                m_cells[stack.m_gridIndex + (stack.m_facing ? 1 : -1)].m_armySide = -1;
                m_cells[stack.m_gridIndex + (stack.m_facing ? 1 : -1)].m_armySlot = -1;
            }
        }
    }

    if (!isQuickCombat()) {
        g_windowManager->saveFizzleSourceX(x, y, width, height);
        drawFrame(0, 0, 1, 0, 1, 0);
        g_windowManager->fizzleForwardX(
            x, y, width, height,
            static_cast<int>(
                g_combatSpeedFactors[g_unnamed698758.m_combatSpeed] * 150.0f));
    }
}

VA(0x00467130, 0x82)  // dc 0x60ee0
unsigned char CombatManager::shouldLowerDoor(Army* thisArmy, long hex) const
{
    int side = thisArmy->m_spellInfluence[60] ? 1 - thisArmy->m_combatSide
                                        : thisArmy->m_combatSide;
    if (side != 1 || m_fortificationLevel == 0 || m_drawbridgeState != DRAWBRIDGE_UP)
        return 0;
    if (hex == COMBAT_HEX_GATE || hex == COMBAT_HEX_GATE_MOAT
            || hex == COMBAT_HEX_OUTER_MOAT)
        return 1;
    if (!(thisArmy->m_monInfo.m_attributes & 1))
        return 0;
    long second = hex + (thisArmy->m_facing != 0 ? 1 : -1);
    if (second == COMBAT_HEX_GATE || second == COMBAT_HEX_GATE_MOAT
            || second == COMBAT_HEX_OUTER_MOAT)
        return 1;
    return 0;
}

VA(0x004671c0, 0x113)  // dc 0x60fa8
void CombatManager::lowerDoor()
{
    if (isQuickCombat()) {
        m_drawbridgeState = DRAWBRIDGE_DOWN;
        return;
    }

    SAMPLE2 sample = loadPlaySample(
        DATA_COMPGEN(0x0066ffb0, drawbridgeSampleName, "drawbrg.82m"));
    m_drawbridgeBounds = g_drawbridgeBounds694f30;
    for (int state = DRAWBRIDGE_UP; state >= DRAWBRIDGE_DOWN; state--) {
        m_drawbridgeState = state;
        drawFrame(1, 0, 1, 100, 1, 1);
    }
    g_searchArray->lowerDoor();
    waitEndSample(sample, -1);
}

VA(0x004672e0, 0x177)  // dc 0x61034
void CombatManager::raiseDoor()
{
    if (!m_defendingTown || m_drawbridgeState != DRAWBRIDGE_DOWN)
        return;
    if (m_cells[COMBAT_HEX_GATE].m_armySide >= 0
            || m_cells[COMBAT_HEX_GATE].m_bodiesInHex)
        return;
    if (m_cells[COMBAT_HEX_GATE_MOAT].m_armySide >= 0
            || m_cells[COMBAT_HEX_GATE_MOAT].m_bodiesInHex)
        return;
    if (m_defendingTown->m_type == TOWN_FORTRESS
            && (m_cells[COMBAT_HEX_OUTER_MOAT].m_armySide >= 0
                || m_cells[COMBAT_HEX_OUTER_MOAT].m_bodiesInHex))
        return;

    if (isQuickCombat()) {
        m_drawbridgeState = DRAWBRIDGE_UP;
        return;
    }

    SAMPLE2 sample = loadPlaySample("drawbrg.82m");
    m_drawbridgeBounds = g_drawbridgeBounds694f30;
    for (int state = DRAWBRIDGE_DOWN; state <= DRAWBRIDGE_UP; state++) {
        m_drawbridgeState = state;
        drawFrame(1, 0, 1, 100, 1, 1);
    }
    waitEndSample(sample, -1);
}

// E:\gamedcs\cmbtmgr.cpp:3426, dc 0x610e0.
// Complete moved the occupancy guards into RaiseDoor itself. WalkTo, FlyTo,
// TeleportTo and ProcessNextAction retain this forwarding source boundary.
void CombatManager::testRaiseDoor()
{
    raiseDoor();
}

VA(0x00467460, 0x22)  // dc 0x61160
unsigned char inCastle(int index)
{
    return index >= g_castleWallColumns[index / 0x11];
}

VA(0x00467490, 0x22)  // dc 0x61180
unsigned char leftOfMoat(int index)
{
    return index < g_moatColumns[index / 0x11];
}

VA(0x004674c0, 0x4C)  // dc 0x611a0
unsigned char CombatManager::isAdjacent(int first, int second) const
{
    if (first >= 0 && first < 187 && second >= 0 && second < 187) {
        for (int i = 0; i < 6; i++) {
            if (m_adjacentCells[first][i] == second)
                return 1;
        }
    }
    return 0;
}

VA(0x00467510, 0xEA)  // dc 0x61224
unsigned char CombatManager::shotIsThroughWall(const Army* shooter, int sourceIndex,
                                               int destIndex) const
{
    int side = shooter->m_spellInfluence[60] ? 1 - shooter->m_combatSide
                                      : shooter->m_combatSide;
    if (shooter->m_creatureType == CREATURE_MAGE
            || shooter->m_creatureType == CREATURE_ARCH_MAGE
            || shooter->m_creatureType == CREATURE_ENCHANTER
            || shooter->m_creatureType == CREATURE_SHARPSHOOTER
            || shooter->m_creatureType == CREATURE_ARROW_TOWER)
        return 0;
    if (inCastle(sourceIndex) || !inCastle(destIndex))
        return 0;
    if (m_heroes[side] != 0
            && (m_heroes[side]->isWieldingArtifact(ARTIFACT_GOLDEN_BOW)
                || m_heroes[side]->isWieldingArtifact(ARTIFACT_BOW_OF_THE_SHARPSHOOTER)))
        return 0;
    return inLineOfSight(sourceIndex, destIndex) == 0;
}

VA(0x00467600, 0x23A)  // dc 0x61284
unsigned char CombatManager::shotIsNotOptimal(const Army* attacker, const Army* defender) const
{
    int side = attacker->m_spellInfluence[60] ? 1 - attacker->m_combatSide
                                       : attacker->m_combatSide;
    if (m_heroes[side]
            && (m_heroes[side]->isWieldingArtifact(ARTIFACT_GOLDEN_BOW)
                || m_heroes[side]->isWieldingArtifact(
                    ARTIFACT_BOW_OF_THE_SHARPSHOOTER)))
        return 0;
    if (attacker->m_creatureType == CREATURE_ARROW_TOWER
            || attacker->m_creatureType == CREATURE_SHARPSHOOTER)
        return 0;

    int source = attacker->m_gridIndex;
    int dest = defender->m_gridIndex;
    if (attacker->m_monInfo.m_attributes & 1)
        source = attacker->getSecondGridIndex();
    if (getDistance(source, dest) <= 10)
        return 0;
    if (!(defender->m_monInfo.m_attributes & 1))
        return 1;
    dest = defender->getSecondGridIndex();
    return getDistance(source, dest) > 10;
}

VA(0x00467840, 0x1B6)  // dc 0x61318
unsigned char CombatManager::inLineOfSight(int sourceIndex, int destIndex) const
{
    if (!m_fortificationLevel)
        return 1;

    int sourceX = sourceIndex % COMBAT_GRID_ROW_STRIDE;
    int sourceY = sourceIndex / COMBAT_GRID_ROW_STRIDE;
    int deltaX = destIndex % COMBAT_GRID_ROW_STRIDE - sourceX;
    int deltaY = destIndex / COMBAT_GRID_ROW_STRIDE - sourceY;
    if (deltaY == 0 && deltaX == 0)
        return 1;
    int sample;
    int absY = abs(deltaY);
    int absX = abs(deltaX);
    float stepX;
    float stepY;
    int distance;
    if (absX > absY) {
        stepX = deltaX > 0 ? 1.0 : -1.0;
        stepY = static_cast<float>(deltaY) / static_cast<float>(absX);
        distance = absX;
    } else {
        distance = absY;
        stepY = deltaY > 0 ? 1.0 : -1.0;
        stepX = static_cast<float>(deltaX) / static_cast<float>(absY);
    }

    stepX /= 17.0f;
    stepY /= 17.0f;
    float x = static_cast<float>(sourceX);
    float y = static_cast<float>(sourceY);
    int samples = distance * COMBAT_GRID_ROW_STRIDE;
    for (sample = 0; sample < samples; sample++) {
        x += stepX;
        y += stepY;
        int hex = static_cast<int>(y) * COMBAT_GRID_ROW_STRIDE
            + static_cast<int>(x);
        if (hex == COMBAT_HEX_GATE) {
            if (m_drawbridgeState == DRAWBRIDGE_UP)
                return 0;
        } else {
            for (int wall = 0; wall < 11; wall++) {
                if (hex == g_castleWallColumns[wall]) {
                    if (m_cells[g_castleWallColumns[wall]].m_attributes & 2)
                        return 0;
                }
            }
        }
    }
    return 1;
}

// THE THREE MISSILE ANIMATORS, SURVEYED 2026-08-20 (not reconstructed).
// They are one body written three times, so the survey is recorded once,
// here, at the first of them.

// VERIFIED DIRECTLY: none of the three touches a single pad byte of this
// class. Every `this`-relative displacement in all three bodies is one of
// +0x54a4, +0x54a5, +0x54a8, +0x54ac - which is just the inlined
// IsQuickCombat guard - plus gpGame+0x1f69d inside it. So the header work
// for this family is declarations and .rdata externs ONLY, with no layout
// archaeology, which makes them far cheaper than InitNonVisualVars
// despite being bigger.

// The shared nine-phase skeleton, in order: the inlined IsQuickCombat
// guard (byte-identical in all three) / deltas / a `steps` divide /
// per-step motion / sprite-or-frame selection / `Bitmap16Bit backup(w,h)`
// plus four limits seeded from the 0x6aace8 quad / DrawFrame / a frame
// delay from gCombatSpeedFactors[combatSpeed] / the animation loop. The
// loop body is Grab, sprite Draw, a four-way union of the sprite rect
// into the limits, a four-way clip against the 0x694f18 quad,
// UpdateScreen with (r-l+1, b-t+1), then DelayTil.

// Where they differ: ShootBallisticMissile is the only PARABOLIC one - it
// recomputes x and y from an `arc` term every frame instead of
// accumulating a step - is the only one with no DrawFrame call, is the
// only one whose backup restore sits INSIDE the loop, and uses a 100.0f
// delay factor where the other two use 33.0f. ShootMissile is the only
// one that does NOT cycle the sprite frame: it picks one frame from the
// angle and holds it. ShootAnimatedMissile is the only one that owns its
// sprite (ResourceManager::GetSprite ... Dispose).

// The union+clip+UpdateScreen block is DC's SLimitData::Include +
// ::Clip + Width()/Height(), and the same block appears in
// army::animate_missile (0x43f2c0), combatManager::DrawFrame (0x494440)
// and ComputeMaxExtent (0x495bf0) - so spelling it right here pays off in
// several more bodies. Suggested order: ShootMissile first (fullest angle
// path, simplest loop), then ShootAnimatedMissile, then the parabola.

// One thing to settle before writing any of them: all four dwords of each
// limits quad are read INDIVIDUALLY, each at displacement 0 against its
// own symbol, so they want four separate externs rather than one struct -
// the gCombatHexLeft694ea8 precedent in the header. And every new
// file-scope extern on cmbtmgr.h fires the include-set wall by itself
// (measured at gCombatSeed66d840), so all of them must be gated.

// E:\gamedcs\cmbtmgr.cpp:3640
// RECONSTRUCTED 2026-08-20, the parabolic member of the trio. DC local
// names again (ARROW_TRAVEL_DIST, MISSILE_PERIOD, nframes, flatness,
// saved, deltaX, deltaY, next_frame_time, update_area). DC has NO
// `flipped` row for this one and retail pushes a literal 0 for hflip,
// which agrees: a ballistic missile is never mirrored, and it needs no
// angle search or DrawFrame at all - the trajectory is arithmetic.

// DC's variable list turned out to be a MATCHING LEVER here, not just a
// naming source. ARROW_TRAVEL_DIST is listed for this body and for
// neither of the other two, and that asymmetry is real: naming the
// pre-division distance is worth 79.99 -> 81.72 here, while the same
// edit costs ShootMissile 91.74 -> 90.43. Where DC names a local, name
// it; where DC does not, fold it.

// The trajectory, for the next reader. `travelX` accumulates deltaX and
// `remaining` counts nframes down to zero, so
//   x = startX + travelX / nframes
//   y = startY + step * (deltaY - remaining * flatness) / nframes
// with flatness = 2*abs(deltaX)/nframes. Both endpoints are exact
// (step 0 gives the start, step nframes gives the destination, since
// remaining is 0 there) and the deviation peaks at abs(deltaX)/2
// halfway - the arc is half the horizontal span, which is what makes
// `flatness` the right name for the coefficient rather than a height.

VA(0x00467a00, 0x3AF)  // anchor-global, dc 0x614f0
void CombatManager::shootBallisticMissile(int startX, int startY, int destX,
                                          int destY, const CSprite* missile)
{
    if (isQuickCombat())
        return;

    const int deltaX = destX - startX;
    const int deltaY = destY - startY;
    const int arrowtraveldist = static_cast<int>(sqrt(static_cast<double>(
        deltaY * deltaY + deltaX * deltaX)));
    const int nframes = (arrowtraveldist + 10) / 20;
    // The arc: half the horizontal span, spread over the flight. The
    // trajectory below subtracts flatness*(nframes - step) from deltaY,
    // so the peak deviation is nframes/4 * flatness = abs(deltaX)/2.
    const double flatness =
        2.0 * abs(deltaX) / static_cast<double>(nframes);

    int width = missile->getWidth();
    int height = missile->getHeight();
    startX -= width / 2;
    startY -= height / 2;
    int x = startX;
    int y = startY;

    Bitmap16Bit saved(width, height);
    TDrawbridgeBounds updateArea = g_combatAreaLimits;
    const int missileperiod = static_cast<int>(
        g_combatSpeedFactors[g_unnamed698758.m_combatSpeed] * 100.0f);

    int frame = 0;
    int step = 0;
    if (nframes > 0) {
        int travelX = 0;
        int remaining = nframes;
        for (; step < nframes; step++) {
            unsigned long nextFrameTime = GameTime::get() + missileperiod;
            if (step != 0) {
                updateArea.m_minX = x;
                updateArea.m_minY = y;
                updateArea.m_maxX = x + width - 1;
                updateArea.m_maxY = y + height - 1;
                x = startX + travelX / nframes;
                y = static_cast<int>(
                    (deltaY - remaining * flatness) * step
                    / static_cast<double>(nframes) + startY);
            }
            saved.grab(g_windowManager->m_screenBitmap->getMap(0, 0), x, y,
                       g_windowManager->m_screenBitmap->getWidth(),
                       g_windowManager->m_screenBitmap->getHeight(),
                       g_windowManager->m_screenBitmap->getPitch());
            const_cast<CSprite*>(missile)->draw(
                0, frame, 0, 0, width, height,
                g_windowManager->m_screenBitmap->getMap(0, 0), x, y,
                g_windowManager->m_screenBitmap->getWidth(),
                g_windowManager->m_screenBitmap->getHeight(),
                g_windowManager->m_screenBitmap->getPitch(), 0, 1);
            int right = x + width - 1;
            int bottom = y + height - 1;
            if (updateArea.m_minX > x)
                updateArea.m_minX = x;
            if (updateArea.m_minY > y)
                updateArea.m_minY = y;
            if (updateArea.m_maxX < right)
                updateArea.m_maxX = right;
            if (updateArea.m_maxY < bottom)
                updateArea.m_maxY = bottom;
            if (updateArea.m_minX < g_combatDrawLimits694f18.m_minX)
                updateArea.m_minX = g_combatDrawLimits694f18.m_minX;
            if (updateArea.m_minY < g_combatDrawLimits694f18.m_minY)
                updateArea.m_minY = g_combatDrawLimits694f18.m_minY;
            if (updateArea.m_maxX > g_combatDrawLimits694f18.m_maxX)
                updateArea.m_maxX = g_combatDrawLimits694f18.m_maxX;
            if (updateArea.m_maxY > g_combatDrawLimits694f18.m_maxY)
                updateArea.m_maxY = g_combatDrawLimits694f18.m_maxY;
            g_windowManager->updateScreen(
                updateArea.m_minX, updateArea.m_minY,
                updateArea.m_maxX - updateArea.m_minX + 1,
                updateArea.m_maxY - updateArea.m_minY + 1);
            saved.draw(0, 0, width, height,
                       g_windowManager->m_screenBitmap->getMap(0, 0), x, y,
                       g_windowManager->m_screenBitmap->getWidth(),
                       g_windowManager->m_screenBitmap->getHeight(),
                       g_windowManager->m_screenBitmap->getPitch(), false);
            ++frame;
            if (frame >= missile->getNumFrames(0))
                frame = 0;
            GameTime::delayTil(nextFrameTime);
            travelX += deltaX;
            --remaining;
        }
    }
}

VA(0x00467db0, 0x46A)  // dc 0x619a8
void CombatManager::shootAnimatedMissile(int startX, int startY, int destX,
                                         int destY, int nsprites,
                                         const float* angles,
                                         const char* const* fileNames)
{
    if (isQuickCombat())
        return;

    const int deltaX = destX - startX;
    int deltaY = destY - startY;
    // DC records flipped as a lowered byte; retail forwards it directly to
    // CSprite's public _N parameter. An unsigned char adds test/setne.
    bool flipped = deltaX < 0;
    const int distance = static_cast<int>(sqrt(static_cast<double>(
        deltaY * deltaY + deltaX * deltaX)));
    const int nframes = (distance + 15) / 31;
    int addX;
    int addY;
    if (nframes > 0) {
        addX = deltaX / nframes;
        addY = deltaY / nframes;
    } else {
        addX = deltaX;
        addY = deltaY;
    }

    int spriteIndex;
    if (deltaX == 0) {
        if (deltaY > 0)
            spriteIndex = nsprites - 1;
        else
            spriteIndex = 0;
    } else {
        float angle;
        double degrees;
        if (flipped)
            degrees = atan(static_cast<double>(deltaY) / deltaX)
                      * 57.2957763671875;
        else
            degrees = atan(static_cast<double>(deltaY) / -deltaX)
                      * 57.2957763671875;
        angle = static_cast<float>(degrees);
        int index = 1;
        while (index < nsprites
                && (angles[index - 1] + angles[index]) / 2.0f >= angle)
            ++index;
        if (index < nsprites)
            spriteIndex = index - 1;
        else
            spriteIndex = nsprites - 1;
    }

    CSprite* const missile = ResourceManager::getSprite(fileNames[spriteIndex]);
    int width = missile->getWidth();
    int height = missile->getHeight();
    int x = startX - width / 2;
    int y = startY - height / 2;

    Bitmap16Bit saved(width, height);
    TDrawbridgeBounds updateArea = g_combatAreaLimits;
    drawFrame(0, 0, 0, 0, 1, 0);
    const int arrowDelay = static_cast<int>(
        g_combatSpeedFactors[g_unnamed698758.m_combatSpeed] * 33.0f);

    int frame = 0;
    int step = 0;
    if (step < nframes) {
        int bottom = y + height - 1;
        int right = x + width - 1;
        for (; step < nframes; step++) {
            unsigned long nextFrameTime = GameTime::get() + arrowDelay;
            if (step != 0) {
                saved.draw(0, 0, width, height,
                           g_windowManager->m_screenBitmap, x, y, false);
                updateArea.m_minX = x;
                updateArea.m_minY = y;
                updateArea.m_maxX = right;
                updateArea.m_maxY = bottom;
                x += addX;
                right += addX;
                y += addY;
                bottom += addY;
            }
            saved.grab(g_windowManager->m_screenBitmap, x, y);
            missile->draw(0, frame, 0, 0, width, height,
                          g_windowManager->m_screenBitmap, x, y, flipped, 1);
            if (updateArea.m_minX > x)
                updateArea.m_minX = x;
            if (updateArea.m_minY > y)
                updateArea.m_minY = y;
            if (updateArea.m_maxX < right)
                updateArea.m_maxX = right;
            if (updateArea.m_maxY < bottom)
                updateArea.m_maxY = bottom;
            if (updateArea.m_minX < g_combatDrawLimits694f18.m_minX)
                updateArea.m_minX = g_combatDrawLimits694f18.m_minX;
            if (updateArea.m_minY < g_combatDrawLimits694f18.m_minY)
                updateArea.m_minY = g_combatDrawLimits694f18.m_minY;
            if (updateArea.m_maxX > g_combatDrawLimits694f18.m_maxX)
                updateArea.m_maxX = g_combatDrawLimits694f18.m_maxX;
            if (updateArea.m_maxY > g_combatDrawLimits694f18.m_maxY)
                updateArea.m_maxY = g_combatDrawLimits694f18.m_maxY;
            g_windowManager->updateScreen(
                updateArea.m_minX, updateArea.m_minY,
                updateArea.m_maxX - updateArea.m_minX + 1,
                updateArea.m_maxY - updateArea.m_minY + 1);
            ++frame;
            if (frame >= missile->getNumFrames(0))
                frame = 0;
            GameTime::delayTil(nextFrameTime);
        }
    }

    saved.draw(0, 0, width, height,
               g_windowManager->m_screenBitmap, x, y, false);
    g_windowManager->updateScreen(x, y, width, height);
    missile->dispose();
}

// E:\gamedcs\cmbtmgr.cpp:3902
// RECONSTRUCTED 2026-08-20. Local names are DC's own
// (evidence/dreamcast/variables.csv): deltaX/deltaY, addX/addY,
// nframes, flipped, saved, frame, next_frame_time, ARROW_DELAY, and
// update_area for the four running limits - which is why update_area is
// spelled as ONE four-int aggregate rather than four scalars: the
// aggregate is worth half a point here (91.22 -> 91.74) because it
// colours the four limits into one slot run the way retail does.

// Two shapes were forced by retail rather than chosen. `angle` is
// assigned from a NAMED DOUBLE, not from a cast expression: retail
// rounds the atan product to double and reloads it before narrowing to
// the float (`fstp qword / fld qword / fstp dword`), and neither
// `angle = expr` nor `angle = (float)expr` emits that middle pair -
// only a real double lvalue does (90.68 -> 91.18). And the multiplier
// is 0x63b8f0, whose stored double is 57.2957763671875 - the FLOAT
// rounding of 180/PI, not the 57.29577951308232 that
// GetMissileStartingPosition multiplies by further down this same TU.
// Two constants, two spellings, both retail's.

VA(0x00468220, 0x48F)  // anchor-global, dc 0x61e60
void CombatManager::shootMissile(int startX, int startY, int destX, int destY,
                                 const float* angles, const CSprite* missile)
{
    if (isQuickCombat())
        return;

    const int deltaX = destX - startX;
    int deltaY = destY - startY;
    // DC records flipped as a lowered byte; retail forwards it directly to
    // CSprite's public _N parameter. An unsigned char adds test/setne.
    bool flipped = deltaX < 0;
    const int nframes = (static_cast<int>(sqrt(static_cast<double>(
                       deltaY * deltaY + deltaX * deltaX))) + 20) / 40;
    int addX;
    int addY;
    if (nframes > 0) {
        addX = deltaX / nframes;
        addY = deltaY / nframes;
    } else {
        addX = deltaX;
        addY = deltaY;
    }

    int width = missile->getWidth();
    int height = missile->getHeight();
    int x = startX - width / 2;
    int y = startY - height / 2;

    int frame;
    if (deltaX == 0) {
        if (deltaY > 0)
            frame = missile->getNumFrames(0) - 1;
        else
            frame = 0;
    } else {
        float angle;
        // The double result is stored, reloaded and only then narrowed
        // to the float: that round-trip is /Op's, and an explicit
        // (float) cast on the expression suppresses it.
        double degrees;
        if (flipped)
            degrees = atan(static_cast<double>(deltaY) / deltaX)
                    * DATA_COMPGEN(0x0063b8f0, missileRadiansToDegrees,
                                   57.2957763671875);
        else
            degrees = atan(static_cast<double>(deltaY) / -deltaX)
                    * 57.2957763671875;
        angle = static_cast<float>(degrees);
        int index = 1;
        while (index < missile->getNumFrames(0)
                && (angles[index - 1] + angles[index]) / 2.0f >= angle)
            ++index;
        if (index < missile->getNumFrames(0))
            frame = index - 1;
        else
            frame = missile->getNumFrames(0) - 1;
    }

    Bitmap16Bit saved(width, height);
    TDrawbridgeBounds updateArea = g_combatAreaLimits;
    drawFrame(0, 0, 0, 0, 1, 0);
    const int arrowdelay = static_cast<int>(
        g_combatSpeedFactors[g_unnamed698758.m_combatSpeed] * 33.0f);

    int bottom = y + height - 1;
    int right = x + width - 1;
    for (int step = 0; step < nframes; step++) {
        unsigned long nextFrameTime = GameTime::get() + arrowdelay;
        if (step != 0) {
            saved.draw(0, 0, width, height,
                       g_windowManager->m_screenBitmap->getMap(0, 0), x, y,
                       g_windowManager->m_screenBitmap->getWidth(),
                       g_windowManager->m_screenBitmap->getHeight(),
                       g_windowManager->m_screenBitmap->getPitch(), false);
            updateArea.m_minX = x;
            updateArea.m_minY = y;
            updateArea.m_maxX = right;
            updateArea.m_maxY = bottom;
            x += addX;
            right += addX;
            y += addY;
            bottom += addY;
        }
        saved.grab(g_windowManager->m_screenBitmap->getMap(0, 0), x, y,
                   g_windowManager->m_screenBitmap->getWidth(),
                   g_windowManager->m_screenBitmap->getHeight(),
                   g_windowManager->m_screenBitmap->getPitch());
        // DC's own mangling makes `missile` a `const CSprite*`
        // (PBVCSprite) while CSprite::Draw is non-const on both builds,
        // so the cast is retail's, not ours.
        const_cast<CSprite*>(missile)->draw(
            0, frame, 0, 0, width, height,
            g_windowManager->m_screenBitmap->getMap(0, 0), x, y,
            g_windowManager->m_screenBitmap->getWidth(),
            g_windowManager->m_screenBitmap->getHeight(),
            g_windowManager->m_screenBitmap->getPitch(), flipped, 1);
        if (updateArea.m_minX > x)
            updateArea.m_minX = x;
        if (updateArea.m_minY > y)
            updateArea.m_minY = y;
        if (updateArea.m_maxX < right)
            updateArea.m_maxX = right;
        if (updateArea.m_maxY < bottom)
            updateArea.m_maxY = bottom;
        if (updateArea.m_minX < g_combatDrawLimits694f18.m_minX)
            updateArea.m_minX = g_combatDrawLimits694f18.m_minX;
        if (updateArea.m_minY < g_combatDrawLimits694f18.m_minY)
            updateArea.m_minY = g_combatDrawLimits694f18.m_minY;
        if (updateArea.m_maxX > g_combatDrawLimits694f18.m_maxX)
            updateArea.m_maxX = g_combatDrawLimits694f18.m_maxX;
        if (updateArea.m_maxY > g_combatDrawLimits694f18.m_maxY)
            updateArea.m_maxY = g_combatDrawLimits694f18.m_maxY;
        g_windowManager->updateScreen(updateArea.m_minX, updateArea.m_minY,
                                      updateArea.m_maxX - updateArea.m_minX + 1,
                                      updateArea.m_maxY - updateArea.m_minY + 1);
        GameTime::delayTil(nextFrameTime);
    }

    saved.draw(0, 0, width, height, g_windowManager->m_screenBitmap->getMap(0, 0), x, y,
               g_windowManager->m_screenBitmap->getWidth(),
               g_windowManager->m_screenBitmap->getHeight(),
               g_windowManager->m_screenBitmap->getPitch(), false);
    g_windowManager->updateScreen(x, y, width, height);
}

VA(0x004686b0, 0x7B)  // dc 0x622bc
void CombatManager::combatSystemOptions()
{
    CombatOptionsWindow options;
    options.doModal();
    m_backgroundDrawn = 0;
    updateGrid(0, 1);
    drawFrame(1, 0, 0, 0, 1, 0);
}

VA(0x00468730, 0x8E)  // dc 0x62358
void CombatManager::removeArmyFromGrid(const Army& a)
{
    m_cells[a.m_gridIndex].m_armySlot = -1;
    m_cells[a.m_gridIndex].m_armySide = -1;
    m_cells[a.m_gridIndex].m_partOfDouble = -1;
    if (a.m_monInfo.m_attributes & 1) {
        int hex = a.m_gridIndex + (a.m_facing != 0 ? 1 : -1);
        m_cells[hex].m_armySlot = -1;
        m_cells[hex].m_armySide = -1;
        m_cells[hex].m_partOfDouble = -1;
    }
}

VA(0x004687c0, 0x99)  // dc 0x623cc
void CombatManager::placeArmyInGrid(const Army& a, int hex)
{
    m_cells[hex].m_armySide = static_cast<signed char>(a.m_combatSide);
    m_cells[hex].m_armySlot = static_cast<signed char>(a.m_bitIndex);
    m_cells[hex].m_partOfDouble = -1;
    if (a.m_monInfo.m_attributes & 1) {
        m_cells[hex].m_partOfDouble = a.m_facing == 0;
        int second = hex + (a.m_facing != 0 ? 1 : -1);
        m_cells[second].m_armySide = static_cast<signed char>(a.m_combatSide);
        m_cells[second].m_armySlot = static_cast<signed char>(a.m_bitIndex);
        m_cells[second].m_partOfDouble = a.m_facing != 0;
    }
}

VA(0x00468860, 0x124)  // dc 0x62478
void CombatManager::viewArmy(Army* thisArmy, int isQuickView)
{
    if (thisArmy) {
        int x = m_cells[thisArmy->m_gridIndex].m_refX - 149;
        int y = m_cells[thisArmy->m_gridIndex].m_refY - 140;
        if (x < 0)
            x = 0;
        else if (x > 502)
            x = 502;
        if (y < 0)
            y = 0;
        else if (y > 275)
            y = 275;

        ViewArmyWindow* view = new ViewArmyWindow(
            thisArmy, x, y, static_cast<unsigned char>(!isQuickView));
        if (isQuickView) {
            view->quickView();
        } else {
            view->doModal();
            if (g_windowManager->m_dialogReturn == ViewArmyWindow::OK_ID) {
                initiateSpell(thisArmy->m_faerieDragonSpell, 1);
                if (m_nextAction == 1)
                    m_nextAction = 10;
            }
        }
        delete view;
    }
}

// E:\gamedcs\cmbtmgr.cpp:4158

// The body is NOT a switch - `spellEffect` is only ever compared with
// -1 and used as a twelve-byte index into akSpellEffectTraits. What it
// is instead is eleven `for(side) for(slot)` walks over armies[2][21],
// split by three separately inlined IsQuickCombat guards: the first
// skips the entire animation half, the second gates the per-stack
// samples, the third gates the wind-down loop.

// Shapes worth keeping:
//   * the frame budget is four chained maximum selects ending on
//     `wince + attack - 1`, which retail forms with one
//     `lea eax,[esi+edi-1]`;
//   * `iNextFrameType = cs_wince + (Is(1u << 27))` is ARITHMETIC, not a
//     ternary - retail emits `setne cl` straight into `add ecx,3`;
//   * walk 4 calls MarkCreatureEffect after its extra POW-specific guards;
//     Dreamcast records that helper boundary at cmbtmgr.cpp:4293, and the
//     Complete inline body adds the retail-only arrow-tower switch;
//   * the wind-down is a `for(;;)` with a bFramesChanged latch, not a
//     counted loop - retail has no bound to test.

// Two measured refinements on top of the first compile (96.1134):
//   * army::bPowSequenceComplete is an INT, not the byte its name
//     suggests. Retail both tests and stores it a dword wide, and
//     retyping it is worth +0.03 (96.1134 -> 96.1439). The field note
//     in army.h carries the bytes.
//   * the attack-frame skip is a GOTO, not a nested if. Retail spells
//     `cmp frameCount, attack_frames-1 / jge <play> / jmp <continue>`,
//     i.e. it tests the POSITIVE and falls through to the continue,
//     which the plain `if (frameCount < attack_frames - 1) continue;`
//     emits with both arms the other way round. +0.055, and it takes
//     the branch-shape distance from 3 to 1. Swapping the enclosing
//     `if (attack_frames)` arms instead was measured and is much worse
//     (94.90) - the flip is on the inner test alone.

// CURRENT (96.2232%, rechecked 2026-09-01): predict-inline reports the call
// multisets AGREE exactly (14 and 14). The DC dossier records 151 source
// rows and the MarkCreatureEffect call at cmbtmgr.cpp:4293; restoring that
// inline boundary is byte-neutral. The structure view is 224 versus 225
// blocks, with the early one-block skew cascading through its alignment;
// the source view localizes the first real divergence to the first animation
// walk's stores and GetNumFrames lowering. The remainder is instruction/slot
// selection rather than a missing DC helper. Negative controls on
// 2026-08-21: DC's wince_frames-before-attack_frames declaration order
// regresses to 96.21098; explicit clear/conditional-set of
// bShowRangeFrames regresses to 95.7939 and flips one branch polarity; a
// 1:0 ternary is byte-identical to the retained boolean assignment; and a
// named bool for the special-wince bit is byte-flat, still folding retail's
// `test/setne/add` to our `and/add`. The earlier six why-branch candidates
// likewise measured +0 or worse.
// The positive frameCount if/else removes play_frame while preserving all
// 2561 compiled bytes and the 25 relocation names/addends at 96.2927%.
// Its true arm permits the common frame body; only the false arm skips it.
// The inverted continue guard still scores 96.2378%, so guard polarity and
// scope matter here even though the source operations are otherwise equal.
VA(0x00468990, 0xA08)  // anchor-global, dc 0x62560
void CombatManager::powEffect(int spellEffect, int resetLimitCreature)
{
    int side;
    int slot;

    if (!isQuickCombat()) {
        for (side = 0; side < 2; side++) {
            for (slot = 0; slot < m_numArmies[side]; slot++) {
                Army& stack = m_armies[side][slot];
                stack.m_showRangeFrames = static_cast<unsigned char>(
                    stack.m_currFrameType == cs_range_ur
                    || stack.m_currFrameType == cs_range_r
                    || stack.m_currFrameType == cs_range_dr);
                stack.m_nextFrameType = -1;
                if (stack.m_someUnitsDamaged || stack.m_showAttackFrames) {
                    if (stack.m_showAttackFrames)
                        stack.m_nextFrameType = stack.m_showAttackFrameType;
                    else if (stack.m_allUnitsKilled)
                        stack.m_nextFrameType = cs_death;
                    else
                        stack.m_nextFrameType = static_cast<signed char>(
                            cs_wince + ((stack.is(1u << 27)) != 0));
                    stack.m_remainingFramesToPlay = static_cast<signed char>(
                        stack.m_stdIcon->getNumFrames(stack.m_nextFrameType));
                    if (stack.m_nextFrameType == stack.m_currFrameType)
                        stack.m_remainingFramesToPlay--;
                    if (stack.m_drawPriority < 5)
                        stack.m_drawPriority = 5;
                }
                stack.m_powSequenceComplete = 0;
            }
        }

        unsigned char showSomePowEffect = 0;
        if (spellEffect != -1) {
            for (side = 0; side < 2; side++) {
                for (slot = 0; slot < m_numArmies[side]; slot++) {
                    if (m_armies[side][slot].m_showPowEffect) {
                        showSomePowEffect = 1;
                        break;
                    }
                }
            }
            if (showSomePowEffect && !loadSpellEffect(spellEffect))
                showSomePowEffect = 0;
        }

        int numFrames = 0;
        if (showSomePowEffect)
            numFrames = m_powSprite->getNumFrames(0);

        int attackFrames = 0;
        int winceFrames = 0;
        for (side = 0; side < 2; side++) {
            for (slot = 0; slot < m_numArmies[side]; slot++) {
                Army& stack = m_armies[side][slot];
                if (stack.m_showAttackFrames)
                    attackFrames = cppMax<long>(attackFrames,
                        stack.m_stdIcon->getNumFrames(
                            stack.m_showAttackFrameType));
                else if (stack.m_allUnitsKilled)
                    winceFrames = cppMax<long>(winceFrames,
                        stack.m_stdIcon->getNumFrames(cs_death));
                else if (stack.m_someUnitsDamaged)
                    winceFrames = cppMax<long>(winceFrames,
                        stack.m_stdIcon->getNumFrames(cs_wince));
            }
        }
        numFrames = cppMax<long>(numFrames, winceFrames);
        numFrames = cppMax<long>(numFrames, attackFrames);
        numFrames = cppMax<long>(numFrames, winceFrames + attackFrames - 1);

        if (resetLimitCreature)
            this->resetLimitCreature();

        for (side = 0; side < 2; side++) {
            for (slot = 0; slot < m_numArmies[side]; slot++) {
                Army& stack = m_armies[side][slot];
                if (stack.is(1u << 21))
                    continue;
                if (!stack.m_someUnitsDamaged && !stack.m_showAttackFrames
                        && !stack.m_showRangeFrames)
                    continue;
                markCreatureEffect(side, slot);
            }
        }

        computeMaxExtent();
        if (spellEffect != -1)
            playImmEffect(g_spellEffectTraits[spellEffect].m_immName, 1);

        for (int frameCount = 0; frameCount < numFrames; frameCount++) {
            const int winceStartOffset = numFrames - 1 - frameCount;
            for (side = 0; side < 2; side++) {
                for (slot = 0; slot < m_numArmies[side]; slot++) {
                    Army& stack = m_armies[side][slot];

                    if (stack.m_showRangeFrames
                            && stack.m_currFrameType != cs_wait) {
                        if (stack.m_currFrameIndex
                                < stack.m_stdIcon->getNumFrames(
                                    stack.m_currFrameType) - 1) {
                            stack.m_currFrameIndex++;
                        } else {
                            stack.m_currFrameType = cs_wait;
                            stack.m_currFrameIndex = 0;
                        }
                    }
                    if (stack.m_nextFrameType == -1)
                        continue;
                    if (stack.m_powSequenceComplete)
                        continue;

                    if (!stack.m_showAttackFrames
                            && winceStartOffset
                                > stack.m_remainingFramesToPlay) {
                        if (attackFrames) {
                            if (frameCount >= attackFrames - 1) {
                                // The attack threshold permits this frame.
                            } else {
                                continue;
                            }
                        } else if (stack.m_currFrameType == cs_wince
                                && stack.m_currFrameIndex
                                    >= stack.m_stdIcon->getNumFrames(
                                        stack.m_currFrameType) - 1) {
                            continue;
                        }
                    }

                    if (stack.m_currFrameType != stack.m_nextFrameType) {
                        if (!isQuickCombat()) {
                            if (stack.m_showAttackFrames)
                                stack.playSample(Army::ATTACK_SAMPLE);
                            else if (stack.m_nextFrameType == cs_wince)
                                stack.playSample(Army::WINCE_SAMPLE);
                            else if (stack.m_nextFrameType == cs_death)
                                stack.playSample(Army::DIE_SAMPLE);
                            else if (stack.m_nextFrameType == cs_defend)
                                stack.playSample(Army::DEFEND_SAMPLE);
                        }
                        stack.m_currFrameType = stack.m_nextFrameType;
                        stack.m_currFrameIndex = 0;
                    } else if (stack.m_currFrameIndex
                            < stack.m_stdIcon->getNumFrames(
                                stack.m_currFrameType) - 1) {
                        stack.m_currFrameIndex++;
                    } else if (stack.m_currFrameType != cs_wait
                            && stack.m_currFrameType != cs_death) {
                        stack.m_currFrameType = cs_wait;
                        stack.m_currFrameIndex = 0;
                        stack.m_powSequenceComplete = 1;
                    }
                }
            }

            if (showSomePowEffect
                    && frameCount < m_powSprite->getNumFrames(0))
                m_powFrameIndex = frameCount;

            drawFrame(0, 1, 0, 100, 1, 1);
            g_windowManager->updateScreen(
                m_drawbridgeBounds.m_minX, m_drawbridgeBounds.m_minY,
                m_drawbridgeBounds.m_maxX - m_drawbridgeBounds.m_minX + 1,
                m_drawbridgeBounds.m_maxY - m_drawbridgeBounds.m_minY + 1);
        }
    }

    for (side = 0; side < 2; side++) {
        for (slot = 0; slot < m_numArmies[side]; slot++) {
            Army& stack = m_armies[side][slot];
            if (stack.m_postPowSpellToCast != -1) {
                if (stack.m_numTroops > 0)
                    castSpell(stack.m_postPowSpellToCast, stack.m_gridIndex,
                              1, -1, 0, 3);
                stack.m_postPowSpellToCast = -1;
            }
        }
    }

    if (!isQuickCombat()) {
        for (;;) {
            int framesChanged = 0;
            for (side = 0; side < 2; side++) {
                for (slot = 0; slot < m_numArmies[side]; slot++) {
                    Army& stack = m_armies[side][slot];
                    if (stack.m_currFrameType == cs_wait)
                        continue;
                    if (stack.m_currFrameIndex
                            < stack.m_stdIcon->getNumFrames(
                                stack.m_currFrameType) - 1) {
                        stack.m_currFrameIndex++;
                    } else if (stack.m_currFrameType == cs_death) {
                        continue;
                    } else {
                        stack.m_currFrameType = cs_wait;
                        stack.m_currFrameIndex = 0;
                    }
                    framesChanged = 1;
                }
            }
            if (!framesChanged)
                break;
            drawFrame(1, 1, 0, 100, 1, 1);
        }
        if (resetLimitCreature)
            this->resetLimitCreature();
    }

    memset(m_creatureIsDead, 0, sizeof(m_creatureIsDead));
    m_someCreaturesVanish = 0;
    for (side = 0; side < 2; side++) {
        for (slot = 0; slot < m_numArmies[side]; slot++) {
            Army& stack = m_armies[side][slot];
            if (stack.m_allUnitsKilled) {
                stack.processDeath(0);
                if (stack.is(1u << 6))
                    m_heroes[side]->destroySiegeWeaponArtifact(
                        stack.m_creatureType);
            }
        }
    }
    if (m_someCreaturesVanish)
        makeCreaturesVanish();

    for (side = 0; side < 2; side++) {
        for (slot = 0; slot < m_numArmies[side]; slot++) {
            Army& stack = m_armies[side][slot];
            stack.m_showPowEffect = 0;
            stack.m_someUnitsDamaged = 0;
            stack.m_drawPriority = 4;
            stack.m_showAttackFrames = 0;
            stack.m_numTroopsToShowOverride = -1;
        }
    }
    drawFrame(1, 0, 0, 0, 1, 0);
}

VA(0x004693a0, 0x9F)
void CombatManager::unnamed4693a0(int side)
{
    g_game->m_isCheater = 1;
    if (g_unk69774c)
        g_game->m_campaign.m_isCheater = 1;
    turnOffHighlighter(1);

    Army* stack = &m_armies[side][0];
    for (int slot = 0; slot < m_numArmies[side]; slot++, stack++) {
        if (stack->m_creatureType != CREATURE_NONE
                && stack->m_creatureType != CREATURE_ARROW_TOWER) {
            stack->m_numTroops = 0;
            stack->processDeath(0);
        }
    }
}

VA(0x00469440, 0x1B2)
void CombatManager::checkRebirth()
{
    for (int side = 0; side < 2; side++) {
        Army* stack = &m_armies[side][0];
        for (int slot = 0; slot < m_numArmies[side]; slot++, stack++) {
            if (stack->m_creatureType != CREATURE_PHOENIX
                    || !(stack->is(1u << 21))
                    || stack->m_monInfo.m_hasSpell <= 0
                    || (stack->is(1u << 23)))
                continue;

            stack->m_monInfo.m_hasSpell--;
            stack->m_numTroops = 0;
            stack->m_topCreatureDamage = 0;
            int resurrected = stack->m_origNumTroops / 5;
            int rolls = stack->m_origNumTroops % 5;
            while (rolls--) {
                if (rand() % 5 == 0)
                    resurrected++;
            }
            if (!resurrected)
                continue;

            stack->m_monInfo.m_attributes |= 1 << 26;
            if (!isQuickCombat())
                launchSample(g_spellTraits[SPELL_RESURRECTION].m_sample,
                              -1, 3);
            resurrect(stack, stack->m_monInfo.m_hitPoints * resurrected, 0);
        }
    }
}

VA(0x00469600, 0x6E)  // dc 0x62db8
unsigned char CombatManager::enemyIsAdjacent(const Army* currentArmy, int gridIndex,
                                               const Army* excluded) const
{
    for (int i = 0; i < 6; i++) {
        int hex = m_adjacentCells[gridIndex][i];
        if (hex >= 0) {
            Army* a = m_cells[hex].getArmy();
            if (currentArmy->isEnemy(a) && a != excluded)
                return 1;
        }
    }
    return 0;
}

VA(0x00469670, 0xD2)  // dc 0x62e4c
long CombatManager::getDistance(long start, long stop)
{
    int sx = start % 0x11;
    int sy = start / 0x11;
    int tx = stop % 0x11;
    int ty = stop / 0x11;
    int a = (sy + 1) / 2 - (ty + 1) / 2 - sx + tx;
    int b = ty / 2 - sy / 2 - sx + tx;
    if ((a < 0) == (b < 0))
        return cppMax<long>(abs(a), abs(b));
    return abs(a) + abs(b);
}

VA(0x00469750, 0x12B)  // dc 0x62f00
void CombatManager::updateArmyLuckAndMorale()
{
    for (int side = 0; side < 2; side++) {
        for (int slot = 0; slot < m_numArmies[side]; slot++) {
            Army& stack = m_armies[side][slot];
            int ownerSide = stack.m_spellInfluence[60]
                ? 1 - stack.m_combatSide : stack.m_combatSide;
            Town* ownerTown;
            if (!ownerSide)
                ownerTown = 0;
            else
                ownerTown = m_defendingTown;
            stack.setLuck(m_heroes[ownerSide], m_armyGroups[ownerSide],
                          ownerTown, m_heroes[1 - ownerSide],
                          m_armyGroups[1 - ownerSide], m_magicTerrain);
            stack.setMorale(m_heroes[ownerSide], m_armyGroups[ownerSide],
                            ownerTown, m_heroes[1 - ownerSide],
                            m_armyGroups[1 - ownerSide], m_magicTerrain,
                            m_hasAngelicAlliance[ownerSide]);
        }
    }
}

VA(0x00469880, 0x190)  // dc 0x62fe4
void getMissileStartingPosition(int armyType, int x, int y, int facing,
                                int destX, int destY,
                                const CSprite* missile, int* startX,
                                int* startY, int* armyDir,
                                int* missileFrame)
{
    const MissileStartInfo& info = g_missileStartInfo[armyType];
    if (!facing)
        *startX = x - info.m_offsets[1][0];
    else
        *startX = x + info.m_offsets[1][0];
    *startY = y + info.m_offsets[1][1];

    int deltaX = destX - *startX;
    int deltaY = destY - *startY;
    float angle;
    if (!deltaX) {
        angle = deltaY > 0 ? 90.0 : -90.0;
    } else {
        angle = atan(-static_cast<double>(deltaY) / fabs(deltaX))
                * DATA_COMPGEN(0x0063d408, radiansToDegrees,
                               57.29577951308232);
    }

    if (missile) {
        int frames = missile->getNumFrames(0);
        int frame = 1;
        while (frame < frames
                && (info.m_angles[frame - 1] + info.m_angles[frame]) / 2.0f
                   >= angle)
            ++frame;
        *missileFrame = frame - 1;
    } else {
        *missileFrame = 0;
    }

    int offset;
    if (angle > DATA_COMPGEN(0x0063d400, upperMissileAngle, 25.0)) {
        *armyDir = 14;
        offset = 0;
    } else if (angle > DATA_COMPGEN(0x0063d3f8, lowerMissileAngle, -25.0)) {
        *armyDir = 15;
        offset = 1;
    } else {
        *armyDir = 16;
        offset = 2;
    }

    if (!facing)
        *startX = x - info.m_offsets[offset][0];
    else
        *startX = x + info.m_offsets[offset][0];
    *startY = y + info.m_offsets[offset][1];
}

// E:\gamedcs\cmbtmgr.cpp:4669
// DC's const-this record and lines 4675/4680/4686 prove the side check and
// both canonical HasArmy calls. Complete expands this ordinary helper in
// HexIsBlocked; its two cell/body tests are the same retail operands.
DC_ONLY(0x63268, 0x5A)
unsigned char CombatManager::doorCanBeLowered() const
{
    if (m_currentSide != 1)
        return 0;
    if (m_cells[COMBAT_HEX_GATE_MOAT].hasArmy()
        || m_cells[COMBAT_HEX_GATE_MOAT].m_bodiesInHex)
        return 0;
    if (m_cells[COMBAT_HEX_OUTER_MOAT].hasArmy()
        || m_cells[COMBAT_HEX_OUTER_MOAT].m_bodiesInHex)
        return 0;
    return 1;
}

VA(0x00469a10, 0x80)  // dc 0x632c4
unsigned char CombatManager::hexIsBlocked(int index) const
{
    if (m_fortificationLevel > 0
            && (index == COMBAT_HEX_GATE || index == COMBAT_HEX_GATE_MOAT)) {
        if (m_drawbridgeState == DRAWBRIDGE_UP && !doorCanBeLowered())
            return 1;
    } else if (m_cells[index].m_attributes & 2)
        return 1;
    return 0;
}

VA(0x00469a90, 0x324)  // dc 0x6335c
void CombatManager::damageMessage(const char* attacker, long attackerQty, long damage, const Army* defender, long deaths)
{
    if (isQuickCombat())
        return;

    std::string message;
    if (attackerQty == 1)
        message = formatString(
            g_generalText->getText(GENERAL_TEXT_COMBAT_DAMAGE_ONE_ATTACKER),
            attacker, damage);
    else
        message = formatString(
            g_generalText->getText(GENERAL_TEXT_COMBAT_DAMAGE_MANY_ATTACKERS),
            attacker, damage);

    if (deaths > 0) {
        std::string deathText;
        const char* name;
        bool stackWipedOut = false;
        if (defender) {
            name = defender->getName(deaths);
            if (defender->is(1u << 6)) {
                deathText = formatString(
                    g_generalText->getText(GENERAL_TEXT_COMBAT_STACK_WIPED_OUT),
                    name);
                stackWipedOut = true;
            }
        } else {
            if (deaths == 1)
                name = g_generalText->getText(GENERAL_TEXT_MIXED_ARMY_ONE);
            else
                name = g_generalText->getText(GENERAL_TEXT_MIXED_ARMY);
        }
        if (!stackWipedOut) {
            if (deaths == 1)
                deathText = formatString(
                    g_generalText->getText(GENERAL_TEXT_COMBAT_ONE_DEATH), name);
            else
                deathText = formatString(
                    g_generalText->getText(GENERAL_TEXT_COMBAT_MANY_DEATHS),
                    deaths, name);
        }
        message += deathText;
    }

    m_combatWindow->combatMessage(message.c_str(), 1, 0);
}

VA(0x00469dc0, 0x8D)  // dc 0x6351c
unsigned char CombatManager::isInMoat(int hex, int* index)
{
    if (m_moatOn) {
        for (int row = 0; row < 11; row++) {
            if (g_moatColumns[row] == hex
                    && (m_drawbridgeState == DRAWBRIDGE_UP
                        || hex != COMBAT_HEX_GATE_MOAT)) {
                if (index)
                    *index = row;
                return 1;
            }
        }
        if (m_defendingTown->m_type == TOWN_FORTRESS) {
            for (int row = 0; row < 11; row++) {
                if (g_outerMoatColumns[row] == hex
                        && (m_drawbridgeState == DRAWBRIDGE_UP
                            || hex != COMBAT_HEX_OUTER_MOAT)) {
                    if (index)
                        *index = row;
                    return 1;
                }
            }
        }
    }
    if (index)
        *index = -1;
    return 0;
}

VA(0x00469e50, 0xC1)
unsigned char CombatManager::unnamed469e50(
    int hex, Army* stack, unsigned char playSound)
{
    if (g_game->m_f1f698 >= 2 && stack->m_numTroops && isInMoat(hex, 0)) {
        if (playSound)
            stack->stopSample(Army::WALK_SAMPLE);

        int damage = g_moatDamage[m_defendingTown->m_type];
        if (damage) {
            int killed = stack->damage(damage);
            damageMessage(g_moatDamageMessages[m_defendingTown->m_type], 1,
                           damage, stack, killed);
            powEffect(-1, 1);
            if (stack->m_numTroops > 0 && playSound)
                stack->playSample(Army::WALK_SAMPLE);
            checkRebirth();
            return 1;
        }
    }
    return 0;
}

VA(0x00469f20, 0xBA)  // dc 0x6359c
void CombatManager::raiseSkeletons(int side)
{
    if (m_raisedCreatureCount > 0) {
        bool added;
        added = m_armyGroups[side]->add(
            m_raisedCreatureType, m_raisedCreatureCount, -1);
        if (!added) {
            CreatureType upgradedType = m_raisedCreatureType;
            if (!g_game->m_f1f698
                && isBaseElemental(upgradedType)) {
                upgradedType = CREATURE_NONE;
            } else {
                upgradedType = upgradedCreatureType(upgradedType);
            }

            m_raisedCreatureType = upgradedType;
            m_raisedCreatureCount = (m_raisedCreatureCount * 2 + 2) / 3;
            added = m_armyGroups[side]->add(
                m_raisedCreatureType, m_raisedCreatureCount, -1);
        }
        if (added)
            return;
    }

    m_raisedCreatureCount = 0;
}

VA(0x00469fe0, 0x88)  // dc 0x63648
void CombatManager::learnSpellFromEagleEye(int side)
{
    for (std::set<SpellID>::iterator it = m_eagleEyeData[side].begin();
         it != m_eagleEyeData[side].end(); it++) {
        SpellID spell = *it;
        if (m_heroes[side]->isWieldingArtifact(ARTIFACT_SPELLBOOK)
            && g_spellTraits[spell].m_level
                <= m_heroes[side]->m_skillLevel[eSecSkillWisdom] + 2)
            m_heroes[side]->addSpell(spell);
    }
}

// E:\gamedcs\cmbtmgr.cpp:4886
// DC 0x63704 owns both artifact loops and their local artifact copies.
// Lines 4900/4922 call hero::get_artifact/get_backpack; lines 4914/4936
// call vector::push_back. VC6's push_back delegates through insert(end(),
// value) to insert(end(), 1, value): retail retains the equipped append
// at 0x46a11a -> 0x54d330 and expands the backpack append. GiveArtifact
// calls at 0x46a0f8/0x46a190 return the success byte before removal.
// The looted_artifacts parameter is a reference: DC's public mangling has
// AAV (not PAV), despite an older roster rendering it as a pointer.

VA(0x0046a070, 0x2D3)  // dc 0x63704
void CombatManager::lootDeadHero(int side,
                                 std::vector<type_artifact>& lootedArtifacts)
{
    if (g_combatFlag6985a3)
        return;
    if (g_combatFlag697744)
        return;
    Hero* dead = m_heroes[1 - side];
    if (!dead)
        return;
    Hero* winner = m_heroes[side];
    for (int slot = 0; slot < 19; slot++) {
        // Complete walks 19 equipped ordinals; getArtifact retains DC's TArtifactSlot argument (Hero.h:18 positions).
        type_artifact artifact = dead->getArtifact(static_cast<ArtifactSlot>(slot) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */);
        if (artifact.m_artifactId == ARTIFACT_NONE
            || artifact.m_artifactId == ARTIFACT_HOLY_GRAIL
            || artifact.m_artifactId == ARTIFACT_SPELLBOOK
            || artifact.m_artifactId == ARTIFACT_CATAPULT
            || artifact.m_artifactId == ARTIFACT_BALLISTA
            || artifact.m_artifactId == ARTIFACT_AMMO_CART
            || artifact.m_artifactId == ARTIFACT_FIRST_AID_TENT)
            continue;
        if (!winner->giveArtifact(&artifact, 1, 0))
            return;
        dead->removeArtifact(slot);
        lootedArtifacts.push_back(artifact);
    }
    for (int index = 63; index >= 0; index--) {
        type_artifact artifact = dead->getBackpack(index);
        if (artifact.m_artifactId == ARTIFACT_NONE
            || artifact.m_artifactId == ARTIFACT_HOLY_GRAIL
            || artifact.m_artifactId == ARTIFACT_SPELLBOOK
            || artifact.m_artifactId == ARTIFACT_CATAPULT
            || artifact.m_artifactId == ARTIFACT_BALLISTA
            || artifact.m_artifactId == ARTIFACT_AMMO_CART
            || artifact.m_artifactId == ARTIFACT_FIRST_AID_TENT)
            continue;
        if (!winner->giveArtifact(&artifact, 1, 0))
            return;
        dead->removeBackpackArtifact(index);
        lootedArtifacts.push_back(artifact);
    }
}

VA(0x0046a350, 0x10C)  // dc 0x6388c
void CombatManager::calculateGainedExperience(int side, int* experienceGained)
{
    int total = experienceValueOfStack(1 - side);
    if (g_combatFlag6985a3 || g_combatFlag697744)
        total -= 500;
    if (m_defendingTown && side == 0)
        total += 500;
    if (m_heroes[side])
        total = static_cast<int>(
            m_heroes[side]->getExperienceBonusFactor()
            * static_cast<float>(total));
    *experienceGained = total;
}

VA(0x0046a460, 0x39)
void CombatManager::markTowerArmy(const Army* tower)
{
    switch (tower->m_gridIndex) {
    case COMBAT_HEX_LOWER_TOWER:
        m_archerEffect[1] = 1;
        break;
    case COMBAT_HEX_KEEP:
        m_archerEffect[0] = 1;
        break;
    case COMBAT_HEX_UPPER_TOWER:
        m_archerEffect[2] = 1;
        break;
    }
}

VA(0x0046a4a0, 0x71)  // dc 0x63900
bool CombatManager::isQuickCombat() const
{
    if (g_game->m_isTutorial)
        return false;
    if (g_videoPaused && m_sideIsAi[0] && m_sideIsAi[1]) {
        if (g_game->m_players[m_playerIds[0]].m_quickCombat
                && g_game->m_players[m_playerIds[1]].m_quickCombat)
            return true;
        return false;
    }
    return g_unnamed698758.m_quickCombat != 0;
}

VA(0x0046a520, 0x44)
void CombatManager::unnamed46a520(Army* stack)
{
    memset(m_obstacleAttackVisited, 0, COMBAT_GRID_CELLS);
    m_obstacleAttackVisited[stack->m_gridIndex] = 1;
    if (stack->m_monInfo.m_attributes & 1)
        m_obstacleAttackVisited[stack->getSecondGridIndex()] = 1;
}

VA(0x0046a570, 0xE0)
unsigned char CombatManager::checkObstacleAttacks(Army* thisArmy,
                                                    unsigned char isWalking)
{
    unsigned char attacked = 0;
    unsigned char moatAttacked = 0;
    int hex = thisArmy->m_gridIndex;

    if (!m_obstacleAttackVisited[hex]) {
        m_obstacleAttackVisited[hex] = 1;
        if (checkFireWall(hex, thisArmy, isWalking))
            attacked = 1;
        if (checkLandmine(hex, thisArmy, isWalking))
            attacked = 1;
        if (unnamed469e50(hex, thisArmy, isWalking)) {
            attacked = 1;
            moatAttacked = 1;
        }
    }

    if (thisArmy->m_monInfo.m_attributes & 1) {
        hex = thisArmy->getSecondGridIndex();
        if (!m_obstacleAttackVisited[hex]) {
            m_obstacleAttackVisited[hex] = 1;
            if (checkFireWall(hex, thisArmy, isWalking))
                attacked = 1;
            if (checkLandmine(hex, thisArmy, isWalking))
                attacked = 1;
            if (!moatAttacked
                    && unnamed469e50(hex, thisArmy, isWalking))
                attacked = 1;
        }
    }
    return attacked;
}

#if 0  // @carcass

// ..\stlport\stl_bvector.h:684
DC_ONLY(0x639c4, 0x18)
void std::vector<bool,std::allocator<bool> >::~vector<bool,std::allocator<bool> >()
{
    // @stub
}

// ..\stlport\stl_tree.h:176
DC_ONLY(0x639dc, 0xC)
unsigned char std::operator!=(const std::_Rb_tree_base_iterator* __x, const std::_Rb_tree_base_iterator* __y)
{
    // @stub
}

// E:\gamedcs\struct.h:270
DC_ONLY(0x639e8, 0x4)
void SLimitData::SLimitData()
{
    // @stub
}

// E:\gamedcs\struct.h:293
DC_ONLY(0x639ec, 0x2A)
unsigned char SLimitData::contains(int x, int y)
{
    // @stub
}

// E:\gamedcs\includes.h:134
DC_ONLY(0x63a18, 0x18)
void PickANumber::~PickANumber()
{
    // @stub
}

// E:\gamedcs\hero.h:695
DC_ONLY(0x63a30, 0x12)
void Hero::adjustPrimarySkill(int skill, int amount)
{
    // @stub
}

// E:\gamedcs\CmbtMgr.h:1500
DC_ONLY(0x63a44, 0xA)
int CombatManager::getHexIndex(int x, int y)
{
    // @stub
}

// E:\gamedcs\CmbtMgr.h:1506
DC_ONLY(0x63a50, 0xA)
unsigned char CombatManager::rowIsOdd(int y)
{
    // @stub
}

// E:\gamedcs\CmbtMgr.h:1537
DC_ONLY(0x63a5c, 0x2C)
Hexcell& CombatManager::getCell(int x, int y)
{
    // @stub
}

// E:\gamedcs\cmbtmgr.cpp:893
DC_ONLY(0x63a88, 0x34)
void* CNetMsgHandlerPause::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// ..\stlport\stl_bvector.h:334
DC_ONLY(0x63abc, 0x28)
void std::_Bvector_base<std::allocator<bool> >::~_Bvector_base<std::allocator<bool> >()
{
    // @stub
}

// ..\stlport\stl_bvector.h:343
DC_ONLY(0x63ae4, 0x2C)
void std::_Bvector_base<std::allocator<bool> >::_M_deallocate()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x63b10, 0x4)
void std::allocator<bool>::~allocator<bool>()
{
    // @stub
}

// ..\stlport\stl_set.h:72
DC_ONLY(0x63b14, 0x3C)
void std::set<enum SpellID,std::less<enum SpellID>,std::allocator<enum SpellID> >::set<enum SpellID,std::less<enum SpellID>,std::allocator<enum SpellID> >()
{
    // @stub
}

// ..\stlport\stl_set.h:119
DC_ONLY(0x63b50, 0x24)
std::_Rb_tree_iterator<enum std::set<enum SpellID,std::less<enum SpellID>,std::allocator<enum SpellID> >::begin(__$ReturnUdt)
{
    // @stub
}

// ..\stlport\stl_set.h:120
DC_ONLY(0x63b74, 0x24)
std::_Rb_tree_iterator<enum std::set<enum SpellID,std::less<enum SpellID>,std::allocator<enum SpellID> >::end(__$ReturnUdt)
{
    // @stub
}

// ..\stlport\stl_set.h:162
DC_ONLY(0x63b98, 0x18)
void std::set<enum SpellID,std::less<enum SpellID>,std::allocator<enum SpellID> >::clear()
{
    // @stub
}

// ..\stlport\stl_tree.h:206
DC_ONLY(0x63bb0, 0x6)
const SpellID* std::_Rb_tree_iterator<enum SpellID,std::_Const_traits<enum SpellID> >::operator*()
{
    // @stub
}

// ..\stlport\stl_tree.h:214
DC_ONLY(0x63bb8, 0x24)
std::_Rb_tree_iterator<enum std::_Rb_tree_iterator<enum SpellID,std::_Const_traits<enum SpellID> >::operator++(__$ReturnUdt, int __formal)
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0x63bdc, 0x4)
CombatManager::Obstacle* std::vector<CombatManager::Obstacle,std::allocator<CombatManager::Obstacle> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x63be0, 0x20)
unsigned std::vector<CombatManager::Obstacle,std::allocator<CombatManager::Obstacle> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0x63c00, 0x1C)
void std::vector<CombatManager::Obstacle,std::allocator<CombatManager::Obstacle> >::vector<CombatManager::Obstacle,std::allocator<CombatManager::Obstacle> >(const std::allocator<CombatManager::Obstacle>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0x63c1c, 0x3C)
void std::vector<CombatManager::Obstacle,std::allocator<CombatManager::Obstacle> >::push_back(const CombatManager::Obstacle* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0x63c58, 0x38)
void std::vector<CombatManager::Obstacle,std::allocator<CombatManager::Obstacle> >::clear()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x63c90, 0x4)
void std::allocator<CombatManager::Obstacle>::allocator<CombatManager::Obstacle>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x63c94, 0x4)
void std::allocator<CombatManager::Obstacle>::~allocator<CombatManager::Obstacle>()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0x63c98, 0x3C)
void std::vector<type_artifact,std::allocator<type_artifact> >::push_back(const type_artifact* __x)
{
    // @stub
}

// ..\stlport\stl_string.h:375
DC_ONLY(0x63cd4, 0x18)
void std::_STL_alloc_proxy<unsigned int *,unsigned int,std::allocator<bool> >::~_STL_alloc_proxy<unsigned int *,unsigned int,std::allocator<bool> >()
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x63cec, 0x2C)
void std::_STL_alloc_proxy<unsigned int *,unsigned int,std::allocator<bool> >::deallocate(unsigned* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_tree.h:424
DC_ONLY(0x63d18, 0x34)
void std::_Rb_tree<enum SpellID,enum SpellID,std::_Identity<enum SpellID>,std::less<enum SpellID>,std::allocator<enum SpellID> >::_Rb_tree<enum SpellID,enum SpellID,std::_Identity<enum SpellID>,std::less<enum SpellID>,std::allocator<enum SpellID> >(const std::less<enum* __comp, const std::allocator<enum* __a)
{
    // @stub
}

// ..\stlport\stl_tree.h:467
DC_ONLY(0x63d4c, 0x2C)
std::_Rb_tree_iterator<enum std::_Rb_tree<enum SpellID,enum SpellID,std::_Identity<enum SpellID>,std::less<enum SpellID>,std::allocator<enum SpellID> >::begin(__$ReturnUdt)
{
    // @stub
}

// ..\stlport\stl_tree.h:469
DC_ONLY(0x63d78, 0x24)
std::_Rb_tree_iterator<enum std::_Rb_tree<enum SpellID,enum SpellID,std::_Identity<enum SpellID>,std::less<enum SpellID>,std::allocator<enum SpellID> >::end(__$ReturnUdt)
{
    // @stub
}

// ..\stlport\stl_tree.h:545
DC_ONLY(0x63d9c, 0x58)
void std::_Rb_tree<enum SpellID,enum SpellID,std::_Identity<enum SpellID>,std::less<enum SpellID>,std::allocator<enum SpellID> >::clear()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0x63df4, 0x3C)
CombatManager::Obstacle* std::vector<CombatManager::Obstacle,std::allocator<CombatManager::Obstacle> >::erase(CombatManager::Obstacle* __first, CombatManager::Obstacle* __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0x63e30, 0x2C)
void std::_Vector_base<CombatManager::Obstacle,std::allocator<CombatManager::Obstacle> >::_Vector_base<CombatManager::Obstacle,std::allocator<CombatManager::Obstacle> >(const std::allocator<CombatManager::Obstacle>* __a)
{
    // @stub
}

// ..\stlport\stl_tree.h:343
DC_ONLY(0x63e5c, 0x6)
std::_Rb_tree_node<enum** std::_Rb_tree<enum SpellID,enum SpellID,std::_Identity<enum SpellID>,std::less<enum SpellID>,std::allocator<enum SpellID> >::_M_root()
{
    // @stub
}

// ..\stlport\stl_tree.h:345
DC_ONLY(0x63e64, 0x6)
std::_Rb_tree_node<enum** std::_Rb_tree<enum SpellID,enum SpellID,std::_Identity<enum SpellID>,std::less<enum SpellID>,std::allocator<enum SpellID> >::_M_leftmost()
{
    // @stub
}

// ..\stlport\stl_tree.h:347
DC_ONLY(0x63e6c, 0x6)
std::_Rb_tree_node<enum** std::_Rb_tree<enum SpellID,enum SpellID,std::_Identity<enum SpellID>,std::less<enum SpellID>,std::allocator<enum SpellID> >::_M_rightmost()
{
    // @stub
}

// ..\stlport\stl_tree.h:445
DC_ONLY(0x63e74, 0x48)
void std::_Rb_tree<enum SpellID,enum SpellID,std::_Identity<enum SpellID>,std::less<enum SpellID>,std::allocator<enum SpellID> >::_M_empty_initialize()
{
    // @stub
}

// ..\stlport\stl_tree.h:259
DC_ONLY(0x63ebc, 0x30)
void std::_Rb_tree_base<enum SpellID,std::allocator<enum SpellID> >::_Rb_tree_base<enum SpellID,std::allocator<enum SpellID> >(const std::allocator<enum* __a)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x63eec, 0x2C)
void std::_STL_alloc_proxy<std::_Rb_tree_node<enum SpellID> *,std::_Rb_tree_node<enum SpellID>,std::allocator<enum SpellID> >::deallocate(std::_Rb_tree_node<enum* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_tree.h:202
DC_ONLY(0x63f18, 0x6)
void std::_Rb_tree_iterator<enum SpellID,std::_Const_traits<enum SpellID> >::_Rb_tree_iterator<enum SpellID,std::_Const_traits<enum SpellID> >(std::_Rb_tree_node<enum* __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x63f20, 0xC)
void std::_STL_alloc_proxy<CombatManager::Obstacle *,CombatManager::Obstacle,std::allocator<CombatManager::Obstacle> >::_STL_alloc_proxy<CombatManager::Obstacle *,CombatManager::Obstacle,std::allocator<CombatManager::Obstacle> >(const std::allocator<CombatManager::Obstacle>* __a, CombatManager::Obstacle** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x63f2c, 0x2C)
void std::_STL_alloc_proxy<CombatManager::Obstacle *,CombatManager::Obstacle,std::allocator<CombatManager::Obstacle> >::deallocate(CombatManager::Obstacle* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x63f58, 0x1C)
void std::allocator<unsigned int>::deallocate(unsigned* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_tree.h:360
DC_ONLY(0x63f74, 0x4)
unsigned char* std::_Rb_tree<enum SpellID,enum SpellID,std::_Identity<enum SpellID>,std::less<enum SpellID>,std::allocator<enum SpellID> >::_S_color(std::_Rb_tree_node<enum* __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x63f78, 0xC)
void std::_STL_alloc_proxy<std::_Rb_tree_node<enum SpellID> *,std::_Rb_tree_node<enum SpellID>,std::allocator<enum SpellID> >::_STL_alloc_proxy<std::_Rb_tree_node<enum SpellID> *,std::_Rb_tree_node<enum SpellID>,std::allocator<enum SpellID> >(const std::allocator<enum* __a, std::_Rb_tree_node<enum** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x63f84, 0x28)
std::_Rb_tree_node<enum* std::_STL_alloc_proxy<std::_Rb_tree_node<enum SpellID> *,std::_Rb_tree_node<enum SpellID>,std::allocator<enum SpellID> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x63fac, 0x20)
void std::allocator<CombatManager::Obstacle>::deallocate(CombatManager::Obstacle* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x63fcc, 0x20)
void std::allocator<std::_Rb_tree_node<enum SpellID> >::deallocate(std::_Rb_tree_node<enum* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x63fec, 0x28)
std::_Rb_tree_node<enum* std::allocator<std::_Rb_tree_node<enum SpellID> >::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_tree.c:552
DC_ONLY(0x64014, 0x54)
void std::_Rb_tree<enum SpellID,enum SpellID,std::_Identity<enum SpellID>,std::less<enum SpellID>,std::allocator<enum SpellID> >::_M_erase(std::_Rb_tree_node<enum* __x)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0x64068, 0xDC)
void std::vector<CombatManager::Obstacle,std::allocator<CombatManager::Obstacle> >::_M_insert_overflow(CombatManager::Obstacle* __position, const CombatManager::Obstacle* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0x64144, 0xD0)
void std::vector<type_artifact,std::allocator<type_artifact> >::_M_insert_overflow(type_artifact* __position, const type_artifact* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_tree.c:311
DC_ONLY(0x64214, 0x30)
void std::_Rb_global<bool>::_M_increment(std::_Rb_tree_base_iterator* __it)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x64244, 0x30)
void std::destroy(CombatManager::Obstacle* __first, CombatManager::Obstacle* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x64274, 0x3C)
void std::construct(CombatManager::Obstacle* __p, const CombatManager::Obstacle* __value)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x642b0, 0x2C)
void std::construct(type_artifact* __p, const type_artifact* __value)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x642dc, 0x4)
std::allocator<unsigned* std::__stl_alloc_rebind(std::allocator<bool>* __a, const unsigned* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0x642e0, 0x50)
CombatManager::Obstacle* std::copy(CombatManager::Obstacle* __first, CombatManager::Obstacle* __last, CombatManager::Obstacle* __result)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x64330, 0x4)
std::allocator<std::_Rb_tree_node<enum* std::__stl_alloc_rebind(std::allocator<enum* __a, const std::_Rb_tree_node<enum* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x64334, 0x4)
std::allocator<CombatManager::Obstacle>* std::__stl_alloc_rebind(std::allocator<CombatManager::Obstacle>* __a, const CombatManager::Obstacle* __formal)
{
    // @stub
}

// ..\stlport\stl_tree.h:333
DC_ONLY(0x64338, 0x30)
void std::_Rb_tree<enum SpellID,enum SpellID,std::_Identity<enum SpellID>,std::less<enum SpellID>,std::allocator<enum SpellID> >::destroy_node(std::_Rb_tree_node<enum* __p)
{
    // @stub
}

// ..\stlport\stl_tree.h:350
DC_ONLY(0x64368, 0x6)
std::_Rb_tree_node<enum** std::_Rb_tree<enum SpellID,enum SpellID,std::_Identity<enum SpellID>,std::less<enum SpellID>,std::allocator<enum SpellID> >::_S_left(std::_Rb_tree_node<enum* __x)
{
    // @stub
}

// ..\stlport\stl_tree.h:352
DC_ONLY(0x64370, 0x6)
std::_Rb_tree_node<enum** std::_Rb_tree<enum SpellID,enum SpellID,std::_Identity<enum SpellID>,std::less<enum SpellID>,std::allocator<enum SpellID> >::_S_right(std::_Rb_tree_node<enum* __x)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x64378, 0x28)
CombatManager::Obstacle* std::_STL_alloc_proxy<CombatManager::Obstacle *,CombatManager::Obstacle,std::allocator<CombatManager::Obstacle> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x643a0, 0xC)
unsigned std::vector<type_artifact,std::allocator<type_artifact> >::size()
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x643ac, 0x28)
type_artifact* std::_STL_alloc_proxy<type_artifact *,type_artifact,std::allocator<type_artifact> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x643d4, 0x2C)
void std::_STL_alloc_proxy<type_artifact *,type_artifact,std::allocator<type_artifact> >::deallocate(type_artifact* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x64400, 0x28)
CombatManager::Obstacle* std::allocator<CombatManager::Obstacle>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x64428, 0x28)
type_artifact* std::allocator<type_artifact>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x64450, 0x1C)
void std::allocator<type_artifact>::deallocate(type_artifact* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x6446c, 0x38)
CombatManager::Obstacle* std::uninitialized_copy(CombatManager::Obstacle* __first, CombatManager::Obstacle* __last, CombatManager::Obstacle* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0x644a4, 0x38)
CombatManager::Obstacle* std::uninitialized_fill_n(CombatManager::Obstacle* __first, unsigned __n, const CombatManager::Obstacle* __x)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x644dc, 0x38)
type_artifact* std::uninitialized_copy(type_artifact* __first, type_artifact* __last, type_artifact* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0x64514, 0x38)
type_artifact* std::uninitialized_fill_n(type_artifact* __first, unsigned __n, const type_artifact* __x)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x6454c, 0x30)
void std::destroy(type_artifact* __first, type_artifact* __last)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x6457c, 0x4)
CombatManager::Obstacle* std::value_type(const CombatManager::Obstacle* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x64580, 0x1C)
void std::__destroy(CombatManager::Obstacle* __first, CombatManager::Obstacle* __last, CombatManager::Obstacle* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0x6459c, 0xC)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const CombatManager::Obstacle* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0x645a8, 0x4)
int* std::distance_type(const CombatManager::Obstacle* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0x645ac, 0x54)
CombatManager::Obstacle* std::__copy(CombatManager::Obstacle* __first, CombatManager::Obstacle* __last, CombatManager::Obstacle* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x64600, 0x4)
std::allocator<type_artifact>* std::__stl_alloc_rebind(std::allocator<type_artifact>* __a, const type_artifact* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x64604, 0x1C)
CombatManager::Obstacle* std::__uninitialized_copy(CombatManager::Obstacle* __first, CombatManager::Obstacle* __last, CombatManager::Obstacle* __result, CombatManager::Obstacle* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0x64620, 0x1C)
CombatManager::Obstacle* std::__uninitialized_fill_n(CombatManager::Obstacle* __first, unsigned __n, const CombatManager::Obstacle* __x, CombatManager::Obstacle* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x6463c, 0x4)
type_artifact* std::value_type(const type_artifact* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x64640, 0x1C)
type_artifact* std::__uninitialized_copy(type_artifact* __first, type_artifact* __last, type_artifact* __result, type_artifact* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0x6465c, 0x1C)
type_artifact* std::__uninitialized_fill_n(type_artifact* __first, unsigned __n, const type_artifact* __x, type_artifact* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x64678, 0x1C)
void std::__destroy(type_artifact* __first, type_artifact* __last, type_artifact* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x64694, 0x30)
void std::__destroy_aux(CombatManager::Obstacle* __first, CombatManager::Obstacle* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x646c4, 0x3C)
CombatManager::Obstacle* std::__uninitialized_copy_aux(CombatManager::Obstacle* __first, CombatManager::Obstacle* __last, CombatManager::Obstacle* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0x64700, 0x3C)
CombatManager::Obstacle* std::__uninitialized_fill_n_aux(CombatManager::Obstacle* __first, unsigned __n, const CombatManager::Obstacle* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x6473c, 0x3C)
type_artifact* std::__uninitialized_copy_aux(type_artifact* __first, type_artifact* __last, type_artifact* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0x64778, 0x3C)
type_artifact* std::__uninitialized_fill_n_aux(type_artifact* __first, unsigned __n, const type_artifact* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x647b4, 0x30)
void std::__destroy_aux(type_artifact* __first, type_artifact* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x647e4, 0x1C)
void std::destroy(CombatManager::Obstacle* __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x64800, 0x1C)
void std::destroy(type_artifact* __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0x6481c, 0x4)
void std::__destroy_aux()
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0x64820, 0x4)
void std::__destroy_aux()
{
    // @stub
}

#endif  // @carcass

VA_COMPGEN(0x0046aeb0, 0x2E4, VECTOR_INSERT_COUNT, Obstacle)
VA_COMPGEN(0x0046b1a0, 0x3B, VECTOR_UCOPY, Obstacle)
VA_COMPGEN(0x0046b1e0, 0x31, VECTOR_UFILL, Obstacle)

VA_COMPGEN(0x0046A680, 0xBE, CLASS_CTOR, set)

// erase(iterator, iterator) - the range form, `ret 0xc` for the hidden
// return plus two by-value iterators.
VA_COMPGEN(0x0046a740, 0x121, TREE_ERASE_RANGE, int_set)

// erase(iterator) - the single-iterator form, `ret 8`.
VA_COMPGEN(0x0046a870, 0x50F, TREE_ERASE_ITERATOR, int_set)

VA_COMPGEN(0x0046ad80, 0x7E, TREE_ERASE, int_set)

VA_COMPGEN(0x0046ae00, 0xA3, TREE_CONST_ITERATOR_INC, int_set)
