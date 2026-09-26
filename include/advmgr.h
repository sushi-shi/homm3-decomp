#ifndef HOMM3_ADVMGR_H
#define HOMM3_ADVMGR_H

#include "basemgr.h"
#include "herospec.h"
#include "kb.h"
#include "mapcell.h"
#include "primaryskill.h"
#include "secondaryskill.h"
#include "sskilltraits.h"
#include "struct.h"
#include "town.h"
#include "window.h"

class BlackBoxData;
class CNetMsgHandler;
class CChatEdit;
class resource;
class sample;
class ds_memsample;
class TreasureData;
struct ExtraInfoUnion;
struct type_creature_bank;
struct type_university;
class armyGroup;

// adventuremapwindow.obj's shared rollover/right-click text table. Dreamcast
// supplies the name and THelpText row type; Complete fixes its 0x6a56e0 base
// through overview and town-screen readers of both columns.
extern THelpText g_adventureWindowHelp[];

struct type_creature_bank;
struct type_university;

// Dreamcast CodeView supplies the domain and ordering. Retail indexes the
// matching 32-byte per-player flag band and the parallel help-name table with
// this value; the shrine helper currently needs only the type, not individual
// enumerator spellings.

// events.obj wants the same domain for game::SetInfoFlag's callers, so the
// gate is widened by exactly that one view rather than opened outright -
// the enum stays invisible to every TU that has no consumer.
enum GlobalInfoFlags {
    BuoyInfo = 0,
    CloverFieldInfo,
    FaerieRingInfo,
    FountainOfFortuneInfo,
    GardenOfRevelationInfo,
    TrainingGroundsInfo,
    LibraryInfo,
    DefenseTowerInfo,
    MercCampInfo,
    MagicSchoolInfo,
    WarSchoolInfo,
    PowerSchoolInfo,
    WitchHutInfo,
    FountainOfYouthInfo,
    HillFortInfo,
    MagicSpringInfo,
    MermaidInfo,
    RallyFlagInfo,
    TreeOfKnowledgeInfo,
    Shrine1Info,
    Shrine2Info,
    Shrine3Info,
    IdolOfFortuneInfo,
    TempleInfo,
    UniversityInfo,
    MagicWellInfo,
    OasisInfo,
    WateringHoleInfo,
    const_sacrifice_info,
    MaxInfoFlags = 32
};

enum WiseTreePrices {
    const_tree_wants_nothing = 0,
    const_tree_wants_gold,
    const_tree_wants_gems,
    const_tree_price_count
};

enum WitchHutSkillEncoding {
    WitchHutNoSkillMask = 0x000fe000
};

// Retail SetRolloverText dispatches id 215 through NewfullMap's five-byte
// quest-guard pool. The name is corroborated by the admitted retail structure
// evidence; the Dreamcast adventure-object enum predates this object.

// The four ids added 2026-08-20 are the rest of that same post-Dreamcast
// block, and every one of them is byte-proven by readObject's (0x502e00)
// jump table - its five highest arms are exactly 214..218, in that order:
//   214  reads an owner byte and a hero id, widens the 0xff sentinel to -1,
//        and only then reads a power-rating byte, appending the record to
//        NewfullMap's +0xa0 pool.  That is a hero PLACEHOLDER, and
//        PlaceObject's skip list agrees - 214 is skipped beside HERO,
//        RANDOM_HERO, BOAT and HOLY_GRAIL, the classes something else
//        places.  (mapcell.h's events-only view spells 212 with this name;
//        212 is BORDER_GATE above and that view's spelling is contradicted
//        by these bytes.  Left standing there - no byte in this lane
//        reaches it.)
//   216  reads owner, a castle id, and - only when that id is zero - a
//        faction mask, then a min and a max level.
//   217  the same record with both levels forced to the object type's own
//        `extra` field, read as a byte.
//   218  the same record with the castle id zeroed and the faction mask
//        set to `1 << extra`, the levels still coming off the stream.
// All three of the 216..218 arms append to the SAME 16-byte-element pool at
// NewfullMap+0xc0 that the retail-only resolution pass at 0x502b60 walks.
// The three cartographer variants, indexed by NewmapCell::objectIndex.
// DispatchEvent's cartographer arm shows one map plane per value, matching
// the three-wide cartographerMask/cartographerFlags arrays.
enum ECartographerType {
    CARTOGRAPHER_WATER = 0,
    CARTOGRAPHER_LAND = 1,
    CARTOGRAPHER_UNDERGROUND = 2
};

enum EAdvmgrRetailObjectType {
    BORDER_GATE = 212,
    // 213: DispatchEvent's 0xd5 arm gates DoFreelancersGuild(hero*) on
    // human_player - the map-object entry of tradpost's guild pair.
    FREELANCERS_GUILD = 213,
    HERO_PLACEHOLDER = 214,
    QUEST_GUARD = 215,
    RANDOM_DWELLING = 216,
    RANDOM_DWELLING_LVL = 217,
    RANDOM_DWELLING_FACTION = 218
};

// events.obj joins the gate for the refugee camp (0x4a4600), which names
// the object it is standing on in both of its dialogs. The guard is SPLIT
// around this one declarator rather than moved, so the preprocessed text
// every quick-info consumer sees is unchanged, line for line.
extern const char* g_terrainNames[];
extern int g_curWatchPlayer;
// Paired with gUnnamed6989c8 by every non-local adventure command gate.
// The role is byte-proven; no surviving symbol attests a semantic name.
extern int g_debugLevel;
// Written at startup by InitializeExtraInfoText (28 rows of
// xtrainfo.txt), so the ELEMENT is not const.
extern int g_aiHeroMoveActive;
extern const char* g_globalInfoFlagNames[];
// Role-derived retail tables used by SetRolloverText. The generator-name
// semantics are corroborated by the DC public roster; the x86 bases and
// owner-color consumer role are fixed directly by the retail switch blocks.
extern const char* g_borderColorNames[];
// Both written at startup by InitializeCreatureGeneratorNames (80 rows of
// crgen1.txt and 2 of crgen4.txt), so the ELEMENT is not const.
extern const char* g_creatureGenerator1RolloverNames[];
// InitializeMineNames copies the eight lines of minename.txt here. The mine
// help-text helper indexes 0..6 by mine type and uses row 7 for an abandoned
// mine, independently fixing both the extent and the table's consumer role.
extern const char* g_creatureGenerator4RolloverNames[];
// events.obj joins the gate for the resource pile (0x4a4be0), which
// strcpy's the resource's own name out of this table and lower-cases its
// first letter before formatting the pickup line. The guard is SPLIT
// around the one declarator rather than moved, so the preprocessed text
// every quick-info consumer sees is unchanged, line for line.
extern const char* g_mineDescriptions[8];
extern const char* g_resourceNames[8];
// The two mine tables advManager::DoEventMine (0x4a39a0) reads, both
// text.obj/game-side globals declared here because this is where the
// adventure-object tables of the events TU already live.

// 0x678288 is the per-mine-type daily yield and its CONTENTS are the
// proof: 2, 1, 2, 1, 1, 1, 1000 - wood, mercury, ore, sulfur, crystal,
// gems, gold, HoMM3's published mine rates in resource order. The
// Dreamcast publishes `?gMineCharacteristics@@3PAHA` and the shape
// agrees exactly.
// 0x6a5e20 is the per-mine-type capture line, indexed by the same type
// and handed straight to NormalDialog as its text. It has exactly ONE
// code consumer image-wide. The Dreamcast publishes TWO char** mine
// tables - `?gMineEventText@@3PAPBDA` and `?gMineDescriptions@@3PAPBDA` -
// and only the ROLE separates them: this one is an event dialog's text,
// so it takes the event name. PROVISIONAL on that ground alone.
extern const int g_mineCharacteristics[7];
// Route-arrow frame selector, retail .data 0x6782ac: sixty-four signed
// bytes read as [previous step direction][current step direction], both
// in the eight-way order gStepDeltaX/gStepDeltaY use. ShowRoute adds 2 to
// the result, clearing the two non-directional frames. The orientation is
// fixed by the matrix itself - the straight-through diagonal runs
// 8,9,..,15, i.e. 8+dir - and the extent is exact, since 0x6782ec begins
// unrelated float data.
extern const char* g_mineEventText[];
extern const signed char g_routeArrowFrames[8][8];

// advManager::advCommand's domain. The Dreamcast prints the member as a
// plain T_INT4 (classes.csv list[171], offset 84) and no surviving symbol
// names any of its values, so these are ORDINAL PLACEHOLDERS carrying
// only the role each value's DoAdvCommand arm proves, corroborated by the
// ProcessHover/ProcessMapSelect assignments already in this tree. The
// member itself stays `int` - this enum exists so the dispatch labels and
// the one `advCommand != WALK_ROUTE` compare are named rather than magic.
// Gated to advmgr.obj's own view for the standing include-set reason.
enum EAdvCommand {
    ADV_COMMAND_NONE = -1,
    // Retargets the current hero's path at lastMapHover, then FALLS
    // THROUGH into WALK_ROUTE - the fallthrough is retail's own, proved
    // by the jump table's 0x57 entry landing 0x43 bytes above 0x9a.
    ADV_COMMAND_MOVE_HERO = 1,
    ADV_COMMAND_VIEW_HERO = 2,
    ADV_COMMAND_VIEW_TOWN = 3,
    ADV_COMMAND_SELECT_HERO = 4,
    ADV_COMMAND_SELECT_TOWN = 5,
    // The hero is standing on the town it obscures; the arm views that
    // town through type_obscuring_object::get_obscured_town.
    ADV_COMMAND_VIEW_OBSCURED_TOWN = 6,
    ADV_COMMAND_WALK_ROUTE = 7,
    ADV_COMMAND_SHIPYARD = 8
};

// Shared adventure state. Original names and retail evidence live beside
// the owning definitions; configuration fields are accessed through g_config.
extern ds_memsample* g_walkSample;
extern sample* g_newWalkSample;
// Retail-only byte cleared after animateMove; original name and role unknown.
extern unsigned char g_unnamed6968e8;
extern int g_heroMoveTriggeredEvent;
extern int g_lowMemory;

DATA(0x006985c0) extern int g_overviewReturnAction;
DATA(0x0069873c) extern int g_overviewReturnActionExtra;

DATA(0x00682a38) extern unsigned char g_followPlayerMode;
extern int g_drawingPuzzle;
extern int g_blackoutPlayer;
extern unsigned char g_goSolo;

extern int g_soloPos;

extern int g_lastCheaterState;
extern int g_lastDebugState;

extern unsigned long g_forceSwitchMusic;


// Retail .bss 0x69928c and the manager that lives there. InitMainClasses
// (0x4edb40) allocates it LAST, immediately after gpSearchArray, and
// Complete InitMainClasses calls philAI::philAI (0x524360) and stores
// the result at 0x69928c; Main calls philAI::doAI (0x525e80). This is
// the canonical philAI class, formerly declared as CAITurnDriver69928c.
class philAI;
extern philAI* g_philAI;

// smackmgr.obj's video-pump bracket (0x5977a0 / 0x597850), the pair
// ProcessKeyPress's ESC arm puts around its exit confirm. Declared here
// rather than by including smackmgr.h: advmgr.obj needs exactly these two
// declarators out of that header, and this tree's include-set sensitivity
// makes widening a compiland's closure a measured cost, not a free one.
void videoPause();
void videoResume();

// Retail .bss 0x6972b8, kb.cpp's game-over latch, also published by
// kb.h as gbGameOver; advManager::Main tests it twice - once on entry
// and once after the dispatch - and turns a set latch into the
// executive's terminate-loop message.
DATA(0x006972b8) extern int g_gameOver;

// giOverviewReturnAction's domain. ONE value is byte-proven - the kingdom-
// overview arm answers 2 by viewing giOverviewReturnActionExtra's town and suppressing the
// screen fade - so the label is an ORDINAL PLACEHOLDER carrying only that
// role. This enum exists so the compare is named rather than magic.
enum EOverviewExit {
    OVERVIEW_EXIT_TOWN = 2
};

// Retail .data 0x68c6b8, the view-world tile scale. DECLARATION ONLY - no
// DATA claim, because viewwrld.obj owns the definition (its own writers,
// plus the fsub at 0x5f73b0 and the fdiv at 0x5fc274, are what prove the
// slot is a FLOAT and not the int at 0x68c6bc it is paired with). The only
// three values UpdateRadar tests against are 16.0f, 11.84f and 7.68f -
// 0x41800000, 0x413d70a4 and 0x40f5c28f exactly. No attested name.
extern float g_viewWorldScaleFloat;

// The four square map dimensions MAP_WIDTH/MAP_HEIGHT take, named so
// UpdateRadar's three `switch (MAP_HEIGHT)` bodies case on a domain rather
// than on literals. The values are retail's own switch labels, decoded out
// of the two 109-byte index tables at 0x4135e4 and 0x413714; the names are
// HoMM3's published map sizes. advmgr.h already carried 144 as
// ADVENTURE_XLARGE_MAP_WIDTH inside advManager's sound-extent enum - that
// enumerator is left alone, this is the domain's own home.
enum EMapDimension {
    MAP_DIMENSION_SMALL = 36,
    MAP_DIMENSION_MEDIUM = 72,
    MAP_DIMENSION_LARGE = 108,
    MAP_DIMENSION_EXTRA_LARGE = 144
};

// The only three values UpdateRadar tests gUnnamed68c6b8 against. Retail
// compares the float's BIT PATTERN with integer `cmp` immediates
// (0x41800000 / 0x413d70a4 / 0x40f5c28f) - VC6 folding an exact float
// equality against a normal constant - and our CL folds a literal the same
// way. MACROS, not `const float`: the const-object spelling was MEASURED
// and it does NOT fold, VC6 emitting a memory load per compare and taking
// UpdateRadar 90.18 -> 85.42. A float domain cannot be an enum, so this is
// the only naming that clears the unnamed-domain-compare floor at zero
// cost.
#define VIEW_WORLD_TILE_SCALE_FULL 16.0f
#define VIEW_WORLD_TILE_SCALE_MID 11.84f
#define VIEW_WORLD_TILE_SCALE_FAR 7.68f


// Retail GetSoundId returns this four-byte enum. The semantic aliases have
// not been admitted; these ordinal names expose only the values proved by
// that function's return blocks.
enum e_looping_sound_id {
    LOOPING_SOUND_INVALID = -1,
    LOOPING_SOUND_0 = 0,
    LOOPING_SOUND_1,
    LOOPING_SOUND_2,
    LOOPING_SOUND_3,
    LOOPING_SOUND_4,
    LOOPING_SOUND_5,
    LOOPING_SOUND_6,
    LOOPING_SOUND_7,
    LOOPING_SOUND_8,
    LOOPING_SOUND_9,
    LOOPING_SOUND_10,
    LOOPING_SOUND_11,
    LOOPING_SOUND_12,
    LOOPING_SOUND_13,
    LOOPING_SOUND_14,
    LOOPING_SOUND_15,
    LOOPING_SOUND_16,
    LOOPING_SOUND_17,
    LOOPING_SOUND_18,
    LOOPING_SOUND_19,
    LOOPING_SOUND_20,
    LOOPING_SOUND_21,
    LOOPING_SOUND_22,
    LOOPING_SOUND_23,
    LOOPING_SOUND_24,
    LOOPING_SOUND_25,
    LOOPING_SOUND_26,
    LOOPING_SOUND_27,
    LOOPING_SOUND_28,
    LOOPING_SOUND_29,
    LOOPING_SOUND_30,
    LOOPING_SOUND_31,
    LOOPING_SOUND_32,
    LOOPING_SOUND_33,
    LOOPING_SOUND_34,
    LOOPING_SOUND_35,
    LOOPING_SOUND_36,
    LOOPING_SOUND_37,
    LOOPING_SOUND_38,
    LOOPING_SOUND_39,
    LOOPING_SOUND_40,
    LOOPING_SOUND_41,
    LOOPING_SOUND_42,
    LOOPING_SOUND_43,
    LOOPING_SOUND_44,
    LOOPING_SOUND_45,
    LOOPING_SOUND_46,
    LOOPING_SOUND_47,
    LOOPING_SOUND_48,
    LOOPING_SOUND_49,
    LOOPING_SOUND_50,
    LOOPING_SOUND_51,
    LOOPING_SOUND_52,
    LOOPING_SOUND_53,
    LOOPING_SOUND_54,
    LOOPING_SOUND_55,
    LOOPING_SOUND_56,
    LOOPING_SOUND_57,
    LOOPING_SOUND_58,
    LOOPING_SOUND_59,
    LOOPING_SOUND_60,
    LOOPING_SOUND_61,
    LOOPING_SOUND_62,
    LOOPING_SOUND_63,
    LOOPING_SOUND_64,
    LOOPING_SOUND_65,
    LOOPING_SOUND_66,
    LOOPING_SOUND_67,
    LOOPING_SOUND_68,
    LOOPING_SOUND_69,
    LOOPING_SOUND_COUNT
};

// Dreamcast CodeView publishes this complete eight-byte record and both
// member names. Retail TrimLoopingSounds independently proves the four-entry
// array, stride, and soundId field.
struct soundNode {
public:
    e_looping_sound_id m_soundId;
    int m_priority;
};
SIZE(soundNode, 8);

// Dreamcast CodeView publishes this complete cursor-frame domain. Retail's
// ProcessHover independently corroborates every value it uses directly.
enum type_adventure_cursor {
    ADV_ARROW_POINTER = 0,
    ADV_WAIT_POINTER = 1,
    ADV_HERO_INFO_POINTER = 2,
    ADV_TOWN_INFO_POINTER = 3,
    ADV_WALK_POINTER = 4,
    ADV_SWORD_POINTER = 5,
    ADV_BOAT_POINTER = 6,
    ADV_ANCHOR_POINTER = 7,
    ADV_EXCHANGE_POINTER = 8,
    ADV_EVENT_POINTER = 9,
    ADV_MULTI_TURN_OFFSET = 6,
    ADV_BOAT_EVENT_POINTER = 28,
    ADV_SCROLL_POINTER = 32,
    ADV_SCROLL_NORTH = 32,
    ADV_SCROLL_NORTHEAST = 33,
    ADV_SCROLL_EAST = 34,
    ADV_SCROLL_SOUTHEAST = 35,
    ADV_SCROLL_SOUTH = 36,
    ADV_SCROLL_SOUTHWEST = 37,
    ADV_SCROLL_WEST = 38,
    ADV_SCROLL_NORTHWEST = 39,
    ADV_HIGHLIGHTED_POINTER = 40,
    ADV_DIMENSION_DOOR_POINTER = 41,
    ADV_SKUTTLE_BOAT_POINTER = 42
};

// GetMapExtra's ninth bit records a monster occupying the map square. The
// role is independently described by the cross-build map inventory and is
// used by retail's normal-cursor decision before inspecting the cell below.
enum EMapExtraFlag {
    MAP_EXTRA_MONSTER = 0x100
};

class textEntryWidget;

class NewfullMap;
class NewmapCell;
class CSprite;
class textWidget;

// Only byte +1 of this 16-byte row is named by behavior, so keep the
// otherwise unknown table raw instead of inventing a partial object type.

// Retail's public .data symbol at 0x65f694. The relocation and final byte
// load in GetCloudLookup prove a 256-entry lookup indexed by the eight
// surrounding-cell bits.
extern unsigned char g_cloudType[256];

// Retail .bss 0x69ccbc. GetCloudLookup tests this byte against the low byte
// of every GetMapExtra result. Its role is proved by those xrefs; no public
// retail name survives, so the spelling remains provisional.

extern unsigned char g_mapVisibilityBit;

// DC publishes this as `int gbInViewWorld`; retail corroborates the role:
// its xrefs gate CompleteDraw's normal layers, ScanForHeroOrBoat, ViewPuzzle,
// and the separate view-world renderer.
extern int g_drawingPuzzle;

// Retail .bss 0x699538. CompleteDraw forces the source origin to (0, 0)
// while this is set, and DrawShroud uses it to bypass normal fog bounds and
// visibility tests. No public retail spelling survives.
extern int g_completeDrawAllCells;

// Retail-only CompleteDraw gates. Their roles and widths are proved by the
// entry predicate at 0x40f3f0; spellings remain provisional.
// Dreamcast names this shared cursor-suppression gate. DrawCursorAlpha's
// second entry predicate and philai's sole writer prove the retail cell.
extern int g_completeDrawEnabled;
DATA(0x006983f8) extern int g_specialHideCursor;

// A .data byte advManager::EraseAndFizzle (0x49e170) saves, CLEARS for the
// duration of the erase, and restores on every one of its three exits -
// the same save/clear/restore bracket it puts around animCtrPaused. 86 of
// its 87 image-wide references sit inside events.obj's link bracket, but
// the 87th does not, so this is a DECLARATION ONLY: no DATA claim is
// taken here and the owning TU keeps it (the winmgr.h gbInDialog
// precedent). Name is the house ordinal placeholder.
extern unsigned char g_colorCyclingEnabled;
extern unsigned char g_completeDrawMessageBypass;

// Six of these records are filled by ScanForHeroOrBoat. Retail writes the
// fields at +0/+4/+8/+c with a 0x10 stride; the names and bool type are the
// surviving CodeView signature/layout evidence.
struct TDrawParts {
public:
    bool m_isValid;
    int m_x;
    int m_y;
    int m_id;
    TDrawParts() : m_isValid(false) {}
};
SIZE(TDrawParts, 0x10);

// The adventure screen's own window. Only the two methods retail bodies
// outside adventuremapwindow.obj call on it are declared here:
// town::Deallocate (0x5be2d0) ends with
// `mov ecx,[gpAdvManager+0x44]` / push 1 / push 1 / push 0 /
// `call 0x403420`, so +0x44 holds a pointer to the class that owns
// 0x403420 and that entry takes THREE stack arguments.

// Arity note: the adventuremapwindow.cpp claim file pairs 0x403420
// with the DC's `TAdventureMapWindow::UpdateTownLocators()`, which the
// DC build carries as a FOUR-byte stub taking no arguments. The three
// pushed arguments refute that spelling; the signature that fits is the
// DC's `TAdvMenu::UpdateTownLocators(int, unsigned char, unsigned
// char)` (dc 0x2668, 374 B vs retail's 305 - ratio 0.82, and its four
// band neighbours ratio 0.81-0.92 against the same TAdvMenu roster).
// On the DC the menu bar was split out of the adventure window; on this
// image the bodies still sit in adventuremapwindow.obj's own band, so
// the CLASS stays TAdventureMapWindow and only the parameter list comes
// from the DC's TAdvMenu twin.
// hero::UseSpell independently reaches 0x403560 with the same three-argument
// shape; that neighbouring row is the hero-locator counterpart.

// It derives heroWindow because executive::CallManager calls
// `SleepAllWidgets` on this very +0x44 pointer and retail's reloc
// resolves to `?SleepAllWidgets@heroWindow@@QAEXE@Z` - the base body,
// not an override.
class TAdventureMapWindow : public heroWindow {
public:
    // Dreamcast TAdventureMapWindow::EWidgetIDs, complete. Retail uses the
    // same ids in the window's broadcast-message helpers.
    enum EWidgetIDs {
        MAP_ID = 0,
        RADAR_ID = 1,
        SELECTION_WINDOW_ID = 2,
        KINGDOM_OVERVIEW_ID = 3,
        ELEVATION_TOGGLE_ID = 4,
        QUEST_LOG_ID = 5,
        SLEEP_ID = 6,
        MOVE_ID = 7,
        CAST_SPELL_ID = 8,
        ADVENTURE_OPTIONS_ID = 9,
        SYSTEM_OPTIONS_ID = 10,
        NEXT_HERO_ID = 11,
        END_TURN_ID = 12,
        HERO_UP_ID = 13,
        HERO_DOWN_ID = 14,
        HERO_0_ID = 15,
        HERO_1_ID = 16,
        HERO_2_ID = 17,
        HERO_3_ID = 18,
        HERO_4_ID = 19,
        HERO_MOVEMENT_0_ID = 20,
        HERO_MOVEMENT_1_ID = 21,
        HERO_MOVEMENT_2_ID = 22,
        HERO_MOVEMENT_3_ID = 23,
        HERO_MOVEMENT_4_ID = 24,
        HERO_MANA_0_ID = 25,
        HERO_MANA_1_ID = 26,
        HERO_MANA_2_ID = 27,
        HERO_MANA_3_ID = 28,
        HERO_MANA_4_ID = 29,
        TOWN_UP_ID = 30,
        TOWN_DOWN_ID = 31,
        TOWN_0_ID = 32,
        TOWN_1_ID = 33,
        TOWN_2_ID = 34,
        TOWN_3_ID = 35,
        TOWN_4_ID = 36,
        CHAT_TEXT_ID = 37,
        CHAT_EDIT_ID = 38,
        // The second hero row, a Complete-era addition the Dreamcast enum
        // (which stops at CHAT_EDIT_ID) never saw, so the spelling is
        // PROVISIONAL and the role is what the bytes prove. Three retail
        // dispatchers agree on the block: advManager::ProcessSelect's
        // byte-index table sends 15..19 AND 39..43 to one arm that
        // normalises the id with `id - 15` below 39 and `id - 39` at or
        // above it and then indexes topHero + slot, and
        // TAdventureMapWindow::ProcessRightSelect (0x402e70) does exactly
        // the same - so 39..43 answer for the same five hero slots as the
        // portrait buttons. ProcessHover (0x403010) leaves them at the
        // default rollover text, which is why the row is the highlight
        // strip rather than a second set of buttons.
        HERO_LOCATOR_0_ID = 39,
        HERO_LOCATOR_1_ID = 40,
        HERO_LOCATOR_2_ID = 41,
        HERO_LOCATOR_3_ID = 42,
        HERO_LOCATOR_4_ID = 43,
        NUM_HERO_BUTTONS = 5,
        NUM_TOWN_BUTTONS = 5
    };
    // Retail-only ids accepted by convertID2HelpID. Their numeric identity
    // and help-row mapping are byte-proven; no surviving symbol source gives
    // the widget roles, so keep the spellings explicitly provisional.
    enum EHelpWidgetIDs {
        ROLLOVER_TEXT_ID = 200,
        HELP_WIDGET_1001_ID = 1001,
        HELP_WIDGET_1002_ID,
        HELP_WIDGET_1003_ID,
        HELP_WIDGET_1004_ID,
        HELP_WIDGET_1005_ID,
        HELP_WIDGET_1006_ID,
        HELP_WIDGET_1007_ID,
        HELP_WIDGET_1008_ID,
        HELP_WIDGET_1009_ID,
        HELP_WIDGET_1010_ID,
        HELP_WIDGET_1011_ID,
        HELP_WIDGET_1012_ID,
        HELP_WIDGET_1013_ID,
        HELP_WIDGET_1014_ID,
        HELP_WIDGET_1015_ID
    };
    // The DC roster puts RadarWidget/MapWidget at +0x44/+0x48. Retail's
    // heroWindow is eight bytes wider, placing them at +0x4c/+0x50;
    // InMapArea independently proves MapWidget's retail offset and reads
    // its widget x/y/width/height fields.
    widget* m_radarWidget;
    widget* m_mapWidget;
    textWidget* m_chatTextWidget;  // +0x54, CompleteDraw's chat update target
    // Retail heroWindow is 8 bytes wider than the Dreamcast base (0x4c
    // versus 0x44). Applying that independently proven shift to the DC
    // TAdventureMapWindow member roster puts chatEdit@0x50 at retail
    // +0x58. KeyboardMessageHandler confirms the result directly with
    // `mov ecx,[advWindow+0x58] / mov al,[ecx+0x6d]`; +0x6d is the
    // byte-proven textEntryWidget::bHasFocus field. Dreamcast's member roster
    // supplies the stronger source type CChatEdit*; Complete retains the same
    // inherited field offset.
    CChatEdit* m_chatEdit;
    // UpdateResourceDisplay (0x403f00) calls through this +0x5c field.
    class TResourceDisplay* m_resourceDisplay;
    class bitmapBackedTextWidget* m_rolloverTextWidget;  // +0x60
    int m_topHero;
    int m_topTown;

private:
    // DC member name at +0x64; retail's independently proven 8-byte base
    // shift places it at +0x6c, exactly where animate_bottom_view reads it.
    unsigned char m_animateInBackground;

public:
    // Dreamcast places three alignment bytes after animate_in_background.
    // Complete keeps the byte at +0x6c before the pointer array at +0x70.
    char m_paddingAfterAnimateInBackground[3];
    // +0x70 and +0x84, sliced 2026-08-14 from UpdateHeroLocator
    // (0x403560) and HighlightLocators (0x4038c0): the first row takes
    // bitmapBorder::SetImage with the portrait name out of akHeroTraits
    // and is indexed `[this + i*4 + 0x70]`, the second takes SetImage
    // with hpsyyy.pcx plus a send_message/Draw pair and is indexed
    // `[this + i*4 + 0x84]`. Five entries each is fixed at both ends -
    // the loops bound on 5 and 0x70 + 2*5*4 lands exactly on bottomView.
    class bitmapBorder* m_heroPortraits[5];
    class bitmapBorder* m_heroLocators[5];

private:
    // ClearBottomView (0x403ee0) owns and clears the pointer at +0x98.
    class type_bottom_view_window* m_bottomView;

public:
    // Complete-only owned popup state: Open constructs it and Close deletes
    // it; the ctor initializes the pointer before installing this vtable.
    // The public name is not attested, so retain the cross-build role name.
    void* m_immersion;
    TAdventureMapWindow();
    ~TAdventureMapWindow();
    virtual int open(int zOrder, unsigned char update);
    virtual void close(unsigned char update);
    virtual void vslot8(unsigned char on);
    unsigned char processRightSelect(const message* msg);
    unsigned char processHover(int hx, int hy);
    void doHeroKnob(unsigned char up);
    void doTownKnob(unsigned char up);
    void updateHeroLocators(int top, unsigned char drawWin,
                            unsigned char update);
    void updateTownLocators(int top, unsigned char drawWin,
                            unsigned char update);
    void updateHeroLocator(int which, unsigned char drawWinSect,
                           unsigned char update);
    void updateTownLocator(int which, unsigned char drawWinSect,
                           unsigned char update);
    void highlightLocators(unsigned char update);
    void updateSpellButton(const class hero* thisHero);
    void updateSleepButton(const class hero* thisHero);
    void updateQuestLogButton(unsigned char update);
    unsigned char setElevationToggleImage(int level);
    // Retail 0x403cc0 is `ret 4` over one stack argument, so the
    // Dreamcast roster's zero-parameter TAdventureMapWindow spelling does
    // not transfer - the same divergence UpdateSleepButton records just
    // above. DC's TAdvMenu sibling (adventuremapwindow.cpp:2056, dc
    // 0x2a74) carries the parameter and names it `image`, and
    // DoAdvCommand's one call site passes the literal 0.
    void setSleepImage(int image);
    void animateBottomView(unsigned char inBackground);
    void clearBottomView();
    void drawBottomView(unsigned char update);
    inline void setBackgroundAnimation(unsigned char enable);
    void setBottomView(class type_bottom_view_window* newView);
    void updateResourceDisplay(bool draw, bool update);
    static void setAdvWinButtonPalette(int id, int player);
    void drawChatText(unsigned char update);
    void updateButtons(unsigned char draw, unsigned char update);

private:
    int convertID2HelpID(int id) const;
};
SIZE(TAdventureMapWindow, 0xa0);

// Derives baseManager (0x38 bytes): executive::CallManager (0x4b0c70)
// compares it against executive::currentManager and writes
// baseManager::status (+0x34) as its suspend/resume mode.
class advManager : public baseManager {
public:
    // Provisional ordinal names. OverrideBottomView proves the complete
    // 0..8 range and distinct behavior for 0, 1..6 and 8; semantic names
    // remain unclaimed.
    enum EBottomViewType {
        BOTTOM_VIEW_DEFAULT = 0,
        BOTTOM_VIEW_1,
        BOTTOM_VIEW_2,
        BOTTOM_VIEW_3,
        BOTTOM_VIEW_4,
        BOTTOM_VIEW_5,
        BOTTOM_VIEW_6,
        BOTTOM_VIEW_7,
        BOTTOM_VIEW_8
    };
    enum ECursorDrawCell {
        CURSOR_DEST_Y0 = 7,
        CURSOR_DEST_Y1 = 8,
        CURSOR_DEST_X0 = 8,
        CURSOR_DEST_X1 = 9,
        CURSOR_DEST_X2 = 10
    };
    // cursorType is DC-attested as a plain int. Retail ProcessHover proves
    // only this distinguished state, so retain an ordinal spelling rather
    // than inventing a semantic domain name.
    enum ECursorTypeState {
        CURSOR_TYPE_8 = 8,
        // DoEventAnchor (0x49e670) parks the map cursor in this state as a
        // hero steps ashore, where DoEventBoat parks it in 8 as one boards.
        // Ordinal for the reason 8 is: no surviving name covers the domain.
        CURSOR_TYPE_34 = 0x22
    };
    enum EObjectDrawLayer {
        OBJECT_DRAW_LAYER_HERO_FRONT = 1,
        OBJECT_DRAW_LAYER_HERO_BACK = 2,
        OBJECT_DRAW_LAYER_LAST = 6
    };
    enum ECompleteDrawExtent {
        COMPLETE_DRAW_LAST_X = 19,
        COMPLETE_DRAW_LAST_Y = 17
    };
    // The view-relative tile the acting hero is always parked on. The
    // adventure view is COMPLETE_DRAW_LAST_X+1 by COMPLETE_DRAW_LAST_Y+1
    // tiles and every recentring path derives radarOrigin as
    // `mapX - 9 / mapY - 8` (ProcessRadarSelect and the kingdom-overview
    // arm of ProcessDeSelect both spell it out), so this pair is that
    // offset read back: ProcessMapSelect compares lastHoverX/lastHoverY
    // against it to recognise a click on the hero's own square.
    // Gated to advmgr.obj's own view for the standing include-set reason,
    // exactly as EAdvCommand is: ProcessMapSelect is the only reader, and
    // an ungated nested enum is a type DEFINITION in the closure of every
    // TU that includes this header. MEASURED score-neutral in both
    // positions today - the gate is prophylaxis, not a repair.
    enum EHeroViewTile {
        HERO_VIEW_TILE_X = 9,
        HERO_VIEW_TILE_Y = 8
    };
    enum EHoverBounds {
        HOVER_SCREEN_WIDTH = 800,
        HOVER_SCREEN_HEIGHT = 600,
        HOVER_SCROLL_MARGIN = 16,
        HOVER_SCROLL_RIGHT = 784,
        HOVER_SCROLL_BOTTOM = 584,
        HOVER_SCROLL_POINTER_FIRST = 32,
        HOVER_SCROLL_POINTER_LAST = 39
    };
    enum ECloudDrawFrame {
        CLOUD_DRAW_FRAME_1 = 1,
        CLOUD_DRAW_FRAME_3 = 3,
        CLOUD_DRAW_FRAME_4 = 4,
        CLOUD_DRAW_FRAME_5 = 5,
        CLOUD_DRAW_FLIPPED_OFFSET = 100
    };
    enum EAdventureScreenUpdate {
        ADVENTURE_SCREEN_X = 0,
        ADVENTURE_SCREEN_Y = 8,
        ADVENTURE_SCREEN_WIDTH = 608,
        ADVENTURE_SCREEN_HEIGHT = 544,
        ADVENTURE_ANIMATION_MAX_ELAPSED = 180
    };
    // The adventure screen's help-id band, proven by ProcessSelect's
    // shared tail: after the widget switch it answers a right-click
    // (MESSAGE_MODIFIER_RIGHT) on any id in this inclusive range with one
    // general-text row through NormalDialog, and ignores every id outside
    // it. Bounds are retail's own `cmp 0x7d0 / jl` and `cmp 0x898 / jg`;
    // no surviving symbol names the band, so the spelling is provisional.
    enum EAdventureHelpIds {
        ADV_HELP_ID_FIRST = 2000,
        ADV_HELP_ID_LAST = 2200
    };
    enum EAdventureSoundExtent {
        ADVENTURE_ACTIVE_SOUND_COUNT = 4,
        ADVENTURE_XLARGE_MAP_WIDTH = 144
    };
    // Open's load-bar pacing: the two mid-batch IncProgressBar ticks fire
    // at the halfway index of the cached-graphics and cursor-icon loops.
    enum EAdventureOpenProgress {
        CACHED_GRAPHIC_TICK = 19,
        CURSOR_ICON_TICK = 9
    };
    // +0x38, the townManager::netMsgHandler counterpart. Retail proves the
    // identity across three bodies: the constructor nulls the slot, Open
    // allocates the handler (its inlined ctor calls ??0CNetMsgHandler@@QAE@XZ
    // for the base), stores it here and hands it to
    // CDPlayHeroes::SetNetMsgHandler(CNetMsgHandler*), and Close deletes it
    // through vtable slot 0 - the same scalar-deleting-destructor slot
    // townManager::Close uses on its own CTownNetMsgHandler. Dreamcast
    // supplies the name and types the slot as the BASE pointer, which is
    // what retail's delete needs: the destructor is CNetMsgHandler's.
    CNetMsgHandler* m_netMsgHandler;
    unsigned char m_debugShowFps;  // +0x3c, DC name; retail FPS branch proves it
    unsigned char m_debugViewAll;  // +0x3d, bypasses hover ownership checks
    // Dreamcast DebugShowFPS/DebugViewAll (+0x50/+0x51) is byte storage; retail
    // retains it at +0x3c/+0x3d. This gap aligns the following
    // original advCommand dword to four bytes.
    char m_paddingBeforeAdvCommand[2];
    int m_advCommand;  // +0x40, set by map-hover actions
    TAdventureMapWindow* m_advWindow;  // +0x44 (the button-status target)
    unsigned short* m_routeArray;  // +0x48 (GetRouteArrayPtr)
    int m_showRoute;  // +0x4c, gates both arrow draw passes
    // Dreamcast supplies both names. Retail SeedTo independently proves the
    // pair at +0x50/+0x54: a zero seedingValid starts a fresh search, while a
    // set fullySeeded suppresses an attempted continuation.
    int m_seedingValid;
    int m_fullySeeded;
    // +0x58, a dword read as the index into soundmgr's 9-entry terrain
    // -> music-id table at 0x678330 (soundManager::SetMusicVolume
    // 0x5994b0: `mov ecx,[gpAdvManager+0x58]` then
    // `mov al, byte ptr [ecx + 0x678330]`), so it carries the current
    // terrain. Name unattested - the role is what the bytes prove.
    int m_lastTerrain;
    // +0x5c. Both GetCell overloads dereference this NewfullMap record:
    // cellData at +0xd0 and Size at +0xd4.
    NewfullMap* m_fullMap;
    // Retail tile-set rows. Dreamcast supplies the surviving names and
    // extents; the retail Draw* passes prove every offset reached here.
    CSprite* m_groundTileset[10];  // +0x60
    CSprite* m_riverTileset[5];  // +0x88
    CSprite* m_roadTileset[4];  // +0x9c
    CSprite* m_borderTileset;  // +0xac
    CSprite* m_arrowTileset;  // +0xb0
    CSprite* m_gemIcons[4];  // +0xb4
    CSprite* m_starTileset;  // +0xc4
    CSprite* m_radarIcons;  // +0xc8
    CSprite* m_cloudIcons;  // +0xcc
    // +0xd0, sixteen bytes. Close proves the Dinkumware vector shape
    // directly: it reads _First at +0xd4 and _Last at +0xd8, derives
    // size() as VC6 spells it (`_First == 0 ? 0 : _Last - _First`, the
    // `cmp eax,0 / je` guard ahead of the `sub/sar 2`), indexes
    // `_First[i]` off +0xd4 to Dispose every element, then inlines
    // erase(begin(), end()) - the provably empty `copy(_L, end(), _F)`
    // loop, the out-of-line `_Destroy(_S, _Last)` COMDAT and
    // `_Last = _S`. The four-byte element stride proves a pointer
    // element and Dispose() proves it is a resource. Dreamcast supplies
    // the name and the element type - `std::vector<resource*>
    // CachedGraphics`, its member 191, sitting between cloudIcons and
    // monAttackSprites exactly as retail does. DC's STL makes the vector
    // twelve bytes against Dinkumware's sixteen, which is the whole of
    // the 232->244 versus 0xd0->0xe0 drift.
    std::vector<resource*> m_cachedGraphics;
    CSprite* m_movingObjectSprite;  // +0xe0, transient object draw override
    // +0xe4. The five-argument UpdateRadar overload forwards this packed
    // point by value as the origin argument of the six-argument overload.
    type_point m_radarOrigin;
    type_point m_lastMapHover;  // +0xe8
    int m_lastHoverX;  // +0xec
    int m_lastHoverY;  // +0xf0
    int m_scrollX;  // +0xf4, DC advManager::scrollX
    int m_scrollY;  // +0xf8, DC advManager::scrollY
    // The constructor zeros both animation counters. The second drives the
    // frame-selection modulo.
    int m_animFrame;  // +0xfc
    int m_animCtr;  // +0x100
    // +0x104. UpdateScreen skips both the frame increment and timer catch-up
    // while this byte is set. Dreamcast supplies the surviving member name.
    unsigned char m_animCtrPaused;
    // Dreamcast animCtrPaused (+0x118) is byte storage; retail
    // retains it at +0x104. This gap aligns the following
    // original flagFrame dword to four bytes.
    char m_paddingBeforeFlagFrame[3];
    // +0x108, a dword the constructor zeroes right after animFrame.
    // Role unattested; the width is what the ctor's dword store proves.
    int m_flagFrame;
    // Retail DrawHeroPart indexes these pointer rows directly. The extents
    // close every gap through +0x1ec and agree with the surviving roster.
    CSprite* m_cursorIcons[18];  // +0x10c, indexed by hero class
    CSprite* m_boatIcons[3];  // +0x154, indexed by boat type
    CSprite* m_boatFrothIcons[3];  // +0x160, indexed by boat type
    CSprite* m_flagIcons[8];  // +0x16c, indexed by player owner
    CSprite* m_boatFlagIcons[3][8];  // +0x18c, [boat type][player owner]
    unsigned char m_drawCursor;  // +0x1ec, gates map cursor overlays
    // Dreamcast cursorVisible (+0x1f8) is byte storage; retail
    // retains it at +0x1ec. This gap aligns the following
    // original cursorType dword to four bytes.
    char m_paddingBeforeCursorType[3];
    int m_cursorType;  // +0x1f0, hover cursor-mode discriminator
    // Cursor animation run. Dreamcast supplies the five consecutive names
    // at +0x200..+0x210; retail's independently proven cursor-array extent
    // and TurnTo body place the same run twelve bytes earlier.
    int m_cursorDirection;  // +0x1f4
    int m_cursorBaseFrame;  // +0x1f8
    int m_cursorSequence;  // +0x1fc
    int m_cursorFrameCount;  // +0x200
    int m_cursorTurning;  // +0x204
    int m_cursorDrawn;  // +0x208, cleared at the start of CompleteDraw
    unsigned char m_curHeroMobile;  // +0x20c, DC name; Mobilize bails when set
    // Dreamcast bCurHeroMobile (+0x218) is byte storage; retail
    // retains it at +0x20c. This gap aligns the following
    // original iShowMode dword to four bytes.
    char m_paddingBeforeShowMode[3];
    int m_showMode;  // +0x210, DC name
    int m_forceCompleteDraw;  // +0x214, DC name
    int m_movingObjectIndex;  // +0x218, transient object-pool index
    int m_movingObjectSequence;  // +0x21c
    int m_movingObjectFrame;  // +0x220
    int m_touchedSounds;  // +0x224, DC name
    soundNode m_soundArray[4];  // +0x228, DC name and extent
    sample* m_loopedSample[LOOPING_SOUND_COUNT];  // +0x248, DC name
    sample* m_heroSamples[11];  // +0x360, DC name and extent
    int m_heroLogoShowing;  // +0x38c, DC name
    // +0x390. SetHeroContext's tail gates the closing
    // ForceMouseMove/lastHoverX reset on Dreamcast's bHeroMoving byte.
    unsigned char m_heroMoving;
    // Dreamcast bHeroMoving (+0x39c) is byte storage; retail
    // retains it at +0x390. This gap aligns the following
    // original CurrentBottomView dword to four bytes.
    char m_paddingBeforeBottomViewType[3];
    // +0x394: UpdBottomViewEnemyTurn compares this against 5 before
    // rebuilding the view, then stores 5 before installing the new window.
    EBottomViewType m_bottomViewType;

private:
    EBottomViewType m_bottomViewOverride;  // +0x398

public:
    unsigned long m_bottomViewDeadline;  // +0x39c
    int m_bottomViewResourceType;  // +0x3a0
    int m_bottomViewResourceQuantity;  // +0x3a4
    std::string m_bottomViewMessage;  // +0x3a8

    advManager();
    virtual int open(int newPriority);
    virtual void close();
    virtual int main(message& msg);
    void updateScreen(int allowIntermediateMouse, int forceDraw);
    BlackBoxData* getBlackBox(const ExtraInfoUnion* cell) const;
    TreasureData* getTreasureData(NewmapCell* cell) const;
    void redrawAdvScreen(unsigned char update, unsigned char forceSaveBorder);
    NewmapCell* doAdvCommand(type_point* triggerPoint);
    // advmgr.obj joins the gate for its own DoAdvCommand, whose route walker
// hands the trigger cell straight to this dispatcher. The guard is SPLIT
// around the one declarator rather than moved, so the preprocessed text
// every events-view consumer sees is unchanged, line for line.
    void doEvent(NewmapCell* eventCell, type_point point);
    void doAIEvent(NewmapCell* cell, class hero* currentHero,
                   type_point point);
    void deactivateCurrTown(unsigned char waitingPlayer);
    void demobilizeCurrHero(unsigned char waitingPlayer, unsigned char update);
    void deactivateCurrHero(unsigned char waitingPlayer);
    void heroSwap(class hero* leftHero, class hero* rightHero);
    void generatorEvent(class hero* who, NewmapCell* eventCell,
                        type_point point);
    void townEvent(NewmapCell* cell, type_point point,
                   unsigned char humanPlayer);
    // monType IS `int` HERE and the Dreamcast's `W4TCreatureType@@` is not.
    // The reason is a call site, not taste: DoEventMine (0x4a39a0) passes
    // armyGroup::armies[0], which this tree spells `int`, so a TCreatureType
    // parameter forces the union bridge INTO the argument list - and VC6
    // HOISTS an inline-expanded call out of the right-to-left argument
    // chain, creating its pseudo before every other argument and permuting
    // the whole EAX/ECX/EDX assignment (96.30 against 100.0, measured with
    // and without). Naming the bridge result in a local first does not
    // help; the pseudo is created early either way. monsters_fight, which
    // passes a TCreatureType local, stays exact across the change, and a
    // call relocation's symbol name is not scored.
    int combatMonsterEvent(class hero* who, int monType,
                           int* numMons, NewmapCell* eventCell,
                           type_point point, enum TCreatureType monType2,
                           int numMons2, int numGroups2,
                           enum TCreatureType monType3, int numMons3,
                           int numGroups3);
    void doWhirlpool(class hero* who);
    unsigned char doSystemOptions();
    void heroLoses(class hero* who, int vanishSound);
    void insertSound(int x, int y, int z, int soundPriority, int soundsType);
    void eraseAndFizzle(NewmapCell* eventCell, type_point point,
                        int fizzleSound);
    int processSelect(const message* msg, type_point* triggerPoint,
                      NewmapCell** peventCell);
    int processDeSelect(const message* msg, unsigned char* exitFlag,
                        type_point* triggerPoint, NewmapCell** peventCell);
    int processKeyPress(const message* msg, unsigned char* exitFlag,
                        type_point* triggerPoint, NewmapCell** peventCell);
    void processRadarSelect(const message* msg);
    void processMapSelect(const message* msg, type_point* triggerPoint,
                          NewmapCell** peventCell);
    void eraseObj(NewmapCell* thisCell, type_point point,
                  unsigned char record);
    void overrideBottomView(EBottomViewType view, int time);
    void hideRoute(int updateScreen, int removeTarget, int changeButton);
    void reseed(int targetX, int targetY);
    void checkDimNextHeroBut();
    // cursor.obj's adjacent-monster sweep (0x481900), an advManager member
    // the Dreamcast declares in cursor.cpp and this tree carries in
    // src/cursor.cpp's carcass. DoEventAnchor is its consumer here and
    // passes a local it never reads back.
    void checkAdjacentMon(int* foughtBattle);
    void setInitialMapOrigin();
    // 0x4ad470, DECLARED not defined - 5425 EH-framed bytes this lane is
    // not reconstructing. `ret 0x28` against the Dreamcast's TEN
    // parameters is the arity screen, the 5425/2540 size ratio sits in
    // the SH4->x86 band, and the DC xref graph makes DoCombat exactly the
    // two-call callee TownEvent has left once every other edge is
    // matched. Both retail call sites fill the left/right pairs in the
    // DC's own order.
    int doCombat(type_point point, class hero* leftHero,
                 armyGroup* leftArmyGroup, long rightPlayer,
                 class town* rightTown, class hero* rightHero,
                 armyGroup* rightArmyGroup, int seed,
                 unsigned char finishHeroes,
                 unsigned char alternateLayout);
    int doNetCombat(class CNetMsg* netMsg);
    void sendHeroTownData(type_point point, hero* leftHero,
                          armyGroup* leftArmyGroup, long rightPlayer,
                          town* rightTown, hero* rightHero,
                          armyGroup* rightArmyGroup, int seed,
                          int toWhoNetPos, int winner,
                          unsigned char retreatWin,
                          unsigned char combatSurrender);
    void bvResMsg(const char* message, int resourceType, int quantity);
    void bvMessage(const char* message);

private:
    unsigned char updBottomViewHero(unsigned char forceUpdate);
    unsigned char updBottomViewTown(unsigned char forceUpdate);
    unsigned char updBottomViewKingdom(unsigned char forceUpdate);
    unsigned char updBottomViewEnemyTurn(unsigned char forceUpdate);
    unsigned char updBottomViewNewTurn(unsigned char forceUpdate);
    unsigned char updBottomViewResMsg(unsigned char forceUpdate);
    unsigned char updBottomViewMessage(unsigned char forceUpdate);
    void doEventAnchor(class hero* currentHero, bool humanPlayer);
    void doEventArena(class hero* currentHero, NewmapCell* cell,
                      bool humanPlayer);
    void doEventArtifact(class hero* currentHero, NewmapCell* cell,
                         type_point point, bool humanPlayer);
    void doArtifactSkillRequirement(class hero* currentHero,
                                    NewmapCell* cell, type_point point,
                                    int skill, const char* dialogText,
                                    bool humanPlayer);
    void doEventFreeArtifact(class hero* currentHero, NewmapCell* cell,
                             type_point point, bool humanPlayer);
    void fightForArtifact(class hero* currentHero, NewmapCell* cell,
                          type_point point, bool humanPlayer);
    void doCustomArtifact(class hero* currentHero, NewmapCell* cell,
                          type_point point, bool humanPlayer);
    void giveArtifact(class hero* currentHero, type_point point,
                      bool humanPlayer);
    void payForArtifact(class hero* currentHero, NewmapCell* cell,
                        type_point point, const char* dialogText,
                        short goldCost, short resourceCost,
                        bool humanPlayer);
    // The movement-only map event shares Pandora's Box's record and reward
    // machinery but does not prompt for acceptance.
    void handleMapEvent(class hero* currentHero, NewmapCell* cell,
                        type_point point, bool humanPlayer);
    void doEventBlackBox(class hero* currentHero, NewmapCell* cell,
                         type_point point, bool humanPlayer);
    // DC events.cpp:852 returns unsigned char and takes a byte player flag.
    // Retail 0x49fa90 returns its saved reward byte after string cleanup.
    unsigned char giveBlackBoxReward(const char* text, class hero* currentHero,
                            NewmapCell* cell, type_point point,
                            unsigned char humanPlayer, class BlackBoxData* blackBox);
    void doEventBoat(class hero* currentHero, NewmapCell* cell);
    void doEventBorderGuard(type_point point, NewmapCell* cell,
                            unsigned char humanPlayer);
    void doEventBorderGate(NewmapCell* cell, unsigned char humanPlayer);
    void doEventBorderTent(NewmapCell* cell, unsigned char humanPlayer);
    void doEventBouy(class hero* currentHero, NewmapCell* cell,
                     unsigned char humanPlayer);
    // The campfire (jump-table arm 0x0c). FOUR arguments and `ret 0x10`:
    // the map point rides along for EraseAndFizzle, which erases the object
    // the hero just stepped on.
    void doEventCampfire(class hero* currentHero, NewmapCell* cell,
                         type_point point, bool humanPlayer);
    void doEventCloverField(class hero* currentHero, NewmapCell* cell,
                            unsigned char humanPlayer);
    void doEventCoverOfDarkness(NewmapCell* cell, type_point point,
                                bool humanPlayer);
    // 0x4abdc0, DECLARED not defined - 1744 bytes this lane is not
    // reconstructing. LOCATED by do_event_dragon_city, which is its only
    // reconstructed caller: retail pushes FIVE arguments in exactly the
    // Dreamcast's parameter order (hero, cell, text, point, human_player)
    // with the advManager in ECX, 1744 B against the DC's 1330 is 1.31 in
    // the SH4->x86 band, and the row sits in the events.obj bracket
    // [0x4ab410..0x4acbb0] that already carries EventSound before it and
    // adjust_army after it, in DC roster order. The DC decorates cText
    // `char*`; retail's one reconstructed call site passes window.h's
    // `const char emptyRolloverText[]`, so the declarator is const-correct
    // rather than casting at the call.
    int creatureBankEvent(class hero* who, NewmapCell* cell,
                          const char* text, type_point point,
                          unsigned char humanPlayer);
    void doEventCreatureBank(class hero* currentHero, NewmapCell* cell,
                             type_point point, bool humanPlayer);
    // The creature dwelling (jump-table arms 0x11 and 0x14 share the one
    // call). Four parameters and `ret 0x10`, the DC's own order; the row
    // (0x4a18b0) stays claimed from events.cpp's carcass until its body
    // lands.
    void doEventCreatureGenerator(class hero* currentHero, NewmapCell* cell,
                                  type_point point, bool humanPlayer);
    void doEventDefenseTower(class hero* currentHero, NewmapCell* cell,
                             bool humanPlayer);
    void doEventDragonCity(class hero* currentHero, NewmapCell* cell,
                              type_point point, bool humanPlayer);
    void doEventFaerieRing(class hero* currentHero, NewmapCell* cell,
                           unsigned char humanPlayer);
    void doEventFlotsam(class hero* currentHero, NewmapCell* cell,
                        type_point point, bool humanPlayer);
    void doEventFountain(class hero* currentHero, ExtraInfoUnion* cell,
                         bool humanPlayer);
    void doEventFountainOfYouth(class hero* currentHero, NewmapCell* cell,
                                bool humanPlayer);
    void doEventGarden(class hero* currentHero, NewmapCell* cell,
                       bool humanPlayer);
    void doEventIdol(class hero* currentHero, NewmapCell* cell,
                     bool humanPlayer);
    // The two objects that pay a resource out of the cell's own packed
    // record. Both take ExtraInfoUnion for the same reason the war school
    // and the two mills do: nothing but the +0x00 dword is ever touched.
    void doEventLeanTo(class hero* currentHero, ExtraInfoUnion* cell,
                       bool humanPlayer);
    void doEventLibrary(class hero* currentHero, NewmapCell* cell,
                        bool humanPlayer);
    void doEventLighthouse(NewmapCell* cell, unsigned char humanPlayer);
    // The School of Magic (jump-table arm 0x2f). FOUR arguments and
    // `ret 0x10` - the map point rides along because the AI arm appraises
    // the tile with AI_value_of_event before it will pay.
    void doEventMagicSchool(class hero* currentHero, NewmapCell* cell,
                            type_point point, bool humanPlayer);
    // The two mana refills. Both take the union pointer for the same
    // reason as the mills: only the +0x00 dword is ever touched, and the
    // well's whole use of it is a single `cell->value = 0`.
    void doEventMagicSpring(class hero* currentHero, ExtraInfoUnion* cell,
                            bool humanPlayer);
    void doEventMagicWell(class hero* currentHero, ExtraInfoUnion* cell,
                          bool humanPlayer);
    void doEventMercenaryCamp(class hero* currentHero, NewmapCell* cell,
                              bool humanPlayer);
    void doEventMermaid(class hero* currentHero, NewmapCell* cell,
                        unsigned char humanPlayer);
    void doEventMine(NewmapCell* cell, class hero* currentHero,
                     type_point point, bool human);
    void doEventMysticalGarden(class hero* currentHero, ExtraInfoUnion* cell,
                               bool humanPlayer);
    void doEventOasis(class hero* currentHero, NewmapCell* cell,
                      bool humanPlayer);
    void doEventPyramid(class hero* currentHero, NewmapCell* cell,
                          type_point point, bool humanPlayer);
    void doEventPowerSchool(class hero* currentHero, NewmapCell* cell,
                            bool humanPlayer);
    void doEventRallyFlag(class hero* currentHero, NewmapCell* cell,
                          bool humanPlayer);
    void doEventRefugeeCamp(class hero* currentHero, NewmapCell* cell,
                            bool humanPlayer);
    void doEventResource(NewmapCell* cell, class hero* currentHero,
                         type_point point, bool humanPlayer);
    void doCustomResource(NewmapCell* cell, class hero* currentHero,
                          type_point point, bool humanPlayer);
    void doEventScholar(class hero* currentHero, NewmapCell* cell,
                        type_point point, bool humanPlayer);
    void doEventSeaChest(class hero* currentHero, NewmapCell* cell,
                         type_point point, bool humanPlayer);
    void doEventSkeleton(class hero* currentHero, ExtraInfoUnion* cell,
                         bool humanPlayer);
    // The shipwreck survivor (jump-table arm 0x56). FOUR arguments and
    // `ret 0x10` - the point is EraseAndFizzle's again.
    void doEventSurvivor(class hero* currentHero, NewmapCell* cell,
                         type_point point, bool humanPlayer);
    // The three shrine tiers share one handler.  Dreamcast publishes the
    // complete five-argument signature; retail's `ret 0x14` agrees and its
    // cell reads include both the packed spell lane and the visit mask.
    void doEventShrine(class hero* currentHero, NewmapCell* cell,
                       const char* prompt, GlobalInfoFlags type,
                       bool humanPlayer);
    // The Sirens (jump-table arm 0x5c), same three-parameter `ret 0xc`
    // shape as the stables below and the cell equally unused.
    void doEventSiren(class hero* currentHero, NewmapCell* cell,
                      bool humanPlayer);
    void doEventSpellScroll(class hero* currentHero, NewmapCell* cell,
                            type_point point, bool humanPlayer);
    // The spell scroll (jump-table arm 0x5d) and the customised-cell
    // handler it delegates to. Both are the Dreamcast's own four-argument
    // signatures with `ret 0x10`; DoCustomSpellScroll is DECLARED only, as
    // a PRIVATE member, and its row (0x4a5a80) is not claimed here.
    void doCustomSpellScroll(class hero* currentHero, NewmapCell* cell,
                             type_point point, bool humanPlayer);
    void doEventStables(class hero* currentHero, NewmapCell* cell,
                        bool humanPlayer);
    void doEventTemple(class hero* currentHero, NewmapCell* cell,
                       bool humanPlayer);
    void doEventTrainingGrounds(class hero* currentHero, NewmapCell* cell,
                                bool humanPlayer);
    // The treasure chest (jump-table arm 0x65) and the payout dialog it
    // hands its two amounts to. Both are the Dreamcast's own signatures -
    // the chest four arguments and `ret 0x10`, the dialog a PRIVATE
    // `(hero*, int, bool)`. DoTreasureDialog is DECLARED only; its row
    // (0x4a6440) is not claimed here.
    void doTreasureDialog(class hero* currentHero, int amount,
                          bool humanPlayer);
    void doEventTreasure(class hero* currentHero, NewmapCell* cell,
                         type_point point, bool humanPlayer);
    void doEventTreeOfKnowledge(class hero* currentHero,
                                ExtraInfoUnion* cell, bool humanPlayer);
    void doEventUndeadLair(class hero* currentHero, NewmapCell* cell,
                              const char* questionText,
                              const char* emptyText,
                              const char* rewardText,
                              unsigned long visitedFlag,
                              type_point point);
    void doEventWagon(class hero* currentHero, ExtraInfoUnion* cell,
                      bool humanPlayer);
    void doEventWanderingMonster(NewmapCell* cell, class hero* currentHero,
                                 type_point point, bool humanPlayer);
    void doWanderingMonsterResult(NewmapCell* cell, class hero* currentHero,
                                  type_point point, bool humanPlayer);
    void doEventWarSchool(class hero* currentHero, ExtraInfoUnion* cell,
                          bool humanPlayer);
    void doEventWarriorTomb(class hero* currentHero, ExtraInfoUnion* cell,
                               bool humanPlayer);
    void doEventWaterWheel(class hero* currentHero, ExtraInfoUnion* cell,
                              bool humanPlayer);
    void doEventWateringHole(class hero* currentHero, NewmapCell* cell,
                                bool humanPlayer);
    void doEventWhirlpool(class hero* currentHero, NewmapCell* cell,
                            unsigned char humanPlayer);
    void doEventWindmill(class hero* currentHero, ExtraInfoUnion* cell,
                           bool humanPlayer);
    void doEventWitchHut(class hero* currentHero, ExtraInfoUnion* cell,
                            bool humanPlayer);
    void monstersFight(class hero* currentHero, NewmapCell* cell,
                        type_point point, bool humanPlayer);
    void monstersFlee(class hero* currentHero, NewmapCell* cell,
                       type_point point, bool humanPlayer);
    void monstersGiveReward(class hero* currentHero, NewmapCell* cell,
                              bool humanPlayer);
    bool monstersJoin(class hero* currentHero, NewmapCell* cell,
                       type_point point, bool wantToFight,
                       bool humanPlayer);
    bool monstersSellOut(class hero* currentHero, NewmapCell* cell,
                           type_point point, bool wantToFight,
                           bool humanPlayer);

public:
    void receiveHeroTownData(class CCombatInitMsg* combatInitMsg,
                             int* fromWho, type_point& point,
                             hero** leftHero,
                             armyGroup** leftArmyGroup,
                             int* rightPlayer, town** rightTown,
                             hero** rightHero,
                             armyGroup** rightArmyGroup, int* seed,
                             signed char* winner,
                             unsigned char* retreatWin,
                             unsigned char* combatSurrender);
    void drawGround(int srcX, int srcY, int z, int destX, int destY);
    void drawUnderlay(int srcX, int srcY, int z, int destX, int destY);
    void drawRoad(int srcX, int srcY, int z, int destX, int destY);
    void drawRiver(int srcX, int srcY, int z, int destX, int destY);
    void drawAdvObjShadow(int srcX, int srcY, int z, int destX, int destY);
    void drawAdvObj(int srcX, int srcY, int z, int destX, int destY);
    void drawArrow(int srcX, int srcY, int z, int destX, int destY);
    void drawArrowShadow(int srcX, int srcY, int z, int destX, int destY);
    void drawShroud(int srcX, int srcY, int z, int destX, int destY);
    void vwDrawGround(int srcX, int srcY, int z, int destX, int destY);
    void vwDrawUnderlay(int srcX, int srcY, int z, int destX, int destY);
    void vwDrawRoad(int srcX, int srcY, int z, int destX, int destY);
    void vwDrawRiver(int srcX, int srcY, int z, int destX, int destY);
    void vwDrawAdvObjShadow(int srcX, int srcY, int z, int destX,
                            int destY);
    void vwDrawAdvObj(int srcX, int srcY, int z, int destX, int destY);
    void vwDrawShroud(int srcX, int srcY, int z, int destX, int destY);
    void vwDrawSymbols(int srcX, int srcY, int z, int destX, int destY);
    bool scanForHeroOrBoat(int srcX, int srcY, int z, unsigned short type,
                           TDrawParts (&parts)[6]);
    void completeDraw(int startX, int startY, int z,
                      unsigned char forceDraw,
                      unsigned char updateBottomView);
    void completeDraw(unsigned char forceDraw);
    void eventSound(NewmapCell* cell);
    void eventSound(int eventID, int extraInfo);
    short recruitEvent(hero* who, TCreatureType creature, short available);
    void setEnvironmentOrigin(type_point point, int reset);
    // HeroView (0x4e1800) calls this on the dismiss path. hero.obj takes
    // the ONE declarator through its own gate rather than joining the
    // whole events view, whose other three members it never names -
    // declarator count is what moves the include-set class here.
    void fizzleCenter(int whichSound);
    void vwCompleteDraw(int startX, int startY, int z, int drawwidth,
                        int drawheight);

private:
    // human_player is spelled bool: the body forwards it dword-wide to a
    // dozen bool-parameter handlers, and an unsigned char here makes VC6
    // renormalize (`test dl,dl / setne al`) at every one of those sites.
    void dispatchEvent(class hero* currentHero, NewmapCell* cell,
                       type_point point, bool humanPlayer);
    // The three previously unnamed callees of the monolith pair, all
    // DECLARED and not defined here; their rows are not claimed from this
    // file and a call relocation's symbol name is not scored. Each of the
    // three is fixed by four independent screens at once:

    void doEventHero(class hero* currentHero, NewmapCell* cell,
                       type_point point, bool humanPlayer);
    void doEventLithOneWay(class hero* currentHero, NewmapCell* cell,
                               bool humanPlayer);
    void doEventLithTwoWay(class hero* currentHero, NewmapCell* cell,
                               bool humanPlayer);

public:
    // E:\gamedcs\AdvMgr.h:1245. DC's fixed viewport center is (6,5);
    // Complete's wider view uses (9,8), as the retail recentering paths prove.
    type_point getMapCenter() const
    {
        return type_point(m_radarOrigin.m_x + HERO_VIEW_TILE_X,
                          m_radarOrigin.m_y + HERO_VIEW_TILE_Y,
                          m_radarOrigin.m_z);
    }
    void turnTo(int newDirection);
    // cursor.cpp:52 (dc 0x79a48). Complete has no retained body, but
    // animate_move contains this ordinary helper's complete expansion.
    // Keep the source call and let VC6 make that per-build inline decision.
    void startCursor(int direction);
    void stopCursor(unsigned char standEnd);
    void drawCursor(int cellX, int cellY);
    void drawCursorShadow(int cellX, int cellY);
    void drawCursorAlpha();
    void drawHeroPart(int part, TDrawParts& heroParts, int baseX, int baseY,
                      int tilex, int tiley, int tilew, int tileh);
    void drawHeroPartShadow(int part, TDrawParts& heroParts, int baseX,
                            int baseY, int tilex, int tiley, int tilew,
                            int tileh);
    void drawBoatPart(int part, TDrawParts& boatParts, int baseX, int baseY,
                      int tilex, int tiley, int tilew, int tileh);
    void drawBoatPartShadow(int part, TDrawParts& boatParts, int baseX,
                            int baseY, int tilex, int tiley, int tilew,
                            int tileh);
    void vwDrawHeroPart(int part, TDrawParts& heroParts, int baseX,
                        int baseY, int tilex, int tiley, int tilew,
                        int tileh);
    void vwDrawHeroPartShadow(int part, TDrawParts& heroParts, int baseX,
                              int baseY, int tilex, int tiley, int tilew,
                              int tileh);
    void vwDrawBoatPart(int part, TDrawParts& boatParts, int baseX,
                        int baseY, int tilex, int tiley, int tilew,
                        int tileh);
    void vwDrawBoatPartShadow(int part, TDrawParts& boatParts, int baseX,
                              int baseY, int tilex, int tiley, int tilew,
                              int tileh);
    void drawAdventureMapGems();
    int moreTreesNear(type_point point);
    void viewPuzzle();
    void updateRadar(type_point origin, unsigned char updateFlag,
                     unsigned char partialUpdate, unsigned char viewMines,
                     unsigned char viewHeroes, unsigned char viewTowns);
    void updateRadar(unsigned char updateFlag,
                     unsigned char partialUpdate, unsigned char viewMines,
                     unsigned char viewHeroes, unsigned char viewTowns);
    void quickInfo(int cellX, int cellY, int z);
    void heroQuickView(int heroId, int x, int y,
                       unsigned char displayDropShadow);
    void townQuickView(int townId, int x, int y,
                       unsigned char displayDropShadow);
    void monsterQuickView(const NewmapCell* cell, int cellx, int celly);
    void setTownContext(int townId, unsigned char waitingPlayer,
                        unsigned char update);
    void mobilizeCurrHero(int inMove, unsigned char waitingPlayer,
                          unsigned char drawChanges);
    void setHeroContext(int heroId, int inMove,
                        unsigned char waitingPlayer,
                        unsigned char drawChanges);
    void drawRolloverText(char* text);
    void setRolloverText(NewmapCell* testCell, int rx, int ry);
    void clearBottomView();
    void getCursorSampleSet(int walkSpeed);
    void processMapSelect2(const message& msg, type_point& triggerPoint, NewmapCell*& eventCell);
    NewmapCell* getCell(int x, int y, int z);
    void checkLoadSample(e_looping_sound_id idNum);
    void checkDimHero();
    void puzzleDraw(int startX, int startY, int z, int ultX, int ultY);
    unsigned short getRouteArray(int x, int y, int z);
    NewmapCell* getCell(type_point point);
    void castSpell(SpellID whichSpell);
    void summonBoat(TSkillMastery level);
    void skuttleBoat(TSkillMastery level);
    void dimensionDoor(TSkillMastery level);
    // The four handlers RETAIL HAS NO BODY FOR. Dreamcast keeps each one out
    // of line (advspells.cpp:605 / 629 / 654 / 674, dc 0x228e8 / 0x229c4 /
    // 0x22a40 / 0x22a9c); every one has exactly one call site - its own arm
    // of CastSpell's jump table - so /Ob2 expands them all, and their 668 DC
    // bytes are what makes retail's CastSpell 1028 against the DC's 344.
    void identify(TSkillMastery level);
    void flight(TSkillMastery level);
    void disguise(TSkillMastery level);
    void waterWalk(TSkillMastery level);
    void townGate(TSkillMastery level);
    void teleportTo(hero* who, type_point destination, const char* sampleName,
                    unsigned char isRemoteMove, unsigned char drawChanges,
                    unsigned char isReplay);
    void doAdventureOptions();
    int processWaitingHover(int mouseX, int mouseY);
    int processHover(int mouseX, int mouseY);
    int processSearch(int x, int y, int z);
    void updBottomView(unsigned char forceUpdate, unsigned char drawWindow,
                       unsigned char update);
    void showRoute(int updateScreen, int reseed, int changeButton);
    void seedTo(type_point target);
    void forceNewHover();
    void screenScroll(int dir, int changeMouse);
    void checkScreenScroll();
    unsigned char findAdjacentMonster(type_point point, type_point* result,
                                      type_point excluded);
    void loadRemote(unsigned char makeOrig);
    void startLocalPlayerTurn();
    int getCloudLookup(int srcX, int srcY, int z);
    void checkCastSpell();
    void trimLoopingSounds(int maxSoundsAllowed);
    e_looping_sound_id getSoundId(int x, int y, int z);
    void disableButtons();
    void enableButtons();
    int mouseInScrollZone();
    void processMapChangeNew(class CMapChange* change);
    void viewWorld(int whatToDraw, TSkillMastery level);
    int inMapArea(int x, int y);
    type_point get_mouse_map_point() const;
    unsigned short* getRouteArrayPtr(int x, int y, int z);

private:
    void garrisonQuickView(int id, int x, int y);
    type_adventure_cursor getGarrisonCursor(NewmapCell* currCell);
    type_adventure_cursor getNormalCursor(NewmapCell* currCell);
    static int getForceModifier(float strengthRatio);
    static int getLikeModifier(class hero* currentHero,
                                 enum TCreatureType creature);

public:
    void animateMove(class hero* curr, int direction, int xInc, int yInc);
    int validMove(class hero* who, int direction, int computerMove,
                  unsigned char landOnly);
    int validMoveWithEvent(class hero* who, int direction);
    // The two out-of-compiland members DoAdvCommand reaches, DECLARED
    // and not defined here - each is defined in its own TU and a call
    // relocation's symbol name is not scored.

    // cursor.obj's 0x4805e0 (cursor.cpp:570, dc 0x7aa54). `ret 0x1c` =
    // seven stack arguments over the thiscall `this`, which is the DC
    // parameter count exactly, and the retail call site pushes them in
    // that order: the direction is the cell's own high nibble, standEnd
    // is `i == 0`, and both `int*` slots are read back immediately
    // after the call as the no-move / fought-battle verdicts.
    NewmapCell* moveHero(int direction, unsigned char standEnd,
                         type_point& triggerPoint, int* noMove,
                         unsigned char computerMove, int* foughtBattle,
                         unsigned char isRemoteMove);
    int getMoveShowIt(class hero* currHero, int direction);
    void onMoveHero(class CMapChange* change);
    void onTeleportHero(class CMapChange* change);
    void onClaimMine(class CMapChange* change);
    void onClaimTown(class CMapChange* change);
    void onBuildBoat(class CMapChange* change);
    void onEraseObject(class CMapChange* change);
    void onDeadHero(class CMapChange* change);
    void onRecruitHero(class CMapChange* change);
    void onDeadPlayer(class CMapChange* change);
    void onClaimGenerator(class CMapChange* change);
    void onClaimGarrison(class CMapChange* change);
    void onClaimShipYard(class CMapChange* change);
    void onHideHero(class CMapChange* change);

private:
    // events.obj's 0x49e2e0 (events.cpp:317, dc 0x903b4). `ret 0xc` =
    // three stack arguments, matching the DC prototype; the shipyard arm
    // hands it the trigger cell, a second copy of the map point and
    // playerData::IsLocalHuman's bool result unwidened.
    void doEventShipyard(NewmapCell* cell, type_point point,
                         unsigned char humanPlayer);
    void doEventPrison(class hero* currentHero, NewmapCell* cell,
                       type_point point, bool humanPlayer);
    NewmapCell* endMoveHero(class hero* curr, NewmapCell* returnCell,
                              unsigned char isRemoteMove, long origX,
                              long origY, unsigned char standEnd,
                              int* foughtBattle);
    NewmapCell* handleStopOnTrigger(class hero* curr,
                                       NewmapCell* destCell,
                                       unsigned char isRemoteMove,
                                       unsigned char standEnd,
                                       int* foughtBattle,
                                       long curMoveCost,
                                       long nextMoveMinCost);
};

unsigned short getMapExtra(int x, int y, int z);
inline int getMapExtra(type_point point)
{
    return ::getMapExtra(point.m_x, point.m_y, point.m_z);
}

// Retail .bss 0x699268 (DC ?gpAdvManager@@3PAVadvManager@@A).
DATA(0x00699268)
extern advManager* g_advManager;
extern int g_thisNetGotAdventureControl;

// Two town.obj-owned globals advManager::Close reads. town::View holds the
// sole DATA claims for both (retail .data 0x6aa5f0 and 0x699548) because it
// is the only body that WRITES them; these are consumer declarations, the
// gpTownManager pattern. town::View raises 0x6aa5f0 for as long as a town
// screen is up and parks the map's size band in 0x699548, which is exactly
// the nesting Close tests: it skips the ambient-music switch and keeps the
// shared tile and cursor sprite sets alive while a town is open.
extern int g_townViewActive;
extern int g_adventureGraphicsPreserveMode;

int mapExtraPosAndAdjacentsSet(int x, int y, int z, unsigned char bit);
void computeAdvNetControl();
bool hasFlag(int objType);
int getFlaggedObjectOwner(NewmapCell* thisCell);
// Retail-only 0x40d670. Ordinal placeholder: SetRolloverText and QuickInfo
// prove this five-parameter /Gr help-text signature, but no surviving name.
void advmgrFn0040D670(char* buffer, NewmapCell* cell, long playerId,
                       const char* separator, unsigned char showFullList);

// --- globals ---
// Dreamcast public ?giHighMemBuffer@@3HA; retail TrimLoopingSounds fixes the
// dword at 0x67f570 through its divide-by-100 adjustment.
extern int g_highMemBuffer;

#endif  /* HOMM3_ADVMGR_H */
