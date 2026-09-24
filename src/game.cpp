#include "prefs.h"
#include "text.h"
#include "va.h"
#include "bitset_iterator.h"
#include "packed_bits.h"
#include <algorithm>
#include <ctype.h>
#if defined(HOMM3_TARGET_MAC)
#include <unistd.h>
#else
#include <direct.h>
#include <io.h>
#endif
#include <fcntl.h>
#include <math.h>
#include <memory>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "game.h"

#include "adventuremapwindow.h"
#include "advmgr.h"
#include "advmgr_objects.h"
#include "bitmap816.h"
#include "creature_bank.h"
#include "creaturetype.h"
#include "cursor.h"
#include "diff.h"
#include "exec.h"
#include "findpath.h"
#include "gamecontext.h"
#include "herospec.h"
#include "imm_mouse.h"
#include "initialize.h"
#include "inputmgr.h"
#include "kb.h"
#include "kbwin.h"
#include "misc.h"
#include "mousemgr.h"
#include "multiplayerwindow_globals.h"
#include "netgame.h"
#include "netplayer.h"
#include "puzzlewindow.h"
#include "quicktownwindow.h"
#include "recruit.h"
#include "remote.h"
#include "remotedlg.h"
#include "resourcedisplay.h"
#include "resourcemanager.h"
#include "savegame.h"
#include "smackmgr.h"
#include "soundmgr.h"
#include "spellbookwindow.h"
#include "terrain.h"
#include "timer.h"
#include "townmgr_globals.h"
#include "turn_update_msg.h"
#include "viewarmywindow.h"
#include "winfile.h"
#include "winmgr.h"

// Initial contents recovered from the pinned Complete image.
DATA(0x0063d570) TCreatureType g_creatureGenerator1Types[80] = { TCreatureType(106), TCreatureType(96), TCreatureType(74), TCreatureType(66), TCreatureType(68), TCreatureType(10), TCreatureType(14), TCreatureType(112), TCreatureType(12), TCreatureType(94), TCreatureType(54), TCreatureType(104), TCreatureType(16), TCreatureType(113), TCreatureType(52), TCreatureType(18), TCreatureType(114), TCreatureType(30), TCreatureType(36), TCreatureType(86), TCreatureType(98), TCreatureType(84), TCreatureType(44), TCreatureType(102), TCreatureType(26), TCreatureType(4), TCreatureType(72), TCreatureType(46), TCreatureType(110), TCreatureType(42), TCreatureType(100), TCreatureType(34), TCreatureType(80), TCreatureType(76), TCreatureType(78), TCreatureType(8), TCreatureType(38), TCreatureType(48), TCreatureType(90), TCreatureType(88), TCreatureType(50), TCreatureType(82), TCreatureType(92), TCreatureType(28), TCreatureType(40), TCreatureType(22), TCreatureType(70), TCreatureType(115), TCreatureType(60), TCreatureType(108), TCreatureType(20), TCreatureType(24), TCreatureType(64), TCreatureType(62), TCreatureType(56), TCreatureType(58), TCreatureType(0), TCreatureType(2), TCreatureType(6), TCreatureType(118), TCreatureType(120), TCreatureType(130), TCreatureType(132), TCreatureType(133), TCreatureType(134), TCreatureType(135), TCreatureType(136), TCreatureType(137), TCreatureType(24), TCreatureType(112), TCreatureType(113), TCreatureType(114), TCreatureType(115), TCreatureType(138), TCreatureType(139), TCreatureType(140), TCreatureType(141), TCreatureType(142), TCreatureType(143), TCreatureType(144) };
DATA(0x00677938) TCreatureType g_creatureGenerator4Types[2][4] = {
    { TCreatureType(112), TCreatureType(114), TCreatureType(113), TCreatureType(115) },
    { TCreatureType(32), TCreatureType(33), TCreatureType(116), TCreatureType(117) }
};
DATA(0x00677974) const char* g_artifactObjectDefFormat = "ava%04d.def";

// Retail initial data; dimensions follow the typed table consumers.
DATA(0x00677998) double g_productionHandicap[3] = { 0.0, 0.15, 0.3 };
DATA(0x00678170) const int g_initResourcesHuman[5][7] = {
    { 30, 15, 30, 15, 15, 15, 30000 },
    { 20, 10, 20, 10, 10, 10, 20000 },
    { 15, 7, 15, 7, 7, 7, 15000 },
    { 10, 4, 10, 4, 4, 4, 10000 },
    { 0, 0, 0, 0, 0, 0, 0 }
};
DATA(0x006781fc) const int g_initResourcesComputer[5][7] = {
    { 5, 2, 5, 2, 2, 2, 5000 },
    { 10, 4, 10, 4, 4, 4, 7500 },
    { 15, 7, 15, 7, 7, 7, 10000 },
    { 15, 7, 15, 7, 7, 7, 10000 },
    { 15, 7, 15, 7, 7, 7, 10000 }
};

DATA(0x0069ccb0) playerData* g_currentPlayer;
DATA(0x0069cca8) int g_netLocalGamePos;
DATA(0x00699554) int g_localGamePos;
// Original DC name: iSandAnim; GetTurnAIVars resets it beside iCurHourGlassPhase.
DATA(0x00691680) int g_sandAnim;


// Retail table initializers, in the layouts used by their named consumers.
DATA(0x00677958) const char* g_resourceObjectDefs[NUM_RESOURCES] = { "avtwood0.def", "avtmerc0.def", "avtore0.def", "avtsulf0.def", "avtcrys0.def", "avtgems0.def", "avtgold0.def" };
DATA(0x00677a0c) const char* g_townVillageObjectDefs[9] = { "AVCcast0.def", "AVCramp0.def", "AVCtowr0.def", "AVCinft0.def", "AVCnecr0.def", "AVCdung0.def", "AVCstro0.def", "AVCftrt0.def", "AVChfor0.def" };
DATA(0x00677a30) const char* g_townFortObjectDefs[9] = { "AVCcasx0.def", "AVCramx0.def", "AVCtowx0.def", "AVCinfx0.def", "AVCnecx0.def", "AVCdunx0.def", "AVCstrx0.def", "AVCftrx0.def", "AVChforx.def" };
DATA(0x00677a54) const char* g_townCapitolObjectDefs[9] = { "AVCcasz0.def", "AVCramz0.def", "AVCtowz0.def", "AVCinfz0.def", "AVCnecz0.def", "AVCdunz0.def", "AVCstrz0.def", "AVCforz0.def", "AVChforz.def" };
DATA(0x00677978) int g_mineProduction[7] = { 2, 1, 2, 1, 1, 1, 1000 };
DATA(0x006779b0) const int g_neutralTownLevelWeights[6] = { 2, 3, 4, 5, 4, 3 };
DATA(0x0069fbf8) int g_newMapStartingBonus[8];
DATA(0x0069fb24) int g_startingHeroOverrides[8];

// Retail scalar state; startup initial values come from the pinned image.
DATA(0x00697294) TTextResource* g_randomTavernText;
DATA(0x0069774c) bool g_inCampaign;
DATA(0x00697750) int g_weekType;
DATA(0x006983fc) int g_weekTypeExtra;
DATA(0x00697748) int g_monthType;
DATA(0x00698834) int g_monthTypeExtra;
DATA(0x006783c8) int g_mapWidth = 72;
DATA(0x006783cc) int g_mapHeight = 72;
// Original DC name: g_playerTurn; StartLocalPlayerTurn and remote turn handoff.
DATA(0x0069d810) int g_playerTurn;
DATA(0x0067814c) int g_heroGoldCost = 2500;
DATA(0x0069950c) int g_grailOwner;
DATA(0x0069951c) unsigned char g_normalVictory;
DATA(0x0069ccc4) unsigned char g_curPlayerBit;

type_point aiAttemptPuzzleGuess(long player);

// Retail/HD evidence names the hourglass animation phase; NextPlayer is its
// game.obj writer. The second dword is the byte-proven autosave preference
// gate, but no surviving symbol attests a semantic spelling for it.
DATA(0x00691684) int g_curHourGlassPhase;

// Hero-setup maps use the native Dinkumware <utility>/<xtree> definitions.
// Retail retains the pair cleanup at 0x4c4df0 and the tree minimum, insert
// and iterator decrement at 0x4d2050/0x4cff50/0x4d27b0. Their VA_COMPGEN
// claims below name those library instances; project-written replacements
// do not own them. The per-TU /MT profile supplies the external _Lockit
// calls used by the native tree scopes (0x60b598/0x60b634).

template <class T>
bool saveObjectVector(TAbstractFile* outfile, std::vector<T>& srcVector);
template <class T>
bool loadObjectVector(TAbstractFile* infile, std::vector<T>& destVector);

const int g_savedCreatureNone = 0xff;
const int g_savedMapCoordinateNone = 0xff;
// The on-disk hero-id domain playerData::load reads. 0xff is the "no
// hero" sentinel the roster stores as -1. Saves older than version 25
// spell two heroes 0x80/0x81 where the shipped roster carries them at
// 0x92/0x9c, so the reader rewrites them on the way in - the retail
// bytes prove the remap, not the two heroes' identities.
const int g_savedHeroNone = 0xff;
const int g_savedHeroPre25First = 0x80;
const int g_savedHeroPre25Second = 0x81;
const int g_heroPre25FirstRemap = 0x92;
const int g_heroPre25SecondRemap = 0x9c;
const int g_mapVersionOldCampaignHeroIds = 14;
const int g_saveVersionLossHeroCoordinates = 16;
const int g_saveVersionCompleteHeroRoster = 25;
const int g_saveVersionMaxHeroLevel = 27;
const int g_saveVersionWideAlignments = 28;
const int g_saveVersionCustomHeroSetups = 30;
const int g_saveVersionCustomHeroAvailability = 31;
const int g_saveGameFailureGeneralText = 10;
const int g_mapVersionErrorGeneralText = 431;
const int g_mapHeaderPlayerCount = 8;
const int g_mapHeaderLegacyHeroCount = 128;
const int g_mapHeaderHeroCount = 156;
const int g_mapHeaderCompleteLegacyHeroFirst = 128;
const int g_mapHeaderCompleteLegacyHeroLast = 143;
const int g_mapHeaderPaddingSize = 31;
const int g_campaignVictoryOverrideFirst = 10;
const int g_campaignVictoryOverrideSecond = 11;
const int g_campaignVictoryOverrideThird = 12;
const int g_campaignVictoryOverrideDays = 112;

// E:\gamedcs\game.cpp's file-static `resources` row (dc game.obj static
// 0x2ffc). Complete retains the same four rare-resource ids in retail
// game.obj at 0x63e668; PerDay indexes it with Random(0, 3) for the
// Rampart's Mystic Pond.
DATA(0x0063e668) static const int g_resources[4] = {
    MERCURY, SULFUR, CRYSTAL, GEMS
};

// E:\gamedcs\game.cpp's file-static `giMonType` row (DC type: const
// char[12]). Retail PerMonth indexes these exact Complete-roster creature
// ids when it rolls an ordinary creature month.
DATA(0x0063e678) static const char g_monType[12] = {
    0x68, 0x04, 0x0e, 0x1c, 0x55, 0x2c,
    0x46, 0x48, 0x64, 0x14, 0x56, 0x3c
};

// Complete artifact 138, the Wizard's Well combination. PerDay is the
// identifying retail body: wearing it restores full mana every day instead
// of applying the ordinary Mysticism increment.
const int g_artifactWizardsWellId = 0x8a;

// Calendar-period values written by PerWeek. The ordinary creature week is
// followed by the Inferno Grail's forced Imp week.
const int g_weekTypeNormal = 0;
const int g_weekTypeCreature = 1;
const int g_weekTypeInfernoGrail = 2;
const int g_weekNameLast = 14;
const int g_weeksPerMonth = 4;
const int g_specialWeekRollMax = 4;
const int g_creatureWeekGrowthBonus = 5;
const int g_creatureImpId = 0x2a;
const int g_creatureFamiliarId = 0x2b;
const int g_monthEffectNormal = 0;
const int g_monthEffectCreature = 1;
const int g_monthEffectPlague = 2;
const int g_monthRollMax = 10;
const int g_monthNormalRollMax = 5;
const int g_monthCreatureRollMax = 9;
const int g_monthCreatureTableLast = 11;
const int g_monthMonsterSpawnRollMax = 200;
const int g_monthMonsterDispositionMax = 10;
const unsigned int g_heroRecruitReservedFlag = 0x20000;
const unsigned int g_heroWeeklyVisitFlag = 2;
const int g_rumourMapThreshold = 33;
const int g_rumourSpecialThreshold = 66;
const int g_neutralTownOpenReinforcementChance = 40;
const int g_neutralTownFortifiedReinforcementChance = 80;

// The ArrayTxt loader fills these calendar-name rows and the eight adjacent
// new-turn message formats. Dreamcast supplies gWeekNames/gMonthNames; the
// remaining spellings describe only the retail use proven in DoNewTurn.
DATA(0x006a79c4) extern const char* g_monthNames[10];
DATA(0x006a7710) extern const char* g_weekNames[15];

// The 256 canned-rumour text pointers are filled by the game-data loader.
// Its first store is 0x696d9c and SetCannedRumour is the table's only
// runtime reader.
DATA(0x00696d9c) const char* g_cannedRumours[256];

// randtvrn.txt itself, kept alive because the table above points into it.
// InitializeRandomTavernText is its only writer and nothing else in the
// image reads the slot.


const int g_allRandomArtifactClasses = 0x1e;
const int g_campaignArmyOverrideHero = 45;
const int g_campaignArmyOverrideCampaign = 14;
const int g_campaignArmyOverrideTraits = 96;
// game::GetRandomMonster gates the Conflux strike-out on
// `campaign.currentCampaign >= 13`. Thirteen is where TCampaignWindow's
// page seeds put the Shadow of Death set - page 0 covers rows 0-6 (Restoration
// of Erathia), page 1 rows 7-12 (Armageddon's Blade), page 2 rows 13-19.
// campaignwindow.h is not in this TU's closure, so the value lives here as a
// local constant, exactly as CAMPAIGN_ARMY_OVERRIDE_CAMPAIGN does.
const int g_firstShadowOfDeathCampaign = 13;
// game::CreateTownHeroes carries the SAME start-level override
// hero.cpp's HeroFn_004D8B30 does, on the same campaign/scenario pair
// and the same donor hero - both bodies spell `campaign == 8 &&
// currentMap == 3` and then read gpGame + 0x4c893, which IS
// heroes[151].level. The names mirror hero.cpp's kStartLevel* block so
// the two readers of the pair stay recognisably one rule; the values are
// retail's literals. The campaign's identity is now more than inference:
// gCampaignFileNames at 0x66cadc holds 'blood.h3c' at ordinal 8 (IDA
// names it aBlood_h3c), and hero.cpp's own kArtifactVialOfDragonBlood
// sits in the same block - but the ordinal is what retail compares, so
// the constants stay role-named rather than importing a title.
const int g_startLevelCampaign = 8;
const int g_startLevelScenario = 3;
const int g_startLevelHeroId = 151;
const int g_startLevelBonus = 5;

const int g_campaignPopulationEventCampaign = 8;
const int g_campaignPopulationEventScenario = 0;
const int g_campaignPopulationEventDay = 36;

const int g_specialRumourFirstCategory = 6;
const int g_specialRumourLastCategory = 9;
const int g_specialRumourAttempts = 200;
const int g_specialRumourChance = 80;
const int g_specialRumourLocationChance = 50;
const int g_specialRumourCategoryText = 209;
const int g_specialRumourGrailObjectText = 213;
const int g_specialRumourGrailAboveText = 264;
const int g_specialRumourGrailBelowText = 265;

const int g_firstArmageddonsBladeCampaign = 7;
// The map's full obelisk roster and the puzzle it uncovers are the same
// 48: game::obeliskFlags is 0x30 entries, puzzlePiecesRemoved is a
// std::bitset<48>, and GetNumObelisks scans exactly 48. The placement
// loop works over the 47 pieces below the last one.
const int g_obeliskCount = 48;
const int g_puzzlePlaceablePieces = 47;

// The Dreamcast enum leaves 18/19 unnamed, but retail's own diagnostics at
// 0x677e38/0x677db0 call these exact switch values CREATURE_GENERATOR_2 and
// CREATURE_GENERATOR_3. Keep the PC-only names scoped to their sole consumer
// instead of rewriting the cross-build enum roster in mapcell.h.
const int g_retailCreatureGenerator2 = 18;
const int g_retailCreatureGenerator3 = 19;

// RandomizeEvents residual (87.01%): retail emits a SEPARATE jump-table arm
// for BLACK_BOX_RANDOM_RELIC (0x4c19d1) where our cross-jumper folds it onto
// the WARRIOR_TOMB if-chain's identical `push 0x10` block, and it keeps BOTH
// SetWagon overloads out of line where we expand the two-argument one.
// Measured and rejected: reordering the black-box arms so RELIC comes second
// (-0.20); pinning the two-argument SetWagon (-1.51, a knock-on re-price);
// routing REFUGEE_CAMP's GetRandomMonster through gpGame as retail does at
// 0x4c1652 (-0.14, so the reload is right and something downstream pays for
// it); narrowing RandomizeShrine's bitset pin to the subscript alone so the
// ctor expands onto retail's `_Tidy` call (-0.50).
// Random-map placeholder domains recovered from RandomizeEvents' retail
// switch. They are source-local because no cross-TU enum identity survives.
const int g_blackBoxRandomAny = 1;
const int g_blackBoxRandomTreasure = 2;
const int g_blackBoxRandomMinor = 3;
const int g_blackBoxRandomMajor = 4;
const int g_blackBoxRandomRelic = 5;
const int g_pyramidSpellLevel = 5;
const int g_shrineLevelOne = 0;
const int g_shrineLevelTwo = 1;
const int g_shrineLevelThree = 2;
const unsigned char g_whirlpoolTriggerXOffset = 2;
const unsigned char g_whirlpoolTriggerYOffset = 0x10;

const int g_productionArtifactCrystal = 0x6d;
const int g_productionArtifactGems = 0x6e;
const int g_productionArtifactMercury = 0x6f;
const int g_productionArtifactOre = 0x70;
const int g_productionArtifactSulfur = 0x71;
const int g_productionArtifactWood = 0x72;
const int g_productionArtifactEndlessSackOfGold = 0x73;
const int g_productionArtifactEndlessBagOfGold = 0x74;
const int g_productionArtifactEndlessPurseOfGold = 0x75;
const int g_productionArtifactCornucopia = 0x8c;
const int g_productionCreatureCrystalDragon = 0x85;
const int g_gameDifficultyEasy = 0;
const int g_gameDifficultyExpert = 3;
const int g_gameDifficultyImpossible = 4;

VA(0x004b8410, 0x33)  // dc 0xa2af8
unsigned char initializeRandomTavernText()
{
    g_randomTavernText = ResourceManager::getText(
        DATA_COMPGEN(0x00677d20, randomTavernTextName, "randtvrn.txt"));
    if (g_randomTavernText == 0)
        return 0;
    for (int i = 0; i < 256; i++)
        g_cannedRumours[i] = g_randomTavernText->getText(i);
    return 1;
}

VA(0x004b8450, 0xF7)
void HeroExtra::heroExtraFn004B8450(int heroId)
{
    m_owner = -1;
    m_id = heroId;
    m_objRef = 0;
    m_location = type_point(-1, -1, -1);
    m_patrolRadius = -1;
    m_hasCustomName = 0;
    m_customExperience = 0;
    m_customPortraitNumber = 0;
    m_customSecondarySkills = 0;
    m_customArmies = 0;
    m_groupFormation = 0;
    m_customArtifacts = 0;
    m_customName = 0;
    m_name = std::string();
    m_sex = -1;
    m_customSpells = 0;
    m_customPrimarySkills = 0;
}

VA(0x004b8550, 0x48)  // dc 0xa2da0
generator::generator()
    : m_genClass(-1), m_genType(-1)
{
    m_playerOwner = -1;
    m_mapX = -1;
    m_mapY = -1;
    m_mapZ = -1;
    m_townId = -1;
    for (int i = 0; i < 4; i++) {
        m_type[i] = CREATURE_NONE;
        m_population[i] = 0;
    }
}

VA(0x004b85a0, 0x13B)  // dc 0xa2e48
bool generator::load(TAbstractFile* infile)
{
    if (infile->read(&m_playerOwner, sizeof(m_playerOwner)) !=
        sizeof(m_playerOwner))
        return 0;
    if (infile->read(&m_genClass, sizeof(m_genClass)) != sizeof(m_genClass))
        return 0;
    if (infile->read(&m_genType, sizeof(m_genType)) != sizeof(m_genType))
        return 0;

    int loaded;
    for (int slot = 0; slot < 4; slot++) {
        infile->read(&loaded, 1);
        int creature = loaded & 0xff;
        {
            m_type[slot] = TCreatureType(creature);
        }
        if (creature == g_savedCreatureNone)
            m_type[slot] = CREATURE_NONE;
    }

    if (infile->read(m_population, sizeof(m_population)) != sizeof(m_population))
        return 0;
    if (infile->read(&m_mapX, sizeof(m_mapX)) != sizeof(m_mapX))
        return 0;
    if (infile->read(&m_mapY, sizeof(m_mapY)) != sizeof(m_mapY))
        return 0;
    if (infile->read(&m_mapZ, sizeof(m_mapZ)) != sizeof(m_mapZ))
        return 0;
    if (m_guards.load(infile) == -1)
        return 0;

    bool success =
        infile->read(&m_townId, sizeof(m_townId)) == sizeof(m_townId);
    return success;
}

VA(0x004b86e0, 0xB1)  // dc 0xa2fdc
bool generator::save(TAbstractFile* outfile)
{
    outfile->write(&m_playerOwner, sizeof(m_playerOwner));
    outfile->write(&m_genClass, sizeof(m_genClass));
    outfile->write(&m_genType, sizeof(m_genType));

    for (int slot = 0; slot < 4; slot++) {
        char creatureType = m_type[slot];
        outfile->write(&creatureType, sizeof(creatureType));
    }

    outfile->write(m_population, sizeof(m_population));
    outfile->write(&m_mapX, sizeof(m_mapX));
    outfile->write(&m_mapY, sizeof(m_mapY));
    outfile->write(&m_mapZ, sizeof(m_mapZ));
    m_guards.save(outfile);
    bool saved =
        outfile->write(&m_townId, sizeof(m_townId)) == sizeof(m_townId);
    return saved;
}

// E:\gamedcs\game.cpp:503
// update_bonus's negative twin, and retail keeps NO out-of-line row for
// it: generator::save ends at 0x4b8791 and update_bonus opens at
// 0x4b87a0, fifteen bytes later, so every retail caller expands it.
// game::ClaimTown is the caller that proves the body - its first
// generator sweep IS this function inlined, down to the `-1`.

// Dreamcast records the alignment local as TTownType and get_alignment's
// header declaration returns that same enum. Complete retains the helper call
// here while update_bonus expands its elemental gate and traits lookup.
// Dreamcast procedure: dc 0xa30c4.
inline void generator::removeBonus()
{
    if (m_playerOwner < 0)
        return;

    playerData& player = g_game->m_players[m_playerOwner];
    int alignment = g_game->getAlignment(m_type[0]);
    if (alignment == -1)
        return;

    for (long i = 0; i < player.m_numTowns; i++) {
        town* currentTown = g_game->getTown(player.m_townIds[i]);
        if (currentTown->m_type == alignment)
            currentTown->changeGeneratorBonus(m_type[0], -1);
    }
}

VA(0x004b87a0, 0xB8)  // dc 0xa3178
inline void generator::updateBonus()
{
    if (m_playerOwner < 0)
        return;

    playerData& player = g_game->m_players[m_playerOwner];
    int creature = m_type[0];
    if (!g_game->m_gameVersion &&
        isBaseElemental(creature))
        return;

    int townType = g_creatureTypeTraits[creature].m_townType;
    if (townType == -1)
        return;

    for (int index = 0; index < player.m_numTowns; index++) {
        town* currentTown = g_game->getTown(player.m_townIds[index]);
        if (currentTown->m_type == townType)
            currentTown->changeGeneratorBonus(m_type[0], 1);
    }
}

// E:\gamedcs\game.cpp:557
inline void generator::setOwner(long owner)
{
    if (owner == m_playerOwner)
        return;

    removeBonus();
    m_playerOwner = owner;
    updateBonus();
}

VA(0x004b8860, 0x1F7)  // dc 0xa3288
void generator::initialize(long newOwner)
{
    for (int slot = 0; slot < 4; slot++) {
        m_type[slot] = CREATURE_NONE;
        m_population[slot] = 0;
    }
    m_guards.initialize();

    TCreatureType* types;
    int typeCount;
    if (m_genClass == CREATURE_GENERATOR_1) {
        int generatorType = m_genType;
        types = &g_creatureGenerator1Types[generatorType];
        typeCount = 1;
    } else {
        int generatorType = m_genType;
        types = g_creatureGenerator4Types[generatorType];
        typeCount = 4;
    }

    m_townId = -1;
    m_playerOwner = -1;
    while (typeCount--) {
        m_type[typeCount] = types[typeCount];
    }

    grow(1);
    setOwner(newOwner);
}

VA(0x004b8a60, 0x88)  // dc 0xa3320
void generator::grow(int unusedArg)
{
    m_guards.initialize();
    for (long i = 0; i < 4; i++) {
        if (m_type[i] != -1) {
            m_population[i] = g_creatureTypeTraits[m_type[i]].m_growthRate;
            // DC game.cpp:614 records this traits assignment after the
            // population write.  That source order also makes VC6 retain
            // grow at both Complete call sites while preserving this body.
            const TCreatureTypeTraits* traits =
                &g_creatureTypeTraits[m_type[i]];
            if (traits->m_level >= 4)
                m_guards.add(m_type[i], m_population[i] * 3, -1);
        }
    }
}

// DC game.cpp:627 fixes the resource parameter as EGameResource.
static long getDayBonus(EGameResource resource, long weekBonus, long day)
{
    long result = weekBonus / 7;
    long remainder = weekBonus % 7;
    if ((day - resource + 6) % 7 < remainder)
        ++result;
    return result;
}

// E:\gamedcs\game.cpp:643. Complete's calculateProduction has 71/71
// CFG blocks and 13/13 calls in retail order. Its two Rampart
// hasBuilding(..., true) expansions differ in bitNumber/active-mask load
// scheduling; reversing the true arm's commutative operands in the canonical
// hasBuilding helper is byte-flat for this caller and its exact retained body.
// Mac 0:0xca978..0xcb1f0 has the same seven artifact-count calls followed by
// daily gold; the proposed pair remains in ignored build/mac/notes until the
// wider game declaration view is supported.
VA(0x004b8af0, 0x573)  // mine/town/player production consumers, dc 0xa3474
void game::calculateProduction()
{
    long playerId;
    long i;
    EGameResource resource;
    // DC calculate_production keeps its EGameResource induction variable.
    // Widen the ordinal locally without adding a conversion call boundary.
    double playerHandicap;
    for (playerId = 0; playerId < 8; ++playerId) {
        if (!m_playerDisabled[playerId]) {
            long (&production)[NUM_RESOURCES] =
                m_players[playerId].m_ai.m_turnProductionResource;
            MEMSET(production, 0, sizeof(production), i);
        }
    }

    long mineId;
    for (mineId = 0; mineId < m_mines.size(); ++mineId) {
        mine& currentMine = m_mines[mineId];
        if (currentMine.m_playerOwner >= 0 && currentMine.m_type < GOLD) {
            m_players[currentMine.m_playerOwner]
                .m_ai.m_turnProductionResource[currentMine.m_type] +=
                    g_mineProduction[currentMine.m_type];
        }
    }

    // Retail zeroes this eight-byte table with TWO dword stores, which is
    // VC6's inline `memset(p, 0, 8)`.  `= {0}` lowers to the 1/4/2/1
    // byte/dword/word/byte run instead and costs 0.65; the fully enumerated
    // `= {0,0,0,0,0,0,0,0}` is worse again (91.26).
    unsigned char crystalDragonIncome[8];
    memset(crystalDragonIncome, 0, sizeof(crystalDragonIncome));
    long townId;
    for (townId = 0; townId < m_towns.size(); ++townId) {
        town& currentTown = m_towns[townId];
        if (currentTown.m_owner < 0)
            continue;

        playerData& currentPlayer = m_players[currentTown.m_owner];
        long* production = currentPlayer.m_ai.m_turnProductionResource;
        if (currentTown.hasBuilding(MARKETPLACE_SILO_ID, false)) {
            int* siloIncome = currentTown.getSiloIncome();
            for (i = 0; i < NUM_RESOURCES; ++i)
                production[i] += siloIncome[i];
        }

        // `currentTown` is `town&`; the static_cast selects retail's
        // const get_army overload, and the Dreamcast-public QB query then
        // consumes its const armyGroup directly.
        {
            int storage;
            storage = g_productionCreatureCrystalDragon;
            if (static_cast<const town&>(currentTown).getArmy()
                    .getCreatureTotal(TCreatureType(storage)) > 0)
                crystalDragonIncome[currentTown.m_owner] = 1;
        }

        if (currentTown.m_type == TOWN_RAMPART && m_day == 1) {
            if (currentTown.hasBuilding(EXTRA_1_ID, true))
                production[GOLD] += currentPlayer.m_resources[GOLD] / 10;
            if (currentTown.hasBuilding(SPECIAL_BUILDING_ID, true)
                && currentTown.m_pondAmount > 0)
                production[currentTown.m_pondResource] += currentTown.m_pondAmount;
        }
    }

    for (playerId = 0; playerId < 8; ++playerId) {
        if (m_playerDisabled[playerId])
            continue;
        playerData& currentPlayer = m_players[playerId];
        long (&production)[NUM_RESOURCES] = currentPlayer.m_ai.m_turnProductionResource;
        int cornucopias = currentPlayer.numOfGivenArtifact(
            g_productionArtifactCornucopia) * 5;
        production[SULFUR] += cornucopias + currentPlayer.numOfGivenArtifact(
            g_productionArtifactSulfur);
        production[MERCURY] += cornucopias + currentPlayer.numOfGivenArtifact(
            g_productionArtifactMercury);
        production[GEMS] += cornucopias + currentPlayer.numOfGivenArtifact(
            g_productionArtifactGems);
        production[WOOD] += currentPlayer.numOfGivenArtifact(
            g_productionArtifactWood);
        production[ORE] += currentPlayer.numOfGivenArtifact(
            g_productionArtifactOre);
        production[CRYSTAL] += cornucopias + currentPlayer.numOfGivenArtifact(
            g_productionArtifactCrystal);
        production[GOLD] += computeDailyGold(playerId, 0);
    }

    for (i = 0; i < HERO_COUNT; ++i) {
        const hero& currHero = m_heroes[i];
        if (currHero.m_owner == -1)
            continue;
        {
            int storage;
            storage = g_productionCreatureCrystalDragon;
            if (currHero.m_army.getCreatureTotal(TCreatureType(storage)) > 0)
                crystalDragonIncome[currHero.m_owner] = 1;
        }
        long (&production)[NUM_RESOURCES] =
            m_players[currHero.m_owner].m_ai.m_turnProductionResource;
        if (g_heroSpecificAbilities[i].m_type == eHeroAbilityResource) {
            if (g_heroSpecificAbilities[i].m_skill >= WOOD
                && g_heroSpecificAbilities[i].m_skill <= GEMS)
                ++production[g_heroSpecificAbilities[i].m_skill];
        }
    }

    for (playerId = 0; playerId < 8; ++playerId) {
        if (m_day == 1 && crystalDragonIncome[playerId])
            m_players[playerId].m_ai.m_turnProductionResource[CRYSTAL] += 3;
    }

    if (m_setup.m_difficulty > 2) {
        for (playerId = 0; playerId < 8; ++playerId) {
            if (isHuman(playerId) || m_playerDisabled[playerId])
                continue;
            playerData& currentPlayer = m_players[playerId];
            long (&production)[NUM_RESOURCES] = currentPlayer.m_ai.m_turnProductionResource;
            production[WOOD] += getDayBonus(
                WOOD, production[WOOD] * 7 / 4, m_day);
            production[ORE] += getDayBonus(
                ORE, production[ORE] * 7 / 4, m_day);
            for (resource = WOOD; resource < GOLD;
                 resource = EGameResource(resource + 1)) {
                long weeklyBonus = (m_setup.m_difficulty - 2) * production[resource];
                production[resource] += getDayBonus(
                    resource, weeklyBonus, m_day);
            }
        }
    }

    for (playerId = 0; playerId < 8; ++playerId) {
        if (!m_setup.m_handicap[playerId] || m_playerDisabled[playerId])
            continue;
        playerData& currentPlayer = m_players[playerId];
        playerHandicap = g_productionHandicap[m_setup.m_handicap[playerId]];
        long (&production)[NUM_RESOURCES] = currentPlayer.m_ai.m_turnProductionResource;
        for (resource = WOOD; resource < GOLD;
                 resource = EGameResource(resource + 1))
            production[resource] -= production[resource] * playerHandicap;
    }
}

VA(0x004b9070, 0x1B3)  // dc 0xa3c68
int game::loadSignPool(TAbstractFile* infile)
{
    signed char count;
    if (infile->read(&count, sizeof(count)) < sizeof(count))
        return -1;

    m_signs.resize(count);
    for (unsigned int i = 0; i < m_signs.size(); ++i) {
        if (loadString(infile, m_signs[i].m_signText) < 0)
            return -1;

        if (infile->read(&count, sizeof(count)) < sizeof(count))
            return -1;
        m_signs[i].m_hasText = count != 0;
    }
    return 0;
}

VA(0x004b9270, 0xCF)  // dc 0xa3d50
int game::saveSignPool(TAbstractFile* outfile)
{
    // DC game.cpp:874..888 records int count, int x, char char_buffer,
    // with each write/save result assigned before its separate guard.
    // Complete uses the abstract-file write in place of DC's gzwrite.
    int count;
    int x;
    char charBuffer;
    charBuffer = static_cast<char>(m_signs.size());
    count = outfile->write(&charBuffer, sizeof(charBuffer));
    if (count < sizeof(charBuffer))
        return -1;

    for (x = 0; x < m_signs.size(); ++x) {
        count = saveString(outfile, m_signs[x].m_signText);
        if (count < 0)
            return -1;

        charBuffer = m_signs[x].m_hasText;
        count = outfile->write(&charBuffer, sizeof(charBuffer));
        if (count < sizeof(charBuffer))
            return -1;
    }
    return 0;
}

// Legacy guard reads use the shared scalar reader. VC6 inlines both calls
// and reproduces the retail body exactly; shared direct reads scored 97.44%.
// Mac retains the first signed result across the second read. This does not
// justify platform-specific statement ordering. DC predates this branch.
// Full native-header Mac comparison awaits the reviewed MSL resize binding.
VA(0x004b9340, 0x240)  // anchor-global (ClaimMine vector) + read-slot, dc 0xa3e5c
int game::loadMinePool(TAbstractFile* infile, int saveVersion)
{
    unsigned char count;
    int x;
    char charBuffer;
    if (infile->read(&count, sizeof(unsigned char)) < sizeof(unsigned char))
        return -1;
    m_mines.resize(static_cast<unsigned char>(count));

    for (x = 0; x < m_mines.size(); ++x) {
        if (infile->read(&charBuffer, sizeof(charBuffer))
            < sizeof(charBuffer))
            return -1;
        m_mines[x].m_playerOwner = charBuffer;
        if (infile->read(&charBuffer, sizeof(charBuffer))
            < sizeof(charBuffer))
            return -1;
        m_mines[x].m_type = charBuffer;
        if (infile->read(&charBuffer, sizeof(charBuffer))
            < sizeof(charBuffer))
            return -1;
        m_mines[x].m_isAbandoned = charBuffer != 0;

        if (saveVersion >= 25) {
            m_mines[x].m_guards.load(infile);
        } else {
            armyGroup* guards = &m_mines[x].m_guards;
            guards->initialize();
            int typeValue = readValue<signed char>(infile);
            int amountValue = readValue<signed char>(infile);
            if (typeValue != -1 && amountValue > 0)
                guards->add(typeValue, amountValue, -1);
        }

        if (infile->read(&count, sizeof(unsigned char)) < sizeof(unsigned char))
            return -1;
        m_mines[x].m_mapX = static_cast<unsigned char>(count);
        if (infile->read(&count, sizeof(unsigned char)) < sizeof(unsigned char))
            return -1;
        m_mines[x].m_mapY = static_cast<unsigned char>(count);
        if (infile->read(&count, sizeof(unsigned char)) < sizeof(unsigned char))
            return -1;
        m_mines[x].m_mapZ = static_cast<unsigned char>(count);
    }
    return 0;
}

VA(0x004b9580, 0x165)  // dc 0xa410c
int game::saveMinePool(TAbstractFile* outfile)
{
    unsigned char count = static_cast<unsigned char>(m_mines.size());
    if (outfile->write(&count, sizeof(count)) < sizeof(count))
        return -1;

    for (unsigned int i = 0; i < m_mines.size(); ++i) {
        unsigned char value = m_mines[i].m_playerOwner;
        if (outfile->write(&value, sizeof(value)) < sizeof(value))
            return -1;
        value = m_mines[i].m_type;
        if (outfile->write(&value, sizeof(value)) < sizeof(value))
            return -1;
        value = m_mines[i].m_isAbandoned;
        if (outfile->write(&value, sizeof(value)) < sizeof(value))
            return -1;

        m_mines[i].m_guards.save(outfile);

        count = m_mines[i].m_mapX;
        if (outfile->write(&count, sizeof(count)) < sizeof(count))
            return -1;
        count = m_mines[i].m_mapY;
        if (outfile->write(&count, sizeof(count)) < sizeof(count))
            return -1;
        count = m_mines[i].m_mapZ;
        if (outfile->write(&count, sizeof(count)) < sizeof(count))
            return -1;
    }
    return 0;
}

VA(0x004b96f0, 0x1CB)  // dc 0xa438c
int game::loadGarrisonPool(TAbstractFile* infile, int saveVersion)
{
    int count;
    if (infile->read(&count, sizeof(unsigned char)) < sizeof(unsigned char))
        return -1;

    m_garrisons.resize(count & 0xff);
    for (unsigned int i = 0; i < m_garrisons.size(); ++i) {
        unsigned char owner;
        if (infile->read(&owner, sizeof(owner)) < sizeof(owner))
            return -1;
        m_garrisons[i].m_playerOwner = owner;

        m_garrisons[i].m_garrisonArmy.load(infile);

        if (infile->read(&count, sizeof(unsigned char)) < sizeof(unsigned char))
            return -1;
        m_garrisons[i].m_mapX = static_cast<unsigned char>(count);
        if (infile->read(&count, sizeof(unsigned char)) < sizeof(unsigned char))
            return -1;
        m_garrisons[i].m_mapY = static_cast<unsigned char>(count);
        if (infile->read(&count, sizeof(unsigned char)) < sizeof(unsigned char))
            return -1;
        m_garrisons[i].m_mapZ = static_cast<unsigned char>(count);

        if (saveVersion < 28) {
            m_garrisons[i].m_removableTroops = !g_inCampaign;
        } else {
            unsigned char value;
            infile->read(&value, sizeof(value));
            m_garrisons[i].m_removableTroops = value != 0;
        }
    }
    return 0;
}

VA(0x004b98c0, 0x139)  // dc 0xa4548
int game::saveGarrisonPool(TAbstractFile* outfile)
{
    unsigned char count = static_cast<unsigned char>(m_garrisons.size());
    if (outfile->write(&count, sizeof(count)) < sizeof(count))
        return -1;

    for (unsigned int i = 0; i < m_garrisons.size(); ++i) {
        unsigned char owner = m_garrisons[i].m_playerOwner;
        if (outfile->write(&owner, sizeof(owner)) < sizeof(owner))
            return -1;

        m_garrisons[i].m_garrisonArmy.save(outfile);

        count = m_garrisons[i].m_mapX;
        if (outfile->write(&count, sizeof(count)) < sizeof(count))
            return -1;
        count = m_garrisons[i].m_mapY;
        if (outfile->write(&count, sizeof(count)) < sizeof(count))
            return -1;
        count = m_garrisons[i].m_mapZ;
        if (outfile->write(&count, sizeof(count)) < sizeof(count))
            return -1;

        unsigned char last = m_garrisons[i].m_removableTroops;
        outfile->write(&last, sizeof(last));
    }
    return 0;
}

VA(0x004b9a00, 0x239)  // dc 0xa46e8
int game::loadBoatPool(TAbstractFile* infile)
{
    unsigned short ushortBuffer;
    int count;
    int x;
    unsigned char ucharBuffer;
    char charBuffer;

    count = infile->read(&ucharBuffer, sizeof(ucharBuffer));
    if (count < sizeof(ucharBuffer))
        return -1;

    m_boats.resize(ucharBuffer);
    for (x = 0; x < m_boats.size(); ++x) {
        count = infile->read(&charBuffer, sizeof(charBuffer));
        if (count < sizeof(charBuffer))
            return -1;
        m_boats[x].m_allocated = charBuffer != 0;

        count = infile->read(&ucharBuffer, sizeof(ucharBuffer));
        if (count < sizeof(ucharBuffer))
            return -1;
        m_boats[x].m_id = ucharBuffer;

        count = infile->read(&charBuffer, sizeof(charBuffer));
        if (count < sizeof(charBuffer))
            return -1;
        m_boats[x].m_type = charBuffer;
        count = infile->read(&charBuffer, sizeof(charBuffer));
        if (count < sizeof(charBuffer))
            return -1;
        m_boats[x].m_facing = charBuffer;
        count = infile->read(&charBuffer, sizeof(charBuffer));
        if (count < sizeof(charBuffer))
            return -1;
        m_boats[x].m_playerOwner = charBuffer;

        count = infile->read(&ushortBuffer, sizeof(ushortBuffer));
        if (count < sizeof(ushortBuffer))
            return -1;
        m_boats[x].m_occupyingHero = ushortBuffer;

        count = infile->read(&charBuffer, sizeof(charBuffer));
        if (count < sizeof(charBuffer))
            return -1;
        m_boats[x].m_occupied = charBuffer != 0;
        if (!m_boats[x].load(infile))
            return -1;
    }
    return 0;
}

VA(0x004b9c40, 0x1AD)  // dc 0xa4980
int game::saveBoatPool(TAbstractFile* outfile)
{
    unsigned short ushortBuffer;
    int count;
    int x;
    unsigned char ucharBuffer;
    char charBuffer;

    ucharBuffer = static_cast<unsigned char>(m_boats.size());
    count = outfile->write(&ucharBuffer, sizeof(ucharBuffer));
    if (count < sizeof(ucharBuffer))
        return -1;

    for (x = 0; x < m_boats.size(); ++x) {
        charBuffer = m_boats[x].m_allocated;
        count = outfile->write(&charBuffer, sizeof(charBuffer));
        if (count < sizeof(charBuffer))
            return -1;
        ucharBuffer = m_boats[x].m_id;
        count = outfile->write(&ucharBuffer, sizeof(ucharBuffer));
        if (count < sizeof(ucharBuffer))
            return -1;
        charBuffer = m_boats[x].m_type;
        count = outfile->write(&charBuffer, sizeof(charBuffer));
        if (count < sizeof(charBuffer))
            return -1;
        charBuffer = m_boats[x].m_facing;
        count = outfile->write(&charBuffer, sizeof(charBuffer));
        if (count < sizeof(charBuffer))
            return -1;
        charBuffer = m_boats[x].m_playerOwner;
        count = outfile->write(&charBuffer, sizeof(charBuffer));
        if (count < sizeof(charBuffer))
            return -1;

        ushortBuffer = static_cast<unsigned short>(m_boats[x].m_occupyingHero);
        count = outfile->write(&ushortBuffer, sizeof(ushortBuffer));
        if (count < sizeof(ushortBuffer))
            return -1;

        charBuffer = m_boats[x].m_occupied;
        count = outfile->write(&charBuffer, sizeof(charBuffer));
        if (count < sizeof(charBuffer))
            return -1;
        if (!m_boats[x].save(outfile))
            return -1;
    }
    return 0;
}

// E:\gamedcs\game.cpp:1235; original LoadObeliskPool.
// Retail expands this ordinary helper in game::load. Complete routes the
// two reads through TAbstractFile instead of Dreamcast's gzread handle.
// Original locals: count, char_buffer.

int game::loadObeliskPool(TAbstractFile* infile)
{
    char charBuffer;
    int count = readValue(infile, charBuffer);
    if (count < sizeof(charBuffer))
        return -1;
    m_numObelisks = charBuffer;
    count = readValue(infile, m_obeliskFlags);
    if (count < sizeof(m_obeliskFlags))
        return -1;
    return 0;
}

// E:\gamedcs\game.cpp:1256; original SaveObeliskPool.
// The ordinary writer mirrors the reader; retail expands it in game::save.
// Original locals: count, char_buffer.

int game::saveObeliskPool(TAbstractFile* outfile)
{
    char charBuffer = m_numObelisks;
    int count = outfile->write(&charBuffer, sizeof(charBuffer));
    if (count < sizeof(charBuffer))
        return -1;
    count = outfile->write(m_obeliskFlags, sizeof(m_obeliskFlags));
    if (count < sizeof(m_obeliskFlags))
        return -1;
    return 0;
}

VA(0x004b9df0, 0x2D)  // dc 0xa4cc8
playerData::playerData()
{
}

VA(0x004b9e20, 0x115)  // dc 0xa4d58
void playerData::init()
{
    m_numHeroes = 0;
    m_currHeroId = -1;
    m_currTownId = 0;
    m_shipyards.clear();

    m_puzzleGuess.m_x = -1;
    m_puzzleGuess.m_y = -1;
    m_puzzleGuess.m_z = -1;

    m_startingNumHeroes = 0;
    m_mysticalGardenFlags = 0;
    m_magicSpringFlags = 0;
    m_deadGuyFlags = 0;
    m_leanToFlags = 0;
    m_numTowns = 0;
    m_deathCountDown = -1;
    m_extraPuzzlePieces = 0;
    m_recruits[0] = -1;
    m_recruits[1] = -1;
    m_personality = 0;
    memset(&m_ai, 0, sizeof(m_ai));
    int heroIndex;
    MEMSET(m_heroes, -1, sizeof(m_heroes), heroIndex);
    memset(m_townIds, 0xff, sizeof(m_townIds));
    m_isLocal = 0;
    m_isHuman = 0;
    m_quickCombat = 0;
    m_placementHelpEnabled = 1;
    m_assembledCombinations.reset();
    strcpy(m_name, g_generalText->getText(GENERAL_TEXT_DEFAULT_PLAYER_NAME));
    m_dpid = 0;
    m_isHuman = 0;
    m_isLocal = 0;
}

VA(0x004b9f40, 0x71)  // dc 0xa4e80
bool playerData::hasCapitol()
{
    int i = 0;
    int towns = m_numTowns;

    if (towns <= 0)
        return false;
    do {
        if (g_game->getTown(m_townIds[i])->hasBuilding(HALL_CAPITOL_ID, false))
            return true;
    } while (++i < towns);
    return false;
}

VA(0x004b9fc0, 0x167)  // dc 0xa4ee8
unsigned char playerData::addGarrisonHero(town* ourTown)
{
    int i;
    hero* ourHero;
    int found;

    if (ourTown->m_visitingHeroId < 0)
        return 0;
    if (ourTown->m_garrisonHeroId >= 0)
        return 0;

    ourHero = g_game->getHero(ourTown->m_visitingHeroId);
    if (!ourHero->m_army.merge(const_cast<armyGroup*>(
            &static_cast<const town*>(ourTown)->getArmy())))
        return 0;

    g_game->recordHideHero(ourHero, ourHero->m_owner, 0);

    if (g_remoteOn) {
        CMCHideHero hideHero(ourHero->m_id);
        sendMapChange(&hideHero);
    }

    found = findHero(ourHero->m_id);
    ourHero->restoreCell();

    for (i = found; i < m_numHeroes - 1; ++i)
        m_heroes[i] = m_heroes[i + 1];
    m_heroes[m_numHeroes - 1] = -1;

    if (m_currHeroId == ourHero->m_id) {
        m_currHeroId = -1;
        if (g_netLocalGamePos == ourHero->m_owner) {
            g_advManager->m_drawCursor = 0;
            g_advManager->m_curHeroMobile = 0;
        }
    }
    --m_numHeroes;

    ourTown->m_garrisonHeroId = ourHero->m_id;
    ourTown->m_visitingHeroId = -1;
    return 1;
}

// Original: playerData::SetName; game.cpp:1383, dc 0xa50ac.
// AssignNetInfo expands this ordinary bounded-copy helper in Complete.
void playerData::setName(char* newName)
{
    strncpy(m_name, newName, 20);
}

VA(0x004ba130, 0x34)  // dc 0xa5108
void playerData::assignNetInfo(CNetPlayerInfo* netPlayerInfo)
{
    setName(netPlayerInfo->m_name);
    m_dpid = netPlayerInfo->m_dpid;
    m_isHuman = 1;
}

// Original: playerData::GetNetInfo; game.cpp:1395, dc 0xa5138.
void playerData::getNetInfo(CNetPlayerInfo* netPlayerInfo)
{
    strcpy(netPlayerInfo->m_name, m_name);
    netPlayerInfo->m_dpid = m_dpid;
}

VA(0x004ba170, 0x4E)  // dc 0xa5168
void playerData::clearNetInfo()
{
    strcpy(m_name, g_generalText->getText(GENERAL_TEXT_DEFAULT_PLAYER_NAME));
    m_dpid = 0;
    m_isHuman = 0;
    m_isLocal = 0;
}

VA(0x004ba1c0, 0x50)
int __fastcall readHeroId(TAbstractFile* infile, int mapVersion)
{
    unsigned long value;
    infile->read(&value, sizeof(unsigned char));
    int heroId = value & 0xff;
    if (heroId == g_savedHeroNone)
        return -1;
    if (mapVersion == g_mapVersionOldCampaignHeroIds) {
        if (heroId == g_savedHeroPre25First)
            return g_heroPre25FirstRemap;
        if (heroId == g_savedHeroPre25Second)
            return g_heroPre25SecondRemap;
    }
    return heroId;
}

VA(0x004ba210, 0x50)
int __fastcall loadHeroId(TAbstractFile* infile, int saveVersion)
{
    unsigned long value;
    infile->read(&value, sizeof(unsigned char));
    int heroId = value & 0xff;
    if (heroId == g_savedHeroNone)
        return -1;
    if (saveVersion < g_saveVersionCompleteHeroRoster) {
        if (heroId == g_savedHeroPre25First)
            return g_heroPre25FirstRemap;
        if (heroId == g_savedHeroPre25Second)
            return g_heroPre25SecondRemap;
    }
    return heroId;
}

VA(0x004ba260, 0x401)  // dc 0xa51b0
int playerData::load(TAbstractFile* infile, int saveVersion)
{
    char value;
    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    m_color = value;
    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    m_numHeroes = value;

    m_currHeroId = loadHeroId(infile, saveVersion);

    int i;
    for (i = 0; i < 8; i++) {
        m_heroes[i] = loadHeroId(infile, saveVersion);
    }

    for (i = 0; i < 2; i++) {
        m_recruits[i] = loadHeroId(infile, saveVersion);
    }

    unsigned char flag;
    if (infile->read(&flag, sizeof(flag)) < sizeof(flag))
        return -1;
    m_startingNumHeroes = flag;

    int number;
    if (infile->read(&number, sizeof(number)) < sizeof(number))
        return -1;
    m_personality = number;

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    m_extraPuzzlePieces = value;

    if (infile->read(&m_puzzleGuess, sizeof(m_puzzleGuess)) < sizeof(m_puzzleGuess))
        return -1;

    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    m_deathCountDown = value;
    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    m_numTowns = value;
    if (infile->read(&value, sizeof(value)) < sizeof(value))
        return -1;
    m_currTownId = value;

    for (i = 0; i < 0x48; i++) {
        if (infile->read(&value, sizeof(value)) < sizeof(value))
            return -1;
        m_townIds[i] = value;
    }

    for (i = 0; i < 7; i++) {
        if (infile->read(&number, sizeof(number)) < sizeof(number))
            return -1;
        m_resources[i] = number;
    }

    unsigned long flags;
    if (infile->read(&flags, sizeof(flags)) < sizeof(flags))
        return -1;
    m_mysticalGardenFlags = flags;
    if (infile->read(&flags, sizeof(flags)) < sizeof(flags))
        return -1;
    m_magicSpringFlags = flags;
    if (infile->read(&flags, sizeof(flags)) < sizeof(flags))
        return -1;
    m_deadGuyFlags = flags;
    if (infile->read(&flags, sizeof(flags)) < sizeof(flags))
        return -1;
    m_leanToFlags = flags;

    if (infile->read(&flag, sizeof(flag)) < sizeof(flag))
        return -1;
    m_placementHelpEnabled = flag != 0;

    if (saveVersion >= 37) {
        unsigned char bits[2];
        std::bitset<12> combos;
        infile->read(bits, sizeof(bits));
        for (unsigned int bit = 0; bit < 12; bit++)
            combos.set(bit, (bits[bit >> 3] & (1 << (bit & 7))) != 0);
        m_assembledCombinations = combos;
    }
    return 0;
}

// E:\gamedcs\game.cpp:1548
// The write side of load, minus the version remap: a saver only ever
// emits the current format. The tavern pair is written longhand here
// where load loops over it, and the trailing combination-artifact word
// is unconditional.
VA(0x004ba670, 0x36A)  // anchor-global, dc 0xa55a8
int playerData::save(TAbstractFile* outfile)
{
    unsigned long flags;
    int number;
    int count;
    int x;
    unsigned char flag;
    char value;

    value = m_color;
    count = outfile->write(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;
    value = m_numHeroes;
    count = outfile->write(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;
    value = static_cast<char>(m_currHeroId);
    count = outfile->write(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;

    for (x = 0; x < 8; x++) {
        value = static_cast<char>(m_heroes[x]);
        count = outfile->write(&value, sizeof(value));
        if (count < sizeof(value))
            return -1;
    }

    value = static_cast<char>(m_recruits[0]);
    count = outfile->write(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;
    value = static_cast<char>(m_recruits[1]);
    count = outfile->write(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;

    flag = m_startingNumHeroes;
    count = outfile->write(&flag, sizeof(flag));
    if (count < sizeof(flag))
        return -1;

    number = m_personality;
    count = outfile->write(&number, sizeof(number));
    if (count < sizeof(number))
        return -1;

    value = m_extraPuzzlePieces;
    count = outfile->write(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;

    count = outfile->write(&m_puzzleGuess, sizeof(m_puzzleGuess));
    if (count < sizeof(m_puzzleGuess))
        return -1;

    value = m_deathCountDown;
    count = outfile->write(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;
    value = m_numTowns;
    count = outfile->write(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;
    value = m_currTownId;
    count = outfile->write(&value, sizeof(value));
    if (count < sizeof(value))
        return -1;

    for (x = 0; x < 0x48; x++) {
        value = m_townIds[x];
        count = outfile->write(&value, sizeof(value));
        if (count < sizeof(value))
            return -1;
    }

    for (x = 0; x < 7; x++) {
        number = m_resources[x];
        count = outfile->write(&number, sizeof(number));
        if (count < sizeof(number))
            return -1;
    }

    flags = m_mysticalGardenFlags;
    count = outfile->write(&flags, sizeof(flags));
    if (count < sizeof(flags))
        return -1;
    flags = m_magicSpringFlags;
    count = outfile->write(&flags, sizeof(flags));
    if (count < sizeof(flags))
        return -1;
    flags = m_deadGuyFlags;
    count = outfile->write(&flags, sizeof(flags));
    if (count < sizeof(flags))
        return -1;
    flags = m_leanToFlags;
    count = outfile->write(&flags, sizeof(flags));
    if (count < sizeof(flags))
        return -1;

    flag = m_placementHelpEnabled;
    count = outfile->write(&flag, sizeof(flag));
    if (count < sizeof(flag))
        return -1;

    // Residual (99.9557%): all 49 blocks, every branch and every opcode agree.
    // Dreamcast proves the top-scope uint_buffer/int_buffer/count/x/
    // uchar_buffer/char_buffer roster, all twenty named count = Write(...)
    // statements, and reuse of x by the three original loops; those facts are
    // restored above and ratcheted even though VC6 lowers them byte-flat. The
    // remaining operands are one Complete-era stack-home cycle: this compile
    // puts x/flags/bits at -0xc/-0x8/-0x4, retail at -0x8/-0xc/-0x6. The int
    // buffer at -0x10 and both byte buffers are exact. Moving bits to function
    // scope and a const-reference combinations alias are byte-flat; removing
    // the alias for a direct member call falls to 98.2109%.
    // DECLARATION ORDER IS NOT THE LEVER (measured 2026-09-06, both byte-flat
    // at 99.9557): hoisting `int x;` above `unsigned long flags;` and sinking
    // it below `char value;` each leave x at -0xc and flags at -0x8.  The
    // slots follow the variables, not their declaration order - the same
    // result the ai.cpp simulate_combat pair gave, where the lever turned out
    // to be the ORDER OF THE ASSIGNMENT STATEMENTS instead. The flat relocation
    // view also names the same bitset<12>::_Xran callee through a synthetic
    // target label because its retail row is unclaimed.
    unsigned char bits[2];
    const std::bitset<12>* combinations = &m_assembledCombinations;
    unsigned int bit = 0;
    memset(bits, 0, sizeof(bits));
    for (; bit < 12; bit++) {
        if (combinations->test(bit))
            bits[bit >> 3] |= static_cast<unsigned char>(1 << (bit & 7));
    }
    outfile->write(bits, sizeof(bits));
    return 0;
}

// Original: game::LoadPlayerData; game.cpp:1668, dc 0xa5998.
// Complete passes the saved version to each playerData::load in the
// expansion at game::load +0x51f. DC's ordinary helper has x and err locals
// and returns the element error; the caller maps a negative result to -1.
int game::loadPlayerData(TAbstractFile* infile, int saveVersion)
{
    for (int x = 0; x < 8; ++x) {
        int err = m_players[x].load(infile, saveVersion);
        if (err < 0)
            return err;
    }
    return 0;
}

// Original: game::SavePlayerData; game.cpp:1683, dc 0xa59ec.
// Ordinary helper expanded at game::save +0x38f; eight player records.
int game::savePlayerData(TAbstractFile* outfile)
{
    for (int x = 0; x < 8; ++x) {
        int err = m_players[x].save(outfile);
        if (err < 0)
            return err;
    }
    return 0;
}

// E:\gamedcs\game.cpp:1698
// Original LoadTownPool; uchar_buffer -> townCount. Complete passes the save
// version to town::load. DC returns the element error unchanged, while the
// game::load caller maps any negative result to -1.

int game::loadTownPool(TAbstractFile* infile, int saveVersion)
{
    unsigned char townCount;
    int count = readValue(infile, townCount);
    if (count < sizeof(townCount))
        return -1;
    m_towns.resize(townCount);
    for (int x = 0; x < m_towns.size(); ++x) {
        int err = m_towns[x].load(infile, saveVersion);
        if (err < 0)
            return err;
    }
    return 0;
}

// E:\gamedcs\game.cpp:1722
// Original SaveTownPool; uchar_buffer -> townCount. Keep the DC vector-size
// loop and element error return; ordinary inlining replaces the copied loop
// and its pinned condition in game::save.

int game::saveTownPool(TAbstractFile* outfile)
{
    unsigned char townCount = m_towns.size();
    int count = outfile->write(&townCount, sizeof(townCount));
    if (count < sizeof(townCount))
        return -1;
    for (int x = 0; x < m_towns.size(); ++x) {
        int err = m_towns[x].save(outfile);
        if (err < 0)
            return err;
    }
    return 0;
}

// Original: game::SaveHeroPool; game.cpp:1745, dc 0xa5b9c.
// Ordinary helper expanded at game::save +0x45a. Complete writes all156
// hero records; the older pressing's pool was128.
int game::saveHeroPool(TAbstractFile* outfile)
{
    // Complete's 156-entry loop uses an unsigned bound (`jb`) in its
    // expansion inside game::save; the older Dreamcast roster had 128 heroes.
    for (unsigned int x = 0; x < HERO_COUNT; ++x) {
        int err = m_heroes[x].save(outfile);
        if (err < 0)
            return err;
    }
    return 0;
}

// Original: game::LoadHeroPool; game.cpp:1760, dc 0xa5bf4.
// Complete's game::load +0x622 tests saveVersion against25, chooses128 or
//156 records, and passes that version to hero::load at +0x658.
int game::loadHeroPool(TAbstractFile* infile, int saveVersion)
{
    int heroCount = HERO_COUNT;
    if (saveVersion < g_saveVersionCompleteHeroRoster)
        heroCount = g_mapHeaderLegacyHeroCount;
    for (int x = 0; x < heroCount; ++x) {
        int err = m_heroes[x].load(infile, saveVersion);
        if (err < 0)
            return err;
    }
    return 0;
}

VA(0x004ba9e0, 0x2D)  // dc 0xa5c4c
int playerData::findHero(int id) const
{
    if (id != -1) {
        for (int i = 0; i < m_numHeroes; i++) {
            if (id == m_heroes[i])
                return i;
        }
    }
    return -1;
}

VA(0x004baa10, 0x2E)  // dc 0xa5c98
int playerData::findTown(int id) const
{
    if (id != -1) {
        for (int i = 0; i < m_numTowns; i++) {
            if (id == m_townIds[i])
                return i;
        }
    }
    return -1;
}

VA(0x004baa40, 0xFA)  // dc 0xa5d10
int playerData::nextHero()
{
    int cur = findHero(m_currHeroId);

    for (int i = cur + 1; i < m_numHeroes; i++) {
        hero* h = g_game->getHero(m_heroes[i]);
        if (h->isMobile() && !h->m_isSleeping)
            return m_heroes[i];
    }
    for (int j = 0; j <= cur; j++) {
        hero* h = g_game->getHero(m_heroes[j]);
        if (h->isMobile() && !h->m_isSleeping)
            return m_heroes[j];
    }
    return -1;
}

VA(0x004bab40, 0x43)  // dc 0xa5e0c
int playerData::nextTown()
{
    if (m_numTowns > 0) {
        if (g_currentPlayer->m_currTownId == -1)
            return m_townIds[0];
        for (int i = 0; i < m_numTowns; i++) {
            if (g_currentPlayer->m_currTownId == m_townIds[i])
                return m_townIds[(i + 1) % m_numTowns];
        }
    }
    return -1;
}

VA(0x004bab90, 0x10)  // dc 0xa5eb0
bool playerData::hasMobileHero()
{
    return nextHero() != -1;
}

VA(0x004baba0, 0x29)  // dc 0xa5ee8
int getNumObelisks(int whichPlayer)
{
    int numFound = 0;

    for (int i = 0; i < 48; i++) {
        if (g_game->m_obeliskFlags[i] & (1 << whichPlayer))
            numFound++;
    }
    return numFound;
}

// Original: playerData::BuildingsOwned; game.cpp:1873, dc 0xa5f30
int playerData::buildingsOwned(int townType, int buildingId, int mageLevel)
{
    int count = 0;
    for (int i = 0; i < m_numTowns; ++i) {
        town* currentTown = &g_game->m_towns[m_townIds[i]];
        if (buildingId < DWELLING_0_ID || currentTown->m_type == townType) {
            if (buildingId == MAGE_GUILD_ID) {
                if (currentTown->hasBuilding(MAGE_GUILD_ID, false)
                    && currentTown->m_mageLevel == mageLevel)
                    ++count;
            } else if (currentTown->hasBuilding(buildingId, false)) {
                ++count;
            }
        }
    }
    return count;
}

VA(0x004babd0, 0xDC)  // dc 0xa5ff0
int playerData::numOfGivenArtifact(int whichArtifact) const
{
    int count = 0;

    for (int heroIndex = 0; heroIndex < m_numHeroes; heroIndex++) {
        hero* currentHero = g_game->getHero(m_heroes[heroIndex]);
        for (int slot = 0; slot < 19; slot++) {
            if (currentHero->getArtifact(TArtifactSlot(slot)).m_artifactId == whichArtifact)
                count++;
        }
    }

    for (int townIndex = 0; townIndex < m_numTowns; townIndex++) {
        town* currentTown = g_game->getTown(m_townIds[townIndex]);
        if (currentTown->m_garrisonHeroId >= 0) {
            hero* currentHero = g_game->getHero(currentTown->m_garrisonHeroId);
            for (int slot = 0; slot < 19; slot++) {
                if (currentHero->getArtifact(TArtifactSlot(slot)).m_artifactId == whichArtifact)
                    count++;
            }
        }
    }

    return count;
}

VA(0x004bacb0, 0xCA)  // hd-crossbuild + anchor-callee
bool playerData::hasGivenArtifact(int artifact)
{
    for (int heroIndex = 0; heroIndex < m_numHeroes; heroIndex++) {
        hero* currentHero = g_game->getHero(m_heroes[heroIndex]);
        if (currentHero->isWieldingArtifact(artifact))
            return true;
    }

    for (int townIndex = 0; townIndex < m_numTowns; townIndex++) {
        town* currentTown = g_game->getTown(m_townIds[townIndex]);
        if (currentTown->m_garrisonHeroId >= 0) {
            hero* currentHero = g_game->getHero(currentTown->m_garrisonHeroId);
            if (currentHero->isWieldingArtifact(artifact))
                return true;
        }
    }

    return false;
}

VA(0x004bad80, 0x1A)  // dc 0xa6114
bool playerData::isLocalHuman() const
{
    if (m_isHuman && m_isLocal)
        return true;
    return false;
}

VA(0x004bada0, 0xC)  // dc 0xa6144
bool playerData::isHuman() const
{
    return m_isHuman ? true : false;
}

VA(0x004badb0, 0x9C)  // dc 0xa6180
char* playerData::getName()
{
    if ((!m_isHuman && _strcmpi(m_name, g_generalText->getText(
            GENERAL_TEXT_DEFAULT_PLAYER_NAME)) == 0) ||
        (m_isHuman && _strcmpi(m_name, DATA_COMPGEN(0x00677d30, defaultHumanName, "Player")) == 0)) {
        strcpy(m_name, g_colors[m_color]);
    }
    m_name[0] = toupper(m_name[0]);
    return m_name;
}

VA(0x004bae50, 0x1B)  // dc 0xa6230
void playerData::guessGrailLocation(long playerId)
{
    type_point guess = aiAttemptPuzzleGuess(playerId);
    m_puzzleGuess = guess;
}

VA(0x004bae70, 0x55)  // dc 0xa6274
int game::mineTypesOwned(int whichPlayer, int mineType)
{
    int count = 0;

    for (unsigned i = 0; i < m_mines.size(); i++) {
        if (m_mines[i].m_playerOwner == whichPlayer && m_mines[i].m_type == mineType)
            count++;
    }
    return count;
}

VA(0x004baed0, 0x2C)  // dc 0xa6328
void computeUALoc(int whichPlayer)
{
    g_game->m_players[whichPlayer].guessGrailLocation(whichPlayer);
}

// E:\gamedcs\game.cpp:1999
// One puzzle piece per obelisk visited, plus a share of the
// 48 - numObelisks pieces that no obelisk pays for, awarded on the
// quadratic curve ((p + 1) * p) / 2 in p, the fraction of obelisks found.
// countOnly stops at the count; otherwise the shared bitset is re-rolled
// from a seed derived from whichPlayer alone, so a player always uncovers
// the same pieces in the same order.

// GetNumObelisks is expanded THREE times by /Ob2 - three 48-iteration
// scans of obeliskFlags - because retail calls it three times in source,
// not because a CSE failed. The Dreamcast dump names every local here
// (dc 0xa6350): iPiecesRemoved, iExtraPieces, piece, i, j and the two
// floats. `47 - i` is an induction expression, not a variable, which is
// why VC6 carries it as a second down-counter. The fild/fstp/fld
// round-trips on each int->float conversion are /Op rounding, not extra
// source variables.
// Residual (98.9637%): two instructions, and it is /Op scheduling. Retail
// interleaves the numerator's `fld` BETWEEN the two int->float
// round-trips of the division; we emit both round-trips and then the
// load. Measured three cast placements - on the numerator, on the
// denominator, on both - all 98.9637, so the order is not source-
// reachable. The remaining rows are relocation-symbol presentation:
// gpGame and the float pools are unnamed on the target side, while the
// candidate's puzzlePiecesRemoved+4 and target's synthetic bss_2976ec+0
// denote the same second dword. Do not model that delinker split in C++.
// The 2026-09-01 structure pass reconfirmed 43/43 exact blocks; why-reg v2
// diagnoses no register-binding divergence, independently closing the
// B-family search without disturbing the recovered local roster.
// Further float-lifetime controls: split percentage assignment/division,
// explicit float casts and a named numerator all remain 98.9637%; a named
// denominator gives 95.5130% (six states, four emitted objects).
VA(0x004baf00, 0x25A)  // linkorder, dc 0xa6350
int game::setupPuzzlePieces(int whichPlayer, int countOnly)
{
    long piece;
    float percentObelisksFound;
    float percentExtraPieces;
    int i;
    long j;
    int extraPieces;
    int piecesRemoved;

    piecesRemoved = getNumObelisks(whichPlayer);
    extraPieces = g_obeliskCount - m_numObelisks;
    percentObelisksFound =
        static_cast<float>(getNumObelisks(whichPlayer)) / m_numObelisks;
    percentExtraPieces =
        (percentObelisksFound + 1.0f) * percentObelisksFound / 2.0f;
    piecesRemoved = static_cast<int>(
        piecesRemoved + extraPieces * percentExtraPieces);
    if (getNumObelisks(whichPlayer) == m_numObelisks)
        piecesRemoved = g_obeliskCount;

    piecesRemoved += m_players[whichPlayer].m_extraPuzzlePieces;
    if (piecesRemoved > g_obeliskCount)
        piecesRemoved = g_obeliskCount;
    if (!m_numObelisks)
        piecesRemoved = 0;
    if (countOnly)
        return piecesRemoved;

    if (piecesRemoved == g_obeliskCount) {
        g_puzzlePiecesRemoved.set();
        return piecesRemoved;
    }
    g_puzzlePiecesRemoved.reset();

    sRand(whichPlayer * 424909 + 423869);

    for (i = 0; i < piecesRemoved; i++) {
        piece = 0;
        while (piece < g_puzzlePlaceablePieces && g_puzzlePiecesRemoved[piece])
            piece += random(1, 5);
        if (piece >= g_puzzlePlaceablePieces) {
            j = random(1, g_puzzlePlaceablePieces - i);
            for (piece = 0; piece < g_puzzlePlaceablePieces; piece++) {
                if (!g_puzzlePiecesRemoved[piece]) {
                    if (--j == 0)
                        break;
                }
            }
        }
        g_puzzlePiecesRemoved.set(piece);
    }
    return piecesRemoved;
}

VA(0x004bb160, 0x7)  // dc 0xa65c4
NewfullMap* game::getWorldMapData()
{
    return &m_worldMap;
}

VA(0x004bb170, 0xD6)  // dc 0xa65d4
int game::getNewBoatId()
{
    unsigned int i;
    for (i = 0; i < m_boats.size(); i++) {
        if (!m_boats[i].m_allocated)
            return i;
    }

    if (m_boats.size() < 64) {
        boat newBoat;
        m_boats.push_back(newBoat);
        return m_boats.size() - 1;
    }
    return -1;
}

VA(0x004bb250, 0x1AA)  // dc 0xa6690
int game::createBoat(int x, int y, int z, int owner, unsigned char isRemoteMove, signed char type)
{
    int id = getNewBoatId();
    if (id == -1)
        return -1;

    boat& thisBoat = m_boats[id];
    if (!isRemoteMove) {
        type_point location(x, y, z);
        CMCBuildBoat change(location, g_netLocalGamePos);
        sendMapChange(&change);
        recordShowBoat(&thisBoat, location);
    }

    thisBoat.initialize();
    thisBoat.m_type = type;
    thisBoat.m_x = x;
    thisBoat.m_y = y;
    thisBoat.m_z = z;
    thisBoat.m_id = static_cast<unsigned char>(id);
    thisBoat.m_allocated = 1;
    thisBoat.m_facing = 2;
    thisBoat.m_playerOwner = owner;
    thisBoat.m_occupyingHero = -1;
    thisBoat.m_occupied = 0;
    thisBoat.obscureCell();
    return id;
}

// Original: game::Scan; game.cpp:2158, dc 0xa67bc
int game::scan(signed char* whichList, int start, int length)
{
    for (int i = start; i < start + length; ++i) {
        if (whichList[i] == -1)
            return i;
    }
    return -1;
}

// Original: game::RandomScan; game.cpp:2173, dc 0xa6850
int game::randomScan(signed char* whichList, int start, int length,
                     signed char scanValue)
{
    TPickANumber picker(start, start + length - 1);
    int id;
    do {
        id = picker.pick();
    } while (whichList[id] != scanValue && id >= start);
    return id >= start ? id : -1;
}

VA(0x004bb400, 0x1DC)  // dc 0xa68d8
int game::getStartingHeroId(int alignment, int playerPos, int mapPosition)
{
    int heroArray[HERO_COUNT];
    THeroClass heroClass1 = classKnight;
    THeroClass heroClass2 = classCleric;

    switch (alignment) {
    case TOWN_CASTLE:
        heroClass1 = classKnight;
        heroClass2 = classCleric;
        break;
    case TOWN_RAMPART:
        heroClass1 = classDruid;
        heroClass2 = classRanger;
        break;
    case TOWN_TOWER:
        heroClass1 = classWizard;
        heroClass2 = classAlchemist;
        break;
    case TOWN_INFERNO:
        heroClass1 = classPagan;
        heroClass2 = classHeretic;
        break;
    case TOWN_NECROPOLIS:
        heroClass1 = classDeathKnight;
        heroClass2 = classNecromancer;
        break;
    case TOWN_DUNGEON:
        heroClass1 = classOverlord;
        heroClass2 = classWarlock;
        break;
    case TOWN_STRONGHOLD:
        heroClass1 = classBarbarian;
        heroClass2 = classBattleMage;
        break;
    case TOWN_FORTRESS:
        heroClass1 = classBeastmaster;
        heroClass2 = classWitch;
        break;
    case TOWN_CONFLUX:
        heroClass1 = classPlanesWalker;
        heroClass2 = classElementalist;
        break;
    }

    int top = 0;
    int heroIndex;
    for (heroIndex = 0; heroIndex < HERO_COUNT; heroIndex++) {
        if (m_heroAvailability[heroIndex] == -1
            && m_heroPoolMap[heroIndex].test(playerPos)
            && (m_heroes[heroIndex].m_heroClass == heroClass1
                || m_heroes[heroIndex].m_heroClass == heroClass2)) {
            heroArray[top++] = heroIndex;
        }
    }

    if (top == 0) {
        for (heroIndex = 0; heroIndex < HERO_COUNT; heroIndex++) {
            if (m_heroAvailability[heroIndex] == -1
                && m_heroPoolMap[heroIndex].test(playerPos)) {
                heroArray[top++] = heroIndex;
            }
        }
    }

    return heroArray[random(1, top) - 1];
}

// E:\gamedcs\game.cpp:2275
// Mac counterpart 0:0xce398 has the same five ordered bzero/bitset/random
// calls; the 18-class and 156-hero loops identify its full 1932-byte span.
// Restoring DC's THeroClass induction local raises CodeWarrior O3 20.96% to
// 27.59% without changing Windows 98.98% or its exact CFG/call sequence. O2
// falls to 10.30%, O4 and swapping array declarations are flat. The Windows
// residual is register homing at the Complete-only Conflux guard. Mac's
// bitset<8>::reference layout stages the pool pointer and index before test;
// restoring operator[] at both sites raises Mac to 539/1932 (27.90%) while
// the focused VC6 build stays 98.98% with the same exact CFG and call order.
VA(0x004bb5e0, 0x282)  // anchor-global, dc 0xa6cd4
int game::getNewHeroId(int playerPos, THeroClass excluded,
                       unsigned char preferAlignment,
                       THeroClass preferredClass)
{
    THeroClass heroClass;
    long totalCount;
    long choice = 0;
    long counts[18];
    int heroId;
    long weights[18];
    long alignedCount;

    totalCount = 0;

    int alignment;
    if (playerPos >= 0)
        alignment = m_setup.m_alignment[playerPos];
    else
        alignment = -1;

    memset(counts, 0, sizeof(counts));
    for (heroClass = classKnight; heroClass < kNumHeroClasses;
         heroClass = THeroClass(heroClass + 1)) {
        weights[heroClass] =
            g_heroClasses[heroClass].m_foundInTownType[alignment];
    }

    for (heroId = 0; heroId < HERO_COUNT; heroId++) {
        if (m_heroAvailability[heroId] == -1
            && (playerPos == -1 || m_heroPoolMap[heroId][playerPos])) {
            totalCount++;
            counts[m_heroes[heroId].m_heroClass]++;
        }
    }

    if (totalCount == 0)
        return -1;

    for (heroClass = classKnight; heroClass < kNumHeroClasses;
         heroClass = THeroClass(heroClass + 1)) {
        if (counts[heroClass] == 0)
            weights[heroClass] = 0;
    }

    if (g_game->m_gameVersion >= 2
        && *g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_LOW
        && alignment != TOWN_CONFLUX
        && counts[classPlanesWalker] + counts[classElementalist]
            < totalCount) {
        if (preferredClass != classPlanesWalker)
            weights[classPlanesWalker] = 0;
        if (preferredClass != classElementalist)
            weights[classElementalist] = 0;
    }

    if (excluded < kNumHeroClasses
        && counts[excluded] < totalCount) {
        weights[excluded] = 0;
    }

    if (preferAlignment) {
        alignedCount = 0;
        for (heroClass = classKnight; heroClass < kNumHeroClasses;
             heroClass = THeroClass(heroClass + 1)) {
            if (g_heroClasses[heroClass].m_townType == alignment)
                alignedCount += weights[heroClass];
        }
        if (alignedCount > 0) {
            for (heroClass = classKnight; heroClass < kNumHeroClasses;
                 heroClass = THeroClass(heroClass + 1)) {
                if (g_heroClasses[heroClass].m_townType != alignment)
                    weights[heroClass] = 0;
            }
        }
    }

    if (preferredClass != kNumHeroClasses && weights[preferredClass] != 0) {
        heroClass = preferredClass;
    } else {
        totalCount = 0;
        for (heroClass = classKnight; heroClass < kNumHeroClasses;
             heroClass = THeroClass(heroClass + 1)) {
            totalCount += weights[heroClass];
        }
        choice = random(1, totalCount);
        for (heroClass = classKnight; heroClass < kNumHeroClasses;
             heroClass = THeroClass(heroClass + 1)) {
            choice -= weights[heroClass];
            if (choice <= 0)
                break;
        }
    }

    choice = random(1, counts[heroClass]);
    for (heroId = 0; heroId < HERO_COUNT; heroId++) {
        if (m_heroAvailability[heroId] == -1
            && (playerPos == -1 || m_heroPoolMap[heroId][playerPos])
            && m_heroes[heroId].m_heroClass == heroClass
            && --choice == 0) {
            return heroId;
        }
    }
    return -1;
}

VA(0x004bb870, 0x89)  // dc 0xa6fd4
int game::getTownId(int x, int y, int z)
{
    for (unsigned i = 0; i < m_towns.size(); i++) {
        if (m_towns[i].m_mapX == x && m_towns[i].m_mapY == y && m_towns[i].m_mapZ == z)
            return i;
    }
    return -1;
}

// Original: game::GetHeroId; game.cpp:2390, dc 0xa707c
int game::getHeroId(type_point heroLocation)
{
    for (int i = 0; i < HERO_COUNT; ++i) {
        if (m_heroes[i].getLocation() == heroLocation)
            return i;
    }
    return -1;
}

// Original GetMineId, game.cpp:2405, dc 0xa710c. The ordinary helper
// scans x/y/z in that order and returns -1 after exhausting the mine pool.
// Complete expands it in RandomizeEvents's LIGHTHOUSE and MINE arms.
int game::getMineId(int x, int y, int z)
{
    int i;
    for (i = 0; i < m_mines.size(); ++i) {
        if (m_mines[i].m_mapX == x && m_mines[i].m_mapY == y
            && m_mines[i].m_mapZ == z)
            return i;
    }
    return -1;
}

VA(0x004bb900, 0x87)  // dc 0xa71b4
int game::getGeneratorId(int x, int y, int z)
{
    for (unsigned i = 0; i < m_generators.size(); i++) {
        if (m_generators[i].m_mapX == x && m_generators[i].m_mapY == y &&
            m_generators[i].m_mapZ == z)
            return i;
    }
    return -1;
}

VA(0x004bb990, 0x1CF)
int __fastcall game::loadString(TAbstractFile* infile, std::string& s)
{
    int count;
    short length;

    count = infile->read(&length, sizeof(length));
    if (count < sizeof(length))
        return -1;

    if (length > 0) {
        char* buffer = new char[length + 1];
        memset(buffer, 0, length + 1);
        count = infile->read(buffer, length);
        if (count < length)
            return -1;
        s = buffer;
        delete[] buffer;
    } else {
        s.erase();
    }

    return length;
}

// Original: game::GetGarrisonId; game.cpp:2433, dc 0xa7278
int game::getGarrisonId(int x, int y, int z)
{
    for (int i = 0; i < m_garrisons.size(); ++i) {
        if (m_garrisons[i].m_mapX == x && m_garrisons[i].m_mapY == y
            && m_garrisons[i].m_mapZ == z)
            return i;
    }
    return -1;
}

// Original: GenerateStandardFileName; game.cpp:2448, dc 0xa7320
// DC retains this ordinary utility without a recorded named caller. Keep its
// source operation without assigning it a retail address or inventing a use.
void generateStandardFileName(char* longName, char* retName)
{
    char* period = strrchr(longName, '.');
    if (!period) {
        strcpy(retName, longName);
        return;
    }
    *period = 0;
    int charCount = 0;
    int length = strlen(longName);
    for (int i = 0; i < length; ++i) {
        char current = longName[i];
        current = toupper(current);
        if ((current >= 'A' && current <= 'Z')
            || (current >= '0' && current <= '9') || current == '_') {
            retName[charCount] = current;
            ++charCount;
        }
        // DC line2482 assigns 999 to i (literal pool0xa74b2), then the
        // ordinary for increment and length test finish the short-name scan.
        if (charCount >= 8)
            i = 999;
    }
    *period = '.';
    strcpy(retName + charCount, period);
}

VA(0x004bbb60, 0xBB)  // dc 0xa750c
int __fastcall game::saveString(TAbstractFile* outfile, std::string& s)
{
    HOMM3_RELEASE_VERIFY(outfile != 0);
    int count;
    short length = s.length();

    count = outfile->write(&length, sizeof(length));
    if (count < sizeof(length))
        return -1;

    if (length > 0) {
        char* buffer = new char[length + 1];
        memset(buffer, 0, length + 1);
        strcpy(buffer, s.c_str());
        count = outfile->write(buffer, length);
        if (count < length)
            return -1;
        delete[] buffer;
    }

    return length;
}

VA(0x004bbc20, 0x21E)  // dc 0xa75d0
int game::saveRumours(TAbstractFile* outfile)
{
    unsigned char boolBuffer;
    std::basic_string<char, std::char_traits<char>, std::allocator<char> >
        currentRumour(m_currentRumour);
    int saveResult = saveString(outfile, currentRumour);
    if (0 > saveResult)
        return -1;

    if (outfile->write(m_rumourState, sizeof(m_rumourState)) < sizeof(int))
        return -1;

    int rumourListSize = m_rumours.size();
    if (outfile->write(&rumourListSize, sizeof(rumourListSize))
        < sizeof(rumourListSize))
        return -1;

    for (TRumour* rit = m_rumours.begin(); rit != m_rumours.end(); ++rit) {
        if (saveString(outfile, rit->m_text) < 0)
            return -1;
        boolBuffer = rit->m_unavailable;
        if (outfile->write(&boolBuffer, sizeof(boolBuffer))
            < sizeof(boolBuffer))
            return -1;
    }
    return 1;
}

VA(0x004bbe40, 0x294)  // dc 0xa77c8
int game::loadRumours(TAbstractFile* infile)
{
    unsigned char value;
    std::basic_string<char, std::char_traits<char>, std::allocator<char> >
        current;
    if (loadString(infile, current) < 0)
        return -1;

    strcpy(m_currentRumour, current.c_str());
    if (infile->read(m_rumourState, sizeof(m_rumourState)) < sizeof(int))
        return -1;

    int count;
    if (infile->read(&count, sizeof(count)) < sizeof(count))
        return -1;

    m_rumours.resize(count);
    for (TRumour* it = m_rumours.begin(); it != m_rumours.end(); ++it) {
        if (loadString(infile, it->m_text) < 0)
            return -1;
        if (infile->read(&value, sizeof(value)) < sizeof(value))
            return -1;
        it->m_unavailable = value != 0;
    }
    return 1;
}

// E:\gamedcs\game.cpp:2975
// Rebuilds the per-player shipyard indices after loading a map. Dreamcast's
// line table constructs location before the two obscurer pointers and names
// vector::push_back directly. With that source order, VC6 expands clear and
// push_back exactly while preserving the hero/boat obscureCell wrappers.
VA(0x004bcb30, 0x26C)  // sole caller game::Load, dc 0xa8144
void game::setupShipyards()
{
    type_point location;
    hero* obscuringHero = 0;
    boat* obscuringBoat = 0;
    long i;
    for (i = 0; i < 8; ++i) {
        m_players[i].m_shipyards.clear();
    }

    for (location.m_z = 0;
         location.m_z < g_game->m_worldMap.getNumLevels();
         ++location.m_z) {
        for (location.m_y = 0; location.m_y < g_mapWidth; ++location.m_y) {
            for (location.m_x = 0; location.m_x < g_mapHeight; ++location.m_x) {
                NewmapCell* mapCell = g_game->m_worldMap.cell(location);

                if (mapCell->m_type == HERO) {
                    obscuringHero = g_game->getHero(mapCell->m_extraInfo);
                    obscuringHero->restoreCell();
                }
                if (mapCell->m_type == BOAT) {
                    obscuringBoat = g_game->getBoat(mapCell->m_extraInfo);
                    obscuringBoat->restoreCell();
                }

                ShipyardInfo* shipyardInfo =
                    static_cast<ShipyardInfo*>(
                        static_cast<void*>(&mapCell->m_extraInfo));
                if (mapCell->m_type == SHIPYARD && mapCell->m_isTrigger &&
                    shipyardInfo->m_owner >= 0) {
                    m_players[shipyardInfo->m_owner].m_shipyards.push_back(
                        location);
                }

                if (obscuringHero) {
                    obscuringHero->obscureCell();
                    obscuringHero = 0;
                }
                if (obscuringBoat) {
                    obscuringBoat->obscureCell();
                    obscuringBoat = 0;
                }
            }
        }
    }
}

// E:\gamedcs\game.cpp:2654.
int game::saveBlackMarkets(TAbstractFile* outfile)
{
    char blackMarketListSize = m_blackMarkets.size();
    int count = outfile->write(&blackMarketListSize, sizeof(blackMarketListSize));
    if (count < sizeof(blackMarketListSize))
        return -1;
    count = outfile->write(&m_blackMarkets[0], blackMarketListSize * sizeof(TBlackMarket));
    if (count < blackMarketListSize * sizeof(TBlackMarket))
        return -1;
    return 0;
}

// E:\gamedcs\game.cpp:2672
// Original LoadBlackMarkets; black_market_list_size -> blackMarketListSize.
// DC calls clear, resize and operator[]. The ordinary helper restores one
// caller cleanup boundary; its natural expansion needs no inline-depth pin.

int game::loadBlackMarkets(TAbstractFile* infile)
{
    m_blackMarkets.clear();
    char blackMarketListSize;
    int count = readValue(infile, blackMarketListSize);
    if (count < sizeof(blackMarketListSize))
        return -1;
    m_blackMarkets.resize(blackMarketListSize);
    count = readValues(infile, &m_blackMarkets[0], blackMarketListSize);
    if (count < blackMarketListSize * sizeof(TBlackMarket))
        return -1;
    return 0;
}

// E:\gamedcs\game.cpp:2698; original load_vector / dest_vector.
// The point, long and university instances share this source template.
// DC 0xc19e8/0xc1a68/0xc1ae8 has one source boundary, a short count,
// resize(count), subscript(0), and two guarded reads. The S_PUB32 names
// prove a bool result and vector reference; Complete uses TAbstractFile.
// Retail Load's gate-pair arm zeroes its fill before resize:
// that is the native long default, not the old point-vector pointer union.
// Complete's native count reader preserves the short output and byte count.
// The point and long calls expand in game::load; the university call remains
// the caller's 98.1693% residual. Direct payload-result returns alter the
// retained creature-bank reader's comparison, so keep the two DC guards.
// The generic university fill stays an aggregate; its elemental-school
// initializer belongs only to the Conflux consumers (see type_university).
template <class T>
bool loadVector(TAbstractFile* infile, std::vector<T>& destVector)
{
    short count;
    if (readValue(infile, count) < sizeof(count))
        return false;
    destVector.resize(count);
    if (readValues(infile, &destVector[0], count) < count * sizeof(T))
        return false;
    return true;
}

// E:\gamedcs\game.cpp:2716; original save_vector / src_vector.
// Complete writes two bytes of an int slot, then uses its signed-short value.
// The guarded return reproduces both retained 96-byte writers exactly,
// including SETAE. Direct boolean/byte-local returns instead use SBB/INC;
// that spelling difference does not refute the DC bool/reference signature.
// The point/long writer, resize, size, _Ucopy and _Ufill instances emit
// identical raw code; retail's folded calls do not require a type adapter.
template <class T>
bool saveVector(TAbstractFile* outfile, std::vector<T>& srcVector)
{
    int count = srcVector.size();
    if (outfile->write(&count, sizeof(short)) < sizeof(short))
        return false;
    if (outfile->write(&srcVector[0], static_cast<short>(count) * sizeof(T))
        < static_cast<short>(count) * sizeof(T))
        return false;
    return true;
}

// Original: load_object_vector; game.cpp:2733, dc 0xc1950 / 0xc1b6c.
// Both DC instantiations have the same source rows and reference interface.
// Raw public YA_N proves bool; the primitive type display uses a byte alias.
// Complete uses TAbstractFile in place of the DC gz handle; generator calls
// expand, while the creature-bank instantiation is retained at 0x4d2870.
template <class T>
bool loadObjectVector(TAbstractFile* infile, std::vector<T>& destVector)
{
    short count;
    if (readValue(infile, count) < sizeof(count))
        return 0;
    destVector.resize(count);
    for (long i = 0; i < count; ++i) {
        if (!destVector[i].load(infile))
            return 0;
    }
    return 1;
}

// Original: save_object_vector; game.cpp:2754, dc 0xc1d38 / 0xc1f64.
// The matching reference writer preserves each record's own save boundary.
// Both raw public symbols return bool (YA_N), like the record members (QAA_N).
template <class T>
bool saveObjectVector(TAbstractFile* outfile, std::vector<T>& srcVector)
{
    short count = srcVector.size();
    if (outfile->write(&count, sizeof(count)) < sizeof(count))
        return 0;
    for (long i = 0; i < count; ++i) {
        if (!srcVector[i].save(outfile))
            return 0;
    }
    return 1;
}

// E:\gamedcs\game.cpp:2774
// Retail inlines this record reader into load_object_vector. The fixed
// bands are the 0x38-byte army, seven 4-byte resources, the creature id and
// reward count; DC2783 then delegates the short-count artifact tail to
// load_vector. Keeping that nested boundary also bounds the ordinary member
// before VC6 chooses its expansion into loadObjectVector.
bool type_creature_bank::load(void* input)
{
    TAbstractFile* infile = static_cast<TAbstractFile*>(input);

    if (infile->read(&m_guards, sizeof(m_guards)) != sizeof(m_guards))
        return 0;
    if (infile->read(m_resources, sizeof(m_resources)) != sizeof(m_resources))
        return 0;
    if (infile->read(&m_rewardCreature, sizeof(m_rewardCreature)) !=
        sizeof(m_rewardCreature))
        return 0;
    if (infile->read(&m_rewardCreatures, sizeof(m_rewardCreatures)) !=
        sizeof(m_rewardCreatures))
        return 0;
    return loadVector(infile, m_artifacts);
}

// Original: type_creature_bank::save; game.cpp:2790, dc 0xa7b60.
// DC2791..2794 write the four fixed bands without checking each result;
// DC2795 returns save_vector for the artifact tail. Complete expands this
// ordinary member in its retained saveObjectVector<type_creature_bank>.
bool type_creature_bank::save(void* output)
{
    TAbstractFile* outfile = static_cast<TAbstractFile*>(output);
    outfile->write(&m_guards, sizeof(m_guards));
    outfile->write(m_resources, sizeof(m_resources));
    outfile->write(&m_rewardCreature, sizeof(m_rewardCreature));
    outfile->write(&m_rewardCreatures, sizeof(m_rewardCreatures));
    return saveVector(outfile, m_artifacts);
}

// Complete reading belongs to SavedGameHeader::load. The caller tests
// its result before restoring any game state and retains the snapshot for
// later version tests. This ordinary application phase owns the demonstrated
// g_game/global field transfers. Its name and free-function binding are
// inferred; no Dreamcast identity or retained address is asserted.
void applySavedGameHeader(const SavedGameHeader& saved)
{
    // Every store in this block goes through gpGame, RELOADED from the
    // global for each one, not through the implicit `this` retail
    // already has in a register: `mov ecx,[gpGame] / mov [ecx+0x1f698],
    // edx`, then `mov ecx,[gpGame]` again for mapHeader, again for
    // setup, again for campaign, again for the filename. Do not cache
    // what retail reloads.
    g_game->m_gameVersion = saved.m_gameVersion;
    g_game->m_mapHeader = saved.m_mapHeader;
    g_game->m_setup = saved.m_mapSetup;
    g_inCampaign = saved.m_campaignGame;
    g_game->m_campaign = saved.m_campaign;
    strcpy(g_game->m_saveFileName, saved.m_fileName.c_str());
    g_game->m_difficultyRating = saved.m_difficultyRating;
    g_game->m_numDeadPlayers = saved.m_numDeadPlayers;
    memcpy(g_game->m_playerDisabled, saved.m_deadPlayer,
           sizeof(g_game->m_playerDisabled));
    g_netLocalGamePos = saved.m_currentPlayer;
    memcpy(g_wasHuman, saved.m_humanPlayer, sizeof(saved.m_humanPlayer));
}

// Complete's packed bit readers use unsigned byte indexing. This inferred
// decoder owns assignment into an existing bitset; callers retain their stream
// reads and any later copy into the live game arrays. Campaign's returned-value
// reader and mapcell's signed division loops retain their distinct operations.
template <size_t N>
void decodePackedBits(const unsigned char* packed, std::bitset<N>& result)
{
    for (unsigned int index = 0; index < N; ++index) {
        result[index] = (packed[index >> 3] & (1 << (index & 7))) != 0;
    }
}

// Original: game::Load; game.cpp:3026, dc 0xa83d0. Complete loads a
// SavedGameHeader value and restores the acting-player and human-player state
// before the map pools. DC's gzread interface became TAbstractFile::read.
// Its scalar reads and final map-extra read stage their results through count.
// Keep the pool loaders canonical and the packed-byte scratch buffer inside
// its hero loop. Default bitset construction reproduces more of the retained
// calls than the unsigned-long constructor; neither requires inline controls.
// The native range reader deduces the serialized count type. Its char instance
// preserves loadBlackMarkets, while its short instances reproduce the register
// allocation of every expanded loadVector call. Together with the counted
// disabled-skill clear and native packed-byte read, game::load matches all
// 139 retail blocks exactly. No inline-control pragma or release assertion is
// needed by this model.
VA(0x004bcda0, 0xEC2)  // anchor-callee set (4 claimed pool loaders) + 'H3SVG', dc 0xa83d0
int game::load(TAbstractFile* infile)
{
    SavedGameHeader saved;
    if (saved.load(infile))
        return -1;
    applySavedGameHeader(saved);

    char byteValue;
    unsigned char extraByteValue;
    short shortValue;
    unsigned short extraShortValue;
    int zero;
    int count;
    int i;

    clearEventRecords();
    // Complete expands SetMapSize at +0x1cc..+0x1e7, before the pool reads.
    // DC calls it later at game.cpp:3193, just before the map-extra plane.
    setMapSize(m_mapHeader.m_size, m_mapHeader.m_size);

    if (saved.m_version >= 41) {
        g_grailOwner = readValue<char>(infile);
    } else {
        g_grailOwner = -1;
    }

    if (saved.m_version >= 34) {
        infile->read(m_artifactDisabled, sizeof(m_artifactDisabled));
        infile->read(m_artifactUsed, sizeof(m_artifactUsed));
    } else if (saved.m_version >= 25) {
        infile->read(m_artifactDisabled, 0x81);
        infile->read(m_artifactUsed, 0x81);
    }

    if (saved.m_version >= 29)
        infile->read(m_ssDisabled, sizeof(m_ssDisabled));
    else
        MEMSET(m_ssDisabled, 0, sizeof(m_ssDisabled), i);

    if (loadRumours(infile) < 0)
        return -1;

    if (loadBlackMarkets(infile) < 0)
        return -1;

    if (m_worldMap.load(infile, m_mapHeader.m_size, m_mapHeader.m_hasTwoLayers,
                      saved.m_version) < 0)
        return -1;
    if (loadSignPool(infile) < 0)
        return -1;
    if (loadMinePool(infile, saved.m_version) < 0)
        return -1;

    if (!loadObjectVector(infile, m_generators))
        return -1;

    if (loadGarrisonPool(infile, saved.m_version) < 0)
        return -1;
    if (loadBoatPool(infile) < 0)
        return -1;

    // DC line 3073 calls LoadObeliskPool. Its canonical body preserves one
    // caller failure/cleanup boundary for the count and payload reads.
    if (loadObeliskPool(infile) < 0)
        return -1;

    if (loadPlayerData(infile, saved.m_version) < 0)
        return -1;

    if (loadTownPool(infile, saved.m_version) < 0)
        return -1;

    if (loadHeroPool(infile, saved.m_version) < 0)
        return -1;

    if (saved.m_version < 31) {
        unsigned char legacyHeroPoolMap[8];
        if (infile->read(legacyHeroPoolMap, sizeof(legacyHeroPoolMap))
            < sizeof(legacyHeroPoolMap))
            return -1;
    }

    // THE AVAILABILITY READ IS AN IF/ELSE ON THE VERSION, NOT ONE READ OF
    // `heroCount`. Retail re-tests `saved.version >= 25` here and emits two
    // separate guarded reads with the literals 0x9c and 0x80 - two
    // teardowns, not one - and the 0x40 fill lives inside the SHORT arm
    // rather than behind an `if (heroCount < HERO_COUNT)`.
    if (saved.m_version >= 25) {
        if (infile->read(m_heroAvailability, sizeof(m_heroAvailability))
            < sizeof(m_heroAvailability))
            return -1;
    } else {
        if (infile->read(m_heroAvailability,
                         g_mapHeaderLegacyHeroCount * sizeof(m_heroAvailability[0]))
            < g_mapHeaderLegacyHeroCount * sizeof(m_heroAvailability[0]))
            return -1;
        std::fill(m_heroAvailability + g_mapHeaderLegacyHeroCount,
                  m_heroAvailability + HERO_COUNT,
                  static_cast<char>(hero::HERO_AVAILABILITY_TAVERN_POOL));
    }

    if (saved.m_version >= 31) {
        for (i = 0; i < HERO_COUNT; ++i) {
            std::bitset<8> poolMap;
            unsigned char poolBits[1];
            readValue(infile, poolBits);
            decodePackedBits(poolBits, poolMap);
            m_heroPoolMap[i] = poolMap;
        }
    }

    // The twelve guarded scalar reads, and they mirror game::Save's twelve
    // writes temp for temp: retail carries FOUR of them, a char reused
    // across reads 1-2 and 7-9, a second char for 5-6, a short for 3-4 and
    // a second short for 10-12.
    count = infile->read(&byteValue, sizeof(byteValue));
    if (count < sizeof(byteValue))
        return -1;
    m_newCampaignStarted = byteValue;
    count = infile->read(&byteValue, sizeof(byteValue));
    if (count < sizeof(byteValue))
        return -1;
    m_numPlayers = byteValue;

    count = infile->read(&shortValue, sizeof(shortValue));
    if (count < sizeof(shortValue))
        return -1;
    m_ultimateArtifactX = shortValue;
    count = infile->read(&shortValue, sizeof(shortValue));
    if (count < sizeof(shortValue))
        return -1;
    m_ultimateArtifactY = shortValue;

    count = infile->read(&extraByteValue, sizeof(extraByteValue));
    if (count < sizeof(extraByteValue))
        return -1;
    m_ultimateArtifactZ = extraByteValue;
    count = infile->read(&extraByteValue, sizeof(extraByteValue));
    if (count < sizeof(extraByteValue))
        return -1;
    m_ultimateRadius = extraByteValue;

    count = infile->read(&byteValue, sizeof(byteValue));
    if (count < sizeof(byteValue))
        return -1;
    m_ultimateArtifactPresent = byteValue != 0;
    // A SEPARATE GUARDED BYTE, not a second use of the one above. Retail
    // reads into the same slot again and only THEN tests the version, and
    // the store is `movsx ecx, byte ptr` into the int at +0x1f698 - which
    // is what makes the temp a signed char. game::Save's mirror writes
    // `static_cast<char>(f_1f698)` as its own eighth scalar.
    count = infile->read(&byteValue, sizeof(byteValue));
    if (count < sizeof(byteValue))
        return -1;
    if (saved.m_version < 40)
        m_gameVersion = byteValue;

    count = infile->read(&byteValue, sizeof(byteValue));
    if (count < sizeof(byteValue))
        return -1;
    m_isCheater = byteValue;

    count = infile->read(&extraShortValue, sizeof(extraShortValue));
    if (count < sizeof(extraShortValue))
        return -1;
    m_day = extraShortValue;
    count = infile->read(&extraShortValue, sizeof(extraShortValue));
    if (count < sizeof(extraShortValue))
        return -1;
    m_week = extraShortValue;
    count = infile->read(&extraShortValue, sizeof(extraShortValue));
    if (count < sizeof(extraShortValue))
        return -1;
    m_month = extraShortValue;

    // Retail asks for all 32 bytes but accepts an eight-byte return here.
    count = infile->read(m_uniqueSystemId, sizeof(m_uniqueSystemId));
    if (count < 8U)
        return -1;
    count = infile->read(m_marketArtifacts, sizeof(m_marketArtifacts));
    if (count < sizeof(m_marketArtifacts))
        return -1;
    count = infile->read(m_globalInfoFlags, sizeof(m_globalInfoFlags));
    if (count < sizeof(m_globalInfoFlags))
        return -1;
    count = infile->read(m_borderTentVisitFlags, sizeof(m_borderTentVisitFlags));
    if (count < sizeof(m_borderTentVisitFlags))
        return -1;
    count = infile->read(m_cartographerMask, sizeof(m_cartographerMask));
    if (count < sizeof(m_cartographerMask))
        return -1;
    count = infile->read(m_cartographerFlags, sizeof(m_cartographerFlags));
    if (count < sizeof(m_cartographerFlags))
        return -1;

    // The four-byte slot game::Save writes as a literal zero. Retail reads
    // it into a stack dword and never looks at it again - the guard is the
    // only thing it is for.
    count = infile->read(&zero, sizeof(zero));
    if (count < sizeof(zero))
        return -1;

    // The map-extra plane, the mirror of game::Save's write: HasTwoLevels
    // read through the GLOBAL gpGame rather than this->worldMap, and the
    // *2 applied LAST (retail's `lea edi,[eax+eax]` follows both imuls).
    int mapExtraSize = (g_mapWidth * g_mapHeight) * g_game->getNumMapLevels();
    count = infile->read(g_mapExtra, mapExtraSize * sizeof(unsigned short));
    if (count < mapExtraSize * sizeof(unsigned short))
        return -1;

    int poolCount = saved.m_version >= 32 ? 8 : 3;
    for (i = 0; i < poolCount; ++i)
        loadVector(infile, m_lithPools[i]);
    for (i = 0; i < poolCount; ++i)
        loadVector(infile, m_lithExitPools[i]);
    loadVector(infile, m_whirlpools);
    loadVector(infile, m_undergroundGateExits);
    loadVector(infile, m_undergroundGatePairs);
    loadVector(infile, m_universities);
    loadObjectVector(infile, m_creatureBanks);

    if (!loadRecordedEvents(infile, saved.m_version))
        return -1;

    g_advManager->m_curHeroMobile = 0;
    g_currentPlayer = &g_game->m_players[g_netLocalGamePos];
    g_curPlayerBit = 1 << g_netLocalGamePos;
    if (!g_remoteOn)
        g_curWatchPlayer = g_netLocalGamePos;
    setupShipyards();
    g_mapVisibilityBit = 1 << g_curWatchPlayer;
    g_completeDrawEnabled = g_game->isLocalHuman(g_netLocalGamePos);
    setupAdjacentMons();
    aiExamineMap();

    return 0;
}

// Retained compiler-generated SCampaign memberwise assignment.
VA_COMPGEN(0x004bdc70, 0x309, IMPLICIT_COPY_ASSIGN, SCampaign)

VA(0x004be140, 0x11E)
int SGameSetupOptions::save(TAbstractFile* outfile)
{
    char charBuffer;

    outfile->write(m_color, sizeof(m_color));
    outfile->write(m_color, sizeof(m_handicap));
    outfile->write(m_alignment, sizeof(m_alignment));
    outfile->write(m_playerPos, sizeof(m_playerPos));

    charBuffer = m_difficulty;
    outfile->write(&charBuffer, sizeof(charBuffer));
    outfile->write(m_filename, sizeof(m_filename));
    outfile->write(m_path, sizeof(m_path));
    outfile->write(m_canFlipFromToComputer, sizeof(m_canFlipFromToComputer));

    charBuffer = m_curSelectedPlayer;
    outfile->write(&charBuffer, sizeof(charBuffer));
    charBuffer = m_fileInitialized;
    outfile->write(&charBuffer, sizeof(charBuffer));
    charBuffer = m_initializationNumHumans;
    outfile->write(&charBuffer, sizeof(charBuffer));
    charBuffer = m_turnDuration;
    outfile->write(&charBuffer, sizeof(charBuffer));

    for (int i = 0; i < 8; ++i) {
        charBuffer = m_startingHero[i];
        outfile->write(&charBuffer, sizeof(charBuffer));
    }

    return outfile->write(m_startingBonus, sizeof(m_startingBonus)) <
                   sizeof(m_startingBonus)
               ? -1
               : 0;
}

VA(0x004be260, 0x188)
int SGameSetupOptions::load(TAbstractFile* infile, int saveVersion)
{
    TAbstractFile* input = infile;
    {
        char charBuffer;

        input->read(m_color, sizeof(m_color));
        input->read(m_color, sizeof(m_handicap));
        input->read(m_alignment, sizeof(m_alignment));
        input->read(m_playerPos, sizeof(m_playerPos));

        input->read(&charBuffer, sizeof(charBuffer));
        m_difficulty = charBuffer;
        input->read(m_filename, sizeof(m_filename));
        input->read(m_path, sizeof(m_path));
        if (saveVersion < 28)
            strcpy(m_path, "maps");
        input->read(m_canFlipFromToComputer, sizeof(m_canFlipFromToComputer));

        input->read(&charBuffer, sizeof(charBuffer));
        m_curSelectedPlayer = charBuffer;
        input->read(&charBuffer, sizeof(charBuffer));
        m_fileInitialized = charBuffer != 0;
        input->read(&charBuffer, sizeof(charBuffer));
        m_initializationNumHumans = charBuffer;
        input->read(&charBuffer, sizeof(charBuffer));
        m_turnDuration = charBuffer;
    }

    int* heroPos = m_startingHero;
    int heroesRemaining = 8;
    do {
        *heroPos = loadHeroId(input, saveVersion);
        ++heroPos;
        --heroesRemaining;
    } while (heroesRemaining);

    return input->read(m_startingBonus, sizeof(m_startingBonus)) <
                   sizeof(m_startingBonus)
               ? -1
               : 0;
}

// E:\gamedcs\game.cpp:3311
// PARTIAL: retail prefix through the town / hero / hero-pool block and
// the five type_point pool writes. Still absent after them, in retail's
// order: twelve guarded scalar writes (0x1f4d4, 0x1f634, then
// 0x1f690/0x1f692 as shorts and 0x1f694, 0x1f695, 0x1f696, 0x1f698,
// 0x1f69c as bytes, then 0x1f63e/0x1f640/0x1f642 as shorts), six array
// writes (0x1f644 asking 0x20 but accepting 8, 0x1f664 0x1c,
// globalInfoFlags 0x20, borderTentVisitFlags 8, cartographerMask 6,
// cartographerFlags 3), a four-byte literal zero, a gMapExtra payload
// of 2*(worldMap.HasTwoLevels+1)*MAP_WIDTH*MAP_HEIGHT bytes, then
// universities via save_vector<type_university> (0x4d2b20),
// creatureBanks via save_object_vector (0x4d2b80) and
// game::save_recorded_events (0x49dc60).

// MEASURED, and the reason they are not written yet: adding them
// LOWERS the score. 42.23 with the block above; 40.62 with the twelve
// scalar and six array writes added; 37.18 with the zero and gMapExtra
// writes on top. The cause is the FRAME, not the statements.

// FIXED 2026-08-20 by BLOCK-SCOPING them. Each guarded write now
// declares its own buffer inside braces instead of reusing two
// function-scope variables; that cuts their live ranges to a single
// statement and lets VC6 colour them where retail put them. The
// small-slot region went from FOUR slots to TWO, `sub esp` from 0x5b4
// to 0x5ac, and the census now shows the same [ebp+0x8] / [ebp+0xb]
// parameter-home pair retail uses. 42.2290 -> 42.3069 on the scoping
// alone. 0x5ac + the five tail slots = 0x5c0 = retail's frame exactly,
// so the tail is UNBLOCKED - but it must be spelled with exactly the
// five temps below and no more, or the frame overshoots again.

// TAIL LANDED 2026-08-20: 42.3069 -> 73.7978. Three things had to be
// right at once, and each was measured on its own.

// 1. PLACEMENT. The missing block goes before the five type_point pool
//    writes. Retail's order
//    is heroPoolMap loop, then the twelve scalars, six arrays, literal
//    zero and gMapExtra, and only THEN lithPools. Three independent
//    proofs: linear disassembly order; the EH-state counter running
//    0x1f..0x48 consecutively over exactly 21 guarded sites, with the
//    lithPools loop numbered LAST at 0x47/0x48; and the base compile's
//    own adjacency of heroPoolMap to lithPools. Only universities,
//    creatureBanks and save_recorded_events append at the end.
// 2. THE TEMPS. Four scalar temps plus the literal-zero int, declared at
//    FUNCTION scope, not in a block. Block-scoped they get coalesced
//    into the parameter home and the frame lands 12 bytes short; hoisted
//    they take five real slots and `sub esp` reaches retail's 0x5c0 with
//    the slot map matching offset for offset and use-count for
//    use-count. The two shorts must be `short`, not `int`: retail loads
//    16 bits and stores 32, which is VC6's narrow-local/widened-store
//    idiom, where an int local would sign- or zero-extend on the load.
// 3. TWO INLINE PINS, and this is the part that is easy to get wrong.
//    Adding ~600 bytes of correct code RAISES this function's /Ob2
//    budget (clamp(2*caller_cb,...)), which then pulls in two expansions
//    retail does not have. Both had to be pinned back out:
//      * std::bitset<8>::test in the heroPoolMap loop - retail emits
//        `push edi / call`. Inlined, it drags its _Xran throw path with
//        it, and a whole std::out_of_range plus its "invalid bitset<N>
//        position" string lands on the frame: 28 bytes retail has not.
//        Removing this pin alone costs 73.7978 -> 43.8831.
//      * the seven save_vector / save_object_vector sites - retail calls
//        every one. Inlined, each contributes a vector-size shl/sar and
//        a `setae`. Adding this pin alone gave 37.4763 -> 73.7978.
//    This is the /Ob2-budget lever running in the direction the module
//    guide warns about: a local win elsewhere in the body changed the
//    inline decisions here.

// 80.0926 -> 82.6492, 2026-08-20, with game::Load's `return`-pin lever:
// `#pragma inline_depth(0)` on a `return` statement reaches the LOCAL'S
// SCOPE-EXIT DESTRUCTOR. Retail calls ~SavedGameHeader out of line at
// THREE exits and expands it at only one; the two at the foot of this
// body - `if (!save_recorded_events(...)) return -1;` and `return 0;` -
// are the pair retail emits as `lea ecx,[ebp-0x5cc] / call 0x4bdf80`
// twice over, once behind the `jne` and once on the fall-through.

// 82.6492 -> 85.4129, 2026-08-20. THE MERGED-RETURN CLASS IS SPELLABLE,
// and the note it replaces was wrong to call it unreachable. The `[ebp-4]`
// cleanup-site census names it exactly: retail emits 37 numbered sites,
// we emitted 39, and the two extra are three merges minus one placement
// difference. A merged site is ONE `return -1` reached by two conditions,
// and a `goto` to a label inside the surviving arm produces it. Which arm
// carries the label decides the BLOCK LAYOUT, and both cases occur here:
//   * generators and towns - "count write guard" + "element save loop
//     guard". The label goes in the COUNT WRITE's own `if` body and the
//     loop `goto`s BACKWARD to it. Retail's merged block then lands where
//     retail puts it (0x10 emitted after 0x12; 0x1a in natural order).
//     Writing it the other way round - the loop inside an `if` with the
//     `return -1` in a trailing `else` - merges the site but SINKS the
//     block to the end of the function and scores 0.02 lower.
//   * The canonical ObeliskPool helper owns these early returns and preserves
//     the caller cleanup boundary.
//   * field_4e3e8 + obeliskFlags - two adjacent guarded writes. Here the
//     label goes in the SECOND guard and the first `goto`s FORWARD into
//     it, because retail's teardown is the fall-through successor of the
//     second test (`jb <teardown>` then `jae <skip>` then the teardown).
//     That reproduces retail's 0x4efd-0x4f47 instruction for instruction.
//     An `||` merges the site too, but as a two-jump join that sinks to
//     the function end; it measured 0.14 HIGHER on its own and 1.34 LOWER
//     once the tail pin below was narrowed, so the four combinations rank
//     non-monotonically - measure the pair, not each knob.
// Worth +1.69 across the three merges.

// THE HERO-POOL BYTE IS AN ARRAY, NOT A SCALAR (+1.10, and it is the
// exact mirror of game::Load's `poolBits[player >> 3] & (1 << (player &
// 7))`). Retail's `mov edx,edi / shr edx,3 / lea eax,[ebp+edx+0xb] / or
// byte ptr [eax],dl` is an INDEXED store: a one-element `unsigned char
// poolBits[1]` written `poolBits[player >> 3] |= 1 << (player & 7)`.
// Spelled as a scalar `poolBits |= 1 << player` VC6 keeps it in a real
// frame slot; spelled as an array, and declared INSIDE the hero loop
// rather than at function scope, it coalesces the way retail's does.

// 85.4129 -> 89.1864, 2026-08-20 (cold-combatpath lane), two of the three
// residual items below closed:
//   * THE FRAME IS RETAIL'S NOW (sub esp,0x5c0 both sides): the two
//     missing dwords were the five BLOCK-scoped serialization buffers
//     (four `char char_buffer` blocks and one `short short_buffer`)
//     sharing slots; promoting them to two more FUNCTION-scope locals
//     (`char char_buffer; short short_buffer;`) is retail's seven-dword
//     temp pool exactly (+0.10 alone - the frame is the enabler, not
//     the win; the doses-combine rule in person).
//   * THE towns.size() OVER-INLINE IS SPELLABLE, +3.68: a statement pin
//     covers a whole `for` including its body, but PRAGMA STATE IS
//     PER-SITE AT COLLECTION - so `i = 0; #pragma inline_depth(0)
//     while (i < towns.size()) { #pragma inline_depth() <body>; ++i; }`
//     confines the pin to the loop CONDITION: size() goes out of line
//     (retail's call) while towns[i].save's receiver subscript stays
//     inline. New lever: PIN A LOOP CONDITION ALONE by resetting the
//     pragma as the first body line.
// Residual (89.1864%): one item, measured and bounded.
//   * ONE guarded-return teardown still has the wrong destructor shape
//     (predict-inline: `_Tidy base x35 vs retail x36`).
//     Retail splits the two pool loops: the lithPools failure expands
//     ~SavedGameHeader and CALLS `_Tidy` (site 0x48 at 0x556e, falling
//     through into the shared tail), while the lithExitPools failure
//     calls ~SavedGameHeader out of line (0x55b6). Taking the lithPools
//     `return -1` out of the `#pragma inline_depth(0)` block - by landing
//     save_vector's result in a `lithSaved` local and pinning only that
//     assignment - reproduces the SPLIT (+0.35, and it is what takes the
//     site census to 37 = retail's) but VC6 then expands `_Tidy` there
//     too. `#pragma inline_depth(1)` on that `return` is byte-flat, as
//     the module guide's bound predicts, so the depth-1 shape retail has
//     is not spellable. Costs 3 branches (58 against 55).
//   * one over-inline the pin cannot reach: retail CALLS
//     vector<town>::size() (0x4cf6c0) in the town loop's CONDITION while
//     inlining the same size() for the byte count two statements earlier.
//     A statement pin on the `for` would also de-inline the `towns[i]`
//     subscript, which retail keeps inline.
//   * the heroes loop's teardown is emitted after the heroAvailability
//     write's (0x1e before 0x1c) where retail emits them in order.

VA(0x004be3f0, 0xAA5)  // SavedGameHeader + write/pool callee sequence, dc 0xa8cd0
int game::save(TAbstractFile* outfile)
{
    char byteValue;
    unsigned char extraByteValue;
    char charBuffer;
    short shortValue;
    unsigned short extraShortValue;
    int zero;
    SavedGameHeader saved;
    saved.reset();
    if (saved.save(outfile) < 0)
        return -1;

    {
        charBuffer = g_grailOwner;
        outfile->write(&charBuffer, sizeof(charBuffer));
    }
    outfile->write(m_artifactDisabled, sizeof(m_artifactDisabled));
    outfile->write(m_artifactUsed, sizeof(m_artifactUsed));
    outfile->write(m_ssDisabled, sizeof(m_ssDisabled));

    if (saveRumours(outfile) < 0)
        return -1;

    if (saveBlackMarkets(outfile) < 0)
        return -1;

    if (m_worldMap.save(outfile, m_mapHeader.m_size, m_mapHeader.m_hasTwoLayers) < 0)
        return -1;
    // DC game.cpp:3515 names SaveSignPool; retail retains the call.
    // Restoring that helper's int result/index and separate char buffer
    // makes the old condition pin byte-inert: 59.5944% with or without it.
    // Control with the previous helper and no pin gives 50.1717%; both
    // retained helper bodies are exact. Keep the source facts, not the pin.
    if (saveSignPool(outfile) < 0)
        return -1;
    if (saveMinePool(outfile) < 0)
        return -1;

    if (!saveObjectVector(outfile, m_generators))
        return -1;

    int i;
    if (saveGarrisonPool(outfile) < 0)
        return -1;
    if (saveBoatPool(outfile) < 0)
        return -1;

    if (saveObeliskPool(outfile) < 0)
        return -1;

    if (savePlayerData(outfile) < 0)
        return -1;

    if (saveTownPool(outfile) < 0)
        return -1;

    if (saveHeroPool(outfile) < 0)
        return -1;

    if (outfile->write(m_heroAvailability, sizeof(m_heroAvailability)) <
        sizeof(m_heroAvailability)) {
        return -1;
    }

    // Complete's packed hero-player masks have no DC loop counterpart.
    // Retail retains bitset<8>::test. Reading through const operator[]
    // preserves that boundary without a pin (59.5944% for the whole save);
    // direct test(), including on a const reference, expands it (57.4921%).
    for (i = 0; i < HERO_COUNT; ++i) {
        const std::bitset<8>& players = m_heroPoolMap[i];
        unsigned char poolBits[1];
        poolBits[0] = 0;
        unsigned int player;
        for (player = 0; player < 8; ++player) {
            if (players[player])
                poolBits[player >> 3] |= 1 << (player & 7);
        }
        outfile->write(poolBits, sizeof(poolBits));
    }

    // The twelve guarded scalar writes. Retail carries FOUR temps for
    // them, not one: a char reused across writes 1-2 and 7-9, an unsigned
    // char for 5-6, a short for 3-4 and an unsigned short for 10-12. Those
    // four plus the literal-zero dword below are exactly the five slots
    // that take `sub esp` from our 0x5ac to retail's 0x5c0.
    // The word buffers must stay narrow, not `int`: retail loads 16 bits
    // (`mov dx, word ptr`) and stores 32 (`mov dword ptr [ebp-N], edx`),
    // which is VC6's narrow-local/widened-store idiom - an int local
    // would sign- or zero-extend on the load instead.
    {
        byteValue = m_newCampaignStarted;
        if (outfile->write(&byteValue, sizeof(byteValue)) < sizeof(byteValue))
            return -1;
        byteValue = m_numPlayers;
        if (outfile->write(&byteValue, sizeof(byteValue)) < sizeof(byteValue))
            return -1;

        shortValue = m_ultimateArtifactX;
        if (outfile->write(&shortValue, sizeof(shortValue)) < sizeof(shortValue))
            return -1;
        shortValue = m_ultimateArtifactY;
        if (outfile->write(&shortValue, sizeof(shortValue)) < sizeof(shortValue))
            return -1;

        extraByteValue = m_ultimateArtifactZ;
        if (outfile->write(&extraByteValue, sizeof(extraByteValue)) <
            sizeof(extraByteValue))
            return -1;
        extraByteValue = m_ultimateRadius;
        if (outfile->write(&extraByteValue, sizeof(extraByteValue)) <
            sizeof(extraByteValue))
            return -1;

        byteValue = m_ultimateArtifactPresent;
        if (outfile->write(&byteValue, sizeof(byteValue)) < sizeof(byteValue))
            return -1;
        // f_1f698 is an int member and retail writes only its low byte.
        byteValue = static_cast<char>(m_gameVersion);
        if (outfile->write(&byteValue, sizeof(byteValue)) < sizeof(byteValue))
            return -1;
        byteValue = m_isCheater;
        if (outfile->write(&byteValue, sizeof(byteValue)) < sizeof(byteValue))
            return -1;

        extraShortValue = m_day;
        if (outfile->write(&extraShortValue, sizeof(extraShortValue)) <
            sizeof(extraShortValue))
            return -1;
        extraShortValue = m_week;
        if (outfile->write(&extraShortValue, sizeof(extraShortValue)) <
            sizeof(extraShortValue))
            return -1;
        extraShortValue = m_month;
        if (outfile->write(&extraShortValue, sizeof(extraShortValue)) <
            sizeof(extraShortValue))
            return -1;
    }

    // Six array writes. The first is retail's own inconsistency: it ASKS
    // for sizeof(field_1f644) == 0x20 and accepts 8. The compare is
    // unsigned (`jae`), so the bound has to be an unsigned expression;
    // sizeof(borderTentVisitFlags) is the one in scope that equals 8.
    // The original expression is not recoverable from the bytes - only
    // its value and its unsignedness are.
    if (outfile->write(m_uniqueSystemId, sizeof(m_uniqueSystemId)) <
        sizeof(m_borderTentVisitFlags))
        return -1;
    if (outfile->write(m_marketArtifacts, sizeof(m_marketArtifacts)) < sizeof(m_marketArtifacts))
        return -1;
    if (outfile->write(m_globalInfoFlags, sizeof(m_globalInfoFlags)) <
        sizeof(m_globalInfoFlags))
        return -1;
    if (outfile->write(m_borderTentVisitFlags, sizeof(m_borderTentVisitFlags)) <
        sizeof(m_borderTentVisitFlags))
        return -1;
    if (outfile->write(m_cartographerMask, sizeof(m_cartographerMask)) <
        sizeof(m_cartographerMask))
        return -1;
    if (outfile->write(m_cartographerFlags, sizeof(m_cartographerFlags)) <
        sizeof(m_cartographerFlags))
        return -1;

    zero = 0;
    if (outfile->write(&zero, sizeof(zero)) < sizeof(zero))
        return -1;

    // The map-extra plane. HasTwoLevels is read through the GLOBAL gpGame,
    // not through this->worldMap, and the *2 is applied LAST - retail's
    // `lea edi,[eax+eax]` follows both imuls. The count is computed once
    // into one local because a virtual call sits between its two uses.
    unsigned int mapExtraBytes =
        (g_game->m_worldMap.getNumLevels()) * g_mapWidth * g_mapHeight *
        sizeof(unsigned short);
    if (outfile->write(g_mapExtra, mapExtraBytes) < mapExtraBytes)
        return -1;

    // The canonical writers now retain the seven retail call boundaries
    // without compiler-state fences.
    for (i = 0; i < 8; ++i) {
        unsigned char lithSaved;
        lithSaved = saveVector(outfile, m_lithPools[i]);
        if (!lithSaved)
            return -1;
    }
    for (i = 0; i < 8; ++i) {
        if (!saveVector(outfile, m_lithExitPools[i]))
            return -1;
    }
    saveVector(outfile, m_whirlpools);
    saveVector(outfile, m_undergroundGateExits);
    saveVector(outfile, m_undergroundGatePairs);
    saveVector(outfile, m_universities);
    saveObjectVector(outfile, m_creatureBanks);

    // Retail calls ~SavedGameHeader out of line at both final exits. The
    // recovered caller mass now selects that cleanup naturally.
    if (!saveRecordedEvents(outfile))
        return -1;

    return 0;
}

VA(0x004beea0, 0x2F6)  // dc 0xa99d0
unsigned char game::saveGame(const char* filename, unsigned char determineSuffix, unsigned char campaignWinMode, unsigned char compressIt, unsigned char xferFile)
{
    char nameNoExtension[351] = {0};
    char saveName[351] = {0};
    char fullPath[351];
    CTimer saveGameTimer(1);

    saveGameTimer.start();
    if (!campaignWinMode)
        g_advManager->demobilizeCurrHero(0, 1);

    if (determineSuffix) {
        strcpy(nameNoExtension, filename);
        strtok(nameNoExtension,
               DATA_COMPGEN(0x006603ec, saveExtensionDot, "."));
        if (g_inCampaign)
            sprintf(saveName,
                    DATA_COMPGEN(0x00677d98, nameWithExtensionFormat, "%s.%s"),
                    nameNoExtension,
                    DATA_COMPGEN(0x00677da4, campaignSaveSuffix, "CGM"));
        else if (m_isTutorial)
            sprintf(saveName,
                    DATA_COMPGEN(0x00677d98, nameWithExtensionFormat, "%s.%s"),
                    nameNoExtension,
                    DATA_COMPGEN(0x00677da0, tutorialSaveSuffix, "TGM"));
        else
            sprintf(saveName,
                    DATA_COMPGEN(0x00677d90, saveSlotNameFormat, "%s.GM%d"),
                    nameNoExtension, g_numHumanPlayers);
    } else {
        strcpy(saveName, filename);
    }

    if (xferFile) {
        sprintf(fullPath,
                DATA_COMPGEN(0x00660358, processSearchFoundFormat, "%s%s"),
                DATA_COMPGEN(0x00677d88, dataDirectoryPrefix, ".\\DATA\\"),
                saveName);
    } else {
        sprintf(fullPath,
                DATA_COMPGEN(0x00660358, processSearchFoundFormat, "%s%s"),
                DATA_COMPGEN(0x0063e65c, gamesDirectoryPrefix, ".\\GAMES\\"),
                saveName);
        // General text 77 and 109 are the two reserved auto-save names;
        // a save under either of them does not become the remembered one.
        if (_strnicmp(saveName, g_generalText->getText(GENERAL_TEXT_AUTOSAVE_NAME), 8)
            && _strnicmp(saveName, g_generalText->getText(GENERAL_TEXT_PLAYER_EXIT_SAVE_NAME), 8))
            strcpy(g_game->m_saveFileName, filename);
    }

    const char* compression =
        DATA_COMPGEN(0x00677d80, gzDeflateWriteMode, "wb6+");
    if (!compressIt)
        compression = DATA_COMPGEN(0x00677d78, gzStoreWriteMode, "wb0+");

    try {
        {
            TGzFile outfile(fullPath, compression);
            save(&outfile);
        }
        saveGameTimer.stop();
        return 1;
    } catch (TGzFile::TOpenFailure) {
        normalDialog(
            formatString(
                g_generalText->getText(g_saveGameFailureGeneralText),
                filename).c_str(),
            1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
        return 0;
    }
}

VA(0x004bf1a0, 0x183)
void game::setupOrigData()
{
    int i;

    g_normalVictory = 0;
    g_grailOwner = -1;
    m_difficultyRating = 1;
    g_weekTypeExtra = 0;
    g_weekType = 0;
    g_monthType = 0;
    g_monthTypeExtra = 0;
    m_isCheater = 0;

    strncpy(m_saveFileName, g_generalText->getText(GENERAL_TEXT_NEW_GAME_SAVE_NAME), sizeof(m_saveFileName));
    m_saveFileName[sizeof(m_saveFileName) - 1] = 0;
    MEMSET(m_playerDisabled, 0, sizeof(m_playerDisabled), i);
    memset(g_startingHeroOverrides, -1, sizeof(g_startingHeroOverrides));

    m_ultimateArtifactX = -1;
    m_ultimateArtifactY = -1;
    m_ultimateArtifactZ = -1;
    m_month = 1;
    m_week = 1;
    m_day = 1;
    m_ultimateRadius = 0x7f;
    m_ultimateArtifactPresent = 0;

    for (i = 0; i < 8; ++i)
        m_uniqueSystemId[i * sizeof(int)] = 0;

    m_numObelisks = 0;
    advManager* manager = g_advManager;
    manager->m_curHeroMobile = 0;
    MEMSET(m_heroAvailability, -1, sizeof(m_heroAvailability), i);

    std::bitset<8> allPlayers;
    allPlayers.set();
    for (i = 0; i < HERO_COUNT; ++i)
        m_heroPoolMap[i] = allPlayers;

    for (i = 0; i < HERO_COUNT; ++i) {
        m_heroSetup[i].heroExtraFn004B8450(i);
        m_heroes[i].initialize(i);
    }

    MEMSET(m_obeliskFlags, 0, sizeof(m_obeliskFlags), i);
    MEMSET(m_spellAllocInfo, 0, sizeof(m_spellAllocInfo), i);
    MEMSET(m_spellDisabledInfo, 0, sizeof(m_spellDisabledInfo), i);
    MEMSET(m_cartographerFlags, 0, sizeof(m_cartographerFlags), i);
}

VA(0x004bf330, 0x23B)
int game::loadGame(const char* filename, int isOrigData, int isQuickLoad)
{
    setupOrigData();
    if (isOrigData)
        return 0;

    char buf[450];
    g_gameOver = 0;
    if (_strnicmp(filename,
                  DATA_COMPGEN(0x00677da8, remoteSavePrefix, "RMT"),
                  3) == 0) {
        sprintf(buf,
                DATA_COMPGEN(0x00660358, processSearchFoundFormat, "%s%s"),
                DATA_COMPGEN(0x00677d88, dataDirectoryPrefix, ".\\DATA\\"),
                filename);
    } else {
        sprintf(buf,
                DATA_COMPGEN(0x00660358, processSearchFoundFormat, "%s%s"),
                DATA_COMPGEN(0x0063e65c, gamesDirectoryPrefix, ".\\GAMES\\"),
                filename);
    }

    try {
        TGzFile infile(
            buf, DATA_COMPGEN(0x00677d6c, gzReadMode, "rb"));

        int i;
        for (i = 0; i < 8; ++i) {
            m_lithPools[i].clear();
            m_lithExitPools[i].clear();
        }
        m_whirlpools.clear();
        m_undergroundGateExits.clear();
        m_undergroundGatePairs.clear();
        m_monsterIdentifiers.clear();

        load(&infile);
        return 1;
    } catch (TGzFile::TOpenFailure) {
        return 0;
    }
}

VA(0x004bf570, 0x203)
void game::giveTroopsToNeutralTown(int townId)
{
    town* currentTown = &m_towns[townId];
    long weekNumber = static_cast<short>(
        (m_month * 4 + m_week - 5) * 7 + m_day) / 7;
    int maxRoll = min(weekNumber, 8) + 1;
    int roll = random(0, maxRoll) + random(0, maxRoll)
              + random(0, maxRoll);

    long monsterLevel;
    for (monsterLevel = 0;
         monsterLevel < 6 && g_neutralTownLevelWeights[monsterLevel] < roll;
         ++monsterLevel) {
        roll -= g_neutralTownLevelWeights[monsterLevel];
    }

    int townType = currentTown->m_type;
    armyGroup* townArmy = &currentTown->getArmy();
    TCreatureType creature;
    TCreatureType upgradedCreature;
    TCreatureType upgradedValue = (g_townDwellingCreatures + TOWN_DWELLING_COUNT)[
        townType * TOWN_DWELLING_SLOTS + monsterLevel];
    creature = g_townDwellingCreatures[
        townType * TOWN_DWELLING_SLOTS + monsterLevel];
    upgradedCreature = upgradedValue;
    if (townArmy->getCreatureTotal(upgradedCreature))
        creature = upgradedCreature;

    long amount = g_creatureTypeTraits[creature].m_growthRate;
    if (!townArmy->canJoin(creature)) {
        long worstArmy = -1;
        long worstValue = g_creatureTypeTraits[creature].m_aiValue * amount;
        for (long slot = 0; slot < armyGroup::ARMY_GROUP_SLOT_COUNT; ++slot) {
            long value = g_creatureTypeTraits[townArmy->m_armies[slot]].m_aiValue
                       * townArmy->m_numTroops[slot];
            if (value < worstValue) {
                worstArmy = slot;
                worstValue = value;
            }
        }
        if (worstArmy < 0)
            return;
        townArmy->dismiss(worstArmy);
    }

    townArmy->add(creature, amount, -1);
    if (currentTown->m_population[monsterLevel] < amount)
        currentTown->m_population[monsterLevel] = 0;
    else
        currentTown->m_population[monsterLevel] -= amount;

    int upgradedSlot = monsterLevel + TOWN_DWELLING_COUNT;
    if (currentTown->m_population[upgradedSlot] < amount)
        currentTown->m_population[upgradedSlot] = 0;
    else
        currentTown->m_population[upgradedSlot] -= amount;

    if (creature != upgradedCreature && random(1, 100) <= 5) {
        for (long slot = 0; slot < armyGroup::ARMY_GROUP_SLOT_COUNT; ++slot) {
            if (townArmy->m_armies[slot] == creature)
                townArmy->m_armies[slot] = upgradedCreature;
        }
    }
}

// Original: game::GiveTroopsToNeutralTowns; game.cpp:4029, dc 0xaa6f8
void game::giveTroopsToNeutralTowns()
{
    for (int i = 0; i < m_towns.size(); ++i) {
        if (m_towns[i].m_owner < 0) {
            if (m_towns[i].isCastle()) {
                if (random(0, 100)
                    < g_neutralTownFortifiedReinforcementChance)
                    giveTroopsToNeutralTown(i);
            } else {
                if (random(0, 100)
                    < g_neutralTownOpenReinforcementChance)
                    giveTroopsToNeutralTown(i);
            }
        }
    }
}

// E:\gamedcs\game.cpp:4050
// Retail's campaign chain cross-jumps every `AllowNormalVictory = 0` tail
// into ONE store at 0x4bf835 and shares a single `je` at 0x4bf840, so each
// arm ends `cmp eax,<last>` + `jmp <shared je>` and the `= 1` store is the
// fall-through.  That is the polarity of `if (map != K) = 1; else = 0;`.
// Spelling the CAMPAIGN_5/3 arm that way is worth 89.0407 -> 89.6120: it
// deletes our odd `mov byte ptr [ecx+0x1f89d], al` (the `test eax,eax` zero
// reused as the stored value) and puts retail's `test eax,eax` at the tail.
// The same inversion on the multi-value arms (7, 14, 15, 16, 18) is
// BYTE-FLAT - VC6 canonicalises `a||b||c` and `!a&&!b&&!c` to one shape - so
// those stay in their positive form.  On the single-value arms (CAMPAIGN_2,
// CAMPAIGN_8) it is a LOSS (89.61 -> 87.49 -> 86.15) and the whole-chain
// inversion scores 86.15: dropping their stores removes a pseudo, and the
// two `type_point` stack slots SWAP (retail and this compile both put
// vchero_loc at [ebp-8] and poolhero_loc at [ebp-0x10]; after the extra
// inversions they trade, which re-displaces ~50 downstream rows).  The
// residual is therefore C2 cross-jump aggressiveness (4 `= 0` stores here
// against retail's 1) plus retail's UNMERGED num_living_players store, and
// no arm spelling reaches it without paying the slot swap.
// The final valid-town path can return directly at unchanged 90.0315%.
// Full do/for failure scopes lose to 87.4352..87.7111%, and the earlier
// result flag gives 89.2426%; these used break as the failure-scope exit.
// These are limits of the tested scopes, not proof of original gotos.
// The town-loss scope uses continue for either failed team check and a
// return for valid ownership. This removes both remaining joins at 90.0315%
// with the full contribution and all relocation names/addends unchanged.
// The earlier failure scopes used break and do not predict this lowering.
VA(0x004bf780, 0x6E2)  // order-map + whole-function identity, dc 0xaa7e0
void game::validateVictoryLossConditions(unsigned char checkMapLocations)
{
    signed char victoryType = m_mapHeader.m_victoryCondition.m_type;
    if (victoryType == VICTORY_CONDITION_ARTIFACT
        || victoryType == VICTORY_CONDITION_BUILD_GRAIL
        || victoryType == VICTORY_CONDITION_TRANSPORT_ARTIFACT) {
        int numLivingPlayers = 0;
        for (int i = 0; i < 8; ++i) {
            if (!m_playerDisabled[i])
                ++numLivingPlayers;
        }

        const int map = m_campaign.m_currentMap;
        int campaignNumber = m_campaign.m_currentCampaign;
        if (numLivingPlayers == 1) {
            m_mapHeader.m_victoryCondition.m_allowNormalVictory = 0;
        } else if (g_inCampaign) {
            if (campaignNumber == GAME_CAMPAIGN_5
                || campaignNumber == GAME_CAMPAIGN_3) {
                if (map != GAME_SCENARIO_0)
                    m_mapHeader.m_victoryCondition.m_allowNormalVictory = 1;
                else
                    m_mapHeader.m_victoryCondition.m_allowNormalVictory = 0;
            } else if (campaignNumber == GAME_CAMPAIGN_2) {
                if (map == GAME_SCENARIO_1)
                    m_mapHeader.m_victoryCondition.m_allowNormalVictory = 0;
                else
                    m_mapHeader.m_victoryCondition.m_allowNormalVictory = 1;
            } else if (campaignNumber == GAME_CAMPAIGN_8) {
                if (map == GAME_SCENARIO_2)
                    m_mapHeader.m_victoryCondition.m_allowNormalVictory = 0;
                else
                    m_mapHeader.m_victoryCondition.m_allowNormalVictory = 1;
            } else if (campaignNumber == GAME_CAMPAIGN_7) {
                if (map == GAME_SCENARIO_1 || map == GAME_SCENARIO_3
                    || map == GAME_SCENARIO_6)
                    m_mapHeader.m_victoryCondition.m_allowNormalVictory = 0;
                else
                    m_mapHeader.m_victoryCondition.m_allowNormalVictory = 1;
            } else if (campaignNumber == GAME_CAMPAIGN_15) {
                if (map == GAME_SCENARIO_1 || map == GAME_SCENARIO_2
                    || map == GAME_SCENARIO_3)
                    m_mapHeader.m_victoryCondition.m_allowNormalVictory = 0;
                else
                    m_mapHeader.m_victoryCondition.m_allowNormalVictory = 1;
            } else if (campaignNumber == GAME_CAMPAIGN_18) {
                if (map == GAME_SCENARIO_1 || map == GAME_SCENARIO_8
                    || map == GAME_SCENARIO_9)
                    m_mapHeader.m_victoryCondition.m_allowNormalVictory = 0;
                else
                    m_mapHeader.m_victoryCondition.m_allowNormalVictory = 1;
            } else if (campaignNumber == GAME_CAMPAIGN_16) {
                if (map == GAME_SCENARIO_1 || map == GAME_SCENARIO_2
                    || map == GAME_SCENARIO_3)
                    m_mapHeader.m_victoryCondition.m_allowNormalVictory = 0;
                else
                    m_mapHeader.m_victoryCondition.m_allowNormalVictory = 1;
            } else if (campaignNumber == GAME_CAMPAIGN_14) {
                if (map == GAME_SCENARIO_2 || map == GAME_SCENARIO_3
                    || map == GAME_SCENARIO_4)
                    m_mapHeader.m_victoryCondition.m_allowNormalVictory = 0;
                else
                    m_mapHeader.m_victoryCondition.m_allowNormalVictory = 1;
            } else {
                m_mapHeader.m_victoryCondition.m_allowNormalVictory = 1;
            }
        } else {
            m_mapHeader.m_victoryCondition.m_allowNormalVictory = 1;
        }
    }

    if (victoryType == VICTORY_CONDITION_DEFEAT_HERO)
        m_mapHeader.m_victoryCondition.m_allowNormalVictory = 1;

    if (!checkMapLocations)
        return;

    VictoryConditionStruct& victory = m_mapHeader.m_victoryCondition;
    if (victoryType == VICTORY_CONDITION_DEFEAT_HERO) {
        int* victoryHeroLocation = &victory.m_heroX;
        type_point vcheroLoc(victoryHeroLocation[0],
                              victoryHeroLocation[1],
                              victoryHeroLocation[2]);
        victory.m_heroId = -1;
        for (int i = 0; i < HERO_COUNT; ++i) {
            type_point poolheroLoc(m_heroes[i].m_x, m_heroes[i].m_y, m_heroes[i].m_z);
            if (vcheroLoc.operator==(poolheroLoc)) {
                int team = m_heroes[i].m_owner;
                if (team >= 0)
                    team = m_mapHeader.m_teamInfo[team];
                if (team >= 0 && isHumanTeam(team)) {
                    victory.m_type = -1;
                    break;
                }
                victory.m_heroId = i;
                break;
            }
        }
        if (victory.m_heroId == -1)
            victory.m_type = -1;
    }

    if (victory.m_type == VICTORY_CONDITION_DEFEAT_MONSTER) {
        const NewmapCell* thisCell = m_worldMap.cell(
            victory.m_monsterX, victory.m_monsterY, victory.m_monsterZ);
        if (thisCell->m_type == MONSTER && thisCell->m_isTrigger) {
            {
                victory.m_creatureType = TCreatureType(thisCell->m_objectIndex);
            }
        } else {
            victory.m_type = -1;
            victory.m_creatureType = CREATURE_NONE;
            victory.m_allowNormalVictory = 1;
        }
    }

    if (victory.m_type == VICTORY_CONDITION_CAPTURE_TOWN) {
        town* thisTown = getTown(getTownId(
            victory.m_townX, victory.m_townY, victory.m_townZ));
        int team = thisTown->m_owner;
        if (team >= 0)
            team = m_mapHeader.m_teamInfo[team];
        if (team >= 0 && isHumanTeam(team))
            victory.m_type = -1;
    }

    LossConditionStruct& loss = m_mapHeader.m_lossCondition;
    if (loss.m_type == LOSS_CONDITION_LOSE_HERO) {
        type_point lcheroLoc(loss.m_heroX, loss.m_heroY, loss.m_heroZ);
        loss.m_heroId = -1;
        for (int i = 0; i < HERO_COUNT; ++i) {
            type_point poolheroLoc(m_heroes[i].m_x, m_heroes[i].m_y, m_heroes[i].m_z);
            if (lcheroLoc.operator==(poolheroLoc)) {
                int numHumanTeams = 0;
                for (int team = 0; team < 8; ++team) {
                    if (isHumanTeam(team))
                        ++numHumanTeams;
                }
                if (numHumanTeams <= 1) {
                    int team = m_heroes[i].m_owner;
                    if (team >= 0)
                        team = m_mapHeader.m_teamInfo[team];
                    unsigned char humanTeam = 0;
                    if (team >= 0)
                        humanTeam = isHumanTeam(team);
                    if (team < 0 || humanTeam) {
                        loss.m_heroId = i;
                        break;
                    }
                }
                loss.m_type = -1;
                break;
            }
        }
        if (loss.m_heroId == -1)
            loss.m_type = -1;
    }

    if (loss.m_type == LOSS_CONDITION_LOSE_TOWN) {
        town* thisTown = getTown(getTownId(
            loss.m_townX, loss.m_townY, loss.m_townZ));
        int numHumanTeams = 0;
        int owner;
        int townTeam;
        for (unsigned int teamCheck = 0; teamCheck < 8; ++teamCheck) {
            if (isHumanTeam(teamCheck))
                ++numHumanTeams;
        }
        do {
            if (numHumanTeams > 1)
                continue;
            owner = thisTown->m_owner;
            townTeam = owner;
            if (townTeam >= 0)
                townTeam = m_mapHeader.m_teamInfo[townTeam];
            if (townTeam >= 0) {
                unsigned char humanTeam =
                    isHumanTeam(townTeam);
                if (!humanTeam)
                    continue;
            }
            if (owner != -1)
                return;
        } while (0);
        loss.m_type = -1;
    }
}

// E:\gamedcs\game.cpp:4236
VA(0x004bfe70, 0x6A8)  // dc 0xaada4
void game::newMap(TAbstractFile* mapFile, int* playerHeroFaces,
                  TCampaignBrief::ScenarioStruct* campaignContext, int gameVersion)
{
    g_inSetup = 1;

    randomizeHeroPool();

    m_numPlayers = 8;
    m_numDeadPlayers = 0;
    if (gameVersion != -1 && !g_inCampaign) {
        m_gameVersion = gameVersion;
    } else {
        m_gameVersion = 2;
        if (g_inCampaign) {
            if (m_campaign.m_currentCampaign < g_firstArmageddonsBladeCampaign)
                m_gameVersion = 0;
            else if (m_campaign.m_currentCampaign < 13)
                m_gameVersion = 1;
        }
    }

    if (playerHeroFaces != NULL) {
        for (int facePlayer = 0; facePlayer < 8; ++facePlayer) {
            if (m_players[facePlayer].m_isHuman
                && m_mapHeader.m_playerSlotAttributes[facePlayer].m_generateHero) {
                int heroId = playerHeroFaces[facePlayer];
                if (heroId != -1) {
                    m_heroAvailability[heroId] = static_cast<char>(facePlayer);
                    if (g_game->m_setup.m_startingHero[facePlayer] == -1)
                        g_game->m_setup.m_startingHero[facePlayer] = heroId;
                }
            }
        }
    }

    loadMap(mapFile);

    for (int playerIndex = 0; playerIndex < 8; ++playerIndex) {
        m_players[playerIndex].m_color = static_cast<signed char>(playerIndex);
        m_players[playerIndex].m_numTowns = 0;
        m_players[playerIndex].m_currTownId = -1;
        m_players[playerIndex].m_numHeroes = 0;
        m_players[playerIndex].m_currHeroId = -1;
    }

    clearEventRecords();
    initRandomArtifacts();
    processRandomObjects();
    randomizeEvents();
    matchUndergroundGates();
    randomizeHolyGrail();
    processOnMapTowns();
    aiExamineMap();
    if (campaignContext != NULL)
        g_game->m_campaign.doPreLoadCustomization();
    processOnMapHeroes();
    if (campaignContext != NULL)
        campaignContext->placeCrossoverHeroes();
    createTownHeroes(playerHeroFaces);

    for (unsigned int mapDataIndex = 0;
         mapDataIndex < m_worldMap.m_mapObjectData.size(); ++mapDataIndex) {
        m_worldMap.m_mapObjectData[mapDataIndex]->newMapVFn38();
    }

    memset(m_playerDisabled, 0, sizeof(m_playerDisabled));
    for (int disabledPlayer = 0; disabledPlayer < 8; ++disabledPlayer)
        m_playerDisabled[disabledPlayer] =
            m_players[disabledPlayer].m_numHeroes == 0
            && m_players[disabledPlayer].m_numTowns == 0;

    m_week = 1;
    setRecruits();

    validateVictoryLossConditions(1);

    if (g_inCampaign && m_campaign.m_currentCampaign == GAME_CAMPAIGN_14) {
        hero* campaignHero = &m_heroes[45];
        if (campaignHero->getArtifact(TArtifactSlot(hero::EQUIPPED_SLOT_SPELLBOOK)).m_artifactId
            != -1)
            campaignHero->removeArtifact(hero::EQUIPPED_SLOT_SPELLBOOK);
        if (m_campaign.m_currentMap == GAME_SCENARIO_2) {
            type_artifact alliance(ARTIFACT_ANGELIC_ALLIANCE);
            campaignHero->giveArtifact(&alliance, 0, 0);
        }
    }

    for (int setupPlayer = 0; setupPlayer < 8; ++setupPlayer) {
        if (m_playerDisabled[setupPlayer])
            continue;

        if (m_players[setupPlayer].m_isHuman) {
            m_players[setupPlayer].m_personality = 3;
            memcpy(m_players[setupPlayer].m_resources,
                   g_initResourcesHuman[m_setup.m_difficulty],
                   sizeof(m_players[setupPlayer].m_resources));
            if (m_isTutorial)
                memcpy(m_players[setupPlayer].m_resources,
                       g_neutralTownLevelWeights + 6,
                       sizeof(m_players[setupPlayer].m_resources));
        } else {
            m_players[setupPlayer].m_personality = random(0, 2);
            memcpy(m_players[setupPlayer].m_resources,
                   g_initResourcesComputer[m_setup.m_difficulty],
                   sizeof(m_players[setupPlayer].m_resources));
        }

        if (!g_inCampaign) {
            int bonus = g_newMapStartingBonus[setupPlayer];
            bool hasHero = true;
            if (getHero(m_players[setupPlayer].m_heroes[0]) == NULL)
                hasHero = false;
            if (bonus == NEW_MAP_BONUS_RANDOM) {
                if (hasHero)
                    bonus = random(0, 2);
                else
                    bonus = random(1, 2);
            }
            g_game->m_setup.m_startingBonus[setupPlayer] =
                static_cast<signed char>(bonus);

            switch (bonus) {
            case NEW_MAP_BONUS_ARTIFACT: {
                int heroId = g_game->m_setup.m_startingHero[setupPlayer];
                if (heroId == -1)
                    heroId = m_players[setupPlayer].m_heroes[0];
                hero* bonusHero = getHero(heroId);
                if (bonusHero != NULL) {
                    type_artifact artifact(getRandomArtifactId(2));
                    bonusHero->giveArtifact(&artifact, 1, 1);
                }
                break;
            }
            case NEW_MAP_BONUS_GOLD:
                m_players[setupPlayer].m_resources[GOLD] += random(5, 10) * 100;
                break;
            case NEW_MAP_BONUS_RESOURCE: {
                int amount = random(3, 6);
                switch (m_setup.m_alignment[setupPlayer]) {
                case TOWN_CASTLE:
                case TOWN_NECROPOLIS:
                case TOWN_STRONGHOLD:
                case TOWN_FORTRESS: {
                    amount = random(5, 10);
                    m_players[setupPlayer].m_resources[WOOD] += amount;
                    m_players[setupPlayer].m_resources[ORE] += amount;
                    break;
                }
                case TOWN_RAMPART:
                    m_players[setupPlayer].m_resources[CRYSTAL] += amount;
                    break;
                case TOWN_TOWER:
                    m_players[setupPlayer].m_resources[GEMS] += amount;
                    break;
                case TOWN_INFERNO:
                case TOWN_CONFLUX:
                    m_players[setupPlayer].m_resources[MERCURY] += amount;
                    break;
                case TOWN_DUNGEON:
                    m_players[setupPlayer].m_resources[SULFUR] += amount;
                    break;
                }
                break;
            }
            }
        }

        if (m_setup.m_handicap[setupPlayer]) {
            for (int resource = 0; resource < NUM_RESOURCES; ++resource) {
                double handicap;
                if (m_setup.m_handicap[setupPlayer] == NEW_MAP_HANDICAP_MILD)
                    handicap = 0.85;
                else
                    handicap = 0.7;
                m_players[setupPlayer].m_resources[resource] =
                    static_cast<long>(
                        m_players[setupPlayer].m_resources[resource] * handicap);
            }
        }
    }

    if (campaignContext != NULL)
        campaignContext->giveCrossoverArtifacts();

    setupAdjacentMons();
    setupNewRumour();

    setMarketArtifacts();

    g_inSetup = 0;
}

// Retail-only PC wrapper between the DC NewMap and SetupFirstPlayer rows.
// Its two callers pass a directory, filename, optional eight-hero override,
// and game-version selector; the body constructs the zlib stream and hands
// exactly those latter two values to the stream-based NewMap overload.
// WALL 99.4%: all 100 explicit instructions, all three returns and the whole
// try/catch funclet graph agree.  The sole byte-level residual is VC6's hidden
// try-state zero at ebp-4: retail reuses the already-zero EAX (`mov [ebp-4],
// eax`), while rebuilding the same source emits the equivalent immediate-zero
// store.  This is compiler EH bookkeeping, not an unrecovered source action.
// Four stream/exit-lifetime candidates produced three reproduced objects:
// a separate stream scope and success after try remain 99.4000%; a shared
// success local drops to 85.4000%. No source alternative is adopted.
VA(0x004c0520, 0x106)  // anchor-callers + contiguous catch funclets, retail-only
unsigned char game::newMap(const char* mapPath, const char* mapName,
                           int* playerHeroFaces, int gameVersion)
{
    try {
        strcpy(g_text, mapPath);
        strcat(g_text,
               DATA_COMPGEN(0x00677dac, newMapPathSeparator, "\\"));
        strcat(g_text, mapName);

        TGzFile mapFile(
            g_text, DATA_COMPGEN(0x00677d6c, newMapGzReadMode, "rb"));
        newMap(&mapFile, playerHeroFaces, NULL, gameVersion);
        return 1;
    } catch (TGzFile::TOpenFailure) {
        return 0;
    }
}

// Original: SetupFirstPlayer; game.cpp:4474, dc 0xab8d0.
// Complete makes this a game member: 0x4c0632 saves incoming ECX and
// indexes the player array through that receiver throughout the body.
VA(0x004c0630, 0xB1)  // dc 0xab8d0
void game::setupFirstPlayer()
{
    // DC locals: startingPos, localPlayer. The calls at dc 0xab8f8 and
    // 0xab942 name IsHuman and GetLocalPlayerGamePos. Retail 0x4c0636
    // expands IsHuman's clamp; 0x4c067a..0x4c06c7 expands the protocol
    // test and descending human scan of GetLocalPlayerGamePos. The latter
    // reads g_netLocalGamePos just written below; it needs no extra formal
    // or separate setupFirstPlayerPosition source helper.
    int startingPos = 0;
    while (startingPos < 8) {
        if (isHuman(startingPos))
            break;
        ++startingPos;
    }

    g_netLocalGamePos = startingPos;
    g_currentPlayer = &m_players[startingPos];
    g_curPlayerBit = static_cast<unsigned char>(1 << startingPos);

    int currentPlayer = getLocalPlayerGamePos();
    g_curWatchPlayer = currentPlayer;
    g_mapVisibilityBit = static_cast<unsigned char>(1 << currentPlayer);
    g_playerTurn = startingPos;
}
// E:\gamedcs\game.cpp:4509. On x86 the unchanged award, secondary skill
// and spell stores fold away, leaving only the randomized primary lane.
static void randomizeScholar(NewmapCell* cell)
{
    ExtraInfoUnion* info = static_cast<ExtraInfoUnion*>(
        static_cast<void*>(&cell->m_extraInfo));
    if (info->getScholarAward() != const_scholar_primary_skill) {
        // DC game.cpp:4515 keeps Random inside the SetScholar expression.
        info->setScholar(info->getScholarAward(),
            TPrimarySkill(random(0, 3)),
            info->getScholarSecondarySkill(), info->getScholarSpell());
    }
}

// Complete's ARTIFACT arm at 0x4c0cc0 retains the customization test,
// Random call and low-nibble clear; the DC guarded-artifact machinery is absent.
// E:\gamedcs\game.cpp:4524, dc 0xab9d4
static void randomizeArtifact(NewmapCell* cell)
{
    if (!cell->isCustomized()) {
        random(0, 99);
        cell->m_extraInfo &= 0xfffffff0;
    }
}

// E:\gamedcs\game.cpp:4613.
static void randomizeSeaChest(NewmapCell* cell)
{
    int i = random(0, 99);
    if (i < 20) {
        cell->m_seaChestInfo.m_reward = 0;
    }
    else if (i < 90) {
        cell->m_seaChestInfo.m_reward = 1;
    }
    else {
        cell->m_seaChestInfo.m_reward = 2;
        cell->m_seaChestInfo.m_artifact =
            g_game->getRandomArtifactId(2);
    }
}

// E:\gamedcs\game.cpp:4639. Retail inlines all three constant-level calls
// into RandomizeEvents, but keeps the Dinkumware bitset operations out of
// line. The subscript/reference spelling is visible in the retail call pair:
// bitset::operator[] followed by _Bit_reference::operator=.
// Complete takes a bitset instead of DC's integer level. Default construction
// corresponds to retail's _Tidy(0) call. Unpinned constructor controls measured
// 87.25% (default) / 87.71% (explicit zero) for RandomizeEvents, versus 88.86%
// pinned; the default also emits the exact retained resource SetWagon body.
// An integer compatibility-overload probe reached only 84.12% and did not
// establish that Complete retained DC's old interface; it is not adopted.
static void randomizeShrine(NewmapCell* cell, const int level)
{
    ExtraInfoUnion* info = static_cast<ExtraInfoUnion*>(
        static_cast<void*>(&cell->m_extraInfo));
    SpellID spell = info->m_shrineInfo.m_spell;
    if (spell == -1) {
        std::bitset<5> spellLevels;
        spellLevels[level] = true;
        spell = g_game->getRandomSpell(spellLevels);
        info->m_shrineInfo.m_spell = spell;
    }
    info->clearVisitedBits();
}

// E:\gamedcs\game.cpp:4654, dc 0xabda8
// RandomizeEvents expands this ordinary static helper.
static void randomizeWagon(NewmapCell* cell)
{
    int i = random(0, 99);
    // DC game.cpp:4662 has both Random calls in SetWagon's expression.
    // Keep them there; the conversion itself has no recovered helper.
    cell->setWagon(EGameResource(random(0, 5)),
        static_cast<short>(random(2, 5)));
    if (i < 10)
        cell->emptyWagon();
    else if (i < 50) {
        TArtifact artifact = g_game->getRandomArtifactId(6);
        cell->setWagon(artifact);
    }
}

// DC 4682..4684 writes the id, clears visit bits, then draws the price.
// Complete's TREE_OF_KNOWLEDGE arm preserves those same packed lanes.
// E:\gamedcs\game.cpp:4681, dc 0xabe30
static void randomizeWiseTree(short id, NewmapCell* cell)
{
    cell->m_extraInfo = (cell->m_extraInfo & 0xffffffe0) | (id & 0x1f);
    cell->m_extraInfo &= 0xffffe01f;
    int price = random(0, 2);
    cell->m_extraInfo = (cell->m_extraInfo & 0xffff1fff) | ((price & 7) << 13);
}

// E:\gamedcs\game.cpp:4691.
static void randomizeTreasure(NewmapCell* cell)
{
    int i = random(0, 99);
    if (g_game->m_isTutorial)
        i = 60;
    cell->m_treasureInfo.m_hasArtifact = 0;
    if (i < 32)
        cell->m_treasureInfo.m_gold = 2;
    else if (i < 64)
        cell->m_treasureInfo.m_gold = 3;
    else if (i < 95)
        cell->m_treasureInfo.m_gold = 4;
    else {
        cell->m_treasureInfo.m_artifact =
            g_game->getRandomArtifactId(2);
        cell->m_treasureInfo.m_hasArtifact = 1;
    }
}

// CodeView names separate i/level locals and the set_tomb boundary. Retail
// 0x4c1b01 reloads gpGame for the artifact draw and expands the packed setter.
// E:\gamedcs\game.cpp:4724, dc 0xabf78
static void randomizeTomb(NewmapCell* cell)
{
    int level;
    int i = random(0, 99);
    if (i < 30)
        level = 2;
    else if (i < 80)
        level = 4;
    else if (i < 95)
        level = 8;
    else
        level = 16;
    ExtraInfoUnion* info = static_cast<ExtraInfoUnion*>(
        static_cast<void*>(&cell->m_extraInfo));
    info->setTomb(g_game->getRandomArtifactId(level));
}

// E:\gamedcs\game.cpp:4753. The vector local and its teardown belong to the
// inlined source helper; retail calls only the packed pyramid setter.
// Ownership probe: the MapCell.h body at 0x4c2330 is currently fully
// expanded here. Replacing this helper's forced-inline spelling with ordinary static
// did not recover the retained call; the fatal header-emission gate remains.
static void randomizePyramid(NewmapCell* cell)
{
    std::vector<int> possibleSpells;
    int i;
    for (i = 0; i < 70; ++i) {
        if (g_spellTraits[i].m_school != const_invalid_school
            && g_spellTraits[i].m_level == g_pyramidSpellLevel
            && !g_game->m_ssDisabled[i])
            possibleSpells.push_back(i);
    }

    SpellID spell = SpellID(
        possibleSpells[random(0, possibleSpells.size() - 1)]);
    cell->setPyramid(true, spell);
    cell->clearVisitedBits();
}

// E:\gamedcs\game.cpp:4770
// Retail builds one availability bit per secondary skill from the scenario's
// disabled-skill row, draws four distinct set bits, and appends those four
// skills as one native university record. DC's local and aggregate type agree
// with retail when Conflux's initializer uses its dedicated type.
// WALL 99.7464%: all 24 blocks, every branch target and every instruction
// count agree.  The explicit-code residual is one whole-loop register tie:
// retail keeps the cached availability bound in EBX and each Random ordinal
// in EDI; this compile assigns those two non-overlapping values the other way
// round.  Dreamcast CodeView attests university/used/choice/i and the
// TSecondarySkill type of skill.  Restoring its indexed i loop raised 93.08
// -> 99.75; restoring the shared local order and enum type is byte-flat and
// source-shape-ratcheted. The 36-state insertion family isolates the old
// ownership error: a native record with automatic elemental initialization
// falls to 97.8623%. The 13-state initializer family restores the native local
// at 99.7464%, removes the pointer union, and preserves all Conflux call sites.
// Remaining insert boundary: DC's push_back can expose the count-insert child
// through the vendor wrapper; retail calls that child. Public single-insert
// and push_back controls score 99.0145/97.5362; removing the existing fence
// expands the child for all three APIs (0% large-body comparisons). Retain
// this boundary debt, not a claim that the DC wrapper itself was absent.
// why-reg confirms equal pseudo-definition slots but a different C1 processing
// order.  Exhausted byte-inert levers: reset vs set(false), int/unsigned/
// register bounds, cached-count vs explicit-highest formulations, split
// declaration/initialization orders, and pre/post-reset induction ordering.
// The direct count-in-loop spelling duplicates the popcount loop and falls to
// 87.05.
VA(0x004c06f0, 0x179)  // dc-order + member receiver, dc 0xac048
void game::randomizeUniversity(NewmapCell* cell)
{
    type_university university;
    std::bitset<28> availableSkills;
    long choice;
    long i;
    TSecondarySkill skill;
    for (i = 0; i < 28; ++i)
        availableSkills[i] = !g_game->m_ssDisabled[i];

    int availableCount = availableSkills.count();
    for (i = 0; i < 4; ++i) {
        choice = random(0, availableCount - 1);
        skill = eSecSkillPathfinding;
        for (;;) {
            if (!availableSkills.test(skill)) {
                {
                    skill = TSecondarySkill(skill + 1);
                }
                continue;
            }
            if (choice == 0)
                break;
            --choice;
            {
                skill = TSecondarySkill(skill + 1);
            }
        }

        university.m_skills[i] = skill;
        availableSkills.set(skill, false);
        --availableCount;
    }

    const unsigned long cellVisitedBits = 0x00001fe0;
    const unsigned long universityIndexBits = 0x01ffe000;
    cell->m_extraInfo &= ~cellVisitedBits;
    unsigned long universityIndex = m_universities.size() & 0xfff;
    cell->m_extraInfo = (cell->m_extraInfo & ~universityIndexBits)
        | (universityIndex << 13);
    m_universities.push_back(university);
}

// E:\gamedcs\game.cpp:4804. Like the shrine helper, this has no PC row:
// /Ob2 expands it into RandomizeEvents while leaving bitset's non-trivial
// operations as calls. DC proves the TSecondarySkill local but predates
// Complete's mask filtering; retail's proxy-call sequence selects operator[].
// A 16-state constructor/type/unpin control produced 10 reproduced objects.
// Removing the loop pin gives about 82% for RandomizeEvents; removing the
// setter pin also loses the exact retained SetWitchSkill body. Both remain
// unresolved debt. Default construction of the inverted zero temporary
// preserves retail's _Tidy(0) boundary instead of a value-constructor call.
static void randomizeWitchHut(NewmapCell* cell)
{
#pragma inline_depth(0)
    std::bitset<28> possibleSkills(cell->m_extraInfo);
    cell->m_extraInfo = 0;
    if (!possibleSkills.any())
        possibleSkills = ~std::bitset<28>();

    int i;
    for (i = 0; i < 28; ++i)
        possibleSkills[i] = possibleSkills[i]
            && !g_game->m_ssDisabled[i];

    TSecondarySkill skill;
    int count = possibleSkills.count();
    if (count < 1) {
        skill = eSecSkillNone;
    }
    else {
        int choice = random(1, count);
        for (skill = eSecSkillPathfinding; skill < kNumSecSkills;
             skill = TSecondarySkill(skill + 1)) {
            if (possibleSkills[skill] && --choice < 1)
                break;
        }
    }
#pragma inline_depth()
    ExtraInfoUnion* info = static_cast<ExtraInfoUnion*>(
        static_cast<void*>(&cell->m_extraInfo));
#pragma inline_depth(0)
    info->setWitchSkill(skill);
#pragma inline_depth()
}

VA(0x004c0870, 0x22A)  // dc 0xac1a4
void game::randomizeHolyGrail()
{
    if (m_ultimateRadius == 0 && m_ultimateArtifactX != -1) {
        m_ultimateArtifactPresent = 1;
        return;
    }

    if (m_ultimateArtifactX == -1) {
        if (m_numObelisks <= 0)
            return;
        m_ultimateArtifactX = g_mapWidth / 2;
        m_ultimateArtifactY = g_mapHeight / 2;
        m_ultimateArtifactZ = random(1, m_worldMap.getNumLevels()) - 1;
        m_ultimateRadius = 0x7f;
    }

    int numValidCells = 0;
    int ultimateXLow = m_ultimateArtifactX - m_ultimateRadius;
    int ultimateXHigh = m_ultimateArtifactX + m_ultimateRadius;
    int ultimateYLow = m_ultimateArtifactY - m_ultimateRadius;
    int ultimateYHigh = m_ultimateArtifactY + m_ultimateRadius;

    if (ultimateXLow < 10)
        ultimateXLow = 10;
    if (ultimateXHigh > g_mapWidth - 10)
        ultimateXHigh = g_mapWidth - 10;
    if (ultimateYLow < 9)
        ultimateYLow = 9;
    if (ultimateYHigh > g_mapWidth - 9)
        ultimateYHigh = g_mapWidth - 9;

    for (int z = 0; z < m_worldMap.getNumLevels(); ++z) {
        for (int x = ultimateXLow; x <= ultimateXHigh; ++x) {
            for (int y = ultimateYLow; y <= ultimateYHigh; ++y) {
                NewmapCell* tempCell =
                    m_worldMap.cell(x, y, m_ultimateArtifactZ);
                if (tempCell->isDiggable())
                    ++numValidCells;
            }
        }
        if (numValidCells > 0)
            break;
        m_ultimateArtifactZ = 1 - m_ultimateArtifactZ;
    }

    int i = random(0, numValidCells - 1);
    numValidCells = 0;
    for (int x = ultimateXLow; x <= ultimateXHigh; ++x) {
        for (int y = ultimateYLow; y <= ultimateYHigh; ++y) {
            NewmapCell* tempCell = m_worldMap.cell(x, y, m_ultimateArtifactZ);
            if (tempCell->isDiggable()) {
                if (numValidCells == i) {
                    m_ultimateArtifactX = static_cast<short>(x);
                    m_ultimateArtifactY = static_cast<short>(y);
                    m_ultimateArtifactPresent = 1;
                    return;
                }
                ++numValidCells;
            }
        }
    }
}

VA(0x004c0aa0, 0xBE)  // dc 0xac494
void game::initRandomArtifacts()
{
    std::copy(m_artifactDisabled,
              m_artifactDisabled + sizeof(m_artifactDisabled), m_artifactUsed);

    int z;
    int x;
    int y;
    for (z = 0; z < getNumMapLevels(); ++z) {
        for (x = 0; x < g_mapWidth; ++x) {
            for (y = 0; y < g_mapHeight; ++y) {
                NewmapCell* tempCell = m_worldMap.cell(x, y, z);
                if (tempCell->m_type == ARTIFACT && tempCell->m_isTrigger)
                    m_artifactUsed[tempCell->m_objectIndex] = 1;
            }
        }
    }
}

// Mac 0:0xd7160..0xd72e0 (384 bytes, SHA-256
// 54d578f01c668bcfb2e78f7cb9c1e665d170d14e048b9873d200a7295b79300a)
// is the same gate-pairing body: it walks the exit/pair arrays, rejects equal
// packed z coordinates, squares x/y differences, calls MathLib sqrt once,
// then stores both partner indices. Its sole caller at 0xd5ce4 sits between
// randomizeEvents and randomizeHolyGrail in newMap's call sequence. The
// preceding/following BLR/prologue bound the admitted Mac target. The current
// shared-source O3 candidate resolves the MathLib sqrt call and conversion
// constant but remains 408 versus 384 bytes (24.02%); the game declaration
// view and compiler profile still need calibration on another body.
// Isolated Mac declaration probes: a word-copy point view makes 392 bytes
// (29.08%), but retains different vector access and register allocation;
// the short-bitfield view alone emits two halfword stores where retail emits
// one word store. Layering the installed MSL vector accessors was byte-flat.
// Neither unverified view replaced the admitted declaration. VC6 why-reg's
// eight declaration/store-order controls were flat or worse at 94.67%.
// E:\\gamedcs\\game.cpp:4950
VA(0x004c0b60, 0x160)  // dc-order + NewMap caller, dc 0xac63c
void game::matchUndergroundGates()
{
    long distance;
    type_point currentGate;
    long i;
    long j;
    type_point exitPoint;
    long bestDistance = 0;
    long closest;

    for (i = 0; i + 1 < m_undergroundGateExits.size(); ++i) {
        if (m_undergroundGatePairs[i] >= 0)
            continue;

        currentGate = m_undergroundGateExits[i];
        closest = -1;
        for (j = i + 1; j < m_undergroundGateExits.size(); ++j) {
            if (m_undergroundGatePairs[j] >= 0)
                continue;

            exitPoint = m_undergroundGateExits[j];
            if (currentGate.m_z == exitPoint.m_z)
                continue;
            distance = static_cast<long>(sqrt(static_cast<double>(
                (currentGate.m_x - exitPoint.m_x)
                    * (currentGate.m_x - exitPoint.m_x)
                + (currentGate.m_y - exitPoint.m_y)
                    * (currentGate.m_y - exitPoint.m_y))));
            if (closest >= 0 && distance >= bestDistance)
                continue;
            closest = j;
            bestDistance = distance;
        }

        if (closest >= 0) {
            m_undergroundGatePairs[i] = closest;
            m_undergroundGatePairs[closest] = i;
        }
    }
}

// E:\gamedcs\game.cpp:5003
// Assign every stateful adventure-map object its new-game payload.  The
// Dreamcast line table supplies the source identity and local roster; the PC
// switch domain, every call, every constant and every packed-field write are
// recovered from retail 0x4c0cc0.  Five creature-bank cases deliberately own
// separate block locals: retail carries five EH states and five independent
// vector<TArtifact> teardown paths for exactly those records.

// 86.92752% wall, 2026-08-22. The semantic skeleton is closed at 213 blocks
// on both sides (88 candidate branches against retail's 87), and the frame is
// within one dword (`0x334` against `0x330`). The remaining deltas are bounded
// compiler-layout classes: retail places BLACK_BOX's ANY and RELIC arms in the
// hot run while this CL places only ANY there; each bank cleanup expands the
// record destructor by exactly one layer in retail; MAGIC_SPRING recomputes
// its left-cell expression where our equivalent local reuses it; and retail
// inlines bitset-reference conversion while leaving its nested test as a call.
// Moving RELIC in source order, `inline_depth(1)` on the bank cleanups,
// repeating the MAGIC_SPRING expression, and hoisting either Witch reference
// conversion were each measured separately and rejected by the ratchet.
// RE-MEASURED 2026-09-05, and the RELIC verdict now has a mechanism worth
// recording. Retail emits FIVE BLACK_BOX arms but SIX artifact-class call
// sites: ANY (0xe) and RELIC (0x10) sit adjacent in the hot run at
// retail+0x351/+0x361 while TREASURE/MINOR/MAJOR are cross-jumped onto
// SHIPWRECK_SURVIVOR's threshold chain, whose OWN relic arm survives
// separately at retail+0xf7d. Writing RELIC second in the source (ANY,
// RELIC, TREASURE, MINOR, MAJOR) reproduces the hot pair exactly and takes
// the block skeleton from 9 exact / 131 flow-kind to 54 exact / 68 - by
// far the largest structural move available here - but the cross-jumper
// then merges SHIPWRECK's relic arm BACKWARDS into the new hot copy
// instead of keeping both, so one site is still missing and fuzzy reads
// 86.7325 against 86.9275. The residual is which of two identical blocks
// the cross-jumper elects as canonical, not a source fact; re-take this
// pairing if a later change gives the two blocks different predecessors.
// The two remaining REAL call divergences are both over-inlines that need
// a statement pin: retail CALLS ExtraInfoUnion::SetWagon(EGameResource,
// short) at randomize_wagon's first store and ExtraInfoUnion::set_pyramid
// at randomize_pyramid's, and this CL expands both.
// 2026-09-06, polish lane 48. READ predict-inline's census HERE BEFORE
// trusting it: most of its reported divergence is COMDAT NAME FOLDING, not
// an inline decision. VC6 emits one body for every POD-pointer vector and
// one for every bitset whose _Nw agrees, so `~type_creature_bank` x5 +
// `~vector<long>` x1 IS retail's `~vector<widget*>` x6; `bitset<5>::
// operator[]` x3 + `bitset<28>::operator[]` x3 IS retail's `bitset<145>::
// operator[]` x6; `vector<type_point>::insert` x4 + `vector<long>::insert`
// x1 IS retail's `vector<widget*>::insert` x5; and the two one-argument
// inserts pair off likewise. All four net to zero.
// The REAL frontier deltas, after that reduction, are four: (1) four bitset
// constructor sites where retail expands the ctor and calls `_Tidy`
// (bitset<5> x3, bitset<28> x1) while the depth-0 pins here emit a ctor
// call instead - the same midpoint LoadMap's note describes, and the same
// candidate fix; (2) two `reference::operator bool` sites where retail
// expands the conversion and calls `test`; (3) SetWagon(EGameResource,
// short) and set_pyramid, both already recorded above; (4) the RELIC arm.
// GetRandomArtifactId's 17-vs-18 call census IS that RELIC arm and NOT a
// missing statement - verified by disassembly: retail's jump table at
// +0x14a dispatches BLACK_BOX's five arms, keeps ANY (`push 0xe`, +0x155)
// and RELIC (`push 0x10`, +0x165) as its own hot pair and cross-jumps
// TREASURE/MINOR/MAJOR away, then runs the seven-call BLACK_MARKET record
// (+0x175..+0x1cf) into vector<TBlackMarket>::insert - nine calls there
// against our eight, with the other nine sites in each object agreeing
// exactly. Do not go looking for an eighteenth source call site.
// Pin census, each removal measured alone against 87.0102: the five
// creature-bank block-scope pins are NOT interchangeable - four cost
// -0.6891 apiece but the FIRST (the CREATURE_BANK case) is BYTE-FLAT
// across the whole TU and has been removed. The rest of this body's
// roster costs -100 (x2, two helper rows stop existing as separate
// symbols), -10.85, -5.13, -1.01 and -0.88.
VA(0x004c0cc0, 0x1668)  // NewMap caller + dc order, dc 0xac910
void game::randomizeEvents()
{
    unsigned long numLithTwoWay = 0;
    unsigned long numMagicSpring = 0;
    unsigned long numLibrary = 0;
    unsigned long numWarriorTomb = 0;
    unsigned long numDefenseTower = 0;
    unsigned long numDeadGuy = 0;
    unsigned long numTrainingGround = 0;
    unsigned long numPowerSchool = 0;
    unsigned long numWhirlpool = 0;
    int x;
    unsigned long numLeanTo = 0;
    int i;
    int y;
    NewmapCell* tempCell;
    int z;
    unsigned long numWarSchool = 0;
    unsigned long numArena = 0;
    unsigned long numTreeOfKnowledge = 0;
    unsigned long numGardenOfRevelation = 0;
    unsigned long numMagicSchool = 0;
    unsigned long numMercCamp = 0;
    int id;
    unsigned long numLithOneWay = 0;
    unsigned long numMysticalGarden = 0;
    TBlackMarket thisMarket;
    int luckBonus;
    unsigned char resQty;
    EGameResource resType;
    NewmapCell::TObjectCell* thisObj;

    const unsigned long visitedBits = 0x00001fe0;
    const unsigned long poolIndexBits = 0x03ffe000;

    for (z = 0; z < getNumMapLevels(); ++z) {
        for (y = 0; y < g_mapHeight; ++y) {
            for (x = 0; x < g_mapWidth; ++x) {
                tempCell = m_worldMap.cell(x, y, z);
                if (!tempCell->m_isTrigger)
                    continue;

                switch (tempCell->m_type) {
                case ARENA:
                    tempCell->m_extraInfo = numArena++;
                    break;

                case ARTIFACT:
                    randomizeArtifact(tempCell);
                    break;

                case BLACK_BOX:
                    if (static_cast<short>(tempCell->m_extraInfo) < 0) {
                        switch (-static_cast<short>(tempCell->m_extraInfo)) {
                        case g_blackBoxRandomAny:
                            tempCell->m_extraInfo = getRandomArtifactId(14);
                            break;
                        case g_blackBoxRandomTreasure:
                            tempCell->m_extraInfo = getRandomArtifactId(2);
                            break;
                        case g_blackBoxRandomMinor:
                            tempCell->m_extraInfo = getRandomArtifactId(4);
                            break;
                        case g_blackBoxRandomMajor:
                            tempCell->m_extraInfo = getRandomArtifactId(8);
                            break;
                        case g_blackBoxRandomRelic:
                            tempCell->m_extraInfo = getRandomArtifactId(16);
                            break;
                        }
                    }
                    break;

                case BLACK_MARKET:
                    thisMarket.m_artifacts[0] = getRandomArtifactId(2);
                    thisMarket.m_artifacts[1] = getRandomArtifactId(2);
                    thisMarket.m_artifacts[2] = getRandomArtifactId(2);
                    thisMarket.m_artifacts[3] = getRandomArtifactId(4);
                    thisMarket.m_artifacts[4] = getRandomArtifactId(4);
                    thisMarket.m_artifacts[5] = getRandomArtifactId(4);
                    thisMarket.m_artifacts[6] = getRandomArtifactId(8);
                    m_blackMarkets.push_back(thisMarket);
                    tempCell->m_extraInfo = m_blackMarkets.size() - 1;
                    break;

                case CAMPFIRE:
                    tempCell->m_extraInfo = random(4, 6) << 4;
                    tempCell->m_extraInfo |= random(0, 5);
                    break;

                case CREATURE_BANK:
                    {
                        tempCell->m_extraInfo &= ~visitedBits;
                        tempCell->m_extraInfo =
                            ((m_creatureBanks.size() & 0xfff) << 13)
                            | (tempCell->m_extraInfo & ~poolIndexBits);
                        type_creature_bank bank;
                        {
                            int converted;
                            converted = tempCell->m_objectIndex;
                            initializeCreatureBank(&bank, type_creature_bank_type(converted));
                        }
                        m_creatureBanks.push_back(bank);
                    }
                    break;

                case CREATURE_GENERATOR_1:
                    id = getGeneratorId(x, y, z);
                    claimGenerator(id, m_generators[id].getOwner());
                    break;

                case g_retailCreatureGenerator2:
                    sprintf(g_text,
                            "CREATURE_GENERATOR_2 found at X=%d Y=%d Z=%d",
                            x, y, z);
                    MessageBoxA(g_hwndApp, g_text, "Invalid Generator", 0);
                    break;

                case g_retailCreatureGenerator3:
                    sprintf(g_text,
                            "CREATURE_GENERATOR_3 found at X=%d Y=%d Z=%d",
                            x, y, z);
                    MessageBoxA(g_hwndApp, g_text, "Invalid Generator", 0);
                    break;

                case CREATURE_GENERATOR_4:
                    id = getGeneratorId(x, y, z);
                    claimGenerator(id, m_generators[id].getOwner());
                    break;

                case DEAD_GUY:
                    {
                        ExtraInfoUnion* info = static_cast<ExtraInfoUnion*>(
                            static_cast<void*>(&tempCell->m_extraInfo));
                        if (random(0, 99) < 20)
                            info->setSkeleton(numDeadGuy++, true,
                                              getRandomArtifactId(6));
                        else
                            info->setSkeleton(numDeadGuy++, false, -1);
                    }
                    break;

                case DEFENSE_TOWER:
                    tempCell->m_extraInfo = numDefenseTower++;
                    break;

                case DERELICT_SHIP:
                    {
                        tempCell->m_extraInfo &= ~visitedBits;
                        tempCell->m_extraInfo =
                            ((m_creatureBanks.size() & 0xfff) << 13)
                            | (tempCell->m_extraInfo & ~poolIndexBits);
                        type_creature_bank bank;
                        initializeCreatureBank(&bank,
                                                 CREATURE_BANK_DERELICT);
                        m_creatureBanks.push_back(bank);
                    }
                    break;

                case SEPULCHER:
                    {
                        tempCell->m_extraInfo &= ~visitedBits;
                        tempCell->m_extraInfo =
                            ((m_creatureBanks.size() & 0xfff) << 13)
                            | (tempCell->m_extraInfo & ~poolIndexBits);
                        type_creature_bank bank;
                        initializeCreatureBank(&bank,
                                                 CREATURE_BANK_SEPULCHER);
                        m_creatureBanks.push_back(bank);
                    }
                    break;

                case SHIPWRECK:
                    {
                        tempCell->m_extraInfo &= ~visitedBits;
                        tempCell->m_extraInfo =
                            ((m_creatureBanks.size() & 0xfff) << 13)
                            | (tempCell->m_extraInfo & ~poolIndexBits);
                        type_creature_bank bank;
                        initializeCreatureBank(&bank,
                                                 CREATURE_BANK_SHIPWRECK);
                        m_creatureBanks.push_back(bank);
                    }
                    break;

                case DRAGON_CITY:
                    {
                        tempCell->m_extraInfo &= ~visitedBits;
                        tempCell->m_extraInfo =
                            ((m_creatureBanks.size() & 0xfff) << 13)
                            | (tempCell->m_extraInfo & ~poolIndexBits);
                        type_creature_bank bank;
                        initializeCreatureBank(&bank,
                                                 CREATURE_BANK_DRAGON);
                        m_creatureBanks.push_back(bank);
                    }
                    break;

                case FLOTSAM:
                    tempCell->m_extraInfo = random(0, 3);
                    break;

                case FOUNTAIN_OF_FORTUNE:
                    {
                        ExtraInfoUnion* info = static_cast<ExtraInfoUnion*>(
                            static_cast<void*>(&tempCell->m_extraInfo));
                        luckBonus = random(0, 3);
                        if (luckBonus == 0)
                            info->m_fountainInfo.m_luck = -1;
                        else
                            info->m_fountainInfo.m_luck = luckBonus;
                        info->clearVisitedBits();
                    }
                    break;

                case GARDEN_OF_REVELATION:
                    tempCell->m_extraInfo = numGardenOfRevelation++;
                    break;

                case GARRISON:
                    {
                        id = tempCell->m_extraInfo;
                        garrison* g = getGarrison(id);
                        // DC game.cpp:5284 calls the ordinary helper. Its
                        // expansion retains CMCClaimGarrison's constructor
                        // without a pin; the standalone helper stays exact.
                        claimGarrison(id, g->m_playerOwner);
                    }
                    break;

                case LEAN_TO:
                    {
                        ExtraInfoUnion* info = static_cast<ExtraInfoUnion*>(
                            static_cast<void*>(&tempCell->m_extraInfo));
                        {
                            resType = EGameResource(random(0, 5));
                        }
                        resQty = static_cast<unsigned char>(random(1, 5));
                        info->setLeanTo(numLeanTo++, resQty, resType);
                    }
                    break;

                case LIBRARY:
                    tempCell->m_extraInfo = numLibrary++;
                    break;

                case LIGHTHOUSE:
                    id = getMineId(x, y, z);
                    tempCell->m_extraInfo = id;
                    break;

                case LITH_ONEWAY_EXIT:
                    {
                        std::vector<type_point>* pool =
                            &m_lithExitPools[tempCell->m_objectIndex];
                        tempCell->m_extraInfo = pool->size();
                        type_point point(x, y, z);
                        pool->push_back(point);
                    }
                    break;

                case UNDERGROUND_GATE:
                    {
                        tempCell->m_extraInfo = m_undergroundGateExits.size();
                        type_point point(x, y, z);
                        m_undergroundGateExits.push_back(point);
                        m_undergroundGatePairs.push_back(-1);
                    }
                    break;

                case LITH_TWOWAY:
                    {
                        std::vector<type_point>* pool =
                            &m_lithPools[tempCell->m_objectIndex];
                        tempCell->m_extraInfo = pool->size();
                        type_point point(x, y, z);
                        pool->push_back(point);
                    }
                    break;

                case MAGIC_SCHOOL:
                    tempCell->m_extraInfo = numMagicSchool++;
                    break;

                case MAGIC_SPRING:
                    // DC 5355/5356 separates the setter from its increment.
                    if (x > 0) {
                        NewmapCell* left = m_worldMap.cell(x - 1, y, z);
                        if (left->m_type == MAGIC_SPRING && left->m_isTrigger) {
                            tempCell->m_extraInfo = left->m_extraInfo;
                            break;
                        }
                    }
                    static_cast<ExtraInfoUnion*>(
                        static_cast<void*>(&tempCell->m_extraInfo))
                        ->setMagicSpring(numMagicSpring, 1);
                    ++numMagicSpring;
                    break;

                case MERC_CAMP:
                    tempCell->m_extraInfo = numMercCamp++;
                    break;

                case MINE:
                    id = getMineId(x, y, z);
                    if (m_mines[id].m_isAbandoned) {
                        m_mines[id].m_guards.m_armies[0] = 0x46;
                        m_mines[id].m_guards.m_numTroops[0] = random(100, 200);
                    }
                    claimMine(id, m_mines[id].m_playerOwner,
                              const_initialization_action);
                    break;

                case MONSTER:
                    if ((tempCell->m_extraInfo & 0xfff) == 0) {
                        tempCell->m_monsterInfo.m_qty =
                            getRandomNumTroops(tempCell->m_objectIndex);
                    }
                    break;

                case MYSTICAL_GARDEN:
                    // DC 5400 keeps Random and SetGarden in one statement;
                    // 5401 increments the pool counter after the setter.
                    static_cast<ExtraInfoUnion*>(
                        static_cast<void*>(&tempCell->m_extraInfo))
                        ->setGarden(numMysticalGarden,
                                    random(0, 1) ? GOLD : GEMS);
                    ++numMysticalGarden;
                    break;

                case OBELISK:
                    if (g_game->m_numObelisks < 48)
                        tempCell->m_extraInfo = g_game->m_numObelisks++;
                    break;

                case POWER_SCHOOL:
                    tempCell->m_extraInfo = numPowerSchool++;
                    break;

                case PYRAMID:
                    randomizePyramid(tempCell);
                    break;

                case REFUGEE_CAMP:
                    {
                        TCreatureType creature = getRandomMonster(0, 6);
                        tempCell->m_objectIndex = creature;
                        tempCell->m_extraInfo =
                            g_creatureTypeTraits[creature].m_growthRate;
                    }
                    break;

                case RESOURCE:
                    if ((tempCell->m_extraInfo & 0x7ffff) == 0) {
                        if (tempCell->m_objectIndex == WOOD
                            || tempCell->m_objectIndex == ORE
                            || tempCell->m_objectIndex == GOLD)
                            id = random(5, 10);
                        else
                            id = random(3, 6);
                        tempCell->m_extraInfo =
                            (tempCell->m_extraInfo & 0xfff80000)
                            | (id & 0x7ffff);
                    }
                    break;

                case SCHOLAR:
                    randomizeScholar(tempCell);
                    break;

                case SEA_CHEST:
                    randomizeSeaChest(tempCell);
                    break;

                case SHIPWRECK_SURVIVOR:
                    id = random(0, 99);
                    if (id < 55)
                        tempCell->m_extraInfo = getRandomArtifactId(2);
                    else if (id < 75)
                        tempCell->m_extraInfo = getRandomArtifactId(4);
                    else if (id < 95)
                        tempCell->m_extraInfo = getRandomArtifactId(8);
                    else
                        tempCell->m_extraInfo = getRandomArtifactId(16);
                    break;

                case SHIPYARD:
                    {
                        signed char oldOwner =
                            static_cast<signed char>(tempCell->m_extraInfo);
                        if (oldOwner != -1) {
                            tempCell->m_extraInfo =
                                (tempCell->m_extraInfo & 0xffffff00) | 0xff;
                            type_point location(x, y, z);
                            claimShipyard(location, oldOwner);
                        }
                    }
                    break;

                case SHRINE1:
                    randomizeShrine(tempCell, g_shrineLevelOne);
                    break;

                case SHRINE2:
                    randomizeShrine(tempCell, g_shrineLevelTwo);
                    break;

                case SHRINE3:
                    randomizeShrine(tempCell, g_shrineLevelThree);
                    break;

                case TRAINING_GROUNDS:
                    tempCell->m_extraInfo = numTrainingGround++;
                    break;

                case TREASURE_CHEST:
                    randomizeTreasure(tempCell);
                    break;

                case TREE_OF_KNOWLEDGE:
                    randomizeWiseTree(static_cast<short>(numTreeOfKnowledge++), tempCell);
                    break;

                case UNIVERSITY:
                    randomizeUniversity(tempCell);
                    break;

                case WAGON:
                    randomizeWagon(tempCell);
                    break;

                case WAR_SCHOOL:
                    tempCell->m_extraInfo = numWarSchool++;
                    break;

                case WARRIOR_TOMB:
                    randomizeTomb(tempCell);
                    break;

                case WATER_WHEEL:
                    {
                        ExtraInfoUnion* info = static_cast<ExtraInfoUnion*>(
                            static_cast<void*>(&tempCell->m_extraInfo));
                        // DC 5558..5559; retail combines both operations
                        // into (extraInfo & 0xffffe001) | 1.
                        info->setWheelGold(500);
                        info->clearVisitedBits();
                    }
                    break;

                case WHIRLPOOL:
                    thisObj = &tempCell->m_objects[0];
                    if ((thisObj->m_offsets & 0xf)
                            == g_whirlpoolTriggerXOffset
                        && (thisObj->m_offsets & 0xf0)
                            == g_whirlpoolTriggerYOffset) {
                        tempCell->m_extraInfo = numWhirlpool++;
                    }
                    else {
                        int xOffset = static_cast<signed char>(
                            thisObj->m_offsets << 4) >> 4;
                        int yOffset =
                            static_cast<signed char>(thisObj->m_offsets) >> 4;
                        tempCell->m_extraInfo = m_worldMap.cell(
                            x + xOffset - 2, y + yOffset - 1, z)->m_extraInfo;
                    }
                    // DC game.cpp:5575 constructs the point and calls
                    // vector::push_back on the same source line.
                    // Alone this unpin measured 73.35% for either point
                    // lifetime. Restoring ClaimGarrison's canonical call
                    // also restores this retained insert, reaching 87.82%.
                    m_whirlpools.push_back(type_point(x, y, z));
                    break;

                case WINDMILL:
                    {
                        ExtraInfoUnion* info = static_cast<ExtraInfoUnion*>(
                            static_cast<void*>(&tempCell->m_extraInfo));
                        resQty = static_cast<unsigned char>(random(3, 6));
                        {
                            resType = EGameResource(random(1, 5));
                        }
                        // DC 5582..5583; retail's 0xfffe001f mask also
                        // clears the visited-player lane, not just amount.
                        info->setWindmill(resType, resQty);
                        info->clearVisitedBits();
                    }
                    break;

                case WITCH_HUT:
                    randomizeWitchHut(tempCell);
                    break;
                }
            }
        }
    }
}

// The four packed-field COMDATs retained after RandomizeEvents at
// 0x4c2330..0x4c23dc are annotated on their canonical mapcell.h bodies.

// CodeView dc 0xbd58c: CV_fldattr_t.compgenx marks this destructor
// as implicit. Its retained retail body performs only base/member teardown.
VA_COMPGEN(0x004c2420, 0x26, IMPLICIT_DTOR, type_creature_bank)

// Complete reads a scenario from the already-open map stream. The PC path
// keeps the three retail map generations distinct while normalizing their
// artifact/spell masks into the Complete-width live rows, then reads the
// rumour and hero customization records before handing the remainder to the
// map-cell owner.

// MAX 78.2855. On 2026-09-07 the inherited body measured 74.47679.
// Restoring all clear calls alone gives 63.79606, separate reads with the
// failure-return pin removed give 68.89874, and both give 64.73558. Keep
// the recovered boundaries through the dip. Removing the normal loop-end
// destructor pin too gives 63.86639 (73.26442 in the inherited body), but
// replaces its destructor call at +0x61b with retail's required _Tidy call
// at +0x61d. Both failure exits also retain _Tidy. Remove both destructor
// pins on that call-sequence evidence; do not keep them for the higher score.
// The passive source-boundary trace has caller cb 1752 / budget 3504.
// Bitset<144>::_Tidy costs 72 against 71/70 and stays called, while the
// 129/70-bit instances get 73/87 and expand. The 28-bit read's _Xran now
// stays called (cost 65, budget 50), correcting the older frontier diagnosis.
// Remaining frontier: bitset/container inner-helper expansion decisions
// still differ. The version-21 artifact arm and merge
// loop also have different placement/register allocation.
// DC's filename-based loader predates these packed artifact/spell/skill planes.
// Retail decodes the legacy mask into a temporary, copies five dwords, then
// traverses the copy through mutable bitset iterators. The value-returning
// readPackedBits shared with ScenarioStruct::read explains that copy and the
// spell/skill temporaries without redundant caller snapshots: 79.8650% versus
// the unpinned decoder/transform model's 71.1435% (bare pin removal: 57.7665%).
// Explicit result(0) gives 80.4993% but lowers the campaign reader; direct wide
// assignment gives 67.4290%. The legacy source dereference still stays called
// where retail expands it. No inline controls are needed for these readers.
VA(0x004c2450, 0x88E)  // sole NewMap caller + full stream/callee sequence
bool game::loadMap(TAbstractFile* mapFile)
{
    if (m_mapHeader.read(mapFile, m_campaign.m_currentMap) < 0)
        return false;

    applyMapHeaderAvailability();
    setMapSize(m_mapHeader.m_size, m_mapHeader.m_size);

    if (m_gameVersion < 1)
        memset(m_heroAvailability + 128, hero::HERO_AVAILABILITY_TAVERN_POOL,
               HERO_COUNT - 128);

    int artifact;
    for (artifact = 0; artifact < 144; ++artifact)
        m_artifactDisabled[artifact] = g_artifactTraits[artifact].m_disabled;

    if (m_gameVersion < 2) {
        artifact = (((m_gameVersion >= 1) - 1) & -2) + 129;
        memset(m_artifactDisabled + artifact, 1,
               sizeof(m_artifactDisabled) - artifact);
    }

    if (m_mapHeader.m_version != MAP_FORMAT_RESTORATION_OF_ERATHIA) {
        std::bitset<144> disabledArtifacts(0);
        if (m_mapHeader.m_version != MAP_FORMAT_ARMAGEDDONS_BLADE) {
            std::bitset<144> serializedArtifacts = readPackedBits<144>(mapFile);
            disabledArtifacts = serializedArtifacts;
        } else {
            for (artifact = 0; artifact < 144; ++artifact) {
                bool isComboArtifact = g_artifactTraits[artifact].m_comboType != -1;
                disabledArtifacts[artifact] = isComboArtifact;
            }

            std::bitset<129> serializedArtifacts = readPackedBits<129>(mapFile);
            std::copy(bitset_iterator<129>(serializedArtifacts, 0),
                      bitset_iterator<129>(serializedArtifacts, 129),
                      bitset_iterator<144>(disabledArtifacts, 0));
        }

        std::transform(m_artifactDisabled, m_artifactDisabled + 144,
                       bitset_iterator<144>(disabledArtifacts, 0),
                       m_artifactDisabled, std::logical_or<bool>());
    }

    if (m_mapHeader.m_version != MAP_FORMAT_RESTORATION_OF_ERATHIA
        && m_mapHeader.m_version != MAP_FORMAT_ARMAGEDDONS_BLADE) {
        const std::bitset<70> serializedSpells = readPackedBits<70>(mapFile);

        for (unsigned int spell = 0; spell < hero::NUM_SPELLS; ++spell) {
            if (serializedSpells[spell]) {
                for (artifact = 0; artifact < 144; ++artifact) {
                    if (g_artifactTraits[artifact].m_givesSpells) {
                        m_artifactDisabled[artifact] =
                            m_artifactDisabled[artifact]
                            || markArtifactSpells(artifact).test(spell);
                    }
                }
            }
            m_spellDisabledInfo[spell] =
                serializedSpells[spell]
                || (g_spellTraits[spell].m_flags & 0x2000) != 0;
        }

        const std::bitset<28> serializedSkills = readPackedBits<28>(mapFile);
        for (int skill = 0; skill < sizeof(m_ssDisabled); ++skill)
            m_ssDisabled[skill] = serializedSkills[skill];
    } else {
        for (int spell = 0; spell < hero::NUM_SPELLS; ++spell) {
            m_spellDisabledInfo[spell] =
                (g_spellTraits[spell].m_flags & 0x2000) != 0;
        }
        memset(m_ssDisabled, 0, sizeof(m_ssDisabled));
    }

    std::copy(m_spellDisabledInfo,
              m_spellDisabledInfo + sizeof(m_spellDisabledInfo), m_spellAllocInfo);

    int rumourListSize;
    if (mapFile->read(&rumourListSize, sizeof(rumourListSize))
        < sizeof(rumourListSize)) {
        return false;
    }
    m_rumours.resize(rumourListSize);
    TRumour* rit;
    for (rit = m_rumours.begin(); rit != m_rumours.end(); ++rit) {
        std::string throwAway;
        int hr = NewSMapHeader::readString(mapFile, throwAway);
        if (hr < 0)
            return false;
        hr = NewSMapHeader::readString(mapFile, rit->m_text);
        if (hr < 0)
            return false;
        rit->m_unavailable = 0;
    }

    if (m_mapHeader.m_version != MAP_FORMAT_RESTORATION_OF_ERATHIA
        && m_mapHeader.m_version != MAP_FORMAT_ARMAGEDDONS_BLADE) {
        for (std::map<int, type_map_hero_info>::iterator it =
                 m_mapHeader.m_heroPlayerSetups.begin();
             it != m_mapHeader.m_heroPlayerSetups.end(); ++it) {
            HeroExtra* setupRecord = &m_heroSetup[it->first];
            type_map_hero_info* headerRecord = &it->second;
            if (headerRecord->m_portrait != -1) {
                setupRecord->m_customPortraitNumber = 1;
                setupRecord->m_portraitNumber = headerRecord->m_portrait;
            }
            if (!headerRecord->m_name.empty()) {
                setupRecord->m_hasCustomName = 1;
                strncpy(setupRecord->m_nameBuffer, headerRecord->m_name.c_str(),
                        sizeof(setupRecord->m_nameBuffer));
                setupRecord->m_nameBuffer[sizeof(setupRecord->m_nameBuffer) - 1] = 0;
            }
        }
        readMapHeroSetups(mapFile, m_mapHeader.m_version);
    }

    for (long i = 0; i < 8; ++i) {
        m_lithPools[i].clear();
        m_lithExitPools[i].clear();
    }
    m_whirlpools.clear();
    m_undergroundGateExits.clear();
    m_undergroundGatePairs.clear();
    m_monsterIdentifiers.clear();

    return m_worldMap.read(mapFile, m_mapHeader.m_size, m_mapHeader.m_hasTwoLayers,
                         m_mapHeader.m_version) >= 0;
}

// Complete appends one optional customization record for each of the 156
// fixed hero identities. Missing records leave SetupOrigData's defaults
// intact; present fields overlay only the editor-selected portions. The
// second argument is part of the proved retail arity (`ret 8`) but this body
// never reads it.

// The artifact records are assigned as complete two-dword values. Besides
// expressing the map format directly, that is the source shape which gives
// retail's `movsx / store / or -1 / store` loop. The custom name likewise
// assigns the complete returned string. No Dreamcast counterpart is known
// for this Complete-only reader. Whole-string operator= currently gives
// 44.0213% versus 52.0030% with explicit assign; a named return-value temporary
// only reaches 44.7652%. String and bitset helper expansion remains unresolved.
VA(0x004c2ce0, 0x3A8)  // sole caller LoadMap + HeroExtra field-offset walk
void game::readMapHeroSetups(TAbstractFile* mapFile, int mapVersion)
{
    for (int heroId = 0; heroId < HERO_COUNT; ++heroId) {
        HeroExtra* heroRecord = &m_heroSetup[heroId];

        if (!readValue<char>(mapFile))
            continue;

        if (readValue<char>(mapFile)) {
            heroRecord->m_customExperience = 1;
            heroRecord->m_experience = readValue<int>(mapFile);
        }

        if (readValue<char>(mapFile)) {
            heroRecord->m_customSecondarySkills = 1;
            heroRecord->m_numSecondarySkills = readValue<int>(mapFile);
            for (int skill = 0;
                 skill < heroRecord->m_numSecondarySkills; ++skill) {
                heroRecord->m_secondarySkill[skill] = readValue<char>(mapFile);
                heroRecord->m_secondarySkillLevel[skill] =
                    readValue<char>(mapFile);
            }
        }

        if (readValue<char>(mapFile)) {
            heroRecord->m_customArtifacts = 1;
            for (int equipped = 0; equipped < 19; ++equipped) {
                heroRecord->m_artifacts[equipped] =
                    // Complete map input stores a signed 16-bit artifact ordinal; the in-memory record retains DC's TArtifact constructor.
                    type_artifact(static_cast<TArtifact>(readValue<short>(mapFile)) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */);
            }

            heroRecord->m_numInBackpack =
                static_cast<unsigned char>(readValue<short>(mapFile));
            for (int carried = 0;
                 carried < heroRecord->m_numInBackpack; ++carried) {
                heroRecord->m_backpack[carried] =
                    // Complete map input stores a signed 16-bit artifact ordinal; the in-memory record retains DC's TArtifact constructor.
                    type_artifact(static_cast<TArtifact>(readValue<short>(mapFile)) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */);
            }

            heroRecord->m_artifacts[hero::EQUIPPED_SLOT_WAR_MACHINE_4] =
                type_artifact(ARTIFACT_CATAPULT);
        }

        if (readValue<char>(mapFile)) {
            heroRecord->m_customName = 1;
            heroRecord->m_name = readLengthPrefixedString(mapFile);
        }

        int sex = readValue<signed char>(mapFile);
        if (sex != -1)
            heroRecord->m_sex = sex;

        if (readValue<char>(mapFile)) {
            heroRecord->m_customSpells = 1;
            unsigned char spellMask[9];
            mapFile->read(spellMask, sizeof(spellMask));
            for (int spell = 0; spell < hero::NUM_SPELLS; ++spell) {
                heroRecord->m_spells.set(
                    spell,
                    (spellMask[spell / 8] & (1 << (spell % 8))) != 0);
            }
        }

        if (readValue<char>(mapFile)) {
            heroRecord->m_customPrimarySkills = 1;
            for (int skill = 0; skill < 4; ++skill) {
                heroRecord->m_primarySkills[skill] = readValue<char>(mapFile);
            }
        }
    }
}

VA(0x004c3200, 0x398)
int NewSMapHeader::readVictoryCondition(char type, TAbstractFile* infile)
{
    char charBuffer;

    infile->read(&charBuffer, sizeof(charBuffer));
    m_victoryCondition.m_allowNormalVictory = charBuffer != 0;
    infile->read(&charBuffer, sizeof(charBuffer));
    m_victoryCondition.m_appliesToComputer = charBuffer != 0;

    switch (type) {
    case VICTORY_CONDITION_ARTIFACT: {
        if (m_version == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
            int intBuffer;
            infile->read(&intBuffer, sizeof(char));
            m_victoryCondition.m_artifactNum =
                static_cast<TArtifact>(intBuffer & 0xff); /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */
        } else {
            short shortBuffer;
            infile->read(&shortBuffer, sizeof(shortBuffer));
            m_victoryCondition.m_artifactNum =
                static_cast<TArtifact>(shortBuffer); /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */
        }
        break;
    }

    case VICTORY_CONDITION_TOTAL_CREATURES: {
        if (m_version == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
            int intBuffer;
            infile->read(&intBuffer, sizeof(char));
            {
                m_victoryCondition.m_creatureType = TCreatureType(intBuffer & 0xff);
            }
        } else {
            short shortBuffer;
            infile->read(&shortBuffer, sizeof(shortBuffer));
            {
                m_victoryCondition.m_creatureType = TCreatureType(shortBuffer);
            }
        }
        {
            int intBuffer;
            infile->read(&intBuffer, sizeof(intBuffer));
            m_victoryCondition.m_numCreatures = intBuffer;
        }
        break;
    }

    case VICTORY_CONDITION_TOTAL_RESOURCES: {
        {
            char resource;
            infile->read(&resource, sizeof(resource));
            m_victoryCondition.m_resourceType = resource;
        }
        {
            int intBuffer;
            infile->read(&intBuffer, sizeof(intBuffer));
            m_victoryCondition.m_resourceAmount = intBuffer;
        }
        break;
    }

    case VICTORY_CONDITION_UPGRADE_TOWN: {
        {
            int intBuffer;
            infile->read(&intBuffer, sizeof(char));
            m_victoryCondition.m_townX = intBuffer & 0xff;
            infile->read(&intBuffer, sizeof(char));
            m_victoryCondition.m_townY = intBuffer & 0xff;
            infile->read(&intBuffer, sizeof(char));
            m_victoryCondition.m_townZ = intBuffer & 0xff;
        }
        {
            char level;
            infile->read(&level, sizeof(level));
            m_victoryCondition.m_hallLevel = level;
            infile->read(&level, sizeof(level));
            m_victoryCondition.m_castleLevel = level;
        }
        break;
    }

    case VICTORY_CONDITION_BUILD_GRAIL: {
        int intBuffer;
        infile->read(&intBuffer, sizeof(char));
        m_victoryCondition.m_townX = intBuffer & 0xff;
        if (m_victoryCondition.m_townX == g_savedMapCoordinateNone)
            m_victoryCondition.m_townX = -1;
        infile->read(&intBuffer, sizeof(char));
        m_victoryCondition.m_townY = intBuffer & 0xff;
        if (m_victoryCondition.m_townY == g_savedMapCoordinateNone)
            m_victoryCondition.m_townY = -1;
        infile->read(&intBuffer, sizeof(char));
        m_victoryCondition.m_townZ = intBuffer & 0xff;
        if (m_victoryCondition.m_townZ == g_savedMapCoordinateNone)
            m_victoryCondition.m_townZ = -1;
        break;
    }

    case VICTORY_CONDITION_DEFEAT_HERO: {
        int intBuffer;
        infile->read(&intBuffer, sizeof(char));
        m_victoryCondition.m_heroX = intBuffer & 0xff;
        infile->read(&intBuffer, sizeof(char));
        m_victoryCondition.m_heroY = intBuffer & 0xff;
        infile->read(&intBuffer, sizeof(char));
        m_victoryCondition.m_heroZ = intBuffer & 0xff;
        break;
    }

    case VICTORY_CONDITION_CAPTURE_TOWN: {
        int intBuffer;
        infile->read(&intBuffer, sizeof(char));
        m_victoryCondition.m_townX = intBuffer & 0xff;
        infile->read(&intBuffer, sizeof(char));
        m_victoryCondition.m_townY = intBuffer & 0xff;
        infile->read(&intBuffer, sizeof(char));
        m_victoryCondition.m_townZ = intBuffer & 0xff;
        break;
    }

    case VICTORY_CONDITION_DEFEAT_MONSTER: {
        int intBuffer;
        infile->read(&intBuffer, sizeof(char));
        m_victoryCondition.m_monsterX = intBuffer & 0xff;
        infile->read(&intBuffer, sizeof(char));
        m_victoryCondition.m_monsterY = intBuffer & 0xff;
        infile->read(&intBuffer, sizeof(char));
        m_victoryCondition.m_monsterZ = intBuffer & 0xff;
        break;
    }

    case VICTORY_CONDITION_FLAG_ALL_GENERATORS:
    case VICTORY_CONDITION_FLAG_ALL_MINES:
    case VICTORY_CONDITION_DEFEAT_ALL_MONSTERS:
        break;

    case VICTORY_CONDITION_SURVIVE_TIME: {
        int intBuffer;
        infile->read(&intBuffer, sizeof(intBuffer));
        m_victoryCondition.m_numDays = intBuffer;
        break;
    }

    case VICTORY_CONDITION_TRANSPORT_ARTIFACT: {
        int intBuffer;
        infile->read(&intBuffer, sizeof(char));
        m_victoryCondition.m_artifactNum =
            static_cast<TArtifact>(intBuffer & 0xff); /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */
        infile->read(&intBuffer, sizeof(char));
        m_victoryCondition.m_townX = intBuffer & 0xff;
        infile->read(&intBuffer, sizeof(char));
        m_victoryCondition.m_townY = intBuffer & 0xff;
        infile->read(&intBuffer, sizeof(char));
        m_victoryCondition.m_townZ = intBuffer & 0xff;
        break;
    }
    }

    g_game->validateVictoryLossConditions(0);
    return 0;
}

VA(0x004c35a0, 0x2E8)
int NewSMapHeader::saveVictoryCondition(char type, TAbstractFile* outfile)
{
    int intBuffer;
    int count;
    char charBuffer;

    charBuffer = m_victoryCondition.m_allowNormalVictory;
    count = outfile->write(&charBuffer, sizeof(charBuffer));
    if (count < sizeof(charBuffer))
        return -1;

    charBuffer = m_victoryCondition.m_appliesToComputer;
    count = outfile->write(&charBuffer, sizeof(charBuffer));
    if (count < sizeof(charBuffer))
        return -1;

    switch (type) {
    case VICTORY_CONDITION_ARTIFACT: {
        char artifact = m_victoryCondition.m_artifactNum;
        outfile->write(&artifact, sizeof(artifact));
        return 0;
    }

    case VICTORY_CONDITION_TOTAL_CREATURES: {
        char creature = m_victoryCondition.m_creatureType;
        outfile->write(&creature, sizeof(creature));
        intBuffer = m_victoryCondition.m_numCreatures;
        count = outfile->write(&intBuffer, sizeof(intBuffer));
        if (count < sizeof(intBuffer))
            return -1;
        break;
    }

    case VICTORY_CONDITION_TOTAL_RESOURCES:
        charBuffer = m_victoryCondition.m_resourceType;
        count = outfile->write(&charBuffer, sizeof(charBuffer));
        if (count < sizeof(charBuffer))
            return -1;
        intBuffer = m_victoryCondition.m_resourceAmount;
        count = outfile->write(&intBuffer, sizeof(intBuffer));
        if (count < sizeof(intBuffer))
            return -1;
        break;

    case VICTORY_CONDITION_UPGRADE_TOWN: {
        char townValue = m_victoryCondition.m_townX;
        outfile->write(&townValue, sizeof(townValue));
        townValue = m_victoryCondition.m_townY;
        outfile->write(&townValue, sizeof(townValue));
        townValue = m_victoryCondition.m_townZ;
        outfile->write(&townValue, sizeof(townValue));
        charBuffer = m_victoryCondition.m_hallLevel;
        count = outfile->write(&charBuffer, sizeof(charBuffer));
        if (count < sizeof(charBuffer))
            return -1;
        charBuffer = m_victoryCondition.m_castleLevel;
        count = outfile->write(&charBuffer, sizeof(charBuffer));
        if (count < sizeof(charBuffer))
            return -1;
        break;
    }

    case VICTORY_CONDITION_BUILD_GRAIL: {
        char grailTown = m_victoryCondition.m_townX;
        outfile->write(&grailTown, sizeof(grailTown));
        grailTown = m_victoryCondition.m_townY;
        outfile->write(&grailTown, sizeof(grailTown));
        grailTown = m_victoryCondition.m_townZ;
        outfile->write(&grailTown, sizeof(grailTown));
        return 0;
    }

    case VICTORY_CONDITION_DEFEAT_HERO: {
        char heroId = m_victoryCondition.m_heroId;
        outfile->write(&heroId, sizeof(heroId));
        return 0;
    }

    case VICTORY_CONDITION_CAPTURE_TOWN: {
        char capturedTown = m_victoryCondition.m_townX;
        outfile->write(&capturedTown, sizeof(capturedTown));
        capturedTown = m_victoryCondition.m_townY;
        outfile->write(&capturedTown, sizeof(capturedTown));
        capturedTown = m_victoryCondition.m_townZ;
        outfile->write(&capturedTown, sizeof(capturedTown));
        return 0;
    }

    case VICTORY_CONDITION_DEFEAT_MONSTER: {
        char monster = m_victoryCondition.m_monsterX;
        outfile->write(&monster, sizeof(monster));
        monster = m_victoryCondition.m_monsterY;
        outfile->write(&monster, sizeof(monster));
        monster = m_victoryCondition.m_monsterZ;
        outfile->write(&monster, sizeof(monster));
        return 0;
    }

    case VICTORY_CONDITION_SURVIVE_TIME: {
        int days = m_victoryCondition.m_numDays;
        outfile->write(&days, sizeof(days));
        return 0;
    }

    case VICTORY_CONDITION_TRANSPORT_ARTIFACT: {
        char transport = m_victoryCondition.m_artifactNum;
        outfile->write(&transport, sizeof(transport));
        transport = m_victoryCondition.m_townX;
        outfile->write(&transport, sizeof(transport));
        transport = m_victoryCondition.m_townY;
        outfile->write(&transport, sizeof(transport));
        transport = m_victoryCondition.m_townZ;
        outfile->write(&transport, sizeof(transport));
        return 0;
    }
    }

    return 0;
}

// Saved victory payloads widen their byte-sized ids and coordinates into the
// live retail record. The two common flags are normalized to bool; only the
// creature/resource amounts, resource id and final upgraded-town byte retain
// short-read checks. Saves before the Complete roster remap two campaign ids.
// Residual (99.8359%, 2026-08-30): all 34 blocks, control flow and operations
// agree. Raw NB11 proves the procedure-scope int_buffer/count/char_buffer
// order, and both leading reads retain Dreamcast's count assignments even
// though Complete removed their short-read guards. The five differing blocks
// are only VC6 stack coloring. Shared DC buffers, shared post-flag case temps
// and function-scope Complete temps all displaced otherwise-exact homes.
VA(0x004c3890, 0x3E4)  // sole Load caller + retail body; dc 0xaeb64
int NewSMapHeader::loadVictoryCondition(char type, TAbstractFile* infile,
                                        int saveVersion)
{
    int intBuffer;
    int count;
    char charBuffer;

    count = infile->read(&charBuffer, sizeof(charBuffer));
    m_victoryCondition.m_allowNormalVictory = charBuffer != 0;
    count = infile->read(&charBuffer, sizeof(charBuffer));
    m_victoryCondition.m_appliesToComputer = charBuffer != 0;

    switch (type) {
    case VICTORY_CONDITION_ARTIFACT: {
        int artifact;
        infile->read(&artifact, sizeof(char));
        m_victoryCondition.m_artifactNum =
            static_cast<TArtifact>(artifact & 0xff); /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */
        return 0;
    }

    case VICTORY_CONDITION_TOTAL_CREATURES: {
        int creature;
        infile->read(&creature, sizeof(char));
        {
            m_victoryCondition.m_creatureType = TCreatureType(creature & 0xff);
        }
        count = infile->read(&intBuffer, sizeof(intBuffer));
        if (count < sizeof(intBuffer))
            return -1;
        m_victoryCondition.m_numCreatures = intBuffer;
        return 0;
    }

    case VICTORY_CONDITION_TOTAL_RESOURCES: {
        char resourceType;
        count = infile->read(&resourceType, sizeof(resourceType));
        if (count < sizeof(resourceType))
            return -1;
        m_victoryCondition.m_resourceType = resourceType;
        count = infile->read(&intBuffer, sizeof(intBuffer));
        if (count < sizeof(intBuffer))
            return -1;
        m_victoryCondition.m_resourceAmount = intBuffer;
        return 0;
    }

    case VICTORY_CONDITION_UPGRADE_TOWN: {
        int townValue;
        char hallLevel;
        char castleLevel;
        infile->read(&townValue, sizeof(char));
        m_victoryCondition.m_townX = townValue & 0xff;
        infile->read(&townValue, sizeof(char));
        m_victoryCondition.m_townY = townValue & 0xff;
        infile->read(&townValue, sizeof(char));
        m_victoryCondition.m_townZ = townValue & 0xff;
        infile->read(&hallLevel, sizeof(hallLevel));
        m_victoryCondition.m_hallLevel = hallLevel;
        count = infile->read(&castleLevel, sizeof(castleLevel));
        if (count < sizeof(castleLevel))
            return -1;
        m_victoryCondition.m_castleLevel = castleLevel;
        return 0;
    }

    case VICTORY_CONDITION_BUILD_GRAIL: {
        int grailTown;
        infile->read(&grailTown, sizeof(char));
        m_victoryCondition.m_townX = grailTown & 0xff;
        if (m_victoryCondition.m_townX == g_savedMapCoordinateNone)
            m_victoryCondition.m_townX = -1;
        infile->read(&grailTown, sizeof(char));
        m_victoryCondition.m_townY = grailTown & 0xff;
        if (m_victoryCondition.m_townY == g_savedMapCoordinateNone)
            m_victoryCondition.m_townY = -1;
        infile->read(&grailTown, sizeof(char));
        m_victoryCondition.m_townZ = grailTown & 0xff;
        if (m_victoryCondition.m_townZ == g_savedMapCoordinateNone)
            m_victoryCondition.m_townZ = -1;
        return 0;
    }

    case VICTORY_CONDITION_DEFEAT_HERO: {
        int heroId;
        infile->read(&heroId, sizeof(char));
        heroId &= 0xff;
        if (heroId == g_savedHeroNone) {
            heroId = -1;
        } else if (saveVersion < g_saveVersionCompleteHeroRoster) {
            if (heroId == g_savedHeroPre25First)
                heroId = g_heroPre25FirstRemap;
            else if (heroId == g_savedHeroPre25Second)
                heroId = g_heroPre25SecondRemap;
        }
        m_victoryCondition.m_heroId = heroId;
        return 0;
    }

    case VICTORY_CONDITION_CAPTURE_TOWN: {
        int capturedTown;
        infile->read(&capturedTown, sizeof(char));
        m_victoryCondition.m_townX = capturedTown & 0xff;
        infile->read(&capturedTown, sizeof(char));
        m_victoryCondition.m_townY = capturedTown & 0xff;
        infile->read(&capturedTown, sizeof(char));
        m_victoryCondition.m_townZ = capturedTown & 0xff;
        return 0;
    }

    case VICTORY_CONDITION_DEFEAT_MONSTER: {
        int monster;
        infile->read(&monster, sizeof(char));
        m_victoryCondition.m_monsterX = monster & 0xff;
        infile->read(&monster, sizeof(char));
        m_victoryCondition.m_monsterY = monster & 0xff;
        infile->read(&monster, sizeof(char));
        m_victoryCondition.m_monsterZ = monster & 0xff;
        return 0;
    }

    case VICTORY_CONDITION_SURVIVE_TIME: {
        int days;
        infile->read(&days, sizeof(days));
        m_victoryCondition.m_numDays = days;
        return 0;
    }

    case VICTORY_CONDITION_TRANSPORT_ARTIFACT: {
        int transport;
        infile->read(&transport, sizeof(char));
        m_victoryCondition.m_artifactNum =
            static_cast<TArtifact>(transport & 0xff); /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */
        infile->read(&transport, sizeof(char));
        m_victoryCondition.m_townX = transport & 0xff;
        infile->read(&transport, sizeof(char));
        m_victoryCondition.m_townY = transport & 0xff;
        infile->read(&transport, sizeof(char));
        m_victoryCondition.m_townZ = transport & 0xff;
        return 0;
    }
    }

    return 0;
}

VA(0x004c3c80, 0x10B)
int NewSMapHeader::readLossCondition(char type, TAbstractFile* infile)
{
    short shortValue;
    int value;
    switch (type) {
    case LOSS_CONDITION_LOSE_TOWN:
        infile->read(&value, sizeof(char));
        m_lossCondition.m_townX = value & 0xff;
        infile->read(&value, sizeof(char));
        m_lossCondition.m_townY = value & 0xff;
        infile->read(&value, sizeof(char));
        m_lossCondition.m_townZ = value & 0xff;
        return 0;

    case LOSS_CONDITION_LOSE_HERO:
        infile->read(&value, sizeof(char));
        m_lossCondition.m_heroX = value & 0xff;
        infile->read(&value, sizeof(char));
        m_lossCondition.m_heroY = value & 0xff;
        infile->read(&value, sizeof(char));
        m_lossCondition.m_heroZ = value & 0xff;
        return 0;

    case LOSS_CONDITION_TIME_LIMIT:
        if (infile->read(&shortValue, sizeof(shortValue))
            < sizeof(shortValue))
            return -1;
        m_lossCondition.m_numDays = shortValue;
        return 0;
    }
    return 0;
}

// Original: NewSMapHeader::saveLossCondition; game.cpp:6390, dc 0xaf2b4
// Complete save 0x4c4f10 expands this helper after the loss-type byte.
// It writes this header through TAbstractFile, uses the saved hero ID rather
// than the old three hero coordinates, and does not test these payload writes.
int NewSMapHeader::saveLossCondition(char type, TAbstractFile* outfile)
{
    char charBuffer;
    short shortBuffer;
    switch (type) {
    case LOSS_CONDITION_LOSE_TOWN:
        charBuffer = m_lossCondition.m_townX;
        outfile->write(&charBuffer, sizeof(charBuffer));
        charBuffer = m_lossCondition.m_townY;
        outfile->write(&charBuffer, sizeof(charBuffer));
        charBuffer = m_lossCondition.m_townZ;
        outfile->write(&charBuffer, sizeof(charBuffer));
        break;

    case LOSS_CONDITION_LOSE_HERO:
        shortBuffer = m_lossCondition.m_heroId;
        outfile->write(&shortBuffer, sizeof(shortBuffer));
        break;

    case LOSS_CONDITION_TIME_LIMIT:
        shortBuffer = m_lossCondition.m_numDays;
        outfile->write(&shortBuffer, sizeof(shortBuffer));
        break;
    }
    return 0;
}

// Complete adds saveVersion; every retained return pops three arguments.
VA(0x004c3d90, 0x15E)  // DC loadLossCondition + sole Load caller + ret 0xc, dc 0xaf488
int NewSMapHeader::loadLossCondition(char type, TAbstractFile* infile,
                                     int saveVersion)
{
    short timeLimit;
    switch (type) {
    case LOSS_CONDITION_LOSE_TOWN: {
        int value;
        infile->read(&value, sizeof(char));
        m_lossCondition.m_townX = value & 0xff;
        infile->read(&value, sizeof(char));
        m_lossCondition.m_townY = value & 0xff;
        infile->read(&value, sizeof(char));
        m_lossCondition.m_townZ = value & 0xff;
        return 0;
    }

    case LOSS_CONDITION_LOSE_HERO:
        if (saveVersion == g_saveVersionLossHeroCoordinates) {
            int value;
            infile->read(&value, sizeof(char));
            m_lossCondition.m_heroX = value & 0xff;
            infile->read(&value, sizeof(char));
            m_lossCondition.m_heroY = value & 0xff;
            infile->read(&value, sizeof(char));
            m_lossCondition.m_heroZ = value & 0xff;
            return 0;
        } else {
            short savedHeroId;
            infile->read(&savedHeroId, sizeof(savedHeroId));
            int heroId = savedHeroId;
            if (saveVersion < g_saveVersionCompleteHeroRoster) {
                if (heroId == g_savedHeroPre25First)
                    heroId = g_heroPre25FirstRemap;
                else if (heroId == g_savedHeroPre25Second)
                    heroId = g_heroPre25SecondRemap;
            }
            m_lossCondition.m_heroId = heroId;
            return 0;
        }

    case LOSS_CONDITION_TIME_LIMIT:
        if (infile->read(&timeLimit, sizeof(timeLimit)) < sizeof(timeLimit))
            return -1;
        m_lossCondition.m_numDays = timeLimit;
        return 0;
    }
    return 0;
}

// Complete's extracted player-slot reader.  Dreamcast's NewSMapHeader::Read
// carries the corresponding logic inline, while the retail caller passes one
// 0x44-byte slot as `this`, the stream, and the map-format version.  The two
// PC-only main-town fields occupy the eight-byte extension absent from the DC
// record.  Player heroes are resized from a dword count after the separate
// one-byte default-placeholder count; their ids use only 0xff as a sentinel.

VA(0x004c3ef0, 0x498)  // sole NewSMapHeader::Read caller + slot stride/layout
void CMapHeaderData::TPlayerSlotAttributes::readMapPlayerSlot(
    TAbstractFile* infile, int mapVersion)
{
    {
        signed char charBuffer;
        infile->read(&charBuffer, sizeof(charBuffer));
        m_canBeHuman = charBuffer != 0;
        infile->read(&charBuffer, sizeof(charBuffer));
        m_canBeComputer = charBuffer != 0;
        infile->read(&charBuffer, sizeof(charBuffer));
        m_aiStrategy = charBuffer;

        if (mapVersion == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
            infile->read(&charBuffer, sizeof(charBuffer));
            m_legalAlignments = static_cast<unsigned char>(charBuffer);
        } else {
            if (mapVersion != MAP_FORMAT_ARMAGEDDONS_BLADE)
                infile->read(&charBuffer, sizeof(charBuffer));
            unsigned short shortBuffer;
            infile->read(&shortBuffer, sizeof(shortBuffer));
            m_legalAlignments = shortBuffer;
        }
    }

    {
        signed char charBuffer;
        infile->read(&charBuffer, sizeof(charBuffer));
        m_hasRandomAlignment = charBuffer != 0;
        if (m_hasRandomAlignment)
            m_legalAlignments |= 0x100;
        if (!g_gameContextFeatures[*g_videoGameState].test(1))
            m_legalAlignments &= 0xfeff;

        infile->read(&charBuffer, sizeof(charBuffer));
        m_hasMainTown = charBuffer != 0;
        m_mainTownType = -1;
        if (!m_hasMainTown) {
            m_generateHero = 0;
        } else {
            if (mapVersion == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
                m_generateHero = 1;
            } else {
                infile->read(&charBuffer, sizeof(charBuffer));
                m_generateHero = charBuffer != 0;
                infile->read(&charBuffer, sizeof(charBuffer));
                m_mainTownType = charBuffer;
            }

            infile->read(&charBuffer, sizeof(charBuffer));
            m_castleLoc.m_x = static_cast<unsigned char>(charBuffer);
            infile->read(&charBuffer, sizeof(charBuffer));
            m_castleLoc.m_y = static_cast<unsigned char>(charBuffer);
            infile->read(&charBuffer, sizeof(charBuffer));
            m_castleLoc.m_z = static_cast<unsigned char>(charBuffer);
        }
    }

    infile->read(&m_hasRandomHero, sizeof(m_hasRandomHero));
    m_nonRandomHeroId = readHeroId(infile, mapVersion);
    m_defaultPlaceholders = 0;
    if (m_nonRandomHeroId != -1) {
        m_nonRandomHeroCustomPortrait =
            readHeroId(infile, mapVersion);
        // Keep the decoded name alive through the copy into the player slot.
        std::string name = readLengthPrefixedString(infile);
        strcpy(m_nonRandomHeroCustomName, name.c_str());
    } else {
        m_nonRandomHeroCustomPortrait = -1;
        m_nonRandomHeroCustomName[0] = 0;
    }

    m_heroes.clear();
    if (mapVersion == MAP_FORMAT_RESTORATION_OF_ERATHIA)
        return;

    {
        unsigned long byteValue;
        infile->read(&byteValue, sizeof(unsigned char));
        m_defaultPlaceholders = byteValue & 0xff;
    }

    int heroCount;
    infile->read(&heroCount, sizeof(heroCount));
    m_heroes.resize(heroCount);
    if (heroCount > 0) {
        int heroIndex = 0;
        do {
            unsigned long heroValue;
            infile->read(&heroValue, sizeof(unsigned char));
            int heroId = heroValue & 0xff;
            if (heroId == g_savedHeroNone)
                heroId = -1;
            m_heroes[heroIndex].m_heroId = heroId;
            m_heroes[heroIndex].m_name = readLengthPrefixedString(infile);
            ++heroIndex;
            --heroCount;
        } while (heroCount != 0);
    }
}

// Scenario-map headers precede the world payload with the compact map-format
// record written by Save. Complete extracts the player-slot reader above and
// extends the original hero mask with 28 heroes plus optional per-player hero
// setup records. Dreamcast CodeView proves the method/local identities; the
// retail stream reads, helper edges and version branches prove the PC schema.
// Current VC6 closure is 91.5614% (130 candidate blocks vs 127 retail): the
// first 70 blocks are exact. The remaining tail is dominated by Dinkumware
// nested-inliner boundaries around bitset::_Tidy and iterator copy/destruction;
// forcing those sites synthetically reached a higher calibration score but was
// rejected because it did not describe retail source. A header access shim was
// also rejected after it perturbed already-exact functions in this TU.
// 2026-09-06, polish lane 36: the mapcell reader family's `int count` local
// - which took readResourceData and readScholarData to EXACT - DOES NOT
// generalise here.  Landing all ten `infile->Read` results in one function-
// scope `count` and comparing that against the sizeof measures 91.5614 ->
// 74.2348.  The DC block does name `count`, so retail's source very likely
// has it; what this body cannot absorb is the change in the two placeholder
// blocks' own block-scoped `count`, which the shared local subsumes.
// Complete adds the campaign-map ordinal to the stream reader (ret 8).
// Complete's returned packed-bitset reader accounts for the legacy temporary
// copied before iterator traversal and the modern/player mask reads. Sharing
// readPackedBits at all three sites removes four pins: 91.5614 -> 92.9547%.
// Eight reproduced combinations establish the coupled choice; using only
// legacy+player reads gives 90.8737%, modern+player 89.5495%.
// Natural placeholder-vector clear() instead of erase(begin(), end()) raises
// this to 95.0513% and retains _Destroy. The preceding retail std::copy still
// expands here; the call sequence is not yet exact.
// Returned-mask lifetime controls: modern const-reference and player
// const-reference forms are flat at 95.0513%. Direct modern assignment alone
// gives 93.1657%; pairing it with a player const-reference returns 95.0513%.
// Keep the named values until stronger evidence distinguishes their lifetime.
// Complete setup records compare the saved hero ID as a byte before mapping
// 0xff to -1. A byte buffer with a widened int preserves that source operation
// and gives 95.2598%; a fused conditional gives 93.2122%. Separating the setup
// count from the reused x buffer gives 93.2408% (93.4827% with the byte ID).
// Current diagnostic routes to the over-expanded std::copy inside clear(),
// ahead of register/CFG differences. A separate DC int count for all ten
// guarded reads, preserving both nested record-count scopes, gives 79.0894%
// against 95.2598%. That does not refute the proven DC result local; its
// Complete source mapping remains a coupled recovery problem, not a pin lever.
// Restoring count at the two readString calls is byte-score flat (95.2598%);
// combining it with all ten stream reads remains 79.0894%. The string-result
// source facts are retained independently of that unresolved stream mapping.
VA(0x004c4390, 0x92E)  // DC Read + LoadMap/Get callers + stream order, dc 0xaf64c
int NewSMapHeader::read(TAbstractFile* infile, int campaignMap)
{
    char padding[g_mapHeaderPaddingSize];
    int count;

    m_mapName.erase();
    m_mapDescription.erase();

    if (infile->read(&m_version, sizeof(m_version)) < sizeof(m_version))
        return -1;

    if (m_version != MAP_FORMAT_SHADOW_OF_DEATH
        && m_version != MAP_FORMAT_RESTORATION_OF_ERATHIA
        && m_version != MAP_FORMAT_ARMAGEDDONS_BLADE) {
        m_mapName = g_generalText->getText(g_mapVersionErrorGeneralText);
        return -1;
    }

    char boolBuffer;
    if (infile->read(&boolBuffer, sizeof(boolBuffer)) < sizeof(boolBuffer))
        return -1;
    m_isPlayable = boolBuffer != 0;

    if (infile->read(&m_size, sizeof(m_size)) < sizeof(m_size))
        return -1;

    if (infile->read(&boolBuffer, sizeof(boolBuffer)) < sizeof(boolBuffer))
        return -1;
    m_hasTwoLayers = boolBuffer != 0;

    // DC game.cpp:6563/6569 stores each string-read result in count;
    // the next recorded source lines test that result separately.
    count = readString(infile, m_mapName);
    if (count < 0)
        return -1;
    count = readString(infile, m_mapDescription);
    if (count < 0)
        return -1;

    unsigned char ucharBuffer;
    if (infile->read(&ucharBuffer, sizeof(ucharBuffer))
        < sizeof(ucharBuffer))
        return -1;
    m_difficulty = ucharBuffer;

    if (m_version != MAP_FORMAT_RESTORATION_OF_ERATHIA) {
        char charBuffer;
        infile->read(&charBuffer, sizeof(charBuffer));
        m_maxHeroLevel = charBuffer;
    } else {
        m_maxHeroLevel = 0;
    }

    m_numPlayers = 0;
    m_minNumHumanPlayers = 0;
    m_maxNumHumanPlayers = 0;

    // This source local exists in the retail lifetime graph even though the
    // Complete-only tail uses a separate returned string for each hero name.
    std::string strTemp;

    TPlayerSlotAttributes* player = m_playerSlotAttributes;
    for (int i = 0; i < g_mapHeaderPlayerCount; ++i, ++player) {
        player->readMapPlayerSlot(infile, m_version);
        if (player->m_canBeHuman && !player->m_canBeComputer)
            ++m_minNumHumanPlayers;
        if (player->m_canBeHuman)
            ++m_maxNumHumanPlayers;
        if (player->m_canBeComputer || player->m_canBeHuman)
            ++m_numPlayers;
    }

    if (!m_minNumHumanPlayers)
        m_minNumHumanPlayers = 1;

    int x;
    if (infile->read(&x, sizeof(unsigned char)) < sizeof(unsigned char))
        return -1;
    m_victoryCondition.m_type = x;
    m_victoryCondition.m_gameWon = 0;
    m_victoryCondition.m_playerWinner = -1;
    if (static_cast<unsigned char>(x) != g_savedHeroNone)
        readVictoryCondition(x, infile);

    if (g_inCampaign) {
        switch (g_game->m_campaign.m_currentCampaign) {
        case g_campaignVictoryOverrideFirst:
            if (campaignMap == GAME_SCENARIO_2) {
                m_victoryCondition.m_type = VICTORY_CONDITION_DEFEAT_ALL_MONSTERS;
                m_victoryCondition.m_allowNormalVictory = 0;
            }
            break;

        case g_campaignVictoryOverrideSecond:
            if (campaignMap == GAME_SCENARIO_2) {
                m_victoryCondition.m_type = VICTORY_CONDITION_SURVIVE_TIME;
                m_victoryCondition.m_numDays = g_campaignVictoryOverrideDays;
            }
            break;

        case g_campaignVictoryOverrideThird:
            if (campaignMap == GAME_SCENARIO_0) {
                m_victoryCondition.m_type = VICTORY_CONDITION_DEFEAT_ALL_MONSTERS;
                m_victoryCondition.m_allowNormalVictory = 1;
            }
            break;
        }
    }

    if (infile->read(&x, sizeof(unsigned char)) < sizeof(unsigned char))
        return -1;
    m_lossCondition.m_type = x;
    m_lossCondition.m_gameLost = 0;
    m_lossCondition.m_playerLoser = -1;
    if (static_cast<unsigned char>(x) != g_savedHeroNone)
        readLossCondition(x, infile);

    if (infile->read(&x, sizeof(unsigned char)) < sizeof(unsigned char))
        return -1;
    m_numTeams = x;
    if (m_numTeams) {
        if (infile->read(m_teamInfo, sizeof(m_teamInfo)) < sizeof(m_teamInfo))
            return -1;
    } else {
        for (int i = 0; i < g_mapHeaderPlayerCount; ++i)
            m_teamInfo[i] = i;
    }

    if (m_version == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
        m_availableHeroes.reset();
        std::bitset<g_mapHeaderLegacyHeroCount> availableHeroesMask =
            readPackedBits<g_mapHeaderLegacyHeroCount>(infile);

        std::copy(
            bitset_iterator<g_mapHeaderLegacyHeroCount>(
                availableHeroesMask, 0),
            bitset_iterator<g_mapHeaderLegacyHeroCount>(
                availableHeroesMask, g_mapHeaderLegacyHeroCount),
            bitset_iterator<g_mapHeaderHeroCount>(m_availableHeroes, 0));

        if (!g_inCampaign) {
            for (int i = g_mapHeaderCompleteLegacyHeroFirst;
                 i <= g_mapHeaderCompleteLegacyHeroLast; ++i)
                m_availableHeroes[i] = true;
        }
    } else {
        std::bitset<g_mapHeaderHeroCount> availableHeroesMask =
            readPackedBits<g_mapHeaderHeroCount>(infile);
        m_availableHeroes = availableHeroesMask;
    }

    m_placeholders.clear();
    if (m_version != MAP_FORMAT_RESTORATION_OF_ERATHIA) {
        // Complete retail tests this dword for zero, not signed positivity.
        unsigned int count;
        infile->read(&count, sizeof(count));
        if (count > 0) {
            do {
                infile->read(&x, sizeof(unsigned char));
                x &= 0xff;
                m_placeholders.push_back(x);
            } while (--count != 0);
        }
    }

    m_heroPlayerSetups.clear();
    if (m_version != MAP_FORMAT_RESTORATION_OF_ERATHIA
        && m_version != MAP_FORMAT_ARMAGEDDONS_BLADE) {
        infile->read(&x, sizeof(unsigned char));
        x &= 0xff;
        if (static_cast<unsigned int>(x) > 0) {
            int count = x;
            do {
                infile->read(&x, sizeof(unsigned char));
                int heroKey = x & 0xff;

                unsigned char savedHeroId;
                infile->read(&savedHeroId, sizeof(savedHeroId));
                int heroId = savedHeroId;
                if (savedHeroId == g_savedHeroNone)
                    heroId = -1;

                std::string heroName = readLengthPrefixedString(infile);
                std::bitset<8> availability = readPackedBits<8>(infile);

                m_heroPlayerSetups.insert(
                    std::pair<const int, type_map_hero_info>(
                        heroKey,
                        type_map_hero_info(heroId, heroName, availability)));
            } while (--count != 0);
        }
    }

    if (infile->read(padding, sizeof(padding)) < sizeof(padding))
        return -1;
    return 0;
}

VA(0x004c4cc0, 0x130)
type_map_hero_info::type_map_hero_info(int portrait, std::string name,
                                      std::bitset<8> availability)
    : m_portrait(portrait), m_name(name), m_players(availability)
{
}

// PC-only post-header normalization. The available-hero bitset controls the
// live availability bytes, explicit per-player hero masks override the
// all-player default, and artifact victory conditions reserve their target
// before random artifact placement starts.
VA(0x004c4e30, 0xD3)  // sole new-map caller + map-header/member layout
void game::applyMapHeaderAvailability()
{
    for (int heroId = 0; heroId < HERO_COUNT; ++heroId) {
        if (m_mapHeader.m_availableHeroes.test(heroId))
            m_heroAvailability[heroId] = -1;
        else
            m_heroAvailability[heroId] = 0x40;
    }

    for (std::map<int, type_map_hero_info>::iterator it =
             m_mapHeader.m_heroPlayerSetups.begin();
         it != m_mapHeader.m_heroPlayerSetups.end(); ++it) {
        std::bitset<8> allPlayers;
        allPlayers.set();
        if (it->second.m_players != allPlayers)
            m_heroPoolMap[it->first] = it->second.m_players;
    }

    if (m_mapHeader.m_victoryCondition.m_type == VICTORY_CONDITION_ARTIFACT
        || m_mapHeader.m_victoryCondition.m_type
               == VICTORY_CONDITION_TRANSPORT_ARTIFACT) {
        m_artifactDisabled[m_mapHeader.m_victoryCondition.m_artifactNum] = 1;
    }
}

// Residual (87.0015%, 2026-08-26): the first fourteen CFG blocks are exact
// and the player/condition/map-entry write order is complete.  Retail keeps
// the fixed-name string's constructor and destructor expanded only through
// calls to _Tidy, while this TU's /Ob2 budget expands those _Tidy bodies and
// adds eleven CFG blocks.  Retail also packs the final availability byte as
// a one-byte array; spelling that literally reproduces its index arithmetic
// but changes the later bitset range-throw phase and falls to 80.44%.  The
// scalar spelling below is the measured whole-function plateau.  Reusing the
// saved name length, rather than calling length() again, is likewise
// codegen-significant: the second inline candidate moves the throw phase and
// falls to 79.19%.
VA(0x004c4f10, 0x71D)  // game::Save caller + DC identity + stream-write order
int NewSMapHeader::save(TAbstractFile* outfile)
{
    char enumBuffer;
    int count;
    int i;
    unsigned char ucharBuffer;
    char charBuffer;
    char boolBuffer;
    char sbyteBuffer;

    if (outfile->write(&m_version, sizeof(m_version)) < sizeof(m_version))
        return -1;

    boolBuffer = m_isPlayable;
    if (outfile->write(&boolBuffer, sizeof(boolBuffer))
        < sizeof(boolBuffer))
        return -1;

    if (outfile->write(&m_size, sizeof(m_size)) < sizeof(m_size))
        return -1;

    boolBuffer = m_hasTwoLayers;
    if (outfile->write(&boolBuffer, sizeof(boolBuffer))
        < sizeof(boolBuffer))
        return -1;

    if (game::saveString(outfile, m_mapName) < 0)
        return -1;
    if (game::saveString(outfile, m_mapDescription) < 0)
        return -1;

    ucharBuffer = m_difficulty;
    if (outfile->write(&ucharBuffer, sizeof(ucharBuffer))
        < sizeof(ucharBuffer))
        return -1;

    charBuffer = m_maxHeroLevel;
    outfile->write(&charBuffer, sizeof(charBuffer));

    TPlayerSlotAttributes* player = m_playerSlotAttributes;
    for (i = 0; i < 8; ++i, ++player) {

        boolBuffer = player->m_canBeHuman;
        if (outfile->write(&boolBuffer, sizeof(boolBuffer))
            < sizeof(boolBuffer))
            return -1;

        boolBuffer = player->m_canBeComputer;
        if (outfile->write(&boolBuffer, sizeof(boolBuffer))
            < sizeof(boolBuffer))
            return -1;

        enumBuffer = player->m_aiStrategy;
        if (outfile->write(&enumBuffer, sizeof(enumBuffer))
            < sizeof(enumBuffer))
            return -1;

        unsigned short alignmentBuffer = player->m_legalAlignments;
        outfile->write(&alignmentBuffer, sizeof(alignmentBuffer));

        boolBuffer = player->m_hasRandomAlignment;
        if (outfile->write(&boolBuffer, sizeof(boolBuffer))
            < sizeof(boolBuffer))
            return -1;

        boolBuffer = player->m_generateHero;
        if (outfile->write(&boolBuffer, sizeof(boolBuffer))
            < sizeof(boolBuffer))
            return -1;

        if (player->m_generateHero) {
            sbyteBuffer = player->m_castleLoc.m_x;
            if (outfile->write(&sbyteBuffer, sizeof(sbyteBuffer))
                < sizeof(sbyteBuffer))
                return -1;
            sbyteBuffer = player->m_castleLoc.m_y;
            if (outfile->write(&sbyteBuffer, sizeof(sbyteBuffer))
                < sizeof(sbyteBuffer))
                return -1;
            sbyteBuffer = player->m_castleLoc.m_z;
            if (outfile->write(&sbyteBuffer, sizeof(sbyteBuffer))
                < sizeof(sbyteBuffer))
                return -1;
        }

        enumBuffer = player->m_nonRandomHeroId;
        if (outfile->write(&enumBuffer, sizeof(enumBuffer))
            < sizeof(enumBuffer))
            return -1;

        if (player->m_nonRandomHeroId != -1) {
            enumBuffer = player->m_nonRandomHeroCustomPortrait;
            if (outfile->write(&enumBuffer, sizeof(enumBuffer))
                < sizeof(enumBuffer))
                return -1;

            std::string s;
            s = player->m_nonRandomHeroCustomName;
            if (game::saveString(outfile, s) < 0)
                return -1;
        }
    }

    enumBuffer = m_victoryCondition.m_type;
    if (outfile->write(&enumBuffer, sizeof(enumBuffer))
        < sizeof(enumBuffer))
        return -1;
    if (m_victoryCondition.m_type != -1)
        saveVictoryCondition(enumBuffer, outfile);

    enumBuffer = m_lossCondition.m_type;
    if (outfile->write(&enumBuffer, sizeof(enumBuffer))
        < sizeof(enumBuffer))
        return -1;

    if (m_lossCondition.m_type != -1)
        saveLossCondition(enumBuffer, outfile);

    enumBuffer = m_numTeams;
    if (outfile->write(&enumBuffer, sizeof(enumBuffer))
        < sizeof(enumBuffer))
        return -1;
    if (m_numTeams) {
        if (outfile->write(m_teamInfo, sizeof(m_teamInfo)) < sizeof(m_teamInfo))
            return -1;
    }

    ucharBuffer = m_heroPlayerSetups.size();
    outfile->write(&ucharBuffer, sizeof(ucharBuffer));
    for (std::map<int, type_map_hero_info>::iterator it =
             m_heroPlayerSetups.begin();
         it != m_heroPlayerSetups.end(); ++it) {
        enumBuffer = it->first;
        outfile->write(&enumBuffer, sizeof(enumBuffer));
        enumBuffer = it->second.m_portrait;
        outfile->write(&enumBuffer, sizeof(enumBuffer));

        count = it->second.m_name.length();
        outfile->write(&count, sizeof(count));
        outfile->write(it->second.m_name.c_str(), count);

        ucharBuffer = 0;
        for (i = 0; i < 8; ++i) {
            if (it->second.m_players.test(i))
                ucharBuffer |= 1 << i;
        }
        outfile->write(&ucharBuffer, sizeof(ucharBuffer));
    }

    return 0;
}

// Saved headers share the scenario header's scalar layout but add four
// versioned migrations: max hero level, widened alignment masks, custom hero
// records, and per-player availability masks.  The two old campaign hero ids
// use the same pre-25 remap as the other saved-game readers.
// Complete's availability loop has no DC spelling record. The direct
// bitset::set spelling used by the scenario reader lowers this saved-header
// reader from 90.8783% to 90.13% (84 to 83 exact CFG blocks); retail's
// retained bitset<8>::_Xran call still does not appear. Keep the proxy form.

VA(0x004c5630, 0x7CD)  // DC Load + saved-header callers + helper edges, dc 0xb0754
int NewSMapHeader::load(TAbstractFile* infile, int saveVersion)
{
    char enumBuffer;
    int count;
    int i;
    unsigned int availabilityIndex;
    unsigned char ucharBuffer;
    char charBuffer;
    char boolBuffer;
    int x;

    if (infile->read(&m_version, sizeof(m_version)) < sizeof(m_version))
        return -1;

    if (infile->read(&boolBuffer, sizeof(boolBuffer))
        < sizeof(boolBuffer))
        return -1;
    m_isPlayable = boolBuffer != 0;

    if (infile->read(&m_size, sizeof(m_size)) < sizeof(m_size))
        return -1;

    if (infile->read(&boolBuffer, sizeof(boolBuffer))
        < sizeof(boolBuffer))
        return -1;
    m_hasTwoLayers = boolBuffer != 0;

    if (game::loadString(infile, m_mapName) < 0)
        return -1;
    if (game::loadString(infile, m_mapDescription) < 0)
        return -1;

    if (infile->read(&ucharBuffer, sizeof(ucharBuffer))
        < sizeof(ucharBuffer))
        return -1;
    m_difficulty = ucharBuffer;

    if (saveVersion >= g_saveVersionMaxHeroLevel) {
        infile->read(&charBuffer, sizeof(charBuffer));
        m_maxHeroLevel = charBuffer;
    } else {
        m_maxHeroLevel = 0;
    }

    m_numPlayers = 0;
    m_minNumHumanPlayers = 0;
    m_maxNumHumanPlayers = 0;

    TPlayerSlotAttributes* player = m_playerSlotAttributes;
    for (i = 0; i < 8; ++i, ++player) {
        if (infile->read(&boolBuffer, sizeof(boolBuffer))
            < sizeof(boolBuffer))
            return -1;
        player->m_canBeHuman = boolBuffer != 0;

        if (infile->read(&boolBuffer, sizeof(boolBuffer))
            < sizeof(boolBuffer))
            return -1;
        player->m_canBeComputer = boolBuffer != 0;

        if (infile->read(&enumBuffer, sizeof(enumBuffer))
            < sizeof(enumBuffer))
            return -1;
        player->m_aiStrategy = enumBuffer;

        if (player->m_canBeHuman && !player->m_canBeComputer)
            ++m_minNumHumanPlayers;
        if (player->m_canBeHuman)
            ++m_maxNumHumanPlayers;
        if (player->m_canBeComputer || player->m_canBeHuman)
            ++m_numPlayers;

        if (saveVersion < g_saveVersionWideAlignments) {
            infile->read(&ucharBuffer, sizeof(ucharBuffer));
            player->m_legalAlignments = ucharBuffer;
        } else {
            unsigned short shortBuffer;
            infile->read(&shortBuffer, sizeof(shortBuffer));
            player->m_legalAlignments = shortBuffer;
        }

        if (infile->read(&boolBuffer, sizeof(boolBuffer))
            < sizeof(boolBuffer))
            return -1;
        player->m_hasRandomAlignment = boolBuffer != 0;

        if (infile->read(&boolBuffer, sizeof(boolBuffer))
            < sizeof(boolBuffer))
            return -1;
        player->m_generateHero = boolBuffer != 0;

        if (player->m_generateHero) {
            if (infile->read(&ucharBuffer, sizeof(ucharBuffer))
                < sizeof(ucharBuffer))
                return -1;
            player->m_castleLoc.m_x = ucharBuffer;
            if (infile->read(&ucharBuffer, sizeof(ucharBuffer))
                < sizeof(ucharBuffer))
                return -1;
            player->m_castleLoc.m_y = ucharBuffer;
            if (infile->read(&ucharBuffer, sizeof(ucharBuffer))
                < sizeof(ucharBuffer))
                return -1;
            player->m_castleLoc.m_z = ucharBuffer;
        }

        player->m_nonRandomHeroId =
            loadHeroId(infile, saveVersion);
        if (player->m_nonRandomHeroId != -1) {
            std::string strTemp;
            player->m_nonRandomHeroCustomPortrait =
                loadHeroId(infile, saveVersion);
            game::loadString(infile, strTemp);
            strcpy(player->m_nonRandomHeroCustomName, strTemp.c_str());
        } else {
            player->m_nonRandomHeroCustomPortrait = -1;
            player->m_nonRandomHeroCustomName[0] = 0;
        }
    }

    if (!m_minNumHumanPlayers)
        m_minNumHumanPlayers = 1;

    if (infile->read(&x, sizeof(unsigned char)) < sizeof(unsigned char))
        return -1;
    m_victoryCondition.m_type = x;
    m_victoryCondition.m_gameWon = 0;
    m_victoryCondition.m_playerWinner = -1;
    if (static_cast<unsigned char>(x) != g_savedHeroNone)
        loadVictoryCondition(x, infile, saveVersion);

    if (infile->read(&x, sizeof(unsigned char)) < sizeof(unsigned char))
        return -1;
    m_lossCondition.m_type = x;
    m_lossCondition.m_gameLost = 0;
    m_lossCondition.m_playerLoser = -1;
    if (static_cast<unsigned char>(x) != g_savedHeroNone)
        loadLossCondition(x, infile, saveVersion);

    if (infile->read(&x, sizeof(unsigned char)) < sizeof(unsigned char))
        return -1;
    m_numTeams = x;
    if (m_numTeams) {
        if (infile->read(m_teamInfo, sizeof(m_teamInfo)) < sizeof(m_teamInfo))
            return -1;
    } else {
        for (i = 0; i < 8; ++i)
            m_teamInfo[i] = i;
    }

    m_heroPlayerSetups.clear();
    if (saveVersion < g_saveVersionCustomHeroSetups)
        return 0;

    infile->read(&x, sizeof(unsigned char));
    x &= 0xff;
    if (static_cast<unsigned int>(x) <= 0)
        return 0;
    count = x;

    do {
        infile->read(&x, sizeof(unsigned char));
        int heroKey = x & 0xff;

        int heroId;
        infile->read(&heroId, sizeof(unsigned char));
        heroId &= 0xff;
        if (heroId == g_savedHeroNone)
            heroId = -1;

        std::string strTemp = readLengthPrefixedString(infile);
        std::bitset<8> availability;
        if (saveVersion >= g_saveVersionCustomHeroAvailability) {
            infile->read(&ucharBuffer, sizeof(ucharBuffer));
            for (availabilityIndex = 0;
                 availabilityIndex < 8; ++availabilityIndex) {
                availability[availabilityIndex] =
                    (ucharBuffer & (1 << (availabilityIndex & 7))) != 0;
            }
        } else {
            availability.set();
        }

        m_heroPlayerSetups.insert(
            std::pair<const int, type_map_hero_info>(
                heroKey, type_map_hero_info(heroId, strTemp, availability)));
    } while (--count != 0);

    return 0;
}

// The retained ret 0xc independently fixes the three explicit PC arguments.
VA(0x004c5e00, 0x210)  // DC Get + five PC callers + TGzFile/Read edges, dc 0xb0ea8
int NewSMapHeader::get(const char* path, const char* filename,
                       int campaignMap)
{
    std::string fullPath(path);
    fullPath += DATA_COMPGEN(0x00677dac, newMapGetPathSeparator, "\\");
    fullPath += filename;

    try {
        TGzFile infile(
            fullPath.c_str(),
            DATA_COMPGEN(0x00677d6c, newMapGetGzReadMode, "rb"));
        int result = read(&infile, campaignMap);
        if (result < 0)
            return -1;
    } catch (TGzFile::TOpenFailure) {
        return -1;
    }
    return 0;
}

// DC NewSMapHeader::readString (0xb1110), static with a string reference.
VA(0x004c6010, 0x1CE)  // dc 0xb1110
int __fastcall NewSMapHeader::readString(TAbstractFile* infile, std::string& s)
{
    int count;
    int length;

    count = infile->read(&length, sizeof(length));
    if (count < sizeof(length))
        return -1;

    if (length > 0 && length < 0xffff) {
        char* buffer = new char[length + 1];
        memset(buffer, 0, length + 1);
        count = infile->read(buffer, length);
        if (count < length)
            return -1;
        s = buffer;
        delete[] buffer;
    } else {
        s.erase();
    }

    return length;
}

VA(0x004c61e0, 0x4A8)  // dc 0xb1230
void game::claimTown(int townId, int newPlayerOwner, unsigned char isRemoteMove, unsigned char checkEndGame)
{
    town* thisTown = &m_towns[townId];
    long oldOwner = thisTown->m_owner;
    long i;
    if (oldOwner == newPlayerOwner)
        return;

    if (!g_inSetup && !isRemoteMove)
        recordClaimTown(townId, newPlayerOwner);

    thisTown->m_isGrouped = 0;

    if (!isRemoteMove) {
        for (i = 0; i < m_generators.size(); i++) {
            if (m_generators[i].getOwner() == oldOwner
                || m_generators[i].getOwner() == newPlayerOwner)
                m_generators[i].removeBonus();
        }
    }

    if (thisTown->m_owner != -1) {
        if (isComputerTeam(getTeam(thisTown->m_owner))
            && isHumanTeam(getTeam(newPlayerOwner)))
            thisTown->m_builtThisTurn = 0;
        g_game->getTown(townId)->deallocate();
    }

    thisTown->m_owner = newPlayerOwner;
    if (isRemoteMove)
        return;

    const_cast<armyGroup&>(
        static_cast<const town*>(thisTown)->getArmy()).initialize();
    if (thisTown->m_owner != -1) {
        m_players[newPlayerOwner].m_townIds[m_players[newPlayerOwner].m_numTowns] =
            static_cast<char>(townId);
        m_players[newPlayerOwner].m_numTowns++;

        if (checkEndGame
            && m_mapHeader.m_victoryCondition.isTownCaptureTarget(thisTown)
            && m_mapHeader.m_victoryCondition.checkForTownCaptureWin())
            ::checkEndGame(0);

        setVisibility(m_towns[townId].m_mapX, m_towns[townId].m_mapY,
                      m_towns[townId].m_mapZ, newPlayerOwner, 5, 0);

        if (thisTown->m_type == TOWN_TOWER) {
            if (thisTown->hasBuilding(EXTRA_0_ID, false))
                g_game->setVisibility(thisTown->m_mapX, thisTown->m_mapY,
                                      thisTown->m_mapZ, newPlayerOwner,
                                      20, false);
            if (thisTown->hasBuilding(HOLY_GRAIL_ID, false)) {
                g_game->setVisibility(g_mapWidth / 2, g_mapHeight / 2, 0,
                                      newPlayerOwner, g_mapWidth, 0);
                if (g_game->getNumMapLevels() > 1)
                    g_game->setVisibility(g_mapWidth / 2, g_mapHeight / 2, 1,
                                          newPlayerOwner, g_mapWidth, 0);
            }
        }
    }

    for (i = 0; i < m_generators.size(); i++) {
        if (m_generators[i].getOwner() == oldOwner
            || m_generators[i].getOwner() == newPlayerOwner)
            m_generators[i].updateBonus();
    }
}

VA(0x004c66e0, 0xCB)  // dc 0xb1748
void game::claimMine(int mineId, int newPlayerOwner, type_action_type actionType)
{
    mine& currentMine = m_mines[mineId];
    type_point location(currentMine.m_mapX, currentMine.m_mapY,
                        currentMine.m_mapZ);

    if (actionType == const_normal_action)
        recordClaimMine(mineId, newPlayerOwner);

    currentMine.m_playerOwner = newPlayerOwner;
    if (newPlayerOwner != -1)
        setVisibility(location.m_x, location.m_y, location.m_z,
                      newPlayerOwner, 3, 0);

    if (actionType && m_mapHeader.m_victoryCondition.checkForFlaggedMineWin())
        checkEndGame(0);
}

VA(0x004c67b0, 0x1A4)  // dc 0xb1828
void game::claimGenerator(int generatorId, int newPlayerOwner)
{
    generator& currentGenerator = m_generators[generatorId];
    sendMapChange(&CMCClaimGenerator(generatorId, newPlayerOwner));

    currentGenerator.setOwner(newPlayerOwner);
    if (newPlayerOwner != -1) {
        type_point location(currentGenerator.m_mapX, currentGenerator.m_mapY,
                            currentGenerator.m_mapZ);
        setVisibility(location.m_x, location.m_y, location.m_z,
                      newPlayerOwner, 3, 0);
    }

    if (m_mapHeader.m_victoryCondition.checkForFlaggedGeneratorWin())
        checkEndGame(0);
}

VA(0x004c6960, 0xC9)  // dc 0xb1988
void game::claimGarrison(int garrisonId, int newPlayerOwner)
{
    garrison& currentGarrison = m_garrisons[garrisonId];
    type_point location(currentGarrison.m_mapX, currentGarrison.m_mapY,
                        currentGarrison.m_mapZ);
    sendMapChange(&CMCClaimGarrison(garrisonId, newPlayerOwner));

    currentGarrison.m_playerOwner = newPlayerOwner;
    if (newPlayerOwner != -1)
        setVisibility(location.m_x, location.m_y, location.m_z,
                      newPlayerOwner, 3, 0);
}

VA(0x004c6a30, 0x21F)  // dc 0xb1a50
void game::claimShipyard(type_point location, int newPlayerOwner)
{
    hero* obscuringHero = 0;
    NewmapCell* mapCell = m_worldMap.cell(location);
    if (mapCell->m_type == HERO) {
        obscuringHero = g_game->getHero(mapCell->m_extraInfo);
        obscuringHero->restoreCell();
    }

    ShipyardInfo* shipyardInfo =
        static_cast<ShipyardInfo*>(
            static_cast<void*>(&mapCell->m_extraInfo));
    if (shipyardInfo->m_owner != newPlayerOwner) {
        if (shipyardInfo->m_owner >= 0) {
            playerData* oldPlayer = &m_players[shipyardInfo->m_owner];
            long index = 0;
            while (index < oldPlayer->m_shipyards.size()) {
                if (oldPlayer->m_shipyards[index].m_x == location.m_x &&
                    oldPlayer->m_shipyards[index].m_y == location.m_y &&
                    oldPlayer->m_shipyards[index].m_z == location.m_z)
                    break;
                ++index;
            }
            if (index < oldPlayer->m_shipyards.size())
                oldPlayer->m_shipyards.erase(
                    oldPlayer->m_shipyards.begin() + index);
        }

        if (newPlayerOwner >= 0) {
            setVisibility(location.m_x, location.m_y, location.m_z,
                          newPlayerOwner, 3, 0);
            m_players[newPlayerOwner].m_shipyards.push_back(location);
        }

        shipyardInfo->m_owner = newPlayerOwner;
        CMCClaimShipYard change(location, newPlayerOwner);
        sendMapChange(&change);
    }

    if (obscuringHero) {
        obscuringHero->obscureCell();
    }
}

VA(0x004c6c50, 0x2EB)  // dc 0xb1c8c
void game::viewArmy(armyGroup& group, int iarmy, const hero* thisHero,
                    const town* thisTown, int x, int y,
                    unsigned char showDismiss, unsigned char isQuickView)
{
    TCreatureType armyType = group.m_armyTypes[iarmy];
    const int numTroops = group.m_numTroops[iarmy];
    unsigned char hasAngelicAlliance = 0;
    TCreatureType upgradeToType = CREATURE_NONE;

    if (thisTown && getAlignment(armyType) == thisTown->m_type) {
        int i = DWELLING_0_ID;
        for (;;) {
            if (g_townDwellingCreatures[thisTown->m_type * 2
                                           * TOWN_DWELLING_COUNT
                                       + i - DWELLING_0_ID]
                    == armyType
                && thisTown->hasBuilding(town::upgradedDwellingID(i), true)) {
                upgradeToType = upgradedCreatureType(armyType);
                break;
            }
            if (++i > DWELLING_6_ID)
                break;
        }
    }

    if (thisHero) {
        const THeroSpecificAbility& ability =
            g_heroSpecificAbilities[thisHero->m_id];
        if (ability.m_type == eHeroAbilityCreatureUpgrade) {
            if (armyType == ability.m_creature
                || armyType == upgradedCreatureType(ability.m_creature)
                || armyType == ability.m_upgradeAlternateSubject
                || armyType == upgradedCreatureType(ability.m_upgradeAlternateSubject))
                upgradeToType = ability.m_upgradeResult;
        }
    }

    if (thisTown) {
        if (thisTown->m_owner >= 0)
            hasAngelicAlliance = m_players[thisTown->m_owner]
                                     .hasGivenArtifact(ARTIFACT_ANGELIC_ALLIANCE);
    } else if (thisHero) {
        if (thisHero->m_owner >= 0)
            hasAngelicAlliance = m_players[thisHero->m_owner]
                                     .hasGivenArtifact(ARTIFACT_ANGELIC_ALLIANCE);
        else
            hasAngelicAlliance =
                thisHero->isWieldingArtifact(
                    ARTIFACT_ANGELIC_ALLIANCE);
    }

    TViewArmyWindow* viewArmyWindow = new TViewArmyWindow(
        &group, iarmy, thisHero, thisTown, x, y, upgradeToType, showDismiss,
        !isQuickView, hasAngelicAlliance);
    if (isQuickView) {
        viewArmyWindow->quickView();
    } else {
        viewArmyWindow->doModal();
        switch (g_windowManager->m_dialogReturn) {
        case TViewArmyWindow::DISMISS_ID:
            group.dismiss(iarmy);
            break;
        case TViewArmyWindow::UPGRADE_ID: {
            long upgradeCost[NUM_RESOURCES];
            getUpgradeCost(armyType, upgradeToType, numTroops, upgradeCost);
            for (int i = 0; i < NUM_RESOURCES; i++)
                g_currentPlayer->m_resources[i] -= upgradeCost[i];
            group.m_armies[iarmy] = upgradeToType;
            break;
        }
        }
    }
    delete viewArmyWindow;
}

// Original GetRandomNumTroops, game.cpp:7572, dc 0xb1f1c. Ordinary
// member expanded in RandomizeEvents; no retained retail row is known.
int game::getRandomNumTroops(int whichMon)
{
    return random(g_creatureTypeTraits[whichMon].m_wanderingLow,
                  g_creatureTypeTraits[whichMon].m_wanderingHigh);
}


VA(0x004c6f40, 0x3F)  // dc 0xb1f60
void startAITheme()
{
    char name[40];

    sprintf(name, DATA_COMPGEN(0x00677eac, aiThemeFormat, "AITheme%d"),
            random(1, 3) - 1);
    g_soundManager->startMP3(name, 0, 1);
}

VA(0x004c6f80, 0x4F)  // dc 0xb1fa4
void game::turnOnAIMusic()
{
    startAITheme();
    g_soundManager->m_playSounds = 0;
}

VA(0x004c6fd0, 0x10)  // dc 0xb1fc0
void game::turnOffAIMusic()
{
    g_soundManager->m_playSounds = 1;
}

// E:\gamedcs\game.cpp:7603
// The retail body preserves the HoMM2 turn-transition skeleton while adding
// Complete's victory-condition sweep, local-human turn timer, network-state
// transfer/retry and visiting/recruit hero refreshes. The roster adjacency
// (immediately after TurnOffAIMusic), five retail callers and the complete DC
// callee set independently identify the row.

// RECONSTRUCTED 2026-08-26 (83.3584%). The frame is retail's exact 0x150 and
// the 256-byte sText home, 40-byte inlined AI-theme scratch, turn message,
// saved-player slot and retained autosave-census write all land in the retail
// stack bands. The remaining structural residual is 69 candidate branches
// against 73 retail. Most of its visible churn is one allocation family around
// the failed-transfer tail retry: our VC6 homes the receiver and carries it in
// ESI while keeping zero in EBX; retail keeps the receiver in EBX and zero in
// ESI. Explicit backward goto, a structured retry loop and recursive-inlining
// pragma all reproduced the same allocation family. Forcing the week-transition
// value to a volatile byte worsened the score to 80.1988% and grew the frame.
// 2026-09-05, the eight missing branches LOCATED and the cause named.
// Retail emits fourteen instructions at fn+0xd9..+0xf9 that our compile
// deletes outright - the `iHumans` scan inside the LocalHuman/698770 arm:
//   d9: xor eax,eax / db: mov cl,[ebx+eax+0x1f636] / e2: test cl,cl
//   e4: jne <inc> / e6: cmp eax,8 / e9: mov [ebp-0xc],eax / ec: jge <z>
//   ee: cmp eax,esi / f0: jge <inc> / f2(z): mov [ebp-0xc],esi
//   f5: inc eax / f6: cmp eax,8 / f9: jl <head>
// which is the loop this source already carries, statement for statement
// and test for test, including the `i >= 8 || i < 0` clamp order.  The
// source is NOT wrong.  `[ebp-0xc]` is written at exactly those two sites
// and READ NOWHERE in retail's whole 0x947 bytes, so the stores are dead
// on BOTH sides and retail's C2 simply did not eliminate them where ours
// does; there is no aliasing, no address-take and no later reader to
// restore.  Four of the eight branch deficit and the four blocks are this
// one loop.  Do not "fix" it with a `volatile` (the cleanliness floor is
// 0) or with a carrier statement; if a reader for iHumans ever turns up
// in the retail bytes, this loop comes back for free.
VA(0x004c6fe0, 0x947)  // dc-name/order + retail caller/callee/body, dc 0xb1fd0
void game::nextPlayer()
{
    int toWho;
    int weekSave;
    int humans;
    int i;
    unsigned char lastWasHuman;
    int giCurPlayerSave;
    int save;
    unsigned char makeOrig;

    m_mapHeader.m_victoryCondition.checkForArtifactWin();
    m_mapHeader.m_victoryCondition.checkForTotalCreatures();
    m_mapHeader.m_victoryCondition.checkForTotalResources();
    m_mapHeader.m_victoryCondition.checkForUpgradedTown();
    m_mapHeader.m_victoryCondition.checkForGrailBuildingWin();
    checkEndGame(0);
    if (g_gameOver)
        return;

    giCurPlayerSave = g_netLocalGamePos;

    for (i = 0; i < 2; ++i) {
        int recruitId = g_currentPlayer->m_recruits[i];
        if (recruitId != -1)
            m_heroes[recruitId].m_flags &= ~g_heroRecruitReservedFlag;
    }

    if (g_currentPlayer->isLocalHuman())
        g_turnDuration.clear();
    g_curHourGlassPhase = 0;

    if (g_currentPlayer->isLocalHuman() && g_config.m_autosave) {
        for (i = 0; i < 8; ++i) {
            if (!m_playerDisabled[i]) {
                humans = i;
                if (i >= 8 || i < 0)
                    humans = 0;
            }
        }
        g_advManager->drawRolloverText(
            const_cast<char*>(g_generalText->getText(GENERAL_TEXT_AUTOSAVING)));
        saveGame(g_generalText->getText(GENERAL_TEXT_AUTOSAVE_NAME), 1, 0, 1, 0);
        g_advManager->drawRolloverText(
            DATA_COMPGEN(0x00691210, nextPlayerEmptyRollover, ""));
    }

    if (g_game->m_players[g_netLocalGamePos].m_deathCountDown > 0)
        --g_game->m_players[g_netLocalGamePos].m_deathCountDown;
    g_advManager->deactivateCurrTown(0);
    g_advManager->deactivateCurrHero(0);

    makeOrig = 0;
    lastWasHuman = m_players[g_netLocalGamePos].isHuman();
    for (;;) {
        ++g_netLocalGamePos;
        if (g_netLocalGamePos < 8) {
            if (!m_playerDisabled[g_netLocalGamePos]
                && static_cast<unsigned char>(
                       m_players[g_netLocalGamePos].isHuman())
                       == lastWasHuman) {
                break;
            }
        } else {
            weekSave = !lastWasHuman;
            if (weekSave) {
                makeOrig = 1;
                perDay();
                if (g_remoteOn) {
                    m_mapHeader.m_lossCondition.checkForTimeLimitExpired();
                    ::checkEndGame(0);
                    if (g_gameOver)
                        return;
                }
            }
            lastWasHuman = static_cast<unsigned char>(weekSave);
            g_netLocalGamePos = -1;
        }
    }

    if (g_goSolo && makeOrig
        && (!g_remoteOn || g_numHumanPlayers == 1)) {
        g_advManager->drawRolloverText(
            const_cast<char*>(g_generalText->getText(GENERAL_TEXT_AUTOSAVING)));
        saveGame(g_generalText->getText(GENERAL_TEXT_AUTOSAVE_NAME), 1, 0, 1, 0);
        g_advManager->drawRolloverText(
            DATA_COMPGEN(0x00691210, nextPlayerSoloEmptyRollover, ""));

        save = g_remoteOn;
        g_remoteOn = 1;
        g_goSolo = 0;
        normalDialogTimeOut(g_generalText->getText(GENERAL_TEXT_PRESS_ESC_TO_CANCEL_SOLO_MODE), 2, 2000,
                            -1, -1, -1, 0, -1, 0, -1, -1, 0);
        g_remoteOn = save;
        if (g_windowManager->m_dialogReturn == DIALOG_RETURN_DECLINE) {
            g_game->m_players[g_soloPos].m_isHuman = 1;
            g_game->m_players[g_soloPos].m_isLocal = 1;
            g_goSolo = 0;
            g_mapVisibilityBit = 1 << g_soloPos;
        } else {
            g_goSolo = 1;
        }
    }

    g_currentPlayer = &g_game->m_players[g_netLocalGamePos];
    g_curPlayerBit = 1 << g_netLocalGamePos;

    if (g_remoteOn && !g_currentPlayer->isHuman()) {
        CTurnUpdateMsg msg(g_netLocalGamePos);
        transmitRemoteData(&msg, 0x7f, false, true);
    }
    clearEventRecords(static_cast<char>(g_netLocalGamePos));

    for (i = 0; i < g_currentPlayer->m_numHeroes; ++i) {
        hero* currentHero = &m_heroes[g_currentPlayer->m_heroes[i]];
        int mobility = currentHero->getMobility();
        currentHero->m_maxMovePoints = mobility;
        currentHero->m_movePoints = mobility;
    }
    for (i = 0; i < 2; ++i) {
        int recruitId = g_currentPlayer->m_recruits[i];
        if (recruitId != -1) {
            hero* currentHero = &m_heroes[recruitId];
            int mobility = currentHero->getMobility();
            currentHero->m_maxMovePoints = mobility;
            currentHero->m_movePoints = mobility;
        }
    }
    for (i = 0; i < g_currentPlayer->m_numTowns; ++i) {
        town* currentTown = getTown(g_currentPlayer->m_townIds[i]);
        if (currentTown->m_visitingHeroId >= 0) {
            hero* currentHero = getHero(currentTown->m_visitingHeroId);
            int mobility = currentHero->getMobility();
            currentHero->m_maxMovePoints = mobility;
            currentHero->m_movePoints = mobility;
            currentHero->m_isSleeping = 0;
        }
    }

    if (!g_currentPlayer->isLocalHuman()) {
        g_mouseManager->setPointer(2, mouseManager::DEFAULT_SET);
        g_advManager->hideRoute(1, 0, 1);
        startAITheme();
        g_soundManager->m_playSounds = 0;
        setNoDialogMenus(0);
        g_advManager->overrideBottomView(advManager::BOTTOM_VIEW_8, -1);
        showComputerScreen();
        g_completeDrawEnabled = 0;

        if (g_remoteOn && isHuman(g_netLocalGamePos)) {
            toWho = g_netLocalGamePos;
            makeOrig = 0;
            g_thisNetGotAdventureControl = 0;
            if (g_playerDrop) {
                toWho = 0x7f;
                g_playerDrop = 0;
            }
            if (isLastHuman(getLocalPlayerGamePos())) {
                toWho = 0x7f;
                g_playerDrop = 0;
                makeOrig = 1;
            }
            save = transmitSaveGame(toWho, 0, 1, makeOrig);
            if (!save && g_playerDrop) {
                g_netLocalGamePos = giCurPlayerSave;
                g_playerDrop = 0;
                g_currentPlayer = &g_game->m_players[g_netLocalGamePos];
                g_curPlayerBit = 1 << g_netLocalGamePos;
                nextPlayer();
                return;
            }
            g_advManager->updateRadar(1, 1, 0, 0, 0);
            g_playerTurn = g_netLocalGamePos;
        }
        g_advManager->overrideBottomView(advManager::BOTTOM_VIEW_DEFAULT, -1);
    } else {
        setNoDialogMenus(1);
        g_inputManager->flush();
        g_curWatchPlayer = g_netLocalGamePos;
        g_mapVisibilityBit = g_curPlayerBit;

        if (g_blackoutPlayer && g_numHumanPlayers > 1) {
            char textBuffer[256];
            sprintf(textBuffer, g_generalText->getText(
                        GENERAL_TEXT_PLAYER_TURN_FORMAT),
                    g_currentPlayer->getName());
            waitForPlayer(textBuffer, g_netLocalGamePos);
        }

        if (g_currentPlayer->isLocalHuman()
            || (g_remoteOn && g_currentPlayer->isHuman())) {
            cancelComputerScreen();
        }
    }

    if (g_currentPlayer->isLocalHuman())
        g_turnDuration.start();
    doNewTurn();
    if (g_currentPlayer->isLocalHuman())
        g_advManager->forceNewHover();
}

// Original: game::clear_recruits; game.cpp:8266, dc 0xb3d8c
void game::clearRecruits(int* recruits)
{
    for (int i = 0; i < 2; ++i) {
        int heroId = recruits[i];
        if (heroId >= 0) {
            hero* recruitHero = getHero(heroId);
            if (recruitHero->m_flags & g_heroRecruitReservedFlag)
                continue;
            m_heroAvailability[heroId] = -1;
            recruits[i] = -1;
        }
    }
}

// Original: get_new_hero; game.cpp:8290, dc 0xb3e00
int getNewHero(THeroClass heroClass)
{
    int heroId = 0;
    for (; heroId < game::HERO_COUNT; ++heroId) {
        if (g_game->getHero(heroId)->m_heroClass == heroClass
            && g_game->m_heroAvailability[heroId] == -1)
            return heroId;
    }
    return heroId;
}

VA(0x004c7930, 0x266)  // dc 0xb2ad4
int game::computeDailyGold(int whichPlayer, unsigned char includeSilo)
{
    const playerData& p = m_players[whichPlayer];
    int gold = 0;
    int i;

    for (i = 0; i < m_mines.size(); ++i) {
        if (m_mines[i].m_playerOwner == whichPlayer && m_mines[i].m_type == GOLD)
            gold += 1000;
    }

    for (i = 0; i < m_towns.size(); ++i) {
        if (m_towns[i].m_owner == whichPlayer) {
            gold += m_towns[i].getGoldIncome(includeSilo);
            if (m_towns[i].m_garrisonHeroId >= 0)
                gold += getHero(m_towns[i].m_garrisonHeroId)->getEstatesBonus();
        }
    }

    gold += p.numOfGivenArtifact(
                 g_productionArtifactEndlessSackOfGold) * 1000;
    gold += p.numOfGivenArtifact(
                 g_productionArtifactEndlessBagOfGold) * 750;
    gold += p.numOfGivenArtifact(
                 g_productionArtifactEndlessPurseOfGold) * 500;

    for (i = 0; i < p.m_numHeroes; ++i)
        gold += getHero(p.m_heroes[i])->getEstatesBonus();

    int humanId = whichPlayer;
    if (humanId >= 8 || humanId < 0)
        humanId = 0;
    if (!m_players[humanId].m_isHuman) {
        if (m_setup.m_difficulty == g_gameDifficultyEasy)
            gold = static_cast<int>(gold * 0.75);
        if (m_setup.m_difficulty == g_gameDifficultyExpert)
            gold = static_cast<int>(gold * 1.25);
        if (m_setup.m_difficulty == g_gameDifficultyImpossible)
            gold = static_cast<int>(gold * 1.5);
    }

    if (m_setup.m_handicap[whichPlayer] == NEW_MAP_HANDICAP_MILD)
        return static_cast<int>(gold * 0.85);
    if (m_setup.m_handicap[whichPlayer] == NEW_MAP_HANDICAP_SEVERE)
        return static_cast<int>(gold * 0.7);
    return gold;
}

// DC game.cpp:7964/7970 uses town-vector accesses directly, not a cached
// pointer. Restoring those accesses preserves the retained retail body at 100%.
// The public's _N return and retail's direct AL load also require a bool
// result local: unsigned char adds a normalization at the return. DC lowers
// both source bool and unsigned char to T_UCHAR, so that record cannot decide.
VA(0x004c7ba0, 0xAC)  // dc 0xb3030
bool game::growCoverOfDarkness()
{
    bool changed = false;
    for (int i = 0; i < m_towns.size(); ++i) {
        if (m_towns[i].m_type == TOWN_NECROPOLIS
            && m_towns[i].hasBuilding(SPECIAL_BUILDING_ID, false)) {
            g_game->resetVisibility(m_towns[i].m_mapX, m_towns[i].m_mapY,
                                    m_towns[i].m_mapZ, m_towns[i].m_owner,
                                    20);
            changed = true;
        }
    }
    return changed;
}

VA(0x004c7c50, 0x389)  // dc 0xb317c
void game::resetAllPlayerVisibility()
{
    int i;
    for (i = 0; i < HERO_COUNT; ++i) {
        if (m_heroes[i].m_owner != -1) {
            setVisibility(m_heroes[i].m_x, m_heroes[i].m_y, m_heroes[i].m_z,
                          m_heroes[i].m_owner, m_heroes[i].getVisibility(), 0);
        }
    }

    for (i = 0; i < m_towns.size(); ++i) {
        int range = 5;
        if (m_towns[i].m_type == TOWN_TOWER
            && m_towns[i].hasBuilding(EXTRA_0_ID, false)) {
            range = 20;
        }

        if (m_towns[i].m_owner != -1) {
            setVisibility(m_towns[i].m_mapX, m_towns[i].m_mapY, m_towns[i].m_mapZ,
                          m_towns[i].m_owner, range, 0);
            if (m_day == 1 && m_week == 1 && m_month == 1
                && m_towns[i].m_type == TOWN_TOWER
                && m_towns[i].hasBuilding(HOLY_GRAIL_ID, false)) {
                setVisibility(g_mapWidth / 2, g_mapHeight / 2, 0,
                              m_towns[i].m_owner, g_mapWidth, 0);
                if (m_worldMap.getNumLevels() > 1) {
                    setVisibility(g_mapWidth / 2, g_mapHeight / 2, 1,
                                  m_towns[i].m_owner, g_mapWidth, 0);
                }
            }
        }
    }

    for (i = 0; i < m_mines.size(); ++i) {
        if (m_mines[i].m_playerOwner != -1) {
            setVisibility(m_mines[i].m_mapX, m_mines[i].m_mapY, m_mines[i].m_mapZ,
                          m_mines[i].m_playerOwner, 3, 0);
        }
    }

    for (i = 0; i < m_generators.size(); ++i) {
        if (m_generators[i].getOwner() != -1) {
            setVisibility(m_generators[i].m_mapX, m_generators[i].m_mapY,
                          m_generators[i].m_mapZ, m_generators[i].getOwner(),
                          3, 0);
        }
    }

    for (i = 0; i < m_garrisons.size(); ++i) {
        if (m_garrisons[i].m_playerOwner != -1) {
            setVisibility(m_garrisons[i].m_mapX, m_garrisons[i].m_mapY,
                          m_garrisons[i].m_mapZ, m_garrisons[i].m_playerOwner,
                          3, 0);
        }
    }

    for (int z = 0; z < g_game->m_worldMap.getNumLevels(); ++z) {
        for (int y = 0; y < g_mapHeight; ++y) {
            for (int x = 0; x < g_mapWidth; ++x) {
                NewmapCell* tempCell = g_game->m_worldMap.cell(x, y, z);
                ShipyardInfo* shipyardInfo = static_cast<ShipyardInfo*>(
                    static_cast<void*>(&tempCell->m_extraInfo));
                if (tempCell->m_isTrigger && tempCell->m_type == SHIPYARD
                    && shipyardInfo->m_owner != -1) {
                    setVisibility(x, y, z, shipyardInfo->m_owner, 3, 0);
                }
            }
        }
    }
}

// Dreamcast game.cpp:8123 passes town.m_owner directly to is_human_ally.
// Its canonical GetTeam wrapper performs the one team lookup visible in
// Windows retail at +0xa5; pre-mapping here made the candidate look it up twice.
// Dreamcast lines 8124 and 8126 place the zero store before the decrement.
// Mac retail retains that arm order; the zero-first condition also preserves
// the exact Windows body.
// Mac perDay is 1746/1764 bytes with 15/15 calls. Its remaining 18 bytes
// encode only a 0xe0 target versus 0xd0 candidate frame and the consequent
// scratch-slot displacements; DC's typed array/reference locals and town_id
// have been restored without changing that frame.
VA(0x004c7fe0, 0x462)  // dc 0xb3858
// DC game.cpp:8094..8254 records hero/array references and a long town_id.
// is_human_ally takes the owner/player, then expands GetTeam and calls
// IsHumanTeam. The previous team-taking duplicate lost that source boundary.
// Restoring the canonical pair and GrowCoverOfDarkness's unsigned-byte result
// recovers 100% without the inline-depth pin. Bool grow returned 63.0871%
// unpinned; the byte result alone preserved the earlier 75.1291% caller.
void game::perDay()
{
    ++m_day;
    if (!g_gameOver) {
        if (m_day > 7) {
            m_day = 1;
            perWeek();
        }
        if (static_cast<unsigned short>(m_week) > 4) {
            m_week = 1;
            perMonth();
        }
    }

    int i;
    for (i = 0; i < m_towns.size(); ++i) {
        if (g_game->m_setup.m_difficulty >= 2
            || isHumanAlly(m_towns[i].m_owner)
            || !m_towns[i].m_builtThisTurn) {
            m_towns[i].m_builtThisTurn = 0;
        } else {
            --m_towns[i].m_builtThisTurn;
        }
    }

    for (i = 0; i < HERO_COUNT; ++i) {
        hero& currHero = m_heroes[i];
        currHero.m_flags &= 0xfffdfffeU;
        currHero.m_disguiseLevel = -1;
        currHero.m_flightLevel = -1;
        currHero.m_waterWalkLevel = -1;
        currHero.m_visionsPower = -1;
        currHero.m_dWalkSpellsCast = 0;
    }

    if (growCoverOfDarkness())
        resetAllPlayerVisibility();

    if (m_day == 1) {
        for (long townId = 0; townId < m_towns.size(); ++townId) {
            town& currentTown = m_towns[townId];
            if (currentTown.m_type == TOWN_RAMPART
                && currentTown.hasBuilding(SPECIAL_BUILDING_ID, true)) {
                currentTown.m_pondResource = g_resources[random(0, 3)];
                currentTown.m_pondAmount = random(1, 4);
            } else {
                currentTown.m_pondResource = -1;
                currentTown.m_pondAmount = 0;
            }
        }
    }

    calculateProduction();
    for (i = 0; i < 8; ++i) {
        if (!m_playerDisabled[i]) {
            long (&production)[NUM_RESOURCES] = m_players[i].m_ai.m_turnProductionResource;
            long (&resources)[NUM_RESOURCES] = m_players[i].m_resources;
            for (int j = 0; j < NUM_RESOURCES; ++j)
                resources[j] += production[j];
        }
    }

    if (m_mapHeader.m_victoryCondition.checkForTotalResources())
        checkEndGame(0);

    for (i = 0; i < HERO_COUNT; ++i) {
        hero& currHero = m_heroes[i];
        int maxMana = currHero.getMaxMana();
        if (currHero.isWieldingArtifact(g_artifactWizardsWellId)) {
            if (maxMana > currHero.m_mana)
                currHero.m_mana = maxMana;
        } else {
            int tempMana = currHero.m_mana;
            tempMana += currHero.getMysticismBonus();
            if (tempMana > maxMana)
                tempMana = maxMana;
            if (tempMana > currHero.m_mana)
                currHero.m_mana = tempMana;
        }
    }

    for (i = 0; i < m_towns.size(); ++i) {
        town* currTown = getTown(i);
        if (currTown->hasBuilding(MAGE_GUILD_ID, true)) {
            if (currTown->m_visitingHeroId != -1) {
                hero* currHero = getHero(currTown->m_visitingHeroId);
                int maxMana = currHero->getMaxMana();
                if (maxMana > currHero->m_mana)
                    currHero->m_mana = static_cast<short>(maxMana);
            }
            if (currTown->m_garrisonHeroId != -1) {
                hero* currHero = getHero(currTown->m_garrisonHeroId);
                int maxMana = currHero->getMaxMana();
                if (maxMana > currHero->m_mana)
                    currHero->m_mana = static_cast<short>(maxMana);
            }
        }
    }

    m_grailAsked = 0;
}

// Original: game::set_weekly_recruits; game.cpp:8308, dc 0xb3e60
// Complete passes a player index instead of the older recruits/align pair.
// The two-slot loop, tutorial choices and equipment/mana/army closeout identify
// this body; DC's nullary set_recruits is the surrounding all-player operation.
// Mac 0xdf0ac initializes the local artifact's id before its extra field.
// The TArtifact constructor preserves that order in VC6; the default
// constructor reverses the two stores in this caller.
VA(0x004c8450, 0x248)
void game::setWeeklyRecruits(int playerPos)
{
    playerData* player = &m_players[playerPos];
    type_artifact artifact(ARTIFACT_NONE);
    int recruitSlot;

    for (recruitSlot = 0; recruitSlot < 2; ++recruitSlot) {
        if (player->m_recruits[recruitSlot] >= 0)
            continue;

        THeroClass otherClass;
        if (player->m_recruits[1 - recruitSlot] < 0)
            otherClass = kNumHeroClasses;
        else
        {
            otherClass = THeroClass(getHero(player->m_recruits[1 - recruitSlot])->m_heroClass);
        }

        int heroId;
        if (m_isTutorial
            && static_cast<unsigned short>(m_week) <= 2) {
            if (m_week == 1) {
                heroId = getNewHero(classCleric);
            } else if (recruitSlot == 0) {
                heroId = getNewHero(classPagan);
            } else {
                heroId = getNewHero(classHeretic);
            }
        } else {
            heroId = getNewHeroId(
                playerPos, otherClass, recruitSlot == 0, kNumHeroClasses);
        }

        player->m_recruits[recruitSlot] = heroId;
        if (heroId == -1)
            continue;

        m_heroAvailability[heroId] = 64;
        hero* newHero = &m_heroes[heroId];
        int backpackSlot = HERO_BACKPACK_CAPACITY - 1;
        do {
            artifact = newHero->getBackpack(backpackSlot);
            if (artifact.m_artifactId != -1
                && newHero->equipArtifact(&artifact, -1))
                newHero->removeBackpackArtifact(backpackSlot);
        } while (backpackSlot--);

        newHero->m_mana = static_cast<short>(newHero->getMaxMana());
        setRandomHeroArmies(heroId, 0, 0);
    }
}

VA(0x004c86a0, 0xD5)
void game::replaceRecruit(int playerPos, long recruitSlot)
{
    THeroClass otherClass = kNumHeroClasses;
    playerData* player = &m_players[playerPos];
    if (player->m_recruits[1 - recruitSlot] != -1)
    {
        otherClass = THeroClass(getHero(player->m_recruits[1 - recruitSlot])->m_heroClass);
    }

    int heroId = getNewHeroId(playerPos, otherClass, 0, kNumHeroClasses);
    player->m_recruits[recruitSlot] = heroId;
    if (heroId != -1) {
        m_heroAvailability[heroId] = 64;
        m_heroes[heroId].m_mana = static_cast<short>(m_heroes[heroId].getMaxMana());
        setRandomHeroArmies(heroId, 0, 1);
    }
}

// Original: game::set_recruits; game.cpp:8373, dc 0xb40f4
// Complete's NewMap and PerWeek both expand these two eight-player passes.
// The DC neutral recruit pair (receiver+0xe188) and its extra clear/fill calls
// disappeared from the desktop game layout and both retail expansions.
void game::setRecruits()
{
    long i;
    for (i = 0; i < 8; ++i)
        clearRecruits(m_players[i].m_recruits);
    for (i = 0; i < 8; ++i) {
        if (m_playerDisabled[i])
            continue;
        setWeeklyRecruits(i);
    }
}

// E:\gamedcs\game.cpp:8398
// Complete's weekly pass retains the Dreamcast phase order and call graph,
// but its creature domain includes the Complete roster and its map-cell
// monster count uses the retail split 16-bit encoding. The retail successor
// relationship from PerDay and predecessor relationship to PerMonth close the
// otherwise ambiguous Dreamcast bracket.

// Residual (98.8948%, HIST 99.8370%): restoring the Dreamcast-proven IsCastle
// source boundary made the entire neutral-town arm exact; Complete's retail
// bytes separately select HasBuilding's built-mask lane for the Summoning
// Portal test. The residual is the opening creature-week scan's C1 handle-state
// ESI/EDI role permutation (`this` versus `i`), which why-reg proves
// source-unaddressable: the reference binds `i` to ESI and `this` to EDI, so
// `i` would have to be the earlier-created call-crossing pseudo. PROVED
// IMPOSSIBLE by the front end: `il-locals` shows this body's handles as
// this 0xc5c6 then obscuringHero/align/alternateBonus/bonusAmount/x/y/i 0xc5ce,
// i.e. `this` precedes every local, and handle-order.md measures params < `this`
// < locals as parse-FIXED with assignment strictly top-to-bottom. No declaration
// order can put `i` ahead of `this`, so the binding is TU state, not source. THE 99.8370 HIST WAS REACHED BY
// THIS EXACT src_hash (1078057cac97 at f8570b07/a5348767, CUR 98.8948 in the
// same row), so the permutation is a TU-state effect that some include closure
// already produced - not a lost source shape. Byte-flat here: `int i = 0` at the
// declaration, moving `i` first in the declaration block, assigning `i` before
// the two CREATURE_NONE stores, and binding the MONSTER arm's packed dword to a
// local. The MONSTER arm's own four-byte split (retail consumes the loaded
// m_extraInfo in place and reloads it for the preserved lanes, ours copies it)
// rides on the same allocator phase.
VA(0x004c8780, 0x7B7)  // PerDay/PerMonth bracket + dc lines/callees, dc 0xb41e0
void game::perWeek()
{
    hero* obscuringHero;
    int align;
    TCreatureType alternateBonus;
    long bonusAmount;
    int x;
    int y;
    int i;
    int z;
    TCreatureType bonusCreature;
    NewmapCell* mapCell;
    int count;
    int increase;
    int luckBonus;
    hero* currHero;

    bonusCreature = CREATURE_NONE;
    alternateBonus = CREATURE_NONE;
    i = 0;

    g_weekType = g_weekTypeNormal;
    g_weekTypeExtra = random(0, g_weekNameLast);
    bonusAmount = g_creatureWeekGrowthBonus;

    if (m_week != g_weeksPerMonth
        && random(1, g_specialWeekRollMax) == 1) {
        g_weekType = g_weekTypeCreature;

        for (align = m_gameVersion ? CREATURE_CATAPULT : CREATURE_PIXIE;
             align--;) {
            if ((m_gameVersion
                 || !isBaseElemental(align))
                && g_creatureTypeTraits[align].m_townType != -1
                && g_creatureTypeTraits[align].m_level >= 0)
                ++i;
        }

        i = rand() % i;
        for (align = m_gameVersion ? CREATURE_CATAPULT : CREATURE_PIXIE;
             align--;) {
            if ((m_gameVersion
                 || !isBaseElemental(align))
                && g_creatureTypeTraits[align].m_townType != -1
                && g_creatureTypeTraits[align].m_level >= 0) {
                if ((m_gameVersion
                     || align == CREATURE_AIR_ELEMENTAL
                     || align == CREATURE_EARTH_ELEMENTAL
                     || align == CREATURE_FIRE_ELEMENTAL
                     || align == CREATURE_WATER_ELEMENTAL
                     || g_creatureTypeTraits[align].m_townType != TOWN_CONFLUX)
                    && i-- <= 0)
                    break;
            }
        }
        g_weekTypeExtra = align;
        {
            bonusCreature = TCreatureType(align);
        }
    }

    for (i = 0; i < m_towns.size(); ++i) {
        if (m_towns[i].m_type == TOWN_INFERNO
            && m_towns[i].hasBuilding(HOLY_GRAIL_ID, false)) {
            g_weekType = g_weekTypeInfernoGrail;
            {
                bonusCreature = TCreatureType(g_creatureImpId);
            }
            {
                alternateBonus = TCreatureType(g_creatureFamiliarId);
            }
            bonusAmount = g_creatureTypeTraits[g_creatureImpId].m_growthRate;
            g_weekTypeExtra = g_creatureImpId;
            break;
        }
    }

    for (i = 0; i < m_towns.size(); ++i)
        m_towns[i].increasePopulation(
            bonusCreature, alternateBonus, bonusAmount);

    for (i = 0; i < m_towns.size(); ++i)
        m_towns[i].m_manaVortexFull = 1;

    ++m_week;

    setRecruits();

    for (i = 0; i < m_generators.size(); ++i) {
        generator* currentGenerator = &m_generators[i];
        currentGenerator->grow(0);
    }

    for (z = 0; z < getNumMapLevels(); ++z) {
        y = 0;
        if (g_mapHeight > 0) {
            do {
                for (x = 0; x < g_mapWidth; ++x) {
                mapCell = m_worldMap.cell(x, y, z);
                if (!mapCell->m_isTrigger)
                    continue;

                if (mapCell->m_type != HERO) {
                    obscuringHero = 0;
                } else {
                    // DC game.cpp:8476 names GetHero here.
                    obscuringHero = getHero(mapCell->m_extraInfo);
                    obscuringHero->restoreCell();
                }

                switch (mapCell->m_type) {
                case MAGIC_SPRING:
                    mapCell->fillMagicSpring(1);
                    break;

                case MONSTER: {
                    if (!(mapCell->m_extraInfo & 0x40000)) {
                        count = ((mapCell->m_extraInfo & 0xfff) << 4)
                                + ((mapCell->m_extraInfo >> 27) & 0xf);
                        increase = count / 10;
                        count += increase;
                        if (count > 64000)
                            count = 64000;
                        // Retail reloads the packed dword before replacing
                        // its split count lanes.
                        mapCell->m_extraInfo =
                            ((count >> 4) & 0xfff)
                            | ((count & 0xf) << 27)
                            | (mapCell->m_extraInfo & 0x87fff000);
                    }
                    break;
                }

                case MYSTICAL_GARDEN: {
                    ExtraInfoUnion* info = static_cast<ExtraInfoUnion*>(
                        static_cast<void*>(&mapCell->m_extraInfo));
                    info->fillGarden(random(0, 1) ? GOLD : GEMS);
                    break;
                }

                case REFUGEE_CAMP: {
                    TCreatureType creature = g_game->getRandomMonster(0, 6);
                    mapCell->m_objectIndex = creature;
                    mapCell->m_extraInfo =
                        g_creatureTypeTraits[creature].m_growthRate;
                    break;
                }

                case WATER_WHEEL: {
                    ExtraInfoUnion* info = static_cast<ExtraInfoUnion*>(
                        static_cast<void*>(&mapCell->m_extraInfo));
                    info->setWheelGold(1000);
                    break;
                }

                case WINDMILL: {
                    ExtraInfoUnion* info = static_cast<ExtraInfoUnion*>(
                        static_cast<void*>(&mapCell->m_extraInfo));
                    int resQty = random(3, 6);
                    EGameResource resType;
                    {
                        resType = EGameResource(random(1, 5));
                    }
                    info->setWindmill(resType, resQty);
                    break;
                }

                case FOUNTAIN_OF_FORTUNE: {
                    luckBonus = random(0, 3);
                    if (luckBonus == 0)
                        mapCell->m_extraInfo =
                            mapCell->m_extraInfo | 0x1e000;
                    else
                        mapCell->m_extraInfo =
                            (mapCell->m_extraInfo & 0xfffe1fff)
                            | ((luckBonus & 0xf) << 13);
                    break;
                }
                }

                if (obscuringHero)
                    obscuringHero->obscureCell();
                }
                ++y;
            } while (y < g_mapHeight);
        }
    }

    for (i = 0; i < HERO_COUNT; ++i) {
        currHero = &m_heroes[i];
        if (currHero->m_flags & g_heroWeeklyVisitFlag)
            currHero->m_flags -= g_heroWeeklyVisitFlag;
    }

    setupNewRumour();
    giveTroopsToNeutralTowns();

    setSummoningGenerators();
}

VA(0x004c8f40, 0x378)  // dc 0xb47b8
void game::perMonth()
{
    const int numcreaturemonthcreatures = 14;
    int growth;
    int x;
    int y;
    int i;
    NewmapCell* tempCell;
    int z;
    int j;
    town* currTown;

    ++m_month;
    int monthRoll = random(1, g_monthRollMax);
    if (g_weekType == g_weekTypeInfernoGrail) {
        g_monthTypeExtra = g_monthEffectCreature;
        g_monthType = g_creatureImpId;
    } else if (monthRoll > g_monthNormalRollMax && !m_isTutorial) {
        if (monthRoll <= g_monthCreatureRollMax) {
            g_monthTypeExtra = g_monthEffectCreature;
            g_monthType = g_monType[random(0, g_monthCreatureTableLast)];
        } else {
            g_monthTypeExtra = g_monthEffectPlague;
        }
    } else {
        g_monthTypeExtra = g_monthEffectNormal;
        g_monthType = random(0, g_monthCreatureRollMax);
    }

    for (i = 0; i < m_towns.size(); ++i) {
        for (j = 0; j <= numcreaturemonthcreatures; ++j) {
            currTown = getTown(i);
            growth = currTown->getGrowthRate(j);
            if (growth > 0) {
                if (g_monthTypeExtra == g_monthEffectCreature
                    && g_weekType != g_weekTypeInfernoGrail
                    && g_townDwellingCreatures[
                        currTown->m_type * TOWN_DWELLING_SLOTS + j]
                       == g_monthType) {
                    currTown->m_population[j] *= 2;
                }

                if (g_monthTypeExtra == g_monthEffectPlague) {
                    growth = currTown->getGrowthRate(j);
                    currTown->m_population[j] -= growth;
                    if (currTown->m_population[j] < 0)
                        currTown->m_population[j] = 0;
                    currTown->m_population[j] >>= 1;
                }
            }
        }
    }

    if (g_monthTypeExtra == g_monthEffectCreature) {
        for (z = 0; z < m_worldMap.getNumLevels(); ++z) {
            for (y = 0; y < g_mapWidth; ++y) {
                for (x = 0; x < g_mapHeight; ++x) {
                    tempCell = m_worldMap.cell(x, y, z);
                    if (!tempCell->m_isTrigger
                        && tempCell->m_passable
                        && tempCell->m_groundSet != eTerrainWater
                        && tempCell->m_groundSet != eTerrainRock
                        && tempCell->m_type != EVENT
                        && random(1, g_monthMonsterSpawnRollMax) == 1) {
                        insertObject(x, y, z, RANDOM_MONSTER,
                                     g_monthType, 0);
                        tempCell->m_monsterInfo.m_qty = 2 * getRandomNumTroops(g_monthType);
                        tempCell->m_monsterInfo.m_disposition =
                            random(1, g_monthMonsterDispositionMax);
                    }
                }
            }
        }
        setupAdjacentMons();
    }

    setMarketArtifacts();
    g_advManager->completeDraw(0);
}

// E:\gamedcs\game.cpp:8707
// Roll a creature id whose level falls in [minLevel, maxLevel], out of a
// bitset that starts all-ones and is punched down by the expansion and
// campaign filters.

// The width is 145, and retail proves it twice: bitset<145>::_Tidy writes
// five words and trims the last with 0x1ffff == (1<<17)-1, which is
// 145 % 32 == 17, and the traits sweep runs to 0x41b4 == 145 * 0x74 ==
// 145 * sizeof(TCreatureTypeTraits). CREATURE_CATAPULT is exactly 145:
// the traits table holds 150 rows but the last five are war machines, so
// the roll domain is every id BELOW the first of them.

// f_1f698 == 0 is the no-expansion map - everything from CREATURE_PIXIE
// up is struck out. On an expansion map only the six Armageddon's Blade
// neutral specials go, plus, in a Shadow of Death campaign, the ten
// Conflux-exclusive creatures.

// Every strike is spelled `monsterOk[id] = false;` uniformly. Retail shows
// three different shapes for it - a bare call to bitset::set, an inlined
// operator[] with an out-of-line reference::operator=, and both out of line -
// because /Ob2's budget decays across one source spelling.

// The final scan has NO bound and no `count() == 0` guard: on an empty
// bitset retail rolls Random(0, -1) and walks off the end. Transcribed
// faithfully.

// Residual (92.6331%, unpinned): retail's 0x4d4ca0 dereference copies the
// iterator's two words into a bitset::reference. VC6 instead expands that
// helper here, retaining operator[]/set and shrinking the frame to 0x1c
// (retail 0x24). The old pin gave 92.2308%, not an exact reconstruction.
// std::fill with temporary/named iterator bounds gives 85.2781/85.2840% and
// does not recover the retail expansion. This Complete-only range has no
// direct statement counterpart in DC's older GetRandomMonster body.
VA(0x004c92c0, 0x202)  // anchor-global, dc 0xb4b58
TCreatureType game::getRandomMonster(int minLevel, int maxLevel)
{
    int i;
    int totalInClass;
    int curCount;
    int x;

    std::bitset<CREATURE_CATAPULT> monsterOk;
    monsterOk.set();

    if (!m_gameVersion) {
        bitset_iterator<CREATURE_CATAPULT> it;
        it = bitset_iterator<CREATURE_CATAPULT>(monsterOk, CREATURE_PIXIE);
        bitset_iterator<CREATURE_CATAPULT> end(monsterOk, CREATURE_CATAPULT);
        for (; it != end; ++it) {
            *it = false;
        }
    } else {
        monsterOk[CREATURE_AZURE_DRAGON] = false;
        monsterOk[CREATURE_CRYSTAL_DRAGON] = false;
        monsterOk[CREATURE_FAERIE_DRAGON] = false;
        monsterOk[CREATURE_RUST_DRAGON] = false;
        monsterOk[CREATURE_ENCHANTER] = false;
        monsterOk[CREATURE_SHARPSHOOTER] = false;
        if (g_inCampaign
            && m_campaign.m_currentCampaign >= g_firstShadowOfDeathCampaign) {
            monsterOk[CREATURE_PIXIE] = false;
            monsterOk[CREATURE_SPRITE] = false;
            monsterOk[CREATURE_PSYCHIC_ELEMENTAL] = false;
            monsterOk[CREATURE_MAGIC_ELEMENTAL] = false;
            monsterOk[CREATURE_ICE_ELEMENTAL] = false;
            monsterOk[CREATURE_MAGMA_ELEMENTAL] = false;
            monsterOk[CREATURE_STORM_ELEMENTAL] = false;
            monsterOk[CREATURE_ENERGY_ELEMENTAL] = false;
            monsterOk[CREATURE_FIREBIRD] = false;
            monsterOk[CREATURE_PHOENIX] = false;
        }
    }

    for (i = 0; i < CREATURE_CATAPULT; ++i) {
        if (g_creatureTypeTraits[i].m_level < minLevel
            || g_creatureTypeTraits[i].m_level > maxLevel)
            monsterOk[i] = false;
    }

    totalInClass = monsterOk.count();
    curCount = random(0, totalInClass - 1);
    x = 0;
    for (;;) {
        if (monsterOk[x]) {
            if (curCount == 0)
                break;
            --curCount;
        }
        ++x;
    }
    return TCreatureType(x);
}

VA(0x004c94d0, 0xCD)  // dc 0xb4c84
TArtifact game::getRandomArtifactId(int artifactClass)
{
    int unallocatedInClass;
    int totalInClass;
    int curCount;
    int x;
    int i;

    totalInClass = 0;
    unallocatedInClass = 0;
    for (i = 0; i < 144; ++i) {
        if (!g_artifactTraits[i].m_disabled
            && (g_artifactTraits[i].m_artifactClass & artifactClass)) {
            ++totalInClass;
            if (!m_artifactUsed[i])
                ++unallocatedInClass;
        }
    }

    curCount = 0;
    if (unallocatedInClass) {
        x = random(0, unallocatedInClass - 1);
        for (i = 0; i < 144; ++i) {
            if (!g_artifactTraits[i].m_disabled
                && (g_artifactTraits[i].m_artifactClass & artifactClass)
                && !m_artifactUsed[i]) {
                if (curCount == x)
                    break;
                ++curCount;
            }
        }
        m_artifactUsed[i] = 1;
        return TArtifact(i);
    } else {
        curCount = 0;
        for (i = 0; i < 144; ++i) {
            if (!g_artifactTraits[i].m_disabled
                && (g_artifactTraits[i].m_artifactClass & artifactClass)) {
                m_artifactUsed[i] = m_artifactDisabled[i];
                if (!m_artifactUsed[i])
                    ++curCount;
            }
        }
        if (curCount > 0)
            return getRandomArtifactId(artifactClass);
        return getRandomArtifactId(g_allRandomArtifactClasses);
    }
}

VA(0x004c95a0, 0x18E)  // dc 0xb4e04
SpellID game::getRandomSpell(const std::bitset<5> spellLevels)
{
    int availableCount = 0;
    int spell;
    for (spell = 0; spell < hero::NUM_SPELLS; ++spell) {
        if (spellLevels.test(g_spellTraits[spell].m_level - 1)
            && g_spellTraits[spell].m_school != const_invalid_school
            && !m_spellAllocInfo[spell]) {
            ++availableCount;
        }
    }

    int ordinal = 0;
    if (availableCount != 0) {
        int selected = random(0, availableCount - 1);
        for (spell = 0; spell < hero::NUM_SPELLS; ++spell) {
            if (spellLevels.test(g_spellTraits[spell].m_level - 1)
                && g_spellTraits[spell].m_school != const_invalid_school
                && !m_spellAllocInfo[spell]) {
                if (ordinal == selected)
                    break;
                ++ordinal;
            }
        }
        m_spellAllocInfo[spell] = 1;
        return spell;
    }

    ordinal = 0;
    for (spell = 0; spell < hero::NUM_SPELLS; ++spell) {
        if (spellLevels.test(g_spellTraits[spell].m_level - 1)
            && g_spellTraits[spell].m_school != const_invalid_school
            && !m_spellDisabledInfo[spell]) {
            m_spellAllocInfo[spell] = 0;
            ++ordinal;
        }
    }
    if (ordinal > 0)
        return getRandomSpell(spellLevels);
    return -1;
}

// Original: game::RandomizeHeroPool; game.cpp:8896, dc 0xb4fa0
// NewMap expands this loop in retail, including Complete's enlarged roster
// and separate last-Wisdom/last-magic-school tracking bytes.
void game::randomizeHeroPool()
{
    for (int heroIndex = 0; heroIndex < HERO_COUNT; ++heroIndex) {
        m_heroes[heroIndex].m_experience = random(0, 50) + 40;
        setRandomHeroArmies(heroIndex, 0, 0);
        int mobility = m_heroes[heroIndex].getMobility();
        m_heroes[heroIndex].m_movePoints = mobility;
        m_heroes[heroIndex].m_maxMovePoints = mobility;
        m_heroes[heroIndex].m_levelSeed =
            static_cast<unsigned char>(random(1, 255));
        m_heroes[heroIndex].m_lastWisdom = 0;
        m_heroes[heroIndex].m_lastMagicSchoolLevel = 0;
    }
}

VA(0x004c9730, 0x159)  // dc 0xb5094
void game::setRandomHeroArmies(int hero, int cheat, unsigned char minimal)
{
    armyGroup* currentArmy = &m_heroes[hero].m_army;
    const THeroTraits* traits = &g_heroTraits[hero];

    if (g_inCampaign
        && hero == g_campaignArmyOverrideHero
        && m_campaign.m_currentCampaign == g_campaignArmyOverrideCampaign
        && m_campaign.m_currentMap) {
        traits = &g_heroTraits[g_campaignArmyOverrideTraits];
    }

    long i;
    for (i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; ++i) {
        currentArmy->m_armies[i] = -1;
        currentArmy->m_numTroops[i] = 0;
    }

    currentArmy->m_armies[0] = traits->m_firstStack;
    currentArmy->m_numTroops[0] = random(traits->m_firstStackLow,
                                        traits->m_firstStackHigh);
    if (minimal) {
        currentArmy->m_numTroops[0] = 1;
        return;
    }

    i = 1;
    if (random(1, 100) <= 88 && traits->m_secondStack != -1) {
        if (traits->m_secondStack == CREATURE_BALLISTA) {
            type_artifact artifact;
            artifact.m_artifactId = ARTIFACT_BALLISTA;
            m_heroes[hero].giveArtifact(&artifact, 0, 0);
        } else if (traits->m_secondStack == CREATURE_FIRST_AID_TENT) {
            type_artifact artifact;
            artifact.m_artifactId = ARTIFACT_FIRST_AID_TENT;
            m_heroes[hero].giveArtifact(&artifact, 0, 0);
        } else {
            currentArmy->m_armies[i] = traits->m_secondStack;
            currentArmy->m_numTroops[i] = random(traits->m_secondStackLow,
                                                traits->m_secondStackHigh);
            ++i;
        }
    }

    if (random(1, 100) <= 25 && traits->m_thirdStack != -1) {
        currentArmy->m_armies[i] = traits->m_thirdStack;
        currentArmy->m_numTroops[i] = random(traits->m_thirdStackLow,
                                            traits->m_thirdStackHigh);
    }
}

VA(0x004c9890, 0xFD)  // dc 0xb52fc
void game::insertObject(int x, int y, int z, int objType, int objectIndex, int m_extraInfo)
{
    if (objType == RANDOM_MONSTER)
        objType = MONSTER;

    CObject object(static_cast<unsigned char>(x),
                   static_cast<unsigned char>(y),
                   static_cast<unsigned char>(z), 0, m_extraInfo);

    if (objType == TERRAIN_HOLE) {
        m_worldMap.setObjectType(
            &object, TERRAIN_HOLE, 0, m_worldMap.cell(x, y, z)->m_groundSet);
    } else {
        m_worldMap.setObjectType(
            &object, objType, objectIndex, -1);
    }

    m_worldMap.m_objects.push_back(object);
    m_worldMap.placeObject(m_worldMap.m_objects.size() - 1, 1);
}

// E:\gamedcs\game.cpp:9195
// The materializer ProcessRandomObjects hands each rolled cell. It CLONES
// the object's current CObjectType onto the back of objectTypes,
// overwrites the clone's .def name / type / extra for the concrete
// object, registers the new sprite, and re-points every map cell the
// object covers at the clone.

// The dispatch is a real jump table: five entries at 0x4c9d58 and a
// 94-byte `type - ARTIFACT` index table at 0x4c9d6c, both inside this
// function's own extent. RANDOM_MONSTER shares MONSTER's arm and
// RANDOM_TOWN shares TOWN's; every other value falls to the default.
// Arm order below is retail's physical order, i.e. the source order.

// The !is_trigger path reaches the tail with tempText UNINITIALIZED. That
// is retail: ProcessRandomObjects only ever calls this for trigger cells,
// so the arm is unreachable in practice. Transcribed, not repaired.

// All three tail loops RE-READ their bounds - objectType's height and
// width are reloaded with movsx at each increment, and
// newCell->m_objects.end() at the bottom of every iteration - and
// objectTypes.size() is recomputed at every match rather than hoisted.
// The sprite push_back goes through this->worldMap while
// CalculateCellExtra RELOADS gpGame; do not unify them.

// DC's twelve locals include short objectToConvert (sp+0x12), not int;
// the entry load and later comparison preserve that signed interpretation.
// Keep its recorded local identities, named tempSprite, and the canonical
// isCapitol/isCastle calls. The frame and all 44 block sizes match retail.
// Residual 99.9756%: the string assignment's inlined _Eos terminator encodes
// [eax+ecx] instead of [ecx+eax], NOT a NewfullMap::cell difference. Eight
// type/declaration/call models and five recorded-name models reproduce the
// same score; do not replace a proven helper or invent a local for that byte.
// DC lines 9245/9246 store the monster cell type before the local type.
// Restoring that order, explicit iterator sequencing and the town predicates'
// public-symbol-proven bool returns are all byte-flat; the latter also preserve
// every measured consumer. SH4 cannot settle an x86 SIB operand-order choice.
// A named m_objectTypes reference across push_back/back/size lowered the Mac
// match from 15.9672% to 15.1460%; the direct member uses were restored.
VA(0x004c9990, 0x43A)  // anchor-global, dc 0xb54f8
void game::convertObject(NewmapCell* tempCell)
{
    char tempText[100];

    short objectToConvert = tempCell->m_objectTypeIndex;
    CObject* object = &m_worldMap.m_objects[objectToConvert];

    m_worldMap.m_objectTypes.push_back(m_worldMap.m_objectTypes[object->m_typeIndex]);
    CObjectType* objectType = &m_worldMap.m_objectTypes.back();

    TAdventureObjectType type = NOTHING;
    if (tempCell->m_isTrigger) {
        type = tempCell->getMapObject();
        switch (type) {
        case RESOURCE:
            strcpy(tempText, g_resourceObjectDefs[tempCell->m_objectIndex]);
            break;
        case ARTIFACT:
            sprintf(tempText, g_artifactObjectDefFormat, tempCell->m_objectIndex);
            break;
        case MONSTER:
        case RANDOM_MONSTER:
            strcpy(tempText,
                   m_worldMap.findObjectType(MONSTER,
                                                  tempCell->m_objectIndex)
                       ->m_imageName.c_str());
            tempCell->m_type = MONSTER;
            type = MONSTER;
            break;
        case RANDOM_TOWN:
        case TOWN: {
            town* thisTown = g_game->getTown(tempCell->getMapExtraInfo());
            if (thisTown->isCapitol()) {
                strcpy(tempText, g_townCapitolObjectDefs[tempCell->m_objectIndex]);
            } else {
                if (thisTown->isCastle())
                    strcpy(tempText, g_townFortObjectDefs[tempCell->m_objectIndex]);
                else
                    strcpy(tempText, g_townVillageObjectDefs[tempCell->m_objectIndex]);
            }
            break;
        }
        }
    }

    int oldType = objectType->m_objectType;
    objectType->m_imageName = tempText;
    objectType->m_objectType = type;
    objectType->m_extra = tempCell->m_objectIndex;
    CSprite* tempSprite =
        ResourceManager::getSprite(objectType->m_imageName.c_str());
    m_worldMap.m_sprites.push_back(tempSprite);

    for (int vert = 0; vert < objectType->m_height; vert++) {
        if (object->m_y - vert < 0 || object->m_y - vert >= g_mapHeight)
            continue;
        for (int horiz = 0; horiz < objectType->m_width; horiz++) {
            if (object->m_x - horiz < 0 || object->m_x - horiz >= g_mapWidth)
                continue;
            NewmapCell* newCell = m_worldMap.cell(object->m_x - horiz,
                                                object->m_y - vert, object->m_z);
            for (NewmapCell::TObjectCell* thisObj = newCell->m_objects.begin();
                 thisObj != newCell->m_objects.end(); thisObj++) {
                if (thisObj->m_objectIndex == objectToConvert) {
                    object->m_typeIndex = static_cast<unsigned short>(
                        m_worldMap.m_objectTypes.size() - 1);
                    if (newCell->m_type == oldType && !newCell->m_isTrigger)
                        newCell->m_type = type;
                }
            }
            g_game->m_worldMap.calculateCellExtra(newCell, 0);
        }
    }
}

// The artifact arguments are artraits.txt class bits: 2 treasure,
// 4 minor, 8 major, 16 relic, and RANDOM_ARTIFACT's 14 is
// treasure|minor|major, i.e. every class except relics.
VA(0x004c9dd0, 0x270)  // dc 0xb5910
void game::processRandomObjects()
{
    int y, z, x;
    NewmapCell* tempCell;

    for (z = 0; z < m_worldMap.getNumLevels(); ++z) {
        for (y = 0; y < g_mapHeight; ++y) {
            for (x = 0; x < g_mapWidth; ++x) {
                tempCell = m_worldMap.cell(x, y, z);
                if (!tempCell->m_isTrigger)
                    continue;

                switch (tempCell->m_type) {
                case RANDOM_MONSTER:
                    tempCell->m_type = MONSTER;
                    tempCell->m_objectIndex =
                        static_cast<short>(getRandomMonster(0, 6));
                    convertObject(tempCell);
                    break;
                case RANDOM_MONSTER_1:
                    tempCell->m_type = MONSTER;
                    tempCell->m_objectIndex =
                        static_cast<short>(getRandomMonster(0, 0));
                    convertObject(tempCell);
                    break;
                case RANDOM_MONSTER_2:
                    tempCell->m_type = MONSTER;
                    tempCell->m_objectIndex =
                        static_cast<short>(getRandomMonster(1, 1));
                    convertObject(tempCell);
                    break;
                case RANDOM_MONSTER_3:
                    tempCell->m_type = MONSTER;
                    tempCell->m_objectIndex =
                        static_cast<short>(getRandomMonster(2, 2));
                    convertObject(tempCell);
                    break;
                case RANDOM_MONSTER_4:
                    tempCell->m_type = MONSTER;
                    tempCell->m_objectIndex =
                        static_cast<short>(getRandomMonster(3, 3));
                    convertObject(tempCell);
                    break;
                case RANDOM_MONSTER_5:
                    tempCell->m_type = MONSTER;
                    tempCell->m_objectIndex =
                        static_cast<short>(getRandomMonster(4, 4));
                    convertObject(tempCell);
                    break;
                case RANDOM_MONSTER_6:
                    tempCell->m_type = MONSTER;
                    tempCell->m_objectIndex =
                        static_cast<short>(getRandomMonster(5, 5));
                    convertObject(tempCell);
                    break;
                case RANDOM_MONSTER_7:
                    tempCell->m_type = MONSTER;
                    tempCell->m_objectIndex =
                        static_cast<short>(getRandomMonster(6, 6));
                    convertObject(tempCell);
                    break;
                case RANDOM_RESOURCE:
                    tempCell->m_type = RESOURCE;
                    tempCell->m_objectIndex = static_cast<short>(random(0, 6));
                    convertObject(tempCell);
                    break;
                case RANDOM_ARTIFACT:
                    tempCell->m_type = ARTIFACT;
                    tempCell->m_objectIndex =
                        static_cast<short>(getRandomArtifactId(14));
                    convertObject(tempCell);
                    break;
                case RANDOM_ARTIFACT_1:
                    tempCell->m_type = ARTIFACT;
                    tempCell->m_objectIndex =
                        static_cast<short>(getRandomArtifactId(2));
                    convertObject(tempCell);
                    break;
                case RANDOM_ARTIFACT_2:
                    tempCell->m_type = ARTIFACT;
                    tempCell->m_objectIndex =
                        static_cast<short>(getRandomArtifactId(4));
                    convertObject(tempCell);
                    break;
                case RANDOM_ARTIFACT_3:
                    tempCell->m_type = ARTIFACT;
                    tempCell->m_objectIndex =
                        static_cast<short>(getRandomArtifactId(8));
                    convertObject(tempCell);
                    break;
                case RANDOM_ARTIFACT_4:
                    tempCell->m_type = ARTIFACT;
                    tempCell->m_objectIndex =
                        static_cast<short>(getRandomArtifactId(16));
                    convertObject(tempCell);
                    break;
                }
            }
        }
    }
}

// E:\gamedcs\game.cpp:9455
// The town-search loop is game::GetTownId (0x4bb870) inlined: on a hit it
// falls into the caller's own `== -1` test with the index still live in
// esi, and on loop exhaustion VC6 constant-folds that test against the
// -1 return and jumps straight to the null pointer.
// Residual (98.6076%, register scheduling): flow-distance is zero and only
// eight register-visible slots differ inside the inlined coordinate compare.
// Retail creates the CastleLoc scratch in EAX one instruction earlier; this
// compiler rotates EAX/EDX around two independent loads.  why-reg's proposed
// named startingHeroIds element is not a legal writable local transform, and
// binding CastleLoc to a const reference regresses to 98.15% by perturbing the
// following y compare.  The direct argument spelling is the measured wall.
// The 2026-09-01 model pass likewise rejects both proposed names
// (startingHeroIds[i] and setup.alignment[i]) at compile time, while the retail
// structure remains 29/29 exact blocks; no legal B14 mutation remains.
// DC line 9464 passes GetTownId directly to GetTown and records thisTown.
// Restoring that canonical accessor and local name is byte-flat at 98.6076%;
// the remaining difference is inside GetTownId's coordinate comparison.
// Mac 0xe1490 retains playerData::isHuman here; VC6 expands the restored
// source call and leaves the Windows score unchanged.
VA(0x004ca040, 0x1F1)  // linkorder, dc 0xb5cdc
void game::createTownHeroes(int* startingHeroIds)
{
    // MAX 99.6203 is NOT reachable as written: it was measured with this
    // loop spelled `i != 8`, an unnamed domain compare that fails the
    // cleanliness floor (docs/vc6/behavior-catalog.md D24).
    for (int i = 0; i < 8; i++) {
        if (!m_mapHeader.m_playerSlotAttributes[i].m_generateHero)
            continue;

        town* thisTown = getTown(
            getTownId(m_mapHeader.m_playerSlotAttributes[i].m_castleLoc.m_x,
                      m_mapHeader.m_playerSlotAttributes[i].m_castleLoc.m_y,
                      m_mapHeader.m_playerSlotAttributes[i].m_castleLoc.m_z));

        int heroId;
        if (startingHeroIds != NULL && m_players[i].isHuman()
            && startingHeroIds[i] != -1)
            heroId = startingHeroIds[i];
        else if (g_inCampaign)
            heroId = getStartingHeroId(m_setup.m_alignment[i], i, 0);
        else
            heroId = getStartingHeroId(m_setup.m_alignment[i], i, 0);

        if (m_setup.m_startingHero[i] == -1)
            m_setup.m_startingHero[i] = heroId;
        m_heroAvailability[heroId] = static_cast<char>(i);
        thisTown->placeInMap(heroId, i, 1);
        thisTown->giveSpells(NULL);

        if (g_inCampaign
            && g_game->m_campaign.m_currentCampaign == g_startLevelCampaign
            && g_game->m_campaign.m_currentMap == g_startLevelScenario)
            m_heroes[heroId].giveExperience(
                hero::getExperience(g_game->m_heroes[g_startLevelHeroId].m_level
                                    + g_startLevelBonus),
                1, 0);
    }
}

VA(0x004ca240, 0xF6)  // dc 0xb5f80
void game::makeTerrainVisible(int whichPlayer, unsigned short visMask)
{
    unsigned char players = 0;
    if (whichPlayer >= 0 && whichPlayer < 8) {
        int team = m_mapHeader.m_teamInfo[whichPlayer];
        for (int player = 0; player < 8; ++player) {
            if (m_mapHeader.m_teamInfo[player] == team)
                players |= 1 << player;
        }
    }

    unsigned short playerMask = players;
    for (int z = 0; z < m_worldMap.getNumLevels(); ++z) {
        for (int x = 0; x < g_mapWidth; ++x) {
            for (int y = 0; y < g_mapHeight; ++y) {
                unsigned int mask = visMask;
                NewmapCell* cell = m_worldMap.cell(x, y, z);
                if (mask & (1 << cell->m_groundSet))
                    *getMapExtraPtr(x, y, z) |= playerMask;
            }
        }
    }
}

VA(0x004ca340, 0x6F)  // dc 0xb6054
void game::giveArmy(armyGroup* thisMonInfo, int monType, int monNum, int slot)
{
    if (slot >= 0) {
        thisMonInfo->m_armies[slot] = monType;
        thisMonInfo->m_numTroops[slot] = monNum;
        return;
    }
    for (int i = 0; i < 7; i++) {
        if (thisMonInfo->m_armies[i] == monType) {
            thisMonInfo->m_numTroops[i] += monNum;
            return;
        }
    }
    for (int j = 0; j < 7; j++) {
        if (thisMonInfo->m_armies[j] < 0) {
            thisMonInfo->m_armies[j] = monType;
            thisMonInfo->m_numTroops[j] = monNum;
            return;
        }
    }
}

VA(0x004ca3b0, 0x58)  // dc 0xb6114
int game::experienceValueOfStack(const armyGroup* whichGroup, const hero* whichHero)
{
    int value = 0;
    for (int i = 0; i < armyGroup::ARMY_GROUP_SLOT_COUNT; ++i) {
        if (whichGroup->m_numTroops[i] > 0)
            value += g_creatureTypeTraits[whichGroup->m_armies[i]].m_hitPoints
                     * whichGroup->m_numTroops[i];
    }
    if (whichHero)
        value += 500;
    return value;
}

VA(0x004ca410, 0x116)  // dc 0xb61d0
void game::setupAdjacentMons()
{
    type_point excluded(0xff, 0xff, 0xff);
    type_point monster;
    int x;
    int y;
    int z;
    unsigned short mask = ~MAP_EXTRA_MONSTER;

    for (z = 0; z < m_worldMap.getNumLevels(); ++z) {
        for (x = 0; x < g_mapWidth; ++x) {
            for (y = 0; y < g_mapHeight; ++y) {
                if (g_advManager->findAdjacentMonster(
                        type_point(x, y, z), &monster, excluded)) {
                    unsigned short* extraByte = getMapExtraPtr(x, y, z);
                    *extraByte |= MAP_EXTRA_MONSTER;
                } else {
                    unsigned short* extraByte = getMapExtraPtr(x, y, z);
                    *extraByte &= mask;
                }
            }
        }
    }
}

VA(0x004ca530, 0x80)  // dc 0xb62f8
void game::cancelComputerScreen()
{
    g_completeDrawEnabled = 1;
    g_advManager->updateRadar(1, 1, 0, 0, 0);
    g_advManager->m_advWindow->getWidget(8)->enable(1);
    g_advManager->m_advWindow->getWidget(7)->enable(1);
    g_advManager->m_advWindow->getWidget(6)->enable(1);
    g_advManager->m_advWindow->getWidget(12)->enable(1);
}

VA(0x004ca5b0, 0x1C9)
void game::showComputerScreen()
{
    g_advManager->m_advWindow->getWidget(8)->enable(0);
    g_advManager->m_advWindow->getWidget(7)->enable(0);
    g_advManager->m_advWindow->getWidget(6)->enable(0);
    g_advManager->m_advWindow->getWidget(12)->enable(0);

    if (g_config.m_blackoutComputer && !g_currentPlayer->isHuman()) {
        g_currentPlayer->m_isLocal = 1;
        g_completeDrawAllCells = 1;
        g_advManager->completeDraw(1);
        g_advManager->m_advWindow->updateHeroLocators(-1, 1, 0);
        g_advManager->m_advWindow->updateTownLocators(-1, 1, 0);
        g_advManager->m_advWindow->updateQuestLogButton(1);
        g_advManager->updBottomView(1, 1, 1);
        g_advManager->m_advWindow->updateResourceDisplay(1, 1);
        g_advManager->updateScreen(0, 1);
        g_completeDrawAllCells = 0;
        g_currentPlayer->m_isLocal = 0;
    } else {
        g_advManager->m_advWindow->updateHeroLocators(-1, 1, 0);
        g_advManager->m_advWindow->updateTownLocators(-1, 1, 0);
        g_advManager->m_advWindow->updateQuestLogButton(1);
        g_advManager->updBottomView(1, 1, 1);
        g_advManager->m_advWindow->updateResourceDisplay(1, 1);
        g_advManager->m_advWindow->drawWindow(
            1, WINDOW_ALL_WIDGETS_LOW, WINDOW_ALL_WIDGETS_HIGH);
        g_windowManager->updateScreen(
            0, 0, WINDOW_SCREEN_WIDTH, WINDOW_SCREEN_HEIGHT);
    }

    if (!g_currentPlayer->isHuman())
        showHeroesLogo();
}

VA(0x004ca780, 0xB4)
void game::showHeroesLogo()
{
    CNetMsgHandler* netMsgHandler;
    Bitmap816* heroLogo;
    int w;
    int h;
    int x;
    int y;

    if (g_remoteOn && g_dPlay) {
        netMsgHandler = g_dPlay->getNetMsgHandler();
        if (netMsgHandler && netMsgHandler->isInPopup())
            return;
    }

    if (g_advManager->m_heroLogoShowing)
        return;

    g_advManager->m_heroLogoShowing = 1;
    heroLogo = ResourceManager::getBitmap816(
        DATA_COMPGEN(0x00677eb8, heroesLogoBitmapName, "aishield.pcx"));
    x = g_advManager->m_advWindow->m_radarWidget->m_x;
    y = g_advManager->m_advWindow->m_radarWidget->m_y;
    w = g_advManager->m_advWindow->m_radarWidget->m_width;
    h = g_advManager->m_advWindow->m_radarWidget->m_height;
    heroLogo->draw(0, 0, w, h, g_windowManager->m_screenBitmap, x, y, false);
    g_windowManager->updateScreen(x, y, w, h);
    heroLogo->dispose();
}

VA(0x004ca840, 0x19C)  // dc 0xb6640
void game::waitForPlayer(char* text, int playerId)
{
    if (!g_blackoutPlayer || g_numHumanPlayers <= 1 || g_remoteOn)
        return;

    g_mouseManager->setPointer(0, mouseManager::DEFAULT_SET);
    g_completeDrawAllCells = 1;
    if (g_currentPlayer->m_isHuman && g_currentPlayer->m_isLocal)
        g_advManager->overrideBottomView(advManager::BOTTOM_VIEW_1, 9999999);
    else
        g_advManager->overrideBottomView(advManager::BOTTOM_VIEW_DEFAULT, 9999999);

    g_soundManager->m_playSounds = 1;
    g_soundManager->stopMP3();
    SAMPLE2 sample2 = loadPlaySample(
        DATA_COMPGEN(0x00677ec8, newWeekSample, "NewWeek.wav"));

    g_advManager->completeDraw(1);
    g_advManager->m_advWindow->updateHeroLocators(0, 1, 0);
    g_advManager->m_advWindow->updateTownLocators(0, 1, 0);
    g_advManager->m_advWindow->updateQuestLogButton(1);
    g_advManager->m_advWindow->updateButtons(1, 0);
    g_advManager->m_advWindow->m_resourceDisplay->clear();
    showHeroesLogo();
    g_windowManager->updateScreen(0, 0, 800, 600);

    g_completeDrawAllCells = 0;
    normalDialog(text, 1, -1, -1, 10, playerId,
                 -1, 0, -1, 0, -1, 0);
    waitEndSample(sample2, -1);

    g_advManager->m_advWindow->updateHeroLocators(0, 1, 0);
    g_advManager->m_advWindow->updateTownLocators(0, 1, 0);
    g_advManager->m_advWindow->updateQuestLogButton(1);
}
// Original: game::SetupTowns; game.cpp:9798, dc 0xb6878
// The DC release body is empty; its 6 bytes only home this and return.
void game::setupTowns()
{
}

// The nine-faction no-repeat town-name samplers are file-static in game.cpp.
// Retail's vector-constructor iterator at 0x4ca9e0 proves nine 24-byte
// TPickRandomTownName objects and its element wrapper proves [0, 15].
DATA(0x006971a0)
// Previous project spelling: gRandomTownNames.
static TPickRandomTownName g_randomTownNames[9];

// Retail indexes this table with a 16-pointer faction stride (`shl 4`),
// agreeing with initializeTownNameText's sixteen filled slots per faction.
DATA(0x006a6048)
const char* g_townNames[9][16];

// E:\gamedcs\game.cpp:9803, dc 0xb6944.
inline const char* getRandomTownName(int townType)
{
    int name = g_randomTownNames[townType].pick();
    while (name == -1) {
        townType = random(0, 8);
        name = g_randomTownNames[townType].pick();
    }
    return g_townNames[townType][name];
}

// E:\gamedcs\game.cpp:9821, dc 0xb69b8
inline void resetRandomTownNames()
{
    for (int i = 0; i < 9; ++i)
        g_randomTownNames[i].reset();
}

// Original: game::CheckHeroConsistency; game.cpp:10132, dc 0xb7554
// The DC release body only homes this and returns; no gameplay operation
// is present to duplicate at its callers.
void game::checkHeroConsistency()
{
}

// E:\gamedcs\game.cpp:9833
// Complete's town-name lookup has a 16-pointer faction stride, matching
// initializeTownNameText and the Mac initializer at 0:0x1adff8. Correcting
// that declaration makes this Windows body exact (57/57 CFG, 18/18 calls).
// The Mac body is independently bounded at 0:0xe2024..0xe21e0; its pair
// proposal stays in build/mac/pairing until the game declaration view grows.
VA(0x004caa70, 0x39C)  // DC name/order + retail map/vector/string shape, dc 0xb69f4
void game::processOnMapTowns()
{
    int numMapLayers;
    town* currTown;
    int x;
    int y;
    NewmapCell* tempCell;
    int z;
    TownExtra* townExtra;
    int townnum;
    int owner;

    resetRandomTownNames();

    m_towns.resize(m_scenarioTowns.size());

    numMapLayers = 1;
    if (g_game->m_mapHeader.m_hasTwoLayers)
        numMapLayers = 2;

    for (z = 0; z < numMapLayers; ++z) {
        for (y = 0; y < g_mapHeight; ++y) {
            for (x = 0; x < g_mapWidth; ++x) {
                tempCell = m_worldMap.cell(x, y, z);
                if ((tempCell->m_type == TOWN
                     || tempCell->m_type == RANDOM_TOWN)
                    && tempCell->m_isTrigger) {
                    townnum = tempCell->m_extraInfo;
                    townExtra = &m_scenarioTowns[townnum];
                    currTown = &m_towns[townnum];

                    currTown->m_mapX = x;
                    currTown->m_mapY = y;
                    currTown->m_mapZ = z;
                    currTown->m_id = townnum;

                    if (tempCell->m_type == RANDOM_TOWN) {
                        tempCell->m_type = TOWN;
                        tempCell->m_objectIndex = townExtra->m_townType;
                        convertObject(tempCell);
                    }

                    if (townExtra->m_customName)
                        currTown->m_name = townExtra->m_name;
                    else
                        currTown->m_name =
                            getRandomTownName(townExtra->m_townType);

                    currTown->initialize(townExtra);
                    convertObject(tempCell);
                }
            }
        }
    }
}

VA(0x004cae10, 0x1B1)
void game::processOnMapHeroes()
{
    HeroExtra* heroExtra;
    hero* currHero;
    int i;
    NewmapCell* townCell;
    type_point townLoc;

    for (i = 0; i < HERO_COUNT; ++i) {
        heroExtra = &m_heroSetup[i];
        if (heroExtra->m_location.m_x >= 0) {
            townLoc = heroExtra->m_location;
            --townLoc.m_x;
            townCell = m_worldMap.cell(townLoc.m_x, townLoc.m_y, townLoc.m_z);
            if (townCell->m_type == TOWN && townCell->m_isTrigger
                && m_heroAvailability[heroExtra->m_id]
                    != hero::HERO_AVAILABILITY_PRISON) {
                --heroExtra->m_location.m_x;
            }

            currHero = getHero(heroExtra->m_id);
            currHero->initialize(heroExtra);

            if (m_heroAvailability[heroExtra->m_id]
                != hero::HERO_AVAILABILITY_PRISON) {
                m_players[currHero->m_owner].m_heroes[
                    m_players[currHero->m_owner].m_numHeroes] = currHero->m_id;
                ++m_players[currHero->m_owner].m_numHeroes;
                currHero->obscureCell();
                setVisibility(currHero->m_x, currHero->m_y, currHero->m_z,
                              currHero->m_owner, currHero->getVisibility(), 1);
            }
        } else {
            currHero = getHero(heroExtra->m_id);
            currHero->initialize(heroExtra);
        }
    }
}

// Retail's EH metadata, live cross-chunk control flow, four-argument ABI and
// SaveGame/diff/transmit/resend fingerprint prove this complete span.
// DC 0xb7560 supplies the signature, named locals, scopes and statement order.
// Its line 10382 tests done before the confirmation loop; line 10391 combines
// the null-message and timeout predicates. Keep CMessageKill alive through
// abort returns, and keep the end-message temporary scoped before resends.

// Retail corrections: +0xa6 tests status 1 (active); +0x21c/+0x247 call
// fileError on compression failure, as DC lines 10215/10222 also do. Only
// the initial removal calls File::deleteFile. At +0x39f, idiv uses the quotient
// totalBlocks as dividend and fileSize as divisor. DC line 10289 also passes
// those operands to __modls: preserve totalBlocks % fileSize despite its
// unusual behavior; fileSize % payloadSize is a different algorithm.

// MAX 85.0958 (2026-09-07), source hash 9fbd6fc3c561: the retail fixes,
// while loop and done initialized before the player bitmap, verified through
// normalized production objects. DC lines 10279..10286 initialize bytesLeft,
// current, done, numMsgs and curBlock earlier, before the transfer UI; retain
// that full order through the current dip. A lower score does not reject it.
// Scratch controls: top-tested loop 84.0000%; combined timeout byte-flat;
// remainder plus earlier done 84.4738%; complete DC initializer order 83.6804%.
// Production normalization scores differ from those raw-object probes.

// Complete uses bool useGuaranteed: DC's unsigned char introduces a test/setne
// conversion before the send loop that retail lacks (81.4869 control).
// Array new in CreateMsg and removing the artificial send-only scope are
// byte-flat. The three player scans use g_game->getLocalPlayerGamePos(), but
// access this->m_players; substituting global player-array reads was worse.
// The first destroy-player path reloads its dpid across opaque calls; the
// broadcast loop has a named killDPID, as DC also records. Preserve both.

// Residual: the DC-order candidate's frame is 0x3b0 versus retail's 0x3a4.
// Retail keeps isDiff/newSize in BL/EBX through disjoint phases and bytesLeft
// in EBX through transmission; the candidate keeps the packet pointer there
// and spills these values. Moving the fileSize declaration alone and swapping
// isDiff/diffSize declarations were byte-flat in earlier controls. Missing DC
// queueSize/attempts/pNetMsg have no independent retail semantics proven yet.
VA(0x004cafd0, 0xD14)  // retail body + typed catch + continuation/tables
int game::transmitSaveGame(int toWho, int thisPlayerDead,
                           unsigned char inGame, unsigned char makeOrig)
{
    CNetMsgHandlerPause netMsgHandlerPause;
    g_advManager->trimLoopingSounds(4);

    CGameTransmitMainMsg* gameTransmitMainMsg =
        CGameTransmitMainMsg::createMsg(GAME_TRANSMIT_PAYLOAD_SIZE);
    unsigned char isDiff = 0;
    unsigned long diffSize = 0;
    int changeSounds = g_soundManager->m_playSounds;
    g_soundManager->m_playSounds = 1;
    g_soundManager->switchAmbientMusic(-1);
    g_soundManager->m_playSounds = changeSounds;

    if (g_advManager->m_status == baseManager::STATUS_ACTIVE)
        g_advManager->bvMessage(g_generalText->getText(GENERAL_TEXT_SENDING_GAME));

    saveGame(g_config.m_scFile, 0, 0, !inGame, 1);

    char fileName[351];
    sprintf(fileName,
            DATA_COMPGEN(0x00660358, processSearchFoundFormat, "%s%s"),
            DATA_COMPGEN(0x00677d88, dataDirectoryPrefix, ".\\DATA\\"),
            g_config.m_scFile);

    if (inGame) {
        File file;
        if (file.open(DATA_COMPGEN(0x00677fb0, xferOriginalFilename,
                                  "data\\orig.dat"), modeRead)) {
            unsigned long oldSize = file.getLength();
            unsigned char* old = new unsigned char[oldSize];
            file.read(old, oldSize);
            file.close();

            file.open(fileName, modeRead);
            unsigned long newSize = file.getLength();
            unsigned char* newValue = new unsigned char[newSize];
            file.read(newValue, newSize);
            file.close();

            CDiffMaker diffMaker(old, oldSize, newValue, newSize);
            CDiffFile* diff = diffMaker.makeDiff(diffSize);
            delete[] old;
            delete[] newValue;

            const char* diffFilename = DATA_COMPGEN(
                0x00677fa0, xferDiffFilename, "data\\diff.dat");
            File::deleteFile(diffFilename);
            int returnValue;
            try {
                TGzFile compressedFile(diffFilename,
                              DATA_COMPGEN(0x00677f9c,
                                           xferDiffWriteMode, "wb6"));
                returnValue = compressedFile.write(diff, diffSize);
            } catch (TGzFile::TOpenFailure) {
                fileError(diffFilename);
                shutDown(0);
            }
            if (returnValue != static_cast<int>(diffSize)) {
                fileError(diffFilename);
                shutDown(0);
            }
            delete diff;
            strcpy(fileName, diffFilename);
            isDiff = 1;
        }
    }

    if (makeOrig)
        saveGame(DATA_COMPGEN(0x00660410, remoteOriginalSaveName,
                             "orig.dat"), 0, 0, 0, 1);

    int fileSize = ::fileSize(fileName);
    int handle = _open(fileName, _O_BINARY);
    if (handle == -1) {
        fileError(fileName);
        return 0;
    }

    unsigned char* data = new unsigned char[fileSize];
    _read(handle, data, fileSize);
    _close(handle);
    int fullGameCRC = calcCrcLong(data, fileSize);

    CGameTransmitInitMsg msg(fileSize, fullGameCRC, thisPlayerDead,
                             isDiff, makeOrig);
    if (!transmitRemoteData(&msg, toWho, false, true))
        shutDown(0);

    int totalBlocks = fileSize / GAME_TRANSMIT_PAYLOAD_SIZE;
    int bytesLeft = fileSize;
    unsigned char* current = data;
    unsigned char done = 0;
    unsigned long numMsgs = 0;
    int curBlock = 0;
    g_playerDrop = 0;
    if (totalBlocks % fileSize)
        ++totalBlocks;

    CGameTransferSmack smack;
    CGameTransferDlg dlg(1);
    CGameTransferSmack* transferSmack;
    if (inGame) {
        smack.setup(622, 403, 1, 1);
        smack.saveScreen();
        transferSmack = &smack;
    } else {
        dlg.setup(DATA_COMPGEN(0x00691210,
                              adventureRolloverEmptyText, ""),
                  g_mediumFont);
        dlg.open(0, 1);
        transferSmack = &dlg.m_smack;
    }

    bool useGuaranteed = false;
    if (g_lobbyLaunched || g_mpNetProtocol == MP_TCP)
    {
        g_logFile.log(DATA_COMPGEN(0x00677f88, xferGuaranteedLog,
                                "Using guaranteed!!"));
        useGuaranteed = true;
    }

    transferSmack->start();
    while (bytesLeft > 0) {
        pollSound();
        checkDoMain(0, 1);

        if (bytesLeft >= GAME_TRANSMIT_PAYLOAD_SIZE)
            gameTransmitMainMsg->m_blockSize = GAME_TRANSMIT_PAYLOAD_SIZE;
        else
            gameTransmitMainMsg->m_blockSize = bytesLeft;
        transferSmack->setPercentage(static_cast<float>(curBlock)
                              / static_cast<float>(totalBlocks));

        gameTransmitMainMsg->m_blockNbr = curBlock;
        gameTransmitMainMsg->update(current,
                                     gameTransmitMainMsg->m_blockSize);
        transmitRemoteData(gameTransmitMainMsg, toWho,
                           false, useGuaranteed);

        current += gameTransmitMainMsg->m_blockSize;
        bytesLeft -= gameTransmitMainMsg->m_blockSize;
        ++curBlock;
    }

    g_logFile.log(DATA_COMPGEN(
        0x00677f54, xferFinishedLog,
        "Finished sending data... Now handling requests.."));
    {
        CGameTransmitEndMsg end(g_monthType, g_monthTypeExtra,
                                 g_weekType, g_weekTypeExtra, diffSize);
        transmitRemoteData(&end, toWho, false, true);
    }

    unsigned char playerDone[8];
    memset(playerDone, 0, sizeof(playerDone));
    unsigned long dataTimeOutStart = GameTime::get();
    int retryCount = 0;

    while (!done) {
        pollSound();
        checkDoMain(0, 1);
        CNetMsg* confirmMsg = getRemoteData(1, 0);
        CMessageKill kill(confirmMsg);

        if (!confirmMsg && GameTime::elapsedSince(dataTimeOutStart)
                > GAME_TRANSMIT_TIMEOUT) {
            ++retryCount;
            g_logFile.log(DATA_COMPGEN(
                            0x00677f34, xferTimeoutLog,
                            "Timeout sending save game [%d]"),
                        retryCount);
            if (retryCount > 1) {
                normalDialog(g_generalText->getText(GENERAL_TEXT_DIRECTPLAY_SEND_RETRY_PROMPT), 2, -1, -1,
                             -1, 0, -1, 0, -1, 0, -1, 0);
                if (g_windowManager->m_dialogReturn
                        != DIALOG_RETURN_ACCEPT) {
                    if (inGame && toWho != NET_MESSAGE_RECIPIENT_ALL) {
                        g_dPlay->destroyPlayer(m_players[toWho].m_dpid);
                        handlePlayerDrop(m_players[toWho].m_dpid);
                        CDestroyPlayerMsg destroyMsg(
                            m_players[toWho].m_dpid);
                        m_players[toWho].clearNetInfo();
                        g_playerDrop = 1;
                        transmitRemoteDataDPID(&destroyMsg, 0,
                                               false, true);
                        return 0;
                    } else {
                        for (int i = 0; i < 8; ++i) {
                            if (m_players[i].isHuman() && !playerDone[i]
                                    && i != g_game->getLocalPlayerGamePos()) {
                                unsigned long killDPID =
                                    m_players[i].m_dpid;
                                g_dPlay->destroyPlayer(killDPID);
                                handlePlayerDrop(killDPID);
                                CDestroyPlayerMsg destroyMsg(killDPID);
                                m_players[i].clearNetInfo();
                                transmitRemoteDataDPID(&destroyMsg, 0,
                                                       false, true);
                            }
                        }
                        g_playerDrop = 1;
                        delete[] data;
                        return 0;
                    }
                }
            }

            dataTimeOutStart = GameTime::get();
            for (int i = 0; i < 8; ++i) {
                if (m_players[i].isHuman() && !playerDone[i]
                        && i != g_game->getLocalPlayerGamePos()) {
                    CGameTransmitEndMsg resendEnd(
                        g_monthType, g_monthTypeExtra,
                        g_weekType, g_weekTypeExtra, diffSize);
                    transmitRemoteData(&resendEnd, i, false, true);
                }
            }
        } else if (confirmMsg) {

            ++numMsgs;
            dataTimeOutStart = GameTime::get();
            switch (confirmMsg->m_subType) {
            case RS_GAME_TRANSMIT_REQ: {
                CGameTransmitReqMsg* receivedMsg =
                    static_cast<CGameTransmitReqMsg*>(confirmMsg);
                int blockOffset = receivedMsg->m_blockNbr
                    * GAME_TRANSMIT_PAYLOAD_SIZE;
                int resendSize = fileSize - blockOffset;
                if (resendSize >= GAME_TRANSMIT_PAYLOAD_SIZE)
                    resendSize = GAME_TRANSMIT_PAYLOAD_SIZE;

                gameTransmitMainMsg->m_blockNbr = receivedMsg->m_blockNbr;
                gameTransmitMainMsg->update(data + blockOffset, resendSize);
                g_logFile.log(DATA_COMPGEN(
                                0x00677f08, xferResendLog,
                                "Transmitting resend %d size %d (offset=%d)"),
                            receivedMsg->m_blockNbr, resendSize, blockOffset);
                transmitRemoteData(gameTransmitMainMsg, toWho,
                                   false, true);
                break;
            }

            case RS_GAME_TRANSMIT_ACK:
                dataTimeOutStart = GameTime::get();
                break;

            case RS_PLAYER_DROPPED:
                if (getGamePosFromDPID(confirmMsg->m_dpidFrom) == toWho) {
                    handlePlayerDrop(confirmMsg->m_dpidFrom);
                    g_playerDrop = 1;
                    delete[] data;
                    return 0;
                }
                handlePlayerDrop(confirmMsg->m_dpidFrom);
                // A dropped broadcast peer owes no end confirmation;
                // share the confirmation tail exactly as the retail switch does.

            case RS_GAME_XFER_CONFIRM_END:
                g_logFile.log(DATA_COMPGEN(
                                0x00677ed4, xferConfirmLog,
                                "Received game transmit end verification from [%d]"),
                            confirmMsg->m_dpidFrom);
                transferSmack->setPercentage(1.0f);
                done = 1;
                if (toWho == NET_MESSAGE_RECIPIENT_ALL) {
                    playerDone[confirmMsg->m_from] = 1;
                    for (int i = 0; i < 8; ++i) {
                        if (m_players[i].isHuman() && !playerDone[i]
                                && i != g_game->getLocalPlayerGamePos()) {
                            done = 0;
                            break;
                        }
                    }
                }
                break;

            case RS_CHAT_MSG: {
                CChatMsg* receivedMsg = static_cast<CChatMsg*>(confirmMsg);
                receiveChat(receivedMsg->m_text, receivedMsg->m_from);
                if (inGame) {
                    g_advManager->completeDraw(0);
                    g_advManager->updateScreen(0, 0);
                }
                break;
            }
            }
        }
    }

    transferSmack->stop();
    if (inGame)
        transferSmack->restoreScreen();
    else
        dlg.close(1);
    delete[] data;
    return 1;
}

VA(0x004cbd40, 0xA83)  // retail body + dc 0xb85c4 source shape
int game::receiveSaveGame(int fileSize, int fullGameCRC, int fromWho,
                          unsigned char inGame, unsigned char isDiff)
{
    CNetMsgHandlerPause netMsgHandlerPause;
    g_advManager->trimLoopingSounds(4);

    if (g_advManager->m_status == baseManager::STATUS_ACTIVE)
        g_advManager->bvMessage(g_generalText->getText(GENERAL_TEXT_RECEIVING_GAME));

    int lastDataReceiveTime = GameTime::get();
    int changeSounds = g_soundManager->m_currentTerrainMusic;
    char soundWasEnabled = g_soundManager->m_playSounds;
    g_soundManager->m_playSounds = 1;
    g_soundManager->switchAmbientMusic(-1);
    g_soundManager->m_playSounds = soundWasEnabled;

    unsigned char* data = new unsigned char[fileSize];
    CNetMsg* netMsg = 0;
    unsigned char done = 0;
    int totalBlocks = fileSize / GAME_TRANSMIT_PAYLOAD_SIZE;
    if (fileSize % GAME_TRANSMIT_PAYLOAD_SIZE)
        ++totalBlocks;

    g_logFile.log(DATA_COMPGEN(
                    0x006780ac, xferReceivingSaveLog,
                    "Receiving save game from [%d]"),
                fromWho);

    unsigned char waitingForRetransmit = 0;
    unsigned char* blockReceived = new unsigned char[totalBlocks];
    memset(blockReceived, 0, totalBlocks);

    CGameTransferSmack smack;
    CGameTransferDlg dlg(0);
    unsigned long diffSize = 0;
    CGameTransferSmack* transferSmack;

    if (fromWho >= 8 || fromWho < 0)
        fromWho = 0;

    unsigned long fromDPID = g_game->m_players[fromWho].m_dpid;

    if (inGame) {
        smack.setup(622, 403, 0, 1);
        smack.saveScreen();
        transferSmack = &smack;
    } else {
        dlg.setup(DATA_COMPGEN(0x00691210,
                              adventureRolloverEmptyText, ""),
                  g_mediumFont);
        dlg.open(0, 1);
        transferSmack = &dlg.m_smack;
    }
    transferSmack->start();

    while (!done) {
        pollSound();
        checkDoMain(0, 1);

        if (GameTime::elapsedSince(lastDataReceiveTime)
                > GAME_TRANSMIT_TIMEOUT) {
            normalDialog(g_generalText->getText(GENERAL_TEXT_NETWORK_RECEIVE_RETRY_PROMPT), 2, -1, -1,
                         -1, 0, -1, 0, -1, 0, -1, 0);
            if (g_windowManager->m_dialogReturn == DIALOG_RETURN_ACCEPT) {
                lastDataReceiveTime = GameTime::get();
                for (int i = 0; i < totalBlocks; ++i) {
                    if (!blockReceived[i]) {
                        g_logFile.log(DATA_COMPGEN(
                                        0x0067808c,
                                        xferRequestResendBlockLog,
                                        "Requesting resend block [%d]"),
                                    i);
                        CGameTransmitReqMsg msg(i);
                        transmitRemoteDataDPID(&msg, fromDPID, false, true);
                        waitingForRetransmit = 1;
                    }
                }
            } else {
                if (inGame) {
                    remoteCleanup();
                    normalDialog(g_generalText->getText(GENERAL_TEXT_REMOTE_SESSION_DESTROYED), 1, -1, -1,
                                 -1, 0, -1, 0, -1, 0, -1, 0);
                    shutDown(0);
                } else {
                    unsigned long killDPID = m_players[fromWho].m_dpid;
                    g_dPlay->destroyPlayer(m_players[fromWho].m_dpid);
                    g_dPlay->handlePlayerDrop(m_players[fromWho].m_dpid);
                    CDestroyPlayerMsg msg(killDPID);
                    transmitRemoteDataDPID(&msg, 0, false, true);
                }
                delete[] data;
                delete[] blockReceived;
                return 0;
            }
        }

        if (netMsg) {
            destroyMsg(netMsg);
            netMsg = 0;
        }
        netMsg = getRemoteData(1, 0);

        if (netMsg) {
            lastDataReceiveTime = GameTime::get();
            switch (netMsg->m_subType) {
            case RS_GAME_TRANSMIT_MAIN: {
                CGameTransmitMainMsg* receivedMsg =
                    static_cast<CGameTransmitMainMsg*>(netMsg);
                memcpy(data + receivedMsg->m_blockNbr
                                * GAME_TRANSMIT_PAYLOAD_SIZE,
                       receivedMsg->getData(), receivedMsg->m_blockSize);

                if (!waitingForRetransmit) {
                    transferSmack->setPercentage(
                        static_cast<float>(receivedMsg->m_blockNbr)
                        / static_cast<float>(totalBlocks));
                    if (!(receivedMsg->m_blockNbr % 30)) {
                        CGameXferAckMsg msg;
                        transmitRemoteDataDPID(
                            &msg, netMsg->m_dpidFrom, false, false);
                    }
                }

                blockReceived[receivedMsg->m_blockNbr] = 1;
                if (waitingForRetransmit) {
                    g_logFile.log(DATA_COMPGEN(
                                    0x00678070,
                                    xferReceivedResendBlockLog,
                                    "Received resend block [%d]"),
                                receivedMsg->m_blockNbr);
                    done = 1;
                    for (int i = 0; i < totalBlocks; ++i) {
                        if (!blockReceived[i]) {
                            g_logFile.log(DATA_COMPGEN(
                                            0x0067801c,
                                            xferStillWaitingBlockLog,
                                            "Still waiting for block #%d"),
                                        i);
                            done = 0;
                            break;
                        }
                    }

                    if (done) {
                        g_logFile.log(DATA_COMPGEN(
                                        0x00678038,
                                        xferAllResendsDoneLog,
                                        "All resends are done.  Sending "
                                        "GameTransmitEndMsg to %d"),
                                    netMsg->m_from);
                        waitingForRetransmit = 0;
                        CGameTransmitConfirmEndMsg msg;
                        transmitRemoteDataDPID(
                            &msg, netMsg->m_dpidFrom, false, true);
                    } else {
                        g_logFile.log(DATA_COMPGEN(
                            0x00678000, xferStillWaitingResendsLog,
                            "Still waiting for resend(s)"));
                    }
                }
                break;
            }

            case RS_GAME_TRANSMIT_END: {
                CGameTransmitEndMsg* receivedMsg =
                    static_cast<CGameTransmitEndMsg*>(netMsg);
                g_logFile.log(DATA_COMPGEN(
                    0x00677fe4, xferReceivedEndLog,
                    "Received game transmit end."));
                transferSmack->setPercentage(1.0f);

                for (int i = 0; i < totalBlocks; ++i) {
                    if (!blockReceived[i]) {
                        g_logFile.log(DATA_COMPGEN(
                                        0x0067808c,
                                        xferRequestResendBlockLog,
                                        "Requesting resend block [%d]"),
                                    i);
                        CGameTransmitReqMsg msg(i);
                        transmitRemoteDataDPID(
                            &msg, netMsg->m_dpidFrom, false, true);
                        waitingForRetransmit = 1;
                    }
                }

                g_monthType = receivedMsg->m_monthType;
                g_monthTypeExtra = receivedMsg->m_monthTypeExtra;
                g_weekType = receivedMsg->m_weekType;
                g_weekTypeExtra = receivedMsg->m_weekTypeExtra;
                diffSize = receivedMsg->m_diffSize;

                if (!waitingForRetransmit) {
                    g_logFile.log(DATA_COMPGEN(
                        0x00677fc0, xferSendingDoneLog,
                        "Sending confirmation we are done!"));
                    done = 1;
                    CGameTransmitConfirmEndMsg msg;
                    transmitRemoteDataDPID(
                        &msg, netMsg->m_dpidFrom, false, true);
                }
                break;
            }

            case RS_PLAYER_DROPPED:
                if (getGamePosFromDPID(netMsg->m_dpidFrom) == fromWho) {
                    remoteCleanup();
                    normalDialog(g_generalText->getText(GENERAL_TEXT_PLAYER_LEFT_DURING_TRANSMISSION), 1, -1, -1,
                                 -1, 0, -1, 0, -1, 0, -1, 0);
                    return 0;
                }
                handlePlayerDrop(netMsg->m_dpidFrom);
                break;

            case RS_SET_AS_HOST:
                g_chatMan.systemMsg(g_generalText->getText(GENERAL_TEXT_LOCAL_PLAYER_IS_HOST));
                break;

            case RS_CHAT_MSG: {
                CChatMsg* receivedMsg = static_cast<CChatMsg*>(netMsg);
                receiveChat(receivedMsg->m_text, receivedMsg->m_from);
                if (inGame) {
                    g_advManager->completeDraw(0);
                    g_advManager->updateScreen(0, 0);
                }
                break;
            }
            }

            destroyMsg(netMsg);
            netMsg = 0;
        }
    }

    transferSmack->stop();
    if (!inGame)
        dlg.close(1);

    if (isDiff) {
        File file;
        const char* diffFilename = DATA_COMPGEN(
            0x00677fa0, xferDiffFilename, "data\\diff.dat");
        File::deleteFile(diffFilename);
        if (!file.open(diffFilename, modeWrite)) {
            fileError(diffFilename);
            shutDown(0);
        }
        if (!file.write(data, fileSize)) {
            file.close();
            fileError(diffFilename);
            shutDown(0);
        }
        file.close();
        delete[] data;

        unsigned char* newSave = new unsigned char[diffSize];
        unsigned long bytesRead;
        try {
            TGzFile gzfile(
                diffFilename,
                DATA_COMPGEN(0x00677d6c, gzReadMode, "rb"));
            bytesRead = gzfile.read(newSave, diffSize);
        } catch (TGzFile::TOpenFailure) {
            fileError(diffFilename);
            shutDown(0);
        }

        if (bytesRead != diffSize) {
            fileError(diffFilename);
            shutDown(0);
        }

        const char* origFilename = DATA_COMPGEN(
            0x00677fb0, xferOriginalFilename, "data\\orig.dat");
        if (!file.open(origFilename, modeRead)) {
            fileError(origFilename);
            shutDown(0);
        }
        unsigned long size = file.getLength();
        if (!size) {
            fileError(origFilename);
            shutDown(0);
        }
        unsigned char* orig = new unsigned char[size];
        if (!file.read(orig, size)) {
            file.close();
            fileError(origFilename);
            shutDown(0);
        }
        file.close();

        CDiffFile* diffFile =
            static_cast<CDiffFile*>(static_cast<void*>(newSave));
        unsigned char* temp =
            static_cast<unsigned char*>(diffFile->apply(orig, size));
        fileSize = diffFile->m_numBytes;
        delete[] orig;
        delete diffFile;
        data = temp;
    }

    char fileName[351];
    sprintf(fileName,
            DATA_COMPGEN(0x00660358, processSearchFoundFormat, "%s%s"),
            DATA_COMPGEN(0x00677d88, dataDirectoryPrefix, ".\\DATA\\"),
            g_config.m_scFile);
    int handle = _open(fileName,
                       _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY,
                       _S_IWRITE);
    if (handle == -1)
        fileError(fileName);
    _write(handle, data, fileSize);
    _close(handle);

    delete[] blockReceived;
    delete[] data;

    if (g_advManager->m_status == baseManager::STATUS_ACTIVE) {
        g_advManager->overrideBottomView(
            advManager::BOTTOM_VIEW_DEFAULT, -1);
        g_advManager->updBottomView(1, 1, 1);
    }

    if (changeSounds != -1) {
        char restoreSoundWasEnabled = g_soundManager->m_playSounds;
        g_soundManager->m_playSounds = 1;
        g_soundManager->switchAmbientMusic(changeSounds);
        g_soundManager->m_playSounds = restoreSoundWasEnabled;
    }

    if (inGame && m_playerDisabled[g_netLocalGamePos])
        nextPlayer();

    return 1;
}

VA(0x004cc7d0, 0x5FE)  // dc 0xb9408
void game::doNewTurn()
{
    int newHero;
    char sample[13];
    char temp[50];

    if (!g_currentPlayer->m_isHuman) {
        checkForTimeEvent();
        checkForTownEvent();
        return;
    }
    if (!g_currentPlayer->m_isLocal)
        return;

    m_mapHeader.m_lossCondition.checkForTimeLimitExpired();
    m_mapHeader.m_victoryCondition.checkForTotalResources();
    m_mapHeader.m_victoryCondition.checkForTimeSurvival();
    checkEndGame(0);
    checkForTimeEvent();
    checkForTownEvent();

    if (g_currentPlayer->m_isHuman && g_currentPlayer->m_isLocal)
        g_soundManager->m_playSounds = 1;
    g_advManager->m_advWindow->updateResourceDisplay(1, 1);
    g_advManager->setInitialMapOrigin();
    g_advManager->redrawAdvScreen(0, 0);
    g_advManager->overrideBottomView(advManager::BOTTOM_VIEW_1, -1);
    g_advManager->updBottomView(1, 1, 0);
    g_windowManager->updateScreen(0, 0, 800, 600);

    if (g_currentPlayer->m_deathCountDown >= 0) {
        if (g_currentPlayer->m_deathCountDown == 1) {
            sprintf(g_text, g_newTurn[1],
                    g_currentPlayer->getName());
        } else {
            sprintf(g_text, g_newTurn[0],
                    g_currentPlayer->getName(),
                    g_currentPlayer->m_deathCountDown);
        }

        if (g_currentPlayer->m_isHuman && g_currentPlayer->m_isLocal) {
            normalDialog(g_text, 1, -1, -1, 10, g_netLocalGamePos,
                         -1, 0, -1, 0, -1, 0);
        }
    }

    g_advManager->deactivateCurrHero(0);
    newHero = g_currentPlayer->nextHero();
    if (newHero >= 0) {
        g_advManager->setHeroContext(newHero, 0, 0, 1);
    } else if (g_currentPlayer->m_numTowns > 0) {
        g_advManager->setTownContext(g_currentPlayer->m_townIds[0], 0, 1);
    }
    g_advManager->checkDimNextHeroBut();

    if (m_day != 1
        || (m_month == 1 && m_week == 1)) {
        g_soundManager->m_playSounds = 1;
        return;
    }
    if (g_weekType == -1)
        return;

    if (m_week == 1)
        strcpy(sample, DATA_COMPGEN(0x006780d8, newMonthTurnSample,
                                    "newmonth.wav"));
    else
        strcpy(sample, DATA_COMPGEN(0x006780cc, newWeekTurnSample,
                                    "newweek.wav"));

    if (m_week == 1 && g_weekType == g_weekTypeNormal) {
        if (g_monthTypeExtra == g_monthEffectNormal) {
            sprintf(g_text, g_newTurn[2], g_monthNames[g_monthType]);
        } else if (g_monthTypeExtra == g_monthEffectCreature) {
            strcpy(temp, getArmyName(g_monthType, 1));
            temp[0] = toupper(temp[0]);
            sprintf(g_text, g_newTurn[3],
                    getArmyName(g_monthType, 1), temp);
        } else {
            strcpy(g_text, g_newTurn[4]);
        }
    } else {
        switch (g_weekType) {
        case g_weekTypeNormal:
            sprintf(g_text, g_newTurn[5], g_weekNames[g_weekTypeExtra]);
            break;

        case g_weekTypeCreature:
            strcpy(temp, getArmyName(g_weekTypeExtra, 1));
            sprintf(g_text, g_newTurn[6], temp, temp);
            break;

        case g_weekTypeInfernoGrail:
            sprintf(g_text, g_newTurn[7],
                    g_creatureTypeTraits[g_creatureImpId].m_name,
                    g_creatureTypeTraits[g_creatureImpId].m_name,
                    g_creatureTypeTraits[g_creatureImpId].m_growthRate,
                    g_creatureTypeTraits[g_creatureFamiliarId].m_name,
                    g_creatureTypeTraits[g_creatureImpId].m_growthRate);
            break;
        }
    }

    g_soundManager->m_playSounds = 1;
    launchSample(sample, 30000, 3);
    g_mouseManager->setPointer(0, mouseManager::DEFAULT_SET);
    g_advManager->m_advWindow->setBackgroundAnimation(1);
    normalDialog(g_text, 1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    g_advManager->m_advWindow->setBackgroundAnimation(0);
}

VA(0x004ccdd0, 0x56)  // dc 0xb99d0
int game::getBoatsBuilt()
{
    int count = 0;
    for (unsigned int i = 0; i < m_boats.size(); i++) {
        if (m_boats[i].m_allocated)
            count++;
    }
    return count;
}

VA(0x004cce30, 0xB8)  // dc 0xb9a34
int game::getNumThievesGuilds(int whichPlayer)
{
    int count = 0;
    for (int i = 0; i < m_players[whichPlayer].m_numTowns; i++) {
        town* currentTown =
            &g_game->m_towns[m_players[whichPlayer].m_townIds[i]];
        if ((currentTown->m_built & g_bitNumber[TAVERN_ID]) ||
            (currentTown->m_type == TOWN_CASTLE &&
             (currentTown->m_built & g_bitNumber[EXTRA_1_ID]))) {
            count++;
        }
    }
    return count;
}

VA(0x004ccef0, 0x23)  // dc 0xb9b24
void game::setMapSize(int width, int height)
{
    g_mapWidth = width;
    g_mapHeight = height;
    g_searchArray->close();
}

// Original: game::HeroIDToHeroPos; game.cpp:11221, dc 0xb9b54
int game::heroIdToHeroPos(playerData* player, int id)
{
    for (int i = 0; i < player->m_numHeroes; ++i) {
        if (player->m_heroes[i] == id)
            return i;
    }
    return -1;
}

// Original: game::TownIDToTownPos; game.cpp:11231, dc 0xb9b98
int game::townIdToTownPos(playerData* player, int id)
{
    for (int i = 0; i < player->m_numTowns; ++i) {
        if (player->m_townIds[i] == id)
            return i;
    }
    return -1;
}

// Original: game::SetMarketArtifacts; game.cpp:11241, dc 0xb9c04
void game::setMarketArtifacts()
{
    m_marketArtifacts[0] = getRandomArtifactId(2);
    m_marketArtifacts[1] = getRandomArtifactId(2);
    m_marketArtifacts[2] = getRandomArtifactId(2);
    m_marketArtifacts[3] = getRandomArtifactId(4);
    m_marketArtifacts[4] = getRandomArtifactId(4);
    m_marketArtifacts[5] = getRandomArtifactId(4);
    m_marketArtifacts[6] = getRandomArtifactId(8);
}

// Original: game::SetSummoningGenerators; game.cpp:11252, dc 0xb9cac
void game::setSummoningGenerators()
{
    for (int i = 0; i < 8; ++i) {
        if (!m_playerDisabled[i]) {
            playerData* player = &m_players[i];
            for (int j = 0; j < player->m_numTowns; ++j) {
                town* currentTown = getTown(player->m_townIds[j]);
                if (currentTown->m_type == TOWN_DUNGEON
                    && currentTown->hasBuilding(EXTRA_1_ID, false))
                    currentTown->setSummoningGenerator();
            }
        }
    }
}

// Original: game::SetupNewRumour; game.cpp:11445, dc 0xbac04
void game::setupNewRumour()
{
    int rumourType = random(1, 100);
    if (rumourType < g_rumourMapThreshold)
        setCannedRumour();
    else if (rumourType < g_rumourSpecialThreshold)
        setMapRumour();
    else
        setSpecialRumour();
}

VA(0x004ccf20, 0x8E)  // dc 0xb9d58
void game::setCannedRumour()
{
    int available = 0;
    int i;
    for (i = 0; i < 256; i++) {
        if (!m_rumourState[i])
            available++;
    }

    if (!available) {
        int rumourSlot;
        MEMSET(m_rumourState, 0, sizeof(m_rumourState), rumourSlot);
        available = 256;
    }

    int selected = random(1, available);
    int seen = 0;
    for (i = 0; i < 256; i++) {
        if (!m_rumourState[i])
            seen++;
        if (seen == selected) {
            m_rumourState[i] = 1;
            strcpy(m_currentRumour, g_cannedRumours[selected]);
            return;
        }
    }
}

VA(0x004ccfb0, 0x1BD)
void game::setMapRumour()
{
    int rumourIndex;
    int x;
    int numValidRumours = 0;

    if (m_rumours.size() == 0) {
        setCannedRumour();
        return;
    }

    for (x = 0; x < m_rumours.size(); ++x) {
        if (!m_rumours[x].m_unavailable)
            ++numValidRumours;
    }

    if (!numValidRumours) {
        for (x = 0; x < m_rumours.size(); ++x)
            m_rumours[x].m_unavailable = 0;
        numValidRumours = m_rumours.size();
    }

    rumourIndex = random(1, numValidRumours);
    numValidRumours = 0;
    for (x = 0; x < m_rumours.size(); ++x) {
        if (!m_rumours[x].m_unavailable)
            ++numValidRumours;
        if (numValidRumours == rumourIndex) {
            m_rumours[x].m_unavailable = 1;
            strcpy(m_currentRumour, m_rumours[x].m_text.c_str());
            return;
        }
    }
}

VA(0x004cd170, 0x59B)  // dc 0xba040
void game::setSpecialRumour()
{
    if (random(1, 100) < g_specialRumourChance && getCurrentTurn() > 1) {
        long values[8];
        int attempt;
        signed char rankedPlayers[8];

        attempt = 0;
        while (attempt++ < g_specialRumourAttempts) {
            int category = random(g_specialRumourFirstCategory,
                                  g_specialRumourLastCategory);
            getCategoryStats(category, values, rankedPlayers);
            sortStats(values, rankedPlayers);

            if (values[0] != values[1]) {
                if (category == g_specialRumourFirstCategory) {
                    sprintf(m_currentRumour,
                            g_generalText->getText(
                                g_specialRumourCategoryText),
                            getPlayerName(rankedPlayers[0]));
                } else if (category
                           == g_specialRumourFirstCategory + 1) {
                    sprintf(m_currentRumour,
                            g_generalText->getText(
                                g_specialRumourCategoryText + 1),
                            getPlayerName(rankedPlayers[0]));
                    return;
                } else if (category
                           == g_specialRumourFirstCategory + 2) {
                    sprintf(m_currentRumour,
                            g_generalText->getText(
                                g_specialRumourCategoryText + 2),
                            getPlayerName(rankedPlayers[0]));
                } else {
                    sprintf(m_currentRumour,
                            g_generalText->getText(
                                g_specialRumourCategoryText + 3),
                            getPlayerName(rankedPlayers[0]));
                }
                return;
            }
        }
    }

    if (!m_ultimateArtifactPresent) {
        setCannedRumour();
        return;
    }

    if (random(1, 100) <= g_specialRumourLocationChance) {
        int direction;

        if (static_cast<double>(m_ultimateArtifactX)
                    < static_cast<double>(m_mapHeader.m_size) * 0.33
            && static_cast<double>(m_ultimateArtifactX)
                   < static_cast<double>(m_mapHeader.m_size) * 0.33)
            direction = 7;
        else if (static_cast<double>(m_ultimateArtifactX)
                         < static_cast<double>(m_mapHeader.m_size) * 0.33
                     && static_cast<double>(m_ultimateArtifactX)
                            > static_cast<double>(m_mapHeader.m_size) * 0.66)
            direction = 5;
        else if (static_cast<double>(m_ultimateArtifactX)
                 < static_cast<double>(m_mapHeader.m_size) * 0.33)
            direction = 6;
        else if (static_cast<double>(m_ultimateArtifactX)
                         > static_cast<double>(m_mapHeader.m_size) * 0.66
                     && static_cast<double>(m_ultimateArtifactX)
                            < static_cast<double>(m_mapHeader.m_size) * 0.33)
            direction = 1;
        else if (static_cast<double>(m_ultimateArtifactX)
                         > static_cast<double>(m_mapHeader.m_size) * 0.66
                     && static_cast<double>(m_ultimateArtifactX)
                            > static_cast<double>(m_mapHeader.m_size) * 0.66)
            direction = 3;
        else if (static_cast<double>(m_ultimateArtifactX)
                 > static_cast<double>(m_mapHeader.m_size) * 0.66)
            direction = 2;
        else if (static_cast<double>(m_ultimateArtifactX)
                 < static_cast<double>(m_mapHeader.m_size) * 0.33)
            direction = 0;
        else if (static_cast<double>(m_ultimateArtifactX)
                 > static_cast<double>(m_mapHeader.m_size) * 0.66)
            direction = 4;
        else
            direction = 8;

        if (!m_ultimateArtifactZ) {
            sprintf(m_currentRumour,
                    g_generalText->getText(
                        g_specialRumourGrailAboveText),
                    g_directions[direction]);
        } else {
            sprintf(m_currentRumour,
                    g_generalText->getText(
                        g_specialRumourGrailBelowText),
                    g_directions[direction]);
        }
    } else {
        type_point artifactLocation(m_ultimateArtifactX, m_ultimateArtifactY,
                                    m_ultimateArtifactZ);
        const NewmapCell* cell = g_advManager->getCell(artifactLocation);
        sprintf(m_currentRumour,
                g_generalText->getText(g_specialRumourGrailObjectText),
                g_rumourTerrainDescriptions[cell->m_groundSet]);
    }
}

VA(0x004cd710, 0x200)
void game::giveTimeEventReward(const TTimedEvent* thisEvent)
{
    std::vector<type_dialog_resource> rewards;
    type_dialog_resource reward;
    int j;
    int resToShow;
    int i;

    for (j = 0; j < NUM_RESOURCES; ++j) {
        resToShow = thisEvent->m_resQty[j];
        if (-resToShow
            > g_game->m_players[g_netLocalGamePos].m_resources[j]) {
            resToShow =
                -g_game->m_players[g_netLocalGamePos].m_resources[j];
        }

        g_game->m_players[g_netLocalGamePos].m_resources[j]
            += thisEvent->m_resQty[j];
        if (g_game->m_players[g_netLocalGamePos].m_resources[j] < 0)
            g_game->m_players[g_netLocalGamePos].m_resources[j] = 0;

        if (j <= GOLD && resToShow < 0)
            resToShow -= 100000;
        if (resToShow) {
            reward.m_resource = j;
            reward.m_qualifier = resToShow;
            rewards.push_back(reward);
        }
    }

    if (g_currentPlayer->isLocalHuman()) {
        g_advManager->m_advWindow->updateResourceDisplay(1, 1);
        extendedDialog(thisEvent->m_message.c_str(), rewards, -1, -1, 0);

        if (g_inCampaign) {
            short currentTurn = getCurrentTurn();
            if (currentTurn == g_campaignPopulationEventDay
                && m_campaign.m_currentCampaign
                    == g_campaignPopulationEventCampaign
                && m_campaign.m_currentMap
                    == g_campaignPopulationEventScenario) {
                for (i = 0; i < g_currentPlayer->m_numTowns; ++i) {
                    town* currentTown = getTown(g_currentPlayer->m_townIds[i]);
                    for (j = 0; j < 4; ++j) {
                        currentTown->m_population[j]
                            = currentTown->m_population[j] / 2;
                        currentTown->m_population[j + 7]
                            = currentTown->m_population[j + 7] / 2;
                    }
                }
            }
        }
    }
}

// Original: game::GiveTownEventReward; game.cpp:11499, dc 0xbaeb0
// Both date arms of Complete checkForTownEvent 0x4cda10 expand this operation:
// select the town, require its owner to be local, then grant the two rewards.
void game::giveTownEventReward(const TTownEvent& thisEvent)
{
    town* thisTown = getTown(thisEvent.m_townNum);
    if (g_netLocalGamePos == thisTown->m_owner) {
        giveTimeEventReward(&thisEvent);
        thisTown->giveEventReward(&thisEvent);
    }
}

VA(0x004cd910, 0xF5)  // unique body/order + 0x34-byte TTimedEvent stride
void game::checkForTimeEvent()
{
    int day = static_cast<short>(
        (m_month * 4 + m_week - 5) * 7 + m_day);

    for (unsigned int i = 0; i < m_worldMap.m_timedEventList.size(); ++i) {
        TTimedEvent* thisEvent = &m_worldMap.m_timedEventList[i];
        int playerIndex = g_netLocalGamePos;
        if (playerIndex >= 8 || playerIndex < 0)
            playerIndex = 0;
        if (!(m_players[playerIndex].m_isHuman
                  ? thisEvent->m_applyToHuman
                  : thisEvent->m_applyToComputer)) {
            continue;
        }
        if (!(g_curPlayerBit & thisEvent->m_playerFlags))
            continue;

        if (thisEvent->m_firstTime == day) {
            giveTimeEventReward(thisEvent);
        } else if (thisEvent->m_interval && day > thisEvent->m_firstTime
                   && (day - thisEvent->m_firstTime) % thisEvent->m_interval
                       == 0) {
            giveTimeEventReward(thisEvent);
        }
    }
}

VA(0x004cda10, 0x164)  // dc 0xbafec
void game::checkForTownEvent()
{
    int day = static_cast<short>(
        (m_month * 4 + m_week - 5) * 7 + m_day);

    for (unsigned int i = 0; i < m_worldMap.m_townEventList.size(); ++i) {
        const TTownEvent& thisEvent = m_worldMap.m_townEventList[i];
        int playerIndex = g_netLocalGamePos;
        if (playerIndex >= 8 || playerIndex < 0)
            playerIndex = 0;
        if (!(m_players[playerIndex].m_isHuman
                  ? thisEvent.m_applyToHuman
                  : thisEvent.m_applyToComputer)) {
            continue;
        }
        if (!(g_curPlayerBit & thisEvent.m_playerFlags))
            continue;

        if (thisEvent.m_firstTime == day) {
            giveTownEventReward(thisEvent);
        } else if (thisEvent.m_interval && day > thisEvent.m_firstTime
                   && (day - thisEvent.m_firstTime) % thisEvent.m_interval == 0) {
            giveTownEventReward(thisEvent);
        }
    }
}

VA(0x004cdb80, 0x231)  // dc 0xbb0e4
unsigned char game::getRandomLith(const std::vector<type_point>& points,
                                    type_point& result, long cellType,
                                    long excluded) const
{
    long lithCount = points.size();
    long openCount = 0;
    type_point exitPoint;
    long i;
    const NewmapCell* exitCell;

    for (i = 0; i < lithCount; ++i) {
        exitPoint = points[i];
        exitCell = m_worldMap.cell(
            exitPoint.m_x, exitPoint.m_y, exitPoint.m_z);
        if (exitCell->m_type == HERO && exitCell->m_isTrigger
            && !onSameTeam(
                m_heroes[exitCell->m_extraInfo].m_owner, g_netLocalGamePos)) {
            ++openCount;
        } else if (exitCell->m_type == cellType
                   && exitCell->m_extraInfo != excluded
                   && exitCell->m_isTrigger) {
            ++openCount;
        }
    }

    if (openCount == 0)
        return 0;

    openCount = random(1, openCount);
    for (i = 0; i < lithCount; ++i) {
        exitPoint = points[i];
        exitCell = m_worldMap.cell(
            exitPoint.m_x, exitPoint.m_y, exitPoint.m_z);
        if (exitCell->m_type == HERO && exitCell->m_isTrigger
            && !onSameTeam(
                m_heroes[exitCell->m_extraInfo].m_owner, g_netLocalGamePos)) {
            if (--openCount == 0) {
                result = exitPoint;
                return 1;
            }
        } else if (exitCell->m_type == cellType
                   && exitCell->m_extraInfo != excluded
                   && exitCell->m_isTrigger) {
            if (--openCount == 0) {
                result = exitPoint;
                return 1;
            }
        }
    }
    return 0;
}

VA(0x004cddc0, 0x22)  // dc 0xbb3e0
unsigned char game::getRandomLithExit(long color, type_point& result) const
{
    return getRandomLith(m_lithExitPools[color], result, 0x2c, -1);
}

VA(0x004cddf0, 0x24)  // dc 0xbb41c
unsigned char game::getRandomLith(long color, long excluded, type_point& result) const
{
    return getRandomLith(m_lithPools[color], result, 0x2d, excluded);
}

VA(0x004cde20, 0x1D)  // dc 0xbb45c
unsigned char game::getRandomWhirlpool(long excluded, type_point& result) const
{
    return getRandomLith(m_whirlpools, result, 0x6f, excluded);
}

VA(0x004cde40, 0xE0)  // dc 0xbb490
type_point game::getUndergroundGateExit(const NewmapCell* cell) const
{
    long exitGate = m_undergroundGatePairs[cell->m_extraInfo];
    if (exitGate < 0)
        return type_point(0xff, 0xff, 0xff);

    type_point exitPoint = m_undergroundGateExits[exitGate];
    const NewmapCell* exitCell = m_worldMap.cell(exitPoint.m_x, exitPoint.m_y, exitPoint.m_z);
    if (exitCell->m_type == HERO && exitCell->m_isTrigger)
        return exitPoint;
    if (exitCell->m_type != UNDERGROUND_GATE)
        return type_point(0xff, 0xff, 0xff);
    return exitPoint;
}

// E:\gamedcs\game.cpp:11684
// The mirror of ~game below: retail's body is almost entirely the
// compiler-generated MEMBER CONSTRUCTION, in declaration order, and this
// class's declaration order is already right - an EMPTY body scores
// 74.6490% on its own. What lands verbatim from the header alone:
// scenarioTowns' inline vector constructor, the `eh vector constructor
// iterator' over heroSetup[156] with HeroExtra's 0x334 stride and the
// 0x4ce4b0/0x4ce520 ctor/dtor pair, SCampaign, the load-event vector,
// SGameSetupOptions' whole loop body (difficulty 0, turnDuration 10,
// memset(filename,0,251) at +0x39 and memset(path,0,100) at +0x134, the
// three bytes at +0x1a0..+0x1a2), NewSMapHeader with its three strings,
// NewfullMap, players[8], towns, heroes[156] on the 0x492 stride, the
// five object pools, rumours and both eight-element teleport-pool
// arrays.

// Residual (87.8496%, 2026-08-21): the explicit initialization is now
// complete. The remaining structural delta is the compiler-generated
// construction of heroPoolMap[156]. Retail emits a 156-trip loop calling
// bitset<8>::_Tidy(0); our CL expands `_Tidy` and folds the loop to one
// `rep stosd`, leaving us one branch and one call short.

// A constructor-scoped `#pragma inline_depth(1)` proves that exact boundary:
// it raises this body to 91.0059 and makes the 2-branch/1-ret CFG exact. It
// cannot be retained because the same scope compiles the address-taken
// HeroExtra constructor with its two type_artifact constructors out of line,
// dropping that exact row to 64.4444. Restoring depth at the opening brace
// preserves HeroExtra but is byte-flat here, and a game-TU forced-inline
// on type_artifact cannot override the depth cap. A layout-identical derived
// heroPoolMap element is also byte-flat here and regresses game::Load from
// 92.3721 to 92.2795. The implicit-member boundary is therefore bounded
// without sacrificing an exact function.
// The scalar array initialisations use MEMSET markers, which expand to counted
// loops rather than CRT memset calls. Retail
// sets EDI up before ECX at m_saveFileName, m_heroAvailability,
// m_artifactUsed, m_artifactDisabled, m_obeliskFlags, m_currentRumour,
// m_globalInfoFlags and m_rumourState, and ECX before EDI at m_setup: the
// loop form and the memset form emit the same rep stosd (plus the stosw /
// stosb tail for the odd bytes, and EAX = -1 for the -1 fill), and only that
// setup order separates them (behavior-catalog D25). m_heroPoolMap keeps
// ECX-first because its fill value is a variable, not a literal.
// Adopting the eight loops moved this body 78.16 -> 80.60 and reset MAX from
// the 88.86 banked before the canonical SCampaign header bodies were
// restored; HIST holds that peak. The rest of the residual is that header
// decision, not these fills: retail CALLS SCampaign::SCampaign (which itself
// expands its string and four vector members) where our TU inlines its body
// and keeps the member ctor calls, and retail opens one more EH state (11).
VA(0x004cdf20, 0x585)  // anchor-global, dc 0xbb62c
game::game()
{
    m_difficultyRating = 0;
    m_newCampaignStarted = 0;
    int nameByte;
    MEMSET(m_saveFileName, 0, sizeof(m_saveFileName), nameByte);
    memset(&m_setup, 0, sizeof(m_setup));
    memset(m_playerDisabled, 0, sizeof(m_playerDisabled));
    m_day = 0;
    m_week = 0;
    m_month = 0;
    int heroSlot;
    MEMSET(m_heroAvailability, -1, sizeof(m_heroAvailability), heroSlot);

    std::bitset<8> allPlayers;
    allPlayers.set();
    for (int i = 0; i < HERO_COUNT; i++)
        m_heroPoolMap[i] = allPlayers;
    int usedArt;
    MEMSET(m_artifactUsed, 0, sizeof(m_artifactUsed), usedArt);
    int disabledArt;
    MEMSET(m_artifactDisabled, 0, sizeof(m_artifactDisabled), disabledArt);
    int obelisk;
    MEMSET(m_obeliskFlags, 0, sizeof(m_obeliskFlags), obelisk);
    m_ultimateArtifactX = -1;
    m_ultimateArtifactY = -1;
    m_ultimateArtifactZ = -1;
    m_ultimateRadius = 0x7f;
    m_ultimateArtifactPresent = 0;
    m_gameVersion = 0;
    m_isCheater = 0;
    int rumourByte;
    MEMSET(m_currentRumour, 0, sizeof(m_currentRumour), rumourByte);
    m_numObelisks = 0;
    int infoFlag;
    MEMSET(m_globalInfoFlags, 0, sizeof(m_globalInfoFlags), infoFlag);
    memset(m_borderTentVisitFlags, 0, sizeof(m_borderTentVisitFlags));
    m_cartographerMask[0] = 0x100;
    m_cartographerMask[1] = 0xbf;
    m_cartographerMask[2] = 0x40;
    memset(m_cartographerFlags, 0, sizeof(m_cartographerFlags));
    initializeGameData();
    memset(m_rumourState, 0, sizeof(m_rumourState));
    m_isTutorial = 0;
    m_grailAsked = 0;
}

// E:\gamedcs\game.cpp:11746
// CodeView dc 0xbd5f4 marks the default constructor compgenx. The
// game::game array construction takes its address at +0x46; all work is
// implicit member initialization, including the artifact arrays and string.
VA_COMPGEN(0x004ce4b0, 0x68, CLASS_CTOR, HeroExtra)

VA_COMPGEN(0x004ce520, 0x4A, IMPLICIT_DTOR, HeroExtra)

// CodeView dc 0xbd630: CV_fldattr_t.compgenx marks this destructor
// as implicit. Its retained retail body performs only base/member teardown.
VA_COMPGEN(0x004ce570, 0x32, IMPLICIT_DTOR, playerData)

// E:\gamedcs\game.cpp:11749
VA(0x004ce5b0, 0x346)  // dc 0xbbd28
game::~game()
{
    clearEventRecords();
}

VA(0x004ce900, 0x3B)  // dc 0xbbe68
boat* game::getHeroBoat(int id, unsigned char occupied)
{
    for (boat* i = m_boats.begin(); i != m_boats.end(); i++) {
        if (i->m_allocated && i->m_occupyingHero == id && i->m_occupied == occupied)
            return i;
    }
    return 0;
}

VA(0x004ce940, 0x27)  // dc 0xbbee4
bool game::isHuman(int gamePos) const
{
    if (gamePos >= 8 || gamePos < 0)
        gamePos = 0;
    return m_players[gamePos].isHuman();
}

VA(0x004ce970, 0x3C)  // dc 0xbbfcc
bool game::isLocalHuman(int gamePos) const
{
    if (gamePos >= 8 || gamePos < 0)
        return false;
    return m_players[gamePos].isLocalHuman();
}

VA(0x004ce9b0, 0x6A)  // dc 0xbc010
playerData* game::getLocalPlayer()
{
    return &m_players[getLocalPlayerGamePos()];
}

VA(0x004cea20, 0x4E)  // dc 0xbc038
int game::getLocalPlayerGamePos() const
{
    if (g_mpNetProtocol == MP_HOTSEAT) {
        int pos = g_netLocalGamePos;
        if (pos >= 0 && pos < 8 && m_players[pos].isHuman())
            return pos;
        return getLastHuman();
    }
    return g_localGamePos;
}

VA(0x004cea70, 0xE7)  // dc 0xbc0c0
type_point game::getPuzzleOrigin() const
{
    type_point result;
    result.m_x = m_ultimateArtifactX - 9;
    result.m_y = m_ultimateArtifactY - 8;
    result.m_z = m_ultimateArtifactZ;

    sRand(m_ultimateArtifactY * 81901
          + m_ultimateArtifactX * 67843 + 79451);
    result.m_x += random(-2, 2);
    result.m_y += random(-2, 2);
    return result;
}

VA(0x004ceb60, 0xBD)  // dc 0xbc1fc
char* game::getPlayerName(int gamePos)
{
    if (gamePos >= 8 || gamePos < 0)
        gamePos = 0;
    return m_players[gamePos].getName();
}

VA(0x004cec20, 0x25)  // dc 0xbc23c
int game::getGamePosFromDPID(unsigned long dpid) const
{
    for (int i = 0; i < 8; i++) {
        if (m_players[i].m_dpid == dpid)
            return i;
    }
    return -1;
}

VA(0x004cec50, 0x3E)  // dc 0xbc2b8
bool game::isLastHuman(int gamePos) const
{
    int i = gamePos + 1;

    if (i >= 8)
        return true;
    do {
        if (isHuman(i))
            return false;
    } while (++i < 8);
    return true;
}

VA(0x004cec90, 0x18)  // dc 0xbc300
bool game::isMultiplayer() const
{
    if (g_remoteOn || g_mpNetProtocol == MP_HOTSEAT)
        return true;
    return false;
}

VA(0x004cecb0, 0x81)
void game::resetGame(int difficulty, int version,
                     NewSMapHeader* defaultMapHeader)
{
    for (int playerIndex = 0; playerIndex < 8; ++playerIndex)
        g_game->m_players[playerIndex].init();

    m_setup.m_fileInitialized = 0;
    g_gameOver = 0;
    g_buildAllBuildings = 0;
    setupOrigData();
    initNewGame(difficulty, version, defaultMapHeader, 0);
    g_turnDuration.clear();
    g_thisNetGotAdventureControl = 0;
    TSpellbookWindow::reset();
    memset(m_borderTentVisitFlags, 0, sizeof(m_borderTentVisitFlags));
}

VA(0x004ced40, 0x1D0)  // sole caller 0x5013b0 + game+0x4e7bc vector layout
void game::recordMonsterIdentifier(int identifier, type_point point)
{
    MonsterIdentifier record;
    record.m_identifier = identifier;
    record.m_point = point;
    m_monsterIdentifiers.push_back(record);
}

// Quest-monster setup resolves the most recently recorded object with this
// identifier; absent objects use the packed all-minus-one point sentinel.
VA(0x004cef10, 0x68)  // sole semantic caller 0x56ef20 + reverse 8-byte walk
type_point game::gameFn004CEF10(int identifier)
{
    for (unsigned int i = m_monsterIdentifiers.size(); i-- != 0;) {
        if (m_monsterIdentifiers[i].m_identifier == identifier)
            return m_monsterIdentifiers[i].m_point;
    }

    type_point point;
    point.m_x = -1;
    point.m_y = -1;
    point.m_z = -1;
    return point;
}

VA_COMPGEN(0x004bdf80, 0x1B1, IMPLICIT_DTOR, SavedGameHeader)

// Sign's one-string destructor is emitted out of line and is the callee used
// by the vector helpers below.
VA_COMPGEN(0x004b9230, 0x3E, IMPLICIT_DTOR, Sign)

VA_COMPGEN(0x004c4df0, 0x3E, PAIR_CONST_INT_DTOR, type_map_hero_info)
VA_COMPGEN(0x004caa40, 0x26, IMPLICIT_DTOR, TPickRandomTownName)
VA_COMPGEN(0x004cbcf0, 0x4B, IMPLICIT_DTOR, CGameTransferDlg)

// InitNewGame's exception path retains Dinkumware's string-taking
// std::logic_error constructor. The late STL anchor emits the identical named
// public until that large caller is reconstructed.
VA_COMPGEN(0x004c3090, 0x162, CLASS_CTOR, logic_error)

VA_COMPGEN(0x004cef80, 0x12, BITSET_SUBSCRIPT, Bitset145)
VA_COMPGEN(0x004cefa0, 0x67, BITSET_REFERENCE_ASSIGN, Bitset70)
VA_COMPGEN(0x004cf010, 0x2E, BITSET_COUNT, Bitset145)
// The shared bitset<4>::test at 0x4cf960 expands here and remains
// emitted in singleselectionwindow, alongside its retained _Xran body.
VA_COMPGEN(0x004cf9a0, 0x63, BITSET_SET, Bitset144)

VA_COMPGEN(0x004cf0b0, 0x3B, VECTOR_DTOR, TownExtra)
VA_COMPGEN(0x004cf0f0, 0x2D6, VECTOR_RESIZE, TBlackMarket)
VA_COMPGEN(0x004cf3d0, 0x3B, VECTOR_DTOR, town)
VA_COMPGEN(0x004cf410, 0x2A1, VECTOR_RESIZE, town)
VA_COMPGEN(0x004cf6c0, 0x23, VECTOR_SIZE, town)
VA_COMPGEN(0x004cf6f0, 0x38, VECTOR_DTOR, Sign)
VA_COMPGEN(0x004cf730, 0x13, VECTOR_SIZE, mine)
VA_COMPGEN(0x004cf750, 0x21, VECTOR_SIZE, boat)
VA_COMPGEN(0x004cf780, 0x38, VECTOR_DTOR, type_creature_bank)
VA_COMPGEN(0x004cf7c0, 0x38, VECTOR_DTOR, TRumour)
VA_COMPGEN(0x004cf800, 0x67, BITSET_REFERENCE_ASSIGN, Bitset5)
VA_COMPGEN(0x004cf870, 0x53, BITSET_CTOR, Bitset28)
VA_COMPGEN(0x004cf8d0, 0x1C, BITSET_COUNT, Bitset28)
VA_COMPGEN(0x004cf8f0, 0x67, BITSET_REFERENCE_ASSIGN, Bitset28)
VA_COMPGEN(0x004cfa40, 0x13, VECTOR_CAPACITY, type_university)
VA_COMPGEN(0x004cfa60, 0x63, BITSET_SET, Bitset145)
VA_COMPGEN(0x004cfad0, 0x37, BITSET_TEST, Bitset145)
// readMapPlayerSlot retains the three-argument insert reached by its expanded
// resize. The 0x14-byte stride and type_map_hero_identity copy/destructor
// callees distinguish this specialization from the other vector inserts.
VA_COMPGEN(0x004cfb10, 0x31C, VECTOR_INSERT, type_map_hero_identity)
VA_COMPGEN(0x004cfef0, 0x34, BITSET_TEST, Bitset8)
VA_COMPGEN(0x004d0070, 0x63, BITSET_SET, Bitset156)
VA_COMPGEN(0x004d00e0, 0x2FC, VECTOR_INSERT, TBlackMarket)
VA_COMPGEN(0x004d03e0, 0x44, VECTOR_ERASE, TBlackMarket)
VA_COMPGEN(0x004d0430, 0x31C, VECTOR_INSERT, town)
VA_COMPGEN(0x004d0750, 0x6D, VECTOR_ERASE, town)
VA_COMPGEN(0x004d07c0, 0x26, VECTOR_DESTROY, town)
VA_COMPGEN(0x004d07f0, 0x31C, VECTOR_INSERT, Sign)
VA_COMPGEN(0x004d0b10, 0x8F, VECTOR_ERASE, Sign)
VA_COMPGEN(0x004d0ba0, 0x26B, VECTOR_INSERT, mine)
VA_COMPGEN(0x004d0e10, 0x44, VECTOR_ERASE, mine)
VA_COMPGEN(0x004d0e60, 0x20A, VECTOR_INSERT, boat)
VA_COMPGEN(0x004d1070, 0x2E4, VECTOR_INSERT, boat)
VA_COMPGEN(0x004d1360, 0x44, VECTOR_ERASE, boat)
VA_COMPGEN(0x004d13b0, 0x23, VECTOR_DESTROY, type_creature_bank)
VA_COMPGEN(0x004d13e0, 0x300, VECTOR_INSERT, TRumour)
VA_COMPGEN(0x004d16e0, 0x7F, VECTOR_ERASE, TRumour)
VA_COMPGEN(0x004d1760, 0x23, VECTOR_DESTROY, TRumour)
VA_COMPGEN(0x004d17b0, 0x11, BITSET_FLIP, Bitset28)
VA_COMPGEN(0x004d17d0, 0x34, BITSET_TEST, Bitset28)
VA_COMPGEN(0x004d1810, 0x15, BITSET_ANY, Bitset28)
// The bitset<4> copy at 0x4d1850 is enrolled in singleselectionwindow,
// whose real retail feature-test callers retain the same specialization.
// The 0x44-byte stride and calls into the generated CObjectType copy helpers
// distinguish this specialization from the smaller CObject record.
VA_COMPGEN(0x004d1920, 0x359, VECTOR_INSERT, CObjectType)
VA_COMPGEN(0x004d1c80, 0xCB, BITSET_XRAN, Bitset70)
VA_COMPGEN(0x004d2090, 0xCB, BITSET_XRAN, Bitset156)
VA_COMPGEN(0x004d2160, 0x31, VECTOR_UFILL, TBlackMarket)
// The black-market vector's retained uninitialized copy advances by its
// proven 28-byte stride.  The emitted specialization matches all 59 bytes.
VA_COMPGEN(0x0054d920, 0x3B, VECTOR_UCOPY, TBlackMarket)
VA_COMPGEN(0x004d21a0, 0x3E, VECTOR_UCOPY, town)
VA_COMPGEN(0x004d21e0, 0x2C, VECTOR_UFILL, town)
VA_COMPGEN(0x004d2210, 0x3B, VECTOR_UCOPY, boat)
VA_COMPGEN(0x004d2250, 0x31, VECTOR_UFILL, boat)
VA_COMPGEN(0x004d2290, 0x372, VECTOR_INSERT, type_creature_bank)
VA_COMPGEN(0x004d2610, 0xCB, BITSET_XRAN, Bitset5)
VA_COMPGEN(0x004d26e0, 0xCB, BITSET_XRAN, Bitset28)

VA_COMPGEN(0x004d3630, 0x250, STD_CONSTRUCT, town)
VA_COMPGEN(0x004d3880, 0x16F, STD_CONSTRUCT, Sign)
VA_COMPGEN(0x004d39f0, 0x14, STD_CONSTRUCT, boat)
VA_COMPGEN(0x004d3a10, 0x15B, STD_CONSTRUCT, TRumour)
VA_COMPGEN(0x004d3b70, 0x1BA, STD_CONSTRUCT, CObjectType)
VA_COMPGEN(0x004d3d30, 0xBF, STD_CONSTRUCT, type_creature_bank)
VA_COMPGEN(0x004d3df0, 0x2C2, IMPLICIT_COPY_ASSIGN, town)
VA_COMPGEN(0x004d40c0, 0x19B, IMPLICIT_COPY_ASSIGN, CObjectType)
VA_COMPGEN(0x004d4260, 0x1E3, IMPLICIT_COPY_ASSIGN, type_creature_bank)
VA_COMPGEN(0x004d4450, 0x3E, IMPLICIT_DTOR, TownExtra)
VA_COMPGEN(0x004d4490, 0x67, BITSET_REFERENCE_ASSIGN, Bitset8)
VA_COMPGEN(0x004d4500, 0x2CF, VECTOR_RESIZE, generator)
VA_COMPGEN(0x004d47d0, 0x23, VECTOR_SIZE, generator)
VA_COMPGEN(0x004d4800, 0x25E, VECTOR_RESIZE, type_university)
VA_COMPGEN(0x004d4a60, 0x40, VECTOR_UFILL, type_university)
VA_COMPGEN(0x004d4aa0, 0x1F1, VECTOR_RESIZE, type_point)
// NewSMapHeader::Read materializes the eight-player availability setter and
// the four-dword default mask initializer. Their immediate bounds/fill counts
// distinguish these two retained bitset widths from the neighboring rows.
VA_COMPGEN(0x004d4cc0, 0x60, BITSET_SET, Bitset8)
// The 0x6c-byte copy stride followed by type_creature_bank tail destruction
// identifies the intervening range erase rather than a generic vector helper.
VA_COMPGEN(0x004d4d20, 0xBF, VECTOR_ERASE, type_creature_bank)
VA_COMPGEN(0x004d4de0, 0x63, BITSET_SET, Bitset128)
VA_COMPGEN(0x004d4e50, 0x37, BITSET_TEST, Bitset128)
VA_COMPGEN(0x004d4e90, 0x1A, BITSET_TIDY, Bitset128)
VA_COMPGEN(0x004d4eb0, 0xCB, BITSET_XRAN, Bitset12)
VA_COMPGEN(0x004d4f80, 0x3B, VECTOR_UCOPY, generator)
VA_COMPGEN(0x004d4fc0, 0x31, VECTOR_UFILL, generator)
VA_COMPGEN(0x004d5000, 0xCB, BITSET_XRAN, Bitset128)

// Retained instantiation of the canonical template at game.cpp2733.
#if 0  // @carcass -- claim-only template instance
VA(0x004d2870, 0x24D)  // dc 0xc1b6c
bool loadObjectVector(TAbstractFile* infile,
                      std::vector<type_creature_bank>& destVector)
{
    // @stub
}
#endif

// The retained template instances are claimed in retail address order.
// Their one active implementation appears at the DC source-order boundary.
#if 0  // @carcass -- claim-only template instances
VA(0x004d2ac0, 0x60)  // point/long ICF twin, dc 0xc1dd4 / 0xc1e58
bool saveVector(TAbstractFile* outfile, std::vector<type_point>& srcVector)
{
    // @stub
}

VA(0x004d2b20, 0x60)  // university stride and sole Save call, dc 0xc1edc
bool saveVector(TAbstractFile* outfile, std::vector<type_university>& srcVector)
{
    // @stub
}
#endif

#if 0  // @carcass -- claim-only template instance
VA(0x004d2b80, 0x102)  // dc 0xc1f64
bool saveObjectVector(TAbstractFile* outfile,
                      std::vector<type_creature_bank>& srcVector)
{
    // @stub
}
#endif

// Original: game::GetLastHuman; game.cpp:11869, dc 0xbc320
int game::getLastHuman() const
{
    for (int i = 7; i >= 0; --i) {
        if (m_players[i].isHuman())
            return i;
    }
    return 0;
}

VA_COMPGEN(0x004cff30, 0x17, BITSET_TIDY, Bitset8)
VA_COMPGEN(0x004d1790, 0x15, BITSET_TIDY, Bitset5)
VA_COMPGEN(0x004d1830, 0x17, BITSET_TIDY, Bitset28)
VA_COMPGEN(0x004cf040, 0x6A, BITSET_REFERENCE_ASSIGN, Bitset156)
VA_COMPGEN(0x004cfe30, 0x8F, VECTOR_ERASE, type_map_hero_identity)
VA_COMPGEN(0x004cfec0, 0x23, VECTOR_DESTROY, type_map_hero_identity)
VA_COMPGEN(0x004d2050, 0x32, TREE_MIN, type_map_hero_info)
VA_COMPGEN(0x004cff50, 0x115, TREE_INSERT, type_map_hero_info)
VA_COMPGEN(0x004d1d50, 0x2F9, TREE_NODE_INSERT, type_map_hero_info)
VA_COMPGEN(0x004d27b0, 0xB3, TREE_CONST_ITERATOR_DEC, type_map_hero_info)

// COMDAT pairing: std::copy<type_map_hero_identity>, agreement 0.987
// (156 base vs 152 retail instructions).
VA_COMPGEN(0x004d2e50, 0x18C, STD_COPY, type_map_hero_identity)

// COMDAT pairing: std::copy_backward<town>, agreement 0.984.
VA_COMPGEN(0x004d32e0, 0x344, STD_COPY_BACKWARD, town)

// COMDAT pairing: std::fill<town>, agreement 0.992.
VA_COMPGEN(0x004d2fe0, 0x2FC, STD_FILL, town)

// COMDAT pairing: _tree::1?$_Tree, mnemonic agreement 0.959.
VA_COMPGEN(0x004b61f0, 0x6E, IMPLICIT_DTOR, _tree)

// COMDAT pairing: out_of_range::1out_of_range, mnemonic agreement 0.909.
VA_COMPGEN(0x004b6be0, 0x4B, IMPLICIT_DTOR, out_of_range)

VA_COMPGEN(0x0045c200, 0x53E, TREE_ERASE_ITERATOR, type_map_hero_info)

// COMDAT pairing: bitset<129>::_Xran. This address arrived on lane 16 as
// `bitset145` from a 0.978 similarity score; the bound compare refutes that.
// All three callers of 0x48edf0 guard with `cmp <reg>, 0x81` (129) - the two
// customcampaign-segment callers 0x8ead0/0x8ece0 and rmg's claimed
// bitset<129>::set at 0x14ded0 - while the four callers of 0x8d9a0 all guard
// with 0x91 (145). The claim therefore moves here: game.obj and rmg.obj are
// the only two that emit `?_Xran@?$bitset@$0IB@@`, customcampaign.obj does
// not emit it at all.
VA_COMPGEN(0x0048edf0, 0xCB, BITSET_XRAN, Bitset129)

// COMDAT pairing: vector<vector<hero>>::operator=, decided by its own callee
// list: it calls the vector<hero>::operator= claimed at 0x5ff30, which is
// exactly what the outer assignment does per element.
VA_COMPGEN(0x0045f5e0, 0x1CD, VECTOR_COPY_ASSIGN, hero_vector)

VA_COMPGEN(0x0048e9f0, 0x6A, BITSET_REFERENCE_ASSIGN, Bitset144)
VA_COMPGEN(0x0048ea60, 0x6A, BITSET_REFERENCE_ASSIGN, Bitset145)
VA_COMPGEN(0x0048ead0, 0x6A, BITSET_REFERENCE_ASSIGN, Bitset129)

// COMDAT pairing: vector<vector<hero>>::~vector - reached from
// TCampaignWindow's constructor, ~SavedGameHeader and three
// singleselectionwindow destructors, all of which hold campaign hero pools.
VA_COMPGEN(0x0045f560, 0x7A, VECTOR_DTOR, hero_vector)

// COMDAT pairing: std::_Construct<vector<hero>>, decided by two callers that
// are themselves claimed COMDATs of the same family - the
// vector<vector<hero>>::operator= at 0x5f5e0 and ::insert at 0x8c270.
VA_COMPGEN(0x0045fce0, 0xD1, STD_CONSTRUCT, hero_vector)

// COMDAT pairing: std::_Construct<vector<type_artifact>>, likewise reached
// from the claimed vector<vector<type_artifact>>::insert at 0x8c7a0 and from
// vector<hero>::_Ucopy - hero carries the artifact vector, so copying a hero
// range constructs one.
VA_COMPGEN(0x0045fdc0, 0x9C, STD_CONSTRUCT, type_artifact_vector)

// COMDAT pairing: vector<vector<type_artifact>>::~vector and its ::_Destroy -
// the artifact half of the campaign carry-over pools whose hero half this
// lane claimed at 0x5f560/0x8c5b0. Arity matches both (`ret` for the nullary
// destructor, `ret 8` for _Destroy's two pointers) and 0x5f7b0 is reached
// from campaignwindow, singleselectionwindow and three further segments.
VA_COMPGEN(0x0045f7b0, 0x59, VECTOR_DTOR, type_artifact_vector)
VA_COMPGEN(0x0048caa0, 0x38, VECTOR_DESTROY, type_artifact_vector)

// COMDAT pairing: vector<vector<type_artifact>>::operator=, the artifact twin
// of the hero-pool assignment claimed at 0x45f5e0 above. Both compile to the
// same 461 bytes because the outer assignment differs only in which inner
// operator= it calls per element - which is also why /OPT:ICF could not fold
// them - and 0x45f810 is the copy whose per-element callee is the
// vector<type_artifact> assignment, mnemonic agreement 0.992.
VA_COMPGEN(0x0045f810, 0x1CD, VECTOR_COPY_ASSIGN, type_artifact_vector)

// COMDAT pairing: vector<type_university>::_Ucopy. Five instantiations of
// _Ucopy resemble this address; type_university wins on agreement (0.907
// against 0.810 for the next) and `ret 0xc` matches its three pointers.
VA_COMPGEN(0x00434c70, 0x49, VECTOR_UCOPY, type_university)

// The string overload, not the CatchableType copy constructor at 0x404700.
// Keep this identity missing when all string-ctor uses expand in this TU.
VA_COMPGEN(0x00487bd0, 0x160, CLASS_NONCOPY_CTOR, out_of_range)

// Slot 5 of that zip: vector<hero>::~vector, the row ~SCampaign,
// vector<vector<hero>>::operator= and vector<vector<hero>>::insert all reach.
VA_COMPGEN(0x0045fc50, 0x3B, VECTOR_DTOR, hero)

// hero declares no destructor, so this is the implicit teardown its own
// vector<hero>::~vector and scalar-deleting dtor call.
VA_COMPGEN(0x0045fc90, 0x4A, IMPLICIT_DTOR, hero)

VA_COMPGEN(0x0045fe60, 0x53, SCALAR_DELETING_DTOR, vector)
VA_COMPGEN(0x0045fec0, 0x3A, SCALAR_DELETING_DTOR, vector)

// Slot 1 of vector<vector<hero>>::~vector: hero's scalar deleting dtor, also
// reached from vector<hero>::operator=, PruneCrossoverHeroes and
// vector<vector<hero>>::erase.
VA_COMPGEN(0x0045ff00, 0x21, SCALAR_DELETING_DTOR, hero)

// COMDAT pairing: _Tree<int, pair<const int, type_map_hero_info>>::_Erase,
// agreement 0.952 at an exactly equal 173-byte extent; this object is the
// only one that instantiates the tree.
VA_COMPGEN(0x0045c8b0, 0xAD, TREE_ERASE, type_map_hero_info)

VA_COMPGEN(0x0048d480, 0x28, BITSET_TIDY, Bitset145)
VA_COMPGEN(0x0048c0b0, 0x28, BITSET_TIDY, Bitset144)
