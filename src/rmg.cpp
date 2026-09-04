// rmg.cpp - Complete-only random-map generator support.
//
// The Dreamcast build has no RMG compiland. Retail's direct caller graph
// reaches this library from TSingleSelectionWindow::GenerateRandomMap, and
// the tree node layout proves an eight-byte TPoint value ordered by y, then x.
#include <va.h>
#define _MT
#include <yvals.h>
#undef _MT
#include <algorithm>
#include <bitset>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "abstractfile.h"
#include "artifact.h"
#include "armygrp.h"
#include "bitset_iterator.h"
#include "rmg.h"

typedef std::set<TPoint> TRmgPointSet;

namespace {

// The cinit at 0x530da0 writes these eight clockwise neighbours.  The river
// search advances by two entries, selecting only the four cardinal offsets.
DATA(0x0069CDC0)
TPoint gRmgDirections[8] = {
    TPoint(1, 0),
    TPoint(1, 1),
    TPoint(0, 1),
    TPoint(-1, 1),
    TPoint(-1, 0),
    TPoint(-1, -1),
    TPoint(0, -1),
    TPoint(1, -1)
};

DATA(0x006409A0)
static const int gLandRiverDeltaIndex[4] = {2, 0, 3, 1};

DATA(0x006409B0)
static const int gSnowRiverDeltaIndex[4] = {7, 5, 4, 6};

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

} // namespace

namespace std {

// WriteMapHeader's ordered retail call stream is the boundary oracle for the
// TU-local Dinkumware definitions below. Temporary inline pins plus the MAX
// ratchets those calls.  Negative control: removing both string constructor
// definitions regresses WriteMapHeader from 95.70% to 94.10%, grows its frame
// from 0x32c to 0x334, and adds three target-only calls.
template <>
inline basic_string<char, char_traits<char>, allocator<char> >::basic_string(
    const std::allocator<char>& value)
    : allocator(value)
{
    // WriteMapHeader -> basic_string::_Tidy: retail retains this call.
#pragma inline_depth(0)
    _Tidy();
#pragma inline_depth()
}

template <>
inline basic_string<char, char_traits<char>, allocator<char> >::basic_string(
    const char* source,
    const std::allocator<char>& value)
    : allocator(value)
{
    // WriteMapHeader -> basic_string::_Tidy: retail retains this call.
#pragma inline_depth(0)
    _Tidy();
#pragma inline_depth()
    // WriteMapHeader -> basic_string::assign: retail retains this call.
#pragma inline_depth(0)
    assign(source, strlen(source));
#pragma inline_depth()
}

} // namespace std

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

// Complete emits this ordinary by-value accessor once, then lets VC6 choose
// its boundary independently at each RMG call site.  The standalone body's
// `ret 0xc` proves that TRmgMapPosition is passed by value rather than by
// reference; CreateSubterraneanGate retains its final two calls while
// expanding the earlier ones.
VA(0x005378E0, 0x27)
TRmgMapItem* type_random_map::GetMapItem(TRmgMapPosition point)
{
    return mapItems
        + (point.z * mapHeight + point.y) * mapWidth
        + point.x;
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
    TRmgObjectProperties* gatePrototype = gateProperties->prototype;

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

    TPoint entrance(
        position.x - gatePrototype->enterX,
        position.y - gatePrototype->enterY);
    source->entrances.push_back(entrance);
    destination->entrances.push_back(entrance);

    int guardValue;
    if (connection->unguarded) {
        guardValue = 0;
    } else {
        int strength = 0;
        int level = monsterStrength;
        if (connection->value > gRmgGuardThresholdLow[level]) {
            strength =
                (connection->value - gRmgGuardThresholdLow[level])
                * gRmgGuardScaleLow[level] / 4;
        }
        if (connection->value > gRmgGuardThresholdHigh[level]) {
            strength +=
                (connection->value - gRmgGuardThresholdHigh[level])
                * gRmgGuardScaleHigh[level] / 4;
        }
        guardValue = strength < 2000 ? 0 : strength;
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

// The Complete RMG has no Dreamcast counterpart.  Retail nevertheless fixes
// the whole source-level algorithm: two parallel vectors form a descending
// cost worklist, four cardinal neighbours relax a randomized Dijkstra search,
// and the predecessor chain is then painted back from the first river target.
// The water-wheel caller at 0x549870 and object type 143 selected below prove
// the river role; the method spelling remains provisional.
VA(0x00548DF0, 0x99F)  // water-wheel caller + river-delta object; retail-only
void type_random_map_generator::CreateRiver(TRmgMapPosition source)
{
    TRmgMapPosition invalidPosition(-1, -1, -1);
    TRmgMapItem* mapItem = map.GetMapItem(0, 0);
    int mapItemCount = map.mapWidth * map.mapHeight * map.numberLevels;
    while (mapItemCount--) {
        mapItem->movement.cost = 32000;
        mapItem->previousTile = invalidPosition;
        ++mapItem;
    }

    std::vector<TRmgMapPosition> openPositions;
    std::vector<int> openCosts;
    int zeroCost = 0;

    openPositions.push_back(source);
    openCosts.push_back(zeroCost);
    mapItem = map.GetMapItem(source);
    mapItem->movement.cost = 0;
    mapItem->previousTile = invalidPosition;

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
    openCosts.push_back(zeroCost);
    mapItem = map.GetMapItem(source);
    mapItem->movement.cost = 0;
    mapItem->previousTile = invalidPosition;

    ++source.x;
    openPositions.push_back(source);
    openCosts.push_back(zeroCost);
    mapItem = map.GetMapItem(source);
    mapItem->movement.cost = 0;
    mapItem->previousTile = invalidPosition;

    TRmgMapPosition position;
    TRmgMapPosition nextPosition;
    int direction;

    while (!openPositions.empty()) {
        position = openPositions.back();
        openCosts.pop_back();
        openPositions.pop_back();

        mapItem = map.GetMapItem(position);
        int positionCost = mapItem->movement.cost;
        direction = 0;
        TPoint* directionOffset = gRmgDirections;
        for (; directionOffset < gRmgDirections + 8;
             directionOffset += 2, direction += 2) {
            nextPosition = TRmgMapPosition(
                position.x + directionOffset->x,
                position.y + directionOffset->y,
                position.z);

            if (nextPosition.x < 0 || nextPosition.x >= map.mapWidth
                || nextPosition.y < 0 || nextPosition.y >= map.mapHeight)
                continue;

            TRmgMapItem* nextMapItem = map.GetMapItem(nextPosition);
            TTerrainType landType = nextMapItem->tile.landType;
            if (landType == eTerrainWater || landType == eTerrainRock
                || nextMapItem->tileData.impassable
                || (landType == eTerrainSnow) != sourceIsSnow)
                continue;

            int nextCost = positionCost + (rand() & 31) + 1;
            if (nextMapItem->tile.decorationType)
                nextCost += 30;

            if (nextCost >= nextMapItem->movement.cost)
                continue;

            int oppositeDirection = ((direction - 4) >> 1) & 3;
            if (nextMapItem->tileData.blockedDirections
                & (1 << oppositeDirection))
                continue;

            nextMapItem->movement.cost = nextCost;
            nextMapItem->previousTile = position;

            int first = 0;
            int last = openPositions.size();
            while (first < last) {
                int middle = (first + last) >> 1;
                if (nextCost < openCosts[middle])
                    first = middle + 1;
                else
                    last = middle;
            }

            openPositions.insert(openPositions.begin() + first, nextPosition);
            openCosts.insert(openCosts.begin() + first, nextCost);

            if (nextMapItem->tileData.riverTarget) {
                openPositions.clear();
                break;
            }
        }
    }

    if (!mapItem->tileData.riverTarget)
        return;

    mapItem->tileData.riverTarget = 1;

    type_random_map levelMap(map, nextPosition.z);
    TRmgMapAdapter mapAdapter(&levelMap);
    TPoint riverPosition(nextPosition.x, nextPosition.y);
    TRmgRiverPainter riverPainter(&mapAdapter, riverType, riverPosition);

    unsigned blockedDirections = mapItem->tileData.blockedDirections;
    if (blockedDirections) {
        direction = 0;
        while (!(blockedDirections & (1 << direction)))
            ++direction;

        static TRmgRiverDeltaOffset deltaOffsets[4] = {
            TRmgRiverDeltaOffset(4, 1),
            TRmgRiverDeltaOffset(1, 4),
            TRmgRiverDeltaOffset(-2, 1),
            TRmgRiverDeltaOffset(1, -2)
        };

        int deltaIndex = sourceIsSnow
            ? gSnowRiverDeltaIndex[direction]
            : gLandRiverDeltaIndex[direction];
        TTerrainType landType = mapItem->tile.landType;
        int prototypeIndex = 0;
        for (; prototypeIndex < objectPrototypes[TERRAIN_RIVER_DELTA].size();
             ++prototypeIndex) {
            TRmgObjectPropertiesRef* properties =
                objectPrototypes[TERRAIN_RIVER_DELTA][prototypeIndex];
            if (properties->prototype->landPage.test(landType)
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

        TRmgMapPosition riverMouth(
            nextPosition.x + gRmgDirections[direction * 2].x,
            nextPosition.y + gRmgDirections[direction * 2].y,
            nextPosition.z);
        TPoint riverMouthPoint(riverMouth.x, riverMouth.y);
        riverPainter.DrawTo(riverMouthPoint);
        mapItem = map.GetMapItem(riverMouth);
        mapItem->tileData.riverTarget = 1;

        riverPosition.x = nextPosition.x;
        riverPosition.y = nextPosition.y;
        riverPainter.DrawTo(riverPosition);
        mapItem = map.GetMapItem(nextPosition);
    }

    while (mapItem->movement.cost > 0) {
        position = mapItem->previousTile;
        mapItem = map.GetMapItem(position);
        mapItem->tileData.riverTarget = 1;
        riverPosition.x = position.x;
        riverPosition.y = position.y;
        riverPainter.DrawTo(riverPosition);
    }
}

// Complete's random-map pipeline calls this routine immediately before the
// generated terrain/object stream is emitted.  The format switch, description
// fragments, player records, team assignment, and packed availability masks
// are all read directly from retail's stream-write CFG.  The Dreamcast port
// has no RMG compiland, so the method spelling remains provisional while its
// class offsets and serialization order are retail-byte facts.
//
// Residual (95.71%, 2026-09-03): all 164 CFG blocks and all 87 branches align;
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
