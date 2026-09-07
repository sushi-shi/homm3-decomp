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
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "abstractfile.h"
#include "advmgr_objects.h"
#include "artifact.h"
#include "armygrp.h"
#include "bitset_iterator.h"
#include "homm3_minmax.h"
#include "objnames.h"
#include "resourcemanager.h"
#include "rmg.h"
#include "rmg_terrain.h"
#include "textresource.h"
#include "town.h"

typedef std::set<TPoint> TRmgPointSet;

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

#if 0 // @carcass - retained placement helper shared by gate and shipyard paths
VA(0x00531CF0, 0x1A5) // anchor-callee 0x541c73; thiscall, ret 0x14; retail-only
unsigned char type_random_map::canPlaceObject(
    TRmgObjectPropertiesRef* properties, TRmgMapPosition position, TRmgZone* zone)
{
    return 0; // @stub
}
#endif

// Vtable 0x6409cc slot 0 and the 0x14-byte concrete map layout identify this
// scalar deleting wrapper. Its non-deleting half destroys the owned array of
// 0x30-byte TRmgMapItem elements before restoring the abstract map vtable.
VA_COMPGEN(0x00530F80, 0x21, SCALAR_DELETING_DTOR, type_random_map)

// Vtable 0x6409cc slot 3 returns the map's two unsigned dimensions.
// The hidden result pointer and two stores fix the coordinate return ABI.
// The proven grid copy constructor moves the width load before the result
// pointer load (97.56%, with 100% banked). Named constructed and assigned
// results keep that scheduling difference and leave createRiver unchanged.
VA(0x00532240, 0x15) // anchor-vtable 0x6409cc+0x0c; retail-only
TRmgGridPoint type_random_map::getSize()
{
    return TRmgGridPoint(m_mapWidth, m_mapHeight);
}

// Complete-only base of the road adapter, exact on the first scored candidate.
// The derived deleting destructor at 0x532320 and one retail cleanup path call
// this retained vptr restoration; Dreamcast has no RMG compiland.
VA(0x00532350, 0x07)
TRmgRoadMapAdapterInterface::~TRmgRoadMapAdapterInterface()
{
}

// Complete-only base of the river adapter, exact on the first scored candidate.
// The derived deleting destructor at 0x5324e0 and two CreateRiver cleanup paths
// call this retained vptr restoration; Dreamcast has no RMG compiland.
VA(0x00532510, 0x07)
TRmgMapAdapterInterface::~TRmgMapAdapterInterface()
{
}

// Vtable 0x640a3c slot 0 and the 0x08 concrete adapter layout identify this
// scalar deleting wrapper. The retained body delegates to the adapter-interface
// destructor at 0x532510 before conditionally releasing the object.
VA_COMPGEN(0x005324E0, 0x21, SCALAR_DELETING_DTOR, TRmgMapAdapter)

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
VA(0x00532BD0, 0xA8) // anchor-callee 0x53b4b7/0x53b5ae; thiscall, ret 4
unsigned char TRmgZone::canConnect(const TRmgZone* other) const
{
    int dx = m_levelPosition.m_x - other->m_levelPosition.m_x;
    int dy = m_levelPosition.m_y - other->m_levelPosition.m_y;
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
        return combinedSize - distance > minimumSize / 2;
    }
    return 11 * combinedSize >= 10 * distance;
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

#if 0 // @carcass - ownable-object vtable 0x640aa4, slot 3
VA(0x00533460, 0xA0) // base serialization plus unowned player and reserved bytes
void rmgOwnableObject::write(TAbstractFile* outfile, int parameter)
{
} // @stub
#endif

VA_COMPGEN(0x00533590, 0x21, SCALAR_DELETING_DTOR, rmgOwnableObject)

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

// The hero-object factory marks the selected index in disabledHeroes before
// construction. Vtable 0x640b14 slot 1 clears that byte when the reservation
// is released.
VA(0x00533C70, 0x0F)  // factory 0x5348d0; Complete-only RMG object
void rmgHeroObject::unknownOperation()
{
    m_generator->m_disabledHeroes[m_heroIndex] = 0;
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
int type_treasure_def::getValue(void*, void*)
{
    return m_value;
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

VA(0x005349D0, 0x29)
type_shrine_def::type_shrine_def(int newObjectType, int newValue)
    : type_treasure_def(newObjectType, 0, newValue, 100)
{
}

VA(0x00534A60, 0x25)
type_witch_hut_def::type_witch_hut_def()
    : type_treasure_def(0x71, 0, 1500, 80)
{
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
int type_key_tent_def::getValue(void*, void* map)
{
    type_random_map_generator* generator =
        static_cast<type_random_map_generator*>(map);
    if (generator->m_nextKeyTentColor != m_subtype)
        return -1;
    return m_value;
}

// The seven-slot abstract map table at 0x6409e8 and sixteen retail cleanup
// tails identify this virtual base destructor, exact on the first scored
// candidate. Dreamcast has no RMG compiland.
VA(0x005361A0, 0x07)
TRmgMapInterface::~TRmgMapInterface()
{
}

// rand_trn.txt supplies one rule per nonempty row starting at row three.
// The two 232x10 vector grids group rules and subtypes by remapped object
// type and preferred terrain. The final reverse scan gives later rows
// precedence for the same subtype. Names are provisional; RMG is absent
// from the Dreamcast build. Retail fixes the record stride at 0x4c.
// Residual (97.58%): push_back still expands the two-argument rule-vector
// insert that retail calls at 0x536701, and bitset<10>::_Xran expands its
// string constructor where retail calls 0x48b370 at 0x536b26. The int/pointer
// vector destructor, size, erase and pointer-insert name differences are
// ICF aliases. The emitted int-vector allocator constructor is 0x5157d0.
// Source controls: initialize row before the vectors, increment it before
// rule destruction, and pass an explicit zero to both resize calls (84.89%
// versus 83.93%). A post-decrement reverse scan gives retail's old-count
// tests (89.99%); a signed size()-1 / >=0 loop does not. Reusing objectType,
// subtype and terrain across parsing/binding preserves their escaped homes
// and restores the binding pass (97.58%). Separate locals strength-reduce
// the type stride and lose the terrain/subtype homes.
// An index-taking rule constructor moves the completed-construction EH
// state past the id assignment (97.31%); retain default-then-assign. Direct
// insert(end(),rule) still expands its two-argument overload and costs
// additional scheduling differences (95.87%). Signed/unsigned prototype
// indices and combined/nested reverse-loop conditions are byte-neutral.
// Temporary depth 1 at push_back and depth 2 at test, intended to stop the
// named nested callees, are byte-neutral at 97.58%; both were removed.
// Removing the old TU-local string constructor specializations and their
// three pins is also neutral here. Canonical library constructors remain;
// WriteMapHeader's 95.71% MAX is preserved with that collateral measured.
VA(0x00536560, 0x5F2) // anchor-string rand_trn.txt; thiscall, ret 0; retail-only
void type_random_map_generator::readObjectPlacementRules()
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
                if (prototype->m_recommendedTerrainMask.test(terrain))
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
int type_random_map_generator::scoreObjectPlacement(
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
    TTerrainType terrain;
    TRmgMapPosition position;
    TRmgMapPosition nearby;
    std::vector<TRmgMapPosition> positions;
    std::vector<TTerrainType> terrains;
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
            TTerrainType lastTerrain = terrains[0];
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

#if 0 // @carcass - retained connection helpers; names describe retail roles
// The ground connection caller passes a 12-byte position and narrow flag.
// The body follows each cell's predecessor while clearing its path cost.
VA(0x005408E0, 0x23F) // anchor-callee createGroundConnection; thiscall, ret 0x10
void type_random_map_generator::openConnectionPath(
    TRmgMapPosition position, unsigned char narrow)
{
} // @stub

// Zone restrictions select a creature, the requested value determines its
// count, and the result is a newly allocated guarded object (vtbl 0x640a84).
VA(0x00540B20, 0x240) // anchor-callee 0x54203b; thiscall, ret 8; retail-only
type_object* type_random_map_generator::createGuard(int value, TRmgZone* zone)
{
    return 0; // @stub
}

#endif

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

#if 0 // @carcass - retained connection decoration
// Both ground border placements call this with their returned direction.
// Retail clips the surrounding rectangle and updates connection/obstacle bits.
VA(0x00540FC0, 0x172) // anchor-callee createGroundConnection; thiscall, ret 0x10
void type_random_map_generator::markBorderObjectArea(
    TRmgMapPosition position, int direction)
{
} // @stub
#endif

// Provisional arithmetic boundary for the Complete-only position value.
// CreateRiver keeps position.x in EBX and jumps over its backedge reload.
// A by-value direction and returned coordinate construction recover that
// sequence; a reference operand leaves a different relaxation register flow.
// Keep this ordinary body visible to the direction-addition call sites.
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
// Residual (80.43471%): the shared by-value guard helper restores final
// map-item calls (flattened bodies: 77.31306%), but they target the scalar
// accessor expansion rather than the retained value overload. The second
// occupancy size call stays out of line and the final vector destructor expands.
// Delegating the by-value accessor to its scalar overload
// restores the first clear's retained range erase; direct accessor arithmetic
// expands that erase into copy/_Destroy (76.75134%).
// Retail additionally preserves a 12-byte position temporary that this
// candidate folds away.  These are unresolved source/lifetime boundaries.
// Controls: direct range-erase expands further (71.29874%); positive eligibility
// scopes versus early continues, a by-value left operand, copy assignment,
// an intermediate position local, and inline versus ordinary operator bodies
// are byte-flat.  A by-value direction scores 74.07692%.  A temporary depth-1
// limit on clear is also flat and has been removed.  Naming candidateCount
// before the empty check scores 75.386406%; using the two natural size calls
// restores retail's reuse of the count through the min wrapper.
VA(0x00541140, 0x63A) // anchor-callee ConnectZones 0x543550; retail-only
unsigned char type_random_map_generator::createGroundConnection(
    TRmgZone* source,
    TRmgZoneConnection* connection,
    std::vector<TRmgMapItem*>* borderItems,
    std::vector<TRmgMapPosition>* borderPositions)
{
    TRmgZone* destination = m_zones[connection->m_destination->m_zoneIndex];
    int sourceZone = source->m_slot->m_zoneIndex;
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

#if 0 // @carcass - retained shipyard connection helpers
VA(0x00541780, 0x18D) // anchor-callee 0x541f1f; thiscall, ret 0x0c
void type_random_map_generator::floodConnectionRegion(TRmgMapPosition position)
{
} // @stub

VA(0x00541960, 0x16C) // anchor-callee 0x541c94; thiscall, ret 0x0c
unsigned char type_random_map_generator::canPlaceShipyard(TRmgMapPosition position)
{
    return 0; // @stub
}
#endif

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
VA(0x00542080, 0x8AA)
unsigned char type_random_map_generator::createSubterraneanGate(
    TRmgZone* source, TRmgZoneConnection* connection)
{
    TRmgZone* destination = m_zones[connection->m_destination->m_zoneIndex];
    int sourceZone = source->m_slot->m_zoneIndex;
    int destinationZone = destination->m_slot->m_zoneIndex;
    TRmgMapPosition sourceLevelPosition = source->m_levelPosition;
    TRmgMapPosition destinationLevelPosition = destination->m_levelPosition;

    if (sourceLevelPosition.m_z == destinationLevelPosition.m_z)
        return 0;
    if (source->m_terrain == eTerrainWater)
        return 0;

    TRmgZoneBounds sourceBounds = source->m_bounds;
    TRmgZoneBounds destinationBounds = destination->m_bounds;
    int minimumX = std::_cpp_max(
        sourceBounds.m_minimumX, destinationBounds.m_minimumX);
    int minimumY = std::_cpp_max(
        sourceBounds.m_minimumY, destinationBounds.m_minimumY);
    int maximumX = std::_cpp_min(
        sourceBounds.m_maximumX, destinationBounds.m_maximumX);
    int maximumY = std::_cpp_min(
        sourceBounds.m_maximumY, destinationBounds.m_maximumY);
    if (minimumX >= maximumX || minimumY >= maximumY)
        return 0;

    int gateIndex = rand() % m_objectPrototypes[103].size();
    TRmgObjectPropertiesRef* gateProperties = m_objectPrototypes[103][gateIndex];
    TObjectType* gatePrototype = gateProperties->m_prototype;

    std::vector<TRmgMapPosition> candidates;
    int bestScore = 0;
    TRmgMapPosition position = sourceLevelPosition;

    for (position.m_y = minimumY; position.m_y < maximumY; ++position.m_y) {
        for (position.m_x = minimumX; position.m_x < maximumX; ++position.m_x) {
            TRmgMapItem* sourceItem = m_map.getMapItem(position);
            int score = sourceItem->m_zoneState.m_score;
            if (sourceItem->m_zoneState.m_zone != sourceZone)
                continue;

            TRmgMapPosition otherPosition = destination->m_levelPosition;
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

    if (candidates.empty())
        return 0;

    position = candidates[rand() % candidates.size()];
    addObject(new type_object(gateProperties), position);

    TRmgMapPosition otherPosition = destination->m_levelPosition;
    otherPosition.m_x = position.m_x;
    otherPosition.m_y = position.m_y;
    addObject(new type_object(gateProperties), otherPosition);

    position.m_x -= gatePrototype->m_triggerCell.m_x;
    position.m_y -= gatePrototype->m_triggerCell.m_y;
    otherPosition = destination->m_levelPosition;
    otherPosition.m_x = position.m_x;
    otherPosition.m_y = position.m_y;
    TPoint entrance(position.m_x, position.m_y);
    source->m_entrances.push_back(entrance);
    destination->m_entrances.push_back(entrance);

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

#if 0 // @carcass - retained object placement in a zone
// Called by placeBorderObject at 0x540e81 with a newly created tent and zone.
// Scans the zone bounds for matching cells accepted by canPlaceObject, then
// chooses a candidate through rand and forwards it to virtual addObject.
VA(0x00542930, 0x1C6) // anchor-callee 0x540e81; thiscall, ret 8; retail-only
unsigned char type_random_map_generator::placeObjectInZone(type_object* object, TRmgZone* zone)
{
    return 0; // @stub
}
#endif

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

// The road/river worklists instantiate all three of these out-of-line STL
// bodies.  Their distinct retail extents disambiguate the two int overloads.
VA_COMPGEN(0x00404200, 0x209, VECTOR_INSERT, Int)
VA_COMPGEN(0x00422F50, 0x1B1, VECTOR_INSERT, Int)
VA_COMPGEN(0x004347A0, 0x32E, VECTOR_INSERT, TRmgMapPosition)

// The RMG position insertion at 0x54c3f0 and spellbook's 12-byte entry
// insertion both call retail 0x54dd60. Their plain three-dword copies are
// ICF-identical; this TU naturally emits the TRmgMapPosition specialization.
VA_COMPGEN(0x0054DD60, 0x15, STD_CONSTRUCT, TRmgMapPosition)

// ReadObjectPlacementRules retains the allocator-taking int-vector ctor;
// its two local vector grids also take the default-constructor closure's
// address. Resolved retail bodies are 27/27 and 24/24 bytes respectively.
VA_COMPGEN(0x005157D0, 0x1B, CLASS_CTOR, vector)
VA_COMPGEN(0x00536BA0, 0x18, DEFAULT_CTOR_CLOSURE, vector)

// FilterZonePositions retains this size calculation four times. Retail
// divides the template connection pointer span by its proven 0x1c stride.
VA_COMPGEN(0x0054C1B0, 0x23, VECTOR_SIZE, TRmgZoneConnection)

// DrawIrregularZoneBoundary retains this single-element erase. Its
// eight-byte copy loop and ret 4 agree in all 61 raw retail bytes.
VA_COMPGEN(0x0054CD70, 0x3D, VECTOR_ERASE, TPoint)

// The reader's two resize shrink arms retain this int-vector erase.
// All 51 raw bytes agree; no calls or data relocations remain unresolved.
VA_COMPGEN(0x0054CDB0, 0x33, VECTOR_ERASE, Int)

// FilterZonePositions erases 12-byte positions through this forward copy;
// the retained body copies three dwords and returns the end pointer.
VA_COMPGEN(0x0054D9E0, 0x39, STD_COPY, TRmgMapPosition)

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
        TTerrainType landType = mapItem->m_tile.m_landType;
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

                player = m_playerIndexMap[player];
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
