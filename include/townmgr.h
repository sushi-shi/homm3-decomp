#ifndef HOMM3_TOWNMGR_H
#define HOMM3_TOWNMGR_H

#include "advmgr_popup.h"
#include "basemgr.h"
#include "remote.h"
#include "terrain_type.h"

struct type_point;

unsigned char doTavern();
void doMapTavern(type_point point);

// The nine columns of the thieves' guild table, in the order
// SetupThievesGuild builds them and GetCategoryStats dispatches them.
// The Dreamcast dump declares no enum for this domain - both bodies pass
// a bare int - so only the NAMES are provisional; the values are the
// retail jump-table order.
enum EThievesGuildStat {
    TG_STAT_TOWNS = 0,
    TG_STAT_HEROES = 1,
    TG_STAT_GOLD = 2,
    TG_STAT_WOOD_ORE = 3,
    TG_STAT_RARE_RESOURCES = 4,
    TG_STAT_OBELISKS = 5,
    TG_STAT_ARTIFACTS = 6,
    TG_STAT_ARMY_STRENGTH = 7,
    TG_STAT_INCOME = 8
};

// The three states townManager::SetupMage sorts a mage-guild slot into
// before it drives the page's two widget runs. The values are retail's
// literals (0x3e7 is compared directly and 1 is stored into the icon
// frame); the NAMES are provisional, the Dreamcast dump declaring no
// enum for this domain either.
enum EMageGuildSlotState {
    // The slot holds a real spell: its frame stays drawn and the scroll
    // takes that spell's icon.
    MAGE_SLOT_KNOWN = 0,
    // The guild reaches this slot but the town has not rolled a spell
    // into it - frame drawn on icon 1, scroll hidden.
    MAGE_SLOT_EMPTY = 1,
    // The slot is past this guild level's width: both runs hidden.
    MAGE_SLOT_ABSENT = 999
};

class armyGroup;
class Bitmap816;
class border;
class CSprite;
class garrison;
class hero;
class iconWidget;
class textWidget;
class town;
class TResourceDisplay;

// IDENTIFIED 2026-08-14: the .bss cell recruit.h models as
// SUnnamed6aacb0 is the game's 16-bit SYSTEM PALETTE. CycleOutline
// walks the band data[128..134] out of it with an induction variable,
// and the offset VC6 strength-reduces that walk to - 0x11c stepping by
// 2 to 0x12a, `mov cx,[eax+edx]` with the pointer RELOADED every
// iteration - is exactly TPalette16's resource head (0x1c) plus 2*i,
// which places that struct's backColors[31] at data[0..30]. The array
// view is declared HERE, under this file's narrowest gate, rather than
// beside the struct in recruit.h: an extern added there moved
// recruitUnit::Update 90.84 -> 88.24 (the include-set wall), and this
// page is its only consumer.
class TPalette16;
extern TPalette16* g_systemPalette;  // retail .bss 0x6aacb0

// The "Town Outlines" preference cell (retail .bss 0x698784). misc.obj's
// registry pair binds it to szPrefTownOutlines at 0x63ff9c - the reader
// at 0x50b2b6 and the writer at 0x50b53c both name it, and two checkbox
// bindings take its address - which is what fixes both the role and the
// int width. townObject::Draw is the only consumer outside misc.obj, so
// the declaration sits HERE rather than beside gbShowSubtitles (0x698780)
// in prefs.h: townmgr.obj does not include that header, and pulling it in
// for one dword would widen this compiland by the whole 29-member
// preference block. Definition and DATA claim stay with misc.obj, whose
// carved span does not reach 0x698784.
extern int g_townOutlines;  // retail .bss 0x698784

// One drawable object of the town panorama - a building, its outline
// and its hotspot - forty-four slots of them on the manager at +0x5c.
// The Dreamcast fieldlist names the members and retail confirms every
// offset in it, with NO shift (unlike townManager itself): Draw
// 0x5c2ff0 reads currFrame@4, x@8, y@0xc, visible@0x18, objId@0x1c and
// objIcon@0x20; CycleOutline 0x5d6910 reads objId and objOutline@0x24;
// and UnloadTown unhooks objBorder@0x2c from the town window before
// freeing the object. Forty-eight bytes, no vtable - the destructor the
// manager runs is inlined, which is why no vptr store exists.
class townObject {
public:
    // Draw's two hard-coded animation bands, both on the Dungeon's
    // EXTRA_0 sprite: the mage-guild-5 variant cycles frames 10..19 and
    // the plain one 0..9, so 10 is the lit band's first frame and 20 the
    // index it wraps at. Retail spells all three as literals and no
    // roster attests a name - these are role names for the two bounds
    // that are not 0.
    enum EGuildFrameBand {
        GUILD_LIT_FIRST_FRAME = 10,
        GUILD_LIT_FRAME_END = 20
    };
    int m_numFrames;  // +0x00
    int m_currFrame;  // +0x04
    int m_x;  // +0x08
    int m_y;  // +0x0c
    int m_w;  // +0x10
    int m_h;  // +0x14

    townObject(int townType, int objPos, const char* basename);
    int m_visible;  // +0x18
    int m_objId;  // +0x1c
    CSprite* m_objIcon;  // +0x20
    Bitmap816* m_objOutline;  // +0x24
    Bitmap816* m_objHotspot;  // +0x28
    border* m_objBorder;  // +0x2c
    // The DC roster gives it its own row (dc 0x16a1a4, 128 SH4 bytes)
    // and retail has NO out-of-line body for it: townmgr.obj's first
    // carve row is the constructor at 0x5c2ea0 (the `%s.def` sprintf +
    // GetSprite body), and the only expansion in the image is the one
    // UnloadTown 0x5c70b0 carries. Declared here, DEFINED in townmgr.cpp
    // - the four members it tears down need border.h, csprite.h and
    // bitmap816.h complete, and none of them may enter this header.
    ~townObject();
    // Retail 0x5c2ff0 (dc 0x16a2b0), reconstructed in townmgr.cpp.
    // CycleOutline's two expansions of the town-redraw block are the
    // arity evidence: thiscall plus two pushed 1s.
    void drawOutline();
    void drawHotspot();
    void draw(int incFrame, unsigned char drawHotspots);
};
SIZE(townObject, 0x30);

// The command domain townManager::DoCommand 0x5d4c10 dispatches on, and
// the one place in this compiland that needs names: retail's switch is a
// DENSE ten-entry jump table at 0x5d5124 whose slot 6 points at the
// default arm, so the value space is 0..9 with SIX unused. The Dreamcast
// passes a bare int and declares no enum, so every NAME here is
// provisional and describes what the arm does; the VALUES are the jump
// table's own order.
enum ETownCommand {
    TOWN_COMMAND_SELECT_SLOT = 0,
    TOWN_COMMAND_VIEW_ARMY = 1,
    TOWN_COMMAND_MERGE_ARMY = 2,
    TOWN_COMMAND_SWAP_ARMY = 3,
    TOWN_COMMAND_VIEW_HERO = 4,
    TOWN_COMMAND_SPLIT_ARMY = 5,
    TOWN_COMMAND_SWAP_HEROES = 7,
    TOWN_COMMAND_MOVE_HERO_FROM_GARRISON = 8,
    TOWN_COMMAND_MOVE_HERO_TO_GARRISON = 9
};

// The panorama's four per-town-type tables, all in townmgr.obj's own
// .rdata band and all read by SetupTown 0x5c6870. Names INVENTED (no DC
// symbol covers them) and the extents come off SetupTown's own index
// arithmetic; every one of them is declared under this file's narrowest
// gate, the include-set reason gSystemPalette's note above gives. The
// extents are spelled as literals rather than as town.h's
// TOWN_TYPE_COUNT / MAX_BUILDING_TYPE because this header is parsed
// BEFORE town.h in its one consumer's include order.

// gTownBuildOrder: `movsx esi,byte [i + type*44 + 0x642eb4]` - a signed
// char [9][44] of type_building_id values in PANORAMA DRAW ORDER, each
// row closed by -1 (Castle's is 26,23,7,8,9,0,... then six -1s).
extern signed char g_townBuildOrder[9][44];
// gTownBackgroundPrefix: the "%sBack.pcx" stem, one per faction -
// TBCs, TBRm, TBTw, TBIn, TBNc, TBDn, TBSt, TBFr, TBEl.
extern const char* g_townBackgroundPrefix[9];
// gTownBuildingSprites: `[objId + type*44]` scaled by four - a
// char*[9][44] of the .def stem per faction and building (Castle's row
// starts TBCsmage, TBCsmag2, TBCsmag3, TBCsmag4, TBCsmag5, TBCstvrn).
extern const char* g_townBuildingSprites[9][44];
// gTownMusic: the town page's MP3 per faction - CstleTown, Rampart,
// TowerTown, InfernoTown, necroTown, dungeon, StrongHold,
// FortressTown, ElemTown.
extern const char* g_townMusic[9];

// The compiland's dialog family. Every one of these classes is fixed by
// a vtable of its own, each the slot-0 owner of one 33-byte scalar
// deleting destructor and each stored by the 107-byte destructor that
// wrapper calls: 0x643764 TThievesGuildWindow, 0x6437a0 THallWindow,
// 0x6437dc TMageGuildWindow, 0x643818 / 0x643854 / 0x643890 the three
// garrison windows, 0x6438cc TBlacksmithWindow, 0x643908 TShipWindow,
// 0x643944 TBuyBuildWindow, 0x643980 TTavernWindow, 0x6439bc
// TCastleWindow - the order the Dreamcast roster declares them in, with
// 0x643980 independently anchored by the Open/Close pair below.
// Only the destructor is declared for each: no constructor is
// reconstructed yet, so no member below CAdvPopup's 0x60 is attested
// except where a destructor reads one, and no size is asserted.

// The town screen's own window, and the one member of the family that is
// NOT a CAdvPopup: its vtable 0x64372c is 9 slots, the plain heroWindow
// width, and its destructor runs ~heroWindow rather than ~CAdvPopup.
class TTownScreenWindow : public heroWindow {
public:
    enum {
        EXIT_BUTTON_ID = 0x7800
    };
    enum {
        // Widget 50: townManager::SetCommandAndText's 0x32 arm reads a
        // rollover cell for it, but no admitted town-screen constructor
        // creates the id and the DC EWidgetIDs roster below starts at
        // 100 - domain unresolved, neutral ordinal name.
        TOWN_WIDGET_50_ID = 0x32,
        // The failed hotspot remap's sentinel: SetCommandAndText's
        // switch carries it as a real case (the -1-biased dispatch
        // table proves it), sharing the default's empty line.
        TOWN_HOTSPOT_NONE = -1,
        // The build-cheat latch's domain (gUnnamed67832c, drained by
        // Main): 100 = build everything the town may ever build; 0x37
        // is a value past MAX_BUILDING_TYPE that Main's eligibility
        // test admits unconditionally - both compares are retail's,
        // names INVENTED.
        TOWN_CHEAT_BUILD_ALL = 100,
        TOWN_CHEAT_BUILD_EXTRA = 0x37
    };
    // The guild-count ladder SetupThievesGuild maps onto category
    // counts (2/4/6/8/9 rows for 1/2/3/4/5+ guilds), and the player
    // column bound its skip-disabled scan runs to. Names INVENTED -
    // the values are the retail compares.
    enum EThievesGuildCounts {
        GUILD_COUNT_2 = 2,
        GUILD_COUNT_3 = 3,
        GUILD_COUNT_4 = 4,
        GUILD_PLAYER_COLUMNS = 8
    };
    // The DC field list's nested enum (LF_ENUM 0x71A1, 85 enumerators),
    // transcribed whole: every id townManager::SetCommandAndText
    // dispatches on is named here, and the values are the widget ids
    // the constructor builds (100 crest, 164+i / 172+i bonus row...).
    enum EWidgetIDs {
        CREST_ID = 100,
        TOWN_GARRISON_0_ID = 101,
        TOWN_GARRISON_1_ID = 102,
        TOWN_GARRISON_2_ID = 103,
        TOWN_GARRISON_3_ID = 104,
        TOWN_GARRISON_4_ID = 105,
        TOWN_GARRISON_5_ID = 106,
        TOWN_GARRISON_6_ID = 107,
        TOWN_GARRISON_0_TEXT_ID = 108,
        TOWN_GARRISON_1_TEXT_ID = 109,
        TOWN_GARRISON_2_TEXT_ID = 110,
        TOWN_GARRISON_3_TEXT_ID = 111,
        TOWN_GARRISON_4_TEXT_ID = 112,
        TOWN_GARRISON_5_TEXT_ID = 113,
        TOWN_GARRISON_6_TEXT_ID = 114,
        TOWN_GARRISON_0_SELECTOR_ID = 115,
        TOWN_GARRISON_1_SELECTOR_ID = 116,
        TOWN_GARRISON_2_SELECTOR_ID = 117,
        TOWN_GARRISON_3_SELECTOR_ID = 118,
        TOWN_GARRISON_4_SELECTOR_ID = 119,
        TOWN_GARRISON_5_SELECTOR_ID = 120,
        TOWN_GARRISON_6_SELECTOR_ID = 121,
        GARRISON_PORTRAIT_ID = 122,
        GARRISON_PORTRAIT_SELECTOR_ID = 123,
        PORTRAIT_ID = 124,
        VISITING_PORTRAIT_SELECTOR_ID = 125,
        HERO_ARMY_0_ID = 126,
        HERO_ARMY_1_ID = 127,
        HERO_ARMY_2_ID = 128,
        HERO_ARMY_3_ID = 129,
        HERO_ARMY_4_ID = 130,
        HERO_ARMY_5_ID = 131,
        HERO_ARMY_6_ID = 132,
        HERO_ARMY_0_TEXT_ID = 133,
        HERO_ARMY_1_TEXT_ID = 134,
        HERO_ARMY_2_TEXT_ID = 135,
        HERO_ARMY_3_TEXT_ID = 136,
        HERO_ARMY_4_TEXT_ID = 137,
        HERO_ARMY_5_TEXT_ID = 138,
        HERO_ARMY_6_TEXT_ID = 139,
        HERO_ARMY_0_SELECTOR_ID = 140,
        HERO_ARMY_1_SELECTOR_ID = 141,
        HERO_ARMY_2_SELECTOR_ID = 142,
        HERO_ARMY_3_SELECTOR_ID = 143,
        HERO_ARMY_4_SELECTOR_ID = 144,
        HERO_ARMY_5_SELECTOR_ID = 145,
        HERO_ARMY_6_SELECTOR_ID = 146,
        PANORAMA_ID = 147,
        TOWN_BOTTOM_ID = 148,
        TOWN_NAME_ID = 149,
        TOWN_PORTRAIT_ID = 150,
        TOWN_TEXT_ID = 151,
        TOWN_UP_ARROW_ID = 152,
        TOWN_DOWN_ARROW_ID = 153,
        DIVIDE_ID = 154,
        TOWN_0_ID = 155,
        TOWN_1_ID = 156,
        TOWN_2_ID = 157,
        HALL_ICON_ID = 158,
        CASTLE_ICON_ID = 159,
        INCOME_TEXT_ID = 160,
        CREST_ICONS = 161,
        HERO_ICONS = 162,
        SELECTOR_ID = 163,
        BONUS_0_ID = 164,
        BONUS_1_ID = 165,
        BONUS_2_ID = 166,
        BONUS_3_ID = 167,
        BONUS_4_ID = 168,
        BONUS_5_ID = 169,
        BONUS_6_ID = 170,
        BONUS_7_ID = 171,
        BONUS_0_TEXT_ID = 172,
        BONUS_1_TEXT_ID = 173,
        BONUS_2_TEXT_ID = 174,
        BONUS_3_TEXT_ID = 175,
        BONUS_4_TEXT_ID = 176,
        BONUS_5_TEXT_ID = 177,
        BONUS_6_TEXT_ID = 178,
        BONUS_7_TEXT_ID = 179,
        TOWN_POP_ID = 180,
        HALL_DOWN = 181,
        HALL_UP = 182,
        GARRISON_ID = 183,
        NUM_TOWN_BUTTONS = 2
    };
    // Dreamcast TTownScreenWindow::topTown at +0x44 shifts to PC
    // +0x4c. Locator indexing and both scroll buttons corroborate the role.
    int m_topTown;  // +0x4c  town-list scroll offset (UpdateTownLocators)
    // +0x50: the panorama's hotspot buffer, one word per pixel, freed
    // with plain operator delete. DC name and type (zBuffer,
    // T_32PUSHORT); SetCommandAndText reads the word under the mouse
    // as the building hotspot id.
    unsigned short* m_zBuffer;
    // +0x54 and +0x74: the growth-bonus row the constructor builds in one
    // eight-iteration loop, each widget kept by the window as it is made.
    // Names are the DC field list's (growth_bonus_icon / growth_bonus_text
    // at DC offsets 76/108; the old resourceIcons/resourceTexts names were
    // invented).
    iconWidget* m_growthBonusIcon[8];
    textWidget* m_growthBonusText[8];
    // +0x94: the creature shown in each bonus slot, seeded -1 and written
    // by set_bonus_display (DC `bonus_creatures`, offset 140). BYTE-PROVEN
    // by townManager::Open's `push 0xb4` operator-new size (the class ends
    // at 0x94 without it) and by set_bonus_display's eight-dword -1 fill
    // at +0x94. The DC element type is TCreatureType; spelled int because
    // armygrp.h is outside this header's include closure and the enum's
    // loads/stores are int-identical under VC6.
    int m_bonusCreatures[8];
    TTownScreenWindow();
    virtual ~TTownScreenWindow();
    void updateTownLocators();
    void doTownKnob(unsigned char up);
    void bonusRightClick(long id);
    // Retail 0x5c5b40 (dc 0x16ad04). The faction-bonus panel of the
    // page's bottom row. townManager::UpdateTownInfo 0x5c66d0 is its
    // only caller in the image and hands it the town being shown.
    void setBonusDisplay(town* currentTown);

private:
    void updateTownLocator(int i);
};

class TThievesGuildWindow : public CAdvPopup {
public:
    // The one non-literal widget id the constructor uses; the rest of
    // this dialog's widgets sit in the 40/600/700/800/900 bands and
    // stay literal. 0x7800 is the same dialog-button enumerator
    // TTavernWindow spells CANCEL_BUTTON_ID.
    enum {
        EXIT_BUTTON_ID = 0x7800
    };
    // The rank grid the rollover setter dispatches on: four category rows over
    // eight player columns, so the hovered cell id (codeY) is category*10 plus
    // the column. Only the first three rows go through SetRolloverText's
    // compressed switch; the fourth (30..37) is handled by a range test, and
    // gaps 9/18/19 fall to the empty line.
    enum {
        RANK_A0 = 1, RANK_A1, RANK_A2, RANK_A3,
        RANK_A4, RANK_A5, RANK_A6, RANK_A7,
        RANK_B0 = 10, RANK_B1, RANK_B2, RANK_B3,
        RANK_B4, RANK_B5, RANK_B6, RANK_B7,
        RANK_C0 = 20, RANK_C1, RANK_C2, RANK_C3,
        RANK_C4, RANK_C5, RANK_C6, RANK_C7,
        // The two portrait rows the right-click opens into a detail view: one
        // hero and one creature cell per player column.
        HERO_P0 = 0x2ee, HERO_P1, HERO_P2, HERO_P3,
        HERO_P4, HERO_P5, HERO_P6, HERO_P7,
        CREATURE_P0 = 0x352, CREATURE_P1, CREATURE_P2, CREATURE_P3,
        CREATURE_P4, CREATURE_P5, CREATURE_P6, CREATURE_P7
    };
    // +0x60: one game-position per player column, filled by SetupThievesGuild.
    // WindowHandler's right-click arms gate the hero/creature view on
    // owners[player] == GetLocalPlayerGamePos(), so only the local player's own
    // column opens the detail view.
    int m_owners[8];
    // +0x80: SetupThievesGuild's last act builds it -
    // `new TResourceDisplay(this, 1)` stored here and Update(0,0)'d -
    // and the destructor deletes it through slot 0 before the widget
    // list. Retyped from widget* 2026-08-27 (rename-free; the virtual
    // teardown is unchanged).
    // Role-derived name: setupThievesGuild creates TResourceDisplay
    // here, and the destructor deletes that same resource-strip object.
    TResourceDisplay* m_resourceDisplay;
    TThievesGuildWindow(int numGuilds);
    virtual ~TThievesGuildWindow();
    // Retail 0x5dda10, the compiland's second largest body and the
    // constructor's last statement. Declared, not reconstructed.
    void setupThievesGuild(int thievesGuilds);
    // Retail 0x5c9710 (dc 0x16e2f4). The page's rollover line.
    void setRolloverText(int codeY);
    virtual int windowHandler(message& msg) OVERRIDE;   // slot 9, 0x5c9930
};

// The town hall page: one background per town type over a grid of
// building slots the constructor lays out from two tables it builds on
// its own stack.
class THallWindow : public CAdvPopup {
public:
    enum {
        EXIT_BUTTON_ID = 0x7800
    };
    // ONE DWORD PAST CAdvPopup, and the allocation size is the whole
    // proof: townManager::DoHall (0x5d27b0) pushes 0x64 to operator new
    // for this class where CAdvPopup alone is 0x60. Neither the
    // constructor (0x5c9be0) nor the destructor (0x5cc910) reads or
    // writes it - the constructor's only `this`-relative stores stop at
    // CAdvPopup's own members and the destructor walks the widget list
    // and chains. The reconstructed castle.obj setupCastle also supplies
    // no access to +0x60. An external consumer is not established.
    // Allocation extent alone does not identify the slot's type or role.
    int m_field60;  // +0x60

    THallWindow(int which);
    virtual ~THallWindow();
    virtual int windowHandler(message& msg) OVERRIDE;
};

// The mage guild, one full-screen page of five spell rows: twenty frame
// icons over twenty scroll icons, both runs numbered by row. Its
// constructor writes nothing past CAdvPopup's 0x60.
class TMageGuildWindow : public CAdvPopup {
public:
    enum {
        EXIT_BUTTON_ID = 0x7800
    };
    TMageGuildWindow();
    virtual ~TMageGuildWindow();
    void setRolloverText(int codeY);
    virtual int windowHandler(message& msg) OVERRIDE;   // slot 9, 0x5ce370
};

class type_garrison_base_window : public CAdvPopup {
public:
    enum {
        BACKGROUND_ID = 148,
        DIVIDE_BUTTON_ID = 154,
        OK_BUTTON_ID = 0x7802,
        // The two seven-slot troop runs, the bottom strip's owner cell
        // and the dialog's cancel id - the four families the page's own
        // SetCommandAndText 0x5d05f0 and WindowHandler 0x5d0910 dispatch
        // on. Both runs are consecutive because both bodies index them
        // with `codeY - <base>`, and both bases are the strip `firstId`
        // townManager::NewStrips 0x5c6e10 hands the two strips (0x64 and
        // 0x7c) plus the strip's own selector offset. NAMES provisional,
        // VALUES retail's own jump-table bounds.
        TOP_SLOT_FIRST_ID = 0x73,
        BOTTOM_OWNER_ID = 0x7c,
        BOTTOM_SLOT_FIRST_ID = 0x8c,
        CANCEL_BUTTON_ID = 0x7800
    };
    // Sixteen bytes of its own past CAdvPopup's 0x60, and nothing in a
    // reconstructed body reads them. The extent is proven by the three
    // modal entry points that build a DERIVED window on the stack: each
    // reserves 0x70 of frame for it (object at [ebp-0x7c] under a
    // 12-byte EH record), and Widgets lands at the object's +0x34 in
    // every one, which fixes the base rather than the leaves.
    // +0x60: the constructor's first parameter, stored straight after
    // the vptr and read by no reconstructed body.
    // Dreamcast names thisHero at +0x58; retail constructor 0x5ce830
    // stores its inHero parameter at the corresponding PC +0x60.
    hero* m_thisHero;
    // +0x64 / +0x68, NAMED 2026-08-21 by the page's own handler
    // 0x5d0910: the hover pair the whole compiland's dialog family
    // carries, tested together and refreshed together before the status
    // line is rewritten - the same shape TTavernWindow's pair has. The
    // handler reaches them through msg->window, not through `this`.
    int m_lastHover;  // +0x64
    int m_lastQualifier;  // +0x68
    // +0x6c: a byte the base constructor clears and
    // type_monster_join_window's sets to 1; SetCommandAndText 0x5d05f0
    // rides it through both troop runs as select_army's third argument
    // and SetArmyCommand's second.
    unsigned char m_isJoinDialog;
    // Dreamcast is_join_dialog is a byte at +0x64; retail places it
    // at +0x6c. These three bytes align the 0x70-byte base extent.
    char m_tailPadding[3];
    type_garrison_base_window(hero* inHero, int garrisonOwner,
                              armyGroup& garrisonArmy);
    virtual ~type_garrison_base_window();

protected:
    virtual int windowHandler(message& msg) OVERRIDE;   // slot 9, 0x5d0910
    // Retail 0x5d05f0 (dc 0x172af0). The dialog's status line, and the
    // town page's pending command with it.
    void setCommandAndText(message* msg);
    void showText();
    void viewArmy();
};

// The two derived garrison windows have EMPTY destructors: retail inlines
// the base body into each and then drops the derived vptr store the
// inlined base immediately overwrites, which is why both bodies store
// 0x643818 even though 0x643854 and 0x643890 exist and hold their ??_G.
class type_monster_join_window : public type_garrison_base_window {
public:
    // +0x70..0x7b unattested; the constructor sets the byte at +0x6c,
    // which is inside the base's own sixteen-byte tail, so it is
    // declared there rather than here.

    // Three parameters, not the Dreamcast's two: both retail entry
    // points push a third literal 0 behind the army pointer.
    type_monster_join_window(hero* inHero, armyGroup* monsters,
                             unsigned char flags);

    // Implicit destructor; CodeView dc 0x181638 compgenx.
};

class TGarrisonWindow : public type_garrison_base_window {
public:
    TGarrisonWindow(hero* inHero, int garrisonOwner, armyGroup& garrisonArmy);

    // Implicit destructor; CodeView dc 0x181684 compgenx.
};

// The blacksmith, which sells one war machine per town type. Its
// constructor 0x5d1360 writes all three members past CAdvPopup's 0x60.
class TBlacksmithWindow : public CAdvPopup {
public:
    enum {
        CANCEL_BUTTON_ID = 0x7801,
        BUY_BUTTON_ID = 0x7802
    };
    // +0x60, the widget the cursor was last over. Read and written by
    // WindowHandler 0x5d1c60 and by NOTHING else - the constructor never
    // initialises it, which is retail's own behaviour and not an
    // omission here: the first hover simply compares against whatever
    // the allocation left behind.
    int m_lastHover;  // +0x60
    int m_townType;  // +0x64  selects the machine and its artifact
    int m_field68;  // +0x68  cleared by the constructor
    iconWidget* m_machineIcon;  // +0x6c

    TBlacksmithWindow(int heroID, int inTownType);
    virtual ~TBlacksmithWindow();
    void setRightClickText(int id);
    void setRolloverText(int id);
    virtual int windowHandler(message& msg) OVERRIDE;   // slot 9, 0x5d1c60
};

// The shipyard dialog. Constructor 0x5d1ef0 writes both members past
// CAdvPopup's 0x60: +0x60 is cleared and nothing else in the compiland
// reads it yet, and +0x64 keeps the boat picture the dialog shows.
class TShipWindow : public CAdvPopup {
public:
    enum {
        CANCEL_BUTTON_ID = 0x7801,
        BUY_BUTTON_ID = 0x7802
    };
    // +0x60, the boat animation's current frame. The constructor clears
    // it and WindowHandler 0x5d25a0 is the only other body that touches
    // it: it steps the frame on every message and wraps at the boat
    // sequence's own length.
    int m_boatFrame;  // +0x60
    iconWidget* m_boatIcon;  // +0x64

    TShipWindow(int type);
    virtual ~TShipWindow();
    void setRightClickText(int codeY);
    void setRolloverText(int codeY);
    virtual int windowHandler(message& msg) OVERRIDE;   // slot 9, 0x5d25a0
};

// The "build this?" confirmation popup. Both members past CAdvPopup's
// 0x60 are written by the constructor 0x5d55c0: the rollover text widget
// it keeps a handle on at +0x60, and the building id it is asking about
// at +0x64 - the latter stored immediately after the vptr, which is the
// member-initializer-list slot.
class TBuyBuildWindow : public CAdvPopup {
public:
    // The two widget ids the constructor and the handler share. Both are
    // in the compiland's 0x78xx dialog-button family; the rest of this
    // dialog's widgets are numbered 1..9 and stay literal.
    enum {
        CANCEL_BUTTON_ID = 0x7801,
        BUY_BUTTON_ID = 0x7802
    };
    textWidget* m_rolloverText;  // +0x60
    int m_buildingId;  // +0x64

    TBuyBuildWindow(int x2, int y2, int id);
    virtual ~TBuyBuildWindow();
    void setRolloverText(int codeY);
    void setRightClickText(int codeY);
    void setPrerequisiteText(const town* currentTown, int building);
    virtual int windowHandler(message& msg) OVERRIDE;   // slot 9, 0x5d6810
};

// The tavern chooser. Its vtable 0x643980 is 15 slots wide - the CAdvPopup
// width - and the two slots reconstructed are the ones retail overrides to
// bracket the dialog with the tavern's background video.
class TTavernWindow : public CAdvPopup {
public:
    // The page's widget ids, all of them proven by the two bodies below:
    // SetRolloverText 0x5d7920 dispatches on exactly this set and
    // WindowHandler 0x5d7b30 acts on three of them. The two portraits are
    // consecutive because both bodies index playerData::recruits with
    // `id - RECRUIT_0_ID`, and the lit/dim pair the handler broadcasts
    // (`slot + 8` and `9 - slot`) is the same two-wide family one band up.
    // NAMES provisional - no roster covers this dialog's ids - VALUES
    // retail's own.
    enum {
        RECRUIT_0_ID = 5,
        RECRUIT_1_ID = 6,
        THIEVES_GUILD_BUTTON_ID = 11,
        HIRE_BUTTON_ID = 12,
        RUMOR_PANEL_ID = 15,
        CANCEL_BUTTON_ID = 0x7800
    };
    // +0x60, the recruit slot the page has selected - 0 or 1, the index
    // of the portrait that was clicked. The constructor clears it and
    // both bodies below read it as the index into playerData::recruits.
    // Role-derived name: the portrait click stores slot 0 or 1, and
    // the hire/detail paths index the local player's recruits with this value.
    int m_selectedRecruit;  // +0x60  cleared by the constructor
    // +0x64 / +0x68, NAMED 2026-08-21 by WindowHandler 0x5d7b30: the
    // hover pair the whole compiland's dialog family carries, tested
    // together and refreshed together before the rollover line is
    // rewritten. Same shape as TBlacksmithWindow's lastHover, with the
    // qualifier alongside it because this page's rollover changes on a
    // right-click as well as on a move.
    int m_lastHover;  // +0x64
    int m_lastQualifier;  // +0x68
    textWidget* m_rolloverText;  // +0x6c

    TTavernWindow(int x2, int y2);
    virtual ~TTavernWindow();
    void setRolloverText(int id);
    virtual int open(int zOrder, unsigned char update);  // slot 1
    virtual void close(unsigned char update);            // slot 2
    virtual int windowHandler(message& msg) OVERRIDE;    // slot 9, 0x5d7b30
};

// The fort page: one row per creature dwelling - a separator strip, a
// faction background, the building picture out of the town's own hall
// .def, the creature's animation, a name, a growth count and six cost
// columns - laid out two rows across and four rows down. The fourth
// row carries two dwellings only when Dungeon's Portal of Summoning
// has an external dwelling to show, and one centred dwelling
// otherwise; `use8` is that switch, re-read before every row that has
// a bottom slot.
class TCastleWindow : public CAdvPopup {
public:
    // The page's widget-id families. Every family is EIGHT wide - one id
    // per dwelling row, the eighth being the summoning portal's - and the
    // constructor below builds each of them in a run at a fixed y. The
    // handler dispatches the two CLICKABLE families: the row frame and
    // the six stat labels. The stat VALUE families (0x29, 0x31, 0x69,
    // 0x71, 0x79, 0x81) and the name/growth lines (0x19, 0x21) carry no
    // case of their own.
    enum {
        // `border(x, y, 386, 126, id, 1)` - the whole row's hit box.
        ROW_FRAME_ID = 0x11,
        // The six stat-column captions, top to bottom (gpGeneralText
        // 191 / 192 / 200 / ... at x = 300 and 694).
        ROW_STAT_LABEL_1_ID = 0x39,
        ROW_STAT_LABEL_2_ID = 0x41,
        ROW_STAT_LABEL_3_ID = 0x49,
        ROW_STAT_LABEL_4_ID = 0x51,
        ROW_STAT_LABEL_5_ID = 0x59,
        ROW_STAT_LABEL_6_ID = 0x61,
        // The eighth member of every family - the summoning portal row,
        // which recruits through the generic dialog instead of ::Recruit.
        ROW_SUMMONING_OFFSET = 7,
        EXIT_BUTTON_ID = 0x7800
    };
    // The resource bar's OWN ids, produced by TResourceDisplay's
    // constructor (resourcedisplay.cpp): seven amount lines at
    // RESOURCE_TEXT_ID + i plus the status line at RESOURCE_TEXT_ID + 7,
    // and seven borders at RESOURCE_BORDER_ID + i. The bar is a
    // subwindow of the town page, so its clicks arrive here, and both
    // bands index the same shared name/description table - which is why
    // the two arms differ only in the addend they subtract.
    enum {
        RESOURCE_TEXT_ID = 0x3e9,
        RESOURCE_BORDER_ID = 0x3f1
    };
    // +0x60. The Dreamcast fieldlist names it `use8` at its own 92,
    // four bytes below SpriteWidget's 100 and forty-four below
    // castleType's 132 - exactly this row's 0x60/0x68/0x88.
    unsigned char m_use8;
    // The preceding byte field and following four-byte field establish
    // this alignment gap; the reference layout retains the same boundary.
    char m_paddingBeforeCastleBank[0x3];
    // +0x64, DC `CastleBank` (its own 96, the same uniform four below
    // ours). TYPED by ::Recruit, which calls
    // ?Update@TResourceDisplay@@QAEXEE@Z through it to refresh the fort
    // page's resource bar after a purchase; the constructor does not
    // write it, so the window is handed the bar from outside.
    TResourceDisplay* m_castleBank;
    TCastleWindow();
    virtual ~TCastleWindow();
    virtual int windowHandler(message& msg) OVERRIDE;   // slot 9, 0x5dcf80

private:
    // +0x68..+0x87, DC `SpriteWidget`: the eight dwelling animations.
    // The constructor fills all eight BEFORE pushing any of them - the
    // eighth goes into the widget list on its own and a seven-trip
    // loop pushes the rest.
    iconWidget* m_spriteWidget[8];
    // +0x88, DC `castleType` - the fort tier whose name the page
    // header prints. int rather than the DC's type_building_id for the
    // reason town.h's get_horde gives: this header does not see that
    // enum, and an enum member would need a cast at the one read.
    int m_castleType;
    // Retail 0x5dcbf0. NOT virtual: 0x5dcbf0 appears in no vtable and in
    // no .rdata cell image-wide, and its one caller (the page's own
    // WindowHandler at 0x5dd2f9) reaches it with a direct call.
    void showText();
    void setRolloverText(message* msg);
    // Retail 0x5dce50, the fort page's buy button for row `i`.
    void recruit(int i);
};

class town;
class strip;
class townObject;
class heroWindow;
class widget;
class bitmapBorder16;
class CSprite;
class TResourceDisplay;
// SetupWell's argument. Defined above under the window gate; consumers
// that take the manager alone (recruit.obj, town.obj) only need the name.
class TCastleWindow;
// DoHall's net-handler hand-over target. Defined above under the window
// gate; the manager only ever holds a pointer to it.
class CTownNetMsgHandler;

// Layout proven store-for-store by townManager::townManager 0x5c3310,
// which writes every member below, and corroborated by ::UnloadTown
// 0x5c70b0 (the two arrays and their counts), ::Close 0x5c71b0 (the two
// owned windows) and ::ResetStrips 0x5d5530 (the two hovered strips -
// it stores -2 into strip::current at +0x2c through both). The tail is
// modelled only as far as retail proves it, so no size is asserted; the
// unattested run at +0x144 stays an opaque pad. The three virtuals are
// the three slots of vtable 0x643720.
// Shared table owned by text.cpp; also exposed to town value accessors.
extern const char* g_townTypeNames[10];

class townManager : public baseManager {
public:
    // Original: townManager::SetTown; TownMgr.h:686, dc 0x168e24.
    // town::view0x5be210 expands the assignment to Complete's +0x38 field.
    void setTown(town* townToView) { m_townToView = townToView; }

    // Original: townManager::TownNativeTerrains; ten entries including
    // the neutral town type -1. Complete retains the table at 0x643694.
    static const TTerrainType s_townNativeTerrains[10];

    // Original: townManager::GetTownTypeName; TownMgr.h:738, dc 0x20280
    static const char* getTownTypeName(int type)
    {
        return g_townTypeNames[type + 1];
    }

    // Original: townManager::GetNativeTerrain; TownMgr.h:745, dc 0x4cc8c.
    static TTerrainType getNativeTerrain(int type)
    {
        return s_townNativeTerrains[type + 1];
    }

    town* m_townToView;  // +0x38
    // +0x3c: the panorama background, a bitmapBorder16 -
    // UpdateTownInfo builds it with `new bitmapBorder16(0, 0, 800,
    // 374, 147, gText, 0x800)` and Main paces its animation with the
    // non-virtual Draw2. Retyped from widget* 2026-08-27 (rename-free:
    // every existing site assigns or deletes it).
    // Dreamcast panorama at +0x44; retail background construction and
    // animation corroborate the corresponding PC pointer at +0x3c.
    bitmapBorder16* m_panorama;  // +0x3c
    // +0x40, NAMED AND TYPED 2026-08-14. The Dreamcast fieldlist's
    // `MonPix` is an LF_ARRAY of seven CSprite* at its own 72, which is
    // this offset under the same -8 shift that puts currTown/panorama/
    // objects/numObjects at +0x38/+0x3c/+0x5c/+0x10c. Retail agrees at
    // both ends: SetupTown 0x5c6870 fills the seven slots from
    // GetSprite 0x55c7b0 (one per dwelling row, indexed through
    // currentDwellingIDOff), and UnloadTown 0x5c70b0 drives each
    // non-null one through vtable slot 1 - CSprite::Dispose - without
    // freeing it, which is exactly how a shared resource is released.
    CSprite* m_monPix[7];
    // +0x5c: the town's building objects, count at +0x10c. UnloadTown
    // walks exactly this many, unhooks each one's widget at +0x2c from
    // the town window and frees the object - which is ~townObject
    // inlined, so the array is townObject* and the count is its own.
    townObject* m_townObjects[44];
    int m_townObjectCount;  // +0x10c
    int m_loadedTownType;  // +0x110  ctor -1
    int m_saveWin;  // +0x114
    heroWindow* m_townWindow;  // +0x118
    // Dreamcast garrisonStrip/heroStrip/currStrip/currIndex/srcStrip/
    // srcIndex/destStrip/destIndex are the consecutive +0x128..+0x144
    // slots; retail retains this sequence at +0x11c..+0x138. newStrips
    // (0x5c6e10) binds garrison and visiting armies. selectArmy (0x5c8080)
    // records the current pair; doCommand (0x5d4c10) latches the source
    // and merges/splits/swaps it into the destination pair.
    strip* m_garrisonStrip;  // +0x11c
    strip* m_heroStrip;  // +0x120
    // +0x124 / +0x128, NAMED AND TYPED 2026-08-14 by select_army
    // 0x5c8080: it stores the strip that was clicked and the slot
    // inside it, in that order, before anything else it does.
    strip* m_currStrip;  // +0x124
    int m_currIndex;  // +0x128  ctor -1
    strip* m_srcStrip;  // +0x12c
    int m_srcIndex;  // +0x130  ctor -1
    strip* m_destStrip;  // +0x134
    int m_destIndex;  // +0x138  ctor -1
    // +0x13c: the town page's resource bar. Typed by recruitUnit::Close
    // (0x550344), which calls ?Update@TResourceDisplay@@QAEXEE@Z through
    // it; the manager constructs it, owns it and deletes it in ::Close.
    TResourceDisplay* m_resourceDisplay;  // +0x13c
    // +0x140, TYPED 2026-08-14 by DoHall 0x5d27b0: it deletes whatever
    // is here through vtable slot 0, puts a fresh
    // `TResourceDisplay(hallWindow, 1)` in its place, calls Update on
    // it and hands it to the net handler - the same bar as +0x13c, but
    // the one a modal page owns for as long as it is up.
    TResourceDisplay* m_dialogResourceDisplay;  // +0x140
    // +0x144, NAMED 2026-08-14: RedrawTownScreen 0x5d5410 hands its
    // ADDRESS to the page's status widget as a message's extraText, so
    // the run is a buffer this object owns, not padding. The extent is
    // still only bounded by the next proven member at +0x194.
    char m_statusText[0x50];  // +0x144
    // +0x194 / +0x198, NAMED 2026-08-14 from the Dreamcast fieldlist
    // (lastHover@416, lastQualifier@420 - this pair under the same -8
    // shift, less the four bytes retail dropped with townMenu). The
    // fort page's WindowHandler 0x5dcf80 is the reader: it refreshes
    // the rollover line only when the pair actually changes.
    int m_lastHover;  // +0x194  ctor -1
    int m_lastQualifier;  // +0x198  ctor -1
    // Dreamcast command at +0x1a8 maps to PC +0x19c; Main and the
    // garrison handler pass this action code directly to doCommand.
    int m_command;  // +0x19c  ctor -1
    // +0x1a0/+0x1a8: Dreamcast townManager fields `canBuyMask` and
    // `canBuildMask`, both T_QUAD. SetupCastle independently proves the
    // Complete layout: it clears each pair of dwords, ORs affordable
    // buildings into the first, and stores town::get_buildable_mask in the
    // second. The -0x10 retail shift is the same one already established by
    // lastHover/lastQualifier and currentDwellingIDOff.
    __int64 m_canBuyMask;  // +0x1a0
    __int64 m_canBuildMask;  // +0x1a8
    // +0x1b0, NAMED AND TYPED 2026-08-14 by DoHall 0x5d27b0, which
    // writes the page's resource bar into +0xc of this object twice -
    // once on the way in and once on the way out. That is
    // CTownNetMsgHandler::SetResourceDisplay against the +0xc this
    // header already proves is that class's bar, so the object is the
    // town's net handler and Close's delete goes through its slot 0.

    // Naming the type here needs the forward declaration above, and
    // that is NOT free: ANY forward declaration added to recruit.cpp's
    // view of this header costs recruitUnit::Update 90.84 -> 88.24 -
    // measured, and name-independent (a dummy `class HOMM3_PROBE_ZZ;`
    // moves it by exactly the same amount), which sharpens the standing
    // note that bare forward declarations are inert for the include-set
    // wall. They are not, for this canary. The room was bought in
    // recruit.cpp: button.h and border.h are two type definitions that
    // TU's own dialog constructor proves it needs, and with them the
    // handle stream absorbs one declaration here. +0x1bc keeps the base
    // pointer for the same budget reason - a second declaration would
    // spend room that buys no bytes, DoHall being byte-exact without it.
    CTownNetMsgHandler* m_netMsgHandler;  // +0x1b0
    // +0x1b4: the previous CDPlayHeroes message handler, saved by Open
    // through GetNetMsgHandler (0x5537a0) and restored by Close through
    // SetNetMsgHandler (0x553770).
    CNetMsgHandler* m_netMsgHandlerSave;
    int m_objToBuild;  // +0x1b8  ctor -1
    // +0x1bc, NAMED AND TYPED 2026-08-14: DoHall 0x5d27b0 stores its
    // `new THallWindow(townToView->type)` here and drives the whole
    // modal run - DrawWindow, DoModal, delete - through it. Held as the
    // base pointer for the reason the note on +0x1b0 gives.
    heroWindow* m_hallWindow;  // +0x1bc
    int m_multiWin;  // +0x1c0
    int m_divideStatus;  // +0x1c4  ResetStrips
    char m_recruitSelected[0x4];  // +0x1c8  untouched by the retail bodies
    // Retail 0x5d8480 (dc 0x17b318). Inferno's Castle Gate.
    void doTownGate();
    void moveHero(town* fromTown, town* toTown);
    void updateTownInfo();
    void moveHeroFromGarrison();
    void drawTown(int update, int incFrame, unsigned char drawHotspots);
    void newStrips();
    void armyCommand(strip* whichStrip, int i, int shift, unsigned char joinDialog);
    void swapHeroes();
    void moveHeroToGarrison();
    // +0x1cc, seven bytes - the dwelling slot each of the fort page's
    // seven base rows is currently showing. TCastleWindow's
    // constructor uses each byte as the column of this town's 14-wide
    // gTownDwellingCreatures row. The DC fieldlist's last member is
    // `currentDwellingIDOff` at its own 476, which is this offset
    // under the same 0x10 shift that puts divideStatus/recruitSelected
    // at +0x1c4/+0x1c8.
    unsigned char m_currentDwellingIdOff[7];
    void resetStrips();
    void setCommandAndText(message* msg);
    void showText();
    void setArmyCommand(int splitEnabled, unsigned char joinDialog);
    void doCommand(int inCommand, unsigned char isGarrison,
                   type_garrison_base_window* garrisonWindow);
    void cycleOutline(const int objectIndex, const int x, const int y,
                      const int w, const int h);
    // Retail 0x5d5f30. Prices `buildingId`, puts up TBuyBuildWindow over
    // the town page and, if the player confirms, debits the cost.
    // `bQuickView` shows the panel read-only through DoQuickView instead
    // of running the dialog, and makes the result unconditionally 0.
    int buyBuild(int buildingId, int infoOnly, int quickView);
    // Retail 0x5d6a80. Commits a purchase to the town: builds it,
    // re-syncs every panorama object's visibility, fizzles the new
    // building in over the saved rectangle and cycles its outline.
    void buildObj(int buildingId);
    void redrawTownScreen();
    // castle.obj's, retail 0x461190 (dc 0x5c278) - the carve row
    // directly after castle.cpp's two claimed ones, and DoHall is its
    // only caller in the image. Declared here, defined over there.
    void setupCastle(heroWindow* inCasWin, int isReset);
    // Retail 0x5dd390. Fills the fort page: eight dwelling frames, their
    // names, populations and creature names, and the six creature-stat
    // columns. Takes the page it is filling, because it broadcasts every
    // one of those through the window rather than through the manager.
    void setupWell(TCastleWindow* wellWin);
    void setupMage(heroWindow* mageWin);
    townManager();
    void unloadTown();
    // Retail 0x5c6870 (dc 0x16bba4) and 0x5c77a0 (dc 0x16c940). Neither
    // is reconstructed; both are declared for DoTownGate below, which
    // expands townManager::ChangeTown inline and so has to name them.
    // Same gate, same measured reason, as SetupExtraStuff above.
    void setupTown(unsigned char fade);
    void setupExtraStuff();
    void changeTown(unsigned char fade);
    void createPopupBank(heroWindow* parent);
    void doPortalOfSummoning();

private:
    void doSkeletonTransformer();
    void doHall();
    void selectArmy(strip* fromStrip, long slot, unsigned char isOwnerCell);

public:
    // Retail 0x5d2da0, retail-only - the Dreamcast townmgr roster runs
    // straight from GetBuildingInfo to Main with nothing between them.
    // Conflux's Magic University: the page's hero, then either the
    // university window over the four elemental schools or the
    // building's own description when there is no hero to teach.
    void doUniversity();
    void handleMageGuildClick();
    void setHeroCommand();
    virtual int open(int newPriority) OVERRIDE;  // slot 0, 0x5c63c0
    virtual void close() OVERRIDE;  // slot 1, 0x5c71b0
    virtual int main(message& msg) OVERRIDE;
    void doTownTavern();

private:
    void handleHallClick();
};

extern townManager* g_townManager;

void doShipyard(int type);

// --- type_monster_join_window ---

// The shared frame-pacing stamp at .bss 0x698998. cmbtmgr.h owns the
// DATA claim (advmgr's Open/Main and drawing.cpp share the cell);
// townManager::Main paces the panorama animation with it.

#endif  /* HOMM3_TOWNMGR_H */
