#ifndef HOMM3_SINGLESELECTIONWINDOW_H
#define HOMM3_SINGLESELECTIONWINDOW_H

#include "va.h"

#include <vector>

#include "advmgr_popup.h"
#include "game.h"
#include "netplayer.h"
#include "remote.h"
#include "rmg_request.h"
#include "town.h"

// Devil / Arch Devil, ids fixed by army.h's Inferno-run arithmetic
// (Demon 0x30 opens it, 0x35..0x37 close it); the wait dialog rerolls
// its random flavor creature past both. TU-private for the same
// include-set reason army.h scopes its own creature ids.
enum EWaitDialogCreatures {
    WAIT_CREATURE_DEVIL = 0x36,
    WAIT_CREATURE_ARCH_DEVIL = 0x37
};

// Update (0x584550) remaps a campaign scenario's version icon from the
// row's campaign ordinal: rows 0..6 are Restoration of Erathia's seven
// campaigns, 7..12 Armageddon's Blade's six, 13..19 Shadow of Death's
// seven - the same shipping-order split campaignwindow.h's
// ECampaignSets pages by (byte table at 0x584bd4: 7x0, 6x1, 7x2).
// Title identities are deliberately not imported (the
// EGameCampaignOrdinal precedent).
enum ECampaignOrdinal {
    CAMPAIGN_ROE_0 = 0,
    CAMPAIGN_ROE_1 = 1,
    CAMPAIGN_ROE_2 = 2,
    CAMPAIGN_ROE_3 = 3,
    CAMPAIGN_ROE_4 = 4,
    CAMPAIGN_ROE_5 = 5,
    CAMPAIGN_ROE_6 = 6,
    CAMPAIGN_AB_0 = 7,
    CAMPAIGN_AB_1 = 8,
    CAMPAIGN_AB_2 = 9,
    CAMPAIGN_AB_3 = 10,
    CAMPAIGN_AB_4 = 11,
    CAMPAIGN_AB_5 = 12,
    CAMPAIGN_SOD_0 = 13,
    CAMPAIGN_SOD_1 = 14,
    CAMPAIGN_SOD_2 = 15,
    CAMPAIGN_SOD_3 = 16,
    CAMPAIGN_SOD_4 = 17,
    CAMPAIGN_SOD_5 = 18,
    CAMPAIGN_SOD_6 = 19
};

// The sort columns SortMaps' jump table dispatches (`how`), in the file
// list's column order. Values are the RS_SORT_MAPS payload rungs.
enum ESortMapsColumn {
    SORT_MAPS_BY_NAME = 0,
    SORT_MAPS_BY_PLAYERS = 1,
    SORT_MAPS_BY_VERSION = 2,
    SORT_MAPS_BY_SIZE = 3,
    SORT_MAPS_BY_VICTORY = 4,
    SORT_MAPS_BY_LOSS = 5
};

// field_18A0[6] selects one of the first three filter buttons; the fourth
// button is the aggregate choice. The underlying category names are not yet
// attested, so only the byte-proven aggregate member is named.
enum EScenarioFilterCategory {
    SCENARIO_FILTER_CATEGORY_ANY = 3
};

// Widget ids consumed by TSingleSelectionWindow::OnWidgetDeselect. The
// constructor, retail jump table, and Dreamcast's named handler calls agree
// on these ranges.
enum ESingleSelectionWidgetId {
    SSW_DIFFICULTY_FIRST = 107,
    SSW_DIFFICULTY_LAST = 111,
    SSW_SCENARIO_OPTIONS = 128,
    SSW_ADVANCED_OPTIONS = 129,
    SSW_FILTER_OPTIONS = 130,
    SSW_CHAT_TOGGLE = 131,
    SSW_SIZE_FILTER_SMALL = 137,
    SSW_SIZE_FILTER_MEDIUM = 138,
    SSW_SIZE_FILTER_LARGE = 139,
    SSW_SIZE_FILTER_XLARGE = 140,
    SSW_SIZE_FILTER_ALL = 141,
    SSW_FILE_ROW_FIRST = 142,
    SSW_FILE_ROW_LAST = 159,
    SSW_BEGIN = 186,
    SSW_BACK = 188,
    SSW_SORT_SIZE = 190,
    SSW_SORT_PLAYERS = 191,
    SSW_SORT_VERSION = 192,
    SSW_SORT_NAME = 193,
    SSW_SORT_VICTORY = 194,
    SSW_SORT_LOSS = 195,
    SSW_HANDICAP_FIRST = 207,
    SSW_HANDICAP_LAST = 214,
    SSW_TOWN_PREV_FIRST = 215,
    SSW_TOWN_PREV_LAST = 222,
    SSW_TOWN_NEXT_FIRST = 223,
    SSW_TOWN_NEXT_LAST = 230,
    SSW_HERO_PREV_FIRST = 231,
    SSW_HERO_PREV_LAST = 238,
    SSW_HERO_NEXT_FIRST = 239,
    SSW_HERO_NEXT_LAST = 246,
    SSW_BONUS_PREV_FIRST = 247,
    SSW_BONUS_PREV_LAST = 254,
    SSW_BONUS_NEXT_FIRST = 255,
    SSW_BONUS_NEXT_LAST = 262,
    SSW_PLAYER_POS_FIRST = 263,
    SSW_PLAYER_POS_LAST = 270,
    SSW_GENERATE_RANDOM_MAP = 279,
    SSW_FILTER_MAP_SMALL = 281,
    SSW_FILTER_MAP_MEDIUM = 282,
    SSW_FILTER_MAP_LARGE = 283,
    SSW_FILTER_MAP_XLARGE = 284,
    SSW_FILTER_MAP_ALL = 285,
    SSW_FILTER_PLAYERS_FIRST = 287,
    SSW_FILTER_PLAYERS_LAST = 294,
    SSW_FILTER_PLAYERS_ANY = 295,
    SSW_FILTER_HUMANS_FIRST = 297,
    SSW_FILTER_HUMANS_LAST = 304,
    SSW_FILTER_HUMANS_ANY = 305,
    SSW_FILTER_TEAMS_FIRST = 307,
    SSW_FILTER_TEAMS_LAST = 314,
    SSW_FILTER_TEAMS_ANY = 315,
    SSW_FILTER_VERSION_FIRST = 317,
    SSW_FILTER_VERSION_LAST = 323,
    SSW_FILTER_VERSION_ANY = 324,
    SSW_FILTER_CATEGORY_FIRST = 326,
    SSW_FILTER_CATEGORY_LAST = 329,
    SSW_FILTER_DURATION_FIRST = 331,
    SSW_FILTER_DURATION_LAST = 333,
    SSW_FILTER_DURATION_ANY = 334,
    SSW_RANDOM_MAPS = 335,
    SSW_NAME_FIRST = 345,
    SSW_NAME_LAST = 352,
    SSW_HERO_DETAIL_FIRST = 362,
    SSW_HERO_DETAIL_LAST = 369,
    SSW_TOWN_DETAIL_FIRST = 370,
    SSW_TOWN_DETAIL_LAST = 377,
    SSW_BONUS_DETAIL_FIRST = 378,
    SSW_BONUS_DETAIL_LAST = 385,
    SSW_TEAM_ALIGNMENT = 387
};

// A cross-module dword at 0x6989f0 the game-selection window branches on
// during teardown; DoModal and ExitDialog each take a distinct path when it
// equals 3, the only value recoverable here. House ordinal placeholder,
// exactly the textntry.h EField68 rule - names the domain member so the
// branch is not a magic compare, without claiming an attested identity.
enum EWindowMode6989f0 {
    WINDOW_MODE_6989F0_3 = 3
};

// Constructor-only domains. DC gives gameMode as int; retail proves the two
// non-default commands by their load/save setup arms. The context values are
// intentionally ordinal until the gpVideoGameState owner supplies names.
enum ESingleSelectionGameMode {
    SINGLE_SELECTION_LOAD_GAME = 1,
    SINGLE_SELECTION_SAVE_GAME = 2
};

enum ESingleSelectionLaunchContext {
    SINGLE_SELECTION_LAUNCHED_FROM_CAMPAIGN = 101
};

// One queued header re-request (CMapHeaderRequestMsg's payload pair);
// CNewPlayerUpdateProc::HandleRequests drains a vector of these. Field
// order is byte-proven by both ends: HeaderRequested's push_back fills
// the byte at +0 and the dword at +4, and HandleRequests reads
// [elem+8*i] as the transfer flag and [elem+8*i+4] as the row number.
struct SHeaderRequest {
public:
    unsigned char m_flag;
    // Retail stores the preceding flag as one byte; these three bytes
    // align the following integer payload to a four-byte boundary.
    char m_paddingBeforeNumber[3];
    int m_number;
};

// Forward-declared for TSingleSelectionWindow's slider members
// (+0x1838..+0x1844); the full layouts stay in the private header so this
// public header's include closure is unchanged for advmgr/townmgr.
class slider;
class Bitmap816;
class CSaveScreen;
class CSprite;
class textEntryWidget;
class CNewPlayerUpdateMan;
class CChatWidget;
class textWidget;
class type_text_scroller;
class textButton;
class button;
struct GameSelectionHeadersStruct;

// Difficulty mirror (DC lastDiff), teardown mode and constructor headings.
// Retail addresses: 0x683454, 0x6989f0 and 0x6a8098 respectively.
extern int g_lastDiff;
extern int g_unnamed6989f0;
extern const char* g_unnamed6a8098[];

// Shared selection/scenario presentation tables. Retail scenarioinfo.obj
// references the same addresses initialized and owned by
// singleselectionwindow.obj, which proves external rather than file linkage.
extern const char* g_turnDurationText[11];
extern int g_difficultyRatingPercent[5];
extern const char* g_unnamed6a77ec[];
extern const char* g_unnamed6a7800[3];
extern const char* g_unnamed6a7e18[];
// Starting-bonus labels shared by the selection window and the Complete-only
// scenario-info row renderer. No source symbol survives for the retail table.
extern const char* g_unnamed6a5e14[];

enum ESingleSelectionGameContext {
    SINGLE_SELECTION_CONTEXT_1 = 1,
    // The middle context RebuildFilteredPlayerSetup groups with
    // SINGLE_SELECTION_CONTEXT_3 when it picks the synthesized map's
    // format version.
    SINGLE_SELECTION_CONTEXT_2 = 2,
    SINGLE_SELECTION_CONTEXT_3 = 3
};

// TRandomMapProgress and its abstract base now live in
// singleselectionpopups.h - singleselectionpopups.obj owns every one of their
// retail bodies (0x576f00..0x577320) and this TU already includes that header.

// Complete-only random-map filename chooser. Retail 0x5879a0 receives the
// hidden std::string result in ECX under /Gr; it is a free function, not a
// TSingleSelectionWindow member.
std::string getRandomMapName();

// One cached map/save header row of the file list. Only the stride is
// modeled: 0xCA4 is fixed by the size() magic-multiply in every
// vector<GameSelectionHeadersStruct>::size() expansion (WindowHandler's
// scroll arms): 0x5102371 * 2^-38 is 1/3236 exactly (the /3235 constant
// would take the add-fix form retail lacks). The DC record
// (SingleSelectionWindow.h:73) awaits field reconstruction.
struct GameSelectionHeadersStruct {
public:
    // COMPOSITION byte-proven round 2 by OnGameHeaderInfoInitMsg's two
    // resize temps (0x58a440): the default construction calls
    // ??0NewSMapHeader at +0, the SGameSetupOptions ctor at +0x304 and
    // ??0SavedGameHeader at +0x700, then zero-fills 61 B at +0x58c and
    // 301 B at +0x5c9 and stores 1 to setup.difficulty; the temp
    // teardown calls ??1SavedGameHeader then ??1NewSMapHeader. Every
    // previously proven field lands inside a member: version/numPlayers/
    // maxNumHumanPlayers/Size/condition types are header's own
    // CMapHeaderData band, slotAlignments +0x314 = setup.alignment,
    // fileName +0x33d = setup.filename, the received flag +0x4a5 =
    // setup.fileInitialized, isCampaign +0xbe0 = saved.campaignGame and
    // campaignIndex +0xbe8 = saved.campaign.currentCampaign. 0xCA4 =
    // 0x304 + 0x1cc + 0xbc + 0x3d + 0x12d + 2 + 8 + 0x5a4 exactly.
    NewSMapHeader m_header;  // +0x000
    SGameSetupOptions m_setup;  // +0x304
    // Retail's synthesized copies establish eight ints at +0x4d0.
    // Role inferred from the older header's eight-int band at DC +0x55c:
    // GetHeader 0x138b76 copies 32 bytes from original g_wasHuman into
    // it; the header ctor 0x14752c zeros the same band. Complete relocates
    // the band and uses SavedGameHeader::humanPlayer for the live restore
    // path. No Complete producer of this legacy band has been located.
    int m_wasHuman[8];  // +0x4d0
    // The selected row copies this complete 156-byte band to the game's
    // per-hero availability array before assigning the map header.
    unsigned char m_heroAvailability[156];  // +0x4f0
    // The row's display title: the name getters return it for the
    // single-player list and the net-mode selected panel, and the name
    // comparator ranks it against the "autosave" prefix rule. Extent =
    // the ctor's first zero-fill.
    char m_title[0x5c9 - 0x58c];  // +0x58c
    // Second ctor-zeroed text band (extent = the second fill); role
    // unexercised by reconstructed bodies - provisional name.
    char m_description[0x6f6 - 0x5c9];  // +0x5c9
    // +0x6f6..+0x6f8 is an UNNAMED alignment hole: retail's synthesized
    // operator= copies description's 0x12d bytes then stores fileTime's
    // two dwords directly (OnMapFileNameMsg's two expansions) - a named
    // pad array here adds a fourth byte-copy loop retail lacks.
    // The row's file stamp - a real FILETIME: DrawBasicMapInfo hands
    // its address to FileTimeToLocalFileTime.
    _FILETIME m_fileTime;  // +0x6f8
    SavedGameHeader m_saved;  // +0x700

    VA(0x00578E00, 0x25F)  // retained retail body; formerly enrolled by CLASS_CTOR
    GameSelectionHeadersStruct()
    {
        memset(m_title, 0, sizeof(m_title));
        memset(m_description, 0, sizeof(m_description));
        m_setup.m_difficulty = 1;
    }

    // Both the copy ctor and operator= are IMPLICIT: the synthesized
    // memberwise bodies are retail's 0x5904f0 and 0x578440 COMDATs, and
    // /Ob2 reproduces retail's per-site split on each - operator= called
    // in SortMaps'/OnDeleteFile's move loops and the SelectionHeaders
    // mirror, expanded for OnGameHeaderInfoMsg's source-list row; the
    // copy ctor called from the six _Sort/_Median/_Unguarded_partition
    // families and expanded into std::_Construct at 0x58f480.
};
SIZE(GameSelectionHeadersStruct, 0xCA4);

// DC supplies the source identities and member names.  Retail independently
// proves the Windows layout used here: DeletePlayer walks eight records with
// a 0x7c stride and clears dpid/+0x20/+0x24/+0x70; the constructor at
// 0x57c790 proves the remaining initialized offsets and the full record
// extent.  The containing handler's two eight-record arrays and four tail
// dwords are the complete 0x7d0-byte Windows layout.  Defined ahead of
// TSingleSelectionWindow because that window embeds one at +0x1064.
// The ctor below seeds version from the game-context cell
// (resourcemanager.cpp owns the claim).
extern int* g_videoGameState;

class CNetPlayerHandlerPlayer : public CNetPlayerInfo {
public:
    int m_heroIndex;  // +0x20
    int m_townIndex;  // +0x24
    int m_availableHeroesCount;  // +0x28
    int m_availableHeroes[16];  // +0x2c
    int m_startBonusIndex;  // +0x6c
    int m_playerPos;  // +0x70
    int m_color;  // +0x74
    // A byte in retail: the ctor 0x57c790 stores it with a byte mov and
    // OnUpdatePlayerPosMsg re-reads it movsx.
    signed char m_handicap;  // +0x78
    // Retail constructor 0x57c790 writes handicap as a byte at +0x78.
    // The 0x7c-byte player-record stride leaves three bytes of tail alignment.
    char m_tailPadding[3];
    // E:\gamedcs\SingleSelectionWindow.h:108
    VA(0x0057C790, 0x40)
    CNetPlayerHandlerPlayer()
    {
        m_heroIndex = -1;
        m_townIndex = -1;
        m_availableHeroesCount = 0;
        m_startBonusIndex = 3;
        m_playerPos = -1;
        m_color = -1;
        m_handicap = 0;
        memset(m_availableHeroes, 0, sizeof(m_availableHeroes));
    }
    // E:\gamedcs\SingleSelectionWindow.h:122
    unsigned char isHuman()
    {
        if (m_dpid)
            return 1;
        return 0;
    }
    // E:\gamedcs\SingleSelectionWindow.h:130
    void clear()
    {
        m_dpid = 0;
        m_playerPos = -1;
        m_townIndex = -1;
        m_heroIndex = -1;
    }
    // DC SingleSelectionWindow.h:138. Complete keeps the same source
    // helper but expands both calls in SetupAdvancedOptions.  Retail x86
    // directly proves that Complete changed the three independent stores
    // from DC's hero/player/town statement order to town/player/hero.
    void resetAdvancedOptions()
    {
        m_townIndex = -1;
        m_playerPos = -1;
        m_heroIndex = -1;
    }
};
SIZE(CNetPlayerHandlerPlayer, 0x7c);

class CNetPlayerHandler {
public:
    enum { MAX_PLAYERS = 8 };
    CNetPlayerHandlerPlayer m_humanPlayers[MAX_PLAYERS];  // +0x000
    CNetPlayerHandlerPlayer m_computerPlayers[MAX_PLAYERS];  // +0x3e0
    int m_playerPos;  // +0x7c0
    int m_playersCount;  // +0x7c4
    int m_unused;  // +0x7c8
    int m_assignedPos;  // +0x7cc

    // Dreamcast singleselectionwindow.cpp:1005 (dc 0x1303fc); retail
    // expands it into the window constructor's member initialisation.
    CNetPlayerHandler();
    int getNetPos(unsigned long dpid);
    int getGamePos(unsigned long dpid);
    bool deletePlayer(unsigned long dpid);
    CNetPlayerHandlerPlayer* getPlayerInPos(int pos);
    CNetPlayerHandlerPlayer* getCompPlayerInPos(int pos);
    CNetPlayerHandlerPlayer* getPlayer(unsigned long dpid);
    unsigned char isFaceTaken(int face, int exclude);
    unsigned char addNewPlayer(CNetPlayerInfo* netPlayer);
    unsigned char playerExists(unsigned long dpid);
    unsigned char setNextPlayer(int pos);
    unsigned char setComputer(int pos);
    int getUnassignedPlayerPos();
    int getPlayerCount(unsigned char assignedOnly);
};
SIZE(CNetPlayerHandler, 0x7d0);

// Dreamcast names this as the window's final shared-build member, and the
// retail constructor independently proves the same composition at +0x1888:
// CNetMsgHandler's ctor is followed by the derived vtable store and a clear
// of m_wasCompressed at +0x0c. The class is embedded by value below;
// its constructor body belongs to singleselectionwindow.cpp:8758.
// The retail vtable at 0x641ce8 has only CNetMsgHandler's four slots;
// CAdvMgrNetMsgHandler adds a fifth (HandleGiftMsg), so it cannot be the
// base. DC0x14515a likewise calls CNetMsgHandler's constructor directly.
class CSingleSelectionNetMsgHandler : public CNetMsgHandler {
public:
    CSingleSelectionNetMsgHandler();
    virtual CNetMsg* checkHandleNet(unsigned char inPopup,
                                    unsigned char* msgReceived);  // slot 1
    virtual CNetMsg* handleNetMsg(CNetMsg* netMsg);  // slot 3

    unsigned char m_wasCompressed;  // +0x0c
};
SIZE(CSingleSelectionNetMsgHandler, 0x10);

// The game-selection megawindow. DC reports size 2928 with SH4 STL; the
// retail extent 0x1970 is frame-derived from advmgr's SaveGame (window local
// ebp-0x19ec..ebp-0x7c around an __alloca_probe frame). The members below are
// the ones the reconstructed methods reach and each is retail-byte proven:
// two mode bytes at +0x64/+0x65 (DoModal/ExitDialog test them), the embedded
// CNetPlayerHandler at +0x1064 (ExitDialog walks it at a 0x7c stride) and the
// file-list slider* at +0x1840 (DoModal calls its SetState). The exact
// UpdateAllyEnemyFlags body adds the DC-named saved-background pointer
// flagBack at +0x1870. The constructor further proves the DC-named embedded
// netMsgHandler at +0x1888; the tail after it remains under reconstruction.
class TSingleSelectionWindow : public CAdvPopup {
public:
    // DC CNewPlayerUpdateProc::Go at singleselectionwindow.cpp:1277
    // loads private gameVersion directly for the message constructor
    // (dc 0x1480cc); retail 0x5789f0 preserves that relationship.
    friend class CNewPlayerUpdateProc;
    // DC names the constructor's timeGetTime snapshot clickTime; retail
    // places it at the first derived dword.
    unsigned long m_clickTime;  // 0x60
    unsigned char m_flag64;  // 0x64
    unsigned char m_flag65;  // 0x65
    // Third mode byte of the run: SortMaps (0x585050) sorts and refills
    // from TransferHeaders when it is set, HeadersA otherwise.
    unsigned char m_flag66;  // 0x66
    // The three PC mode bytes end at +0x67; textIndex starts at the
    // next dword boundary, +0x68. This byte aligns the integer.
    char m_paddingBeforeTextIndex;
    // Original Dreamcast textIndex follows the load/save mode bytes.
    // Retail initializes -1 and stores the clicked file-row index here.
    int m_textIndex;  // 0x68, selected/start row sentinel
    // The scenario list's three icon strips, byte-proven by Update
    // (0x584550): each is the receiver of a CSprite::Draw at columns
    // 91/309/342 with its own Width/Height re-read off the same load -
    // map format (frames 0..2), victory condition (0..11) and loss
    // condition (0..3).
    CSprite* m_versionIcon;  // 0x6c, Complete-only format strip
    CSprite* m_victoryIcon;  // 0x70, DC name
    CSprite* m_lossIcon;  // 0x74, DC name
    // The advanced-options row art, all byte-proven by
    // DrawHeroAdvancedOption (0x58d510): the town strip (frames
    // 2*town+2 at column 0xb0), the bonus strip (frames 3/8/9/10/town
    // at 0x148), the eight seat flags at 0x39 (element 0 supplies the
    // blit extent), the portrait plates at 0xfc (again extent from
    // element 0; the array extent to +0x35c is the layout bound, the
    // interior count is not otherwise attested), and the three special
    // plates - random town (drawn with the random-hero plate's
    // extent, exactly as retail does), random hero, and the locked
    // no-hero plate.
    CSprite* m_townPix;  // 0x78, DC name
    CSprite* m_resource;  // 0x7c, DC name
    CSprite* m_heroSpecificAbility;  // 0x80, DC name
    char m_goldBox[0x88 - 0x84];
    Bitmap816* m_flags[8];  // 0x88, DC name; adopflg%c.pcx
    Bitmap816* m_panels[8];  // 0xa8, DC name; adop_cpnl.pcx
    Bitmap816* m_heroPix[164];  // 0xc8..0x357, expanded retail roster
    char m_pad358[0x35c - 0x358];
    Bitmap816* m_randomTownBmp;  // 0x35c
    Bitmap816* m_randomHeroBmp;  // 0x360
    // Dreamcast noDice is Bitmap816* between randomHero and noHero
    // (+0x350/+0x354/+0x358). Retail's adjacent bitmap pointers are
    // +0x360/+0x368, preserving the +0x10 shift and intervening slot.
    // Source/layout recovery; no retail access to this member located.
    Bitmap816* m_noDice;  // 0x364
    Bitmap816* m_noHeroBmp;  // 0x368
    // The DC currentIndex/currentMap/durationIndex run (dc offsets
    // 864/868/872) followed by the two option-mode bytes, the save-name
    // editor and the update-proc manager (dc 876/877/880/888) - the whole
    // block sits at DC+0x10 here. Retail bytes prove each: the
    // WindowHandler scroll arms read/write +0x370 and gate on +0x37d,
    // CSaveGameEdit::OnKeyPress compares +0x374 against -1 before its
    // SetCurrentMap(-1, 1) call, the duration-slider callback 0x57c7f0
    // stores its state to +0x378 alongside gpGame->setup.turnDuration,
    // WindowHandler's Enter arm reads the editor's text through +0x380,
    // and its net pump ticks the manager at +0x388.
    unsigned char m_sortDirection;  // 0x36c (DC sortDirection, a byte here)
    // 0x5850ae and direction toggles 0x587188/0x5871c5 all access
    // sortDirection as one byte, unlike Dreamcast's int. These three
    // bytes align currentIndex at +0x370. The dword access at 0x57a2a6
    // belongs to gGeneralText's string table, not this window.
    char m_paddingBeforeCurrentIndex[0x370 - 0x36d];
    int m_currentIndex;  // 0x370, top visible file row
    int m_currentMap;  // 0x374
    int m_durationIndex;  // 0x378
    unsigned char m_inAdvancedOptions;  // 0x37c
    unsigned char m_inScenarioOptions;  // 0x37d
    // The third right-panel mode byte of the run: the scenario-filter
    // panel (retail-only - no DC counterpart). Update gates the
    // general-text 739/740 title pair and the filter-widget refresh
    // (0x57ef70) on it.
    unsigned char m_inFilterOptions;  // 0x37e
    // 0x37f: a setup byte the duration slider snapshots into
    // CNewSetupInfoMsg and OnNewSetupInfoMsg writes back. Role
    // unattested - ordinal placeholder.
    // Role-derived: entering random-map options sets this flag;
    // updateGameVars uses the synthesized localHeader when it is set.
    unsigned char m_randomMapSelected;
    textEntryWidget* m_saveGameEdit;  // 0x380
    // Dreamcast mode is a byte at +0x374 between saveGameEdit (+0x370)
    // and pNewPlayerUpdateMan (+0x378). Both retail pointer anchors shift
    // by +0x10, preserving this slot. No retail mode access located.
    unsigned char m_mode;  // 0x384
    // +0x385..0x387: alignment before the update-manager pointer.
    CNewPlayerUpdateMan* m_newPlayerUpdateMan;  // 0x388
    // The window's own scratch header row - 0x38c..0x1030 is exactly one
    // 0xCA4 element. The 11.6KB ctor constructs it in place (the
    // SavedGameHeader member ctor at this+0x38c+0x700 on its claim), and
    // SetCurrentMap points pCurrentHeader at it when field_37F selects
    // the setup-local row.
    GameSelectionHeadersStruct m_localHeader;  // 0x38c
    // The three header lists are real Dinkumware vectors, 0x1030/0x1040/
    // 0x1050 (allocator, _First, _Last, _End - the size() null-_First
    // ternary and the 0xCA4 magic-multiply in every consumer, plus
    // SortMaps' erase/insert calling the out-of-line element operator=
    // 0x578440 and the _Destroy loop 0x58f080, and the out-of-line
    // size() COMDAT 0x58eab0 seventeen callers reach with ecx =
    // this+0x1050). The full vector view needs the element type
    // complete, so it is gated to this TU; other includers keep the
    // byte-proven _First/_Last pointer pairs.
    // HeadersA: scanned by CheckMissingHeaders with request-flag 0.
    // TransferHeaders: counted by CNewPlayerUpdateProc::Go's init msg;
    // the transfer path walks it. SortMaps sorts one of the two by
    // m_flag66 and refills SelectionHeaders from it through the
    // mapSizeFilter.
    std::vector<GameSelectionHeadersStruct> m_headersA;  // 0x1030
    std::vector<GameSelectionHeadersStruct> m_transferHeaders;  // 0x1040
    std::vector<GameSelectionHeadersStruct> m_selectionHeaders;  // 0x1050
    // The header row of the currently selected map/save;
    // UpdatePlayerPositions reads the per-slot alignments through it.
    GameSelectionHeadersStruct* m_currentHeader;  // 0x1060
    CNetPlayerHandler m_players;  // 0x1064

private:
    unsigned char m_receivedMaps;  // 0x1834 (DC receivedMaps)

public:
    // Dreamcast receivedMaps is one byte, followed by aligned chatSlider.
    // Retail retains that boundary at +0x1834 and +0x1838.
    char m_paddingBeforeChatSlider[0x1838 - 0x1835];

private:
    // The window handler scrolls the file slider. On teardown, doModal resets
    // the duration slider to 11 (unlimited turns).
    slider* m_chatSlider;  // 0x1838
    slider* m_fileSlider;  // 0x183c
    slider* m_durationSlider;  // 0x1840
    slider* m_nameSlider;  // 0x1844
    textWidget* m_chatWidget;  // 0x1848 (DC chatWidget: textWidget*)
    // The DC chatWidget..flagBack member run (dc 2848..2888) maps onto
    // retail 0x1848..0x1870 LINEARLY (constant delta 3368, every
    // already-proven anchor agrees: chatShowing 2877->0x1865, chatToggle
    // 2880->0x1868, receivingMaps 2884->0x186c, flagBack 2888->0x1870).
    // The two 4-seat name columns: UpdateNameLists (0x58c960) rebuilds
    // their text (virtual SetText, slot 13) and the TurnChat pair
    // shows/hides them.
    textWidget* m_nameList1;  // 0x184c (DC nameList1)
    textWidget* m_nameList2;  // 0x1850 (DC nameList2)
    // DC mapChanged/readingMaps; no reconstructed body exercises them
    // yet - position is the linear-run proof above.
    unsigned char m_mapChanged;  // 0x1854
    unsigned char m_readingMaps;  // 0x1855

public:
    // Dreamcast mapChanged/readingMaps are bytes preceding chatEdit;
    // retail retains two alignment bytes before the pointer at +0x1858.
    char m_paddingBeforeChatEdit[0x1858 - 0x1856];

private:
    // DC chatEdit is CChatEdit*: TurnChatOn (0x58ca80) focuses its id.
    // Complete constructs CSingleSelectionChatEdit through the same base.
    CChatEdit* m_chatEdit;  // 0x1858
    int m_sortWhich;  // 0x185c

public:
    // The scenario size filter (0 = all, else an EMapDimension):
    // SortMaps admits a row into SelectionHeaders only when it is clear
    // or equal to the row's Size; SetFilter stores it.
    int m_mapSizeFilter;  // 0x1860

private:
    // DC scenarioOptionsStarted (2876 on the linear run); cleared by
    // the host-handover reset before SetupScenarioOptions(0).
    unsigned char m_scenarioOptionsStarted;  // 0x1864
    unsigned char m_chatShowing;  // 0x1865 (DC chatShowing), gates the 179 widget show

public:
    // Dreamcast scenarioOptionsStarted/chatShowing are bytes before
    // chatToggle; retail preserves two alignment bytes at +0x1866.
    char m_paddingBeforeChatToggle[0x1868 - 0x1866];

private:
    // DC chatToggle: the show/hide-chat textButton whose label the
    // TurnChat pair rewrites from general-text rows 532/533.
    textButton* m_chatToggle;  // 0x1868
    unsigned char m_receivingMaps;  // 0x186c (DC receivingMaps), cleared on header-end

public:
    // Dreamcast receivingMaps is one byte followed by aligned flagBack;
    // retail preserves this three-byte pointer-alignment gap.
    char m_paddingBeforeFlagBack[0x1870 - 0x186d];

private:
    CSaveScreen* m_flagBack;  // 0x1870, DC-attested name
    // DC gameVersion (a 20-byte TFileVersionInfo product string there);
    // OnBadVersionMsg formats it against the offender's.
    char m_gameVersion[20];  // 0x1874
    // DC's last window member. Retail constructs it in place with the
    // CNetMsgHandler base ctor, the 0x641ce8 derived vtable and a zero byte at
    // +0x0c before installing TSingleSelectionWindow's own vtable. The public
    // include view keeps the same proven extent without importing remote.h.
    CSingleSelectionNetMsgHandler m_netMsgHandler;  // 0x1888

public:
    // Retail 0x58ea00 intersects the seated humans'
    // version feature sets and returns their highest common version.
    // Construction seeds this from the local version; join/drop refresh it.
    // PC-only role-derived name; not a player count or the product string.
    int m_commonGameVersion;  // +0x1898
    // Role-derived: the constructor creates the general-text 519 heading
    // above the town controls (x=163, width=75), with ID 341 or 342 by
    // game context. Advanced-options open/close shows/hides this same ID;
    // the player-position renderer places town controls in that column.
    int m_townHeadingId;  // +0x189c
    // Eight setup dwords CNewSetupInfoMsg carries behind the
    // SGameSetupOptions copy; the random-map controls read them back out.
    // Role-derived: generateRandomMap (0x5860e0) consumes these eight
    // options for dimensions, layers, player/team counts, monsters and water.
    // CNewSetupInfoMsg transmits this same option array to the other peers.
    int m_randomMapOptions[8];  // 0x18a0
    // Complete-only random-map filter controls. CreateFilterWidgets constructs
    // each family explicitly, then walks these contiguous pointer arrays to
    // select the disabled/highlight frames and register them with the window.
    // The six loop bases and counts in retail prove every boundary below; the
    // ordinal names avoid claiming meanings not present in the older DC UI.
    button* m_filterCountAButtons[9];  // 0x18c0, ids 0x11f..0x127
    button* m_filterCountBButtons[9];  // 0x18e4, ids 0x129..0x131
    button* m_filterCountCButtons[9];  // 0x1908, ids 0x133..0x13b
    button* m_filterCountDButtons[8];  // 0x192c, ids 0x13d..0x144
    button* m_filterWaterButtons[4];  // 0x194c, ids 0x146..0x149
    button* m_filterStrengthButtons[4];  // 0x195c, ids 0x14b..0x14e
    // Retail-only tail member (no DC counterpart - DC's roster ends at
    // netMsgHandler): the widget the TurnChat pair shows with widget 105
    // when chat is OFF and hides when it is ON, always addressed
    // directly, never through GetWidget.
    // Role-derived: construction creates a CScrollTextWidget here;
    // updateGameVars (0x583580) fills it from the selected map description.
    // Retain the concrete type: ctor retail +0xb3c/+0xb3f binds the widget
    // vector insert to a converted pointer temporary at EBP+8, not this
    // member's address. All text accesses use this same scroller class.
    // Complete-only type inference; the older DC class has no such member.
    type_text_scroller* m_descriptionWidget;  // 0x196c

    TSingleSelectionWindow(int gameMode);
    virtual ~TSingleSelectionWindow();
    virtual void doModal(bool fadeIn);
    void updatePlayerPositions(unsigned char updateCurPlayer);
    virtual int windowHandler(message& msg);  // slot 9
    void onNameSlider(int newIndex);
    void onChatWindowSlider(int newIndex);
    void onDurationSlider(int newIndex);
    void onFileMenuSlider(int newIndex);
    // DC publics retain native bool (_N), although lowered CV says UCHAR.
    void updateAllyEnemyFlags(bool update);

private:
    virtual unsigned char processRightSelect(int id);  // slot 11

public:
    virtual int exitDialog(message& msg);   // slot 14
    int update();
    const char* getMapName(int which);
    const char* getFileName(int which);
    void drawBasicMapInfo();
    unsigned char onGameTransmitInitMsg(CNetMsg* netMsg);
    void updateFilterWidgets();
    // Retail 0x584c40 (no DC row proven): the post-join roster
    // re-seat OnNewPlayerMsg's non-advanced arm runs. Ordinal name.
    // DC SetHumanSlot (dc 0x13b22c, 0.84x): re-seat the human players
    // against the refreshed slot attributes.
    void setHumanSlot();
    bool handleNetMsg(CNetMsg* netMsg, bool& cancel);
    void onSortMaps(int how);
    unsigned char onBeginGame();
    void onPlayerPosClick(int pos);
    int getThisPlayerGamePos();
    void setDifficultyHiLite();
    int onWidgetDeselect(message* msg, unsigned char* exitFlag,
                         unsigned char remoteClick);
    unsigned char canChooseTown(int gamePos);
    unsigned char canChooseHero(int gamePos);
    unsigned char hasMultipleTowns(int gamePos);
    int getDisplayFace(int gamePos);
    int getHeroInPos(int gamePos);
    // Dreamcast names the enum return, and Complete's inlined nine-town
    // callers retain that enum-typed local and mask lowering.
    TTownType getDisplayTown(int gamePos);
    const char* getHeroName(int gamePos);
    void onNameChange(int gamePos, const char* newName);
    void updateNames();
    unsigned char highlightFile(char* filename);
    void onNameClick(int pos);
    unsigned char isVersionCompatible(const char* otherVersion);
    // Complete-only random-map helpers at 0x5879a0 and 0x5860e0. Their
    // provisional role names describe the byte-decoded caller contract.
    unsigned char generateRandomMap(const char* name);
    void setCurrentMap(int map, bool update);
    void drawHeroAdvancedOption(int playerPos, unsigned char update,
                                int position);
    void onDeleteFile();
    unsigned char onNewSetupInfoMsg(CNetMsg* netMsg);
    unsigned char onNewPlayerMsg(CNetMsg* netMsg);
    // DC ordinary OnPlayerDroppedMsg, line 6937; QAA_N return.
    bool onPlayerDroppedMsg(CNetMsg* netMsg);
    // DC ordinary OnNewMapHeaderInfo, source line 6968; QAA_N return.
    bool onNewMapHeaderInfo(CNetMsg* netMsg);
    unsigned char onGameHeaderInfoInitMsg(CNetMsg* netMsg);
    void onGameHeaderInfoInitMsgEx(CNetMsg* netMsg);
    void makeHeroFilter();
    void sortMaps(int how, unsigned char sendSortMsg,
                  unsigned char update);
    unsigned char onSetAsHostMsg(CNetMsg* netMsg);
    unsigned char onGameHeaderInfoMsg(CNetMsg* netMsg);
    bool onGameHeaderInfoEndMsg(CNetMsg* netMsg);
    bool onScrollMsg(CNetMsg* netMsg);
    unsigned char onBadVersionMsg(CNetMsg* netMsg);
    void setFilter(int size);
    void sendChat(unsigned long dpid, const char* chat);
    void receiveChat(unsigned long dpid, char* chat,
                     bool inPopup);
    void displayChat();
    void onRequestHeroFaceMsg(CNetMsg* netMsg,
                              bool inPopup);
    void onRequestHeroFaceReplyMsg(CNetMsg* netMsg,
                                   bool inPopup);
    void onPingMsg(CNetMsg* netMsg);
    void onPingResponseMsg(CNetMsg* netMsg, unsigned char inPopup);
    void getHeroFace(int which, CNetPlayerHandlerPlayer* player);
    void onSetAGRMsg(CNetMsg* netMsg, bool inPopup);
    void onNewHostMsg(CNetMsg* netMsg);
    void onUpdatePlayerPosMsg(CNetMsg* netMsg);
    void checkFaces();
    // Always returns 1 (retail sets al on every exit); DC agrees.
    unsigned char onMapFileNameMsg(CNetMsg* netMsg);
    bool onHeaderConfirmMsg(CNetMsg* netMsg);
    bool onReqHeaderConfirmMsg(CNetMsg* netMsg);
    bool onMapHeaderRequestMsg(CNetMsg* netMsg);
    unsigned char checkMissingHeaders(unsigned long dpidHost);
    void turnOffScenarioOptions();
    void turnOffAdvancedOptions();
    bool onClickMsg(CNetMsg* netMsg);
    void turnChatOn(bool update);
    void turnChatOff(unsigned char update);
    void onTownUpdateMsg(CNetMsg* netMsg, bool inPopup);
    void updateNameLists();
    void updateTown(int pos, TTownType town, unsigned char inPopup);
    void setNewPlayerSlot(CNetPlayerInfo* playerInfo);
    void setupLoadGameMode();
    void setupNewGameMode();
    int getCommonGameVersion();
    // Retail 0x583580: copies the selected header's planes into
    // gpGame (+0x1f6a0 header band, +0x4df18 setup band) - the DC
    // UpdateGameVars role (dc 0x139090, void()). Called after the
    // header transfer completes.
    void updateGameVars();
    unsigned char beginSavedGame();
    bool beginNewGame();
    void updateMainWindow();
    // The disk header reader family around it, visible only to the
    // owning TU (the vectors gate): GetHeaders scans the picked
    // directory ("random_maps"/"games"/"maps" by mode) into the lists;
    // GetHeader fills one row's header temp from (dir, filename) -
    // retail widened DC's (cFilename, pHeader) with the dir argument
    // its chdir dance needs.
    int getFileSpecNbr();
    void getHeaders(std::vector<GameSelectionHeadersStruct>* headers);
    void windowFn00582e90(
        std::vector<GameSelectionHeadersStruct>* headers);
    int getHeader(char* dir, char* filename,
                  GameSelectionHeadersStruct* header);
    void setupScenarioOptions(unsigned char randomMaps);
    void setupAdvancedOptions();
    void setupFilterOptions();
    void createFilterWidgets();
    // Complete-only member at 0x580430. Its body rebuilds the map header and
    // player slots from field_18A0, redraws, and broadcasts the resulting
    // setup. The role name remains provisional until its body is claimed.
    void rebuildFilteredPlayerSetup();
    unsigned char sendPlayerPositions(unsigned long dpidTo);
    unsigned char sendSetupInfo(unsigned long dpid);
    bool isHost();
    void sendPlayerFaces();
    bool isMultiPlayer();
    void showWidget(int id);
    void turnOffFilterOptions();
    int calcPosition(int playerPos);

private:
    CNetPlayerHandlerPlayer* getThisPlayer();
};
SIZE(TSingleSelectionWindow, 0x1970);

// Cross-TU cells used by advmgr's SaveGame and the selection window.
// game.h owns the inCampaign DATA claim; the other cells retain their
// provisional address-based names.
//   0x69fc2c  the chosen save filename (empty = the dialog was cancelled)
//   0x691268  the extension scratch SaveGame sprintf's (.GM%d / .CGM)
//   0x69774c  campaign-game byte: picks the .CGM extension
//   0x697774  set to 1 the moment a save filename is committed
extern char g_unnamed69fc2c[];
extern char g_unnamed691268[];
extern bool g_inCampaign;
extern int g_unnamed697774;

#endif  /* HOMM3_SINGLESELECTIONWINDOW_H */
