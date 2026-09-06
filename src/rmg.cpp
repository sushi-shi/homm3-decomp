// rmg.cpp - Complete-only random-map generator support.
//
// The Dreamcast build has no RMG compiland. Retail's direct caller graph
// reaches this library from TSingleSelectionWindow::GenerateRandomMap, and
// the tree node layout proves an eight-byte TPoint value ordered by y, then x.
#include <va.h>
#include <algorithm>
#include <bitset>
#include <ctype.h>
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
#include "textresource.h"

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

// Shipyards are three tiles wide.  The connection repair pass probes the
// four water-facing squares beside their upper and lower edges before it
// floods the reachable water region.
DATA(0x0069CE00)
TPoint gRmgShipyardWaterOffsets[RMG_SHIPYARD_WATER_OFFSET_COUNT] = {
    TPoint(-3, 0),
    TPoint(1, 0),
    TPoint(-3, 1),
    TPoint(1, 1)
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

// ReadRmgTemplateZones repeatedly tests a nullable field for a nonempty,
// non-space leading character. Keep the shared predicate as an ordinary
// helper; its original name and declaration are not in the DC corpus.
static bool IsRmgTemplateFieldSet(const char* value)
{
    return value && value[0] && value[0] != ' ';
}

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
            TPoint perpendicular;
            {
                TPoint delta = to - from;
                perpendicular = TPoint(-delta.y, delta.x);
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
// takes its eight-byte right operand by value; a const reference leaves
// the final maximum-Y multiply in the wrong registers. Passing by value
// closes all bytes. Changing operator- to by value is independently flat.
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

    TPoint delta = toward - point;
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

    position.x -= gatePrototype->enterX;
    position.y -= gatePrototype->enterY;
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
                        position.x + gRmgDirections[direction].x,
                        position.y + gRmgDirections[direction].y,
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
            if (object->properties->prototype->type == SHIPYARD) {
                position = object->position;
                if (map.GetMapItem(position)->zoneState.zone == zoneIndex) {
                    TRmgMapPosition shipyardPosition = position;
                    int waterOffset = 0;
                    for (;
                         waterOffset < RMG_SHIPYARD_WATER_OFFSET_COUNT;
                         ++waterOffset) {
                        TRmgMapPosition waterPosition =
                            shipyardPosition
                            + gRmgShipyardWaterOffsets[waterOffset];
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

// The road/river worklists instantiate all three of these out-of-line STL
// bodies.  Their distinct retail extents disambiguate the two int overloads.
VA_COMPGEN(0x00404200, 0x209, VECTOR_INSERT, Int)
VA_COMPGEN(0x00422F50, 0x1B1, VECTOR_INSERT, Int)
VA_COMPGEN(0x004347A0, 0x32E, VECTOR_INSERT, TRmgMapPosition)

// DrawIrregularZoneBoundary retains this single-element erase. Its
// eight-byte copy loop and ret 4 agree in all 61 raw retail bytes.
VA_COMPGEN(0x0054CD70, 0x3D, VECTOR_ERASE, TPoint)

// BuildRoadCostMap and CreateRiver both materialize a separate by-value
// position immediately before this identical descending binary search.  The
// same expansion recurs in the surrounding retail RMG corpus, with no retained
// standalone body.  An ordinary internal helper reproduces that boundary and
// lets VC6 /Ob2 decide the expansions; Dreamcast has no RMG compiland, so the
// original spelling and linkage remain provisional.
// The search uses one top test with two unconditional back edges in retail.
// VC6 rotates for (;;) and while (first < last) spellings; while (1) keeps
// this top test and restores that flow in CreateRiver (76.51% -> 79.82%).
static void InsertRmgWorkItem(
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
            TRmgObjectProperties* properties = object->properties->prototype;
            int objectType = properties->type;
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

                    nextMapItem->SetMovementCost(nextCost, position);
                    InsertRmgWorkItem(
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

                    nextMapItem->SetMovementCost(nextCost, position);
                    InsertRmgWorkItem(
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
                    nextMapItem->SetMovementCost(nextCost, position);
                    InsertRmgWorkItem(
                        openPositions, openCosts,
                        nextPosition, nextCost);
                }
                break;
            }
            }
        }

        while (direction--) {
            TPoint* directionOffset = &gRmgDirections[direction];
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
                    nextMapItem->objects[0]->properties->prototype->type;
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

            nextMapItem->SetMovementCost(nextCost, position);
            InsertRmgWorkItem(
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
// Named dimensions, scalar getters and TPoint size queries do not recover
// retail's height temporary: they either stay flat or spill the map-item
// pointer instead. Do not infer dimension accessors from their fuzzy score.
void type_random_map_generator::ResetMovementCosts()
{
    TRmgMapPosition resetPosition(-1, -1, -1);
    TRmgMapItem* mapItem = map.GetMapItem(0, 0);
    int mapItemCount = map.mapWidth * map.mapHeight * map.numberLevels;
    while (mapItemCount--) {
        mapItem->ResetMovement(resetPosition);
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
// for the missing natural boundary. Live C2 tracing measures caller cb=1530:
// the early empty _Destroy helpers cost 49 but receive 68/65. Later map
// cleanups already retain/expand correctly at budgets 91/251 for cost 97.
// Direct erase() calls expand even further (61.45% before the seed-copy
// correction). An explicit predecessor copy and const by-value parameter
// are byte-flat. A const-ref setter changes the shared road helper's proved by-value boundary and is
// rejected; a combined reset/cost setter and by-value position assignment
// also fail the reset's constant-cost and copy sequence.
VA(0x00548DF0, 0x99F)  // water-wheel caller + river-delta object; retail-only
void type_random_map_generator::CreateRiver(TRmgMapPosition source)
{
    ResetMovementCosts();

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
            nextPosition = position + gRmgDirections[direction];

            if (nextPosition.x < 0 || nextPosition.x >= map.mapWidth
                || nextPosition.y < 0 || nextPosition.y >= map.mapHeight)
                continue;

            mapItem = map.GetMapItem(nextPosition);
            if (mapItem->tile.landType == eTerrainWater
                || mapItem->tile.landType == eTerrainRock
                || mapItem->IsImpassable()
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

            mapItem->SetMovementCost(nextCost, position);
            InsertRmgWorkItem(
                openPositions, openCosts, nextPosition, nextCost);

            if (mapItem->IsRiverTarget()) {
                openPositions.clear();
                break;
            }
        }
    }

    if (!mapItem->IsRiverTarget())
        return;

    mapItem->tileData.riverTarget = 1;
    position = nextPosition;

    type_random_map levelMap(map, nextPosition.z);
    TRmgMapAdapter mapAdapter(&levelMap);
    TRmgRiverPainter riverPainter(
        &mapAdapter, riverType, TPoint(nextPosition.x, nextPosition.y));

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
            ? gSnowRiverDeltaIndex[direction]
            : gLandRiverDeltaIndex[direction];
        TTerrainType landType = mapItem->tile.landType;
        int prototypeIndex = 0;
        for (; prototypeIndex < objectPrototypes[TERRAIN_RIVER_DELTA].size();
             ++prototypeIndex) {
            TRmgObjectPropertiesRef* properties =
                objectPrototypes[TERRAIN_RIVER_DELTA][prototypeIndex];
            if (properties->prototype->landPage[landType]
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

        nextPosition = nextPosition + gRmgDirections[direction * 2];
        riverPainter.DrawTo(TPoint(nextPosition.x, nextPosition.y));
        mapItem = map.GetMapItem(nextPosition);
        mapItem->tileData.riverTarget = 1;

        riverPainter.DrawTo(TPoint(position.x, position.y));
        mapItem = map.GetMapItem(position);
    }

    while (mapItem->movement.cost > 0) {
        position = mapItem->previousTile;
        mapItem = map.GetMapItem(position);
        mapItem->tileData.riverTarget = 1;
        riverPainter.DrawTo(TPoint(position.x, position.y));
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
