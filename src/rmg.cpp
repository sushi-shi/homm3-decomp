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

namespace {

// The cinit at 0x530da0 writes these eight clockwise neighbours.  The river
// search advances by two entries, selecting only the four cardinal offsets.
// The current TPoint constructor reproduces all 127 initializer bytes after
// resolving the 16 table references, including repeated constant loads.
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
DATA(0x0069CE00)
TPoint g_rmgShipyardWaterOffsets[RMG_SHIPYARD_WATER_OFFSET_COUNT] = {
    TPoint(-3, 0),
    TPoint(1, 0),
    TPoint(-3, 1),
    TPoint(1, 1)
};

DATA(0x006409A0)
static const int g_landRiverDeltaIndex[4] = {2, 0, 3, 1};

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
DATA(0x006823F0) extern int gRmgGuardThresholdLow[];
DATA(0x00682408) extern int gRmgGuardThresholdHigh[];
DATA(0x00682420) extern int gRmgGuardScaleLow[];
DATA(0x00682438) extern int gRmgGuardScaleHigh[];

DATA(0x00682700)
static const char* gRmgWaterNames[3] = {
    DATA_COMPGEN(0x006827EC, rmgWaterNone, "None"),
    DATA_COMPGEN(0x006827E4, rmgWaterNormal, "normal"),
    DATA_COMPGEN(0x006827DC, rmgWaterIslands, "islands")
};

DATA(0x0068270C)
static const char* gRmgPlayerNames[8] = {
    DATA_COMPGEN(0x006827D8, rmgPlayerRed, "red"),
    DATA_COMPGEN(0x006827D0, rmgPlayerBlue, "blue"),
    DATA_COMPGEN(0x006827CC, rmgPlayerTan, "tan"),
    DATA_COMPGEN(0x006827C4, rmgPlayerGreen, "green"),
    DATA_COMPGEN(0x006827BC, rmgPlayerOrange, "orange"),
    DATA_COMPGEN(0x006827B4, rmgPlayerPurple, "purple"),
    DATA_COMPGEN(0x006827AC, rmgPlayerTeal, "teal"),
    DATA_COMPGEN(0x006827A4, rmgPlayerPink, "pink")
};

DATA(0x0068272C)
static const char* gRmgTownNames[9] = {
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
static bool IsRmgTemplateFieldSet(const char* value)
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

static void __fastcall assign_rmg_teams(
    int teamCount,
    int playerCount,
    int firstTeam,
    const unsigned char* players,
    char* teams);

template <unsigned int N>
static void set_available_rmg_heroes(
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

// Vtable 0x6409cc slot 3 returns the map's two unsigned dimensions.
// The hidden result pointer and two stores fix the coordinate return ABI.
// The proven grid copy constructor moves the width load before the result
// pointer load (97.56%, with 100% banked). Named constructed and assigned
// results keep that scheduling difference and leave createRiver unchanged.
VA(0x00532240, 0x15) // anchor-vtable 0x6409cc+0x0c; retail-only
TRmgGridPoint type_random_map::GetSize()
{
    return TRmgGridPoint(mapWidth, mapHeight);
}

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
    slot = newSlot;
    int available = 0;
    for (int town = 0; town < 9; ++town) {
        if (newSlot->allowedTowns[town])
            ++available;
    }
    int selectedTown;
    if (available) {
        int selected = rand() % available;
        for (selectedTown = 0; selectedTown < 9; ++selectedTown) {
            if (newSlot->allowedTowns[selectedTown] && --selected < 0)
                goto townSelected;
        }
    }
    selectedTown = -1;
townSelected:
    alignment = selectedTown;
    boundaryRoughness = newSlot->size;
    bounds.minimumX = 32000;
    bounds.maximumX = -32000;
    bounds.minimumY = 32000;
    bounds.maximumY = -32000;
    active = 0;
    memset(counts0044, 0, sizeof(counts0044));
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
TRmgMapPosition TRmgZone::GetLevelPosition() const
{
    return levelPosition;
}

// Candidate placement loads all three coordinates before writing the zone,
// consistent with passing the coordinate value through an ordinary setter.
void TRmgZone::SetLevelPosition(TRmgMapPosition position)
{
    levelPosition = position;
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
unsigned char TRmgZone::CanConnect(const TRmgZone* other) const
{
    int dx = levelPosition.x - other->levelPosition.x;
    int dy = levelPosition.y - other->levelPosition.y;
    int distance = static_cast<int>(sqrt(static_cast<double>(dx * dx + dy * dy)));
    int otherSize = other->slot->size;
    int thisSize = slot->size;
    int combinedSize = thisSize + otherSize;
    if (other->levelPosition.z != levelPosition.z) {
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
void TRmgObjectPropertiesRef::BuildOverlapPriorities()
{
    if (prioritiesInitialized)
        return;
    prioritiesInitialized = 1;
    for (unsigned int x = 0; x < prototype->GetWidth(); ++x) {
        int priority = !prototype->isUnderlay;
        unsigned int y = 0;
        for (;;) {
            if (prototype->imageInfo.drawMask.test(CObjectType::_getBitPos(x, y)))
                overlapPriorities[x][y] = priority;
            if (++y >= prototype->GetHeight())
                break;
            if (!prototype->isUnderlay) {
                if (prototype->passableMask.test(CObjectType::_getBitPos(x, y))) {
                    if (x > 0 && !prototype->passableMask.test(
                            CObjectType::_getBitPos(x - 1, y)))
                        priority = overlapPriorities[x - 1][y];
                    else
                        ++priority;
                } else {
                    if (prototype->passableMask.test(
                            CObjectType::_getBitPos(x, y - 1)))
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
    for (int zone = 0; zone < zones.size(); ++zone)
        delete zones[zone];
}

// The rmg.txt connection reader calls this for both endpoint identifiers.
// It searches the template's pointer vector and compares each slot's first
// field; ret 4 fixes the member's one integer argument.
VA(0x005330A0, 0x3E) // anchor-callee 0x53824c/0x538257; retail-only
TRmgTownSlot* TRmgTemplate::FindZone(int zoneIndex)
{
    for (int zone = 0; zone < zones.size(); ++zone) {
        if (zones[zone]->zoneIndex == zoneIndex)
            return zones[zone];
    }
    return 0;
}

// Complete-only helper called by InitializeObjectGenerators at 0x538b10.
// The four argument loads, five stores, vtable relocation, and `ret 0x10`
// independently prove this constructor and the shared 0x14-byte prefix.
VA(0x00534160, 0x27)
type_treasure_def::type_treasure_def(
    int newObjectType, int newSubtype, int newValue, int newDensity)
{
    objectType = newObjectType;
    subtype = newSubtype;
    value = newValue;
    density = newDensity;
}

// The compiler expands the common four-store constructor in each of these
// derived definitions; retail retains only the derived vptr store.  This is
// ordinary /Ob2 expansion of a real helper boundary, not a hand-flattened
// substitute for that boundary.
VA(0x00534250, 0xB5)
type_black_box_creature_def::type_black_box_creature_def(int newCreatureType)
    : type_treasure_def(6, 0, -1, 3),
      creatureType(newCreatureType)
{
    adjustedValue =
        gRmgCreatureValueByLevel[akCreatureTypeTraits[newCreatureType].level]
        / akCreatureTypeTraits[newCreatureType].AI_value;

    if (adjustedValue > 50)
        adjustedValue = ((adjustedValue + 5) / 10) * 10;
    else if (adjustedValue > 12)
        adjustedValue = ((adjustedValue + 2) / 5) * 5;
    else if (adjustedValue > 5)
        adjustedValue = ((adjustedValue + 1) / 2) * 2;
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
    spellLevel = newSpellLevel;
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
void type_random_map_generator::ReadObjectPlacementRules()
{
    TSpreadsheetResource* sheet = ResourceManager::GetSpreadsheet(
        DATA_COMPGEN(0x006827F4, rmgPlacementRulesFilename, "rand_trn.txt"));
    int row = 3;
    std::vector<int> objectTypes;
    std::vector<int> terrains;
    std::vector<int> subtypes;
    int objectType;
    int subtype;
    int terrain;
    for (; row < sheet->GetNumberOfRows();) {
        const TSpreadsheetResource::TStringVector& values = sheet->GetRow(row);
        if (values[0][0] == ' ' || values[0][0] == 0)
            break;
        TRmgObjectPlacementRule rule;
        rule.index = row - 3;
        objectType = atoi(values[3]);
        subtype = atoi(values[4]);
        terrain = atoi(values[6]);
        objectTypes.push_back(objectType);
        terrains.push_back(terrain);
        subtypes.push_back(subtype);
        for (terrain = 0; terrain <= eTerrainWater; ++terrain)
            rule.terrainScores[terrain] = atoi(values[terrain + 7]);
        for (; terrain < 10; ++terrain)
            rule.terrainScores[terrain] = RMG_PLACEMENT_INVALID;
        placementRules.push_back(rule);
        ++row;
    }
    int ruleCount = placementRules.size();
    for (row = 3; row < ruleCount + 3; ++row) {
        const TSpreadsheetResource::TStringVector& values = sheet->GetRow(row);
        TRmgObjectPlacementRule& rule = placementRules[row - 3];
        rule.adjacentScores.resize(ruleCount, 0);
        for (int index = 0; index < ruleCount; ++index)
            rule.adjacentScores[index] = atoi(values[index + 16]);
        rule.blockedScores.resize(ruleCount, 0);
        for (index = 0; index < ruleCount; ++index)
            rule.blockedScores[index] = atoi(values[index + ruleCount + 16]);
    }
    sheet->Dispose();

    std::vector<TRmgObjectPlacementRule*> rulesByType[ADVENTURE_OBJECT_TRAIT_COUNT][10];
    std::vector<int> subtypesByType[ADVENTURE_OBJECT_TRAIT_COUNT][10];
    for (int index = 0; index < ruleCount; ++index) {
        TRmgObjectPlacementRule* rule = &placementRules[index];
        rulesByType[objectTypes[index]][terrains[index]].push_back(rule);
        subtypesByType[objectTypes[index]][terrains[index]].push_back(subtypes[index]);
    }
    for (objectType = 0; objectType < ADVENTURE_OBJECT_TRAIT_COUNT; ++objectType) {
        for (int index = 0; index < objectPrototypes[objectType].size();
             ++index) {
            TRmgObjectPropertiesRef* properties = objectPrototypes[objectType][index];
            TObjectType* prototype = properties->prototype;
            properties->placementRule = 0;
            for (terrain = 0; terrain < eTerrainRock; ++terrain) {
                if (prototype->recommendedTerrainMask.test(terrain))
                    break;
            }
            properties->preferredTerrain = terrain;
            if (terrain != eTerrainRock) {
                subtype = prototype->subtype;
                int mappedType;
                // Same canonical byte table used by readObjectType: the
                // dword at +8 remaps aliases to their objnames.txt row.
                memcpy(&mappedType, &gAdventureObjectLandBlocked[objectType][8],
                       sizeof(mappedType));
                int match = rulesByType[mappedType][terrain].size();
                while (match-- && subtypesByType[mappedType][terrain][match] != subtype)
                    ;
                if (match >= 0)
                    properties->placementRule = rulesByType[mappedType][terrain][match];
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
int type_random_map_generator::ScoreObjectPlacement(
    TRmgObjectPropertiesRef* properties, TRmgMapPosition position)
{
    TObjectType* prototype = properties->prototype;
    std::vector<type_object*> affected;
    unsigned char terrainSeen[10];
    memset(terrainSeen, 0, sizeof(terrainSeen));
    unsigned int marks[10][8];
    memset(marks, 0, sizeof(marks));
    for (unsigned int row = 0; row < prototype->GetHeight(); ++row) {
        int y = position.y - row;
        if (y < 0 || y >= map.mapHeight)
            continue;
        for (unsigned int column = 0; column < prototype->GetWidth(); ++column) {
            int x = position.x - column;
            if (x < 0 || x >= map.mapWidth)
                continue;
            if (!prototype->imageInfo.drawMask[
                    CObjectType::_getBitPos(column, row)])
                continue;
            marks[column + 1][row + 1] |= RMG_PLACEMENT_OVERLAP;
            if (!prototype->passableMask[CObjectType::_getBitPos(column, row)]) {
                marks[column + 1][row + 1] |= RMG_PLACEMENT_BLOCKED;
                TRmgMapItem* item = map.GetMapItem(x, y, position.z);
                if (!prototype->terrainMask[item->tile.landType])
                    return RMG_PLACEMENT_INVALID;
                if (item->HasSubterraneanGate())
                    return RMG_PLACEMENT_INVALID;

                // Retail 0x536d34 overwrites the complete mark with one
                // before marking the surrounding area; retain that store.
                marks[column + 1][row + 1] = RMG_PLACEMENT_ADJACENT;
                terrainSeen[item->tile.landType] = 1;
                int firstRow = position.y - min(y + 1, map.mapHeight) + 1;
                int lastRow = position.y - max(y - 2, 0) + 1;
                int firstColumn = position.x - min(x + 1, map.mapWidth) + 1;
                int lastColumn = position.x - max(x - 2, 0) + 1;
                for (int nearColumn = firstColumn; nearColumn < lastColumn;
                     ++nearColumn) {
                    for (int nearRow = firstRow; nearRow < lastRow; ++nearRow)
                        marks[nearColumn][nearRow] |= RMG_PLACEMENT_ADJACENT;
                }
            }
        }
    }

    TRmgObjectPlacementRule* rule = properties->placementRule;
    int score = 0;
    unsigned char hasPositiveTerrain = 0;
    for (int terrain = 0; terrain < 10; ++terrain) {
        if (terrainSeen[terrain]) {
            score += rule->terrainScores[terrain];
            if (rule->terrainScores[terrain] > 0)
                hasPositiveTerrain = 1;
        }
    }
    if (score < RMG_PLACEMENT_MINIMUM_TERRAIN_SCORE)
        return score;
    if (!hasPositiveTerrain)
        return RMG_PLACEMENT_NO_TERRAIN_PREFERENCE;

    properties->BuildOverlapPriorities();
    for (row = 0; row < prototype->GetHeight() + 2; ++row) {
        int y = position.y + 1 - row;
        if (y < 0 || y >= map.mapHeight)
            continue;
        for (unsigned int column = 0; column < prototype->GetWidth() + 2;
             ++column) {
            int x = position.x + 1 - column;
            if (x < 0 || x >= map.mapWidth)
                continue;
            unsigned int mark = marks[column][row];
            if (!mark)
                continue;
            TRmgMapItem* item = map.GetMapItem(x, y, position.z);
            if (item->tileData.roadPassable && item->tile.landType != eTerrainRock)
                continue;
            int priority;
            if (mark & RMG_PLACEMENT_OVERLAP)
                priority = properties->overlapPriorities[column - 1][row - 1];
            for (int index = 0; index < static_cast<int>(item->objects.size());
                 ++index) {
                type_object* object = item->objects[index];
                unsigned char wasTouched = object->IsPlacementTouched();
                if (mark & RMG_PLACEMENT_OVERLAP) {
                    object->properties->BuildOverlapPriorities();
                    if (object->properties->overlapPriorities
                            [object->position.x - x][object->position.y - y]
                        <= priority)
                        object->candidateCovers = 1;
                    else
                        object->candidateBehind = 1;
                    object->overlapsCandidate = 1;
                }
                if (mark & RMG_PLACEMENT_ADJACENT)
                    object->adjacentToCandidate = 1;
                if (mark & RMG_PLACEMENT_BLOCKED)
                    object->blockedByCandidate = 1;
                if (!wasTouched && object->IsPlacementTouched())
                    affected.push_back(object);
                if (object->candidateBehind && object->candidateCovers)
                    break;
            }
        }
    }

    for (unsigned int index = 0; index < affected.size(); ++index) {
        type_object* object = affected[index];
        if (object->blockedByCandidate) {
            if (!object->properties->placementRule)
                score = RMG_PLACEMENT_INVALID;
            else
                score += rule->blockedScores[object->properties->placementRule->index];
        } else if (object->adjacentToCandidate) {
            if (object->properties->placementRule)
                score += rule->adjacentScores[object->properties->placementRule->index];
        }
        if (object->candidateBehind && object->candidateCovers)
            score = RMG_PLACEMENT_INVALID;
        object->ClearPlacementMarks();
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
TRmgMapItem* type_random_map::GetMapItem(TRmgMapPosition point)
{
    return GetMapItem(point.x, point.y, point.z);
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
void ReadRmgTemplateZones(
    const TSpreadsheetResource* sheet, TRmgTemplate* mapTemplate,
    int firstRow, int endRow, int humanPlayers, int computerPlayers,
    int mapVersion)
{
    for (int row = firstRow; row < endRow; ++row) {
        const TSpreadsheetResource::TStringVector& values = sheet->GetRow(row);
        if (values.size() >= 3 && IsRmgTemplateFieldSet(values[3]) &&
            values.size() > 75) {

            TRmgTownSlot* slot = new TRmgTownSlot;
            slot->zoneIndex = atoi(values[3]);
            slot->kind = RMG_TEMPLATE_TREASURE;
            if (IsRmgTemplateFieldSet(values[4]))
                slot->kind = RMG_TEMPLATE_HUMAN;
            if (IsRmgTemplateFieldSet(values[5]))
                slot->kind = RMG_TEMPLATE_COMPUTER;
            if (IsRmgTemplateFieldSet(values[6]))
                slot->kind = RMG_TEMPLATE_TREASURE;
            if (IsRmgTemplateFieldSet(values[7]))
                slot->kind = RMG_TEMPLATE_JUNCTION;
            slot->size = atoi(values[8]);
            slot->minimumHumanPlayers = atoi(values[9]);
            slot->maximumHumanPlayers = atoi(values[10]);
            slot->minimumPlayers = atoi(values[11]);
            slot->maximumPlayers = atoi(values[12]);
            if (slot->minimumHumanPlayers > humanPlayers ||
                slot->maximumHumanPlayers < humanPlayers ||
                slot->minimumPlayers > humanPlayers + computerPlayers ||
                slot->maximumPlayers < humanPlayers + computerPlayers) {
                delete slot;
            } else {
                slot->playerIndex = atoi(values[13]) - 1;
                slot->parameters0020[0] = atoi(values[14]);
                slot->parameters0020[1] = atoi(values[15]);
                slot->parameters0020[2] = atoi(values[16]);
                slot->parameters0020[3] = atoi(values[17]);
                slot->parameters0020[4] = atoi(values[18]);
                slot->parameters0020[5] = atoi(values[19]);
                slot->parameters0020[6] = atoi(values[20]);
                slot->parameters0020[7] = atoi(values[21]);
                slot->flag0040 = 0;
                if (IsRmgTemplateFieldSet(values[22]))
                    slot->flag0040 = 1;
                int townCount;
                if (mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE)
                    townCount = 9;
                else {
                    townCount = 8;
                    slot->allowedTowns[8] = 0;
                }
                while (townCount--) {
                    if (IsRmgTemplateFieldSet(values[23 + townCount]))
                        slot->allowedTowns[townCount] = 1;
                    else
                        slot->allowedTowns[townCount] = 0;
                }
                for (int mine = 0; mine < 7; ++mine)
                    slot->parameters004c[mine] = atoi(values[32 + mine]);
                for (int resource = 0; resource < 7; ++resource)
                    slot->parameters0068[resource] = atoi(values[39 + resource]);
                slot->flag0084 = IsRmgTemplateFieldSet(values[46]);
                unsigned char anyTerrain = 0;
                for (int terrain = 0; terrain < 8; ++terrain) {
                    slot->allowedTerrain[terrain] =
                        IsRmgTemplateFieldSet(values[47 + terrain]);
                    if (slot->allowedTerrain[terrain])
                        anyTerrain = 1;
                }
                if (!anyTerrain)
                    slot->allowedTerrain[0] = 1;
                switch (tolower(values[55][0])) {
                case 'n': slot->monsterStrength = 0; break;
                case 'w': slot->monsterStrength = 2; break;
                case 's': slot->monsterStrength = 4; break;
                case 'a': slot->monsterStrength = 3; break;
                default: slot->monsterStrength = 3; break;
                }
                slot->flag0094 = IsRmgTemplateFieldSet(values[56]);
                for (int monster = 0; monster < 10; ++monster)
                    slot->allowedMonsters[monster] =
                        IsRmgTemplateFieldSet(values[57 + monster]);
                if (mapVersion < RMG_MAP_ARMAGEDDONS_BLADE)
                    slot->allowedMonsters[8] = 0;
                for (int treasure = 0; treasure < 3; ++treasure) {
                    slot->treasure[treasure].minimum = atoi(values[67 + 3 * treasure]);
                    slot->treasure[treasure].maximum = atoi(values[68 + 3 * treasure]);
                    slot->treasure[treasure].density = atoi(values[69 + 3 * treasure]);
                }
                mapTemplate->zones.push_back(slot);
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
void type_random_map_generator::InitializeObjectGenerators()
{
    objectGenerators.push_back(new type_treasure_def(2, 0, 100, 20));
    objectGenerators.push_back(new type_treasure_def(4, 0, 3000, 50));

    {
        int creatureCount = mapVersion >= 1 ? 145 : 118;
        for (int creature = creatureCount; creature--;) {
            if (akCreatureTypeTraits[creature].level >= 0)
                objectGenerators.push_back(
                    new type_black_box_creature_def(creature));
        }
    }

    objectGenerators.push_back(
        new type_black_box_experience_def(6000, 5000));
    objectGenerators.push_back(
        new type_black_box_experience_def(12000, 10000));
    objectGenerators.push_back(
        new type_black_box_experience_def(18000, 15000));
    objectGenerators.push_back(
        new type_black_box_experience_def(24000, 20000));

    objectGenerators.push_back(new type_black_box_gold_def(5000, 5000));
    objectGenerators.push_back(new type_black_box_gold_def(10000, 10000));
    objectGenerators.push_back(new type_black_box_gold_def(15000, 15000));
    objectGenerators.push_back(new type_black_box_gold_def(20000, 20000));

    objectGenerators.push_back(new type_black_box_spells_def(5000, 1, 1, 15));
    objectGenerators.push_back(new type_black_box_spells_def(7500, 2, 2, 15));
    objectGenerators.push_back(new type_black_box_spells_def(10000, 3, 3, 15));
    objectGenerators.push_back(new type_black_box_spells_def(12500, 4, 4, 15));
    objectGenerators.push_back(new type_black_box_spells_def(15000, 5, 5, 15));
    objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, 1));
    objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, 2));
    objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, 4));
    objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, 8));
    objectGenerators.push_back(new type_black_box_spells_def(30000, 1, 5, 15));

    {
        int player = objectPrototypes[10].size();
        disabledKeyTents.resize(player);
        for (; player--;) {
            disabledKeyTents[player] = 0;
            objectGenerators.push_back(new type_key_tent_def(player, 5000));
            objectGenerators.push_back(new type_key_tent_def(player, 7500));
            objectGenerators.push_back(new type_key_tent_def(player, 10000));
            objectGenerators.push_back(new type_key_tent_def(player, 15000));
            objectGenerators.push_back(new type_key_tent_def(player, 20000));
        }
    }

    objectGenerators.push_back(new type_treasure_def(7, 0, 8000, 20));
    objectGenerators.push_back(new type_treasure_def(11, 0, 100, 100));
    objectGenerators.push_back(new type_treasure_def(12, 0, 2000, 500));
    objectGenerators.push_back(new type_treasure_def(13, 0, 5000, 20));
    objectGenerators.push_back(new type_treasure_def(13, 1, 10000, 20));
    objectGenerators.push_back(new type_treasure_def(13, 2, 7500, 20));
    objectGenerators.push_back(new type_treasure_def(14, 0, 100, 100));
    objectGenerators.push_back(new type_treasure_def(16, 0, 3000, 100));
    objectGenerators.push_back(new type_treasure_def(16, 1, 2000, 100));
    objectGenerators.push_back(new type_treasure_def(16, 2, 2000, 100));
    objectGenerators.push_back(new type_treasure_def(16, 3, 5000, 100));
    objectGenerators.push_back(new type_treasure_def(16, 4, 1500, 100));
    objectGenerators.push_back(new type_treasure_def(16, 5, 3000, 100));
    objectGenerators.push_back(new type_treasure_def(16, 6, 9000, 100));

    int dwelling = 80;
    if (mapVersion < 1)
        dwelling = 58;
    for (; dwelling--;)
        objectGenerators.push_back(new type_map_dwelling_def(dwelling));

    objectGenerators.push_back(new type_treasure_def(22, 0, 500, 100));
    objectGenerators.push_back(new type_treasure_def(23, 0, 1500, 100));
    objectGenerators.push_back(new type_treasure_def(24, 0, 4000, 20));
    objectGenerators.push_back(new type_treasure_def(25, 0, 10000, 100));
    objectGenerators.push_back(new type_treasure_def(28, 0, 100, 100));
    objectGenerators.push_back(new type_treasure_def(29, 0, 500, 1000));
    objectGenerators.push_back(new type_treasure_def(30, 0, 100, 100));
    objectGenerators.push_back(new type_treasure_def(31, 0, 100, 50));
    objectGenerators.push_back(new type_treasure_def(32, 0, 1500, 100));
    objectGenerators.push_back(new type_treasure_def(35, 0, 7000, 20));
    objectGenerators.push_back(new type_treasure_def(38, 0, 100, 100));
    objectGenerators.push_back(new type_treasure_def(39, 0, 500, 100));
    objectGenerators.push_back(new type_treasure_def(41, 0, 12000, 20));
    objectGenerators.push_back(new type_treasure_def(47, 0, 1000, 50));
    objectGenerators.push_back(new type_treasure_def(48, 0, 500, 50));
    objectGenerators.push_back(new type_treasure_def(49, 0, 250, 100));
    objectGenerators.push_back(new type_treasure_def(51, 0, 1500, 100));
    objectGenerators.push_back(new type_treasure_def(52, 0, 100, 100));
    objectGenerators.push_back(new type_treasure_def(55, 0, 500, 50));
    objectGenerators.push_back(new type_treasure_def(56, 0, 100, 50));
    objectGenerators.push_back(new type_treasure_def(57, 0, 3500, 200));
    objectGenerators.push_back(new type_treasure_def(58, 0, 750, 100));
    objectGenerators.push_back(new type_treasure_def(60, 0, 750, 100));
    objectGenerators.push_back(new type_treasure_def(61, 0, 1500, 100));

    objectGenerators.push_back(new type_prison_def(2500, 0));
    objectGenerators.push_back(new type_prison_def(5000, 5000));
    objectGenerators.push_back(new type_prison_def(10000, 15000));
    objectGenerators.push_back(new type_prison_def(20000, 90000));
    objectGenerators.push_back(new type_prison_def(30000, 500000));
    objectGenerators.push_back(new type_treasure_def(63, 0, 5000, 20));
    objectGenerators.push_back(new type_treasure_def(64, 0, 100, 100));

    objectGenerators.push_back(new type_artifact_def(66, 2000));
    objectGenerators.push_back(new type_artifact_def(67, 5000));
    objectGenerators.push_back(new type_artifact_def(68, 10000));
    objectGenerators.push_back(new type_artifact_def(69, 20000));

    objectGenerators.push_back(new type_resource_lump_def(76, 0, 1500, 2000));
    objectGenerators.push_back(new type_treasure_def(78, 0, 5000, 20));
    objectGenerators.push_back(new type_resource_lump_def(79, 0, 1400, 300));
    objectGenerators.push_back(new type_resource_lump_def(79, 2, 1400, 300));
    objectGenerators.push_back(new type_resource_lump_def(79, 1, 2000, 300));
    objectGenerators.push_back(new type_resource_lump_def(79, 3, 2000, 300));
    objectGenerators.push_back(new type_resource_lump_def(79, 4, 2000, 300));
    objectGenerators.push_back(new type_resource_lump_def(79, 5, 2000, 300));
    objectGenerators.push_back(new type_resource_lump_def(79, 6, 750, 300));
    objectGenerators.push_back(new type_treasure_def(80, 0, 100, 50));
    objectGenerators.push_back(new type_scholar_def());
    objectGenerators.push_back(new type_treasure_def(82, 0, 1500, 500));

    for (int quest = 0; quest < objectPrototypes[83].size(); ++quest) {
        int creatureCount = mapVersion >= 1 ? 145 : 118;
        for (int creature = creatureCount; creature--;) {
            if (akCreatureTypeTraits[creature].level >= 0)
                objectGenerators.push_back(
                    new type_quest_creature_def(creature, quest));
        }

        objectGenerators.push_back(
            new type_quest_experience_def(quest, 2000, 5000));
        objectGenerators.push_back(
            new type_quest_experience_def(quest, 5333, 10000));
        objectGenerators.push_back(
            new type_quest_experience_def(quest, 8666, 15000));
        objectGenerators.push_back(
            new type_quest_experience_def(quest, 12000, 20000));
        objectGenerators.push_back(new type_quest_gold_def(quest, 2000, 5000));
        objectGenerators.push_back(new type_quest_gold_def(quest, 5333, 10000));
        objectGenerators.push_back(new type_quest_gold_def(quest, 8666, 15000));
        objectGenerators.push_back(new type_quest_gold_def(quest, 12000, 20000));
    }

    objectGenerators.push_back(new type_treasure_def(84, 0, 1000, 100));
    objectGenerators.push_back(new type_treasure_def(85, 0, 2000, 100));
    objectGenerators.push_back(new type_treasure_def(86, 0, 1500, 50));
    objectGenerators.push_back(new type_shrine_def(88, 500));
    objectGenerators.push_back(new type_shrine_def(89, 2000));
    objectGenerators.push_back(new type_shrine_def(90, 3000));
    objectGenerators.push_back(new type_treasure_def(92, 0, 100, 20));
    objectGenerators.push_back(new type_spell_scroll_def(1, 500));
    objectGenerators.push_back(new type_spell_scroll_def(2, 2000));
    objectGenerators.push_back(new type_spell_scroll_def(3, 3000));
    objectGenerators.push_back(new type_spell_scroll_def(4, 4000));
    objectGenerators.push_back(new type_spell_scroll_def(5, 5000));
    objectGenerators.push_back(new type_treasure_def(94, 0, 200, 40));
    objectGenerators.push_back(new type_treasure_def(95, 0, 100, 20));
    objectGenerators.push_back(new type_treasure_def(96, 0, 100, 100));
    objectGenerators.push_back(new type_treasure_def(97, 0, 100, 100));
    objectGenerators.push_back(new type_treasure_def(99, 0, 100, 100));
    objectGenerators.push_back(new type_treasure_def(100, 0, 1500, 200));
    objectGenerators.push_back(new type_treasure_def(101, 0, 1500, 1000));
    objectGenerators.push_back(new type_treasure_def(102, 0, 2500, 50));
    objectGenerators.push_back(new type_treasure_def(104, 0, 2500, 20));
    objectGenerators.push_back(new type_treasure_def(105, 0, 500, 50));
    objectGenerators.push_back(new type_treasure_def(106, 0, 1500, 50));
    objectGenerators.push_back(new type_treasure_def(107, 0, 1000, 50));
    objectGenerators.push_back(new type_treasure_def(108, 0, 6000, 20));
    objectGenerators.push_back(new type_treasure_def(109, 0, 750, 50));
    objectGenerators.push_back(new type_treasure_def(110, 0, 500, 50));
    objectGenerators.push_back(new type_treasure_def(112, 0, 2500, 150));
    objectGenerators.push_back(new type_witch_hut_def());
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
unsigned char type_random_map_generator::CanPlaceZone(TRmgZone* zone)
{
    TRmgTownSlot* slot = zone->slot;
    TRmgMapPosition position = zone->GetLevelPosition();
    int size = slot->size;
    if ((slot->kind == RMG_TEMPLATE_HUMAN ||
         slot->kind == RMG_TEMPLATE_COMPUTER) &&
        position.z == 1 && zone->alignment != TOWN_INFERNO &&
        zone->alignment != TOWN_NECROPOLIS && zone->alignment != TOWN_DUNGEON)
        return 0;
    int zoneIndex = slot->zoneIndex;
    for (int other = 0; other < zones.size(); ++other) {
        TRmgZone* otherZone = zones[other];
        if (otherZone->GetLevelPosition().z != position.z ||
            otherZone->slot->zoneIndex == zoneIndex)
            continue;
        TRmgMapPosition otherPosition = otherZone->GetLevelPosition();
        int dy = otherPosition.y - position.y;
        int dx = otherPosition.x - position.x;
        int distance = static_cast<int>(sqrt(static_cast<double>(dx * dx + dy * dy)));
        if (10 * distance < 8 * (otherZone->slot->size + size))
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
int type_random_map_generator::CountPlacedZoneConnections(TRmgZone* zone) const
{
    int result = 0;
    TRmgTownSlot* slot = zone->slot;
    for (int connection = 0; connection < slot->connections.size(); ++connection) {
        int destination = slot->connections[connection].destination->zoneIndex;
        if (destination < zones.size() && zones[destination]->CanConnect(zone))
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
void type_random_map_generator::FilterZonePositions(
    TRmgZone* zone, std::vector<TRmgMapPosition>& candidates, int mapSize)
{
    int bestConnections = 0;
    if (map.numberLevels > 1) {
        unsigned char occupiedLevels[2] = {0, 0};
        for (int other = 0; other < zones.size(); ++other) {
            if (zones[other] != zone)
                occupiedLevels[zones[other]->GetLevelPosition().z] = 1;
        }
        if (!occupiedLevels[0] || !occupiedLevels[1]) {
            int candidate = candidates.size();
            while (candidate--) {
                if (!occupiedLevels[candidates[candidate].z])
                    break;
            }
            if (candidate > 0) {
                candidate = candidates.size();
                while (candidate--) {
                    if (occupiedLevels[candidates[candidate].z])
                        candidates.erase(candidates.begin() + candidate);
                }
            }
        }
    }

    for (int candidate = 0; candidate < candidates.size(); ++candidate) {
        zone->SetLevelPosition(candidates[candidate]);
        int connections = CountPlacedZoneConnections(zone);
        if (connections > bestConnections)
            bestConnections = connections;
    }
    for (candidate = candidates.size() - 1; candidate >= 0; --candidate) {
        zone->SetLevelPosition(candidates[candidate]);
        if (CountPlacedZoneConnections(zone) < bestConnections)
            candidates.erase(candidates.begin() + candidate);
    }

    int bestSize = 32000;
    int minimumY = 0;
    int minimumX = 0;
    int maximumY = 0;
    int maximumX = 0;
    for (int other = 0; other < zones.size(); ++other) {
        if (zones[other] != zone) {
            TRmgMapPosition position = zones[other]->GetLevelPosition();
            int size = zones[other]->slot->size;
            minimumY = min(minimumY, position.y - size);
            minimumX = min(minimumX, position.x - size);
            maximumY = max(maximumY, position.y + size + 1);
            maximumX = max(maximumX, position.x + size + 1);
        }
    }
    int size = zone->slot->size;
    for (candidate = 0; candidate < candidates.size(); ++candidate) {
        int candidateMinimumY = min(minimumY, candidates[candidate].y - size);
        int candidateMinimumX = min(minimumX, candidates[candidate].x - size);
        int candidateMaximumY = max(maximumY, candidates[candidate].y + size + 1);
        int candidateMaximumX = max(maximumX, candidates[candidate].x + size + 1);
        int candidateSize = max(mapSize, candidateMaximumY - candidateMinimumY);
        candidateSize = max(candidateSize, candidateMaximumX - candidateMinimumX);
        bestSize = min(bestSize, candidateSize);
    }
    for (candidate = candidates.size() - 1; candidate >= 0; --candidate) {
        int candidateMinimumY = min(minimumY, candidates[candidate].y - size);
        int candidateMinimumX = min(minimumX, candidates[candidate].x - size);
        int candidateMaximumY = max(maximumY, candidates[candidate].y + size + 1);
        int candidateMaximumX = max(maximumX, candidates[candidate].x + size + 1);
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
void type_random_map_generator::DrawIrregularZoneBoundary(
    TPoint from, TPoint to, int zoneIndex, int level, int roughness)
{
    std::vector<TPoint> pending;
    unsigned char markBoundary = level == 1 || waterContent != RMG_WATER_ISLANDS;
    pending.push_back(to);
    while (pending.size() > 0) {
        to = pending.back();
        pending.pop_back();
        TPoint midpoint((from.x + to.x + 1) / 2, (from.y + to.y + 1) / 2);
        if (midpoint != from && midpoint != to) {
            TRmgVector perpendicular;
            {
                TRmgVector delta = to - from;
                perpendicular = TRmgVector(-delta.y, delta.x);
            }
            int length = perpendicular.Length();
            if (length > 1) {
                int limit = std::_cpp_min<long>(length, roughness);
                int displacement = rand() % limit - limit / 2;
                perpendicular = perpendicular * displacement / length;
                midpoint += perpendicular;
            }
            pending.push_back(to);
            pending.push_back(midpoint);
        } else {
            long x = std::_cpp_max<long>(from.x, 0);
            x = std::_cpp_min<long>(x, map.mapWidth - 1);
            long y = std::_cpp_max<long>(from.y, 0);
            y = std::_cpp_min<long>(y, map.mapHeight - 1);
            TRmgMapItem* item = map.GetMapItem(x, y, level);
            item->zoneState.zone = zoneIndex;
            if (markBoundary)
                item->tileData.zoneBoundary = 1;
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
void type_random_map_generator::DrawStraightZoneBoundary(
    TPoint from, TPoint to, int zoneIndex, int level)
{
    if (from.x > to.x)
        std::swap(from, to);
    int dx = to.x - from.x;
    int dy = to.y - from.y;
    int verticalDistance = abs(dy);
    int major;
    int minor;
    TPoint straight;
    TPoint diagonal;
    if (dx > verticalDistance) {
        major = dx;
        minor = verticalDistance;
        straight.x = 1;
        straight.y = 0;
        diagonal.y = dy > 0 ? 1 : -1;
    } else {
        major = verticalDistance;
        minor = dx;
        straight = TPoint(0, dy > 0 ? 1 : -1);
        diagonal = straight;
    }
    diagonal.x = 1;
    unsigned char markBoundary = level == 1 || waterContent != RMG_WATER_ISLANDS;
    int error = major / 2;
    while (from.x != to.x || from.y != to.y) {
        TRmgMapItem* item = map.GetMapItem(from.x, from.y, level);
        item->zoneState.zone = zoneIndex;
        if (markBoundary)
            item->tileData.zoneBoundary = 1;
        error += minor;
        if (error < major) {
            from.x += straight.x;
            from.y += straight.y;
        } else {
            error -= major;
            from.x += diagonal.x;
            from.y += diagonal.y;
        }
    }
    TRmgMapItem* lastItem = map.GetMapItem(from.x, from.y, level);
    lastItem->zoneState.zone = zoneIndex;
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
void type_random_map_generator::TraceZoneBoundary(
    TRmgBoundaryVertex* first, unsigned char irregular)
{
    TRmgBoundaryVertex* vertex = first;
    TRmgZone* zone = vertex->zone;
    int zoneIndex = zone->slot->zoneIndex;
    TRmgMapPosition zonePosition = zone->levelPosition;
    TRmgZoneBounds bounds;
    bounds.maximumY = map.mapHeight;
    bounds.minimumX = bounds.minimumY = 0;
    bounds.maximumX = map.mapWidth;
    TRmgBoundaryVertex* next;
    TPoint originalFrom;
    TPoint originalTo;
    TPoint from;
    TPoint to;

    bool found = false;
    do {
        next = vertex->next;
        originalFrom = vertex->position;
        originalTo = next->position;
        from = ClipRmgBoundaryPoint(bounds, vertex->position, next->position);
        to = ClipRmgBoundaryPoint(bounds, originalTo, originalFrom);
        if (bounds.Contains(from) && from != to) {
            found = true;
            break;
        }
        vertex = next;
    } while (vertex != first);
    if (!found) {
        TPoint upperLeft(bounds.minimumX, bounds.minimumY);
        TPoint upperRight(bounds.maximumX - 1, bounds.minimumY);
        TPoint lowerLeft(bounds.minimumX, bounds.maximumY - 1);
        TPoint lowerRight(bounds.maximumX - 1, bounds.maximumY - 1);
        DrawStraightZoneBoundary(lowerRight, upperRight, zoneIndex, zonePosition.z);
        DrawStraightZoneBoundary(upperRight, upperLeft, zoneIndex, zonePosition.z);
        DrawStraightZoneBoundary(upperLeft, lowerLeft, zoneIndex, zonePosition.z);
        DrawStraightZoneBoundary(lowerLeft, lowerRight, zoneIndex, zonePosition.z);
        zone->boundary.push_back(TPoint(lowerRight));
        zone->boundary.push_back(TPoint(upperRight));
        zone->boundary.push_back(TPoint(upperLeft));
        zone->boundary.push_back(TPoint(lowerLeft));
        return;
    }

    first = vertex;
    do {
        next = vertex->next;
        TRmgZone* neighbour = next->twin->zone;
        originalFrom = vertex->position;
        originalTo = next->position;
        from = ClipRmgBoundaryPoint(bounds, vertex->position, next->position);
        to = ClipRmgBoundaryPoint(bounds, originalTo, originalFrom);
        zone->boundary.push_back(TPoint(from));

        if (!neighbour || neighbour->slot->zoneIndex > zoneIndex) {
            int roughness = zone->boundaryRoughness;
            if (neighbour) {
                int ownRoughness = roughness;
                int neighbourRoughness = neighbour->boundaryRoughness;
                roughness = std::_cpp_min(ownRoughness, neighbourRoughness);
            }
            if (irregular)
                DrawIrregularZoneBoundary(from, to, zoneIndex, zonePosition.z, roughness);
            else
                DrawStraightZoneBoundary(from, to, zoneIndex, zonePosition.z);
        }

        vertex = next;
        if (to != originalTo) {
            from = to;
            for (;;) {
                next = next->next;
                to = ClipRmgBoundaryPoint(bounds, vertex->position, next->position);
                if (bounds.Contains(to))
                    break;
                vertex = next;
            }
            while (from.x != to.x && from.y != to.y) {
                TPoint corner;
                if (from.x == bounds.minimumX && from.y != bounds.minimumY)
                    corner = TPoint(bounds.minimumX, bounds.minimumY);
                else if (from.y == bounds.minimumY && from.x != bounds.maximumX - 1)
                    corner = TPoint(bounds.maximumX - 1, bounds.minimumY);
                else if (from.x == bounds.maximumX - 1 && from.y != bounds.maximumY - 1)
                    corner = TPoint(bounds.maximumX - 1, bounds.maximumY - 1);
                else
                    corner = TPoint(bounds.minimumX, bounds.maximumY - 1);
                DrawStraightZoneBoundary(from, corner, zoneIndex, zonePosition.z);
                zone->boundary.push_back(TPoint(from));
                from = corner;
            }
            DrawStraightZoneBoundary(from, to, zoneIndex, zonePosition.z);
            zone->boundary.push_back(TPoint(from));
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
TPoint ClipRmgBoundaryPoint(
    const TRmgZoneBounds& bounds, TPoint point, TPoint toward)
{
    if (bounds.Contains(point))
        return point;

    TRmgVector delta = toward - point;
    TPoint clipped = point;
    if (clipped.x < bounds.minimumX && delta.x) {
        clipped = clipped + delta * (bounds.minimumX - clipped.x) / delta.x;
        if (point.y >= bounds.minimumY && clipped.y < bounds.minimumY)
            return point;
        if (point.y < bounds.maximumY && clipped.y >= bounds.maximumY)
            return point;
    }
    if (clipped.y < bounds.minimumY && delta.y) {
        clipped = clipped + delta * (bounds.minimumY - clipped.y) / delta.y;
        if (point.x >= bounds.minimumX && clipped.x < bounds.minimumX)
            return point;
        if (point.x < bounds.maximumX && clipped.x >= bounds.maximumX)
            return point;
    }
    if (clipped.x >= bounds.maximumX && delta.x) {
        clipped = clipped + delta * (bounds.maximumX - clipped.x - 1) / delta.x;
        if (point.y >= bounds.minimumY && clipped.y < bounds.minimumY)
            return point;
        if (point.y < bounds.maximumY && clipped.y >= bounds.maximumY)
            return point;
    }
    if (clipped.y >= bounds.maximumY && delta.y) {
        clipped = clipped + delta * (bounds.maximumY - clipped.y - 1) / delta.y;
        if (point.x >= bounds.minimumX && clipped.x < bounds.minimumX)
            return point;
        if (point.x < bounds.maximumX && clipped.x >= bounds.maximumX)
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
void type_random_map_generator::BuildZoneBoundaries(
    TRmgTemplate* mapTemplate, int level)
{
    TRmgVoronoi diagram;
    for (int zone = 0; zone < zones.size(); ++zone) {
        if (zones[zone]->GetLevelPosition().z == level) {
            TRmgMapPosition position = zones[zone]->GetLevelPosition();
            diagram.AddSite(TPoint(position.x, position.y), zones[zone]);
        }
    }
    int originalZones = zones.size();
    if (level == 1 || waterContent != RMG_WATER_NONE) {
        TRmgTownSlot testSlot;
        testSlot.zoneIndex = -1;
        testSlot.kind = RMG_TEMPLATE_JUNCTION;
        testSlot.size = 0;
        TRmgZone testZone(&testSlot);
        TRmgZone* addedZone = 0;
        for (int zone = 0; zone < originalZones; ++zone) {
            TRmgZone* current = zones[zone];
            if (current->GetLevelPosition().z != level)
                continue;
            int radius = current->boundaryRoughness;
            testSlot.size = radius;
            TRmgMapPosition position = current->GetLevelPosition();
            for (int direction = 0; direction < 32; direction += 4) {
                TRmgMapPosition horizontalCenter = current->GetLevelPosition();
                double dx = radius * gRmgDirectionCosines[direction];
                position.x = static_cast<int>(horizontalCenter.x + dx * 2);
                TRmgMapPosition verticalCenter = current->GetLevelPosition();
                double dy = radius * gRmgDirectionSines[direction];
                position.y = static_cast<int>(verticalCenter.y + dy * 2);
                if (position.x < 0 && position.x < dx)
                    continue;
                if (position.x >= map.mapWidth) {
                    if (position.x >= map.mapWidth + dx)
                        continue;
                }
                if (position.y < 0 && position.y < dy)
                    continue;
                if (position.y >= map.mapHeight) {
                    if (position.y >= map.mapHeight + dy)
                        continue;
                }
                testZone.SetLevelPosition(position);
                if (!CanPlaceZone(&testZone))
                    continue;
                if (position.z == 0) {
                    TRmgTownSlot* slot = new TRmgTownSlot;
                    slot->zoneIndex = mapTemplate->zones.size();
                    slot->size = radius;
                    memset(slot->allowedMonsters, 0, sizeof(slot->allowedMonsters));
                    memset(slot->allowedTerrain, 0, sizeof(slot->allowedTerrain));
                    memset(slot->parameters004c, 0, sizeof(slot->parameters004c));
                    memset(slot->parameters0068, 0, sizeof(slot->parameters0068));
                    slot->parameters0020[0] = 0;
                    slot->parameters0020[1] = 0;
                    slot->parameters0020[2] = 0;
                    slot->parameters0020[3] = 0;
                    slot->parameters0020[4] = 0;
                    slot->parameters0020[5] = 0;
                    slot->parameters0020[6] = 0;
                    slot->parameters0020[7] = 0;
                    slot->monsterStrength = 0;
                    slot->playerIndex = -1;
                    memset(slot->treasure, 0, sizeof(slot->treasure));
                    slot->treasure[0].density = 5;
                    slot->treasure[0].maximum = 1000;
                    slot->treasure[0].minimum = 100;
                    slot->treasure[1].density = 1;
                    slot->treasure[1].maximum = 6000;
                    slot->treasure[1].minimum = 2000;
                    slot->kind = RMG_TEMPLATE_JUNCTION;
                    addedZone = new TRmgZone(slot);
                    addedZone->terrain = eTerrainWater;
                    addedZone->SetLevelPosition(position);
                    mapTemplate->zones.push_back(slot);
                    zones.push_back(addedZone);
                }
                diagram.AddSite(TPoint(position.x, position.y), addedZone);
            }
        }
    }
    diagram.BuildVertices();
    for (zone = 0; zone < zones.size(); ++zone) {
        if (zones[zone]->GetLevelPosition().z == level) {
            TRmgMapPosition position = zones[zone]->GetLevelPosition();
            TRmgBoundaryVertex* first = diagram.Locate(TPoint(position.x, position.y));
            TraceZoneBoundary(first,
                zone < originalZones && (waterContent != RMG_WATER_ISLANDS || level == 1));
        }
    }
    for (zone = 0; zone < zones.size(); ++zone) {
        TRmgZone* current = zones[zone];
        if (current->GetLevelPosition().z == level) {
            TRmgMapPosition position = current->GetLevelPosition();
            FillZoneArea(current, diagram.Locate(TPoint(position.x, position.y)));
        }
    }
    JoinExtraZones(originalZones, &diagram);
}

// The inlined search at 0x53fe7a returns an element pointer and its caller
// then tests that pointer, even on the found arm. Preserve that ordinary
// helper boundary rather than reducing the search to a boolean.
TRmgZoneConnection* TRmgTownSlot::FindConnection(int destinationZone)
{
    for (unsigned int i = 0; i < connections.size(); ++i) {
        if (connections[i].destination->zoneIndex == destinationZone)
            return &connections[i];
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
void type_random_map_generator::RepairWaterZoneBorders()
{
    TRmgMapItem* current = map.mapItems;
    TTerrainType terrain;
    TRmgMapPosition position;
    TRmgMapPosition nearby;
    std::vector<TRmgMapPosition> positions;
    std::vector<TTerrainType> terrains;
    for (position.z = 0; position.z < map.numberLevels; ++position.z) {
        for (position.y = 0; position.y < map.mapHeight; ++position.y) {
            for (position.x = 0; position.x < map.mapWidth; ++position.x, ++current) {
                int zoneIndex = current->zoneState.zone;
                if (zoneIndex < 0 || current->tile.landType != eTerrainWater)
                    continue;
                int destinationZone = current->zoneState.connectionEligibility;
                if (destinationZone < 0)
                    continue;

                unsigned char found = 0;
                TRmgZoneBounds bounds;
                {
                    int row = position.y;
                    TPoint lower(position.x - 1, row - 1);
                    bounds.minimumY = max(lower.y, 0);
                    bounds.minimumX = max(lower.x, 0);
                    int height = map.mapHeight;
                    bounds.maximumY = min(row + 2, height);
                    int width = map.mapWidth;
                    bounds.maximumX = min(position.x + 2, width);
                }
                nearby.z = position.z;
                TRmgZone* zone = zones[zoneIndex];
                for (nearby.y = bounds.minimumY;
                     nearby.y < bounds.maximumY && !found; ++nearby.y) {
                    nearby.x = bounds.minimumX;
                    if (nearby.x < bounds.maximumX) {
                        do {
                            TRmgMapItem* item = map.GetMapItem(nearby);
                            if (item->tile.landType != eTerrainWater
                                && item->tile.landType != eTerrainRock
                                && !item->HasBorderObject()
                                && item->tileData.roadPassable) {
                                terrain = item->tile.landType;
                                found = 1;
                                break;
                            }
                            ++nearby.x;
                            if (nearby.x >= bounds.maximumX)
                                break;
                        } while (1);
                    }
                }
                if (!found || zone->slot->FindConnection(destinationZone))
                    continue;

                {
                    int row = position.y;
                    TPoint lower(position.x - 1, row - 1);
                    bounds.minimumY = max(lower.y, 0);
                    bounds.minimumX = max(lower.x, 0);
                    int height = map.mapHeight;
                    bounds.maximumY = min(row + 2, height);
                    int width = map.mapWidth;
                    bounds.maximumX = min(position.x + 2, width);
                }
                for (nearby.y = bounds.minimumY; nearby.y < bounds.maximumY; ++nearby.y) {
                    for (nearby.x = bounds.minimumX; nearby.x < bounds.maximumX; ++nearby.x) {
                        TRmgMapItem* item = map.GetMapItem(nearby);
                        if (!item->connection.present) {
                            item->tileData.subterraneanGate = 0;
                            item->tileData.borderObject = 1;
                        }
                        if (item->tile.landType == eTerrainWater) {
                            positions.push_back(nearby);
                            terrains.push_back(terrain);
                        }
                    }
                }

                {
                    int row = position.y;
                    TPoint lower(position.x - 2, row - 2);
                    bounds.minimumY = max(lower.y, 0);
                    bounds.minimumX = max(lower.x, 0);
                    int height = map.mapHeight;
                    bounds.maximumY = min(row + 3, height);
                    int width = map.mapWidth;
                    bounds.maximumX = min(position.x + 3, width);
                }
                for (nearby.y = bounds.minimumY; nearby.y < bounds.maximumY; ++nearby.y) {
                    for (nearby.x = bounds.minimumX; nearby.x < bounds.maximumX; ++nearby.x) {
                        TRmgMapItem* item = map.GetMapItem(nearby);
                        if (static_cast<int>(item->objects.size()) <= 0
                            && !item->connection.present)
                            item->tileData.subterraneanGate = 0;
                    }
                }
            }
            if (progress)
                progress->Advance(20);
        }
        if (positions.size()) {
            TTerrainType lastTerrain = terrains[0];
            type_random_map levelMap(map.GetMapItem(0, 0, position.z),
                map.mapWidth, map.mapHeight);
            TRmgTerrainBrush brush(&levelMap, lastTerrain, 4);
            for (unsigned int i = 0; i < positions.size(); ++i) {
                terrain = terrains[i];
                if (terrain != lastTerrain) {
                    brush.ChangeTerrain(terrain, 4);
                    lastTerrain = terrain;
                }
                position = positions[i];
                brush.PaintRectangle(position.x, position.y, 1, 1);
            }
            positions.clear();
            terrains.clear();
        }
    }
}

// Provisional arithmetic boundary for the Complete-only position value.
// CreateRiver keeps position.x in EBX and jumps over its backedge reload.
// A by-value direction and returned coordinate construction recover that
// sequence; a reference operand leaves a different relaxation register flow.
// Keep this ordinary body visible to the direction-addition call sites.
TRmgMapPosition TRmgMapPosition::operator+(TPoint offset) const
{
    return TRmgMapPosition(x + offset.x, y + offset.y, z);
}

// Complete-only ground connection pass.  ConnectZones passes the paired
// boundary item/position vectors.  Retail selects all equally cheap empty
// crossings, opens their predecessor paths, and records both zone entrances
// before choosing border objects or a guard.  There is no Dreamcast RMG
// counterpart; the helper names describe their retained retail bodies.
// Residual (77.31306%): the two final GetMapItem calls and the final vector
// destructor expand. Delegating the by-value accessor to its scalar overload
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
unsigned char type_random_map_generator::CreateGroundConnection(
    TRmgZone* source,
    TRmgZoneConnection* connection,
    std::vector<TRmgMapItem*>* borderItems,
    std::vector<TRmgMapPosition>* borderPositions)
{
    TRmgZone* destination = zones[connection->destination->zoneIndex];
    int sourceZone = source->slot->zoneIndex;
    int destinationZone = destination->slot->zoneIndex;
    if (source->GetLevelPosition().z != destination->GetLevelPosition().z)
        return 0;
    if (source->terrain == eTerrainWater)
        return 0;
    if (destination->terrain == eTerrainWater)
        return 0;

    std::vector<TRmgMapPosition> candidates;
    int eligibleCount = 0;
    int bestCost = 100;
    for (int index = 0; index < borderItems->size(); ++index) {
        TRmgMapItem* item = (*borderItems)[index];
        if (item->zoneState.zone == sourceZone
            && item->zoneState.connectionEligibility == destinationZone
            && static_cast<int>(item->objects.size()) <= 0) {
            TRmgMapPosition other = (*borderPositions)[index]
                + g_rmgDirections[item->tileData.connectionDirection];
            if (static_cast<int>(map.GetMapItem(other)->objects.size()) <= 0) {
                ++eligibleCount;
                int cost = item->movement.unknown;
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
    if (connection->unguarded) {
        guardValue = 0;
    } else {
        guardValue = GetRmgGuardValue(connection->value, monsterStrength);
    }

    if (bestCost == 1 && guardValue == 0 && !connection->placeBorderObjects)
        return 1;

    int count = min(candidates.size(), (eligibleCount + 39) / 40);
    for (int crossing = 0; crossing < count; ++crossing) {
        int selected = rand() % candidates.size();
        TRmgMapPosition position = candidates[selected];
        TPoint direction = g_rmgDirections[
            map.GetMapItem(position)->tileData.connectionDirection];
        TRmgMapPosition otherPosition = candidates[selected] + direction;

        OpenConnectionPath(candidates[selected], connection->placeBorderObjects);
        source->entrances.push_back(TPoint(position.x, position.y));
        OpenConnectionPath(otherPosition, connection->placeBorderObjects);
        destination->entrances.push_back(TPoint(otherPosition.x, otherPosition.y));
        candidates.erase(candidates.begin() + selected);

        if (connection->placeBorderObjects) {
            int borderDirection = PlaceBorderObject(position, 1, destination);
            if (borderDirection >= 0) {
                MarkBorderObjectArea(position, borderDirection);
                guardValue = 0;
            }
            borderDirection = PlaceBorderObject(otherPosition, 1, source);
            if (borderDirection >= 0) {
                MarkBorderObjectArea(otherPosition, borderDirection);
                guardValue = 0;
            }
        }

        if (guardValue > 0) {
            if (!(rand() & 1)) {
                TRmgMapItem* item = map.GetMapItem(position);
                TRmgZone* zone = zones[item->zoneState.zone];
                if (static_cast<int>(item->objects.size()) <= 0) {
                    type_object* guard = CreateGuard(guardValue, zone);
                    if (guard)
                        AddObject(guard, position);
                }
            } else {
                TRmgMapItem* item = map.GetMapItem(otherPosition);
                TRmgZone* zone = zones[item->zoneState.zone];
                if (static_cast<int>(item->objects.size()) <= 0) {
                    type_object* guard = CreateGuard(guardValue, zone);
                    if (guard)
                        AddObject(guard, otherPosition);
                }
            }
        }
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
unsigned char type_random_map_generator::CreateSubterraneanGate(
    TRmgZone* source, TRmgZoneConnection* connection)
{
    TRmgZone* destination = zones[connection->destination->zoneIndex];
    int sourceZone = source->slot->zoneIndex;
    int destinationZone = destination->slot->zoneIndex;
    TRmgMapPosition sourceLevelPosition = source->levelPosition;
    TRmgMapPosition destinationLevelPosition = destination->levelPosition;

    if (sourceLevelPosition.z == destinationLevelPosition.z)
        return 0;
    if (source->terrain == eTerrainWater)
        return 0;

    TRmgZoneBounds sourceBounds = source->bounds;
    TRmgZoneBounds destinationBounds = destination->bounds;
    int minimumX = std::_cpp_max(
        sourceBounds.minimumX, destinationBounds.minimumX);
    int minimumY = std::_cpp_max(
        sourceBounds.minimumY, destinationBounds.minimumY);
    int maximumX = std::_cpp_min(
        sourceBounds.maximumX, destinationBounds.maximumX);
    int maximumY = std::_cpp_min(
        sourceBounds.maximumY, destinationBounds.maximumY);
    if (minimumX >= maximumX || minimumY >= maximumY)
        return 0;

    int gateIndex = rand() % objectPrototypes[103].size();
    TRmgObjectPropertiesRef* gateProperties = objectPrototypes[103][gateIndex];
    TObjectType* gatePrototype = gateProperties->prototype;

    std::vector<TRmgMapPosition> candidates;
    int bestScore = 0;
    TRmgMapPosition position = sourceLevelPosition;

    for (position.y = minimumY; position.y < maximumY; ++position.y) {
        for (position.x = minimumX; position.x < maximumX; ++position.x) {
            TRmgMapItem* sourceItem = map.GetMapItem(position);
            int score = sourceItem->zoneState.score;
            if (sourceItem->zoneState.zone != sourceZone)
                continue;

            TRmgMapPosition otherPosition = destination->levelPosition;
            otherPosition.x = position.x;
            otherPosition.y = position.y;
            TRmgMapItem* destinationItem = map.GetMapItem(otherPosition);
            if (destinationItem->zoneState.zone != destinationZone)
                continue;

            score += destinationItem->zoneState.score;
            if (score < bestScore)
                continue;
            if (!map.CanPlaceObject(gateProperties, position, source))
                continue;
            if (!map.CanPlaceObject(
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
    AddObject(new type_object(gateProperties), position);

    TRmgMapPosition otherPosition = destination->levelPosition;
    otherPosition.x = position.x;
    otherPosition.y = position.y;
    AddObject(new type_object(gateProperties), otherPosition);

    position.x -= gatePrototype->triggerCell.x;
    position.y -= gatePrototype->triggerCell.y;
    otherPosition = destination->levelPosition;
    otherPosition.x = position.x;
    otherPosition.y = position.y;
    TPoint entrance(position.x, position.y);
    source->entrances.push_back(entrance);
    destination->entrances.push_back(entrance);

    int guardValue;
    if (connection->unguarded) {
        guardValue = 0;
    } else {
        guardValue = GetRmgGuardValue(connection->value, monsterStrength);
    }

    ++position.y;
    ++otherPosition.y;
    TRmgMapItem* sourceEntrance = map.GetMapItem(position);
    if (!sourceEntrance->connection.present) {
        sourceEntrance->tileData.borderObject = 0;
        sourceEntrance->tileData.subterraneanGate = 1;
    }
    TRmgMapItem* destinationEntrance = map.GetMapItem(otherPosition);
    if (!destinationEntrance->connection.present) {
        destinationEntrance->tileData.borderObject = 0;
        destinationEntrance->tileData.subterraneanGate = 1;
    }

    if (connection->placeBorderObjects) {
        int direction = PlaceBorderObject(position, 1, destination);
        if (direction >= 0) {
            --position.x;
            guardValue = 0;
            TRmgMapItem* item = map.GetMapItem(position);
            if (item->objects.size() == 0) {
                if (!item->connection.present) {
                    item->tileData.subterraneanGate = 0;
                    item->tileData.borderObject = 1;
                }
                item->connection.direction = direction;
                item->connection.present = 1;
            }

            position.x += 2;
            item = map.GetMapItem(position);
            if (item->objects.size() == 0) {
                if (!item->connection.present) {
                    item->tileData.subterraneanGate = 0;
                    item->tileData.borderObject = 1;
                }
                item->connection.direction = direction;
                item->connection.present = 1;
            }
        }

        direction = PlaceBorderObject(otherPosition, 1, source);
        if (direction >= 0) {
            --otherPosition.x;
            TRmgMapItem* item = map.GetMapItem(otherPosition);
            if (item->objects.size() == 0) {
                if (!item->connection.present) {
                    item->tileData.subterraneanGate = 0;
                    item->tileData.borderObject = 1;
                }
                item->connection.direction = direction;
                item->connection.present = 1;
            }

            otherPosition.x += 2;
            item = map.GetMapItem(otherPosition);
            if (item->objects.size() == 0) {
                if (!item->connection.present) {
                    item->tileData.subterraneanGate = 0;
                    item->tileData.borderObject = 1;
                }
                item->connection.direction = direction;
                item->connection.present = 1;
            }
            return 1;
        }
    }

    if (guardValue > 0) {
        TRmgMapItem* item = map.GetMapItem(position);
        TRmgZone* zone = zones[item->zoneState.zone];
        if (static_cast<int>(item->objects.size()) <= 0) {
            type_object* guard = CreateGuard(guardValue, zone);
            if (guard)
                AddObject(guard, position);
        }

        item = map.GetMapItem(otherPosition);
        zone = zones[item->zoneState.zone];
        if (static_cast<int>(item->objects.size()) <= 0) {
            type_object* guard = CreateGuard(guardValue, zone);
            if (guard)
                AddObject(guard, otherPosition);
        }
    }

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
void type_random_map_generator::ConnectZones()
{
    std::vector<TRmgMapItem*> borderItems;
    std::vector<TRmgMapPosition> borderPositions;

    TRmgMapItem* mapItem = map.mapItems;
    TRmgMapPosition position;
    for (position.z = 0; position.z < map.numberLevels; ++position.z) {
        for (position.y = 0; position.y < map.mapHeight; ++position.y) {
            for (position.x = 0; position.x < map.mapWidth;
                 ++position.x, ++mapItem) {
                if (mapItem->zoneState.connectionEligibility < 0)
                    continue;

                if (mapItem->tile.landType == eTerrainWater
                    || !mapItem->tileData.roadPassable
                    || mapItem->tile.landType == eTerrainRock)
                    continue;

                int direction = mapItem->tileData.connectionDirection;
                TRmgMapItem* otherMapItem = map.GetMapItem(
                    TRmgMapPosition(
                        position.x + g_rmgDirections[direction].x,
                        position.y + g_rmgDirections[direction].y,
                        position.z));
                if (otherMapItem->tile.landType != eTerrainWater
                    && otherMapItem->zoneState.zone
                           != mapItem->zoneState.zone) {
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
    for (zoneIndex = 0; zoneIndex < zones.size(); ++zoneIndex) {
        TRmgZone* zone = zones[zoneIndex];
        TRmgTownSlot* zoneTemplate = zone->slot;
        if (zone->terrain == eTerrainWater)
            continue;

        TRmgMapPosition levelPosition = zone->levelPosition;
        mapItem = map.GetMapItem(0, 0, levelPosition.z);
        for (int remaining = map.mapWidth * map.mapHeight;
             remaining--; ++mapItem)
            mapItem->tileData.connectionVisited = 0;

        for (int connectionIndex = 0;
             connectionIndex < zoneTemplate->connections.size();
             ++connectionIndex) {
            TRmgZoneConnection* connection =
                &zoneTemplate->connections[connectionIndex];
            if (connection->connected)
                continue;

            TRmgZone* destination =
                zones[connection->destination->zoneIndex];
            TRmgTownSlot* destinationTemplate = destination->slot;
            TRmgZoneConnection* oppositeConnection;
            int oppositeIndex = 0;
            for (;; ++oppositeIndex) {
                if (oppositeIndex
                    >= destinationTemplate->connections.size()) {
                    oppositeConnection = 0;
                    break;
                }
                if (destinationTemplate->connections[oppositeIndex]
                        .destination->zoneIndex == zoneIndex) {
                    oppositeConnection =
                        &destinationTemplate->connections[oppositeIndex];
                    break;
                }
            }

            if (CreateGroundConnection(
                    zone,
                    connection,
                    &borderItems,
                    &borderPositions)) {
                connection->connected = 1;
                oppositeConnection->connected = 1;
                continue;
            }

            if (CreateBorderConnection(zone, connection)) {
                connection->connected = 1;
                continue;
            }

            if (destination->terrain == eTerrainWater)
                continue;

            if (CreateSubterraneanGate(zone, connection)) {
                connection->connected = 1;
                oppositeConnection->connected = 1;
            }
        }
    }

    for (zoneIndex = 0; zoneIndex < zones.size(); ++zoneIndex) {
        TRmgZone* zone = zones[zoneIndex];
        TRmgTownSlot* zoneTemplate = zone->slot;
        if (zone->terrain == eTerrainWater)
            continue;

        int firstConnection = 0;
        while (firstConnection < zoneTemplate->connections.size()
               && zoneTemplate->connections[firstConnection].connected)
            ++firstConnection;
        if (firstConnection == zoneTemplate->connections.size())
            continue;

        TRmgMapPosition levelPosition = zone->levelPosition;
        mapItem = map.GetMapItem(0, 0, levelPosition.z);
        for (int remaining = map.mapWidth * map.mapHeight;
             remaining--; ++mapItem)
            mapItem->tileData.connectionVisited = 0;

        int objectIndex = 0;
        while (objectIndex < positions.size()) {
            type_object* object = positions[objectIndex];
            if (object->properties->prototype->objectType == SHIPYARD) {
                position = object->position;
                if (map.GetMapItem(position)->zoneState.zone == zoneIndex) {
                    TRmgMapPosition shipyardPosition = position;
                    int waterOffset = 0;
                    for (;
                         waterOffset < RMG_SHIPYARD_WATER_OFFSET_COUNT;
                         ++waterOffset) {
                        TRmgMapPosition waterPosition =
                            shipyardPosition
                            + g_rmgShipyardWaterOffsets[waterOffset];
                        if (waterPosition.x >= 0
                            && waterPosition.x < map.mapWidth
                            && map.GetMapItem(waterPosition)->tile.landType
                                   == eTerrainWater)
                            break;
                    }

                    if (waterOffset != RMG_SHIPYARD_WATER_OFFSET_COUNT)
                        FloodConnectionRegion(object->position);
                }
            }
            ++objectIndex;
        }

        for (int connectionIndex = firstConnection;
             connectionIndex < zoneTemplate->connections.size();
             ++connectionIndex) {
            TRmgZoneConnection* connection =
                &zoneTemplate->connections[connectionIndex];
            if (connection->connected)
                continue;

            TRmgZone* destination =
                zones[connection->destination->zoneIndex];
            TRmgTownSlot* destinationTemplate = destination->slot;
            TRmgZoneConnection* oppositeConnection;
            int oppositeIndex = 0;
            for (;; ++oppositeIndex) {
                if (oppositeIndex
                    >= destinationTemplate->connections.size()) {
                    oppositeConnection = 0;
                    break;
                }
                if (destinationTemplate->connections[oppositeIndex]
                        .destination->zoneIndex == zoneIndex) {
                    oppositeConnection =
                        &destinationTemplate->connections[oppositeIndex];
                    break;
                }
            }

            if (CreateBorderConnection(zone, connection)) {
                connection->connected = 1;
                continue;
            }

            if (destination->terrain == eTerrainWater)
                continue;

            CreateMonolithConnection(
                zone, connection, prototypeIndex);
            connection->connected = 1;
            oppositeConnection->connected = 1;
            prototypeIndex = (prototypeIndex + 1)
                % (objectPrototypes[LITH_TWOWAY].size()
                   + objectPrototypes[LITH_ONEWAY_ENTRANCE].size());
        }
    }

    if (progress)
        progress->Advance(0x1900);
}

// Retail retains this ordinary fastcall helper and expands the same four
// table accesses in ground, border, gate and monolith connections.  ECX is
// the requested value, EDX the strength index, and values below 2000 vanish.
// The name is provisional; the shared helper boundary is retail-byte proof.
// Exact: 91/91 raw bytes after resolving the four table references. Both
// implemented connection callers inline this ordinary definition naturally.
VA(0x00545E00, 0x5B) // anchor-callee 0x545990 cluster; retail-only
int GetRmgGuardValue(int value, int strength)
{
    int guardValue = 0;
    if (value > gRmgGuardThresholdLow[strength]) {
        guardValue = (value - gRmgGuardThresholdLow[strength])
            * gRmgGuardScaleLow[strength] / 4;
    }
    if (value > gRmgGuardThresholdHigh[strength]) {
        guardValue += (value - gRmgGuardThresholdHigh[strength])
            * gRmgGuardScaleHigh[strength] / 4;
    }
    return guardValue < 2000 ? 0 : guardValue;
}

// The road/river worklists instantiate all three of these out-of-line STL
// bodies.  Their distinct retail extents disambiguate the two int overloads.
VA_COMPGEN(0x00404200, 0x209, VECTOR_INSERT, Int)
VA_COMPGEN(0x00422F50, 0x1B1, VECTOR_INSERT, Int)
VA_COMPGEN(0x004347A0, 0x32E, VECTOR_INSERT, TRmgMapPosition)

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
void type_random_map_generator::BuildRoadCostMap(TRmgMapPosition position)
{
    std::vector<TRmgMapPosition> openPositions;
    std::vector<int> openCosts;

    openPositions.push_back(position);
    openCosts.push_back(0);

    TRmgMapItem* mapItem = map.GetMapItem(position);
    mapItem->movement.cost = 0;
    mapItem->previousTile.x = -1;
    mapItem->previousTile.y = -1;
    mapItem->previousTile.z = -1;

    while (openPositions.size()) {
        position = openPositions.back();
        openCosts.pop_back();
        openPositions.pop_back();

        mapItem = map.GetMapItem(position);
        int positionCost = mapItem->movement.cost;
        unsigned char currentDecorated = mapItem->tile.decorationType != 0;
        int direction = 8;
        unsigned char roadEntrance = mapItem->tileData.roadEntrance;

        if (roadEntrance) {
            type_object* object = mapItem->objects[0];
            TObjectType* properties = object->properties->prototype;
            int objectType = properties->objectType;
            if (!gAdventureObjectLandBlocked[objectType][1]
                && !gAdventureObjectLandBlocked[objectType][2])
                direction = 5;

            switch (objectType) {
            case LITH_ONEWAY_ENTRANCE:
            case LITH_ONEWAY_EXIT: {
                int subtype = properties->subtype;
                for (int i = 0; i < monolithsOneWay.size(); ++i) {
                    type_object* destination = monolithsOneWay[i];
                    if (destination->properties->prototype->subtype != subtype)
                        continue;

                    TRmgMapPosition nextPosition = destination->position;
                    TRmgMapItem* nextMapItem = map.GetMapItem(nextPosition);
                    int nextCost = positionCost + 50;
                    if (nextMapItem->movement.cost <= nextCost)
                        continue;

                    nextMapItem->setMovementCost(nextCost, position);
                    insertRmgWorkItem(
                        openPositions, openCosts, nextPosition, nextCost);
                }
                break;
            }

            case LITH_TWOWAY: {
                int subtype = properties->subtype;
                for (int i = 0; i < monolithsTwoWay.size(); ++i) {
                    type_object* destination = monolithsTwoWay[i];
                    if (destination->properties->prototype->subtype != subtype)
                        continue;

                    TRmgMapPosition nextPosition = destination->position;
                    TRmgMapItem* nextMapItem = map.GetMapItem(nextPosition);
                    int nextCost = positionCost + 50;
                    if (nextMapItem->movement.cost <= nextCost)
                        continue;

                    nextMapItem->setMovementCost(nextCost, position);
                    insertRmgWorkItem(
                        openPositions, openCosts, nextPosition, nextCost);
                }
                break;
            }

            case UNDERGROUND_GATE: {
                TRmgMapPosition nextPosition;
                nextPosition.x = position.x;
                nextPosition.y = position.y;
                nextPosition.z = 1 - position.z;
                TRmgMapItem* nextMapItem = map.GetMapItem(nextPosition);
                int nextCost = positionCost + 1;
                if (nextMapItem->movement.cost > nextCost) {
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
            nextPosition.x = position.x + directionOffset->x;
            nextPosition.y = position.y + directionOffset->y;
            nextPosition.z = position.z;

            if (nextPosition.x < 0 || nextPosition.x >= map.mapWidth
                || nextPosition.y < 0 || nextPosition.y >= map.mapHeight)
                continue;

            TRmgMapItem* nextMapItem = map.GetMapItem(nextPosition);
            if (nextMapItem->tile.landType == eTerrainWater
                || !nextMapItem->tileData.roadPassable
                || nextMapItem->tile.landType == eTerrainRock)
                continue;

            unsigned char nextRoadEntrance =
                nextMapItem->tileData.roadEntrance;
            if (nextRoadEntrance) {
                int objectType =
                    nextMapItem->objects[0]->properties->prototype->objectType;
                const unsigned char* traits =
                    gAdventureObjectLandBlocked[objectType];
                if (traits[0] && !traits[2])
                    continue;
                if (!traits[1] && !traits[2]
                    && direction > 0 && direction < 4)
                    continue;
            }

            int nextCost = currentDecorated
                               && nextMapItem->tile.decorationType
                           ? 2 : 20;
            if (direction & 1)
                nextCost *= 3;
            nextCost += positionCost;

            if (nextMapItem->movement.cost <= nextCost)
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
    TRmgMapItem* mapItem = map.GetMapItem(0, 0);
    unsigned width = map.mapWidth;
    unsigned height = map.mapHeight;
    TRmgGridPoint mapSize(width, height);
    int mapItemCount = mapSize.x * mapSize.y * map.numberLevels;
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
VA(0x00548DF0, 0x99F)  // water-wheel caller + river-delta object; retail-only
void type_random_map_generator::createRiver(TRmgMapPosition source)
{
    resetMovementCosts();

    TRmgMapItem* mapItem;
    TRmgMapPosition emptyPosition;
    emptyPosition.x = -1;
    emptyPosition.y = -1;
    emptyPosition.z = -1;

    std::vector<TRmgMapPosition> openPositions;
    std::vector<int> openCosts;

    openPositions.push_back(source);
    openCosts.push_back(0);
    mapItem = map.GetMapItem(source);
    mapItem->movement.cost = 0;
    mapItem->previousTile = emptyPosition;

    unsigned char sourceIsSnow;
    int riverType;
    if (mapItem->tile.landType == eTerrainSnow) {
        sourceIsSnow = 1;
        riverType = 2;
    } else {
        sourceIsSnow = 0;
        riverType = 1;
    }

    --source.y;
    openPositions.push_back(source);
    openCosts.push_back(0);
    mapItem = map.GetMapItem(source);
    mapItem->movement.cost = 0;
    mapItem->previousTile = emptyPosition;

    ++source.x;
    openPositions.push_back(source);
    openCosts.push_back(0);
    mapItem = map.GetMapItem(source);
    mapItem->movement.cost = 0;
    mapItem->previousTile = emptyPosition;

    TRmgMapPosition position;
    TRmgMapPosition nextPosition;
    int direction;

    while (!openPositions.empty()) {
        position = openPositions.back();
        openCosts.pop_back();
        openPositions.pop_back();

        mapItem = map.GetMapItem(position);
        int positionCost = mapItem->movement.cost;
        for (direction = 0; direction < 8; direction += 2) {
            nextPosition = position + g_rmgDirections[direction];

            if (nextPosition.x < 0 || nextPosition.x >= map.mapWidth
                || nextPosition.y < 0 || nextPosition.y >= map.mapHeight)
                continue;

            mapItem = map.GetMapItem(nextPosition);
            if (mapItem->tile.landType == eTerrainWater
                || mapItem->tile.landType == eTerrainRock
                || mapItem->isImpassable()
                || (mapItem->tile.landType == eTerrainSnow) != sourceIsSnow)
                continue;

            int nextCost = positionCost + (rand() & 31) + 1;
            if (mapItem->tile.decorationType)
                nextCost += 30;

            if (nextCost >= mapItem->movement.cost)
                continue;

            int oppositeDirection = ((direction - 4) >> 1) & 3;
            if (mapItem->tileData.blockedDirections
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

    mapItem->tileData.riverTarget = 1;
    position = nextPosition;

    type_random_map levelMap(map.GetMapItem(0, 0, nextPosition.z),
        map.mapWidth, map.mapHeight);
    TRmgMapAdapter mapAdapter(&levelMap);
    TRmgRiverPainter riverPainter(
        &mapAdapter, riverType, TRmgGridPoint(nextPosition.x, nextPosition.y));

    if (mapItem->tileData.blockedDirections) {
        for (direction = 0; direction < 4; ++direction) {
            if (mapItem->tileData.blockedDirections & (1 << direction))
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
        TTerrainType landType = mapItem->tile.landType;
        int prototypeIndex = 0;
        for (; prototypeIndex < objectPrototypes[TERRAIN_RIVER_DELTA].size();
             ++prototypeIndex) {
            TRmgObjectPropertiesRef* properties =
                objectPrototypes[TERRAIN_RIVER_DELTA][prototypeIndex];
            if (properties->prototype->recommendedTerrainMask[landType]
                && deltaIndex-- == 0)
                break;
        }

        if (prototypeIndex == objectPrototypes[TERRAIN_RIVER_DELTA].size())
            return;

        type_object* riverDelta = new type_object(
            objectPrototypes[TERRAIN_RIVER_DELTA][prototypeIndex]);
        AddObject(
            riverDelta,
            TRmgMapPosition(
                nextPosition.x + deltaOffsets[direction].x,
                nextPosition.y + deltaOffsets[direction].y,
                nextPosition.z));

        nextPosition = nextPosition + g_rmgDirections[direction * 2];
        riverPainter.DrawTo(TRmgGridPoint(nextPosition.x, nextPosition.y));
        mapItem = map.GetMapItem(nextPosition);
        mapItem->tileData.riverTarget = 1;

        riverPainter.DrawTo(TRmgGridPoint(position.x, position.y));
        mapItem = map.GetMapItem(position);
    }

    while (mapItem->movement.cost > 0) {
        position = mapItem->previousTile;
        mapItem = map.GetMapItem(position);
        mapItem->tileData.riverTarget = 1;
        riverPainter.DrawTo(TRmgGridPoint(position.x, position.y));
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
void type_random_map_generator::WriteMapHeader(TAbstractFile* outfile)
{
    {
        int intBuffer = GetSerializedMapVersion();
        outfile->Write(&intBuffer, sizeof(intBuffer));
    }

    {
        char byteBuffer = 1;
        outfile->Write(&byteBuffer, sizeof(byteBuffer));
    }

    {
        int intBuffer = map.mapWidth;
        outfile->Write(&intBuffer, sizeof(intBuffer));
    }

    {
        char byteBuffer = map.numberLevels > 1;
        outfile->Write(&byteBuffer, sizeof(byteBuffer));
    }

    std::string mapName(
        DATA_COMPGEN(0x00682900, rmgMapName, "Random Map"));
    {
        int intBuffer = mapName.length();
        outfile->Write(&intBuffer, sizeof(intBuffer));
    }
    outfile->Write(mapName.c_str(), mapName.length());

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
        templateName ? templateName
                     : DATA_COMPGEN(0x0063A608, rmgEmptyText, ""),
        randomSeed,
        map.mapWidth,
        map.numberLevels,
        humanPlayerCount,
        computerPlayerCount,
        gRmgWaterNames[waterContent],
        monsterStrength);

    switch (mapVersion) {
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
        if (fixedHumanPlayers[descriptionPlayer]) {
            strcat(
                description,
                DATA_COMPGEN(0x0066032C, rmgListSeparator, ", "));
            strcat(description, gRmgPlayerNames[descriptionPlayer]);
            strcat(
                description,
                DATA_COMPGEN(0x00682820, rmgIsHuman, " is human"));
        }

        if (townChoices[descriptionPlayer] != -1) {
            strcat(
                description,
                DATA_COMPGEN(0x0066032C, rmgListSeparator, ", "));
            strcat(description, gRmgPlayerNames[descriptionPlayer]);
            strcat(
                description,
                DATA_COMPGEN(
                    0x0068280C, rmgTownChoiceIs, " town choice is "));
            strcat(
                description,
                gRmgTownNames[townChoices[descriptionPlayer]]);
        }
    }

    {
        int intBuffer = strlen(description);
        outfile->Write(&intBuffer, sizeof(intBuffer));
    }
    outfile->Write(description, strlen(description));

    {
        char byteBuffer = 1;
        outfile->Write(&byteBuffer, sizeof(byteBuffer));
    }
    if (mapVersion >= 1) {
        char byteBuffer = 0;
        outfile->Write(&byteBuffer, sizeof(byteBuffer));
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
            for (; townIndex < zones.size(); ++townIndex) {
                TRmgZone* town = zones[townIndex];
                TRmgTownSlot* slot = town->slot;
                int player = slot->playerIndex;
                if (player < 0)
                    continue;

                player = playerIndexMap[player];
                if (player < 0 || !town->active)
                    continue;

                if (slot->kind == 0 && !canBeHuman[player]) {
                    ++generatedHumanTowns;
                    canBeHuman[player] = 1;
                    mainTowns[player] = town->position;
                }

                if (slot->kind == 1 && !canBeComputer[player]) {
                    canBeComputer[player] = 1;
                    mainTowns[player] = town->position;
                }

                legalAlignments[player] |= 1 << town->alignment;
            }
        }

        generatedHumanTowns -= humanPlayerCount;
        int reversePlayer = 7;
        do {
            if (canBeHuman[reversePlayer]
                && !fixedHumanPlayers[reversePlayer]
                && generatedHumanTowns > 0) {
                canBeComputer[reversePlayer] = 1;
                canBeHuman[reversePlayer] = 0;
                --generatedHumanTowns;
            }
        } while (reversePlayer-- != 0);

        computerPlayerCount = humanPlayerCount = 0;

        for (int serializedPlayer = 0; serializedPlayer < 8;
             ++serializedPlayer) {
            {
                char byteBuffer = canBeHuman[serializedPlayer];
                outfile->Write(&byteBuffer, sizeof(byteBuffer));
            }

            {
                char byteBuffer =
                    canBeHuman[serializedPlayer] || canBeComputer[serializedPlayer];
                outfile->Write(&byteBuffer, sizeof(byteBuffer));
            }

            {
                char byteBuffer = 0;
                outfile->Write(&byteBuffer, sizeof(byteBuffer));
            }

            if (mapVersion >= 2) {
                char byteBuffer = 0;
                outfile->Write(&byteBuffer, sizeof(byteBuffer));
            }

            if (mapVersion >= 1) {
                unsigned short alignment = legalAlignments[serializedPlayer];
                outfile->Write(&alignment, sizeof(alignment));
            } else {
                char byteBuffer = legalAlignments[serializedPlayer];
                outfile->Write(&byteBuffer, sizeof(byteBuffer));
            }

            {
                char byteBuffer = 0;
                outfile->Write(&byteBuffer, sizeof(byteBuffer));
            }

            if (!canBeHuman[serializedPlayer]
                && !canBeComputer[serializedPlayer]) {
                char byteBuffer = 0;
                outfile->Write(&byteBuffer, sizeof(byteBuffer));
            } else {
                if (canBeHuman[serializedPlayer])
                    ++humanPlayerCount;
                else
                    ++computerPlayerCount;

                {
                    char byteBuffer = 1;
                    outfile->Write(&byteBuffer, sizeof(byteBuffer));
                }

                if (mapVersion >= 1) {
                    {
                        char byteBuffer = 1;
                        outfile->Write(&byteBuffer, sizeof(byteBuffer));
                    }
                    {
                        char byteBuffer = -1;
                        outfile->Write(&byteBuffer, sizeof(byteBuffer));
                    }
                }

                {
                    char byteBuffer = mainTowns[serializedPlayer].x;
                    outfile->Write(&byteBuffer, sizeof(byteBuffer));
                }
                {
                    char byteBuffer = mainTowns[serializedPlayer].y;
                    outfile->Write(&byteBuffer, sizeof(byteBuffer));
                }
                {
                    char byteBuffer = mainTowns[serializedPlayer].z;
                    outfile->Write(&byteBuffer, sizeof(byteBuffer));
                }
            }

            {
                char byteBuffer = 0;
                outfile->Write(&byteBuffer, sizeof(byteBuffer));
            }
            {
                char byteBuffer = -1;
                outfile->Write(&byteBuffer, sizeof(byteBuffer));
            }

            if (mapVersion >= 1) {
                {
                    char byteBuffer = 0;
                    outfile->Write(&byteBuffer, sizeof(byteBuffer));
                }
                int intBuffer = 0;
                outfile->Write(&intBuffer, sizeof(intBuffer));
            }
        }

        {
            char byteBuffer = -1;
            outfile->Write(&byteBuffer, sizeof(byteBuffer));
        }
        {
            char byteBuffer = -1;
            outfile->Write(&byteBuffer, sizeof(byteBuffer));
        }

        if (!computerTeamCount)
            computerTeamCount = computerPlayerCount;
        if (!humanTeamCount)
            humanTeamCount = humanPlayerCount;
        if (!computerPlayerCount) {
            int teamCount = humanTeamCount;
            humanTeamCount = std::_cpp_max(teamCount, 2);
        }

        if (humanTeamCount >= humanPlayerCount
            && computerTeamCount >= computerPlayerCount) {
            char byteBuffer = 0;
            outfile->Write(&byteBuffer, sizeof(byteBuffer));
        } else {
            char teams[8];
            memset(teams, 0, sizeof(teams));

            {
                int teamCount = humanTeamCount;
                humanTeamCount = std::_cpp_max(teamCount, 1);
            }
            {
                int teamCount = computerTeamCount;
                computerTeamCount = std::_cpp_max(teamCount, 1);
            }
            {
                int playerCount = humanPlayerCount;
                int teamCount = humanTeamCount;
                humanTeamCount = std::_cpp_min(playerCount, teamCount);
            }
            {
                int playerCount = computerPlayerCount;
                int teamCount = computerTeamCount;
                computerTeamCount = std::_cpp_min(playerCount, teamCount);
            }

            assign_rmg_teams(
                humanTeamCount,
                humanPlayerCount,
                0,
                canBeHuman,
                teams);
            assign_rmg_teams(
                computerTeamCount,
                computerPlayerCount,
                humanTeamCount,
                canBeComputer,
                teams);

            {
                char byteBuffer = humanTeamCount + computerTeamCount;
                outfile->Write(&byteBuffer, sizeof(byteBuffer));
            }
            outfile->Write(teams, sizeof(teams));
        }
    }

    if (mapVersion >= 1) {
        std::bitset<156> availableHeroes;
        set_available_rmg_heroes(
            &availableHeroes, disabledHeroes, disabledHeroes + 156);

        unsigned char packedHeroes[20];
        memset(packedHeroes, 0, sizeof(packedHeroes));
        for (unsigned int heroBit = 0; heroBit < 156; ++heroBit) {
            if (availableHeroes.test(heroBit))
                packedHeroes[heroBit >> 3] |= 1 << (heroBit & 7);
        }
        outfile->Write(packedHeroes, sizeof(packedHeroes));
    } else {
        std::bitset<128> availableHeroes;
        set_available_rmg_heroes(
            &availableHeroes, disabledHeroes, disabledHeroes + 128);

        unsigned char packedHeroes[16];
        memset(packedHeroes, 0, sizeof(packedHeroes));
        for (unsigned int roeHeroBit = 0; roeHeroBit < 128; ++roeHeroBit) {
            if (availableHeroes.test(roeHeroBit))
                packedHeroes[roeHeroBit >> 3] |= 1 << (roeHeroBit & 7);
        }
        outfile->Write(packedHeroes, sizeof(packedHeroes));
    }

    if (mapVersion >= 1) {
        int intBuffer = 0;
        outfile->Write(&intBuffer, sizeof(intBuffer));
    }
    if (mapVersion >= 2) {
        char byteBuffer = 0;
        outfile->Write(&byteBuffer, sizeof(byteBuffer));
    }

    char reserved[31];
    memset(reserved, 0, sizeof(reserved));
    outfile->Write(reserved, sizeof(reserved));

    std::bitset<144> disabledArtifacts;
    for (int artifactIndex = 0; artifactIndex < 144; ++artifactIndex) {
        disabledArtifacts[artifactIndex] =
            akArtifactTraits[artifactIndex].comboType != -1;
    }
    disabledArtifacts.set(0);
    disabledArtifacts.set(63);

    if (mapVersion >= 2) {
        unsigned char packedArtifacts[18];
        memset(packedArtifacts, 0, sizeof(packedArtifacts));
        for (unsigned int artifactBit = 0; artifactBit < 144;
             ++artifactBit) {
            if (disabledArtifacts.test(artifactBit))
                packedArtifacts[artifactBit >> 3] |=
                    1 << (artifactBit & 7);
        }
        outfile->Write(packedArtifacts, sizeof(packedArtifacts));
    } else if (mapVersion >= 1) {
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
        outfile->Write(packedArtifacts, sizeof(packedArtifacts));
    }

    if (mapVersion >= 2) {
        std::bitset<70> disabledSpells;
        unsigned char packedSpells[9];
        memset(packedSpells, 0, sizeof(packedSpells));
        for (unsigned int spell = 0; spell < 70; ++spell) {
            if (disabledSpells.test(spell))
                packedSpells[spell >> 3] |= 1 << (spell & 7);
        }
        outfile->Write(packedSpells, sizeof(packedSpells));

        std::bitset<28> disabledSkills;
        unsigned char packedSkills[4];
        memset(packedSkills, 0, sizeof(packedSkills));
        for (unsigned int skill = 0; skill < 28; ++skill) {
            if (disabledSkills.test(skill))
                packedSkills[skill >> 3] |= 1 << (skill & 7);
        }
        outfile->Write(packedSkills, sizeof(packedSkills));

        char byteBuffer = 0;
        for (int hero = 0; hero < 156; ++hero)
            outfile->Write(&byteBuffer, sizeof(byteBuffer));
    }
}

// The helper's fastcall ABI is fixed by its two retail call sites: team and
// player counts arrive in ECX/EDX, followed by the first team id and the two
// eight-byte arrays.  Keeping it as a real helper preserves the source-level
// boundary retail chose not to inline.
VA(0x0054AB40, 0xAD)  // sole caller: WriteMapHeader; retail-only RMG
static void __fastcall assign_rmg_teams(
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
void __fastcall EmitRmgPointSetIncrement(TRmgPointSet::const_iterator* it)
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
    return TRmgVector(x + other.x, y + other.y);
}

VA(0x005FDCD0, 0x1D) // caller 0x5fdc2f; thiscall, ret 8
TRmgVector TRmgVector::operator*(int scale) const
{
    return TRmgVector(x * scale, y * scale);
}

VA(0x005FDCF0, 0x25) // callers 0x5fdc36/0x5fdc50; signed division, ret 8
TRmgVector TRmgVector::operator/(int divisor) const
{
    return TRmgVector(x / divisor, y / divisor);
}

VA(0x005FDD20, 0x20) // caller 0x5fdc64; hidden result ECX, two 8-byte values
TPoint operator+(TPoint point, TRmgVector offset)
{
    return TPoint(point.x + offset.x, point.y + offset.y);
}

VA(0x005FDD40, 0x20) // callers 0x5fdbd8/0x5fdbf5; hidden result ECX, ret 16
TRmgVector operator-(TPoint left, TPoint right)
{
    return TRmgVector(left.x - right.x, left.y - right.y);
}
