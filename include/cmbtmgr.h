#ifndef HOMM3_CMBTMGR_H
#define HOMM3_CMBTMGR_H

#include <set>
#include <vector>

#include "army.h"
#include "armygrp.h"
#include "basemgr.h"
#include "hexcell.h"
#include "struct.h"
#include "winmgr.h"

class Bitmap16Bit;
class CNetMsgHandlerPause;
class Bitmap816;
class CSprite;
class hero;
class heroWindow;
class iconWidget;
class textWidget;
class TCombatWindow;
class NewmapCell;
class searchArray;
class town;
struct type_AI_combat_parameters;
struct tagPOINT;
struct type_artifact;

// Four inclusive drawing bounds copied as one value before a drawbridge
// animation. Drawing.cpp's decoded readers prove the same min/max layout as
// SLimitData. Keep the concrete source type consumer-scoped so the other VC6
// units retain their already measured type-handle state.
typedef SLimitData TDrawbridgeBounds;
SIZE(TDrawbridgeBounds, 0x10);

// Polymorphic objects owned by combatManager at offsets where Close only
// proves scalar deletion. Their concrete roles remain unattested.
class CCombatOwnedObject {
public:
    virtual ~CCombatOwnedObject();
};

// One segment of an animated lightning bolt. THE DREAMCAST DUMP HAS NO
// MEMBER EVIDENCE FOR THIS TYPE AT ALL - members.csv carries zero rows
// and the NAME comes only from the three spells.cpp prototypes that take
// an SBolt* - so the layout below is reconstructed from retail bytes and
// nothing else, and the field names are ordinal wherever the bytes do
// not name them.

// The 0x78 STRIDE is byte-proven twice over by DoBolt (0x5a5c20): it
// allocates `new(0xbb8)` = 3000 bytes for the working set and walks it
// with five separate `add r32, 0x78` steps, i.e. 25 records of 120.

// The named fields are named because AddBolt (0x5a5a90) stores a DC-named
// PARAMETER straight into them - it takes thirteen arguments and lands
// twenty-four field writes, which is what fixes the first half of this
// record. The rest is ResetBoltAngle (0x5a5260) and DrawBolt (0x5a5440).
struct SBolt {
public:
    // The four endpoints, clamped by AddBolt into the 800x556 screen
    // (`0..0x31f` on x and `0..0x22b` on y) before they are stored.
    int m_sourceX;  // +0x00
    int m_sourceY;  // +0x04
    int m_destX;  // +0x08
    int m_destY;  // +0x0c
    int m_splitFrequency;  // +0x10
    // The CURRENT thickness. AddBolt seeds it from iStartThickness and
    // ResetBoltAngle re-interpolates it toward iEndThickness each step.
    int m_thickness;  // +0x14
    int m_color;  // +0x18
    int m_field1c;
    int m_segmentLength;  // +0x20
    // The pen position, carried in float so the walk can advance by a
    // fractional step; AddBolt seeds both from the (clamped) source.
    float m_x;  // +0x24
    float m_y;  // +0x28
    // The same position rounded to whole pixels - the end of the segment
    // drawn so far, which is what ResetBoltAngle measures the remaining
    // distance from.
    int m_pixelX;  // +0x2c
    int m_pixelY;  // +0x30
    // Set when the run is longer than it is tall. AddBolt decides it from
    // `abs(dx) > abs(dy)` for an ordinary bolt, and for the two colours
    // below from whether the source x is strictly inside the screen -
    // those two are drawn as screen-edge flashes, so the shallow/steep
    // choice is made by where the bolt STARTS rather than by its run.
    int m_shallow;  // +0x34
    float m_angle;  // +0x38
    // fAngle plus the progress-weighted distortion ResetBoltAngle adds.
    float m_distortedAngle;  // +0x3c
    // "This segment has reached its destination". AddBolt clears it and
    // DrawBolt sets it; DoBolt (0x5a5c20) is what names it - after each
    // drawn pass it walks all live bolts and stops the whole animation
    // only when EVERY one of them has this set, and it skips a bolt's
    // ResetBoltAngle and its split check on the same test.
    int m_atDestination;  // +0x40
    // ResetBoltAngle returns at once while this is set - the "this
    // segment is finished" latch.
    int m_done;  // +0x44
    // Retail DrawBolt 0x5a5440 seeds this when Manhattan distance to the
    // endpoint falls below 15, then only lowers it. Passing the closest
    // approach by more than one pixel ends the bolt. Role-derived name.
    int m_closestDistance;
    // The half-width span the thick line is drawn over, recomputed from
    // iThickness every reset: -(t >> 1) to -(t >> 1) + t - 1.
    int m_spanFirst;  // +0x4c
    int m_spanLast;  // +0x50
    // The segment's own start, kept while fX/fY walk away from it, and
    // RESTAMPED to the current position every time DoBolt forks a new
    // bolt off this one - that is how a fork throttles the next fork,
    // since the split test measures the manhattan distance from here.
    int m_startX;  // +0x54
    int m_startY;  // +0x58
    int m_startThickness;  // +0x5c
    int m_endThickness;  // +0x60
    // The straight-line distance from source to destination, measured
    // once by AddBolt; ResetBoltAngle divides the remaining distance by
    // it to get fProgress.
    int m_totalLength;  // +0x64
    int m_angleDistortMin;  // +0x68
    int m_angleDistortMax;  // +0x6c
    // 0 at the source, 1 at the destination.
    float m_progress;  // +0x70
    int m_distortAlways;  // +0x74
};
SIZE(SBolt, 0x78);

// SBolt::iColor's SPECIAL values. DrawBolt (0x5a5440) proves both the
// domain and its extent: it lowers `iColor - 0x12c` against 6 into a
// jump table and for anything outside that window writes the value
// straight into the framebuffer as a raw 16-bit pixel. Only one member
// can be named from a caller - ChainLightning (0x5a6360) pushes the
// literal 0x131 into DoBolt - so the other five stay ORDINAL, exactly
// as army.h's wallTargets rows do where no roster names the individual
// segments.

enum EBoltColor {
    BOLT_COLOR_0 = 0x12c,
    BOLT_COLOR_1 = 0x12d,
    BOLT_COLOR_2 = 0x12e,
    BOLT_COLOR_3 = 0x12f,
    BOLT_COLOR_4 = 0x130,
    BOLT_COLOR_CHAIN_LIGHTNING = 0x131
};

// The combat's spell-restriction code, held in combatManager+0x53c0.
// The per-combat initializer at 0x4643b0 writes it exactly once, as -1
// or one of 0..9 from ten straight-line branches, and clears the two
// bytes above it in the same breath. The only value any decoded reader
// tests is 2: can_cast_spells (0x41f890) refuses a CREATURE cast under
// it while still allowing a hero cast. BOOTSTRAP INVENTION - no roster
// attests the domain or the spelling, and the other nine codes stay
// unnamed until one of the 0x4643b0 branches is decoded.

// FIVE MORE CODES DECODED 2026-08-20 from the other side, by
// ai_tactical's creature-spell pricers get_ogre_mage_value (0x43c330)
// and get_caliph_value (0x43c4a0). Both open with the same
// jump-table switch over this word, and each arm's only effect is to
// raise the mastery the spell is priced at from ADVANCED to EXPERT:
// code 1 raises it unconditionally, and codes 6/7/8/9 raise it only
// when the spell's own school mask (`akSpellTraits[spell].school`,
// +0x1c) carries bit 2 / bit 1 / bit 3 / bit 0 - i.e. eSchoolWater,
// eSchoolFire, eSchoolEarth and eSchoolAir respectively. The
// enumerators are named after that measured EFFECT rather than after
// the battlefield that sets the code, so no roster is being invented;
// for the record the effects are the five spell-affecting special
// battlefields' published rules, and code 2's already-recorded
// "creature casts refused" is the sixth.
enum ECombatSpellRestriction {
    COMBAT_SPELL_RESTRICTION_NO_CREATURE_SPELLS = 0x2
    ,
    COMBAT_SPELL_RESTRICTION_ALL_EXPERT = 0x1,
    COMBAT_SPELL_RESTRICTION_WATER_EXPERT = 0x6,
    COMBAT_SPELL_RESTRICTION_FIRE_EXPERT = 0x7,
    COMBAT_SPELL_RESTRICTION_EARTH_EXPERT = 0x8,
    COMBAT_SPELL_RESTRICTION_AIR_EXPERT = 0x9
};

// Combat-grid geometry, byte-proven wherever a cmbtmgr or findpath body
// touches it: the cell array is 187 entries (`cmp esi, 0xbb` guards it
// in move_toward, SeedCombatPosition and FindCombatPath alike) laid out
// 17 to a row (`idiv 0x11` in get_distance, IsInMoat, PlaceObstacle and
// RemoveObstacle), which makes columns 0 and 16 the two off-field
// margins SeedCombatPosition refuses to seed a wide stack's tail into.
enum ECombatGrid {
    COMBAT_GRID_CELLS = 0xbb,
    COMBAT_GRID_ROW_STRIDE = 0x11,
    COMBAT_GRID_LAST_COLUMN = 0x10,
    // The two hero-portrait pseudo-hexes GetGridIndex answers with when
    // the cursor is over a hero panel rather than the field. RightClick
    // (0x4769c0) is what pairs each with a side: 0xfc opens heroes[0]'s
    // panel and 0xfd heroes[1]'s, which is also the order the four
    // screen hit rectangles at 0x694ea8..0x694f08 are tested in.
    COMBAT_HEX_ATTACKER_HERO = 0xfc,
    COMBAT_HEX_DEFENDER_HERO = 0xfd,
    COMBAT_HEX_KEEP = 0xfe,
    COMBAT_HEX_UPPER_TOWER = 0xff
    ,
    COMBAT_HEX_LOWER_TOWER = 0xfb
};

// SetCombatDirections has two approach records per ordinary six-way combat
// direction. These four endpoints are the source's asymmetric tie-break set.
enum ECombatAttackAngle {
    COMBAT_ATTACK_ANGLE_0 = 0,
    COMBAT_ATTACK_ANGLE_5 = 5,
    COMBAT_ATTACK_ANGLE_6 = 6,
    COMBAT_ATTACK_ANGLE_11 = 11,
    COMBAT_ATTACK_ANGLE_COUNT = 12
};

// The drawbridge state held in combatManager+0x53a4. LowerDoor
// (0x4671c0) walks it 3 -> 2 -> 1 with one DrawFrame per step and
// leaves it at 1; RaiseDoor (0x4672e0) refuses to run unless it already
// reads 1. Its three decoded readers (HexIsBlocked 0x469a10,
// should_lower_door 0x467130, IsInMoat 0x469dc0) all gate on the value
// 3 - the bridge still up. The intermediate 2 is an animation frame
// only, so it stays unnamed. BOOTSTRAP INVENTION - no roster attests
// the domain.
enum EDrawbridgeState {
    DRAWBRIDGE_DOWN = 0x1,
    DRAWBRIDGE_UP = 0x3
};

// The defending town's wall tier. InitializeArchers proves that a Citadel
// installs the keep archer and a Castle installs the two tower archers too;
// the other combat readers only distinguish an absent wall from any tier.
enum ECombatFortification {
    COMBAT_FORTIFICATION_NONE = 0,
    COMBAT_FORTIFICATION_FORT = 1,
    COMBAT_FORTIFICATION_CITADEL = 2,
    COMBAT_FORTIFICATION_CASTLE = 3
};

// Row 5's three special columns of the 11x17 combat grid, named from
// the byte tables they are the fifth entry of: 0x60 is
// gCastleWallColumns[5] (the gate itself), 0x5f gMoatColumns[5] and
// 0x5e gOuterMoatColumns[5]. HexIsBlocked admits the first two,
// should_lower_door all three, and IsInMoat excludes 0x5f from the
// moat proper (0x5e from the Fortress row) while the bridge is down.
// Names are BOOTSTRAP INVENTIONS.
enum ECombatGateHex {
    // The ROW those three columns are the fifth entry of, which is what
    // SetupAndLoadObstacles compares against directly: it lays a mine in
    // every moat row of a Tower except this one, and it tests the row
    // INDEX before loading gMoatColumns rather than testing the column
    // against COMBAT_HEX_GATE_MOAT afterwards. Behind the obstacle view
    // so only its one consumer pays the include-set threshold, and
    // FIRST in the enum so the guard does not leave a trailing comma.
    COMBAT_GATE_ROW = 5,
    COMBAT_HEX_OUTER_MOAT = 0x5e,
    COMBAT_HEX_GATE_MOAT = 0x5f,
    COMBAT_HEX_GATE = 0x60
};

// One faction row in the siege-archer table at 0x63cf88. Retail indexes
// nine 0x20-byte rows by town::type, then uses the three coordinate pairs
// for the keep, lower tower and upper tower respectively.
struct TSiegeArcherPosition {
public:
    int m_x;
    int m_y;
};

struct TSiegeArcherInfo {
public:
    int m_creatureType;
    TSiegeArcherPosition m_positions[3];
    const char* m_shadowSpriteName;
};

extern const TSiegeArcherInfo g_siegeArcherInfo[9];

// Two additional TTerrainType values needed only by combat terrain
// selection. Kept out of armygrp.h's include-sensitive enum: adding
// enumerators there perturbs VC6 code generation in initialize.obj.
enum ECombatTerrainValue {
    COMBAT_TERRAIN_SAND = 1,
    COMBAT_TERRAIN_SUBTERRANEAN = 6
};

// Mine type sentinels that force an underground combat background.
// Their values are retail-proven; their semantic names are not.
enum ECombatMineType {
    COMBAT_MINE_TYPE_4 = 4,
    COMBAT_MINE_TYPE_6 = 6
};

// The eighteen combat hero animations, keyed `2 * townType + sex`.
// Retail .rdata 0x63bd40; LoadIcons' `shl eax, 4` proves the 16-byte
// stride. CastSpell reads all three trailing dwords as the cast origin
// and animation length. DC independently types the row combatManager::SCmbtHero
// {SpriteName, castX, castY, castFrame}; the retail table is 18 rows
// where the Dreamcast's is 16.
struct TCombatHeroSprite {
public:
    const char* m_defName;
    int m_castX;
    int m_castY;
    int m_castFrame;
};
extern const TCombatHeroSprite g_combatHeroSprites[18];

// Head model from the byte-proven leaves. The battlefield holds two
// sides of 21 army slots (20 used - ResetHitByCreature clears exactly
// 20 per side - plus one spare making the 0x6ee8 side stride); the
// army array base 0x54cc and the 0x548 record stride are byte-proven
// by hexcell::get_army/get_dead_army, which index side*21 + slot.
// (Rebased 2026-08-06: the earlier TCombatSide/TStack view sat 0x34
// past the real army boundary and its field offsets were 0x50 short -
// objdiff's immediate masking hid the error as the old 99.9 residual
// on ResetHitByCreature.)
// The pending-order CODES combatManager::field_3c carries. All three
// rungs below are byte-witnessed, and the two writers corroborate each
// other:
//   - army::berserk_attack (0x4222c0) stores 6 on BOTH of its melee
//     paths and 12 on the path where it gives up, and it is also the
//     writer that fills the (field_40, field_44) pair the field_40
//     comment describes as "(where I go, what I hit)" - so 6 is the
//     move-and-strike order;
//   - army::GoBerserk (0x4456d0) stores 7 on the branch it takes for a
//     ballista, an arrow tower or a stack with shots left, writing only
//     field_44, and 12 when the target list comes back empty.
// ai_tactical's consider_teleport (0x43aa60) reads the pair back and
// accepts a teleport only while the code is 6, which is the third
// witness for that rung.
// The SPELLINGS are behaviour-derived and provisional - no roster or
// string reaches this domain, exactly as the three field names it
// describes are address ordinals.
enum EAIOrder {
    AI_ORDER_CAST_SPELL = 1,
    AI_ORDER_MOVE_AND_ATTACK = 6,
    AI_ORDER_SHOOT = 7,
    AI_ORDER_CREATURE_SPELL = 10,
    AI_ORDER_NONE = 12
};

// Per-castle hex-index table at 0x63bd00: InCastle divides the hex by
// 0x11 (the row stride) and compares against the row's wall column.
// The retail body takes its argument in ECX with no stack frame - a
// FREE fastcall function, not a method (the DC name is scoped to
// combatManager, so retail moved it out of the class). It is declared
// here because the Dreamcast-proven TWallTarget inline below uses it.
extern const unsigned char g_castleWallColumns[];

// The combat-hero sprite state is stored as an int rather than as a
// CodeView enum. One is the timed idle fidget; the two event-driven states
// have no surviving public names, so their source-facing names keep the
// retail ordinals explicit.
enum CombatHeroFrameType {
    COMBAT_HERO_FRAME_IDLE = 0,
    COMBAT_HERO_FRAME_FIDGET = 1,
    COMBAT_HERO_FRAME_EVENT_2 = 2,
    COMBAT_HERO_FRAME_EVENT_3 = 3
};

class combatManager : public baseManager {
public:
    // Original DC statics: LeftHeroLimits, RightHeroLimits,
    // MainBuildingLimits and UpperTowerLimits (GetGridIndex).
    static const SLimitData s_leftHeroLimits;
    static const SLimitData s_rightHeroLimits;
    static const SLimitData s_mainBuildingLimits;
    static const SLimitData s_upperTowerLimits;
    // drawing.cpp:666, Dreamcast dc 0x841d4. range_attack uses this
    // five-argument overload to center the Magog effect before animating it.
    unsigned char scrollTo(int x, int y, unsigned char draw,
                           unsigned char doscrollX,
                           unsigned char doscrollY);
    // DC CmbtMgr.h's complete nested enum. Command's get_tower_string takes
    // this type by value; retail indexes the same eighteen wall rows.
    enum TWallSection {
        eWallSectionDoor = 0,
        eWallSectionDoorRope = 1,
        eWallSectionMoat = 2,
        eWallSectionMoatLip = 3,
        eWallSectionBackWall = 4,
        eWallSectionUpperTower = 5,
        eWallSectionUpperWall = 6,
        eWallSectionUpperButtress = 7,
        eWallSectionMidUpperWall = 8,
        eWallSectionGate = 9,
        eWallSectionMidLowerWall = 10,
        eWallSectionLowerButtress = 11,
        eWallSectionLowerWall = 12,
        eWallSectionLowerTower = 13,
        eWallSectionMainBuilding = 14,
        eWallSectionMainBuildingCover = 15,
        eWallSectionLowerTowerCover = 16,
        eWallSectionUpperTowerCover = 17,
        kNumWallSections = 18
    };
    // The exact Dreamcast nested record. Retail independently proves the
    // whole 20-byte layout across PlaceAllObstacles, place_obstacle,
    // PlaceObstacle, RemoveObstacle and DrawFrame: the catalogue masks are
    // at +0/+2, placement bounds at +4/+5, signed occupied-hex offsets at
    // +8, and the sprite name at +0x10.
    struct TObstacleInfo {
        unsigned short m_terrainMask;
        unsigned short m_specialTerrainMask;
        unsigned char m_minRow;
        unsigned char m_width;
        unsigned char m_extraHexCount;
        unsigned char m_underlay;
        signed char m_extraHexOffsets[8];
        const char* m_spriteName;
    };

private:
    static const TObstacleInfo s_obstacleInfo[];
    // Dreamcast records all four as private static arrays. Their raw MSVC
    // names (`@@0QBU...`) independently distinguish this declaration from
    // both a singleton object and a public static member. Retail fixes the
    // first elements at 0x63cee8, 0x63cf00 and 0x63cf18 respectively.
    static const TObstacleInfo s_quicksandInfo[];
    static const TObstacleInfo s_landMineInfo[];
    static const TObstacleInfo s_wallObstacleInfo[];
    // One 68-byte elevation-overlay catalogue at retail 0x0063bec0.
    // Dreamcast PlaceLargeObstacle references sElevationOverlay directly;
    // retail placement reads masks at +0/+2 and 25 blocked shorts at +0xc.
    // DrawBackground reads x/y at +4/+8 and FileName at +0x40 from the
    // same rows. The unused synthetic TLargeObstacleInfo/LargeObstacleInfo
    // declaration duplicated this type: opaque_04 was x/y; opaque_3e was
    // two alignment bytes followed by FileName. Keep one owning record.
    struct SElevationOverlay {
        unsigned short m_terrainMask;
        // PC adds a second terrain mask in Dreamcast's alignment slot.
        // Retail PlaceLargeObstacle reads 0x0063bec2 + row*68.
        unsigned short m_specialTerrainMask;
        int m_x;
        int m_y;
        // Dreamcast type 0x4328 gives 25 entries (50 bytes); retail's
        // loop also stops at 25. +0x3e..+0x3f align the filename pointer.
        short m_blockedSquares[25];
        const char* m_fileName;
    };
    static const SElevationOverlay s_elevationOverlay[34];

public:
    // Retail writes only name/hitpoints here; Dreamcast CodeView supplies
    // the intervening field identities and confirms the 36-byte extent.
    struct TWallTraits {
        short m_x;
        short m_y;
        short m_hex;
        // Dreamcast x/y/hex are shorts at 0/2/4, followed by filenames
        // at +8. These two bytes align the retail pointer array.
        short m_paddingBeforeFilenames;
        const char* m_filenames[5];
        const char* m_name;
        short m_hitpoints;
        // Dreamcast hitpoints is a short at +0x20; the retail table
        // uses a 0x24-byte record stride. These two bytes align its extent.
        short m_tailPadding;
    };

private:
    static TWallTraits s_wallTraits[9][18];

public:
    enum {
        // The moat row of a town's eighteen wall records: LoadIcons
        // (0x463370) suppresses exactly this row's five icons for
        // Stronghold under the pre-expansion ruleset, and the static
        // table's row 2 is the SgCsMoat.pcx group. An include-set
        // trigger: this enumerator moved command.obj's GetCommand when
        // visible to that TU (measured; max/hist hold the peak).
        WALL_TRAITS_ROW_MOAT = 2
    };
    // One placed obstacle. Stride 0x18 is byte-proven by RemoveObstacle
    // (0x466b30), which divides the manager's obstacle vector extent
    // (+0x13d5c .. +0x13d60) by 24 with the 0x2aaaaaab/sar 2 magic; the
    // shape pointer at +0x4 and the anchor hex BYTE at +0x8 are proven
    // by the same body plus PlaceObstacle. Slot 0 holds a polymorphic
    // object RemoveObstacle virtual-calls (vtable slot 1) and then
    // clears - left padded until that class is modelled.
    struct TObstacle {
        // RemoveObstacle calls vtable slot 1 on it with no arguments
        // and then clears the slot - CSprite's slot 1 is Dispose().
        CSprite* m_sprite;                    // +0x0
        const TObstacleInfo* m_shape;         // +0x4
        unsigned char m_hex;                  // +0x8
        // DC CodeView names these owner/is_visible. Retail independently
        // fixes both offsets: searchArray::set_moat (0x4b3290) marks an
        // obstacle's cell as moat-slowed when either the acting stack's
        // combatSide equals the signed byte at +9 or the byte at +0xa is
        // set; DrawFrame expands the same IsVisible predicate.
        signed char m_owner;                  // +0x9
        unsigned char m_isVisible;           // +0xa
        // Dreamcast grid_index/owner/is_visible occupy bytes 8/9/10;
        // retail mark_firewalls reads damage at +12. This byte aligns it.
        char m_paddingBeforeDamage[0x1];
        // Sliced 2026-08-08 by mark_firewalls (0x4214f0), which feeds
        // this dword straight into ModifySpellDamage as the base
        // damage for the fire-wall spell - so it is the damage the
        // obstacle deals, stored per obstacle when it is placed.
        // Name provisional; no roster reaches the slot.
        long m_spellDamage;                  // +0xc
        // place_obstacle stamps these two on every obstacle it builds -
        // +0x10 zero and +0x14 all-ones - alongside owner/-1 and
        // is_visible/1. No reader is decoded, so both stay ordinals.
        long m_duration;                      // +0x10
        long m_dispelEffect;                      // +0x14

        // Dreamcast CodeView names this one-argument const member and fixes
        // its bool result; the retail DrawFrame expansion proves the two
        // participating bytes at +0x9/+0xa.
        bool isVisible(int side) const
        {
            return side == m_owner || m_isVisible;
        }
    };
    // Dreamcast CodeView records this exact nested type and the public static
    // `combatManager::wallTargets` member. Retail independently proves the
    // 0xc-byte row, all five fields, and the eight-row extent. Keeping the
    // table on the class also lets get_wall_strength retain its original
    // source-visible inline boundary instead of flattening it into callers.
    struct TWallTarget {
        short m_targetHex;             // +0x0
        short m_blockedRow;            // +0x2
        short m_hitX;                  // +0x4
        short m_hitY;                  // +0x6
        TWallSection m_wall;            // +0x8

        int getBlockedHex() const
        {
            if (m_blockedRow != -1)
                return g_castleWallColumns[m_blockedRow];
            return -1;
        }
    };
    static const TWallTarget s_wallTargets[8];
    // One of the three defending-town archer positions. InitializeArchers
    // clears three contiguous 0x24-byte rows at +0x13d78 and fills these
    // members in this order; DamageWall later uses armySlot from each row
    // when the corresponding tower is destroyed.
    // DC TArcher (0x431d) has raw Sprite/Missile pointers. Complete owns
    // both through four-byte resource handles, as in army: 0x462920 clears
    // +4/+8 and 0x462930 releases them in reverse order with EH cleanup.
    struct TArcher {
        int m_creatureType;             // +0x0
        TResourceHandle<CSprite> m_sprite;          // +0x4
        TResourceHandle<CSprite> m_shadowSprite;    // +0x8
        int m_x;                        // +0xc
        int m_y;                        // +0x10
        int m_facing;                 // +0x14
        int m_sequence;                 // +0x18
        int m_frame;                 // +0x1c
        int m_armySlot;                 // +0x20
    };
    // InitializeArchers' two simultaneously live resource locals. Keeping
    // them as one record preserves retail VC6's [-8]/[-4] stack ordering.
    struct TArcherLoadState {
        CSprite* m_sprite;
        const char* m_spriteName;
    };
    // baseManager occupies +0x00..+0x37. soundManager::SetMusicVolume
    // (0x5994b0) independently reads its status member at +0x34 while
    // combat music is active; the retail ctor's baseManager call and
    // combatManager vptr store prove the inheritance directly.
    // A CNetMsgHandlerPause, not a CCombatOwnedObject (retyped in place
    // 2026-08-20, a rename): Open (0x462a20) assigns
    // `new CNetMsgHandlerPause()` here off an operator new(0x10), which
    // is SIZE(CNetMsgHandlerPause, 0x10) exactly, and Close deletes it.
    CNetMsgHandlerPause* m_netMsgHandlerPause;
    // The pending AI order, written as a (code, hex) pair. move_toward
    // (0x41f580) sets the code to 2 the moment a path exists, raises it
    // to 8 when waiting still looks better than the hex it settled on,
    // and finally forces `consider_waiting ? 8 : 3` whenever the hex it
    // settled on is the stack's own. The hex slot starts at the stack's
    // own gridIndex and is overwritten with each accepted step - and
    // the same body compares it back against gridIndex, so it is a
    // combat cell index, not a pointer. Both names are ADDRESS
    // ORDINALS: no DC layout exists for combatManager at all (the
    // Dreamcast dump carries no fieldlist for it) and no string or
    // roster entry reaches either slot.
    int m_nextAction;  // +0x3c
    // The order's FIRST hex slot. berserk_attack (0x4222c0) writes the
    // acting stack's own gridIndex here when the target is already
    // adjacent and the next step of the path when it is not, with
    // field_44 taking the target's hex in both cases - so the pair is
    // (where I go, what I hit). Name is an address ordinal for the same
    // reason its two neighbours are.
    int m_nextActionExtra;  // +0x40
    int m_nextActionGridIndex;  // +0x44
    // The order's fourth slot, and ai_tactical's cast_spell (0x43c800)
    // is the writer that slices it out of the pad: it stamps the
    // chosen spell into field_40, its target hex into field_44 and the
    // choice's own field_18 here, all three behind field_3c = 1.
    // Address ordinal for the same reason its neighbours are.
    int m_nextActionGridIndex2;  // +0x48
    // Two 187-byte per-hex rows, both cleared by Open (0x462a20) with
    // `mov ecx,0x2e / xor eax,eax / rep stosd / stosw / stosb` - 0x2e
    // dwords plus a word plus a byte is exactly COMBAT_GRID_CELLS, and
    // 0x4c + 0xbb == 0x107 and 0x107 + 0xbb == 0x1c2 closes the band
    // exactly against `cells`. Same shape as the 187-byte row at
    // +0x14031. No reader is decoded for either, so both stay ordinals.
    unsigned char m_lastDrawGridShade[COMBAT_GRID_CELLS];  // +0x4c
    unsigned char m_curDrawGridShade[COMBAT_GRID_CELLS];  // +0x107
    // The 187-byte current-shading array ends at +0x1c2; retail
    // ValidAttack places the four-byte-aligned cell array at +0x1c4.
    // Dreamcast retains the same two-byte gap at +0x1d2.
    char m_paddingBeforeCells[0x2];
    // 187 combat cells, stride 0x70 - byte-proven by ValidAttack
    // (0x523bb0: index*112 + 0x1c4).
    hexcell m_cells[187];  // +0x1c4, ends 0x5394
    // PlaceAllObstacles shifts one by this dword while field_53c0 is -1;
    // it is the current combat terrain selector for the catalogue mask.
    int m_terrainType;  // +0x5394
    // GetBackgroundName resets these two dwords after selecting the image.
    // Their wider animation roles await decoded readers/writers.
    int m_combatFringe;  // +0x5398
    int m_combatCycleType;  // +0x539c
    // Selected large-obstacle catalogue id, written by PlaceLargeObstacle.
    int m_largeObstacleId;  // +0x53a0
    // The drawbridge state (EDrawbridgeState). LowerDoor stores 3/2/1
    // through it one frame at a time; RaiseDoor gates on 1;
    // HexIsBlocked, should_lower_door and IsInMoat all gate on 3.
    int m_drawbridgeState;  // +0x53a4
    // "This combat has a moat at all": IsInMoat (0x469dc0) answers 0
    // outright while this byte is clear, before it looks at any row.
    // Name provisional.
    unsigned char m_moatOn;  // +0x53a8
    // "This combat has a SECOND moat row": searchArray::set_moat
    // (0x4b3290) stamps the eleven hexes of the second table, and later
    // re-opens that table's gate hex, only while this byte is set -
    // always nested inside the field_53a8 test above. Name provisional.
    unsigned char m_moatIsWide;  // +0x53a9
    // Dreamcast bMoatOn/moatIsWide are bytes at +0x53b8/9 before
    // the saved-screen pointer at +0x53bc; retail shifts this run by -0x10.
    char m_paddingBeforeSaveScreenPreGrid[0x2];
    // A Bitmap16Bit, not a CCombatOwnedObject (retyped in place
    // 2026-08-20, a rename): Open constructs all THREE of these with
    // Bitmap16Bit::Bitmap16Bit(w, h) at 0x44df70 off an
    // `operator new(0x38)`, which is sizeof(Bitmap16Bit), and field_53b0
    // between them was already spelled that way.
    Bitmap16Bit* m_saveScreenPreGrid;
    // RETYPED 2026-08-13: army::Fly (0x4b4a40) calls Bitmap16Bit::Draw
    // on this slot once per animation frame, blitting the clean
    // battlefield back over the previous frame's extent - so it is the
    // combat back-buffer, not a bare polymorphic object. Close still
    // deletes it through the same slot-0 virtual destructor either way.
    Bitmap16Bit* m_saveScreenPostGrid;
    Bitmap16Bit* m_combatMouseBackground;
    // CombatSystemOptions clears this dword after the modal dialog closes.
    int m_backgroundDrawn;  // +0x53b8
    // Adventure-map cell under the battlefield. DetermineCombatTerrain
    // reads its terrain, object and special-terrain state.
    NewmapCell* m_combatCell;  // +0x53bc
    // can_cast_spells (0x41f890) refuses a CREATURE cast (hero_spell
    // clear) while this word reads 2, and refuses every cast at all
    // while the byte below is set. Both names await a writer.
    int m_magicTerrain;  // +0x53c0
    unsigned char m_onAntiMagicGarrison;  // +0x53c4
    unsigned char m_isSurrounded;  // +0x53c5
    // GetBackgroundName selects CmBkDeck.pcx while this byte is set.
    // Name remains ordinal until its writer is reconstructed.
    unsigned char m_onBoats;  // +0x53c6
    char m_onBeach[0x1];
    // The defending town. DetermineCombatTerrain calls its
    // GetNativeTerrain method; RaiseDoor and IsInMoat independently
    // read the faction byte at town+4.
    town* m_defendingTown;  // +0x53c8
    // The two combat heroes, indexed by side: can_cast_spells indexes
    // heroes[side] for the spellbook test and then walks both slots for
    // the Orb of Inhibition, and army::get_controller (0x442690) /
    // army::get_owner (0x4426d0) do the same lookup off
    // gpCombatManager, with and without the hypnotize flip.
    hero* m_heroes[2];  // +0x53cc
    int m_spellPower[2];  // +0x53d4
    // A per-side latch berserk_attack (0x4222c0) raises, indexed by
    // SIDE as a byte, on exactly one path: when a berserked stack's
    // chosen target turns out to be on its OWN side. Name awaits a
    // writer - no roster or string reaches the pair.
    unsigned char m_playDoh[2];  // +0x53dc
    // Three more per-side byte pairs, all cleared together by
    // InitNonVisualVars (0x463c60) in the order (0x53e0, 0x53e1),
    // (0x53e2, 0x53e3), (0x53dc, 0x53dd), (0x53de, 0x53df) - each pair
    // written HIGH slot first, which is the `x[0] = x[1] = 0` chained
    // form. No reader is decoded for any of them.
    unsigned char m_playYeah[2];  // +0x53de
    unsigned char m_dohPlayedThisRound[2];  // +0x53e0
    unsigned char m_yeahPlayedThisRound[2];  // +0x53e2
    // Two per-side dword pairs LoadIcons (0x463370) clears alongside the
    // two sprite pointers, at full int width (`mov [esi + 4*edi + 0x53e4],
    // edx` with edx held at zero). No reader is decoded yet, so both
    // names stay address ordinals.
    int m_cmbtHeroFrameType[2];  // +0x53e4
    int m_cmbtHeroFrameIndex[2];  // +0x53ec
    // Retail ResetCycleTimers stamps both entries from one GameTime::Get.
    // Dreamcast names the same semantic row cmbtHeroLastFidgetTime; its
    // offset is 0x10 later in that older layout, so the retail placement
    // here comes only from the x86 stores at 0x479f43/0x479f49.
    char m_cmbtHeroDataSet[0x8];
    unsigned long m_cmbtHeroLastFidgetTime[2];  // +0x53fc
    CSprite* m_creatureSprites[2];  // +0x5404
    CSprite* m_heroFlagSprites[2];  // +0x540c
    int m_cmbtHeroFlagFrame[2];  // +0x5414
    // DC CodeView names these two adjacent SLimitData[2] arrays; DrawFrame's
    // four DrawCombatHero calls independently prove the retail offsets.
    SLimitData m_cmbtHeroLimitData[2];  // +0x541c
    SLimitData m_cmbtHeroFlagLimitData[2];  // +0x543c
    // Per-side spells observed during combat and eligible for Eagle Eye.
    // LearnSpellFromEagleEye proves two adjacent 16-byte Dinkumware sets:
    // `(side + 0x546) << 4` addresses the selected set at +0x5460.
    std::set<SpellID> m_eagleEyeData[2];  // +0x545c; set roots at +0x5460
    // Per-stack "this stack has already been affected" marks, indexed
    // [combatSide][army slot]. THREE independent readings agree on the
    // shape, which is why it is sliced in place from the old pad_547c[0x28]
    // with no byte moves and no change in declarator count:
    //   * ClearEffects (0x5a66b0) wipes it as ten dwords - a `rep stosd`
    //     with ecx=10 over [this+0x547c] - which bounds the extent.
    //   * SetMassSpellInfluence (0x5a66d0) writes a 1 per stack that took
    //     the spell, walking a row pointer that advances 0x14 per SIDE and
    //     is indexed by the army slot - which gives the [2][20].
    //   * ai_tactical's get_chain_lightning_value (0x437190) raises
    //     [this + bitIndex + 20*combatSide + 0x547c] before asking
    //     GetNextChainLightningTarget for the next hop - same indexing.
    // Named generally rather than for chain lightning: the mass-spell
    // writer and the chain-lightning writer both use it.
    unsigned char m_effected[2][20];  // +0x547c
    // Per-side "this side is played by the computer" latch: ai_tactical
    // crosses it with gpGame's own AI flag before scaling a shooter's
    // value (get_ranged_attack_value 0x435cb0, the type_AI_combat_
    // parameters ctor 0x435ec0). Name provisional.
    // WRITER FOUND 2026-08-20 (SetupCombat 0x4639f0): the pair is stamped
    // with `gpGame->IsHuman(playerIds[side])`, and with 0 when the side
    // has no player id at all - so the latch reads "this side is HUMAN"
    // and the provisional name above is inverted. Recorded rather than
    // renamed: seventeen readers across ai.cpp, ai_tactical.cpp,
    // combatcontrolsubwindow.cpp and this TU would move with it, and this
    // lane touches none of them (the field_54b4 precedent).
    unsigned char m_sideIsAi[2];  // +0x54a4
    unsigned char m_sideIsLocalHuman[2];  // +0x54a6
    // The adventure-map player ids behind the two combat sides. LowerDoor's
    // inlined IsQuickCombat indexes the 360-byte gpGame->players row with
    // each value and reads that player's +0xe4 quickCombat preference;
    // UpdateArmyGroup applies its creatureId bit-22 exclusion only while
    // the selected id is not -1.
    int m_playerIds[2];  // +0x54a8
    unsigned char m_artifactCast[2];  // +0x54b0
    // Passed as the final, byte-wide SetMorale input for every stack
    // controlled by the indexed side. Its meaning and public name are
    // not attested by the available symbols. SetupCombat (0x4639f0)
    // sets it from hasGivenArtifact(0x81), Angelic Alliance; this
    // role-derived name records the exact artifact test.
    unsigned char m_hasAngelicAlliance[2];  // +0x54b2
    // Per-side "this side's hero has already cast this round" latch:
    // DoCommand (0x476bd0) refuses to open the spell book, and shows
    // genrltxt entry 129 instead, while the acting side's word is set
    // and field_13d74 is clear. Name is an address ordinal - no roster
    // or string reaches the pair.
    int m_spellsCast[2];  // +0x54b4
    // Live stack count per side; every ai_tactical walk of armies[side]
    // bounds itself with it (type_AI_spellcaster ctor 0x4369c0,
    // set_melee_enemies 0x43bf20).
    int m_numArmies[2];  // +0x54bc
    // The two persistent army groups combat was initialized from.
    // RaiseSkeletons indexes this pair by side and tries to merge the
    // post-combat raised stack into the selected group.
    armyGroup* m_armyGroups[2];  // +0x54c4
    army m_armies[2][21];  // +0x54cc
    // Per-side dword SetupCombat initialises to 30000 (0x7530) for both
    // sides, in the same loop as playerIds and field_54b0 - retail reaches
    // it as `[&playerIds[side] + 0xddf8]`, which is what proves the stride
    // and the pair. The magnitude reads like a millisecond budget but no
    // reader is decoded, so the name stays an address ordinal.
    // GATED, AND THAT IS A MEASUREMENT - the fourth firing of the
    // include-set class from this header, and the first one that says
    // WHICH edit shape fires it. Bisected 2026-08-20 against command.obj's
    // GetCommand (92.5714 is that canary's ceiling), one edit at a time:
    //   * slicing one pad declarator into three (this row), or adding any
    //     further member declarator to the class          -> 92.5357;
    //   * RETYPING a pad in place at the SAME declarator count,
    //     `char pad_54a6[2]` -> `unsigned char sideIsLocalHuman[2]`,
    //                                                     -> 92.5714,
    //     unmoved. Three such renames landed un-gated and cost nothing.
    // So the trigger is the DECLARATOR COUNT C1XX numbers member handles
    // from - not the member's name, its type, or the bytes it covers.
    // The per-side pair InitNonVisualVars' closing walk raises when a
    // side owns at least one stack with army::field_4d8 set. Byte-wide
    // and indexed by side.
    // Original Dreamcast OnNativeTerrain (+0x12970): retail
    // InitNonVisualVars scans each side for an army on native terrain;
    // quicksand and landmine creation use it to set enemy visibility.
    unsigned char m_onNativeTerrain[2];  // +0x1329c
    // The native-terrain flags are two bytes at +0x1329c; the next
    // per-side integer array starts at +0x132a0. These bytes align it.
    char m_paddingBeforeTurnSinceLastEnchanter[0x2];
    int m_turnSinceLastEnchanter[2];  // +0x132a0
    // Two dwords InitNonVisualVars sets to -1.

    // RETYPED AS A PER-SIDE PAIR 2026-08-20 by combatManager::
    // SummonElemental (0x5a7080), which stamps the summoned creature's
    // type through `[this + 4*currentSide + 0x132a8]` - so the two
    // dwords are one row indexed by SIDE, exactly like the six other
    // per-side pairs this class already carries, and not two unrelated
    // ordinals. Dreamcast supplies the matching array name below.
    // Original Dreamcast SummonedElemental (+0x12974): retail
    // SummonElemental (0x5a7080) writes the creature type, and the
    // spell eligibility check rejects a different elemental type.
    int m_summonedElemental[2];  // +0x132a8 .. +0x132af
    // Two per-side "this side has already lost / fled" latches, byte
    // proven by CombatIsOver (0x465830) and IsWinner (0x4658b0): both
    // index them by SIDE as bytes (`byte [this + side + 0x132b2]`,
    // `byte [this + side + 0x132b0]`) and answer "combat over" / "this
    // side won" the moment either reads non-zero. Retail tests the
    // 0x132b2 pair FIRST in both functions. HandleCombatPlayerDrop writes
    // field_132b0[1] when the remote combat player disappears. The retreat
    // and surrender writers distinguish the pair names below.
    // Original Dreamcast SideRetreated (+0x1297c): retail
    // DoCommand sets this pair in the retreat action, also used on peer drop.
    unsigned char m_sideRetreated[2];  // +0x132b0
    // Original Dreamcast SideSurrendered (+0x1297e): retail
    // DoCommand sets this pair in the surrender action before transferring gold.
    unsigned char m_sideSurrendered[2];  // +0x132b2
    // Read as a full dword by is_computer_action (0x474bf0) and paired
    // there with soundmgr's byte gbUnk691209: when that byte is set and
    // this slot is non-zero the acting stack is computer-driven no
    // matter whose side it is on, whatever the per-machine options say.
    // Original Dreamcast gbThisNetHasControl (+0x12980): retail
    // command dispatch derives this dword from local/network control and
    // gates locally issued actions on it.
    int m_thisNetHasControl;  // +0x132b4
    // The stack whose turn it is, as a (side, slot) pair into armies:
    // should_attack_now (0x436c60) forms armies[actingSide][actingSlot]
    // with the flattened index actingSide*21 + actingSlot and then
    // excludes that stack from both of its censuses. Names provisional.
    int m_actingSide;  // +0x132b8
    int m_actingSlot;  // +0x132bc
    // The side whose stack is acting: get_hex_attack_value (0x436180)
    // rejects a neighbour whose combatSide equals it. Name provisional.
    int m_currentSide;  // +0x132c0
    // The automation-preference gate. is_computer_action (0x474bf0)
    // reads this dword before every one of the four preference fields it
    // consults in gUnnamed698758 (combatCatapult for the catapult,
    // combatBallista for the ballista and the arrow tower,
    // combatFirstAidTent for the tent, combatAutoCreatures for every
    // other stack) and only honours the preference while it is non-zero.
    // Name is an address ordinal.
    int m_autoCombatOn;  // +0x132c4
    // The stack that most recently finished a move. army::simple_move
    // (0x445950) clears it before it moves anything and stores `this`
    // into it on EVERY exit afterwards - the successful fly, the
    // successful teleport, the walk and both ValidFlight refusals -
    // and DoCompAI (0x4221f0) zeroes it as the very first thing it
    // does, before it even turns the highlighter off. An `army*`, not
    // a counter: simple_move's store is `mov [ecx+0x132c8], esi` with
    // esi holding `this`. Name is provisional; no roster reaches the
    // slot (the Dreamcast dump carries no combatManager fieldlist).
    army* m_lastMovedArmy;  // +0x132c8
    // Dreamcast TurnOffHighlighter (dc 0x6ed18) uses highlighterOn /
    // highlighterIndex at +0x1299c/+0x129a0; retail 0x477e10 uses this
    // byte/int pair at +0x132cc/+0x132d0. The earlier reference mapping
    // called them selectorOn/selectorIndex and shifted the following names.
    unsigned char m_highlighterOn;  // +0x132cc
    char m_paddingBeforeHighlighterIndex[3];
    int m_highlighterIndex;  // +0x132d0
    // Command-loop consumers prove the hovered cell, chosen attack hex,
    // last displayed command and current command respectively. Dreamcast
    // names the same four consecutive ints at +0x129a4..+0x129b0.
    int m_lastCellIndex;  // +0x132d4
    int m_lastMoveToIndex;  // +0x132d8
    int m_lastCommand;  // +0x132dc
    int m_combatCommand;  // +0x132e0
    // InitNonVisualVars clears Dreamcast CastleAttackDone at +0x129b4
    // (dc 0x5e6c2, cmbtmgr.cpp:1358) and retail +0x132e4 (0x463c91)
    // at the corresponding reset. Both stores and the DC member are bytes.
    unsigned char m_castleAttackDone;  // +0x132e4
    // InitNonVisualVars clears the preceding slot as a byte at +0x132e4.
    // LoadSpellEffect proves the pointer at +0x132e8; these bytes align it.
    char m_paddingBeforePowSprite[0x3];
    // LoadSpellEffect (0x5a92f0) disposes/refills the sprite and caches
    // the effect ID; PowEffect (0x468990) queries its frame count and
    // advances the frame index, proving this three-member sequence.
    CSprite* m_powSprite;  // +0x132e8
    int m_powSpellEffect;  // +0x132ec
    int m_powFrameIndex;  // +0x132f0
    // "This combat is fought over a walled town": HexIsBlocked
    // (0x469a10) only consults the gate hexes while it is positive and
    // should_lower_door (0x467130) while it is non-zero. Name pending
    // a writer.
    ECombatFortification m_fortificationLevel;  // +0x132f4
    // Cleared by InitNonVisualVars at full width. Ordinal.
    int m_battleOver;  // +0x132f8
    TCombatWindow* m_combatWindow;  // +0x132fc
    int m_combatShowIt;  // +0x13300
    // Replaces synthetic pad_13304. Dreamcast combatManager type 0x66c7
    // names iconWidgetWL/textWidgetWL, each an array of 25 widget pointers.
    // NH3API confirms PC offsets +0x13304/+0x13368; retail mainWindow /
    // combatDirections accesses bound the same 200-byte interval.
    iconWidget* m_iconWidgetWL[25];  // +0x13304
    textWidget* m_textWidgetWL[25];  // +0x13368
    // SetCombatDirections fills twelve direction/hex pairs here. Retail
    // addresses the second row exactly 0x30 bytes after the first.
    int m_combatDirections[2][12];  // +0x133cc
    // CheckSetMouseDirection caches the current combat cursor frame here and
    // only calls mouseManager::SetPointer when the selected direction maps
    // to a different frame. No roster names the storage, so it stays ordinal.
    int m_lastAttackCursor;  // +0x1342c
    // Replaces synthetic pad_13430. Dreamcast originals iTtlCombatDirections
    // and iBackgroundFrame are int; NH3API confirms +0x13430/+0x13434.
    // Retail lastAttackCursor and creatureIsDead bound this eight-byte span.
    int m_ttlCombatDirections;  // +0x13430
    int m_backgroundFrame;  // +0x13434
    // Per-army "this stack is leaving the field" latch, twenty bytes per
    // side, tested by BOTH of MakeCreaturesVanish's walks - the first to
    // raise the matching +0x14000 effect byte (or an arrow-tower latch),
    // the second to clear the stack's hexcell occupancy. Twenty per side
    // against armies[2][21] is retail's own asymmetry, not a mis-slice:
    // the walk steps the byte arrays by 20 and the army index by 21 in
    // the same loop, and ResetLimitCreature memsets exactly 2x20 at
    // +0x14000.
    unsigned char m_creatureIsDead[2][20];  // +0x13438
    // Sliced in place off PowEffect, which zeroes it beside field_13438
    // and then asks it, after the death sweep, whether MakeCreaturesVanish
    // needs running. A retype, not a new declarator.

    unsigned char m_someCreaturesVanish;  // +0x13460
    // Dreamcast bSomeCreaturesVanish is one byte followed by cBkgName;
    // retail retains the three-byte pointer-alignment gap at +0x13461.
    char m_paddingBeforeBackgroundName[0x3];
    // The battlefield background image name. SetupCombat caches
    // GetBackgroundName()'s answer here as its very last act; the pointer
    // is into the .rdata name tables that method selects from, so it is
    // never owned or freed.
    const char* m_backgroundName;  // +0x13464
    // Adjacency table [cell][direction] of int16 cell indexes (-1 =
    // off-grid); path.cpp's whole direction system reads it. Slots
    // 6/7 are resolved to real directions by facing first.
    short m_adjacentCells[187][6];      // +0x13468
    bool m_saveBiggestExtent;        // +0x13d2c
    // Dreamcast SaveBiggestExtent is one byte before the LimitToExtent
    // dword; retail preserves this alignment boundary at +0x13d2c/30.
    char m_paddingBeforeLimitToExtent[0x3];
    int m_limitToExtent;  // +0x13d30
    int m_computeExtentOnly;  // +0x13d34
    // Four-dword drawing bounds copied from 0x694f30..0x694f3c before
    // every drawbridge animation. The role of each coordinate awaits a
    // decoded drawing reader, so the member stays an ordinal array.
    TDrawbridgeBounds m_drawbridgeBounds;  // +0x13d38
    // Set to 3 by InitNonVisualVars, out of the same register as
    // field_5418. Ordinal.
    int m_winner;  // +0x13d48
    // Pending post-combat raised stack. RaiseSkeletons first attempts
    // this pair unchanged, then promotes the creature and converts the
    // count at a two-for-three ratio if the destination group is full.
    int m_raisedCreatureCount;  // +0x13d4c
    TCreatureType m_raisedCreatureType;  // +0x13d50
    Bitmap816* m_combatGridBitmap;  // +0x13d54
    // The placed-obstacle array, as the raw first/last pair retail
    // tests: RemoveObstacle (0x466b30) null-checks the FIRST pointer,
    // then divides last-first by sizeof(TObstacle) for the bound. The
    // DC roster's std::vector<combatManager::TObstacle> COMDATs say
    // this really is a vector; only its first two members are proven.
    // Complete uses Dinkumware: allocator at +0,
    // pointers at +4/+8/+0xc; TObstacle remains a 0x18-byte value.
    std::vector<TObstacle> m_obstacles;  // +0x13d58
    // Placement-phase latch: FindPath/ValidPath forward it into
    // FindCombatPath's in_placement_phase and lift the speed limit
    // to 99 while it is set. Name provisional.
    unsigned char m_creaturePlacement;  // +0x13d68
    // Dreamcast InPlacementPhase is a byte followed by turn_number;
    // retail preserves the three-byte integer-alignment gap at +0x13d69.
    char m_paddingBeforeTurnNumber[0x3];
    // Cleared by Open just before the acting pair is stamped. Ordinal.
    int m_turnNumber;  // +0x13d6c
    // Placement inset measured in combat-grid columns. command.obj's
    // is_outside_placement_boundry reads it at +0x13d70 and forms the
    // two side limits as 2*n+1 and 2*n+15 respectively.
    int m_placementBoundaryDepth;  // +0x13d70
    // Second half of DoCommand's spell-book gate: the dialog only fires
    // while field_54b4[currentSide] is set AND this byte is clear, so it
    // reads as a "casting restriction lifted" latch. Name is an address
    // ordinal - nothing else decoded reaches it.
    unsigned char m_debugNoSpellLimit;  // +0x13d74
    // Its two neighbours, cleared with it in one run by
    // InitNonVisualVars. Ordinals.
    unsigned char m_debugShowHiddenObjects;  // +0x13d75
    unsigned char m_debugShowBlockedHexes;  // +0x13d76
    // The three debug bytes end at +0x13d77; the retail archer records
    // start at +0x13d78. This byte aligns the records, as in Dreamcast.
    char m_paddingBeforeArchers[0x1];

private:
    TArcher m_archers[3];  // +0x13d78; armySlot at +0x20

public:
    // "Move order is reversed for this combat": find_move_order
    // (0x41f179) reads it through the gpCombatManager GLOBAL - not
    // through its own `this` - and, when it is set, keys every stack
    // that is not already flagged with the NEGATED speed, which turns
    // the descending sort into an ascending one. move_toward (0x41f580)
    // is the second reader. Byte width is proven by the `mov al, [ecx +
    // 0x13de4] / test al, al` pair. Name provisional.
    // Army-slot indexes for the main building, lower tower and upper
    // tower defenders. DamageWall reads one selected slot when the
    // corresponding target falls and marks that stack removed.
    unsigned char m_inSecondPhase;  // +0x13de4
    // Dreamcast in_second_phase is a byte followed by OriginalAttackSkill;
    // retail preserves this three-byte alignment gap before the stat snapshot.
    char m_paddingBeforeOriginalAttackSkill[0x3];
    // The DEFENDING hero's combat snapshot, taken by InitNonVisualVars
    // before the town's own bonuses are applied: stats[0] and stats[1]
    // clamped to [0, 99], stats[2] clamped to [1, 99] - spell power is
    // never zero - and the mana word widened. All four are zeroed when
    // there is no defending hero.
    int m_originalAttackSkill;  // +0x13de8
    int m_originalDefenseSkill;  // +0x13dec
    int m_originalPowerSkill;  // +0x13df0
    int m_originalMana;  // +0x13df4
    Bitmap816* m_combatIcons[18][5];  // +0x13df8
    // Per-wall-segment hit points, indexed by TWallTargetId. Sliced
    // 2026-08-08 by should_stay_in_castle (0x4213f0), which reads
    // `[this + 4*id + 0x13f60]` for each of the five wall ids and
    // treats a ZERO as "this segment is down". Fifteen entries because
    // wallTargets' own `wall` column runs 5..14 and the largest id the
    // located reader passes is 12; only 6, 8, 9, 10 and 12 are
    // retail-proven. The DC roster attests the accessor
    // (combatManager::get_wall_strength, cmbtmgr.h:1473, dc 0x27edc),
    // not the field, so the name is the accessor's.
    // EIGHTEEN, not fifteen plus a separate three (merged 2026-08-20).
    // SetupAndLoadObstacles (0x466290) settles the extent outright: it
    // fills this band with ONE 18-iteration loop out of
    // akWallTraits[defendingTown->type][i].hitpoints, then stores 1
    // into slots 15, 16 and 17, then copies all eighteen into
    // wallStanding with a second 18-iteration loop whose source is
    // exactly dst - 0x48. A 15+3 split cannot express either loop.
    // The three tail slots are the two arrow towers and the keep - what
    // DamageWall's targets 7, 6 and 0 clear - so the old field_13f9c /
    // field_13fe4 rows were the same array seen through their one
    // decoded writer, and DamageWall now spells them wallStrength[17]
    // / [16] / [15] at byte-identical offsets.
    int m_wallStrength[18];  // +0x13f60
    // One dword per wall id (5..14 used): 1 while strength remains, 0 when
    // the segment reaches zero. Same 18 extent, same reason.
    int m_wallStanding[18];  // +0x13fa8

private:
    // The battle's packed adventure-map coordinate. GetBackgroundName
    // passes it by value to advManager::MoreTreesNear.
    type_point m_mapPoint;  // +0x13ff0

public:
    Bitmap816* m_combatCellGridBitmap;  // +0x13ff4
    Bitmap816* m_combatShadowBitmap;  // +0x13ff8
    int m_obstacleAnimationFrame;  // +0x13ffc
    // "This slot's stack was added mid-combat and still owes a fizzle-in
    // frame": AddArmy (0x47a100) stamps [iSide][slot] with the flattened
    // index 20*iSide + slot for every stack that is NOT an arrow tower.
    // Twenty slots a side, not the twenty-one `armies` carries - AddArmy
    // only ever searches 0..19. Name is an address ordinal.
    unsigned char m_creatureEffect[2][20];  // +0x14000
    unsigned char m_heroEffect[2];  // +0x14028
    unsigned char m_flagEffect[2];  // +0x1402a
    // The three arrow-tower latches, keyed by the tower's grid index by
    // 0x46a460: hex 254 -> +0x1402c, hex 251 -> +0x1402d, hex 255 ->
    // +0x1402e.

    unsigned char m_archerEffect[3];  // +0x1402c
    // Original Dreamcast auto_retreat_on (+0x136f7). Retail SetupCombat
    // enables this byte at +0x1402f; command processing asks whether to
    // retreat and records the answer here. Earlier any_action_taken
    // correspondence was shifted by one byte.
    unsigned char m_autoRetreatOn;  // +0x1402f
    unsigned char m_anyActionTaken;  // +0x14030
    unsigned char m_obstacleAttackVisited[COMBAT_GRID_CELLS];  // +0x14031

    combatManager();
    unsigned char isWinner(int thisSide) const;
    unsigned char combatIsOver() const;
    void resetHitByCreature();
    // DC LF_MFUNCTION records have no this type: these are static helpers.
    static TWallTargetId getTargetWallIndex(int gridIndex);
    static unsigned char inCastle(int index);
    static unsigned char leftOfMoat(int index);
    static void getMissileStartingPosition(int armyType, int x, int y, int facing,
                                          int destX, int destY,
                                          const CSprite* missile, int* startX,
                                          int* startY, int* armyDir,
                                          int* missileFrame);
    void damageWall(TWallTargetId targetWall, int damage);
    void highlightHex(int hex);
    void highlightHex(int x, int y);
    int validAttackHex(int hex);
    void getHexXY(int hex, int& x, int& y);
    void setCombatViewArmy(int newCombatViewArmy);
    int getGridIndex(int x, int y) const;
    // The SGTWDEF explosion frame army::attack_wall (0x445fd0) lets
    // play before it lands the DamageWall - the wall visibly breaks
    // mid-animation, not on the last frame.
    enum { WALL_EXPLOSION_HIT_FRAME = 0x5 };
    unsigned char enemyIsAdjacent(const army* currentArmy, int gridIndex,
                                    const army* excluded) const;
    unsigned char isAdjacent(int first, int second) const;
    void viewArmy(army* thisArmy, int isQuickView);
    void removeArmyFromGrid(const army& a);
    void placeArmyInGrid(const army& a, int hex);
    // Retail 0x59ec50 extends Dreamcast's one-argument spells.cpp:176
    // routine with the creature-cast selector passed by command.cpp.
    void initiateSpell(SpellID spellToCast, int creatureSpell);
    unsigned char placeObstacle(int obstacleId);
    void markMovingArmy(army* stack);  // 0x46a520
    unsigned char checkObstacleAttacks(army* thisArmy,
                                         unsigned char isWalking);
    void lootDeadHero(int side,
                      std::vector<type_artifact>& lootedArtifacts);
    void calculateGainedExperience(int side, int* experienceGained);
    unsigned char checkFireWall(long hex, army* currentArmy,
                                  unsigned char isWalking);
    unsigned char checkLandmine(long hex, army* currentArmy,
                                 unsigned char isWalking);
    unsigned char shouldLowerDoor(army* thisArmy, long hex) const;
    int experienceValueOfStack(int whichGroup);
    void makeCreaturesVanish();
    void raiseDoor();
    void testRaiseDoor();
    void lowerDoor();
    bool isQuickCombat() const;
    // ?show_eagle_eye@combatManager@@AAAXHH@Z (two ints),
    // ?DoVictory@combatManager@@QAAXH@Z (one int) and
    // ?show_looted_artifacts@combatManager@@AAAXAAV?$vector@Utype_artifact@@...@Z,
    void freeArmies();
    void close();
    void initializeArchers();
    void updateArmyLuckAndMorale();
    void doVictory(int winningGroup);
    int checkApplyBadMorale(int group, int index);
    void checkApplyGoodMorale(int group, int index);
    void spellEffect(int effect, army* targetArmy, int delay,
                     bool doWince);
    unsigned char shotIsNotOptimal(const army* attacker,
                                   const army* defender) const;
    unsigned char shotIsThroughWall(const army* shooter, int sourceIndex,
                                    int destIndex) const;
    unsigned char inLineOfSight(int sourceIndex, int destIndex) const;
    unsigned char hexIsBlocked(int index) const;
    unsigned char isInMoat(int hex, int* index);

private:
    void loadIcons();
    void freeIcons();
    unsigned char doorCanBeLowered() const;

public:
    // Complete-only moat damage worker; its sole caller passes the entered
    // hex, moving stack and a byte sound-control flag.
    unsigned char unnamed469e50(int hex, army* stack,
                                unsigned char playSound);
    int open(int newPriority);
    void initNonVisualVars();
    void setupAndLoadObstacles();
    void determineCombatTerrain();
    const char* getBackgroundName();
    void generateMap();
    void combatSystemOptions();
    void placeObstacle(const TObstacle* obstacle, int id, int hex,
                       unsigned attributes);
    void removeObstacle(int index);
    void placeAllObstacles();

private:
    void setupAdjacencyArray();
    void updateArmyGroup(int whichSide);

public:
    int placeLargeObstacle(unsigned terrainMask,
                           unsigned specialTerrainMask);
    static unsigned char loadWallTraitsTable();
    void raiseSkeletons(int side);
    void learnSpellFromEagleEye(int side);
    void resetLimitCreature();
    void setupGridForArmy(const army* thisArmy);
    int updateGrid(int postGridIsClean, int setupGrid);
    void drawBackground();
    void updateCombatArea();  // 0x493780
    unsigned char handleCombatPlayerDrop(unsigned long dpid, message* msg);

private:
    unsigned char isComputerAction();

public:
    void updateMouseGrid(int gridIndex, std::vector<long>& hexes,
                         unsigned char forceUpdate);
    void updateMouseGrid(int gridIndex, int allowDuringAction);
    // Preserve DC UpdateCombatArea's by-value extent and coordinate facade.
    // The Windows fixed-viewport definitions and their platform evidence are
    // below. The coordinate ScrollTo facade remains ordinary in drawing.cpp.
    void updateCombatArea(SLimitData area);
    void updateCombatArea(int x, int y, int width, int height);
    bool scrollTo(int x, int y, int width, int height, bool draw,
                  bool doscrollX, bool doscrollY);
    bool scrollTo(SLimitData extent, bool draw,
                  bool doscrollX, bool doscrollY);
    // Dreamcast's LF_FIELDLIST fixes this complete renderer band (entries
    // 197..212). Keep even the helpers which Complete inlines away: their
    // declaration order and source boundaries are compiler-state evidence.
    void drawFrame(bool update,
                   bool limitCreatureEffect,
                   bool limitDraw, int delay,
                   bool refreshBackground,
                   bool doDelayTil);
    int drawArcher(const CSprite* sprite, int sequence, int frame,
                   int x, int y, SLimitData* limits,
                   bool isFlipped, unsigned char colorRow);
    // DC header inline (cmbtmgr.h:1460, dc 0x27ec8, 18 B). Its S_PUB32
    // identity is ?ValidHex@combatManager@@SA_NH@Z: static bool. No retail
    // body; place_shooter (0x422060) carries two copies of it, one on
    // the loop index (which VC6 strength-reduces onto the same 30-byte
    // induction variable the cellData walk uses, so it reads as a
    // `test/jl` plus `cmp 0x15ea/jge` pair) and one on the adjacent hex.
    static bool validHex(int hex)
    {
        return hex >= 0 && hex < COMBAT_GRID_CELLS;
    }
    // command.cpp:224 (0x474040) paces the frame loop and hands each frame
    // to drawing.cpp's CycleCombatScreen (0x4960d0).
    void doAnimations();
    void shootMissile(int startX, int startY, int destX, int destY,
                      const float* angles, const CSprite* missile);
    void shootAnimatedMissile(int startX, int startY, int destX, int destY,
                              int nsprites, const float* angles,
                              const char* const* fileNames);
    // The three missile animators. Every pointer parameter's constness
    // is read off the DC S_PUB32 mangling rather than guessed:
    // ?ShootMissile@combatManager@@QAAXHHHHPBMPBVCSprite@@@Z gives
    // `const float*` (PBM) and `const CSprite*` (PBVCSprite), and
    // ShootAnimatedMissile's PBQBD gives `const char* const*`.
    void shootBallisticMissile(int startX, int startY, int destX, int destY,
                               const CSprite* missile);
    void cycleCombatScreen();
    int drawCreature(const CSprite* sprite, int sequence, int frame,
                     int x, int y, struct SLimitData* limitData,
                     int id, bool isFlipped, int color);
    int drawCreatureAlpha(const CSprite* sprite, int sequence, int frame,
                          int x, int y, SLimitData* limits,
                          bool isFlipped, int color);
    int drawCombatHero(const CSprite* sprite, int sequence, int frame,
                       int x, int y, SLimitData* limits,
                       bool isFlipped);
    int drawSpriteObject(const CSprite* sprite, int frame, int x, int y,
                         bool isFlipped);
    int drawSpellEffect(const CSprite* sprite, int frame, int x, int y,
                        bool isFlipped, bool isAlpha);
    int drawWall(const Bitmap816* image, int x, int y, int width, int height,
                 int destX, int destY);
    int drawObject(const Bitmap816* image, int x, int y);
    int drawObstacle(const hexcell& cell);
    int drawMoatOverlay(int index);
    void drawOccupant(int index, int drawPriority, int numBoxOnly);
    void drawDeadOccupants(int index);
    void drawWallAt(int hexIndex, int rowOffset);
    void drawObstacleAt(int hexIndex);
    int drawCreatureAndHeroSubwindows();

private:
    void computeExtent(const CSprite* sprite, int sequence, int frame,
                       int x, int y, SLimitData* limits, int isFlipped,
                       bool saveBiggestExtent);

public:
    // Dreamcast's spells.cpp helper. CastSpell calls this boundary while
    // placing Fire Wall segments; retail VC6 expands it into the row-parity
    // ladder, so no standalone Complete body is required by that caller.
    enum ESpellWallRowOffset {
        SPELL_WALL_SECOND_ROW = 2
    };
    int getSpellWallHex(int baseIndex, int rowOffset, int side);
    void checkChangeSelector();  // 0x477ac0
    void checkChangeHighlighter(int currentIndex);  // 0x478040
    void turnOffSelector(unsigned char drawIt);
    void turnOffHighlighter(unsigned char restore);  // 0x477e10
    void setCombatGrid(int showEntireGrid, int showMouseHex, int gridLevel,
                       unsigned char drawNow);  // 0x479fc0
    // 0x46a520 (68 B), army::simple_move's second call: it zeroes a
    // 187-byte per-hex row at this + 0x14031 with a `rep stosd` of 46
    // dwords plus a word plus a byte - the cell count exactly - and
    // then raises the hex, or the two hexes, the passed stack stands
    // on. It sits in cmbtmgr.obj's own bracket (immediately after
    // IsQuickCombat 0x46a4a0) but the DC roster has no row for it
    // there, so both the NAME and the parameter's constness are
    // PROVISIONAL and its claim waits for the lane that reconstructs
    // the body. The row it clears is deliberately NOT modelled here:
    // this declaration alone already costs GetCommand 92.5714 ->
    // 92.5357 unconditionally (include-set class, bisected), so it is
    // scoped to army.cpp and the field waits for the same lane.
    // 0x465ad0 (0x443), already carved and carcassed in cmbtmgr.cpp.
    // army::range_attack (0x440160) short-circuits into it for an ARROW
    // TOWER, passing that stack's indexToAttack as the tower position -
    // a thiscall on gpCombatManager with one stack argument, which is
    // the arity the DC row's single TArcherID parameter says. THE
    // PARAMETER TYPE IS THE ONE DIVERGENCE: the DC prototype takes
    // combatManager::TArcherID, an enum this header does not model yet,
    // so the declaration takes the int retail actually pushes and the
    // enum waits for the lane that reconstructs the body.
    // Complete adds the arrow-tower early return in army::rangeAttack
    // (0x440160), directly calling KeepAttack (0x465ad0). DC lacks that
    // arm but records KeepAttack private. This narrow friendship is a
    // retail-supported source hypothesis: bytes do not distinguish it from
    // an unrecorded inline wrapper, and no original friend syntax survives.
    friend void army::rangeAttack();

private:
    // 0x465ad0 (0x443), already carved and carcassed in cmbtmgr.cpp.
    // army::range_attack (0x440160) short-circuits into it for an ARROW
    // TOWER, passing that stack's indexToAttack as the tower position -
    // a thiscall on gpCombatManager with one stack argument, which is
    // the arity the DC row's single TArcherID parameter says. THE
    // PARAMETER TYPE IS THE ONE DIVERGENCE: the DC prototype takes
    // combatManager::TArcherID, an enum this header does not model yet,
    // so the declaration takes the int retail actually pushes and the
    // enum waits for the lane that reconstructs the body.
    void keepAttack(int towerPos);  // 0x465ad0

public:
    void unnamed465f20();  // 0x465f20
    // The arrow tower's target selector, at the HEAD of ai.obj rather
    // than in cmbtmgr.obj: 0x41e190 is the first ai.cpp body after that
    // compiland's ten terrain.h bitset initializers, and the DC roster,
    // the NH3API IDB (0x41e310 in the HD pressing, a constant +0x180
    // ahead of retail through this region) and the body's own
    // find_AI_targets / get_loss_combat_value calls all name it
    // ChooseBallistaTarget with these three parameters.
    int chooseBallistaTarget(int targetGroup, int attackSkill,
                             int averageDamage);
    void unnamed4693a0(int side);  // 0x4693a0
    void checkRebirth();  // 0x469440
    unsigned char canCastSpells(long side,
                                  unsigned char heroSpell) const;  // 0x41f890
    long computeFireShieldDamage(long damage, const army* attacker,
                                    const army* target,
                                    long targetHits) const;
    // (?damage_message@combatManager@@QAAXPBDJJPBVarmy@@J@Z) fixes both
    void damageMessage(const char* attacker, long attackerQty,
                        long damage, const army* defender,
                        long deaths);  // 0x469a90
    void findMoveOrder(std::vector<army*>* result);
    long getTotalCombatValue(long side, long lowestAttack,
                                long lowestDefense,
                                unsigned char includeCripples) const;
    CSprite* loadSpellEffect(int effect);  // 0x5a92f0

private:
    unsigned char chooseToRun(const army* ourArmy,
                                const long* enemyAttacks,
                                const searchArray* currentSearchArray);  // 0x4208f0
    long getAttackChange(const army* currentArmy, const army* enemy,
                           type_AI_combat_parameters& data);  // 0x41f3b0
    void markFirewalls(const army* currentArmy, long* enemyAttacks,
                        type_AI_combat_parameters* estimate);  // 0x4214f0
    unsigned char moveToward(const army* currentArmy, long targetHex,
                              const long* enemyAttacks,
                              unsigned char considerWaiting);  // 0x41f580

public:
    void markMoat(const army* currentArmy, long* enemyAttacks,
                   type_AI_combat_parameters* estimate);  // 0x421590
    unsigned char chooseCyclopsAction(long bestValue, long side,
                                        type_AI_combat_parameters* estimate);  // 0x41eea0

private:
    unsigned char chooseCreatureSpell(const army* currentArmy,
                                        long* bestValue,
                                        type_AI_combat_parameters* estimate);  // 0x420d20
    unsigned char chooseMeleeTarget(const army* currentArmy,
                                      unsigned char teleport,
                                      long* actionValue,
                                      type_AI_combat_parameters* estimate);  // 0x421680
    unsigned char chooseResurrectAction(
        const army* currentArmy, long* bestValue,
        type_AI_combat_parameters* estimate);  // 0x421000
    long chooseShooterTarget(const army* currentArmy,
                               type_AI_combat_parameters* data,
                               long* bestValue) const;  // 0x41eb80
    unsigned char chooseSpellAction(const army* currentArmy,
                                      long* bestValue,
                                      type_AI_combat_parameters* estimate);
    long getAreaEffect(long side, const army* ourArmy,
                         long markedEnemies,
                         const type_AI_combat_parameters* estimate) const;  // 0x41f920
    // Non-const `estimate` for the same reason mark_multiheaded_enemy
    // below is: this body calls that one and reaches
    // get_simple_attack_effect itself.
    void markEnemyAttacks(const army* ourArmy, long* enemyAttacks,
                            long* dangerousEnemies,
                            type_AI_combat_parameters* estimate) const;  // 0x420260
    void markFriendlyArmies(const army* ourArmy, long* enemyAttacks,
                              long markedEnemies,
                              const type_AI_combat_parameters* estimate) const;  // 0x41fb60
    // `estimate` is NON-const where the Dreamcast roster prints
    // `const type_AI_combat_parameters*`, for the same reason the
    // searchArray pair above is: the body reaches
    // type_AI_combat_parameters::get_simple_attack_effect through it and
    // ai_tactical.h declares that method non-const. Spelling only.
    void markMultiheadedEnemy(const army* ourArmy, const army* enemy,
                                long* enemyAttacks, long limitValue,
                                searchArray* currentSearchArray,
                                type_AI_combat_parameters* estimate) const;  // 0x41fd60
    // 0x420f00, the RETAIL-ONLY third spell chooser: same shape as the
    // two above and the third arm of choose_spell_action's switch, the
    // one creatureType 0x86 (Faerie Dragon) takes. The address is fixed
    // independently by that switch; the HD/cross-build symbol can then
    // supply the otherwise unattested private bool/reference declarator.
    bool sodChooseFaerieDragonSpell(
        const army* currentArmy, long& bestValue,
        type_AI_combat_parameters& estimate);  // 0x420f00

public:
    void berserkAttack(army* currentArmy, const army* target);  // 0x4222c0
    long chooseMeleeAction(const army* currentArmy, unsigned char teleport,
                             unsigned char simulated, long side);  // 0x421f80
    unsigned char failedSiege();  // 0x41e440
    // 0x422b20 (632 B), NOT YET CLAIMED and NOT in any TU's carve span
    // here - `homm3 sema rva` files it under seg_0002. The DC roster
    // puts combatManager::find_AI_targets in ai.obj (ai.cpp:2608, dc
    // 0x27888) with exactly these five parameters, and the DC xref
    // graph lists it as a callee of choose_shooter_action, which is
    // where 0x422b20 is called from on the retail side too - along
    // with choose_cyclops_action, choose_melee_action and two
    // ai_tactical bodies, every one of which the DC graph also records
    // as a find_AI_targets caller. Its own VA claim wants the ai.obj
    // span recomputed past 0x4224d9 and is left to that lane.
    // CodeView ai.cpp:2608 (dc 0x27888) declares a const parameters
    // receiver. Its calculations are const too; the old non-const claim
    // came from our reconstructed declaration, not the retail bytes.
    void findAITargets(long ourGroup, const army* currentArmy,
                         unsigned char meleeOnly,
                         const type_AI_combat_parameters* data,
                         searchArray* currentSearchArray);
    unsigned char isValidTeleport(const army* thisArmy, long newHex);
    void simulateCombat(long side, unsigned char simulated);  // 0x422a40
    unsigned char validWallTarget(TWallTargetId wall);  // 0x476440
    void doCompAI(int whichGroup);  // 0x4221f0
    // command.cpp calls this ai.obj leaf from CheckGetAIMove.
    unsigned char doSpellAI();  // 0x422da0
    // command.cpp:3038. The retail call at 0x477f3d occupies the exact
    // AICheckRetreat statement slot in Dreamcast CheckGetAIMove, and the
    // helper's other retail caller sits in ai.obj.
    unsigned char aiCheckRetreat();  // 0x41e570
    void clearEffects();  // 0x5a66b0
    void checkGetAIMove();
    unsigned char ableToSummonElemental(SpellID spell, long side);
    int getNextChainLightningTarget(army* lastTargetArmy,
                                    int useSRandom);  // 0x5a61f0
    // command.obj's leaf (0x4763f0, claimed in src/command.cpp); ai.cpp
    // and findpath.cpp are both located callers and both reach it
    // through gpCombatManager with (army::combatSide, hex).
    unsigned char isOutsidePlacementBoundry(int group, int index);

private:
    unsigned char automateCatapult();  // 0x473c00
    unsigned char attemptShooterDefense(
        const army* currentArmy, searchArray* currentSearchArray,
        const type_AI_combat_parameters* estimate);  // 0x420760
    unsigned char chooseDefenseHex(const army* currentArmy,
                                     const army* client, long* bestHex,
                                     long* openHexes,
                                     searchArray* currentSearchArray);  // 0x4205d0
    void chooseShooterAction(const army* currentArmy,
                               unsigned char simulated, long side);  // 0x41f060
    unsigned char hasRangedAdvantage(
        type_AI_combat_parameters* data);  // 0x420a80
    void placeShooter(const army* currentArmy);  // 0x422060
    unsigned char shouldStayInCastle(
        type_AI_combat_parameters* estimate);
    bool showCreatureSpellError(char* buffer,
                                   const army* currentArmy);
    void showEagleEye(int winningGroup, int dialogTimeout);
    void showLootedArtifacts(std::vector<type_artifact>& lootedArtifacts,
                               int dialogTimeout);
    long simulateActions(std::vector<army*>& list, long i,
                          long ourGroup);
    void simulateMeleeAttack(army* currentArmy, long hex, army* target,
                               long enemyHex, long ourGroup);  // 0x4224e0
    void simulateMeleeAttack(army* currentArmy, army* target,
                               long ourGroup);  // 0x4227a0

public:
    unsigned char isComputerAction(const army* currentArmy);
    // DC publishes void(int,int,int); Complete's x86 body changes the result
    // to an unsigned-byte "pointer changed" flag. Its retail field/call graph
    // fixes the three arguments as mouse x, mouse y and combat hex.
    unsigned char checkSetMouseDirection(int x, int y, int hex);
    // 0x476490 / 0x476bd0, the command.obj pair: GetCommand answers
    // "what would clicking this hex do", DoCommand performs it.
    int getCommand(int newIndex);
    void doCommand(int command);
    int rightClick(int newIndex);  // 0x4769c0
    // 0x59e900, spells.obj. LOCATED 2026-08-13 from DoCommand's
    // spell-book case, which calls it with `this` only, compares the
    // result against -1 and forwards it straight into InitiateSpell.
    // The DC spells.obj roster puts combatManager::ViewSpells
    // (spells.cpp:97, 680 SH4 B, ONE parameter) immediately before
    // InitiateSpell (spells.cpp:176), and retail's 847-byte row at
    // 0x59e900 ends exactly where the already-claimed InitiateSpell
    // begins at 0x59ec50 - the same order with no gap.
    int viewSpells() const;
    // 0x47a100. Claims the first free (or expendable) slot on a side,
    // initialises the stack there and optionally fizzles it in.
    army* addArmy(int side, int monType, int monQty, int gridIndex,
                  int setAttributes, int fizzleItIn);
    void viewCastleBallista(int isQuickInfo);
    void markTowerArmy(const army* tower);
    void demonicResurrection(const army* caster, army* target);
    void removeCorpse(army* corpse);
    void removeCorpse(hexcell* hex, long side, long slot);  // 0x5a7320
    unsigned char hasValidSpellTarget(SpellID spellId, long mastery,
                                      long castingSide,
                                      unsigned char firstTarget,
                                      long creatureSpell);  // 0x5a40d0
    // 0x5a39c0 and its 0x5a40d0 driver. HasValidSpellTarget sweeps the
    // 187-cell grid and answers whether ANY cell passes ValidSpellTarget;
    // the two share every parameter but the cell index, which is what
    // fixes the parameter ORDER of the callee from the caller's push
    // sequence. Both parameter lists are the DC roster's
    // (spells.cpp:2645 / 3078); `mastery` is their TSkillMastery,
    // spelled long here for the same reason mark_area_effect's is - the
    // typedef lives in the header that includes this one.
    unsigned char validSpellTarget(SpellID spellId, long mastery,
                                   long targetIndex, long castingSide,
                                   unsigned char firstTarget,
                                   long creatureSpell);  // 0x5a39c0
    // 0x5a7560, carcass in spells.cpp; declared here because
    // army::cast_spell's Archangel arm calls it (the carcass stub is a
    // good-enough callee - the reloc pairs).
    void resurrect(army* targetArmy, long hitPointsResurrected,
                   unsigned char temporary);
    // Dreamcast spells.cpp:4984. Complete has no separate retail body:
    // VC6 expands this source helper into CastSpell's shared
    // Resurrection/Animate Dead arm.
    inline void resurrect(SpellID spell, int targetHex, int power,
                          int mastery, const hero* castingHero);
    long computeSpellDamage(SpellID spell, long spellPower, long mastery,
                            hero* castingHero, hero* targetHero,
                            const army* target,
                            unsigned char simulated) const;  // 0x5a7890
    long modifySpellDamage(long baseDamage, SpellID spell,
                           const hero* castingHero, const hero* targetHero,
                           const army* target,
                           unsigned char simulated) const;
    long modifySpellDamageForSpells(long damage, SpellID spell,
                                    const army* target) const;  // 0x5a7bb0
    // 0x495bf0, carried by drawing.obj (drawing.cpp:2093) and located
    // there already; declared here because AddArmy calls it.
    void computeMaxExtent();

private:
    long getSurrenderCost();  // 0x477a00

public:
    // The three cells a WALL spell occupies, in the order
    // ValidSpellTarget (0x5a39c0) walks them: the aimed hex, then the
    // one a row above it (with a parity nudge that keeps the wall
    // straight across the half-offset rows), then the one two rows
    // above. Basic and Advanced reach the first two, Expert all three -
    // retail's own count is `(mastery >= 2) + 2`. The domain is named
    // because the walk compares the index against 2 directly.
    enum EWallCell {
        WALL_CELL_ANCHOR = 0,
        WALL_CELL_NEAR = 1,
        WALL_CELL_FAR = 2
    };
    // 0x5a66d0, the mass-spell applier ClearEffects (0x5a66b0) clears
    // `effected` for. It rolls SpellCastWorkChance separately per stack
    // on both sides and records which ones took the spell.
    void setMassSpellInfluence(const hero* castingHero, SpellID spell,
                               long level, long power,
                               long castingSide,
                               long creatureSpell);  // 0x5a66d0
    // 0x5a4970, the spells.obj body every AREA damage spell runs
    // through: play the spell's own sprite effect over the centre hex,
    // collect the stacks the area covers, roll each one separately and
    // damage the ones that take it. The DC prototype (spells.cpp:3324)
    // supplies the four parameter names; `mastery` is spelled long here
    // for the reason mark_area_effect's is.
    void areaEffect(long targetCell, SpellID spellType, long mastery,
                    long power);  // 0x5a4970
    // 0x5a4bc0, the field-wide burn. The DC prototype (spells.cpp:3389)
    // supplies both parameter names; `level` is what indexes the spell's
    // mastery_bonus row, i.e. it is the mastery the cast landed at.
    void armageddon(int level, int power);  // 0x5a4bc0
    // 0x5a6360, the bouncing bolt. The DC prototype (spells.cpp:4255)
    // supplies all three parameter names; `level` indexes both the
    // spell's mastery_bonus row and the per-mastery bounce table.
    void chainLightning(int index, int level, int power);  // 0x5a6360
    void earthquake(int level);
    void mirrorImage(int targetIndex, int level);  // 0x5a6c70
    // The last three spells.obj bodies this header had no declarator for.
    // Every parameter name is the DC prototype's (spells.cpp:4424 / 4705
    // / 5164); the three types the DC roster leaves open are read off the
    // retail bodies:
    //   * ShowMassSpell's first slot is the `effected` row itself - a
    //     [2][20] byte array indexed `[side][slot]` with a 0x14 stride,
    //     which is why it is spelled as a pointer to the row rather than
    //     the DC's bare `[]*`.
    //   * SummonElemental's iMonType goes straight into a 116-byte
    //     akCreatureTypeTraits copy and into AddArmy, so it is the
    //     creature domain.
    //   * Earthquake's `level` indexes akSpellTraits' mastery_bonus row
    //     for the number of wall sections to bring down.
    void showMassSpell(const unsigned char (*effected)[20], int spellEffect,
                       unsigned char showWince);  // 0x5a67c0
    void summonElemental(SpellID spell, TCreatureType monType,
                         int spellPower, int level);  // 0x5a7080
    void resetBoltAngle(SBolt* bolt);  // 0x5a5260
    void drawBolt(SBolt* bolt, int drawLength);  // 0x5a5440
    void addBolt(SBolt* bolt, int sourceX, int sourceY, int destX,
                 int destY, int splitFrequency, int startThickness,
                 int endThickness, int color, int angleDistortMin,
                 int angleDistortMax, int segmentLength,
                 int distortAlways);
    void spellEffect(int effect, int hex, int delay,
                     bool leaveLastFrame);          // 0x496a10
    // 0x59fde0 (68 B), the Enchanter's shot resolution army::
    // animate_missile hands its volley to instead of a missile flight
    // (three stack arguments: the launch point and the target stack).
    // ORDINAL PLACEHOLDER name - no roster row reaches it; the body
    // stays spells.obj's to reconstruct.
    void unnamed59FDE0(int x, int y, army* target);
    void spellTargetMessage(SpellID spellId, int targetIndex,
                            unsigned char firstTarget);  // 0x5a8690
    void doBolt(int handleResets, int sourceX, int sourceY, int destX,
                int destY, int splitFrequency, int maxSplitLength,
                int startThickness, int endThickness, int color,
                int angleDistortMin, int angleDistortMax,
                int segmentLength, int drawsPerSegment,
                int distortAlways, int delay,
                int flashLighten);  // 0x5a5c20
    void displayFailureReason(SpellID spell, const char* msg,
                                long hex);  // 0x5a2c60

private:
    std::string getFailureReason(SpellID spell, const char* msg,
                                   long hex);  // 0x5a2880

public:
    void markAreaEffect(long hex, long radius,
                          unsigned char includeCenter,
                          std::vector<long>& hexes);  // 0x5a4170
    void markBerserkAreaEffect(long hex, long mastery,
                                  std::vector<long>& hexes);  // 0x5a4430
    // DC spells.cpp:3214; expanded into HandleCastWallSpell, no retail
    // body of its own.
    void markWallAreaEffect(long targetHex, TSkillMastery mastery,
                               std::vector<long>& result);
    // CodeView declares the axial helpers static and defines them in
    // spells.cpp:3103/3121/3138 using the Win32 POINT record.
    static tagPOINT hexToPoint(long hex);
    static long pointToHex(tagPOINT point);
    static long getDistance(tagPOINT start, tagPOINT stop);
    // The integer-hex overload belongs to cmbtmgr.cpp (retail 0x469670).
    static long getDistance(long start, long stop);
    unsigned char validSpellTargetArmy(SpellID spellId, int castingSide,
                                       const army* targetArmy,
                                       unsigned char firstTarget,
                                       long creatureSpell) const;  // 0x5a3c80
    void castSpell(SpellID spellId, int targetIndex,
                   int isMonsterSpell, int secondaryIndex,
                   int monsterSkill, long monsterPower);
    float spellCastWorkChance(SpellID spell, long side, const army* target,
                              unsigned char redirected,
                              unsigned char firstTarget,
                              long creatureSpell) const;
    unsigned char spellCastWorks(SpellID spell, long side,
                                 const army* target,
                                 unsigned char redirected,
                                 long creatureSpell) const;  // 0x5a8640
    army* findResurrectionTarget(int armyGroup, int targetIndex,
                                   long creatureSpell);
    army* findAnimateDeadTarget(int armyGroup, int targetIndex);

private:
    // 0x5a3950, the selector in front of the two rows above: it bounds the
    // hex against the 187-cell grid, routes Resurrection and the gated
    // Sacrifice arm to find_resurrection_target and Animate Dead to
    // find_animate_dead_target, and otherwise answers cells[hex].get_army().
    // Declared beside the leaves it calls rather than at the end of the
    // class because this run of spells.obj leaves is already unconditional.
    army* findSpellTarget(SpellID spell, long side, long hex,
                            unsigned char firstTarget,
                            long creatureSpell);  // 0x5a3950

public:
    // WHO cast the spell ShowSpellMessage is about to announce. The DC
    // roster calls the parameter `bIsMonsterSpell`, but retail's body
    // (0x5a8950) is a three-way `dec eax / je` chain, not a bool test,
    // and each value picks a different NOUN for the message's subject:
    //   1 names the acting stack, out of armies[actingSide][actingSlot];
    //   2 names an ARTIFACT, out of akArtifactTraits;
    //   everything else names the casting hero, heroes[currentSide].
    // 1 is corroborated from outside this body - retail's ResetRound
    // calls 0x5a8950 with exactly (1, SPELL_POISON, this) - and 2 by
    // combatManager::SetNextArmy (0x465330), whose two auto-cast
    // artifacts are precisely the two rows the 2-arm special-cases.
    // The parameter keeps its int width and its roster name; the enum
    // exists so the arms can be spelled as what they are.
    enum ESpellCaster {
        SPELL_CASTER_HERO = 0,
        SPELL_CASTER_CREATURE = 1,
        SPELL_CASTER_ARTIFACT = 2
    };
    // 0x468990, cmbtmgr.obj's own. DC cmbtmgr.cpp:4158 spells it
    // PowEffect(TSpellEffectID spellEffect, int bResetLimitCreature);
    // the first parameter is int-wide either way and the enum lives in
    // a header this one does not include.
    void powEffect(int spellEffect, int resetLimitCreature);  // 0x468990
    void showSpellMessage(int isMonsterSpell, SpellID spellId,
                          army* targetArmy);  // 0x5a8950
    // Dreamcast spells.cpp:5041 emits this inline helper separately;
    // Complete VC6 expands its only surviving call into CastSpell's
    // failure path at +0x2159.
    inline void showSpellCastFailure(army* targetArmy, int spellId);
    // The ONE TSpellEffectID this header needs so far. Value from the
    // Dreamcast enum table (NB11 enum records:
    // TSpellEffectID.eSpellEffectFireShield = 11), and retail proves the
    // number at the only site that uses it: army::do_fire_shield
    // (0x4409c0) pushes the literal 11 into PowEffect. Named rather than
    // spelled 11 because this tree keeps its magic-constant floor at
    // zero; the rest of the enum waits for the lane that reconstructs
    // PowEffect's own body.
    enum TSpellEffectID {
        eSpellEffectFireShield = 11,
        // Dreamcast's TSpellEffectID table and CastSpell's Berserk arm
        // both fix the mass-animation row to 35.
        eSpellEffectBerserk = 35,
        // Dreamcast enum table and CastSpell's retail Sacrifice arm agree:
        // effect 51 is the slaying flash over the sacrificed stack.
        eSpellEffectSacrifice_Slay = 51,
        // Dreamcast spells.cpp:1788..1792 names these three effect rows;
        // Complete's CastSpell tail independently pushes 76, 75, and 78
        // for the channel-spew, channel-suck, and resisted-spell flashes.
        eSpellEffectMagicChannel_Suck = 75,
        eSpellEffectMagicChannel_Spew = 76,
        eSpellEffectMagicResistance = 78,
        // Dreamcast TSpellEffectID.eSpellEffectFortune = 18; retail
        // proves the number at army::do_attack (0x441610), which hands
        // it to SpellEffect as the good-luck sparkle over the striking
        // stack.
        eSpellEffectFortune = 18,
        // Retail proves the number at army::new_turn (0x446e30), which
        // hands it to SpellEffect over a regenerating stack.
        eSpellEffectRegeneration = 79,
        // Proven at army::ComputeAttackerDamageBonuses (0x443840):
        // the Dread Knight's death-blow flash over the defender.
        eSpellEffectDeathBlow = 73
    };
    // DC cmbtmgr.h:1466. Retail expands this selector in both sacrifice
    // lookup sites; no standalone body survives.
    army* findResurrectionTarget(SpellID spell, long group, long hex,
                                   unsigned char creatureSpell)
    {
        if (spell == SPELL_ANIMATE_DEAD)
            return findAnimateDeadTarget(group, hex);
        return findResurrectionTarget(group, hex, creatureSpell);
    }
    // DC header inline (cmbtmgr.h:1473, dc 0x27edc, 32 B); SH4 proves the
    // typed target -> wallTargets[target].wall -> wallStrength chain.
    // Retail has no body because /Ob2 folds the same chain into its callers.
    long getWallStrength(TWallTargetId target) const
    {
        return m_wallStrength[s_wallTargets[target].m_wall];
    }
    // DC header inline (cmbtmgr.h:1478, dc 0x27efc); the DC xref graph
    // lists it among DoCompAI's callees and retail carries no
    // out-of-line copy, so it is the /Ob2 inline-away case.
    army* getCurrentArmy() { return &m_armies[m_actingSide][m_actingSlot]; }
    // Original: combatManager::get_current_army; CmbtMgr.h:1483, dc 0x1581b8.
    const army* getCurrentArmy() const
    {
        return &m_armies[m_actingSide][m_actingSlot];
    }
    // E:\gamedcs\CmbtMgr.h:1488. Dreamcast proves the single-expression
    // helper and its four ordered bounds. Complete widens the window to the
    // retail 800x556 combat area; ProcessCombatMsg retains the source call
    // and VC6 expands it into the four retail comparisons.
    // DC dc 0x70a2c has x/y only, no receiver: this is static.
    static unsigned char inCombatArea(int x, int y)
    {
        return x >= 0 && x < 800 && y >= 0 && y < 556;
    }
    // Original: combatManager::is_in_second_phase; CmbtMgr.h:1494, dc 0x27f28.
    unsigned char isInSecondPhase() const { return m_inSecondPhase; }
    // Dreamcast S_PUB32 fixes this entire inline band: GetHexIndex and GridX
    // are static int helpers, RowIsOdd is a const bool member, and
    // InInvisibleColumn is static bool. Their CodeView lines also fix this
    // definition order (1500, 1506, 1519, 1525, 1537, 1542).
    static int getHexIndex(int x, int y)
    {
        return y * COMBAT_GRID_ROW_STRIDE + x;
    }
    bool rowIsOdd(int y) const
    {
        return (y & 1) != 0;
    }
    // LF_MFUNCTION has no this type: this is a static header helper.
    // E:\gamedcs\CmbtMgr.h:1513, dc 0x27f34
    static int gridY(int index) { return index / COMBAT_GRID_ROW_STRIDE; }
    static int gridX(int index)
    {
        return index % COMBAT_GRID_ROW_STRIDE;
    }
    // DC header inline (cmbtmgr.h:1525, dc 0x27f64). mark_teleport's
    // retail expansion retains the ValidHex bounds checks and the two
    // invisible edge columns, 0 and 16 of each 17-cell row.
    static bool inInvisibleColumn(int index)
    {
        if (!validHex(index))
            return false;
        int column = gridX(index);
        return column == 0 || column == COMBAT_GRID_LAST_COLUMN;
    }
    // Returns a REFERENCE on its own public
    // (?GetCell@combatManager@@QAAAAVhexcell@@HH@Z); the roster text
    // renders every reference as a pointer, which is what this
    // declaration read before the S_PUB32 pass.
    hexcell& getCell(int x, int y)
    {
        return m_cells[getHexIndex(x, y)];
    }
    // DC header inline (CmbtMgr.h:1542, dc 0x27fa0). Retail re-loads
    // obstacles_begin here rather than reusing the copy RemoveObstacle's
    // own guards just tested; routing the call through this inline does
    // NOT reproduce that (VC6 CSEs the second load away either way) -
    // it is kept because the DC roster attests the accessor, not as a
    // matching lever.
    TObstacle& getObstacle(int index) { return m_obstacles[index]; }
    void markCreatureEffect(int group, int index)
    {
        if (m_armies[group][index].m_creatureType == army::ARMY_CREATURE_ARROW_TOWER)
            markTowerArmy(&m_armies[group][index]);
        else
            m_creatureEffect[group][index] = 1;
    }
    army* findDemonicResurrectionTarget(int armyGroup, int targetIndex);
    void markAreaEffect(SpellID spell, long hex, long mastery,
                          std::vector<army*>& targets);
    void markAreaEffect(long hex, long radius,
                              unsigned char includeCenter,
                              std::vector<army*>& targets);
    void markBerserkAreaEffect(long hex, long mastery,
                                  std::vector<army*>& targets);
    // DC ?SetupCombat@combatManager@@QAAXUtype_point@@PAVhero@@PAVarmyGroup@@
    // JPAVtown@@12HHH_N@Z - the S_PUB32 run types every parameter. The
    void setupCombat(type_point point, hero* leftHero,
                     armyGroup* leftArmyGroup, long rightPlayer,
                     town* rightTown, hero* rightHero,
                     armyGroup* rightArmyGroup, int x, int y, int seed,
                     unsigned char isSurrounded);
    // DC ?NextArmy@combatManager@@QAA_N_N@Z - returns and takes _N,
    unsigned char nextArmy(unsigned char checkingForBadMorale);
    void setNextArmy(int group, int index);

private:
    // DC ?LoadArmies@combatManager@@AAAX_N@Z - PRIVATE on the Dreamcast
    // (`A` access), which costs nothing here and is recorded rather than
    // acted on: this header keeps one public block.
    void loadArmies(unsigned char isSurrounded);
    void checkNativeTerrain();
    void combineGroups(armyGroup* src, armyGroup* dest);

public:
    static float computeDamageModifier(int attack, int defense);
    unsigned char unnamed464d40(army* selected);
    unsigned char unnamed464f50(const army* incumbent, const army* candidate);
    virtual int main(message& msg);
    int processCombatMsg(message& msg);
    int processNextAction(message& msg, unsigned char automaticTurn);
    void setCombatDirections(int hex);
    // DC command.cpp:2800. Complete likewise expands its sole call, while
    // retaining the helper's source-level surrender-dialog boundary.
    int doSurrender();
    int checkWin(message* msg);
    void resetRound();
    // The named command rearm helper. 0x4782d0 (1461 B, command.obj) is
    // Dreamcast's named GetControl method; SetNextArmy calls it immediately
    // after clearing lastMovedArmy, re-arming the command bar for the new
    // stack. Declared, not claimed: its body is outside this lane.

    void getControl();
    // DC command.cpp:907. Complete has no standalone copy: ProcessCombatMsg
    // carries this two-compare helper expanded at its sole retail site.
    int getPointer(int inCombatCommand, int hexIndex);
    void resetMouse();
    void resetCycleTimers();
    void resetCyclingCreatures();
    // drawing.cpp:326. Dreamcast retains the source member and Complete
    // retains its out-of-line body; this is class structure, not a
    // command-TU declaration view.
    void combatMessage(int command);

private:
    std::string getTowerString(TWallSection wall, long archers,
                                 long skill) const;
    void autoResolveCombat();
    unsigned char automateFirstAidTent();
    void processFirstAid(army* currentArmy);
    unsigned char processMoveThenAttack(message* msg);
};
SIZE(combatManager::TWallTraits, 0x24);

// Retail .bss 0x6993d0 (DC ?gpCombatManager@@3PAVcombatManager@@A).
extern combatManager* g_combatManager;

// Two single-byte .bss flags that are always read as a PAIR, in this
// order, and always to suppress something: CalculateGainedExperience
// (0x46a350) docks 500 experience when either is set, and two more
// cmbtmgr bodies (0x463fc9, 0x46a077) test the same pair. NAMES ARE
// UNATTESTED - no DC global, roster or string reaches either byte, so
// both keep an address-ordinal placeholder rather than a guess at the
// rule they encode. Neither is defined here; cmbtmgr is only a reader.
// Two GameTime stamps Open takes on its way in, both shared with other
// compilands - 0x698998 with advmgr.obj's own Open/Main/ProcessKeyPress
// and command.obj's animation frame pacer, 0x6989b8 with Main and
// CycleCombatScreen. Behind views because a single file-scope extern added
// to this header is its own measured include-set trigger (the field_132a0
// bisection).
// CheckGetAIMove caches the displayed surrender price here. No surviving
// retail or Dreamcast symbol supplies a public spelling, so the name keeps
// its address ordinal.
extern long g_surrenderCost;
extern unsigned char g_combatRetreated;
// Set while the combat action pump is active; process_move_then_attack clears
// it on a win before the ResetMouse path. Definition belongs to drawing.cpp.
extern unsigned char g_combatSurrendered;
DATA(0x006989ec) extern int g_processingCombatAction;

// The combat random seed, .data 0x66d840. SetupCombat parks its iSeed
// parameter here and NOTHING in the image ever reads it back - the reloc
// sweep over the whole image finds exactly one reference to the address,
// that store. Declared extern rather than defined: an unclaimed data
// extern still pairs (the DoDialog precedent) and a definition would add
// a word to a .data section that already reads 100%. The name states what
// its only writer calls the value.

// GATED, AND THAT IS A MEASUREMENT that CORRECTS THE DOCTRINE. The match
// skill's include-set note records "`extern int` ... do NOT move it",
// from the 200-extern probe that cleared the wrong hypothesis. On THIS
// header that is false: adding this one file-scope `extern int`, with
// every other edit of the lane held gated, costs command.obj's GetCommand
// 92.5714 -> 92.5357 by itself, and gating it restores the ceiling. A
// bulk probe of externs added together evidently does not reproduce what
// a single extern added to a header this widely included does.
extern int g_combatSeed;

// THE FOUR COMBAT DEPLOYMENT TABLES, .rdata, and their BOUNDS ARE PROVEN
// BY ADJACENCY rather than assumed: 0x63d0a8 + 2*7*4 = 0x63d0e0,
// + 2*7*4 = 0x63d118, + 7*7*4 = 0x63d1dc, + 7*7*4 = 0x63d2a0, which is
// exactly where gTownCombatBackgrounds already starts. Four tables, no
// slack, and every stride matches the indexing LoadArmies performs.

// LoadArmies uses them as a two-stage lookup: the SLOT tables are indexed
// [numArmies-1][nth stack placed] and yield a position ordinal, which then
// indexes the HEX table [side][ordinal]. The surrounded table skips the
// indirection and gives the hex directly. Which of the two slot tables is
// chosen turns on the defending hero's formation byte, so the pair is the
// game's tight/loose deployment split - but no roster row or string
// reaches any of the four, so the names carry their addresses.
extern const int g_combatDeployHexes[2][7];
extern const int g_combatDeploySurroundedHexes[2][7];
extern const int g_combatDeploySpreadSlots[7][7];
extern const int g_combatDeployGroupedSlots[7][7];

// Source aggregate copied into combatManager+0x13d38 by the constructor,
// LowerDoor and RaiseDoor. The current DATA contract cannot express its
// size, so the stripped target still represents interior relocations as
// separate symbols; source keeps the retail-proven aggregate shape.
extern TDrawbridgeBounds g_drawbridgeBounds;

// The clip rectangle every combat-drawing pass intersects its dirty
// region with before handing it to heroWindowManager::UpdateScreen.
// Sixteen readers image-wide (DrawFrame twice, UpdateCombatArea,
// ComputeMaxExtent, DrawObstacleAt, DrawWallAt, army::animate_missile
// and all three missile animators). It is .bss seeded by the
// initializer at 0x462610, which writes exactly {0, 0, 0x31f, 0x22b} -
// i.e. left 0, top 0, right 799, bottom 555 - so this is the combat
// viewport in screen coordinates, not a hex-space bound. Spelled as
// the same four-int aggregate as the drawbridge bounds because the
// readers take its four dwords one at a time; the reloc addend that
// choice produces is masked (ResetLimitCreature is exact through the
// identical aggregate copy). NAME IS A SOURCE-FACING INVENTION and
// carries its address - no roster row, string or DC global reaches it.
extern TDrawbridgeBounds g_combatDrawLimits;

// Combat-background pointer tables decoded from retail .rdata. The first
// table is indexed by town type, the second by special-terrain mode (slot
// zero is null), and the last by combatTerrain*3 + MoreTreesNear(mapPoint).
// Names are source-facing inventions; their addresses, extents and contents
// are all direct retail data.
extern const char* const g_townCombatBackgrounds[9];          // 0x63d2a0
extern const char* const g_magicTerrainCombatBackgrounds[10]; // 0x63d2c8
extern const char* const g_terrainCombatBackgrounds[9][3];    // 0x63d2f0

// The leading two words of each 20-byte obstacle-catalogue row. They
// are separate declarations because the delinked target relocates each
// referenced address independently; indexing by ten shorts preserves
// the common 20-byte stride.
// One spell-effect row, at .rdata 0x641e08 with a TWELVE-byte stride.
// Two retail bodies fix the layout between them and neither needs the
// other: LoadSpellEffect (0x5a92f0) forms `[12*effect + 0x641e08]` and
// hands +0 to ResourceManager::GetSprite, so +0 is a .def sprite name;
// PowEffect (0x468990) forms the same row and hands +4 to the
// Immersion helper at 0x4b69f0, so +4 is a force-feedback effect name.
// The remaining dword is unread by anything decoded here and stays a
// pad. The Dreamcast roster supplies the TYPE and the array name -
// ?akSpellEffectTraits@@3QBUTSpellEffectTraits@@B, with m_name at 0 -
// but its own record is EIGHT bytes, so the +4 Immersion slot is a
// retail insertion and its name is a bootstrap invention.
// The placement nibble (flags & 0xf) of a spell-effect row: where
// army::DrawToBuffer (0x43e140) anchors the pow sprite against the
// target's hex. BOOTSTRAP INVENTION - the four modes are named from
// that one decoded switch and nothing else attests spellings.
DATA(0x0063c7c8) extern const unsigned short g_obstacleTerrainMasks[];
enum TSpellEffectPlacement {
    SPELL_EFFECT_PLACE_OVERHEAD = 0x0,
    SPELL_EFFECT_PLACE_CENTERED = 0x1,
    SPELL_EFFECT_PLACE_ABOVE = 0x2,
    SPELL_EFFECT_PLACE_FLANK = 0x3,
    // The hex-target SpellEffect overload anchors this mode at the combat
    // cell's field_04/field_06 corner instead of at an army sprite; both
    // Dreamcast 0x8703c and retail 0x496a10 select it with value 4.
    SPELL_EFFECT_PLACE_HEX = 0x4
};

struct TSpellEffectTraits {
public:
    const char* m_name;  // +0x0, the .def sprite
    const char* m_immName;  // +0x4, the Immersion effect
    // +0x8, retyped from pad 2026-08-20: army::DrawToBuffer (0x43e140)
    // reads it as the pow overlay's placement word - bits 0..3 select
    // a TSpellEffectPlacement anchor mode over the stack and bit 8 is
    // the draw-alpha flag it hands to DrawSpellEffect.
    unsigned int m_flags;  // +0x8
};
SIZE(TSpellEffectTraits, 0xc);
extern const TSpellEffectTraits g_spellEffectTraits[];

// The moat's per-town base damage, at .rdata 0x63bd18 and indexed by
// town type: SetupAndLoadObstacles folds [0x63bd20] for the Tower,
// which is 0x63bd18 + 4*TOWN_TOWER. searchArray::set_moat (0x4b3290)
// and mark_firewalls (0x4215e0) read the same table with a live index.
// Name is a BOOTSTRAP INVENTION - no roster attests it.
extern const int g_moatDamage[];

extern const char* g_moatDamageMessages[9];

// The thirty-two hexes two facing boats occupy, at .rdata 0x63d368.
// SetupAndLoadObstacles walks it as a POINTER and ends the walk on the
// literal address 0x63d3e8 - the next symbol in .rdata, which is why
// the delinked reference names the combatManager vtable there - so the
// extent is exactly (0x63d3e8 - 0x63d368) / 4 == 32. Name is a
// BOOTSTRAP INVENTION.
extern const int g_boatBlockedHexes[];
DATA(0x0063c7ca) extern const unsigned short g_obstacleMagicTerrainMasks[];

// LowerDoor's quick-combat bypass and the four redraw-bound sources.
// Names are address ordinals because no surviving public symbol names
// them; widths and uses are byte-proven by the retail body.
extern int g_combatActive;

// Rectangles built by the retail static initializers at 0x4626a0..0x462759.


// The row-column table one hex LEFT of gCastleWallColumns, at 0x63bce8
// (retail bytes 0b 1c 2c 3d 4d 5f 6f 81 92 a4 b5 - each entry exactly
// one less than the wall column of the same row). LeftOfMoat divides
// the hex by the same 0x11 row stride and answers `hex < moat column`;
// IsInMoat walks all eleven entries looking for an exact hit. Name is
// a BOOTSTRAP INVENTION in the style of gCastleWallColumns - no roster
// attests it.
extern const unsigned char g_moatHexes[];

// The row-column table one hex left again, at 0x63bcf4 (bytes 0a 1b 2b
// 3c 4c 5e 6e 80 91 a3 b4). Only IsInMoat reads it, and only when the
// defending town is a Fortress - the second moat ring. Name is a
// BOOTSTRAP INVENTION.
extern const unsigned char g_innerMoatHexes[];


// The five wall segments the castle AI checks, at 0x63abe0: the
// TWallSection values {6, 8, 9, 10, 12}, i.e. wallTargets rows 1..5 by
// their `wall` column - the five bombardable WALL sections, with the two
// towers and the keep left out. should_stay_in_castle walks it as a
// POINTER, `for (p = begin; p < end; p++)`, and the end address is a
// reloc of its own in the retail instruction stream. Hence the second
// symbol: our reloc has to carry addend ZERO to reproduce the bytes,
// exactly the representation constraint that made town.h's four hit
// rectangles sixteen separate ints. Neither is defined here - findpath
// and ai only read them, and an unclaimed extern still pairs.
extern const long g_castleWallGateTargets[5];   // 0x63abe0


// Windows fixed-viewport implementations. CE drawing.cpp:513/514 forwards
// a by-value extent to the four-int UpdateCombatArea (dc 0x83ec0/0x83ee8).
// Preserve that call and inclusive dimensions. The CE leaf clips/translates
// viewport offsets, calls six-int Window::UpdateScreen and redraws a combat
// window; retail Fly instead calls four-int updateScreen at 0x4b4df3.
// Likewise CE ScrollTo (drawing.cpp:598, dc 0x8405c) moves/redraws a viewport;
// retail Fly has neither its scrolling work nor a scrolled-result branch.
// One shared Windows definition accounts for those cross-TU expansions.
// Header placement is a platform visibility inference, not recovered lexical
// source. dc_only.tsv retains the CE origins separately. The by-value helper
// chain reproduces all 1102 Fly bytes; an inlined copy does not prove a
// const-reference parameter. The coordinate ScrollTo facade stays in drawing.cpp.
inline void combatManager::updateCombatArea(int x, int y, int width, int height)
{
    g_windowManager->updateScreen(x, y, width, height);
}

inline void combatManager::updateCombatArea(SLimitData area)
{
    updateCombatArea(area.m_minX, area.m_minY, area.width(), area.height());
}

inline bool combatManager::scrollTo(SLimitData, bool, bool, bool)
{
    return false;
}

#endif  /* HOMM3_CMBTMGR_H */
