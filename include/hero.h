// hero.h - prototypes of hero.cpp (compiland hero.obj)
#ifndef HOMM3_HERO_H
#define HOMM3_HERO_H

#include <string>
#include <va.h>
#include "armygrp.h"
#include "mapcell.h"
// TArtifact - the id domain hero::remove_artifact takes. It is the
// artifact domain's own type and artifact.h is deliberately outside
// initialize.cpp's include closure (see the placement note there), so
// this edge cannot reach the initialize_game_data tripwire: nothing in
// that closure includes hero.h.
#include "artifact.h"
#include "herospec.h"
// TSpellSchool - the mask hero::GetSpellSchoolLevel and
// hero::GetHighestSchool take. Its own domain header rather than a second
// copy here.
#include "spellschool.h"
#include "advmgr_popup.h"

// hero.obj's four primary-stat descriptions.  Dreamcast supplies the name
// and type; Complete fixes the 0x6a7540 address and all four indexed readers.
extern const char* g_statDesc[4];

// Hero-class ids. Dreamcast CodeView supplies the original 0..15 ladder;
// retail GetNewHeroId extends it with the two Conflux classes, indexes all
// eighteen class-traits rows, and uses 18 as the no-class sentinel.
enum THeroClass {
    eClassKnight = 0,
    eClassCleric = 1,
    eClassRanger = 2,
    eClassDruid = 3,
    eClassAlchemist = 4,
    eClassWizard = 5,
    eClassPagan = 6,
    eClassHeretic = 7,
    eClassDeathKnight = 8,
    eClassNecromancer = 9,
    eClassOverlord = 10,
    eClassWarlock = 11,
    eClassBarbarian = 12,
    eClassBattleMage = 13,
    eClassBeastmaster = 14,
    eClassWitch = 15,
    eClassPlanesWalker = 16,
    eClassElementalist = 17,
    kNumHeroClasses = 18
};

// Hero/boat sprite sequence ids, transcribed COMPLETE from the
// Dreamcast CodeView enum `hero_seqid` (the creature_seqid precedent in
// csprite.h). Retail proves the five STAND values and their order
// directly: hero::GetStandSequence (0x4d9110) and boat::GetStandSequence
// (0x4d9160) are one eight-entry jump table each over the compass
// facing, returning 0/1/2/3/4 with 5,6,7 folded back onto 3,2,1 - i.e.
// the west-facing frames are the east-facing ones mirrored, which is
// exactly what a n/ne/e/se/s roster with no west members implies.
enum hero_seqid {
    hs_stand_n = 0,
    hs_stand_ne = 1,
    hs_stand_e = 2,
    hs_stand_se = 3,
    hs_stand_s = 4,
    hs_walk_n = 5,
    hs_walk_ne = 6,
    hs_walk_e = 7,
    hs_walk_se = 8,
    hs_walk_s = 9,
    hs_turn_n_ne = 10,
    hs_turn_ne_n = 11,
    hs_turn_ne_e = 12,
    hs_turn_e_ne = 13,
    hs_turn_e_se = 14,
    hs_turn_se_e = 15,
    hs_turn_se_s = 16,
    hs_turn_s_se = 17,
    hs_max = 18
};

// Byte-proven by HasArtifact 0x4d91b0: 19 equipped slots of 8 bytes
// starting at 0x12d, then 64 backpack slots of 8 bytes at 0x1d4; each
// slot's first dword is the artifact id. The 0x12d start is NOT
// 4-aligned, so hero is a packed record (retail reads the ids with
// unaligned dword loads). Names provisional.
#pragma pack(push, 1)

enum EHeroBackpackLimit {
    HERO_BACKPACK_CAPACITY = 64
};

// Dreamcast's public wearable-position type. Complete adds a nineteenth
// equipped position, but retains the same dword parameter ABI and may pass
// that retail-only ordinal through functions which use this shared type.
enum TArtifactSlot {
    eArtifactSlotHead = 0,
    eArtifactSlotShoulders,
    eArtifactSlotNeck,
    eArtifactSlotRightHand,
    eArtifactSlotLeftHand,
    eArtifactSlotTorso,
    eArtifactSlotRightRing,
    eArtifactSlotLeftRing,
    eArtifactSlotFeet,
    eArtifactSlotMisc1,
    eArtifactSlotMisc2,
    eArtifactSlotMisc3,
    eArtifactSlotMisc4,
    eArtifactSlotWarMachine1,
    eArtifactSlotWarMachine2,
    eArtifactSlotWarMachine3,
    eArtifactSlotWarMachine4,
    eArtifactSlotSpellbook,
    kNumArtifactSlots,
    const_first_artifact_slot = eArtifactSlotHead
};

// Shared packed prefix of heroes and boats. Dreamcast CodeView proves both
// inheritance edges and supplies the member identities; retail proves the
// 0x18-byte extent and every serialized offset. Retail packs type_point at
// +0x07, one byte earlier than the naturally aligned Dreamcast build.
struct type_obscuring_object {
public:
    short m_x;
                              // +0x00 (DC mapX)
    short m_y;
                              // +0x02 (DC mapY)
    short m_z;

private:
                              // +0x04 (DC mapZ)
    unsigned char m_valid;
                  // +0x06
    type_point m_obscuredLocation;

public:
         // +0x07
    char m_paddingBeforeObscuredType;
    TAdventureObjectType m_obscuredType;

private:
    // +0x0c (DC type)
    unsigned char m_wasTrigger;

public:
            // +0x10
    char m_paddingBeforeExtraInfo[3];
             // +0x14

    type_obscuring_object();
    class town* getObscuredTown() const;
    // E:\gamedcs\hero.h:117. The Dreamcast tiny helper is the direct byte
    // accessor; retail expands it to the same +0x10 load at its callers.
    bool obscuredIsTrigger() const
    {
        return m_wasTrigger;
    }
    // DC records the public const accessor; NewmapCell::getExtraInfo
    // expands its +0x14 load for obscuring heroes and boats in retail.
    unsigned long getObscuredExtraInfo() const { return m_extraInfo; }
    void initialize();
    // E:\\gamedcs\\Hero.h:145.  The DC tiny helper is the validity byte;
    // retail folds it into unblock_lith before temporarily restoring the
    // hero's underlying map cell.
    bool isOnMap() const { return m_valid != 0; }
    // E:\gamedcs\Hero.h:150. Dreamcast retains an out-of-line copy, while
    // retail expands the validity test at every admitted Windows caller.
    TAdventureObjectType getObscuredObject() const
    {
        if (m_valid)
            return m_obscuredType;
        return NOTHING;
    }
    // E:\gamedcs\Hero.h:157
    VA(0x0042ec70, 0x4f)  // exact body/callers x2, dc 0x1fb2c
    type_point getLocation() const
    {
        return type_point(m_x, m_y, m_z);
    }
    bool load(void* infile);
    // Dreamcast proves this Hero.h helper boundary. Retail SetupHeroView
    // folds it to the same three field tests; keep the call in source so
    // an exact lowering cannot erase the attested source shape again.
    __forceinline unsigned char obscuresTown() const
    {
        return m_valid && m_wasTrigger && m_obscuredType == TOWN;
    }
    void restoreCell();
    bool save(void* outfile);

protected:
    void obscureCell(TAdventureObjectType newType, long id);

private:
    unsigned long m_extraInfo;

};
SIZE(type_obscuring_object, 0x18);

// Preserve the caller's packing for boat, as at its former location.
#pragma pack(pop)

// boat - the adventure-map vessel. It lives here for the same reason
// type_obscuring_object does: the Dreamcast roster declares it in
// E:\gamedcs\Hero.h (boat::boat, dc 0xbcc18, Hero.h:188), even though
// its one reconstructed method sits in hero.cpp.
// ONE field is retail-byte-proven - boat::GetStandSequence (0x4d9160)
// reads the compass facing at +0x1b with `movsx eax, byte [this+0x1b]`
// and feeds it the same eight-entry jump table hero's twin uses, which
// is what fixes both the offset and the SIGNED type. The DC roster
// lands `facing` on offset 27 too, and because its whole 24..28 band is
// one-byte fields there is no packing ambiguity to resolve; the four
// neighbours it names (allocated 24, id 25, type 26, playerOwner 28)
// are therefore very likely at +0x18/+0x19/+0x1a/+0x1c.
// SLICED AND CLOSED 2026-08-08 by game::GetHeroBoat (0x4ce900), which
// walks the pool with a 0x28 stride and reads exactly three of those
// DC slots at their unshifted DC offsets - `allocated` +0x18,
// `occupying_hero` +0x20 (a dword compare) and `occupied` +0x24 (a byte
// compare). With facing already proven at +0x1b that is four hits and
// the stride, so the DC roster transfers to retail unrepacked and the
// extent is now proven: sizeof is 0x28.
class boat : public type_obscuring_object {
public:
    unsigned char m_allocated;
        // +0x18
    unsigned char m_id;
               // +0x19
    char m_type;
                      // +0x1a
    signed char m_facing;
             // +0x1b
    char m_playerOwner;
               // +0x1c
    char m_paddingBeforeOccupyingHero[3];
    int m_occupyingHero;
             // +0x20 (THeroID)
    unsigned char m_occupied;
         // +0x24
    char m_paddingAfterOccupied[3];
    boat() : m_allocated(0) {}
    hero_seqid getStandSequence();
    // Hero.h:196 in Dreamcast. Complete expands this ordinary header helper
    // in MoveHero, CreateBoat and the event-record undo path; retaining the
    // named boundary also preserves the byte-id zero extension at each site.
    void obscureCell()
    {
        type_obscuring_object::obscureCell(BOAT, m_id);
    }

};
SIZE(boat, 0x28);

#pragma pack(push, 1)

// The 8-byte artifact record - what an equipped slot or a backpack
// slot actually holds. NAME CORRECTED 2026-08-08: this header used to
// call it TArtifactSlot, but on the Dreamcast build TArtifactSlot is
// the ENUM of wearable positions (eArtifactSlotHead 0 ..
// eArtifactSlotSpellbook 17) and `type_artifact` is the record - DC
// members.csv gives it exactly these two dwords, `TArtifact type` at 0
// and `SpellID spell` at 4. Retail agrees: hero::add_to_backpack
// (0x4e2f90) takes a `const type_artifact*`, reads its two dwords and
// copies them straight into a backpack slot. Every carcass declarator
// in this tree already used the DC meaning of both names, so the live
// struct was the odd one out. The member spellings stay provisional
// (no retail body names them).
struct type_artifact {
public:
    TArtifact m_artifactId;
    int m_extra;
    // DC Hero.h:211 stores the artifact argument at +0, then line 212 stores
    // the -1 sentinel at +4. Retail value_of_town preserves that order in
    // its register allocation even though the eventual by-value pushes are
    // ordered by record layout.
    // The generated offering constructor at dc 0x128714 calls this
    // constructor with -1. That proves the default argument: no separate
    // zero-argument type_artifact constructor exists in the DC class API.
    explicit type_artifact(TArtifact id = ARTIFACT_NONE)
    {
        m_artifactId = id;
        m_extra = -1;
    }
    // Dreamcast Hero.h:214-218. A spell scroll is represented by artifact
    // id 1 and its SpellID payload; this semantic constructor is distinct
    // from the generic TArtifact constructor above.
    explicit type_artifact(SpellID spell)
    {
        m_artifactId = ARTIFACT_SPELL_SCROLL;
        m_extra = spell;
    }
    void getRolloverText(char* buffer) const;
    // The reconstruction-only (int, int) overload was removed. Ordinary
    // artifacts use the TArtifact constructor; scrolls use SpellID. A
    // separately decoded payload is assigned explicitly by its owning caller.
    // Both proven constructors retain their DC id-before-payload store order.

// townmgr.cpp's blacksmith right-click text (0x5d1aa0) calls this on a
// copy of the war machine's artifact record; hero.obj owns the
// DEFINITION (0x4db3e0). Dreamcast LF_MFUNCTION 0x4d51 carries a const
// type_artifact this pointer.
    std::basic_string<char, std::char_traits<char>, std::allocator<char> >
        getDescription() const;

};

class boat;

// The two combat latches hero::Deallocate consults before dismissing the
// army and before re-rolling the garrison. DECLARATION ONLY - cmbtmgr.h
// owns the DATA claims on 0x6985a3 / 0x697744, and a second claim on the
// same RVA is a fatal duplicate at delink time. Declared here rather than
// by including cmbtmgr.h, which hero.obj's measured include closure does
// not otherwise need.
extern unsigned char g_combatFlag6985a3;
extern unsigned char g_combatFlag697744;

// 0x485d90, a /Gr free helper claimed in customcampaign.cpp. The returned
// string's hidden pointer takes ECX and infile takes EDX, as hero::load's
// call proves. The semantic name is provisional; no Dreamcast symbol covers it.
std::string readLengthPrefixedString(TAbstractFile* infile);

// hero::CheckLevel's three outside names. DECLARATIONS ONLY - the DATA
// claims on 0x6a7570 and 0x69954c belong to levelupwindow.cpp and
// kbwin.cpp, and a second claim on one RVA is a fatal duplicate at
// delink (the gCombatFlag pair above is the same case). They live here
// rather than in a .cpp because a line-initial `extern` in a .cpp is a
// cleanliness-floor violation, and here rather than by including
// kbwin.h / philai.h, whose closures hero.obj does not otherwise need.
extern const char* g_skillMasteryNames[3];
// Retail SetupHeroView indexes mastery values 1..3 from the pointer cell
// immediately before gSkillMasteryNames, giving that biased view its own
// relocation at 0x6a756c.
// HeroScrn.txt row declarations shared with swapmgr's hero-exchange screen.
// src/hero.cpp owns the DATA claims; these declarations only expose the
// already-proven contiguous runtime text table to its source twin.
DATA(0x006a756c) extern const char* g_skillMasteryNamesBiased[4];
extern const char* g_heroScreenText0;
extern const char* g_heroScreenNameFormat;
extern const char* g_heroScreenMoraleHighText;
extern const char* g_heroScreenMoraleNeutralText;
extern const char* g_heroScreenMoraleLowText;
extern const char* g_heroScreenLuckHighText;
extern const char* g_heroScreenLuckNeutralText;
extern const char* g_heroScreenLuckLowText;
extern const char* g_heroScreenText9;
extern const char* g_heroScreenArmyMoveFormat;
extern const char* g_heroScreenSecondarySkillFormat;
extern const char* g_heroScreenText22;
extern const char* g_heroScreenText27;
extern const char* g_heroScreenMixedArmyHelp;
extern int g_videoPaused;
// 0x6aa9d8. DECLARATION ONLY - src/townmgr.cpp:163 owns the DATA claim,
// and a second claim on one RVA is a fatal duplicate at delink. hero.obj
// reads it at 0x4db7d3, 0x4dd9f1, 0x4dda8d, 0x4e1bad and 0x4e1c13;
// SetupHeroView treats it as the "hero list is suppressed" latch.
extern int g_unnamed6aa9d8;
// movement.txt row 6 column 5, DECLARATION ONLY - include/events.h:439
// owns the DATA claim on 0x698a94, and a second claim on one RVA is a
// fatal duplicate at delink. hero::GetMobility adds it on the flag-bit-1
// arm, the third reader that fixes its role.
extern int g_stablesMovementBonus;

// The two ARRAYTXT.TXT runs text.obj's loader (0x5b9cc0) fills, read by
// hero::get_morale_description / get_luck_description. DECLARATION ONLY -
// viewarmywindow.cpp owns the DATA claims on 0x6a57bc / 0x6a532c, and a
// second claim on the same RVA is a fatal duplicate at delink time.
extern const char* g_moraleTexts[42];
extern const char* g_luckTexts[25];

class hero : public type_obscuring_object {
public:
    enum {
        CLASS_NAME_OVERRIDE_HERO_ID = 27,
        CLASS_NAME_OVERRIDE_SCENARIO = 15,
        CUSTOM_NAME_CAMPAIGN_EXCLUDED_SCENARIO = 20,
        CUSTOM_NAME_CAMPAIGN_PORTRAIT = 156,
        PRIMARY_STAT_DIALOG_TYPE = 1,
        PRIMARY_STAT_QUICK_DIALOG_TYPE = 4,
        PRIMARY_STAT_DIALOG_Y = 28,
        PRIMARY_STAT_RESOURCE_FIRST = 31,
        PRIMARY_STAT_RESOURCE_QUANTITY = 0x10000
    };
    // The nineteenth equipped position - the one Shadow of Death added on
    // top of the Dreamcast TArtifactSlot roster's eighteen.
    // hero::HeroFn_004E2550 (0x4e2550) refuses it outright while the
    // engine gate reports a pre-SoD game. GATED to hero.obj's own view:
    // an ungated enumerator is a measured include-set cost in this tree
    // (netmsg.h's RS_ERASE_OBJECT note records 90.84 -> 88.24 on an
    // unrelated TU), and no other compiland needs the name.
    enum { EQUIPPED_SLOT_SOD_MISC = 18 };
    // Two more equipped positions WindowHandler special-cases by INDEX
    // (`if (slot == 0x11)` opens the spellbook, `if (slot == 0x10)`
    // refuses the fourth war-machine position with general text 313).
    // The spellings are the Dreamcast TArtifactSlot roster's; the values
    // are retail's own compares. Same gate and same reason as above.
    enum {
        EQUIPPED_SLOT_WAR_MACHINE_4 = 16,
        EQUIPPED_SLOT_SPELLBOOK = 17
    };
    // hero::Deallocate's domain, same gate and same reason. The
    // availability byte's 0x40 rung is the "sits in a tavern recruit
    // pool" sentinel game::Load memsets the whole array with; the two
    // campaign rungs are the scenarios that keep one specific hero
    // recruitable after death, addressed once by portrait and once by id
    // (retail stores through a CONSTANT displacement for the second, so
    // the id really is written twice in the source).
    enum {
        HERO_AVAILABILITY_TAVERN_POOL = 0x40,
        HERO_AVAILABILITY_PRISON = 0x41,
        DEALLOCATE_CAMPAIGN_BY_PORTRAIT = 8,
        DEALLOCATE_CAMPAIGN_BY_HERO_ID = 12,
        DEALLOCATE_KEPT_PORTRAIT = 0x9e,
        DEALLOCATE_KEPT_HERO_ID = 150
    };
    // hero::CheckLevel's domain, same gate and same reason. The
    // class-traits row carries TWO primary-skill chance columns and the
    // level-up takes the second from level 10 up (retail `cmp cx,9 / jg`,
    // a 16-bit signed test on the already-incremented level). The
    // campaign/hero pair is the one scenario that swaps in the Barbarian
    // class row instead of the hero's own, and re-rolls against that
    // row's first two chances rather than a percentile. Note that
    // LEVEL_UP_CAMPAIGN_OVERRIDE's 14 collides in VALUE with
    // eSecSkillSchoolOfAirMagic - different domains, which is exactly
    // why both want names.
    enum {
        LEVEL_UP_LOW_LEVEL_LAST = 9,
        LEVEL_UP_CAMPAIGN_OVERRIDE = 14,
        LEVEL_UP_OVERRIDE_HERO_ID = 45,
        LEVEL_UP_SKILL_CHOICES = 2
    };
    // playerData::personality (+0x34) as hero::GetMobility reads it: an
    // AI player above the difficulty threshold gets +75 movement, and +50
    // more when its h3m AI tactic is the aggressive one. The VALUE is
    // retail-proven; the spelling follows the h3m roster (0 pacifist,
    // 1 friendly, 2 aggressive, 3 explorer) and is PROVISIONAL.
    enum { AI_PERSONALITY_AGGRESSIVE = 2 };
    // Spell points. Byte-proven SHORT: the type_AI_combat_data ctor
    // (0x423f3d) widens it into the combat record's long mana, and
    // AI_auto_combat (0x4275a6/0x4275b6) writes the simulated mana back
    // with 16-bit stores.
    short m_mana;
                     // +0x18
    // The hero's own id - the index of this record in gpGame->heroes.
    // Byte-proven a full DWORD by town.obj: town::remove_garrison_hero
    // (0x5be407) and town::SwapHeroes both read `mov edx,[hero+0x1a]`
    // and feed it straight back into the 1170-stride heroes index, and
    // town::View (0x5be3fa) pushes it to advManager::SetHeroContext.
    int m_id;
                         // +0x1a
    // +0x1e. HeroFn_004D8B30 copies the setup record's +0x08 dword
    // straight in here, which is the only retail body that touches these
    // four bytes at all - hence a full DWORD and hence a member rather
    // than a pad. No name survives and nothing else reads it, so the
    // spelling stays ORDINAL. (The h3m Shadow of Death hero record's
    // leading questIdentifier is the obvious candidate and is exactly
    // the field retail's HeroExtra has that the Dreamcast's lacks, but
    // that is inference, not evidence.)
    int m_order;
                  // +0x1e
    // Owning player. SIGNED char: town::View widens it with
    // `movsx edx, byte [gpGame + 1170*id + 0x21642]` before comparing
    // it against the acting-player id. Name provisional.
    signed char m_owner;
              // +0x22
    // +0x23. HeroFn_004D8B30 copies exactly thirteen bytes of the setup
    // record's name here; SetRolloverText passes this band to sprintf.
    char m_name[13];
    // +0x30. DrawHeroPart indexes the eighteen-entry cursorIcons sprite row
    // directly with this dword; the surviving roster names the domain.
    int m_heroClass;
    // +0x34. The current-hero gate in HeroFn_004D8FB0 compares this byte
    // directly against portrait id 156. Dreamcast independently places its
    // `portrait` byte at the same offset.
    unsigned char m_portrait;
    // BuildPath copies this packed target point out of the record. The
    // two leading coordinates are dwords - their four-byte spacing
    // proves the stored type, and BuildPath's own load widths are
    // narrowed only by the destination bitfields.
    int m_pathTargetX;
                    // +0x35
    int m_pathTargetY;
                    // +0x39
    // +0x3d..+0x42, three SHORTS - narrowed 2026-08-20 out of the old
    // `int pathTargetZ; char pad_041[2];` by hero::save (0x4d80c0),
    // which serialises this band as `mov cx, word [this+0x3d]` /
    // `+0x3f` / `+0x41` into a 16-bit scratch and writes each with
    // size 2. An int at +0x3d cannot produce those loads: assigning
    // one to a short scratch would still be a 16-bit load, but there
    // would be no lvalue at all at +0x3f, and retail reads one there.
    // The narrowing is invisible to the only other consumer -
    // searchArray::BuildPath (0x56a0d0) takes `mov dl, byte [eax+0x3d]`
    // on BOTH sides because type_point's z is a four-bit bitfield.
    // The two trailing shorts have no other reader; ORDINAL PLACEHOLDERS.
    short m_pathTargetZ;
                  // +0x3d
    // +0x3f, DC-attested (`hero,66,T_SHORT,last_magic_school_level`,
    // retail +0x3f under the same -5 repack). hero::CheckLevel writes the
    // new `level` here whenever the level-up offers a magic school.
    short m_lastMagicSchoolLevel;
      // +0x3f
    short m_targetDistance;
                    // +0x41
    unsigned char m_targetIsCritical;
       // +0x43
    // The patrol triple at +0x44..+0x46 and the compass facing at
    // +0x47, all byte-proven by hero::is_in_patrol_radius (0x4e56e0)
    // and hero::GetStandSequence (0x4d9110). The two coordinates are
    // read ZERO-extended (`and ebx,0xff`, `xor edx,edx / mov dl,..`)
    // and the radius SIGNED (`test cl,cl / jl`, then `movsx ecx,cl`) -
    // the widths the DC roster gives them too (patrolX/patrolY
    // T_UCHAR, patrolRadius T_CHAR). `facing` is likewise the DC name
    // and is loaded UNSIGNED here (`xor eax,eax / mov al,[this+0x47]`)
    // where boat's twin at +0x1b takes a movsx.

    // kPatrolNone is the "not patrolling" sentinel patrolX carries.
    // Retail tests it at BYTE width (`cmp bl,-1` == 0xff) while every
    // arithmetic use of the field zero-extends, which is what fixes
    // both the value and the unsigned type. Name role-inferred,
    // PROVISIONAL.
    enum { kPatrolNone = 0xff };
    // The eight compass values `facing` takes. UNATTESTED NAMES - no DC
    // enum covers the field (its roster types it a bare T_UCHAR), so
    // these are placeholders. The MAPPING is not guessed: the stand
    // jump table sends facing 0->hs_stand_n, 1->hs_stand_ne,
    // 2->hs_stand_e, 3->hs_stand_se, 4->hs_stand_s, and folds 5,6,7
    // back onto se, e, ne - which fixes 0..4 as n..s going clockwise
    // and 5..7 as the mirrored sw, w, nw.
    enum {
        kFacingN = 0, kFacingNE = 1, kFacingE = 2, kFacingSE = 3,
        kFacingS = 4, kFacingSW = 5, kFacingW = 6, kFacingNW = 7
    };
    unsigned char m_patrolX;
          // +0x44
    unsigned char m_patrolY;
          // +0x45
    signed char m_patrolRadius;
       // +0x46
    unsigned char m_facing;
           // +0x47
    unsigned char m_formation;
        // +0x48 (DC name)
    // +0x49. ProcessHover uses this full signed DWORD as the movement
    // allowance for each later day when converting a path cost to turns.
    // Dreamcast names the corresponding field maxMobility.
    int m_maxMovePoints;
    // +0x4d, the hero's remaining movement points. A full DWORD read
    // SIGNED: hero::GetMobilityFrame (0x4e5330) loads it whole, takes
    // the <= 0 arm with `jg`, and divides it by 100 with the signed
    // 0x51eb851f reciprocal - an unsigned field would use the unsigned
    // magic instead. Name provisional.
    int m_movePoints;
                 // +0x4d
    int m_experience;
                 // +0x51 (DC name; retail packed)
    // +0x55, a SHORT the five specialty factor getters (0x4e42b0,
    // 0x4e4310, 0x4e4840, 0x4e48b0, 0x4e4920) widen with `movsx eax,
    // word [this+0x55]` and turn into the specialty scale
    // `x * 0.05f + 1.0f`. That is HoMM3's per-level specialty growth,
    // so the field is the hero's level - name role-inferred, PROVISIONAL.
    short m_level;
                    // +0x55
    // This visit-flag run is fixed by SetRolloverText's retail loads and
    // the uniform retail/DC hero-layout repack described below. These are
    // canonical members: the former pad-vs-fields include personality was
    // unnecessary because both arms had the same layout.
    unsigned long m_trainingGroundsFlags;
    // +0x57
    unsigned long m_defenseTowerFlags;
       // +0x5b
    unsigned long m_gardenOfRevelationFlags;
 // +0x5f
    unsigned long m_mercCampFlags;
           // +0x63
    unsigned long m_powerSchoolFlags;
        // +0x67
    // +0x6b, one visit bit per Tree of Knowledge id. SetTreeHelpText
    // masks the cell's low five extra-info bits into this dword.
    unsigned long m_treeOfKnowledgeFlags;
    unsigned long m_libraryFlags;
             // +0x6f
    // +0x73. One dword of "this hero has already used that object"
    // bits, indexed by the cell's extraInfo: hero::VisitedArena
    // (0x4e53c0) tests `(1 << cell->extraInfo) & [this+0x73]` and
    // SetVisitedArena (0x4e53e0) ORs the same bit in.
    // The DC repack (no alignment, ArenaFlags at DC 120 against retail
    // 0x73 == 115, a uniform -5) puts ten more such dwords around it -
    // GardenOfRevelation +0x5f, MercCamp +0x63, PowerSchool +0x67,
    // TreeOfKnowledge +0x6b, Library +0x6f, then MagicSchool +0x77,
    // WarSchool +0x7b, University +0x7f, Shrine1 +0x83, Shrine2 +0x87.
    // SetTreeHelpText now independently proves TreeOfKnowledgeFlags above;
    // SetRolloverText proves the fields it reads directly.
    unsigned long m_arenaFlags;
    unsigned long m_magicSchoolFlags;
         // +0x77
    unsigned long m_warSchoolFlags;
           // +0x7b
    unsigned long m_universityFlags;
          // +0x7f
    unsigned long m_shrine1Flags;
             // +0x83
    unsigned long m_shrine2Flags;
             // +0x87
    unsigned long m_shrine3Flags;
             // +0x8b
    // +0x8f / +0x90, DC-attested (evidence/dreamcast/members.csv rows
    // `hero,148,iLevelSeed` and `hero,149,lastWisdom` - the same uniform
    // -5 repack the flag band above already answers to, and the two rows
    // sit between Shrine3Flags (DC 144, retail +0x8b) and heroArmy
    // (DC 152, retail +0x91), so the pair fills this pad exactly).
    // RETAIL BYTE-PROOF, both from hero::CheckLevel (0x4da720): it seeds
    // the level-up RNG with
    //   SRand(level*214013 + iLevelSeed*156823 + 154079)
    // reading +0x8f ZERO-extended (`xor ecx,ecx / mov cl,[ebx+0x8f]`,
    // which is what types it unsigned), and it stores the new level's LOW
    // BYTE into +0x90 (`mov dl,[ebx+0x55] / mov [ebx+0x90],dl`) whenever
    // the level-up offers Wisdom - the "guaranteed Wisdom within N
    // levels" bookkeeping, which is what makes the DC name fit.
    unsigned char m_levelSeed;
           // +0x8f
    unsigned char m_lastWisdom;
           // +0x90
    // Seven army slots: creature type at 0x91+i*4, count at 0xad+i*4
    // (CreatureTypeCount reads [ecx-0x1c] against [ecx] with ecx
    // walking from 0xad) - exactly armyGroup's 56-byte layout, and
    // AI_approximate_strength (0x427657) hands `hero + 0x91` straight
    // to armyGroup::get_AI_value as a this pointer.
    armyGroup m_army;
    // Secondary-skill mastery bytes, a 28-entry band starting at 0xc9,
    // all read as SIGNED chars. THREE slots are byte-proven, and each
    // lands exactly where the standard secondary-skill order puts it -
    // which is what promotes the band from a guess to a model:
    //   +0xd0 slot 7  Wisdom     - do_aftermath's inlined do_eagle_eye
    //                              (0x426f44) caps the learnable spell
    //                              level at wisdom + 2
    //   +0xd3 slot 10 Ballistics - check_wall_archery_penalty (0x424848)
    //                              subtracts it from the wall distance
    //   +0xd4 slot 11 Eagle Eye  - do_aftermath (0x426eee) gates the
    //                              spell-learning pass on it and adds 1
    //                              to it as the level bound (0x426f20)
    // The 28-entry extent, the SIGNED element type and the 0xc9 base are
    // byte-proven by the SS trio (0x4e2210 SetSS / 0x4e2250 TakeSS /
    // 0x4e22d0 GiveSS): all three address the band as
    // `[iWhichSS + this + 0xc9]`, load it with `movsx`, and TakeSS's
    // companion sweep over the 0xe5 band runs `cmp ebx,0x1c` - 28.
    // PLAIN ARRAY, and it must stay one: the union that used to carry a
    // named-slot second arm (wisdomLevel/ballisticsLevel/eagleEyeLevel
    // over pad arrays) made VC6's synthesized memberwise operator=
    // copy the band ONCE PER UNION ARM - DoCombat's inline hero copies
    // showed the 28-byte loop followed by the pads' 7/2/16-byte loops
    // over the same bytes, where retail runs the single 0x1c loop
    // (found 2026-08-27; the town.h pad-array retirement is the same
    // disease). Consumers spell the proven slots through the enum:
    // skillLevel[eSecSkillWisdom] +0xd0, [eSecSkillSiegeBallistics]
    // +0xd3, [eSecSkillEagleEye] +0xd4, and Artillery
    // [eSecSkillBattlefieldBallistics] +0xdd - all byte-identical
    // addressing (see the trio note above for the slot proofs).
    signed char m_skillLevel[28];
     // +0xc9
    // Acquisition-order band, 28 entries at +0xe5, read UNSIGNED
    // (TakeSS's renumbering sweep compares with `jbe`, not `jle`).
    // GetNthSS scans it for order iWhich+1 and returns the slot index;
    // GiveSS writes skillCount+1 into the newly-learned slot and TakeSS
    // decrements every entry above the vacated one before zeroing it.
    unsigned char m_skillOrder[28];
       // +0xe5
    // Number of secondary skills known. A full DWORD: GiveSS's cap test
    // is `cmp dword [this+0x101],8` and both trio bodies increment /
    // decrement it 32 bits wide; the narrowed `mov al,byte [this+0x101]`
    // in GiveSS is just VC6 sinking the load into a byte store.
    int m_skillCount;
                     // +0x101
    // +0x105, the hero's flag word. Read as a full DWORD and tested
    // bitwise: hero::GetMobility() (0x4e4d90) hands bit 18 (0x40000)
    // to the sea-movement overload, and hero::can_land (0x4e5ce0)
    // tests the same 0x40000 for "aboard a boat". Name provisional -
    // no DC symbol covers the word; the extent and the read are
    // byte-proven.
    unsigned int m_flags;
                 // +0x105
    float m_turnExperienceToRvRatio;
          // +0x109 (DC name)
    signed char m_dWalkSpellsCast;
            // +0x10d (DC name)
    // +0x10e. TQuickHeroWindow reads the full mastery value to decide
    // whether an enemy army is shown normally, as copies of its strongest
    // stack, or as the strongest creature of the owner's alignment. The
    // Dreamcast roster independently names the corresponding dword
    // `disguiseLevel`; retail's later flight/water-walk pair fixes the
    // four-byte extent from the other side.
    int m_disguiseLevel;
    // +0x112, the FLIGHT level - the twin of waterWalkLevel below and
    // the other half of the movement-override pair hero::IsMobile
    // (0x4e5f30) loads together. Sliced 2026-08-08 out of the old
    // pad_109 by findpath's GetTerrainCost (0x4b18c0), which reads
    // +0x112 and +0x116 back to back as the two mastery levels it
    // forwards to CalcTerrainCost's `flying` and `water_walking`
    // slots, raises EITHER to eMasteryExpert when the matching
    // artifact is worn, and drives BOTH to -1 while the boat bit
    // (flags & 0x40000) is set. Name from the role; the sibling's
    // comment already called this slot an ordinal placeholder.
    int m_flightLevel;
                    // +0x112
    // +0x116, the water-walking level. Byte-proven by
    // hero::WalkOnWater (0x4e5dd0), whose entire body is
    // `mov [ecx+0x116], arg`, and by hero::IsMobile (0x4e5f30), which
    // loads +0x116 and +0x112 together as the movement-override pair.
    // Name from the writer; ordinal placeholder for the sibling.
    int m_waterWalkLevel;
                 // +0x116
    // Two one-byte battle temporaries. hero::ApplyBattleWinTemps
    // (0x4da510) opens by zeroing both from one `xor al,al`, storing
    // +0x11b BEFORE +0x11a, and then clears twenty-two `flags` bits -
    // which is what marks the whole body as post-combat cleanup.
    // Neither byte has a Dreamcast row at these offsets and no other
    // retail body reads them yet, so the names are ORDINAL PLACEHOLDERS
    // and the signedness is unproven (a zero store shows neither).
    signed char m_moraleBonus;
              // +0x11a
    signed char m_luckBonus;
              // +0x11b
    // +0x11c, the "skip me" byte. playerData::NextHero (0x4baa40)
    // takes a hero only when IsMobile() is true AND this byte is zero,
    // which is exactly the adventure screen's next-hero button skipping
    // a sleeping hero. The DC roster has `IsSleeping` (T_UCHAR) a few
    // slots along in this band, so that is very likely the field - but
    // the repack shift stops being uniform right here, so the name
    // stays an ORDINAL PLACEHOLDER and only the offset and the role
    // are claimed.
    unsigned char m_isSleeping;
            // +0x11c
    int m_bounty;
                         // +0x11d (DC name)
    // Packed retail counterpart of DC's std::bitset<48> member. Its reset
    // writes the two backing dwords at +0x121/+0x125.
    std::bitset<48> m_townSpecialGrantedMask;
 // +0x121 (DC name)
    // +0x129, a dword compared against 3 - the secondary-skill
    // mastery domain. hero::HeroFn_004E5DE0 (0x4e5de0) returns it
    // unless it is below 3 and the hero's army holds creature 0x8f,
    // and hero::IsInIdentifyRange (0x4e5e10) opens with the same
    // block inlined. Name unattested - ORDINAL PLACEHOLDER.
    int m_visionsPower;

private:
                      // +0x129
    type_artifact m_equipped[19];

public:
    // One byte per artifact slot class. remove_artifact decrements the
    // component's class after dismantling a combination artifact, except
    // for the first component occupying the assembled artifact's class.
    unsigned char m_artifactSlotCounts[15];

private:
 // +0x1c5
    type_artifact m_backpack[64];
    // +0x3d4, a cached backpack count. hero::get_number_in_backpack
    // (0x4d90c0) returns it with `movsx eax, byte [ecx+0x3d4]` on its
    // flag arm instead of walking the 64 slots, which is what proves
    // both the offset and the SIGNED char width. Name provisional.
    signed char m_backpackCount;

public:
          // +0x3d4
    // Per-hero sex copied from THeroTraits during initialize. The retail
    // build added this four-byte field ahead of the custom-name state.
    int m_sex;
                              // +0x3d5 (DC trait name)
    unsigned char m_hasCustomName;
         // +0x3d9
    // Dinkumware std::string object, not merely its internal +4 pointer.
    // hero::initialize assigns the shared empty string through the normal
    // operator= path, which proves the full 16-byte object at +0x3da.
    std::string m_customName;
               // +0x3da

    // TWO per-spell byte tables, stride 1, 70 entries each, byte-proven
    // by hero::AddSpell 0x4d9330 - all 26 bytes of it are
    //     mov dl,1
    //     mov [eax + ecx + 0x3ea], dl     ; in_spellbook[whichSpell] = 1
    //     mov [eax + ecx + 0x430], dl     ; available_spells[whichSpell] = 1
    // with ecx = this and eax = whichSpell, i.e. two bases exactly
    // 0x46 = 70 apart. The 70-entry extent is pinned at BOTH ends:
    //   - 0x3ea + 70 == 0x430, the second table's own base;
    //   - 0x430 + 70 == 0x476, and 0x476 is where the four primary
    //     skills start - hero::get_primary_skill_total 0x4e5960 runs a
    //     four-iteration stride-1 signed-char loop from [ecx+0x476]
    //     (clamped to 0..99), and 0x4e6120 adds artifact bonuses into
    //     the same band. A whole-image scan of byte displacements in
    //     0x3ea..0x47f matches: scattered 1-2-reference reads inside
    //     the two tables (+0x3ea, +0x3f4, +0x404, +0x420, +0x430,
    //     +0x436..+0x439, +0x43e, +0x453, +0x455, +0x461), then a jump
    //     to 20-28 references at +0x476..+0x479.
    // NAMES ARE DC-ATTESTED, not invented: evidence/dreamcast/members.csv
    // carries `hero,969,in_spellbook`, `hero,1039,available_spells` and
    // `hero,1109,stats` - the same 70/70 spacing as retail's
    // 0x3ea/0x430/0x476, and the DC SpellID enum ends `kNumSpells,70`.
    // Corroboration for the second table specifically:
    // hero::can_summon_boat 0x4e5550 opens by testing
    // byte [this+0x430] and then handles spell 0 = eSpellSummonBoat.
    // The retail x86 offsets are byte-proven here; only the names come
    // from the Dreamcast build.
    enum { NUM_SPELLS = 70 };

private:
       // DC SpellID::kNumSpells
    unsigned char m_inSpellbook[NUM_SPELLS];
     // +0x3ea
    unsigned char m_availableSpells[NUM_SPELLS];
    // The four primary skills (DC name `stats`), byte-proven by
    // hero::get_primary_skill_total 0x4e5960 - a four-iteration
    // stride-1 SIGNED-char loop from [this+0x476], clamped to 0..99 -
    // and by 0x4e6120, which adds artifact bonuses into the same band.
    signed char m_stats[4];

public:
    // +0x47a. AI_value_of_combat (0x42730f) reads this as a float,
    // widens it to double and uses it as the attacking side's combat
    // modifier. The role remains provisional, so keep the ordinal name.
    float m_aggression;
    // 0x4d85f0, retail's own default constructor. The base and the four
    // members that carry constructors run first (type_obscuring_object,
    // army, TownSpecialGrantedMask, equipped/backpack, customName), then
    // the body re-clears the identity band. The class ALREADY had a
    // non-trivial implicit constructor - army, the bitset and the
    // Dinkumware string each have one - so declaring this changes what
    // that constructor does, not whether one exists.
    hero();
    void initialize(short index);


private:
    // +0x47e..0x491. Dreamcast names the same five-dword tail in this
    // order; retail independently proves every dword boundary through the
    // five direct writers recorded in the retail layout scan, while
    // TSeerReward::getValue reads value_of_power and value_of_knowledge at
    // +0x47e/+0x486. The +0x1e cross-build shift follows the already-proven
    // retail packing above; the band still closes SIZE(hero) exactly.
    long m_valueOfPower;
    long m_valueOfDuration;
    long m_valueOfKnowledge;
    long m_valueOfSpring;
    long m_valueOfWell;

public:
    // Retail level-update messages carry the raw four-byte skill band,
    // including values outside GetPrimarySkill's clamped gameplay range.
    // Bulk-copy boundary names provisional; bodies precede their callers.
    void copyPrimarySkills(signed char* stats) const;
    void setPrimarySkills(const signed char* stats);
    unsigned char hasArtifact(int whichArtifact) const;
    unsigned char hasSecondarySkill(int whichSkill);
    // 0x4d9330 - sets both per-spell byte tables for one spell.
    // Complete's campaign carry-over resets both spell tables together.
    // No DC counterpart survives for that added path; name provisional.
    void clearSpells();
    // 0x4d9070 / 0x4d90c0, the two artifact tallies.
    long getEquippedArtifacts(unsigned char countWarMachines) const;
    long getNumberInBackpack(unsigned char countWarMachines) const;
    // 0x4d9330 - sets both per-spell byte tables for one spell.
    void addSpell(int whichSpell);

private:
    // 0x4d95d0 - rebuilds available_spells after artifact changes.
    void updateSpellList();

public:
    void deallocate(unsigned char gameLoaded, unsigned char remoteMove);
    // 0x4d9b30, `ret 4` with `this` UNUSED - retail never reads ECX.
    // The hero screen's yes/no prompt for taking a combination artifact
    // apart: it builds `<artifact description>\n\n<general text 734>`
    // and returns the dialog reply. This Complete-only combination-artifact
    // prompt has no ViewStat identity; the old positional assignment was
    // wrong. Its provisional name and one-argument ABI are retained.
    // 0x4d97f0, `ret 0` with no arguments and `this` a HERO. Its message
    // construction, seven-slot loop and widget branches are the retail
    // lowering of Dreamcast hero::UpdateArmies (dc 0xcc540).
    void updateArmies();
    // hero.obj's own view of the same two. GiveExperience calls
    // CheckLevel on both of its arms, and GetLevel (dc 0xccc8c) is a
    // DC row with NO retail body - GiveExperience carries it expanded.
    // DC hero.cpp:1862 has only the experience parameter, no receiver;
    // retail GiveExperience expands the same receiver-independent helper.
    static int getLevel(int experience);
    // 0x4da720, hero.cpp:2147 in the Dreamcast roster (dc 0xcd17c) - the
    // no-argument level-up check advManager::TownEvent runs after each
    // combat it starts. Declared only; the body is not reconstructed and
    // the row is not claimed from here.
    void checkLevel();
    // 0x4d8b30, `ret 4`, a hero MEMBER: it copies one map/scenario setup
    // record into this hero. The Dreamcast keeps the counterpart as the
    // free function initialize_hero(hero*, const HeroExtra*)
    // (E:\gamedcs\game.cpp:9912, dc 0xb6c84); retail moved it into
    // hero.cpp as a member, so the name stays an ORDINAL PLACEHOLDER.
    // Gated with HeroExtra itself, which is what the parameter is.
    void heroFn004D8B30(const class HeroExtra* setup);
    int heroFn004D9B30(int artifact);
    // 0x4d9cc0, the ASSEMBLE partner of the row above and the same
    // shape: `ret 4`, `this` unused, one artifact id in. It resolves the
    // component's targetCombo, describes the ASSEMBLED artifact and asks
    // general text 733 with the component's name formatted in. The old DC
    // bracket assignment to ViewArtifact is disproved by that name's exact
    // retail identity at 0x4d9a00; this remains an ordinal retail-only name.
    int heroFn004D9CC0(int artifact);
    void viewArtifact(const type_artifact* artifact, int isQuickView);
    // 0x4e16d0 - repaints the hero screen's four primary-stat texts and
    // its luck and morale icon frames. Same gate, same reason.
    void updateStats();
    // Gated to hero.obj's own view: these five are used only inside
    // hero.cpp, and declaring them unconditionally moved an UNRELATED
    // compiland's score (recruitUnit::Update 90.84 -> 88.24) through the
    // include-set sensitivity class with no semantic change anywhere.

    // 0x4e2550, RETAIL-ONLY (no DC row), `ret 8`: the actual equip
    // attempt HeroFn_004E2840 wraps. ORDINAL PLACEHOLDER.
    unsigned char heroFn004E2550(long artifact, long slot);
    // 0x4e2840, RETAIL-ONLY (no DC row), `ret 8`: decides whether the
    // artifact being dragged may drop into an equipment slot.
    // THeroScreenWindow::update_slot calls it THISCALL on gpCurrentHero
    // with both ids on the stack. It is NOT DC's artifactAllowedInSlot
    // (dc 0x37d88, an artifact.h FREE inline of 44 B already
    // reconstructed in ai_player.cpp) - retail's is a 412-byte hero
    // member. ORDINAL PLACEHOLDER name.
    unsigned char heroFn004E2840(long artifact, long slot);
    void upgradeCreatures(int sourceCreatureType, int destCreatureType);
    // The mobility pair at 0x4e4990 / 0x4e4d90: the no-arg form reads
    // the boat bit out of `flags` and forwards to the other.
    // Dreamcast hero.cpp:5709/5734; ordinary movement helpers expanded here.
    // Original names: GetLogisticsFactor, GetNavigationFactor.
    float getLogisticsFactor() const;
    // 0x4e5550 - checks spell access, mana, boat reachability and pool space.
    unsigned char canSummonBoat() const;
    long getNavigationFactor() const;
    int getMobility(unsigned char seaMovement) const;
    int getMobility() const;
    // 0x4e5960 - the four primary skills, each clamped to 0..99, with
    // slots 2 and 3 floored at 1.
    short getPrimarySkillTotal() const;
    // 0x4e59a0 - enables overland flight and charges its terrain-adjusted
    // per-mastery mana cost.
    void fly(int level);
    // 0x4e5dd0 - one-argument setter for waterWalkLevel.
    void walkOnWater(int level);
    // 0x4e5e10 - tests whether a packed map point is inside Visions range.
    unsigned char isInIdentifyRange(const type_point* location) const;

private:
    // 0x4e5ce0 - checks terrain, passability and blocking trigger objects.
    unsigned char canLand() const;

public:
    int heroFn004E5DE0() const;
    void heroFn004E6120(int creatureType,
                         TCreatureTypeTraits* traits) const;
    // 0x4d9050 / 0x4e56b0, the two owner-record accessors; both open
    // with the same `owner < 0` guard.
    unsigned char belongsToHuman() const;
    class playerData* getPlayer() const;
    // 0x4e5330 / 0x4e5380, the two status-bar gauge frames.
    int getMobilityFrame() const;
    int getManaFrame() const;
    static int getExperience(int level);
    // 0x4da420 - STATIC for the same two reasons, and its whole body is
    // GetExperience inlined twice.
    static int getExperienceIncrement(int level);
    // 0x4e4390 - Estates gold per day, the int twin of the five
    // specialty-factor getters.
    int getEstatesBonus() const;
    long getHitPointBonus(int creatureType) const;
    // The three backpack primitives at 0x004dbd90 / 0x004dbdb0 /
    // 0x004dbe10; all three walk `backpack` above.
    long getLastBackpackIndex() const;
    // 0x004e56e0 - the patrol test: Manhattan distance from the patrol
    // anchor, same level, against patrolRadius.
    unsigned char isInPatrolRadius(struct type_point point) const;
    // 0x004e2d50 - the fourth backpack primitive; closes the hole a
    // removed slot leaves and drops `backpackCount`.
    void removeBackpackArtifact(short slot);
    void rotateBackpackLeft();
    void rotateBackpackRight();
    void applyBattleWinTemps();
    void applyBattleLossTemps();
    // 0x004e2bd0 - unequips ONE equipped slot, by slot INDEX. The
    // Dreamcast spells the parameter `TArtifactSlot`, but on that build
    // TArtifactSlot is an ENUM of the wearable positions
    // (eArtifactSlotHead 0 .. eArtifactSlotSpellbook 17,
    // kNumArtifactSlots 18) - NOT the 8-byte slot record this header
    // already binds that name to. Retail's caller pushes a bare loop
    // counter, so the index spelling is what compiles; the DC enum is
    // recorded here rather than modelled, because retail scans NINETEEN
    // slots, one more than the DC roster's eighteen.
    void removeArtifact(long slot);
    // 0x004e2a00 - equips an artifact record into an ordinal slot;
    // negative slot selects the first legal position.
    unsigned char equipArtifact(const type_artifact* artifact, long slot);
    // 0x004dc070 - disassembles the combination artifact in one equipped
    // slot, then equips each component into its first legal position.
    void heroFn004DC070(long slot);
    // 0x004d9260 - drops the artifact backing a war machine when the
    // machine dies.
    void destroySiegeWeaponArtifact(int creatureType);
    // 0x004d92d0 - spends mana and refreshes the local adventure hero
    // locators while that manager is active.
    void useSpell(int cost);
    // 0x004d7890 - consumes this hero from one player's tavern offers,
    // charges the standard gold cost and places the hero on the map.
    void hire(int playerId, type_point point);
    // E:\gamedcs\Hero.h:334, dc 0x1fbc8. DrawHeroPart and its shadow
    // twin call this header helper at each sprite draw; retail folds the
    // branchless facing > 4 body into the caller.
    unsigned char getHflip()
    {
        return m_facing > kFacingS;
    }
    // 0x004d9110 - the idle frame for the hero's current facing.
    hero_seqid getStandSequence();
    // 0x004e2f90 - inserts an artifact into the backpack, shifting the
    // tail up when the requested slot is occupied. `slot` < 0 means
    // "first free".
    unsigned char addToBackpack(const type_artifact* artifact, long slot);
    std::string getBackpackError(TArtifact artifact) const;
    // 0x004e3070 - gives or equips one artifact and performs the optional
    // end-condition check. ProcessSearch calls it for the Holy Grail.
    // SIGNATURE CORRECTED FROM RETAIL (2026-08-20): `ret 0xc`, and BOTH
    // flags are read as BYTES (`mov al,[ebp+0xc]` / `mov al,[ebp+0x10]`)
    // against the dword reads GiveExperience's two `int` parameters take
    // in its now-exact body; the inlined allocator temporary lives at
    // [ebp+0xf], inside parameter 2's home, which only exists as padding
    // if that parameter is one byte wide. The 0x4e3bf8 exit is
    // `mov al,1`, so the return is an 8-bit value, not void. The DC row
    // declares `void ... int bCheckEnd, unsigned char equip_it`; retail's
    // SECOND flag gates the combination announcement and its THIRD gates
    // CheckForArtifactWin, so the DC names do not carry over. `bAnnounce`
    // is an invented spelling for a byte-proven role.
    unsigned char giveArtifact(const type_artifact* artifact,
                               unsigned char announce,
                               unsigned char checkEnd);
    const char* heroFn004D8F70();
    // (?VisitedArena@hero@@QBA_NPBVNewmapCell@@@Z) gives the const and
    bool visitedArena(const NewmapCell* cell) const;
    void setVisitedArena(const NewmapCell* cell);
    unsigned char isWieldingArtifact(int whichArtifact) const;
    // 0x004e2dd0 - the by-id overload: finds the artifact in the
    // backpack first, then in the equipped slots, and unequips it.
    unsigned char removeArtifact(TArtifact artifact);
    // 0x004e23d0 - drains another hero's equipped slots and backpack
    // into this hero's backpack.
    void transferArtifacts(hero* src);
    // 0x4e5f30 - "this hero can still be given an order this turn".
    // Declared for playerData::NextHero, which inlines nothing of it -
    // it is a real call from game.obj.
    unsigned char isMobile() const;
    const char* getSpecificAbilityTextShort();
    int valueOfSpell(SpellID spell) const;
    std::basic_string<char, std::char_traits<char>, std::allocator<char> >
        getLuckDescription() const;
    // hero.obj OWNS both definitions (0x4dc320 / 0x4dcac0).
    std::basic_string<char, std::char_traits<char>, std::allocator<char> >
        getMoraleDescription() const;
    int getLuck(const hero* otherHero, unsigned char onCursedGround,
                unsigned char applyLimits) const;
    int getMorale(const hero* otherHero, unsigned char onCursedGround,
                  unsigned char applyLimits) const;
    int moraleIncreaseValue(int value);
    int luckIncreaseValue(int value);
    int soDGetSeerSkillValue(int skill, int level);
    int getSpellDurationBonus() const;
    int giveExperience(int howMuch, int checkForLevelUp,
                       unsigned char showCapWindow);
    void giveResource(int whichRes, int howMuch);
    int getVisibility() const;
    float getMagicResistanceFactor() const;
    // The rest of the specialty factor family, all one shape (see the
    // note over GetOffenseFactor in src/hero.cpp): 0x4e42b0 / 0x4e4310 /
    // 0x4e48b0 / 0x4e4920.
    float getArcheryFactor() const;
    float getEagleEyeChance() const;
    // 0x4e4840, claimed in hero.cpp - cmbtmgr's
    // CalculateGainedExperience (0x46a350) scales the whole award by it
    // with a single-precision fmul.
    float getExperienceBonusFactor() const;
    int getMysticismBonus() const;
    TAdventureObjectType heroFn004E4EC0();
    long getCombatSpeedBonus() const;
 // +0x430
    // Header inline at E:\gamedcs\Hero.h:634 (dc 0x669fc). Dreamcast's
    // xrefs put direct calls in both hero-screen functions and the combat
    // sub-window update; the retail sites expand it byte-for-byte under
    // /Ob2. Keep the recovered header helper canonical for every consumer.
    int getMaxMana() const
    {
        return static_cast<int>(
            getPrimarySkill(3) * 10 * getIntelligenceFactor());
    }
    // Hero.h source helpers retained as calls by Dreamcast and expanded in
    // Complete's AI_AttemptMove and cursor movement family. check_terrain is
    // honored by the Hero.h:645/654 can_land call. The movement driver
    // uses checkTerrain=1; Complete expands the helper before calling canLand.
    // These are real shared header bodies, not an ai_player.obj declaration
    // view: cursor.obj proves the same nested IsWieldingArtifact boundary.
    __forceinline unsigned char isFlying(unsigned char checkTerrain) const
    {
        return !(m_flags & 0x40000)
            && (m_flightLevel != -1 || isWieldingArtifact(0x48))
            && (!checkTerrain || !canLand());
    }
    __forceinline unsigned char canWalkOnWater(unsigned char checkTerrain) const
    {
        return !(m_flags & 0x40000)
            && (m_waterWalkLevel != -1 || isWieldingArtifact(0x5a))
            && (!checkTerrain || !canLand());
    }
    // E:\gamedcs\Hero.h:664. Dreamcast emits this header helper from
    // overview.obj. Complete emits no standalone wrapper; ProcessIconSelect
    // expands it and retains the underlying get_obscured_town call.
    inline town* getOccupiedTown()
    {
        return getObscuredTown();
    }
    VA(0x005bde40, 0x31)  // exact body + sole caller above, dc 0x2c668
    int getPrimarySkill(int skill) const
    {
        signed char value = m_stats[skill];
        if (value > 99)
            return 99;
        if (value > 0)
            return value;
        return skill >= 2 ? 1 : 0;
    }
                       // +0x476
    void obscureCell()
    {
        type_obscuring_object::obscureCell(HERO, m_id);
    }
    // DC-attested header inline (E:\gamedcs\hero.h:687, dc 0x70a1c, 16
    // SH4 bytes, params `skill` and `amount` both T_INT4) - and its own
    // command.obj attribution is the reason it is written out here:
    // combatManager::DoVictory hands it three combatManager ints, and
    // retail loads each of them with a full `mov r32, dword ptr [..]`
    // before the byte store. A direct `stats[i] = field` narrows the
    // load to `mov r8, byte ptr [..]` instead; the int PARAMETER is what
    // keeps the dword.
    void setPrimarySkill(int skill, int amount) { m_stats[skill] = amount; }
    // `?AdjustPrimarySkill@hero@@QAAXHH@Z`, a Hero.h inline the Dreamcast
    // build calls OUT OF LINE and retail's /Ob2 expands. The DC line table
    // for advManager::DoEventLibrary (dc 0x93bf8) is what found it: source
    // lines 2120..2123 are FOUR separate statements and the first resolves
    // to this decoration, the other three reusing its pooled address - so
    // the Library of Enlightenment's four +2 awards are four CALLS in the
    // source, not four `stats[i] += 2` expressions.

    // The retail x86 bytes corroborate the shape from the other side: the
    // expansion is `mov al,2` ONCE followed by `add dl,al` / `add cl,al`,
    // i.e. the amount materialised in a register and reused, which is what
    // an inlined int PARAMETER produces and not what a literal in an `+=`
    // does. No clamp - the byte read-modify-write is all there is.
    void adjustPrimarySkill(int skill, int amount) { m_stats[skill] += amount; }
    // Original ViewStat is the ordinary hero.cpp:1709 body at 0x4d9990.
    void viewStat(int whichStat, int isQuickView);
    int giveSS(int whichSS, int numLevelsToGive);
    // The secondary-skill trio at 0x4e2210 / 0x4e2250 / 0x4e22d0; SetSS
    // dispatches to the other two, so all three need the declaration.
    void setSS(int whichSS, int levelToSet);
    int takeSS(int whichSS, int numLevelsToTake);
    int creatureTypeCount(int creatureType);
    int getNthSS(int which);
    float getSurrenderCostFactor() const;
    float getOffenseFactor() const;
    float getDefenseFactor() const;
    float getIntelligenceFactor() const;
    float getFirstAidFactor() const;
    int getSpecialTerrain() const;
    // Claimed in src/hero.cpp (0x4e5760 / 0x4e5ff0); declared here so
    // ai_combat's inlined get_spell_damage / get_resurrection_value can
    // call them.
    long modifySpellDamage(int spell, int damage, const class army* targetArmy) const;
    TSkillMastery getSpellLevel(SpellID spell, int magicTerrain) const;
    TSkillMastery getSpellSchoolLevel(TSpellSchool schoolMask,
                                      int magicTerrain) const;
    TSpellSchool getHighestSchool(TSpellSchool schoolMask) const;
    int getManaCost(int whichSpell, const class armyGroup* enemy,
                    int magicTerrain) const;
    float getCombatValueModifier() const;
    // Original: hero::HasArmy; Hero.h:702, dc 0xd58f8.
    // The header helper used by GetManaCost's own-stack discount. Complete
    // folds it back to the same armyGroup::IsMember bytes.
    unsigned char hasArmy(TCreatureType type) const
    {
        return m_army.isMember(type);
    }
    // Original: hero::GetManaCost; Hero.h:707, dc 0x23058.
    int getManaCost(int whichSpell) const
    {
        return getManaCost(
            whichSpell, 0,
            getSpecialTerrain());
    }
    // Original: hero::get_spell_level; Hero.h:718, dc 0x2308c.
    TSkillMastery getSpellLevel(SpellID spell) const
    {
        return getSpellLevel(spell, getSpecialTerrain());
    }
    float getNecromancyFactor(unsigned char applyLimit) const;
    int getHeroSpellBonus(int spellId, int targetLevel, int value) const;
    // E:\gamedcs\Hero.h:724. Dreamcast retains this header helper as a
    // standalone inline body. Complete stores the resolved sex on the live
    // hero and expands this test at its spells.cpp caller.
    unsigned char isMale() const
    {
        return m_sex == 0;
    }
    // DC-attested inline accessor (ai_combat.h roster, dc 0x2c690).
    // Retail has no out-of-line row; AI_value_of_combat expands this
    // one-field return and preserves its float temporary before widening.
    float getAggression() const { return m_aggression; }
    // Original: hero::get_artifact; Hero.h:965, dc 0x27e8c.
    // LF_MFUNCTION returns const type_artifact&, with TArtifactSlot as the
    // equipped-slot domain. UI-decoded indices convert to that domain at use.
    const type_artifact& getArtifact(TArtifactSlot slot) const
    {
        return m_equipped[slot];
    }
    // Original: hero::get_backpack; Hero.h:970, dc 0x27e9c.
    const type_artifact& getBackpack(long slot) const
    {
        return m_backpack[slot];
    }
    // E:\gamedcs\Hero.h:976. Dreamcast keeps this const header wrapper as
    // a separate public; Complete folds it at each use into the retail-proven
    __forceinline int getExperienceIncrement() const
    {
        return getExperienceIncrement(m_level);
    }
    // E:\gamedcs\Hero.h:981. Dreamcast records this exact typed boundary;
    // the byte-backed skillLevel array is widened by the inline return.
    TSkillMastery getSecondarySkill(TSecondarySkill skill) const
    {
        // skillLevel is the packed byte-backed persistence array.  The
        // original typed facade necessarily widens that stored ordinal back
        // into its CodeView-proven enum domain at this boundary.
        return TSkillMastery(m_skillLevel[skill]);
    }
    // E:\gamedcs\Hero.h:986, dc 0x1fd30. SetHeroContext preserves this
    // header-inline helper in source. Retail folds its packed-point
    // construction into SetHeroContext and move_hero, so the canonical
    // declaration belongs to hero rather than either TU's private view.
    type_point getTarget() const
    {
        return type_point(m_pathTargetX, m_pathTargetY, m_pathTargetZ);
    }

    // DC hero.h:991 (0x37dc4) and the class signature record the const
    // long-returning duration accessor used by AI reward valuation.
    long getValueOfDuration() const { return m_valueOfDuration; }
    __forceinline long getValueOfKnowledge() const
    {
        return m_valueOfKnowledge;
    }
    // Dreamcast hero.h:1001/1006. Retail folds both one-field accessors into
    // get_skill_value; retaining the source boundaries still emits the direct
    // loads proved at +0x47e/+0x486.
    __forceinline long getValueOfPower() const
    {
        return m_valueOfPower;
    }

    // DC hero.h:1006/1011, dc 0x114b88/0x114b90; each is one
    // cached-value load, also expanded by the retail philai callers.
    long getValueOfSpring() const { return m_valueOfSpring; }

    long getValueOfWell() const { return m_valueOfWell; }
    // DC hero.h:1016, dc 0x37ddc. The const-bool mangling
    // (?is_in_spellbook@hero@@QBA_NW4SpellID@@@Z) and CastSpell's direct
    // retail byte load prove this is a source-visible header inline.
    bool isInSpellbook(SpellID spell) const
    {
        return m_inSpellbook[spell] != 0;
    }
    // Dreamcast keeps these five source-visible setter boundaries. Retail
    // /Ob2 folds them into AI_set_hero_bonuses, but the calls remain
    // authoritative source shape rather than anonymous stores.
    // E:\gamedcs\Hero.h:1021
    __forceinline void setValueOfDuration(long arg)
    {
        m_valueOfDuration = arg;
    }
    // E:\gamedcs\Hero.h:1026
    __forceinline void setValueOfKnowledge(long arg)
    {
        m_valueOfKnowledge = arg;
    }
    // E:\gamedcs\Hero.h:1031
    __forceinline void setValueOfPower(long arg)
    {
        m_valueOfPower = arg;
    }
    // E:\gamedcs\Hero.h:1036
    __forceinline void setValueOfSpring(long arg)
    {
        m_valueOfSpring = arg;
    }
    // E:\gamedcs\Hero.h:1041
    __forceinline void setValueOfWell(long arg)
    {
        m_valueOfWell = arg;
    }
    // DC-attested inline helper; SetShrineHelpText proves the direct
    // byte-indexed availability read in retail.
    unsigned char spellIsAvailable(int spell) const
    {
        return m_availableSpells[spell];
    }
    TCreatureType getNecromancyCreature();
    const char* heroFn004D8FB0();
    unsigned char heroFn004DBE80(int combination);
    // Same gate and same reason as the equip pair above.
    // 0x4dbf30, the two-argument member of the combination family:
    // strips every worn component of `combination` (plus whatever sits
    // in `slot`) and equips the assembled artifact. ORDINAL PLACEHOLDER.
    unsigned char heroFn004DBF30(int combination, long slot);
    // 0x4dc100, the family's NOTIFIER: called after a slot changes, it
    // records the assembled combination the artifact belongs to, or -
    // when every component of a combination is now worn - offers the
    // assembly through a NormalDialog and calls HeroFn_004DBF30 on yes.
    // ORDINAL PLACEHOLDER.
    void heroFn004DC100(long slot);
    boat* findSummonableBoat() const;
    void placeInMap(int playerId, type_point point, unsigned char resetFlags);
    int load(TAbstractFile* infile, int saveVersion);
    int save(TAbstractFile* outfile);

};
// sizeof(hero) == 1170 (0x492), byte-proven THREE independent ways:
//   - the save walk at 0x4be841 runs `lea edi,[gpGame+0x21620]` and
//     then `add edi,0x492` / `inc eax` / `cmp eax,0x9c` / `jb` - a
//     156-iteration stride-0x492 sweep of the hero array, which pins
//     the element size and the array bound in one loop;
//   - five town.obj bodies index `gpGame + 0x21620 + 1170*id` with the
//     shl 6 / add / lea x8 / lea x2 chain (65*9*2 == 1170);
//   - two `push 0x492` / `call operator new` sites at 0x4af225 and
//     0x4af26c, each immediately followed by `call hero::hero`.
// 0x548 is army, not hero: it has ZERO `operator new` sites image-wide.
SIZE(hero, 0x492);

#pragma pack(pop)

// THeroTraits - the per-hero static-traits record, 92 B stride
// byte-proven by strip::DrawOwner 0x5aa060/0x5aa230-adjacent bodies:
// akHeroTraits[frame] is addressed as frame*23 dwords, and the +0x34
// dword rides a WIDGET_SET_IMAGE message, i.e. an image-name string
// (Dreamcast and NH3API name it m_large_portrait_name). The recovered
// members below follow their consumers and reference layouts; the PC-only
// four-byte slot at +0x3c remains unresolved.
struct THeroTraits {
public:
    int m_sex;
                        // +0x00 (DC m_sex)
    int m_race;
                       // +0x04 (DC m_race)
    THeroClass m_heroClass;
           // +0x08 (DC m_class)
    int m_firstSkill;
                 // +0x0c (TSecondarySkill)
    int m_firstSkillLevel;
            // +0x10 (TSkillMastery)
    int m_secondSkill;
                // +0x14 (TSecondarySkill)
    int m_secondSkillLevel;
           // +0x18 (TSkillMastery)
    unsigned char m_startsWithSpellbook;
 // +0x1c
    // Dreamcast m_startsWithSpellbook is one byte at +0x1c, followed
    // by m_startingSpell at +0x20. Retail uses the byte flag; the intervening
    // three bytes align the spell ID, despite NH3API widening the flag to bool32.
    char m_paddingBeforeStartingSpell[3];
    int m_startingSpell;
              // +0x20 (SpellID)
    TCreatureType m_firstStack;
       // +0x24
    TCreatureType m_secondStack;
      // +0x28
    TCreatureType m_thirdStack;
       // +0x2c
    // UpdateHeroLocator sends this pointer to the portrait widget. Dreamcast
    // independently names the same +0x30 member m_small_portrait_name.
    const char* m_smallPortraitName;
  // +0x30 image name for locator portraits
    const char* m_largePortraitName;
  // +0x34 image name for WIDGET_SET_IMAGE
    unsigned int m_attributes;
        // +0x38 (DC name)
    char m_pad3c[4];
                 // +0x3c retail-only field
    // HeroFn_004D8FB0 strcmp's the live hero name against this pointer.
    // InitializeHeroTraitsTable independently fills it from hotraits.txt.
    const char* m_defaultName;
         // +0x40
    // Retail parses columns 1/2, 4/5, and 7/8 into these six dwords;
    // Dreamcast independently names the same three low/high stack pairs.
    int m_firstStackLow;
               // +0x44
    int m_firstStackHigh;
              // +0x48
    int m_secondStackLow;
              // +0x4c
    int m_secondStackHigh;
             // +0x50
    int m_thirdStackLow;
               // +0x54
    int m_thirdStackHigh;
              // +0x58
};
SIZE(THeroTraits, 0x5c);

// Retail's eighteen 64-byte hero-class rows. The class-name getter reaches
// only the pointer at +4; cursor rendering independently proves the eighteen
// class extent.
struct THeroClassTraits {
public:
    int m_townType;
                       // +0x00
    const char* m_className;
              // +0x04
    float m_aggression;
                   // +0x08
    signed char m_initialPrimarySkill[4];
 // +0x0c
    signed char m_gainPrimarySkillChance[4];
    // +0x10
    signed char m_gainPrimarySkillChance10P[4];
 // +0x14
    signed char m_gainSecondarySkillChance[28];
 // +0x18
    signed char m_foundInTownType[9];
      // +0x34
    // Complete expands foundInTownType to nine bytes at +0x34.
    // NH3API confirms the three trailing alignment bytes and 0x40-byte PC stride.
    char m_paddingAfterTownChances[3];

};
SIZE(THeroClassTraits, 0x40);

// Dreamcast names this public aggregate and its retail producer preserves the
// exact layout: 21 land-speed entries, four Navigation masteries, then five
// movement bonuses. Complete keeps the Stables bonus in the preceding cell.
struct type_movement_constants {
public:
    int m_land[21];
    int m_sea[4];
    int m_equestriansGlovesBonus;
    int m_bootsOfSpeedBonus;
    int m_oceanGuidanceBonus;
    int m_seaCaptainsHatBonus;
    int m_lighthouseBonus;

};
SIZE(type_movement_constants, 0x78);
extern type_movement_constants g_moveConstants;
extern int g_landMovement[21];
DATA(0x0067d868) extern THeroClassTraits g_heroClassTraits[18];
extern const THeroClassTraits (&g_heroClasses)[18];

// Retail .data 0x67dce8 (reloc-evidence datum; read by strip::DrawOwner
// as pointer+index). The IDA-lineage mangling
// ?akHeroTraits@@3AAY0KD@$$CBUTHeroTraits@@A types it as an array
// reference. Retail's parser settles the Complete bound: its direct
// 0x5c-stride walk covers exactly 156 rows. The reference cell's retail
// value is 0x679dd0; the loader begins at the +0x40 defaultName field.
DATA(0x00679dd0) extern THeroTraits g_heroTraitsStorage[156];
DATA(0x0067dce8) extern const THeroTraits (&g_heroTraits)[156];

// --- globals ---
// CODEVIEW(E:\gamedcs\hero.cpp:267, dc 0xca7e8) unsigned char initialize_move_constants();
// Complete returns the 70-bit grant set by value; game::LoadMap consumes it
// when a scenario disables a spell supplied by an artifact.
// Provisional name: markArtifactSpells. The former mark_spells label belongs
// to hero.cpp's school helper, whose DC signature is recorded below.
std::bitset<70> markArtifactSpells(int artifactId);
// CODEVIEW(E:\gamedcs\hero.cpp:1527, dc 0xcc360) void mark_spells(unsigned char* spell_list, TSpellSchool school);
// CODEVIEW(E:\gamedcs\hero.cpp:2014, dc 0xccf78) TSecondarySkill get_skill_award(const hero* current_hero, TSkillMastery min_level, TSkillMastery max_level, TSecondarySkill excluded);
// CODEVIEW(E:\gamedcs\hero.cpp:2340, dc 0xcd68c) void update_artifact_slot(long id, TArtifact artifact);
// CODEVIEW(E:\gamedcs\hero.cpp:2393, dc 0xcd76c) void UpdateBackpackItem(int i);
// CODEVIEW(E:\gamedcs\hero.cpp:2726, dc 0xcdf30) void handle_artifact_click(long code, unsigned char right_mouse);
// CODEVIEW(E:\gamedcs\hero.cpp:3181, dc 0xcea3c) void handle_backpack_click(long code, unsigned char right_mouse);
int heroView(int heroID, int noDismiss, int alreadyFaded,
             unsigned char quickView);

// --- CMCDeadHero ---
// CODEVIEW(E:\gamedcs\netmsg.h:675, dc 0xd5964) void CMCDeadHero::CMCDeadHero(signed char heroId, type_point point);

// --- SCampaign ---
// CODEVIEW(E:\gamedcs\CustomCampaign.h:225, dc 0xd5944) int SCampaign::GetExpCap();

// --- THeroScreenWindow ---
// Retail hero-screen state. The first datum is an actual type_artifact:
// its adjacent dword is initialized to -1 by the same static initializer,
// and every artifact-drag path treats the pair as one artifact record.
// The second is the selected army slot used by the hero-screen message
// paths. Both spellings are role-derived because no retail symbols survive.
DATA(0x00698a88) extern type_artifact g_heroScreenDraggedArtifact;
DATA(0x00697738) extern int g_heroScreenArmySlot;
// HeroView stores GetLocalPlayer()->FindHero(gpCurrentHero->id) here before
// SetupHeroView. UpdateHeroLocator compares it with topHero + locator index.
DATA(0x00698b20) extern hero* g_currentHero;
// HeroView's second argument, stashed on entry (0x4e1809 stores EDX
// straight into this cell). SetupHeroView reads it as the "dismiss button
// stays dead" latch, a full DWORD. The name is role-derived from
// HeroView's own parameter and is PROVISIONAL.
DATA(0x00698a84) extern int g_heroScreenHeroPosition;
// HeroView's FIRST argument, stashed on entry beside the one above
// (0x4e1805 stores ECX straight into this cell). Role-derived from that
// parameter and PROVISIONAL for the same reason.
DATA(0x00698a90) extern int g_heroScreenNoDismiss;
DATA(0x00698a50) extern int g_heroScreenHeroId;

// The vtable and destructor prove direct CAdvPopup inheritance. Complete
// carries nineteen equipped positions, one more than the DC TArtifactSlot
// roster; keep the refresh argument as the retail-proven ordinal index.
// The constructor also touches a still-unmodelled two-dword retail tail;
// no method admitted here relies on those fields.
class THeroScreenWindow : public CAdvPopup {
public:
    enum EArtifactSlotBounds {
        ARTIFACT_SLOT_FIRST = 0,
        ARTIFACT_SLOT_COUNT = 19
    };
    // The hero screen's widget ids, as UpdateHeroScreenStatusBar's switch
    // surfaces them in message::codeY. Decoded from the three dispatch
    // nodes at 0x4db682 / 0x4dba4f / 0x4dbbb0 and their byte-index and
    // jump tables at 0x4dbce4 / 0x4dbcbc / 0x4dbd58. Enumerated
    // exhaustively even across consecutive runs, the
    // TCombatOptionsWindow::*_ID precedent - and required, because raw
    // hex `case` labels trip the magic-case-label cleanliness floor.
    // Ids with no attested role keep an ORDINAL PLACEHOLDER spelling: the
    // three id-triplets below are structurally "three widgets share one
    // rollover row" (icon/label/value), but nothing in the bytes says
    // which triplet is which.
    enum EHeroScreenWidgetId {
        ARTIFACT_SLOT_0_ID = 0x02,  ARTIFACT_SLOT_1_ID,  ARTIFACT_SLOT_2_ID,
        ARTIFACT_SLOT_3_ID,  ARTIFACT_SLOT_4_ID,  ARTIFACT_SLOT_5_ID,
        ARTIFACT_SLOT_6_ID,  ARTIFACT_SLOT_7_ID,  ARTIFACT_SLOT_8_ID,
        ARTIFACT_SLOT_9_ID,  ARTIFACT_SLOT_10_ID, ARTIFACT_SLOT_11_ID,
        ARTIFACT_SLOT_12_ID, ARTIFACT_SLOT_13_ID, ARTIFACT_SLOT_14_ID,
        ARTIFACT_SLOT_15_ID, ARTIFACT_SLOT_16_ID, ARTIFACT_SLOT_17_ID,
        ARTIFACT_SLOT_18_ID,
        BACKPACK_SLOT_0_ID = 0x28, BACKPACK_SLOT_1_ID, BACKPACK_SLOT_2_ID,
        BACKPACK_SLOT_3_ID, BACKPACK_SLOT_4_ID,
        PORTRAIT_ID = 0x2d,
        PRIMARY_SKILL_0_ID = 0x32, PRIMARY_SKILL_1_ID, PRIMARY_SKILL_2_ID,
        PRIMARY_SKILL_3_ID,
        ARMY_SLOT_0_ID = 0x44, ARMY_SLOT_1_ID, ARMY_SLOT_2_ID,
        ARMY_SLOT_3_ID, ARMY_SLOT_4_ID, ARMY_SLOT_5_ID, ARMY_SLOT_6_ID,
        // The two backpack pagers, byte-proven by WindowHandler: its
        // WIDGET_DESELECT arms for these two ids expand
        // rotate_backpack_left / rotate_backpack_right respectively.
        BACKPACK_SCROLL_LEFT_ID = 0x4d, BACKPACK_SCROLL_RIGHT_ID = 0x4e,
        SKILL_ICON_FIRST_ID  = 0x4f, SKILL_ICON_LAST_ID  = 0x56,
        SKILL_NAME_FIRST_ID  = 0x57, SKILL_NAME_LAST_ID  = 0x5e,
        SKILL_LEVEL_FIRST_ID = 0x5f, SKILL_LEVEL_LAST_ID = 0x66,
        WIDGET_6B_ID = 0x6b, WIDGET_6C_ID = 0x6c, WIDGET_6D_ID = 0x6d,
        WIDGET_70_ID = 0x70, WIDGET_71_ID = 0x71,
        STATUS_BAR_BORDER_ID = 0x72,
        STATUS_BAR_ID = 0x73,
        MORALE_ID = 0x74,
        LUCK_ID = 0x75,
        WIDGET_76_ID = 0x76, WIDGET_77_ID = 0x77, WIDGET_78_ID = 0x78,
        WIDGET_7A_ID = 0x7a, WIDGET_7C_ID = 0x7c,
        FORMATION_ID = 0x7e,
        MIXED_ARMY_ID = 0x7f,
        WIDGET_80_ID = 0x80,
        HERO_NAME_ID = 0x81,
        WIDGET_8B_ID = 0x8b,
        // The eight hero locators down the right edge. WindowHandler's
        // second jump table routes exactly 0x82..0x89 to one arm that
        // indexes the local player's hero list by `topHero + (codeY -
        // 0x82)`, and SetupHeroView's own locator loop runs 0..7.
        HERO_LOCATOR_0_ID = 0x82, HERO_LOCATOR_1_ID, HERO_LOCATOR_2_ID,
        HERO_LOCATOR_3_ID, HERO_LOCATOR_4_ID, HERO_LOCATOR_5_ID,
        HERO_LOCATOR_6_ID, HERO_LOCATOR_7_ID,
        WIDGET_7800_ID = 0x7800
    };
    // The "no army slot selected" sentinel gHeroScreenArmySlot carries.
    enum { HERO_SCREEN_NO_ARMY_SLOT = -1 };
    // The hero::formation bit the status bar tests (`test byte
    // [hero+0x48],2`). Bit 1 for a nominally 0/1 field is unexplained;
    // transcribed as retail emits it.
    enum { HERO_FORMATION_GROUPED = 2 };
    // Bit 0 of the same field. WindowHandler's WIDGET_DESELECT arms clear
    // it for widget 0x7a and set it for 0x7c - the tight/loose pair -
    // where FORMATION_ID (0x7e) toggles GROUPED above. SetupHeroView
    // already reads this bit, with a bare literal.
    enum { HERO_FORMATION_TIGHT = 1 };
    // Constructor-initialized hero-list scroll origin. Locator i displays
    // localPlayer->heroes[topHero + i].
    int m_topHero;
                    // +0x60
    // +0x64. Retail constructor 0x4de7e6 reads the vector end, then
    // dereferences end-1 before the first widget push. The base constructors
    // do not populate the vector, so this does not establish a background
    // or last-base-widget role. Keep the semantic name unresolved.
    widget* m_field64;
    THeroScreenWindow();
    virtual ~THeroScreenWindow();
    virtual int windowHandler(class message* msg);
    void updateSlot(TArtifactSlot slot);
    void updateAllSlots();
    void updateHeroScreenStatusBar(class message* msg);
    void updateHeroLocator(int which);
    void updateHeroLocators();
    // 0x4e1a50, thiscall with no arguments and NOT virtual (absent from
    // vtable 0x63eae8). It dereferences no THeroScreenWindow member at
    // all - `this` only ever feeds member calls - so the layout above
    // needs nothing for it.
    void setupHeroView();
    virtual int exitDialog(class message* msg);

};
SIZE(THeroScreenWindow, 0x68);

// The live hero-screen window. HeroView (0x4e1800) stores its
// `new THeroScreenWindow` result here (0x4e181c) before calling
// SetupHeroView, and every hero-screen updater in this compiland
// broadcasts through it - 49 retail references, all inside hero.obj's
// hero-screen block plus hero::hero's clear. Spelling role-derived; no
// public symbol survives for it.
// Retail 0x698a44, the hero screen's "army strip is live" flag. HeroView
// clears it on entry; the army repaint at 0x4d97f0 uses it three ways -
// it decides whether an EMPTY slot's widget is drawn at all, and it
// gates both selection-highlight arms of an occupied slot. Role
// inferred from those three reads; ORDINAL PLACEHOLDER name.
DATA(0x00698a78) extern THeroScreenWindow* g_heroScreenWindow;
DATA(0x00698a44) extern int g_heroScreenArmyStripLive;

// CODEVIEW(E:\gamedcs\hero.cpp:1594, dc 0xcc49c) void THeroScreenWindow::HeroMessageUpdate(char* cText);
// CODEVIEW(E:\gamedcs\hero.cpp:3239, dc 0xcec1c) void THeroScreenWindow::ShowWidgets();
// CODEVIEW(E:\gamedcs\hero.cpp:3421, dc 0xcf3ac) void THeroScreenWindow::show_skills();
// CODEVIEW(E:\gamedcs\hero.cpp:3486, dc 0xcf54c) int THeroScreenWindow::WindowHandler(message* msg);
// CODEVIEW(E:\gamedcs\hero.cpp:4231, dc 0xd2d24) void THeroScreenWindow::UpdateHeroLocators();
// CODEVIEW(E:\gamedcs\hero.cpp:4186, dc 0xd59d0) void* THeroScreenWindow::`scalar deleting destructor'(unsigned __flags);

// --- boat ---

// --- hero ---
// CODEVIEW(E:\gamedcs\hero.cpp:254, dc 0xca7c0) const char* hero::GetSpecificAbilityText();
// CODEVIEW(E:\gamedcs\hero.cpp:577, dc 0xcaf98) int hero::load(void* infile);
// CODEVIEW(E:\gamedcs\hero.cpp:1208, dc 0xcbdb8) void hero::hero();
// CODEVIEW(E:\gamedcs\hero.cpp:1233, dc 0xcbe80) void hero::initialize(short index);
// CODEVIEW(E:\gamedcs\hero.cpp:1466, dc 0xcc2a8) void hero::DestroySiegeWeaponArtifact(int creature_type);
// CODEVIEW(E:\gamedcs\hero.cpp:1632, dc 0xcc540) void hero::UpdateArmies();
// CODEVIEW(E:\gamedcs\hero.cpp:1862, dc 0xccc8c) int hero::GetLevel(int iExperience);
// CODEVIEW(E:\gamedcs\hero.cpp:2147, dc 0xcd17c) void hero::CheckLevel();
// CODEVIEW(E:\gamedcs\hero.cpp:2849, dc 0xce260) std::basic_string<char,std::char_traits<char>,std::allocator<char> hero::get_morale_description(__$ReturnUdt);
// CODEVIEW(E:\gamedcs\hero.cpp:3021, dc 0xce648) std::basic_string<char,std::char_traits<char>,std::allocator<char> hero::get_luck_description(__$ReturnUdt);
// CODEVIEW(E:\gamedcs\hero.cpp:4689, dc 0xd38d8) unsigned char hero::HasSecondarySkill(int iWhich);
// CODEVIEW(E:\gamedcs\hero.cpp:4919, dc 0xd3ad0) void hero::remove_artifact(TArtifactSlot slot);
// CODEVIEW(E:\gamedcs\hero.cpp:5044, dc 0xd3de4) void hero::GiveArtifact(const type_artifact* artifact, int bCheckEnd, unsigned char equip_it);
// CODEVIEW(E:\gamedcs\hero.cpp:5064, dc 0xd3e40) int hero::GiveRandomArtifact();
// CODEVIEW(E:\gamedcs\hero.cpp:5709, dc 0xd49a8) float hero::GetLogisticsFactor();
// CODEVIEW(E:\gamedcs\hero.cpp:5734, dc 0xd49f0) long hero::GetNavigationFactor();
// CODEVIEW(E:\gamedcs\hero.cpp:5758, dc 0xd4a40) float hero::GetSorceryFactor();
// CODEVIEW(E:\gamedcs\hero.cpp:6025, dc 0xd4ed0) TSpellSchool hero::GetHighestSchool(TSpellSchool school_mask) const;
// CODEVIEW(E:\gamedcs\hero.cpp:6493, dc 0xd5800) void hero::reset_artifacts();
// CODEVIEW(E:\gamedcs\Hero.h:702, dc 0xd58f8) unsigned char hero::HasArmy(TCreatureType type);
// CODEVIEW(E:\gamedcs\Hero.h:712, dc 0xd5914) TSkillMastery hero::GetSpellSchoolLevel(TSpellSchool school_mask);

// --- std ---
// CODEVIEW(..\stlport\stl_bitset.h:379, dc 0xd5a04) std::bitset<48,unsigned* std::bitset<48,unsigned long>::reference::operator=(unsigned char __x);

// --- type_artifact ---
// CODEVIEW(E:\gamedcs\hero.cpp:2450, dc 0xcd8b8) std::basic_string<char,std::char_traits<char>,std::allocator<char> type_artifact::get_description(__$ReturnUdt);
// CODEVIEW(E:\gamedcs\hero.cpp:1226, dc 0xd59b8) void type_artifact::`default constructor closure'();

// --- type_obscuring_object ---
// CODEVIEW(E:\gamedcs\hero.cpp:380, dc 0xcaac8) mine* type_obscuring_object::get_obscured_mine();
// CODEVIEW(E:\gamedcs\Hero.h:162, dc 0xd58cc) unsigned char type_obscuring_object::obscures_town();

#endif  /* HOMM3_HERO_H */
