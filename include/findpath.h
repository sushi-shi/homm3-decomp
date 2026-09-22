#ifndef HOMM3_FINDPATH_H
#define HOMM3_FINDPATH_H

#include "va.h"

#include <vector>
#include <windows.h>
#include <windows.h>

#include "struct.h"

class army;
class hero;
class NewmapCell;

// Dreamcast CodeView supplies the complete domain and names; SeedTo's retail
// call proves const_normal_search == 0 on x86.
enum type_search_type {
    const_normal_search = 0,
    const_AI_treasure_search,
    const_AI_allied_search,
    const_AI_enemy_search,
    const_AI_search,
    const_AI_alternate_search
};

// The path grid record. Field names and offsets are the Dreamcast
// fieldlist, with the PC-only flag described below; the STRIDE is retail-proven at THIRTY bytes by
// searchArray::get_travel_time (0x4b3f20 indexes cellData with
// `lea eax,[eax+eax*2]; lea eax,[eax+eax*4]; lea esi,[ecx+eax*2]`),
// which the default packing cannot produce - the two longs at 16/20
// would round sizeof up to 32. Hence pack(1), the same tool hero.h
// and mapcell.h already use.

// Complete adds a starting-trigger flag at bit 9, shifting canStop and
// lastCanStop to bits 10 and 11, direction to 12..15, and flightCost to 26..31.
// Path reconstruction uses lastCanStop to select the flying or ground predecessor.
// The trigger flag lets AI movement process an event at the starting tile.
#pragma pack(push, 1)
struct pathCell {
public:
    type_point m_point;
    unsigned int m_visited : 1;
    unsigned int m_isTrigger : 1;
    unsigned int m_inBoat : 1;
    unsigned int m_magicForbidden : 1;
    unsigned int m_flying : 1;
    unsigned int m_waterWalking : 1;
    unsigned int m_townPortal : 1;
    unsigned int m_dimensionDoor : 1;
    unsigned int m_castleGate : 1;
    // PC-only starting-trigger flag.
    unsigned int m_startAtTrigger : 1;
    unsigned int m_canStop : 1;
    unsigned int m_lastCanStop : 1;
    unsigned int m_direction : 4;
    int m_deltaX : 5;
    int m_deltaY : 5;
    unsigned int m_flightCost : 6;
    type_point m_lastPoint;
    type_point m_monster;
    long m_barrierValue;
    long m_dangerValue;
    unsigned short m_cost;
    unsigned short m_adjustedCost;
    unsigned short m_moveLeft;

    // CodeView dc 0xa115c explicitly marks the default constructor generated
    // (compgenx). Its type_point members already make construction nontrivial;
    // searchArray::Init's array preamble does not prove a user-written body.
};
#pragma pack(pop)
SIZE(pathCell, 30);

// Retail .data 0x6783c8 / 0x6783cc - the world's x- and y-extents. Every
// cell index in the engine is ((z * MAP_HEIGHT + y) * MAP_WIDTH + x);
// get_danger_cell below is the smallest reader, game::get_cell the
// nearest relative (it reads the map record's own square Size instead).
// DECLARATIONS ONLY - game.h owns the DATA claims on the pair, and a second
// claim on one RVA is a fatal duplicate at delink time. The names are the
// Dreamcast roster's (`?MAP_WIDTH@@3HA` / `?MAP_HEIGHT@@3HA`).
extern int g_mapWidth;
extern int g_mapHeight;

// Dreamcast roster with the STLport->VC6 vector shift; the retail ctor
// 0x4b1370 stores every named field and the dtor 0x4b13e0 frees
// cellData and bIsMoatSlowed (a heap map, not a flag) before the
// implicit vector teardowns. valid_rectangle (0x28..0x37) stays
// uninitialized by the ctor - Init fills it.
class searchArray {
private:
    void boardBoat(const hero* currentHero, pathCell& cell);
    unsigned char validMoveAdjacent(const army* currentArmy, int hex);
    unsigned char validMoveAdjacent(const army* currentArmy, const army& enemy);
    int m_maxQueueCount;
    unsigned char m_payTransitionCosts;
    int m_thisTurnsMovement;
    int m_landMovement;
    int m_seaMovement;
    unsigned char m_canSummonBoat;
    unsigned char m_canCastTeleport;
    unsigned char m_canCastFlight;
    unsigned char m_canCastWaterWalk;
    int m_waterWalkLevel;
    int m_flightLevel;
    unsigned char m_limitReached;
    pathCell* m_cellData;

public:
    // Dreamcast fieldlist: valid_rectangle at +0x28. Init fills the bounds;
    // the constructor leaves them uninitialized. Preserve the aggregate so
    // setRectangle keeps its original one-statement assignment.
    tagRECT m_validRectangle;
    searchArray();
    ~searchArray();
    void close();
    // Retail 0x4b3b90 checks receiver+0x24 for null, then indexes the
    // 30-byte pathCell array and returns ret 4. FindCombatPath calls at
    // 0x4b382f/0x4b3881/0x4b393e/0x4b3990 correspond to the four
    // expansions of DC mark_enemy's get_hex call.
    // E:\gamedcs\FindPath.h:194, dc 0x27fe8
    VA(0x004b3b90, 0x20)  // caller/get_hex correlation, dc 0x27fe8
    pathCell* getHex(long x) const
    {
        if (m_cellData == 0)
            return 0;
        return &m_cellData[x];
    }
    VA(0x0042ecc0, 0x62)  // hd-crossbuild + exact body/callers x2, dc 0x20064
    pathCell* getCell(type_point point, bool flying) const
    {
        if (!m_cellData)
            return m_cellData;
        return &m_cellData[((point.m_z * 2 + flying) * g_mapHeight + point.m_y)
                         * g_mapWidth + point.m_x];
    }
    long getDangerValue(type_point point) const;  // 0x42ed30 (ai_player.obj)
    void seedPosition(hero* currentHero, type_point start,
                      type_point target, int maxMobility,
                      unsigned char isBoat,
                      type_search_type searchType,
                      int curTempMobility,
                      unsigned char seedContinuation);
    int buildPath(const hero* currentHero, long limit);

    // E:\gamedcs\FindPath.h:211-213, dc 0x200e8: vector::clear.
    void clearPath()
    {
        m_result.clear();
    }
    // Dreamcast FindPath.h:216/226. Both const header helpers retain public
    // SH4 copies, while Complete expands build_path's calls into the result
    // vector's size and indexed pointer load.
    long getPathSteps() const
    {
        return m_result.size();
    }
    // DC FindPath.h:221-223, get_step: the route direction is a byte result
    // read from the indexed pathCell, distinct from get_step_cell's pointer.
    unsigned char getStep(long i) const
    {
        return m_result[i]->m_direction;
    }
    const pathCell* getStepCell(long i) const
    {
        return m_result[i];
    }
    // Dreamcast FindPath.h:231/236/257.  These source helpers are all
    // folded into ai_player.obj's destination chooser on retail x86.  Keep
    // the boundaries visible in C++ even where the selected lowering is a
    // vector::size call or direct field/index arithmetic.
    long getVisitedCount() const { return m_visitedPoints.size(); }
    pathCell* getVisitedCell(long index) { return m_visitedPoints[index]; }
    long getTravelTime(const army* currentArmy, long hex) const;
    // findpath.h:242 in the DC roster (ai.obj carries the only 10-byte
    // out-of-line copy). The PARAMETER IS A SHORT, and that is what the
    // retail bodies prove: move_toward (0x41f580) and FindCombatPath
    // (0x4b3400) both index bIsMoatSlowed through a `movsx` from a
    // 16-bit value, and FindCombatPath even does the neighbour's
    // `+/- 1` in 16-bit arithmetic (`mov di, word [..]; sar di, 6;
    // add; movsx ecx, cx`) - which only a short parameter forces. The
    // one call whose argument is already a sign-extended 16-bit value
    // loses the movsx, exactly as it should.
    unsigned char isMoat(short hex) const { return m_isMoatSlowed[hex]; }
    // E:\\gamedcs\\findpath.h:247 (dc 0x37e7c). Retail folds this
    // const tiny helper into move_hero and AI_choose_destination as the
    // byte read at +0x20.
    bool limitWasReached() const { return m_limitReached != 0; }
    // 0x4b3f10. Clears the two drawbridge hexes in the moat map.
    void lowerDoor();
    unsigned char findCombatPath(const army* currentArmy, long currentGroup,
                                 long destination, unsigned char inPlacementPhase,
                                 long limit, long baseSpeed);  // 0x4b3400
    // 0x4b2ff0. Rebuilds the teleport-reachable combat cells, then keeps
    // enemy occupied cells marked when they border that reachable set.
    void markTeleport(const army* currentArmy, long currentGroup);
    void seedCombatPosition(const army* thisArmy, long currentGroup,
                            long limit, unsigned char inPlacementPhase,
                            long baseSpeed);
    // Dreamcast FindPath.h:252. MoveHero brackets its move_hero call with
    // this setter; Complete expands both calls to the +0x6c store.
    void setDangerZones(long* dangerZoneMap)
    {
        m_dangerZones = dangerZoneMap;
    }
    // E:\gamedcs\FindPath.h:257, dc 0x37e84
    void setRectangle(tagRECT& rect)
    {
        m_validRectangle = rect;
    }

private:
    bool buildCombatPath(const army* currentArmy, int startHex,
                         int endHex, int destination);
    // DC findpath.cpp:1187. ValidHex belongs to the ordinary helper;
    // retail eliminates it at the first, already-checked caller site.
    bool checkEnemyArmies(long hex, long cost, long currentGroup,
                          long destination);
    void checkTownPortal(const hero* currentHero,
                           const pathCell* startCell, long maxMobility);
    // 0x4b1530. Empties the three vectors, then zeroes the cellData rows
    // inside the valid rectangle for every (z, fly-plane) combination.
    void clear(long flyLevel, long startZ, long stopZ);
    void enterGate(const pathCell* cell, const NewmapCell* mapCell,
                    long limit);
    unsigned char enterHostileTrigger(const hero* currentHero,
                                     pathCell& cell);
    // search.obj 0x56a400 / 0x56a730, the lith-family and underground
    // gate seeders; both parameter lists are the DC roster's
    // (search.cpp:155 and :244).
    void enterLith(const hero* currentHero,
                    const std::vector<type_point>* list, long cellType,
                    long excluded, pathCell* entryPoint, long limit,
                    type_search_type searchType);
    void enterTown(const hero* currentHero, long startTown,
                    const pathCell* currentPathCell, long limit,
                    type_search_type searchType);
    unsigned char enterTrigger(const hero* currentHero, pathCell* cell,
                                long limit, type_search_type searchType);
    // 0x4b1460 / 0x4b1500. Init frees whatever Close would have freed
    // and then re-allocates both maps; SeedCombatPosition calls it
    // whenever cellData is still null.
    void init();
    void markEnemy(long hex, long cost);
    void pushCombatPoint(int index, int direction, int cost,
                         int flightCost, int limit);  // 0x4b3bb0
    // DC findpath.cpp:271 proves both pathCell reference parameters.
    void pushPoint(const pathCell& oldCell, pathCell& point, int direction,
                   int moveCost, int limit, long barrierValue,
                   type_point monster, int isTrigger);
    // 0x4b3290. Rebuilds bIsMoatSlowed for one acting stack.
    void setMoat(const army* currentArmy);
    // DC's first parameter here is const hero*. The current hero member
    // declarations require a mutable pointer; retail cannot distinguish it.
    void testPossibleDirections(hero* currentHero, pathCell* source,
                                long turnMobility, long maxMobility,
                                unsigned char adjacentMonster,
                                type_point monsterLocation, long pathfinding,
                                type_search_type searchType,
                                long nativeTerrain);
    // Elements are pathCells BY VALUE: FindCombatPath (0x4b3400) pops
    // the back with `mov esi,[queue+8]; add esi,-0x1e; mov [queue+8],esi`
    // - a 30-byte stride on _Last, which only a by-value pathCell gives.
    std::vector<pathCell> m_queue;
    // ELEMENT TYPE PROVEN, 2026-08-08. move_toward (0x41f580) walks
    // this vector's extent with `sar 2` (4-byte elements) and then
    // dereferences `[_First + 4*i]` at +4 to read a pathCell bitfield -
    // so the elements are pathCell POINTERS, not the admitted int
    // placeholder. FindCombatPath (0x4b3400) corroborates: it loads
    // `[result._First]` and immediately reads `word [that + 8]`,
    // pathCell::last_point.
    std::vector<pathCell*> m_result;
    // ELEMENT TYPE PROVEN, 2026-08-14, by PushPoint (0x4b1a70) - the one
    // located body that touches it. Its tail inserts into this vector with
    // the four-byte stride (`sub ecx,eax; sar ecx,2` for the capacity test,
    // `add ebx,4` / `add edx,4` in the copy loops) and the value it hands
    // the insert is `lea edx,[ebp+0x14]`, the frame slot holding the
    // get_cell() result - i.e. a pathCell*. The DC fieldlist prints all
    // three of these as std::vector<pathCell...> and the dump truncates the
    // argument list at the comma, which is why the admitted placeholder was
    // `int`; queue is pinned by its 30-byte stride, result and this one by
    // their 4-byte strides plus what retail stores through them.
    std::vector<pathCell*> m_visitedPoints;
    // One byte per combat hex, indexed by a SIGN-EXTENDED hex
    // (`movsx edx, si; cmp byte [edx + eax], 0` in move_toward
    // 0x41f580) - a map, as the dtor's `delete` already implied.
    unsigned char* m_isMoatSlowed;
    // +0x6c. `long*` (not void*) from get_danger_value's `[ecx + edx*4]`
    // load - the danger map is one signed word per cell.
    long* m_dangerZones;
};

// Original: get_danger_cell; E:\gamedcs\FindPath.h:265, dc 0x37e98.
// CodeView proves a long& result referring to the existing map element.
inline long& getDangerCell(long* dangerZones, type_point point)
{
    return dangerZones[(point.m_z * g_mapHeight + point.m_y) * g_mapWidth + point.m_x];
}

// E:\gamedcs\FindPath.h:270, dc 0x37eec
VA(0x0042ed30, 0x4E)  // dc 0x37eec
inline long searchArray::getDangerValue(type_point point) const
{
    if (!m_dangerZones)
        return 0;
    return getDangerCell(m_dangerZones, point);
}

// Retail .rdata 0x63bd18, nine dwords indexed by town::type:
// { 70, 70, 150, 90, 70, 90, 70, 90, 70 }. FindCombatPath (0x4b3400)
// is the only located reader and it uses the row twice - once at x4 as
// the "this stack is weak enough to be pushed into the moat" floor and
// once at x40 as the ceiling above which the pressure is dropped again.
// Left as an UNCLAIMED extern: the row sits between findpath's own moat
// tables (0x63bce8) and combatManager::wallTargets (0x63be60), so the owning
// TU is not settled, and the tenth dword reads 0 - it may or may not be
// part of the array. Name is an address ordinal.
extern const int g_moatDamage[];

// Retail .bss 0x699284; the DATA claim lands with findpath.cpp's
// globals when that TU's data is modeled.
extern searchArray* g_searchArray;

// The eight-direction step table at 0x678150, four bytes a row:
// (dx, dy, 0x10, 0) for N, NE, E, SE, S, SW, W, NW in that order.
// Dreamcast publishes the source name/type as `tilePoint* normalDirTable`;
// retail names the same base and reads its x/y fields at +0/+1. The two
// legacy stride-four aliases are now expressed as fields of this aggregate.
struct tilePoint {
public:
    signed char m_x;
    signed char m_y;
    short m_frameOffset;
};
SIZE(tilePoint, 4);

enum EMapDirection {
    MAP_DIRECTION_NORTH = 0,
    MAP_DIRECTION_NORTHEAST = 1,
    MAP_DIRECTION_EAST = 2,
    MAP_DIRECTION_SOUTHEAST = 3,
    MAP_DIRECTION_SOUTH = 4,
    MAP_DIRECTION_SOUTHWEST = 5,
    MAP_DIRECTION_WEST = 6,
    MAP_DIRECTION_NORTHWEST = 7,
    MAP_DIRECTION_COUNT = 8
};

extern tilePoint g_normalDirTable[8];

// 0x56a360, search.obj's, still @stub there. Declared here because
// TestPossibleDirections calls it as a free fastcall (hero* in ECX,
// pathCell* in EDX, the search type on the stack); the pairing is the DC
// roster's search.cpp:113 row - the free three-argument predicate that
// immediately follows BuildPath in both link orders, 182 DC bytes against
// retail's 158.
unsigned char checkAdjacentMonster(const hero* currentHero,
                                     pathCell* entryPoint,
                                     type_search_type searchType);
int minimumTerrainCost(const NewmapCell* cell, int pointsLeft,
                       long pathfinding, long flying, long waterWalking,
                       unsigned char hasNomad);
int getTerrainCost(hero* currentHero, type_point start, int direction,
                   int moveLeft);

#endif  /* HOMM3_FINDPATH_H */
