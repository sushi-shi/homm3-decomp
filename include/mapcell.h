#ifndef HOMM3_MAPCELL_H
#define HOMM3_MAPCELL_H

#include <string>
#include <vector>

#include "artifact.h"
#include "domains.h"
#include "herospec.h"
#include "primaryskill.h"
#include "secondaryskill.h"
#include "terrain_type.h"
#include "town.h"

class BlackBoxData;
struct type_creature_bank;
struct type_university;

class CObject;

// The adventure-map object domain, the type of NewmapCell::type.
// Transcribed COMPLETE from the Dreamcast CodeView enum (163
// enumerators, every value written out explicitly so the four values
// the DC build leaves unnamed - 1, 18, 19 and 40 - stay visibly
// unnamed rather than being invented). Retail proof so far is one
// value: find_magus_hut_value (0x429910) compares the dword at
// NewmapCell+0x1e against 0x1b, and 0x1b is EYE_OF_MAGI - the object
// a Hut of Magi reveals, which is exactly what that function sums.
enum TAdventureObjectType {
    NOTHING                    = 0,
    ALTAR_OF_SACRIFICE         = 2,
    ANCHOR_POINT               = 3,
    ARENA                      = 4,
    ARTIFACT                   = 5,
    BLACK_BOX                  = 6,
    BLACK_MARKET               = 7,
    BOAT                       = 8,
    BORDER_GUARD               = 9,
    BORDER_TENT                = 10,
    BUOY                       = 11,
    CAMPFIRE                   = 12,
    CARTOGRAPHER               = 13,
    CLOVER_FIELD               = 14,
    COVER_OF_DARKNESS          = 15,
    CREATURE_BANK              = 16,
    CREATURE_GENERATOR_1       = 17,
    CREATURE_GENERATOR_4       = 20,
    CURSED_GROUND              = 21,
    DEAD_GUY                   = 22,
    DEFENSE_TOWER              = 23,
    DERELICT_SHIP              = 24,
    DRAGON_CITY                = 25,
    EVENT                      = 26,
    EYE_OF_MAGI                = 27,
    FAERIE_RING                = 28,
    FLOTSAM                    = 29,
    FOUNTAIN_OF_FORTUNE        = 30,
    FOUNTAIN_OF_YOUTH          = 31,
    GARDEN_OF_REVELATION       = 32,
    GARRISON                   = 33,
    HERO                       = 34,
    HILL_FORT                  = 35,
    HOLY_GRAIL                 = 36,
    HUT_OF_MAGI                = 37,
    IDOL_OF_FORTUNE            = 38,
    LEAN_TO                    = 39,
    LIBRARY                    = 41,
    LIGHTHOUSE                 = 42,
    LITH_ONEWAY_ENTRANCE       = 43,
    LITH_ONEWAY_EXIT           = 44,
    LITH_TWOWAY                = 45,
    MAGIC_PLAINS               = 46,
    MAGIC_SCHOOL               = 47,
    MAGIC_SPRING               = 48,
    MAGIC_WELL                 = 49,
    MARKET_OF_TIME             = 50,
    MERC_CAMP                  = 51,
    MERMAID                    = 52,
    MINE                       = 53,
    MONSTER                    = 54,
    MYSTICAL_GARDEN            = 55,
    OASIS                      = 56,
    OBELISK                    = 57,
    OBSERVATORY                = 58,
    OCEAN_BOTTLE               = 59,
    PILLAR_OF_FIRE             = 60,
    POWER_SCHOOL               = 61,
    PRISON                     = 62,
    PYRAMID                    = 63,
    RALLY_FLAG                 = 64,
    RANDOM_ARTIFACT            = 65,
    RANDOM_ARTIFACT_1          = 66,
    RANDOM_ARTIFACT_2          = 67,
    RANDOM_ARTIFACT_3          = 68,
    RANDOM_ARTIFACT_4          = 69,
    RANDOM_HERO                = 70,
    RANDOM_MONSTER             = 71,
    RANDOM_MONSTER_1           = 72,
    RANDOM_MONSTER_2           = 73,
    RANDOM_MONSTER_3           = 74,
    RANDOM_MONSTER_4           = 75,
    RANDOM_RESOURCE            = 76,
    RANDOM_TOWN                = 77,
    REFUGEE_CAMP               = 78,
    RESOURCE                   = 79,
    SANCTUARY                  = 80,
    SCHOLAR                    = 81,
    SEA_CHEST                  = 82,
    SEER                       = 83,
    SEPULCHER                  = 84,
    SHIPWRECK                  = 85,
    SHIPWRECK_SURVIVOR         = 86,
    SHIPYARD                   = 87,
    SHRINE1                    = 88,
    SHRINE2                    = 89,
    SHRINE3                    = 90,
    SIGN                       = 91,
    SIREN                      = 92,
    SPELL_SCROLL               = 93,
    STABLES                    = 94,
    TAVERN                     = 95,
    TEMPLE                     = 96,
    THIEVES_DEN                = 97,
    TOWN                       = 98,
    TRADING_POST               = 99,
    TRAINING_GROUNDS           = 100,
    TREASURE_CHEST             = 101,
    TREE_OF_KNOWLEDGE          = 102,
    UNDERGROUND_GATE           = 103,
    UNIVERSITY                 = 104,
    WAGON                      = 105,
    WAR_MACHINE_FACTORY        = 106,
    WAR_SCHOOL                 = 107,
    WARRIOR_TOMB               = 108,
    WATER_WHEEL                = 109,
    WATERING_HOLE              = 110,
    WHIRLPOOL                  = 111,
    WINDMILL                   = 112,
    WITCH_HUT                  = 113,
    TERRAIN_BRUSH              = 114,
    const_first_terrain_object = 114,
    TERRAIN_BUSH               = 115,
    TERRAIN_CACTUS             = 116,
    TERRAIN_CANYON             = 117,
    TERRAIN_CRATER             = 118,
    TERRAIN_DEAD_VEGETATION    = 119,
    TERRAIN_FLOWER             = 120,
    TERRAIN_FROZEN_LAKE        = 121,
    TERRAIN_HEDGE              = 122,
    TERRAIN_HILL               = 123,
    TERRAIN_HOLE               = 124,
    TERRAIN_KELP               = 125,
    TERRAIN_LAKE               = 126,
    TERRAIN_LAVA_FLOW          = 127,
    TERRAIN_LAVA_LAKE          = 128,
    TERRAIN_MUSHROOM           = 129,
    TERRAIN_LOG                = 130,
    TERRAIN_MANDRAKE           = 131,
    TERRAIN_MOSS               = 132,
    TERRAIN_MOUND              = 133,
    TERRAIN_MOUNTAIN           = 134,
    TERRAIN_OAK_TREE           = 135,
    TERRAIN_OUTCROPPING        = 136,
    TERRAIN_PINE_TREE          = 137,
    TERRAIN_PLANT              = 138,
    TERRAIN_RIVER_1            = 139,
    TERRAIN_RIVER_2            = 140,
    TERRAIN_RIVER_3            = 141,
    TERRAIN_RIVER_4            = 142,
    TERRAIN_RIVER_DELTA        = 143,
    TERRAIN_ROAD_1             = 144,
    TERRAIN_ROAD_2             = 145,
    TERRAIN_ROAD_3             = 146,
    TERRAIN_ROCK               = 147,
    TERRAIN_SAND_DUNE          = 148,
    TERRAIN_SAND_PIT           = 149,
    TERRAIN_SHRUB              = 150,
    TERRAIN_SKULL              = 151,
    TERRAIN_STALAGMITE         = 152,
    TERRAIN_STUMP              = 153,
    TERRAIN_TAR_PIT            = 154,
    TERRAIN_TREE               = 155,
    TERRAIN_VINE               = 156,
    TERRAIN_VOLCANIC_VENT      = 157,
    TERRAIN_VOLCANO            = 158,
    TERRAIN_WILLOW_TREE        = 159,
    TERRAIN_YUCCA_TREE         = 160,
    TERRAIN_REEF               = 161,
    CLOVER_FIELD_2             = 222,
    EVIL_FOG                   = 224,
    FAVORABLE_WINDS            = 225,
    FIERY_FIELDS               = 226,
    HOLY_GROUND                = 227,
    LUCID_POOLS                = 228,
    MAGIC_CLOUDS               = 229,
    ROCKLANDS                  = 231,
    RANDOM_MONSTER_5           = 162,
    RANDOM_MONSTER_6           = 163,
    RANDOM_MONSTER_7           = 164,
    MAX_EVENT_TYPE             = 165
};

// The save-version <25 views used by mapcell.cpp's retail-only compatibility
// pass. The legacy layouts are published by the Dreamcast CodeView record;
// the current layouts are fixed by retail's seven conversion arms. They are
// canonical declarations even though only mapcell.obj converts serialized
// legacy dwords.
struct LegacyArtifactInfo {
public:
    signed long m_price : 4;
    signed long m_guard : 8;
    signed long m_resourcePrice : 4;
    unsigned long m_guardQty : 15;
    unsigned long m_custom : 1;
};

struct CurrentArtifactInfo {
public:
    signed long m_price : 4;
    signed long m_guard : 9;
    signed long m_resourcePrice : 4;
    unsigned long m_guardQty : 14;
    unsigned long m_custom : 1;
};

struct LegacySkeletonInfo {
public:
    unsigned long m_id : 5;
    unsigned long m_unused : 1;
    unsigned long m_artifact : 7;
    unsigned long m_hasTreasure : 1;
    unsigned long m_tail : 18;
};

struct LegacyMonsterInfo {
public:
    unsigned long m_qty : 12;
    signed long m_disposition : 5;
    unsigned long m_neverFlee : 1;
    unsigned long m_dontGrow : 1;
    unsigned long m_index : 12;
    unsigned long m_custom : 1;
};

struct LegacyPyramidInfo {
public:
    unsigned long m_guarded : 1;
    unsigned long m_unused : 3;
    unsigned long m_visitedBits : 8;
    signed long m_spell : 8;
    unsigned long m_tail : 12;
};

struct LegacyTreasureInfo {
public:
    signed long m_artifact : 8;
    unsigned long m_isArtifact : 1;
    unsigned long m_goldAmount : 4;
    unsigned long m_tail : 19;
};

struct LegacyWagonInfo {
public:
    unsigned long m_resourceAmount : 5;
    unsigned long m_visitedBits : 8;
    unsigned long m_full : 1;
    unsigned long m_hasArtifact : 1;
    signed long m_artifact : 8;
    signed long m_resource : 4;
    unsigned long m_tail : 5;
};

struct CurrentUpgradeWagonInfo {
public:
    unsigned long m_resourceAmount : 5;
    unsigned long m_visitedBits : 8;
    unsigned long m_full : 1;
    unsigned long m_hasArtifact : 1;
    signed long m_artifact : 10;
    signed long m_resource : 4;
    unsigned long m_tail : 3;
};

struct LegacyTombInfo {
public:
    unsigned long m_full : 1;
    unsigned long m_unused : 4;
    unsigned long m_visitedBits : 8;
    signed long m_artifact : 8;
    unsigned long m_tail : 11;
};

struct CurrentVisitedInfo {
public:
    unsigned long m_head : 5;
    unsigned long m_visitedBits : 8;
    unsigned long m_tail : 19;
};

struct ShipyardInfo {
public:
    enum { NO_BOAT = 0xff };
    signed int m_owner : 8;
    unsigned int m_boatX : 8;
    unsigned int m_boatY : 8;
    unsigned int m_unused : 8;
};
SIZE(ShipyardInfo, 4);

// The wandering-monster arm of the cell's extra-info dword. The Dreamcast
// publishes the arm (`ExtraInfoUnion::MonsterInfo monster_info`) and all
// six member names - qty, disposition, never_flee, dont_grow, index,
// custom - and advManager::DoWanderingMonsterResult (0x4a7740) fixes every
// position and signedness against retail:
//   * bits 0..11  UNSIGNED qty                `and ebx, 0xfff`
//   * bits 12..16 SIGNED disposition          `shl esi,0xf / sar esi,0x1b`
//   * bit  17     never_flee                  `test dword [esi], 0x20000`
//   * bit  18     dont_grow                   (named by the DC, unread here)
//   * bits 19..26 index                       `shr eax,0x13 / and eax,0xff`
//   * bit  31     custom                      `shr edx,0x1f / test dl,1`
// Dreamcast's older index is 12 bits. Complete uses an eight-bit index and
// clears the intervening four bits explicitly; Windows readMonsterData's
// 0x87fbffff mask and Mac code0+0x123e64..0x123e6c prove that clearing.
// GATED: this is a type DEFINITION in a header that rides initialize.cpp's
// include closure - see the cellFlags note inside the class for what an
// ungated one costs there.
struct MonsterInfo {
public:
    unsigned long m_qty : 12;
    signed long m_disposition : 5;
    unsigned long m_neverFlee : 1;
    unsigned long m_dontGrow : 1;
    unsigned long m_index : 8;
    unsigned long m_unused27 : 4;
    unsigned long m_custom : 1;
};
SIZE(MonsterInfo, 4);

// The campfire's arm of the same dword. advManager::DoEventCampfire
// (0x4a1120) fixes both lanes: a four-bit UNSIGNED resource id at bits
// 0..3 (`and eax,0xf`, no sign extension - the lean-to's lane again) and
// the count in EVERYTHING above it, read as `shr ecx,4 / movsx edi,cx`.
// That pair is the signature of a WIDE unsigned field truncated by a
// `short` return, not of a sixteen-bit signed bitfield, which would have
// been `shl / sar`. GATED for MonsterInfo's reason.
struct CampfireInfo {
public:
    unsigned long m_resource : 4;
    unsigned long m_size : 28;
};
SIZE(CampfireInfo, 4);

// The treasure chest's arm. advManager::DoEventTreasure (0x4a6520) fixes
// three lanes: a SIGNED ten-bit artifact id at bits 0..9 (`shl eax,0x16 /
// sar eax,0x16`), the "this chest holds an artifact" flag at bit 10 (`shr
// ecx,0xa / test cl,1`) and a four-bit UNSIGNED gold count at bits 11..14
// (`shr eax,0xb / and eax,0xf`) in units of 500. GATED for MonsterInfo's
// reason.
struct TreasureInfo {
public:
    signed long m_artifact : 10;
    unsigned long m_hasArtifact : 1;
    unsigned long m_gold : 4;
    unsigned long m_tail : 17;
};
SIZE(TreasureInfo, 4);

// The scholar's arm - four lanes, one per award kind plus the selector.
// advManager::DoEventScholar (0x4a4dc0) reads every one of them SIGNED:
// the award at bits 0..2 (`shl ecx,0x1d / sar ecx,0x1d`), the primary
// skill at bits 3..5 (`shl edi,0x1a / sar edi,0x1d`), the secondary skill
// at bits 6..12 (`shl eax,0x13 / sar eax,0x19`) and the spell at bits
// 13..22 (`shl eax,9 / sar eax,0x16`). Only ONE of the three payload lanes
// is ever read on a given visit, which is what the award selects. GATED
// for MonsterInfo's reason.

enum ScholarAwards {
    const_scholar_primary_skill = 0,
    const_scholar_secondary_skill = 1,
    const_scholar_spell = 2
};

struct ScholarInfo {
public:
    signed long m_award : 3;
    signed long m_primary : 3;
    signed long m_secondary : 7;
    signed long m_spell : 10;
    unsigned long m_tail : 9;
};
SIZE(ScholarInfo, 4);

// The shrine's arm, and the only lane of it any retail body writes.
// readObject's (0x502e00) SHRINE1/2/3 arm sign-extends the stream byte
// (`movsx ecx, byte`), masks it to ten bits and shifts it thirteen up,
// clearing the same window first (`and edx, 0xff801fff`) - a SIGNED ten-bit
// field at bits 13..22, which is bit-for-bit where ScholarInfo puts its own
// `spell`. Carried as its own arm rather than borrowed from the scholar's:
// the two objects share a lane, not a record. GATED to the one view that
// deserializes it, for the reason the ScholarInfo note gives.
struct ShrineInfo {
public:
    unsigned long m_head : 13;
    signed long m_spell : 10;
    unsigned long m_tail : 9;
};
SIZE(ShrineInfo, 4);

// MapCell.h's artifact-price domain, published in full by Dreamcast.  It
// lives beside MapArtifactInfo rather than in events.h: the enum is part of
// the packed map-cell representation and is also the return type of
// ExtraInfoUnion::GetArtifactPrice.
enum ArtifactPrices {
    const_free_artifact = 0,
    const_artifact_costs_2000 = 1,
    const_artifact_requires_wisdom = 2,
    const_artifact_requires_leadership = 3,
    const_artifact_costs_2500 = 4,
    const_artifact_costs_3000 = 5,
    const_artifact_defended = 6
};

// events.obj needs the plain packed dword arm plus the object views its
// reconstructed handlers actually read. Keeping this narrow avoids
// importing the remaining eighteen bitfield views into its TU.

// do_event_water_wheel (0x4a7de0) loads bits 0..4 with `mov al,[cell] /
// and eax,0x1f`, multiplies the result by 500 and clears the same bits
// again with `and al,0xe0` after paying - an UNSIGNED five-bit count of
// 500-gold units.
struct type_water_wheel_info {
public:
    unsigned long m_gold : 5;
    unsigned long m_tail : 27;
};
SIZE(type_water_wheel_info, 4);

// do_event_windmill (0x4a7fc0) reads a SIGNED four-bit resource id at
// bits 0..3 (`shl eax,0x1c / sar eax,0x1c`) and an UNSIGNED four-bit
// amount at bits 13..16 (`shr esi,0xd / and esi,0xf`); the payout
// clears both with `and eax,0xfffe1ff0` and re-inserts the resource.
struct type_windmill_info {
public:
    EGameResource m_resource : 4;
    unsigned long m_unused : 9;
    unsigned long m_amount : 4;
    unsigned long m_tail : 15;
};
SIZE(type_windmill_info, 4);

// DoEventLeanTo (0x4a31a0) reads a five-bit id at bits 0..4 (`mov al,
// [cell] / and eax,0x1f`), an UNSIGNED four-bit amount at bits 6..9 and
// an UNSIGNED four-bit resource id at bits 10..13 (`shr eax,N / and
// eax,0xf` both times). Emptying the lean-to rewrites all three fields at
// once - `and eax,0xffffc020 / or eax,id` - which is what pins bit 5 as
// nobody's and puts the tail at 14.
struct type_lean_to_info {
public:
    unsigned long m_id : 5;
    unsigned long m_unused : 1;
    unsigned long m_amount : 4;
    unsigned long m_resource : 4;
    unsigned long m_tail : 18;
};
SIZE(type_lean_to_info, 4);

// DoEventMagicSpring (0x4a3590) shares the same id lane and carries one
// "still full" bit at 6 (`shr eax,6 / test al,1`); drinking clears it
// alone (`and dword ptr [cell], 0xffffffbf`).
struct type_magic_spring_info {
public:
    unsigned long m_id : 5;
    unsigned long m_unused : 1;
    unsigned long m_full : 1;
    unsigned long m_tail : 25;
};
SIZE(type_magic_spring_info, 4);

// DoEventMysticalGarden (0x4a3bc0) shares the id lane but puts a SIGNED
// four-bit resource at bits 6..9 (`shl edi,0x16 / sar edi,0x1c`) and a
// one-bit "still full" flag at bit 10 (`shr eax,0xa / test al,1`);
// emptying it clears that bit alone (`and ah,0xfb`).
struct type_garden_info {
public:
    unsigned long m_id : 5;
    unsigned long m_unused : 1;
    EGameResource m_resource : 4;
    unsigned long m_full : 1;
    unsigned long m_tail : 21;
};
SIZE(type_garden_info, 4);

// do_event_warrior_tomb (0x4a7c30) reads a ONE-BIT occupancy flag at bit 0
// (`test byte ptr [cell],1`) and a SIGNED ten-bit artifact id at bits
// 13..22 (`shl eax,9 / sar eax,0x16`); emptying the tomb clears bit 0
// alone (`and al,0xfe` over the whole dword), so the two lanes are
// separate fields rather than one packed value.
struct type_tomb_info {
public:
    unsigned long m_hasArtifact : 1;
    unsigned long m_unused : 12;
    signed long m_artifact : 10;
    unsigned long m_tail : 9;
};
SIZE(type_tomb_info, 4);

// do_event_witch_hut (0x4a8080) reads a SIGNED seven-bit secondary-skill
// id at bits 13..19 (`shl esi,0xc / sar esi,0x19`) and compares it with
// -1 - the all-ones encoding this header already names
// WitchHutNoSkillMask (0x000fe000), i.e. exactly these seven bits.
struct type_witch_hut_info {
public:
    unsigned long m_unused : 13;
    signed long m_skill : 7;
    unsigned long m_tail : 12;
};
SIZE(type_witch_hut_info, 4);

// The Fountain of Fortune's luck tier, a SIGNED four-bit field at bits
// 13..16. DoEventFountain (0x4a2480) proves both ends of it: the value
// reads are `shl eax,0xf / sar eax,0x1c`, the signature of a signed
// bitfield at bit 13, and the range check that guards the jump table is
// `lea eax,[luck+1] / cmp eax,4 / ja`, i.e. a dense -1..3 domain.
struct type_fountain_info {
public:
    unsigned long m_unused : 13;
    signed long m_luck : 4;
    unsigned long m_tail : 15;
};
SIZE(type_fountain_info, 4);

// The eight-bit team-visibility lane SetCellVisited writes. Proven from
// the READER side here: do_event_warrior_tomb's inlined PlayerKnowsCell
// narrows the AND to one byte (`mov ecx,[cell] / shr ecx,5 / test cl,al`),
// which is only legal because the field is exactly eight bits wide.
struct type_cell_visited_info {
public:
    unsigned long m_unused : 5;
    unsigned long m_visited : 8;
    unsigned long m_tail : 19;
};
SIZE(type_cell_visited_info, 4);

// The Pyramid's guarded flag at bit 0 and signed eight-bit spell lane at
// bits 13..20. Retail's out-of-line set_pyramid merges the two bitfield
// assignments into one dword read-modify-write.
// The pyramid's arm. advManager::do_event_pyramid (0x4a4230) reads a
// one-bit guarded flag at bit 0 - as a plain byte test (`test byte
// ptr [cell],1`), which is what a bool-returning accessor over a bit-0
// field folds to - and a SIGNED eight-bit spell at bits 13..20 (`shl
// esi,0xb / sar esi,0x18`). Emptying it writes both at once: `and
// edx,0xffe01ffe / or edx,(spell & 0xff) << 0xd`, and that mask names the
// two lanes and nothing between them.
struct type_pyramid_info {
public:
    unsigned long m_guarded : 1;
    unsigned long m_unused : 12;
    signed long m_spell : 8;
    unsigned long m_tail : 11;
};
SIZE(type_pyramid_info, 4);

// DoEventWagon (0x4a69b0) packs five lanes into the one dword and the
// arm proves every width: an UNSIGNED five-bit resource amount at bits
// 0..4, read with a BYTE load because the field ends inside the first
// byte (`mov al,[cell] / and eax,0x1f`); a "still loaded" flag at bit 13
// and a "carries an artifact" flag at bit 14, both `shr / test cl,1`
// against the SAME cached dword; a SIGNED ten-bit artifact id at bits
// 15..24 (`shl eax,7 / sar eax,0x16`); and a SIGNED four-bit resource id
// at bits 25..28 (`shl esi,3 / sar esi,0x1c`). Emptying the wagon clears
// bit 13 alone - `and ah,0xdf` over the dword, the same one-byte
// read-modify-write SetGardenEmpty produces at bit 10.
// Original CodeView WagonInfo fields: resource_amount, visited_bits, full,
// has_artifact, artifact, resource. Complete widens the artifact lane to 10 bits.
struct WagonInfo {
    unsigned long m_resourceAmount : 5;
    unsigned long m_visitedBits : 8;
    unsigned long m_full : 1;
    unsigned long m_hasArtifact : 1;
    signed long m_artifact : 10;
    EGameResource m_resource : 4;
    unsigned long m_tail : 3;
};
SIZE(WagonInfo, 4);

// DoEventSkeleton (0x4a5480) - the Corpse, adventure object 22, whose
// per-player flag game.h already names DeadGuyFlags. Three lanes: a
// five-bit UNSIGNED item id at bits 0..4, read with a BYTE load
// (`mov cl,[cell] / and ecx,0x1f`); a SIGNED ten-bit artifact at bits
// 6..15 (`shl eax,0x10 / sar eax,0x16`); and the "still holds something"
// flag at bit 16 (`shr eax,0x10 / test al,1`). Bit 5 belongs to nobody,
// and the emptying write is what proves it: SetSkeleton folds its three
// stores into `and eax,0xfffeffe0 / xor eax,id / or eax,0xffc0`, a mask
// that spares bit 5 while clearing the id lane and bit 16, and an OR
// rather than a masked insert because the artifact is set to -1.
struct type_skeleton_info {
public:
    unsigned long m_id : 5;
    unsigned long m_unused : 1;
    signed long m_artifact : 10;
    unsigned long m_hasTreasure : 1;
    unsigned long m_tail : 15;
};
SIZE(type_skeleton_info, 4);

// Dreamcast CodeView publishes all five MapArtifactInfo fields and their
// order. Complete retains that record but expands the guard lane from eight
// to nine bits for its larger creature domain; AI_value_of_event proves the
// resulting retail positions directly: signed price 0..3, signed guard
// 4..12, signed resource 13..16, a 14-bit guard count, and custom at bit 31.
struct MapArtifactInfo {
public:
    ArtifactPrices m_price : 4;
    TCreatureType m_guard : 9;
    EGameResource m_resourcePrice : 4;
    unsigned long m_guardQty : 14;
    unsigned long m_custom : 1;
};
SIZE(MapArtifactInfo, 4);

// DoEventTreeOfKnowledge (0x4a6710) shares the corpse's five-bit id lane -
// it reads it through the same GetItemId, as the Dreamcast line table
// says at events.cpp:3454 - and adds a SIGNED three-bit price selector at
// bits 13..15 (`shl eax,0x10 / sar eax,0x1d`).
struct type_tree_info {
public:
    unsigned long m_unused : 13;
    signed long m_price : 3;
    unsigned long m_tail : 16;
};
SIZE(type_tree_info, 4);

// Two more retail-used arms of the four-byte union. Both getters extract
// bits 13..24 as a pool index; Dreamcast supplies the arm and field names.
// The other DC arms remain unmodelled until a retail consumer needs them.
struct type_creature_bank_info {
public:
    unsigned long m_unused : 13;
    unsigned long m_index : 12;
    unsigned long m_tail : 7;
};
SIZE(type_creature_bank_info, 4);

struct type_university_info {
public:
    unsigned long m_unused : 13;
    unsigned long m_index : 12;
    unsigned long m_tail : 7;
};
SIZE(type_university_info, 4);

// The sea chest's arm. advManager::DoEventSeaChest (0x4a5030) reads a
// SIGNED three-bit reward kind at bits 0..2 (`shl edi,0x1d / sar
// edi,0x1d`) and a SIGNED ten-bit artifact at bits 3..12 (`shl eax,0x13 /
// sar eax,0x16`) - the artifact lane starts one bit above the selector,
// where the scholar's payload lanes start six and thirteen bits up.
struct SeaChestInfo {
public:
    signed long m_reward : 3;
    signed long m_artifact : 10;
    unsigned long m_tail : 19;
};
SIZE(SeaChestInfo, 4);

// CodeView LF_STRUCTURE 0x2fc4: ExtraInfoUnion is a four-byte struct
// with overlapping arms, not a union type. LF_CLASS 0x1a64 gives NewmapCell
// this public base at +0. Retail passes the cell pointer unchanged to
// SetCellVisited (0x4fbf90) and get_black_box (0x405de0).
struct ExtraInfoUnion {
public:
    union {
        unsigned long m_value;
        MapArtifactInfo m_artifactInfo;
        type_water_wheel_info m_waterWheelInfo;
        type_windmill_info m_windmillInfo;
        type_lean_to_info m_leanToInfo;
        type_magic_spring_info m_magicSpringInfo;
        type_garden_info m_gardenInfo;
        type_tomb_info m_tombInfo;
        type_witch_hut_info m_witchHutInfo;
        type_fountain_info m_fountainInfo;
        type_cell_visited_info m_cellVisitedInfo;
        type_pyramid_info m_pyramidInfo;
        WagonInfo m_wagonInfo;
        type_skeleton_info m_skeletonInfo;
        type_tree_info m_treeInfo;
        ShrineInfo m_shrineInfo;
        type_creature_bank_info m_creatureBankInfo;
        type_university_info m_universityInfo;
        // Original CodeView arm names: extraInfo, monster_info, campfire_info,
        // treasure_info, scholar_info, sea_chest_info, shipyard_info.
        unsigned long m_extraInfo;
        MonsterInfo m_monsterInfo;
        CampfireInfo m_campfireInfo;
        TreasureInfo m_treasureInfo;
        ScholarInfo m_scholarInfo;
        SeaChestInfo m_seaChestInfo;
        ShipyardInfo m_shipyardInfo;
    };
    bool isCustomized() const;
    TCreatureType getArtifactDefender() const;
    ArtifactPrices getArtifactPrice() const;
    bool isDefendedArtifact() const;
    short getCampfireSize() const;
    int getCampfireResource() const;
    enum EGameResource getArtifactResourceCost() const;
    BlackBoxData* getBlackBox() const;
    type_creature_bank& getCreatureBank() const;
    void clearVisitedBits();
    short getCustomIndex() const;
    short getItemId() const;
    bool playerKnowsCell(short player) const;
    void setCellVisited(short player);
    unsigned char gardenIsFull() const;
    enum EGameResource getGardenResource() const;
    void fillGarden(EGameResource resource);
    void setGarden(short id, EGameResource resource);
    void setGardenEmpty();
    int getPyramidSpell() const;
    bool pyramidIsGuarded() const;
    short getLeanToAmount() const;
    int getLeanToResource() const;
    void setLeanTo(short id, short amount, int resource);
    unsigned char magicSpringIsFull() const;
    void fillMagicSpring(unsigned char full);
    void setMagicSpring(short id, unsigned char full);
    void setPyramid(bool guards, int newSpell);
    ScholarAwards getScholarAward() const;
    TPrimarySkill getScholarPrimarySkill() const;
    TSecondarySkill getScholarSecondarySkill() const;
    SpellID getScholarSpell() const;
    void setScholar(ScholarAwards award, TPrimarySkill primary,
                    TSecondarySkill secondary, SpellID spell);
    SpellID getShrineSpell() const;
    int getTreasureArtifact() const;
    short getTreasureSize() const;
    bool treasureIsArtifact() const;
    bool skeletonHasTreasure() const;
    int getSkeletonArtifact() const;
    void setSkeleton(int id, bool hasTreasure, short artifact);
    int getSeaChestReward() const;
    int getSeaChestArtifact() const;
    int getTreePrice() const;
    type_university* getUniversity() const;
    void emptyWagon();
    short getWagonAmount() const;
    int getWagonArtifact() const;
    enum EGameResource getWagonResource() const;
    bool wagonHasArtifact() const;
    bool wagonIsFull() const;
    void setWagon(EGameResource resource, short amount);
    void setWagon(int artifact);
    void emptyTomb();
    int getTombArtifact() const;
    unsigned char tombIsFull() const;
    void setTomb(TArtifact artifact);
    short getWheelGold() const;
    void setWheelGold(short amount);
    short getWindmillAmount() const;
    enum EGameResource getWindmillResource() const;
    void setWindmill(enum EGameResource resource, short amount);
    int getWitchSkill() const;
    void setWitchSkill(TSecondarySkill skill);
};
SIZE(ExtraInfoUnion, 4);

// MapCell.h owns the retained object record and runtime object type.
// DC CodeView supplies the three field names and order. Retail widens the
// STL string from 12 to 16 bytes, then proves the flag at +0x10 and the
// armyGroup at +0x14 in NewfullMap::saveTreasureData; 0x14 + 0x38 closes
// the independently proven 0x4c vector stride exactly. The three alignment
// bytes before Guardians stay implicit so generated copies skip them.
class TreasureData {
public:
    std::basic_string<char, std::char_traits<char>, std::allocator<char> > m_message;
    unsigned char m_hasCustomGuardians;
    armyGroup m_guardians;
    // E:\gamedcs\MapCell.h:333, dc 0xf4790
    TreasureData() : m_hasCustomGuardians(0) {}
};
SIZE(TreasureData, 0x4c);

// DC CodeView names every field in the derived record. Retail independently
// proves their +4-shifted offsets (its Dinkumware string/vector objects are
// four bytes wider than STLport's) through BlackBoxData's destructor.
// Preserve the CodeView TSecondarySkill/TSkillMastery member types.
// loadBlackBox widens each serialized byte into its four-byte enum slot.
struct SecondarySkillData {
public:
    TSecondarySkill m_type;
    TSkillMastery m_level;
};
SIZE(SecondarySkillData, 8);

class BlackBoxData : public TreasureData {
public:
    unsigned char m_hasCustomTreasure;  // +0x4c
    int m_experienceBonus;  // +0x50
    int m_manaBonus;  // +0x54
    signed char m_moraleBonus;  // +0x58
    signed char m_luckBonus;  // +0x59
    int m_resQty[7];  // +0x5c
    signed char m_primarySkillBonus[4];  // +0x78
    std::vector<SecondarySkillData> m_secondarySkills;  // +0x7c
    // CodeView preserves vector<TArtifact> and vector<SpellID>.
    // loadBlackBox widens the serialized identifiers at the read boundary.
    std::vector<TArtifact> m_artifacts;  // +0x8c
    std::vector<SpellID> m_spells;  // +0x9c
    armyGroup m_creatures;  // +0xac

    // loadBlackBoxList's resize temp proves the constructor: after the
    // TreasureData base and the three vectors have run their own, the only
    // remaining store is a zero into +0x4c.
    // E:\gamedcs\MapCell.h:364, dc 0xf47cc
    BlackBoxData() : m_hasCustomTreasure(0) {}

    // Implicit destructor; CodeView dc 0xf4bfc compgenx.
};
SIZE(BlackBoxData, 0xe4);

class TAbstractFile;

// Dreamcast CodeView supplies the shared member names. Retail widens the
// leading STL string to 16 bytes and adds ApplyToHuman before the DC-attested
// ApplyToComputer byte; TTimedEvent::Save/Load prove both bytes and every
// remaining offset, while saveTimedEventList closes the 0x34-byte stride.
// The alignment byte before FirstTime is deliberately implicit: naming it
// makes VC6's generated copies treat retail padding as a real member.
class TTimedEvent {
public:
    std::basic_string<char, std::char_traits<char>, std::allocator<char> > m_message;
    int m_resQty[7];
    unsigned char m_playerFlags;
    unsigned char m_applyToHuman;
    unsigned char m_applyToComputer;
    unsigned short m_firstTime;
    unsigned short m_interval;
    // `ret 8`: the save version is a second argument, gating the
    // apply-to-human flag at 28 exactly as LoadGarrisonPool does.
    int read(TAbstractFile* infile, int saveVersion);
    int save(TAbstractFile* outfile);
    int load(TAbstractFile* infile, int saveVersion);
};
SIZE(TTimedEvent, 0x34);

// DC CodeView names the derived town-event payload. Retail's four-byte-wider
// base shifts the three fields to +0x34/+0x38/+0x40; saveTownEventList proves
// those offsets, the seven-word generator band, and the 0x50-byte stride.
// Alignment before BuildBuildings and the tail rounding are likewise left
// implicit so generated copies do not copy padding bytes.
class TTownEvent : public TTimedEvent {
public:
    signed char m_townNum;
    __int64 m_buildBuildings;
    unsigned short m_generatorBonuses[7];
    // MapCell.h:400 in the DC roster - a header-inline default constructor.
    // loadTownEventList's resize temp proves its whole body: after the base
    // string is tidied it zeroes the eight bytes of BuildBuildings and
    // nothing else, TownNum and the generator band staying uninitialized.
    // E:\gamedcs\MapCell.h:400, dc 0xf48d8
    TTownEvent() { m_buildBuildings = 0; }
    // Complete forwards the additional version argument to the base reader.
    int read(TAbstractFile* infile, int mapVersion);
    int save(TAbstractFile* outfile);
    int load(TAbstractFile* infile, int saveVersion);
};
SIZE(TTownEvent, 0x50);

struct TObjectType;
// Canonical Random declaration needed by CObject's inline construction.
int __fastcall random(int minimum, int maximum);

class CObjectType {
public:
    // Complete adds the conversion constructor at 0x506080. Its user
    // declaration suppresses implicit default construction, so C++98 needs
    // this written empty default for the existing resize temporaries.
    // DC class 0x309b has only generated default/copy constructors (0x103).
    CObjectType() {}
    // Dreamcast retains an out-of-line copy, while Complete
    // expands this header helper at the view-world draw-cell test.
    // E:\gamedcs\MapCell.h:565, dc 0x1f958
    MAC_ADDRESS(0x127c38, 0x14)
    static unsigned getBitPos(unsigned x, unsigned y)
    {
        return 47 - y * 8 - x;
    }
    CObjectType(TObjectType* source);  // 0x506080
    // The DC field list names every member of this record - ImageName,
    // Width, Height, then the FOUR 48-cell masks PlacementMask,
    // PassableMask, ShadowMask, TriggerMask, then Type/Extra/IsUnderlay -
    // at DC offsets 0/12/13/16/24/32/40/48/52/56. Retail widens the leading
    // string from 12 to 16 bytes and every offset after it moves by four,
    // which saveObjectType then confirms one Write at a time.
    std::basic_string<char, std::char_traits<char>, std::allocator<char> >
        m_imageName;
    // Dreamcast field list 0x309c records Width/Height as T_RCHAR.
    char m_width;
    char m_height;
    // +0x12..+0x13 is alignment before the first bitset.  Keep it implicit:
    // retail's generated assignment skips these bytes.
    std::bitset<48> m_drawCells;
    std::bitset<48> m_passableCells;
    std::bitset<48> m_shadowCells;
    // Fourth 48-cell mask, byte-proven at +0x2c by FindTrigger. The prior
    // padding spelling incorrectly conflated it with shadowCells at +0x24.
    std::bitset<48> m_triggerCells;
    // +0x34, FOUR bytes and unchanged in layout, but not padding: the
    // default constructor CObjectType's `resize` temporary runs calls SIX
    // sub-constructors, and the sixth targets this slot through
    // std::bitset<10>::_Tidy at 0x506880 (`and eax,0x3ff` - _Trim with
    // 10 % 32 = 10), where the four masks above go through the
    // bitset<48> _Tidy at 0x4e66c0. `char pad_34[4]` emits five and cannot
    // produce the sixth. It also resolves the Dreamcast offset arithmetic:
    // DC's Type at 48 maps to retail 52 = 0x34, yet saveObjectType
    // byte-proves objectType at 0x38 - retail inserted one 4-byte member
    // the DC record does not have, and this is it.

    // The mask's MEANING is unproven and its name is deliberately ordinal:
    // no serializer in this compiland reads or writes it, readObjectType
    // included.
    std::bitset<10> m_mask34;
    // loadObjectType stores one full dword at +0x38. A scalar preserves
    // that field and its single generated copy; no alternative view exists.
    TAdventureObjectType m_objectType;
    int m_extra;
    unsigned char m_suppressDraw;
    // +0x41 remains implicit alignment, but retail's generated assignment
    // explicitly copies a word at +0x42; the old pad_41[3] hid that real
    // field and also made VC6 copy the otherwise-skipped +0x41 byte.
    unsigned short m_objectTypeIndex;
};
SIZE(CObjectType, 0x44);

// CodeView CObject records 0x30aa and 0x6401 both give the public
// ExtraInfoUnion base at +0, x/y/z at +4/+5/+6, TypeID at +8 and frameOffset
// at +0xa. Complete's readers/writers use the same payload and offsets.
class CObject : public ExtraInfoUnion {
public:
    unsigned char m_x;
    unsigned char m_y;
    unsigned char m_z;
    unsigned char m_paddingBeforeTypeId;
    unsigned short m_typeIndex;
    unsigned char m_animationOffset;
    unsigned char m_paddingAfterFrameOffset;
    void findTrigger(int& resultX, int& resultY) const;
    // MapCell.cpp:1119/1131. Dreamcast publishes both members as const;
    // FindTrigger's AAH parameters are references, and get_trigger is the
    // source helper which retail expands into get_trigger_cell.
    type_point getTrigger() const;
    CObjectType* getObjectTypePtr() const;
    TAdventureObjectType getType() const;
    // Original: CObject::CObject; MapCell.h:587, dc 0xf4944.
    // loadMapObjects' vector resize expands the distinct default ctor;
    // the recorded overload is not a five-argument ctor with defaults.
    CObject() : m_x(0xff), m_y(0xff), m_z(0xff), m_typeIndex(0xffff)
    {
        m_extraInfo = 0xffffffff;
        m_animationOffset = static_cast<unsigned char>(random(0, 255));
    }
    // Original: CObject::CObject; MapCell.h:595, dc 0xbc868.
    // game::InsertObject expands this coordinate/type/extra-info overload.
    CObject(unsigned char newX, unsigned char newY, unsigned char newZ,
            unsigned short newType, unsigned long newExtraInfo)
    {
        m_x = newX;
        m_y = newY;
        m_z = newZ;
        m_typeIndex = newType;
        m_extraInfo = newExtraInfo;
        m_animationOffset = static_cast<unsigned char>(random(0, 255));
    }
};
SIZE(CObject, 0xc);

// Stride 38 (0x26), byte-proven by game::get_cell's `*19` then `*2`
// address math. A 38-byte record cannot be 4-aligned, which is also how
// the 4-byte object-type field lands at the odd-dword offset 0x1e that
// find_magus_hut_value reads - hence the pack. The DC layout (36 B, an
// STLport vector at +16) does not transfer; only the fields a retail
// reader touches are named.
#pragma pack(push, 1)
class NewmapCell : public ExtraInfoUnion {
public:
    // The inherited extra-info dword occupies +0x00. Retail's arena and
    // shipyard readers use this same base address (0x4e53c0 / 0x4e53e0
    // and PlayerDead); the typed arms belong to ExtraInfoUnion above.
    // +0x04 and +0x08: two int allocation units of SIGNED 8-BIT
    // BITFIELDS, sliced 2026-08-08 out of the old `unsigned char
    // terrain; char pad_05[7]` pair. findpath's CalcTerrainCost
    // (0x4b1740) reads BOTH units as `mov eax, dword; shl eax, 0x18;
    // sar eax, 0x18` - a 32-bit load whose low byte is sign-extended,
    // which is what VC6 emits for a signed 8-bit bitfield at bit 0 of
    // an int unit and NOT what it emits for a `char` member (that
    // would be `movsx eax, byte`). The bit-0 field of the first unit
    // is the terrain id the old `terrain` byte named:
    // check_shipyard_square (town.obj 0x5c0c90) accepts a dock tile
    // only when it equals 8 and terrain.h's ten-mask permutation pins
    // Water at TTerrainType 8. Names and the six-way split are the
    // Dreamcast fieldlist's - GroundSet/GroundIndex/RiverSet/RiverIndex
    // all at DC offset 4, RoadSet/RoadIndex at DC offset 8 - and retail
    // corroborates the two the code reaches: GroundSet is the terrain
    // and RoadSet indexes findpath's four-entry road-row table.
    // GroundSet stays a plain `int` bitfield rather than TTerrainType:
    // that enum is declared in armygrp.h, which INCLUDES this header,
    // so the domain type is not nameable from here. findpath spells
    // CalcTerrainCost's matching parameter `long` for the same reason
    // its three mastery parameters are `long` (see the note there).
    // The equality compare in check_shipyard_square stays a BYTE
    // compare through the bitfield (measured, still exact): VC6 folds
    // `field == small_constant` on a byte-aligned 8-bit field into
    // `cmp byte ptr`, so the slice costs nothing there.
    int m_groundSet : 8;
    int m_groundIndex : 8;
    int m_riverSet : 8;
    int m_riverIndex : 8;
    int m_roadSet : 8;
    int m_roadIndex : 8;
    // Dreamcast gives RoadSet/RoadIndex eight bits each in the unit
    // at +8, then starts cellFlags at +0xc. NH3API confirms that +0xa
    // and +0xb are unused. Retain the retail-proven int allocation unit.
    int m_paddingAfterRoadIndex : 16;
    // +0x0c is the DC's cellFlags word. find_magus_hut_value tests bit
    // 12 of it (`test byte ptr [cell + 0xd], 0x10`); the DC's flag list
    // for this word has is_trigger thirteenth, so that is the name used
    // here - provisional, the bit ORDER is inferred from listing order.
    // Bit 6 is `Passable`, the seventh name on the same DC list, and
    // CalcTerrainCost (0x4b1740) corroborates the position
    // independently: it only lets a flier take min(ground cost, flight
    // cost) on a tile whose bit 6 is set (`test byte [cell+0xc],
    // 0x40`) and charges the flat flight cost everywhere else.
    // NOT SLICED OUT, and the reason is the include-set sensitivity
    // class again, MEASURED 2026-08-08: splitting this one bitfield
    // into `flags_00_05:6 / Passable:1 / flags_07_11:5` - three named
    // members within one existing record, with no new type or semantic change -
    // takes initialize_game_data 96.0880 -> 26.1806, one of the values
    // that class's own struct sweep produced. So the sensitivity is to
    // the MEMBER population of this header's types, not only to the
    // count of type definitions in the TU; the note below was written
    // when only the latter had been measured. findpath spells the test
    // `cell->flags_00_11 & 0x40` instead, which compiles to retail's
    // `test byte ptr [cell+0xc], 0x40` unchanged.
    // Retail check_shipyard_square reads this field as a whole word
    // (`mov si, word ptr [cell+0xc]`), and get_special_terrain tests it
    // with `test word ptr [cell+0xc], 0x1000`. Both use the canonical
    // cellFlags overlay below. Earlier include-set experiments predated
    // this shared representation and do not justify a local offset view.
    union {
        struct {
            unsigned short m_flags0011 : 12;
            unsigned short m_isTrigger : 1;
            unsigned short m_flags1315 : 3;
        };
        // readMapLayer needs the low ten bits as individual declarators: it
        // writes the six map-format flip flags ONE BIT AT A TIME (VC6 merges
        // them into a single word RMW that preserves bits 6..15), and then
        // sets Passable / IsBlocked / IsBeachBorder / Animated as separate
        // single-bit stores - `or byte ptr [cell+0xd], 0x2` for
        // IsBeachBorder is a 1-bit field write, not a word mask.

        // The bit POSITIONS are stated Dreamcast evidence, not inferred from
        // listing order: the CodeView LF_BITFIELD records 0x3E16..0x3E22
        // each carry an explicit `starting position` (0, 1, ... 12), which
        // retires the "bit ORDER is inferred from listing order" caveat
        // above for the twelve bits named here. MoveHero's inlined
        // mark_shipyards/clear_shipyards pair reaches bit 11 directly, so
        // the DC `can_build_ship` name is now admitted as well.
        struct {
            unsigned short m_groundFlippedHorizontal : 1;
            unsigned short m_groundFlippedVertical : 1;
            unsigned short m_riverFlippedHorizontal : 1;
            unsigned short m_riverFlippedVertical : 1;
            unsigned short m_roadFlippedHorizontal : 1;
            unsigned short m_roadFlippedVertical : 1;
            unsigned short m_passable : 1;
            unsigned short m_animated : 1;
            unsigned short m_isBlocked : 1;
            unsigned short m_isBeachBorder : 1;
            unsigned short m_unusedBit : 1;
            unsigned short m_canBuildShip : 1;
            unsigned short m_flags1215 : 4;
        };
        unsigned short m_cellFlags;
    };
    // Retail mapcell.obj constructs and destroys a Dinkumware vector here.
    // Its empty allocator occupies +0x0e..+0x11 and its first/last/end
    // pointers are the three dwords at +0x12/+0x16/+0x1a. The four-byte
    // element stride is independently proven by the object renderers.

    // events.obj joins the view for advManager::EraseObj (0x4aabb0), which
    // walks this vector element by element looking for the object it is
    // erasing and then splices it out - `_First` at +0x12, `_Last` at
    // +0x16, a four-byte stride and the SIXTEEN-BIT objectIndex compare are
    // all in that one body.
    struct TObjectCell {
        // denoted the same unsigned word, not alternative representations.
        unsigned short m_objectIndex;
        union {
            unsigned char m_offsets;
            struct {
                signed char m_cellX : 4;
                signed char m_cellY : 4;
            };
        };
        signed char m_layer;

        CObject* getObject() const;
    };
    std::vector<TObjectCell> m_objects;
    // loadMapLayer stores the serialized object type as a FULL DWORD: the
    // stream carries two bytes and retail widens them into all four of
    // +0x1e..+0x21 (`mov ecx, dword; and ecx, 0xffff; mov dword [cell+0x1e],
    // ecx`). Spelled as an overlay of the same proven field rather than a
    // cast into the enum domain - the cellFlags word above is modelled the
    // same way and for the same reason, and a cast here would be the tree's
    // first cast into an enum.
    union {
        TAdventureObjectType m_type;  // +0x1e
        unsigned long m_typeValue;
    };
    short m_objectIndex;  // +0x22
    short m_objectTypeIndex;  // +0x24

    // The retained vector-construction callback belongs to this header body;
    // mapcell.obj is its retail emission site, after NewfullMap::Init.
    // E:\gamedcs\MapCell.h:685, dc 0xf49a4
    VA(0x004fd650, 0x3E)  // dc 0xf49a4
    NewmapCell()
    {
        m_groundSet = 0;
        m_groundIndex = 0;
        m_riverSet = 0;
        m_riverIndex = 0;
        m_roadSet = 0;
        m_roadIndex = 0;
        m_flags0011 = 0;
        m_isTrigger = 0;
        m_flags1315 = 0;
        m_type = NOTHING;
        m_objectIndex = -1;
        m_extraInfo = 0;
        m_objectTypeIndex = -1;
    }
    TAdventureObjectType getSpecialTerrain() const;
    // Implicit destructor; CodeView dc 0xf4bdc compgenx.
    const unsigned char hasTriggerableEvent() const;
    int getMagicTerrainType();
    unsigned char isDiggable() const;
    TAdventureObjectType getMapObject() const;
    unsigned long getMapExtraInfo() const;
    unsigned char cellIsTrigger() const;
    TArtifact getArtifactIndex() const;
    NewmapCell* getTriggerCell();
};
#pragma pack(pop)

// Dreamcast CodeView supplies the member names and order. Retail keeps the
// seven-dword resource array but widens the leading STL string from 12 to 16
// bytes, placing Artifact at +0x2c; saveMonsterData independently proves both
// the array base/extent and that final offset.
// The map file's five wandering-monster quantity presets.  PROVISIONAL
// NAMES: no Dreamcast enum covers this domain, so each is named for the roll
// readMonsterData performs on it (0x5013b0's five-arm jump table).  Grade 0
// stores the sentinel -4 and is resolved elsewhere.
enum EMonsterQuantityPreset {
    MONSTER_QTY_UNRESOLVED = 0,
    MONSTER_QTY_RANDOM_1_7 = 1,
    MONSTER_QTY_RANDOM_1_10 = 2,
    MONSTER_QTY_RANDOM_4_10 = 3,
    MONSTER_QTY_FIXED_10 = 4
};

class MonsterData {
public:
    std::basic_string<char, std::char_traits<char>, std::allocator<char> > m_message;
    int m_resQty[7];
    // Spelled int, not TArtifact, for the reason armyGroup::armies is
    // spelled int: readMonsterData deserializes it from a one- or two-byte
    // stream field and saveMonsterData narrows it back to a byte, so an
    // enum here would put a cast on every crossing.  ARTIFACT_NONE still
    // assigns.  The Dreamcast declarator's enum is preserved in the name.
    int m_artifact;
    // E:\gamedcs\MapCell.h:735, dc 0xf4a50
    MonsterData() { m_artifact = ARTIFACT_NONE; }
};
SIZE(MonsterData, 0x30);

// 0x4fe6c0, RETAIL-ONLY: no Dreamcast function counterpart exists (the DC
// roster for mapcell.cpp is exhausted), so the NAME remains provisional and
// is taken from the now-reconstructed body. loadMapLayer hands it every cell
// it finishes, together with the save version, and it returns at once unless
// that version is below 25; older saves go through a seven-arm jump table on
// the cell's object type that repacks extraInfo's bitfields into their current
// positions. The body is enrolled as a documented compiler wall.

// A free function, so /Gr makes it __fastcall - which is what retail emits:
// the cell in ECX, the save version in EDX.
void upgradeCellExtraInfo(NewmapCell* cell, int saveVersion);

// Retail .rdata 0x660428 stores a pointer to sixteen bytes per adventure-
// object type. can_land proves byte zero as the trigger-object landing veto;
// the remaining bytes stay opaque.

// --- type_obscuring_object ---

// MapCell.h owns the map container and its inline accessors. These object
// records are complete at the out-of-line constructor/destructor definitions.
class CObjectType;
class CSprite;
class TreasureData;
class MonsterData;
class TSeerHut;
class TQuestGuard;
class TTimedEvent;
class TTownEvent;
struct HeroPlaceholderData;
struct RandomDwellingData;
class CMapObjectData;

class NewfullMap {
public:
    // advManager::EraseObj indexes objects with a TWELVE-byte stride out of
    // +0x14 and objectTypes with a SIXTY-EIGHT-byte one out of +0x04, which
    // is sizeof(CObject) and sizeof(CObjectType) exactly; the three vectors
    // fill the same 0x30 the pad did.
    std::vector<CObjectType> m_objectTypes;  // +0x00, first at +0x04
    std::vector<CObject> m_objects;  // +0x10, first at +0x14
    std::vector<CSprite*> m_sprites;  // +0x20
    std::vector<TreasureData> m_customTreasure;  // +0x30, first at +0x34
    // +0x40, first at +0x44. DoWanderingMonsterResult indexes it with the
    // cell's eight-bit custom-record field and a 48-byte stride, which is
    // exactly sizeof(MonsterData) once the Dinkumware string is 16 wide;
    // the record's own +0x04/+0x08 are that string's _Ptr and _Len. The
    // DC roster names the member CustomMonsterList and carries the
    // matching std::vector<MonsterData> operator[]/begin rows in
    // events.obj.
    std::vector<MonsterData> m_customMonsterList;
    std::vector<BlackBoxData> m_blackBoxes;  // +0x50, first at +0x54
    std::vector<TSeerHut> m_seerHutList;  // +0x60
    std::vector<TQuestGuard> m_questGuardList;  // +0x70
    std::vector<TTimedEvent> m_timedEventList;  // +0x80
    std::vector<TTownEvent> m_townEventList;  // +0x90
    // +0xa0 and +0xc0, sliced by readObject: it appends a sixteen-byte
    // record to each through push_back's `insert(_Last, 1, x)`, reading
    // _Last at +0xa8 and +0xc8.
    std::vector<HeroPlaceholderData> m_heroPlaceholders;
    std::vector<CMapObjectData*> m_mapObjectData;  // +0xb0
    std::vector<RandomDwellingData> m_randomDwellings;  // +0xc0

private:
    NewmapCell* m_cellData;
    int m_size;
    unsigned char m_hasTwoLevels;

public:
    std::vector<CObjectType> m_objectTypeIndex[232];
    // DC records this public const MapCell.h accessor; retail callers
    // read the same size member used by zCell's row and level strides.
    // Retail callers read the size used by the map stride calculations.
    // The DC declaration survives, but no body source location does.
    // Header ownership is provisional; no source order is claimed.
    // @dc-declaration-only: 0x345c
    int getMapSize() const { return m_size; }
    const NewmapCell* cell(int x, int y, int z) const;
    NewmapCell* cell(int x, int y, int z);
    NewmapCell* cell(type_point point);
    int getNumLevels();

private:
    const NewmapCell* zCell(int x, int y, int z) const;
    NewmapCell* zCell(int x, int y, int z);

public:
    int load(TAbstractFile* infile, int size, unsigned char twoLayers,
             int saveVersion);
    int save(TAbstractFile* outfile, int size, unsigned char twoLayers);
    // `ret 0x10`: FOUR arguments, one more than Save's three. The fourth is
    // the map version, and Read forwards it verbatim to readMapObjects and
    // readTimedEventList and reads it nowhere else.
    int read(TAbstractFile* infile, int size, unsigned char twoLayers,
             int mapVersion);

private:
    int readMapObjects(TAbstractFile* infile, int mapVersion);
    // `ret 4`: ONE argument, unlike readMapObjects' two - the save stream
    // carries no map version.
    int loadMapObjects(TAbstractFile* infile);

public:
    // 0x4fd950, `ret 8`. One of the four retail-only rows this compiland's
    // span audit already flags as having no Dreamcast counterpart; Load
    // reaches it, and only when the save version is at least 25.
    void loadQuestGuardList(TAbstractFile* infile, int saveVersion);
    // 0x5042c0, nullary. Reached by BOTH readMapObjects and loadMapObjects,
    // right after the object-type list is deserialized. It rebuilds the
    // per-class object-type index: the 232-entry array of vectors at +0xdc.
    // This role-based name is provisional: the Dreamcast roster has no
    // corresponding procedure between loadObjectType and readMapObjects.
    void rebuildObjectTypeIndex();
    void soDTransformRandomDwellings();
    void loadShipyards();
    int readObjectType(TAbstractFile* infile, CObjectType& objectType);
    int saveObjectType(TAbstractFile* outfile, CObjectType* objectType);
    int loadObjectType(TAbstractFile* infile, CObjectType* objectType);
    int saveObject(TAbstractFile* outfile, CObject& tempObject);
    int loadObject(TAbstractFile* infile, CObject* object);

private:
    void init(int size, unsigned char twoLayers);
    void close();  // Original: Close, mapcell.cpp:537, dc 0xec724.
    // `ret 0xc`: the layer index is the third argument, and the return is
    // the cell count (size * size), not a status.
    int readMapLayer(TAbstractFile* infile, int size, int layer);
    int saveMapObjects(TAbstractFile* outfile);
    // `ret 0x10`: a fourth argument, the save version, which reaches only
    // the per-cell upgrade pass.
    int loadMapLayer(TAbstractFile* infile, int size, int layer,
                     int saveVersion);

public:
    int readTreasureData(TAbstractFile* infile, TreasureData* treasure);
    int saveBlackBox(TAbstractFile* outfile, BlackBoxData* thisBox);
    int saveTreasureData(TAbstractFile* outfile, TreasureData* treasure);
    int saveMonsterData(TAbstractFile* outfile, MonsterData* monster);

private:
    int saveMapLayer(TAbstractFile* outfile, int size, int layer);

public:
    int loadBlackBoxList(TAbstractFile* infile, int saveVersion);
    int loadBlackBox(TAbstractFile* infile, BlackBoxData* thisBox,
                     int saveVersion);
    int loadMonsterList(TAbstractFile* infile);
    int loadSeerList(TAbstractFile* infile, int saveVersion);
    int saveSeerList(TAbstractFile* outfile);
    void saveQuestGuardList(TAbstractFile* outfile);
    // `ret 8`: the save version rides along to TTimedEvent::Read.
    int readTimedEventList(TAbstractFile* infile, int saveVersion);
    int loadTimedEventList(TAbstractFile* infile, int saveVersion);
    int saveTimedEventList(TAbstractFile* outfile);
    int saveTownEventList(TAbstractFile* outfile);
    int saveBlackBoxList(TAbstractFile* outfile);
    // Canonical mapcell.cpp members, expanded by Complete's save/load drivers.
    // CodeView source lines: 1293, 1362, 1729, 2695 and 2768 respectively.
    int saveTreasureList(TAbstractFile* outfile);
    int loadTreasureData(TAbstractFile* infile, TreasureData& thisTreasure);
    int saveMonsterList(TAbstractFile* outfile);
    int loadMonsterData(TAbstractFile* infile, MonsterData& thisMonster);
    int loadTownEventList(TAbstractFile* infile, int saveVersion);
    // Ordinary mapcell.cpp readers expanded in Complete's readObject.
    // DC source lines 1095, 1199, 1224 and 2383; pointer object parameters.
    int readBoatData(TAbstractFile* infile, CObject* boatObject);
    int readHolyGrailData(TAbstractFile* infile, CObject* grailObject);
    int readShrineData(TAbstractFile* infile, CObject* shrineObject);
    int readShipyardData(TAbstractFile* infile, CObject* shipyardObject);
    int readArtifactData(TAbstractFile* infile, CObject* artifactObject);
    int readResourceData(TAbstractFile* infile, CObject* resourceObject);
    int readGeneratorData(TAbstractFile* infile, CObject* object);
    int readSpellScrollData(TAbstractFile* infile, CObject* scrollObject);
    // Three arguments, readGarrisonData's divergence again: retail's `ret 0xc`
    // against the Dreamcast's two, and mapVersion again picks the creature
    // field's width. readBlackBoxData carries it only to pass it through.
    int readBlackBox(TAbstractFile* infile, BlackBoxData* thisBox,
                     int mapVersion);
    int readBlackBoxData(TAbstractFile* infile, CObject* blackboxObject,
                         int mapVersion);
    int readEventData(TAbstractFile* infile, CObject* eventObject,
                      int mapVersion);
    int readMonsterData(TAbstractFile* infile, CObject* monsterObject);
    int readSeerData(TAbstractFile* infile, CObject* seerObject);
    int readScholarData(TAbstractFile* infile, CObject* scholarObject);
    void readHeroPlaceholderData(TAbstractFile* infile, CObject* object);
    void readRandomDwellingData(TAbstractFile* infile, CObject* object);
    void readRandomDwellingLevelData(TAbstractFile* infile, CObject* object);
    void readRandomDwellingFactionData(TAbstractFile* infile, CObject* object);
    void readQuestGuardData(TAbstractFile* infile, CObject* object);
    // The map-object dispatcher. `ret 0xc`: three arguments, and the third
    // is the map version every version-sensitive reader below takes - it is
    // forwarded verbatim to readTownData, readHeroData, readEventData,
    // readBlackBoxData and readGarrisonData and read nowhere else.
    int readObject(TAbstractFile* infile, CObject* tempObject, int mapVersion);
    // Three arguments, readGarrisonData's divergence again: readObject
    // pushes the map version to both of these as well.
    int readTownData(TAbstractFile* infile, CObject* townObject,
                     int mapVersion);
    int readHeroData(TAbstractFile* infile, CObject* heroObject,
                     int mapVersion);
    int readGarrisonData(TAbstractFile* infile, CObject* garrisonObject,
                         int mapVersion);
    // 0x505a10. advManager::EraseObj re-derives every touched cell's extra
    // info through it once the object's entry has been spliced out.
    void calculateCellExtra(NewmapCell* cell, unsigned char setExtraInfo);
    int readSignData(TAbstractFile* infile, CObject* object);
    int readMineData(TAbstractFile* infile, CObject* object);
    int readAbandonedMineData(TAbstractFile* infile, CObject* object);
    int loadTreasureList(TAbstractFile* infile);

private:
    void calcCellExtra(NewmapCell* cell, unsigned char setExtraInfo);

public:
    // Role-based names. DoCombat (0x4ad470) broadcasts the defeated hero
    // and winning player at 0x4ae45a/0x4ae483; monster removal broadcasts
    // the defeated/joined/fled stack's location. Quest slots +0x24/+0x28
    // implement the same notifications (hero_defeated/monster_defeated).
    void notifyHeroDefeated(int heroId, int player);
    void notifyMonsterDefeated(type_point point, int player);
    void loadObjectTypeTemplates();
    // Retail-only helper at 0x505f20. Its behavior selects or appends the
    // matching object-type/sprite pair and writes the resulting type index.
    // No surviving symbol names it; setObjectType describes its retail role.
    void setObjectType(CObject* object, int objectType,
                               int objectIndex, int terrain);
    // Retail-only helper at 0x505ea0, used by ConvertObject and hiscore.
    // It scans m_objectTypeIndex[objectType] backwards for a matching extra
    // and returns its address. The array is a Complete addition absent in
    // CodeView NewfullMap type 0x3450. No surviving symbol names the helper;
    // findObjectType is a provisional role-based name.
    CObjectType* findObjectType(int objectType, int extra);
    NewfullMap();
    ~NewfullMap();
    void stampObject(NewmapCell* cell, NewmapCell::TObjectCell* objectCell);
    void generateHeightMap(const CObject* object, signed char heightMap[8][6]);
    int placeObject(int objectIndex, unsigned char setExtraInfo);
    int placeObjects();
};

// Canonical inline definitions in Dreamcast MapCell.h source-line order.

// MapCell.h:769 in the DC roster (dc 0x2e48), i.e. a header inline of
// this class - and retail keeps no out-of-line row for it either.
// advManager::ProcessDeSelect's elevation-toggle arm expands it in
// place: `movzx edx,[gpGame+0x1fc48] / inc edx / cmp edx,1 / jle`, the
// zero-extended flag plus one, tested against one. Gated to the
// compilation personalities whose call sites prove the expansion
// (victorylossconditions' z bound in CheckForDefeatedMonsterWin is
// the same movzx/inc shape, 2026-08-20).
inline int NewfullMap::getNumLevels() { return m_hasTwoLevels + 1; }

// Const route lookup retains the recovered helper and level arithmetic.
// E:\gamedcs\MapCell.h:847, dc 0xbc8dc
inline const NewmapCell* NewfullMap::zCell(int x, int y, int z) const
{
    return m_cellData + x + y * m_size + z * m_size * m_size;
}

// MapCell.h:850, dc 0x1f974. This worker reproduces all 49 retail bytes
// at 0x408770, including the boat callers' retained zero-coordinate lookup.
// The former scalar-cell claim incorrectly distinguished 49 x86 bytes from
// 82 SH4 bytes and alleged a zCell bounds test absent on both platforms.
// Identical folded bodies cannot prove a unique original retail symbol;
// this annotation owns the emitted canonical worker, not a renamed wrapper.
VA(0x00408770, 0x31)  // exact body + anchor-callees, dc 0x1f974
inline NewmapCell* NewfullMap::zCell(int x, int y, int z)
{
    return m_cellData + x + y * m_size + z * m_size * m_size;
}

// E:\gamedcs\MapCell.h:889, dc 0xbc930
inline const NewmapCell* NewfullMap::cell(int x, int y, int z) const
{
    return zCell(x, y, z);
}

// THE TWO MAP-SQUARE ACCESSORS (DC MapCell.h:895/906).

// cell(type_point) has NO retail body - the DC row at dc 0x1f9f4 is the
// WinCE build's out-of-line copy of a header inline - so it is a header
// inline for EVERY compiland. DC line 907 calls zCell directly.

// DC MapCell.h:897 calls zCell. The unrecorded line 896 does not prove
// a release VERIFY. Removing the inferred storage check preserves this
// helper chain and restores the boat callers' retail expansion decisions;
// the retained 49-byte arithmetic body is owned by zCell above.
inline NewmapCell* NewfullMap::cell(int x, int y, int z)
{
    return zCell(x, y, z);
}

inline NewmapCell* NewfullMap::cell(type_point point)
{
    return zCell(point.m_x, point.m_y, point.m_z);
}

// public decoration is `?PlayerKnowsCell@ExtraInfoUnion@@QBA_NF@Z`),

// The visited-player mask is eight bits at +5.
VA(0x00529690, 0x33)  // hd-crossbuild + anchor-callee x3, dc 0x1fa40
inline bool ExtraInfoUnion::playerKnowsCell(short player) const
{
    if (player < 0 || player >= 8)
        return 0;
    return (m_cellVisitedInfo.m_visited & (1 << player)) != 0;
}

// MapCell.h:923-945. These are source-real accessors, not convenience
// wrappers: Dreamcast publishes their decorated signatures and bodies,
// while Complete inlines them into the artifact event/appraisal paths.
// DoWanderingMonsterResult (0x4a7740) also calls this inherited accessor.
// A direct monster-info bitfield test folded to test eax,0x80000000;
// the value-returning helper preserved retail's shr/test of the top bit.
inline bool ExtraInfoUnion::isCustomized() const { return m_artifactInfo.m_custom != 0; }

inline TCreatureType ExtraInfoUnion::getArtifactDefender() const
{
    return H3_ENUM_DECODE(TCreatureType, m_artifactInfo.m_guard);
}

inline ArtifactPrices ExtraInfoUnion::getArtifactPrice() const
{
    return H3_ENUM_DECODE(ArtifactPrices, m_artifactInfo.m_price);
}

inline enum EGameResource ExtraInfoUnion::getArtifactResourceCost() const
{
    return m_artifactInfo.m_resourcePrice;
}

inline bool ExtraInfoUnion::isDefendedArtifact() const
{
    return m_artifactInfo.m_price == const_artifact_defended;
}

// MapCell.h:959/964, dc 0x9c7b4 / 0x9c7c0. DoEventCampfire (0x4a1120)
// proves the short size truncation and unsigned resource lane.
inline short ExtraInfoUnion::getCampfireSize() const { return m_campfireInfo.m_size; }

inline int ExtraInfoUnion::getCampfireResource() const { return m_campfireInfo.m_resource; }

inline void ExtraInfoUnion::clearVisitedBits() { m_cellVisitedInfo.m_visited = 0; }

// Original: ExtraInfoUnion::get_custom_index; MapCell.h:974, dc 0x9c7c8.
// Complete narrows MonsterInfo::index from DC's 12 bits to eight; retail
// MonstersGiveReward (0x4a6b30) and DoWanderingMonsterResult (0x4a7740)
// both extract that eight-bit field before indexing the custom list.
inline short ExtraInfoUnion::getCustomIndex() const { return m_monsterInfo.m_index; }

inline short ExtraInfoUnion::getItemId() const { return m_skeletonInfo.m_id; }

inline void ExtraInfoUnion::setLeanTo(short id, short amount, int resource)
{
    m_leanToInfo.m_id = id;
    m_leanToInfo.m_amount = amount;
    m_leanToInfo.m_resource = resource;
}

// The lean-to trio, all three DC-published (MapCell.h:985/992/997).
// GetLeanToAmount is decorated `short` and that WIDTH is what makes
// DoEventLeanTo's emptiness test a sixteen-bit `test si,si` and its
// dialog argument a `movsx`. GetLeanToResource is decorated
// EGameResource; it is spelled `int` here because the field is read
// UNSIGNED and an enum bitfield sign-extends under VC6 - the width is
// what the bytes constrain and an enum return is int-wide anyway, so
// no truncation barrier is lost. The id has no DC accessor and is
// read off the arm directly.
inline short ExtraInfoUnion::getLeanToAmount() const { return m_leanToInfo.m_amount; }

inline int ExtraInfoUnion::getLeanToResource() const { return m_leanToInfo.m_resource; }

// The magic-spring pair (MapCell.h:1002/1007). The setter takes the
// new state rather than clearing unconditionally, which is what the
// DC decoration `void (unsigned char)` says and what makes the
// drink-it write a plain bit clear at the one site that passes 0.
inline unsigned char ExtraInfoUnion::magicSpringIsFull() const { return m_magicSpringInfo.m_full; }

inline void ExtraInfoUnion::fillMagicSpring(unsigned char full) { m_magicSpringInfo.m_full = full; }

// DC MapCell.h:1012..1015 records the resource store followed by the full
// flag store, and game::PerWeek calls this canonical helper.
inline void ExtraInfoUnion::fillGarden(enum EGameResource resource)
{
    m_gardenInfo.m_resource = resource;
    m_gardenInfo.m_full = 1;
}

// The mystical-garden trio (MapCell.h:1018/1023/1035). GardenIsFull
// is `unsigned char () const` and its `(value >> 10) & 1` shape is
// what retail inlines; a direct bitfield test would fold to a byte
// `test` on cell+1 instead.
inline unsigned char ExtraInfoUnion::gardenIsFull() const { return m_gardenInfo.m_full; }

inline enum EGameResource ExtraInfoUnion::getGardenResource() const
{
    return H3_ENUM_DECODE(EGameResource, m_gardenInfo.m_resource);
}

// Original SetGarden, MapCell.h:1028..1032, dc 0xbc9b0.
inline void ExtraInfoUnion::setGarden(short id, EGameResource resource)
{
    m_gardenInfo.m_id = id;
    m_gardenInfo.m_resource = resource;
    m_gardenInfo.m_full = 1;
}

inline void ExtraInfoUnion::setGardenEmpty() { m_gardenInfo.m_full = 0; }

// Original SetMagicSpring, MapCell.h:1040..1043, dc 0xbca04.
inline void ExtraInfoUnion::setMagicSpring(short id, unsigned char full)
{
    m_magicSpringInfo.m_id = id;
    m_magicSpringInfo.m_full = full;
}

// MapCell.h:1046/1051, dc 0x9c864 / 0x9c870. do_event_pyramid
// (0x4a4230) proves the signed spell lane and bit-zero guarded flag.
inline int ExtraInfoUnion::getPyramidSpell() const { return m_pyramidInfo.m_spell; }

inline bool ExtraInfoUnion::pyramidIsGuarded() const { return m_pyramidInfo.m_guarded; }

// E:\gamedcs\MapCell.h:1056, dc 0x9c878
VA(0x004c2330, 0x27)
inline void ExtraInfoUnion::setPyramid(bool guards, int newSpell)
{
    m_pyramidInfo.m_guarded = guards;
    m_pyramidInfo.m_spell = newSpell;
}

// All four scholar reward fields are signed.
inline ScholarAwards ExtraInfoUnion::getScholarAward() const
{
    return ScholarAwards(m_scholarInfo.m_award);
}

inline TPrimarySkill ExtraInfoUnion::getScholarPrimarySkill() const
{
    return TPrimarySkill(m_scholarInfo.m_primary);
}

inline TSecondarySkill ExtraInfoUnion::getScholarSecondarySkill() const
{
    return TSecondarySkill(m_scholarInfo.m_secondary);
}

inline SpellID ExtraInfoUnion::getScholarSpell() const
{
    return SpellID(m_scholarInfo.m_spell);
}

// E:\gamedcs\MapCell.h:1089, dc 0xbca4c
inline void ExtraInfoUnion::setScholar(ScholarAwards award, TPrimarySkill primary,
                                      TSecondarySkill secondary, SpellID spell)
{
    m_scholarInfo.m_award = award;
    m_scholarInfo.m_primary = primary;
    m_scholarInfo.m_secondary = secondary;
    m_scholarInfo.m_spell = spell;
}

// The corpse's four MapCell.h accessors, named and decorated by the
// Dreamcast line table over DoEventSkeleton (dc 0x95650):
// SkeletonHasTreasure is `_N`, GetItemId and GetSkeletonArtifact are
// both `F` (short) and SetSkeleton is `void (short, bool, short)` -
// MapCell.h:1104, which this file's carcass already carried.
// GetSkeletonArtifact is spelled `int` for the reason get_tomb_artifact
// is: retail stores the sign-extended ten-bit field straight into the
// artifact record with NO `movsx`, which a short return would have
// forced.

// SetSkeleton's ID PARAMETER IS INT-WIDE and the Dreamcast's `F` is
// not: the three folded stores end in `and eax,0xfffeffe0 / xor edx,eax
// / or edx,0xffc0`, i.e. the id is merged into the masked dword FIRST
// and the artifact constant last. A `short` parameter makes VC6
// reassociate the same value as `(id | 0xffc0) | masked` and emit the
// two ops the other way round. All four width combinations were
// measured against the retail bytes and exactly one is exact - short
// getter, int setter parameter (100.0, against 99.53 / 99.49 / 90.96),
// so the width is byte-determined, not a guess.
inline bool ExtraInfoUnion::skeletonHasTreasure() const { return m_skeletonInfo.m_hasTreasure; }

inline int ExtraInfoUnion::getSkeletonArtifact() const { return m_skeletonInfo.m_artifact; }

inline void ExtraInfoUnion::setSkeleton(int id, bool hasTreasure, short artifact)
{
    m_skeletonInfo.m_id = id;
    m_skeletonInfo.m_artifact = artifact;
    m_skeletonInfo.m_hasTreasure = hasTreasure;
}

// MapCell.h:1111/1116, dc 0x9c918 / 0x9c924. The original return domains
// are SeaChestRewardTypes / TArtifact; retain the existing int-wide reads.
inline int ExtraInfoUnion::getSeaChestReward() const { return m_seaChestInfo.m_reward; }

inline int ExtraInfoUnion::getSeaChestArtifact() const { return m_seaChestInfo.m_artifact; }

inline SpellID ExtraInfoUnion::getShrineSpell() const
{
    return m_shrineInfo.m_spell;
}

// The artifact is signed; the gold amount is truncated to a short after
// multiplying the stored count by 500.
inline int ExtraInfoUnion::getTreasureArtifact() const { return m_treasureInfo.m_artifact; }

inline short ExtraInfoUnion::getTreasureSize() const { return m_treasureInfo.m_gold * 500; }

inline bool ExtraInfoUnion::treasureIsArtifact() const { return m_treasureInfo.m_hasArtifact; }

// `?GetTreePrice@ExtraInfoUnion@@QBA?AW4WiseTreePrices@@XZ`, named by
// the Dreamcast line table over DoEventTreeOfKnowledge (dc 0x964c4)
// and spelled `int` for get_tomb_artifact's reason.
inline int ExtraInfoUnion::getTreePrice() const { return m_treeInfo.m_price; }

inline void ExtraInfoUnion::emptyWagon() { m_wagonInfo.m_full = 0; }

inline short ExtraInfoUnion::getWagonAmount() const { return m_wagonInfo.m_resourceAmount; }

inline int ExtraInfoUnion::getWagonArtifact() const { return m_wagonInfo.m_artifact; }

inline enum EGameResource ExtraInfoUnion::getWagonResource() const
{
    return H3_ENUM_DECODE(EGameResource, m_wagonInfo.m_resource);
}

inline bool ExtraInfoUnion::wagonHasArtifact() const { return m_wagonInfo.m_hasArtifact; }

// The wagon's six MapCell.h accessors, all six named and decorated by
// the Dreamcast line table over DoEventWagon (dc 0x96784):
// WagonIsFull and WagonHasArtifact are `_N` - bool, not the unsigned
// char the tomb's twin returns - GetWagonArtifact is `?AW4TArtifact`,
// GetWagonResource `?AW4EGameResource`, GetWagonAmount `F` (short,
// which is what makes the payout argument a `movsx ecx,di`) and
// EmptyWagon `void ()`. GetWagonArtifact is spelled `int` for the
// same reason get_tomb_artifact is: TArtifact has no modelled
// definition here and an enum return is int-wide under VC6 anyway.
inline bool ExtraInfoUnion::wagonIsFull() const { return m_wagonInfo.m_full; }

// DC 1177..1181 writes resource, amount, full, has_artifact, visited_bits.
// The Complete masks prove the corresponding five fields. With the shrine
// bitset default-constructed and unpinned, RandomizeEvents retains this call
// and the 0x4c2360 body matches exactly.
// E:\gamedcs\MapCell.h:1176, dc 0xbcac8
VA(0x004c2360, 0x27)
inline void ExtraInfoUnion::setWagon(EGameResource resource, short amount)
{
    m_wagonInfo.m_resource = resource;
    m_wagonInfo.m_resourceAmount = amount;
    m_wagonInfo.m_full = 1;
    m_wagonInfo.m_hasArtifact = 0;
    m_wagonInfo.m_visitedBits = 0;
}

// DC MapCell.h:1186..1189 assigns artifact, full, has_artifact, visited_bits.
// Mac randomizeWagon 0xd67f4..0xd6820 expands the same four field stores.
// These named stores retain the exact 0x4c2390 body; the packed-mask
// spelling made VC6 expand every use and omit the standalone helper.
// E:\gamedcs\MapCell.h:1185, dc 0xbcb3c
VA(0x004c2390, 0x21)
inline void ExtraInfoUnion::setWagon(int artifact)
{
    m_wagonInfo.m_artifact = artifact;
    m_wagonInfo.m_full = 1;
    m_wagonInfo.m_hasArtifact = 1;
    m_wagonInfo.m_visitedBits = 0;
}

inline void ExtraInfoUnion::emptyTomb() { m_tombInfo.m_hasArtifact = 0; }

inline int ExtraInfoUnion::getTombArtifact() const { return m_tombInfo.m_artifact; }

inline unsigned char ExtraInfoUnion::tombIsFull() const { return m_tombInfo.m_hasArtifact; }

// DC 1209..1211 writes artifact, fullness, visit bits. Complete widens
// the artifact to ten bits; RandomizeEvents expands all three stores.
// E:\gamedcs\MapCell.h:1208, dc 0xbcb94
inline void ExtraInfoUnion::setTomb(TArtifact artifact)
{
    m_tombInfo.m_artifact = artifact;
    m_tombInfo.m_hasArtifact = 1;
    m_cellVisitedInfo.m_visited = 0;
}

// The five MapCell.h accessors the two mill handlers inline. The
// Dreamcast publishes all five with their signatures - get_wheel_gold
// and get_windmill_amount return `short` (?...@@QBAFXZ),
// get_windmill_resource returns EGameResource, and both setters take
// the same pair - and the retail bytes fix the bodies:
//   * the wheel's *500 lives INSIDE get_wheel_gold, which is why
//     do_event_water_wheel truncates the product with `movsx esi,ax`
//     even though 31*500 provably fits in a short;
//   * set_windmill writes BOTH fields, which is why the payout tail
//     is a single `and eax,0xfffe1ff0 / xor eax,edi` on the dword
//     instead of two read-modify-writes.
inline short ExtraInfoUnion::getWheelGold() const { return m_waterWheelInfo.m_gold * 500; }

inline void ExtraInfoUnion::setWheelGold(short amount) { m_waterWheelInfo.m_gold = amount / 500; }

inline short ExtraInfoUnion::getWindmillAmount() const { return m_windmillInfo.m_amount; }

inline enum EGameResource ExtraInfoUnion::getWindmillResource() const
{
    return H3_ENUM_DECODE(EGameResource, m_windmillInfo.m_resource);
}

inline void ExtraInfoUnion::setWindmill(enum EGameResource resource, short amount)
{
    m_windmillInfo.m_resource = resource;
    m_windmillInfo.m_amount = amount;
}

inline int ExtraInfoUnion::getWitchSkill() const { return m_witchHutInfo.m_skill; }

// E:\gamedcs\MapCell.h:1251, dc 0xbcbdc
// DC lines 1252/1253 assign the typed skill, then clear visits. Retail folds
// those field stores into the combined 0xfff0001f mask; retain the source fields.
VA(0x004c23c0, 0x1c)
inline void ExtraInfoUnion::setWitchSkill(TSecondarySkill skill)
{
    m_witchHutInfo.m_skill = skill;
    m_cellVisitedInfo.m_visited = 0;
}

// MapCell.h:1260. Dreamcast returns TArtifact; this foundational header
// cannot name artifact.h's enum without creating a circular include, so
// the ABI-equivalent int spelling is used at the declaration and callers
// cross into the enum domain explicitly. Dreamcast masks the older
// seven-bit object index; Complete's inlined artifact readers load the
// full signed word, proving that the later accessor preserves it.
inline TArtifact NewmapCell::getArtifactIndex() const
{
    // The packed signed ordinal crosses the enum boundary in this source
    // accessor; no separate artifactFromInt helper is evidenced.
    return TArtifact(m_objectIndex);
}

// E:\gamedcs\MapCell.h:1269 (dc 0xf4a78). Dreamcast retains an out-of-line
// copy, while retail /Ob2 expands this header helper at its callers.
inline TAdventureObjectType CObject::getType() const
{
    return getObjectTypePtr()->m_objectType;
}

#endif  /* HOMM3_MAPCELL_H */
