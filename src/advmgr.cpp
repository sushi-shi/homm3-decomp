// advmgr.cpp - E:\gamedcs\advmgr.cpp (compiland advmgr.obj)

// LOCATE SWEEP 2026-08-08 (span 0x5de0..0x1b0da, 112 carve rows). The
// DC roster was order-mapped bracket by bracket and every pairing was
// checked against `ret N` arity (thiscall = this in ecx, so ret N ==
// 4*(params-1); /Gr free functions put the first two in ecx/edx, so
// ret N == 4*(params-2)), against the claimed-callee graph, and
// against size plausibility. Three brackets came out exhaustive:

//   [0xfd20..0x15d60]  19 DC rows -> 17 retail rows, and the arity
//     column alone separates them: four `ret 0x20` (the 9-param
//     DrawHeroPart/DrawHeroPartShadow/DrawBoatPart/DrawBoatPartShadow
//     quartet), then nine consecutive `ret 0x14` for the nine 6-param
//     Draw* bodies CompleteDraw calls, then GetCell, the two
//     UpdateRadar overloads (`ret 0x18` = 7 params, `ret 0x14` = 6)
//     and QuickInfo (`ret 0xc` = 4). The two DC rows with no retail
//     body are the 4-param GetCell overload (0x14b08 - retail keeps
//     only the `ret 4` type_point form at 0x412bd0) and
//     ClearBottomView (0x18c2c), whose slot would have to sit between
//     QuickInfo and OverrideBottomView and the carve shows those two
//     abutting exactly.

//   [0x19570..0x1a1e0]  7 DC rows -> 6 retail rows; MouseInScrollZone
//     is inlined into its one caller CheckScreenScroll (which calls
//     ScreenScroll, and PopupPlayerTurnInfo is the only 0-param free
//     function in the run - it loads sysmsg.wav and sprintf's the
//     player name).

//   [0x1a460..0x1b040]  10 DC rows -> 7 retail rows. Anchored on
//     ViewPuzzle (loads obelisk.wav, PUZZLE.PCX, SetupPuzzlePieces),
//     DoAdventureOptions (ADVOPTS.PCX, and it dispatches to
//     ProcessSearch and ViewPuzzle) and DoSystemOptions (SysOpBck.pcx,
//     SaveGame). PuzzleDraw, DoAdvMenu and the by-value GetRouteArray
//     are the three with no retail row - the surviving 39-byte
//     0x41b010 returns `lea eax,[ecx+eax*2]`, a pointer, so it is
//     GetRouteArrayPtr and not its `unsigned short` sibling.

// Other DC rows proven bodyless here: InitializeCreatureGeneratorNames
// and InitializeExtraInfoText (before the span's first row),
// GetCursorSampleSet, getMouseMapPoint, ProcessAdvMenu,
// ProcessMapSelect2, get_garrison_cursor, get_identify_level (a static
// inlined into all four *QuickView bodies), type_cell_adjuster's ctor
// and restore_cell, and six of the eleven help-text builders.

// The retail-only 0xabe0 army describer is now claimed by its complete
// operation set and caller role (see the reconstruction at that address).
// 0xd670 (595 B, ret 0xc), a help-text builder with the same two callees
// as get_creature_bank_help_text, remains UNCLAIMED: the DC roster has no
// third 5-param help builder, so its call surface keeps an ordinal placeholder
// and its body remains unclaimed. See the help-text note further down.
#include "includes.h"
#include "creaturetype.h"
#include "homm3_limit.h"
#include <va.h>
#include <stdio.h>
#include <string.h>
#include "herospec.h"
#include "advmgr.h"
#include "ai_player.h"
#include "adventureoptionswindow.h"
#include "university_window.h"
#include "advmgr_objects.h"
#include "bitmap16.h"
#include "bitmap816.h"
#include "bottomviewsubwindow.h"
#include "button.h"
#include "csprite.h"
#include "game.h"
#include "creature_bank.h"
#include "exec.h"
#include "findpath.h"
#include "inputmgr.h"
#include "kb.h"
#include "kbwin.h"
#include "mousemgr.h"
#include "misc.h"
#include "prefs.h"
#include "recruit.h"
#include "remote.h"
#include "sample.h"
#include "resourcemanager.h"
#include "soundmgr.h"
#include "textntry.h"
#include "winmgr.h"
#include "window.h"
#include "widget.h"
#include "quickherowindow.h"
#include "quicktownwindow.h"
#include "quickinfowindow.h"
#include "questlogwindow.h"
#include "puzzlewindow.h"
#include "singleselectionwindow.h"
#include "netgame.h"
#include "systemoptionswindow.h"

// Adventure-turn ownership for this machine. Dreamcast publishes the
// original name; retail fixes the dword at 0x697788 through this routine and
// every remote-turn consumer.
DATA(0x00697788) int g_thisNetGotAdventureControl;

// includes.h:134 supplies the shared limit calls below.

// The three text resources this compiland keeps alive for the rollover
// tables below. Every reference to all three in the whole image is one of
// the four stores in the two readers that follow (config/retail-reloc-
// evidence.tsv), so they are source-private here under the same rule
// kb.cpp's oldmain cells use. Names are role inventions.
DATA(0x00691350) static TextResource* g_creatureGenerator1Text;
DATA(0x0069163c) static TextResource* g_creatureGenerator4Text;
DATA(0x00691368) static TextResource* g_extraInfoText;

VA(0x00405d20, 0x60)  // dc 0x5714
unsigned char initializeCreatureGeneratorNames()
{
    g_creatureGenerator1Text = ResourceManager::getText(
        DATA_COMPGEN(0x00660278, creatureGenerator1TextName, "crgen1.txt"));
    if (g_creatureGenerator1Text == 0)
        return 0;
    int i;
    for (i = 0; i < 80; i++)
        g_creatureGenerator1RolloverNames[i] =
            g_creatureGenerator1Text->getText(i);

    g_creatureGenerator4Text = ResourceManager::getText(
        DATA_COMPGEN(0x0066026c, creatureGenerator4TextName, "crgen4.txt"));
    if (g_creatureGenerator4Text == 0)
        return 0;
    for (i = 0; i < 2; i++)
        g_creatureGenerator4RolloverNames[i] =
            g_creatureGenerator4Text->getText(i);
    return 1;
}

VA(0x00405d80, 0x30)  // dc 0x57cc
unsigned char initializeExtraInfoText()
{
    g_extraInfoText = ResourceManager::getText(
        DATA_COMPGEN(0x00660284, extraInfoTextName, "xtrainfo.txt"));
    if (g_extraInfoText == 0)
        return 0;
    for (int i = 0; i < 28; i++)
        g_globalInfoFlagNames[i] = g_extraInfoText->getText(i);
    return 1;
}

VA(0x00405de0, 0xD)  // dc 0x5864
BlackBoxData* ExtraInfoUnion::getBlackBox() const
{
    return g_advManager->getBlackBox(this);
}

VA(0x00405df0, 0x20)  // dc 0x5888
CreatureBank& ExtraInfoUnion::getCreatureBank() const
{
    return g_game->m_creatureBanks[m_creatureBankInfo.m_index];
}

VA(0x00405e10, 0x1C)  // dc 0x58bc
University* ExtraInfoUnion::getUniversity() const
{
    return &g_game->m_universities[m_universityInfo.m_index];
}

VA(0x00405e30, 0x64B)  // dc 0x58f0
CNetMsg* CAdvMgrNetMsgHandler::handleNetMsg(CNetMsg* netMsg)
{
    if (netMsg->m_subType >= RS_MAP_CHANGE_START
        && netMsg->m_subType <= RS_MAP_CHANGE_END) {
        g_advManager->processMapChangeNew(
            static_cast<CMapChange*>(netMsg));
        destroyMsg(netMsg);
        return 0;
    }

    switch (netMsg->m_subType) {
    case RS_GAME_TRANSMIT_INIT: {
        if (m_inPopup) {
            m_abortPopupMsg = netMsg;
            return 0;
        }
        CGameTransmitInitMsg* msg =
            static_cast<CGameTransmitInitMsg*>(netMsg);
        if (g_game->receiveSaveGame(msg->m_fileSize, msg->m_fullGameCrc,
                                    msg->m_from, 1, msg->m_isDiff)) {
            if (msg->m_thisPlayerDead)
                handleRemoteDeadPlayerExit(msg->m_from, 1);
            g_advManager->loadRemote(msg->m_makeOrig);
        }
        break;
    }
    case RS_CHAT_MSG: {
        CChatMsg* msg = static_cast<CChatMsg*>(netMsg);
        receiveChat(msg->m_text, msg->m_from);
        break;
    }
    case RS_COMBAT_INIT:
        if (m_inPopup) {
            m_abortPopupMsg = netMsg;
            return 0;
        }
        g_advManager->doNetCombat(netMsg);
        break;
    case RS_TURN_UPDATE: {
        CTurnUpdateMsg* msg = static_cast<CTurnUpdateMsg*>(netMsg);
        int pos = msg->m_gamePos;
        g_netLocalGamePos = pos;
        g_currentPlayer = &g_game->m_players[pos];
        g_unnamed69ccc4 = 1 << pos;
        if (!m_inPopup) {
            g_advManager->m_bottomViewType = AdvManager::BOTTOM_VIEW_DEFAULT;
            g_advManager->updBottomView(1, 1, 1);
        }
        if (g_currentPlayer->isHuman()) {
            g_chatMan.systemMsg(g_generalText->getText(352),
                      g_currentPlayer->m_name);
            g_unnamed69d810 = g_netLocalGamePos;
        }
        break;
    }
    case RS_PLAYER_DROPPED: {
        CPlayerDroppedMsg* msg = static_cast<CPlayerDroppedMsg*>(netMsg);
        if (m_inPopup
            && g_game->getGamePosFromDPID(msg->m_dpidFrom) == g_unnamed69d810) {
            m_abortPopupMsg = netMsg;
            return 0;
        }
        handlePlayerDrop(msg->m_gamePos);
        break;
    }
    case RS_PLAYER_DROP_UPDATE: {
        if (m_inPopup) {
            m_abortPopupMsg = netMsg;
            return 0;
        }
        CPlayerDropUpdateMsg* msg =
            static_cast<CPlayerDropUpdateMsg*>(netMsg);
        onPlayerDropUpdateMsg(msg->m_dpidDropped);
        break;
    }
    case RS_PLAYER_DEAD: {
        CPlayerDeadMsg* msg = static_cast<CPlayerDeadMsg*>(netMsg);
        handlePlayerDead(msg->m_gamePos, 1);
        break;
    }
    case RS_PLAYER_WON:
        if (m_inPopup) {
            m_abortPopupMsg = netMsg;
            return 0;
        }
        handlePlayerWon(netMsg);
        break;
    case RS_PLAYER_LOST:
        if (m_inPopup) {
            m_abortPopupMsg = netMsg;
            return 0;
        }
        handlePlayerLost(netMsg);
        break;
    case RS_SET_VISIBILITY: {
        CSetVisibilityMsg* msg = static_cast<CSetVisibilityMsg*>(netMsg);
        g_game->setVisibility(msg->m_point.m_x, msg->m_point.m_y,
                              msg->m_point.m_z, msg->m_playerPos,
                              msg->m_range, 0);
        g_advManager->updateRadar(g_advManager->m_radarOrigin, 1, 1, 0, 0, 0);
        g_advManager->completeDraw(g_advManager->m_radarOrigin.m_x,
                                   g_advManager->m_radarOrigin.m_y,
                                   g_advManager->m_radarOrigin.m_z, 0, 1);
        AdvManager* manager = g_advManager;
        g_windowManager->updateScreen(AdvManager::ADVENTURE_SCREEN_X,
                                      AdvManager::ADVENTURE_SCREEN_Y,
                                      AdvManager::ADVENTURE_SCREEN_WIDTH,
                                      AdvManager::ADVENTURE_SCREEN_HEIGHT);
        unsigned long curTime = GameTime::get();
        if (static_cast<long>(
                curTime - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT])
                >= 0
            && !manager->m_animCtrPaused) {
            ++manager->m_animCtr;
            long elapsedTime =
                curTime - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT];
            g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT] +=
                cppMax(elapsedTime,
                         static_cast<long>(
                             AdvManager::ADVENTURE_ANIMATION_MAX_ELAPSED));
        }
        process1WindowsMessage();
        break;
    }
    case RS_RESET_VISIBILITY: {
        CSetVisibilityMsg* msg = static_cast<CSetVisibilityMsg*>(netMsg);
        g_game->setVisibility(msg->m_point.m_x, msg->m_point.m_y,
                              msg->m_point.m_z, msg->m_playerPos,
                              msg->m_range, 0);
        g_advManager->updateRadar(g_advManager->m_radarOrigin, 1, 1, 0, 0, 0);
        g_advManager->completeDraw(g_advManager->m_radarOrigin.m_x,
                                   g_advManager->m_radarOrigin.m_y,
                                   g_advManager->m_radarOrigin.m_z, 0, 1);
        AdvManager* manager = g_advManager;
        g_windowManager->updateScreen(AdvManager::ADVENTURE_SCREEN_X,
                                      AdvManager::ADVENTURE_SCREEN_Y,
                                      AdvManager::ADVENTURE_SCREEN_WIDTH,
                                      AdvManager::ADVENTURE_SCREEN_HEIGHT);
        unsigned long curTime = GameTime::get();
        if (static_cast<long>(
                curTime - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT])
                >= 0
            && !manager->m_animCtrPaused) {
            ++manager->m_animCtr;
            long elapsedTime =
                curTime - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT];
            g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT] +=
                cppMax(elapsedTime,
                         static_cast<long>(
                             AdvManager::ADVENTURE_ANIMATION_MAX_ELAPSED));
        }
        process1WindowsMessage();
        break;
    }
    case RS_COMBAT_TYPE: {
        CCombatTypeMsg* msg = static_cast<CCombatTypeMsg*>(netMsg);
        g_game->m_players[msg->m_from].m_quickCombat = msg->m_quick;
        break;
    }
    case RS_TRADE_REQUEST: {
        if (m_inPopup) {
            m_abortPopupMsg = netMsg;
            return 0;
        }
        CTradeRequestMsg* msg = static_cast<CTradeRequestMsg*>(netMsg);
#pragma inline_depth(0)
        g_game->m_heroes[msg->m_left.m_id] = msg->m_left;
        g_game->m_heroes[msg->m_right.m_id] = msg->m_right;
#pragma inline_depth()
        g_advManager->heroSwap(&g_game->m_heroes[msg->m_left.m_id],
                               &g_game->m_heroes[msg->m_right.m_id]);
        break;
    }
    case RS_PLAYER_ACTIVE:
        g_chatMan.systemMsg(g_generalText->getText(40),
            g_game->getPlayerName(g_game->getLocalPlayerGamePos()));
        break;
    case RS_GIFT:
        handleTradeRequestMsg(netMsg);
        break;
    case RS_GIFT_REQUEST:
        handleGiftMsg(netMsg);
        break;
    case RS_SESSION_LOST:
        if (m_inPopup) {
            m_abortPopupMsg = netMsg;
            return 0;
        }
        netMsg = CNetMsgHandler::handleNetMsg(netMsg);
        break;
    case RS_NORMAL_WIN:
        if (m_inPopup) {
            m_abortPopupMsg = netMsg;
            return 0;
        }
        handleNormalWinMsg(netMsg);
        break;
    default:
        netMsg = CNetMsgHandler::handleNetMsg(netMsg);
        break;
    }

    if (netMsg)
        destroyMsg(netMsg);
    return 0;
}

#if 0  // @carcass

// E:\gamedcs\advmgr.cpp:651
DC_ONLY(0x5fc8, 0x204)
void CAdvMgrNetMsgHandler::handleGiftRequestMsg(CNetMsg* pNetMsg)
{
    // @stub
}

// The retained source name handles a resource request (RS_GIFT_REQUEST).
// E:\gamedcs\advmgr.cpp:677
#endif  // @carcass

VA_COMPGEN(0x00406480, 0x59F, IMPLICIT_COPY_ASSIGN, Hero)

VA(0x00406a20, 0x1C7)  // dc 0x61cc
void CAdvMgrNetMsgHandler::handleGiftMsg(CNetMsg* netMsg)
{
    CGiftRequestMsg* msg = static_cast<CGiftRequestMsg*>(netMsg);
    std::string text;
    if (g_game->m_players[msg->m_greedyGuy].isHuman()) {
        text = formatString(
            g_generalText->getText(GENERAL_TEXT_AI_SINGLE_RESOURCE_REQUEST),
            g_game->m_players[msg->m_greedyGuy].m_name,
            g_resourceNames[msg->m_resource]);
    } else {
        text = formatString(
            g_generalText->getText(GENERAL_TEXT_AI_SINGLE_RESOURCE_REQUEST),
            g_playerColorNames[msg->m_greedyGuy],
            g_resourceNames[msg->m_resource]);
    }

    std::vector<DialogResource> resources;
    DialogResource resource;
    resource.m_resource = msg->m_resource;
    resource.m_qualifier = 0;
    resources.push_back(resource);
    extendedDialog(text.c_str(), resources, -1, -1, 15000);
    resources.clear();
}

// The retained source name handles a gift (RS_GIFT), using its giver, resource,
// and quantity payload.
VA(0x00406bf0, 0x1FA)  // dc 0x6428
void CAdvMgrNetMsgHandler::handleTradeRequestMsg(CNetMsg* netMsg)
{
    CGiftMsg* msg = static_cast<CGiftMsg*>(netMsg);
    std::string text;
    if (g_game->m_players[msg->m_niceGuy].isHuman()) {
        text = formatString(
            g_generalText->getText(GENERAL_TEXT_AI_GIFT_RECEIVED),
            g_game->m_players[msg->m_niceGuy].m_name);
    } else {
        text = formatString(
            g_generalText->getText(GENERAL_TEXT_AI_GIFT_RECEIVED),
            g_playerColorNames[msg->m_niceGuy]);
    }

    std::vector<DialogResource> resources;
    DialogResource resource;
    resource.m_resource = msg->m_resource;
    resource.m_qualifier = msg->m_qty;
    resources.push_back(resource);
    extendedDialog(text.c_str(), resources, -1, -1, 15000);
    resources.clear();

    PlayerData* localPlayer = g_game->getLocalPlayer();
    if (localPlayer) {
        localPlayer->m_resources[msg->m_resource] += msg->m_qty;
        g_advManager->m_advWindow->updateResourceDisplay(!isInPopup(), 1);
    }
}

// E:\gamedcs\advmgr.cpp:734
// The adventure manager's big zero fill, in retail's own statement
// grouping: the ground tileset and hero sample rows share one counted
// loop (the bytes walk both off a single induction pointer at +0x360
// with a -0x300 displacement, as in Close), the river and road rows are
// small memsets starting at index 1 (slot 0 is the never-loaded "none"
// entry, and each memset zeroes its own scratch register - the xor
// eax/xor ecx pair), the boat and froth rows share a second counted
// loop, and the four dword-array fills past the 16-byte unroll cutoff
// (cursorIcons, flagIcons, boatFlagIcons, loopedSample) are rep-stosd
// memsets. radarIcons is nulled twice - both stores are retail's.
// radarOrigin is a BODY assignment from a type_point(0,0,0) temporary
// (the masked ebp-0x14 temp copied as one dword), not an init-list item:
// as an init-list item VC6 emits it in declaration order between the
// vector and the string (83.04), as the first body statement it lands
// after the string exactly as retail schedules it (90.30). Listing
// members out of declaration order in the init list is byte-inert -
// VC6 initializes in declaration order regardless (measured). The
// implicit vector and string defaults carry the two EH states;
// advCommand and the moving-object pair share one or-edx,-1.

VA(0x00406df0, 0x1DF)  // anchor-global, dc 0x66f4
AdvManager::AdvManager()
{
    m_radarOrigin = type_point(0, 0, 0);
    m_radarIcons = 0;
    m_scrollX = 0;
    m_scrollY = 0;
    m_animFrame = 0;
    m_animCtr = 0;
    m_flagFrame = 0;
    m_advCommand = -1;
    m_drawCursor = 0;
    m_debugShowFps = 0;
    m_debugViewAll = 0;
    m_heroMoving = 0;

    for (int i = 0; i < 10; i++) {
        m_groundTileset[i] = 0;
        m_heroSamples[i] = 0;
    }
    memset(&m_riverTileset[1], 0, 4 * sizeof(CSprite*));
    memset(&m_roadTileset[1], 0, 3 * sizeof(CSprite*));
    m_borderTileset = 0;
    m_arrowTileset = 0;
    m_gemIcons[0] = 0;
    m_gemIcons[1] = 0;
    m_gemIcons[2] = 0;
    m_gemIcons[3] = 0;
    m_starTileset = 0;
    m_cloudIcons = 0;
    memset(m_cursorIcons, 0, sizeof(m_cursorIcons));
    for (int boat = 0; boat < 3; boat++) {
        m_boatIcons[boat] = 0;
        m_boatFrothIcons[boat] = 0;
    }
    memset(m_flagIcons, 0, sizeof(m_flagIcons));
    memset(m_boatFlagIcons, 0, sizeof(m_boatFlagIcons));
    memset(m_loopedSample, 0, sizeof(m_loopedSample));
    m_radarIcons = 0;
    m_advWindow = 0;
    m_routeArray = 0;
    m_curHeroMobile = 0;
    m_showMode = 0;
    g_completeDrawEnabled = 1;
    m_movingObjectIndex = -1;
    m_movingObjectSequence = -1;
    m_fullMap = g_game->getWorldMapData();
    m_cursorFrameCount = 0;
    m_cursorTurning = 0;
    m_netMsgHandler = 0;
}

// The adventure managers's resource-name tables, retail .data local to
// this TU. Declared like gLoopingSoundNames below: extern rows with the
// DATA claims, values left to the data phase (read from the verified
// image 2026-08-20: dirttl..rocktl, clrrvr/icyrvr/mudrvr/lavrvr,
// dirtrd/gravrd/cobbrd, ah00_..ah17_, af00..af07, ab01_..ab03_,
// abm01_..abm03_, abf01l..abf03k, and the 38-entry cached-graphics list
// diboxbck.pcx..HALLFORT.def whose entry 26 re-points at pskill.def).
DATA(0x0065f4c4) extern const char* const g_advCachedGraphicNames[38];
DATA(0x0065f55c) extern const char* const g_groundTilesetNames[10];
DATA(0x0065f588) extern const char* const g_riverTilesetNames[4];
DATA(0x0065f59c) extern const char* const g_roadTilesetNames[3];
DATA(0x0065f5a8) extern const char* const g_cursorIconNames[18];
DATA(0x0065f5f0) extern const char* const g_flagIconNames[8];
DATA(0x0065f610) extern const char* const g_boatFlagIconNames[3][8];
DATA(0x0065f670) extern const char* const g_boatIconNames[3];
DATA(0x0065f67c) extern const char* const g_boatFrothIconNames[3];

// E:\gamedcs\advmgr.cpp:837

// Retail calls vector<resource*>::insert at both push_back sites. Keep the
// site-level inline pins on those calls.

// Residual (97.80%): the tail of the same cascade, register/slot only.
// The adventure screen setup. Retail's grouping is preserved: the route
// array and the map window are allocated lazily with MemError guards,
// the cached-graphics list reverses each name and picks GetSprite for
// .def rows ("fed" after _strrev) with the push_back duplicated per arm,
// the tileset batches are separated by IncProgressBar ticks (index 19 of
// the cached list, index 9 of the cursor list get mid-batch ticks), the
// sound slots write touchedSounds = 0 INSIDE the four-step loop, and the
// hotseat (MP_HOTSEAT) turn banner runs between the two same-condition
// ifs - retail re-tests iMPNetProtocol rather than folding the arms.
VA(0x00406fd0, 0x7D6)  // anchor-vtable, dc 0x6b24
int AdvManager::open(int newPriority)
{
    m_bottomViewType = BOTTOM_VIEW_DEFAULT;
    m_heroLogoShowing = 0;
    g_completeDrawEnabled = 0;

    if (m_routeArray == 0) {
        m_routeArray = new unsigned short[(g_game->m_worldMap.getNumLevels())
                                        * g_mapHeight * g_mapWidth];
        memset(m_routeArray, 0,
               (g_game->m_worldMap.getNumLevels()) * g_mapHeight * g_mapWidth
                   * sizeof(unsigned short));
        if (m_routeArray == 0)
            memError();
    }

    m_showRoute = 0;
    m_fullySeeded = 0;
    m_seedingValid = 0;
    m_animCtrPaused = 0;

    if (m_advWindow == 0) {
        m_advWindow = new AdventureMapWindow();
        if (m_advWindow == 0)
            memError();
    }
    g_windowManager->addWindow(m_advWindow, 0, 1);
    if (g_game->m_worldMap.getNumLevels() < 2)
        m_advWindow->widgetSetStatus(4, 8);

    // The cache loop's counter is UNSIGNED: retail closes it with
    // `cmp esi,0x26 / jb` at 0x406fd0+0x28e where a signed `int` can only
    // emit `jl`. 97.7989 -> 97.9314 and the branch view goes clean 36/36.
    // MEASURED AND REJECTED at that plateau: hoisting one shared
    // `resource* graphic` above the if/else so both arms share retail's
    // [ebp-0x10] slot - 97.9057, the per-arm declarations are right.
    m_cachedGraphics.reserve(38);
    for (unsigned int cached = 0; cached < 38; cached++) {
        char reversed[16];
        strcpy(reversed, g_advCachedGraphicNames[cached]);
        _strrev(reversed);
        if (_strnicmp(reversed,
                      DATA_COMPGEN(0x00660328, defExtensionReversed, "fed"),
                      3) == 0) {
            Resource* graphic = ResourceManager::getSprite(g_advCachedGraphicNames[cached]);
#pragma inline_depth(0)
            m_cachedGraphics.push_back(graphic);
#pragma inline_depth()
        } else {
            Resource* graphic = ResourceManager::getBitmap816(g_advCachedGraphicNames[cached]);
#pragma inline_depth(0)
            m_cachedGraphics.push_back(graphic);
#pragma inline_depth()
        }
        if (cached == CACHED_GRAPHIC_TICK)
            incProgressBar(1);
    }
    incProgressBar(1);

    m_movingObjectSprite =
        ResourceManager::getSprite(DATA_COMPGEN(0x00660318, movingObjectSpriteName,
                               "avwattak.def"));
    for (int ground = 0; ground < 10; ground++)
        m_groundTileset[ground] = ResourceManager::getSprite(g_groundTilesetNames[ground]);
    incProgressBar(1);
    for (int river = 0; river < 4; river++)
        m_riverTileset[river + 1] = ResourceManager::getSprite(g_riverTilesetNames[river]);
    incProgressBar(1);
    for (int road = 0; road < 3; road++)
        m_roadTileset[road + 1] = ResourceManager::getSprite(g_roadTilesetNames[road]);
    incProgressBar(1);
    m_borderTileset =
        ResourceManager::getSprite(DATA_COMPGEN(0x00660310, borderTilesetName, "edg.def"));
    m_arrowTileset =
        ResourceManager::getSprite(DATA_COMPGEN(0x00660304, arrowTilesetName, "adag.def"));
    m_gemIcons[0] =
        ResourceManager::getSprite(DATA_COMPGEN(0x006602f8, gemIconName0, "agemul.def"));
    m_gemIcons[1] =
        ResourceManager::getSprite(DATA_COMPGEN(0x006602ec, gemIconName1, "agemur.def"));
    m_gemIcons[2] =
        ResourceManager::getSprite(DATA_COMPGEN(0x006602e0, gemIconName2, "agemll.def"));
    m_gemIcons[3] =
        ResourceManager::getSprite(DATA_COMPGEN(0x006602d4, gemIconName3, "agemlr.def"));
    m_starTileset =
        ResourceManager::getSprite(DATA_COMPGEN(0x006602c8, starTilesetName, "tshrc.def"));
    m_cloudIcons =
        ResourceManager::getSprite(DATA_COMPGEN(0x006602bc, cloudIconsName, "tshre.def"));
    incProgressBar(1);
    for (int cursor = 0; cursor < 18; cursor++) {
        m_cursorIcons[cursor] = ResourceManager::getSprite(g_cursorIconNames[cursor]);
        if (cursor == CURSOR_ICON_TICK)
            incProgressBar(1);
    }
    incProgressBar(1);
    for (int boat = 0; boat < 3; boat++) {
        m_boatIcons[boat] = ResourceManager::getSprite(g_boatIconNames[boat]);
        m_boatFrothIcons[boat] = ResourceManager::getSprite(g_boatFrothIconNames[boat]);
        for (int boatOwner = 0; boatOwner < 8; boatOwner++)
            m_boatFlagIcons[boat][boatOwner] =
                ResourceManager::getSprite(g_boatFlagIconNames[boat][boatOwner]);
    }
    incProgressBar(1);
    for (int owner = 0; owner < 8; owner++)
        m_flagIcons[owner] = ResourceManager::getSprite(g_flagIconNames[owner]);
    m_radarIcons =
        ResourceManager::getSprite(DATA_COMPGEN(0x006602b0, radarIconsName, "radar.def"));

    memset(m_loopedSample, 0, sizeof(m_loopedSample));
    for (int slot = 0; slot < ADVENTURE_ACTIVE_SOUND_COUNT; slot++) {
        m_soundArray[slot].m_soundId = LOOPING_SOUND_INVALID;
        m_soundArray[slot].m_priority = 0x7f;
        m_touchedSounds = 0;
    }
    for (int horse = 0; horse <= 10; horse++) {
        sprintf(g_text,
                DATA_COMPGEN(0x006602a0, heroSampleFormat, "horse%02d.wav"),
                horse);
        m_heroSamples[horse] = ResourceManager::getSample(g_text);
    }

    if (!g_currentPlayer->isLocalHuman()) {
        g_game->turnOnAIMusic();
        setNoDialogMenus(0);
    } else {
        setNoDialogMenus(1);
    }
    g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT] =
        GameTime::get() + ADVENTURE_ANIMATION_MAX_ELAPSED;
    g_completeDrawEnabled = g_currentPlayer->isLocalHuman();
    g_currentPlayer = &g_game->m_players[g_netLocalGamePos];
    showProgressBar();
    g_windowManager->fadeScreen(1, 4, 1);

    forceNewHover();
    if (!g_currentPlayer->isLocalHuman())
        g_game->showComputerScreen();

    if (g_mpNetProtocol == MP_HOTSEAT) {
        g_unnamed6993dc = 1;
        g_completeDrawEnabled = 1;
    }
    m_bottomViewType = BOTTOM_VIEW_DEFAULT;
    m_bottomViewOverride = BOTTOM_VIEW_DEFAULT;
    g_soundManager->adjustSoundVolumes();
    g_game->resetAllPlayerVisibility();
    setInitialMapOrigin();
    redrawAdvScreen(1, 0);

    if (g_game->isMultiplayer()) {
        g_dfltMenu = LoadMenuA(g_instance, MAKEINTRESOURCE(0x6e));
        g_gameMenu = LoadMenuA(g_instance, MAKEINTRESOURCE(0x70));
    } else if (g_unnamed698a34 != 0) {
        g_dfltMenu = LoadMenuA(g_instance, MAKEINTRESOURCE(0x6f));
        g_gameMenu = LoadMenuA(g_instance, MAKEINTRESOURCE(0x71));
    }
    kbChangeMenu(g_dfltMenu);

    if (g_mpNetProtocol == MP_HOTSEAT) {
        g_unnamed6993dc = 1;
        g_completeDrawEnabled = g_currentPlayer->isLocalHuman();
        char text[256];
        sprintf(text, g_generalText->getText(14), g_currentPlayer->getName());
        g_windowManager->m_isWaitingForFadeIn = 0;
        g_game->waitForPlayer(text, g_netLocalGamePos);
        redrawAdvScreen(1, 0);
        m_bottomViewOverride = BOTTOM_VIEW_1;
        m_bottomViewDeadline = GameTime::get() + 3000;
    }
    if (g_mpNetProtocol != MP_HOTSEAT)
        g_windowManager->fadeScreen(0, 4, 0);

    m_advWindow->updateResourceDisplay(1, 1);
    m_status = STATUS_ACTIVE;
    m_priority = newPriority;
    m_id = 0x400;
    strcpy(m_mgrName,
           DATA_COMPGEN(0x00660294, advManagerMgrName, "advManager"));
    g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);

    if (g_dPlay) {
        m_netMsgHandler = new CAdvMgrNetMsgHandler();
        g_dPlay->setNetMsgHandler(m_netMsgHandler);
    }
    if (g_currentPlayer->m_heroes[0] != -1)
        setHeroContext(g_currentPlayer->m_heroes[0], 0, 0, 1);
    return 0;
}

// The admitted retail vtable inventory bounds advManager at exactly three
// slots (0x63a678..0x63a683): Open, Close and Main, matching Dreamcast.
// The following five-slot table at 0x63a684 belongs instead to the now-
// modelled CAdvMgrNetMsgHandler. Its slot 0 is this deleting destructor;
// VC6's emitted ??_G public and all 33 retail bytes agree. The five-byte
// implicit destructor it calls is folded with another owner in retail, but
// VC6 still emits the class's named ??1 public; the direct-symbol compgen
// claim binds that existing body without inventing a source destructor.
VA_COMPGEN(0x004077b0, 0x21, SCALAR_DELETING_DTOR,
           CAdvMgrNetMsgHandler)
VA_COMPGEN(0x0057d160, 0x05, IMPLICIT_DTOR, CAdvMgrNetMsgHandler)

// E:\gamedcs\advmgr.cpp:1092
// The sprite teardown, gated on the town-nesting depth. Retail's own
// grouping is preserved statement for statement: the two river and road
// tilesets start at index 1 (slot 0 is the "no river"/"no road" entry that
// is never loaded), the ground tileset and the hero sample run share ONE
// counted loop - the bytes walk both off a single induction pointer at
// +0x360 with a -0x300 displacement - and the four gem icons are four
// statements rather than a loop, which is why their null stores sink past
// the following Dispose calls into one batch.

// Residual (86.37%): ONE over-inline, and the whole rest of the delta is
// its cascade. predict-inline reports exactly one divergence - retail
// emits an out-of-line call to the Dinkumware destroy-range helper that
// clear() -> erase(begin(), end()) reaches (`mov ecx,ebx; push [ebx+8];
// push edi; call`, the ICF-folded empty pointer-destroy body the delinked
// object labels sample_vslot03), where this compile elides it entirely.
// That call is what pins retail's EBX to `lea ebx,[esi+0xd0]`, which in
// turn evicts the element counter into [ebp-4], frees EDI to walk
// boatFlagIcons (so retail spells the movingObjectSprite null store as an
// immediate where ours reuses the EDI zero), and flips the EAX/EDX scratch
// parity for every `mov <scratch>,[ecx]; call [<scratch>+4]` pair after
// it. Our body is eight bytes shorter than retail's, which is that one
// call plus its argument setup. Same family as mainmenu's TMainMenu ctor
// and viewarmywindow's create_upgrade_widget; per docs/vc6/inliner.md the
// knob is caller body mass, not a vector spelling.

VA(0x004077e0, 0x2D1)  // anchor-vtable, dc 0x74ec
void AdvManager::close()
{
    m_advWindow->clearBottomView();
    m_bottomViewType = BOTTOM_VIEW_DEFAULT;
    if (g_unnamed6aa5f0 == 0) {
        g_soundManager->switchAmbientMusic(-1);
        g_soundManager->stopAllSamples(1);
    } else {
        g_soundManager->stopAllSamples(0);
    }

    if (g_unnamed699548 <= 0) {
        m_radarIcons->dispose();
        m_radarIcons = 0;
        m_cloudIcons->dispose();
        m_cloudIcons = 0;
        for (int cursor = 0; cursor < 18; cursor++) {
            m_cursorIcons[cursor]->dispose();
            m_cursorIcons[cursor] = 0;
        }
        for (unsigned int cached = 0; cached < m_cachedGraphics.size(); cached++)
            m_cachedGraphics[cached]->dispose();
        // MEASURED BYTE-FLAT, do not retry: `#pragma inline_depth(2)`
        // here, to keep clear()+erase() inline while forcing the
        // destroy-range helper out of line the way retail has it, changes
        // nothing (86.3656 either way). inline_depth only bites at 0 in
        // VC6 - the same result hero.cpp records for depths 1/2/3/4 - so
        // there is no way to spell "inline the parent, call the child".
        m_cachedGraphics.clear();
        m_movingObjectSprite->dispose();
        m_movingObjectSprite = 0;
        for (int boat = 0; boat < 3; boat++) {
            m_boatIcons[boat]->dispose();
            m_boatIcons[boat] = 0;
            m_boatFrothIcons[boat]->dispose();
            m_boatFrothIcons[boat] = 0;
            for (int boatOwner = 0; boatOwner < 8; boatOwner++) {
                m_boatFlagIcons[boat][boatOwner]->dispose();
                m_boatFlagIcons[boat][boatOwner] = 0;
            }
        }
        for (int owner = 0; owner < 8; owner++) {
            m_flagIcons[owner]->dispose();
            m_flagIcons[owner] = 0;
        }
    }

    for (int looping = 0; looping < LOOPING_SOUND_COUNT; looping++) {
        if (m_loopedSample[looping]) {
            m_loopedSample[looping]->dispose();
            m_loopedSample[looping] = 0;
        }
    }
    for (int river = 1; river < 5; river++) {
        m_riverTileset[river]->dispose();
        m_riverTileset[river] = 0;
    }
    for (int road = 1; road < 4; road++) {
        m_roadTileset[road]->dispose();
        m_roadTileset[road] = 0;
    }
    m_borderTileset->dispose();
    m_borderTileset = 0;
    m_arrowTileset->dispose();
    m_arrowTileset = 0;
    m_gemIcons[0]->dispose();
    m_gemIcons[1]->dispose();
    m_gemIcons[2]->dispose();
    m_gemIcons[3]->dispose();
    m_gemIcons[0] = 0;
    m_gemIcons[1] = 0;
    m_gemIcons[2] = 0;
    m_gemIcons[3] = 0;
    for (int ground = 0; ground < 10; ground++) {
        m_groundTileset[ground]->dispose();
        m_groundTileset[ground] = 0;
        m_heroSamples[ground]->dispose();
        m_heroSamples[ground] = 0;
    }

    g_windowManager->removeWindow(m_advWindow);
    delete m_advWindow;
    m_advWindow = 0;
    delete m_routeArray;
    m_routeArray = 0;
    m_status = 0;
    if (m_netMsgHandler) {
        delete m_netMsgHandler;
        m_netMsgHandler = 0;
    }
}

VA(0x00407ac0, 0x44)  // dc 0x793c
int AdvManager::inMapArea(int x, int y)
{
    const Widget* mapWidget = m_advWindow->m_mapWidget;
    return x >= mapWidget->m_x && x < mapWidget->m_y + mapWidget->m_width
        && y >= mapWidget->m_y && y < mapWidget->m_y + mapWidget->m_height;
}

#if 0  // @carcass

// E:\gamedcs\advmgr.cpp:1229
DC_ONLY(0x79b0, 0x52)
void AdvManager::GetCursorSampleSet(int walkSpeed)
{
    // @stub
}

// The ordinary getMouseMapPoint body follows this reference block.

#endif  // @carcass

// E:\gamedcs\advmgr.cpp:1245. This is the mouse-relative point, not
// get_map_center: DC 0x7a04 adds the mouse offsets, while header dc 0x1f000
// adds fixed viewport offsets. Retail reads +0xec/+0xf0 and keeps this
// ordinary body for the cross-TU spell/window callers; DoAdvCommand expands
// its four source calls. A header-inline spelling emitted no retained body.
VA(0x00407b10, 0x6F)  // field loads + four cross-TU call sites, dc 0x7a04
type_point AdvManager::getMouseMapPoint() const
{
    return type_point(m_radarOrigin.m_x + m_lastHoverX,
                      m_radarOrigin.m_y + m_lastHoverY,
                      m_radarOrigin.m_z);
}

// E:\gamedcs\advmgr.cpp:1253

// Seven declarators were added for this body, all gated to advmgr.obj's own
// view: advManager::MoveHero, advManager::DoEventShipyard,
// TAdventureMapWindow::SetSleepImage(int) (retail 0x403cc0 is `ret 4`, so
// the DC's zero-parameter spelling does not transfer), the EAdvCommand
// domain enum, and the four .bss cells 0x6968e0 / 0x69777c / 0x698774 /
// 0x699560. HeroView's existing gate was widened rather than duplicated.

// 77.1683 -> 77.3765 (2026-08-21): Dreamcast CodeView records a
// function-scope `town* newTown`, and retail likewise materialises the
// get_obscured_town result before the View call. Restoring that named
// temporary is the measured gain below. Moving the also-attested
// `bSaveShowRoute` and `msg` declarations to function scope is byte-flat
// with and without newTown, so their narrower existing scopes are kept.
// Release-elided TRACE carriers at 1/3/5/10 sites are byte-flat too: unlike
// the town constructors, this body's already-exact call ledger does not
// cross an optimizer threshold from those dormant call candidates.

// Residual (77.38%): why-branch reports 123 vs 122 blocks and 80 vs 80
// branches - structurally converged - with exactly TWO polarity flips left
// and no size or count divergence anywhere.
//   #15 (+0x211) is the BuildPath budget select: retail lays the 0xea5f
//     arm out as the fall-through, ours lays out movePoints. Three
//     spellings were measured and ALL THREE emit byte-identical code
//     (now 77.38 each), so VC6 normalises this block pair itself and the
//     order is not source-addressable here: the positive
//     `if (F1 || F2) movePoints else 0xea5f` kept below, its De Morgan
//     twin `if (!F1 && !F2) 0xea5f else movePoints`, and the same
//     condition as a ternary feeding one initialiser. A fourth spelling,
//     initialising to 0xea5f and conditionally assigning movePoints,
//     adds a branch and regresses 77.3765 -> 76.9612.
//     RE-MEASURED 2026-09-06 on the post-unpeel 94.2359 shape, because
//     every verdict above predates it: the ternary and the De Morgan twin
//     are still byte-flat at 94.2359, initialising to 0xea5f and
//     conditionally assigning movePoints is 93.9100 (and still flips), and
//     the untried INVERSE - initialise to movePoints, overwrite with
//     0xea5f under the negated guard - is the one spelling that takes the
//     branch census CLEAN at 80/80 and still LOSES, 93.7900. So the flip
//     is now known to be reachable and known to cost 0.45; the kept
//     positive form remains the scoring winner.
//     2026-09-06, polish lane 22 - and the 0.45 is NOT a second polarity
//     flip or a spelling: the inverse form buys the clean 80/80 census by
//     MERGING a block (120 against retail's 121, one missing) and the
//     skeleton collapses with it, 113 exact blocks down to 16 and 0
//     flow-kind mismatches up to 53. The two are in tension, not additive.
//     Retail's own layout at 0x407d97 confirms the POSITIVE source: every
//     `||` arm that succeeds jumps FORWARD to the movePoints block at
//     0x407d9e, the last test falls through into the 0xea5f block at
//     0x407d97, and that block jumps OVER movePoints - retail simply emits
//     the else-arm first. It is a block-placement choice VC6 makes for us;
//     do not spend another lane on this spelling.
//   The remaining lead is the FRAME, and it is now localised. Ours is 0x84
//   against retail's 0x80 and the shift is uniform: -0x4 / -0x8 / -0xc
//   agree, and from there down every slot of ours is retail's minus four
//   (0x14->0x10, 0x1a->0x16, 0x1c->0x18, 0x22->0x1e, 0x24->0x20,
//   0x44->0x40, 0x64->0x60, 0x84->0x80). So we allocate ONE dead dword at
//   [ebp-0x10] that no instruction in the body ever reads or writes. The
//   first real divergence after the prologue is the radarOrigin type_point
//   pack at advmgr.cpp:1528: we update the packed word IN PLACE with VC6's
//   xor idiom (`xor word ptr [ebp-0x1c], ax`) where retail builds the value
//   whole with and/shl/or. Whoever takes this next should look for the
//   type_point local whose lifetime forces that dead slot, not for another
//   branch spelling.
//   #46 (+0x46d) is the step loop's back edge: retail exits with `js`
//     and returns with an UNCONDITIONAL `jmp` because it keeps
//     gpSearchArray live in eax across the edge and reloads it at the
//     bottom; ours closes with a conditional `jns`. That is the
//     rematerialisation half of the register-homing family, not a loop
//     form - why-branch's own guided search moved all six D1/D2 loop
//     mutations and none changed the distance (two made it far worse).
//     Rechecking a hand-written top-tested `while (i >= 0)` after the
//     newTown gain adds two branches and regresses 77.3765 -> 77.2027.
// Dreamcast's local records put bBreak, bNoMove, bFoughtBattle and i in
// the same enclosing route block, in that order. Transplanting that scope
// to x86 is MEASURED AND REJECTED (2026-08-21): 77.3765 -> 77.2780, 82
// branches against retail's 80, with both polarity flips unchanged. The
// narrower per-iteration output-flag scope below is the retail-byte winner;
// DC local scopes are name/type evidence, not x86 allocation evidence.
// Earlier loop controls measured 88.5537 -> 90.2547 with an explicit
// decrement and unconditional back edge instead of a conventional for-test.
// The earlier goto-only model still peeled 29 instructions into the back
// edge; an enclosing while(1) removed that duplication (90.2547 -> 94.2359
// in that older implementation). These observations support the explicit
// decrement and loop scope, not a requirement for goto. The current audit
// below removes all remaining exits and forward-continue labels, retaining
// the initial empty-path guard and the same body decrement.
// Restoring both HideRoute sites plus the IsFlying/CanWalkOnWater mode
// helpers improves the current caller from 83.3621% to 84.9679%, without TU
// collateral in that checkpoint. Hero.h:645/654 owns the CanLand checks;
// the caller now passes checkTerrain=1 instead of flattening that branch.
// Goto audit: a positive initial guard and nested breaks remove seven route
// jumps without changing any TU score. DC 1349..1357 sets bBreak, exits the
// event loop, then tests it to exit the route loop; interrupted carries it.
// Rotating the decrement into a guarded do/while loses 3.3322 points, so the
// decrement stays in the body. Positive landing scopes remove the other two
// jumps, preserving the same bytes with either nested or compound guards.
VA(0x00407b80, 0xBF0)  // anchor-global, dc 0x7a8c
NewmapCell* AdvManager::doAdvCommand(type_point* triggerPoint)
{
    Town* newTown;
    NewmapCell* eventCell = 0;
    triggerPoint->m_x = -1;
    Hero* currHero = g_game->getHero(g_currentPlayer->m_currHeroId);

    switch (m_advCommand) {
    case ADV_COMMAND_MOVE_HERO:
        if (!currHero)
            break;
        currHero->m_pathTargetX = m_lastMapHover.m_x;
        currHero->m_pathTargetY = m_lastMapHover.m_y;
        currHero->m_pathTargetZ = m_lastMapHover.m_z;
        /* FALLS THROUGH into ADV_COMMAND_WALK_ROUTE - retail's own */

    case ADV_COMMAND_WALK_ROUTE: {
        if (!currHero)
            break;
        if (currHero->m_pathTargetX == -1)
            break;
        if (currHero->m_pathTargetY == -1)
            break;

        {
            type_point heroPoint(currHero->m_x, currHero->m_y, currHero->m_z);
            // The BY-VALUE copy is retail's, not decoration: it emits
            // `mov ecx,[ebp-0x20] / mov [ebp-0x20],ecx`, a dword self-store,
            // which is a struct copy whose source and destination the
            // allocator coalesced onto one slot. Together with the fullMap
            // load sitting BETWEEN the is_valid call and its test, that is
            // the signature of this file's own by-value DrawHeroCell helper
            // being expanded here.
            type_point cellPoint = heroPoint;
            unsigned char valid = cellPoint.isValid();
            NewfullMap* map = m_fullMap;
            NewmapCell* standingOn;
            if (!valid)
                standingOn = map->cell(0, 0, 0);
            else
                standingOn = map->cell(cellPoint.m_x, cellPoint.m_y,
                                       cellPoint.m_z);
            // The row past the nine terrain samples is the flight sample.
            Sample* walkSample = m_heroSamples[standingOn->m_groundSet];
            if (currHero->isFlying(0))
                walkSample = m_heroSamples[10];
            walkSample->m_memSample.m_memLooping = 0;
            g_unnamed6968e0 = g_soundManager->memorySample(walkSample);
        }

        {
            type_point target(currHero->m_pathTargetX, currHero->m_pathTargetY,
                              currHero->m_pathTargetZ);
            seedTo(target);
        }

        int moveBudget;
        if ((currHero->isFlying(0))
            || (currHero->canWalkOnWater(0)))
            moveBudget = currHero->m_movePoints;
        else
            moveBudget = 0xea5f;
        g_searchArray->buildPath(currHero, moveBudget);

        currHero->m_isSleeping = 0;
        m_advWindow->setSleepImage(0);

        if (static_cast<int>(g_searchArray->getPathSteps()) <= 0)
            break;

        int savedShowRoute = m_showRoute;
        mobilizeCurrHero(1, 0, 0);
        if (g_unnamed698774 || savedShowRoute) {
            showRoute(1, 0, 1);
        } else if (m_showRoute && m_advCommand != ADV_COMMAND_WALK_ROUTE) {
            hideRoute(1, 0, 1);
        }

        g_mouseManager->hidePointer();
        g_inputManager->flush();

        unsigned char interrupted = 0;
        int i = g_searchArray->getPathSteps() - 1;
        if (i >= 0) {
            while (1) {
                {
                    int noMove;
                    int foughtBattle;
                    eventCell = moveHero(g_searchArray->getStep(i),
                                         i == 0, *triggerPoint, &noMove, 0,
                                         &foughtBattle, 0);
                    m_advWindow->updateHeroLocator(-1, 1, 1);
                    if (eventCell)
                        break;
                    if (noMove)
                        break;
                    if (foughtBattle)
                        break;
                    if (g_unnamed69777c)
                        break;

                    if (!currHero->isFlying(1) && !currHero->canWalkOnWater(1)) {
                        process1WindowsMessage();
                        Message msg = g_inputManager->getEvent();
                        while (msg.m_id) {
                            if (msg.m_id == MESSAGE_KEY_DOWN
                                || msg.m_id == MESSAGE_LEFT_BUTTON_DOWN
                                || msg.m_id == MESSAGE_RIGHT_BUTTON_DOWN
                                || msg.m_id == MESSAGE_WIDGET) {
                                interrupted = 1;
                                stopCursor(1);
                                break;
                            }
                            process1WindowsMessage();
                            msg = g_inputManager->getEvent();
                        }
                        if (interrupted)
                            break;
                    }
                }
                if (--i < 0)
                    break;
            }
        }
        m_seedingValid = 0;
        if ((i <= 0 && currHero->m_x == currHero->m_pathTargetX
             && currHero->m_y == currHero->m_pathTargetY)
            || (interrupted && !g_unnamed698774) || eventCell) {
            hideRoute(0, 1, 1);
        } else if (m_advCommand == ADV_COMMAND_WALK_ROUTE || g_unnamed698774) {
            showRoute(0, 1, 1);
        }

        stopCursor(1);

        if (eventCell) {
            doEvent(eventCell, *triggerPoint);
            triggerPoint->m_x = -1;
            eventCell = 0;
            m_seedingValid = 0;
        }

        forceNewHover();
        g_mouseManager->showPointer(1);
        g_soundManager->switchAmbientMusic(g_terrainMusicIds[m_lastTerrain]);

        if (!g_currentPlayer->isLocalHuman())
            break;
        if (g_currentPlayer->m_currHeroId == -1)
            break;
        if (g_game->m_heroes[g_currentPlayer->m_currHeroId].isMobile())
            break;
        showRoute(1, 0, 0);
        g_advManager->m_advWindow->updateHeroLocators(-1, 1, 1);
        g_advManager->checkDimNextHeroBut();
        break;
    }

    case ADV_COMMAND_VIEW_OBSCURED_TOWN:
        demobilizeCurrHero(0, 1);
        g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);
        newTown = currHero->getObscuredTown();
        newTown->view(0);
        eventCell = 0;
        break;

    case ADV_COMMAND_VIEW_TOWN: {
        if (g_currentPlayer->isLocalHuman())
            demobilizeCurrHero(0, 1);
        g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);
        PlayerData* viewingPlayer = g_game->getLocalPlayer();
        if (!viewingPlayer)
            break;
        if (viewingPlayer->m_currTownId == -1)
            break;
        Town* viewedTown = g_game->getTown(viewingPlayer->m_currTownId);
        type_point townPoint(viewedTown->m_mapX, viewedTown->m_mapY,
                             viewedTown->m_mapZ);
        // The lookup's result is DISCARDED - retail makes the call and
        // never reads eax. Transcribed as retail wrote it, through the
        // by-value cell helper (see the WALK_ROUTE site for the self-store
        // that proves the copy).
        type_point cellPoint = townPoint;
        unsigned char valid = cellPoint.isValid();
        NewfullMap* map = m_fullMap;
        if (!valid)
            map->cell(0, 0, 0);
        else
            map->cell(cellPoint.m_x, cellPoint.m_y, cellPoint.m_z);
        viewedTown->view(0);
        eventCell = 0;
        break;
    }

    case ADV_COMMAND_VIEW_HERO: {
        PlayerData* viewingPlayer = g_game->getLocalPlayer();
        if (!viewingPlayer)
            break;
        if (viewingPlayer->m_currHeroId == -1)
            break;
        g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);
        if (g_unnamed699560) {
            type_point offMap(-1, -1, 0);
            setEnvironmentOrigin(offMap, 1);
        }
        trimLoopingSounds(0);
        heroView(viewingPlayer->m_currHeroId, 0, 0, 0);
        if (g_unnamed699560) {
            type_point centre = getMapCenter();
            setEnvironmentOrigin(centre, 1);
        }
        if (g_networkActive69954c && g_dPlay) {
            CNetMsgHandler* handler = g_dPlay->getNetMsgHandler();
            if (handler)
                handler->setInPopup(0);
        }
        redrawAdvScreen(1, 0);
        break;
    }

    case ADV_COMMAND_SELECT_HERO: {
        type_point mapPoint = getMouseMapPoint();
        type_point cellPoint = mapPoint;
        unsigned char valid = cellPoint.isValid();
        NewfullMap* map = m_fullMap;
        NewmapCell* heroCell;
        if (!valid)
            heroCell = map->cell(0, 0, 0);
        else
            heroCell = map->cell(cellPoint.m_x, cellPoint.m_y, cellPoint.m_z);
        setHeroContext(heroCell->m_extraInfo, 0,
                       !g_currentPlayer->isLocalHuman(), 1);
        break;
    }

    case ADV_COMMAND_SELECT_TOWN: {
        type_point mapPoint = getMouseMapPoint();
        type_point cellPoint = mapPoint;
        unsigned char valid = cellPoint.isValid();
        NewfullMap* map = m_fullMap;
        NewmapCell* townCell;
        if (!valid)
            townCell = map->cell(0, 0, 0);
        else
            townCell = map->cell(cellPoint.m_x, cellPoint.m_y, cellPoint.m_z);
        setTownContext(townCell->m_extraInfo,
                       !g_currentPlayer->isLocalHuman(), 1);
        break;
    }

    case ADV_COMMAND_SHIPYARD: {
        g_mouseManager->showPointer(0);
        g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);
        type_point mapPoint = getMouseMapPoint();
        type_point dockPoint = getMouseMapPoint();
        type_point cellPoint = mapPoint;
        unsigned char valid = cellPoint.isValid();
        NewfullMap* map = m_fullMap;
        NewmapCell* dockCell;
        if (!valid)
            dockCell = map->cell(0, 0, 0);
        else
            dockCell = map->cell(cellPoint.m_x, cellPoint.m_y, cellPoint.m_z);
        doEventShipyard(dockCell, dockPoint,
                        g_currentPlayer->isLocalHuman());
        updateRadar(m_radarOrigin, 1, 1, 0, 0, 0);
        completeDraw(m_radarOrigin.m_x, m_radarOrigin.m_y, m_radarOrigin.m_z, 0, 1);
        this->updateScreen(0, 0);
        g_soundManager->switchAmbientMusic(g_terrainMusicIds[m_lastTerrain]);
        break;
    }
    }

    m_advCommand = ADV_COMMAND_NONE;
    m_lastHoverX = m_lastHoverY = -1;
    return eventCell;
}

VA(0x004087b0, 0x487)  // dc 0x8644
int AdvManager::main(Message& msg)
{
    if (m_status == STATUS_SUSPENDED)
        return 0;

    if (g_gameOver) {
        msg.m_id = MESSAGE_EXECUTIVE;
        msg.m_codeX = EXECUTIVE_COMMAND_TERMINATE_LOOP;
        return MESSAGE_DISPATCH_FORWARD;
    }

    if (g_turnDuration69d630.isExpired()) {
        // Row 202 of the general text, the turn-timer expiry notice. No
        // surviving symbol names the row.
        normalDialogTimeOut(g_generalText->getText(202), 1, 15000, -1, -1,
                            -1, 0, -1, 0, -1, -1, 0);
        g_turnDuration69d630.clear();
        g_game->nextPlayer();
        return 0;
    }

    if (m_netMsgHandler)
        m_netMsgHandler->checkHandleNet(0, 0);

    if ((!g_currentPlayer->isHuman()
         || (g_unnamed691209 && g_netLocalGamePos == g_unnamed69120c))
        && (!g_videoPaused
            || g_game->isLastHuman(g_game->getLocalPlayerGamePos())
            || (g_unnamed691209 && g_netLocalGamePos == g_unnamed69120c))) {
        if (g_unnamed691209 && g_netLocalGamePos == g_unnamed69120c) {
            g_currentPlayer->m_isHuman = 0;
            g_currentPlayer->m_isLocal = 0;
        }
        g_unnamed69928c->startPlayerTurn(g_netLocalGamePos);
        if (g_unnamed691209 && g_netLocalGamePos == g_unnamed69120c) {
            g_currentPlayer->m_isHuman = 1;
            g_currentPlayer->m_isLocal = 1;
            if (!g_videoPaused)
                g_mapVisibilityBit = 0xff;
        }
        g_game->nextPlayer();
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (g_foregroundApp)
        checkScreenScroll();

    if (!g_noSound && g_unk698760) {
        unsigned long ambientStamp = g_unnamed699544;
        if (ambientStamp
            && static_cast<long>(GameTime::get() - ambientStamp) > 6000) {
            g_unnamed699544 = 0;
            g_soundManager->switchAmbientMusic(g_terrainMusicIds[m_lastTerrain]);

            type_point ambientCentre;
            int centreX = m_radarOrigin.m_x + 9;
            int centreY = m_radarOrigin.m_y + 8;
            int centreZ = m_radarOrigin.m_z;
            ambientCentre.m_x = centreX;
            ambientCentre.m_y = centreY;
            ambientCentre.m_z = centreZ;
            setEnvironmentOrigin(ambientCentre, 1);
        }
    }

    unsigned char exitFlag;
    int result;
    NewmapCell* eventCell;
    type_point triggerPoint;
    result = MESSAGE_DISPATCH_CONSUME;
    exitFlag = 0;
    eventCell = 0;

    if (msg.m_id != MESSAGE_NONE) {
        switch (msg.m_id) {
        case MESSAGE_KEY_DOWN:
            result = processKeyPress(&msg, &exitFlag, &triggerPoint,
                                     &eventCell);
            break;

        case MESSAGE_MOUSE_MOVE:
            result = processHover(msg.m_mouseX, msg.m_mouseY);
            break;

        case MESSAGE_WIDGET:
            switch (msg.m_codeX) {
            case Widget::WIDGET_SELECT:
                result = processSelect(&msg, &triggerPoint, &eventCell);
                break;

            case Widget::WIDGET_DESELECT:
                if (!(msg.m_qualifier & MESSAGE_MODIFIER_RIGHT))
                    result = processDeSelect(&msg, &exitFlag, &triggerPoint,
                                             &eventCell);
                break;

            case Widget::WIDGET_RIGHT_SELECT:
                if (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT) {
                    if (!m_advWindow->processRightSelect(&msg))
                        result = processSelect(&msg, &triggerPoint,
                                               &eventCell);
                }
                break;

            default:
                break;
            }
            break;

        default:
            break;
        }
    } else {
        unsigned long lastFrame =
            g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT];
        if (static_cast<long>(GameTime::get() - lastFrame) >= 0) {
            m_cursorFrameCount = 0;
            completeDraw(m_radarOrigin.m_x, m_radarOrigin.m_y, m_radarOrigin.m_z, 0, 1);
            g_windowManager->updateScreen(ADVENTURE_SCREEN_X,
                                          ADVENTURE_SCREEN_Y,
                                          ADVENTURE_SCREEN_WIDTH,
                                          ADVENTURE_SCREEN_HEIGHT);

            unsigned long curTime = GameTime::get();
            if (static_cast<long>(
                    curTime
                    - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT])
                    >= 0
                && !m_animCtrPaused) {
                ++m_animCtr;
                long elapsedTime =
                    curTime
                    - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT];
                g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT] +=
                    cppMax(elapsedTime,
                             static_cast<long>(
                                 ADVENTURE_ANIMATION_MAX_ELAPSED));
            }
            process1WindowsMessage();
        }
    }

    if (eventCell)
        doEvent(eventCell, triggerPoint);

    if (g_gameOver || exitFlag) {
        msg.m_id = MESSAGE_EXECUTIVE;
        msg.m_codeX = EXECUTIVE_COMMAND_TERMINATE_LOOP;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return result;
}

unsigned char saveGame(unsigned char campaignWinMode);

// The exit-command latch DoSystemOptions fills; owner TU unlocated, nearest
// consumer declares - and that is now ProcessKeyPress below, whose N, L,
// ESC and I arms each latch a different command into it, with
// ProcessDeSelect's adventure-options arm latching SYSOPT_QUIT further on.
// Dreamcast publishes gGameCommand; the Complete front-end and adventure
// option handlers independently read/write this same command latch.
DATA(0x006976d8) extern int g_gameCommand;

// E:\gamedcs\advmgr.cpp:1688
// The adventure map's keyboard dispatcher, and the largest switch in this
// compiland. Retail lowers it to a byte-index table at fn+0xb4c over
// `codeX - 1` in 0..0x50 and a nineteen-target dword table at fn+0xafc, and
// that index table IS the hotkey map. It decodes to PC scan codes:
//   0x39 SPACE, then the keypad ring 0x48/0x49/0x4d/0x51/0x50/0x4f/0x4b/
//   0x47, then 0x20 D, 0x19 P, 0x2f V, 0x31 N, 0x26 L, 0x01 ESC, 0x1f S,
//   0x17 I, 0x14 T, 0x1c ENTER.

// ARM ORDER IS SOURCE ORDER, and here that is a real lever rather than a
// coincidence: this is a JUMP TABLE, so the physical layout is the source
// layout - the exact opposite of advManager::Main's compare chain one
// function up, whose bodies are ordered by case VALUE and carry no
// source-order information at all. The order the table gives is the list
// above, and the eight keypad arms come out in the engine's own
// clockwise-from-north order: UP, PGUP, RIGHT, PGDN, DOWN, END, LEFT, HOME
// = gStepDelta directions 0..7.

// The eight keypad arms share ONE body, and it is a GOTO, not a tail
// merge: retail keeps the whole walk block inside the first arm (UP),
// immediately after that arm's `direction = 0`, and the other seven reach
// it with `mov ebx,<dir>; jmp` - so each arm's own ctrl-scroll early-out
// stays private ahead of the join.

// Four shapes are transcribed as retail has them:
//   * CheckDimNextHeroBut is EXPANDED here, where DoAdvCommand calls it -
//     retail loads gpAdvManager into ESI ahead of BOTH IsLocalHuman and
//     HasMobileHero, which no reload of a global across two calls can
//     produce. The pinned `auto_inline(off)` on that member is what
//     DoAdvCommand needs, so the body is written longhand here with the
//     manager hoisted into a local, which is the same code.
//   * The SPACE arm's cell lookup takes a COPY of the hero point
//     (`mov edx,[ebp-0xc] / mov [ebp-0xc],edx` - a copy VC6 coalesced
//     onto one slot), the same shape ProcessMapSelect's block-scoped
//     cellPoint carries.
//   * Both of that arm's points are DELIBERATELY left uninitialised: the
//     x field goes in with an XOR-merge against whatever the slot already
//     holds, which is only emitted when nothing zeroed it.
//   * The D arm keeps retail's own redundant pair - `currHeroId == -1`
//     threaded straight to the tail AND a null test on the resulting
//     pointer - which is what an inlined game::GetHero whose null arm got
//     branch-threaded leaves behind.

// Residual (97.61%, from 80.40; 2026-09-04): the keypad walk is NOT a
// goto-shared tail inside the KP_8 arm. The Dreamcast dossier names
// the local `iMoveDir` (sp+0x34) and its line table puts the eight
// `ScreenScroll` arms in a row (1711..1761), every other arm after
// them, and the walk body LAST at lines 1890..1928, at scope depth 1 -
// after the switch. So each keypad arm is `if (ctrl) { ScreenScroll(n,
// 0); return 1; } iMoveDir = n; break;` and the walk follows the switch
// behind `iMoveDir >= 0 && !waitingPlayer && currHeroId != -1 &&
// ValidMove(..)`, with its own late guards nested (retail jumps all
// three to the shared return-1 and lets the `WidgetSetStatus` arm sink
// into it). VC6's constant threading then does the rest by itself:
// every keypad `break` knows iMoveDir and lands straight on the
// ValidMove guard, the walk block is placed as KP_8's fall-through
// (retail's `xor ebx,ebx` at fn+0x257), the other seven arms sink
// after it with `mov ebx,n; jmp`, every non-keypad `break` (iMoveDir
// == -1) threads to the one shared return-1 at the end, and the seven
// explicit `return 1` copies give retail's 24 rets. All of the placement
// experiments recorded before (eight bodies, two bodies, hoisting the
// ctrl test, goto counts) were fighting this device with gotos.
// The same `<flag> set in arms, tested once after the switch` device
// closed townManager::Main's 41-vs-3 ret residual; look for a
// DC-named flag/direction local before touching any goto-shared tail.
// What is left (3 size-only blocks, 111 = 111 otherwise exact): retail
// pushes only esi ahead of the chat-focus early return and sinks the
// ebx/edi pushes past it (a 5-instruction `xor eax,eax` exit) where we
// push all three in the prologue, and the SPACE arm's type_point cell
// lookup keeps `fullMap` in a register slot where ours reloads it.
VA(0x00408c40, 0xB9D)  // anchor-callee, dc 0x8b70
int AdvManager::processKeyPress(const Message* msg, unsigned char* exitFlag, type_point* triggerPoint, NewmapCell** peventCell)
{
    if (m_advWindow->m_chatEdit->m_hasFocus)
        return 0;

    PlayerData* localPlayer = g_game->getLocalPlayer();
    unsigned char waitingPlayer = !g_currentPlayer->isLocalHuman();
    // NOT game::GetHero: that header inline tests `== -1` first, so VC6
    // emits its null arm ahead of the address computation. Retail has the
    // computation first and jumps AWAY for the null, which is the opposite
    // arm order and therefore the opposite source spelling.
    Hero* currHero;
    if (localPlayer->m_currHeroId != -1)
        currHero = &g_game->m_heroes[localPlayer->m_currHeroId];
    else
        currHero = 0;

    int moveDir = -1;
    Hero* walker;

    switch (msg->m_codeX) {
    case KEYCODE_SPACE: {
        if (!currHero)
            break;
        if (waitingPlayer)
            break;

        if (!m_curHeroMobile) {
            if (g_currentPlayer->isLocalHuman()
                || (g_unnamed6989c8 && g_unnamed69ccd4)) {
                g_windowManager->broadcastMessage(
                    MESSAGE_WIDGET, Widget::WIDGET_SET_STATUS,
                    AdventureMapWindow::MOVE_ID,
                    Widget::WIDGET_UPDATE | Widget::WIDGET_DIMMED);
                if (m_showRoute) {
                    m_showRoute = 0;
                    completeDraw(m_radarOrigin.m_x, m_radarOrigin.m_y, m_radarOrigin.m_z,
                                 0, 1);
                    this->updateScreen(0, 0);
                }
            }
            setHeroContext(localPlayer->m_currHeroId, 0, 0, 1);
        }

        type_point heroPoint(currHero->m_x, currHero->m_y, currHero->m_z);

        NewmapCell* standingOn;
        {
            type_point cellPoint = heroPoint;
            if (!cellPoint.isValid())
                standingOn = m_fullMap->cell(0, 0, 0);
            else
                standingOn = m_fullMap->cell(cellPoint.m_x, cellPoint.m_y,
                                           cellPoint.m_z);
        }
        if (!standingOn->m_isTrigger)
            break;
        if (standingOn->m_type == ANCHOR_POINT)
            break;

        type_point eventPoint(currHero->m_x, currHero->m_y, currHero->m_z);
        doEvent(standingOn, eventPoint);
        return 1;
    }

    case KEYCODE_KP_8:
        if (msg->m_qualifier & MESSAGE_MODIFIER_CONTROL_KEYS) {
            screenScroll(0, 0);
            return 1;
        }
        moveDir = 0;
        break;

    case KEYCODE_KP_9:
        if (msg->m_qualifier & MESSAGE_MODIFIER_CONTROL_KEYS) {
            screenScroll(1, 0);
            return 1;
        }
        moveDir = 1;
        break;

    case KEYCODE_KP_6:
        if (msg->m_qualifier & MESSAGE_MODIFIER_CONTROL_KEYS) {
            screenScroll(2, 0);
            return 1;
        }
        moveDir = 2;
        break;

    case KEYCODE_KP_3:
        if (msg->m_qualifier & MESSAGE_MODIFIER_CONTROL_KEYS) {
            screenScroll(3, 0);
            return 1;
        }
        moveDir = 3;
        break;

    case KEYCODE_KP_2:
        if (msg->m_qualifier & MESSAGE_MODIFIER_CONTROL_KEYS) {
            screenScroll(4, 0);
            return 1;
        }
        moveDir = 4;
        break;

    case KEYCODE_KP_1:
        if (msg->m_qualifier & MESSAGE_MODIFIER_CONTROL_KEYS) {
            screenScroll(5, 0);
            return 1;
        }
        moveDir = 5;
        break;

    case KEYCODE_KP_4:
        if (msg->m_qualifier & MESSAGE_MODIFIER_CONTROL_KEYS) {
            screenScroll(6, 0);
            return 1;
        }
        moveDir = 6;
        break;

    case KEYCODE_KP_7:
        if (msg->m_qualifier & MESSAGE_MODIFIER_CONTROL_KEYS) {
            screenScroll(7, 0);
            return 1;
        }
        moveDir = 7;
        break;

    case KEYCODE_D:
        if (waitingPlayer)
            break;
        if (!g_game->getHero(g_currentPlayer->m_currHeroId))
            break;
        processSearch(-1, -1, -1);
        return 1;

    case KEYCODE_P:
        viewPuzzle();
        return 1;

    case KEYCODE_V:
        viewWorld(0, eMasteryNone);
        return 1;

    case KEYCODE_N:
        if (g_game->isMultiplayer())
            break;
        // Rows 68/69/70 of the general text, the new-game, load-game and
        // exit confirms. No surviving symbol names any of the three.
        normalDialog(g_generalText->getText(68), 2, -1, -1, -1, 0, -1, 0,
                     -1, 0, -1, 0);
        if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT)
            break;
        *exitFlag = 1;
        g_gameCommand = SYSOPT_COMMAND_101;
        return 1;

    case KEYCODE_L:
        if (g_game->isMultiplayer())
            break;
        normalDialog(g_generalText->getText(69), 2, -1, -1, -1, 0, -1, 0,
                     -1, 0, -1, 0);
        if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT)
            break;
        *exitFlag = 1;
        g_gameCommand = SYSOPT_COMMAND_102;
        return 1;

    case KEYCODE_ESCAPE:
        videoPause();
        normalDialog(g_generalText->getText(70), 2, -1, -1, -1, 0, -1, 0,
                     -1, 0, -1, 0);
        videoResume();
        if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT)
            break;
        *exitFlag = 1;
        g_gameCommand = SYSOPT_COMMAND_108;
        return 1;

    case KEYCODE_S:
        saveGame(0);
        return 1;

    case KEYCODE_I:
        g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);
        g_game->showScenInfo();
        if (g_windowManager->m_dialogReturn != SYSOPT_COMMAND_111)
            break;
        normalDialog(g_generalText->getText(68), 2, -1, -1, -1, 0, -1, 0,
                     -1, 0, -1, 0);
        if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT)
            break;
        g_gameCommand = SYSOPT_QUIT;
        *exitFlag = 1;
        return 1;

    case KEYCODE_T: {
        int townId = localPlayer->nextTown();
        if (townId == -1)
            break;
        if (!waitingPlayer) {
            if (g_currentPlayer->isLocalHuman()
                || (g_unnamed6989c8 && g_unnamed69ccd4)) {
                g_windowManager->broadcastMessage(
                    MESSAGE_WIDGET, Widget::WIDGET_SET_STATUS,
                    AdventureMapWindow::MOVE_ID,
                    Widget::WIDGET_UPDATE | Widget::WIDGET_DIMMED);
                if (m_showRoute) {
                    m_showRoute = 0;
                    completeDraw(m_radarOrigin.m_x, m_radarOrigin.m_y, m_radarOrigin.m_z,
                                 0, 1);
                    this->updateScreen(0, 0);
                }
            }
        }
        setTownContext(townId, waitingPlayer, 1);
        return 1;
    }

    case KEYCODE_ENTER:
        if (localPlayer->m_currTownId != -1) {
            m_advCommand = ADV_COMMAND_VIEW_TOWN;
            doAdvCommand(triggerPoint);
            return 1;
        }
        if (localPlayer->m_currHeroId == -1)
            break;
        if (m_curHeroMobile) {
            m_advCommand = ADV_COMMAND_VIEW_HERO;
            doAdvCommand(triggerPoint);
            return 1;
        }
        if (!waitingPlayer) {
            if (g_currentPlayer->isLocalHuman()
                || (g_unnamed6989c8 && g_unnamed69ccd4)) {
                g_windowManager->broadcastMessage(
                    MESSAGE_WIDGET, Widget::WIDGET_SET_STATUS,
                    AdventureMapWindow::MOVE_ID,
                    Widget::WIDGET_UPDATE | Widget::WIDGET_DIMMED);
                if (m_showRoute) {
                    m_showRoute = 0;
                    completeDraw(m_radarOrigin.m_x, m_radarOrigin.m_y, m_radarOrigin.m_z,
                                 0, 1);
                    this->updateScreen(0, 0);
                }
            }
        }
        setHeroContext(localPlayer->m_currHeroId, 0, waitingPlayer, 1);
        return 1;

    default:
        break;
    }
    if (moveDir >= 0 && !waitingPlayer
        && g_currentPlayer->m_currHeroId != -1) {
        walker = &g_game->m_heroes[g_currentPlayer->m_currHeroId];
        if (validMove(walker, moveDir, 0, 1)) {
            if (g_currentPlayer->isLocalHuman()
                || (g_unnamed6989c8 && g_unnamed69ccd4)) {
                g_windowManager->broadcastMessage(
                    MESSAGE_WIDGET, Widget::WIDGET_SET_STATUS,
                    AdventureMapWindow::MOVE_ID,
                    Widget::WIDGET_UPDATE | Widget::WIDGET_DIMMED);
                int heroId = g_currentPlayer->m_currHeroId;
                if (heroId != -1) {
                    Hero* pathHero = &g_game->m_heroes[heroId];
                    pathHero->m_pathTargetX = -1;
                    pathHero->m_pathTargetY = -1;
                }
                if (m_showRoute) {
                    m_showRoute = 0;
                    completeDraw(m_radarOrigin.m_x, m_radarOrigin.m_y, m_radarOrigin.m_z,
                                 0, 1);
                    g_windowManager->updateScreen(ADVENTURE_SCREEN_X,
                                                  ADVENTURE_SCREEN_Y,
                                                  ADVENTURE_SCREEN_WIDTH,
                                                  ADVENTURE_SCREEN_HEIGHT);

                    unsigned long curTime = GameTime::get();
                    if (static_cast<long>(
                            curTime
                            - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT])
                            >= 0
                        && !m_animCtrPaused) {
                        ++m_animCtr;
                        long elapsedTime =
                            curTime
                            - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT];
                        g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT] +=
                            cppMax(elapsedTime,
                                     static_cast<long>(
                                         ADVENTURE_ANIMATION_MAX_ELAPSED));
                    }
                    process1WindowsMessage();
                }
            }

            g_mouseManager->hidePointer();
            walker->m_pathTargetX = walker->m_x + g_stepDeltaX[4 * moveDir];
            walker->m_pathTargetY = walker->m_y + g_stepDeltaY[4 * moveDir];
            walker->m_pathTargetZ = walker->m_z;

            {
            type_point walkTrigger;
            int noMove;
            int foughtBattle;
            *peventCell = moveHero(moveDir, 1, walkTrigger, &noMove, 0,
                                   &foughtBattle, 0);
            m_advWindow->updateHeroLocator(-1, 1, 1);
            g_mouseManager->showPointer(1);
            g_soundManager->switchAmbientMusic(g_terrainMusicIds[m_lastTerrain]);

            if (*peventCell) {
                stopCursor(1);
                doEvent(*peventCell, walkTrigger);
                *peventCell = 0;
            }
            m_seedingValid = 0;

            forceNewHover();
            updBottomView(1, 1, 1);

            if (g_currentPlayer->isLocalHuman()
                && g_currentPlayer->m_currHeroId != -1
                && !g_game->m_heroes[g_currentPlayer->m_currHeroId]
                        .isMobile()) {
                showRoute(1, 0, 0);
                g_advManager->m_advWindow->updateHeroLocators(-1, 1, 1);

                AdvManager* dimTarget = g_advManager;
                if (g_currentPlayer->isLocalHuman()
                    && g_currentPlayer->hasMobileHero())
                    dimTarget->m_advWindow->widgetClearStatus(
                        AdventureMapWindow::NEXT_HERO_ID,
                        Widget::WIDGET_UPDATE | Widget::WIDGET_DIMMED);
                else
                    dimTarget->m_advWindow->widgetSetStatus(
                        AdventureMapWindow::NEXT_HERO_ID,
                        Widget::WIDGET_UPDATE | Widget::WIDGET_DIMMED);
            }
            }
        }
    }
    return 1;
}

// The tail runs on EVERY path, including the ones that dispatched: a
// right-click (MESSAGE_MODIFIER_RIGHT in the qualifier) on any id in the
// help band pops the general-text row through kb's NormalDialog. The
// function always returns 1.

// Both hero paths take the acting player TWICE, exactly as SetTownContext
// four functions down does: once at the top for the waitingPlayer byte
// this passes on to SetHeroContext/SetTownContext, and once more inside
// the town arm's guard, uncached.

VA(0x004097e0, 0x290)  // dc 0x9330
int AdvManager::processSelect(const Message* msg, type_point* triggerPoint, NewmapCell** peventCell)
{
    PlayerData* localPlayer = g_game->getLocalPlayer();
    unsigned char waitingPlayer = !g_currentPlayer->isLocalHuman();

    switch (msg->m_codeY) {
    case AdventureMapWindow::HERO_0_ID:
    case AdventureMapWindow::HERO_1_ID:
    case AdventureMapWindow::HERO_2_ID:
    case AdventureMapWindow::HERO_3_ID:
    case AdventureMapWindow::HERO_4_ID:
    case AdventureMapWindow::HERO_LOCATOR_0_ID:
    case AdventureMapWindow::HERO_LOCATOR_1_ID:
    case AdventureMapWindow::HERO_LOCATOR_2_ID:
    case AdventureMapWindow::HERO_LOCATOR_3_ID:
    case AdventureMapWindow::HERO_LOCATOR_4_ID: {
        int heroSlot = msg->m_codeY - AdventureMapWindow::HERO_0_ID;
        if (heroSlot > AdventureMapWindow::NUM_HERO_BUTTONS - 1)
            heroSlot = msg->m_codeY - AdventureMapWindow::HERO_LOCATOR_0_ID;
        int heroId = localPlayer->m_heroes[m_advWindow->m_topHero + heroSlot];
        if (heroSlot < localPlayer->m_numHeroes) {
            if (heroId == localPlayer->m_currHeroId) {
                m_advCommand = ADV_COMMAND_VIEW_HERO;
                doAdvCommand(triggerPoint);
            } else {
                setHeroContext(heroId, 0, waitingPlayer, 1);
            }
        }
        break;
    }

    case AdventureMapWindow::TOWN_0_ID:
    case AdventureMapWindow::TOWN_1_ID:
    case AdventureMapWindow::TOWN_2_ID:
    case AdventureMapWindow::TOWN_3_ID:
    case AdventureMapWindow::TOWN_4_ID: {
        int townSlot = msg->m_codeY - AdventureMapWindow::TOWN_0_ID;
        int townId = localPlayer->m_townIds[m_advWindow->m_topTown + townSlot];
        if (!waitingPlayer) {
            if (g_currentPlayer->isLocalHuman()
                || (g_unnamed6989c8 && g_unnamed69ccd4)) {
                g_windowManager->broadcastMessage(
                    MESSAGE_WIDGET, Widget::WIDGET_SET_STATUS,
                    AdventureMapWindow::MOVE_ID,
                    Widget::WIDGET_UPDATE | Widget::WIDGET_DIMMED);
                if (m_showRoute) {
                    m_showRoute = 0;
                    completeDraw(m_radarOrigin.m_x, m_radarOrigin.m_y, m_radarOrigin.m_z,
                                 0, 1);
                    g_windowManager->updateScreen(ADVENTURE_SCREEN_X,
                                                  ADVENTURE_SCREEN_Y,
                                                  ADVENTURE_SCREEN_WIDTH,
                                                  ADVENTURE_SCREEN_HEIGHT);

                    unsigned long curTime = GameTime::get();
                    if (static_cast<long>(
                            curTime
                            - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT])
                            >= 0
                        && !m_animCtrPaused) {
                        ++m_animCtr;
                        long elapsedTime =
                            curTime
                            - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT];
                        g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT] +=
                            cppMax(elapsedTime,
                                     static_cast<long>(
                                         ADVENTURE_ANIMATION_MAX_ELAPSED));
                    }
                    process1WindowsMessage();
                }
            }
        }
        if (townId == localPlayer->m_currTownId) {
            m_advCommand = ADV_COMMAND_VIEW_TOWN;
            *peventCell = doAdvCommand(triggerPoint);
        } else {
            setTownContext(townId, waitingPlayer, 1);
        }
        break;
    }

    case AdventureMapWindow::MAP_ID:
    case AdventureMapWindow::CHAT_TEXT_ID:
        processMapSelect(msg, triggerPoint, peventCell);
        break;

    case AdventureMapWindow::RADAR_ID:
        processRadarSelect(msg);
        break;

    default:
        break;
    }

    if ((msg->m_qualifier & MESSAGE_MODIFIER_RIGHT)
        && msg->m_codeY >= ADV_HELP_ID_FIRST && msg->m_codeY <= ADV_HELP_ID_LAST) {
        // Row 110 of the general text, the only help string the whole
        // adventure-button band answers with. No surviving symbol names
        // the row, so the index stays a literal.
        normalDialog(g_generalText->getText(110), 4, -1, -1, -1, 0, -1, 0,
                     -1, 0, -1, 0);
    }
    return 1;
}

#if 0  // @carcass

// E:\gamedcs\advmgr.cpp:2031
DC_ONLY(0x9614, 0x47E)
void AdvManager::ProcessAdvMenu(Message* msg)
{
    // @stub
}

#endif  // @carcass

VA(0x00409a70, 0x641)  // dc 0x9a94
int AdvManager::processDeSelect(const Message* msg, unsigned char* exitFlag, type_point* triggerPoint, NewmapCell** peventCell)
{
    PlayerData* localPlayer = g_game->getLocalPlayer();
    unsigned char waitingPlayer = !g_currentPlayer->isLocalHuman();

    switch (msg->m_codeY) {
    case AdventureMapWindow::QUEST_LOG_ID:
        doQuestLog(g_game->getLocalPlayerGamePos());
        break;

    case AdventureMapWindow::HERO_UP_ID:
        m_advWindow->doHeroKnob(1);
        break;

    case AdventureMapWindow::HERO_DOWN_ID:
        m_advWindow->doHeroKnob(0);
        break;

    case AdventureMapWindow::TOWN_UP_ID:
        m_advWindow->doTownKnob(1);
        break;

    case AdventureMapWindow::TOWN_DOWN_ID:
        m_advWindow->doTownKnob(0);
        break;

    case AdventureMapWindow::SLEEP_ID: {
        Hero* sleeper = g_game->getHero(localPlayer->m_currHeroId);
        if (sleeper) {
            sleeper->m_isSleeping = !sleeper->m_isSleeping;
            if (sleeper->m_isSleeping) {
                if (!waitingPlayer) {
                    if (g_currentPlayer->isLocalHuman()
                        || (g_unnamed6989c8 && g_unnamed69ccd4)) {
                        g_windowManager->broadcastMessage(
                            MESSAGE_WIDGET, Widget::WIDGET_SET_STATUS,
                            AdventureMapWindow::MOVE_ID,
                            Widget::WIDGET_UPDATE | Widget::WIDGET_DIMMED);
                        if (m_showRoute) {
                            m_showRoute = 0;
                            completeDraw(m_radarOrigin.m_x, m_radarOrigin.m_y,
                                         m_radarOrigin.m_z, 0, 1);
                            this->updateScreen(0, 0);
                        }
                    }
                }
                setHeroContext(localPlayer->nextHero(), 0, waitingPlayer, 1);
                sleeper = g_game->getHero(localPlayer->m_currHeroId);
            }
            m_advWindow->setSleepImage(sleeper->m_isSleeping);
            if (g_currentPlayer->isLocalHuman()
                && g_currentPlayer->hasMobileHero())
                m_advWindow->widgetClearStatus(
                    AdventureMapWindow::NEXT_HERO_ID,
                    Widget::WIDGET_UPDATE | Widget::WIDGET_DIMMED);
            else
                m_advWindow->widgetSetStatus(
                    AdventureMapWindow::NEXT_HERO_ID,
                    Widget::WIDGET_UPDATE | Widget::WIDGET_DIMMED);
        }
        break;
    }

    case AdventureMapWindow::MOVE_ID:
        m_advCommand = ADV_COMMAND_WALK_ROUTE;
        *peventCell = doAdvCommand(triggerPoint);
        break;

    case AdventureMapWindow::ADVENTURE_OPTIONS_ID:
        doAdventureOptions();
        if (g_windowManager->m_dialogReturn == SYSOPT_COMMAND_111) {
            normalDialog(g_generalText->getText(68), 2, -1, -1, -1, 0, -1, 0,
                         -1, 0, -1, 0);
            if (g_windowManager->m_dialogReturn == DIALOG_RETURN_ACCEPT) {
                g_gameCommand = SYSOPT_QUIT;
                *exitFlag = 1;
            }
        }
        break;

    case AdventureMapWindow::SYSTEM_OPTIONS_ID:
        *exitFlag = doSystemOptions();
        break;

    case AdventureMapWindow::END_TURN_ID:
        if (!g_game->m_isTutorial && g_currentPlayer->hasMobileHero()
            && g_unnamed698778) {
            // Row 56 of the general text, the "you still have heroes who
            // can move" confirm. No surviving symbol names the row.
            normalDialog(g_generalText->getText(56), 2, -1, -1, -1, 0, -1, 0,
                         -1, 0, -1, 0);
            if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT)
                break;
        }
        g_game->nextPlayer();
        break;

    case AdventureMapWindow::NEXT_HERO_ID:
        if (!waitingPlayer) {
            if (g_currentPlayer->isLocalHuman()
                || (g_unnamed6989c8 && g_unnamed69ccd4)) {
                if (m_showRoute) {
                    m_showRoute = 0;
                    completeDraw(m_radarOrigin.m_x, m_radarOrigin.m_y, m_radarOrigin.m_z,
                                 0, 1);
                    this->updateScreen(0, 0);
                }
            }
        }
        setHeroContext(g_currentPlayer->nextHero(), 0, waitingPlayer, 1);
        break;

    case AdventureMapWindow::KINGDOM_OVERVIEW_ID: {
        if (g_unnamed699560) {
            type_point invalidOrigin(-1, -1, 0);
            setEnvironmentOrigin(invalidOrigin, 1);
        }
        trimLoopingSounds(0);
        g_game->overview();

        int fadeOut = 1;
        if (g_overviewReturnAction == OVERVIEW_EXIT_TOWN) {
            demobilizeCurrHero(0, 1);
            g_mouseManager->showPointer(1);
            g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);
            g_game->getTown(g_overviewReturnActionExtra)->view(1);
            fadeOut = 0;
        } else if (g_unnamed699560) {
            type_point viewCentre;
            int centreX = m_radarOrigin.m_x + 9;
            int centreY = m_radarOrigin.m_y + 8;
            int centreZ = m_radarOrigin.m_z;
            viewCentre.m_x = centreX;
            viewCentre.m_y = centreY;
            viewCentre.m_z = centreZ;
            setEnvironmentOrigin(viewCentre, 1);
        }
        redrawAdvScreen(1, 0);
        if (fadeOut)
            g_windowManager->fadeScreen(0, 4, 0);
        break;
    }

    case AdventureMapWindow::CAST_SPELL_ID:
        checkCastSpell();
        break;

    case AdventureMapWindow::ELEVATION_TOGGLE_ID:
        if (g_game->m_worldMap.getNumLevels() > 1) {
            demobilizeCurrHero(0, 1);
            m_radarOrigin.m_z = 1 - m_radarOrigin.m_z;
            m_advWindow->setElevationToggleImage(m_radarOrigin.m_z);
            redrawAdvScreen(1, 0);
        }
        break;

    default:
        break;
    }

    if (msg->m_codeY >= ADV_HELP_ID_FIRST && msg->m_codeY <= ADV_HELP_ID_LAST) {
        if (m_bottomViewOverride == BOTTOM_VIEW_2) {
            m_bottomViewOverride = BOTTOM_VIEW_1;
            m_bottomViewDeadline = GameTime::get() + 3000;
        } else if (m_bottomViewOverride != BOTTOM_VIEW_DEFAULT) {
            m_bottomViewOverride = BOTTOM_VIEW_DEFAULT;
        } else if (m_bottomViewType == BOTTOM_VIEW_2) {
            m_bottomViewOverride = BOTTOM_VIEW_1;
            m_bottomViewDeadline = GameTime::get() + 3000;
        } else {
            m_bottomViewOverride = BOTTOM_VIEW_2;
            m_bottomViewDeadline = GameTime::get() + 3000;
        }
        updBottomView(1, 1, 1);
    }
    return 1;
}

VA(0x0040a0c0, 0x50D)  // dc 0xa168
void AdvManager::processRadarSelect(const Message* msg)
{
    if (msg->m_qualifier & MESSAGE_MODIFIER_RIGHT) {
        normalDialog(g_generalText->getText(55), 4, -1, -1, -1, 0, -1, 0,
                     -1, 0, -1, 0);
        return;
    }

    demobilizeCurrHero(0, 1);

    float radarScale;
    switch (g_mapHeight) {
    case MAP_DIMENSION_SMALL:
        radarScale = 4.0f;
        break;
    case MAP_DIMENSION_MEDIUM:
        radarScale = 2.0f;
        break;
    case MAP_DIMENSION_LARGE:
        radarScale = 1.3333f;
        break;
    default:
        radarScale = 1.0f;
        break;
    }

    int mapY = static_cast<int>(
        (msg->m_mouseY - m_advWindow->m_radarWidget->m_y) / radarScale);
    int mapX = static_cast<int>(
        (msg->m_mouseX - m_advWindow->m_radarWidget->m_x) / radarScale);
    m_radarOrigin.m_x = mapX - 9;
    m_radarOrigin.m_y = mapY - 8;

    if (m_radarOrigin.m_x < -9)
        m_radarOrigin.m_x = -9;
    if (m_radarOrigin.m_y < -8)
        m_radarOrigin.m_y = -8;
    if (m_radarOrigin.m_x > g_mapWidth - 10)
        m_radarOrigin.m_x = g_mapWidth - 10;
    if (m_radarOrigin.m_y > g_mapHeight - 9)
        m_radarOrigin.m_y = g_mapHeight - 9;

    updateRadar(m_radarOrigin, 1, 1, 0, 0, 0);
    completeDraw(m_radarOrigin.m_x, m_radarOrigin.m_y, m_radarOrigin.m_z, 0, 1);
    g_windowManager->updateScreen(ADVENTURE_SCREEN_X, ADVENTURE_SCREEN_Y,
                                  ADVENTURE_SCREEN_WIDTH,
                                  ADVENTURE_SCREEN_HEIGHT);

    unsigned long curTime = GameTime::get();
    if (static_cast<long>(
            curTime - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT]) >= 0
        && !m_animCtrPaused) {
        ++m_animCtr;
        long elapsedTime =
            curTime - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT];
        g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT] +=
            cppMax(elapsedTime,
                     static_cast<long>(ADVENTURE_ANIMATION_MAX_ELAPSED));
    }
    process1WindowsMessage();

    Message dragMsg;
    Message event;
    do {
        process1WindowsMessage();
        event = g_inputManager->getEvent();
        dragMsg = event;
        while (event.m_id != MESSAGE_LEFT_BUTTON_UP) {
            if (!event.m_id)
                break;
            if (event.m_id == MESSAGE_MOUSE_MOVE)
                dragMsg = event;
            process1WindowsMessage();
            event = g_inputManager->getEvent();
        }

        if (dragMsg.m_id == MESSAGE_MOUSE_MOVE) {
            if (dragMsg.m_codeX < m_advWindow->m_radarWidget->m_x)
                dragMsg.m_codeX = m_advWindow->m_radarWidget->m_x;
            if (dragMsg.m_codeX >= m_advWindow->m_radarWidget->m_x
                                    + m_advWindow->m_radarWidget->m_width)
                dragMsg.m_codeX = m_advWindow->m_radarWidget->m_x
                                + m_advWindow->m_radarWidget->m_width - 1;
            if (dragMsg.m_codeY < m_advWindow->m_radarWidget->m_y)
                dragMsg.m_codeY = m_advWindow->m_radarWidget->m_y;
            if (dragMsg.m_codeY >= m_advWindow->m_radarWidget->m_y
                                    + m_advWindow->m_radarWidget->m_height)
                dragMsg.m_codeY = m_advWindow->m_radarWidget->m_y
                                + m_advWindow->m_radarWidget->m_width - 1;

            g_mouseManager->main(dragMsg);

            int dragY = static_cast<int>(
                (dragMsg.m_codeY - m_advWindow->m_radarWidget->m_y) / radarScale);
            int dragX = static_cast<int>(
                (dragMsg.m_codeX - m_advWindow->m_radarWidget->m_x) / radarScale);
            int newX = cppMin(cppMax(0, dragX), g_mapWidth - 1);
            int newY = cppMin(cppMax(0, dragY), g_mapWidth - 1);
            m_radarOrigin.m_x = newX - 9;
            m_radarOrigin.m_y = newY - 8;

            updateRadar(m_radarOrigin, 1, 1, 0, 0, 0);
            completeDraw(m_radarOrigin.m_x, m_radarOrigin.m_y, m_radarOrigin.m_z, 0, 1);
            g_windowManager->updateScreen(ADVENTURE_SCREEN_X,
                                          ADVENTURE_SCREEN_Y,
                                          ADVENTURE_SCREEN_WIDTH,
                                          ADVENTURE_SCREEN_HEIGHT);

            unsigned long dragTime = GameTime::get();
            if (static_cast<long>(
                    dragTime
                    - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT]) >= 0
                && !m_animCtrPaused) {
                ++m_animCtr;
                long dragElapsed =
                    dragTime
                    - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT];
                g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT] +=
                    cppMax(dragElapsed,
                             static_cast<long>(
                                 ADVENTURE_ANIMATION_MAX_ELAPSED));
            }
            process1WindowsMessage();
            dragMsg.m_id = 0;
        }
    } while (event.m_id != MESSAGE_LEFT_BUTTON_UP);
}

// E:\gamedcs\advmgr.cpp:2434
// The map widget's own click handler, split down the middle by the right
// mouse button: a right click is a quick view of whatever the cursor is
// over, a left click either retargets the current hero's path or selects
// the object under the pointer.

// Both halves lean on accessors that retail CALLS rather than expands -
// type_point::is_valid and advManager::GetCell are out-of-line members, so
// the is_valid/cell pair is spelled longhand here exactly as DoAdvCommand's
// arms already spell it eight hundred lines up. searchArray::get_cell IS a
// header inline and does expand, flying = 0 folding `(z*2+0)*MAP_HEIGHT`
// into the single `lea edi,[eax+2*edi]`.

// The LEFT-click object dispatch below is an IF-CHAIN and not a switch, and
// that is a byte fact rather than a taste call: retail compares HERO(34),
// TOWN(98), SHIPYARD(87) in SOURCE order, where a switch makes VC6 sort the
// three compares ascending and relocate the whole hero arm past the town
// arm. 77.76 -> 90.90 on that one edit.

// Residual (90.90%): three shapes, all measured, none closed.
//   * Retail reads the MEMBER lastMapHover for every comparison in the
//     left-click half (`mov ax,[esi+0xea]`) where we read the local copy
//     (`mov eax,[ebp-0xa]`). Spelling those four sites on lastMapHover
//     matches those instructions and CUTS the raw instruction-level
//     disagreement from 166 rows to 126 - but costs 2.5 points of fuzzy
//     (90.90 -> 88.33), because it perturbs the allocation around them.
//     Measured both ways with and without the currHeroId guard below:
//     member/guard 87.94, member/no-guard 88.33, local/no-guard 90.89,
//     local/guard 90.90. Kept the highest; the delta is real and the
//     spelling above is NOT retail's. SetHeroContext's frame-shrinking
//     lever does NOT rescue it either: block-scoping hoverPoint/cellPoint
//     forces the left half onto the member (the local is out of scope) and
//     lands at 87.95, so the two findings are independent.
//   * We spill `cell` to [ebp-8] where retail keeps it in EDI for the whole
//     body, so our frame is 0x1c against retail's 0x14 and localPlayer and
//     heroMobile fail to share retail's one [ebp-4] slot.
//   * Retail keeps its ONE `advCommand = VIEW_HERO; DoAdvCommand` block at
//     the FIRST of the two sites and jumps back into it from the hero arm;
//     our CL emits both source sites. An explicit backward `goto` with the
//     DC-roster locals hoisted to make the label legal does recover retail's
//     25-call/12-return census, but leaves 37 branches/60 blocks against
//     retail's 38/64 and regresses 91.10057 -> 88.463%. The source-directed
//     merge is therefore not the missing shape under VC6 SP3. Putting the
//     label at the SECOND site canonicalizes to the same 88.463% object, so
//     block polarity is bounded too. Hoisting only the three DC-roster locals
//     while retaining both bodies is byte-flat at 91.10057%, so lifetime alone
//     is not the remaining lever either.
// The redundant `currHeroId != -1` guard is retail's own: its inlined
// GetHero re-tests the id off the same flags and leaves a dead
// `xor ebx,ebx` arm behind. Dropping the guard reproduces that dead block
// but does not pay (see the four measurements above).
VA(0x0040a5d0, 0x606)  // anchor-callee, dc 0xa88c
void AdvManager::processMapSelect(const Message* msg, type_point* triggerPoint, NewmapCell** peventCell)
{
    int visibilityBit = 1 << g_game->getLocalPlayerGamePos();
    int localPlayer = g_game->getLocalPlayerGamePos();

    type_point hoverPoint = m_lastMapHover;
    if (!hoverPoint.isValid())
        return;

    m_lastHoverX = m_lastMapHover.m_x - m_radarOrigin.m_x;
    m_lastHoverY = m_lastMapHover.m_y - m_radarOrigin.m_y;

    unsigned char visible =
        (getMapExtra(hoverPoint.m_x, hoverPoint.m_y, hoverPoint.m_z)
         & visibilityBit) != 0;

    // The cell lookup's point is copied from the MEMBER, not from
    // hoverPoint, and it is block-scoped: taking it off hoverPoint keeps
    // that local alive across the whole body and costs the frame a slot.
    NewmapCell* cell;
    {
        type_point cellPoint = m_lastMapHover;
        if (!cellPoint.isValid())
            cell = m_fullMap->cell(0, 0, 0);
        else
            // Use the recovered cell helper; retail chooses its expansion.
            cell = m_fullMap->cell(cellPoint.m_x, cellPoint.m_y, cellPoint.m_z);
    }

    if (msg->m_qualifier & MESSAGE_MODIFIER_RIGHT) {
        if (!visible) {
            quickInfo(m_lastHoverX, m_lastHoverY, m_lastMapHover.m_z);
            return;
        }

        AdventureObjectType objType;
        int objIndex;
        if (g_currentPlayer == &g_game->m_players[localPlayer]
            && m_lastHoverX == HERO_VIEW_TILE_X
            && m_lastHoverY == HERO_VIEW_TILE_Y
            && g_currentPlayer->m_currHeroId != -1 && m_curHeroMobile) {
            objIndex = g_currentPlayer->m_currHeroId;
            objType = HERO;
        } else {
            objType = cell->m_type;
            objIndex = cell->m_extraInfo;
        }

        switch (objType) {
        case HERO:
            heroQuickView(objIndex, m_lastHoverX * 32, m_lastHoverY * 32, 1);
            return;
        case GARRISON:
            garrisonQuickView(objIndex, m_lastHoverX * 32, m_lastHoverY * 32);
            return;
        case TOWN:
            townQuickView(objIndex, m_lastHoverX * 32, m_lastHoverY * 32, 1);
            return;
        case MONSTER:
            monsterQuickView(cell, m_lastHoverX, m_lastHoverY);
            return;
        default:
            quickInfo(m_lastHoverX, m_lastHoverY, m_lastMapHover.m_z);
            return;
        }
    }

    if (!visible)
        return;

    int currHeroId = g_game->getLocalPlayer()->m_currHeroId;
    if (currHeroId != -1) {
        Hero* currHero = g_game->getHero(currHeroId);
        int heroMobile = currHero->isMobile();
        if (currHero && currHero->m_z == hoverPoint.m_z) {
            if (currHero->m_x == hoverPoint.m_x && currHero->m_y == hoverPoint.m_y) {
                m_advCommand = ADV_COMMAND_VIEW_HERO;
                doAdvCommand(triggerPoint);
                return;
            }

            PathCell* pathAt = g_searchArray->getCell(m_lastMapHover, 0);
            if (g_currentPlayer->isLocalHuman() && pathAt && pathAt->m_visited) {
                if (!heroMobile
                    || (msg->m_qualifier & MESSAGE_MODIFIER_CONTROL_KEYS)
                    || (g_unnamed698774
                        && (currHero->m_pathTargetX != hoverPoint.m_x
                            || currHero->m_pathTargetY != hoverPoint.m_y))) {
                    currHero->m_pathTargetX = hoverPoint.m_x;
                    currHero->m_pathTargetY = hoverPoint.m_y;
                    currHero->m_pathTargetZ = m_lastMapHover.m_z;
                    showRoute(1, 1, 1);
                    return;
                }
            }
            *peventCell = doAdvCommand(triggerPoint);
            return;
        }
    }

    int myPos = g_game->getLocalPlayerGamePos();
    AdventureObjectType clickedType = cell->m_type;
    int clickedIndex = cell->m_extraInfo;

    if (clickedType == HERO) {
        if (clickedIndex == g_game->getLocalPlayer()->m_currHeroId) {
            m_advCommand = ADV_COMMAND_VIEW_HERO;
            doAdvCommand(triggerPoint);
            return;
        }
        if (myPos != g_game->getHero(clickedIndex)->m_owner)
            return;
        // Retail homes this bool at [ebp+0xc] and pushes SetHeroContext's
        // trailing 1 AFTER the IsLocalHuman call, but naming it is a loss
        // in both widths: `unsigned char waitingPlayer` 89.99 and
        // `int waitingPlayer` 90.55 against 91.10 for the folded call.
        setHeroContext(clickedIndex, 0, !g_currentPlayer->isLocalHuman(), 1);
        return;
    }

    if (clickedType == TOWN) {
        if (clickedIndex == g_game->getLocalPlayer()->m_currTownId) {
            m_advCommand = ADV_COMMAND_VIEW_TOWN;
            *peventCell = doAdvCommand(triggerPoint);
            return;
        }
        if (clickedIndex == -1)
            return;
        Town* clickedTown = &g_game->m_towns[clickedIndex];
        unsigned char waitingPlayer = !g_currentPlayer->isLocalHuman();
        if (g_game->onSameTeam(g_unnamed69778c, clickedTown->m_owner)
            || m_debugViewAll)
            setTownContext(clickedIndex, waitingPlayer, 1);
        return;
    }

    if (clickedType == SHIPYARD)
        *peventCell = doAdvCommand(triggerPoint);
}

#if 0  // @carcass

// E:\gamedcs\advmgr.cpp:2624
DC_ONLY(0xaf3c, 0x2CA)
void AdvManager::ProcessMapSelect2(const Message* msg, type_point* trigger_point, NewmapCell** peventCell)
{
    // @stub
}

// E:\gamedcs\advmgr.cpp:2708
DC_ONLY(0xb208, 0x50)
void setTownHelp(char* buffer, const NewmapCell* cell)
{
    // @stub
}

// E:\gamedcs\advmgr.cpp:2721
DC_ONLY(0xb258, 0x60)
void setHeroHelp(char* buffer, const NewmapCell* cell)
{
    // @stub
}

// E:\gamedcs\advmgr.cpp:2736
DC_ONLY(0xb2b8, 0x104)
void setPyramidHelp(char* buffer, const NewmapCell* cell, const Hero* current_hero, const char* separator)
{
    // @stub
}

// E:\gamedcs\advmgr.cpp:2919
DC_ONLY(0xbaac, 0x96)
void setWagonHelpText(char* buffer, NewmapCell* cell, const char* separator)
{
    // @stub
}

// E:\gamedcs\advmgr.cpp:2941
DC_ONLY(0xbb44, 0xBC)
void setTombHelpText(char* buffer, NewmapCell* cell, const char* separator)
{
    // @stub
}

// E:\gamedcs\advmgr.cpp:2959
DC_ONLY(0xbc00, 0xAC)
void setWaterWheelHelpText(char* buffer, NewmapCell* cell, const char* separator)
{
    // @stub
}

// E:\gamedcs\advmgr.cpp:2981
DC_ONLY(0xbcac, 0xD8)
void setWindmillHelpText(char* buffer, NewmapCell* cell, const char* separator)
{
    // @stub
}

#endif  // @carcass

static void setTownHelp(char* buffer, const NewmapCell* cell)
{
    const Town* mapTown = g_game->getTown(cell->m_extraInfo);
    const char* townTypeName = g_unnamed6a74f4[cell->m_objectIndex];
    const char* townName = mapTown->m_name.begin();
    if (!townName)
        townName = DATA_COMPGEN(0x0063a608, townRolloverEmptyText, "");
    sprintf(buffer, DATA_COMPGEN(
        0x0065f3d4, rolloverTownFormat, "%s, %s"),
        townName, townTypeName);
}

static void setHeroHelp(char* buffer, const NewmapCell* cell)
{
    Hero* mapHero = g_game->getHero(cell->m_extraInfo);
    sprintf(buffer,
            g_generalText->getText(GENERAL_TEXT_HERO_ROLLOVER_FORMAT),
            mapHero->m_name, mapHero->heroFn004D8F70());
}

static void setPyramidHelp(
    char* buffer, const NewmapCell* cell, const Hero* currentHero,
    const char* separator)
{
    strcpy(buffer, g_adventureObjectNames[PYRAMID]);
    if (cell->m_isTrigger && currentHero) {
        strcat(buffer, separator);
        strcat(buffer,
               cell->playerKnowsCell(currentHero->m_owner)
                   ? g_generalText->getText(GENERAL_TEXT_VISITED_OBJECT)
                   : g_generalText->getText(GENERAL_TEXT_UNVISITED_OBJECT));
    }
}

static void setWagonHelpText(
    char* buffer, NewmapCell* cell, const char* separator)
{
    strcpy(buffer, g_adventureObjectNames[WAGON]);
    if (cell->m_isTrigger) {
        strcat(buffer, separator);
        strcat(buffer,
               cell->playerKnowsCell(g_netLocalGamePos)
                   ? g_generalText->getText(GENERAL_TEXT_VISITED_OBJECT)
                   : g_generalText->getText(GENERAL_TEXT_UNVISITED_OBJECT));
    }
}

static void setTombHelpText(
    char* buffer, NewmapCell* cell, const char* separator)
{
    strcpy(buffer, g_adventureObjectNames[WARRIOR_TOMB]);
    if (cell->m_isTrigger) {
        strcat(buffer, separator);
        strcat(buffer,
               cell->playerKnowsCell(g_netLocalGamePos)
                   ? g_generalText->getText(GENERAL_TEXT_VISITED_OBJECT)
                   : g_generalText->getText(GENERAL_TEXT_UNVISITED_OBJECT));
    }
}

// The ternary is retail's own shape at BOTH callers: SetRolloverText's arm
// (+0x1ca4, 0xb6 B) and QuickInfo's (+0x1f73, the same 0xb6) each hoist
// `gpGeneralText->m_texts` OUT of the branch - `mov eax,[gpGeneralText] /
// shl edx,2 / mov eax,[eax+0x20] / test dx,dx` - which only a common
// subexpression across the two GetText arms produces.  The if/else spelling
// duplicates the load into each arm: measured 2026-09-06 at WATER_WHEEL +0x44
// against retail (SetRolloverText 96.0784, QuickInfo 95.0826) where the
// ternary lands at +7 (96.8953 / 95.8819).
static void setWaterWheelHelpText(
    char* buffer, NewmapCell* cell, const char* separator)
{
    strcpy(buffer, g_adventureObjectNames[WATER_WHEEL]);
    if (cell->m_isTrigger && cell->playerKnowsCell(g_netLocalGamePos)) {
        strcat(buffer, separator);
        short gold = (cell->m_extraInfo & 0x1f) * 500;
        strcat(buffer,
               gold == 0
                   ? g_generalText->getText(GENERAL_TEXT_VISITED_OBJECT)
                   : g_generalText->getText(GENERAL_TEXT_UNVISITED_OBJECT));
    }
}

// Retail's WINDMILL keeps TWO complete strcat expansions (+0x1e8a, 0xf4 B),
// each loading gpGeneralText itself and into a DIFFERENT register (ecx then
// eax), which reads as if/else rather than a ternary.  It is not: spelling
// this arm if/else while WATER_WHEEL above stays a ternary costs three points
// on each caller - SetRolloverText 96.8953 -> 93.8242, QuickInfo 95.8819 ->
// 92.7502 (2026-09-06) - because the WAGON/WARRIOR_TOMB/WATER_WHEEL arms all
// cross-jump INTO this arm's two blocks and the if/else form moves their entry
// points.  The residual -30 B here is that merge depth, not the branch shape.
static void setWindmillHelpText(
    char* buffer, NewmapCell* cell, const char* separator)
{
    strcpy(buffer, g_adventureObjectNames[WINDMILL]);
    if (cell->m_isTrigger && cell->playerKnowsCell(g_netLocalGamePos)) {
        strcat(buffer, separator);
        unsigned long amount = cell->m_extraInfo >> 13;
        strcat(buffer,
               (amount & 0xf) == 0
                   ? g_generalText->getText(GENERAL_TEXT_VISITED_OBJECT)
                   : g_generalText->getText(GENERAL_TEXT_UNVISITED_OBJECT));
    }
}

// DC advmgr.cpp:3054..3058 gives this ordinary constructor and its three
// zero stores in member order. Retail's SetRolloverText/QuickInfo expand
// those stores before getTriggerCell; retain the constructor boundary
// instead of aggregate-initializing its implementation in both callers.
DC_ONLY(0xbeac, 0x1A)
CellAdjuster::CellAdjuster()
{
    m_obscuringHero = 0;
    m_obscuringBoat = 0;
    m_mobileHero = 0;
}

// RETAIL-RECONSTRUCTED 2026-08-09. The body consolidates repeated creature
// slots before choosing either a detailed seven-slot list or one approximate
// size/name pair. The detailed form stops at the first empty consolidated
// slot. Its caller set is the creature-bank help group and QuickInfo; no
// Dreamcast standalone copy survives, so the name is role-derived.

// Both creature-name sites go through the TU's own GetArmyName helper and
// pass the REAL count - numTroops[i] in the detailed list, the consolidated
// total in the approximate form - so a stack of one takes the singular name.
// Writing the bounds guard longhand against m_plural_name instead cost 28
// points (64.73 against 93.21): it duplicated a guard retail shares and
// dropped the count test retail folds, which is what put our body at 57
// blocks / 32 branches against retail's 42 / 23.

// THE PREFIX ASSIGNS, IT DOES NOT APPEND (byte-flat, 2026-09-06, reloc
// census). Retail's call at fn+0x8a is basic_string::assign(const char*,
// size_type) where ours was append(const char*, size_type); the inlined
// strlen ahead of it and both pushes are identical, so only the relocation
// target differed. `result` is empty there, so the two are behaviourally the
// same; with the assign spelling this row's call sequence AGREES 23/23.
VA(0x0040abe0, 0x37D)
std::string getArmyHelpText(const ArmyGroup* source,
                              unsigned char showFullList)
{
    ArmyGroup consolidatedArmy;
    consolidatedArmy.initialize();
    int i;
    for (i = 0; i < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++i) {
        if (source->m_armies[i] != CREATURE_NONE)
            consolidatedArmy.add(source->m_armies[i], source->m_numTroops[i], -1);
    }

    std::string result;
    result = g_generalText->getText(GENERAL_TEXT_ARMY_HELP_PREFIX);
    result.append(" ");
    if (showFullList) {
        for (i = 0; i < ArmyGroup::ARMY_GROUP_SLOT_COUNT; ++i) {
            if (consolidatedArmy.m_armies[i] == CREATURE_NONE)
                break;
            if (i > 0) {
                result += ", ";
                result += g_generalText->getText(
                    GENERAL_TEXT_ARMY_ENTRY_SEPARATOR);
            }
            result += ArmyGroup::getArmySizeName(
                consolidatedArmy.m_numTroops[i], 2);
            result += " ";
            result += getArmyName(consolidatedArmy.m_armies[i],
                                  consolidatedArmy.m_numTroops[i]);
        }
    } else {
        int amount = consolidatedArmy.getCreatureTotal();
        const char* armyName;
        if (consolidatedArmy.m_armies[1] == CREATURE_NONE) {
            armyName = getArmyName(consolidatedArmy.m_armies[0], amount);
        } else {
            armyName = g_generalText->getText(GENERAL_TEXT_MIXED_ARMY);
        }
        result.append(ArmyGroup::getArmySizeName(amount, 2));
        result.append(" ");
        result.append(armyName);
    }
    return result;
}

VA(0x0040af60, 0x4A)  // dc 0xbec8
CellAdjuster::~CellAdjuster()
{
    restoreCell();
}

VA(0x0040afb0, 0x12F)  // dc 0xbf1c
NewmapCell* CellAdjuster::getTriggerCell(NewmapCell* mapCell, int x, int y)
{
    restoreCell();

    if (x == MOBILE_HERO_CELL_X && y == MOBILE_HERO_CELL_Y
            && g_currentPlayer->m_currHeroId != -1) {
        m_mobileHero = &g_game->m_heroes[g_currentPlayer->m_currHeroId];
        if (m_mobileHero && !m_mobileHero->isOnMap()) {
            m_mobileHero->obscureCell();
            return mapCell;
        }
        m_mobileHero = 0;
    }

    NewmapCell* triggerCell = mapCell->getTriggerCell();
    if (!triggerCell || triggerCell == mapCell)
        return mapCell;

    if (triggerCell->m_type != mapCell->m_type) {
        if (triggerCell->m_type == HERO) {
            m_obscuringHero = g_game->getHero(triggerCell->m_extraInfo);
            m_obscuringHero->restoreCell();
        } else if (triggerCell->m_type == BOAT) {
            m_obscuringBoat = &g_game->m_boats[triggerCell->m_extraInfo];
            m_obscuringBoat->restoreCell();
        }
    }
    return triggerCell;
}

// DC advmgr.cpp:3111..3127 proves the ordinary helper and its three
// conditional restores. Retail expands it in the destructor, getTriggerCell,
// and setRolloverText; keep its body at the original source position.
DC_ONLY(0xc038, 0x5C)
void CellAdjuster::restoreCell()
{
    if (m_obscuringHero) {
        m_obscuringHero->obscureCell();
        m_obscuringHero = 0;
    }
    if (m_obscuringBoat) {
        m_obscuringBoat->obscureCell();
        m_obscuringBoat = 0;
    }
    if (m_mobileHero) {
        m_mobileHero->restoreCell();
        m_mobileHero = 0;
    }
}

VA(0x0040b0e0, 0x64)  // dc 0xc094
void AdvManager::drawRolloverText(char* text)
{
    union {
        char* m_pointer;
        int m_value;
    } extra;
    extra.m_pointer = text;
    m_advWindow->broadcastMessage(0x200, 3, 200, extra.m_value);
    m_advWindow->drawWindow(0, 200, 200);

    Widget* rollover = m_advWindow->m_rolloverTextWidget;
    g_windowManager->updateScreen(m_advWindow->m_x + rollover->m_x,
                                  m_advWindow->m_y + rollover->m_y,
                                  rollover->m_width, rollover->m_height);
}

void getCreatureBankHelpText(char* buffer, NewmapCell* cell,
    CreatureBankType type, long playerId, const char* separator,
    unsigned char showFullList);
void setShrineHelpText(char* buffer, Hero* currentHero, NewmapCell* cell,
    GlobalInfoFlags type, const char* separator1, const char* separator2);
void setTreeHelpText(char* buffer, Hero* currentHero, NewmapCell* cell,
    const char* separator1, const char* separator2);
void setWitchHutHelpText(char* buffer, Hero* currentHero,
    NewmapCell* cell, const char* separator1, const char* separator2);

// E:\gamedcs\advmgr.cpp:3146
// 90.0426 -> 90.9026 (2026-08-20) on the explicit type_cell_adjuster
// restore at the foot of the body - see the comment at that site for why
// the obscure_cell/restore_cell census is exactly 2x ours and why the
// first copy is a STATEMENT, not a scope exit.

// CURRENT 2026-08-21 (93.938970%, from 90.902565%). Dreamcast CodeView's
// function-scope roster is visited, player, iThisPlayer, currHero,
// tempText[500], adjuster, playerbit, and infolevel. Restoring that roster
// exposes the decisive negative evidence: the four per-case arena/dead-guy/
// lean-to/garden bit temporaries were invented. Feeding their shifts straight
// into the shared visited carrier raises this body by 2.82 points. Routing
// HILL_FORT and UNIVERSITY through the separate infolevel carrier adds another
// 0.21. The DC-proven set_town_help/set_hero_help/set_pyramid_help source
// carriers inline away and are byte-flat here; the four 3-parameter helper
// calls regress because this retail caller uses its local player index while
// QuickInfo's inlined copies use gNetLocalGamePos, so those arms stay direct.

// The real-code branch deficit is now completely localized: ours has 184
// conditionals before RET against retail's 188. Three are the retail-inlined
// QUEST_GUARD temporary destructor (retail keeps the returned string in a
// dedicated [ebp-0x4c] EH local; ours uses the outgoing [ebp-0x240] area and
// calls _Tidy), and the fourth is retail's separate OBELISK visit test where
// ours jumps into a shared test. Four bounded source probes do not recover
// that phase: reversing the special-terrain condition scores 93.910890;
// binding the returned string through a const reference canonicalizes to the
// already-rejected named-string 93.931694 object; a one-call inline helper
// around the QuestGuard copy scores 93.894590; and the real size() VERIFY is
// byte-flat. None is retained.
// Two more OBELISK probes, 2026-09-06, both rejected against 93.938970:
// spelling the arm longhand (dropping the shared `visited` carrier and
// testing `obeliskFlags[...] & playerBit` directly) DOES buy retail's
// separate test - branches 184 -> 185, the right direction - but costs the
// arm's own shape, 92.7855; normalising the carrier to
// `(... & playerBit) != 0` is branch-flat at 184 and scores 93.7986.  So the
// missing branch is reachable and its price is higher than the branch, which
// puts the OBELISK quarter of the deficit with the other three (the
// QUEST_GUARD temporary's inlined _Tidy) in the budget class.
// 2026-09-06, polish lane 36, the DC LOCAL-SCOPE SWEEP (93.9598 -> 94.0915):
// the block names `thisHut` (CodeView 0x2664, sp+0x38) for the SEER arm's
// SeerHutList row, so the row is addressed once through a named reference
// instead of subscripted inside the rollover call.  Measured and rejected on
// top of that: naming the LIGHTHOUSE arm's twice-read
// `gpGame->mines[extraInfo].playerOwner` in the `owner` local the DC also
// carries (sp+0x3c) - 93.8845, retail re-reads it; and moving the
// `type_cell_adjuster` declaration up to the DC's slot order (between
// tempText and playerbit) - 93.4910.  The DC's `cTemp` buffers x4 and its
// `abandoned`/`guarded` pair do not transfer: Complete writes the global
// gText here, and the mine arm is the separate AdvmgrFn_0040D670 body the DC
// had inlined. Its `player`/`iThisPlayer` now retain the semantic names
// `player`/`thisPlayer`; `this_generator`/`type` are
// `mapGenerator`/`generatorType`.
// 2026-09-06, polish lane 40 (94.0915 -> 96.9535).  THE ARM MAP.  Both
// objects carry this switch's two tables inside the function - retail's
// dword arm table at +0x20d8 and its 216-entry byte index table at +0x21c4,
// ours as $L89522/$L89521 - and reading them settles the layout question
// outright.  Sorting the 59 arm addresses recovers retail's SOURCE order of
// the cases, and it is OURS, case for case: the four values sharing arm 0,
// the BORDER_GATE/BORDER_GUARD pair, SEPULCHER and SHIPWRECK between
// DERELICT_SHIP and DRAGON_CITY, QUEST_GUARD (215) after DRAGON_CITY,
// HILL_FORT before HERO, the lot.  Retail's arm INDEX assignment is
// ascending case value and so is ours.  So arm ORDER is not the debt and
// never was; the debt is per-arm SIZE, and consecutive table entries give
// each arm's length on both sides for free.  That reading is what this
// lane's four wins came out of, and what is
// left of the residual is now itemised rather than guessed: OBELISK -0x30
// and SIREN -0x11 and STABLES -5 (one cross-jump knot: retail keeps
// OBELISK's visited sprintf in the arm, we merge it into STABLES's copy),
// FOUNTAIN_OF_FORTUNE -0x18 (see its own note), WINDMILL -30 with
// WAGON/WARRIOR_TOMB/WATER_WHEEL +7/+6/+7 (one knot: those three cross-jump
// into WINDMILL's two blocks and retail's entry points include the
// gpGeneralText load), PYRAMID +0x11 (the same knot through
// set_pyramid_help), MAGIC_SPRING and MYSTICAL_GARDEN -6 each (retail
// stores the raw flag into `visited` and tests the second guard as a
// one-bit field, `shr ebx,0xa / test bl,1`; VC6 folds our shift spelling
// straight back to `test bh,4`), NOTHING +6 (retail keeps the
// special-terrain strcpy inline and shares the empty-string one; we do the
// reverse, and inverting the condition is already measured at 93.9109),
// and three small ones.  Twenty-four of the 59 arms differed in length when
// this lane opened; fifteen do now.  The sibling QuickInfo's table reads the
// same way (57 arms, 17 differing) and shares five of those rows exactly -
// PYRAMID, WAGON, WARRIOR_TOMB, WATER_WHEEL and WINDMILL are the same
// inlined helpers, so a fix there is worth double.
VA(0x0040b150, 0x229C)  // anchor-global, dc 0xc13c
void AdvManager::setRolloverText(NewmapCell* testCell, int rx, int ry)
{
    if (m_advWindow->m_chatEdit->m_hasFocus)
        return;

    int visited;
    PlayerData* player;
    int thisPlayer;
    Hero* currHero;
    char tempText[500];
    int playerBit;
    int infolevel;

    thisPlayer = g_game->getLocalPlayerGamePos();
    player = g_game->getLocalPlayer();
    playerBit = 1 << thisPlayer;
    currHero = g_game->getHero(player->m_currHeroId);

    CellAdjuster adjuster;
    NewmapCell* cell = adjuster.getTriggerCell(testCell, rx, ry);
    const char* separator = DATA_COMPGEN(
        0x00660330, rolloverSpaceSeparator, " ");
    const char* visitedFormat = DATA_COMPGEN(
        0x0066034c, rolloverVisitedFormat, " %s");

    switch (cell->m_type) {
    case NOTHING:
    case ANCHOR_POINT:
    case EVENT:
    case HOLY_GRAIL: {
        AdventureObjectType special = cell->getSpecialTerrain();
        if (special != NOTHING)
            strcpy(g_text, g_adventureObjectNames[special]);
        else
            strcpy(g_text, DATA_COMPGEN(
                0x00691210, rolloverEmptyText, ""));
        break;
    }
    case ARENA:
        strcpy(g_text, g_adventureObjectNames[ARENA]);
        if (cell->m_isTrigger && currHero) {
            visited = (currHero->m_arenaFlags & (1UL << (cell->m_extraInfo & 0x1f)));
            if (visited)
                sprintf(tempText, visitedFormat,
                        (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
            else
                sprintf(tempText, visitedFormat,
                        (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
            strcat(g_text, tempText);
        }
        break;
    case BORDER_GATE:
    case BORDER_GUARD:
        sprintf(g_text, DATA_COMPGEN(
            0x00660344, rolloverBorderFormat, "%s %s"),
            g_borderColorNames[cell->m_objectIndex],
            g_adventureObjectNames[cell->m_type]);
        break;
    case BORDER_TENT:
        sprintf(g_text, DATA_COMPGEN(
            0x00660344, rolloverBorderFormat, "%s %s"),
            g_borderColorNames[cell->m_objectIndex],
            g_adventureObjectNames[BORDER_TENT]);
        if (cell->m_isTrigger) {
            visited = (g_game->m_borderTentVisitFlags[cell->m_objectIndex] & playerBit);
            if (visited)
                sprintf(tempText, visitedFormat,
                        (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
            else
                sprintf(tempText, visitedFormat,
                        (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
            strcat(g_text, tempText);
        }
        break;
    case BUOY:
        strcpy(g_text, g_adventureObjectNames[BUOY]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(BuoyInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[BuoyInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = (currHero->m_flags & 0x4);
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case CLOVER_FIELD:
        strcpy(g_text, g_adventureObjectNames[CLOVER_FIELD]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(CloverFieldInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[CloverFieldInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = (currHero->m_flags & 0x8);
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case CREATURE_BANK:
        int bankType;
        bankType = cell->m_objectIndex;
        getCreatureBankHelpText(g_text, cell,
            CreatureBankType(bankType), g_unnamed69778c, separator, 0);
        break;
    case CREATURE_GENERATOR_1: {
        // DC3321/3342 records generator&.
        Generator& mapGenerator = g_game->m_generators[cell->m_extraInfo];
        int owner = mapGenerator.getOwner();
        // DC3324/3345 records int.
        int generatorType = mapGenerator.m_genType;
        if (owner != -1) {
            sprintf(g_text, DATA_COMPGEN(
                0x0066033c, rolloverOwnedObjectFormat, "%s - %s"),
                g_creatureGenerator1RolloverNames[generatorType],
                g_objectOwnerColorNames[owner]);
        } else {
            strcpy(g_text, g_creatureGenerator1RolloverNames[generatorType]);
        }
        break;
    }
    case CREATURE_GENERATOR_4: {
        // DC3321/3342 records generator&.
        Generator& mapGenerator = g_game->m_generators[cell->m_extraInfo];
        int owner = mapGenerator.getOwner();
        // DC3324/3345 records int.
        int generatorType = mapGenerator.m_genType;
        if (owner != -1) {
            sprintf(g_text, DATA_COMPGEN(
                0x0066033c, rolloverOwnedObjectFormat, "%s - %s"),
                g_creatureGenerator4RolloverNames[generatorType],
                g_objectOwnerColorNames[owner]);
        } else {
            strcpy(g_text, g_creatureGenerator4RolloverNames[generatorType]);
        }
        break;
    }
    case DEAD_GUY:
        strcpy(g_text, g_adventureObjectNames[DEAD_GUY]);
        if (cell->m_isTrigger && currHero) {
            visited = (player->m_deadGuyFlags & (1UL << (cell->m_extraInfo & 0x1f)));
            if (visited)
                sprintf(tempText, visitedFormat,
                        (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
            else
                sprintf(tempText, visitedFormat,
                        (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
            strcat(g_text, tempText);
        }
        break;
    case DEFENSE_TOWER:
        strcpy(g_text, g_adventureObjectNames[DEFENSE_TOWER]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(DefenseTowerInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[DefenseTowerInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = (currHero->m_defenseTowerFlags
                    & (1UL << (cell->m_extraInfo & 0x1f)));
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case DERELICT_SHIP:
        getCreatureBankHelpText(g_text, cell, CREATURE_BANK_DERELICT,
            g_unnamed69778c, separator, 0);
        break;
    case SEPULCHER:
        getCreatureBankHelpText(g_text, cell, CREATURE_BANK_SEPULCHER,
            g_unnamed69778c, separator, 0);
        break;
    case SHIPWRECK:
        getCreatureBankHelpText(g_text, cell, CREATURE_BANK_SHIPWRECK,
            g_unnamed69778c, separator, 0);
        break;
    case DRAGON_CITY:
        getCreatureBankHelpText(g_text, cell, CREATURE_BANK_DRAGON,
            g_unnamed69778c, separator, 0);
        break;
    case QUEST_GUARD: {
        strcpy(g_text,
            m_fullMap->m_questGuardList[cell->m_extraInfo]
                .questGuardFn00573040(g_unnamed69778c).c_str());
        break;
    }
    case FAERIE_RING:
        strcpy(g_text, g_adventureObjectNames[FAERIE_RING]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(FaerieRingInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[FaerieRingInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = (currHero->m_flags & 0x2000);
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    // DC3491 (ce94..cec4) adds four masked flag terms. Retail+0x8b1
    // likewise has four ANDs and three ADDs. Preserve the sum and DC's
    // operand order; both tested orders emit identical bytes under VC6.
    // The six-state query/sum family reproduced four objects. The sum
    // scores 96.1491% versus 96.2649% for the flattened mask because the
    // visited-format tail merges differently. The lower score is not
    // contrary source evidence. DC3477's separate GetInfoFlag assignment
    // below is byte-flat but remains present in the compiler input.
    case FOUNTAIN_OF_FORTUNE:
        strcpy(g_text, g_adventureObjectNames[FOUNTAIN_OF_FORTUNE]);
        if (cell->m_isTrigger) {
            // DC3477 assigns infolevel from GetInfoFlag before DC3479's
            // independent PlayerKnowsCell test. VC6 may elide the unused
            // result, but the recorded query is real source work.
            infolevel = g_game->getInfoFlag(FountainOfFortuneInfo, thisPlayer);
            if (cell->playerKnowsCell(g_unnamed69778c)) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[FountainOfFortuneInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = (currHero->m_flags & 0x20)
                    + (currHero->m_flags & 0x08000000)
                    + (currHero->m_flags & 0x10000000)
                    + (currHero->m_flags & 0x20000000);
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case FOUNTAIN_OF_YOUTH:
        strcpy(g_text, g_adventureObjectNames[FOUNTAIN_OF_YOUTH]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(FountainOfYouthInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[FountainOfYouthInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = (currHero->m_flags & 0x4000);
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case GARDEN_OF_REVELATION:
        strcpy(g_text, g_adventureObjectNames[GARDEN_OF_REVELATION]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(GardenOfRevelationInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[GardenOfRevelationInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = (currHero->m_gardenOfRevelationFlags
                    & (1UL << (cell->m_extraInfo & 0x1f)));
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case HILL_FORT:
        strcpy(g_text, g_adventureObjectNames[HILL_FORT]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(HillFortInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[HillFortInfo]);
                strcat(g_text, tempText);
            }
        }
        break;
    case HERO:
        setHeroHelp(g_text, cell);
        break;
    case IDOL_OF_FORTUNE:
        strcpy(g_text, g_adventureObjectNames[IDOL_OF_FORTUNE]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(IdolOfFortuneInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[IdolOfFortuneInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = ((currHero->m_flags
                    & 0x02000000UL) + (currHero->m_flags & 0x10UL));
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case LEAN_TO:
        strcpy(g_text, g_adventureObjectNames[LEAN_TO]);
        if (cell->m_isTrigger && currHero) {
            visited = (player->m_leanToFlags & (1UL << (cell->m_extraInfo & 0x1f)));
            if (visited)
                sprintf(tempText, visitedFormat,
                        (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
            else
                sprintf(tempText, visitedFormat,
                        (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
            strcat(g_text, tempText);
        }
        break;
    case LIBRARY:
        strcpy(g_text, g_adventureObjectNames[LIBRARY]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(LibraryInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[LibraryInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = (currHero->m_libraryFlags & (1UL << (cell->m_extraInfo & 0x1f)));
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case LIGHTHOUSE:
        strcpy(g_text, g_adventureObjectNames[LIGHTHOUSE]);
        if (cell->m_isTrigger) {
            if (g_game->m_mines[cell->m_extraInfo].m_playerOwner != -1) {
                sprintf(tempText, DATA_COMPGEN(
                    0x00660334, rolloverOwnerSuffixFormat, " - %s"),
                    g_objectOwnerColorNames[
                        g_game->m_mines[cell->m_extraInfo].m_playerOwner]);
                strcat(g_text, tempText);
            }
        }
        break;
    case MAGIC_SCHOOL:
        strcpy(g_text, g_adventureObjectNames[MAGIC_SCHOOL]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(MagicSchoolInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[MagicSchoolInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = (currHero->m_magicSchoolFlags
                    & (1UL << (cell->m_extraInfo & 0x1f)));
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case MAGIC_SPRING:
        strcpy(g_text, g_adventureObjectNames[MAGIC_SPRING]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(MagicSpringInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[MagicSpringInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = ((player->m_magicSpringFlags
                    & (1UL << (cell->m_extraInfo & 0x1f))) && !((cell->m_extraInfo >> 6) & 1));
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case MAGIC_WELL:
        strcpy(g_text, g_adventureObjectNames[MAGIC_WELL]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(MagicWellInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[MagicWellInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = (currHero->m_flags & 0x1);
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case MERC_CAMP:
        strcpy(g_text, g_adventureObjectNames[MERC_CAMP]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(MercCampInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[MercCampInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = (currHero->m_mercCampFlags & (1UL << (cell->m_extraInfo & 0x1f)));
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case MERMAID:
        strcpy(g_text, g_adventureObjectNames[MERMAID]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(MermaidInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[MermaidInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = (currHero->m_flags & 0x8000);
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case MINE:
        advmgrFn0040D670(g_text, cell, thisPlayer, separator, 0);
        break;
    case MONSTER:
        if (cell->m_isTrigger) {
            // DC 3918 calls GetArmyName(type, 2); retail expands the same
            // bounds check and plural-name selection. Preserve that helper.
            const char* creatureName = getArmyName(cell->m_objectIndex, 2);
            sprintf(g_text, DATA_COMPGEN(
                0x00660344, rolloverMonsterFormat, "%s %s"),
                ArmyGroup::getArmySizeName(cell->m_extraInfo & 0xfff, 1),
                creatureName);
        }
        break;
    case MYSTICAL_GARDEN:
        strcpy(g_text, g_adventureObjectNames[MYSTICAL_GARDEN]);
        if (cell->m_isTrigger) {
            visited = ((player->m_mysticalGardenFlags
                & (1UL << (cell->m_extraInfo & 0x1f))) && !((cell->m_extraInfo >> 10) & 1));
            if (visited)
                sprintf(tempText, visitedFormat,
                        (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
            else
                sprintf(tempText, visitedFormat,
                        (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
            strcat(g_text, tempText);
        }
        break;
    case OASIS:
        strcpy(g_text, g_adventureObjectNames[OASIS]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(OasisInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[OasisInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = (currHero->m_flags & 0x80);
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case OBELISK:
        strcpy(g_text, g_adventureObjectNames[OBELISK]);
        if (cell->m_isTrigger) {
            visited = (g_game->m_obeliskFlags[cell->m_extraInfo] & playerBit);
            if (visited)
                sprintf(tempText, visitedFormat,
                        (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
            else
                sprintf(tempText, visitedFormat,
                        (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
            strcat(g_text, tempText);
        }
        break;
    case POWER_SCHOOL:
        strcpy(g_text, g_adventureObjectNames[POWER_SCHOOL]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(PowerSchoolInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[PowerSchoolInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = (currHero->m_powerSchoolFlags
                    & (1UL << (cell->m_extraInfo & 0x1f)));
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case PYRAMID:
        setPyramidHelp(g_text, cell, currHero, separator);
        break;
    case RALLY_FLAG:
        strcpy(g_text, g_adventureObjectNames[RALLY_FLAG]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(RallyFlagInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[RallyFlagInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = (currHero->m_flags & 0x10000);
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case RESOURCE:
        strcpy(g_text, g_resourceNames[cell->m_objectIndex]);
        break;
    case SEER: {
        const SeerHut& thisHut = m_fullMap->m_seerHutList[cell->m_extraInfo];
        strcpy(g_text, thisHut.seerHutFn005741B0(thisPlayer).c_str());
        break;
    }
    case SHRINE1:
        setShrineHelpText(g_text, currHero, cell, Shrine1Info,
                          separator, separator);
        break;
    case SHRINE2:
        setShrineHelpText(g_text, currHero, cell, Shrine2Info,
                          separator, separator);
        break;
    case SHRINE3:
        setShrineHelpText(g_text, currHero, cell, Shrine3Info,
                          separator, separator);
        break;
    case SIREN:
        strcpy(g_text, g_adventureObjectNames[SIREN]);
        if (cell->m_isTrigger && currHero)
            visited = (currHero->m_flags & 0x100000);
        if (visited)
            sprintf(tempText, visitedFormat,
                    (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
        else
            sprintf(tempText, visitedFormat,
                    (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
        strcat(g_text, tempText);
        break;
    case STABLES:
        strcpy(g_text, g_adventureObjectNames[STABLES]);
        if (cell->m_isTrigger && currHero)
            visited = (currHero->m_flags & 0x2);
        if (visited)
            sprintf(tempText, visitedFormat,
                    (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
        else
            sprintf(tempText, visitedFormat,
                    (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
        strcat(g_text, tempText);
        break;
    case TEMPLE:
        strcpy(g_text, g_adventureObjectNames[TEMPLE]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(TempleInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[TempleInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = ((currHero->m_flags
                    & 0x04000000UL) + (currHero->m_flags & 0x100UL));
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case TOWN:
        setTownHelp(g_text, cell);
        break;
    case TRAINING_GROUNDS:
        strcpy(g_text, g_adventureObjectNames[TRAINING_GROUNDS]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(TrainingGroundsInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[TrainingGroundsInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = (currHero->m_trainingGroundsFlags
                    & (1UL << (cell->m_extraInfo & 0x1f)));
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case TREE_OF_KNOWLEDGE:
        setTreeHelpText(g_text, currHero, cell, separator, separator);
        break;
    case UNIVERSITY:
        strcpy(g_text, g_adventureObjectNames[UNIVERSITY]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(UniversityInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[UniversityInfo]);
                strcat(g_text, tempText);
            }
        }
        break;
    case WAGON:
        setWagonHelpText(g_text, cell, separator);
        break;
    case WAR_SCHOOL:
        strcpy(g_text, g_adventureObjectNames[WAR_SCHOOL]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(WarSchoolInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[WarSchoolInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = (currHero->m_warSchoolFlags & (1UL << (cell->m_extraInfo & 0x1f)));
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case WARRIOR_TOMB:
        setTombHelpText(g_text, cell, separator);
        break;
    case WATER_WHEEL:
        setWaterWheelHelpText(g_text, cell, separator);
        break;
    case WATERING_HOLE:
        strcpy(g_text, g_adventureObjectNames[WATERING_HOLE]);
        if (cell->m_isTrigger) {
            infolevel = g_game->getInfoFlag(WateringHoleInfo, thisPlayer);
            if (infolevel) {
                sprintf(tempText, visitedFormat,
                        g_globalInfoFlagNames[WateringHoleInfo]);
                strcat(g_text, tempText);
            }
            if (currHero) {
                visited = (currHero->m_flags & 0x40);
                if (visited)
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_VISITED_OBJECT]);
                else
                    sprintf(tempText, visitedFormat,
                            (*g_generalText)[GENERAL_TEXT_UNVISITED_OBJECT]);
                strcat(g_text, tempText);
            }
        }
        break;
    case WINDMILL:
        setWindmillHelpText(g_text, cell, separator);
        break;
    case WITCH_HUT:
        setWitchHutHelpText(g_text, currHero, cell,
                                separator, separator);
        break;
    default: {
        if (cell->m_type >= NOTHING && cell->m_type < 232)
            strcpy(g_text, g_adventureObjectNames[cell->m_type]);
        else
            strcpy(g_text, DATA_COMPGEN(
                0x00691210, rolloverEmptyText, ""));
        break;
    }
    }

    // DC names the explicit restore before DrawRolloverText. Retail expands
    // it here and again at scope exit through ~type_cell_adjuster.
    adjuster.restoreCell();

    drawRolloverText(g_text);
}

// The help-text group. Retail files these five statics AFTER
// SetRolloverText, not before it as the DC source does (the same
// define-the-helper-after-its-caller shape border.cpp records); all
// five are called from BOTH SetRolloverText (0xb150) and QuickInfo
// (0x137c0), which is why /Ob2 left them out of line. Only five of the
// DC's eleven survive as separate bodies - set_town_help,
// set_hero_help, set_pyramid_help and the four 3-param wagon/tomb/
// water_wheel/windmill builders have no retail row at all.

// Arity pins the group. Free functions are /Gr fastcall, so ret N
// counts (params - 2): the run is 6, 5, 6, 5, 5 at 0xd3f0, 0xd670,
// 0xd8d0, 0xdb00, 0xdcc0 against a DC sequence of 6 (creature bank),
// 6 (shrine), 5 (tree), 5 (witch hut). The order-preserving, arity-
// respecting embedding of the DC four into the retail five is UNIQUE,
// and it leaves 0xd670 - a 5-param row that, like the creature-bank
// builder, calls the army describer 0xabe0 and armyGroup::HasCreatures
// - as a retail-only sibling with no DC counterpart. Its two callers and
// `ret 0xc` admit the ordinal five-parameter declaration used above and the
// reconstructed body below. 0xabe0 is named only by its independently
// reconstructed army-description role.

// 0xd3f0 is the one with independent body evidence: 6 params AND the
// two creature-bank-only callees (0xabe0, armyGroup::HasCreatures).
// E:\gamedcs\advmgr.cpp:2762
// RETAIL-RECONSTRUCTED 2026-08-09 (87.2833%). The std::string bank name,
// short-width visited-player gate and branch-local bank lookups restore the
// retail control flow. Sibling full/compact scopes now share retail's -0x14
// result-string home and 0x14-byte frame; the residual is register allocation
// and the associated cleanup-tail scheduling.

VA(0x0040d3f0, 0x27C)  // anchor-callee, dc 0xb3bc
void getCreatureBankHelpText(char* buffer, NewmapCell* cell, CreatureBankType type, long playerId, const char* separator, unsigned char showFullList)
{
    strcpy(buffer, g_constCreatureBankTraits[type].m_name.c_str());
    strcat(buffer, separator);

    const char* armyName;
    if (!cell->playerKnowsCell(playerId)) {
        armyName = g_generalText->getText(GENERAL_TEXT_UNVISITED_OBJECT);
    } else {
        unsigned long testFlag = cell->m_extraInfo;
        if ((testFlag & 0x02000000)
            || !g_game->m_creatureBanks[
                    (testFlag >> 13) & 0xfff].m_guards.hasCreatures()) {
            armyName = g_generalText->getText(GENERAL_TEXT_VISITED_OBJECT);
        } else {
            if (showFullList) {
                std::string result = getArmyHelpText(
                    &g_game->m_creatureBanks[
                        (cell->m_extraInfo >> 13) & 0xfff].m_guards, 1);
                strcat(buffer, result.c_str());
                return;
            } else {
                strcat(buffer, "(");
                std::string result = getArmyHelpText(
                    &g_game->m_creatureBanks[
                        (cell->m_extraInfo >> 13) & 0xfff].m_guards, 0);
                strcat(buffer, result.c_str());
                armyName = ")";
            }
        }
    }
    strcat(buffer, armyName);
}

// RETAIL-RECONSTRUCTED (97.3418%): no distinct Dreamcast row survives, but the
// two retail callers fix this five-parameter /Gr ABI and the MINE case role.
// The body reads the byte-proven mine pool, chooses the ordinary/abandoned
// description, adds owner and allied-resource text, then appends the guard-army
// description. The direct string temporary raised 91.0717% to 96.3924%; the
// symmetric player/owner OnSameTeam order raises it to the retained score and
// makes every instruction from that comparison onward exact. Both sides have
// the same 17 blocks, 10 branches and two returns. The residue is only the
// earlier owner/player EAX<->EDI homing: the guided nine-mutation register
// sweep found no improvement, while the allocator model reports identical
// first definitions and therefore no source-addressable minimum slice.
VA(0x0040d670, 0x253)
void advmgrFn0040D670(char* buffer, NewmapCell* cell, long playerId,
                       const char* separator, unsigned char showFullList)
{
    Mine* currentMine = &g_game->m_mines[cell->m_extraInfo];
    int owner = currentMine->m_playerOwner;
    int mineType = currentMine->m_type;
    const char* description = g_mineDescriptions[7];
    if (!currentMine->m_isAbandoned)
        description = g_mineDescriptions[mineType];
    strcpy(buffer, description);

    if (owner != -1) {
        strcat(buffer, separator);
        strcat(buffer, g_objectOwnerColorNames[owner]);
    }

    if (owner >= 0) {
        if (playerId >= 0 && g_game->onSameTeam(playerId, owner)) {
            strcat(buffer, separator);
            strcat(buffer, DATA_COMPGEN(
                0x00660354, mineResourceOpen, "("));
            strcat(buffer, g_resourceNames[mineType]);
            strcat(buffer, DATA_COMPGEN(
                0x00660350, mineResourceClose, ")"));
        }
    }

    ArmyGroup* guards = &currentMine->m_guards;
    if (guards->hasCreatures()) {
        strcat(buffer, separator);
        strcat(buffer,
               getArmyHelpText(guards, showFullList).c_str());
    }
}

VA(0x0040d8d0, 0x229)  // dc 0xb788
void setShrineHelpText(char* buffer, Hero* currentHero, NewmapCell* cell, GlobalInfoFlags type, const char* separator1, const char* separator2)
{
    g_game->getLocalPlayerGamePos();
    strcpy(buffer, g_adventureObjectNames[cell->m_type]);
    if (!cell->m_isTrigger)
        return;

    unsigned char knowsShrineType =
        g_game->getInfoFlag(type, g_netLocalGamePos);
    if (cell->playerKnowsCell(g_netLocalGamePos)) {
        SpellID spell = static_cast<int>(cell->m_extraInfo << 9) >> 22;
        strcat(buffer, separator1);
        char temp[500];
        sprintf(temp, g_generalText->getText(GENERAL_TEXT_SHRINE_SPELL_FORMAT),
                g_spellTraits[spell].m_name);
        strcat(buffer, temp);
        if (currentHero && currentHero->spellIsAvailable(spell)) {
            strcat(buffer, separator2);
            strcat(buffer,
                   g_generalText->getText(GENERAL_TEXT_KNOWN_SHRINE_SPELL));
        }
    } else if (knowsShrineType) {
        strcat(buffer, separator1);
        strcat(buffer, g_globalInfoFlagNames[type]);
    }
}

VA(0x0040db00, 0x1BD)  // dc 0xb930
void setTreeHelpText(char* buffer, Hero* currentHero, NewmapCell* cell, const char* separator1, const char* separator2)
{
    strcpy(buffer, g_treeOfKnowledgeName);
    if (!cell->m_isTrigger)
        return;

    unsigned char visited =
        g_game->getInfoFlag(TreeOfKnowledgeInfo, g_netLocalGamePos);
    int infolevel = visited;
    if (cell->playerKnowsCell(g_netLocalGamePos)) {
        strcat(buffer, separator1);
        int price = static_cast<int>(cell->m_extraInfo << 16) >> 29;
        strcat(buffer, g_wiseTreePriceNames[price]);
    } else if (infolevel) {
        strcat(buffer, separator1);
        strcat(buffer, g_knownTreePriceText);
    }

    if (currentHero) {
        unsigned char heroVisited =
            (currentHero->m_treeOfKnowledgeFlags
             & (1UL << (static_cast<unsigned char>(cell->m_extraInfo)
                        & 0x1f))) != 0;
        strcat(buffer, separator2);
        if (heroVisited)
            strcat(buffer,
                   g_generalText->getText(GENERAL_TEXT_VISITED_OBJECT));
        else
            strcat(buffer,
                   g_generalText->getText(GENERAL_TEXT_UNVISITED_OBJECT));
    }
}

VA(0x0040dcc0, 0x1E4)  // dc 0xbd84
void setWitchHutHelpText(char* buffer, Hero* currentHero, NewmapCell* cell, const char* separator1, const char* separator2)
{
    strcpy(buffer, g_witchHutName);
    if (!cell->m_isTrigger)
        return;

    if ((cell->m_extraInfo & WitchHutNoSkillMask) == WitchHutNoSkillMask)
        return;

    if (cell->playerKnowsCell(g_netLocalGamePos)) {
        int skill = static_cast<int>(cell->m_extraInfo << 12) >> 25;
        strcat(buffer, separator1);
        char tempText[50];
        sprintf(tempText,
                g_generalText->getText(GENERAL_TEXT_WITCH_SKILL_FORMAT),
                g_sSkillTraits[skill].m_name);
        strcat(buffer, tempText);
        if (currentHero && currentHero->m_skillLevel[skill]) {
            strcat(buffer, separator2);
            strcat(buffer, g_generalText->getText(
                GENERAL_TEXT_HERO_KNOWS_WITCH_SKILL));
        }
    } else if (g_game->getInfoFlag(WitchHutInfo, g_netLocalGamePos)) {
        strcat(buffer, separator1);
        strcat(buffer, g_knownWitchSkillText);
    }
}

// E:\gamedcs\advmgr.cpp:4385
// RETAIL-RECONSTRUCTED 2026-08-11 (80.9400%). Retail proves the complete
// map-area, packed-hover, visibility/current-level, rollover, owned hero/town
// cursor and scroll-zone fallback paths. The 27-branch and four-return counts
// and every symbolic branch target agree. The remaining delta is VC6
// register/stack scheduling around the inlined point helpers: retail retains
// mouseY/mouseX in EBX/EDI and reuses their parameter homes for local-player
// state, giving an eight-byte local frame; this compile spills that state and
// uses sixteen bytes. A shared rx/ry scope was tested and rejected (80.27%).
VA(0x0040deb0, 0x3CF)  // anchor-callee, dc 0xed7c
int AdvManager::processWaitingHover(int mouseX, int mouseY)
{
    if (inMapArea(mouseX, mouseY)) {
        int rx = mouseX / 32;
        int ry = mouseY / 32;
        m_advCommand = -1;
        m_lastHoverX = rx;
        m_lastHoverY = ry;
        m_lastMapHover.m_x = m_radarOrigin.m_x + rx;
        m_lastMapHover.m_y = m_radarOrigin.m_y + ry;
        m_lastMapHover.m_z = m_radarOrigin.m_z;

        int thisPlayerBit = 1 << g_game->getLocalPlayerGamePos();
        PlayerData* thisPlayer = g_game->getLocalPlayer();
        if (m_lastMapHover.isValid()
            && (getMapExtra(m_lastMapHover) & thisPlayerBit)) {
            if (thisPlayer->m_currHeroId == -1
                || g_game->getHero(thisPlayer->m_currHeroId)->m_z
                    == m_radarOrigin.m_z) {
                type_point point(m_radarOrigin.m_x + m_lastHoverX, m_radarOrigin.m_y + m_lastHoverY,
                                 m_radarOrigin.m_z);

                type_point cellPoint = point;
                NewmapCell* currCell;
                if (!cellPoint.isValid())
                    currCell = m_fullMap->cell(0, 0, 0);
                else
                    currCell = m_fullMap->cell(cellPoint.m_x, cellPoint.m_y,
                                             cellPoint.m_z);

                // rx/ry, NOT mouseX/mouseY - retail passes the /32 CELL
                // coordinates here, exactly as ProcessHover does. At
                // fn+0x246 it pushes ebx and edi, and edi is built at
                // fn+0x28 as `mov eax,edi / cdq / and edx,0x1f / add
                // eax,edx / sar edi,5` from [ebp+8] - the same value it
                // then stores into lastHoverX at [esi+0xec]. Passing the
                // raw pixel coordinates kept both parameters live to this
                // point and cost VC6 the two dead parameter homes retail
                // spills into (`mov [ebp+8],eax` right at this call).
                setRolloverText(currCell, rx, ry);
                switch (currCell->m_type) {
                case TOWN: {
                    class Town* town = g_game->getTown(currCell->m_extraInfo);
                    if (g_game->isLocalHuman(town->m_owner)) {
                        m_advCommand = thisPlayer->m_currTownId == -1 ? 5 : 3;
                        g_mouseManager->setPointer(
                            3, MouseManager::ADVENTURE_SET);
                        return 1;
                    }
                    break;
                }
                case HERO: {
                    Hero* hero = g_game->getHero(currCell->m_extraInfo);
                    if (g_game->isLocalHuman(hero->m_owner)
                        && hero->m_owner == g_netLocalGamePos) {
                        g_mouseManager->setPointer(
                            2, MouseManager::ADVENTURE_SET);
                        m_advCommand = thisPlayer->m_currHeroId == -1 ? 4 : 2;
                        return 1;
                    }
                    break;
                }
                }
            }
        }

        g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);
        return 1;
    }

    if (g_mouseManager->m_frame >= HOVER_SCROLL_POINTER_FIRST
        && g_mouseManager->m_frame <= HOVER_SCROLL_POINTER_LAST) {
        if (!mouseInScrollZone())
            g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);
    }

    m_advWindow->processHover(mouseX, mouseY);
    return 1;
}

// DC advmgr.cpp:4514..4524 proves the private member get_garrison_cursor.
// ProcessHover's retail GARRISON arm expands it and retains getNormalCursor.
AdventureCursor AdvManager::getGarrisonCursor(NewmapCell* currCell)
{
    if (currCell->m_isTrigger) {
        Garrison& mapGarrison = *g_game->getGarrison(currCell->m_extraInfo);
        if (!g_game->onSameTeam(mapGarrison.m_playerOwner, g_netLocalGamePos)
            && mapGarrison.m_garrisonArmy.hasCreatures())
            return ADV_SWORD_POINTER;
    }
    return getNormalCursor(currCell);
}

VA(0x0040e280, 0xD3)  // dc 0xf2c0
AdventureCursor AdvManager::getNormalCursor(NewmapCell* currCell)
{
    HOMM3_RELEASE_VERIFY(currCell != 0);
    if ((getMapExtra(m_lastMapHover) & MAP_EXTRA_MONSTER)
        && (!currCell->m_isTrigger
            || !g_adventureObjectLandBlocked[currCell->m_type][0])) {
        return ADV_SWORD_POINTER;
    }

    if (m_cursorType == CURSOR_TYPE_8) {
        if (currCell->m_isTrigger && currCell->m_groundSet == eTerrainWater)
            return ADV_BOAT_EVENT_POINTER;
        return ADV_BOAT_POINTER;
    }

    if (currCell->m_isTrigger) {
        if (currCell->m_groundSet != eTerrainWater)
            return ADV_EVENT_POINTER;
        if (currCell->m_type == SHIPWRECK)
            return ADV_EVENT_POINTER;
    }
    return ADV_WALK_POINTER;
}

// E:\gamedcs\advmgr.cpp:4556
// RETAIL-RECONSTRUCTED 2026-08-09 (74.7787%). Retail proves the complete
// local-human, visibility, ownership, path reachability/turn count, object
// cursor dispatch and scroll-zone control flow. The rollover call receives
// map-cell coordinates, not screen pixels. The current-hero path constructs
// and compares the same packed type_point as retail. Residual codegen
// differences are concentrated in the Dinkumware result-vector erase
// expansion and merged exit layout.

// THE `currHeroId` LOCAL WAS THE WALL, NOT THE ACCESSOR (74.7786 ->
// 78.8021, 2026-08-15). dc 0xf3a8 line 4601 tests
// `gpCurrentPlayer->currHeroId == -1` against the FIELD - there is no
// local anywhere in its 125-line table - and retail agrees at byte level:
// it loads gpCurrentPlayer once, reads `[edx+4]` as a DWORD into ECX and
// then spends that one register on all three `cmp ecx,-1` tests (the
// z-check's `!= -1`, GetHero's own, and the no-hero block's). Our copy
// into an `int currHeroId` broke that chain. Reading the field at each
// use restores it.

// AND THE DC's OWN ACCESSOR IS REFUSED HERE, which is the asymmetry rule
// biting the other way. dc 0xf3a8 line 4642 is a call to
// `game::GetCurrHero`, but landing it measures 74.7786 against 78.8021
// for `GetHero(gpCurrentPlayer->currHeroId)` - a 4.02-point loss, with
// and without the local. Retail's bytes say why: GetCurrHero re-reads the
// id inside its taken arm and compares it at CHAR width (that re-read is
// exactly what made it right in TBottomViewTown/TBottomViewHero), where
// this body's three tests share ONE dword already in a register. The DC
// is an older revision; retail's bytes outrank it.

// 79.7982 -> 83.5000 (2026-08-21): the cursor-type switch was in enum
// value order, but retail's physical arm order puts HERO before GARRISON.
// Moving that source block is semantic-order neutral and cuts why-branch's
// distance 117 -> 105; moving it back is the inverse regression. The
// post-edit CFG census now agrees at 91 conditional branches and 8 returns
// (764 instructions / 134 blocks against retail's 770 / 137).

// The DC local roster was checked at the same time. Sharing its one cTown
// local across both town arms, widening currCell to that block, and grouping
// `iTurns, iMouseOffset, currHero, new_cursor, path_cell` at the current-hero
// block head are all byte-flat, separately and together, so the narrower
// x86-winning scopes remain below.

// Residual (91.00%, from 83.50; 2026-09-05): the exits are INLINE, not
// goto-shared. The Dreamcast line table spells every cursor exit as
// `SetPointer(n, ADVENTURE_SET); advCommand = m; return 1;` at its own
// site, and retail's layout is what VC6's cross-jumper makes of that:
// the copy with a fall-through predecessor survives (the hero-location
// SetPointer(2), the hero-mode shipyard SetPointer(6), the clear_path
// SetPointer(0)) and the earlier no-hero sites jump to it, where the
// old labels put every shared block at the end with no fall-through
// at all. Two semantic corrections rode along, both retail-proven: the
// same-hover path returns 1 WITHOUT the window ProcessHover (retail
// `je` lands on the epilogue after that call), and the off-map tail is
// `frame < FIRST || frame > LAST || !MouseInScrollZone()` (retail's
// `jl`/`jg` both reach the SetPointer(0) call; ProcessWaitingHover's
// tail is the other way round and keeps its `&&` form). The hero id
// split is if/else, the path-cursor block hoists `iTurns` /
// `iMouseOffset = 0` / `new_cursor` ahead of the visited test, computes
// `iMouseOffset = iTurns * 6` right after `advCommand = 1` (retail
// stores it to [ebp+0xc] before the switch), takes `else new_cursor =
// 0` (retail's `xor eax,eax` after the arms - which is what stops the
// default arm from falling into the join and lets ANCHOR_POINT's
// get_normal_cursor copy survive), and orders the arms as the DC does:
// BOAT, ANCHOR_POINT, MONSTER, HERO, GARRISON, TOWN, default. GetCell,
// get_garrison_cursor and MouseInScrollZone are the DC's call sites.
// The `NewfullMap::cell` CALL inside the GetCell expansion is now spelled
// (91.0034 -> 91.6263): the guard is written out at this site so the
// square is reached through the accessor, which this TU compiles out of
// line, while the standalone GetCell body at 0x412bd0 keeps the longhand
// subscript and stays exact. Left: `_Destroy` is a CALL inside retail's
// clear_path expansion and an expansion in ours - the vendored-header
// child-call shape, unreachable without a pin inside <vector> - plus one
// branch and register parity in the GetHero expansions (retail loads the
// id into ecx, ours eax).
// Residual (91.6263%), LOCALISED 2026-09-06 and it is ONE inline decision.
// The call streams carry exactly one retail-only entry - the ICF-folded
// `vector<pathCell>::_Destroy` at fn+0x596 - and it sits inside the third
// `gpSearchArray->clear_path()` (findpath.h's `result.erase(begin(), end())`).
// Retail expands the erase there and CALLS the empty `_Destroy(_S, _Last)`
// before writing `_Last = _S`; we expand the erase AND the (trivial) _Destroy,
// so the call, its one branch (91 retail against our 90) and the two frame
// dwords it prices (0x10 against our 0x8) all go together.  That is an
// OVER-inline of a template leaf with no admissible lever: a statement pin is
// a falling-only floor and caller-shrink would need an invented static.
// 2026-09-06, polish lane 36, the DC LOCAL-SCOPE SWEEP - measured and
// rejected.  The Dreamcast block names TWO `cellExtra` locals
// (ExtraInfoUnion, sp+0x44 and sp+0x40), i.e. the trigger cell's extraInfo
// is read once into a named union per block and both the TOWN id and the
// SHIPYARD owner come out of it, where this body calls
// `get_trigger_cell()->get_map_extraInfo()` at all four sites.  One
// `ExtraInfoUnion cellExtra;` per big block scores 91.6133 and one per ARM
// scores the same, against 91.6263 - retail re-reads.
VA(0x0040e360, 0x918)  // anchor-callee, dc 0xf3a8
int AdvManager::processHover(int mouseX, int mouseY)
{
    if (!g_currentPlayer->isLocalHuman())
        return processWaitingHover(mouseX, mouseY);

    if (inMapArea(mouseX, mouseY)) {
        int rx = mouseX / 32;
        int ry = mouseY / 32;
        if (m_lastHoverX != rx || m_lastHoverY != ry) {
        m_advCommand = -1;
        m_lastHoverX = rx;
        m_lastHoverY = ry;
        m_lastMapHover.m_x = m_radarOrigin.m_x + rx;
        m_lastMapHover.m_y = m_radarOrigin.m_y + ry;
        m_lastMapHover.m_z = m_radarOrigin.m_z;

        if (!m_lastMapHover.isValid()
            || !(getMapExtra(m_lastMapHover) & g_mapVisibilityBit)) {
            g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);
            return 1;
        }

        // Retail expands the GetCell guard here but reaches the square
        // through NewfullMap::cell, which this TU compiles out of line -
        // the standalone GetCell body at 0x412bd0 keeps the longhand
        // subscript and is exact, so the two spellings really do coexist
        // in retail's own source.
        NewmapCell* currCell;
        if (!m_lastMapHover.isValid())
            currCell = m_fullMap->cell(0, 0, 0);
        else
            currCell = m_fullMap->cell(m_lastMapHover.m_x, m_lastMapHover.m_y,
                                     m_lastMapHover.m_z);
        setRolloverText(currCell, rx, ry);

        if (g_currentPlayer->m_currHeroId != -1
            && g_game->getHero(g_currentPlayer->m_currHeroId)->m_z
               != m_lastMapHover.m_z) {
            g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);
            return 1;
        }

        if (g_currentPlayer->m_currHeroId == -1) {
            if (currCell->m_type == TOWN) {
                Town* currentTown = g_game->getTown(
                    currCell->getTriggerCell()->getMapExtraInfo());
                if (g_game->onSameTeam(currentTown->m_owner, g_netLocalGamePos)
                    || m_debugViewAll) {
                    g_mouseManager->setPointer(3,
                                               MouseManager::ADVENTURE_SET);
                    m_advCommand = 3;
                    return 1;
                }
            }

            if (currCell->m_type == HERO) {
                Hero* mapHero = g_game->getHero(currCell->m_extraInfo);
                if (mapHero->m_owner == g_netLocalGamePos) {
                    g_mouseManager->setPointer(2, MouseManager::ADVENTURE_SET);
                    m_advCommand = 2;
                    return 1;
                }
            }

            if (currCell->m_type == SHIPYARD) {
                int owner = static_cast<int>(
                    currCell->getTriggerCell()->getMapExtraInfo() << 24)
                    >> 24;
                if (g_game->onSameTeam(owner, g_netLocalGamePos)) {
                    g_mouseManager->setPointer(6, MouseManager::ADVENTURE_SET);
                    m_advCommand = 8;
                    return 1;
                }
            }

            g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);
            return 1;
        } else {

        Hero* currHero = g_game->getHero(g_currentPlayer->m_currHeroId);
        // FIELD ASSIGNMENT, not the ctor, at this ONE site: the ctor form
        // that is worth +2.5 to +5.7 in the neighbouring hover/context
        // bodies costs 0.62 here (83.5000 -> 82.8763). Measured per site.
        type_point heroPoint;
        heroPoint.m_x = currHero->m_x;
        heroPoint.m_y = currHero->m_y;
        heroPoint.m_z = currHero->m_z;
        if (heroPoint == m_lastMapHover) {
            g_mouseManager->setPointer(2, MouseManager::ADVENTURE_SET);
            m_advCommand = 2;
            return 1;
        }

        if (currCell->m_flags0011 & 0x100) {
            if (currCell->m_type == TOWN) {
                Town* currentTown = g_game->getTown(
                    currCell->getTriggerCell()->getMapExtraInfo());
                if (g_game->onSameTeam(currentTown->m_owner, g_netLocalGamePos)
                    || m_debugViewAll) {
                    g_mouseManager->setPointer(3,
                                               MouseManager::ADVENTURE_SET);
                    m_advCommand = 5;
                    return 1;
                }
            } else if (currCell->m_type == SHIPYARD) {
                int owner = static_cast<int>(
                    currCell->getTriggerCell()->getMapExtraInfo() << 24)
                    >> 24;
                if (g_game->onSameTeam(owner, g_netLocalGamePos)) {
                    g_mouseManager->setPointer(6, MouseManager::ADVENTURE_SET);
                    m_advCommand = 8;
                    return 1;
                }
            }

            g_searchArray->clearPath();
            g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);
            return 1;
        }

        int inBoat = currHero->m_flags & 0x40000;
        if (!inBoat) {
            if (currCell->m_groundSet == eTerrainWater
                && (currCell->m_type != HERO || !currCell->m_isTrigger)
                && (currCell->m_type != BOAT || !currCell->m_isTrigger)
                && (currCell->m_type != SHIPWRECK || !currCell->m_isTrigger)) {
                g_searchArray->clearPath();
                g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);
                return 1;
            }
        } else if (currCell->m_groundSet != eTerrainWater
                   && currCell->m_type != ANCHOR_POINT) {
            g_searchArray->clearPath();
            g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);
            return 1;
        }

        seedTo(m_lastMapHover);
        PathCell* currentPathCell = g_searchArray->getCell(m_lastMapHover, 0);
        int turns;
        int mouseOffset = 0;
        int newCursor;
        if (currentPathCell->m_visited) {
            if (currentPathCell->m_cost <= currHero->m_movePoints) {
                turns = 0;
            } else {
                turns = (currentPathCell->m_cost - currHero->m_movePoints - 1)
                         / currHero->m_maxMovePoints + 1;
                if (turns > 3)
                    turns = 3;
            }

            m_advCommand = 1;
            mouseOffset = turns * 6;
            switch (currCell->m_type) {
            case BOAT:
                if (m_cursorType != CURSOR_TYPE_8) {
                    newCursor = 6;
                    m_advCommand = 1;
                } else {
                    newCursor = 0;
                    m_advCommand = -1;
                }
                break;
            case ANCHOR_POINT:
                if (m_cursorType == CURSOR_TYPE_8)
                    newCursor = 7;
                else
                    newCursor = getNormalCursor(currCell);
                break;
            case MONSTER:
                newCursor = 5;
                break;
            case HERO: {
                Hero* mapHero = g_game->getHero(currCell->m_extraInfo);
                if (g_game->onSameTeam(mapHero->m_owner, g_netLocalGamePos)) {
                    newCursor = 8;
                    m_advCommand = 1;
                } else {
                    newCursor = 5;
                }
                break;
            }
            case GARRISON:
                newCursor = getGarrisonCursor(currCell);
                break;
            case TOWN: {
                Town* currentTown = g_game->getTown(currCell->m_extraInfo);
                if (currCell->m_isTrigger
                    && !g_game->onSameTeam(currentTown->m_owner, g_netLocalGamePos)
                    && currentTown->hasGarrison())
                    newCursor = 5;
                else
                    newCursor = getNormalCursor(currCell);
                break;
            }
            default:
                newCursor = getNormalCursor(currCell);
                break;
            }
        } else {
            newCursor = 0;
        }

        newCursor += (m_cursorType == CURSOR_TYPE_8
                       && newCursor == ADV_BOAT_EVENT_POINTER)
                          ? turns
                          : mouseOffset;
        g_mouseManager->setPointer(newCursor,
                                   MouseManager::ADVENTURE_SET);
        return 1;
        }
        }
    } else {
        if (g_mouseManager->m_frame < HOVER_SCROLL_POINTER_FIRST
            || g_mouseManager->m_frame > HOVER_SCROLL_POINTER_LAST
            || !mouseInScrollZone())
            g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);
        m_advWindow->processHover(mouseX, mouseY);
    }
    return 1;
}

VA(0x0040ec80, 0xA)  // dc 0xfd6c
void AdvManager::reseed(int targetX, int targetY)
{
    m_seedingValid = 0;
}

// E:\gamedcs\advmgr.cpp:4840
// RETAIL-RECONSTRUCTED 2026-08-09. Retail proves the complete dig command:
// movement/backpack gates, map-cell eligibility, Grail award, sound/dialog
// split, every player's puzzle refresh and the post-action route/button cleanup.

// 91.44 -> 92.19 (2026-08-20): the frame sweep's 0x38-vs-0x30 pointed at
// the two artifact records. Both are TWO-ARG CTOR declarations at their
// use sites, not default-then-assign: a top-level `type_artifact grail;`
// runs the header's defaulting ctor, and our compile spilled {-1,-1} into
// two slots at ENTRY (`or edi,-1` + two stores) and CSE'd that -1 into
// GetCurrHero's `cmp edx,edi` - retail compares the IMMEDIATE and writes
// each record exactly twice at its use site ({2,-1} at [ebp-0x2c] for
// grail, [ebp-0x24] for describedGrail). `type_artifact grail(
// ARTIFACT_HOLY_GRAIL, -1)` inside the award arm is the faithful form.

// Residual (92.19%): flow-distance 0; a whole-body EBX/EDI role swap
// (currHero edi on retail, ebx ours; z the reverse) why-reg --model
// proves is C2 handle state - creation order agrees on both sides, the
// permutation is not source-reachable, capped after one compile. The
// frame stays 0x38 vs 0x30: our two records do not share slots with the
// description string temp the way retail packs them.

// 94.68 -> 96.15 (polish 49, lever A census): the CheckDimNextHeroBut tail
// is the SAME longhand shape DoAdvCommand already carries. Retail loads
// gpAdvManager into ESI at 0x40f1e0, ahead of BOTH IsLocalHuman and
// HasMobileHero, and reads [esi+0x44] in each arm; two source reads of the
// global cannot survive two calls, so the manager is hoisted into a local
// here exactly as at advmgr.cpp:2235.
VA(0x0040ec90, 0x5AD)  // anchor-callee, dc 0xfd84
int AdvManager::processSearch(int x, int y, int z)
{
    Hero* currHero;
    SAMPLE2 digSample;
    int player;
    NewmapCell* currCell;

    currHero = g_game->getCurrHero();

    if (currHero->m_movePoints != currHero->m_maxMovePoints) {
        if (!g_currentPlayer->isHuman()) {
            type_point invalidPoint(-1, -1, -1);
            g_currentPlayer->m_puzzleGuess = invalidPoint;
            return 1;
        }
        normalDialog(g_generalText->getText(
                         GENERAL_TEXT_SEARCH_NEEDS_FULL_MOVE),
                     1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return 1;
    }

    if (currHero->getNumberInBackpack(true)
            == HERO_BACKPACK_CAPACITY) {
        if (!g_currentPlayer->isHuman()) {
            type_point invalidPoint(-1, -1, -1);
            g_currentPlayer->m_puzzleGuess = invalidPoint;
            return 1;
        }
        normalDialog(g_generalText->getText(
                         GENERAL_TEXT_SEARCH_BACKPACK_FULL),
                     1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return 1;
    }

    mobilizeCurrHero(0, 0, 0);

    if (x == -1) {
        x = currHero->m_x;
        y = currHero->m_y;
        z = currHero->m_z;
    }

    type_point point(x, y, z);

    type_point lookupPoint = point;
    if (!lookupPoint.isValid())
        currCell = m_fullMap->cell(0, 0, 0);
    else
        currCell = m_fullMap->cell(lookupPoint.m_x, lookupPoint.m_y, lookupPoint.m_z);

    if (!g_currentPlayer->isHuman() && !currCell->isDiggable()) {
        type_point invalidPoint(-1, -1, -1);
        g_currentPlayer->m_puzzleGuess = invalidPoint;
        return 1;
    }

    if (currCell->m_groundSet == eTerrainWater) {
        normalDialog(g_generalText->getText(GENERAL_TEXT_SEARCH_WATER),
                     1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return 1;
    }

    if (!currCell->isDiggable()) {
        normalDialog(g_generalText->getText(
                         GENERAL_TEXT_SEARCH_NOT_DIGGABLE),
                     1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return 1;
    }

    if (g_currentPlayer->isHuman())
        digSample = loadPlaySample(DATA_COMPGEN(
            0x00660378, processSearchDigSample, "DIGSOUND.82M"));

    g_game->insertObject(x, y, z, TERRAIN_HOLE, -1, -1);

    if (g_game->m_ultimateArtifactX == x
        && g_game->m_ultimateArtifactY == y
        && g_game->m_ultimateArtifactZ == z
        && g_game->m_ultimateArtifactPresent) {
        if (currHero->getNumberInBackpack(true)
                >= HERO_BACKPACK_CAPACITY) {
            if (g_currentPlayer->isHuman())
                normalDialog(
                    g_generalText->getText(
                        GENERAL_TEXT_SEARCH_BACKPACK_FULL_FOUND),
                    1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        } else {
            type_artifact grail(ARTIFACT_HOLY_GRAIL);

            if (g_currentPlayer->isHuman()) {
                g_unnamed69950c = g_netLocalGamePos;
                launchSample(DATA_COMPGEN(
                                  0x00660360, processSearchGrailSample,
                                  "UltimateArtifact.wav"),
                              -1, 3);
                sprintf(g_text,
                        DATA_COMPGEN(0x00660358,
                                     processSearchFoundFormat, "%s%s"),
                        g_generalText->getText(GENERAL_TEXT_SEARCH_FOUND_FORMAT),
                        g_artifactTraits[ARTIFACT_HOLY_GRAIL].m_name);
                normalDialog(g_text, 1, -1, -1, -1, 0, -1, 0,
                             -1, 0, -1, 0);

                type_artifact describedGrail(ARTIFACT_HOLY_GRAIL);
                std::string description = describedGrail.getDescription();
                normalDialog(description.c_str(), 1, -1, -1, -1, 0,
                             -1, 0, -1, 0, -1, 0);
            }

            g_soundManager->switchAmbientMusic(g_terrainMusicIds[m_lastTerrain]);
            currHero->giveArtifact(&grail, 1, 1);
            g_game->m_ultimateArtifactPresent = 0;
        }
    } else if (g_currentPlayer->isHuman()) {
        normalDialog(g_generalText->getText(
                         GENERAL_TEXT_SEARCH_NOTHING_FOUND),
                     1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    }

    if (g_currentPlayer->isHuman())
        waitEndSample(digSample, -1);

    for (player = 0; player < 8; player++) {
        if (!g_game->m_playerDisabled[player])
            computeUALoc(player);
    }

    currHero->m_movePoints = 0;
    updBottomView(1, 1, 1);

    if (g_currentPlayer->isLocalHuman()
        && g_currentPlayer->m_currHeroId != -1
        && !g_game->m_heroes[g_currentPlayer->m_currHeroId].isMobile()) {
        showRoute(1, 0, 0);
        g_advManager->m_advWindow->updateHeroLocators(-1, 1, 1);

        AdvManager* dimTarget = g_advManager;
        if (g_currentPlayer->isLocalHuman()
            && g_currentPlayer->hasMobileHero())
            dimTarget->m_advWindow->widgetClearStatus(
                11, Widget::WIDGET_UPDATE | Widget::WIDGET_DIMMED);
        else
            dimTarget->m_advWindow->widgetSetStatus(
                11, Widget::WIDGET_UPDATE | Widget::WIDGET_DIMMED);
    }

    m_seedingValid = 0;
    return 1;
}

VA(0x0040f270, 0x7D)  // dc 0x10520
void AdvManager::updateScreen(int allowIntermediateMouse, int forceDraw)
{
    g_windowManager->updateScreen(ADVENTURE_SCREEN_X, ADVENTURE_SCREEN_Y,
                                  ADVENTURE_SCREEN_WIDTH,
                                  ADVENTURE_SCREEN_HEIGHT);

    unsigned long curTime = GameTime::get();
    if (static_cast<long>(
            curTime - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT]) >= 0
        && !m_animCtrPaused) {
        ++m_animCtr;
        unsigned long elapsedTime =
            curTime - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT];
        g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT] +=
            max(ADVENTURE_ANIMATION_MAX_ELAPSED, elapsedTime);
    }
    process1WindowsMessage();
}

VA(0x0040f2f0, 0xF8)  // dc 0x10640
void AdvManager::drawAdventureMapGems()
{
    int player = g_game->getLocalPlayerGamePos();
    m_gemIcons[0]->draw(0, player, 0, 0, 46, 46,
                      g_windowManager->m_screenBitmap, 6, 6, 0, 1);
    m_gemIcons[1]->draw(0, player, 0, 0, 46, 46,
                      g_windowManager->m_screenBitmap, 556, 6, 0, 1);
    m_gemIcons[2]->draw(0, player, 0, 0, 46, 46,
                      g_windowManager->m_screenBitmap, 6, 508, 0, 1);
    m_gemIcons[3]->draw(0, player, 0, 0, 46, 46,
                      g_windowManager->m_screenBitmap, 556, 508, 0, 1);
}

VA(0x0040f3f0, 0x47D)  // dc 0x10788
void AdvManager::completeDraw(int startX, int startY, int z, unsigned char forceDraw, unsigned char updateBottomView)
{
    pollSound();

    if (!forceDraw && !g_completeDrawEnabled && !g_videoPaused)
        return;

    if (g_videoPaused) {
        CNetMsgHandler* messageHandler =
            g_dPlay->getNetMsgHandler();
        if (messageHandler && messageHandler->isInPopup()
            && !g_completeDrawMessageBypass && !g_inViewWorld)
            return;
    }

    if (g_completeDrawAllCells) {
        startY = 0;
        startX = 0;
    }

    m_cursorDrawn = 0;

    int drawheight = COMPLETE_DRAW_LAST_Y;
    int drawwidth = COMPLETE_DRAW_LAST_X;
    int x;
    int y;

    for (y = -1; y <= drawheight; ++y) {
        for (x = -1; x <= drawwidth; ++x)
            drawGround(startX + x, startY + y, z, x, y);
    }

    if (!g_inViewWorld) {
        for (y = -1; y <= drawheight; ++y) {
            for (x = -1; x <= drawwidth; ++x)
                drawRiver(startX + x, startY + y, z, x, y);
        }

        for (y = -1; y <= drawheight; ++y) {
            for (x = -1; x <= drawwidth; ++x)
                drawRoad(startX + x, startY + y, z, x, y);
        }

        for (y = -1; y <= drawheight; ++y) {
            for (x = -1; x <= drawwidth; ++x)
                drawUnderlay(startX + x, startY + y, z, x, y);
        }
    }

    for (y = -1; y <= drawheight; ++y) {
        for (x = -1; x <= drawwidth; ++x)
            drawAdvObjShadow(startX + x, startY + y, z, x, y);
    }

    if (m_showRoute && !g_inViewWorld) {
        for (y = -1; y <= drawheight; ++y) {
            for (x = -1; x <= drawwidth; ++x)
                drawArrowShadow(startX + x, startY + y, z, x, y);
        }
    }

    for (y = -1; y <= drawheight; ++y) {
        for (x = -1; x <= drawwidth; ++x)
            drawAdvObj(startX + x, startY + y, z, x, y);
    }

    if (m_showRoute && !g_inViewWorld) {
        for (y = -1; y <= drawheight; ++y) {
            for (x = -1; x <= drawwidth; ++x)
                drawArrow(startX + x, startY + y, z, x, y);
        }
    }

    if (g_currentPlayer->m_currHeroId == -1)
        m_drawCursor = false;
    if (m_drawCursor)
        drawAdventureCursor();

    if (!g_inViewWorld) {
        for (y = -1; y <= drawheight; ++y) {
            for (x = -1; x <= drawwidth; ++x)
                drawShroud(startX + x, startY + y, z, x, y);
        }
    }

    drawAdventureMapGems();

    if (m_debugShowFps) {
        if (g_completeDrawFpsFrame >= 0) {
            unsigned long currentTime = GameTime::get();
            g_completeDrawFpsTimes[g_completeDrawFpsFrame] =
                currentTime - g_completeDrawFpsLastTime;
            int elapsedTime = 0;
            for (int frame = 0; frame < COMPLETE_DRAW_FPS_FRAME_COUNT;
                 ++frame)
                elapsedTime += g_completeDrawFpsTimes[frame];
            sprintf(g_completeDrawFpsText, g_completeDrawFpsFormat,
                    100.0 / (elapsedTime * 0.001));
            g_completeDrawFpsLastTime = currentTime;
        } else {
            g_completeDrawFpsLastTime = GameTime::get();
        }

        g_completeDrawFpsFrame = (g_completeDrawFpsFrame + 1)
                                % COMPLETE_DRAW_FPS_FRAME_COUNT;
        g_chatMan.clearChat();
        updateCompleteDrawFps(&g_chatMan, g_completeDrawFpsText);
    }

    pollSound();

    if (!g_inViewWorld) {
        g_chatMan.updateWidget(m_advWindow->m_chatTextWidget, true, 20);
        m_advWindow->drawChatText(false);
    }

    if (updateBottomView)
        updBottomView(0, true, true);
}

VA(0x0040f870, 0x43)  // dc 0x10c9c
void AdvManager::completeDraw(unsigned char forceDraw)
{
    completeDraw(m_radarOrigin.m_x, m_radarOrigin.m_y, m_radarOrigin.m_z,
                 forceDraw, true);
}

VA(0x0040f8c0, 0x265)  // dc 0x10cf4
int AdvManager::getCloudLookup(int srcX, int srcY, int z)
{
    int lookup = 0;

    if (srcX < 1)
        lookup = 0xc8;
    else if (srcX >= g_mapWidth - 1)
        lookup = 0x32;

    if (srcY < 1)
        lookup |= 0x91;
    else if (srcY >= g_mapHeight - 1)
        lookup |= 0x64;

    if (!lookup) {
        if (!(getMapExtra(srcX, srcY - 1, z) & g_mapVisibilityBit))
            lookup = 1;
        if (!(getMapExtra(srcX + 1, srcY, z) & g_mapVisibilityBit))
            lookup |= 2;
        if (!(getMapExtra(srcX, srcY + 1, z) & g_mapVisibilityBit))
            lookup |= 4;
        if (!(getMapExtra(srcX - 1, srcY, z) & g_mapVisibilityBit))
            lookup |= 8;
        if (!(getMapExtra(srcX + 1, srcY - 1, z) & g_mapVisibilityBit))
            lookup |= 0x10;
        if (!(getMapExtra(srcX + 1, srcY + 1, z) & g_mapVisibilityBit))
            lookup |= 0x20;
        if (!(getMapExtra(srcX - 1, srcY + 1, z) & g_mapVisibilityBit))
            lookup |= 0x40;
        if (!(getMapExtra(srcX - 1, srcY - 1, z) & g_mapVisibilityBit))
            lookup |= 0x80;
    } else {
        if (!(lookup & 1)
            && !(getMapExtra(srcX, srcY - 1, z) & g_mapVisibilityBit))
            lookup |= 1;
        if (!(lookup & 2)
            && !(getMapExtra(srcX + 1, srcY, z) & g_mapVisibilityBit))
            lookup |= 2;
        if (!(lookup & 4)
            && !(getMapExtra(srcX, srcY + 1, z) & g_mapVisibilityBit))
            lookup |= 4;
        if (!(lookup & 8)
            && !(getMapExtra(srcX - 1, srcY, z) & g_mapVisibilityBit))
            lookup |= 8;
        if (!(lookup & 0x10)
            && !(getMapExtra(srcX + 1, srcY - 1, z) & g_mapVisibilityBit))
            lookup |= 0x10;
        if (!(lookup & 0x20)
            && !(getMapExtra(srcX + 1, srcY + 1, z) & g_mapVisibilityBit))
            lookup |= 0x20;
        if (!(lookup & 0x40)
            && !(getMapExtra(srcX - 1, srcY + 1, z) & g_mapVisibilityBit))
            lookup |= 0x40;
        if (!(lookup & 0x80)
            && !(getMapExtra(srcX - 1, srcY - 1, z) & g_mapVisibilityBit))
            lookup |= 0x80;
    }

    return g_cloudType[lookup];
}

VA(0x0040fb30, 0x167)  // dc 0x110c0
bool AdvManager::scanForHeroOrBoat(int srcX, int srcY, int z,
                                   unsigned short type,
                                   DrawParts (&parts)[6])
{
    if (g_inViewWorld)
        return false;

    int partNum = 0;
    bool found = false;
    for (int cy = 0; cy < 2; ++cy) {
        for (int cx = -1; cx < 2; ++cx) {
            if (srcX + cx >= 0 && srcY + cy >= 0
                && srcX + cx < g_mapWidth && srcY + cy < g_mapHeight) {
                NewmapCell* tempCell =
                    getCell(type_point(srcX + cx, srcY + cy, z));
                if (tempCell->m_type == type && tempCell->m_isTrigger
                    && tempCell->m_extraInfo != ~0UL) {
                    parts[partNum].m_isValid = true;
                    parts[partNum].m_x = cx;
                    parts[partNum].m_y = cy;
                    parts[partNum].m_id = tempCell->m_extraInfo;
                    found = true;
                }
            }
            ++partNum;
        }
    }
    return found;
}

VA(0x0040fca0, 0x7A)  // dc 0x11238
bool hasFlag(int objType)
{
    switch (objType) {
    case CREATURE_GENERATOR_1:
    case CREATURE_GENERATOR_4:
    case GARRISON:
    case LIGHTHOUSE:
    case MINE:
    case RANDOM_TOWN:
    case SHIPYARD:
    case TOWN:
        return true;
    default:
        return false;
    }
}

VA(0x0040fd20, 0x10E)  // dc 0x1129c
int getFlaggedObjectOwner(NewmapCell* thisCell)
{
    AdventureObjectType type = thisCell->m_type;
    int extraInfo = thisCell->m_extraInfo;
    int owner = -1;

    if (type == HERO) {
        Hero* thisHero;
        if (extraInfo == -1)
            thisHero = 0;
        else
            thisHero = &g_game->m_heroes[extraInfo];
        type = thisHero->m_obscuredType;
        extraInfo = thisHero->getObscuredExtraInfo();
    }

    switch (type) {
    case RANDOM_TOWN:
    case TOWN:
        owner = g_game->m_towns[extraInfo].m_owner;
        break;
    case LIGHTHOUSE:
    case MINE:
        owner = g_game->m_mines[extraInfo].m_playerOwner;
        break;
    case GARRISON:
        owner = g_game->m_garrisons[extraInfo].m_playerOwner;
        break;
    case CREATURE_GENERATOR_1:
    case CREATURE_GENERATOR_4:
        owner = g_game->m_generators[extraInfo].getOwner();
        break;
    case SHIPYARD:
        owner = extraInfo << 24 >> 24;
        break;
    }

    return owner;
}

VA(0x0040fe30, 0x484)  // dc 0x11424
void AdvManager::drawHeroPart(int part, DrawParts& heroParts, int baseX,
                              int baseY, int tilex, int tiley, int tilew,
                              int tileh)
{
    Hero* currHero = g_game->getHero(heroParts.m_id);

    int heroCellY = part % 3;
    int heroCellX = part / 3;

    if (currHero->m_flags & 0x40000) {
        Boat* currBoat = g_game->getHeroBoat(currHero->m_id, true);
        NewmapCell* heroCell = getCell(currHero->getLocation());

        if (!(heroCell->m_flags0011 & 0x200)) {
            m_boatFrothIcons[currBoat->m_type]->drawHero(
                currHero->getStandSequence(),
                m_animCtr
                    % m_boatFrothIcons[currBoat->m_type]->getNumFrames(hs_stand_n),
                tilex + (2 - heroCellY) * 32,
                tiley - heroCellX * 32 + 32, tilew, tileh,
                g_windowManager->m_screenBitmap, baseX, baseY + 8,
                currHero->getHflip());
        }

        m_boatFlagIcons[currBoat->m_type][currBoat->m_playerOwner]->drawHero(
            currHero->getStandSequence(),
            m_animCtr % m_boatFlagIcons[currBoat->m_type][currBoat->m_playerOwner]
                                ->getNumFrames(hs_stand_n),
            tilex + (2 - heroCellY) * 32,
            tiley - heroCellX * 32 + 32, tilew, tileh,
            g_windowManager->m_screenBitmap, baseX, baseY + 8,
            currHero->getHflip());

        m_boatIcons[currBoat->m_type]->drawHero(
            currHero->getStandSequence(),
            m_animCtr
                % m_boatIcons[currBoat->m_type]->getNumFrames(hs_stand_n),
            tilex + (2 - heroCellY) * 32,
            tiley - heroCellX * 32 + 32, tilew, tileh,
            g_windowManager->m_screenBitmap, baseX, baseY + 8,
            currHero->getHflip());
    } else if (currHero->m_owner >= 0 && currHero->m_owner < 8) {
        m_flagIcons[currHero->m_owner]->drawHero(
            currHero->getStandSequence(),
            m_animCtr % m_flagIcons[currHero->m_owner]->getNumFrames(hs_stand_n),
            tilex + (2 - heroCellY) * 32,
            tiley - heroCellX * 32 + 32, tilew, tileh,
            g_windowManager->m_screenBitmap, baseX, baseY + 8,
            currHero->getHflip());

        m_cursorIcons[currHero->m_heroClass]->drawHero(
            currHero->getStandSequence(),
            m_animCtr
                % m_cursorIcons[currHero->m_heroClass]->getNumFrames(hs_stand_n),
            tilex + (2 - heroCellY) * 32,
            tiley - heroCellX * 32 + 32, tilew, tileh,
            g_windowManager->m_screenBitmap, baseX, baseY + 8,
            currHero->getHflip());
    }
}

VA(0x004102c0, 0x494)  // dc 0x11958
void AdvManager::drawHeroPartShadow(int part, DrawParts& heroParts,
                                    int baseX, int baseY, int tilex,
                                    int tiley, int tilew, int tileh)
{
    Hero* currHero = g_game->getHero(heroParts.m_id);

    int heroCellY = part % 3;
    int heroCellX = part / 3;

    if (currHero->m_flags & 0x40000) {
        if (currHero->m_owner < 0 || currHero->m_owner >= 8)
            return;

        Boat* currBoat = g_game->getHeroBoat(currHero->m_id, true);
        NewmapCell* heroCell = getCell(currHero->getLocation());

        if (!(heroCell->m_flags0011 & 0x200)) {
            m_boatFrothIcons[currBoat->m_type]->drawHeroShadow(
                currHero->getStandSequence(),
                m_animCtr
                    % m_boatFrothIcons[currBoat->m_type]->getNumFrames(hs_stand_n),
                tilex + (2 - heroCellY) * 32,
                tiley - heroCellX * 32 + 32, tilew, tileh,
                g_windowManager->m_screenBitmap, baseX, baseY + 8,
                currHero->getHflip());
        }

        m_boatFlagIcons[currBoat->m_type][currBoat->m_playerOwner]->drawHeroShadow(
            currHero->getStandSequence(),
            m_animCtr % m_boatFlagIcons[currBoat->m_type][currBoat->m_playerOwner]
                                ->getNumFrames(hs_stand_n),
            tilex + (2 - heroCellY) * 32,
            tiley - heroCellX * 32 + 32, tilew, tileh,
            g_windowManager->m_screenBitmap, baseX, baseY + 8,
            currHero->getHflip());

        m_boatIcons[currBoat->m_type]->drawHeroShadow(
            currHero->getStandSequence(),
            m_animCtr
                % m_boatIcons[currBoat->m_type]->getNumFrames(hs_stand_n),
            tilex + (2 - heroCellY) * 32,
            tiley - heroCellX * 32 + 32, tilew, tileh,
            g_windowManager->m_screenBitmap, baseX, baseY + 8,
            currHero->getHflip());
    } else if (currHero->m_owner >= 0 && currHero->m_owner < 8) {
        m_flagIcons[currHero->m_owner]->drawHeroShadow(
            currHero->getStandSequence(),
            m_animCtr % m_flagIcons[currHero->m_owner]->getNumFrames(hs_stand_n),
            tilex + (2 - heroCellY) * 32,
            tiley - heroCellX * 32 + 32, tilew, tileh,
            g_windowManager->m_screenBitmap, baseX, baseY + 8,
            currHero->getHflip());

        m_cursorIcons[currHero->m_heroClass]->drawHeroShadow(
            currHero->getStandSequence(),
            m_animCtr
                % m_cursorIcons[currHero->m_heroClass]->getNumFrames(hs_stand_n),
            tilex + (2 - heroCellY) * 32,
            tiley - heroCellX * 32 + 32, tilew, tileh,
            g_windowManager->m_screenBitmap, baseX, baseY + 8,
            currHero->getHflip());
    }
}

VA(0x00410760, 0x24F)  // dc 0x11ea4
void AdvManager::drawBoatPart(int part, DrawParts& boatParts, int baseX,
                              int baseY, int tilex, int tiley, int tilew,
                              int tileh)
{
    Boat* currBoat = g_game->getBoat(boatParts.m_id);
    int boatCellY = part % 3;
    int boatCellX = part / 3;
    NewmapCell* boatCell = getCell(
        currBoat->getLocation());

    if (!(boatCell->m_flags0011 & 0x200)) {
        m_boatFrothIcons[currBoat->m_type]->drawHero(
            currBoat->getStandSequence(),
            m_animCtr
                % m_boatFrothIcons[currBoat->m_type]->getNumFrames(hs_stand_n),
            tilex + (2 - boatCellY) * 32,
            tiley - boatCellX * 32 + 32, tilew, tileh,
            g_windowManager->m_screenBitmap, baseX, baseY + 8,
            currBoat->m_facing > Hero::kFacingS);
    }

    m_boatIcons[currBoat->m_type]->drawHero(
        currBoat->getStandSequence(),
        m_animCtr % m_boatIcons[currBoat->m_type]->getNumFrames(hs_stand_n),
        tilex + (2 - boatCellY) * 32,
        tiley - boatCellX * 32 + 32, tilew, tileh,
        g_windowManager->m_screenBitmap, baseX, baseY + 8,
        currBoat->m_facing > Hero::kFacingS);
}

VA(0x004109b0, 0x24F)  // dc 0x120ec
void AdvManager::drawBoatPartShadow(int part, DrawParts& boatParts,
                                    int baseX, int baseY, int tilex,
                                    int tiley, int tilew, int tileh)
{
    Boat* currBoat = g_game->getBoat(boatParts.m_id);
    int boatCellY = part % 3;
    int boatCellX = part / 3;
    NewmapCell* boatCell = getCell(
        currBoat->getLocation());

    if (!(boatCell->m_flags0011 & 0x200)) {
        m_boatFrothIcons[currBoat->m_type]->drawHeroShadow(
            currBoat->getStandSequence(),
            m_animCtr
                % m_boatFrothIcons[currBoat->m_type]->getNumFrames(hs_stand_n),
            tilex + (2 - boatCellY) * 32,
            tiley - boatCellX * 32 + 32, tilew, tileh,
            g_windowManager->m_screenBitmap, baseX, baseY + 8,
            currBoat->m_facing > Hero::kFacingS);
    }

    m_boatIcons[currBoat->m_type]->drawHeroShadow(
        currBoat->getStandSequence(),
        m_animCtr % m_boatIcons[currBoat->m_type]->getNumFrames(hs_stand_n),
        tilex + (2 - boatCellY) * 32,
        tiley - boatCellX * 32 + 32, tilew, tileh,
        g_windowManager->m_screenBitmap, baseX, baseY + 8,
        currBoat->m_facing > Hero::kFacingS);
}

// E:\gamedcs\advmgr.cpp:5941
// Residual (87.5901%): BOUNDED, and the model proves it (2026-08-21). The
// whole delta is one callee-saved role swap - retail holds `this` in ESI and
// the object-list pointer in EDI, we hold them the other way round - plus the
// four-byte frame shift that follows it. Flow-distance 2, 125 blocks against
// retail's 125, identical call multiset. `why-reg --model` reads the SAME
// three definition slots in the SAME order on both sides (#0@8, #1@17, #2@21)
// and reports the only lever as "make `this` the first-created call-crossing
// pseudo"; `this` is minted between the parameters and the body locals, so no
// declaration, include or spelling change can create a local's handle before
// it. That is C2-side handle STATE (catalog C1 class), not handle order, and
// it is not source-reachable. The one candidate the model still compiled
// (swapping the baseX/baseY declarations) measured +8 distance, no
// improvement. DrawAdvObjShadow below carries the identical wall.
// 2026-09-06, polish lane 36 (87.5901 -> 87.9441), the DC LOCAL-SCOPE SWEEP,
// and it PARTLY REFUTES the paragraph above: a source knob does move this
// row.  The Dreamcast block names `Obj` (CodeView 0x30b6 = `CObject*`,
// sp+0x9c) beside ObjCell/ObjType/SprPtr, i.e. the map's object row is
// addressed ONCE through a named pointer and both `typeIndex` reads go
// through it, where this body subscripted `mapObjects->objects[...]` twice.
// The other three names in that group are renames this body already has
// (ObjCell = objCell, ObjType = objType, SprPtr = sprite).  The `this`
// ESI/EDI permutation the note above describes is unchanged; this was the
// last missing named local, not a fix for it.
// 2026-09-06, polish lane 38, the DC TYPE-RECORD sweep: the block types both
// of this loop nest's counters T_INT4 where they were written `unsigned`.
// `numObj` is byte-flat as `int` and is taken (the sibling nests in this file
// and in viewwrld already spell it that way); `row` as `int` COSTS 0.06
// (87.9441 -> 87.8809, alone or together with numObj) because the layer
// compare against the byte member goes signed, so it stays `unsigned`.
// Canonical map types: current 87.9441 -> 87.7661. Disposable Gruntz
// forests (seed 20260906, baseline + 16 variants before includes and
// before this function) retain 87.7661 in all 34 trials. No probe noise
// is retained; these two search placements do not recover the loss.
VA(0x00410c00, 0x98E)  // anchor-callee, dc 0x12334
void AdvManager::drawAdvObj(int srcX, int srcY, int z, int destX, int destY)
{
    if (srcX < 0 || srcY < 0 || srcX >= g_mapWidth || srcY >= g_mapHeight)
        return;

    NewmapCell* thisCell;
    if (z >= 0)
        thisCell = m_fullMap->cell(srcX, srcY, z);
    else
        thisCell = m_fullMap->cell(0, 0, 0);

    int baseX = m_scrollX + destX * 32;
    int baseY = m_scrollY + destY * 32;
    int tilex = 0;
    int tiley = 0;
    int tilew = 32;
    int tileh = 32;

    if (baseX < 8) {
        tilex = 8 - baseX;
        tilew = baseX + 24;
        baseX = 8;
    }
    if (baseY < 0) {
        tiley = -baseY;
        tileh = baseY + 32;
        baseY = 0;
    }
    if (baseX + tilew > 600)
        tilew = 600 - baseX;
    if (baseY + tileh > 544)
        tileh = 544 - baseY;
    if (tilew <= 0 || tileh <= 0)
        return;

    DrawParts heroParts[6];
    DrawParts boatParts[6];
    bool foundHero = scanForHeroOrBoat(srcX, srcY, z, HERO, heroParts);
    bool foundBoat = scanForHeroOrBoat(srcX, srcY, z, BOAT, boatParts);

    NewmapCell* cellObjects = thisCell;
    if (cellObjects->m_objects.size() > 0) {
        for (int row = 0; row <= OBJECT_DRAW_LAYER_LAST; ++row) {
            for (int numObj = 0; numObj < cellObjects->m_objects.size();
                 ++numObj) {
                NewmapCell::ObjectCell* objCell = &cellObjects->m_objects[numObj];
                if (objCell->m_layer != row)
                    continue;

                NewfullMap* mapObjects = m_fullMap;
                CObject* obj = &mapObjects->m_objects[objCell->m_objectIndex];
                CObjectType* objType =
                    &mapObjects->m_objectTypes[obj->m_typeIndex];
                CSprite* sprite = mapObjects->m_sprites[obj->m_typeIndex];
                // THE OFFSETS ARE RE-DERIVED PER DRAW ARM, not hoisted
                // (86.3772 -> 87.5901, 2026-08-19). Retail recomputes
                // `movsx ecx,dl / sar ecx,4` and then `shl dl,4 / movsx /
                // sar` at EVERY call site, which is why its body carried
                // eight more sar and eight more movsx than a single hoisted
                // pair can produce; with the four arms spelling their own,
                // both counts match exactly. This copy survives only for the
                // drawCells bit and is named for that.

                // NOT a family rule - measured, not assumed. Shadow's old
                // isolated re-derivation loss was superseded once its cases
                // were grouped into retail's selector-table source shape;
                // that function now keeps per-arm copies too. DrawUnderlay's
                // arms already spell their own.

                // Residual (87.5901%): a whole-function callee-saved
                // permutation - retail holds `this` in ESI and this compile
                // holds it in EDI, and every downstream binding follows.
                // Schedule-aligned, flow-distance 2 of 125 blocks, and both
                // sides emit the same two jump tables (checked directly: the
                // diff's apparent table-only-on-our-side is an alignment
                // artifact). No source knob reaches a `this` register
                // choice; it is the B-family wall the campaign prices at
                // ~0 closures. The DC roster's two scoped triples named
                // `part`, `partHigh`, and `partLow` are now restored in both
                // hero/boat loops below; they compile byte-flat at 87.5901,
                // closing the last missing-local hypothesis without changing
                // the allocator wall.
                signed char bitOffsets = objCell->m_offsets;
                signed char bitY = bitOffsets >> 4;
                bitOffsets <<= 4;
                signed char bitX = bitOffsets >> 4;
                int bit = 47 - bitY * 8 - bitX;
                if (!objType->m_drawCells[bit] || objType->m_suppressDraw)
                    continue;

                if (g_inViewWorld) {
                    switch (objType->m_objectType) {
                    case TERRAIN_BRUSH:             break;
                    case TERRAIN_BUSH:              break;
                    case TERRAIN_CACTUS:            break;
                    case TERRAIN_CANYON:            break;
                    case TERRAIN_CRATER:            break;
                    case TERRAIN_DEAD_VEGETATION:   break;
                    case TERRAIN_FLOWER:            break;
                    case TERRAIN_FROZEN_LAKE:       break;
                    case TERRAIN_HEDGE:             break;
                    case TERRAIN_HILL:              break;
                    case TERRAIN_HOLE:              continue;
                    case TERRAIN_KELP:              break;
                    case TERRAIN_LAKE:              break;
                    case TERRAIN_LAVA_FLOW:         break;
                    case TERRAIN_LAVA_LAKE:         break;
                    case TERRAIN_MUSHROOM:          break;
                    case TERRAIN_LOG:               break;
                    case TERRAIN_MANDRAKE:          break;
                    case TERRAIN_MOSS:              break;
                    case TERRAIN_MOUND:             break;
                    case TERRAIN_MOUNTAIN:          break;
                    case TERRAIN_OAK_TREE:          break;
                    case TERRAIN_OUTCROPPING:       break;
                    case TERRAIN_PINE_TREE:         break;
                    case TERRAIN_PLANT:             break;
                    case TERRAIN_RIVER_1:           continue;
                    case TERRAIN_RIVER_2:           continue;
                    case TERRAIN_RIVER_3:           continue;
                    case TERRAIN_RIVER_4:           continue;
                    case TERRAIN_RIVER_DELTA:       continue;
                    case TERRAIN_ROAD_1:            continue;
                    case TERRAIN_ROAD_2:            continue;
                    case TERRAIN_ROAD_3:            continue;
                    case TERRAIN_ROCK:              break;
                    case TERRAIN_SAND_DUNE:         break;
                    case TERRAIN_SAND_PIT:          break;
                    case TERRAIN_SHRUB:             break;
                    case TERRAIN_SKULL:             break;
                    case TERRAIN_STALAGMITE:        break;
                    case TERRAIN_STUMP:             break;
                    case TERRAIN_TAR_PIT:           break;
                    case TERRAIN_TREE:              break;
                    case TERRAIN_VINE:              break;
                    case TERRAIN_VOLCANIC_VENT:     break;
                    case TERRAIN_VOLCANO:           break;
                    case TERRAIN_WILLOW_TREE:       break;
                    case TERRAIN_YUCCA_TREE:        break;
                    case TERRAIN_REEF:              break;
                    default:                        continue;
                    }

                    signed char offsets = objCell->m_offsets;
                    signed char yOffset = offsets >> 4;
                    offsets <<= 4;
                    signed char xOffset = offsets >> 4;
                    int frame = (m_animCtr
                                 + mapObjects->m_objects[objCell->m_objectIndex]
                                       .m_animationOffset)
                                % sprite->getNumFrames(0);
                    sprite->drawAdvObj(
                        frame,
                        tilex + (objType->m_width - xOffset - 1) * 32,
                        tiley + (objType->m_height - yOffset - 1) * 32,
                        tilew, tileh, g_windowManager->m_screenBitmap,
                        baseX, baseY + 8, false);
                } else {
                    switch (objType->m_objectType) {
                    case CREATURE_GENERATOR_1:
                    case CREATURE_GENERATOR_4:
                    case GARRISON:
                    case LIGHTHOUSE:
                    case MINE:
                    case RANDOM_TOWN:
                    case SHIPYARD:
                    case TOWN: {
                        int triggerX;
                        int triggerY;
                        mapObjects->m_objects[objCell->m_objectIndex].findTrigger(
                            triggerX, triggerY);
                        NewmapCell* triggerCell =
                            getCell(type_point(triggerX, triggerY, z));
                        int owner = getFlaggedObjectOwner(triggerCell);
                        signed char offsets = objCell->m_offsets;
                        signed char yOffset = offsets >> 4;
                        offsets <<= 4;
                        signed char xOffset = offsets >> 4;
                        int frame = (m_animCtr
                                     + mapObjects->m_objects[objCell->m_objectIndex]
                                           .m_animationOffset)
                                    % sprite->getNumFrames(0);
                        sprite->drawAdvObjWithFlag(
                            frame,
                            tilex + (objType->m_width - xOffset - 1) * 32,
                            tiley + (objType->m_height - yOffset - 1) * 32,
                            tilew, tileh, g_windowManager->m_screenBitmap,
                            baseX, baseY + 8,
                            g_unnamed6aacb0->m_data[64 + owner], false);
                        break;
                    }
                    default:
                        if (objCell->m_objectIndex == m_movingObjectIndex) {
                            signed char offsets = objCell->m_offsets;
                            signed char yOffset = offsets >> 4;
                            offsets <<= 4;
                            signed char xOffset = offsets >> 4;
                            m_movingObjectSprite->drawAdvObj(
                                m_movingObjectSequence * 2 + m_movingObjectFrame,
                                tilex + (objType->m_width - xOffset - 1) * 32,
                                tiley + (objType->m_height - yOffset - 1) * 32,
                                tilew, tileh, g_windowManager->m_screenBitmap,
                                baseX, baseY + 8, false);
                        } else {
                            signed char offsets = objCell->m_offsets;
                            signed char yOffset = offsets >> 4;
                            offsets <<= 4;
                            signed char xOffset = offsets >> 4;
                            int frame = (m_animCtr
                                         + mapObjects
                                               ->m_objects[objCell->m_objectIndex]
                                               .m_animationOffset)
                                        % sprite->getNumFrames(0);
                            sprite->drawAdvObj(
                                frame,
                                tilex + (objType->m_width - xOffset - 1) * 32,
                                tiley + (objType->m_height - yOffset - 1) * 32,
                                tilew, tileh, g_windowManager->m_screenBitmap,
                                baseX, baseY + 8, false);
                        }
                        break;
                    }
                }
            }

            if (foundHero || foundBoat) {
                int part;
                int partHigh;
                int partLow;
                if (row == OBJECT_DRAW_LAYER_HERO_FRONT) {
                    partLow = 0;
                    partHigh = 2;
                } else if (row == OBJECT_DRAW_LAYER_HERO_BACK) {
                    partLow = 3;
                    partHigh = 5;
                } else {
                    continue;
                }
                for (part = partLow; part <= partHigh; ++part) {
                    if (heroParts[part].m_isValid)
                        drawHeroPart(part, heroParts[part], baseX, baseY,
                                     tilex, tiley, tilew, tileh);
                    if (boatParts[part].m_isValid)
                        drawBoatPart(part, boatParts[part], baseX, baseY,
                                     tilex, tiley, tilew, tileh);
                }
            }

            if (row == OBJECT_DRAW_LAYER_HERO_BACK
                && destY == CURSOR_DEST_Y0 && m_drawCursor && !g_inViewWorld) {
                if (destX == CURSOR_DEST_X0)
                    drawCursor(0, 0);
                else if (destX == CURSOR_DEST_X1)
                    drawCursor(1, 0);
                else if (destX == CURSOR_DEST_X2)
                    drawCursor(2, 0);
            } else if (row == OBJECT_DRAW_LAYER_HERO_FRONT
                       && destY == CURSOR_DEST_Y1
                       && m_drawCursor && !g_inViewWorld) {
                if (destX == CURSOR_DEST_X0)
                    drawCursor(0, 1);
                else if (destX == CURSOR_DEST_X1)
                    drawCursor(1, 1);
                else if (destX == CURSOR_DEST_X2)
                    drawCursor(2, 1);
            }
        }
        return;
    }

    if (foundHero || foundBoat) {
        int part;
        int partHigh;
        int partLow;

        partLow = 0;
        partHigh = 5;
        for (part = partLow; part <= partHigh; ++part) {
            if (heroParts[part].m_isValid)
                drawHeroPart(part, heroParts[part], baseX, baseY,
                             tilex, tiley, tilew, tileh);
            if (boatParts[part].m_isValid)
                drawBoatPart(part, boatParts[part], baseX, baseY,
                             tilex, tiley, tilew, tileh);
        }
    }

    if (destY == CURSOR_DEST_Y0) {
        if (m_drawCursor && !g_inViewWorld) {
            if (destX == CURSOR_DEST_X0)
                drawCursor(0, 0);
            else if (destX == CURSOR_DEST_X1)
                drawCursor(1, 0);
            else if (destX == CURSOR_DEST_X2)
                drawCursor(2, 0);
        }
    } else if (destY == CURSOR_DEST_Y1) {
        if (m_drawCursor && !g_inViewWorld) {
            if (destX == CURSOR_DEST_X0)
                drawCursor(0, 1);
            else if (destX == CURSOR_DEST_X1)
                drawCursor(1, 1);
            else if (destX == CURSOR_DEST_X2)
                drawCursor(2, 1);
        }
    }
}

// E:\gamedcs\advmgr.cpp:6239
// Canonical map types: current 85.2335 -> 85.1872. The same 34
// disposable forest trials recorded beside drawAdvObj retain 85.1872
// for this function. No probe noise is retained; recovery remains open.
VA(0x00411590, 0x5E4)  // anchor-callee, dc 0x12fcc
void AdvManager::drawAdvObjShadow(int srcX, int srcY, int z, int destX, int destY)
{
    if (srcX < 0 || srcY < 0 || srcX >= g_mapWidth || srcY >= g_mapHeight)
        return;

    type_point point;
    point = type_point(srcX, srcY, z);
    NewmapCell* thisCell;
    unsigned char valid = point.isValid();
    NewfullMap* map = m_fullMap;
    if (!valid)
        thisCell = map->cell(0, 0, 0);
    else
        thisCell = map->cell(point.m_x, point.m_y, point.m_z);

    int baseX = m_scrollX + destX * 32;
    int baseY = m_scrollY + destY * 32;
    int tilex = 0;
    int tiley = 0;
    int tilew = 32;
    int tileh = 32;

    if (baseX < 8) {
        tilex = 8 - baseX;
        tilew = baseX + 24;
        baseX = 8;
    }
    if (baseY < 0) {
        tiley = -baseY;
        tileh = baseY + 32;
        baseY = 0;
    }
    if (baseX + tilew > 600)
        tilew = 600 - baseX;
    if (baseY + tileh > 544)
        tileh = 544 - baseY;
    if (tilew <= 0 || tileh <= 0)
        return;

    DrawParts heroParts[6];
    DrawParts boatParts[6];
    unsigned char foundHero =
        scanForHeroOrBoat(srcX, srcY, z, HERO, heroParts);
    unsigned char foundBoat =
        scanForHeroOrBoat(srcX, srcY, z, BOAT, boatParts);

    NewmapCell* cellObjects = thisCell;
    for (int numObj = 0; numObj < cellObjects->m_objects.size(); ++numObj) {
        NewmapCell::ObjectCell* objCell = &cellObjects->m_objects[numObj];
        NewfullMap* mapObjects = m_fullMap;
        CObjectType* objType = &mapObjects->m_objectTypes[
            mapObjects->m_objects[objCell->m_objectIndex].m_typeIndex];
        CSprite* sprite = mapObjects->m_sprites[
            mapObjects->m_objects[objCell->m_objectIndex].m_typeIndex];

        signed char offsets = objCell->m_offsets;
        // Retail keeps BOTH shifts 8-BIT and widens at use: `mov dl,al /
        // sar dl,4 / movsx edx,dl`, then `shl al,4 / sar al,4 / movsx`.
        // The packed offset is re-derived in each draw arm below, shortening
        // these locals' live range. Grouping the allowed terrain cases under
        // one shared break is equally material: it makes VC6 emit retail's
        // byte selector plus two-entry jump table instead of a 48-entry
        // pointer table. Together those two source shapes raise the max from
        // 83.2068% to 85.2335% and restore `this` in EDI.

        signed char yOffset = offsets >> 4;
        offsets <<= 4;
        signed char xOffset = offsets >> 4;
        int bit = -yOffset * 8 - xOffset + 47;
        if (!objType->m_shadowCells[bit] || objType->m_suppressDraw)
            continue;

        if (g_inViewWorld) {
            switch (objType->m_objectType) {
            case TERRAIN_BRUSH:
            case TERRAIN_BUSH:
            case TERRAIN_CACTUS:
            case TERRAIN_CANYON:
            case TERRAIN_CRATER:
            case TERRAIN_DEAD_VEGETATION:
            case TERRAIN_FLOWER:
            case TERRAIN_FROZEN_LAKE:
            case TERRAIN_HEDGE:
            case TERRAIN_HILL:
            case TERRAIN_KELP:
            case TERRAIN_LAKE:
            case TERRAIN_LAVA_FLOW:
            case TERRAIN_LAVA_LAKE:
            case TERRAIN_MUSHROOM:
            case TERRAIN_LOG:
            case TERRAIN_MANDRAKE:
            case TERRAIN_MOSS:
            case TERRAIN_MOUND:
            case TERRAIN_MOUNTAIN:
            case TERRAIN_OAK_TREE:
            case TERRAIN_OUTCROPPING:
            case TERRAIN_PINE_TREE:
            case TERRAIN_PLANT:
            case TERRAIN_ROCK:
            case TERRAIN_SAND_DUNE:
            case TERRAIN_SAND_PIT:
            case TERRAIN_SHRUB:
            case TERRAIN_SKULL:
            case TERRAIN_STALAGMITE:
            case TERRAIN_STUMP:
            case TERRAIN_TAR_PIT:
            case TERRAIN_TREE:
            case TERRAIN_VINE:
            case TERRAIN_VOLCANIC_VENT:
            case TERRAIN_VOLCANO:
            case TERRAIN_WILLOW_TREE:
            case TERRAIN_YUCCA_TREE:
            case TERRAIN_REEF:
                break;
            default:
                continue;
            }
            signed char drawOffsets = objCell->m_offsets;
            signed char drawY = drawOffsets >> 4;
            drawOffsets <<= 4;
            signed char drawX = drawOffsets >> 4;
            int frame = (m_animCtr
                         + mapObjects->m_objects[objCell->m_objectIndex]
                               .m_animationOffset)
                        % sprite->getNumFrames(0);
            sprite->drawAdvObjShadow(
                frame,
                tilex + (objType->m_width - drawX - 1) * 32,
                tiley + (objType->m_height - drawY - 1) * 32,
                tilew, tileh, g_windowManager->m_screenBitmap,
                baseX, baseY + 8, false);
        } else {
            if (objCell->m_objectIndex == m_movingObjectIndex) {
                signed char drawOffsets = objCell->m_offsets;
                signed char drawY = drawOffsets >> 4;
                drawOffsets <<= 4;
                signed char drawX = drawOffsets >> 4;
                m_movingObjectSprite->drawAdvObjShadow(
                    m_movingObjectSequence * 2 + m_movingObjectFrame,
                    tilex + (objType->m_width - drawX - 1) * 32,
                    tiley + (objType->m_height - drawY - 1) * 32,
                    tilew, tileh, g_windowManager->m_screenBitmap,
                    baseX, baseY + 8, false);
            } else {
                signed char drawOffsets = objCell->m_offsets;
                signed char drawY = drawOffsets >> 4;
                drawOffsets <<= 4;
                signed char drawX = drawOffsets >> 4;
                int frame = (m_animCtr
                             + mapObjects->m_objects[objCell->m_objectIndex]
                                   .m_animationOffset)
                            % sprite->getNumFrames(0);
                sprite->drawAdvObjShadow(
                    frame,
                    tilex + (objType->m_width - drawX - 1) * 32,
                    tiley + (objType->m_height - drawY - 1) * 32,
                    tilew, tileh, g_windowManager->m_screenBitmap,
                    baseX, baseY + 8, false);
            }
        }
    }

    if (destY == CURSOR_DEST_Y0) {
        if (m_drawCursor && !g_inViewWorld) {
            if (destX == CURSOR_DEST_X0)
                drawCursorShadow(0, 0);
            else if (destX == CURSOR_DEST_X1)
                drawCursorShadow(1, 0);
            else if (destX == CURSOR_DEST_X2)
                drawCursorShadow(2, 0);
        }
    } else if (destY == CURSOR_DEST_Y1) {
        if (m_drawCursor && !g_inViewWorld) {
            if (destX == CURSOR_DEST_X0)
                drawCursorShadow(0, 1);
            else if (destX == CURSOR_DEST_X1)
                drawCursorShadow(1, 1);
            else if (destX == CURSOR_DEST_X2)
                drawCursorShadow(2, 1);
        }
    }

    if (foundHero || foundBoat) {
        for (int part = 0; part <= 5; ++part) {
            if (heroParts[part].m_isValid)
                drawHeroPartShadow(part, heroParts[part], baseX, baseY,
                                   tilex, tiley, tilew, tileh);
            if (boatParts[part].m_isValid)
                drawBoatPartShadow(part, boatParts[part], baseX, baseY,
                                   tilex, tiley, tilew, tileh);
        }
    }
}

VA(0x00411b80, 0x1D7)  // dc 0x13890
void AdvManager::drawRiver(int srcX, int srcY, int z, int destX, int destY)
{
    if (srcX < 0 || srcY < 0 || srcX >= g_mapWidth || srcY >= g_mapHeight)
        return;

    type_point point;
    point = type_point(srcX, srcY, z);
    NewmapCell* thisCell;
    if (!point.isValid()) {
        thisCell = m_fullMap->cell(0, 0, 0);
    } else {
        thisCell = m_fullMap->cell(point.m_x, point.m_y, point.m_z);
    }
    if (!thisCell->m_riverSet)
        return;

    int baseX = m_scrollX + destX * 32;
    int baseY = m_scrollY + destY * 32;
    int tilex = 0;
    int tiley = 0;
    int tilew = 32;
    int tileh = 32;

    if (baseX < 8) {
        tilex = 8 - baseX;
        tilew = baseX + 24;
        baseX = 8;
    }
    if (baseY < 0) {
        tiley = -baseY;
        tileh = baseY + 32;
        baseY = 0;
    }
    if (baseX + tilew > 600)
        tilew = 600 - baseX;
    if (baseY + tileh > 544)
        tileh = 544 - baseY;
    if (tilew <= 0 || tileh <= 0)
        return;

    m_riverTileset[thisCell->m_riverSet]->drawTile(
        thisCell->m_riverIndex, tilex, tiley, tilew, tileh,
        g_windowManager->m_screenBitmap, baseX, baseY + 8,
        (thisCell->m_flags0011 >> 2) & 1,
        (thisCell->m_flags0011 >> 3) & 1);
}

VA(0x00411d60, 0x1EC)  // dc 0x13a64
void AdvManager::drawRoad(int srcX, int srcY, int z, int destX, int destY)
{
    if (srcX < 0 || srcY < 0 || srcX >= g_mapWidth || srcY >= g_mapHeight)
        return;

    type_point point;
    point = type_point(srcX, srcY, z);
    NewmapCell* thisCell;
    if (!point.isValid()) {
        thisCell = m_fullMap->cell(0, 0, 0);
    } else {
        thisCell = m_fullMap->cell(point.m_x, point.m_y, point.m_z);
    }
    if (!thisCell->m_roadSet)
        return;

    int baseX = m_scrollX + destX * 32;
    int baseY = m_scrollY + destY * 32 + 16;
    int tilex = 0;
    int tiley = 0;
    int tilew = 32;
    int tileh = 32;

    if (baseX < 8) {
        tilex = 8 - baseX;
        tilew = baseX + 24;
        baseX = 8;
    }
    if (baseY < 0) {
        tiley = -baseY;
        tileh = baseY + 32;
        baseY = 0;
    }
    if (baseX + tilew > 600)
        tilew = 600 - baseX;
    if (baseY + tileh > 544)
        tileh = 544 - baseY;
    if (srcY == g_mapHeight - 1)
        tileh -= 16;
    if (tilew <= 0 || tileh <= 0)
        return;

    m_roadTileset[thisCell->m_roadSet]->drawTile(
        thisCell->m_roadIndex, tilex, tiley, tilew, tileh,
        g_windowManager->m_screenBitmap, baseX, baseY + 8,
        (thisCell->m_flags0011 >> 4) & 1,
        (thisCell->m_flags0011 >> 5) & 1);
}

VA(0x00411f50, 0x15F)  // dc 0x13c68
void AdvManager::drawArrowShadow(int srcX, int srcY, int z, int destX,
                                 int destY)
{
    if (srcX < 0 || srcY < 0 || srcX >= g_mapWidth || srcY >= g_mapHeight)
        return;

    int arrow = m_routeArray[(z * g_mapHeight + srcY) * g_mapWidth + srcX];
    if (!arrow)
        return;

    int baseX = m_scrollX + destX * 32;
    int baseY = m_scrollY + destY * 32;

    type_point point;
    point = type_point(srcX, srcY, z);
    getCell(point);

    int tilex = 0;
    int tiley = 0;
    int tilew = 32;
    int tileh = 32;

    if (baseX < 8) {
        tilex = 8 - baseX;
        tilew = baseX + 24;
        baseX = 8;
    }
    if (baseY < 0) {
        tiley = -baseY;
        tileh = baseY + 32;
        baseY = 0;
    }
    if (baseX + tilew > 600)
        tilew = 600 - baseX;
    if (baseY + tileh > 544)
        tileh = 544 - baseY;
    if (tilew <= 0 || tileh <= 0)
        return;

    m_arrowTileset->drawTileShadow(arrow - 1, tilex, tiley, tilew, tileh,
                                 g_windowManager->m_screenBitmap, baseX,
                                 baseY + 8, 0, 0);
}

VA(0x004120b0, 0x162)  // dc 0x13e28
void AdvManager::drawArrow(int srcX, int srcY, int z, int destX, int destY)
{
    if (srcX < 0 || srcY < 0 || srcX >= g_mapWidth || srcY >= g_mapHeight)
        return;

    int arrow = m_routeArray[(z * g_mapHeight + srcY) * g_mapWidth + srcX];
    if (!arrow)
        return;

    type_point point;
    point = type_point(srcX, srcY, z);
    getCell(point);

    int baseX = m_scrollX + destX * 32;
    int baseY = m_scrollY + destY * 32;
    int tilex = 0;
    int tiley = 0;
    int tilew = 32;
    int tileh = 32;

    if (baseX < 8) {
        tilex = 8 - baseX;
        tilew = baseX + 24;
        baseX = 8;
    }
    if (baseY < 0) {
        tiley = -baseY;
        tileh = baseY + 32;
        baseY = 0;
    }
    if (baseX + tilew > 600)
        tilew = 600 - baseX;
    if (baseY + tileh > 544)
        tileh = 544 - baseY;
    if (tilew <= 0 || tileh <= 0)
        return;

    m_arrowTileset->drawTile(arrow - 1, tilex, tiley, tilew, tileh,
                           g_windowManager->m_screenBitmap, baseX, baseY + 8,
                           0, 0);
}

VA(0x00412220, 0x248)  // dc 0x13fc8
void AdvManager::drawShroud(int srcX, int srcY, int z, int destX, int destY)
{
    if (srcX < 0 || srcY < 0 || srcX >= g_mapWidth)
        return;
    if (srcY >= g_mapHeight && !g_completeDrawAllCells)
        return;

    {
        type_point point;
        point = type_point(srcX, srcY, z);
        point.isValid();
    }

    int baseX = m_scrollX + destX * 32;
    int baseY = m_scrollY + destY * 32;
    unsigned char hflip = false;
    int tilex = 0;
    int tiley = 0;
    int tilew = 32;
    int tileh = 32;

    if (baseX < 8) {
        tilex = 8 - baseX;
        tilew = baseX + 24;
        baseX = 8;
    }
    if (baseY < 0) {
        tiley = -baseY;
        tileh = baseY + 32;
        baseY = 0;
    }
    if (baseX + tilew > 600)
        tilew = 600 - baseX;
    if (baseY + tileh > 544)
        tileh = 544 - baseY;
    if (tilew <= 0 || tileh <= 0)
        return;

    if (!g_completeDrawAllCells
        && (getMapExtra(srcX, srcY, z) & g_mapVisibilityBit))
        return;

    do {
        int lookup;
        if (g_completeDrawAllCells)
            break;

        lookup = getCloudLookup(srcX, srcY, z);
        if (!lookup)
            break;
        if (lookup >= CLOUD_DRAW_FLIPPED_OFFSET) {
            hflip = true;
            lookup -= CLOUD_DRAW_FLIPPED_OFFSET;
        }
        if ((lookup == CLOUD_DRAW_FRAME_1 || lookup == CLOUD_DRAW_FRAME_5)
            && (srcX & 1))
            ++lookup;
        if (lookup == CLOUD_DRAW_FRAME_3 && (srcY & 1))
            lookup = CLOUD_DRAW_FRAME_4;
        m_cloudIcons->drawShroudTile(
            lookup - 1, tilex, tiley, tilew, tileh,
            g_windowManager->m_screenBitmap, baseX, baseY + 8, hflip, false);
        return;
    } while (0);

    int frame = ((srcX * 85 ^ srcY * 85) / 64) & 3;
    m_starTileset->drawShroudTile(
        frame, tilex, tiley, tilew, tileh,
        g_windowManager->m_screenBitmap, baseX, baseY + 8, false, false);
}

// E:\gamedcs\advmgr.cpp:6805
// Residual (92.7034%): register homing/scheduling only. The two streams have
// exactly 22 branches and one return with identical symbolic targets; why-reg
// measures flow distance 0 and 223 register-visible slots, while its allocator
// model finds the initial ESI/EDI/EBX definitions identical and the divergence
// entirely past those first definitions. Dreamcast's lexical scopes were
// restored and retained: the top roster `tilew, tileh, tilex, tiley, baseX,
// baseY, thisCell`; signed `numObj` plus `SprPtr, ObjCell, ObjType`; and the
// flag arm's `triggerCell, triggerY, triggerX, Obj, owner`. All three changes
// are byte-flat at 92.7034%, including the attested Obj temporary used only by
// FindTrigger. With exact flow and no extra evaluated expression in retail,
// there is no byte evidence for manufacturing a release VERIFY carrier here.
// The type_point constructor lever (a PER-SITE fact elsewhere in the tree) is
// bounded here too, 2026-09-06: at the DrawHeroCell site, three field stores
// score 91.6110 and `type_point point(srcX, srcY, z)` is byte-flat at
// 92.7034; at the FindTrigger site `type_point triggerPoint(triggerX,
// triggerY, z)` scores 91.2827.  The written default-then-assign form is the
// maximum at both.
VA(0x00412470, 0x482)  // linkorder, dc 0x142e0
void AdvManager::drawUnderlay(int srcX, int srcY, int z, int destX, int destY)
{
    if (srcX < 0 || srcY < 0 || srcX >= g_mapWidth || srcY >= g_mapHeight)
        return;

    int tilew;
    int tileh;
    int tilex;
    int tiley;
    int baseX;
    int baseY;
    NewmapCell* thisCell;

    type_point point;
    point = type_point(srcX, srcY, z);
    thisCell = getCell(point);

    baseX = m_scrollX + destX * 32;
    baseY = m_scrollY + destY * 32;
    tilex = 0;
    tiley = 0;
    tilew = 32;
    tileh = 32;

    if (baseX < 8) {
        tilex = 8 - baseX;
        tilew = baseX + 24;
        baseX = 8;
    }
    if (baseY < 0) {
        tiley = -baseY;
        tileh = baseY + 32;
        baseY = 0;
    }
    if (baseX + tilew > 600)
        tilew = 600 - baseX;
    if (baseY + tileh > 544)
        tileh = 544 - baseY;
    if (tilew <= 0 || tileh <= 0)
        return;

    if (thisCell->m_objects.size() > 0) {
        for (int numObj = 0; numObj < thisCell->m_objects.size();
             ++numObj) {
            CSprite* sprite;
            NewmapCell::ObjectCell* objCell;
            CObjectType* objType;
            objCell = &thisCell->m_objects[numObj];
            objType = &m_fullMap->m_objectTypes[
                m_fullMap->m_objects[objCell->m_objectIndex].m_typeIndex];
            sprite = m_fullMap->m_sprites[
                m_fullMap->m_objects[objCell->m_objectIndex].m_typeIndex];
            if (!objType->m_suppressDraw)
                continue;

            switch (objType->m_objectType) {
            case CREATURE_GENERATOR_1:
            case CREATURE_GENERATOR_4:
            case GARRISON:
            case LIGHTHOUSE:
            case MINE:
            case RANDOM_TOWN:
            case SHIPYARD:
            case TOWN: {
                NewmapCell* triggerCell;
                int triggerY;
                int triggerX;
                CObject* obj;
                int owner;
                obj = &m_fullMap->m_objects[objCell->m_objectIndex];
                obj->findTrigger(triggerX, triggerY);
                type_point triggerPoint;
                triggerPoint = type_point(triggerX, triggerY, z);
                // The `valid` and `map` locals are the file's own
                // trigger-cell idiom (DrawHeroCell, DrawAdvObjShadow), and
                // both halves are measured here: 92.2180 bare, 92.5775 with
                // `valid` alone, 92.7034 with both. Retail's two arms each
                // re-read fullMap where `map` reads it once, so the byte
                // reading argues against the local and the score argues for
                // it - the score is the verdict.
                unsigned char valid = triggerPoint.isValid();
                NewfullMap* map = m_fullMap;
                if (!valid)
                    triggerCell = map->cell(0, 0, 0);
                else
                    triggerCell = map->cell(
                        triggerPoint.m_x, triggerPoint.m_y, triggerPoint.m_z);
                owner = getFlaggedObjectOwner(triggerCell);
                int frame = (m_animCtr
                             + m_fullMap->m_objects[objCell->m_objectIndex]
                                   .m_animationOffset)
                            % sprite->getNumFrames(0);
                signed char offsets = objCell->m_offsets;
                int yOffset = offsets >> 4;
                offsets <<= 4;
                int xOffset = offsets >> 4;
                sprite->drawAdvObjWithFlag(
                    frame,
                    tilex + (objType->m_width - xOffset - 1) * 32,
                    tiley + (objType->m_height - yOffset - 1) * 32,
                    tilew, tileh, g_windowManager->m_screenBitmap,
                    baseX, baseY + 8,
                    g_unnamed6aacb0->m_data[64 + owner], false);
                break;
            }
            default: {
                int frame = (m_animCtr
                             + m_fullMap->m_objects[objCell->m_objectIndex]
                                   .m_animationOffset)
                            % sprite->getNumFrames(0);
                signed char offsets = objCell->m_offsets;
                int yOffset = offsets >> 4;
                offsets <<= 4;
                int xOffset = offsets >> 4;
                sprite->drawAdvObj(
                    frame,
                    tilex + (objType->m_width - xOffset - 1) * 32,
                    tiley + (objType->m_height - yOffset - 1) * 32,
                    tilew, tileh, g_windowManager->m_screenBitmap,
                    baseX, baseY + 8, false);
                break;
            }
            }
        }
    }
}

VA(0x00412900, 0x2CB)  // dc 0x147c4
void AdvManager::drawGround(int srcX, int srcY, int z, int destX, int destY)
{
    NewmapCell* thisCell = getCell(
        type_point(srcX, srcY, z));

    int baseX = m_scrollX + destX * 32;
    int baseY = m_scrollY + destY * 32;
    int tilex = 0;
    int tiley = 0;
    int tilew = 32;
    int tileh = 32;

    if (baseX < 8) {
        tilex = 8 - baseX;
        tilew = baseX + 24;
        baseX = 8;
    }
    if (baseY < 0) {
        tiley = -baseY;
        tileh = baseY + 32;
        baseY = 0;
    }
    if (baseX + tilew > 600)
        tilew = 600 - baseX;
    if (baseY + tileh > 544)
        tileh = 544 - baseY;
    if (tilew <= 0 || tileh <= 0)
        return;

    if (srcX >= 0 && srcY >= 0 && srcX < g_mapWidth
        && srcY < g_mapHeight) {
        m_groundTileset[thisCell->m_groundSet]->drawTile(
            thisCell->m_groundIndex, tilex, tiley, tilew, tileh,
            g_windowManager->m_screenBitmap, baseX, baseY + 8,
            thisCell->m_flags0011 & 1,
            (thisCell->m_flags0011 >> 1) & 1);
        return;
    }

    int frame = -1;
    if (srcX == -1) {
        if (srcY == -1)
            frame = 16;
        else if (srcY == g_mapHeight)
            frame = 19;
        else if (srcY >= 0 && srcY < g_mapHeight)
            frame = 32 + (srcY & 3);
    } else if (srcX == g_mapWidth) {
        if (srcY == -1)
            frame = 17;
        else if (srcY == g_mapHeight)
            frame = 18;
        else if (srcY >= 0 && srcY < g_mapHeight)
            frame = 24 + (srcY & 3);
    } else if (srcY == -1) {
        if (srcX >= 0 && srcX < g_mapWidth)
            frame = 20 + (srcX & 3);
    } else if (srcY == g_mapHeight) {
        if (srcX >= 0 && srcX < g_mapHeight)
            frame = 28 + (srcX & 3);
    }

    if (frame == -1)
        frame = (srcX + 16) % 4 + 4 * ((srcY + 16) % 4);

    m_borderTileset->drawTile(
        frame, tilex, tiley, tilew, tileh,
        g_windowManager->m_screenBitmap, baseX, baseY + 8, false, false);
}

#if 0  // @carcass

// E:\gamedcs\advmgr.cpp:7019
DC_ONLY(0x14b08, 0x88)
NewmapCell* AdvManager::getCell(int x, int y, int z)
{
    // @stub
}

// E:\gamedcs\advmgr.cpp:7026
#endif  // @carcass

// Read by both mobilization draw gates and by UpdateRadar's radar-icon
// frame select before repainting the adventure screen; role (an "adventure
// repaint suppressed" latch) is what the bytes prove. No located writer yet
// - nearest consumer holds the declaration, and UpdateRadar below is now
// the earliest of them, so the claim sits here rather than beside
// DemobilizeCurrHero.
DATA(0x006aac3c) extern int g_unnamed6aac3c;

VA(0x00412bd0, 0x6C)  // dc 0x14b90
NewmapCell* AdvManager::getCell(type_point point)
{
    // DC advmgr.cpp:7028/7029 preserve both NewfullMap overloads.
    // Retail 0x412be6 folds the zero-coordinate call to cellData, and
    // 0x412bf0..0x412c35 expands the point overload's row arithmetic.
    if (!point.isValid())
        return m_fullMap->cell(0, 0, 0);
    return m_fullMap->cell(point);
}

// E:\gamedcs\advmgr.cpp:7037

// All three of the decode's blockers are now declared: gUnnamed6aac3c's
// DATA claim is hoisted above this function, the view-world tile scale at
// .data 0x68c6b8 is declared (no claim - viewwrld.obj owns it), and
// game::GameFn_004CA780 takes the ordinal-placeholder
// convention.

// The three switches the decode describes are all `switch (MAP_HEIGHT)`
// with FOUR real labels - 36, 72, 108, 144 - and a default. What made them
// look like 108-arm monsters is VC6's dense byte-index form: a 109-byte
// map over MAP_HEIGHT-36 in front of a five-entry jump table, which is
// what it emits for a sparse switch over a bounded range. Both index
// tables were decoded out of the image (0x4135e4 and 0x413714) and agree
// exactly: 36->arm0, 72->arm1, 108->arm2, 144->arm3, everything else
// ->arm4. The per-cell colour switch at 0x413654/0x413670 decodes the same
// way - SIX bodies over 28 real labels, and every one of the 28 lands on a
// TAdventureObjectType enumerator this tree already carries.

// Retail's own inconsistency, transcribed rather than tidied: the row
// advance uses the LIVE screenBitmap->Pitch while the writes inside a
// pixel block use a hardcoded 0x640-byte stride.

VA(0x00412c40, 0xB41)  // linkorder, dc 0x14bec
void AdvManager::updateRadar(type_point origin, unsigned char updateFlag, unsigned char partialUpdate, unsigned char viewMines, unsigned char viewHeros, unsigned char viewTowns)
{
    Widget* radar = m_advWindow->m_radarWidget;
    int rectX = radar->m_x;
    int rectY = radar->m_y;
    int rectWidth = radar->m_width;
    int rectHeight = radar->m_height;

    if (g_networkActive69954c && !g_currentPlayer->isLocalHuman()) {
        CNetMsgHandler* handler = g_dPlay->getNetMsgHandler();
        if (handler && handler->isInPopup() && !g_completeDrawMessageBypass
            && !g_unnamed6aac3c)
            return;
    }

    int lastColumn = g_mapWidth - 1;
    int lastRow = g_mapHeight - 1;
    PlayerData* localPlayer = g_game->getLocalPlayer();

    if (!g_currentPlayer->isHuman() && m_heroLogoShowing == 0
        && (!g_networkActive69954c || g_unk691209))
        g_game->showHeroesLogo();

    if (!g_networkActive69954c && !g_currentPlayer->isHuman()
        && !g_unk691209)
        return;
    if (m_heroLogoShowing && !g_currentPlayer->isHuman())
        return;
    m_heroLogoShowing = 0;

    // The acting player's live hero, and its map square, so the cell loop
    // below can paint that one square in the owner's colour.
    // The two knobs the 2026-08-21 note banked as REJECTED (-0.69 for the
    // explicit else arm, -0.25 for dropping the `int z = origin.z;` cache)
    // are worth +0.60 TOGETHER, which is the non-monotone-combination rule
    // exactly: measure the pair, not each knob. With both applied the
    // branch polarity at the currentHero guard flips to retail's `jne`
    // (the zero store is the FALL-THROUGH arm, so the null case is the
    // `if` and the lookup the `else`), and the third knob - initialising
    // `revealed` from the whole && chain instead of `= 0` plus a guarded
    // `= 1` - gives retail's `mov al,1 / jmp / xor al,al` and takes the
    // branch view CLEAN. 90.6495 -> 91.2477.
    // Still rejected, re-measured here: widening `visibilityBit` to the
    // `int` retail plainly holds at [ebp-0x44] (`and eax,0xffff / test
    // ecx,eax` against our byte `test cl,al`) costs 0.12 with the bool
    // initialiser in place and 0.71 without it. The rest is the
    // callee-saved role of `this`: retail keeps it in ECX and spills to
    // [ebp-0x8], we move it to ESI - the bounded C1 handle-state class.
    int heroX = -1;
    int heroY = -1;
    const Hero* currentHero;
    if (localPlayer->m_currHeroId == -1) {
        currentHero = 0;
    } else {
        currentHero = &g_game->m_heroes[localPlayer->m_currHeroId];
        if (currentHero && currentHero->m_z == origin.m_z) {
            heroX = currentHero->m_x;
            heroY = currentHero->m_y;
        }
    }

    int rowPhase = 0;
    int blockPhase = 0;
    unsigned short* destRow;
    if (g_mapHeight == MAP_DIMENSION_SMALL
        || g_mapHeight == MAP_DIMENSION_MEDIUM) {
        destRow = g_windowManager->m_screenBitmap->getMap(0, 0)
                  + g_windowManager->m_screenBitmap->getPitch() * rectY / 2 + rectX;
    } else if (g_mapHeight == MAP_DIMENSION_LARGE) {
        rowPhase = 0;
        blockPhase = 0;
        destRow = g_windowManager->m_screenBitmap->getMap(0, 0)
                  + g_windowManager->m_screenBitmap->getPitch() * rectY / 2 + rectX;
    } else {
        destRow = g_windowManager->m_screenBitmap->getMap(0, 0)
                  + g_windowManager->m_screenBitmap->getPitch() * rectY / 2 + rectX;
    }

    unsigned char visibilityBit = g_mapVisibilityBit;
    for (int y = 0; y <= lastRow; y++) {
        unsigned short* dest = destRow;
        switch (g_mapHeight) {
        case MAP_DIMENSION_SMALL:
            destRow += 4 * g_windowManager->m_screenBitmap->getPitch();
            break;
        case MAP_DIMENSION_MEDIUM:
            destRow += 2 * g_windowManager->m_screenBitmap->getPitch();
            break;
        case MAP_DIMENSION_LARGE:
            destRow += g_windowManager->m_screenBitmap->getPitch();
            if (++rowPhase > 2) {
                rowPhase = 0;
                destRow += g_windowManager->m_screenBitmap->getPitch();
            } else if (rowPhase == 0) {
                destRow += g_windowManager->m_screenBitmap->getPitch();
            }
            break;
        case MAP_DIMENSION_EXTRA_LARGE:
            destRow += g_windowManager->m_screenBitmap->getPitch();
            break;
        }

        for (int x = 0; x <= lastColumn; x++) {
            NewmapCell* cell = m_fullMap->cell(x, y, origin.m_z);

            unsigned char revealed =
                !g_completeDrawAllCells
                && (visibilityBit & getMapExtra(x, y, origin.m_z)) && x >= 0
                && y >= 0 && x < g_mapWidth && y < g_mapHeight;
            if (viewMines && cell->m_type == MINE)
                revealed = 1;
            if (viewHeros && cell->m_type == HERO)
                revealed = 1;

            unsigned short colour;
            if (!(viewTowns && cell->m_type == TOWN) && !revealed) {
                colour = 0;
            } else {
                colour = m_groundTileset[cell->m_groundSet]->m_p->m_data[8];
                if (x == heroX && y == heroY) {
                    colour = g_unnamed6aacb0->m_data[64 + currentHero->m_owner];
                } else if (cell->m_type == HERO
                           && (cell->m_cellFlags & 0x1000)) {
                    colour = g_unnamed6aacb0->m_data[64 +
                        g_game->m_heroAvailability[cell->m_extraInfo]];
                } else {
                    switch (cell->getMapObject()) {
                    case TERRAIN_CACTUS:
                    case TERRAIN_CRATER:
                    case TERRAIN_DEAD_VEGETATION:
                    case TERRAIN_FROZEN_LAKE:
                    case TERRAIN_HILL:
                    case TERRAIN_LAKE:
                    case TERRAIN_LAVA_LAKE:
                    case TERRAIN_MANDRAKE:
                    case TERRAIN_MOUND:
                    case TERRAIN_MOUNTAIN:
                    case TERRAIN_OAK_TREE:
                    case TERRAIN_PINE_TREE:
                    case TERRAIN_SAND_DUNE:
                    case TERRAIN_SAND_PIT:
                    case TERRAIN_STALAGMITE:
                    case TERRAIN_STUMP:
                    case TERRAIN_TAR_PIT:
                    case TERRAIN_TREE:
                    case TERRAIN_VOLCANO:
                    case TERRAIN_WILLOW_TREE:
                    case TERRAIN_YUCCA_TREE:
                        if (!(cell->m_cellFlags & 0x40))
                            colour = m_groundTileset[cell->m_groundSet]
                                         ->m_p->m_data[9];
                        break;
                    case TOWN:
                        if (!(cell->m_cellFlags & 0x40)
                            || (cell->m_cellFlags & 0x1000)) {
                            NewmapCell* trigger = cell->getTriggerCell();
                            if (trigger)
                                colour = g_unnamed6aacb0->m_data[64 +
                                    g_game->m_towns[trigger
                                        ->getMapExtraInfo()].m_owner];
                        }
                        break;
                    case LIGHTHOUSE:
                    case MINE:
                        if (!(cell->m_cellFlags & 0x40)
                            || (cell->m_cellFlags & 0x1000)) {
                            NewmapCell* trigger = cell->getTriggerCell();
                            if (trigger)
                                colour = g_unnamed6aacb0->m_data[64 +
                                    g_game->m_mines[trigger
                                        ->getMapExtraInfo()].m_playerOwner];
                        }
                        break;
                    case CREATURE_GENERATOR_1:
                    case CREATURE_GENERATOR_4:
                        if (!(cell->m_cellFlags & 0x40)
                            || (cell->m_cellFlags & 0x1000)) {
                            NewmapCell* trigger = cell->getTriggerCell();
                            if (trigger)
                                colour = g_unnamed6aacb0->m_data[64 +
                                    g_game->m_generators[trigger
                                        ->getMapExtraInfo()].getOwner()];
                        }
                        break;
                    case GARRISON:
                        if (!(cell->m_cellFlags & 0x40)
                            || (cell->m_cellFlags & 0x1000)) {
                            NewmapCell* trigger = cell->getTriggerCell();
                            if (trigger)
                                colour = g_unnamed6aacb0->m_data[64 +
                                    g_game->m_garrisons[trigger
                                        ->getMapExtraInfo()].m_playerOwner];
                        }
                        break;
                    case SHIPYARD:
                        if (!(cell->m_cellFlags & 0x40)
                            || (cell->m_cellFlags & 0x1000)) {
                            NewmapCell* trigger = cell->getTriggerCell();
                            if (trigger)
                                colour = g_unnamed6aacb0->m_data[64 +
                                    static_cast<signed char>(trigger
                                        ->getMapExtraInfo())];
                        }
                        break;
                    }
                }
            }

            // The write side of the 4/3 stretch. The 0x640 stride is
            // retail's own hardcode; the row advance above uses the live
            // Pitch instead.
            switch (g_mapHeight) {
            case MAP_DIMENSION_SMALL:
                dest[0] = colour;
                dest[1] = colour;
                dest[2] = colour;
                dest[3] = colour;
                dest[0x320] = colour;
                dest[0x321] = colour;
                dest[0x322] = colour;
                dest[0x323] = colour;
                dest[0x640] = colour;
                dest[0x641] = colour;
                dest[0x642] = colour;
                dest[0x643] = colour;
                dest[0x960] = colour;
                dest[0x961] = colour;
                dest[0x962] = colour;
                dest[0x963] = colour;
                dest += 4;
                break;
            case MAP_DIMENSION_MEDIUM:
                dest[0] = colour;
                dest[1] = colour;
                dest[0x320] = colour;
                dest[0x321] = colour;
                dest += 2;
                break;
            case MAP_DIMENSION_LARGE:
                if (blockPhase) {
                    dest[0] = colour;
                    if (rowPhase)
                        dest += 1;
                    else {
                        dest[0x320] = colour;
                        dest += 1;
                    }
                } else {
                    dest[0] = colour;
                    dest[1] = colour;
                    if (!rowPhase) {
                        dest[0x320] = colour;
                        dest[0x321] = colour;
                    }
                    dest += 2;
                }
                if (++blockPhase > 2)
                    blockPhase = 0;
                break;
            case MAP_DIMENSION_EXTRA_LARGE:
                dest[0] = colour;
                dest += 1;
                break;
            }
        }
    }

    // Radar-icon frame and the origin-to-screen scale.
    int radarFrame = -1;
    int suppressIcon = 0;
    float scale;
    if (g_unnamed6aac3c) {
        switch (g_mapHeight) {
        case MAP_DIMENSION_SMALL:
            scale = 4.0f;
            if (g_unnamed68c6b8 == VIEW_WORLD_TILE_SCALE_FULL)
                radarFrame = 13;
            else
                suppressIcon = 1;
            break;
        case MAP_DIMENSION_MEDIUM:
            scale = 2.0f;
            if (g_unnamed68c6b8 == VIEW_WORLD_TILE_SCALE_FULL)
                radarFrame = 11;
            else if (g_unnamed68c6b8 == VIEW_WORLD_TILE_SCALE_MID)
                radarFrame = 12;
            else if (g_unnamed68c6b8 == VIEW_WORLD_TILE_SCALE_FAR)
                suppressIcon = 1;
            break;
        case MAP_DIMENSION_LARGE:
            scale = 1.33f;
            if (g_unnamed68c6b8 == VIEW_WORLD_TILE_SCALE_FULL)
                radarFrame = 10;
            else if (g_unnamed68c6b8 == VIEW_WORLD_TILE_SCALE_MID)
                radarFrame = 9;
            else if (g_unnamed68c6b8 == VIEW_WORLD_TILE_SCALE_FAR)
                radarFrame = 10;
            break;
        default:
            scale = 1.0f;
            if (g_unnamed68c6b8 == VIEW_WORLD_TILE_SCALE_FULL)
                radarFrame = 5;
            else if (g_unnamed68c6b8 == VIEW_WORLD_TILE_SCALE_MID)
                radarFrame = 6;
            else if (g_unnamed68c6b8 == VIEW_WORLD_TILE_SCALE_FAR)
                radarFrame = 7;
            break;
        }
    } else {
        switch (g_mapHeight) {
        case MAP_DIMENSION_SMALL:
            radarFrame = 4;
            scale = 4.0f;
            break;
        case MAP_DIMENSION_MEDIUM:
            radarFrame = 3;
            scale = 2.0f;
            break;
        case MAP_DIMENSION_LARGE:
            radarFrame = 2;
            scale = 1.33f;
            break;
        default:
            radarFrame = 1;
            scale = 1.0f;
            break;
        }
    }

    int srcX;
    if (origin.m_x < 0)
        srcX = static_cast<long>(-scale * origin.m_x);
    else
        srcX = 0;
    int srcY;
    if (origin.m_y < 0)
        srcY = static_cast<long>(-scale * origin.m_y);
    else
        srcY = 0;

    CSprite* icons = m_radarIcons;
    int drawWidth = icons->getWidth() - srcX;
    int drawHeight = icons->getHeight() - srcY;

    int destX;
    if (origin.m_x < 0)
        destX = static_cast<long>(static_cast<float>(rectX));
    else
        destX = static_cast<long>(rectX + origin.m_x * scale);
    int destY;
    if (origin.m_y < 0)
        destY = static_cast<long>(static_cast<float>(rectY));
    else
        destY = static_cast<long>(rectY + origin.m_y * scale);

    if (icons->getWidth() + destX > rectX + rectWidth)
        drawWidth += rectWidth - icons->getWidth() - destX + rectX;
    if (icons->getHeight() + destY > rectY + rectHeight)
        drawHeight += rectHeight - icons->getHeight() - destY + rectY;
    if (drawWidth < 0)
        drawWidth = 0;
    if (drawHeight < 0)
        drawHeight = 0;

    if (!suppressIcon)
        icons->drawInterface(radarFrame, srcX, srcY, drawWidth, drawHeight,
                             g_windowManager->m_screenBitmap->getMap(0, 0), destX,
                             destY, g_windowManager->m_screenBitmap->getWidth(),
                             g_windowManager->m_screenBitmap->getHeight(),
                             g_windowManager->m_screenBitmap->getPitch(), 0);

    if (updateFlag)
        g_windowManager->updateScreen(rectX, rectY, rectWidth, rectHeight);
}

VA(0x00413790, 0x27)  // dc 0x15f7c
void AdvManager::updateRadar(unsigned char updateFlag, unsigned char partialUpdate, unsigned char viewMines, unsigned char viewHeroes, unsigned char viewTowns)
{
    updateRadar(m_radarOrigin, updateFlag, partialUpdate, viewMines,
                viewHeroes, viewTowns);
}

// E:\gamedcs\advmgr.cpp:7543. Current reconstruction: 95.5740%.
// Retail owns the 216-byte case index at 0x415c88 and the 57-entry jump
// table at 0x415ba4. DC supplies the source interfaces, local types and
// statement groups. Its observed 7543..8813 span has 525 recorded rows;
// the 746 unrecorded lines do not identify missing source text or recover
// a total function line count.

// The continued source audit raised 94.7326 -> 95.5740 with all 93 exact
// advmgr siblings preserved. Generator owner/type initialization separately
// evaluates the vector subscript (DC7739/7740 and 7757/7758), unlike the
// named references in SetRolloverText. DC7890's meaningful fountain query
// remains even where release removes it; the later cell-knowledge test is
// independent. Its four masked terms accumulate into visited, as do the
// two temple terms and the lean-to result. Nested trigger/hero guards keep
// the separate DC source stages in ARENA, DEAD_GUY, LEAN_TO, SIREN and STABLES.

// Object names use the cell's type, not manually specialized case constants:
// DC7612 (0x1646c), 7644 (0x165b4), 7886 (0x16e00), 8184 (0x17804) and the
// other object-name loads all read cell+28 before indexing gQuickViewText.
// Restoring the 32 subscripts is byte-flat on the selected parent, but
// changes some alternative formatting results through natural compiler state.

// Failed controls: direct ARENA mask accumulation on the final parent
// changes natural inlining and falls to 91.7578. The garden expression and
// temple operand order are byte-flat; eight states produce two objects.
// Earlier copied-point scope, non-const alias and forced out-of-line GetCell
// probes do not justify erasing the canonical GetCell call. Likewise,
// flipping GetHero's own branch order loses exact consumers; its ordinary
// retained definition and this source call remain. Discarded string empty()
// and size() probes were byte-flat and are not retained as VERIFY statements.
// Binding the seer return to an extra named value/reference was also worse.

// Complete calls the exact mine/shrine/tree/witch helpers and the distinct
// quick-info quest/seer builders 0x572e40/0x5743e0. DC's older in-caller mine
// and seer operations do not replace those retail-proven calls. The current
// quest temporary's destructor expands naturally as retail does. Remaining
// differences include GetHero arm layout, the nested cell/zCell decision,
// and switch-tail scheduling. Compare named sites, not aggregate call counts.
// ExtraInfoUnion's DC inheritance is represented by NewmapCell's existing
// data/accessor surface; the audit retains that ownership gap, including
// GetItemId. It is not evidence for a copied helper body in this caller.
VA(0x004137c0, 0x25A0)  // linkorder, dc 0x15fdc
void AdvManager::quickInfo(int cellX, int cellY, int z)
{
    // DC records tempText[500]. Retail bases it at [ebp-0x238] with
    // separate dwords beginning at [ebp-0x44], exactly 500 bytes later.
    // The shared display locals retain the DC declaration order and types.
    unsigned long testFlag;
    int width;
    int visited;
    NewmapCell* testCell;
    PlayerData* player;
    type_point mapPoint;
    long x;
    Hero* currHero;
    long y;
    int playerId;
    char tempText[500];
    int playerBit;
    int height;
    int infolevel;

    player = g_game->getLocalPlayer();
    playerId = g_game->getLocalPlayerGamePos();
    playerBit = 1 << playerId;
    currHero = g_game->getHero(player->m_currHeroId);
    mapPoint.m_x = m_radarOrigin.m_x + cellX;
    mapPoint.m_y = m_radarOrigin.m_y + cellY;
    mapPoint.m_z = z;

    if (!mapPoint.isValid()) {
        strcpy(g_text, (*g_generalText)[
            GENERAL_TEXT_QUICK_INFO_INVALID_POINT]);
    } else {
        // DC names GetCell here; its ordinary retained body owns the
        // validity branch and canonical map indexing.
        testCell = getCell(mapPoint);

        if (!(getMapExtra(mapPoint) & playerBit)) {
            strcpy(g_text, (*g_generalText)[
                GENERAL_TEXT_QUICK_INFO_SHROUDED]);
        } else {
            CellAdjuster adjuster;
            testCell = adjuster.getTriggerCell(testCell, cellX, cellY);

            const char* separator = DATA_COMPGEN(
                0x006603b0, quickInfoSeparator, "\n\n");
            const char* newLine = DATA_COMPGEN(
                0x006603bc, quickInfoNewLine, "\n");
            const char* visitFormat = DATA_COMPGEN(
                0x006603c4, quickInfoVisitFormat, "\n\n%s");
            const char* knownFormat = DATA_COMPGEN(
                0x006603c0, quickInfoKnownFormat, "\n%s");
            // LEAN_TO alone hangs its visit line off a leading SPACE
            // rather than the two newlines every other arm uses; the
            // literal is read straight from the retail image, where
            // 0x0066034c follows the "%s %s" border format at 0x00660344.
            const char* leanToFormat = DATA_COMPGEN(
                0x0066034c, quickInfoLeanToFormat, " %s");

            // Known-object text precedes per-hero visited text. Both stay
            // inside the trigger guard. VC6 reuses the z parameter's stack
            // slot for several locals; the source retains their real names.

            switch (testCell->m_type) {
            case NOTHING:
            case ANCHOR_POINT:
            case EVENT:
            case HOLY_GRAIL: {
                std::string result;
                AdventureObjectType special =
                    testCell->getSpecialTerrain();
                if (special == NOTHING)
                    result = g_terrainNames[testCell->m_groundSet];
                else
                    result =
                        g_adventureObjectNames[special];

                if (testCell->isDiggable()) {
                    result += '\n';
                    result +=
                        (*g_generalText)[GENERAL_TEXT_QUICK_INFO_DIGGABLE];
                }
                strcpy(g_text, result.c_str());
                break;
            }
            case ARENA:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    if (currHero) {
                        testFlag = 1UL << (testCell->m_extraInfo & 0x1f);
                        visited = testFlag & currHero->m_arenaFlags;
                        sprintf(tempText, visitFormat,
                            visited
                                ? (*g_generalText)[
                                      GENERAL_TEXT_VISITED_OBJECT]
                                : (*g_generalText)[
                                      GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case BORDER_GUARD:
            case BORDER_GATE:
                sprintf(g_text, DATA_COMPGEN(
                    0x00660344, rolloverBorderFormat, "%s %s"),
                    g_borderColorNames[testCell->m_objectIndex],
                    g_adventureObjectNames[testCell->m_type]);
                break;
            case BORDER_TENT:
                sprintf(g_text, DATA_COMPGEN(
                    0x00660344, rolloverBorderFormat, "%s %s"),
                    g_borderColorNames[testCell->m_objectIndex],
                    g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    visited = g_game->m_borderTentVisitFlags[
                        testCell->m_objectIndex] & playerBit;
                    if (visited)
                        sprintf(tempText, visitFormat,
                                (*g_generalText)[
                                    GENERAL_TEXT_VISITED_OBJECT]);
                    else
                        sprintf(tempText, visitFormat,
                                (*g_generalText)[
                                    GENERAL_TEXT_UNVISITED_OBJECT]);
                    strcat(g_text, tempText);
                }
                break;
            case BUOY:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(BuoyInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[BuoyInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = currHero->m_flags & 0x4;
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case CLOVER_FIELD:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(CloverFieldInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[CloverFieldInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = (currHero->m_flags & 0x8);
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case CREATURE_BANK: {
                int bankType;
                bankType = testCell->m_objectIndex;
                getCreatureBankHelpText(
                    g_text, testCell, CreatureBankType(bankType), g_unnamed69778c,
                    newLine, 1);
                break;
            }
            case CREATURE_GENERATOR_1: {
                int owner = g_game->m_generators[testCell->m_extraInfo].getOwner();
                int type = g_game->m_generators[testCell->m_extraInfo].m_genType;
                if (owner != -1) {
                    sprintf(g_text, DATA_COMPGEN(
                        0x006603b4, quickInfoOwnedObjectFormat,
                        "%s\n\n%s"),
                        g_creatureGenerator1RolloverNames[type],
                        g_objectOwnerColorNames[owner]);
                } else {
                    strcpy(g_text,
                        g_creatureGenerator1RolloverNames[type]);
                }
                break;
            }
            case CREATURE_GENERATOR_4: {
                int owner = g_game->m_generators[testCell->m_extraInfo].getOwner();
                int type = g_game->m_generators[testCell->m_extraInfo].m_genType;
                if (owner != -1) {
                    sprintf(g_text, DATA_COMPGEN(
                        0x006603b4, quickInfoOwnedObjectFormat,
                        "%s\n\n%s"),
                        g_creatureGenerator4RolloverNames[type],
                        g_objectOwnerColorNames[owner]);
                } else {
                    strcpy(g_text,
                        g_creatureGenerator4RolloverNames[type]);
                }
                break;
            }
            case DEAD_GUY:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    if (currHero) {
                        visited = (g_currentPlayer->m_deadGuyFlags
                            & (1UL << (testCell->m_extraInfo & 0x1f)));
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case DEFENSE_TOWER:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(DefenseTowerInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[DefenseTowerInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = (currHero->m_defenseTowerFlags
                            & (1UL << (testCell->m_extraInfo & 0x1f)));
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case DERELICT_SHIP:
                getCreatureBankHelpText(
                    g_text, testCell, CREATURE_BANK_DERELICT,
                    g_unnamed69778c, separator, 1);
                break;
            case SEPULCHER:
                getCreatureBankHelpText(
                    g_text, testCell, CREATURE_BANK_SEPULCHER,
                    g_unnamed69778c, separator, 1);
                break;
            case SHIPWRECK:
                getCreatureBankHelpText(
                    g_text, testCell, CREATURE_BANK_SHIPWRECK,
                    g_unnamed69778c, separator, 1);
                break;
            case DRAGON_CITY:
                getCreatureBankHelpText(
                    g_text, testCell, CREATURE_BANK_DRAGON,
                    g_unnamed69778c, separator, 1);
                break;
            case QUEST_GUARD:
                strcpy(g_text,
                    m_fullMap->m_questGuardList[testCell->m_extraInfo]
                        .questGuardFn00572E40(g_unnamed69778c).c_str());
                break;
            case FAERIE_RING:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(FaerieRingInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[FaerieRingInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = (currHero->m_flags & 0x2000);
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case FOUNTAIN_OF_FORTUNE:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    // DC7890 queries this before the independent 7892 test.
                    infolevel = g_game->getInfoFlag(FountainOfFortuneInfo, playerId);
                    if (testCell->playerKnowsCell(g_unnamed69778c)) {
                        sprintf(tempText, knownFormat,
                            g_globalInfoFlagNames[FountainOfFortuneInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited =
                            (currHero->m_flags & 0x20UL)
                            + (currHero->m_flags & 0x08000000UL)
                            + (currHero->m_flags & 0x10000000UL)
                            + (currHero->m_flags & 0x20000000UL);
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case FOUNTAIN_OF_YOUTH:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(FountainOfYouthInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[FountainOfYouthInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = (currHero->m_flags & 0x4000);
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case GARDEN_OF_REVELATION:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(GardenOfRevelationInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[GardenOfRevelationInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = (currHero->m_gardenOfRevelationFlags
                            & (1UL << (testCell->m_extraInfo & 0x1f)));
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case HILL_FORT:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(HillFortInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[HillFortInfo]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case HERO:
                setHeroHelp(g_text, testCell);
                break;
            case IDOL_OF_FORTUNE:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(IdolOfFortuneInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[IdolOfFortuneInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = ((currHero->m_flags
                            & 0x02000000UL) + (currHero->m_flags & 0x10UL));
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case LEAN_TO:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    if (currHero) {
                        visited = g_currentPlayer->m_leanToFlags
                            & (1UL << (testCell->m_extraInfo & 0x1f));
                        if (visited)
                            sprintf(tempText, leanToFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, leanToFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case LIBRARY:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(LibraryInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[LibraryInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = (currHero->m_libraryFlags
                            & (1UL << (testCell->m_extraInfo & 0x1f)));
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case LIGHTHOUSE:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    char owner =
                        g_game->m_mines[testCell->m_extraInfo].m_playerOwner;
                    if (owner != -1) {
                        sprintf(tempText, visitFormat,
                                g_objectOwnerColorNames[owner]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case MAGIC_SCHOOL:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(MagicSchoolInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[MagicSchoolInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = (currHero->m_magicSchoolFlags
                            & (1UL << (testCell->m_extraInfo & 0x1f)));
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case MAGIC_SPRING:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(MagicSpringInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[MagicSpringInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = ((g_currentPlayer->m_magicSpringFlags
                            & (1UL << (testCell->m_extraInfo & 0x1f))) && !((testCell->m_extraInfo >> 6) & 1));
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case MAGIC_WELL:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(MagicWellInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[MagicWellInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = (currHero->m_flags & 0x1);
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case MERC_CAMP:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(MercCampInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[MercCampInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = (currHero->m_mercCampFlags
                            & (1UL << (testCell->m_extraInfo & 0x1f)));
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case MERMAID:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(MermaidInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[MermaidInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = (currHero->m_flags & 0x8000);
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case MINE:
                advmgrFn0040D670(g_text, testCell, playerId, newLine, 1);
                break;
            case MYSTICAL_GARDEN:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    visited = (g_currentPlayer->m_mysticalGardenFlags
                        & (1UL << (testCell->m_extraInfo & 0x1f)))
                        && !((testCell->m_extraInfo >> 10) & 1);
                    if (visited)
                        sprintf(tempText, visitFormat,
                                (*g_generalText)[
                                    GENERAL_TEXT_VISITED_OBJECT]);
                    else
                        sprintf(tempText, visitFormat,
                                (*g_generalText)[
                                    GENERAL_TEXT_UNVISITED_OBJECT]);
                    strcat(g_text, tempText);
                }
                break;
            case OASIS:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(OasisInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[OasisInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = (currHero->m_flags & 0x80);
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case OBELISK:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    visited = g_game->m_obeliskFlags[testCell->m_extraInfo]
                        & playerBit;
                    if (visited)
                        sprintf(tempText, visitFormat,
                                (*g_generalText)[
                                    GENERAL_TEXT_VISITED_OBJECT]);
                    else
                        sprintf(tempText, visitFormat,
                                (*g_generalText)[
                                    GENERAL_TEXT_UNVISITED_OBJECT]);
                    strcat(g_text, tempText);
                }
                break;
            case POWER_SCHOOL:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(PowerSchoolInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[PowerSchoolInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = (currHero->m_powerSchoolFlags
                            & (1UL << (testCell->m_extraInfo & 0x1f)));
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case PYRAMID:
                setPyramidHelp(g_text, testCell, currHero, separator);
                break;
            case RALLY_FLAG:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(RallyFlagInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[RallyFlagInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = (currHero->m_flags & 0x10000);
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case RESOURCE:
                strcpy(g_text, g_resourceNames[testCell->m_objectIndex]);
                break;
            case SEER: {
                const SeerHut& thisHut = m_fullMap->m_seerHutList[testCell->m_extraInfo];
                strcpy(g_text,
                       thisHut.seerHutFn005743E0(playerId).c_str());
                break;
            }
            case SHRINE1:
                setShrineHelpText(g_text, currHero, testCell, Shrine1Info,
                                  newLine, separator);
                break;
            case SHRINE2:
                setShrineHelpText(g_text, currHero, testCell, Shrine2Info,
                                  newLine, separator);
                break;
            case SHRINE3:
                setShrineHelpText(g_text, currHero, testCell, Shrine3Info,
                                  newLine, separator);
                break;
            case SIREN:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    if (currHero) {
                        visited = (currHero->m_flags & 0x100000);
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case STABLES:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    if (currHero) {
                        visited = (currHero->m_flags & 0x2);
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case TEMPLE:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(TempleInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[TempleInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = ((currHero->m_flags
                            & 0x100UL) + (currHero->m_flags & 0x04000000UL));
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case TRAINING_GROUNDS:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(TrainingGroundsInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[TrainingGroundsInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = (currHero->m_trainingGroundsFlags
                            & (1UL << (testCell->m_extraInfo & 0x1f)));
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case TREE_OF_KNOWLEDGE:
                setTreeHelpText(g_text, currHero, testCell,
                                newLine, separator);
                break;
            case UNIVERSITY:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(UniversityInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[UniversityInfo]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case WAGON:
                setWagonHelpText(g_text, testCell, separator);
                break;
            case WAR_SCHOOL:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(WarSchoolInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[WarSchoolInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = (currHero->m_warSchoolFlags
                            & (1UL << (testCell->m_extraInfo & 0x1f)));
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case WARRIOR_TOMB:
                setTombHelpText(g_text, testCell, separator);
                break;
            case WATER_WHEEL:
                setWaterWheelHelpText(g_text, testCell, separator);
                break;
            case WATERING_HOLE:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                if (testCell->m_isTrigger) {
                    infolevel = g_game->getInfoFlag(WateringHoleInfo, playerId);
                    if (infolevel) {
                        sprintf(tempText, knownFormat,
                                g_globalInfoFlagNames[WateringHoleInfo]);
                        strcat(g_text, tempText);
                    }
                    if (currHero) {
                        visited = (currHero->m_flags & 0x40);
                        if (visited)
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_VISITED_OBJECT]);
                        else
                            sprintf(tempText, visitFormat,
                                    (*g_generalText)[
                                        GENERAL_TEXT_UNVISITED_OBJECT]);
                        strcat(g_text, tempText);
                    }
                }
                break;
            case WINDMILL:
                setWindmillHelpText(g_text, testCell, separator);
                break;
            case WITCH_HUT:
                setWitchHutHelpText(g_text, currHero, testCell,
                                        newLine, separator);
                break;
            default:
                strcpy(g_text, g_adventureObjectNames[testCell->m_type]);
                break;
            }

        }
    }

    if (g_unnamed6989c8 > 0) {
        // 100, NOT 128, by the same frame reading: retail bases it at
        // [ebp-0x2bc] and uses [ebp-0x258] above it, so 0x2bc-0x258 = 0x64
        // = 100 is its size. Byte-flat; the frame carries the evidence.
        char debugText[100];
        sprintf(debugText, DATA_COMPGEN(
            0x00660394, quickInfoCoordinateFormat,
            "\n\nX: %3d - Y: %3d - Z: %3d"),
            mapPoint.m_x, mapPoint.m_y, mapPoint.m_z);
        strcat(g_text, debugText);
    }

    getQuickviewSize(g_text, &width, &height);

    x = cellX * 32;
    y = cellY * 32;
    if (x < 8)
        x = 8;
    if (y < 8)
        y = 8;
    if (x + width > 600)
        x = 600 - width;
    if (y + height > 552)
        y = 552 - height;

    normalDialog(g_text, 4, x, y, -1, 0, -1, 0, -1, 0, -1, 0);
}

#if 0  // @carcass

// E:\gamedcs\advmgr.cpp:8816
DC_ONLY(0x18c2c, 0x58)
void AdvManager::clearBottomView()
{
    // @stub
}

// E:\gamedcs\advmgr.cpp:8822
#endif  // @carcass

VA(0x00415d60, 0x78)  // dc 0x18c84
void AdvManager::overrideBottomView(AdvManager::BottomViewType view, int time)
{
    m_bottomViewOverride = view;
    if (view != BOTTOM_VIEW_DEFAULT && view != BOTTOM_VIEW_8) {
        if (time < 0) {
            switch (view) {
            case BOTTOM_VIEW_6:
                time = 5000;
                break;
            case BOTTOM_VIEW_1:
                time = 3000;
                break;
            case BOTTOM_VIEW_2:
                time = 3000;
                break;
            case BOTTOM_VIEW_3:
                time = 3000;
                break;
            case BOTTOM_VIEW_4:
                time = 3000;
                break;
            case BOTTOM_VIEW_5:
                break;
            }
        }
        m_bottomViewDeadline = GameTime::get() + time;
    }
}

VA(0x00415de0, 0x140)  // dc 0x18d38
void AdvManager::updBottomView(unsigned char forceUpdate, unsigned char drawWindow, unsigned char update)
{
    if (m_bottomViewOverride == BOTTOM_VIEW_8)
        return;

    unsigned long deadline = m_bottomViewDeadline;
    if (static_cast<long>(GameTime::get() - deadline) >= 0)
        m_bottomViewOverride = BOTTOM_VIEW_DEFAULT;

    int changed = 0;
    if (m_bottomViewOverride != BOTTOM_VIEW_DEFAULT) {
        switch (m_bottomViewOverride) {
        case BOTTOM_VIEW_1:
            changed = updBottomViewNewTurn(forceUpdate);
            break;
        case BOTTOM_VIEW_2:
            changed = updBottomViewKingdom(forceUpdate);
            break;
        case BOTTOM_VIEW_6:
            changed = updBottomViewResMsg(forceUpdate);
            break;
        case BOTTOM_VIEW_7:
            changed = updBottomViewMessage(forceUpdate);
            break;
        case BOTTOM_VIEW_3:
            changed = updBottomViewHero(forceUpdate);
            break;
        case BOTTOM_VIEW_4:
            changed = updBottomViewTown(forceUpdate);
            break;
        }
    } else if (g_currentPlayer->isLocalHuman() && !g_completeDrawAllCells) {
        if (g_currentPlayer->m_currHeroId != -1)
            changed = updBottomViewHero(forceUpdate);
        else if (g_currentPlayer->m_currTownId != -1)
            changed = updBottomViewTown(forceUpdate);
        else
            changed = updBottomViewNewTurn(forceUpdate);
    } else {
        changed = updBottomViewEnemyTurn(forceUpdate);
    }

    if (changed && drawWindow)
        m_advWindow->drawBottomView(update != 0);
}

VA(0x00415f20, 0x87)  // dc 0x18f48
unsigned char AdvManager::updBottomViewEnemyTurn(unsigned char forceUpdate)
{
    unsigned char changed = 0;

    if (m_bottomViewType != BOTTOM_VIEW_5) {
        changed = 1;
        m_advWindow->clearBottomView();
        m_bottomViewType = BOTTOM_VIEW_5;
        m_advWindow->setBottomView(new BottomViewEnemyTurn(m_advWindow));
    }

    return changed;
}

VA(0x00415fb0, 0xB0)  // dc 0x18fc4
unsigned char AdvManager::updBottomViewNewTurn(unsigned char forceUpdate)
{
    if (!forceUpdate && m_bottomViewType == BOTTOM_VIEW_1) {
        m_advWindow->animateBottomView(0);
        return 0;
    }

    m_advWindow->clearBottomView();
    m_bottomViewType = BOTTOM_VIEW_1;
    m_advWindow->setBottomView(new BottomViewNewTurn(m_advWindow));
    m_advWindow->updateResourceDisplay(1, 1);
    return 1;
}

// E:\gamedcs\advmgr.cpp:8968
// Residual (93.33%): identical to BVMessage's one string-library choice -
// retail calls basic_string::_Tidy(0) for an empty shared string while this
// invocation clears the representation inline. All resource stores, override
// timing, forced refresh and resource-display update instructions agree.
VA(0x00416060, 0xF7)  // anchor-global, dc 0x19098
void AdvManager::bvResMsg(const char* msg, int resType, int resQty)
{
    m_bottomViewResourceType = resType;
    m_bottomViewResourceQuantity = resQty;
    // MEASURED NEGATIVE, do not retry: `#pragma inline_depth(0)` on this
    // assignment, to chase retail's out-of-line basic_string::_Tidy(0)
    // (base x2 vs retail x3), costs 93.33 -> 26.42. Retail INLINES the
    // assign here and only its _Tidy tail is out of line, and a statement
    // pin cannot express "inline the parent, call the child".
    m_bottomViewMessage = msg;
    m_bottomViewOverride = BOTTOM_VIEW_6;
    m_bottomViewDeadline = GameTime::get() + 5000;
    g_advManager->updBottomView(1, 1, 1);
    m_advWindow->updateResourceDisplay(1, 1);
}

VA(0x00416160, 0xAF)  // dc 0x190fc
unsigned char AdvManager::updBottomViewResMsg(unsigned char forceUpdate)
{
    if (!forceUpdate && m_bottomViewType == BOTTOM_VIEW_6)
        return 0;

    m_advWindow->clearBottomView();
    m_bottomViewType = BOTTOM_VIEW_6;
    m_advWindow->setBottomView(new BottomViewResourceMessage(
        m_advWindow, m_bottomViewResourceType, m_bottomViewResourceQuantity,
        &m_bottomViewMessage));
    return 1;
}

// E:\gamedcs\advmgr.cpp:8994
// Residual (92.68%): the complete string assignment and update tail match;
// only the empty shared-string arm differs. Retail decrements the reference
// count and calls basic_string::_Tidy(0), while this VC6 invocation clears the
// three representation words inline. operator=, assign(const char*), and
// assign(const char*, length) are byte-identical; iterator-range assign is
// worse and was rejected.
VA(0x00416210, 0xD7)  // anchor-global, dc 0x19194
void AdvManager::bvMessage(const char* msg)
{
    // MEASURED NEGATIVE, do not retry: same pin as BVResMsg above, same
    // reason - it costs 92.68 -> 21.28 here.
    m_bottomViewMessage = msg;
    m_bottomViewOverride = BOTTOM_VIEW_7;
    m_bottomViewDeadline = GameTime::get() - 1;
    g_advManager->updBottomView(1, 1, 1);
}

VA(0x004162f0, 0xA1)  // dc 0x191d0
unsigned char AdvManager::updBottomViewMessage(unsigned char forceUpdate)
{
    if (!forceUpdate && m_bottomViewType == BOTTOM_VIEW_7)
        return 0;

    m_advWindow->clearBottomView();
    m_bottomViewType = BOTTOM_VIEW_7;
    m_advWindow->setBottomView(
        new BottomViewMessage(m_advWindow, &m_bottomViewMessage));
    return 1;
}

VA(0x004163a0, 0xA6)  // dc 0x1927c
unsigned char AdvManager::updBottomViewKingdom(unsigned char forceUpdate)
{
    if (!forceUpdate && m_bottomViewType == BOTTOM_VIEW_2)
        return 0;

    m_advWindow->clearBottomView();
    m_bottomViewType = BOTTOM_VIEW_2;
    m_advWindow->setBottomView(new BottomViewKingdom(m_advWindow));
    m_advWindow->updateResourceDisplay(1, 1);
    return 1;
}

VA(0x00416450, 0x9A)  // dc 0x1930c
unsigned char AdvManager::updBottomViewHero(unsigned char forceUpdate)
{
    if (!forceUpdate && m_bottomViewType == BOTTOM_VIEW_3)
        return 0;

    m_advWindow->clearBottomView();
    m_bottomViewType = BOTTOM_VIEW_3;
    m_advWindow->setBottomView(new BottomViewHero(m_advWindow));
    return 1;
}

VA(0x004164f0, 0x9A)  // dc 0x19388
unsigned char AdvManager::updBottomViewTown(unsigned char forceUpdate)
{
    if (!forceUpdate && m_bottomViewType == BOTTOM_VIEW_4)
        return 0;

    m_advWindow->clearBottomView();
    m_bottomViewType = BOTTOM_VIEW_4;
    m_advWindow->setBottomView(new BottomViewTown(m_advWindow));
    return 1;
}

// E:\gamedcs\advmgr.cpp:9063
// DC carries this free helper out of line (dc 0x19420, 0x9C); retail's
// /Ob2 inlines the static into every quick-view caller and drops the body.
DC_ONLY(0x19420, 0x9C)
static SkillMastery getIdentifyLevel(type_point point)
{
    int identifyLevel = eMasteryInvalid;
    PlayerData* player = &g_game->m_players[g_unnamed69778c];

    for (int i = 0; i < player->m_numHeroes; i++) {
        Hero* currentHero = g_game->getHero(player->m_heroes[i]);
        if (currentHero->heroFn004E5DE0() > identifyLevel
            && currentHero->isInIdentifyRange(&point))
            identifyLevel = currentHero->heroFn004E5DE0();
    }
    return (SkillMastery)identifyLevel;
}

VA(0x00416590, 0x210)  // dc 0x194bc
void AdvManager::heroQuickView(int heroId, int x, int y,
                               unsigned char displayDropShadow)
{
    Hero* theHero = g_game->getHero(heroId);
    type_point heroPoint(theHero->m_x, theHero->m_y, theHero->m_z);

    SkillMastery identifyLevel = getIdentifyLevel(heroPoint);

    QuickHeroWindow::ViewLevel level;
    if (g_game->onSameTeam(theHero->m_owner, g_game->getLocalPlayerGamePos())
        || identifyLevel >= eMasteryAdvanced || m_debugViewAll)
        level = QuickHeroWindow::ViewAll;
    else
        level = QuickHeroWindow::ViewSome;

    QuickHeroWindow window(theHero, level);
    window.m_x = limit(window.m_width / 2, x,
                     WINDOW_SCREEN_WIDTH - 1 - window.m_width / 2)
               - window.m_width / 2;
    window.m_y = limit(window.m_height / 2, y,
                     WINDOW_SCREEN_HEIGHT - 1 - window.m_height / 2)
               - window.m_height / 2;
    if (!displayDropShadow)
        window.m_type &= ~WINDOW_FLAG_SHADOWED;
    window.quickWindowWait();
}

// castle.obj's building-name reader, declared file locally (the
// AI_approximate_strength precedent).
const char* getBuildingName(int townType, int buildingId);

// E:\gamedcs\advmgr.cpp:9115

VA(0x004167a0, 0x7DB)  // anchor-callee, dc 0x19674
void AdvManager::townQuickView(int townId, int x, int y,
                               unsigned char displayDropShadow)
{
    if (townId == -1)
        return;

    int localPos = g_game->getLocalPlayerGamePos();
    // CONST, and the bytes require it: retail's four get_army() calls below
    // are ?get_army@town@@QBEABVarmyGroup@@XZ, the const overload, which is
    // a DIFFERENT function at a different address from the non-const one a
    // plain `town*` binds (town.h models both; DC 0x168bf8 / 0x168bd0).
    // `predict-inline` reports the pair as get_army base x0 vs retail x4 -
    // a mangled-name divergence its OVER-inline bucket cannot tell apart
    // from an inlining decision. TQuickTownWindow's ctor already takes
    // `const town*`, so this costs no extra declarator. MEASURED BYTE-FLAT
    // (85.3187 either way): objdiff does not gate on the reloc's symbol
    // name, so this is a fidelity fix - it makes the object reference the
    // function retail references - and it retires a false OVER-inline row
    // that would otherwise send the next lane after an inliner knob.
    const Town* thisTown = &g_game->m_towns[townId];
    type_point point(thisTown->m_mapX, thisTown->m_mapY, thisTown->m_mapZ);

    SkillMastery identifyLevel = SkillMastery(getIdentifyLevel(point));

    if (m_debugViewAll && thisTown->m_owner != g_netLocalGamePos) {
        std::string text;
        PlayerData* ownerPlayer = &g_game->m_players[thisTown->m_owner];
        unsigned char first = 1;

        text = thisTown->m_name;
        text += "\n\n";
        if (thisTown->m_garrisonHeroId >= 0) {
            text += g_game->getHero(thisTown->m_garrisonHeroId)->m_name;
            first = 0;
        }

        for (long i = 0; i < ArmyGroup::ARMY_GROUP_SLOT_COUNT; i++) {
            if (thisTown->getArmy().m_armies[i] != -1) {
                if (!first)
                    text += ", ";
                first = 0;
                text += formatString(
                    "%i %s", thisTown->getArmy().m_numTroops[i],
                    getArmyName(thisTown->getArmy().m_armies[i],
                                thisTown->getArmy().m_numTroops[i]));
            }
        }

        if (!first)
            text += "\n\n";
        first = 1;
        for (int building = 0; building < MAX_BUILDING_TYPE; building++) {
            if (thisTown->m_built & g_bitNumber[building]) {
                int storage;
                storage = building;
                if (thisTown->isLegalBuilding(type_building_id(storage))) {
                    // Retail CALLS basic_string::append(const char*,
                    // size_type) at BOTH of this arm's appends - fn+0x349 for
                    // the separator and fn+0x371 for the name - with the
                    // strlen expanded in front of each as `repne scasb`, and
                    // our CL expanded the append too. `operator+=` reaches
                    // append(const char*) which reaches this two-argument one,
                    // so the site has to be spelled at the depth retail stops
                    // at before a statement pin can impose the call.
                    // MEASURED NEGATIVE, do not extend: the same treatment on
                    // the other seven `text += <literal>` sites in this block
                    // costs 90.9834 -> 73.8082. Retail calls append at THESE
                    // two and expands it at the rest.
                    if (!first) {
                        const char* sep = ", ";
                        size_t sepLen = strlen(sep);
#pragma inline_depth(0)
                        text.append(sep, sepLen);
#pragma inline_depth()
                    }
                    first = 0;
                    const char* buildingName =
                        getBuildingName(thisTown->m_type, building);
                    size_t buildingNameLen = strlen(buildingName);
#pragma inline_depth(0)
                    text.append(buildingName, buildingNameLen);
#pragma inline_depth()
                }
            }
        }

        text += "\n\n";
        for (int res = 0; res < 7; res++) {
            if (res > 0)
                text += ", ";
            text += formatString(
                "%i %s", ownerPlayer->m_resources[res], g_resourceNames[res]);
        }

        text += "\n\nIncome:\n";
        g_game->calculateProduction();
        first = 1;
        for (int inc = 0; inc < 7; inc++) {
            if (ownerPlayer->m_ai.m_turnProductionResource[inc] > 0) {
                if (!first)
                    text += ", ";
                first = 0;
                text += formatString(
                    "%i %s", ownerPlayer->m_ai.m_turnProductionResource[inc],
                    g_resourceNames[inc]);
            }
        }

        normalDialog(text.c_str(), 4, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return;
    }

    QuickTownWindow::ViewLevel level;
    if (g_game->onSameTeam(thisTown->m_owner, localPos)
        || identifyLevel == eMasteryExpert)
        level = QuickTownWindow::ViewAll;
    else if (g_game->getNumThievesGuilds(localPos) >= 2)
        level = QuickTownWindow::ViewArmySizes;
    else
        level = g_game->getNumThievesGuilds(localPos) >= 1
                    ? QuickTownWindow::ViewArmyTypes
                    : QuickTownWindow::ViewNone;

    QuickTownWindow window(thisTown, level);
    window.m_x = limit(window.m_width / 2, x,
                     WINDOW_SCREEN_WIDTH - 1 - window.m_width / 2)
               - window.m_width / 2;
    window.m_y = limit(window.m_height / 2, y,
                     WINDOW_SCREEN_HEIGHT - 1 - window.m_height / 2)
               - window.m_height / 2;
    if (!displayDropShadow)
        window.m_type &= ~WINDOW_FLAG_SHADOWED;
    window.quickWindowWait();
}

// E:\gamedcs\advmgr.cpp:9243
VA(0x00416f80, 0x1CD)  // anchor-callee, dc 0x19cdc
void AdvManager::garrisonQuickView(int id, int x, int y)
{
    if (id == -1)
        return;

    Garrison* const thisGarrison = &g_game->m_garrisons[id];
    type_point point(thisGarrison->m_mapX, thisGarrison->m_mapY,
                     thisGarrison->m_mapZ);

    SkillMastery identifyLevel = SkillMastery(getIdentifyLevel(point));

    QuickTownWindow::ViewLevel level;
    if (g_game->onSameTeam(thisGarrison->m_playerOwner, g_unnamed69778c)
        || m_debugViewAll || identifyLevel == eMasteryExpert)
        level = QuickTownWindow::ViewAll;
    else if (g_game->getNumThievesGuilds(g_unnamed69778c) >= 2)
        level = QuickTownWindow::ViewArmySizes;
    else
        level = g_game->getNumThievesGuilds(g_unnamed69778c) >= 1
                    ? QuickTownWindow::ViewArmyTypes
                    : QuickTownWindow::ViewNone;

    QuickTownWindow window(thisGarrison, level);
    window.center(x, y);
    window.quickWindowWait();
}

// The strength appraisal, declared file-locally (the events.cpp/townmgr.cpp
// precedent for ai_combat.obj's address).
long aiApproximateStrength(const Hero* currentHero);

VA(0x00417150, 0x2C9)  // dc 0x19e80
void AdvManager::monsterQuickView(const NewmapCell* cell, int cellx, int celly)
{
    const int count = cell->m_extraInfo & 0xfff;
    CreatureType type;
    {
        type = CreatureType(cell->m_objectIndex);
    }

    PlayerData* localPlayer = g_game->getLocalPlayer();
    g_game->getLocalPlayerGamePos();

    QuickCreatureWindow* window;
    bool showDetails = false;
    Hero* const currHero = g_game->getHero(localPlayer->m_currHeroId);
    if (currHero) {
        unsigned char inIdentifyRange;
        {
            type_point point(m_radarOrigin.m_x + cellx, m_radarOrigin.m_y + celly,
                             m_radarOrigin.m_z);
            inIdentifyRange = currHero->isInIdentifyRange(&point);
        }
        if ((inIdentifyRange
             && currHero->heroFn004E5DE0() != eMasteryInvalid)
            || m_debugViewAll) {
            int like = getLikeModifier(currHero, type);
            const int diplomacy = currHero->m_skillLevel[eSecSkillDiplomacy];
            const float strengthRatio =
                static_cast<float>(aiApproximateStrength(currHero))
                / static_cast<float>(g_creatureTypeTraits[type].m_aiValue
                                     * count);
            int force = getForceModifier(strengthRatio);
            QuickCreatureWindow::Disposition disposition =
                static_cast<QuickCreatureWindow::Disposition>(
                    static_cast<long>(cell->m_extraInfo << 15) >> 27);

            QuickCreatureWindow::Disposition mood;
            if (disposition > force + diplomacy + like)
                mood = QuickCreatureWindow::Attack;
            else if (disposition <= diplomacy + like + 1)
                mood = QuickCreatureWindow::Join;
            else if (disposition <= like + 2 * diplomacy + 1)
                mood = QuickCreatureWindow::JoinPrice;
            else if ((cell->m_extraInfo & 0x20000)
                     || disposition == force + diplomacy + like)
                mood = QuickCreatureWindow::Attack;
            else
                mood = QuickCreatureWindow::Flee;

            int cost = g_creatureTypeTraits[type].m_cost[6] * count;
            window = new QuickCreatureWindow(
                QuickCreatureWindow::ViewAll, type, count, mood, cost);
            showDetails = true;
        }
    }
    if (!showDetails) {
        window = new QuickCreatureWindow(QuickCreatureWindow::ViewNone, type,
                                          count, QuickCreatureWindow::Flee, 0);

    }
    window->m_x = limit(window->m_width / 2, cellx * 32,
                      WINDOW_SCREEN_WIDTH - 1 - window->m_width / 2)
                - window->m_width / 2;
    window->m_y = limit(window->m_height / 2, celly * 32,
                      WINDOW_SCREEN_HEIGHT - 1 - window->m_height / 2)
                - window->m_height / 2;
    window->quickWindowWait();
    if (window)
        delete window;
}

VA(0x00417420, 0x146)  // dc 0x1a230
void AdvManager::redrawAdvScreen(unsigned char update, unsigned char forceSaveBorder)
{
    const int playerId = g_game->getLocalPlayerGamePos();
    Bitmap816* const bmp = ResourceManager::getBitmap816("AdvMap.pcx");

    if (bmp) {
        setPlayerPaletteColors(bmp->m_p16.m_colors.m_data, playerId);
        bmp->draw(0, 0, bmp->getWidth(), bmp->getHeight(),
                  g_windowManager->m_screenBitmap, 0, 0, 0);
        bmp->dispose();
        m_heroLogoShowing = 0;
    }

    m_advWindow->updateButtons(0, 0);
    m_advWindow->updateHeroLocators(-1, 0, 0);
    m_advWindow->updateTownLocators(-1, 0, 0);
    m_advWindow->updateQuestLogButton(0);
    updBottomView(1, 0, 0);
    m_advWindow->updateResourceDisplay(1, 0);
    m_advWindow->drawWindow(0, -65535, 65535);
    m_advWindow->highlightLocators(0);
    completeDraw(m_radarOrigin.m_x, m_radarOrigin.m_y, m_radarOrigin.m_z, 0, 1);
    updateRadar(m_radarOrigin, 0, 1, 0, 0, 0);

    if (update)
        g_windowManager->updateScreen(0, 0, 800, 600);
}

VA(0x00417570, 0x2A)  // dc 0x1a3b0
void AdvManager::deactivateCurrTown(unsigned char waitingPlayer)
{
    if (waitingPlayer)
        g_game->getLocalPlayer()->m_currTownId = 0xff;
    else
        g_currentPlayer->m_currTownId = 0xff;
}

VA(0x004175a0, 0x3A)  // dc 0x1a3f0
void AdvManager::deactivateCurrHero(unsigned char waitingPlayer)
{
    demobilizeCurrHero(waitingPlayer, 0);
    if (waitingPlayer)
        g_game->getLocalPlayer()->m_currHeroId = -1;
    else
        g_currentPlayer->m_currHeroId = -1;
}

VA(0x004175e0, 0x9D)  // dc 0x1a440
void AdvManager::mobilizeCurrHero(int inMove, unsigned char waitingPlayer, unsigned char drawChanges)
{
    PlayerData* player = g_currentPlayer;

    if (waitingPlayer)
        player = g_game->getLocalPlayer();
    else if (m_curHeroMobile)
        return;

    if (player->m_currHeroId == -1) {
        int heroId = player->nextHero();
        int tID = player->nextTown();

        if (heroId != -1)
            setHeroContext(heroId, inMove, waitingPlayer, drawChanges);
        else if (tID != -1)
            setTownContext(tID, waitingPlayer, drawChanges);
    } else {
        setHeroContext(player->m_currHeroId, inMove, waitingPlayer,
                       drawChanges);
    }
}

VA(0x00417680, 0x1AF)  // dc 0x1a520
void AdvManager::demobilizeCurrHero(unsigned char waitingPlayer,
                                    unsigned char drawChanges)
{
    if (!waitingPlayer && g_currentPlayer
        && g_currentPlayer->m_currHeroId != -1 && m_curHeroMobile) {
        m_curHeroMobile = 0;
        Hero* currHero;
        if (g_currentPlayer->m_currHeroId != -1)
            currHero = &g_game->m_heroes[g_currentPlayer->m_currHeroId];
        else
            currHero = 0;
        stopCursor(1);
        currHero->obscureCell();

        type_point point;
        point = type_point(currHero->m_x, currHero->m_y, currHero->m_z);
        getCell(point);

        currHero->m_facing = m_cursorDirection;
        m_drawCursor = 0;

        if (!g_unnamed6aac3c && drawChanges && g_completeDrawEnabled) {
            completeDraw(m_radarOrigin.m_x, m_radarOrigin.m_y, m_radarOrigin.m_z, 0, 1);
            g_windowManager->updateScreen(0, 8, 608, 544);

            unsigned long now = GameTime::get();
            if (static_cast<long>(
                    now - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT])
                    >= 0
                && !m_animCtrPaused) {
                m_animCtr++;
                g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT] += cppMax(
                    static_cast<long>(
                        now
                        - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT]),
                    180L);
            }
            process1WindowsMessage();
        }
    }
}

// E:\gamedcs\advmgr.cpp:9476
// Makes a town the adventure view's subject: drop any mobile hero, clear
// the current-hero slot, centre the radar on the town and repoint the
// locator strip, the spell/sleep buttons and the ambient music at it.
// Retail resolves the acting player TWICE and caches neither, but spells
// the two differently, which the bytes insist on: the first is a ternary,
// the second an if-assignment over a gpCurrentPlayer default, re-reading
// the parameter off the stack (85.39 -> 88.13 for that one line). It also
// reaches RedrawAdvScreen through the gpAdvManager global, not `this`.

// Residual (88.13%): instruction selection only - branch sequences agree
// mnemonic-for-mnemonic (14 branches, 1 return) and the call multisets are
// identical; what is left is the register choice in the GetTown index
// chain and reloc-name-only rows on data.
VA(0x00417830, 0x2EB)  // anchor-global, dc 0x1a65c
void AdvManager::setTownContext(int townId, unsigned char waitingPlayer, unsigned char update)
{
    demobilizeCurrHero(waitingPlayer, 0);

    PlayerData* heroOwner = waitingPlayer ? g_game->getLocalPlayer()
                                          : g_currentPlayer;
    heroOwner->m_currHeroId = -1;

    PlayerData* player = g_currentPlayer;
    if (waitingPlayer)
        player = g_game->getLocalPlayer();
    player->m_currTownId = townId;

    Town* currTown = g_game->getTown(player->m_currTownId);
    m_radarOrigin.m_x = currTown->m_mapX - 9;
    m_radarOrigin.m_y = currTown->m_mapY - 8;
    m_radarOrigin.m_z = currTown->m_mapZ;

    m_advWindow->setElevationToggleImage(m_radarOrigin.m_z);

    int townSlot = 0;
    int i;
    for (i = 0; i < player->m_numTowns; i++) {
        if (player->m_townIds[i] == townId)
            townSlot = i;
    }

    if (waitingPlayer || g_currentPlayer->isLocalHuman()) {
        m_advWindow->updateTownLocators(townSlot, 1, 0);
        m_advWindow->updateHeroLocators(-1, 1, 0);
        m_advWindow->updateSpellButton(0);
        m_advWindow->updateSleepButton(0);
    }

    if (g_currentPlayer->isLocalHuman()
        || (g_unnamed6989c8 && g_unnamed69ccd4)) {
        g_windowManager->broadcastMessage(
            MESSAGE_WIDGET, Widget::WIDGET_SET_STATUS,
            AdventureMapWindow::MOVE_ID,
            Widget::WIDGET_UPDATE | Widget::WIDGET_DIMMED);
        if (m_showRoute)
            m_showRoute = 0;
    }

    g_advManager->redrawAdvScreen(update, 0);

    type_point point(m_radarOrigin.m_x + 9, m_radarOrigin.m_y + 8, m_radarOrigin.m_z);
    setEnvironmentOrigin(point, 1);

    point.m_x = currTown->m_mapX;
    point.m_y = currTown->m_mapY;
    point.m_z = currTown->m_mapZ;

    int ground = getCell(point)->m_groundSet;
    if (ground != m_lastTerrain) {
        m_lastTerrain = ground;
        g_soundManager->switchAmbientMusic(g_terrainMusicIds[ground]);
    }

    g_inputManager->forceMouseMove();
    m_lastHoverX = 0;
}

// E:\gamedcs\advmgr.cpp:9544
// Makes a hero the acting one: drops the old town and hero selection,
// recentres the view on the new hero, refreshes the whole button strip and
// reseeds his route. ProcessSelect, ProcessDeSelect and ProcessMapSelect are
// its three callers in this file and they fix the parameter roles.

// Two shapes are worth naming:
//   * retail INLINES MapExtraPosAndAdjacentsSet (0x41a750, reconstructed
//     further down this file) for the visibility probe around the hero -
//     the whole `GetMapExtra` + 3x3 neighbour double loop is expanded in
//     place, and its out-of-line body still exists because the callee has
//     extern linkage.
//   * The three points are THREE SEPARATE block-scoped type_points, even
//     though retail writes all of them through the same [ebp-0x20] slot.
//     One function-scope point keeps that slot live across the visibility
//     scan, so the scan's loop bound needs a slot of its own (frame 0x2c
//     against retail's 0x28) and the spill cascades from there: found
//     loses EBX to the parameter slot and the scan's x loses its register
//     too. Scoping the three lets VC6 coalesce them onto one slot AND
//     overlap the loop bound on it, exactly as retail does. 90.56 -> 99.27.

// Residual (99.27%): two instructions in the route-target write, and the
// cause is a CSE our CL makes and retail does not. Both sides load
// pathTargetX as a DWORD for the `>= 0` guard. Retail then loads the
// point's old word into AX first, clobbering that register, so the x
// insert has to re-read pathTargetX from memory
// (`mov ax,[ebp-0x20] / ... / mov cx,ax / xor cx,[edi+0x35]`); our CL
// schedules `mov cx,ax` first and keeps the guard's value, then loads the
// old word (`mov cx,ax / mov ax,[ebp-0x20] / xor cx,ax`). Commutative and
// identical in effect - it is the SCHEDULE, not the operand order, that
// differs. Eleven spellings measured and rejected: named int locals for the
// three coordinates (96.14), the same as shorts (99.27), y/x/z order
// (96.89), z/y/x order (96.73), an explicit short cast (99.27), sharing one
// point between the route target and the view centre (99.27), a `!= -1`
// guard (99.02), a `!(x < 0)` guard (99.27), and hoisting the point's
// declaration above the guard (99.27), default construction followed by
// separate x/y/z assignments (99.27), and eliding the named routeTarget
// temporary at the SeedTo call (99.27). Dreamcast nevertheless proves the
// named hero::get_target boundary before SeedTo; restoring that inline
// helper is byte-flat and is source-shape truth rather than a score lever.
VA(0x00417b20, 0x63E)  // anchor-global, dc 0x1a878
void AdvManager::setHeroContext(int heroId, int inMove, unsigned char waitingPlayer, unsigned char drawChanges)
{
    if (heroId == -1)
        return;

    if (waitingPlayer) {
        g_game->getLocalPlayer()->m_currTownId = -1;
    } else {
        g_currentPlayer->m_currTownId = -1;
        if (drawChanges) {
            if (g_currentPlayer->isLocalHuman()
                || (g_unnamed6989c8 && g_unnamed69ccd4)) {
                g_windowManager->broadcastMessage(
                    MESSAGE_WIDGET, Widget::WIDGET_SET_STATUS,
                    AdventureMapWindow::MOVE_ID,
                    Widget::WIDGET_UPDATE | Widget::WIDGET_DIMMED);
                if (m_showRoute)
                    m_showRoute = 0;
            }
        }
    }

    deactivateCurrHero(waitingPlayer);

    PlayerData* player = g_currentPlayer;
    if (waitingPlayer)
        player = g_game->getLocalPlayer();
    else
        m_curHeroMobile = 1;

    player->m_currHeroId = heroId;

    Hero* curr = &g_game->m_heroes[heroId];
    NewmapCell* cell;
    {
        type_point heroPoint(curr->m_x, curr->m_y, curr->m_z);

        type_point cellPoint = heroPoint;
        if (!cellPoint.isValid())
            cell = m_fullMap->cell(0, 0, 0);
        else
            // Use the recovered cell helper; retail chooses its expansion.
            cell = m_fullMap->cell(cellPoint.m_x, cellPoint.m_y, cellPoint.m_z);
    }

    if (!waitingPlayer) {
        m_cursorType = (curr->m_flags & 0x40000) ? CURSOR_TYPE_8
                                             : CURSOR_TYPE_34;
        m_cursorDirection = curr->m_facing;
        m_cursorSequence = curr->getStandSequence();
        m_cursorFrameCount = 0;
        if (g_videoPaused && !g_followPlayerMode && !g_completeDrawMessageBypass) {
            if (g_currentPlayer->isHuman()) {
                if (!g_currentPlayer->isLocalHuman())
                    return;
            } else if (!g_unnamed691209) {
                if (!g_game->isLastHuman(g_game->getLocalPlayerGamePos()))
                    return;
            }
        }
        curr->restoreCell();
    }

    unsigned char screenRedrawn = 0;
    if ((player->isLocalHuman() || !g_unnamed698790)
        && mapExtraPosAndAdjacentsSet(curr->m_x, curr->m_y, curr->m_z,
                                      g_mapVisibilityBit)) {
        if (drawChanges)
            g_completeDrawEnabled = 1;
        if (!waitingPlayer && drawChanges)
            m_drawCursor = 1;
        m_radarOrigin.m_x = curr->m_x - 9;
        m_radarOrigin.m_y = curr->m_y - 8;
        m_radarOrigin.m_z = curr->m_z;
        if (m_advWindow->setElevationToggleImage(m_radarOrigin.m_z))
            g_advManager->redrawAdvScreen(0, 0);
    }

    int found = player->findHero(heroId);
    if (found == -1)
        found = 0;

    if (waitingPlayer
        || (drawChanges && g_currentPlayer->isLocalHuman()
            && !g_completeDrawMessageBypass)) {
        m_advWindow->updateHeroLocators(found, drawChanges, 0);
        m_advWindow->updateTownLocators(-1, drawChanges, 0);
        m_advWindow->updateSpellButton(curr);
        m_advWindow->setSleepImage(curr->m_isSleeping);
        m_advWindow->updateSleepButton(curr);
        m_advWindow->updateResourceDisplay(1, 0);
        m_advWindow->drawWindow(0, -65535, 65535);
    }

    if (drawChanges && !inMove
        && (m_status == STATUS_ACTIVE || g_currentPlayer->isLocalHuman())) {
        m_seedingValid = 0;
        if (curr->m_pathTargetX >= 0) {
            type_point routeTarget = curr->getTarget();
            seedTo(routeTarget);
        }
        showRoute(0, 0, 1);
    }

    if (player->isLocalHuman() && drawChanges && !g_completeDrawMessageBypass)
        updBottomView(1, 1, 0);

    if (g_currentPlayer->isLocalHuman())
        m_drawCursor = 1;

    if (drawChanges && g_completeDrawEnabled) {
        updateRadar(m_radarOrigin, 0, 1, 0, 0, 0);
        completeDraw(m_radarOrigin.m_x, m_radarOrigin.m_y, m_radarOrigin.m_z, 0, 1);
        g_windowManager->updateScreen(0, 0, HOVER_SCREEN_WIDTH,
                                      HOVER_SCREEN_HEIGHT);
    }

    // The same merged y|z store ProcessDeSelect's kingdom-overview arm
    // needs: both values have to be in hand before the first bitfield
    // write or VC6 emits two read-modify-write inserts instead of one
    // masked store.
    type_point viewCentre;
    int centreX = m_radarOrigin.m_x + 9;
    int centreY = m_radarOrigin.m_y + 8;
    int centreZ = m_radarOrigin.m_z;
    viewCentre.m_x = centreX;
    viewCentre.m_y = centreY;
    viewCentre.m_z = centreZ;
    setEnvironmentOrigin(viewCentre, 1);

    if (cell->m_groundSet != m_lastTerrain && drawChanges) {
        m_lastTerrain = cell->m_groundSet;
        g_soundManager->switchAmbientMusic(g_terrainMusicIds[m_lastTerrain]);
    }

    if (!m_heroMoving && drawChanges) {
        g_inputManager->forceMouseMove();
        m_lastHoverX = 0;
    }
}

VA(0x00418160, 0x270)  // dc 0x1ae38
unsigned char saveGame(unsigned char campaignWinMode)
{
    unsigned char result = 0;
    int humanCount = 0;
    if (!campaignWinMode) {
        g_advManager->disableButtons();
        for (int i = 0; i < 8; i++) {
            if (!g_game->m_playerDisabled[i] && g_game->isHuman(i))
                humanCount++;
        }
    }
    g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);

    if (g_unnamed69774c)
        sprintf(g_unnamed691268,
                DATA_COMPGEN(0x006603f8, campaignSaveExtension, ".CGM"));
    else
        sprintf(g_unnamed691268,
                DATA_COMPGEN(0x006603f0, saveExtensionFormat, ".GM%d"),
                humanCount);

    {
        SingleSelectionWindow selection(2);
        selection.doModal(0);
    }

    if (!campaignWinMode)
        g_advManager->redrawAdvScreen(1, 0);

    if (strlen(g_unnamed69fc2c) != 0) {
        strcat(g_unnamed69fc2c, g_unnamed691268);
        g_unnamed697774 = 1;
        int oldPos = g_netLocalGamePos;
        if (g_videoPaused) {
            int pos = g_game->getLocalPlayerGamePos();
            g_netLocalGamePos = pos;
            g_currentPlayer = &g_game->m_players[pos];
            g_unnamed69ccc4 = 1 << pos;
        }
        CHourGlass hourGlass(1);
        result = g_game->saveGame(g_unnamed69fc2c, g_game->m_isTutorial,
                                  campaignWinMode, 1, 0);
        hourGlass.stop();
        if (g_videoPaused) {
            g_netLocalGamePos = oldPos;
            g_currentPlayer = &g_game->m_players[oldPos];
            g_unnamed69ccc4 = 1 << oldPos;
        }
        if (result) {
            strtok(g_unnamed69fc2c,
                   DATA_COMPGEN(0x006603ec, saveExtensionDot, "."));
            char text[100];
            sprintf(text, g_generalText->getText(351), g_unnamed69fc2c);
            normalDialog(text, 1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        }
    }

    if (!campaignWinMode)
        g_advManager->enableButtons();
    return result;
}

// Per-priority playback volume for the adventure ambience. The row is
// retail .rdata local to this TU (name provisional; the akScrollSpeedInc
// pattern). It sits HERE, ahead of SetEnvironmentOrigin, because that is
// the first of its two readers in retail's source order (advmgr.cpp:9785
// against InsertSound's 10372) - it cannot have been declared any later.
DATA(0x0063a64c) static const int g_soundVolumes[8] = { 32, 28, 20, 10,
                                                        3,  2,  1,  0 };

VA(0x004183d0, 0x245)  // dc 0x1b164
void AdvManager::setEnvironmentOrigin(type_point point, int reset)
{
    const int maxRange = 4;
    if (!g_soundManager->m_playSounds)
        return;

    int i;
    for (i = 0; i < ADVENTURE_ACTIVE_SOUND_COUNT; i++) {
        if (m_soundArray[i].m_soundId != LOOPING_SOUND_INVALID) {
            if (reset) {
                g_soundManager->stopSample(
                    m_loopedSample[m_soundArray[i].m_soundId]->m_memSample.m_memSampleHandle);
                m_soundArray[i].m_soundId = LOOPING_SOUND_INVALID;
                m_soundArray[i].m_priority = 0x7f;
            } else {
                m_soundArray[i].m_priority = 0x7f;
            }
        }
    }

    if (point.m_x == -1)
        return;

    if (!g_unk698764)
        return;

    m_touchedSounds = 0;

    int soundsType;
    for (soundsType = 1; soundsType <= 2; soundsType++) {
        insertSound(point.m_x, point.m_y, point.m_z, 0, soundsType);

        for (int priority = 0; priority < maxRange; ++priority) {
            for (i = 0; i < 2 * priority; ++i) {
                insertSound(point.m_x - priority + i, point.m_y - priority,
                            point.m_z, priority, soundsType);
                insertSound(point.m_x + priority, point.m_y - priority + i,
                            point.m_z, priority, soundsType);
                insertSound(point.m_x + priority - i, point.m_y + priority,
                            point.m_z, priority, soundsType);
                insertSound(point.m_x - priority, point.m_y + priority - i,
                            point.m_z, priority, soundsType);
            }
        }
    }

    for (i = 0; i < ADVENTURE_ACTIVE_SOUND_COUNT; i++) {
        if (m_soundArray[i].m_soundId != LOOPING_SOUND_INVALID
            && m_soundArray[i].m_priority > 5) {
            g_soundManager->stopSample(
                m_loopedSample[m_soundArray[i].m_soundId]->m_memSample.m_memSampleHandle);
            m_soundArray[i].m_soundId = LOOPING_SOUND_INVALID;
        }
        if (m_soundArray[i].m_soundId != LOOPING_SOUND_INVALID
            && (m_touchedSounds & (1 << m_soundArray[i].m_soundId))) {
            g_soundManager->modifySample(
                m_loopedSample[m_soundArray[i].m_soundId]->m_memSample.m_memSampleHandle, 100,
                g_soundVolumes[m_soundArray[i].m_priority]);
        }
    }
}

#if 0  // @carcass

// E:\gamedcs\advmgr.cpp:9929
DC_ONLY(0x1b520, 0x88)
void AdvManager::CheckLoadSample(ELoopingSoundId id_num)
{
    // @stub
}

#endif  // @carcass

// E:\gamedcs\advmgr.cpp:9945
// 89.50 -> 93.76 (2026-08-21): the DC line table proves two source shapes
// that combine non-linearly on x86. The MINE arm declares `type` and
// `abandoned` from two separate `mines[extraInfo]` expressions; that makes
// the mine load/test sequence instruction-exact. The non-trigger tail is a
// switch on object type with a nested switch on special terrain, not an if
// chain; that prevents VC6 from lowering both decisions to branchless
// arithmetic and gives retail's test/cmp control flow. The mine locals alone
// are slightly negative (89.44), and the nested switches alone reach 92.88;
// together they reach 93.76. Replacing the two two-value nested switches
// (GARRISON and CREATURE_GENERATOR_4) with their DC-looking if chains was
// also measured and rejected on the pre-tail shape (88.37).

// 93.7570 -> 94.5074 (2026-08-21): THAT REJECTION EXPIRED WITH THE TAIL
// REWRITE. Re-measured on the current shape, the two two-value if-chains now
// PAY. The bytes name them directly: retail dispatches GARRISON with
// `mov cx, word ptr [ecx+0x22] / test cx,cx / jne / cmp cx,1`, a word-width
// if-chain with INLINE arms, where a `switch` forces `movsx eax,word / sub /
// je / dec` because a jump table needs the index sign-extended. Equality
// against a short compares at 16 bits; a switch cannot.

VA(0x00418620, 0x5E4)  // anchor-global, dc 0x1b5a8
ELoopingSoundId AdvManager::getSoundId(int x, int y, int z)
{
    NewmapCell* thisCell = m_fullMap->cell(x, y, z);

    if (thisCell->m_groundSet == eTerrainWater && thisCell->m_groundIndex < 21)
        return LOOPING_SOUND_69;

    if (thisCell->m_isTrigger) {
        switch (thisCell->m_type) {
        case CREATURE_BANK:
            switch (thisCell->m_objectIndex) {
            case GET_SOUND_BANK_0: return LOOPING_SOUND_7;
            case GET_SOUND_BANK_1: return LOOPING_SOUND_50;
            case GET_SOUND_BANK_2: return LOOPING_SOUND_19;
            case GET_SOUND_BANK_3: return LOOPING_SOUND_14;
            case GET_SOUND_BANK_4: return LOOPING_SOUND_59;
            case GET_SOUND_BANK_5: return LOOPING_SOUND_60;
            case GET_SOUND_BANK_6: return LOOPING_SOUND_23;
            default: return LOOPING_SOUND_INVALID;
            }
        case MINE: {
            int type = g_game->m_mines[thisCell->m_extraInfo].m_type;
            unsigned char abandoned =
                g_game->m_mines[thisCell->m_extraInfo].m_isAbandoned;
            if (abandoned)
                return LOOPING_SOUND_7;
            switch (type) {
            case GET_SOUND_MINE_0: return LOOPING_SOUND_24;
            case GET_SOUND_MINE_1: return LOOPING_SOUND_39;
            case GET_SOUND_MINE_2:
            case GET_SOUND_MINE_3: return LOOPING_SOUND_40;
            case GET_SOUND_MINE_4: return LOOPING_SOUND_47;
            case GET_SOUND_MINE_5: return LOOPING_SOUND_17;
            case GET_SOUND_MINE_6: return LOOPING_SOUND_29;
            default: return LOOPING_SOUND_INVALID;
            }
        }
        case GARRISON:
            if (thisCell->m_objectIndex == GET_SOUND_GARRISON_0)
                return LOOPING_SOUND_41;
            if (thisCell->m_objectIndex == GET_SOUND_GARRISON_1)
                return LOOPING_SOUND_25;
            break;
        case WINDMILL:
            return LOOPING_SOUND_66;
        case WHIRLPOOL:
            return LOOPING_SOUND_67;
        case THIEVES_DEN:
            return LOOPING_SOUND_49;
        case ARENA:
            return LOOPING_SOUND_2;
        case SIREN:
            return LOOPING_SOUND_62;
        case BUOY:
            return LOOPING_SOUND_5;
        case CAMPFIRE:
            return LOOPING_SOUND_6;
        case UNDERGROUND_GATE:
            return LOOPING_SOUND_55;
        case FOUNTAIN_OF_YOUTH:
            return LOOPING_SOUND_13;
        case RALLY_FLAG:
            return LOOPING_SOUND_15;
        case MYSTICAL_GARDEN:
            return LOOPING_SOUND_57;
        case FAERIE_RING:
            return LOOPING_SOUND_53;
        case BLACK_MARKET:
        case TRADING_POST:
            return LOOPING_SOUND_26;
        case TAVERN:
            return LOOPING_SOUND_64;
        case MERC_CAMP:
        case REFUGEE_CAMP:
            return LOOPING_SOUND_27;
        case WATER_WHEEL:
            return LOOPING_SOUND_28;
        case LITH_ONEWAY_ENTRANCE:
        case LITH_ONEWAY_EXIT:
            return LOOPING_SOUND_30;
        case LITH_TWOWAY:
            return LOOPING_SOUND_31;
        case SHRINE1:
        case SHRINE2:
        case SHRINE3:
            return LOOPING_SOUND_38;
        case POWER_SCHOOL:
            return LOOPING_SOUND_39;
        case CREATURE_GENERATOR_1:
            switch (g_creatureGenerator1Types[thisCell->m_objectIndex]) {
            case GET_SOUND_CREATURE_106:
            case GET_SOUND_CREATURE_108: return LOOPING_SOUND_33;
            case GET_SOUND_CREATURE_096: return LOOPING_SOUND_3;
            case GET_SOUND_CREATURE_010:
            case GET_SOUND_CREATURE_014: return LOOPING_SOUND_21;
            case GET_SOUND_CREATURE_112: return LOOPING_SOUND_46;
            case GET_SOUND_CREATURE_012: return LOOPING_SOUND_37;
            case GET_SOUND_CREATURE_054: return LOOPING_SOUND_9;
            case GET_SOUND_CREATURE_104: return LOOPING_SOUND_23;
            case GET_SOUND_CREATURE_016: return LOOPING_SOUND_50;
            case GET_SOUND_CREATURE_113: return LOOPING_SOUND_51;
            case GET_SOUND_CREATURE_018: return LOOPING_SOUND_52;
            case GET_SOUND_CREATURE_086: return LOOPING_SOUND_68;
            case GET_SOUND_CREATURE_084: return LOOPING_SOUND_56;
            case GET_SOUND_CREATURE_044:
            case GET_SOUND_CREATURE_052: return LOOPING_SOUND_65;
            case GET_SOUND_CREATURE_072: return LOOPING_SOUND_20;
            case GET_SOUND_CREATURE_046: return LOOPING_SOUND_10;
            case GET_SOUND_CREATURE_110: return LOOPING_SOUND_22;
            case GET_SOUND_CREATURE_080: return LOOPING_SOUND_58;
            case GET_SOUND_CREATURE_076: return LOOPING_SOUND_59;
            case GET_SOUND_CREATURE_078:
            case GET_SOUND_CREATURE_102: return LOOPING_SOUND_0;
            case GET_SOUND_CREATURE_008: return LOOPING_SOUND_32;
            case GET_SOUND_CREATURE_038: return LOOPING_SOUND_60;
            case GET_SOUND_CREATURE_090: return LOOPING_SOUND_61;
            case GET_SOUND_CREATURE_088:
            case GET_SOUND_CREATURE_098: return LOOPING_SOUND_34;
            case GET_SOUND_CREATURE_042:
            case GET_SOUND_CREATURE_050:
            case GET_SOUND_CREATURE_114: return LOOPING_SOUND_14;
            case GET_SOUND_CREATURE_026:
            case GET_SOUND_CREATURE_068:
            case GET_SOUND_CREATURE_082: return LOOPING_SOUND_11;
            case GET_SOUND_CREATURE_092: return LOOPING_SOUND_4;
            case GET_SOUND_CREATURE_028: return LOOPING_SOUND_18;
            case GET_SOUND_CREATURE_022: return LOOPING_SOUND_54;
            case GET_SOUND_CREATURE_115: return LOOPING_SOUND_16;
            case GET_SOUND_CREATURE_020: return LOOPING_SOUND_35;
            case GET_SOUND_CREATURE_024: return LOOPING_SOUND_44;
            case GET_SOUND_CREATURE_056: return LOOPING_SOUND_63;
            case GET_SOUND_CREATURE_058:
            case GET_SOUND_CREATURE_060:
            case GET_SOUND_CREATURE_062:
            case GET_SOUND_CREATURE_064:
            case GET_SOUND_CREATURE_066: return LOOPING_SOUND_8;
            case GET_SOUND_CREATURE_000: return LOOPING_SOUND_36;
            case GET_SOUND_CREATURE_002:
            case GET_SOUND_CREATURE_100: return LOOPING_SOUND_1;
            case GET_SOUND_CREATURE_004:
            case GET_SOUND_CREATURE_030: return LOOPING_SOUND_19;
            case GET_SOUND_CREATURE_034:
            case GET_SOUND_CREATURE_036: return LOOPING_SOUND_25;
            case GET_SOUND_CREATURE_048:
            case GET_SOUND_CREATURE_070:
            case GET_SOUND_CREATURE_074:
            case GET_SOUND_CREATURE_094: return LOOPING_SOUND_7;
            case GET_SOUND_CREATURE_040: return LOOPING_SOUND_43;
            default: return LOOPING_SOUND_42;
            }
        case CREATURE_GENERATOR_4:
            if (thisCell->m_objectIndex == GET_SOUND_GENERATOR4_0)
                return LOOPING_SOUND_43;
            if (thisCell->m_objectIndex == GET_SOUND_GENERATOR4_1)
                return LOOPING_SOUND_12;
            break;
        case DEFENSE_TOWER:
        case HILL_FORT:
        case WAR_SCHOOL:
            return LOOPING_SOUND_41;
        case DRAGON_CITY:
            return LOOPING_SOUND_11;
        case FOUNTAIN_OF_FORTUNE:
        case MAGIC_SPRING:
            return LOOPING_SOUND_16;
        case GARDEN_OF_REVELATION:
            return LOOPING_SOUND_54;
        case MAGIC_SCHOOL:
            return LOOPING_SOUND_25;
        case PILLAR_OF_FIRE:
            return LOOPING_SOUND_14;
        case SANCTUARY:
        case TEMPLE:
            return LOOPING_SOUND_37;
        case SEPULCHER:
            return LOOPING_SOUND_8;
        case SHIPYARD:
            return LOOPING_SOUND_24;
        case STABLES:
            return LOOPING_SOUND_21;
        case TRAINING_GROUNDS:
            return LOOPING_SOUND_23;
        case WAR_MACHINE_FACTORY:
            return LOOPING_SOUND_12;
        default:
            return LOOPING_SOUND_INVALID;
        }
    } else {
        switch (thisCell->m_type) {
        case NOTHING:
            switch (thisCell->getSpecialTerrain()) {
            case CURSED_GROUND:
                return LOOPING_SOUND_48;
            case MAGIC_PLAINS:
                return LOOPING_SOUND_25;
            }
            break;
        case TERRAIN_VOLCANO:
            return LOOPING_SOUND_45;
        }
    }
    return LOOPING_SOUND_INVALID;
}

// The looping-sound resource names, one per e_looping_sound_id row.
// Consumed by InsertSound's lazy loader; owner TU unlocated, so the
// nearest consumer declares (name provisional, role byte-proven).
DATA(0x0065f794) extern const char* const g_loopingSoundNames[LOOPING_SOUND_COUNT];

VA(0x00418c10, 0x1B1)  // dc 0x1be10
void AdvManager::insertSound(int x, int y, int z, int soundPriority,
                             int soundsType)
{
    if (x < 0 || y < 0 || z < 0 || x >= g_mapWidth || y >= g_mapHeight)
        return;

    ELoopingSoundId idNum = getSoundId(x, y, z);
    if (idNum == LOOPING_SOUND_INVALID)
        return;

    int i;
    for (i = 0; i < ADVENTURE_ACTIVE_SOUND_COUNT; i++) {
        if (m_soundArray[i].m_soundId == idNum) {
            if (m_soundArray[i].m_priority > soundPriority) {
                m_soundArray[i].m_priority = soundPriority;
                m_touchedSounds |= 1 << m_soundArray[i].m_soundId;
            }
            return;
        }
    }

    if (soundsType == 1)
        return;

    int best = -1;
    int bestPriority = soundPriority;
    for (i = 0; i < ADVENTURE_ACTIVE_SOUND_COUNT; i++) {
        if (m_soundArray[i].m_priority > bestPriority) {
            bestPriority = m_soundArray[i].m_priority;
            best = i;
        }
    }
    if (best == -1)
        return;

    if (m_soundArray[best].m_soundId != LOOPING_SOUND_INVALID)
        g_soundManager->stopSample(
            m_loopedSample[m_soundArray[best].m_soundId]->m_memSample.m_memSampleHandle);

    m_soundArray[best].m_soundId = idNum;
    m_soundArray[best].m_priority = soundPriority;

    if (idNum > LOOPING_SOUND_INVALID && idNum < LOOPING_SOUND_COUNT
        && !m_loopedSample[idNum]) {
        trimLoopingSounds(4);
        m_loopedSample[idNum] = ResourceManager::getSample(
            g_loopingSoundNames[idNum]);
    }

    m_loopedSample[idNum]->m_memSample.m_memVolume = g_soundVolumes[soundPriority];
    m_loopedSample[idNum]->m_memSample.m_memLooping = 0;
    m_loopedSample[idNum]->m_memSample.m_memCindex = 3;
    g_soundManager->memorySample(m_loopedSample[idNum]);
    m_touchedSounds ^= 1 << m_soundArray[best].m_soundId;
}

VA(0x00418dd0, 0x4DF)  // dc 0x1c05c
void AdvManager::showRoute(int updateScreen, int reseed, int changeButton)
{
    int steps;
    Hero* curr;
    int moveAvail = 0;
    int testMobility;
    int i;
    TerrainType nativeTerrain;
    type_point point;
    unsigned short* routeArrayPtr;
    int dir;
    int nextDir;
    int widgetStatus;

    if (!g_currentPlayer->isLocalHuman())
        return;
    if (g_currentPlayer->m_currHeroId == -1) {
        hideRoute(updateScreen, 0, 1);
        return;
    }
    curr = g_game->getCurrHero();
    if (curr->m_pathTargetX == -1) {
        hideRoute(updateScreen, 1, 1);
        return;
    }

    seedTo(curr->getTarget());
    steps = g_searchArray->buildPath(curr, 0xea5f);
    if (g_searchArray->getPathSteps() > 0 && steps > 0) {
        memset(m_routeArray, 0,
               g_game->getNumMapLevels() * g_mapHeight * g_mapWidth
                   * sizeof(unsigned short));
        m_showRoute = 1;
        testMobility = curr->m_movePoints;
        point = curr->getLocation();
        nativeTerrain = curr->m_army.getNativeTerrain();

        for (i = g_searchArray->getPathSteps() - 1; i >= 0; i--) {
            dir = g_searchArray->getStep(i);
            testMobility -= getTerrainCost(curr, point, dir, testMobility);
            point.m_x += g_normalDirTable[dir].m_x;
            point.m_y += g_normalDirTable[dir].m_y;
            if (i == 0) {
                routeArrayPtr = getRouteArrayPtr(point.m_x, point.m_y, point.m_z);
                *routeArrayPtr = 1;
            } else {
                nextDir = g_searchArray->getStep(i - 1);
                routeArrayPtr = getRouteArrayPtr(point.m_x, point.m_y, point.m_z);
                *routeArrayPtr = g_routeArrowFrames[nextDir][dir] + 2;
            }
            if (testMobility < 0) {
                routeArrayPtr = getRouteArrayPtr(point.m_x, point.m_y, point.m_z);
                *routeArrayPtr += 0x19;
            } else {
                moveAvail = 1;
            }
        }
        if (changeButton) {
            widgetStatus = moveAvail ? Widget::WIDGET_CLEAR_STATUS
                                     : Widget::WIDGET_SET_STATUS;
            g_windowManager->broadcastMessage(
                MESSAGE_WIDGET, widgetStatus, AdventureMapWindow::MOVE_ID,
                Widget::WIDGET_UPDATE | Widget::WIDGET_DIMMED);
        }
    } else {
        hideRoute(updateScreen, 1, 1);
    }
    if (updateScreen) {
        completeDraw(0);
        this->updateScreen(0, 0);
    }
}

VA(0x00419300, 0x14C)  // dc 0x1c484
void AdvManager::hideRoute(int updateScreen, int removeTarget,
                           int changeButton)
{
    if (!g_currentPlayer->isLocalHuman()
        && (!g_unnamed6989c8 || !g_unnamed69ccd4))
        return;

    if (changeButton) {
        g_windowManager->broadcastMessage(
            MESSAGE_WIDGET, Widget::WIDGET_SET_STATUS,
            AdventureMapWindow::MOVE_ID,
            Widget::WIDGET_UPDATE | Widget::WIDGET_DIMMED);
    }

    if (removeTarget) {
        int heroId = g_currentPlayer->m_currHeroId;
        if (heroId != -1) {
            Hero* currentHero = g_game->getCurrHero();
            currentHero->m_pathTargetX = -1;
            currentHero->m_pathTargetY = -1;
        }
    }

    if (!m_showRoute)
        return;
    m_showRoute = 0;
    if (!updateScreen)
        return;

    completeDraw(0);
    this->updateScreen(0, 0);
}

#if 0  // @carcass
// E:\gamedcs\advmgr.cpp:10558
DC_ONLY(0x1c580, 0x16)
void AdvManager::CheckDimHero()
{
    // @stub
}
#endif  // @carcass

VA(0x00419450, 0x43)  // dc 0x1c5ec
#pragma auto_inline(off)
void AdvManager::checkDimNextHeroBut()
{
    if (g_currentPlayer->isLocalHuman() && g_currentPlayer->hasMobileHero())
        m_advWindow->widgetClearStatus(11, Widget::WIDGET_UPDATE | Widget::WIDGET_DIMMED);
    else
        m_advWindow->widgetSetStatus(11, Widget::WIDGET_UPDATE | Widget::WIDGET_DIMMED);
}
#pragma auto_inline(on)

VA(0x004194a0, 0xC7)  // dc 0x1c64c
void AdvManager::seedTo(type_point target)
{
    if (!g_currentPlayer->isLocalHuman())
        return;

    int heroId = g_currentPlayer->m_currHeroId;
    if (heroId == -1)
        return;

    Hero* currentHero = &g_game->m_heroes[heroId];
    type_point start(currentHero->m_x, currentHero->m_y, currentHero->m_z);

    if (!m_seedingValid) {
        g_searchArray->seedPosition(currentHero, start, target, 59999,
                                    m_cursorType == CURSOR_TYPE_8,
                                    const_normal_search,
                                    currentHero->m_movePoints, 0);
    } else {
        if (m_fullySeeded)
            return;
        g_searchArray->seedPosition(currentHero, start, target, 59999,
                                    m_cursorType == CURSOR_TYPE_8,
                                    const_normal_search,
                                    currentHero->m_movePoints, 1);
    }
}

VA(0x00419570, 0x49)  // dc 0x1c750
void AdvManager::forceNewHover()
{
    if (g_currentPlayer->isLocalHuman()) {
        int x;
        int y;
        g_mouseManager->mouseCoords(x, y);
        m_lastHoverX = -1;
        processHover(x, y);
    }
}

// The per-speed scroll step. Dreamcast advmgr.obj publishes the static
// (S_LDATA32 akScrollSpeedInc); retail's ScreenScroll indexes the same
// three-int row.
DATA(0x0063a66c) static const int g_scrollSpeedInc[3] = { 1, 2, 3 };

// Written on every scroll tick with the fresh GameTime::Get stamp; no
// located reader yet. Nearest consumer holds the declaration (the
// town.cpp gUnnamed69778c precedent); ownership stays open for the data
// phase.
DATA(0x00691674) extern unsigned long g_unnamed691674;

// E:\gamedcs\advmgr.cpp:10624
VA(0x004195c0, 0x258)  // anchor-callee, dc 0x1c7e4
void AdvManager::screenScroll(int dir, int changeMouse)
{
    g_unnamed698758.m_windowScrollSpeed =
        limit(0, g_unnamed698758.m_windowScrollSpeed, 2);

    int x = m_radarOrigin.m_x;
    int y = m_radarOrigin.m_y;
    int inc = g_scrollSpeedInc[g_unnamed698758.m_windowScrollSpeed];
    g_unnamed691674 = GameTime::get();

    switch (dir) {
    case ADV_SCROLL_NORTH - ADV_SCROLL_POINTER:
        y -= inc;
        break;
    case ADV_SCROLL_NORTHEAST - ADV_SCROLL_POINTER:
        x += inc;
        y -= inc;
        break;
    case ADV_SCROLL_EAST - ADV_SCROLL_POINTER:
        x += inc;
        break;
    case ADV_SCROLL_SOUTHEAST - ADV_SCROLL_POINTER:
        x += inc;
        y += inc;
        break;
    case ADV_SCROLL_SOUTH - ADV_SCROLL_POINTER:
        y += inc;
        break;
    case ADV_SCROLL_SOUTHWEST - ADV_SCROLL_POINTER:
        x -= inc;
        y += inc;
        break;
    case ADV_SCROLL_WEST - ADV_SCROLL_POINTER:
        x -= inc;
        break;
    case ADV_SCROLL_NORTHWEST - ADV_SCROLL_POINTER:
        x -= inc;
        y -= inc;
        break;
    }

    if (changeMouse)
        g_mouseManager->setPointer(dir + ADV_SCROLL_POINTER,
                                   MouseManager::ADVENTURE_SET);

    if (x < -9)
        x = -9;
    if (x > g_mapWidth - 10)
        x = g_mapWidth - 10;
    if (y < -8)
        y = -8;
    if (y > g_mapHeight - 9)
        y = g_mapHeight - 9;

    if (x != m_radarOrigin.m_x || y != m_radarOrigin.m_y) {
        demobilizeCurrHero(0, 0);
        m_radarOrigin.m_x = x;
        m_radarOrigin.m_y = y;
        updateRadar(m_radarOrigin, 1, 1, 0, 0, 0);
        completeDraw(m_radarOrigin.m_x, m_radarOrigin.m_y, m_radarOrigin.m_z, 0, 1);
        g_windowManager->updateScreen(0, 8, 608, 544);

        unsigned long now = GameTime::get();
        if (static_cast<long>(
                now - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT]) >= 0
            && !m_animCtrPaused) {
            m_animCtr++;
            g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT] += cppMax(
                static_cast<long>(
                    now - g_timers[GLOBAL_ADVENTURE_ANIMATION_TIMER_SLOT]),
                180L);
        }
        process1WindowsMessage();
    }
}

// The DC also emits MouseInScrollZone (dc 0x1ccf8); its canonical member
// definition follows this routine. Retail expands the hover callers' tests.
VA(0x00419820, 0x169)  // dc 0x1cb08
void AdvManager::checkScreenScroll()
{
    int x;
    int y;
    g_mouseManager->mouseCoords(x, y);

    int dir;
    if (x < 0 || x >= WINDOW_SCREEN_WIDTH || y < 0
        || y >= WINDOW_SCREEN_HEIGHT) {
        g_unnamed691674 = GameTime::get();
        return;
    }
    if (x < 16) {
        if (y < 16)
            dir = ADV_SCROLL_NORTHWEST - ADV_SCROLL_POINTER;
        else
            dir = y <= WINDOW_SCREEN_HEIGHT - 16
                       ? ADV_SCROLL_WEST - ADV_SCROLL_POINTER
                       : ADV_SCROLL_SOUTHWEST - ADV_SCROLL_POINTER;
    } else if (x > WINDOW_SCREEN_WIDTH - 16) {
        if (y < 16)
            dir = ADV_SCROLL_NORTHEAST - ADV_SCROLL_POINTER;
        else
            dir = y > WINDOW_SCREEN_HEIGHT - 16
                       ? ADV_SCROLL_SOUTHEAST - ADV_SCROLL_POINTER
                       : ADV_SCROLL_EAST - ADV_SCROLL_POINTER;
    } else if (y < 16) {
        dir = ADV_SCROLL_NORTH - ADV_SCROLL_POINTER;
    } else if (y > WINDOW_SCREEN_HEIGHT - 16) {
        dir = ADV_SCROLL_SOUTH - ADV_SCROLL_POINTER;
    } else {
        g_unnamed691674 = GameTime::get();
        return;
    }

    unsigned long now = GameTime::get();
    if (now - g_unnamed691674 < 70)
        return;
    g_unnamed691674 += 70;
    if (now - g_unnamed691674 >= 70)
        g_unnamed691674 = now - 70;

    int origX = m_radarOrigin.m_x;
    int origY = m_radarOrigin.m_y;
    screenScroll(dir, 1);
    if (g_mouseManager->m_frame >= ADV_SCROLL_POINTER
        && g_mouseManager->m_frame <= ADV_SCROLL_NORTHWEST
        && origX == m_radarOrigin.m_x && origY == m_radarOrigin.m_y)
        g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);
}

// DC advmgr.cpp:10756 records MouseInScrollZone as an ordinary public member.
// Complete expands this body in ProcessHover; lack of a retained body does
// not change its source ownership.
int AdvManager::mouseInScrollZone()
{
    int rx;
    int ry;
    g_mouseManager->mouseCoords(rx, ry);
    if (rx < 0 || rx >= AdvManager::HOVER_SCREEN_WIDTH || ry < 0
        || ry >= AdvManager::HOVER_SCREEN_HEIGHT)
        return 0;
    if (rx >= AdvManager::HOVER_SCROLL_MARGIN && rx <= AdvManager::HOVER_SCROLL_RIGHT
        && ry >= AdvManager::HOVER_SCROLL_MARGIN && ry <= AdvManager::HOVER_SCROLL_BOTTOM)
        return 0;
    return 1;
}

VA(0x00419990, 0x2E3)  // dc 0x1cd68
void AdvManager::setInitialMapOrigin()
{
    m_lastHoverX = m_lastHoverY = 0;

    if (g_currentPlayer->isLocalHuman() && g_currentPlayer->m_currTownId != -1) {
        Town* startTown = &g_game->m_towns[g_currentPlayer->m_currTownId];
        m_radarOrigin.m_x = startTown->m_mapX - 9;
        m_radarOrigin.m_y = startTown->m_mapY - 8;
        m_radarOrigin.m_z = startTown->m_mapZ;
    } else if (g_currentPlayer->isLocalHuman()) {
        mobilizeCurrHero(0, 0, 0);
    } else {
        PlayerData* player = g_currentPlayer->isLocalHuman()
                                 ? g_currentPlayer
                                 : g_game->getLocalPlayer();
        if (player->m_numHeroes > 0) {
            Hero* startHero = &g_game->m_heroes[player->m_heroes[0]];
            m_radarOrigin.m_x = startHero->m_x - 9;
            m_radarOrigin.m_y = startHero->m_y - 8;
            m_radarOrigin.m_z = startHero->m_z;
        } else if (player->m_numTowns > 0) {
            Town* startTown = &g_game->m_towns[player->m_townIds[0]];
            m_radarOrigin.m_x = startTown->m_mapX - 9;
            m_radarOrigin.m_y = startTown->m_mapY - 8;
            m_radarOrigin.m_z = startTown->m_mapZ;
        } else {
            m_radarOrigin.m_x = 0;
            m_radarOrigin.m_y = 0;
            m_radarOrigin.m_z = 0;
        }
    }

    m_advWindow->setElevationToggleImage(m_radarOrigin.m_z);

    type_point center(m_radarOrigin.m_x + 9, m_radarOrigin.m_y + 8, m_radarOrigin.m_z);

    m_lastTerrain = getCell(center)->m_groundSet;
    g_soundManager->switchAmbientMusic(g_terrainMusicIds[m_lastTerrain]);
    setEnvironmentOrigin(center, 1);
    m_seedingValid = 0;

    if (g_currentPlayer->isLocalHuman() && g_currentPlayer->hasMobileHero())
        m_advWindow->widgetClearStatus(11, 0x4008);
    else
        m_advWindow->widgetSetStatus(11, 0x4008);
}

VA(0x00419c80, 0x171)  // dc 0x1d0bc
void popupPlayerTurnInfo()
{
    if (IsIconic(g_hwndApp))
        ShowWindow(g_hwndApp, SW_RESTORE);

    g_soundManager->stopMP3();
    SetForegroundWindow(g_hwndApp);
    g_mouseManager->setPointer(0, MouseManager::DEFAULT_SET);

    if (!g_currentPlayer->isLocalHuman())
        g_advManager->overrideBottomView(AdvManager::BOTTOM_VIEW_DEFAULT, -1);

    g_soundManager->m_playSounds = 1;
    SAMPLE2 sample2 = loadPlaySample("SysMsg.wav");

    if (!g_unk691209) {
        char text[256];
        const char* format =
            g_generalText->getText(GENERAL_TEXT_PLAYER_TURN_FORMAT);
        sprintf(text, format, g_game->getLocalPlayer()->getName());

        int dialogReturn;
        do {
            normalDialogTimeOut(text, 1, 15000, -1, -1, 10,
                                g_game->getLocalPlayerGamePos(), -1, 0,
                                -1, -1, 0);
            dialogReturn = g_windowManager->m_dialogReturn;
            if (dialogReturn == DIALOG_RETURN_TIMEOUT) {
                if (IsIconic(g_hwndApp))
                    ShowWindow(g_hwndApp, SW_RESTORE);
                SetForegroundWindow(g_hwndApp);
                Sleep(500);
                sample2 = loadPlaySample("SysMsg.wav");
            }
        } while (dialogReturn == DIALOG_RETURN_TIMEOUT
                 && !g_turnDuration69d630.isExpired());

        clearMemSample(sample2);
    }
}

VA(0x00419e00, 0x300)  // dc 0x1d30c
void AdvManager::startLocalPlayerTurn()
{
    if (g_game->m_playerDisabled[g_netLocalGamePos])
        computeAdvNetControl();

    if (g_currentPlayer->isLocalHuman()) {
        if (g_unnamed691209) {
            g_unnamed691209 = 0;
            normalDialogTimeOut(g_generalText->getText(663), 2, 2000, -1, -1,
                                -1, 0, -1, 0, -1, -1, 0);
            if (g_windowManager->m_dialogReturn == DIALOG_RETURN_DECLINE) {
                g_game->m_players[g_unnamed69120c].m_isHuman = 1;
                g_game->m_players[g_unnamed69120c].m_isLocal = 1;
                g_unnamed691209 = 0;
                g_mapVisibilityBit = 1 << g_unnamed69120c;
            } else {
                g_unnamed691209 = 1;
            }
        }

        g_game->cancelComputerScreen();
        g_thisNetGotAdventureControl = 1;
        g_soundManager->m_playSounds = 0;

        CTurnUpdateMsg msg(g_game->getLocalPlayerGamePos());
        transmitRemoteData(&msg, 0x7f, 0, 1);
        g_turnDuration69d630.start();
        popupPlayerTurnInfo();
        g_unnamed69d810 = g_game->getLocalPlayerGamePos();
        g_weMoved = 1;
    }

    if ((g_game->m_day != 1
         || (g_game->m_week == 1 && g_game->m_month == 1))
        && g_videoPaused && g_currentPlayer->isLocalHuman()) {
        g_soundManager->m_playSounds = 1;
        startAITheme();
        g_soundManager->m_playSounds = 0;
        g_unnamed699544 = GameTime::get();
    }
    g_game->doNewTurn();

    m_advWindow->updateHeroLocators(-1, 1, 1);
    m_advWindow->updateTownLocators(-1, 1, 1);
    m_advWindow->updateResourceDisplay(1, 1);

    AdvManager* adv = g_advManager;
    if (g_currentPlayer->isLocalHuman()) {
        int mouseX;
        int mouseY;
        g_mouseManager->mouseCoords(mouseX, mouseY);
        adv->m_lastHoverX = -1;
        adv->processHover(mouseX, mouseY);
    }

    g_soundManager->m_playSounds = 1;
    if (g_game->m_isCheater && !g_unnamed691678) {
        g_unnamed691678 = 1;
        sprintf(g_text, DATA_COMPGEN(0x0066040c, turnPopupLineFormat, "%s\n"),
                g_generalText->getText(332));
        normalDialog(g_text, 1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    }
    if (g_unnamed6989c8 > 0 && !g_unnamed69167c) {
        g_unnamed69167c = 1;
        sprintf(g_text, "%s\n", g_generalText->getText(333));
        normalDialog(g_text, 1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    }
}

VA(0x0041a100, 0xD9)  // dc 0x1d6ec
void AdvManager::loadRemote(unsigned char makeOrig)
{
    g_turnDuration69d630.clear();
    CHourGlass hourGlass(1);

    int weekTypeExtra = g_weekTypeExtra;
    int weekType = g_weekType;
    int monthType = g_monthType;
    int monthTypeExtra = g_monthTypeExtra;

    g_game->loadGame(g_unnamed698758.m_rcFile, 0, 1);
    if (makeOrig)
        g_game->saveGame("orig.dat", 0, 0, 0, 1);

    g_weekTypeExtra = weekTypeExtra;
    g_weekType = weekType;
    g_monthType = monthType;
    g_monthTypeExtra = monthTypeExtra;

    hourGlass.stop();
    startLocalPlayerTurn();
}

VA(0x0041a1e0, 0xF1)  // dc 0x1d804
void AdvManager::trimLoopingSounds(int maxSoundsAllowed)
{
    if (g_highMemBuffer > 0)
        maxSoundsAllowed += g_highMemBuffer / 100;

    if (g_mapWidth != ADVENTURE_XLARGE_MAP_WIDTH)
        ++maxSoundsAllowed;

    if (maxSoundsAllowed >= LOOPING_SOUND_COUNT)
        return;

    int soundsFound = 0;
    char saveSounds[LOOPING_SOUND_COUNT];
    memset(saveSounds, 0, sizeof(saveSounds));

    int i;
    for (i = 0; i < ADVENTURE_ACTIVE_SOUND_COUNT; ++i) {
        int soundId = m_soundArray[i].m_soundId;
        if (soundId >= LOOPING_SOUND_0 && soundId < LOOPING_SOUND_COUNT)
            ++saveSounds[soundId];
    }

    for (i = 0; i < LOOPING_SOUND_COUNT; ++i) {
        if (saveSounds[i])
            ++soundsFound;
    }

    if (soundsFound < maxSoundsAllowed) {
        for (i = 0; i < LOOPING_SOUND_COUNT; ++i) {
            if (!saveSounds[i] && m_loopedSample[i]) {
                ++soundsFound;
                ++saveSounds[i];
                if (soundsFound >= maxSoundsAllowed)
                    break;
            }
        }
    }

    for (i = 0; i < LOOPING_SOUND_COUNT; ++i) {
        if (m_loopedSample[i] && !saveSounds[i]) {
            m_loopedSample[i]->dispose();
            m_loopedSample[i] = 0;
        }
    }
}

VA(0x0041a2e0, 0xBC)  // dc 0x1d9e0
void AdvManager::disableButtons()
{
    if (g_advManager->m_status != BaseManager::STATUS_ACTIVE)
        return;
    m_advWindow->widgetClearStatus(AdventureMapWindow::KINGDOM_OVERVIEW_ID,
                                 Widget::WIDGET_ACTIVE);
    m_advWindow->widgetClearStatus(AdventureMapWindow::ELEVATION_TOGGLE_ID,
                                 Widget::WIDGET_ACTIVE);
    m_advWindow->widgetClearStatus(AdventureMapWindow::QUEST_LOG_ID,
                                 Widget::WIDGET_ACTIVE);
    m_advWindow->widgetClearStatus(AdventureMapWindow::SLEEP_ID,
                                 Widget::WIDGET_ACTIVE);
    m_advWindow->widgetClearStatus(AdventureMapWindow::MOVE_ID,
                                 Widget::WIDGET_ACTIVE);
    m_advWindow->widgetClearStatus(AdventureMapWindow::CAST_SPELL_ID,
                                 Widget::WIDGET_ACTIVE);
    m_advWindow->widgetClearStatus(AdventureMapWindow::ADVENTURE_OPTIONS_ID,
                                 Widget::WIDGET_ACTIVE);
    m_advWindow->widgetClearStatus(AdventureMapWindow::SYSTEM_OPTIONS_ID,
                                 Widget::WIDGET_ACTIVE);
    m_advWindow->widgetClearStatus(AdventureMapWindow::NEXT_HERO_ID,
                                 Widget::WIDGET_ACTIVE);
    m_advWindow->widgetClearStatus(AdventureMapWindow::END_TURN_ID,
                                 Widget::WIDGET_ACTIVE);
    m_advWindow->widgetClearStatus(AdventureMapWindow::HERO_UP_ID,
                                 Widget::WIDGET_ACTIVE);
    m_advWindow->widgetClearStatus(AdventureMapWindow::HERO_DOWN_ID,
                                 Widget::WIDGET_ACTIVE);
    m_advWindow->widgetClearStatus(AdventureMapWindow::TOWN_UP_ID,
                                 Widget::WIDGET_ACTIVE);
    m_advWindow->widgetClearStatus(AdventureMapWindow::TOWN_DOWN_ID,
                                 Widget::WIDGET_ACTIVE);
}

VA(0x0041a3a0, 0xBC)  // dc 0x1dafc
void AdvManager::enableButtons()
{
    if (g_advManager->m_status != BaseManager::STATUS_ACTIVE)
        return;
    m_advWindow->widgetSetStatus(AdventureMapWindow::KINGDOM_OVERVIEW_ID,
                               Widget::WIDGET_ACTIVE);
    m_advWindow->widgetSetStatus(AdventureMapWindow::ELEVATION_TOGGLE_ID,
                               Widget::WIDGET_ACTIVE);
    m_advWindow->widgetSetStatus(AdventureMapWindow::QUEST_LOG_ID,
                               Widget::WIDGET_ACTIVE);
    m_advWindow->widgetSetStatus(AdventureMapWindow::SLEEP_ID,
                               Widget::WIDGET_ACTIVE);
    m_advWindow->widgetSetStatus(AdventureMapWindow::MOVE_ID,
                               Widget::WIDGET_ACTIVE);
    m_advWindow->widgetSetStatus(AdventureMapWindow::CAST_SPELL_ID,
                               Widget::WIDGET_ACTIVE);
    m_advWindow->widgetSetStatus(AdventureMapWindow::ADVENTURE_OPTIONS_ID,
                               Widget::WIDGET_ACTIVE);
    m_advWindow->widgetSetStatus(AdventureMapWindow::SYSTEM_OPTIONS_ID,
                               Widget::WIDGET_ACTIVE);
    m_advWindow->widgetSetStatus(AdventureMapWindow::NEXT_HERO_ID,
                               Widget::WIDGET_ACTIVE);
    m_advWindow->widgetSetStatus(AdventureMapWindow::END_TURN_ID,
                               Widget::WIDGET_ACTIVE);
    m_advWindow->widgetSetStatus(AdventureMapWindow::HERO_UP_ID,
                               Widget::WIDGET_ACTIVE);
    m_advWindow->widgetSetStatus(AdventureMapWindow::HERO_DOWN_ID,
                               Widget::WIDGET_ACTIVE);
    m_advWindow->widgetSetStatus(AdventureMapWindow::TOWN_UP_ID,
                               Widget::WIDGET_ACTIVE);
    m_advWindow->widgetSetStatus(AdventureMapWindow::TOWN_DOWN_ID,
                               Widget::WIDGET_ACTIVE);
}

VA(0x0041a460, 0x1FB)  // dc 0x1dc24
unsigned char AdvManager::findAdjacentMonster(type_point point, type_point* result, type_point excluded)
{
    RECT rect;
    int x;
    int y;
    NewmapCell* mapCell;

    rect.left = cppMax<int>(point.m_x - 1, 0);
    rect.top = cppMax<int>(point.m_y - 1, 0);
    rect.right = cppMin<int>(g_mapWidth, point.m_x + 2);
    rect.bottom = cppMin<int>(g_mapHeight, point.m_y + 2);

    mapCell = m_fullMap->cell(point.m_x, point.m_y, point.m_z);
    unsigned char centerIsWater = mapCell->m_groundSet == eTerrainWater;
    if (mapCell->cellIsTrigger()
        && !g_adventureObjectTraits[mapCell->getMapObject()][1])
        rect.top = point.m_y;

    for (x = rect.left; x < rect.right; ++x) {
        for (y = rect.top; y < rect.bottom; ++y) {
            mapCell = m_fullMap->cell(x, y, point.m_z);
            if (mapCell->m_type == MONSTER && mapCell->m_isTrigger
                && (mapCell->m_groundSet == eTerrainWater) == centerIsWater
                && (x != excluded.m_x || y != excluded.m_y
                    || point.m_z != excluded.m_z)) {
                result->m_x = x;
                result->m_y = y;
                result->m_z = point.m_z;
                return 1;
            }
        }
    }
    return 0;
}

VA(0x0041a660, 0xEB)  // dc 0x1dde4
void computeAdvNetControl()
{
    if (!g_networkActive69954c) {
        g_thisNetGotAdventureControl = 1;
        return;
    }

    int lastHuman = -1;
    int player;
    if (g_game->m_playerDisabled[g_netLocalGamePos]) {
        player = (g_netLocalGamePos + 1) % 8;
        while (player != g_netLocalGamePos) {
            if (!g_game->m_playerDisabled[player] && g_game->isHuman(player)) {
                g_thisNetGotAdventureControl = g_game->isLocalHuman(player);
                return;
            }
            player = (player + 1) % 8;
        }
    }

    player = (g_netLocalGamePos + 1) % 8;
    while (player != g_netLocalGamePos) {
        player = (player + 1) % 8;
        if (!g_game->m_playerDisabled[player] && g_game->isHuman(player))
            lastHuman = player;
    }
    g_thisNetGotAdventureControl = g_game->isLocalHuman(lastHuman);
}

VA(0x0041a750, 0x97)  // dc 0x1df7c
int mapExtraPosAndAdjacentsSet(int x, int y, int z, unsigned char bit)
{
    if (getMapExtra(x, y, z) & bit)
        return 1;

    for (int testX = x - 1; testX <= x + 1; ++testX) {
        if (testX >= 0 && testX < g_mapWidth) {
            for (int testY = y - 1; testY <= y + 1; ++testY) {
                if (testY >= 0 && testY < g_mapHeight
                    && (getMapExtra(testX, testY, z) & bit))
                    return 1;
            }
        }
    }
    return 0;
}

// E:\gamedcs\advmgr.cpp:11220
// The obelisk puzzle viewer. The puzzle art is picked by the local
// player's ALIGNMENT (setup.alignment[pos], the faction), the grail
// X-mark is the arrow tileset's frame 0 centred on the ultimate-
// artifact cell over a CompleteDraw of the puzzle origin, colorized
// to 0.625 hue and fizzled forward over 220ms unless every piece is
// already down (0x30 = all 48 revealed), and one type_point local is
// reused for the puzzle origin and the closing view re-centre (the
// tail's x/y/z writes are RMW bitfield inserts into the same slot).

// Residual (83.31%): pure schedule/register-homing inside the grail
// draw block - branch shape agrees 4/4+1ret, call multiset agrees, and
// the raw 13-arg DrawAdvObjWithFlag overload is byte-proven the right
// spelling (the Bitmap16Bit* wrapper evaluates dx/dy before the bitmap
// field loads and measures 75.35; the raw call measures 83.31; the
// SetHeroContext merged y|z idiom in the tail was worth +5.1 before
// that). What remains: retail re-loads arrowTileset per use where our
// CL folds one load (sub eax,[edx+0x34] vs mov/sub), and the dy/dx
// arithmetic interleaves with the bitmap pushes differently. Tried and
// rejected: grailY-before-grailX declaration order (+0.01, copy-prop
// eats it). why-reg finds first defs aligned - past-first-defs
// schedule, the bounded class.
VA(0x0041a7f0, 0x307)  // anchor-callee, dc 0x1e068
void AdvManager::viewPuzzle()
{
    g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);
    demobilizeCurrHero(0, 1);
    int pos = g_game->getLocalPlayerGamePos();
    g_game->setupPuzzlePieces(pos, 0);
    PuzzleWindow puzzle(pos >= 0 ? g_game->m_setup.m_alignment[pos] : -1);
    SAMPLE2 sample2 = loadPlaySample("Obelisk.wav");
    puzzle.updatePuzzle(1);
    drawAdventureMapGems();
    g_windowManager->updateScreen(0, 0, 800, 600);

    if (g_game->m_numObelisks > 0) {
        g_windowManager->saveFizzleSourceX(8, 8, 592, 544);
        type_point centre = g_game->getPuzzleOrigin();
        int grailY = g_game->m_ultimateArtifactY;
        int grailX = g_game->m_ultimateArtifactX;
        g_unnamed6989f4 = 1;
        completeDraw(centre.m_x, centre.m_y, centre.m_z, 0, 0);
        g_unnamed6989f4 = 0;
        // DrawTile (0x47bf50), NOT DrawAdvObjWithFlag (0x47be10): the
        // delinked target resolves this relocation to the tile entry, and
        // the two are separate claimed bodies, so it is not an ICF fold.
        // Byte-flat (a call relocation's name is not scored) but it stops
        // the call census reporting a phantom divergence here.
        m_arrowTileset->drawTile(
            0, 0, 0, 32, 32, g_windowManager->m_screenBitmap->getMap(0, 0),
            (grailX - centre.m_x) * 32 + (32 - m_arrowTileset->getWidth()) / 2,
            (grailY - centre.m_y) * 32 + (32 - m_arrowTileset->getHeight()) / 2,
            g_windowManager->m_screenBitmap->getWidth(),
            g_windowManager->m_screenBitmap->getHeight(),
            g_windowManager->m_screenBitmap->getPitch(), 0, 0);
        g_windowManager->m_screenBitmap->colorize(8, 8, 592, 544, 0.625f,
                                                0.0f);
        int revealed = puzzle.updatePuzzle(0);
        drawAdventureMapGems();
        if (revealed != PuzzleWindow::PUZZLE_PIECE_COUNT)
            g_windowManager->fizzleForwardX(8, 8, 592, 544, 220);
        else
            g_windowManager->releaseFizzleSource();
    }

    puzzle.doModal(0);
    clearMemSample(sample2);

    if (!g_unnamed6aac3c) {
        redrawAdvScreen(1, 0);
        g_soundManager->switchAmbientMusic(g_terrainMusicIds[m_lastTerrain]);
        type_point centre(m_radarOrigin.m_x + 9, m_radarOrigin.m_y + 8,
                          m_radarOrigin.m_z);
        setEnvironmentOrigin(centre, 1);
    }
}

#if 0  // @carcass

// E:\gamedcs\advmgr.cpp:11287
DC_ONLY(0x1e360, 0xE8)
void AdvManager::PuzzleDraw(int startX, int startY, int z, int ultX, int ultY)
{
    // @stub
}

// E:\gamedcs\advmgr.cpp:11310
#endif  // @carcass

VA(0x0041ab00, 0xF8)  // dc 0x1e448
void AdvManager::doAdventureOptions()
{
    trimLoopingSounds(4);
    g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);

    unsigned char saveMobile = m_curHeroMobile;
    demobilizeCurrHero(0, 1);

    {
        AdventureOptionsWindow adventureOptionsWindow;
        adventureOptionsWindow.doModal(0);
    }

    switch (g_windowManager->m_dialogReturn) {
    case AdventureOptionsWindow::VIEW_WORLD_ID:
        viewWorld(0, eMasteryNone);
        break;
    case AdventureOptionsWindow::VIEW_PUZZLE_ID:
        viewPuzzle();
        break;
    case AdventureOptionsWindow::REPLAY_ID:
        g_game->playRecordedEvents();
        break;
    case AdventureOptionsWindow::VIEW_SCENARIO_ID:
        g_game->showScenInfo();
        break;
    case AdventureOptionsWindow::DIG_ID:
        processSearch(-1, -1, -1);
        break;
    }

    if (saveMobile)
        mobilizeCurrHero(0, 0, 1);
}

#if 0  // @carcass

// E:\gamedcs\advmgr.cpp:11352
DC_ONLY(0x1e598, 0x4E)
void AdvManager::DoAdvMenu()
{
    // @stub
}

#endif  // @carcass

unsigned char saveGame(unsigned char campaignWinMode);

VA(0x0041ac00, 0x1AC)  // dc 0x1e5e8
unsigned char AdvManager::doSystemOptions()
{
    int result = -1;
    trimLoopingSounds(4);
    g_mouseManager->setPointer(0, MouseManager::ADVENTURE_SET);

    unsigned char saveMobile = m_curHeroMobile;
    int walkSpeed = g_unnamed698758.m_walkSpeed;
    demobilizeCurrHero(0, 1);

    {
        SystemOptionsWindow systemOptionsWindow;
        systemOptionsWindow.doModal();
    }

    switch (g_windowManager->m_dialogReturn) {
    case SYSOPT_QUIT:
        result = g_windowManager->m_dialogReturn;
        normalDialog(g_generalText->getText(68), 2, -1, -1, -1, 0, -1, 0,
                     -1, 0, -1, 0);
        if (g_windowManager->m_dialogReturn != DIALOG_RETURN_ACCEPT)
            result = -1;
        break;
    case SYSOPT_COMMAND_102:
    case SYSOPT_COMMAND_105:
    case SYSOPT_COMMAND_108:
        result = g_windowManager->m_dialogReturn;
        break;
    case SYSOPT_SAVE_GAME:
        saveGame(0);
        break;
    }

    if (saveMobile)
        mobilizeCurrHero(0, 0, 1);

    if (g_unnamed698758.m_walkSpeed != walkSpeed) {
        int i;
        for (i = 0; i < 10; i++)
            m_heroSamples[i]->dispose();
        for (i = 0; i <= 10; i++) {
            sprintf(g_text, "horse%02d.wav", i);
            m_heroSamples[i] = ResourceManager::getSample(g_text);
        }
    }

    if (saveMobile)
        mobilizeCurrHero(0, 0, 1);

    if (result != -1) {
        g_gameCommand = result;
        return 1;
    }
    return 0;
}

VA(0x0041adb0, 0x25F)  // dc 0x1e86c
int AdvManager::moreTreesNear(type_point point)
{
    RECT rect;
    const int radius = 3;
    int trees = 0;
    int mountains = 0;
    int dead = 0;
    type_point pt;

    rect.top = max(point.m_y - radius, 0);
    rect.bottom = cppMin(point.m_y + radius + 1, g_mapHeight);
    rect.left = max(point.m_x - radius, 0);
    rect.right = cppMin(point.m_x + radius + 1, g_mapWidth);

    pt.m_z = point.m_z;
    for (pt.m_y = rect.top; pt.m_y < rect.bottom; pt.m_y++) {
        for (pt.m_x = rect.left; pt.m_x < rect.right; pt.m_x++) {
            NewmapCell* cell = getCell(pt);
            switch (cell->m_type) {
            case TERRAIN_BRUSH:
            case TERRAIN_BUSH:
            case TERRAIN_OAK_TREE:
            case TERRAIN_PINE_TREE:
            case TERRAIN_TREE:
            case TERRAIN_WILLOW_TREE:
            case TERRAIN_YUCCA_TREE:
                trees++;
                break;
            case TERRAIN_HILL:
            case TERRAIN_MOUND:
            case TERRAIN_MOUNTAIN:
            case TERRAIN_OUTCROPPING:
            case TERRAIN_VOLCANIC_VENT:
            case TERRAIN_VOLCANO:
                mountains++;
                break;
            case TERRAIN_DEAD_VEGETATION:
                dead++;
                break;
            }
        }
    }

    if (dead > trees && dead > mountains)
        return 0;
    if (mountains > trees && mountains > dead)
        return 1;
    return 2;
}

#if 0  // @carcass

// E:\gamedcs\advmgr.cpp:11501
DC_ONLY(0x1eb78, 0x40)
unsigned short AdvManager::GetRouteArray(int x, int y, int z)
{
    // @stub
}

// E:\gamedcs\advmgr.cpp:11507
#endif  // @carcass

VA(0x0041b010, 0x27)  // dc 0x1ebb8
unsigned short* AdvManager::getRouteArrayPtr(int x, int y, int z)
{
    return &m_routeArray[(z * g_mapHeight + y) * g_mapWidth + x];
}

VA(0x0041b040, 0x9A)  // dc 0x1ebf4
CAdvPopup::CAdvPopup(int winX, int winY, int winWidth, int winHeight,
                     unsigned winType)
    : CHeroWindowEx(winX, winY, winWidth, winHeight, winType)
{
    m_exitId = 0x200;
    m_exitCodeX = 10;
    m_exitCommand = 0x7801;
    m_savedPlayerState = 0;

    if (g_videoPaused) {
        CNetMsgHandler* netMsgHandler = g_dPlay->getNetMsgHandler();
        if (netMsgHandler) {
            if (netMsgHandler->isInPopup())
                m_savedPlayerState = 1;
            netMsgHandler->setInPopup(1);
        }
    }
}

VA_COMPGEN(0x0041b0e0, 0x21, SCALAR_DELETING_DTOR, CAdvPopup)

VA_COMPGEN(0x0041b110, 0x5, IMPLICIT_DTOR, CHeroWindowEx)

// CodeView marks dc 0x34c8 compiler-generated (compgenx): only the
// CHeroWindowEx base teardown runs there. The exact Windows-only source
// exception is reviewed in config/win_only.tsv.
VA(0x0041b120, 0x67)  // dc 0x34c8
CAdvPopup::~CAdvPopup()
{
    if (g_videoPaused) {
        CNetMsgHandler* netMsgHandler = g_dPlay->getNetMsgHandler();
        if (netMsgHandler)
            netMsgHandler->setInPopup(m_savedPlayerState);
    }
}

// E:\gamedcs\advmgr.cpp:11528
// Retail keeps the same seven-statement, branchless shape: publish the saved
// result, rewrite the message from this object's three command fields, and
// forward it to the executive.
VA(0x0041b190, 0x2D)  // CAdvPopup vtable 0x63a6a8 slot 14, dc 0x1ec80
int CAdvPopup::exitDialog(Message& msg)
{
    g_windowManager->m_dialogReturn = m_exitCommand;
    msg.m_id = m_exitId;
    msg.m_codeX = m_exitCodeX;
    msg.m_codeY = 10;
    return MESSAGE_DISPATCH_FORWARD;
}

// E:\gamedcs\advmgr.cpp:11539
// Dreamcast proves the local inventory and statement order. Retail preserves
// that control flow but gates the network pump on Complete's bVideoPaused:
// base handler, expired-turn exit, CheckHandleNet, abort-message exit.
VA(0x0041b1c0, 0x87)  // CAdvPopup vtable 0x63a6a8 slot 9, dc 0x1ecb4
int CAdvPopup::windowHandler(Message& msg)
{
    int ret = CHeroWindowEx::windowHandler(msg);
    if (ret)
        return ret;

    if (g_turnDuration69d630.isExpired())
        return exitDialog(msg);

    if (g_videoPaused) {
        unsigned char msgReceived = 0;
        CNetMsgHandler* netMsgHandler = g_dPlay->getNetMsgHandler();
        if (netMsgHandler)
            netMsgHandler->checkHandleNet(1, &msgReceived);
        if (msgReceived && netMsgHandler->getAbortPopupMsg())
            return exitDialog(msg);
    }
    return 0;
}

#if 0  // @carcass

// E:\gamedcs\struct.h:100
DC_ONLY(0x1eda8, 0x8)
void type_point::type_point()
{
    // @stub
}

// E:\gamedcs\struct.h:110
DC_ONLY(0x1ee20, 0x7A)
unsigned char type_point::operator==(const type_point* arg)
{
    // @stub
}

// E:\gamedcs\struct.h:273
DC_ONLY(0x1ee9c, 0x26)
void SLimitData::SLimitData(int minx, int miny, int maxx, int maxy)
{
    // @stub
}

// E:\gamedcs\struct.h:403
DC_ONLY(0x1eec4, 0x10)
long GameTime::Elapsed(unsigned long stop, unsigned long start)
{
    // @stub
}

// E:\gamedcs\struct.h:411
DC_ONLY(0x1eed4, 0x30)
long GameTime::elapsedSince(unsigned long time)
{
    // @stub
}

// E:\gamedcs\struct.h:419
DC_ONLY(0x1ef04, 0x24)
bool GameTime::isPast(unsigned long time)
{
    // @stub
}

// E:\gamedcs\includes.h:97
DC_ONLY(0x1ef28, 0x34)
int max(int a, int b)
{
    // @stub
}

// E:\gamedcs\includes.h:134
DC_ONLY(0x1ef5c, 0x38)
int limit(int min, int value, int max)
{
    // @stub
}

// E:\gamedcs\CreatureType.h:296
DC_ONLY(0x1ef94, 0x5C)
const char* getArmyName(int type, int count)
{
    // @stub
}

// E:\gamedcs\AdvMgr.h:187
DC_ONLY(0x1eff0, 0xE)
void DrawParts::DrawParts()
{
    // @stub
}

// E:\gamedcs\AdvMgr.h:1254
DC_ONLY(0x1f084, 0x4C)
int getMapExtra(type_point point)
{
    // @stub
}

// E:\gamedcs\MouseMgr.h:189
DC_ONLY(0x1f0d0, 0xE)
int MouseManager::Enable()
{
    // @stub
}

// E:\gamedcs\MouseMgr.h:197
DC_ONLY(0x1f0e0, 0xE)
int MouseManager::Disable()
{
    // @stub
}

// E:\gamedcs\MouseMgr.h:210
DC_ONLY(0x1f0f0, 0xE)
int MouseManager::getFrame()
{
    // @stub
}

// E:\gamedcs\Bitmap16.h:111
DC_ONLY(0x1f100, 0xC)
int Bitmap16Bit::getWidth()
{
    // @stub
}

// E:\gamedcs\Bitmap16.h:112
DC_ONLY(0x1f10c, 0xC)
int Bitmap16Bit::getHeight()
{
    // @stub
}

// E:\gamedcs\Bitmap16.h:113
DC_ONLY(0x1f118, 0xC)
int Bitmap16Bit::getPitch()
{
    // @stub
}

// E:\gamedcs\Bitmap16.h:150
DC_ONLY(0x1f124, 0x22)
unsigned short* Bitmap16Bit::getMap(int x, int y)
{
    // @stub
}

// E:\gamedcs\CSprite.h:145
DC_ONLY(0x1f148, 0x2C)
int CSprite::getWidth()
{
    // @stub
}

// E:\gamedcs\CSprite.h:146
DC_ONLY(0x1f174, 0x2C)
int CSprite::getHeight()
{
    // @stub
}

// E:\gamedcs\CSprite.h:287
DC_ONLY(0x1f1a0, 0x3C)
unsigned short CSprite::GetPaletteColor(unsigned char index)
{
    // @stub
}

// E:\gamedcs\CSprite.h:293
DC_ONLY(0x1f1dc, 0x58)
int CSprite::getNumFrames(int seqnum)
{
    // @stub
}

// E:\gamedcs\CSprite.h:294
DC_ONLY(0x1f234, 0x32)
int CSprite::isValidSeq(int seqnum)
{
    // @stub
}

// E:\gamedcs\CSprite.h:324
DC_ONLY(0x1f268, 0xBC)
void CSprite::draw(int seqnum, int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx, int dy, unsigned char hflip, unsigned char tblit)
{
    // @stub
}

// E:\gamedcs\CSprite.h:354
DC_ONLY(0x1f324, 0xA4)
void CSprite::drawAdvObj(int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx, int dy, unsigned char hflip)
{
    // @stub
}

// E:\gamedcs\CSprite.h:360
DC_ONLY(0x1f3c8, 0xB8)
void CSprite::drawAdvObjWithFlag(int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx, int dy, unsigned short outcolor, unsigned char hflip)
{
    // @stub
}

// E:\gamedcs\CSprite.h:372
DC_ONLY(0x1f480, 0xA4)
void CSprite::drawAdvObjShadow(int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx, int dy, unsigned char hflip)
{
    // @stub
}

// E:\gamedcs\CSprite.h:385
DC_ONLY(0x1f524, 0xA4)
void CSprite::drawInterface(int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx, int dy, unsigned char hflip)
{
    // @stub
}

// E:\gamedcs\CSprite.h:392
DC_ONLY(0x1f5c8, 0xB8)
void CSprite::drawTile(int tilenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx, int dy, unsigned char hflip, unsigned char vflip)
{
    // @stub
}

// E:\gamedcs\CSprite.h:404
DC_ONLY(0x1f680, 0xB8)
void CSprite::drawTileShadow(int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx, int dy, unsigned char hFlipped, unsigned char vFlipped)
{
    // @stub
}

// E:\gamedcs\CSprite.h:421
DC_ONLY(0x1f738, 0xB8)
void CSprite::drawShroudTile(int tilenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx, int dy, unsigned char hflip, unsigned char vflip)
{
    // @stub
}

// E:\gamedcs\CSprite.h:426
DC_ONLY(0x1f7f0, 0xB4)
void CSprite::drawHero(int seqnum, int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx, int dy, unsigned char hflip)
{
    // @stub
}

// E:\gamedcs\CSprite.h:432
DC_ONLY(0x1f8a4, 0xB4)
void CSprite::drawHeroShadow(int seqnum, int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx, int dy, unsigned char hflip)
{
    // @stub
}

// E:\gamedcs\MapCell.h:565
DC_ONLY(0x1f958, 0x1A)
unsigned CObjectType::getBitPos(unsigned x, unsigned y)
{
    // @stub
}

// E:\gamedcs\MapCell.h:850
DC_ONLY(0x1f974, 0x52)
NewmapCell* NewfullMap::zCell(int x, int y, int z)
{
    // @stub
}

// E:\gamedcs\MapCell.h:906
DC_ONLY(0x1f9f4, 0x4C)
NewmapCell* NewfullMap::cell(type_point point)
{
    // @stub
}

// E:\gamedcs\MapCell.h:914
DC_ONLY(0x1fa40, 0x40)
unsigned char ExtraInfoUnion::playerKnowsCell(short player)
{
    // @stub
}

// E:\gamedcs\MapCell.h:979
DC_ONLY(0x1fa80, 0x12)
short ExtraInfoUnion::getItemId()
{
    // @stub
}

// E:\gamedcs\MapCell.h:1121
DC_ONLY(0x1fa94, 0x14)
SpellID ExtraInfoUnion::getShrineSpell()
{
    // @stub
}

// E:\gamedcs\MapCell.h:1141
DC_ONLY(0x1faa8, 0x12)
WiseTreePrices ExtraInfoUnion::getTreePrice()
{
    // @stub
}

// E:\gamedcs\MapCell.h:1220
DC_ONLY(0x1fabc, 0x1A)
short ExtraInfoUnion::getWheelGold()
{
    // @stub
}

// E:\gamedcs\MapCell.h:1230
DC_ONLY(0x1fad8, 0x16)
short ExtraInfoUnion::getWindmillAmount()
{
    // @stub
}

// E:\gamedcs\MapCell.h:1246
DC_ONLY(0x1faf0, 0x14)
SecondarySkill ExtraInfoUnion::getWitchSkill()
{
    // @stub
}

// E:\gamedcs\Hero.h:116
DC_ONLY(0x1fb04, 0xC)
// Before normalization (function): type_obscuring_object::get_obscured_type.
AdventureObjectType ObscuringObject::getObscuredType()
{
    // @stub
}

// E:\gamedcs\Hero.h:118
DC_ONLY(0x1fb10, 0xC)
unsigned long ObscuringObject::get_obscured_extra_info()
{
    // @stub
}

// E:\gamedcs\Hero.h:145
DC_ONLY(0x1fb1c, 0x10)
unsigned char ObscuringObject::isOnMap()
{
    // @stub
}

// E:\gamedcs\Hero.h:157
DC_ONLY(0x1fb2c, 0x60)
type_point ObscuringObject::getLocation() const
{
    // @stub
}

// E:\gamedcs\Hero.h:190
DC_ONLY(0x1fb8c, 0x12)
unsigned char Boat::getHflip()
{
    // @stub
}

// E:\gamedcs\Hero.h:196
DC_ONLY(0x1fba0, 0x28)
void Boat::obscureCell()
{
    // @stub
}

// E:\gamedcs\Hero.h:334
DC_ONLY(0x1fbc8, 0x14)
unsigned char Hero::getHflip()
{
    // @stub
}

// E:\gamedcs\Hero.h:641
DC_ONLY(0x1fbdc, 0x98)
unsigned char Hero::isFlying(unsigned char check_terrain)
{
    // @stub
}

// E:\gamedcs\Hero.h:650
DC_ONLY(0x1fc74, 0x98)
unsigned char Hero::canWalkOnWater(unsigned char check_terrain)
{
    // @stub
}

// E:\gamedcs\Hero.h:682
DC_ONLY(0x1fd0c, 0x24)
void Hero::obscureCell()
{
    // @stub
}

// E:\gamedcs\Hero.h:986
DC_ONLY(0x1fd30, 0x60)
type_point Hero::getTarget(__$ReturnUdt)
{
    // @stub
}

// E:\gamedcs\Hero.h:1046
DC_ONLY(0x1fd90, 0x1A)
unsigned char Hero::spellIsAvailable(int spell)
{
    // @stub
}

// E:\gamedcs\Town.h:311
DC_ONLY(0x1fdac, 0x68)
type_point Town::getLocation(__$ReturnUdt)
{
    // @stub
}

// E:\gamedcs\Town.h:324
DC_ONLY(0x1fe14, 0x98)
bool Town::hasBuilding(int buildingId, bool check_included) const
{
    // @stub
}

// E:\gamedcs\game.h:419
DC_ONLY(0x1feac, 0xE)
long Generator::getOwner()
{
    // @stub
}

// E:\gamedcs\game.h:785
DC_ONLY(0x1febc, 0x40)
unsigned char Game::onSameTeam(int player1, int player2)
{
    // @stub
}

// E:\gamedcs\game.h:933
DC_ONLY(0x1fefc, 0x44)
unsigned char Game::getInfoFlag(GlobalInfoFlags flag, const int playerNum)
{
    // @stub
}

// E:\gamedcs\game.h:1023
DC_ONLY(0x1ff40, 0x58)
Town* Game::getCurrTown()
{
    // @stub
}

// E:\gamedcs\game.h:1024
DC_ONLY(0x1ff98, 0x14)
int Game::GetCurrTownId()
{
    // @stub
}

// E:\gamedcs\game.h:1056
DC_ONLY(0x1ffac, 0x30)
Garrison* Game::getGarrison(int which)
{
    // @stub
}

// E:\gamedcs\game.h:1070
DC_ONLY(0x1ffdc, 0x30)
Boat* Game::getBoat(int which)
{
    // @stub
}

// E:\gamedcs\game.h:1375
DC_ONLY(0x2000c, 0x58)
TownType Game::getAlignment(int player_id)
{
    // @stub
}

// E:\gamedcs\Findpath.h:202
DC_ONLY(0x20064, 0x84)
PathCell* SearchArray::getCell(type_point point, bool flying) const
{
    // @stub
}

// E:\gamedcs\Findpath.h:211
DC_ONLY(0x200e8, 0x20)
void SearchArray::clearPath()
{
    // @stub
}

// E:\gamedcs\Findpath.h:216
DC_ONLY(0x20108, 0x24)
long SearchArray::getPathSteps()
{
    // @stub
}

// E:\gamedcs\Findpath.h:221
DC_ONLY(0x2012c, 0x38)
unsigned char SearchArray::get_step(long i)
{
    // @stub
}

// E:\gamedcs\bitmap816.h:69
DC_ONLY(0x20164, 0xC)
int Bitmap816::getWidth()
{
    // @stub
}

// E:\gamedcs\bitmap816.h:70
DC_ONLY(0x20170, 0xC)
int Bitmap816::getHeight()
{
    // @stub
}

// E:\gamedcs\bitmap816.h:72
DC_ONLY(0x2017c, 0x10)
Palette16* Bitmap816::getPalette()
{
    // @stub
}

// E:\gamedcs\netmsg.h:167
DC_ONLY(0x2018c, 0x2A)
void CNetMsg::CNetMsg(ERSMessages subType, unsigned long size)
{
    // @stub
}

// E:\gamedcs\netmsg.h:444
DC_ONLY(0x201b8, 0x30)
void CTurnUpdateMsg::CTurnUpdateMsg(int gamePos)
{
    // @stub
}

// E:\gamedcs\remote.h:628
DC_ONLY(0x201e8, 0x10)
unsigned char CNetMsgHandler::isInPopup()
{
    // @stub
}

// E:\gamedcs\remote.h:629
DC_ONLY(0x201f8, 0xC)
CNetMsg* CNetMsgHandler::getAbortPopupMsg()
{
    // @stub
}

// E:\gamedcs\remote.h:638
DC_ONLY(0x20204, 0x16)
void CNetMsgHandler::setInPopup(unsigned char b)
{
    // @stub
}

// E:\gamedcs\seerhut.h:117
DC_ONLY(0x2021c, 0x26)
unsigned char SeerHut::PlayerHasInfo(const unsigned char playerNum)
{
    // @stub
}

// E:\gamedcs\seerhut.h:121
DC_ONLY(0x20244, 0x1C)
const char* SeerHut::getName()
{
    // @stub
}

// E:\gamedcs\seerhut.h:123
DC_ONLY(0x20260, 0x20)
const char* SeerHut::GetQuestArtifactName()
{
    // @stub
}

// E:\gamedcs\townmgr.h:738
DC_ONLY(0x20280, 0x18)
const char* TownManager::GetTownTypeName(int type)
{
    // @stub
}

// E:\gamedcs\advmgr.cpp:1084
DC_ONLY(0x20298, 0x50)
void CAdvMgrNetMsgHandler::CAdvMgrNetMsgHandler()
{
    // @stub
}

// E:\gamedcs\advmgr.cpp:11525
DC_ONLY(0x202e8, 0x38)
void* CAdvPopup::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// ..\stlport\stl_string.h:385
DC_ONLY(0x20320, 0x34)
std::basic_string<char,std::char_traits<char>,std::allocator<char>* std::basic_string<char,std::char_traits<char>,std::allocator<char> >::operator=(const std::basic_string<char,std::char_traits<char>,std::allocator<char>* __s)
{
    // @stub
}

// ..\stlport\stl_string.h:579
DC_ONLY(0x20354, 0x24)
std::basic_string<char,std::char_traits<char>,std::allocator<char>* std::basic_string<char,std::char_traits<char>,std::allocator<char> >::operator+=(const std::basic_string<char,std::char_traits<char>,std::allocator<char>* __s)
{
    // @stub
}

// ..\stlport\stl_string.h:580
DC_ONLY(0x20378, 0x24)
std::basic_string<char,std::char_traits<char>,std::allocator<char>* std::basic_string<char,std::char_traits<char>,std::allocator<char> >::operator+=(const char* __s)
{
    // @stub
}

// ..\stlport\stl_string.h:581
DC_ONLY(0x2039c, 0x24)
std::basic_string<char,std::char_traits<char>,std::allocator<char>* std::basic_string<char,std::char_traits<char>,std::allocator<char> >::operator+=(char __c)
{
    // @stub
}

// ..\stlport\stl_bitset.h:564
DC_ONLY(0x203c0, 0x28)
std::bitset<48,unsigned std::bitset<48,unsigned long>::operator[](__$ReturnUdt, unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_bitset.h:376
DC_ONLY(0x203e8, 0x6)
void std::bitset<48,unsigned long>::reference::~reference()
{
    // @stub
}

// ..\stlport\stl_bitset.h:402
DC_ONLY(0x203f0, 0x34)
unsigned char std::bitset<48,unsigned long>::reference::operator bool()
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0x20424, 0x24)
void std::vector<Resource *,std::allocator<Resource *> >::vector<Resource *,std::allocator<Resource *> >(const std::allocator<Resource* __a)
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x20448, 0x8)
void std::allocator<Resource *>::allocator<Resource *>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x20450, 0x6)
void std::allocator<Resource *>::~allocator<Resource *>()
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x20458, 0x16)
unsigned std::vector<NewmapCell::ObjectCell,std::allocator<NewmapCell::ObjectCell> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x20470, 0x28)
NewmapCell::ObjectCell* std::vector<NewmapCell::ObjectCell,std::allocator<NewmapCell::ObjectCell> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x20498, 0x2C)
CObjectType* std::vector<CObjectType,std::allocator<CObjectType> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x204c4, 0x2C)
CObject* std::vector<CObject,std::allocator<CObject> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x204f0, 0x28)
CSprite** std::vector<CSprite *,std::allocator<CSprite *> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x20518, 0x2C)
Mine* std::vector<Mine,std::allocator<Mine> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x20544, 0x2C)
Generator* std::vector<Generator,std::allocator<Generator> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x20570, 0x2C)
Garrison* std::vector<Garrison,std::allocator<Garrison> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x2059c, 0x2C)
Boat* std::vector<Boat,std::allocator<Boat> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x205c8, 0x2C)
University* std::vector<University,std::allocator<University> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x205f4, 0x2C)
CreatureBank* std::vector<CreatureBank,std::allocator<CreatureBank> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x20620, 0x16)
unsigned std::vector<PathCell *,std::allocator<PathCell *> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:204
DC_ONLY(0x20638, 0x28)
PathCell** std::vector<PathCell *,std::allocator<PathCell *> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0x20660, 0x3C)
void std::vector<PathCell *,std::allocator<PathCell *> >::clear()
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0x2069c, 0x24)
void std::vector<DialogResource,std::allocator<DialogResource> >::vector<DialogResource,std::allocator<DialogResource> >(const std::allocator<DialogResource>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0x206c0, 0x34)
void std::vector<DialogResource,std::allocator<DialogResource> >::~vector<DialogResource,std::allocator<DialogResource> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0x206f4, 0x54)
void std::vector<DialogResource,std::allocator<DialogResource> >::push_back(const DialogResource* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0x20748, 0x3C)
void std::vector<DialogResource,std::allocator<DialogResource> >::clear()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x20784, 0x8)
void std::allocator<DialogResource>::allocator<DialogResource>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x2078c, 0x6)
void std::allocator<DialogResource>::~allocator<DialogResource>()
{
    // @stub
}

// ..\stlport\stl_string.h:584
DC_ONLY(0x20794, 0x30)
std::basic_string<char,std::char_traits<char>,std::allocator<char>* std::basic_string<char,std::char_traits<char>,std::allocator<char> >::append(const std::basic_string<char,std::char_traits<char>,std::allocator<char>* __s)
{
    // @stub
}

// ..\stlport\stl_string.h:598
DC_ONLY(0x207c4, 0x3C)
std::basic_string<char,std::char_traits<char>,std::allocator<char>* std::basic_string<char,std::char_traits<char>,std::allocator<char> >::append(const char* __s)
{
    // @stub
}

// ..\stlport\stl_string.h:617
DC_ONLY(0x20800, 0xA4)
void std::basic_string<char,std::char_traits<char>,std::allocator<char> >::push_back(char __c)
{
    // @stub
}

// ..\stlport\stl_bitset.h:370
DC_ONLY(0x208a4, 0x40)
void std::bitset<48,unsigned long>::reference::reference(std::bitset<48,unsigned* __b, unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0x208e4, 0x44)
void std::_Vector_base<Resource *,std::allocator<Resource *> >::_Vector_base<Resource *,std::allocator<Resource *> >(const std::allocator<Resource* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x20928, 0xC)
NewmapCell::ObjectCell* std::vector<NewmapCell::ObjectCell,std::allocator<NewmapCell::ObjectCell> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x20934, 0xC)
CObjectType* std::vector<CObjectType,std::allocator<CObjectType> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x20940, 0xC)
CObject* std::vector<CObject,std::allocator<CObject> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x2094c, 0xC)
CSprite** std::vector<CSprite *,std::allocator<CSprite *> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x20958, 0xC)
Mine* std::vector<Mine,std::allocator<Mine> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x20964, 0xC)
Generator* std::vector<Generator,std::allocator<Generator> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x20970, 0xC)
Garrison* std::vector<Garrison,std::allocator<Garrison> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x2097c, 0xC)
Boat* std::vector<Boat,std::allocator<Boat> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x20988, 0xC)
University* std::vector<University,std::allocator<University> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x20994, 0xC)
CreatureBank* std::vector<CreatureBank,std::allocator<CreatureBank> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x209a0, 0xC)
PathCell** std::vector<PathCell *,std::allocator<PathCell *> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:180
DC_ONLY(0x209ac, 0xC)
PathCell** std::vector<PathCell *,std::allocator<PathCell *> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0x209b8, 0xC)
PathCell** std::vector<PathCell *,std::allocator<PathCell *> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0x209c4, 0x48)
PathCell** std::vector<PathCell *,std::allocator<PathCell *> >::erase(PathCell** __first, PathCell** __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x20a0c, 0xC)
DialogResource* std::vector<DialogResource,std::allocator<DialogResource> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0x20a18, 0xC)
DialogResource* std::vector<DialogResource,std::allocator<DialogResource> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0x20a24, 0x48)
DialogResource* std::vector<DialogResource,std::allocator<DialogResource> >::erase(DialogResource* __first, DialogResource* __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0x20a6c, 0x44)
void std::_Vector_base<DialogResource,std::allocator<DialogResource> >::_Vector_base<DialogResource,std::allocator<DialogResource> >(const std::allocator<DialogResource>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0x20ab0, 0x44)
void std::_Vector_base<DialogResource,std::allocator<DialogResource> >::~_Vector_base<DialogResource,std::allocator<DialogResource> >()
{
    // @stub
}

// ..\stlport\stl_bitset.h:120
DC_ONLY(0x20af4, 0x24)
unsigned long std::_Base_bitset<48,unsigned long>::_S_maskbit(unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_string.h:122
DC_ONLY(0x20b18, 0x1C)
void std::_STL_alloc_proxy<DialogResource *,DialogResource,std::allocator<DialogResource> >::~_STL_alloc_proxy<DialogResource *,DialogResource,std::allocator<DialogResource> >()
{
    // @stub
}

// ..\stlport\stl_bitset.h:124
DC_ONLY(0x20b34, 0x28)
unsigned long* std::_Base_bitset<2,unsigned long>::_M_getword(unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x20b5c, 0x1E)
void std::_STL_alloc_proxy<Resource * *,Resource *,std::allocator<Resource *> >::_STL_alloc_proxy<Resource * *,Resource *,std::allocator<Resource *> >(const std::allocator<Resource* __a, Resource*** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x20b7c, 0x1E)
void std::_STL_alloc_proxy<DialogResource *,DialogResource,std::allocator<DialogResource> >::_STL_alloc_proxy<DialogResource *,DialogResource,std::allocator<DialogResource> >(const std::allocator<DialogResource>* __a, DialogResource** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x20b9c, 0x30)
void std::_STL_alloc_proxy<DialogResource *,DialogResource,std::allocator<DialogResource> >::deallocate(DialogResource* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_bitset.h:117
DC_ONLY(0x20bcc, 0xE)
unsigned std::_Base_bitset<48,unsigned long>::_S_whichbit(unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_bitset.h:111
DC_ONLY(0x20bdc, 0xE)
unsigned std::_Base_bitset<2,unsigned long>::_S_whichword(unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x20bec, 0x2C)
void std::allocator<DialogResource>::deallocate(DialogResource* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_string.c:214
DC_ONLY(0x20c18, 0xEC)
void std::basic_string<char,std::char_traits<char>,std::allocator<char> >::reserve(unsigned __res_arg)
{
    // @stub
}

// E:\gamedcs\DC_precompiledheaders.h:33
DC_ONLY(0x20d04, 0x28)
const int* cppMax(const int* _X, const int* _Y)
{
    // @stub
}

// E:\gamedcs\includes.h:124
DC_ONLY(0x20d2c, 0x38)
const int& tLimit(const int& minimum, const int& value, const int& maximum)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0x20d64, 0x120)
void std::vector<DialogResource,std::allocator<DialogResource> >::_M_insert_overflow(DialogResource* __position, const DialogResource* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_string.h:736
DC_ONLY(0x20e84, 0x2C)
std::basic_string<char,std::char_traits<char>,std::allocator<char> >::assign(char* __first, char* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x20eb0, 0x30)
void std::destroy(DialogResource* __first, DialogResource* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x20ee0, 0x4C)
void std::construct(DialogResource* __p, const DialogResource* __value)
{
    // @stub
}

// ..\stlport\stl_string.h:606
DC_ONLY(0x20f2c, 0x2C)
std::basic_string<char,std::char_traits<char>,std::allocator<char> >::append(char* __first, char* __last)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0x20f58, 0x58)
PathCell** std::copy(PathCell** __first, PathCell** __last, PathCell** __result)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x20fb0, 0x30)
void std::destroy(PathCell** __first, PathCell** __last)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0x20fe0, 0x58)
DialogResource* std::copy(DialogResource* __first, DialogResource* __last, DialogResource* __result)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x21038, 0xA)
std::allocator<DialogResource>* std::__stl_alloc_rebind(std::allocator<DialogResource>* __a, const DialogResource* __formal)
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x21044, 0x16)
unsigned std::vector<DialogResource,std::allocator<DialogResource> >::size()
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x2105c, 0x38)
DialogResource* std::_STL_alloc_proxy<DialogResource *,DialogResource,std::allocator<DialogResource> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x21094, 0x3C)
DialogResource* std::allocator<DialogResource>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x210d0, 0x3C)
DialogResource* std::uninitialized_copy(DialogResource* __first, DialogResource* __last, DialogResource* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0x2110c, 0x3C)
DialogResource* std::uninitialized_fill_n(DialogResource* __first, unsigned __n, const DialogResource* __x)
{
    // @stub
}

// ..\stlport\stl_string.h:755
DC_ONLY(0x21148, 0x8C)
std::basic_string<char,std::char_traits<char>,std::allocator<char> >::_M_assign_dispatch(char* __f, char* __l, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x211d4, 0x8)
DialogResource* std::value_type(const DialogResource* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x211dc, 0x24)
void std::__destroy(DialogResource* __first, DialogResource* __last, DialogResource* __formal)
{
    // @stub
}

// ..\stlport\stl_string.h:698
DC_ONLY(0x21200, 0x48)
std::basic_string<char,std::char_traits<char>,std::allocator<char> >::_M_append_dispatch(char* __f, char* __l, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0x21248, 0x14)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, PathCell** __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0x2125c, 0x8)
int* std::distance_type(PathCell** __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0x21264, 0x40)
PathCell** std::__copy(PathCell** __first, PathCell** __last, PathCell** __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x212a4, 0x8)
PathCell** std::value_type(PathCell** __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x212ac, 0x24)
void std::__destroy(PathCell** __first, PathCell** __last, PathCell** __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0x212d0, 0x14)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const DialogResource* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0x212e4, 0x8)
int* std::distance_type(const DialogResource* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0x212ec, 0x58)
DialogResource* std::__copy(DialogResource* __first, DialogResource* __last, DialogResource* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x21344, 0x30)
DialogResource* std::__uninitialized_copy(DialogResource* __first, DialogResource* __last, DialogResource* __result, DialogResource* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0x21374, 0x30)
DialogResource* std::__uninitialized_fill_n(DialogResource* __first, unsigned __n, const DialogResource* __x, DialogResource* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x213a4, 0x34)
void std::__destroy_aux(DialogResource* __first, DialogResource* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_string.h:645
DC_ONLY(0x213d8, 0x1B0)
std::basic_string<char,std::char_traits<char>,std::allocator<char> >::append(char* __first, char* __last, std::forward_iterator_tag __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x21588, 0x34)
void std::__destroy_aux(PathCell** __first, PathCell** __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x215bc, 0x44)
DialogResource* std::__uninitialized_copy_aux(DialogResource* __first, DialogResource* __last, DialogResource* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0x21600, 0x44)
DialogResource* std::__uninitialized_fill_n_aux(DialogResource* __first, unsigned __n, const DialogResource* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x21644, 0x20)
void std::destroy(DialogResource* __pointer)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:336
DC_ONLY(0x21664, 0x3C)
void std::distance(char* __first, char* __last, int* __n)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x216a0, 0x20)
void std::destroy(PathCell** __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0x216c0, 0x8)
void std::__destroy_aux(DialogResource* __pointer, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:329
DC_ONLY(0x216c8, 0x1C)
void std::__distance(char* __first, char* __last, int* __n, std::random_access_iterator_tag __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0x216e4, 0x8)
void std::__destroy_aux(PathCell** __pointer, __false_type __formal)
{
    // @stub
}

#endif  // @carcass

VA_COMPGEN(0x0041b410, 0xCB, BITSET_XRAN, bitset48)

// COMDAT pairing: append on the char instantiation, mnemonic agreement 0.960.
VA_COMPGEN(0x0041b250, 0xE6, BASIC_STRING_APPEND_STR, char)

// COMDAT pairing: append on the char instantiation, mnemonic agreement 0.955.
VA_COMPGEN(0x0041b340, 0xC2, BASIC_STRING_APPEND_PTR, char)

// <string>'s pointer/count assignment is shared with SendChat (0x4022e0).
// Retail's hero assignment (0x406480) and getArmyHelpText (0x40abe0) call
// it too. It expands in adventuremapwindow but remains naturally emitted
// here: all 161 bytes agree outside four matching named call relocations.
VA_COMPGEN(0x00404150, 0xA1, BASIC_STRING_ASSIGN_PTR_SIZE, char)

VA_COMPGEN(0x0041bc00, 0xD, LOGIC_ERROR_WHAT, char)
