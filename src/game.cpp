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
DATA(0x0063d570) const TCreatureType g_creatureGenerator1Types[80] = { TCreatureType(106), TCreatureType(96), TCreatureType(74), TCreatureType(66), TCreatureType(68), TCreatureType(10), TCreatureType(14), TCreatureType(112), TCreatureType(12), TCreatureType(94), TCreatureType(54), TCreatureType(104), TCreatureType(16), TCreatureType(113), TCreatureType(52), TCreatureType(18), TCreatureType(114), TCreatureType(30), TCreatureType(36), TCreatureType(86), TCreatureType(98), TCreatureType(84), TCreatureType(44), TCreatureType(102), TCreatureType(26), TCreatureType(4), TCreatureType(72), TCreatureType(46), TCreatureType(110), TCreatureType(42), TCreatureType(100), TCreatureType(34), TCreatureType(80), TCreatureType(76), TCreatureType(78), TCreatureType(8), TCreatureType(38), TCreatureType(48), TCreatureType(90), TCreatureType(88), TCreatureType(50), TCreatureType(82), TCreatureType(92), TCreatureType(28), TCreatureType(40), TCreatureType(22), TCreatureType(70), TCreatureType(115), TCreatureType(60), TCreatureType(108), TCreatureType(20), TCreatureType(24), TCreatureType(64), TCreatureType(62), TCreatureType(56), TCreatureType(58), TCreatureType(0), TCreatureType(2), TCreatureType(6), TCreatureType(118), TCreatureType(120), TCreatureType(130), TCreatureType(132), TCreatureType(133), TCreatureType(134), TCreatureType(135), TCreatureType(136), TCreatureType(137), TCreatureType(24), TCreatureType(112), TCreatureType(113), TCreatureType(114), TCreatureType(115), TCreatureType(138), TCreatureType(139), TCreatureType(140), TCreatureType(141), TCreatureType(142), TCreatureType(143), TCreatureType(144) };
DATA(0x00677938) TCreatureType g_creatureGenerator4Types[2][4] = {
    { TCreatureType(112), TCreatureType(114), TCreatureType(113), TCreatureType(115) },
    { TCreatureType(32), TCreatureType(33), TCreatureType(116), TCreatureType(117) }
};
DATA(0x00677974) const char* g_artifactObjectDefFormat = "ava%04d.def";

// Retail initial data; dimensions follow the typed table consumers.
DATA(0x00677998) double g_productionHandicap[3] = { 0.0, 0.15, 0.3 };
DATA(0x00678170) int g_initResourcesHuman[5][7] = {
    { 30, 15, 30, 15, 15, 15, 30000 },
    { 20, 10, 20, 10, 10, 10, 20000 },
    { 15, 7, 15, 7, 7, 7, 15000 },
    { 10, 4, 10, 4, 4, 4, 10000 },
    { 0, 0, 0, 0, 0, 0, 0 }
};
DATA(0x006781fc) int g_initResourcesComputer[5][7] = {
    { 5, 2, 5, 2, 2, 2, 5000 },
    { 10, 4, 10, 4, 4, 4, 7500 },
    { 15, 7, 15, 7, 7, 7, 10000 },
    { 15, 7, 15, 7, 7, 7, 10000 },
    { 15, 7, 15, 7, 7, 7, 10000 }
};

// Legacy multiplier tables retained in Complete without code references.
// DC names gfSSLogisticsMod/gfSSNavigationMod/gfSSArcheryMod/gfSSAIArcheryMod
// identify the same four mastery rows; all 64 bytes agree between the builds.
DATA(0x006782ec) float g_ssLogisticsMod[4] = { 1.0f, 1.1f, 1.2f, 1.3f };
DATA(0x006782fc) float g_ssNavigationMod[4] = { 1.0f, 1.33f, 1.66f, 2.0f };
DATA(0x0067830c) float g_ssArcheryMod[4] = { 1.0f, 1.1f, 1.25f, 1.5f };
DATA(0x0067831c) float g_ssAIArcheryMod[4] = { 1.0f, 1.04f, 1.1f, 1.2f };

DATA(0x0069ccb0) playerData* g_currentPlayer;
DATA(0x0069cca8) int g_netLocalGamePos;
DATA(0x00699554) int g_localGamePos;
// Original DC name: iSandAnim; GetTurnAIVars resets it beside iCurHourGlassPhase.
DATA(0x00691680) int g_sandAnim;


// Retail table initializers, in the layouts used by their named consumers.
DATA(0x00677958) const char* g_resourceObjectDefs[NUM_RESOURCES] = { "avtwood0.def", "avtmerc0.def", "avtore0.def", "avtsulf0.def", "avtcrys0.def", "avtgems0.def", "avtgold0.def" };
// Original DC name: HoleSpriteFilenames. Terrain order is dirt through rock;
// water and rock have no digging sprite. The retained retail table is unused.
DATA(0x006779e4) const char* g_holeSpriteFilenames[10] = {
    "avlhold0.def", "avlhlds0.def", "avlholg0.def", "avlhlsn0.def",
    "avlhols0.def", "avlholr0.def", "avlholx0.def", "avlholl0.def", "", ""
};
DATA(0x00677a0c) const char* g_townVillageObjectDefs[9] = { "AVCcast0.def", "AVCramp0.def", "AVCtowr0.def", "AVCinft0.def", "AVCnecr0.def", "AVCdung0.def", "AVCstro0.def", "AVCftrt0.def", "AVChfor0.def" };
DATA(0x00677a30) const char* g_townFortObjectDefs[9] = { "AVCcasx0.def", "AVCramx0.def", "AVCtowx0.def", "AVCinfx0.def", "AVCnecx0.def", "AVCdunx0.def", "AVCstrx0.def", "AVCftrx0.def", "AVChforx.def" };
DATA(0x00677a54) const char* g_townCapitolObjectDefs[9] = { "AVCcasz0.def", "AVCramz0.def", "AVCtowz0.def", "AVCinfz0.def", "AVCnecz0.def", "AVCdunz0.def", "AVCstrz0.def", "AVCforz0.def", "AVChforz.def" };
DATA(0x00677978) int g_mineProduction[7] = { 2, 1, 2, 1, 1, 1, 1000 };
DATA(0x006779b0) int g_neutralTownLevelWeights[6] = { 2, 3, 4, 5, 4, 3 };
// Retail newMap copies this independent seven-resource tutorial row.
DATA(0x006779c8) int g_tutorialStartingResources[NUM_RESOURCES] =
    { 50, 50, 50, 50, 50, 50, 50000 };
DATA(0x0069fbf8) int g_newMapStartingBonus[8];
DATA(0x0069fb24) int g_startingHeroOverrides[8];

// Retail scalar state; startup initial values come from the pinned image.
DATA(0x00697294) TTextResource* g_randomTavernText;
DATA(0x0069774c) bool g_inCampaign;
DATA(0x00697750) int g_weekType;
DATA(0x006983fc) int g_weekTypeExtra;
// DC PerMonth stores the effect to giMonthType and the creature to
// giMonthTypeExtra; retail perMonth writes the effect to 0x698834.
DATA(0x00698834) int g_monthType;
DATA(0x00697748) int g_monthTypeExtra;
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
// saveGame and loadGame share one .rdata copy (0x63e65c) ahead of this
// unit's other constant tables; the pooled "%s%s" format stays in .data.
DATA(0x0063e65c) static const char g_gamesDirectoryPrefix[] = ".\\GAMES\\";
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
// narrowing RandomizeShrine's bitset pin to the subscript alone so the
// ctor expands onto retail's `_Tidy` call (-0.50).
// Random-map placeholder domains recovered from RandomizeEvents' retail
// switch. They are source-local because no cross-TU enum identity survives.
const int g_blackBoxRandomAny = 1;
const int g_blackBoxRandomTreasure = 2;
const int g_blackBoxRandomMinor = 3;
const int g_blackBoxRandomMajor = 4;
const int g_blackBoxRandomRelic = 5;
const int g_pyramidSpellLevel = 5;
const int g_shrineLevelOne = 1;
const int g_shrineLevelTwo = 2;
const int g_shrineLevelThree = 3;
const unsigned char g_whirlpoolTriggerXOffset = 2;
const unsigned char g_whirlpoolTriggerYOffset = 1;

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

VA(0x004b8410, 0x33)
DC_ADDRESS(0x0a2af8, 0x62)
MAC_ADDRESS(0x0c9fa8, 0xa0)
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
MAC_ADDRESS(0x0ca048, 0xd4)
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

VA(0x004b8550, 0x48)
DC_ADDRESS(0x0a2da0, 0xa6)
MAC_ADDRESS(0x0ca11c, 0x88)
generator::generator()
    : m_genClass(-1), m_genType(-1)
{
    m_playerOwner = -1;
    m_mapX = -1;
    m_mapY = -1;
    m_mapZ = -1;
    m_townId = -1;
    clearCreatureSlots();
}

// Project-inferred creature/population reset shared by construction and map
// initialization. Guard-army and ownership setup are separate operations.
void generator::clearCreatureSlots()
{
    for (int i = 0; i < 4; i++) {
        m_type[i] = CREATURE_NONE;
        m_population[i] = 0;
    }
}

VA(0x004b85a0, 0x13B)
DC_ADDRESS(0x0a2e48, 0x194)
MAC_ADDRESS(0x0ca1a4, 0x200)
bool generator::load(TAbstractFile* infile)
{
    if (infile->read(&m_playerOwner, sizeof(m_playerOwner)) !=
        sizeof(m_playerOwner))
        return 0;
    if (infile->read(&m_genClass, sizeof(m_genClass)) != sizeof(m_genClass))
        return 0;
    if (infile->read(&m_genType, sizeof(m_genType)) != sizeof(m_genType))
        return 0;

    for (int slot = 0; slot < 4; slot++) {
        int creature = readValue<unsigned char>(infile);
        m_type[slot] = TCreatureType(creature);
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

VA(0x004b86e0, 0xB1)
DC_ADDRESS(0x0a2fdc, 0xe6)
MAC_ADDRESS(0x0ca3a4, 0x188)
bool generator::save(TAbstractFile* outfile)
{
    outfile->write(&m_playerOwner, sizeof(m_playerOwner));
    outfile->write(&m_genClass, sizeof(m_genClass));
    outfile->write(&m_genType, sizeof(m_genType));

    for (int slot = 0; slot < 4; slot++) {
        writeValue<char>(outfile, m_type[slot]);
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
// Mac retains removeBonus, updateBonus and setOwner consecutively at code0
// +0xca52c, +0xca624 and +0xca71c between save and initialize, as in the
// Dreamcast game.cpp line order. Its calls do not establish inline spelling or
// header placement. VC6 also expands removeBonus and setOwner from ordinary
// game.cpp definitions at their known call sites.
DC_ADDRESS(0x0a30c4, 0xb2)
MAC_ADDRESS(0x0ca52c, 0xf8)
void generator::removeBonus()
{
    adjustTownBonuses(-1);
}

// Project-inferred inverse operation. Keep the cached owner row, alignment
// sentinel and per-town creature read used by both native wrappers.
void generator::adjustTownBonuses(long change)
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
            currentTown->changeGeneratorBonus(m_type[0], change);
    }
}

VA(0x004b87a0, 0xB8)
DC_ADDRESS(0x0a3178, 0xd8)
MAC_ADDRESS(0x0ca624, 0xf8)
// VC6 control: without inline here, initialize retains an updateBonus call
// and has 26 blocks instead of the retail 38-block expansion.
inline void generator::updateBonus()
{
    adjustTownBonuses(1);
}

// E:\gamedcs\game.cpp:557
DC_ADDRESS(0x0a3250, 0x38)
MAC_ADDRESS(0x0ca71c, 0x5c)
void generator::setOwner(long owner)
{
    if (owner == m_playerOwner)
        return;

    removeBonus();
    m_playerOwner = owner;
    updateBonus();
}

VA(0x004b8860, 0x1F7)
DC_ADDRESS(0x0a3288, 0x96)
MAC_ADDRESS(0x0ca778, 0xf4)
void generator::initialize(long newOwner)
{
    clearCreatureSlots();
    m_guards.initialize();

    const TCreatureType* types;
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

VA(0x004b8a60, 0x88)
DC_ADDRESS(0x0a3320, 0xfc)
MAC_ADDRESS(0x0ca86c, 0xa8)
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
DC_ADDRESS(0x0a341c, 0x58)
MAC_ADDRESS(0x0ca914, 0x64)
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
// scheduling; reversing the true arm's commutative operands or naming the
// loaded building mask is byte-flat for this caller and the exact retained
// helper. The named-mask probe reproduces across game, town and townmgr.
// The call-stream diagnostic's getArmy difference is the mutable/const
// overload label at the same folded Windows entry, not an absent helper.
// Mac 0:0xca978..0xcb1f0 is compared from the ordinary shared headers and has
// the same seven artifact-count calls followed by daily gold. Its remaining
// zeroing, stack and register differences require source-model recovery.
VA(0x004b8af0, 0x573)
DC_ADDRESS(0x0a3474, 0x7f2)
MAC_ADDRESS(0x0ca978, 0x878)  // mine/town/player production consumers
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

        // Mac calculateProduction calls the mutable getArmy at 0xcac1c.
        // Windows folds both overload bodies at 0x5c1460.
        {
            int storage;
            storage = g_productionCreatureCrystalDragon;
            if (currentTown.getArmy()
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

// DC game.cpp:838..864 records int count, int x and char char_buffer.
// Complete reads through TAbstractFile in place of the older gzread stream.
// Retail's reads are the direct virtual calls: the inline readValue
// template spelling costs VC6 86.44%, the direct reads are exact.
VA(0x004b9070, 0x1B3)
DC_ADDRESS(0x0a3c68, 0xe8)
MAC_ADDRESS(0x0cb1f0, 0x118)
int game::loadSignPool(TAbstractFile* infile)
{
    int count;
    int x;
    char charBuffer;

    count = infile->read(&charBuffer, sizeof(charBuffer));
    if (count < sizeof(charBuffer))
        return -1;

    m_signs.resize(charBuffer);
    for (x = 0; x < m_signs.size(); ++x) {
        count = loadString(infile, m_signs[x].m_signText);
        if (count < 0)
            return -1;

        count = infile->read(&charBuffer, sizeof(charBuffer));
        if (count < sizeof(charBuffer))
            return -1;
        m_signs[x].m_hasText = charBuffer != 0;
    }
    return 0;
}

VA(0x004b9270, 0xCF)
DC_ADDRESS(0x0a3d50, 0x10c)
MAC_ADDRESS(0x0cb308, 0xfc)
int game::saveSignPool(TAbstractFile* outfile)
{
    // DC game.cpp:874..888 records int count, int x, char char_buffer,
    // with each write/save result assigned before its separate guard.
    // Complete uses the abstract-file write in place of DC's gzwrite.
    // Retail retains saveString in the loop; saveString's direct Windows
    // length write keeps it out of line. The count byte is written straight
    // from charBuffer, whose IL cost also keeps this pool writer a call in
    // game::save.
    int count;
    int x;
    char charBuffer;

    charBuffer = m_signs.size();
    count = outfile->write(&charBuffer, sizeof(charBuffer));
    if (count < sizeof(char))
        return -1;

    for (x = 0; x < m_signs.size(); ++x) {
        count = saveString(outfile, m_signs[x].m_signText);
        if (count < 0)
            return -1;

        charBuffer = m_signs[x].m_hasText;
        count = outfile->write(&charBuffer, sizeof(charBuffer));
        if (count < sizeof(char))
            return -1;
    }
    return 0;
}

// Legacy guard reads use the shared scalar reader. VC6 inlines both calls
// and reproduces the retail body exactly; shared direct reads scored 97.44%.
// Mac retains the first signed result across the second read. This does not
// justify platform-specific statement ordering. DC predates this branch.
// Full native-header Mac comparison awaits the reviewed MSL resize binding.
VA(0x004b9340, 0x240)
DC_ADDRESS(0x0a3e5c, 0x2b0)
MAC_ADDRESS(0x0cb404, 0x2a4)  // anchor-global (ClaimMine vector) + read-slot
int game::loadMinePool(TAbstractFile* infile, int saveVersion)
{
    unsigned char count;
    int x;
    char charBuffer;
    if (readValue(infile, count) < sizeof(unsigned char))
        return -1;
    m_mines.resize(static_cast<unsigned char>(count));

    for (x = 0; x < m_mines.size(); ++x) {
        if (readValue(infile, charBuffer)
            < sizeof(charBuffer))
            return -1;
        m_mines[x].m_playerOwner = charBuffer;
        if (readValue(infile, charBuffer)
            < sizeof(charBuffer))
            return -1;
        m_mines[x].m_type = charBuffer;
        if (readValue(infile, charBuffer)
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

        if (readValue(infile, count) < sizeof(unsigned char))
            return -1;
        m_mines[x].m_mapX = static_cast<unsigned char>(count);
        if (readValue(infile, count) < sizeof(unsigned char))
            return -1;
        m_mines[x].m_mapY = static_cast<unsigned char>(count);
        if (readValue(infile, count) < sizeof(unsigned char))
            return -1;
        m_mines[x].m_mapZ = static_cast<unsigned char>(count);
    }
    return 0;
}

// DC records count, x and two byte staging locals; Mac separates its signed
// owner/type/abandoned slot from the unsigned count/coordinate slot.
VA(0x004b9580, 0x165)
DC_ADDRESS(0x0a410c, 0x280)
MAC_ADDRESS(0x0cb6a8, 0x214)
int game::saveMinePool(TAbstractFile* outfile)
{
    int count;
    int x;
    unsigned char ucharBuffer;
    char charBuffer;

    ucharBuffer = m_mines.size();
    count = writeScalar(outfile, ucharBuffer);
    if (count < sizeof(ucharBuffer))
        return -1;

    for (x = 0; x < m_mines.size(); ++x) {
        charBuffer = m_mines[x].m_playerOwner;
        count = writeScalar(outfile, charBuffer);
        if (count < sizeof(charBuffer))
            return -1;
        charBuffer = m_mines[x].m_type;
        count = writeScalar(outfile, charBuffer);
        if (count < sizeof(charBuffer))
            return -1;
        charBuffer = m_mines[x].m_isAbandoned;
        count = writeScalar(outfile, charBuffer);
        if (count < sizeof(charBuffer))
            return -1;

        m_mines[x].m_guards.save(outfile);

        ucharBuffer = m_mines[x].m_mapX;
        count = writeScalar(outfile, ucharBuffer);
        if (count < sizeof(ucharBuffer))
            return -1;
        ucharBuffer = m_mines[x].m_mapY;
        count = writeScalar(outfile, ucharBuffer);
        if (count < sizeof(ucharBuffer))
            return -1;
        ucharBuffer = m_mines[x].m_mapZ;
        count = writeScalar(outfile, ucharBuffer);
        if (count < sizeof(ucharBuffer))
            return -1;
    }
    return 0;
}

// DC game.cpp:1024..1068 records int count, int x and separate unsigned/signed
// byte staging locals for the five shared gzread calls, which stay direct.
// Complete adds saveVersion's removable-troops branch; Mac 0xcba68..0xcba80
// sign-extends its independent by-value read, so that arm keeps readValue.
// The residual includes the ICF-folded vector<garrison>::insert that retail
// names as vector<mine>::insert.
VA(0x004b96f0, 0x1CB)
DC_ADDRESS(0x0a438c, 0x1bc)
MAC_ADDRESS(0x0cb8bc, 0x1f8)
int game::loadGarrisonPool(TAbstractFile* infile, int saveVersion)
{
    int count;
    int x;
    unsigned char ucharBuffer;
    char charBuffer;

    count = infile->read(&ucharBuffer, sizeof(ucharBuffer));
    if (count < sizeof(ucharBuffer))
        return -1;

    m_garrisons.resize(ucharBuffer);
    for (x = 0; x < m_garrisons.size(); ++x) {
        count = infile->read(&charBuffer, sizeof(charBuffer));
        if (count < sizeof(charBuffer))
            return -1;
        m_garrisons[x].m_playerOwner = charBuffer;

        m_garrisons[x].m_garrisonArmy.load(infile);

        count = infile->read(&ucharBuffer, sizeof(ucharBuffer));
        if (count < sizeof(ucharBuffer))
            return -1;
        m_garrisons[x].m_mapX = ucharBuffer;
        count = infile->read(&ucharBuffer, sizeof(ucharBuffer));
        if (count < sizeof(ucharBuffer))
            return -1;
        m_garrisons[x].m_mapY = ucharBuffer;
        count = infile->read(&ucharBuffer, sizeof(ucharBuffer));
        if (count < sizeof(ucharBuffer))
            return -1;
        m_garrisons[x].m_mapZ = ucharBuffer;

        if (saveVersion < 28) {
            m_garrisons[x].m_removableTroops = !g_inCampaign;
        } else {
            m_garrisons[x].m_removableTroops =
                readValue<char>(infile) != 0;
        }
    }
    return 0;
}

// Preserve DC's two byte staging locals. Mac retains a separate temporary
// for the later removableTroops field, so that final by-value helper stays.
VA(0x004b98c0, 0x139)
DC_ADDRESS(0x0a4548, 0x1a0)
MAC_ADDRESS(0x0cbab4, 0x1c8)
int game::saveGarrisonPool(TAbstractFile* outfile)
{
    int count;
    int x;
    unsigned char ucharBuffer;
    char charBuffer;

    ucharBuffer = m_garrisons.size();
    count = writeScalar(outfile, ucharBuffer);
    if (count < sizeof(ucharBuffer))
        return -1;

    for (x = 0; x < m_garrisons.size(); ++x) {
        charBuffer = m_garrisons[x].m_playerOwner;
        count = writeScalar(outfile, charBuffer);
        if (count < sizeof(charBuffer))
            return -1;

        m_garrisons[x].m_garrisonArmy.save(outfile);

        ucharBuffer = m_garrisons[x].m_mapX;
        count = writeScalar(outfile, ucharBuffer);
        if (count < sizeof(ucharBuffer))
            return -1;
        ucharBuffer = m_garrisons[x].m_mapY;
        count = writeScalar(outfile, ucharBuffer);
        if (count < sizeof(ucharBuffer))
            return -1;
        ucharBuffer = m_garrisons[x].m_mapZ;
        count = writeScalar(outfile, ucharBuffer);
        if (count < sizeof(ucharBuffer))
            return -1;

        writeValue<unsigned char>(outfile, m_garrisons[x].m_removableTroops);
    }
    return 0;
}

// DC game.cpp:1114..1174 records all five typed locals and the repeated
// vector indexing/read order below. Retail retains every abstract-file read
// and the boat loader, but expands one vector::size call that this TU keeps.
// Direct virtual reads are exact; the readValue template staging costs
// VC6 86.75% here.
VA(0x004b9a00, 0x239)
DC_ADDRESS(0x0a46e8, 0x296)
MAC_ADDRESS(0x0cbc7c, 0x28c)
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

// DC records separate unsigned-byte, signed-byte and short staging locals;
// Mac reuses slots +0x82/+0x83/+0x80 across the retained scalar writes.
VA(0x004b9c40, 0x1AD)
DC_ADDRESS(0x0a4980, 0x288)
MAC_ADDRESS(0x0cbf08, 0x260)
int game::saveBoatPool(TAbstractFile* outfile)
{
    unsigned short ushortBuffer;
    int count;
    int x;
    unsigned char ucharBuffer;
    char charBuffer;

    ucharBuffer = m_boats.size();
    count = writeScalar(outfile, ucharBuffer);
    if (count < sizeof(unsigned char))
        return -1;

    for (x = 0; x < m_boats.size(); ++x) {
        charBuffer = m_boats[x].m_allocated;
        count = writeScalar(outfile, charBuffer);
        if (count < sizeof(char))
            return -1;
        ucharBuffer = m_boats[x].m_id;
        count = writeScalar(outfile, ucharBuffer);
        if (count < sizeof(unsigned char))
            return -1;
        charBuffer = m_boats[x].m_type;
        count = writeScalar(outfile, charBuffer);
        if (count < sizeof(char))
            return -1;
        charBuffer = m_boats[x].m_facing;
        count = writeScalar(outfile, charBuffer);
        if (count < sizeof(char))
            return -1;
        charBuffer = m_boats[x].m_playerOwner;
        count = writeScalar(outfile, charBuffer);
        if (count < sizeof(char))
            return -1;

        ushortBuffer = m_boats[x].m_occupyingHero;
        count = writeScalar(outfile, ushortBuffer);
        if (count < sizeof(unsigned short))
            return -1;

        charBuffer = m_boats[x].m_occupied;
        count = writeScalar(outfile, charBuffer);
        if (count < sizeof(char))
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
DC_ADDRESS(0x0a4c08, 0x5e)
MAC_ADDRESS(0x0cc168, 0x9c)
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
DC_ADDRESS(0x0a4c68, 0x5e)
MAC_ADDRESS(0x0cc204, 0x9c)
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

// Project-inferred shared player visit operations. Native field list 0x35de
// proves these masks public. Preserve caller-selected masks and player
// identity; ordinary owner-TU placement is provisional.
bool playerData::hasSkeletonVisit(unsigned long visitMask) const
{
    return (m_deadGuyFlags & visitMask) != 0;
}

void playerData::markSkeletonVisited(unsigned long visitMask)
{
    m_deadGuyFlags |= visitMask;
}

bool playerData::hasLeanToVisit(unsigned long visitMask) const
{
    return (m_leanToFlags & visitMask) != 0;
}

void playerData::markLeanToVisited(unsigned long visitMask)
{
    m_leanToFlags |= visitMask;
}

bool playerData::hasMagicSpringVisit(unsigned long visitMask) const
{
    return (m_magicSpringFlags & visitMask) != 0;
}

void playerData::markMagicSpringVisited(unsigned long visitMask)
{
    m_magicSpringFlags |= visitMask;
}

bool playerData::hasMysticalGardenVisit(unsigned long visitMask) const
{
    return (m_mysticalGardenFlags & visitMask) != 0;
}

void playerData::markMysticalGardenVisited(unsigned long visitMask)
{
    m_mysticalGardenFlags |= visitMask;
}

// Project names for the fixed shipyard price. UI text retains its native
// literals; affordability, payment and AI budgets share these numeric values.
// The ordinary helper bodies below have inferred owner-TU placement, without
// native symbol or explicit inline claims.
enum EBoatPurchaseCost {
    BOAT_GOLD_COST = 1000,
    BOAT_WOOD_COST = 10
};

bool playerData::canAffordBoat() const
{
    return m_resources[GOLD] >= BOAT_GOLD_COST
        && m_resources[WOOD] >= BOAT_WOOD_COST;
}

void playerData::payBoatCost()
{
    m_resources[GOLD] -= BOAT_GOLD_COST;
    m_resources[WOOD] -= BOAT_WOOD_COST;
}

void addBoatCost(int* cost)
{
    cost[WOOD] += BOAT_WOOD_COST;
    cost[GOLD] += BOAT_GOLD_COST;
}

// Project-inferred full-row payment shared by quests, building and
// creature/engine purchases. Costs are int rows in quests/traits/buildings
// and long rows from GetUpgradeCost. Keep their types and one subtraction loop without copying
// or reinterpreting either row. Each read remains immediately before its
// matching debit, including zero and negative entries.
template <class Cost>
static void subtractResourceCost(long* resources, const Cost* cost)
{
    for (int resource = 0; resource < NUM_RESOURCES; ++resource)
        resources[resource] -= cost[resource];
}

// Ordinary owner-TU placement is provisional; these interfaces and the
// implementation template do not claim original native helper identities.
void playerData::payResourceCost(const int* cost)
{
    subtractResourceCost(m_resources, cost);
}

void playerData::payResourceCost(const long* cost)
{
    subtractResourceCost(m_resources, cost);
}

VA(0x004b9df0, 0x2D)
DC_ADDRESS(0x0a4cc8, 0x90)
MAC_ADDRESS(0x0cc2a0, 0x5c)
playerData::playerData()
{
}

VA(0x004b9e20, 0x115)
DC_ADDRESS(0x0a4d58, 0x128)
MAC_ADDRESS(0x0cc360, 0x10c)
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
    setComputer();
    m_quickCombat = 0;
    m_placementHelpEnabled = 1;
    m_assembledCombinations.reset();
    // Mac init retains this call at code0+0xcc44c; VC6 expands it here.
    clearNetInfo();
}

VA(0x004b9f40, 0x71)
DC_ADDRESS(0x0a4e80, 0x66)
MAC_ADDRESS(0x0cc46c, 0x90)
bool playerData::hasCapitol()
{
    int i = 0;
    int towns = m_numTowns;

    if (towns <= 0)
        return false;
    do {
        if (g_game->getTown(m_townIds[i])->isCapitol())
            return true;
    } while (++i < towns);
    return false;
}

// Project-inferred operation shared by garrison entry and town hero exchange.
void playerData::clearHiddenHeroSelection(const hero& hiddenHero)
{
    if (m_currHeroId == hiddenHero.m_id) {
        m_currHeroId = -1;
        if (g_netLocalGamePos == hiddenHero.m_owner)
            g_advManager->clearHeroCursor();
    }
}

VA(0x004b9fc0, 0x167)
DC_ADDRESS(0x0a4ee8, 0x1c2)
MAC_ADDRESS(0x0cc4fc, 0x1bc)
unsigned char playerData::addGarrisonHero(town* ourTown)
{
    hero* ourHero;
    int found;

    if (ourTown->m_visitingHeroId < 0)
        return 0;
    if (ourTown->m_garrisonHeroId >= 0)
        return 0;

    ourHero = g_game->getHero(ourTown->m_visitingHeroId);
    if (!ourHero->m_army.merge(&ourTown->getArmy()))
        return 0;

    g_game->recordHideHero(ourHero, ourHero->m_owner, 0);

    if (g_remoteOn) {
        CMCHideHero hideHero(ourHero->m_id);
        sendMapChange(&hideHero);
    }

    found = findHero(ourHero->m_id);
    ourHero->restoreCell();

    removeHeroAt(found);

    clearHiddenHeroSelection(*ourHero);
    ourTown->m_garrisonHeroId = ourHero->m_id;
    ourTown->m_visitingHeroId = -1;
    return 1;
}

// Original: playerData::SetName; game.cpp:1383
// AssignNetInfo expands this ordinary bounded-copy helper in Complete.
DC_ADDRESS(0x0a50ac, 0x5c)
MAC_ADDRESS(0x0cc6b8, 0x28)
void playerData::setName(char* newName)
{
    strncpy(m_name, newName, 20);
}

VA(0x004ba130, 0x34)
DC_ADDRESS(0x0a5108, 0x2e)
MAC_ADDRESS(0x0cc6e0, 0x4c)
void playerData::assignNetInfo(CNetPlayerInfo* netPlayerInfo)
{
    setName(netPlayerInfo->m_name);
    m_dpid = netPlayerInfo->m_dpid;
    m_isHuman = 1;
}

// Original: playerData::GetNetInfo; game.cpp:1395
DC_ADDRESS(0x0a5138, 0x2e)
void playerData::getNetInfo(CNetPlayerInfo* netPlayerInfo)
{
    strcpy(netPlayerInfo->m_name, m_name);
    netPlayerInfo->m_dpid = m_dpid;
}

VA(0x004ba170, 0x4E)
DC_ADDRESS(0x0a5168, 0x48)
MAC_ADDRESS(0x0cc72c, 0x5c)
void playerData::clearNetInfo()
{
    strcpy(m_name, g_generalText->getText(GENERAL_TEXT_DEFAULT_PLAYER_NAME));
    m_dpid = 0;
    setComputer();
}

// Project-inferred map/save scalar operations. Optional unsigned-byte values
// use 0xff for absence; this is not a signed-char conversion and must not be
// applied to required coordinates, raw placeholder IDs or the short-ID reader.
static int decodeOptionalByte(int value)
{
    if (value == 0xff)
        return -1;
    return value;
}

// Keep the format/version gates in the native readers. Portrait IDs use the
// optional-byte decoder but never this legacy hero-roster remap.
static int remapLegacyHeroId(int heroId)
{
    if (heroId == g_savedHeroPre25First)
        return g_heroPre25FirstRemap;
    if (heroId == g_savedHeroPre25Second)
        return g_heroPre25SecondRemap;
    return heroId;
}

VA(0x004ba1c0, 0x50)
MAC_ADDRESS(0x0cc788, 0x78)
int __fastcall readHeroId(TAbstractFile* infile, int mapVersion)
{
    int heroId = decodeOptionalByte(readValue<unsigned char>(infile));
    if (heroId == -1)
        return -1;
    if (mapVersion == g_mapVersionOldCampaignHeroIds)
        return remapLegacyHeroId(heroId);
    return heroId;
}

VA(0x004ba210, 0x50)
MAC_ADDRESS(0x0cc800, 0x78)
int __fastcall loadHeroId(TAbstractFile* infile, int saveVersion)
{
    int heroId = decodeOptionalByte(readValue<unsigned char>(infile));
    if (heroId == -1)
        return -1;
    if (saveVersion < g_saveVersionCompleteHeroRoster)
        return remapLegacyHeroId(heroId);
    return heroId;
}

// Mac 0:0xcc878 retains this short-ID reader immediately after loadHeroId.
// Windows expands its call in NewSMapHeader::loadLossCondition.
MAC_ADDRESS(0x0cc878, 0x74)
static int loadHeroIdShort(TAbstractFile* infile, int saveVersion)
{
    int heroId = readValue<short>(infile);
    if (saveVersion < g_saveVersionCompleteHeroRoster)
        heroId = remapLegacyHeroId(heroId);
    return heroId;
}

// Mac 0xccce0..0xccd60 constructs a local mask, reads two packed bytes,
// expands the decoder and copies the mask through a second temporary into
// the member. The existing readPackedBits return and nested decodePackedBits
// preserve those boundaries; its source name is inferred. Windows stays exact.
VA(0x004ba260, 0x401)
DC_ADDRESS(0x0a51b0, 0x3f6)
MAC_ADDRESS(0x0cc8ec, 0x490)
int playerData::load(TAbstractFile* infile, int saveVersion)
{
    char value;
    if (readValue(infile, value) < sizeof(value))
        return -1;
    m_color = value;
    if (readValue(infile, value) < sizeof(value))
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
    if (readValue(infile, flag) < sizeof(flag))
        return -1;
    m_startingNumHeroes = flag;

    int number;
    if (readValue(infile, number) < sizeof(number))
        return -1;
    m_personality = number;

    if (readValue(infile, value) < sizeof(value))
        return -1;
    m_extraPuzzlePieces = value;

    if (infile->read(&m_puzzleGuess, sizeof(m_puzzleGuess)) < sizeof(m_puzzleGuess))
        return -1;

    if (readValue(infile, value) < sizeof(value))
        return -1;
    m_deathCountDown = value;
    if (readValue(infile, value) < sizeof(value))
        return -1;
    m_numTowns = value;
    if (readValue(infile, value) < sizeof(value))
        return -1;
    m_currTownId = value;

    for (i = 0; i < 0x48; i++) {
        if (readValue(infile, value) < sizeof(value))
            return -1;
        m_townIds[i] = value;
    }

    for (i = 0; i < 7; i++) {
        if (readValue(infile, number) < sizeof(number))
            return -1;
        m_resources[i] = number;
    }

    unsigned long flags;
    if (readValue(infile, flags) < sizeof(flags))
        return -1;
    m_mysticalGardenFlags = flags;
    if (readValue(infile, flags) < sizeof(flags))
        return -1;
    m_magicSpringFlags = flags;
    if (readValue(infile, flags) < sizeof(flags))
        return -1;
    m_deadGuyFlags = flags;
    if (readValue(infile, flags) < sizeof(flags))
        return -1;
    m_leanToFlags = flags;

    if (readValue(infile, flag) < sizeof(flag))
        return -1;
    m_placementHelpEnabled = flag != 0;

    if (saveVersion >= 37) {
        m_assembledCombinations = readPackedBits<12>(infile);
    }
    return 0;
}

// E:\gamedcs\game.cpp:1548
// The write side of load, minus the version remap: a saver only ever
// emits the current format. The tavern pair is written longhand here
// where load loops over it, and the trailing combination-artifact word
// is unconditional.
// DC records four scalar staging buffers besides count/x; Mac separates
// unsigned-long/int slots +0x40/+0x44 and unsigned/signed bytes +0x48/+0x49.
// Native Mac 0xcd1f0..0xcd208 loads the const zero value twice into its
// packed byte output. std::fill_n reproduces those loads; memset instead
// retains an absent CRT call. Native 0xcd220..0xcd234 compounds the integer
// shift directly; an explicit byte cast adds a mask before the OR. These
// paired operations preserve all scalar writers and the bitset test helper.
// Declare the bit counter in its loop after the fill: VC6 then retains the
// native bitset bounds check and reproduces the scalar-buffer homes. The
// remaining 99.9557% residual is the x/uintBuffer stack-home permutation;
// all 49 blocks and 22 calls agree. Outer local-order controls are object-identical.
// With fill_n and the loop-local bit counter, direct member.test(bit) still
// changes the packed-bit update from retail's eight instructions to eleven
// (98.21%). The const pointer retains that expansion; all 22 calls agree.
VA(0x004ba670, 0x36A)
DC_ADDRESS(0x0a55a8, 0x3f0)
MAC_ADDRESS(0x0ccd7c, 0x4fc)  // anchor-global
int playerData::save(TAbstractFile* outfile)
{
    unsigned long uintBuffer;
    int intBuffer;
    int count;
    int x;
    unsigned char ucharBuffer;
    char charBuffer;

    charBuffer = m_color;
    count = writeScalar(outfile, charBuffer);
    if (count < sizeof(char))
        return -1;
    charBuffer = m_numHeroes;
    count = writeScalar(outfile, charBuffer);
    if (count < sizeof(char))
        return -1;
    charBuffer = static_cast<char>(m_currHeroId);
    count = writeScalar(outfile, charBuffer);
    if (count < sizeof(char))
        return -1;

    for (x = 0; x < 8; x++) {
        charBuffer = static_cast<char>(m_heroes[x]);
        count = writeScalar(outfile, charBuffer);
        if (count < sizeof(char))
            return -1;
    }

    charBuffer = static_cast<char>(m_recruits[0]);
    count = writeScalar(outfile, charBuffer);
    if (count < sizeof(char))
        return -1;
    charBuffer = static_cast<char>(m_recruits[1]);
    count = writeScalar(outfile, charBuffer);
    if (count < sizeof(char))
        return -1;

    ucharBuffer = m_startingNumHeroes;
    count = writeScalar(outfile, ucharBuffer);
    if (count < sizeof(unsigned char))
        return -1;

    intBuffer = m_personality;
    count = writeScalar(outfile, intBuffer);
    if (count < sizeof(int))
        return -1;

    charBuffer = m_extraPuzzlePieces;
    count = writeScalar(outfile, charBuffer);
    if (count < sizeof(char))
        return -1;

    count = outfile->write(&m_puzzleGuess, sizeof(m_puzzleGuess));
    if (count < sizeof(m_puzzleGuess))
        return -1;

    charBuffer = m_deathCountDown;
    count = writeScalar(outfile, charBuffer);
    if (count < sizeof(char))
        return -1;
    charBuffer = m_numTowns;
    count = writeScalar(outfile, charBuffer);
    if (count < sizeof(char))
        return -1;
    charBuffer = m_currTownId;
    count = writeScalar(outfile, charBuffer);
    if (count < sizeof(char))
        return -1;

    for (x = 0; x < 0x48; x++) {
        charBuffer = m_townIds[x];
        count = writeScalar(outfile, charBuffer);
        if (count < sizeof(char))
            return -1;
    }

    for (x = 0; x < 7; x++) {
        intBuffer = m_resources[x];
        count = writeScalar(outfile, intBuffer);
        if (count < sizeof(int))
            return -1;
    }

    uintBuffer = m_mysticalGardenFlags;
    count = writeScalar(outfile, uintBuffer);
    if (count < sizeof(unsigned long))
        return -1;
    uintBuffer = m_magicSpringFlags;
    count = writeScalar(outfile, uintBuffer);
    if (count < sizeof(unsigned long))
        return -1;
    uintBuffer = m_deadGuyFlags;
    count = writeScalar(outfile, uintBuffer);
    if (count < sizeof(unsigned long))
        return -1;
    uintBuffer = m_leanToFlags;
    count = writeScalar(outfile, uintBuffer);
    if (count < sizeof(unsigned long))
        return -1;

    ucharBuffer = m_placementHelpEnabled;
    count = writeScalar(outfile, ucharBuffer);
    if (count < sizeof(unsigned char))
        return -1;

    // Mac passes the bitset and index directly to test; no mutable proxy.
    unsigned char bits[2];
    const std::bitset<12>* combinations = &m_assembledCombinations;
    std::fill_n(bits, sizeof(bits), 0);
    for (unsigned int bit = 0; bit < 12; bit++) {
        if (combinations->test(bit))
            bits[bit >> 3] |= 1 << (bit & 7);
    }
    outfile->write(bits, sizeof(bits));
    return 0;
}

// Original: game::LoadPlayerData; game.cpp:1668
// Complete passes the saved version to each playerData::load in the
// expansion at game::load +0x51f. DC's ordinary helper has x and err locals
// and returns the element error; the caller maps a negative result to -1.
DC_ADDRESS(0x0a5998, 0x54)
MAC_ADDRESS(0x0cd278, 0x80)
int game::loadPlayerData(TAbstractFile* infile, int saveVersion)
{
    for (int x = 0; x < 8; ++x) {
        int err = m_players[x].load(infile, saveVersion);
        if (err < 0)
            return err;
    }
    return 0;
}

// Original: game::SavePlayerData; game.cpp:1683
// Ordinary helper expanded at game::save +0x38f; eight player records.
DC_ADDRESS(0x0a59ec, 0x54)
MAC_ADDRESS(0x0cd2f8, 0x70)
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
MAC_ADDRESS(0x0cd368, 0xb4)
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
DC_ADDRESS(0x0a5af8, 0xa4)
MAC_ADDRESS(0x0cd41c, 0xb8)
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

// Original: game::SaveHeroPool; game.cpp:1745
// Ordinary helper expanded at game::save +0x45a. Complete writes all156
// hero records; the older pressing's pool was128.
DC_ADDRESS(0x0a5b9c, 0x56)
MAC_ADDRESS(0x0cd4d4, 0x70)
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

// Original: game::LoadHeroPool; game.cpp:1760
// Complete's game::load +0x622 tests saveVersion against25, chooses128 or
//156 records, and passes that version to hero::load at +0x658.
DC_ADDRESS(0x0a5bf4, 0x56)
MAC_ADDRESS(0x0cd544, 0x7c)
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

VA(0x004ba9e0, 0x2D)
DC_ADDRESS(0x0a5c4c, 0x4a)
MAC_ADDRESS(0x0cd5c0, 0x44)
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

// Project-inferred operations shared by placement, rescue, campaign setup
// and removal. They only maintain the ordered active-hero roster; callers
// retain their capacity/presence guards and map, network and UI work.
void playerData::addHero(int id)
{
    m_heroes[m_numHeroes] = id;
    ++m_numHeroes;
}

void playerData::removeHeroAt(int index)
{
    for (int i = index; i < m_numHeroes - 1; ++i)
        m_heroes[i] = m_heroes[i + 1];
    m_heroes[m_numHeroes - 1] = -1;
    --m_numHeroes;
}

VA(0x004baa10, 0x2E)
DC_ADDRESS(0x0a5c98, 0x76)
MAC_ADDRESS(0x0cd604, 0x48)
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

VA(0x004baa40, 0xFA)
DC_ADDRESS(0x0a5d10, 0xfc)
MAC_ADDRESS(0x0cd64c, 0x128)
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

VA(0x004bab40, 0x43)
DC_ADDRESS(0x0a5e0c, 0xa4)
MAC_ADDRESS(0x0cd774, 0x94)
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

VA(0x004bab90, 0x10)
DC_ADDRESS(0x0a5eb0, 0x38)
MAC_ADDRESS(0x0cd808, 0x2c)
bool playerData::hasMobileHero()
{
    return nextHero() != -1;
}

VA(0x004baba0, 0x29)
DC_ADDRESS(0x0a5ee8, 0x48)
MAC_ADDRESS(0x0cd834, 0x128)
int getNumObelisks(int whichPlayer)
{
    int numFound = 0;

    for (int i = 0; i < 48; i++) {
        if (g_game->m_obeliskFlags[i] & (1 << whichPlayer))
            numFound++;
    }
    return numFound;
}

// Project-inferred query shared by hero morale/luck and their descriptions.
// Keep the native town::hasBuilding path (DC hero.cpp:2989/3149), testing
// included buildings before town type and stopping at the first match.
bool playerData::hasGrailTown(TTownType townType) const
{
    for (int i = 0; i < m_numTowns; ++i) {
        town* ownedTown = g_game->getTown(m_townIds[i]);
        if (ownedTown->hasBuilding(HOLY_GRAIL_ID, true)
            && ownedTown->m_type == townType)
            return true;
    }
    return false;
}

// Original: playerData::BuildingsOwned; game.cpp:1873
DC_ADDRESS(0x0a5f30, 0xbe)
int playerData::buildingsOwned(int townType, int buildingId, int mageLevel)
{
    int count = 0;
    for (int i = 0; i < m_numTowns; ++i) {
        town* currentTown = g_game->getTown(m_townIds[i]);
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

VA(0x004babd0, 0xDC)
DC_ADDRESS(0x0a5ff0, 0x122)
MAC_ADDRESS(0x0cd95c, 0x120)
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

VA(0x004bacb0, 0xCA)
MAC_ADDRESS(0x0cda7c, 0x128)  // hd-crossbuild + anchor-callee
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

VA(0x004bad80, 0x1A)
DC_ADDRESS(0x0a6114, 0x2e)
MAC_ADDRESS(0x0cdba4, 0x4c)
bool playerData::isLocalHuman() const
{
    if (isHuman() && m_isLocal)
        return true;
    return false;
}

// Dreamcast game.cpp:1946 and the retained Mac body place this ordinary
// query after isLocalHuman. Windows retains 35 direct calls to this body;
// Mac has 55 direct references. Both retained bodies normalize the byte.
VA(0x004bada0, 0xC)
DC_ADDRESS(0x0a6144, 0x3c)
MAC_ADDRESS(0x0cdbf0, 0x18)
bool playerData::isHuman() const
{
    if (m_isHuman)
        return true;
    return false;
}

// DC game.cpp:1959 tests IsHuman() twice and Mac 0xcdc1c/0xcdc58 retains
// both isHuman calls.
// The human-name logical zero test keeps this retained body exact and
// restores its first nested expansion in setSpecialRumour. Four natural
// ==0/!strcmp states yield two reproduced objects: with both ==0 tests the
// caller is 85.6147%; with the human !strcmp test it is 100%, with no game
// sibling score changes. Keep this ordinary helper and both isHuman calls.
VA(0x004badb0, 0x9C)
DC_ADDRESS(0x0a6180, 0xb0)
MAC_ADDRESS(0x0cdc08, 0xd0)
char* playerData::getName()
{
    if ((!isHuman() && _strcmpi(m_name, g_generalText->getText(
            GENERAL_TEXT_DEFAULT_PLAYER_NAME)) == 0) ||
        (isHuman() && !_strcmpi(m_name, DATA_COMPGEN(0x00677d30, defaultHumanName, "Player")))) {
        strcpy(m_name, g_colors[m_color]);
    }
    m_name[0] = toupper(m_name[0]);
    return m_name;
}

VA(0x004bae50, 0x1B)
DC_ADDRESS(0x0a6230, 0x44)
MAC_ADDRESS(0x0cdcd8, 0x38)
void playerData::guessGrailLocation(long playerId)
{
    type_point guess = aiAttemptPuzzleGuess(playerId);
    m_puzzleGuess = guess;
}

VA(0x004bae70, 0x55)
DC_ADDRESS(0x0a6274, 0xb4)
MAC_ADDRESS(0x0cdd10, 0x54)
int game::mineTypesOwned(int whichPlayer, int mineType)
{
    int count = 0;

    for (unsigned i = 0; i < m_mines.size(); i++) {
        if (m_mines[i].m_playerOwner == whichPlayer && m_mines[i].m_type == mineType)
            count++;
    }
    return count;
}

VA(0x004baed0, 0x2C)
DC_ADDRESS(0x0a6328, 0x28)
MAC_ADDRESS(0x0cdd64, 0x3c)
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
// not because a CSE failed. The Dreamcast dump names every local here:
// iPiecesRemoved, iExtraPieces, piece, i, j and the two
// floats. `47 - i` is an induction expression, not a variable, which is
// why VC6 carries it as a second down-counter. The fild/fstp/fld
// round-trips on each int->float conversion are /Op rounding, not extra
// source variables.
// Mac's fmadds and Dreamcast's multiply/add show the original fraction as
// (percentObelisksFound * percentObelisksFound + percentObelisksFound) / 2.
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
VA(0x004baf00, 0x25A)
DC_ADDRESS(0x0a6350, 0x272)
MAC_ADDRESS(0x0cdda0, 0x220)  // linkorder
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
        (percentObelisksFound * percentObelisksFound + percentObelisksFound) / 2.0f;
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

    // Dreamcast names SRandom at game.cpp:2051/2052; Mac calls it at
    // 0:0xcdf30/0:0xcdf50. Its VC6 body aliases retail Random at 0x50b230.
    for (i = 0; i < piecesRemoved; i++) {
        piece = 0;
        while (piece < g_puzzlePlaceablePieces && g_puzzlePiecesRemoved[piece])
            piece += sRandom(1, 5);
        if (piece >= g_puzzlePlaceablePieces) {
            j = sRandom(1, g_puzzlePlaceablePieces - i);
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

VA(0x004bb160, 0x7)
DC_ADDRESS(0x0a65c4, 0xe)
MAC_ADDRESS(0x0cdfc0, 0xc)
NewfullMap* game::getWorldMapData()
{
    return &m_worldMap;
}

VA(0x004bb170, 0xD6)
DC_ADDRESS(0x0a65d4, 0xba)
MAC_ADDRESS(0x0cdfcc, 0xa4)
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

VA(0x004bb250, 0x1AA)
DC_ADDRESS(0x0a6690, 0x12c)
MAC_ADDRESS(0x0ce070, 0x158)
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

// Original: game::Scan; game.cpp:2158
DC_ADDRESS(0x0a67bc, 0x94)
int game::scan(signed char* whichList, int start, int length)
{
    for (int i = start; i < start + length; ++i) {
        if (whichList[i] == -1)
            return i;
    }
    return -1;
}

// Original: game::RandomScan; game.cpp:2173
DC_ADDRESS(0x0a6850, 0x86)
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

VA(0x004bb400, 0x1DC)
DC_ADDRESS(0x0a68d8, 0x3fc)
MAC_ADDRESS(0x0ce1c8, 0x1d0)
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
            && m_heroPoolMap[heroIndex][playerPos]
            && (m_heroes[heroIndex].m_heroClass == heroClass1
                || m_heroes[heroIndex].m_heroClass == heroClass2)) {
            heroArray[top++] = heroIndex;
        }
    }

    if (top == 0) {
        for (heroIndex = 0; heroIndex < HERO_COUNT; heroIndex++) {
            if (m_heroAvailability[heroIndex] == -1
                && m_heroPoolMap[heroIndex][playerPos]) {
                heroArray[top++] = heroIndex;
            }
        }
    }

    return heroArray[random(1, top) - 1];
}

// E:\gamedcs\game.cpp:2275
// Mac counterpart 0:0xce398 has five ordered zero-fill/bitset/random calls;
// the 18-class and 156-hero loops identify its full 1932-byte span. Its
// zero-fill is the two-argument bzero ABI. ZeroMemory preserves the ordinary
// Windows SDK zeroing operation and selects only this Mac zero-fill routine;
// nonzero memset fills are untouched. The real shared-header candidate now
// has the same five ordered calls, Mac18.37 -> 27.90%, Windows98.98 flat.
// The deleted declaration view had rewritten all memset fills to bzero, so
// its older agreement was not compiler evidence. std::fill_n emits stores
// and aggregate initialization copies a static72-byte array instead.
// Restoring DC's THeroClass induction local raises CodeWarrior O3 20.96% to
// 27.59% without changing Windows 98.98% or its exact CFG/call sequence. O2
// falls to 10.30%, O4 and swapping array declarations are flat. The Windows
// residual is register homing at the Complete-only Conflux guard. Mac's
// bitset<8>::reference layout stages the pool pointer and index before test;
// restoring operator[] at both sites raises Mac to 539/1932 (27.90%) while
// the focused VC6 build stays 98.98% with the same exact CFG and call order.
// Mac 0xce654..0xce6b8 directly loads the product version, then tests
// video state, alignment and the two Conflux counts in this order. A named
// version snapshot and a separately nested version guard are VC6-flat;
// why-reg v2 also leaves the six scratch-register rows unchanged.
VA(0x004bb5e0, 0x282)
DC_ADDRESS(0x0a6cd4, 0x2fe)
MAC_ADDRESS(0x0ce398, 0x78c)  // anchor-global
int game::getNewHeroId(int playerPos, THeroClass excluded,
                       unsigned char preferAlignment,
                       THeroClass preferredClass)
{
    THeroClass heroClass;
    long totalCount;
    long choice = 0;
    long counts[18];
    // CodeView records THeroID; Complete retains the same signed domain.
    HeroId heroId;
    long weights[18];
    long alignedCount;

    totalCount = 0;

    int alignment;
    if (playerPos >= 0)
        alignment = m_setup.m_alignment[playerPos];
    else
        alignment = -1;

    ZeroMemory(counts, sizeof(counts));
    for (heroClass = classKnight; heroClass < kNumHeroClasses;
         heroClass = THeroClass(heroClass + 1)) {
        weights[heroClass] =
            g_heroClasses[heroClass].m_foundInTownType[alignment];
    }

    for (heroId = HeroId(0); heroId < HERO_COUNT; heroId = HeroId(heroId + 1)) {
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
        && g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_LOW
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
    for (heroId = HeroId(0); heroId < HERO_COUNT; heroId = HeroId(heroId + 1)) {
        if (m_heroAvailability[heroId] == -1
            && (playerPos == -1 || m_heroPoolMap[heroId][playerPos])
            && m_heroes[heroId].m_heroClass == heroClass
            && --choice == 0) {
            return heroId;
        }
    }
    return -1;
}

VA(0x004bb870, 0x89)
DC_ADDRESS(0x0a6fd4, 0xa6)
MAC_ADDRESS(0x0ceb24, 0x5c)
int game::getTownId(int x, int y, int z)
{
    for (unsigned i = 0; i < m_towns.size(); i++) {
        if (m_towns[i].m_mapX == x && m_towns[i].m_mapY == y && m_towns[i].m_mapZ == z)
            return i;
    }
    return -1;
}

// Original: game::GetHeroId; game.cpp:2390
DC_ADDRESS(0x0a707c, 0x90)
int game::getHeroId(type_point heroLocation)
{
    for (int i = 0; i < HERO_COUNT; ++i) {
        if (m_heroes[i].getLocation() == heroLocation)
            return i;
    }
    return -1;
}

// Original GetMineId, game.cpp:2405. The ordinary helper
// scans x/y/z in that order and returns -1 after exhausting the mine pool.
// Complete expands it in RandomizeEvents's LIGHTHOUSE and MINE arms.
DC_ADDRESS(0x0a710c, 0xa6)
MAC_ADDRESS(0x0ceb80, 0x5c)
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

VA(0x004bb900, 0x87)
DC_ADDRESS(0x0a71b4, 0xc2)
MAC_ADDRESS(0x0cebdc, 0x5c)
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
DC_ADDRESS(0x0a7414, 0xf8)
MAC_ADDRESS(0x0cec38, 0x104)
int __fastcall game::loadString(TAbstractFile* infile, std::string& s)
{
    int count;
    short length;

    // Mac 0xcec70 checks the scalar count before decoding the short at
    // 0xcec84; the canonical reader preserves that caller slot and guard.
    count = readLittleEndianValue(infile, length);
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

// Original: game::GetGarrisonId; game.cpp:2433
DC_ADDRESS(0x0a7278, 0xa6)
int game::getGarrisonId(int x, int y, int z)
{
    for (int i = 0; i < m_garrisons.size(); ++i) {
        if (m_garrisons[i].m_mapX == x && m_garrisons[i].m_mapY == y
            && m_garrisons[i].m_mapZ == z)
            return i;
    }
    return -1;
}

// Original: GenerateStandardFileName; game.cpp:2448
// DC retains this ordinary utility without a recorded named caller. Keep its
// source operation without assigning it a retail address or inventing a use.
DC_ADDRESS(0x0a7320, 0xf2)
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

// Function-entry versus block-owned buffer declarations produce identical
// game-unit VC6 objects with the canonical endian writer; this lifetime
// spelling does not explain the retained saveString calls in retail callers.
VA(0x004bbb60, 0xBB)
DC_ADDRESS(0x0a750c, 0xc2)
MAC_ADDRESS(0x0ced3c, 0xf0)
int __fastcall game::saveString(TAbstractFile* outfile, std::string& s)
{
    HOMM3_RELEASE_VERIFY(outfile != 0);
    short length = s.size();
    // Mac 0xced70..0xced8c encodes the owned length before writing it.
    // The canonical writer keeps Windows' direct scalar representation;
    // bypassing its call solely to affect VC6 cost hid this source boundary.
    int count = writeLittleEndianValue(outfile, length);
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

// Native Mac retains saveString at 0xcee68 and 0xcef18; Windows also
// retains both calls. Current C2 admits cb174 at budgets939/723 (flags106),
// then expands the canonical endian/scalar writer chain. Its constructor
// chain instead rejects string::assign(pointer,length), cb69 at budget67.
// Direct versus copy string initialization and function-owned buffer locals
// are object-identical controls; neither explains these compiler decisions.
VA(0x004bbc20, 0x21E)
DC_ADDRESS(0x0a75d0, 0x1f8)
MAC_ADDRESS(0x0cee2c, 0x194)
int game::saveRumours(TAbstractFile* outfile)
{
    int rumourListSize;
    int count;
    TRumour* rit;
    std::string currentRumour(m_currentRumour);
    char boolBuffer;

    count = saveString(outfile, currentRumour);
    if (count < 0)
        return -1;

    count = outfile->write(m_rumourState, sizeof(m_rumourState));
    if (count < sizeof(int))
        return -1;

    rumourListSize = m_rumours.size();
    count = writeScalar(outfile, rumourListSize);
    if (count < sizeof(rumourListSize))
        return -1;

    for (rit = m_rumours.begin(); rit != m_rumours.end(); ++rit) {
        count = saveString(outfile, rit->m_text);
        if (count < 0)
            return -1;
        boolBuffer = rit->m_unavailable;
        count = writeScalar(outfile, boolBuffer);
        if (count < sizeof(boolBuffer))
            return -1;
    }
    return 1;
}

VA(0x004bbe40, 0x294)
DC_ADDRESS(0x0a77c8, 0x192)
MAC_ADDRESS(0x0cefc0, 0x1b8)
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
    if (readValue(infile, count) < sizeof(count))
        return -1;

    m_rumours.resize(count);
    for (TRumour* it = m_rumours.begin(); it != m_rumours.end(); ++it) {
        if (loadString(infile, it->m_text) < 0)
            return -1;
        if (readValue(infile, value) < sizeof(value))
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
VA(0x004bcb30, 0x26C)
DC_ADDRESS(0x0a8144, 0x28a)
MAC_ADDRESS(0x0d02a8, 0x274)  // sole caller game::Load
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
         location.m_z < g_game->getNumMapLevels();
         ++location.m_z) {
        for (location.m_y = 0; location.m_y < g_mapWidth; ++location.m_y) {
            for (location.m_x = 0; location.m_x < g_mapHeight; ++location.m_x) {
                NewmapCell* mapCell = g_game->getCell(location);

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

// E:\gamedcs\game.cpp:2654. Mac retains this helper at 0xcf178;
// Windows expands the call from game::save.
DC_ADDRESS(0x0a795c, 0xc6)
MAC_ADDRESS(0x0cf178, 0xb0)
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

// E:\gamedcs\game.cpp:2672. Mac retains this helper at 0xcf228;
// Windows expands the call from game::load.
// Original LoadBlackMarkets; black_market_list_size -> blackMarketListSize.
// DC calls clear, resize and operator[]. The ordinary helper restores one
// caller cleanup boundary; its natural expansion needs no inline-depth pin.
DC_ADDRESS(0x0a7a24, 0x98)
MAC_ADDRESS(0x0cf228, 0xc8)
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
// DC gives these template emissions one source boundary, a short count,
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
DC_ADDRESS(0x0c184c, 0x80)
DC_ADDRESS(0x0c19e8, 0x80)
DC_ADDRESS(0x0c1a68, 0x80)
DC_ADDRESS(0x0c1ae8, 0x84)
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
// Mac stages a signed-short count before the contiguous element payload.
// Windows writes the low half of an int count directly: this spelling is
// exact for all three retained instances (0x4d2ac0/0x4d2b20/0x4d2b80, the
// short+writeValue form scores 69.26/71.47), and its IL cost keeps the
// retail call boundaries in game::save.
// The guarded return reproduces both retained 96-byte writers exactly,
// including SETAE. Direct boolean/byte-local returns instead use SBB/INC;
// that spelling difference does not refute the DC bool/reference signature.
// The point/long writer, resize, size, _Ucopy and _Ufill instances emit
// identical raw code; retail's folded calls do not require a type adapter.
template <class T>
DC_ADDRESS(0x0c18cc, 0x84)
DC_ADDRESS(0x0c1dd4, 0x84)
DC_ADDRESS(0x0c1e58, 0x84)
DC_ADDRESS(0x0c1edc, 0x88)
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

// Original: load_object_vector; game.cpp:2733.
// Both DC instantiations have the same source rows and reference interface.
// Raw public YA_N proves bool; the primitive type display uses a byte alias.
// Complete uses TAbstractFile in place of the DC gz handle; generator calls
// expand, while the creature-bank instantiation is retained at 0x4d2870.
template <class T>
DC_ADDRESS(0x0c1950, 0x98)
DC_ADDRESS(0x0c1b6c, 0x98)
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

// Original: save_object_vector; game.cpp:2754.
// The matching reference writer preserves each record's own save boundary.
// Both raw public symbols return bool (YA_N), like the record members (QAA_N).
template <class T>
DC_ADDRESS(0x0c1d38, 0x9c)
DC_ADDRESS(0x0c1f64, 0x9c)
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
DC_ADDRESS(0x0a7abc, 0xa4)
MAC_ADDRESS(0x0cf2f0, 0xf0)
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

// Original: type_creature_bank::save; game.cpp:2790
// DC2791..2794 write the four fixed bands without checking each result;
// DC2795 returns save_vector for the artifact tail. Complete expands this
// ordinary member in its retained saveObjectVector<type_creature_bank>.
DC_ADDRESS(0x0a7b60, 0x64)
MAC_ADDRESS(0x0cf3e0, 0xb0)
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
// g_game/global field transfers. Mac retains the source boundary at 0xcffd8;
// its name and free-function binding remain inferred. No Dreamcast identity
// or standalone Windows address is asserted.
MAC_ADDRESS(0x0cffd8, 0x2d0)
void applySavedGameHeader(const SavedGameHeader& saved)
{
    // Every store in this block goes through gpGame, RELOADED from the
    // global for each one, not through the implicit `this` retail
    // already has in a register: `mov ecx,[gpGame] / mov [ecx+0x1f698],
    // edx`, then `mov ecx,[gpGame]` again for mapHeader, again for
    // setup, again for campaign, again for the filename. Do not cache
    // what retail reloads.
    g_game->setGameVersion(saved.m_gameVersion);
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

// Complete places the save-header constructor before its owning game calls.
// Those calls expand it; the selection TU retains the constructor call.
// Mac places it before the save-header reset/save/load and game::load cluster.
// E:\gamedcs\Game.h:1301
VA(0x004bc0e0, 0x251)
DC_ADDRESS(0x0bceb4, 0x4c)
MAC_ADDRESS(0x0cf490, 0x260)
SavedGameHeader::SavedGameHeader()
{
    memset(m_id, 0, sizeof(m_id));
    strcpy(m_id, "H3SVG");
    m_version = 42;
}

// Original: game::Load; game.cpp:3026 Complete loads a
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
VA(0x004bcda0, 0xEC2)
DC_ADDRESS(0x0a83d0, 0x778)
MAC_ADDRESS(0x0d051c, 0x1eac)  // anchor-callee set (4 claimed pool loaders) + 'H3SVG'
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
            // readPackedBits<8> (a returned bitset temporary) keeps
            // decodePackedBits out of line here; retail expands its
            // bitset::reference loop in place.
            std::bitset<8> poolMap;
            unsigned char poolBits[1];
            readValue(infile, poolBits);
            decodePackedBits(poolBits, poolMap);
            m_heroPoolMap[i] = poolMap;
        }
    }

    // These scalar fields mirror game::save, with each read checked before
    // assigning the decoded value. Retail carries four temporaries: a char
    // reused across reads 1-2 and 7-9, a second char for 5-6, a short for
    // 3-4 and a second short for 10-12. Direct virtual reads are an IL-cost
    // fact: staging them through readValue shrinks this caller's /Ob2 budget
    // until ~SavedGameHeader's final expansion and loadVector<type_university>
    // stay out of line (retail FuncInfo 0x64d1a8 has 78 states: the last
    // SCampaign/NewSMapHeader pair is that final inline teardown; 92.81%).
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
    // Retail multiplies by the width first; (W * H) and L * W * H both emit
    // the height first under this TU state.
    int mapExtraSize = (g_mapHeight * g_mapWidth) * g_game->getNumMapLevels();
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
DC_ADDRESS(0x0a8b48, 0x2a)
MAC_ADDRESS(0x0d2508, 0x228)
int SGameSetupOptions::save(TAbstractFile* outfile)
{
    outfile->write(m_color, sizeof(m_color));
    outfile->write(m_color, sizeof(m_handicap));
    outfile->write(m_alignment, sizeof(m_alignment));
    outfile->write(m_playerPos, sizeof(m_playerPos));

    writeValue<char>(outfile, m_difficulty);
    outfile->write(m_filename, sizeof(m_filename));
    outfile->write(m_path, sizeof(m_path));
    outfile->write(m_canFlipFromToComputer, sizeof(m_canFlipFromToComputer));

    writeValue<char>(outfile, m_curSelectedPlayer);
    writeValue<char>(outfile, m_fileInitialized);
    writeValue<char>(outfile, m_initializationNumHumans);
    writeValue<char>(outfile, m_turnDuration);

    for (int i = 0; i < 8; ++i) {
        writeValue<char>(outfile, m_startingHero[i]);
    }

    return outfile->write(m_startingBonus, sizeof(m_startingBonus)) <
                   sizeof(m_startingBonus)
               ? -1
               : 0;
}

VA(0x004be260, 0x188)
MAC_ADDRESS(0x0d2730, 0x224)
int SGameSetupOptions::load(TAbstractFile* infile, int saveVersion)
{
    TAbstractFile* input = infile;
    input->read(m_color, sizeof(m_color));
    input->read(m_color, sizeof(m_handicap));
    input->read(m_alignment, sizeof(m_alignment));
    input->read(m_playerPos, sizeof(m_playerPos));

    m_difficulty = readValue<char>(input);
    input->read(m_filename, sizeof(m_filename));
    input->read(m_path, sizeof(m_path));
    if (saveVersion < 28)
        strcpy(m_path, "maps");
    input->read(m_canFlipFromToComputer, sizeof(m_canFlipFromToComputer));

    m_curSelectedPlayer = readValue<char>(input);
    m_fileInitialized = readValue<char>(input) != 0;
    m_initializationNumHumans = readValue<char>(input);
    m_turnDuration = readValue<char>(input);

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

// DC game.cpp:3311 records a distinct int count at sp+0x20 for
// scalar-write results. Helper-return guards use their own temporary, so keep
// their canonical calls separate from this status local. Complete moves the
// older campaign/header prefix into SavedGameHeader.
// Retail's four narrow scalar staging locals plus zero live at function scope;
// narrowing their lifetimes changes the frame. The hero-pool membership byte
// is an indexed one-byte array, local to the hero loop.
// The scalar write groups own no locals; removing their artificial scopes and
// testing saveVector directly preserves the natural lifetime of saved.
// Residual: VC6 expands both pool-loop saveVector calls that retail retains,
// and the associated failure cleanup differs. Bracing every error return with
// count restored falls from 92.8015 to 90.7320; moving the shared index to
// entry and removing the remaining availability-guard braces are byte-flat.
// Keep the canonical helper calls.
VA(0x004be3f0, 0xAA5)
DC_ADDRESS(0x0a8cd0, 0xcfe)
MAC_ADDRESS(0x0d2954, 0x1ba8)  // SavedGameHeader + write/pool callee sequence
int game::save(TAbstractFile* outfile)
{
    char byteValue;
    unsigned char extraByteValue;
    char charBuffer;
    short shortValue;
    unsigned short extraShortValue;
    int zero;
    int count;
    SavedGameHeader saved;
    saved.reset();
    if (saved.save(outfile) < 0)
        return -1;

    charBuffer = g_grailOwner;
    outfile->write(&charBuffer, sizeof(charBuffer));
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

    count = outfile->write(m_heroAvailability, sizeof(m_heroAvailability));
    if (count < sizeof(m_heroAvailability)) {
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
    byteValue = m_newCampaignStarted;
    count = outfile->write(&byteValue, sizeof(byteValue));
    if (count < sizeof(byteValue))
        return -1;
    byteValue = m_numPlayers;
    count = outfile->write(&byteValue, sizeof(byteValue));
    if (count < sizeof(byteValue))
        return -1;

    shortValue = m_ultimateArtifactX;
    count = outfile->write(&shortValue, sizeof(shortValue));
    if (count < sizeof(shortValue))
        return -1;
    shortValue = m_ultimateArtifactY;
    count = outfile->write(&shortValue, sizeof(shortValue));
    if (count < sizeof(shortValue))
        return -1;

    extraByteValue = m_ultimateArtifactZ;
    count = outfile->write(&extraByteValue, sizeof(extraByteValue));
    if (count < sizeof(extraByteValue))
        return -1;
    extraByteValue = m_ultimateRadius;
    count = outfile->write(&extraByteValue, sizeof(extraByteValue));
    if (count < sizeof(extraByteValue))
        return -1;

    byteValue = m_ultimateArtifactPresent;
    count = outfile->write(&byteValue, sizeof(byteValue));
    if (count < sizeof(byteValue))
        return -1;
    // f_1f698 is an int member and retail writes only its low byte.
    byteValue = static_cast<char>(m_gameVersion);
    count = outfile->write(&byteValue, sizeof(byteValue));
    if (count < sizeof(byteValue))
        return -1;
    byteValue = m_isCheater;
    count = outfile->write(&byteValue, sizeof(byteValue));
    if (count < sizeof(byteValue))
        return -1;

    extraShortValue = m_day;
    count = outfile->write(&extraShortValue, sizeof(extraShortValue));
    if (count < sizeof(extraShortValue))
        return -1;
    extraShortValue = m_week;
    count = outfile->write(&extraShortValue, sizeof(extraShortValue));
    if (count < sizeof(extraShortValue))
        return -1;
    extraShortValue = m_month;
    count = outfile->write(&extraShortValue, sizeof(extraShortValue));
    if (count < sizeof(extraShortValue))
        return -1;

    // Six array writes. The first is retail's own inconsistency: it ASKS
    // for sizeof(field_1f644) == 0x20 and accepts 8. The compare is
    // unsigned (`jae`), so the bound has to be an unsigned expression;
    // sizeof(borderTentVisitFlags) is the one in scope that equals 8.
    // The original expression is not recoverable from the bytes - only
    // its value and its unsignedness are.
    count = outfile->write(m_uniqueSystemId, sizeof(m_uniqueSystemId));
    if (count < sizeof(m_borderTentVisitFlags))
        return -1;
    count = outfile->write(m_marketArtifacts, sizeof(m_marketArtifacts));
    if (count < sizeof(m_marketArtifacts))
        return -1;
    count = outfile->write(m_globalInfoFlags, sizeof(m_globalInfoFlags));
    if (count < sizeof(m_globalInfoFlags))
        return -1;
    count = outfile->write(m_borderTentVisitFlags, sizeof(m_borderTentVisitFlags));
    if (count < sizeof(m_borderTentVisitFlags))
        return -1;
    count = outfile->write(m_cartographerMask, sizeof(m_cartographerMask));
    if (count < sizeof(m_cartographerMask))
        return -1;
    count = outfile->write(m_cartographerFlags, sizeof(m_cartographerFlags));
    if (count < sizeof(m_cartographerFlags))
        return -1;

    zero = 0;
    count = outfile->write(&zero, sizeof(zero));
    if (count < sizeof(zero))
        return -1;

    // The map-extra plane. HasTwoLevels is read through the GLOBAL gpGame,
    // not through this->worldMap, and the *2 is applied LAST - retail's
    // `lea edi,[eax+eax]` follows both imuls. The count is computed once
    // into one local because a virtual call sits between its two uses.
    unsigned int mapExtraBytes =
        g_game->getNumMapLevels() * g_mapHeight * g_mapWidth *
        sizeof(unsigned short);
    count = outfile->write(g_mapExtra, mapExtraBytes);
    if (count < mapExtraBytes)
        return -1;

    // Keep the canonical pool writers; retail retains all seven calls.
    for (i = 0; i < 8; ++i) {
        if (!saveVector(outfile, m_lithPools[i]))
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

// Complete retains calls to these ordinary members in game::save.
// Dreamcast attributes their older definitions to Game.h; the Mac
// build also retains both calls.
// E:\gamedcs\Game.h:1312
VA(0x004bc350, 0x271)
DC_ADDRESS(0x0bcf00, 0x6c)
MAC_ADDRESS(0x0cf6f0, 0x330)  // anchor-caller (game::Save) + layout
void SavedGameHeader::reset()
{
    if (g_inCampaign)
        strcpy(m_id, "H3SVC");
    else
        strcpy(m_id, "H3SVG");

    m_version = 42;
    m_gameVersion = g_game->getGameVersion();

    m_campaign = g_game->m_campaign;

    m_mapHeader = g_game->m_mapHeader;

    m_currentPlayer = g_netLocalGamePos;
    m_mapSetup = g_game->m_setup;
    m_campaignGame = g_inCampaign;
    m_fileName = g_game->m_saveFileName;
    m_difficultyRating = g_game->m_difficultyRating;
    m_numDeadPlayers = g_game->m_numDeadPlayers;
    memcpy(m_deadPlayer, g_game->m_playerDisabled, sizeof(m_deadPlayer));

    int* human = m_humanPlayer;
    for (int i = 0; i < 8; ++i)
        *human++ = g_game->m_players[i].isHuman();
}

// Complete serializes the expanded snapshot through its abstract stream.
// Mac stages each scalar separately before its stream write.
// E:\gamedcs\Game.h:1325
VA(0x004bc5d0, 0x17A)
DC_ADDRESS(0x0bcf6c, 0x78)
MAC_ADDRESS(0x0cfa20, 0x214)  // anchor-layout + game::Save caller
int SavedGameHeader::save(TAbstractFile* outfile)
{
    char fileNameBuffer[0x15f];
    char compatibilityBuffer[32];

    outfile->write(m_id, sizeof(m_id));

    writeValue<int>(outfile, m_version);
    writeValue<int>(outfile, m_gameVersion);

    if (outfile->write(compatibilityBuffer, sizeof(compatibilityBuffer)) <
        sizeof(compatibilityBuffer))
        return -1;

    if (m_mapHeader.save(outfile) < 0)
        return -1;
    if (m_mapSetup.save(outfile) < 0)
        return -1;

    writeValue<short>(outfile, m_campaignGame);
    if (m_campaignGame)
        m_campaign.save(outfile);

    strcpy(fileNameBuffer, m_fileName.c_str());
    outfile->write(fileNameBuffer, sizeof(fileNameBuffer));

    writeValue<short>(outfile, m_difficultyRating);
    writeValue<char>(outfile, m_numDeadPlayers);
    outfile->write(m_deadPlayer, sizeof(m_deadPlayer));
    outfile->write(m_humanPlayer, sizeof(m_humanPlayer));
    writeValue<int>(outfile, m_currentPlayer);

    return 0;
}

VA(0x004beea0, 0x2F6)
DC_ADDRESS(0x0a99d0, 0x4b8)
MAC_ADDRESS(0x0d44fc, 0x2e8)
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
                g_gamesDirectoryPrefix,
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

// Project-inferred whole-array reset. Keep the second launch-path reset after
// setupOrigData and its intervening progress/update work rather than eliding it.
void clearStartingHeroOverrides()
{
    memset(g_startingHeroOverrides, -1, sizeof(g_startingHeroOverrides));
}

// Project-inferred initial Grail search state, shared by construction and
// new-game setup. Digging up the Grail only clears presence and keeps the
// location for the puzzle; it must not use this reset.
void game::resetHolyGrail()
{
    m_ultimateArtifactX = -1;
    m_ultimateArtifactY = -1;
    m_ultimateArtifactZ = -1;
    m_ultimateRadius = 0x7f;
    m_ultimateArtifactPresent = 0;
}

VA(0x004bf1a0, 0x183)
DC_ADDRESS(0x0a9e88, 0x246)
MAC_ADDRESS(0x0d47e4, 0x3fc)
void game::setupOrigData()
{
    int i;

    g_normalVictory = 0;
    g_grailOwner = -1;
    m_difficultyRating = 1;
    g_monthType = 0;
    g_monthTypeExtra = 0;
    g_weekType = 0;
    g_weekTypeExtra = 0;
    m_isCheater = 0;

    strncpy(m_saveFileName, g_generalText->getText(GENERAL_TEXT_NEW_GAME_SAVE_NAME), sizeof(m_saveFileName));
    m_saveFileName[sizeof(m_saveFileName) - 1] = 0;
    MEMSET(m_playerDisabled, 0, sizeof(m_playerDisabled), i);
    clearStartingHeroOverrides();

    resetHolyGrail();
    m_month = 1;
    m_week = 1;
    m_day = 1;

    for (i = 0; i < 8; ++i)
        m_uniqueSystemId[i * sizeof(int)] = 0;

    m_numObelisks = 0;
    advManager* manager = g_advManager;
    manager->m_curHeroMobile = 0;
    MEMSET(m_heroAvailability, -1, sizeof(m_heroAvailability), i);

    std::bitset<8> allPlayers = ~std::bitset<8>();
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
DC_ADDRESS(0x0aa0d0, 0x31e)
MAC_ADDRESS(0x0d4be0, 0x14c)
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
                g_gamesDirectoryPrefix,
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

// DC records townArmy as armyGroup&; Mac 0xd4e30 retains the mutable
// getArmy call. Keep that reference and all army helper calls. Both builds
// subscript m_towns directly: getTown's sentinel guard is absent from
// retail (96.75% -> 99.91%, Mac 23.78% -> 50.00%).
VA(0x004bf570, 0x203)
DC_ADDRESS(0x0aa3f0, 0x308)
MAC_ADDRESS(0x0d4d2c, 0x2d4)
void game::giveTroopsToNeutralTown(int townId)
{
    town* currentTown = &m_towns[townId];
    long weekNumber = getCurrentTurn() / 7;
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
    armyGroup& townArmy = currentTown->getArmy();
    TCreatureType creature;
    TCreatureType upgradedCreature;
    TCreatureType upgradedValue = (g_townDwellingCreatures + TOWN_DWELLING_COUNT)[
        townType * TOWN_DWELLING_SLOTS + monsterLevel];
    creature = g_townDwellingCreatures[
        townType * TOWN_DWELLING_SLOTS + monsterLevel];
    upgradedCreature = upgradedValue;
    if (townArmy.getCreatureTotal(upgradedCreature))
        creature = upgradedCreature;

    long amount = g_creatureTypeTraits[creature].m_growthRate;
    if (!townArmy.canJoin(creature)) {
        long worstArmy = -1;
        long worstValue = g_creatureTypeTraits[creature].m_aiValue * amount;
        for (long slot = 0; slot < armyGroup::ARMY_GROUP_SLOT_COUNT; ++slot) {
            long value = g_creatureTypeTraits[townArmy.m_armies[slot]].m_aiValue
                       * townArmy.m_numTroops[slot];
            if (value < worstValue) {
                worstArmy = slot;
                worstValue = value;
            }
        }
        if (worstArmy < 0)
            return;
        townArmy.dismiss(worstArmy);
    }

    townArmy.add(creature, amount, -1);
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
            if (townArmy.m_armies[slot] == creature)
                townArmy.m_armies[slot] = upgradedCreature;
        }
    }
}

// Original: game::GiveTroopsToNeutralTowns; game.cpp:4029
DC_ADDRESS(0x0aa6f8, 0xe8)
MAC_ADDRESS(0x0d5000, 0x13c)
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

// Project-inferred count over team identifiers, using the native team query.
int game::countHumanTeams() const
{
    int count = 0;
    for (int team = 0; team < 8; ++team) {
        if (isHumanTeam(team))
            ++count;
    }
    return count;
}

// E:\gamedcs\game.cpp:4050
// Mac 0xd5290..0xd53bc tests each campaign/scenario pair in order and
// joins them at one disabled-normal-victory store (0xd53c8). Preserve that
// disjunction rather than a nested per-campaign assignment chain. VC6
// factors the comparisons and raises 91.1630% to 93.9000%. Direct named
// coordinate arguments to the canonical point constructor reach 94.8833%.
// The final valid-town path can return directly at unchanged 90.0315%.
// Full do/for failure scopes lose to 87.4352..87.7111%, and the earlier
// result flag gives 89.2426%; these used break as the failure-scope exit.
// These are limits of the tested scopes, not proof of original gotos.
// The town-loss scope uses continue for either failed team check and a
// return for valid ownership. This removes both remaining joins at 90.0315%
// with the full contribution and all relocation names/addends unchanged.
// The earlier failure scopes used break and do not predict this lowering.
VA(0x004bf780, 0x6E2)
DC_ADDRESS(0x0aa7e0, 0x5c4)
MAC_ADDRESS(0x0d513c, 0x960)  // order-map + whole-function identity
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
        const int campaignNumber = m_campaign.m_currentCampaign;
        if (numLivingPlayers == 1) {
            m_mapHeader.m_victoryCondition.m_allowNormalVictory = 0;
        } else if (g_inCampaign) {
            if ((campaignNumber == GAME_CAMPAIGN_5 && map == GAME_SCENARIO_0)
                || (campaignNumber == GAME_CAMPAIGN_3 && map == GAME_SCENARIO_0)
                || (campaignNumber == GAME_CAMPAIGN_2 && map == GAME_SCENARIO_1)
                || (campaignNumber == GAME_CAMPAIGN_8 && map == GAME_SCENARIO_2)
                || (campaignNumber == GAME_CAMPAIGN_7 && map == GAME_SCENARIO_1)
                || (campaignNumber == GAME_CAMPAIGN_7 && map == GAME_SCENARIO_3)
                || (campaignNumber == GAME_CAMPAIGN_7 && map == GAME_SCENARIO_6)
                || (campaignNumber == GAME_CAMPAIGN_15 && map == GAME_SCENARIO_1)
                || (campaignNumber == GAME_CAMPAIGN_15 && map == GAME_SCENARIO_2)
                || (campaignNumber == GAME_CAMPAIGN_15 && map == GAME_SCENARIO_3)
                || (campaignNumber == GAME_CAMPAIGN_18 && map == GAME_SCENARIO_1)
                || (campaignNumber == GAME_CAMPAIGN_18 && map == GAME_SCENARIO_8)
                || (campaignNumber == GAME_CAMPAIGN_18 && map == GAME_SCENARIO_9)
                || (campaignNumber == GAME_CAMPAIGN_16 && map == GAME_SCENARIO_1)
                || (campaignNumber == GAME_CAMPAIGN_16 && map == GAME_SCENARIO_2)
                || (campaignNumber == GAME_CAMPAIGN_16 && map == GAME_SCENARIO_3)
                || (campaignNumber == GAME_CAMPAIGN_14 && map == GAME_SCENARIO_2)
                || (campaignNumber == GAME_CAMPAIGN_14 && map == GAME_SCENARIO_3)
                || (campaignNumber == GAME_CAMPAIGN_14 && map == GAME_SCENARIO_4))
                m_mapHeader.m_victoryCondition.m_allowNormalVictory = 0;
            else
                m_mapHeader.m_victoryCondition.m_allowNormalVictory = 1;
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
        type_point vcheroLoc(victory.m_heroX, victory.m_heroY,
                            victory.m_heroZ);
        victory.m_heroId = -1;
        for (int i = 0; i < HERO_COUNT; ++i) {
            type_point poolheroLoc = m_heroes[i].getLocation();
            if (vcheroLoc == poolheroLoc) {
                int team = getTeam(m_heroes[i].m_owner);
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
        int team = getTeam(thisTown->m_owner);
        if (team >= 0 && isHumanTeam(team))
            victory.m_type = -1;
    }

    LossConditionStruct& loss = m_mapHeader.m_lossCondition;
    if (loss.m_type == LOSS_CONDITION_LOSE_HERO) {
        type_point lcheroLoc(loss.m_heroX, loss.m_heroY, loss.m_heroZ);
        loss.m_heroId = -1;
        for (int i = 0; i < HERO_COUNT; ++i) {
            type_point poolheroLoc = m_heroes[i].getLocation();
            if (lcheroLoc == poolheroLoc) {
                int numHumanTeams = countHumanTeams();
                if (numHumanTeams <= 1) {
                    if (!isComputerTeam(getTeam(m_heroes[i].m_owner))) {
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
        int numHumanTeams;
        int owner;
        int townTeam;
        numHumanTeams = countHumanTeams();
        do {
            if (numHumanTeams > 1)
                continue;
            owner = thisTown->m_owner;
            townTeam = getTeam(owner);
            if (isComputerTeam(townTeam))
                continue;
            if (owner != -1)
                return;
        } while (0);
        loss.m_type = -1;
    }
}

// E:\gamedcs\game.cpp:4236
// DC hasHero is an unsigned byte. Mac retains separate resource-bonus
// arms in town enumeration order; restoring that order gives VC6 EXACT.
// The Mac comparison remains nonexact, with all helper paths retained.
VA(0x004bfe70, 0x6A8)
DC_ADDRESS(0x0aada4, 0xb2a)
MAC_ADDRESS(0x0d5a9c, 0x8b4)
void game::newMap(TAbstractFile* mapFile, int* playerHeroFaces,
                  TCampaignBrief::ScenarioStruct* campaignContext, int gameVersion)
{
    g_inSetup = 1;

    randomizeHeroPool();

    m_numPlayers = 8;
    m_numDeadPlayers = 0;
    if (gameVersion == -1 || g_inCampaign) {
        m_gameVersion = 2;
        if (g_inCampaign) {
            if (m_campaign.m_currentCampaign < g_firstArmageddonsBladeCampaign)
                m_gameVersion = 0;
            else if (m_campaign.m_currentCampaign < 13)
                m_gameVersion = 1;
        }
    } else {
        m_gameVersion = gameVersion;
    }

    if (playerHeroFaces != NULL) {
        for (int facePlayer = 0; facePlayer < 8; ++facePlayer) {
            // Mac retains playerData::isHuman here and in the setup loop.
            if (m_players[facePlayer].isHuman()
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

    // Mac expands the constant-reference fill; VC6 folds its eight stores.
    std::fill_n(m_playerDisabled, 8, false);
    for (int disabledPlayer = 0; disabledPlayer < 8; ++disabledPlayer)
        m_playerDisabled[disabledPlayer] =
            m_players[disabledPlayer].m_numHeroes == 0
            && m_players[disabledPlayer].m_numTowns == 0;

    m_week = 1;
    setRecruits();

    validateVictoryLossConditions(1);

    if (g_inCampaign && m_campaign.m_currentCampaign == GAME_CAMPAIGN_14) {
        hero* campaignHero = getHero(45);
        if (campaignHero->getArtifact(TArtifactSlot(hero::EQUIPPED_SLOT_SPELLBOOK)).m_artifactId
            != -1)
            campaignHero->removeArtifact(hero::EQUIPPED_SLOT_SPELLBOOK);
        if (m_campaign.m_currentMap == GAME_SCENARIO_2) {
            type_artifact alliance(ARTIFACT_ANGELIC_ALLIANCE);
            campaignHero->giveArtifact(alliance, 0, 0);
        }
    }

    for (int setupPlayer = 0; setupPlayer < 8; ++setupPlayer) {
        if (m_playerDisabled[setupPlayer])
            continue;

        if (m_players[setupPlayer].isHuman()) {
            m_players[setupPlayer].m_personality = 3;
            memcpy(m_players[setupPlayer].m_resources,
                   g_initResourcesHuman[m_setup.m_difficulty],
                   sizeof(m_players[setupPlayer].m_resources));
            if (m_isTutorial)
                memcpy(m_players[setupPlayer].m_resources,
                       g_tutorialStartingResources,
                       sizeof(m_players[setupPlayer].m_resources));
        } else {
            m_players[setupPlayer].m_personality = random(0, 2);
            memcpy(m_players[setupPlayer].m_resources,
                   g_initResourcesComputer[m_setup.m_difficulty],
                   sizeof(m_players[setupPlayer].m_resources));
        }

        if (!g_inCampaign) {
            int bonus = g_newMapStartingBonus[setupPlayer];
            unsigned char hasHero = true;
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
                // Mac exits before the retained getHero sentinel guard.
                if (heroId == -1)
                    break;
                hero* bonusHero = getHero(heroId);
                if (bonusHero != NULL) {
                    type_artifact artifact(getRandomArtifactId(2));
                    bonusHero->giveArtifact(artifact, 1, 1);
                }
                break;
            }
            case NEW_MAP_BONUS_GOLD:
                m_players[setupPlayer].m_resources[GOLD] += random(5, 10) * 100;
                break;
            case NEW_MAP_BONUS_RESOURCE: {
                int amount = random(3, 6);
                switch (m_setup.m_alignment[setupPlayer]) {
                // Mac retains a separate random call in each of these four
                // town arms at 0:0xd6188/0xd61f0/0xd6230/0xd625c.
                case TOWN_CASTLE: {
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
                case TOWN_NECROPOLIS: {
                    amount = random(5, 10);
                    m_players[setupPlayer].m_resources[WOOD] += amount;
                    m_players[setupPlayer].m_resources[ORE] += amount;
                    break;
                }
                case TOWN_DUNGEON:
                    m_players[setupPlayer].m_resources[SULFUR] += amount;
                    break;
                case TOWN_STRONGHOLD: {
                    amount = random(5, 10);
                    m_players[setupPlayer].m_resources[WOOD] += amount;
                    m_players[setupPlayer].m_resources[ORE] += amount;
                    break;
                }
                case TOWN_FORTRESS: {
                    amount = random(5, 10);
                    m_players[setupPlayer].m_resources[WOOD] += amount;
                    m_players[setupPlayer].m_resources[ORE] += amount;
                    break;
                }
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
VA(0x004c0520, 0x106)
MAC_ADDRESS(0x0d6350, 0xc0)  // anchor-callers + contiguous catch funclets, retail-only
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

// Original: SetupFirstPlayer; game.cpp:4474
// Complete makes this a game member: 0x4c0632 saves incoming ECX and
// indexes the player array through that receiver throughout the body.
VA(0x004c0630, 0xB1)
DC_ADDRESS(0x0ab8d0, 0x9a)
MAC_ADDRESS(0x0d6410, 0xbc)
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
DC_ADDRESS(0x0ab96c, 0x66)
MAC_ADDRESS(0x0d64cc, 0xb4)
static void randomizeScholar(NewmapCell* cell)
{
    if (cell->getScholarAward() != const_scholar_primary_skill) {
        // DC game.cpp:4515 keeps Random inside the SetScholar expression.
        cell->setScholar(cell->getScholarAward(),
            TPrimarySkill(random(0, 3)),
            cell->getScholarSecondarySkill(), cell->getScholarSpell());
    }
}

// Complete's ARTIFACT arm at 0x4c0cc0 retains the customization test,
// Random call and low-nibble clear; the DC guarded-artifact machinery is absent.
// E:\gamedcs\game.cpp:4524
DC_ADDRESS(0x0ab9d4, 0x2c8)
MAC_ADDRESS(0x0d6580, 0x50)
static void randomizeArtifact(NewmapCell* cell)
{
    if (!cell->isCustomized()) {
        random(0, 99);
        cell->m_extraInfo &= 0xfffffff0;
    }
}

// Mac code 0:0xd65d0 retains this helper between randomizeArtifact and
// randomizeSeaChest. Calls at 0:0xd7dfc and 0:0xdf9a4 come from
// randomizeEvents and perWeek; the shared body stores creature and growth.
MAC_ADDRESS(0x0d65d0, 0x5c)
static void randomizeRefugeeCamp(NewmapCell* cell)
{
    TCreatureType creature = g_game->getRandomMonster(0, 6);
    cell->m_objectIndex = creature;
    cell->m_extraInfo = g_creatureTypeTraits[creature].m_growthRate;
}

// Project-inferred operation shared by the five bank cases in randomizeEvents.
// Keep the packed cell writes before bank construction, then append a copy and
// destroy the local bank before returning. Ordinary source placement is
// provisional; the nested initializeCreatureBank keeps its native boundary.
void game::addCreatureBank(NewmapCell* cell, type_creature_bank_type type)
{
    cell->clearVisitedBits();
    cell->m_creatureBankInfo.m_index = m_creatureBanks.size();
    cell->setCreatureBankEmpty(false);
    type_creature_bank bank;
    initializeCreatureBank(bank, type);
    m_creatureBanks.push_back(bank);
}

// E:\gamedcs\game.cpp:4613.
DC_ADDRESS(0x0abc9c, 0xb0)
MAC_ADDRESS(0x0d662c, 0x9c)
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

// Mac 0:d66f8 calls the retained level overload at 0:e07f4. Its three
// randomizeEvents callers pass levels 1, 2 and 3 at d7f60/d7f70/d7f80.
DC_ADDRESS(0x0abd4c, 0x5c)
MAC_ADDRESS(0x0d66c8, 0x64)
static void randomizeShrine(NewmapCell* cell, const int level)
{
    SpellID spell = cell->getShrineSpell();
    if (spell == -1) {
        spell = g_game->getRandomSpell(level);
        cell->m_shrineInfo.m_spell = spell;
    }
    cell->clearVisitedBits();
}

// E:\gamedcs\game.cpp:4654
// RandomizeEvents expands this ordinary static helper.
DC_ADDRESS(0x0abda8, 0x86)
MAC_ADDRESS(0x0d672c, 0x118)
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
// E:\gamedcs\game.cpp:4681
DC_ADDRESS(0x0abe30, 0x5e)
MAC_ADDRESS(0x0d6844, 0x60)
static void randomizeWiseTree(short id, NewmapCell* cell)
{
    cell->m_extraInfo = (cell->m_extraInfo & 0xffffffe0) | (id & 0x1f);
    cell->clearVisitedBits();
    int price = random(0, 2);
    cell->m_extraInfo = (cell->m_extraInfo & 0xffff1fff) | ((price & 7) << 13);
}

// E:\gamedcs\game.cpp:4691.
DC_ADDRESS(0x0abe90, 0xe6)
MAC_ADDRESS(0x0d68a4, 0xe8)
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
// E:\gamedcs\game.cpp:4724
DC_ADDRESS(0x0abf78, 0x6e)
MAC_ADDRESS(0x0d698c, 0xa0)
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
    cell->setTomb(g_game->getRandomArtifactId(level));
}

// E:\gamedcs\game.cpp:4753. The vector local and its teardown belong to the
// inlined source helper; retail calls only the packed pyramid setter.
// Ownership probe: the MapCell.h body at 0x4c2330 is currently fully
// expanded here. Replacing this helper's forced-inline spelling with ordinary static
// did not recover the retained call; the fatal header-emission gate remains.
DC_ADDRESS(0x0abfe8, 0x60)
MAC_ADDRESS(0x0d6a2c, 0x118)
static void randomizePyramid(NewmapCell* cell)
{
    std::vector<int> possibleSpells;
    int i;
    for (i = 0; i < 70; ++i) {
        if (g_spellTraits[i].m_school != const_invalid_school
            && g_spellTraits[i].m_level == g_pyramidSpellLevel
            && !g_game->m_spellDisabledInfo[i])
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
VA(0x004c06f0, 0x179)
DC_ADDRESS(0x0ac048, 0x11e)
MAC_ADDRESS(0x0d6b44, 0x13c)  // dc-order + member receiver
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
            if (!availableSkills[skill]) {
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
        availableSkills[skill] = false;
        --availableCount;
    }

    const unsigned long universityIndexBits = 0x01ffe000;
    cell->clearVisitedBits();
    unsigned long universityIndex = m_universities.size() & 0xfff;
    cell->m_extraInfo = (cell->m_extraInfo & ~universityIndexBits)
        | (universityIndex << 13);
    m_universities.push_back(university);
}

// E:\gamedcs\game.cpp:4804. Like the shrine helper, this has no PC row:
// /Ob2 expands it into RandomizeEvents while leaving bitset's non-trivial
// operations as calls. DC proves the TSecondarySkill local but predates
// Complete's mask filtering; retail's proxy-call sequence selects operator[].
// Mac d6d98..d6dd4 expands setWitchSkill separately in the selected-skill
// and no-available-skill branches. Keep both source calls at those boundaries.
DC_ADDRESS(0x0ac168, 0x3a)
MAC_ADDRESS(0x0d6c80, 0x16c)
static void randomizeWitchHut(NewmapCell* cell)
{
    std::bitset<28> possibleSkills(cell->m_extraInfo);
    cell->m_extraInfo = 0;
    if (possibleSkills.none())
        possibleSkills = ~std::bitset<28>();

    int i;
    for (i = 0; i < 28; ++i)
        possibleSkills[i] = possibleSkills[i]
            && !g_game->m_ssDisabled[i];

    TSecondarySkill skill;
    int count = possibleSkills.count();
    if (count < 1) {
        cell->setWitchSkill(eSecSkillNone);
    }
    else {
        int choice = random(1, count);
        for (skill = eSecSkillPathfinding; skill < kNumSecSkills;
             skill = TSecondarySkill(skill + 1)) {
            if (possibleSkills[skill] && --choice < 1)
                break;
        }
        cell->setWitchSkill(skill);
    }
}

VA(0x004c0870, 0x22A)
DC_ADDRESS(0x0ac1a4, 0x2ee)
MAC_ADDRESS(0x0d6dec, 0x270)
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
        m_ultimateArtifactZ = random(1, getNumMapLevels()) - 1;
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

    for (int z = 0; z < getNumMapLevels(); ++z) {
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

VA(0x004c0aa0, 0xBE)
DC_ADDRESS(0x0ac494, 0x1a6)
MAC_ADDRESS(0x0d705c, 0x104)
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
                    m_artifactUsed[tempCell->getArtifactIndex()] = 1;
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
// preceding/following BLR/prologue bound the admitted Mac target. The shared
// O3 candidate retains the MathLib sqrt call, but its ordinary type_point copy
// uses two halfword stores where Mac retail uses one word store; Mac remains
// non-exact (23.28%). Windows VC6 matches all 0x160 retail bytes when the z
// equality names exitPoint first and the squared y term precedes squared x.
// E:\\gamedcs\\game.cpp:4950
VA(0x004c0b60, 0x160)
DC_ADDRESS(0x0ac63c, 0x2d4)
MAC_ADDRESS(0x0d7160, 0x180)  // dc-order + NewMap caller
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
            if (exitPoint.m_z == currentGate.m_z)
                continue;
            distance = static_cast<long>(sqrt(static_cast<double>(
                (currentGate.m_y - exitPoint.m_y)
                    * (currentGate.m_y - exitPoint.m_y)
                + (currentGate.m_x - exitPoint.m_x)
                    * (currentGate.m_x - exitPoint.m_x))));
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

// Historical probes below predate the recovered helper calls and removal of
// all inline-depth controls; their scores describe that older source model.
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
// That model's two remaining real call divergences were over-expansions:
// retail CALLS ExtraInfoUnion::SetWagon(EGameResource,
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
// DC records int new_owner; Mac 0xd7f08..0xd7f50 widens the saved byte
// before passing it to claimShipyard. Keep that procedure local and the
// point constructors as call-argument temporaries at the recorded push_back
// and claim sites; all canonical helper calls remain intact. Mac 0xd7dfc
// calls randomizeRefugeeCamp, so the refugee-camp arm uses that shared helper
// (80.00%; 84.79% with its body written in place).
// DC 5349 calls cell(x-1,y,z) separately for the type and trigger tests,
// then 5351 calls it again for extraInfo. Retail 0x4c17d0..0x4c1846
// repeats the address calculation; a cached left pointer erases those
// source calls. Restore the compound guard and its else setter/increment:
// 80.0034 -> 87.8205 in two reproduced source-family states, with no
// sibling MAX loss. Mac 0xd7c10..0xd7c78 combines the three calculations.
// DC 5121..5123 and Mac 0xd75b4..0xd75d4 keep bank visits, index and
// empty as three field operations. The RESOURCE arm likewise dispatches
// type before its two quantity guards (DC 5439..5455, Mac 0xd7e04..0xd7e78).
// Restoring those typed operations together reaches 89.2299%; the five
// distinct bank lifetimes and all canonical helper calls remain intact.
// DC 5008..5027 proves counter initialization order through the named
// stack homes (Mac d730c..d739c retains the live counters). The CodeView
// local listing does not establish declaration order. ExtraInfoUnion is
// NewmapCell's canonical base: use its inherited setters directly, as in
// DC ad278..ad280, including within scholar/shrine/tomb/witch helpers.
// DC 5127 passes the bank type field directly without a conversion scope.
// These combined recoveries reach 89.6121% from 89.2441% in reproduced
// source families, preserving every canonical body/call and sibling MAX.
VA(0x004c0cc0, 0x1668)
DC_ADDRESS(0x0ac910, 0x1278)
MAC_ADDRESS(0x0d72e0, 0xebc)  // NewMap caller + dc order
void game::randomizeEvents()
{
    unsigned long numTrainingGround = 0;
    unsigned long numDefenseTower = 0;
    unsigned long numGardenOfRevelation = 0;
    unsigned long numMercCamp = 0;
    unsigned long numPowerSchool = 0;
    unsigned long numTreeOfKnowledge = 0;
    unsigned long numLibrary = 0;
    unsigned long numArena = 0;
    unsigned long numMagicSchool = 0;
    unsigned long numWarSchool = 0;
    unsigned long numMysticalGarden = 0;
    unsigned long numMagicSpring = 0;
    unsigned long numDeadGuy = 0;
    unsigned long numLeanTo = 0;
    unsigned long numWarriorTomb = 0;
    unsigned long numLithOneWay = 0;
    unsigned long numLithTwoWay = 0;
    unsigned long numWhirlpool = 0;
    int x;
    int i;
    int y;
    NewmapCell* tempCell;
    int z;
    int id;
    TBlackMarket thisMarket;
    unsigned char resQty;
    EGameResource resType;
    int newOwner;
    NewmapCell::TObjectCell* thisObj;

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
                    addCreatureBank(tempCell,
                                    type_creature_bank_type(tempCell->m_objectIndex));
                    break;

                case CREATURE_GENERATOR_1:
                    id = getGeneratorId(x, y, z);
                    claimGenerator(id, m_generators[id].getOwner());
                    break;

                case g_retailCreatureGenerator2:
                    sprintf(g_text,
                            DATA_COMPGEN(0x00677e38, invalidCreatureGenerator2Format,
                                "CREATURE_GENERATOR_2 found at X:%03d - Y: %03d - Z: %03d\nPlease replace it with the appropriate single generator."),
                            x, y, z);
                    MessageBoxA(g_hwndApp, g_text,
                        DATA_COMPGEN(0x00677e24, invalidCreatureGeneratorTitle,
                            "Invalid Generator"), 0);
                    break;

                case g_retailCreatureGenerator3:
                    sprintf(g_text,
                            DATA_COMPGEN(0x00677db0, invalidCreatureGenerator3Format,
                                "CREATURE_GENERATOR_3 found at X:%03d - Y: %03d - Z: %03d\nPlease replace it with the appropriate single generator."),
                            x, y, z);
                    MessageBoxA(g_hwndApp, g_text,
                        DATA_COMPGEN(0x00677e24, invalidCreatureGeneratorTitle,
                            "Invalid Generator"), 0);
                    break;

                case CREATURE_GENERATOR_4:
                    id = getGeneratorId(x, y, z);
                    claimGenerator(id, m_generators[id].getOwner());
                    break;

                case DEAD_GUY:
                    {
                        if (random(0, 99) < 20)
                            tempCell->setSkeleton(numDeadGuy++, true,
                                                  getRandomArtifactId(6));
                        else
                            tempCell->setSkeleton(numDeadGuy++, false, -1);
                    }
                    break;

                case DEFENSE_TOWER:
                    tempCell->m_extraInfo = numDefenseTower++;
                    break;

                case DERELICT_SHIP:
                    addCreatureBank(tempCell,
                                    CREATURE_BANK_DERELICT);
                    break;

                case SEPULCHER:
                    addCreatureBank(tempCell,
                                    CREATURE_BANK_SEPULCHER);
                    break;

                case SHIPWRECK:
                    addCreatureBank(tempCell,
                                    CREATURE_BANK_SHIPWRECK);
                    break;

                case DRAGON_CITY:
                    addCreatureBank(tempCell,
                                    CREATURE_BANK_DRAGON);
                    break;

                case FLOTSAM:
                    tempCell->m_extraInfo = random(0, 3);
                    break;

                case FOUNTAIN_OF_FORTUNE:
                    {
                        tempCell->randomizeFountainLuck();
                        tempCell->clearVisitedBits();
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
                        {
                            resType = EGameResource(random(0, 5));
                        }
                        resQty = static_cast<unsigned char>(random(1, 5));
                        tempCell->setLeanTo(numLeanTo++, resQty, resType);
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
                        pool->push_back(type_point(x, y, z));
                    }
                    break;

                case UNDERGROUND_GATE:
                    {
                        tempCell->m_extraInfo = m_undergroundGateExits.size();
                        m_undergroundGateExits.push_back(type_point(x, y, z));
                        m_undergroundGatePairs.push_back(-1);
                    }
                    break;

                case LITH_TWOWAY:
                    {
                        std::vector<type_point>* pool =
                            &m_lithPools[tempCell->m_objectIndex];
                        tempCell->m_extraInfo = pool->size();
                        pool->push_back(type_point(x, y, z));
                    }
                    break;

                case MAGIC_SCHOOL:
                    tempCell->m_extraInfo = numMagicSchool++;
                    break;

                case MAGIC_SPRING:
                    if (x > 0
                            && m_worldMap.cell(x - 1, y, z)->m_type == MAGIC_SPRING
                            && m_worldMap.cell(x - 1, y, z)->m_isTrigger) {
                        tempCell->m_extraInfo =
                            m_worldMap.cell(x - 1, y, z)->m_extraInfo;
                    }
                    else {
                        tempCell->setMagicSpring(numMagicSpring, 1);
                        ++numMagicSpring;
                    }
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
                    tempCell->setGarden(numMysticalGarden,
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
                    randomizeRefugeeCamp(tempCell);
                    break;

                case RESOURCE:
                    // DC 5439 dispatches the resource type before the
                    // separate quantity tests at 5444 and 5453. Mac
                    // 0xd7e04..0xd7e78 retains those same two write arms.
                    switch (tempCell->m_objectIndex) {
                    case WOOD:
                    case ORE:
                    case GOLD:
                        if (tempCell->m_customResourceInfo.m_qty == 0)
                            tempCell->m_customResourceInfo.m_qty = random(5, 10);
                        break;
                    default:
                        if (tempCell->m_customResourceInfo.m_qty == 0)
                            tempCell->m_customResourceInfo.m_qty = random(3, 6);
                        break;
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
                        // DC 5497/5501/5502 tests the owner before saving it
                        // and resetting that field. Mac 0xd7f08..0xd7f20 agrees.
                        if (tempCell->m_shipyardInfo.m_owner != -1) {
                            newOwner = tempCell->m_shipyardInfo.m_owner;
                            tempCell->m_shipyardInfo.m_owner = -1;
                            claimShipyard(type_point(x, y, z), newOwner);
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
                        // DC 5558..5559; retail combines both operations
                        // into (extraInfo & 0xffffe001) | 1.
                        tempCell->setWheelGold(500);
                        tempCell->clearVisitedBits();
                    }
                    break;

                case WHIRLPOOL:
                    thisObj = &tempCell->m_objects[0];
                    // DC 5567/5573 and Mac 0xd8034..0xd8098 use the
                    // signed cell coordinates, including the (2, 1) trigger.
                    if (thisObj->m_cellX == g_whirlpoolTriggerXOffset
                        && thisObj->m_cellY == g_whirlpoolTriggerYOffset) {
                        tempCell->m_extraInfo = numWhirlpool++;
                    }
                    else {
                        tempCell->m_extraInfo = m_worldMap.cell(
                            x + thisObj->m_cellX - 2,
                            y + thisObj->m_cellY - 1, z)->m_extraInfo;
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
                        resQty = static_cast<unsigned char>(random(3, 6));
                        {
                            resType = EGameResource(random(1, 5));
                        }
                        // DC 5582..5583; retail's 0xfffe001f mask also
                        // clears the visited-player lane, not just amount.
                        tempCell->setWindmill(resType, resQty);
                        tempCell->clearVisitedBits();
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
// The spell-processing index is signed: Mac d8a4c uses cmpwi r27,70,
// and retail's outer spell-stride latch (+0x439, target B51) uses jl at
// 0x2530. The packed decoder instead uses cmplwi/jb and stays unsigned.
// Restoring the outer int index raises Windows 82.7736 -> 82.8580;
// sibling Windows CUR and all available Mac scores are unchanged.
// The older DC filename loader has no spell-plane counterpart; its rumour
// string lifetime and later pool clear calls already agree with this source.
VA(0x004c2450, 0x88E)
DC_ADDRESS(0x0adb88, 0x3b0)
MAC_ADDRESS(0x0d82cc, 0xbcc)  // sole NewMap caller + full stream/callee sequence
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
        if (m_gameVersion < 1)
            artifact = 127;
        else
            artifact = 129;
        memset(m_artifactDisabled + artifact, 1,
               sizeof(m_artifactDisabled) - artifact);
    }

    if (m_mapHeader.m_version != MAP_FORMAT_RESTORATION_OF_ERATHIA) {
        std::bitset<144> disabledArtifacts;
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
        std::bitset<70> serializedSpells = readPackedBits<70>(mapFile);

        for (int spell = 0; spell < hero::NUM_SPELLS; ++spell) {
            if (serializedSpells[spell]) {
                for (artifact = 0; artifact < 144; ++artifact) {
                    if (g_artifactTraits[artifact].m_givesSpells) {
                        m_artifactDisabled[artifact] =
                            m_artifactDisabled[artifact]
                            || markArtifactSpells(artifact)[spell];
                    }
                }
            }
            m_spellDisabledInfo[spell] =
                serializedSpells[spell]
                || (g_spellTraits[spell].m_flags & 0x2000) != 0;
        }

        std::bitset<28> serializedSkills = readPackedBits<28>(mapFile);
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
    if (readLittleEndianValue(mapFile, rumourListSize)
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
// for this Complete-only reader. Mac uses lwbrx for experience/skill count
// and lhbrx plus extsh for artifact IDs/backpack count; retain those scalar
// reader semantics through the shared little-endian helper. Windows is exact.
VA(0x004c2ce0, 0x3A8)
MAC_ADDRESS(0x0d8ec0, 0x460)  // sole caller LoadMap + HeroExtra field-offset walk
void game::readMapHeroSetups(TAbstractFile* mapFile, int mapVersion)
{
    for (int heroId = 0; heroId < HERO_COUNT; ++heroId) {
        HeroExtra* heroRecord = &m_heroSetup[heroId];

        if (!readValue<char>(mapFile))
            continue;

        if (readValue<char>(mapFile)) {
            heroRecord->m_customExperience = 1;
            heroRecord->m_experience = readLittleEndianValue<int>(mapFile);
        }

        if (readValue<char>(mapFile)) {
            heroRecord->m_customSecondarySkills = 1;
            heroRecord->m_numSecondarySkills = readLittleEndianValue<int>(mapFile);
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
                    type_artifact(static_cast<TArtifact>(readLittleEndianValue<short>(mapFile)) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */);
            }

            heroRecord->m_numInBackpack =
                static_cast<unsigned char>(readLittleEndianValue<short>(mapFile));
            for (int carried = 0;
                 carried < heroRecord->m_numInBackpack; ++carried) {
                heroRecord->m_backpack[carried] =
                    // Complete map input stores a signed 16-bit artifact ordinal; the in-memory record retains DC's TArtifact constructor.
                    type_artifact(static_cast<TArtifact>(readLittleEndianValue<short>(mapFile)) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */);
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
            decodeMapBits(spellMask, heroRecord->m_spells);
        }

        if (readValue<char>(mapFile)) {
            heroRecord->m_customPrimarySkills = 1;
            for (int skill = 0; skill < 4; ++skill) {
                heroRecord->m_primarySkills[skill] = readValue<char>(mapFile);
            }
        }
    }
}

// Project-inferred player-header operations, shared by construction and
// both readers. Clearing customization retains the hero selection and the
// unused bytes after the string terminator. The census preserves byte counts
// and counts a human-only slot toward the minimum human requirement.
void CMapHeaderData::TPlayerSlotAttributes::clearHeroCustomization()
{
    m_nonRandomHeroCustomPortrait = -1;
    m_nonRandomHeroCustomName[0] = 0;
}

void CMapHeaderData::clearPlayerCounts()
{
    m_numPlayers = 0;
    m_minNumHumanPlayers = 0;
    m_maxNumHumanPlayers = 0;
}

void CMapHeaderData::countPlayerSlot(const TPlayerSlotAttributes& slot)
{
    if (slot.m_canBeHuman && !slot.m_canBeComputer)
        ++m_minNumHumanPlayers;
    if (slot.m_canBeHuman)
        ++m_maxNumHumanPlayers;
    if (slot.m_canBeComputer || slot.m_canBeHuman)
        ++m_numPlayers;
}

// Project-inferred shared team payload operation. A zero team count consumes
// no payload and gives each player its own team; otherwise preserve the exact
// short-read check on the signed-byte array and its partial-write behavior.
bool CMapHeaderData::readTeamAssignments(TAbstractFile* infile)
{
    if (m_numTeams) {
        if (infile->read(m_teamInfo, sizeof(m_teamInfo)) < sizeof(m_teamInfo))
            return false;
    } else {
        for (int i = 0; i < g_mapHeaderPlayerCount; ++i)
            m_teamInfo[i] = i;
    }
    return true;
}

// Project-inferred pair from random-map setup and unsupported-map display.
// This disables special conditions without reinitializing their result or
// payload; the stream readers use each condition's resetForType instead.
void CMapHeaderData::disableSpecialConditions()
{
    m_lossCondition.m_type = -1;
    m_victoryCondition.m_type = -1;
}

VA(0x004c3200, 0x398)
DC_ADDRESS(0x0adf38, 0x67a)
MAC_ADDRESS(0x0d9320, 0x588)
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
        m_victoryCondition.m_townX = decodeOptionalByte(intBuffer & 0xff);
        infile->read(&intBuffer, sizeof(char));
        m_victoryCondition.m_townY = decodeOptionalByte(intBuffer & 0xff);
        infile->read(&intBuffer, sizeof(char));
        m_victoryCondition.m_townZ = decodeOptionalByte(intBuffer & 0xff);
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

// DC records int_buffer/count/char_buffer at procedure scope. Mac checked
// writes reuse char storage at sp+0x53 and integer storage at sp+0x54;
// unchecked writes have separate temporaries. Preserve those two lifetimes
// through the canonical scalar helpers; Windows reproduces exactly.
VA(0x004c35a0, 0x2E8)
DC_ADDRESS(0x0ae5b4, 0x5ae)
MAC_ADDRESS(0x0d98a8, 0x4cc)
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
// Mac uses separate value-read temporaries for the unchecked leading flags,
// shared char storage at sp+0x81 for checked bytes, and int storage at sp+0x84
// for checked little-endian amounts. Keep these lifetimes through the scalar
// helpers; the combined source model reproduces Windows exactly.
VA(0x004c3890, 0x3E4)
DC_ADDRESS(0x0aeb64, 0x5b4)
MAC_ADDRESS(0x0d9d74, 0x500)  // sole Load caller + retail body;
int NewSMapHeader::loadVictoryCondition(char type, TAbstractFile* infile,
                                        int saveVersion)
{
    int intBuffer;
    int count;
    char charBuffer;

    m_victoryCondition.m_allowNormalVictory = readValue<char>(infile) != 0;
    m_victoryCondition.m_appliesToComputer = readValue<char>(infile) != 0;

    switch (type) {
    case VICTORY_CONDITION_ARTIFACT: {
        unsigned char artifact = readValue<unsigned char>(infile);
        m_victoryCondition.m_artifactNum =
            static_cast<TArtifact>(artifact); /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */
        return 0;
    }

    case VICTORY_CONDITION_TOTAL_CREATURES: {
        unsigned char creature = readValue<unsigned char>(infile);
        m_victoryCondition.m_creatureType = TCreatureType(creature);
        count = readLittleEndianValue(infile, intBuffer);
        if (count < sizeof(intBuffer))
            return -1;
        m_victoryCondition.m_numCreatures = intBuffer;
        return 0;
    }

    case VICTORY_CONDITION_TOTAL_RESOURCES: {
        count = readValue(infile, charBuffer);
        if (count < sizeof(charBuffer))
            return -1;
        m_victoryCondition.m_resourceType = charBuffer;
        count = readLittleEndianValue(infile, intBuffer);
        if (count < sizeof(intBuffer))
            return -1;
        m_victoryCondition.m_resourceAmount = intBuffer;
        return 0;
    }

    case VICTORY_CONDITION_UPGRADE_TOWN: {
        unsigned char townValue = readValue<unsigned char>(infile);
        m_victoryCondition.m_townX = townValue;
        townValue = readValue<unsigned char>(infile);
        m_victoryCondition.m_townY = townValue;
        townValue = readValue<unsigned char>(infile);
        m_victoryCondition.m_townZ = townValue;
        m_victoryCondition.m_hallLevel = readValue<char>(infile);
        count = readValue(infile, charBuffer);
        if (count < sizeof(charBuffer))
            return -1;
        m_victoryCondition.m_castleLevel = charBuffer;
        return 0;
    }

    case VICTORY_CONDITION_BUILD_GRAIL: {
        unsigned char grailTown = readValue<unsigned char>(infile);
        m_victoryCondition.m_townX = decodeOptionalByte(grailTown);
        grailTown = readValue<unsigned char>(infile);
        m_victoryCondition.m_townY = decodeOptionalByte(grailTown);
        grailTown = readValue<unsigned char>(infile);
        m_victoryCondition.m_townZ = decodeOptionalByte(grailTown);
        return 0;
    }

    case VICTORY_CONDITION_DEFEAT_HERO: {
        // Mac retains loadHeroId at 0:0xda0a8; VC6 expands its body here.
        m_victoryCondition.m_heroId = loadHeroId(infile, saveVersion);
        return 0;
    }

    case VICTORY_CONDITION_CAPTURE_TOWN: {
        unsigned char capturedTown = readValue<unsigned char>(infile);
        m_victoryCondition.m_townX = capturedTown;
        capturedTown = readValue<unsigned char>(infile);
        m_victoryCondition.m_townY = capturedTown;
        capturedTown = readValue<unsigned char>(infile);
        m_victoryCondition.m_townZ = capturedTown;
        return 0;
    }

    case VICTORY_CONDITION_DEFEAT_MONSTER: {
        unsigned char monster = readValue<unsigned char>(infile);
        m_victoryCondition.m_monsterX = monster;
        monster = readValue<unsigned char>(infile);
        m_victoryCondition.m_monsterY = monster;
        monster = readValue<unsigned char>(infile);
        m_victoryCondition.m_monsterZ = monster;
        return 0;
    }

    case VICTORY_CONDITION_SURVIVE_TIME: {
        m_victoryCondition.m_numDays = readLittleEndianValue<int>(infile);
        return 0;
    }

    case VICTORY_CONDITION_TRANSPORT_ARTIFACT: {
        unsigned char transport = readValue<unsigned char>(infile);
        m_victoryCondition.m_artifactNum =
            static_cast<TArtifact>(transport); /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */
        transport = readValue<unsigned char>(infile);
        m_victoryCondition.m_townX = transport;
        transport = readValue<unsigned char>(infile);
        m_victoryCondition.m_townY = transport;
        transport = readValue<unsigned char>(infile);
        m_victoryCondition.m_townZ = transport;
        return 0;
    }
    }

    return 0;
}

VA(0x004c3c80, 0x10B)
DC_ADDRESS(0x0af118, 0x19c)
MAC_ADDRESS(0x0da274, 0x16c)
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

// Mac retains the time-limit short-write check; the caller ignores this
// helper's result, so Windows can discard that check when expanding it.
DC_ADDRESS(0x0af2b4, 0x1d4)
MAC_ADDRESS(0x0da3e0, 0x138)
int NewSMapHeader::saveLossCondition(char type, TAbstractFile* outfile)
{
    switch (type) {
    case LOSS_CONDITION_LOSE_TOWN:
        writeValue<char>(outfile, m_lossCondition.m_townX);
        writeValue<char>(outfile, m_lossCondition.m_townY);
        writeValue<char>(outfile, m_lossCondition.m_townZ);
        break;

    case LOSS_CONDITION_LOSE_HERO:
        writeValue<short>(outfile, m_lossCondition.m_heroId);
        break;

    case LOSS_CONDITION_TIME_LIMIT:
        if (static_cast<unsigned>(writeValue<short>(outfile,
                m_lossCondition.m_numDays)) < sizeof(short))
            return -1;
        break;
    }
    return 0;
}

// Complete adds saveVersion; every retained return pops three arguments.
VA(0x004c3d90, 0x15E)
DC_ADDRESS(0x0af488, 0x1c4)
MAC_ADDRESS(0x0da518, 0x19c)  // DC loadLossCondition + sole Load caller + ret 0xc
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
            m_lossCondition.m_heroId = loadHeroIdShort(infile, saveVersion);
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
// Mac 0xdaa84 decodes the count once; 0xdaafc..0xdab08 increments an
// independent index against it. Preserve that counted loop and the scalar
// readers; counting the saved total down changes the Windows inline frontier.
// Mac 0xdaa94 constructs a string at r1+0xa8 before the resize call, passes
// it nowhere, and destroys it after the loop (0xdab14): an unused function-
// level string local, DC NewSMapHeader::Read's std::string strTemp. Windows
// retail shows the same extra _Tidy call and delete. The feature test reads through the bitset's reference proxy, which
// puts test() at depth 2 where retail refuses it (.test(1) gives 95.21%).
// Together 74.02 -> 95.89%; retail still calls one more vector size() inside
// the resize expansion (budget 103/61 here; retail refuses the second).
VA(0x004c3ef0, 0x498)
MAC_ADDRESS(0x0da6b4, 0x478)  // sole NewSMapHeader::Read caller + slot stride/layout
void CMapHeaderData::TPlayerSlotAttributes::readMapPlayerSlot(
    TAbstractFile* infile, int mapVersion)
{
    m_canBeHuman = readValue<signed char>(infile) != 0;
    m_canBeComputer = readValue<signed char>(infile) != 0;
    m_aiStrategy = readValue<signed char>(infile);

    if (mapVersion == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
        m_legalAlignments = readValue<unsigned char>(infile);
    } else {
        if (mapVersion != MAP_FORMAT_ARMAGEDDONS_BLADE)
            readValue<signed char>(infile);
        m_legalAlignments = readLittleEndianValue<unsigned short>(infile);
    }

    m_hasRandomAlignment = readValue<signed char>(infile) != 0;
    if (m_hasRandomAlignment)
        m_legalAlignments |= 0x100;
    if (!g_gameContextFeatures[g_videoGameState][1])
        m_legalAlignments &= 0xfeff;

    m_hasMainTown = readValue<signed char>(infile) != 0;
    m_mainTownType = -1;
    if (!m_hasMainTown) {
        m_generateHero = 0;
    } else {
        if (mapVersion == MAP_FORMAT_RESTORATION_OF_ERATHIA) {
            m_generateHero = 1;
        } else {
            m_generateHero = readValue<signed char>(infile) != 0;
            m_mainTownType = readValue<signed char>(infile);
        }

        m_castleLoc.m_x = readValue<unsigned char>(infile);
        m_castleLoc.m_y = readValue<unsigned char>(infile);
        m_castleLoc.m_z = readValue<unsigned char>(infile);
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
        clearHeroCustomization();
    }

    m_heroes.clear();
    if (mapVersion == MAP_FORMAT_RESTORATION_OF_ERATHIA)
        return;

    m_defaultPlaceholders = readValue<unsigned char>(infile);

    int heroCount = readLittleEndianValue<int>(infile);
    std::string temp;
    m_heroes.resize(heroCount);
    for (int heroIndex = 0; heroIndex < heroCount; ++heroIndex) {
        // The version-14 return above makes readHeroId's legacy
        // remapping unreachable here; Windows expands only its sentinel
        // check, while Mac retains the call at 0xdaac0.
        m_heroes[heroIndex].m_heroId = readHeroId(infile, mapVersion);
        m_heroes[heroIndex].m_name = readLengthPrefixedString(infile);
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
// Complete setup records compare the saved portrait ID as a byte before mapping
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
VA(0x004c4390, 0x92E)
DC_ADDRESS(0x0af64c, 0xb3a)
MAC_ADDRESS(0x0dab84, 0xa60)  // DC Read + LoadMap/Get callers + stream order
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
        m_maxHeroLevel = readValue<char>(infile);
    } else {
        m_maxHeroLevel = 0;
    }

    clearPlayerCounts();

    // This source local exists in the retail lifetime graph even though the
    // Complete-only tail uses a separate returned string for each hero name.
    std::string strTemp;

    TPlayerSlotAttributes* player = m_playerSlotAttributes;
    for (int i = 0; i < g_mapHeaderPlayerCount; ++i, ++player) {
        player->readMapPlayerSlot(infile, m_version);
        countPlayerSlot(*player);
    }

    if (!m_minNumHumanPlayers)
        m_minNumHumanPlayers = 1;

    int x;
    if (infile->read(&x, sizeof(unsigned char)) < sizeof(unsigned char))
        return -1;
    m_victoryCondition.resetForType(static_cast<signed char>(x));
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
    m_lossCondition.resetForType(static_cast<signed char>(x));
    if (static_cast<unsigned char>(x) != g_savedHeroNone)
        readLossCondition(x, infile);

    if (infile->read(&x, sizeof(unsigned char)) < sizeof(unsigned char))
        return -1;
    m_numTeams = x;
    if (!readTeamAssignments(infile))
        return -1;

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
        // Mac 0xdb36c decodes this saved dword with lwbrx. Windows tests
        // it for zero and decrements after the append, not signed positivity.
        unsigned int count = readLittleEndianValue<unsigned int>(infile);
        if (count > 0) {
            do {
                x = readValue<unsigned char>(infile);
                m_placeholders.push_back(H3_ENUM_DECODE(HeroId, x));
            } while (--count != 0);
        }
    }

    m_heroPlayerSetups.clear();
    if (m_version != MAP_FORMAT_RESTORATION_OF_ERATHIA
        && m_version != MAP_FORMAT_ARMAGEDDONS_BLADE) {
        x = readValue<unsigned char>(infile);
        if (static_cast<unsigned int>(x) > 0) {
            int count = x;
            do {
                int heroKey = readValue<unsigned char>(infile);

                unsigned char savedPortrait = readValue<unsigned char>(infile);
                int portrait = decodeOptionalByte(savedPortrait);

                std::string heroName = readLengthPrefixedString(infile);
                std::bitset<8> availability = readPackedBits<8>(infile);

                m_heroPlayerSetups.insert(
                    std::pair<const int, type_map_hero_info>(
                        heroKey,
                        type_map_hero_info(portrait, heroName, availability)));
            } while (--count != 0);
        }
    }

    if (infile->read(padding, sizeof(padding)) < sizeof(padding))
        return -1;
    return 0;
}

VA(0x004c4cc0, 0x130)
MAC_ADDRESS(0x0db644, 0x48)
type_map_hero_info::type_map_hero_info(int portrait, std::string name,
                                      std::bitset<8> availability)
    : m_portrait(portrait), m_name(name), m_players(availability)
{
}

// Post-header normalization. The available-hero bitset controls the
// live availability bytes, explicit per-player hero masks override the
// all-player default, and artifact victory conditions reserve their target
// before random artifact placement starts.
VA(0x004c4e30, 0xD3)
MAC_ADDRESS(0x0db6b0, 0x154)  // sole new-map caller + map-header/member layout
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
        std::bitset<8> allPlayers = ~std::bitset<8>();
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
// DC game.cpp:6920 records construction of the local string from the saved
// hero name; Mac 0xdb804+0x40c retains that constructor call. Direct
// construction raises VC6 from 89.9519% to 90.3579% and gives the Mac
// candidate the same 1776-byte extent as retail, with one direct-call gap.
// DC 0xb0754's twenty gzwrite calls stage each field DC shares through a
// local buffer, so those writes stay direct. Mac swaps the Complete-only
// alignment and name length as owned values, which keep writeValue; the
// setup key and portrait reuse enumBuffer like the other enum writes
// (95.51%; writeValue<char> for those two gives 78.12%).
VA(0x004c4f10, 0x71D)
DC_ADDRESS(0x0b0188, 0x5cc)
MAC_ADDRESS(0x0db804, 0x6f0)  // game::Save caller + DC identity + stream-write order
int NewSMapHeader::save(TAbstractFile* outfile)
{
    char enumBuffer;
    int count;
    int i;
    unsigned char ucharBuffer;
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

    writeValue<char>(outfile, m_maxHeroLevel);

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

        writeValue<unsigned short>(outfile,
            LITTLE_ENDIAN_SHORT(player->m_legalAlignments));

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

            std::string s(player->m_nonRandomHeroCustomName);
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

    writeValue<unsigned char>(outfile, m_heroPlayerSetups.size());
    for (std::map<int, type_map_hero_info>::iterator it =
             m_heroPlayerSetups.begin();
         it != m_heroPlayerSetups.end(); ++it) {
        enumBuffer = it->first;
        outfile->write(&enumBuffer, sizeof(enumBuffer));
        enumBuffer = it->second.m_portrait;
        outfile->write(&enumBuffer, sizeof(enumBuffer));

        count = it->second.m_name.length();
        writeValue<int>(outfile, LITTLE_ENDIAN_LONG(count));
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

// Mac native decodes the saved-header version/size at 0xdbf3c/0xdbfb8
// after successful reads, and the unchecked wide-alignment value at 0xdc230.
// Reuse the canonical endian readers; the Windows TU remains byte-flat.
// Saved headers share the scenario header's scalar layout but add four
// versioned migrations: max hero level, widened alignment masks, custom hero
// records, and per-player availability masks.  The two old campaign hero ids
// use the same pre-25 remap as the other saved-game readers.
// Complete's availability loop has no DC spelling record. The direct
// bitset::set spelling used by the scenario reader lowers this saved-header
// reader from 90.8783% to 90.13% (84 to 83 exact CFG blocks); retail's
// retained bitset<8>::_Xran call still does not appear. Keep the proxy form.
VA(0x004c5630, 0x7CD)
DC_ADDRESS(0x0b0754, 0x752)
MAC_ADDRESS(0x0dbef4, 0x8b8)  // DC Load + saved-header callers + helper edges
int NewSMapHeader::load(TAbstractFile* infile, int saveVersion)
{
    char enumBuffer;
    int count;
    int i;
    unsigned char ucharBuffer;
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
        m_maxHeroLevel = readValue<char>(infile);
    } else {
        m_maxHeroLevel = 0;
    }

    clearPlayerCounts();

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

        countPlayerSlot(*player);

        if (saveVersion < g_saveVersionWideAlignments) {
            infile->read(&ucharBuffer, sizeof(ucharBuffer));
            player->m_legalAlignments = ucharBuffer;
        } else {
            player->m_legalAlignments = readLittleEndianValue<unsigned short>(infile);
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
            player->clearHeroCustomization();
        }
    }

    if (!m_minNumHumanPlayers)
        m_minNumHumanPlayers = 1;

    if (infile->read(&x, sizeof(unsigned char)) < sizeof(unsigned char))
        return -1;
    m_victoryCondition.resetForType(static_cast<signed char>(x));
    if (static_cast<unsigned char>(x) != g_savedHeroNone)
        loadVictoryCondition(x, infile, saveVersion);

    if (infile->read(&x, sizeof(unsigned char)) < sizeof(unsigned char))
        return -1;
    m_lossCondition.resetForType(static_cast<signed char>(x));
    if (static_cast<unsigned char>(x) != g_savedHeroNone)
        loadLossCondition(x, infile, saveVersion);

    if (infile->read(&x, sizeof(unsigned char)) < sizeof(unsigned char))
        return -1;
    m_numTeams = x;
    if (!readTeamAssignments(infile))
        return -1;

    m_heroPlayerSetups.clear();
    if (saveVersion < g_saveVersionCustomHeroSetups)
        return 0;

    x = readValue<unsigned char>(infile);
    if (static_cast<unsigned int>(x) <= 0)
        return 0;
    count = x;

    do {
        int heroKey = readValue<unsigned char>(infile);

        int portrait = decodeOptionalByte(readValue<unsigned char>(infile));

        std::string strTemp = readLengthPrefixedString(infile);
        std::bitset<8> availability =
            saveVersion >= g_saveVersionCustomHeroAvailability
                ? readPackedBits<8>(infile) : ~std::bitset<8>();

        m_heroPlayerSetups.insert(
            std::pair<const int, type_map_hero_info>(
                heroKey, type_map_hero_info(portrait, strTemp, availability)));
    } while (--count != 0);

    return 0;
}

// The retained ret 0xc independently fixes the three explicit PC arguments.
VA(0x004c5e00, 0x210)
DC_ADDRESS(0x0b0ea8, 0x266)
MAC_ADDRESS(0x0dc7ac, 0xcc)  // DC Get + five PC callers + TGzFile/Read edges
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

// DC NewSMapHeader::readString, static with a string reference.
// Mac 0xdc8b0 checks the read count before decoding length at 0xdc8c4.
// DC names size/count/instr. Declaration-order and entry-vs-arm pointer
// lifetime controls keep this body exact and read() at 59.53%; those scopes
// do not explain VC6 expanding the two readString calls in that caller.
VA(0x004c6010, 0x1CE)
DC_ADDRESS(0x0b1110, 0x11e)
MAC_ADDRESS(0x0dc878, 0x114)
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

// DC game.cpp:7290 subscripts m_towns and 7317 names IsComputerTeam followed
// by IsHumanTeam on GetTeam results; that spelling is exact (the getTown and
// isHumanAlly wrappers score 76.05%).
VA(0x004c61e0, 0x4A8)
DC_ADDRESS(0x0b1230, 0x518)
MAC_ADDRESS(0x0dc98c, 0x3dc)
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

    thisTown->getArmy().initialize();
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

VA(0x004c66e0, 0xCB)
DC_ADDRESS(0x0b1748, 0xde)
MAC_ADDRESS(0x0dcd68, 0x10c)
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

VA(0x004c67b0, 0x1A4)
DC_ADDRESS(0x0b1828, 0x15e)
MAC_ADDRESS(0x0dce74, 0x11c)
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

VA(0x004c6960, 0xC9)
DC_ADDRESS(0x0b1988, 0xc8)
MAC_ADDRESS(0x0dcf90, 0xfc)
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

// DC game.cpp:7462 calls NewfullMap::cell(int, int, int) with the three
// location fields (byte-neutral against cell(type_point)), and DC 7484 calls
// type_point::operator== for the shipyard search; Mac 0xdd1bc..0xdd224
// expands the operator's Boolean chain. Keep it (72.12%; three field tests
// reach 99.97%). Retail spills `this` into a 0x2c frame; while/for loops,
// either operand order, this->getHero and the getCell wrapper reach at
// most 73.04%.
// Four bounded source families (28 states, eight reproduced objects) preserve
// the recorded acquisition order/names at 72.1173%. Moving player/index scope,
// using a CMC argument temporary, and naming the point comparison do not improve
// it; int/long owner snapshots lower it. why-branch loop rotations also leave
// the structural residual unchanged (25/29 blocks, same five call sites; the
// vector insert target is the native folded point/pointer alias). Preserve
// operator== rather than the historical flattened comparison.
// DC 7473/7475/7481/7498 reads and writes the cell's shipyard owner directly.
// Use its inherited union member, without a cast-through-void pointer alias;
// this restores the native access model at the same Windows matching score.
VA(0x004c6a30, 0x21F)
DC_ADDRESS(0x0b1a50, 0x23c)
MAC_ADDRESS(0x0dd08c, 0x298)
void game::claimShipyard(type_point location, int newPlayerOwner)
{
    // Original DC locals: cell, this_hero, current_player, i.
    // DC 7462 acquires cell before 7464 initializes the hero pointer.
    NewmapCell* cell = m_worldMap.cell(location.m_x, location.m_y, location.m_z);
    hero* thisHero = 0;
    if (cell->m_type == HERO) {
        thisHero = g_game->getHero(cell->m_extraInfo);
        thisHero->restoreCell();
    }

    if (cell->m_shipyardInfo.m_owner != newPlayerOwner) {
        if (cell->m_shipyardInfo.m_owner >= 0) {
            playerData* currentPlayer = &m_players[cell->m_shipyardInfo.m_owner];
            long i = 0;
            while (i < currentPlayer->m_shipyards.size()) {
                if (currentPlayer->m_shipyards[i] == location)
                    break;
                ++i;
            }
            if (i < currentPlayer->m_shipyards.size())
                currentPlayer->m_shipyards.erase(
                    currentPlayer->m_shipyards.begin() + i);
        }

        if (newPlayerOwner >= 0) {
            setVisibility(location.m_x, location.m_y, location.m_z,
                          newPlayerOwner, 3, 0);
            m_players[newPlayerOwner].m_shipyards.push_back(location);
        }

        cell->m_shipyardInfo.m_owner = newPlayerOwner;
        CMCClaimShipYard change(location, newPlayerOwner);
        sendMapChange(&change);
    }

    if (thisHero) {
        thisHero->obscureCell();
    }
}

VA(0x004c6c50, 0x2EB)
DC_ADDRESS(0x0b1c8c, 0x28e)
MAC_ADDRESS(0x0dd324, 0x3e8)
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
            g_currentPlayer->payResourceCost(upgradeCost);
            group.m_armies[iarmy] = upgradeToType;
            break;
        }
        }
    }
    delete viewArmyWindow;
}

// Original GetRandomNumTroops, game.cpp:7572. Ordinary
// member expanded in RandomizeEvents; no retained retail row is known.
DC_ADDRESS(0x0b1f1c, 0x42)
MAC_ADDRESS(0x0dd70c, 0x38)
int game::getRandomNumTroops(int whichMon)
{
    return random(g_creatureTypeTraits[whichMon].m_wanderingLow,
                  g_creatureTypeTraits[whichMon].m_wanderingHigh);
}

VA(0x004c6f40, 0x3F)
DC_ADDRESS(0x0b1f60, 0x44)
MAC_ADDRESS(0x0dd744, 0x54)
void startAITheme()
{
    char name[40];

    sprintf(name, DATA_COMPGEN(0x00677eac, aiThemeFormat, "AITheme%d"),
            random(1, 3) - 1);
    g_soundManager->startMP3(name, 0, 1);
}

VA(0x004c6f80, 0x4F)
DC_ADDRESS(0x0b1fa4, 0x1c)
MAC_ADDRESS(0x0dd798, 0x30)
void game::turnOnAIMusic()
{
    startAITheme();
    g_soundManager->setPlaybackState(0);
}

VA(0x004c6fd0, 0x10)
DC_ADDRESS(0x0b1fc0, 0x10)
MAC_ADDRESS(0x0dd7c8, 0x14)
void game::turnOffAIMusic()
{
    g_soundManager->setPlaybackState(1);
}

// E:\gamedcs\game.cpp:7603
// The retail body preserves the HoMM2 turn-transition skeleton while adding
// Complete's victory-condition sweep, local-human turn timer, network-state
// transfer/retry and visiting/recruit hero refreshes. The roster adjacency
// (immediately after TurnOffAIMusic), five retail callers and the complete DC
// callee set independently identify the row.

// Mac 0xdd9fc..0xddab8 retains an inner player scan and an outer -1
// sentinel loop. It flips lastWasHuman before perDay, as DC game.cpp:7674
// also does. Restoring those loops and the byte assignment raises Windows
// 82.4458% -> 89.4563%; the receiver allocation now agrees with retail.
// DC game.cpp:7638 and Mac 0xdd92c retain game::isHuman in the human census.
// Remaining differences include byte-flag stack homes and later expansions.
// Grouping the byte declarations and moving makeOrig initialization are flat.
// DC 7741 and Mac ddca0..ddcc0 acquire the owned hero directly; retail also
// has no getHero -1 guard here. The later recruit/garrison refreshes retain
// getHero. All three rosters come from this->m_players, not g_currentPlayer;
// Mac ddd90 and retail load town+0xc (garrison), not +0x10 (visiting).
// DC temp locals and single-line movement assignments (7742/7750/7762)
// agree with native move-before-max stores. DC 7747/7749 retains continue
// and a fresh recruit field expression; naming an intermediate ID wrongly
// lets VC6 discard the getter guard. DC 7798/7805 and Mac ddea8/dded4 use
// g_game for the last-human query and save transfer. DC 7783/7785/7787
// initializes makeOrig/control before toWho; Mac ddf98 updates visibility
// before the watch player. Together these recover Windows 89.3660 -> 99.9789
// with all 114 CFG blocks and 167 relocations agreeing; byte scratch homes
// remain different. All game sibling CUR and available Mac scores are flat.
VA(0x004c6fe0, 0x947)
DC_ADDRESS(0x0b1fd0, 0xb04)
MAC_ADDRESS(0x0dd7dc, 0x89c)  // dc-name/order + retail caller/callee/body
void game::nextPlayer()
{
    int toWho;
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
        humans = 0;
        for (i = 0; i < 8; ++i) {
            if (!m_playerDisabled[i] && isHuman(i))
                ++humans;
        }
        g_advManager->drawRolloverText(
            const_cast<char*>((*g_generalText)[GENERAL_TEXT_AUTOSAVING]));
        saveGame((*g_generalText)[GENERAL_TEXT_AUTOSAVE_NAME], 1, 0, 1, 0);
        g_advManager->drawRolloverText(
            DATA_COMPGEN(0x00691210, nextPlayerEmptyRollover, ""));
    }

    if (g_game->m_players[g_netLocalGamePos].m_deathCountDown > 0)
        --g_game->m_players[g_netLocalGamePos].m_deathCountDown;
    g_advManager->deactivateCurrTown(0);
    g_advManager->deactivateCurrHero(0);

    lastWasHuman = m_players[g_netLocalGamePos].isHuman();
    makeOrig = 0;
    do {
        while (++g_netLocalGamePos < g_mapHeaderPlayerCount) {
            if (!m_playerDisabled[g_netLocalGamePos]
                && static_cast<unsigned char>(
                       m_players[g_netLocalGamePos].isHuman())
                       == lastWasHuman)
                break;
        }
        if (g_netLocalGamePos == g_mapHeaderPlayerCount) {
            lastWasHuman = !lastWasHuman;
            if (lastWasHuman) {
                makeOrig = 1;
                perDay();
                if (g_remoteOn) {
                    m_mapHeader.m_lossCondition.checkForTimeLimitExpired();
                    ::checkEndGame(0);
                    if (g_gameOver)
                        return;
                }
            }
            g_netLocalGamePos = -1;
        }
    } while (g_netLocalGamePos == -1);

    if (g_goSolo && makeOrig
        && (!g_remoteOn || g_numHumanPlayers == 1)) {
        g_advManager->drawRolloverText(
            const_cast<char*>((*g_generalText)[GENERAL_TEXT_AUTOSAVING]));
        saveGame((*g_generalText)[GENERAL_TEXT_AUTOSAVE_NAME], 1, 0, 1, 0);
        g_advManager->drawRolloverText(
            DATA_COMPGEN(0x00691210, nextPlayerSoloEmptyRollover, ""));

        save = g_remoteOn;
        g_remoteOn = 1;
        g_goSolo = 0;
        normalDialogTimeOut((*g_generalText)[GENERAL_TEXT_PRESS_ESC_TO_CANCEL_SOLO_MODE], 2, 2000,
                            -1, -1, -1, 0, -1, 0, -1, -1, 0);
        g_remoteOn = save;
        if (g_windowManager->m_dialogReturn == DIALOG_RETURN_DECLINE) {
            g_game->m_players[g_soloPos].setLocalHuman();
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

    for (i = 0; i < m_players[g_netLocalGamePos].m_numHeroes; ++i) {
        hero* temp = &m_heroes[m_players[g_netLocalGamePos].m_heroes[i]];
        temp->refreshMovement();
    }
    for (i = 0; i < 2; ++i) {
        if (m_players[g_netLocalGamePos].m_recruits[i] == -1)
            continue;
        hero* temp = getHero(m_players[g_netLocalGamePos].m_recruits[i]);
        temp->refreshMovement();
    }
    for (i = 0; i < m_players[g_netLocalGamePos].m_numTowns; ++i) {
        town* currentTown = getTown(m_players[g_netLocalGamePos].m_townIds[i]);
        if (currentTown->m_garrisonHeroId >= 0) {
            hero* currentHero = getHero(currentTown->m_garrisonHeroId);
            currentHero->refreshMovement();
            currentHero->m_isSleeping = 0;
        }
    }

    if (!g_currentPlayer->isLocalHuman()) {
        g_mouseManager->setPointer(2, mouseManager::DEFAULT_SET);
        g_advManager->hideRoute(1, 0, 1);
        // Mac nextPlayer calls the retained turnOnAIMusic body at 0:0xdd798;
        // VC6 expands this same ordinary member into the Windows caller.
        turnOnAIMusic();
        setNoDialogMenus(0);
        g_advManager->overrideBottomView(advManager::BOTTOM_VIEW_8, -1);
        showComputerScreen();
        g_completeDrawEnabled = 0;

        if (g_remoteOn && isHuman(g_netLocalGamePos)) {
            makeOrig = 0;
            g_thisNetGotAdventureControl = 0;
            toWho = g_netLocalGamePos;
            if (g_playerDrop) {
                toWho = 0x7f;
                g_playerDrop = 0;
            }
            if (g_game->isLastHuman(g_game->getLocalPlayerGamePos())) {
                toWho = 0x7f;
                g_playerDrop = 0;
                makeOrig = 1;
            }
            save = g_game->transmitSaveGame(toWho, 0, 1, makeOrig);
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
        g_mapVisibilityBit = g_curPlayerBit;
        g_curWatchPlayer = g_netLocalGamePos;

        if (g_blackoutPlayer && g_numHumanPlayers > 1) {
            char textBuffer[256];
            sprintf(textBuffer, (*g_generalText)[
                        GENERAL_TEXT_PLAYER_TURN_FORMAT],
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

VA(0x004c7930, 0x266)
DC_ADDRESS(0x0b2ad4, 0x55c)
MAC_ADDRESS(0x0de078, 0x308)
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

    if (!isHuman(whichPlayer)) {
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
VA(0x004c7ba0, 0xAC)
DC_ADDRESS(0x0b3030, 0x14c)
MAC_ADDRESS(0x0de380, 0xb8)  // MAC_ABSTRACTION_FROM(tokens1:dfc52e9173c5,100.0000): hasBuilding now delegates its active-mask read to canonical getBuildingMask; CW changes expansion context of this false-arm call.
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

VA(0x004c7c50, 0x389)
DC_ADDRESS(0x0b317c, 0x6da)
MAC_ADDRESS(0x0de438, 0x3e0)
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
                if (getNumMapLevels() > 1) {
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

    for (int z = 0; z < g_game->getNumMapLevels(); ++z) {
        for (int y = 0; y < g_mapHeight; ++y) {
            for (int x = 0; x < g_mapWidth; ++x) {
                NewmapCell* tempCell = g_game->getWorldMapData()->cell(x, y, z);
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
VA(0x004c7fe0, 0x462)
DC_ADDRESS(0x0b3858, 0x532)
MAC_ADDRESS(0x0de818, 0x6e4)
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
            || !townAlreadyBuiltOn(i)) {
            m_towns[i].m_builtThisTurn = 0;
        } else {
            --m_towns[i].m_builtThisTurn;
        }
    }

    for (i = 0; i < HERO_COUNT; ++i) {
        hero& currHero = m_heroes[i];
        currHero.m_flags &= 0xfffdfffeU;
        currHero.resetAdventureSpells();
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
            currHero.raiseManaTo(maxMana);
        } else {
            int tempMana = currHero.m_mana;
            tempMana += currHero.getMysticismBonus();
            if (tempMana > maxMana)
                tempMana = maxMana;
            currHero.raiseManaTo(tempMana);
        }
    }

    for (i = 0; i < m_towns.size(); ++i) {
        town* currTown = getTown(i);
        if (currTown->hasBuilding(MAGE_GUILD_ID, true)) {
            if (currTown->m_visitingHeroId != -1) {
                hero* currHero = getHero(currTown->m_visitingHeroId);
                int maxMana = currHero->getMaxMana();
                currHero->raiseManaTo(maxMana);
            }
            if (currTown->m_garrisonHeroId != -1) {
                hero* currHero = getHero(currTown->m_garrisonHeroId);
                int maxMana = currHero->getMaxMana();
                currHero->raiseManaTo(maxMana);
            }
        }
    }

    m_grailAsked = 0;
}

// Original: game::clear_recruits; game.cpp:8266
// DC 8266/8290 and Mac 0xdeefc/0xdef98 place these two helpers between
// perDay and setWeeklyRecruits. DC names the long loop index recruit and
// the selected hero pointer old_hero.
DC_ADDRESS(0x0b3d8c, 0x74)
MAC_ADDRESS(0x0deefc, 0x9c)
void game::clearRecruits(int* recruits)
{
    for (long recruit = 0; recruit < 2; ++recruit) {
        int heroId = recruits[recruit];
        if (heroId >= 0) {
            hero* oldHero = getHero(heroId);
            if (oldHero->m_flags & g_heroRecruitReservedFlag)
                continue;
            m_heroAvailability[heroId] = -1;
            recruits[recruit] = -1;
        }
    }
}

// Original: get_new_hero; game.cpp:8290
DC_ADDRESS(0x0b3e00, 0x5e)
MAC_ADDRESS(0x0def98, 0x114)
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

// Original: game::set_weekly_recruits; game.cpp:8308
// Complete passes a player index instead of the older recruits/align pair.
// The two-slot loop, tutorial choices and equipment/mana/army closeout identify
// this body; DC's nullary set_recruits is the surrounding all-player operation.
// Mac 0xdf0ac initializes the local artifact's id before its extra field.
// The TArtifact constructor preserves that order in VC6; the default
// constructor reverses the two stores in this caller.
VA(0x004c8450, 0x248)
DC_ADDRESS(0x0b3e60, 0x1ee)
MAC_ADDRESS(0x0df0ac, 0x238)
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
        hero* newHero = getHero(heroId);
        int backpackSlot = HERO_BACKPACK_CAPACITY - 1;
        do {
            artifact = newHero->getBackpack(backpackSlot);
            if (artifact.m_artifactId != -1
                && newHero->equipArtifact(artifact, -1))
                newHero->removeBackpackArtifact(backpackSlot);
        } while (backpackSlot--);

        newHero->resetManaToMaximum();
        setRandomHeroArmies(heroId, 0, 0);
    }
}

VA(0x004c86a0, 0xD5)
MAC_ADDRESS(0x0df2e4, 0x164)
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
        m_heroes[heroId].resetManaToMaximum();
        setRandomHeroArmies(heroId, 0, 1);
    }
}

// Original: game::set_recruits; game.cpp:8373
// Complete's NewMap and PerWeek both expand these two eight-player passes.
// The DC neutral recruit pair (receiver+0xe188) and its extra clear/fill calls
// disappeared from the desktop game layout and both retail expansions.
DC_ADDRESS(0x0b40f4, 0xea)
MAC_ADDRESS(0x0df448, 0x90)
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

// DC game.cpp:8514/8529/8534 passes map_cell directly to its inherited
// garden, wheel and windmill setters. Both random draws belong to line 8534;
// Mac 0xdf9c0..0xdf9f4 and Windows evaluate the amount before the resource.
// DC 8543/8547 and Mac 0xdfa10..0xdfa2c write the signed fountain bitfield.
// DC 8462/8464/8466 records the ordinary z/y/x loops. Do not reproduce
// optimizer induction or cast m_extraInfo through void* to call base helpers.
// The native receiver model currently leaves clearRecruits out of line in
// setRecruits' expansion; retail expands it and retains getHero. Prior raw
// aliases scored 96.9467%; the canonical model scores 94.1719%. The native
// loop spelling, obscuringHero initialization and helper body placement are
// byte-flat for that boundary. Historical 99.8370% is a TU-context lead.
VA(0x004c8780, 0x7B7)
DC_ADDRESS(0x0b41e0, 0x5d8)
MAC_ADDRESS(0x0df4d8, 0x6bc)  // PerDay/PerMonth bracket + dc lines/callees
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
    hero* currHero;

    bonusCreature = CREATURE_NONE;
    alternateBonus = CREATURE_NONE;

    g_weekType = g_weekTypeNormal;
    g_weekTypeExtra = random(0, g_weekNameLast);
    bonusAmount = g_creatureWeekGrowthBonus;

    if (m_week != g_weeksPerMonth
        && random(1, g_specialWeekRollMax) == 1) {
        g_weekType = g_weekTypeCreature;
        // Mac 0xdf54c starts the creature census here; the older DC body
        // initializes i at entry. Both paths overwrite i before later uses.
        i = 0;

        for (align = m_gameVersion ? CREATURE_CATAPULT : CREATURE_PIXIE;
             align--;) {
            if (getAlignment(align) != -1
                && g_creatureTypeTraits[align].m_level >= 0)
                ++i;
        }

        i = rand() % i;
        for (align = m_gameVersion ? CREATURE_CATAPULT : CREATURE_PIXIE;
             align--;) {
            if (getAlignment(align) != -1
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
        for (y = 0; y < g_mapHeight; ++y) {
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
                    if (!mapCell->m_monsterInfo.m_dontGrow) {
                        count = (mapCell->m_monsterInfo.m_qty << 4)
                                + mapCell->m_monsterInfo.m_unused27;
                        increase = count / 10;
                        count += increase;
                        if (count > 64000)
                            count = 64000;
                        // Windows reloads the packed dword between lanes;
                        // Mac 0xdf93c..0xdf954 stores the same qty and high
                        // nibble through the existing monster bitfields.
                        mapCell->m_monsterInfo.m_qty = count >> 4;
                        mapCell->m_monsterInfo.m_unused27 = count;
                    }
                    break;
                }

                case MYSTICAL_GARDEN: {
                    mapCell->fillGarden(random(0, 1) ? GOLD : GEMS);
                    break;
                }

                case REFUGEE_CAMP: {
                    randomizeRefugeeCamp(mapCell);
                    break;
                }

                case WATER_WHEEL: {
                    mapCell->setWheelGold(1000);
                    break;
                }

                case WINDMILL: {
                    mapCell->setWindmill(EGameResource(random(1, 5)), random(3, 6));
                    break;
                }

                case FOUNTAIN_OF_FORTUNE: {
                    mapCell->randomizeFountainLuck();
                    break;
                }
                }

                if (obscuringHero)
                    obscuringHero->obscureCell();
            }
        }
    }

    for (i = 0; i < HERO_COUNT; ++i) {
        currHero = getHero(i);
        if (currHero->m_flags & g_heroWeeklyVisitFlag)
            currHero->m_flags -= g_heroWeeklyVisitFlag;
    }

    setupNewRumour();
    giveTroopsToNeutralTowns();

    setSummoningGenerators();
}

VA(0x004c8f40, 0x378)
DC_ADDRESS(0x0b47b8, 0x39e)
MAC_ADDRESS(0x0dfb94, 0x340)
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
        g_monthType = g_monthEffectCreature;
        g_monthTypeExtra = g_creatureImpId;
    } else if (monthRoll > g_monthNormalRollMax && !m_isTutorial) {
        if (monthRoll <= g_monthCreatureRollMax) {
            g_monthType = g_monthEffectCreature;
            g_monthTypeExtra = g_monType[random(0, g_monthCreatureTableLast)];
        } else {
            g_monthType = g_monthEffectPlague;
        }
    } else {
        g_monthType = g_monthEffectNormal;
        g_monthTypeExtra = random(0, g_monthCreatureRollMax);
    }

    for (i = 0; i < m_towns.size(); ++i) {
        for (j = 0; j <= numcreaturemonthcreatures; ++j) {
            currTown = getTown(i);
            growth = currTown->getGrowthRate(j);
            if (growth > 0) {
                if (g_monthType == g_monthEffectCreature
                    && g_weekType != g_weekTypeInfernoGrail
                    && g_townDwellingCreatures[
                        currTown->m_type * TOWN_DWELLING_SLOTS + j]
                       == g_monthTypeExtra) {
                    currTown->m_population[j] *= 2;
                }

                if (g_monthType == g_monthEffectPlague) {
                    growth = currTown->getGrowthRate(j);
                    currTown->m_population[j] -= growth;
                    if (currTown->m_population[j] < 0)
                        currTown->m_population[j] = 0;
                    currTown->m_population[j] >>= 1;
                }
            }
        }
    }

    if (g_monthType == g_monthEffectCreature) {
        for (z = 0; z < getNumMapLevels(); ++z) {
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
                                     g_monthTypeExtra, 0);
                        tempCell->m_monsterInfo.m_qty = 2 * getRandomNumTroops(g_monthTypeExtra);
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

// Retail retains the receiver-based iterator dereference at 0x4d4ca0.
// Mac 0xe026c instead constructs the nested MSL bit reference from owner
// and index; these have different ABIs and are not paired helper bodies.
// The shared in-class dereference exposes that nested constructor to CW.
// Native 0xdff14..0xdff6c constructs the end first, then adds CREATURE_PIXIE
// to a zero-offset iterator; its fill loop reads a referenced false value.
// Keep the canonical iterator addition and fill helpers. The by-value
// addition replaces the provisional fromOffset factory, raising Windows
// 85.54% to 85.96% with two extra inequality calls remaining. A const
// member-copy addition reaches 84.72%; no native declaration distinguishes
// their placement. Complete's range has no counterpart in the older DC body.
VA(0x004c92c0, 0x202)
DC_ADDRESS(0x0b4b58, 0x12a)
MAC_ADDRESS(0x0dfed4, 0x398)  // MAC_ABSTRACTION_FROM(tokens1:38ec85859b6c,28.1385): native iterator addition replaces the provisional fromOffset factory; by-value temporary copies shift CW stack and register allocation.
TCreatureType game::getRandomMonster(int minLevel, int maxLevel)
{
    int i;
    int totalInClass;
    int curCount;
    int x;

    std::bitset<CREATURE_CATAPULT> monsterOk;
    monsterOk.set();

    if (!m_gameVersion) {
        std::fill(bitset_iterator<CREATURE_CATAPULT>(monsterOk) + CREATURE_PIXIE,
                  bitset_iterator<CREATURE_CATAPULT>(monsterOk, CREATURE_CATAPULT),
                  false);
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

VA(0x004c94d0, 0xCD)
DC_ADDRESS(0x0b4c84, 0x180)
MAC_ADDRESS(0x0e0278, 0x398)
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

VA(0x004c95a0, 0x18E)
DC_ADDRESS(0x0b4e04, 0x19c)
MAC_ADDRESS(0x0e0610, 0x1e4)
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

// Mac 0:e07f4 constructs the level mask, sets level-1, and calls the
// bitset overload at 0:e0610. It follows that overload in the same TU.
MAC_ADDRESS(0x0e07f4, 0x68)
SpellID game::getRandomSpell(int level)
{
    std::bitset<5> spellLevels;
    spellLevels[level - 1] = true;
    return getRandomSpell(spellLevels);
}

// Original: game::RandomizeHeroPool; game.cpp:8896
// NewMap expands this loop in retail, including Complete's enlarged roster
// and separate last-Wisdom/last-magic-school tracking bytes.
// Mac newMap calls the retained body at code0+0xe085c. Its four ordered calls
// and indexed loop follow DC's ten source rows. CodeWarrior emits the same
// 45-instruction shape from the function-scope index after call-stub collapse;
// the current Mac declaration places the hero fields 0x20 bytes later.
// Keeping the index at function scope also makes VC6 expand this ordinary
// helper in newMap while retaining its nested calls, matching Windows retail.
DC_ADDRESS(0x0b4fa0, 0xf4)
MAC_ADDRESS(0x0e085c, 0xb4)
void game::randomizeHeroPool()
{
    int heroIndex;
    for (heroIndex = 0; heroIndex < HERO_COUNT; ++heroIndex) {
        m_heroes[heroIndex].m_experience = random(0, 50) + 40;
        setRandomHeroArmies(heroIndex, 0, 0);
        m_heroes[heroIndex].refreshMovement();
        m_heroes[heroIndex].m_levelSeed =
            static_cast<unsigned char>(random(1, 255));
        m_heroes[heroIndex].m_lastWisdom = 0;
        m_heroes[heroIndex].m_lastMagicSchoolLevel = 0;
    }
}

// Windows retail and Mac 0xe0918..0xe0928 index m_heroes without getHero's
// -1 sentinel, so the hero is subscripted directly. Mac 0xe0a44..0xe0a5c
// stores each artifact id before its -1 payload: the converting
// type_artifact constructor builds both war-machine artifacts.
// DC 8930..8932 records the alternating slot-clear loop below. Mac
// 0xe0984..0xe09b8 likewise interleaves its seven type/count stores;
// armyGroup::initialize instead retains two bulk clears at 0x58128/0x58134.
// Keep the recorded loop and express its type/count clear with dismiss;
// equal final state alone does not identify a bulk initialize() call.
VA(0x004c9730, 0x159)
DC_ADDRESS(0x0b5094, 0x268)
MAC_ADDRESS(0x0e0910, 0x1c4)
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
        currentArmy->dismiss(i);
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
            type_artifact artifact(ARTIFACT_BALLISTA);
            m_heroes[hero].giveArtifact(artifact, 0, 0);
        } else if (traits->m_secondStack == CREATURE_FIRST_AID_TENT) {
            type_artifact artifact(ARTIFACT_FIRST_AID_TENT);
            m_heroes[hero].giveArtifact(artifact, 0, 0);
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

VA(0x004c9890, 0xFD)
DC_ADDRESS(0x0b52fc, 0x1fc)
MAC_ADDRESS(0x0e0ad4, 0x114)
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
VA(0x004c9990, 0x43A)
DC_ADDRESS(0x0b54f8, 0x416)
MAC_ADDRESS(0x0e0be8, 0x448)  // anchor-global
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
            sprintf(tempText, g_artifactObjectDefFormat, tempCell->getArtifactIndex());
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
            g_game->getWorldMapData()->calculateCellExtra(newCell, 0);
        }
    }
}

// The artifact arguments are artraits.txt class bits: 2 treasure,
// 4 minor, 8 major, 16 relic, and RANDOM_ARTIFACT's 14 is
// treasure|minor|major, i.e. every class except relics.
VA(0x004c9dd0, 0x270)
DC_ADDRESS(0x0b5910, 0x3ca)
MAC_ADDRESS(0x0e1030, 0x3c0)
void game::processRandomObjects()
{
    int y, z, x;
    NewmapCell* tempCell;

    for (z = 0; z < getNumMapLevels(); ++z) {
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
// Its HeroID/index/town declaration order is retained. Mac 0xe1578..0xe1580
// computes the bonus hero address in two stages: the canonical getHero
// expansion reproduces that boundary and closes Windows at 100%, with the
// original less-than loop and all helpers preserved. Mac reaches 99.37%;
// only its larger stack frame remains different. Do not invent frame padding.
// Mac 0xe1490 retains playerData::isHuman; Windows expands that source call.
VA(0x004ca040, 0x1F1)
DC_ADDRESS(0x0b5cdc, 0x2a2)
MAC_ADDRESS(0x0e13f0, 0x1dc)  // linkorder
void game::createTownHeroes(int* startingHeroIds)
{
    int heroId;
    int i;
    town* thisTown;

    for (i = 0; i < 8; i++) {
        if (!m_mapHeader.m_playerSlotAttributes[i].m_generateHero)
            continue;

        thisTown = getTown(
            getTownId(m_mapHeader.m_playerSlotAttributes[i].m_castleLoc.m_x,
                      m_mapHeader.m_playerSlotAttributes[i].m_castleLoc.m_y,
                      m_mapHeader.m_playerSlotAttributes[i].m_castleLoc.m_z));

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
                hero::getExperience(g_game->getHero(g_startLevelHeroId)->m_level
                                    + g_startLevelBonus),
                1, 0);
    }
}

// Mac 0xe15cc..0xe1670 expands getTeamMask before the terrain sweep.
VA(0x004ca240, 0xF6)
DC_ADDRESS(0x0b5f80, 0xd2)
MAC_ADDRESS(0x0e15cc, 0x1bc)
void game::makeTerrainVisible(int whichPlayer, unsigned short visMask)
{
    unsigned char players = getTeamMask(whichPlayer);
    unsigned short playerMask = players;
    for (int z = 0; z < getNumMapLevels(); ++z) {
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

VA(0x004ca340, 0x6F)
DC_ADDRESS(0x0b6054, 0xbe)
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

VA(0x004ca3b0, 0x58)
DC_ADDRESS(0x0b6114, 0xbc)
MAC_ADDRESS(0x0e1788, 0x50)
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

VA(0x004ca410, 0x116)
DC_ADDRESS(0x0b61d0, 0x128)
MAC_ADDRESS(0x0e17d8, 0x138)
void game::setupAdjacentMons()
{
    type_point excluded(0xff, 0xff, 0xff);
    type_point monster;
    int x;
    int y;
    int z;
    unsigned short mask = ~MAP_EXTRA_MONSTER;

    for (z = 0; z < getNumMapLevels(); ++z) {
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

// Mac 0xe1910 retains this helper between setupAdjacentMons and
// cancelComputerScreen. Both Mac callers pass 1 or 0; VC6 expands the four
// widget operations in those callers. The older Dreamcast build lacks these
// calls in CancelComputerScreen, so its absence is a snapshot difference.
MAC_ADDRESS(0x0e1910, 0xc4)
static void setComputerScreenWidgetsEnabled(unsigned char enabled)
{
    g_advManager->m_advWindow->getWidget(8)->enable(enabled);
    g_advManager->m_advWindow->getWidget(7)->enable(enabled);
    g_advManager->m_advWindow->getWidget(6)->enable(enabled);
    g_advManager->m_advWindow->getWidget(12)->enable(enabled);
}

VA(0x004ca530, 0x80)
DC_ADDRESS(0x0b62f8, 0x64)
MAC_ADDRESS(0x0e19d4, 0x50)
void game::cancelComputerScreen()
{
    g_completeDrawEnabled = 1;
    g_advManager->updateRadar(1, 1, 0, 0, 0);
    setComputerScreenWidgetsEnabled(1);
}

VA(0x004ca5b0, 0x1C9)
DC_ADDRESS(0x0b635c, 0x1dc)
MAC_ADDRESS(0x0e1a24, 0x1d8)
void game::showComputerScreen()
{
    setComputerScreenWidgetsEnabled(0);

    if (g_config.m_blackoutComputer && !g_currentPlayer->isHuman()) {
        // Mac saves isLocalHuman at 0:0xe1a70 before the temporary override
        // and restores that byte at 0:0xe1b18 after drawing.
        // This exposes the computer's local view without making it human;
        // the paired setLocalHuman/setComputer transitions do not apply.
        unsigned char wasLocalHuman = g_currentPlayer->isLocalHuman();
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
        g_currentPlayer->m_isLocal = wasLocalHuman;
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
DC_ADDRESS(0x0b6538, 0x108)
MAC_ADDRESS(0x0e1bfc, 0x100)
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

VA(0x004ca840, 0x19C)
DC_ADDRESS(0x0b6640, 0x236)
MAC_ADDRESS(0x0e1cfc, 0x21c)
void game::waitForPlayer(char* text, int playerId)
{
    if (!g_blackoutPlayer || g_numHumanPlayers <= 1 || g_remoteOn)
        return;

    g_mouseManager->setPointer(0, mouseManager::DEFAULT_SET);
    g_completeDrawAllCells = 1;
    // Mac waitForPlayer retains playerData::isLocalHuman at code0+0xe1d78.
    if (g_currentPlayer->isLocalHuman())
        g_advManager->overrideBottomView(advManager::BOTTOM_VIEW_1, 9999999);
    else
        g_advManager->overrideBottomView(advManager::BOTTOM_VIEW_DEFAULT, 9999999);

    g_soundManager->setPlaybackState(1);
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

// Original: game::SetupTowns; game.cpp:9798
// The DC release body is empty; its 6 bytes only home this and return.
DC_ADDRESS(0x0b6878, 0x6)
void game::setupTowns()
{
}

// The nine-faction no-repeat town-name samplers are defined in game.cpp.
// Retail's vector-constructor iterator at 0x4ca9e0 proves nine 24-byte
// TPickRandomTownName objects and its element wrapper proves [0, 15].
DATA(0x006971a0)
// Previous project spelling: gRandomTownNames.
static TPickRandomTownName g_randomTownNames[9];

// E:\gamedcs\game.cpp:9803
// Mac retains this helper at 0:0xe1f18; VC6 expands its call in
// processOnMapTowns.
DC_ADDRESS(0x0b6944, 0x72)
MAC_ADDRESS(0x0e1f18, 0x80)
const char* getRandomTownName(int townType)
{
    int name = g_randomTownNames[townType].pick();
    while (name == -1) {
        townType = random(0, 8);
        name = g_randomTownNames[townType].pick();
    }
    return g_townNames[townType][name];
}

// E:\gamedcs\game.cpp:9821
// Mac retains this helper at 0:0xe1f98; VC6 expands its source call.
DC_ADDRESS(0x0b69b8, 0x3a)
MAC_ADDRESS(0x0e1f98, 0x8c)
void resetRandomTownNames()
{
    for (int i = 0; i < 9; ++i)
        g_randomTownNames[i].reset();
}

// Original: game::CheckHeroConsistency; game.cpp:10132
// The DC release body only homes this and returns; Mac retains a single BLR
// at code 0:0xe2410, called by townManager::open and philAI::doAI.
DC_ADDRESS(0x0b7554, 0xc)
MAC_ADDRESS(0x0e2410, 0x4)
void game::checkHeroConsistency()
{
}

// E:\gamedcs\game.cpp:9833
// Complete's town-name lookup has a 16-pointer faction stride, matching
// initializeTownNameText. The direct m_towns subscript and string assignment
// give 99.98% (getTown plus assign(): 95.79%); the residual is the unclaimed
// g_randomTownNames end-bound data label.
VA(0x004caa70, 0x39C)
DC_ADDRESS(0x0b69f4, 0x290)
MAC_ADDRESS(0x0e2024, 0x1bc)  // DC name/order + retail map/vector/string shape
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
DC_ADDRESS(0x0b7204, 0x350)
MAC_ADDRESS(0x0e21e0, 0x230)
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
            townCell = getCell(townLoc);
            if (townCell->m_type == TOWN && townCell->m_isTrigger
                && m_heroAvailability[heroExtra->m_id]
                    != hero::HERO_AVAILABILITY_PRISON) {
                --heroExtra->m_location.m_x;
            }

            currHero = getHero(heroExtra->m_id);
            currHero->initialize(heroExtra);

            if (m_heroAvailability[heroExtra->m_id]
                != hero::HERO_AVAILABILITY_PRISON) {
                m_players[currHero->m_owner].addHero(currHero->m_id);
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
// DC supplies the signature, named locals, scopes and statement order.
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

// Mac 0xe2904..0xe2a48 retains the initial-send completion loop, with
// bytesLeft/current/curBlock reset inside it; 0xe2a08 sets done before the
// end-message send. Restoring that loop raises Windows 83.8044% -> 85.0897%.
// Placing done around the final log/end-message statements is Windows-flat.
// Residual: the DC-order candidate's frame is 0x3b0 versus retail's 0x3a4.
// Retail keeps isDiff/newSize in BL/EBX through disjoint phases and bytesLeft
// in EBX through transmission; the candidate keeps the packet pointer there
// and spills these values. Moving the fileSize declaration alone and swapping
// isDiff/diffSize declarations were byte-flat in earlier controls. Missing DC
// queueSize/attempts/pNetMsg have no independent retail semantics proven yet.
// DC 10473..10491, Mac e2e84..e2ef0 and retail's resend arm store the block
// number first, reuse current/bytesLeft, form unsigned char* end=data+fileSize,
// then choose the message's blockSize before update(current, blockSize). The
// log reads the message fields and current-data. Restoring those lifetimes
// removes the invented offset/size locals and raises Windows 85.0816 ->
// 92.7077; the frame is now retail's 0x3a4. All 122 CFG blocks align (113
// exact, nine size-only). DC 10326/10327 and Mac e2908/e290c reset current
// before bytesLeft; this source-order correction is Windows-flat. The timeout
// start is DC-proven int; GameTime's unsigned result/argument conversions
// preserve its bit pattern, and that declaration correction is byte-flat.
// The remaining call-report difference is a shifted switch-table target;
// predict-inline's unmatched pair names that table, not a lost game helper.
// Available Mac scores and sibling Windows MAX are unchanged.
// DC 10409/10430 records killDPID before the opaque destroy/drop calls;
// those calls reread the player field, while the later message uses the saved
// ID. DC 10413/10418 and 10434/10437, Mac e2b84/e2bc0 and e2ca0/e2cd8,
// and retail +0x97f/+0xad0 clear network info before constructing the message.
// The broadcast clear intentionally uses m_players[toWho], not m_players[i]:
// retail's +0xad0 block loads [ebp+8] (toWho), Mac e2c2c..e2c44 forms that row before
// reusing r23 as the loop index, and DC 10434 reads the argument slot.
// Preserve this native indexing despite its unusual broadcast behavior.
// These lifetime/receiver corrections raise Windows 92.7077 -> 97.4264;
// all 122 CFG blocks, 101 call entries and 172 relocations now agree.
VA(0x004cafd0, 0xD14)
DC_ADDRESS(0x0b7560, 0x1064)
MAC_ADDRESS(0x0e2414, 0xd20)  // retail body + typed catch + continuation/tables
int game::transmitSaveGame(int toWho, int thisPlayerDead,
                           unsigned char inGame, unsigned char makeOrig)
{
    CNetMsgHandlerPause netMsgHandlerPause;
    g_advManager->trimLoopingSounds(4);

    CGameTransmitMainMsg* gameTransmitMainMsg =
        CGameTransmitMainMsg::createMsg(GAME_TRANSMIT_PAYLOAD_SIZE);
    unsigned char isDiff = 0;
    unsigned long diffSize = 0;
    int changeSounds = g_soundManager->enablePlayback();
    g_soundManager->switchAmbientMusic(-1);
    g_soundManager->setPlaybackState(changeSounds);

    if (g_advManager->getStatus() == baseManager::STATUS_ACTIVE)
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
    while (!done) {
        current = data;
        bytesLeft = fileSize;
        curBlock = 0;
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
        done = 1;
        {
            CGameTransmitEndMsg endMsg(g_monthType, g_monthTypeExtra,
                                     g_weekType, g_weekTypeExtra, diffSize);
            transmitRemoteData(&endMsg, toWho, false, true);
        }
    }
    done = 0;

    unsigned char playerDone[8];
    memset(playerDone, 0, sizeof(playerDone));
    int dataTimeOutStart = GameTime::get();
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
                        unsigned long killDPID = m_players[toWho].m_dpid;
                        g_dPlay->destroyPlayer(m_players[toWho].m_dpid);
                        handlePlayerDrop(m_players[toWho].m_dpid);
                        m_players[toWho].clearNetInfo();
                        g_playerDrop = 1;
                        CDestroyPlayerMsg destroyMsg(killDPID);
                        transmitRemoteDataDPID(&destroyMsg, NET_BROADCAST_DPID,
                                               false, true);
                        return 0;
                    } else {
                        for (int i = 0; i < 8; ++i) {
                            if (m_players[i].isHuman() && !playerDone[i]
                                    && i != g_game->getLocalPlayerGamePos()) {
                                unsigned long killDPID =
                                    m_players[i].m_dpid;
                                g_dPlay->destroyPlayer(m_players[i].m_dpid);
                                handlePlayerDrop(m_players[i].m_dpid);
                                m_players[toWho].clearNetInfo();
                                CDestroyPlayerMsg destroyMsg(killDPID);
                                transmitRemoteDataDPID(&destroyMsg, NET_BROADCAST_DPID,
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
                gameTransmitMainMsg->m_blockNbr = receivedMsg->m_blockNbr;
                current = data + receivedMsg->m_blockNbr
                    * GAME_TRANSMIT_PAYLOAD_SIZE;
                unsigned char* end = data + fileSize;
                bytesLeft = end - current;
                if (bytesLeft >= GAME_TRANSMIT_PAYLOAD_SIZE)
                    gameTransmitMainMsg->m_blockSize = GAME_TRANSMIT_PAYLOAD_SIZE;
                else
                    gameTransmitMainMsg->m_blockSize = bytesLeft;
                gameTransmitMainMsg->update(current, gameTransmitMainMsg->m_blockSize);
                g_logFile.log(DATA_COMPGEN(
                                0x00677f08, xferResendLog,
                                "Transmitting resend %d size %d (offset=%d)"),
                            gameTransmitMainMsg->m_blockNbr,
                            gameTransmitMainMsg->m_blockSize, current - data);
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

VA(0x004cbd40, 0xA83)
DC_ADDRESS(0x0b85c4, 0xe44)
MAC_ADDRESS(0x0e31b0, 0xc0c)  // retail body +  source shape
int game::receiveSaveGame(int fileSize, int fullGameCRC, int fromWho,
                          unsigned char inGame, unsigned char isDiff)
{
    CNetMsgHandlerPause netMsgHandlerPause;
    g_advManager->trimLoopingSounds(4);

    if (g_advManager->getStatus() == baseManager::STATUS_ACTIVE)
        g_advManager->bvMessage(g_generalText->getText(GENERAL_TEXT_RECEIVING_GAME));

    int lastDataReceiveTime = GameTime::get();
    int changeSounds = g_soundManager->m_currentTerrainMusic;
    char soundWasEnabled = g_soundManager->enablePlayback();
    g_soundManager->switchAmbientMusic(-1);
    g_soundManager->setPlaybackState(soundWasEnabled);

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
                    transmitRemoteDataDPID(&msg, NET_BROADCAST_DPID, false, true);
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
                unsigned char* blockData = data + receivedMsg->m_blockNbr
                                                 * GAME_TRANSMIT_PAYLOAD_SIZE;
                memcpy(blockData,
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
        // Mac checks path creation and data-fork opening separately, with a
        // fileError call for each. Windows File::open has one error path.
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
        delete[] newSave;
        data = temp;
    }

    char fileName[351];
    sprintf(fileName,
            DATA_COMPGEN(0x00660358, processSearchFoundFormat, "%s%s"),
            DATA_COMPGEN(0x00677d88, dataDirectoryPrefix, ".\\DATA\\"),
            g_config.m_rcFile);
    int handle = _open(fileName,
                       _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY,
                       _S_IWRITE);
    if (handle == -1)
        fileError(fileName);
    _write(handle, data, fileSize);
    _close(handle);

    delete[] blockReceived;
    delete[] data;

    if (g_advManager->getStatus() == baseManager::STATUS_ACTIVE) {
        g_advManager->overrideBottomView(
            advManager::BOTTOM_VIEW_DEFAULT, -1);
        g_advManager->updBottomView(1, 1, 1);
    }

    if (changeSounds != -1) {
        char restoreSoundWasEnabled = g_soundManager->enablePlayback();
        g_soundManager->switchAmbientMusic(changeSounds);
        g_soundManager->setPlaybackState(restoreSoundWasEnabled);
    }

    if (inGame && m_playerDisabled[g_netLocalGamePos])
        nextPlayer();

    return 1;
}

VA(0x004cc7d0, 0x5FE)
DC_ADDRESS(0x0b9408, 0x5c6)
MAC_ADDRESS(0x0e3dbc, 0x4f0)
void game::doNewTurn()
{
    int newHero;
    char sample[13];
    char temp[50];

    // Mac retains isHuman at 0xe3df4 and isLocalHuman at three later checks.
    if (!g_currentPlayer->isHuman()) {
        checkForTimeEvent();
        checkForTownEvent();
        return;
    }
    if (!g_currentPlayer->isLocalHuman())
        return;

    m_mapHeader.m_lossCondition.checkForTimeLimitExpired();
    m_mapHeader.m_victoryCondition.checkForTotalResources();
    m_mapHeader.m_victoryCondition.checkForTimeSurvival();
    checkEndGame(0);
    checkForTimeEvent();
    checkForTownEvent();

    if (g_currentPlayer->isLocalHuman())
        g_soundManager->setPlaybackState(1);
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

        if (g_currentPlayer->isLocalHuman()) {
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
        turnOffAIMusic();
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
        if (g_monthType == g_monthEffectNormal) {
            sprintf(g_text, g_newTurn[2], g_monthNames[g_monthTypeExtra]);
        } else if (g_monthType == g_monthEffectCreature) {
            strcpy(temp, getArmyName(g_monthTypeExtra, 1));
            temp[0] = toupper(temp[0]);
            sprintf(g_text, g_newTurn[3],
                    getArmyName(g_monthTypeExtra, 1), temp);
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

    // Mac doNewTurn retains turnOffAIMusic at 0:0xe420c and 0:0xe4294;
    // the earlier conditional sound enable remains a direct store.
    turnOffAIMusic();
    // Mac uses channel 2 here; Windows retail passes 3.
    launchSample(sample, 30000, 3);
    g_mouseManager->setPointer(0, mouseManager::DEFAULT_SET);
    g_advManager->m_advWindow->setBackgroundAnimation(1);
    normalDialog(g_text, 1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
    g_advManager->m_advWindow->setBackgroundAnimation(0);
}

VA(0x004ccdd0, 0x56)
DC_ADDRESS(0x0b99d0, 0x62)
MAC_ADDRESS(0x0e42ac, 0x40)
int game::getBoatsBuilt()
{
    int count = 0;
    for (unsigned int i = 0; i < m_boats.size(); i++) {
        if (m_boats[i].m_allocated)
            count++;
    }
    return count;
}

// Both retail bodies index the town list without the mutable getTown's -1
// sentinel arm. A read-only game view selects the existing const overload;
// Mac matches all 196 bytes, with both hasBuilding expansions preserved.
VA(0x004cce30, 0xB8)
DC_ADDRESS(0x0b9a34, 0xf0)
MAC_ADDRESS(0x0e42ec, 0xc4)
int game::getNumThievesGuilds(int whichPlayer)
{
    int count = 0;
    for (int i = 0; i < m_players[whichPlayer].m_numTowns; i++) {
        const game& gameState = *g_game;
        const town* currentTown = gameState.getTown(m_players[whichPlayer].m_townIds[i]);
        if (currentTown->hasBuilding(TAVERN_ID, false) ||
            (currentTown->m_type == TOWN_CASTLE &&
             currentTown->hasBuilding(EXTRA_1_ID, false))) {
            count++;
        }
    }
    return count;
}

VA(0x004ccef0, 0x23)
DC_ADDRESS(0x0b9b24, 0x30)
MAC_ADDRESS(0x0e43b0, 0x38)
void game::setMapSize(int width, int height)
{
    g_mapWidth = width;
    g_mapHeight = height;
    g_searchArray->close();
}

// Original: game::HeroIDToHeroPos; game.cpp:11221
DC_ADDRESS(0x0b9b54, 0x44)
int game::heroIdToHeroPos(playerData* player, int id)
{
    for (int i = 0; i < player->m_numHeroes; ++i) {
        if (player->m_heroes[i] == id)
            return i;
    }
    return -1;
}

// Original: game::TownIDToTownPos; game.cpp:11231
DC_ADDRESS(0x0b9b98, 0x6c)
int game::townIdToTownPos(playerData* player, int id)
{
    for (int i = 0; i < player->m_numTowns; ++i) {
        if (player->m_townIds[i] == id)
            return i;
    }
    return -1;
}

// Original: game::SetMarketArtifacts; game.cpp:11241
DC_ADDRESS(0x0b9c04, 0xa6)
MAC_ADDRESS(0x0e43e8, 0xa0)
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

// Original: game::SetSummoningGenerators; game.cpp:11252
DC_ADDRESS(0x0b9cac, 0xaa)
MAC_ADDRESS(0x0e4488, 0xe4)
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

// Original: game::SetupNewRumour; game.cpp:11445
DC_ADDRESS(0x0bac04, 0xa0)
MAC_ADDRESS(0x0e4d38, 0x64)
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

VA(0x004ccf20, 0x8E)
DC_ADDRESS(0x0b9d58, 0x122)
MAC_ADDRESS(0x0e456c, 0x1a4)
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
DC_ADDRESS(0x0b9e7c, 0x1c2)
MAC_ADDRESS(0x0e4710, 0x12c)
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

// DC 11388 returns after the category chain; Mac 0xe4920/0xe4964/
// 0xe49a8/0xe49e4 routes all four arms to one exit. Removing the prior
// second-arm-only return recovers Windows 81.5183% -> 85.6147%, with no
// sibling score changes. Eight roll-lifetime states give three reproduced
// objects; three exit states give three. Canonical getPlayerName/getName
// and getText paths stay intact (DC uses the older text operator[]).
// The human-name logical zero test in getName above completes the first
// nested name expansion and brings this body to 100%; other game scores
// and all available Mac comparisons are unchanged.
VA(0x004cd170, 0x59B)
DC_ADDRESS(0x0ba040, 0xbc4)
MAC_ADDRESS(0x0e483c, 0x4fc)
void game::setSpecialRumour()
{
    // Original DC locals: iRoll, value, iRetries, index, two iRoll2,
    // iLoc, point and cell. The rolls are separate integer acquisitions
    // (11360/11362, 11401/11403), not floating-point threshold temporaries.
    int roll = random(1, 100);
    if (roll < g_specialRumourChance && getCurrentTurn() > 1) {
        long value[8];
        int retries;
        signed char index[8];

        retries = 0;
        while (retries++ < g_specialRumourAttempts) {
            int roll2 = random(g_specialRumourFirstCategory,
                                  g_specialRumourLastCategory);
            getCategoryStats(roll2, value, index);
            sortStats(value, index);

            if (value[0] != value[1]) {
                if (roll2 == g_specialRumourFirstCategory) {
                    sprintf(m_currentRumour,
                            g_generalText->getText(
                                g_specialRumourCategoryText),
                            getPlayerName(index[0]));
                } else if (roll2
                           == g_specialRumourFirstCategory + 1) {
                    sprintf(m_currentRumour,
                            g_generalText->getText(
                                g_specialRumourCategoryText + 1),
                            getPlayerName(index[0]));
                } else if (roll2
                           == g_specialRumourFirstCategory + 2) {
                    sprintf(m_currentRumour,
                            g_generalText->getText(
                                g_specialRumourCategoryText + 2),
                            getPlayerName(index[0]));
                } else {
                    sprintf(m_currentRumour,
                            g_generalText->getText(
                                g_specialRumourCategoryText + 3),
                            getPlayerName(index[0]));
                }
                return;
            }
        }
    }

    if (!m_ultimateArtifactPresent) {
        setCannedRumour();
        return;
    }

    int roll2 = random(1, 100);
    if (roll2 <= g_specialRumourLocationChance) {
        int loc;

        if (static_cast<double>(m_ultimateArtifactX)
                    < static_cast<double>(m_mapHeader.m_size) * 0.33
            && static_cast<double>(m_ultimateArtifactX)
                   < static_cast<double>(m_mapHeader.m_size) * 0.33)
            loc = 7;
        else if (static_cast<double>(m_ultimateArtifactX)
                         < static_cast<double>(m_mapHeader.m_size) * 0.33
                     && static_cast<double>(m_ultimateArtifactX)
                            > static_cast<double>(m_mapHeader.m_size) * 0.66)
            loc = 5;
        else if (static_cast<double>(m_ultimateArtifactX)
                 < static_cast<double>(m_mapHeader.m_size) * 0.33)
            loc = 6;
        else if (static_cast<double>(m_ultimateArtifactX)
                         > static_cast<double>(m_mapHeader.m_size) * 0.66
                     && static_cast<double>(m_ultimateArtifactX)
                            < static_cast<double>(m_mapHeader.m_size) * 0.33)
            loc = 1;
        else if (static_cast<double>(m_ultimateArtifactX)
                         > static_cast<double>(m_mapHeader.m_size) * 0.66
                     && static_cast<double>(m_ultimateArtifactX)
                            > static_cast<double>(m_mapHeader.m_size) * 0.66)
            loc = 3;
        else if (static_cast<double>(m_ultimateArtifactX)
                 > static_cast<double>(m_mapHeader.m_size) * 0.66)
            loc = 2;
        else if (static_cast<double>(m_ultimateArtifactX)
                 < static_cast<double>(m_mapHeader.m_size) * 0.33)
            loc = 0;
        else if (static_cast<double>(m_ultimateArtifactX)
                 > static_cast<double>(m_mapHeader.m_size) * 0.66)
            loc = 4;
        else
            loc = 8;

        if (!m_ultimateArtifactZ) {
            sprintf(m_currentRumour,
                    g_generalText->getText(
                        g_specialRumourGrailAboveText),
                    g_directions[loc]);
        } else {
            sprintf(m_currentRumour,
                    g_generalText->getText(
                        g_specialRumourGrailBelowText),
                    g_directions[loc]);
        }
    } else {
        type_point point(m_ultimateArtifactX, m_ultimateArtifactY,
                                    m_ultimateArtifactZ);
        const NewmapCell* cell = g_advManager->getCell(point);
        sprintf(m_currentRumour,
                g_generalText->getText(g_specialRumourGrailObjectText),
                g_rumourTerrainDescriptions[cell->m_groundSet]);
    }
}

VA(0x004cd710, 0x200)
DC_ADDRESS(0x0baca4, 0x20c)
MAC_ADDRESS(0x0e4d9c, 0x2a8)
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

// Original: game::GiveTownEventReward; game.cpp:11499
// Both date arms of Complete checkForTownEvent 0x4cda10 expand this operation:
// select the town, require its owner to be local, then grant the two rewards.
DC_ADDRESS(0x0baeb0, 0x44)
MAC_ADDRESS(0x0e5044, 0x84)
void game::giveTownEventReward(const TTownEvent& thisEvent)
{
    town* thisTown = getTown(thisEvent.m_townNum);
    if (g_netLocalGamePos == thisTown->m_owner) {
        giveTimeEventReward(&thisEvent);
        thisTown->giveEventReward(thisEvent);
    }
}

VA(0x004cd910, 0xF5)
DC_ADDRESS(0x0baef4, 0xf8)
MAC_ADDRESS(0x0e50c8, 0x10c)  // unique body/order + 0x34-byte TTimedEvent stride
void game::checkForTimeEvent()
{
    int day = getCurrentTurn();

    for (unsigned int i = 0; i < m_worldMap.m_timedEventList.size(); ++i) {
        TTimedEvent* thisEvent = &m_worldMap.m_timedEventList[i];
        // Mac retains game::isHuman here at code0+0xe512c.
        if (!(isHuman(g_netLocalGamePos)
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

VA(0x004cda10, 0x164)
DC_ADDRESS(0x0bafec, 0xf8)
MAC_ADDRESS(0x0e51d4, 0x10c)
void game::checkForTownEvent()
{
    int day = getCurrentTurn();

    for (unsigned int i = 0; i < m_worldMap.m_townEventList.size(); ++i) {
        const TTownEvent& thisEvent = m_worldMap.m_townEventList[i];
        if (!(isHuman(g_netLocalGamePos)
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

VA(0x004cdb80, 0x231)
DC_ADDRESS(0x0bb0e4, 0x2fc)
MAC_ADDRESS(0x0e52e0, 0x2cc)
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

VA(0x004cddc0, 0x22)
DC_ADDRESS(0x0bb3e0, 0x3c)
MAC_ADDRESS(0x0e55ac, 0x38)
unsigned char game::getRandomLithExit(long color, type_point& result) const
{
    return getRandomLith(getLithExits(color), result, 0x2c, -1);
}

VA(0x004cddf0, 0x24)
DC_ADDRESS(0x0bb41c, 0x3e)
MAC_ADDRESS(0x0e55e4, 0x3c)
unsigned char game::getRandomLith(long color, long excluded, type_point& result) const
{
    return getRandomLith(getLiths(color), result, 0x2d, excluded);
}

VA(0x004cde20, 0x1D)
DC_ADDRESS(0x0bb45c, 0x32)
MAC_ADDRESS(0x0e5620, 0x30)
unsigned char game::getRandomWhirlpool(long excluded, type_point& result) const
{
    return getRandomLith(getWhirlpools(), result, 0x6f, excluded);
}

VA(0x004cde40, 0xE0)
DC_ADDRESS(0x0bb490, 0x19c)
MAC_ADDRESS(0x0e5650, 0x134)
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
// Member construction follows the shared class declaration order; SCampaign's
// ordinary constructor remains out of line. Mac 0xe5c2c..0xe60d4 repeatedly
// loads the scalar fill arguments from constant storage. std::fill_n recovers
// those reference-based fills, including their byte/int argument types, and
// raises Windows 91.3953% -> 99.2861% without flattening member constructors.
// Both retail builds clear cartographerFlags before assigning cartographerMask
// (Mac 0xe605c..0xe607c); that final ordering makes Windows byte-exact.
// The former counted-loop markers over-inlined bitset<8>'s default construction.
// No inline-depth control is needed with the recovered library operations.
VA(0x004cdf20, 0x585)
DC_ADDRESS(0x0bb62c, 0x6fc)
MAC_ADDRESS(0x0e5784, 0x97c)  // anchor-global
game::game()
{
    m_difficultyRating = 0;
    m_newCampaignStarted = 0;
    std::fill_n(m_saveFileName, sizeof(m_saveFileName), static_cast<char>(0));
    memset(&m_setup, 0, sizeof(m_setup));
    std::fill_n(m_playerDisabled, sizeof(m_playerDisabled), 0);
    m_day = 0;
    m_week = 0;
    m_month = 0;
    std::fill_n(m_heroAvailability, sizeof(m_heroAvailability), -1);

    std::bitset<8> allPlayers = ~std::bitset<8>();
    std::fill_n(m_heroPoolMap, static_cast<int>(HERO_COUNT), allPlayers);
    std::fill_n(m_artifactUsed, sizeof(m_artifactUsed), static_cast<unsigned char>(0));
    std::fill_n(m_artifactDisabled, sizeof(m_artifactDisabled), static_cast<unsigned char>(0));
    std::fill_n(m_obeliskFlags, sizeof(m_obeliskFlags), static_cast<signed char>(0));
    resetHolyGrail();
    m_gameVersion = 0;
    m_isCheater = 0;
    std::fill_n(m_currentRumour, sizeof(m_currentRumour), static_cast<char>(0));
    m_numObelisks = 0;
    std::fill_n(m_globalInfoFlags, sizeof(m_globalInfoFlags), 0);
    std::fill_n(m_borderTentVisitFlags, sizeof(m_borderTentVisitFlags), 0);
    std::fill_n(m_cartographerFlags, sizeof(m_cartographerFlags), 0);
    m_cartographerMask[0] = 0x100;
    m_cartographerMask[1] = 0xbf;
    m_cartographerMask[2] = 0x40;
    initializeGameData();
    std::fill_n(m_rumourState, sizeof(m_rumourState), static_cast<char>(0));
    m_isTutorial = 0;
    m_grailAsked = 0;
}

// E:\gamedcs\game.cpp:11746
// CodeView dc 0xbd5f4 marks the default constructor compgenx. The
// game::game array construction takes its address at +0x46; all work is
// implicit member initialization, including the artifact arrays and string.
VA_COMPGEN(0x004ce4b0, 0x68, CLASS_CTOR, HeroExtra)
MAC_COMPGEN_ADDRESS(0x0e66d4, 0x74, CLASS_CTOR, HeroExtra)

VA_COMPGEN(0x004ce520, 0x4A, IMPLICIT_DTOR, HeroExtra)
MAC_COMPGEN_ADDRESS(0x0e667c, 0x58, IMPLICIT_DTOR, HeroExtra)

// CodeView dc 0xbd630: CV_fldattr_t.compgenx marks this destructor
// as implicit. Its retained retail body performs only base/member teardown.
VA_COMPGEN(0x004ce570, 0x32, IMPLICIT_DTOR, playerData)
MAC_COMPGEN_ADDRESS(0x0e65b0, 0x68, IMPLICIT_DTOR, playerData)

// E:\gamedcs\game.cpp:11749
VA(0x004ce5b0, 0x346)
DC_ADDRESS(0x0bbd28, 0x140)
MAC_ADDRESS(0x0e67ac, 0x3f4)
game::~game()
{
    clearEventRecords();
}

VA(0x004ce900, 0x3B)
DC_ADDRESS(0x0bbe68, 0x7c)
MAC_ADDRESS(0x0e6ba0, 0x54)
boat* game::getHeroBoat(int id, unsigned char occupied)
{
    for (boat* i = m_boats.begin(); i != m_boats.end(); i++) {
        if (i->m_allocated && i->m_occupyingHero == id && i->m_occupied == occupied)
            return i;
    }
    return 0;
}

VA(0x004ce940, 0x27)
DC_ADDRESS(0x0bbee4, 0xe8)
MAC_ADDRESS(0x0e6bf4, 0x48)
bool game::isHuman(int gamePos) const
{
    if (gamePos >= 8 || gamePos < 0)
        gamePos = 0;
    return m_players[gamePos].isHuman();
}

VA(0x004ce970, 0x3C)
DC_ADDRESS(0x0bbfcc, 0x44)
MAC_ADDRESS(0x0e6c3c, 0x4c)
bool game::isLocalHuman(int gamePos) const
{
    if (gamePos >= 8 || gamePos < 0)
        return false;
    return m_players[gamePos].isLocalHuman();
}

VA(0x004ce9b0, 0x6A)
DC_ADDRESS(0x0bc010, 0x28)
MAC_ADDRESS(0x0e6c88, 0x3c)
playerData* game::getLocalPlayer()
{
    return &m_players[getLocalPlayerGamePos()];
}

VA(0x004cea20, 0x4E)
DC_ADDRESS(0x0bc038, 0x88)
MAC_ADDRESS(0x0e6cc4, 0x90)
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

VA(0x004cea70, 0xE7)
DC_ADDRESS(0x0bc0c0, 0x13a)
MAC_ADDRESS(0x0e6d54, 0xe0)
type_point game::getPuzzleOrigin() const
{
    type_point result(m_ultimateArtifactX - 9,
                      m_ultimateArtifactY - 8,
                      m_ultimateArtifactZ);

    sRand(m_ultimateArtifactY * 81901
          + m_ultimateArtifactX * 67843 + 79451);
    // DC lines 11822/11823 and Mac call sRandom; Complete folds its retail
    // body with random at 0x50b230.
    result.m_x += sRandom(-2, 2);
    result.m_y += sRandom(-2, 2);
    return result;
}

VA(0x004ceb60, 0xBD)
DC_ADDRESS(0x0bc1fc, 0x40)
MAC_ADDRESS(0x0e6e34, 0x48)
char* game::getPlayerName(int gamePos)
{
    if (gamePos >= 8 || gamePos < 0)
        gamePos = 0;
    return m_players[gamePos].getName();
}

VA(0x004cec20, 0x25)
DC_ADDRESS(0x0bc23c, 0x7c)
MAC_ADDRESS(0x0e6e7c, 0xe4)
int game::getGamePosFromDPID(unsigned long dpid) const
{
    for (int i = 0; i < 8; i++) {
        if (m_players[i].m_dpid == dpid)
            return i;
    }
    return -1;
}

VA(0x004cec50, 0x3E)
DC_ADDRESS(0x0bc2b8, 0x46)
MAC_ADDRESS(0x0e6f60, 0x64)
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

VA(0x004cec90, 0x18)
DC_ADDRESS(0x0bc300, 0x20)
MAC_ADDRESS(0x0e6fc4, 0x30)
bool game::isMultiplayer() const
{
    if (g_remoteOn || g_mpNetProtocol == MP_HOTSEAT)
        return true;
    return false;
}

VA(0x004cecb0, 0x81)
MAC_ADDRESS(0x0e7054, 0xc4)
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

VA(0x004ced40, 0x1D0)
MAC_ADDRESS(0x0e7118, 0x3c)  // sole caller 0x5013b0 + game+0x4e7bc vector layout
void game::recordMonsterIdentifier(int identifier, type_point point)
{
    MonsterIdentifier record;
    record.m_identifier = identifier;
    record.m_point = point;
    m_monsterIdentifiers.push_back(record);
}

// Quest-monster setup resolves the most recently recorded object with this
// identifier; absent objects use the packed all-minus-one point sentinel.
VA(0x004cef10, 0x68)
MAC_ADDRESS(0x0e7154, 0x78)  // sole semantic caller 0x56ef20 + reverse 8-byte walk
type_point game::gameFn004CEF10(int identifier)
{
    for (unsigned int i = m_monsterIdentifiers.size(); i-- != 0;) {
        if (m_monsterIdentifiers[i].m_identifier == identifier)
            return m_monsterIdentifiers[i].m_point;
    }

    return type_point(-1, -1, -1);
}

VA_COMPGEN(0x004bdf80, 0x1B1, IMPLICIT_DTOR, SavedGameHeader)
MAC_COMPGEN_ADDRESS(0x0d23c8, 0x140, IMPLICIT_DTOR, SavedGameHeader)

// Sign's one-string destructor is emitted out of line and is the callee used
// by the vector helpers below.
VA_COMPGEN(0x004b9230, 0x3E, IMPLICIT_DTOR, Sign)
MAC_COMPGEN_ADDRESS(0x0e8408, 0x58, IMPLICIT_DTOR, Sign)

VA_COMPGEN(0x004c4df0, 0x3E, PAIR_CONST_INT_DTOR, type_map_hero_info)

VA_COMPGEN(0x004caa40, 0x26, IMPLICIT_DTOR, TPickRandomTownName)
MAC_COMPGEN_ADDRESS(0x0f0ef8, 0x74, IMPLICIT_DTOR, TPickRandomTownName)

VA_COMPGEN(0x004cbcf0, 0x4B, IMPLICIT_DTOR, CGameTransferDlg)
MAC_COMPGEN_ADDRESS(0x0e3134, 0x7c, IMPLICIT_DTOR, CGameTransferDlg)

// InitNewGame's exception path retains Dinkumware's string-taking
// std::logic_error constructor. The late STL anchor emits the identical named
// public until that large caller is reconstructed.
VA_COMPGEN(0x004c3090, 0x162, CLASS_CTOR, logic_error)

VA_COMPGEN(0x004cef80, 0x12, BITSET_SUBSCRIPT, Bitset145)

VA_COMPGEN(0x004cf010, 0x2E, BITSET_COUNT, Bitset145)

// The shared bitset<4>::test at 0x4cf960 expands here and remains
// emitted in singleselectionwindow, alongside its retained _Xran body.
VA_COMPGEN(0x004cf9a0, 0x63, BITSET_SET, Bitset144)

VA_COMPGEN(0x004cf0b0, 0x3B, VECTOR_DTOR, TownExtra)
MAC_COMPGEN_ADDRESS(0x0e92b0, 0x64, VECTOR_DTOR, TownExtra)

VA_COMPGEN(0x004cf0f0, 0x2D6, VECTOR_RESIZE, TBlackMarket)

VA_COMPGEN(0x004cf3d0, 0x3B, VECTOR_DTOR, town)
MAC_COMPGEN_ADDRESS(0x0e93b4, 0x64, VECTOR_DTOR, town)

VA_COMPGEN(0x004cf410, 0x2A1, VECTOR_RESIZE, town)

VA_COMPGEN(0x004cf6c0, 0x23, VECTOR_SIZE, town)

VA_COMPGEN(0x004cf6f0, 0x38, VECTOR_DTOR, Sign)
MAC_COMPGEN_ADDRESS(0x0e9418, 0x64, VECTOR_DTOR, Sign)

VA_COMPGEN(0x004cf730, 0x13, VECTOR_SIZE, mine)

VA_COMPGEN(0x004cf750, 0x21, VECTOR_SIZE, boat)

VA_COMPGEN(0x004cf780, 0x38, VECTOR_DTOR, type_creature_bank)
MAC_COMPGEN_ADDRESS(0x0e9670, 0x64, VECTOR_DTOR, type_creature_bank)

VA_COMPGEN(0x004cf7c0, 0x38, VECTOR_DTOR, TRumour)
MAC_COMPGEN_ADDRESS(0x0e96d4, 0x64, VECTOR_DTOR, TRumour)

VA_COMPGEN(0x004cf800, 0x67, BITSET_REFERENCE_ASSIGN, Bitset5)

VA_COMPGEN(0x004cf870, 0x53, BITSET_CTOR, Bitset28)

VA_COMPGEN(0x004cf8d0, 0x1C, BITSET_COUNT, Bitset28)

VA_COMPGEN(0x004cf8f0, 0x67, BITSET_REFERENCE_ASSIGN, Bitset28)

// SCampaign::operator= reaches this capacity through vector<vector<hero>>;
// the vector<vector<type_artifact>> spelling folds onto the same body.
VA_COMPGEN(0x004cfa40, 0x13, VECTOR_CAPACITY, hero_vector)

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
MAC_COMPGEN_ADDRESS(0x124a2c, 0x58, IMPLICIT_DTOR, TownExtra)

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

// Mac retains the three contiguous-element loadVector specializations.
// game::load calls the point body four times, then the long and university
// bodies once each; Windows expands these calls in its caller.
#if 0  // @carcass -- claim-only template instances

MAC_ADDRESS(0x0e7688, 0x9c)
bool loadVector(TAbstractFile* infile, std::vector<type_point>& destVector)
{
    // @stub
}

MAC_ADDRESS(0x0e7724, 0xa0)
bool loadVector(TAbstractFile* infile, std::vector<long>& destVector)
{
    // @stub
}

MAC_ADDRESS(0x0e77c4, 0x9c)
bool loadVector(TAbstractFile* infile,
                std::vector<type_university>& destVector)
{
    // @stub
}

// This body calls generator::load at 0xca1a4 for every element.
MAC_ADDRESS(0x0e7a34, 0xbc)
bool loadObjectVector(TAbstractFile* infile,
                      std::vector<generator>& destVector)
{
    // @stub
}
#endif

// Retained instantiation of the canonical template at game.cpp2733.
#if 0  // @carcass -- claim-only template instance

VA(0x004d2870, 0x24D)
MAC_ADDRESS(0x0e7af0, 0xbc)
bool loadObjectVector(TAbstractFile* infile,
                      std::vector<type_creature_bank>& destVector)
{
    // @stub
}
#endif

// The retained template instances share the active definitions above.
#if 0  // @carcass -- claim-only template instances

MAC_ADDRESS(0x0e7494, 0xbc)  // game::save m_generators; elements call generator::save
bool saveObjectVector(TAbstractFile* outfile, std::vector<generator>& srcVector)
{
    // @stub
}

VA(0x004d2ac0, 0x60)
MAC_ADDRESS(0x0e7550, 0x9c)  // point instance; retail folds long here
bool saveVector(TAbstractFile* outfile, std::vector<type_point>& srcVector)
{
    // @stub
}

MAC_ADDRESS(0x0e75ec, 0x9c)  // type_creature_bank artifact tail
bool loadVector(TAbstractFile* infile, std::vector<TArtifact>& destVector)
{
    // @stub
}

MAC_ADDRESS(0x0e7860, 0x9c)  // type_creature_bank artifact tail
bool saveVector(TAbstractFile* outfile, std::vector<TArtifact>& srcVector)
{
    // @stub
}

// Mac keeps the long-vector writer separate at game::save 0xd439c;
// retail folds it into the point-vector body above.
MAC_ADDRESS(0x0e78fc, 0x9c)
bool saveVector(TAbstractFile* outfile, std::vector<long>& srcVector)
{
    // @stub
}

VA(0x004d2b20, 0x60)
MAC_ADDRESS(0x0e7998, 0x9c)  // university stride and sole Save call
bool saveVector(TAbstractFile* outfile, std::vector<type_university>& srcVector)
{
    // @stub
}
#endif

#if 0  // @carcass -- claim-only template instance

VA(0x004d2b80, 0x102)
MAC_ADDRESS(0x0e7bac, 0xbc)
bool saveObjectVector(TAbstractFile* outfile,
                      std::vector<type_creature_bank>& srcVector)
{
    // @stub
}
#endif

// Original: game::GetLastHuman; game.cpp:11869
DC_ADDRESS(0x0bc320, 0x64)
MAC_ADDRESS(0x0e6ff4, 0x60)
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

// The former 0x4b61f0 mnemonic-only pairing was a different specialization:
// ForceFeedback's registered map cleanup calls it, and its erase target is
// the CImmEnclosure*/RECT tree at 0x4b7200, not this hero-info tree.
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

// Slot 5 of that zip: vector<hero>::~vector, the row ~SCampaign,
// vector<vector<hero>>::operator= and vector<vector<hero>>::insert all reach.
VA_COMPGEN(0x0045fc50, 0x3B, VECTOR_DTOR, hero)

// hero declares no destructor, so this is the implicit teardown its own
// vector<hero>::~vector and scalar-deleting dtor call.
VA_COMPGEN(0x0045fc90, 0x4A, IMPLICIT_DTOR, hero)
MAC_COMPGEN_ADDRESS(0x09d060, 0x58, IMPLICIT_DTOR, hero)

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
