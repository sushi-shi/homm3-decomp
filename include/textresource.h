#ifndef HOMM3_TEXTRESOURCE_H
#define HOMM3_TEXTRESOURCE_H

#include "va.h"

#include <vector>

#include "resource.h"

// Named indices into genrltxt.txt. Every value is retail-byte-proven by the
// corresponding TTextResource::Text[index] consumer; names describe those
// consumers until the original source roster supplies stronger wording.
enum EGeneralTextIndex {
    GENERAL_TEXT_SHUTDOWN = 1,
    GENERAL_TEXT_LEVEL_UP_OR = 5,
    // The dismiss-this-stack confirmation TViewArmyWindow::WindowHandler
    // raises from the DISMISS button (a folded [Text._First + 0x34]).
    GENERAL_TEXT_DISMISS_ARMY_PROMPT = 13,
    GENERAL_TEXT_PLAYER_TURN_FORMAT = 14,
    GENERAL_TEXT_HERO_ROLLOVER_FORMAT = 16,
    GENERAL_TEXT_RECRUIT_TITLE = 17,
    // The two combat morale lines, both "%s" formats over the affected
    // stack's name: combatManager::CheckApplyGoodMorale (0x464920) folds
    // [Text._First + 0x88] and CheckApplyBadMorale (0x464b40)
    // [Text._First + 0x8c]. Gated for the reason GENERAL_TEXT_SKELETON_
    // GOLD below is - an ungated enumerator counts toward the include-set
    // threshold in every consumer.
    GENERAL_TEXT_GOOD_MORALE = 34,
    GENERAL_TEXT_BAD_MORALE = 35,
    // The singular partner of GENERAL_TEXT_MIXED_ARMY below, and the one
    // consumer that proves the pair is a pair: combatManager::
    // damage_message (0x469a90) picks between the folded
    // [Text._First + 0xac] and [Text._First + 0xb0] on `deaths == 1`
    // when the dying stack has no army record to name itself from.
    // Gated for the reason GENERAL_TEXT_GOOD_MORALE above is - an
    // ungated enumerator counts toward the include-set threshold in
    // every consumer.
    GENERAL_TEXT_MIXED_ARMY_ONE = 43,
    GENERAL_TEXT_MIXED_ARMY = 44,
    // DoEventSkeleton (0x4a5480) shows this row - and nothing else in the
    // image does. The index is the folded `[Text._First + 0xbc]` load at
    // 0x4a5540, and a scan of every such load reachable from a
    // gpGeneralText reference finds exactly ONE, so the row has a single
    // consumer and the name can only describe it: the 1000 gold a Corpse
    // pays a hero whose sixty-four backpack slots are all full, wrapped in
    // the pooled "%s." format. Gated to the events view for the reason
    // GENERAL_TEXT_DRAGON_CITY_EMPTIED below is - an ungated enumerator
    // counts toward the include-set threshold in every consumer.
    GENERAL_TEXT_SKELETON_GOLD = 47,
    // TurnDurationMsg prefixes its caller-supplied warning with this row.
    GENERAL_TEXT_TURN_DURATION_PREFIX = 54,
    // town.obj's three event-reward rows: the " and " list separator
    // (folded [Text._First + 0x238] in both show_* helpers), and the
    // two town-event dialog formats give_event_reward's helpers wrap
    // around the reward list ([+0x92c] buildings, [+0x930] creatures).
    // Gated: an ungated enumerator counts toward the include-set
    // threshold in every consumer.
    GENERAL_TEXT_LIST_AND = 142,
    GENERAL_TEXT_EVENT_BUILDINGS = 587,
    GENERAL_TEXT_EVENT_CREATURES = 588,
    GENERAL_TEXT_SEARCH_NEEDS_FULL_MOVE = 57,
    GENERAL_TEXT_SEARCH_BACKPACK_FULL_FOUND = 58,
    GENERAL_TEXT_SEARCH_FOUND_FORMAT = 59,
    GENERAL_TEXT_SEARCH_NOTHING_FOUND = 60,
    GENERAL_TEXT_SEARCH_WATER = 61,
    GENERAL_TEXT_QUICK_INFO_SHROUDED = 62,
    GENERAL_TEXT_RESOURCE_DISPLAY_0 = 63,
    GENERAL_TEXT_RESOURCE_DISPLAY_1 = 64,
    GENERAL_TEXT_RESOURCE_DISPLAY_2 = 65,
    GENERAL_TEXT_QUIT = 70,
    // CDPlayHeroes::HandleLowLevelMsg's RS_PING_REPLY arm (0x552f9b) is the
    // only consumer of the folded [Text._First + 0x10c]: it sprintf()s the
    // round trip GameTime::ElapsedSince measured against the echoed ping
    // stamp into a 256-byte buffer and hands the line to ReceiveChat.
    GENERAL_TEXT_CHAT_PING_RESULT_FORMAT = 67,
    // advspells.obj's DimensionDoor (0x41d090) posts this when the
    // targeted square disagrees with the caster's boat bit - a folded
    // [Text._First + 0x11c]. The INDEX is byte-proven; the NAME is
    // role-based and PROVISIONAL, like its three neighbours below.
    GENERAL_TEXT_DIMENSION_DOOR_BLOCKED = 71,
    // SendChat's command/status rows. The indices are the folded retail
    // TTextResource loads; their roles are fixed by the surrounding ping
    // and recipient-control flow.
    GENERAL_TEXT_CHAT_PING_COMMAND = 73,
    GENERAL_TEXT_CHAT_PING_PLAYER_FORMAT = 74,
    GENERAL_TEXT_CHAT_PING_ALL = 75,
    // CDPlayHeroes::SendIt shows this two-button row after six failed send
    // attempts and retries only when the window returns ACCEPT.
    GENERAL_TEXT_DPLAY_SEND_RETRY = 82,
    GENERAL_TEXT_SEARCH_NOT_DIGGABLE = 98,
    GENERAL_TEXT_MAIN_MENU_CD_GENERIC_FORMAT = 107,
    GENERAL_TEXT_QUICK_INFO_INVALID_POINT = 111,
    // combatManager::DoCommand (0x476bd0) shows this entry instead of
    // opening the spell book while the acting side's field_54b4 latch
    // is set - i.e. the hero has already cast this round. The NAME
    // describes that consumer, which is this enum's stated convention;
    // the index is retail-byte-proven (a folded [Text._First + 0x204]).
    GENERAL_TEXT_COMBAT_SPELL_ALREADY_CAST = 129,
    // The Visions arm of advManager::CastSpell posts this line after
    // raising the caster's own visions level ([Text._First + 0x108]).
    GENERAL_TEXT_VISIONS_CAST = 66,
    // The Fly arm refuses here when the caster is aboard a boat
    // ([Text._First + 0x304]). Both indexes retail-byte-proven; names
    // describe the consumers.
    GENERAL_TEXT_SPELL_NOT_WHILE_ON_BOAT = 193,
    // advManager::SummonBoat's four outcome lines, all folded
    // [Text._First + N] loads in one body: 0x538 when the caster is already
    // at sea (sprintf'd with the hero's name), 0x53c when no adjacent water
    // tile is free, 0x540 when neither a summonable boat nor a new one can
    // be produced, and 0x544 when the mastery roll fails (also name-fed).
    // Indexes are retail-byte-proven; the names describe the consumers.
    GENERAL_TEXT_SUMMON_BOAT_ALREADY_AT_SEA_FORMAT = 334,
    GENERAL_TEXT_SUMMON_BOAT_NO_WATER = 335,
    GENERAL_TEXT_SUMMON_BOAT_NONE_AVAILABLE = 336,
    GENERAL_TEXT_SUMMON_BOAT_FAILED_FORMAT = 337,
    // advManager::TownGate's three refusals, all folded [Text._First + N]
    // loads in one body: 0x1f0 when the chosen town already has a visiting
    // hero, 0x1f4 when the caster's team owns no town at all, and 0x220
    // when the caster is aboard a boat (hero flags & 0x40000). Indexes are
    // retail-byte-proven; the names describe those three consumers.
    GENERAL_TEXT_TOWN_PORTAL_TOWN_OCCUPIED = 124,
    GENERAL_TEXT_TOWN_PORTAL_NO_TOWN = 125,
    GENERAL_TEXT_SPELL_NOT_FROM_BOAT = 136,
    // DimensionDoor's movement gate refuses here when the caster has no
    // movement points left ([Text._First + 0x1f8]). Name provisional.
    GENERAL_TEXT_SPELL_NEEDS_MOVEMENT = 126,
    // ResetRound posts this line after every non-placement, non-quick round;
    // retail folds Text._First + 0x674, i.e. row 413.
    GENERAL_TEXT_COMBAT_ROUND = 413,
    GENERAL_TEXT_SYSTEM_OPTIONS_AUDIO_UNAVAILABLE = 151,
    GENERAL_TEXT_BACKPACK_FULL = 153,
    // HandleCombatPlayerDrop's two 15-second notification rows.
    GENERAL_TEXT_COMBAT_LOCAL_PLAYER_DROPPED = 416,
    GENERAL_TEXT_COMBAT_REMOTE_PLAYER_DROPPED = 417,
    GENERAL_TEXT_BACKPACK_ARTIFACT_FORMAT = 154,
    // get_tower_string's two folded vector loads at Text._First + 0x26c and
    // +0x270. The first takes the wall name; the second takes that name,
    // skill and the double/triple archer counts.
    GENERAL_TEXT_COMBAT_WALL_DESTROYED_FORMAT = 155,
    GENERAL_TEXT_COMBAT_WALL_STATUS_FORMAT = 156,
    GENERAL_TEXT_VIEW_ARMY_SPEED = 194,
    // The two rows above HEALTH_REMAINING in the popup, both folded
    // [Text._First + N] loads: 0x31c in
    // TViewArmyWindow::create_shots_widget (0x5f5b30) and 0x320 in
    // create_damage_widget (0x5f5860).
    GENERAL_TEXT_VIEW_ARMY_SHOTS = 199,
    GENERAL_TEXT_VIEW_ARMY_DAMAGE = 200,
    GENERAL_TEXT_VIEW_ARMY_HEALTH_REMAINING = 201,
    GENERAL_TEXT_ARMY_HELP_PREFIX = 203,
    GENERAL_TEXT_LEVEL_UP_SINGLE_CHOICE = 204,
    GENERAL_TEXT_LEVEL_UP_CHOICE = 205,
    // The upgrade-this-stack confirmation, shown with the gold slot and
    // the highest non-gold resource the upgrade costs (a folded
    // [Text._First + 0x340] in TViewArmyWindow::WindowHandler).
    GENERAL_TEXT_UPGRADE_ARMY_PROMPT = 208,
    // kb's MemError (0x4f42c0) formats this entry into gText and hands
    // the result to ShutDown: the allocation-failure line.
    GENERAL_TEXT_OUT_OF_MEMORY = 223,
    GENERAL_TEXT_ARMY_ENTRY_SEPARATOR = 238,
    GENERAL_TEXT_QUICK_CREATURE_JOIN = 244,
    GENERAL_TEXT_QUICK_CREATURE_JOIN_COST = 245,
    GENERAL_TEXT_QUICK_CREATURE_FLEE = 246,
    GENERAL_TEXT_QUICK_CREATURE_ATTACK = 247,
    GENERAL_TEXT_SEARCH_BACKPACK_FULL = 248,
    GENERAL_TEXT_SPLIT_CREATURE_ROLLOVER = 257,
    GENERAL_TEXT_SPLIT_OTHER_ROLLOVER = 258,
    // The single-button dialog CDPlayHeroes::HandleLowLevelMsg (0x552e92)
    // raises when the RS_DESTROY_PLAYER order names this machine's own DPID:
    // the folded [Text._First + 0x524], shown after RemoteCleanup and
    // immediately before ShutDown(0).
    GENERAL_TEXT_REMOTE_SESSION_DESTROYED = 329,
    GENERAL_TEXT_QUICK_INFO_DIGGABLE = 331,
    // The one-vararg format advManager::SkuttleBoat (0x41cdf0) sprintf's
    // the caster's name into when the mastery roll fails
    // ([Text._First + 0x548]) - the row immediately below DimensionDoor's
    // own limit format. Name provisional.
    GENERAL_TEXT_SCUTTLE_BOAT_FAILED_FORMAT = 338,
    // A one-vararg format DimensionDoor sprintf's the caster's name into
    // when dWalkSpellsCast has reached this mastery's cap
    // ([Text._First + 0x54c]). Name provisional.
    GENERAL_TEXT_DIMENSION_DOOR_LIMIT_FORMAT = 339,
    // The hero screen's "Level %d %s" line (widget 0x8c): a folded
    // [Text._First + 0x55c] in THeroScreenWindow::SetupHeroView, fed the
    // hero's level and the class name HeroFn_004D8F70 picks. The INDEX is
    // byte-proven; the NAME describes the two arguments retail feeds it.
    GENERAL_TEXT_HERO_LEVEL_CLASS_FORMAT = 343,
    // The seer hut's display line: a folded [Text._First + 0x570] fed the
    // hut's own name out of seerhut.obj's name list. Three consumers, all
    // in seerhut.obj - the two TSeerHut text builders (0x5741b0 and
    // 0x5743e0) and the nullary 0x574070. The INDEX is byte-proven; the
    // NAME describes the one argument retail feeds it.
    GENERAL_TEXT_SEER_HUT_NAME_FORMAT = 348,
    GENERAL_TEXT_VISITED_OBJECT = 353,
    GENERAL_TEXT_UNVISITED_OBJECT = 354,
    GENERAL_TEXT_KNOWN_SHRINE_SPELL = 355,
    GENERAL_TEXT_SHRINE_SPELL_FORMAT = 356,
    GENERAL_TEXT_WITCH_SKILL_FORMAT = 357,
    GENERAL_TEXT_HERO_KNOWS_WITCH_SKILL = 358,
    GENERAL_TEXT_AI_GIFT_RECEIVED = 359,
    GENERAL_TEXT_AI_SINGLE_RESOURCE_REQUEST = 360,
    GENERAL_TEXT_AI_MULTIPLE_RESOURCE_REQUEST = 361,
    // The tactics-phase help dialog combatManager::Open (0x462a20)
    // raises once per player, behind that player's own
    // placement_help_enabled flag which it then clears - a folded
    // [Text._First + 0x5d4] and the only load of that row in the image.
    // The index is byte-proven; the name describes the one consumer,
    // since the TXT resources are not in this tree.
    GENERAL_TEXT_COMBAT_PLACEMENT_HELP = 373,
    // The wraith's mana-drain line, in its two counts. Both are folded
    // [Text._First + N] loads in combatManager::SetNextArmy (0x465330)
    // - 0x5d8 on `numTroops == 1` and 0x5dc otherwise - and both are
    // fed (the draining stack's name, the drained hero's name).
    GENERAL_TEXT_COMBAT_MANA_DRAIN_ONE = 374,
    GENERAL_TEXT_COMBAT_MANA_DRAIN_MANY = 375,
    // The attacker NAME combatManager::KeepAttack (0x465ad0) hands to
    // damage_message for an arrow tower's shot - a folded
    // [Text._First + 0x5e0], and the only load of that row in the image.
    // It is passed with a count of 1, so it is a singular noun rather
    // than a format. The index is byte-proven; the name describes the
    // one consumer, since the TXT resources are not in the image.
    GENERAL_TEXT_COMBAT_ARROW_TOWER_ATTACKER = 376,
    // The four rows combatManager::damage_message (0x469a90) builds its
    // line out of, every index a folded [Text._First + N] load in that
    // one body: 0x5e4/0x5e8 are the damage clause, selected on
    // `attacker_qty == 1` and both fed (attacker name, damage); 0x5ec is
    // the one-death clause, fed the dying stack's name alone, and 0x5f0
    // the many-death clause, fed (deaths, name). Gated for the reason
    // GENERAL_TEXT_GOOD_MORALE above is.
    GENERAL_TEXT_COMBAT_DAMAGE_ONE_ATTACKER = 377,
    GENERAL_TEXT_COMBAT_DAMAGE_MANY_ATTACKERS = 378,
    GENERAL_TEXT_COMBAT_ONE_DEATH = 379,
    GENERAL_TEXT_COMBAT_MANY_DEATHS = 380,
    GENERAL_TEXT_VIEW_ARMY_HEALTH = 389,
    // do_event_dragon_city (0x4a2140) shows this row - and nothing else in
    // the image does. The index is the folded `[Text._First + 0x6a4]` load
    // at 0x4a2183, and a scan of every `mov r32,[r32+0x6a4]` in .text finds
    // exactly ONE, so the row has a single consumer and the name can only
    // describe it: the line a hero gets on a Dragon Utopia whose cell
    // already carries the emptied bit 0x2000000. The generic creature bank
    // (0x4a15a0) tests the same bit on the same dword but formats
    // advevent.txt row 33 with the bank's own name instead, so the two are
    // NOT shared. Gated to the events view: no other modeled consumer
    // proves the value, and an ungated enumerator counts toward the
    // include-set threshold - winmgr.h's DIALOG_RETURN_DECLINE sets that
    // precedent.
    GENERAL_TEXT_DRAGON_CITY_EMPTIED = 425,
    GENERAL_TEXT_LEVEL_UP_TITLE_FORMAT = 445,
    GENERAL_TEXT_LEVEL_UP_HERO_FORMAT = 446,
    GENERAL_TEXT_PUZZLE_WINDOW = 464,
    GENERAL_TEXT_DEFAULT_PLAYER_NAME = 469,
    GENERAL_TEXT_PLAYER_DROPPED = 470,
    GENERAL_TEXT_CHAT_NONHUMAN_WIRE_TAG = 474,
    GENERAL_TEXT_CHAT_NONHUMAN_LINE_TAG = 475,
    // OnPlayerDropUpdateMsg displays this row while reloading the shared
    // recovery save. Retail fixes it at [Text._First + 0xa4c].
    GENERAL_TEXT_PLAYER_DROP_RELOAD = 659,
    GENERAL_TEXT_SYSTEM_OPTIONS_COMMAND_CONFIRM = 579,
    // The four spell-influence rollover rows TViewArmyWindow's spell
    // icons print, all folded [Text._First + N] loads in its
    // WindowHandler: 0x98c carries the spell name and a turn count,
    // 0xaa0 the same name with a fixed descriptor instead, and
    // 0xaa4/0xaa8/0xaac are that descriptor for the three spells whose
    // effect has no turn count.
    GENERAL_TEXT_ARMY_SPELL_ROUNDS_FORMAT = 611,
    // damage_message's third death clause, and the only [Text._First +
    // 0xa70] load in the image. It replaces the one/many pair above
    // when the defending stack carries creatureId bit 6 - the same bit
    // SideIsWipedOut (0x465830) and IsWinner (0x4658b0) read as "this
    // stack is out of the fight" - and it takes the stack's name alone,
    // with no count. Gated for the reason GENERAL_TEXT_GOOD_MORALE is.
    GENERAL_TEXT_COMBAT_STACK_WIPED_OUT = 668,
    GENERAL_TEXT_COMBAT_SPELL_DAMAGE_LOWERED_ONE = 545,
    GENERAL_TEXT_COMBAT_SPELL_DAMAGE_LOWERED_MANY = 546,
    GENERAL_TEXT_COMBAT_SPELL_DAMAGE_RAISED_ONE = 547,
    GENERAL_TEXT_COMBAT_SPELL_DAMAGE_RAISED_MANY = 548,
    GENERAL_TEXT_ARMY_SPELL_FOREVER_FORMAT = 680,
    GENERAL_TEXT_ARMY_SPELL_BIND = 681,
    GENERAL_TEXT_ARMY_SPELL_BERSERK = 682,
    GENERAL_TEXT_ARMY_SPELL_DISRUPTING_RAY = 683,
    GENERAL_TEXT_GARRISON_ADVENTURE_SPELL = 685,
    GENERAL_TEXT_MAIN_MENU_LOW_DISK = 708,
    GENERAL_TEXT_COMBAT_FEAR = 729,
    GENERAL_TEXT_MAIN_MENU_CD_DRIVE_FORMAT = 730,
    // The refusal both adventure-targeting spells share when their popup
    // comes back without a usable square: DimensionDoor (0x41d090) and
    // SkuttleBoat (0x41cdf0) both fold [Text._First + 0xb70]. Name
    // provisional.
    GENERAL_TEXT_ADVENTURE_SPELL_NO_TARGET = 732,
    GENERAL_TEXT_CAMPAIGN_HERO_CLASS = 736,
    GENERAL_TEXT_MAIN_MENU_CD_DRIVE_6 = 746,
    GENERAL_TEXT_MAIN_MENU_CD_DEFAULT_ARGUMENT = 747,
    GENERAL_TEXT_CURSED_GROUND_HIGH_LEVEL_SPELL = 748,
    GENERAL_TEXT_MAIN_MENU_CD_DRIVE_5 = 763
};

// PROVEN layout (retail InitializeCampaignMapTraitsTable 0x45dee0):
// the text pointer vector begins at +0x1c and its Dinkumware _First
// member is loaded from +0x20. This is the same retail vector layout as
// TSpreadsheetResource above. The names are Dreamcast-attested; Data's
// +0x2c position follows the adjacent vector/data members used by both
// text-resource variants.
class TTextResource : public resource {
public:
    typedef std::vector<char*> TTextArray;
    TTextResource();
    TTextResource(const char* name, int size, const char* data);
    virtual ~TTextResource();
    virtual unsigned int getSize() const;
#include "inline/textresource_get_text.inl"
#include "inline/textresource_index.inl"

private:
    TTextArray m_text;  // +0x1c (_First +0x20)
    char* m_data;  // +0x2c
};
SIZE(TTextResource, 48);

// PROVEN layout (retail monframeinfo parser 0x50c810/0x50ca00): the
// Spreadsheet row vector sits at +0x1c on the resource base - VC6
// Dinkumware vector, so _First lands at +0x20 and _Last at +0x24
// (size()'s null-check ternary appears verbatim at 0x50c82c; the
// Dreamcast build's 12-byte STLport vector has no such check) - and
// rows index *4, so the elements are heap TStringVector pointers.
// Dreamcast type 0x1a41 names the members (Spreadsheet@28, Data@40
// with the STLport layout; Dinkumware puts them at 0x1c/0x2c, sizeof
// 0x30 vs the DC 44). GetNumberOfRows/GetRow are the TextResource.h
// header inlines (dc 0x5088c/0x508a4), inlined into callers by /Ob2.
class TSpreadsheetResource : public resource {
public:
    typedef std::vector<char*> TStringVector;
    typedef std::vector<TStringVector*> TArray;
    TSpreadsheetResource();
    TSpreadsheetResource(const char* name, int size, const char* data);
    virtual ~TSpreadsheetResource();
    virtual unsigned int getSize() const;
    int getNumberOfRows() const { return m_spreadsheet.size(); }
    // Original: TSpreadsheetResource::GetNumberOfColumns; TextResource.h:113, dc 0x162910.
    int getNumberOfColumns(int row) const { return m_spreadsheet[row]->size(); }
    // DC TextResource.h:120/124 (text.obj:0x162934) returns const char* and
    // indexes the row and cell vectors directly. High-score defaults call
    // this accessor; their char* table entries require the explicit cast.
    const char* getSpreadsheet(int r, int c) const {
        return (*m_spreadsheet[r])[c];
    }
    const TStringVector& getRow(int r) const { return *m_spreadsheet[r]; }

private:
    TArray m_spreadsheet;  // +0x1c (_First +0x20, _Last +0x24)
    char* m_data;  // +0x2c
    int m_dataSize;  // +0x30, retail constructor stores size here
};
SIZE(TSpreadsheetResource, 52);

// DC ?GameText@@3PBVTTextResource@@B proves a pointer-to-const resource.
extern const TTextResource* g_generalText;  // retail .data 0x6a5d5c

#endif  /* HOMM3_TEXTRESOURCE_H */
