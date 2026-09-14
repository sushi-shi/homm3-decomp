// game.h - prototypes of game.cpp (compiland game.obj)
#ifndef HOMM3_GAME_H
#define HOMM3_GAME_H

#include <map>
#include <memory>
#include <direct.h>
#include "savegame.h"
#include "smackmgr.h"
#include <ctype.h>
#include <string.h>
#include <vector>
#include "mapcell.h"
#include "netmsg.h"
#include "secondaryskill.h"
#include "creaturetype.h"
#include "struct.h"
// `class game` embeds the hero array by value, so the COMPLETE hero
// type has to be visible here. hero.h pulls armygrp.h; armygrp.h no
// longer pulls this header back (see the note at its top) - that is the
// edge that was cut to make this include legal.
#include "hero.h"
#include "creature_bank_types.h"
#include "town.h"
#include "victorylossconditions.h"
#include "creaturetype_fwd.h"
#include "advmgr_objects.h"
#include "seerhut.h"
#include "customcampaign.h"

// The one decoded value of game::field_1f63e shared by events.obj and
// philai.obj: Sunday is the seventh day.  DoEventTemple doubles its morale
// reward on this rung; move_hero stops a low-value full-hourglass move on it.
enum EDayOfWeek {
    DAY_OF_WEEK_SUNDAY = 7
};

int __fastcall readHeroId(AbstractFile* infile, int mapVersion);
int __fastcall loadHeroId(AbstractFile* infile, int saveVersion);

// The map record GetWorldMapData hands out. Its first 0xd0 bytes are the
// scenario's object/event vectors (13 of them at VC6's 16-byte
// std::vector; the DC build has 9 at STLport's 12). The seventh and eighth
// vectors are exposed only to advmgr.cpp: retail SetRolloverText reads their
// first pointers at +0x64/+0x74; the DC vector order names the seventh
// SeerHutList, while admitted retail structure evidence names the SoD-only
// eighth QuestGuardList. cellData/Size/HasTwoLevels are the DC's own tail
// names, and the offsets are byte-proven: game::get_cell reads the pointer at
// +0xd0 and the square extent at +0xd4, find_magus_hut_value the level count
// at +0xd8.
class CObjectType;
class CObject;
class type_event_record;
class MonsterData;
// Retail NewfullMap::Init proves the deleting destructor at slot zero.
// NewMap and the mapcell broadcasts prove the remaining slot count and the
// signatures at +0x24, +0x28 and +0x38; the other names remain address-based.
class CMapObjectData {
public:
    virtual ~CMapObjectData();
    virtual void newMapVFn04();
    virtual void newMapVFn08();
    virtual void newMapVFn0c();
    virtual void newMapVFn10();
    virtual void newMapVFn14();
    virtual void newMapVFn18();
    virtual void newMapVFn1c();
    virtual void newMapVFn20();
    virtual void newMapVFn24(int heroId, int player);
    virtual void newMapVFn28(type_point point, int player);
    virtual void newMapVFn2c();
    virtual void newMapVFn30();
    virtual void newMapVFn34();
    virtual void newMapVFn38();
};

// The two map-object pools readObject (0x502e00) appends to that no other
// claimed body reaches yet. Both are sixteen bytes wide, which is exactly
// the pad each replaces inside NewfullMap - the class size does not move.

// The hero-placeholder record, from readObject's id-214 arm. The object
// pointer is stored FIRST, before the first stream read; the hero id widens
// an 0xff sentinel to -1 and only that case reads a power rating, which is
// why the rating byte can be left uninitialised.
struct HeroPlaceholderData {
public:
    // The stream's "no fixed hero" sentinel. readObject widens it to -1 in
    // the record and only that case goes on to read the power-rating byte,
    // which is why the rating can stay uninitialised on every other record.
    enum { HERO_ID_BY_POWER_RATING = 0xff };
    CObject* m_object;
    unsigned char m_owner;
    // readObject's hero-placeholder arm stores the byte owner at +4
    // and the full hero ID at +8. Three bytes align that ID in the 0x10-byte record.
    char m_paddingBeforeHeroId[3];
    int m_heroId;
    unsigned char m_powerRating;
    // The optional power rating is the final byte at +0x0c in the
    // retail 0x10-byte vector element; three trailing bytes round its alignment.
    char m_paddingAfterPowerRating[3];
};
SIZE(HeroPlaceholderData, 0x10);

// The random-dwelling record, shared by readObject's three id-216/217/218
// arms: 216 takes both levels off the stream, 217 forces both to the object
// type's own `extra` byte, and 218 zeroes the castle id and sets the faction
// mask to `1 << extra`. The retail-only resolution pass at 0x502b60 walks
// the same vector with this stride.
struct RandomDwellingData {
public:
    int m_castleId;
    // SoD_transformRandomDwellings sign-extends all four values before
    // handing them to pick_alignment, Random, and generator::Initialize.
    short m_factionMask;
    signed char m_owner;
    signed char m_minLevel;
    signed char m_maxLevel;
    // The retail dwelling arm ends its short/byte options at +8;
    // the object pointer begins at +0x0c. Three bytes align that pointer.
    char m_paddingBeforeObject[3];
    CObject* m_object;
};
SIZE(RandomDwellingData, 0x10);

// The town-definition pool uses town.h's canonical TownExtra record.
// readTownData and ProcessOnMapTowns share its 0x88-byte PC layout.

#ifndef Town
#define Town town
#endif
class Town;

// HeroExtra is naturally aligned except for the packed +0x300..+0x306 band,
// which places location at +0x301. Compiler-generated copies skip alignment
// padding outside that band. The complete record occupies 0x334 bytes.
#pragma pack(push, 8)
class HeroExtra {
public:
    // +0x00..+0x67 decoded by hero::HeroFn_004D8B30, which reads every
    // field here. Names are the Dreamcast HeroExtra roster's: from DC
    // offset 8 onward all sixteen of its members land on retail at
    // DC + 4, in order and without exception, which is what identifies
    // this record. The two fields the Dreamcast build has no counterpart
    // for are the dword at +0x08 and the flag at +0x1a.
    signed char m_owner;  // +0x00
    int m_id;  // +0x04
    int m_objRef;  // +0x08 - copied to hero::field_01e
    unsigned char m_hasCustomName;  // +0x0c
    char m_nameBuffer[13];  // +0x0d - Complete strcpy destination
    unsigned char m_customExperience;  // +0x1a
    int m_experience;  // +0x1c
    unsigned char m_customPortraitNumber;  // +0x20
    unsigned char m_portraitNumber;  // +0x21
    unsigned char m_customSecondarySkills;  // +0x22
    int m_numSecondarySkills;  // +0x24 - signed, the loop bound
    char m_secondarySkill[8];  // +0x28 - movsx, so plain char
    char m_secondarySkillLevel[8];  // +0x30
    unsigned char m_customArmies;  // +0x38
    int m_armies[7];  // +0x3c
    short m_numTroops[7];  // +0x58 - movsx word
    unsigned char m_groupFormation;  // +0x66 - no retail body reads it
    unsigned char m_customArtifacts;  // +0x67
    type_artifact m_artifacts[19];
    type_artifact m_backpack[64];
    #pragma pack(push, 1)
    unsigned char m_numInBackpack;  // +0x300 - no retail body reads it
    type_point m_location;  // +0x301 - unaligned, hence the band
    signed char m_patrolRadius;  // +0x305 - sign gates the patrol XY
    unsigned char m_customName;  // +0x306
#pragma pack(pop)
    std::basic_string<char, std::char_traits<char>, std::allocator<char> > m_name;
    // +0x318 is the hero's SEX, not experience: HeroFn_004D8B30 gates it
    // on `!= -1` and stores it into hero::sex at +0x3d5. The real
    // Experience is the dword at +0x1c above. Renamed 2026-08-20.
    int m_sex;
    unsigned char m_customSpells;  // +0x31c
    std::bitset<70> m_spells;  // +0x320
    unsigned char m_customPrimarySkills;
    signed char m_primarySkills[4];  // +0x32d, class trails to 0x334

    // Implicit default constructor; CodeView dc 0xbd5f4 compgenx.
    void heroExtraFn004B8450(int heroId);
};
#pragma pack(pop)
SIZE(HeroExtra, 0x334);

// The four secondary skills a university offers, in slot order. Dreamcast
// type 0x1adf / field list 0x3521 proves the sixteen-byte aggregate with one
// skills array and no constructor. Complete's game::Load likewise passes an
// uninitialized fill record to opaque vector::resize, and RandomizeUniversity
// fills this native local with four selected skills. Neither path initializes
// the elemental schools: that operation belongs to the Conflux callers.
struct type_university {
public:
    SecondarySkill m_skills[4];
    type_university* initializeMagicSkills();
};
SIZE(type_university, 0x10);

// Canonical declaration and inline constructors are in struct.h, as
// identified by Dreamcast CodeView. Only a forward declaration is needed here.
class CNetPlayerInfo;

enum EMapFormatVersion {
    MAP_FORMAT_RESTORATION_OF_ERATHIA = 14,
    // Both byte-proven by readHeroData (0x5021c0), which gates on each of
    // them separately: `cmp esi,0x15` decides whether the hero record
    // carries a single signed spell id or a 70-bit mask, and
    // `cmp [gpGame+0x1f86c],0x1c` decides whether the equipped-artifact
    // band is eighteen positions or nineteen.
    MAP_FORMAT_ARMAGEDDONS_BLADE = 21,
    MAP_FORMAT_SHADOW_OF_DEATH = 28
};

// Retail victory/loss records embedded in game at +0x1f89c/+0x1f8e8.
// Only the fields reached by reconstructed consumers are exposed. The
// defeat-hero ids are fixed independently by AI_value_of_combat's two
// objective-bonus branches.
// hero.cpp owns the DATA claim (0x69774c); CheckForDefeatedHeroLoss's
// campaign-mode gate reads it.
extern unsigned char g_campaignMode;

// The upgrade-town victory's two level domains (map-format ordinals).
// CheckForUpgradedTown (0x5f1d40) maps each to the matching
// type_building_id bit: town/city/capitol halls, fort/citadel/castle.
enum EVictoryHallLevel {
    VICTORY_HALL_TOWN = 0,
    VICTORY_HALL_CITY = 1,
    VICTORY_HALL_CAPITOL = 2
};
enum EVictoryCastleLevel {
    VICTORY_CASTLE_FORT = 0,
    VICTORY_CASTLE_CITADEL = 1,
    VICTORY_CASTLE_CASTLE = 2
};

enum EVictoryConditionType {
    // 0x5f1610 CheckForArtifactWin's main arm gates on `cmp Type,0`,
    // the map-format acquire-artifact ordinal.
    VICTORY_CONDITION_ARTIFACT = 0,
    // 0x5f1b10 CheckForTotalCreatures gates on `cmp Type,1` and
    // 0x5f1d40 CheckForUpgradedTown on `cmp Type,3`; the values agree
    // with the map-format victory-condition ordinal (0 artifact,
    // 1 creatures, 2 resources, 3 upgrade town, ...).
    VICTORY_CONDITION_TOTAL_CREATURES = 1,
    // ai_player.obj joins for purchase_building's two inlined helpers:
    // value_of_castle_upgrade and value_of_hall both price the 5,000,000
    // upgrade-town victory bonus behind `cmp Type,3`.
    VICTORY_CONDITION_UPGRADE_TOWN = 3,
    // town.obj joins for initialize_buildings' Grail-slot gate;
    // ai_player.obj for find_all_destinations' fixed grail-spot value.
    VICTORY_CONDITION_BUILD_GRAIL = 4,
    // 0x5f2390 CheckForDefeatedMonsterWin dispatches on both: 7 is the
    // map-format defeat-monster ordinal, 11 the engine's
    // every-monster-dead sweep of the same routine.
    VICTORY_CONDITION_DEFEAT_MONSTER = 7,
    VICTORY_CONDITION_DEFEAT_ALL_MONSTERS = 11,
    // Complete adds the survive-until victory immediately after the
    // original map-format range. 0x5f2810 gates on `cmp Type,0xc`.
    VICTORY_CONDITION_SURVIVE_TIME = 12,
    // 0x5f2860 CheckForArtifactTransportWin and AI_get_value_of_artifact
    // gate on `cmp Type,0xa`, the map-format transport-artifact ordinal.
    VICTORY_CONDITION_TRANSPORT_ARTIFACT = 10,
    VICTORY_CONDITION_TOTAL_RESOURCES = 2,
    VICTORY_CONDITION_DEFEAT_HERO = 5,
    VICTORY_CONDITION_CAPTURE_TOWN = 6,
    VICTORY_CONDITION_FLAG_ALL_GENERATORS = 8,
    VICTORY_CONDITION_FLAG_ALL_MINES = 9
};

enum ELossConditionType {
    LOSS_CONDITION_LOSE_TOWN = 0,
    LOSS_CONDITION_LOSE_HERO = 1,
    LOSS_CONDITION_TIME_LIMIT = 2
};

// Campaign/scenario ordinals used by the new-map and condition-validation
// blocks. Retail proves the values; title identities are deliberately not
// imported from external tables.
enum EGameCampaignOrdinal {
    GAME_CAMPAIGN_2 = 2,
    GAME_CAMPAIGN_3 = 3,
    GAME_CAMPAIGN_5 = 5,
    GAME_CAMPAIGN_7 = 7,
    GAME_CAMPAIGN_8 = 8,
    GAME_CAMPAIGN_14 = 14,
    GAME_CAMPAIGN_15 = 15,
    GAME_CAMPAIGN_16 = 16,
    GAME_CAMPAIGN_18 = 18
};

enum EGameScenarioOrdinal {
    GAME_SCENARIO_0 = 0,
    GAME_SCENARIO_1 = 1,
    GAME_SCENARIO_2 = 2,
    GAME_SCENARIO_3 = 3,
    GAME_SCENARIO_4 = 4,
    GAME_SCENARIO_6 = 6,
    GAME_SCENARIO_8 = 8,
    GAME_SCENARIO_9 = 9
};

enum ENewMapStartingBonus {
    NEW_MAP_BONUS_ARTIFACT = 0,
    NEW_MAP_BONUS_GOLD = 1,
    NEW_MAP_BONUS_RESOURCE = 2,
    NEW_MAP_BONUS_RANDOM = 3,
    // Complete's scenario-info row constructor seeds this sentinel and its
    // Draw switch has an explicit fifth jump-table arm which shares the
    // random icon frame. The older Dreamcast enum does not contain it.
    NEW_MAP_BONUS_NONE = 4
};

enum ENewMapHandicap {
    NEW_MAP_HANDICAP_NONE = 0,
    NEW_MAP_HANDICAP_MILD = 1,
    NEW_MAP_HANDICAP_SEVERE = 2
};

// Retail's player-slot destructor walks these records at a 0x14 stride and
// destroys the Dinkumware string at +0x4. readMapPlayerSlot reads a hero ID
// byte (0xff -> -1) and that hero's name into this vector. NH3API calls the
// owning vector TPlayerSlotAttributes::heroes, but its shared HeroIdentity
// facade labels the first word portrait. This player's hero roster instead
// stores an ID; keep the retail-derived heroId name and four-byte storage.
struct type_map_hero_identity {
public:
    int m_heroId;
    std::string m_name;
};
SIZE(type_map_hero_identity, 0x14);

// The map header's per-player hero customization record: the identity above
// plus one eight-player availability mask. Complete's new-map normalization
// compares that mask with bitset<8>(0xff) and copies its single backing
// dword straight into game::heroPoolMap, which is what types the +0x14 word
// as a bitset rather than a plain int; the 0x18 size and every offset are
// the same either way. It was modelled BOTH ways in this tree - this record
// for the four TUs that read the mask, an inherited `int field_14` twin for
// everyone else - and the two spellings never met.
// The members are DIRECT rather than inherited, and retail settles it with
// three rows: the inherited spelling (which needs the three-argument
// constructor to assign the base members in its body instead of
// initialising them) costs ??0type_map_hero_info 100.0000 -> 84.0169,
// campaignbrief's _Tree::_Copy 100.0000 -> 84.1429 and game's _Tree::erase
// 100.0000 -> 92.1530. Its one point in favour - it retains newgame's
// type_map_hero_identity copy ctor through the base copy - is bought back
// by the plain field_34 vector below - an argument that does not
// stands either way: that "retained copy ctor" was 0x517c30, and
// 0x517c30 is objecttype's `pair<const string, int>` constructor.
// The map keyed by hero ID has a different first-word role from the player
// roster above: game::loadMap copies it into HeroExtra::portraitNumber after
// comparing the complete -1 dword. NH3API HeroIdentity::portrait/name and
// HeroPlayerInfo::players supply these semantic names; its byte portrait and
// inherited/STLport-era offset descriptions do not override retail layout.
struct type_map_hero_info {
public:
    int m_portrait;
    std::string m_name;
    std::bitset<8> m_players;
    // Map readers supply all three fields below; Dinkumware map insertion
    // copies the supplied value and does not default-construct this record.
    type_map_hero_info(int portrait, std::string name,
                       std::bitset<8> availability);
};
SIZE(type_map_hero_info, 0x18);

// The three serialized aggregates embedded consecutively in `game` are
// fixed by retail's SavedGameHeader constructor, assignment calls, and copy
// widths. Opaque bands remain opaque, but there is only one class layout for
// every translation unit.
class CMapHeaderData {
public:
// Before normalization (type): CMapHeaderData::TPlayerSlotAttributes.
#ifndef PlayerSlotAttributes
#define PlayerSlotAttributes TPlayerSlotAttributes
#endif
    class PlayerSlotAttributes {
    public:
        unsigned char m_canBeHuman;
        unsigned char m_canBeComputer;
        // +0x02..+0x03 is an UNNAMED alignment hole, byte-proven by retail's
        // synthesized copy ctor 0x45da70: it copies +0 and +1 as bytes and
        // then the dword at +4 directly. A named pad here adds a word copy
        // retail does not have.
        int m_aiStrategy;
        // Complete's InitNewGame expansion zero-extends this nine-town
        // mask (xor ebx,ebx; mov bx,[slot+8]), as do the lobby consumers.
        unsigned short m_legalAlignments;
        unsigned char m_hasRandomAlignment;
        unsigned char m_generateHero;
        // +0x14. CreateTownHeroes (0x4ca040) walks the eight slots with a
        // 0x44-stride induction pointer and feeds three fields of this
        // dword to GetTownId: `word[+0x14] << 6 >> 6`, `word[+0x16] << 6
        // >> 6` and `word[+0x16] << 2 >> 12` - the exact 10/10/4 signed
        // bitfield triple type_point already carries. Dreamcast names the
        // member CastleLoc at its own +0x0c; retail's eight extra bytes
        // above shift it and everything after it by 8.
        // CastleLoc keeps type_point's empty default constructor; the
        // slot constructor leaves the coordinate bits untouched in retail.
        // Complete split these two main-town fields out of the opaque PC
        // extension.  The player-slot reader writes the presence flag at
        // +0x0c and the signed town type at +0x10 before unpacking the
        // coordinate at +0x14.
        unsigned char m_hasMainTown;
        // +0x0d..+0x0f: alignment hole (0x45da70 goes +0x0c byte -> +0x10 dword).
        int m_mainTownType;
        type_point m_castleLoc;
        signed char m_hasRandomHero;
        // +0x19..+0x1b: alignment hole (0x45da70 goes +0x18 byte -> +0x1c dword).
        int m_nonRandomHeroId;
        int m_nonRandomHeroCustomPortrait;
        char m_nonRandomHeroCustomName[12];
        int m_defaultPlaceholders;
        // Hero IDs and names read from the map player slot.
        std::vector<type_map_hero_identity> m_heroes;

        VA(0x0045a950, 0x3F)  // retained retail body; formerly enrolled by CLASS_CTOR
        PlayerSlotAttributes()
        {
            m_canBeHuman = 0;
            m_canBeComputer = 0;
            m_aiStrategy = -1;
            m_legalAlignments = 0;
            m_hasRandomAlignment = 0;
            m_generateHero = 0;
            m_hasRandomHero = 0;
            m_nonRandomHeroId = -1;
            m_nonRandomHeroCustomPortrait = -1;
            m_nonRandomHeroCustomName[0] = 0;
            m_defaultPlaceholders = 0;
        }

        // Retail extracts this PC-only reader from NewSMapHeader::Read and
        // passes the stream plus map-format version (`ret 8`).  Dreamcast
        // keeps the equivalent logic in the parent reader, so the role name
        // is provisional.
        void readMapPlayerSlot(AbstractFile* infile, int mapVersion);
    };
    int m_version;
    unsigned char m_isPlayable;
    unsigned char m_difficulty;
    unsigned char m_numPlayers;
    unsigned char m_minNumHumanPlayers;
    unsigned char m_maxNumHumanPlayers;
    unsigned char m_lastTownNameAssigned;
    unsigned char m_mapHasNotBeenSaved;
    unsigned char m_maxHeroLevel;
    unsigned char m_numTeams;
    signed char m_teamInfo[8];
    // +0x15..+0x17: alignment hole. Retail's synthesized GameSelection-
    // HeadersStruct copy ctor (0x5904f0) copies the dword at +0x11 and then
    // the dword at +0x18 with nothing between; a named pad adds a word+byte
    // pair retail lacks.
    int m_size;
    unsigned char m_hasTwoLayers;
    // +0x1d..+0x1f: alignment hole (0x5904f0 goes +0x1c byte -> +0x20 vector).
    std::vector<int> m_placeholders;
    VictoryConditionStruct m_victoryCondition;
    LossConditionStruct m_lossCondition;
    PlayerSlotAttributes m_playerSlotAttributes[8];
    // +0x2c0, and it belongs to THIS class, not to NewSMapHeader - byte-
    // proven 2026-08-20 by the two constructors. CMapHeaderData's own
    // compiler-generated ctor at 0x45a990 writes the Dinkumware _Tree
    // triple in place - `mov [esi+0x2c0],dl` / `mov [esi+0x2c1],al` /
    // `mov [esi+0x2c8],ebx` - and then runs `_Lockit` + `operator
    // new(0x2c)` to buy the head node, which is `_Tree::_Tree` inlined.
    // game::game correspondingly has NO _Tree call: after the one
    // `call CMapHeaderData::CMapHeaderData` its next three constructions
    // are at mapHeader+0x2d0, +0x2e0 and +0x2f0 (the two strings and the
    // bitset). Modelled on NewSMapHeader instead, the map's ctor call
    // lands in game::game and shifts every construction after it.
    std::map<int, type_map_hero_info> m_heroPlayerSetups;
};
SIZE(CMapHeaderData, 0x2d0);
SIZE(CMapHeaderData::PlayerSlotAttributes, 0x44);

class NewSMapHeader : public CMapHeaderData {
public:
    std::string m_mapName;
    std::string m_mapDescription;
    std::bitset<156> m_availableHeroes;
    int save(AbstractFile* outfile);
    VA(0x0045a7a0, 0x1A3)  // retained retail body; formerly enrolled by CLASS_CTOR
    NewSMapHeader()
    {
        m_version = 0;
        m_difficulty = 0;
        m_numPlayers = 0;
        m_minNumHumanPlayers = 0;
        m_maxNumHumanPlayers = 0;
        m_lastTownNameAssigned = 0;
        m_mapHasNotBeenSaved = 0;
        m_mapName = "";
        m_mapDescription = "";
    }
    // Compiler-generated. Dreamcast retains its standalone COMDAT at the
    // campaignbrief.cpp use site rather than at a Game.h definition, while
    // retail expands this exact member teardown into ~SavedGameHeader.
    // Copy assignment is compiler-generated. BackupGameHeaders proves its
    // member walk directly: base assignment, both strings, then the bitset.
    // DC game.h:312 copies the older POD base with memcpy; Complete calls
    // CMapHeaderData's memberwise assignment because its base owns containers.
    // DC lines 314/315 preserve the two string operator= boundaries. Retail
    // expands them to different depths at different callers; retain the source
    // operations without forcing a shared inline-depth setting.
    void assignData(CMapHeaderData* data, char* name, char* description)
    {
        static_cast<CMapHeaderData&>(*this) = *data;
        m_mapName = name;
        m_mapDescription = description;
    }
    // Complete's scenario reader consumes the abstract stream and the
    // selected campaign-map ordinal (`ret 8` at retail 0x4c4390).
    int read(AbstractFile* infile, int campaignMap);
    // DC game.cpp:7232 and the class method record name this static
    // string-reference reader. Retail 0x4c6010 uses the same two-register
    // ABI as game's short-length reader, with a dword map length instead.
    static int __fastcall readString(AbstractFile* infile, std::string& value);
    int readVictoryCondition(char type, AbstractFile* infile);
    int readLossCondition(char type, AbstractFile* infile);
    int saveVictoryCondition(char type, AbstractFile* outfile);
    // Complete's saved-header reader carries the save version as a third
    // argument so pre-25 campaign hero ids can be remapped.
    int loadVictoryCondition(char type, AbstractFile* infile,
                             int saveVersion);
    int loadLossCondition(char type, AbstractFile* infile, int saveVersion);
    // Retail carries the save-version argument absent from the Dreamcast
    // declarator; the 0x4c5630 body returns with `ret 8`.
    int load(AbstractFile* infile, int saveVersion);
    int get(const char* path, const char* filename, int saveVersion);
};
SIZE(NewSMapHeader, 0x304);

// `mine` and `generator`, two of the four object-pool element types.
// The DC roster declares both in E:\gamedcs\Game.h, i.e. here.

// `generator` transfers from the Dreamcast unchanged - DC size 92 IS
// retail's pool stride, and DC's mapX/mapY/mapZ at 84/85/86 are exactly
// the three bytes game::GetGeneratorId compares at +0x54/+0x55/+0x56.
// So the whole DC record is carried, with armyGroup (56 B) closing
// 28..84 on the nose.

// `mine` does NOT transfer: the DC record is 12 bytes and retail's pool
// stride is 64 (game::MineTypesOwned divides the byte span by `sar 6`).
// SaveMinePool serializes the first three bytes, passes +0x04 to
// armyGroup::save, then serializes +0x3c..+0x3e. The remaining alignment
// bytes stay opaque.
class generator {
public:
    char m_genClass;  // +0x00
    char m_genType;  // +0x01
    char m_paddingBeforeCreatureTypes[2];
    CreatureType m_type[4];  // +0x04  (DC 0x1CF0, 16 B)
    // +0x14. Four SHORTS, not two ints: the constructor's fused loop
    // walks `type` by 4 and this row by 2 over the same four
    // iterations.
    short m_population[4];
    ArmyGroup m_guards;  // +0x1c
    unsigned char m_mapX;  // +0x54
    unsigned char m_mapY;  // +0x55
    unsigned char m_mapZ;  // +0x56

protected:
    char m_playerOwner;  // +0x57
    char m_townId;  // +0x58

public:
    char m_paddingAfterTownId[3];
    generator();
    void initialize(long newOwner);
    // Dreamcast's generator-event xref records three calls to get_owner;
    // retail expands the signed owner-byte load and has no out-of-line row.
    inline long getOwner() const { return m_playerOwner; }
    unsigned char load(AbstractFile* infile);
    // update_bonus's negative twin. Retail has no out-of-line row for it
    // (nothing fits between generator::save's end at 0x4b8791 and
    // update_bonus at 0x4b87a0), so it is inline-only - the same shape
    // set_owner below carries.
    inline void removeBonus();
    unsigned char save(AbstractFile* outfile);
    inline void setOwner(long owner);
    void updateBonus();
    void grow(int unusedArg);
};
SIZE(generator, 0x5c);

class mine {
public:
    // Lighthouse objects share the mine ownership pool so sea-mobility and
    // the overview can count them with the ordinary mine walk. Retail proves
    // the discriminator value in both independent consumers.
    enum EMineType {
        MINE_TYPE_LIGHTHOUSE = 100
    };
    // +0x00/+0x01, both retail-proven: MineTypesOwned sign-extends
    // each and compares it against an int parameter.
    char m_playerOwner;
    char m_type;
    // Retail abandoned-mine loading sets it; NewMap installs the guards
    // for these mines and the overview counts them separately.
    unsigned char m_isAbandoned;
    char m_paddingBeforeGuards;
    ArmyGroup m_guards;
    // NH3API confirms PC +0x3c. Map loading stores the trigger coordinate.
    unsigned char m_mapX;
    // NH3API confirms PC +0x3d. Map loading stores the trigger coordinate.
    unsigned char m_mapY;
    // NH3API confirms PC +0x3e. Map loading stores the trigger coordinate.
    unsigned char m_mapZ;
    // after mapZ; it rounds the retail mine stride to 0x40.
    char m_paddingAfterCoordinates;
    mine()
        : m_playerOwner(-1), m_type(-1), m_isAbandoned(0)
    {
        m_mapX = -1;
        m_mapY = -1;
        m_mapZ = -1;
        m_guards.initialize();
    }
};
SIZE(mine, 0x40);

class SGameSetupOptions {
public:
    signed char m_color[8];
    signed char m_handicap[8];
    int m_alignment[8];
    signed char m_playerPos[8];
    signed char m_difficulty;
    char m_filename[251];
    char m_path[100];
    unsigned char m_canFlipFromToComputer[8];
    signed char m_curSelectedPlayer;
    unsigned char m_fileInitialized;
    signed char m_initializationNumHumans;
    signed char m_turnDuration;
    int m_startingHero[8];
    signed char m_startingBonus[8];
    int save(AbstractFile* outfile);
    VA(0x0045ac20, 0xD2)  // retained retail body; formerly enrolled by CLASS_CTOR
    SGameSetupOptions()
    {
        for (int i = 0; i < 8; ++i) {
            m_color[i] = i;
            m_handicap[i] = 0;
            m_alignment[i] = i % 9;
            m_playerPos[i] = i;
            m_canFlipFromToComputer[i] = i;
            m_startingHero[i] = -1;
            m_startingBonus[i] = 3;
        }
        m_difficulty = 0;
        m_turnDuration = 10;
        memset(m_filename, 0, sizeof(m_filename));
        memset(m_path, 0, sizeof(m_path));
        m_curSelectedPlayer = 0;
        m_fileInitialized = 0;
        m_initializationNumHumans = 0;
    }
    int load(AbstractFile* infile, int saveVersion);
};
SIZE(SGameSetupOptions, 0x1cc);

// The PC campaign-new-map caller passes this object as NewMap's third
// argument.  Retail invokes two nullary members at 0x487290/0x487900;
// neither has a surviving name, so the address-bearing spellings remain
// provisional while preserving the proved receiver and arity.
// Complete's NewMap takes the selected TCampaignBrief::ScenarioStruct
// (StartScenario 0x4884c0 passes `this`); game.h cannot name a nested
// type, so ScenarioStruct derives from this empty stand-in and the two
// bodies below (0x487290 / 0x487900, customcampaign.obj) are its methods.
class NewMapCampaignContext {
public:
    void newMapFn00487290();
    void newMapFn00487900();
};

// Product generation recorded in SavedGameHeader::gameVersion.  The save
// loader derives the same three rungs from the on-disk format version when an
// older header does not carry the field explicitly.
enum EGameVersion {
    GAME_VERSION_ROE = 0,
    GAME_VERSION_AB = 1,
    GAME_VERSION_SOD = 2
};

// Placement recovery control: moving this class beside its inline bodies,
// or embedding the same bodies inside it there, does not recover the
// retained Reset/Save calls in game::save. The three-state 64-TU family
// preserves every field and helper; the embedded form only perturbs two
// unrelated callers slightly downward. Keep the existing header structure.
class SavedGameHeader {
public:
    char m_id[8];
    int m_version;
    int m_gameVersion;
    NewSMapHeader m_mapHeader;
    SGameSetupOptions m_mapSetup;
    unsigned char m_campaignGame;
    // +0x4e1..+0x4e3 is natural alignment, not a source member. Naming it
    // makes VC6's implicit operator= copy three bytes retail deliberately
    // skips before the aligned SCampaign member.
    SCampaign m_campaign;
    std::string m_fileName;
    int m_difficultyRating;
    int m_numDeadPlayers;
    unsigned char m_deadPlayer[8];
    int m_humanPlayer[8];
    int m_currentPlayer;
    SavedGameHeader();
    void reset();
    int save(AbstractFile* outfile);
    int load(AbstractFile* infile);
};
SIZE(SavedGameHeader, 0x5a4);

// Before normalization (type): TBlackMarket.
#ifndef BlackMarket
#define BlackMarket TBlackMarket
#endif
struct BlackMarket {
public:
    Artifact m_artifacts[7];
};
SIZE(BlackMarket, 0x1c);

class Sign {
public:
    unsigned char m_hasText;
    std::basic_string<char, std::char_traits<char>, std::allocator<char> > m_signText;
    Sign() : m_hasText(0) {}
};
SIZE(Sign, 0x14);

struct legacyMineGuard {
public:
    signed char m_type;
    signed char m_amount;
};
SIZE(legacyMineGuard, 2);

enum type_action_type {
    const_initialization_action = 0,
    const_normal_action = 1,
    const_remote_action = 2,
    const_recorded_action = 3
};

// Retail's garrison pool element. GetFlaggedObjectOwner reads the first byte
// as a signed owner and indexes records with a 0x40 stride. ProcessHover
// independently reaches the army at +4; the DC roster names that member and
// the three trailing map bytes at +0x3c..+0x3e.
class garrison {
public:
    char m_playerOwner;
    // Dreamcast and retail place the one-byte owner at +0 and army
    // at +4. The intervening three bytes align the armyGroup.
    char m_paddingBeforeArmy[3];
    ArmyGroup m_garrisonArmy;
    // +0x3c, retyped in place (no declarator added, the include-set-free
    // edit class): AI_enter_garrison (0x524370) gates the troop grab on
    // this byte being nonzero - the AB/SoD "removable troops" property.
    // The DC roster instead starts its map triple here (mapX at DC 60);
    // retail's one byte-proven reader of +0x3c is the boolean gate, so
    // the flag keeps the slot and the triple stays at +0x3d..+0x3f.
    unsigned char m_removableTroops;
    unsigned char m_mapX;
    unsigned char m_mapY;
    unsigned char m_mapZ;
};
SIZE(garrison, 0x40);

#pragma pack(push, 8)
// Dreamcast NB11 type 0x3591, AI: six members, 0x78 bytes. Complete
// playerData::operator= (0x58f750) copies this entire subobject with
// rep movsd (30 dwords) from +0xf0, after the separate +0xe8 bitset.
// playerData::Init independently clears the same +0xf0..+0x168 range.
struct AI {
public:
    float m_gameAttentionValue[3];
    float m_turnAttentionValue[3];
    long m_turnProductionResource[7];
    double m_resourceValue[7];
    int m_averageResourceValue;
    float m_turnValueOfAvgArtifact;
};
SIZE(AI, 0x78);

#pragma pack(pop)

// playerData head: NextHero returns -1 when the player has no mobile
// hero (HasMobileHero is its bool wrapper). sizeof is 360, byte-proven
// by the gpGame->players index arithmetic every town.obj gate emits
// (`45*owner` then `[ecx + 8*eax + 0x20ad0]`), and corroborated from
// the other side: eight such records starting at 0x20ad0 end at
// 0x21610, sixteen bytes short of the hero array at 0x21620.

// The 0x00..0x40 head and the 0x88..0xe8 tail were sliced 2026-08-08 by
// REPACKING THE DREAMCAST ROSTER (evidence/dreamcast/members.csv, class
// `playerData`, 344 B / 26 members) onto retail's alignment - the same
// lever the hero roster answered to. One member changes width and
// everything else follows: DC's `currHero` is a char at 2, retail's is a
// dword at +4, so every later DC offset shifts by +4 until the two
// type_point/vector width differences absorb it. Five independent retail
// facts fall out of the repack with no further assumption, and all five
// were already byte-proven from unrelated bodies:
//   +0x3e numTowns, +0x3f currTown, +0x40 towns   (town::Deallocate),
//   +0x9c resources                               (recruit.obj),
//   +0xc8 dpid, +0xe1 isLocal, +0xe2 isHuman      (this TU's accessors).
// The repack additionally predicted +0x01 numHeroes, +0x04 currHero,
// +0x08 heroes and +0x39 puzzle_guess before those bytes were read, and
// all four then confirmed (FindHero/NextHero/guess_grail_location).

// Only the +0x38..+0x3f band is packed. puzzleGuess occupies the unaligned
// dword at +0x39; compiler-generated copies skip padding elsewhere.
// Natural alignment gives the complete record its 0x168-byte size.
#pragma pack(push, 8)

class playerData {
public:
    // The width of the `heroes` row below, and the cap the game enforces
    // on it. Byte-proven twice: playerData::Init (0x4b9e20) fills the row
    // with `mov ecx,8 / rep stosd`, and the tavern's rollover
    // (TTavernWindow::SetRolloverText 0x5d7920) both COMPARES numHeroes
    // against the literal 8 and prints the same 8 as the refusal's one
    // vararg.
    enum {
        HERO_SLOT_COUNT = 8
    };
    // +0x00. playerData::GetName indexes the colour-name table at
    // .bss 0x6a7df8 with `movsx` of this byte.
    signed char m_color;
    // +0x01. Signed: FindHero/NextHero both `movsx` it and gate on
    // `test/jle`.
    signed char m_numHeroes;
    // +0x04. DC spells it `currHero` (a char at DC 2); retail widened
    // it to a dword - NextHero compares the full register against -1.
    // Name kept as-is because advmgr.obj already writes it.
    int m_currHeroId;  // +0x04 (Deactivate stores -1)
    // +0x08. Extent from the DC repack (DC heroes 4..36 == 32 B) and
    // then PROVEN from retail: playerData::Init (0x4b9e20) fills it
    // with `lea edi,[this+8] / mov ecx,8 / rep stosd` of -1.
    int m_heroes[8];
    // +0x28, the two heroes the player's taverns are currently
    // offering. DC type 0x35C7 is 8 bytes; retail reads them as DWORDS
    // - hero::hire (0x4d7890) scans `[player + 0x28 + 4*i]` for the
    // hero's own id with a stride of 4 - so the row is two ints, not
    // eight bytes.
    int m_recruits[2];
    unsigned char m_startingNumHeroes;  // +0x30
    int m_personality;  // +0x34
#pragma pack(push, 1)
    char m_extraPuzzlePieces;  // +0x38
    // +0x39. A type_point by DC type; see the alignment note above.
    // playerData::Init settles both the offset and the BIT layout:
    // `or word [this+0x39], 0x3ff` then `or word [this+0x3b], 0x3fff`
    // is struct.h's short-bitfield type_point saturated field by field
    // (x 10 bits in the first unit, y 10 + z 4 in the second) at an
    // ODD base - which is the alignment finding above, from the other
    // side.
    type_point m_puzzleGuess;
    char m_deathCountDown;  // +0x3d
    char m_numTowns;  // +0x3e
    char m_currTownId;  // +0x3f (advManager::DeactivateCurrTown stores -1)
#pragma pack(pop)
    // 0x48 entries, now PROVEN three ways: "nothing addresses
    // +0x40..+0x88" from the retail side; the DC repack lands
    // `placement_help_enabled` exactly at +0x88 (DC towns 61..133 ==
    // 72 == 0x48 bytes); and playerData::Init (0x4b9e20) clears the row
    // with `lea edi,[this+0x40] / mov ecx,0x12 / rep stosd` - eighteen
    // dwords, i.e. exactly 72 bytes.
    char m_townIds[0x48];  // +0x40
    unsigned char m_placementHelpEnabled;  // +0x88
    // +0x8c. DC `std::vector<type_point> shipyards` - twelve bytes of
    // STLport there, sixteen of Dinkumware here, which is exactly the
    // slack that puts resources back on its proven +0x9c. Retail then
    // confirms all four words from the other side: the constructor at
    // 0x4b9df0 copies an EMPTY ALLOCATOR into +0x8c (which is where its
    // otherwise inexplicable read of uninitialised stack comes from -
    // `mov cl,[ebp-1]` is the temporary `allocator<type_point>()`) and
    // zeroes +0x90/+0x94/+0x98, and the destructor at 0x4ce570 frees
    // +0x90 and re-zeroes the same triple.
    std::vector<type_point> m_shipyards;
    // The seven-resource row, sliced 2026-08-08 for recruit.obj.
    // Byte-proven twice over, from two unrelated TUs:
    // recruitUnit::Update (0x550274) divides `[player+0xb4]` by the
    // gold price and `[player + 4*altResource + 0x9c]` by the alt
    // price, and TResourceDisplay::Update (0x558f45) prints
    // `[player + 4*id + 0x9c]` for the seven ids in the table at
    // 0x641008..0x641024. Gold is index 6 (0x9c + 4*6 == 0xb4).
    long m_resources[7];
    unsigned long m_mysticalGardenFlags;  // +0xb8
    unsigned long m_magicSpringFlags;  // +0xbc
    unsigned long m_deadGuyFlags;  // +0xc0
    unsigned long m_leanToFlags;  // +0xc4
    // +0xc8 / +0xcc. AssignNetInfo copies both out of a
    // CNetPlayerInfo; SetName's strncpy bound (0x14) fits the
    // twenty-one byte buffer the DC repack gives (DC 196..217).
    unsigned long m_dpid;
    char m_name[21];
    unsigned char m_isLocal;  // +0xe1
    unsigned char m_isHuman;  // +0xe2
    int m_quickCombat;  // +0xe4
    // Complete adds the combination-artifact bitset before the older AI
    // member. Constructors and load/save prove bitset<12> at +0xe8;
    // hero::HeroFn_004DC100 uses its Dinkumware bounds check and word index.
    // +0xec..+0xef is implicit alignment padding: retail assignment skips
    // it, then copies the complete AI member, including its internal pad.
    std::bitset<12> m_assembledCombinations;  // +0xe8

    playerData();
    // AI's production row lands at +0x108, resource values at +0x128,
    // average at +0x160, and artifact value at +0x164, matching the retail
    // get_total_value/calculate_demand accesses independently.
    AI m_ai;  // +0xf0
    // Implicit destructor; CodeView dc 0xbd630 compgenx.
    int load(AbstractFile* infile, int saveVersion);
    // 0x4bada0 (claimed in src/game.cpp). town::buy_building calls it
    // on gpGame->players[owner] to split the human and computer
    // resource paths.
    bool isHuman() const;
    int save(AbstractFile* outfile);
    // 0x4b9fc0 (located in src/game.cpp, body not reconstructed).
    // townManager::SwapHeroes 0x5d5150 calls it on
    // gpGame->players[townToView->owner] with the town it is showing,
    // and branches on the byte result - the arity and the return type
    // both. Nested under this gate, not added to the class outright: a
    // bare member-function declarator on a class this widely included
    // is the include-set wall's own trigger shape (the townManager
    // precedent), and townmgr.cpp is the only live consumer.
    unsigned char addGarrisonHero(Town* ourTown);
    bool hasMobileHero();
    int nextHero();
    int nextTown();
    int numOfGivenArtifact(int artifact) const;
    int findHero(int id) const;
    int findTown(int id) const;
    // ?IsHuman@playerData@@QBA_NXZ / ?IsLocalHuman@playerData@@QBA_NXZ
    bool isLocalHuman() const;
    char* getName();
    void assignNetInfo(CNetPlayerInfo* netPlayerInfo);
    void clearNetInfo();
    // 0x4b9f40 (claimed in src/game.cpp). town::can_build,
    // can_ever_build and get_buildable_mask all call it on
    // gpGame->players[town->owner] to veto a second Capitol.
    bool hasCapitol();
    void init();
    bool hasGivenArtifact(int artifact);
    void guessGrailLocation(long playerId);  // 0x4bae50
};
#pragma pack(pop)
SIZE(playerData, 360);

class game;
// Retail .bss 0x6994e8 (the game record) and 0x69ccb0 (the acting
// player's record). Names provisional. 2,264 dir32 references
// image-wide make gpGame the central object.
DATA(0x006994e8) extern game* g_game;
extern playerData* g_currentPlayer;

// Head model: GetWorldMapData hands out the embedded map record at
// 0x1fb70. Names provisional. (Merged 2026-08-07 with the second `game`
// view that lived in armygrp.h - ai_player.cpp is the first TU to need
// armyGroup and the map record at once, and two headers cannot each
// define the class.)
class game {
public:
    game();
    ~game();
    game& __fastcall operator=(const game& that);
// Before normalization (type): game::TRumour.
#ifndef Rumour
#define Rumour TRumour
#endif
    struct Rumour {
        std::basic_string<char, std::char_traits<char>, std::allocator<char> > m_text;
        unsigned char m_unavailable;
    };
    // +0x90, one byte: a shown-once latch on the "the Grail cannot be built
// in this town" message. townmgr's handle_hall_click (0x5d30d0) is the
// only body in the admitted surface that touches it - it refuses to put
// the message up a second time and sets the byte on the way out - so the
// ROLE is retail-proven; Dreamcast names the corresponding byte bGrailAsked.
    // Dreamcast game::newGameWin is heroWindow* at +0; spellAllocInfo
    // follows at +4 in both builds. Retail preserves the four-byte slot
    // before that array, with no located access to the pointer itself.
    heroWindow* m_newGameWin;
    // +0x04 and +0x4a, the current draw mask and scenario prohibition mask
    // used by game::GetRandomSpell; mapcell.obj's readScholarData rolls a
    // random scholar reward by collecting every spell whose prohibition
    // byte is clear and picking one.
    unsigned char m_spellAllocInfo[70];
    unsigned char m_spellDisabledInfo[70];
    // Dreamcast bGrailAsked follows spellAllocInfo at +0x4a; Complete
    // adds spellDisabled[70], placing the same shown-once latch at +0x90.
    unsigned char m_grailAsked;
    // Alignment after the grailAsked byte and before the scenario-town
    // vector at +0x94; Dreamcast likewise aligns townExtraPool after it.
    char m_paddingBeforeScenarioTowns[3];
    // +0x94, sliced out of the pad for NewfullMap::Read and ::Load, which
    // clear it.
    std::vector<TownExtra> m_scenarioTowns;
    // +0xa4, the map's per-hero setup pool. readHeroData (0x5021c0)
    // addresses it with the multiply chain `lea edx,[ecx+4*ecx] /
    // lea eax,[ecx+8*edx] / lea eax,[eax+4*eax] / lea esi,[edx+4*eax+0xa4]`
    // = gpGame + 820*id + 0xa4, and 156 * 0x334 is EXACTLY the 0x1f3b0 the
    // pad it replaces measured - 0xa4 + 0x1f3b0 lands on difficultyRating.
    HeroExtra m_heroSetup[156];
    short m_difficultyRating;
    // Dreamcast difficultyRating is a short before aligned sCampaign.
    // Retail retains this two-byte alignment gap at +0x1f456.
    char m_paddingBeforeCampaign[2];
    SCampaign m_campaign;  // +0x1f458
    unsigned char m_newCampaignStarted;
    // +0x1f4d5. RETYPED IN PLACE 2026-08-20 (no declarator added, so the
    // include-set population is unchanged): game::SaveGame (0x4beea0)
    // strcpy's its `filename` argument here through gpGame whenever the
    // composed name is neither of the two general-text rows it screens
    // against, i.e. this is the remembered save name. The 0x15f extent
    // is the pad's, not proven for the string - but 351 is exactly the
    // width of SaveGame's own two stack buffers and of misc.h's
    // gcRegAppPath, so the whole pad reading as one name buffer is the
    // economical hypothesis rather than a stretch. CORROBORATED FROM THE
    // OTHER SIDE: game::Load's already-matched header copy-out strcpy's
    // SavedGameHeader::fileName into this exact offset, so the writer
    // and the reader agree on what the field is.
    char m_saveFileName[0x15f];
    unsigned char m_numPlayers;
    signed char m_numDeadPlayers;
    // Eight per-player disabled/dead flags. type_AI_player::end_turn
    // skips a gift candidate when the indexed byte is nonzero.
    unsigned char m_playerDisabled[8];  // +0x1f636
    // Unsigned word gate used by calculate_demand: from value five on,
    // current dwelling population is augmented by one growth cycle.
    // Its wider calendar role is not yet attested, so the name remains
    // ordinal rather than importing a semantic guess.
    unsigned short m_day;
    short m_week;
    short m_month;
    char m_uniqueSystemId[0x20];
    Artifact m_marketArtifacts[7];
    // NH3API global.hpp confirms the PC vector at +0x1f680. Map loading
    // appends each black market and stores its index in the map cell.
    std::vector<BlackMarket> m_blackMarkets;
    short m_ultimateArtifactX;
    short m_ultimateArtifactY;
    unsigned char m_ultimateArtifactZ;
    unsigned char m_ultimateRadius;
    unsigned char m_ultimateArtifactPresent;
    // The ultimate-artifact coordinate/radius/validity run ends with
    // a byte at +0x1f696; this byte aligns the PC dword at +0x1f698.
    char m_paddingAfterUltimateArtifactPresent;
    int m_f1f698;
    unsigned char m_isCheater;
    // Byte gate town::can_build and get_buildable_mask test before the
    // Castle-Griffin-Tower special case that drops the Blacksmith
    // requirement; it sits four bytes past f_1f698 in the same band.
    // Role unattested - ordinal placeholder.
    char m_isTutorial;
    // Dreamcast bIsCheater/is_tutorial are adjacent bytes; retail
    // places them at +0x1f69c/d before setup at +0x1f6a0. This gap aligns it.
    char m_paddingBeforeSetup[2];
    SGameSetupOptions m_setup;  // +0x1f6a0
    NewSMapHeader m_mapHeader;  // +0x1f86c
    NewfullMap m_worldMap;  // +0x1fb70
    playerData m_players[8];
    // +0x21610. The scenario's town pool, and it is a std::vector, not
    // a bare pointer: game::GetTownId (0x4bb870) reads _First at
    // +0x21614 AND _Last at +0x21618, divides the byte span by 360 with
    // the signed magic-multiply, and guards the whole thing on
    // `_First != 0` - which is Dinkumware's own
    // `size() { return _First == 0 ? 0 : _Last - _First; }`.
    // town::can_build reads gpGame->towns[this->id].field_02 and
    // town::Deallocate writes gpGame->towns[this->id].owner, both with
    // that same 360-byte stride.
    std::vector<Town> m_towns;
    enum { HERO_COUNT = 156 };
    Hero m_heroes[HERO_COUNT];
    char m_heroAvailability[0x9c];  // +0x4df18
    // One eight-player eligibility mask per hero. GetStartingHeroId tests
    // the caller's player position through Dinkumware bitset::test(), and
    // the hero-placement path sets the same bit through bitset::set().
    std::bitset<8> m_heroPoolMap[0x9c];  // +0x4dfb4
    unsigned char m_artifactUsed[0x90];
    unsigned char m_artifactDisabled[0x90];
    unsigned char m_globalInfoFlags[32];
    unsigned char m_borderTentVisitFlags[8];
    unsigned short m_cartographerMask[3];
    unsigned char m_cartographerFlags[3];
    // The three cartographerFlags bytes end at +0x4e375; retail
    // places the sign vector at +0x4e378. Dreamcast preserves the same
    // three-byte alignment gap after cartographerFlags[3].
    char m_paddingBeforeSigns[3];
    // Sign text and the four object pools, +0x4e378 / +0x4e388 / +0x4e398 /
    // +0x4e3a8 / +0x4e3b8,
    // sixteen bytes apiece - VC6's Dinkumware vector, whose empty
    // allocator sits at +0 so _First/_Last/_End follow at +4/+8/+0xc.
    // Bases: ClaimMine reads +0x4e38c and ClaimGarrison +0x4e3ac (the
    // Load/Save bracket note), GetGeneratorId reads the +0x4e39c /
    // +0x4e3a0 pair with a 92-byte stride, and GetHeroBoat the
    // +0x4e3bc / +0x4e3c0 pair with a 40-byte one.
    std::vector<Sign> m_signs;  // +0x4e378
    std::vector<mine> m_mines;  // +0x4e388
    std::vector<generator> m_generators;  // +0x4e398
    std::vector<garrison> m_garrisons;  // +0x4e3a8
    std::vector<boat> m_boats;  // +0x4e3b8
    std::vector<type_university> m_universities;  // +0x4e3c8
    std::vector<type_creature_bank> m_creatureBanks;  // +0x4e3d8
    // +0x4e3e8, the map's obelisk count. RETYPED unsigned -> plain (i.e.
    // signed) char 2026-08-20: SetupPuzzlePieces (0x4baf00) reads it with
    // `mov al,[this+0x4e3e8]` followed by `movsx ebx,al`, which an
    // unsigned char cannot produce, and it does so at three sites - the
    // 48 - x, the float divisor and the == x compare. The Dreamcast dump
    // types the same member T_RCHAR and names it `numObelisks`, and the
    // DC body loads it with the sign-extending mov.b. The rename is
    // available on the same evidence but is left as a separate decision;
    // the only two other users are plain byte copies in Load/Save.
    char m_numObelisks;
    // +0x4e3e9, one signed byte of per-player visit bits per obelisk;
    // GetNumObelisks tests `(1 << player) & flags[i]` over exactly 48
    // entries.
    signed char m_obeliskFlags[0x30];
    char m_currentRumour[0x12d];  // +0x4e419
    char m_rumourState[0x100];  // +0x4e546
    // Dreamcast rumourAllocInfo[256] ends at +0x31ffe before MapRumours
    // at +0x32000; retail retains two alignment bytes before its vector.
    char m_paddingBeforeRumours[2];
    std::vector<Rumour> m_rumours;  // +0x4e648
    char m_ssDisabled[0x1c];
    char m_armyWindow[4];
    // Dreamcast original viewFrame is int at +0x32010. InitVars
    // (dc 0xe3a04, kb.cpp:3771) clears it immediately before copying
    // test.h3m into setup. Retail InitVars, expanded in EarlySetup
    // 0x4eda80, performs the same reset at +0x4e678 between the same
    // neighboring operations, proving the relocated member identity.
    int m_viewFrame;
    // +0x4e67c / +0x4e6fc / +0x4e77c - the teleport-destination pools,
    // all std::vector<type_point>. The two arrays are indexed by the
    // monolith colour (`color << 4` in both wrappers) and the gap
    // between the three bases is 0x80 each, i.e. eight vectors per
    // array; the extent rides on that arithmetic, not on a retail
    // bound check. Cell types come from the wrappers: 0x2d for the
    // first array, 0x2c for the second, 0x6f for the whirlpool pool.
    // NAMES ARE PROVISIONAL - nothing attests them.
    std::vector<type_point> m_lithPools[8];  // +0x4e67c
    std::vector<type_point> m_lithExitPools[8];  // +0x4e6fc

private:
    std::vector<type_point> m_whirlpools;  // +0x4e77c
    std::vector<type_point> m_undergroundGateExits;  // +0x4e78c

public:
    // One reciprocal exit index per entry above.  Dreamcast names the
    // std::vector<long> operator[] calls in match_underground_gates, while
    // retail compares and stores each four-byte element as a signed index.
    std::vector<long> m_undergroundGatePairs;  // +0x4e79c
    // Recorded adventure actions. Retail clear/replay/load/save methods
    // prove the Dinkumware pointer-vector at +0x4e7ac.
    std::vector<type_event_record*> m_eventRecords;
    // +0x4e7bc, and it is the LAST member: game::~game (0x4ce5b0) opens
    // its teardown here, at the highest offset it touches, with the bare
    // `operator delete(_First)` plus the three-word zeroing that a
    // Dinkumware vector of a trivially destructible element emits - no
    // `_Destroy` loop at all. The exact record_monster_identifier body at
    // 0x4ced40 appends the identifier/packed-point pair at an eight-byte
    // stride, byte-proving both the member and its element layout.
    struct MonsterIdentifier {
        int m_identifier;
        type_point m_point;
    };
    std::vector<MonsterIdentifier> m_monsterIdentifiers;
    NewfullMap* getWorldMapData();
    type_point gameFn004CEF10(int identifier);
    int getStartingHeroId(int alignment, int playerPos,
                          int mapPosition);  // 0x4bb400
    int getNewBoatId();  // 0x4bb170
    int createBoat(int x, int y, int z, int owner,
                   unsigned char remoteMove, signed char type);  // 0x4bb250
    int getNewHeroId(int playerPos, HeroClass excluded,
                     unsigned char preferAlignment,
                     HeroClass preferredClass);  // 0x4bb5e0
    // Retail 0x486110, customcampaign.obj's own game member and
    // DoPreLoadCustomization's per-hero callee: the map's setup record for
    // `heroId` is copied wholesale onto a newly allocated hero of the same
    // class, the original slot is retired by setting its placement x to -1,
    // and the availability table follows. Complete-only, name provisional -
    // no Dreamcast row covers it.
    void rehomeCampaignHeroSetup(int heroId);  // 0x486110
                 // DC game.cpp:10132
    type_point getPuzzleOrigin() const;  // 0x4cea70
    void setRandomHeroArmies(int heroId, int cheat,
                             unsigned char minimal);  // 0x4c9730
    Artifact getRandomArtifactId(int artifactClass);  // 0x4c94d0
    void checkHeroConsistency();
    void setupDynamicStuff(int update, int forceUpdate);  // 0x51bd50
    void setupNewOverviewType(int whichType,
                              unsigned char update);  // 0x51e330
    int processIconSelect(int codeY, unsigned char rightMouse);  // 0x51ee50
    playerData* getLocalPlayer();
    int getLocalPlayerGamePos() const;  // 0x4cea20
    SpellID getRandomSpell(std::bitset<5> spellLevels);  // 0x4c95a0
    boat* getHeroBoat(int id, unsigned char occupied);  // 0x4ce900
    int getTownId(int x, int y, int z);  // 0x4bb870
    int mineTypesOwned(int whichPlayer, int mineType);  // 0x4bae70
    int getGeneratorId(int x, int y, int z);  // 0x4bb900
    int getBoatsBuilt();  // 0x4cce30
    // 0x4baf00, its link-order neighbour. countOnly stops at the piece
    // count; otherwise the shared puzzlePiecesRemoved bitset is re-rolled.
    int setupPuzzlePieces(int whichPlayer, int countOnly);
    void giveArmy(ArmyGroup* thisMonInfo, int monType,
                  int monNum, int slot);  // 0x4ca340
    int experienceValueOfStack(const ArmyGroup* whichGroup,
                               const Hero* whichHero);  // 0x4ca3b0
    void insertObject(int x, int y, int z, int objType,
                      int objectIndex, int extraInfo);  // 0x4c9890
    bool isLocalHuman(int gamePos) const;  // 0x4ce970
    char* getPlayerName(int gamePos);  // 0x4ceb60
    int getGamePosFromDPID(unsigned long dpid) const;  // 0x4cec20
    // Same `_N`-and-const family as playerData's pair above.
    bool isLastHuman(int gamePos) const;  // 0x4cec50
    bool isMultiplayer() const;  // 0x4cec90
    // 0x4ca040. Retail carries a parameter the Dreamcast declarator
    // (`?CreateTownHeroes@game@@QAAXXZ`, no arguments) does not: the body
    // ends `ret 4`, tests [ebp+8] for null once per slot and separately
    // strength-reduces it into a four-byte-stride walker it dereferences,
    // i.e. an eight-entry int array of pre-chosen starting heroes that
    // overrides GetStartingHeroId for human players.
    void createTownHeroes(int* startingHeroIds);
    int getAlignment(int creature) const;
    void claimShipyard(type_point location, int newPlayerOwner);  // 0x4c6a30
    void claimTown(int townId, int newPlayerOwner,
                   unsigned char isRemoteMove,
                   unsigned char checkEndGame);  // 0x4c61e0
    void claimMine(int mineId, int newPlayerOwner,
                   type_action_type actionType);  // 0x4c66e0
    void claimGenerator(int generatorId, int newPlayerOwner);  // 0x4c67b0
    void claimGarrison(int garrisonId, int newPlayerOwner);  // 0x4c6960
    void recordClaimMine(long id, long newOwner);  // 0x49bf90

    void recordClaimTown(long id, long newOwner);
    static int __fastcall saveString(AbstractFile* outfile, std::string& text);
    // DC game.cpp:2492/2531 and the class method records explicitly
    // declare static loadString/saveString with a string reference.
    // Retail passes stream in ECX and string address in EDX at
    // 0x4bb990/0x4bbb60. Static /Gr members have that same ABI.
    static int __fastcall loadString(AbstractFile* infile, std::string& value);

private:
    int loadRumours(AbstractFile* infile);  // 0x4bbe40
    int saveRumours(AbstractFile* outfile);  // 0x4bbc20
    int loadSignPool(AbstractFile* infile);  // 0x4b9070
    int saveSignPool(AbstractFile* outfile);  // 0x4b9270

public:
    bool isHumanAlly(int teamNum) const;
    // event_record.obj owns 0x49d6c0's body.
    void clearEventRecords(char playerId);
    type_point getUndergroundGateExit(const NewmapCell* cell) const;
    unsigned char getRandomLithExit(long color, type_point& result) const;
    unsigned char getRandomLith(const std::vector<type_point>& points,
                                  type_point& result, long cellType,
                                  long excluded) const;  // 0x4cdb80
    unsigned char getRandomLith(long color, long excluded,
                                  type_point& result) const;
    unsigned char getRandomWhirlpool(long excluded, type_point& result) const;
    // event_record.cpp:1061 in the DC roster (dc 0x8e0b8). advManager::
    // EraseObj is its caller and pins the retail row: a 0x18-byte record
    // built with `new`, two vtable stores and the cell's +0x00/+0x22/+0x24
    // copied into it, reached with the cell and the point on the stack.
    void recordEraseObject(NewmapCell* cell, type_point point);  // 0x49c390
    void recordShowBoat(boat* currentBoat, type_point point);  // 0x49c900
    void calculateProduction();
    short getBaseMapScore() const;
    short getCurrentTurn() const;
    short getMapScore() const;
    void perDay();
    void perWeek();
    void perMonth();
    void setVisibility(int startX, int startY, int z,
                       int whichPlayer, int range,
                       unsigned char remoteMove);  // 0x49cdd0
    // event_record.cpp:1189 in the DC roster (dc 0x8e54c), the negative
    // twin of SetVisibility below and the same five parameters in the same
    // order. DoEventCoverOfDarkness is the caller that needs the
    // declarator; the row is claimed as a carcass stub in event_record.cpp.
    void resetVisibility(int startX, int startY, int z,
                         int whichPlayer, int range);
    void resetAllPlayerVisibility();
    void turnOnAIMusic();  // 0x4c6f80
    void turnOffAIMusic();
    // dump does name it - `?SetupAdjacentMons@game@@QAAXXZ`, public,
    void setupAdjacentMons();
    void showComputerScreen();
    // Open hands it the formatted turn banner and the acting game position
    // when the protocol is hotseat.
    void waitForPlayer(char* text, int gamePos);  // 0x4ca840
    int transmitSaveGame(int toWho, int thisPlayerDead,
                         unsigned char inGame, unsigned char makeOrig);
    // DC game.cpp:10587 names the received-save body. Retail's transmit-init
    // handlers independently prove the five arguments and 0x4cbd40 entry.
    int receiveSaveGame(int fileSize, int fullGameCRC, int fromWho,
                        unsigned char inGame, unsigned char isDiff);
    void doNewTurn();
    void showHeroesLogo();
    void setMapSize(int width, int height);  // 0x4ccef0
    void checkForTimeEvent();  // 0x4cd910
    void giveTimeEventReward(const TimedEvent* thisEvent);  // 0x4cd710
    void checkForTownEvent();  // 0x4cda10
    bool isHuman(int gamePos) const;  // 0x4ce940

private:
    int loadMinePool(AbstractFile* infile, int saveVersion);

public:
    void recordMonsterIdentifier(int identifier, type_point point);

private:
    int loadGarrisonPool(AbstractFile* infile, int saveVersion);
    int loadTownPool(AbstractFile* infile, int saveVersion);

public:
    int loadGame(const char* filename, int isOrigData, int isQuickLoad);
    unsigned char saveGame(const char* filename,
                           unsigned char determineSuffix,
                           unsigned char campaignWinMode,
                           unsigned char compressIt,
                           unsigned char xferFile);

private:
    int load(AbstractFile* infile);  // 0x4bcda0
    int loadBlackMarkets(AbstractFile* infile);
    int saveBlackMarkets(AbstractFile* outfile);
    int saveMinePool(AbstractFile* outfile);  // 0x4b9580
    int saveGarrisonPool(AbstractFile* outfile);  // 0x4b98c0
    int loadBoatPool(AbstractFile* infile);  // 0x4b9a00
    int saveBoatPool(AbstractFile* outfile);  // 0x4b9c40
    int loadObeliskPool(AbstractFile* infile);
    int saveObeliskPool(AbstractFile* outfile);
    int saveTownPool(AbstractFile* outfile);

public:
    // 0x4bf780 (dc 0xaa7e0).
    void validateVictoryLossConditions(unsigned char checkMapLocations);
    void giveTroopsToNeutralTown(int townId);  // 0x4bf570
    void setupOrigData();
    void newMap(AbstractFile* mapFile, int* playerHeroFaces,
                NewMapCampaignContext* campaignContext, int gameVersion);
    unsigned char newMap(const char* mapPath, const char* mapName,
                         int* playerHeroFaces, int gameVersion);
    void setupFirstPlayer();
    bool loadMap(AbstractFile* mapFile);
    void applyMapHeaderAvailability();
    void readMapHeroSetups(AbstractFile* mapFile, int mapVersion);
    void randomizeHolyGrail();
    void randomizeEvents();
    void processOnMapTowns();
    void processOnMapHeroes();
    void initRandomArtifacts();

private:
    // Raw LF_FIELDLIST 0x3edc orders this shared pair as written. Its
    // private access flag is DC-only: retail decorates both as public QAEX.
    void matchUndergroundGates();
    void randomizeUniversity(NewmapCell* cell);

public:
    void setRecruits(int player);
    void replaceRecruit(int playerPos, long recruitSlot);
    bool growCoverOfDarkness();
    void initNewGame(int difficulty, int version,
                     NewSMapHeader* mapHeader, AbstractFile* infile);
    void resetGame(int difficulty, int version, NewSMapHeader* mapHeader);

private:
    int save(AbstractFile* outfile);  // 0x4be3f0
    void setCannedRumour();
    void setMapRumour();
    void setSpecialRumour();

public:
    void clearEventRecords();
    void recordShowHero(Hero* who, signed char player, type_point point,
                          unsigned char reset);  // 0x49cb20
    void processRandomObjects();  // 0x4c9dd0
    // The random-object pass and the monster roll it drives. Both bodies
    // are claimed in game.cpp.
    CreatureType getRandomMonster(int minLevel, int maxLevel);  // 0x4c92c0
    int computeDailyGold(int player, unsigned char includeSilo);
    void cancelComputerScreen();
    void makeTerrainVisible(int whichPlayer, unsigned short visMask);
    // 0x4c9990. town.obj needs this declaration for
    // town::destroy_extra_capitol; keeping it TU-scoped preserves the
    // retail-sensitive game member population in the other compilands.
    // game.obj joins on its own gate for ProcessRandomObjects, which
    // calls it once per random-object case.
    void convertObject(NewmapCell* tempCell);
    // The four remaining recorders, all located by the same vtable-store
    // evidence as their claimed siblings: each expands `new type_record_X`
    // in line and the class it constructs is named by the derived vftable
    // it stores last (0x63deec / 0x63df34 / 0x63de8c / 0x63dea4). Arity is
    // retail's own `ret` immediate - 0xc, 0xc, 0xc, 8. GATED to
    // event_record.obj until a real caller elsewhere needs one: game.h
    // rides in every compiland's closure and this header's declarator
    // population is codegen-sensitive.
    void recordHideBoat(boat* currentBoat, unsigned char occupied,
                          int occupyingHero);  // 0x49c560
    void recordMove(Hero* who, int direction,
                     type_point destination);  // 0x49cd50
    void recordTeleport(Hero* who, type_point destination);  // 0x49cf50
    void showLuckInfo(Hero* who, int dialogType);
    void showMoraleInfo(Hero* who, int dialogType);
    void recordHideHero(Hero* who, char newOwner,
                          unsigned char townGarrison);
    // 0x4c86a0. town::hire passes the player id and consumed two-slot
    // recruit index; hero::hire uses the same closeout call. The body
    // remains outside the admitted surface.
    void finishTownHire(long playerId, int recruitSlot);
    // Dreamcast's public symbol is `?OnSameTeam@game@@QBA_NHH@Z`: bool,
    VA(0x005296d0, 0x37)  // hd-crossbuild + anchor-callee x3, dc 0x1febc
    bool onSameTeam(int player1, int player2) const
    {
        if (player1 < 0 || player2 < 0)
            return 0;
        return m_mapHeader.m_teamInfo[player1] == m_mapHeader.m_teamInfo[player2];
    }
    // 0x4c6690, and the Dreamcast's own `?get_alignment@game@@QBA?AW4
    // TTownType@@H@Z` (game.h:1375, i.e. a header inline - which is why
    // Own the retained inline body here with the game interface. The selected
    // retail copy is in philai.obj; emission does not give that TU ownership.
    VA(0x00529710, 0x34)
    CreatureType upgradedCreatureType(CreatureType creature) const
    {
        if (m_f1f698 == 0
            && (creature == CREATURE_AIR_ELEMENTAL
                || creature == CREATURE_EARTH_ELEMENTAL
                || creature == CREATURE_FIRE_ELEMENTAL
                || creature == CREATURE_WATER_ELEMENTAL))
            return CREATURE_NONE;
        return ::upgradedCreatureType(creature);
    }
    // the Dreamcast decoration is `?is_human_ally@game@@QBA_NH@Z` and
    // for ClaimTown; the canonical body is below in CodeView source order.
    bool isHumanTeam(int teamNum) const
    {
        for (int player = 0; player < 8; ++player) {
            if (m_mapHeader.m_teamInfo[player] == teamNum && g_game->isHuman(player))
                return true;
        }
        return false;
    }
    // Dreamcast Game.h:856 proves ClaimTown's source-visible
    // IsComputerTeam boundary. Complete keeps the same boundary but its
    // retail lowering calls the exact is_human_ally COMDAT above; retaining
    // the wrapper is what preserves the materialized logical negation.
    inline unsigned char isComputerTeam(int teamNum) const
    {
        if (teamNum < 0)
            return 0;
        return !isHumanAlly(teamNum);
    }
    VA(0x004a5960, 0x16)  // exact selected events.obj COMDAT, dc 0x37fbc
    int getTeam(int playerNum) const
    {
        if (playerNum < 0)
            return playerNum;
        return m_mapHeader.m_teamInfo[playerNum];
    }
    // Game.h:877. DispatchEvent's obelisk arm preserves this named helper;
    // retail /Ob2 folds both it and GetTeam into the arm. MoveHero's
    // Dreamcast line stream names the same nested pair, and Complete folds
    // both while retaining GetTeam's selected events.obj COMDAT.
    unsigned char getTeamMask(int playerNum) const
    {
        unsigned char mask = 0;
        if (playerNum >= 0 && playerNum < 8) {
            int team = getTeam(playerNum);
            for (int i = 0; i < 8; ++i) {
                if (m_mapHeader.m_teamInfo[i] == team)
                    mask |= 1 << i;
            }
        }
        return mask;
    }
    // Game.h:897. Dreamcast emits this header helper after the town-gate
    // callback and records one nested GetTeam call. Complete expands the
    // same source boundary into TTownGateWindow's constructor: retail's
    // range guard, signed teamInfo load and eight-entry count are exact.
    unsigned char getNumAllies(int playerNum) const
    {
        unsigned char numAllies = 0;
        if (playerNum >= 0 && playerNum < 8) {
            int team = getTeam(playerNum);
            for (int i = 0; i < 8; ++i) {
                if (m_mapHeader.m_teamInfo[i] == team)
                    ++numAllies;
            }
        }
        return numAllies;
    }
    // Game.h:917, GetInfoFlag's setter twin. It marks the whole of
    // playerNum's TEAM, which is why every events.obj handler that
    // visits a global-info object ends in an eight-iteration teamInfo
    // scan rather than a single OR. The Dreamcast statement/call row names
    // GetTeam for the source-level team lookup; retail VC6 then decides per
    // expansion whether that tiny nested helper remains a call or folds to
    // the signed teamInfo load.

    // As with GetInfoFlag above, VC6 accepts the elaborated enum before
    // advmgr.h supplies its definition, retaining the attested parameter
    // domain even in this include order.
    void setInfoFlag(enum GlobalInfoFlags flag, const int playerNum)
    {
        if (playerNum < 0 || playerNum >= 8)
            return;
        int team = getTeam(playerNum);
        for (int i = 0; i < 8; i++) {
            if (m_mapHeader.m_teamInfo[i] == team)
                m_globalInfoFlags[flag] |= 1 << i;
        }
    }
    // DC-attested inline helper. Retail's shrine consumer proves the signed
    // [0,8) player guard and the byte bitset at +0x4e344.
    unsigned char getInfoFlag(enum GlobalInfoFlags flag, const int playerNum) const
    {
        if (playerNum < 0 || playerNum >= 8)
            return 0;
        return (m_globalInfoFlags[flag] & (1 << playerNum)) != 0;
    }
    void playRecordedEvents();
    unsigned char replayAvailable() const;
    int getNumThievesGuilds(int whichPlayer);

private:
    unsigned char saveRecordedEvents(AbstractFile* outfile);
    // declarator (`?load_recorded_events@game@@AAA_NPAX@Z`, private,
    unsigned char loadRecordedEvents(AbstractFile* infile, int version);
    // 0x4bcb30 (dc 0xa8144, E:\gamedcs\game.cpp:2975,
    // `?setup_shipyards@game@@AAAXXZ`). Clears all eight
    // players[i].shipyards and re-derives them by sweeping the map,
    // temporarily restoring any hero or boat obscuring a cell so the
    // shipyard underneath is visible. game::Load's tail is the caller
    // the Dreamcast xref graph records.
    void setupShipyards();

public:
    void viewArmy(ArmyGroup& group, int iarmy, const Hero* thisHero,
                  const Town* thisTown, int x, int y,
                  unsigned char showDismiss, unsigned char isQuickView);
    void overview();
    VA(0x004317d0, 0x26)  // hd-crossbuild + exact body/callers x15, dc 0x2eb0
    Hero* getHero(int which)
    {
        if (which == -1)
            return 0;
        return m_heroes + which;
    }
    // DC `game::GetCurrHero` (dc 0x2ed4, E:\gamedcs\Game.h:991) and
    // `game::GetCurrTown` (dc 0x1ff40, Game.h:1023) - the acting player's
    // pair, and NOT GetHero/GetTown applied to the id. Two retail facts
    // separate them from the general accessors, and both are visible in
    // TBottomViewTown/TBottomViewHero, whose Dreamcast bodies call these
    // by name where ours spelled the general accessor:
    //   * the id is compared at CHAR width and only widened INSIDE the
    //     taken arm (`mov al,[player+0x3f] / cmp al,-1 / je / movsx eax,al`),
    //     which is what re-reading the field in the arm produces and what
    //     passing it through an `int` parameter cannot - the widening would
    //     then dominate the test;
    //   * the null arm comes LAST (`je` to it, body falls through) and
    //     reuses whatever zero register is already live, i.e. the source
    //     tests `!= -1` and returns the pointer first.
    // GetHero/GetTown keep the opposite spelling because TBottomViewKingdom
    // and playerData::HasCapitol prove theirs; these are separate members,
    // so the two spellings do not contend.
    // A THIRD body agrees on the arm order: advManager::DoEvent (0x4aaaa0)
    // expands GetCurrHero with the non-null arm falling through and the
    // null arm placed after, whereas GetHero's `if (id == -1) return 0;`
    // lays the arms out the other way round. DC sizes them apart too - 68 B
    // against GetHero's 36 - so this is a separate inline, not a forwarder.
    Hero* getCurrHero()
    {
        if (g_currentPlayer->m_currHeroId != -1)
            return &m_heroes[g_currentPlayer->m_currHeroId];
        return 0;
    }
    // DC-attested inline Game.h member (dc 0x2f18). Retail CheckCastSpell
    // expands it to the acting player's widened currHero load; no standalone
    // retail row exists in the adventure-map header-method bracket.
    int getCurrHeroId()
    {
        return g_currentPlayer->m_currHeroId;
    }
    VA(0x0042ba30, 0x24)  // hd-crossbuild + exact body/callers x5, dc 0x2f24
    Town* getTown(int townId)
    {
        if (townId == -1)
            return 0;
        return &m_towns[townId];
    }
    Town* getCurrTown()
    {
        if (g_currentPlayer->m_currTownId != -1)
            return &m_towns[g_currentPlayer->m_currTownId];
        return 0;
    }
    bool townAlreadyBuiltOn(int townId) const;
    // DC `game::GetTownName` (?GetTownName@game@@QBAPBDH@Z), and another
    // inline-only member: retail has no out-of-line row and
    // townManager::SetupTown 0x5c68a4 expands it in place - the towns
    // vector's _First out of +0x21614, the 360-byte stride, +0xc4 for
    // cName and its own `_Ptr == 0 ? "" : _Ptr`. Note it does NOT go
    // through GetTown: no `cmp id,-1` is emitted at that site.

    // GATED, for the reason town::get_location's note gives: this
    // header rides in initialize.cpp's closure, which carries the
    // tree's include-set canary. Every consumer opens the macro for
    // itself and re-measures.
    const char* getTownName(int townId) const
    {
        return m_towns[townId].m_name.c_str();
    }
    // Game.h:1056. GetGarrison is expanded into both DispatchEvent and
    // philai's value_of_garrison; its nested vector access remains visible
    // so the recovered source hierarchy is not flattened again.
    garrison* getGarrison(int which) { return &m_garrisons[which]; }
    // DC `game::GetBoat`, declared inline in Game.h. Retail has no
    // out-of-line row; map-cell consumers expand the 40-byte vector indexing
    // directly at their call sites.
    boat* getBoat(int which)
    {
        return &m_boats[which];
    }

    // The end-turn body, game.obj's own at 0x4c6fe0. Also ORDER-MAPPED: it
    // abuts the claimed TurnOffAIMusic (0x4c6fd0, 0x10 B) exactly, and
    // game::NextPlayer (dc 0xb1fd0) is the very next DC roster row after
    // TurnOffAIMusic (dc 0xb1fc0). ProcessDeSelect's END_TURN arm calls it
    // once the "heroes can still move" confirm is past.
    void nextPlayer();
    // DC Game.h:1197. The Dreamcast keeps this header helper as a row;
    // retail expands the map's byte flag plus one at both cheat loops.
    int getNumMapLevels() { return m_worldMap.getNumLevels(); }
    void showScenInfo();

    // DC Game.h:1197. The Dreamcast keeps this header helper as a row;
    // retail expands the map's byte flag plus one at both cheat loops.
    // DC game.h:1405, dc 0x12cabc: const reference to the whirlpool list.
    // searchArray::enterTrigger passes its address to enterLith in retail.
    inline const std::vector<type_point>& getWhirlpools() const;
    NewmapCell* getCell(type_point point);
    void getLossConditionText(char* text);
    void getVictoryConditionText(char* text);
};

// The five .def-name tables game::ConvertObject (0x4c9990) rewrites a
// converted object's CObjectType::ImageName from. Their sole reader in
// the whole image is that body (config/retail-reloc-evidence.tsv rows
// 0xc9a33 / 0xc9a42 / 0xc9b04 / 0xc9b50 / 0xc9b63), so they are declared
// on game.obj's own gate. Contents read from the hash-verified image:
// the resource row is avtwood0/avtmerc0/avtore0/avtsulf0/avtcrys0/
// avtgems0/avtgold0.def in the seven-resource order the tree already
// uses, and the three town rows are AVCcast0..AVChfor0 (village),
// AVCcasx0..AVChforx (fort) and AVCcasz0..AVChforz (capitol), nine
// entries each in TTownType order. Names are house placeholders - no DC
// roster row covers any of the five.
DATA(0x00677958) extern const char* g_resourceObjectDefs[NUM_RESOURCES];
DATA(0x00677974) extern const char* g_artifactObjectDefFormat;
DATA(0x00677a0c) extern const char* g_townVillageObjectDefs[9];
DATA(0x00677a30) extern const char* g_townFortObjectDefs[9];
// Calendar-state globals saved across advManager::LoadRemote. Dreamcast
// supplies the names; retail fixes these four dword cells and their paired
// reset/restore use around game::LoadGame.
DATA(0x00677a54) extern const char* g_townCapitolObjectDefs[9];
DATA(0x00697750) extern int g_weekType;
DATA(0x006983fc) extern int g_weekTypeExtra;
DATA(0x00697748) extern int g_monthType;
// Shared UI text table: attack, defense, spell power, and knowledge.
DATA(0x00698834) extern int g_monthTypeExtra;
// The map's live width and height, Dreamcast-named (`?MAP_WIDTH@@3HA` /
// `?MAP_HEIGHT@@3HA` in kb.obj's PlayerDead scan) and initialised to 72 -
// a Medium map - in retail's .data.  96 retail bodies reference the pair,
// so it belongs in this header rather than any one consumer's; kb.obj's
// PlayerDead is the byte-proven reader here, walking y over MAP_HEIGHT and
// x over MAP_WIDTH while indexing worldMap by its own Size.
DATA(0x006a5390) extern const char* g_primarySkillNames[4];
DATA(0x006783c8) extern int g_mapWidth;
DATA(0x006783cc) extern int g_mapHeight;
DATA(0x00677978) extern int g_mineProduction[6];
// Six weighted neutral-town dwelling levels, byte-proven as
// {2,3,4,5,4,3} by game::GiveTroopsToNeutralTown.
DATA(0x00677998) extern double g_productionHandicap[];
DATA(0x006779b0) extern const int g_neutralTownLevelWeights[6];
// NewMap's seven-resource rows, indexed by setup.difficulty.  The first
// address is also the seven-int tutorial row immediately following the
// neutral-town weights above.
DATA(0x006779c8) extern const int g_neutralTownLevelWeightsEnd;
DATA(0x00678170) extern const int g_initResourcesHuman[][NUM_RESOURCES];
// NewMap reads one dword per player here before narrowing the selected value
// into setup.startingBonus.  The other known readers do not yet prove a
// broader semantic name, so keep the address-bearing role provisional.
DATA(0x006781fc) extern const int g_initResourcesComputer[][NUM_RESOURCES];
// SetupFirstPlayer writes its first-human scan result here alongside
// gNetLocalGamePos.  StartLocalPlayerTurn later consumes the same cell;
// no surviving symbol attests a semantic spelling.
DATA(0x0069fbf8) extern int g_newMapStartingBonus[8];
// remote.obj owns the DATA claim. NextPlayer consumes the adjacent recovery
// latch while retrying a failed turn-state transfer.
DATA(0x0069d810) extern int g_unnamed69d810;
extern unsigned char g_unnamed69d80d;
// advmgr.cpp owns the retail datum; ResetGame only clears the turn-control
// latch after rebuilding the session.
extern int g_thisNetGotAdventureControl;
DATA(0x0067814c) extern int g_heroGoldCost;
// Save version 41 added this signed-byte session value. game::Load owns the
// restore path; advManager also updates it when the local player finds the
// Holy Grail. Its wider role is not yet byte-proven.
DATA(0x0069774c) extern unsigned char g_unk69774c;
// One-byte session latch reset by game::SetupOrigData. No surviving symbol
// names its wider role, so retain the address-ordinal spelling.
DATA(0x0069950c) extern int g_unnamed69950c;
// Eight ints indexed by PLAYER, and readHeroData (0x5021c0) CONSUMES an
// entry: `movsx eax,[owner] / mov ecx,[4*eax + 0x69fb24]`, and when that is
// not -1 it becomes the hero id and the slot is stored -1 again. A reserved
// id therefore wins over GetStartingHeroId, exactly once. game::Init
// (0x4bf1a0) is what fills the array with -1 to begin with
// (`mov edi,0x69fb24 / mov ecx,8 / or eax,-1 / rep stosd`), so game.obj owns
// the definition; this is the shared declaration.

// No Dreamcast or NH3API symbol covers it, so the spelling stays ordinal on
// gUnnamed69950c's precedent rather than inventing a role name.
DATA(0x0069951c) extern unsigned char g_unnamed69951c;
DATA(0x0069fb24) extern int g_unnamed69fb24[8];
// Dreamcast public `iCurHourGlassPhase`; game.cpp owns the retail word and
// philAI::DoAI advances it as computer heroes are processed.
extern int g_curHourGlassPhase;
// Retail-only companion word cleared beside the hourglass phase by
// philAI::GetTurnAIVars. It has no surviving source symbol or other reader.
extern int g_unnamed691680;
// Retail .bss 0x69ccc4, and the SIBLING of advmgr.h's gMapVisibilityBit
// (0x69ccbc) rather than an alias of it - it has 38 relocation sites of
// its own, and advManager::ProcessHover gates fog on it with the same
// `test byte ptr [...], al` shape. game::Load's tail writes the pair one
// after the other and that is what separates them: 0x69ccbc takes
// `1 << gUnnamed69778c` (the acting player) while this one takes
// `1 << gNetLocalGamePos` (this machine's own seat). NAME UNATTESTED -
// address-ordinal placeholder, as gUnnamed69778c is.
// 0x69954c, extern-only here: kbwin.cpp owns the DATA claim under the
// name bVideoPaused, which recruit.cpp already records as CONTRADICTED
// with the storage correct. remote.h spells the same word
// gNetworkActive69954c and game::Load's use agrees with remote.h - not
// networked means the acting player IS the local seat.
DATA(0x0069ccc4) extern unsigned char g_unnamed69ccc4;
extern int g_networkActive69954c;
// E:\gamedcs\philai.cpp:4126, `?AI_examine_map@@YAXXZ`); declared here
void __cdecl aiExamineMap();
// hero.cpp owns the DATA claim on 0x698400 (name unattested,
// address-ordinal placeholder) and game.obj is a second reader, so this
// is an extern-only declaration - the MAP_WIDTH/gpCurrentPlayer pattern.
// hero.cpp's note already records THIS call site: every reader treats
// nonzero as "suppress the interactive path", and game::ClaimTown skips
// its notify call.
extern int g_inSetup698400;

// --- the local-player pair, read by GetLocalPlayer and
// GetLocalPlayerGamePos (both in this TU). The mode selector they
// branch on is iMPNetProtocol, in netgame.h.
// 0x69cca8: the hot-seat game position of this machine's player - the
// dword eight bytes ahead of gpCurrentPlayer, and range-checked
// against [0,8) before use. Ordinal placeholder.
extern int g_netLocalGamePos;                // .bss 0x69cca8
extern unsigned char g_unnamed69ccc4;

void startAITheme();
// 0x699554: the same answer for every other protocol, handed back
// unchecked. Ordinal placeholder.
extern int g_localGamePos;                   // .bss 0x699554
// 0x6a7df8: eight char* colour names; playerData::GetName copies
// gPlayerColorNames[color] over an empty/default name. Defined by a TU
// not yet located - extern only (the bitNumber pattern).
extern char* g_playerColorNames[];           // .bss 0x6a7df8
// SetSpecialRumour shares the nine-way direction table with seer-hut quest
// descriptions, and indexes the terrain-name table by a Grail cell's ground
// set when producing the alternative location hint.
extern const char* g_questMonsterDirections[9];
DATA(0x006a5d24) extern const char* const g_grailTerrainNames[];

// Located game.cpp bodies kbwin calls (the Imm/tablet mouse hooks;
// bodies not yet reconstructed - declarators match the kbwin call
// sites).
// ForceFeedback.cpp owns the Immersion initializer and window-move bodies.

unsigned char initImmMouse(void* instance, void* hwnd);  // 0x4b6890
void immMouseWindowMoved();
unsigned char playImmEffect(const char* effectName, int count);  // 0x4b69f0
unsigned char initializeRandomTavernText();
void computeUALoc(int whichPlayer);                   // 0x4baed0

// Canonical Game.h inline definitions after all referenced layouts/globals.

// Complete save files use the H3SVG signature and version 42.
// E:\gamedcs\Game.h:1301, dc 0xbceb4
VA(0x004bc0e0, 0x251)
inline SavedGameHeader::SavedGameHeader()
{
    memset(m_id, 0, sizeof(m_id));
    strcpy(m_id, "H3SVG");
    m_version = 42;
}

// E:\gamedcs\Game.h:1312, dc 0xbcf00
VA(0x004bc350, 0x271)  // anchor-caller (game::Save) + layout, dc 0xbcf00
inline void SavedGameHeader::reset()
{
    if (g_unk69774c)
        strcpy(m_id, "H3SVC");
    else
        strcpy(m_id, "H3SVG");

    m_version = 42;
    m_gameVersion = g_game->m_f1f698;

    m_campaign = g_game->m_campaign;

    m_mapHeader = g_game->m_mapHeader;

    m_currentPlayer = g_netLocalGamePos;
    m_mapSetup = g_game->m_setup;
    m_campaignGame = g_unk69774c;
    m_fileName = g_game->m_saveFileName;
    m_difficultyRating = g_game->m_difficultyRating;
    m_numDeadPlayers = g_game->m_numDeadPlayers;
    memcpy(m_deadPlayer, g_game->m_playerDisabled, sizeof(m_deadPlayer));

    int* human = m_humanPlayer;
    for (int i = 0; i < 8; ++i)
        *human++ = g_game->m_players[i].isHuman();
}

// Complete serializes the expanded snapshot through its abstract stream.
// Preserve the disjoint scalar staging scopes used by retail stack slots.
// E:\gamedcs\Game.h:1325, dc 0xbcf6c
VA(0x004bc5d0, 0x17A)  // anchor-layout + game::Save caller
inline int SavedGameHeader::save(AbstractFile* outfile)
{
    char fileNameBuffer[0x15f];
    char compatibilityBuffer[32];

    outfile->write(m_id, sizeof(m_id));

    {
        int buffer = m_version;
        outfile->write(&buffer, sizeof(buffer));
    }
    {
        int buffer = m_gameVersion;
        outfile->write(&buffer, sizeof(buffer));
    }

    if (outfile->write(compatibilityBuffer, sizeof(compatibilityBuffer)) <
        sizeof(compatibilityBuffer))
        return -1;

    if (m_mapHeader.save(outfile) < 0)
        return -1;
    if (m_mapSetup.save(outfile) < 0)
        return -1;

    {
        short buffer = m_campaignGame;
        outfile->write(&buffer, sizeof(buffer));
    }
    if (m_campaignGame)
        m_campaign.save(outfile);

    strcpy(fileNameBuffer, m_fileName.c_str());
    outfile->write(fileNameBuffer, sizeof(fileNameBuffer));

    {
        short buffer = m_difficultyRating;
        outfile->write(&buffer, sizeof(buffer));
    }
    {
        char buffer = m_numDeadPlayers;
        outfile->write(&buffer, sizeof(buffer));
    }
    outfile->write(m_deadPlayer, sizeof(m_deadPlayer));
    outfile->write(m_humanPlayer, sizeof(m_humanPlayer));
    {
        int buffer = m_currentPlayer;
        outfile->write(&buffer, sizeof(buffer));
    }

    return 0;
}

// Complete reads versioned nested records through the abstract stream;
// Dreamcast uses gzread directly.
// E:\gamedcs\Game.h:1344, dc 0xbcfe4
VA(0x004bc750, 0x3D5)  // dc 0xbcfe4
inline int SavedGameHeader::load(AbstractFile* infile)
{
    std::string openedName;
    unsigned char inputWasProvided = infile != 0;
    std::auto_ptr<AbstractFile> ownedInput;

    if (!inputWasProvided) {
        openedName = g_game->m_setup.m_filename;
        _chdir("games");
        try {
            infile = new GzFile(openedName.c_str(), "rb");
            ownedInput = std::auto_ptr<AbstractFile>(infile);
        }
        catch (GzFile::OpenFailure) {
            return -1;
        }
        _chdir("..");
        if (!infile)
            return -1;
    }

    if (infile->read(m_id, sizeof(m_id)) < sizeof(m_id))
        return -1;

    {
        int buffer;
        infile->read(&buffer, sizeof(buffer));
        m_version = buffer;
    }
    if (m_version > 42)
        return -1;

    if (m_version >= 40) {
        int buffer;
        infile->read(&buffer, sizeof(buffer));
        m_gameVersion = buffer;
    } else {
        if (m_version < 25 && (m_version < 16 || m_version > 18))
            return -1;
        if (m_version <= 18)
            m_gameVersion = 0;
        else if (m_version <= 30)
            m_gameVersion = 1;
        else
            m_gameVersion = 2;
    }

    if (m_gameVersion == 1 &&
        *g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_LOW)
        return -1;

    char compatibilityBuffer[32];
    infile->read(compatibilityBuffer, sizeof(compatibilityBuffer));
    if (m_mapHeader.load(infile, m_version) < 0)
        return -1;
    if (m_mapSetup.load(infile, m_version) < 0)
        return -1;

    {
        short buffer;
        infile->read(&buffer, sizeof(buffer));
        m_campaignGame = buffer != 0;
    }
    if (m_campaignGame)
        m_campaign.load(infile, m_version);

    char fileNameBuffer[0x15f];
    infile->read(fileNameBuffer, sizeof(fileNameBuffer));
    m_fileName = fileNameBuffer;

    {
        short buffer;
        infile->read(&buffer, sizeof(buffer));
        m_difficultyRating = buffer;
    }
    {
        char buffer;
        infile->read(&buffer, sizeof(buffer));
        m_numDeadPlayers = buffer;
    }
    infile->read(m_deadPlayer, sizeof(m_deadPlayer));
    infile->read(m_humanPlayer, sizeof(m_humanPlayer));
    {
        int buffer;
        infile->read(&buffer, sizeof(buffer));
        m_currentPlayer = buffer;
    }

    if (!inputWasProvided)
        strcpy(m_mapSetup.m_filename, openedName.c_str());
    return 0;
}

// E:\gamedcs\Game.h:1370, dc 0x37fd8
VA(0x0042b9e0, 0x45)  // dc 0x37fd8
inline bool game::isHumanAlly(int teamNum) const
{
    if (teamNum >= 0) {
        for (int player = 0; player < 8; ++player) {
            if (m_mapHeader.m_teamInfo[player] == teamNum
                && g_game->isHuman(player))
                return true;
        }
    }
    return false;
}

// E:\gamedcs\Game.h:1375, dc 0x2000c
VA(0x004c6690, 0x43)  // dc 0x2000c
inline int game::getAlignment(int creature) const
{
    if (!m_f1f698
        && (creature == CREATURE_AIR_ELEMENTAL
            || creature == CREATURE_EARTH_ELEMENTAL
            || creature == CREATURE_FIRE_ELEMENTAL
            || creature == CREATURE_WATER_ELEMENTAL))
        return -1;
    return g_creatureTypeTraits[creature].m_townType;
}

// Game.h:1380. DispatchEvent expands this cell accessor; the
// out-of-line copy is ai_player.obj's, 0x42ed80.
// E:\gamedcs\game.h:1380. Retail retains this header-inline copy in
// ai_player.obj; all consumers use the same canonical body.
VA(0x0042ed80, 0x4D)  // anchor-global, dc 0x38000
inline NewmapCell* game::getCell(type_point point)
{
    return m_worldMap.cell(point.m_x, point.m_y, point.m_z);
}

// Game.h:1390 in the DC roster. Retail expands this short calendar
// accessor at every game.obj call site and retains no standalone row.
inline short game::getCurrentTurn() const
{
    return (m_month * 4 + m_week - 5) * 7 + m_day;
}

// E:\gamedcs\Game.h:1405.
inline const std::vector<type_point>& game::getWhirlpools() const
{
    return m_whirlpools;
}

// Dreamcast Game.h:1410 names this ordinary inline query and retains a
// selected out-of-line copy in ai_player.obj. THallWindow expands the
// same source operation to the retail town-vector lookup.
inline bool game::townAlreadyBuiltOn(int townId) const
{
    return m_towns[townId].m_builtThisTurn != 0;
}

// --- globals ---
// CODEVIEW(E:\gamedcs\game.cpp:365, dc 0xa2bb4) int bufwrite(const void* buf, int size);
// CODEVIEW(E:\gamedcs\game.cpp:396, dc 0xa2d5c) int bufread(void* buf, int size);
// CODEVIEW(E:\gamedcs\game.cpp:627, dc 0xa341c) long get_day_bonus(EGameResource resource, long week_bonus, long day);
// CODEVIEW(E:\gamedcs\game.cpp:2448, dc 0xa7320) void GenerateStandardFileName(char* cLongName, char* cRetName);
// CODEVIEW(E:\gamedcs\game.cpp:3275, dc 0xa8ba0) int hero_power(hero* this_hero);
// CODEVIEW(E:\gamedcs\game.cpp:3288, dc 0xa8c6c) int compare_heroes(const void* arg1, const void* arg2);
// CODEVIEW(E:\gamedcs\game.cpp:4509, dc 0xab96c) void RandomizeScholar(NewmapCell* cell);
// CODEVIEW(E:\gamedcs\game.cpp:4524, dc 0xab9d4) void RandomizeArtifact(NewmapCell* cell);
// CODEVIEW(E:\gamedcs\game.cpp:4613, dc 0xabc9c) void RandomizeSeaChest(NewmapCell* cell);
// CODEVIEW(E:\gamedcs\game.cpp:4639, dc 0xabd4c) void RandomizeShrine(NewmapCell* cell, const int level);
// CODEVIEW(E:\gamedcs\game.cpp:4654, dc 0xabda8) void randomize_wagon(NewmapCell* cell);
// CODEVIEW(E:\gamedcs\game.cpp:4681, dc 0xabe30) void RandomizeWiseTree(short id, NewmapCell* cell);
// CODEVIEW(E:\gamedcs\game.cpp:4691, dc 0xabe90) void RandomizeTreasure(NewmapCell* cell);
// CODEVIEW(E:\gamedcs\game.cpp:4724, dc 0xabf78) void randomize_tomb(NewmapCell* cell);
// CODEVIEW(E:\gamedcs\game.cpp:4753, dc 0xabfe8) void randomize_pyramid(NewmapCell* cell);
// CODEVIEW(E:\gamedcs\game.cpp:4804, dc 0xac168) void randomize_witch_hut(NewmapCell* cell);
// CODEVIEW(E:\gamedcs\game.cpp:8290, dc 0xb3e00) THeroID get_new_hero(THeroClass hero_class);
// CODEVIEW(E:\gamedcs\game.cpp:9803, dc 0xb6944) const char* GetRandomTownName(int townType);
// CODEVIEW(E:\gamedcs\game.cpp:9821, dc 0xb69b8) void ResetRandomTownNames();
// CODEVIEW(E:\gamedcs\game.cpp:9912, dc 0xb6c84) void initialize_hero(hero* current_hero, const HeroExtra* setup);
// CODEVIEW(E:\gamedcs\game.cpp:11918, dc 0xbc500) unsigned char DCFileConv(char* name);
// CODEVIEW(E:\gamedcs\game.cpp:2698, dc 0xc184c) unsigned char load_vector(void* infile, std::vector<enum* dest_vector);
// CODEVIEW(E:\gamedcs\game.cpp:2716, dc 0xc18cc) unsigned char save_vector(void* outfile, std::vector<enum* src_vector);
// CODEVIEW(E:\gamedcs\game.cpp:2733, dc 0xc1950) unsigned char load_object_vector(void* infile, std::vector<generator,std::allocator<generator>* dest_vector);
// CODEVIEW(E:\gamedcs\game.cpp:2698, dc 0xc19e8) unsigned char load_vector(void* infile, std::vector<type_point,std::allocator<type_point>* dest_vector);
// CODEVIEW(E:\gamedcs\game.cpp:2698, dc 0xc1a68) unsigned char load_vector(void* infile, std::vector<long,std::allocator<long>* dest_vector);
// CODEVIEW(E:\gamedcs\game.cpp:2698, dc 0xc1ae8) unsigned char load_vector(void* infile, std::vector<type_university,std::allocator<type_university>* dest_vector);
// CODEVIEW(E:\gamedcs\game.cpp:2733, dc 0xc1b6c) unsigned char load_object_vector(void* infile, std::vector<type_creature_bank,std::allocator<type_creature_bank>* dest_vector);
// CODEVIEW(E:\gamedcs\game.cpp:2754, dc 0xc1d38) unsigned char save_object_vector(void* outfile, std::vector<generator,std::allocator<generator>* src_vector);
// CODEVIEW(E:\gamedcs\game.cpp:2716, dc 0xc1e58) unsigned char save_vector(void* outfile, std::vector<long,std::allocator<long>* src_vector);

// --- Buffer ---
// CODEVIEW(E:\gamedcs\game.cpp:348, dc 0xa2b5c) void Buffer::Buffer();
// CODEVIEW(E:\gamedcs\game.cpp:356, dc 0xa2b98) void Buffer::~Buffer();
// CODEVIEW(E:\gamedcs\game.cpp:2969, dc 0xbd4fc) void* Buffer::`scalar deleting destructor'(unsigned __flags);

// --- CDestroyPlayerMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:873, dc 0xbd3d0) void CDestroyPlayerMsg::CDestroyPlayerMsg(unsigned long dpid);

// --- CGameTransferDlg ---
// CODEVIEW(E:\gamedcs\game.cpp:10583, dc 0xbd5c8) void CGameTransferDlg::~CGameTransferDlg();

// --- CGameTransmitConfirmEndMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:374, dc 0xbd1e4) void CGameTransmitConfirmEndMsg::CGameTransmitConfirmEndMsg();

// --- CGameTransmitEndMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:389, dc 0xbd20c) void CGameTransmitEndMsg::CGameTransmitEndMsg(int iMonthType, int iMonthTypeExtra, int iWeekType, int iWeekTypeExtra, unsigned long diffSize);

// --- CGameTransmitInitMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:295, dc 0xbd0b4) void CGameTransmitInitMsg::CGameTransmitInitMsg(unsigned long fileSize, unsigned long fullGameCRC, unsigned long thisPlayerDead, unsigned char isDiff, unsigned char makeOrig);

// --- CGameTransmitMainMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:324, dc 0xbd13c) CGameTransmitMainMsg* CGameTransmitMainMsg::CreateMsg(unsigned long maxSize);
// CODEVIEW(E:\gamedcs\netmsg.h:337, dc 0xbd184) void CGameTransmitMainMsg::Update(unsigned char* pData, unsigned long blockSize);
// CODEVIEW(E:\gamedcs\netmsg.h:351, dc 0xbd1c8) unsigned long CGameTransmitMainMsg::GetSize();
// CODEVIEW(E:\gamedcs\netmsg.h:357, dc 0xbd1d8) unsigned char* CGameTransmitMainMsg::GetData();

// --- CGameTransmitReqMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:312, dc 0xbd10c) void CGameTransmitReqMsg::CGameTransmitReqMsg(int blockNbr);

// --- CGameXferAckMsg ---
// CODEVIEW(E:\gamedcs\netmsg.h:882, dc 0xbd400) void CGameXferAckMsg::CGameXferAckMsg();

// --- CMCBuildBoat ---
// CODEVIEW(E:\gamedcs\netmsg.h:649, dc 0xbd334) void CMCBuildBoat::CMCBuildBoat(type_point point, int playerPos);

// --- CMCClaimGarrison ---
// CODEVIEW(E:\gamedcs\netmsg.h:619, dc 0xbd290) void CMCClaimGarrison::CMCClaimGarrison(int garrisonId, int playerPos);

// --- CMCClaimGenerator ---
// CODEVIEW(E:\gamedcs\netmsg.h:605, dc 0xbd258) void CMCClaimGenerator::CMCClaimGenerator(int generatorId, int playerPos);

// --- CMCClaimShipYard ---
// CODEVIEW(E:\gamedcs\netmsg.h:633, dc 0xbd2c8) void CMCClaimShipYard::CMCClaimShipYard(type_point point, int playerPos);

// --- CMCHideHero ---
// CODEVIEW(E:\gamedcs\netmsg.h:717, dc 0xbd3a0) void CMCHideHero::CMCHideHero(int heroId);

// --- CObject ---
// CODEVIEW(E:\gamedcs\MapCell.h:595, dc 0xbc868) void CObject::CObject(unsigned char _x, unsigned char _y, unsigned char _z, unsigned short _type, unsigned long _extraInfo);

// --- CObjectType ---
// CODEVIEW(..\stlport\stl_alloc.h:970, dc 0xc2b2c) void CObjectType::CObjectType(const CObjectType* __that);
// CODEVIEW(..\stlport\stl_vector.h:706, dc 0xc8eb0) void* CObjectType::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(..\stlport\stl_vector.h:706, dc 0xc8ee8) void CObjectType::~CObjectType();

// --- CTimer ---
// CODEVIEW(E:\gamedcs\timer.h:39, dc 0xbd428) void CTimer::CTimer(unsigned char _enabled);
// CODEVIEW(E:\gamedcs\timer.h:49, dc 0xbd45c) void CTimer::start();
// CODEVIEW(E:\gamedcs\timer.h:60, dc 0xbd494) void CTimer::stop();

// --- ExtraInfoUnion ---
// CODEVIEW(E:\gamedcs\MapCell.h:969, dc 0xbc95c) void ExtraInfoUnion::clear_visited_bits();
// CODEVIEW(E:\gamedcs\MapCell.h:1012, dc 0xbc974) void ExtraInfoUnion::FillGarden(EGameResource resource);
// CODEVIEW(E:\gamedcs\MapCell.h:1028, dc 0xbc9b0) void ExtraInfoUnion::SetGarden(short id, EGameResource resource);
// CODEVIEW(E:\gamedcs\MapCell.h:1040, dc 0xbca04) void ExtraInfoUnion::SetMagicSpring(short id, unsigned char full);
// CODEVIEW(E:\gamedcs\MapCell.h:1084, dc 0xbca4c) void ExtraInfoUnion::SetScholar(ScholarAwards award, TPrimarySkill primary, TSecondarySkill secondary, SpellID spell);
// CODEVIEW(E:\gamedcs\MapCell.h:1176, dc 0xbcac8) void ExtraInfoUnion::SetWagon(EGameResource resource, short amount);
// CODEVIEW(E:\gamedcs\MapCell.h:1185, dc 0xbcb3c) void ExtraInfoUnion::SetWagon(TArtifact artifact);
// CODEVIEW(E:\gamedcs\MapCell.h:1208, dc 0xbcb94) void ExtraInfoUnion::set_tomb(TArtifact artifact);
// CODEVIEW(E:\gamedcs\MapCell.h:1251, dc 0xbcbdc) void ExtraInfoUnion::set_witch_skill(TSecondarySkill skill);

// --- LossConditionStruct ---
// CODEVIEW(E:\gamedcs\VictoryLossConditions.h:141, dc 0xbccfc) void LossConditionStruct::LossConditionStruct();

// --- NewSMapHeader ---
// CODEVIEW(E:\gamedcs\game.cpp:5687, dc 0xadf38) int NewSMapHeader::readVictoryCondition(char type, void* infile);
// CODEVIEW(E:\gamedcs\game.cpp:5921, dc 0xae5b4) int NewSMapHeader::saveVictoryCondition(char type, void* outfile);
// CODEVIEW(E:\gamedcs\game.cpp:6110, dc 0xaeb64) int NewSMapHeader::loadVictoryCondition(char type, void* infile);
// CODEVIEW(E:\gamedcs\game.cpp:6324, dc 0xaf118) int NewSMapHeader::readLossCondition(char type, void* infile);
// CODEVIEW(E:\gamedcs\game.cpp:6390, dc 0xaf2b4) int NewSMapHeader::saveLossCondition(char type, void* outfile);
// CODEVIEW(E:\gamedcs\game.cpp:6448, dc 0xaf488) int NewSMapHeader::loadLossCondition(char type, void* infile);
// CODEVIEW(E:\gamedcs\game.cpp:6513, dc 0xaf64c) int NewSMapHeader::Read(void* infile);
// CODEVIEW(E:\gamedcs\game.cpp:6792, dc 0xb0188) int NewSMapHeader::Save(void* outfile);
// CODEVIEW(E:\gamedcs\game.cpp:6974, dc 0xb0754) int NewSMapHeader::Load(void* infile);
// CODEVIEW(E:\gamedcs\game.cpp:7185, dc 0xb0ea8) int NewSMapHeader::Get(const char* filename);

// --- NewfullMap ---
// CODEVIEW(E:\gamedcs\MapCell.h:847, dc 0xbc8dc) const NewmapCell* NewfullMap::zCell(int x, int y, int z);
// CODEVIEW(E:\gamedcs\MapCell.h:889, dc 0xbc930) const NewmapCell* NewfullMap::cell(int x, int y, int z);

// --- SCampaign ---

// --- SGameSetupOptions ---
// CODEVIEW(E:\gamedcs\game.cpp:3257, dc 0xa8b48) int SGameSetupOptions::save(void* outfile);
// CODEVIEW(E:\gamedcs\game.cpp:3266, dc 0xa8b74) int SGameSetupOptions::load(void* infile);

// --- SavedGameHeader ---
// CODEVIEW(E:\gamedcs\Game.h:1301, dc 0xbceb4) void SavedGameHeader::SavedGameHeader();
// CODEVIEW(E:\gamedcs\Game.h:1312, dc 0xbcf00) void SavedGameHeader::Reset();
// CODEVIEW(E:\gamedcs\Game.h:1325, dc 0xbcf6c) int SavedGameHeader::Save(void* outfile);

// --- Sign ---
// CODEVIEW(E:\gamedcs\Game.h:578, dc 0xbce54) void Sign::Sign();
// CODEVIEW(..\stlport\stl_string.h:537, dc 0xbe808) void Sign::~Sign();
// CODEVIEW(..\stlport\stl_alloc.h:970, dc 0xc2abc) void Sign::Sign(const Sign* __that);
// CODEVIEW(..\stlport\stl_alloc.h:326, dc 0xc5f54) Sign* Sign::operator=(const Sign* __that);
// CODEVIEW(..\stlport\stl_vector.h:609, dc 0xc8d98) void* Sign::`scalar deleting destructor'(unsigned __flags);

// --- TAdventureMapWindow ---
// CODEVIEW(E:\gamedcs\AdventureMapWindow.h:238, dc 0xbd0a0) void TAdventureMapWindow::set_background_animation(unsigned char enable);

// --- TArtifactRequirement ---
// CODEVIEW(E:\gamedcs\CustomCampaign.h:95, dc 0xbcd20) void TArtifactRequirement::TArtifactRequirement();
// CODEVIEW(E:\gamedcs\CustomCampaign.h:102, dc 0xbcd40) void TArtifactRequirement::init();

// --- TCustomCampaignTraits ---
// CODEVIEW(E:\gamedcs\CustomCampaign.h:135, dc 0xbcd58) void TCustomCampaignTraits::TCustomCampaignTraits();

// --- TPickRandomTownName ---
// CODEVIEW(E:\gamedcs\includes.h:175, dc 0xbc7c8) void TPickRandomTownName::TPickRandomTownName();
// CODEVIEW(E:\gamedcs\includes.h:178, dc 0xbc7ec) void TPickRandomTownName::Reset();
// CODEVIEW(E:\gamedcs\game.cpp:9801, dc 0xbd5ac) void TPickRandomTownName::~TPickRandomTownName();

// --- VictoryConditionStruct ---

// --- boat ---
// CODEVIEW(E:\gamedcs\Hero.h:188, dc 0xbcc18) void boat::boat();

// --- game ---
// CODEVIEW(E:\gamedcs\game.cpp:643, dc 0xa3474) void game::calculate_production();
// CODEVIEW(E:\gamedcs\game.cpp:896, dc 0xa3e5c) int game::LoadMinePool(void* infile);
// CODEVIEW(E:\gamedcs\game.cpp:1235, dc 0xa4c08) int game::LoadObeliskPool(void* infile);
// CODEVIEW(E:\gamedcs\game.cpp:1256, dc 0xa4c68) int game::SaveObeliskPool(void* outfile);
// CODEVIEW(E:\gamedcs\game.cpp:1668, dc 0xa5998) int game::LoadPlayerData(void* infile);
// CODEVIEW(E:\gamedcs\game.cpp:1683, dc 0xa59ec) int game::SavePlayerData(void* outfile);
// CODEVIEW(E:\gamedcs\game.cpp:1698, dc 0xa5a40) int game::LoadTownPool(void* infile);
// CODEVIEW(E:\gamedcs\game.cpp:1722, dc 0xa5af8) int game::SaveTownPool(void* outfile);
// CODEVIEW(E:\gamedcs\game.cpp:1745, dc 0xa5b9c) int game::SaveHeroPool(void* outfile);
// CODEVIEW(E:\gamedcs\game.cpp:1760, dc 0xa5bf4) int game::LoadHeroPool(void* infile);
// CODEVIEW(E:\gamedcs\game.cpp:1999, dc 0xa6350) int game::SetupPuzzlePieces(int whichPlayer, int countOnly);
// CODEVIEW(E:\gamedcs\game.cpp:2158, dc 0xa67bc) int game::Scan(signed char* whichList, int start, int length);
// CODEVIEW(E:\gamedcs\game.cpp:2173, dc 0xa6850) int game::RandomScan(signed char* whichList, int start, int length, signed char scanValue);
// CODEVIEW(E:\gamedcs\game.cpp:2275, dc 0xa6cd4) THeroID game::GetNewHeroId(TTownType alignment, THeroClass excluded, unsigned char prefer_alignment);
// CODEVIEW(E:\gamedcs\game.cpp:2390, dc 0xa707c) int game::GetHeroId(type_point hero_location);
// CODEVIEW(E:\gamedcs\game.cpp:2405, dc 0xa710c) int game::GetMineId(int x, int y, int z);
// CODEVIEW(E:\gamedcs\game.cpp:2433, dc 0xa7278) int game::GetGarrisonId(int x, int y, int z);
// CODEVIEW(E:\gamedcs\game.cpp:2654, dc 0xa795c) int game::SaveBlackMarkets(void* outfile);
// CODEVIEW(E:\gamedcs\game.cpp:2672, dc 0xa7a24) int game::LoadBlackMarkets(void* infile);
// CODEVIEW(E:\gamedcs\game.cpp:2806, dc 0xa7bec) int game::GetSaveGameHeaders(void* infile);
// CODEVIEW(E:\gamedcs\game.cpp:2975, dc 0xa8144) void game::setup_shipyards();
// CODEVIEW(E:\gamedcs\game.cpp:3026, dc 0xa83d0) int game::Load(void* infile);
// CODEVIEW(E:\gamedcs\game.cpp:3311, dc 0xa8cd0) int game::Save(void* outfile);
// CODEVIEW(E:\gamedcs\game.cpp:3790, dc 0xa9e88) void game::SetupOrigData();
// CODEVIEW(E:\gamedcs\game.cpp:3842, dc 0xaa0d0) int game::LoadGame(const char* filename, int bIsOrigData, int bIsQuickLoad);
// CODEVIEW(E:\gamedcs\game.cpp:3953, dc 0xaa3f0) void game::GiveTroopsToNeutralTown(int iTownId);
// CODEVIEW(E:\gamedcs\game.cpp:4029, dc 0xaa6f8) void game::GiveTroopsToNeutralTowns();
// CODEVIEW(E:\gamedcs\game.cpp:4050, dc 0xaa7e0) void game::ValidateVictoryLossConditions(unsigned char check_map_locations);
// CODEVIEW(E:\gamedcs\game.cpp:4770, dc 0xac048) void game::randomize_university(NewmapCell* cell);
// CODEVIEW(E:\gamedcs\game.cpp:4950, dc 0xac63c) void game::match_underground_gates();
// CODEVIEW(E:\gamedcs\game.cpp:5003, dc 0xac910) void game::RandomizeEvents();
// CODEVIEW(E:\gamedcs\game.cpp:5600, dc 0xadb88) int game::LoadMap(char* mapName);
// CODEVIEW(E:\gamedcs\game.cpp:7572, dc 0xb1f1c) int game::GetRandomNumTroops(int whichMon);
// CODEVIEW(E:\gamedcs\game.cpp:7603, dc 0xb1fd0) void game::NextPlayer();
// CODEVIEW(E:\gamedcs\game.cpp:7958, dc 0xb3030) unsigned char game::GrowCoverOfDarkness();
// CODEVIEW(E:\gamedcs\game.cpp:8266, dc 0xb3d8c) void game::clear_recruits(THeroID* recruits);
// CODEVIEW(E:\gamedcs\game.cpp:8308, dc 0xb3e60) void game::set_weekly_recruits(THeroID* recruits, TTownType alignment);
// CODEVIEW(E:\gamedcs\game.cpp:8358, dc 0xb4050) void game::replace_recruit(THeroID* recruits, long recruit_slot, TTownType alignment);
// CODEVIEW(E:\gamedcs\game.cpp:8373, dc 0xb40f4) void game::set_recruits();
// CODEVIEW(E:\gamedcs\game.cpp:8398, dc 0xb41e0) void game::PerWeek();
// CODEVIEW(E:\gamedcs\game.cpp:8707, dc 0xb4b58) TCreatureType game::GetRandomMonster(int minLevel, int maxLevel);
// CODEVIEW(E:\gamedcs\game.cpp:8896, dc 0xb4fa0) void game::RandomizeHeroPool();
// CODEVIEW(E:\gamedcs\game.cpp:9195, dc 0xb54f8) void game::ConvertObject(NewmapCell* tempCell);
// CODEVIEW(E:\gamedcs\game.cpp:9455, dc 0xb5cdc) void game::CreateTownHeroes();
// CODEVIEW(E:\gamedcs\game.cpp:9660, dc 0xb635c) void game::ShowComputerScreen();
// CODEVIEW(E:\gamedcs\game.cpp:9720, dc 0xb6538) void game::ShowHeroesLogo();
// CODEVIEW(E:\gamedcs\game.cpp:9798, dc 0xb6878) void game::SetupTowns();
// CODEVIEW(E:\gamedcs\game.cpp:9833, dc 0xb69f4) void game::ProcessOnMapTowns();
// CODEVIEW(E:\gamedcs\game.cpp:10060, dc 0xb7204) void game::ProcessOnMapHeroes();
// CODEVIEW(E:\gamedcs\game.cpp:10132, dc 0xb7554) void game::CheckHeroConsistency();
// CODEVIEW(E:\gamedcs\game.cpp:10142, dc 0xb7560) int game::TransmitSaveGame(int iToWho, int thisPlayerDead, unsigned char inGame, unsigned char makeOrig);
// CODEVIEW(E:\gamedcs\game.cpp:10587, dc 0xb85c4) int game::ReceiveSaveGame(int iFileSize, int iFullGameCRC, int iFromWho, unsigned char inGame, unsigned char isDiff);
// CODEVIEW(E:\gamedcs\game.cpp:11221, dc 0xb9b54) int game::HeroIDToHeroPos(playerData* pPlayer, int id);
// CODEVIEW(E:\gamedcs\game.cpp:11231, dc 0xb9b98) int game::TownIDToTownPos(playerData* pPlayer, int id);
// CODEVIEW(E:\gamedcs\game.cpp:11241, dc 0xb9c04) void game::SetMarketArtifacts();
// CODEVIEW(E:\gamedcs\game.cpp:11252, dc 0xb9cac) void game::SetSummoningGenerators();
// CODEVIEW(E:\gamedcs\game.cpp:11311, dc 0xb9e7c) void game::SetMapRumour();
// CODEVIEW(E:\gamedcs\game.cpp:11445, dc 0xbac04) void game::SetupNewRumour();
// CODEVIEW(E:\gamedcs\game.cpp:11459, dc 0xbaca4) void game::GiveTimeEventReward(const TTimedEvent* thisEvent);
// CODEVIEW(E:\gamedcs\game.cpp:11499, dc 0xbaeb0) void game::GiveTownEventReward(const TTownEvent* thisEvent);
// CODEVIEW(E:\gamedcs\game.cpp:11512, dc 0xbaef4) void game::CheckForTimeEvent();
// CODEVIEW(E:\gamedcs\game.cpp:11684, dc 0xbb62c) void game::game();
// CODEVIEW(E:\gamedcs\game.cpp:11869, dc 0xbc320) int game::GetLastHuman();
// CODEVIEW(E:\gamedcs\game.cpp:11884, dc 0xbc384) void game::mark_campaign_map_won();
// CODEVIEW(E:\gamedcs\game.cpp:11895, dc 0xbc418) void game::ResetGame();
// CODEVIEW(E:\gamedcs\Game.h:856, dc 0xbce7c) unsigned char game::IsComputerTeam(int teamNum);
// CODEVIEW(E:\gamedcs\Game.h:1390, dc 0xbd05c) short game::get_current_turn();
// CODEVIEW(..\stlport\stl_string.h:537, dc 0xbe84c) void game::TRumour::TRumour();
// CODEVIEW(..\stlport\stl_string.h:537, dc 0xbe86c) void game::TRumour::~TRumour();
// CODEVIEW(..\stlport\stl_alloc.h:970, dc 0xc2af4) void game::TRumour::TRumour(const game::TRumour* __that);
// CODEVIEW(..\stlport\stl_alloc.h:326, dc 0xc5f8c) game::TRumour* game::TRumour::operator=(const game::TRumour* __that);
// CODEVIEW(..\stlport\stl_vector.h:609, dc 0xc8e08) void* game::TRumour::`scalar deleting destructor'(unsigned __flags);

// --- garrison ---
// CODEVIEW(..\stlport\stl_string.h:537, dc 0xbe828) void garrison::garrison();

// --- generator ---
// CODEVIEW(E:\gamedcs\game.cpp:503, dc 0xa30c4) void generator::remove_bonus();
// CODEVIEW(E:\gamedcs\game.cpp:557, dc 0xa3250) void generator::set_owner(long owner);

// --- mine ---
// CODEVIEW(E:\gamedcs\Game.h:458, dc 0xbce00) void mine::mine();

// --- playerData ---
// CODEVIEW(E:\gamedcs\game.cpp:1383, dc 0xa50ac) void playerData::SetName(char* cNewName);
// CODEVIEW(E:\gamedcs\game.cpp:1395, dc 0xa5138) void playerData::GetNetInfo(CNetPlayerInfo* pNetPlayerInfo);
// CODEVIEW(E:\gamedcs\game.cpp:1548, dc 0xa55a8) int playerData::save(void* outfile);
// CODEVIEW(E:\gamedcs\game.cpp:1873, dc 0xa5f30) int playerData::BuildingsOwned(int townType, int buildingId, int mageLevel);

// --- std ---
// CODEVIEW(..\stlport\stl_bvector.h:49, dc 0xbc59c) void std::_Bit_reference::_Bit_reference(unsigned* __x, unsigned __y);
// CODEVIEW(..\stlport\stl_bvector.h:56, dc 0xbc5b4) std::_Bit_reference* std::_Bit_reference::operator=(unsigned char __x);
// CODEVIEW(..\stlport\stl_bvector.h:162, dc 0xbc5f0) void std::_Bit_iterator_base::_M_advance(int __i);
// CODEVIEW(..\stlport\stl_bvector.h:174, dc 0xbc668) int std::_Bit_iterator_base::_M_subtract(const std::_Bit_iterator_base* __x);
// CODEVIEW(..\stlport\stl_bvector.h:578, dc 0xbc694) std::_Bit_iter<std::_Bit_reference,std::_Bit_reference std::vector<bool,std::allocator<bool> >::begin(__$ReturnUdt);
// CODEVIEW(..\stlport\stl_bvector.h:579, dc 0xbc6b8) std::_Bit_iter<bool,bool std::vector<bool,std::allocator<bool> >::begin(__$ReturnUdt);
// CODEVIEW(..\stlport\stl_bvector.h:581, dc 0xbc6e0) std::_Bit_iter<bool,bool std::vector<bool,std::allocator<bool> >::end(__$ReturnUdt);
// CODEVIEW(..\stlport\stl_bvector.h:592, dc 0xbc708) unsigned std::vector<bool,std::allocator<bool> >::size();
// CODEVIEW(..\stlport\stl_bvector.h:599, dc 0xbc754) std::_Bit_reference std::vector<bool,std::allocator<bool> >::operator[](std::_Bit_reference* __$ReturnUdt, unsigned __n);
// CODEVIEW(E:\gamedcs\game.cpp:11746, dc 0xbd654) void std::vector<type_point,std::allocator<type_point> >::`default constructor closure'();
// CODEVIEW(..\stlport\stl_string.h:727, dc 0xbd698) std::basic_string<char,std::char_traits<char>,std::allocator<char>* std::basic_string<char,std::char_traits<char>,std::allocator<char> >::assign(const char* __s);
// CODEVIEW(..\stlport\stl_string.h:972, dc 0xbd6d4) std::basic_string<char,std::char_traits<char>,std::allocator<char>* std::basic_string<char,std::char_traits<char>,std::allocator<char> >::erase(unsigned __pos, unsigned __n);
// CODEVIEW(..\stlport\stl_bvector.h:249, dc 0xbd770) void std::_Bit_iter<std::_Bit_reference,std::_Bit_reference *>::_Bit_iter<std::_Bit_reference,std::_Bit_reference *>(const std::_Bit_iter<std::_Bit_reference,std::_Bit_reference* __x);
// CODEVIEW(..\stlport\stl_bvector.h:250, dc 0xbd794) std::_Bit_reference std::_Bit_iter<std::_Bit_reference,std::_Bit_reference *>::operator*(std::_Bit_reference* __$ReturnUdt);
// CODEVIEW(..\stlport\stl_bvector.h:272, dc 0xbd7e0) std::_Bit_iter<std::_Bit_reference,std::_Bit_reference* std::_Bit_iter<std::_Bit_reference,std::_Bit_reference *>::operator+=(int __i);
// CODEVIEW(..\stlport\stl_bvector.h:280, dc 0xbd804) std::_Bit_iter<std::_Bit_reference,std::_Bit_reference std::_Bit_iter<std::_Bit_reference,std::_Bit_reference *>::operator+(__$ReturnUdt, int __i);
// CODEVIEW(..\stlport\stl_bvector.h:288, dc 0xbd84c) int std::_Bit_iter<bool,bool const *>::operator-(const std::_Bit_iter<bool,bool* __x);
// CODEVIEW(..\stlport\stl_bitset.h:513, dc 0xbd874) std::bitset<48,unsigned* std::bitset<48,unsigned long>::set();
// CODEVIEW(..\stlport\stl_bitset.h:519, dc 0xbd8a0) std::bitset<48,unsigned* std::bitset<48,unsigned long>::set(unsigned __pos);
// CODEVIEW(..\stlport\stl_bitset.h:533, dc 0xbd8cc) std::bitset<48,unsigned* std::bitset<48,unsigned long>::reset();
// CODEVIEW(..\stlport\stl_vector.h:218, dc 0xbd8ec) void std::vector<enum TArtifact,std::allocator<enum TArtifact> >::vector<enum TArtifact,std::allocator<enum TArtifact> >(const std::allocator<enum* __a);
// CODEVIEW(..\stlport\stl_vector.h:288, dc 0xbd910) void std::vector<enum TArtifact,std::allocator<enum TArtifact> >::~vector<enum TArtifact,std::allocator<enum TArtifact> >();
// CODEVIEW(..\stlport\stl_alloc.h:527, dc 0xbd944) void std::allocator<enum TArtifact>::allocator<enum TArtifact>();
// CODEVIEW(..\stlport\stl_alloc.h:537, dc 0xbd94c) void std::allocator<enum TArtifact>::~allocator<enum TArtifact>();
// CODEVIEW(..\stlport\stl_vector.h:195, dc 0xbd954) unsigned std::vector<CObjectType,std::allocator<CObjectType> >::size();
// CODEVIEW(..\stlport\stl_vector.h:365, dc 0xbd980) CObjectType* std::vector<CObjectType,std::allocator<CObjectType> >::back();
// CODEVIEW(..\stlport\stl_vector.h:368, dc 0xbd9a4) void std::vector<CObjectType,std::allocator<CObjectType> >::push_back(const CObjectType* __x);
// CODEVIEW(..\stlport\stl_vector.h:368, dc 0xbd9f8) void std::vector<CObject,std::allocator<CObject> >::push_back(const CObject* __x);
// CODEVIEW(..\stlport\stl_vector.h:368, dc 0xbda4c) void std::vector<CSprite *,std::allocator<CSprite *> >::push_back(CSprite** __x);
// CODEVIEW(..\stlport\stl_vector.h:195, dc 0xbdaa0) unsigned std::vector<TTimedEvent,std::allocator<TTimedEvent> >::size();
// CODEVIEW(..\stlport\stl_vector.h:203, dc 0xbdacc) TTimedEvent* std::vector<TTimedEvent,std::allocator<TTimedEvent> >::operator[](unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:195, dc 0xbdaf8) unsigned std::vector<TTownEvent,std::allocator<TTownEvent> >::size();
// CODEVIEW(..\stlport\stl_vector.h:203, dc 0xbdb24) TTownEvent* std::vector<TTownEvent,std::allocator<TTownEvent> >::operator[](unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:179, dc 0xbdb50) type_point* std::vector<type_point,std::allocator<type_point> >::begin();
// CODEVIEW(..\stlport\stl_vector.h:203, dc 0xbdb5c) type_point* std::vector<type_point,std::allocator<type_point> >::operator[](unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:218, dc 0xbdb84) void std::vector<type_point,std::allocator<type_point> >::vector<type_point,std::allocator<type_point> >(const std::allocator<type_point>* __a);
// CODEVIEW(..\stlport\stl_vector.h:288, dc 0xbdba8) void std::vector<type_point,std::allocator<type_point> >::~vector<type_point,std::allocator<type_point> >();
// CODEVIEW(..\stlport\stl_vector.h:368, dc 0xbdbdc) void std::vector<type_point,std::allocator<type_point> >::push_back(const type_point* __x);
// CODEVIEW(..\stlport\stl_vector.h:480, dc 0xbdc30) type_point* std::vector<type_point,std::allocator<type_point> >::erase(type_point* __position);
// CODEVIEW(..\stlport\stl_vector.h:506, dc 0xbdc90) void std::vector<type_point,std::allocator<type_point> >::clear();
// CODEVIEW(..\stlport\stl_alloc.h:527, dc 0xbdccc) void std::allocator<type_point>::allocator<type_point>();
// CODEVIEW(..\stlport\stl_alloc.h:537, dc 0xbdcd4) void std::allocator<type_point>::~allocator<type_point>();
// CODEVIEW(..\stlport\stl_vector.h:195, dc 0xbdcdc) unsigned std::vector<TownExtra,std::allocator<TownExtra> >::size();
// CODEVIEW(..\stlport\stl_vector.h:203, dc 0xbdd08) TownExtra* std::vector<TownExtra,std::allocator<TownExtra> >::operator[](unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:218, dc 0xbdd34) void std::vector<TownExtra,std::allocator<TownExtra> >::vector<TownExtra,std::allocator<TownExtra> >(const std::allocator<TownExtra>* __a);
// CODEVIEW(..\stlport\stl_vector.h:288, dc 0xbdd58) void std::vector<TownExtra,std::allocator<TownExtra> >::~vector<TownExtra,std::allocator<TownExtra> >();
// CODEVIEW(..\stlport\stl_alloc.h:527, dc 0xbdd8c) void std::allocator<TownExtra>::allocator<TownExtra>();
// CODEVIEW(..\stlport\stl_alloc.h:537, dc 0xbdd94) void std::allocator<TownExtra>::~allocator<TownExtra>();
// CODEVIEW(..\stlport\stl_vector.h:195, dc 0xbdd9c) unsigned std::vector<TBlackMarket,std::allocator<TBlackMarket> >::size();
// CODEVIEW(..\stlport\stl_vector.h:218, dc 0xbddc8) void std::vector<TBlackMarket,std::allocator<TBlackMarket> >::vector<TBlackMarket,std::allocator<TBlackMarket> >(const std::allocator<TBlackMarket>* __a);
// CODEVIEW(..\stlport\stl_vector.h:288, dc 0xbddec) void std::vector<TBlackMarket,std::allocator<TBlackMarket> >::~vector<TBlackMarket,std::allocator<TBlackMarket> >();
// CODEVIEW(..\stlport\stl_vector.h:368, dc 0xbde20) void std::vector<TBlackMarket,std::allocator<TBlackMarket> >::push_back(const TBlackMarket* __x);
// CODEVIEW(..\stlport\stl_vector.h:505, dc 0xbde74) void std::vector<TBlackMarket,std::allocator<TBlackMarket> >::resize(unsigned __new_size);
// CODEVIEW(..\stlport\stl_vector.h:506, dc 0xbde98) void std::vector<TBlackMarket,std::allocator<TBlackMarket> >::clear();
// CODEVIEW(..\stlport\stl_alloc.h:527, dc 0xbded4) void std::allocator<TBlackMarket>::allocator<TBlackMarket>();
// CODEVIEW(..\stlport\stl_alloc.h:537, dc 0xbdedc) void std::allocator<TBlackMarket>::~allocator<TBlackMarket>();
// CODEVIEW(..\stlport\stl_vector.h:218, dc 0xbdee4) void std::vector<town,std::allocator<town> >::vector<town,std::allocator<town> >(const std::allocator<town>* __a);
// CODEVIEW(..\stlport\stl_vector.h:288, dc 0xbdf08) void std::vector<town,std::allocator<town> >::~vector<town,std::allocator<town> >();
// CODEVIEW(..\stlport\stl_vector.h:505, dc 0xbdf3c) void std::vector<town,std::allocator<town> >::resize(unsigned __new_size);
// CODEVIEW(..\stlport\stl_alloc.h:527, dc 0xbdf80) void std::allocator<town>::allocator<town>();
// CODEVIEW(..\stlport\stl_alloc.h:537, dc 0xbdf88) void std::allocator<town>::~allocator<town>();
// CODEVIEW(..\stlport\stl_vector.h:195, dc 0xbdf90) unsigned std::vector<Sign,std::allocator<Sign> >::size();
// CODEVIEW(..\stlport\stl_vector.h:218, dc 0xbdfa8) void std::vector<Sign,std::allocator<Sign> >::vector<Sign,std::allocator<Sign> >(const std::allocator<Sign>* __a);
// CODEVIEW(..\stlport\stl_vector.h:288, dc 0xbdfcc) void std::vector<Sign,std::allocator<Sign> >::~vector<Sign,std::allocator<Sign> >();
// CODEVIEW(..\stlport\stl_vector.h:505, dc 0xbe000) void std::vector<Sign,std::allocator<Sign> >::resize(unsigned __new_size);
// CODEVIEW(..\stlport\stl_alloc.h:527, dc 0xbe044) void std::allocator<Sign>::allocator<Sign>();
// CODEVIEW(..\stlport\stl_alloc.h:537, dc 0xbe04c) void std::allocator<Sign>::~allocator<Sign>();
// CODEVIEW(..\stlport\stl_vector.h:195, dc 0xbe054) unsigned std::vector<mine,std::allocator<mine> >::size();
// CODEVIEW(..\stlport\stl_vector.h:218, dc 0xbe080) void std::vector<mine,std::allocator<mine> >::vector<mine,std::allocator<mine> >(const std::allocator<mine>* __a);
// CODEVIEW(..\stlport\stl_vector.h:288, dc 0xbe0a4) void std::vector<mine,std::allocator<mine> >::~vector<mine,std::allocator<mine> >();
// CODEVIEW(..\stlport\stl_vector.h:505, dc 0xbe0d8) void std::vector<mine,std::allocator<mine> >::resize(unsigned __new_size);
// CODEVIEW(..\stlport\stl_alloc.h:527, dc 0xbe10c) void std::allocator<mine>::allocator<mine>();
// CODEVIEW(..\stlport\stl_alloc.h:537, dc 0xbe114) void std::allocator<mine>::~allocator<mine>();
// CODEVIEW(..\stlport\stl_vector.h:195, dc 0xbe11c) unsigned std::vector<generator,std::allocator<generator> >::size();
// CODEVIEW(..\stlport\stl_vector.h:218, dc 0xbe148) void std::vector<generator,std::allocator<generator> >::vector<generator,std::allocator<generator> >(const std::allocator<generator>* __a);
// CODEVIEW(..\stlport\stl_vector.h:288, dc 0xbe16c) void std::vector<generator,std::allocator<generator> >::~vector<generator,std::allocator<generator> >();
// CODEVIEW(..\stlport\stl_alloc.h:527, dc 0xbe1a0) void std::allocator<generator>::allocator<generator>();
// CODEVIEW(..\stlport\stl_alloc.h:537, dc 0xbe1a8) void std::allocator<generator>::~allocator<generator>();
// CODEVIEW(..\stlport\stl_vector.h:195, dc 0xbe1b0) unsigned std::vector<garrison,std::allocator<garrison> >::size();
// CODEVIEW(..\stlport\stl_vector.h:218, dc 0xbe1c8) void std::vector<garrison,std::allocator<garrison> >::vector<garrison,std::allocator<garrison> >(const std::allocator<garrison>* __a);
// CODEVIEW(..\stlport\stl_vector.h:288, dc 0xbe1ec) void std::vector<garrison,std::allocator<garrison> >::~vector<garrison,std::allocator<garrison> >();
// CODEVIEW(..\stlport\stl_vector.h:505, dc 0xbe220) void std::vector<garrison,std::allocator<garrison> >::resize(unsigned __new_size);
// CODEVIEW(..\stlport\stl_alloc.h:527, dc 0xbe258) void std::allocator<garrison>::allocator<garrison>();
// CODEVIEW(..\stlport\stl_alloc.h:537, dc 0xbe260) void std::allocator<garrison>::~allocator<garrison>();
// CODEVIEW(..\stlport\stl_vector.h:181, dc 0xbe268) boat* std::vector<boat,std::allocator<boat> >::end();
// CODEVIEW(..\stlport\stl_vector.h:195, dc 0xbe274) unsigned std::vector<boat,std::allocator<boat> >::size();
// CODEVIEW(..\stlport\stl_vector.h:218, dc 0xbe2a0) void std::vector<boat,std::allocator<boat> >::vector<boat,std::allocator<boat> >(const std::allocator<boat>* __a);
// CODEVIEW(..\stlport\stl_vector.h:288, dc 0xbe2c4) void std::vector<boat,std::allocator<boat> >::~vector<boat,std::allocator<boat> >();
// CODEVIEW(..\stlport\stl_vector.h:368, dc 0xbe2f8) void std::vector<boat,std::allocator<boat> >::push_back(const boat* __x);
// CODEVIEW(..\stlport\stl_vector.h:505, dc 0xbe34c) void std::vector<boat,std::allocator<boat> >::resize(unsigned __new_size);
// CODEVIEW(..\stlport\stl_alloc.h:527, dc 0xbe384) void std::allocator<boat>::allocator<boat>();
// CODEVIEW(..\stlport\stl_alloc.h:537, dc 0xbe38c) void std::allocator<boat>::~allocator<boat>();
// CODEVIEW(..\stlport\stl_vector.h:195, dc 0xbe394) unsigned std::vector<type_university,std::allocator<type_university> >::size();
// CODEVIEW(..\stlport\stl_vector.h:218, dc 0xbe3ac) void std::vector<type_university,std::allocator<type_university> >::vector<type_university,std::allocator<type_university> >(const std::allocator<type_university>* __a);
// CODEVIEW(..\stlport\stl_vector.h:288, dc 0xbe3d0) void std::vector<type_university,std::allocator<type_university> >::~vector<type_university,std::allocator<type_university> >();
// CODEVIEW(..\stlport\stl_vector.h:368, dc 0xbe404) void std::vector<type_university,std::allocator<type_university> >::push_back(const type_university* __x);
// CODEVIEW(..\stlport\stl_alloc.h:527, dc 0xbe458) void std::allocator<type_university>::allocator<type_university>();
// CODEVIEW(..\stlport\stl_alloc.h:537, dc 0xbe460) void std::allocator<type_university>::~allocator<type_university>();
// CODEVIEW(..\stlport\stl_vector.h:195, dc 0xbe468) unsigned std::vector<type_creature_bank,std::allocator<type_creature_bank> >::size();
// CODEVIEW(..\stlport\stl_vector.h:218, dc 0xbe494) void std::vector<type_creature_bank,std::allocator<type_creature_bank> >::vector<type_creature_bank,std::allocator<type_creature_bank> >(const std::allocator<type_creature_bank>* __a);
// CODEVIEW(..\stlport\stl_vector.h:288, dc 0xbe4b8) void std::vector<type_creature_bank,std::allocator<type_creature_bank> >::~vector<type_creature_bank,std::allocator<type_creature_bank> >();
// CODEVIEW(..\stlport\stl_vector.h:368, dc 0xbe4ec) void std::vector<type_creature_bank,std::allocator<type_creature_bank> >::push_back(const type_creature_bank* __x);
// CODEVIEW(..\stlport\stl_alloc.h:527, dc 0xbe540) void std::allocator<type_creature_bank>::allocator<type_creature_bank>();
// CODEVIEW(..\stlport\stl_alloc.h:537, dc 0xbe548) void std::allocator<type_creature_bank>::~allocator<type_creature_bank>();
// CODEVIEW(..\stlport\stl_vector.h:179, dc 0xbe550) game::TRumour* std::vector<game::TRumour,std::allocator<game::TRumour> >::begin();
// CODEVIEW(..\stlport\stl_vector.h:181, dc 0xbe55c) game::TRumour* std::vector<game::TRumour,std::allocator<game::TRumour> >::end();
// CODEVIEW(..\stlport\stl_vector.h:195, dc 0xbe568) unsigned std::vector<game::TRumour,std::allocator<game::TRumour> >::size();
// CODEVIEW(..\stlport\stl_vector.h:203, dc 0xbe580) game::TRumour* std::vector<game::TRumour,std::allocator<game::TRumour> >::operator[](unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:218, dc 0xbe5ac) void std::vector<game::TRumour,std::allocator<game::TRumour> >::vector<game::TRumour,std::allocator<game::TRumour> >(const std::allocator<game::TRumour>* __a);
// CODEVIEW(..\stlport\stl_vector.h:288, dc 0xbe5d0) void std::vector<game::TRumour,std::allocator<game::TRumour> >::~vector<game::TRumour,std::allocator<game::TRumour> >();
// CODEVIEW(..\stlport\stl_vector.h:505, dc 0xbe604) void std::vector<game::TRumour,std::allocator<game::TRumour> >::resize(unsigned __new_size);
// CODEVIEW(..\stlport\stl_alloc.h:527, dc 0xbe648) void std::allocator<game::TRumour>::allocator<game::TRumour>();
// CODEVIEW(..\stlport\stl_alloc.h:537, dc 0xbe650) void std::allocator<game::TRumour>::~allocator<game::TRumour>();
// CODEVIEW(..\stlport\stl_vector.h:204, dc 0xbe658) const long* std::vector<long,std::allocator<long> >::operator[](unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:218, dc 0xbe680) void std::vector<type_event_record *,std::allocator<type_event_record *> >::vector<type_event_record *,std::allocator<type_event_record *> >(const std::allocator<type_event_record* __a);
// CODEVIEW(..\stlport\stl_vector.h:288, dc 0xbe6a4) void std::vector<type_event_record *,std::allocator<type_event_record *> >::~vector<type_event_record *,std::allocator<type_event_record *> >();
// CODEVIEW(..\stlport\stl_alloc.h:527, dc 0xbe6d8) void std::allocator<type_event_record *>::allocator<type_event_record *>();
// CODEVIEW(..\stlport\stl_alloc.h:537, dc 0xbe6e0) void std::allocator<type_event_record *>::~allocator<type_event_record *>();
// CODEVIEW(..\stlport\stl_vector.h:195, dc 0xbe6e8) unsigned std::vector<hero,std::allocator<hero> >::size();
// CODEVIEW(..\stlport\stl_vector.h:203, dc 0xbe718) hero* std::vector<hero,std::allocator<hero> >::operator[](unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:218, dc 0xbe748) void std::vector<hero,std::allocator<hero> >::vector<hero,std::allocator<hero> >(const std::allocator<hero>* __a);
// CODEVIEW(..\stlport\stl_vector.h:288, dc 0xbe76c) void std::vector<hero,std::allocator<hero> >::~vector<hero,std::allocator<hero> >();
// CODEVIEW(..\stlport\stl_vector.h:368, dc 0xbe7a0) void std::vector<hero,std::allocator<hero> >::push_back(const hero* __x);
// CODEVIEW(..\stlport\stl_alloc.h:527, dc 0xbe7f8) void std::allocator<hero>::allocator<hero>();
// CODEVIEW(..\stlport\stl_alloc.h:537, dc 0xbe800) void std::allocator<hero>::~allocator<hero>();
// CODEVIEW(..\stlport\stl_bitset.h:353, dc 0xbe888) void std::bitset<48,unsigned long>::_M_do_sanitize();
// CODEVIEW(..\stlport\stl_bitset.h:483, dc 0xbe8b4) std::bitset<48,unsigned* std::bitset<48,unsigned long>::_Unchecked_set(unsigned __pos);
// CODEVIEW(..\stlport\stl_bitset.h:158, dc 0xbe8f0) void std::_Base_bitset<2,unsigned long>::_M_do_set();
// CODEVIEW(..\stlport\stl_bitset.h:164, dc 0xbe920) void std::_Base_bitset<2,unsigned long>::_M_do_reset();
// CODEVIEW(..\stlport\stl_vector.h:89, dc 0xbe950) void std::_Vector_base<enum TArtifact,std::allocator<enum TArtifact> >::_Vector_base<enum TArtifact,std::allocator<enum TArtifact> >(const std::allocator<enum* __a);
// CODEVIEW(..\stlport\stl_vector.h:101, dc 0xbe994) void std::_Vector_base<enum TArtifact,std::allocator<enum TArtifact> >::~_Vector_base<enum TArtifact,std::allocator<enum TArtifact> >();
// CODEVIEW(..\stlport\stl_vector.h:181, dc 0xbe9d8) CObjectType* std::vector<CObjectType,std::allocator<CObjectType> >::end();
// CODEVIEW(..\stlport\stl_vector.h:179, dc 0xbe9e4) TTimedEvent* std::vector<TTimedEvent,std::allocator<TTimedEvent> >::begin();
// CODEVIEW(..\stlport\stl_vector.h:179, dc 0xbe9f0) TTownEvent* std::vector<TTownEvent,std::allocator<TTownEvent> >::begin();
// CODEVIEW(..\stlport\stl_vector.h:181, dc 0xbe9fc) type_point* std::vector<type_point,std::allocator<type_point> >::end();
// CODEVIEW(..\stlport\stl_vector.h:490, dc 0xbea08) type_point* std::vector<type_point,std::allocator<type_point> >::erase(type_point* __first, type_point* __last);
// CODEVIEW(..\stlport\stl_vector.h:89, dc 0xbea50) void std::_Vector_base<type_point,std::allocator<type_point> >::_Vector_base<type_point,std::allocator<type_point> >(const std::allocator<type_point>* __a);
// CODEVIEW(..\stlport\stl_vector.h:101, dc 0xbea94) void std::_Vector_base<type_point,std::allocator<type_point> >::~_Vector_base<type_point,std::allocator<type_point> >();
// CODEVIEW(..\stlport\stl_vector.h:179, dc 0xbead8) TownExtra* std::vector<TownExtra,std::allocator<TownExtra> >::begin();
// CODEVIEW(..\stlport\stl_vector.h:89, dc 0xbeae4) void std::_Vector_base<TownExtra,std::allocator<TownExtra> >::_Vector_base<TownExtra,std::allocator<TownExtra> >(const std::allocator<TownExtra>* __a);
// CODEVIEW(..\stlport\stl_vector.h:101, dc 0xbeb28) void std::_Vector_base<TownExtra,std::allocator<TownExtra> >::~_Vector_base<TownExtra,std::allocator<TownExtra> >();
// CODEVIEW(..\stlport\stl_vector.h:181, dc 0xbeb7c) TBlackMarket* std::vector<TBlackMarket,std::allocator<TBlackMarket> >::end();
// CODEVIEW(..\stlport\stl_vector.h:490, dc 0xbeb88) TBlackMarket* std::vector<TBlackMarket,std::allocator<TBlackMarket> >::erase(TBlackMarket* __first, TBlackMarket* __last);
// CODEVIEW(..\stlport\stl_vector.h:499, dc 0xbebd0) void std::vector<TBlackMarket,std::allocator<TBlackMarket> >::resize(unsigned __new_size, const TBlackMarket* __x);
// CODEVIEW(..\stlport\stl_vector.h:89, dc 0xbec64) void std::_Vector_base<TBlackMarket,std::allocator<TBlackMarket> >::_Vector_base<TBlackMarket,std::allocator<TBlackMarket> >(const std::allocator<TBlackMarket>* __a);
// CODEVIEW(..\stlport\stl_vector.h:101, dc 0xbeca8) void std::_Vector_base<TBlackMarket,std::allocator<TBlackMarket> >::~_Vector_base<TBlackMarket,std::allocator<TBlackMarket> >();
// CODEVIEW(..\stlport\stl_vector.h:499, dc 0xbecfc) void std::vector<town,std::allocator<town> >::resize(unsigned __new_size, const town* __x);
// CODEVIEW(..\stlport\stl_vector.h:89, dc 0xbed90) void std::_Vector_base<town,std::allocator<town> >::_Vector_base<town,std::allocator<town> >(const std::allocator<town>* __a);
// CODEVIEW(..\stlport\stl_vector.h:101, dc 0xbedd4) void std::_Vector_base<town,std::allocator<town> >::~_Vector_base<town,std::allocator<town> >();
// CODEVIEW(..\stlport\stl_vector.h:499, dc 0xbee28) void std::vector<Sign,std::allocator<Sign> >::resize(unsigned __new_size, const Sign* __x);
// CODEVIEW(..\stlport\stl_vector.h:89, dc 0xbeeb8) void std::_Vector_base<Sign,std::allocator<Sign> >::_Vector_base<Sign,std::allocator<Sign> >(const std::allocator<Sign>* __a);
// CODEVIEW(..\stlport\stl_vector.h:101, dc 0xbeefc) void std::_Vector_base<Sign,std::allocator<Sign> >::~_Vector_base<Sign,std::allocator<Sign> >();
// CODEVIEW(..\stlport\stl_vector.h:499, dc 0xbef40) void std::vector<mine,std::allocator<mine> >::resize(unsigned __new_size, const mine* __x);
// CODEVIEW(..\stlport\stl_vector.h:89, dc 0xbefd4) void std::_Vector_base<mine,std::allocator<mine> >::_Vector_base<mine,std::allocator<mine> >(const std::allocator<mine>* __a);
// CODEVIEW(..\stlport\stl_vector.h:101, dc 0xbf018) void std::_Vector_base<mine,std::allocator<mine> >::~_Vector_base<mine,std::allocator<mine> >();
// CODEVIEW(..\stlport\stl_vector.h:89, dc 0xbf06c) void std::_Vector_base<generator,std::allocator<generator> >::_Vector_base<generator,std::allocator<generator> >(const std::allocator<generator>* __a);
// CODEVIEW(..\stlport\stl_vector.h:101, dc 0xbf0b0) void std::_Vector_base<generator,std::allocator<generator> >::~_Vector_base<generator,std::allocator<generator> >();
// CODEVIEW(..\stlport\stl_vector.h:499, dc 0xbf104) void std::vector<garrison,std::allocator<garrison> >::resize(unsigned __new_size, const garrison* __x);
// CODEVIEW(..\stlport\stl_vector.h:89, dc 0xbf194) void std::_Vector_base<garrison,std::allocator<garrison> >::_Vector_base<garrison,std::allocator<garrison> >(const std::allocator<garrison>* __a);
// CODEVIEW(..\stlport\stl_vector.h:101, dc 0xbf1d8) void std::_Vector_base<garrison,std::allocator<garrison> >::~_Vector_base<garrison,std::allocator<garrison> >();
// CODEVIEW(..\stlport\stl_vector.h:499, dc 0xbf21c) void std::vector<boat,std::allocator<boat> >::resize(unsigned __new_size, const boat* __x);
// CODEVIEW(..\stlport\stl_vector.h:89, dc 0xbf2b0) void std::_Vector_base<boat,std::allocator<boat> >::_Vector_base<boat,std::allocator<boat> >(const std::allocator<boat>* __a);
// CODEVIEW(..\stlport\stl_vector.h:101, dc 0xbf2f4) void std::_Vector_base<boat,std::allocator<boat> >::~_Vector_base<boat,std::allocator<boat> >();
// CODEVIEW(..\stlport\stl_vector.h:89, dc 0xbf348) void std::_Vector_base<type_university,std::allocator<type_university> >::_Vector_base<type_university,std::allocator<type_university> >(const std::allocator<type_university>* __a);
// CODEVIEW(..\stlport\stl_vector.h:101, dc 0xbf38c) void std::_Vector_base<type_university,std::allocator<type_university> >::~_Vector_base<type_university,std::allocator<type_university> >();
// CODEVIEW(..\stlport\stl_vector.h:89, dc 0xbf3d0) void std::_Vector_base<type_creature_bank,std::allocator<type_creature_bank> >::_Vector_base<type_creature_bank,std::allocator<type_creature_bank> >(const std::allocator<type_creature_bank>* __a);
// CODEVIEW(..\stlport\stl_vector.h:101, dc 0xbf414) void std::_Vector_base<type_creature_bank,std::allocator<type_creature_bank> >::~_Vector_base<type_creature_bank,std::allocator<type_creature_bank> >();
// CODEVIEW(..\stlport\stl_vector.h:499, dc 0xbf468) void std::vector<game::TRumour,std::allocator<game::TRumour> >::resize(unsigned __new_size, const game::TRumour* __x);
// CODEVIEW(..\stlport\stl_vector.h:89, dc 0xbf4f8) void std::_Vector_base<game::TRumour,std::allocator<game::TRumour> >::_Vector_base<game::TRumour,std::allocator<game::TRumour> >(const std::allocator<game::TRumour>* __a);
// CODEVIEW(..\stlport\stl_vector.h:101, dc 0xbf53c) void std::_Vector_base<game::TRumour,std::allocator<game::TRumour> >::~_Vector_base<game::TRumour,std::allocator<game::TRumour> >();
// CODEVIEW(..\stlport\stl_vector.h:180, dc 0xbf580) const long* std::vector<long,std::allocator<long> >::begin();
// CODEVIEW(..\stlport\stl_vector.h:89, dc 0xbf58c) void std::_Vector_base<type_event_record *,std::allocator<type_event_record *> >::_Vector_base<type_event_record *,std::allocator<type_event_record *> >(const std::allocator<type_event_record* __a);
// CODEVIEW(..\stlport\stl_vector.h:101, dc 0xbf5d0) void std::_Vector_base<type_event_record *,std::allocator<type_event_record *> >::~_Vector_base<type_event_record *,std::allocator<type_event_record *> >();
// CODEVIEW(..\stlport\stl_vector.h:179, dc 0xbf614) hero* std::vector<hero,std::allocator<hero> >::begin();
// CODEVIEW(..\stlport\stl_vector.h:89, dc 0xbf620) void std::_Vector_base<hero,std::allocator<hero> >::_Vector_base<hero,std::allocator<hero> >(const std::allocator<hero>* __a);
// CODEVIEW(..\stlport\stl_vector.h:101, dc 0xbf664) void std::_Vector_base<hero,std::allocator<hero> >::~_Vector_base<hero,std::allocator<hero> >();
// CODEVIEW(..\stlport\stl_string.h:122, dc 0xbf6b8) void std::_STL_alloc_proxy<enum TArtifact *,enum TArtifact,std::allocator<enum TArtifact> >::~_STL_alloc_proxy<enum TArtifact *,enum TArtifact,std::allocator<enum TArtifact> >();
// CODEVIEW(..\stlport\stl_string.h:122, dc 0xbf6d4) void std::_STL_alloc_proxy<type_point *,type_point,std::allocator<type_point> >::~_STL_alloc_proxy<type_point *,type_point,std::allocator<type_point> >();
// CODEVIEW(..\stlport\stl_string.h:122, dc 0xbf6f0) void std::_STL_alloc_proxy<TownExtra *,TownExtra,std::allocator<TownExtra> >::~_STL_alloc_proxy<TownExtra *,TownExtra,std::allocator<TownExtra> >();
// CODEVIEW(..\stlport\stl_string.h:122, dc 0xbf70c) void std::_STL_alloc_proxy<TBlackMarket *,TBlackMarket,std::allocator<TBlackMarket> >::~_STL_alloc_proxy<TBlackMarket *,TBlackMarket,std::allocator<TBlackMarket> >();
// CODEVIEW(..\stlport\stl_string.h:122, dc 0xbf728) void std::_STL_alloc_proxy<town *,town,std::allocator<town> >::~_STL_alloc_proxy<town *,town,std::allocator<town> >();
// CODEVIEW(..\stlport\stl_string.h:122, dc 0xbf744) void std::_STL_alloc_proxy<Sign *,Sign,std::allocator<Sign> >::~_STL_alloc_proxy<Sign *,Sign,std::allocator<Sign> >();
// CODEVIEW(..\stlport\stl_string.h:122, dc 0xbf760) void std::_STL_alloc_proxy<mine *,mine,std::allocator<mine> >::~_STL_alloc_proxy<mine *,mine,std::allocator<mine> >();
// CODEVIEW(..\stlport\stl_string.h:122, dc 0xbf77c) void std::_STL_alloc_proxy<generator *,generator,std::allocator<generator> >::~_STL_alloc_proxy<generator *,generator,std::allocator<generator> >();
// CODEVIEW(..\stlport\stl_string.h:122, dc 0xbf798) void std::_STL_alloc_proxy<garrison *,garrison,std::allocator<garrison> >::~_STL_alloc_proxy<garrison *,garrison,std::allocator<garrison> >();
// CODEVIEW(..\stlport\stl_string.h:122, dc 0xbf7b4) void std::_STL_alloc_proxy<boat *,boat,std::allocator<boat> >::~_STL_alloc_proxy<boat *,boat,std::allocator<boat> >();
// CODEVIEW(..\stlport\stl_string.h:122, dc 0xbf7d0) void std::_STL_alloc_proxy<type_university *,type_university,std::allocator<type_university> >::~_STL_alloc_proxy<type_university *,type_university,std::allocator<type_university> >();
// CODEVIEW(..\stlport\stl_string.h:122, dc 0xbf7ec) void std::_STL_alloc_proxy<type_creature_bank *,type_creature_bank,std::allocator<type_creature_bank> >::~_STL_alloc_proxy<type_creature_bank *,type_creature_bank,std::allocator<type_creature_bank> >();
// CODEVIEW(..\stlport\stl_string.h:122, dc 0xbf808) void std::_STL_alloc_proxy<game::TRumour *,game::TRumour,std::allocator<game::TRumour> >::~_STL_alloc_proxy<game::TRumour *,game::TRumour,std::allocator<game::TRumour> >();
// CODEVIEW(..\stlport\stl_string.h:122, dc 0xbf824) void std::_STL_alloc_proxy<type_event_record * *,type_event_record *,std::allocator<type_event_record *> >::~_STL_alloc_proxy<type_event_record * *,type_event_record *,std::allocator<type_event_record *> >();
// CODEVIEW(..\stlport\stl_string.h:122, dc 0xbf840) void std::_STL_alloc_proxy<hero *,hero,std::allocator<hero> >::~_STL_alloc_proxy<hero *,hero,std::allocator<hero> >();
// CODEVIEW(..\stlport\stl_bitset.h:120, dc 0xbf85c) unsigned long std::_Base_bitset<2,unsigned long>::_S_maskbit(unsigned __pos);
// CODEVIEW(..\stlport\stl_bitset.h:127, dc 0xbf880) unsigned long* std::_Base_bitset<2,unsigned long>::_M_hiword();
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0xbf88c) void std::_STL_alloc_proxy<enum TArtifact *,enum TArtifact,std::allocator<enum TArtifact> >::_STL_alloc_proxy<enum TArtifact *,enum TArtifact,std::allocator<enum TArtifact> >(const std::allocator<enum* __a, TArtifact** __p);
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0xbf8ac) void std::_STL_alloc_proxy<type_point *,type_point,std::allocator<type_point> >::_STL_alloc_proxy<type_point *,type_point,std::allocator<type_point> >(const std::allocator<type_point>* __a, type_point** __p);
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0xbf8cc) void std::_STL_alloc_proxy<type_point *,type_point,std::allocator<type_point> >::deallocate(type_point* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0xbf8fc) void std::_STL_alloc_proxy<TownExtra *,TownExtra,std::allocator<TownExtra> >::_STL_alloc_proxy<TownExtra *,TownExtra,std::allocator<TownExtra> >(const std::allocator<TownExtra>* __a, TownExtra** __p);
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0xbf91c) void std::_STL_alloc_proxy<TownExtra *,TownExtra,std::allocator<TownExtra> >::deallocate(TownExtra* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:472, dc 0xbf94c) void std::vector<TBlackMarket,std::allocator<TBlackMarket> >::insert(TBlackMarket* __pos, unsigned __n, const TBlackMarket* __x);
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0xbf974) void std::_STL_alloc_proxy<TBlackMarket *,TBlackMarket,std::allocator<TBlackMarket> >::_STL_alloc_proxy<TBlackMarket *,TBlackMarket,std::allocator<TBlackMarket> >(const std::allocator<TBlackMarket>* __a, TBlackMarket** __p);
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0xbf994) void std::_STL_alloc_proxy<TBlackMarket *,TBlackMarket,std::allocator<TBlackMarket> >::deallocate(TBlackMarket* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:181, dc 0xbf9c4) town* std::vector<town,std::allocator<town> >::end();
// CODEVIEW(..\stlport\stl_vector.h:472, dc 0xbf9d0) void std::vector<town,std::allocator<town> >::insert(town* __pos, unsigned __n, const town* __x);
// CODEVIEW(..\stlport\stl_vector.h:490, dc 0xbf9f8) town* std::vector<town,std::allocator<town> >::erase(town* __first, town* __last);
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0xbfa40) void std::_STL_alloc_proxy<town *,town,std::allocator<town> >::_STL_alloc_proxy<town *,town,std::allocator<town> >(const std::allocator<town>* __a, town** __p);
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0xbfa60) void std::_STL_alloc_proxy<town *,town,std::allocator<town> >::deallocate(town* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:181, dc 0xbfa90) Sign* std::vector<Sign,std::allocator<Sign> >::end();
// CODEVIEW(..\stlport\stl_vector.h:472, dc 0xbfa9c) void std::vector<Sign,std::allocator<Sign> >::insert(Sign* __pos, unsigned __n, const Sign* __x);
// CODEVIEW(..\stlport\stl_vector.h:490, dc 0xbfac4) Sign* std::vector<Sign,std::allocator<Sign> >::erase(Sign* __first, Sign* __last);
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0xbfb0c) void std::_STL_alloc_proxy<Sign *,Sign,std::allocator<Sign> >::_STL_alloc_proxy<Sign *,Sign,std::allocator<Sign> >(const std::allocator<Sign>* __a, Sign** __p);
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0xbfb2c) void std::_STL_alloc_proxy<Sign *,Sign,std::allocator<Sign> >::deallocate(Sign* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:181, dc 0xbfb5c) mine* std::vector<mine,std::allocator<mine> >::end();
// CODEVIEW(..\stlport\stl_vector.h:472, dc 0xbfb68) void std::vector<mine,std::allocator<mine> >::insert(mine* __pos, unsigned __n, const mine* __x);
// CODEVIEW(..\stlport\stl_vector.h:490, dc 0xbfb90) mine* std::vector<mine,std::allocator<mine> >::erase(mine* __first, mine* __last);
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0xbfbd8) void std::_STL_alloc_proxy<mine *,mine,std::allocator<mine> >::_STL_alloc_proxy<mine *,mine,std::allocator<mine> >(const std::allocator<mine>* __a, mine** __p);
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0xbfbf8) void std::_STL_alloc_proxy<mine *,mine,std::allocator<mine> >::deallocate(mine* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0xbfc28) void std::_STL_alloc_proxy<generator *,generator,std::allocator<generator> >::_STL_alloc_proxy<generator *,generator,std::allocator<generator> >(const std::allocator<generator>* __a, generator** __p);
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0xbfc48) void std::_STL_alloc_proxy<generator *,generator,std::allocator<generator> >::deallocate(generator* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:181, dc 0xbfc78) garrison* std::vector<garrison,std::allocator<garrison> >::end();
// CODEVIEW(..\stlport\stl_vector.h:472, dc 0xbfc84) void std::vector<garrison,std::allocator<garrison> >::insert(garrison* __pos, unsigned __n, const garrison* __x);
// CODEVIEW(..\stlport\stl_vector.h:490, dc 0xbfcac) garrison* std::vector<garrison,std::allocator<garrison> >::erase(garrison* __first, garrison* __last);
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0xbfcf4) void std::_STL_alloc_proxy<garrison *,garrison,std::allocator<garrison> >::_STL_alloc_proxy<garrison *,garrison,std::allocator<garrison> >(const std::allocator<garrison>* __a, garrison** __p);
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0xbfd14) void std::_STL_alloc_proxy<garrison *,garrison,std::allocator<garrison> >::deallocate(garrison* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:472, dc 0xbfd44) void std::vector<boat,std::allocator<boat> >::insert(boat* __pos, unsigned __n, const boat* __x);
// CODEVIEW(..\stlport\stl_vector.h:490, dc 0xbfd6c) boat* std::vector<boat,std::allocator<boat> >::erase(boat* __first, boat* __last);
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0xbfdb4) void std::_STL_alloc_proxy<boat *,boat,std::allocator<boat> >::_STL_alloc_proxy<boat *,boat,std::allocator<boat> >(const std::allocator<boat>* __a, boat** __p);
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0xbfdd4) void std::_STL_alloc_proxy<boat *,boat,std::allocator<boat> >::deallocate(boat* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0xbfe04) void std::_STL_alloc_proxy<type_university *,type_university,std::allocator<type_university> >::_STL_alloc_proxy<type_university *,type_university,std::allocator<type_university> >(const std::allocator<type_university>* __a, type_university** __p);
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0xbfe24) void std::_STL_alloc_proxy<type_university *,type_university,std::allocator<type_university> >::deallocate(type_university* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0xbfe54) void std::_STL_alloc_proxy<type_creature_bank *,type_creature_bank,std::allocator<type_creature_bank> >::_STL_alloc_proxy<type_creature_bank *,type_creature_bank,std::allocator<type_creature_bank> >(const std::allocator<type_creature_bank>* __a, type_creature_bank** __p);
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0xbfe74) void std::_STL_alloc_proxy<type_creature_bank *,type_creature_bank,std::allocator<type_creature_bank> >::deallocate(type_creature_bank* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:472, dc 0xbfea4) void std::vector<game::TRumour,std::allocator<game::TRumour> >::insert(game::TRumour* __pos, unsigned __n, const game::TRumour* __x);
// CODEVIEW(..\stlport\stl_vector.h:490, dc 0xbfecc) game::TRumour* std::vector<game::TRumour,std::allocator<game::TRumour> >::erase(game::TRumour* __first, game::TRumour* __last);
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0xbff14) void std::_STL_alloc_proxy<game::TRumour *,game::TRumour,std::allocator<game::TRumour> >::_STL_alloc_proxy<game::TRumour *,game::TRumour,std::allocator<game::TRumour> >(const std::allocator<game::TRumour>* __a, game::TRumour** __p);
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0xbff34) void std::_STL_alloc_proxy<game::TRumour *,game::TRumour,std::allocator<game::TRumour> >::deallocate(game::TRumour* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0xbff64) void std::_STL_alloc_proxy<type_event_record * *,type_event_record *,std::allocator<type_event_record *> >::_STL_alloc_proxy<type_event_record * *,type_event_record *,std::allocator<type_event_record *> >(const std::allocator<type_event_record* __a, type_event_record*** __p);
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0xbff84) void std::_STL_alloc_proxy<hero *,hero,std::allocator<hero> >::_STL_alloc_proxy<hero *,hero,std::allocator<hero> >(const std::allocator<hero>* __a, hero** __p);
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0xbffa4) void std::_STL_alloc_proxy<hero *,hero,std::allocator<hero> >::deallocate(hero* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_bitset.h:327, dc 0xbffd4) void std::_Sanitize<unsigned long,16>::_M_do_sanitize(unsigned long* __val);
// CODEVIEW(..\stlport\stl_bitset.h:117, dc 0xbffe8) unsigned std::_Base_bitset<2,unsigned long>::_S_whichbit(unsigned __pos);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0xbfff8) void std::allocator<type_point>::deallocate(type_point* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0xc0020) void std::allocator<TownExtra>::deallocate(TownExtra* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0xc004c) void std::allocator<TBlackMarket>::deallocate(TBlackMarket* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0xc0078) void std::allocator<town>::deallocate(town* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0xc00a8) void std::allocator<Sign>::deallocate(Sign* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0xc00d4) void std::allocator<mine>::deallocate(mine* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0xc0100) void std::allocator<generator>::deallocate(generator* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0xc012c) void std::allocator<garrison>::deallocate(garrison* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0xc0158) void std::allocator<boat>::deallocate(boat* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0xc0184) void std::allocator<type_university>::deallocate(type_university* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0xc01b0) void std::allocator<type_creature_bank>::deallocate(type_creature_bank* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0xc01dc) void std::allocator<game::TRumour>::deallocate(game::TRumour* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0xc0208) void std::allocator<hero>::deallocate(hero* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_vector.c:248, dc 0xc0238) void std::vector<CObjectType,std::allocator<CObjectType> >::_M_insert_overflow(CObjectType* __position, const CObjectType* __x, unsigned __fill_len);
// CODEVIEW(..\stlport\stl_vector.c:248, dc 0xc0368) void std::vector<CObject,std::allocator<CObject> >::_M_insert_overflow(CObject* __position, const CObject* __x, unsigned __fill_len);
// CODEVIEW(..\stlport\stl_vector.c:248, dc 0xc0498) void std::vector<CSprite *,std::allocator<CSprite *> >::_M_insert_overflow(CSprite** __position, CSprite** __x, unsigned __fill_len);
// CODEVIEW(..\stlport\stl_vector.c:248, dc 0xc05b4) void std::vector<type_point,std::allocator<type_point> >::_M_insert_overflow(type_point* __position, const type_point* __x, unsigned __fill_len);
// CODEVIEW(..\stlport\stl_vector.c:248, dc 0xc06d0) void std::vector<TBlackMarket,std::allocator<TBlackMarket> >::_M_insert_overflow(TBlackMarket* __position, const TBlackMarket* __x, unsigned __fill_len);
// CODEVIEW(..\stlport\stl_vector.c:283, dc 0xc0800) void std::vector<TBlackMarket,std::allocator<TBlackMarket> >::_M_fill_insert(TBlackMarket* __position, unsigned __n, const TBlackMarket* __x);
// CODEVIEW(..\stlport\stl_vector.c:283, dc 0xc0a24) void std::vector<town,std::allocator<town> >::_M_fill_insert(town* __position, unsigned __n, const town* __x);
// CODEVIEW(..\stlport\stl_vector.c:283, dc 0xc0cb0) void std::vector<Sign,std::allocator<Sign> >::_M_fill_insert(Sign* __position, unsigned __n, const Sign* __x);
// CODEVIEW(..\stlport\stl_vector.c:283, dc 0xc0e20) void std::vector<mine,std::allocator<mine> >::_M_fill_insert(mine* __position, unsigned __n, const mine* __x);
// CODEVIEW(..\stlport\stl_vector.c:283, dc 0xc0fa4) void std::vector<garrison,std::allocator<garrison> >::_M_fill_insert(garrison* __position, unsigned __n, const garrison* __x);
// CODEVIEW(..\stlport\stl_vector.c:248, dc 0xc118c) void std::vector<boat,std::allocator<boat> >::_M_insert_overflow(boat* __position, const boat* __x, unsigned __fill_len);
// CODEVIEW(..\stlport\stl_vector.c:283, dc 0xc12bc) void std::vector<boat,std::allocator<boat> >::_M_fill_insert(boat* __position, unsigned __n, const boat* __x);
// CODEVIEW(..\stlport\stl_vector.c:248, dc 0xc148c) void std::vector<type_university,std::allocator<type_university> >::_M_insert_overflow(type_university* __position, const type_university* __x, unsigned __fill_len);
// CODEVIEW(..\stlport\stl_vector.c:248, dc 0xc15ac) void std::vector<type_creature_bank,std::allocator<type_creature_bank> >::_M_insert_overflow(type_creature_bank* __position, const type_creature_bank* __x, unsigned __fill_len);
// CODEVIEW(..\stlport\stl_vector.c:283, dc 0xc16dc) void std::vector<game::TRumour,std::allocator<game::TRumour> >::_M_fill_insert(game::TRumour* __position, unsigned __n, const game::TRumour* __x);
// CODEVIEW(..\stlport\stl_vector.c:248, dc 0xc1c04) void std::vector<hero,std::allocator<hero> >::_M_insert_overflow(hero* __position, const hero* __x, unsigned __fill_len);
// CODEVIEW(..\stlport\stl_construct.h:85, dc 0xc2000) void std::construct(CObjectType* __p, const CObjectType* __value);
// CODEVIEW(..\stlport\stl_construct.h:85, dc 0xc2044) void std::construct(CObject* __p, const CObject* __value);
// CODEVIEW(..\stlport\stl_construct.h:85, dc 0xc209c) void std::construct(CSprite** __p, CSprite** __value);
// CODEVIEW(..\stlport\stl_construct.h:128, dc 0xc20d8) void std::destroy(type_point* __first, type_point* __last);
// CODEVIEW(..\stlport\stl_construct.h:85, dc 0xc2108) void std::construct(type_point* __p, const type_point* __value);
// CODEVIEW(..\stlport\stl_algobase.h:322, dc 0xc2160) type_point* std::copy(type_point* __first, type_point* __last, type_point* __result);
// CODEVIEW(..\stlport\stl_construct.h:59, dc 0xc21b8) void std::destroy(type_point* __pointer);
// CODEVIEW(..\stlport\stl_construct.h:128, dc 0xc21d8) void std::destroy(TownExtra* __first, TownExtra* __last);
// CODEVIEW(..\stlport\stl_construct.h:128, dc 0xc2208) void std::destroy(TBlackMarket* __first, TBlackMarket* __last);
// CODEVIEW(..\stlport\stl_construct.h:85, dc 0xc2238) void std::construct(TBlackMarket* __p, const TBlackMarket* __value);
// CODEVIEW(..\stlport\stl_construct.h:128, dc 0xc232c) void std::destroy(town* __first, town* __last);
// CODEVIEW(..\stlport\stl_construct.h:128, dc 0xc235c) void std::destroy(Sign* __first, Sign* __last);
// CODEVIEW(..\stlport\stl_construct.h:128, dc 0xc238c) void std::destroy(mine* __first, mine* __last);
// CODEVIEW(..\stlport\stl_construct.h:128, dc 0xc23bc) void std::destroy(generator* __first, generator* __last);
// CODEVIEW(..\stlport\stl_construct.h:128, dc 0xc23ec) void std::destroy(garrison* __first, garrison* __last);
// CODEVIEW(..\stlport\stl_construct.h:128, dc 0xc241c) void std::destroy(boat* __first, boat* __last);
// CODEVIEW(..\stlport\stl_construct.h:85, dc 0xc244c) void std::construct(boat* __p, const boat* __value);
// CODEVIEW(..\stlport\stl_construct.h:128, dc 0xc24e8) void std::destroy(type_university* __first, type_university* __last);
// CODEVIEW(..\stlport\stl_construct.h:85, dc 0xc2518) void std::construct(type_university* __p, const type_university* __value);
// CODEVIEW(..\stlport\stl_construct.h:128, dc 0xc257c) void std::destroy(type_creature_bank* __first, type_creature_bank* __last);
// CODEVIEW(..\stlport\stl_construct.h:85, dc 0xc25ac) void std::construct(type_creature_bank* __p, const type_creature_bank* __value);
// CODEVIEW(..\stlport\stl_construct.h:128, dc 0xc25f0) void std::destroy(game::TRumour* __first, game::TRumour* __last);
// CODEVIEW(..\stlport\stl_construct.h:128, dc 0xc2620) void std::destroy(hero* __first, hero* __last);
// CODEVIEW(..\stlport\stl_construct.h:85, dc 0xc2650) void std::construct(hero* __p, const hero* __value);
// CODEVIEW(..\stlport\stl_algobase.h:322, dc 0xc27b8) TBlackMarket* std::copy(TBlackMarket* __first, TBlackMarket* __last, TBlackMarket* __result);
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0xc2810) std::allocator<type_point>* std::__stl_alloc_rebind(std::allocator<type_point>* __a, const type_point* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0xc281c) std::allocator<TownExtra>* std::__stl_alloc_rebind(std::allocator<TownExtra>* __a, const TownExtra* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0xc2828) std::allocator<TBlackMarket>* std::__stl_alloc_rebind(std::allocator<TBlackMarket>* __a, const TBlackMarket* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:322, dc 0xc2834) town* std::copy(town* __first, town* __last, town* __result);
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0xc288c) std::allocator<town>* std::__stl_alloc_rebind(std::allocator<town>* __a, const town* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:322, dc 0xc2898) Sign* std::copy(Sign* __first, Sign* __last, Sign* __result);
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0xc28f0) std::allocator<Sign>* std::__stl_alloc_rebind(std::allocator<Sign>* __a, const Sign* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:322, dc 0xc28fc) mine* std::copy(mine* __first, mine* __last, mine* __result);
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0xc2954) std::allocator<mine>* std::__stl_alloc_rebind(std::allocator<mine>* __a, const mine* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0xc2960) std::allocator<generator>* std::__stl_alloc_rebind(std::allocator<generator>* __a, const generator* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:322, dc 0xc296c) garrison* std::copy(garrison* __first, garrison* __last, garrison* __result);
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0xc29c4) std::allocator<garrison>* std::__stl_alloc_rebind(std::allocator<garrison>* __a, const garrison* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:322, dc 0xc29d0) boat* std::copy(boat* __first, boat* __last, boat* __result);
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0xc2a28) std::allocator<boat>* std::__stl_alloc_rebind(std::allocator<boat>* __a, const boat* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0xc2a34) std::allocator<type_university>* std::__stl_alloc_rebind(std::allocator<type_university>* __a, const type_university* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0xc2a40) std::allocator<type_creature_bank>* std::__stl_alloc_rebind(std::allocator<type_creature_bank>* __a, const type_creature_bank* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:322, dc 0xc2a4c) game::TRumour* std::copy(game::TRumour* __first, game::TRumour* __last, game::TRumour* __result);
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0xc2aa4) std::allocator<game::TRumour>* std::__stl_alloc_rebind(std::allocator<game::TRumour>* __a, const game::TRumour* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0xc2ab0) std::allocator<hero>* std::__stl_alloc_rebind(std::allocator<hero>* __a, const hero* __formal);
// CODEVIEW(..\stlport\stl_vector.h:236, dc 0xc2cd0) void std::vector<enum TArtifact,std::allocator<enum TArtifact> >::vector<enum TArtifact,std::allocator<enum TArtifact> >(const std::vector<enum* __x);
// CODEVIEW(..\stlport\stl_vector.h:505, dc 0xc2d50) void std::vector<enum TArtifact,std::allocator<enum TArtifact> >::resize(unsigned __new_size);
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0xc2d78) CObjectType* std::_STL_alloc_proxy<CObjectType *,CObjectType,std::allocator<CObjectType> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0xc2db0) void std::_STL_alloc_proxy<CObjectType *,CObjectType,std::allocator<CObjectType> >::deallocate(CObjectType* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0xc2de0) CObject* std::_STL_alloc_proxy<CObject *,CObject,std::allocator<CObject> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0xc2e18) void std::_STL_alloc_proxy<CObject *,CObject,std::allocator<CObject> >::deallocate(CObject* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:195, dc 0xc2e48) unsigned std::vector<CSprite *,std::allocator<CSprite *> >::size();
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0xc2e60) CSprite** std::_STL_alloc_proxy<CSprite * *,CSprite *,std::allocator<CSprite *> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0xc2e98) void std::_STL_alloc_proxy<CSprite * *,CSprite *,std::allocator<CSprite *> >::deallocate(CSprite** __p, unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:505, dc 0xc2ec8) void std::vector<type_point,std::allocator<type_point> >::resize(unsigned __new_size);
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0xc2efc) type_point* std::_STL_alloc_proxy<type_point *,type_point,std::allocator<type_point> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0xc2f34) TBlackMarket* std::_STL_alloc_proxy<TBlackMarket *,TBlackMarket,std::allocator<TBlackMarket> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:505, dc 0xc2f6c) void std::vector<generator,std::allocator<generator> >::resize(unsigned __new_size);
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0xc2fa4) boat* std::_STL_alloc_proxy<boat *,boat,std::allocator<boat> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:505, dc 0xc2fdc) void std::vector<type_university,std::allocator<type_university> >::resize(unsigned __new_size);
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0xc3000) type_university* std::_STL_alloc_proxy<type_university *,type_university,std::allocator<type_university> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:505, dc 0xc3038) void std::vector<type_creature_bank,std::allocator<type_creature_bank> >::resize(unsigned __new_size);
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0xc3084) type_creature_bank* std::_STL_alloc_proxy<type_creature_bank *,type_creature_bank,std::allocator<type_creature_bank> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:505, dc 0xc30bc) void std::vector<long,std::allocator<long> >::resize(unsigned __new_size);
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0xc30e4) hero* std::_STL_alloc_proxy<hero *,hero,std::allocator<hero> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:153, dc 0xc311c) std::allocator<enum std::vector<enum TArtifact,std::allocator<enum TArtifact> >::get_allocator(__$ReturnUdt);
// CODEVIEW(..\stlport\stl_vector.h:499, dc 0xc3130) void std::vector<enum TArtifact,std::allocator<enum TArtifact> >::resize(unsigned __new_size, const TArtifact* __x);
// CODEVIEW(..\stlport\stl_vector.h:94, dc 0xc31c0) void std::_Vector_base<enum TArtifact,std::allocator<enum TArtifact> >::_Vector_base<enum TArtifact,std::allocator<enum TArtifact> >(unsigned __n, const std::allocator<enum* __a);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0xc3234) CObjectType* std::allocator<CObjectType>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0xc3270) void std::allocator<CObjectType>::deallocate(CObjectType* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0xc329c) CObject* std::allocator<CObject>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0xc32d8) void std::allocator<CObject>::deallocate(CObject* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0xc3304) CSprite** std::allocator<CSprite *>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0xc333c) void std::allocator<CSprite *>::deallocate(CSprite** __p, unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:499, dc 0xc3364) void std::vector<type_point,std::allocator<type_point> >::resize(unsigned __new_size, const type_point* __x);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0xc33f4) type_point* std::allocator<type_point>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0xc342c) TBlackMarket* std::allocator<TBlackMarket>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_vector.h:499, dc 0xc3468) void std::vector<generator,std::allocator<generator> >::resize(unsigned __new_size, const generator* __x);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0xc34fc) boat* std::allocator<boat>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_vector.h:499, dc 0xc3538) void std::vector<type_university,std::allocator<type_university> >::resize(unsigned __new_size, const type_university* __x);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0xc35c8) type_university* std::allocator<type_university>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_vector.h:499, dc 0xc3604) void std::vector<type_creature_bank,std::allocator<type_creature_bank> >::resize(unsigned __new_size, const type_creature_bank* __x);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0xc3698) type_creature_bank* std::allocator<type_creature_bank>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_vector.h:499, dc 0xc36d4) void std::vector<long,std::allocator<long> >::resize(unsigned __new_size, const long* __x);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0xc3764) hero* std::allocator<hero>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_vector.h:181, dc 0xc37a4) TArtifact* std::vector<enum TArtifact,std::allocator<enum TArtifact> >::end();
// CODEVIEW(..\stlport\stl_vector.h:472, dc 0xc37b0) void std::vector<enum TArtifact,std::allocator<enum TArtifact> >::insert(TArtifact* __pos, unsigned __n, const TArtifact* __x);
// CODEVIEW(..\stlport\stl_vector.h:490, dc 0xc37d8) TArtifact* std::vector<enum TArtifact,std::allocator<enum TArtifact> >::erase(TArtifact* __first, TArtifact* __last);
// CODEVIEW(..\stlport\stl_vector.h:472, dc 0xc3820) void std::vector<type_point,std::allocator<type_point> >::insert(type_point* __pos, unsigned __n, const type_point* __x);
// CODEVIEW(..\stlport\stl_vector.h:181, dc 0xc3848) generator* std::vector<generator,std::allocator<generator> >::end();
// CODEVIEW(..\stlport\stl_vector.h:472, dc 0xc3854) void std::vector<generator,std::allocator<generator> >::insert(generator* __pos, unsigned __n, const generator* __x);
// CODEVIEW(..\stlport\stl_vector.h:490, dc 0xc387c) generator* std::vector<generator,std::allocator<generator> >::erase(generator* __first, generator* __last);
// CODEVIEW(..\stlport\stl_vector.h:181, dc 0xc38c4) type_university* std::vector<type_university,std::allocator<type_university> >::end();
// CODEVIEW(..\stlport\stl_vector.h:472, dc 0xc38d0) void std::vector<type_university,std::allocator<type_university> >::insert(type_university* __pos, unsigned __n, const type_university* __x);
// CODEVIEW(..\stlport\stl_vector.h:490, dc 0xc38f8) type_university* std::vector<type_university,std::allocator<type_university> >::erase(type_university* __first, type_university* __last);
// CODEVIEW(..\stlport\stl_vector.h:181, dc 0xc3940) type_creature_bank* std::vector<type_creature_bank,std::allocator<type_creature_bank> >::end();
// CODEVIEW(..\stlport\stl_vector.h:472, dc 0xc394c) void std::vector<type_creature_bank,std::allocator<type_creature_bank> >::insert(type_creature_bank* __pos, unsigned __n, const type_creature_bank* __x);
// CODEVIEW(..\stlport\stl_vector.h:490, dc 0xc3974) type_creature_bank* std::vector<type_creature_bank,std::allocator<type_creature_bank> >::erase(type_creature_bank* __first, type_creature_bank* __last);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0xc39bc) CObjectType* std::uninitialized_copy(CObjectType* __first, CObjectType* __last, CObjectType* __result);
// CODEVIEW(..\stlport\stl_uninitialized.h:263, dc 0xc39f8) CObjectType* std::uninitialized_fill_n(CObjectType* __first, unsigned __n, const CObjectType* __x);
// CODEVIEW(..\stlport\stl_construct.h:128, dc 0xc3a34) void std::destroy(CObjectType* __first, CObjectType* __last);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0xc3a64) CObject* std::uninitialized_copy(CObject* __first, CObject* __last, CObject* __result);
// CODEVIEW(..\stlport\stl_uninitialized.h:263, dc 0xc3aa0) CObject* std::uninitialized_fill_n(CObject* __first, unsigned __n, const CObject* __x);
// CODEVIEW(..\stlport\stl_construct.h:128, dc 0xc3adc) void std::destroy(CObject* __first, CObject* __last);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0xc3b0c) CSprite** std::uninitialized_copy(CSprite** __first, CSprite** __last, CSprite** __result);
// CODEVIEW(..\stlport\stl_uninitialized.h:263, dc 0xc3b48) CSprite** std::uninitialized_fill_n(CSprite** __first, unsigned __n, CSprite** __x);
// CODEVIEW(..\stlport\stl_construct.h:128, dc 0xc3b84) void std::destroy(CSprite** __first, CSprite** __last);
// CODEVIEW(..\stlport\stl_vector.c:283, dc 0xc3bb4) void std::vector<type_point,std::allocator<type_point> >::_M_fill_insert(type_point* __position, unsigned __n, const type_point* __x);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0xc3d08) type_point* std::uninitialized_copy(type_point* __first, type_point* __last, type_point* __result);
// CODEVIEW(..\stlport\stl_uninitialized.h:263, dc 0xc3d44) type_point* std::uninitialized_fill_n(type_point* __first, unsigned __n, const type_point* __x);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0xc3d80) TBlackMarket* std::uninitialized_copy(TBlackMarket* __first, TBlackMarket* __last, TBlackMarket* __result);
// CODEVIEW(..\stlport\stl_uninitialized.h:263, dc 0xc3dbc) TBlackMarket* std::uninitialized_fill_n(TBlackMarket* __first, unsigned __n, const TBlackMarket* __x);
// CODEVIEW(..\stlport\stl_algobase.h:442, dc 0xc3df8) TBlackMarket* std::copy_backward(TBlackMarket* __first, TBlackMarket* __last, TBlackMarket* __result);
// CODEVIEW(..\stlport\stl_algobase.h:495, dc 0xc3e50) void std::fill(TBlackMarket* __first, TBlackMarket* __last, const TBlackMarket* __value);
// CODEVIEW(..\stlport\stl_vector.c:248, dc 0xc3f30) void std::vector<town,std::allocator<town> >::_M_insert_overflow(town* __position, const town* __x, unsigned __fill_len);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0xc4064) town* std::uninitialized_copy(town* __first, town* __last, town* __result);
// CODEVIEW(..\stlport\stl_algobase.h:442, dc 0xc40a0) town* std::copy_backward(town* __first, town* __last, town* __result);
// CODEVIEW(..\stlport\stl_algobase.h:495, dc 0xc40f8) void std::fill(town* __first, town* __last, const town* __value);
// CODEVIEW(..\stlport\stl_uninitialized.h:263, dc 0xc4218) town* std::uninitialized_fill_n(town* __first, unsigned __n, const town* __x);
// CODEVIEW(..\stlport\stl_vector.c:248, dc 0xc4254) void std::vector<Sign,std::allocator<Sign> >::_M_insert_overflow(Sign* __position, const Sign* __x, unsigned __fill_len);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0xc4374) Sign* std::uninitialized_copy(Sign* __first, Sign* __last, Sign* __result);
// CODEVIEW(..\stlport\stl_algobase.h:442, dc 0xc43b0) Sign* std::copy_backward(Sign* __first, Sign* __last, Sign* __result);
// CODEVIEW(..\stlport\stl_algobase.h:495, dc 0xc4408) void std::fill(Sign* __first, Sign* __last, const Sign* __value);
// CODEVIEW(..\stlport\stl_uninitialized.h:263, dc 0xc4440) Sign* std::uninitialized_fill_n(Sign* __first, unsigned __n, const Sign* __x);
// CODEVIEW(..\stlport\stl_vector.c:248, dc 0xc447c) void std::vector<mine,std::allocator<mine> >::_M_insert_overflow(mine* __position, const mine* __x, unsigned __fill_len);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0xc45ac) mine* std::uninitialized_copy(mine* __first, mine* __last, mine* __result);
// CODEVIEW(..\stlport\stl_algobase.h:442, dc 0xc45e8) mine* std::copy_backward(mine* __first, mine* __last, mine* __result);
// CODEVIEW(..\stlport\stl_algobase.h:495, dc 0xc4640) void std::fill(mine* __first, mine* __last, const mine* __value);
// CODEVIEW(..\stlport\stl_uninitialized.h:263, dc 0xc4688) mine* std::uninitialized_fill_n(mine* __first, unsigned __n, const mine* __x);
// CODEVIEW(..\stlport\stl_vector.c:248, dc 0xc46c4) void std::vector<garrison,std::allocator<garrison> >::_M_insert_overflow(garrison* __position, const garrison* __x, unsigned __fill_len);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0xc47e4) garrison* std::uninitialized_copy(garrison* __first, garrison* __last, garrison* __result);
// CODEVIEW(..\stlport\stl_algobase.h:442, dc 0xc4820) garrison* std::copy_backward(garrison* __first, garrison* __last, garrison* __result);
// CODEVIEW(..\stlport\stl_algobase.h:495, dc 0xc4878) void std::fill(garrison* __first, garrison* __last, const garrison* __value);
// CODEVIEW(..\stlport\stl_uninitialized.h:263, dc 0xc4938) garrison* std::uninitialized_fill_n(garrison* __first, unsigned __n, const garrison* __x);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0xc4974) boat* std::uninitialized_copy(boat* __first, boat* __last, boat* __result);
// CODEVIEW(..\stlport\stl_uninitialized.h:263, dc 0xc49b0) boat* std::uninitialized_fill_n(boat* __first, unsigned __n, const boat* __x);
// CODEVIEW(..\stlport\stl_algobase.h:442, dc 0xc49ec) boat* std::copy_backward(boat* __first, boat* __last, boat* __result);
// CODEVIEW(..\stlport\stl_algobase.h:495, dc 0xc4a44) void std::fill(boat* __first, boat* __last, const boat* __value);
// CODEVIEW(..\stlport\stl_vector.c:283, dc 0xc4ac8) void std::vector<type_university,std::allocator<type_university> >::_M_fill_insert(type_university* __position, unsigned __n, const type_university* __x);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0xc4c34) type_university* std::uninitialized_copy(type_university* __first, type_university* __last, type_university* __result);
// CODEVIEW(..\stlport\stl_uninitialized.h:263, dc 0xc4c70) type_university* std::uninitialized_fill_n(type_university* __first, unsigned __n, const type_university* __x);
// CODEVIEW(..\stlport\stl_vector.c:283, dc 0xc4cac) void std::vector<type_creature_bank,std::allocator<type_creature_bank> >::_M_fill_insert(type_creature_bank* __position, unsigned __n, const type_creature_bank* __x);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0xc4e40) type_creature_bank* std::uninitialized_copy(type_creature_bank* __first, type_creature_bank* __last, type_creature_bank* __result);
// CODEVIEW(..\stlport\stl_uninitialized.h:263, dc 0xc4e7c) type_creature_bank* std::uninitialized_fill_n(type_creature_bank* __first, unsigned __n, const type_creature_bank* __x);
// CODEVIEW(..\stlport\stl_vector.c:248, dc 0xc4eb8) void std::vector<game::TRumour,std::allocator<game::TRumour> >::_M_insert_overflow(game::TRumour* __position, const game::TRumour* __x, unsigned __fill_len);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0xc4fd8) game::TRumour* std::uninitialized_copy(game::TRumour* __first, game::TRumour* __last, game::TRumour* __result);
// CODEVIEW(..\stlport\stl_algobase.h:442, dc 0xc5014) game::TRumour* std::copy_backward(game::TRumour* __first, game::TRumour* __last, game::TRumour* __result);
// CODEVIEW(..\stlport\stl_algobase.h:495, dc 0xc506c) void std::fill(game::TRumour* __first, game::TRumour* __last, const game::TRumour* __value);
// CODEVIEW(..\stlport\stl_uninitialized.h:263, dc 0xc50a4) game::TRumour* std::uninitialized_fill_n(game::TRumour* __first, unsigned __n, const game::TRumour* __x);
// CODEVIEW(..\stlport\stl_vector.c:283, dc 0xc50e0) void std::vector<enum TArtifact,std::allocator<enum TArtifact> >::_M_fill_insert(TArtifact* __position, unsigned __n, const TArtifact* __x);
// CODEVIEW(..\stlport\stl_vector.c:283, dc 0xc5218) void std::vector<generator,std::allocator<generator> >::_M_fill_insert(generator* __position, unsigned __n, const generator* __x);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0xc5474) hero* std::uninitialized_copy(hero* __first, hero* __last, hero* __result);
// CODEVIEW(..\stlport\stl_uninitialized.h:263, dc 0xc54b0) hero* std::uninitialized_fill_n(hero* __first, unsigned __n, const hero* __x);
// CODEVIEW(..\stlport\stl_iterator_base.h:262, dc 0xc54ec) type_point* std::value_type(const type_point* __formal);
// CODEVIEW(..\stlport\stl_construct.h:121, dc 0xc54f4) void std::__destroy(type_point* __first, type_point* __last, type_point* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:243, dc 0xc5518) std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const type_point* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:291, dc 0xc552c) int* std::distance_type(const type_point* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:209, dc 0xc5534) type_point* std::__copy(type_point* __first, type_point* __last, type_point* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_construct.h:53, dc 0xc5594) void std::__destroy_aux(type_point* __pointer, __false_type __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:262, dc 0xc559c) TownExtra* std::value_type(const TownExtra* __formal);
// CODEVIEW(..\stlport\stl_construct.h:121, dc 0xc55a4) void std::__destroy(TownExtra* __first, TownExtra* __last, TownExtra* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:262, dc 0xc55c8) TBlackMarket* std::value_type(const TBlackMarket* __formal);
// CODEVIEW(..\stlport\stl_construct.h:121, dc 0xc55d0) void std::__destroy(TBlackMarket* __first, TBlackMarket* __last, TBlackMarket* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:262, dc 0xc55f4) town* std::value_type(const town* __formal);
// CODEVIEW(..\stlport\stl_construct.h:121, dc 0xc55fc) void std::__destroy(town* __first, town* __last, town* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:262, dc 0xc5620) Sign* std::value_type(const Sign* __formal);
// CODEVIEW(..\stlport\stl_construct.h:121, dc 0xc5628) void std::__destroy(Sign* __first, Sign* __last, Sign* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:262, dc 0xc564c) mine* std::value_type(const mine* __formal);
// CODEVIEW(..\stlport\stl_construct.h:121, dc 0xc5654) void std::__destroy(mine* __first, mine* __last, mine* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:262, dc 0xc5678) generator* std::value_type(const generator* __formal);
// CODEVIEW(..\stlport\stl_construct.h:121, dc 0xc5680) void std::__destroy(generator* __first, generator* __last, generator* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:262, dc 0xc56a4) garrison* std::value_type(const garrison* __formal);
// CODEVIEW(..\stlport\stl_construct.h:121, dc 0xc56ac) void std::__destroy(garrison* __first, garrison* __last, garrison* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:262, dc 0xc56d0) boat* std::value_type(const boat* __formal);
// CODEVIEW(..\stlport\stl_construct.h:121, dc 0xc56d8) void std::__destroy(boat* __first, boat* __last, boat* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:262, dc 0xc56fc) type_university* std::value_type(const type_university* __formal);
// CODEVIEW(..\stlport\stl_construct.h:121, dc 0xc5704) void std::__destroy(type_university* __first, type_university* __last, type_university* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:262, dc 0xc5728) type_creature_bank* std::value_type(const type_creature_bank* __formal);
// CODEVIEW(..\stlport\stl_construct.h:121, dc 0xc5730) void std::__destroy(type_creature_bank* __first, type_creature_bank* __last, type_creature_bank* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:262, dc 0xc5754) game::TRumour* std::value_type(const game::TRumour* __formal);
// CODEVIEW(..\stlport\stl_construct.h:121, dc 0xc575c) void std::__destroy(game::TRumour* __first, game::TRumour* __last, game::TRumour* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:262, dc 0xc5780) hero* std::value_type(const hero* __formal);
// CODEVIEW(..\stlport\stl_construct.h:121, dc 0xc5788) void std::__destroy(hero* __first, hero* __last, hero* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:243, dc 0xc57ac) std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const TBlackMarket* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:291, dc 0xc57c0) int* std::distance_type(const TBlackMarket* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:209, dc 0xc57c8) TBlackMarket* std::__copy(TBlackMarket* __first, TBlackMarket* __last, TBlackMarket* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:243, dc 0xc58d8) std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const town* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:291, dc 0xc58ec) int* std::distance_type(const town* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:209, dc 0xc58f4) town* std::__copy(town* __first, town* __last, town* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:243, dc 0xc5a50) std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const Sign* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:291, dc 0xc5a64) int* std::distance_type(const Sign* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:209, dc 0xc5a6c) Sign* std::__copy(Sign* __first, Sign* __last, Sign* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:243, dc 0xc5ac0) std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const mine* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:291, dc 0xc5ad4) int* std::distance_type(const mine* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:209, dc 0xc5adc) mine* std::__copy(mine* __first, mine* __last, mine* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:243, dc 0xc5b50) std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const garrison* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:291, dc 0xc5b64) int* std::distance_type(const garrison* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:209, dc 0xc5b6c) garrison* std::__copy(garrison* __first, garrison* __last, garrison* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:243, dc 0xc5c4c) std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const boat* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:291, dc 0xc5c60) int* std::distance_type(const boat* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:209, dc 0xc5c68) boat* std::__copy(boat* __first, boat* __last, boat* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:243, dc 0xc5d24) std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const game::TRumour* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:291, dc 0xc5d38) int* std::distance_type(const game::TRumour* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:209, dc 0xc5d40) game::TRumour* std::__copy(game::TRumour* __first, game::TRumour* __last, game::TRumour* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0xc5d94) TArtifact* std::uninitialized_copy(const TArtifact* __first, const TArtifact* __last, TArtifact* __result);
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0xc5dd0) std::allocator<CObjectType>* std::__stl_alloc_rebind(std::allocator<CObjectType>* __a, const CObjectType* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0xc5ddc) std::allocator<CObject>* std::__stl_alloc_rebind(std::allocator<CObject>* __a, const CObject* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0xc5de8) std::allocator<CSprite* std::__stl_alloc_rebind(std::allocator<CSprite* __a, CSprite** __formal);
// CODEVIEW(..\stlport\stl_algobase.h:322, dc 0xc5df4) TArtifact* std::copy(TArtifact* __first, TArtifact* __last, TArtifact* __result);
// CODEVIEW(..\stlport\stl_algobase.h:322, dc 0xc5e4c) generator* std::copy(generator* __first, generator* __last, generator* __result);
// CODEVIEW(..\stlport\stl_algobase.h:322, dc 0xc5ea4) type_university* std::copy(type_university* __first, type_university* __last, type_university* __result);
// CODEVIEW(..\stlport\stl_algobase.h:322, dc 0xc5efc) type_creature_bank* std::copy(type_creature_bank* __first, type_creature_bank* __last, type_creature_bank* __result);
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0xc5fc4) town* std::_STL_alloc_proxy<town *,town,std::allocator<town> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0xc5ffc) Sign* std::_STL_alloc_proxy<Sign *,Sign,std::allocator<Sign> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0xc6034) mine* std::_STL_alloc_proxy<mine *,mine,std::allocator<mine> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0xc606c) garrison* std::_STL_alloc_proxy<garrison *,garrison,std::allocator<garrison> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0xc60a4) game::TRumour* std::_STL_alloc_proxy<game::TRumour *,game::TRumour,std::allocator<game::TRumour> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0xc60dc) town* std::allocator<town>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0xc611c) Sign* std::allocator<Sign>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0xc6158) mine* std::allocator<mine>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0xc6194) garrison* std::allocator<garrison>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0xc61d0) game::TRumour* std::allocator<game::TRumour>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:262, dc 0xc620c) CObjectType* std::value_type(const CObjectType* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0xc6214) CObjectType* std::__uninitialized_copy(CObjectType* __first, CObjectType* __last, CObjectType* __result, CObjectType* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:255, dc 0xc6244) CObjectType* std::__uninitialized_fill_n(CObjectType* __first, unsigned __n, const CObjectType* __x, CObjectType* __formal);
// CODEVIEW(..\stlport\stl_construct.h:121, dc 0xc6274) void std::__destroy(CObjectType* __first, CObjectType* __last, CObjectType* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:262, dc 0xc6298) CObject* std::value_type(const CObject* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0xc62a0) CObject* std::__uninitialized_copy(CObject* __first, CObject* __last, CObject* __result, CObject* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:255, dc 0xc62d0) CObject* std::__uninitialized_fill_n(CObject* __first, unsigned __n, const CObject* __x, CObject* __formal);
// CODEVIEW(..\stlport\stl_construct.h:121, dc 0xc6300) void std::__destroy(CObject* __first, CObject* __last, CObject* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:262, dc 0xc6324) CSprite** std::value_type(CSprite** __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0xc632c) CSprite** std::__uninitialized_copy(CSprite** __first, CSprite** __last, CSprite** __result, CSprite** __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:255, dc 0xc635c) CSprite** std::__uninitialized_fill_n(CSprite** __first, unsigned __n, CSprite** __x, CSprite** __formal);
// CODEVIEW(..\stlport\stl_construct.h:121, dc 0xc638c) void std::__destroy(CSprite** __first, CSprite** __last, CSprite** __formal);
// CODEVIEW(..\stlport\stl_algobase.h:442, dc 0xc63b0) type_point* std::copy_backward(type_point* __first, type_point* __last, type_point* __result);
// CODEVIEW(..\stlport\stl_algobase.h:495, dc 0xc6408) void std::fill(type_point* __first, type_point* __last, const type_point* __value);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0xc644c) type_point* std::__uninitialized_copy(type_point* __first, type_point* __last, type_point* __result, type_point* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:255, dc 0xc647c) type_point* std::__uninitialized_fill_n(type_point* __first, unsigned __n, const type_point* __x, type_point* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0xc64ac) TBlackMarket* std::__uninitialized_copy(TBlackMarket* __first, TBlackMarket* __last, TBlackMarket* __result, TBlackMarket* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:255, dc 0xc64dc) TBlackMarket* std::__uninitialized_fill_n(TBlackMarket* __first, unsigned __n, const TBlackMarket* __x, TBlackMarket* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:382, dc 0xc650c) TBlackMarket* std::__copy_backward(TBlackMarket* __first, TBlackMarket* __last, TBlackMarket* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_construct.h:85, dc 0xc6618) void std::construct(town* __p, const town* __value);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0xc674c) town* std::__uninitialized_copy(town* __first, town* __last, town* __result, town* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:382, dc 0xc677c) town* std::__copy_backward(town* __first, town* __last, town* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:255, dc 0xc68d4) town* std::__uninitialized_fill_n(town* __first, unsigned __n, const town* __x, town* __formal);
// CODEVIEW(..\stlport\stl_construct.h:85, dc 0xc6904) void std::construct(Sign* __p, const Sign* __value);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0xc6948) Sign* std::__uninitialized_copy(Sign* __first, Sign* __last, Sign* __result, Sign* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:382, dc 0xc6978) Sign* std::__copy_backward(Sign* __first, Sign* __last, Sign* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:255, dc 0xc69cc) Sign* std::__uninitialized_fill_n(Sign* __first, unsigned __n, const Sign* __x, Sign* __formal);
// CODEVIEW(..\stlport\stl_construct.h:85, dc 0xc69fc) void std::construct(mine* __p, const mine* __value);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0xc6a54) mine* std::__uninitialized_copy(mine* __first, mine* __last, mine* __result, mine* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:382, dc 0xc6a84) mine* std::__copy_backward(mine* __first, mine* __last, mine* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:255, dc 0xc6af4) mine* std::__uninitialized_fill_n(mine* __first, unsigned __n, const mine* __x, mine* __formal);
// CODEVIEW(..\stlport\stl_construct.h:85, dc 0xc6b24) void std::construct(garrison* __p, const garrison* __value);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0xc6bf8) garrison* std::__uninitialized_copy(garrison* __first, garrison* __last, garrison* __result, garrison* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:382, dc 0xc6c28) garrison* std::__copy_backward(garrison* __first, garrison* __last, garrison* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:255, dc 0xc6cfc) garrison* std::__uninitialized_fill_n(garrison* __first, unsigned __n, const garrison* __x, garrison* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0xc6d2c) boat* std::__uninitialized_copy(boat* __first, boat* __last, boat* __result, boat* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:255, dc 0xc6d5c) boat* std::__uninitialized_fill_n(boat* __first, unsigned __n, const boat* __x, boat* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:382, dc 0xc6d8c) boat* std::__copy_backward(boat* __first, boat* __last, boat* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:442, dc 0xc6e3c) type_university* std::copy_backward(type_university* __first, type_university* __last, type_university* __result);
// CODEVIEW(..\stlport\stl_algobase.h:495, dc 0xc6e94) void std::fill(type_university* __first, type_university* __last, const type_university* __value);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0xc6ee8) type_university* std::__uninitialized_copy(type_university* __first, type_university* __last, type_university* __result, type_university* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:255, dc 0xc6f18) type_university* std::__uninitialized_fill_n(type_university* __first, unsigned __n, const type_university* __x, type_university* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:442, dc 0xc6f48) type_creature_bank* std::copy_backward(type_creature_bank* __first, type_creature_bank* __last, type_creature_bank* __result);
// CODEVIEW(..\stlport\stl_algobase.h:495, dc 0xc6fa0) void std::fill(type_creature_bank* __first, type_creature_bank* __last, const type_creature_bank* __value);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0xc6fd8) type_creature_bank* std::__uninitialized_copy(type_creature_bank* __first, type_creature_bank* __last, type_creature_bank* __result, type_creature_bank* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:255, dc 0xc7008) type_creature_bank* std::__uninitialized_fill_n(type_creature_bank* __first, unsigned __n, const type_creature_bank* __x, type_creature_bank* __formal);
// CODEVIEW(..\stlport\stl_construct.h:85, dc 0xc7038) void std::construct(game::TRumour* __p, const game::TRumour* __value);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0xc707c) game::TRumour* std::__uninitialized_copy(game::TRumour* __first, game::TRumour* __last, game::TRumour* __result, game::TRumour* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:382, dc 0xc70ac) game::TRumour* std::__copy_backward(game::TRumour* __first, game::TRumour* __last, game::TRumour* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:255, dc 0xc7100) game::TRumour* std::__uninitialized_fill_n(game::TRumour* __first, unsigned __n, const game::TRumour* __x, game::TRumour* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:442, dc 0xc7130) TArtifact* std::copy_backward(TArtifact* __first, TArtifact* __last, TArtifact* __result);
// CODEVIEW(..\stlport\stl_algobase.h:495, dc 0xc7188) void std::fill(TArtifact* __first, TArtifact* __last, const TArtifact* __value);
// CODEVIEW(..\stlport\stl_vector.c:248, dc 0xc71b0) void std::vector<generator,std::allocator<generator> >::_M_insert_overflow(generator* __position, const generator* __x, unsigned __fill_len);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0xc72e0) generator* std::uninitialized_copy(generator* __first, generator* __last, generator* __result);
// CODEVIEW(..\stlport\stl_algobase.h:442, dc 0xc731c) generator* std::copy_backward(generator* __first, generator* __last, generator* __result);
// CODEVIEW(..\stlport\stl_algobase.h:495, dc 0xc7374) void std::fill(generator* __first, generator* __last, const generator* __value);
// CODEVIEW(..\stlport\stl_uninitialized.h:263, dc 0xc746c) generator* std::uninitialized_fill_n(generator* __first, unsigned __n, const generator* __x);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0xc74a8) hero* std::__uninitialized_copy(hero* __first, hero* __last, hero* __result, hero* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:255, dc 0xc74d8) hero* std::__uninitialized_fill_n(hero* __first, unsigned __n, const hero* __x, hero* __formal);
// CODEVIEW(..\stlport\stl_construct.h:110, dc 0xc7508) void std::__destroy_aux(type_point* __first, type_point* __last, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:110, dc 0xc753c) void std::__destroy_aux(TownExtra* __first, TownExtra* __last, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:110, dc 0xc7570) void std::__destroy_aux(TBlackMarket* __first, TBlackMarket* __last, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:110, dc 0xc75a4) void std::__destroy_aux(town* __first, town* __last, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:110, dc 0xc75dc) void std::__destroy_aux(Sign* __first, Sign* __last, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:110, dc 0xc7610) void std::__destroy_aux(mine* __first, mine* __last, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:110, dc 0xc7644) void std::__destroy_aux(generator* __first, generator* __last, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:110, dc 0xc7678) void std::__destroy_aux(garrison* __first, garrison* __last, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:110, dc 0xc76ac) void std::__destroy_aux(boat* __first, boat* __last, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:110, dc 0xc76e0) void std::__destroy_aux(type_university* __first, type_university* __last, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:110, dc 0xc7714) void std::__destroy_aux(type_creature_bank* __first, type_creature_bank* __last, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:110, dc 0xc7748) void std::__destroy_aux(game::TRumour* __first, game::TRumour* __last, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:110, dc 0xc777c) void std::__destroy_aux(hero* __first, hero* __last, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0xc77b4) TArtifact* std::__uninitialized_copy(const TArtifact* __first, const TArtifact* __last, TArtifact* __result, TArtifact* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:243, dc 0xc77e4) std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const TArtifact* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:291, dc 0xc77f8) int* std::distance_type(const TArtifact* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:209, dc 0xc7800) TArtifact* std::__copy(TArtifact* __first, TArtifact* __last, TArtifact* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:243, dc 0xc7840) std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const generator* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:291, dc 0xc7854) int* std::distance_type(const generator* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:209, dc 0xc785c) generator* std::__copy(generator* __first, generator* __last, generator* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:243, dc 0xc7994) std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const type_university* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:291, dc 0xc79a8) int* std::distance_type(const type_university* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:209, dc 0xc79b0) type_university* std::__copy(type_university* __first, type_university* __last, type_university* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:243, dc 0xc7a1c) std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const type_creature_bank* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:291, dc 0xc7a30) int* std::distance_type(const type_creature_bank* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:209, dc 0xc7a38) type_creature_bank* std::__copy(type_creature_bank* __first, type_creature_bank* __last, type_creature_bank* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0xc7b98) generator* std::_STL_alloc_proxy<generator *,generator,std::allocator<generator> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0xc7bd0) generator* std::allocator<generator>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0xc7c0c) CObjectType* std::__uninitialized_copy_aux(CObjectType* __first, CObjectType* __last, CObjectType* __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:239, dc 0xc7c50) CObjectType* std::__uninitialized_fill_n_aux(CObjectType* __first, unsigned __n, const CObjectType* __x, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:110, dc 0xc7c94) void std::__destroy_aux(CObjectType* __first, CObjectType* __last, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0xc7cc8) CObject* std::__uninitialized_copy_aux(CObject* __first, CObject* __last, CObject* __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:239, dc 0xc7d0c) CObject* std::__uninitialized_fill_n_aux(CObject* __first, unsigned __n, const CObject* __x, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:110, dc 0xc7d50) void std::__destroy_aux(CObject* __first, CObject* __last, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0xc7d84) CSprite** std::__uninitialized_copy_aux(CSprite** __first, CSprite** __last, CSprite** __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:239, dc 0xc7dc8) CSprite** std::__uninitialized_fill_n_aux(CSprite** __first, unsigned __n, CSprite** __x, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:110, dc 0xc7e0c) void std::__destroy_aux(CSprite** __first, CSprite** __last, __false_type __formal);
// CODEVIEW(..\stlport\stl_algobase.h:382, dc 0xc7e40) type_point* std::__copy_backward(type_point* __first, type_point* __last, type_point* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0xc7ea0) type_point* std::__uninitialized_copy_aux(type_point* __first, type_point* __last, type_point* __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:239, dc 0xc7ee4) type_point* std::__uninitialized_fill_n_aux(type_point* __first, unsigned __n, const type_point* __x, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0xc7f28) TBlackMarket* std::__uninitialized_copy_aux(TBlackMarket* __first, TBlackMarket* __last, TBlackMarket* __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:239, dc 0xc7f6c) TBlackMarket* std::__uninitialized_fill_n_aux(TBlackMarket* __first, unsigned __n, const TBlackMarket* __x, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0xc7fb0) town* std::__uninitialized_copy_aux(town* __first, town* __last, town* __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:239, dc 0xc7ffc) town* std::__uninitialized_fill_n_aux(town* __first, unsigned __n, const town* __x, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0xc8044) Sign* std::__uninitialized_copy_aux(Sign* __first, Sign* __last, Sign* __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:239, dc 0xc8088) Sign* std::__uninitialized_fill_n_aux(Sign* __first, unsigned __n, const Sign* __x, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0xc80cc) mine* std::__uninitialized_copy_aux(mine* __first, mine* __last, mine* __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:239, dc 0xc8110) mine* std::__uninitialized_fill_n_aux(mine* __first, unsigned __n, const mine* __x, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0xc8154) garrison* std::__uninitialized_copy_aux(garrison* __first, garrison* __last, garrison* __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:239, dc 0xc8198) garrison* std::__uninitialized_fill_n_aux(garrison* __first, unsigned __n, const garrison* __x, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0xc81dc) boat* std::__uninitialized_copy_aux(boat* __first, boat* __last, boat* __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:239, dc 0xc8220) boat* std::__uninitialized_fill_n_aux(boat* __first, unsigned __n, const boat* __x, __false_type __formal);
// CODEVIEW(..\stlport\stl_algobase.h:382, dc 0xc8264) type_university* std::__copy_backward(type_university* __first, type_university* __last, type_university* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0xc82cc) type_university* std::__uninitialized_copy_aux(type_university* __first, type_university* __last, type_university* __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:239, dc 0xc8310) type_university* std::__uninitialized_fill_n_aux(type_university* __first, unsigned __n, const type_university* __x, __false_type __formal);
// CODEVIEW(..\stlport\stl_algobase.h:382, dc 0xc8354) type_creature_bank* std::__copy_backward(type_creature_bank* __first, type_creature_bank* __last, type_creature_bank* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0xc83b4) type_creature_bank* std::__uninitialized_copy_aux(type_creature_bank* __first, type_creature_bank* __last, type_creature_bank* __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:239, dc 0xc83f8) type_creature_bank* std::__uninitialized_fill_n_aux(type_creature_bank* __first, unsigned __n, const type_creature_bank* __x, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0xc843c) game::TRumour* std::__uninitialized_copy_aux(game::TRumour* __first, game::TRumour* __last, game::TRumour* __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:239, dc 0xc8480) game::TRumour* std::__uninitialized_fill_n_aux(game::TRumour* __first, unsigned __n, const game::TRumour* __x, __false_type __formal);
// CODEVIEW(..\stlport\stl_vector.c:207, dc 0xc84c4) std::vector<enum* std::vector<enum TArtifact,std::allocator<enum TArtifact> >::operator=(const std::vector<enum* __x);
// CODEVIEW(..\stlport\stl_algobase.h:382, dc 0xc8628) TArtifact* std::__copy_backward(TArtifact* __first, TArtifact* __last, TArtifact* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_construct.h:85, dc 0xc8668) void std::construct(generator* __p, const generator* __value);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0xc8778) generator* std::__uninitialized_copy(generator* __first, generator* __last, generator* __result, generator* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:382, dc 0xc87a8) generator* std::__copy_backward(generator* __first, generator* __last, generator* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:255, dc 0xc88d0) generator* std::__uninitialized_fill_n(generator* __first, unsigned __n, const generator* __x, generator* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0xc8900) hero* std::__uninitialized_copy_aux(hero* __first, hero* __last, hero* __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:239, dc 0xc894c) hero* std::__uninitialized_fill_n_aux(hero* __first, unsigned __n, const hero* __x, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:59, dc 0xc8994) void std::destroy(TownExtra* __pointer);
// CODEVIEW(..\stlport\stl_construct.h:59, dc 0xc89b4) void std::destroy(TBlackMarket* __pointer);
// CODEVIEW(..\stlport\stl_construct.h:59, dc 0xc89d4) void std::destroy(town* __pointer);
// CODEVIEW(..\stlport\stl_construct.h:59, dc 0xc89f4) void std::destroy(Sign* __pointer);
// CODEVIEW(..\stlport\stl_construct.h:59, dc 0xc8a14) void std::destroy(mine* __pointer);
// CODEVIEW(..\stlport\stl_construct.h:59, dc 0xc8a34) void std::destroy(generator* __pointer);
// CODEVIEW(..\stlport\stl_construct.h:59, dc 0xc8a54) void std::destroy(garrison* __pointer);
// CODEVIEW(..\stlport\stl_construct.h:59, dc 0xc8a74) void std::destroy(boat* __pointer);
// CODEVIEW(..\stlport\stl_construct.h:59, dc 0xc8a94) void std::destroy(type_university* __pointer);
// CODEVIEW(..\stlport\stl_construct.h:59, dc 0xc8ab4) void std::destroy(type_creature_bank* __pointer);
// CODEVIEW(..\stlport\stl_construct.h:59, dc 0xc8ad4) void std::destroy(game::TRumour* __pointer);
// CODEVIEW(..\stlport\stl_construct.h:59, dc 0xc8af4) void std::destroy(hero* __pointer);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0xc8b14) TArtifact* std::__uninitialized_copy_aux(const TArtifact* __first, const TArtifact* __last, TArtifact* __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_vector.h:199, dc 0xc8b58) unsigned std::vector<enum TArtifact,std::allocator<enum TArtifact> >::capacity();
// CODEVIEW(..\stlport\stl_construct.h:59, dc 0xc8b70) void std::destroy(CObjectType* __pointer);
// CODEVIEW(..\stlport\stl_construct.h:59, dc 0xc8b90) void std::destroy(CObject* __pointer);
// CODEVIEW(..\stlport\stl_construct.h:59, dc 0xc8bb0) void std::destroy(CSprite** __pointer);
// CODEVIEW(..\stlport\stl_vector.h:514, dc 0xc8bd0) std::vector<enum TArtifact,std::allocator<enum TArtifact> >::_M_allocate_and_copy(unsigned __n, const TArtifact* __first, const TArtifact* __last);
// CODEVIEW(..\stlport\stl_algobase.h:322, dc 0xc8c10) TArtifact* std::copy(const TArtifact* __first, const TArtifact* __last, TArtifact* __result);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0xc8c68) generator* std::__uninitialized_copy_aux(generator* __first, generator* __last, generator* __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:239, dc 0xc8cac) generator* std::__uninitialized_fill_n_aux(generator* __first, unsigned __n, const generator* __x, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:53, dc 0xc8cf0) void std::__destroy_aux(TownExtra* __pointer, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:53, dc 0xc8cf8) void std::__destroy_aux(TBlackMarket* __pointer, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:53, dc 0xc8d00) void std::__destroy_aux(town* __pointer, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:53, dc 0xc8d08) void std::__destroy_aux(Sign* __pointer, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:53, dc 0xc8d28) void std::__destroy_aux(mine* __pointer, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:53, dc 0xc8d30) void std::__destroy_aux(generator* __pointer, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:53, dc 0xc8d38) void std::__destroy_aux(garrison* __pointer, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:53, dc 0xc8d40) void std::__destroy_aux(boat* __pointer, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:53, dc 0xc8d48) void std::__destroy_aux(type_university* __pointer, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:53, dc 0xc8d50) void std::__destroy_aux(type_creature_bank* __pointer, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:53, dc 0xc8d70) void std::__destroy_aux(game::TRumour* __pointer, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:53, dc 0xc8d90) void std::__destroy_aux(hero* __pointer, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:53, dc 0xc8e40) void std::__destroy_aux(CObjectType* __pointer, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:53, dc 0xc8e60) void std::__destroy_aux(CObject* __pointer, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:53, dc 0xc8e68) void std::__destroy_aux(CSprite** __pointer, __false_type __formal);
// CODEVIEW(..\stlport\stl_algobase.h:209, dc 0xc8e70) TArtifact* std::__copy(const TArtifact* __first, const TArtifact* __last, TArtifact* __result, std::random_access_iterator_tag __formal, int* __formal);

// --- town ---
// CODEVIEW(E:\gamedcs\Town.h:337, dc 0xbcc40) unsigned char town::IsCastle();
// CODEVIEW(E:\gamedcs\Town.h:342, dc 0xbccb4) unsigned char town::IsCapitol();

// --- type_creature_bank ---
// CODEVIEW(E:\gamedcs\game.cpp:2774, dc 0xa7abc) unsigned char type_creature_bank::load(void* infile);
// CODEVIEW(E:\gamedcs\game.cpp:2790, dc 0xa7b60) unsigned char type_creature_bank::save(void* outfile);
// CODEVIEW(E:\gamedcs\game.cpp:5595, dc 0xbd534) void type_creature_bank::type_creature_bank();
// CODEVIEW(E:\gamedcs\game.cpp:5595, dc 0xbd58c) void type_creature_bank::~type_creature_bank();
// CODEVIEW(..\stlport\stl_alloc.h:970, dc 0xc2be4) void type_creature_bank::type_creature_bank(const type_creature_bank* __that);
// CODEVIEW(..\stlport\stl_alloc.h:739, dc 0xc7a98) type_creature_bank* type_creature_bank::operator=(const type_creature_bank* __that);
// CODEVIEW(..\stlport\stl_vector.h:609, dc 0xc8dd0) void* type_creature_bank::`scalar deleting destructor'(unsigned __flags);

// Dreamcast Game.h proves the complete 200-byte class and its single char
// array. Retail's adventure/combat cheat handlers inline the string-taking
// constructor and compare members while sharing the encoder at 0x402a30.
// The default constructor and GetCode are ordinary declarations in DC type
// 0x3dc2 (method types 0x3dc5/0x3dcb), without procedure/source locations.
// Neither has an active caller; leave their bodies unreconstructed.
// Before normalization (type): TCheatCode.
#ifndef CheatCode
#define CheatCode TCheatCode
#endif
class CheatCode {
public:
    CheatCode();
    CheatCode(const char* value) { encode(value); }
    bool compare(const char* value) const
    {
        return _strcmpi(m_code, value) == 0;
    }
    const char* getCode() const;

private:
    void encode(const char* value);
    static const char* s_a;
    static const char* s_b;
    char m_code[200];
};
SIZE(CheatCode, 200);

// E:\gamedcs\Game.h:1439
VA(0x00402a30, 0xA1)
inline void CheatCode::encode(const char* value)
{
    int i = 0;
    const int maximum = 199;
    for (;;) {
        int length = static_cast<int>(strlen(value));
        const int* limit = &maximum;
        if (length <= maximum)
            limit = &length;
        if (i >= *limit)
            break;

        if (isalpha(value[i]))
            m_code[i] = s_b[tolower(value[i]) - 'a'];
        else
            m_code[i] = value[i];
        i++;
    }
    m_code[i] = 0;
}

#endif  /* HOMM3_GAME_H */
