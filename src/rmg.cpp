// rmg.cpp - Complete-only random-map generator support.
// Evidence lookup spellings retained by the normalized river reconstruction:
// CreateRiver, ResetMovementCosts, InsertRmgWorkItem, IsRiverTarget,
// IsImpassable, SetMovementCost, ResetMovement; globals gRmgDirections,
// gRmgShipyardWaterOffsets, gLandRiverDeltaIndex and gSnowRiverDeltaIndex.
//
// The Dreamcast build has no RMG compiland. Retail's direct caller graph
// reaches this library from TSingleSelectionWindow::GenerateRandomMap, and
// the tree node layout proves an eight-byte TPoint value ordered by y, then x.
#include <va.h>
#include <algorithm>
#include <bitset>
#include <ctype.h>
#include <math.h>
#include <list>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <string>
#include "abstractfile.h"
#include "advmgr_objects.h"
#include "artifact.h"
#include "armygrp.h"
#include "bitset_iterator.h"
#include "homm3_minmax.h"
#include "hero.h"
#include "objnames.h"
#include "resourcemanager.h"
#include "rmg_request.h"
#include "rmg.h"
#include "savegame.h"
#include "rmg_terrain.h"
#include "textresource.h"
#include "town.h"

typedef std::set<TPoint> TRmgPointSet;

// Retail constructor defaults and overrides; 0x546257 compares map counts,
// while 0x546270 compares per-zone counts. Names are role-derived.
DATA(0x0069CE4C)
int g_rmgMapObjectLimits[232];
DATA(0x0069D1F4)
int g_rmgZoneObjectLimits[232];
DATA(0x00640718)
static const TRmgObjectLimit g_rmgMapObjectLimitOverrides[30] = {
    { 26, 200 },
    { 6, 200 },
    { 57, 48 },
    { 8, 64 },
    { 100, 32 },
    { 23, 32 },
    { 32, 32 },
    { 51, 32 },
    { 61, 32 },
    { 102, 32 },
    { 41, 32 },
    { 4, 32 },
    { 47, 32 },
    { 107, 32 },
    { 104, 32 },
    { 113, 32 },
    { 88, 32 },
    { 89, 32 },
    { 90, 32 },
    { 92, 32 },
    { 55, 32 },
    { 109, 32 },
    { 112, 32 },
    { 48, 32 },
    { 22, 32 },
    { 39, 32 },
    { 108, 32 },
    { 105, 32 },
    { 83, 48 },
    { 7, 32 },
};
DATA(0x00640808)
static const TRmgObjectLimit g_rmgZoneObjectLimitOverrides[24] = {
    { 2, 1 },
    { 13, 1 },
    { 14, 1 },
    { 15, 1 },
    { 27, 1 },
    { 28, 1 },
    { 30, 1 },
    { 31, 1 },
    { 35, 1 },
    { 38, 1 },
    { 42, 1 },
    { 48, 1 },
    { 49, 1 },
    { 56, 1 },
    { 58, 1 },
    { 60, 1 },
    { 64, 1 },
    { 80, 1 },
    { 94, 1 },
    { 96, 1 },
    { 99, 1 },
    { 106, 1 },
    { 110, 1 },
    { 113, 3 },
};


// Complete-only pattern globals: retail cinit 0x55ed70/0x55f2f0 passes the
// array and count to the shared support constructor, then registers cleanup
// at 0x55ed90/0x55f310. Both cleanups retain the support destructor. Defining
// these beside that destructor instead expands delete into the callbacks;
// keep the ordinary library boundary, with no inline controls or new helpers.
DATA(0x00641140)
static const int g_rmgRiverPatterns[13] = {
    4, 4, 4, 4, 8, 7, 7, 6, 6, 2, 2, 3, 3
};
DATA(0x0069E5D0)
TRmgLinePatternTable g_rmgRiverPatternTable(13, g_rmgRiverPatterns);
VA_COMPGEN(0x0055ED70, 0x1D, STATIC_CTOR, g_rmgRiverPatternTable)
VA_COMPGEN(0x0055ED90, 0x0A, STATIC_DTOR, g_rmgRiverPatternTable)

DATA(0x006411AC)
static const int g_rmgRoadPatterns[17] = {
    4, 4, 5, 5, 5, 5, 6, 6, 7, 7, 2, 2, 3, 3, 0, 1, 8
};
DATA(0x0069E650)
TRmgLinePatternTable g_rmgRoadPatternTable(17, g_rmgRoadPatterns);
VA_COMPGEN(0x0055F2F0, 0x1D, STATIC_CTOR, g_rmgRoadPatternTable)
VA_COMPGEN(0x0055F310, 0x0A, STATIC_DTOR, g_rmgRoadPatternTable)

// Complete-only progress base constructor. Retail's sole caller is the
// TRandomMapProgress constructor; vtable 0x6409c0 and the existing SetTotal
// body prove the total at +4, followed by the zeroed completed count at +8.
VA(0x00530E20, 0x1C)
TProgressSink::TProgressSink(int totalSteps)
{
    m_steps = totalSteps;
    m_done = 0;
}

// Vtable 0x6409c0 slot 0 retains the generated deleting wrapper between the
// constructor and the ordinary destructor in retail link order.
VA_COMPGEN(0x00530E40, 0x23, SCALAR_DELETING_DTOR, TProgressSink)

// Complete-only RMG base virtual, exact on the first scored candidate. Vtable
// 0x6409c0 and three retail cleanup callers all restore this same vptr;
// Dreamcast has no RMG compiland.
VA(0x00530E70, 0x07)
TProgressSink::~TProgressSink()
{
}

// Vtable 0x6409c0 slot 1 stores the new total at +4. The derived progress
// dialog overrides the same interface while the base Advance slot stays pure.
VA(0x00530E80, 0x0D)  // Complete-only RMG progress base
void TProgressSink::setTotal(int totalSteps)
{
    m_steps = totalSteps;
}

namespace {

// The cinit at 0x530da0 writes these eight clockwise neighbours.  The river
// search advances by two entries, selecting only the four cardinal offsets.
// The current TPoint constructor reproduces all 127 initializer bytes after
// resolving the 16 table references, including repeated constant loads.
// Before normalization: gRmgDirections.
DATA(0x0069CDC0)
TPoint g_rmgDirections[8] = {
    TPoint(1, 0),
    TPoint(1, 1),
    TPoint(0, 1),
    TPoint(-1, 1),
    TPoint(-1, 0),
    TPoint(-1, -1),
    TPoint(0, -1),
    TPoint(1, -1)
};

// Shipyards are three tiles wide.  The connection repair pass probes the
// four water-facing squares beside their upper and lower edges before it
// floods the reachable water region.
// Before normalization: gRmgShipyardWaterOffsets.
DATA(0x0069CE00)
TPoint g_rmgShipyardWaterOffsets[RMG_SHIPYARD_WATER_OFFSET_COUNT] = {
    TPoint(-3, 0),
    TPoint(1, 0),
    TPoint(-3, 1),
    TPoint(1, 1)
};

// Before normalization: gLandRiverDeltaIndex.
DATA(0x006409A0)
static const int g_landRiverDeltaIndex[4] = {2, 0, 3, 1};

// Before normalization: gSnowRiverDeltaIndex.
DATA(0x006409B0)
static const int g_snowRiverDeltaIndex[4] = {7, 5, 4, 6};

// Thirty-two radial directions used by the placement and boundary passes.
DATA(0x00682500)
double gRmgDirectionCosines[32] = {
    1.0, 0.9807, 0.9239, 0.8315, 0.7071, 0.5556, 0.3827, 0.1951,
    0.0, -0.1951, -0.3827, -0.5556, -0.7071, -0.8315, -0.9239, -0.9807,
    -1.0, -0.9807, -0.9239, -0.8315, -0.7071, -0.5556, -0.3827, -0.1951,
    0.0, 0.1951, 0.3827, 0.5556, 0.7071, 0.8315, 0.9239, 0.9807
};
DATA(0x00682600)
double gRmgDirectionSines[32] = {
    0.0, 0.1951, 0.3827, 0.5556, 0.7071, 0.8315, 0.9239, 0.9807,
    1.0, 0.9807, 0.9239, 0.8315, 0.7071, 0.5556, 0.3827, 0.1951,
    0.0, -0.1951, -0.3827, -0.5556, -0.7071, -0.8315, -0.9239, -0.9807,
    -1.0, -0.9807, -0.9239, -0.8315, -0.7071, -0.5556, -0.3827, -0.1951
};

// Four six-entry tables drive Complete's guarded-zone connection strength.
// Their contents are retail data owned elsewhere; these address claims give
// the candidate relocations semantic identities without copying game data.
// Before normalization: gRmgGuardThresholdLow.
// Before normalization: gRmgGuardThresholdHigh.
DATA(0x006823F0) extern int g_rmgGuardThresholdLow[];
// Before normalization: gRmgGuardScaleLow.
DATA(0x00682408) extern int g_rmgGuardThresholdHigh[];
// Before normalization: gRmgGuardScaleHigh.
DATA(0x00682420) extern int g_rmgGuardScaleLow[];
DATA(0x00682438) extern int g_rmgGuardScaleHigh[];

// Before normalization: gRmgWaterNames.
DATA(0x00682700)
static const char* g_rmgWaterNames[3] = {
    DATA_COMPGEN(0x006827EC, rmgWaterNone, "None"),
    DATA_COMPGEN(0x006827E4, rmgWaterNormal, "normal"),
    DATA_COMPGEN(0x006827DC, rmgWaterIslands, "islands")
};

// Before normalization: gRmgPlayerNames.
DATA(0x0068270C)
static const char* g_rmgPlayerNames[8] = {
    DATA_COMPGEN(0x006827D8, rmgPlayerRed, "red"),
    DATA_COMPGEN(0x006827D0, rmgPlayerBlue, "blue"),
    DATA_COMPGEN(0x006827CC, rmgPlayerTan, "tan"),
    DATA_COMPGEN(0x006827C4, rmgPlayerGreen, "green"),
    DATA_COMPGEN(0x006827BC, rmgPlayerOrange, "orange"),
    DATA_COMPGEN(0x006827B4, rmgPlayerPurple, "purple"),
    DATA_COMPGEN(0x006827AC, rmgPlayerTeal, "teal"),
    DATA_COMPGEN(0x006827A4, rmgPlayerPink, "pink")
};

// Before normalization: gRmgTownNames.
DATA(0x0068272C)
static const char* g_rmgTownNames[9] = {
    DATA_COMPGEN(0x0068279C, rmgTownCastle, "castle"),
    DATA_COMPGEN(0x00682794, rmgTownRampart, "rampart"),
    DATA_COMPGEN(0x0068278C, rmgTownTower, "tower"),
    DATA_COMPGEN(0x00682784, rmgTownInferno, "inferno"),
    DATA_COMPGEN(0x00682778, rmgTownNecropolis, "necropolis"),
    DATA_COMPGEN(0x00682770, rmgTownDungeon, "dungeon"),
    DATA_COMPGEN(0x00682764, rmgTownStronghold, "stronghold"),
    DATA_COMPGEN(0x00682758, rmgTownFortress, "fortress"),
    DATA_COMPGEN(0x00682750, rmgTownConflux, "conflux")
};

// ReadRmgTemplateZones repeatedly tests a nullable field for a nonempty,
// non-space leading character. Keep the shared predicate as an ordinary
// helper; its original name and declaration are not in the DC corpus.
// Before normalization (function): IsRmgTemplateFieldSet.
static bool isRmgTemplateFieldSet(const char* value)
{
    return value && value[0] && value[0] != ' ';
}

} // namespace


template <>
inline bool std::bitset<156>::test(size_t position) const
{
    if (156 <= position) {
        // WriteMapHeader -> bitset<156>::_Xran: retail retains this call.
#pragma inline_depth(0)
        _Xran();
#pragma inline_depth()
    }
    return ((_A[position / _Nb] & ((_Ty)1 << position % _Nb)) != 0);
}

template <>
inline bool std::bitset<128>::test(size_t position) const
{
    if (128 <= position) {
        // WriteMapHeader -> bitset<128>::_Xran: retail retains this call.
#pragma inline_depth(0)
        _Xran();
#pragma inline_depth()
    }
    return ((_A[position / _Nb] & ((_Ty)1 << position % _Nb)) != 0);
}

template <>
inline bool std::bitset<144>::test(size_t position) const
{
    if (144 <= position) {
        // WriteMapHeader -> bitset<144>::_Xran: retail retains this call.
#pragma inline_depth(0)
        _Xran();
#pragma inline_depth()
    }
    return ((_A[position / _Nb] & ((_Ty)1 << position % _Nb)) != 0);
}

template <>
inline void std::bitset<144>::_Xran() const
{
    // WriteMapHeader -> string::_Tidy: retail keeps the nested ctor boundary.
#pragma inline_depth(0)
    string message("invalid bitset<N> position");
#pragma inline_depth()
    // WriteMapHeader -> out_of_range construction: retail retains this call.
#pragma inline_depth(0)
    _THROW(out_of_range, message);
#pragma inline_depth()
}

template <>
inline void std::bitset<129>::_Xran() const
{
    string message("invalid bitset<N> position");
    // WriteMapHeader -> out_of_range construction: retail retains this call.
#pragma inline_depth(0)
    _THROW(out_of_range, message);
#pragma inline_depth()
}

template <>
inline void std::bitset<70>::_Xran() const
{
    const char* text = "invalid bitset<N> position";
    string message;
    // WriteMapHeader -> string::assign: retail retains this nested call.
#pragma inline_depth(0)
    message.assign(text, strlen(text));
#pragma inline_depth()
    // WriteMapHeader -> out_of_range construction: retail retains this call.
#pragma inline_depth(0)
    _THROW(out_of_range, message);
#pragma inline_depth()
}

template <>
inline void std::bitset<28>::_Xran() const
{
    const char* text = "invalid bitset<N> position";
    string message;
    // WriteMapHeader -> string::assign: retail retains this nested call.
#pragma inline_depth(0)
    message.assign(text, strlen(text));
#pragma inline_depth()
    // WriteMapHeader -> out_of_range construction: retail retains this call.
#pragma inline_depth(0)
    _THROW(out_of_range, message);
#pragma inline_depth()
}

template <>
inline std::bitset<144>::reference::operator bool() const
{
    // WriteMapHeader -> bitset<144>::test: retail retains this nested call.
#pragma inline_depth(0)
    return _Pbs->test(_Off);
#pragma inline_depth()
}

template <>
inline std::bitset<129>::reference&
std::bitset<129>::reference::operator=(bool value)
{
    // WriteMapHeader -> bitset<129>::set: retail retains this nested call.
#pragma inline_depth(0)
    _Pbs->set(_Off, value);
#pragma inline_depth()
    return *this;
}

// Before normalization (function): assign_rmg_teams.
static void __fastcall assignRmgTeams(
    int teamCount,
    int playerCount,
    int firstTeam,
    const unsigned char* players,
    char* teams);

template <unsigned int N>
// Before normalization (function): set_available_rmg_heroes.
static void setAvailableRmgHeroes(
    std::bitset<N>* availableHeroes,
    unsigned char* heroFlag,
    unsigned char* end)
{
    int heroIndex = 0;
    while (heroFlag != end) {
        bool available = !*heroFlag;
        // WriteMapHeader -> bitset<N>::set: retail retains both call sites.
#pragma inline_depth(0)
        availableHeroes->set(heroIndex, available);
#pragma inline_depth()
        ++heroFlag;
        ++heroIndex;
    }
}

// Vtable 0x6409cc slot 0 and the 0x14-byte concrete map layout identify this
// scalar deleting wrapper. Its non-deleting half destroys the owned array of
// 0x30-byte TRmgMapItem elements before restoring the abstract map vtable.
VA_COMPGEN(0x00530F80, 0x21, SCALAR_DELETING_DTOR, type_random_map)

// The array constructor first builds m_objects, then calls clear for the
// remaining packed cell state. Dreamcast has no RMG compiland, but retail's
// EH edge, zero-initialized vector triplet, and direct call to 0x530f10 prove
// this ordinary constructor boundary.
VA(0x00530E90, 0x4A)
TRmgMapItem::TRmgMapItem()
{
    clear();
}

// The array construction at 0x530fb0 passes this body to VC6's vector
// destructor iterator with a 0x30-byte stride. It destroys TRmgMapItem's
// vector<type_object*> at offset zero; rmg.obj emits the implicit destructor
// byte-for-byte from the recovered aggregate declaration.
VA_COMPGEN(0x00530EE0, 0x26, IMPLICIT_DTOR, TRmgMapItem)

// The array constructor at 0x530e90 calls this initializer after constructing
// m_objects, and type_random_map::clear calls it for every allocated cell.
// Dreamcast has no RMG compiland, so the original method spelling is unknown;
// `clear` describes the retail operation while preserving its real boundary.
// Four compiled forms peaked at 76.95%. Direct member writes store the packed
// connection word too early (73.68%); local packed-field snapshots recover the
// retail masks and final store order, leaving only EDI lifetime/load scheduling.
// A Cartesian source family also varied all three snapshot/update group
// orders and named begin/end iterator lifetimes. None changed this body's
// score or recovered EDI's lifetime across the vector-copy guard.
// Carrying the connection snapshot across the pointer-vector erase reaches
// 91.03%. The vector owns pointers only, so that snapshot is independent of
// its clear operation. Taking every snapshot afterward restores 76.95%;
// taking tile or tileData early instead does not recover the connection
// lifetime. Public clear() is equivalent here; resize(0) adds a size guard.
// Separating declarations from post-erase assignments does not preserve that
// allocation: generated declaration/read permutations return to 76.95%, even
// with all three real locals declared before the erase and a vector reference.
VA(0x00530F10, 0x6F)
void TRmgMapItem::clear()
{
    TRmgConnectionDecoration connection = m_connection;
    m_objects.erase(m_objects.begin(), m_objects.end());
    TRmgGroundTile tile = m_tile;
    TRmgGroundTileData tileData = m_tileData;

    connection.m_present = 0;
    tile.m_landType = eTerrainWater;
    tile.m_terrainFrame = 21;
    tile.m_riverType = 0;
    tile.m_riverFrame = 0;
    tile.m_roadType = 0;
    tileData.m_roadFrame = 0;
    tileData.m_blockedDirections = 0;
    tileData.m_connectionDirection = 0;
    tileData.m_terrainFlipX = 0;
    tileData.m_terrainFlipY = 0;
    tileData.m_riverFlipX = 0;
    tileData.m_riverFlipY = 0;
    tileData.m_roadFlipX = 0;
    tileData.m_roadFlipY = 0;
    tileData.m_coastal = 0;
    tileData.m_roadEntrance = 0;
    tileData.m_placementOutline = 0;
    tileData.m_roadPassable = 1;
    tileData.m_borderObject = 0;
    tileData.m_subterraneanGate = 1;
    tileData.m_zoneBoundary = 0;
    tileData.m_hasRiver = 0;
    tileData.m_riverTarget = 0;
    tileData.m_impassable = 0;
    m_connection = connection;
    m_tile = tile;
    m_movement.m_cost = 32700;
    m_movement.m_zonePathCost = 32700;
    m_zoneState.m_score = 32700;
    m_zoneState.m_zone = -1;
    m_zoneState.m_connectionEligibility = -1;
    m_previousTile.m_x = -1;
    m_tileData = tileData;
}

// Owned-map constructor called by the generator base at 0x53609f. Retail
// multiplies width*height*levels and allocates a cookie plus 0x30-byte cells,
// passing the canonical TRmgMapItem constructor/destructor to the EH iterator.
// This overload owns its cells; the existing buffer view leaves ownership off.
// Exact: 160 bytes, including the cookie and exception-safe construction loop.
VA(0x00530FB0, 0xA0) // anchor-callee 0x53609f + array ctor/dtor/stride; retail-only
type_random_map::type_random_map(int width, int height, int levels)
{
    m_mapWidth = width;
    m_mapHeight = height;
    m_numberLevels = levels;
    m_ownsMapItems = 1;
    m_mapItems = new TRmgMapItem[width * height * levels];
}


// The array-delete helper for TRmgMapItem uses the recovered 0x30-byte stride
// and delegates every element to the implicit destructor above.
VA_COMPGEN(0x00531050, 0x58, VECTOR_DELETING_DTOR, TRmgMapItem)

// Non-deleting half called by 0x530f80. Retail owns the tile-array cleanup
// here and restores map/interface vtables at 0x6409cc and 0x6409e8.
// Exact: all 131 bytes, including both ownership/empty-array exit paths.
VA(0x005310B0, 0x83)
type_random_map::~type_random_map()
{
    if (m_ownsMapItems)
        delete[] m_mapItems;
}

// Retail's generation retry path invokes this on its temporary map before
// reinitializing every cell. The post-decrement count produces the zero guard
// and single 0x30-stride loop seen in all 42 bytes at 0x531140.
VA(0x00531140, 0x2A)
void type_random_map::clear()
{
    TRmgMapItem* mapItem = m_mapItems;
    int mapItemCount = m_mapWidth * m_mapHeight * m_numberLevels;
    while (mapItemCount--) {
        mapItem->clear();
        ++mapItem;
    }
}

// Walk the perimeter once plus its first point to close the circular run.
// Retail rejects a second open-to-blocked transition and a fully blocked
// perimeter. The outline must be nonempty: its size is the modulo divisor,
// with no empty guard in 0x531170. Preserve that caller precondition.
// Exact: all 412 raw bytes, with no relocations. Initialize the two walk
// predicates before the zone snapshots, and keep the previous state local
// to each iteration. Zone-first initialization gives 95.9937%; returning
// the final boolean expression instead of its guard adds an extra exit.
VA(0x00531170, 0x19C) // anchor-callee 0x531d93; thiscall, ret 0x1c; retail-only
unsigned char type_random_map::hasConnectedOutline(
    const std::vector<TPoint>& outline, TRmgMapPosition position,
    unsigned char allowEntrances, TRmgZone* zone, unsigned char requireGate)
{
    unsigned char blocked = 1;
    unsigned char foundBoundary = 0;
    unsigned char waterZone = zone->m_terrain == eTerrainWater;
    int zoneIndex = zone->m_slot->m_zoneIndex;
    for (unsigned int index = 0; index < outline.size() + 1; ++index) {
        unsigned char previouslyBlocked = blocked;
        TPoint offset(outline[index % outline.size()]);
        int x = position.m_x + offset.m_x;
        int y = position.m_y + offset.m_y;
        if (x < 0 || x >= m_mapWidth || y < 0 || y >= m_mapHeight) {
            blocked = 1;
        } else {
            TRmgMapItem* item = getMapItem(x, y, position.m_z);
            if (!allowEntrances && item->isRoadEntrance())
                return 0;
            blocked = !item->m_tileData.m_roadPassable
                || item->m_tile.m_landType == eTerrainRock || item->isRoadEntrance();
            if (requireGate && !item->hasSubterraneanGate())
                blocked = 1;
            if ((item->m_tile.m_landType == eTerrainWater) != waterZone)
                blocked = 1;
            if (item->m_zoneState.m_zone != zoneIndex)
                blocked = 1;
        }
        if (blocked && !previouslyBlocked) {
            if (foundBoundary)
                return 0;
            foundBoundary = 1;
        }
    }
    if (blocked && !foundBoundary)
        return 0;
    return 1;
}

// Retail generation calls this on the owned map at 0x549c8a. A water
// cell marks every non-water/non-rock neighbor in its clipped 3x3 square
// as coastal. The source spelling is role-derived; no DC RMG counterpart.
// Exact: 331 bytes. The rectangle pattern recovered in markBorderPatch
// plus one coordinate record restores retail's local lifetimes and frame.
// Scalar bounds/coordinates peak at 85.22%; rectangle alone gives 85.17%.
VA(0x00531310, 0x14B)
void type_random_map::markCoastalTiles()
{
    TRmgMapPosition position;
    TRmgMapItem* item = m_mapItems;
    for (position.m_z = 0; position.m_z < m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_mapWidth; ++position.m_x, ++item) {
                if (item->m_tile.m_landType == eTerrainWater) {
                    TRmgZoneBounds bounds;
                    bounds.m_minimumX = max(position.m_x - 1, 0);
                    bounds.m_minimumY = max(position.m_y - 1, 0);
                    bounds.m_maximumX = min(position.m_x + 2, m_mapWidth);
                    bounds.m_maximumY = min(position.m_y + 2, m_mapHeight);
                    for (int nearY = bounds.m_minimumY; nearY < bounds.m_maximumY; ++nearY) {
                        for (int nearX = bounds.m_minimumX; nearX < bounds.m_maximumX; ++nearX) {
                            TRmgMapItem* neighbor = getMapItem(nearX, nearY, position.m_z);
                            if (neighbor->m_tile.m_landType != eTerrainWater
                                && neighbor->m_tile.m_landType != eTerrainRock)
                                neighbor->m_tileData.m_coastal = 1;
                        }
                    }
                }
            }
        }
    }
}

// BuildRoadCostMap and CreateRiver both materialize a separate by-value
// position immediately before this identical descending binary search.  The
// same expansion recurs in the surrounding retail RMG corpus, with no retained
// standalone body.  An ordinary internal helper reproduces that boundary and
// lets VC6 /Ob2 decide the expansions; Dreamcast has no RMG compiland, so the
// original spelling and linkage remain provisional.
// The search uses one top test with two unconditional back edges in retail.
// VC6 rotates for (;;) and while (first < last) spellings; while (1) keeps
// this top test and restores that flow in CreateRiver (76.51% -> 79.82%).
// Before normalization (function): InsertRmgWorkItem.
static void insertRmgWorkItem(
    std::vector<TRmgMapPosition>& positions,
    std::vector<int>& costs,
    TRmgMapPosition position,
    int cost)
{
    int first = 0;
    int last = positions.size();
    int middle;
    while (1) {
        middle = (first + last) >> 1;
        if (first >= last)
            break;
        if (cost < costs[middle])
            first = middle + 1;
        else
            last = middle;
    }

    positions.insert(positions.begin() + middle, position);
    costs.insert(costs.begin() + middle, cost);
}

// Retail 0x5407dd/0x5408a2 pass the map, a by-value position and a water byte.
// Complete-only cost flood; its original source name is unavailable.
// Partial 62.4753%: retail retains single-element seed insertion and erase
// bodies, while VC6 currently expands their wrappers and retains _Destroy.
// Explicit seed insert and scalar neighbour coordinates improve 61.9258%;
// operator+ retains a position constructor absent from this retail caller.
// The canonical descending worklist helper is shared with road/river floods.
// Retail inserts cost before position here (0x531826/0x53183e); recover that
// helper boundary and its other callers before changing its shared ordering.
VA(0x00531460, 0x441)
void type_random_map::floodConnectionCosts(TRmgMapPosition position, unsigned char waterZone)
{
    std::vector<int> costs;
    std::vector<TRmgMapPosition> positions;
    TRmgMapItem* seed = getMapItem(position);
    int zone = seed->m_zoneState.m_zone;
    positions.insert(positions.end(), position);
    costs.insert(costs.end(), 0);
    seed->m_movement.m_cost = 0;
    seed->m_previousTile.m_x = -1;
    seed->m_previousTile.m_y = -1;
    seed->m_previousTile.m_z = -1;
    while (positions.size()) {
        TRmgMapPosition currentPosition = positions.back();
        int queuedCost = costs.back();
        TRmgMapItem* current = getMapItem(currentPosition);
        positions.erase(positions.end() - 1);
        costs.erase(costs.end() - 1);
        int currentZone = current->m_zoneState.m_zone;
        int currentCost = currentZone == zone
            ? current->m_movement.m_cost : current->m_movement.m_zonePathCost;
        int direction = 8;
        if (current->isRoadEntrance()) {
            int objectType = current->m_objects[0]->m_properties->m_prototype->m_objectType;
            if (!g_adventureObjectLandBlocked[objectType][1])
                direction = 5;
        }
        while (direction--) {
            int nextCost = currentCost + 1;
            TRmgMapPosition nextPosition;
            nextPosition.m_x = currentPosition.m_x + g_rmgDirections[direction].m_x;
            nextPosition.m_y = currentPosition.m_y + g_rmgDirections[direction].m_y;
            nextPosition.m_z = currentPosition.m_z;
            if (nextPosition.m_x < 0 || nextPosition.m_x >= m_mapWidth
                || nextPosition.m_y < 0 || nextPosition.m_y >= m_mapHeight)
                continue;
            TRmgMapItem* next = getMapItem(nextPosition);
            if (next->m_zoneState.m_zone < 0 || !next->m_tileData.m_roadPassable
                || next->m_tile.m_landType == eTerrainRock)
                continue;
            if (next->isRoadEntrance()) {
                int objectType = next->m_objects[0]->m_properties->m_prototype->m_objectType;
                const unsigned char* traits = g_adventureObjectLandBlocked[objectType];
                if (traits[0] && !traits[2])
                    continue;
                if (!traits[1] && direction > 0 && direction < 4)
                    continue;
            }
            if (next->m_zoneState.m_zone != zone) {
                nextCost = currentCost + 10;
                if (currentZone != zone && currentZone != next->m_zoneState.m_zone)
                    continue;
                if (next->m_movement.m_zonePathCost <= nextCost)
                    continue;
                next->m_movement.m_zonePathCost = nextCost;
                next->m_tileData.m_connectionDirection = direction - 4;
                next->m_zoneState.m_connectionEligibility = zone;
            } else {
                if (currentZone != zone)
                    continue;
                if (next->m_tile.m_landType == eTerrainWater)
                    nextCost = currentCost + 10;
                if (next->m_movement.m_cost <= nextCost)
                    continue;
                if (!currentCost && next->hasSubterraneanGate()
                    && (next->m_tile.m_landType != eTerrainWater || waterZone))
                    nextCost = 0;
                next->setMovementCost(nextCost, currentPosition);
            }
            insertRmgWorkItem(positions, costs, nextPosition, nextCost);
        }
    }
}

// Retail checks trigger and blocked-mask cells separately, even when both
// select the same tile. Only the trigger arm observes rejectBorder. The
// category-zero water rule belongs to the blocked-mask arm, not the whole
// footprint. Bounds are signed; the two mask loops use unsigned indices.
// Independently recomputed mask indices and a prototype reference reach
// 83.0343%; keeping a descending map position alongside ascending mask
// indices reaches 90.2171%. Both retained mask-range calls agree. The
// current 0x20 frame still differs from retail's 0x1c frame and reloads.
VA(0x005318B0, 0x212) // anchor-callee 0x531d29; thiscall, ret 0x18; retail-only
unsigned char type_random_map::isPlacementBlocked(
    TRmgObjectPropertiesRef* properties, TRmgMapPosition position,
    int zoneIndex, unsigned char rejectBorder)
{
    TObjectType& prototype = *properties->m_prototype;
    if (position.m_x < prototype.getWidth() - 1 || position.m_x >= m_mapWidth
        || position.m_y < prototype.getHeight() - 1 || position.m_y >= m_mapHeight)
        return 1;
    TRmgMapPosition nearby = position;
    for (unsigned int y = 0; y < prototype.getHeight(); ++y, --nearby.m_y) {
        nearby.m_x = position.m_x;
        for (unsigned int x = 0; x < prototype.getWidth(); ++x, --nearby.m_x) {
            TRmgMapItem* item = getMapItem(nearby);
            if (prototype.m_triggerMask.test(CObjectType::getBitPos(x, y))) {
                if (!item->m_tileData.m_roadPassable || item->m_tile.m_landType == eTerrainRock
                    || item->isRoadEntrance() || item->m_zoneState.m_zone != zoneIndex)
                    return 1;
                if (rejectBorder && item->hasBorderObject())
                    return 1;
            }
            if (!prototype.m_passableMask.test(CObjectType::getBitPos(x, y))) {
                if (!item->m_tileData.m_roadPassable || item->m_tile.m_landType == eTerrainRock
                    || item->isRoadEntrance() || item->m_zoneState.m_zone != zoneIndex)
                    return 1;
                if (item->m_tile.m_landType == eTerrainWater) {
                    if (prototype.m_slotCategory != TObjectType::SLOT_CATEGORY_0
                        || !prototype.m_recommendedTerrainMask.test(eTerrainWater))
                        return 1;
                } else if (prototype.m_slotCategory == TObjectType::SLOT_CATEGORY_0
                           && prototype.m_recommendedTerrainMask.test(eTerrainWater)) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

// Both map helpers are retained by carveBranchingPaths. Connection-decorated
// tiles keep their existing border/gate flags. These provisional names refer
// to the same cell roles used by the surrounding connection routines.
// Both bodies are exact: the scalar-coordinate patch is 256 bytes and the
// value-coordinate border patch is 287 bytes, including their clipped scans.
VA(0x00531AD0, 0x100) // anchor-callee 0x5441a1; Complete-only, ret 0xc
void type_random_map::openPathPatch(int x, int y, int level)
{
    TRmgMapItem* item = getMapItem(x, y, level);
    if (!item->m_connection.m_present) {
        item->m_tileData.m_borderObject = 0;
        item->m_tileData.m_subterraneanGate = 1;
    }
    TRmgZoneBounds bounds;
    bounds.m_minimumX = max(x - 1, 0);
    bounds.m_minimumY = max(y - 1, 0);
    bounds.m_maximumX = min(x + 2, m_mapWidth);
    bounds.m_maximumY = min(y + 2, m_mapHeight);
    for (int row = bounds.m_minimumY; row < bounds.m_maximumY; ++row) {
        for (int column = bounds.m_minimumX; column < bounds.m_maximumX; ++column) {
            TRmgMapItem* nearby = getMapItem(column, row, level);
            if (!nearby->m_connection.m_present) {
                nearby->m_tileData.m_borderObject = 0;
                nearby->m_tileData.m_subterraneanGate = 1;
            }
        }
    }
}

VA(0x00531BD0, 0x11F) // anchor-callee 0x544343; Complete-only, ret 0xc
void type_random_map::markBorderPatch(TRmgMapPosition position)
{
    TRmgMapItem* item = getMapItem(position);
    if (!item->m_connection.m_present) {
        item->m_tileData.m_subterraneanGate = 0;
        item->m_tileData.m_borderObject = 1;
    }
    TRmgZoneBounds bounds;
    bounds.m_minimumX = max(position.m_x - 1, 0);
    bounds.m_minimumY = max(position.m_y - 1, 0);
    bounds.m_maximumX = min(position.m_x + 2, m_mapWidth);
    bounds.m_maximumY = min(position.m_y + 2, m_mapHeight);
    for (int row = bounds.m_minimumY; row < bounds.m_maximumY; ++row) {
        for (int column = bounds.m_minimumX; column < bounds.m_maximumX; ++column) {
            TRmgMapItem* nearby = getMapItem(column, row, position.m_z);
            if (!nearby->isRoadEntrance() && nearby->m_tileData.m_roadPassable
                && nearby->m_tile.m_landType != eTerrainRock
                && nearby->m_tile.m_landType != eTerrainWater
                && !nearby->m_connection.m_present)
                nearby->m_tileData.m_subterraneanGate = 0;
        }
    }
}

// The footprint/outline helper calls are retained, followed by an expanded
// entrance lookup one row below the trigger. Retail checks a negative zone
// separately from a different zone, then compares water membership as ints.
// Scalar trigger coordinates and a named byte result reach 61.5761%; a
// prototype reference and separate passability/rock guards reach 86.6848%.
// Moving coordinate snapshots to function entry does not recover retail's
// EDI/EBX homes across the helper calls; terrain snapshots also remain lower.
VA(0x00531CF0, 0x1A5) // anchor-callee 0x541c73; thiscall, ret 0x14; retail-only
unsigned char type_random_map::canPlaceObject(
    TRmgObjectPropertiesRef* properties, TRmgMapPosition position, TRmgZone* zone)
{
    TObjectType& prototype = *properties->m_prototype;
    int zoneIndex = zone->m_slot->m_zoneIndex;
    if (isPlacementBlocked(properties, position, zoneIndex, 0))
        return 0;
    int objectType = prototype.m_objectType;
    properties->buildOutline();
    if (!hasConnectedOutline(properties->m_outline, position,
            g_adventureObjectLandBlocked[objectType][2] && g_adventureObjectLandBlocked[objectType][1],
            zone, 0))
        return 0;
    if (!prototype.m_hasTrigger)
        return 1;
    int x = position.m_x - prototype.m_triggerCell.m_x;
    int y = position.m_y - prototype.m_triggerCell.m_y;
    ++y;
    TRmgMapPosition entrance(x, y, position.m_z);
    if (y >= m_mapHeight)
        return 0;
    TRmgMapItem* item = getMapItem(entrance);
    if (!item->m_tileData.m_roadPassable)
        return 0;
    if (item->m_tile.m_landType == eTerrainRock)
        return 0;
    if (item->m_zoneState.m_zone < 0)
        return 0;
    if (item->m_zoneState.m_zone != zoneIndex)
        return 0;
    if (item->isRoadEntrance()) {
        int entranceType = item->m_objects[0]->m_properties->m_prototype->m_objectType;
        if (!g_adventureObjectLandBlocked[entranceType][2])
            return 0;
    }
    unsigned char result = (zone->m_terrain == eTerrainWater) == (item->m_tile.m_landType == eTerrainWater);
    return result;
}

// Terrain vtable 0x6409cc slot 1 stores the generic adapter's integer
// kind and frame, then its two flip bytes. Retail's mask/sign-extension
// pair proves signed integer storage; the same generic interface also
// carries road and river kinds. Preserve that domain through selection and
// terrain-copy locals rather than introducing an int-to-enum conversion.
// Exact: a 97-candidate input-order/binding batch found 24 exact forms.
// Capture both flip bytes before the frame: the prior flipY/frame/flipX
// order gives 69.2143%, with the same one-block CFG and no call differences.
VA(0x00532190, 0x6D) // anchor-vtable + packed-field writes; retail-only
void type_random_map::setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile)
{
    TRmgMapItem& item = m_mapItems[point.m_y * m_mapWidth + point.m_x];
    unsigned char flipY = tile.m_flipY;
    unsigned char flipX = tile.m_flipX;
    int frame = tile.m_frame;
    int terrain = tile.m_terrain;
    item.m_tile.m_landType = terrain;
    item.m_tile.m_terrainFrame = frame;
    item.m_tileData.m_terrainFlipX = flipX;
    item.m_tileData.m_terrainFlipY = flipY;
}

// Slot 2 updates only the packed eight-bit terrain frame.
VA(0x00532200, 0x3C)
void type_random_map::setOverlay(const TRmgGridPoint& point, int value)
{
    TRmgMapItem& item = m_mapItems[point.m_y * m_mapWidth + point.m_x];
    item.m_tile.m_terrainFrame = value;
}

// Vtable 0x6409cc slot 3 returns the map's two unsigned dimensions.
// The hidden result pointer and two stores fix the coordinate return ABI.
// The proven grid copy constructor moves the width load before the result
// pointer load (97.56%, with 100% banked). Named constructed and assigned
// results keep that scheduling difference and leave createRiver unchanged.
// A 16-state batch of member assignment, signed/unsigned input locals, both
// input orders, and named/temporary returns also remains at 97.5556%.
VA(0x00532240, 0x15) // anchor-vtable 0x6409cc+0x0c; retail-only
TRmgGridPoint type_random_map::getSize()
{
    return TRmgGridPoint(m_mapWidth, m_mapHeight);
}

// Slot 4 expands the packed terrain fields into the adapter's three-dword
// value, including the two independent flip bytes.
VA(0x00532260, 0x60)
rmgTerrainTile type_random_map::getTile(const TRmgGridPoint& point)
{
    TRmgMapItem& item = m_mapItems[point.m_y * m_mapWidth + point.m_x];
    rmgTerrainTile tile;
    tile.m_terrain = item.m_tile.m_landType;
    tile.m_frame = item.m_tile.m_terrainFrame;
    tile.m_flipX = item.m_tileData.m_terrainFlipX;
    tile.m_flipY = item.m_tileData.m_terrainFlipY;
    return tile;
}

// Vtable 0x6409cc slots 5 and 6 index the 0x30-byte cell array with the
// supplied x/y point. The two signed extracts select the six-bit land kind
// and its adjacent eight-bit terrain frame from TRmgGroundTile.
VA(0x005322C0, 0x2A)
int type_random_map::getLand(const TRmgGridPoint& point)
{
    return m_mapItems[point.m_y * m_mapWidth + point.m_x].m_tile.m_landType;
}

VA(0x005322F0, 0x2A)
int type_random_map::getOverlay(const TRmgGridPoint& point)
{
    return m_mapItems[point.m_y * m_mapWidth + point.m_x]
        .m_tile.m_terrainFrame;
}

// Complete-only base of the road adapter, exact on the first scored candidate.
// The derived deleting destructor at 0x532320 and one retail cleanup path call
// this retained vptr restoration; Dreamcast has no RMG compiland.
VA(0x00532350, 0x07)
TRmgRoadMapAdapterInterface::~TRmgRoadMapAdapterInterface()
{
}

// Vtable 0x640a20 slot 0 retains the road-interface deleting wrapper; retail
// places this generated COMDAT later than the ordinary destructor.
VA_COMPGEN(0x00537940, 0x23, SCALAR_DELETING_DTOR, TRmgRoadMapAdapterInterface)

// The concrete road adapter is built at 0x548120 with a type_random_map
// view at +4. Vtable 0x640a04 slots 1/2/4/5/6 name the following bodies;
// the class and method names describe retail roles (no Dreamcast RMG TU).
// Exact: capture flipY, frame, flipX, then terrain before writing the cell.
// Retail 0x53237d..0x532399 establishes these input lifetimes. Direct reads
// during the stores score 38.36%; a whole-tile or packed-word copy changes
// the load/store schedule. The 13-state batch isolates this scalar form.
VA(0x00532360, 0x6E) // anchor-vtable 0x640a04+4; retail-only
void TRmgRoadMapAdapter::setTile(
    const TRmgGridPoint& point, const rmgTerrainTile& tile)
{
    TRmgMapItem& item = m_map->m_mapItems[
        point.m_y * m_map->m_mapWidth + point.m_x];
    unsigned char flipY = tile.m_flipY;
    int frame = tile.m_frame;
    unsigned char flipX = tile.m_flipX;
    int terrain = tile.m_terrain;
    item.m_tile.m_roadType = terrain;
    item.m_tileData.m_roadFrame = frame;
    item.m_tileData.m_roadFlipX = flipX;
    item.m_tileData.m_roadFlipY = flipY;
}

// Naming the cell keeps its base address live and is exact. Addressing only
// the nested bitfield produces a field-address LEA and scores 84.5652%.
VA(0x005323D0, 0x3C) // anchor-vtable 0x640a04+8; retail-only
void TRmgRoadMapAdapter::setOverlay(const TRmgGridPoint& point, int value)
{
    TRmgMapItem& item = m_map->m_mapItems[point.m_y * m_map->m_mapWidth + point.m_x];
    item.m_tile.m_roadType = value;
}

VA(0x00532410, 0x62) // anchor-vtable 0x640a04+0x10; retail-only
rmgTerrainTile TRmgRoadMapAdapter::getTile(const TRmgGridPoint& point)
{
    TRmgMapItem& item = m_map->m_mapItems[
        point.m_y * m_map->m_mapWidth + point.m_x];
    rmgTerrainTile tile;
    tile.m_terrain = item.m_tile.m_roadType;
    tile.m_frame = item.m_tileData.m_roadFrame;
    tile.m_flipX = item.m_tileData.m_roadFlipX;
    tile.m_flipY = item.m_tileData.m_roadFlipY;
    return tile;
}

VA(0x00532480, 0x2D) // anchor-vtable 0x640a04+0x14; retail-only
int TRmgRoadMapAdapter::getLand(const TRmgGridPoint& point)
{
    return m_map->m_mapItems[point.m_y * m_map->m_mapWidth + point.m_x]
        .m_tile.m_roadType;
}

VA(0x005324B0, 0x2D) // anchor-vtable 0x640a04+0x18; retail-only
int TRmgRoadMapAdapter::getOverlay(const TRmgGridPoint& point)
{
    return m_map->m_mapItems[point.m_y * m_map->m_mapWidth + point.m_x]
        .m_tile.m_landType;
}

// The two concrete adapter vtables share retail 0x532790. Keep both source
// methods; the river method below owns their joint ICF representative.
TRmgGridPoint TRmgRoadMapAdapter::getSize()
{
    TRmgGridPoint size = m_map->getSize();
    return size;
}

// The real road-painting stack construction at 0x548120 retains the
// concrete adapter vtable and this ordinary deleting wrapper naturally.
VA_COMPGEN(0x00532320, 0x21, SCALAR_DELETING_DTOR, TRmgRoadMapAdapter)

// Complete-only base of the river adapter, exact on the first scored candidate.
// The derived deleting destructor at 0x5324e0 and two CreateRiver cleanup paths
// call this retained vptr restoration; Dreamcast has no RMG compiland.
VA(0x00532510, 0x07)
TRmgMapAdapterInterface::~TRmgMapAdapterInterface()
{
}

// Vtable 0x640a58 slot 0 retains the interface's generated deleting wrapper;
// retail places its COMDAT later than the ordinary destructor.
VA_COMPGEN(0x00537910, 0x23, SCALAR_DELETING_DTOR, TRmgMapAdapterInterface)

// Vtable 0x640a3c slot 0 and the 0x08 concrete adapter layout identify this
// scalar deleting wrapper. The retained body delegates to the adapter-interface
// destructor at 0x532510 before conditionally releasing the object.
VA_COMPGEN(0x005324E0, 0x21, SCALAR_DELETING_DTOR, TRmgMapAdapter)

// Concrete river vtable 0x640a3c slot 1. Retail 0x53257f/0x532594 writes
// the river sprite and flips, then 0x5325ac sets presence from the full kind.
// For nonzero input, 0x532648 marks the clipped 3x3 neighbourhood impassable;
// 0x532700..0x532705 clears routing targets in the clipped 5x5 neighbourhood
// only where the stored four-bit river kind is zero. A zero input does not
// undo either neighbourhood. These are separate flags, not a single target.
// Signed, end-exclusive bounds and y-major traversal follow both retail loops.
// Complete-only: no Dreamcast RMG compiland supplies source names or scopes.
// Exact: snapshot the four sprite inputs before either packed store, as
// 0x532540..0x532558 does; retain the two later full-kind reads. Direct field
// reads during the stores produce extra writes (79.11%). A bounds record keeps
// retail's operand homes (98.48%); naming the first loop's cell then preserves
// its base instead of forming a flag-field address. Of 60 receiver refinements,
// 12 reproduce all 517 bytes; no other RMG score changes with this form.
VA(0x00532520, 0x205) // anchor-vtable + packed writes and neighbourhood CFG
void TRmgMapAdapter::setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile)
{
    TRmgMapItem& item = m_map->m_mapItems[point.m_y * m_map->m_mapWidth + point.m_x];
    unsigned char flipX = tile.m_flipX;
    int terrain = tile.m_terrain;
    unsigned char flipY = tile.m_flipY;
    int frame = tile.m_frame;
    item.m_tile.m_riverType = terrain;
    item.m_tile.m_riverFrame = frame;
    item.m_tileData.m_riverFlipX = flipX;
    item.m_tileData.m_riverFlipY = flipY;
    unsigned char present = tile.m_terrain != 0;
    item.m_tileData.m_hasRiver = present;
    if (tile.m_terrain != 0) {
        {
            TRmgZoneBounds bounds;
            bounds.m_minimumX = max(static_cast<int>(point.m_x) - 1, 0);
            bounds.m_minimumY = max(static_cast<int>(point.m_y) - 1, 0);
            bounds.m_maximumX = min(static_cast<int>(point.m_x) + 2, m_map->m_mapWidth);
            bounds.m_maximumY = min(static_cast<int>(point.m_y) + 2, m_map->m_mapHeight);
            for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
                for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
                    TRmgMapItem& neighbour = m_map->m_mapItems[y * m_map->m_mapWidth + x];
                    neighbour.m_tileData.m_impassable = 1;
                }
            }
        }
        {
            TRmgZoneBounds bounds;
            bounds.m_minimumX = max(static_cast<int>(point.m_x) - 2, 0);
            bounds.m_minimumY = max(static_cast<int>(point.m_y) - 2, 0);
            bounds.m_maximumX = min(static_cast<int>(point.m_x) + 3, m_map->m_mapWidth);
            bounds.m_maximumY = min(static_cast<int>(point.m_y) + 3, m_map->m_mapHeight);
            for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
                for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
                    TRmgMapItem& neighbour = m_map->m_mapItems[y * m_map->m_mapWidth + x];
                    if (neighbour.m_tile.m_riverType == 0) {
                        neighbour.m_tileData.m_riverTarget = 0;
                    }
                }
            }
        }
    }
}

// Concrete river vtable 0x640a3c slot 2. The four-bit field at +0x24 bit 14
// is the river kind, and +0x28 bit 29 records whether a river is present.
// The former m_riverTarget write selected bit 30, contradicting retail's
// AND 0xdfffffff / SHL 29. The full tile setter at 0x53259b..0x5325ac
// independently proves this bit; bit 30 remains the routing endpoint flag.
// Exact: a byte predicate keeps SETNE in the argument's low register and
// schedules it before the kind store. The direct int predicate clears another
// register (83.2813%). Of 60 real addressing/predicate hypotheses, 42 reach
// all 87 retail bytes; this named byte leaves every other RMG score unchanged.
VA(0x00532730, 0x57) // anchor-vtable + packed-field evidence; Complete-only
void TRmgMapAdapter::setOverlay(const TRmgGridPoint& point, int value)
{
    TRmgMapItem& item = m_map->m_mapItems[point.m_y * m_map->m_mapWidth + point.m_x];
    item.m_tile.m_riverType = value;
    unsigned char present = value != 0;
    item.m_tileData.m_hasRiver = present;
}

// Both concrete adapter vtables share this size forwarding body. The river
// adapter's existing construction path independently establishes its owner.
// Five forwarding-copy controls preserve the call, CFG and ABI but leave
// the temporary in the opposite register pair (85.1765%). Twenty-two
// value/reference/assignment/component controls retain that peak; direct
// return scores 84.6471%. Keep the same named result in both ICF owners.
VA(0x00532790, 0x27) // vtable 0x640a3c slot 3, ICF with road slot 3
TRmgGridPoint TRmgMapAdapter::getSize()
{
    TRmgGridPoint size = m_map->getSize();
    return size;
}

// Concrete river vtable 0x640a3c slot 4 returns the river sprite. The signed
// shifts in retail prove riverType and riverFrame, and bits 17/18 supply flips.
// Exact: the fieldwise returned value reproduces all 99 normalized bytes.
VA(0x005327C0, 0x63) // anchor-vtable + packed-field evidence; Complete-only
rmgTerrainTile TRmgMapAdapter::getTile(const TRmgGridPoint& point)
{
    const TRmgMapItem& item = m_map->m_mapItems[point.m_y * m_map->m_mapWidth + point.m_x];
    rmgTerrainTile tile;
    tile.m_terrain = item.m_tile.m_riverType;
    tile.m_frame = item.m_tile.m_riverFrame;
    tile.m_flipX = item.m_tileData.m_riverFlipX;
    tile.m_flipY = item.m_tileData.m_riverFlipY;
    return tile;
}

// Concrete river-adapter vtable 0x640a3c slots 5 and 6 read the packed river
// kind and underlying land kind from the wrapped map's 0x30-byte cell array.
VA(0x00532830, 0x2D)
int TRmgMapAdapter::getLand(const TRmgGridPoint& point)
{
    return m_map->m_mapItems[point.m_y * m_map->m_mapWidth + point.m_x]
        .m_tile.m_riverType;
}

VA(0x00532860, 0x2D)
int TRmgMapAdapter::getOverlay(const TRmgGridPoint& point)
{
    return m_map->m_mapItems[point.m_y * m_map->m_mapWidth + point.m_x]
        .m_tile.m_landType;
}

// Map writer 0x54ac46 walks cells by 0x30. Retail emits seven byte writes:
// terrain/frame, river/frame, road/frame, then six flips and the coastal bit.
// All signed field widths are corroborated by the shift/sign-extension pairs.
// Exact with a separate char flags accumulator copied into the existing
// output byte. Accumulating directly in the address-taken output byte keeps
// every update in memory (93.68%); a distinct final output local uses the
// wrong stack slot (99.88%); widening the accumulator reaches 99.85%.
VA(0x00532890, 0x104) // anchor-callee 0x54ac46 + packed cell fields; retail-only
void TRmgMapItem::write(TAbstractFile* outfile)
{
    char land = m_tile.m_landType;
    outfile->write(&land, sizeof(land));
    char value = m_tile.m_terrainFrame;
    outfile->write(&value, sizeof(value));
    value = m_tile.m_riverType;
    outfile->write(&value, sizeof(value));
    value = m_tile.m_riverFrame;
    outfile->write(&value, sizeof(value));
    value = m_tile.m_roadType;
    outfile->write(&value, sizeof(value));
    value = m_tileData.m_roadFrame;
    outfile->write(&value, sizeof(value));
    char flags = 0;
    if (m_tileData.m_terrainFlipX) flags |= 1;
    if (m_tileData.m_terrainFlipY) flags |= 2;
    if (m_tileData.m_riverFlipX) flags |= 4;
    if (m_tileData.m_riverFlipY) flags |= 8;
    if (m_tileData.m_roadFlipX) flags |= 16;
    if (m_tileData.m_roadFlipY) flags |= 32;
    if (m_tileData.m_coastal) flags |= 64;
    value = flags;
    outfile->write(&value, sizeof(value));
}
// BuildZoneBoundaries owns a temporary TRmgTownSlot. Its unwind reaches
// this implicit destructor with the whole slot receiver, so the released
// pointer at +0xc8 is m_connections._First (vector itself starts at +0xc4).
// The existing canonical slot lifetime naturally emits this 50-byte body;
// treating it as a vector destructor would select the wrong receiver ABI.
VA_COMPGEN(0x005329A0, 0x32, IMPLICIT_DTOR, TRmgTownSlot)

// The boundary coordinator constructs both a temporary zone and owned
// water zones through this same retained body. The final three members are
// vectors; 0x53d9ae/0x53da0d prove signed-short connection distances.
// Exact: 207/207 raw bytes, including rand. Assigning slot in the body
// preserves vector construction first; retaining the parameter avoids
// reloading slot across rand. The shared town-selection exit is required:
// a result initialized to -1 and assigned before break adds a stack home
// (91.47%), while a post-loop selectedTown == 9 test adds a comparison.
VA(0x005329E0, 0xCF) // anchor-callee 0x53e149/0x53e45c; thiscall, ret 4
TRmgZone::TRmgZone(TRmgTownSlot* newSlot)
{
    m_slot = newSlot;
    int available = 0;
    for (int town = 0; town < 9; ++town) {
        if (newSlot->m_allowedTowns[town])
            ++available;
    }
    int selectedTown;
    if (available) {
        int selected = rand() % available;
        for (selectedTown = 0; selectedTown < 9; ++selectedTown) {
            if (newSlot->m_allowedTowns[selectedTown] && --selected < 0)
                goto townSelected;
        }
    }
    selectedTown = -1;
townSelected:
    m_alignment = selectedTown;
    m_boundaryRoughness = newSlot->m_size;
    m_bounds.m_minimumX = 32000;
    m_bounds.m_maximumX = -32000;
    m_bounds.m_minimumY = 32000;
    m_bounds.m_maximumY = -32000;
    m_active = 0;
    memset(m_objectCountByType, 0, sizeof(m_objectCountByType));
}


// Three trivial member vectors account for all 118 retained destructor
// bytes, including the three independently resolved operator-delete calls.
VA(0x00532B50, 0x76)
TRmgZone::~TRmgZone()
{
}

// Both the level-occupancy pass and the bounds pass in FilterZonePositions
// copy the whole coordinate before selecting a component. That retained
// value-copy shape motivates this ordinary accessor; no DC name is known.
TRmgMapPosition TRmgZone::getLevelPosition() const
{
    return m_levelPosition;
}

// Candidate placement loads all three coordinates before writing the zone,
// consistent with passing the coordinate value through an ordinary setter.
void TRmgZone::setLevelPosition(TRmgMapPosition position)
{
    m_levelPosition = position;
}



// FilterZonePositions calls this predicate at 0x53b4b7 and 0x53b5ae.
// The two center coordinates, template sizes and map-level comparison prove
// its role independently of the provisional name. Return value is in al.
// Residual: otherSize and combinedSize exchange ECX/EBX (96.38%).
// A separate branch-local minimum reproduces the value-select sequence;
// min(otherSize, thisSize) and _cpp_min force addressable operands instead
// (84.74/87.06%). A conditional minimum, clamping thisSize in place, or
// extending minimumSize outside the level branch loses that sequence.
// Swapping size initialization order, reading fields again in the minimum,
// reversing the sum operands and giving the sum a branch-local lifetime
// do not settle the remaining register assignment.
// Generated arithmetic lifetimes reach 96.43% with dy before dx and an
// in-place clearance subtraction. Named squares and comparison results do
// not improve the remaining allocation; keep the signed sqrt/ftol boundary.
VA(0x00532BD0, 0xA8) // anchor-callee 0x53b4b7/0x53b5ae; thiscall, ret 4
unsigned char TRmgZone::canConnect(const TRmgZone* other) const
{
    int dy = m_levelPosition.m_y - other->m_levelPosition.m_y;
    int dx = m_levelPosition.m_x - other->m_levelPosition.m_x;
    int distance = static_cast<int>(sqrt(static_cast<double>(dx * dx + dy * dy)));
    int otherSize = other->m_slot->m_size;
    int thisSize = m_slot->m_size;
    int combinedSize = thisSize + otherSize;
    if (other->m_levelPosition.m_z != m_levelPosition.m_z) {
        if (combinedSize < distance)
            return 0;
        int minimumSize = thisSize;
        if (otherSize < minimumSize)
            minimumSize = otherSize;
        combinedSize -= distance;
        return combinedSize > minimumSize / 2;
    }
    return 11 * combinedSize >= 10 * distance;
}

// Lazy exterior boundary of blocked/trigger cells. Start below the first
// occupied bottom-row cell, then turn in cardinal steps around the mask.
// The eight-direction table is shared with the connection/river routines.
// Retail repeats the first-point comparison after translation and reverses
// the last search direction by four; it never appends a duplicate endpoint.
// Exact after normal relocation resolution. Independent mask-index calls,
// size() > 0, y-before-x initialization and a copied direction retain the
// 0x18 frame and load order. Canonical point addition supplies translation;
// comparing position != start instead reverses the last operands (99.8323%).
VA(0x00532C80, 0x1BA) // anchor-callee 0x531d49; thiscall, ret 0; retail-only
void TRmgObjectPropertiesRef::buildOutline()
{
    if (m_outline.size() > 0)
        return;
    TPoint position;
    position.m_y = 0;
    position.m_x = 0;
    while (static_cast<unsigned int>(-position.m_x) < m_prototype->getWidth()) {
        if (!m_prototype->m_passableMask.test(CObjectType::getBitPos(-position.m_x, 0)) || m_prototype->m_triggerMask.test(CObjectType::getBitPos(-position.m_x, 0)))
            break;
        --position.m_x;
    }
    if (position.m_x == -m_prototype->getWidth())
        return;
    position.m_y = 1;
    TPoint start = position;
    int direction = 6;
    do {
        m_outline.push_back(position);
        int attempts = 0;
        do {
            direction = (direction - 2) & 7;
            TPoint offset = g_rmgDirections[direction];
            TPoint nearby(position.m_x + offset.m_x, position.m_y + offset.m_y);
            if (nearby.m_x > 0 || static_cast<unsigned int>(-nearby.m_x) >= m_prototype->getWidth()
                || nearby.m_y > 0 || static_cast<unsigned int>(-nearby.m_y) >= m_prototype->getHeight())
                break;
            if (m_prototype->m_passableMask.test(CObjectType::getBitPos(-nearby.m_x, -nearby.m_y)) && !m_prototype->m_triggerMask.test(CObjectType::getBitPos(-nearby.m_x, -nearby.m_y)))
                break;
        } while (++attempts < 4);
        position = position + TRmgVector(g_rmgDirections[direction].m_x, g_rmgDirections[direction].m_y);
        direction = (direction - 4) & 7;
    } while (start != position);
}

// Cached ordering for two object images that overlap the same map square.
// Underlays have priority zero; other columns inherit or advance priority
// according to the passable mask. Only cells in the draw mask are written.
// Exact: all 414 resolved retail bytes, including the four _Xran calls.
// A height-tested inner for-loop with priority updated before drawing gave
// 43.93%; retail draws once, increments/tests y, then updates priority for
// the next cell. Writing unsigned x > 0 rather than x preserves the jbe
// at the previous-column guard (99.58% with the boolean spelling).
VA(0x00532E40, 0x19E) // anchor-callee 0x536ee9/0x53700b; retail-only
void TRmgObjectPropertiesRef::buildOverlapPriorities()
{
    if (m_prioritiesInitialized)
        return;
    m_prioritiesInitialized = 1;
    for (unsigned int x = 0; x < m_prototype->getWidth(); ++x) {
        int priority = !m_prototype->m_isUnderlay;
        unsigned int y = 0;
        for (;;) {
            if (m_prototype->m_imageInfo.m_drawMask.test(CObjectType::getBitPos(x, y)))
                m_overlapPriorities[x][y] = priority;
            if (++y >= m_prototype->getHeight())
                break;
            if (!m_prototype->m_isUnderlay) {
                if (m_prototype->m_passableMask.test(CObjectType::getBitPos(x, y))) {
                    if (x > 0 && !m_prototype->m_passableMask.test(
                            CObjectType::getBitPos(x - 1, y)))
                        priority = m_overlapPriorities[x - 1][y];
                    else
                        ++priority;
                } else {
                    if (m_prototype->m_passableMask.test(
                            CObjectType::getBitPos(x, y - 1)))
                        priority = 1;
                    else
                        ++priority;
                }
            }
        }
    }
}

// The generator destructor calls this body at 0x537e84, then frees the
// template. It deletes every owned slot, destroys zones, and finally name;
// the member offsets agree with the rmg.txt coordinator and zone reader.
VA(0x00532FE0, 0xB4) // anchor-callee 0x537e84; thiscall, ret 0; retail-only
TRmgTemplate::~TRmgTemplate()
{
    for (int zone = 0; zone < m_zones.size(); ++zone)
        delete m_zones[zone];
}

// The rmg.txt connection reader calls this for both endpoint identifiers.
// It searches the template's pointer vector and compares each slot's first
// field; ret 4 fixes the member's one integer argument.
VA(0x005330A0, 0x3E) // anchor-callee 0x53824c/0x538257; retail-only
TRmgTownSlot* TRmgTemplate::findZone(int zoneIndex)
{
    for (int zone = 0; zone < m_zones.size(); ++zone) {
        if (m_zones[zone]->m_zoneIndex == zoneIndex)
            return m_zones[zone];
    }
    return 0;
}

// Shipyard water probing copies all three fields from the object before
// adding offsets. This ordinary value accessor models that copy boundary;
// the source name is inferred from the Complete-only retail use.
TRmgMapPosition type_object::getPosition() const
{
    return m_position;
}

// Base constructor retained by the shipyard's derived construction at
// 0x541d3b. The property reference and five placement marks prove the body.
// Exact: assignment in the body places the vptr before m_properties.
// A member initializer reverses those stores (98.5238%).
VA(0x005330E0, 0x39) // anchor-callee 0x541d3b; thiscall, ret 4; retail-only
type_object::type_object(TRmgObjectPropertiesRef* newProperties)
{
    m_properties = newProperties;
    ++m_properties->m_refCount;
    m_position.m_x = -1;
    m_position.m_y = -1;
    m_position.m_z = -1;
    clearPlacementMarks();
}

// The base-sized default-payload classes retain separate serialization
// vtables. Their ordinary constructors expand the same canonical base call.
rmgResourceObject::rmgResourceObject(TRmgObjectPropertiesRef* properties)
    : type_object(properties)
{
}

rmgScholarObject::rmgScholarObject(TRmgObjectPropertiesRef* properties)
    : type_object(properties)
{
}

rmgShrineObject::rmgShrineObject(TRmgObjectPropertiesRef* properties)
    : type_object(properties)
{
}

rmgWitchHutObject::rmgWitchHutObject(TRmgObjectPropertiesRef* properties)
    : type_object(properties)
{
}

// The three simple reward factories expand this same constructor. The
// vector's automatic construction precedes these scalar/default writes.
rmgBlackBoxObject::rmgBlackBoxObject(TRmgObjectPropertiesRef* properties)
    : type_object(properties)
{
    m_creatureType = -1;
    m_creatureCount = 0;
    m_experience = 0;
    memset(m_resources, 0, sizeof(m_resources));
}

// Base-object vtable 0x640a74 slot 0 retains the generated deleting wrapper;
// its non-deleting half is the shared refcount release at 0x5338d0.
VA_COMPGEN(0x00533120, 0x2D, SCALAR_DELETING_DTOR, type_object)

// Retained reset at 0x533150; its expansion also ends the preceding ctor.
// Preserve the ordinary helper's retail order after that constructor.
VA(0x00533150, 0x12) // five placement marks, thiscall, ret 0; retail-only
void type_object::clearPlacementMarks()
{
    m_candidateCovers = 0;
    m_candidateBehind = 0;
    m_adjacentToCandidate = 0;
    m_overlapsCandidate = 0;
    m_blockedByCandidate = 0;
}

// Base-object vtable 0x640a74 slot 3. Retail writes three coordinate bytes,
// the four-byte prototype index, then five zero bytes. The ownable writer
// expands this ordinary method before appending its own serialization.
// Scoped buffers preserve the observed independent narrow-value lifetimes.
// Both retained base (121 bytes) and derived (160 bytes) match exactly with
// this ordinary definition visible; no inline controls or duplicated body.
VA(0x00533170, 0x79) // anchor-vtable 0x640a74+0x0c; retail-only, ret 8
void type_object::write(TAbstractFile* outfile, int parameter)
{
    {
        char byteBuffer = m_position.m_x;
        outfile->write(&byteBuffer, sizeof(byteBuffer));
    }
    {
        char byteBuffer = m_position.m_y;
        outfile->write(&byteBuffer, sizeof(byteBuffer));
    }
    {
        char byteBuffer = m_position.m_z;
        outfile->write(&byteBuffer, sizeof(byteBuffer));
    }
    {
        int intBuffer = m_properties->m_prototypeIndex;
        outfile->write(&intBuffer, sizeof(intBuffer));
    }
    char reserved[5];
    memset(reserved, 0, sizeof(reserved));
    outfile->write(reserved, sizeof(reserved));
}

// Monster vtable 0x640a84 slot 3. Retail expands the canonical base writer,
// gates the id on AB-or-newer format, then emits count/disposition followed
// by three distinct byte writes and a word. +0x28 is not serialized here.
// Exact 253 bytes with a short count buffer. An int buffer leaves a dword
// load at +0x8b (99.9550%); both signed/unsigned short buffers recover the
// word load and VC6's coalesced dword stack store. Preserve all twelve calls.
VA(0x005331F0, 0xFD) // anchor-vtable 0x640a84+0x0c; thiscall, ret 8
void rmgMonsterObject::write(TAbstractFile* outfile, int version)
{
    type_object::write(outfile, version);
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
        int intBuffer = m_objectId;
        outfile->write(&intBuffer, sizeof(intBuffer));
    }
    {
        short intBuffer = m_count;
        outfile->write(&intBuffer, sizeof(short));
    }
    {
        char byteBuffer = m_disposition;
        outfile->write(&byteBuffer, sizeof(byteBuffer));
    }
    {
        char byteBuffer = 0;
        outfile->write(&byteBuffer, sizeof(byteBuffer));
    }
    {
        char byteBuffer = 0;
        outfile->write(&byteBuffer, sizeof(byteBuffer));
    }
    {
        char byteBuffer = 0;
        outfile->write(&byteBuffer, sizeof(byteBuffer));
    }
    {
        int intBuffer = 0;
        outfile->write(&intBuffer, sizeof(short));
    }
}

// Town-object vtable 0x640a94 slot 3; fields established by town placement.
// Exact: 362 bytes. Preserve the canonical base writer, distinct byte
// fields and version gates. memset for the nine/three-byte buffers restores
// aligned stores and keeps version in EBX; aggregate {0} initialization
// instead hoists a zero register and scores 91.0455%.
VA(0x005332F0, 0x16A)
void rmgTownObject::write(TAbstractFile* outfile, int version)
{
    type_object::write(outfile, version);
    if (version >= 1) {
        int value = m_objectId;
        outfile->write(&value, sizeof(value));
    }
    {
        char value = m_player;
        outfile->write(&value, sizeof(value));
    }
    {
        char value = 0;
        outfile->write(&value, sizeof(value));
    }
    {
        char value = 0;
        outfile->write(&value, sizeof(value));
    }
    {
        char value = 0;
        outfile->write(&value, sizeof(value));
    }
    {
        char value = 0;
        outfile->write(&value, sizeof(value));
    }
    {
        char value = m_townOption;
        outfile->write(&value, sizeof(value));
    }
    char spells[9];
    memset(spells, 0, sizeof(spells));
    if (version >= 1)
        outfile->write(spells, sizeof(spells));
    outfile->write(spells, sizeof(spells));
    {
        int value = 0;
        outfile->write(&value, sizeof(value));
    }
    if (version >= 2) {
        char value = -1;
        outfile->write(&value, sizeof(value));
    }
    char reserved[3];
    memset(reserved, 0, sizeof(reserved));
    outfile->write(reserved, sizeof(reserved));
}

// Ownable-object vtable 0x640aa4 slot 3. Preserve the canonical base call;
// retail expands it, then writes unowned player 0xff and three zero bytes.
VA(0x00533460, 0xA0) // base serialization plus unowned player and reserved bytes
void rmgOwnableObject::write(TAbstractFile* outfile, int parameter)
{
    type_object::write(outfile, parameter);
    {
        char player = -1;
        outfile->write(&player, sizeof(player));
    }
    char reserved[3];
    memset(reserved, 0, sizeof(reserved));
    outfile->write(reserved, sizeof(reserved));
}

// Artifact vtable 0x640ab4 is the only change from the base constructor.
// Its factory at 0x5341f0 expands this ordinary constructor and the canonical
// base body; no independent retained artifact-constructor address is claimed.
rmgArtifactObject::rmgArtifactObject(TRmgObjectPropertiesRef* properties)
    : type_object(properties)
{
}

rmgSeerHutObject::rmgSeerHutObject(TRmgObjectPropertiesRef* properties)
    : type_object(properties)
{
    m_experience = 0;
    m_artifact = -1;
    m_resourceType = 6;
    m_resourceCount = 0;
    m_creatureType = -1;
    m_creatureCount = 0;
}

rmgQuestArtifactObject::rmgQuestArtifactObject(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, rmgSeerHutObject* seerHut,
    type_treasure_def* definition)
    : rmgArtifactObject(properties), m_generator(generator),
      m_seerHut(seerHut), m_definition(definition)
{
}

rmgKeyTentObject::rmgKeyTentObject(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, int value)
    : type_object(properties), m_generator(generator), m_value(value)
{
}

// The artifact record is the ordinary object record followed by the empty
// custom-treasure flag consumed by NewfullMap::readArtifactData. Retail
// 0x533500 retains the same five base writes before
// the final one-byte zero at 0x53357b..0x53357f.
// Exact: a block-scoped flag reuses the dead argument byte at [ebp+0xb].
// The former function-scoped flag occupied [ebp-2] (99.8033%); byte type
// and split initialization alone are flat. The joint 60-case buffer family
// closes all four simple payload writers in 18 states without collateral.
VA(0x00533500, 0x8A) // anchor-vtable 0x640ab4 slot 3; thiscall ret 8; retail-only
void rmgArtifactObject::write(TAbstractFile* outfile, int parameter)
{
    type_object::write(outfile, parameter);
    {
        char hasCustomTreasure = 0;
        outfile->write(&hasCustomTreasure, sizeof(hasCustomTreasure));
    }
}

VA_COMPGEN(0x00533590, 0x21, SCALAR_DELETING_DTOR, rmgOwnableObject)

// Resource vtable 0x640ac4 appends a zero custom-treasure flag, a zero
// resource count and a reserved dword to the canonical object record.
// Preserve all three writes, including the second independent dword zero.
// Exact with independent buffer scopes: retail reuses [ebp+0xb]/[ebp+8].
// Flat lifetimes allocate a 0x14 frame instead of 8 bytes (99.4933%);
// one shared tail scope only reaches 99.6533%. All calls already agreed.
VA(0x005335C0, 0xB2) // anchor-vtable 0x640ac4 slot 3; thiscall ret 8
void rmgResourceObject::write(TAbstractFile* outfile, int parameter)
{
    type_object::write(outfile, parameter);
    {
        char hasCustomTreasure = 0;
        outfile->write(&hasCustomTreasure, sizeof(hasCustomTreasure));
    }
    {
        int amount = 0;
        outfile->write(&amount, sizeof(amount));
    }
    {
        int reserved = 0;
        outfile->write(&reserved, sizeof(reserved));
    }
}

VA_COMPGEN(0x00533680, 0x21, SCALAR_DELETING_DTOR, rmgBlackBoxObject)

// Vtable 0x640ad4's deleting wrapper calls this retained implicit destructor.
// Its vector cleanup is followed by the canonical base's property release.
// An explicit empty override adds an absent derived-vptr store (95.00%);
// retail has only automatic member/base teardown, as in the ownable class.
VA_COMPGEN(0x005336B0, 0x36, IMPLICIT_DTOR, rmgBlackBoxObject)

// Pandora's Box writer: ordinary object header, empty message/guard flag,
// experience, mana, morale/luck, resources, primary/secondary skills,
// artifacts, spells and creature reward, then eight reserved bytes.
// Retail preserves these individual file writes and version-dependent
// creature width; the spell count and loop bound come from the real vector.
// Exact: the creature-count buffer needs its own block after the type's
// version arms. A 24-candidate scope/load batch leaves the unscoped buffer
// at 99.9517% (different stack slot); signed and unsigned short blocks match.
VA(0x005336F0, 0x1E0) // anchor-vtable 0x640ad4 slot 3; ret 8; retail-only
void rmgBlackBoxObject::write(TAbstractFile* outfile, int version)
{
    type_object::write(outfile, version);
    {
        char hasCustomTreasure = 0;
        outfile->write(&hasCustomTreasure, sizeof(hasCustomTreasure));
    }
    {
        int experience = m_experience;
        outfile->write(&experience, sizeof(experience));
    }
    {
        int mana = 0;
        outfile->write(&mana, sizeof(mana));
    }
    {
        char morale = 0;
        outfile->write(&morale, sizeof(morale));
    }
    {
        char luck = 0;
        outfile->write(&luck, sizeof(luck));
    }
    outfile->write(m_resources, sizeof(m_resources));
    {
        int primarySkills = 0;
        outfile->write(&primarySkills, sizeof(primarySkills));
    }
    {
        char secondarySkillCount = 0;
        outfile->write(&secondarySkillCount, sizeof(secondarySkillCount));
    }
    {
        char artifactCount = 0;
        outfile->write(&artifactCount, sizeof(artifactCount));
    }
    {
        char spellCount = m_spells.size();
        outfile->write(&spellCount, sizeof(spellCount));
    }
    for (unsigned int i = 0; i < m_spells.size(); ++i) {
        char spell = m_spells[i];
        outfile->write(&spell, sizeof(spell));
    }
    if (m_creatureType == -1) {
        char creatureCount = 0;
        outfile->write(&creatureCount, sizeof(creatureCount));
    } else {
        {
            char creatureCount = 1;
            outfile->write(&creatureCount, sizeof(creatureCount));
        }
        if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
            short creatureType = m_creatureType;
            outfile->write(&creatureType, sizeof(creatureType));
        } else {
            char creatureType = m_creatureType;
            outfile->write(&creatureType, sizeof(creatureType));
        }
        {
            short creatureCount = m_creatureCount;
            outfile->write(&creatureCount, sizeof(creatureCount));
        }
    }
    {
        int reserved = 0;
        outfile->write(&reserved, sizeof(reserved));
    }
    {
        int reserved = 0;
        outfile->write(&reserved, sizeof(reserved));
    }
}

// Vptr restoration and the property reference release at 0x5338d0.
// Keep the body visible to the ownable destructor so the base cleanup can
// expand there. A separate TU leaves a five-byte derived tail-call thunk;
// an explicit empty derived destructor adds its own vptr store as well.
// With the body visible, base and ownable destructors have identical 13-byte
// bodies and the same base-vtable relocation, proving their ICF identity.
VA(0x005338D0, 0x0D) // anchor-callee 0x533596; retail-only, thiscall, ret 0
type_object::~type_object()
{
    --m_properties->m_refCount;
}

// Key-tent vtable 0x640ae4 slot 2 first tries to place a same-color guard.
// Failure removes this tent from the map and requests replacement treasure
// in the original zone and position. Keep the generator/value/position
// snapshots across removeObject: retail preserves all three before that call.
VA(0x005338E0, 0xD4) // anchor-vtable + retained placement/removal calls; retail-only
unsigned char rmgKeyTentObject::isWritable()
{
    if (m_generator->placeKeyTentGuard(this, m_value * 3 / 2))
        return 1;
    type_random_map_generator* generator = m_generator;
    int value = m_value;
    TRmgMapPosition position = m_position;
    generator->removeObject(this);
    TRmgZone* zone = generator->m_zones[
        generator->m_map.getMapItem(position)->m_zoneState.m_zone];
    int actualValue;
    type_object* object = generator->generateTreasure(
        zone, value, value * 3 / 2, &actualValue, 0, 0, 0, position);
    if (object)
        generator->addObject(object, position);
    return 0;
}

// Vtable 0x640af4 owns a pending polymorphic seer-hut object. The retained
// destructor deletes it before the ordinary artifact/base property release.
VA_COMPGEN(0x005339C0, 0x21, SCALAR_DELETING_DTOR, rmgQuestArtifactObject)
VA(0x005339F0, 0x58) // anchor-vtable + polymorphic member delete; retail-only
rmgQuestArtifactObject::~rmgQuestArtifactObject()
{
    delete m_seerHut;
}

// The generator places the seer hut on success and takes ownership. On
// failure the wrapper destroys its pending hut. Both paths clear ownership.
VA(0x00533A50, 0x33) // vtable 0x640af4 slot 2 + retained callee 0x54b490
unsigned char rmgQuestArtifactObject::isWritable()
{
    if (m_generator->placeQuestArtifact(this)) {
        m_seerHut = 0;
        return 1;
    }
    delete m_seerHut;
    m_seerHut = 0;
    return 0;
}

// Vtable 0x640b04 serializes the artifact quest followed by exactly one
// reward: experience, creatures, or resources, in that precedence order.
// AB adds the quest kind, artifact count, deadline and three empty strings.
VA(0x00533A90, 0x1E0) // anchor-vtable + ordered H3M writes; retail-only
void rmgSeerHutObject::write(TAbstractFile* outfile, int version)
{
    type_object::write(outfile, version);
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
        {
            char questKind = 5;
            outfile->write(&questKind, sizeof(questKind));
        }
        {
            char artifactCount = 1;
            outfile->write(&artifactCount, sizeof(artifactCount));
        }
        {
            short artifact = m_artifact;
            outfile->write(&artifact, sizeof(artifact));
        }
        {
            int deadline = -1;
            outfile->write(&deadline, sizeof(deadline));
        }
        {
            int firstVisitLength = 0;
            outfile->write(&firstVisitLength, sizeof(firstVisitLength));
        }
        {
            int nextVisitLength = 0;
            outfile->write(&nextVisitLength, sizeof(nextVisitLength));
        }
        {
            int completionLength = 0;
            outfile->write(&completionLength, sizeof(completionLength));
        }
    } else {
        char artifact = m_artifact;
        outfile->write(&artifact, sizeof(artifact));
    }
    if (m_experience > 0) {
        {
            char rewardKind = 1;
            outfile->write(&rewardKind, sizeof(rewardKind));
        }
        {
            int experience = m_experience;
            outfile->write(&experience, sizeof(experience));
        }
    } else if (m_creatureType != -1) {
        {
            char rewardKind = 10;
            outfile->write(&rewardKind, sizeof(rewardKind));
        }
        if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
            short creature = m_creatureType;
            outfile->write(&creature, sizeof(creature));
        } else {
            char creature = m_creatureType;
            outfile->write(&creature, sizeof(creature));
        }
        {
            short count = m_creatureCount;
            outfile->write(&count, sizeof(count));
        }
    } else {
        {
            char rewardKind = 5;
            outfile->write(&rewardKind, sizeof(rewardKind));
        }
        {
            char resourceType = m_resourceType;
            outfile->write(&resourceType, sizeof(resourceType));
        }
        {
            int resourceCount = m_resourceCount;
            outfile->write(&resourceCount, sizeof(resourceCount));
        }
    }
    {
        int reserved = 0;
        outfile->write(&reserved, sizeof(short));
    }
}

// The hero-object factory marks the selected index in disabledHeroes before
// construction. Vtable 0x640b14 slot 1 clears that byte when the reservation
// is released.
VA(0x00533C70, 0x0F)  // factory 0x5348d0; Complete-only RMG object
void rmgHeroObject::unknownOperation()
{
    m_generator->m_disabledHeroes[m_heroIndex] = 0;
}

// Scholar vtable 0x640b24 writes the default reward tag/value, then six
// reserved bytes as a dword and word. Retail zeroes a full dword temporary
// before the final two-byte write; preserve that scalar width and call size.
// Exact: individual scopes recover the dead argument slots and 8-byte frame.
// Flat locals gave 99.4146%; one tail scope gives 99.6098%, and grouping
// both byte buffers together still leaves a slot difference (99.8537%).
VA(0x00533E70, 0xC3) // anchor-vtable + default serialization bytes; ret 8
void rmgScholarObject::write(TAbstractFile* outfile, int parameter)
{
    type_object::write(outfile, parameter);
    {
        char rewardKind = -1;
        outfile->write(&rewardKind, sizeof(rewardKind));
    }
    {
        char rewardValue = 0;
        outfile->write(&rewardValue, sizeof(rewardValue));
    }
    {
        int reserved = 0;
        outfile->write(&reserved, sizeof(reserved));
    }
    {
        int reserved = 0;
        outfile->write(&reserved, sizeof(short));
    }
}

// Shrine vtable 0x640b34 emits its default spell marker and three reserved
// bytes through byte/word/byte writes, after the ordinary object record.
// Exact: separate buffer scopes reuse [ebp+0xb]/[ebp+8]. Keeping all three
// locals function-scoped gives 99.4933%; one tail scope only reaches 99.7067%.
VA(0x00533F40, 0xAF) // anchor-vtable + ordered write sizes; ret 8
void rmgShrineObject::write(TAbstractFile* outfile, int parameter)
{
    type_object::write(outfile, parameter);
    {
        char spell = -1;
        outfile->write(&spell, sizeof(spell));
    }
    {
        int reserved = 0;
        outfile->write(&reserved, sizeof(short));
    }
    {
        char reservedByte = 0;
        outfile->write(&reservedByte, sizeof(reservedByte));
    }
}

// Witch-hut vtable 0x640b54 appends the default skill mask only in AB and
// later map versions. Retail uses a signed comparison against version 1.
// Exact with the shared base writer expanded and the conditional mask local.
VA(0x005340C0, 0x93) // anchor-vtable + version guard and mask 0xefdf; ret 8
void rmgWitchHutObject::write(TAbstractFile* outfile, int parameter)
{
    type_object::write(outfile, parameter);
    if (parameter >= RMG_MAP_ARMAGEDDONS_BLADE) {
        unsigned int allowedSkills = 0xefdf;
        outfile->write(&allowedSkills, sizeof(allowedSkills));
    }
}

// Complete-only helper called by InitializeObjectGenerators at 0x538b10.
// The four argument loads, five stores, vtable relocation, and `ret 0x10`
// independently prove this constructor and the shared 0x14-byte prefix.
VA(0x00534160, 0x27)
type_treasure_def::type_treasure_def(
    int newObjectType, int newSubtype, int newValue, int newDensity)
{
    m_objectType = newObjectType;
    m_subtype = newSubtype;
    m_value = newValue;
    m_density = newDensity;
}

// Complete-only RMG virtual recovered from the inherited slot in the
// type_treasure_def family of retail vtables; Dreamcast has no RMG compiland.
VA(0x00534190, 0x06)
int type_treasure_def::getValue(TRmgZone*, type_random_map_generator*)
{
    return m_value;
}

// Vtable 0x640b64 slot 0 is an object factory, not a constructor. Retail
// allocates 0x1c bytes and expands type_object's canonical constructor using
// the first explicit argument as TRmgObjectPropertiesRef*. All 77 bytes
// match with that ordinary constructor call. The remaining two
// interface arguments are unused. The allocated object's vtable is 0x640a74.
VA(0x005341A0, 0x4D) // anchor-vtable 0x640b64 + base-object construction; retail-only
type_object* type_treasure_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new type_object(properties);
}

// Exact: the ordinary constructor chain reproduces all 83 retail bytes.
// Artifact-definition vtable 0x640b70 slot 0 allocates the base-sized
// artifact class and installs its distinct serialization vtable 0x640ab4.
VA(0x005341F0, 0x53) // anchor-vtable + allocated-object vptr; retail-only
type_object* type_artifact_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new rmgArtifactObject(properties);
}

// The compiler expands the common four-store constructor in each of these
// derived definitions; retail retains only the derived vptr store.  This is
// ordinary /Ob2 expansion of a real helper boundary, not a hand-flattened
// substitute for that boundary.
VA(0x00534250, 0xB5)
type_black_box_creature_def::type_black_box_creature_def(int newCreatureType)
    : type_treasure_def(6, 0, -1, 3),
      m_creatureType(newCreatureType)
{
    m_adjustedValue =
        g_rmgCreatureValueByLevel[g_creatureTypeTraits[newCreatureType].m_level]
        / g_creatureTypeTraits[newCreatureType].m_aiValue;

    if (m_adjustedValue > 50)
        m_adjustedValue = ((m_adjustedValue + 5) / 10) * 10;
    else if (m_adjustedValue > 12)
        m_adjustedValue = ((m_adjustedValue + 2) / 5) * 5;
    else if (m_adjustedValue > 5)
        m_adjustedValue = ((m_adjustedValue + 1) / 2) * 2;
}

// Creature-definition vtable 0x640b7c slot 1. The zone test reads +8
// (townType2), and the alignment weighting uses the generator's active-zone
// counts. These offsets distinguish both arguments from the old placeholders.
VA(0x00534310, 0x64) // anchor-vtable + creature traits 0x6747b0; retail-only
int type_black_box_creature_def::getValue(
    TRmgZone* zone, type_random_map_generator* generator)
{
    int alignment = g_creatureTypeTraits[m_creatureType].m_townType;
    if (alignment != zone->m_townType2)
        return -1;
    int value = g_creatureTypeTraits[m_creatureType].m_aiValue * m_adjustedValue;
    int alignmentCount = 0;
    if (alignment != -1)
        alignmentCount = generator->m_activeZoneCountsByAlignment[alignment];
    int zoneCount = generator->m_activeZoneCount;
    if (zoneCount > 0)
        value += alignmentCount * value / zoneCount;
    return value;
}

// The three definition tables select the same concrete Pandora's Box
// class. They initialize its defaults through the canonical constructor,
// then supply the definition-specific creature, experience or gold reward.
// All three factories are exact. For the creature payload, retail loads
// both definition fields before either object store: the count local closes
// the direct-store form's 99.24% scheduling residual in the payload batch.
VA(0x00534380, 0x85) // anchor-vtable 0x640b7c slot 0; ret 0xc; retail-only
type_object* type_black_box_creature_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    rmgBlackBoxObject* object = new rmgBlackBoxObject(properties);
    int count = m_adjustedValue;
    object->m_creatureType = m_creatureType;
    object->m_creatureCount = count;
    return object;
}

VA(0x00534410, 0x7F) // anchor-vtable 0x640b88 slot 0; ret 0xc; retail-only
type_object* type_black_box_experience_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    rmgBlackBoxObject* object = new rmgBlackBoxObject(properties);
    object->m_experience = m_experience;
    return object;
}

VA(0x00534490, 0x84) // anchor-vtable 0x640b94 slot 0; ret 0xc; retail-only
type_object* type_black_box_gold_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    rmgBlackBoxObject* object = new rmgBlackBoxObject(properties);
    object->m_resources[6] += m_gold;
    return object;
}

// Both dwelling-definition tables (0x640bac/0x640bb8) share this factory.
// Its allocation and base initialization match the ordinary factory, followed
// by the proven ownable-object vptr 0x640aa4. All 83 bytes match while
// preserving the real constructor.
VA(0x00534790, 0x53) // anchor-vtables + ownable constructor expansion; retail-only
type_object* type_dwelling_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new rmgOwnableObject(properties);
}

// Vtable 0x640bb8 slot 1 identifies the map-dwelling valuation override.
// Complete-only: creature-generator subtype table 0x63d570, trait stride
// 0x74, and generator zone counts at +0xf60/+0xf64 prove the operands.
// Exact: all 121 retail bytes match, including both signed divisions.
VA(0x005347F0, 0x79)
int type_map_dwelling_def::getValue(TRmgZone* zone, type_random_map_generator* generator)
{
    const TCreatureTypeTraits& creature =
        g_creatureTypeTraits[g_creatureGenerator1Types[m_subtype]];
    if (creature.m_townType != zone->m_townType2)
        return -1;

    int value = creature.m_growthRate * creature.m_aiValue;
    int zoneCount = 0;
    if (creature.m_townType != -1)
        zoneCount = generator->m_activeZoneCountsByAlignment[creature.m_townType];
    if (generator->m_activeZoneCount > 0)
        value += zoneCount * value / generator->m_activeZoneCount;
    return value + creature.m_aiValue * zoneCount / 2;
}

// Resource-definition table 0x640bc4 constructs the base-sized resource
// object and replaces its vptr with 0x640ac4 after the canonical base call.
// This and the scholar/shrine/witch-hut factories reproduce all 83 bytes.
VA(0x00534870, 0x53) // anchor-definition table + allocated-object vptr; ret 0xc
type_object* type_resource_lump_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new rmgResourceObject(properties);
}

// Scholar-definition table 0x640bdc selects object vtable 0x640b24.
VA(0x00534970, 0x53) // anchor-definition and object vtables; ret 0xc
type_object* type_scholar_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new rmgScholarObject(properties);
}

VA(0x005349D0, 0x29)
type_shrine_def::type_shrine_def(int newObjectType, int newValue)
    : type_treasure_def(newObjectType, 0, newValue, 100)
{
}

// Shrine-definition table 0x640be8 selects object vtable 0x640b34.
VA(0x00534A00, 0x53) // anchor-definition and object vtables; ret 0xc
type_object* type_shrine_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new rmgShrineObject(properties);
}

VA(0x00534A60, 0x25)
type_witch_hut_def::type_witch_hut_def()
    : type_treasure_def(0x71, 0, 1500, 80)
{
}

// Witch-hut definition 0x640bf4 selects object vtable 0x640b54.
VA(0x00534A90, 0x53) // anchor-definition and object vtables; ret 0xc
type_object* type_witch_hut_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new rmgWitchHutObject(properties);
}

// Seer-hut definition tables 0x640c0c and 0x640c18 share this ICF body.
// Both classes exist independently and use the same availability checks:
// current prototype at +0xf58, then the exhausted-artifact flag at +0x10b4.
VA(0x00534C80, 0x34) // anchor-vtables + generator fields; retail-only
int type_quest_experience_def::getValue(TRmgZone*, type_random_map_generator* generator)
{
    if (generator->m_nextSeerHutPrototypeIndex != m_subtype)
        return -1;
    if (generator->m_questArtifactPoolLow)
        return -1;
    return m_value;
}

int type_quest_gold_def::getValue(TRmgZone*, type_random_map_generator* generator)
{
    if (generator->m_nextSeerHutPrototypeIndex != m_subtype)
        return -1;
    if (generator->m_questArtifactPoolLow)
        return -1;
    return m_value;
}

// The reward is carried by a pending seer hut, while the returned object
// is its artifact wrapper. Both allocations use their canonical constructors.
// Exact: a 24-candidate batch requires all three wrapper fields in the
// initializer list, so their stores precede the final derived vptr. Body
// assignments leave experience at 99.5432%. The gold amount local also
// preserves retail's reward load before either field store (direct: 95.122%).
VA(0x00534CC0, 0xE1) // definition vtable 0x640c0c slot 0; retail-only
type_object* type_quest_experience_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, TRmgZone*)
{
    rmgSeerHutObject* seerHut = new rmgSeerHutObject(properties);
    TRmgObjectPropertiesRef* artifact = generator->selectObjectPrototype(eTerrainDirt, 0x41, 0);
    rmgQuestArtifactObject* object = new rmgQuestArtifactObject(artifact, generator, seerHut, this);
    seerHut->m_experience = m_experience;
    return object;
}

VA(0x00534DB0, 0xE8) // definition vtable 0x640c18 slot 0; retail-only
type_object* type_quest_gold_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, TRmgZone*)
{
    rmgSeerHutObject* seerHut = new rmgSeerHutObject(properties);
    TRmgObjectPropertiesRef* artifact = generator->selectObjectPrototype(eTerrainDirt, 0x41, 0);
    rmgQuestArtifactObject* object = new rmgQuestArtifactObject(artifact, generator, seerHut, this);
    int amount = m_gold;
    seerHut->m_resourceType = 6;
    seerHut->m_resourceCount = amount;
    return object;
}

VA(0x00534EA0, 0x30)
type_spell_scroll_def::type_spell_scroll_def(int newSpellLevel, int newValue)
    : type_treasure_def(0x5d, 0, newValue, 30)
{
    m_spellLevel = newSpellLevel;
}

// Vtable 0x640c30 slot 1 belongs to type_key_tent_def. The key-tent
// registration loop stores the color in m_subtype, and retail returns this
// definition's value only while that color is the generator's next free one.
VA(0x00534FA0, 0x21)
int type_key_tent_def::getValue(TRmgZone*, type_random_map_generator* generator)
{
    if (generator->m_nextKeyTentColor != m_subtype)
        return -1;
    return m_value;
}

// Definition vtable 0x640c30 slot 0 constructs the concrete 0x24-byte tent.
// The generator is the second factory argument; +0x20 receives m_value,
// not the definition subtype/color (that is already in its properties).
VA(0x00534FD0, 0x64) // anchor-definition table + object vtable 0x640ae4; retail-only
type_object* type_key_tent_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, TRmgZone*)
{
    return new rmgKeyTentObject(properties, generator, m_value);
}

// Retail's derived generator constructor 0x537b10 calls this six-argument
// base initializer. Automatic member construction owns the map, object table
// and 232 property vectors before the progress total and seed are installed.
// TProgressSink's +4 total/slot-1 setter prove the earlier opaque progress
// interface was a duplicate model of the existing canonical sink.
// Integration collateral: exposing the recovered owned-map constructor at
// 0x530fb0 makes VC6 expand it here (48.0526% CUR versus the retained retail
// call at 0x53609f). The unchanged constructor keeps its 100% MAX/HIST.
// Preserve the canonical body and retail source order while recovering the
// remaining caller/TU context; hiding it would discard proven source work.
VA(0x00536070, 0xFB) // caller 0x537b52 + base vtable 0x640c3c; retail-only
TRmgGeneratorBase::TRmgGeneratorBase(int width, int height, int levels,
    TProgressSink* progress, int additionalSteps, int version)
    : m_map(width, height, levels)
{
    m_progress = progress;
    m_mapVersion = version;
    if (progress)
        progress->setTotal(progress->m_steps + additionalSteps + 0x3bc4);
    time(&m_randomSeed);
    srand(m_randomSeed);
    loadObjectPrototypes();
}

VA_COMPGEN(0x00536170, 0x21, SCALAR_DELETING_DTOR, TRmgGeneratorBase)

// The seven-slot abstract map table at 0x6409e8 and sixteen retail cleanup
// tails identify this virtual base destructor, exact on the first scored
// candidate. Dreamcast has no RMG compiland.
VA(0x005361A0, 0x07)
TRmgMapInterface::~TRmgMapInterface()
{
}

// Vtable 0x6409e8 slot 0 retains this generated wrapper immediately after
// the exact abstract-base destructor in retail link order.
VA_COMPGEN(0x005361B0, 0x23, SCALAR_DELETING_DTOR, TRmgMapInterface)

// The base constructor passes this default-constructor closure to the
// 232-element EH vector constructor iterator. The int-vector grid below
// has its own identical retail closure at 0x536ba0.
VA_COMPGEN(0x005361E0, 0x18, DEFAULT_CTOR_CLOSURE, TRmgObjectPropertiesRef)

// The loader inlines construction of each reference. Its owned outline
// vector is constructed before the scalar stores below; the 8x6 priority
// table is left uninitialized until the lazy builder sets its flag.
TRmgObjectPropertiesRef::TRmgObjectPropertiesRef(TObjectType* prototype)
{
    m_prototype = prototype;
    m_placementRule = 0;
    m_refCount = 0;
    m_prototypeIndex = 0;
    m_preferredTerrain = -1;
    m_prioritiesInitialized = 0;
}

// Retail calls TObjectTypeTable::load on +0x24, then filters by map version
// and remaps object categories through trait-row +8. The final nested loop
// swaps prototype pointers (not owning references) in the creature bucket.
// Residual (99.5878%): filtering address/register allocation differs; all
// source calls and branch paths agree. Named left/right records with a
// manual swap or std::swap reach 94.3243%; indexed manual swap 96.8581%;
// indexed std::swap 99.4527%. Reusing the category local for its remapped
// value reaches 99.5878%. Pointer versus reference record locals are flat.
VA(0x00536200, 0x1AC) // anchor-callee 0x536152 + objects.txt and base fields; retail-only
void TRmgGeneratorBase::loadObjectPrototypes()
{
    m_objectsTxt.load("objects.txt");
    for (unsigned int index = 0; index < m_objectsTxt.m_objectTypes.size(); ++index) {
        TObjectType& object = m_objectsTxt.m_objectTypes[index];
        int type = object.m_objectType;
        if (m_mapVersion < 2 && type >= 222)
            continue;
        if (m_mapVersion < 1 && type >= 165)
            continue;
        if (m_mapVersion < 2 && (type == LITH_TWOWAY || type == LITH_ONEWAY_ENTRANCE || type == LITH_ONEWAY_EXIT)
            && object.m_subtype >= 3)
            continue;
        if (type < 0 || type >= 232)
            continue;
        TRmgObjectPropertiesRef* properties =
            new TRmgObjectPropertiesRef(&m_objectsTxt.m_objectTypes[index]);
        memcpy(&type, &g_adventureObjectLandBlocked[type][8], sizeof(type));
        m_objectPrototypes[type].push_back(properties);
    }
    for (unsigned int first = 0; first < m_objectPrototypes[54].size() - 1; ++first) {
        for (unsigned int second = first + 1; second < m_objectPrototypes[54].size(); ++second) {
            if (m_objectPrototypes[54][first]->m_prototype->m_subtype > m_objectPrototypes[54][second]->m_prototype->m_subtype) {
                std::swap(m_objectPrototypes[54][first]->m_prototype, m_objectPrototypes[54][second]->m_prototype);
            }
        }
    }
    readObjectPlacementRules();
    if (m_progress)
        m_progress->advance(15300);
}

// Retail first deletes every placed object through its virtual destructor,
// then deletes prototype references in 232 vectors. Each reference owns its
// outline vector at +0x14, which the implicit destructor releases inline.
// The remaining member cleanup is automatic, ending with the owned map.
// Exact: all 425 bytes and 20 CFG blocks, including nested vector/map cleanup.
VA(0x005363B0, 0x1A9) // anchor-callee 0x537fda + base-owned member cleanup; retail-only
TRmgGeneratorBase::~TRmgGeneratorBase()
{
    for (unsigned int object = 0; object < m_positions.size(); ++object)
        delete m_positions[object];
    for (int type = 0; type < 232; ++type)
        for (unsigned int prototype = 0; prototype < m_objectPrototypes[type].size(); ++prototype)
            delete m_objectPrototypes[type][prototype];
}

// rand_trn.txt supplies one rule per nonempty row starting at row three.
// The two 232x10 vector grids group rules and subtypes by remapped object
// type and preferred terrain. The final reverse scan gives later rows
// precedence for the same subtype. Names are provisional; RMG is absent
// from the Dreamcast build. Retail fixes the record stride at 0x4c.
// Residual (99.7862%): the rule push_back expands single-value insertion
// into the count overload, adding one push of 1 where retail calls the
// retained single-value wrapper at 0x536701. The checked bitset subscript
// recovers the exception string constructor and the rule's stack homes;
// direct .test is the 97.5804% negative control.
// A 48-case append/mask/row-scope/scalar-lifetime matrix and a 32-case
// independent scalar-append follow-up retain all five exact rule-vector
// bodies below. Two scalar insert(end(),value) calls with the remaining
// push_back yield reader 99.5723%; all three direct insertions give 97.3075%.
// These ordinary public calls recover the parent and child boundaries
// without changing a library definition. Bank those unchanged-source MAXs
// separately, then retain this higher reader peak. A 54-case count-one
// insertion follow-up does not improve either reader form. No pin remains.
// Earlier source controls: initialize row before the vectors, increment it
// before rule destruction, and explicitly zero both resize calls. Reuse
// objectType/subtype/terrain across parsing and binding to preserve escaped
// homes; the old-count reverse scan matches retail. An index-taking rule
// constructor instead shifts the EH state past the id assignment, so keep
// default construction followed by assignment. Signed/unsigned prototype
// indices and combined/nested reverse-loop conditions are byte-neutral.
// Removed inline-depth diagnostics (1 at push_back, 2 at .test) were also
// byte-neutral at 97.5804%; the canonical library definitions stay in use.
VA(0x00536560, 0x5F2) // anchor-string rand_trn.txt; thiscall, ret 0; retail-only
void TRmgGeneratorBase::readObjectPlacementRules()
{
    TSpreadsheetResource* sheet = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x006827F4, rmgPlacementRulesFilename, "rand_trn.txt"));
    int row = 3;
    std::vector<int> objectTypes;
    std::vector<int> terrains;
    std::vector<int> subtypes;
    int objectType;
    int subtype;
    int terrain;
    for (; row < sheet->getNumberOfRows();) {
        const TSpreadsheetResource::TStringVector& values = sheet->getRow(row);
        if (values[0][0] == ' ' || values[0][0] == 0)
            break;
        TRmgObjectPlacementRule rule;
        rule.m_index = row - 3;
        objectType = atoi(values[3]);
        subtype = atoi(values[4]);
        terrain = atoi(values[6]);
        objectTypes.push_back(objectType);
        terrains.push_back(terrain);
        subtypes.push_back(subtype);
        for (terrain = 0; terrain <= eTerrainWater; ++terrain)
            rule.m_terrainScores[terrain] = atoi(values[terrain + 7]);
        for (; terrain < 10; ++terrain)
            rule.m_terrainScores[terrain] = RMG_PLACEMENT_INVALID;
        m_placementRules.push_back(rule);
        ++row;
    }
    int ruleCount = m_placementRules.size();
    for (row = 3; row < ruleCount + 3; ++row) {
        const TSpreadsheetResource::TStringVector& values = sheet->getRow(row);
        TRmgObjectPlacementRule& rule = m_placementRules[row - 3];
        rule.m_adjacentScores.resize(ruleCount, 0);
        for (int index = 0; index < ruleCount; ++index)
            rule.m_adjacentScores[index] = atoi(values[index + 16]);
        rule.m_blockedScores.resize(ruleCount, 0);
        for (index = 0; index < ruleCount; ++index)
            rule.m_blockedScores[index] = atoi(values[index + ruleCount + 16]);
    }
    sheet->dispose();

    std::vector<TRmgObjectPlacementRule*> rulesByType[ADVENTURE_OBJECT_TRAIT_COUNT][10];
    std::vector<int> subtypesByType[ADVENTURE_OBJECT_TRAIT_COUNT][10];
    for (int index = 0; index < ruleCount; ++index) {
        TRmgObjectPlacementRule* rule = &m_placementRules[index];
        rulesByType[objectTypes[index]][terrains[index]].push_back(rule);
        subtypesByType[objectTypes[index]][terrains[index]].push_back(subtypes[index]);
    }
    for (objectType = 0; objectType < ADVENTURE_OBJECT_TRAIT_COUNT; ++objectType) {
        for (int index = 0; index < m_objectPrototypes[objectType].size();
             ++index) {
            TRmgObjectPropertiesRef* properties = m_objectPrototypes[objectType][index];
            TObjectType* prototype = properties->m_prototype;
            properties->m_placementRule = 0;
            for (terrain = 0; terrain < eTerrainRock; ++terrain) {
                if (prototype->m_recommendedTerrainMask[terrain])
                    break;
            }
            properties->m_preferredTerrain = terrain;
            if (terrain != eTerrainRock) {
                subtype = prototype->m_subtype;
                int mappedType;
                // Same canonical byte table used by readObjectType: the
                // dword at +8 remaps aliases to their objnames.txt row.
                memcpy(&mappedType, &g_adventureObjectLandBlocked[objectType][8],
                       sizeof(mappedType));
                int match = rulesByType[mappedType][terrain].size();
                while (match-- && subtypesByType[mappedType][terrain][match] != subtype)
                    ;
                if (match >= 0)
                    properties->m_placementRule = rulesByType[mappedType][terrain][match];
            }
        }
    }
}

// Rank a footprint against terrain and already placed objects. The caller
// at 0x5375ff keeps only positive scores in its weighted candidate pool.
// This method temporarily marks affected objects and clears all five marks
// before returning. Names describe retail roles; there is no DC counterpart.
// Residual (84.68%): retail retains bitset<48>::test at 0x536ca3/0x536cc1
// and bitset<10>::test at 0x536d06; these subscript/conversion expansions
// still inline test and retain _Xran. Direct .test calls also expand the
// exception construction (70.55%). Provisional footprint wrappers reached
// 83.96% but did not recover those calls, so they were removed.
// A temporary inline_depth(0) diagnostic on just those three .test calls
// reached 91.29% before the clamp/accessor corrections; all pins are removed.
// That probe matched the opening draw/passability call sequence and left
// the gate extraction, clamp operands, neighbor-loop lowering and local
// homes divergent. Canonical min/max with (coordinate, bound) gives retail's
// compare polarity; the byte gate accessor gives its shr/test-byte form.
// The remaining neighbor loop is strength-reduced to pointers here while
// retail recomputes its array address. Do not infer source assertions from
// its redundant lea. The insert callee's widget* name is an ICF alias of
// this pointer-vector instantiation, not another inlining difference.
VA(0x00536BC0, 0x5F4) // anchor-callee 0x5375ff; thiscall, ret 0x10; retail-only
int TRmgGeneratorBase::scoreObjectPlacement(
    TRmgObjectPropertiesRef* properties, TRmgMapPosition position)
{
    TObjectType* prototype = properties->m_prototype;
    std::vector<type_object*> affected;
    unsigned char terrainSeen[10];
    memset(terrainSeen, 0, sizeof(terrainSeen));
    unsigned int marks[10][8];
    memset(marks, 0, sizeof(marks));
    for (unsigned int row = 0; row < prototype->getHeight(); ++row) {
        int y = position.m_y - row;
        if (y < 0 || y >= m_map.m_mapHeight)
            continue;
        for (unsigned int column = 0; column < prototype->getWidth(); ++column) {
            int x = position.m_x - column;
            if (x < 0 || x >= m_map.m_mapWidth)
                continue;
            if (!prototype->m_imageInfo.m_drawMask[
                    CObjectType::getBitPos(column, row)])
                continue;
            marks[column + 1][row + 1] |= RMG_PLACEMENT_OVERLAP;
            if (!prototype->m_passableMask[CObjectType::getBitPos(column, row)]) {
                marks[column + 1][row + 1] |= RMG_PLACEMENT_BLOCKED;
                TRmgMapItem* item = m_map.getMapItem(x, y, position.m_z);
                if (!prototype->m_terrainMask[item->m_tile.m_landType])
                    return RMG_PLACEMENT_INVALID;
                if (item->hasSubterraneanGate())
                    return RMG_PLACEMENT_INVALID;

                // Retail 0x536d34 overwrites the complete mark with one
                // before marking the surrounding area; retain that store.
                marks[column + 1][row + 1] = RMG_PLACEMENT_ADJACENT;
                terrainSeen[item->m_tile.m_landType] = 1;
                int firstRow = position.m_y - min(y + 1, m_map.m_mapHeight) + 1;
                int lastRow = position.m_y - max(y - 2, 0) + 1;
                int firstColumn = position.m_x - min(x + 1, m_map.m_mapWidth) + 1;
                int lastColumn = position.m_x - max(x - 2, 0) + 1;
                for (int nearColumn = firstColumn; nearColumn < lastColumn;
                     ++nearColumn) {
                    for (int nearRow = firstRow; nearRow < lastRow; ++nearRow)
                        marks[nearColumn][nearRow] |= RMG_PLACEMENT_ADJACENT;
                }
            }
        }
    }

    TRmgObjectPlacementRule* rule = properties->m_placementRule;
    int score = 0;
    unsigned char hasPositiveTerrain = 0;
    for (int terrain = 0; terrain < 10; ++terrain) {
        if (terrainSeen[terrain]) {
            score += rule->m_terrainScores[terrain];
            if (rule->m_terrainScores[terrain] > 0)
                hasPositiveTerrain = 1;
        }
    }
    if (score < RMG_PLACEMENT_MINIMUM_TERRAIN_SCORE)
        return score;
    if (!hasPositiveTerrain)
        return RMG_PLACEMENT_NO_TERRAIN_PREFERENCE;

    properties->buildOverlapPriorities();
    for (row = 0; row < prototype->getHeight() + 2; ++row) {
        int y = position.m_y + 1 - row;
        if (y < 0 || y >= m_map.m_mapHeight)
            continue;
        for (unsigned int column = 0; column < prototype->getWidth() + 2;
             ++column) {
            int x = position.m_x + 1 - column;
            if (x < 0 || x >= m_map.m_mapWidth)
                continue;
            unsigned int mark = marks[column][row];
            if (!mark)
                continue;
            TRmgMapItem* item = m_map.getMapItem(x, y, position.m_z);
            if (item->m_tileData.m_roadPassable && item->m_tile.m_landType != eTerrainRock)
                continue;
            int priority;
            if (mark & RMG_PLACEMENT_OVERLAP)
                priority = properties->m_overlapPriorities[column - 1][row - 1];
            for (int index = 0; index < static_cast<int>(item->m_objects.size());
                 ++index) {
                type_object* object = item->m_objects[index];
                unsigned char wasTouched = object->isPlacementTouched();
                if (mark & RMG_PLACEMENT_OVERLAP) {
                    object->m_properties->buildOverlapPriorities();
                    if (object->m_properties->m_overlapPriorities
                            [object->m_position.m_x - x][object->m_position.m_y - y]
                        <= priority)
                        object->m_candidateCovers = 1;
                    else
                        object->m_candidateBehind = 1;
                    object->m_overlapsCandidate = 1;
                }
                if (mark & RMG_PLACEMENT_ADJACENT)
                    object->m_adjacentToCandidate = 1;
                if (mark & RMG_PLACEMENT_BLOCKED)
                    object->m_blockedByCandidate = 1;
                if (!wasTouched && object->isPlacementTouched())
                    affected.push_back(object);
                if (object->m_candidateBehind && object->m_candidateCovers)
                    break;
            }
        }
    }

    for (unsigned int index = 0; index < affected.size(); ++index) {
        type_object* object = affected[index];
        if (object->m_blockedByCandidate) {
            if (!object->m_properties->m_placementRule)
                score = RMG_PLACEMENT_INVALID;
            else
                score += rule->m_blockedScores[object->m_properties->m_placementRule->m_index];
        } else if (object->m_adjacentToCandidate) {
            if (object->m_properties->m_placementRule)
                score += rule->m_adjacentScores[object->m_properties->m_placementRule->m_index];
        }
        if (object->m_candidateBehind && object->m_candidateCovers)
            score = RMG_PLACEMENT_INVALID;
        object->clearPlacementMarks();
    }
    return score;
}

// Map-decoration caller 0x537a59 passes a position value and progress share.
// The body uses base fields and virtual object insertion; ownership/name provisional.
#if 0 // @carcass
VA(0x005373A0, 0x53D)
void TRmgGeneratorBase::decorateMapCell(TRmgMapPosition position, int progressSteps)
{} // @stub
#endif

// Complete emits this ordinary by-value accessor once, then lets VC6 choose
// its boundary independently at each RMG call site.  The standalone body's
// The 12 argument bytes exclude a single position reference, but cannot
// distinguish a position value from three scalar coordinates. The overload
// identity is provisional. CreateSubterraneanGate retains its final two
// calls while expanding the earlier ones.
// Keep one coordinate-indexing formula in the scalar overload. Both this
// delegation and its direct arithmetic control retain all 39 retail bytes.
// The nested call changes other inlining decisions: CreateGroundConnection's
// first clear retains range erase, while CreateRiver's final map destruction
// calls its vector deleting destructor. Original delegation remains provisional.
VA(0x005378E0, 0x27)
TRmgMapItem* type_random_map::getMapItem(TRmgMapPosition point)
{
    return getMapItem(point.m_x, point.m_y, point.m_z);
}

// Retail 0x549c91 calls this base-prefix pass after coastal marking.
// Keep the canonical tile-field names: bits 26/27 and 25 are observed here,
// regardless of their additional roles in zone connection and road routing.
// Partial 91.13%: one shared position value preserves all three scans and
// removes the extra position constructor (separate scalar loops: 71.91%).
// Progress/placement branch order remains reversed; positive/negative first
// arms and an explicit skip-to-next-cell continue are byte-neutral.
VA(0x00537970, 0x199)
void TRmgGeneratorBase::decorateMap()
{
    TRmgMapPosition position;
    int count = 0;
    TRmgMapItem* item = m_map.m_mapItems;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z)
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y)
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++item)
                if (item->hasBorderObject())
                    ++count;
    if (!count)
        return;
    int progressSteps = 276300 / count;
    item = m_map.m_mapItems;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++item) {
                if (item->hasBorderObject()) {
                    if (!item->m_tileData.m_roadPassable || item->m_tile.m_landType == eTerrainRock) {
                        if (m_progress)
                            m_progress->advance(progressSteps);
                        continue;
                    }
                    decorateMapCell(position, progressSteps);
                }
            }
        }
    }
    item = m_map.m_mapItems;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++item) {
                if (!item->hasSubterraneanGate() && item->m_tileData.m_roadPassable
                    && item->m_tile.m_landType != eTerrainRock && !item->m_connection.m_present) {
                    item->m_tileData.m_borderObject = 0;
                    item->m_tileData.m_subterraneanGate = 1;
                }
            }
        }
    }
}

// Retail-only constructor: base and member initialization precede template
// loading. Hero eligibility uses bytes within the canonical attributes field;
// 0x537d11/+0x3a excludes special heroes, +0x38/+0x39 selects map-version availability.
VA(0x00537B10, 0x2A8)
type_random_map_generator::type_random_map_generator(
    int width, int height, int levels, int humanPlayers, int humanTeams,
    int computerPlayers, int computerTeams, int waterContent,
    int monsterStrength, TProgressSink* progress, int mapVersion)
    : TRmgGeneratorBase(width, height, levels, progress,
        width * height + 326900, mapVersion)
{
    m_nextObjectId = 1;
    m_questArtifactPoolLow = 0;
    m_waterContent = waterContent;
    m_monsterStrength = monsterStrength;
    m_humanPlayerCount = humanPlayers;
    m_humanTeamCount = humanTeams;
    m_computerPlayerCount = computerPlayers;
    m_computerTeamCount = computerTeams;
    if (m_waterContent == RMG_WATER_RANDOM)
        m_waterContent = rand() % 3;
    loadTemplates();
    if (m_templates.size()) {
        m_nextSeerHutPrototypeIndex = 0;
        memset(m_usedQuestArtifacts, 0, sizeof(m_usedQuestArtifacts));
        memset(m_disabledHeroes, 0, sizeof(m_disabledHeroes));
        memset(m_objectCountByType, 0, sizeof(m_objectCountByType));
        initializeObjectGenerators();
        for (int hero = 0; hero < 156; ++hero) {
            if (static_cast<unsigned char>(g_heroTraits[hero].m_attributes >> 16)
                || (m_mapVersion >= 1
                    ? !static_cast<unsigned char>(g_heroTraits[hero].m_attributes >> 8)
                    : !static_cast<unsigned char>(g_heroTraits[hero].m_attributes)))
                m_disabledHeroes[hero] = 1;
        }
        std::fill_n(g_rmgZoneObjectLimits, 232, 32000);
        std::fill_n(g_rmgMapObjectLimits, 232, 32000);
        for (int mapLimit = 30; mapLimit--;)
            g_rmgMapObjectLimits[g_rmgMapObjectLimitOverrides[mapLimit].m_objectType]
                = g_rmgMapObjectLimitOverrides[mapLimit].m_limit;
        for (int zoneLimit = 24; zoneLimit--;)
            g_rmgZoneObjectLimits[g_rmgZoneObjectLimitOverrides[zoneLimit].m_objectType]
                = g_rmgZoneObjectLimitOverrides[zoneLimit].m_limit;
        memset(m_fixedHumanPlayers, 0, sizeof(m_fixedHumanPlayers));
    }
}

VA_COMPGEN(0x00537DC0, 0x21, SCALAR_DELETING_DTOR, type_random_map_generator)

// Retail deletes zones, templates and treasure definitions in forward order,
// then lets the derived containers/string and the 0xed8-byte base unwind.
// The base call at 0x537fda follows string cleanup at +0x10c0; preserving
// inheritance avoids flattening the base's separate retained destructor.
// Exact: 512 bytes and all 17 blocks after restoring the base boundary;
// compiler-generated pointer-vector cleanup uses the retail ICF helpers.
VA(0x00537DF0, 0x200) // anchor-callee 0x54c032/0x54c076 + member cleanup; retail-only
type_random_map_generator::~type_random_map_generator()
{
    for (unsigned int zone = 0; zone < m_zones.size(); ++zone)
        delete m_zones[zone];
    for (unsigned int mapTemplate = 0; mapTemplate < m_templates.size(); ++mapTemplate)
        delete m_templates[mapTemplate];
    for (unsigned int definition = 0; definition < m_objectGenerators.size(); ++definition)
        delete m_objectGenerators[definition];
}

// Role-derived ordinary helpers: the retail coordinator snapshots player
// counts before each pass. Separate lifetimes preserve those parameter values
// and let VC6 choose the nested findZone/vector call boundaries.
static void readRmgTemplateConnections(const TSpreadsheetResource* sheet,
    TRmgTemplate* mapTemplate, int firstRow, int endRow,
    int humanPlayers, int computerPlayers)
{
    for (int connectionRow = firstRow; connectionRow < endRow; ++connectionRow) {
        const TSpreadsheetResource::TStringVector& fields = sheet->getRow(connectionRow);
        if (fields.size() > 84 && fields[76][0]
            && fields[76][0] != ' ' && fields[77][0]) {
            int firstZone = atoi(fields[76]);
            int secondZone = atoi(fields[77]);
            TRmgTownSlot* first = mapTemplate->findZone(firstZone);
            TRmgTownSlot* second = mapTemplate->findZone(secondZone);
            if (first && second) {
                TRmgZoneConnection connection;
                connection.m_destination = second;
                connection.m_value = atoi(fields[78]);
                connection.m_unguarded = fields[79][0] && fields[79][0] != ' ';
                connection.m_placeBorderObjects = fields[80][0] && fields[80][0] != ' ';
                connection.m_minimumHumanPlayers = atoi(fields[81]);
                connection.m_maximumHumanPlayers = atoi(fields[82]);
                connection.m_minimumPlayers = atoi(fields[83]);
                connection.m_maximumPlayers = atoi(fields[84]);
                connection.m_connected = 0;
                if (connection.m_minimumHumanPlayers <= humanPlayers
                    && connection.m_maximumHumanPlayers >= humanPlayers
                    && connection.m_minimumPlayers <= humanPlayers + computerPlayers
                    && connection.m_maximumPlayers >= humanPlayers + computerPlayers) {
                    first->m_connections.push_back(connection);
                    connection.m_destination = first;
                    second->m_connections.push_back(connection);
                }
            }
        }
    }
}

static bool hasRmgTemplatePlayerSlots(TRmgTemplate* mapTemplate,
    int humanPlayers, int computerPlayers)
{
    int playerSlots = 0;
    for (unsigned int slot = 0; slot < mapTemplate->m_zones.size(); ++slot)
        if (mapTemplate->m_zones[slot]->m_kind == RMG_TEMPLATE_HUMAN)
            ++playerSlots;
    if (playerSlots < humanPlayers)
        return false;
    for (slot = 0; slot < mapTemplate->m_zones.size(); ++slot)
        if (mapTemplate->m_zones[slot]->m_kind == RMG_TEMPLATE_COMPUTER)
            ++playerSlots;
    return playerSlots >= humanPlayers + computerPlayers;
}

// Retail-only rmg.txt coordinator. The normalized map volume uses 36*36
// cells per size unit; islands halve it with a minimum of one. Rows 76..84
// describe bidirectional connections after the canonical zone reader.
// Partial: 78.63%. Flattened parsing/validation passes score 22.20%,
// expanding findZone and vector operations that retail retains. Ordinary
// helpers restore those boundaries (78.33% before row/assignment/order edits).
// Remaining differences include string assignment expansion, rejection
// cleanup, and frame/register homes. No inlining controls are retained.
VA(0x00537FF0, 0x482)
void type_random_map_generator::loadTemplates()
{
    TSpreadsheetResource* sheet = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x00682804, rmgTemplatesFilename, "rmg.txt"));
    int mapSize = m_map.m_mapWidth * m_map.m_mapHeight * m_map.m_numberLevels / 1296;
    int row = 3;
    if (m_waterContent == RMG_WATER_ISLANDS)
        mapSize = max(mapSize / 2, 1);
    for (; row < sheet->getNumberOfRows();) {
        const TSpreadsheetResource::TStringVector& values = sheet->getRow(row);
        if (values.size() < 2) {
            ++row;
            continue;
        }
        TRmgTemplate* mapTemplate = new TRmgTemplate;
        mapTemplate->m_minimumSize = atoi(values[1]);
        mapTemplate->m_maximumSize = atoi(values[2]);
        mapTemplate->m_name.assign(values[0]);
        int endRow = row + 1;
        while (endRow < sheet->getNumberOfRows()
            && (!sheet->getRow(endRow)[0][0] || sheet->getRow(endRow)[0][0] == ' '))
            ++endRow;
        if (mapSize < mapTemplate->m_minimumSize
            || mapSize > mapTemplate->m_maximumSize) {
            delete mapTemplate;
        } else {
            readRmgTemplateZones(sheet, mapTemplate, row, endRow,
                m_humanPlayerCount, m_computerPlayerCount, m_mapVersion);
            readRmgTemplateConnections(sheet, mapTemplate, row, endRow,
                m_humanPlayerCount, m_computerPlayerCount);
            if (!hasRmgTemplatePlayerSlots(mapTemplate,
                m_humanPlayerCount, m_computerPlayerCount)) {
                delete mapTemplate;
            } else {
                for (unsigned int zone = 0; zone < mapTemplate->m_zones.size(); ++zone)
                    mapTemplate->m_zones[zone]->m_zoneIndex = zone;
                m_templates.push_back(mapTemplate);
            }
        }
        row = endRow;
    }
    sheet->dispose();
}

// The rmg.txt coordinator at 0x5381ad passes the spreadsheet in ecx,
// template in edx, then row bounds/player counts/map version on the stack.
// Retail's new(0xd4), field stores and connection-vector constructor prove
// TRmgTownSlot's layout independently of the generated-zone consumers.
// GetRow is the canonical DC-proven TextResource.h helper (dc 0x508a4,
// lines 128/131, with two absent lines); there is no DC RMG counterpart.
// Names and the shared field predicate remain provisional.
// Exact: all 1,671 retail bytes, including the switch tables, after resolving
// 33 relocations; 139/139 CFG blocks agree. The rejected-player arm must
// precede the accepted arm in source, although VC6 places its cleanup last.
// That order retains the connection-vector destructor at 0x46a650; using
// ordinary push_back then reproduces the full insertion expansion.
// Controls: accepted arm first with count-insert scores 96.94682% and
// expands vector cleanup; rejection first with count-insert reaches 98.08062%
// but has different growth temporaries/registers. The exact push_back form
// preserves the canonical STL interface rather than selecting its nested
// overload to compensate for the wrong source order.
// The positive row-validation scope is also significant: an early continue
// leaves slot too broadly scoped (92.97% vs 96.95% with the old count-insert).
// Grouping case 'a' with default replaces retail's jump table with compares;
// town flags require separate 0/1 store arms. Named insertion iterators,
// a narrower terrain-local scope, explicit empty slot special members and
// a byte-returning field predicate were neutral on the 96.95% control.
// Flattening the provisional predicate with count-insert changes two SIB
// operands (96.91252%); with push_back it retains nested leaves (68.95026%).
// Temporary inline-depth controls were removed: pinning delete either did
// nothing or retained the wrong scalar-deleting boundary; pinning implicit
// member cleanup left growth differences and added a non-retail row call.
// GetRow's DC line gap motivated bounds-check probes, but the row-count
// accessor added a non-retail size call and direct size was byte-neutral.
// Neither supplies evidence for a retained release-elided assertion.
VA(0x00538480, 0x687) // anchor-callee 0x5381ad; fastcall, ret 0x14; retail-only
void readRmgTemplateZones(
    const TSpreadsheetResource* sheet, TRmgTemplate* mapTemplate,
    int firstRow, int endRow, int humanPlayers, int computerPlayers,
    int mapVersion)
{
    for (int row = firstRow; row < endRow; ++row) {
        const TSpreadsheetResource::TStringVector& values = sheet->getRow(row);
        if (values.size() >= 3 && isRmgTemplateFieldSet(values[3]) &&
            values.size() > 75) {

            TRmgTownSlot* slot = new TRmgTownSlot;
            slot->m_zoneIndex = atoi(values[3]);
            slot->m_kind = RMG_TEMPLATE_TREASURE;
            if (isRmgTemplateFieldSet(values[4]))
                slot->m_kind = RMG_TEMPLATE_HUMAN;
            if (isRmgTemplateFieldSet(values[5]))
                slot->m_kind = RMG_TEMPLATE_COMPUTER;
            if (isRmgTemplateFieldSet(values[6]))
                slot->m_kind = RMG_TEMPLATE_TREASURE;
            if (isRmgTemplateFieldSet(values[7]))
                slot->m_kind = RMG_TEMPLATE_JUNCTION;
            slot->m_size = atoi(values[8]);
            slot->m_minimumHumanPlayers = atoi(values[9]);
            slot->m_maximumHumanPlayers = atoi(values[10]);
            slot->m_minimumPlayers = atoi(values[11]);
            slot->m_maximumPlayers = atoi(values[12]);
            if (slot->m_minimumHumanPlayers > humanPlayers ||
                slot->m_maximumHumanPlayers < humanPlayers ||
                slot->m_minimumPlayers > humanPlayers + computerPlayers ||
                slot->m_maximumPlayers < humanPlayers + computerPlayers) {
                delete slot;
            } else {
                slot->m_playerIndex = atoi(values[13]) - 1;
                slot->m_parameters0020[0] = atoi(values[14]);
                slot->m_parameters0020[1] = atoi(values[15]);
                slot->m_parameters0020[2] = atoi(values[16]);
                slot->m_parameters0020[3] = atoi(values[17]);
                slot->m_parameters0020[4] = atoi(values[18]);
                slot->m_parameters0020[5] = atoi(values[19]);
                slot->m_parameters0020[6] = atoi(values[20]);
                slot->m_parameters0020[7] = atoi(values[21]);
                slot->m_flag0040 = 0;
                if (isRmgTemplateFieldSet(values[22]))
                    slot->m_flag0040 = 1;
                int townCount;
                if (mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE)
                    townCount = 9;
                else {
                    townCount = 8;
                    slot->m_allowedTowns[8] = 0;
                }
                while (townCount--) {
                    if (isRmgTemplateFieldSet(values[23 + townCount]))
                        slot->m_allowedTowns[townCount] = 1;
                    else
                        slot->m_allowedTowns[townCount] = 0;
                }
                for (int mine = 0; mine < 7; ++mine)
                    slot->m_parameters004c[mine] = atoi(values[32 + mine]);
                for (int resource = 0; resource < 7; ++resource)
                    slot->m_parameters0068[resource] = atoi(values[39 + resource]);
                slot->m_flag0084 = isRmgTemplateFieldSet(values[46]);
                unsigned char anyTerrain = 0;
                for (int terrain = 0; terrain < 8; ++terrain) {
                    slot->m_allowedTerrain[terrain] =
                        isRmgTemplateFieldSet(values[47 + terrain]);
                    if (slot->m_allowedTerrain[terrain])
                        anyTerrain = 1;
                }
                if (!anyTerrain)
                    slot->m_allowedTerrain[0] = 1;
                switch (tolower(values[55][0])) {
                case 'n': slot->m_monsterStrength = 0; break;
                case 'w': slot->m_monsterStrength = 2; break;
                case 's': slot->m_monsterStrength = 4; break;
                case 'a': slot->m_monsterStrength = 3; break;
                default: slot->m_monsterStrength = 3; break;
                }
                slot->m_flag0094 = isRmgTemplateFieldSet(values[56]);
                for (int monster = 0; monster < 10; ++monster)
                    slot->m_allowedMonsters[monster] =
                        isRmgTemplateFieldSet(values[57 + monster]);
                if (mapVersion < RMG_MAP_ARMAGEDDONS_BLADE)
                    slot->m_allowedMonsters[8] = 0;
                for (int treasure = 0; treasure < 3; ++treasure) {
                    slot->m_treasure[treasure].m_minimum = atoi(values[67 + 3 * treasure]);
                    slot->m_treasure[treasure].m_maximum = atoi(values[68 + 3 * treasure]);
                    slot->m_treasure[treasure].m_density = atoi(values[69 + 3 * treasure]);
                }
                mapTemplate->m_zones.push_back(slot);
            }
        }
    }
}

// Complete-only object-generator roster.  Retail proves the source-level
// `push_back(new ...)` chain through all three stages of VC6's real inline
// ladder: early sites retain vector::insert(pos, value), middle sites retain
// vector::insert(pos, 1, value), and late sites retain vector::push_back.
// Those are compiler expansion choices for one honest source operation, not
// three manually selected overloads.  The four loops below are likewise the
// only repeated structures present in retail; every other registration is an
// unrolled source statement.
VA(0x00538B10, 0x2241)
void type_random_map_generator::initializeObjectGenerators()
{
    m_objectGenerators.push_back(new type_treasure_def(2, 0, 100, 20));
    m_objectGenerators.push_back(new type_treasure_def(4, 0, 3000, 50));

    {
        int creatureCount = m_mapVersion >= 1 ? 145 : 118;
        for (int creature = creatureCount; creature--;) {
            if (g_creatureTypeTraits[creature].m_level >= 0)
                m_objectGenerators.push_back(
                    new type_black_box_creature_def(creature));
        }
    }

    m_objectGenerators.push_back(
        new type_black_box_experience_def(6000, 5000));
    m_objectGenerators.push_back(
        new type_black_box_experience_def(12000, 10000));
    m_objectGenerators.push_back(
        new type_black_box_experience_def(18000, 15000));
    m_objectGenerators.push_back(
        new type_black_box_experience_def(24000, 20000));

    m_objectGenerators.push_back(new type_black_box_gold_def(5000, 5000));
    m_objectGenerators.push_back(new type_black_box_gold_def(10000, 10000));
    m_objectGenerators.push_back(new type_black_box_gold_def(15000, 15000));
    m_objectGenerators.push_back(new type_black_box_gold_def(20000, 20000));

    m_objectGenerators.push_back(new type_black_box_spells_def(5000, 1, 1, 15));
    m_objectGenerators.push_back(new type_black_box_spells_def(7500, 2, 2, 15));
    m_objectGenerators.push_back(new type_black_box_spells_def(10000, 3, 3, 15));
    m_objectGenerators.push_back(new type_black_box_spells_def(12500, 4, 4, 15));
    m_objectGenerators.push_back(new type_black_box_spells_def(15000, 5, 5, 15));
    m_objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, 1));
    m_objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, 2));
    m_objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, 4));
    m_objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, 8));
    m_objectGenerators.push_back(new type_black_box_spells_def(30000, 1, 5, 15));

    {
        int player = m_objectPrototypes[10].size();
        m_disabledKeyTents.resize(player);
        for (; player--;) {
            m_disabledKeyTents[player] = 0;
            m_objectGenerators.push_back(new type_key_tent_def(player, 5000));
            m_objectGenerators.push_back(new type_key_tent_def(player, 7500));
            m_objectGenerators.push_back(new type_key_tent_def(player, 10000));
            m_objectGenerators.push_back(new type_key_tent_def(player, 15000));
            m_objectGenerators.push_back(new type_key_tent_def(player, 20000));
        }
    }

    m_objectGenerators.push_back(new type_treasure_def(7, 0, 8000, 20));
    m_objectGenerators.push_back(new type_treasure_def(11, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(12, 0, 2000, 500));
    m_objectGenerators.push_back(new type_treasure_def(13, 0, 5000, 20));
    m_objectGenerators.push_back(new type_treasure_def(13, 1, 10000, 20));
    m_objectGenerators.push_back(new type_treasure_def(13, 2, 7500, 20));
    m_objectGenerators.push_back(new type_treasure_def(14, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(16, 0, 3000, 100));
    m_objectGenerators.push_back(new type_treasure_def(16, 1, 2000, 100));
    m_objectGenerators.push_back(new type_treasure_def(16, 2, 2000, 100));
    m_objectGenerators.push_back(new type_treasure_def(16, 3, 5000, 100));
    m_objectGenerators.push_back(new type_treasure_def(16, 4, 1500, 100));
    m_objectGenerators.push_back(new type_treasure_def(16, 5, 3000, 100));
    m_objectGenerators.push_back(new type_treasure_def(16, 6, 9000, 100));

    int dwelling = 80;
    if (m_mapVersion < 1)
        dwelling = 58;
    for (; dwelling--;)
        m_objectGenerators.push_back(new type_map_dwelling_def(dwelling));

    m_objectGenerators.push_back(new type_treasure_def(22, 0, 500, 100));
    m_objectGenerators.push_back(new type_treasure_def(23, 0, 1500, 100));
    m_objectGenerators.push_back(new type_treasure_def(24, 0, 4000, 20));
    m_objectGenerators.push_back(new type_treasure_def(25, 0, 10000, 100));
    m_objectGenerators.push_back(new type_treasure_def(28, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(29, 0, 500, 1000));
    m_objectGenerators.push_back(new type_treasure_def(30, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(31, 0, 100, 50));
    m_objectGenerators.push_back(new type_treasure_def(32, 0, 1500, 100));
    m_objectGenerators.push_back(new type_treasure_def(35, 0, 7000, 20));
    m_objectGenerators.push_back(new type_treasure_def(38, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(39, 0, 500, 100));
    m_objectGenerators.push_back(new type_treasure_def(41, 0, 12000, 20));
    m_objectGenerators.push_back(new type_treasure_def(47, 0, 1000, 50));
    m_objectGenerators.push_back(new type_treasure_def(48, 0, 500, 50));
    m_objectGenerators.push_back(new type_treasure_def(49, 0, 250, 100));
    m_objectGenerators.push_back(new type_treasure_def(51, 0, 1500, 100));
    m_objectGenerators.push_back(new type_treasure_def(52, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(55, 0, 500, 50));
    m_objectGenerators.push_back(new type_treasure_def(56, 0, 100, 50));
    m_objectGenerators.push_back(new type_treasure_def(57, 0, 3500, 200));
    m_objectGenerators.push_back(new type_treasure_def(58, 0, 750, 100));
    m_objectGenerators.push_back(new type_treasure_def(60, 0, 750, 100));
    m_objectGenerators.push_back(new type_treasure_def(61, 0, 1500, 100));

    m_objectGenerators.push_back(new type_prison_def(2500, 0));
    m_objectGenerators.push_back(new type_prison_def(5000, 5000));
    m_objectGenerators.push_back(new type_prison_def(10000, 15000));
    m_objectGenerators.push_back(new type_prison_def(20000, 90000));
    m_objectGenerators.push_back(new type_prison_def(30000, 500000));
    m_objectGenerators.push_back(new type_treasure_def(63, 0, 5000, 20));
    m_objectGenerators.push_back(new type_treasure_def(64, 0, 100, 100));

    m_objectGenerators.push_back(new type_artifact_def(66, 2000));
    m_objectGenerators.push_back(new type_artifact_def(67, 5000));
    m_objectGenerators.push_back(new type_artifact_def(68, 10000));
    m_objectGenerators.push_back(new type_artifact_def(69, 20000));

    m_objectGenerators.push_back(new type_resource_lump_def(76, 0, 1500, 2000));
    m_objectGenerators.push_back(new type_treasure_def(78, 0, 5000, 20));
    m_objectGenerators.push_back(new type_resource_lump_def(79, 0, 1400, 300));
    m_objectGenerators.push_back(new type_resource_lump_def(79, 2, 1400, 300));
    m_objectGenerators.push_back(new type_resource_lump_def(79, 1, 2000, 300));
    m_objectGenerators.push_back(new type_resource_lump_def(79, 3, 2000, 300));
    m_objectGenerators.push_back(new type_resource_lump_def(79, 4, 2000, 300));
    m_objectGenerators.push_back(new type_resource_lump_def(79, 5, 2000, 300));
    m_objectGenerators.push_back(new type_resource_lump_def(79, 6, 750, 300));
    m_objectGenerators.push_back(new type_treasure_def(80, 0, 100, 50));
    m_objectGenerators.push_back(new type_scholar_def());
    m_objectGenerators.push_back(new type_treasure_def(82, 0, 1500, 500));

    for (int quest = 0; quest < m_objectPrototypes[83].size(); ++quest) {
        int creatureCount = m_mapVersion >= 1 ? 145 : 118;
        for (int creature = creatureCount; creature--;) {
            if (g_creatureTypeTraits[creature].m_level >= 0)
                m_objectGenerators.push_back(
                    new type_quest_creature_def(creature, quest));
        }

        m_objectGenerators.push_back(
            new type_quest_experience_def(quest, 2000, 5000));
        m_objectGenerators.push_back(
            new type_quest_experience_def(quest, 5333, 10000));
        m_objectGenerators.push_back(
            new type_quest_experience_def(quest, 8666, 15000));
        m_objectGenerators.push_back(
            new type_quest_experience_def(quest, 12000, 20000));
        m_objectGenerators.push_back(new type_quest_gold_def(quest, 2000, 5000));
        m_objectGenerators.push_back(new type_quest_gold_def(quest, 5333, 10000));
        m_objectGenerators.push_back(new type_quest_gold_def(quest, 8666, 15000));
        m_objectGenerators.push_back(new type_quest_gold_def(quest, 12000, 20000));
    }

    m_objectGenerators.push_back(new type_treasure_def(84, 0, 1000, 100));
    m_objectGenerators.push_back(new type_treasure_def(85, 0, 2000, 100));
    m_objectGenerators.push_back(new type_treasure_def(86, 0, 1500, 50));
    m_objectGenerators.push_back(new type_shrine_def(88, 500));
    m_objectGenerators.push_back(new type_shrine_def(89, 2000));
    m_objectGenerators.push_back(new type_shrine_def(90, 3000));
    m_objectGenerators.push_back(new type_treasure_def(92, 0, 100, 20));
    m_objectGenerators.push_back(new type_spell_scroll_def(1, 500));
    m_objectGenerators.push_back(new type_spell_scroll_def(2, 2000));
    m_objectGenerators.push_back(new type_spell_scroll_def(3, 3000));
    m_objectGenerators.push_back(new type_spell_scroll_def(4, 4000));
    m_objectGenerators.push_back(new type_spell_scroll_def(5, 5000));
    m_objectGenerators.push_back(new type_treasure_def(94, 0, 200, 40));
    m_objectGenerators.push_back(new type_treasure_def(95, 0, 100, 20));
    m_objectGenerators.push_back(new type_treasure_def(96, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(97, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(99, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(100, 0, 1500, 200));
    m_objectGenerators.push_back(new type_treasure_def(101, 0, 1500, 1000));
    m_objectGenerators.push_back(new type_treasure_def(102, 0, 2500, 50));
    m_objectGenerators.push_back(new type_treasure_def(104, 0, 2500, 20));
    m_objectGenerators.push_back(new type_treasure_def(105, 0, 500, 50));
    m_objectGenerators.push_back(new type_treasure_def(106, 0, 1500, 50));
    m_objectGenerators.push_back(new type_treasure_def(107, 0, 1000, 50));
    m_objectGenerators.push_back(new type_treasure_def(108, 0, 6000, 20));
    m_objectGenerators.push_back(new type_treasure_def(109, 0, 750, 50));
    m_objectGenerators.push_back(new type_treasure_def(110, 0, 500, 50));
    m_objectGenerators.push_back(new type_treasure_def(112, 0, 2500, 150));
    m_objectGenerators.push_back(new type_witch_hut_def());
}

// Candidate generators and the boundary coordinator call this predicate.
// Player zones placed underground require an underground town alignment;
// same-level zones with different template IDs must keep 80% of the sum of
// their nominal radii. The whole-position copies are retained retail evidence.
// Residual: the subtraction/square temporaries exchange registers (98.04%).
// Keeping the input slot before its position restores the first source group;
// naming dy before dx restores the trailing sqrt/size sequence. A constructed
// TPoint delta is 95.36% and changes that sequence; independent initial
// field reads were 90.05%. No DC counterpart establishes the math boundary.
// The signed-distance family also varied named/in-place squares, running
// sums and subtraction temporaries; none exceeded the 98.04% baseline.
// A further 60-case declaration/copy matrix is also flat: separating dx/dy
// declarations from evaluation and assigning the returned position do not
// recover the extra retail register move. No candidate raised collateral MAX.
VA(0x0053AD60, 0x113) // anchor-callee 0x53e2ea/0x53af04; thiscall, ret 4
unsigned char type_random_map_generator::canPlaceZone(TRmgZone* zone)
{
    TRmgTownSlot* slot = zone->m_slot;
    TRmgMapPosition position = zone->getLevelPosition();
    int size = slot->m_size;
    if ((slot->m_kind == RMG_TEMPLATE_HUMAN ||
         slot->m_kind == RMG_TEMPLATE_COMPUTER) &&
        position.m_z == 1 && zone->m_alignment != TOWN_INFERNO &&
        zone->m_alignment != TOWN_NECROPOLIS && zone->m_alignment != TOWN_DUNGEON)
        return 0;
    int zoneIndex = slot->m_zoneIndex;
    for (int other = 0; other < m_zones.size(); ++other) {
        TRmgZone* otherZone = m_zones[other];
        if (otherZone->getLevelPosition().m_z != position.m_z ||
            otherZone->m_slot->m_zoneIndex == zoneIndex)
            continue;
        TRmgMapPosition otherPosition = otherZone->getLevelPosition();
        int dy = otherPosition.m_y - position.m_y;
        int dx = otherPosition.m_x - position.m_x;
        int distance = static_cast<int>(sqrt(static_cast<double>(dx * dx + dy * dy)));
        if (10 * distance < 8 * (otherZone->m_slot->m_size + size))
            return 0;
    }
    return 1;
}

// Both connection-count passes in FilterZonePositions retain the same
// vector-size and CanConnect calls. Keep the shared operation as one
// ordinary helper; its source name/boundary remain retail hypotheses.
// Including the by-value position/setter in this helper is byte-neutral,
// as is naming a reference to the current connection. Neither restores
// the two vector::size calls over-inlined in the first counting pass.
// A temporary depth-0 pin on that loop condition restores those two calls
// but expands the first zone-pointer vector::size instead (94.56%); the
// ordinary unpinned source is the negative control. Early-continue for an
// unplaced destination is also byte-neutral. No diagnostic pin is retained.
int type_random_map_generator::countPlacedZoneConnections(TRmgZone* zone) const
{
    int result = 0;
    TRmgTownSlot* slot = zone->m_slot;
    for (int connection = 0; connection < slot->m_connections.size(); ++connection) {
        int destination = slot->m_connections[connection].m_destination->m_zoneIndex;
        if (destination < m_zones.size() && m_zones[destination]->canConnect(zone))
            ++result;
    }
    return result;
}

// Called by the zone-position selector at 0x53bb38 with a generated zone,
// its vector of 12-byte candidate coordinates and the requested map size.
// Prefer unused levels, then maximum connections, then the smallest square
// enclosing the existing zones plus this candidate. Complete-only code;
// the role and call ABI are retail-proven; source names remain provisional.
// The ordinary by-value position setter preserves three loads before the
// stores (91.50 -> 94.35%); direct field assignment interleaves them. The
// bounds comparison and initialization order reach 94.45%. Retail retains
// all four connection-vector size calls; our first pass expands two.
// Preserve the ordinary helper and STL interfaces while resolving that
// frontier; later bounds differences are scheduling and SIB operand order.
VA(0x0053B2F0, 0x678) // anchor-callee 0x53bb38; thiscall, ret 0xc
void type_random_map_generator::filterZonePositions(
    TRmgZone* zone, std::vector<TRmgMapPosition>& candidates, int mapSize)
{
    int bestConnections = 0;
    if (m_map.m_numberLevels > 1) {
        unsigned char occupiedLevels[2] = {0, 0};
        for (int other = 0; other < m_zones.size(); ++other) {
            if (m_zones[other] != zone)
                occupiedLevels[m_zones[other]->getLevelPosition().m_z] = 1;
        }
        if (!occupiedLevels[0] || !occupiedLevels[1]) {
            int candidate = candidates.size();
            while (candidate--) {
                if (!occupiedLevels[candidates[candidate].m_z])
                    break;
            }
            if (candidate > 0) {
                candidate = candidates.size();
                while (candidate--) {
                    if (occupiedLevels[candidates[candidate].m_z])
                        candidates.erase(candidates.begin() + candidate);
                }
            }
        }
    }

    for (int candidate = 0; candidate < candidates.size(); ++candidate) {
        zone->setLevelPosition(candidates[candidate]);
        int connections = countPlacedZoneConnections(zone);
        if (connections > bestConnections)
            bestConnections = connections;
    }
    for (candidate = candidates.size() - 1; candidate >= 0; --candidate) {
        zone->setLevelPosition(candidates[candidate]);
        if (countPlacedZoneConnections(zone) < bestConnections)
            candidates.erase(candidates.begin() + candidate);
    }

    int bestSize = 32000;
    int minimumY = 0;
    int minimumX = 0;
    int maximumY = 0;
    int maximumX = 0;
    for (int other = 0; other < m_zones.size(); ++other) {
        if (m_zones[other] != zone) {
            TRmgMapPosition position = m_zones[other]->getLevelPosition();
            int size = m_zones[other]->m_slot->m_size;
            minimumY = min(minimumY, position.m_y - size);
            minimumX = min(minimumX, position.m_x - size);
            maximumY = max(maximumY, position.m_y + size + 1);
            maximumX = max(maximumX, position.m_x + size + 1);
        }
    }
    int size = zone->m_slot->m_size;
    for (candidate = 0; candidate < candidates.size(); ++candidate) {
        int candidateMinimumY = min(minimumY, candidates[candidate].m_y - size);
        int candidateMinimumX = min(minimumX, candidates[candidate].m_x - size);
        int candidateMaximumY = max(maximumY, candidates[candidate].m_y + size + 1);
        int candidateMaximumX = max(maximumX, candidates[candidate].m_x + size + 1);
        int candidateSize = max(mapSize, candidateMaximumY - candidateMinimumY);
        candidateSize = max(candidateSize, candidateMaximumX - candidateMinimumX);
        bestSize = min(bestSize, candidateSize);
    }
    for (candidate = candidates.size() - 1; candidate >= 0; --candidate) {
        int candidateMinimumY = min(minimumY, candidates[candidate].m_y - size);
        int candidateMinimumX = min(minimumX, candidates[candidate].m_x - size);
        int candidateMaximumY = max(maximumY, candidates[candidate].m_y + size + 1);
        int candidateMaximumX = max(maximumX, candidates[candidate].m_x + size + 1);
        int candidateSize = max(mapSize, candidateMaximumY - candidateMinimumY);
        candidateSize = max(candidateSize, candidateMaximumX - candidateMinimumX);
        if (bestSize < candidateSize)
            candidates.erase(candidates.begin() + candidate);
    }
}

// Retained by generation coordinator 0x549930; retail-only role/ABI.
#if 0 // @carcass
VA(0x0053BCB0, 0x33B)
void type_random_map_generator::initializeZones(TRmgTemplate* mapTemplate) {} // @stub
#endif

// Retail keeps a vector of pending endpoints. Splitting pushes the old
// endpoint followed by the perturbed midpoint; completed unit edges mark
// the clamped starting cell and advance the current point.
// Exact: 555/555 raw retail bytes, resolving all 11 relocations. Keeping
// the comparison operators makes subdivision precede marking (38.31 ->
// 92.82%); flattened comparisons, reversed predicates and nested continue
// leave the arms misplaced. The arithmetic operators and vector calls
// preserve the retained insert/erase/Length/rand/insert/insert/delete
// sequence; std::stack instead retains the vector constructor.
// The long min/max arguments require conversion temporaries from int,
// but long clamp results bind directly (96.76%). size() > 0 preserves
// retail's shifted element count (98.02%); empty() and a bare size() test
// fold it into a masked byte-count test. Ending delta's scope before
// Length restores the register roles (99.90%); from-before-to midpoint
// operands settle the final SIB encoding. Extending delta's scope or
// assigning perpendicular's components independently loses exactness.
// Source boundaries remain provisional: no RMG counterpart or TPoint
// declaration was found in the DC corpus. DC type_point's retained ==,
// != and DistanceSquared use a different, four-byte packed x/y/z type.
// Neither this byte match nor that roster's absence settles whether the
// midpoint expression expanded another helper. Do not infer blank lines
// or assertions without a corresponding source-line record and evidence.
VA(0x0053BFF0, 0x22B) // caller 0x53c65b; thiscall, ret 0x1c; retail-only
void type_random_map_generator::drawIrregularZoneBoundary(
    TPoint from, TPoint to, int zoneIndex, int level, int roughness)
{
    std::vector<TPoint> pending;
    unsigned char markBoundary = level == 1 || m_waterContent != RMG_WATER_ISLANDS;
    pending.push_back(to);
    while (pending.size() > 0) {
        to = pending.back();
        pending.pop_back();
        TPoint midpoint((from.m_x + to.m_x + 1) / 2, (from.m_y + to.m_y + 1) / 2);
        if (midpoint != from && midpoint != to) {
            TRmgVector perpendicular;
            {
                TRmgVector delta = to - from;
                perpendicular = TRmgVector(-delta.m_y, delta.m_x);
            }
            int length = perpendicular.length();
            if (length > 1) {
                int limit = std::_cpp_min<long>(length, roughness);
                int displacement = rand() % limit - limit / 2;
                perpendicular = perpendicular * displacement / length;
                midpoint += perpendicular;
            }
            pending.push_back(to);
            pending.push_back(midpoint);
        } else {
            long x = std::_cpp_max<long>(from.m_x, 0);
            x = std::_cpp_min<long>(x, m_map.m_mapWidth - 1);
            long y = std::_cpp_max<long>(from.m_y, 0);
            y = std::_cpp_min<long>(y, m_map.m_mapHeight - 1);
            TRmgMapItem* item = m_map.getMapItem(x, y, level);
            item->m_zoneState.m_zone = zoneIndex;
            if (markBoundary)
                item->m_tileData.m_zoneBoundary = 1;
            from = to;
        }
    }
}

// Exact: 362/362 raw retail bytes, with no relocations. Both constructor
// assignments for the shallow/steep steps keep diagonal.x in memory;
// component initialization in the shallow arm and the common x=1 reproduce
// retail's inc in the diagonal loop. Hoisting the y sign before the slope
// test is wrong (77.57%). At the endpoint, flattening lastItem into the
// zone assignment changes only the final address calculation and loses
// exactness. Keep the actual map-item local, as in the loop above it.
VA(0x0053C220, 0x16A) // caller 0x53c4a2; thiscall, ret 0x18; retail-only
void type_random_map_generator::drawStraightZoneBoundary(
    TPoint from, TPoint to, int zoneIndex, int level)
{
    if (from.m_x > to.m_x)
        std::swap(from, to);
    int dx = to.m_x - from.m_x;
    int dy = to.m_y - from.m_y;
    int verticalDistance = abs(dy);
    int major;
    int minor;
    TPoint straight;
    TPoint diagonal;
    if (dx > verticalDistance) {
        major = dx;
        minor = verticalDistance;
        straight.m_x = 1;
        straight.m_y = 0;
        diagonal.m_y = dy > 0 ? 1 : -1;
    } else {
        major = verticalDistance;
        minor = dx;
        straight = TPoint(0, dy > 0 ? 1 : -1);
        diagonal = straight;
    }
    diagonal.m_x = 1;
    unsigned char markBoundary = level == 1 || m_waterContent != RMG_WATER_ISLANDS;
    int error = major / 2;
    while (from.m_x != to.m_x || from.m_y != to.m_y) {
        TRmgMapItem* item = m_map.getMapItem(from.m_x, from.m_y, level);
        item->m_zoneState.m_zone = zoneIndex;
        if (markBoundary)
            item->m_tileData.m_zoneBoundary = 1;
        error += minor;
        if (error < major) {
            from.m_x += straight.m_x;
            from.m_y += straight.m_y;
        } else {
            error -= major;
            from.m_x += diagonal.m_x;
            from.m_y += diagonal.m_y;
        }
    }
    TRmgMapItem* lastItem = m_map.getMapItem(from.m_x, from.m_y, level);
    lastItem->m_zoneState.m_zone = zoneIndex;
}

// The zone coordinator at 0x53e050 calls this with its generator receiver
// and a vertex returned by the Voronoi lookup at 0x5fd6b0. The body clips
// each edge, marks the owning zone's map cells, and records the polygon at
// zone+0x3f4. All names are provisional; Dreamcast has no RMG compiland.
// The found flag folds away, but its do/while plus post-search fallback
// recovers retail's backward jne, rectangle fall-through, and later success
// block. Returning from inside the search sinks the rectangle to the end:
// with the same copied points that control is 60.27%, versus 86.10% here.
// All seven appends copy a point. Reconstructing the main-loop copies from
// x/y instead costs 56.56% in the old search; a named copy is byte-identical
// to TPoint(from). Direct single/count insert calls bypass the push_back
// chain and measured 43.07%/8.56%; nesting the success body measured 29.83%.
// Retail stores height, zero y/x, then width. Its min also spills separate
// inputs only when the neighbour exists, keeping the result unaliased.
// These two corrections reach 86.68%. Moving the owning-zone read inside
// the neighbour arm loses that lifetime (79.98%); a bounds ctor is 80.77%.
// Residual: 90 vs 93 blocks, 37 calls on both sides. The search topology now
// agrees; two vector allocation paths still choose different _Ucopy/_Ufill/
// _Destroy expansions, followed by local-slot and register differences.
// Account for cross-type ICF before treating a template name as a new call.
VA(0x0053C390, 0x730) // caller 0x53e5f4/0x53e602, ret 8; retail-only
void type_random_map_generator::traceZoneBoundary(
    TRmgBoundaryVertex* first, unsigned char irregular)
{
    TRmgBoundaryVertex* vertex = first;
    TRmgZone* zone = vertex->m_zone;
    int zoneIndex = zone->m_slot->m_zoneIndex;
    TRmgMapPosition zonePosition = zone->m_levelPosition;
    TRmgZoneBounds bounds;
    bounds.m_maximumY = m_map.m_mapHeight;
    bounds.m_minimumX = bounds.m_minimumY = 0;
    bounds.m_maximumX = m_map.m_mapWidth;
    TRmgBoundaryVertex* next;
    TPoint originalFrom;
    TPoint originalTo;
    TPoint from;
    TPoint to;

    bool found = false;
    do {
        next = vertex->m_next;
        originalFrom = vertex->m_position;
        originalTo = next->m_position;
        from = clipRmgBoundaryPoint(bounds, vertex->m_position, next->m_position);
        to = clipRmgBoundaryPoint(bounds, originalTo, originalFrom);
        if (bounds.contains(from) && from != to) {
            found = true;
            break;
        }
        vertex = next;
    } while (vertex != first);
    if (!found) {
        TPoint upperLeft(bounds.m_minimumX, bounds.m_minimumY);
        TPoint upperRight(bounds.m_maximumX - 1, bounds.m_minimumY);
        TPoint lowerLeft(bounds.m_minimumX, bounds.m_maximumY - 1);
        TPoint lowerRight(bounds.m_maximumX - 1, bounds.m_maximumY - 1);
        drawStraightZoneBoundary(lowerRight, upperRight, zoneIndex, zonePosition.m_z);
        drawStraightZoneBoundary(upperRight, upperLeft, zoneIndex, zonePosition.m_z);
        drawStraightZoneBoundary(upperLeft, lowerLeft, zoneIndex, zonePosition.m_z);
        drawStraightZoneBoundary(lowerLeft, lowerRight, zoneIndex, zonePosition.m_z);
        zone->m_boundary.push_back(TPoint(lowerRight));
        zone->m_boundary.push_back(TPoint(upperRight));
        zone->m_boundary.push_back(TPoint(upperLeft));
        zone->m_boundary.push_back(TPoint(lowerLeft));
        return;
    }

    first = vertex;
    do {
        next = vertex->m_next;
        TRmgZone* neighbour = next->m_twin->m_zone;
        originalFrom = vertex->m_position;
        originalTo = next->m_position;
        from = clipRmgBoundaryPoint(bounds, vertex->m_position, next->m_position);
        to = clipRmgBoundaryPoint(bounds, originalTo, originalFrom);
        zone->m_boundary.push_back(TPoint(from));

        if (!neighbour || neighbour->m_slot->m_zoneIndex > zoneIndex) {
            int roughness = zone->m_boundaryRoughness;
            if (neighbour) {
                int ownRoughness = roughness;
                int neighbourRoughness = neighbour->m_boundaryRoughness;
                roughness = std::_cpp_min(ownRoughness, neighbourRoughness);
            }
            if (irregular)
                drawIrregularZoneBoundary(from, to, zoneIndex, zonePosition.m_z, roughness);
            else
                drawStraightZoneBoundary(from, to, zoneIndex, zonePosition.m_z);
        }

        vertex = next;
        if (to != originalTo) {
            from = to;
            for (;;) {
                next = next->m_next;
                to = clipRmgBoundaryPoint(bounds, vertex->m_position, next->m_position);
                if (bounds.contains(to))
                    break;
                vertex = next;
            }
            while (from.m_x != to.m_x && from.m_y != to.m_y) {
                TPoint corner;
                if (from.m_x == bounds.m_minimumX && from.m_y != bounds.m_minimumY)
                    corner = TPoint(bounds.m_minimumX, bounds.m_minimumY);
                else if (from.m_y == bounds.m_minimumY && from.m_x != bounds.m_maximumX - 1)
                    corner = TPoint(bounds.m_maximumX - 1, bounds.m_minimumY);
                else if (from.m_x == bounds.m_maximumX - 1 && from.m_y != bounds.m_maximumY - 1)
                    corner = TPoint(bounds.m_maximumX - 1, bounds.m_maximumY - 1);
                else
                    corner = TPoint(bounds.m_minimumX, bounds.m_maximumY - 1);
                drawStraightZoneBoundary(from, corner, zoneIndex, zonePosition.m_z);
                zone->m_boundary.push_back(TPoint(from));
                from = corner;
            }
            drawStraightZoneBoundary(from, to, zoneIndex, zonePosition.m_z);
            zone->m_boundary.push_back(TPoint(from));
        }
    } while (vertex != first);
}

// Exact: preserve the original point, and update a separate clipped point
// through value-returning addition. Compound += gives 64.11% and a 0x24
// frame; the sum gives 98.04%, retail's 0x1c frame and all 40 flow blocks.
// The added operator declaration alone is byte-flat: this is the arithmetic
// boundary, not a header-population change. Mutating the input argument and
// saving an original copy is 80.07%; reusing toward is 63.97%, so retail's
// later stores into an argument slot do not prove source-argument mutation.
// Keep the distance inside each scaling expression (99.11%). operator+
// in the earlier member model needed a value argument to close the last
// multiply. The retained Voronoi bodies now prove free point/vector addition
// and point subtraction with both operands by value; this caller stays exact.
// Scale operand order, a scalar-left overload, member-wise scale result,
// named numerators/bounds, const delta/distance and upper-bound regrouping
// were flat at 99.11%; none substitutes for the addition parameter fact.
// All arithmetic stays integer: multiply both components before division
// and retain the original point for every rejected-intersection return.
VA(0x0053CAC0, 0x266) // caller 0x53c407; hidden result ecx, bounds edx; retail-only
TPoint clipRmgBoundaryPoint(
    const TRmgZoneBounds& bounds, TPoint point, TPoint toward)
{
    if (bounds.contains(point))
        return point;

    TRmgVector delta = toward - point;
    TPoint clipped = point;
    if (clipped.m_x < bounds.m_minimumX && delta.m_x) {
        clipped = clipped + delta * (bounds.m_minimumX - clipped.m_x) / delta.m_x;
        if (point.m_y >= bounds.m_minimumY && clipped.m_y < bounds.m_minimumY)
            return point;
        if (point.m_y < bounds.m_maximumY && clipped.m_y >= bounds.m_maximumY)
            return point;
    }
    if (clipped.m_y < bounds.m_minimumY && delta.m_y) {
        clipped = clipped + delta * (bounds.m_minimumY - clipped.m_y) / delta.m_y;
        if (point.m_x >= bounds.m_minimumX && clipped.m_x < bounds.m_minimumX)
            return point;
        if (point.m_x < bounds.m_maximumX && clipped.m_x >= bounds.m_maximumX)
            return point;
    }
    if (clipped.m_x >= bounds.m_maximumX && delta.m_x) {
        clipped = clipped + delta * (bounds.m_maximumX - clipped.m_x - 1) / delta.m_x;
        if (point.m_y >= bounds.m_minimumY && clipped.m_y < bounds.m_minimumY)
            return point;
        if (point.m_y < bounds.m_maximumY && clipped.m_y >= bounds.m_maximumY)
            return point;
    }
    if (clipped.m_y >= bounds.m_maximumY && delta.m_y) {
        clipped = clipped + delta * (bounds.m_maximumY - clipped.m_y - 1) / delta.m_y;
        if (point.m_x >= bounds.m_minimumX && clipped.m_x < bounds.m_minimumX)
            return point;
        if (point.m_x < bounds.m_maximumX && clipped.m_x >= bounds.m_maximumX)
            return point;
    }
    return clipped;
}

// The pointer-valued worklist uses the same descending search as the map
// position overload below, but inserts the cost before the zone. Retail
// 0x53da1b..0x53da6f expands this boundary, including a separate pointer
// argument snapshot. Preserve the canonical overload and its source call.
// The spelling is provisional; the Complete-only RMG has no DC compiland.
static void insertRmgWorkItem(
    std::vector<TRmgZone*>& zones, std::vector<int>& costs,
    TRmgZone* zone, int cost)
{
    int first = 0;
    int last = zones.size();
    int middle;
    while (1) {
        middle = (first + last) >> 1;
        if (first >= last)
            break;
        if (cost < costs[middle])
            first = middle + 1;
        else
            last = middle;
    }
    costs.insert(costs.begin() + middle, cost);
    zones.insert(zones.begin() + middle, 1, zone);
}

// JoinExtraZones initializes the short distance columns to 32000, zeros
// each original zone's own column, and calls this relaxation after adding
// graph edges. Parallel vectors keep pending zones sorted by distance;
// popping from the back takes the smallest distance. The per-zone short
// table, not the queued priority, supplies the next relaxed distance.
// Residual 77.1353%: 32 seed/pop/insertion/slot-lifetime hypotheses raise
// 61.3176% without other RMG score changes. Direct public seed insertion and
// count insertion of the queued zone retain both vector erasures. The initial
// short-vector size still expands, and the seeds call count insert where
// retail retains single-element insert. Every variant passes 16,585 directed
// graph/root-count cases against an independent Floyd-Warshall distance table.
VA(0x0053D8E0, 0x1EC) // anchor-callee 0x53dcd2/0x53e020; Complete-only, ret 4
void type_random_map_generator::propagateZoneDistances(TRmgZone* zone)
{
    std::vector<TRmgZone*> pending;
    std::vector<int> costs;
    int distanceCount = zone->m_zoneDistances.size();
    for (int column = 0; column < distanceCount; ++column) {
        pending.insert(pending.end(), zone);
        costs.insert(costs.end(), 0);
        while (pending.size()) {
            TRmgZone* current = pending.back();
            pending.pop_back();
            TRmgTownSlot* slot = current->m_slot;
            costs.pop_back();
            int distance = current->m_zoneDistances[column] + 1;
            for (unsigned int connection = 0; connection < slot->m_connections.size(); ++connection) {
                TRmgZone* next = m_zones[slot->m_connections[connection].m_destination->m_zoneIndex];
                if (next->m_zoneDistances[column] > distance) {
                    next->m_zoneDistances[column] = distance;
                    insertRmgWorkItem(pending, costs, next, distance);
                }
            }
        }
    }
}

// The retained size call in propagation uses zone+0x3e4, and the signed
// two-byte loads above prove the short element independently of ICF peers.
VA_COMPGEN(0x0054C3D0, 0x12, VECTOR_SIZE, Short)

// BuildZoneBoundaries passes the count from before the radial sites were
// added and its live Voronoi diagram. Extra-to-extra edges become completed
// unguarded connections when their shared boundary intersects the map.
// Extra-to-original edges are admitted only if they do not shorten another
// original-zone distance, then the changed graph is propagated again.
// The connection's four input-filter limits are not initialized here in
// retail; only destination/value and its three policy bytes are assigned.
// Residual 68.8871%: 48 record-lifetime/bounds/public-insertion forms raise
// the initial 30.0554%. The final direct single-element insert restores the
// retained _Construct<TRmgZoneConnection> body; all four push_back calls
// instead inline two count-insert bodies and omit that construction symbol.
// Reusing one connection per function or outer loop remains lower. Short
// vector resizing and the third connection insertion still expand differently
// from retail; preserve the real operations and their canonical helpers.
VA(0x0053DAD0, 0x57F) // anchor-callee buildZoneBoundaries; Complete-only, ret 8
void type_random_map_generator::joinExtraZones(int originalZones, TRmgVoronoi* diagram)
{
    TRmgZoneBounds bounds = {0, 0, m_map.m_mapWidth, m_map.m_mapHeight};
    for (int index = originalZones; index < m_zones.size(); ++index) {
        TRmgZone* zone = m_zones[index];
        TRmgMapPosition position = zone->getLevelPosition();
        TRmgBoundaryVertex* first = diagram->locate(TPoint(position.m_x, position.m_y));
        for (int other = index + 1; other < m_zones.size(); ++other) {
            TRmgZone* destination = m_zones[other];
            if (destination->getLevelPosition().m_z != zone->getLevelPosition().m_z)
                continue;
            TRmgBoundaryVertex* edge = first;
            do {
                edge = edge->m_next;
                if (edge->m_twin->m_zone == destination)
                    break;
            } while (edge != first);
            if (edge->m_twin->m_zone != destination)
                continue;
            TPoint clipped = clipRmgBoundaryPoint(bounds, edge->m_position, edge->m_previous->m_position);
            if (bounds.contains(clipped)) {
                TRmgZoneConnection connection;
                connection.m_destination = destination->m_slot;
                connection.m_value = 0;
                connection.m_unguarded = 1;
                connection.m_placeBorderObjects = 0;
                connection.m_connected = 1;
                zone->m_slot->m_connections.push_back(connection);
                connection.m_destination = zone->m_slot;
                destination->m_slot->m_connections.push_back(connection);
            }
        }
    }
    for (index = 0; index < m_zones.size(); ++index) {
        TRmgZone* zone = m_zones[index];
        zone->m_zoneDistances.resize(originalZones);
        for (int column = originalZones; column--;)
            zone->m_zoneDistances[column] = 32000;
        if (zone->m_slot->m_zoneIndex < originalZones)
            zone->m_zoneDistances[zone->m_slot->m_zoneIndex] = 0;
    }
    for (index = 0; index < originalZones; ++index)
        propagateZoneDistances(m_zones[index]);

    for (index = originalZones; index < m_zones.size(); ++index) {
        TRmgZone* zone = m_zones[index];
        TRmgMapPosition position = zone->getLevelPosition();
        TRmgBoundaryVertex* first = diagram->locate(TPoint(position.m_x, position.m_y));
        for (int other = 0; other < originalZones; ++other) {
            TRmgZone* destination = m_zones[other];
            if (destination->getLevelPosition().m_z != zone->getLevelPosition().m_z)
                continue;
            TRmgBoundaryVertex* edge = first;
            do {
                edge = edge->m_next;
                if (edge->m_twin->m_zone == destination)
                    break;
            } while (edge != first);
            if (edge->m_twin->m_zone != destination)
                continue;
            int column = 0;
            for (; column < originalZones; ++column) {
                if (column != other
                    && destination->m_zoneDistances[column] > zone->m_zoneDistances[column] + 1)
                    break;
            }
            if (column < originalZones)
                continue;
            TRmgZoneConnection connection;
            connection.m_destination = destination->m_slot;
            connection.m_value = 0;
            connection.m_unguarded = 1;
            connection.m_placeBorderObjects = 0;
            connection.m_connected = 0;
            zone->m_slot->m_connections.push_back(connection);
            connection.m_destination = zone->m_slot;
            destination->m_slot->m_connections.insert(destination->m_slot->m_connections.end(), connection);
            propagateZoneDistances(destination);
        }
    }
}

// The final direct single-element insertion in JoinExtraZones expands the
// vector body while retaining this null-guarded seven-dword construction.
// All 20 raw bytes agree. With the resizing parent visible, this TU also
// emits the exact 18-byte short-vector size specialization.
VA_COMPGEN(0x0054DE90, 0x14, STD_CONSTRUCT, TRmgZoneConnection)

// The map-generation driver calls this once per level with its selected
// template. Sites for existing zones seed a subdivision; radial sites add
// water zones on the surface and unowned boundaries underground. Cleanup
// proves one subdivision lifetime and a nested temporary slot/zone pair.
// Retail-only source reconstruction: the original class/method names are
// unavailable in Dreamcast. The retained callees establish their interfaces.
// The selected zone stays live across radial inserts (83.26 -> 92.89%);
// repeatedly indexing zones loses that evidence. Explicit coordinate copies
// before the radial multiplications reproduce retail scheduling (96.32%).
// A single boolean TraceZoneBoundary argument preserves its ECX-valued
// true/false arms; separate literal calls use push-immediate instead.
// Remaining: the temporary zone's boundary vector destructor is retained
// where retail expands it, plus two width/height floating operand stores.
// Splitting the maximum-coordinate guards into nested ifs is byte-neutral.
VA(0x0053E050, 0x64D) // anchor-callee 0x549af9; thiscall, ret 8
void type_random_map_generator::buildZoneBoundaries(
    TRmgTemplate* mapTemplate, int level)
{
    TRmgVoronoi diagram;
    for (int zone = 0; zone < m_zones.size(); ++zone) {
        if (m_zones[zone]->getLevelPosition().m_z == level) {
            TRmgMapPosition position = m_zones[zone]->getLevelPosition();
            diagram.addSite(TPoint(position.m_x, position.m_y), m_zones[zone]);
        }
    }
    int originalZones = m_zones.size();
    if (level == 1 || m_waterContent != RMG_WATER_NONE) {
        TRmgTownSlot testSlot;
        testSlot.m_zoneIndex = -1;
        testSlot.m_kind = RMG_TEMPLATE_JUNCTION;
        testSlot.m_size = 0;
        TRmgZone testZone(&testSlot);
        TRmgZone* addedZone = 0;
        for (int zone = 0; zone < originalZones; ++zone) {
            TRmgZone* current = m_zones[zone];
            if (current->getLevelPosition().m_z != level)
                continue;
            int radius = current->m_boundaryRoughness;
            testSlot.m_size = radius;
            TRmgMapPosition position = current->getLevelPosition();
            for (int direction = 0; direction < 32; direction += 4) {
                TRmgMapPosition horizontalCenter = current->getLevelPosition();
                double dx = radius * gRmgDirectionCosines[direction];
                position.m_x = static_cast<int>(horizontalCenter.m_x + dx * 2);
                TRmgMapPosition verticalCenter = current->getLevelPosition();
                double dy = radius * gRmgDirectionSines[direction];
                position.m_y = static_cast<int>(verticalCenter.m_y + dy * 2);
                if (position.m_x < 0 && position.m_x < dx)
                    continue;
                if (position.m_x >= m_map.m_mapWidth) {
                    if (position.m_x >= m_map.m_mapWidth + dx)
                        continue;
                }
                if (position.m_y < 0 && position.m_y < dy)
                    continue;
                if (position.m_y >= m_map.m_mapHeight) {
                    if (position.m_y >= m_map.m_mapHeight + dy)
                        continue;
                }
                testZone.setLevelPosition(position);
                if (!canPlaceZone(&testZone))
                    continue;
                if (position.m_z == 0) {
                    TRmgTownSlot* slot = new TRmgTownSlot;
                    slot->m_zoneIndex = mapTemplate->m_zones.size();
                    slot->m_size = radius;
                    memset(slot->m_allowedMonsters, 0, sizeof(slot->m_allowedMonsters));
                    memset(slot->m_allowedTerrain, 0, sizeof(slot->m_allowedTerrain));
                    memset(slot->m_parameters004c, 0, sizeof(slot->m_parameters004c));
                    memset(slot->m_parameters0068, 0, sizeof(slot->m_parameters0068));
                    slot->m_parameters0020[0] = 0;
                    slot->m_parameters0020[1] = 0;
                    slot->m_parameters0020[2] = 0;
                    slot->m_parameters0020[3] = 0;
                    slot->m_parameters0020[4] = 0;
                    slot->m_parameters0020[5] = 0;
                    slot->m_parameters0020[6] = 0;
                    slot->m_parameters0020[7] = 0;
                    slot->m_monsterStrength = 0;
                    slot->m_playerIndex = -1;
                    memset(slot->m_treasure, 0, sizeof(slot->m_treasure));
                    slot->m_treasure[0].m_density = 5;
                    slot->m_treasure[0].m_maximum = 1000;
                    slot->m_treasure[0].m_minimum = 100;
                    slot->m_treasure[1].m_density = 1;
                    slot->m_treasure[1].m_maximum = 6000;
                    slot->m_treasure[1].m_minimum = 2000;
                    slot->m_kind = RMG_TEMPLATE_JUNCTION;
                    addedZone = new TRmgZone(slot);
                    addedZone->m_terrain = eTerrainWater;
                    addedZone->setLevelPosition(position);
                    mapTemplate->m_zones.push_back(slot);
                    m_zones.push_back(addedZone);
                }
                diagram.addSite(TPoint(position.m_x, position.m_y), addedZone);
            }
        }
    }
    diagram.buildVertices();
    for (zone = 0; zone < m_zones.size(); ++zone) {
        if (m_zones[zone]->getLevelPosition().m_z == level) {
            TRmgMapPosition position = m_zones[zone]->getLevelPosition();
            TRmgBoundaryVertex* first = diagram.locate(TPoint(position.m_x, position.m_y));
            traceZoneBoundary(first,
                zone < originalZones && (m_waterContent != RMG_WATER_ISLANDS || level == 1));
        }
    }
    for (zone = 0; zone < m_zones.size(); ++zone) {
        TRmgZone* current = m_zones[zone];
        if (current->getLevelPosition().m_z == level) {
            TRmgMapPosition position = current->getLevelPosition();
            fillZoneArea(current, diagram.locate(TPoint(position.m_x, position.m_y)));
        }
    }
    joinExtraZones(originalZones, &diagram);
}

// Retained by generation coordinator 0x549930; retail-only role/ABI.
#if 0 // @carcass
VA(0x0053E6A0, 0x337)
void type_random_map_generator::paintZoneTerrain() {} // @stub
#endif

// The midpoint-noise generator passes its work vector in ECX, center sample
// in EDX, then the complete nine-dword region and four edge midpoints by
// value. Each nondegenerate quadrant preserves the original variation.
// Retail 0x53ed00 consumes these records as a stack and clamps final samples
// to bytes; the terrain painter at 0x53efa0 consumes the resulting noise.
// Residual 99.9545%: Y-before-X reproduces the operation schedule; the three
// center/X/Y stack homes are permuted. The 24 midpoint-order/operand probes
// improve the initial 99.6818%, but 49 center-snapshot/lifetime probes are
// flat at that peak. All 32 CFG blocks and insertion decisions agree.
// A separate 3,136-case coverage/corner invariant check includes negative
// origins and degenerate bounds; every child preserves the sample meanings.
VA(0x0053E9E0, 0x31E) // anchor-callee 0x53ed91; Complete-only, fastcall ret 0x34
void subdivideRmgNoiseRegion(std::vector<TRmgNoiseRegion>& pending,
    int centerValue, TRmgNoiseRegion region, TRmgNoiseMidpoints midpoints)
{
    int middleY = (region.m_bounds.m_minimumY + region.m_bounds.m_maximumY) / 2;
    int middleX = (region.m_bounds.m_minimumX + region.m_bounds.m_maximumX) / 2;
    TRmgNoiseRegion part = region;
    part.m_bounds.m_minimumX = middleX;
    part.m_bounds.m_minimumY = middleY;
    part.m_corners[0] = centerValue;
    part.m_corners[1] = midpoints.m_maxYValue;
    part.m_corners[2] = midpoints.m_maxXValue;
    if (part.m_bounds.m_minimumX != part.m_bounds.m_maximumX
        && part.m_bounds.m_minimumY != part.m_bounds.m_maximumY)
        pending.push_back(part);

    part = region;
    part.m_bounds.m_minimumX = middleX;
    part.m_bounds.m_maximumY = middleY;
    part.m_corners[0] = midpoints.m_minYValue;
    part.m_corners[1] = centerValue;
    part.m_corners[3] = midpoints.m_maxXValue;
    if (part.m_bounds.m_minimumX != part.m_bounds.m_maximumX
        && part.m_bounds.m_minimumY != part.m_bounds.m_maximumY)
        pending.push_back(part);

    part = region;
    part.m_bounds.m_maximumX = middleX;
    part.m_bounds.m_minimumY = middleY;
    part.m_corners[0] = midpoints.m_minXValue;
    part.m_corners[2] = centerValue;
    part.m_corners[3] = midpoints.m_maxYValue;
    if (part.m_bounds.m_minimumX != part.m_bounds.m_maximumX
        && part.m_bounds.m_minimumY != part.m_bounds.m_maximumY)
        pending.push_back(part);

    part = region;
    part.m_bounds.m_maximumX = middleX;
    part.m_bounds.m_maximumY = middleY;
    part.m_corners[1] = midpoints.m_minXValue;
    part.m_corners[2] = midpoints.m_minYValue;
    part.m_corners[3] = centerValue;
    if (part.m_bounds.m_minimumX != part.m_bounds.m_maximumX
        && part.m_bounds.m_minimumY != part.m_bounds.m_maximumY)
        pending.push_back(part);
}

// Retained by the subdivision helper's ordinary vector insertion paths.
// The nine-dword copy stride identifies the specialization independently of
// all other 36-byte structures. No source-only emission anchor is required.
VA_COMPGEN(0x0054C670, 0x21, VECTOR_SIZE, TRmgNoiseRegion)
// First three quadrant appends retain count-insert; the fourth expands it.
// Retail's three arguments and 36-byte element arithmetic prove this overload.
VA_COMPGEN(0x0054D5C0, 0x2E4, VECTOR_INSERT_COUNT, TRmgNoiseRegion)
VA_COMPGEN(0x0054D960, 0x3B, VECTOR_UCOPY, TRmgNoiseRegion)
VA_COMPGEN(0x0054D9A0, 0x31, VECTOR_UFILL, TRmgNoiseRegion)

// The inlined search at 0x53fe7a returns an element pointer and its caller
// then tests that pointer, even on the found arm. Preserve that ordinary
// helper boundary rather than reducing the search to a boolean.
TRmgZoneConnection* TRmgTownSlot::findConnection(int destinationZone)
{
    for (unsigned int i = 0; i < m_connections.size(); ++i) {
        if (m_connections[i].m_destination->m_zoneIndex == destinationZone)
            return &m_connections[i];
    }
    return 0;
}

// Complete-only island and distance helpers, called at 0x53f81d and 0x53f603.
// Retail 0x53f048 passes the byte mask in ECX, width in EDX and height
// on the stack. This free fastcall boundary has no Dreamcast counterpart.
void __fastcall generateRmgIslandMask(unsigned char* mask, int width, int height);
// First reconstruction: 82.2075%. All 28 CFG blocks agree. Four blocks
// differ in size, including root initialization and the common subdivision
// call; preserve the nine-dword work item and four-edge by-value boundary.
VA(0x0053ED00, 0x29B)
void __fastcall generateRmgIslandMask(unsigned char* mask, int width, int height)
{
    std::vector<TRmgNoiseRegion> patches;
    TRmgNoiseRegion patch;
    patch.m_bounds.m_minimumX = 0;
    patch.m_bounds.m_minimumY = 0;
    patch.m_bounds.m_maximumX = height;
    patch.m_bounds.m_maximumY = width;
    patch.m_corners[0] = patch.m_corners[1] = patch.m_corners[2] = patch.m_corners[3] = 0;
    patch.m_variation = (height + width) / 4 + 1;
    TRmgNoiseMidpoints edges = { 0, 0, 0, 0 };
    subdivideRmgNoiseRegion(patches, patch.m_variation / 2, patch, edges);
    while (patches.size()) {
        patch = patches.back();
        patches.erase(patches.end() - 1);
        if (patch.m_bounds.m_maximumY == patch.m_bounds.m_minimumY + 1
            && patch.m_bounds.m_maximumX == patch.m_bounds.m_minimumX + 1) {
            if (patch.m_bounds.m_minimumX < 0 || patch.m_bounds.m_minimumX >= height
                || patch.m_bounds.m_minimumY < 0 || patch.m_bounds.m_minimumY >= width)
                continue;
            patch.m_corners[0] = min(max(patch.m_corners[0], 0), 255);
            mask[patch.m_bounds.m_minimumX * width + patch.m_bounds.m_minimumY] = patch.m_corners[0];
            continue;
        }
        if (patch.m_bounds.m_maximumX < 0 || patch.m_bounds.m_maximumY < 0
            || patch.m_bounds.m_minimumX >= height || patch.m_bounds.m_minimumY >= width)
            continue;
        edges.m_minXValue = (patch.m_corners[1] + patch.m_corners[0]) / 2;
        edges.m_minYValue = (patch.m_corners[2] + patch.m_corners[0]) / 2;
        edges.m_maxXValue = (patch.m_corners[3] + patch.m_corners[2]) / 2;
        edges.m_maxYValue = (patch.m_corners[3] + patch.m_corners[1]) / 2;
        int center = (patch.m_corners[3] + patch.m_corners[2]
            + patch.m_corners[1] + patch.m_corners[0]) / 4;
        int range = patch.m_variation;
        if (range > 1) {
            int half = range / 2;
            edges.m_minXValue += rand() % range - half;
            edges.m_minYValue += rand() % range - half;
            edges.m_maxXValue += rand() % range - half;
            edges.m_maxYValue += rand() % range - half;
            center += rand() % range - half;
        }
        patch.m_variation = (range - 1) / 2 + 1;
        subdivideRmgNoiseRegion(patches, center, patch, edges);
    }
}

// Paint the generated mask through a borrowed single-level map, destroy
// the brush before tagging dry tiles, then release the mask and report work.
// First reconstruction: 93.5789%. All 19 CFG blocks agree; the initial
// allocation/view/brush block has one extra instruction. Preserve the
// recovered brush/view destruction boundary while resolving its locals.
VA(0x0053EFA0, 0x1F2)
void type_random_map_generator::createWaterZoneIsland(const TRmgZoneBounds& bounds, int level)
{
    int height = bounds.m_maximumY - bounds.m_minimumY;
    int width = bounds.m_maximumX - bounds.m_minimumX;
    unsigned char* mask = new unsigned char[width * height];
    int terrain = rand() % 6;
    {
        type_random_map map(m_map.getMapItem(0, 0, level),
            m_map.m_mapWidth, m_map.m_mapHeight);
        TRmgTerrainBrush brush(&map, terrain, 4);
        generateRmgIslandMask(mask, width, height);
        for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
            for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
                if (mask[(y - bounds.m_minimumY) * width + x - bounds.m_minimumX] > 0)
                    brush.paintRectangle(x, y, 1, 1);
            }
        }
    }
    for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
        for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
            TRmgMapItem* item = m_map.getMapItem(x, y, level);
            if (item->m_tile.m_landType != eTerrainWater && !item->m_connection.m_present) {
                item->m_tileData.m_subterraneanGate = 0;
                item->m_tileData.m_borderObject = 1;
            }
        }
    }
    delete[] mask;
    if (m_progress)
        m_progress->advance(1000);
}
// Complete's island-spacing flood uses eight neighbours with costs 2/3,
// propagating only into the supplied zone. It resets connection metadata
// as each shorter distance is accepted, without testing terrain or objects.
// First reconstruction: 65.6485%. Seed insert and pop wrappers over-expand
// into count insert/_Destroy calls; retail retains the single-element
// insert and erase bodies. The shared worklist helper's position insertion
// instead remains a single-element call where retail expands that wrapper.
// Recover these caller-specific inline decisions without duplicating it.
VA(0x0053F1A0, 0x2C6)
void type_random_map_generator::floodWaterZoneDistances(TRmgMapPosition position, int zoneIndex)
{
    std::vector<TRmgMapPosition> positions;
    std::vector<int> costs;
    positions.insert(positions.end(), position);
    costs.insert(costs.end(), 0);
    TRmgMapItem* seed = m_map.getMapItem(position);
    seed->m_movement.m_zonePathCost = 0;
    seed->m_tileData.m_connectionDirection = 0;
    seed->m_zoneState.m_connectionEligibility = 0;
    while (positions.size()) {
        position = positions.back();
        costs.erase(costs.end() - 1);
        positions.erase(positions.end() - 1);
        unsigned currentCost = m_map.getMapItem(position)->m_movement.m_zonePathCost;
        for (int direction = 0; direction < 8; ++direction) {
            TRmgMapPosition next;
            next.m_x = position.m_x + g_rmgDirections[direction].m_x;
            next.m_y = position.m_y + g_rmgDirections[direction].m_y;
            next.m_z = position.m_z;
            if (next.m_x < 0 || next.m_x >= m_map.m_mapWidth
                || next.m_y < 0 || next.m_y >= m_map.m_mapHeight)
                continue;
            TRmgMapItem* item = m_map.getMapItem(next);
            if (item->m_zoneState.m_zone != zoneIndex)
                continue;
            unsigned nextCost = currentCost + ((direction & 1) ? 3 : 2);
            if (nextCost >= item->m_movement.m_zonePathCost)
                continue;
            item->m_movement.m_zonePathCost = nextCost;
            item->m_tileData.m_connectionDirection = direction;
            item->m_zoneState.m_connectionEligibility = 0;
            insertRmgWorkItem(positions, costs, next, nextCost);
        }
    }
}

// Retail-only: repeatedly seed islands in water areas at least 20 distance
// units from the current coast. Rebuild the candidate list after each island.
// First reconstruction: 93.4037%. Retail and candidate preserve the full
// reset, perimeter flood and repeated island loop. Remaining differences
// include reset-loop register scheduling and an extra CFG block; inspect
// candidate-list lifetime and clamp scheduling before altering semantics.
VA(0x0053F470, 0x409)
void type_random_map_generator::prepareWaterZoneConnections(TRmgZone* zone)
{
    if (zone->m_terrain != eTerrainWater)
        return;
    TRmgZoneBounds bounds = zone->m_bounds;
    int zoneIndex = zone->m_slot->m_zoneIndex;
    TRmgMapPosition position = zone->m_levelPosition;
    for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
            TRmgMapItem* item = m_map.getMapItem(position);
            item->m_movement.m_zonePathCost = 32000;
            item->m_tileData.m_connectionDirection = 0;
            item->m_zoneState.m_connectionEligibility = 0;
        }
    }
    TRmgZoneBounds surrounding;
    surrounding.m_minimumX = max(bounds.m_minimumX - 1, 0);
    surrounding.m_minimumY = max(bounds.m_minimumY - 1, 0);
    surrounding.m_maximumX = min(bounds.m_maximumX + 1, m_map.m_mapWidth);
    surrounding.m_maximumY = min(bounds.m_maximumY + 1, m_map.m_mapHeight);
    for (position.m_y = surrounding.m_minimumY; position.m_y < surrounding.m_maximumY; ++position.m_y) {
        for (position.m_x = surrounding.m_minimumX; position.m_x < surrounding.m_maximumX; ++position.m_x) {
            if (m_map.getMapItem(position)->m_zoneState.m_zone != zoneIndex)
                floodWaterZoneDistances(position, zoneIndex);
        }
    }
    bounds.m_minimumX = max(bounds.m_minimumX, 3);
    bounds.m_minimumY = max(bounds.m_minimumY, 3);
    bounds.m_maximumX = min(bounds.m_maximumX, m_map.m_mapWidth - 4);
    bounds.m_maximumY = min(bounds.m_maximumY, m_map.m_mapHeight - 4);
    while (1) {
        std::vector<TRmgMapPosition> candidates;
        for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
            for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
                if (m_map.getMapItem(position)->m_movement.m_zonePathCost >= 20)
                    candidates.push_back(position);
            }
        }
        if (!candidates.size())
            break;
        position = candidates[rand() % candidates.size()];
        int range = m_map.getMapItem(position)->m_movement.m_zonePathCost / 3 - 5;
        int radius = rand() % range + 3;
        if (radius > 6)
            radius = 6;
        TRmgZoneBounds island;
        island.m_minimumX = max(position.m_x - radius, 0);
        island.m_minimumY = max(position.m_y - radius, 0);
        island.m_maximumX = min(position.m_x + radius, m_map.m_mapWidth);
        island.m_maximumY = min(position.m_y + radius, m_map.m_mapHeight);
        createWaterZoneIsland(island, position.m_z);
        floodWaterZoneDistances(position, zoneIndex);
    }
}

// Retail 0x544932 handles dry assigned cells near water or another zone.
// Connection presence, level and guard policy decide whether to separate it.
// The three clipped rectangles follow the source pattern independently
// recovered in repairWaterZoneBorders; Complete has no Dreamcast RMG TU.
// Current match: 86.9809%. Direct clamps and named row/TPoint/dimension
// temporaries emit identical bytes. Remaining differences include the outer
// X induction variable (-1 here, +2 in retail) and rectangle register homes;
// preserve the two separate connection-policy tests at 0x53fa3f..0x53fa58.
VA(0x0053F880, 0x429)
void type_random_map_generator::expandObstacleClearance()
{
    TRmgMapItem* current = m_map.m_mapItems;
    TRmgMapPosition position;
    TRmgMapPosition nearby;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++current) {
                int zoneIndex = current->m_zoneState.m_zone;
                if (zoneIndex < 0 || current->m_tile.m_landType == eTerrainWater)
                    continue;
                TRmgZoneBounds bounds;
                {
                    bounds.m_minimumY = max(position.m_y - 1, 0);
                    bounds.m_minimumX = max(position.m_x - 1, 0);
                    bounds.m_maximumY = min(position.m_y + 2, m_map.m_mapHeight);
                    bounds.m_maximumX = min(position.m_x + 2, m_map.m_mapWidth);
                }
                TRmgZone* zone = m_zones[zoneIndex];
                unsigned char found = 0;
                nearby.m_z = position.m_z;
                for (nearby.m_y = bounds.m_minimumY; nearby.m_y < bounds.m_maximumY; ++nearby.m_y) {
                    for (nearby.m_x = bounds.m_minimumX; nearby.m_x < bounds.m_maximumX; ++nearby.m_x) {
                        TRmgMapItem* item = m_map.getMapItem(nearby);
                        int otherZone = item->m_zoneState.m_zone;
                        if (otherZone < 0) {
                            if (item->m_tile.m_landType == eTerrainWater)
                                found = 1;
                        } else if (otherZone != zoneIndex) {
                            TRmgZoneConnection* connection = zone->m_slot->findConnection(otherZone);
                            if (!connection || position.m_z == 1)
                                found = 1;
                            if (connection && !connection->m_unguarded)
                                found = 1;
                        }
                    }
                }
                if (!found)
                    continue;
                if (!current->m_connection.m_present) {
                    current->m_tileData.m_subterraneanGate = 0;
                    current->m_tileData.m_borderObject = 1;
                }
                {
                    bounds.m_minimumY = max(position.m_y, 0);
                    bounds.m_minimumX = max(position.m_x, 0);
                    bounds.m_maximumY = min(position.m_y + 1, m_map.m_mapHeight);
                    bounds.m_maximumX = min(position.m_x + 1, m_map.m_mapWidth);
                }
                for (nearby.m_y = bounds.m_minimumY; nearby.m_y < bounds.m_maximumY; ++nearby.m_y) {
                    for (nearby.m_x = bounds.m_minimumX; nearby.m_x < bounds.m_maximumX; ++nearby.m_x) {
                        TRmgMapItem* item = m_map.getMapItem(nearby);
                        if (item->m_tile.m_landType != eTerrainWater
                            && static_cast<int>(item->m_objects.size()) <= 0
                            && !item->m_connection.m_present) {
                            item->m_tileData.m_subterraneanGate = 0;
                            item->m_tileData.m_borderObject = 1;
                        }
                    }
                }
                {
                    bounds.m_minimumY = max(position.m_y - 1, 0);
                    bounds.m_minimumX = max(position.m_x - 1, 0);
                    bounds.m_maximumY = min(position.m_y + 2, m_map.m_mapHeight);
                    bounds.m_maximumX = min(position.m_x + 2, m_map.m_mapWidth);
                }
                for (nearby.m_y = bounds.m_minimumY; nearby.m_y < bounds.m_maximumY; ++nearby.m_y) {
                    for (nearby.m_x = bounds.m_minimumX; nearby.m_x < bounds.m_maximumX; ++nearby.m_x) {
                        TRmgMapItem* item = m_map.getMapItem(nearby);
                        if (static_cast<int>(item->m_objects.size()) <= 0 && !item->m_connection.m_present)
                            item->m_tileData.m_subterraneanGate = 0;
                    }
                }
            }
        }
    }
    if (m_progress)
        m_progress->advance(1600);
}

// Convert water beside usable land when its zone has no template connection
// to the marked neighbour. The inner square becomes border terrain and the
// outer empty square loses gate eligibility. Painting is deferred per level.
// All role names are provisional: this Complete-only pass has no DC body.
// Exact: 1516 bytes. Each clamp group keeps the original row, then names
// height and width immediately before their upper clamps. Flattening those
// dimension values reintroduces zero CSE/row scheduling at 0x53fee8 and the
// final maximum-X EAX/ECX schedule at 0x540030 (98.3965%).
// The guarded do loop keeps the exhaustion exit forward (0x53fe49) and
// jumps back to the item lookup (0x53fe4b). A for/while condition instead
// uses a backward jl plus a forward jmp with the same operation sequence.
// One four-int bounds aggregate preserves retail's contiguous -0x50..-0x44
// rectangle, including the dead minimumY home. Together with the shared
// terrain local, it restores the 0x84 frame and all observed local homes.
// Two TPoint corners or four independent bounds scalars instead take 0x7c.
// Splitting search/painting terrain lifetimes scores 97.0449%, but shifts
// the vectors/current pointer four bytes; preserve the retail frame shape.
// The outer coordinate must be assigned from each queued position: retail
// writes that value's z into the outer level slot at 0x5401e6. Source clear
// order is positions then terrains; VC6 schedules the terrain clear first.
// Controls: bool/byte found flags, scalar/nearby declaration scopes, for/while
// search, positive match/early continue, found/terrain assignment order,
// and int/terrain-enum vectors were byte-neutral in isolated controls.
// Additional neutral controls: explicit for-loop top exit, reference max,
// temporary clamp centers, whole positive repair guard, a named connection
// result, signed vector indices, a shared item pointer, and outer bounds scope.
// Three distinct bounds objects instead grow the frame to 0xa4. Initializing
// the second scan's y directly stores it before the remaining clamps (95.23%).
// Moving current's initialization past the vectors changes the entry loads.
// The three-scalar and by-value GetMapItem overloads expand identically.
// Terrain-vector insert matches all 521 bytes at 0x54d120; the retail
// widget-vector label there is a shared body, not a different operation.
// GetSize() in the six clamps adds virtual calls absent from retail (76.64%).
// TPoint's reference-argument constructor is neutral here, but is unproved
// for the signed type and changes DrawIrregularZoneBoundary's arithmetic.
// The old member subtraction hypothesis reached 98.3965%, but the retained
// 0x5fdd40 interface takes both points by value and returns a vector. Keep
// that interface: the old helper's 0.043-point gain is not declaration proof.
// Applying free subtraction or negative-vector translation to these lower
// corners changes the outer induction to x-1 rather than retail's x+2.
// Direct component construction and in-place translation keep x+2 (98.3535%).
// Naming the row through the upper clamps restores the second map-index
// operand order (98.3965%) with the retained point/vector APIs intact.
// Updating row in place changes already matching upper-Y loads (97.4473%);
// naming column before row loses that map-index order (98.3535%). Item
// references are byte-neutral. Reusing the lower value or radius across
// scans changes outer-loop registers. Reusing only the
// variable, or assigning it after default construction, was byte-neutral.
// Two TPoint members or by-value corner setters make bounds lose the 0x84
// frame. Named clamped corners add homes. Deferred upper-field stores do not
// fix the schedule; naming maximumX alone also moves homes without fixing it.
// Upper point addition changes the first upper-Y loads; constructing both
// corners before clamping promotes the outer row into EBX. An origin-plus-
// extent form keeps lower.y live rather than retail's original row value.
// Shared center values and int/long bounds/map fields are byte-neutral.
// Long point fields are also neutral here, but change the irregular edge
// arithmetic. A reused clamped corner still grows the frame to 0x88.
// Buffer-first constructor arguments recover the map view and painting loop;
// the rejected map/level pair, dimensions-first arguments, plane local, and
// initializer-list controls are recorded beside the constructor in rmg.h.
VA(0x0053FCB0, 0x5EC) // anchor-callee 0x544a31; thiscall, ret 0; retail-only
void type_random_map_generator::repairWaterZoneBorders()
{
    TRmgMapItem* current = m_map.m_mapItems;
    int terrain;
    TRmgMapPosition position;
    TRmgMapPosition nearby;
    std::vector<TRmgMapPosition> positions;
    std::vector<int> terrains;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++current) {
                int zoneIndex = current->m_zoneState.m_zone;
                if (zoneIndex < 0 || current->m_tile.m_landType != eTerrainWater)
                    continue;
                int destinationZone = current->m_zoneState.m_connectionEligibility;
                if (destinationZone < 0)
                    continue;

                unsigned char found = 0;
                TRmgZoneBounds bounds;
                {
                    int row = position.m_y;
                    TPoint lower(position.m_x - 1, row - 1);
                    bounds.m_minimumY = max(lower.m_y, 0);
                    bounds.m_minimumX = max(lower.m_x, 0);
                    int height = m_map.m_mapHeight;
                    bounds.m_maximumY = min(row + 2, height);
                    int width = m_map.m_mapWidth;
                    bounds.m_maximumX = min(position.m_x + 2, width);
                }
                nearby.m_z = position.m_z;
                TRmgZone* zone = m_zones[zoneIndex];
                for (nearby.m_y = bounds.m_minimumY;
                     nearby.m_y < bounds.m_maximumY && !found; ++nearby.m_y) {
                    nearby.m_x = bounds.m_minimumX;
                    if (nearby.m_x < bounds.m_maximumX) {
                        do {
                            TRmgMapItem* item = m_map.getMapItem(nearby);
                            if (item->m_tile.m_landType != eTerrainWater
                                && item->m_tile.m_landType != eTerrainRock
                                && !item->hasBorderObject()
                                && item->m_tileData.m_roadPassable) {
                                terrain = item->m_tile.m_landType;
                                found = 1;
                                break;
                            }
                            ++nearby.m_x;
                            if (nearby.m_x >= bounds.m_maximumX)
                                break;
                        } while (1);
                    }
                }
                if (!found || zone->m_slot->findConnection(destinationZone))
                    continue;

                {
                    int row = position.m_y;
                    TPoint lower(position.m_x - 1, row - 1);
                    bounds.m_minimumY = max(lower.m_y, 0);
                    bounds.m_minimumX = max(lower.m_x, 0);
                    int height = m_map.m_mapHeight;
                    bounds.m_maximumY = min(row + 2, height);
                    int width = m_map.m_mapWidth;
                    bounds.m_maximumX = min(position.m_x + 2, width);
                }
                for (nearby.m_y = bounds.m_minimumY; nearby.m_y < bounds.m_maximumY; ++nearby.m_y) {
                    for (nearby.m_x = bounds.m_minimumX; nearby.m_x < bounds.m_maximumX; ++nearby.m_x) {
                        TRmgMapItem* item = m_map.getMapItem(nearby);
                        if (!item->m_connection.m_present) {
                            item->m_tileData.m_subterraneanGate = 0;
                            item->m_tileData.m_borderObject = 1;
                        }
                        if (item->m_tile.m_landType == eTerrainWater) {
                            positions.push_back(nearby);
                            terrains.push_back(terrain);
                        }
                    }
                }

                {
                    int row = position.m_y;
                    TPoint lower(position.m_x - 2, row - 2);
                    bounds.m_minimumY = max(lower.m_y, 0);
                    bounds.m_minimumX = max(lower.m_x, 0);
                    int height = m_map.m_mapHeight;
                    bounds.m_maximumY = min(row + 3, height);
                    int width = m_map.m_mapWidth;
                    bounds.m_maximumX = min(position.m_x + 3, width);
                }
                for (nearby.m_y = bounds.m_minimumY; nearby.m_y < bounds.m_maximumY; ++nearby.m_y) {
                    for (nearby.m_x = bounds.m_minimumX; nearby.m_x < bounds.m_maximumX; ++nearby.m_x) {
                        TRmgMapItem* item = m_map.getMapItem(nearby);
                        if (static_cast<int>(item->m_objects.size()) <= 0
                            && !item->m_connection.m_present)
                            item->m_tileData.m_subterraneanGate = 0;
                    }
                }
            }
            if (m_progress)
                m_progress->advance(20);
        }
        if (positions.size()) {
            int lastTerrain = terrains[0];
            type_random_map levelMap(m_map.getMapItem(0, 0, position.m_z),
                m_map.m_mapWidth, m_map.m_mapHeight);
            TRmgTerrainBrush brush(&levelMap, lastTerrain, 4);
            for (unsigned int i = 0; i < positions.size(); ++i) {
                terrain = terrains[i];
                if (terrain != lastTerrain) {
                    brush.changeTerrain(terrain, 4);
                    lastTerrain = terrain;
                }
                position = positions[i];
                brush.paintRectangle(position.m_x, position.m_y, 1, 1);
            }
            positions.clear();
            terrains.clear();
        }
    }
}

// Complete-only connection pass. Reset both costs and predecessor state,
// seed each zone at its first usable gate (or its last empty candidate),
// then connect every remaining dry gate and extend the reachable cost set.
// Partial 99.3582%: the resetPosition lifetime inside the cell loop keeps
// separate cost writes and three predecessor registers. Hoisting it merges
// the costs (91.23%); direct predecessor stores also merge them (93.07%).
// The search row shares the later path coordinate; independent scalars grow
// the frame by four bytes. A guarded do loop keeps retail's forward exit.
// Residual: terrain extraction at 0x540701 masks six bits in retail, whereas
// the enum-to-unsigned local sign-extends. Byte narrowing scores 78.62%;
// preserve the signed canonical terrain field until its caller is resolved.
VA(0x005405D0, 0x304)
void type_random_map_generator::buildZoneConnectionPaths()
{
    int count = m_map.m_numberLevels * m_map.m_mapHeight * m_map.m_mapWidth;
    TRmgMapItem* item = m_map.m_mapItems;
    while (count--) {
        item->m_movement.m_zonePathCost = 32000;
        item->m_tileData.m_connectionDirection = 0;
        item->m_zoneState.m_connectionEligibility = -1;
        TRmgMapPosition previous;
        previous.m_x = -1;
        previous.m_y = -1;
        previous.m_z = -1;
        item->resetMovement(previous);
        ++item;
    }
    for (unsigned int zoneIndex = 0; zoneIndex < m_zones.size(); ++zoneIndex) {
        TRmgZone* zone = m_zones[zoneIndex];
        TRmgZoneBounds bounds = zone->m_bounds;
        TRmgMapPosition position = zone->m_levelPosition;
        TRmgMapPosition seed;
        TRmgMapPosition pathPosition;
        unsigned char found = 0;
        for (pathPosition.m_y = bounds.m_minimumY;
             pathPosition.m_y < bounds.m_maximumY && !found; ++pathPosition.m_y) {
            int x = bounds.m_minimumX;
            if (x < bounds.m_maximumX) {
                do {
                    TRmgMapItem* current = m_map.getMapItem(x, pathPosition.m_y, position.m_z);
                    if (current->m_zoneState.m_zone == zoneIndex) {
                        unsigned terrain = current->m_tile.m_landType;
                        if ((terrain != eTerrainWater || zone->m_terrain == terrain)
                            && static_cast<int>(current->m_objects.size()) <= 0) {
                            seed.m_x = x;
                            seed.m_y = pathPosition.m_y;
                            seed.m_z = position.m_z;
                            if (current->hasSubterraneanGate() && current->m_tileData.m_roadPassable
                                && terrain != eTerrainRock) {
                                found = 1;
                                break;
                            }
                        }
                    }
                    ++x;
                    if (x >= bounds.m_maximumX)
                        break;
                } while (1);
            }
        }
        if (!found) {
            TRmgMapItem* current = m_map.getMapItem(seed);
            if (!current->m_connection.m_present) {
                current->m_tileData.m_borderObject = 0;
                current->m_tileData.m_subterraneanGate = 1;
            }
        }
        m_map.floodConnectionCosts(seed, zone->m_terrain == eTerrainWater);
        pathPosition = zone->m_levelPosition;
        for (pathPosition.m_y = bounds.m_minimumY; pathPosition.m_y < bounds.m_maximumY; ++pathPosition.m_y) {
            for (pathPosition.m_x = bounds.m_minimumX; pathPosition.m_x < bounds.m_maximumX; ++pathPosition.m_x) {
                TRmgMapItem* current = m_map.getMapItem(pathPosition);
                if (current->m_zoneState.m_zone == zoneIndex
                    && current->hasSubterraneanGate() && current->m_tileData.m_roadPassable
                    && current->m_tile.m_landType != eTerrainRock && current->m_movement.m_cost
                    && current->m_tile.m_landType != eTerrainWater) {
                    openConnectionPath(pathPosition, 0);
                    m_map.floodConnectionCosts(pathPosition, zone->m_terrain == eTerrainWater);
                }
            }
        }
    }
}

// Complete's ground connection follows predecessor cells with positive
// movement cost, stopping at the zero-cost seed. It does not change costs:
// it clears border obstacles and marks the route traversable. Decoration
// cells select a BORDER_GUARD by direction and place the ordinary object.
// Retail retains the first getMapItem and selectObjectPrototype calls, then
// expands later map accesses. After virtual addObject it rechecks present.
// The non-narrow path clears only borderObject in the clipped same-zone 3x3
// neighbourhood; it must not mark all of those neighbours as route cells.
// Residual (79.9698%): the first map lookup over-expands, the frame is 0x28
// instead of 0x2c, and the common post-placement query adds a CFG block.
// A generated family of scalar/corner/rectangle bounds, independent bound
// orders, predecessor copy forms and nested present/absent scopes did not
// improve it. Keep the retained retail map/helper calls as the next target;
// do not replace them with a pasted body or an inline-depth pin.
VA(0x005408E0, 0x23F) // anchor-callee createGroundConnection; thiscall, ret 0x10
void type_random_map_generator::openConnectionPath(
    TRmgMapPosition position, unsigned char narrow)
{
    TRmgMapItem* item = m_map.getMapItem(position);
    int zone = item->m_zoneState.m_zone;
    if (item->m_movement.m_cost >= 30000)
        return;
    while (item->m_movement.m_cost > 0) {
        if (item->m_connection.m_present) {
            TRmgObjectPropertiesRef* properties = selectObjectPrototype(
                eTerrainDirt, BORDER_GUARD, item->m_connection.m_direction);
            type_object* object = new type_object(properties);
            item->m_connection.m_present = 0;
            item->m_connection.m_direction = 0;
            if (!item->m_connection.m_present) {
                item->m_tileData.m_borderObject = 0;
                item->m_tileData.m_subterraneanGate = 1;
            }
            addObject(object, position);
        }
        if (!item->m_connection.m_present) {
            item->m_tileData.m_borderObject = 0;
            item->m_tileData.m_subterraneanGate = 1;
        }
        TRmgMapPosition previous = item->m_previousTile;
        if (!narrow) {
            TRmgZoneBounds bounds;
            bounds.m_minimumX = max(position.m_x - 1, 0);
            bounds.m_minimumY = max(position.m_y - 1, 0);
            bounds.m_maximumX = min(position.m_x + 2, m_map.m_mapWidth);
            bounds.m_maximumY = min(position.m_y + 2, m_map.m_mapHeight);
            for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
                for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
                    TRmgMapItem* nearby = m_map.getMapItem(x, y, position.m_z);
                    if (nearby->m_zoneState.m_zone == zone
                        && !nearby->m_connection.m_present)
                        nearby->m_tileData.m_borderObject = 0;
                }
            }
        }
        position = previous;
        item = m_map.getMapItem(position);
    }
}

// Zone restrictions select a creature, the requested value determines its
// count, and the result is a newly allocated guarded object (vtbl 0x640a84).
// Retail fills 145 prototype indices, filters creature traits in reverse,
// then selects in reverse across all 145 slots (also for RoE). Do not clear
// slot 117 in the RoE exclusion or require a prototype in the eligibility
// count: neither extra check occurs in the retail body. The two variation
// draws are ordered; the first remainder is reduced by the second.
// Source-family residual (95.6907%): predecrement restores 30 retail CFG
// blocks, with 27 exact shapes and all four calls. Assigning the version
// limit before exclusion and writing derived fields in the constructor body
// restores the corresponding lifetimes; the limit setup, count spill and
// constructor store scheduling remain different. Local/parameter count and
// paired/delta draws were measured; keeping the real intermediate is best.
VA(0x00540B20, 0x240) // anchor-callee 0x54203b; thiscall, ret 8; retail-only
type_object* type_random_map_generator::createGuard(int value, TRmgZone* zone)
{
    unsigned char allowed[10];
    if (zone->m_slot->m_flag0094 && zone->m_alignment != -1) {
        memset(allowed, 0, sizeof(allowed));
        allowed[zone->m_alignment + 1] = 1;
    } else {
        memcpy(allowed, zone->m_slot->m_allowedMonsters, sizeof(allowed));
    }
    int prototypeIndices[RMG_GUARD_CREATURE_COUNT];
    memset(prototypeIndices, -1, sizeof(prototypeIndices));
    for (unsigned int index = 0; index < m_objectPrototypes[MONSTER].size(); ++index) {
        TRmgObjectPropertiesRef* properties = m_objectPrototypes[MONSTER][index];
        prototypeIndices[properties->m_prototype->m_subtype] = index;
    }
    int eligibleCount = 0;
    int creatureLimit = RMG_GUARD_CREATURE_COUNT;
    if (m_mapVersion < RMG_MAP_ARMAGEDDONS_BLADE) {
        creatureLimit = RMG_GUARD_ROE_CREATURE_LIMIT;
        for (int creature = RMG_GUARD_CREATURE_COUNT - 1;
             creature >= RMG_GUARD_ROE_EXCLUDED_FIRST; --creature)
            prototypeIndices[creature] = -1;
    }
    for (int creature = creatureLimit; --creature >= 0;) {
        const TCreatureTypeTraits& traits = g_creatureTypeTraits[creature];
        if ((traits.m_wanderingHigh + traits.m_wanderingLow) / 2 * traits.m_aiValue <= value
            && value <= traits.m_aiValue * RMG_GUARD_MAXIMUM_COUNT
            && traits.m_level >= 0 && allowed[traits.m_townType + 1]) {
            ++eligibleCount;
        } else {
            prototypeIndices[creature] = -1;
        }
    }
    if (!eligibleCount)
        return 0;
    int chosen = rand() % eligibleCount;
    for (creature = RMG_GUARD_CREATURE_COUNT - 1; creature >= 0; --creature) {
        if (prototypeIndices[creature] >= 0 && --chosen < 0)
            break;
    }
    TRmgObjectPropertiesRef* properties = m_objectPrototypes[MONSTER][prototypeIndices[creature]];
    int aiValue = g_creatureTypeTraits[creature].m_aiValue;
    int count = (value + aiValue / 2) / aiValue;
    int variation = count / 4 + 1;
    if (variation > 1) {
        int delta = rand() % variation;
        delta -= rand() % variation;
        count += delta;
    }
    return new rmgMonsterObject(properties, m_nextObjectId++, count);
}

// The shipyard caller at 0x541fc5 passes its entrance, count 3 and destination.
// Retail selects matching BORDER_TENT/BORDER_GUARD prototypes by color, places
// the tent in the destination zone, then lays adjacent guards at the entrance.
// Missing tent returns -1; missing guard returns 0, as the two retail exits
// at 0x540dbf and 0x540e1f prove. These source names are Complete-only roles.
// Exact: 598/598 raw retail bytes after resolving all five relocations.
// Reuse index in all three loops. A separate guardIndex changes only the
// SIB bytes at 0x540f6a and 0x540f9e (596/598 bytes, 99.90566%). Naming
// byte-vector bases or using begin()[index] leaves those two bytes wrong.
// The entrance clears borderObject (bit 26) and sets subterraneanGate (27);
// swapped flags can hide behind the fuzzy score, so verify raw operands.
VA(0x00540D60, 0x256) // anchor-callee createShipyardConnection; thiscall, ret 0x14
int type_random_map_generator::placeBorderObject(
    TRmgMapPosition position, int count, TRmgZone* zone)
{
    int color = m_nextKeyTentColor;
    int index = 0;
    for (; index < m_objectPrototypes[BORDER_TENT].size(); ++index) {
        if (m_objectPrototypes[BORDER_TENT][index]->m_prototype->m_subtype == color)
            break;
    }
    if (index == m_objectPrototypes[BORDER_TENT].size())
        return -1;
    TRmgObjectPropertiesRef* tentProperties = m_objectPrototypes[BORDER_TENT][index];

    index = 0;
    for (; index < m_objectPrototypes[BORDER_GUARD].size(); ++index) {
        if (m_objectPrototypes[BORDER_GUARD][index]->m_prototype->m_subtype == color)
            break;
    }
    if (index == m_objectPrototypes[BORDER_GUARD].size())
        return 0;
    TRmgObjectPropertiesRef* guardProperties = m_objectPrototypes[BORDER_GUARD][index];
    type_object* tent = new type_object(tentProperties);
    if (!placeObjectInZone(tent, zone)) {
        delete tent;
        return -1;
    }

    for (index = 0; index < count; ++index) {
        type_object* guard = new type_object(guardProperties);
        TRmgMapItem* item = m_map.getMapItem(position);
        item->m_connection.m_present = 0;
        item->m_connection.m_direction = 0;
        if (!item->m_connection.m_present) {
            item->m_tileData.m_borderObject = 0;
            item->m_tileData.m_subterraneanGate = 1;
        }
        addObject(guard, position);
        ++position.m_x;
    }

    m_disabledKeyTents[color] = 1;
    m_nextKeyTentColor = 0;
    while (m_nextKeyTentColor < m_disabledKeyTents.size()
           && m_disabledKeyTents[m_nextKeyTentColor])
        ++m_nextKeyTentColor;
    return color;
}

// Both ground border placements call this with their returned direction.
// Retail clips the surrounding rectangle and updates connection/obstacle bits.
// Four signed min/max selections bound the 3x3 area. Only empty object cells
// receive the connection decoration; the center's predecessor is then cleared
// if its x/y coordinates are valid. The post-clear present test also occurs
// in placeBorderObject's exact body and remains explicit here.
// The generated family closes all 370 bytes with a rectangle value, whose
// four fields retain retail's 0x18 frame. Independent scalar bounds preserve
// the instructions but reuse their homes and shrink the frame to 0x10 (99.9143).
VA(0x00540FC0, 0x172) // anchor-callee createGroundConnection; thiscall, ret 0x10
void type_random_map_generator::markBorderObjectArea(
    TRmgMapPosition position, int direction)
{
    TRmgZoneBounds bounds;
    bounds.m_minimumX = max(position.m_x - 1, 0);
    bounds.m_maximumX = min(position.m_x + 2, m_map.m_mapWidth);
    bounds.m_minimumY = max(position.m_y - 1, 0);
    bounds.m_maximumY = min(position.m_y + 2, m_map.m_mapHeight);
    for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
        for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
            TRmgMapItem* item = m_map.getMapItem(x, y, position.m_z);
            if (item->m_objects.size() == 0) {
                if (!item->m_connection.m_present) {
                    item->m_tileData.m_subterraneanGate = 0;
                    item->m_tileData.m_borderObject = 1;
                }
                item->m_connection.m_direction = direction;
                item->m_connection.m_present = 1;
            }
        }
    }
    TRmgMapPosition previous = m_map.getMapItem(position)->m_previousTile;
    if (previous.m_x >= 0 && previous.m_x < m_map.m_mapWidth
        && previous.m_y >= 0 && previous.m_y < m_map.m_mapHeight) {
        TRmgMapItem* item = m_map.getMapItem(previous);
        item->m_connection.m_present = 0;
        item->m_connection.m_direction = 0;
        if (!item->m_connection.m_present) {
            item->m_tileData.m_borderObject = 0;
            item->m_tileData.m_subterraneanGate = 1;
        }
    }
}

// Provisional arithmetic boundary for the Complete-only position value.
// CreateRiver supports a by-value direction. Ground connection retains the
// original coordinate before translation at 0x54128c..0x5412be and
// 0x5414cb..0x5414ff. Construct that value, apply the canonical compound
// translation, then return it. This remains an ordinary helper.
// With ground's source-slot locals, this reaches 81.81753%; the prior direct
// translated construction reaches 78.071556%. Copying *this instead of
// constructing the coordinate collapses the temporary (76.631485% without
// the slot locals). The precise constructor expansion remains unresolved.
// The map-position source family preserves implicit special members and
// tests real copy/return/field orders. Direct-copy then returning += brings
// CreateRiver's frame to retail 0xbc and 53 CFG blocks to exact shape
// (75.6332%, versus 73.2613% and nine blocks with coordinate construction).
// Ground/flood collateral CUR falls; their unchanged own-source MAX stays.
// An explicit x/z/y copy constructor restores OpenConnectionPath's initial
// lookup and 0x2c frame, but emits an extra lookup and disrupts retained STL
// copies: do not infer that special member from its 82.0101% score alone.
// New positive evidence: markRiverCoastTarget at 0x548a75/0x548b2c calls
// the three-coordinate constructor on already translated x/y. Direct
// translated construction restores both named calls; copy-plus-compound
// omits both. Coast scores 65.57% -> 75.98%, then x/y compound order 76.72%.
// Measured constructor collateral: GroundConnection 78.07%, ConnectZones
// 93.62%, OpenConnectionPath 79.97%, CreateRiver 85.73%. Prior peaks stay
// banked; lower scores do not refute these newly proven constructor calls.
TRmgMapPosition TRmgMapPosition::operator+(TPoint offset) const
{
    return TRmgMapPosition(m_x + offset.m_x, m_y + offset.m_y, m_z);
}

TRmgMapPosition& TRmgMapPosition::operator+=(const TPoint& offset)
{
    m_x += offset.m_x;
    m_y += offset.m_y;
    return *this;
}

TRmgMapPosition& TRmgMapPosition::operator-=(const TPoint& offset)
{
    m_x -= offset.m_x;
    m_y -= offset.m_y;
    return *this;
}

// The ground, shipyard and gate paths share this placement sequence.
// Shipyard retains an independent y/z coordinate copy at 0x541ff8/0x54200c,
// consistent with this by-value helper boundary. Name and boundary are a
// retail-only hypothesis. All five connection sites use this ordinary body;
// flattening the ground copies loses its final retained map-item accesses.
// Shipyard is byte-neutral versus a flat body with a separate position copy.
void type_random_map_generator::placeGuard(TRmgMapPosition position, int value)
{
    TRmgMapItem* item = m_map.getMapItem(position);
    TRmgZone* zone = m_zones[item->m_zoneState.m_zone];
    if (static_cast<int>(item->m_objects.size()) > 0)
        return;
    type_object* guard = createGuard(value, zone);
    if (guard)
        addObject(guard, position);
}

// Complete-only ground connection pass.  ConnectZones passes the paired
// boundary item/position vectors.  Retail selects all equally cheap empty
// crossings, opens their predecessor paths, and records both zone entrances
// before choosing border objects or a guard.  There is no Dreamcast RMG
// counterpart; the helper names describe their retained retail bodies.
// Residual (81.81753%): retail preserves coordinate copies before translation;
// construct the ordinary addition helper's result before applying its offset.
// The candidate still retains two coordinate constructors that retail expands;
// its frame is 0x50 versus retail 0x5c.
// The final guard accessors call the scalar overload instead of the value
// overload, and the second guard's occupancy size remains out of line.
// Source-slot locals reproduce source-index-before-destination lookup; alone
// they score 78.071556%, combined with the value construction 81.81753%.
// Controls on the prior helper: a separate scan position copy is 78.48837%;
// replacing the first/both additions with caller-side copy/+= is
// 78.701256%/78.31127%; entrance locals are 76.9034%, guard-input locals
// 77.27907%. Keep the canonical helper calls. Inside the helper, returning
// the += reference is 81.15385% versus a separate return's 81.63685% before
// slot locals; copy-initializing from a constructed temporary is 75.432915%.
// Older controls: direct range erase expands further (71.29874%); naming
// candidateCount before the empty test is 75.386406%. The two size calls
// preserve retail's count reuse through min. No inline pin is retained.
VA(0x00541140, 0x63A) // anchor-callee ConnectZones 0x543550; retail-only
unsigned char type_random_map_generator::createGroundConnection(
    TRmgZone* source,
    TRmgZoneConnection* connection,
    std::vector<TRmgMapItem*>* borderItems,
    std::vector<TRmgMapPosition>* borderPositions)
{
    TRmgTownSlot* sourceSlot = source->m_slot;
    int sourceZone = sourceSlot->m_zoneIndex;
    TRmgTownSlot* destinationSlot = connection->m_destination;
    TRmgZone* destination = m_zones[destinationSlot->m_zoneIndex];
    int destinationZone = destination->m_slot->m_zoneIndex;
    if (source->getLevelPosition().m_z != destination->getLevelPosition().m_z)
        return 0;
    if (source->m_terrain == eTerrainWater)
        return 0;
    if (destination->m_terrain == eTerrainWater)
        return 0;

    std::vector<TRmgMapPosition> candidates;
    int eligibleCount = 0;
    int bestCost = 100;
    for (int index = 0; index < borderItems->size(); ++index) {
        TRmgMapItem* item = (*borderItems)[index];
        if (item->m_zoneState.m_zone == sourceZone
            && item->m_zoneState.m_connectionEligibility == destinationZone
            && static_cast<int>(item->m_objects.size()) <= 0) {
            TRmgMapPosition other = (*borderPositions)[index]
                + g_rmgDirections[item->m_tileData.m_connectionDirection];
            if (static_cast<int>(m_map.getMapItem(other)->m_objects.size()) <= 0) {
                ++eligibleCount;
                int cost = item->m_movement.m_zonePathCost;
                if (cost <= bestCost) {
                    if (cost < bestCost) {
                        candidates.clear();
                        bestCost = cost;
                    }
                    candidates.push_back((*borderPositions)[index]);
                }
            }
        }
    }

    if (candidates.size() == 0)
        return 0;

    int guardValue;
    if (connection->m_unguarded) {
        guardValue = 0;
    } else {
        guardValue = getRmgGuardValue(connection->m_value, m_monsterStrength);
    }

    if (bestCost == 1 && guardValue == 0 && !connection->m_placeBorderObjects)
        return 1;

    int count = min(candidates.size(), (eligibleCount + 39) / 40);
    for (int crossing = 0; crossing < count; ++crossing) {
        int selected = rand() % candidates.size();
        TRmgMapPosition position = candidates[selected];
        TPoint direction = g_rmgDirections[
            m_map.getMapItem(position)->m_tileData.m_connectionDirection];
        TRmgMapPosition otherPosition = candidates[selected] + direction;

        openConnectionPath(candidates[selected], connection->m_placeBorderObjects);
        source->m_entrances.push_back(TPoint(position.m_x, position.m_y));
        openConnectionPath(otherPosition, connection->m_placeBorderObjects);
        destination->m_entrances.push_back(TPoint(otherPosition.m_x, otherPosition.m_y));
        candidates.erase(candidates.begin() + selected);

        if (connection->m_placeBorderObjects) {
            int borderDirection = placeBorderObject(position, 1, destination);
            if (borderDirection >= 0) {
                markBorderObjectArea(position, borderDirection);
                guardValue = 0;
            }
            borderDirection = placeBorderObject(otherPosition, 1, source);
            if (borderDirection >= 0) {
                markBorderObjectArea(otherPosition, borderDirection);
                guardValue = 0;
            }
        }

        if (guardValue > 0) {
            if (!(rand() & 1)) {
                placeGuard(position, guardValue);
            } else {
                placeGuard(otherPosition, guardValue);
            }
        }
    }
    return 1;
}

// Retail uses a LIFO vector of three-dword positions and walks cardinal
// directions 0/2/4/6 with signed induction. Mark each admitted neighbour
// visited, but enqueue only water; gate-marked water cells are traversable.
// Byte extractions at +0x11c/+0x125 and the two terrain tests fix the narrow
// predicate values. Preserve the ordinary map and point helper boundaries.
// Source-family result: a value insert at the neighbour site restores the
// retained copy/_Destroy pair in pop_back (71.4786 -> 84.8000). Assigning
// rather than copy-initializing nearby reaches the same island. Keep the
// public STL calls; the seed insert still expands one level past retail.
VA(0x00541780, 0x18D) // anchor-callee 0x541f1f; thiscall, ret 0x0c
void type_random_map_generator::floodConnectionRegion(TRmgMapPosition position)
{
    std::vector<TRmgMapPosition> openPositions;
    openPositions.push_back(position);
    m_map.getMapItem(position)->m_tileData.m_connectionVisited = 1;
    while (openPositions.size()) {
        position = openPositions.back();
        openPositions.pop_back();
        for (int direction = 0; direction < 8; direction += 2) {
            TRmgMapPosition nearby = position;
            nearby += g_rmgDirections[direction];
            if (nearby.m_x < 0 || nearby.m_x >= m_map.m_mapWidth
                || nearby.m_y < 0 || nearby.m_y >= m_map.m_mapHeight)
                continue;
            TRmgMapItem* item = m_map.getMapItem(nearby);
            unsigned char visited = item->m_tileData.m_connectionVisited;
            if (visited)
                continue;
            if (!item->hasSubterraneanGate()) {
                unsigned char terrain = item->m_tile.m_landType;
                if (terrain == eTerrainWater)
                    continue;
            }
            item->m_tileData.m_connectionVisited = 1;
            unsigned char terrain = item->m_tile.m_landType;
            if (terrain == eTerrainWater)
                openPositions.insert(openPositions.end(), nearby);
        }
    }
}

// Retail first checks the six land cells at x-2..x, y..y+1, then searches
// the four signed water offsets for a gate-marked water tile. The opposite
// side must be in bounds and non-water. Footprint x validity is a caller
// precondition: retail checks only y+1 here, and only x for the side offsets.
// Preserve its ordered water/entrance/passability/rock tests and byte queries.
// Source-family result: copy the side offset after the position assignment,
// and preserve the terrain enum through both snapshots (83.4060 -> 90.7594).
// Narrow byte locals retain sign-extension shifts absent from retail; the
// enum snapshots instead fold to and/cmp byte. A conditional opposite-x is
// byte-neutral. Returned-point translation and keeping only the old z do
// not recover the remaining coordinate homes/registers or side-branch shape.
VA(0x00541960, 0x16C) // anchor-callee 0x541c94; thiscall, ret 0x0c
unsigned char type_random_map_generator::canPlaceShipyard(TRmgMapPosition position)
{
    if (position.m_y + 1 >= m_map.m_mapHeight)
        return 0;
    TRmgMapPosition nearby = position;
    for (nearby.m_y = position.m_y; nearby.m_y <= position.m_y + 1; ++nearby.m_y) {
        for (nearby.m_x = position.m_x - 2; nearby.m_x <= position.m_x; ++nearby.m_x) {
            TRmgMapItem* item = m_map.getMapItem(nearby);
            if (item->m_tile.m_landType == eTerrainWater)
                return 0;
            unsigned char entrance = item->m_tileData.m_roadEntrance;
            if (entrance || !item->m_tileData.m_roadPassable
                || item->m_tile.m_landType == eTerrainRock)
                return 0;
        }
    }
    int waterOffset;
    for (waterOffset = 0; waterOffset < RMG_SHIPYARD_WATER_OFFSET_COUNT; ++waterOffset) {
        nearby = position;
        TPoint offset = g_rmgShipyardWaterOffsets[waterOffset];
        nearby += offset;
        if (nearby.m_x < 0 || nearby.m_x >= m_map.m_mapWidth)
            continue;
        TRmgMapItem* item = m_map.getMapItem(nearby);
        int terrain = item->m_tile.m_landType;
        if (terrain == eTerrainWater && item->hasSubterraneanGate())
            break;
    }
    if (waterOffset == RMG_SHIPYARD_WATER_OFFSET_COUNT)
        return 0;
    nearby = position;
    if (g_rmgShipyardWaterOffsets[waterOffset].m_x < 0)
        ++nearby.m_x;
    else
        nearby.m_x -= 3;
    if (nearby.m_x < 0 || nearby.m_x >= m_map.m_mapWidth)
        return 0;
    int terrain = m_map.getMapItem(nearby)->m_tile.m_landType;
    return terrain != eTerrainWater;
}

// Complete-only shipyard connection pass. connectZones calls this at
// 0x54356a and 0x5437f8 after a failed ground connection. Retail selects
// prototype 87 (SHIPYARD), scans the source zone's eligible coastal cells,
// places an ownable object, then opens its water route and entrance guard.
// Names are inferred roles; there is no Dreamcast RMG compiland.
// Residual (94.8591%): branch destinations and polarities agree, and the
// retained constructor/call sequence matches. Destination-lookup registers,
// the two trigger-x stores and the final guard registers/temporary still differ.
// Keep one nearby coordinate across scanning and placement: retail reuses
// EBP-0x30. The scan initializes only z; copying a whole position extends the
// wrong coordinate lifetimes. Capture visited as a byte for the dword load,
// shift and byte-test at 0x541c15. Name the prototype index before operator[]
// so the vector base reloads after rand instead of surviving across it.
// A trigger-offset value evaluated before nearby's copy recovers both paired
// loads and subtraction order. Direct entrance-y increment, the object's
// value-returning position accessor, and a copied water offset with offset
// first in the additions recover the entrance/water instructions and slots.
// Capture strength before requested value: either input alone leaves the
// table calculation's registers wrong. Clear guardValue on border success;
// a shared success label produces identical bytes, including the direct edge.
// The late value addition preserves the retained base constructor naturally.
// Frozen-front-end C2 tracing measures nested budget 95 against base cost 96;
// no pragma, alternate declaration or release-elided carrier is retained.
// Controls: plain final ++y expands the base constructor and retains reset
// (91.93% before opening fixes); compound += reuses an earlier y+1 unlike
// retail. A default result with direct field calculations removes the extra
// x store (95.50%) but changes frame homes and also reuses that earlier y+1.
// Copy-initialize versus assign the addition's local, and a free by-value
// left operand, are byte-neutral. A full saved entrance coordinate grows the
// frame; a position setter changes the retained constructor/reset boundary.
// Separate nearby locals, value-returning trigger subtraction, a full scan
// position copy and reversed water addends all lose matching instructions.
// Initializing guardValue to zero before its test changes the branch shape;
// normalizing the requested input in guardValue is neutral. Rewriting the
// cutoff as assignment breaks the independently exact retained value helper.
// Naming the source slot/value before the destination slot improves the
// opening; adding a separate destination index reverses that improvement.
// Further controls: a saved TPoint entrance adds a four-byte stack home;
// initializing guardValue from the requested value before the policy test
// hoists its load into the unguarded path. A const conditional result changes
// table registers but still spills guardValue. Reusing the prototype index
// in the water loop is neutral. Value-returning trigger subtraction with a
// plain final increment grows the frame and expands the base constructor.
// A by-value addition offset is neutral here but moves the ground caller
// from 80.43471% to 76.63149%, without resolving this final coordinate copy.
// An entry-wide zero initialization grows the frame from 0x68 to 0x6c
// (93.24324%) and still homes guardValue. Consuming operator-='s returned
// reference for entranceX, and returning the named operator+ result after
// a separate += statement, are both byte-neutral at 94.85907%.
// A long guardValue is also neutral. A separate boolean guard decision
// emits an absent setg/byte home and changes table registers (93.57529%).
// A reference to the chosen candidate shrinks the frame to 0x5c and removes
// retail's coordinate value homes (89.96718%); retain the value copy.
// Moving only guardValue's declaration before the candidate vector is
// neutral. Returning immediately on border success adds a cleanup branch
// and reverses the border-result branch (94.02123%); keep the shared exit.
// A short-lived entrance value copied into nearby still grows the frame to
// 0x74 and changes the trigger-load/subtraction sequence (92.79536%).
VA(0x00541AD0, 0x5B0) // anchor-callee connectZones; thiscall, ret 8; retail-only
unsigned char type_random_map_generator::createShipyardConnection(
    TRmgZone* source, TRmgZoneConnection* connection)
{
    TRmgTownSlot* sourceSlot = source->m_slot;
    int sourceZone = sourceSlot->m_zoneIndex;
    TRmgTownSlot* destinationSlot = connection->m_destination;
    TRmgZone* destination = m_zones[destinationSlot->m_zoneIndex];
    int destinationZone = destination->m_slot->m_zoneIndex;
    if (source->getLevelPosition().m_z != destination->getLevelPosition().m_z)
        return 0;

    std::vector<TRmgMapPosition> candidates;
    int prototypeIndex = rand() % m_objectPrototypes[SHIPYARD].size();
    TRmgObjectPropertiesRef* properties = m_objectPrototypes[SHIPYARD][prototypeIndex];
    TObjectType* prototype = properties->m_prototype;
    TRmgMapPosition nearby;
    {
        TRmgZoneBounds bounds = source->m_bounds;
        TRmgMapPosition position;
        position.m_z = source->getLevelPosition().m_z;
        for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY;
             ++position.m_y) {
            for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX;
                 ++position.m_x) {
                TRmgMapItem* item = m_map.getMapItem(position);
                if (item->m_zoneState.m_zone == sourceZone
                    && item->m_zoneState.m_connectionEligibility == destinationZone) {
                    unsigned char visited = item->m_tileData.m_connectionVisited;
                    if (visited)
                        return 1;
                    if (item->m_tile.m_landType != eTerrainWater) {
                        nearby = position;
                        if (nearby.m_y + 1 < m_map.m_mapHeight) {
                            for (nearby.m_x = position.m_x;
                                 nearby.m_x <= position.m_x + 2; ++nearby.m_x) {
                                if (m_map.canPlaceObject(properties, nearby, source)
                                    && canPlaceShipyard(nearby))
                                    candidates.push_back(nearby);
                            }
                        }
                    }
                }
            }
        }
    }
    if (candidates.size() == 0)
        return 0;

    rmgOwnableObject* shipyard = new rmgOwnableObject(properties);
    TRmgMapPosition position = candidates[rand() % candidates.size()];
    addObject(shipyard, position);

    {
        TPoint triggerOffset(prototype->m_triggerCell.m_x, prototype->m_triggerCell.m_y);
        nearby = position;
        nearby -= triggerOffset;
    }
    int entranceX = nearby.m_x;
    m_roadTargets.push_back(nearby);

    nearby = position;
    ++nearby.m_y;
    for (nearby.m_x = position.m_x - prototype->getWidth() + 1;
         nearby.m_x <= position.m_x; ++nearby.m_x) {
        TRmgMapItem* item = m_map.getMapItem(nearby);
        if (!item->m_connection.m_present) {
            item->m_tileData.m_borderObject = 0;
            item->m_tileData.m_subterraneanGate = 1;
        }
        source->m_entrances.push_back(TPoint(nearby.m_x, nearby.m_y));
    }

    TRmgMapPosition shipyardPosition = shipyard->getPosition();
    TRmgMapPosition waterPosition;
    int waterOffset = 0;
    for (; waterOffset < RMG_SHIPYARD_WATER_OFFSET_COUNT; ++waterOffset) {
        TPoint offset = g_rmgShipyardWaterOffsets[waterOffset];
        waterPosition = TRmgMapPosition(
            offset.m_x + shipyardPosition.m_x,
            offset.m_y + shipyardPosition.m_y,
            shipyardPosition.m_z);
        if (waterPosition.m_x >= 0 && waterPosition.m_x < m_map.m_mapWidth
            && m_map.getMapItem(waterPosition)->m_tile.m_landType == eTerrainWater)
            break;
    }
    if (waterOffset != RMG_SHIPYARD_WATER_OFFSET_COUNT)
        floodConnectionRegion(waterPosition);

    int guardValue;
    if (connection->m_unguarded)
        guardValue = 0;
    else {
        int strength = m_monsterStrength;
        int value = connection->m_value;
        guardValue = getRmgGuardValue(value, strength);
    }

    if (connection->m_placeBorderObjects) {
        nearby.m_x = entranceX - 1;
        if (placeBorderObject(nearby, 3, destination) >= 0)
            guardValue = 0;
    }
    if (guardValue > 0) {
        nearby = position + TPoint(0, 1);
        nearby.m_x = entranceX;
        placeGuard(nearby, guardValue);
    }
    return 1;
}

// Complete-only subterranean connection pass.  The caller walks paired
// 0x1c-byte zone-connection records and invokes this method only when the two
// zones are on different levels.  Retail proves the source algorithm through
// the intersection bounds, the vector of equal-best legal positions, two
// type_object constructions from objectPrototypes[103], and the mirrored
// entrance/guard updates.  Dreamcast has no RMG compiland, so the method name
// is role-based while every field and branch below is Windows-retail evidence.
// Retail reloads the source level-position after rand(), so keep the
// returned accessor value at that point rather than reusing the early
// level comparison. This raises 79.4987% to 79.8603%. Applying the existing
// coordinate subtraction helper gives 80.8917%; separate entrance-point
// temporaries for each zone give 81.9530%; updating the intersection in
// sourceBounds gives 82.1005% (2026-09-07). The resulting candidate has
// 2192 padded bytes. Position accessors versus direct reloaded members,
// size() versus empty(), placing score after the zone check, and using
// returned temporaries for the first level comparison are byte-flat.
// Remaining differences include bound-temporary/frame allocation and
// over-expansion of the two final placeGuard -> getMapItem calls. Keep
// those ordinary helper boundaries; source fact recovery, not forced
// calls or extracted arbitrary blocks, must recover their inline split.
VA(0x00542080, 0x8AA)
unsigned char type_random_map_generator::createSubterraneanGate(
    TRmgZone* source, TRmgZoneConnection* connection)
{
    TRmgZone* destination = m_zones[connection->m_destination->m_zoneIndex];
    int sourceZone = source->m_slot->m_zoneIndex;
    int destinationZone = destination->m_slot->m_zoneIndex;
    if (source->getLevelPosition().m_z == destination->getLevelPosition().m_z)
        return 0;
    if (source->m_terrain == eTerrainWater)
        return 0;

    TRmgZoneBounds sourceBounds = source->m_bounds;
    TRmgZoneBounds destinationBounds = destination->m_bounds;
    sourceBounds.m_minimumX = std::_cpp_max(
        sourceBounds.m_minimumX, destinationBounds.m_minimumX);
    sourceBounds.m_minimumY = std::_cpp_max(
        sourceBounds.m_minimumY, destinationBounds.m_minimumY);
    sourceBounds.m_maximumX = std::_cpp_min(
        sourceBounds.m_maximumX, destinationBounds.m_maximumX);
    sourceBounds.m_maximumY = std::_cpp_min(
        sourceBounds.m_maximumY, destinationBounds.m_maximumY);
    if (sourceBounds.m_minimumX >= sourceBounds.m_maximumX || sourceBounds.m_minimumY >= sourceBounds.m_maximumY)
        return 0;

    int gateIndex = rand() % m_objectPrototypes[103].size();
    TRmgObjectPropertiesRef* gateProperties = m_objectPrototypes[103][gateIndex];
    TObjectType* gatePrototype = gateProperties->m_prototype;

    std::vector<TRmgMapPosition> candidates;
    int bestScore = 0;
    TRmgMapPosition position = source->getLevelPosition();

    for (position.m_y = sourceBounds.m_minimumY; position.m_y < sourceBounds.m_maximumY; ++position.m_y) {
        for (position.m_x = sourceBounds.m_minimumX; position.m_x < sourceBounds.m_maximumX; ++position.m_x) {
            TRmgMapItem* sourceItem = m_map.getMapItem(position);
            if (sourceItem->m_zoneState.m_zone != sourceZone)
                continue;
            int score = sourceItem->m_zoneState.m_score;

            TRmgMapPosition otherPosition = destination->getLevelPosition();
            otherPosition.m_x = position.m_x;
            otherPosition.m_y = position.m_y;
            TRmgMapItem* destinationItem = m_map.getMapItem(otherPosition);
            if (destinationItem->m_zoneState.m_zone != destinationZone)
                continue;

            score += destinationItem->m_zoneState.m_score;
            if (score < bestScore)
                continue;
            if (!m_map.canPlaceObject(gateProperties, position, source))
                continue;
            if (!m_map.canPlaceObject(
                    gateProperties, otherPosition, destination))
                continue;

            if (score > bestScore) {
                candidates.clear();
                bestScore = score;
            }
            candidates.push_back(position);
        }
    }

    if (candidates.size() == 0)
        return 0;

    position = candidates[rand() % candidates.size()];
    addObject(new type_object(gateProperties), position);

    TRmgMapPosition otherPosition = destination->getLevelPosition();
    otherPosition.m_x = position.m_x;
    otherPosition.m_y = position.m_y;
    addObject(new type_object(gateProperties), otherPosition);

    position -= TPoint(gatePrototype->m_triggerCell.m_x,
                       gatePrototype->m_triggerCell.m_y);
    otherPosition = destination->getLevelPosition();
    otherPosition.m_x = position.m_x;
    otherPosition.m_y = position.m_y;
    source->m_entrances.push_back(TPoint(position.m_x, position.m_y));
    destination->m_entrances.push_back(
        TPoint(otherPosition.m_x, otherPosition.m_y));

    int guardValue;
    if (connection->m_unguarded) {
        guardValue = 0;
    } else {
        guardValue = getRmgGuardValue(connection->m_value, m_monsterStrength);
    }

    ++position.m_y;
    ++otherPosition.m_y;
    TRmgMapItem* sourceEntrance = m_map.getMapItem(position);
    if (!sourceEntrance->m_connection.m_present) {
        sourceEntrance->m_tileData.m_borderObject = 0;
        sourceEntrance->m_tileData.m_subterraneanGate = 1;
    }
    TRmgMapItem* destinationEntrance = m_map.getMapItem(otherPosition);
    if (!destinationEntrance->m_connection.m_present) {
        destinationEntrance->m_tileData.m_borderObject = 0;
        destinationEntrance->m_tileData.m_subterraneanGate = 1;
    }

    if (connection->m_placeBorderObjects) {
        int direction = placeBorderObject(position, 1, destination);
        if (direction >= 0) {
            --position.m_x;
            guardValue = 0;
            TRmgMapItem* item = m_map.getMapItem(position);
            if (item->m_objects.size() == 0) {
                if (!item->m_connection.m_present) {
                    item->m_tileData.m_subterraneanGate = 0;
                    item->m_tileData.m_borderObject = 1;
                }
                item->m_connection.m_direction = direction;
                item->m_connection.m_present = 1;
            }

            position.m_x += 2;
            item = m_map.getMapItem(position);
            if (item->m_objects.size() == 0) {
                if (!item->m_connection.m_present) {
                    item->m_tileData.m_subterraneanGate = 0;
                    item->m_tileData.m_borderObject = 1;
                }
                item->m_connection.m_direction = direction;
                item->m_connection.m_present = 1;
            }
        }

        direction = placeBorderObject(otherPosition, 1, source);
        if (direction >= 0) {
            --otherPosition.m_x;
            TRmgMapItem* item = m_map.getMapItem(otherPosition);
            if (item->m_objects.size() == 0) {
                if (!item->m_connection.m_present) {
                    item->m_tileData.m_subterraneanGate = 0;
                    item->m_tileData.m_borderObject = 1;
                }
                item->m_connection.m_direction = direction;
                item->m_connection.m_present = 1;
            }

            otherPosition.m_x += 2;
            item = m_map.getMapItem(otherPosition);
            if (item->m_objects.size() == 0) {
                if (!item->m_connection.m_present) {
                    item->m_tileData.m_subterraneanGate = 0;
                    item->m_tileData.m_borderObject = 1;
                }
                item->m_connection.m_direction = direction;
                item->m_connection.m_present = 1;
            }
            return 1;
        }
    }

    if (guardValue > 0) {
        placeGuard(position, guardValue);
        placeGuard(otherPosition, guardValue);
    }

    return 1;
}

// Called by placeBorderObject at 0x540e81 with a newly created tent and zone.
// Scans the zone bounds for matching cells accepted by canPlaceObject, then
// chooses a candidate through rand and forwards it to virtual addObject.
// Retail copies the four bounds and the zone's level coordinate, offsets
// minimum x/y by the prototype footprint minus one, and walks y then x.
// The chosen candidate is assigned back to that coordinate before the
// virtual call; keep both the real coordinate copy and the placement helper.
// The source family recovers retail's height-before-width adjustment and
// keeps an assigned loop coordinate (93.5774%). All 15 blocks and six calls
// agree in shape/order; the final success block remains one instruction
// short. Direct/copy-selected arguments lose its duplicate homes; public
// vector insertion alternatives do not resolve that retained copy either.
VA(0x00542930, 0x1C6) // anchor-callee 0x540e81; thiscall, ret 8; retail-only
unsigned char type_random_map_generator::placeObjectInZone(type_object* object, TRmgZone* zone)
{
    TRmgObjectPropertiesRef* properties = object->m_properties;
    TObjectType* prototype = properties->m_prototype;
    std::vector<TRmgMapPosition> candidates;
    TRmgZoneBounds bounds = zone->m_bounds;
    int zoneIndex = zone->m_slot->m_zoneIndex;
    bounds.m_minimumY += prototype->getHeight() - 1;
    bounds.m_minimumX += prototype->getWidth() - 1;
    TRmgMapPosition position;
    position = zone->m_levelPosition;
    for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
            if (m_map.getMapItem(position)->m_zoneState.m_zone == zoneIndex
                && m_map.canPlaceObject(properties, position, zone))
                candidates.push_back(position);
        }
    }
    if (!candidates.size())
        return 0;
    position = candidates[rand() % candidates.size()];
    addObject(object, position);
    return 1;
}

// Complete's connection coordinator has no Dreamcast counterpart.  Retail
// proves the three-stage source shape: collect cross-zone boundary squares,
// try each template connection through the ordinary ground/border/gate
// helpers, then repair every remaining non-water connection with shipyard
// reachability and monolith placement.  Helper spellings are role-based until
// their own bodies are admitted, but the calls and signatures are fixed by
// this function's ABI and retail CFG.  The current candidate has retail's
// 96-block / 57-branch shape and 14-call order.  Its remaining source-level
// residuals are VC6's excess expansion of the two initial vector inserts and
// register/layout choices around the paired-connection searches.
VA(0x00543240, 0x797)
void type_random_map_generator::connectZones()
{
    std::vector<TRmgMapItem*> borderItems;
    std::vector<TRmgMapPosition> borderPositions;

    TRmgMapItem* mapItem = m_map.m_mapItems;
    TRmgMapPosition position;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth;
                 ++position.m_x, ++mapItem) {
                if (mapItem->m_zoneState.m_connectionEligibility < 0)
                    continue;

                if (mapItem->m_tile.m_landType == eTerrainWater
                    || !mapItem->m_tileData.m_roadPassable
                    || mapItem->m_tile.m_landType == eTerrainRock)
                    continue;

                int direction = mapItem->m_tileData.m_connectionDirection;
                TRmgMapItem* otherMapItem = m_map.getMapItem(
                    TRmgMapPosition(
                        position.m_x + g_rmgDirections[direction].m_x,
                        position.m_y + g_rmgDirections[direction].m_y,
                        position.m_z));
                if (otherMapItem->m_tile.m_landType != eTerrainWater
                    && otherMapItem->m_zoneState.m_zone
                           != mapItem->m_zoneState.m_zone) {
                    borderItems.insert(borderItems.end(), mapItem);
                    borderPositions.insert(
                        borderPositions.end(), position);
                }
            }
        }
    }

    int prototypeIndex = 0;

    // Retail constructs and destroys this empty work vector.  Its element
    // type and abandoned role are not recoverable from the optimized body.
    std::vector<TRmgMapPosition> connectionPositionsScratch;

    int zoneIndex;
    for (zoneIndex = 0; zoneIndex < m_zones.size(); ++zoneIndex) {
        TRmgZone* zone = m_zones[zoneIndex];
        TRmgTownSlot* zoneTemplate = zone->m_slot;
        if (zone->m_terrain == eTerrainWater)
            continue;

        TRmgMapPosition levelPosition = zone->m_levelPosition;
        mapItem = m_map.getMapItem(0, 0, levelPosition.m_z);
        for (int remaining = m_map.m_mapWidth * m_map.m_mapHeight;
             remaining--; ++mapItem)
            mapItem->m_tileData.m_connectionVisited = 0;

        for (int connectionIndex = 0;
             connectionIndex < zoneTemplate->m_connections.size();
             ++connectionIndex) {
            TRmgZoneConnection* connection =
                &zoneTemplate->m_connections[connectionIndex];
            if (connection->m_connected)
                continue;

            TRmgZone* destination =
                m_zones[connection->m_destination->m_zoneIndex];
            TRmgTownSlot* destinationTemplate = destination->m_slot;
            TRmgZoneConnection* oppositeConnection;
            int oppositeIndex = 0;
            for (;; ++oppositeIndex) {
                if (oppositeIndex
                    >= destinationTemplate->m_connections.size()) {
                    oppositeConnection = 0;
                    break;
                }
                if (destinationTemplate->m_connections[oppositeIndex]
                        .m_destination->m_zoneIndex == zoneIndex) {
                    oppositeConnection =
                        &destinationTemplate->m_connections[oppositeIndex];
                    break;
                }
            }

            if (createGroundConnection(
                    zone,
                    connection,
                    &borderItems,
                    &borderPositions)) {
                connection->m_connected = 1;
                oppositeConnection->m_connected = 1;
                continue;
            }

            if (createShipyardConnection(zone, connection)) {
                connection->m_connected = 1;
                continue;
            }

            if (destination->m_terrain == eTerrainWater)
                continue;

            if (createSubterraneanGate(zone, connection)) {
                connection->m_connected = 1;
                oppositeConnection->m_connected = 1;
            }
        }
    }

    for (zoneIndex = 0; zoneIndex < m_zones.size(); ++zoneIndex) {
        TRmgZone* zone = m_zones[zoneIndex];
        TRmgTownSlot* zoneTemplate = zone->m_slot;
        if (zone->m_terrain == eTerrainWater)
            continue;

        int firstConnection = 0;
        while (firstConnection < zoneTemplate->m_connections.size()
               && zoneTemplate->m_connections[firstConnection].m_connected)
            ++firstConnection;
        if (firstConnection == zoneTemplate->m_connections.size())
            continue;

        TRmgMapPosition levelPosition = zone->m_levelPosition;
        mapItem = m_map.getMapItem(0, 0, levelPosition.m_z);
        for (int remaining = m_map.m_mapWidth * m_map.m_mapHeight;
             remaining--; ++mapItem)
            mapItem->m_tileData.m_connectionVisited = 0;

        int objectIndex = 0;
        while (objectIndex < m_positions.size()) {
            type_object* object = m_positions[objectIndex];
            if (object->m_properties->m_prototype->m_objectType == SHIPYARD) {
                position = object->m_position;
                if (m_map.getMapItem(position)->m_zoneState.m_zone == zoneIndex) {
                    TRmgMapPosition shipyardPosition = position;
                    int waterOffset = 0;
                    for (;
                         waterOffset < RMG_SHIPYARD_WATER_OFFSET_COUNT;
                         ++waterOffset) {
                        TRmgMapPosition waterPosition =
                            shipyardPosition
                            + g_rmgShipyardWaterOffsets[waterOffset];
                        if (waterPosition.m_x >= 0
                            && waterPosition.m_x < m_map.m_mapWidth
                            && m_map.getMapItem(waterPosition)->m_tile.m_landType
                                   == eTerrainWater)
                            break;
                    }

                    if (waterOffset != RMG_SHIPYARD_WATER_OFFSET_COUNT)
                        floodConnectionRegion(object->m_position);
                }
            }
            ++objectIndex;
        }

        for (int connectionIndex = firstConnection;
             connectionIndex < zoneTemplate->m_connections.size();
             ++connectionIndex) {
            TRmgZoneConnection* connection =
                &zoneTemplate->m_connections[connectionIndex];
            if (connection->m_connected)
                continue;

            TRmgZone* destination =
                m_zones[connection->m_destination->m_zoneIndex];
            TRmgTownSlot* destinationTemplate = destination->m_slot;
            TRmgZoneConnection* oppositeConnection;
            int oppositeIndex = 0;
            for (;; ++oppositeIndex) {
                if (oppositeIndex
                    >= destinationTemplate->m_connections.size()) {
                    oppositeConnection = 0;
                    break;
                }
                if (destinationTemplate->m_connections[oppositeIndex]
                        .m_destination->m_zoneIndex == zoneIndex) {
                    oppositeConnection =
                        &destinationTemplate->m_connections[oppositeIndex];
                    break;
                }
            }

            if (createShipyardConnection(zone, connection)) {
                connection->m_connected = 1;
                continue;
            }

            if (destination->m_terrain == eTerrainWater)
                continue;

            createMonolithConnection(
                zone, connection, prototypeIndex);
            connection->m_connected = 1;
            oppositeConnection->m_connected = 1;
            prototypeIndex = (prototypeIndex + 1)
                % (m_objectPrototypes[LITH_TWOWAY].size()
                   + m_objectPrototypes[LITH_ONEWAY_ENTRANCE].size());
        }
    }

    if (m_progress)
        m_progress->advance(0x1900);
}

// Retained by generation coordinator 0x549930; retail-only role/ABI.
#if 0 // @carcass
VA(0x005439E0, 0x283)
void type_random_map_generator::decorateUnderground() {} // @stub
#endif

// The queued side branch supplies two points by value and a level. This
// integer ray continues beyond 'toward' until the map edge or an existing
// gate-marked tile in the 3x3 neighbourhood; the first two steps ignore
// those neighbours. Return the last point before the obstruction.
// Residual 99.8683%: all 23 CFG blocks and their operations agree; independent
// reloads retain a different schedule. The 48 direction/query/lifetime forms
// lift 67.5569% to 95.9761% by copying the axial step before changing its
// diagonal component and using one mutable map-position value for the scan.
// Another 48 step-setup/update-order forms reach 99.8563%; nine x-store and
// neighbour-lifetime controls leave the present 99.8683% peak. The previous
// point snapshot precedes the error/count updates. All 105 candidates pass
// 36,504 native scenarios against a closed-form lattice-ray oracle, covering
// zero/axis/diagonal directions, map edges and several obstacle patterns.
VA(0x00543C70, 0x1A2) // anchor-callee 0x544226; Complete-only, hidden result, ret 0x18
TPoint type_random_map::traceBranchEnd(TPoint from, TPoint toward, int level)
{
    int dx = toward.m_x - from.m_x;
    int dy = toward.m_y - from.m_y;
    int major;
    int minor;
    TRmgVector axial;
    TRmgVector diagonal;
    if (abs(dx) > abs(dy)) {
        major = abs(dx);
        minor = abs(dy);
        axial.m_y = 0;
        axial.m_x = dx > 0 ? 1 : -1;
        diagonal.m_x = axial.m_x;
        diagonal.m_y = dy > 0 ? 1 : -1;
    } else {
        major = abs(dy);
        minor = abs(dx);
        axial = TRmgVector(0, dy > 0 ? 1 : -1);
        diagonal = axial;
        diagonal.m_x = dx > 0 ? 1 : -1;
    }
    int error = major / 2;
    int steps = 0;
    for (;;) {
        toward = from;
        error += minor;
        ++steps;
        if (error < major)
            from += axial;
        else {
            error -= major;
            from += diagonal;
        }
        if (from.m_x < 1 || from.m_x >= m_mapWidth - 1
            || from.m_y < 1 || from.m_y >= m_mapHeight - 1)
            return toward;
        if (steps > 2) {
            TRmgMapPosition nearby;
            nearby.m_z = level;
            for (nearby.m_x = from.m_x - 1; nearby.m_x <= from.m_x + 1; ++nearby.m_x) {
                for (nearby.m_y = from.m_y - 1; nearby.m_y <= from.m_y + 1; ++nearby.m_y) {
                    if (getMapItem(nearby)->hasSubterraneanGate())
                        return toward;
                }
            }
        }
    }
}

// The eight-byte values are coordinate pairs: midpoint and perpendicular
// arithmetic prove TPoint, independently of the ICF-shared vector labels.
// Pending segments use a vector stack; long segments enqueue two outward
// side branches as consecutive point pairs in an ordinary std::list.
// Residual 73.0200%: a 64-case point-lifetime/endpoint/container-API matrix
// lifts the initial 67.2335%; the 13-case public-list follow-up is lower or
// flat. Component endpoint stores and explicit public iterator erasure retain
// the best schedule. Retail calls vector erase at both stack pops and list
// range erase during cleanup; VC6 still expands those boundaries, with the
// latter COMDAT absent. No emission anchor or inline-depth pin is used.
VA(0x00543E20, 0x574) // anchor-callee 0x544920; Complete-only, thiscall, no arguments
void type_random_map_generator::carveBranchingPaths()
{
    TRmgMapItem* item = m_map.m_mapItems;
    for (int remaining = m_map.m_mapWidth * m_map.m_mapHeight * m_map.m_numberLevels;
         remaining--; ++item) {
        if (!item->m_objects.size()) {
            if (!item->m_connection.m_present) {
                item->m_tileData.m_subterraneanGate = 0;
                item->m_tileData.m_borderObject = 1;
            }
        } else if (!item->m_connection.m_present) {
            item->m_tileData.m_borderObject = 0;
            item->m_tileData.m_subterraneanGate = 1;
        }
    }
    for (int level = 0; level < m_map.m_numberLevels; ++level) {
        TPoint first;
        TPoint last;
        switch (rand() % 4) {
        case RMG_BRANCH_SEED_MAIN_DIAGONAL:
            first.m_x = 0;
            first.m_y = 0;
            last.m_x = m_map.m_mapWidth - 1;
            last.m_y = m_map.m_mapHeight - 1;
            break;
        case RMG_BRANCH_SEED_VERTICAL:
            first.m_x = m_map.m_mapWidth / 2;
            first.m_y = 0;
            last.m_x = first.m_x;
            last.m_y = m_map.m_mapHeight - 1;
            break;
        case RMG_BRANCH_SEED_ANTI_DIAGONAL:
            first.m_x = m_map.m_mapWidth - 1;
            first.m_y = 0;
            last.m_x = 0;
            last.m_y = m_map.m_mapHeight - 1;
            break;
        case RMG_BRANCH_SEED_HORIZONTAL:
            first.m_x = 0;
            first.m_y = m_map.m_mapHeight / 2;
            last.m_x = m_map.m_mapWidth - 1;
            last.m_y = first.m_y;
            break;
        }
        std::vector<TPoint> pending;
        std::list<TPoint> branches;
        pending.push_back(first);
        pending.push_back(last);
        while (pending.size()) {
            while (pending.size()) {
                last = pending.back();
                pending.pop_back();
                first = pending.back();
                pending.pop_back();
                TPoint middle((first.m_x + last.m_x + 1) / 2,
                    (first.m_y + last.m_y + 1) / 2);
                if (middle != first && middle != last) {
                    TRmgVector perpendicular(-(last.m_y - first.m_y), last.m_x - first.m_x);
                    int length = perpendicular.length();
                    if (length > 1) {
                        int displacement = rand() % length - length / 2;
                        middle += perpendicular * displacement / length;
                    }
                    pending.push_back(last);
                    pending.push_back(middle);
                    pending.push_back(middle);
                    pending.push_back(first);
                    if (length >= 8 && middle.m_x >= 0 && middle.m_x < m_map.m_mapWidth
                        && middle.m_y >= 0 && middle.m_y < m_map.m_mapHeight) {
                        first = middle + perpendicular;
                        branches.push_back(middle);
                        branches.push_back(first);
                        first = TPoint(middle.m_x - perpendicular.m_x, middle.m_y - perpendicular.m_y);
                        branches.push_back(middle);
                        branches.push_back(first);
                    }
                } else if (first.m_x >= 0 && first.m_x < m_map.m_mapWidth
                           && first.m_y >= 0 && first.m_y < m_map.m_mapHeight) {
                    m_map.openPathPatch(first.m_x, first.m_y, level);
                }
            }
            while (branches.size() > 0 && pending.empty()) {
                first = branches.front();
                branches.erase(branches.begin());
                last = branches.front();
                branches.erase(branches.begin());
                last = m_map.traceBranchEnd(first, last, level);
                int dx = last.m_x - first.m_x;
                int dy = last.m_y - first.m_y;
                if (dx * dx + dy * dy >= 25) {
                    pending.push_back(last);
                    pending.push_back(first);
                }
            }
        }
    }
    item = m_map.m_mapItems;
    TRmgMapPosition position;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++item) {
                if (item->m_tile.m_landType == eTerrainWater || item->m_tile.m_landType == eTerrainRock) {
                    if (!item->m_connection.m_present) {
                        item->m_tileData.m_borderObject = 0;
                        item->m_tileData.m_subterraneanGate = 1;
                    }
                }
                if (item->m_tileData.m_borderObject)
                    m_map.markBorderPatch(position);
            }
        }
    }
}

// Retained by generation coordinator 0x549930; retail-only role/ABI.
#if 0 // @carcass
VA(0x005446A0, 0x27E)
void type_random_map_generator::prepareJunctionZone(TRmgZone* zone) {} // @stub
#endif

// Retail-only generation coordinator, called at 0x549b65. After the layout
// passes it marks unassigned dry cells without objects or entrances, then
// prepares water zones and runs the shared path/border/connection passes.
// Exact: all 292 bytes. The signed object-count test, signed zone field,
// shared coordinate record and seven-call sequence preserve retail lowering.
VA(0x00544920, 0x124)
void type_random_map_generator::prepareZoneConnections()
{
    carveBranchingPaths();
    expandObstacleClearance();
    TRmgMapItem* item = m_map.m_mapItems;
    TRmgMapPosition position;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++item) {
                if (!item->hasBorderObject() && item->m_tileData.m_roadPassable
                    && item->m_tile.m_landType != eTerrainRock && !item->isRoadEntrance()
                    && static_cast<int>(item->m_objects.size()) <= 0
                    && item->m_zoneState.m_zone < 0 && item->m_tile.m_landType != eTerrainWater)
                    m_map.markBorderPatch(position);
            }
        }
    }
    for (unsigned int zone = 0; zone < m_zones.size(); ++zone)
        prepareWaterZoneConnections(m_zones[zone]);
    buildZoneConnectionPaths();
    repairWaterZoneBorders();
    connectZones();
}

// Generation's first town pass at 0x549b30. Retail tries template counts
// +0x24/+0x20 for the mapped player, then +0x34/+0x30 for neutral ownership;
// option 1 precedes option 0 in each pair. Role-derived names: option meaning
// and the template count group remain unresolved pending town serialization.
// Exact: all 144 bytes, all 12 blocks, and all four placement calls agree.
VA(0x00544A50, 0x90)
void type_random_map_generator::placePrimaryTown(TRmgZone* zone)
{
    TRmgTownSlot* slot = zone->m_slot;
    int alignment = zone->m_alignment;
    int player = m_playerIndexMap[slot->m_playerIndex + 1];
    if (slot->m_parameters0020[1] > 0
        && tryPlacePrimaryTown(zone, alignment, player, 1))
        return;
    if (slot->m_parameters0020[0] > 0
        && tryPlacePrimaryTown(zone, alignment, player, 0))
        return;
    if (slot->m_parameters0020[5] > 0
        && tryPlacePrimaryTown(zone, alignment, -1, 1))
        return;
    if (slot->m_parameters0020[4] > 0)
        tryPlacePrimaryTown(zone, alignment, -1, 0);
}

// Retained by generation coordinator 0x549930; retail-only role/ABI.
#if 0 // @carcass
VA(0x00544AE0, 0x2B0)
void type_random_map_generator::placeAdditionalTowns(TRmgZone* zone) {} // @stub
#endif

// Direct caller 0x544a50 proves four stack arguments and byte success.
// Retail rejects alignment -1, selects a town prototype, tests placement,
// constructs a 0x28-byte object and sets the zone's primary town position.
// First reconstruction: 76.3310%. Candidate has three extra CFG blocks;
// inspect vector cleanup and the selected-position copy before changing
// the nearest-site rule or the verified town construction boundary.
VA(0x00545250, 0x324)
unsigned char type_random_map_generator::tryPlacePrimaryTown(
    TRmgZone* zone, int alignment, int player, unsigned char townOption)
{
    if (alignment == -1)
        return 0;
    std::vector<TRmgMapPosition> candidates;
    int zoneIndex = zone->m_slot->m_zoneIndex;
    TRmgMapPosition position = zone->m_levelPosition;
    TRmgObjectPropertiesRef* properties = m_objectPrototypes[TOWN][alignment];
    TObjectType* prototype = properties->m_prototype;
    int bestDistance = 32000;
    TRmgMapPosition nearby;
    nearby.m_z = position.m_z;
    TRmgZoneBounds bounds = zone->m_bounds;
    for (nearby.m_y = bounds.m_minimumY; nearby.m_y < bounds.m_maximumY; ++nearby.m_y) {
        for (nearby.m_x = bounds.m_minimumX; nearby.m_x < bounds.m_maximumX; ++nearby.m_x) {
            if (m_map.getMapItem(nearby)->m_zoneState.m_zone != zoneIndex)
                continue;
            int dx = nearby.m_x - position.m_x;
            int dy = nearby.m_y - position.m_y;
            int distance = dx * dx + dy * dy;
            if (distance <= bestDistance && m_map.canPlaceObject(properties, nearby, zone)) {
                if (distance < bestDistance) {
                    bestDistance = distance;
                    candidates.erase(candidates.begin(), candidates.end());
                }
                candidates.push_back(nearby);
            }
        }
    }
    if (!candidates.size())
        return 0;
    rmgTownObject* town = new rmgTownObject(properties, m_nextObjectId++, player, townOption);
    position = candidates[rand() % candidates.size()];
    addObject(town, position);
    position.m_x -= prototype->m_triggerCell.m_x;
    position.m_y -= prototype->m_triggerCell.m_y;
    zone->m_position = position;
    zone->m_active = 1;
    m_roadTargets.push_back(position);
    ++position.m_y;
    TRmgMapItem* item = m_map.getMapItem(position);
    if (!item->m_connection.m_present) {
        item->m_tileData.m_borderObject = 0;
        item->m_tileData.m_subterraneanGate = 1;
    }
    return 1;
}

// Site selector called at 0x545aee. Complete-only names are provisional.
// First reconstruction: 84.2575%. Preserve the ordered distance, border
// count and score filters, including their separate candidate resets.
VA(0x00545580, 0x401)
unsigned char type_random_map_generator::placeMineSite(type_object* object,
    TRmgZone* zone, unsigned char startingMine, int spacing)
{
    TRmgObjectPropertiesRef* properties = object->m_properties;
    TObjectType* prototype = properties->m_prototype;
    std::vector<TRmgMapPosition> candidates;
    int bestBorderCount = 0;
    int bestDistance = 40000;
    TRmgZoneBounds bounds = zone->m_bounds;
    int zoneIndex = zone->m_slot->m_zoneIndex;
    TRmgMapPosition townPosition;
    if (startingMine) {
        townPosition = zone->m_position;
        townPosition.m_x += prototype->m_triggerCell.m_x;
        townPosition.m_y += prototype->m_triggerCell.m_y;
    }
    bounds.m_minimumY += prototype->getHeight() - 1;
    bounds.m_minimumX += prototype->getWidth() - 1;
    TRmgMapPosition position = zone->m_levelPosition;
    properties->buildOutline();
    for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
            TRmgMapItem* item = m_map.getMapItem(position);
            if (item->m_zoneState.m_zone != zoneIndex || !m_map.canPlaceObject(properties, position, zone))
                continue;
            if (startingMine) {
                int dx = position.m_x - townPosition.m_x;
                int dy = position.m_y - townPosition.m_y;
                int distance = dx * dx + dy * dy;
                if (distance > bestDistance || distance < 16)
                    continue;
                if (distance < 144)
                    distance = 144;
                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestBorderCount = 0;
                    spacing = 0;
                    candidates.erase(candidates.begin(), candidates.end());
                }
            }
            int score = item->m_zoneState.m_score;
            if (score < spacing)
                continue;
            int borderCount = 0;
            for (unsigned int i = 0; i < properties->m_outline.size(); ++i) {
                int x = position.m_x + properties->m_outline[i].m_x;
                int y = position.m_y + properties->m_outline[i].m_y;
                if (x < 0 || x >= m_map.m_mapWidth || y < 0 || y >= m_map.m_mapHeight || y > position.m_y)
                    continue;
                TRmgMapItem* nearby = m_map.getMapItem(x, y, position.m_z);
                if (nearby->m_tileData.m_roadPassable && nearby->m_tile.m_landType != eTerrainRock
                    && nearby->hasBorderObject())
                    ++borderCount;
            }
            if (borderCount > 5)
                borderCount = 5;
            if (borderCount < bestBorderCount)
                continue;
            if (borderCount > bestBorderCount) {
                candidates.erase(candidates.begin(), candidates.end());
                bestBorderCount = borderCount;
            }
            if (score > spacing) {
                spacing = score;
                candidates.erase(candidates.begin(), candidates.end());
            }
            candidates.push_back(position);
        }
    }
    if (!candidates.size())
        return 0;
    position = candidates[rand() % candidates.size()];
    addObject(object, position);
    return 1;
}

// Retail +0x388 selects the MINE prototype vector. The caller supplies
// zone/resource/starting flag/spacing; names are role-derived.
// First reconstruction: 69.3906%. Keep prototype as the last scanned
// prototype: retail stores it at 0x5459f5/0x545a5d and reloads that same
// local at 0x545b7e/0x545ca9 without replacing it after random selection.
// This includes the retained trigger/width quirk in the resource strip.
VA(0x00545990, 0x466)
unsigned char type_random_map_generator::tryPlaceMine(TRmgZone* zone,
    int resource, unsigned char startingMine, int spacing)
{
    std::vector<TRmgObjectPropertiesRef*> candidates;
    TTerrainType terrain = zone->m_terrain;
    TRmgObjectPropertiesRef* properties;
    TObjectType* prototype;
    for (unsigned i = 0; i < m_objectPrototypes[MINE].size(); ++i) {
        properties = m_objectPrototypes[MINE][i];
        prototype = properties->m_prototype;
        if (prototype->m_subtype == resource && prototype->m_recommendedTerrainMask.test(terrain))
            candidates.insert(candidates.end(), properties);
    }
    if (!candidates.size()) {
        for (unsigned i = 0; i < m_objectPrototypes[MINE].size(); ++i) {
            properties = m_objectPrototypes[MINE][i];
            prototype = properties->m_prototype;
            if (prototype->m_subtype == resource)
                candidates.push_back(properties);
        }
    }
    if (!candidates.size())
        return 0;
    properties = candidates[rand() % candidates.size()];
    rmgOwnableObject* mine = new rmgOwnableObject(properties);
    if (!placeMineSite(mine, zone, startingMine, spacing)) {
        delete mine;
        return 0;
    }
    int value;
    switch (resource) {
    case WOOD: case ORE: value = 1500; break;
    case GOLD: value = 7000; break;
    default: value = 3500; break;
    }
    int guardValue = 0;
    if (zone->m_slot->m_monsterStrength) {
        int strength = zone->m_slot->m_monsterStrength + m_monsterStrength - 3;
        if (strength > 5) strength = 5;
        else if (strength < 0) strength = 0;
        guardValue = getRmgGuardValue(value, strength);
    }
    TRmgMapPosition entrance = mine->m_position;
    entrance.m_x -= prototype->m_triggerCell.m_x;
    entrance.m_y += 1 - prototype->m_triggerCell.m_y;
    TRmgMapItem* item = m_map.getMapItem(entrance);
    if (!item->m_connection.m_present) {
        item->m_tileData.m_borderObject = 0;
        item->m_tileData.m_subterraneanGate = 1;
    }
    if (guardValue > 0) {
        item = m_map.getMapItem(entrance);
        TRmgZone* guardZone = m_zones[item->m_zoneState.m_zone];
        if (static_cast<int>(item->m_objects.size()) <= 0) {
            type_object* guard = createGuard(guardValue, guardZone);
            if (guard)
                addObject(guard, entrance);
        }
    }
    int placed = 0;
    properties = selectObjectPrototype(terrain, RESOURCE, resource);
    if (!properties)
        return 1;
    TRmgMapPosition position = mine->m_position;
    TRmgZoneBounds bounds;
    bounds.m_minimumY = max(position.m_y + 1, 0);
    bounds.m_maximumY = min(position.m_y + 2, m_map.m_mapHeight);
    bounds.m_minimumX = max(position.m_x - prototype->getWidth(), 0);
    bounds.m_maximumX = min(position.m_x + 2, m_map.m_mapWidth);
    for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX && placed <= 2; ++position.m_x) {
            if (rand() % 2 == 0 && m_map.canPlaceObject(properties, position, zone)) {
                ++placed;
                addObject(new rmgResourceObject(properties), position);
            }
        }
    }
    return 1;
}

// Retail retains this ordinary fastcall helper and expands the same four
// table accesses in ground, border, gate and monolith connections.  ECX is
// the requested value, EDX the strength index, and values below 2000 vanish.
// The name is provisional; the shared helper boundary is retail-byte proof.
// Exact: 91/91 raw bytes after resolving the four table references. Both
// implemented connection callers inline this ordinary definition naturally.
VA(0x00545E00, 0x5B) // anchor-callee 0x545990 cluster; retail-only
int getRmgGuardValue(int value, int strength)
{
    int guardValue = 0;
    if (value > g_rmgGuardThresholdLow[strength]) {
        guardValue = (value - g_rmgGuardThresholdLow[strength])
            * g_rmgGuardScaleLow[strength] / 4;
    }
    if (value > g_rmgGuardThresholdHigh[strength]) {
        guardValue += (value - g_rmgGuardThresholdHigh[strength])
            * g_rmgGuardScaleHigh[strength] / 4;
    }
    return guardValue < 2000 ? 0 : guardValue;
}

// Retail-only density scheduler. Positive weights determine increments;
// requested mine counts determine the initial per-resource scores.
// Exact: 250 bytes and all 20 blocks. Clearing the exhausted flag before
// updating the product restores retail's register-store order (98.79% -> 100%).
VA(0x00545E60, 0xFA)
void type_random_map_generator::placeExtraMines(TRmgZone* zone)
{
    TRmgTownSlot* slot = zone->m_slot;
    unsigned char exhausted[7];
    int total = 0;
    int product = 1;
    for (int resource = 0; resource < 7; ++resource) {
        int weight = slot->m_parameters0068[resource];
        if (weight <= 0) {
            exhausted[resource] = 1;
        } else {
            total += weight;
            exhausted[resource] = 0;
            product *= weight;
        }
    }
    if (!total)
        return;
    int spacing = sqrt(static_cast<double>(82944 / total));
    int increment[7];
    int score[7];
    for (resource = 0; resource < 7; ++resource) {
        if (slot->m_parameters0068[resource] > 0) {
            increment[resource] = product / slot->m_parameters0068[resource];
            score[resource] = slot->m_parameters004c[resource] * increment[resource];
        }
    }
    while (1) {
        int selected = -1;
        int best = 0;
        for (resource = 0; resource < 7; ++resource) {
            if (!exhausted[resource] && (selected == -1 || score[resource] < best)) {
                best = score[resource];
                selected = resource;
            }
        }
        if (selected == -1)
            break;
        score[selected] += increment[selected];
        if (!tryPlaceMine(zone, selected, 0, spacing))
            exhausted[selected] = 1;
    }
}

// Generation pass 0x549ba9 places fixed counts before density placement.
// The first wood/ore mine is special only in active human/computer zones.
// Exact: 214 bytes and all 21 blocks. The inclusive GOLD bound preserves
// retail's cmp 6 / jle; the equivalent resource < 7 form scores 99.27%.
VA(0x00545F60, 0xD6)
void type_random_map_generator::placeMines()
{
    for (unsigned int index = 0; index < m_zones.size(); ++index) {
        TRmgZone* zone = m_zones[index];
        TRmgTownSlot* slot = zone->m_slot;
        for (int resource = 0; resource <= GOLD; ++resource) {
            unsigned char startingMine = 0;
            if ((resource == WOOD || resource == ORE)
                && (slot->m_kind == RMG_TEMPLATE_HUMAN || slot->m_kind == RMG_TEMPLATE_COMPUTER)
                && zone->m_active)
                startingMine = 1;
            for (int mine = 0; mine < slot->m_parameters004c[resource]; ++mine) {
                if (!tryPlaceMine(zone, resource, startingMine, 0))
                    break;
                startingMine = 0;
            }
        }
        placeExtraMines(zone);
    }
    if (m_progress)
        m_progress->advance(3900);
}

// Retail 0x546040 filters the object-type vector by subtype, admitting slot
// categories 4/5 on non-water terrain and other categories through the ten-
// terrain recommended mask. It returns null when no candidate survives,
// otherwise rand()%size selects a property reference. The original spelling
// is unavailable: selectObjectPrototype is a Complete-only role name.
// Keep this ordinary boundary: openConnectionPath calls it with terrain 0,
// object type 9 and its stored border direction at retail 0x540954.
// Residual (99.6581%): all 17 block sizes and ten branches agree; the
// terrain/range roles occupy ESI/EDI in the opposite order. Named mutable
// or const range references, nested subtype admission and the public value
// or count insert overloads did not recover that allocation in the family.
// Naming the selected pointer is neutral. Named random indices/counts change
// the result register lifetime (94.13% or lower), without settling the range
// and terrain allocation; preserve the direct result expression.
// A focused 60-case lifetime matrix (counter initialization, prototype-range
// binding order and enum/mask-index captures) also remains at 99.6581%.
// Sixty filter/receiver/insertion variants do not improve it either. Conditional
// admission, split insertion arms and a boolean admission result fail to retain
// the leading source's register/CFG combination; the existing source stays put.
VA(0x00546040, 0x141) // anchor-callee openConnectionPath; thiscall, ret 0x0c
TRmgObjectPropertiesRef* type_random_map_generator::selectObjectPrototype(
    int terrain, int objectType, int subtype)
{
    std::vector<TRmgObjectPropertiesRef*> candidates;
    for (unsigned int index = 0; index < m_objectPrototypes[objectType].size(); ++index) {
        TRmgObjectPropertiesRef* properties = m_objectPrototypes[objectType][index];
        TObjectType* prototype = properties->m_prototype;
        if (prototype->m_subtype != subtype)
            continue;
        if (prototype->m_slotCategory == TObjectType::SLOT_CATEGORY_4
            || prototype->m_slotCategory == TObjectType::SLOT_CATEGORY_5) {
            if (terrain == eTerrainWater)
                continue;
        } else if (!prototype->m_recommendedTerrainMask.test(terrain)) {
            continue;
        }
        candidates.push_back(properties);
    }
    if (!candidates.size())
        return 0;
    return candidates[rand() % candidates.size()];
}

// The reset loops at 0x546758/0x5468ca and 0x547647/0x547739 pass a
// TRmgMapItem in ECX and four scalar values. Retail writes land/frame in
// +0x24 and the two terrain flips in +0x28. setTerrain is a Complete-only
// role name; both the cell owner and this retained helper boundary are proven.
// Exact: the four canonical bitfield assignments reproduce all 73 bytes.
VA(0x00546940, 0x49) // anchor-callers + packed cell fields; thiscall ret 0x10
void TRmgMapItem::setTerrain(int terrain, int frame,
    unsigned char flipX, unsigned char flipY)
{
    m_tile.m_landType = terrain;
    m_tile.m_terrainFrame = frame;
    m_tileData.m_terrainFlipX = flipX;
    m_tileData.m_terrainFlipY = flipY;
}

// The road/river worklists instantiate all three of these out-of-line STL
// bodies.  Their distinct retail extents disambiguate the two int overloads.
VA_COMPGEN(0x00404200, 0x209, VECTOR_INSERT, Int)
VA_COMPGEN(0x00422F50, 0x1B1, VECTOR_INSERT, Int)
VA_COMPGEN(0x004347A0, 0x32E, VECTOR_INSERT, TRmgMapPosition)

// The RMG position insertion at 0x54c3f0 and spellbook's 12-byte entry
// insertion both call retail 0x54dd60. Their plain three-dword copies are
// ICF-identical; this TU naturally emits the TRmgMapPosition specialization.
VA_COMPGEN(0x0054DD60, 0x15, STD_CONSTRUCT, TRmgMapPosition)

// Retail 0x54c730 is the single-value insertion overload (ret 8), with
// count insertion expanded for one 0x4c-byte placement rule. It retains
// the three loops below and the implicit deep-copy construction helper.
// All five bodies match exactly when the checked-subscript reader uses
// direct single-value insertion for at least two scalar vectors. The all-push_back reader
// expands the wrapper and loops but raises the reader's own peak; these
// canonical library definitions and their banked MAXs remain unchanged.
// Retail's size call is ICF-shared with vector<TObjectType> (same stride);
// the fill and copy_backward calls have matching 0x4c-value ownership.
VA_COMPGEN(0x0054C730, 0x1DD, VECTOR_INSERT_SINGLE, TRmgObjectPlacementRule)
VA_COMPGEN(0x0054C940, 0x23, VECTOR_DESTROY, TRmgObjectPlacementRule)
VA_COMPGEN(0x0054D8B0, 0x38, VECTOR_UCOPY, TRmgObjectPlacementRule)
VA_COMPGEN(0x0054D8F0, 0x29, VECTOR_UFILL, TRmgObjectPlacementRule)
VA_COMPGEN(0x0054DD80, 0x104, STD_CONSTRUCT, TRmgObjectPlacementRule)

// ReadObjectPlacementRules retains the allocator-taking int-vector ctor;
// its two local vector grids also take the default-constructor closure's
// address. Resolved retail bodies are 27/27 and 24/24 bytes respectively.
VA_COMPGEN(0x005157D0, 0x1B, CLASS_CTOR, vector)
VA_COMPGEN(0x00536BA0, 0x18, DEFAULT_CTOR_CLOSURE, vector)

// TRmgObjectPlacementRule owns the two int vectors at +0x2c and +0x3c.
// Their reverse destruction order accounts for all 61 retail bytes.
VA_COMPGEN(0x00536B60, 0x3D, IMPLICIT_DTOR, TRmgObjectPlacementRule)

// The recovered generator-base constructor's exception cleanup naturally
// retains the value-vector destructor. Its 0x4c-stride loop and calls to
// TRmgObjectPlacementRule::~TRmgObjectPlacementRule distinguish it from
// the separate pointer-vector destructor emitted by the rule loader.
VA_COMPGEN(0x0054C170, 0x38, VECTOR_DTOR, TRmgObjectPlacementRule)

// FilterZonePositions retains this size calculation four times. Retail
// divides the template connection pointer span by its proven 0x1c stride.
VA_COMPGEN(0x0054C1B0, 0x23, VECTOR_SIZE, TRmgZoneConnection)

// DrawIrregularZoneBoundary retains this single-element erase. Its
// eight-byte copy loop and ret 4 agree in all 61 raw retail bytes.
VA_COMPGEN(0x0054CD70, 0x3D, VECTOR_ERASE, TPoint)

// The neighboring 12-byte position worklist retains the same single-element
// erase specialization with TRmgMapPosition's three-dword copy loop.
VA_COMPGEN(0x0054C610, 0x53, VECTOR_ERASE, TRmgMapPosition)

// The reader's two resize shrink arms retain this int-vector erase.
// All 51 raw bytes agree; no calls or data relocations remain unresolved.
VA_COMPGEN(0x0054CDB0, 0x33, VECTOR_ERASE, Int)

// InitializeObjectGenerators removes a byte range from its temporary work
// vector through this specialization.  The emitted COMDAT has the same five
// blocks and all 47 retail bytes; its byte-copy loop fixes the element type.
VA_COMPGEN(0x0054CFD0, 0x2F, VECTOR_ERASE, unsigned_char)

// FilterZonePositions erases 12-byte positions through this forward copy;
// the retained body copies three dwords and returns the end pointer.
VA_COMPGEN(0x0054D9E0, 0x39, STD_COPY, TRmgMapPosition)

// Retained by generation coordinator 0x549930; retail-only role/ABI.
#if 0 // @carcass
VA(0x00547360, 0x460)
void type_random_map_generator::placeZoneTreasures(TRmgZone* zone) {} // @stub
#endif

// Complete's road-target pass at 0x548290 invokes this flood once for each
// prospective source.  Retail proves the source-level worklist shape: two
// parallel vectors sorted by descending cost, special transitions through
// one-way/two-way monoliths and underground gates, then eight-neighbour road
// relaxation.  The Dreamcast build has no RMG compiland, so the original
// method spelling is unavailable and the role name remains provisional.
// Worklist push_back/pop_back restore the outer container boundaries and
// raise 67.8599% to 73.7139%; flattening them into insert/erase was the
// negative control. The shared by-value predecessor setter raises that to
// 75.6686% and restores the separate coordinate snapshot before the cost
// store. Its spelling remains provisional without Dreamcast source.
// The shared top-tested search also raises this to 81.8701%.
// Residual: the seed inserts and popped-element erases still expand deeper
// than retail. Both monolith position inserts now retain the two-argument
// boundary; the underground-gate site still expands it. The final neighbour
// inserts already select the retail count-insert calls. Keep the canonical
// helpers while recovering the remaining source/optimizer state.
VA(0x00547880, 0x7B1)  // roadTargets caller + monolith vectors; retail-only
void type_random_map_generator::buildRoadCostMap(TRmgMapPosition position)
{
    std::vector<TRmgMapPosition> openPositions;
    std::vector<int> openCosts;

    openPositions.push_back(position);
    openCosts.push_back(0);

    TRmgMapItem* mapItem = m_map.getMapItem(position);
    mapItem->m_movement.m_cost = 0;
    mapItem->m_previousTile.m_x = -1;
    mapItem->m_previousTile.m_y = -1;
    mapItem->m_previousTile.m_z = -1;

    while (openPositions.size()) {
        position = openPositions.back();
        openCosts.pop_back();
        openPositions.pop_back();

        mapItem = m_map.getMapItem(position);
        int positionCost = mapItem->m_movement.m_cost;
        unsigned char currentDecorated = mapItem->m_tile.m_roadType != 0;
        int direction = 8;
        unsigned char roadEntrance = mapItem->m_tileData.m_roadEntrance;

        if (roadEntrance) {
            type_object* object = mapItem->m_objects[0];
            TObjectType* properties = object->m_properties->m_prototype;
            int objectType = properties->m_objectType;
            if (!g_adventureObjectLandBlocked[objectType][1]
                && !g_adventureObjectLandBlocked[objectType][2])
                direction = 5;

            switch (objectType) {
            case LITH_ONEWAY_ENTRANCE:
            case LITH_ONEWAY_EXIT: {
                int subtype = properties->m_subtype;
                for (int i = 0; i < m_monolithsOneWay.size(); ++i) {
                    type_object* destination = m_monolithsOneWay[i];
                    if (destination->m_properties->m_prototype->m_subtype != subtype)
                        continue;

                    TRmgMapPosition nextPosition = destination->m_position;
                    TRmgMapItem* nextMapItem = m_map.getMapItem(nextPosition);
                    int nextCost = positionCost + 50;
                    if (nextMapItem->m_movement.m_cost <= nextCost)
                        continue;

                    nextMapItem->setMovementCost(nextCost, position);
                    insertRmgWorkItem(
                        openPositions, openCosts, nextPosition, nextCost);
                }
                break;
            }

            case LITH_TWOWAY: {
                int subtype = properties->m_subtype;
                for (int i = 0; i < m_monolithsTwoWay.size(); ++i) {
                    type_object* destination = m_monolithsTwoWay[i];
                    if (destination->m_properties->m_prototype->m_subtype != subtype)
                        continue;

                    TRmgMapPosition nextPosition = destination->m_position;
                    TRmgMapItem* nextMapItem = m_map.getMapItem(nextPosition);
                    int nextCost = positionCost + 50;
                    if (nextMapItem->m_movement.m_cost <= nextCost)
                        continue;

                    nextMapItem->setMovementCost(nextCost, position);
                    insertRmgWorkItem(
                        openPositions, openCosts, nextPosition, nextCost);
                }
                break;
            }

            case UNDERGROUND_GATE: {
                TRmgMapPosition nextPosition;
                nextPosition.m_x = position.m_x;
                nextPosition.m_y = position.m_y;
                nextPosition.m_z = 1 - position.m_z;
                TRmgMapItem* nextMapItem = m_map.getMapItem(nextPosition);
                int nextCost = positionCost + 1;
                if (nextMapItem->m_movement.m_cost > nextCost) {
                    nextMapItem->setMovementCost(nextCost, position);
                    insertRmgWorkItem(
                        openPositions, openCosts,
                        nextPosition, nextCost);
                }
                break;
            }
            }
        }

        while (direction--) {
            TPoint* directionOffset = &g_rmgDirections[direction];
            TRmgMapPosition nextPosition;
            nextPosition.m_x = position.m_x + directionOffset->m_x;
            nextPosition.m_y = position.m_y + directionOffset->m_y;
            nextPosition.m_z = position.m_z;

            if (nextPosition.m_x < 0 || nextPosition.m_x >= m_map.m_mapWidth
                || nextPosition.m_y < 0 || nextPosition.m_y >= m_map.m_mapHeight)
                continue;

            TRmgMapItem* nextMapItem = m_map.getMapItem(nextPosition);
            if (nextMapItem->m_tile.m_landType == eTerrainWater
                || !nextMapItem->m_tileData.m_roadPassable
                || nextMapItem->m_tile.m_landType == eTerrainRock)
                continue;

            unsigned char nextRoadEntrance =
                nextMapItem->m_tileData.m_roadEntrance;
            if (nextRoadEntrance) {
                int objectType =
                    nextMapItem->m_objects[0]->m_properties->m_prototype->m_objectType;
                const unsigned char* traits =
                    g_adventureObjectLandBlocked[objectType];
                if (traits[0] && !traits[2])
                    continue;
                if (!traits[1] && !traits[2]
                    && direction > 0 && direction < 4)
                    continue;
            }

            int nextCost = currentDecorated
                               && nextMapItem->m_tile.m_roadType
                           ? 2 : 20;
            if (direction & 1)
                nextCost *= 3;
            nextCost += positionCost;

            if (nextMapItem->m_movement.m_cost <= nextCost)
                continue;

            nextMapItem->setMovementCost(nextCost, position);
            insertRmgWorkItem(
                openPositions, openCosts, nextPosition, nextCost);
        }
    }
}


// Retail's road-target pass 0x548290 passes a position by value followed
// by the road kind (0x5483ed..0x548408). This predecessor walk paints only
// same-level cardinal runs and restarts its map view at a diagonal or level
// transition. Reaching zero movement cost ends the walk and returns whether
// any painter was constructed. The source spelling is a Complete-only role.
// The stack adapter at 0x548120 is the natural construction site for its
// concrete vtable and deleting wrapper; both cleanup exits restore 0x640a20.
// Sixteen loop/lifetime controls: a cached cell pointer in the decorated
// run reaches 70.4101%; repeated lookup is 51.8371%. A continue guard loses
// the shared cleanup layout (68.9944%); direct/goto loop tests remain lower.
VA(0x00548040, 0x244) // anchor-callee 0x548408 + adapter/painter vtables; retail-only
unsigned char type_random_map_generator::paintRoad(TRmgMapPosition position, int roadType)
{
    unsigned char painted = 0;
    for (;;) {
        int level = position.m_z;
        type_random_map levelMap(m_map.getMapItem(0, 0, level),
            m_map.m_mapWidth, m_map.m_mapHeight);
        TRmgMapPosition previous = position;
        TRmgMapItem* existing = m_map.getMapItem(position);
        while (existing->m_tile.m_roadType == roadType) {
            if (existing->m_movement.m_cost == 0)
                return painted;
            previous = position;
            position = existing->m_previousTile;
            existing = m_map.getMapItem(position);
        }
        if (position.m_z == level) {
            position = previous;
            TRmgRoadMapAdapter adapter(&levelMap);
            TRmgRoadPainter painter(
                &adapter, roadType, TRmgGridPoint(position.m_x, position.m_y));
            painted = 1;
            for (;;) {
                TRmgMapItem* item = m_map.getMapItem(position);
                if (item->m_movement.m_cost == 0)
                    return painted;
                position = item->m_previousTile;
                if (position.m_z != level
                    || (position.m_x != previous.m_x && position.m_y != previous.m_y))
                    break;
                painted = 1;
                painter.drawTo(TRmgGridPoint(position.m_x, position.m_y));
                previous = position;
            }
        }
    }
}

// CreateRiver and the retail route at 0x548500 share this constructor,
// GetMapItem(0, 0), and whole-map predecessor/cost reset sequence. Keeping
// that common pass as an ordinary generator helper recovers all six seed
// insert calls, both popped-element erase calls, and the range erase in
// CreateRiver (75.23%, versus 71.47% with the pass flattened there).
// Dreamcast has no RMG compiland; the role name/linkage remain provisional.
// Unsigned width/height values passed through the recovered grid constructor
// restore all 90 raw bytes at CreateRiver +0x42..+0x9c: the height temporary,
// volume calculation and by-value predecessor copy. Scalar products/getters
// and signed TPoint size queries lose those homes. Direct grid construction
// from the signed fields instead spills width; naming height before width
// also reverses the retail dimension loads. The loop retains its 0xbc frame.
// CreateRiver reaches 85.96%; early vector _Destroy calls still over-expand.
// Its final empty-vector cleanup now has separate returns where retail shares
// the final delete epilogue. Preserve the exact reset sequence through that
// remaining caller cleanup work.
void type_random_map_generator::resetMovementCosts()
{
    TRmgMapPosition resetPosition(-1, -1, -1);
    TRmgMapItem* mapItem = m_map.getMapItem(0, 0);
    unsigned width = m_map.m_mapWidth;
    unsigned height = m_map.m_mapHeight;
    TRmgGridPoint mapSize(width, height);
    int mapItemCount = mapSize.m_x * mapSize.m_y * m_map.m_numberLevels;
    while (mapItemCount--) {
        mapItem->resetMovement(resetPosition);
        ++mapItem;
    }
}

// Retail 0x549c98 connects every ordered target pair using one random road
// style. A successful draw changes traversal costs for subsequent targets.
// Partial 71.72%: resetMovementCosts retains its position constructor and
// two-coordinate map lookup at each call; retail expands both. Keep the
// canonical shared reset rather than copying its body into this caller.
VA(0x00548290, 0x26E)
void type_random_map_generator::createRoads()
{
    int roadType = rand() % 3 + 1;
    for (unsigned int first = 0; first < m_roadTargets.size() - 1; ++first) {
        TRmgMapPosition source = m_roadTargets[first];
        resetMovementCosts();
        buildRoadCostMap(source);
        for (unsigned int second = first + 1; second < m_roadTargets.size(); ++second) {
            if (m_map.getMapItem(m_roadTargets[second])->m_movement.m_cost <= 30000
                && paintRoad(m_roadTargets[second], roadType)
                && second < m_roadTargets.size() - 1) {
                resetMovementCosts();
                buildRoadCostMap(source);
                if (m_progress)
                    m_progress->advance(1000);
            }
        }
        if (m_progress)
            m_progress->advance(1000);
    }
}

// Retail 0x5498de routes from a water-wheel trigger to a marked object,
// then paints the predecessor chain. Role-derived name; no DC RMG counterpart.
// Unlike the coast-bound river, this search ignores impassable/direction flags
// and stops on the shared roadTarget bit set by markRiverObjectTargets.
// Partial 89.61%: direct invalid-predecessor field stores remove the extra
// constructor in the first 87.80% candidate. A separate default-then-filled
// invalid-position local scores 83.63%. Retained vector cleanup boundaries
// and frame/register homes remain unresolved; no inlining pins are used.
VA(0x00548500, 0x533)
void type_random_map_generator::createRiverToObject(TRmgMapPosition source)
{
    resetMovementCosts();
    std::vector<TRmgMapPosition> openPositions;
    std::vector<int> openCosts;
    openPositions.push_back(source);
    openCosts.push_back(0);
    TRmgMapItem* mapItem = m_map.getMapItem(source.m_x, source.m_y, source.m_z);
    mapItem->m_movement.m_cost = 0;
    mapItem->m_previousTile.m_x = -1;
    mapItem->m_previousTile.m_y = -1;
    mapItem->m_previousTile.m_z = -1;
    unsigned char sourceIsSnow;
    int riverType;
    if (mapItem->m_tile.m_landType == eTerrainSnow) {
        sourceIsSnow = 1;
        riverType = 2;
    } else {
        sourceIsSnow = 0;
        riverType = 1;
    }
    TRmgMapPosition position;
    TRmgMapPosition nextPosition;
    while (!openPositions.empty()) {
        position = openPositions.back();
        openCosts.pop_back();
        openPositions.pop_back();
        mapItem = m_map.getMapItem(position.m_x, position.m_y, position.m_z);
        int positionCost = mapItem->m_movement.m_cost;
        for (int direction = 0; direction < 8; direction += 2) {
            nextPosition = position + g_rmgDirections[direction];
            if (nextPosition.m_x < 0 || nextPosition.m_x >= m_map.m_mapWidth
                || nextPosition.m_y < 0 || nextPosition.m_y >= m_map.m_mapHeight)
                continue;
            mapItem = m_map.getMapItem(nextPosition.m_x, nextPosition.m_y, nextPosition.m_z);
            if (mapItem->m_tile.m_landType == eTerrainWater
                || mapItem->m_tile.m_landType == eTerrainRock
                || (mapItem->m_tile.m_landType == eTerrainSnow) != sourceIsSnow)
                continue;
            int nextCost = positionCost + (rand() & 31) + 1;
            if (mapItem->m_tile.m_roadType)
                nextCost += 30;
            if (nextCost >= mapItem->m_movement.m_cost)
                continue;
            mapItem->setMovementCost(nextCost, position);
            insertRmgWorkItem(openPositions, openCosts, nextPosition, nextCost);
            if (mapItem->hasRiver()) {
                openPositions.clear();
                break;
            }
        }
    }
    if (!mapItem->hasRiver())
        return;
    type_random_map levelMap(m_map.getMapItem(0, 0, nextPosition.m_z),
        m_map.m_mapWidth, m_map.m_mapHeight);
    TRmgMapAdapter mapAdapter(&levelMap);
    TRmgRiverPainter riverPainter(
        &mapAdapter, riverType, TRmgGridPoint(nextPosition.m_x, nextPosition.m_y));
    while (mapItem->m_movement.m_cost > 0) {
        position = mapItem->m_previousTile;
        mapItem = m_map.getMapItem(position.m_x, position.m_y, position.m_z);
        riverPainter.drawTo(TRmgGridPoint(position.m_x, position.m_y));
    }
}

// Retail-only coastal target test: three water cells across the shore,
// three dry entrance-free cells beside them, then four cells inland.
// Preserve retail's asymmetric x > width versus y >= height bounds check.
// The last inland cell gains a river target and an approach-direction bit.
// Partial 76.72%: all 25 CFG blocks align and both retained position
// constructors agree; loop counter/register homes remain different.
// A copy-based operator+ removes the retail constructor calls (65.57%).
VA(0x00548A40, 0x222)
void type_random_map_generator::markRiverCoastTarget(TRmgMapPosition position, int direction)
{
    TRmgMapPosition point = position + g_rmgDirections[(direction + 2) & 7];
    TPoint step = g_rmgDirections[(direction - 2) & 7];
    for (int count = 0; count < 3; ++count) {
        if (point.m_x < 0 || point.m_x > m_map.m_mapWidth
            || point.m_y < 0 || point.m_y >= m_map.m_mapHeight)
            return;
        if (m_map.getMapItem(point.m_x, point.m_y, point.m_z)->m_tile.m_landType != eTerrainWater)
            return;
        point += step;
    }
    point = position + g_rmgDirections[(direction + 1) & 7];
    for (count = 0; count < 3; ++count) {
        if (point.m_x < 0 || point.m_x > m_map.m_mapWidth
            || point.m_y < 0 || point.m_y >= m_map.m_mapHeight)
            return;
        TRmgMapItem* item = m_map.getMapItem(point.m_x, point.m_y, point.m_z);
        if (item->m_tile.m_landType == eTerrainWater || item->isRoadEntrance())
            return;
        point += step;
    }
    point = position;
    point += g_rmgDirections[direction];
    TRmgMapItem* item;
    for (count = 0; count < 4; ++count) {
        if (point.m_x < 0 || point.m_x > m_map.m_mapWidth
            || point.m_y < 0 || point.m_y >= m_map.m_mapHeight)
            return;
        item = m_map.getMapItem(point.m_x, point.m_y, point.m_z);
        if (item->m_tile.m_landType == eTerrainWater || item->isRoadEntrance())
            return;
        point += g_rmgDirections[direction];
    }
    item->m_tileData.m_blockedDirections |= 1 << (((direction - 4) >> 1) & 3);
    item->m_tileData.m_riverTarget = 1;
}

// Retail scans water cells in z/y/x order, visits four cardinal coast
// directions, then sets the canonical riverTarget flag on every map edge.
// Partial 80.94%; loop/register lowering remains to be recovered.
VA(0x00548C70, 0x17D)
void type_random_map_generator::markRiverTargets()
{
    TRmgMapItem* item = m_map.m_mapItems;
    for (int z = 0; z < m_map.m_numberLevels; ++z) {
        for (int y = 0; y < m_map.m_mapHeight; ++y) {
            for (int x = 0; x < m_map.m_mapWidth; ++x, ++item) {
                if (item->m_tile.m_landType == eTerrainWater) {
                    for (int direction = 0; direction < 8; direction += 2)
                        markRiverCoastTarget(TRmgMapPosition(x, y, z), direction);
                }
            }
        }
    }
    for (z = 0; z < m_map.m_numberLevels; ++z) {
        for (int y = 0; y < m_map.m_mapHeight; ++y) {
            m_map.getMapItem(0, y, z)->m_tileData.m_riverTarget = 1;
            m_map.getMapItem(m_map.m_mapWidth - 1, y, z)->m_tileData.m_riverTarget = 1;
        }
        for (int x = 0; x < m_map.m_mapWidth; ++x) {
            m_map.getMapItem(x, 0, z)->m_tileData.m_riverTarget = 1;
            m_map.getMapItem(x, m_map.m_mapHeight - 1, z)->m_tileData.m_riverTarget = 1;
        }
    }
    if (m_progress)
        m_progress->advance(1000);
}

// The Complete RMG has no Dreamcast counterpart.  Retail nevertheless fixes
// the whole source-level algorithm: two parallel vectors form a descending
// cost worklist, four cardinal neighbours relax a randomized Dijkstra search,
// and the predecessor chain is then painted back from the first river target.
// The water-wheel caller at 0x549870 and object type 143 selected below prove
// the river role; the method spelling remains provisional.
// Retail reuses ESI for every in-bounds neighbour, then tests that same tile
// at +0x4b7 after the worklist empties.  A separate nextMapItem leaves the
// final test on the preceding tile and can skip painting a found river.
// The saved position is reused at +0x53e before the mouth temporarily replaces
// nextPosition; the delta-direction scan explicitly stops at four directions.
// The neighbour scan compares a strength-reduced direction-table address
// with signed JL at +0x491: its source induction variable is the integer
// direction (0, 2, 4, 6), not a pointer. The pointer loop lowers this to JB
// and scores 40.29%; restoring the signed index reaches 65.44%. Keeping the
// canonical coordinate addition also preserves the returned temporary;
// spelling its component sums directly scores 64.38%.
// Its point operand is by value: this restores the first-iteration jump over
// the coordinate reloads and the full relaxation register flow (84.00%).
// With const-ref, those loop edges differ and the checkpoint is 81.24%.
// Indexed landPage access keeps _Xran out of line and removes the extra
// 0x24-byte exception frame (68.45%); direct test() leaves it expanded.
// The three seed predecessors copy one explicit invalid position, retaining
// its z home across the first two inserts as retail does (71.86%). Keeping
// the invalid x/y/z writes directly on each tile instead leaves 68.45%.
// Keep the reset helper's initial position separate from the worklist position:
// its out-of-line constructor receives its address. Ending that lifetime
// lets VC6 remove the relaxation setter's redundant predecessor snapshot,
// preserve the queue insertion's distinct next-position copy, and recover
// retail's 0xbc-byte frame (71.47%, with 71.86% banked). A separate but
// unscoped reset position leaves a 0xc8-byte frame and scores 71.31%.
// The shared reset helper recovers the seed/worklist vector boundaries.
// Test blockedDirections as a bitfield at both uses: retail tests AH before
// shifting and keeps the four-bit mask in each direction test. A cached
// unsigned value instead normalizes the field up front (75.23% vs 74.77%).
// The one-bit river/impassable predicates return byte values: bool queries
// restore all three SHR/TEST-byte sequences (76.51%); unsigned-char queries
// are identical, while direct field tests select dword masks.
// The terrain filter compares the field directly: its equality-only uses
// lower to retail's AND 0x3f (81.24%). A named signed terrain local, even
// const, instead retains SHL/SAR sign extension (79.82%). The delta-path
// terrain local remains signed because it is also used as a bitset index.
// Residual: early-return vector destruction still expands beyond retail.
// River-target setters/markers are byte-flat. Delta copy constructors,
// reference components and a TPoint base are also flat. Giving TPoint an
// empty destructor adds cleanup states absent from retail; using a trivial
// TPoint for the delta table removes retail's atexit call. Neither resolves
// the ordered static initialization, so keep the existing type boundary.
// Map-view body assignments/accessors, explicit final return and a shared
// zero-cost seed initializer do not restore early cleanup. A separate
// painting scope changes the frame to 0xac; explicit position copy members
// change it to 0xb0/0xc8 and lose retail CFG blocks. These are not substitutes
// for the missing natural boundary. At 81.24%, C2 measures caller cb=1530:
// the early empty _Destroy helpers cost 49 but receive 68/65. Later map
// cleanups already retain/expand correctly at budgets 91/251 for cost 97.
// With the value operand, an empty position destructor changes the frame to
// 0xd4 and adds four CFG blocks. Default invalid coordinates retain 0xbc but
// disturb later cleanup. Coordinate/cost getters retain only one early
// _Destroy and over-expand final map-item cleanup; their 84.22% is not proof
// of that interface. Loop-local indices, delta constructor body/visibility,
// a const delta table, a predecessor setter and volume regrouping are flat.
// A three-dimensional size query leaves an extra GetSize call; output
// references spill the map pointer instead of retail's height. Moving the
// map-view ownership write to the end does not recover the constructor.
// The real virtual GetSize slot (0x532240) returns the two-dimensional size;
// using it here retains a virtual call absent from retail's reset sequence.
// With the grid reset recovered, reference dimensions on the map-view ctor
// do not settle the painter entry: signed refs score 81.50% and lose the exact
// water-border caller; unsigned refs preserve that caller but score 85.73%
// without restoring the missing load order. The value signature stays.
// A copy-and-increment translation body also loses the matching loop flow
// (76.72%); keep the returned coordinate construction.
// Direct erase() calls expand even further (61.45% before the seed-copy
// correction). An explicit predecessor copy and const by-value parameter
// are byte-flat. A const-ref setter changes the shared road helper's proved by-value boundary and is
// rejected; a combined reset/cost setter and by-value position assignment
// also fail the reset's constant-cost and copy sequence.
// Additional controls with the grid reset: reusing the seed position or
// shortening its scope leaves the cleanup mismatch (84.93/85.34%). Empty
// sized-vector constructors lose reset/seed regions and may grow the frame
// to 0xc0. A grid projection constructor and canonical delta addition do not
// recover the painting lifetimes. A const prototype query scores 86.40% but
// removes two CFG blocks; named bitset references/results also fail to restore
// the retained range-check pointer. None is evidence for replacing the
// current interface or hiding the early vector cleanup mismatch.
// Paired seed-append helpers retain neither early _Destroy call (84.07%
// with a value cost, 83.87% with a reference cost; unused-helper control flat).
// A map/level view overload also misses the painting construction order
// (80.32%); naming the plane buffer first loses the exact reset homes.
// Tail-local positions and named grid temporaries remove the z snapshot in
// some forms but still change the painting stores. Scalar tail lookup reaches
// 86.24% and restores the final shared cleanup, while incorrectly merging
// the early return into it and growing the painting loop to 24 instructions
// versus retail's 21. This is not proof of replacing the position overload.
// The terrain painter's default-then-assigned grid lifetime does not transfer
// to the river's start/drawing arguments: separate controls grow the frame to
// 0xc0, and applying both reaches 86.41% with a non-retail 0xc4 frame.
// Boolean snow/ownership fields and moving the buffer store into the view's
// initializer are byte-flat, as is consuming the predecessor assignment result.
// A grid point built directly from the predecessor instead repeats coordinate
// loads before lookup (84.75%); it does not recover the retail painting loop.
VA(0x00548DF0, 0x99F)  // water-wheel caller + river-delta object; retail-only
void type_random_map_generator::createRiver(TRmgMapPosition source)
{
    resetMovementCosts();

    TRmgMapItem* mapItem;
    TRmgMapPosition emptyPosition;
    emptyPosition.m_x = -1;
    emptyPosition.m_y = -1;
    emptyPosition.m_z = -1;

    std::vector<TRmgMapPosition> openPositions;
    std::vector<int> openCosts;

    openPositions.push_back(source);
    openCosts.push_back(0);
    mapItem = m_map.getMapItem(source);
    mapItem->m_movement.m_cost = 0;
    mapItem->m_previousTile = emptyPosition;

    unsigned char sourceIsSnow;
    int riverType;
    if (mapItem->m_tile.m_landType == eTerrainSnow) {
        sourceIsSnow = 1;
        riverType = 2;
    } else {
        sourceIsSnow = 0;
        riverType = 1;
    }

    --source.m_y;
    openPositions.push_back(source);
    openCosts.push_back(0);
    mapItem = m_map.getMapItem(source);
    mapItem->m_movement.m_cost = 0;
    mapItem->m_previousTile = emptyPosition;

    ++source.m_x;
    openPositions.push_back(source);
    openCosts.push_back(0);
    mapItem = m_map.getMapItem(source);
    mapItem->m_movement.m_cost = 0;
    mapItem->m_previousTile = emptyPosition;

    TRmgMapPosition position;
    TRmgMapPosition nextPosition;
    int direction;

    while (!openPositions.empty()) {
        position = openPositions.back();
        openCosts.pop_back();
        openPositions.pop_back();

        mapItem = m_map.getMapItem(position);
        int positionCost = mapItem->m_movement.m_cost;
        for (direction = 0; direction < 8; direction += 2) {
            nextPosition = position + g_rmgDirections[direction];

            if (nextPosition.m_x < 0 || nextPosition.m_x >= m_map.m_mapWidth
                || nextPosition.m_y < 0 || nextPosition.m_y >= m_map.m_mapHeight)
                continue;

            mapItem = m_map.getMapItem(nextPosition);
            if (mapItem->m_tile.m_landType == eTerrainWater
                || mapItem->m_tile.m_landType == eTerrainRock
                || mapItem->isImpassable()
                || (mapItem->m_tile.m_landType == eTerrainSnow) != sourceIsSnow)
                continue;

            int nextCost = positionCost + (rand() & 31) + 1;
            if (mapItem->m_tile.m_roadType)
                nextCost += 30;

            if (nextCost >= mapItem->m_movement.m_cost)
                continue;

            int oppositeDirection = ((direction - 4) >> 1) & 3;
            if (mapItem->m_tileData.m_blockedDirections
                & (1 << oppositeDirection))
                continue;

            mapItem->setMovementCost(nextCost, position);
            insertRmgWorkItem(
                openPositions, openCosts, nextPosition, nextCost);

            if (mapItem->isRiverTarget()) {
                openPositions.clear();
                break;
            }
        }
    }

    if (!mapItem->isRiverTarget())
        return;

    mapItem->m_tileData.m_riverTarget = 1;
    position = nextPosition;

    type_random_map levelMap(m_map.getMapItem(0, 0, nextPosition.m_z),
        m_map.m_mapWidth, m_map.m_mapHeight);
    TRmgMapAdapter mapAdapter(&levelMap);
    TRmgRiverPainter riverPainter(
        &mapAdapter, riverType, TRmgGridPoint(nextPosition.m_x, nextPosition.m_y));

    if (mapItem->m_tileData.m_blockedDirections) {
        for (direction = 0; direction < 4; ++direction) {
            if (mapItem->m_tileData.m_blockedDirections & (1 << direction))
                break;
        }

        static TRmgRiverDeltaOffset deltaOffsets[4] = {
            TRmgRiverDeltaOffset(4, 1),
            TRmgRiverDeltaOffset(1, 4),
            TRmgRiverDeltaOffset(-2, 1),
            TRmgRiverDeltaOffset(1, -2)
        };

        int deltaIndex = sourceIsSnow
            ? g_snowRiverDeltaIndex[direction]
            : g_landRiverDeltaIndex[direction];
        int landType = mapItem->m_tile.m_landType;
        int prototypeIndex = 0;
        for (; prototypeIndex < m_objectPrototypes[TERRAIN_RIVER_DELTA].size();
             ++prototypeIndex) {
            TRmgObjectPropertiesRef* properties =
                m_objectPrototypes[TERRAIN_RIVER_DELTA][prototypeIndex];
            if (properties->m_prototype->m_recommendedTerrainMask[landType]
                && deltaIndex-- == 0)
                break;
        }

        if (prototypeIndex == m_objectPrototypes[TERRAIN_RIVER_DELTA].size())
            return;

        type_object* riverDelta = new type_object(
            m_objectPrototypes[TERRAIN_RIVER_DELTA][prototypeIndex]);
        addObject(
            riverDelta,
            TRmgMapPosition(
                nextPosition.m_x + deltaOffsets[direction].m_x,
                nextPosition.m_y + deltaOffsets[direction].m_y,
                nextPosition.m_z));

        nextPosition = nextPosition + g_rmgDirections[direction * 2];
        riverPainter.drawTo(TRmgGridPoint(nextPosition.m_x, nextPosition.m_y));
        mapItem = m_map.getMapItem(nextPosition);
        mapItem->m_tileData.m_riverTarget = 1;

        riverPainter.drawTo(TRmgGridPoint(position.m_x, position.m_y));
        mapItem = m_map.getMapItem(position);
    }

    while (mapItem->m_movement.m_cost > 0) {
        position = mapItem->m_previousTile;
        mapItem = m_map.getMapItem(position);
        mapItem->m_tileData.m_riverTarget = 1;
        riverPainter.drawTo(TRmgGridPoint(position.m_x, position.m_y));
    }
}

// Retail 0x5497d8 selects mountains, lakes and gem mines.
// Triggered objects use their trigger offset; scenery uses unsigned half-size.
// It marks bit 29 (canonical roadTarget), also consumed by river routing.
// The coordinator role name is provisional; the shared flag keeps its owner name.
// Partial 95.84%: selecting offsets before the common subtraction restores
// retail statement order; subtracting in both arms scores 90.00%. Residual
// register allocation and tile-address lowering remain.
VA(0x005497A0, 0xCE)
void type_random_map_generator::markRiverObjectTargets()
{
    for (unsigned int index = 0; index < m_positions.size(); ++index) {
        type_object* object = m_positions[index];
        TObjectType* prototype = object->m_properties->m_prototype;
        if (prototype->m_objectType == TERRAIN_MOUNTAIN
            || prototype->m_objectType == TERRAIN_LAKE
            || (prototype->m_objectType == MINE && prototype->m_subtype == GEMS)) {
            TRmgMapPosition position = object->m_position;
            int offsetX;
            int offsetY;
            if (prototype->m_hasTrigger) {
                offsetX = prototype->m_triggerCell.m_x;
                offsetY = prototype->m_triggerCell.m_y;
            } else {
                offsetX = static_cast<unsigned int>(prototype->m_imageInfo.m_objectSize.m_x) / 2;
                offsetY = static_cast<unsigned int>(prototype->m_imageInfo.m_objectSize.m_y) / 2;
            }
            position.m_x -= offsetX;
            position.m_y -= offsetY;
            if (position.m_x >= 0 && position.m_x < m_map.m_mapWidth
                && position.m_y >= 0 && position.m_y < m_map.m_mapHeight)
                m_map.getMapItem(position.m_x, position.m_y, position.m_z)->m_tileData.m_hasRiver = 1;
        }
    }
    if (m_progress)
        m_progress->advance(1000);
}

// The water-wheel coordinator calls the two preparation passes, then builds
// a cost map at each wheel's trigger and routes from two cells to its left.
// Retail 0x5498b1 proves the object kind and 0x5498eb the source displacement.
// Partial 90.51%; the position-copy/call argument homes remain different.
VA(0x00549870, 0xB1)
void type_random_map_generator::createRivers()
{
    markRiverObjectTargets();
    markRiverTargets();
    for (unsigned int index = 0; index < m_positions.size(); ++index) {
        type_object* object = m_positions[index];
        TObjectType* prototype = object->m_properties->m_prototype;
        if (prototype->m_objectType == WATER_WHEEL) {
            TRmgMapPosition position = object->m_position;
            position.m_x -= prototype->m_triggerCell.m_x;
            position.m_y -= prototype->m_triggerCell.m_y;
            createRiverToObject(position);
            position.m_x -= 2;
            createRiver(position);
            if (m_progress)
                m_progress->advance(1000);
        }
    }
}

// Retail 0x54bf60 calls this Complete-only coordinator. Preserve the player
// ordering, separate town passes, two connection-cost passes, and final
// coastal/decorative/road/river order. No Dreamcast counterpart exists.
// First reconstruction: 96.8971%. The nine-entry mapping preserves retail
// offsets; the two existing mapping readers retain their prior scores.
VA(0x00549930, 0x37B)
unsigned char type_random_map_generator::generate()
{
    if (!m_templates.size())
        return 0;
    unsigned int selected = rand() % m_templates.size();
    m_templateName = m_templates[selected]->m_name;
    char humanSlots[8] = {0};
    char allSlots[8] = {0};
    TRmgTemplate* mapTemplate = m_templates[selected];
    for (unsigned int zone = 0; zone < mapTemplate->m_zones.size(); ++zone) {
        TRmgTownSlot* slot = mapTemplate->m_zones[zone];
        if (slot->m_kind == RMG_TEMPLATE_HUMAN) {
            humanSlots[slot->m_playerIndex] = 1;
            allSlots[slot->m_playerIndex] = 1;
        } else if (slot->m_kind == RMG_TEMPLATE_COMPUTER) {
            allSlots[slot->m_playerIndex] = 1;
        }
    }
    memset(m_playerIndexMap, -1, sizeof(m_playerIndexMap));
    int players[8];
    int count = 0;
    for (int player = 0; player < 8; ++player)
        if (m_fixedHumanPlayers[player])
            players[count++] = player;
    for (player = 0; player < 8; ++player)
        if (!m_fixedHumanPlayers[player])
            players[count++] = player;
    int slot = 0;
    for (player = 0; player < m_humanPlayerCount; ++player) {
        while (slot < 8 && !humanSlots[slot])
            ++slot;
        allSlots[slot] = 0;
        m_playerIndexMap[++slot] = players[player];
    }
    slot = 0;
    for (; player < m_humanPlayerCount + m_computerPlayerCount; ++player) {
        while (slot < 8 && !allSlots[slot])
            ++slot;
        m_playerIndexMap[++slot] = players[player];
    }
    initializeZones(m_templates[selected]);
    for (int level = 0; level < m_map.m_numberLevels; ++level)
        buildZoneBoundaries(m_templates[selected], level);
    paintZoneTerrain();
    for (zone = 0; zone < m_zones.size(); ++zone)
        placePrimaryTown(m_zones[zone]);
    for (zone = 0; zone < m_zones.size(); ++zone)
        placeAdditionalTowns(m_zones[zone]);
    prepareZoneConnections();
    for (zone = 0; zone < m_zones.size(); ++zone)
        if (m_zones[zone]->m_slot->m_kind == RMG_TEMPLATE_JUNCTION
            && m_zones[zone]->m_terrain != eTerrainWater)
            prepareJunctionZone(m_zones[zone]);
    placeMines();
    memset(m_activeZoneCountsByAlignment, 0, sizeof(m_activeZoneCountsByAlignment));
    m_activeZoneCount = 0;
    for (zone = 0; zone < m_zones.size(); ++zone) {
        if (m_zones[zone]->m_active) {
            ++m_activeZoneCountsByAlignment[m_zones[zone]->m_alignment];
            ++m_activeZoneCount;
        }
    }
    buildZoneConnectionPaths();
    for (zone = 0; zone < m_zones.size(); ++zone) {
        placeZoneTreasures(m_zones[zone]);
        if (m_progress)
            m_progress->advance(7000 / m_zones.size());
    }
    if (m_map.m_numberLevels > 1)
        decorateUnderground();
    m_map.markCoastalTiles();
    decorateMap();
    createRoads();
    createRivers();
    return 1;
}

// Complete's random-map pipeline calls this routine immediately before the
// generated terrain/object stream is emitted.  The format switch, description
// fragments, player records, team assignment, and packed availability masks
// are all read directly from retail's stream-write CFG.  The Dreamcast port
// has no RMG compiland, so the method spelling remains provisional while its
// class offsets and serialization order are retail-byte facts.
//
// Current 94.10% after removing the TU-local string constructor
// specializations and their three inline pins. Their removal is byte-neutral
// in ReadObjectPlacementRules; the canonical library definitions stay in use.
// Historical peak (95.71%): all 164 CFG blocks and all 87 branches align;
// 152 blocks also have exact emitted sizes.  The remaining twelve are local
// lowering differences.  Retail's frame is 0x318 versus 0x310 here and its
// legacy-artifact copy preserves one extra two-word end iterator.  Directly
// naming all three iterators and default-constructing then assigning the first
// each regress to 95.66%; making the iterator non-trivial regresses to 93.97%
// and destroys the matching tail CFG.  Those source-false forms remain out.
VA(0x00549CB0, 0xE90)  // GenerateRandomMap caller chain; retail-only RMG
void type_random_map_generator::writeMapHeader(TAbstractFile* outfile)
{
    {
        int intBuffer = getSerializedMapVersion();
        outfile->write(&intBuffer, sizeof(intBuffer));
    }

    {
        char byteBuffer = 1;
        outfile->write(&byteBuffer, sizeof(byteBuffer));
    }

    {
        int intBuffer = m_map.m_mapWidth;
        outfile->write(&intBuffer, sizeof(intBuffer));
    }

    {
        char byteBuffer = m_map.m_numberLevels > 1;
        outfile->write(&byteBuffer, sizeof(byteBuffer));
    }

    std::string mapName(
        DATA_COMPGEN(0x00682900, rmgMapName, "Random Map"));
    {
        int intBuffer = mapName.length();
        outfile->write(&intBuffer, sizeof(intBuffer));
    }
    outfile->write(mapName.c_str(), mapName.length());

    // Retail places description at [ebp-0x324] and mainTowns at
    // [ebp-0x130]; their 0x1f4-byte separation proves the 500-byte extent.
    char description[500];
    sprintf(
        description,
        DATA_COMPGEN(
            0x0068286C,
            rmgDescriptionFormat,
            "Map created by the Random Map Generator.  Template was %s, "
            "Random seed was %i, size %i, levels %i, humans %i, "
            "computers %i, water %s, monsters %i"),
        m_templateName.c_str(),
        m_randomSeed,
        m_map.m_mapWidth,
        m_map.m_numberLevels,
        m_humanPlayerCount,
        m_computerPlayerCount,
        g_rmgWaterNames[m_waterContent],
        m_monsterStrength);

    switch (m_mapVersion) {
    case RMG_MAP_RESTORATION_OF_ERATHIA:
        strcat(
            description,
            DATA_COMPGEN(0x0068282C, rmgOriginalMap, ", original map"));
        break;
    case RMG_MAP_ARMAGEDDONS_BLADE:
        strcat(
            description,
            DATA_COMPGEN(
                0x0068283C, rmgFirstExpansionMap, ", first expansion map"));
        break;
    case RMG_MAP_SHADOW_OF_DEATH:
        strcat(
            description,
            DATA_COMPGEN(
                0x00682854,
                rmgSecondExpansionMap,
                ", second expansion map"));
        break;
    }

    for (int descriptionPlayer = 0; descriptionPlayer < 8;
         ++descriptionPlayer) {
        if (m_fixedHumanPlayers[descriptionPlayer]) {
            strcat(
                description,
                DATA_COMPGEN(0x0066032C, rmgListSeparator, ", "));
            strcat(description, g_rmgPlayerNames[descriptionPlayer]);
            strcat(
                description,
                DATA_COMPGEN(0x00682820, rmgIsHuman, " is human"));
        }

        if (m_townChoices[descriptionPlayer] != -1) {
            strcat(
                description,
                DATA_COMPGEN(0x0066032C, rmgListSeparator, ", "));
            strcat(description, g_rmgPlayerNames[descriptionPlayer]);
            strcat(
                description,
                DATA_COMPGEN(
                    0x0068280C, rmgTownChoiceIs, " town choice is "));
            strcat(
                description,
                g_rmgTownNames[m_townChoices[descriptionPlayer]]);
        }
    }

    {
        int intBuffer = strlen(description);
        outfile->write(&intBuffer, sizeof(intBuffer));
    }
    outfile->write(description, strlen(description));

    {
        char byteBuffer = 1;
        outfile->write(&byteBuffer, sizeof(byteBuffer));
    }
    if (m_mapVersion >= 1) {
        char byteBuffer = 0;
        outfile->write(&byteBuffer, sizeof(byteBuffer));
    }

    {
        unsigned char canBeHuman[8];
        int legalAlignments[8];
        TRmgMapPosition mainTowns[8];
        unsigned char canBeComputer[8];
        memset(canBeHuman, 0, sizeof(canBeHuman));
        memset(legalAlignments, 0, sizeof(legalAlignments));
        memset(mainTowns, 0, sizeof(mainTowns));
        memset(canBeComputer, 0, sizeof(canBeComputer));
        int generatedHumanTowns = 0;
        {
            unsigned int townIndex = 0;
            for (; townIndex < m_zones.size(); ++townIndex) {
                TRmgZone* town = m_zones[townIndex];
                TRmgTownSlot* slot = town->m_slot;
                int player = slot->m_playerIndex;
                if (player < 0)
                    continue;

                player = m_playerIndexMap[player + 1];
                if (player < 0 || !town->m_active)
                    continue;

                if (slot->m_kind == 0 && !canBeHuman[player]) {
                    ++generatedHumanTowns;
                    canBeHuman[player] = 1;
                    mainTowns[player] = town->m_position;
                }

                if (slot->m_kind == 1 && !canBeComputer[player]) {
                    canBeComputer[player] = 1;
                    mainTowns[player] = town->m_position;
                }

                legalAlignments[player] |= 1 << town->m_alignment;
            }
        }

        generatedHumanTowns -= m_humanPlayerCount;
        int reversePlayer = 7;
        do {
            if (canBeHuman[reversePlayer]
                && !m_fixedHumanPlayers[reversePlayer]
                && generatedHumanTowns > 0) {
                canBeComputer[reversePlayer] = 1;
                canBeHuman[reversePlayer] = 0;
                --generatedHumanTowns;
            }
        } while (reversePlayer-- != 0);

        m_computerPlayerCount = m_humanPlayerCount = 0;

        for (int serializedPlayer = 0; serializedPlayer < 8;
             ++serializedPlayer) {
            {
                char byteBuffer = canBeHuman[serializedPlayer];
                outfile->write(&byteBuffer, sizeof(byteBuffer));
            }

            {
                char byteBuffer =
                    canBeHuman[serializedPlayer] || canBeComputer[serializedPlayer];
                outfile->write(&byteBuffer, sizeof(byteBuffer));
            }

            {
                char byteBuffer = 0;
                outfile->write(&byteBuffer, sizeof(byteBuffer));
            }

            if (m_mapVersion >= 2) {
                char byteBuffer = 0;
                outfile->write(&byteBuffer, sizeof(byteBuffer));
            }

            if (m_mapVersion >= 1) {
                unsigned short alignment = legalAlignments[serializedPlayer];
                outfile->write(&alignment, sizeof(alignment));
            } else {
                char byteBuffer = legalAlignments[serializedPlayer];
                outfile->write(&byteBuffer, sizeof(byteBuffer));
            }

            {
                char byteBuffer = 0;
                outfile->write(&byteBuffer, sizeof(byteBuffer));
            }

            if (!canBeHuman[serializedPlayer]
                && !canBeComputer[serializedPlayer]) {
                char byteBuffer = 0;
                outfile->write(&byteBuffer, sizeof(byteBuffer));
            } else {
                if (canBeHuman[serializedPlayer])
                    ++m_humanPlayerCount;
                else
                    ++m_computerPlayerCount;

                {
                    char byteBuffer = 1;
                    outfile->write(&byteBuffer, sizeof(byteBuffer));
                }

                if (m_mapVersion >= 1) {
                    {
                        char byteBuffer = 1;
                        outfile->write(&byteBuffer, sizeof(byteBuffer));
                    }
                    {
                        char byteBuffer = -1;
                        outfile->write(&byteBuffer, sizeof(byteBuffer));
                    }
                }

                {
                    char byteBuffer = mainTowns[serializedPlayer].m_x;
                    outfile->write(&byteBuffer, sizeof(byteBuffer));
                }
                {
                    char byteBuffer = mainTowns[serializedPlayer].m_y;
                    outfile->write(&byteBuffer, sizeof(byteBuffer));
                }
                {
                    char byteBuffer = mainTowns[serializedPlayer].m_z;
                    outfile->write(&byteBuffer, sizeof(byteBuffer));
                }
            }

            {
                char byteBuffer = 0;
                outfile->write(&byteBuffer, sizeof(byteBuffer));
            }
            {
                char byteBuffer = -1;
                outfile->write(&byteBuffer, sizeof(byteBuffer));
            }

            if (m_mapVersion >= 1) {
                {
                    char byteBuffer = 0;
                    outfile->write(&byteBuffer, sizeof(byteBuffer));
                }
                int intBuffer = 0;
                outfile->write(&intBuffer, sizeof(intBuffer));
            }
        }

        {
            char byteBuffer = -1;
            outfile->write(&byteBuffer, sizeof(byteBuffer));
        }
        {
            char byteBuffer = -1;
            outfile->write(&byteBuffer, sizeof(byteBuffer));
        }

        if (!m_computerTeamCount)
            m_computerTeamCount = m_computerPlayerCount;
        if (!m_humanTeamCount)
            m_humanTeamCount = m_humanPlayerCount;
        if (!m_computerPlayerCount) {
            int teamCount = m_humanTeamCount;
            m_humanTeamCount = std::_cpp_max(teamCount, 2);
        }

        if (m_humanTeamCount >= m_humanPlayerCount
            && m_computerTeamCount >= m_computerPlayerCount) {
            char byteBuffer = 0;
            outfile->write(&byteBuffer, sizeof(byteBuffer));
        } else {
            char teams[8];
            memset(teams, 0, sizeof(teams));

            {
                int teamCount = m_humanTeamCount;
                m_humanTeamCount = std::_cpp_max(teamCount, 1);
            }
            {
                int teamCount = m_computerTeamCount;
                m_computerTeamCount = std::_cpp_max(teamCount, 1);
            }
            {
                int playerCount = m_humanPlayerCount;
                int teamCount = m_humanTeamCount;
                m_humanTeamCount = std::_cpp_min(playerCount, teamCount);
            }
            {
                int playerCount = m_computerPlayerCount;
                int teamCount = m_computerTeamCount;
                m_computerTeamCount = std::_cpp_min(playerCount, teamCount);
            }

            assignRmgTeams(
                m_humanTeamCount,
                m_humanPlayerCount,
                0,
                canBeHuman,
                teams);
            assignRmgTeams(
                m_computerTeamCount,
                m_computerPlayerCount,
                m_humanTeamCount,
                canBeComputer,
                teams);

            {
                char byteBuffer = m_humanTeamCount + m_computerTeamCount;
                outfile->write(&byteBuffer, sizeof(byteBuffer));
            }
            outfile->write(teams, sizeof(teams));
        }
    }

    if (m_mapVersion >= 1) {
        std::bitset<156> availableHeroes;
        setAvailableRmgHeroes(
            &availableHeroes, m_disabledHeroes, m_disabledHeroes + 156);

        unsigned char packedHeroes[20];
        memset(packedHeroes, 0, sizeof(packedHeroes));
        for (unsigned int heroBit = 0; heroBit < 156; ++heroBit) {
            if (availableHeroes.test(heroBit))
                packedHeroes[heroBit >> 3] |= 1 << (heroBit & 7);
        }
        outfile->write(packedHeroes, sizeof(packedHeroes));
    } else {
        std::bitset<128> availableHeroes;
        setAvailableRmgHeroes(
            &availableHeroes, m_disabledHeroes, m_disabledHeroes + 128);

        unsigned char packedHeroes[16];
        memset(packedHeroes, 0, sizeof(packedHeroes));
        for (unsigned int roeHeroBit = 0; roeHeroBit < 128; ++roeHeroBit) {
            if (availableHeroes.test(roeHeroBit))
                packedHeroes[roeHeroBit >> 3] |= 1 << (roeHeroBit & 7);
        }
        outfile->write(packedHeroes, sizeof(packedHeroes));
    }

    if (m_mapVersion >= 1) {
        int intBuffer = 0;
        outfile->write(&intBuffer, sizeof(intBuffer));
    }
    if (m_mapVersion >= 2) {
        char byteBuffer = 0;
        outfile->write(&byteBuffer, sizeof(byteBuffer));
    }

    char reserved[31];
    memset(reserved, 0, sizeof(reserved));
    outfile->write(reserved, sizeof(reserved));

    std::bitset<144> disabledArtifacts;
    for (int artifactIndex = 0; artifactIndex < 144; ++artifactIndex) {
        disabledArtifacts[artifactIndex] =
            g_artifactTraits[artifactIndex].m_comboType != -1;
    }
    disabledArtifacts.set(0);
    disabledArtifacts.set(63);

    if (m_mapVersion >= 2) {
        unsigned char packedArtifacts[18];
        memset(packedArtifacts, 0, sizeof(packedArtifacts));
        for (unsigned int artifactBit = 0; artifactBit < 144;
             ++artifactBit) {
            if (disabledArtifacts.test(artifactBit))
                packedArtifacts[artifactBit >> 3] |=
                    1 << (artifactBit & 7);
        }
        outfile->write(packedArtifacts, sizeof(packedArtifacts));
    } else if (m_mapVersion >= 1) {
        std::bitset<129> legacyDisabledArtifacts;
        std::copy(
            bitset_iterator<144>(disabledArtifacts, 0),
            bitset_iterator<144>(disabledArtifacts, 129),
            bitset_iterator<129>(legacyDisabledArtifacts, 0));

        unsigned char packedArtifacts[17];
        memset(packedArtifacts, 0, sizeof(packedArtifacts));
        for (unsigned int legacyArtifactBit = 0; legacyArtifactBit < 129;
             ++legacyArtifactBit) {
            if (legacyDisabledArtifacts.test(legacyArtifactBit))
                packedArtifacts[legacyArtifactBit >> 3] |=
                    1 << (legacyArtifactBit & 7);
        }
        outfile->write(packedArtifacts, sizeof(packedArtifacts));
    }

    if (m_mapVersion >= 2) {
        std::bitset<70> disabledSpells;
        unsigned char packedSpells[9];
        memset(packedSpells, 0, sizeof(packedSpells));
        for (unsigned int spell = 0; spell < 70; ++spell) {
            if (disabledSpells.test(spell))
                packedSpells[spell >> 3] |= 1 << (spell & 7);
        }
        outfile->write(packedSpells, sizeof(packedSpells));

        std::bitset<28> disabledSkills;
        unsigned char packedSkills[4];
        memset(packedSkills, 0, sizeof(packedSkills));
        for (unsigned int skill = 0; skill < 28; ++skill) {
            if (disabledSkills.test(skill))
                packedSkills[skill >> 3] |= 1 << (skill & 7);
        }
        outfile->write(packedSkills, sizeof(packedSkills));

        char byteBuffer = 0;
        for (int hero = 0; hero < 156; ++hero)
            outfile->write(&byteBuffer, sizeof(byteBuffer));
    }
}

// The helper's fastcall ABI is fixed by its two retail call sites: team and
// player counts arrive in ECX/EDX, followed by the first team id and the two
// eight-byte arrays.  Keeping it as a real helper preserves the source-level
// boundary retail chose not to inline.
VA(0x0054AB40, 0xAD)  // sole caller: WriteMapHeader; retail-only RMG
static void __fastcall assignRmgTeams(
    int teamCount,
    int playerCount,
    int firstTeam,
    const unsigned char* players,
    char* teams)
{
    int playersPerTeam[8];
    int team;

    for (team = 0; team < teamCount; ++team) {
        playersPerTeam[team] =
            playerCount / teamCount + (playerCount % teamCount > team);
    }

    for (int player = 0; player < 8; ++player) {
        if (!players[player])
            continue;

        int nonemptyTeams = 0;
        for (team = 0; team < teamCount; ++team) {
            if (playersPerTeam[team] > 0)
                ++nonemptyTeams;
        }

        int selected = rand() % nonemptyTeams;
        for (team = 0; team < teamCount; ++team) {
            if (playersPerTeam[team] > 0 && --selected < 0)
                break;
        }

        teams[player] = firstTeam + static_cast<char>(team);
        --playersPerTeam[team];
    }
}

// Retail 0x54ae30 serializes a TObjectType using its image name and
// masks; ECX is the output file and EDX the prototype. Preserve the
// retained ordinary fastcall helper while its body is reconstructed.
void __fastcall writeRmgObjectPrototype(TAbstractFile*, TObjectType*);

// Retail stream order: header, zero reserved count, every cell, prototype
// indices and records, then placed objects in two trait-ordered passes.
// +0x4a8/+0x7f8 are objectPrototypes[71/124]. Their first entries occupy
// reserved prototype slots 0 and 1; referenced prototypes start at slot 2.
// All names below are provisional Complete-only roles, proved by offsets.
// Residual (95.5071%): all 41 blocks align and 40 sizes match. The main
// outstanding difference is stack-local reuse: frame 0xc versus retail 0x14,
// with terrain-loop counters sharing homes retail keeps separate. Prototype
// and placed-object iteration, call decisions and branch directions agree.
VA(0x0054ABF0, 0x235) // anchor-callee 0x54c05b + WriteMapHeader and map loops; retail-only
unsigned char type_random_map_generator::writeMap(TAbstractFile* outfile)
{
    writeMapHeader(outfile);
    {
        int reserved = 0;
        outfile->write(&reserved, sizeof(reserved));
    }
    TRmgMapItem* item = m_map.m_mapItems;
    for (int z = 0; z < m_map.m_numberLevels; ++z)
        for (int y = 0; y < m_map.m_mapHeight; ++y)
            for (int x = 0; x < m_map.m_mapWidth; ++x) {
                item->write(outfile);
                ++item;
            }
    int prototypeCount = 2;
    for (int type = 0; type < 232; ++type)
        for (unsigned int index = 0; index < m_objectPrototypes[type].size(); ++index) {
            TRmgObjectPropertiesRef* properties = m_objectPrototypes[type][index];
            if (static_cast<int>(properties->m_refCount) > 0)
                properties->m_prototypeIndex = prototypeCount++;
        }
    {
        int count = prototypeCount;
        outfile->write(&count, sizeof(count));
    }
    writeRmgObjectPrototype(outfile, m_objectPrototypes[71][0]->m_prototype);
    writeRmgObjectPrototype(outfile, m_objectPrototypes[124][0]->m_prototype);
    for (int objectType = 0; objectType < 232; ++objectType)
        for (unsigned int prototype = 0; prototype < m_objectPrototypes[objectType].size(); ++prototype) {
            TRmgObjectPropertiesRef* properties = m_objectPrototypes[objectType][prototype];
            if (static_cast<int>(properties->m_refCount) > 0)
                writeRmgObjectPrototype(outfile, properties->m_prototype);
        }
    {
        int count = m_positions.size();
        outfile->write(&count, sizeof(count));
    }
    for (unsigned int first = 0; first < m_positions.size(); ++first) {
        type_object* object = m_positions[first];
        if (g_adventureObjectLandBlocked[object->m_properties->m_prototype->m_objectType][12])
            object->write(outfile, m_mapVersion);
    }
    for (unsigned int second = 0; second < m_positions.size(); ++second) {
        type_object* object = m_positions[second];
        if (!g_adventureObjectLandBlocked[object->m_properties->m_prototype->m_objectType][12])
            object->write(outfile, m_mapVersion);
    }
    if (m_progress)
        m_progress->advance(2000);
    int reserved = 0;
    return outfile->write(&reserved, sizeof(reserved)) == sizeof(reserved);
}

// Retail-only serializer: two getImageName calls, the four bitset fields
// at +4/+0xc/+0x14/+0x18, then type/subtype/category/underlay and 16 zeros.
// The 48-cell loops walk y=5..0 and x=7..0 using the canonical getBitPos;
// the output bit index advances independently and has signed /8 and %8.
// Residual (94.9532%): post-decrement row tests recover retail's zero-test
// backedges. Signed and unsigned counters tie; y>=0 scores 92.1403%, and
// y!=-1 reaches 92.9173%. The remaining frame/local-home differences need
// further lifetime evidence. All seventeen calls agree with retail.
VA(0x0054AE30, 0x2C5) // anchor-callee 0x54acdc + TObjectType image/masks; retail-only
void __fastcall writeRmgObjectPrototype(TAbstractFile* outfile, TObjectType* prototype)
{
    int nameLength = prototype->getImageName().size();
    {
        int length = nameLength;
        outfile->write(&length, sizeof(length));
    }
    outfile->write(prototype->getImageName().c_str(), nameLength);
    {
        unsigned char mask[6] = {0};
        int bit = 0;
        for (int y = 6; y--;)
            for (int x = 7; x >= 0; --x) {
                if (prototype->m_passableMask.test(CObjectType::getBitPos(x, y)))
                    mask[bit / 8] |= 1 << (bit % 8);
                ++bit;
            }
        outfile->write(mask, sizeof(mask));
    }
    {
        unsigned char mask[6] = {0};
        int bit = 0;
        for (int y = 6; y--;)
            for (int x = 7; x >= 0; --x) {
                if (prototype->m_triggerMask.test(CObjectType::getBitPos(x, y)))
                    mask[bit / 8] |= 1 << (bit % 8);
                ++bit;
            }
        outfile->write(mask, sizeof(mask));
    }
    {
        unsigned char mask[2] = {0, 0};
        for (int terrain = 0; terrain < 10; ++terrain)
            if (prototype->m_terrainMask.test(terrain))
                mask[terrain / 8] |= 1 << (terrain % 8);
        outfile->write(mask, sizeof(mask));
    }
    {
        unsigned char mask[2] = {0, 0};
        for (int terrain = 0; terrain < 10; ++terrain)
            if (prototype->m_recommendedTerrainMask.test(terrain))
                mask[terrain / 8] |= 1 << (terrain % 8);
        outfile->write(mask, sizeof(mask));
    }
    {
        int value = prototype->m_objectType;
        outfile->write(&value, sizeof(value));
    }
    {
        int value = prototype->m_subtype;
        outfile->write(&value, sizeof(value));
    }
    {
        char value = prototype->m_slotCategory;
        outfile->write(&value, sizeof(value));
    }
    {
        char value = prototype->m_isUnderlay;
        outfile->write(&value, sizeof(value));
    }
    int reserved[4] = {0, 0, 0, 0};
    outfile->write(reserved, sizeof(reserved));
}

// Retail-only RMG: the prison factory at 0x5348dc passes its generator here,
// then stores the returned hero ID in the prison's +0x24 field. The two reverse
// scans read the existing disabledHeroes mask at +0xf88 and select among its
// zero entries, marking the chosen hero used. The version at +8 selects the
// 128/145 roster bound. selectPrisonHero is a provisional role-derived name;
// Dreamcast contains no RMG compiland.
VA(0x0054B100, 0x71) // anchor-callee 0x5348dc + generator layout; retail-only
int type_random_map_generator::selectPrisonHero()
{
    int available = 0;
    int hero;
    for (hero = (m_mapVersion >= 1 ? 145 : 128) - 1; hero >= 0; --hero) {
        if (!m_disabledHeroes[hero])
            ++available;
    }
    if (!available)
        return -1;

    int selected = rand() % available;
    for (hero = (m_mapVersion >= 1 ? 145 : 128) - 1; hero >= 0; --hero) {
        if (!m_disabledHeroes[hero] && --selected < 0)
            break;
    }
    m_disabledHeroes[hero] = 1;
    return hero;
}

// GenerateRandomMap's call at 0x5862e8 passes width, height and level count.
// The request worker 0x54bf60 independently reads each scalar and copies the
// human-seat and town-choice arrays into the generator. The shared request
// declaration retains these proven UI/RMG fields and their constructor defaults.
// Exact: clear the two arrays with memset and assign strength before water.
// The 36-form array/scalar batch reached 96%; the eight-form store-boundary
// refinement found three exact forms. Initializing both fields in declaration
// order, even with their initializer text reversed, keeps the 96% store swap.
VA(0x0054BF00, 0x57) // anchor-callee 0x5862e8; Complete-only, thiscall ret 0xc
TRandomMapRequest::TRandomMapRequest(int width, int height, int levels)
    : m_width(width), m_height(height), m_levels(levels),
      m_humanPlayerCount(2), m_humanTeamCount(2),
      m_computerPlayerCount(0), m_computerTeamCount(8),
      m_mapVersion(2)
{
    m_monsterStrength = 0;
    m_waterContent = 3;
    memset(m_isHumanSeat, 0, sizeof(m_isHumanSeat));
    memset(m_townType, -1, sizeof(m_townType));
}

// Retail 0x54bf60 constructs the 0x14e0-byte generator on its stack.
// Its eleven pushes agree with ctor 0x537b10's parameter loads and ret 0x2c.
// The eight-seat loop copies request towns to +0xf24 and sets human flags
// at +0xed8. Generation failure returns 3; a failed write returns 2.
// Provisional method names describe these retail-only roles.
// Residual (85.5534%): all 15 CFG blocks and seven branch sites agree.
// Retail retains lower-bound 1 in EBX across the player-count repair and
// seat loop; this compile rematerializes it and changes loop addressing.
// Five scored clamp candidates: the explicit if/else remains best; nested
// min(5,max(1,x)) and its operand reversal reach 75.5049%, while
// max(1,min(5,x)) and its reversal reach 75.4952%. No helper form adopted.
VA(0x0054BF60, 0x130) // anchor-callee 0x54c0d7 + request layout; retail-only
int TRandomMapRequest::generateToFile(TAbstractFile* outfile, void* progress)
{
    int strength = m_monsterStrength + 3;
    if (strength < 1)
        strength = 1;
    else if (strength > 5)
        strength = 5;
    if (m_humanPlayerCount + m_computerPlayerCount < 2) {
        m_humanPlayerCount = 1;
        m_computerPlayerCount = 1;
    }
    type_random_map_generator generator(m_width, m_height, m_levels,
        m_humanPlayerCount, m_humanTeamCount, m_computerPlayerCount,
        m_computerTeamCount, m_waterContent, strength,
        static_cast<TProgressSink*>(progress), m_mapVersion);
    for (int seat = 0; seat < 8; ++seat) {
        if (m_isHumanSeat[seat])
            generator.m_fixedHumanPlayers[seat] = 1;
        generator.m_townChoices[seat] = m_townType[seat];
    }
    if (!generator.generate())
        return RANDOM_MAP_FAILED_3;
    int result = RANDOM_MAP_OK;
    if (!generator.writeMap(outfile))
        result = RANDOM_MAP_FAILED_2;
    return result;
}

// Complete-only request wrapper, called by the lobby at 0x586422. Retail
// constructs an eight-byte TGzFile, forwards it and the progress pointer,
// then destroys it before returning. EH info 0x651f98 has one handler with
// flags 9 and type descriptor 0x677d48: const TOpenFailure&. Its continuation
// at 0x54c104 returns 1. This is positive typed-catch evidence, not catch-all.
VA(0x0054C090, 0x8C) // anchor-callee 0x586422 + TGzFile ctor/dtor; retail-only
int TRandomMapRequest::generate(const char* fileName, void* progress)
{
    try {
        TGzFile outfile(fileName, "wb6");
        return generateToFile(&outfile, progress);
    } catch (const TGzFile::TOpenFailure&) {
        return RANDOM_MAP_FAILED_1;
    }
}

// The branch queue naturally emits these ordinary Dinkumware members.
// Eight-byte coordinate values and 16-byte linked nodes identify list<TPoint>;
// erase's iterator result is returned through a hidden stack pointer.
VA_COMPGEN(0x0054C6A0, 0x4D, LIST_DTOR, TPoint)
VA_COMPGEN(0x0054D000, 0x5E, LIST_INSERT_SINGLE, TPoint)
VA_COMPGEN(0x0054D060, 0x36, LIST_ERASE_ITERATOR, TPoint)
VA_COMPGEN(0x0054D0F0, 0x2D, LIST_BUYNODE, TPoint)

// The legacy artifact-mask conversion calls Dinkumware's 129-bit setter.
VA_COMPGEN(0x0054DED0, 0x63, BITSET_SET, Bitset129)

// Retail 0x5b8bc0 is the nine-block Dinkumware tree-successor walk. Its
// sentinel at 0x6a52c4 is shared only by the RMG set cluster, while the
// enclosing callers lead back to GenerateRandomMap. Dreamcast's STLport
// _M_increment at dc 0x64214 corroborates the helper boundary and CFG shape;
// the RMG type itself is retail-only.
VA_COMPGEN(0x005B8BC0, 0xA3, TREE_CONST_ITERATOR_INC, TPoint)

// Minimum ODR use needed to retain the real VC6/Dinkumware COMDAT. This
// wrapper is not a retail claim and adds no target/report row.
// Before normalization (function): EmitRmgPointSetIncrement.
void __fastcall emitRmgPointSetIncrement(TRmgPointSet::const_iterator* it)
{
    ++*it;
}

// The three-point orientation helper at 0x5fdae0 belongs with the retained
// Voronoi operations in rmg_support.cpp. The earlier emission probe preceded
// recovery of the canonical site/point ownership and retained helper surface.

// BuildVertices at 0x5fdb40 distinguishes displacement arithmetic from point
// translation. It calls these five bodies while forming the circumcenter:
// origin + (edge + perpendicular * numerator / denominator) / 2.
// All 160 raw bytes match, including the stack cleanup sizes. Names are
// provisional; Dreamcast has no corresponding geometry/RMG source records.
// Keep ordinary definitions visible to the RMG arithmetic callers; each
// retained body and each caller's expansion decision are separate evidence.
VA(0x005FDCB0, 0x1E) // caller 0x5fdc49; thiscall, hidden result + eight-byte operand
TRmgVector TRmgVector::operator+(TRmgVector other) const
{
    return TRmgVector(m_x + other.m_x, m_y + other.m_y);
}

VA(0x005FDCD0, 0x1D) // caller 0x5fdc2f; thiscall, ret 8
TRmgVector TRmgVector::operator*(int scale) const
{
    return TRmgVector(m_x * scale, m_y * scale);
}

VA(0x005FDCF0, 0x25) // callers 0x5fdc36/0x5fdc50; signed division, ret 8
TRmgVector TRmgVector::operator/(int divisor) const
{
    return TRmgVector(m_x / divisor, m_y / divisor);
}

VA(0x005FDD20, 0x20) // caller 0x5fdc64; hidden result ECX, two 8-byte values
TPoint operator+(TPoint point, TRmgVector offset)
{
    return TPoint(point.m_x + offset.m_x, point.m_y + offset.m_y);
}

VA(0x005FDD40, 0x20) // callers 0x5fdbd8/0x5fdbf5; hidden result ECX, ret 16
TRmgVector operator-(TPoint left, TPoint right)
{
    return TRmgVector(left.m_x - right.m_x, left.m_y - right.m_y);
}
