// town::HasBuilding - hero::GetLuck's Grail gate is a real CALL to the
// Town.h inline (retail `push 1 / push 0x1a / call 0x4305a0`), so this
// compiland needs the declarator. It has to be defined before the FIRST
// include that reaches town.h (game.h does, several lines below the
// explicit `#include "town.h"`), and it stays defined for the whole
// compiland: includes are TU-local, so no other object sees it. town.h's
// own note measures what declaring it unconditionally would cost.
#include <va.h>
// VC6's own shipped Dinkumware headers - retail links Dinkumware, NOT
// STLport (P2.3, answered 2026-08-07 and now byte-proven: see the COMDAT
// tail near the end of this file). No vendoring: the toolchain already
// carries the exact headers that built the retail COMDATs.
#include <bitset>
#include <functional>
#include <algorithm>
#include "bitset_iterator.h"
#include "homm3_limit.h"
#include <vector>
#include <stdio.h>
#include <string.h>
// abs() - the intrinsic form retail uses (cdq / xor / sub), the
// stdlib.h precedent already carried by cmbtmgr, window and ai_combat.
#include <stdlib.h>
// sqrt() - hero::get_combat_value_modifier (0x4e5400) calls the CRT
// entry, not an inlined fsqrt.
#include <math.h>
// hero.obj owns get_morale_description / get_luck_description, so the
// owning compiland joins their gate rather than defining armygrp's
// wider HOMM3_ARMYGRP_DESCRIPTION_API.
#include "hero.h"
// class army - hero::modify_spell_damage (0x4e5760) reads the target
// stack's embedded creature-traits level at +0x78.
#include "army.h"
// gpGame / playerData / game::IsHuman: the owner-record accessors
// (belongs_to_human, get_player) and every gpGame walk in this TU need
// the real definitions, and game.h is where they live.
#include "game.h"
// advManager::FizzleCenter - HeroView's dismiss path calls it. The one
// declarator, not the whole events view; advmgr.h's own note records why.
#include "advmgr.h"
#include "cursor.h"
// TSecondarySkill / TSkillMastery / akHeroSpecificAbilities - see the
// placement note at the top of that header.
// The creature arm of THeroSpecificAbility's +0x04 union: hero::GetMobility
// reads the specialty subject as a TCreatureType. hero.h above already
// pulled armygrp.h, so TCreatureType is in scope by the time this is read.
#include "herospec.h"
// UpgradedCreatureType - GetMobility's creature-specialty check asks for
// the specialty stack's upgrade; the cmbtmgr / hillfortwindow precedent.
#include "creaturetype.h"
// TArtifact / akArtifactTraits / gCombinationArtifacts - same placement
// rationale as herospec.h.
#include "artifact.h"
#include "exec.h"
#include "findpath.h"
#include "town.h"
// GetBuildingName - the Grail arm of both description bodies names the
// building it credits.
#include "castle.h"
// TMagicTerrain - the battlefield magic-terrain id the spell-school
// quartet takes as its second argument.
#include "magicterrain.h"
#include "kb.h"
#include "message.h"
#include "misc.h"
#include "winmgr.h"
#include "levelupwindow.h"
#include "sskilltraits.h"
// The two windows THeroScreenWindow::WindowHandler constructs on the
// stack: the spellbook on the artifact slot 17 arm and the quick-hero
// popup on the locator right-click arm.
#include "spellbookwindow.h"
#include "quickherowindow.h"
// DoQuestLog - the quest-log button arm.
#include "questlogwindow.h"
// gpInputManager - the two SHIFT arms force a mouse-move so the
// rollover text re-evaluates under the new modifier.
#include "inputmgr.h"
// gTurnDuration69d630 - CheckLevel shortens the multiplayer dialog
// deadline once the turn timer has expired.
#include "remote.h"
// launch_sample - the level-up jingle.
#include "soundmgr.h"
#include "resourcemanager.h"
#include "textresource.h"
// After Windows headers: undefine their min/max macros before using the
// includes.h integer wrappers recovered from Dreamcast.
#include "homm3_minmax.h"

#include "mousemgr.h"
#include "widget.h"

// The hero screen's own widget zoo: THeroScreenWindow's constructor
// builds 138 of them by hand.
#include "border.h"
#include "button.h"
#include "iconwdgt.h"
#include "textwdgt.h"
#include "includes.h"

DATA(0x0069774c) extern unsigned char g_campaignMode;
DATA(0x0067dcec) extern const THeroClassTraits (&g_heroClasses)[18];
// Runtime hero-view state used by the retail-only name getter below. The
// storage addresses and access widths are byte-proven; no public symbol
// roster survives for the two name-table pointer spellings, so they are
// provisional. gpCurrentHero and the hero-screen globals live in hero.h.
DATA(0x006a7540) extern const char* g_statDesc[4];
DATA(0x006a66d8) extern const char* g_sharedHeroNames[156];
// Runtime-loaded artifact rollover text. Retail fixes the three storage
// cells and their roles; no surviving public names them, so the spellings
// remain provisional.
DATA(0x006a6948) extern const char* g_campaignHeroName;
DATA(0x006a8040) extern const char* g_emptyArtifactRolloverText;
DATA(0x006a804c) extern const char* g_spellbookRolloverText;
DATA(0x006a8050) extern const char* g_artifactRolloverFormat;

// Runtime tables loaded by initialize_ballistics_table and its inlined
// initialize_move_constants tail. The relocation at 0x679c84 makes the
// public const view an array reference to the four writable 8-byte rows.
DATA(0x00698a58) static type_ballistics_traits g_ballisticsTraits[4];
DATA(0x00679c84)
const type_ballistics_traits (&g_constBallisticsTraits)[4] =
    g_ballisticsTraits;
DATA(0x00698a98) extern int g_landMovement[21];
DATA(0x00698aec) extern int g_seaMovement[kNumMasteries];
DATA(0x00698afc) extern int g_equestriansGlovesMovementBonus;
DATA(0x00698b00) extern int g_bootsOfSpeedMovementBonus;
DATA(0x00698b04) extern int g_oceanGuidanceMovementBonus;
DATA(0x00698b08) extern int g_seaCaptainsHatMovementBonus;
DATA(0x00698b0c) extern int g_lighthouseMovementBonus;

// The per-mastery specialty factor rows, one four-float .rdata run per
// skill (retail 0x63e9f8 / 0x63ea08 / 0x63ea58 / 0x63ea88 / 0x63ea98,
// read from the pinned image; they sit in one contiguous band of
// four-float rows starting at 0x63e9e8, one row per factor getter in
// this TU). File-static const, the kTown0Buildings precedent: retail
// emits them into .rdata with no external name, and each is read only
// by its own getter.
// The two INT rows of the same band (0x63e9c8 and 0x63e9d8): mana per
// day by Mysticism mastery and scouting radius in tiles.
// Leadership's morale bonus by mastery (retail 0x63e9a8, the same
// four-int band as the two rows below).
// Luck's own mastery row (retail 0x63e998) and Leadership's (0x63e9a8),
// the two four-int rows below the float band.
static const int g_luckBonuses[kNumMasteries] = { 0, 1, 2, 3 };
static const int g_leadershipBonuses[kNumMasteries] = { 0, 1, 2, 3 };
static const int g_mysticismBonuses[kNumMasteries] = { 1, 2, 3, 4 };
static const int g_scoutingVisibility[kNumMasteries] = { 5, 6, 7, 8 };
// Estates gold per day by mastery (retail 0x63ea18, the same band).
static const int g_estatesGold[kNumMasteries] = { 0, 125, 250, 500 };
static const float g_archeryFactors[kNumMasteries] =
    { 0.0f, 0.1f, 0.25f, 0.5f };
static const float g_eagleEyeFactors[kNumMasteries] =
    { 0.0f, 0.4f, 0.5f, 0.6f };
static const float g_diplomacyFactors[kNumMasteries] =
    { 0.0f, 0.2f, 0.4f, 0.6f };
static const float g_magicResistanceFactors[kNumMasteries] =
    { 0.0f, 0.05f, 0.1f, 0.2f };
static const float g_offenseFactors[kNumMasteries] =
    { 0.0f, 0.1f, 0.2f, 0.3f };
static const float g_defenseFactors[kNumMasteries] =
    { 0.0f, 0.05f, 0.1f, 0.15f };
static const float g_learningFactors[kNumMasteries] =
    { 0.0f, 0.05f, 0.1f, 0.15f };
static const float g_intelligenceFactors[kNumMasteries] =
    { 0.0f, 0.25f, 0.5f, 1.0f };
static const float g_firstAidFactors[kNumMasteries] =
    { 0.0f, 1.0f, 2.0f, 3.0f };
// Sorcery's spell-damage bonus by mastery (retail 0x63ea78).
// Necromancy's raise-rate by mastery (retail 0x63e9b8, same band).
static const float g_necromancyFactors[kNumMasteries] =
    { 0.0f, 0.1f, 0.2f, 0.3f };
static const float g_sorceryFactors[kNumMasteries] =
    { 0.0f, 0.05f, 0.1f, 0.15f };
// The two SPELL-specialty ladders GetHeroSpellBonus (0x4e5ff0) indexes
// by the target creature's level, seven entries each: retail 0x63eaa8
// (shared by the six buff spells) and 0x63eac4 (Slayer's own).
static const int g_buffSpecialtyBonus[7] = { 3, 3, 2, 2, 1, 1, 0 };
static const int g_slayerSpecialtyBonus[7] = { 4, 3, 2, 1, 0, 0, 0 };
// These three switch-only ids remain source-private because adding otherwise
// unused enumerators to armygrp.h changes initialize.obj's VC6 include
// personality. Retail and the DC SpellID roster prove the values.
static const SpellID g_spellFireWall = 0xd;
static const SpellID g_spellMagicArrow = 0xf;
static const SpellID g_spellHaste = 0x35;
// Source-private for the same reason: mark_spells' Sea Captain's Hat arm
// grants spells 0 and 1, and its Spellbinder's Hat arm sweeps akSpellTraits
// for level 5. SPELL_SUMMON_BOAT is already named in armygrp.h; its partner
// and the level are not, and nothing else in the image wants either.
static const SpellID g_spellScuttleBoat = 0x1;
static const int g_fifthLevelSpell = 5;
// HeroView's two remaining bare numbers. 0x81 is the hero screen's
// "dismiss" result, the only dialogReturn that reaches hero::Deallocate;
// winmgr.h's DIALOG_RETURN_* band is 0x78xx and does not carry it, and
// that header's own note measures what growing it costs. 6 is the text
// bank SetWinText loads for this window.
static const int g_dialogReturnDismissHero = 0x81;
static const int g_heroScreenWinText = 6;
// HeroFn_004E6120's three source-private ids, kept out of the shared
// headers for the same reason. 0x7f is the Vial of Dragon Blood (+5
// attack and +5 defense to dragons, which is the identification); the
// attribute bit is NH3API's CF_DRAGON; and hero 155 is the one hero whose
// universal-creature specialty also grants +1 speed - NH3API names him
// Xeron, and only that spelling is borrowed.
// WindowHandler re-evaluates the rollover whenever either SHIFT changes.
// These are PS/2 scan codes in message::codeX; inputmgr.h's own note
// measures what adding ungated enumerators there costs, so they stay here.
// HeroFn_004D8B30's start-level override: one campaign/scenario pair
// whose hero starts at another scenario hero's level plus five. The
// displacements are exact (gpGame + 0x4c893 IS heroes[151].level), but
// the campaign's identity is inference, so the names stay role-based.
static const int g_startLevelCampaign = 8;
static const int g_startLevelScenario = 3;
static const int g_startLevelHeroId = 151;
static const int g_startLevelBonus = 5;
static const int g_keyCodeLeftShift = 0x2a;
static const int g_keyCodeRightShift = 0x36;
static const int g_artifactVialOfDragonBlood = 0x7f;
static const int g_vialOfDragonBloodBonus = 5;
static const unsigned int g_creatureAttrDragon = 0x80000000;
static const int g_heroXeron = 0x9b;

void setWinText(heroWindow* win, int which);

// Experience needed to REACH each level, levels 1..12. Retail keeps it
// in .DATA at 0x679c88 (not .rdata - hence no `const`), immediately
// after the two reference cells at 0x679c80 / 0x679c84, and addresses
// it as `&table[-1]` (base 0x679c86) because the index is the 1-based
// level. Values read from the pinned image; they are HoMM3's own
// ladder, and the 1.2 extrapolation past level 12 reproduces the
// published level-13 threshold of 24320 exactly.
static short g_experienceForLevel[12] = {
    0, 1000, 2000, 3200, 4600, 6200,
    8000, 10000, 12200, 14700, 17500, 20600
};

// Per-campaign disabled secondary skills, one byte per TSecondarySkill.
// Retail .DATA at 0x679ca0, directly after kExperienceForLevel's 24 bytes
// - hence NOT const, the same reason that table is not. Read only through
// get_skill_award's campaign arm, where it REPLACES the scenario's own
// gpGame->field_4e658 row. Values read from the pinned image.
static char g_campaignDisabledSkills[kNumSecSkills] = {
    0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1,
    1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0
};

// The four magic schools as a table, retail .DATA 0x679cbc. NOT const:
// get_skill_award walks it with a live `mov eax,[esi]` each iteration,
// which a const array would let VC6 fold away.
static TSecondarySkill g_magicSchools[4] = {
    eSecSkillSchoolOfFireMagic, eSecSkillSchoolOfAirMagic,
    eSecSkillSchoolOfWaterMagic, eSecSkillSchoolOfEarthMagic
};

#if 0  // @carcass

// E:\gamedcs\hero.cpp:219 - promoted to a live claim (see below).

// E:\gamedcs\hero.cpp:254
DC_ONLY(0xca7c0, 0x12)
const char* hero::GetSpecificAbilityText()
{
    // @stub
}

// E:\gamedcs\hero.cpp:259
DC_ONLY(0xca7d4, 0x12)
const char* hero::getSpecificAbilityTextShort()
{
    // @stub
}

// E:\gamedcs\hero.cpp:267
DC_ONLY(0xca7e8, 0x19C)
unsigned char initializeMoveConstants()
{
    // @stub
}

// E:\gamedcs\hero.cpp:329
DC_ONLY(0xca984, 0x11C)
unsigned char initializeBallisticsTable()
{
    // @stub
}

#endif  // @carcass

// The specialty table itself and the const reference every reader goes
// through, the campaignmap.obj pattern: retail's writable array is at
// 0x678420 and the reference cell immediately after it at 0x679c80, which is
// what fixes the 156-row extent (0x679c80 - 0x678420 = 156 * 40).
DATA(0x00678420)
THeroSpecificAbility g_heroSpecificAbilitiesImp[156];

DATA(0x00679c80)
const THeroSpecificAbility (&g_heroSpecificAbilities)[156] =
    g_heroSpecificAbilitiesImp;

VA(0x004d71a0, 0x71)  // dc 0xca728
unsigned char initializeHeroSpecificAbilitiesTable()
{
    TSpreadsheetResource* text = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x00679ccc, heroSpecificAbilityTextName, "herospec.txt"));
    if (text == 0)
        return 0;

    if (text->getNumberOfRows() < 158) {
        text->dispose();
        return 0;
    }

    for (int i = 2; i < 158; i++) {
        const TSpreadsheetResource::TStringVector& row = text->getRow(i);
        g_heroSpecificAbilitiesImp[i - 2].m_shortText = row[0];
        g_heroSpecificAbilitiesImp[i - 2].m_mediumText = row[1];
        g_heroSpecificAbilitiesImp[i - 2].m_longText = row[2];
    }
    return 1;
}

VA(0x004d7220, 0x11)  // dc 0xca7d4
const char* hero::getSpecificAbilityTextShort()
{
    return g_heroSpecificAbilities[m_id].m_shortText;
}

// E:\gamedcs\hero.cpp:267
// Source-private in the Dreamcast roster and called only as the successful
// tail of initialize_ballistics_table. Complete keeps that helper boundary
// in source but /Ob2 expands it into the caller and emits no separate body.
static unsigned char initializeMoveConstants()
{
    TSpreadsheetResource* resource = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x00679cdc, movementSpreadsheetName, "movement.txt"));
    if (!resource)
        return 0;

    if (resource->getNumberOfRows() < 23) {
        resource->dispose();
        return 0;
    }

    int i;
    for (i = 0; i <= 20; ++i)
        g_landMovement[i] = atoi(resource->getRow(i + 2)[1]);

    int* seaMovement = g_seaMovement;
    for (int row = 2; row < 2 + kNumMasteries; ++row, ++seaMovement) {
        const TSpreadsheetResource::TStringVector& values =
            resource->getRow(row);
        *seaMovement = atoi(values[3]);
    }

    i = 2;
    g_equestriansGlovesMovementBonus = atoi(resource->getRow(i++)[5]);
    g_bootsOfSpeedMovementBonus = atoi(resource->getRow(i++)[5]);
    g_oceanGuidanceMovementBonus = atoi(resource->getRow(i++)[5]);
    g_seaCaptainsHatMovementBonus = atoi(resource->getRow(i++)[5]);
    g_stablesMovementBonus = atoi(resource->getRow(i++)[5]);
    g_lighthouseMovementBonus = atoi(resource->getRow(i)[5]);

    resource->dispose();
    return 1;
}

VA(0x004d7240, 0x223)  // dc 0xca984
unsigned char initializeBallisticsTable()
{
    TSpreadsheetResource* resource = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x00679cec, ballisticsSpreadsheetName, "ballist.txt"));
    if (!resource)
        return 0;

    if (resource->getNumberOfRows() < 6) {
        resource->dispose();
        return 0;
    }

    int i;
    for (i = 0; i < 4; ++i) {
        const TSpreadsheetResource::TStringVector& values =
            resource->getRow(i + 2);
        int column = 2;
        g_ballisticsTraits[i].m_chanceToHitMainBuilding = atoi(values[column++]);
        g_ballisticsTraits[i].m_chanceToHitTower = atoi(values[column++]);
        g_ballisticsTraits[i].m_chanceToHitDrawbridge = atoi(values[column++]);
        g_ballisticsTraits[i].m_chanceToHitWall = atoi(values[column++]);
        g_ballisticsTraits[i].m_shots = atoi(values[column++]);
        int level;
        for (level = 0; level < 3; ++level)
            g_ballisticsTraits[i].m_levelChance[level] =
                atoi(values[column++]);
    }

    resource->dispose();
    return initializeMoveConstants();
}

VA(0x004d7470, 0x1F)  // dc 0xcaaa0
type_obscuring_object::type_obscuring_object()
{
    initialize();
}

#if 0  // @carcass

// E:\gamedcs\hero.cpp:380
DC_ONLY(0xcaac8, 0x3A)
mine* type_obscuring_object::get_obscured_mine()
{
    // @stub
}

#endif  // @carcass

VA(0x004d7490, 0x35)  // dc 0xcab04
town* type_obscuring_object::getObscuredTown() const
{
    if (m_valid && m_obscuredType == TOWN && m_wasTrigger)
        return g_game->getTown(m_extraInfo);
    return 0;
}

VA(0x004d74d0, 0x1D)  // dc 0xcab3c
void type_obscuring_object::initialize()
{
    m_x = -1;
    m_y = -1;
    m_z = -1;
    m_valid = 0;
    m_obscuredType = NOTHING;
    m_wasTrigger = 0;
    m_extraInfo = 0;
}

VA(0x004d74f0, 0xD6)  // dc 0xcab54
bool type_obscuring_object::load(void* inputHandle)
{
    TAbstractFile* infile = static_cast<TAbstractFile*>(inputHandle);
    if (infile->read(&m_x, sizeof(m_x)) < sizeof(m_x))
        return 0;
    if (infile->read(&m_y, sizeof(m_y)) < sizeof(m_y))
        return 0;
    if (infile->read(&m_z, sizeof(m_z)) < sizeof(m_z))
        return 0;
    if (infile->read(&m_valid, sizeof(m_valid)) < sizeof(m_valid))
        return 0;
    if (infile->read(&m_obscuredLocation, sizeof(m_obscuredLocation))
            < sizeof(m_obscuredLocation))
        return 0;
    if (infile->read(&m_obscuredType, sizeof(m_obscuredType))
            < sizeof(m_obscuredType))
        return 0;
    if (infile->read(&m_wasTrigger, sizeof(m_wasTrigger))
            < sizeof(m_wasTrigger))
        return 0;
    bool success = infile->read(&m_extraInfo, sizeof(m_extraInfo))
        >= sizeof(m_extraInfo);
    return success;
}

VA(0x004d75d0, 0x10A)  // dc 0xcac58
void type_obscuring_object::obscureCell(TAdventureObjectType newType, long id)
{
    if (!m_valid) {
        type_point location;
        location.m_x = m_x;
        location.m_y = m_y;
        location.m_z = m_z;
        m_obscuredLocation = location;
        NewmapCell* cell = g_game->m_worldMap.cell(m_obscuredLocation);
        m_valid = 1;
        m_obscuredType = cell->m_type;
        m_wasTrigger = cell->m_isTrigger;
        m_extraInfo = cell->m_extraInfo;
        cell->m_isTrigger = 1;
        cell->m_type = newType;
        cell->m_extraInfo = id;

        if (m_obscuredType == TOWN && m_wasTrigger)
            g_game->getTown(m_extraInfo)->m_visitingHeroId = id;
    }
}

VA(0x004d76e0, 0xD0)  // dc 0xcacfc
void type_obscuring_object::restoreCell()
{
    if (m_valid) {
        NewmapCell* cell = g_game->m_worldMap.cell(m_obscuredLocation);
        m_valid = 0;
        cell->m_type = m_obscuredType;
        cell->m_isTrigger = m_wasTrigger;
        cell->m_extraInfo = m_extraInfo;

        if (m_obscuredType == TOWN && m_wasTrigger)
            g_game->getTown(m_extraInfo)->m_visitingHeroId = -1;
    }
}

VA(0x004d77b0, 0xD6)  // dc 0xcad80
bool type_obscuring_object::save(void* outputHandle)
{
    TAbstractFile* outfile = static_cast<TAbstractFile*>(outputHandle);
    if (outfile->write(&m_x, sizeof(m_x)) < sizeof(m_x))
        return 0;
    if (outfile->write(&m_y, sizeof(m_y)) < sizeof(m_y))
        return 0;
    if (outfile->write(&m_z, sizeof(m_z)) < sizeof(m_z))
        return 0;
    if (outfile->write(&m_valid, sizeof(m_valid)) < sizeof(m_valid))
        return 0;
    if (outfile->write(&m_obscuredLocation, sizeof(m_obscuredLocation))
            < sizeof(m_obscuredLocation))
        return 0;
    if (outfile->write(&m_obscuredType, sizeof(m_obscuredType))
            < sizeof(m_obscuredType))
        return 0;
    if (outfile->write(&m_wasTrigger, sizeof(m_wasTrigger))
            < sizeof(m_wasTrigger))
        return 0;
    bool success = outfile->write(&m_extraInfo, sizeof(m_extraInfo))
        >= sizeof(m_extraInfo);
    return success;
}

VA(0x004d7890, 0x64)  // dc 0xcae60
void hero::hire(int playerId, type_point point)
{
    playerData* player = &g_game->m_players[playerId];
    int recruitSlot = 0;
    while (player->m_recruits[recruitSlot] != m_id)
        recruitSlot++;

    player->m_resources[GOLD] =
        player->m_resources[GOLD] - g_heroGoldCost;
    placeInMap(playerId, point, 1);
    g_game->finishTownHire(playerId, recruitSlot);
}

VA(0x004d7900, 0x11B)  // dc 0xcaedc
void hero::placeInMap(int playerId, type_point point, unsigned char resetFlags)
{
    playerData* player = &g_game->m_players[playerId];
    g_game->recordShowHero(this, static_cast<signed char>(playerId),
                             point, 0);

    player->m_heroes[player->m_numHeroes] = m_id;
    ++player->m_numHeroes;
    g_game->m_heroAvailability[m_id] = static_cast<char>(playerId);
    g_game->m_heroPoolMap[m_id].set(playerId);

    m_owner = static_cast<signed char>(playerId);
    m_x = point.m_x;
    m_y = point.m_y;
    m_z = point.m_z;
    m_facing = 2;
    if (resetFlags)
        m_flags &= 0xfff9ffff;

    type_obscuring_object::obscureCell(HERO, m_id);

    CMCRecruitHero change(m_id, point, g_netLocalGamePos);
    sendMapChange(&change);
}

// E:\gamedcs\hero.cpp:577
// save's twin, field for field and in the same order. Three SAVE-VERSION
// gates are the only asymmetry, and each is a real format migration:
//   >= 25  the sex byte, the custom-name flag and the name itself;
//   <= 30  only EIGHTEEN equipped slots on the wire, after which the
//          nineteenth - the one Shadow of Death added, and the one
//          HeroFn_004E2550 refuses in a pre-SoD game - is cleared here
//          by hand. Independent corroboration of that slot's identity;
//   >= 32  the per-class artifact counts.
// Unlike save, load takes `ret 8` for its two arguments; only
// type_obscuring_object::load's result is checked and every scalar Read
// is unchecked. EH-bearing: the name assignment owns a string temporary
// and the bitset's inlined set() carries its range throw.

// Residual (87.9%): the /Ob2 budget inside the name assignment, nothing
// source-local. `predict-inline` pairs 5 of the 13 out-of-line calls off
// by COUNT (retail names its unclaimed basic_string callees, and the
// 0x485d90 reader itself, with synth labels our side can never emit) and
// leaves ONE real item, which it reports as `_Tidy` out of line 4 times
// here against retail's 1.

// READ THAT ROW THE OTHER WAY ROUND (2026-08-20, bytes). It is an
// OVER-inline of basic_string::_Grow on OUR side, not an under-inline of
// _Tidy. Both compiles inline std::bitset<48>::set AND its whole _Xran
// throw at the granted-mask loop - our fn+0x652 and retail's fn+0x5e3
// both carry the `invalid bitset<N> position` string construction inline.
// Inside it, `assign(const char*, size_type)` calls `_Grow(_N, true)`,
// and THAT is where the two diverge: retail leaves _Grow a CALL
// (`push 1 / push ebx / lea ecx,[ebp-0x30] / call 0x4a90`), while our CL
// expands _Grow and so exposes the `_Tidy` and `_Copy` calls that live
// inside it. Same for the customName temporary at the head, where both
// sides are already identical (`call assign(const basic_string&,
// size_type, size_type)` then one `_Tidy`).
// No statement in this body reaches _Grow - it is three levels down
// inside an expansion both compiles agree to make - so the pin lever does
// not apply and the depth lever (spelling the site one wrapper deeper)
// has no shallower or deeper form to choose from here.

VA(0x004d7a20, 0x69F)  // linkorder, dc 0xcaf98
int hero::load(TAbstractFile* infile, int saveVersion)
{
    unsigned int uintBuffer;
    unsigned short ushortBuffer;
    int intBuffer;
    short shortBuffer;
    unsigned char ucharBuffer;
    char charBuffer;

    if (!type_obscuring_object::load(infile))
        return -1;

    if (saveVersion >= 25) {
        infile->read(&charBuffer, sizeof(charBuffer));
        m_sex = static_cast<signed char>(charBuffer);
        infile->read(&ucharBuffer, sizeof(ucharBuffer));
        m_hasCustomName = ucharBuffer != 0;
        m_customName = readLengthPrefixedString(infile);
    }

    infile->read(&charBuffer, sizeof(charBuffer));
    m_owner = charBuffer;
    infile->read(&charBuffer, sizeof(charBuffer));
    m_patrolRadius = charBuffer;
    infile->read(&charBuffer, sizeof(charBuffer));
    m_moraleBonus = charBuffer;
    infile->read(&charBuffer, sizeof(charBuffer));
    m_luckBonus = charBuffer;
    infile->read(&charBuffer, sizeof(charBuffer));
    m_backpackCount = charBuffer;
    infile->read(&charBuffer, sizeof(charBuffer));
    m_disguiseLevel = static_cast<signed char>(charBuffer);
    infile->read(&charBuffer, sizeof(charBuffer));
    m_flightLevel = static_cast<signed char>(charBuffer);
    infile->read(&charBuffer, sizeof(charBuffer));
    m_waterWalkLevel = static_cast<signed char>(charBuffer);
    infile->read(&charBuffer, sizeof(charBuffer));
    m_dWalkSpellsCast = charBuffer;
    infile->read(&charBuffer, sizeof(charBuffer));
    m_visionsPower = static_cast<signed char>(charBuffer);
    infile->read(&charBuffer, sizeof(charBuffer));
    m_id = static_cast<signed char>(charBuffer);
    infile->read(&charBuffer, sizeof(charBuffer));
    m_heroClass = static_cast<signed char>(charBuffer);
    infile->read(&ucharBuffer, sizeof(ucharBuffer));
    m_portrait = ucharBuffer;
    infile->read(&ucharBuffer, sizeof(ucharBuffer));
    m_patrolX = ucharBuffer;
    infile->read(&ucharBuffer, sizeof(ucharBuffer));
    m_patrolY = ucharBuffer;
    infile->read(&ucharBuffer, sizeof(ucharBuffer));
    m_facing = ucharBuffer;
    infile->read(&ucharBuffer, sizeof(ucharBuffer));
    m_formation = ucharBuffer;
    infile->read(&ucharBuffer, sizeof(ucharBuffer));
    m_levelSeed = ucharBuffer;
    infile->read(&ucharBuffer, sizeof(ucharBuffer));
    m_lastWisdom = ucharBuffer;

    infile->read(&intBuffer, sizeof(intBuffer));
    m_pathTargetX = intBuffer;
    infile->read(&intBuffer, sizeof(intBuffer));
    m_pathTargetY = intBuffer;
    infile->read(&shortBuffer, sizeof(shortBuffer));
    m_pathTargetZ = shortBuffer;
    infile->read(&shortBuffer, sizeof(shortBuffer));
    m_lastMagicSchoolLevel = shortBuffer;
    infile->read(&intBuffer, sizeof(intBuffer));
    m_maxMovePoints = intBuffer;
    infile->read(&intBuffer, sizeof(intBuffer));
    m_movePoints = intBuffer;
    infile->read(&intBuffer, sizeof(intBuffer));
    m_experience = intBuffer;
    infile->read(&intBuffer, sizeof(intBuffer));
    m_skillCount = intBuffer;
    infile->read(&shortBuffer, sizeof(shortBuffer));
    m_mana = shortBuffer;
    infile->read(&shortBuffer, sizeof(shortBuffer));
    m_level = shortBuffer;
    infile->read(&ushortBuffer, sizeof(ushortBuffer));
    m_targetDistance = ushortBuffer;

    infile->read(&uintBuffer, sizeof(uintBuffer));
    m_trainingGroundsFlags = uintBuffer;
    infile->read(&uintBuffer, sizeof(uintBuffer));
    m_defenseTowerFlags = uintBuffer;
    infile->read(&uintBuffer, sizeof(uintBuffer));
    m_gardenOfRevelationFlags = uintBuffer;
    infile->read(&uintBuffer, sizeof(uintBuffer));
    m_mercCampFlags = uintBuffer;
    infile->read(&uintBuffer, sizeof(uintBuffer));
    m_powerSchoolFlags = uintBuffer;
    infile->read(&uintBuffer, sizeof(uintBuffer));
    m_treeOfKnowledgeFlags = uintBuffer;
    infile->read(&uintBuffer, sizeof(uintBuffer));
    m_libraryFlags = uintBuffer;
    infile->read(&uintBuffer, sizeof(uintBuffer));
    m_arenaFlags = uintBuffer;
    infile->read(&uintBuffer, sizeof(uintBuffer));
    m_magicSchoolFlags = uintBuffer;
    infile->read(&uintBuffer, sizeof(uintBuffer));
    m_warSchoolFlags = uintBuffer;
    infile->read(&uintBuffer, sizeof(uintBuffer));
    m_universityFlags = uintBuffer;
    infile->read(&uintBuffer, sizeof(uintBuffer));
    m_shrine1Flags = uintBuffer;
    infile->read(&uintBuffer, sizeof(uintBuffer));
    m_shrine2Flags = uintBuffer;
    infile->read(&uintBuffer, sizeof(uintBuffer));
    m_shrine3Flags = uintBuffer;
    infile->read(&uintBuffer, sizeof(uintBuffer));
    m_flags = uintBuffer;

    m_army.load(infile);

    infile->read(m_name, sizeof(m_name));
    infile->read(m_skillLevel, sizeof(m_skillLevel));
    infile->read(m_skillOrder, sizeof(m_skillOrder));
    infile->read(m_stats, sizeof(m_stats));
    infile->read(m_inSpellbook, sizeof(m_inSpellbook));
    infile->read(m_availableSpells, sizeof(m_availableSpells));

    if (saveVersion <= 30) {
        infile->read(m_equipped, 18 * sizeof(type_artifact));
        m_equipped[EQUIPPED_SLOT_SOD_MISC].m_artifactId = ARTIFACT_NONE;
        m_equipped[EQUIPPED_SLOT_SOD_MISC].m_extra = -1;
    } else {
        infile->read(m_equipped, sizeof(m_equipped));
    }
    infile->read(m_backpack, sizeof(m_backpack));
    if (saveVersion >= 32)
        infile->read(m_artifactSlotCounts, sizeof(m_artifactSlotCounts));

    infile->read(&ucharBuffer, sizeof(ucharBuffer));
    m_isSleeping = ucharBuffer != 0;

    std::bitset<48> granted;
    unsigned char inBuf[6];
    infile->read(inBuf, sizeof(inBuf));
    for (unsigned int i = 0; i < 48; i++)
        granted.set(i, (inBuf[i >> 3] & (1 << (i & 7))) != 0);
    m_townSpecialGrantedMask = granted;
    return 0;
}

// The record serialiser's field writers. Each takes its value BY VALUE, so
// the parameter itself is the stack temp whose address Write() receives, and
// the PARAMETER's type - not the member's - fixes the width, which is what
// makes the write widths independent of the member widths. Inlined at every
// call, they coalesce into retail's 0x8 frame; six hand-written function-scope
// buffers cannot coalesce and cost 0x1c with seven slots (99.2181).
// Dreamcast records exactly one local per width in hero::save - char, uchar,
// short, ushort, int and uint buffers - and 57 nested lexical-scope pairs, one
// per write site. That is the residue of these bodies inlined at every call,
// which is also why no standalone procedure for them survives in the DC
// roster: a helper inlined everywhere emits none.
inline void writeField(TAbstractFile* outfile, char value)
{
    outfile->write(&value, sizeof(value));
}

inline void writeField(TAbstractFile* outfile, unsigned char value)
{
    outfile->write(&value, sizeof(value));
}

inline void writeField(TAbstractFile* outfile, short value)
{
    outfile->write(&value, sizeof(value));
}

inline void writeField(TAbstractFile* outfile, unsigned short value)
{
    outfile->write(&value, sizeof(value));
}

inline void writeField(TAbstractFile* outfile, int value)
{
    outfile->write(&value, sizeof(value));
}

inline void writeField(TAbstractFile* outfile, unsigned int value)
{
    outfile->write(&value, sizeof(value));
}

// E:\gamedcs\hero.cpp:914
// The record serialiser. Typed scratch locals carry every scalar into
// the stream - retail copies each field into a stack temp and hands
// Write() that temp's address, never the member's, which is what makes
// the write widths independent of the member widths (the byte writes of
// `sex`, `id`, `heroClass`, `disguiseLevel`, `flightLevel`,
// `waterWalkLevel` and `field_129` are all truncating stores out of
// four-byte members, emitted as plain `mov al, byte [this+off]`).

// Only the leading type_obscuring_object::save is checked; every
// hero-level Write ignores its result and the body always returns 0.
// The custom name goes out as its length followed by c_str()'s bytes -
// the `test ecx,ecx / mov ecx, heroNameEmptyText` pair at +0x64 is
// Dinkumware's c_str() null fallback inlined, the same expansion
// HeroFn_004D8FB0 carries.

VA(0x004d80c0, 0x526)  // dc 0xcb698
int hero::save(TAbstractFile* outfile)
{
    if (!type_obscuring_object::save(outfile))
        return -1;

    writeField(outfile, static_cast<char>(m_sex));
    writeField(outfile, static_cast<unsigned char>(m_hasCustomName));

    writeField(outfile, static_cast<unsigned int>(m_customName.length()));
    outfile->write(m_customName.c_str(), m_customName.length());

    writeField(outfile, static_cast<char>(m_owner));
    writeField(outfile, static_cast<char>(m_patrolRadius));
    writeField(outfile, static_cast<char>(m_moraleBonus));
    writeField(outfile, static_cast<char>(m_luckBonus));
    writeField(outfile, static_cast<char>(m_backpackCount));
    writeField(outfile, static_cast<char>(m_disguiseLevel));
    writeField(outfile, static_cast<char>(m_flightLevel));
    writeField(outfile, static_cast<char>(m_waterWalkLevel));
    writeField(outfile, static_cast<char>(m_dWalkSpellsCast));
    writeField(outfile, static_cast<char>(m_visionsPower));
    writeField(outfile, static_cast<char>(m_id));
    writeField(outfile, static_cast<char>(m_heroClass));
    writeField(outfile, static_cast<unsigned char>(m_portrait));
    writeField(outfile, static_cast<unsigned char>(m_patrolX));
    writeField(outfile, static_cast<unsigned char>(m_patrolY));
    writeField(outfile, static_cast<unsigned char>(m_facing));
    writeField(outfile, static_cast<unsigned char>(m_formation));
    writeField(outfile, static_cast<unsigned char>(m_levelSeed));
    writeField(outfile, static_cast<unsigned char>(m_lastWisdom));

    writeField(outfile, static_cast<int>(m_pathTargetX));
    writeField(outfile, static_cast<int>(m_pathTargetY));
    writeField(outfile, static_cast<short>(m_pathTargetZ));
    writeField(outfile, static_cast<short>(m_lastMagicSchoolLevel));
    writeField(outfile, static_cast<int>(m_maxMovePoints));
    writeField(outfile, static_cast<int>(m_movePoints));
    writeField(outfile, static_cast<int>(m_experience));
    writeField(outfile, static_cast<int>(m_skillCount));
    writeField(outfile, static_cast<short>(m_mana));
    writeField(outfile, static_cast<short>(m_level));
    writeField(outfile, static_cast<unsigned short>(m_targetDistance));

    writeField(outfile, static_cast<unsigned int>(m_trainingGroundsFlags));
    writeField(outfile, static_cast<unsigned int>(m_defenseTowerFlags));
    writeField(outfile, static_cast<unsigned int>(m_gardenOfRevelationFlags));
    writeField(outfile, static_cast<unsigned int>(m_mercCampFlags));
    writeField(outfile, static_cast<unsigned int>(m_powerSchoolFlags));
    writeField(outfile, static_cast<unsigned int>(m_treeOfKnowledgeFlags));
    writeField(outfile, static_cast<unsigned int>(m_libraryFlags));
    writeField(outfile, static_cast<unsigned int>(m_arenaFlags));
    writeField(outfile, static_cast<unsigned int>(m_magicSchoolFlags));
    writeField(outfile, static_cast<unsigned int>(m_warSchoolFlags));
    writeField(outfile, static_cast<unsigned int>(m_universityFlags));
    writeField(outfile, static_cast<unsigned int>(m_shrine1Flags));
    writeField(outfile, static_cast<unsigned int>(m_shrine2Flags));
    writeField(outfile, static_cast<unsigned int>(m_shrine3Flags));
    writeField(outfile, static_cast<unsigned int>(m_flags));

    m_army.save(outfile);

    outfile->write(m_name, sizeof(m_name));
    outfile->write(m_skillLevel, sizeof(m_skillLevel));
    outfile->write(m_skillOrder, sizeof(m_skillOrder));
    outfile->write(m_stats, sizeof(m_stats));
    outfile->write(m_inSpellbook, sizeof(m_inSpellbook));
    outfile->write(m_availableSpells, sizeof(m_availableSpells));
    outfile->write(m_equipped, sizeof(m_equipped));
    outfile->write(m_backpack, sizeof(m_backpack));
    outfile->write(m_artifactSlotCounts, sizeof(m_artifactSlotCounts));

    writeField(outfile, static_cast<unsigned char>(m_isSleeping));

    const std::bitset<48>& granted = m_townSpecialGrantedMask;
    unsigned char outBuf[6];
    memset(outBuf, 0, sizeof(outBuf));
    for (unsigned int i = 0; i < 48; ++i) {
        if (granted.test(i))
            outBuf[i >> 3] |= 1 << (i & 7);
    }
    outfile->write(outBuf, sizeof(outBuf));
    return 0;
}

// E:\gamedcs\hero.cpp:1208
// The first half of the body is entirely implicit: the base and the
// four constructed members run in declaration order and are all visible
// in the bytes - type_obscuring_object::initialize() inlined at +0
// (three -1 words, then valid / obscuredType / was_trigger /
// extra_info), a CALL to armyGroup::armyGroup at +0x91, the bitset's
// own descending two-word _Tidy at +0x121/+0x125, the nineteen and
// sixty-four stride-8 type_artifact() loops at +0x12d / +0x1d4, and the
// Dinkumware string at +0x3da (the `mov al,[ebp-1]` is the empty
// allocator temp being copied, which is what identifies this as a
// constructor and not a reset method).
// The body then re-clears the identity band. Retail really does write
// the two hero-SCREEN globals from here: 0x698a78 is the window pointer
// HeroView news up and 0x697738 the selected army slot, and this is the
// only reference to either outside the hero-screen block.

// CURRENT (89.27957%, from 70.98925%): the missing /Ob2 candidate is a
// conventional release VERIFY invariant. `bitset<48>::size()` is a real
// inline accessor and therefore enters C2's site budget, but returns a
// compile-time nonzero capacity, so `(void)size()` emits no check. That
// single source-history carrier makes VC6 retain retail's out-of-line
// `basic_string::_Tidy` from the inlined default string constructor. Both
// sides now emit two calls and flow-distance is zero.

// The exact macro name and original invariant are unattested; the spelling
// below is explicitly provisional. Its carrier class is source-plausible and
// distinguished by compiler evidence: release-elided TRACE-shaped printf
// sites at doses 1/3/5 are byte-flat at 70.98925%; VERIFY(bitset.none())
// reaches the right two-call phase but emits two excess runtime branches and
// scores 80.03226%; VERIFY(!bitset.test(0)) and VERIFY(!bitset[0]) emit no
// excess flow but select a worse register phase at 74.95699%. The constant
// size invariant alone reproduces the previously synthetic ceiling.

// Residual (89.27957%): one instruction and a whole-body EBX/EDI role swap
// over 53 register-visible slots. why-reg's complete mutation set finds no
// improving declaration/store-order edit; the call and branch structures are
// already retail's.
// THE BUDGET THRESHOLD IS MEASURED (2026-08-19). Adding N byte-inert
// user-defined inline sites to the body (`_cpp_max(i, i);`, the
// armygrp probe idiom) moves the row:
//     N=0 -> 70.99    N=1..5 -> 89.28    N=6 -> 83.63    N=8 -> 68.00
// i.e. ONE more inline candidate site denies `_Tidy` its nested share of
// the /Ob2 budget (`budget / sites-remaining`, docs/vc6/inliner.md) and
// the call comes back. So retail's body carries one to five candidate
// sites this reconstruction does not, and the window is wide enough that
// a single statement accounts for it. The DC call census CANNOT name it:
// dc 0xcbdb8's callee list is exactly what is already written here -
// type_obscuring_object::type_obscuring_object x1, armyGroup::armyGroup
// x1, bitset<48>::bitset x1, bitset<48>::reset x1, type_artifact's
// default-constructor closure x2 and the vector-constructor iterator x2 -
// with NO std::string at all, because the DC record has no customName.
// The member that creates this wall is precisely the one the Dreamcast
// build does not have. The release VERIFY below is the minimum honest
// replacement for that old instrument.
// MEASURED NEGATIVE, do not retry: `#pragma inline_depth(0)` spanning the
// member-initialiser expansion, to chase retail's out-of-line
// basic_string::_Tidy (base x0 vs retail x1), costs 70.99 -> 53.99. The
// pin DOES reach a member-init - which is worth knowing, the eh-cleanup
// doc's caveat notwithstanding - but it de-inlines the whole init closure
// (type_obscuring_object, armyGroup, bitset<48>, the type_artifact
// default-ctor pair) where retail keeps all of those expanded, and only
// customName's _Tidy out of line.
VA(0x004d85f0, 0x12E)  // anchor-bracket, dc 0xcbdb8
hero::hero()
{
    HOMM3_RELEASE_VERIFY(m_townSpecialGrantedMask.size());

    m_x = 0;
    m_y = 0;
    m_heroClass = 0;
    m_portrait = 0;
    m_name[0] = 0;
    m_id = -1;
    m_owner = -1;

    int i;
    for (i = 0; i < 19; i++)
        m_equipped[i] = type_artifact();
    memset(m_artifactSlotCounts, 0, sizeof(m_artifactSlotCounts));
    for (i = 0; i < 64; i++)
        m_backpack[i] = type_artifact();
    m_townSpecialGrantedMask.reset();

    g_heroScreenWindow = 0;
    g_heroScreenArmySlot = -1;
    m_isSleeping = 0;
}

// E:\gamedcs\hero.cpp:1233
// Slot pinned by the two flanking claims (hero::hero 0x4d85f0 above,
// get_equipped_artifacts 0x4d9070 below) and by the body: `movsx edx,
// word [ebp+8]` takes the SHORT index DC's prototype declares, and the
// initialiser writes exactly the modelled layout - -1 into the three
// packed coordinate words, 19 stride-8 equipped slots from +0x12d, 64
// stride-8 backpack slots from +0x1d4, and the two 28-byte
// secondary-skill bands at +0xc9 / +0xe5.

// DC hero.cpp:1260-1262 calls SetPrimarySkill from a signed-short loop;
// the increment at dc 0xcbf44 truncates/sign-extends it, as does the army
// loop at 0xcc054. Restoring the canonical call and short i reproduces the
// entire retail stats loop (+0x15f..+0x17f), raising 82.8591 -> 85.6745%.
// Standard artifact fills then restore both retained string::_Tidy calls
// without changing the string assignment, reaching 91.0235%. The remaining
// early differences are the fills' pointer-end guards versus retail's
// countdown loops, plus memset/trait-load scheduling. The old diagnosis of
// an unavoidable string-inliner wall was false: these preceding source
// operations determine that nested boundary.
// A shared function-scope int index with per-element artifact loops was
// independently measured at 89.67%; a fresh int index gives 82.5336%.
// Controls: short skill-only index 85.6409%;
// literal empty-name assignment and explicit ARTIFACT_NONE construction are
// byte-flat against the 85.6745% loop candidate. fill_n emits an overlapping
// rep-movsd fill instead of retail's two-store loops (79.5302%). Older string
// assign/operator= wrappers and inline_depth 2/3/4 were byte-flat; a zero-depth
// assign pin lost to 62.31%. Synthetic invariant carriers were also byte-flat
// and are not retained.
VA(0x004d8720, 0x410)  // anchor-bracket + layout, dc 0xcbe80
void hero::initialize(short index)
{
    const int& initialSex = g_heroTraits[index].m_sex;

    type_obscuring_object::initialize();
    memset(m_inSpellbook, 0, sizeof(m_inSpellbook));
    memset(m_availableSpells, 0, sizeof(m_availableSpells));

    short i;
    std::fill(m_equipped, m_equipped + 19, type_artifact());

    memset(m_artifactSlotCounts, 0, sizeof(m_artifactSlotCounts));
    std::fill(m_backpack, m_backpack + 64, type_artifact());
    m_backpackCount = 0;
    memset(m_skillLevel, 0, sizeof(m_skillLevel));
    memset(m_skillOrder, 0, sizeof(m_skillOrder));

    m_patrolY = kPatrolNone;
    m_patrolX = kPatrolNone;
    m_id = index;
    m_portrait = static_cast<unsigned char>(index);
    m_sex = initialSex;
    m_townSpecialGrantedMask.reset();
    m_owner = -1;
    m_facing = kFacingE;

    strncpy(m_name, g_heroTraits[index].m_defaultName, sizeof(m_name));
    m_name[sizeof(m_name) - 1] = 0;
    m_heroClass = g_heroTraits[index].m_heroClass;
    m_skillCount = 0;
    for (i = 0; i < 4; ++i) {
        setPrimarySkill(i, g_heroClasses[m_heroClass].m_initialPrimarySkill[i]);
    }

    if (g_heroTraits[index].m_firstSkill != eSecSkillNone) {
        giveSS(g_heroTraits[index].m_firstSkill,
               g_heroTraits[index].m_firstSkillLevel);
    }
    if (g_heroTraits[index].m_secondSkill != eSecSkillNone) {
        giveSS(g_heroTraits[index].m_secondSkill,
               g_heroTraits[index].m_secondSkillLevel);
    }
    if (g_heroTraits[index].m_startsWithSpellbook) {
        m_equipped[17].m_artifactId = TArtifact(ARTIFACT_SPELLBOOK);
    }
    if (g_heroTraits[index].m_startingSpell != -1)
        addSpell(g_heroTraits[index].m_startingSpell);

    m_aggression = static_cast<float>(random(75, 100)) *
                g_heroClasses[m_heroClass].m_aggression /
                static_cast<float>(random(100, 125));

    m_equipped[16].m_artifactId = ARTIFACT_CATAPULT;
    for (i = 0; i < 7; i++)
        m_army.m_armies[i] = CREATURE_NONE;
    m_pathTargetY = -1;
    m_pathTargetX = -1;
    m_level = 1;

    m_mana = static_cast<short>(getMaxMana());

    m_maxMovePoints = 0;
    m_movePoints = 0;
    m_flightLevel = eMasteryInvalid;
    m_waterWalkLevel = eMasteryInvalid;
    m_disguiseLevel = eMasteryInvalid;
    m_dWalkSpellsCast = 0;
    m_visionsPower = eMasteryInvalid;
    m_hasCustomName = 0;
    m_customName = g_emptyRolloverText;
    m_isSleeping = 0;
    m_formation = 2;
    m_flags = 0;
    m_trainingGroundsFlags = 0;
    m_defenseTowerFlags = 0;
    m_gardenOfRevelationFlags = 0;
    m_mercCampFlags = 0;
    m_powerSchoolFlags = 0;
    m_treeOfKnowledgeFlags = 0;
    m_libraryFlags = 0;
    m_arenaFlags = 0;
    m_magicSchoolFlags = 0;
    m_warSchoolFlags = 0;
    m_universityFlags = 0;
    m_shrine1Flags = 0;
    m_shrine2Flags = 0;
    m_shrine3Flags = 0;
    m_moraleBonus = 0;
    m_luckBonus = 0;
}

// RETAIL-ONLY x3. The DC roster runs hero::initialize (0xcbe80)
// straight into belongs_to_human (0xcc0bc) with nothing between them,
// but retail carves THREE bodies into that gap. All three are hero
// members (ecx is used as a hero and every displacement lands inside
// the modelled 0x492 record) and none of them can be named from the
// Dreamcast build, so the names below are ORDINAL PLACEHOLDERS flagged
// unattested (HeroFn_004E5DE0 / WIDGET_RETURN_32 precedent). The HD
// crossbuild map has no row for any of them either.

// 0x004d8b30 `ret 4`: copies one map/scenario setup record into the
// hero. Its single caller is inside game.obj (0x4cae10), which walks 156
// records with `add ebx, 0x334` - which is what closes the record's size.

// Dreamcast initialize_hero(hero*, const HeroExtra*) is a free function at
// hero.cpp:1098, dc 0xb6c84; Complete implements it as this member.
// Keep basic_string::assign out of line at the customName assignment.

// Residual (94.67%): register-homing only, and why-reg v2's model CAPS it -
// "bindings agree at every first definition, the divergence is past the
// first defs", i.e. not the B1 minimum slice and no creation-order edit
// reaches it. The schedule is aligned and flow-distance is 0; what is left
// is edi->ecx x7 / ecx->edx x5 over 54 register-visible slots.
// 94.6716 -> 97.5836 (2026-09-06), three source facts read straight off the
// retail bytes.  All 65 blocks, 37 branches and 13 out-of-line calls now
// agree.
//   * the patrol else-arm writes patrolY BEFORE patrolX.  Written the other
//     way both arms end with the same store and the cross-jumper merges it
//     into a shared block; retail keeps both stores duplicated per arm, in
//     opposite orders, which is exactly what stops the merge (+0.33);
//   * the custom-name assignment is the THREE-argument
//     `assign(const basic_string&, size_type, size_type)` with npos loaded
//     from its out-of-line definition, not `operator=` (+1.78 under the
//     existing pin);
//   * the troop count is NAMED once per iteration.  Retail widens it, tests
//     the widened value and stores it (`movsx ecx,word[edx] / test ecx,ecx /
//     mov [eax+0x1c],ecx`); re-reading the member for the comparison costs
//     the register and re-reads memory.  `int` is the type - `short` scores
//     95.5204 (+0.80).
// Residual (97.5836%): the `lea edi` of the FIRST memset in each zeroing
// pair.  Retail issues the destination address ahead of `mov ecx,<count> /
// xor eax,eax`; this compile issues it last, which is the right-to-left
// argument order.  The SECOND memset of the second pair already agrees, so
// it is a block-entry schedule and not the spelling: `&arr[0]` is byte-flat
// and hoisting `skillCount = 0` above the pair costs 0.36.
VA(0x004d8b30, 0x434)  // retail-only, hero member, ret 4
void hero::heroFn004D8B30(const HeroExtra* setup)
{
    m_order = setup->m_objRef;
    m_x = setup->m_location.m_x;
    m_y = setup->m_location.m_y;
    m_z = setup->m_location.m_z;
    m_owner = setup->m_owner;
    m_id = setup->m_id;
    m_heroClass = g_heroTraits[setup->m_id].m_heroClass;

    m_patrolRadius = setup->m_patrolRadius;
    if (setup->m_patrolRadius >= 0) {
        m_patrolX = m_x;
        m_patrolY = m_y;
    } else {
        m_patrolY = kPatrolNone;
        m_patrolX = kPatrolNone;
    }

    if (setup->m_hasCustomName) {
        strncpy(m_name, setup->m_nameBuffer, sizeof(m_name));
        m_name[sizeof(m_name) - 1] = 0;
    }
    if (setup->m_customPortraitNumber)
        m_portrait = setup->m_portraitNumber;

    if (setup->m_customPrimarySkills) {
        for (int i = 0; i < 4; i++)
            m_stats[i] = setup->m_primarySkills[i];
    }

    if (setup->m_customSecondarySkills) {
        memset(m_skillLevel, 0, sizeof(m_skillLevel));
        memset(m_skillOrder, 0, sizeof(m_skillOrder));
        m_skillCount = 0;
        for (int i = 0; i < setup->m_numSecondarySkills; i++)
            giveSS(setup->m_secondarySkill[i], setup->m_secondarySkillLevel[i]);
    }

    if (setup->m_customArmies) {
        for (int i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; i++) {
            int count = setup->m_numTroops[i];
            m_army.m_numTroops[i] = count;
            if (count > 0)
                m_army.m_armies[i] = setup->m_armies[i];
            else
                m_army.m_armies[i] = CREATURE_NONE;
        }
    }

    if (setup->m_customSpells) {
        memset(m_inSpellbook, 0, sizeof(m_inSpellbook));
        memset(m_availableSpells, 0, sizeof(m_availableSpells));
        for (int i = 0; i < NUM_SPELLS; i++) {
            if (setup->m_spells.test(i)) {
                m_inSpellbook[i] = 1;
                m_availableSpells[i] = 1;
            }
        }
    }

    if (setup->m_customArtifacts) {
        int i;
        for (i = 0; i < 19; i++) {
            if (getArtifact(TArtifactSlot(i)).m_artifactId != ARTIFACT_NONE)
                removeArtifact(i);
        }
        for (i = 0; i < 19; i++) {
            if (setup->m_artifacts[i].m_artifactId != ARTIFACT_NONE)
                equipArtifact(&setup->m_artifacts[i], i);
        }
        for (i = 0; i < 64; i++)
            m_backpack[i] = type_artifact();
        for (i = 0; i < 64; i++) {
            if (setup->m_backpack[i].m_artifactId != ARTIFACT_NONE)
                addToBackpack(&setup->m_backpack[i], -1);
        }
    }

    if (setup->m_sex != -1)
        m_sex = setup->m_sex;

    if (setup->m_customName) {
        m_hasCustomName = 1;
        // Retail keeps basic_string::assign an out-of-line CALL here; our
        // CL expanded it and spilled its internals (_Grow x2, _Split x2,
        // _Eos, memmove, operator delete) into this body. inline_depth(0)
        // is STATEMENT-granular in VC6, so it pins this one site.
        // And the call retail makes is the THREE-argument assign, not
        // operator=: push npos, push 0, push src, call assign(str,I,I),
        // with npos LOADED from its out-of-line definition. Spelling the
        // three-argument form under the same pin is worth 95.0000 ->
        // 96.7770; unpinned it collapses (3-arg 55.7360, 1-arg 55.5946).
#pragma inline_depth(0)
        m_customName.assign(setup->m_name, 0, std::string::npos);
#pragma inline_depth()
    }

    if (setup->m_customExperience) {
        m_experience = 0;
        int amount = setup->m_experience;
        // The one campaign that starts its hero at a level derived from
        // another scenario's hero. GetExperience (0x4da3a0) is EMITTED
        // AFTER this body, so VC6 cannot inline it here - yet its ladder
        // is expanded in place at 0x4d999b, induction rewrite and all.
        // The only consistent reading is that retail's source repeats the
        // statements rather than calling it.
        if (g_campaignMode
            && g_game->m_campaign.m_currentCampaign == g_startLevelCampaign
            && g_game->m_campaign.m_currentMap == g_startLevelScenario) {
            int level = g_game->m_heroes[g_startLevelHeroId].m_level
                       + g_startLevelBonus;
            if (level <= 12) {
                amount = g_experienceForLevel[level - 1];
            } else {
                amount = g_experienceForLevel[11];
                int increment = static_cast<int>(
                    (amount - g_experienceForLevel[10]) * 1.2);
                amount += increment;
                for (int i = 13; i < level; i++) {
                    increment = static_cast<int>(increment * 1.2);
                    amount += increment;
                }
            }
        }
        giveExperience(amount, 1, 0);
        checkLevel();
    }

    m_mana = static_cast<short>(getMaxMana());
    m_maxMovePoints = m_movePoints = getMobility((m_flags >> 18) & 1);
}

// 0x004d8f70 `ret 0`: returns a string - the campaign override
// (hero id 0x1b under scenario 0xf) or akHeroClasses[class].field_4,
// the 64-byte-stride class record at 0x67dcec.
VA(0x004d8f70, 0x3E)
const char* hero::heroFn004D8F70()
{
    if (m_id == CLASS_NAME_OVERRIDE_HERO_ID && g_campaignMode &&
        g_game->m_campaign.m_currentCampaign == CLASS_NAME_OVERRIDE_SCENARIO)
        return g_generalText->getText(GENERAL_TEXT_CAMPAIGN_HERO_CLASS);
    return g_heroClasses[m_heroClass].m_className;
}

// 0x004d8fb0 `ret 0`: the custom-name path - returns the +0x3de pointer
// when the +0x3d9 flag is set (falling back to the empty literal at
// 0x63a608), otherwise strcmp's the +0x23 name band against
// akHeroTraits[id] and substitutes the shared name table 0x6a66d8[id]
// only while the live name still equals its default.
VA(0x004d8fb0, 0xA0)
const char* hero::heroFn004D8FB0()
{
    const char* emptyName = DATA_COMPGEN(0x0063a608, heroNameEmptyText,
                                         "");

    if (m_hasCustomName)
        return m_customName.c_str();

    if (g_campaignMode &&
        g_game->m_campaign.m_currentCampaign != CUSTOM_NAME_CAMPAIGN_EXCLUDED_SCENARIO &&
        g_currentHero->m_portrait == CUSTOM_NAME_CAMPAIGN_PORTRAIT)
        return g_campaignHeroName;

    const char* heroName = m_name;
    const char* defaultName = g_heroTraits[m_id].m_defaultName;
    if (strcmp(heroName, defaultName) == 0)
        return g_sharedHeroNames[m_id];
    return heroName;
}

VA(0x004d9050, 0x20)  // dc 0xcc0bc
unsigned char hero::belongsToHuman() const
{
    if (m_owner < 0)
        return 0;
    return g_game->isHuman(m_owner) != 0;
}

VA(0x004d9070, 0x45)  // dc 0xcc0e4
long hero::getEquippedArtifacts(unsigned char countWarMachines) const
{
    long count = 0;
    for (int slot = 0; slot < 19; slot++) {
        int id = m_equipped[slot].m_artifactId;
        if (id != -1 && id != ARTIFACT_SPELLBOOK && !countWarMachines &&
            id != ARTIFACT_CATAPULT && id != ARTIFACT_BALLISTA &&
            id != ARTIFACT_AMMO_CART && id != ARTIFACT_FIRST_AID_TENT)
            count++;
    }
    return count;
}

VA(0x004d90c0, 0x4A)  // dc 0xcc138
long hero::getNumberInBackpack(unsigned char countWarMachines) const
{
    long count = 0;
    if (countWarMachines)
        return m_backpackCount;
    for (int slot = 0; slot < 64; slot++) {
        int id = m_backpack[slot].m_artifactId;
        if (id != -1 && id != ARTIFACT_CATAPULT && id != ARTIFACT_BALLISTA &&
            id != ARTIFACT_AMMO_CART && id != ARTIFACT_FIRST_AID_TENT)
            count++;
    }
    return count;
}

VA(0x004d9110, 0x4C)  // dc 0xcc1b0
hero_seqid hero::getStandSequence()
{
    switch (m_facing) {
    case kFacingN:
        return hs_stand_n;
    case kFacingNE:
    case kFacingNW:
        return hs_stand_ne;
    case kFacingSE:
    case kFacingSW:
        return hs_stand_se;
    case kFacingS:
        return hs_stand_s;
    }
    return hs_stand_e;
}

VA(0x004d9160, 0x4C)  // dc 0xcc1e8
hero_seqid boat::getStandSequence()
{
    switch (m_facing) {
    case hero::kFacingN:
        return hs_stand_n;
    case hero::kFacingNE:
    case hero::kFacingNW:
        return hs_stand_ne;
    case hero::kFacingSE:
    case hero::kFacingSW:
        return hs_stand_se;
    case hero::kFacingS:
        return hs_stand_s;
    }
    return hs_stand_e;
}

VA(0x004d91b0, 0x3F)  // dc 0xcc220
unsigned char hero::hasArtifact(int whichArtifact) const
{
    for (int slot = 0; slot < 19; slot++) {
        if (m_equipped[slot].m_artifactId == whichArtifact)
            return 1;
    }
    for (int pack = 0; pack < 64; pack++) {
        if (m_backpack[pack].m_artifactId == whichArtifact)
            return 1;
    }
    return 0;
}

VA(0x004d91f0, 0x70)  // dc 0xcc26c
unsigned char hero::isWieldingArtifact(int whichArtifact) const
{
    if (whichArtifact == ARTIFACT_SPELLBOOK) {
        return m_equipped[17].m_artifactId == ARTIFACT_SPELLBOOK;
    } else {
        for (int slot = 0; slot < 19; slot++) {
            if (m_equipped[slot].m_artifactId == whichArtifact)
                return 1;
        }
    }
    int combination = g_artifactTraits[whichArtifact].m_targetCombo;
    return combination != -1
        && isWieldingArtifact(g_combinationArtifacts[
               g_artifactTraits[whichArtifact].m_targetCombo].m_artifactId);
}

// E:\gamedcs\hero.cpp:1466
// Residual (96.5278%): 11 of 11 blocks, both branch targets, the jump table's
// four arms in source order and the equipped-slot walk are exact. Eight bytes
// differ, all one decision: retail loads the parameter into EAX, consumes it in
// place (`add eax,-0x91`) for the switch index, and RELOADS `[ebp+8]` in the
// not-matched arm; ours keeps the load in EDX (`lea eax,[edx-0x91]`) and reuses
// it (`mov esi,edx`), i.e. VC6 CSEs the two parameter reads for us and did not
// for retail. Measured byte-flat: default arm first/last, dropping its `break`,
// moving the CATAPULT arm last, `long`/`TArtifact` artifact, switching on a copy
// of the parameter, declaring `slot` outside the loop, `long slot`, and the
// while form - and EXHAUSTIVELY so: a 64-member source family over the switch
// head, the default arm and the loop form produced ONE object for all 64
// combinations, so nothing in this body reaches the residual. Measured WORSE:
// reassigning the parameter in the arms and dropping
// the default (89.86 - VC6 then enregisters the parameter at entry and the
// not-matched block disappears entirely), initialising `artifact` from the
// parameter before the switch (89.86, same cause), and reversing the equipped
// compare (96.25). DC records no locals for this body.
VA(0x004d9260, 0x68)  // dc-bracket forced, dc 0xcc2a8
void hero::destroySiegeWeaponArtifact(int creatureType)
{
    int artifact;
    switch (creatureType) {
    case CREATURE_CATAPULT:
        return;
    case CREATURE_BALLISTA:
        artifact = ARTIFACT_BALLISTA;
        break;
    case CREATURE_FIRST_AID_TENT:
        artifact = ARTIFACT_FIRST_AID_TENT;
        break;
    case CREATURE_AMMO_CART:
        artifact = ARTIFACT_AMMO_CART;
        break;
    default:
        artifact = creatureType;
        break;
    }
    // Nineteen equipped slots, one more than the DC build's eighteen.
    for (int slot = 0; slot < 19; slot++) {
        if (m_equipped[slot].m_artifactId == artifact) {
            removeArtifact(slot);
            return;
        }
    }
}

VA(0x004d92d0, 0x59)  // dc 0xcc300
void hero::useSpell(int cost)
{
    int remainingMana = max(m_mana - cost, 0);
    m_mana = remainingMana;
    if (g_advManager->m_status == baseManager::STATUS_ACTIVE &&
        g_currentPlayer->isLocalHuman())
        g_advManager->m_advWindow->updateHeroLocator(-1, 1, 1);
}
VA(0x004d9330, 0x1A)  // dc 0xcc348
void hero::addSpell(int whichSpell)
{
    m_inSpellbook[whichSpell] = 1;
    m_availableSpells[whichSpell] = 1;
}

// E:\gamedcs\hero.cpp:1527, dc 0xcc360.
// DC mark_spells is the ordinary global school helper called by the Tome
// arms of UpdateSpellList at dc 0xcc446. Complete collects each school's
// grants in a returned bitset: retail 0x4d9386..0x4d93d2 constructs a
// separate three-dword result, tests akSpellTraits.schoolBits, and copies
// it into the artifact result. The other three Tome arms repeat this.
std::bitset<70> markSpells(TSpellSchool school)
{
    std::bitset<70> granted(0);
    for (int spell = 0; spell < hero::NUM_SPELLS; spell++) {
        if (g_spellTraits[spell].m_schoolBits & school)
            granted[spell] = true;
    }
    return granted;
}

// The dispatch is `lea eax,[id-0x56] / cmp eax,0x31 / ja default` over a
// 50-byte index table at +0x240 selecting a nine-entry dword jump table
// at +0x21c. Read out of the image the 50 index bytes collapse to EIGHT
// real cases; every other id in 0x56..0x87 falls to the bare epilogue.
// The arm order below is the EMITTED order, hence source order.

// Two shape facts are in the bytes rather than inferred. First, the four
// Tome arms build a SEPARATE bitset at [ebp-0x28] and copy-assign all
// three dwords into the result, while the level arm and the four literal
// arms write the result in place at [ebp-0x1c] - the asymmetry is in the
// addressing, not in inlining. Second, `granted[spell] = true` is
// bitset::operator[] plus reference::operator=(bool), and the first
// three Tome arms inline operator[] where the fourth calls it out of
// line: that is purely the /Ob2 budget running out mid-switch, so all
// four arms are written identically here.

// 92.71 -> 93.96 (2026-08-20): the diagnosis below was right about WHICH
// sites diverged, and `#pragma inline_depth(0)` on the Sea Captain's
// set(SPELL_SUMMON_BOAT) site alone is the lever - it is STATEMENT-
// granular in VC6, so it converts exactly that one expansion into the
// call retail has, taking the census from set x2 to retail's x3. The
// Armageddon's Blade site is deliberately LEFT expanded: retail calls set
// three times out of four, not four, so pinning both would overshoot.

// Residual (93.96%): Armageddon's Blade still folds to
// `or dword ptr [result], imm` - which is what retail does at one of its
// four sites too, so this may already be right and the remainder
// elsewhere. Everything
// else - both loops, the jump tables, the tail-merged set chain, the
// `result[SPELL_TITANS_LIGHTNING_BOLT] = false` epilogue and its
// registers - agrees. Tried and rejected: the DEFAULT bitset ctor, which
// is what retail's `_Tidy` calls actually prove the source used, but
// which frees enough budget to lose the operator[] pair (90.49%, and
// 84.86% before the level helper); its call shape is identical to the
// `(0)` ctor's at all five sites, so no byte is given up by spelling it
// this way.
VA(0x004d9350, 0x272)  // retail artifact-id dispatch + bitset return, retail-only
std::bitset<70> markArtifactSpells(int artifactId)
{
    std::bitset<70> result(0);
    switch (artifactId) {
    case ARTIFACT_TOME_OF_AIR_MAGIC:
        result = markSpells(eSchoolAir);
        break;
    case ARTIFACT_TOME_OF_FIRE_MAGIC:
        result = markSpells(eSchoolFire);
        break;
    case ARTIFACT_TOME_OF_WATER_MAGIC:
        result = markSpells(eSchoolWater);
        break;
    case ARTIFACT_TOME_OF_EARTH_MAGIC:
        result = markSpells(eSchoolEarth);
        break;
    case ARTIFACT_SPELLBINDERS_HAT: {
        for (int spell = 0; spell < hero::NUM_SPELLS; spell++) {
            if (g_spellTraits[spell].m_level == g_fifthLevelSpell)
                result.set(spell, true);
        }
        break;
    }
    case ARTIFACT_ARMAGEDDONS_BLADE:
        result[SPELL_ARMAGEDDON] = true;
        break;
    case ARTIFACT_SEA_CAPTAINS_HAT:
        result[SPELL_SUMMON_BOAT] = true;
        result[g_spellScuttleBoat] = true;
        break;
    case ARTIFACT_TITANS_THUNDER:
        result[SPELL_TITANS_LIGHTNING_BOLT] = true;
        break;
    }
    if (artifactId != ARTIFACT_TITANS_THUNDER)
        result[SPELL_TITANS_LIGHTNING_BOLT] = false;
    return result;
}

VA(0x004d95d0, 0x212)  // dc 0xcc38c
void hero::updateSpellList()
{
    std::copy(m_inSpellbook, m_inSpellbook + NUM_SPELLS, m_availableSpells);

    int remaining = 19;
    const type_artifact* slot = m_equipped;
    do {
        type_artifact current = *slot;
        int artifactId = current.m_artifactId;
        long extra = current.m_extra;
        if (artifactId != ARTIFACT_NONE) {
            if (artifactId == ARTIFACT_SPELL_SCROLL) {
                m_availableSpells[extra] = 1;
            } else {
                if (g_artifactTraits[artifactId].m_givesSpells) {
                    std::bitset<70> granted = markArtifactSpells(artifactId);
                    std::transform(m_availableSpells,
                                   m_availableSpells + NUM_SPELLS,
                                   bitset_iterator<70>(granted, 0),
                                   m_availableSpells, std::logical_or<bool>());
                }
                int comboType = g_artifactTraits[artifactId].m_comboType;
                if (comboType != -1) {
                    const std::bitset<144>& components =
                        g_combinationArtifacts[comboType].m_components;
                    for (int component = 0; component < 144; component++) {
                        if (components.test(component) &&
                            g_artifactTraits[component].m_givesSpells) {
                            std::bitset<70> granted = markArtifactSpells(component);
                            std::transform(m_availableSpells,
                                           m_availableSpells + NUM_SPELLS,
                                           bitset_iterator<70>(granted, 0),
                                           m_availableSpells,
                                           std::logical_or<bool>());
                        }
                    }
                }
            }
        }
        slot++;
    } while (--remaining);
}

// E:\gamedcs\hero.cpp:1632
// Dreamcast's UpdateArmies source shape and retail agree: one message
// local, a seven-army-slot loop, and the same empty/populated/selected
// widget branches. The old order-map assignment to HeroMessageUpdate was
// a slot-alignment error, not retail evidence for a different identity.
// Seven army slots, widget ids 0x44..0x4a, each with an icon at id-14,
// a count text at id-7 and the slot itself. The literal 4 does double
// duty as WIDGET_SET_ICON_FRAME and as WIDGET_DRAWN - retail CSEs it
// into EBX and uses the one register for both fields.

// Residual (96.72%): the Dreamcast-attested 0..6 army-index loop makes
// every body block exact. Only the retail loop tail schedules the implicit
// strength-reduced army cursor after incrementing the biased widget IV;
// our CL schedules the cursor first. The explicit cursor spelling reaches
// 99.04% but contradicts the source-line shape and is intentionally not
// kept. why-reg classifies the remainder as a two-instruction C3 schedule,
// not a register binding.
VA(0x004d97f0, 0x1A0)  // source-shape + retail body, dc 0xcc540
void hero::updateArmies()
{
    message msg;
    msg.m_id = MESSAGE_WIDGET;

    for (int slot = 0; slot < 7; ++slot) {
        if (m_army.m_armies[slot] == CREATURE_NONE) {
            msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
            msg.m_extra = widget::WIDGET_DRAWN;
            msg.m_codeY = slot + 0x36;
            g_heroScreenWindow->broadcastMessage(msg);
            msg.m_codeY = slot + 0x3d;
            g_heroScreenWindow->broadcastMessage(msg);
            if (g_heroScreenArmyStripLive)
                msg.m_codeX = widget::WIDGET_SET_STATUS;
            msg.m_codeY = slot + 0x44;
            g_heroScreenWindow->broadcastMessage(msg);
            continue;
        }

        msg.m_codeX = widget::WIDGET_SET_ICON_FRAME;
        msg.m_extra = m_army.m_armies[slot] + 2;
        msg.m_codeY = slot + 0x36;
        g_heroScreenWindow->broadcastMessage(msg);
        msg.m_codeX = widget::WIDGET_SET_STATUS;
        msg.m_extra = widget::WIDGET_DRAWN;
        g_heroScreenWindow->broadcastMessage(msg);

        sprintf(g_text, "%d", m_army.m_numTroops[slot]);
        msg.m_codeX = widget::WIDGET_SET_TEXT;
        msg.m_codeY = slot + 0x3d;
        msg.m_extraText = g_text;
        g_heroScreenWindow->broadcastMessage(msg);
        msg.m_codeX = widget::WIDGET_SET_STATUS;
        msg.m_extra = widget::WIDGET_DRAWN;
        g_heroScreenWindow->broadcastMessage(msg);

        if (g_heroScreenArmySlot == slot) {
            if (g_heroScreenArmyStripLive)
                g_heroScreenWindow->widgetClearStatus(slot + 0x44,
                                                      widget::WIDGET_DRAWN);
            else
                g_heroScreenWindow->widgetSetStatus(slot + 0x44,
                                                    widget::WIDGET_DRAWN);
        } else if (g_heroScreenArmyStripLive && g_heroScreenArmySlot >= 0 &&
                   m_army.m_armies[slot] == m_army.m_armies[g_heroScreenArmySlot]) {
            g_heroScreenWindow->widgetSetStatus(slot + 0x44,
                                                widget::WIDGET_DRAWN);
        } else {
            g_heroScreenWindow->widgetClearStatus(slot + 0x44,
                                                  widget::WIDGET_DRAWN);
        }
    }
}

// Retail reads both arguments, expands GetPrimarySkill, and passes gStatDesc
// plus the quick/normal dialog type to NormalDialog. DC confirms the same
// calls and arguments; the old HeroScreenUpdate association was positional.
// E:\gamedcs\hero.cpp:1709, dc 0xcc708
VA(0x004d9990, 0x65)  // stat-dialog semantics and two-argument ABI, dc 0xcc708
void hero::viewStat(int whichStat, int isQuickView)
{
    unsigned short statValue = getPrimarySkill(whichStat);
    normalDialog(g_statDesc[whichStat],
                 isQuickView ? hero::PRIMARY_STAT_QUICK_DIALOG_TYPE
                             : hero::PRIMARY_STAT_DIALOG_TYPE,
                 -1, PRIMARY_STAT_DIALOG_Y,
                 whichStat + PRIMARY_STAT_RESOURCE_FIRST,
                 statValue | PRIMARY_STAT_RESOURCE_QUANTITY,
                 -1, 0, -1, 0, -1, 0);
}

// the same dialog-type pair viewStat uses (4 quick, 1 normal),
VA(0x004d9a00, 0x128)  // dc 0xcc75c
void hero::viewArtifact(const type_artifact* artifact, int isQuickView)
{
    if (artifact->m_artifactId == ARTIFACT_SPELL_SCROLL) {
        normalDialog(artifact->getDescription().c_str(),
                     isQuickView ? hero::PRIMARY_STAT_QUICK_DIALOG_TYPE
                                 : hero::PRIMARY_STAT_DIALOG_TYPE,
                     -1, PRIMARY_STAT_DIALOG_Y, 9, artifact->m_extra,
                     -1, 0, -1, 0, -1, 0);
    } else {
        normalDialog(artifact->getDescription().c_str(),
                     isQuickView ? hero::PRIMARY_STAT_QUICK_DIALOG_TYPE
                                 : hero::PRIMARY_STAT_DIALOG_TYPE,
                     -1, PRIMARY_STAT_DIALOG_Y, -1, 0, -1, 0, -1, 0,
                     -1, 0);
    }
}

VA(0x004d9b30, 0x18D)  // combination-artifact caller + settled retail ABI
int hero::heroFn004D9B30(int artifact)
{
    // Complete's combination prompt receives an integer id; its record constructor retains the older DC TArtifact API.
    type_artifact record(static_cast<TArtifact>(artifact) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */);
    std::string text = record.getDescription();
    text += "\n\n";
    text += g_generalText->getText(734);
    normalDialog(text.c_str(), 2, -1, PRIMARY_STAT_DIALOG_Y, -1, 0, -1, 0,
                 -1, 0, -1, 0);
    return g_windowManager->m_dialogReturn;
}

// Retail-only body immediately after the independently mapped ViewArtifact.
// The ASSEMBLE partner of 0x4d9b30 above, same arity settlement: `ret 4`,
// ECX untouched, one artifact id in and the dialog reply out. It reads
// the component's targetCombo, describes the ASSEMBLED artifact, and
// asks general text 733 - the same prompt HeroFn_004DC100 uses - with
// the assembled artifact's name formatted in and its icon (resource
// type 8) shown beside the question. Retail reuses the incoming
// parameter slot for the assembled id, which is why the format argument
// and the dialog's resource extra are the same value.
// The record describes the COMPONENT, not the assembled artifact -
// retail's `mov [ebp-0x14], ecx` takes the incoming parameter straight
// into the stack type_artifact - and `assembled` is computed FIRST even
// though the record is used first (94.17 against 85.75 for the other
// order, and 93.66 for the semantically wrong `record(assembled, -1)`).
// Residual (94.2%): prologue instruction SCHEDULING only - the same
// instructions, permuted around the two pushes - plus the unwind-table
// addend in the frame push, which is a relocation and not a state count.
VA(0x004d9cc0, 0x200)  // retail body + settled arity; old DC bracket retired
int hero::heroFn004D9CC0(int artifact)
{
    int assembled =
        g_combinationArtifacts[g_artifactTraits[artifact].m_targetCombo]
            .m_artifactId;
    // Complete's combination prompt receives an integer id; its record constructor retains the older DC TArtifact API.
    type_artifact record(static_cast<TArtifact>(artifact) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */);
    std::string text = record.getDescription();
    text += "\n\n";
    text += formatString(g_generalText->getText(733),
                          g_artifactTraits[assembled].m_name);
    normalDialog(text.c_str(), 2, -1, PRIMARY_STAT_DIALOG_Y, 8, assembled,
                 -1, 0, -1, 0, -1, 0);
    return g_windowManager->m_dialogReturn;
}

VA(0x004d9ec0, 0x4D3)  // dc 0xcc800
void hero::deallocate(unsigned char gameLoaded, unsigned char remoteMove)
{
    unsigned char freedTownVisitor = 0;
    int townId = g_game->getTownId(m_x, m_y, m_z);
    if (townId >= 0) {
        town* visited = g_game->getTown(townId);
        if (visited->m_garrisonHeroId == m_id) {
            visited->m_garrisonHeroId = -1;
            freedTownVisitor = 1;
        }
    }

    if (gameLoaded && !remoteMove) {
        type_point location;
        location.m_x = m_x;
        location.m_y = m_y;
        location.m_z = m_z;
        CMCDeadHero change(m_id, location);
        sendMapChange(&change);
        g_game->recordHideHero(this, -1, freedTownVisitor);
    }

    int oldOwner = m_owner;
    playerData* player = &g_game->m_players[oldOwner];
    if (gameLoaded) {
        g_advManager->mobilizeCurrHero(0, 0, 1);
        g_advManager->hideRoute(0, 0, 0);
    }

    if (m_flags & 0x40000) {
        g_game->getHeroBoat(m_id, 1)->m_allocated = 0;
        m_flags &= 0xfffbffff;
    }

    type_obscuring_object::restoreCell();

    if (!g_combatFlag697744) {
        for (int slot = 0; slot < 7; slot++)
            m_army.dismiss(slot);
    }

    int pos = player->findHero(m_id);
    if (pos >= 0) {
        for (int i = pos; i < player->m_numHeroes - 1; i++)
            player->m_heroes[i] = player->m_heroes[i + 1];
        player->m_heroes[player->m_numHeroes - 1] = -1;
        player->m_numHeroes--;
    }
    if (player->m_currHeroId == m_id) {
        player->m_currHeroId = -1;
        if (g_netLocalGamePos == m_owner)
            g_advManager->m_drawCursor = 0;
        if (oldOwner == g_netLocalGamePos)
            g_advManager->m_curHeroMobile = 0;
    }
    g_advManager->m_advWindow->updateHeroLocators(0, 1, 1);
    g_game->m_heroAvailability[m_id] = -1;

    if (g_combatFlag6985a3 || g_combatFlag697744) {
        int slot = random(0, 1);
        int other = g_game->m_players[m_owner].m_recruits[slot];
        if (other != -1) {
            if (g_game->m_heroes[other].m_flags & 0x20000) {
                slot = 1 - slot;
                other = g_game->m_players[m_owner].m_recruits[slot];
            }
            if (other != -1 && g_game->m_heroAvailability[other] == HERO_AVAILABILITY_TAVERN_POOL)
                g_game->m_heroAvailability[other] = -1;
        }
        g_game->m_players[m_owner].m_recruits[slot] = m_id;
        g_game->m_heroAvailability[m_id] = HERO_AVAILABILITY_TAVERN_POOL;
        m_flags |= 0x20000;
    }

    if (g_campaignMode) {
        switch (g_game->m_campaign.m_currentCampaign) {
        case DEALLOCATE_CAMPAIGN_BY_PORTRAIT:
            if (m_portrait == DEALLOCATE_KEPT_PORTRAIT)
                g_game->m_heroAvailability[m_id] = HERO_AVAILABILITY_TAVERN_POOL;
            break;
        case DEALLOCATE_CAMPAIGN_BY_HERO_ID:
            if (m_id == DEALLOCATE_KEPT_HERO_ID)
                g_game->m_heroAvailability[DEALLOCATE_KEPT_HERO_ID] =
                    HERO_AVAILABILITY_TAVERN_POOL;
            break;
        }
    }

    m_owner = -1;
    m_pathTargetY = -1;
    m_pathTargetX = -1;
    if (!(m_flags & 0x20000)) {
        m_mana = static_cast<short>(getMaxMana());
        m_maxMovePoints = m_movePoints = getMobility((m_flags >> 18) & 1);
    }

    if (!g_combatFlag697744)
        g_game->setRandomHeroArmies(m_id, 0, 1);
}

VA(0x004da3a0, 0x76)  // dc 0xccb80
int hero::getExperience(int level)
{
    if (level <= 12)
        return g_experienceForLevel[level - 1];
    int increment = static_cast<int>(
        (g_experienceForLevel[11] - g_experienceForLevel[10]) * 1.2);
    int total = g_experienceForLevel[11] + increment;
    for (int i = 13; i < level; i++) {
        increment = static_cast<int>(increment * 1.2);
        total += increment;
    }
    return total;
}

VA(0x004da420, 0xE4)  // dc 0xccc68
int hero::getExperienceIncrement(int level)
{
    return getExperience(level + 1) - getExperience(level);
}

// E:\gamedcs\hero.cpp:1862
// No retail row: the carve is gap-free from GetExperienceIncrement
// straight into ApplyBattleWinTemps, and hero::GiveExperience carries
// the whole body expanded. `inline` reproduces the absence, the
// strip::DrawNumber precedent. Same table and same 1.2 extrapolation as
// GetExperience above, walked forwards instead of indexed.
DC_ONLY(0xccc8c, 0x110)
inline int hero::getLevel(int experience)
{
    int heroLevel = 1;
    // INDEX loop, not a pointer walk: retail closes this with `jle`, and a
    // C++ pointer relational compare is UNSIGNED (`jbe`). VC6 strength-
    // reduces the signed `i <= 11` into the pointer form retail emits while
    // keeping the original compare's signedness.
    for (int i = 0; i <= 11; i++, heroLevel++) {
        if (experience < g_experienceForLevel[i])
            return heroLevel - 1;
    }
    int total = g_experienceForLevel[11];
    int increment =
        static_cast<int>((total - g_experienceForLevel[10]) * 1.2);
    total += increment;
    for (heroLevel = 13; experience >= total; heroLevel++) {
        increment = static_cast<int>(increment * 1.2);
        total += increment;
    }
    return heroLevel - 1;
}

VA(0x004da510, 0x1F1)  // dc 0xccd9c
void hero::applyBattleWinTemps()
{
    m_moraleBonus = m_luckBonus = 0;
    if (m_flags & 0x8000)
        m_flags -= 0x8000;
    if (m_flags & 0x400)
        m_flags -= 0x400;
    if (m_flags & 0x200)
        m_flags -= 0x200;
    if (m_flags & 0x4)
        m_flags -= 0x4;
    if (m_flags & 0x80)
        m_flags -= 0x80;
    if (m_flags & 0x100)
        m_flags -= 0x100;
    if (m_flags & 0x4000000)
        m_flags -= 0x4000000;
    if (m_flags & 0x8)
        m_flags -= 0x8;
    if (m_flags & 0x10)
        m_flags -= 0x10;
    if (m_flags & 0x2000000)
        m_flags -= 0x2000000;
    if (m_flags & 0x20)
        m_flags -= 0x20;
    if (m_flags & 0x8000000)
        m_flags -= 0x8000000;
    if (m_flags & 0x10000000)
        m_flags -= 0x10000000;
    if (m_flags & 0x20000000)
        m_flags -= 0x20000000;
    if (m_flags & 0x4000)
        m_flags -= 0x4000;
    if (m_flags & 0x40)
        m_flags -= 0x40;
    if (m_flags & 0x800)
        m_flags -= 0x800;
    if (m_flags & 0x100000)
        m_flags -= 0x100000;
    if (m_flags & 0x1000)
        m_flags -= 0x1000;
    if (m_flags & 0x2000)
        m_flags -= 0x2000;
    if (m_flags & 0x10000)
        m_flags -= 0x10000;
    if (m_flags & 0x200000)
        m_flags -= 0x200000;
}

VA(0x004da710, 0x5)  // dc 0xccf68
void hero::applyBattleLossTemps()
{
    applyBattleWinTemps();
}

// hero::GetLevel (dc 0xccc8c, 272 B) has NO retail row: the carve is
// gap-free from GetExperienceIncrement 0x004da420 straight into
// ApplyBattleWinTemps 0x004da510. Left DC_ONLY above.

// Retail .bss 0x698400. 0x4bfe70 sets it for the whole of its
// scenario-setup pass and clears it at the end; every reader treats
// nonzero as "suppress the interactive path" - game::ClaimTown skips its
// notify call, CheckLevel skips the level-up window entirely. Eight
// references image-wide, three in the body below. NAME UNATTESTED,
// address-ordinal placeholder.
DATA(0x00698400) extern int g_inSetup698400;

TSecondarySkill aiChooseSecondarySkill(const hero* ourHero,
                                          TSecondarySkill first,
                                          TSecondarySkill second,
                                          unsigned char complexChoice);

// get_skill_award is defined further down (0x4dad00): retail emits this
// caller FIRST, so the helper needs a declaration here. Being only
// declared is also what keeps /Ob2 from expanding it - retail calls it
// four times.
TSecondarySkill getSkillAward(const hero* currentHero,
                                TSkillMastery minLevel,
                                TSkillMastery maxLevel,
                                TSecondarySkill excluded);

// STATIC-HELPER-AFTER-CALLER: retail emits CheckLevel BEFORE
// get_skill_award, the reverse of the DC source order. Proven by the
// call graph, not by rank - 0x004da720 calls 0x004dad00, and it also
// calls philai's AI_choose_secondary_skill and hero::GiveSS
// (0x004e22d0), which is what a level-up does. The two bodies also
// disagree on arity in the direction the prototypes demand: 0x004da720
// reads ecx only (`void hero::CheckLevel()`), 0x004dad00 reads ecx AND
// edx, i.e. the /Gr free function `get_skill_award(hero*, ...)`.
// E:\gamedcs\hero.cpp:2147

// The level-up loop. EH-bearing, but NOT from a source try/catch: three
// TLevelUpWindow locals in three mutually exclusive arms plus /GX give
// the fs:[0] frame and the 0/1/2 trylevel by themselves, and the
// prologue's `push 0xb` is a relocation ADDEND, not a state count.
// hero::GetLevel is /Ob2-expanded at the top (it is `inline` and defined
// above); get_skill_award is not, and must not be. The magic-school test
// on skills[0] really is emitted TWICE - once before skills[1] is drawn
// and again inside the two-entry loop - so it is written twice here.
// The inner braces in the third arm are load-bearing: that window's
// destructor runs BEFORE the dialogReturn chain, where the second arm's
// GiveSS runs INSIDE its window's lifetime.

// Residual (87.13%): three source facts landed 2026-09-05, in this order.
// (1) The SRand seed constant is 154079 (0x259df), read straight off
// retail's `lea ecx,[eax+ecx+0x259df]`; the tree had 153567 (0x257df).
// (2) VC6 lowers `signed * 214013` with `imul r,r,0x343fd` but expands the
// SAME constant into retail's seven-step lea/shift chain when the
// multiplicand is UNSIGNED (`lea [eax+2*eax]` 3x, `lea [eax+4*ecx]` 13x,
// `shl 4` 208x, `add` 209x, `shl 8` 53504x, `sub` 53503x,
// `lea [eax+4*edx]` 214013x). `static_cast<unsigned>(level)` is worth
// 85.17 -> 87.13; the sibling `iLevelSeed * 156823` stays signed and
// retail keeps its `imul` there, which corroborates the split.
// (3) `int roll = Random(1,100);` is declared BEFORE `int stat = 0;` -
// retail schedules `xor esi,esi / mov [ebp-0x14],esi` between the
// `cmp cx,9` operand loads, after the Random call (84.18 -> 85.17); and
// both `chances` selections are written `if (level <= LOW_LEVEL_LAST)
// <plain>; else <10P>`, which is the fall-through polarity retail has
// (`jg` to the 10P arm), worth 83.44 -> 84.18.
// Rejected at the new plateau: reordering the SRand sum (byte-flat three
// ways) and a nested `if` with an `unsigned char isOverrideHero` local for
// retail's `sete dl` (85.41, WORSE).

// Earlier history: closed from 46.28 by pinning hero::GiveSS with
// `#pragma auto_inline(off)` - `predict-inline` named it the sole
// OVER-inline, expanded at all six sites here where retail keeps a real
// call at the two that survive cross-jumping, worth 23 conditional
// branches. Tried and rejected since, one compile each: the campaign
// gate's third operand bound to an `unsigned char` local to chase
// retail's `sete dl` (81.23, WORSE - the branch-kind report names the
// symptom, not the lever, exactly as it did on UpdateStats); and
// `Random(1, chances[1] + chances[0])` for the load order (byte-flat).
// hero::GetLevel is deliberately left as a pointer walk - it already
// emits retail's signed `jle`, so the pointer-compare rule does not
// apply and respelling would risk the now-exact GiveExperience.

// THE AI_choose_secondary_skill x1-vs-x4 IS OUR CROSS-JUMP, and the bytes
// name the mechanism (2026-08-20). Retail keeps `call
// AI_choose_secondary_skill / push eax` inside each of the four arms and
// merges only at the shared `mov ecx,ebx / call GiveSS`; the arms are at
// fn+0x533, +0x547, +0x5b7 and +0x5cb. Our CL merges one step earlier, at
// the AI_choose call itself, so the four arms become four two-push stubs
// jumping into one call site.
// What DIFFERS between retail's arms - and is presumably why its
// cross-jumper declined - is the SCHEDULE, not the content: the true arms
// materialise the constant into EAX and push it twice
// (`mov eax,1 / mov edx,esi / push eax / push eax / push edi`), sharing
// one register between GiveSS's second argument and complex_choice, while
// the false arms push the 1 as an immediate and take the 0 from
// `xor eax,eax`. That puts `mov edx,<second>` in FRONT of the pushes in
// the true arms and AFTER them in the false arms, so no two arm tails are
// identical. Ours pushes two immediates in every arm and schedules
// `mov edx,<second>` identically, which is exactly what a cross-jumper
// wants.
// MEASURED NEGATIVE, do not retry: binding the flag to an
// `unsigned char complexChoice` local in each of the four arms, to chase
// that shared-register materialisation, is BYTE-FLAT on this row (83.4342
// either way) and costs hero 92.1826 -> 92.1392 fuzzy elsewhere.
// The integer variants are now bounded too (2026-08-21): scoped `int one`
// in the true arms and scoped `int zero` in the false arms are each
// byte-flat at 83.4342, with AI_choose_secondary_skill still x1 against
// retail's x4.  Two force-inlined source-context helpers, one per polarity,
// also emit the baseline bytes and no helper bodies.  C1 normalises all
// three forms before C2 chooses this cross-jump set; no scalar/helper
// spelling reaches retail's four separately scheduled calls.
VA(0x004da720, 0x5DD)  // anchor-callgraph + arity, dc 0xcd17c
void hero::checkLevel()
{
    int newLevel = getLevel(m_experience);
    if (m_level != newLevel) {
        while (m_level < newLevel) {
            m_level = m_level + 1;
            sprintf(g_text,
                    g_generalText->getText(GENERAL_TEXT_LEVEL_UP_TITLE_FORMAT),
                    m_name);
            sRand(static_cast<unsigned>(m_level) * 214013
                  + m_levelSeed * 156823 + 154079);

            int roll = random(1, 100);
            int stat = 0;
            const signed char* chances;
            if (m_level <= LEVEL_UP_LOW_LEVEL_LAST)
                chances = g_heroClasses[m_heroClass].m_gainPrimarySkillChance;
            else
                chances = g_heroClasses[m_heroClass].m_gainPrimarySkillChance10P;
            if (g_campaignMode &&
                g_game->m_campaign.m_currentCampaign == LEVEL_UP_CAMPAIGN_OVERRIDE &&
                m_id == LEVEL_UP_OVERRIDE_HERO_ID) {
                if (m_level <= LEVEL_UP_LOW_LEVEL_LAST)
                    chances = g_heroClasses[classBarbarian]
                                  .m_gainPrimarySkillChance;
                else
                    chances = g_heroClasses[classBarbarian]
                                  .m_gainPrimarySkillChance10P;
                roll = random(1, chances[0] + chances[1]);
            }
            while (roll > chances[stat]) {
                roll -= chances[stat];
                stat++;
            }

            m_stats[stat]++;
            char text[200];
            sprintf(text, "\n%s +1", g_primarySkillNames[stat]);
            strcat(g_text, text);

            TSecondarySkill skills[LEVEL_UP_SKILL_CHOICES];
            skills[0] = getSkillAward(this, eMasteryBasic, eMasteryExpert,
                                        eSecSkillNone);
            if (skills[0] == eSecSkillNone)
                skills[0] = getSkillAward(this, eMasteryNone,
                                            eMasteryExpert, eSecSkillNone);
            if (skills[0] == eSecSkillSchoolOfFireMagic ||
                skills[0] == eSecSkillSchoolOfAirMagic ||
                skills[0] == eSecSkillSchoolOfWaterMagic ||
                skills[0] == eSecSkillSchoolOfEarthMagic)
                m_lastMagicSchoolLevel = m_level;

            skills[1] = getSkillAward(this, eMasteryNone, eMasteryBasic,
                                        skills[0]);
            if (skills[1] == eSecSkillNone)
                skills[1] = getSkillAward(this, eMasteryNone,
                                            eMasteryExpert, skills[0]);

            for (int i = 0; i < LEVEL_UP_SKILL_CHOICES; i++) {
                if (skills[i] == eSecSkillWisdom)
                    m_lastWisdom = static_cast<unsigned char>(m_level);
                if (skills[i] == eSecSkillSchoolOfFireMagic ||
                    skills[i] == eSecSkillSchoolOfAirMagic ||
                    skills[i] == eSecSkillSchoolOfWaterMagic ||
                    skills[i] == eSecSkillSchoolOfEarthMagic)
                    m_lastMagicSchoolLevel = m_level;
            }

            if (!g_inSetup698400 && m_owner >= 0 &&
                g_game->isLocalHuman(m_owner)) {
                launchSample("nwherolv.82m", -1, 3);
                if (g_videoPaused)
                    g_currentPlayer->isLocalHuman();

                if (skills[0] == eSecSkillNone) {
                    TLevelUpWindow window(this, stat, -1, -1);
                    if (g_game->isMultiplayer() &&
                        g_turnDuration69d630.isExpired())
                        g_dialogDeadline697784 = 15000;
                    window.doModal(0);
                } else if (skills[1] == eSecSkillNone) {
                    TLevelUpWindow window(
                        this, stat,
                        skills[0] * 3 + 3 + m_skillLevel[skills[0]], -1);
                    if (g_game->isMultiplayer() &&
                        g_turnDuration69d630.isExpired())
                        g_dialogDeadline697784 = 15000;
                    window.doModal(0);
                    giveSS(skills[0], 1);
                } else {
                    sprintf(text,
                            g_generalText->getText(
                                GENERAL_TEXT_LEVEL_UP_CHOICE),
                            g_skillMasteryNames[m_skillLevel[skills[0]]],
                            g_sSkillTraits[skills[0]].m_name,
                            g_skillMasteryNames[m_skillLevel[skills[1]]],
                            g_sSkillTraits[skills[1]].m_name);
                    strcat(g_text, text);
                    {
                        TLevelUpWindow window(
                            this, stat,
                            skills[0] * 3 + 3 + m_skillLevel[skills[0]],
                            skills[1] * 3 + 3 + m_skillLevel[skills[1]]);
                        if (g_game->isMultiplayer() &&
                            g_turnDuration69d630.isExpired())
                            g_dialogDeadline697784 = 15000;
                        window.doModal(0);
                    }
                    if (g_windowManager->m_dialogReturn ==
                        DIALOG_RETURN_TIMEOUT) {
                        if (!g_inSetup698400 && m_owner >= 0)
                            giveSS(aiChooseSecondarySkill(
                                       this, skills[0], skills[1], 1), 1);
                        else
                            giveSS(aiChooseSecondarySkill(
                                       this, skills[0], skills[1], 0), 1);
                    } else if (g_windowManager->m_dialogReturn ==
                               TLevelUpWindow::SKILLICON_1_ID) {
                        giveSS(skills[0], 1);
                    } else if (g_windowManager->m_dialogReturn ==
                               TLevelUpWindow::SKILLICON_2_ID) {
                        giveSS(skills[1], 1);
                    }
                }
            } else if (skills[0] != eSecSkillNone) {
                if (skills[1] == eSecSkillNone)
                    giveSS(skills[0], 1);
                else if (!g_inSetup698400 && m_owner >= 0)
                    giveSS(aiChooseSecondarySkill(
                               this, skills[0], skills[1], 1), 1);
                else
                    giveSS(aiChooseSecondarySkill(
                               this, skills[0], skills[1], 0), 1);
            }
        }
        m_level = newLevel;
    }
}

// E:\gamedcs\hero.cpp:2014
// /Gr free function: ecx = current_hero, edx = min_level, the remaining
// two on the stack. Body reads the 64-byte-stride hero-class record
// (akHeroClasses, 0x67dcec, indexed by hero+0x30) for the per-class
// skill probabilities plus a campaign-specific override - exactly the
// inputs a skill award needs - and its ONLY caller is CheckLevel.

// Three weighted draws in one body: the guaranteed-Wisdom window
// (lastWisdom + 3 or 6 <= level), the guaranteed-magic-school window
// (last_magic_school_level + 3 or 4 <= level) drawn out of the four-school
// table, then the general 28-skill draw. The per-scenario disabled-skill
// row is gpGame->field_4e658, swapped for a private campaign row under
// the SAME campaign/hero gate hero::CheckLevel uses - which is what pins
// LEVEL_UP_CAMPAIGN_OVERRIDE / _OVERRIDE_HERO_ID as a shared pair rather
// than two coincidences. `min_level` is assigned in the body, so it stays
// a by-value parameter.

// The general draw returns its loop COUNTER as a TSecondarySkill. That
// conversion goes through a union overlay, NOT static_cast: a cast into
// an enum domain is a cleanliness floor at zero in this tree, and the
// overlay is byte-inert (the value is already in EAX).

// WALL (98.8073%, audited 2026-08-22): spelling both weighted passes as
// `if (!skillDisabled[i]) chance = trait; else chance = 0` restores
// retail's fall-through load and raises this row from 94.1284%.  All 75
// blocks, 51 conditional branches and both returns now agree.  The sole
// explicit-code residual is the campaign gate's final id comparison:
// retail materializes it as `sete cl; test cl,cl; je`, while this VC6
// compile folds every honest spelling to the equivalent direct `jne`.
// A named byte, explicit 1/0 arms, force/ordinary inline predicates with
// byte and bool returns (including a parameterized form), and a class-body
// inline accessor were byte-flat.  `why-branch` found only two unrelated
// D13 mutations and neither moved the branch shape.  The remaining flat-asm
// rows are name-only relocations for already-proven globals and the two ends
// of the four-entry magic-school table; no source operand differs.
// Three more materialisation spellings measured 2026-09-06, all byte-flat at
// 98.8073: `!(id != LEVEL_UP_OVERRIDE_HERO_ID)`, `(id == ...) != 0` and
// `(id == ...) == true`.  VC6 folds every one back to the direct `jne`, so
// the `sete cl / test cl,cl / je` triple is not reachable from the operand
// side of this comparison.
// Polish lane 37 found the construct that DOES emit the triple under this
// exact toolchain, by scanning every base object for `set(n)e r8` followed
// by `test r8,r8`: 148 sites, and the clean witness is
// victorylossconditions.cpp's `same_team` - a PLAIN `static bool` free
// function DEFINED IN THE .CPP with no `inline` keyword, whose body is one
// comparison. /Ob2 auto-inlining (not `inline` expansion) is what leaves the
// return value materialized, which is why every header `inline` predicate
// measured above came back byte-flat: the two expansions run through
// different paths in C1XX. The same gate is byte-for-byte identical in
// hero::CheckLevel (0x4da720) and townmgr.cpp:3745, so one admissible
// spelling would pay three times. It is NOT admissible here: the Dreamcast
// hero.obj roster names only get_skill_award, handle_artifact_click,
// handle_backpack_click and initialize_move_constants as hero.cpp statics,
// so a fourth file-static predicate would be invented source. Recorded as a
// lead, not a fix.
VA(0x004dad00, 0x283)  // anchor-caller + arity, dc 0xccf78
TSecondarySkill getSkillAward(const hero* currentHero, TSkillMastery minLevel, TSkillMastery maxLevel, TSecondarySkill excluded)
{
    int heroClass = currentHero->m_heroClass;
    const THeroClassTraits& classTraits = g_heroClasses[heroClass];
    const char* skillDisabled = g_game->m_ssDisabled;
    if (g_campaignMode &&
        g_game->m_campaign.m_currentCampaign == hero::LEVEL_UP_CAMPAIGN_OVERRIDE &&
        currentHero->m_id == hero::LEVEL_UP_OVERRIDE_HERO_ID)
        skillDisabled = g_campaignDisabledSkills;

    if (currentHero->m_skillCount >= 8)
        minLevel = eMasteryBasic;
    if (minLevel >= maxLevel)
        return eSecSkillNone;

    int wisdomGap;
    int magicGap;
    if (heroClass == classCleric || heroClass == classDruid ||
        heroClass == classWizard || heroClass == classHeretic ||
        heroClass == classNecromancer || heroClass == classWarlock ||
        heroClass == classBattleMage || heroClass == classWitch) {
        wisdomGap = 3;
        magicGap = 3;
    } else {
        wisdomGap = 6;
        magicGap = 4;
    }

    int level = currentHero->m_level;
    if (currentHero->m_lastWisdom + wisdomGap <= level &&
        currentHero->m_skillLevel[eSecSkillWisdom] < maxLevel &&
        currentHero->m_skillLevel[eSecSkillWisdom] >= minLevel &&
        excluded != eSecSkillWisdom &&
        !skillDisabled[eSecSkillWisdom])
        return eSecSkillWisdom;

    int i;
    if (currentHero->m_lastMagicSchoolLevel + magicGap <= level &&
        excluded != eSecSkillSchoolOfFireMagic &&
        excluded != eSecSkillSchoolOfAirMagic &&
        excluded != eSecSkillSchoolOfWaterMagic &&
        excluded != eSecSkillSchoolOfEarthMagic) {
        int schoolTotal = 0;
        for (i = 0; i < 4; i++) {
            TSecondarySkill school = g_magicSchools[i];
            if (currentHero->m_skillLevel[school] < maxLevel &&
                currentHero->m_skillLevel[school] >= minLevel &&
                !skillDisabled[school]) {
                if (currentHero->m_skillLevel[school] > 0)
                    schoolTotal++;
                else
                    schoolTotal += classTraits.m_gainSecondarySkillChance[school];
            }
        }
        if (schoolTotal > 0) {
            int schoolRoll = random(1, schoolTotal);
            for (i = 0; i < 4; i++) {
                TSecondarySkill school = g_magicSchools[i];
                if (currentHero->m_skillLevel[school] < maxLevel &&
                    currentHero->m_skillLevel[school] >= minLevel &&
                    !skillDisabled[school]) {
                    if (currentHero->m_skillLevel[school] > 0)
                        schoolRoll--;
                    else
                        schoolRoll -=
                            classTraits.m_gainSecondarySkillChance[school];
                    if (schoolRoll <= 0)
                        return school;
                }
            }
        }
    }

    int total = 0;
    for (i = 0; i < kNumSecSkills; i++) {
        if (currentHero->m_skillLevel[i] < maxLevel &&
            currentHero->m_skillLevel[i] >= minLevel &&
            i != excluded) {
            int chance;
            if (!skillDisabled[i])
                chance = classTraits.m_gainSecondarySkillChance[i];
            else
                chance = 0;
            if (chance == 0 && currentHero->m_skillLevel[i] > 0)
                chance = 1;
            total += chance;
        }
    }
    if (total == 0)
        return eSecSkillNone;

    int roll = random(1, total);
    for (i = 0; i < kNumSecSkills; i++) {
        if (currentHero->m_skillLevel[i] < maxLevel &&
            currentHero->m_skillLevel[i] >= minLevel &&
            i != excluded) {
            int chance;
            if (!skillDisabled[i])
                chance = classTraits.m_gainSecondarySkillChance[i];
            else
                chance = 0;
            if (chance == 0 && currentHero->m_skillLevel[i] > 0)
                chance = 1;
            roll -= chance;
            if (roll <= 0) {
                return TSecondarySkill(i);
            }
        }
    }
    return eSecSkillNone;
}

// E:\gamedcs\hero.cpp:2340. Ordinary update_artifact_slot(long, TArtifact)
// helper: retail expands its four updateSlot calls. Absence of a retained
// retail body is not evidence for an inline keyword. The message constructor
// owns zero initialization; lines 2349/2358 set WIDGET_DRAWN inside each arm.
// Keep both source stores and let VC6 decide which expansions share them.
DC_ONLY(0xcd68c, 0x5C)
void updateArtifactSlot(long id, TArtifact artifact)
{
    message msg;
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeY = id;
    if (artifact == ARTIFACT_NONE) {
        msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
        msg.m_extra = widget::WIDGET_DRAWN;
    } else {
        msg.m_codeX = widget::WIDGET_SET_ICON_FRAME;
        msg.m_extra = artifact;
        g_heroScreenWindow->broadcastMessage(msg);
        msg.m_codeX = widget::WIDGET_SET_STATUS;
        msg.m_extra = widget::WIDGET_DRAWN;
    }
    g_heroScreenWindow->broadcastMessage(msg);
}

VA(0x004daf90, 0x23A)  // dc 0xcd6e8
void THeroScreenWindow::updateSlot(TArtifactSlot slot)
{
    TArtifact artifact = TArtifact(g_currentHero->getArtifact(slot).m_artifactId);
    if (artifact == ARTIFACT_NONE) {
        int type = g_artifactSlotTraits[slot].m_type;
        unsigned int remaining = g_currentHero->m_artifactSlotCounts[type];
        if (remaining > 0) {
            int i = ARTIFACT_SLOT_COUNT;
            while (true) {
                --i;
                if (!g_artifactSlotMasks[type].test(i))
                    continue;
                if (i == slot) {
                    artifact = TArtifact(0x91);
                    break;
                }
                if (g_currentHero->getArtifact(TArtifactSlot(i)).m_artifactId == ARTIFACT_NONE
                    && --remaining == 0)
                    break;
            }
        }
    }

    if (g_heroScreenDraggedArtifact.m_artifactId != ARTIFACT_NONE
        && g_currentHero->heroFn004E2840(
               g_heroScreenDraggedArtifact.m_artifactId, slot)) {
        updateArtifactSlot(slot + 0x15, artifact);
        updateArtifactSlot(slot + 2, TArtifact(0x90));
    } else {
        updateArtifactSlot(slot + 0x15, ARTIFACT_NONE);
        updateArtifactSlot(slot + 2, artifact);
    }
}

// E:\gamedcs\hero.cpp:2383
// NOT PINNED, and the attempt is recorded so it is not repeated: retail
// CALLS this three times from WindowHandler's artifact and backpack arms
// while our CL expands it (23 bytes of loop is under any budget), but
// `#pragma auto_inline(off)` here is too blunt - it also stops the
// expansion in SetupHeroView, which retail DOES inline, and the ratchet
// caught the cost (WindowHandler 71.10 -> 72.89 but SetupHeroView 99.53
// -> 98.69). A per-call-site lever would be needed; VC6 has none.
VA(0x004db1d0, 0x17)  // dc 0xcd740
void THeroScreenWindow::updateAllSlots()
{
    for (long slot = ARTIFACT_SLOT_FIRST;
         slot < ARTIFACT_SLOT_COUNT; slot++)
        updateSlot(TArtifactSlot(slot));
}

// E:\gamedcs\hero.cpp:2393
// No retail row: the one call site below is /Ob2-inlined into
// UpdateBackpack, and `inline` reproduces the absence (the
// strip::DrawNumber precedent).
DC_ONLY(0xcd76c, 0x22)
inline void updateBackpackItem(int i)
{
    updateArtifactSlot(i + 0x28,
                       TArtifact(g_currentHero->getBackpack(i).m_artifactId));
}

VA(0x004db1f0, 0x160)  // dc 0xcd790
void updateBackpack()
{
    message arrows;
    arrows.m_codeX = 0;
    arrows.m_codeY = 0;
    arrows.m_qualifier = 0;
    arrows.m_mouseX = 0;
    arrows.m_mouseY = 0;
    arrows.m_extra = 0;
    arrows.m_window = 0;
    arrows.m_id = MESSAGE_WIDGET;

    for (int i = 0; i < 5; i++)
        updateBackpackItem(i);

    arrows.m_codeX = g_currentHero->getLastBackpackIndex() + 1 > 5
                       ? widget::WIDGET_CLEAR_STATUS
                       : widget::WIDGET_SET_STATUS;
    arrows.m_extra = widget::WIDGET_DIMMED_NODRAW;
    arrows.m_codeY = 0x4d;
    g_heroScreenWindow->broadcastMessage(arrows);
    arrows.m_codeY = 0x4e;
    g_heroScreenWindow->broadcastMessage(arrows);

    arrows.m_codeX = g_currentHero->getLastBackpackIndex() + 1 > 5
                       ? widget::WIDGET_SET_STATUS
                       : widget::WIDGET_CLEAR_STATUS;
    arrows.m_extra = widget::WIDGET_ACTIVE;
    arrows.m_codeY = 0x4d;
    g_heroScreenWindow->broadcastMessage(arrows);
    arrows.m_codeY = 0x4e;
    g_heroScreenWindow->broadcastMessage(arrows);
}

VA(0x004db350, 0x86)  // dc 0xcd86c
void type_artifact::getRolloverText(char* buffer) const
{
    if (m_artifactId == ARTIFACT_NONE)
        strcpy(buffer, g_emptyArtifactRolloverText);
    else if (m_artifactId == ARTIFACT_SPELLBOOK)
        strcpy(buffer, g_spellbookRolloverText);
    else
        sprintf(buffer, g_artifactRolloverFormat,
                g_artifactTraits[m_artifactId].m_name);
}

// E:\gamedcs\hero.cpp:2450
// Only free row between get_rollover_text (0x004db350, claimed) and
// UpdateHeroScreenStatusBar; carries a /GX frame and a hidden
// return-UDT pointer, which is what a std::string-by-value member needs.
// Everything but a SPELL SCROLL returns the traits description straight
// into the return slot. A scroll's description carries a `[...]`
// placeholder, and the body walks it by hand: copy up to the '[', splice
// in akSpellTraits[spell].name, skip to the ']' and append whatever
// follows. The 136-byte spell stride and the +0x10 name are the ones
// armygrp.h already models, indexed by the record's second dword.

VA(0x004db3e0, 0x277)  // anchor-bracket, dc 0xcd8b8
std::string type_artifact::getDescription() const
{
    if (m_artifactId != ARTIFACT_SPELL_SCROLL)
        return g_artifactTraits[m_artifactId].m_description;

    std::string result;
    const char* cursor = g_artifactTraits[m_artifactId].m_description;
    char c = *cursor;
    while (c != 0 && c != '[') {
        cursor++;
        result += c;
        c = *cursor;
    }
    if (*cursor == '[') {
        result += g_spellTraits[m_extra].m_name;
        while (*cursor != 0 && *cursor != ']')
            cursor++;
        if (*cursor == ']') {
            cursor++;
            result += cursor;
        }
    }
    return result;
}

// HeroScrn.txt's runtime-loaded rows. The loader at 0x5b97c0 fetches the
// resource and copies Text[0..32] into one contiguous .bss run starting
// at 0x6a8014, which fixes both the extent and each cell's row index -
// gEmptyArtifactRolloverText (row 11), gSpellbookRolloverText (row 14)
// and gArtifactRolloverFormat (row 15) declared near the top of this file
// are part of the same run. Only the rows this compiland reads are
// declared. Names are role-derived from the retail consumer and
// PROVISIONAL; rows no consumer pins keep an ORDINAL PLACEHOLDER
// spelling keyed to the row index. Declared HERE rather than beside the
// other three so the ~23 new symbols do not shift C1 handle numbers for
// every already-matched body above.
DATA(0x006a8014) extern const char* g_heroScreenText0;                 // row 0
DATA(0x006a8018) extern const char* g_heroScreenNameFormat;            // row 1
DATA(0x006a8020) extern const char* g_heroScreenMoraleHighText;        // row 3
DATA(0x006a8024) extern const char* g_heroScreenMoraleNeutralText;     // row 4
DATA(0x006a8028) extern const char* g_heroScreenMoraleLowText;         // row 5
DATA(0x006a802c) extern const char* g_heroScreenLuckHighText;          // row 6
DATA(0x006a8030) extern const char* g_heroScreenLuckNeutralText;       // row 7
DATA(0x006a8034) extern const char* g_heroScreenLuckLowText;           // row 8
DATA(0x006a8038) extern const char* g_heroScreenText9;                 // row 9
DATA(0x006a803c) extern const char* g_heroScreenArmySlotFormat;        // row 10
DATA(0x006a8044) extern const char* g_heroScreenArmySlotIdleFormat;    // row 12
DATA(0x006a8048) extern const char* g_heroScreenArmySwapFormat;        // row 13
DATA(0x006a8054) extern const char* g_heroScreenHeroNameFormat;        // row 16
DATA(0x006a8058) extern const char* g_heroScreenText17;                // row 17
DATA(0x006a8060) extern const char* g_heroScreenArmyMergeFormat;       // row 19
DATA(0x006a8064) extern const char* g_heroScreenArmyMoveFormat;        // row 20
DATA(0x006a8068) extern const char* g_heroScreenSecondarySkillFormat;  // row 21
DATA(0x006a806c) extern const char* g_heroScreenText22;                // row 22
DATA(0x006a8070) extern const char* g_heroScreenText23;                // row 23
DATA(0x006a8074) extern const char* g_heroScreenText24;                // row 24
DATA(0x006a8078) extern const char* g_heroScreenFormationGroupedText;  // row 25
DATA(0x006a807c) extern const char* g_heroScreenFormationSpreadText;   // row 26
// Rows 28..32 continue the same stride-4 run. WindowHandler's six
// right-click help arms are what pin them: each hands one of these
// straight to NormalDialog with dialog type 4, so the role is "help text
// for widget N" and the names follow the widget they answer for.
DATA(0x006a8080) extern const char* g_heroScreenText27;                // row 27
DATA(0x006a8084) extern const char* g_heroScreenHeroNameHelp;          // row 28
DATA(0x006a8088) extern const char* g_heroScreenWidget7aHelp;          // row 29
DATA(0x006a808c) extern const char* g_heroScreenWidget7cHelp;          // row 30
DATA(0x006a8090) extern const char* g_heroScreenFormationHelp;         // row 31
// The quest-log button's help text, and the ONE oddity in the set: it is
// NOT in the 0x6a8014 run and has exactly one reference image-wide, this
// call site. Provenance genuinely undetermined - 0x6a5704 is 0x6a56e0 +
// 0x24, which would make it a cell of townmgr's stride-8 table, but
// nothing in the bytes decides that. ORDINAL PLACEHOLDER spelling.
DATA(0x006a8094) extern const char* g_heroScreenMixedArmyHelp;         // row 32
DATA(0x006a5704) extern const char* g_unnamed6a5704;

VA(0x004db660, 0x728)  // dc 0xcd9c4
void THeroScreenWindow::updateHeroScreenStatusBar(message* msg)
{
    if (g_heroScreenDraggedArtifact.m_artifactId != ARTIFACT_NONE)
        return;

    switch (msg->m_codeY) {
    case PRIMARY_SKILL_0_ID:
    case PRIMARY_SKILL_1_ID:
    case PRIMARY_SKILL_2_ID:
    case PRIMARY_SKILL_3_ID:
        sprintf(g_text, g_heroScreenNameFormat,
                g_primarySkillNames[msg->m_codeY - PRIMARY_SKILL_0_ID]);
        break;

    case PORTRAIT_ID:
        sprintf(g_text,
                g_generalText->getText(GENERAL_TEXT_HERO_ROLLOVER_FORMAT),
                g_currentHero->m_name, g_currentHero->heroFn004D8F70());
        break;

    case MORALE_ID:
        if (g_currentHero->getMorale(0, 0, 1) > 0)
            strcpy(g_text, g_heroScreenMoraleHighText);
        else if (g_currentHero->getMorale(0, 0, 1) == 0)
            strcpy(g_text, g_heroScreenMoraleNeutralText);
        else
            strcpy(g_text, g_heroScreenMoraleLowText);
        break;

    case LUCK_ID:
        if (g_currentHero->getLuck(0, 0, 1) > 0)
            strcpy(g_text, g_heroScreenLuckHighText);
        else if (g_currentHero->getLuck(0, 0, 1) == 0)
            strcpy(g_text, g_heroScreenLuckNeutralText);
        else
            strcpy(g_text, g_heroScreenLuckLowText);
        break;

    case ARMY_SLOT_0_ID:
    case ARMY_SLOT_1_ID:
    case ARMY_SLOT_2_ID:
    case ARMY_SLOT_3_ID:
    case ARMY_SLOT_4_ID:
    case ARMY_SLOT_5_ID:
    case ARMY_SLOT_6_ID: {
        int slot = msg->m_codeY - ARMY_SLOT_0_ID;
        if (g_heroScreenArmySlot == HERO_SCREEN_NO_ARMY_SLOT) {
            if (g_currentHero->m_army.m_armies[slot] != CREATURE_NONE)
                sprintf(g_text, g_heroScreenArmySlotFormat,
                        getArmyName(g_currentHero->m_army.m_armies[slot], 1));
            else
                strcpy(g_text, g_emptyArtifactRolloverText);
        } else if (g_heroScreenArmySlot == slot) {
            sprintf(g_text, g_heroScreenArmySlotFormat,
                    getArmyName(g_currentHero->m_army.m_armies[slot], 1));
        } else if (g_unnamed6aa9d8 != 0) {
            if (g_currentHero->m_army.m_armies[slot] != CREATURE_NONE)
                sprintf(g_text, g_heroScreenArmySlotFormat,
                        getArmyName(g_currentHero->m_army.m_armies[slot], 1));
            else
                strcpy(g_text, g_emptyArtifactRolloverText);
        } else if (g_currentHero->m_army.m_armies[slot] == CREATURE_NONE) {
            if (g_heroScreenArmyStripLive)
                sprintf(g_text, g_heroScreenArmyMoveFormat,
                        getArmyName(g_currentHero->m_army.m_armies[
                                        g_heroScreenArmySlot],
                                    2));
            else
                sprintf(g_text, g_heroScreenArmySlotIdleFormat,
                        getArmyName(g_currentHero->m_army.m_armies[
                                        g_heroScreenArmySlot],
                                    2));
        } else if (g_currentHero->m_army.m_armies[slot]
                   == g_currentHero->m_army.m_armies[g_heroScreenArmySlot]) {
            sprintf(g_text, g_heroScreenArmyMergeFormat,
                    getArmyName(g_currentHero->m_army.m_armies[slot], 2));
        } else {
            sprintf(g_text, g_heroScreenArmySwapFormat,
                    getArmyName(g_currentHero->m_army.m_armies[
                                    g_heroScreenArmySlot],
                                1),
                    getArmyName(g_currentHero->m_army.m_armies[slot], 1));
        }
        break;
    }

    case ARTIFACT_SLOT_0_ID:  case ARTIFACT_SLOT_1_ID:
    case ARTIFACT_SLOT_2_ID:  case ARTIFACT_SLOT_3_ID:
    case ARTIFACT_SLOT_4_ID:  case ARTIFACT_SLOT_5_ID:
    case ARTIFACT_SLOT_6_ID:  case ARTIFACT_SLOT_7_ID:
    case ARTIFACT_SLOT_8_ID:  case ARTIFACT_SLOT_9_ID:
    case ARTIFACT_SLOT_10_ID: case ARTIFACT_SLOT_11_ID:
    case ARTIFACT_SLOT_12_ID: case ARTIFACT_SLOT_13_ID:
    case ARTIFACT_SLOT_14_ID: case ARTIFACT_SLOT_15_ID:
    case ARTIFACT_SLOT_16_ID: case ARTIFACT_SLOT_17_ID:
    case ARTIFACT_SLOT_18_ID:
        g_currentHero->getArtifact(TArtifactSlot(msg->m_codeY - ARTIFACT_SLOT_0_ID))
            .getRolloverText(g_text);
        break;

    case BACKPACK_SLOT_0_ID: case BACKPACK_SLOT_1_ID:
    case BACKPACK_SLOT_2_ID: case BACKPACK_SLOT_3_ID:
    case BACKPACK_SLOT_4_ID:
        g_currentHero->getBackpack(msg->m_codeY - BACKPACK_SLOT_0_ID)
            .getRolloverText(g_text);
        break;

    case WIDGET_6B_ID:
    case WIDGET_76_ID:
    case WIDGET_8B_ID:
        strcpy(g_text, g_heroScreenText27);
        break;

    case WIDGET_6C_ID:
    case WIDGET_70_ID:
    case WIDGET_77_ID:
        strcpy(g_text, g_heroScreenText9);
        break;

    case WIDGET_6D_ID:
    case WIDGET_71_ID:
    case WIDGET_78_ID:
        strcpy(g_text, g_heroScreenText22);
        break;

    case WIDGET_7A_ID:
        strcpy(g_text, g_heroScreenText23);
        break;

    case WIDGET_7C_ID:
        strcpy(g_text, g_heroScreenText24);
        break;

    case FORMATION_ID:
        if (g_currentHero->m_formation & HERO_FORMATION_GROUPED)
            strcpy(g_text, g_heroScreenFormationGroupedText);
        else
            strcpy(g_text, g_heroScreenFormationSpreadText);
        break;

    case MIXED_ARMY_ID:
        sprintf(g_text, g_heroScreenArmyMoveFormat,
                g_generalText->getText(GENERAL_TEXT_MIXED_ARMY));
        break;

    case WIDGET_80_ID:
        strcpy(g_text, g_heroScreenText0);
        break;

    case HERO_NAME_ID:
        sprintf(g_text, g_heroScreenHeroNameFormat,
                g_currentHero->m_name, g_currentHero->heroFn004D8F70());
        break;

    case WIDGET_7800_ID:
        strcpy(g_text, g_heroScreenText17);
        break;

    default: {
        int nth;
        if (msg->m_codeY >= SKILL_ICON_FIRST_ID
            && msg->m_codeY <= SKILL_ICON_LAST_ID)
            nth = msg->m_codeY - SKILL_ICON_FIRST_ID;
        else if (msg->m_codeY >= SKILL_NAME_FIRST_ID
                 && msg->m_codeY <= SKILL_NAME_LAST_ID)
            nth = msg->m_codeY - SKILL_NAME_FIRST_ID;
        else if (msg->m_codeY >= SKILL_LEVEL_FIRST_ID
                 && msg->m_codeY <= SKILL_LEVEL_LAST_ID)
            nth = msg->m_codeY - SKILL_LEVEL_FIRST_ID;
        else {
            g_text[0] = 0;
            break;
        }
        if (nth < g_currentHero->m_skillCount) {
            int skill = g_currentHero->getNthSS(nth);
            sprintf(g_text, g_heroScreenSecondarySkillFormat,
                    g_skillMasteryNames[g_currentHero->m_skillLevel[skill] - 1],
                    g_sSkillTraits[skill].m_name);
        } else {
            g_text[0] = 0;
        }
        break;
    }
    }

    message update;
    update.m_qualifier = 0;
    update.m_mouseX = 0;
    update.m_mouseY = 0;
    update.m_window = 0;
    update.m_id = MESSAGE_WIDGET;
    update.m_codeX = widget::WIDGET_SET_TEXT;
    update.m_codeY = STATUS_BAR_ID;
    update.m_extraText = g_text;
    broadcastMessage(update);
    drawWindow(1, STATUS_BAR_BORDER_ID, STATUS_BAR_ID);
}

#if 0  // @carcass

// E:\gamedcs\hero.cpp:2726
DC_ONLY(0xcdf30, 0x20E)
void handleArtifactClick(long code, unsigned char right_mouse)
{
    // @stub
}

// E:\gamedcs\hero.cpp:2796
// Whole body proven: `mov eax,0x3f / add ecx,0x3cc` starts at
// backpack[63] (0x1d4 + 63*8 == 0x3cc) and walks back by 8 until a slot
// id is not -1, then `or eax,-1` for the not-found return. Both the
// stride and the base are the modelled backpack array.
#endif  // @carcass

VA(0x004dbd90, 0x1E)  // dc 0xce140
long hero::getLastBackpackIndex() const
{
    for (long slot = 64; slot--; ) {
        if (m_backpack[slot].m_artifactId != -1)
            return slot;
    }
    return -1;
}

VA(0x004dbdb0, 0x57)  // dc 0xce168
void hero::rotateBackpackLeft()
{
    long last = getLastBackpackIndex();
    if (last < 0)
        return;
    type_artifact saved = m_backpack[last];
    for (long slot = last; slot > 0; slot--)
        m_backpack[slot] = m_backpack[slot - 1];
    m_backpack[0] = saved;
}

VA(0x004dbe10, 0x68)  // dc 0xce1cc
void hero::rotateBackpackRight()
{
    long last = getLastBackpackIndex();
    if (last <= 0)
        return;
    type_artifact saved = m_backpack[0];
    for (long slot = 0; slot < last; slot++)
        m_backpack[slot] = m_backpack[slot + 1];
    m_backpack[last] = saved;
}

VA(0x004dbe80, 0xA4)
unsigned char hero::heroFn004DBE80(int combination)
{
    std::bitset<144> missingComponents =
        g_combinationArtifacts[combination].m_components;
    for (int slot = 0; slot < 19; slot++) {
        int artifactId = m_equipped[slot].m_artifactId;
        if (artifactId != ARTIFACT_NONE)
            missingComponents.reset(artifactId);
    }
    return !missingComponents.any();
}

VA(0x004dbf30, 0x133)
unsigned char hero::heroFn004DBF30(int combination, long slot)
{
    std::bitset<144> components =
        g_combinationArtifacts[combination].m_components;
    if (slot != -1) {
        components.reset(m_equipped[slot].m_artifactId);
        removeArtifact(slot);
    }

    int i = 19;
    while (components.any()) {
        i--;
        int artifactId = m_equipped[i].m_artifactId;
        if (artifactId == ARTIFACT_NONE)
            continue;
        if (!components.test(artifactId))
            continue;
        components.reset(m_equipped[i].m_artifactId);
        removeArtifact(i);
    }

    return equipArtifact(
        // The Complete combination table stores the added artifact ordinal; equipArtifact receives the canonical DC-typed record.
        &type_artifact(static_cast<TArtifact>(g_combinationArtifacts[combination].m_artifactId) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */),
        -1);
}

VA(0x004dc070, 0x87)
void hero::heroFn004DC070(long slot)
{
    int combination =
        g_artifactTraits[m_equipped[slot].m_artifactId].m_comboType;
    removeArtifact(slot);

    const std::bitset<144>& components =
        g_combinationArtifacts[combination].m_components;
    for (int artifactId = 0; artifactId < 144; artifactId++) {
        if (components.test(artifactId)) {
            // Complete enumerates all 144 component bits, beyond DC's 128 artifact ids; each set bit becomes a typed artifact record.
            type_artifact artifact(static_cast<TArtifact>(artifactId) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */);
            equipArtifact(&artifact, -1);
        }
    }
}

// The NOTIFIER half of the family. Two pieces are byte-proven beyond
// the arithmetic: the player record's +0xe8 dword is walked as a
// std::bitset<12> (the `cmp x,0xc` bounds check reaches bitset<12>'s own
// _Xran, and the set is the `or` arm of `set(_P, true)` with the else
// folded away), and the component sweep calls the TWO-ARGUMENT
// `set(id, false)` OUT OF LINE where the sibling above inlines its
// one-argument `reset` - a per-caller /Ob2 budget difference, not a
// spelling one. `any()` is likewise a call here and inline there.
// The owner index is taken WITHOUT the `owner < 0` guard get_player
// carries; retail indexes gpGame->players directly.

// Keep bitset<144>::set(size_t, bool) and bitset<144>::any() out of line at
// their two call sites.

// 65.9718 -> 87.2712 (2026-08-20), AND THE NOTE BELOW NAMED THE WALL AND
// THEN DECLARED IT UNSPELLABLE. It was right that retail calls the
// bitset<12> `_Xran` helper at all three sites (fn+0x61, +0xb7, +0x13a,
// each `cmp <idx>,0xc / jb / call 0x4d4eb0`) with set/test themselves
// INLINE, and right that our CL expanded the throw body at two of the
// three. Its conclusion - "inline_depth cannot express 'inline the
// parent, not the child'" - is true of the PRAGMA and false of the match.

// The lever is DEPTH, spelled in the source. VC6's <bitset> defines
//     bool operator[](size_t _P) const   { return (test(_P)); }
//     reference operator[](size_t _P)    { return reference(*this,_P); }
//     reference& reference::operator=(bool _X) { _Pbs->set(_Off,_X); }
// so `b[i]` and `b[i] = true` reach test/set one level DEEPER than
// `b.test(i)` and `b.set(i)` do, which puts `_Xran` at depth 3 instead of
// 2 - past what /Ob2 expands here. All three sites are written that way
// and all three throws collapse into retail's calls.

// Residual (87.3%): the esi/edi/ebx role permutation plus the prologue's
// unwind-table addend, which is a reloc addend and not a state count. The
// DC roster has NO row for this function at all, so its call census
// cannot be consulted.
VA(0x004dc100, 0x217)  // retail-only, hero member, ret 4
void hero::heroFn004DC100(long slot)
{
    playerData& player = g_game->m_players[m_owner];
    const TArtifactTraits& traits =
        g_artifactTraits[m_equipped[slot].m_artifactId];

    if (traits.m_comboType != -1) {
        player.m_assembledCombinations[traits.m_comboType] = true;
        return;
    }

    int targetCombo = traits.m_targetCombo;
    if (targetCombo == -1)
        return;
    if (player.m_assembledCombinations[targetCombo])
        return;

    std::bitset<144> missing =
        g_combinationArtifacts[targetCombo].m_components;
    for (int i = 0; i < 19; i++) {
        int artifactId = m_equipped[i].m_artifactId;
        if (artifactId != ARTIFACT_NONE)
#pragma inline_depth(0)
            missing.set(artifactId, false);
#pragma inline_depth()
    }
#pragma inline_depth(0)
    if (missing.any())
#pragma inline_depth()
        return;

    // THE ONE SITE OF THE THREE THAT STILL EXPANDS (residual 87.27%), and
    // it is the budget being handed downstream, not a wrong spelling.
    // Retail emits `cmp edi,0xc / jb / call bitset<12>::_Xran` at fn+0x12e;
    // we still build the `invalid bitset<N> position` string at [ebp-0x40]
    // and the out_of_range at [ebp-0x70] here, which is the whole 44-byte
    // frame surplus (`sub esp,0x64` against retail's 0x38). Fixing the two
    // EARLIER sites is what freed the budget this one then spends - the
    // A9 sequential-charge behaviour, observed from the receiving end.
    // <bitset> offers no rung deeper than `operator[] -> operator= ->
    // set -> _Xran` to push it back out: `reset(_P)` and `flip(_P)` sit at
    // the same depth, and `at(_P)` emits TWO bounds checks where retail
    // has one.
    player.m_assembledCombinations[targetCombo] = true;

    int assembled = g_combinationArtifacts[targetCombo].m_artifactId;
    std::string prompt = formatString(g_generalText->getText(733),
                                       g_artifactTraits[assembled].m_name);
    normalDialog(prompt.c_str(), 2, -1, -1, 8, assembled, -1, 0, -1, 0,
                 -1, 0);
    if (g_windowManager->m_dialogReturn == DIALOG_RETURN_ACCEPT)
        heroFn004DBF30(targetCombo, slot);
}

// E:\gamedcs\hero.cpp:2849
// PROVEN BY CALLER: armygrp's armyGroup::get_morale_description
// (0x44b960) calls exactly this address, and window.obj's hero-screen
// status path (0x4f32a0) is the second site. /GX frame + hidden
// return-UDT pointer, as the by-value std::string return needs.

// The description twin of hero::GetMorale (0x4e39b0): the same modifier
// ladder, but each rung appends its ARRAYTXT line and accumulates what
// it described, and the tail prints GetMorale's own answer MINUS that
// sum as the "other modifiers" line. Every IsWieldingArtifact is
// /Ob2-expanded exactly as in GetMorale. The Grail arm expands
// town::HasBuilding INLINE here - `active & bitNumber[HOLY_GRAIL_ID]` -
// where GetLuck's own arm is a real call, and it credits TOWN_CASTLE.
// The opening flag arm ASSIGNS (Dinkumware `assign(const char*,
// size_type)`), it does not append.

VA(0x004dc320, 0x793)  // anchor-caller (armyGroup::get_morale_description), dc 0xce260
std::string hero::getMoraleDescription() const
{
    int morale = 0;
    std::string result;

    if (m_flags & 0x800000) {
        result = g_generalText->getText(438);
        morale = 500;
    }

    if (this->isWieldingArtifact(0x6c)) {
        result += g_moraleTexts[26];
        morale += 3;
    }
    if (this->isWieldingArtifact(0x2d)) {
        result += g_moraleTexts[4];
        morale++;
    }
    if (this->isWieldingArtifact(0x31)) {
        result += g_moraleTexts[5];
        morale++;
    }
    if (this->isWieldingArtifact(0x32)) {
        result += g_moraleTexts[6];
        morale++;
    }
    if (this->isWieldingArtifact(0x33)) {
        result += g_moraleTexts[7];
        morale++;
    }

    if (m_flags & 0x2000000) {
        result += g_moraleTexts[8];
        morale++;
    }
    if (m_flags & 0x4) {
        result += g_moraleTexts[9];
        morale++;
    }
    if (m_flags & 0x80) {
        result += g_moraleTexts[10];
        morale++;
    }
    if (m_flags & 0x100) {
        result += g_moraleTexts[11];
        morale++;
    }
    if (m_flags & 0x4000000) {
        result += g_moraleTexts[12];
        morale += 2;
    }
    if (m_flags & 0x400) {
        result += g_moraleTexts[13];
        morale--;
    }
    if (m_flags & 0x200) {
        result += g_moraleTexts[14];
        morale--;
    }
    if (m_flags & 0x40) {
        result += g_moraleTexts[15];
        morale++;
    }
    if (m_flags & 0x800) {
        result += g_moraleTexts[16];
        morale--;
    }
    if (m_flags & 0x10000) {
        result += g_moraleTexts[17];
        morale++;
    }
    if (m_flags & 0x4000) {
        result += g_moraleTexts[18];
        morale++;
    }
    if (m_flags & 0x200000) {
        // Residual (91.65%) - MEASURED, BLOCKED BY THE PIN FLOOR
        // (polish 26).  Retail CALLS append(const char*,size_type) at THIS
        // rung too (0x4dc71b `repne scasb` + call, a 0x20-byte arm against
        // our 0x73-byte expansion); the fn-level census is retail 18
        // PBDI-append calls + 3 _Grow/_Xlen/_Eos expansions against our 17
        // + 4, and this is the surplus expansion.  Spelling it
        // `const char* t = gMoraleTexts[19]; result.append(t, strlen(t));`
        // under a statement `inline_depth(0)` moves the first divergence
        // from +0x408 to +0x551 and aligns two of the three _Grow sites
        // exactly - but the freed budget then over-inlines the Grail and
        // negative-modifier `+=` sites below, so the pin only pays PAIRED
        // with pins on those two spelled as
        // `result.append(format_string(...), 0, std::string::npos)`:
        // 91.65 -> 89.97 (this rung alone) -> 93.21 (+ both `+=` pins)
        // -> 93.91 (+ the two tail sites respelled as append).  All three
        // pins are needed together; the same three call-site spellings
        // WITHOUT the pins score 76.26.  Not shippable while the
        // cleanliness ratchet holds inline-depth pins at 355 falling-only.
        // Also rejected: a named `std::string grailText` local for the
        // Grail temp gives a PERFECT skeleton (109/110 blocks, 0 missing,
        // 88 exact) and only 92.16 - the extra frame slot costs the tail's
        // register allocation.
        result += g_moraleTexts[19];
        morale -= 3;
    }

    if (m_skillLevel[eSecSkillLeadership] == eMasteryBasic) {
        // Retail CALLS append(const char*, size_type) at THIS rung only -
        // its `repne scasb` strlen + `call` sit at fn+0x454 where our CL
        // expanded the append (the extra _Xlen/_Grow/_Eos exposure) - and
        // expands the Advanced and Expert rungs exactly as we do. The
        // TownQuickView lever: spell the site at the depth retail stops
        // at, then the statement pin imposes exactly retail's call.
        const char* basicText = g_moraleTexts[20];
        size_t basicTextLen = strlen(basicText);
#pragma inline_depth(0)
        result.append(basicText, basicTextLen);
#pragma inline_depth()
        morale++;
    }
    if (m_skillLevel[eSecSkillLeadership] == eMasteryAdvanced) {
        result += g_moraleTexts[21];
        morale += 2;
    }
    if (m_skillLevel[eSecSkillLeadership] == eMasteryExpert) {
        result += g_moraleTexts[22];
        morale += 3;
    }

    if (m_owner >= 0) {
        playerData& player = g_game->m_players[m_owner];
        for (int i = 0; i < player.m_numTowns; i++) {
            town* ownedTown = g_game->getTown(player.m_townIds[i]);
            if ((ownedTown->m_active & g_bitNumber[HOLY_GRAIL_ID]) != 0
                && ownedTown->m_type == TOWN_CASTLE) {
                result += formatString(
                    "\n%s +2",
                    getBuildingName(TOWN_CASTLE, HOLY_GRAIL_ID));
                morale += 2;
                break;
            }
        }
    }

    int otherModifier =
        this->getMorale(0, 0, 0) - morale;
    if (otherModifier < 0)
        result += formatString(g_moraleTexts[24], abs(otherModifier));
    else if (otherModifier > 0)
        result += formatString(g_moraleTexts[25], abs(otherModifier));

    return result;
}

// E:\gamedcs\hero.cpp:3021
// PROVEN BY CALLER: armygrp's armyGroup::get_luck_description
// (0x44c1c0) calls exactly this address; 0x4f3540 is the second site.
// Same /GX + return-UDT shape as its morale twin above.

// hero::GetLuck's describer, the exact shape of the morale twin. FOUR of
// the flag rungs carry a SIGN as a format argument instead of their own
// line: they share ONE carrier format, gLuckTexts[10], and pass the
// literals "-1"/"+1"/"+2"/"+3" out of hero.obj's own pool. The Grail arm
// expands town::HasBuilding INLINE against TOWN_RAMPART, unlike
// hero::GetLuck's own arm, which calls it.

// 85.74 -> 93.71 (2026-08-20): retail CALLS append(const char*,
// size_type) at the [14] mist rung (fn+0x485's run of four scasb+call
// rungs ends there) and expands [15],[16],[17]; the imposed-call lever
// on [14] alone is the whole gain.

// Residual (93.71%): retail ALSO calls append at the Basic-luck rung
// [15] (guarded by the eMasteryBasic cmp at fn+0x465) - the census is
// _Xlen x4 ours vs x3 retail's. MEASURED NEGATIVE both ways, do not
// retry as spelled: pinning [15] flips the four gLuckTexts[10] carrier
// rungs' format_string-temp dtors from inline to `call _Tidy` (x5) and
// costs 93.71 -> 85.95; the unpinned append(p,len) spelling costs
// 93.71 -> 78.66. Imposition reachable, imposition net-negative - the
// same coupling the morale twin's tail shows.
VA(0x004dcac0, 0x7E0)  // anchor-caller (armyGroup::get_luck_description), dc 0xce648
std::string hero::getLuckDescription() const
{
    int luck = 0;
    std::string result;

    if (m_flags & 0x400000) {
        result = g_generalText->getText(438);
        luck = 500;
    }

    if (this->isWieldingArtifact(0x6c)) {
        result += g_luckTexts[21];
        luck += 3;
    }
    if (this->isWieldingArtifact(0x2d)) {
        result += g_luckTexts[4];
        luck++;
    }
    if (this->isWieldingArtifact(0x2e)) {
        result += g_luckTexts[5];
        luck++;
    }
    if (this->isWieldingArtifact(0x2f)) {
        result += g_luckTexts[6];
        luck++;
    }
    if (this->isWieldingArtifact(0x30)) {
        result += g_luckTexts[7];
        luck++;
    }

    if (m_flags & 0x8) {
        result += g_luckTexts[8];
        luck += 2;
    }
    if (m_flags & 0x10) {
        result += g_luckTexts[9];
        luck++;
    }
    if (m_flags & 0x20) {
        result += formatString(g_luckTexts[10], "-1");
        luck--;
    }
    if (m_flags & 0x8000000) {
        result += formatString(g_luckTexts[10], "+1");
        luck++;
    }
    if (m_flags & 0x10000000) {
        result += formatString(g_luckTexts[10], "+2");
        luck += 2;
    }
    if (m_flags & 0x20000000) {
        result += formatString(g_luckTexts[10], "+3");
        luck += 3;
    }
    if (m_flags & 0x1000) {
        result += g_luckTexts[11];
        luck -= 2;
    }
    if (m_flags & 0x2000) {
        result += g_luckTexts[12];
        luck++;
    }
    if (m_flags & 0x8000) {
        result += g_luckTexts[13];
        luck++;
    }
    if (m_flags & 0x10000) {
        // Retail CALLS append(const char*, size_type) at THIS rung -
        // four `repne scasb`+call rungs end at fn+0x486 and [14] is the
        // last of them - then expands [15],[16],[17] as we do. Same
        // TownQuickView lever as the morale twin's [20] rung.
        const char* mistText = g_luckTexts[14];
        size_t mistTextLen = strlen(mistText);
#pragma inline_depth(0)
        result.append(mistText, mistTextLen);
#pragma inline_depth()
        luck++;
    }

    if (m_skillLevel[eSecSkillLuck] == eMasteryBasic) {
        // Residual (93.71%) - MEASURED, BLOCKED BY THE PIN FLOOR
        // (polish 26).  Identical to the morale twin: retail's PBDI-append
        // census is 12 calls + 3 expansions against our 11 + 4, and the
        // surplus expansion sits at THIS rung and at the [13] siren rung
        // above - pinning either alone just moves the flip to the other.
        // Pinning BOTH (`const char* t = gLuckTexts[n];
        // result.append(t, strlen(t));` under `inline_depth(0)`) reaches
        // 92.15, and adding the negative-modifier arm respelled as
        // `result.append(format_string(...), 0, std::string::npos)` under
        // the same pin reaches 95.45 (+1.74 over the banked max).
        // Three new pins; not shippable while the ratchet holds pins at
        // 355 falling-only.
        result += g_luckTexts[15];
        luck++;
    }
    if (m_skillLevel[eSecSkillLuck] == eMasteryAdvanced) {
        result += g_luckTexts[16];
        luck += 2;
    }
    if (m_skillLevel[eSecSkillLuck] == eMasteryExpert) {
        result += g_luckTexts[17];
        luck += 3;
    }

    if (m_owner >= 0) {
        playerData& player = g_game->m_players[m_owner];
        for (int i = 0; i < player.m_numTowns; i++) {
            town* ownedTown = g_game->getTown(player.m_townIds[i]);
            if ((ownedTown->m_active & g_bitNumber[HOLY_GRAIL_ID]) != 0
                && ownedTown->m_type == TOWN_RAMPART) {
                result += formatString(
                    "\n%s +2",
                    getBuildingName(TOWN_RAMPART, HOLY_GRAIL_ID));
                luck += 2;
                break;
            }
        }
    }

    int otherModifier = this->getLuck(0, 0, 0) - luck;
    if (otherModifier < 0)
        result += formatString(g_luckTexts[19], abs(otherModifier));
    else if (otherModifier > 0)
        result += formatString(g_luckTexts[20], abs(otherModifier));

    return result;
}

#if 0  // @carcass

// E:\gamedcs\hero.cpp:3181
DC_ONLY(0xcea3c, 0x1A4)
void handleBackpackClick(long code, unsigned char right_mouse)
{
    // @stub
}

#endif  // @carcass

// E:\gamedcs\hero.cpp:3229
// ANCHOR-VTABLE: 0x004dd2a0 has no rel32 caller at all - it is reached
// only through slot 14 of vtable 0x63eae8, and 0x63eae8 is the vtable
// THeroScreenWindow's destructor (0x004e1550, claimed below) stores. It
// is `ret 4` returning 2 and it fills the pointed-to message with
// {0x200, 10, 10}, matching `int ExitDialog(message*)`.
// Retail emits it AFTER the two description bodies, where the DC source
// has it before ShowWidgets; ShowWidgets itself has no retail row.
VA(0x004dd2a0, 0x2C)  // anchor-vtable (slot 14 of 0x63eae8), dc 0xcebe0
int THeroScreenWindow::exitDialog(message& msg)
{
    g_windowManager->m_dialogReturn = DIALOG_RETURN_SPLIT_ACCEPT;
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeY = 10;
    msg.m_codeX = 10;
    return MESSAGE_DISPATCH_FORWARD;
}

#if 0  // @carcass

// E:\gamedcs\hero.cpp:3239
DC_ONLY(0xcec1c, 0x78E)
void THeroScreenWindow::ShowWidgets()
{
    // @stub
}

// E:\gamedcs\hero.cpp:3421
DC_ONLY(0xcf3ac, 0x1A0)
void THeroScreenWindow::show_skills()
{
    // @stub
}

#endif  // @carcass

static void handleArtifactClick(long code, unsigned char rightMouse)
{
    long slot = code;
    type_artifact record = g_currentHero->getArtifact(TArtifactSlot(slot));

    if (g_heroScreenDraggedArtifact.m_artifactId != ARTIFACT_NONE) {
        if (rightMouse)
            return;
        // Was pinned per SITE, not per function: `predict-inline` reported
        // HeroFn_004E2840 base x0 vs retail x1 inside WindowHandler, i.e.
        // retail CALLS 0x4e2840 here where our CL expanded it (this helper
        // is itself a single-call-site static that /Ob2 folds into
        // WindowHandler, so the budget reaches through). With two call
        // sites in the compiland the expansion does not happen, and this
        // pin and update_slot's sibling are both byte-flat, so both went
        // (2026-09-06, polish lane 50).
        if (!g_currentHero->heroFn004E2840(
                g_heroScreenDraggedArtifact.m_artifactId, slot))
            return;
        if (record.m_artifactId != ARTIFACT_NONE) {
            g_currentHero->removeArtifact(slot);
            g_currentHero->equipArtifact(
                &g_heroScreenDraggedArtifact, slot);
            if (g_game->m_f1f698 >= 2)
                g_currentHero->heroFn004DC100(slot);
            g_currentHero->updateStats();
            g_heroScreenDraggedArtifact = record;
#pragma inline_depth(0)
            g_heroScreenWindow->updateAllSlots();
#pragma inline_depth()
            g_heroScreenWindow->drawWindow(1, 0xffff0001, 0xffff);
            g_mouseManager->setPointer(
                g_heroScreenDraggedArtifact.m_artifactId,
                mouseManager::ARTIFACT_SET);
        } else {
            g_currentHero->equipArtifact(
                &g_heroScreenDraggedArtifact, slot);
            if (g_game->m_f1f698 >= 2)
                g_currentHero->heroFn004DC100(slot);
            g_currentHero->updateStats();
            g_heroScreenDraggedArtifact.m_artifactId = ARTIFACT_NONE;
#pragma inline_depth(0)
            g_heroScreenWindow->updateAllSlots();
#pragma inline_depth()
            g_heroScreenWindow->drawWindow(1, 0xffff0001, 0xffff);
            g_mouseManager->setPointer(0,
                                       mouseManager::DEFAULT_SET);
        }
        return;
    }

    if (record.m_artifactId == ARTIFACT_NONE)
        return;

    if (rightMouse) {
        if (g_game->m_f1f698 >= 2) {
            if (g_artifactTraits[record.m_artifactId].m_comboType
                != -1) {
                if (g_currentHero->heroFn004D9B30(
                        record.m_artifactId)
                    == DIALOG_RETURN_ACCEPT) {
                    g_currentHero->heroFn004DC070(slot);
                    g_currentHero->updateStats();
                    for (long i = THeroScreenWindow::ARTIFACT_SLOT_FIRST;
                         i < THeroScreenWindow::ARTIFACT_SLOT_COUNT; i++)
                        g_heroScreenWindow->updateSlot(TArtifactSlot(i));
                    g_heroScreenWindow->drawWindow(1, 0xffff0001,
                                                   0xffff);
                }
                return;
            }
            int targetCombo =
                g_artifactTraits[record.m_artifactId].m_targetCombo;
            if (targetCombo != -1) {
                std::bitset<144> missing =
                    g_combinationArtifacts[targetCombo].m_components;
                for (int k = 0; k < 19; k++) {
                    int worn =
                        g_currentHero->getArtifact(TArtifactSlot(k)).m_artifactId;
                    if (worn != ARTIFACT_NONE)
                        missing[worn] = false;
                }
                // MEASURED NEGATIVE THREE WAYS, do not retry. Retail
                // CALLS three bitset members in this block that our CL
                // expands: `bitset<144>::operator[]` (0x4cef80, the
                // 18-byte reference ctor), `reference::operator=`
                // (0x48e9f0, which carries set()'s body inlined) and
                // `any()` (0x4e64c0) - our compile instead calls
                // `set(size_t,bool)` once and expands any().
                // `#pragma inline_depth(0)` reproduces each of those
                // calls and every variant LOSES:
                //   pin on the whole `if (!missing.any())` statement
                //       74.4733 -> 70.80 (recorded 2026-08-20)
                //   pin on a HOISTED `bool = !missing.any();` alone,
                //       so the guarded body is out of the pin's reach
                //       74.4733 -> 70.2438 (2026-08-20)
                //   pin on `missing[worn] = false;` alone
                //       74.4733 -> 69.5800
                //   both site pins together
                //       74.4733 -> 70.3412
                // So the hoist DOES isolate the pin - the earlier
                // "the pin also de-inlines the guarded body" reading is
                // wrong - and imposing retail's calls still costs four
                // points. The cross-jumping defect below is upstream of
                // all of them.
                if (!missing.any()) {
                    if (g_currentHero->heroFn004D9CC0(
                            record.m_artifactId)
                        == DIALOG_RETURN_ACCEPT) {
                        g_currentHero->heroFn004DBF30(targetCombo,
                                                       slot);
                        g_currentHero->updateStats();
                        for (long i = THeroScreenWindow::ARTIFACT_SLOT_FIRST;
                             i < THeroScreenWindow::ARTIFACT_SLOT_COUNT; i++)
                            g_heroScreenWindow->updateSlot(TArtifactSlot(i));
                        g_heroScreenWindow->drawWindow(
                            1, 0xffff0001, 0xffff);
                    }
                    return;
                }
            }
        }
        g_currentHero->viewArtifact(&record, rightMouse);
        return;
    }

    if (slot == hero::EQUIPPED_SLOT_SPELLBOOK) {
#pragma inline_depth(0)
        TSpellbookWindow spellbook(
            *g_currentHero, 0, TSpellbookWindow::eContextNeither,
            g_currentHero->getSpecialTerrain());
#pragma inline_depth()
        spellbook.doModal(0);
        return;
    }
    if (slot == hero::EQUIPPED_SLOT_WAR_MACHINE_4) {
        normalDialog(g_generalText->getText(313), 1, -1, -1, 8, 3,
                     -1, 0, -1, 0, -1, 0);
        return;
    }
    if (!g_currentPlayer->isLocalHuman())
        return;
    g_heroScreenDraggedArtifact = record;
    g_currentHero->removeArtifact(slot);
    g_currentHero->updateStats();
#pragma inline_depth(0)
    g_heroScreenWindow->updateAllSlots();
#pragma inline_depth()
    g_heroScreenWindow->drawWindow(1, 0xffff0001, 0xffff);
    g_mouseManager->setPointer(
        g_heroScreenDraggedArtifact.m_artifactId,
        mouseManager::ARTIFACT_SET);
}

static void handleBackpackClick(long code, unsigned char rightMouse)
{
    long index = code;
    type_artifact record = g_currentHero->getBackpack(index);

    if (g_heroScreenDraggedArtifact.m_artifactId != ARTIFACT_NONE) {
        if (rightMouse)
            return;
        if (!g_currentHero->addToBackpack(
                &g_heroScreenDraggedArtifact, index)) {
            normalDialog(
                g_currentHero
                    ->getBackpackError(
                        g_heroScreenDraggedArtifact.m_artifactId)
                    .c_str(),
                1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            return;
        }
        updateBackpack();
        g_heroScreenDraggedArtifact.m_artifactId = ARTIFACT_NONE;
        for (long i = THeroScreenWindow::ARTIFACT_SLOT_FIRST;
             i < THeroScreenWindow::ARTIFACT_SLOT_COUNT; i++)
            g_heroScreenWindow->updateSlot(TArtifactSlot(i));
        g_heroScreenWindow->drawWindow(1, 0xffff0001, 0xffff);
        g_mouseManager->setPointer(0, mouseManager::DEFAULT_SET);
        return;
    }

    if (record.m_artifactId == ARTIFACT_NONE)
        return;

    if (rightMouse) {
        g_currentHero->viewArtifact(&record, rightMouse);
        return;
    }

    if (!g_currentPlayer->isLocalHuman())
        return;
    g_heroScreenDraggedArtifact = record;
    g_currentHero->removeBackpackArtifact(static_cast<short>(index));
    updateBackpack();
    for (long j = THeroScreenWindow::ARTIFACT_SLOT_FIRST;
         j < THeroScreenWindow::ARTIFACT_SLOT_COUNT; j++)
        g_heroScreenWindow->updateSlot(TArtifactSlot(j));
    g_heroScreenWindow->drawWindow(1, 0xffff0001, 0xffff);
    g_mouseManager->setPointer(
        g_heroScreenDraggedArtifact.m_artifactId,
        mouseManager::ARTIFACT_SET);
}

// E:\gamedcs\hero.cpp:3486. Retail has 5182 bytes; the DC caller has
// 3128 and retains calls to the two click handlers and its widget-update
// show_skills member. Preserve helper identities without conflating that
// widget update with the Complete skill-description popup.
// Both byte-indexed widget switches already have the retail case-to-arm
// assignments and source arm order. The id triplets group by statistic:
// {0x6b,0x76,0x8b} specialty, {0x6c,0x70,0x77} experience, and
// {0x6d,0x71,0x78} mana; SetupHeroView independently corroborates them.

// MAX 78.6458%: DC's exitFlag is NOT the base-handler result. It is zeroed
// before that call (dc 0xcf566), set by the accepted name dialog (line 3594),
// and tested after the switch (3949..3953) to forward the changed message.
// Restoring that flag alone leaves the shared consume return too late.
// The unchanged-hover EARLY RETURN (DC 3507..3508), together with the flag,
// places the consume return at retail's exact +0x79. Both sides now retain
// all fifteen normalDialog calls; the old end-of-function return merged
// help-dialog tails. The explicit rightMouse 1/0 assignments follow DC
// 3499..3502 and are byte-identical to the normalized expression here.

// Controls in the current TU: reread-status ternaries plus exitFlag give
// 69.0893 raw-probe similarity; adding the hover early return gives 76.6445
// (76.6464 in the normalized build). Explicit status call arms give 78.0658
// normalized; the left-click refresh guard raises that to 78.6458. A logical
// AND/OR spelling of the view-army test changes its lowering but gives
// 77.9125 normalized before the refresh guard; with the guard it remains
// below the retained ternary's raw-probe score (77.9857 vs 78.6439).

// Residual: retail's frame is 0x14c, ours 0x144; retail homes rightMouse in
// the dead message parameter slot while ours uses [ebp-0x20]. Artifact-click
// paths still merge one updateAllSlots/setPointer pair retail keeps separate.
// These cross-TU setPointer calls cannot be an inliner decision. The apparent
// QuickTown/QuickHero wait difference is an ICF-folded body, not a class error.
// Full sema CFG views still truncate this row at embedded jump-table data;
// use complete sema disassembly or an arm range, not the reported block total.

// THE FRONT END IS NOW RULED OUT TOO (2026-09-06). `genab run --gen rtm-fe`
// swaps C1XX 12.00.8168 in beside the RTM back end and sweeps all 146
// units: this function's bytes are IDENTICAL on both sides (it is absent
// from evidence/vc6/fe-generation-verdicts.tsv, which lists every function
// that differs at all), and hero.obj's five functions that DO differ
// (HeroFn_004E2550, equip_artifact, remove_artifact,
// THeroScreenWindow::update_slot, update_spell_list) are all back-end-only
// jb/jl loop-guard twins that move AWAY from retail. The captured IL is
// byte-identical between the two front ends apart from its own two-byte
// version word (rtm-generation.md §6), so there is no front-end lever
// here. The merge-set flip is a model gap, not a vintage.

// Retail's three army refresh arms also do NOT pass the status code as the
// constant the arm already knows: each emits
// `mov [gHeroScreenArmySlot],edi / call UpdateArmies / mov
// eax,[gHeroScreenArmySlot] / cmp eax,-1 / jne push 6 / push 5` in front
// of heroWindowManager::BroadcastMessage(0x200, .., 0x7f, 0x4008), at
// fn+0x6c5, fn+0x7e0 and fn+0x8bb - i.e. it RE-READS the global after
// the call and
// selects WIDGET_CLEAR_STATUS(6) / WIDGET_SET_STATUS(5) at run time. The
// values our three compiled arms pass as constants agree with what that select
// produces, so this is a spelling and not a polarity error. Writing it as
// the ternary in both arms costs 74.4733 -> 74.2419, so the select alone
// does not pay: the rest of those two blocks has to converge with it.
// why-branch's current `right_mouse`-as-int suggestion is also rejected:
// it costs 75.4051 -> 71.9404, and retail homes the normalized flag as a
// byte, agreeing with the unsigned-char source and the DC helper arities.
// Consolidating the three duplicated army-refresh tails into a tiny inline
// helper is a second six-statement shrink control (2026-08-21). Passing the
// status as an argument is byte-flat at 75.4051 with 129 branches and 87
// calls; selecting it inside the helper from gHeroScreenArmySlot regresses
// to 74.13 with 128 branches. The shared-tail helper therefore cannot move
// the whole-function cross-jump phase either, and is not retained.
// Conventional release VERIFY is bounded as well (2026-08-21): evaluating
// the header-inline `gpGame->GetCurrHero() == gpCurrentHero` invariant at
// doses 1, 3 and 5 is byte-flat at 75.4051%. The release evaluation is a real
// accessor candidate rather than an elided TRACE/ASSERT arm, but it does not
// move this whole-function cross-jump phase.
// The DC local census is exhausted too (2026-08-21): its only named locals
// are `exitFlag` and `infowin`. `exitFlag` is the base-handler result tested
// at entry, and type 0x4D89 proves `infowin` is the block-scoped
// TQuickHeroWindow already constructed in the hero-locator right-click arm.
// Retail likewise constructs and destroys that object in this arm, so neither
// local exposes missing source structure; their identifier spelling cannot
// affect code generation.
// Lead for the next lane (2026-09-04, polish lane 2): the DC dossier's
// `exitFlag` is NOT the base-handler result - it is initialised to 0 in
// the CAdvPopup::WindowHandler call's delay slot, written once (`= 1`)
// in the HERO_NAME_ID arm when dialogReturn == DIALOG_RETURN_ACCEPT (dc
// line 3594), and tested once after the switch (line 3949: `if
// (exitFlag) { dialogReturn = codeY; codeY = codeX = 10; return 2; }
// return 1;`). Retail's shape agrees: one return-1 epilogue at fn+0x79
// with 64 arms jumping back to it and the return-2 block inline after
// the HERO_NAME_ID arm. Spelled that way here (townManager::Main closed
// 23 points on the identical device) VC6 does produce the single shared
// epilogue, but places it after the source-last SELECT arm
// (show_hero_skills) instead of retail's MOUSE_MOVE `return 1`, and the
// score falls 75.41 -> 69.34; the current inline `return 2` keeps the
// epilogue at the end and scores higher only because more arms then
// align. What decides where VC6 lands the threaded return-1 block is
// the open question (Main lands it after the TOWN_x arm where retail
// has the selector arm); solve that and this device is worth ~15 pts.
// CENSUS 2026-09-05 (the sema CFG/call/reloc views are UNUSABLE on this row -
// the block builder gives up at the first inline jump table and reports
// "target 24 blocks / 8 calls"; the RAW `sema disasm` of both sides is
// complete, 1431 rows, and is the only usable oracle here).
// Retail emits 15 `NormalDialog` CALLS; this compile emits SIX out of the
// eleven source sites, because our C2 cross-jumps every right-click help arm
// (`test bl,bl / je <exit> / 12 pushes / mov ecx,<help text> / mov edx,4 /
// JMP <shared call>`) while retail ends each arm with its own `call
// NormalDialog / jmp <exit>` - behaviour-catalog D7, cross-jumping we perform
// and retail does not, and the same class as the shared `je` in
// game::ValidateVictoryLossConditions.  Retail also expands
// hero::viewStat's two dialog sites inline (0x4de1da..0x4de320:
// `add eax,-0x32 / mov cl,[eax+ecx+0x476]`, the 99-clamp, `setge cl`, the
// `or ecx,0x10000` pack and `mov ecx,[4*eax+0x6a7540]`).
// Two independent frame facts, both unexplained: retail reserves 0x14c and
// this compile 0x144 - EIGHT bytes short, i.e. two named locals missing -
// and retail homes `right_mouse` in the DEAD `msg` PARAMETER SLOT [ebp+8]
// with `this` at [ebp-0x18] and localPlayer at [ebp-0x10], where we use
// [ebp-0x20] / [ebp-0x24] / [ebp-0x18].  Retail's shared `return 1` epilogue
// sits at fn+0x7e, immediately after the MOUSE_MOVE arm's `mov eax,1`
// fall-through; ours is threaded to fn+0x128a at the very end.
// ARM ORDER IS NOT THE PROBLEM - MEASURED AND REFUTED 2026-09-05.  A
// standing brief said "the jump-table arm ORDER differs"; it does not.
// Both jump tables were decoded on both sides and they agree exactly.
// The two BYTE index tables are byte-identical (00 01 08 08 ... 08 04 05
// 06 07 and 00 00 00 11 ... 10 10 11 05), so the case-to-arm assignment
// is already right; and sorting each table's targets by address gives the
// SAME arm sequence on both sides - DESELECT 8,7,0,1,5,2,3,4,6 and SELECT
// 3,2,8,9,5,7,6,4,0,1,16,14,15,10,11,12,13,17.  Reordering the `case`
// labels can only make this worse.  Do not spend a round on it.

// AND THE 15-vs-6 NormalDialog CENSUS IS NOT AN INDEPENDENT DEFECT: it is
// the epilogue placement wearing a second face.  Retail's merged `return
// 1` block sits at fn+0x79, as the FALL-THROUGH successor of the
// MOUSE_MOVE arm, and all 64 other exits `jmp` back to it; ours sits at
// fn+0x128a, as the fall-through successor of the SOURCE-LAST arm
// (show_hero_skills).  Because our epilogue is the last block, the last
// arm's `call NormalDialog` + fall-through is a two-instruction tail that
// the six right-click help arms share, so C2's cross-jumper merges all six
// into it (`mov edx,4 / jmp 0x1285`).  Retail's arms end `call
// NormalDialog / jmp 0x5fd9` - the SAME two-instruction tail, six times,
// UNMERGED - purely because their shared successor is far away.  So there
// is exactly ONE decision left in this function, not two: which member of
// the epilogue merge-set C2 emits in place.  Retail keeps the FIRST
// (source-order) `return 1`; we keep the LAST.  Every documented attempt
// (the exitFlag device, caller shrink at four doses, the shared-tail
// helper, the status-code select, `right_mouse` as int, RTM-vs-SP3) moved
// something else and left that decision untouched - the exitFlag device in
// particular DOES produce the single shared epilogue and still places it
// after show_hero_skills (75.41 -> 69.34), which is what proves the
// placement is a separate knob from the merge.
// Two facts that any candidate explanation has to carry: retail's frame is
// 0x14c against our 0x144 (two named locals we do not have), and retail
// homes `right_mouse` in the DEAD `msg` parameter slot [ebp+8] while we
// spend a numbered local on it - both consistent with the frame being
// allocated after a different set of blocks survived.
// A third fact that BOUNDS the search: both sides emit exactly TWO `ret 4`
// sites, and the SECOND one (the HERO_NAME arm's `return 2`) is at the same
// place on both - base fn+0x198 against retail fn+0x19b.  So the merge-set
// question is only about which copy of `return 1` survives in place, and
// everything downstream of fn+0x19b is already aligned.
// MEASURED AND REJECTED 2026-09-05: wrapping everything after the MOUSE_MOVE
// arm in an `else` (dropping that arm's own `return 1` so the arm falls into
// a single trailing one - the shape retail's layout literally has, if/body,
// join, else-body) scores 70.97 against 74.22, and the epilogue does NOT
// move: it is still the last block, at fn+0x1291.  So the `else` is not the
// construct that puts retail's join early, and no source bracketing tried so
// far reaches C2's choice of surviving copy.
VA(0x004dd2d0, 0x143E)  // anchor-bracket + absent-callees, dc 0xcf54c
int THeroScreenWindow::windowHandler(message& msg)
{
    int exitFlag = 0;
    int result = CAdvPopup::windowHandler(msg);
    if (result)
        return result;

    playerData* localPlayer = g_game->getLocalPlayer();
    unsigned char rightMouse;
    if (msg.m_qualifier & MESSAGE_MODIFIER_RIGHT)
        rightMouse = 1;
    else
        rightMouse = 0;

    if (msg.m_id == MESSAGE_MOUSE_MOVE) {
        g_windowManager->convertToHover(msg);
        if (g_windowManager->m_lastHover == msg.m_codeY)
            return MESSAGE_DISPATCH_CONSUME;
        g_windowManager->m_lastHover = msg.m_codeY;
        updateHeroScreenStatusBar(&msg);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (msg.m_id == MESSAGE_KEY_UP
        && (msg.m_codeX == g_keyCodeLeftShift
            || msg.m_codeX == g_keyCodeRightShift)) {
        g_windowManager->m_lastHover = -1;
        g_inputManager->forceMouseMove();
    }
    if (msg.m_id == MESSAGE_KEY_DOWN
        && (msg.m_codeX == g_keyCodeLeftShift
            || msg.m_codeX == g_keyCodeRightShift)) {
        g_windowManager->m_lastHover = -1;
        g_inputManager->forceMouseMove();
    }
    if (msg.m_id != MESSAGE_WIDGET)
        return MESSAGE_DISPATCH_CONSUME;

    g_windowManager->m_lastHover = -1;

    switch (msg.m_codeX) {
    case widget::WIDGET_DESELECT:
        if (rightMouse)
            break;
        switch (msg.m_codeY) {
        case HERO_NAME_ID:
            normalDialog(g_generalText->getText(23), 2, -1, -1, -1, 0,
                         -1, 0, -1, 0, -1, 0);
            if (g_windowManager->m_dialogReturn == DIALOG_RETURN_ACCEPT) {
                exitFlag = 1;
            }
            break;
        case BACKPACK_SCROLL_LEFT_ID:
            g_currentHero->rotateBackpackLeft();
            updateBackpack();
            drawWindow(1, 0xffff0001, 0xffff);
            break;
        case BACKPACK_SCROLL_RIGHT_ID:
            g_currentHero->rotateBackpackRight();
            updateBackpack();
            drawWindow(1, 0xffff0001, 0xffff);
            break;
        case MIXED_ARMY_ID:
            g_heroScreenArmyStripLive = 1;
            g_currentHero->updateArmies();
            drawWindow(1, 0xffff0001, 0xffff);
            break;
        case WIDGET_7A_ID:
            g_currentHero->m_formation &= ~HERO_FORMATION_TIGHT;
            setupHeroView();
            drawWindow(1, 0xffff0001, 0xffff);
            break;
        case WIDGET_7C_ID:
            g_currentHero->m_formation |= HERO_FORMATION_TIGHT;
            setupHeroView();
            drawWindow(1, 0xffff0001, 0xffff);
            break;
        case FORMATION_ID:
            g_currentHero->m_formation ^= HERO_FORMATION_GROUPED;
            setupHeroView();
            drawWindow(1, 0xffff0001, 0xffff);
            break;
        case WIDGET_80_ID:
            doQuestLog(g_currentHero->m_owner);
            break;
        }
        break;

    case widget::WIDGET_SELECT:
    case widget::WIDGET_RIGHT_SELECT:
        switch (msg.m_codeY) {
        case PRIMARY_SKILL_0_ID:
        case PRIMARY_SKILL_1_ID:
        case PRIMARY_SKILL_2_ID:
        case PRIMARY_SKILL_3_ID:
            if (g_heroScreenDraggedArtifact.m_artifactId != ARTIFACT_NONE)
                break;
            g_currentHero->viewStat(msg.m_codeY - PRIMARY_SKILL_0_ID,
                                            rightMouse);
            break;

        case PORTRAIT_ID:
            if (g_heroScreenDraggedArtifact.m_artifactId != ARTIFACT_NONE)
                break;
            normalDialog(g_currentHero->heroFn004D8FB0(),
                         rightMouse ? hero::PRIMARY_STAT_QUICK_DIALOG_TYPE
                                     : hero::PRIMARY_STAT_DIALOG_TYPE,
                         -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            break;

        case MORALE_ID:
            if (g_heroScreenDraggedArtifact.m_artifactId != ARTIFACT_NONE)
                break;
            g_game->showMoraleInfo(g_currentHero,
                                   rightMouse ? hero::PRIMARY_STAT_QUICK_DIALOG_TYPE
                                               : hero::PRIMARY_STAT_DIALOG_TYPE);
            break;

        case LUCK_ID:
            if (g_heroScreenDraggedArtifact.m_artifactId != ARTIFACT_NONE)
                break;
            g_game->showLuckInfo(g_currentHero,
                                 rightMouse ? hero::PRIMARY_STAT_QUICK_DIALOG_TYPE
                                             : hero::PRIMARY_STAT_DIALOG_TYPE);
            break;

        case WIDGET_6B_ID:
        case WIDGET_76_ID:
        case WIDGET_8B_ID:
            if (g_heroScreenDraggedArtifact.m_artifactId != ARTIFACT_NONE)
                break;
            strcpy(g_text, g_heroSpecificAbilities[g_currentHero->m_id].m_longText);
            normalDialog(g_text,
                         rightMouse ? hero::PRIMARY_STAT_QUICK_DIALOG_TYPE
                                     : hero::PRIMARY_STAT_DIALOG_TYPE,
                         -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            break;

        case WIDGET_6D_ID:
        case WIDGET_71_ID:
        case WIDGET_78_ID:
            {
                if (g_heroScreenDraggedArtifact.m_artifactId != ARTIFACT_NONE)
                    break;
                sprintf(g_text, g_generalText->getText(206),
                        g_currentHero->m_name, g_currentHero->m_mana,
                        g_currentHero->getMaxMana());
                normalDialog(g_text,
                             rightMouse ? hero::PRIMARY_STAT_QUICK_DIALOG_TYPE
                                         : hero::PRIMARY_STAT_DIALOG_TYPE,
                             -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            }
            break;

        case WIDGET_6C_ID:
        case WIDGET_70_ID:
        case WIDGET_77_ID:
            if (g_heroScreenDraggedArtifact.m_artifactId != ARTIFACT_NONE)
                break;
            sprintf(g_text, g_generalText->getText(3),
                    g_currentHero->m_level,
                    hero::getExperience(g_currentHero->m_level + 1),
                    g_currentHero->m_experience);
            normalDialog(g_text,
                         rightMouse ? hero::PRIMARY_STAT_QUICK_DIALOG_TYPE
                                     : hero::PRIMARY_STAT_DIALOG_TYPE,
                         -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
            break;

        case ARMY_SLOT_0_ID:
        case ARMY_SLOT_1_ID:
        case ARMY_SLOT_2_ID:
        case ARMY_SLOT_3_ID:
        case ARMY_SLOT_4_ID:
        case ARMY_SLOT_5_ID:
        case ARMY_SLOT_6_ID:
            {
                long slot = msg.m_codeY - ARMY_SLOT_0_ID;
                if (!rightMouse
                    && g_heroScreenArmySlot == HERO_SCREEN_NO_ARMY_SLOT) {
                    if (g_currentHero->m_army.m_armies[slot] != CREATURE_NONE) {
                        g_heroScreenArmySlot = slot;
                        g_currentHero->updateArmies();
                        if (g_heroScreenArmySlot == HERO_SCREEN_NO_ARMY_SLOT) {
                            g_windowManager->broadcastMessage(
                                MESSAGE_WIDGET, widget::WIDGET_SET_STATUS,
                                MIXED_ARMY_ID,
                                widget::WIDGET_UPDATE | widget::WIDGET_DIMMED);
                        } else {
                            g_windowManager->broadcastMessage(
                                MESSAGE_WIDGET, widget::WIDGET_CLEAR_STATUS,
                                MIXED_ARMY_ID,
                                widget::WIDGET_UPDATE | widget::WIDGET_DIMMED);
                        }
                        g_heroScreenWindow->drawWindow(1, 0xffff0001, 0xffff);
                    }
                } else if (rightMouse
                               ? g_currentHero->m_army.m_armies[slot]
                                     != CREATURE_NONE
                               : g_heroScreenArmySlot == slot) {
                    g_heroScreenArmyStripLive = 0;
                    int showDismiss = 0;
                    if (!g_unnamed6aa9d8
                        && g_currentHero->m_army.getNumArmies() > 1)
                        showDismiss = 1;
                    g_game->viewArmy(g_currentHero->m_army, slot, g_currentHero,
                                     0, 0x77, 0x14, showDismiss, rightMouse);
                    if (!rightMouse)
                        g_heroScreenArmySlot = HERO_SCREEN_NO_ARMY_SLOT;
                    setupHeroView();
                    drawWindow(1, 0xffff0001, 0xffff);
                } else if (!rightMouse && g_unnamed6aa9d8) {
                    if (g_currentHero->m_army.m_armies[slot] != CREATURE_NONE) {
                        g_heroScreenArmySlot = slot;
                        g_currentHero->updateArmies();
                        if (g_heroScreenArmySlot == HERO_SCREEN_NO_ARMY_SLOT) {
                            g_windowManager->broadcastMessage(
                                MESSAGE_WIDGET, widget::WIDGET_SET_STATUS,
                                MIXED_ARMY_ID,
                                widget::WIDGET_UPDATE | widget::WIDGET_DIMMED);
                        } else {
                            g_windowManager->broadcastMessage(
                                MESSAGE_WIDGET, widget::WIDGET_CLEAR_STATUS,
                                MIXED_ARMY_ID,
                                widget::WIDGET_UPDATE | widget::WIDGET_DIMMED);
                        }
                        g_heroScreenWindow->drawWindow(1, 0xffff0001, 0xffff);
                    }
                } else if (!rightMouse) {
                    if ((g_heroScreenArmyStripLive
                         || (msg.m_qualifier & MESSAGE_MODIFIER_SHIFT_KEYS))
                        && (g_currentHero->m_army.m_armies[slot] == CREATURE_NONE
                            || g_currentHero->m_army.m_armies[slot]
                                   == g_currentHero->m_army
                                          .m_armies[g_heroScreenArmySlot])) {
                        g_heroScreenArmyStripLive = 0;
                        g_currentHero->m_army.splitArmy(
                            g_heroScreenArmySlot, &g_currentHero->m_army, slot,
                            0, 0);
                    } else if (g_currentHero->m_army.m_armies[slot]
                               == g_currentHero->m_army
                                      .m_armies[g_heroScreenArmySlot]) {
                        g_currentHero->m_army.m_numTroops[slot] +=
                            g_currentHero->m_army.m_numTroops[g_heroScreenArmySlot];
                        g_currentHero->m_army.m_numTroops[g_heroScreenArmySlot] = 0;
                        g_currentHero->m_army.m_armies[g_heroScreenArmySlot] =
                            CREATURE_NONE;
                    } else {
                        g_currentHero->m_army.swap(slot, &g_currentHero->m_army,
                                                 g_heroScreenArmySlot);
                    }
                    g_heroScreenArmySlot = HERO_SCREEN_NO_ARMY_SLOT;
                    g_currentHero->updateArmies();
                    if (g_heroScreenArmySlot == HERO_SCREEN_NO_ARMY_SLOT) {
                        g_windowManager->broadcastMessage(
                            MESSAGE_WIDGET, widget::WIDGET_SET_STATUS,
                            MIXED_ARMY_ID,
                            widget::WIDGET_UPDATE | widget::WIDGET_DIMMED);
                    } else {
                        g_windowManager->broadcastMessage(
                            MESSAGE_WIDGET, widget::WIDGET_CLEAR_STATUS,
                            MIXED_ARMY_ID,
                            widget::WIDGET_UPDATE | widget::WIDGET_DIMMED);
                    }
                    g_heroScreenWindow->drawWindow(1, 0xffff0001, 0xffff);
                }
                if (!rightMouse) {
                    g_windowManager->m_lastHover = -1;
                    updateHeroScreenStatusBar(&msg);
                }
            }
            break;

        case ARTIFACT_SLOT_0_ID:  case ARTIFACT_SLOT_1_ID:
        case ARTIFACT_SLOT_2_ID:  case ARTIFACT_SLOT_3_ID:
        case ARTIFACT_SLOT_4_ID:  case ARTIFACT_SLOT_5_ID:
        case ARTIFACT_SLOT_6_ID:  case ARTIFACT_SLOT_7_ID:
        case ARTIFACT_SLOT_8_ID:  case ARTIFACT_SLOT_9_ID:
        case ARTIFACT_SLOT_10_ID: case ARTIFACT_SLOT_11_ID:
        case ARTIFACT_SLOT_12_ID: case ARTIFACT_SLOT_13_ID:
        case ARTIFACT_SLOT_14_ID: case ARTIFACT_SLOT_15_ID:
        case ARTIFACT_SLOT_16_ID: case ARTIFACT_SLOT_17_ID:
        case ARTIFACT_SLOT_18_ID:
            handleArtifactClick(msg.m_codeY - ARTIFACT_SLOT_0_ID,
                                  rightMouse);
            break;

        case BACKPACK_SLOT_0_ID: case BACKPACK_SLOT_1_ID:
        case BACKPACK_SLOT_2_ID: case BACKPACK_SLOT_3_ID:
        case BACKPACK_SLOT_4_ID:
            handleBackpackClick(msg.m_codeY - BACKPACK_SLOT_0_ID,
                                  rightMouse);
            break;

        case HERO_LOCATOR_0_ID: case HERO_LOCATOR_1_ID:
        case HERO_LOCATOR_2_ID: case HERO_LOCATOR_3_ID:
        case HERO_LOCATOR_4_ID: case HERO_LOCATOR_5_ID:
        case HERO_LOCATOR_6_ID: case HERO_LOCATOR_7_ID:
            if (rightMouse) {
                TQuickHeroWindow quick(
                    g_game->getHero(localPlayer->m_heroes[
                        m_topHero + msg.m_codeY - HERO_LOCATOR_0_ID]),
                    TQuickHeroWindow::ViewAll);
                quick.m_x = 0x1a4;
                quick.m_y = 0x172;
                quick.quickWindowWait();
                break;
            }
            if (g_heroScreenArmySlot != HERO_SCREEN_NO_ARMY_SLOT)
                break;
            if (g_heroScreenDraggedArtifact.m_artifactId != ARTIFACT_NONE)
                break;
            g_heroScreenHeroPosition =
                m_topHero + msg.m_codeY - HERO_LOCATOR_0_ID;
            g_currentHero =
                g_game->getHero(localPlayer->m_heroes[g_heroScreenHeroPosition]);
            setupHeroView();
            drawWindow(1, 0xffff0001, 0xffff);
            break;

        case WIDGET_80_ID:
            if (rightMouse)
                normalDialog(g_unnamed6a5704, 4, -1, -1, -1, 0,
                             -1, 0, -1, 0, -1, 0);
            break;
        case HERO_NAME_ID:
            if (rightMouse)
                normalDialog(g_heroScreenHeroNameHelp, 4, -1, -1, -1, 0,
                             -1, 0, -1, 0, -1, 0);
            break;
        case WIDGET_7A_ID:
            if (rightMouse)
                normalDialog(g_heroScreenWidget7aHelp, 4, -1, -1, -1, 0,
                             -1, 0, -1, 0, -1, 0);
            break;
        case WIDGET_7C_ID:
            if (rightMouse)
                normalDialog(g_heroScreenWidget7cHelp, 4, -1, -1, -1, 0,
                             -1, 0, -1, 0, -1, 0);
            break;
        case FORMATION_ID:
            if (rightMouse)
                normalDialog(g_heroScreenFormationHelp, 4, -1, -1, -1, 0,
                             -1, 0, -1, 0, -1, 0);
            break;
        case MIXED_ARMY_ID:
            if (rightMouse)
                normalDialog(g_heroScreenMixedArmyHelp, 4, -1, -1, -1, 0,
                             -1, 0, -1, 0, -1, 0);
            break;

        default: {
            // Retail 0x4de4fc..0x4de5d4 handles the skill-popup range here.
            // DC show_skills instead calls GetWidget/show; it cannot own
            // this widget-id/rightMouse popup or justify a new wrapper.
            if (g_heroScreenDraggedArtifact.m_artifactId != ARTIFACT_NONE)
                break;
            int code = msg.m_codeY;
            int nth;
            if (code >= THeroScreenWindow::SKILL_ICON_FIRST_ID
                && code <= THeroScreenWindow::SKILL_ICON_LAST_ID)
                nth = code - THeroScreenWindow::SKILL_ICON_FIRST_ID;
            else if (code >= THeroScreenWindow::SKILL_NAME_FIRST_ID
                     && code <= THeroScreenWindow::SKILL_NAME_LAST_ID)
                nth = code - THeroScreenWindow::SKILL_NAME_FIRST_ID;
            else if (code < THeroScreenWindow::SKILL_LEVEL_FIRST_ID
                     || code > THeroScreenWindow::SKILL_LEVEL_LAST_ID)
                break;
            else
                nth = code - THeroScreenWindow::SKILL_LEVEL_FIRST_ID;
            if (nth >= g_currentHero->m_skillCount)
                break;
            int skill = g_currentHero->getNthSS(nth);
            strcpy(g_text,
                   g_sSkillTraits[skill]
                       .m_levelNames[g_currentHero->m_skillLevel[skill] - 1]);
            normalDialog(g_text,
                         rightMouse ? hero::PRIMARY_STAT_QUICK_DIALOG_TYPE
                                     : hero::PRIMARY_STAT_DIALOG_TYPE,
                         -1, -1, 0x14,
                         3 * skill + g_currentHero->m_skillLevel[skill] + 2,
                         -1, 0, -1, 0, -1, 0);
            break;
        }
        }
        break;
    }

    if (exitFlag) {
        g_windowManager->m_dialogReturn = msg.m_codeY;
        msg.m_codeY = 10;
        msg.m_codeX = 10;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

VA(0x004de710, 0x2C52)  // dc 0xd0184
THeroScreenWindow::THeroScreenWindow()
    : CAdvPopup(0x40, 7, 0x2a0, 0x24a, 0x12)
{
    m_topHero = 0;
    m_widgets.reserve(121);
    m_field64 = m_widgets.back();

    const char* background =
        g_game->m_f1f698 >= 2 ? "heroscr4.pcx" : "heroscr3.pcx";
    m_widgets.push_back(new bitmapBorder(
        0, 0, m_width, m_height, 0, background, 0x800));
    m_widgets.push_back(new textWidget(
        0x52, 0x1e, 0xdc, 0x28, 0, "bigfont.fnt",
        font::HEADING, 0x1, 0x1, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x52, 0x39, 0xdc, 0x1e, 0, "medfont.fnt",
        font::PRIMARY, 0x8c, 0x1, 0, 0x8));
    m_widgets.push_back(new bitmapBorder(
        0x13, 0x13, 0x3a, 0x40, 0x2d, 0, 0x800));
    m_widgets.push_back(new textWidget(
        0x14, 0x5b, 0x42, 0x12, 0, "smalfont.fnt",
        font::HEADING, 0x67, 0x1, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x5a, 0x5b, 0x42, 0x12, 0, "smalfont.fnt",
        font::HEADING, 0x68, 0x1, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0xa0, 0x5b, 0x42, 0x12, 0, "smalfont.fnt",
        font::HEADING, 0x69, 0x1, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0xe6, 0x5b, 0x42, 0x12, 0, "smalfont.fnt",
        font::HEADING, 0x6a, 0x1, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x44, 0xb8, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x6b, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x44, 0xe8, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x6c, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0xd3, 0xe8, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x6d, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x173, 0x1af, 0x42, 0x28, 0, "smalfont.fnt",
        font::PRIMARY, 0x6e, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x1ff, 0x1af, 0x42, 0x28, 0, "smalfont.fnt",
        font::PRIMARY, 0x6f, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x1f, 0x9e, 0x2c, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x2e, 0x1, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x65, 0x9e, 0x2c, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x2f, 0x1, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0xab, 0x9e, 0x2c, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x30, 0x1, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0xf1, 0x9e, 0x2c, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x31, 0x1, 0, 0x8));
    m_widgets.push_back(new iconWidget(
        0xf, 0x1e5, 0x3a, 0x40, 0x36, "twcrport.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x51, 0x1e5, 0x3a, 0x40, 0x37, "twcrport.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x93, 0x1e5, 0x3a, 0x40, 0x38, "twcrport.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0xd5, 0x1e5, 0x3a, 0x40, 0x39, "twcrport.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x117, 0x1e5, 0x3a, 0x40, 0x3a, "twcrport.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x159, 0x1e5, 0x3a, 0x40, 0x3b, "twcrport.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x19b, 0x1e5, 0x3a, 0x40, 0x3c, "twcrport.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new textWidget(
        0xf, 0x216, 0x3a, 0x12, 0, "Verd10B.fnt",
        font::PRIMARY, 0x3d, 0x2, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x51, 0x216, 0x3a, 0x12, 0, "Verd10B.fnt",
        font::PRIMARY, 0x3e, 0x2, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x93, 0x216, 0x3a, 0x12, 0, "Verd10B.fnt",
        font::PRIMARY, 0x3f, 0x2, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0xd5, 0x216, 0x3a, 0x12, 0, "Verd10B.fnt",
        font::PRIMARY, 0x40, 0x2, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x117, 0x216, 0x3a, 0x12, 0, "Verd10B.fnt",
        font::PRIMARY, 0x41, 0x2, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x159, 0x216, 0x3a, 0x12, 0, "Verd10B.fnt",
        font::PRIMARY, 0x42, 0x2, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x19b, 0x216, 0x3a, 0x12, 0, "Verd10B.fnt",
        font::PRIMARY, 0x43, 0x2, 0, 0x8));
    m_widgets.push_back(new iconWidget(
        0x20, 0x6f, 0x2a, 0x2a, 0x32, "pskil42.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x66, 0x6f, 0x2a, 0x2a, 0x33, "pskil42.def",
        0x1, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0xac, 0x6f, 0x2a, 0x2a, 0x34, "pskil42.def",
        0x2, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0xf2, 0x6f, 0x2a, 0x2a, 0x35, "pskil42.def",
        0x5, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0xf, 0x1e5, 0x3a, 0x40, 0x44, "twcrport.def",
        0x1, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x51, 0x1e5, 0x3a, 0x40, 0x45, "twcrport.def",
        0x1, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x93, 0x1e5, 0x3a, 0x40, 0x46, "twcrport.def",
        0x1, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0xd5, 0x1e5, 0x3a, 0x40, 0x47, "twcrport.def",
        0x1, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x117, 0x1e5, 0x3a, 0x40, 0x48, "twcrport.def",
        0x1, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x159, 0x1e5, 0x3a, 0x40, 0x49, "twcrport.def",
        0x1, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x19b, 0x1e5, 0x3a, 0x40, 0x4a, "twcrport.def",
        0x1, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x12, 0x114, 0x2c, 0x2c, 0x4f, "secskill.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0xa1, 0x114, 0x2c, 0x2c, 0x50, "secskill.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x12, 0x144, 0x2c, 0x2c, 0x51, "secskill.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0xa1, 0x144, 0x2c, 0x2c, 0x52, "secskill.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x12, 0x174, 0x2c, 0x2c, 0x53, "secskill.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0xa1, 0x174, 0x2c, 0x2c, 0x54, "secskill.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x12, 0x1a4, 0x2c, 0x2c, 0x55, "secskill.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0xa1, 0x1a4, 0x2c, 0x2c, 0x56, "secskill.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new textWidget(
        0x44, 0x12c, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x57, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0xd3, 0x12c, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x58, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x44, 0x15b, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x59, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0xd3, 0x15b, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x5a, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x44, 0x18b, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x5b, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0xd3, 0x18b, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x5c, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x44, 0x1bb, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x5d, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0xd3, 0x1bb, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x5e, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x44, 0x118, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x5f, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0xd3, 0x118, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x60, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x44, 0x148, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x61, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0xd3, 0x148, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x62, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x44, 0x178, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x63, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0xd3, 0x178, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x64, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x44, 0x1a8, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x65, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0xd3, 0x1a8, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x66, 0, 0, 0x8));
    m_widgets.push_back(new iconWidget(
        0x1bc, 0x16, 0x2c, 0x2c, 0x15, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x1f7, 0xea, 0x2c, 0x2c, 0x16, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x1bc, 0x48, 0x2c, 0x2c, 0x17, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x13e, 0x3d, 0x2c, 0x2c, 0x18, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x1f1, 0xb0, 0x2c, 0x2c, 0x19, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x1bc, 0x7b, 0x2c, 0x2c, 0x1a, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x16e, 0x3d, 0x2c, 0x2c, 0x1b, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x221, 0xb0, 0x2c, 0x2c, 0x1c, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x1c2, 0x11f, 0x2c, 0x2c, 0x1d, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x13e, 0x87, 0x2c, 0x2c, 0x1e, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x14e, 0xb9, 0x2c, 0x2c, 0x1f, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x15e, 0xec, 0x2c, 0x2c, 0x20, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x16e, 0x11f, 0x2c, 0x2c, 0x21, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x1f3, 0x16, 0x2c, 0x2c, 0x22, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x221, 0x16, 0x2c, 0x2c, 0x23, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x221, 0x44, 0x2c, 0x2c, 0x24, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x221, 0x72, 0x2c, 0x2c, 0x25, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x221, 0x12f, 0x2c, 0x2c, 0x26, "artifact.def",
        0, 0, 0, 0, 0x10));
    if (g_game->m_f1f698 >= 2)
        m_widgets.push_back(new iconWidget(
            0x13c, 0x11f, 0x2c, 0x2c, 0x27, "artifact.def",
            0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x1bc, 0x16, 0x2c, 0x2c, 0x2, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x1f7, 0xea, 0x2c, 0x2c, 0x3, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x1bc, 0x48, 0x2c, 0x2c, 0x4, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x13e, 0x3d, 0x2c, 0x2c, 0x5, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x1f1, 0xb0, 0x2c, 0x2c, 0x6, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x1bc, 0x7b, 0x2c, 0x2c, 0x7, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x16e, 0x3d, 0x2c, 0x2c, 0x8, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x221, 0xb0, 0x2c, 0x2c, 0x9, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x1c2, 0x11f, 0x2c, 0x2c, 0xa, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x13e, 0x87, 0x2c, 0x2c, 0xb, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x14e, 0xb9, 0x2c, 0x2c, 0xc, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x15e, 0xec, 0x2c, 0x2c, 0xd, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x16e, 0x11f, 0x2c, 0x2c, 0xe, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x1f3, 0x16, 0x2c, 0x2c, 0xf, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x221, 0x16, 0x2c, 0x2c, 0x10, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x221, 0x44, 0x2c, 0x2c, 0x11, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x221, 0x72, 0x2c, 0x2c, 0x12, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x221, 0x12f, 0x2c, 0x2c, 0x13, "artifact.def",
        0, 0, 0, 0, 0x10));
    if (g_game->m_f1f698 >= 2)
        m_widgets.push_back(new iconWidget(
            0x13c, 0x11f, 0x2c, 0x2c, 0x14, "artifact.def",
            0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x152, 0x165, 0x2c, 0x2c, 0x28, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x180, 0x165, 0x2c, 0x2c, 0x29, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x1ae, 0x165, 0x2c, 0x2c, 0x2a, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x1dc, 0x165, 0x2c, 0x2c, 0x2b, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x20a, 0x165, 0x2c, 0x2c, 0x2c, "artifact.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0xb6, 0xb8, 0x2c, 0x2c, 0x74, "imrlb.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0xf0, 0xb8, 0x2c, 0x2c, 0x75, "ilckb.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x12, 0xb4, 0x2c, 0x2c, 0x76, "un44.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x13, 0xe5, 0x2a, 0x2a, 0x77, "pskil42.def",
        0x4, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0xa2, 0xe5, 0x2a, 0x2a, 0x78, "pskil42.def",
        0x3, 0, 0, 0, 0x10));
    m_widgets.push_back(new iconWidget(
        0x25d, 0x7, 0x3a, 0x40, 0x8d, "crest58.def",
        0, 0, 0, 0, 0x10));
    m_widgets.push_back(new bitmapBorder(
        0x263, 0x57, 0x30, 0x20, 0x82, 0, 0x800));
    m_widgets.push_back(new bitmapBorder(
        0x263, 0x8d, 0x30, 0x20, 0x83, 0, 0x800));
    m_widgets.push_back(new bitmapBorder(
        0x263, 0xc3, 0x30, 0x20, 0x84, 0, 0x800));
    m_widgets.push_back(new bitmapBorder(
        0x263, 0xf9, 0x30, 0x20, 0x85, 0, 0x800));
    m_widgets.push_back(new bitmapBorder(
        0x263, 0x12f, 0x30, 0x20, 0x86, 0, 0x800));
    m_widgets.push_back(new bitmapBorder(
        0x263, 0x165, 0x30, 0x20, 0x87, 0, 0x800));
    m_widgets.push_back(new bitmapBorder(
        0x263, 0x19b, 0x30, 0x20, 0x88, 0, 0x800));
    m_widgets.push_back(new bitmapBorder(
        0x263, 0x1d1, 0x30, 0x20, 0x89, 0, 0x800));
    m_widgets.push_back(new bitmapBorder(
        0x263, 0x57, 0x30, 0x20, 0x8a, "hpsyyy.pcx", 0x800));
    m_widgets.push_back(new bitmapBorder(
        0x8, 0x22f, 0x290, 0x13, 0x72, "HeroBar.pcx", 0x800));
    m_widgets.push_back(new textWidget(
        0x8, 0x22f, 0x290, 0x13, 0, "smalfont.fnt",
        font::PRIMARY, 0x73, 0x1, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x44, 0xcc, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x8b, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0x44, 0xfc, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x70, 0, 0, 0x8));
    m_widgets.push_back(new textWidget(
        0xd3, 0xfc, 0x5a, 0x12, 0, "smalfont.fnt",
        font::PRIMARY, 0x71, 0, 0, 0x8));
    m_widgets.push_back(new button(
        0x21b, 0x207, 0x36, 0x20, 0x7f, "hsbtns9.def",
        0, 0x1, 0, 0, 0x2));
    m_widgets.push_back(new button(
        0x13b, 0x1ae, 0x34, 0x24, 0x80, "hsbtns4.def",
        0, 0x1, 0, 0x10, 0x2));
    m_widgets.push_back(new button(
        0x1c6, 0x1ae, 0x34, 0x24, 0x81, "hsbtns2.def",
        0, 0x1, 0, 0x20, 0x2));
    button* exitButton = new button(
        0x262, 0x204, 0x34, 0x24, 0x7800, "hsbtns.def",
        0, 0x1, 0x1, 0x1c, 0x2);
    exitButton->setHotkey(1);
    m_widgets.push_back(exitButton);
    m_widgets.push_back(new button(
        0x13a, 0x164, 0x16, 0x2e, 0x4d, "hsbtns3.def",
        0, 0x1, 0, 0x4b, 0x2));
    m_widgets.push_back(new button(
        0x237, 0x164, 0x16, 0x2e, 0x4e, "hsbtns5.def",
        0, 0x1, 0, 0x4d, 0x2));
    m_widgets.push_back(new button(
        0x1e1, 0x1e3, 0x36, 0x20, 0x7a, "hsbtns6.def",
        0, 0x1, 0, 0x26, 0x2));
    m_widgets.push_back(new button(
        0x1e1, 0x207, 0x36, 0x20, 0x7c, "hsbtns7.def",
        0, 0x1, 0, 0x14, 0x2));
    m_widgets.push_back(new button(
        0x21b, 0x1e3, 0x36, 0x20, 0x7e, "hsbtns8.def",
        0, 0x1, 0, 0x30, 0x2));

    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            addWidget(*it, -1);
        else
            memError();
    }
}

#if 0  // @carcass

// Canonical body and VA: include/button.h.

VA_COMPGEN(0x004e1520, 0x21, SCALAR_DELETING_DTOR, THeroScreenWindow)

#endif  // @carcass

VA(0x004e1550, 0xA2)  // dc 0xd2be8
THeroScreenWindow::~THeroScreenWindow()
{
    if (g_heroScreenDraggedArtifact.m_artifactId != ARTIFACT_NONE) {
        g_currentHero->giveArtifact(&g_heroScreenDraggedArtifact, 0, 0);
        g_heroScreenDraggedArtifact.m_artifactId = ARTIFACT_NONE;
        g_mouseManager->setPointer(0, mouseManager::DEFAULT_SET);
    }

    g_heroScreenArmySlot = -1;
    for (widget** it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (*it)
            delete *it;
    }
}

VA(0x004e1600, 0xCB)  // dc 0xd2c80
void THeroScreenWindow::updateHeroLocator(int which)
{
    playerData* localPlayer = g_game->getLocalPlayer();
    if (which >= localPlayer->m_numHeroes) {
        widgetClearStatus(which + 0x82, 2);
        return;
    }

    hero* displayedHero = g_game->getHero(
        localPlayer->m_heroes[m_topHero + which]);
    const char* portraitName =
        g_heroTraits[displayedHero->m_portrait].m_smallPortraitName;
    union {
        const char* m_pointer;
        int m_value;
    } portraitMessage;
    portraitMessage.m_pointer = portraitName;
    broadcastMessage(0x200, 0xb, which + 0x82,
                     portraitMessage.m_value);

    if (m_topHero + which == g_heroScreenHeroPosition) {
        widgetSetStatus(0x8a, 6);
        broadcastMessage(0x200, 0x35, 0x8a, 0x57 + 54 * which);
    }
}

// E:\gamedcs\hero.cpp:4231
void THeroScreenWindow::updateHeroLocators()
{
    widgetClearStatus(0x8a,
                      widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
    for (int locator = 0; locator < 8; locator++)
        updateHeroLocator(locator);
}

VA(0x004e16d0, 0x130)  // dc 0xd2d58
void hero::updateStats()
{
    message msg;
    msg.m_codeY = 0;
    msg.m_qualifier = 0;
    msg.m_mouseX = 0;
    msg.m_mouseY = 0;
    msg.m_window = 0;
    msg.m_id = MESSAGE_WIDGET;
    msg.m_codeX = widget::WIDGET_SET_TEXT;
    msg.m_extraText = g_text;

    for (int i = 0; i < 4; i++) {
        sprintf(g_text, "%d", g_currentHero->getPrimarySkill(i));
        msg.m_codeY = i + 0x2e;
        g_heroScreenWindow->broadcastMessage(msg);
    }

    int luckFrame = limit(-3, getLuck(0, 0, 1), 3) + 3;
    msg.m_codeX = widget::WIDGET_SET_ICON_FRAME;
    msg.m_codeY = 0x75;
    msg.m_extra = luckFrame;
    g_heroScreenWindow->broadcastMessage(msg);

    int moraleFrame = limit(-3, getMorale(0, 0, 1), 3) + 3;
    msg.m_codeY = 0x74;
    msg.m_extra = moraleFrame;
    g_heroScreenWindow->broadcastMessage(msg);
}

VA(0x004e1800, 0x24F)  // dc 0xd2e80
int heroView(int heroID, int noDismiss, int alreadyFaded, unsigned char quickView)
{
    g_heroScreenNoDismiss = noDismiss;
    g_heroScreenHeroId = heroID;
    g_heroScreenDraggedArtifact.m_artifactId = ARTIFACT_NONE;
    g_heroScreenArmyStripLive = 0;
    g_advManager->trimLoopingSounds(4);

    g_currentHero = g_game->getHero(heroID);

    g_heroScreenWindow = new THeroScreenWindow();
    if (!g_heroScreenWindow)
        memError();
    setWinText(g_heroScreenWindow, g_heroScreenWinText);

    if (g_currentPlayer->isLocalHuman()
        && g_currentPlayer->m_currHeroId == g_currentHero->m_id) {
        type_point position;
        position.m_x = g_currentHero->m_x;
        position.m_y = g_currentHero->m_y;
        position.m_z = g_currentHero->m_z;
        NewmapCell* cell = g_advManager->getCell(position);
        if (cell->m_type != HERO || !cell->m_isTrigger)
            g_advManager->demobilizeCurrHero(0, 0);
    }

    playerData* localPlayer = g_game->getLocalPlayer();
    g_heroScreenHeroPosition = localPlayer->findHero(g_currentHero->m_id);
    g_heroScreenWindow->setupHeroView();

    if (quickView) {
        g_windowManager->doQuickView(g_heroScreenWindow);
    } else {
        g_heroScreenWindow->doModal(0);
        g_advManager->reseed(0, 0);
    }
    delete g_heroScreenWindow;

    if (g_windowManager->m_dialogReturn == g_dialogReturnDismissHero) {
        g_currentHero->deallocate(1, 0);
        if (!alreadyFaded) {
            g_advManager->fizzleCenter(0);
            g_advManager->updateRadar(1, 1, 0, 0, 0);
            g_advManager->m_advWindow->updateHeroLocators(-1, 1, 1);
        }
        return 1;
    }
    g_currentHero->m_maxMovePoints =
        g_currentHero->getMobility((g_currentHero->m_flags >> 18) & 1);
    g_currentHero = 0;
    return 0;
}

VA(0x004e1a50, 0x7BB)  // dc 0xd30b0
void THeroScreenWindow::setupHeroView()
{
    int noDismiss = g_heroScreenNoDismiss;
    if (g_currentHero->obscuresTown())
        noDismiss = 1;
    if (g_game->m_mapHeader.m_lossCondition.checkForDefeatedHeroLoss(g_currentHero))
        noDismiss = 1;

    playerData* localPlayer = g_game->getLocalPlayer();

    message msg;
    msg.m_id = MESSAGE_WIDGET;
    msg.m_qualifier = 0;
    msg.m_mouseX = 0;
    msg.m_mouseY = 0;
    msg.m_extra = 0;
    msg.m_window = 0;
    msg.m_codeX = widget::WIDGET_SET_PLAYER_PALETTE_COLORS;
    msg.m_codeY = 0;
    msg.m_extra = g_game->getLocalPlayerGamePos();
    broadcastMessage(msg);

    strcpy(g_text, g_currentHero->m_name);
    msg.m_codeX = widget::WIDGET_SET_TEXT;
    msg.m_codeY = 0x1;
    msg.m_extraText = g_text;
    broadcastMessage(msg);

    sprintf(g_text,
            g_generalText->getText(GENERAL_TEXT_HERO_LEVEL_CLASS_FORMAT),
            g_currentHero->m_level, g_currentHero->heroFn004D8F70());
    msg.m_codeY = 0x8c;
    msg.m_extraText = g_text;
    broadcastMessage(msg);

    if (g_unnamed6aa9d8) {
        widgetClearStatus(0x8a, widget::WIDGET_DRAWN);
    } else {
        updateHeroLocators();
    }

    msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
    msg.m_extra = widget::WIDGET_DRAWN;
    for (int slotIcon = 0; slotIcon < 7; slotIcon++) {
        msg.m_codeY = slotIcon + 0x44;
        broadcastMessage(msg);
    }

    if (!noDismiss && !g_unnamed6aa9d8 &&
        (localPlayer->m_numTowns || localPlayer->m_numHeroes != 1)) {
        broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_STATUS, 0x81,
                         widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
        broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_CLEAR_STATUS, 0x81,
                         widget::WIDGET_DIMMED_NODRAW);
    } else {
        broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_STATUS, 0x81,
                         widget::WIDGET_DIMMED_NODRAW);
    }

    union {
        const char* m_pointer;
        int m_value;
    } portraitMessage;
    portraitMessage.m_pointer =
        g_heroTraits[g_currentHero->m_portrait].m_largePortraitName;
    broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_IMAGE, 0x2d,
                     portraitMessage.m_value);

    g_currentHero->updateStats();

    msg.m_codeX = widget::WIDGET_SET_ICON_FRAME;
    msg.m_codeY = 0x76;
    msg.m_extra = g_currentHero->m_id;
    broadcastMessage(msg);

    sprintf(g_text, g_heroSpecificAbilities[g_currentHero->m_id].m_shortText);
    msg.m_codeX = widget::WIDGET_SET_TEXT;
    msg.m_codeY = 0x8b;
    msg.m_extraText = g_text;
    broadcastMessage(msg);

    sprintf(g_text, "%d", g_currentHero->m_experience);
    msg.m_codeY = 0x70;
    broadcastMessage(msg);

    if (g_currentHero->m_formation & 1) {
        msg.m_codeX = widget::WIDGET_SET_STATUS;
        msg.m_codeY = 0x7c;
        msg.m_extra = widget::WIDGET_HIGHLIGHTED;
        broadcastMessage(msg);
        msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
        msg.m_codeY = 0x7a;
        broadcastMessage(msg);
    } else {
        msg.m_codeX = widget::WIDGET_SET_STATUS;
        msg.m_codeY = 0x7a;
        msg.m_extra = widget::WIDGET_HIGHLIGHTED;
        broadcastMessage(msg);
        msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
        msg.m_codeY = 0x7c;
        broadcastMessage(msg);
    }

    if (g_currentHero->hasSecondarySkill(eSecSkillBattleTactics)) {
        broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_STATUS, 0x7e,
                         widget::WIDGET_ACTIVE | widget::WIDGET_DRAWN);
        broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_CLEAR_STATUS, 0x7e,
                         widget::WIDGET_DIMMED_NODRAW);
        if (g_currentHero->m_formation & 2)
            broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_STATUS, 0x7e,
                             widget::WIDGET_HIGHLIGHTED);
        else
            broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_CLEAR_STATUS, 0x7e,
                             widget::WIDGET_HIGHLIGHTED);
    } else {
        broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_SET_STATUS, 0x7e,
                         widget::WIDGET_DIMMED_NODRAW);
        broadcastMessage(MESSAGE_WIDGET, widget::WIDGET_CLEAR_STATUS, 0x7e,
                         widget::WIDGET_HIGHLIGHTED);
    }

    sprintf(g_text, "%d/%d", g_currentHero->m_mana,
            g_currentHero->getMaxMana());
    msg.m_codeX = widget::WIDGET_SET_TEXT;
    msg.m_codeY = 0x71;
    msg.m_extraText = g_text;
    broadcastMessage(msg);

    msg.m_codeX = widget::WIDGET_SET_ICON_FRAME;
    msg.m_codeY = 0x8d;
    if (g_currentHero->m_owner == -1)
        msg.m_extra = 8;
    else
        msg.m_extra = g_currentHero->m_owner;
    broadcastMessage(msg);

    g_currentHero->updateArmies();

    for (int i = 0; i < 8; i++) {
        if (i < g_currentHero->m_skillCount) {
            int skill = g_currentHero->getNthSS(i);

            msg.m_codeX = widget::WIDGET_SET_ICON_FRAME;
            msg.m_codeY = i + 0x4f;
            msg.m_extra = skill * 3 + g_currentHero->m_skillLevel[skill] + 2;
            broadcastMessage(msg);

            strcpy(g_text, g_sSkillTraits[skill].m_name);
            msg.m_codeX = widget::WIDGET_SET_TEXT;
            msg.m_codeY = i + 0x57;
            msg.m_extraText = g_text;
            broadcastMessage(msg);

            strcpy(g_text,
                   g_skillMasteryNamesBiased[g_currentHero->m_skillLevel[skill]]);
            msg.m_codeY = i + 0x5f;
            broadcastMessage(msg);

            msg.m_codeX = widget::WIDGET_SET_STATUS;
            msg.m_codeY = i + 0x4f;
            msg.m_extra = widget::WIDGET_DRAWN;
            broadcastMessage(msg);
            msg.m_codeY = i + 0x57;
            msg.m_extra = widget::WIDGET_DRAWN;
            broadcastMessage(msg);
            msg.m_codeY = i + 0x5f;
            msg.m_extra = widget::WIDGET_DRAWN;
            broadcastMessage(msg);
        } else {
            msg.m_codeX = widget::WIDGET_CLEAR_STATUS;
            msg.m_codeY = i + 0x4f;
            msg.m_extra = widget::WIDGET_DRAWN;
            broadcastMessage(msg);
            msg.m_codeY = i + 0x57;
            msg.m_extra = widget::WIDGET_DRAWN;
            broadcastMessage(msg);
            msg.m_codeY = i + 0x5f;
            msg.m_extra = widget::WIDGET_DRAWN;
            broadcastMessage(msg);
        }
    }

    updateAllSlots();
    updateBackpack();

    msg.m_codeX = widget::WIDGET_SET_STATUS;
    msg.m_codeY = 0x7f;
    msg.m_extra = widget::WIDGET_DIMMED_NODRAW;
    broadcastMessage(msg);

    if (!g_currentPlayer->isLocalHuman()) {
        getWidget(0x44)->enable(0);
        getWidget(0x45)->enable(0);
        getWidget(0x46)->enable(0);
        getWidget(0x47)->enable(0);
        getWidget(0x48)->enable(0);
        getWidget(0x49)->enable(0);
        getWidget(0x4a)->enable(0);
        getWidget(0x7f)->enable(0);
        getWidget(0x81)->enable(0);
        getWidget(0x7a)->enable(0);
        getWidget(0x7c)->enable(0);
        getWidget(0x7e)->enable(0);
        getWidget(0x7800)->enable(1);
    }
}

VA(0x004e2210, 0x3F)  // dc 0xd36e0
void hero::setSS(int whichSS, int levelToSet)
{
    if (levelToSet == 0) {
        takeSS(whichSS, 3);
        return;
    }
    if (m_skillLevel[whichSS] == 0) {
        giveSS(whichSS, levelToSet);
        return;
    }
    m_skillLevel[whichSS] = levelToSet;
}

VA(0x004e2250, 0x76)  // dc 0xd3730
int hero::takeSS(int whichSS, int numLevelsToTake)
{
    int oldLevel = m_skillLevel[whichSS];
    if (m_skillLevel[whichSS] > 0) {
        m_skillLevel[whichSS] -= numLevelsToTake;
        if (m_skillLevel[whichSS] < 0)
            m_skillLevel[whichSS] = 0;
        if (m_skillLevel[whichSS] == 0) {
            for (int i = 0; i < 28; i++) {
                if (m_skillOrder[i] > m_skillOrder[whichSS])
                    m_skillOrder[i]--;
            }
            m_skillOrder[whichSS] = 0;
            m_skillCount--;
        }
    }
    return oldLevel - m_skillLevel[whichSS];
}

VA(0x004e22d0, 0x61)  // dc 0xd37c0
int hero::giveSS(int whichSS, int numLevelsToGive)
{
    HOMM3_RELEASE_VERIFY(whichSS >= 0
        && whichSS < sizeof(m_skillLevel) / sizeof(m_skillLevel[0]));
    int oldLevel = m_skillLevel[whichSS];
    if (m_skillLevel[whichSS] > 0) {
        m_skillLevel[whichSS] += numLevelsToGive;
    } else {
        if (m_skillCount < 8) {
            m_skillLevel[whichSS] = numLevelsToGive;
            m_skillOrder[whichSS] = m_skillCount + 1;
            m_skillCount++;
        }
    }
    if (m_skillLevel[whichSS] > 3)
        m_skillLevel[whichSS] = 3;
    return m_skillLevel[whichSS] - oldLevel;
}

// The DC formal type is non-const, and its source body tests skillOrder.
// SetupHeroView calls this ordinary TU helper; no header force-inline view.
// E:\gamedcs\hero.cpp:4689, dc 0xd38d8
unsigned char hero::hasSecondarySkill(int whichSkill)
{
    return m_skillOrder[whichSkill] > 0;
}

VA(0x004e2340, 0x2A)  // dc 0xd3830
int hero::creatureTypeCount(int creatureType)
{
    int count = 0;
    for (int slot = 0; slot < 7; slot++) {
        if (m_army.m_armies[slot] == creatureType && m_army.m_numTroops[slot] > 0)
            count++;
    }
    return count;
}

VA(0x004e2370, 0x26)  // dc 0xd3874
void hero::upgradeCreatures(int sourceCreatureType, int destCreatureType)
{
    for (int slot = 0; slot < armyGroup::ARMY_GROUP_SLOT_COUNT; slot++) {
        if (m_army.m_armies[slot] == sourceCreatureType)
            m_army.m_armies[slot] = destCreatureType;
    }
}

VA(0x004e23a0, 0x27)  // dc 0xd38b0
int hero::getNthSS(int which)
{
    for (int skill = 0; skill < 28; skill++) {
        if (m_skillOrder[skill] == which + 1)
            return skill;
    }
    return -1;
}

VA(0x004e23d0, 0x176)  // dc 0xd38ec
void hero::transferArtifacts(hero* src)
{
    if (!src)
        return;
    type_artifact artifact;
    for (int slot = 0; slot < 19; slot++) {
        artifact = src->m_equipped[slot];
        if (artifact.m_artifactId == ARTIFACT_NONE ||
            artifact.m_artifactId == ARTIFACT_HOLY_GRAIL ||
            artifact.m_artifactId == ARTIFACT_SPELLBOOK ||
            artifact.m_artifactId == ARTIFACT_CATAPULT ||
            artifact.m_artifactId == ARTIFACT_BALLISTA ||
            artifact.m_artifactId == ARTIFACT_AMMO_CART ||
            artifact.m_artifactId == ARTIFACT_FIRST_AID_TENT)
            continue;
        if (!addToBackpack(&artifact, -1))
            return;
        src->removeArtifact(slot);
    }
    for (int index = 63; index >= 0; index--) {
        artifact = src->m_backpack[index];
        if (artifact.m_artifactId == ARTIFACT_NONE ||
            artifact.m_artifactId == ARTIFACT_HOLY_GRAIL ||
            artifact.m_artifactId == ARTIFACT_SPELLBOOK ||
            artifact.m_artifactId == ARTIFACT_CATAPULT ||
            artifact.m_artifactId == ARTIFACT_BALLISTA ||
            artifact.m_artifactId == ARTIFACT_AMMO_CART ||
            artifact.m_artifactId == ARTIFACT_FIRST_AID_TENT)
            continue;
        if (!addToBackpack(&artifact, -1))
            return;
        src->removeBackpackArtifact(index);
    }
}

// Can `artifact` be worn in `slot` (or, with slot -1, in ANY of the 19)?
// NOT EH-bearing: retail leaves all four bitset::_Xran sites out of line,
// so no throw is expanded and no /GX frame is needed.

// The redundant-looking DOUBLE capacity check in each block is real. The
// empty-mask fold at +0x321 (`test eax,eax / je <continue>`) only exists
// because a FIRST `worn >= capacity` test precedes the `capacity -
// occupied` one; with capacity 0 VC6 folds it to always-taken. Do not
// simplify it away. `worn` is UNSIGNED - retail compares it with `jbe`
// and every later bound with `jae`, which is what bitset::count()'s
// size_t return gives.

// 83.5960 -> 92.9185 (2026-08-20) ON THE COMPONENT LOOP'S SHAPE, and the
// evidence that found it is a MISSING RELOCATION, not a branch report.
// `predict-inline` said base 3 out-of-line calls against retail's 4: we
// emit bitset<19>::_Xran three times and retail emits it three times PLUS
// one bitset<144>::_Xran (0x4346d0) that we did not emit at all. That
// fourth call is `components.test(component)`'s bounds check, and the two
// compiles disagree about it because they disagree about the LOOP FORM:
//   retail  fn+0x1f3: `cmp esi,0x1200 / jb <body> / mov ecx,ebx /
//                      call bitset<144>::_Xran`  - an UNSIGNED guard at
//           the top of the body, with the loop's own exit test at the
//           BOTTOM (rotated), so the guard survives as its own branch.
//   base    fn+0x1a4: `cmp esi,0x1200 / jge <exit>` - one SIGNED test
//           doing both jobs. VC6 had merged _Xran's `144 <= component`
//           into the top-tested `for`'s exit test and folded the throw
//           away entirely.
// 0x1200 is 144*32 on both sides: the guard is strength-reduced onto the
// akArtifactTraits byte offset that the body already needs.
// Spelling the loop `int component = 0; do { ... } while (++component <
// 144);` forces the bottom test, and the guard comes back with retail's
// register assignment and its jb.

// MEASURED BYTE-FLAT, do not re-spend: the same do/while on the OTHER
// three `component < 144` loops in this file (update_spell_list,
// equip_artifact, remove_artifact) changes nothing - VC6 already rotates
// those, so their guards were never folded. This lever only pays where
// the loop came out top-tested.
// The two component-capacity failures set a slotFits result and break the
// component scan; a false result continues the enclosing slot scan. This
// removes both next_slot jumps with the predicate unchanged at 92.9185%.
// Testing the exhausted component index instead loses 1.2018 points. This
// predicate is Complete-only; the retail capacity checks and one spared
// component establish the result, without inventing a source helper.
VA(0x004e2550, 0x2EC)  // retail-only, hero member, ret 8
unsigned char hero::heroFn004E2550(long artifact, long slot)
{
    if (g_game->m_f1f698 < 2 && slot == EQUIPPED_SLOT_SOD_MISC)
        return 0;

    long remaining = 1;
    if (slot == -1) {
        slot = 0;
        remaining = 19;
    }

    for (; remaining != 0; remaining--, slot++) {
        if (m_equipped[slot].m_artifactId != ARTIFACT_NONE)
            continue;
        if (!g_artifactSlotMasks[g_artifactTraits[artifact].m_allowableSlotMask]
                 .test(slot))
            continue;

        int slotClass = g_artifactSlotTraits[slot].m_type;
        unsigned int worn = m_artifactSlotCounts[slotClass];
        if (worn > 0) {
            std::bitset<19> classSlots = g_artifactSlotMasks[slotClass];
            size_t capacity = classSlots.count();
            if (worn >= capacity)
                continue;
            int occupied = 0;
            for (int i = 0; i < 19; i++) {
                if (classSlots.test(i) &&
                    m_equipped[i].m_artifactId != ARTIFACT_NONE)
                    occupied++;
            }
            if (worn >= capacity - occupied)
                continue;
        }

        int combination = g_artifactTraits[artifact].m_comboType;
        if (combination != -1) {
            int counts[15];
            const unsigned char* src = m_artifactSlotCounts;
            int* dst = counts;
            for (; src != m_artifactSlotCounts + 15; ++dst, ++src)
                *dst = *src;

            const std::bitset<144>& components =
                g_combinationArtifacts[combination].m_components;
            bool keptSlot = false;
            bool slotFits = true;
            int component = 0;
            do {
                if (!components.test(component))
                    continue;
                int componentClass =
                    g_artifactTraits[component].m_allowableSlotMask;
                if (componentClass
                        == g_artifactTraits[artifact].m_allowableSlotMask
                    && !keptSlot) {
                    keptSlot = true;
                    continue;
                }
                std::bitset<19> classSlots =
                    g_artifactSlotMasks[componentClass];
                size_t capacity = classSlots.count();
                if (counts[componentClass] >= capacity) {
                    slotFits = false;
                    break;
                }
                {
                    int occupied =
                        (g_artifactTraits[artifact].m_allowableSlotMask
                         == componentClass) ? 1 : 0;
                    for (int i = 0; i < 19; i++) {
                        if (classSlots.test(i) &&
                            m_equipped[i].m_artifactId != ARTIFACT_NONE)
                            occupied++;
                    }
                    if (counts[componentClass] >= capacity - occupied) {
                        slotFits = false;
                        break;
                    }
                }
                counts[componentClass]++;
            } while (++component < 144);
            if (!slotFits)
                continue;
        }
        return 1;
    }
    return 0;
}

// The wrapper: it validates the physical slot against the artifact's
// allowable-slot class, then hands the real work to HeroFn_004E2550 -
// but when the slot is already occupied it SAVES the displaced record,
// removes it, runs the attempt, and puts the displaced artifact back
// whatever the answer was. The restore is retail's, on BOTH paths: the
// image's EH metadata proves a `try`/`catch (...)` here. The FuncInfo
// this body's handler stub points at has nTryBlocks=1, tryLow=tryHigh=2,
// catchHigh=3 and a NULL type descriptor (catch-all), and the state
// stores bracket exactly one statement - `mov [ebp-4],2` at +0x166 just
// before the second HeroFn_004E2550 and `mov [ebp-4],-1` at +0x17b right
// after it. The catch funclet the HandlerType names (0x4e29dc, 25 B, no
// prologue, the parent's EBP frame) reads &displaced off [ebp-0x1c] and
// `this` off [ebp-0x14], calls equip_artifact, and rethrows with
// `push 0 / push 0 / call __CxxThrowException@8`. So the source is
// `try { accepted = HeroFn_004E2550(...); } catch (...) { restore;
// throw; }` with the normal-path restore repeated below - the same
// restore-on-unwind idiom artifact.cpp's va_end pair has.

// That try is ALSO what makes `.test(slot)` behave: VC6 will not expand
// a callee that introduces EH state into a caller with no EH frame, so
// with no catch scope here bitset<19>::_Xran stayed a CALL where retail
// expands its whole `_THROW(out_of_range, "invalid bitset<N> position")`
// body inline. That under-inline is what pinned this row at 44.93 for a
// week and what a hand-spelled range guard plus an explicit throw bought
// back structurally at 78.86 (2026-08-21). With the real try/catch in
// place the natural `allowable.test(slot)` scores IDENTICALLY to that
// hand-spelled guard - 85.7554 either way, same call multiset, same
// blocks - so the invented union-of-pointers bitset view is retired and
// the plain accessor stands. 78.8633 -> 85.7554 on the try/catch alone.

// The remaining 25 bytes were a CARVE boundary: 0x4e29dc's funclet was
// its own carve row, so our emitted body carried it and retail's target
// symbol stopped short. Absorbing it (412 -> 437, the LoadFontData
// precedent) took the row to 94.5203.

// Residual (94.5%): one block, and one call. Retail leaves
// basic_string::_Eos a CALL inside the inlined _Xran throw path where we
// expand it (+0x98 target-only); everything else - prologue, 0x3c frame,
// both EH homes, four branches, three rets, the bit test and all three
// tails - is byte-identical. That is an OVER-inline of a five-line
// private member at the same depth as the `_Grow` both sides call, so it
// is a budget quotient, not a spelling.
// Tried and rejected before the try/catch landed, one compile each:
// routing the gate through DC's artifact.h inline `artifactAllowedInSlot`
// (44.93, byte-flat); eight byte-inert dead assignments to grow the
// caller (44.93, byte-flat); `#pragma inline_depth(0)` on the four call
// sites after the .test() (44.93, byte-flat - the /Ob2 divisor counts
// call SITES whether or not they are pinned, so site pins cannot buy an
// under-inline back); a shallower bitset accessor (none exists - `at`
// checks twice); explicit guard plus the original `test` (57.60); a
// one-use inline range helper (44.38); a one-use string-return helper
// (56.58); local unused type-count probes at 1, 2 and 8 (44.93,
// byte-flat); an empty-destructor carrier (55.53); a contradictory-throw
// carrier (58.29) and the same with a string local (53.21). Those all
// measured a caller with no catch scope and none of them is evidence
// about this body now.
// Re-measured WITH the catch scope, since a rejected knob is only
// rejected for the inline structure it was measured in: the depth ladder
// (`allowable[slot]`, which reaches test through operator[]) is now
// BYTE-FLAT at 94.5203 - with an EH frame present it keeps
// _Xran inline, so the ladder has nothing left to trade here. The
// _Eos direction is a confirmed OVER-inline (base 0 calls vs retail 1),
// whose doctrinal lever is caller-shrink, and a 437-byte body with no
// liftable block and no DC-named helper has no dose to give.
VA(0x004e2840, 0x1B5)  // retail-only, hero member, ret 8; size absorbs the
unsigned char hero::heroFn004E2840(long artifact, long slot)
{
    const std::bitset<19>& allowable =
        g_artifactSlotMasks[g_artifactTraits[artifact].m_allowableSlotMask];
    if (!allowable.test(slot))
        return 0;

    if (m_equipped[slot].m_artifactId == ARTIFACT_NONE)
        return heroFn004E2550(artifact, slot);

    type_artifact displaced = m_equipped[slot];
    removeArtifact(slot);
    unsigned char accepted;
    try {
        accepted = heroFn004E2550(artifact, slot);
    } catch (...) {
        equipArtifact(&displaced, slot);
        throw;
    }
    equipArtifact(&displaced, slot);
    return accepted;
}

VA(0x004e2a00, 0x1C7)  // dc 0xd39d8
unsigned char hero::equipArtifact(const type_artifact* artifact, long slot)
{
    if (slot == -1) {
        slot = 0;
        while (1) {
            if (slot >= 19)
                return 0;
            if (heroFn004E2550(artifact->m_artifactId, slot))
                break;
            ++slot;
        }
    } else {
        if (!heroFn004E2550(artifact->m_artifactId, slot))
            return 0;
    }

    m_equipped[slot].m_artifactId = artifact->m_artifactId;
    m_equipped[slot].m_extra = artifact->m_extra;

    if (artifact->m_artifactId == ARTIFACT_TITANS_THUNDER
        && m_equipped[17].m_artifactId == ARTIFACT_NONE) {
        type_artifact spellbook(ARTIFACT_SPELLBOOK);
        equipArtifact(&spellbook, 17);
    }

    bool updateSpells = false;
    int combinationIndex =
        g_artifactTraits[artifact->m_artifactId].m_comboType;
    if (combinationIndex != -1) {
        const std::bitset<144>& components =
            g_combinationArtifacts[combinationIndex].m_components;
        bool keptSlot = false;
        for (int component = 0; component < 144; component++) {
            if (components.test(component)) {
                for (int skill = 0; skill < 4; skill++)
                    m_stats[skill] +=
                        g_artifactPrimarySkillBonuses[component][skill];
                updateSpells = updateSpells
                    || g_artifactTraits[component].m_givesSpells;
                int componentSlot =
                    g_artifactTraits[component].m_allowableSlotMask;
                if (componentSlot
                        == g_artifactTraits[artifact->m_artifactId]
                               .m_allowableSlotMask
                    && !keptSlot)
                    keptSlot = true;
                else
                    m_artifactSlotCounts[componentSlot]++;
            }
        }
    }

    for (int skill = 0; skill < 4; skill++)
        m_stats[skill] +=
            g_artifactPrimarySkillBonuses[artifact->m_artifactId][skill];

    if (updateSpells
        || g_artifactTraits[artifact->m_artifactId].m_givesSpells)
        updateSpellList();
    return 1;
}

// E:\gamedcs\hero.cpp:4919
// Dismantling a combination artifact removes every component's four primary
// skill bonuses, maintains the per-slot counts (preserving the one component
// that occupies the assembled artifact's slot), then removes the assembled
// artifact's own bonuses. The spell list is rebuilt if either the assembled
// artifact or any component affects it.

// Residual (90.2%): register-homing only. Retail binds the artifact id to
// ESI as its first call-crossing pseudo and saves ESI in the prologue; our
// CL binds `this` there and sinks the push. `why-reg --model` reports the
// definition slots and ORDER agree on both sides with only the ebx/esi
// bindings permuted, and that the value retail puts in ESI would have to
// be created before `this` - which is minted between the parameters and
// the body locals, so no declaration, include or spelling change can
// precede it. That is C2-side handle STATE (catalog C1), not source-
// reachable; its one model-passing candidate measured +16 (worse).
VA(0x004e2bd0, 0x174)  // anchor-bracket, dc 0xd3ad0
void hero::removeArtifact(long slot)
{
    type_artifact artifact = m_equipped[slot];
    if (artifact.m_artifactId == ARTIFACT_NONE)
        return;

    bool updateSpells = false;
    int combinationIndex =
        g_artifactTraits[artifact.m_artifactId].m_comboType;
    if (combinationIndex != -1) {
        const std::bitset<144>& components =
            g_combinationArtifacts[combinationIndex].m_components;
        bool keptSlot = false;
        for (int component = 0; component < 144; component++) {
            if (components.test(component)) {
                for (int skill = 0; skill < 4; skill++)
                    m_stats[skill] -=
                        g_artifactPrimarySkillBonuses[component][skill];
                updateSpells = updateSpells
                    || g_artifactTraits[component].m_givesSpells;
                int componentSlot =
                    g_artifactTraits[component].m_allowableSlotMask;
                if (componentSlot
                        == g_artifactTraits[artifact.m_artifactId].m_allowableSlotMask
                    && !keptSlot)
                    keptSlot = true;
                else
                    m_artifactSlotCounts[componentSlot]--;
            }
        }
    }

    m_equipped[slot].m_artifactId = ARTIFACT_NONE;
    m_equipped[slot].m_extra = -1;
    for (int skill = 0; skill < 4; skill++)
        m_stats[skill] -= g_artifactPrimarySkillBonuses[artifact.m_artifactId][skill];
    if (updateSpells
        || g_artifactTraits[artifact.m_artifactId].m_givesSpells)
        updateSpellList();
}

VA(0x004e2d50, 0x7E)  // dc 0xd3b74
void hero::removeBackpackArtifact(short slot)
{
    if (m_backpack[slot].m_artifactId == -1)
        return;
    long last = getLastBackpackIndex();
    while (slot < last) {
        m_backpack[slot] = m_backpack[slot + 1];
        slot++;
    }
    m_backpack[slot].m_artifactId = ARTIFACT_NONE;
    m_backpackCount--;
}

VA(0x004e2dd0, 0xFC)  // dc 0xd3bec
unsigned char hero::removeArtifact(TArtifact artifact)
{
    if (artifact == ARTIFACT_SPELL_SCROLL)
        return 0;
    long last = getLastBackpackIndex();
    short slot;
    for (slot = 0; slot <= last; slot++) {
        if (m_backpack[slot].m_artifactId == artifact) {
            removeBackpackArtifact(slot);
            return 1;
        }
    }
    for (slot = 0; slot < 19; slot++) {
        if (m_equipped[slot].m_artifactId == artifact) {
            removeArtifact(slot);
            return 1;
        }
    }
    return 0;
}

VA(0x004e2ed0, 0xB2)  // dc 0xd3c64
std::string hero::getBackpackError(TArtifact artifact) const
{
    if (m_backpackCount >= 64) {
        return std::string(g_generalText->getText(GENERAL_TEXT_BACKPACK_FULL));
    }
    return formatString(g_generalText->getText(GENERAL_TEXT_BACKPACK_ARTIFACT_FORMAT),
                         g_artifactTraits[artifact].m_name);
}

VA(0x004e2f90, 0xD1)  // dc 0xd3cfc
unsigned char hero::addToBackpack(const type_artifact* artifact, long slot)
{
    if (m_backpackCount >= 64)
        return 0;
    if (artifact->m_artifactId == ARTIFACT_CATAPULT ||
        artifact->m_artifactId == ARTIFACT_BALLISTA ||
        artifact->m_artifactId == ARTIFACT_AMMO_CART ||
        artifact->m_artifactId == ARTIFACT_FIRST_AID_TENT)
        return 0;
    if (slot < 0) {
        for (slot = 0; slot < 64; slot++) {
            if (m_backpack[slot].m_artifactId == -1)
                break;
        }
    }
    if (m_backpack[slot].m_artifactId != -1) {
        long last = getLastBackpackIndex();
        for (long i = last; i >= slot; i--)
            m_backpack[i + 1] = m_backpack[i];
    }
    m_backpack[slot] = *artifact;
    m_backpackCount++;
    return 1;
}

// E:\gamedcs\hero.cpp:5044

// The `else if` shape is exact - the owner-is-not-the-local-player
// branch and the still-holding-the-combination branch BOTH fall into the
// `!player.isHuman` arm, and a zero `bAnnounce` skips both arms while
// still reaching the assembledCombinations store. `player` is
// materialised BEFORE the `bAnnounce` test (retail leas it first); the
// second half re-derives the same address inline, so it is spelled out
// rather than reusing the reference. `owner >= 0 && owner < 8` is
// written twice because retail tests it twice, 8-bit both times.
// `prompt` must be a NAMED local: its _Tidy runs AFTER the dialogReturn
// block, not at the end of the NormalDialog full-expression.

// Residual (MAX 76.3360%, rechecked 2026-09-07): retail keeps the first
// bitset<12>::_Xran out of line and expands the two later bounds failures.
// The candidate expands the first throw too, adding an EH state and growing
// the frame from 0x64 to 0x78. The retained bitset<144> set/any pins reproduce
// those two retail calls; removing both expands them (64.9514%).
// Controls on the current TU: reading through a const bitset<12> subscript,
// either a cast or a named reference, is byte-identical at 76.3360%; direct
// test() is 72.2875%. Removing both pins with the const read gives 64.5870%.
// Neither constness nor removing the pins restores the first _Xran boundary.
// The first write uses the bitset proxy assignment (old control 75.1012 ->
// 76.3360); the later write stays set(). Earlier site pins on the read also
// de-inlined its parent accessor and GetLocalPlayerGamePos, losing ground.
// Earlier discarded release-elided call carriers changed several exception
// expansions together and supplied no proof of a missing source invariant.
// DC's older GiveArtifact has no combination-assembly path; it proves only
// the equipment/backpack and end-check helper boundaries here. The remaining
// per-site inlining decision needs positive Complete/VC6 evidence.
VA(0x004e3070, 0x339)  // anchor-global, dc 0xd3de4
unsigned char hero::giveArtifact(const type_artifact* artifact,
                                 unsigned char announce,
                                 unsigned char checkEnd)
{
    if (equipArtifact(artifact, -1)) {
        if (g_game->m_f1f698 >= 2) {
            int targetCombo =
                g_artifactTraits[artifact->m_artifactId].m_targetCombo;
            if (targetCombo != -1 && m_owner >= 0 && m_owner < 8) {
                std::bitset<144> missing =
                    g_combinationArtifacts[targetCombo].m_components;
                for (int i = 0; i < 19; i++) {
                    int artifactId = m_equipped[i].m_artifactId;
                    if (artifactId != ARTIFACT_NONE)
#pragma inline_depth(0)
                        missing.set(artifactId, false);
#pragma inline_depth()
                }
                if (!missing.any()) {
                    playerData& player = g_game->m_players[m_owner];
                    if (announce) {
                        if (m_owner == g_game->getLocalPlayerGamePos() &&
                            !player.m_assembledCombinations[targetCombo]) {
                            int assembled =
                                g_combinationArtifacts[targetCombo].m_artifactId;
                            std::string prompt = formatString(
                                g_generalText->getText(733),
                                g_artifactTraits[assembled].m_name);
                            normalDialog(prompt.c_str(), 2, -1, -1, 8,
                                         assembled, -1, 0, -1, 0, -1, 0);
                            if (g_windowManager->m_dialogReturn ==
                                DIALOG_RETURN_ACCEPT)
                                heroFn004DBF30(targetCombo, -1);
                        } else if (!player.m_isHuman) {
                            heroFn004DBF30(targetCombo, -1);
                        }
                    }
                    player.m_assembledCombinations[targetCombo] = true;
                }
            }
        }
    } else if (!addToBackpack(artifact, -1)) {
        return 0;
    }

    int comboType = g_artifactTraits[artifact->m_artifactId].m_comboType;
    if (comboType != -1 && m_owner >= 0 && m_owner < 8)
        g_game->m_players[m_owner].m_assembledCombinations.set(comboType);

    if (checkEnd &&
        g_game->m_mapHeader.m_victoryCondition.checkForArtifactWin())
        checkEndGame(0);
    return 1;
}

#if 0  // @carcass

// E:\gamedcs\hero.cpp:5064
DC_ONLY(0xd3e40, 0x46)
int hero::GiveRandomArtifact()
{
    // @stub
}

#endif  // @carcass

VA(0x004e33b0, 0x24A)  // dc 0xd3e88
int hero::giveExperience(int howMuch, int checkForLevelUp,
                         unsigned char showCapWindow)
{
    int entryLevel = m_level;
    if (g_game->m_mapHeader.m_maxHeroLevel > 0) {
        int cap = getExperience(g_game->m_mapHeader.m_maxHeroLevel);
        if (m_experience + howMuch > cap) {
            if (m_experience < cap) {
                m_experience = cap;
                if (checkForLevelUp)
                    checkLevel();
            }
            if (m_experience > cap)
                m_experience = cap;
            if (showCapWindow && g_game->isLocalHuman(m_owner)) {
                std::string text =
                    formatString(g_generalText->getText(2), m_name);
                normalDialog(text.c_str(), 1, -1, -1, 0x11, 0, -1, 0, -1,
                             0, -1, 0);
            }
            return 0;
        }
    }

    m_experience += howMuch;
    int newLevel = getLevel(m_experience);
    if (checkForLevelUp)
        checkLevel();
    return newLevel - entryLevel;
}

VA(0x004e3600, 0xB2)  // dc 0xd3fb8
void hero::giveResource(int whichRes, int howMuch)
{
    if (whichRes >= 0 && whichRes <= NUM_RESOURCES - 1) {
        g_game->m_players[m_owner].m_resources[whichRes] += howMuch;
        if (g_game->m_players[m_owner].m_resources[whichRes] < 0)
            g_game->m_players[m_owner].m_resources[whichRes] = 0;
    }

    if (&g_game->m_players[m_owner] == g_currentPlayer
        && g_advManager->m_status == baseManager::STATUS_ACTIVE)
        g_advManager->m_advWindow->updateResourceDisplay(1, 1);

    g_game->isHuman(m_owner);
}

VA(0x004e36c0, 0x2E8)  // dc 0xd4070
int hero::getLuck(const hero* otherHero, unsigned char onCursedGround,
                  unsigned char applyLimits) const
{
    if (!(m_flags & 0x400000)) {
        if (onCursedGround)
            return 0;
        if (isWieldingArtifact(ARTIFACT_HOURGLASS_OF_THE_EVIL_HOUR) ||
            (otherHero &&
             otherHero->isWieldingArtifact(
                 ARTIFACT_HOURGLASS_OF_THE_EVIL_HOUR)))
            return 0;
    }

    int luck = g_luckBonuses[m_skillLevel[eSecSkillLuck]];
    if (m_skillLevel[eSecSkillLuck] > 0) {
        const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];
        if (ability.m_type == eHeroAbilitySecondarySkill &&
            ability.m_skill == eSecSkillLuck)
            luck = static_cast<long>((m_level * 0.05f + 1.0f) * luck);
    }

    if (isWieldingArtifact(0x6c))
        luck += 3;
    if (isWieldingArtifact(0x2d))
        luck++;
    if (isWieldingArtifact(0x2e))
        luck++;
    if (isWieldingArtifact(0x2f))
        luck++;
    if (isWieldingArtifact(0x30))
        luck++;

    if (m_owner >= 0) {
        playerData& player = g_game->m_players[m_owner];
        for (int i = 0; i < player.m_numTowns; i++) {
            town* ownedTown = g_game->getTown(player.m_townIds[i]);
            if (ownedTown->hasBuilding(HOLY_GRAIL_ID, 1) &&
                ownedTown->m_type == TOWN_RAMPART) {
                luck += 2;
                break;
            }
        }
    }

    luck += m_luckBonus;
    if (m_flags & 0x400000)
        luck += 500;
    return applyLimits ? limit(-3, luck, 3) : luck;
}

VA(0x004e39b0, 0x2A9)  // dc 0xd41fc
int hero::getMorale(const hero* otherHero, unsigned char onCursedGround,
                    unsigned char applyLimits) const
{
    int morale;

    if (onCursedGround)
        return 0;

    morale = g_leadershipBonuses[m_skillLevel[eSecSkillLeadership]];
    if (m_skillLevel[eSecSkillLeadership] > 0) {
        const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];
        if (ability.m_type == eHeroAbilitySecondarySkill &&
            ability.m_skill == eSecSkillLeadership)
            morale = static_cast<long>((m_level * 0.05f + 1.0f) * morale);
    }

    if (isWieldingArtifact(0x6c))
        morale += 3;
    if (isWieldingArtifact(0x2d))
        morale++;
    if (isWieldingArtifact(0x31))
        morale++;
    if (isWieldingArtifact(0x32))
        morale++;
    if (isWieldingArtifact(0x33))
        morale++;

    if (m_owner >= 0) {
        playerData& player = g_game->m_players[m_owner];
        for (int i = 0; i < player.m_numTowns; i++) {
            town* ownedTown = g_game->getTown(player.m_townIds[i]);
            if (ownedTown->hasBuilding(HOLY_GRAIL_ID, 1) &&
                ownedTown->m_type == TOWN_CASTLE) {
                morale += 2;
                break;
            }
        }
    }

    morale += m_moraleBonus;
    if (m_flags & 0x800000)
        morale += 500;
    return applyLimits ? limit(-3, morale, 3) : morale;
}

VA(0x004e3c60, 0x70)
TCreatureType hero::getNecromancyCreature()
{
    if (isWieldingArtifact(ARTIFACT_CLOAK_OF_THE_UNDEAD_KING)) {
        if (m_skillLevel[eSecSkillNecromancy] >= 3)
            return CREATURE_LICH;
        if (m_skillLevel[eSecSkillNecromancy] >= 2)
            return CREATURE_WIGHT;
        if (m_skillLevel[eSecSkillNecromancy] >= 1)
            return CREATURE_WALKING_DEAD;
    }
    return CREATURE_SKELETON;
}

VA(0x004e3cd0, 0x268)  // dc 0xd4390
float hero::getNecromancyFactor(unsigned char applyLimit) const
{
    float factor = g_necromancyFactors[m_skillLevel[eSecSkillNecromancy]];
    if (m_skillLevel[eSecSkillNecromancy] > 0) {
        const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];
        if (ability.m_type == eHeroAbilitySecondarySkill &&
            ability.m_skill == eSecSkillNecromancy)
            factor = (m_level * 0.05f + 1.0f) * factor;

        if (isWieldingArtifact(0x36))
            factor += 0.05f;
        if (isWieldingArtifact(0x37))
            factor += 0.1f;
        if (isWieldingArtifact(0x38))
            factor += 0.15f;

        if (m_owner >= 0) {
            playerData& player = g_game->m_players[m_owner];
            for (int i = 0; i < player.m_numTowns; i++) {
                town* ownedTown = g_game->getTown(player.m_townIds[i]);
                if (ownedTown->m_type == TOWN_NECROPOLIS) {
                    if ((ownedTown->m_active & g_bitNumber[EXTRA_0_ID]) != 0)
                        factor += 0.1f;
                    if ((ownedTown->m_active & g_bitNumber[HOLY_GRAIL_ID]) != 0)
                        factor += 0.2f;
                }
            }
        }
    } else if (isWieldingArtifact(0x82)) {
        factor += 0.3f;
    }

    if (applyLimit && factor > 1.0f)
        factor = 1.0f;
    return factor;
}

VA(0x004e3f40, 0x12F)  // dc 0xd44a4
int hero::getMysticismBonus() const
{
    int bonus = g_mysticismBonuses[m_skillLevel[eSecSkillMysticism]];
    if (m_skillLevel[eSecSkillMysticism] > 0) {
        const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];
        if (ability.m_type == eHeroAbilitySecondarySkill && ability.m_skill == eSecSkillMysticism) {
            bonus = (m_level * 0.05f + 1.0f) * bonus;
            bonus++;
        }
    }
    if (isWieldingArtifact(ARTIFACT_CHARM_OF_MANA))
        bonus++;
    if (isWieldingArtifact(ARTIFACT_TALISMAN_OF_MANA))
        bonus += 2;
    if (isWieldingArtifact(ARTIFACT_MYSTIC_ORB_OF_MANA))
        bonus += 3;
    return bonus;
}

VA(0x004e4070, 0xEF)  // dc 0xd4560
int hero::getVisibility() const
{
    int visibility = g_scoutingVisibility[m_skillLevel[eSecSkillScouting]];
    if (m_skillLevel[eSecSkillScouting] > 0) {
        const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];
        if (ability.m_type == eHeroAbilitySecondarySkill && ability.m_skill == eSecSkillScouting)
            visibility = (m_level * 0.05f + 1.0f) * visibility;
    }
    if (isWieldingArtifact(ARTIFACT_SPECULUM))
        visibility++;
    if (isWieldingArtifact(ARTIFACT_SPYGLASS))
        visibility++;
    return visibility;
}

VA(0x004e4160, 0x143)  // dc 0xd45d8
float hero::getArcheryFactor() const
{
    float factor = g_archeryFactors[m_skillLevel[eSecSkillArchery]];
    if (m_skillLevel[eSecSkillArchery] > 0) {
        const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];
        if (ability.m_type == eHeroAbilitySecondarySkill && ability.m_skill == eSecSkillArchery)
            factor = (m_level * 0.05f + 1.0f) * factor;
        if (isWieldingArtifact(ARTIFACT_BOW_OF_ELVEN_CHERRYWOOD))
            factor = factor + 0.05f;
        if (isWieldingArtifact(ARTIFACT_BOWSTRING_OF_THE_UNICORNS_MANE))
            factor = factor + 0.1f;
        if (isWieldingArtifact(ARTIFACT_ANGEL_FEATHER_ARROWS))
            factor = factor + 0.15f;
    }
    return factor;
}

VA(0x004e42b0, 0x60)  // dc 0xd4664
float hero::getOffenseFactor() const
{
    float factor = g_offenseFactors[m_skillLevel[eSecSkillOffense]];
    if (m_skillLevel[eSecSkillOffense] > 0) {
        const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];
        if (ability.m_type == eHeroAbilitySecondarySkill && ability.m_skill == eSecSkillOffense)
            factor = (m_level * 0.05f + 1.0f) * factor;
    }
    return factor;
}

VA(0x004e4310, 0x7D)  // dc 0xd46a8
float hero::getDefenseFactor() const
{
    float factor = g_defenseFactors[m_skillLevel[eSecSkillDefense]];
    if (m_skillLevel[eSecSkillDefense] > 0) {
        const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];
        if (ability.m_type == eHeroAbilitySecondarySkill && ability.m_skill == eSecSkillDefense)
            factor = (m_level * 0.05f + 1.0f) * factor;
    }
    if (factor > 1.0f)
        factor = 1.0f;
    return 1.0f - factor;
}

VA(0x004e4390, 0x89)  // dc 0xd46f8
int hero::getEstatesBonus() const
{
    int bonus = g_estatesGold[m_skillLevel[eSecSkillEstates]];
    if (m_skillLevel[eSecSkillEstates] > 0) {
        const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];
        if (ability.m_type == eHeroAbilitySecondarySkill
            && ability.m_skill == eSecSkillEstates)
            bonus = static_cast<int>((m_level * 0.05f + 1.0f) * bonus);
    }
    const THeroSpecificAbility& resource = g_heroSpecificAbilities[m_id];
    if (resource.m_type == eHeroAbilityResource
        && static_cast<int>(resource.m_skill) == GOLD)
        bonus += 350;
    return bonus;
}

VA(0x004e4420, 0x15A)  // dc 0xd4768
float hero::getEagleEyeChance() const
{
    float factor = g_eagleEyeFactors[m_skillLevel[eSecSkillEagleEye]];
    if (m_skillLevel[eSecSkillEagleEye] > 0) {
        const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];
        if (ability.m_type == eHeroAbilitySecondarySkill && ability.m_skill == eSecSkillEagleEye)
            factor = (m_level * 0.05f + 1.0f) * factor;
        if (isWieldingArtifact(ARTIFACT_BIRD_OF_PERCEPTION))
            factor = factor + 0.05f;
        if (isWieldingArtifact(ARTIFACT_STOIC_WATCHMAN))
            factor = factor + 0.1f;
        if (isWieldingArtifact(ARTIFACT_EMBLEM_OF_COGNIZANCE))
            factor = factor + 0.15f;
    }
    if (factor > 1.0f)
        factor = 1.0f;
    return factor;
}

VA(0x004e4580, 0x15C)  // dc 0xd482c
float hero::getSurrenderCostFactor() const
{
    float factor = g_diplomacyFactors[m_skillLevel[eSecSkillDiplomacy]];
    if (m_skillLevel[eSecSkillDiplomacy] > 0) {
        const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];
        if (ability.m_type == eHeroAbilitySecondarySkill && ability.m_skill == eSecSkillDiplomacy)
            factor = (m_level * 0.05f + 1.0f) * factor;
    }
    if (isWieldingArtifact(ARTIFACT_STATESMANS_MEDAL))
        factor = factor + 0.1f;
    if (isWieldingArtifact(ARTIFACT_DIPLOMATS_RING))
        factor = factor + 0.1f;
    if (isWieldingArtifact(ARTIFACT_AMBASSADORS_SASH))
        factor = factor + 0.1f;
    if (factor > 0.9f)
        factor = 0.9f;
    return 1.0f - factor;
}

VA(0x004e46e0, 0x15C)  // dc 0xd48c8
float hero::getMagicResistanceFactor() const
{
    float factor = g_magicResistanceFactors[m_skillLevel[eSecSkillMagicResistance]];
    if (m_skillLevel[eSecSkillMagicResistance] > 0) {
        const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];
        if (ability.m_type == eHeroAbilitySecondarySkill && ability.m_skill == eSecSkillMagicResistance)
            factor = (m_level * 0.05f + 1.0f) * factor;
    }
    if (isWieldingArtifact(ARTIFACT_GARNITURE_OF_INTERFERENCE))
        factor = factor + 0.05f;
    if (isWieldingArtifact(ARTIFACT_SURCOAT_OF_COUNTERPOISE))
        factor = factor + 0.1f;
    if (isWieldingArtifact(ARTIFACT_BOOTS_OF_POLARITY))
        factor = factor + 0.15f;
    if (factor > 1.0f)
        factor = 1.0f;
    return 1.0f - factor;
}

VA(0x004e4840, 0x66)  // dc 0xd4960
float hero::getExperienceBonusFactor() const
{
    float factor = g_learningFactors[m_skillLevel[eSecSkillLearning]];
    if (m_skillLevel[eSecSkillLearning] > 0) {
        const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];
        if (ability.m_type == eHeroAbilitySecondarySkill && ability.m_skill == eSecSkillLearning)
            factor = (m_level * 0.05f + 1.0f) * factor;
    }
    return factor + 1.0f;
}

// Logistics' land-movement factor by mastery (retail .rdata 0x63ea68,
// the same four-float band as the specialty rows above).
static const float g_logisticsFactors[kNumMasteries] =
    { 0.0f, 0.1f, 0.2f, 0.3f };

// E:\gamedcs\hero.cpp:5709.
DC_ONLY(0xd49a8, 0x48)
float hero::getLogisticsFactor() const
{
    float factor = g_logisticsFactors[m_skillLevel[eSecSkillLogistics]];
    if (m_skillLevel[eSecSkillLogistics] > 0) {
        const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];
        if (ability.m_type == eHeroAbilitySecondarySkill && ability.m_skill == eSecSkillLogistics)
            factor = (m_level * 0.05f + 1.0f) * factor;
    }
    return factor + 1.0f;
}

// E:\gamedcs\hero.cpp:5734.
DC_ONLY(0xd49f0, 0x4E)
long hero::getNavigationFactor() const
{
    long movement = g_seaMovement[m_skillLevel[eSecSkillNavigation]];
    if (m_skillLevel[eSecSkillNavigation] > 0) {
        const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];
        if (ability.m_type == eHeroAbilitySecondarySkill && ability.m_skill == eSecSkillNavigation)
            movement += m_level * g_seaMovement[0] / 20;
    }
    return movement;
}

#if 0  // @carcass

// E:\gamedcs\hero.cpp:5758
DC_ONLY(0xd4a40, 0x48)
float hero::GetSorceryFactor()
{
    // @stub
}

#endif  // @carcass

VA(0x004e48b0, 0x66)  // dc 0xd4a88
float hero::getIntelligenceFactor() const
{
    float factor = g_intelligenceFactors[m_skillLevel[eSecSkillIntelligence]];
    if (m_skillLevel[eSecSkillIntelligence] > 0) {
        const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];
        if (ability.m_type == eHeroAbilitySecondarySkill && ability.m_skill == eSecSkillIntelligence)
            factor = (m_level * 0.05f + 1.0f) * factor;
    }
    return factor + 1.0f;
}

VA(0x004e4920, 0x66)  // dc 0xd4b08
float hero::getFirstAidFactor() const
{
    float factor = g_firstAidFactors[m_skillLevel[eSecSkillFirstAid]];
    if (m_skillLevel[eSecSkillFirstAid] > 0) {
        const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];
        if (ability.m_type == eHeroAbilitySecondarySkill && ability.m_skill == eSecSkillFirstAid)
            factor = (m_level * 0.05f + 1.0f) * factor;
    }
    return factor + 1.0f;
}

VA(0x004e4990, 0x3F6)  // dc 0xd4b50
int hero::getMobility(unsigned char seaMovement) const
{
    if (m_flags & 0x1000000)
        return 1000000;

    int mobility;
    if (seaMovement) {
        mobility = getNavigationFactor();

        if (m_owner != -1)
            mobility += g_game->mineTypesOwned(m_owner, 100) *
                        g_lighthouseMovementBonus;

        if (isWieldingArtifact(0x7b))
            mobility += g_lighthouseMovementBonus;

        for (unsigned int t = 0; t < g_game->m_towns.size(); t++) {
            if (g_game->m_towns[t].m_type == TOWN_CASTLE &&
                g_game->m_towns[t].hasBuilding(SPECIAL_BUILDING_ID, 0))
                mobility += g_lighthouseMovementBonus;
        }

        if (isWieldingArtifact(0x47))
            mobility += g_oceanGuidanceMovementBonus;
    } else {
        int slowest = 20;
        for (int slot = 0; slot < 7; slot++) {
            int creature = m_army.m_armies[slot];
            if (creature != CREATURE_NONE) {
                int speed = g_creatureTypeTraits[creature].m_speed;
                if (g_heroSpecificAbilities[m_id].m_type == eHeroAbilityCreature) {
                    if (creature == g_heroSpecificAbilities[m_id].m_creature)
                        speed++;
                    else if (g_heroSpecificAbilities[m_id].m_creature !=
                                 CREATURE_BALLISTA &&
                             creature == g_game->upgradedCreatureType(
                                 g_heroSpecificAbilities[m_id].m_creature))
                        speed++;
                }
                if (speed < slowest)
                    slowest = speed;
            }
        }

        mobility = g_landMovement[slowest];
        mobility = static_cast<int>(mobility * getLogisticsFactor());

        if (isWieldingArtifact(0x62))
            mobility += g_bootsOfSpeedMovementBonus;
        if (isWieldingArtifact(0x46))
            mobility += g_equestriansGlovesMovementBonus;
        if (m_flags & 2)
            mobility += g_stablesMovementBonus;
    }

    if (m_owner >= 0 && m_owner < 6 && !g_game->isHuman(m_owner) &&
        g_game->m_setup.m_difficulty > 2) {
        mobility += 75;
        if (g_game->m_players[m_owner].m_personality == AI_PERSONALITY_AGGRESSIVE)
            mobility += 50;
    }
    return mobility;
}

VA(0x004e4d90, 0x12)  // dc 0xd4d60
int hero::getMobility() const
{
    return getMobility((m_flags >> 18) & 1);
}

VA(0x004e4db0, 0x10D)  // dc 0xd4db0
int hero::getSpellDurationBonus() const
{
    int bonus = 0;
    if (isWieldingArtifact(ARTIFACT_COLLAR_OF_CONJURING))
        bonus++;
    if (isWieldingArtifact(ARTIFACT_RING_OF_CONJURING))
        bonus += 2;
    if (isWieldingArtifact(ARTIFACT_CAPE_OF_CONJURING))
        bonus += 3;
    if (isWieldingArtifact(ARTIFACT_RING_OF_THE_MAGI))
        bonus += 50;
    return bonus;
}

VA(0x004e4ec0, 0xD6)
TAdventureObjectType hero::heroFn004E4EC0()
{
    type_point point;
    point.m_x = m_x;
    point.m_y = m_y;
    point.m_z = m_z;

    type_point invalid;
    invalid.m_x = -1;
    invalid.m_y = -1;
    invalid.m_z = -1;

    if (invalid.m_x == point.m_x && invalid.m_y == point.m_y
        && invalid.m_z == point.m_z)
        return NOTHING;

    const NewmapCell* cell = g_game->m_worldMap.cell(point.m_x, point.m_y, point.m_z);
    return cell->getSpecialTerrain();
}

VA(0x004e4fa0, 0xD7)  // dc 0xd4df0
inline int hero::getSpecialTerrain() const
{
    type_point location = getLocation();
    if (location == type_point(-1, -1, -1))
        return kMagicTerrainNone;
    NewmapCell* cell = g_game->getCell(location);
    return cell->getMagicTerrainType();
}

VA(0x004e5080, 0x7D)  // dc 0xd4e4c
TSkillMastery hero::getSpellLevel(SpellID spell, int magicTerrain) const
{
    if (spell == SPELL_ARMAGEDDON
        && isWieldingArtifact(ARTIFACT_ARMAGEDDONS_BLADE))
        return eMasteryExpert;
    return getSpellSchoolLevel(g_spellTraits[spell].m_school, magicTerrain);
}

VA(0x004e5100, 0xBC)  // dc 0xd4e68
TSkillMastery hero::getSpellSchoolLevel(TSpellSchool schoolMask,
                                        int magicTerrain) const
{
    TSpellSchool terrainSchool = const_invalid_school;
    switch (magicTerrain) {
    case kMagicTerrainMagicPlains:
        terrainSchool = eSchoolAll;
        break;
    case kMagicTerrainLucidPools:
        terrainSchool = eSchoolWater;
        break;
    case kMagicTerrainFieryFields:
        terrainSchool = eSchoolFire;
        break;
    case kMagicTerrainRocklands:
        terrainSchool = eSchoolEarth;
        break;
    case kMagicTerrainMagicClouds:
        terrainSchool = eSchoolAir;
        break;
    }
    if (schoolMask & terrainSchool)
        return eMasteryExpert;
    TSkillMastery level = eMasteryNone;
    if (schoolMask & eSchoolAir) {
        if (m_skillLevel[eSecSkillSchoolOfAirMagic] > level)
            level = TSkillMastery(m_skillLevel[eSecSkillSchoolOfAirMagic]);
    }
    if (schoolMask & eSchoolFire) {
        if (m_skillLevel[eSecSkillSchoolOfFireMagic] > level)
            level = TSkillMastery(m_skillLevel[eSecSkillSchoolOfFireMagic]);
    }
    if (schoolMask & eSchoolEarth) {
        if (m_skillLevel[eSecSkillSchoolOfEarthMagic] > level)
            level = TSkillMastery(m_skillLevel[eSecSkillSchoolOfEarthMagic]);
    }
    if (schoolMask & eSchoolWater) {
        if (m_skillLevel[eSecSkillSchoolOfWaterMagic] > level)
            level = TSkillMastery(m_skillLevel[eSecSkillSchoolOfWaterMagic]);
    }
    return level;
}

// E:\gamedcs\hero.cpp:6025
VA(0x004e51c0, 0x73)  // anchor-global, dc 0xd4ed0
TSpellSchool hero::getHighestSchool(TSpellSchool schoolMask) const
{
    TSpellSchool bestSchool;
    int bestLevel = -1;
    if ((schoolMask & eSchoolAir)
        && m_skillLevel[eSecSkillSchoolOfAirMagic] > bestLevel) {
        bestLevel = m_skillLevel[eSecSkillSchoolOfAirMagic];
        bestSchool = eSchoolAir;
    } else {
        bestSchool = schoolMask;
    }
    if ((schoolMask & eSchoolFire)
        && m_skillLevel[eSecSkillSchoolOfFireMagic] > bestLevel) {
        bestLevel = m_skillLevel[eSecSkillSchoolOfFireMagic];
        bestSchool = eSchoolFire;
    }
    if ((schoolMask & eSchoolEarth)
        && m_skillLevel[eSecSkillSchoolOfEarthMagic] > bestLevel) {
        bestLevel = m_skillLevel[eSecSkillSchoolOfEarthMagic];
        bestSchool = eSchoolEarth;
    }
    if ((schoolMask & eSchoolWater)
        && m_skillLevel[eSecSkillSchoolOfWaterMagic] > bestLevel) {
        bestLevel = m_skillLevel[eSecSkillSchoolOfWaterMagic];
        bestSchool = eSchoolWater;
    }
    return bestSchool;
}

// Complete adds the Armageddon's Blade mastery override before indexing the mana row.
VA(0x004e5240, 0xEF)  // dc 0xd4f64
int hero::getManaCost(int whichSpell, const armyGroup* enemy,
    int magicTerrain) const
{
    if (whichSpell == SPELL_TITANS_LIGHTNING_BOLT)
        return 0;
    TSkillMastery mastery = this->getSpellLevel(whichSpell, magicTerrain);
    int cost = g_spellTraits[whichSpell].m_manaCost[mastery];
    if (enemy) {
        if (enemy->isMember(CREATURE_PEGASUS)
            || enemy->isMember(CREATURE_SILVER_PEGASUS))
            cost += 2;
        if (hasArmy(CREATURE_MAGE) || hasArmy(CREATURE_ARCH_MAGE))
            cost -= 2;
    }
    if (cost < 1)
        cost = 1;
    return cost;
}

VA(0x004e5330, 0x43)  // dc 0xd4fe0
int hero::getMobilityFrame() const
{
    int frame;
    if (m_movePoints <= 0)
        frame = 0;
    else if (m_movePoints < 2300)
        frame = m_movePoints / 100;
    else if (m_movePoints < 2500)
        frame = 23;
    else
        frame = 24 + (m_movePoints >= 2800);
    return frame;
}

VA(0x004e5380, 0x3E)  // dc 0xd5024
int hero::getManaFrame() const
{
    short currentMana = m_mana;
    int frame;
    if (currentMana < 116)
        frame = (currentMana + 4) / 5;
    else if (currentMana < 145)
        frame = 23;
    else
        frame = 24 + (currentMana >= 170);
    return frame;
}

VA(0x004e53c0, 0x1E)  // dc 0xd5060
bool hero::visitedArena(const NewmapCell* cell) const
{
    return (m_arenaFlags & (1 << cell->m_extraInfo)) != 0;
}

VA(0x004e53e0, 0x18)  // dc 0xd5074
void hero::setVisitedArena(const NewmapCell* cell)
{
    m_arenaFlags |= 1 << cell->m_extraInfo;
}

VA(0x004e5400, 0x93)  // dc 0xd50a0
float hero::getCombatValueModifier() const
{
    signed char attack = m_stats[0];
    int attackValue;
    if (attack > 99)
        attackValue = 99;
    else if (attack > 0)
        attackValue = attack;
    else
        attackValue = 0;
    signed char defense = m_stats[1];
    int defenseValue;
    if (defense > 99)
        defenseValue = 99;
    else if (defense > 0)
        defenseValue = defense;
    else
        defenseValue = 0;
    return static_cast<float>(sqrt((attackValue * 0.05 + 1.0)
                                   * (defenseValue * 0.05 + 1.0)));
}

VA(0x004e54a0, 0xAA)  // dc 0xd519c
boat* hero::findSummonableBoat() const
{
    boat* result = g_game->getHeroBoat(m_id, 0);
    if (result)
        return result;

    int closestDistance = 0;
    for (boat* candidate = g_game->m_boats.begin();
         candidate != g_game->m_boats.end(); candidate++) {
        if (candidate->m_allocated && !candidate->m_occupied
            && (candidate->m_playerOwner == g_netLocalGamePos
                || candidate->m_playerOwner == -1)) {
            int distance = abs(candidate->m_x - m_x) + abs(candidate->m_y - m_y);
            if (!result || distance <= closestDistance) {
                result = candidate;
                closestDistance = distance;
            }
        }
    }
    return result;
}

VA(0x004e5550, 0x15E)  // dc 0xd524c
unsigned char hero::canSummonBoat() const
{
    if (!spellIsAvailable(SPELL_SUMMON_BOAT))
        return 0;

    TSkillMastery baseMastery = getSpellLevel(
        SPELL_SUMMON_BOAT, kMagicTerrainNone);

    int cost = getManaCost(SPELL_SUMMON_BOAT);
    if (m_mana < cost)
        return 0;
    if (findSummonableBoat())
        return 1;
    if (baseMastery < eMasteryAdvanced)
        return 0;
    return g_game->getNewBoatId() != -1;
}

VA(0x004e56b0, 0x21)  // dc 0xd52b0
playerData* hero::getPlayer() const
{
    if (m_owner < 0)
        return 0;
    return &g_game->m_players[m_owner];
}

VA(0x004e56e0, 0x7C)  // dc 0xd52d0
unsigned char hero::isInPatrolRadius(type_point point) const
{
    if (m_patrolRadius < 0 || m_patrolX == kPatrolNone)
        return 1;
    if (point.m_z != m_z)
        return 0;
    return abs(point.m_x - m_patrolX) + abs(point.m_y - m_patrolY) <= m_patrolRadius;
}

VA(0x004e5760, 0x1F2)  // dc 0xd53a0
long hero::modifySpellDamage(SpellID spell, int damage,
                               const class army* targetArmy) const
{
    float value = static_cast<float>(damage);
    int school = g_spellTraits[spell].m_school;
    if (((school & eSchoolAir) && isWieldingArtifact(ARTIFACT_ORB_OF_THE_FIRMAMENT))
        || ((school & eSchoolEarth) && isWieldingArtifact(ARTIFACT_ORB_OF_SILT))
        || ((school & eSchoolFire) && isWieldingArtifact(ARTIFACT_ORB_OF_TEMPESTUOUS_FIRE))
        || ((school & eSchoolWater) && isWieldingArtifact(ARTIFACT_ORB_OF_DRIVING_RAIN)))
        value = value * 1.5f;
    float factor = g_sorceryFactors[m_skillLevel[eSecSkillSorcery]];
    if (m_skillLevel[eSecSkillSorcery] > 0) {
        const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];
        if (ability.m_type == eHeroAbilitySecondarySkill
            && ability.m_skill == eSecSkillSorcery)
            factor = (m_level * 0.05f + 1.0f) * factor;
    }
    value = (factor + 1.0f) * value;
    if (targetArmy)
        value = static_cast<float>(
                    getHeroSpellBonus(spell, targetArmy->m_monInfo.m_level,
                                      static_cast<int>(value)))
                + value;
    return static_cast<long>(value);
}

VA(0x004e5960, 0x38)  // dc 0xd544c
short hero::getPrimarySkillTotal() const
{
    short total = 0;
    for (short skill = 0; skill < 4; ++skill) {
        total += getPrimarySkill(skill);
    }
    return total;
}

VA(0x004e59a0, 0xF8)  // dc 0xd5488
void hero::fly(int level)
{
    m_flightLevel = level;
    useSpell(getManaCost(SPELL_FLY));
}

VA(0x004e5aa0, 0xE0)  // dc 0xd54ac
long hero::getCombatSpeedBonus() const
{
    long bonus = 0;
    if (isWieldingArtifact(ARTIFACT_NECKLACE_OF_SWIFTNESS))
        bonus++;
    if (isWieldingArtifact(ARTIFACT_RING_OF_THE_WAYFARER))
        bonus++;
    if (isWieldingArtifact(ARTIFACT_CAPE_OF_VELOCITY))
        bonus += 2;
    if (g_heroSpecificAbilities[m_id].m_type == eHeroAbilityKind5)
        bonus += 2;
    return bonus;
}

VA(0x004e5b80, 0x15C)  // dc 0xd5508
long hero::getHitPointBonus(int creatureType) const
{
    long bonus = 0;
    if (isWieldingArtifact(ARTIFACT_RING_OF_VITALITY))
        bonus = 1;
    if (isWieldingArtifact(ARTIFACT_RING_OF_LIFE))
        bonus++;
    if (isWieldingArtifact(ARTIFACT_VIAL_OF_LIFEBLOOD))
        bonus += 2;
    if ((g_creatureTypeTraits[creatureType].m_attributes & 0x10)
        && isWieldingArtifact(ARTIFACT_ELIXIR_OF_LIFE))
        bonus += g_creatureTypeTraits[creatureType].m_hitPoints / 4;
    return bonus;
}

VA(0x004e5ce0, 0xE7)  // dc 0xd5548
unsigned char hero::canLand() const
{
    type_point point;
    point.m_x = m_x;
    point.m_y = m_y;
    point.m_z = m_z;

    NewmapCell* cell = g_game->m_worldMap.cell(point.m_x, point.m_y, point.m_z);
    if ((cell->m_groundSet == eTerrainWater)
        == ((m_flags & 0x40000) == 0)) {
        return 0;
    }
    if (!(cell->m_flags0011 & 0x40))
        return 0;
    if (cell->m_isTrigger && g_adventureObjectLandBlocked[cell->m_type][0])
        return 0;
    return 1;
}

VA(0x004e5dd0, 0x10)  // dc 0xd55b8
void hero::walkOnWater(int level)
{
    m_waterWalkLevel = level;
}

VA(0x004e5de0, 0x2D)
int hero::heroFn004E5DE0() const
{
    if (m_visionsPower < 3 && m_army.getCreatureTotal(CREATURE_ROGUE) != 0)
        return 3;
    return m_visionsPower;
}

VA(0x004e5e10, 0x11C)  // dc 0xd55c0
unsigned char hero::isInIdentifyRange(const type_point* location) const
{
    int identifyLevel = heroFn004E5DE0();
    int range = g_spellTraits[SPELL_VISIONS].m_masteryBonus[identifyLevel]
        * getPrimarySkill(2);
    if (range < 3)
        range = 3;

    if (m_z == location->m_z) {
        // Constructor form, not default-then-assign: it merges the y|z
        // bitfield unit into one clear-then-or (98.6813 -> 100.0000).
        type_point heroLocation(m_x, m_y, m_z);

        int xDistance = location->m_x - heroLocation.m_x;
        int yDistance = location->m_y - heroLocation.m_y;
        if (xDistance * xDistance + yDistance * yDistance < range * range)
            return 1;
    }
    return 0;
}

VA(0x004e5f30, 0xBF)  // dc 0xd5644
unsigned char hero::isMobile() const
{
    type_point point;
    point.m_x = m_x;
    point.m_y = m_y;
    point.m_z = m_z;

    NewmapCell* cell = g_advManager->getCell(point);
    int pathfinding = m_skillLevel[eSecSkillPathfinding];
    int cost;
    if (m_flags & 0x40000) {
        cost = minimumTerrainCost(
            cell, m_movePoints, pathfinding, -1, -1,
            m_army.getCreatureTotal(CREATURE_NOMAD) > 0);
    } else {
        cost = minimumTerrainCost(
            cell, m_movePoints, pathfinding, m_flightLevel, m_waterWalkLevel,
            m_army.getCreatureTotal(CREATURE_NOMAD) > 0);
    }
    return m_movePoints >= cost;
}

VA(0x004e5ff0, 0x123)  // dc 0xd5710
int hero::getHeroSpellBonus(SpellID spellId, int targetLevel, int value) const
{
    HOMM3_RELEASE_VERIFY(m_id >= 0);
    HOMM3_RELEASE_VERIFY(m_id < sizeof(g_heroSpecificAbilities)
        / sizeof(g_heroSpecificAbilities[0]));
    int bonus = 0;
    const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];
    if (ability.m_type == eHeroAbilitySpell
        && static_cast<int>(ability.m_skill) == spellId) {
        switch (spellId) {
        case SPELL_BLOODLUST:
        case SPELL_PRECISION:
        case SPELL_WEAKNESS:
        case SPELL_STONE_SKIN:
        case SPELL_PRAYER:
        case g_spellHaste:
            return g_buffSpecialtyBonus[targetLevel];
        case SPELL_SLAYER:
            return g_slayerSpecialtyBonus[targetLevel];
        case SPELL_FORTUNE:
            return 3 - value;
        case g_spellFireWall:
            return value;
        case SPELL_DISRUPTING_RAY:
            return 2;
        case g_spellMagicArrow:
            return value >> 1;
        }
        bonus = static_cast<int>(
            ceil(m_level / (targetLevel + 1) * value * 0.03));
    }
    return bonus;
}

// E:\gamedcs\hero.cpp:6493

VA(0x004e6120, 0x39E)
void hero::heroFn004E6120(int creatureType,
                           TCreatureTypeTraits* traits) const
{
    traits->m_attackSkill += getPrimarySkill(0);
    traits->m_defenseSkill += getPrimarySkill(1);

    const THeroSpecificAbility& ability = g_heroSpecificAbilities[m_id];

    if (isWieldingArtifact(g_artifactVialOfDragonBlood)
        && (traits->m_attributes & g_creatureAttrDragon)) {
        int bonus = g_vialOfDragonBloodBonus;
        traits->m_attackSkill += bonus;
        traits->m_defenseSkill += bonus;
    }

    switch (ability.m_type) {
    case eHeroAbilityCreature:
    case eHeroAbilityCreatureUniversal:
        if (creatureType == ability.m_creature
            || (ability.m_creature != CREATURE_BALLISTA
                && creatureType == g_game->upgradedCreatureType(ability.m_creature))) {
            if (ability.m_type == eHeroAbilityCreature) {
                double scale = m_level / (traits->m_level + 1) * 0.05;
                traits->m_attackSkill = static_cast<int>(
                    ceil(g_creatureTypeTraits[creatureType].m_attackSkill * scale)
                    + traits->m_attackSkill);
                traits->m_defenseSkill = static_cast<int>(
                    ceil(g_creatureTypeTraits[creatureType].m_defenseSkill * scale)
                    + traits->m_defenseSkill);
                if (!(traits->m_attributes & g_ctaSiegeWeapon))
                    traits->m_speed++;
            } else {
                traits->m_attackSkill += ability.m_creatureAttackBonus;
                traits->m_defenseSkill += ability.m_creatureDefenseBonus;
                traits->m_damageLowBound += ability.m_creatureDamageBonus;
                traits->m_damageHighBound += ability.m_creatureDamageBonus;
                if (m_id == g_heroXeron)
                    traits->m_speed++;
            }
        }
        break;
    case eHeroAbilityDragons:
        if (g_creatureTypeTraits[creatureType].m_attributes
            & g_creatureAttrDragon) {
            traits->m_attackSkill += ability.m_creatureAttackBonus;
            traits->m_defenseSkill += ability.m_creatureDefenseBonus;
            traits->m_damageLowBound += ability.m_creatureDamageBonus;
            traits->m_damageHighBound += ability.m_creatureDamageBonus;
        }
        break;
    }

    if (!(traits->m_attributes & g_ctaSiegeWeapon))
        traits->m_speed += getCombatSpeedBonus();
    traits->m_hitPoints += getHitPointBonus(creatureType);
}

#if 0  // @carcass

VA_COMPGEN(0x004e64c0, 0x1B, BITSET_ANY, Bitset144)
bool std::bitset<144>::any() const
{
    // @stub - <bitset>'s own definition; see h3_stl_comdat_anchor
}

VA(0x004e64e0, 0x4)  // stl-comdat, retail-only
int* std::vector<int>::begin()
{
    // @stub - <vector>'s own definition; see h3_stl_comdat_anchor
}

VA(0x004e64f0, 0x4)  // stl-comdat, retail-only
int* std::vector<int>::end()
{
    // @stub - <vector>'s own definition; see h3_stl_comdat_anchor
}

// std::vector<T>::push_back(const T&) - 434 B, `ret 4`; see the note
// above for why this is push_back and not either insert overload.
VA(0x004e6500, 0x1B2)  // stl-comdat, retail-only
void std::vector<int>::push_back(const int& _X)
{
    // @stub - <vector>'s own definition; see h3_stl_comdat_anchor
}

VA_COMPGEN(0x004e66f0, 0x60, BITSET_SET, Bitset70)
std::bitset<70>& std::bitset<70>::set(size_t _P, bool _X)
{
    // @stub - <bitset>'s own definition; see h3_stl_comdat_anchor
}

VA(0x004e6750, 0x21)  // anchor-caller + reference ABI/body, dc 0x20d2c
inline const int& tLimit(const int& minimum, const int& value, const int& maximum);

// E:\gamedcs\hero.cpp:6493
DC_ONLY(0xd5800, 0xCC)
void hero::reset_artifacts()
{
    // @stub
}

// E:\gamedcs\hero.cpp:1226
DC_ONLY(0xd59b8, 0x18)
void type_artifact::`default constructor closure'()
{
    // @stub
}

// E:\gamedcs\hero.cpp:4186
// (moved to retail link order at 0x004e1520, immediately before the
// destructor it calls; the VA_COMPGEN claim lives there)

#endif  // @carcass

// ---------------------------------------------------------------------
// STL COMDAT emission anchor - SCAFFOLDING, flagged for ratification.

// The five claims in the COMDAT-tail block above name template members
// whose definitions live in VC6's <vector>/<bitset>.  A claim alone does
// not make the compiler emit anything: an inline member only gets its
// out-of-line COMDAT when some call site declines to inline it, and with
// /Ob2 every ordinary call site inlines.  `#pragma inline_depth(0)` is
// the smallest construct that reproduces retail's emission decision, and
// it costs exactly one extra symbol in hero.obj.

VA_COMPGEN(0x0048d940, 0x26, VECTOR_UFILL, int)

// COMDAT pairing: bitset<144>::test, agreement 1.000 at an exactly equal
// 55-byte extent.
VA_COMPGEN(0x0044d4d0, 0x37, BITSET_TEST, Bitset144)

// Retail loadMap, markArtifactSpells (0x4d9350) and readTownData share this
// bitset<70> cleanup. It expands in game but remains naturally emitted here
// and in mapcell. All 37 bytes agree, including the three-word fill and
// six-bit high-word mask; there are no relocations or added source calls.
VA_COMPGEN(0x004cfa10, 0x25, BITSET_TIDY, Bitset70)
