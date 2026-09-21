#include "va.h"
#include "includes.h"

#include <stdlib.h>

#include "advmgr.h"
#include "findpath.h"
#include "game.h"
#include "hero.h"
#include "herospec.h"
#include "kb.h"
#include "quest.h"

// DC struct.h proves the const-reference comparison operators. Their canonical
// definitions now live in struct.h; use them directly instead of TU-local
// duplicate free helpers. The direct != body preserves retail's three tests;
// negating operator== instead scored 64.78% in the earlier buildPath probe.

// E:\gamedcs\search.cpp:32
// Residual (86.3333%): two enregistration choices, both measured unreachable
// on 2026-08-14. (1) retail materialises `this + 0x48` (the `result` vector)
// once into ESI and addresses every field through it (`[esi+4]`, `[esi+8]`),
// where our CL keeps `this` in EDI, reads the fields as `[edi+0x4c]`/`[edi+0x50]`
// and separately computes `lea esi,[edi+0x48]` for the calls. Binding `result`
// to a reference, to a pointer, before or after the loop locals, or for the
// insert only, all give the same 86.4571 - a +0.12 tie among four spellings,
// i.e. different-but-not-retail, so none is landed. (2) retail keeps
// previous_cost (0x30d400) in EBX; our CL spills it into the incoming
// parameter slot [ebp+8]. An EXHAUSTIVE sweep of all 24 orderings of
// {path_cell decl, flying, previous_cost, clear_path()} is byte-flat at
// 86.3333, so statement order does not reach the allocator here.
VA(0x0056a0d0, 0x282)  // anchor-global, dc 0x12b2e0
int searchArray::buildPath(const hero* currentHero, long limit)
{
    type_point source(currentHero->m_x, currentHero->m_y, currentHero->m_z);
    type_point dest(currentHero->m_pathTargetX, currentHero->m_pathTargetY,
                    currentHero->m_pathTargetZ);
    pathCell* currentPathCell;

    unsigned char flying = 0;
    int previousCost = 0x30d400;

    // Source order matters to VC6's register allocator: initializing these
    // loop locals before clearing result raises the retail score materially.
    clearPath();

    while (dest != source) {
        if (!dest.isValid()) {
            clearPath();
            break;
        }

        currentPathCell = getCell(dest, flying);
        if (currentPathCell->m_adjustedCost > previousCost) {
            clearPath();
            break;
        }
        previousCost = currentPathCell->m_adjustedCost;

        if (currentPathCell->m_point != dest || !currentPathCell->m_visited) {
            clearPath();
            break;
        }

        if (currentPathCell->m_cost <= limit)
            m_result.insert(m_result.end(), 1, currentPathCell);

        if (currentPathCell->m_lastPoint == dest) {
            clearPath();
            break;
        }

        dest = currentPathCell->m_lastPoint;
        flying = !currentPathCell->m_lastCanStop;
    }

    return m_result.size();
}

long aiValueOfEvent(const hero* currentHero, type_point point);
int aiResourceCost(const playerData* player, const int* resources);

#ifdef min
#undef min
#endif

// E:\gamedcs\search.cpp:113
// A monster guarding the cell being entered: the pathCell's `monster`
// slot remembers the one already charged for, so a second sighting of the
// same stack costs nothing. The enemy-only search never prices it.
VA(0x0056a360, 0x9E)  // exhaustive search.obj order-map, dc 0x12b3f0
unsigned char checkAdjacentMonster(const hero* currentHero,
                                     pathCell* entryPoint,
                                     type_search_type searchType)
{
    type_point monster;
    if (getMapExtra(entryPoint->m_point.m_x, entryPoint->m_point.m_y,
                    entryPoint->m_point.m_z)
        & MAP_EXTRA_MONSTER) {
        if (g_advManager->findAdjacentMonster(entryPoint->m_point, &monster,
                                              entryPoint->m_monster)) {
            if (searchType == const_AI_enemy_search)
                return 1;
            long value = aiValueOfEvent(currentHero, monster);
            if (value <= -500000000)
                return 1;
            if (value < 0) {
                entryPoint->m_monster = monster;
                entryPoint->m_barrierValue += value;
            }
        }
    }
    return 0;
}

// E:\gamedcs\search.cpp:155
// Every exit of a lith family (or whirlpool) is seeded from the entry
// cell. The AI first prices the worst monster guarding any exit into the
// shared barrier, then pushes one exit cell per list entry: an enemy hero
// standing on an exit is a target, a friendly one is skipped, and the
// exit must be a live trigger of the entry's own type that is not the
// entry itself. Whirlpools cost 16 per exit and a flat 500 of barrier
// value; liths 100 per exit.
VA(0x0056a400, 0x32F)  // exhaustive search.obj order-map, dc 0x12b4a8
void searchArray::enterLith(const hero* currentHero,
                             const std::vector<type_point>* list,
                             long cellType, long excluded,
                             pathCell* entryPoint, long limit,
                             type_search_type searchType)
{
    type_point monster;
    if (entryPoint->m_cost > 0
        && checkAdjacentMonster(currentHero, entryPoint, searchType))
        return;
    int count = list->size();
    long barrierValue = 0;
    if (searchType >= const_AI_search && cellType != LITH_TWOWAY
        && count > 0) {
        for (int i = 0; i < count; i++) {
            type_point exitPoint = (*list)[i];
            NewmapCell* cell = g_game->m_worldMap.cell(exitPoint);
            if (cell->m_type == cellType && cell->m_extraInfo != excluded
                && cell->m_isTrigger) {
                if (g_advManager->findAdjacentMonster(
                        exitPoint, &monster, entryPoint->m_monster)) {
                    long value = aiValueOfEvent(currentHero, monster);
                    if (value <= -500000000)
                        return;
                    if (barrierValue > value)
                        barrierValue = value;
                }
            }
        }
    }
    pathCell exitCell = *entryPoint;
    for (int i = 0; i < count; i++) {
        type_point exitPoint = (*list)[i];
        NewmapCell* cell = g_game->m_worldMap.cell(exitPoint);
        if (cell->m_type == HERO) {
            if (g_game->onSameTeam(g_game->getHero(cell->m_extraInfo)->m_owner,
                                   currentHero->m_owner))
                continue;
        } else if (cell->m_type != cellType || cell->m_extraInfo == excluded
                   || !cell->m_isTrigger) {
            continue;
        }
        monster = entryPoint->m_monster;
        if (searchType >= const_AI_search && cellType != LITH_TWOWAY)
            g_advManager->findAdjacentMonster(exitPoint, &monster,
                                              entryPoint->m_monster);
        exitCell.m_point.m_x = exitPoint.m_x;
        exitCell.m_point.m_y = exitPoint.m_y;
        exitCell.m_point.m_z = exitPoint.m_z;
        if (cellType == WHIRLPOOL) {
            barrierValue -= 500;
            exitCell.m_adjustedCost =
                entryPoint->m_adjustedCost + (count - 1) * 16;
        } else {
            exitCell.m_adjustedCost =
                entryPoint->m_adjustedCost + (count - 1) * 100;
        }
        pushPoint(*entryPoint, exitCell, entryPoint->m_direction, 0, limit,
                  entryPoint->m_barrierValue + barrierValue, monster,
                  cell->m_type == HERO);
    }
}

VA(0x0056a730, 0x111)  // dc 0x12b7f4
void searchArray::enterGate(const pathCell* cell, const NewmapCell* mapCell,
                             long limit)
{
    type_point exitPoint = g_game->getUndergroundGateExit(mapCell);
    const int noGateExit = 0xff;
    if (exitPoint.m_x != noGateExit) {
        pathCell exitCell = *cell;
        NewmapCell* exitMapCell = g_game->m_worldMap.cell(exitPoint);
        exitCell.m_point.m_x = exitPoint.m_x;
        exitCell.m_point.m_y = exitPoint.m_y;
        exitCell.m_point.m_z = exitPoint.m_z;
        pushPoint(*cell, exitCell, cell->m_direction, 0, limit,
                  cell->m_barrierValue, cell->m_monster,
                  exitMapCell->m_type == HERO);
    }
}

// Original: searchArray::board_boat; search.cpp:264, dc 0x12b900.
// Complete expands this ordinary helper in enterTrigger's boat arm.
void searchArray::boardBoat(const hero* currentHero, pathCell& cell)
{
    if (m_payTransitionCosts && !cell.m_inBoat) {
        cell.m_cost += cell.m_moveLeft;
        cell.m_moveLeft = m_seaMovement;
        cell.m_flying = 0;
        cell.m_waterWalking = 0;
    }
    cell.m_inBoat = 1;
    getCell(cell.m_point, !cell.m_canStop)->m_inBoat = 1;
}


// E:\gamedcs\search.cpp:283
// Castle Gate travel between the player's Inferno towns. The AI search
// counts how many gates the treasury can still afford (capped at two:
// one here, one at the far end) and charges each unbuilt gate's cost
// into the barrier; the normal search only routes through built gates.
// A town with a visiting hero cannot receive.
VA(0x0056a850, 0x27E)  // exhaustive search.obj order-map, dc 0x12b988
void searchArray::enterTown(const hero* currentHero, long startTown,
                             const pathCell* currentPathCell, long limit,
                             type_search_type searchType)
{
    const town* ourTown = g_game->getTown(startTown);
    if (!g_game->onSameTeam(ourTown->m_owner, currentHero->m_owner))
        return;
    if (ourTown->m_type != TOWN_INFERNO)
        return;
    long barrierValue = 0;
    int gates = 0;
    int* cost = ourTown->getBuildCostArray(EXTRA_1_ID);
    playerData* player = currentHero->getPlayer();
    if (searchType == const_AI_search) {
        gates = 2;
        for (int i = 0; i < 7; i++) {
            if (cost[i] > 0) {
                gates = min(player->m_resources[i] / cost[i], gates);
                if (!gates)
                    break;
            }
        }
    }
    if (!ourTown->hasBuilding(EXTRA_1_ID, 1)) {
        if (!ourTown->canBuild(EXTRA_1_ID))
            return;
        if (!gates)
            return;
        barrierValue = -aiResourceCost(player, cost);
        gates--;
    }
    pathCell newCell;
    for (int i = 0; i < player->m_numTowns; i++) {
        if (player->m_townIds[i] == startTown)
            continue;
        town* otherTown = g_game->getTown(player->m_townIds[i]);
        if (otherTown->m_type != TOWN_INFERNO)
            continue;
        if (otherTown->m_visitingHeroId >= 0)
            continue;
        newCell = *currentPathCell;
        if (!otherTown->hasBuilding(EXTRA_1_ID, 1)) {
            if (!otherTown->canBuild(EXTRA_1_ID))
                continue;
            if (!gates)
                continue;
            newCell.m_barrierValue -= aiResourceCost(player, cost);
        }
        newCell.m_point.m_x = otherTown->m_mapX;
        newCell.m_point.m_y = otherTown->m_mapY;
        newCell.m_point.m_z = otherTown->m_mapZ;
        newCell.m_castleGate = 1;
        pushPoint(*currentPathCell, newCell, 0, 0, limit,
                  newCell.m_barrierValue + barrierValue, newCell.m_monster,
                  0);
    }
}

VA(0x0056aad0, 0x68)  // dc 0x12bbc8
unsigned char searchArray::enterHostileTrigger(const hero* currentHero,
                                              pathCell& cell)
{
    if (cell.m_point != cell.m_monster) {
        type_point point = cell.m_point;
        long value = aiValueOfEvent(currentHero, point);
        if (value <= -500000000)
            return 0;
        if (value < 0) {
            cell.m_barrierValue += value;
            cell.m_monster = cell.m_point;
        }
    }
    return 1;
}

// E:\gamedcs\search.cpp:393
// THE TRIGGER-CELL DISPATCH: what the search does on landing on an
// object cell, keyed on the object type (retail lowers it as one
// 208-entry jump table over types 8..215, so the arm order below is the
// source's). The normal search only ever stops at garrisons and border
// gates; the AI searches price and seed everything else. Returns whether
// the cell is enterable at all.

// The border-guard/gate pair shares one arm (the guard first insists on
// the full AI search), the garrison falls into the monster's
// `search_type >= const_AI_search` answer, and the hero arm's identical
// answer is cross-jumped onto it.
VA(0x0056ab40, 0x50C)  // exhaustive search.obj order-map, dc 0x12bc3c
unsigned char searchArray::enterTrigger(const hero* currentHero,
                                         pathCell* cell, long limit,
                                         type_search_type searchType)
{
    NewmapCell* mapCell = g_advManager->getCell(cell->m_point);
    int type = mapCell->m_type;
    if (searchType == const_normal_search && type != GARRISON
        && type != BORDER_GATE)
        return 0;
    switch (type) {
    case BORDER_GUARD:
        if (searchType < const_AI_search)
            return 0;
    case BORDER_GATE: {
        unsigned char visited =
            (g_game->m_borderTentVisitFlags[mapCell->m_objectIndex]
             & g_unnamed69ccc4)
            != 0;
        return visited;
    }
    case QUEST_GUARD: {
        if (searchType < const_AI_search)
            return 0;
        TQuestGuard* guard =
            &g_game->m_worldMap.m_questGuardList[mapCell->m_extraInfo];
        if (!guard->m_quest || guard->m_quest->hasExpired())
            return 0;
        if (!(guard->m_visitedPlayers & (1 << currentHero->m_owner)))
            return 1;
        if (!guard->m_quest->isSatisfied(const_cast<hero*>(currentHero)))
            return 0;
        type_quest* quest = guard->m_quest;
        int player = currentHero->m_owner;
        cell->m_barrierValue -= quest->getAIValue(player);
        return 1;
    }
    case HERO:
        if (g_game->onSameTeam(g_game->m_heroAvailability[mapCell->m_extraInfo],
                               currentHero->m_owner))
            return 0;
        if (searchType == const_AI_enemy_search)
            return 1;
        if (searchType >= const_AI_search)
            return 1;
        return 0;
    case GARRISON: {
        garrison* currentGarrison =
            g_game->getGarrison(mapCell->m_extraInfo);
        if (g_game->onSameTeam(currentGarrison->m_playerOwner,
                               currentHero->m_owner))
            return 1;
    }
    case MONSTER:
        if (searchType >= const_AI_search)
            return 1;
        return 0;
    case BOAT:
        if (cell->m_inBoat)
            return 0;
        if (searchType < const_AI_search)
            return 0;
        boardBoat(currentHero, *cell);
        return 1;
    case UNDERGROUND_GATE:
        if (searchType < const_AI_enemy_search)
            return 0;
        if (cell->m_cost > 0
            && checkAdjacentMonster(currentHero, cell, searchType))
            return 0;
        enterGate(cell, mapCell, limit);
        return 0;
    case LITH_ONEWAY_ENTRANCE:
        if (searchType < const_AI_enemy_search)
            return 0;
        enterLith(currentHero, &g_game->getLithExits(mapCell->m_objectIndex),
                   LITH_ONEWAY_EXIT, -1, cell, limit, searchType);
        return 0;
    case LITH_TWOWAY:
        if (searchType < const_AI_enemy_search)
            return 0;
        enterLith(currentHero, &g_game->getLiths(mapCell->m_objectIndex),
                   LITH_TWOWAY, mapCell->m_extraInfo, cell, limit,
                   searchType);
        return 0;
    case WHIRLPOOL:
        if (searchType < const_AI_enemy_search)
            return 0;
        enterLith(currentHero, &g_game->getWhirlpools(), WHIRLPOOL,
                   mapCell->m_extraInfo, cell, limit, searchType);
        return 0;
    case TOWN:
        if (searchType < const_AI_enemy_search)
            return 0;
        if (checkAdjacentMonster(currentHero, cell, searchType))
            return 0;
        enterTown(currentHero, mapCell->m_extraInfo, cell, limit,
                   searchType);
        return 1;
    }
    return g_adventureObjectLandBlocked[type][0] == 0;
}

// E:\gamedcs\search.cpp:494
// Dreamcast retains this file-static helper as a separate procedure and
// SeedPosition calls it. Complete's VC6 expands the same source boundary at
// the only retail call site; keep the helper ordinary and available before
// the caller so the compiler makes that decision naturally.
static unsigned char checkSummonBoat(const hero* currentHero)
{
    if (!currentHero->canSummonBoat())
        return 0;

    type_point point = currentHero->getLocation();
    type_point newPoint;
    // DC has no breakpoint row for source line 502 between new_point (501)
    // and the loop (503). The original VC6-era spelling declares the
    // register-promoted counter separately; both target compilers erase it.
    int i;
    for (i = 0; i < MAP_DIRECTION_COUNT; ++i) {
        newPoint.m_x = point.m_x + g_normalDirTable[i].m_x;
        newPoint.m_y = point.m_y + g_normalDirTable[i].m_y;
        newPoint.m_z = point.m_z;
        if (newPoint.isValid()
            && g_game->m_worldMap.cell(newPoint)->m_type == BOAT
            && g_game->getCell(newPoint)->m_isTrigger)
            return 0;
    }
    return 1;
}

VA(0x0056b050, 0x3E7)  // dc 0x12bfe0
void searchArray::checkTownPortal(const hero* currentHero,
                                    const pathCell* startCell,
                                    long maxMobility)
{
    hero* caster = const_cast<hero*>(currentHero);
    if (!currentHero->spellIsAvailable(SPELL_TOWN_PORTAL))
        return;
    if (currentHero->m_mana < caster->getManaCost(SPELL_TOWN_PORTAL) + 20)
        return;
    if (startCell->m_inBoat)
        return;
    std::vector<type_point> destinations;
    playerData* player = caster->getPlayer();
    int i;
    if (caster->getSpellLevel(SPELL_TOWN_PORTAL) >= eMasteryAdvanced) {
        for (i = 0; i < player->m_numTowns; i++) {
            town* currentTown = g_game->getTown(player->m_townIds[i]);
            if (currentTown->m_visitingHeroId < 0)
                destinations.push_back(currentTown->getLocation());
        }
    } else {
        town* closestTown = 0;
        long closest = 0;
        for (i = 0; i < player->m_numTowns; i++) {
            town* currentTown = g_game->getTown(player->m_townIds[i]);
            if (currentTown->m_mapZ != startCell->m_point.m_z)
                continue;
            int deltaY = currentTown->m_mapY - startCell->m_point.m_y;
            int deltaX = currentTown->m_mapX - startCell->m_point.m_x;
            int distance = deltaX * deltaX + deltaY * deltaY;
            if (!closestTown || distance < closest) {
                closestTown = currentTown;
                closest = distance;
            }
        }
        if (!closestTown)
            return;
        if (closestTown->m_visitingHeroId >= 0)
            return;
        destinations.push_back(closestTown->getLocation());
    }
    pathCell newCell;
    int cost = caster->getSpellLevel(SPELL_TOWN_PORTAL) == eMasteryExpert
        ? 200 : 300;
    for (unsigned int destinationIndex = 0;
         destinationIndex < destinations.size(); destinationIndex++) {
        newCell = *startCell;
        newCell.m_point.m_x = destinations[destinationIndex].m_x;
        newCell.m_point.m_y = destinations[destinationIndex].m_y;
        newCell.m_point.m_z = destinations[destinationIndex].m_z;
        newCell.m_townPortal = 1;
        int distance = abs(newCell.m_point.m_z - startCell->m_point.m_z)
                * (g_mapHeight + g_mapWidth) / 2
            + abs(newCell.m_point.m_x - startCell->m_point.m_x)
            + abs(newCell.m_point.m_y - startCell->m_point.m_y);
        newCell.m_adjustedCost += (distance + 4) * 50;
        pushPoint(*startCell, newCell, 0, cost, maxMobility, 0,
                  startCell->m_monster, 0);
    }
}

// E:\gamedcs\search.cpp:621
VA(0x0056b440, 0x8EC)  // exhaustive search.obj order-map, dc 0x12c36c
void searchArray::seedPosition(hero* currentHero, type_point start,
                               type_point target, int maxMobility,
                               unsigned char isBoat,
                               type_search_type searchType,
                               int curTempMobility,
                               unsigned char seedContinuation)
{
    TSkillMastery pathfinding =
        currentHero->getSecondarySkill(eSecSkillPathfinding);
    m_thisTurnsMovement = curTempMobility;
    // Complete snapshots the underlying byte directly (`mov dl,[hero+6]`).
    // Dreamcast's older source calls is_on_map here, but its bool facade
    // normalizes the value; the retail load is direct evidence for this
    // later-revision spelling.
    unsigned char wasOnMap = currentHero->isOnMap();
    currentHero->restoreCell();

    m_limitReached = 0;
    m_payTransitionCosts = 0;
    m_canSummonBoat = 0;
    m_canCastTeleport = 0;
    m_canCastFlight = 0;
    m_canCastWaterWalk = 0;
    m_landMovement = currentHero->getMobility(0);
    m_seaMovement = currentHero->getMobility(1);
    m_flightLevel = currentHero->m_flightLevel;
    m_waterWalkLevel = currentHero->m_waterWalkLevel;

    if (currentHero->isWieldingArtifact(0x48))
        m_flightLevel = eMasteryExpert;
    if (currentHero->isWieldingArtifact(0x5a))
        m_waterWalkLevel = eMasteryExpert;

    unsigned char flying = m_flightLevel > eMasteryInvalid && !isBoat;
    unsigned char waterWalking =
        m_waterWalkLevel > eMasteryInvalid && !isBoat;

    if (searchType >= const_AI_search) {
        m_payTransitionCosts = 1;
        if (searchType == const_AI_search) {
            if (currentHero->spellIsAvailable(SPELL_DIMENSION_DOOR)
                && currentHero->getManaCost(SPELL_DIMENSION_DOOR) + 20
                       <= currentHero->m_mana)
                m_canCastTeleport = 1;

            m_canSummonBoat = checkSummonBoat(currentHero);

            if (currentHero->isWieldingArtifact(0x48)) {
                m_canCastFlight = 1;
            } else {
                m_canCastFlight =
                    currentHero->spellIsAvailable(SPELL_FLY)
                    && currentHero->getManaCost(SPELL_FLY) + 20
                           <= currentHero->m_mana;
                m_flightLevel = currentHero->getSpellLevel(SPELL_FLY);
            }

            if (currentHero->isWieldingArtifact(0x5a)) {
                m_canCastWaterWalk = 1;
            } else {
                m_canCastWaterWalk =
                    currentHero->spellIsAvailable(SPELL_WATER_WALK)
                    && currentHero->getManaCost(SPELL_WATER_WALK) + 20
                           <= currentHero->m_mana;
                m_waterWalkLevel =
                    currentHero->getSpellLevel(SPELL_WATER_WALK);
            }

            if (currentHero->m_flightLevel > eMasteryInvalid
                && m_flightLevel > currentHero->m_flightLevel)
                m_flightLevel = currentHero->m_flightLevel;
            if (currentHero->m_waterWalkLevel > eMasteryInvalid
                && m_waterWalkLevel > currentHero->m_waterWalkLevel)
                m_waterWalkLevel = currentHero->m_waterWalkLevel;
        }
    }

    if (m_cellData == 0)
        init();

    if (!seedContinuation) {
        g_advManager->m_fullySeeded = 0;
        int flyLevel;
        if (searchType == const_normal_search) {
            flyLevel = m_waterWalkLevel > eMasteryInvalid
                        || m_flightLevel > eMasteryInvalid;
        } else {
            flyLevel = m_waterWalkLevel > eMasteryInvalid
                        || m_flightLevel > eMasteryInvalid
                        || m_canCastTeleport || m_canCastFlight
                        || m_canCastWaterWalk;
        }
        clear(flyLevel, 0, g_game->getNumMapLevels());
    }

    g_advManager->m_seedingValid = 1;
    type_point monster(-1, -1, -1);
    pathCell cell;

    if (!seedContinuation) {
        cell.m_point.m_x = start.m_x;
        cell.m_point.m_y = start.m_y;
        cell.m_point.m_z = start.m_z;
        cell.m_inBoat = isBoat;
        cell.m_cost = 0;
        cell.m_dangerValue = 0;
        // Dreamcast's older order is flying, move_left, transition flags,
        // then water_walking (lines 727..733). Complete's retail lowering
        // clears bits 4..9 with one 0xfffffc0f mask, then inserts both mode
        // bits before the movement store. With the pinned VC6 compiler, that
        // sequence requires the later revision's contiguous flag group.
        cell.m_townPortal = 0;
        cell.m_dimensionDoor = 0;
        cell.m_castleGate = 0;
        cell.m_startAtTrigger = 0;
        cell.m_flying = flying;
        cell.m_waterWalking = waterWalking;
        cell.m_moveLeft = curTempMobility;
        cell.m_canStop = 1;

        // Complete adds terrain-aware stopping here. Use the canonical
        // Hero.h helpers (checkTerrain=1), which own the private canLand call.
        if (currentHero->isFlying(1) || currentHero->canWalkOnWater(1))
            cell.m_canStop = 0;

        // Complete inserts the can-land rule before materializing this value;
        // retail keeps the constructor stores on the far side of both calls.
        cell.m_lastPoint = type_point(-1, -1, -1);
        cell.m_adjustedCost = 0;

        pushPoint(cell, cell, 2, 0, maxMobility, 0, monster, 0);

        if (searchType >= const_AI_enemy_search) {
            int startTown = g_game->getTownId(
                cell.m_point.m_x, cell.m_point.m_y, cell.m_point.m_z);
            if (startTown >= 0) {
                enterTown(currentHero, startTown, &cell, maxMobility,
                           searchType);
            } else {
                NewmapCell* mapCell = g_game->m_worldMap.cell(cell.m_point);
                if (mapCell->m_isTrigger
                    && (mapCell->m_type == LITH_ONEWAY_ENTRANCE
                        || mapCell->m_type == LITH_TWOWAY
                        || mapCell->m_type == UNDERGROUND_GATE)) {
                    pathCell triggerCell = cell;
                    triggerCell.m_startAtTrigger = 1;
                    triggerCell.m_adjustedCost += 50;
                    enterTrigger(currentHero, &triggerCell, maxMobility,
                                  searchType);
                }
            }
        }
        if (searchType == const_AI_search)
            checkTownPortal(currentHero, &cell, maxMobility);
    }

    pathCell nextCell;
    TTerrainType nativeTerrain = currentHero->m_army.getNativeTerrain();

    while (m_queue.size()) {
        cell = m_queue.back();
        m_queue.pop_back();
        cell = *getCell(cell.m_point, !cell.m_canStop);

        if (m_queue.size()) {
            nextCell = m_queue.back();
            if (nextCell.m_point == cell.m_point
                && nextCell.m_canStop == cell.m_canStop)
                continue;
        }

        cell.m_townPortal = 0;
        cell.m_castleGate = cell.m_startAtTrigger = 0;
        long turnMobility = curTempMobility - cell.m_cost;

        if (cell.m_isTrigger) {
            if (!enterTrigger(currentHero, &cell, maxMobility,
                               searchType))
                continue;
        }

        unsigned char adjacentMonster = 0;
        if (cell.m_inBoat) {
            if (g_advManager->getCell(cell.m_point)->m_groundSet
                != eTerrainWater)
                continue;
        }

        if (!cell.m_flying && !cell.m_dimensionDoor
            && searchType < const_AI_search
            && (getMapExtra(cell.m_point.m_x, cell.m_point.m_y, cell.m_point.m_z)
                & MAP_EXTRA_MONSTER)
            && cell.m_point != start
            && g_advManager->findAdjacentMonster(
                cell.m_point, &monster, cell.m_monster))
            adjacentMonster = 1;

        testPossibleDirections(currentHero, &cell, turnMobility,
                               maxMobility, adjacentMonster, monster,
                               pathfinding, searchType, nativeTerrain);

        if (target.isValid() && getCell(target, false)->m_visited) {
            if (wasOnMap)
                currentHero->obscureCell();
            return;
        }
    }

    g_advManager->m_fullySeeded = 1;
    if (wasOnMap)
        currentHero->obscureCell();
}

VA_COMPGEN(0x0056bd30, 0x33, VECTOR_ERASE, pathCell)
