#ifndef HOMM3_ARMYGRP_H
#define HOMM3_ARMYGRP_H

#include "va.h"
#include "spell_traits.h"
#include "creature_traits.h"
#include "creature_type.h"

#include "abstractfile.h"
#include "artifact_type.h"
#include "creature_flags.h"
#include "spellschool.h"
#include "spelleffect_type.h"
#include "struct.h"
#include "terrain_type.h"

namespace std {
template<class T> class allocator;
template<class E> struct char_traits;
template<class E, class Tr, class A> class basic_string;
}


// The spell-id domain (the full roster gets its own header when spell
// work begins in earnest); DC CodeView types the parameters SpellID.
typedef int SpellID;

// Named spell ids as retail compares and case labels surface them,
// grown per consumer (armygrp's work-chance/damage switches,
// ai_combat's damage-spell dispatch). NH3API spells.hpp spellings;
// the DC SpellID enum corroborates every value (eSpellStoneGaze for
// SPELL_STONE).
enum ESpellId {
    // Original DC LF_ENUM SpellID 0x1b61 uses signed int (0x74) and
    // eSpellNone = -1. The default type_spell_choice constructor passes
    // this sentinel to type_enchant_data; retain the signed domain through
    // canonical enum spell interfaces as well as the Complete positive IDs.
    SPELL_NONE = -1,
    SPELL_SUMMON_BOAT = 0x0,
    // advManager::SkuttleBoat (0x41cdf0) is the witness and it proves the
    // id twice in one body: it takes its traits row at `akSpellTraits +
    // 0x88` (one 136-byte record past SUMMON_BOAT's) for the per-mastery
    // success chance, and charges the cast with
    // `hero::GetManaCost(1, 0, get_special_terrain())`.
    SPELL_SCUTTLE_BOAT = 0x1,
    // IsInIdentifyRange indexes this row's mastery bonus and multiplies it by
    // spell power to derive the scouting radius; retail displacement 0x144
    // proves spell id 2 independently of the Dreamcast spelling.
    SPELL_VISIONS = 0x2,
    // advManager::CastSpell (0x41c490) dispatches the ten adventure spells
    // through one ascending jump table, which fixes every id in the run: its
    // arms 3 and 5 both play "view.wav" and hand their OWN case value to
    // advManager::ViewWorld, and arm 4 writes hero::disguiseLevel. Each arm
    // then charges `GetManaCost(<that id>, ...)`, so the traits row and the
    // id agree twice over.
    SPELL_VIEW_EARTH = 0x3,
    SPELL_DISGUISE = 0x4,
    SPELL_VIEW_AIR = 0x5,
    // Dreamcast's first combat spell and CastSpell's first jump-table arm;
    // retail indexes the mastery row at 0x642214 and places QuicksandInfo.
    SPELL_QUICKSAND = 0xa,
    // 11. combatManager::SetupAndLoadObstacles (0x466290) prices the
    // Tower's moat with ComputeSpellDamage(11, ...) and builds each
    // moat hex out of the obstacle shape whose sprite is C09spF1.def -
    // i.e. the Tower's moat IS a minefield, which is what identifies
    // the spell. The id also sits one below SPELL_EARTHQUAKE 0xe in the
    // battlefield-obstacle run this enum already anchors at that end.
    SPELL_LAND_MINE = 0xb,
    // The two WALL spells, byte-proven by combatManager::ValidSpellTarget
    // (0x5a39c0): its tail tests the spell against 0xc and then 0xd and
    // gives each one its own placement rule - 0xc walks the two spell
    // obstacle-shape rows cmbtmgr.h declares (whose sprites are the
    // C15spE pair) and 0xd walks a hard-coded two-or-three cell column
    // at -17 / -34 with a row-parity nudge. Both sit inside the
    // battlefield-obstacle run this enum already anchors at both ends
    // (LAND_MINE 0xb above, EARTHQUAKE 0xe below), which is what fixes
    // WHICH is which: 0xc is one below Fire Wall, i.e. Force Field.
    SPELL_FORCE_FIELD = 0xc,
    SPELL_FIRE_WALL = 0xd,
    // hero::Fly at 0x4e59a0 passes 6 to get_spell_level and indexes the
    // corresponding 0x88-byte traits row's per-mastery mana-cost band.
    SPELL_FLY = 0x6,
    // AI_AttemptMove's two path-mode helpers pass these ids, and its
    // town-portal arm independently charges spell 9's mana cost.
    SPELL_WATER_WALK = 0x7,
    // attempt_teleport passes 8 to all three spell helpers and indexes row
    // 8's per-mastery cast-count limit before the retained TeleportTo call.
    // Dreamcast independently names the same SpellID rung.
    SPELL_DIMENSION_DOOR = 0x8,
    SPELL_TOWN_PORTAL = 0x9,
    // DC eSpellEarthquake = 14. Read by check_wall_archery_penalty
    // (0x42482b) out of hero::available_spells - a hero who can bring
    // the town wall down is modeled as taking no wall archery penalty.
    SPELL_EARTHQUAKE = 0xe,
    SPELL_ICE_BOLT = 0x10,
    SPELL_LIGHTNING_BOLT = 0x11,
    SPELL_CHAIN_LIGHTNING = 0x13,
    SPELL_FROST_RING = 0x14,
    SPELL_FIREBALL = 0x15,
    SPELL_INFERNO = 0x16,
    SPELL_METEOR_SHOWER = 0x17,
    SPELL_DEATH_RIPPLE = 0x18,
    SPELL_DESTROY_UNDEAD = 0x19,
    SPELL_ARMAGEDDON = 0x1a,
    // The two damage-reduction shields, byte-proven 2026-08-08 by the
    // pricers' traits displacement `K*136 + 0x34` (the mastery_bonus
    // row): get_shield_value (0x438c60) indexes +0xe8c = 27*136 + 0x34
    // and get_air_shield_value (0x438bc0) indexes +0xf14 = 28*136 +
    // 0x34. The pair sits immediately below FIRE_SHIELD, which the
    // surrounding roster already pins.
    SPELL_SHIELD = 0x1b,
    SPELL_AIR_SHIELD = 0x1c,
    // compute_fire_shield_damage (0x422440) prices the shield's
    // retaliation through ModifySpellDamage under THIS id, which fixes
    // it against the surrounding roster (0x1e is the first protection).
    SPELL_FIRE_SHIELD = 0x1d,
    // The anti-magic family ai_combat's get_enchantment_value
    // (0x425510) writes off against a side that cannot cast. Their
    // values are fixed by the surrounding roster, which is already
    // byte-proven at both ends (DISPEL 0x23, RESURRECTION 0x26).
    SPELL_PROTECTION_FROM_AIR = 0x1e,
    SPELL_PROTECTION_FROM_FIRE = 0x1f,
    SPELL_PROTECTION_FROM_WATER = 0x20,
    SPELL_PROTECTION_FROM_EARTH = 0x21,
    SPELL_ANTI_MAGIC = 0x22,
    SPELL_DISPEL = 0x23,
    SPELL_MAGIC_MIRROR = 0x24,
    SPELL_CURE = 0x25,
    SPELL_RESURRECTION = 0x26,
    SPELL_ANIMATE_DEAD = 0x27,
    // The gap between ANIMATE_DEAD and BLESS, byte-proven 2026-08-20 by
    // combatManager::find_spell_target (0x5a3950): its three-way jump
    // chain is `sub edx, 0x26 / dec / dec`, so the third arm is 0x28, and
    // that arm is the one gated on `first_target` before it reaches
    // find_resurrection_target - the sacrifice beneficiary lookup.

    // Corroborated independently by ai_tactical's consider_spell, whose
    // 56-entry byte table sends `spell - 14 == 26` to the arm that calls
    // type_AI_spellcaster::consider_sacrifice and nothing else, and a
    // third time by combatManager::SpellTargetMessage (0x5a8690), whose
    // 0x28 arm is the one gated on `first_target`.
    SPELL_SACRIFICE = 0x28,
    SPELL_BLESS = 0x29,
    SPELL_CURSE = 0x2a,
    // The 41..52 enchantment ladder, byte-proven end to end 2026-08-08
    // by the ai_tactical pricers' `K*136 + 0x34` mastery_bonus
    // displacement. Every rung below is measured, and the two rungs
    // already here (PRECISION 44, MIRTH 49) plus SORROW 50 and
    // MISFORTUNE 52 are carried by functions that compile EXACT, so
    // the ladder is anchored at both ends and no rung can shift:
    //   get_blood_lust_value    0x438100  +0x170c = 43*136 + 0x34
    //   get_weakness_value      0x438ed0  +0x181c = 45*136 + 0x34
    //   get_tough_skin_value    0x438d90  +0x18a4 = 46*136 + 0x34
    //   get_disruptive_ray_value 0x438dc0 +0x192c = 47*136 + 0x34
    //   get_prayer_value        0x438ac0  +0x19b4 = 48*136 + 0x34
    // DISRUPTING_RAY corroborates independently: 0x438dc0 also PUSHES
    // the literal 0x2f into the work-chance leaf at 0x5a8090.
    // STONE_SKIN is the DC roster's get_tough_skin_value; the spell it
    // prices is Stone Skin (the HD crossbuild names that same body
    // get_stone_skin_value), so the enumerator takes the spell's name.
    SPELL_BLOODLUST = 0x2b,
    SPELL_PRECISION = 0x2c,
    SPELL_WEAKNESS = 0x2d,
    SPELL_STONE_SKIN = 0x2e,
    SPELL_DISRUPTING_RAY = 0x2f,
    SPELL_PRAYER = 0x30,
    SPELL_MIRTH = 0x31,
    SPELL_SORROW = 0x32,
    SPELL_FORTUNE = 0x33,
    SPELL_MISFORTUNE = 0x34,
    // 54, and army.h's spell-influence block already fixes the value
    // from the other side: "+0x198 + 54*4 == +0x270" is the word that
    // header reads for Slow. The one consumer in this tree is
    // combatManager::SetNextArmy (0x465330), where 0x36 is the FIRST of
    // the four spells the Armor of the Damned auto-casts - Slow, Curse,
    // Weakness, Misfortune - which is also what corroborates it, since
    // the other three are already byte-proven enumerators above.
    // Corroborated a third time by combatManager::ShowSpellMessage
    // (0x5a8950): its artifact arm groups exactly 0x36 with CURSE 0x2a,
    // WEAKNESS 0x2d and MISFORTUNE 0x34 onto one Armor-of-the-Damned
    // line, which is the same four-spell set from the other side.
    SPELL_SLOW = 0x36,
    SPELL_SLAYER = 0x37,
    SPELL_TITANS_LIGHTNING_BOLT = 0x39,
    // 58, byte-proven by ai_tactical's get_counterstroke_value
    // (0x439e80), which folds the row into the displacement as
    // `[akSpellTraits + 4*mastery + 0x1f04]` = 58*136 + 0x34 - the
    // constant form, so the enumerator is what the source names.
    SPELL_COUNTERSTRIKE = 0x3a,
    SPELL_BERSERK = 0x3b,
    SPELL_HYPNOTIZE = 0x3c,
    SPELL_FORGETFULNESS = 0x3d,
    SPELL_BLIND = 0x3e,
    SPELL_TELEPORT = 0x3f,
    // 64, byte-proven by combatManager::SpellTargetMessage (0x5a8690).
    // Its jump table covers exactly 0xc..0x40 and the TOP entry, 0x40,
    // gets an arm of its own: `strcpy(gText, <the row two above the two
    // target-naming rows the sacrifice arms use>)` and nothing else -
    // a fixed line with no target to name, which is what a spell that
    // removes a battlefield obstacle prints. It sits one above
    // SPELL_TELEPORT 0x3f and one below SPELL_CLONE 0x41, both already
    // proven, so no other value fits.

    // UNGATED DELIBERATELY. armygrp.h reaches initialize.cpp's include
    // closure and a new enumerator is an input to the include-set class
    // recorded on SSpellTraits below; the trade was authorised, is
    // recorded above initialize_game_data's baseline row, and the
    // ratchet keeps the peak in `hist` for a later lane to re-measure.
    SPELL_REMOVE_OBSTACLE = 0x40,
    // get_elemental_type's 0x5a9360 jump table independently proves this
    // contiguous summon family and its mapping to the four base elementals.
    // Dreamcast CodeView supplies the enumerator spellings.
    SPELL_SUMMON_FIRE_ELEMENTAL = 0x42,
    SPELL_SUMMON_EARTH_ELEMENTAL = 0x43,
    SPELL_SUMMON_WATER_ELEMENTAL = 0x44,
    SPELL_SUMMON_AIR_ELEMENTAL = 0x45,
    SPELL_STONE = 0x46,
    SPELL_POISON = 0x47,
    // The nine rows ai_tactical's enchantment dispatch
    // get_enchantment_function (0x43b690) needs and this roster did
    // not carry. Each is byte-proven by that function's own two
    // tables: the 61-entry BYTE index table at +0x214 maps
    // `spell - 15` onto a slot of the 39-entry dword table at +0x178,
    // and every slot's target is a `mov eax, offset get_<x>_value`
    // whose relocation names the handler outright. So the value ->
    // handler pairing is READ, not inferred, and the enumerator takes
    // the handler's spell:
    //   15, 18 -> get_damage_spell_value, the two flanks of the
    //             already-proven ICE_BOLT 16 / LIGHTNING_BOLT 17
    //   53 -> get_haste_value      56 -> get_frenzy_value
    //   65 -> get_clone_value      73 -> get_disease_value
    //   74 -> get_blind_value      75 -> get_age_value
    //   72 -> the table's second default slot (no pricer of its own)
    // Four of the nine are independently corroborated by army.h's
    // +0x198 spell-influence row, which already fixes FRENZY 0x38,
    // BIND 0x48 and AGE 0x4b from the round-counter side and puts
    // PARALYZE on 74. (SPELL_SACRIFICE 0x28 belongs to this set too and
    // is declared once, in value order, above.)
    SPELL_MAGIC_ARROW = 0xf,
    SPELL_IMPLOSION = 0x12,
    SPELL_HASTE = 0x35,
    // Round-side corroboration: the one influence row ResetRound
    // (0x447120) refuses to decrement, paired with army.h's frenzyRounds
    // (+0x278 == +0x198 + 56*4) by ComputeAttackerDamageReduction.
    SPELL_FRENZY = 0x38,
    SPELL_CLONE = 0x41,
    // Round-side corroboration: remove_binding (0x43ee10) cancels this
    // row the moment a bound stack's `binders` list empties - `push 0x48
    // / call CancelIndividualSpell` - the same index 72 army.h's +0x2b8
    // bindRounds field pairs. Also the folded `akSpellTraits + 0x2650`
    // row address in viewarmywindow.cpp's Dendroid-hold arm.
    SPELL_BIND = 0x48,
    SPELL_DISEASE = 0x49,
    SPELL_PARALYZE = 0x4a,
    SPELL_AGE = 0x4b,
    // The Lich shot's pseudo-spell row: army::range_attack (0x43f900)
    // reads akSpellTraits[0x4c].m_sample/m_effect for the death-cloud
    // animation over the target hex. NH3API's SPELL_DEATH_CLOUD at the
    // same value; the neighbours 0x4a/0x4b above and 0x4e below bracket
    // it. Canaries measured on admission (the ESpellId class is
    // non-monotonic - see the SSpellTraits school note).
    SPELL_DEATH_CLOUD = 0x4c,
    // 78, byte-proven by combatManager::ShowSpellMessage (0x5a8950).
    // Its creature-spell dispatch is a jump table over 0x2a..0x4e and
    // 0x4e is the TOP entry, with an arm of its own that prints one
    // text row over the affected stack's name and nothing else - the
    // shape every other creature-ability arm in that table has. The
    // value is fixed at both ends by the same table: 0x4b AGE is the
    // last already-proven rung below it, and the window closes at 0x4e.
    SPELL_DISPEL_HELPFUL = 0x4e,
    // The Mighty Gorgon's pseudo-spell row: army::do_post_attack
    // (0x440bc0) plays akSpellTraits[0x4f].m_sample over its death
    // stare, exactly as the Rust Dragon plays row 0x50's below.
    // Bracketed by the proven 0x4e above and 0x50 below.
    SPELL_DEATH_STARE = 0x4f,
    // 80, the one row Complete's 81-wide influence table carries past
    // the Dreamcast build's kNumSpellsAndCreatureEffects = 80: the
    // Rust Dragon's acid. Byte-proven by check_special_attack
    // (0x440500), whose rust-dragon arm stores 0x50 into the pending
    // iPostPowSpellToCast gated on the victim's defenseSkill being
    // positive - the defense-eating half of Acid Breath. NH3API
    // spelling.
    SPELL_ACID_BREATH_DEFENSE = 0x50
};

SIZE(SSpellTraits, 136);

// The spell table is reached through a stored pointer, exactly like
// akCreatureTypeTraits: retail loads [0x687f58] before indexing.
// The 81-entry count is now retail-proven: spelldefs constructs 81 strings
// and writes the contiguous 136-byte backing rows at 0x685450, whose exact
// end is this pointer cell (0x685450 + 81*136 == 0x687f58).
extern const SSpellTraits (&g_spellTraits)[81];

unsigned char spellTargetsASingleArmy(int spell, int sslevel);

// The special-ground MODE GetArmyMorale/GetArmyLuck dispatch on (the
// dword param with sentinels 2..5): cursed ground zeroes the stat,
// holy ground/evil fog swing morale by town alignment, the clover
// field lifts neutral-town luck. NH3API terrain.hpp EMagicTerrain
// spellings. NewmapCell::get_magic_terrain_type (0x4fcf40) proves the
// remaining return values against its five-way special-terrain switch.
enum EMagicTerrain {
    MAGIC_TERRAIN_INVALID = -1,
    MAGIC_TERRAIN_COAST = 0,
    MAGIC_TERRAIN_MAGIC_PLAINS = 0x1,
    MAGIC_TERRAIN_CURSED_GROUND = 0x2,
    MAGIC_TERRAIN_HOLY_GROUND = 0x3,
    MAGIC_TERRAIN_EVIL_FOG = 0x4,
    MAGIC_TERRAIN_CLOVER_FIELD = 0x5,
    MAGIC_TERRAIN_LUCID_POOLS = 0x6,
    MAGIC_TERRAIN_FIERY_FIELDS = 0x7,
    MAGIC_TERRAIN_ROCKLANDS = 0x8,
    MAGIC_TERRAIN_MAGIC_CLOUDS = 0x9
};

SIZE(TCreatureTypeTraits, 116);

// attributes bits proven by retail tests: 0x40000 by HasAllUndead
// (0x44ab20); 0x40 by GetAlignments (0x44abb0), which skips such
// creatures in the alignment census - the war-machine bit.
const unsigned int g_ctaUndead = 0x40000;
const unsigned int g_ctaSiegeWeapon = 0x40;
// 0x20000 zeroes a stack's morale outright (GetArmyMorale 0x44b11e) -
// the no-morale trait (undead/elemental/war-machine family).
const unsigned int g_ctaNoMorale = 0x20000;
// Bit 4, byte-proven by army::new_turn (0x446e30): the Elixir of Life
// regenerates a stack only when its traits row carries this bit - the
// living-creature marker (the Elixir does nothing for the undead and
// the war machines).
const unsigned int g_ctaAlive = 0x10;

// Artifact ids as the IsWieldingArtifact gates surface them (NH3API
// artifact.hpp spellings; every name is corroborated by the byte-
// proven behavior at its call site). 0x54 zeroes POSITIVE morale
// (GetArmyMorale 0x44b23a); 0x55 zeroes luck for BOTH sides when
// either hero wields it (GetLuck 0x44b2e4). The rest are
// get_spell_work_chance's immunity gates: the eight pendants block
// exactly their lore spells, the Badge blocks the mind family, the
// Sphere of Permanence blocks Dispel, the Orb of Vulnerability makes
// work certain, and 0x86 - Power of the Dragon Father - blocks
// level<=4 spells (the decode note's "Orb of Inhibition" inference
// does not survive the NH3API roster: inhibition is 0x7e).

// The traits table is reached through a stored pointer (reference
// global): retail loads [0x6747b0] before indexing. NH3API names it
// akCreatureTypeTraits (a const reference to the 150-entry array).
extern const TCreatureTypeTraits (&g_creatureTypeTraits)[150];

// CreatureBackgroundNames: neutral first, then the nine town alignments.
// Index with alignment + 1 so neutral alignment -1 selects the first entry.
extern const char* g_creatureBackgroundNames[10];

// Army-size name tables (BSS at 0x6a5bb8, runtime-filled from game
// text): nine threshold bands x three name sets, 12-byte row stride
// proven by GetArmySizeName's nine reloc targets. The NAME is a
// bootstrap invention (no Dreamcast/NH3API name survives for these).

// GetMorale's two town-building tests were bootstrapped here as
// separate `unsigned int[2]` mask objects (gTavernMask /
// gBrotherhoodOfTheSwordMask) while another lane owned town.h. They
// were never separate objects: the town lane byte-proved
// `bitNumber[i] == 1i64 << i` for every i < 48 in the pinned image, so
// 0x66cdc0 and 0x66ce48 are simply bitNumber[5] and bitNumber[22].
// GetMorale now spells them `built & bitNumber[TAVERN_ID]` and
// `active & bitNumber[EXTRA_1_ID]` off town.h's own table - the Tavern
// and, for a Castle, the Brotherhood of the Sword. Merged 2026-08-07.

// The game singleton and gpGame now live in their owner's header
// (game.h); f_1f698 is the one field GetAlignments reads (nonzero ->
// elementals keep their town alignment; zero -> they census as
// neutral; likely the expansion/map-version gate).

// PROVEN layout (2026-08-04): Dreamcast CodeView class size 56 with
// armies @0 and numTroops @28, corroborated by retail codegen - the
// 7-slot loops and the CREATURE_NONE sentinel in
// GetNumArmies (0x44acc0) / IsMember (0x44ab80).
class armyGroup {
public:
    enum { ARMY_GROUP_SLOT_COUNT = 7 };

    // Spelled int (not TCreatureType) so slot writes from int-typed
    // parameters (Add) stay cast-free; the enum appears where the
    // Dreamcast prototypes demand it - and game::ViewArmy (0x4c6c50) is
    // the first body that needs the READING side typed as well, because
    // it hands a slot straight to UpgradedCreatureType and
    // get_upgrade_cost and both take the enum. A union of the two views
    // over ONE storage keeps readers and writers alike cast-free, which
    // this tree's zero enum-cast floor requires; `armyTypes` is the
    // Dreamcast's own typing of the array and `armies` the writers'
    // convenience. Retyping the array outright was measured and does
    // NOT work: it breaks five int-typed slot writes in armygrp.cpp and
    // one in townmgr.cpp.
    union {
        int m_armies[ARMY_GROUP_SLOT_COUNT];
        TCreatureType m_armyTypes[ARMY_GROUP_SLOT_COUNT];
    };

    armyGroup();
    armyGroup(TCreatureType type, int amount);
    int m_numTroops[ARMY_GROUP_SLOT_COUNT];
    void initialize();
    int getAlignments(unsigned char* alignments) const;
    int getHomogeneityMoraleAdjust() const;
    void damageGroup(float casualtyRate);
    long getAIValue() const;
    int getCreatureTotal() const;
    int getCreatureTotal(TCreatureType monType) const;
    unsigned char isMember(TCreatureType monType) const;
    int canJoin(int monType) const;
    unsigned char hasAllUndead() const;
    // Dreamcast armygrp.cpp:668. Complete retains the same source helper at
    // its morale consumers; VC6 /Ob2 expands the loop and /OPT:REF removes
    // the unreferenced out-of-line copy from retail.
    unsigned char hasSomeUndead() const;
    unsigned char merge(armyGroup* ag);
    void mergeArmies(armyGroup& source);
    void splitArmy(int srcIndex, armyGroup* ag, int destIndex,
                   unsigned char inSrcRestricted,
                   unsigned char inDestRestricted);
    unsigned char hasCreatures() const;
    TTerrainType getNativeTerrain() const;
    // Original DC GetLuck/GetMorale publics end in _N3@Z: both the
    // cursed-ground and apply-limits flags are bool, despite CodeView's
    // lowered unsigned-char display. Complete's added grouping byte is
    // independent of those two native flags.
    int getLuck(const class hero* ownerHero, const class town* ownerTown,
                const class hero* otherHero, const armyGroup* otherGroup,
                bool onCursedGround,
                bool applyLimits) const;
    int getMorale(const class hero* ownerHero, const class town* ownerTown,
                  const class hero* otherHero, const armyGroup* otherGroup,
                  bool onCursedGround,
                  unsigned char groupAlignments,
                  bool applyLimits) const;
    int getArmyMorale(int index, const class hero* ownerHero,
                      const class town* ownerTown, int mode,
                      unsigned char arg5,
                      unsigned char applyLimits) const;
    int getArmyLuck(int index, const class hero* ownerHero,
                    const class town* ownerTown, int mode,
                    unsigned char applyLimits) const;
    // The older DC description publics also use _N for cursed ground.
    // Complete replaces it with the multi-valued magicTerrain argument.
    std::basic_string<char, std::char_traits<char>, std::allocator<char> >
        getMoraleDescription(TCreatureType creature, int morale,
                               const class hero* ownerHero,
                               const class town* ownerTown,
                               const class hero* otherHero,
                               const armyGroup* otherGroup,
                               int magicTerrain,
                               unsigned char groupAlignments) const;
    // Retail Complete added CREATURE and widened the final magic-terrain
    // parameter relative to the older Dreamcast prototype. The body indexes
    // creature traits from the first argument and returns with `ret 20h`.
    std::basic_string<char, std::char_traits<char>, std::allocator<char> >
        getLuckDescription(TCreatureType creature, int luck,
                             const class hero* ourHero,
                             const class town* ourTown,
                             const class hero* enemyHero,
                             const armyGroup* enemyGroup,
                             int magicTerrain) const;
    int save(TAbstractFile* outfile);
    int load(TAbstractFile* infile);
    int add(int armyType, int newNumTroops, int newIndex);
    void dismiss(int whichIndex);
    void swap(int srcIndex, armyGroup* destGroup, int destIndex);
    int getNumArmies() const;
    static const char* getArmySizeName(int howMany, int nameSet);
};
SIZE(armyGroup, 56);

// Live prototypes (claimed armygrp.cpp bodies; ai_combat's spell-work
// chain calls both).
float getSpellWorkChance(SpellID spell, TCreatureType targetArmyType,
                            const class hero* const castingHero,
                            const class hero* const targetHero);            // 0x44a4d0
long modifySpellDamage(long damage, SpellID spell, TCreatureType creature);  // 0x44b4b0

// --- TSplitWindow ---

// E:\gamedcs\armygrp.cpp:668, dc 0x4eb88
// E:\gamedcs\armygrp.cpp:748, dc 0x4ec98
// E:\gamedcs\armygrp.cpp:885, dc 0x4ee08
// E:\gamedcs\armygrp.cpp:1347, dc 0x4f708
// E:\gamedcs\armygrp.cpp:1464, dc 0x4fab4

#endif  /* HOMM3_ARMYGRP_H */
