#ifndef HOMM3_ARMY_H
#define HOMM3_ARMY_H

#include "va.h"
#include "includes.h"

#include <deque>
#include <vector>

#include "armygrp.h"
#include "herospec.h"
#include "monframeinfo.h"

class hero;
class armyGroup;
class town;
class sample;
class CSprite;

inline const char* getArmyName(int type, int count);

// Combat-grid directions as path.cpp's walkers consume them: 0..5 are
// the six hex neighbours (combatManager::adjacentCells columns); 6/7
// are the two WIDE-CREATURE extra attack slots appended after them,
// remapped to a real neighbour of the far hex by `facing` before the
// table lookup (GetAdjacentCellIndex 0x523d90; GetAttackMask pre-sets
// their mask bits for one-hex creatures; OppositeDirection pairs them
// 6<->7). Names are bootstrap inventions - no DC/NH3API roster
// survives for the direction ids.
enum ECombatDirection {
    COMBAT_DIRECTION_0 = 0x0,
    COMBAT_DIRECTION_1 = 0x1,
    COMBAT_DIRECTION_2 = 0x2,
    COMBAT_DIRECTION_3 = 0x3,
    COMBAT_DIRECTION_4 = 0x4,
    COMBAT_DIRECTION_5 = 0x5,
    COMBAT_DIRECTION_COUNT = 0x6,
    COMBAT_DIRECTION_WIDE_UPPER = 0x6,
    COMBAT_DIRECTION_WIDE_LOWER = 0x7
};

// Two-hex creature orientation (army::facing, +0x44). ValidAttack
// dispatches on it with a real `switch` - byte-proven at 0x523bb0,
// where retail emits VC6's switch chain (`sub ecx,0` / `dec ecx`) and
// lays the DEFENDER arm out first, which an if/else-if chain on the
// same values cannot reproduce. Names are bootstrap inventions.
enum EFacing {
    FACING_ATTACKER = 0x0,
    FACING_DEFENDER = 0x1
};

// ValidAttack's target-filter modes (byte-derived from the criteria
// switch at 0x523bb0; names are bootstrap inventions): SELF accepts
// only this army's own cell, ENEMY a hostile army, OCCUPIED any army.
enum EAttackCriteria {
    ATTACK_CRITERIA_SELF = 0x0,
    ATTACK_CRITERIA_ENEMY = 0x1,
    ATTACK_CRITERIA_OCCUPIED = 0x2
};

// The eight rows of combatManager::wallTargets. Retail DamageWall dispatches
// on all eight values; names remain ordinal because no local roster names
// the individual segments.

// DECLARED HERE RATHER THAN IN cmbtmgr.h, WHICH IS WHERE IT READS: that
// header INCLUDES this one, so a domain army's own members take by value
// - attack_wall's `wall` - cannot be named there and seen here. The move
// is include-set NEUTRAL for every TU that takes both headers (the same
// nine enumerators, one file earlier in the same closure); hero.cpp is
// the only consumer that gains them, and its rows were re-measured
// unchanged.
enum TWallTargetId {
    WALL_TARGET_0 = 0,
    WALL_TARGET_1 = 1,
    WALL_TARGET_2 = 2,
    WALL_TARGET_3 = 3,
    WALL_TARGET_4 = 4,
    WALL_TARGET_5 = 5,
    WALL_TARGET_6 = 6,
    WALL_TARGET_7 = 7,
    WALL_TARGET_COUNT = 8
};

enum EArmySpellCancelType {
    ARMY_CANCEL_SPELLS_AFTER_MOVE = 0,
    ARMY_CANCEL_SPELLS_AFTER_ATTACK = 1,
    ARMY_CANCEL_SPELLS_AFTER_DAMAGE = 2
};

// One row of the ballistics table the catapult fires by, indexed by the
// attacker's Ballistics mastery. EIGHT BYTES, byte-proven by its two
// readers: AttackWall (0x445d30) indexes the table pointer at 0x679c84
// with `movsx ebx, [edx + eax*8 + 4]` and hands `edx + eax*8` straight
// to attack_wall as the row, and attack_wall (0x445ec0) movsx-loads
// +0x0..+0x3 and +0x5..+0x6 off that pointer. Every read is a movsx, so
// every field is a SIGNED char.

// The first four are per-target hit chances out of 100, and
// attack_wall's own switch is what maps them: +0x1 answers the two
// tower rows (WALL_TARGET_0 and _6), +0x0 the keep (WALL_TARGET_7),
// +0x2 WALL_TARGET_3, and +0x3 every remaining segment. Names stay
// ordinal past that pairing - no roster reaches the row.

// Dreamcast's field list names this type in army's unconditional private
// attack_wall overload. It therefore cannot remain a TU-view score control.
struct type_ballistics_traits {
    signed char m_chanceToHitMainBuilding;
    signed char m_chanceToHitTower;
    signed char m_chanceToHitDrawbridge;
    signed char m_chanceToHitWall;
    // Shots per bombardment: AttackWall reads it as the trip count of
    // the loop that calls attack_wall.
    signed char m_shots;                // +0x4
    // The cumulative rolls that decide how many levels one hit
    // takes off: attack_wall subtracts [0] and then [1] from a single
    // Random(1, 100) and stops at the first non-positive remainder, so
    // the answer is 0, 1 or 2. The ballist.txt parser independently
    // proves THREE source entries at +5..+7; only the first two need be
    // subtracted because falling through both already selects level 2.
    signed char m_levelChance[3];       // +0x5
};
SIZE(type_ballistics_traits, 8);

// The four rows themselves - one per Ballistics mastery - reached
// through the cell at 0x679c84, which is a REFERENCE TO AN ARRAY OF
// FOUR and not a pointer: the Dreamcast public is
// ?const_ballistics_traits@@3AAY03$$CBUtype_ballistics_traits@@A,
// i.e. `const type_ballistics_traits (&)[4]`, and that is exactly why
// retail's read is an indirect load off 0x679c84 followed by an
// 8-byte-strided index rather than a direct displacement. hero.cpp
// OWNS the definition - the cell sits in .data, not .rdata, because
// its initialize_ballistics_table (dc 0xca984, still a carcass) is
// what binds the reference at startup, immediately before the
// experience ladder at 0x679c88 - so no DATA claim is made here.
// army.cpp is the only located reader.
extern const type_ballistics_traits (&g_constBallisticsTraits)[4];

// Opaque head model. The 0x548 stride and the 0x54cc array base in
// combatManager are byte-proven by hexcell::get_army/get_dead_army
// (0x4e7170/0x4e71b0: index = side*21 + slot, scaled by 0x548 from
// gpCombatManager + 0x54cc). ResetHitByCreature (0x465fe0) clears the
// byte at army+0xf0 (manager+0x55bc); the dc-attested IsWinner walk
// tests the int at army+0x84 against -1. Field names provisional.
// TResourceHandle<T> - PROVISIONAL NAME, and a PROVISIONAL TYPE: no roster
// names it. It is Complete-era, and the Dreamcast build does not have it -
// DC type 0x1A63 is a plain LF_POINTER to 0x17D3 and 0x1FE5 a plain LF_ARRAY
// of 32 bytes of those pointers. The retail x86 bytes are the whole
// evidence, and they are unambiguous: army's compiler-generated copy
// constructor (0x437a00) copies stdIcon, missileIcon and every element of
// armySample[8] as
//     mov ecx,[src+off] / xor edx,edx / cmp ecx,edx / mov [dst+off],ecx
//     / je +5 / inc dword ptr [ecx+0x18]
// with an eight-iteration counted loop over the array (`mov [ebp+8],8` ...
// `dec edi` / `jne`) and unwind-state stores across the run - state 0 right
// after stdIcon, a byte store of state 2 inside the loop. A raw pointer
// member cannot produce any of that. +0x18 is resource::ReferenceCount
// (resource.h: vptr 0, Name[13] 4..0x10, resType 0x14, ReferenceCount 0x18),
// and image_height at +0x16c is copied as a PLAIN dword in the middle of the
// run, which proves the refcount belongs to the pointer members themselves
// rather than to a wrapper spanning the band.

// Retail retains the handle destructor at 0x43cb10: a non-null resource
// receives its virtual dispose() call. The ordinary army destructor at
// 0x43d400 owns member cleanup in army.cpp. A TU instantiating these handle
// constructors/destructors must see the complete resource types.
template<class T>
class TResourceHandle {
public:
    T* m_resource;

    TResourceHandle() { m_resource = 0; }
    TResourceHandle(const TResourceHandle& that)
    {
        m_resource = that.m_resource;
        if (m_resource)
            m_resource->addRef();
    }
    ~TResourceHandle() { if (m_resource) m_resource->dispose(); }

    TResourceHandle& operator=(T* newResource)
    {
        m_resource = newResource;
        return *this;
    }
    operator T*() const { return m_resource; }
    T* operator->() const { return m_resource; }
};

class army {
public:
    // Dreamcast Army.h enum; retail Teleport passes the two corresponding
    // immediate values to the independently located play_sample body.
    enum TSampleID {
        WALK_SAMPLE = 0,
        ATTACK_SAMPLE = 1,
        WINCE_SAMPLE = 2,
        SHOOT_SAMPLE = 3,
        DIE_SAMPLE = 4,
        DEFEND_SAMPLE = 5,
        PRE_WALK_SAMPLE = 6,
        POST_WALK_SAMPLE = 7,
        MAX_SAMPLES = 8
    };

    // DC army.iDrawPriority (members.csv army@8), UNSHIFTED in this band
    // exactly like groupToAttack 16/+0x10 two lines below. army::Walk
    // (0x43f0b0) writes three literals into it - 3 for a step to
    // direction 0 or 5, 7 for a step to 2 or 3, and 4 once the stack has
    // been re-placed in the grid - i.e. the draw order a moving stack
    // takes against the row it is entering. Pad slice, behind the move
    // view with the rest of Walk's surface.
    // The five animation-state bytes that open the record, all proven by
    // combatManager::PowEffect (0x468990) and named from the Dreamcast
    // members.csv run army@0..army@4. PowEffect's first walk writes
    // bShowRangeFrames from the current sequence, picks iNextFrameType
    // (-1 = nothing to play), loads iRemainingFramesToPlay out of
    // CSprite::GetNumFrames and raises iDrawPriority to at least 5; its
    // animation loop then drives all five. bShowAttackFrames and
    // iDrawPriority are now canonical: CastSpell's shared post-cast walk
    // independently writes both, so hiding either behind a TU view would
    // discard proven class structure merely to preserve optimizer state.
    unsigned char m_showAttackFrames;     // +0x00
    // The retained copy constructor (0x437a00) proves these five
    // animation-state bytes and the byte at +0x0c are independent
    // members: retail copies each one and leaves their alignment bytes
    // untouched, so no pad is spelled after them.
    unsigned char m_showRangeFrames;      // +0x01
    signed char m_showAttackFrameType;    // +0x02
    signed char m_nextFrameType;          // +0x03
    signed char m_remainingFramesToPlay;  // +0x04
    int m_drawPriority;                   // +0x08
    unsigned char m_showTroopCount;              // +0x0c
    // Grid identity, byte-proven by ValidAttack (0x523bb0): the target
    // hexcell's armySide/armySlot pair compares against these.

    int m_side;                     // +0x10 == DC groupToAttack
    int m_slot;                     // +0x14 == DC indexToAttack
    // Retype in place: army's own constructor (0x43d250) zeroes this slot
    // as a DWORD (`mov dword ptr [esi+0x18], ebx`) in the same store run
    // that clears pathTarget, field_100 and field_104. Name is the house
    // ordinal placeholder - the width is proven, the role is not.
    int m_attackLimit;                 // +0x18
    // ValidPath stores the validated destination here on success.
    int m_pathTarget;               // +0x1c
    // DC army.bShowPowEffect (members.csv army@32) and
    // army.iRoundsLeftBeforeVanish (army@44) - the low run this class
    // already pairs UNSHIFTED, IsMoving 48/+0x30 through origSpeed
    // 100/+0x64. ResetRound (0x447120) raises the first when the round's
    // poison bites and counts the second down, sending the stack to
    // ProcessDeath the moment it reaches zero.
    unsigned char m_showPowEffect;  // +0x20
    // DC army.iMirrorSourceIndex / army.iMirrorDestIndex (members.csv
    // army@36 and @40, the same unshifted low run bShowPowEffect 32 and
    // iRoundsLeftBeforeVanish 44 already pair). InitClean (0x43d5c0)
    // resets both to -1 out of the same `or eax,-1` it uses for
    // iPostPowSpellToCast, originalIndex and numTroopsToShowOverride,
    // which is what fixes them as ints rather than the pad.
    int m_mirrorSourceIndex;        // +0x24
    int m_mirrorDestIndex;          // +0x28
    int m_roundsLeftBeforeVanish;   // +0x2c
    // DC army.IsMoving (members.csv army@48, which is retail +0x30 -
    // the whole DC run 48..100 lands on retail 0x30..0x64 unshifted).
    // army::Fly (0x4b4a40) raises it for the duration of the flight
    // animation and clears it in the same quick-combat-gated tail that
    // stops the walk sample.
    unsigned char m_isMoving;       // +0x30
    // DC army.LetsPretendImNotHere (members.csv army@49, the byte
    // straight after IsMoving in the run this header already pairs
    // unshifted). SetupAnimation (0x446830) raises it across the single
    // combatManager::DrawFrame that captures the clean background and
    // drops it again immediately after - "draw the field without me".
    unsigned char m_letsPretendImNotHere; // +0x31
    // Creature roster id: ai_tactical compares it against the war
    // machines 0x93/0x94 (get_ranged_attack_value 0x435cb0,
    // set_melee_enemies 0x43bf20) and 0x95 (get_damage_value 0x436e30).
    // enum, not int: hero::modify_spell_damage takes a TCreatureType and
    // retail's S_PUB32 mangling for that slot is `W4TCreatureType`. The
    // ELABORATED spelling parses in every include order without armygrp.h
    // being visible, which is why this needs no view macro.
    enum TCreatureType m_creatureType;   // +0x34, DC army::armyType
    // Occupied combat cell. ai_tactical's find_attack_hex (0x436840)
    // feeds it straight into check_adjacent_hexes as the enemy hex,
    // and the type_AI_spellcaster ctor walks armies by it.
    int m_gridIndex;                // +0x38
    // DC army.currFrameType / army.currFrameIndex (members.csv army@60
    // and @64). army::Fly drives BOTH: it parks frame type 2 (the stand
    // pose) with index 0 before the drawbridge redraw, switches to type
    // 0 (cs_walk) for the flight, and uses currFrameIndex itself as the
    // per-step frame loop's induction variable.
    int m_currFrameType;            // +0x3c
    int m_currFrameIndex;           // +0x40
    // 0 = attacker-facing, 1 = defender-facing: selects the 6/7
    // special-direction remaps in GetAdjacentCellIndex.
    int m_facing;                   // +0x44
    // DC army.walkDirection (members.csv army@72), the direction id of
    // the step in progress - the run 48..100 this class already pairs
    // unshifted (facing 68/+0x44 one line above, numTroops 76/+0x4c one
    // line below). army::Walk (0x43f0b0) stores its `direction`
    // parameter here before it touches the animation.
    int m_walkDirection;            // +0x48
    // Stack size: set_melee_enemies (0x43bf20) feeds it to
    // get_average_damage as the attacking creature count.
    int m_numTroops;                // +0x4c
    // Dreamcast names the adjacent display override and the count restored
    // during this battle. Retail get_surrender_cost subtracts the latter
    // before pricing the surviving stack, independently fixing +0x54.
    int m_numTroopsToShowOverride;  // +0x50
    int m_numTroopsBattleResurrected; // +0x54
    // Damage already carried by the stack's top creature: the AI adds
    // its expected extra damage to it and compares against hitPoints to
    // decide whether a boost saves a creature (get_defense_boost_value
    // 0x4387c0). Name provisional.
    int m_topCreatureDamage;        // +0x58
    // Original army-group slot, restored by combatManager::UpdateArmyGroup
    // when it writes surviving stacks back after combat.
    int m_originalIndex;            // +0x5c (DC origPos)
    // The stack's size at the START of the combat, so that
    // origNumTroops - numTroops is the count this side destroyed:
    // CalculateGainedExperience (0x46a350) multiplies exactly that
    // difference by the creature's table hitPoints to price the award.
    // The NAME is the DC roster's army.origNumTroops - and the DC
    // layout is aligned with retail's across this whole run
    // (armyType/creatureType 0x34, facing 0x44, numTroops 0x4c,
    // residualDamage 0x58, origPos 0x5c, origNumTroops 0x60,
    // origSpeed 0x64 all land on the offsets already proven here),
    // so the pairing is positional, not just nominal.
    int m_origNumTroops;            // +0x60
    // Unmodified speed: get_speed_value (0x439550) adds the spell's
    // increase to it and re-times the stack against the result, while
    // GetSpeed() returns the modified value.
    int m_baseSpeed;                // +0x64
    // DC army.origWalkCycleTime (members.csv army@104), the slot the
    // iLuckStatus note below always placed here. Sliced 2026-08-20 by
    // CancelIndividualSpell (0x444510): its HASTE and SLOW arms both
    // restore frameInfoWalkCycleTime (+0x158) from this word.
    int m_origWalkCycleTime;         // +0x68
    // DC army.origHitPoints (members.csv army@108), the third of the
    // orig* trio after origPos 92/+0x5c and origSpeed 100/+0x64 that
    // this class already carries unshifted. ResetRound recomputes
    // hitPoints from it every round rather than from the live word.
    int m_origHitPoints;             // +0x6c
    // DC army.iLuckStatus (members.csv army@112), UNSHIFTED in this band
    // exactly like origHitPoints 108/+0x6c above and origWalkCycleTime
    // 104/+0x68 in the pad. do_multi_head_attack (0x440310) clears it
    // once per head it lands, between the Damage call and the
    // fire-shield test, so the luck roll is spent by the first head and
    // the rest of the sweep swings plain. Pad slice - the include-set
    // canaries do not move for one.
    int m_luckStatus;               // +0x70
    // +0x74 is an EMBEDDED copy of the creature's traits row - the
    // Dreamcast roster's `TCreatureTypeTraits sMonInfo` at 116, ADOPTED
    // AS SUCH 2026-09-05. The retail offsets this class had already
    // proven agree with the DC record field for field: `creatureId`
    // +0x84 is sMonInfo.attributes +0x10, `hitPoints` +0xc0 is
    // sMonInfo.hitPoints +0x4c, `field_c4` +0xc4 is sMonInfo.speed
    // +0x50, and hero::modify_spell_damage (0x4e5760) hands
    // `[army + 0x78]` to GetHeroSpellBonus as the target LEVEL, which
    // is sMonInfo + 4, TCreatureTypeTraits::level.

    // The include cost is NIL: every one of the 24 TUs whose closure
    // reaches army.h already had armygrp.h in that closure, so the
    // `#include "armygrp.h"` this member needs adds no declarator to any
    // consumer's include-set population.

    // THE SLOT IS FIXED BY THE RECORD, not by a roster. The DC
    // TCreatureTypeTraits run hitPoints 60 / speed 64 / attackSkill 68
    // / defenseSkill 72 / damageLowBound 76 / damageHighBound 80 /
    // numShots 84 lands on retail +0x4c/+0x50/+0x54/+0x58/+0x5c/+0x60/
    // +0x64 inside the row (a flat +16), which this header already
    // pairs field for field as hitPoints..shotsLeft; the DC record then
    // ends 88/92 wanderingLow/wanderingHigh at 96 bytes where retail's
    // stride is 0x74 - byte-proven by get_resurrection_size's own
    // akCreatureTypeTraits[0x30].hitPoints, which retail addresses at
    // +0x160c == 0x30*0x74 + 0x4c. The extra dword retail carries is
    // the one CRTRAITS.TXT column the Dreamcast port dropped, and it
    // sits exactly here at +0x68 in the row. Name provisional.

    // This is an original class member, not a TU-specific score control.
    // Hiding its declaration changed VC6's member-handle population and
    // could manufacture a higher local score in unrelated consumers.
    // -> sMonInfo.hasSpell        (+0xdc, row +0x68)
    // -> sMonInfo.wanderingLow/wanderingHigh (+0xe0/+0xe4)
    TCreatureTypeTraits m_monInfo;  // +0x74 .. +0xe8, 116 B
    // NAMED 2026-08-15 from the Dreamcast member table: DC army@212 is
    // show_fire_shield against this band's already-anchored +20 shift
    // (DC hitByCreature 220 = retail +0xf0, six lines below).
    // do_multi_head_attack (0x440310) is the witness - it raises the
    // byte on the head it burned, and only when adjust_damage handed
    // back a non-zero fire component.
    unsigned char m_showFireShield;  // +0xe8
    // Damage sets m_someUnitsDamaged on every hit and m_allUnitsKilled when the
    // stack becomes empty.
    unsigned char m_someUnitsDamaged; // +0xe9
    unsigned char m_allUnitsKilled;   // +0xea
    // DC army.iPostPowSpellToCast (members.csv army@216, a SpellID -
    // retail sits a flat +0x14 above the DC record from hitByCreature
    // onward, so DC 216 lands on +0xec). PowEffect's post-animation
    // walk is its only decoded reader: any stack whose value is not -1
    // gets that spell cast on its own hex, and the slot is reset to -1
    // straight after. Spelled int because SpellID's enum lives in a
    // header this one does not include. The ROUND view is admitted to
    // the same slice 2026-08-20: InitClean (0x43d5c0) resets it to -1
    // out of the same shared `or eax,-1` it uses for originalIndex and
    // the iMirror pair, which is a second and independent witness that
    // +0xec is a dword field and not pad.
    int m_postPowSpellToCast;         // +0xec
    unsigned char m_hitByCreature;  // +0xf0
    // The side that OWNS this stack, written once by Init and never by
    // a spell: the hypnotize flip is applied ON READ by
    // get_controlling_side (0x440140), which is why is_enemy (0x442880)
    // compares its own flipped side against the other stack's raw one
    // and why get_owner (0x4426d0) reads this field directly while
    // get_controller (0x442690) flips it. FindPath forwards the flipped
    // value into FindCombatPath.
    int m_combatSide;               // +0xf4
    // Bit position of this stack in the AI's "already counted" masks:
    // get_hex_attack_value (0x436180) builds 1 << it and folds the bit
    // into the caller's checked word. It is also the stack's SLOT:
    // PlaceArmyInGrid (0x4687c0) narrows this word to a byte and stores
    // it as the occupied hexcell's armySlot, exactly as it stores
    // combatSide (+0xf4) as armySide. Renaming waits on a lane that
    // owns the ai_tactical call sites.
    int m_bitIndex;                 // +0xf8

    // The evidence for each field follows in offset order, as it was
    // recorded while the fields were sliced out one at a time.
    // +0x10c, the DC roster's `yModify` (army@248, a char*) - retail
    // sits a flat +0x14 above the DC record from hitByCreature onward,
    // the same shift combatSide/bitIndex already carry (DC group 224 ->
    // +0xf4, index 228 -> +0xf8, sMonFrameInfo 252 -> +0x110), so DC
    // 248 lands here. range_attack (0x440160) clears it as its first
    // statement and nothing located yet reads it.
    // DC army.iLastFidgetTime (members.csv army@232, the flat +0x14
    // this band carries from hitByCreature onward - the same shift that
    // lands DC index 228 on combatSide's neighbour +0xf8). InitClean
    // (0x43d5c0) stamps GameTime::Get() into it, the command.obj cycle
    // reset rewrites it from GameTime, and the name is independently
    // corroborated by set_inside_area_effect (0x43efe0), whose whole
    // animation arm is about the cs_fidget sequence.
    unsigned long m_lastFidgetTime; // +0xfc
    // The per-frame DRAW OFFSET a stack is currently displaced by,
    // byte-proven by MirrorImage (0x5a6c70): it sets the pair from the
    // difference between the source hex's and the clone's own hexcell
    // screen coordinates (+0x1c4 / +0x1c6), divides both by 16, counts
    // them down over sixteen DrawFrame steps and zeroes them at the end
    // - i.e. the clone slides out of the caster's hex into its own.
    // +0x100 carries the Y difference and +0x104 the X one. Names stay
    // ADDRESS ORDINALS: the behaviour is proven, the roster has no row
    // for either, and nothing else decoded reads them yet.
    int m_ySpecialMod;                 // +0x100
    int m_xSpecialMod;                 // +0x104
    // +0x108, the DC roster's `bPowSequenceComplete` (army@244, the same
    // flat +0x14 the band above carries). PowEffect clears it for every
    // stack before the animation loop and raises it the frame a stack
    // falls back to cs_wait, which is what stops that stack advancing
    // for the rest of the sequence.
    // An INT, not the byte the name suggests (byte-proven 2026-08-20):
    // PowEffect both TESTS and STORES it a dword wide -
    // `mov eax,[esi+0x108] / test eax,eax` and
    // `mov dword ptr [esi+0x108],1` - where a char field would emit
    // `mov al` / `mov byte ptr`. Measured +0.03 on that body.
    int m_powSequenceComplete;      // +0x108
    char* m_yModify;                 // +0x10c
    // sMonFrameInfo.iMissileOffset (DC SMonFrameInfo@0, short[6] -
    // monframeinfo.h carries the record), the three launch-point pairs
    // for the ranged poses ur/r/dr. Byte-proven by attack_wall
    // (0x445fd0): the first angle estimate reads the middle pair
    // [2]/[3] and the aimed shot reads [2*pose]/[2*pose+1], both
    // movsx'd shorts at +0x110..+0x11b.
    // -> sMonFrameInfo.iMissileOffset          (+0x110, row +0x00)
    // -> sMonFrameInfo.fArrowAngle[12]         (+0x11c, row +0x0c)
    // sMonFrameInfo.iExtraNumTroopsXOffset (+0x3c of the record):
    // DrawToBuffer (0x43e140) shifts the troop-count box by it when the
    // box's neighbour hex is free.
    // -> sMonFrameInfo.iExtraNumTroopsXOffset  (+0x14c, row +0x3c)
    // Two more fields sliced out of the embedded animation-traits row,
    // both byte-proven by DoBolt (0x5a5c20): its reset tail guards the
    // whole attack-animation flush on +0x150 and divides +0x15c by the
    // sequence's frame count for the per-frame delay, which is exactly
    // SMonFrameInfo's iAttackFrames (+0x40 of the row) and
    // iAttackStartCycleTime (+0x4c). The pair is sliced the same way
    // army::Fly's two already are, and in the same band comment's terms:
    // read the fields you need, do not model the record.
    // -> sMonFrameInfo.iAttackFrames           (+0x150, row +0x40)
    // Embedded SMonFrameInfo::iFidgetFrequency (+0x44 in the 0x54 row).
    // ResetCycleTimers compares it with 51 and uses it as Random's upper
    // bound before retiming iLastFidgetTime.
    // -> sMonFrameInfo.iFidgetFrequency        (+0x154, row +0x44)
    // -> sMonFrameInfo.iWalkCycleTime          (+0x158, row +0x48)
    // sMonFrameInfo.iAttackStartCycleTime (DC TMonFrameInfo@76, between
    // iWalkCycleTime@72 and iFlightPixelSpan@80 exactly as the two
    // proven neighbours sit here). Byte-proven by cast_spell (0x448260):
    // the cast loop's per-frame delay is this word over the sequence's
    // frame count.
    // -> sMonFrameInfo.iAttackStartCycleTime   (+0x15c, row +0x4c)
    // -> sMonFrameInfo.iFlightPixelSpan        (+0x160, row +0x50)
    SMonFrameInfo m_monFrameInfo;  // +0x110 .. +0x164, 84 B
    // DC army.stdIcon (members.csv army@336). army::Fly asks it for the
    // walk sequence's frame count through CSprite::GetNumFrames, which
    // is what fixes the type: the retail expansion is that inline's
    // exact three-part shape (numSequences at +0x28, validSeqMask at
    // +0x2c, s at +0x1c) against the CSprite layout csprite.h proves.
    // NOT A RAW POINTER - see the TResourceHandle note above the class.
    // ADOPTED 2026-09-05 and byte-confirmed: army's compiler-generated copy
    // constructor (0x437a00) now reproduces the whole refcount run -
    // stdIcon, missileIcon, image_height's plain dword in the middle of it,
    // and the eight-iteration armySample loop - instruction for instruction
    // against retail, fn+0x192..fn+0x202.
    TResourceHandle<CSprite> m_stdIcon;    // +0x164
    // DC army.missileIcon (members.csv army@340, right between stdIcon
    // @336 = +0x164 and image_height @344 = +0x16c). attack_wall
    // (0x445fd0) hands it to ShootBallisticMissile as the CSprite*
    // missile, which is what fixes the type.
    // image_height is DC army@344: MidY (0x446630) subtracts HALF of it
    // from the hexcell's own y, and LoadResources (0x43dd62) writes it
    // as `0x10b - <stdIcon frame metric>` - the stack's own vertical
    // span on the combat field. InitClean zeroes it.
    TResourceHandle<CSprite> m_missileIcon;  // +0x168
    int m_imageHeight;             // +0x16c
    // DC army::armySample is sample*[8] at +0x15c; retail's preceding STL
    // expansion shifts it to +0x170, independently confirmed by play_sample.
    TResourceHandle<sample> m_armySample[8];  // +0x170
    // Ordering key the AI compares BETWEEN stacks: should_attack_now
    // (0x436c60) refuses to cast now when any other still-able stack
    // on our side outranks the target's own value here. Name pending a
    // writer.
    int m_expectedMoveOrder;                // +0x190
    // DC army.numSpellInfluences (members.csv army@384, which is retail
    // +0x194 on the flat +0x14 shift this class carries from
    // hitByCreature 220/+0xf0 onward).

    // The two spell rows and their queue are original aggregate members.
    // They stay source-visible in every TU; replacing them with padding in
    // selected consumers changed the class declaration stream seen by C1.
    int m_numSpellInfluences;       // +0x194
    int m_spellInfluence[81];          // +0x198 .. +0x2db
    // THE SECOND ROW, and it is the one the spellInfluence note above
    // already predicted: "DC's own spellInfluence[80] at 388 with
    // spell_level[80] straight after it at 708". Retail's pair is
    // 81 wide, so +0x198 + 81*4 == +0x2dc is where the second row
    // starts, and DC's 708 - 388 == 320 == 80*4 is the same
    // back-to-back layout one element shorter.
    // BYTE-PROVEN by ai_tactical's get_cancel_value (0x439a80), which
    // walks ONE pointer over this row - `p = &current_army[0x304]`,
    // stride 4 - and reads the ROUNDS row through the same pointer at
    // a constant `-0x144`, i.e. exactly 81 dwords lower. It feeds the
    // dereference straight into a type_enchant_data's `mastery` field,
    // which is what names the row: the mastery the standing spell was
    // cast at.
    // THE SPELL QUEUE, and it is a std::deque - not this header's
    // guess but retail's own container arithmetic. InitClean (0x43d5c0)
    // calls ONE function on the object at +0x420, handing it the two
    // SIXTEEN-BYTE values at +0x424 and +0x434 BY VALUE and taking a
    // sixteen-byte answer back through a hidden return pointer; that
    // callee (0x448db0) opens by comparing the +0xc words of the two
    // and, when they differ, computes `((a-b)>>2 + 0x3fffff) << 10`
    // plus a second scaled difference. That is Dinkumware's
    // `deque::erase(iterator, iterator)` with its map/block distance,
    // over 4-byte elements and a 1024-entry block - so +0x424 is
    // `_First`, +0x434 is `_Last`, and the whole record is
    // allocator(4) + two 16-byte iterators + _Map + _Mapsize + _Size =
    // 48 bytes, 0x420..0x44f. The call is `clear()`, which /Ob2 folds
    // into `erase(begin(), end())` exactly as seen.

    // DC names it (members.csv army@1028 SpellInfluenceQueue, nested
    // type TSpellQueue) and the DC procedure inventory for army.obj records
    // its whole COMDAT set, `deque<enum SpellID, allocator<enum SpellID>,
    // 0>`. THE OFFSET IS THE LAYOUT PROOF: DC 1028 -> retail 0x420 is a
    // 28-byte shift while DC 1072 retaliationCount -> retail +0x454 is
    // 36, and the difference is exactly the 8 bytes Dinkumware's deque
    // is bigger than STLport's - the same "retail's preceding STL
    // expansion shifts it" the armySample note records.

    // Spelled `int` rather than SpellID for the reason
    // iPostPowSpellToCast above already carries: that enum lives in a
    // header this one does not include. Same 4-byte element, same
    // codegen; only the mangled COMDAT name differs, and those are
    // unclaimed rows whose reloc names are cosmetic anyway.
    typedef std::deque<int> TSpellQueue;
    int m_spellLevel[81];          // +0x2dc .. +0x41f
    TSpellQueue m_spellInfluenceQueue;  // +0x420 .. +0x44f
    float m_paletteEffect;          // +0x450 (DC army@1068)
    // Retaliations left this round: simulate_attack (0x4359b0) only
    // lets the defender strike back while it is positive, and the DC
    // roster has army::set_retaliation_count feeding it.
    int m_retaliationCount;         // +0x454
    // The two signed amounts get_average_damage (0x4426f0) pairs with
    // the Bless and Curse round counters above: it adds +0x458 to
    // maxDamage under Bless and subtracts +0x45c from minDamage under
    // Curse. Sliced from that body alone - the amounts row is offset
    // from the rounds row by one slot against the morale/luck pair
    // below, so no array relation is asserted here.
    int m_blessAmount;              // +0x458
    int m_curseAmount;              // +0x45c
    // DC army.antiMagicSpellLevel (members.csv army@1084, the slot
    // before bloodlustBonus 1088/+0x464 on the same +0x24 shift).
    // PROVEN FROM BOTH SIDES, by two lanes independently:
    //   * the WRITER - army::SetSpellInfluence (0x4448f0): its ANTI_MAGIC
    //     arm stores the per-mastery amount here and then cancels every
    //     standing spell whose traits level sits below it;
    //   * the READER - combatManager::SpellCastWorkChance (0x5a8090): a
    //     cast is refused outright while the stack's Anti-Magic round
    //     counter (spellInfluence[34], +0x220) is up AND the cast spell's
    //     own akSpellTraits level sits BELOW this dword.
    // That is Anti-Magic's rule from each end, and the same rounds/amount
    // pairing the Bless and Curse fields above already carry. Retyped IN
    // PLACE out of the pad, so the declarator count does not move.
    int m_antiMagicSpellLevel;      // +0x460
    // The two attack amounts get_adjusted_attack pairs with the
    // Bloodlust and Precision round counters above.
    int m_bloodlustAmount;          // +0x464
    int m_precisionAmount;          // +0x468
    // Three more of the DC amounts run (members.csv army@1096/1100/1108
    // weaknessPenalty / toughskinBonus / prayerBonus, on the same +0x24
    // shift slayerLevel 1128/+0x48c below anchors), each byte-proven by
    // CancelIndividualSpell (0x444510): its WEAKNESS arm adds +0x46c
    // back onto attackSkill, its STONE_SKIN arm subtracts +0x470 from
    // defenseSkill, and its PRAYER arm subtracts +0x478 from attack,
    // defense and speed. The slot between them, DC 1104
    // disruptiverayPenalty/+0x474, stays a pad until a body reads it.
    int m_weaknessPenalty;          // +0x46c
    int m_toughskinBonus;           // +0x470
    char m_disruptiverayPenalty[0x4];
    int m_prayerBonus;              // +0x478
    int m_moraleBonus;              // +0x47c
    int m_moralePenalty;            // +0x480
    int m_luckBonus;                // +0x484
    int m_luckPenalty;              // +0x488
    // Slayer's mastery level: get_adjusted_attack admits creature bit 7
    // at any level, bit 8 from 2 up and bit 9 from 3 up.
    int m_slayerLevel;              // +0x48c
    // DC army.counterstrokeBonus (members.csv army@1136 on the flat
    // +0x24 shift retaliationCount 1072/+0x454 fixes for this run;
    // slayerLevel 1128/+0x48c two lines above is the same shift).
    // ResetRound adds it to the round's retaliation allowance exactly
    // while spellInfluence[SPELL_COUNTERSTRIKE] is standing.
    // DC army.joustBonus (members.csv army@1132), the same +0x24 shift
    // one slot earlier - and the name is exactly what the field does:
    // do_multi_head_attack (0x440310) hands it straight to
    // adjust_damage's fifth parameter, the one the DC prototype calls
    // `distance`, which is the Champion's per-hex charge bonus.
    // process_move_then_attack clears it before movement and once more
    // after the strike.
    int m_joustBonus;                // +0x490
    int m_counterstrokeBonus;        // +0x494
    // Frenzy's defense-to-attack conversion factor: while frenzyRounds
    // is up, get_adjusted_attack answers
    // `get_adjusted_defense(enemy, 0) * this + attack`. It is also what
    // makes get_adjusted_defense's own Frenzy early-out consistent -
    // the defense is spent, not counted twice.
    float m_frenzyFactor;           // +0x498
    // The damage this stack still does while it is shaking off a blind:
    // ComputeAttackerDamageReduction (0x443b90) multiplies the whole
    // reduction by it whenever residualBlindness (+0x4c0) is up. DC name
    // (members.csv army@1144 blindFactor), and the DC run 1140..1181 -
    // frenzyAdjust / blindFactor / fire_shield_strength ... /
    // shieldDamageFactor / airShieldDamageFactor / residualBlindness /
    // residualParalyze - lands on retail +0x498..+0x4c1 unshifted
    // against the three names this header already proved from bodies.
    float m_blindFactor;            // +0x49c
    // Active Fire Shield multiplier.  get_fire_shield_strength loads
    // this float whenever fireShieldRounds is non-zero; otherwise the
    // innate Efreet Sultan path supplies the shared 0.2f constant.
    float m_fireShieldStrength;     // +0x4a0
    // DC army.poison_penalty (members.csv army@1152), the word straight
    // after fire_shield_strength 1148/+0x4a0 in the same DC run. It is
    // a MULTIPLIER, not a count: ResetRound subtracts 0.1f from it once
    // per round, floors the result at 0.5 and rescales the stack's
    // hitPoints by what is left.
    float m_poisonPenalty;           // +0x4a4
    // The four Protection-from-<school> damage multipliers, DC-named
    // (members.csv army 1156/1160/1164/1168 protectionFrom{Air,Fire,
    // Water,Earth}Factor) and pinned to these retail offsets by the same
    // unshifted DC run that fixes their neighbours: DC 1152
    // poison_penalty is retail +0x4a4 and DC 1172 shieldDamageFactor is
    // retail +0x4b8, so the four words between them are +0x4a8..+0x4b4
    // in DC order. ModifySpellDamageForSpells (0x5a7bb0) reads all four
    // and pairs each with the round counter at +0x210..+0x21c above.
    float m_protectionFromAirFactor;   // +0x4a8
    float m_protectionFromFireFactor;  // +0x4ac
    float m_protectionFromWaterFactor; // +0x4b0
    float m_protectionFromEarthFactor; // +0x4b4
    // The two damage multipliers ComputeDefenderDamageReduction pairs
    // with shieldRounds and airShieldRounds above.
    float m_shieldFactor;           // +0x4b8
    float m_airShieldFactor;        // +0x4bc
    // The two "still recovering" flags ComputeAttackerDamageReduction
    // pairs at its tail: residualBlindness scales the attack by
    // blindFactor (+0x49c), residualParalyze by Blind's own advanced
    // mastery percentage, and a stack carrying BOTH takes the smaller of
    // the two. DC names (members.csv army@1180 / @1181), both T_UCHAR,
    // and the retail body loads each with a byte `mov`/`test` pair.
    unsigned char m_residualBlindness;  // +0x4c0
    unsigned char m_residualParalyze;   // +0x4c1
    // Forgetfulness's mastery level, the gate can_shoot pairs with
    // forgetfulnessRounds: `>= 2` (advanced or expert) stops the stack
    // shooting outright. It does NOT land on any of the per-spell rows
    // this class already models - neither the +0x198 round base nor
    // slayerLevel's - so it is sliced on the body alone and the row it
    // belongs to is left open.
    int m_forgetfulnessLevel;       // +0x4c4
    // Slow's speed multiplier, applied by GetSpeed while slowRounds is up.
    float m_slowFactor;             // +0x4c8
    // The tail of the DC amounts run on the +0x20 shift this band
    // carries (DC 1188 forgetfulness_level -> +0x4c4 and DC 1192
    // slowPenalty -> +0x4c8 anchor it): 1196 tailwindBonus, 1200
    // diseaseDefensePenalty, 1204 diseaseAttackPenalty. Byte-proven by
    // CancelIndividualSpell (0x444510): its HASTE arm subtracts +0x4cc
    // from the speed word and its DISEASE arm adds +0x4d0/+0x4d4 back
    // onto defenseSkill/attackSkill.
    int m_tailwindBonus;            // +0x4cc
    int m_diseaseDefensePenalty;    // +0x4d0
    int m_diseaseAttackPenalty;     // +0x4d4
    // +0x4d8. combatManager::InitNonVisualVars (0x463c60) is the one
    // decoded reader: its closing walk scans each side's stacks and
    // raises the per-side latch at combatManager+0x1329c the moment it
    // finds a stack whose byte here is non-zero, so the byte is a
    // per-stack "this side has one of these" marker whose meaning that
    // latch does not name either.
    unsigned char m_onNativeTerrain;      // +0x4d8
    // The DEFEND stance's banked defense bonus: new_turn (0x446e30)
    // subtracts it back out of defenseSkill and clears creatureId bit
    // 27 in the same breath, so the bit is "is defending" and this word
    // is what the stance added. Name stays ordinal - no roster row
    // reaches it and the writer (the defend command) is not decoded.
    int m_defendBonus;                // +0x4dc
    // Read by combatManager::ViewArmy and forwarded as the first
    // argument of the post-dialog command. The DC name for the nearby
    // scalar run does not survive the retail STL-layout shift, so keep
    // this address-ordinal until that callee is identified.
    int m_faerieDragonSpell;                // +0x4e0
    // Magic Mirror redirect percentage; DC/NH3API name backlash_chance.
    unsigned int m_backlashChance;  // +0x4e4

    // Dreamcast LF_FIELDLIST 0x205b, entries 97..114.  This is the exact
    // declaration prefix through do_attack, after the public data run and
    // before every later member.  Keep source facts here even while a retail
    // candidate temporarily scores lower: C1XX assigns member handles from
    // this stream before C2 optimizes any individual function.
    army();
    void init(int armyId, int newNumTroops, const hero* owner, int side,
              int inIndex, int gridIndex, int origPos);
    void initialize(TCreatureType type, long number, const hero* owner,
                    long newGroup, long newIndex, long newGridIndex);
    void initClean();
    void loadResources();
    void resetRound();
    void endWalk();
    void walk(int direction, unsigned char endWalk,
              unsigned char initialWalk);
    unsigned char walkTo(int destIndex, unsigned char restoreFacing);
    int fly(int destIndex);
    int flyTo(int destIndex, unsigned char restoreFacing);
    int teleport(int destIndex);
    int teleportTo(int destIndex, unsigned char restoreFacing);
    long adjustDamage(army* enemy, long baseDamage, unsigned char isShot,
                       unsigned char simulated, long distance,
                       long* fireDamage) const;
    void adjustHitpoints();
    unsigned char attackHex(int hex, unsigned char restoreFacing);
    unsigned char doAttack(army* armyToAttack, int direction);
    void doAttack(int direction);

    // LF_FIELDLIST 0x205b entries 115..227. Overloads share one roster
    // entry; Complete-only additions are kept adjacent to the closest shared
    // family without changing the attested relative order below.
    void doMultiHeadAttack(unsigned attackMask, int* damage, int* killed,
                              long* fireDamage);
    void rangeAttack(army* armyToAttack);
    void rangeAttack();
    void attackWall(int targetGridIndex);
    void turn(unsigned char animateTurn);
    bool needToTurn(int direction) const;
    bool canCastResurrect(long hex) const;
    bool canCastResurrect() const;
    unsigned char canCastSpell(long hex) const;
    bool canRetaliate(const army& attacker) const;
    unsigned char canShoot(const army* excluded) const;
    void castCaliphSpell(long hex);
    void castResurrect(long hex);
    void castDemonicResurrect(long hex);
    unsigned char checkSpecialAttack(army* target);
    void castSpell(long hex);
    // Complete retains this ordinary destructor in Army code at 0x43d400,
    // immediately after army(). Its body owns the member cleanup; callers
    // such as getCureValue keep the call. DC attributes it to an ai.cpp use site.
    ~army();
    void faerieDragonSpell();
    unsigned char unnamed447fe0();
    unsigned char checkObstacleAttacks(unsigned char isWalking);
    void clearAIValues();
    void considerAttack(const army* enemy, long value,
                         long attackDistance);
    unsigned char enemyIsAdjacent(const army* excluded) const;
    unsigned getAttackMask(int currIndex, int criteria,
                           int literalTargetIndex) const;
    long getAdjustedAttack(const army* enemy,
                             unsigned char rangedAttack) const;
    long getAdjustedDefense(const army* enemy,
                              unsigned char frenzyIncluded) const;
    long getAIExpectedDamage() const;
    const army* getAITarget() const;
    long getAITargetValue() const;
    long getAITargetTime(long speed) const;
    long getAITargetTime() const;
    long getAIPossibleTargets() const;
    long getAttackModifier(const army* enemy,
                             unsigned char rangedAttack) const;
    long getAverageDamage(const army* enemy, unsigned char rangedAttack,
                            long amount, unsigned char limitDamage,
                            long distance) const;
    double getAverageDamage() const;
    long getEstimatedDamage(const army* target, long amount,
                              unsigned char ranged, long distance) const;
    void getBerserkTargets(std::vector<army*>& armies) const;
    int getOwningSide() const;
    int getControllingSide() const;
    hero* getOwner() const;
    hero* getController() const;
    inline double getDefenseDamageModifier(
        unsigned char rangedAttack) const;
    long getDefenseModifier() const;
    long getClockwise(long direction) const;
    long getCounterClockwise(long direction) const;
    float getFireShieldStrength() const;
    long getLossCombatValue(long lowestAttack, long lowestDefense,
                               unsigned char ranged, long damage,
                               unsigned char killsOnly) const;
    long getResurrectionSize(const army* target) const;
    int getSecondGridIndex() const;
    long getTotalCombatValue(long lowestAttack,
                                long lowestDefense) const;
    long getTotalHitPoints(unsigned char simulated) const;
    double getUnitCombatValue(long lowestAttack, long lowestDefense,
                                 unsigned char ranged,
                                 const army* excluded) const;
    long getValidCaliphSpells(const army* target) const;
    int getBestDirection(int start, int target, int direction);
    unsigned char isAdjacent(const army& otherArmy) const;
    unsigned char isAdjacent(int hex) const;
    unsigned char isEnemy(const army* arg) const;
    bool isInAura() const;
    unsigned char moveTo(int hex, unsigned char restoreFacing);
    void newTurn();
    void setAIExpectedDamage(long arg);
    int findPath(int fpTargetCellIndex, int maxMoves,
                 unsigned char moveUnlimited,
                 unsigned char literalTarget);
    void setRetaliationCount();
    int validAttack(int currIndex, int direction, int criteria,
                    int literalIndex, int* testCellIndex) const;
    void resetPath();
    unsigned char validPath(int destIndex, unsigned char literalTest);
    unsigned char validFlight(int destIndex,
                              unsigned char literalTest) const;
    int validRange(int destIndex);
    inline long damageEnemy(army* enemy, int* damageOut, int* killed,
                            unsigned char isShot);
    int damage(int damage);
    int computeBaseDamage(unsigned char simulateOnly) const;
    int computeAttackerDamageBonuses(int baseDamage,
                                     unsigned char isShooting,
                                     army* defender,
                                     unsigned char simulateOnly,
                                     long distance) const;
    inline int computeDefenderDamageBonuses(int baseDamage) const;
    double computeAttackerDamageReduction(const army* defender,
                                          unsigned char isShooting) const;
    double computeDefenderDamageReduction(
        unsigned char isShooting) const;
    int computeAttackerBonus(int baseDamage, unsigned char isShooting,
                               army* defender, unsigned char simulateOnly,
                               long distance) const;
    void cancelSpellType(int spellType);
    void decrementSpellRounds();
    void goBerserk();
    void cure(int level, int spellPower, const hero* castingHero);
    int canFit(int destIndex, int allowShifting,
               int* newDestIndex) const;
    void drawToBuffer(int x, int y, int numBoxOnly);
    void playAnimation(int sequence, int nframes, int startFrame);
    void setupAnimation();
    unsigned long strength();
    bool isActive() const;
    inline void checkLuck();
    void setSpellInfluence(int spell, int power, int mastery,
                           const hero* castingHero);
    void cancelIndividualSpell(int spell);
    void cancelAllSpells();
    int bottomY() const;
    int midY() const;
    int topY() const;
    int leftX() const;
    int rightX() const;
    int frontX() const;
    int midX() const;
    bool is(unsigned attribute) const;
    bool isInAreaHighlight() const;
    int offsetToFront(int direction) const;
    void processDeath(int fadeElementals);
    int otherArmyAdjacent(int group, int index);
    int getAdjacentCellIndex(int currIndex, int direction) const;
    long getAdjacentHex(long hex, long direction) const;
    long getAdjacentHex(long direction) const;
    long getAttackDirection(long ourHex, const army* enemy,
                              long enemyHex) const;
    inline long getAttackDirection(long ourHex, const army* enemy) const;
    long getAttackDirection(const army* enemy) const;
    long getMultiHeadDirections(long ourHex, const army* enemy,
                                   long enemyHex) const;
    long getSpellTime(int spell) const;
    TSkillMastery getSpellLevel(int spell) const;
    unsigned char setInsideAreaEffect(unsigned char arg);
    void playSample(TSampleID id);
    void stopSample(TSampleID id);
    void waitSample(TSampleID which);
    void addAura();
    void removeAura();
    void removeBinding();
    bool cannotAttack() const;
    inline const char* getName() const;
    inline const char* getName(int count) const;
    bool isIncapacitated() const;
    void setLuck(const hero* ownerHero, const armyGroup* ownerGroup,
                 const town* ownerTown, const hero* otherHero,
                 const armyGroup* otherGroup, int magicTerrain);
    int getLuck(unsigned char applyLimits) const;
    void setMorale(const hero* ownerHero, const armyGroup* ownerGroup,
                   const town* ownerTown, const hero* otherHero,
                   const armyGroup* otherGroup, int magicTerrain,
                   unsigned char groupAlignments);
    int getMorale(unsigned char applyLimits) const;
    int getSpeed() const;

    int getMirrorEffect() const;

    // Complete's NextArmy directly combines the private reset latch with the
    // shared IsIncapacitated helper. The exact retail lowering proves that
    // combatManager can read this tail without making the Dreamcast-private
    // data public; friendship is the source-level access that preserves both.
    friend class combatManager;

private:
    void attackWall(TWallTargetId wall, long levelsDestroyed);
    void attackWall(TWallTargetId wall,
                     const type_ballistics_traits& ballistics);

    void animateMissile(army* armyToAttack);
    void doFireShield(long damage);
    void doPostAttack(army* target, int attackDamage, int killedCount,
                        int totalLife);
    void doHydraAttack(int direction);
    // DC find_flyer_attack_cell parameters are start/target and target.
    bool findFlyerAttackCell(int start, int target) const;
    bool findFlyerAttackCell(int target) const;
    bool leavesNoBody() const;
    unsigned char simpleMove(int hex, unsigned char restoreFacing);
    double computeKarma() const;

    // LF_FIELDLIST entries 237..249. The established source aliases preserve
    // retail layout while the gate records the Dreamcast names.
    int m_morale;                           // +0x4e8, DC iMorale
    int m_luck;                             // +0x4ec, DC iLuck
    unsigned char m_resetThisRound;              // +0x4f0, DC reset_this_round
    unsigned char m_isAreaEffectTarget;  // +0x4f1
    std::vector<army*> m_boundArmies;      // +0x4f4
    std::vector<army*> m_binders;           // +0x504
    std::vector<army*> m_auraClients;      // +0x514
    std::vector<army*> m_auraSources;      // +0x524
    int m_aiExpectedDamage;               // +0x534
    army* m_aiTarget;                      // +0x538
    long m_aiTargetValue;                 // +0x53c
    long m_aiTargetTime;                  // +0x540, DC AI_target_distance
    unsigned m_aiPossibleTargets;         // +0x544

#if 0  // superseded unordered/view-fragmented declaration reconstruction

    int findPath(int fpTargetCellIndex, int maxMoves,
                 unsigned char bMoveUnlimited, unsigned char bLiteralTarget);
    unsigned char validPath(int destIndex, unsigned char bLiteralTest);
    // Both const (?GetAttackMask@army@@QBAIHHH@Z,
    // ?ValidAttack@army@@QBAHHHHHPAH@Z); neither body writes through
    // `this` and both drive GetAdjacentCellIndex, already const.
    unsigned getAttackMask(int currIndex, int criteria,
                           int iLiteralTargetIndex) const;
    int validAttack(int currIndex, int direction, int criteria,
                    int iLiteralIndex, int* testCellIndex) const;
    // Both const: ai_tactical's get_breath_bonus (0x436760) drives the
    // pair off the `const army*` it takes as its second parameter.
    int getAdjacentCellIndex(int currIndex, int direction) const;
    long getAdjacentHex(long hex, long direction) const;
    // 0x445840, claimed in army.cpp. Const because ai_tactical's
    // get_breath_bonus (0x436760) calls it on the `const army*` it
    // takes as its second parameter.
    long getAttackDirection(long our_hex, const army* enemy,
                              long enemy_hex) const;
    inline long getAttackDirection(long our_hex, const army* enemy) const;
    // 0x448ab0 (claimed in army.cpp): the bitmask of the directions a
    // wide/multi-headed stack would also strike. Const for the same
    // reason - get_multi_head_bonus (0x436620) drives it off a
    // `const army*`.
    long getMultiHeadDirections(long our_hex, const army* enemy,
                                   long enemy_hex) const;
    // The two neighbours of a combat direction. DC rows 0x45fc0 /
    // 0x46008, and NEITHER has a retail out-of-line slot - the whole
    // bracket 0x440160..0x440306 is accounted for and the next carve
    // row is 0x440310 - so retail inlines both away. They are defined
    // `inline` in army.cpp for exactly that reason.
    // BEHIND A VIEW, and the reason is MEASURED: declaring this pair
    // unconditionally in the class costs command.obj's
    // combatManager::GetCommand 92.5714 -> 92.5357, with no semantic
    // change anywhere and nothing else in the tree moving. That is the
    // include-set-sensitivity class - two more member declarations
    // shift VC6's optimizer state in every TU that sees this header -
    // and army.cpp is the only consumer, so the surface is scoped to
    // it the way advmgr / events / hero already scope theirs. The
    // measurement is the whole justification: remove the guard and
    // GetCommand drops again.
    long getClockwise(long direction) const;
    long getCounterClockwise(long direction) const;
    // DC public ?CanFit@army@@QBAHHHPAH@Z; mark_teleport's retail call
    // passes (hex, 0, 0) through a const army pointer.
    int canFit(int destIndex, int bAllowShifting,
               int* iNewDestIndex) const;
    int getSpeed() const;                    // 0x448cd0, claimed in army.cpp
    // Combat movement surface. The retail call graph from simple_move and
    // the five-function block at 0x4b46c0..0x4b5011 locate fly.obj; these
    // declarations are consumed by the admitted FlyTo/TeleportTo wrappers.
    void addAura();                         // 0x43ea70
    void removeAura();                      // 0x43ec50
    void removeBinding();                   // 0x43ee10
    // Raise or lower the "this stack is standing in an area effect"
    // latch and re-pose it: the retail body (0x43efe0) returns 0 when
    // the latch is already the requested value, so callers use the
    // answer as "did anything change". `_N_N` on the DC public
    // (?set_inside_area_effect@army@@QAA_N_N@Z) is both the byte
    // argument and the byte return.
    unsigned char setInsideAreaEffect(unsigned char arg);  // 0x43efe0
    void playSample(TSampleID id);          // 0x43d540
    void stopSample(TSampleID id);          // 0x43d580
    void waitSample(TSampleID which);
    // simple_move is PRIVATE on its own public
    // (?simple_move@army@@AAA_NH_N@Z) and every member of this movement
    // family returns `_N` - bool - and takes `restore_facing` as one:
    // WalkTo, attack_hex, move_to and ValidFlight all mangle _NH_N.
    // RECORDED, NOT ACTED ON: the access change and the bool retype are
    // one measured pass over the whole family (bool is not free in VC6
    // - it normalizes), and this lane only needed the declarations.
    unsigned char simpleMove(int hex, unsigned char restore_facing);
    unsigned char moveTo(int hex, unsigned char restore_facing);
    // ProcessNextAction's two dispatch-only army calls.
    void attackWall(int iTargetGridIndex);
    void castSpell(long hex);
    // The shooting pair. 0x440160 is the public no-argument entry - it
    // resolves groupToAttack/indexToAttack into the target stack, turns
    // to face it, and fires between one and three volleys - and it
    // hands each volley to the private one-argument overload at
    // 0x43f900 (still a carcass), which is the animation-and-damage
    // worker.
    void rangeAttack();
    void rangeAttack(army* armyToAttack);
    // 0x43f2c0, EH-bearing carcass in army.cpp; declared because the
    // volley worker above calls it once per shot.
    void animateMissile(army* armyToAttack);
    unsigned char checkObstacleAttacks(unsigned char is_walking);
    // 0x440500, reconstructed in army.cpp: the attacker's on-attack
    // debuff roll (bind/blind/disease/curse/age/stone/poison/acid/
    // paralyze); returns 1 for the three incapacitators.
    unsigned char checkSpecialAttack(army* target);
    // 0x440bc0, EH-bearing carcass in army.cpp; declared for
    // do_attack's kill-accounting tail.
    void doPostAttack(army* target, int iDamage, int iKilled,
                        int total_life);
    void turn(unsigned char play_animation); // 0x446720
    void setupAnimation();                   // 0x446830
    void playAnimation(int sequence, int nframes, int start_frame);
    // 0x43e140, carcass in army.cpp; declared here because army::Fly
    // (fly.obj) calls it once per animation frame.
    void drawToBuffer(int x, int y, int bNumBoxOnly);
    void cancelSpellType(int iSpellType);    // 0x4444d0
    void cancelIndividualSpell(int spell);   // 0x444510
    // The Cure spell's whole effect: restore hit points, clamp the top
    // creature's damage to what the stack can still survive, cancel the
    // fourteen negative influences one by one and heal the remainder.
    // The DC prototype (army.cpp:4739) names the three parameters and
    // retail's `ret 0xc` agrees.
    void cure(int level, int iSpellPower, const hero* casting_hero);  // 0x446500
    // 0x4448f0, claimed and reconstructed in army.cpp (an earlier
    // revision of this note said 0x4443f0 - a typo, that address is
    // inside ProcessDeath's span; the carve row 0x4448f0/0xB99 with
    // dc 0x499e8 = army.cpp:3816 is the body). spells.obj's
    // SetMassSpellInfluence (0x5a66d0) and src/spells.cpp remain
    // callers, and army.cpp now consumes the declaration too.
    // `mastery` is DC's TSkillMastery, spelled int: herospec.h's enum
    // is not in this header's closure, spells.cpp's
    // SetMassSpellInfluence forwards a plain long into the slot, and
    // the two spellings are byte-identical everywhere the body uses it
    // (an index, two stores, one signed compare). Only the mangled
    // name differs, and the VA claim owns the pairing.
    void setSpellInfluence(int spell, int power, int mastery,
                           const hero* casting_hero);
    // Const (?ValidFlight@army@@QBA_NH_N@Z): the fly.obj body only
    // reads, and both callees it drives on `this` are already const.
    unsigned char validFlight(int destIndex,
                              unsigned char bLiteralTest) const;
    void setLuck(const hero* ownerHero, const armyGroup* ownerGroup,
                 const town* ownerTown, const hero* otherHero,
                 const armyGroup* otherGroup, int magicTerrain);
    void setMorale(const hero* ownerHero, const armyGroup* ownerGroup,
                   const town* ownerTown, const hero* otherHero,
                   const armyGroup* otherGroup, int magicTerrain,
                   unsigned char m_field54b2);
    // THREE CREATURE IDS THAT BELONG IN armygrp.h's TCreatureType and
    // are parked here instead. Byte-proven 2026-08-14 by two army.obj
    // bodies (get_adjusted_defense 0x442590 multiplies the defender's
    // rating by 0.4f / 0.8f against exactly 0x60 / 0x61 - the Behemoth
    // and Ancient Behemoth defense-ignore rule and nothing else in the
    // roster; get_resurrection_size 0x447330 prices every non-Archangel
    // resurrect in akCreatureTypeTraits[0x30].hitPoints, the Pit Lord's
    // raise-Demons rule). Both id runs are bracketed by ids TCreatureType
    // already proves - Efreet Sultan 0x35 / Devil 0x36 / Arch Devil 0x37
    // put Demon on 0x30, and the Stronghold block's top tier is
    // 0x60/0x61. NH3API spellings.
    // They are NOT in armygrp.h because ANY enumerator added to
    // TCreatureType costs initialize.obj's initialize_game_data
    // 100.0000 -> 96.0880 - the include-set-sensitivity class, and the
    // same defect armygrp.h's SSpellTraits note already records for
    // three ESpellId enumerators. MEASURED 2026-08-14 over enumerator
    // counts 0..6: the canary sits at 100.0 for zero added enumerators
    // and at 96.0880 for every count from one upward, so the wall fires
    // on the FIRST one and no count restores it. army.h is outside
    // initialize.cpp's include closure (terrain.h + town.h -> armygrp.h),
    // which is the whole reason this enum can exist at all. Fold it back
    // into TCreatureType when an owner moves initialize.cpp's includes.
#endif  // superseded unordered/view-fragmented declarations
public:
    enum EArmyCreatureId {
        // The three-headed attacker. get_multi_head_directions
        // (0x448ab0) hands every OTHER creature the full adjacency
        // mask (0xff wide, 0x3f narrow) and only this id gets the
        // three-direction fan, which is exactly the Cerberus rule.
        // The id is fixed by arithmetic, not by a roster: Demon is
        // 0x30 two lines below on independent evidence, and 0x2f is
        // the slot immediately before it - the Inferno tier that ends
        // Imp / Familiar / Gog / Magog / Hell Hound / CERBERUS /
        // Demon. NH3API spelling.
        ARMY_CREATURE_CERBERUS = 0x2f,
        ARMY_CREATURE_DEMON = 0x30,
        // The top tier of the Dungeon block and the two Conflux
        // elementals whose attacks ComputeAttackerDamageReduction
        // (0x443b90) halves. All three ids are fixed by ids
        // armygrp.h's TCreatureType already proves and by the rule the
        // body implements, not by a roster:
        //   - Troglodyte 0x46 / Infernal Troglodyte 0x47 open the
        //     fourteen-id Dungeon block and Minotaur 0x4e / Minotaur
        //     King 0x4f sit in it, which puts its top tier - the black
        //     dragon - on 0x53;
        //   - Air / Earth / Fire / Water Elemental 0x70..0x73 and Gold
        //     / Diamond Golem 0x74/0x75 leave 0x76/0x77 for the pixie
        //     pair and 0x78/0x79 for the psychic and magic elementals,
        //     which is also exactly what puts Ice 0x7b, Magma 0x7d,
        //     Storm 0x7f and Energy 0x81 where TCreatureType already
        //     has them - the four unused slots between them.
        // The body corroborates both readings on the retail rules:
        // 0x78 halves against a defender carrying the mind-immunity
        // bit (the psychic elemental's rule) and 0x79 halves against
        // 0x79 or 0x53 (the magic elemental against black dragons and
        // its own kind). NH3API spellings.

        // These domain names are ordinary source facts. Their declaration
        // changes VC6's class-source state, which is why hiding them once
        // produced a higher local score; that measurement is not permission
        // to replace the declarations with literals in selected TUs.
        // The Pit Lord id is fixed by
        // arithmetic against ids the block above already proves - Demon
        // 0x30 opens the Inferno upgrade run and Efreet Sultan 0x35 /
        // Devil 0x36 / Arch Devil 0x37 close it, so 0x33 is the Pit
        // Lord - and the body corroborates it: this is exactly the id
        // that takes the DEMONIC resurrection arm, priced (in
        // get_resurrection_size 0x447330, already exact) in
        // akCreatureTypeTraits[0x30].hitPoints. NH3API spelling.
        ARMY_CREATURE_ARCHANGEL = 0x0d,
        ARMY_CREATURE_BLACK_DRAGON = 0x53,
        ARMY_CREATURE_PSYCHIC_ELEMENTAL = 0x78,
        ARMY_CREATURE_MAGIC_ELEMENTAL = 0x79,
        ARMY_CREATURE_PIT_LORD = 0x33,
        // Complete's cannot_attack adds the two non-attacking war machines.
        // Keep these aliases in army's local creature-id view: army.h is
        // parsed before armygrp.h's complete TCreatureType declaration in
        // several retail TUs.
        ARMY_CREATURE_FIRST_AID_TENT = 0x93,
        ARMY_CREATURE_AMMO_CART = 0x94,
        // The two creatures with a retaliation rule of their own, and
        // ResetRound (0x447120) is what proves both: id 4 gets a
        // retaliation allowance of 2 and id 5 gets 5000, which is
        // "unlimited" in a word that is only ever counted down. Two
        // retaliations is the Griffin's rule and unlimited retaliation
        // is the Royal Griffin's, and nothing else in the roster has
        // either. Their ids follow from the Castle block that opens the
        // table - Pikeman / Halberdier / Archer / Marksman fill 0..3,
        // which puts the tier-three pair on 4 and 5, exactly as
        // CREATURE_ANGEL 0xc / CREATURE_ARCHANGEL 0xd close the same
        // town's run eight slots later. NH3API spellings.
        ARMY_CREATURE_GRIFFIN = 0x4,
        ARMY_CREATURE_ROYAL_GRIFFIN = 0x5,
        // The two Dungeon flyers that strike and return to their starting
        // hex. process_move_then_attack compares exactly 0x48/0x49 before
        // applying the Blind/Stone/Paralyze return guards. NH3API spellings;
        // the ids also follow from Dungeon's 0x46..0x53 fourteen-slot run.
        ARMY_CREATURE_HARPY = 0x48,
        ARMY_CREATURE_HARPY_HAG = 0x49,
        // The two creatures that bring down a wall segment without a
        // catapult, and AttackWall (0x445d30) is what proves both: its
        // switch answers 0x5e with ballistics row 1 and 0x5f with row
        // 2, and the Dreamcast build's own AttackWall (dc 0x4a97c)
        // compares the SAME two ids - 94 and 95 - which is what says
        // they are below the Complete renumbering's break (the DC row
        // compares 118 for the catapult where retail compares 145).
        // The ids follow from the pair already byte-proven directly
        // below: Stronghold's fourteen slots end Behemoth 0x60 /
        // Ancient Behemoth 0x61, so the tier-five pair is 0x5e/0x5f,
        // and Cyclopes are HoMM3's one wall-breaking creature. Dreamcast
        // AttackWall compares the same 94/95 pair, and retail 0x445d30
        // corroborates both ids.
        ARMY_CREATURE_CYCLOPS = 0x5e,
        ARMY_CREATURE_CYCLOPS_KING = 0x5f,
        // The aura pair. add_aura (0x43ea70) is the only reader and it
        // tests BOTH stacks against the same two ids, once in each
        // direction, which is what says these are the creature that
        // GIVES the aura rather than one that receives it. 0x18/0x19
        // land on Unicorn / War Unicorn in Rampart's fourteen-slot run,
        // and the Dendroid pair two slots earlier (0x16/0x17) is
        // corroborated from the other side by remove_binding's own
        // subject - Rampart owns both of HoMM3's stack-to-stack
        // relationships, the unicorn's resistance aura and the
        // dendroid's bind. Dreamcast add_aura compares the same 24/25
        // pair, and retail 0x43ea70 corroborates both values.
        ARMY_CREATURE_UNICORN = 0x18,
        ARMY_CREATURE_WAR_UNICORN = 0x19,
        ARMY_CREATURE_BEHEMOTH = 0x60,
        ARMY_CREATURE_ANCIENT_BEHEMOTH = 0x61,
        // The three creatures with a START-OF-TURN ability, and
        // combatManager::SetNextArmy (0x465330) is the one body that
        // proves all three: its compare chain answers 0x3d, 0x86 and
        // 0x88 and nothing else. Each id follows from the arithmetic the
        // elemental block above already fixes - Psychic / Magic
        // Elemental on 0x78/0x79 and Ice 0x7b / Magma 0x7d / Storm 0x7f
        // / Energy 0x81 with the four unused slots between them, which
        // continues Firebird 0x82 / Phoenix 0x83, Azure 0x84 / Crystal
        // 0x85 / FAERIE 0x86 / Rust 0x87, then ENCHANTER 0x88 - and
        // 0x3d closes the Necropolis run that opens at Skeleton 0x38.
        // The bodies corroborate each id independently:
        //   - 0x3d drains two mana off the OTHER side's hero and plays
        //     ManaDrai.wav, which is the wraith's rule and no other
        //     creature's;
        //   - 0x86 calls 0x447510, whose body picks a spell at random
        //     out of a weighted table - the faerie dragon's rule - and
        //     which the HD crossbuild map independently names
        //     FaerieDragonSpell;
        //   - 0x88 is gated on a per-side counter that must exceed 2 and
        //     is reset to 0 on success, i.e. an ability with a cooldown
        //     of three rounds, which is the enchanter's mass cast.
        ARMY_CREATURE_WRAITH = 0x3d,
        ARMY_CREATURE_FAERIE_DRAGON = 0x86,
        ARMY_CREATURE_ENCHANTER = 0x88,
        // The two combat participants can_shoot (0x4428f0) admits as
        // shooters unconditionally, ahead of every other test - the
        // ballista and the arrow tower, which are the only war machines
        // that shoot. NH3API spellings; ai_tactical's own 0x93/0x94
        // comparisons put the first-aid tent and ammo cart between them.
        ARMY_CREATURE_BALLISTA = 0x92,
        // The one creature obstacles never fire at:
        // check_obstacle_attacks (0x441f70) compares creatureType
        // against 0x95 and returns 0 before it reaches the combat
        // manager's worker. NH3API spells the same guard
        // `armyType != CREATURE_ARROW_TOWER` with the same 149, and a
        // siege tower is the one combat participant that never moves.
        ARMY_CREATURE_ARROW_TOWER = 0x95,
        // The highest id GetName (0x440100) accepts: its range guard is
        // `type < 0 || type > 0x96`, so the name rows it indexes run
        // 0..150 inclusive - one past the 150-entry bound armygrp.h
        // currently declares for akCreatureTypeTraits.
        ARMY_CREATURE_LAST = 0x96
    };

#if 0  // superseded unordered/view-fragmented declaration reconstruction

    // The two start-of-turn ability bodies combatManager::SetNextArmy
    // (0x465330) dispatches to off creatureType. Both take `this` only
    // and both sit in army.obj; neither is claimed here, and neither is
    // named by the DC roster, so 0x447fe0 keeps the address-ordinal
    // shape this tree uses for exactly that case.
    //   FaerieDragonSpell (0x447510, 424 B) spends one of the +0xdc
    //     counters and picks a spell out of the weighted table at
    //     .rdata 0x63b850 with a single rand()%total draw. The NAME is
    //     the HD crossbuild map's, adopted because the creature id that
    //     reaches it - 0x86 - independently lands on the faerie dragon
    //     by the elemental-block arithmetic recorded on the enumerator.
    //   Unnamed447fe0 (638 B) answers a byte; SetNextArmy calls it only
    //     for the enchanter and only while the per-side counter at
    //     combatManager+0x132a0 exceeds 2, clearing that counter when
    //     the answer is non-zero.
    void faerieDragonSpell();                // 0x447510
    unsigned char unnamed447fe0();           // 0x447fe0
    // 0x448260, reconstructed in army.cpp: the animated creature-cast
    // dispatcher. combatManager::SetNextArmy-family code is the retail
    // caller; declared with its family here.
    void castSpell(long hex);
    // Const on the DC roster's own mangling
    // (?is_enemy@army@@QBA_NPBV1@@Z), which is what lets
    // combatManager::enemy_is_adjacent take a const army* as the
    // roster spells it, and can_shoot pass its own const this.
    // 0x446660 / 0x446630, the two screen-midpoint accessors, both const
    // on the DC roster (army.cpp:4790 / 4773). combatManager::KeepAttack
    // (0x465ad0) is the decoded consumer and it proves both roles: the
    // MidX result is compared against the firing archer's own x to form
    // the horizontal flip, and both are handed to
    // GetMissileStartingPosition as the destination. Retail's bodies
    // read gpCombatManager->cells[gridIndex] at +0x1c4 and +0x1c6 with
    // the 112-byte hexcell stride.
    int midX() const;                        // 0x446660
    int midY() const;                        // 0x446630
    unsigned char isEnemy(const army* arg) const; // 0x442880
    // 0x4429f0: asks the combat manager whether any enemy stack (other
    // than `excluded`) neighbours this stack's own hex, and for a
    // two-hex creature its second hex as well. Const
    // (?enemy_is_adjacent@army@@QBA_NPBV1@@Z) - the last of the chain
    // combatManager::enemy_is_adjacent's own const `this` needs.
    unsigned char enemyIsAdjacent(const army* excluded) const;
    // 0x4430d0: clamps the AI's committed damage to what the stack can
    // actually absorb - `_cpp_min(get_total_hit_points(), arg)`.
    void setAIExpectedDamage(long arg);
    long getAdjustedAttack(const army* enemy,
                             unsigned char ranged_attack) const;
    long getAttackModifier(const army* enemy,
                             unsigned char ranged_attack) const;
    // 0x442660 (41 B). The 2026-08-08 note on ai_tactical's
    // type_AI_combat_parameters ctor called this leaf "unidentified";
    // it is get_defense_modifier, and three things say so together: the
    // DC roster order army.cpp:2623 get_attack_modifier (dc 0x477b8) <
    // army.cpp:2670 get_defense_modifier (dc 0x478c8) maps onto retail
    // 0x442550 < 0x442660 with nothing between; the ctor's call site
    // passes NO arguments, which only the defense signature fits; and
    // its result is stored as the defense half of the same
    // lowest-attack / lowest-defense pair get_attack_modifier feeds.
    // Claimed nowhere yet - declared here so ai_tactical can call it,
    // exactly as can_shoot above.
    long getDefenseModifier() const;                          // 0x442660
    // Combat-AI leaves, all claimed in army.cpp; declared here so
    // ai_tactical can call them (the retail callsites are the
    // location evidence for get_average_damage's own claim).
    unsigned char canShoot(const army* excluded) const;        // 0x4428f0
    // 0x4473d0 / 0x4476c0, carcasses in army.cpp; declared here because
    // combatManager::GetCommand (command.obj) is a caller of both.
    // Both const (?can_cast_resurrect@army@@QBA_NJ@Z,
    // ?can_cast_spell@army@@QBA_NJ@Z).
    unsigned char canCastResurrect(long hex) const;
    unsigned char canCastSpell(long hex) const;
    long getLossCombatValue(long lowest_attack, long lowest_defense,
                               unsigned char ranged, long damage,
                               unsigned char kills_only) const; // 0x442fd0
    long getTotalHitPoints(unsigned char simulated) const;   // 0x443080
    inline void checkLuck();
    inline long damageEnemy(army* enemy, int* iDamage, int* iKilled,
                            unsigned char bIsShot);
    // 0x442590: the stack's defense as the ATTACKER sees it - zero
    // under Frenzy, reduced 40%/80% against a Behemoth / Ancient
    // Behemoth, and 3 lower while either of the stack's hexes stands
    // in a moat.
    // Const because get_adjusted_attack (0x442410), which army.h
    // already has to declare const for get_attack_modifier's sake,
    // calls it on `this` in the Frenzy tail. Nothing in the body
    // writes.
    long getAdjustedDefense(const army* enemy,
                              unsigned char frenzy_included) const;
    // 0x443840 / 0x443b90, carcasses in army.cpp; declared because
    // adjust_damage (0x443f40) calls both and retail does NOT inline
    // either. Both const
    // (?ComputeAttackerDamageBonuses@army@@QBAHH_NPAV1@0J@Z,
    // ?ComputeAttackerDamageReduction@army@@QBANPBV1@_N@Z); the first
    // takes its defender NON-const, the second const, which is the
    // roster's own split and not a transcription slip.
    int computeAttackerDamageBonuses(int base_damage,
                                     unsigned char is_shooting,
                                     army* defender,
                                     unsigned char simulate_only,
                                     long distance) const;
    // E:\gamedcs\army.cpp:3230
    // The Dreamcast body is the single source statement `return 0;`.
    // Keep the named boundary source-visible even though retail VC6 can
    // erase both this body and its call from adjust_damage.
    inline int computeDefenderDamageBonuses(int base_damage) const;
    double computeAttackerDamageReduction(const army* defender,
                                          unsigned char is_shooting) const;
    // 0x443320, the retail-only numeric half of the row above: the
    // offense / archery / spell-bonus arithmetic with no combat
    // message and no sound, so that get_estimated_damage (0x443e30)
    // can price a blow without playing one. Same five parameters and
    // the same `ret 0x14`, and its `defender` stays NON-const for the
    // same reason adjust_damage's does - the const caller casts, the
    // declaration does not drop it. Declared, not claimed here;
    // army.cpp owns the body.
    int computeAttackerBonus(int base_damage, unsigned char is_shooting,
                               army* defender, unsigned char simulate_only,
                               long distance) const;
    // 0x443160: one swing's RAW damage - the effective creature count,
    // the damage range (hero-attack-scaled for a ballista), then the
    // Bless / Curse / simulation / dice arms. Const
    // (?ComputeBaseDamage@army@@QBAH_N@Z).
    int computeBaseDamage(unsigned char simulate_only) const;
    // 0x443d90: the multiplier a defender's own Shield / Air Shield,
    // petrification and hero defense skill put on incoming damage.
    // Const (?ComputeDefenderDamageReduction@army@@QBAN_N@Z), and so is
    // the whole ComputeXxxDamage family beside it in army.cpp.
    double computeDefenderDamageReduction(unsigned char is_shooting) const;
    // 0x447330: how many creatures a resurrect from THIS stack would
    // restore to `target` - the Archangel rule, or the Pit Lord's
    // raise-Demons rule for every other caster. Const
    // (?get_resurrection_size@army@@QBAJPBV1@@Z).
    long getResurrectionSize(const army* target) const;
    // 0x444090: applies one blow's damage to the stack and answers how
    // many creatures it killed.
    int damage(int damage);
    // 0x440310, the hydra's eight-way sweep, and 0x4409c0, the Fire
    // Shield retaliation. Behind a view because a bare member
    // declaration is this header's own measured include-set trigger;
    // army.cpp is the only consumer of either.
    void doMultiHeadAttack(unsigned attackMask, int* damage, int* killed,
                              long* fire_damage);
    void doFireShield(long damage);
    long getAverageDamage(const army* enemy, unsigned char ranged_attack,
                            long amount, unsigned char limit_damage,
                            long distance) const;               // 0x442780
    // The no-argument overload (0x4426f0, claimed in army.cpp): what
    // this stack averages per swing, as a double in st(0).
    // ai_tactical's get_curse_value (0x43b370) is the located caller -
    // `mov ecx, enemy / call` with no stack arguments, and it divides
    // the cursed damage by the result.
    double getAverageDamage() const;                          // 0x4426f0
    // 0x443e30 (257 B, ret 0x10 - four stack args, `this` the
    // attacker): null target answers 0, then it runs the same
    // base-damage / attacker-reduction / hero-defense-factor chain
    // get_average_damage does. UNNAMED in every roster (the DC dump,
    // the HD name map and IDA all leave it blank); ai_tactical's
    // get_attack_skill_value and get_defense_skill_value are its only
    // callers, both asking "what would a 100-creature stack of mine do
    // to this target?" - so the name below is a bootstrap invention.
    long getEstimatedDamage(const army* target, long amount,
                              unsigned char ranged, long distance) const;
    // 0x445490, claimed in army.cpp. Fills the caller's vector with the
    // stacks a berserked `this` would be allowed to strike; ai_tactical's
    // get_berserk_value (0x43a400) is the located caller and passes a
    // freshly default-constructed local, so the callee is the only
    // writer. Const because that caller holds the stack as `const army*`.
    void getBerserkTargets(std::vector<army*>& armies) const;
    // 0x4456d0, claimed in army.cpp: the consumer of the vector above.
    // NON-const, and the body is what says so - it hands `this` to
    // combatManager::berserk_attack, whose first parameter is a plain
    // army*.

    // 0x444120, LOCATED 2026-08-14 from ResetRound's own tail
    // (`push 1 / mov ecx,esi / call`) - a thiscall with ONE stack
    // argument, which is what the DC roster's two-parameter row and the
    // body's `ret 4` both say. Declared here so ResetRound can spell the
    // call; the BODY is still a carcass. Split out of the round view
    // 2026-08-20 so combatManager::PowEffect, whose death sweep is its
    // second decoded caller, can reach it without also taking
    // ResetRound and the round view's other twenty-six declarators.
    void processDeath(int bFadeElementals);
    int getSecondGridIndex() const;                          // 0x4466a0
    int getMirrorEffect() const;                              // 0x4487f0
    void considerAttack(const army* enemy, long value,
                         long attack_distance);                 // 0x448840
    long getAITargetTime(long speed) const;                  // 0x448bd0
    long getTotalCombatValue(long lowest_attack,
                                long lowest_defense) const;     // 0x442e60
    double getUnitCombatValue(long lowest_attack, long lowest_defense,
                                 unsigned char ranged,
                                 const army* excluded) const;   // 0x442a50
    // Returns float in st(0): the stack's own 0x4a0 while
    // fireShieldRounds is set, else the Efreet Sultan's innate
    // constant, else the zero constant (0x443130).
    float getFireShieldStrength() const;                     // 0x443130
    // The controller/owner pair. combatSide (+0xf4) stores the OWNER's
    // side, so the body that APPLIES the hypnotize flip is the
    // controller and the raw read is the owner - the inversion this
    // header carried until 2026-08-14, CLOSED 2026-08-15 on the DC
    // line table plus a full `call rel32` scan of retail .text. The
    // decisive half is that the dump names the field itself:
    // army::get_owning_side (Army.h:795, dc 0x27d3c, 8 B) is a bare
    // load of the side field and army::get_controlling_side
    // (Army.h:800, dc 0x27d44, 0x30 B) is the `1 - get_owning_side()`
    // flip - so the raw read is the OWNING side by construction. The
    // full argument, including the retail callsite multiplicities
    // (2:0 in compute_fire_shield_damage, 2:1 in the TViewArmyWindow
    // ctor) and the withdrawal of the old is_enemy argument, is in
    // army.cpp's note above the pair. Do not re-litigate.
    // 0x442690 (57 B, ecx only): heroes[get_controlling_side()], the
    // hero currently DIRECTING this stack.
    hero* getController() const;                               // 0x442690
    // 0x4426d0 (20 B, ecx only): heroes[combatSide], the hero who
    // OWNS this stack regardless of who is directing it.
    hero* getOwner() const;                                    // 0x4426d0
    // 0x43d8b0 / 0x43d9f0, LOCATED 2026-08-13 from combatManager::AddArmy
    // (0x47a100), which calls them back to back on the freshly claimed
    // slot. Init's SEVEN stack arguments are an exact arity match for the
    // DC army.cpp:261 prototype and AddArmy supplies each one in the DC
    // order (armyId, count, the side's hero, side, slot, hex, -1);
    // LoadResources takes only `this`. Size ratios corroborate the pair
    // at the same 1.35 (309/228) and 1.36 (1317/970).
    // 0x446e30 (737 B), the DC roster's army::new_turn (army.cpp:5177,
    // dc 0x4ba88), attributed to army.obj by link order. Void and
    // argument-less: combatManager::NextArmy (0x465080) calls it with
    // the selected stack in ecx and nothing on the stack, once per stack
    // as that stack comes up. Declared, not claimed - army.cpp owns the
    // body.
    void newTurn();
    // 0x43d5c0 (358 B) <- army::InitClean (dc 0x438e8, 200 B, 1 param =
    // `this` only; SH4->x86 ratio 1.79, in band). LoadArmies (0x463600)
    // calls it on each of the twenty slots it has just blanked, and the
    // DC order InitClean < Init < LoadResources holds in retail too -
    // 0x43d5c0 < 0x43d8b0 < 0x43d9f0 - which is what corroborates the
    // arity match. The Init/LoadResources pair above was located
    // independently from AddArmy and needs no gate, and neither does
    // this one any more: the gate it carried was a declarator-count
    // gate, which is the class the view-gate audit is unwinding.
    // Init/initialize/InitClean/LoadResources are declared in the
    // evidence-ordered prefix above; army.cpp owns their bodies.

    // Header-inline declarations. Their exact positions in the LF_FIELDLIST
    // are audited separately; their bodies follow the class in Army.h source
    // order; keep them instead of TU-specific score scaffolding.
    unsigned char canCastResurrect() const;
    int getMorale(unsigned char apply_limits) const;
    int getLuck(unsigned char apply_limits) const;
    int offsetToFront(int direction) const;
    void clearAIValues();
    unsigned char needToTurn(int direction) const;
    unsigned char is(unsigned attribute) const;
    long getAIExpectedDamage() const;
    const army* getAITarget() const;
    long getAITargetValue() const;
    long getAITargetTime() const;
    long getAIPossibleTargets() const;
    int getOwningSide() const;
    int getControllingSide() const;
    const char* getName() const;
    const char* getName(int count) const;
    long getSpellTime(int spell) const;
    TSkillMastery getSpellLevel(int spell) const;
    unsigned char isActive() const;
    unsigned char isInAura() const;
    unsigned char isIncapacitated() const;
    unsigned char canRetaliate(const army& attacker) const;
    unsigned char cannotAttack() const;
    long getAdjacentHex(long direction) const;
    long getAttackDirection(const army* enemy) const;
    unsigned char isInAreaHighlight() const;

private:
    unsigned char leavesNoBody() const;

    // LF_FIELDLIST 0x205b entries 237..249: the private data tail follows
    // every public and private method declaration.  The names at +0x4e8,
    // +0x4ec, +0x4f0 and +0x540 retain their established retail aliases;
    // the order and access are retained Dreamcast source facts.
    int morale;                           // +0x4e8, DC iMorale
    int luck;                             // +0x4ec, DC iLuck
    unsigned char m_resetThisRound;              // +0x4f0, DC reset_this_round
    unsigned char m_isAreaEffectTarget;  // +0x4f1
    std::vector<army*> m_boundArmies;      // +0x4f4
    std::vector<army*> m_binders;           // +0x504
    std::vector<army*> m_auraClients;      // +0x514
    std::vector<army*> m_auraSources;      // +0x524
    int m_aiExpectedDamage;               // +0x534
    army* m_aiTarget;                      // +0x538
    long m_aiTargetValue;                 // +0x53c
    long m_aiTargetTime;                  // +0x540, DC AI_target_distance
    unsigned m_aiPossibleTargets;         // +0x544
#endif  // superseded unordered/view-fragmented declarations
};
SIZE(army, 0x548);

// Dreamcast CodeView source lines Army.h:718..881 are out-of-class header
// definitions.  LF_FIELDLIST 0x205b proves this: these declarations are
// interspersed through the class roster while the bodies form this later
// source-line run.

    // E:\gamedcs\Army.h:718
inline bool army::canCastResurrect() const
    {
        return (m_creatureType == ARMY_CREATURE_ARCHANGEL
                || m_creatureType == ARMY_CREATURE_PIT_LORD)
               && m_monInfo.m_hasSpell > 0;
    }

    // E:\gamedcs\Army.h:724
inline int army::getMorale(unsigned char applyLimits) const
    {
        return applyLimits ? limit(-3, m_morale, 3) : m_morale;
    }

    // E:\gamedcs\Army.h:730
inline int army::getLuck(unsigned char applyLimits) const
    {
        return applyLimits ? limit(-3, m_luck, 3) : m_luck;
    }

    // E:\gamedcs\Army.h:736
VA(0x00445cd0, 0x38)  // anchor-caller + exact header-inline body, dc 0x27c9c
inline int army::offsetToFront(int direction) const
    {
        if (direction >= 0 && direction <= 2)
            return 1;
        if (direction >= 3 && direction <= 5)
            return -1;
        return m_facing ? 1 : -1;
    }

    // E:\gamedcs\Army.h:752
inline void army::clearAIValues()
    {
        m_aiExpectedDamage = 0;
        m_aiTarget = 0;
        m_aiTargetTime = 0;
        m_aiTargetValue = 0;
    }

    // E:\gamedcs\Army.h:760
inline bool army::needToTurn(int direction) const
    {
        return direction < 6 && (m_facing == 0) != (direction >= 3);
    }

    // E:\gamedcs\Army.h:765
inline bool army::is(unsigned attribute) const
    {
        return (m_monInfo.m_attributes & attribute) != 0;
    }

    // E:\gamedcs\Army.h:770
inline long army::getAIExpectedDamage() const
    {
        return m_aiExpectedDamage;
    }

    // E:\gamedcs\Army.h:775
inline const army* army::getAITarget() const
    {
        return m_aiTarget;
    }

    // E:\gamedcs\Army.h:780
inline long army::getAITargetValue() const
    {
        return m_aiTargetValue;
    }

    // E:\gamedcs\Army.h:785
inline long army::getAITargetTime() const
    {
        return getAITargetTime(getSpeed());
    }

    // E:\gamedcs\Army.h:790
inline long army::getAIPossibleTargets() const
    {
        return m_aiPossibleTargets;
    }

    // E:\gamedcs\Army.h:795
inline int army::getOwningSide() const
    {
        return m_combatSide;
    }

    // E:\gamedcs\Army.h:800
VA(0x00440140, 0x1F)  // anchor-callee + body identity, retail-only slot
inline int army::getControllingSide() const
    {
        if (m_spellInfluence[60])
            return 1 - getOwningSide();
        return getOwningSide();
    }

    // E:\gamedcs\Army.h:810
inline const char* army::getName() const
    {
        return getArmyName(m_creatureType, m_numTroops);
    }

    // E:\gamedcs\Army.h:815
inline const char* army::getName(int count) const
    {
        return getArmyName(m_creatureType, count);
    }

    // SpellID is still represented by its retail-width int domain here.
    // E:\gamedcs\Army.h:820
inline long army::getSpellTime(int spell) const
    {
        return m_spellInfluence[spell];
    }

    // E:\gamedcs\Army.h:825
inline TSkillMastery army::getSpellLevel(int spell) const
    {
        return TSkillMastery(m_spellLevel[spell]);
    }

    // E:\gamedcs\Army.h:830
inline bool army::isActive() const
    {
        return m_creatureType >= 0 && m_numTroops > 0;
    }

    // E:\gamedcs\Army.h:835
inline bool army::isInAura() const
    {
        return m_auraSources.size() > 0;
    }

VA(0x0041f380, 0x27)  // anchor-callee, dc 0x27d9c
inline bool army::isIncapacitated() const
    {
        return m_spellInfluence[62] || m_spellInfluence[70]
               || m_spellInfluence[74];
    }

    // E:\gamedcs\Army.h:847
inline bool army::canRetaliate(const army& attacker) const
    {
        return !(attacker.is(1u << 16)) && !m_spellInfluence[70]
               && m_retaliationCount > 0;
    }

    // E:\gamedcs\Army.h:855
// Complete's inlined copy in consider_single_enchantment keeps the recovered
// incapacity/attribute prefix but directly contradicts Dreamcast's final
// Psychic/Magic Elemental pair: retail compares First Aid Tent and Ammo Cart.
inline bool army::cannotAttack() const
    {
        return isIncapacitated() || is(1u << 21)
               || m_creatureType == ARMY_CREATURE_FIRST_AID_TENT
               || m_creatureType == ARMY_CREATURE_AMMO_CART;
    }

    // E:\gamedcs\Army.h:864
inline long army::getAdjacentHex(long direction) const
    {
        return getAdjacentHex(m_gridIndex, direction);
    }

    // E:\gamedcs\Army.h:869
inline long army::getAttackDirection(const army* enemy) const
    {
        return getAttackDirection(m_gridIndex, enemy);
    }

    // E:\gamedcs\Army.h:875
inline bool army::leavesNoBody() const
    {
        return is((1u << 22) | (1u << 28));
    }
    // E:\gamedcs\Army.h:881
inline bool army::isInAreaHighlight() const
    {
        return m_isAreaEffectTarget;
    }
// The WIDE-creature direction ring, byte-read from the hash-verified
// image and self-proving: the two tables are exact mutual inverses
// (0x660878 = {0,1,2,4,5,6,7,3}, 0x660898 = {0,1,2,7,3,4,5,6}), so one
// maps a direction id onto its position around an eight-slot ring and
// the other maps back. That is what lets a two-hex stack's clockwise /
// counter-clockwise neighbours be found with a single +-1 step modulo
// 8 where a one-hex stack needs only +-1 modulo 6: the two WIDE slots
// 6 and 7 do not sit at the end of the ring, they sit between the real
// neighbours (the ring order is 0,1,2,7,3,4,5,6), and the tables exist
// to say where. Sliced by army::get_clockwise / get_counter_clockwise,
// whose only located expansion is get_multi_head_directions
// (0x448ab0). Names are bootstrap inventions - no roster attests them.
DATA(0x00660878) extern const long g_wideDirectionRingIndex[8];
DATA(0x00660898) extern const long g_wideDirectionRingOrder[8];

// The five globals a walk publishes for the redraw, and their NAMES ARE
// THE DREAMCAST LITERAL POOL'S - army::Walk's own SH4 body (dc 0x45254)
// loads ?giWalkingFrom@@3HA / ?giWalkingFrom2@@3HA / ?giWalkingTo@@3HA /
// ?giWalkingTo2@@3HA / ?giWalkingYMod@@3HA in exactly the order retail's
// five stores use, so the pairing is positional AND nominal. The `2`
// pair is the SECOND hex of a two-hex stack and is set to -1 for a
// one-hex one; all four are reset to -1 once the move has been placed.
// They sit immediately below akWideDirectionRingIndex at 0x660878,
// which is the four dwords 0x660868..0x660874 exactly.
DATA(0x00660868) extern int g_walkingFrom;
DATA(0x0066086c) extern int g_walkingFrom2;
DATA(0x00660870) extern int g_walkingTo;
DATA(0x00660874) extern int g_walkingTo2;
DATA(0x00693858) extern int g_walkingYMod;

unsigned char isValidCaliphSpell(SpellID spell, const army* target);
// 0x447a80 (1065 B), the worker is_valid_caliph_spell tail-jumps to
// and army::can_cast_spell (0x4476c0) also calls. It opens by
// rejecting a target that already carries the spell
// (`target->rounds[spell]`, the +0x198 row base again). The DC build
// has NO row for it - that logic lives inside DC's
// is_valid_caliph_spell and can_cast_spell - so it is a retail-only
// factoring and the NAME BELOW IS A BOOTSTRAP INVENTION, same class as
// get_estimated_damage. Declared so the wrapper can call it; not claimed.
unsigned char spellIsValidOnTarget(int spell, const army* target);

// E:\gamedcs\army.cpp:917, dc 0x44e14
// E:\gamedcs\army.cpp:2708, dc 0x47944
// E:\gamedcs\army.cpp:4436, dc 0x4a8c8
// E:\gamedcs\army.cpp:5433, dc 0x4c0f0
// E:\gamedcs\army.cpp:5451, dc 0x4c154
// E:\gamedcs\army.cpp:5469, dc 0x4c1b8
// E:\gamedcs\includes.h:117, dc 0x4c9c0
// E:\gamedcs\DC_precompiledheaders.h:41, dc 0x4d044

#endif  /* HOMM3_ARMY_H */
