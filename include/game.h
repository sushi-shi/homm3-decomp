#ifndef HOMM3_GAME_H
#define HOMM3_GAME_H

#include <ctype.h>
#include <direct.h>
#include <map>
#include <memory>
#include <string.h>
#include <vector>

#include "advmgr_objects.h"
#include "creature_bank_types.h"
#include "creaturetype.h"
#include "creaturetype_fwd.h"
#include "customcampaign.h"
#include "campaignbrief.h"
#include "hero.h"
#include "mapcell.h"
#include "netmsg.h"
#include "savegame.h"
#include "secondaryskill.h"
#include "seerhut.h"
#include "smackmgr.h"
#include "struct.h"
#include "town.h"
#include "victorylossconditions.h"

// The one decoded value of game::field_1f63e shared by events.obj and
// philai.obj: Sunday is the seventh day.  DoEventTemple doubles its morale
// reward on this rung; move_hero stops a low-value full-hourglass move on it.
enum EDayOfWeek {
    DAY_OF_WEEK_SUNDAY = 7
};

int __fastcall readHeroId(TAbstractFile* infile, int mapVersion);
int __fastcall loadHeroId(TAbstractFile* infile, int saveVersion);

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



class town;

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
    TSecondarySkill m_skills[4];
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
// Original bool gbInCampaign (DC public ?gbInCampaign@@3_NA, data 0x2c9cc).
// Retail 0x69774c selects H3SVC versus H3SVG. SavedGameHeader::reset and
// game::load copy it directly to/from the canonical saved-header bool.
// This is not DC's separate campaignMode selection-window flag (0x327d8).
extern bool g_inCampaign;

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
    class TPlayerSlotAttributes {
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
        TPlayerSlotAttributes()
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
        void readMapPlayerSlot(TAbstractFile* infile, int mapVersion);
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
    TPlayerSlotAttributes m_playerSlotAttributes[8];
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
SIZE(CMapHeaderData::TPlayerSlotAttributes, 0x44);



class NewSMapHeader : public CMapHeaderData {
public:
    std::string m_mapName;
    std::string m_mapDescription;
    std::bitset<156> m_availableHeroes;
    int save(TAbstractFile* outfile);
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
    int read(TAbstractFile* infile, int campaignMap);
    // DC game.cpp:7232 and the class method record name this static
    // string-reference reader. Retail 0x4c6010 uses the same two-register
    // ABI as game's short-length reader, with a dword map length instead.
    static int __fastcall readString(TAbstractFile* infile, std::string& value);
    int readVictoryCondition(char type, TAbstractFile* infile);
    int readLossCondition(char type, TAbstractFile* infile);
    int saveVictoryCondition(char type, TAbstractFile* outfile);
    int saveLossCondition(char type, TAbstractFile* outfile);
    // Complete's saved-header reader carries the save version as a third
    // argument so pre-25 campaign hero ids can be remapped.
    int loadVictoryCondition(char type, TAbstractFile* infile,
                             int saveVersion);
    int loadLossCondition(char type, TAbstractFile* infile, int saveVersion);
    // Retail carries the save-version argument absent from the Dreamcast
    // declarator; the 0x4c5630 body returns with `ret 8`.
    int load(TAbstractFile* infile, int saveVersion);
    int get(const char* path, const char* filename, int saveVersion);
};
SIZE(NewSMapHeader, 0x304);

// Game.h owns the town-definition record shared by map loading and towns.
class TownExtra {
public:
    // Original: TownExtra::TownExtra; Game.h:377, dc 0xf4af0.
    // DC clears its 13-byte name array at +0x55. Complete's readTownData
    // (0x5019f0) constructs the replacement std::string and both spell
    // bitsets as members of this 0x88-byte record.
    TownExtra() {}
    int m_objRef;
    char m_playerOwner;
    char m_customBuildings;
    // Six bytes of stream reach each of these two; the record keeps eight.
    __int64 m_buildingBuiltMask;
    __int64 m_buildingDisabledMask;
    char m_hasFort;
    char m_customArmies;
    armyGroup m_townArmy;
    char m_customName;
    std::basic_string<char, std::char_traits<char>, std::allocator<char> > m_name;
    // A char in the Dreamcast record, a DWORD here: readTownData assigns it
    // from CObjectType::extra and from setup.alignment[], both int.
    int m_townType;
    char m_isGrouped;
    std::bitset<70> m_spells;
    std::bitset<70> m_fixedSpells;
};
SIZE(TownExtra, 0x88);

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
    TCreatureType m_type[4];  // +0x04  (DC 0x1CF0, 16 B)
    // +0x14. Four SHORTS, not two ints: the constructor's fused loop
    // walks `type` by 4 and this row by 2 over the same four
    // iterations.
    short m_population[4];
    armyGroup m_guards;  // +0x1c
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
    // Raw DC publics for load/save return bool (QAA_N); Complete ports the file argument.
    bool load(TAbstractFile* infile);
    // update_bonus's negative twin. Retail has no out-of-line row for it
    // (nothing fits between generator::save's end at 0x4b8791 and
    // update_bonus at 0x4b87a0), so it is inline-only - the same shape
    // set_owner below carries.
    inline void removeBonus();
    bool save(TAbstractFile* outfile);
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
    armyGroup m_guards;
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
    int save(TAbstractFile* outfile);
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
    int load(TAbstractFile* infile, int saveVersion);
};
SIZE(SGameSetupOptions, 0x1cc);

struct CampaignScenarioPreview : public NewSMapHeader {
    SGameSetupOptions m_gameSetup;
    bool m_available;
};
SIZE(CampaignScenarioPreview, 0x4d4);

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
    // Complete-only field: bool inferred from direct copies to/from the
    // DC-proven bool g_inCampaign. Load normalizes the on-disk short with
    // != 0 before storing this byte; Save retains the two-byte file format.
    bool m_campaignGame;
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
    int save(TAbstractFile* outfile);
    int load(TAbstractFile* infile);
};
SIZE(SavedGameHeader, 0x5a4);

struct TBlackMarket {
public:
    TArtifact m_artifacts[7];
};
SIZE(TBlackMarket, 0x1c);

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
    armyGroup m_garrisonArmy;
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
// REPACKING THE DREAMCAST ROSTER (NB11 member records, class
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
    int load(TAbstractFile* infile, int saveVersion);
    // 0x4bada0 (claimed in src/game.cpp). town::buy_building calls it
    // on gpGame->players[owner] to split the human and computer
    // resource paths.
    bool isHuman() const;
    int save(TAbstractFile* outfile);
    // 0x4b9fc0 (located in src/game.cpp, body not reconstructed).
    // townManager::SwapHeroes 0x5d5150 calls it on
    // gpGame->players[townToView->owner] with the town it is showing,
    // and branches on the byte result - the arity and the return type
    // both. Nested under this gate, not added to the class outright: a
    // bare member-function declarator on a class this widely included
    // is the include-set wall's own trigger shape (the townManager
    // precedent), and townmgr.cpp is the only live consumer.
    unsigned char addGarrisonHero(town* ourTown);
    int buildingsOwned(int townType, int buildingId, int mageLevel);
    bool hasMobileHero();
    int nextHero();
    int nextTown();
    int numOfGivenArtifact(int artifact) const;
    int findHero(int id) const;
    int findTown(int id) const;
    // ?IsHuman@playerData@@QBA_NXZ / ?IsLocalHuman@playerData@@QBA_NXZ
    bool isLocalHuman() const;
    char* getName();
    void setName(char* newName);
    void assignNetInfo(CNetPlayerInfo* netPlayerInfo);
    void getNetInfo(CNetPlayerInfo* netPlayerInfo);
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
extern game* g_game;
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
    struct TRumour {
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
    TArtifact m_marketArtifacts[7];
    // NH3API global.hpp confirms the PC vector at +0x1f680. Map loading
    // appends each black market and stores its index in the map cell.
    std::vector<TBlackMarket> m_blackMarkets;
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
    std::vector<town> m_towns;
    enum { HERO_COUNT = 156 };
    hero m_heroes[HERO_COUNT];
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
    std::vector<TRumour> m_rumours;  // +0x4e648
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
    int scan(signed char* whichList, int start, int length);
    int randomScan(signed char* whichList, int start, int length,
                   signed char scanValue);
    int getNewBoatId();  // 0x4bb170
    int createBoat(int x, int y, int z, int owner,
                   unsigned char remoteMove, signed char type);  // 0x4bb250
    int getNewHeroId(int playerPos, THeroClass excluded,
                     unsigned char preferAlignment,
                     THeroClass preferredClass);  // 0x4bb5e0
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
    TArtifact getRandomArtifactId(int artifactClass);  // 0x4c94d0
    void setupTowns();
    void checkHeroConsistency();
    int getRandomNumTroops(int whichMon);
    void setupDynamicStuff(int update, int forceUpdate);  // 0x51bd50
    void setupNewOverviewType(int whichType,
                              unsigned char update);  // 0x51e330
    int processIconSelect(int codeY, unsigned char rightMouse);  // 0x51ee50
    playerData* getLocalPlayer();
    int getLastHuman() const;
    int getLocalPlayerGamePos() const;  // 0x4cea20
    SpellID getRandomSpell(std::bitset<5> spellLevels);  // 0x4c95a0
    boat* getHeroBoat(int id, unsigned char occupied);  // 0x4ce900
    int getHeroId(type_point heroLocation);
    int getMineId(int x, int y, int z);
    int getGarrisonId(int x, int y, int z);
    int heroIdToHeroPos(playerData* player, int id);
    int townIdToTownPos(playerData* player, int id);
    int getTownId(int x, int y, int z);  // 0x4bb870
    int mineTypesOwned(int whichPlayer, int mineType);  // 0x4bae70
    int getGeneratorId(int x, int y, int z);  // 0x4bb900
    int getBoatsBuilt();  // 0x4cce30
    // 0x4baf00, its link-order neighbour. countOnly stops at the piece
    // count; otherwise the shared puzzlePiecesRemoved bitset is re-rolled.
    int setupPuzzlePieces(int whichPlayer, int countOnly);
    void giveArmy(armyGroup* thisMonInfo, int monType,
                  int monNum, int slot);  // 0x4ca340
    int experienceValueOfStack(const armyGroup* whichGroup,
                               const hero* whichHero);  // 0x4ca3b0
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
    static int __fastcall saveString(TAbstractFile* outfile, std::string& text);
    // DC game.cpp:2492/2531 and the class method records explicitly
    // declare static loadString/saveString with a string reference.
    // Retail passes stream in ECX and string address in EDX at
    // 0x4bb990/0x4bbb60. Static /Gr members have that same ABI.
    static int __fastcall loadString(TAbstractFile* infile, std::string& value);

private:
    int loadRumours(TAbstractFile* infile);  // 0x4bbe40
    int saveRumours(TAbstractFile* outfile);  // 0x4bbc20
    int loadSignPool(TAbstractFile* infile);  // 0x4b9070
    int saveSignPool(TAbstractFile* outfile);  // 0x4b9270

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
    void giveTimeEventReward(const TTimedEvent* thisEvent);  // 0x4cd710
    void giveTownEventReward(const TTownEvent& thisEvent);
    void checkForTownEvent();  // 0x4cda10
    bool isHuman(int gamePos) const;  // 0x4ce940

private:
    int loadMinePool(TAbstractFile* infile, int saveVersion);

public:
    void recordMonsterIdentifier(int identifier, type_point point);

private:
    int loadGarrisonPool(TAbstractFile* infile, int saveVersion);
    int loadTownPool(TAbstractFile* infile, int saveVersion);
    int loadPlayerData(TAbstractFile* infile, int saveVersion);
    int loadHeroPool(TAbstractFile* infile, int saveVersion);
    int savePlayerData(TAbstractFile* outfile);
    int saveHeroPool(TAbstractFile* outfile);

public:
    int loadGame(const char* filename, int isOrigData, int isQuickLoad);
    unsigned char saveGame(const char* filename,
                           unsigned char determineSuffix,
                           unsigned char campaignWinMode,
                           unsigned char compressIt,
                           unsigned char xferFile);

private:
    int load(TAbstractFile* infile);  // 0x4bcda0
    int loadBlackMarkets(TAbstractFile* infile);
    int saveBlackMarkets(TAbstractFile* outfile);
    int saveMinePool(TAbstractFile* outfile);  // 0x4b9580
    int saveGarrisonPool(TAbstractFile* outfile);  // 0x4b98c0
    int loadBoatPool(TAbstractFile* infile);  // 0x4b9a00
    int saveBoatPool(TAbstractFile* outfile);  // 0x4b9c40
    int loadObeliskPool(TAbstractFile* infile);
    int saveObeliskPool(TAbstractFile* outfile);
    int saveTownPool(TAbstractFile* outfile);

public:
    // 0x4bf780 (dc 0xaa7e0).
    void validateVictoryLossConditions(unsigned char checkMapLocations);
    void giveTroopsToNeutralTowns();
    void giveTroopsToNeutralTown(int townId);  // 0x4bf570
    void setupOrigData();
    void newMap(TAbstractFile* mapFile, int* playerHeroFaces,
                TCampaignBrief::ScenarioStruct* campaignContext, int gameVersion);
    unsigned char newMap(const char* mapPath, const char* mapName,
                         int* playerHeroFaces, int gameVersion);
    void setupFirstPlayer();
    bool loadMap(TAbstractFile* mapFile);
    void applyMapHeaderAvailability();
    void readMapHeroSetups(TAbstractFile* mapFile, int mapVersion);
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
    void setRecruits();
    void setWeeklyRecruits(int playerPos);
    void clearRecruits(int* recruits);
    void randomizeHeroPool();
    void replaceRecruit(int playerPos, long recruitSlot);
    bool growCoverOfDarkness();
    void initNewGame(int difficulty, int version,
                     NewSMapHeader* mapHeader, TAbstractFile* infile);
    void resetGame(int difficulty, int version, NewSMapHeader* mapHeader);

private:
    int save(TAbstractFile* outfile);  // 0x4be3f0
    void setMarketArtifacts();
    void setSummoningGenerators();
    void setupNewRumour();
    void setCannedRumour();
    void setMapRumour();
    void setSpecialRumour();

public:
    void clearEventRecords();
    void recordShowHero(hero* who, signed char player, type_point point,
                          unsigned char reset);  // 0x49cb20
    void processRandomObjects();  // 0x4c9dd0
    // The random-object pass and the monster roll it drives. Both bodies
    // are claimed in game.cpp.
    TCreatureType getRandomMonster(int minLevel, int maxLevel);  // 0x4c92c0
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
    void recordMove(hero* who, int direction,
                     type_point destination);  // 0x49cd50
    void recordPlayerDeath(char playerId);
    void recordTeleport(hero* who, type_point destination);  // 0x49cf50
    void showLuckInfo(hero* who, int dialogType);
    void showMoraleInfo(hero* who, int dialogType);
    void recordHideHero(hero* who, char newOwner,
                          unsigned char townGarrison);
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
    TCreatureType upgradedCreatureType(TCreatureType creature) const
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
    unsigned char saveRecordedEvents(TAbstractFile* outfile);
    // declarator (`?load_recorded_events@game@@AAA_NPAX@Z`, private,
    unsigned char loadRecordedEvents(TAbstractFile* infile, int version);
    // 0x4bcb30 (dc 0xa8144, E:\gamedcs\game.cpp:2975,
    // `?setup_shipyards@game@@AAAXXZ`). Clears all eight
    // players[i].shipyards and re-derives them by sweeping the map,
    // temporarily restoring any hero or boat obscuring a cell so the
    // shipyard underneath is visible. game::Load's tail is the caller
    // the Dreamcast xref graph records.
    void setupShipyards();

public:
    void viewArmy(armyGroup& group, int iarmy, const hero* thisHero,
                  const town* thisTown, int x, int y,
                  unsigned char showDismiss, unsigned char isQuickView);
    void overview();
    VA(0x004317d0, 0x26)  // hd-crossbuild + exact body/callers x15, dc 0x2eb0
    hero* getHero(int which)
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
    hero* getCurrHero()
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
    town* getTown(int townId)
    {
        if (townId == -1)
            return 0;
        return &m_towns[townId];
    }
    // Original: game::GetTown; Game.h:1022, dc 0x169c60.
    // The const overload indexes directly; the non-const overload above
    // separately handles the -1 sentinel.
    const town* getTown(int which) const { return &m_towns[which]; }
    town* getCurrTown()
    {
        if (g_currentPlayer->m_currTownId != -1)
            return &m_towns[g_currentPlayer->m_currTownId];
        return 0;
    }
    // Original: game::GetCurrTownId; game.h:1024, dc 0x1ff98
    int getCurrTownId() { return g_currentPlayer->m_currTownId; }
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
    // Original: game::GetMine; Game.h:1036, dc 0x9ca84.
    mine* getMine(int which) { return &m_mines[which]; }
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
    inline const std::vector<type_point>& getLiths(long color) const;
    inline const std::vector<type_point>& getLithExits(long color) const;
    inline const std::vector<type_point>& getWhirlpools() const;
    NewmapCell* getCell(type_point point);
    void getLossConditionText(char* text);
    void getVictoryConditionText(char* text);
};

// The five .def-name tables game::ConvertObject (0x4c9990) rewrites a
// converted object's CObjectType::ImageName from. Their sole reader in
// the whole image is that body (config/retail/reloc-evidence.tsv rows
// 0xc9a33 / 0xc9a42 / 0xc9b04 / 0xc9b50 / 0xc9b63), so they are declared
// on game.obj's own gate. Contents read from the hash-verified image:
// the resource row is avtwood0/avtmerc0/avtore0/avtsulf0/avtcrys0/
// avtgems0/avtgold0.def in the seven-resource order the tree already
// uses, and the three town rows are AVCcast0..AVChfor0 (village),
// AVCcasx0..AVChforx (fort) and AVCcasz0..AVChforz (capitol), nine
// entries each in TTownType order. Names are house placeholders - no DC
// roster row covers any of the five.
extern const char* g_resourceObjectDefs[NUM_RESOURCES];
extern const char* g_artifactObjectDefFormat;
extern const char* g_townVillageObjectDefs[9];
extern const char* g_townFortObjectDefs[9];
// Calendar-state globals saved across advManager::LoadRemote. Dreamcast
// supplies the names; retail fixes these four dword cells and their paired
// reset/restore use around game::LoadGame.
extern const char* g_townCapitolObjectDefs[9];
extern int g_weekType;
extern int g_weekTypeExtra;
extern int g_monthType;
// Shared UI text table: attack, defense, spell power, and knowledge.
extern int g_monthTypeExtra;
// The map's live width and height, Dreamcast-named (`?MAP_WIDTH@@3HA` /
// `?MAP_HEIGHT@@3HA` in kb.obj's PlayerDead scan) and initialised to 72 -
// a Medium map - in retail's .data.  96 retail bodies reference the pair,
// so it belongs in this header rather than any one consumer's; kb.obj's
// PlayerDead is the byte-proven reader here, walking y over MAP_HEIGHT and
// x over MAP_WIDTH while indexing worldMap by its own Size.
extern int g_mapWidth;
extern int g_mapHeight;
extern int g_mineProduction[7];
// Six weighted neutral-town dwelling levels, byte-proven as
// {2,3,4,5,4,3} by game::GiveTroopsToNeutralTown.
extern double g_productionHandicap[];
extern const int g_neutralTownLevelWeights[6];
// NewMap's seven-resource rows, indexed by setup.difficulty.  The first
// address is also the seven-int tutorial row immediately following the
// neutral-town weights above.
extern const int g_initResourcesHuman[][NUM_RESOURCES];
// NewMap reads one dword per player here before narrowing the selected value
// into setup.startingBonus.  The other known readers do not yet prove a
// broader semantic name, so keep the address-bearing role provisional.
extern const int g_initResourcesComputer[][NUM_RESOURCES];
// SetupFirstPlayer writes its first-human scan result here alongside
// gNetLocalGamePos.  StartLocalPlayerTurn later consumes the same cell;
// no surviving symbol attests a semantic spelling.
extern int g_newMapStartingBonus[8];
// remote.obj owns the DATA claim. NextPlayer consumes the adjacent recovery
// latch while retrying a failed turn-state transfer.
extern int g_playerTurn;
extern unsigned char g_playerDrop;
// advmgr.cpp owns the retail datum; ResetGame only clears the turn-control
// latch after rebuilding the session.
extern int g_thisNetGotAdventureControl;
extern int g_heroGoldCost;
// One-byte session latch reset by game::SetupOrigData. No surviving symbol
// names its wider role, so retain the address-ordinal spelling.
extern int g_grailOwner;
// Eight ints indexed by PLAYER, and readHeroData (0x5021c0) CONSUMES an
// entry: `movsx eax,[owner] / mov ecx,[4*eax + 0x69fb24]`, and when that is
// not -1 it becomes the hero id and the slot is stored -1 again. A reserved
// id therefore wins over GetStartingHeroId, exactly once. game::Init
// (0x4bf1a0) is what fills the array with -1 to begin with
// (`mov edi,0x69fb24 / mov ecx,8 / or eax,-1 / rep stosd`), so game.obj owns
// the definition; this is the shared declaration.

// No Dreamcast or NH3API symbol covers it, so the spelling stays ordinal on
// gUnnamed69950c's precedent rather than inventing a role name.
extern unsigned char g_normalVictory;
extern int g_startingHeroOverrides[8];
// Dreamcast public `iCurHourGlassPhase`; game.cpp owns the retail word and
// philAI::DoAI advances it as computer heroes are processed.
extern int g_curHourGlassPhase;
// Retail-only companion word cleared beside the hourglass phase by
// philAI::GetTurnAIVars. It has no surviving source symbol or other reader.
extern int g_sandAnim;
// Retail .bss 0x69ccc4, and the SIBLING of advmgr.h's gMapVisibilityBit
// (0x69ccbc) rather than an alias of it - it has 38 relocation sites of
// its own, and advManager::ProcessHover gates fog on it with the same
// `test byte ptr [...], al` shape. game::Load's tail writes the pair one
// after the other and that is what separates them: 0x69ccbc takes
// `1 << gUnnamed69778c` (the acting player) while this one takes
// `1 << gNetLocalGamePos` (this machine's own seat). NAME UNATTESTED -
// address-ordinal placeholder, as gUnnamed69778c is.
extern unsigned char g_curPlayerBit;
// Network-session latch; canonical storage is owned by kbwin.cpp.
extern int g_remoteOn;
// E:\gamedcs\philai.cpp:4126, `?AI_examine_map@@YAXXZ`); declared here
void __cdecl aiExamineMap();
// hero.cpp owns the DATA claim on 0x698400 (name unattested,
// address-ordinal placeholder) and game.obj is a second reader, so this
// is an extern-only declaration - the MAP_WIDTH/gpCurrentPlayer pattern.
// hero.cpp's note already records THIS call site: every reader treats
// nonzero as "suppress the interactive path", and game::ClaimTown skips
// its notify call.
extern int g_inSetup;

// --- the local-player pair, read by GetLocalPlayer and
// GetLocalPlayerGamePos (both in this TU). The mode selector they
// branch on is iMPNetProtocol, in netgame.h.
// 0x69cca8: the hot-seat game position of this machine's player - the
// dword eight bytes ahead of gpCurrentPlayer, and range-checked
// against [0,8) before use. Ordinal placeholder.
extern int g_netLocalGamePos;                // .bss 0x69cca8
extern unsigned char g_curPlayerBit;

void startAITheme();
// 0x699554: the same answer for every other protocol, handed back
// unchecked. Ordinal placeholder.
extern int g_localGamePos;                   // .bss 0x699554
// 0x6a7df8: eight char* colour names; playerData::GetName copies
// gPlayerColorNames[color] over an empty/default name. Defined by a TU
// not yet located - extern only (the bitNumber pattern).
           // .bss 0x6a7df8
// SetSpecialRumour shares the nine-way direction table with seer-hut quest
// descriptions, and indexes the terrain-name table by a Grail cell's ground
// set when producing the alternative location hint.

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
    if (g_inCampaign)
        strcpy(m_id, "H3SVC");
    else
        strcpy(m_id, "H3SVG");

    m_version = 42;
    m_gameVersion = g_game->m_f1f698;

    m_campaign = g_game->m_campaign;

    m_mapHeader = g_game->m_mapHeader;

    m_currentPlayer = g_netLocalGamePos;
    m_mapSetup = g_game->m_setup;
    m_campaignGame = g_inCampaign;
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
inline int SavedGameHeader::save(TAbstractFile* outfile)
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
// Dreamcast uses gzread directly and records the checked ID-read count.
// The six unchecked scalar reads use returned values rather than artificial
// caller scopes. VC6 then matches all 62 retail blocks and 26 named calls,
// including the shared failure cleanup; flattening those reads loses it.
// E:\gamedcs\Game.h:1344, dc 0xbcfe4
VA(0x004bc750, 0x3D5)  // dc 0xbcfe4
inline int SavedGameHeader::load(TAbstractFile* infile)
{
    std::string openedName;
    unsigned char inputWasProvided = infile != 0;
    std::auto_ptr<TAbstractFile> ownedInput;
    int count;

    if (!inputWasProvided) {
        openedName = g_game->m_setup.m_filename;
        _chdir("games");
        try {
            infile = new TGzFile(openedName.c_str(), "rb");
            ownedInput = std::auto_ptr<TAbstractFile>(infile);
        }
        catch (TGzFile::TOpenFailure) {
            return -1;
        }
        _chdir("..");
        if (!infile)
            return -1;
    }

    count = infile->read(m_id, sizeof(m_id));
    if (count < sizeof(m_id))
        return -1;

    m_version = readValue<int>(infile);
    if (m_version > 42)
        return -1;

    if (m_version >= 40) {
        m_gameVersion = readValue<int>(infile);
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
        g_gameContext == VIDEO_GAME_STATE_FORCED_BINK_LOW)
        return -1;

    char compatibilityBuffer[32];
    infile->read(compatibilityBuffer, sizeof(compatibilityBuffer));
    if (m_mapHeader.load(infile, m_version) < 0)
        return -1;
    if (m_mapSetup.load(infile, m_version) < 0)
        return -1;

    m_campaignGame = readValue<short>(infile) != 0;
    if (m_campaignGame)
        m_campaign.load(infile, m_version);

    char fileNameBuffer[0x15f];
    infile->read(fileNameBuffer, sizeof(fileNameBuffer));
    m_fileName = fileNameBuffer;

    m_difficultyRating = readValue<short>(infile);
    m_numDeadPlayers = readValue<char>(infile);
    infile->read(m_deadPlayer, sizeof(m_deadPlayer));
    infile->read(m_humanPlayer, sizeof(m_humanPlayer));
    m_currentPlayer = readValue<int>(infile);

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

// Original: game::get_liths; Game.h:1395, dc 0x12ca94.
inline const std::vector<type_point>& game::getLiths(long color) const
{
    return m_lithPools[color];
}

// Original: game::get_lith_exits; Game.h:1400, dc 0x12caa8.
inline const std::vector<type_point>& game::getLithExits(long color) const
{
    return m_lithExitPools[color];
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

// --- type_creature_bank ---

// Dreamcast Game.h proves the complete 200-byte class and its single char
// array. Retail's adventure/combat cheat handlers inline the string-taking
// constructor and compare members while sharing the encoder at 0x402a30.
// The default constructor and GetCode are ordinary declarations in DC type
// 0x3dc2 (method types 0x3dc5/0x3dcb), without procedure/source locations.
// Neither has an active caller; leave their bodies unreconstructed.
class TCheatCode {
public:
    TCheatCode();
    TCheatCode(const char* value) { encode(value); }
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
SIZE(TCheatCode, 200);

// E:\gamedcs\Game.h:1439
VA(0x00402a30, 0xA1)
inline void TCheatCode::encode(const char* value)
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
