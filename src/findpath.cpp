#include <va.h>
#include <stdlib.h>
#include <string.h>
#include "advmgr.h"
#include "army.h"
#include "cmbtmgr.h"
#include "findpath.h"
#include "game.h"
#include "herospec.h"  // TSkillMastery, for the Dimension Door mastery test
#include "kb.h"
#include "path.h"
#include "includes.h"

// ai_player.cpp:4643. Kept local because findpath's narrow include set does
// not otherwise depend on the ai_player class declarations.
long aiGetShipCost(const hero* ourHero, type_point point);

VA(0x004b1330, 0x3B)  // dc 0x9ed40
bool type_point::isValid() const
{
    return m_x >= 0 && m_x < g_mapWidth && m_y >= 0 && m_y < g_mapHeight;
}

VA(0x004b1370, 0x62)  // dc 0x9ed88
searchArray::searchArray()
{
    m_cellData = 0;
    m_isMoatSlowed = 0;
    m_dangerZones = 0;
    m_maxQueueCount = 0;
    m_payTransitionCosts = 0;
    m_thisTurnsMovement = 0;
    m_landMovement = 0;
    m_seaMovement = 0;
    m_canSummonBoat = 0;
    m_canCastTeleport = 0;
    m_waterWalkLevel = -1;
    m_flightLevel = -1;
    m_limitReached = 0;
}

VA(0x004b13e0, 0x78)  // dc 0x9ee08
searchArray::~searchArray()
{
    if (m_cellData)
        delete m_cellData;
    if (m_isMoatSlowed)
        delete m_isMoatSlowed;
    m_cellData = 0;
    m_isMoatSlowed = 0;
}

VA(0x004b1460, 0x9F)  // dc 0x9ee34
void searchArray::init()
{
    if (m_cellData)
        delete m_cellData;
    if (m_isMoatSlowed)
        delete m_isMoatSlowed;
    m_cellData = 0;
    m_isMoatSlowed = 0;
    m_validRectangle.left = 0;
    m_validRectangle.right = g_mapWidth;
    m_validRectangle.top = 0;
    m_validRectangle.bottom = g_mapHeight;
    m_cellData = new pathCell[(g_game->m_worldMap.getNumLevels()) * g_mapHeight
            * g_mapWidth * 2];
    m_isMoatSlowed = new unsigned char[187];
}

VA(0x004b1500, 0x2F)  // dc 0x9eee8
void searchArray::close()
{
    if (m_cellData)
        delete m_cellData;
    if (m_isMoatSlowed)
        delete m_isMoatSlowed;
    m_cellData = 0;
    m_isMoatSlowed = 0;
}

VA(0x004b1530, 0x20F)  // dc 0x9ef20
void searchArray::clear(long flyLevel, long startZ, long stopZ)
{
    m_queue.clear();
    m_result.clear();
    m_visitedPoints.clear();

    long width = m_validRectangle.right - m_validRectangle.left;
    if (width <= 0)
        return;

    type_point point;
    point.m_x = static_cast<short>(m_validRectangle.left);
    for (point.m_z = static_cast<short>(startZ); point.m_z < stopZ; point.m_z++) {
        for (long fly = 0; fly <= flyLevel; fly++) {
            for (point.m_y = static_cast<short>(m_validRectangle.top); point.m_y < m_validRectangle.bottom;
                    point.m_y++) {
                pathCell* row = m_cellData;
                if (row != 0) {
                    unsigned char plane = fly != 0;
                    row += ((point.m_z * 2 + plane) * g_mapHeight
                            + point.m_y) * g_mapWidth + point.m_x;
                }
                memset(row, 0, width * sizeof(pathCell));
            }
        }
    }
}

// THE TERRAIN COST TABLES, all four in findpath's own .rdata/.data
// run and all four read only from CalcTerrainCost (0x4b1740), which
// is why they are claimed here. Every read is a scaled index off the
// symbol with a ZERO addend, so each table pairs cleanly.

// gTerrainCost is thirteen four-wide rows because the same table
// serves two index domains: rows 0..8 are the nine TTerrainType
// grounds (row 4 = Swamp is the expensive one, 175/150/125/100 by
// Pathfinding mastery; row 1 = Sand and row 3 = Snow are 150/125,
// row 5 = Rough is 125), row 9 is an unused zero row, and rows 10..12
// are the three ROAD surfaces at a flat 75 / 65 / 50 percent. The
// road rows are reached through gRoadCostRow, which maps a
// NewmapCell::RoadSet id straight onto the row - so the table is one
// array with a road appendix, not two.
DATA(0x0063e510) const long g_terrainCost[13][4] = {
    { 100, 100, 100, 100 },
    { 150, 125, 100, 100 },
    { 100, 100, 100, 100 },
    { 150, 125, 100, 100 },
    { 175, 150, 125, 100 },
    { 125, 100, 100, 100 },
    { 100, 100, 100, 100 },
    { 100, 100, 100, 100 },
    { 100, 100, 100, 100 },
    {   0,   0,   0,   0 },
    {  75,  75,  75,  75 },
    {  65,  65,  65,  65 },
    { 50,  50,  50,  50 }
};
// 0x3fb504f3 exactly - the float nearest sqrt(2), and the multiplier
// every diagonal step pays. Written as a named `const float` because
// it sits in the middle of this TU's own .rdata run rather than in the
// compiler's float pool.
DATA(0x0063e5e0) const long g_roadCostRow[4] = { 0, 10, 11, 12 };
// The .data twin, indexed by TSkillMastery instead of by terrain: what
// a tile costs a hero who is FLYING or WATER-WALKING over it. Expert
// costs the same 100 as flat ground; the lower masteries pay a
// surcharge. Name is a bootstrap invention - no roster reaches it.
DATA(0x0063e5f0) const float g_diagonalCost = 1.4142135f;
DATA(0x006778ac) long g_masteryTerrainCost[4] = { 140, 140, 120, 100 };

// simply forwards its own trailing parameter. Retail GetTerrainCost calls
// GetCreatureTotal with creature 0x8e at 0x4b1a1b/0x4b1a22 and passes
// its positive-result byte to CalcTerrainCost: hasNomad names that proven
// creature predicate, which removes the Sand penalty.

VA(0x004b1740, 0x13E)  // dc 0x9f034
int calcTerrainCost(const NewmapCell* cell, int dir, int pointsLeft,
                    long pathfinding, long endRoad, long flying,
                    long waterWalking, long nativeTerrain,
                    unsigned char hasNomad)
{
    long terrain = cell->m_groundSet;
    long road = cell->m_roadSet;
    if (hasNomad && terrain == eTerrainSand)
        terrain = eTerrainDirt;
    TAdventureObjectType special = cell->getSpecialTerrain();
    long cost;
    if (road != 0 && endRoad != 0)
        cost = g_terrainCost[g_roadCostRow[road]][pathfinding];
    else if (terrain == nativeTerrain && special != CURSED_GROUND)
        cost = 100;
    else
        cost = g_terrainCost[terrain][pathfinding];
    // A third off every sea step. 0xe1 is one of the ten object ids
    // get_special_terrain (0x4fce20) can answer with and the only one
    // this body reacts to on water; mapcell.h carries the whole
    // ten-value answer set now, with the provenance of the eight SoD
    // spellings spelled out there.
    if (terrain == eTerrainWater
            && special == FAVORABLE_WINDS)
        cost = cost * 2 / 3;
    if (waterWalking >= 0 && terrain == eTerrainWater)
        cost = g_masteryTerrainCost[waterWalking];
    if (flying >= 0) {
        if ((cell->m_flags0011 & 0x40) && terrain != eTerrainWater)
            cost = cppMin(cost, g_masteryTerrainCost[flying]);
        else
            cost = g_masteryTerrainCost[flying];
    }
    if (dir & 1) {
        long full = g_terrainCost[terrain][pathfinding];
        if (pointsLeft < full
                || static_cast<float>(pointsLeft)
                        >= static_cast<float>(full) * g_diagonalCost)
            cost = static_cast<long>(static_cast<float>(cost) * g_diagonalCost);
    }
    return cost;
}

VA(0x004b1880, 0x33)  // dc 0x9f154
int minimumTerrainCost(const NewmapCell* cell, int pointsLeft,
                       long pathfinding, long flying, long waterWalking,
                       unsigned char hasNomad)
{
    return calcTerrainCost(cell, 0, pointsLeft, pathfinding, cell->m_roadSet,
                           flying, waterWalking,
                           cell->m_groundSet, hasNomad);
}

VA(0x004b18c0, 0x1A2)  // dc 0x9f184
int getTerrainCost(hero* currentHero, type_point start, int direction, int moveLeft)
{
    const int destX = start.m_x + g_stepDeltaX[4 * direction];
    const int destY = start.m_y + g_stepDeltaY[4 * direction];
    NewmapCell* from = g_game->m_worldMap.cell(start.m_x, start.m_y, start.m_z);
    type_point to(destX, destY, start.m_z);
    NewmapCell* dest = g_game->m_worldMap.cell(to.m_x, to.m_y, to.m_z);
    long flying = currentHero->m_flightLevel;
    long waterWalking = currentHero->m_waterWalkLevel;
    if (currentHero->isWieldingArtifact(0x48))
        flying = 3;
    if (currentHero->isWieldingArtifact(0x5a))
        waterWalking = 3;
    if (currentHero->m_flags & 0x40000)
        waterWalking = flying = -1;
    long mastery = currentHero->m_skillLevel[0];
    return calcTerrainCost(from, direction, moveLeft, mastery,
                           dest->m_roadSet, flying, waterWalking,
                           currentHero->m_army.getNativeTerrain(),
                           currentHero->m_army.getCreatureTotal(
                               CREATURE_NOMAD) > 0);
}

// E:\gamedcs\findpath.cpp:271..447 (dc 0x9f2a4). Dreamcast proves
// const pathCell& old_cell, pathCell& point, the local upper/lower binary
// search, and the eight point stores at lines 429..436. Retail's ret 0x20
// agrees with the eight stack arguments. Original local spellings include
// adjusted_cost, pCell, move_cost, barrier_value, delta_x and delta_y.
// The queue orders barrier plus danger after this turn's movement, with
// adjusted cost breaking ties. Only after queue/visited insertion is the
// point copied to its grid cell. Retail's dead erase copy loop requires
// erase(end()-1) for the 500-entry cap; DC line 396 instead calls pop_back.

// Removed the old terrainForbidsMagic, findQueueSlot and fillPathCell
// compiler-budget probes and the tail-insert inline_depth(0) pin. Their
// prior 87.5488% peak is superseded; none had source-boundary evidence.
// The real game::get_cell accessor remains a canonical game.h inline.

// 2026-09-08: 72 public vector-overload/arm-order/midpoint-lifetime
// hypotheses recover 97.8175% with DC's push_back/insert/push_back sequence.
// Retail also supports the explicit three-way key comparison, rather than
// the probe's combined Boolean test. Computing the first midpoint before
// the loop and updating it after each bound recovers retail's loop layout;
// the equivalent top-tested loop is 94.3098%. All 72 candidates pass an
// independent linear-order oracle across 12,800 cap/tie/cost/alias cases.
// A further 24 declaration/scope/erase-iterator controls reach 98.2622%
// by initializing cheaper before the key comparison, as retail does.
// Hoisting upper/lower or adding visited-branch braces is byte-flat;
// a named erase iterator falls to 98.1234%. No other TU score falls.
// Residual: stack homes, the folded erase iterator and pointer-vector
// _Destroy retention in visitedPoints' grow arm. All interior queue-insert
// calls, including the retained _Construct<pathCell>, now agree naturally.
VA(0x004b1a70, 0x88D)  // anchor-bracket, dc 0x9f2a4
void searchArray::pushPoint(const pathCell& oldCell, pathCell& point,
                            int direction, int moveCost, int limit,
                            long barrierValue, type_point monster,
                            int isTrigger)
{
    long cost = oldCell.m_cost + moveCost;
    long adjustedCost = point.m_adjustedCost - point.m_cost + cost;

    if (!point.m_point.isValid())
        return;
    if (point.m_point.m_x < m_validRectangle.left || point.m_point.m_x >= m_validRectangle.right
            || point.m_point.m_y < m_validRectangle.top || point.m_point.m_y >= m_validRectangle.bottom)
        return;

    point.m_lastPoint = oldCell.m_point;
    point.m_lastCanStop = oldCell.m_canStop;
    if (point.m_dimensionDoor) {
        if (point.m_canStop && isTrigger)
            return;
        if (!oldCell.m_canStop) {
            point.m_lastPoint = oldCell.m_lastPoint;
            point.m_lastCanStop = 1;
        }
        long deltaX = point.m_point.m_x - point.m_lastPoint.m_x;
        long deltaY = point.m_point.m_y - point.m_lastPoint.m_y;
        if (abs(deltaX) > 9)
            return;
        if (abs(deltaY) > 8)
            return;
        point.m_deltaX = deltaX;
        point.m_deltaY = deltaY;
    } else {
        point.m_deltaX = 0;
        point.m_deltaY = 0;
    }

    long danger = 0;
    if (m_dangerZones != 0) {
        danger = *getDangerCell(m_dangerZones, point.m_point);
        if (cost > m_thisTurnsMovement) {
            danger = cppMin(oldCell.m_dangerValue, danger);
            // The "unreachable" sentinel the danger map carries; every
            // producer that vetoes a square outright writes a value at or
            // below it. Spelled as the literal retail compares against.
            if (danger <= -500000000)
                return;
        }
    }

    pathCell* cell = getCell(point.m_point, !point.m_canStop);

    unsigned char cheaper = 0;
    long key = barrierValue;
    if (cost > m_thisTurnsMovement)
        key = danger + barrierValue;

    if (cell->m_visited) {
        long cellKey = cell->m_barrierValue;
        if (cell->m_cost > m_thisTurnsMovement)
            cellKey += cell->m_dangerValue;
        if (cellKey > key)
            return;
        if (cellKey < key)
            cheaper = 1;
        else if (adjustedCost >= cell->m_adjustedCost)
            return;
    }

    if (cost > limit && limit > 0 && !cheaper) {
        m_limitReached = 1;
        return;
    }

    if (cell->m_visited) {
        point.m_magicForbidden = cell->m_magicForbidden;
    } else {
        point.m_magicForbidden = 0;
        if (m_canCastTeleport || m_canSummonBoat || m_canCastFlight
                || m_canCastWaterWalk) {
            NewmapCell* mapCell = g_game->getCell(point.m_point);
            TAdventureObjectType special = mapCell->getSpecialTerrain();
            if (special == CURSED_GROUND || special == GARRISON)
                point.m_magicForbidden = 1;
        }
    }

    if (m_queue.size() >= 500)
        m_queue.erase(m_queue.end() - 1);

    int upper = m_queue.size();
    int lower = 0;
    int middle = (upper + lower) >> 1;
    while (upper > lower) {
        long entryKey = m_queue[middle].m_barrierValue;
        if (m_queue[middle].m_cost > m_thisTurnsMovement)
            entryKey += m_queue[middle].m_dangerValue;
        if (entryKey < key)
            lower = middle + 1;
        else if (entryKey > key)
            upper = middle;
        else if (adjustedCost < m_queue[middle].m_adjustedCost)
            lower = middle + 1;
        else
            upper = middle;
        middle = (upper + lower) >> 1;
    }

    point.m_direction = direction;
    point.m_cost = static_cast<unsigned short>(cost);
    point.m_adjustedCost = static_cast<unsigned short>(adjustedCost);
    point.m_barrierValue = barrierValue;
    point.m_dangerValue = danger;
    point.m_isTrigger = isTrigger;
    point.m_monster = monster;
    point.m_visited = 1;

    if (middle >= m_queue.size())
        m_queue.push_back(point);
    else
        m_queue.insert(m_queue.begin() + middle, point);

    if (!cell->m_visited && point.m_canStop)
        m_visitedPoints.push_back(cell);

    *cell = point;
}

// E:\gamedcs\findpath.cpp:461
// `ret 0x24` = nine stack arguments over `this`, the DC count exactly.
// The opening three instructions corroborate the second parameter:
// `mov ebx,[ebp+0xc]; mov eax,[ebx]; call advManager::GetCell` - it
// reads the type_point at pathCell+0 out of `source` and asks the
// adventure manager for that cell.

// RECONSTRUCTED 2026-08-14 from the decoded map banked here on
// 2026-08-13. THE BODY IS ONE EIGHT-DIRECTION LOOP over a by-value COPY
// of `*source`; every write in it lands on that copy, and the copy is
// what the three PushPoint calls hand on. The three byte latches the
// frame carries at [ebp-1] / [ebp-2] / [ebp-3] are the three ways a
// square can be unreachable on foot - `blocked` (something stands in
// the way), `impassable` (the square itself is not walkable) and
// `needs_boat` (open water) - and each one has its own spell remedy
// further down: Fly / Dimension Door for the first, Dimension Door for
// the second, Summon Boat / Water Walk for the third.

// THE CANDIDATE POINT is built in [ebp-0x60]/[ebp-0x5e] with the
// engine-wide read-modify-write bitfield triple (`shl 6 / sar 6` to
// extract, `xor / and 0x3ff / xor word` to insert) and then bound-checked
// by type_point::is_valid, which /Ob2 expands here while still emitting
// its own body at 0x4b1330. When adjacent_monster is set the candidate is
// compared against monster_location field by field (x, then y, then the z
// nibble masked with 0x3c00) and every direction that does NOT land on
// the monster is skipped - the "you may only step onto the thing that is
// blocking you" rule, and the inverse of what the earlier map note said.

// THE THREE UNLOCATED CALLEES, all three confirmed against the bytes and
// now declared (see findpath.h / hero.h for the pairing evidence):
// searchArray::enter_hostile_trigger (0x56aad0), check_adjacent_monster
// (0x56a360) and AI_get_ship_cost (0x431160).

// THE SECOND FOG TEST TAKES ITS POINT BY VALUE, and that is worth 89.0387 ->
// 90.1238 (2026-08-20).  Retail loads `source->point` there as ONE DWORD
// (`mov eax,[edi]` plus a frame copy) and pulls x, y and z out of the
// register; three separate member reads give two 16-bit loads instead.  The
// DC roster names the spelling exactly - AdvMgr.h:1254 (dc 0x1f084) declares
// `int GetMapExtra(type_point point)` beside kb.h's three-argument reader -
// so the by-value overload is a source fact, not a device.  The FIRST fog
// test is byte-FLAT either way (90.1238 both), because `candidate` is already
// a local pathCell copy, and is left in the three-argument form the retail
// bytes show there.

// THE WATER-CROSSING PAIR READS THE MAP THROUGH `NewfullMap::cell`, AND
// THAT IS WORTH 93.0950 -> 98.7182 (2026-08-20).  Retail reloads BOTH
// `worldMap.Size` and `worldMap.cellData` for each of the two subscripts -
// `mov edx,[edi+0x1fc44]` twice and `mov ecx,[edi+0x1fc40]` twice - while
// keeping the `gpGame` load ITSELF in EDI across the whole block and on into
// the hero scan below.  Written longhand as
// `gpGame->worldMap.cellData[(z*Size + y)*Size + x]` our CL does the exact
// inverse: it hoists the two MEMBERS into edx/edi and rematerialises the
// GLOBAL at each block.  Spelling both subscripts as game.h's own
// `worldMap.cell(x, y, z)` header inline gives retail's shape, and the whole
// EBX/EDI transposition that the standing note called a register wall falls
// in behind it - the register story was downstream of the accessor.
// Same edit, same round: `GetTerrainCost` 88.0238 -> 98.6032, and
// `terrain_forbids_magic` (PushPoint's helper) +0.09.

// AND THE READ-BACK IS PART OF THE SAME DOSE.  `across_x.x = across_x.x +
// gStepDeltaX[...]` rather than a second read of `source->point.x` is worth
// 0.17 on its own and 4.75 once the accessor lands (93.9702 against
// 98.7182, measured both ways).  Retail extracts each coordinate from the
// dword it has JUST stored into the copy - `mov ebx,eax / shl ebx,6 / sar
// ebx,6` on across_x and `mov eax,[ebp-0x2e]` on across_y - where re-reading
// the member gives a 16-bit `mov bx,ax / shl bx,6` off its own word
// container.  Note the FIRST candidate at the top of the loop is the
// opposite: retail reads `source->point` there as a word (`mov dx,word ptr
// [ebx]`) and that spelling is already exact, so this is not a blanket rule.

// TWO CHEAPER SHAPE FACTS, both measured on the way (90.1238 -> 93.0950):
//   * the anchor test is `if (type == ANCHOR_POINT) { ... } else { latch }`,
//     not `if (type != ANCHOR_POINT) { latch } else if (...)`.  Retail's
//     `cmp/jne` into a latch it SHARES with the water arm's dimension-door
//     copy - a backward cross-jump - only appears with the equality form;
//     the inequality form emits a THIRD full copy of the latch.  +2.28, and
//     it is the "one extra fall-through copy" the standing note described
//     from the wrong side.
//   * `cost = 0` is the FIRST statement of the rock and dimension-door arms,
//     not the last: retail's `xor esi,esi` precedes the `add word ptr` on
//     adjusted_cost in both. +0.52.

// Residual (99.2000%): the initial Nomad predicate is scheduled differently,
// then after the second GetCell retail keeps `source` in EDI and the result in
// EBX while our compiler does the reverse. `why-reg --model` confirms both
// sides have identical first definitions (ebx@4, esi@30, edi@31), placing the
// swap beyond the B1 minimum slice.  Rejected at this plateau: swapping the
// srcGround/predicate order (98.4199%); bool, an explicit army pointer, a
// one-site inline predicate helper, and `register` on either competing value
// (all byte-flat); and 1/2/4/6 complete local type definitions (byte-flat
// across all findpath rows, with all thirteen exact functions preserved).
// DC-local follow-up: moving candidate and the three path flags to function
// scope/order is byte-flat; spelling source_terrain/terrain as TTerrainType is
// byte-flat but rejected because the required bitfield-to-enum casts violate
// the zero-cast cleanliness floor; reversing the two diagonal corner
// declarations regresses to 99.19779%; and DC's const srcCell cannot be
// expressed without changing the still-non-const NewmapCell accessors.
VA(0x004b2300, 0xA94)  // anchor-callee, dc 0x9f718
void searchArray::testPossibleDirections(hero* currentHero, pathCell* source,
                                         long turnMobility, long maxMobility,
                                         unsigned char adjacentMonster,
                                         type_point monsterLocation,
                                         long pathfinding,
                                         type_search_type searchType,
                                         long nativeTerrain)
{
    NewmapCell* srcCell = g_advManager->getCell(source->m_point);
    long srcGround = srcCell->m_groundSet;
    unsigned char hasNomad =
        currentHero->m_army.getCreatureTotal(CREATURE_NOMAD) > 0;

    for (long direction = 0; direction < 8; direction++) {
        pathCell candidate = *source;
        candidate.m_point.m_x = source->m_point.m_x + g_stepDeltaX[4 * direction];
        candidate.m_point.m_y = source->m_point.m_y + g_stepDeltaY[4 * direction];
        if (!candidate.m_point.isValid())
            continue;
        if (adjacentMonster) {
            if (candidate.m_point.m_x != monsterLocation.m_x)
                continue;
            if (candidate.m_point.m_y != monsterLocation.m_y)
                continue;
            if (candidate.m_point.m_z != monsterLocation.m_z)
                continue;
        }

        NewmapCell* destCell = g_advManager->getCell(candidate.m_point);
        long destGround = destCell->m_groundSet;
        unsigned char blocked = 0;
        unsigned char impassable = 0;
        unsigned char needsBoat = 0;
        candidate.m_canStop = 1;
        if (source->m_dimensionDoor && source->m_canStop)
            candidate.m_dimensionDoor = 0;

        long cost;
        if (destGround == eTerrainRock) {
            cost = 0;
            impassable = 1;
            candidate.m_canStop = 0;
            candidate.m_adjustedCost += 100;
        } else if (candidate.m_dimensionDoor) {
            cost = 0;
            candidate.m_adjustedCost += 100;
        } else if (source->m_inBoat) {
            cost = calcTerrainCost(srcCell, direction, turnMobility,
                                   pathfinding, 0, -1, -1, -1, hasNomad);
        } else {
            cost = calcTerrainCost(srcCell, direction, turnMobility,
                                   pathfinding, destCell->m_roadSet,
                                   candidate.m_flying ? m_flightLevel : -1,
                                   candidate.m_waterWalking ? m_waterWalkLevel
                                                           : -1,
                                   nativeTerrain, hasNomad);
        }

        if (cost <= source->m_moveLeft) {
            candidate.m_moveLeft = source->m_moveLeft - cost;
        } else {
            if (!source->m_canStop)
                continue;
            candidate.m_moveLeft = source->m_inBoat ? m_seaMovement
                                                  : m_landMovement;
            if (candidate.m_flying || candidate.m_waterWalking)
                cost = calcTerrainCost(srcCell, direction, turnMobility,
                                       pathfinding, destCell->m_roadSet,
                                       -1, -1, nativeTerrain, hasNomad);
            candidate.m_flying = 0;
            candidate.m_waterWalking = 0;
        }

        if (destCell->m_flags0011 & 0x100) {
            blocked = 1;
            candidate.m_canStop = 0;
        }

        if (!(getMapExtra(candidate.m_point.m_x, candidate.m_point.m_y,
                          candidate.m_point.m_z) & g_mapVisibilityBit)
                && searchType != const_AI_enemy_search
                && (g_currentPlayer->isHuman()
                    || (!(getMapExtra(source->m_point) & g_mapVisibilityBit)
                        && g_currentPlayer->m_numTowns > 0))) {
            blocked = 1;
            candidate.m_canStop = 0;
        }

        if (destCell->m_type == SANCTUARY && destCell->m_isTrigger
                && searchType == const_AI_enemy_search) {
            blocked = 1;
            candidate.m_canStop = 0;
        }

        if (((1 << direction) & 0x83) && srcCell->cellIsTrigger()
                && g_adventureObjectTraits[srcCell->getMapObject()][1] == 0)
            blocked = 1;
        if (((1 << direction) & 0x38) && destCell->cellIsTrigger()
                && g_adventureObjectTraits[destCell->getMapObject()][1] == 0)
            continue;

        if (destGround == eTerrainWater) {
            if (source->m_inBoat) {
                if (destCell->m_type == BOAT && destCell->m_isTrigger) {
                    impassable = 1;
                    candidate.m_canStop = 0;
                }
                if (srcGround == eTerrainWater
                        && g_stepDeltaX[4 * direction] != 0
                        && g_stepDeltaY[4 * direction] != 0) {
                    // READ-BACK, not a re-read of `source`. Retail extracts
                    // both coordinates from the dword it has just stored into
                    // the copy (`mov ebx,eax / shl ebx,6` on across_x, `mov
                    // eax,[ebp-0x2e]` on across_y), where reading
                    // `source->point.x` again gives a 16-bit `mov bx,ax /
                    // shl bx,6` off the member's own word container.
                    type_point acrossX = source->m_point;
                    type_point acrossY = source->m_point;
                    acrossX.m_x = acrossX.m_x + g_stepDeltaX[4 * direction];
                    acrossY.m_y = acrossY.m_y + g_stepDeltaY[4 * direction];
                    if (g_game->m_worldMap.cell(acrossX.m_x, acrossX.m_y,
                                              acrossX.m_z)->m_groundSet
                                != eTerrainWater
                            || g_game->m_worldMap.cell(acrossY.m_x, acrossY.m_y,
                                                     acrossY.m_z)->m_groundSet
                                != eTerrainWater)
                        impassable = 1;
                }
            } else {
                if (!(destCell->m_isTrigger
                        && (destCell->m_type == HERO || destCell->m_type == BOAT
                            || destCell->m_type == SHIPWRECK))) {
                    if (destCell->m_isTrigger)
                        blocked = 1;
                    else
                        needsBoat = 1;
                    candidate.m_canStop = 0;
                }
                if (source->m_dimensionDoor) {
                    impassable = 1;
                    candidate.m_canStop = 0;
                }
            }
        } else if (source->m_inBoat) {
            if (source->m_dimensionDoor) {
                impassable = 1;
                candidate.m_canStop = 0;
            }
            if (destCell->m_type == ANCHOR_POINT) {
                if (candidate.m_canStop
                        && searchType >= const_AI_search) {
                    if (source->m_moveLeft < cost)
                        cost = source->m_moveLeft + m_seaMovement;
                    else
                        cost = source->m_moveLeft;
                    candidate.m_moveLeft = m_landMovement;
                    candidate.m_inBoat = 0;
                    candidate.m_flying = 0;
                    candidate.m_waterWalking = 0;
                }
            } else {
                impassable = 1;
                candidate.m_canStop = 0;
            }
        }

        if (destCell->m_type == HERO && destCell->m_isTrigger) {
            hero* other = g_game->getHero(destCell->m_extraInfo);
            if (other->obscuredIsTrigger() && other->m_obscuredType == SANCTUARY
                    && other->m_owner != currentHero->m_owner) {
                blocked = 1;
                candidate.m_canStop = 0;
            }
        }

        if (destCell->m_isTrigger && searchType >= const_AI_search
                && (destCell->m_type == HERO || destCell->m_type == GARRISON
                    || destCell->m_type == MONSTER)
                && !enterHostileTrigger(currentHero, candidate)) {
            blocked = 1;
            candidate.m_canStop = 0;
        } else if (!blocked && !impassable
                   && searchType >= const_AI_search
                   && checkAdjacentMonster(currentHero, &candidate,
                                             searchType)) {
            blocked = 1;
            candidate.m_canStop = 0;
        }

        if (impassable && !candidate.m_dimensionDoor) {
            if (!m_canCastTeleport)
                continue;
            if (!source->m_canStop)
                continue;
            if (source->m_magicForbidden)
                continue;
            candidate.m_dimensionDoor = 1;
            candidate.m_adjustedCost += 500;
            // Spell 8 is Dimension Door - the id is spelled as a literal
            // for the same reason GetTerrainCost spells artifacts 0x48 and
            // 0x5a as literals: naming it means a new enumerator in
            // armygrp.h, whose include closure is measured and live.
            cost = currentHero->getSpellLevel(
                       8, currentHero->getSpecialTerrain())
                       == eMasteryExpert ? 200 : 300;
        }

        if (blocked && !candidate.m_flying && !candidate.m_dimensionDoor) {
            if (!source->m_canStop)
                continue;
            if (source->m_magicForbidden)
                continue;
            if (!m_canCastFlight && !m_canCastTeleport)
                continue;
            candidate.m_adjustedCost += 500;
            if (m_canCastTeleport) {
                candidate.m_dimensionDoor = 1;
                cost = currentHero->getSpellLevel(
                           8, currentHero->getSpecialTerrain()) == eMasteryExpert ? 200
                                                                       : 300;
            } else {
                if (source->m_inBoat)
                    continue;
                candidate.m_flying = 1;
                cost = calcTerrainCost(srcCell, direction, turnMobility,
                                       pathfinding, destCell->m_roadSet,
                                       m_flightLevel,
                                       candidate.m_waterWalking
                                           ? m_waterWalkLevel : -1,
                                       nativeTerrain, hasNomad);
                candidate.m_moveLeft = m_landMovement - cost;
            }
        }

        if (needsBoat && !candidate.m_flying && !candidate.m_waterWalking
                && !candidate.m_dimensionDoor) {
            if (!source->m_canStop)
                continue;
            if (!destCell->m_isTrigger
                    && ((m_canSummonBoat && !source->m_magicForbidden)
                        || (destCell->m_flags0011 & 0x800))) {
                pathCell boatCell = candidate;
                boatCell.m_inBoat = 1;
                boatCell.m_canStop = 1;
                long boatCost = calcTerrainCost(srcCell, direction,
                                                 turnMobility, pathfinding,
                                                 0, -1, -1, -1, hasNomad);
                boatCell.m_adjustedCost += 500;
                if (!(m_canSummonBoat && !source->m_magicForbidden))
                    boatCell.m_barrierValue +=
                        aiGetShipCost(currentHero, boatCell.m_point);
                if (m_payTransitionCosts) {
                    if (source->m_moveLeft < boatCost)
                        boatCost = source->m_moveLeft + m_landMovement;
                    else
                        boatCost = source->m_moveLeft;
                    boatCell.m_moveLeft = m_seaMovement;
                    boatCell.m_flying = 0;
                    boatCell.m_waterWalking = 0;
                }
                pushPoint(*source, boatCell, direction, boatCost,
                          maxMobility, boatCell.m_barrierValue,
                          boatCell.m_monster, 0);
            }
            if (source->m_magicForbidden)
                continue;
            if (!m_canCastTeleport
                    && !(m_canCastFlight && !source->m_inBoat)
                    && !(m_canCastWaterWalk && !source->m_inBoat))
                continue;
            if (m_canCastTeleport) {
                candidate.m_adjustedCost += 500;
                candidate.m_dimensionDoor = 1;
                cost = currentHero->getSpellLevel(
                           8, currentHero->getSpecialTerrain()) == eMasteryExpert ? 200
                                                                       : 300;
            } else if (m_flightLevel <= m_waterWalkLevel) {
                if (!currentHero->isWieldingArtifact(0x5a))
                    candidate.m_adjustedCost += 500;
                candidate.m_waterWalking = 1;
                cost = calcTerrainCost(srcCell, direction, turnMobility,
                                       pathfinding, destCell->m_roadSet,
                                       candidate.m_flying ? m_flightLevel : -1,
                                       m_waterWalkLevel, nativeTerrain,
                                       hasNomad);
                candidate.m_moveLeft = m_landMovement - cost;
            } else {
                if (!currentHero->isWieldingArtifact(0x48))
                    candidate.m_adjustedCost += 500;
                candidate.m_flying = 1;
                cost = calcTerrainCost(srcCell, direction, turnMobility,
                                       pathfinding, destCell->m_roadSet,
                                       m_flightLevel,
                                       candidate.m_waterWalking
                                           ? m_waterWalkLevel : -1,
                                       nativeTerrain, hasNomad);
                candidate.m_moveLeft = m_landMovement - cost;
            }
        }

        if (source->m_canStop || !destCell->m_isTrigger
                || (g_adventureObjectTraits[destCell->m_type][0] == 0
                    && destCell->m_type != TOWN))
            pushPoint(*source, candidate, direction, cost, maxMobility,
                      candidate.m_barrierValue, candidate.m_monster,
                      destCell->m_isTrigger);
        if ((candidate.m_flying || candidate.m_dimensionDoor)
                && destCell->m_isTrigger && candidate.m_canStop) {
            candidate.m_canStop = 0;
            pushPoint(*source, candidate, direction, cost, maxMobility,
                      candidate.m_barrierValue, candidate.m_monster, 0);
        }
    }
}

// Original: searchArray::valid_move_adjacent; findpath.cpp:877, dc 0xa02c8.
// The two ordinary predicates test reachable, non-moat hexes next to either
// half of an enemy. Preserve the source definitions without inventing a
// standalone retail address or replacing Complete's different teleport scan.
unsigned char searchArray::validMoveAdjacent(const army* currentArmy, int hex)
{
    for (long i = 0; i < 6; i++) {
        int adjacent = g_combatManager->m_adjacentCells[hex][i];
        if (combatManager::validHex(adjacent)
            && g_combatManager->m_cells[adjacent].m_validMove
            && !m_isMoatSlowed[adjacent])
            return 1;
        if (currentArmy->is(1)) {
            adjacent -= currentArmy->offsetToFront(-1);
            if (combatManager::validHex(adjacent)
                && g_combatManager->m_cells[adjacent].m_validMove
                && !m_isMoatSlowed[adjacent])
                return 1;
        }
    }
    return 0;
}

// Original: searchArray::valid_move_adjacent; findpath.cpp:905, dc 0xa0390.
unsigned char searchArray::validMoveAdjacent(const army* currentArmy,
                                            const army& enemy)
{
    if (validMoveAdjacent(currentArmy, enemy.m_gridIndex))
        return 1;
    if (enemy.is(1)
        && validMoveAdjacent(currentArmy, enemy.getSecondGridIndex()))
        return 1;
    return 0;
}


// Retail hands FindCombatPath 1000 for BOTH limit and base_speed in
// the placement phase, materialising the constant once; outside it,
// base_speed falls back to the stack's own speed when the caller
// passed a negative and the limit collapses to zero for a bound stack.

VA(0x004b2da0, 0x24B)  // anchor-global, dc 0xa03fc
void searchArray::seedCombatPosition(const army* thisArmy, long currentGroup, long limit, unsigned char inPlacementPhase, long baseSpeed)
{
    if (m_cellData == 0)
        init();
    if (inPlacementPhase) {
        baseSpeed = 1000;
        limit = 1000;
    } else {
        if (baseSpeed < 0)
            baseSpeed = thisArmy->getSpeed();
        if (thisArmy->m_spellInfluence[72])
            limit = 0;
    }
    findCombatPath(thisArmy, currentGroup, -1, inPlacementPhase, limit,
                   baseSpeed);

    for (long i = 0; i < COMBAT_GRID_CELLS; i++) {
        const pathCell* cell = m_cellData == 0 ? 0 : &m_cellData[i];
        if (cell->m_visited && static_cast<long>(cell->m_cost) <= baseSpeed
                && cell->m_flightCost == 0
                && (!inPlacementPhase
                    || !g_combatManager->isOutsidePlacementBoundry(
                            currentGroup, i))) {
            g_combatManager->m_cells[i].m_validMove = 1;
            if ((thisArmy->m_monInfo.m_attributes & 1)
                    && (static_cast<unsigned char>(static_cast<unsigned>(
                            thisArmy->m_monInfo.m_attributes) >> 6) & 1) == 0) {
                long second = i + (thisArmy->m_facing != 0 ? 1 : -1);
                if (second < 0 || second >= COMBAT_GRID_CELLS
                        || (second % COMBAT_GRID_ROW_STRIDE != 0
                            && second % COMBAT_GRID_ROW_STRIDE
                                != COMBAT_GRID_LAST_COLUMN)) {
                    if (inPlacementPhase
                            && g_combatManager->isOutsidePlacementBoundry(
                                    currentGroup, second))
                        g_combatManager->m_cells[i].m_validMove = 0;
                    else
                        g_combatManager->m_cells[second].m_frontMove = 1;
                }
            }
        }
    }

    if (thisArmy->canShoot(0) && !inPlacementPhase
            && thisArmy->m_creatureType != CREATURE_CATAPULT) {
        long other = 1 - currentGroup;
        const army* enemy = g_combatManager->m_armies[other];
        for (long j = 0; j < g_combatManager->m_numArmies[other]; j++, enemy++) {
            if ((static_cast<unsigned char>(static_cast<unsigned>(
                        enemy->m_monInfo.m_attributes) >> 21) & 1) == 0
                    && enemy->m_creatureType != CREATURE_ARROW_TOWER) {
                g_combatManager->m_cells[enemy->m_gridIndex].m_validMove = 1;
                if (enemy->m_monInfo.m_attributes & 1)
                    g_combatManager->m_cells[enemy->getSecondGridIndex()]
                            .m_validMove = 1;
            }
        }
    }
}

VA(0x004b2ff0, 0x298)  // dc 0xa0630
void searchArray::markTeleport(const army* currentArmy, long currentGroup)
{
    if (m_cellData == 0)
        init();

    for (long hex = 0; hex < COMBAT_GRID_CELLS; ++hex) {
        pathCell* cell = m_cellData == 0 ? 0 : &m_cellData[hex];
        cell->m_point.m_x = static_cast<short>(hex);
        if (!g_combatManager->inInvisibleColumn(hex)
                && currentArmy->canFit(hex, 0, 0)
                && g_combatManager->isValidTeleport(currentArmy, hex)) {
            g_combatManager->m_cells[hex].m_validMove = 1;
            cell->m_flightCost = 0;
            cell->m_cost = 1;
            cell->m_visited = 1;
        } else {
            g_combatManager->m_cells[hex].m_validMove = 0;
            cell->m_visited = 0;
        }
    }

    long otherGroup = 1 - currentGroup;
    for (long enemyGroup = 0; enemyGroup < 2; ++enemyGroup) {
        if (enemyGroup == currentGroup)
            continue;
        const army* enemy = &g_combatManager->m_armies[enemyGroup][0];
        for (long enemyIndex = 0;
                enemyIndex < g_combatManager->m_numArmies[otherGroup];
                ++enemyIndex, ++enemy) {
            if (enemy == currentArmy
                    || (static_cast<unsigned char>(static_cast<unsigned>(
                            enemy->m_monInfo.m_attributes) >> 21) & 1)
                    || enemy->m_creatureType == CREATURE_ARROW_TOWER)
                continue;

            long direction =
                (static_cast<unsigned char>(static_cast<unsigned>(
                    enemy->m_monInfo.m_attributes)) & 1) ? 8 : 6;
            while (direction-- > 0) {
                long adjacent = enemy->getAdjacentHex(enemy->m_gridIndex,
                                                        direction);
                if (g_combatManager->validHex(adjacent)) {
                    pathCell* adjacentCell =
                        m_cellData == 0 ? 0 : &m_cellData[adjacent];
                    if (adjacentCell->m_visited)
                        break;
                }
            }
            if (direction < 0)
                continue;

            markEnemy(enemy->m_gridIndex, 1);

            if (static_cast<unsigned char>(static_cast<unsigned>(
                    enemy->m_monInfo.m_attributes)) & 1)
                markEnemy(enemy->m_gridIndex, 1);
        }
    }
}

// The two eleven-entry hex tables the body stamps. Both are read
// element by element in one 0..10 loop and then indexed by the literal
// 5 (the gate hex) after it, which is why retail addresses
// gMoatHexes[5] and gInnerMoatHexes[5] as folded absolute slots.
// The rows are the two concentric arcs of a Fortress-style moat: the
// second table is the first shifted one hex left on every row.
// Names are provisional - no roster reaches either array.
DATA(0x0063bce8) const unsigned char g_moatHexes[11] = {
    11, 28, 44, 61, 77, 95, 111, 129, 146, 164, 181
};
DATA(0x0063bcf4) const unsigned char g_innerMoatHexes[11] = {
    10, 27, 43, 60, 76, 94, 110, 128, 145, 163, 180
};

VA(0x004b3290, 0x16F)  // dc 0xa0804
void searchArray::setMoat(const army* currentArmy)
{
    memset(m_isMoatSlowed, 0, 187);
    unsigned char flying = static_cast<unsigned char>(static_cast<unsigned>(currentArmy->m_monInfo.m_attributes) >> 1);
    if (flying & 1)
        return;
    if (currentArmy->m_creatureType == CREATURE_ARCH_DEVIL)
        return;
    if (currentArmy->m_creatureType == CREATURE_DEVIL)
        return;
    if (g_combatManager->m_moatOn) {
        { for (int hex = 0; hex < 11; ++hex) {
            m_isMoatSlowed[g_moatHexes[hex]] = 1;
            if (g_combatManager->m_moatIsWide)
                m_isMoatSlowed[g_innerMoatHexes[hex]] = 1;
        } }
        if (g_combatManager->m_drawbridgeState != DRAWBRIDGE_UP
                || (currentArmy->m_spellInfluence[60] ? 1 - currentArmy->m_combatSide
                                                : currentArmy->m_combatSide) == 1) {
            m_isMoatSlowed[g_moatHexes[5]] = 0;
            if (g_combatManager->m_moatIsWide)
                m_isMoatSlowed[g_innerMoatHexes[5]] = 0;
        }
    }
    { for (int cell = 0; cell < 187; ++cell) {
        if (g_combatManager->m_cells[cell].m_attributes & 4) {
            const combatManager::TObstacle* obstacle =
                &g_combatManager->m_obstacles[g_combatManager->m_cells[cell].m_obstacleIndex];
            if (currentArmy->m_combatSide == obstacle->m_owner || obstacle->m_isVisible)
                m_isMoatSlowed[cell] = 1;
        }
    } }
    m_isMoatSlowed[currentArmy->m_gridIndex] = 0;
    if (currentArmy->m_monInfo.m_attributes & 1)
        m_isMoatSlowed[currentArmy->getSecondGridIndex()] = 0;
}

// E:\gamedcs\findpath.cpp:1136, dc 0xa0970
bool searchArray::buildCombatPath(const army* currentArmy,
                                 int startHex, int endHex, int destination)
{
    if (!combatManager::validHex(destination))
        return 0;
    if (currentArmy->m_side == -1) {
        if (endHex != destination)
            return 0;
    } else if (m_result.size() == 0) {
        return 0;
    }

    while (endHex != startHex) {
        pathCell* stepCell = getHex(endHex);
        m_result.push_back(stepCell);
        endHex = currentArmy->getAdjacentCellIndex(
            endHex, oppositeDirection(stepCell->m_direction));
    }
    return m_result.size() > 0;
}

// E:\gamedcs\findpath.cpp:1172
void searchArray::markEnemy(long hex, long cost)
{
    hexcell* combatCell = &g_combatManager->m_cells[hex];
    pathCell* cell = getHex(hex);
    if (combatCell->m_validMove) {
        if (cell->m_cost <= cost)
            return;
    }
    combatCell->m_validMove = 1;
    cell->m_cost = static_cast<unsigned short>(cost);
}

// E:\gamedcs\findpath.cpp:1187
// Both of FindCombatPath's expansions are here; the DC body's own range
// check is not, because retail's first call site has it hoisted into the
// direction loop and its second spells it at the site. It answers "the
// hex this enemy stands on IS the destination", which is why retail's
// caller consumes the result with `sete`/`test`/`jne` rather than with a
// plain compare.
bool searchArray::checkEnemyArmies(long hex, long cost,
                                  long currentGroup, long destination)
{
    if (!combatManager::validHex(hex))
        return 0;
    const army* enemy = g_combatManager->m_cells[hex].getArmy();
    if (enemy == 0)
        return 0;
    if (enemy->getOwningSide() == currentGroup)
        return 0;

    markEnemy(enemy->m_gridIndex, cost);
    if (enemy->is(1))
        markEnemy(enemy->getSecondGridIndex(), cost);
    return hex == destination;
}

// E:\gamedcs\findpath.cpp:1218

// THE SIEGE-PRESSURE PREAMBLE is the only part of this body that is not
// a plain Dijkstra. It fires only while a town is defending AND the
// acting stack is computer-driven (is_computer_action, landed in
// command.obj), and it decides whether a stack that CANNOT FIT on a hex,
// or that is moat-slowed there, may still press the attack. Both
// hit-point comparisons re-read the whole
// gpCombatManager->defendingTown->type chain and re-call
// get_total_hit_points rather than caching either - transcribed
// faithfully. The second comparison's `siege_pressure &&` guard is
// invisible in the bytes on the path where the first comparison has just
// set the flag, which is exactly the retail branch layout.

// Whole-TU controls (2026-09-09), see the helper/accessor family generators:
// removing the three budget-only helpers and the duplicate mark helper
// while restoring the real private declarations initially gave 60.0911%
// (nested mark guard). That is incomplete source recovery, not contrary
// evidence. Restoring Is/get_owning_side, OffsetToFront/get_spell_time,
// ValidHex at its actual helper boundary, and const get_hex brings
// FindCombatPath from the old 87.9780% to 90.2669%; every other tracked
// findpath function is score-flat, including mark_teleport at 100%.
// The compound mark guard instead of the DC nested return gives 88.6656%.
// Omitting the geometry accessors gives 84.7692%; omitting ValidHex's
// recovered boundary gives 74.6845%. Keep the positive helper/accessor
// evidence through those isolated score dips.

// Retail calls the 0x4b3b90 cell accessor at +0x42f, +0x481, +0x53e, +0x590,
// within check_enemy_armies' four mark expansions, not at the found block
// or build_combat_path's tail walk. It calls vector<pathCell*>::insert at
// +0x672 and +0x734; source calls push_back, whose retained/expanded child
// decision must be recovered without spelling insert in its caller.
// Earlier artificial preamble extractions and a pinned second mark body
// reached 87.9780%; they do not establish original source boundaries.

// The current direction is the search result: both check_enemy_armies
// successes break the six-direction scan, and direction < 6 admits the
// shared reached-cell block. Under the restored canonical helpers this
// removes three gotos and improves 90.2669% to 92.2920%. Direction != 6
// reaches 92.2779%; a separate bool/byte/int found flag stays at 90.2669%.
// is_moat(short) preserves the retail 16-bit neighbour arithmetic.
// The six differently named call references are folded template aliases:
// pointer copy (37 B), both empty vector destructors (3 B), and pointer
// insertion (521 B) match the retail int/type_artifact/widget-labelled
// bodies byte-for-byte after relocation, including their callee references.

// Candidate /Z7 labels are candidate-only, and aggregate call counts or
// unclaimed synthetic labels do not prove a missing source statement.

VA(0x004b3400, 0x787)  // anchor-global, dc 0xa0b18
unsigned char searchArray::findCombatPath(const army* currentArmy,
                                          long currentGroup, long destination,
                                          unsigned char inPlacementPhase,
                                          long limit, long baseSpeed)
{
    if (currentArmy == 0)
        return 0;

    unsigned char siegePressure = 0;
    if (g_combatManager->m_defendingTown != 0
            && g_combatManager->isComputerAction(currentArmy)) {
        if (currentGroup == 1
                || g_combatManager->m_drawbridgeState != DRAWBRIDGE_UP)
            siegePressure = 1;
        if (currentArmy->getTotalHitPoints(0)
                <= g_townSiegeStrength63bd18[
                        g_combatManager->m_defendingTown->m_type] * 4)
            siegePressure = 1;
        if (siegePressure
                && currentArmy->getTotalHitPoints(0)
                    > g_townSiegeStrength63bd18[
                            g_combatManager->m_defendingTown->m_type] * 40)
            siegePressure = 0;
    }

    for (long clearHex = 0; clearHex < COMBAT_GRID_CELLS; clearHex++) {
        g_combatManager->m_cells[clearHex].m_validMove = 0;
        g_combatManager->m_cells[clearHex].m_frontMove = 0;
    }

    if (inPlacementPhase) {
        baseSpeed = limit = 1000;
    } else {
        if (baseSpeed < 0)
            baseSpeed = currentArmy->getSpeed();
        if (baseSpeed == 0 || currentArmy->getSpellTime(72))
            limit = 0;
    }

    long startHex = currentArmy->m_gridIndex;
    if (m_cellData == 0)
        init();
    setMoat(currentArmy);

    long bestDistance = 800;
    long bestHex = -1;
    // Dreamcast CodeView names function-scope `pathCell pc` and emits its
    // empty constructor before both vector clears. Restoring that lifetime
    // is byte-flat at 87.9780 but preserves the positive source evidence.
    pathCell pc;
    m_result.clear();
    // The BFS queue NAMED AS A REFERENCE: 87.6468 -> 87.9780.
    std::vector<pathCell>& rQueue = m_queue;
    rQueue.clear();
    memset(m_cellData, 0, COMBAT_GRID_CELLS * sizeof(pathCell));

    pushCombatPoint(startHex, currentArmy->m_facing ? 1 : 4, 0, 0, limit);

    while (rQueue.size() > 0) {
        pc = rQueue.back();
        rQueue.pop_back();

        long cost = pc.m_cost;
        if (cost > limit)
            continue;

        if (combatManager::validHex(destination)
                && pc.m_flightCost == 0) {
            long distance = combatManager::getDistance(pc.m_point.m_x, destination);
            if (distance < bestDistance) {
                bestHex = pc.m_point.m_x;
                bestDistance = distance;
                if (distance == 0)
                    break;
            }
        }

        long hex = pc.m_point.m_x;
        long adjacent;
        long direction;
        for (direction = 0; direction < 6; direction++) {
            adjacent = currentArmy->getAdjacentCellIndex(hex, direction);
            if (!combatManager::validHex(adjacent))
                continue;

            int flightCost = 0;
            unsigned char moat = 0;
            if (!(currentArmy->is(1))) {
                moat = isMoat(adjacent);
            } else {
                long sideStep = currentArmy->offsetToFront(-1);
                long tail = adjacent + sideStep;
                if (isMoat(adjacent) && adjacent != hex + sideStep)
                    moat = 1;
                if (isMoat(tail) && tail != hex)
                    moat = 1;
            }

            long step = 1;
            if (moat && baseSpeed > 0)
                step = baseSpeed - cost % baseSpeed;

            if (!currentArmy->canFit(adjacent, 0, 0)
                    || (siegePressure && moat)) {
                long enemyCost = cost;
                if (isMoat(hex))
                    enemyCost += baseSpeed;
                unsigned char blocked = 0;
                if (limit <= baseSpeed) {
                    if (isMoat(hex))
                        blocked = 1;
                    if ((currentArmy->is(1))
                            && isMoat(static_cast<short>(
                                    pc.m_point.m_x
                                    + (currentArmy->offsetToFront(-1)))))
                        blocked = 1;
                }
                if (enemyCost <= limit && !blocked) {
                    if (checkEnemyArmies(adjacent, enemyCost, currentGroup,
                                           destination))
                        break;
                    if (currentArmy->is(1)) {
                        long tail = adjacent
                            + (currentArmy->offsetToFront(-1));
                        if (checkEnemyArmies(tail, enemyCost,
                                             currentGroup, destination))
                            break;
                    }
                }
                if (!(currentArmy->is(2)
                        || currentArmy->m_creatureType == CREATURE_DEVIL
                        || currentArmy->m_creatureType == CREATURE_ARCH_DEVIL))
                    continue;
                flightCost = pc.m_flightCost + 1;
                if (flightCost >= currentArmy->getSpeed())
                    continue;
            }
            pushCombatPoint(adjacent, direction, cost + step, flightCost,
                            limit);
        }
        if (direction < 6) {
            pathCell* reached = getHex(adjacent);
            reached->m_point.m_x = static_cast<short>(adjacent);
            reached->m_direction = direction;
            reached->m_lastPoint = pc.m_point;
            m_result.push_back(reached);
        }

        if (m_result.size() > 0) {
            bestHex = m_result[0]->m_lastPoint.m_x;
            break;
        }
    }

    return buildCombatPath(currentArmy, startHex, bestHex,
                             destination);
}

VA(0x004b3bb0, 0x35C)  // dc 0xa0f54
void searchArray::pushCombatPoint(int index, int direction, int cost, int flightCost, int limit)
{
    if (!combatManager::validHex(index))
        return;
    if (cost > limit)
        return;

    pathCell* cell = getHex(index);
    if (cell->m_visited && cell->m_cost <= cost)
        return;
    if (m_queue.size() >= 500)
        return;

    int last = m_queue.size();
    int first = 0;
    int middle;
    for (;;) {
        middle = (first + last) / 2;
        if (last <= first)
            break;
        if (cost < m_queue[middle].m_cost)
            first = middle + 1;
        else
            last = middle;
    }

    pathCell currentPathCell;
    currentPathCell.m_point.m_x = index;
    currentPathCell.m_visited = 1;
    currentPathCell.m_point.m_y = 0;
    currentPathCell.m_direction = direction;
    currentPathCell.m_cost = cost;
    currentPathCell.m_flightCost = flightCost;

    if (middle < m_queue.size()) {
        m_queue.insert(m_queue.begin() + middle, currentPathCell);
    } else {
        m_queue.push_back(currentPathCell);
    }
    *cell = currentPathCell;
}

VA(0x004b3f10, 0xF)  // dc 0xa10b0
void searchArray::lowerDoor()
{
    m_isMoatSlowed[0x5f] = 0;
    m_isMoatSlowed[0x5e] = 0;
}

VA(0x004b3f20, 0x41)  // dc 0xa10c4
long searchArray::getTravelTime(const army* currentArmy, long hex) const
{
    pathCell* cell = m_cellData == 0 ? 0 : &m_cellData[hex];
    long speed = currentArmy->getSpeed();
    long turns = (cell->m_cost + speed - 1) / speed;

    if (turns < 1)
        turns = 1;
    return turns;
}

#if 0  // @carcass

// E:\gamedcs\findpath.cpp:79
DC_ONLY(0xa115c, 0x50)
void pathCell::pathCell()
{
    // @stub
}

#endif  // @carcass

VA_COMPGEN(0x004b3f70, 0x2F3, VECTOR_INSERT, pathCell)

VA_COMPGEN(0x004b4270, 0x35, STD_COPY, pathCell)

VA_COMPGEN(0x004b42b0, 0x16, STD_CONSTRUCT, pathCell)

VA_COMPGEN(0x00434c30, 0x33, VECTOR_UFILL, pathCell)
