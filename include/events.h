#ifndef HOMM3_EVENTS_H
#define HOMM3_EVENTS_H

#include <va.h>
#include "armygrp.h"  // SpellID, used by spell_level_order

class garrison;
class hero;

// E:\gamedcs\events.cpp:1883 (dc 0x9cdc0). The exchange dialog sorts
// spells by descending level, then alphabetically within a level. The body
// remains beside its original events.cpp source location so VC6 sees the
// ordinary inline member before the two std::sort instantiations.
struct spell_level_order {
    unsigned char operator()(SpellID first, SpellID second) const;
};

// townmgr.cpp's 0x5d1130. advManager::DispatchEvent's garrison arm is the
// one out-of-TU caller; the declarator lives HERE rather than in townmgr.h
// because townmgr.h rides in recruit.cpp's closure and one declarator there
// costs recruitUnit::Update (the known include-set-sensitive row, and
// recruit.cpp's own +0x1b0 note records the same trade).
void doEventGarrison(hero* inHero, garrison* thisGarrison);

// Retail .data 0x691208, the byte directly ahead of gUnnamed691209 (the
// "gosolo" handed-to-AI byte advmgr.h declares and documents). DoCombat's
// fast-path guard reads the two TOGETHER - `gUnnamed691209 &&
// gUnnamed691208` gates the quick-combat shortcut off - so this is the
// pair's second arm, plausibly "a replay/sub-battle is being watched".
// Name ordinal. Declared HERE rather than beside its sibling because
// advmgr.h rides in ~40 closures and events.h in two; DoCombat is the
// first consumer, so events.h holds the claim until the producer is
// decoded (the iCombatControlNetPos / command.h precedent).
DATA(0x00691208) extern unsigned char g_unnamed691208;

// DoCombat's two AI callees, declared HERE on the DoEventGarrison
// precedent above: their owning headers cannot enter events.cpp's
// closure (ai_combat.h drags ai_tactical.h, whose TSkillMastery model
// collides with herospec.h's), and a declarator in either would ride
// into every AI TU's closure besides. ai_combat.cpp / ai_player.cpp
// keep the defining claims.
class armyGroup;
class town;
class NewmapCell;

// The domain of combatManager::field_13d48 (+0x13d48), the post-combat
// verdict DoCombat returns and switches its finish-heroes teardown on:
// 0 = the left (attacking) side won, 1 = the right (defending) side won,
// -1 = nobody did (retreat / surrender / mutual destruction). Declared
// HERE rather than in cmbtmgr.h because three enumerators in that
// header's closure are exactly the include-set perturbation command's
// GetCommand row has twice measured; move them beside field_13d48 when
// the field's cmbtmgr writers reconstruct.
enum ECombatWinner {
    COMBAT_WINNER_NONE = -1,
    COMBAT_WINNER_LEFT = 0,
    COMBAT_WINNER_RIGHT = 1
};

// E:\gamedcs\events.cpp:6248/6261 (dc 0x9ce40 / 0x9ceb0) - the RAII
// turn-duration pause DoCombat holds across a whole battle. Both
// bodies are events.cpp's own (their DC lines are events.cpp lines).
class CTurnDurationPause {
public:
    CTurnDurationPause();
    ~CTurnDurationPause();
};

unsigned char aiQuickCombat(hero* attackingHero, hero* defendingHero,
                              armyGroup& defendingArmy, town* defendingTown,
                              NewmapCell* cell);
// Dreamcast ai_player.cpp:2817 proves the enemy reference parameter.
void splitArmies(hero* currentHero, const hero* enemyHero,
                  const armyGroup& enemy);

// Named indices into advevent.txt, the adventure-object text resource
// events.obj loads through InitializeAdventureEventText (0x49e0e0).
// Every value is retail-byte-proven by the [Text._First + 4*N] load in
// the handler named beside it; the names describe those consumers,
// which is the convention textresource.h's EGeneralTextIndex sets.
// The rows run ALPHABETICALLY by adventure object, which is what makes
// each object's lines contiguous and what places every triple below
// exactly where it falls: war school 158..160, warrior's tomb 161..163,
// water wheel 164/165, watering hole 166/167, whirlpool 168, windmill
// 169/170, witch hut 171..173.
enum EAdventureEventText {
    // DoEventArena (0x49e7d0), and this pair is the enum's anchor: rows 0
    // and 1, the very first in the file, because "arena" is the first
    // adventure object alphabetically. 0 is the iMBType-10 two-picture
    // choice (+2 Attack against +2 Defense) and 1 the already-fought line.
    ADV_EVENT_TEXT_ARENA = 0,
    ADV_EVENT_TEXT_ARENA_VISITED = 1,
    // The "no room in your backpack" line, byte-proven by the folded
    // [Text._First + 8] load at 0x4a5ec8 in DoEventSpellScroll (0x4a5ea0).
    // The ROLE is what names it: row 2 sits immediately after the arena's
    // pair rather than anywhere near an object whose name begins with a
    // letter, so it is a generic refusal rather than one object's line.
    ADV_EVENT_TEXT_BACKPACK_FULL = 2,
    ADV_EVENT_TEXT_ARTIFACT_WISDOM = 3,
    ADV_EVENT_TEXT_ARTIFACT_LEADERSHIP = 4,
    ADV_EVENT_TEXT_ARTIFACT_COST_2000 = 5,
    ADV_EVENT_TEXT_ARTIFACT_COST_2500 = 6,
    ADV_EVENT_TEXT_ARTIFACT_COST_3000 = 7,
    // The sprintf format BOTH customised guarded arms pay out with -
    // DoCustomSpellScroll (0x4a5a80) formats the spell name into it and
    // DoCustomArtifact (0x49f070) the artifact name - one row after the
    // artifact-cost block.
    ADV_EVENT_TEXT_CUSTOM_GUARDED = 8,
    // PayForArtifact (0x49ed50): the affordability refusal followed by
    // the line shown when the player declines the offered artifact.
    ADV_EVENT_TEXT_ARTIFACT_CANT_AFFORD = 12,
    ADV_EVENT_TEXT_ARTIFACT_DECLINED = 13,
    // The four "one permanent primary skill, once per hero" objects, and
    // each one is a PAIR: the reward line, then the already-visited line
    // straight after it. The rows land where the alphabet puts each
    // object, exactly as the war school / witch hut run below does.
    // DoEventBlackBox (0x4a0c50) - Pandora's Box. 14 is the yes/no prompt
    // asked with iMBType 2, 15 the line a box that paid nothing shows and
    // 16 the one its guardians get. All three are byte-proven by the
    // [Text._First + 0x38] / [+0x3c] / [+0x40] loads in that body. The
    // custom MESSAGE the box may carry is not a row at all - it comes out
    // of the record's own std::string.
    ADV_EVENT_TEXT_BLACK_BOX_PROMPT = 14,
    ADV_EVENT_TEXT_BLACK_BOX_NOTHING = 15,
    ADV_EVENT_TEXT_BLACK_BOX_GUARDED = 16,
    // The rows DispatchEvent (0x4a84f0) consumes directly - the arms whose
    // handlers were folded into the dispatcher itself. Every index is
    // byte-proven by a [Text._First + 4*i] load inside that body, and each
    // lands where the alphabet puts its object, corroborating the run:
    // border guard/tent 17..20, buoy 21/22, cartographer 24..28, clover
    // field 29/30, derelict ship 41..43, eye of the magi 48, faerie ring
    // 49/50, hut of the magi 61, lighthouse 69, monolith exit 70, mermaid
    // 82/83, obelisk 96/97, observatory 98, pillar of fire 99, sanctuary
    // 114, sepulcher 119..121, shipwreck 122..124, shrines 127..129,
    // thieves' den 142, blocked subterranean gate 153, war machine
    // factory 157 (directly before the war school's proven 158).
    ADV_EVENT_TEXT_BORDER_GUARD_PROMPT = 17,
    ADV_EVENT_TEXT_BORDER_GUARD_DENIED = 18,
    ADV_EVENT_TEXT_BORDER_TENT = 19,
    ADV_EVENT_TEXT_BORDER_TENT_VISITED = 20,
    ADV_EVENT_TEXT_BUOY = 21,
    ADV_EVENT_TEXT_BUOY_VISITED = 22,
    ADV_EVENT_TEXT_CARTOGRAPHER_VISITED = 24,
    ADV_EVENT_TEXT_CARTOGRAPHER_WATER = 25,
    ADV_EVENT_TEXT_CARTOGRAPHER_LAND = 26,
    ADV_EVENT_TEXT_CARTOGRAPHER_UNDERGROUND = 27,
    ADV_EVENT_TEXT_CARTOGRAPHER_NO_GOLD = 28,
    ADV_EVENT_TEXT_CLOVER_FIELD = 29,
    ADV_EVENT_TEXT_CLOVER_FIELD_VISITED = 30,
    ADV_EVENT_TEXT_DERELICT_PROMPT = 41,
    ADV_EVENT_TEXT_DERELICT_EMPTY = 42,
    ADV_EVENT_TEXT_DERELICT_TREASURE = 43,
    ADV_EVENT_TEXT_EYE_OF_MAGI = 48,
    ADV_EVENT_TEXT_FAERIE_RING = 49,
    ADV_EVENT_TEXT_FAERIE_RING_VISITED = 50,
    ADV_EVENT_TEXT_HUT_OF_MAGI = 61,
    ADV_EVENT_TEXT_LIGHTHOUSE = 69,
    ADV_EVENT_TEXT_LITH_EXIT = 70,
    ADV_EVENT_TEXT_MERMAID_VISITED = 82,
    ADV_EVENT_TEXT_MERMAID = 83,
    ADV_EVENT_TEXT_OBELISK = 96,
    ADV_EVENT_TEXT_OBELISK_VISITED = 97,
    ADV_EVENT_TEXT_OBSERVATORY = 98,
    ADV_EVENT_TEXT_PILLAR_OF_FIRE = 99,
    ADV_EVENT_TEXT_SANCTUARY = 114,
    ADV_EVENT_TEXT_SEPULCHER_PROMPT = 119,
    ADV_EVENT_TEXT_SEPULCHER_EMPTY = 120,
    ADV_EVENT_TEXT_SEPULCHER_TREASURE = 121,
    ADV_EVENT_TEXT_SHIPWRECK_PROMPT = 122,
    ADV_EVENT_TEXT_SHIPWRECK_EMPTY = 123,
    ADV_EVENT_TEXT_SHIPWRECK_TREASURE = 124,
    ADV_EVENT_TEXT_SHRINE1 = 127,
    ADV_EVENT_TEXT_SHRINE2 = 128,
    ADV_EVENT_TEXT_SHRINE3 = 129,
    ADV_EVENT_TEXT_THIEVES_DEN = 142,
    ADV_EVENT_TEXT_SUBTERRANEAN_BLOCKED = 153,
    ADV_EVENT_TEXT_WAR_FACTORY_PROMPT = 157,
    // DoEventCampfire (0x4a1120), the object's only row: one line carrying
    // BOTH payouts, the gold and the resource, as its two picture/quantity
    // pairs. 23 sits below the cover of darkness at 31, which is where the
    // alphabet puts "campfire".
    ADV_EVENT_TEXT_CAMPFIRE = 23,
    // DoEventCoverOfDarkness (0x4a14b0), the object's only row - it has
    // nothing to report but the fact of the visit. 31 sits below the
    // defense tower's 39, which is where the alphabet puts "cover of
    // darkness", and the DC roster agrees: DoEventCoverOfDarkness
    // (dc 0x92540) runs before DoEventDefenseTower (dc 0x92d40).
    ADV_EVENT_TEXT_COVER_OF_DARKNESS = 31,
    // DoEventSkeleton (0x4a5480) - the Corpse, whose per-player band
    // game.h names DeadGuyFlags. 37 is a sprintf FRAGMENT joined to the
    // artifact name by the pooled "%s %s", 38 the nothing-here line. The
    // pair sits between the cover of darkness at 31 and the defense tower
    // at 39/40, i.e. one rung ABOVE where a strict sort on "corpse" would
    // put it - the second wobble this enum records in the 31..47 window
    // (the refugee camp's 44 is the other), and unlike the 39..173 run the
    // low band is not in handler-address order either. Both indices are
    // byte-proven by the [Text._First + 0x94] / [+0x98] loads at
    // 0x4a54d4 / 0x4a55b6.
    ADV_EVENT_TEXT_SKELETON_ARTIFACT = 37,
    ADV_EVENT_TEXT_SKELETON_EMPTY = 38,
    // DoEventDefenseTower (0x4a2050); the reward line carries picture
    // 0x20, the +1 Defense icon.
    ADV_EVENT_TEXT_DEFENSE_TOWER = 39,
    ADV_EVENT_TEXT_DEFENSE_TOWER_VISITED = 40,
    // do_event_dragon_city (0x4a2140), asked with iMBType 2 - the yes/no
    // form whose answer comes back out of heroWindowManager::dialogReturn.
    // The utopia has only this ONE advevent.txt row: its already-emptied
    // line comes out of genrltxt.txt instead (textresource.h's
    // GENERAL_TEXT_DRAGON_CITY_EMPTIED). 47 lands between the defense
    // tower's pair at 39/40 and the fountain of youth's at 57/58, which is
    // where the alphabet puts "dragon city" and where the DC roster puts
    // do_event_dragon_city - between DoEventDefenseTower (dc 0x92d40) and
    // DoEventFaerieRing (dc 0x92f08). The two orderings agree again.
    ADV_EVENT_TEXT_DRAGON_CITY_PROMPT = 47,
    // DoEventIdol (0x4a12f0). ONE reward row serves all three arms - the
    // pictures are what differ - and 63 is the already-visited line.
    ADV_EVENT_TEXT_IDOL = 62,
    ADV_EVENT_TEXT_IDOL_VISITED = 63,
    // DoEventFlotsam (0x4a2230), and the flotsam is the only object in
    // this enum with FOUR contiguous rows besides the stables: its four
    // sizes each have their own line, 51 the empty one and 52..54 the
    // three payouts. The run sits between the dragon city's 47 and the
    // fountain of fortune's 55, which is where the alphabet puts
    // "flotsam".
    ADV_EVENT_TEXT_FLOTSAM_NOTHING = 51,
    ADV_EVENT_TEXT_FLOTSAM_WOOD = 52,
    ADV_EVENT_TEXT_FLOTSAM_WOOD_AND_GOLD = 53,
    ADV_EVENT_TEXT_FLOTSAM_LARGE = 54,
    // DoEventFountain (0x4a2480), the Fountain of FORTUNE - 55 is the
    // reward and 56 the already-drunk line, immediately before the
    // fountain of youth's own pair, which is exactly where the alphabet
    // puts the two and where the DC roster puts DoEventFountain
    // (dc 0x9312c) against DoEventFountainOfYouth (dc 0x93298).
    ADV_EVENT_TEXT_FOUNTAIN_OF_FORTUNE = 55,
    ADV_EVENT_TEXT_FOUNTAIN_OF_FORTUNE_VISITED = 56,
    // DoEventFountainOfYouth (0x4a25f0). The reward carries picture 0x0e,
    // the same morale icon do_event_watering_hole shows, and the pair is
    // reward-then-visited like the rest of this block.
    ADV_EVENT_TEXT_FOUNTAIN_OF_YOUTH = 57,
    ADV_EVENT_TEXT_FOUNTAIN_OF_YOUTH_VISITED = 58,
    // DoEventGarden (0x4a2710), picture 0x22 = +1 Knowledge.
    ADV_EVENT_TEXT_GARDEN = 59,
    ADV_EVENT_TEXT_GARDEN_VISITED = 60,
    // DoEventLeanTo (0x4a31a0). 64 is the payout - a resource picture and
    // its amount - and 65 the picked-clean line.
    ADV_EVENT_TEXT_LEAN_TO = 64,
    ADV_EVENT_TEXT_LEAN_TO_EMPTY = 65,
    // DoEventLibrary (0x4a3280). 66 is the reward, 67 the already-read
    // line and 68 the refusal a hero gets while `level + 2*diplomacy`
    // is under ten.
    ADV_EVENT_TEXT_LIBRARY = 66,
    ADV_EVENT_TEXT_LIBRARY_VISITED = 67,
    ADV_EVENT_TEXT_LIBRARY_UNWORTHY = 68,
    // DoEventMagicSchool (0x4a33e0) - the School of Magic - and its rows
    // sit under M, immediately before the magic spring's, which is where
    // the INTERNAL name puts it and not where the displayed one would.
    // 71 is the two-picture choice offered with iMBType 10 (+1 Spell Power
    // against +1 Knowledge, pictures 0x21 and 0x22), 72 the already-taught
    // line and 73 the "under 1000 gold" refusal - the same triple, in the
    // same roles, as the war school's 158..160.
    ADV_EVENT_TEXT_MAGIC_SCHOOL_CHOOSE = 71,
    ADV_EVENT_TEXT_MAGIC_SCHOOL_VISITED = 72,
    ADV_EVENT_TEXT_MAGIC_SCHOOL_NO_GOLD = 73,
    // DoEventMagicSpring (0x4a3590) and DoEventMagicWell (0x4a3730), two
    // contiguous triples in the same alphabetical run. Each spends one
    // row on the refill, one on the object being spent for the week or
    // the day, and one on a hero whose mana was already at its cap.
    ADV_EVENT_TEXT_MAGIC_SPRING = 74,
    ADV_EVENT_TEXT_MAGIC_SPRING_EMPTY = 75,
    ADV_EVENT_TEXT_MAGIC_SPRING_NO_ROOM = 76,
    ADV_EVENT_TEXT_MAGIC_WELL = 77,
    ADV_EVENT_TEXT_MAGIC_WELL_VISITED = 78,
    ADV_EVENT_TEXT_MAGIC_WELL_NO_ROOM = 79,
    // DoEventMercenaryCamp (0x4a38b0), picture 0x1f = +1 Attack.
    ADV_EVENT_TEXT_MERC_CAMP = 80,
    ADV_EVENT_TEXT_MERC_CAMP_VISITED = 81,
    // DoEventMine (0x4a39a0), and the mine is the one object in this enum
    // whose rows are SPLIT across the file. 84 is the prompt an unowned,
    // monster-guarded mine asks with iMBType 2 and 85 the line its cleared
    // tile shows; both sit between the mercenary camp's 80/81 and the
    // mystical garden's 92/93, which is where the alphabet puts "mine".
    // 187 is the same prompt for a mine that belongs to an ENEMY - a
    // single-consumer row out past the witch hut's block, so the ROLE is
    // what names it, as with WITCH_HUT_NO_SKILL at 190.
    ADV_EVENT_TEXT_MINE_GUARDED = 84,
    ADV_EVENT_TEXT_MINE_CLEARED = 85,
    ADV_EVENT_TEXT_MINE_DEFENDED = 187,
    // The wandering-stack outcome handlers, and the run is contiguous
    // because "monster" is a single alphabetical entry: 86 is the join
    // offer monsters_join (0x4a7000) asks with iMBType 2, 91 the
    // surrender line monsters_flee (0x4a6df0) asks the same way, and
    // both are sprintf formats taking the creature name alone. 87 is
    // SHARED by monsters_join and monsters_sell_out - the line either
    // refusal shows when the stack wanted to fight anyway. 88..90 are
    // monsters_sell_out's (0x4a7250) prompt, split by count: 88 is the
    // single-creature form taking (name, gold), while for a real stack
    // 89 is strcpy'd whole and 90 - taking (count, name, gold) - is
    // formatted into a 300-byte local and strcat'd onto it.
    ADV_EVENT_TEXT_MONSTERS_JOIN = 86,
    ADV_EVENT_TEXT_MONSTERS_INSULTED = 87,
    ADV_EVENT_TEXT_MONSTERS_SELL_OUT_ONE = 88,
    ADV_EVENT_TEXT_MONSTERS_SELL_OUT_LEAD = 89,
    ADV_EVENT_TEXT_MONSTERS_SELL_OUT_MANY = 90,
    ADV_EVENT_TEXT_MONSTERS_FLEE = 91,
    // DoEventMysticalGarden (0x4a3bc0). 92 is the payout, 93 the
    // already-harvested line.
    ADV_EVENT_TEXT_MYSTICAL_GARDEN = 92,
    ADV_EVENT_TEXT_MYSTICAL_GARDEN_EMPTY = 93,
    // DoEventOasis (0x4a3ca0), and this pair is REVERSED against every
    // other one in this enum: 94 is the already-visited line and 95 the
    // reward. Both indices are byte-proven by the arm that loads them.
    ADV_EVENT_TEXT_OASIS_VISITED = 94,
    ADV_EVENT_TEXT_OASIS = 95,
    // do_event_pyramid (0x4a4230), and the pyramid has the longest run in
    // this enum after the stables: 105 is the yes/no prompt asked with
    // iMBType 2, 106 the sprintf FRAGMENT the spell name is quoted onto
    // with the pooled "%s'%s'.", 107 the -2 luck line an already-robbed
    // pyramid shows with the luck picture TWICE, 108 the strcat'd tail for
    // a hero whose wisdom is too low and 109 the one for a hero with no
    // spellbook. The run sits between the power school's 100/101 and the
    // rally flag's 110/111, which is where the alphabet puts "pyramid".
    ADV_EVENT_TEXT_PYRAMID_PROMPT = 105,
    ADV_EVENT_TEXT_PYRAMID_SPELL = 106,
    ADV_EVENT_TEXT_PYRAMID_ROBBED = 107,
    ADV_EVENT_TEXT_PYRAMID_NO_WISDOM = 108,
    ADV_EVENT_TEXT_PYRAMID_NO_SPELLBOOK = 109,
    // DoEventRallyFlag (0x4a44c0), and this pair is REVERSED like the
    // oasis: 110 is the already-visited line and 111 the reward, which
    // shows the morale and luck pictures side by side.
    ADV_EVENT_TEXT_RALLY_FLAG_VISITED = 110,
    ADV_EVENT_TEXT_RALLY_FLAG = 111,
    // DoEventRefugeeCamp (0x4a4600), and this object is where the
    // alphabetical run BREAKS. Its offer line lands at 112, directly after
    // the rally flag's pair, exactly where "refugee camp" belongs - but its
    // empty line is 44, out in the "d" band between the defense tower's
    // pair at 39/40 and the dragon city's 47. Both indices are byte-proven
    // (the [Text._First + 0xb0] and [+0x1c0] loads at 0x4a462c/0x4a46aa)
    // and a full scan of every advevent.txt index reachable from
    // gpAdventureEventText shows 44 has exactly ONE consumer image-wide, so
    // it is not a shared generic row that happens to be borrowed. The ROLE
    // is what names it, as with WITCH_HUT_NO_SKILL at 190 below; only the
    // index is evidence.
    ADV_EVENT_TEXT_REFUGEE_CAMP_EMPTY = 44,
    ADV_EVENT_TEXT_REFUGEE_CAMP = 112,
    // DoEventSeaChest (0x4a5030): 116 is the empty chest, 117 the sprintf
    // format taking the artifact name - the only row in this enum whose
    // dialog shows THREE pictures, the artifact plus a 0x24/1000 pair -
    // and 118 the plain 1500-gold line. The run sits directly after the
    // scholar's 115, which is where the alphabet puts "sea chest".
    ADV_EVENT_TEXT_SEA_CHEST_EMPTY = 116,
    ADV_EVENT_TEXT_SEA_CHEST_ARTIFACT = 117,
    ADV_EVENT_TEXT_SEA_CHEST_GOLD = 118,
    // DoEventScholar (0x4a4dc0), and ONE row serves all three awards - the
    // pictures and their quantities are what differ, spell class 9 for a
    // spell, class 0x14 with a computed icon index for a secondary skill,
    // and the primary-skill picture 0x1f + skill for a stat point. 115 sits
    // directly after the resource pile's 113, which is where the alphabet
    // puts "scholar".
    ADV_EVENT_TEXT_SCHOLAR = 115,
    // The resource-pile pickup line, SHARED by DoEventResource (0x4a4be0)
    // and the customised-cell handler it delegates to (0x4a4780, which
    // formats it twice). It is a sprintf format taking the resource's own
    // name with its first letter lower-cased, which is why the handler
    // copies the name out of gResourceNames instead of passing it through.
    // 113 sits directly after the refugee camp's 112, which is where the
    // alphabet puts "resource".
    ADV_EVENT_TEXT_RESOURCE_PICKUP = 113,
    // DoEventPowerSchool (0x4a3dc0), picture 0x21 = +1 Spell Power - the
    // fourth member of the primary-skill quartet above.
    ADV_EVENT_TEXT_POWER_SCHOOL = 100,
    ADV_EVENT_TEXT_POWER_SCHOOL_VISITED = 101,
    // DoEventSurvivor (0x4a52a0), the shipwreck survivor: 125 is a sprintf
    // format taking the artifact name, 126 the line a hero with all
    // sixty-four backpack slots full gets - and unlike every other
    // artifact-bearing object in this enum the survivor's cell carries the
    // artifact id in its RAW extra-info dword, not in a bitfield lane. The
    // pair sits below the sirens' block, which is where the alphabet puts
    // "shipwreck survivor".
    ADV_EVENT_TEXT_SURVIVOR_ARTIFACT = 125,
    ADV_EVENT_TEXT_SURVIVOR_BACKPACK_FULL = 126,
    // DoEventSiren (0x4a5980). 132 is a sprintf format taking the
    // experience the drowned troops were worth, 133 the already-visited
    // line and 134 the row a hero with nothing to lose gets. The trio
    // sits just below the stables' block, which is where the alphabet
    // puts "sirens" - and the DC roster agrees, running DoEventSiren
    // (dc 0x95a34) before DoEventSpellScroll and DoEventStables.
    ADV_EVENT_TEXT_SIRENS = 132,
    ADV_EVENT_TEXT_SIRENS_VISITED = 133,
    ADV_EVENT_TEXT_SIRENS_NO_LOSS = 134,
    // The scroll pickup line, SHARED by DoEventSpellScroll (0x4a5ea0) and
    // the customised-cell handler it delegates to (0x4a5a80). It is a
    // sprintf format taking the SPELL's name out of akSpellTraits, not an
    // artifact name, and the dialog shows it with picture class 9 - the
    // spell class - where every other artifact row in this enum uses 8.
    ADV_EVENT_TEXT_SPELL_SCROLL = 135,
    // DoEventStables (0x4a60a0), and this is the only object in the enum
    // with FOUR contiguous rows: the handler accumulates what it actually
    // did into a two-bit value and switches over it, so every combination
    // of "granted the movement" and "upgraded the Cavaliers" has its own
    // line. The two upgrade rows carry picture class 0x15 with extra 11,
    // the Champion. 136..139 sits directly before the temple's 140/141,
    // which is where the alphabet puts "stables".
    ADV_EVENT_TEXT_STABLES_NOTHING = 136,
    ADV_EVENT_TEXT_STABLES_MOVEMENT = 137,
    ADV_EVENT_TEXT_STABLES_UPGRADE = 138,
    ADV_EVENT_TEXT_STABLES_BOTH = 139,
    // DoEventTemple (0x4a6200). ONE reward row serves both arms - the
    // Sunday visit differs only in showing the morale picture twice -
    // and 141 is the already-worshipped line.
    ADV_EVENT_TEXT_TEMPLE = 140,
    ADV_EVENT_TEXT_TEMPLE_VISITED = 141,
    // DoEventTrainingGrounds (0x4a6330). The reward line carries picture
    // 0x11 and the experience amount as its quantity.
    ADV_EVENT_TEXT_TRAINING_GROUNDS = 143,
    ADV_EVENT_TEXT_TRAINING_GROUNDS_VISITED = 144,
    // DoEventWagon (0x4a69b0), the run directly below the war school's -
    // which is where the alphabet puts "wagon" - and the only object in
    // this enum whose reward comes in TWO flavours: 154 is the resource
    // payout (a resource picture and its amount), 155 the sprintf format
    // taking the artifact name and 156 the already-looted line. The DC
    // roster agrees: DoEventWagon (dc 0x96784) runs before DoEventWarSchool
    // (dc 0x97628).
    ADV_EVENT_TEXT_WAGON_RESOURCE = 154,
    ADV_EVENT_TEXT_WAGON_ARTIFACT = 155,
    ADV_EVENT_TEXT_WAGON_EMPTY = 156,
    // DoEventWarSchool (0x4a7a40). 158 is the two-picture choice offered
    // with iMBType 10 (+1 Attack against +1 Defense), 159 the
    // already-trained line and 160 the "under 1000 gold" refusal.
    ADV_EVENT_TEXT_WAR_SCHOOL_CHOOSE = 158,
    ADV_EVENT_TEXT_WAR_SCHOOL_VISITED = 159,
    ADV_EVENT_TEXT_WAR_SCHOOL_NO_GOLD = 160,
    // DoEventTreasure (0x4a6520), the treasure chest: 145 is the sprintf
    // format taking the artifact name. Its twin 146 belongs to
    // DoTreasureDialog (0x4a6440), which is not reconstructed here, so
    // only the row this body proves is named. 145 sits directly after the
    // training grounds' pair at 143/144, which is where the alphabet puts
    // "treasure chest".
    ADV_EVENT_TEXT_TREASURE_ARTIFACT = 145,
    // DoEventTreeOfKnowledge (0x4a6710), the longest run in this enum after
    // the pyramid's: 147 is the already-taught line and then the three
    // prices take two rows apiece - 148 the free tree, 149/150 the
    // 2000-gold prompt and its refusal, 151/152 the ten-gems pair. The run
    // sits directly after the treasure chest's 145/146, which is where the
    // alphabet puts "tree of knowledge".
    ADV_EVENT_TEXT_TREE_VISITED = 147,
    ADV_EVENT_TEXT_TREE_FREE = 148,
    ADV_EVENT_TEXT_TREE_GOLD_PROMPT = 149,
    ADV_EVENT_TEXT_TREE_NO_GOLD = 150,
    ADV_EVENT_TEXT_TREE_GEMS_PROMPT = 151,
    ADV_EVENT_TEXT_TREE_NO_GEMS = 152,
    // do_event_warrior_tomb (0x4a7c30). 161 is asked with iMBType 2 - the
    // yes/no form whose answer the handler reads back out of
    // heroWindowManager::dialogReturn - 162 is a sprintf format taking the
    // artifact name, and 163 is the empty-tomb line that costs -3 morale.
    ADV_EVENT_TEXT_WARRIOR_TOMB_PROMPT = 161,
    ADV_EVENT_TEXT_WARRIOR_TOMB_ARTIFACT = 162,
    ADV_EVENT_TEXT_WARRIOR_TOMB_EMPTY = 163,
    ADV_EVENT_TEXT_WATER_WHEEL_GOLD = 164,
    ADV_EVENT_TEXT_WATER_WHEEL_EMPTY = 165,
    ADV_EVENT_TEXT_WATERING_HOLE = 166,
    ADV_EVENT_TEXT_WATERING_HOLE_VISITED = 167,
    ADV_EVENT_TEXT_WINDMILL_EMPTY = 169,
    ADV_EVENT_TEXT_WINDMILL_RESOURCE = 170,
    // do_event_witch_hut (0x4a8080). All three take the secondary-skill
    // name through sprintf; only the first one actually teaches, and the
    // roles follow from which arm of the cascade reaches each row.
    ADV_EVENT_TEXT_WITCH_HUT_LEARN = 171,
    ADV_EVENT_TEXT_WITCH_HUT_KNOWN = 172,
    ADV_EVENT_TEXT_WITCH_HUT_FULL = 173,
    // The row the same handler shows when the cell carries no skill at
    // all (the WitchHutNoSkillMask encoding, i.e. a -1 in the seven-bit
    // field). It sits well outside the witch hut's own alphabetical run,
    // so the ROLE is what names it; only the index 190 is byte-proven.
    ADV_EVENT_TEXT_WITCH_HUT_NO_SKILL = 190
};

// The Fountain of Fortune's luck tiers, and the domain is closed by
// construction: DoEventFountain (0x4a2480) range-checks `luck + 1`
// against 4 and jump-tables the five slots, so nothing outside -1..3 can
// reach an arm. The VALUES are the luck the fountain grants, which is why
// the enumerators are spelled as the amounts; only the cursed tier and
// the three positive ones carry a hero flag bit, so 0 has no arm.
enum EFountainLuck {
    FOUNTAIN_LUCK_CURSED = -1,
    FOUNTAIN_LUCK_NONE = 0,
    FOUNTAIN_LUCK_PLUS_1 = 1,
    FOUNTAIN_LUCK_PLUS_2 = 2,
    FOUNTAIN_LUCK_PLUS_3 = 3
};

// The Tree of Knowledge's price selector, enumerator NAMES the
// The tree-price selector enum WiseTreePrices lives in advmgr.h (the
// one copy). DoEventTreeOfKnowledge (0x4a6710) switches over it with the
// decrement chain and sends anything else straight to the experience
// award, which is why the count enumerator has no arm of its own.

// The sea chest's reward selector, enumerator NAMES the Dreamcast's own
// (evidence/dreamcast/enums.csv, enum SeaChestRewardTypes). The domain is
// closed by construction: DoEventSeaChest (0x4a5030) switches over it with
// the decrement chain and sends anything else straight to the pick-up.
enum SeaChestRewardTypes {
    const_sea_chest_nothing = 0,
    const_sea_chest_gold = 1,
    const_sea_chest_artifact = 2
};

// The scholar's award selector enum ScholarAwards lives in mapcell.h
// (the one copy; DoEventScholar 0x4a4dc0 tests the three values in the
// order 2, 1, 0 and lets anything else fall through unrewarded).

// DoEventFlotsam's (0x4a2230) size selector, held in the cell's own
// extra-info dword. The domain is closed by construction: retail
// range-checks the UNSIGNED value against 3 and jump-tables the four
// slots, sending anything larger straight to the pick-up. The
// enumerators are spelled for what each slot pays, which is what the
// four advevent.txt rows above describe.
enum EFlotsamSize {
    FLOTSAM_NOTHING = 0,
    FLOTSAM_WOOD = 1,
    FLOTSAM_WOOD_AND_GOLD = 2,
    FLOTSAM_LARGE = 3
};

// DoEventStables' (0x4a60a0) report selector: a two-bit accumulator the
// handler builds as it goes - bit 0 set when the movement bonus was
// granted, bit 1 when a stack of Cavaliers was upgraded - and then
// switches over to pick one of the four advevent.txt rows above. The
// domain is complete and closed by construction: retail range-checks the
// widened value against 3 and lets nothing else through.
enum EStablesResult {
    STABLES_NOTHING = 0,
    STABLES_MOVEMENT = 1,
    STABLES_UPGRADE = 2,
    STABLES_MOVEMENT_AND_UPGRADE = 3
};

// Retail .bss 0x698a94, one int, loaded by the ballist.txt parser
// (0x4d7240) out of row 6 column 5 alongside five siblings at
// 0x698afc..0x698b0c. The DEFINITION belongs to that parser's compiland,
// which this tree does not own yet; it is declared here because
// DoEventStables is the modeled consumer that needs it.

// The role is fixed by all three readers agreeing, not by the table:
// DoEventStables adds it to hero::maxMovePoints and hero::movePoints
// under the flag-bit-1 guard; town.obj's 0x5bd8e0 makes the SAME pair of
// adds under the SAME guard, i.e. the Stables building; and philai's
// ValueOfStables (0x52aac0) appraises a visit as
// `(8 - dayOfWeek) * this / 2`, the movement still to be had this week.
DATA(0x00698a94) extern int g_stablesMovementBonus;

// Retail .bss 0x699540. DoCombat raises it across the whole interactive
// battle (set to 1 right after the mouse pointer swap, cleared just
// before the final ShowPointer) - an "a combat is on screen" latch by
// role. Name ordinal; DoCombat is the first consumer, so events.h holds
// the claim until the band's producer is decoded (the gUnnamed691208
// rationale above).
DATA(0x00699540) extern int g_unnamed699540;

// advManager::FizzleCenter's (0x4acbb0) sound selector, also the second
// argument of advManager::HeroLoses. Retail lowers the two arms as a
// SWITCH (`sub eax,0 / je` then `dec eax / jne`), and every value outside
// the pair suppresses the flash entirely.
enum EFizzleSound {
    FIZZLE_SOUND_KILL_FADE = 0,
    FIZZLE_SOUND_PICKUP = 1
};

#endif  /* HOMM3_EVENTS_H */
