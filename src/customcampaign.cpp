// 11 functions in the Dreamcast roster; 20 compiler-generated $-thunks
// omitted. Retail's currently proven 0x8b200..0x8b2d7 unit span contains the
// two exact methods below. The flanking cursor/customcampaign and
// customcampaign/dialogbox gaps remain ambiguous, so the older-revision
// carcass is not force-claimed merely from roster order.
#include "text.h"
#include "va.h"
#include "homm3_minmax.h"
#include "bitset_iterator.h"

#include <algorithm>
#ifdef _WIN32
#include <direct.h>
#endif
#include <fstream>
#include <string.h>
#include <strstream>

#include "customcampaign.h"

#include "abstractfile.h"
#include "artifact.h"
#include "packed_bits.h"
#include "campaignbrief.h"
#include "castle.h"
#include "customcampaign_legacy.h"
#include "gzinflatebuf.h"
#include "hero.h"
#include "bitmap16.h"
#include "campaignmap.h"
#include "creaturetype.h"
#include "font.h"
#include "game.h"
#include "inputmgr.h"
#include "kb.h"
#include "kbwin.h"
#include "message.h"
#include "misc.h"
#include "prefs.h"
#include "resourcemanager.h"
#include "sample.h"
#include "smackmgr.h"
#include "soundmgr.h"
#include "sskilltraits.h"
#include "textresource.h"
#include "town.h"
#include "winmgr.h"

// Initial contents recovered from the pinned Complete image.
DATA(0x0063d734) const int g_campaignVideoIds[101] = { 38, 39, 40, 45, 46, 47, 41, 42, 43, 48, 49, 50, 51, 55, 62, 56, 57, 58, 52, 53, 54, 59, 60, 61, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 102, 103, 104, 105, 106, 107, 113, 114, 115, 116, 117, 97, 98, 99, 100, 101, 108, 109, 110, 111, 112, 118, 119, 120, 121, 122, 136, 137, 138, 139, 140, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135 };
DATA(0x00675be8) const char* g_campaignVideoSounds[101] = { "g1a", "g1b", "g1c", "e1a", "e1b", "e1c", "n1a", "n1b", "n1c_d", "g2a", "g2b", "g2c", "g2d", "e2a", "e2Ae", "e2b", "e2c", "e2d", "g3a", "g3b", "g3c", "s1a", "s1b", "s1c", "ABvoAB1", "ABvoAB2", "ABvoAB3", "ABvoAB4", "ABvoAB5", "ABvoAB6", "ABvoAB7", "ABvoAB8", "ABvoAB9", "ABvoDB1", "ABvoDB2", "ABvoDB3", "ABvoDB4", "ABvoDB5", "ABvoDS1", "ABvoDS2", "ABvoDS3", "ABvoDS4", "ABvoDS5", "ABvoFL1", "ABvoFL2", "ABvoFL3", "ABvoFL4", "ABvoFL5", "ABvoFW1", "ABvoFW2", "ABvoFW3", "ABvoFW4", "ABvoFW5", "ABvoPF1", "ABvoPF2", "ABvoPF3", "ABvoPF4", "H3x2BBa", "H3x2BBb", "H3x2BBc", "H3x2BBd", "H3x2BBe", "H3x2BBf", "H3x2ELa", "H3x2ELb", "H3x2ELc", "H3x2ELd", "H3x2ELe", "H3x2HSa", "H3x2HSb", "H3x2HSc", "H3x2HSd", "H3x2HSe", "H3x2NBa", "H3x2NBb", "H3x2NBc", "H3x2NBd", "H3x2NBe", "H3x2RNa", "H3x2RNb", "H3x2RNc", "H3x2RNd", "H3x2RNe", "H3x2SPa", "H3x2SPb", "H3x2SPc", "H3x2SPd", "H3x2SPe", "H3x2UAa", "H3x2UAb", "H3x2UAc", "H3x2UAd", "H3x2UAe", "H3x2UAf", "H3x2UAg", "H3x2UAh", "H3x2UAi", "H3x2UAj", "H3x2UAk", "H3x2UAl", "H3x2UAm" };

// Retail initial data; dimensions follow the typed table consumers.
DATA(0x006755b8) const char* g_campaignBuildingIconNames[9][44] = {
    {
    "BoCsMag1.pcx", "BoCsMag2.pcx", "BoCsMag3.pcx", "BoCsMag4.pcx", "BoCsMag5.pcx", "BoCsTav1.pcx", "BoCsDock.pcx", "BoCsCas1.pcx",
    "BoCsCas2.pcx", "BoCsCas3.pcx", "BoCsHal1.pcx", "BoCsHal2.pcx", "BoCsHal3.pcx", "BoCsHal4.pcx", "BoCsMrk1.pcx", "BoCsMrk2.pcx",
    "BoCsBlak.pcx", "BoCsLite.pcx", "BoCsGr1H.pcx", "BoCsGr2H.pcx", "", "BoCsCv2S.pcx", "BoCsTav2.pcx", "",
    "", "", "BoCsHoly.pcx", "", "", "", "BoCsPik1.pcx", "BoCsMrk1.pcx",
    "BoCsGr1.pcx", "BoCsSwd1.pcx", "BoCsMon1.pcx", "BoCsCv1.pcx", "BoCsAng1.pcx", "BoCsPik2.pcx", "BoCsMrk2.pcx", "BoCsGr2.pcx",
    "BoCsSwd1.pcx", "BoCsMon2.pcx", "BoCsCv2.pcx", "BoCsAng2.pcx"
},
    {
    "BoRMag1.pcx", "BoRMag2.pcx", "BoRMag3.pcx", "BoRMag4.pcx", "BoRMag5.pcx", "BoRTav.pcx", "", "BoRCas1.pcx",
    "BoRCas2.pcx", "BoRCas3.pcx", "BoRHal1.pcx", "BoRHal2.pcx", "BoRHal3.pcx", "BoRHal4.pcx", "BoRMrk1.pcx", "BoRMrk2.pcx",
    "BoRAid.pcx", "BoRGar1.pcx", "BoRDwf1h.pcx", "BoRDwf2h.pcx", "", "BoRGar2.pcx", "", "",
    "BoRTre1h.pcx", "BoRTre2h.pcx", "BoRHoly.pcx", "", "", "", "BoRCen1.pcx", "BoRDwf1.pcx",
    "BoRElf1.pcx", "BoRPeg1.pcx", "BoRTre1.pcx", "BoRUni1.pcx", "BoRDra1.pcx", "BoRCen2.pcx", "BoRDwf2.pcx", "BoRElf2.pcx",
    "BoRPeg2.pcx", "BoRTre2.pcx", "BoRUni2.pcx", "BoRDra2.pcx"
},
    {
    "BoTGld1.pcx", "BoTGld2.pcx", "BoTGld3.pcx", "BoTGld4.pcx", "BoTGld5.pcx", "BoTTav.pcx", "", "BoTCas1.pcx",
    "BoTCas2.pcx", "BoTCas3.pcx", "BoTHal1.pcx", "BoTHal2.pcx", "BoTHal3.pcx", "BoTHal4.pcx", "BoTMark.pcx", "BoTMarkS.pcx",
    "BoTBlack.pcx", "BoTMarkA.pcx", "BoTGa1H.pcx", "BoTGa2h.pcx", "", "BoTCasW.pcx", "BoTGldL.pcx", "BoTGldW.pcx",
    "", "", "BoTHoly.pcx", "", "", "", "BoTGrem1.pcx", "BoTGar1.pcx",
    "BoTGolm1.pcx", "BoTMag1.pcx", "BoTGen1.pcx", "BoTNaga1.pcx", "BoTTit1.pcx", "BoTGrem2.pcx", "BoTGar2.pcx", "BoTGolm2.pcx",
    "BoTMag2.pcx", "BoTGen2.pcx", "BoTNaga2.pcx", "BoTTit2.pcx"
},
    {
    "BoIMag1.pcx", "BoIMag2.pcx", "BoIMag3.pcx", "BoIMag4.pcx", "BoIMag5.pcx", "BoITav.pcx", "", "BoICas1.pcx",
    "BoICas2.pcx", "BoICas3.pcx", "BoIHal1.pcx", "BoIHal2.pcx", "BoIHal3.pcx", "BoIHal4.pcx", "BoIMrk1.pcx", "BoIMrk2.pcx",
    "BoIBlak.pcx", "", "BoIImpH.pcx", "BoIImp2H.pcx", "", "BoICasB.pcx", "BoICasG.pcx", "BoIMagO.pcx",
    "BoIHndH.pcx", "BoIHnd2H.pcx", "BoIHoly.pcx", "", "", "", "BoIImp1.pcx", "BoIGog1.pcx",
    "BoIHnd1.pcx", "BoIDmn1.pcx", "BoIPit1.pcx", "BoIEfr1.pcx", "BoIDvl1.pcx", "BoIImp2.pcx", "BoIGog2.pcx", "BoIHnd2.pcx",
    "BoIDmn2.pcx", "BoIPit2.pcx", "BoIEfr2.pcx", "BoIDvl2.pcx"
},
    {
    "BoNmage1.pcx", "BoNmage2.pcx", "BoNmage3.pcx", "BoNmage4.pcx", "BoNmage5.pcx", "BoNtav.pcx", "BoNship.pcx", "BoNcast1.pcx",
    "BoNcast2.pcx", "BoNcast3.pcx", "BoNhall1.pcx", "BoNhall2.pcx", "BoNhall3.pcx", "BoNhall4.pcx", "BoNmark1.pcx", "BoNmark2.pcx",
    "BoNsmith.pcx", "BoNshrod.pcx", "BoNskelH.pcx", "BoNskelH.pcx", "", "BoNnecro.pcx", "BoNskelT.pcx", "",
    "", "", "BoNholyG.pcx", "", "", "", "BoNskel1.pcx", "BoNzomb1.pcx",
    "BoNwigh1.pcx", "BoNvamp1.pcx", "BoNlich1.pcx", "BoNbkni1.pcx", "BoNbone1.pcx", "BoNskel2.pcx", "BoNzomb2.pcx", "BoNwigh2.pcx",
    "BoNvamp2.pcx", "BoNlich2.pcx", "BoNbkni2.pcx", "BoNbone2.pcx"
},
    {
    "BoDmage1.pcx", "BoDmage2.pcx", "BoDmage3.pcx", "BoDmage4.pcx", "BoDmage5.pcx", "BoDtav.pcx", "", "BoDcas1.pcx",
    "BoDcas2.pcx", "BoDcas3.pcx", "BoDhall1.pcx", "BoDhall2.pcx", "BoDhall3.pcx", "BoDhall4.pcx", "BoDmark1.pcx", "BoDmark2.pcx",
    "BoDsmith.pcx", "BoDmarkA.pcx", "BoDtrogH.pcx", "BoDtrogH.pcx", "", "BoDvort.pcx", "BoDport.pcx", "BoDacad.pcx",
    "", "", "BoDholy.pcx", "", "", "", "BoDtrog1.pcx", "BoDharp1.pcx",
    "BoDbeh1.pcx", "BoDmedu1.pcx", "BoDmino1.pcx", "BoDmant1.pcx", "BoDdrag1.pcx", "BoDtrog2.pcx", "BoDharp2.pcx", "BoDbeh2.pcx",
    "BoDmedu2.pcx", "BoDmino2.pcx", "BoDmant2.pcx", "BoDdrag2.pcx"
},
    {
    "BoSmage1.pcx", "BoSmage2.pcx", "BoSmage3.pcx", "BoSmage4.pcx", "BoSmage5.pcx", "BoStav1.pcx", "", "BoScas1.pcx",
    "BoScas2.pcx", "BoScas3.pcx", "BoShall1.pcx", "BoShall2.pcx", "BoShall3.pcx", "BoShall4.pcx", "BoSmrk1.pcx", "BoSmrk2.pcx",
    "BoSblak1.pcx", "BoSescap.pcx", "BoSgob1h.pcx", "BoSgob2h.pcx", "", "BoSmrk1c.pcx", "BoSblak2.pcx", "BoSvahal.pcx",
    "", "", "BoSholy.pcx", "", "", "", "BoSgob1.pcx", "BoSwolf1.pcx",
    "BoSorc1.pcx", "BoSogre1.pcx", "BoSroc1.pcx", "BoScyc1.pcx", "BoSbeh1.pcx", "BoSgob2.pcx", "BoSwolf2.pcx", "BoSorc2.pcx",
    "BoSogre2.pcx", "BoSroc2.pcx", "BoScyc2.pcx", "BoSbeh2.pcx"
},
    {
    "BoFmage1.pcx", "BoFmage2.pcx", "BoFmage3.pcx", "BoFmage4.pcx", "BoFmage5.pcx", "BoFtav.pcx", "BoFship.pcx", "BoFcast1.pcx",
    "BoFcast2.pcx", "BoFcast3.pcx", "BoFhall1.pcx", "BoFhall2.pcx", "BoFhall3.pcx", "BoFhall4.pcx", "BoFmark1.pcx", "BoFmark2.pcx",
    "BoFapoth.pcx", "BoFcage.pcx", "BoFgno1h.pcx", "BoFgno2h.pcx", "", "BoFcastD.pcx", "BoFcastA.pcx", "",
    "", "", "BoFgrail.pcx", "", "", "", "BoFgnol1.pcx", "BoFlizr1.pcx",
    "BoFfly1.pcx", "BoFbas1.pcx", "BoFgorg1.pcx", "BoFwyvr1.pcx", "BoFhydr1.pcx", "BoFgnol2.pcx", "BoFlizr2.pcx", "BoFfly2.pcx",
    "BoFbas2.pcx", "BoFgorg2.pcx", "BoFwyvr2.pcx", "BoFhydr2.pcx"
},
    {
    "BoEgld1.pcx", "BoEgld2.pcx", "BoEgld3.pcx", "BoEgld4.pcx", "BoEgld5.pcx", "BoEtav.pcx", "BoEship.pcx", "BoEcast1.pcx",
    "BoEcast2.pcx", "BoEcast3.pcx", "BoEhall1.pcx", "BoEhall2.pcx", "BoEhall3.pcx", "BoEhall4.pcx", "BoEmark1.pcx", "BoEmarkS.pcx",
    "BoEblack.pcx", "BoEmarkA.pcx", "BoDhrd1.pcx", "BoDhrd2.pcx", "", "BoEuniv.pcx", "", "",
    "", "", "BoEgrail.pcx", "", "", "", "BoEdn_0.pcx", "BoEdn_1.pcx",
    "BoEdn_2.pcx", "BoEdn_3.pcx", "BoEdn_4.pcx", "BoEdn_5.pcx", "BoEdn_6.pcx", "BoEup_0.pcx", "BoEup_1.pcx", "BoEup_2.pcx",
    "BoEup_3.pcx", "BoEup_4.pcx", "BoEup_5.pcx", "BoEup_6.pcx"
}
};
DATA(0x0066c218) const SCampaignMusicCue* g_campaignMusicTraits = g_campaignMusicCues;

// Scenario ordinals used when the fixed legacy matrices are promoted to the
// current variable-length CampaignScenarioInfo vector.
DATA(0x0063d8c8) static int g_legacyCampaignScenarioIndices[7][4];

// Retail .data 0x66c218 is a reference cell (the akHeroTraits pattern)
// holding the campaign music table at 0x66c090; only StartMusic and
// MapTextStruct::Play read the cell, so customcampaign.obj owns it.


// The campaign file version at which a scenario's crossover-artifact plane
// widened from 129 bits to 144; ScenarioStruct::Read still reads and widens
// the narrow plane below it.
static const int g_campaignVersionWideArtifacts = 6;
static const int g_crossoverCreatureBits = 145;
static const int g_crossoverArtifactBits = 144;
static const int g_crossoverLegacyArtifactBits = 129;

static const int g_crossoverPrimaryArtifactSlots = 16;
static const int g_crossoverEquippedArtifactSlots = 19;
static const int g_crossoverPrimarySkills = 4;
static const int g_crossoverSplitCampaign = 9;
static const int g_crossoverSplitFirstScenario = 0;
static const int g_crossoverSplitLastScenario = 1;
static const int g_crossoverBonusCampaign = 8;
static const int g_crossoverBonusScenario = 3;
static const int g_crossoverBonusHero = 151;
static const int g_crossoverBonusPortrait = 153;
static const int g_crossoverBonusAmount = 4;
static const int g_crossoverPatrolCampaign = 7;
static const int g_crossoverPatrolFirstScenario = 6;
static const int g_crossoverPatrolLastScenario = 7;
static const int g_crossoverPatrolHero = 155;
static const int g_crossoverPatrolRadius = 10;
static const int g_crossoverSecondarySkills = 28;
static const int g_crossoverBackpackSlots = 64;

// The three scenario overrides the bonus appliers carry, all four values
// retail's: the creature bonus hands its stack to a faction-matching town
// on campaign 1's first map and campaign 3's second, and the building
// bonus grants a zero building index to EVERY town on campaign 1's fourth
// map. Names are role inventions.
static const int g_creatureBonusTownCampaignA = 1;
static const int g_creatureBonusTownScenarioA = 0;
static const int g_creatureBonusTownCampaignB = 3;
static const int g_creatureBonusTownScenarioB = 1;
static const int g_buildingBonusAllTownsCampaign = 1;
static const int g_buildingBonusAllTownsScenario = 3;
// Building indices from 0x25 up are the upgraded dwellings; granting one
// also grants the base row seven slots below it.
static const int g_buildingBonusFirstUpgrade = 0x25;
static const int g_buildingBonusUpgradeStride = 7;

// MapTextStruct::Play's own domain. Every value is retail's; the names are
// ROLE inventions - no Dreamcast row survives for this Complete-only body.
// The two video ordinals that select the lowered origin are the compare set
// the entry chain tests (`== 0x11`, `>= 0x18 && != 0x45`), and the id the
// sixth VideoOpen argument is switched on happens to share the lowered
// origin's value without being the same quantity.
static const int g_campaignVideoLowered = 0x11;
static const int g_campaignVideoFirstLowered = 0x18;
static const int g_campaignVideoRaised = 0x45;
static const int g_campaignVideoLoweredY = 100;
static const int g_campaignVideoNoFade = 100;
// The subtitle strip: 608 wide at the screen's (96, 510), 82 tall, scrolled
// one line at a time. The strip bitmap is one font line taller than the text
// so the first line can slide in.
static const int g_campaignSubtitleWidth = 0x260;
static const int g_campaignSubtitleHeight = 0x52;
static const int g_campaignSubtitleX = 0x60;
static const int g_campaignSubtitleY = 0x1fe;
static const int g_campaignSubtitleScrollMargin = 0x4d;
static const int g_campaignSubtitleColor = 0x11f;
static const int g_campaignScrollInterval = 0x8e;
static const int g_campaignLingerMs = 3000;
static const int g_campaignVideoLingerMs = 5000;
static const int g_campaignSpeechSampleStatus = 4;
// The one key the player may press without ending the sequence.
static const int g_campaignSkipKey = 0x3e;

// The two per-video tables MapTextStruct::Play indexes with `video`. Each is
// referenced from exactly one site in the whole image - this body - so
// customcampaign.obj owns both.



// The TAbstractFile view of a streambuf: Read is sgetn, Write is sputn.
// Size 8 is byte-proven by every stack instance (vftable, streambuf*).
class TStreamBufFile : public TAbstractFile {
public:
    TStreamBufFile(std::streambuf* newBuffer) : m_buffer(newBuffer) {}
    virtual int read(void* data, int size);         // 0x483f10
    virtual int write(const void* data, int size);  // 0x483f30

    std::streambuf* m_buffer;  // +4
};

// The campaign stream adapter owns these two virtuals and its local
// constructor above. Retail 0x63dacc supplies the read/write slots.
VA(0x00483f10, 0x17)
int TStreamBufFile::read(void* data, int size)
{
    return m_buffer->sgetn(static_cast<char*>(data), size);
}

VA(0x00483f30, 0x17)
int TStreamBufFile::write(const void* data, int size)
{
    return m_buffer->sputn(static_cast<const char*>(data), size);
}

// Original: hero_power; game.cpp:3275, dc 0xa8ba0
// The primary-total plus 28 secondary-skill sum moved with campaign hero
// sorting into this TU. Retail 0x483f50 retains the same hero-pointer helper.
VA(0x00483f50, 0x26)
int heroPower(hero* candidate)
{
    int primary = candidate->getPrimarySkillTotal();
    int skills = 0;
    for (int skill = 0; skill < g_crossoverSecondarySkills; ++skill)
        skills += candidate->m_skillLevel[skill];
    return skills + primary;
}

struct CrossoverHeroStronger {
    bool operator()(hero& lhs, hero& rhs) const;
};

// DC compare_heroes (game.cpp:3288, dc 0xa8c6c) returns a signed qsort
// difference after score and experience. Complete uses this bool predicate
// on hero references and adds hero ID as the final tie-break for std::sort.
VA(0x00483f80, 0x9B)  // retained written predicate; score/experience/hero-id ordering
bool CrossoverHeroStronger::operator()(hero& lhs, hero& rhs) const
{
    int leftValue = heroPower(&lhs);
    int rightValue = heroPower(&rhs);
    if (leftValue != rightValue)
        return leftValue > rightValue;
    leftValue += lhs.m_experience;
    rightValue += rhs.m_experience;
    if (leftValue != rightValue)
        return leftValue > rightValue;
    return lhs.m_id > rhs.m_id;
}

// --- the eight campaign start bonuses ---

VA_COMPGEN(0x00484020, 0x23, SCALAR_DELETING_DTOR, TCampaignBonus)

VA(0x00484050, 0x3D)
void TCampaignSpellBonus::read(TAbstractFile* file)
{
    {
        short heroId;
        file->read(&heroId, sizeof(short));
        m_hero = heroId;
    }
    {
        unsigned char spell;
        file->read(&spell, sizeof(unsigned char));
        m_spell = spell;
    }
}

VA(0x00484090, 0x6)
const char* TCampaignSpellBonus::getIconDefName() const
{
    return DATA_COMPGEN(0x00677248, spellBonusDefName, "SpellBon.def");
}

VA(0x004840a0, 0x25)
void TCampaignSpellBonus::apply(int whichPlayer) const
{
    hero* target = getCampaignBonusHero(m_hero, whichPlayer);
    if (target != 0)
        target->addSpell(m_spell);
}

// The shared hero picker. `best` and the player record are both live
// before the selector is tested, which is what puts them in the
// prologue; numHeroes is re-read from the record on every pass. Mac calls
// heroPower for candidate and then best at 0:0x91ecc/0:0x91ed8;
// Complete expands both copies of this ordinary helper.
VA(0x004840d0, 0x155)
hero* getCampaignBonusHero(int heroSelector, int whichPlayer)
{
    hero* best = 0;
    playerData* player = &g_game->m_players[whichPlayer];
    switch (heroSelector) {
    case CAMPAIGN_BONUS_HERO_STRONGEST: {
        for (int heroIndex = 0; heroIndex < player->m_numHeroes; ++heroIndex) {
            int heroId = player->m_heroes[heroIndex];
            hero* candidate = heroId == -1 ? 0 : &g_game->m_heroes[heroId];
            if (best != 0) {
                int candidatePower = heroPower(candidate);
                int bestPower = heroPower(best);
                if (bestPower >= candidatePower)
                    continue;
            }
            best = candidate;
        }
        return best;
    }
    case CAMPAIGN_BONUS_HERO_FIRST:
        if (player->m_numHeroes == 0)
            return 0;
        if (player->m_heroes[0] == -1)
            return 0;
        return &g_game->m_heroes[player->m_heroes[0]];
    case CAMPAIGN_BONUS_HERO_NONE:
        return 0;
    }
    hero* chosen = &g_game->m_heroes[heroSelector];
    return chosen->m_owner == whichPlayer ? chosen : 0;
}

// The two spell rows share the general-text pair 708/709: the campaign
// brief shows one for a learned spell and one for the scroll that
// carries it.
VA(0x00484230, 0x46)
std::string TCampaignSpellBonus::getText() const
{
    return formatString(g_generalText->getText(GENERAL_TEXT_CAMPAIGN_START_WITH_ITEM_FORMAT), g_spellTraits[m_spell].m_name);
}

VA(0x00484280, 0x46)
std::string TCampaignSpellScrollBonus::getText() const
{
    return formatString(g_generalText->getText(GENERAL_TEXT_CAMPAIGN_START_WITH_SCROLL_FORMAT), g_spellTraits[m_spell].m_name);
}

VA(0x004842d0, 0x3B)
void TCampaignSpellScrollBonus::apply(int whichPlayer) const
{
    hero* target = getCampaignBonusHero(m_hero, whichPlayer);
    if (target != 0) {
        // Complete reads this spell from an unsigned byte at 0x484050; the canonical scroll constructor takes DC SpellID.
        type_artifact scroll(static_cast<SpellID>(m_spell) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */);
        target->giveArtifact(&scroll, 0, 0);
    }
}

VA(0x00484310, 0x1D5)
void TCampaignCreatureBonus::apply(int whichPlayer) const
{
    playerData* player = &g_game->m_players[whichPlayer];
    if ((g_game->m_campaign.m_currentCampaign == g_creatureBonusTownCampaignA &&
         g_game->m_campaign.m_currentMap == g_creatureBonusTownScenarioA) ||
        (g_game->m_campaign.m_currentCampaign == g_creatureBonusTownCampaignB &&
         g_game->m_campaign.m_currentMap == g_creatureBonusTownScenarioB)) {
        int creature = m_creature;
        int faction;
        if (g_game->m_gameVersion == 0 &&
            isBaseElemental(creature))
            faction = -1;
        else
            faction = g_creatureTypeTraits[creature].m_townType;
        for (int townIndex = 0; townIndex < player->m_numTowns; ++townIndex) {
            town* garrison = g_game->getTown(player->m_townIds[townIndex]);
            if (garrison->m_type == faction) {
                const_cast<armyGroup&>(
                    static_cast<const town*>(garrison)->getArmy())
                    .add(creature, m_count, -1);
                return;
            }
        }
    }
    hero* target = getCampaignBonusHero(m_hero, whichPlayer);
    if (target == 0)
        return;
    if (target->m_army.add(m_creature, m_count, -1) != 0)
        return;
    int i;
    for (i = 0; i < player->m_numHeroes; ++i) {
        hero* other = g_game->getHero(player->m_heroes[i]);
        if (other->m_army.add(m_creature, m_count, -1) != 0)
            return;
    }
    for (i = 0; i < player->m_numTowns; ++i) {
        town* garrison = g_game->getTown(player->m_townIds[i]);
        if (const_cast<armyGroup&>(
                static_cast<const town*>(garrison)->getArmy())
                .add(m_creature, m_count, -1) != 0)
            return;
    }
}

VA(0x004844f0, 0x51)
void TCampaignCreatureBonus::read(TAbstractFile* file)
{
    {
        short value;
        file->read(&value, sizeof(short));
        m_hero = value;
        file->read(&value, sizeof(short));
        m_creature = value;
    }
    {
        unsigned short count;
        file->read(&count, sizeof(unsigned short));
        m_count = count;
    }
}

VA(0x00484550, 0x6)
const char* TCampaignCreatureBonus::getIconDefName() const
{
    return "twcrport.def";
}

VA(0x00484560, 0x7)
int TCampaignCreatureBonus::getIconIndex() const
{
    return m_creature + 2;
}

// Singular against plural on a count of exactly one, and an empty name
// for any creature outside the 0..150 table.
VA(0x00484570, 0x7A)
std::string TCampaignCreatureBonus::getText() const
{
    const char* name;
    if (m_creature < 0 || m_creature > 150)
        name = "";
    else if (m_count == 1)
        name = g_creatureTypeTraits[m_creature].m_name;
    else
        name = g_creatureTypeTraits[m_creature].m_pluralName;
    return formatString(g_generalText->getText(GENERAL_TEXT_CAMPAIGN_START_WITH_QUANTITY_FORMAT), m_count, name);
}

VA(0x004845f0, 0x24)
void TCampaignBuildingBonus::read(TAbstractFile* file)
{
    {
        unsigned char building;
        file->read(&building, sizeof(unsigned char));
        m_building = building;
    }
}

VA(0x00484620, 0x3)
bool TCampaignBuildingBonus::isBuildingBonus() const
{
    return true;
}

VA(0x00484630, 0x17)
const char* TCampaignBuildingBonus::getIconDefName() const
{
    return g_campaignBuildingIconNames[m_town][m_building];
}

VA(0x00484650, 0x146)
void TCampaignBuildingBonus::apply(int whichPlayer) const
{
    playerData* player = &g_game->m_players[whichPlayer];
    CMapHeaderData::TPlayerSlotAttributes* slot =
        &g_game->m_mapHeader.m_playerSlotAttributes[whichPlayer];
    if (player->m_numTowns == 0)
        return;
    if (g_game->m_campaign.m_currentCampaign == g_buildingBonusAllTownsCampaign &&
        g_game->m_campaign.m_currentMap == g_buildingBonusAllTownsScenario &&
        m_building == 0) {
        for (int townIndex = 0; townIndex < player->m_numTowns; ++townIndex) {
            town* each = g_game->getTown(player->m_townIds[townIndex]);
            each->buildBuilding(m_building, 0, 0);
        }
        return;
    }
    town* target = 0;
    if (slot->m_hasMainTown) {
        int townId = g_game->getTownId(slot->m_castleLoc.m_x, slot->m_castleLoc.m_y,
                                       slot->m_castleLoc.m_z);
        if (townId >= 0)
            target = g_game->getTown(townId);
    }
    if (target == 0)
        target = g_game->getTown(player->m_townIds[0]);
    if (m_building >= g_buildingBonusFirstUpgrade)
        target->buildBuilding(m_building - g_buildingBonusUpgradeStride, 0, 0);
    target->buildBuilding(m_building, 0, 0);
}

VA(0x004847a0, 0x3C)
std::string TCampaignBuildingBonus::getText() const
{
    const char* format = g_generalText->getText(GENERAL_TEXT_CAMPAIGN_START_WITH_ITEM_FORMAT);
    return formatString(format, getBuildingName(m_town, m_building));
}

VA(0x004847e0, 0x22)
void TCampaignBuildingBonus::setTown(int town)
{
    m_town = town;
    m_building = g_eventBuildingIds[town][m_building];
}

VA(0x00484810, 0x6)
const char* TCampaignArtifactBonus::getIconDefName() const
{
    return DATA_COMPGEN(0x00677258, artifactBonusDefName, "ArtifBon.def");
}

VA(0x00484820, 0x40)
std::string TCampaignArtifactBonus::getText() const
{
    return formatString(g_generalText->getText(GENERAL_TEXT_CAMPAIGN_START_WITH_ITEM_FORMAT),
                         g_artifactTraits[m_artifact].m_name);
}

VA(0x00484860, 0x3B)
void TCampaignArtifactBonus::apply(int whichPlayer) const
{
    hero* target = getCampaignBonusHero(m_hero, whichPlayer);
    if (target != 0) {
        // Complete reads this bonus as a signed word at 0x4848a0; the canonical artifact constructor takes DC TArtifact.
        type_artifact granted(static_cast<TArtifact>(m_artifact) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */);
        target->giveArtifact(&granted, 0, 0);
    }
}

VA(0x004848a0, 0x38)
void TCampaignArtifactBonus::read(TAbstractFile* file)
{
    short value;
    file->read(&value, sizeof(short));
    m_hero = value;
    file->read(&value, sizeof(short));
    m_artifact = value;
}

VA(0x004848e0, 0x6)
const char* TCampaignPrimarySkillBonus::getIconDefName() const
{
    return DATA_COMPGEN(0x00677268, primarySkillBonusDefName, "PSkilBon.def");
}

// The frame is the strongest of the four deltas, and ties keep the FIRST:
// the compare is `>` against a running best that starts at zero, so an
// all-negative row still answers 0.
VA(0x004848f0, 0x1E)
int TCampaignPrimarySkillBonus::getIconIndex() const
{
    int best = 0;
    int bestValue = 0;
    for (int skill = 0; skill < 4; ++skill) {
        if (m_skills[skill] > bestValue) {
            bestValue = m_skills[skill];
            best = skill;
        }
    }
    return best;
}

VA(0x00484910, 0x275)
std::string TCampaignPrimarySkillBonus::getText() const
{
    std::string list;
    int remaining = 0;
    int stat;
    for (stat = 0; stat < 4; ++stat)
        if (m_skills[stat] > 0)
            ++remaining;
    for (stat = 0; stat < 4; ++stat) {
        if (m_skills[stat] > 0) {
            list += formatString(
                DATA_COMPGEN(0x00677278, primarySkillBonusFormat, "+%d %s"),
                m_skills[stat], g_statNames[stat]);
            --remaining;
            if (remaining == 1)
                list += g_generalText->getText(GENERAL_TEXT_LIST_AND);
            else if (remaining > 0)
                list += ", ";
        }
    }
    list = formatString(g_generalText->getText(GENERAL_TEXT_CAMPAIGN_START_WITH_ITEM_FORMAT), list.c_str());
    return list;
}

// Each delta is added to the CLAMPED current value: anything above 99
// saturates there, a positive value is taken as it stands, and a
// non-positive one falls back to the stat's own floor - zero for attack
// and defence, one for power and knowledge.
VA(0x00484b90, 0x5D)  // anchor-vtable (0x63da00+0x14), retail-only
void TCampaignPrimarySkillBonus::apply(int whichPlayer) const
{
    hero* target = getCampaignBonusHero(m_hero, whichPlayer);
    if (target != 0) {
        for (int stat = 0; stat < 4; ++stat) {
            int current = target->getPrimarySkill(stat);
            target->setPrimarySkill(stat, current + m_skills[stat]);
        }
    }
}

VA(0x00484bf0, 0x31)
void TCampaignPrimarySkillBonus::read(TAbstractFile* file)
{
    short heroId;
    file->read(&heroId, sizeof(short));
    m_hero = heroId;
    file->read(m_skills, sizeof(m_skills));
}

VA(0x00484c30, 0x6)
const char* TCampaignSecondarySkillBonus::getIconDefName() const
{
    return DATA_COMPGEN(0x00677280, secondarySkillBonusDefName, "SSkilBon.def");
}

VA(0x00484c40, 0xE)
int TCampaignSecondarySkillBonus::getIconIndex() const
{
    return m_skill * 3 + m_level - 1;
}

VA(0x00484c50, 0x4B)
std::string TCampaignSecondarySkillBonus::getText() const
{
    return formatString(g_generalText->getText(GENERAL_TEXT_CAMPAIGN_START_WITH_SKILL_FORMAT),
                         g_secondarySkillLevels[m_level - 1],
                         g_sSkillTraits[m_skill].m_name);
}

// A skill the hero does not have yet goes through GiveSS, which also
// takes the free slot; an already-known one is written in place. Either
// way a hero who is already better keeps what he has.
VA(0x00484ca0, 0x4F)
void TCampaignSecondarySkillBonus::apply(int whichPlayer) const
{
    hero* target = getCampaignBonusHero(m_hero, whichPlayer);
    if (target != 0 && target->m_skillLevel[m_skill] <= m_level) {
        if (target->m_skillLevel[m_skill] == 0)
            target->giveSS(m_skill, m_level);
        else
            target->m_skillLevel[m_skill] = m_level;
    }
}

VA(0x00484cf0, 0x56)
void TCampaignSecondarySkillBonus::read(TAbstractFile* file)
{
    {
        short heroId;
        file->read(&heroId, sizeof(short));
        m_hero = heroId;
    }
    {
        unsigned char value;
        file->read(&value, sizeof(unsigned char));
        m_skill = value;
        file->read(&value, sizeof(unsigned char));
        m_level = value;
    }
}

VA(0x00484d50, 0x3)
bool TCampaignBonus::isBuildingBonus() const
{
    return false;
}

VA(0x00484d60, 0x6)
const char* TCampaignResourceBonus::getIconDefName() const
{
    return DATA_COMPGEN(0x00677290, resourceBonusDefName, "BoRes.def");
}

VA(0x00484d70, 0x15)
int TCampaignResourceBonus::getIconIndex() const
{
    if (m_resource < 0)
        return (m_resource != -3) + 7;
    return m_resource;
}

// The mixed rows take their own general-text lines; the seven plain ones
// take the shared resource-name table, and anything else leaves the name
// null for format_string to print as an empty %s.
VA(0x00484d90, 0x8E)
std::string TCampaignResourceBonus::getText() const
{
    const char* name = 0;
    switch (m_resource) {
    case WOOD:
    case MERCURY:
    case ORE:
    case SULFUR:
    case CRYSTAL:
    case GEMS:
    case GOLD:
        name = g_resourceNames[m_resource];
        break;
    case CAMPAIGN_BONUS_RESOURCE_WOOD_AND_ORE:
        name = g_generalText->getText(GENERAL_TEXT_CAMPAIGN_WOOD_AND_ORE);
        break;
    case CAMPAIGN_BONUS_RESOURCE_RARE:
        name = g_generalText->getText(GENERAL_TEXT_CAMPAIGN_RARE_RESOURCES);
        break;
    case CAMPAIGN_BONUS_RESOURCE_NONE:
        break;
    }
    return formatString(g_generalText->getText(GENERAL_TEXT_CAMPAIGN_START_WITH_QUANTITY_FORMAT), m_amount, name);
}

VA(0x00484e20, 0xDE)
void TCampaignResourceBonus::apply(int whichPlayer) const
{
    playerData* player = &g_game->m_players[whichPlayer];
    switch (m_resource) {
    case WOOD:
    case MERCURY:
    case ORE:
    case SULFUR:
    case CRYSTAL:
    case GEMS:
    case GOLD:
        player->m_resources[m_resource] += m_amount;
        break;
    case CAMPAIGN_BONUS_RESOURCE_WOOD_AND_ORE:
        player->m_resources[WOOD] += m_amount;
        player->m_resources[ORE] += m_amount;
        break;
    case CAMPAIGN_BONUS_RESOURCE_RARE:
        player->m_resources[MERCURY] += m_amount;
        player->m_resources[SULFUR] += m_amount;
        player->m_resources[CRYSTAL] += m_amount;
        player->m_resources[GEMS] += m_amount;
        break;
    case CAMPAIGN_BONUS_RESOURCE_NONE:
        break;
    }
}

VA(0x00484f00, 0x37)
void TCampaignResourceBonus::read(TAbstractFile* file)
{
    {
        char resource;
        file->read(&resource, sizeof(char));
        m_resource = resource;
    }
    {
        int amount;
        file->read(&amount, sizeof(int));
        m_amount = amount;
    }
}

// --- the scenario's starting-options chooser (see customcampaign.h) ---

// The root's destructor, out of line. Nothing calls it - the `??_G` below
// and both sibling destructors expand the single vptr store - but a plain
// non-COMDAT body is emitted and kept regardless.
VA(0x00484f40, 0x7)
TCampaignStartOption::~TCampaignStartOption()
{
}

// The root's scalar deleting destructor: the base teardown is one vptr
// store, so nothing is called.
VA_COMPGEN(0x00484f50, 0x23, SCALAR_DELETING_DTOR, TCampaignStartOption)

// Slot 5, inherited by the bonus list and the starting-hero option (the
// crossover option overrides it at 0x4859b0). The answer is the campaign's
// running crossover slot as soon as the scenario carries anything to carry
// over - a crossover artifact, a hero placeholder, or a positive status for
// this option's own player - and -1 otherwise.
VA(0x00484f80, 0x7F)
int TCampaignStartOption::slot5(void* scenarioRecord, int which) const
{
    TCampaignBrief::ScenarioStruct* scenario =
        static_cast<TCampaignBrief::ScenarioStruct*>(scenarioRecord);
    int player = getPlayer(which);
    int result = -1;
    if (scenario->m_crossoverArtifacts.count() > 0
        || scenario->m_heroPlaceholders.size() > 0
        || scenario->m_heroesStatus[player] > 0)
        result = g_game->m_campaign.m_crossoverArrayIndex;
    return result;
}

// Slot 12, inherited by the bonus list and the starting-hero option (the
// crossover option overrides it at 0x4859e0): every scenario this one lists
// as a prerequisite must already be completed, and then one of the option's
// own choices must answer slot 5 with the value asked about. An option with
// no choices at all is asked with -1.
VA(0x00485000, 0x8B)
bool TCampaignStartOption::slot12(void* scenarioRecord, int value) const
{
    TCampaignBrief::ScenarioStruct* scenario =
        static_cast<TCampaignBrief::ScenarioStruct*>(scenarioRecord);
    if (!scenario->prerequisitesMet())
        return false;

    int count = getCount();
    if (count == 0) {
        if (slot5(scenario, -1) == value)
            return true;
    } else {
        for (int choice = 0; choice < count; ++choice)
            if (slot5(scenario, choice) == value)
                return true;
    }
    return false;
}

// Slot 7, inherited unchanged by all three concrete options.
VA(0x00485090, 0x6)
int TCampaignStartOption::slot7(int which) const
{
    return -1;
}

// The bonus list's own destructor: every element is deleted through
// TCampaignBonus's virtual destructor, then the vector's own teardown.
VA(0x004850a0, 0x8E)
TCampaignStartBonusOption::~TCampaignStartBonusOption()
{
    for (unsigned int i = 0; i < m_bonuses.size(); ++i)
        delete m_bonuses[i];
}

VA_COMPGEN(0x00485130, 0x21, SCALAR_DELETING_DTOR, TCampaignStartBonusOption)

VA(0x00485160, 0x13)
int TCampaignStartBonusOption::getCount() const
{
    return m_bonuses.size();
}

VA(0x00485180, 0x6)
int TCampaignStartBonusOption::getPlayer(int which) const
{
    return m_player;
}

VA(0x00485190, 0x1B0)
void TCampaignStartBonusOption::read(TAbstractFile* file)
{
    int count;
    {
        unsigned char value;
        file->read(&value, sizeof(unsigned char));
        m_player = value;
        file->read(&value, sizeof(unsigned char));
        count = value;
    }
    while (count--) {
        unsigned char type;
        file->read(&type, sizeof(unsigned char));
        TCampaignBonus* bonus;
        switch (type) {
        case CAMPAIGN_BONUS_SPELL:
            bonus = new TCampaignSpellBonus;
            break;
        case CAMPAIGN_BONUS_CREATURE:
            bonus = new TCampaignCreatureBonus;
            break;
        case CAMPAIGN_BONUS_BUILDING:
            bonus = new TCampaignBuildingBonus;
            break;
        case CAMPAIGN_BONUS_ARTIFACT:
            bonus = new TCampaignArtifactBonus;
            break;
        case CAMPAIGN_BONUS_SPELL_SCROLL:
            bonus = new TCampaignSpellScrollBonus;
            break;
        case CAMPAIGN_BONUS_PRIMARY_SKILL:
            bonus = new TCampaignPrimarySkillBonus;
            break;
        case CAMPAIGN_BONUS_SECONDARY_SKILL:
            bonus = new TCampaignSecondarySkillBonus;
            break;
        case CAMPAIGN_BONUS_RESOURCE:
            bonus = new TCampaignResourceBonus;
            break;
        }
        bonus->read(file);
        m_bonuses.push_back(bonus);
    }
}

// The scalar deleting destructor the SEVEN derived bonus classes share:
// each one's teardown is the base's, so /OPT:ICF folds all seven onto this
// address, which every derived vftable's slot 0 points at.
VA_COMPGEN(0x00485340, 0x21, SCALAR_DELETING_DTOR, TCampaignSpellBonus)

VA(0x00485370, 0x7)
TCampaignBonus::~TCampaignBonus()
{
}

VA(0x00485380, 0x32)
void TCampaignStartBonusOption::apply(void* scenario)
{
    std::vector<TCampaignBonus*>& bonuses = m_bonuses;
    unsigned int chosen = g_game->m_campaign.m_briefingChoice;
    if (chosen < bonuses.size())
        bonuses[chosen]->apply(m_player);
}

// The town every building bonus is bound to is the map header's own main
// town type for this option's player.
VA(0x004853c0, 0x46)
void TCampaignStartBonusOption::setTown(CMapHeaderData* header)
{
    for (unsigned int i = 0; i < m_bonuses.size(); ++i)
        m_bonuses[i]->setTown(header->m_playerSlotAttributes[m_player].m_mainTownType);
}

VA(0x00485410, 0x15)
int TCampaignStartBonusOption::getIconIndex(int which) const
{
    return m_bonuses[which]->getIconIndex();
}

VA(0x00485430, 0x15)
bool TCampaignStartBonusOption::isBuildingBonus(int which) const
{
    return m_bonuses[which]->isBuildingBonus();
}

VA(0x00485450, 0x15)
const char* TCampaignStartBonusOption::getIconDefName(void* scenario,
                                                     int which) const
{
    return m_bonuses[which]->getIconDefName();
}

VA(0x00485470, 0x27)
std::string TCampaignStartBonusOption::getText(void* scenario,
                                               int which) const
{
    return m_bonuses[which]->getText();
}

// --- the crossover-hero starting option (vftable 0x63dad8) ---

VA(0x004854a0, 0x12)
int TCampaignStartCrossoverOption::getCount() const
{
    return m_choices.size();
}

VA(0x004854c0, 0x6E)
const char* TCampaignStartCrossoverOption::getIconDefName(void* campaignRecord,
                                                          int which) const
{
    SCampaign* campaign = static_cast<SCampaign*>(campaignRecord);
    std::vector<hero>& pool = campaign->m_carryOverHeroes
        [campaign->m_mapScores[m_choices[which].m_scenario].m_index];
    hero* first = pool.size() != 0 ? &pool[0] : 0;
    if (first == 0)
        return "hpl000kn.pcx";
    return g_heroTraits[first->m_portrait].m_largePortraitName;
}

// The help text names the MAP the heroes come from, which is not the choice's
// own scenario but the last completed scenario sharing its crossover slot;
// the map name itself is only available after re-opening the campaign file
// and inflating that scenario's header.
// Mac +0x93974 calls the retained campaign score scan at +0x98b7c, and
// +0x93a58 calls CampaignHeaderStruct::loadScenario (+0x96c64), ignoring
// its result. Complete expands both; the retained Windows loadScenario
// body (0x488810) is exact and its expansion calls loadMapHeader.
// With both canonical calls, VC6 currently gives 64.62% and 14 blocks
// against retail's 19: constructor and nested inflater calls diverge.
// The old flattened source gave 86.04%, but omitted both Mac helpers.
VA(0x00485530, 0x260)  // anchor-callee(CampaignHeaderStruct::Load 0x488880), retail-only
std::string TCampaignStartCrossoverOption::getText(void* campaignRecord,
                                                   int which) const
{
    TCampaignBrief::CampaignHeaderStruct* campaign =
        static_cast<TCampaignBrief::CampaignHeaderStruct*>(campaignRecord);
    int slot = g_game->m_campaign.m_mapScores[m_choices[which].m_scenario].m_index;
    int source = g_game->m_campaign.findLatestCrossoverScenario(slot);

    NewSMapHeader mapHeader;
    campaign->loadScenario(source, &mapHeader);
    return formatString(g_generalText->getText(GENERAL_TEXT_CAMPAIGN_START_WITH_MAP_HEROES_FORMAT), mapHeader.m_mapName.c_str());
}

// The player position the pool is handed to. Slot 12 asks with -1 when the
// option carries no choices at all, which reads the first slot instead.
VA(0x00485790, 0x17)
int TCampaignStartCrossoverOption::getPlayer(int which) const
{
    if (which < 0)
        which = 0;
    return m_choices[which].m_player;
}

VA(0x004857b0, 0x1F4)
void TCampaignStartCrossoverOption::read(TAbstractFile* file)
{
    int count;
    {
        unsigned char value;
        file->read(&value, sizeof(unsigned char));
        count = value;
    }
    while (count--) {
        TCampaignCrossoverChoice choice;
        {
            signed char player;
            file->read(&player, sizeof(signed char));
            choice.m_player = player;
        }
        {
            signed char scenario;
            file->read(&scenario, sizeof(signed char));
            choice.m_scenario = scenario;
        }
        m_choices.push_back(choice);
    }
}

VA(0x004859b0, 0x24)
int TCampaignStartCrossoverOption::slot5(void* scenario, int which) const
{
    return g_game->m_campaign.m_mapScores[m_choices[which].m_scenario].m_index;
}

VA(0x004859e0, 0x44)
bool TCampaignStartCrossoverOption::slot12(void* scenario, int value) const
{
    for (unsigned int choice = 0; choice < m_choices.size(); ++choice)
        if (slot5(scenario, choice) == value)
            return true;
    return false;
}

// Slot 1 for BOTH crossover options - `mov al,1; ret 4`, and /OPT:ICF folds
// the two identical bodies onto this one address, so only this copy carries
// the claim (the starting-hero twin below is defined and left unclaimed).
VA(0x00485a30, 0x5)
bool TCampaignStartCrossoverOption::isBuildingBonus(int which) const
{
    return true;
}

// --- the starting-hero option (vftable 0x63db0c) ---

bool TCampaignStartHeroOption::isBuildingBonus(int which) const
{
    return true;
}

VA(0x00485a40, 0x13)
int TCampaignStartHeroOption::getCount() const
{
    return m_choices.size();
}

VA(0x00485a60, 0x30)
const char* TCampaignStartHeroOption::getIconDefName(void* campaign,
                                                     int which) const
{
    if (m_choices[which].m_hero == -1)
        return "CBONN1A3.pcx";
    return g_heroTraits[m_choices[which].m_hero].m_largePortraitName;
}

VA(0x00485a90, 0xBA)
std::string TCampaignStartHeroOption::getText(void* campaign, int which) const
{
    if (m_choices[which].m_hero == -1)
        return g_generalText->getText(GENERAL_TEXT_CAMPAIGN_START_WITH_RANDOM_HERO);
    return formatString(g_generalText->getText(GENERAL_TEXT_CAMPAIGN_START_WITH_ITEM_FORMAT),
                         g_heroTraits[m_choices[which].m_hero].m_defaultName);
}

VA(0x00485b50, 0x10)
int TCampaignStartHeroOption::getPlayer(int which) const
{
    return m_choices[which].m_player;
}

// 2026-09-06, polish lane 38: the read loop is `while (count--)`, not
// `for (i = 0; i != count; ++i)` - retail's `mov X,count / dec count /
// test X,X / je` tests the PRE-decrement value, and the index is never used
// in the body, which is the source-side tell for the whole family. That took
// the two sibling readers below to EXACT and this one 92.6089 -> 93.9951.
// Spelling the reset as the canonical vector::clear operation then restores
// retail's EBX file and ESI receiver bindings and matches all 26 blocks
// exactly. The erase(begin, end) spelling had the same behavior, branches and
// call multiset but assigned the callee-saved roles differently (93.9951%).
VA(0x00485b60, 0x1FB)  // anchor-vtable (0x63db0c+0x24), retail-only
void TCampaignStartHeroOption::read(TAbstractFile* file)
{
    int count;
    {
        signed char value;
        file->read(&value, sizeof(signed char));
        count = value;
    }
    m_choices.clear();
    while (count--) {
        TCampaignHeroChoice choice;
        {
            signed char player;
            file->read(&player, sizeof(signed char));
            choice.m_player = player;
        }
        {
            short heroId;
            file->read(&heroId, sizeof(short));
            choice.m_hero = heroId;
        }
        m_choices.push_back(choice);
    }
}

VA(0x00485d60, 0x11)
int TCampaignStartHeroOption::slot7(int which) const
{
    return m_choices[which].m_hero;
}

VA(0x00485d80, 0x3)
void TCampaignBonus::setTown(int)
{
}

// Retail-only length-prefixed string reader, declared in hero.h and owned
// by this TU. /Gr passes the hidden return object in ECX and infile in EDX.
// Retail freezes the resized string buffer, then reads/copies 512-byte chunks.

// Residual (95.1988%): end the scalar-read temporary's scope before string
// construction, carrying its value into resize and the remaining-byte loop.
// That restores the string at ebp-0x1c, the EBX result and ESI count, raising
// 81.6506 -> 95.1988. Changing resize(length) alone is byte-flat at 81.6506;
// the temporary lifetime is essential. Moving the chunk buffer outside the
// loop, separate zero assignment, memset/value initialization, signed/long
// read-word types, and ordinary/template value-return readers are all byte-flat
// on the scoped form. No speculative helper is retained.
// Retail still has one immediate zero store to the read word at ebp-0x34;
// VC6 emits two zero-register stores at ebp-0x24. The allocator temporary,
// saved infile/result and destination slots differ; the string/resize/copy
// code and call decisions otherwise align. The passive inline trace shows
// default construction, resize/append, begin/Freeze, copy and destruction
// expanding with the retained Grow/Eos/assign calls. No Dreamcast counterpart.
VA(0x00485d90, 0x1BB)  // anchor-caller(ScenarioStruct::Read +0x2b), retail-only
std::string readLengthPrefixedString(TAbstractFile* infile)
{
    unsigned int remaining;
    {
        unsigned int length = 0;
        infile->read(&length, sizeof(unsigned int));
        remaining = length;
    }
    std::string text;
    text.resize(remaining);
    std::string::iterator dest = text.begin();
    while (remaining > 0) {
        char chunk[512];
        unsigned int count = remaining;
        if (count >= sizeof(chunk))
            count = sizeof(chunk);
        infile->read(chunk, count);
        std::copy(chunk, chunk + count, dest);
        dest += count;
        remaining -= count;
    }
    return text;
}

VA(0x00485f50, 0x8B)
TCampaignBrief::ScenarioStruct::ScenarioStruct()
{
    m_prologue = 0;
    m_epilogue = 0;
    m_options = 0;
    memset(m_heroesStatus, 0, sizeof(m_heroesStatus));
}

VA(0x00485fe0, 0x12C)
TCampaignBrief::ScenarioStruct::~ScenarioStruct()
{
    delete m_prologue;
    delete m_epilogue;
    delete m_options;
}

// Complete-only. DoPreLoadCustomization's per-hero half: the map's own
// setup record for a carry-over hero is copied onto a freshly allocated
// hero of the same class (the whole HeroExtra assignment is compiler-
// generated and expanded here), the original slot is retired by driving its
// placement x to -1, and the availability table follows - the new id takes
// the record's owner, the old one goes back to the tavern pool. Name
// provisional.
// 2026-09-07: 98.4901 -> 99.9509 by reading setup.m_owner directly in the
// GetNewHeroId call; the prior named int owner rotated EAX/ECX/EDX throughout
// the copy. heroClass remains a named THeroClass because that fixes its
// evaluation before the direct owner read. All 38 blocks, 20 branches and 5
// calls agree. The only remaining code delta exchanges the compiler copy's
// two stack homes: retail puts setup at [ebp-0x14] and the source/destination
// delta at [ebp-0x10], while this compile does the reverse. Rejected controls:
// pointer versus reference and function-scope heroClass/newHeroId are
// byte-flat; binding setup after the guard falls to 97.55; signed-char owner
// scored 94.62, unsigned-char owner 97.41, explicit operator= and naming the
// destination before the copy 98.46. Do not manufacture a spill carrier.
// The major recovery still depends on game.h's HeroExtra pads not being
// members: retail's generated assignment skips every one of them.
VA(0x00486110, 0x32F)  // anchor-caller(DoPreLoadCustomization +0x117), retail-only
void game::rehomeCampaignHeroSetup(int heroId)
{
    HeroExtra& setup = m_heroSetup[heroId];
    if (setup.m_location.m_x < 0)
        return;

    THeroClass heroClass = g_heroTraits[heroId].m_heroClass;
    int newHeroId = getNewHeroId(setup.m_owner, kNumHeroClasses, 1, heroClass);
    if (newHeroId == -1) {
        setup.m_location.m_x = -1;
        return;
    }

    m_heroSetup[newHeroId] = setup;
    HeroExtra& newSetup = m_heroSetup[newHeroId];
    newSetup.m_id = newHeroId;
    setup.m_location.m_x = -1;
    m_heroAvailability[newHeroId] = setup.m_owner;
    m_heroAvailability[heroId] = hero::HERO_AVAILABILITY_TAVERN_POOL;
    if (newSetup.m_portraitNumber == heroId)
        newSetup.m_portraitNumber = newHeroId;
}

VA(0x00486440, 0x145)  // dc 0x7d22c
void SCampaign::doPreLoadCustomization()
{
    unsigned int poolIndex;
    for (poolIndex = m_carryOverHeroes.size(); poolIndex--;) {
        std::vector<hero>& pool = m_carryOverHeroes[poolIndex];
        for (unsigned int heroIndex = pool.size(); heroIndex--;)
            g_game->m_heroAvailability[pool[heroIndex].m_id] =
                hero::HERO_AVAILABILITY_TAVERN_POOL;
    }

    for (poolIndex = m_carryOverHeroes.size(); poolIndex--;) {
        std::vector<hero>& pool = m_carryOverHeroes[poolIndex];
        for (unsigned int heroIndex = pool.size(); heroIndex--;)
            g_game->rehomeCampaignHeroSetup(pool[heroIndex].m_id);
    }
}

// Complete-only helper hypothesis, name provisional. At 0x486590 the
// retained-spellbook arm zeroes hero's two 70-byte tables before AddSpell
// rebuilds them from the source hero. The DC method roster predates this
// campaign path, so it supplies neither this name nor an inline keyword.
// Keep the ordinary body visible before its caller for VC6 auto-inlining.
void hero::clearSpells()
{
    memset(m_inSpellbook, 0, sizeof(m_inSpellbook));
    memset(m_availableSpells, 0, sizeof(m_availableSpells));
}

// Complete-only campaign carry-over expansion. Dreamcast's campaign path has
// no counterpart, but its debug types still corroborate hero, army and
// artifact source boundaries. Retail independently proves the ScenarioStruct
// receiver (+0x44..+0xa4), HeroPlaceholderData argument, and source hero. Its
// code also proves the pointer-end artifact fill, custom-name flag order,
// guarded west-adjacent TOWN test, and the final hero-id value lifetime.
// The custom-name assignment still lacks retail's out-of-line _Eos;
// spelling it as string::assign is byte-flat at 94.5908%.
VA(0x00486590, 0xA84)  // two calls from ScenarioStruct's 0x487290 map setup
void TCampaignBrief::ScenarioStruct::initializeCrossoverHero(
    HeroPlaceholderData* placeholder, hero* sourceHero)
{
    CObject* object = placeholder->m_object;
    hero* currentHero = g_game->getHero(sourceHero->m_id);
    currentHero->m_order = 0;
    currentHero->m_id = sourceHero->m_id;
    SCampaign* currentCampaign = &g_game->m_campaign;
    int slot;

    if (currentCampaign->m_currentCampaign == g_crossoverSplitCampaign
        && (currentCampaign->m_currentMap == g_crossoverSplitFirstScenario
            || currentCampaign->m_currentMap
                == g_crossoverSplitLastScenario)
        && (currentCampaign->m_mapScores[0].m_completed
            || currentCampaign->m_mapScores[1].m_completed)) {
        m_crossoverCreatures.reset();
    }

    if (m_retainXp) {
        currentHero->m_experience = sourceHero->m_experience;
        currentHero->m_level = sourceHero->m_level;
    }

    if (m_retainPskills) {
        type_artifact savedArtifacts[g_crossoverPrimaryArtifactSlots];
        std::fill(savedArtifacts,
                  savedArtifacts + g_crossoverPrimaryArtifactSlots,
                  type_artifact());

        for (slot = 0; slot < g_crossoverPrimaryArtifactSlots; ++slot) {
            type_artifact artifact = currentHero->getArtifact(TArtifactSlot(slot));
            if (artifact.m_artifactId != ARTIFACT_NONE) {
                savedArtifacts[slot] = artifact;
                currentHero->removeArtifact(slot);
            }
        }

        for (slot = 0; slot < g_crossoverPrimaryArtifactSlots; ++slot) {
            type_artifact artifact = sourceHero->getArtifact(TArtifactSlot(slot));
            if (artifact.m_artifactId != ARTIFACT_NONE)
                currentHero->equipArtifact(&artifact, slot);
        }

        for (int skill = 0; skill < g_crossoverPrimarySkills; ++skill) {
            currentHero->setPrimarySkill(
                skill, sourceHero->getPrimarySkill(skill));
        }

        for (slot = 0; slot < g_crossoverPrimaryArtifactSlots; ++slot) {
            type_artifact artifact = currentHero->getArtifact(TArtifactSlot(slot));
            if (artifact.m_artifactId != ARTIFACT_NONE)
                currentHero->removeArtifact(slot);
        }

        for (slot = 0; slot < g_crossoverPrimaryArtifactSlots; ++slot) {
            if (savedArtifacts[slot].m_artifactId != ARTIFACT_NONE)
                currentHero->equipArtifact(&savedArtifacts[slot], slot);
        }
    }

    if (m_retainSskills) {
        currentHero->m_skillCount = sourceHero->m_skillCount;
        memcpy(currentHero->m_skillLevel, sourceHero->m_skillLevel,
               sizeof(currentHero->m_skillLevel));
        memcpy(currentHero->m_skillOrder, sourceHero->m_skillOrder,
               sizeof(currentHero->m_skillOrder));
    }

    int triggerX;
    int triggerY;
    object->findTrigger(triggerX, triggerY);
    currentHero->m_x = static_cast<short>(triggerX);
    currentHero->m_y = static_cast<short>(triggerY);
    currentHero->m_z = object->m_z;
    currentHero->m_owner = static_cast<signed char>(placeholder->m_owner);
    currentHero->m_heroClass = sourceHero->m_heroClass;
    currentHero->m_patrolRadius = -1;
    currentHero->m_patrolX = hero::kPatrolNone;
    strcpy(currentHero->m_name, sourceHero->m_name);
    currentHero->m_portrait = sourceHero->m_portrait;

    if (g_inCampaign) {
        if (currentCampaign->m_currentCampaign == g_crossoverBonusCampaign
            && currentCampaign->m_currentMap == g_crossoverBonusScenario
            && sourceHero->m_id == g_crossoverBonusHero) {
            currentHero->m_portrait = g_crossoverBonusPortrait;
            currentHero->setPrimarySkill(
                0, currentHero->getPrimarySkill(0)
                    + g_crossoverBonusAmount);
            currentHero->setPrimarySkill(
                1, currentHero->getPrimarySkill(1)
                    + g_crossoverBonusAmount);
        }
        if (currentCampaign->m_currentCampaign == g_crossoverPatrolCampaign
            && (currentCampaign->m_currentMap
                    == g_crossoverPatrolFirstScenario
                || currentCampaign->m_currentMap
                    == g_crossoverPatrolLastScenario)
            && sourceHero->m_id == g_crossoverPatrolHero) {
            currentHero->m_patrolRadius = g_crossoverPatrolRadius;
            currentHero->m_patrolX = static_cast<unsigned char>(triggerX);
            currentHero->m_patrolY = static_cast<unsigned char>(triggerY);
        }
    }

    currentHero->m_army.initialize();
    for (slot = 0; slot < armyGroup::ARMY_GROUP_SLOT_COUNT; ++slot) {
        if (sourceHero->m_army.m_armies[slot] != -1
            && m_crossoverCreatures.test(sourceHero->m_army.m_armies[slot])) {
            currentHero->m_army.add(sourceHero->m_army.m_armies[slot],
                                  sourceHero->m_army.m_numTroops[slot], -1);
        }
    }
    if (currentHero->m_army.getCreatureTotal() == 0)
        g_game->setRandomHeroArmies(currentHero->m_id, 0, 0);

    if (m_retainSpellbook) {
        currentHero->clearSpells();
        for (int spell = 0; spell < hero::NUM_SPELLS; ++spell) {
            if (sourceHero->isInSpellbook(spell))
                currentHero->addSpell(spell);
        }
        if (!m_retainArtifacts
            && sourceHero->getArtifact(TArtifactSlot(hero::EQUIPPED_SLOT_SPELLBOOK)).m_artifactId
                == ARTIFACT_SPELLBOOK
            && currentHero->getArtifact(TArtifactSlot(hero::EQUIPPED_SLOT_SPELLBOOK)).m_artifactId
                == ARTIFACT_NONE) {
            type_artifact spellbook(ARTIFACT_SPELLBOOK);
            currentHero->equipArtifact(&spellbook, -1);
        }
    }

    if (m_retainArtifacts) {
        for (slot = 0; slot < g_crossoverEquippedArtifactSlots; ++slot) {
            type_artifact artifact = currentHero->getArtifact(TArtifactSlot(slot));
            if (artifact.m_artifactId != ARTIFACT_NONE)
                currentHero->removeArtifact(slot);
        }
        for (slot = 0; slot < g_crossoverEquippedArtifactSlots; ++slot) {
            type_artifact artifact = sourceHero->getArtifact(TArtifactSlot(slot));
            if (artifact.m_artifactId != ARTIFACT_NONE)
                currentHero->equipArtifact(&artifact, slot);
        }
        while (currentHero->getNumberInBackpack(1) > 0) {
            currentHero->removeBackpackArtifact(
                static_cast<short>(currentHero->getLastBackpackIndex()));
        }
        for (slot = 0; slot < HERO_BACKPACK_CAPACITY; ++slot) {
            if (sourceHero->getBackpack(slot).m_artifactId != ARTIFACT_NONE)
                currentHero->addToBackpack(&sourceHero->getBackpack(slot), -1);
        }
    } else {
        for (slot = 0; slot < g_crossoverEquippedArtifactSlots; ++slot) {
            type_artifact artifact = sourceHero->getArtifact(TArtifactSlot(slot));
            if (artifact.m_artifactId != ARTIFACT_NONE
                && m_crossoverArtifacts.test(artifact.m_artifactId)) {
                type_artifact displaced = currentHero->getArtifact(TArtifactSlot(slot));
                if (displaced.m_artifactId != ARTIFACT_NONE)
                    currentHero->removeArtifact(slot);
                currentHero->equipArtifact(&artifact, slot);
            }
        }
        for (slot = 0; slot < HERO_BACKPACK_CAPACITY; ++slot) {
            type_artifact artifact = sourceHero->getBackpack(slot);
            if (artifact.m_artifactId != ARTIFACT_NONE
                && m_crossoverArtifacts.test(artifact.m_artifactId)) {
                currentHero->addToBackpack(&artifact, -1);
            }
        }
    }

    currentHero->m_sex = sourceHero->m_sex;
    if (sourceHero->m_hasCustomName) {
        currentHero->m_hasCustomName = 1;
        currentHero->m_customName = sourceHero->heroFn004D8FB0();
    }

    currentHero->m_mana = static_cast<short>(currentHero->getMaxMana());
    currentHero->m_maxMovePoints = currentHero->m_movePoints
        = currentHero->getMobility();

    type_point heroLocation = currentHero->getLocation();
    --heroLocation.m_x;
    if (heroLocation.m_x >= 0) {
        NewmapCell* cell = g_game->m_worldMap.cell(heroLocation);
        if (cell->m_type == TOWN && cell->m_isTrigger)
            --currentHero->m_x;
    }

    g_game->m_players[currentHero->m_owner].m_heroes[
        g_game->m_players[currentHero->m_owner].m_numHeroes] = currentHero->m_id;
    ++g_game->m_players[currentHero->m_owner].m_numHeroes;
    currentHero->obscureCell();
    g_game->m_heroAvailability[currentHero->m_id] = currentHero->m_owner;
    g_game->m_heroPoolMap[currentHero->m_id].set(currentHero->m_owner);
    g_game->setVisibility(currentHero->m_x, currentHero->m_y, currentHero->m_z,
                          currentHero->m_owner, currentHero->getVisibility(), 1);
    int heroId = currentHero->m_id;
    g_game->m_campaign.m_assignedCarryover.push_back(heroId);
}

// Mac retains this artifact offer at code 0:0x951b0. Its only caller passes
// one collected artifact and the selected player; VC6 expands the hero walk.
// Original helper spelling and linkage are unproved.
static void offerArtifactToPlayerHeroes(const type_artifact& artifact,
                                       int player)
{
    playerData& recipient = g_game->m_players[player];
    for (int playerHero = 0; playerHero < recipient.m_numHeroes; ++playerHero) {
        hero* target = g_game->getHero(recipient.m_heroes[playerHero]);
        if (target->giveArtifact(&artifact, 0, 0))
            break;
    }
}

// Complete-only, and the sibling of InitializeCrossoverHero above: the map
// hero placeholders that carry no crossover hero. The record's own hero id
// picks the path - -1 asks the game for a starting hero of the player's
// alignment, anything else re-homes the carried setup record first - and
// the rest is the shared tail: the hero lands on the object's trigger cell,
// steps one west off a town entrance, and is registered with its player,
// the availability table, the hero pool map and the fog.
// Mac stores the packed point's x/y/z fields in order. Expanding the point
// constructor into three field assignments drops Windows from 93.5215% to
// 88.94%; preserve the canonical constructor call.
VA(0x00487020, 0x263)  // anchor-caller(0x487290's two placeholder loops), retail-only
void TCampaignBrief::ScenarioStruct::placeStartingHero(
    HeroPlaceholderData* placeholder)
{
    int heroId = placeholder->m_heroId;
    int owner = static_cast<signed char>(placeholder->m_owner);
    if (heroId == -1) {
        heroId = g_game->getStartingHeroId(g_game->m_setup.m_alignment[owner],
                                           owner, 0);
    } else {
        g_game->rehomeCampaignHeroSetup(heroId);
    }

    if (g_game->m_setup.m_startingHero[owner] == -1)
        g_game->m_setup.m_startingHero[owner] = heroId;

    CObject* object = placeholder->m_object;
    hero* currentHero = g_game->getHero(heroId);
    currentHero->initialize(static_cast<short>(heroId));
    currentHero->m_order = 0;

    int triggerX;
    int triggerY;
    object->findTrigger(triggerX, triggerY);
    currentHero->m_x = static_cast<short>(triggerX);
    currentHero->m_y = static_cast<short>(triggerY);
    currentHero->m_z = object->m_z;
    currentHero->m_owner = static_cast<signed char>(placeholder->m_owner);
    g_game->setRandomHeroArmies(currentHero->m_id, 0, 0);
    currentHero->m_maxMovePoints = currentHero->m_movePoints
        = currentHero->getMobility();

    type_point heroLocation = currentHero->getLocation();
    --heroLocation.m_x;
    if (heroLocation.m_x >= 0) {
        NewmapCell* cell = g_game->m_worldMap.cell(heroLocation);
        if (cell->m_type == TOWN && cell->m_isTrigger)
            --currentHero->m_x;
    }

    g_game->m_players[currentHero->m_owner].m_heroes[
        g_game->m_players[currentHero->m_owner].m_numHeroes] = currentHero->m_id;
    ++g_game->m_players[currentHero->m_owner].m_numHeroes;
    currentHero->obscureCell();
    g_game->m_heroAvailability[currentHero->m_id] = currentHero->m_owner;
    g_game->m_heroPoolMap[currentHero->m_id].set(currentHero->m_owner);
    g_game->setVisibility(currentHero->m_x, currentHero->m_y, currentHero->m_z,
                          currentHero->m_owner, currentHero->getVisibility(), 1);
}

// The map placeholders are handed the campaign's carried heroes strongest
// first, so retail sorts them by their power rating before the pass below.
// Retail-only, name provisional (the whole std::sort instantiation this
// predicate pulls into customcampaign.obj is at 0x48eec0 and its helpers).
// Keep the comparator declaration in this module with its sole consumer.
struct HeroPlaceholderStronger {
    bool operator()(const HeroPlaceholderData& left,
                    const HeroPlaceholderData& right) const;
};

bool HeroPlaceholderStronger::operator()(const HeroPlaceholderData& left,
                                         const HeroPlaceholderData& right)
    const
{
    return static_cast<signed char>(left.m_powerRating)
        > static_cast<signed char>(right.m_powerRating);
}

hero* SCampaign::findCrossoverHero(int heroId)
{
    for (int poolIndex = m_carryOverHeroes.size(); poolIndex--;) {
        std::vector<hero>& pool = m_carryOverHeroes[poolIndex];
        for (int heroIndex = pool.size(); heroIndex--;) {
            if (pool[heroIndex].m_id == heroId)
                return &pool[heroIndex];
        }
    }
    return 0;
}

// Complete-only, and game::NewMap's second campaign callee (the first is
// SCampaign::DoPreLoadCustomization). The scenario's chosen start option
// names the crossover slot, that slot's hero pool is copied out of the
// campaign, and the map's placeholders take from it in power-rating order:
// a placeholder that names a specific hero id takes that hero (and drops it
// from the local pool so it cannot be handed out twice), and the remaining
// unnamed placeholders belonging to the option's own player take whatever is
// left. Anything the loss condition pins to a specific cell, and the
// player's first hero if it still has none, falls back to the placeholder
// path in PlaceStartingHero above.

// 2026-09-06: reverse searches use `for (i = size(); i--;)`, as retail's
// tests consume the old count (67.93 -> 74.5681). FindCrossoverHero restores
// the lookup boundary (+0x332..+0x3ad), and cell(lossHero) restores the
// existing packed-point wrapper: +0x528 calls cell(int,int,int). Flattening
// that wrapper to the three fields scores 87.2742; the canonical call
// reaches 98.0287 with all 71 blocks and 40 branches aligned. No DC body
// survives for this Complete-only caller; the cell wrapper is DC-proven.
// Residual (98.03%): 0x48 vs retail's 0x4c frame, packed-coordinate/trigger
// local sharing, and registers in the loss-condition tail. Both vector
// destructors now expand on the two early returns and stay called on the
// final exit, as retail requires. POD/STL folded names differ at six calls.
// Moving triggerX/Y before the point construction keeps the wrong 0x48
// frame (97.29); copy-initializing lossHero from a point value also keeps
// that frame and adds coordinate-packing differences (96.68). Both probes
// are rejected; neither recovers retail's separate trigger-output homes.
VA(0x00487290, 0x664)  // anchor-caller(game::NewMap +0x7ce), retail-only
void TCampaignBrief::ScenarioStruct::placeCrossoverHeroes()
{
    SCampaign* campaign = &g_game->m_campaign;
    int choice = campaign->m_briefingChoice;
    int player = m_options->getPlayer(choice);
    int slot = m_options->slot5(this, choice);
    campaign->m_mapScores[campaign->m_currentMap].m_index = slot;

    std::vector<hero> heroes;
    std::vector<HeroPlaceholderData> placeholders =
        g_game->m_worldMap.m_heroPlaceholders;
    if (placeholders.size() == 0)
        return;

    std::sort(placeholders.begin(), placeholders.end(),
              HeroPlaceholderStronger());

    if (slot >= 0)
        heroes = campaign->m_carryOverHeroes[slot];

    HeroPlaceholderData* placeholder;
    unsigned int placeholderIndex;
    for (placeholderIndex = 0; placeholderIndex < placeholders.size();
         ++placeholderIndex) {
        placeholder = &placeholders[placeholderIndex];
        if (placeholder->m_heroId == -1)
            continue;

        int carried;
        for (carried = heroes.size(); carried--;) {
            if (heroes[carried].m_id == placeholder->m_heroId)
                break;
        }
        if (carried >= 0)
            heroes.erase(heroes.begin() + carried);

        hero* carriedHero = campaign->findCrossoverHero(placeholder->m_heroId);
        if (carriedHero)
            initializeCrossoverHero(placeholder, carriedHero);
    }

    if (heroes.size() != 0) {
        for (placeholderIndex = 0; placeholderIndex < placeholders.size();
             ++placeholderIndex) {
            placeholder = &placeholders[placeholderIndex];
            if (placeholder->m_heroId == -1
                && static_cast<signed char>(placeholder->m_owner) == player) {
                initializeCrossoverHero(placeholder, heroes.begin());
                heroes.erase(heroes.begin());
                if (heroes.size() == 0)
                    break;
            }
        }
    }

    if (g_game->m_mapHeader.m_lossCondition.m_type == 1) {
        type_point lossHero(g_game->m_mapHeader.m_lossCondition.m_heroX,
                            g_game->m_mapHeader.m_lossCondition.m_heroY,
                            g_game->m_mapHeader.m_lossCondition.m_heroZ);
        NewmapCell* cell = g_game->m_worldMap.cell(lossHero);
        if (!cell->m_isTrigger || cell->m_type != HERO) {
            for (placeholderIndex = 0; placeholderIndex < placeholders.size();
                 ++placeholderIndex) {
                placeholder = &placeholders[placeholderIndex];
                CObject* object = placeholder->m_object;
                int triggerX;
                int triggerY;
                object->findTrigger(triggerX, triggerY);
                if (triggerX == lossHero.m_x && triggerY == lossHero.m_y
                    && object->m_z == lossHero.m_z) {
                    placeStartingHero(placeholder);
                    return;
                }
            }
        }
    }

    if (g_game->m_players[player].m_numHeroes == 0)
        placeStartingHero(placeholder);
}

// Complete-only helper shared by the scenario handoff and campaign pool
// pruning. One receiver and one artifact temporary span both artifact loops.
static void collectCrossoverArtifacts(const hero& sourceHero,
                                      std::vector<type_artifact>& artifacts)
{
    type_artifact artifact;
    int slot;
    for (slot = 0; slot < g_crossoverEquippedArtifactSlots; ++slot) {
        artifact = sourceHero.getArtifact(TArtifactSlot(slot));
        if (artifact.m_artifactId != ARTIFACT_NONE)
            artifacts.push_back(artifact);
    }
    for (slot = 0; slot < g_crossoverBackpackSlots; ++slot) {
        artifact = sourceHero.getBackpack(slot);
        if (artifact.m_artifactId != ARTIFACT_NONE)
            artifacts.push_back(artifact);
    }
}

// Complete-only, and game::NewMap's third campaign callee. The crossover
// slot's own artifact pool is copied out of the campaign and every artifact
// still equipped or carried by a pool hero that has NOT been placed on this
// map (its availability byte is still the tavern-pool sentinel) is added to
// it; then every artifact the scenario's crossover plane enables is offered
// to the option player's heroes in turn until one accepts it. The chosen
// start option's own Apply runs last, on every path.

// Retail snapshots the selected hero pool before allocating the artifact
// vector, and snapshots the recipient player across giveArtifact calls.
// The canonical collector shared with pruneCrossoverHeroes owns one artifact
// temporary across both inner loops. Its expansion restores retail's 0x64
// frame, shared stack home, and the first push_back's retained single-element
// insert wrapper. The two outer loops also reuse one index, restoring the
// retail -0x14 home. Mac code+0x95bc4..0x95bd4 initializes the local artifact's
// ID word before its payload word, supporting the TArtifact constructor here;
// this also makes the Windows body exact. Retail's
// ICF label names vector<type_dialog_resource>::insert at the wrapper address;
// the source-correct vector<type_artifact> specialization resolves there too.
VA(0x00487900, 0x2CD)  // anchor-caller(game::NewMap +0x5cb), retail-only
void TCampaignBrief::ScenarioStruct::giveCrossoverArtifacts()
{
    SCampaign* campaign = &g_game->m_campaign;
    int choice = campaign->m_briefingChoice;
    int player = m_options->getPlayer(choice);
    int slot = m_options->slot5(this, choice);
    if (slot >= 0) {
        std::vector<hero>& heroes = campaign->m_carryOverHeroes[slot];
        type_artifact artifact(ARTIFACT_NONE);
        std::vector<type_artifact> artifacts = campaign->m_carryoverArtifact[slot];

        unsigned int itemIndex;
        for (itemIndex = 0; itemIndex < heroes.size(); ++itemIndex) {
            hero& carried = heroes[itemIndex];
            if (g_game->m_heroAvailability[carried.m_id]
                != hero::HERO_AVAILABILITY_TAVERN_POOL)
                continue;
            collectCrossoverArtifacts(carried, artifacts);
        }

        for (itemIndex = 0; itemIndex < artifacts.size(); ++itemIndex) {
            artifact = artifacts[itemIndex];
            if (artifact.m_artifactId == ARTIFACT_NONE)
                continue;
            if (!m_crossoverArtifacts.at(artifact.m_artifactId))
                continue;
            offerArtifactToPlayerHeroes(artifact, player);
        }
    }

    m_options->apply(this);
}

// Complete-only. Seeks the campaign stream to this scenario's map data
// and reads the map header out of a gzip-inflating view of it.
VA(0x00487d30, 0x96)
void TCampaignBrief::ScenarioStruct::loadMapHeader(
    std::streambuf* stream, NewSMapHeader* mapHeader, int which)
{
    stream->pubseekoff(m_offset, std::ios::beg, std::ios::in);
    TGzInflateBuf inflateBuf(stream);
    TStreamBufFile file(&inflateBuf);
    mapHeader->read(&file, which);
}

VA_COMPGEN(0x00487dd0, 0x23, SCALAR_DELETING_DTOR, TAbstractFile)

VA_COMPGEN(0x00487e00, 0x07, IMPLICIT_DTOR, TStreamBufFile)

VA(0x00487e10, 0x2D)
void TCampaignBrief::ScenarioStruct::markCrossoverHeroes(unsigned char* wanted)
{
    for (unsigned int placeholderIndex = 0;
         placeholderIndex < m_heroPlaceholders.size(); ++placeholderIndex)
        wanted[m_heroPlaceholders[placeholderIndex]] = 1;
}

// Complete-only, reached only from CampaignHeaderStruct::Load's creation
// loop, which hands it the region's scenario count and the campaign file
// version alongside the inflating stream. Every read goes through the same
// TAbstractFile vtable slot 1.

// The prerequisite bitmap is (numScenarios + 7) / 8 bytes read into ONE
// dword and unpacked a bit at a time into the byte vector at +0x18, and the
// two crossover planes are Dinkumware bitsets built the same way that
// game::LoadMap builds the map's own artifact plane. Campaign files older
// than version 6 carry a 129-bit artifact plane, which is copied bit by bit
// into the 144-bit member.

// The tail is the whole reason this row waited: a type byte selects one of
// THREE starting-options records, all of them Complete-only classes that
// customcampaign.h now models (0x63d98c, 0x63dad8, 0x63db0c behind the
// abstract 0x63d958), and the function ends by handing the stream to
// whichever one it built through its own slot 9.

// Both the reader and campaign-brief window use the implemented
// TCampaignStartOption hierarchy. Its retained bodies prove the const query
// signatures and all thirteen vtable slots; no second interface is needed.

// The retained destination dereference at 0x48eb40 is annotated on the
// canonical generic body in bitset_iterator.h, with its 144-bit instance.

// The prerequisite-insert and packed-bit inline boundaries remain unfinished.
// Retail puts each absent-text arm first and retains the allocated MapTextStruct
// in EDI across all three reads. Null-first branches raise 70.55 -> 73.36%; the
// local text pointers then reach 80.95%. Member reloads after each virtual Read
// are the negative control. Explicitly widening the one-byte read buffers to
// masked ints was byte-flat and is not retained.
// Both present arms publish the new record before reading its fields. One
// ordinary MapTextStruct reader preserves that ordering and the EDI receiver;
// VC6 expands both calls and raises this reader from 84.9232% to 86.4073%.
// A pointer-returning factory reaches 87.7846% but publishes only after the
// virtual reads, contradicting retail, and is not retained.
// Keeping inflated-size's temporary in the function scope gives it retail's
// local home instead of reusing the infile parameter slot (86.4073 -> 87.4222).
// A named proxy in the shared readPackedBits changes the nested code generation
// in all three expansions (87.4222 -> 89.4456); the proxy-call boundary itself
// remains unfinished.
void TCampaignBrief::MapTextStruct::read(TAbstractFile* infile)
{
    unsigned char value;
    infile->read(&value, sizeof(unsigned char));
    m_video = value;
    infile->read(&value, sizeof(unsigned char));
    m_audio = value;
    m_subtitles = readLengthPrefixedString(infile);
}

VA(0x00487e40, 0x586)  // anchor-caller(CampaignHeaderStruct::Load +0x379), retail-only
void TCampaignBrief::ScenarioStruct::read(TAbstractFile* infile,
                                          int numScenarios,
                                          int campaignVersion)
{
    m_name = readLengthPrefixedString(infile);

    int size;
    infile->read(&size, sizeof(int));
    m_inflatedSize = size;

    int prerequisiteBits = 0;
    infile->read(&prerequisiteBits, (numScenarios + 7) / 8);
    for (int prereq = 0; prereq < numScenarios; ++prereq) {
        m_prerequisites.push_back((prerequisiteBits & (1 << prereq)) != 0);
    }

    {
        unsigned char value;
        infile->read(&value, sizeof(unsigned char));
        m_regionColor = value;
        infile->read(&value, sizeof(unsigned char));
        m_difficulty = value;
    }

    m_regionDesc = readLengthPrefixedString(infile);

    {
        unsigned char present;
        infile->read(&present, sizeof(unsigned char));
        if (!present) {
            m_prologue = 0;
        } else {
            MapTextStruct* text = new MapTextStruct;
            m_prologue = text;
            text->read(infile);
        }
    }

    {
        unsigned char present;
        infile->read(&present, sizeof(unsigned char));
        if (!present) {
            m_epilogue = 0;
        } else {
            MapTextStruct* text = new MapTextStruct;
            m_epilogue = text;
            text->read(infile);
        }
    }

    {
        unsigned char flags;
        infile->read(&flags, sizeof(unsigned char));
        m_retainXp = flags & 1;
        m_retainPskills = (flags >> 1) & 1;
        m_retainSskills = (flags >> 2) & 1;
        m_retainSpellbook = (flags >> 3) & 1;
        m_retainArtifacts = (flags >> 4) & 1;
    }

    m_crossoverCreatures = readPackedBits<g_crossoverCreatureBits>(infile);

    if (campaignVersion >= g_campaignVersionWideArtifacts) {
        m_crossoverArtifacts = readPackedBits<g_crossoverArtifactBits>(infile);
    } else {
        std::bitset<129> legacyArtifacts = readPackedBits<129>(infile);
        std::copy(
            bitset_iterator<129>(legacyArtifacts, 0),
            bitset_iterator<129>(legacyArtifacts, g_crossoverLegacyArtifactBits),
            bitset_iterator<144>(m_crossoverArtifacts, 0));
    }

    unsigned char optionType;
    infile->read(&optionType, sizeof(unsigned char));
    TCampaignStartOption* record;
    switch (optionType) {
    case CAMPAIGN_START_OPTION_BONUS:
        record = new TCampaignStartBonusOption;
        m_options = record;
        break;
    case CAMPAIGN_START_OPTION_CROSSOVER:
        record = new TCampaignStartCrossoverOption;
        m_options = record;
        break;
    case CAMPAIGN_START_OPTION_HERO:
        record = new TCampaignStartHeroOption;
        m_options = record;
        break;
    default:
        m_options = 0;
        break;
    }
    if (m_options)
        m_options->read(infile);
}

VA(0x004883d0, 0x21)  // anchor-caller(ScenarioStruct::Read's type-3 arm)
TCampaignStartHeroOption::TCampaignStartHeroOption()
{
}

VA_COMPGEN(0x00488400, 0x21, SCALAR_DELETING_DTOR, TCampaignStartCrossoverOption)
VA_COMPGEN(0x00488430, 0x21, SCALAR_DELETING_DTOR, TCampaignStartHeroOption)

VA_COMPGEN(0x00488460, 0x2C, IMPLICIT_DTOR, TCampaignStartCrossoverOption)
VA_COMPGEN(0x00488490, 0x2C, IMPLICIT_DTOR, TCampaignStartHeroOption)

VA(0x004884c0, 0x103)  // CampaignHeaderStruct::StartScenario sole caller
void TCampaignBrief::ScenarioStruct::startScenario(
    std::streambuf* stream, int option)
{
    int position = m_options->getPlayer(option);
    g_game->m_players[position].m_isHuman = 1;
    g_game->m_players[position].m_isLocal = 1;

    int playerHeroFaces[8];
    int i;
    MEMSET(playerHeroFaces, -1, sizeof(playerHeroFaces), i);
    playerHeroFaces[position] = m_options->slot7(option);
    g_game->setupFirstPlayer();

    stream->pubseekoff(m_offset, std::ios::beg, std::ios::in);
    TGzInflateBuf inflateBuf(stream);
    TStreamBufFile file(&inflateBuf);
    g_game->newMap(&file, playerHeroFaces, this, -1);
}

// Complete's constructor and destructor live before their callers in this
// TU. VC6 expands them in selectCampaign; other TUs retain source calls.
VA(0x004885d0, 0xCB)  // retained body and cross-TU callers
TCampaignBrief::CampaignHeaderStruct::CampaignHeaderStruct(
    const char* filename)
{
    m_fileName = filename;
    m_data = 0;
    m_stream = 0;
    m_fileError = CAMPAIGN_FILE_OK;
}

VA(0x004886a0, 0x132)  // retained body and cross-TU callers
TCampaignBrief::CampaignHeaderStruct::~CampaignHeaderStruct()
{
    clearScenarios();
}

// Complete-only; also reached from the custom-campaign list scanner
// (0x482fd0 family). Name provisional.
VA(0x004887e0, 0x30)
void TCampaignBrief::CampaignHeaderStruct::freeData()
{
    if (m_stream) {
        delete m_stream;
        m_stream = 0;
    }
    if (m_data) {
        delete[] m_data;
        m_data = 0;
    }
}

VA(0x00488810, 0x32)
bool TCampaignBrief::CampaignHeaderStruct::loadScenario(
    int which, NewSMapHeader* mapHeader)
{
    if (!load())
        return false;
    m_scenarios[which]->loadMapHeader(m_stream, mapHeader, which);
    return true;
}

// Complete-only; the 0x482fd0-family caller sizes the campaign list with
// it. Name provisional.
VA(0x00488850, 0x2F)
int TCampaignBrief::CampaignHeaderStruct::getNumMaps() const
{
    int numMaps = 0;
    for (unsigned int i = 0; i < m_scenarios.size(); ++i) {
        if (m_scenarios[i]->m_inflatedSize > 0)
            ++numMaps;
    }
    return numMaps;
}

// Complete-only campaign-file loader. Six callees identify it end to end:
//   * 0x488eb0, ScenarioStruct's scalar deleting destructor, reached by the
//     `delete scenarios[i]` sweep the destructor above already writes;
//   * 0x48b4a0, the out-of-line vector<T*>::clear COMDAT (mapcell.cpp's
//     ~NewfullMap reaches the same one);
//   * 0x485d90, hero.h's ReadLengthPrefixedString - the /Gr free reader that
//     returns by value, so the hidden return pointer takes ECX and the
//     stream is pushed out to EDX;
//   * 0x485f50 / 0x487e40, ScenarioStruct's constructor and record reader,
//     both claimed above;
//   * std::filebuf and std::strstreambuf, proven by `_Fiopen(name, 0x21)`
//     into `_Init(_F, _Openfl)` + `_Initcvt()` on a 0x54-byte object, and by
//     `_Init(size, data, 0, 0)` on a 0x50-byte one.
// The file is looked for in the maps directory first and falls back to the
// LOD bitmap resources; either way the raw campaign stream is wrapped in a
// TGzInflateBuf for the header block, and again per scenario for the map
// header the start-options record then folds into the scenario.

// homm3 vc6 predict-inline's passive trace measures caller cb=1077 and
// initial budget=2154. The scalar deleting destructor is cb=50 with budget
// 2112 and 28 sites remaining; freeData is cb=101 with budget 1952 and 26
// remaining. Both expand. Later, vector assignment is cb=369 with budget
// 571 and two sites remaining, and also expands. These are candidate
// measurements; retail's original caller budget has not been recovered.

// Complete shares clearScenarios with the exact destructor. Keeping the
// scenario-count loop here and moving one allocate/read/append operation to
// addCampaignScenario preserves the natural record boundary. The post-map
// helper leaves the exact loadMapHeader call and NewSMapHeader lifetime in
// this caller. Together these boundaries raise MAX to 82.4518%; moving the
// whole map loop behind a helper falls to 68.8816%. Static and class-member
// ownership forms produce identical loader bytes.

// Negative controls: direct insert(end(), 1, scenario) scores 44.4627%; a
// countdown scenario-record loop produces identical function bytes; limiting
// currentDirectory to the file-open scope leaves the score at 53.9715%.
// Keep reads through TAbstractFile*: retail uses the virtual slot at +4.
// Calling streamFile.read directly instead devirtualizes and expands them.
static void addCampaignScenario(
    TCampaignBrief::CampaignHeaderStruct& campaign, TAbstractFile* file,
    int numScenarios)
{
    TCampaignBrief::ScenarioStruct* scenario =
        new TCampaignBrief::ScenarioStruct;
    scenario->read(file, numScenarios, campaign.m_campaignVersion);
    campaign.m_scenarios.push_back(scenario);
}

static void applyCampaignMapHeader(
    TCampaignBrief::ScenarioStruct& scenario, NewSMapHeader& mapHeader)
{
    scenario.m_options->setTown(&mapHeader);
    scenario.m_heroPlaceholders = mapHeader.m_placeholders;
    for (int slotIndex = 0; slotIndex < 8; ++slotIndex)
        scenario.m_heroesStatus[slotIndex] =
            mapHeader.m_playerSlotAttributes[slotIndex].m_defaultPlaceholders;
}

VA(0x00488880, 0x5D6)  // anchor-caller(TCampaignBrief ctor), retail-only
bool TCampaignBrief::CampaignHeaderStruct::load()
{
    if (m_stream)
        return true;

    clearScenarios();

    std::filebuf* fileBuf = new std::filebuf;
    char currentDirectory[200];
    _getcwd(currentDirectory, sizeof(currentDirectory));
    _chdir(DATA_COMPGEN(0x006772d0, oldMainMapsDir, "maps"));
    fileBuf->open(m_fileName.c_str(), std::ios::in | std::ios::binary);
    _chdir(DATA_COMPGEN(0x006755a0, parentDirectory, ".."));
    if (!fileBuf->is_open()) {
        delete fileBuf;
        LODFile* resource =
            ResourceManager::pointToBitmapResource(m_fileName.c_str());
        if (!resource) {
            m_fileError = CAMPAIGN_FILE_OPEN_FAILED;
            return false;
        }
        int resourceSize =
            ResourceManager::getBitmapResourceSize(m_fileName.c_str());
        m_data = new unsigned char[resourceSize];
        ResourceManager::readFromBitmapResource(resource, m_data, resourceSize);
        m_stream = new std::strstreambuf(m_data, resourceSize);
    } else {
        m_stream = fileBuf;
    }

    int numScenarios;
    {
        TGzInflateBuf inflateBuf(m_stream);
        TStreamBufFile streamFile(&inflateBuf);
        // Retail reads through the abstract interface, not through the
        // concrete local: every read is an indirect `call [vptr+4]` on
        // TStreamBufFile's slot 1. Spelling `streamFile.Read(...)` instead
        // lets VC6 resolve the call statically and expand the one-line body,
        // which turns all six reads into direct sgetn calls on the inflater.
        TAbstractFile* file = &streamFile;
        int intBuffer;
        file->read(&intBuffer, 4);
        m_campaignVersion = intBuffer;
        if (m_campaignVersion < 4) {
            m_fileError = CAMPAIGN_FILE_VERSION_UNSUPPORTED;
            return false;
        }
        file->read(&intBuffer, 1);
        m_regionMap = intBuffer & 0xff;
        m_campaignName = readLengthPrefixedString(file);
        if (m_campaignName.length() == 0)
            m_campaignName = g_generalText->getText(GENERAL_TEXT_UNNAMED);
        m_campaignDesc = readLengthPrefixedString(file);
        char charBuffer;
        file->read(&charBuffer, 1);
        m_variableDifficulty = charBuffer != 0;
        if (m_campaignVersion < 5)
            m_campaignMusic = 0x25;
        else {
            file->read(&charBuffer, 1);
            m_campaignMusic = charBuffer;
        }
        numScenarios = g_campaignMapTraits[m_regionMap].m_numRegions;
        for (int newValue = 0; newValue < numScenarios; ++newValue)
            addCampaignScenario(*this, file, numScenarios);
    }

    int mapOffset = m_stream->pubseekoff(0, std::ios::cur, std::ios::in);
    NewSMapHeader mapHeader;
    for (int scenario2 = 0; scenario2 < numScenarios; ++scenario2) {
        m_scenarios[scenario2]->m_offset = mapOffset;
        if (m_scenarios[scenario2]->m_inflatedSize > 0) {
            mapOffset += m_scenarios[scenario2]->m_inflatedSize;
            m_scenarios[scenario2]->loadMapHeader(m_stream, &mapHeader,
                                                 scenario2);
            applyCampaignMapHeader(*m_scenarios[scenario2], mapHeader);
        }
    }
    return true;
}

VA(0x00488ee0, 0x1D)
void TCampaignBrief::CampaignHeaderStruct::startMusic()
{
    g_soundManager->startMP3(g_campaignMusicTraits[m_campaignMusic].m_name, 0, 1);
}

// Mac retains this shared prerequisite check at code 0:0x9758c. Both the
// start-option predicate and getAvailableScenarios call it; VC6 expands the
// same loop in their retail bodies. Original method spelling is unproved.
bool TCampaignBrief::ScenarioStruct::prerequisitesMet() const
{
    for (unsigned int i = 0; i < m_prerequisites.size(); ++i)
        if (m_prerequisites[i]
            && !g_game->m_campaign.m_mapScores[i].m_completed)
            return false;
    return true;
}

// Complete-only. A scenario without map data is marked unavailable and
// already completed; otherwise every prerequisite scenario must be
// completed in the running campaign's score table.
VA(0x00488f00, 0xAC)
void TCampaignBrief::CampaignHeaderStruct::getAvailableScenarios(
    unsigned char* available) const
{
    for (unsigned int i = 0; i < m_scenarios.size(); ++i) {
        ScenarioStruct* scenario = m_scenarios[i];
        available[i] = 1;
        if (scenario->m_inflatedSize <= 0) {
            available[i] = 0;
            g_game->m_campaign.m_mapScores[i].m_completed = true;
        } else if (!scenario->prerequisitesMet())
            available[i] = 0;
    }
}

// Mac code+0x976c4 follows getAvailableScenarios and retains the selection
// as a call from the campaign-header playback helper. Complete expands it.
void TCampaignBrief::ScenarioStruct::playText(bool epilogue)
{
    MapTextStruct* text = epilogue ? m_epilogue : m_prologue;
    if (text)
        text->play();
}

VA(0x00488fb0, 0x528)  // PlayScenarioPrologue callee + music-cell reader, retail-only
void TCampaignBrief::MapTextStruct::play()
{
    // The subtitle completion flag can also end playback after input.
    // Retaining the event switch and testing finished removes three jumps
    // at unchanged 83.8848%; an if-chain for the same events gives 80.7396%.
    // Retail assigns each playback endpoint only on the path that consumes it;
    // dropping three eager zero stores raises MAX 85.4677 -> 86.1889.
    if (m_video < 0)
        return;

    int videoY = 0;
    if (m_video == g_campaignVideoLowered
        || (m_video >= g_campaignVideoFirstLowered
            && m_video != g_campaignVideoRaised))
        videoY = g_campaignVideoLoweredY;

    videoOpen(g_campaignVideoIds[m_video], 0, videoY, 0, 0,
              m_video != g_campaignVideoNoFade, 0, 1);

    unsigned char finished = 0;
    unsigned char speechStarted = 0;
    int mp3Started = 0;
    int savedVolume = g_config.m_musicVolume;
    long nextScroll = GameTime::get() + g_campaignScrollInterval;
    int textHeight = g_bigFont->lineLength(m_subtitles.c_str(),
                                            g_campaignSubtitleWidth)
                      * g_bigFont->m_fs.m_height;

    Bitmap16Bit* strip = 0;
    int scrollY = 0;
    int scrollDelay = g_bigFont->m_fs.m_height;
    unsigned char videoDone = 0;
    unsigned char redraw = 1;
    const char* music = 0;
    const char* speechName = 0;

    if (g_game->m_campaign.m_currentCampaign
        != SCampaign::PRE36_CAMPAIGN_REMAP_TARGET) {
        speechName = g_campaignVideoSounds[m_video];
        if (!*speechName)
            speechName = 0;
    }
    if (m_audio != -1)
        music = g_campaignMusicTraits[m_audio].m_name;

    g_windowManager->m_screenBitmap->fillRect(0, 0, 800, 600, 0);

    if (m_subtitles.length() > 0 && (g_config.m_videoSubtitles || !speechName)) {
        if (textHeight < g_campaignSubtitleHeight)
            textHeight = g_campaignSubtitleHeight;
        strip = new Bitmap16Bit(g_campaignSubtitleWidth,
                                textHeight + g_bigFont->m_fs.m_height);
        if (!strip)
            memError();
        strip->fillRect(0, 0, strip->getWidth(), strip->getHeight(), 0);
        g_bigFont->drawBoundedString(m_subtitles.c_str(), strip, 0, 0,
                                     strip->getWidth(), strip->getHeight(),
                                     font::TColor(g_campaignSubtitleColor),
                                     font::CENTER_JUSTIFIED, -1);
    }

    sample* speech;
    if (!speechName)
        speech = 0;
    else
        speech = ResourceManager::getSample(speechName);

    g_inputManager->flush();
    g_windowManager->updateScreen(0, 0, 800, 600);

    unsigned char textDone;
    long textEnd;
    if (strip) {
        textDone = 0;
    } else {
        textDone = 1;
        textEnd = GameTime::get();
    }

    unsigned char speechDone;
    long speechEnd;
    if (!speech) {
        speechDone = 1;
    } else {
        speechDone = 0;
        speechEnd = GameTime::get();
    }

    long videoEnd;
    if (!g_smackVideo2) {
        videoDone = 1;
        videoEnd = GameTime::get();
    }

    for (;;) {
        pollSound();
        process1WindowsMessage();
        if (videoNeedsUpdate())
            videoDrawRects();

        if (static_cast<long>(GameTime::get() - nextScroll) >= 0) {
            if (strip) {
                if (redraw) {
                    if (scrollDelay)
                        --scrollDelay;
                    else if (scrollY < textHeight
                                            - g_campaignSubtitleScrollMargin)
                        ++scrollY;
                }
                strip->fillRect(g_campaignSubtitleX, g_campaignSubtitleY,
                                g_campaignSubtitleWidth,
                                g_campaignSubtitleHeight, 0);
                if (scrollDelay) {
                    strip->draw(0, 0, g_campaignSubtitleWidth,
                                g_campaignSubtitleHeight - scrollDelay,
                                g_windowManager->m_screenBitmap->getMap(0, 0),
                                g_campaignSubtitleX,
                                g_campaignSubtitleY + scrollDelay,
                                g_windowManager->m_screenBitmap->getWidth(),
                                g_windowManager->m_screenBitmap->getHeight(),
                                g_windowManager->m_screenBitmap->getPitch(), false);
                } else {
                    if (scrollY >= textHeight - g_campaignSubtitleHeight) {
                        if (!textDone)
                            textEnd = GameTime::get();
                        textDone = 1;
                    }
                    strip->draw(0, scrollY, g_campaignSubtitleWidth,
                                g_campaignSubtitleHeight,
                                g_windowManager->m_screenBitmap->getMap(0, 0),
                                g_campaignSubtitleX, g_campaignSubtitleY,
                                g_windowManager->m_screenBitmap->getWidth(),
                                g_windowManager->m_screenBitmap->getHeight(),
                                g_windowManager->m_screenBitmap->getPitch(), false);
                }
                if (redraw) {
                    g_windowManager->updateScreen(g_campaignSubtitleX,
                                                  g_campaignSubtitleY,
                                                  g_campaignSubtitleWidth,
                                                  g_campaignSubtitleHeight);
                    redraw = 0;
                }
            }

            if (!speechStarted && (mp3Started || m_audio == -1)
                && speech) {
                g_soundManager->memorySample(speech);
                speechStarted = 1;
            } else if (mp3Started) {
                if (speech && !speechDone) {
                    speechDone = g_soundManager->getSampleInfo(
                                      speech->m_memSample.m_memSampleHandle,
                                      g_campaignSpeechSampleStatus)
                                  == 0;
                    if (speechDone)
                        speechEnd = GameTime::get();
                }
            } else if (music && AIL_stream_status(g_mp3Stream)
                                    != AIL_STREAM_PLAYING) {
                g_config.m_musicVolume /= 2;
                g_soundManager->startMP3(music, 0, 1);
                mp3Started = 1;
            }

            if (g_config.m_videoSubtitles)
                redraw = 1;
            if (!videoDone && !g_smackVideo) {
                videoDone = 1;
                videoEnd = GameTime::get();
            }
            nextScroll = GameTime::get() + g_campaignScrollInterval;

            if (speechDone
                && static_cast<long>(GameTime::get() - speechEnd)
                       >= g_campaignLingerMs
                && textDone
                && static_cast<long>(GameTime::get() - textEnd)
                       >= g_campaignLingerMs
                && videoDone
                && static_cast<long>(GameTime::get() - videoEnd)
                       >= g_campaignVideoLingerMs)
                finished = 1;
        }

        message msg = g_inputManager->getEvent();
        switch (msg.m_id) {
        case MESSAGE_KEY_DOWN:
            if (msg.m_codeX != g_campaignSkipKey)
                finished = 1;
            break;
        case MESSAGE_LEFT_BUTTON_DOWN:
        case MESSAGE_RIGHT_BUTTON_DOWN:
            finished = 1;
            break;
        }
        if (finished)
            break;
    }

    if (speech) {
        g_soundManager->stopSample(speech->m_memSample.m_memSampleHandle);
        speech->dispose();
    }
    delete strip;
    g_config.m_musicVolume = savedVolume;
    videoClose();
}

// Mac code+0x97dbc follows MapTextStruct::play and calls playText. The two
// campaign wrappers are its only direct Mac callers, with three call sites.
void TCampaignBrief::CampaignHeaderStruct::playScenarioText(int which, bool epilogue)
{
    g_soundManager->stopAllSamples(1);
    m_scenarios[which]->playText(epilogue);
}

VA(0x004894e0, 0x1D)
void TCampaignBrief::CampaignHeaderStruct::startScenario(
    int which, int option)
{
    m_scenarios[which]->startScenario(m_stream, option);
}

// Mac retains the per-scenario score reader at code 0:0x97e70. SCampaign's
// load loop calls it once per row; VC6 expands the same five ordered reads.
void CampaignScenarioInfo::read(TAbstractFile* infile)
{
    m_completed = readValue<unsigned char>(infile) != 0;
    m_days = readValue<int>(infile);
    m_score = readValue<int>(infile);
    m_completeOrder =
        static_cast<signed char>(readValue<unsigned char>(infile));
    m_index =
        static_cast<signed char>(readValue<unsigned char>(infile));
}

// Mac retains this per-scenario score writer at code 0:0x97f74. SCampaign's
// save loop calls it once per row; VC6 expands its five ordered writes.
void CampaignScenarioInfo::write(TAbstractFile* outfile) const
{
    {
        char flag = m_completed;
        outfile->write(&flag, sizeof(flag));
    }
    {
        int intBuffer = m_days;
        outfile->write(&intBuffer, sizeof(intBuffer));
    }
    {
        int intBuffer = m_score;
        outfile->write(&intBuffer, sizeof(intBuffer));
    }
    {
        char flag = m_completeOrder;
        outfile->write(&flag, sizeof(flag));
    }
    {
        char flag = m_index;
        outfile->write(&flag, sizeof(flag));
    }
}

VA(0x00489590, 0x233)
void SCampaign::selectCampaign(int campaignIndex, const char* filename)
{
    m_mapScores.clear();
    m_currentCampaign = campaignIndex;
    m_currentMap = 0;
    m_crossoverArrayIndex = -1;
    m_campaignFilename = filename;
    m_carryOverHeroes.clear();

    TCampaignBrief::CampaignHeaderStruct campaign(filename);
    campaign.load();
    int count = campaign.m_scenarios.size();
    CampaignScenarioInfo blank;
    for (int scenarioIndex = 0; scenarioIndex < count; ++scenarioIndex)
        m_mapScores.push_back(blank);
}

// The campaign and map ordinals retail's end-of-map bookkeeping compares
// against literally; the same spelling kb.cpp uses for oldmain's own pair.
const int g_campaignOrdinal02 = 2;
const int g_campaignOrdinal07 = 7;
const int g_campaignOrdinal13 = 13;
const int g_campaignOrdinal18 = 18;
const int g_campaignMapOrdinal06 = 6;
const int g_campaignMapOrdinal07 = 7;

// PRICED 2026-09-06 - do not spend a lane on the /Ob2 side of this row. An
// `if (0)` mass titration over N = 1,2,4,8,16,32,64 inert statements is flat
// at 78.6801 to the digit through N=16, peaks at 79.2549 (N=32) and falls to
// 76.5288 (N=64); in the other direction, lifting the exclusion loop out of
// `caller_cb` costs 11.4 (67.32) and lifting the complete_order loop costs
// 2.5 (76.20, at 95-vs-95 blocks and 22 exact). So the whole reachable
// budget spread here is under 0.6 points and the residual below is the
// entire remaining story.

// 2026-09-06, polish lane 38 (78.6801 -> 85.5843): the excluded-hero scan is
// a post-decrement `while (pool--)` / `while (which--)` pair over a named
// `pooled` reference, the same shape DoPreLoadCustomization proves; retail's
// tell sits at fn+0xaad (`mov eax,edx / dec edx / test eax,eax`).
// Retail computes days before publishing completion and delays the zero
// complete-order store until after getMapScore (85.5843 -> 86.2644). A
// function-scope map-score counter then reproduces the zero held in EDI from
// the prologue and raises MAX to 87.6437. The remaining call mismatch is the
// vector temporary/insert inline family: retail retains both `_Destroy`
// helpers and the two-argument hero insert wrapper.
// LADDER, measured 2026-09-06 and NOT shipped: all nine appends spelled
// `insert(end(), x)` instead of `push_back(x)` is worth 78.6801 -> 78.8448,
// 0.16 of a point (about 2.5 B of a 1536 B body) for nine rewritten call
// sites - noise, and the same size lane 29 declined on its own
// CompleteCurrentMap twin.  The reverse rung on PruneCrossoverHeroes below
// LOSES 1.98.
VA(0x00489820, 0x600)  // anchor-caller(oldmain end-of-campaign arm), retail-only
void SCampaign::completeCurrentMap(void* campaignHeader)
{
    CampaignScenarioInfo& scenario = m_mapScores[m_currentMap];
    unsigned int i = 0;

    if (scenario.m_completed)
        return;

    scenario.m_days = g_game->getCurrentTurn();
    scenario.m_completed = true;
    scenario.m_completeOrder = 0;
    scenario.m_score = g_game->getMapScore();

    if (scenario.m_index < 0) {
        scenario.m_index = m_carryOverHeroes.size();
        // LADDER, measured 2026-09-06 and REVERTED: this append spelled
        // `insert(carryOverHeroes.end(), ...)` is worth 78.6801 -> 80.2280
        // (+24 B), but the direct spelling lets VC6 expand
        // `vector<vector<hero>>::insert` in full and FIVE named COMDATs stop
        // being emitted - insert (528 B), _Ucopy, _Ufill, std::fill and
        // std::copy_backward, all four banked EXACT - for a net loss of four
        // exact rows and 0.08 tree fuzzy.  When a rung would delete the last
        // out-of-line instantiation of a template in the TU, price the
        // COMDATs it takes with it, not just the row.
        m_carryOverHeroes.push_back(std::vector<hero>());
        m_carryoverArtifact.push_back(std::vector<type_artifact>());
    }

    m_crossoverArrayIndex = scenario.m_index;

    for (; i < m_mapScores.size(); ++i) {
        if (m_mapScores[i].m_completed
            && m_mapScores[i].m_completeOrder > scenario.m_completeOrder)
            scenario.m_completeOrder = m_mapScores[i].m_completeOrder;
    }
    ++scenario.m_completeOrder;

    for (unsigned int excluded = 0; excluded < m_assignedCarryover.size(); ++excluded) {
        int heroId = m_assignedCarryover[excluded];
        int pool = m_carryOverHeroes.size();
        while (pool--) {
            std::vector<hero>& pooled = m_carryOverHeroes[pool];
            int which = pooled.size();
            while (which--) {
                if (pooled[which].m_id == heroId)
                    break;
            }
            if (which >= 0) {
                pooled.erase(pooled.begin() + which);
                break;
            }
        }
    }

    std::vector<hero>& crossover = m_carryOverHeroes[m_crossoverArrayIndex];

    int gamePos;
    for (gamePos = 0; gamePos < 8; ++gamePos) {
        if (g_game->m_players[gamePos].isHuman())
            break;
    }

    playerData* player = &g_game->m_players[gamePos];
    for (int heroIndex = 0; heroIndex < player->m_numHeroes; ++heroIndex)
        crossover.push_back(*g_game->getHero(player->m_heroes[heroIndex]));

    for (int townIndex = 0; townIndex < player->m_numTowns; ++townIndex) {
        town* thisTown = g_game->getTown(player->m_townIds[townIndex]);
        if (thisTown->m_garrisonHeroId >= 0)
            crossover.push_back(*g_game->getHero(thisTown->m_garrisonHeroId));
    }

    if (m_currentCampaign == g_campaignOrdinal07
        && m_currentMap == g_campaignMapOrdinal06) {
        if (g_game->m_heroes[155].m_owner != gamePos)
            crossover.push_back(g_game->m_heroes[155]);
    }
    if (m_currentCampaign == g_campaignOrdinal18
        && m_currentMap == g_campaignMapOrdinal07) {
        if (g_game->m_heroes[27].m_owner != gamePos)
            crossover.push_back(g_game->m_heroes[27]);
        if (g_game->m_heroes[102].m_owner != gamePos)
            crossover.push_back(g_game->m_heroes[102]);
        if (g_game->m_heroes[148].m_owner != gamePos)
            crossover.push_back(g_game->m_heroes[148]);
        if (g_game->m_heroes[96].m_owner != gamePos)
            crossover.push_back(g_game->m_heroes[96]);
    }

    m_carryoverArtifact[m_crossoverArrayIndex].clear();

    if (campaignComplete()) {
        m_campaignCompleted[m_currentCampaign] = 1;
        if (m_currentCampaign >= g_campaignOrdinal07
            && m_currentCampaign < g_campaignOrdinal13 && !m_isCheater) {
            // Mac completeCurrentMap +0x480 calls the retained getScore
            // body, then compares its result with 350.
            if (getScore() >= 350)
                m_secretActive = 1;
        }
    }

    pruneCrossoverHeroes(campaignHeader);
}

// Four ordinary helpers have retained Mac bodies: markRequiredCampaignHeroes
// at code+0x96cd8 (including the wanted-array clear), usesCrossoverPool at
// +0x96060, getMaxCrossoverHeroes at +0x95ea8, and collectCrossoverArtifacts
// at +0x95a78. Mac pruneCrossoverHeroes (+0x98934) calls all four. VC6 expands
// them into this caller; the flattened spelling has 91 CFG blocks against
// retail's 62. getCampaignScenarioCount remains a Windows-side inference.
static int getCampaignScenarioCount(TCampaignBrief::CampaignHeaderStruct& header)
{
    return header.m_scenarios.size();
}

static void markRequiredCampaignHeroes(TCampaignBrief::CampaignHeaderStruct& header, unsigned char* wanted)
{
    memset(wanted, 0, game::HERO_COUNT);
    for (unsigned int mapIndex = 0; mapIndex < header.m_scenarios.size(); ++mapIndex) {
        if (!g_game->m_campaign.m_mapScores[mapIndex].m_completed)
            header.m_scenarios[mapIndex]->markCrossoverHeroes(wanted);
    }
}

static bool usesCrossoverPool(TCampaignBrief::ScenarioStruct& scenario, int pool)
{
    return scenario.m_inflatedSize > 0
        && scenario.m_options->slot12(&scenario, pool);
}

static int getMaxCrossoverHeroes(TCampaignBrief::ScenarioStruct& scenario)
{
    int best = 0;
    if (scenario.m_inflatedSize > 0) {
        if (scenario.m_options->getCount() == 0)
            best = scenario.m_heroesStatus[scenario.m_options->getPlayer(-1)];
        else
            for (int option = scenario.m_options->getCount(); option--;)
                best = max(best, scenario.m_heroesStatus[scenario.m_options->getPlayer(option)]);
    }
    return best;
}

// CompleteCurrentMap's tail call flags heroes requested by unfinished
// scenarios, keeps those heroes plus each pool's strongest permitted ones,
// gathers the leftovers' artifacts, then replaces the pool with its keep list.
// Role-based provisional name; no Dreamcast counterpart survives.

// Retail's reverse loops test the count before decrementing. Pool and
// option counters are signed; hero selection and artifact counters are
// unsigned. Unsigned `i--` gives the JE entry / JA backedge also proved by
// exact DoPreLoadCustomization. `size()-1; i>=0; --i` adds wrong signed
// exits; `i-- > 0` instead leaves JBE entries. The forward scenario loop
// ends in signed JL at +0x264, requiring a signed index and comparison.

VA(0x00489e20, 0x450)  // anchor-caller(CompleteCurrentMap +0x5e8), retail-only
void SCampaign::pruneCrossoverHeroes(void* campaignHeader)
{
    TCampaignBrief::CampaignHeaderStruct* header =
        static_cast<TCampaignBrief::CampaignHeaderStruct*>(campaignHeader);

    unsigned char wanted[game::HERO_COUNT];
    markRequiredCampaignHeroes(*header, wanted);

    for (int pool = m_carryOverHeroes.size(); pool--;) {
        std::vector<hero>& pooled = m_carryOverHeroes[pool];
        std::vector<hero> kept;

        for (unsigned int which = pooled.size(); which--;) {
            if (wanted[pooled[which].m_id]) {
                kept.push_back(pooled[which]);
                pooled.erase(pooled.begin() + which);
            }
        }

        int keepCount = 0;
        for (int scenarioIndex = 0;
             scenarioIndex < getCampaignScenarioCount(*header);
             ++scenarioIndex) {
            TCampaignBrief::ScenarioStruct* scenario =
                header->m_scenarios[scenarioIndex];
            if (!m_mapScores[scenarioIndex].m_completed && usesCrossoverPool(*scenario, pool))
                keepCount = max(keepCount, getMaxCrossoverHeroes(*scenario));
        }

        std::sort(pooled.begin(), pooled.end(), CrossoverHeroStronger());

        for (int taken = 0; taken < keepCount; ++taken) {
            if (pooled.size() == 0)
                break;
            kept.push_back(pooled.front());
            pooled.erase(pooled.begin());
        }

        for (unsigned int rest = pooled.size(); rest--;) {
            std::vector<type_artifact>& pooledArtifacts = m_carryoverArtifact[pool];
            collectCrossoverArtifacts(pooled[rest], pooledArtifacts);
        }

        std::sort(kept.begin(), kept.end(), CrossoverHeroStronger());
        pooled = kept;
    }
}

// Mac +0x98b7c retains this scan between pruneCrossoverHeroes and the
// epilogue wrapper. Its sole direct caller is getText at +0x93974; the
// Windows caller expands the same loop. The provisional name describes
// the returned score-table index.
int SCampaign::findLatestCrossoverScenario(int slot) const
{
    unsigned int score;
    int source = -1;
    for (score = 0; score < m_mapScores.size(); ++score)
        if (m_mapScores[score].m_completed
            && m_mapScores[score].m_index == slot
            && (source < 0
                || m_mapScores[score].m_completeOrder
                       >= m_mapScores[source].m_completeOrder))
            source = score;
    return source;
}

VA(0x0048a270, 0x2F)
void SCampaign::playScenarioPrologue(void* campaignHeader)
{
    int map = m_currentMap;
    TCampaignBrief::CampaignHeaderStruct* header =
        static_cast<TCampaignBrief::CampaignHeaderStruct*>(campaignHeader);
    header->playScenarioText(map, false);
}

VA(0x0048a2a0, 0x70)
void SCampaign::playScenarioEpilogue(void* campaignHeader)
{
    int map = m_currentMap;
    TCampaignBrief::CampaignHeaderStruct* header =
        static_cast<TCampaignBrief::CampaignHeaderStruct*>(campaignHeader);
    header->playScenarioText(map, true);
    if (m_currentCampaign == g_campaignOrdinal02 && m_mapScores[0].m_completed
        && m_mapScores[1].m_completed && !m_mapScores[2].m_completed) {
        header->playScenarioText(g_campaignOrdinal02, false);
    }
}

// Complete campaign deserialization; no Dreamcast SCampaign counterpart.
// Retail proves the 0x66a9-byte legacy record, sixteen 0x462-byte heroes,
// and field-by-field promotion (not a whole-record copy). The modern arm
// mirrors save(): one pool count resizes both carry-over vectors.
// Pool references preserve addresses evaluated before reads and constructor
// calls; the legacy hero-count bound is re-read each turn. The score vector
// is accessed through its campaign owner, without a cached outer reference.
// Both leading clear() wrappers must remain: their nested erase calls agree
// with retail, while spelling erase(begin(), end()) expands those workers.

// Retail's count loads mask to 8/16 bits and keep signed int loop bounds.
// Artifact fields sign-extend two-byte reads into the shared TArtifact type.
// The legacy secret flag is reset after assigning the campaign filename.

// Mac retains this pre-v28 per-hero conversion at code 0:0x98cb0 and calls it
// from SCampaign::load. VC6 expands the helper at that same source call and
// changes the surrounding vector inlining. Its original spelling is unproved.
static void convertLegacyCampaignHero(hero& newHero,
                                      const LegacyCampaignHero& oldHero)
{
    newHero.m_id = oldHero.m_id;
    newHero.m_owner = oldHero.m_owner;
    strcpy(newHero.m_name, oldHero.m_name);
    newHero.m_heroClass = oldHero.m_heroClass;
    newHero.m_portrait = oldHero.m_portrait;
    newHero.m_lastMagicSchoolLevel = oldHero.m_lastMagicSchoolLevel;
    newHero.m_experience = oldHero.m_experience;
    newHero.m_level = oldHero.m_level;
    newHero.m_levelSeed = oldHero.m_levelSeed;
    newHero.m_lastWisdom = oldHero.m_lastWisdom;
    newHero.m_army = oldHero.m_army;
    memcpy(newHero.m_skillLevel, oldHero.m_skillLevel,
           sizeof(newHero.m_skillLevel));
    memcpy(newHero.m_skillOrder, oldHero.m_skillOrder,
           sizeof(newHero.m_skillOrder));
    newHero.m_skillCount = oldHero.m_skillCount;

    for (int equippedSlot = 0; equippedSlot < 19; ++equippedSlot) {
        type_artifact artifact = oldHero.m_equipped[equippedSlot];
        if (artifact.m_artifactId != ARTIFACT_NONE)
            newHero.equipArtifact(&artifact, equippedSlot);
    }
    for (int backpackSlot = 0; backpackSlot < 64; ++backpackSlot) {
        type_artifact artifact = oldHero.m_backpack[backpackSlot];
        if (artifact.m_artifactId != ARTIFACT_NONE)
            newHero.addToBackpack(&artifact, backpackSlot);
    }
    for (int spell = 0; spell < 70; ++spell) {
        if (oldHero.m_inSpellbook[spell])
            newHero.addSpell(spell);
    }
    for (int stat = 0; stat < 4; ++stat)
        newHero.setPrimarySkill(stat, oldHero.m_stats[stat]);
}

// Inferred serialized-record operations: the assigned-hero count and signed
// ID stream belong to the campaign, while a scenario score reader fills one
// already selected record. Both ordinary helpers auto-inline into load.
// They preserve the scalar read order and widths; no DC declaration or
// retained retail procedure proves these names or interfaces.
static void readAssignedCampaignHeroes(TAbstractFile* infile,
                                      SCampaign& campaign)
{
    int count = readValue<unsigned char>(infile);
    campaign.m_assignedCarryover.resize(count);
    for (int assignedIndex = 0; assignedIndex < count; ++assignedIndex) {
        campaign.m_assignedCarryover[assignedIndex] = readValue<short>(infile);
    }
}

// 99.0228%: the record readers, direct score-vector ownership and shared
// inner counter recover every retail call decision, stack home and the
// legacy arm. Of 89 blocks, 88 have exact sizes; the modern hero-load loop
// still forms its receiver differently, followed by artifact-read register
// differences. Named hero pointer/reference receiver probes are byte-flat.
// Further indexed-iterator, shared-count and free/member reader models do
// not improve this. DC has no corresponding load: its campaign pools are
// fixed arrays and its hero loader predates the versioned stream interface.
VA(0x0048a310, 0xB1E)  // SavedGameHeader::Load caller + member/helper graph
void SCampaign::load(TAbstractFile* infile, int saveVersion)
{
    int i;
    int pool;
    int inner;
    m_mapScores.clear();
    m_carryOverHeroes.clear();

    if (saveVersion < 28) {
        LegacyCampaignSave saved;
        infile->read(&saved, sizeof(saved));

        m_currentMap = saved.m_currentMap;
        m_isCheater = saved.m_isCheater;
        m_currentCampaign = saved.m_currentCampaign;
        m_numMapRegions = -1;
        m_briefingChoice = saved.m_briefingChoice;
        m_crossoverArrayIndex = 0;
        m_campaignFilename = saved.m_campaignFilename;
        m_secretActive = false;

        memset(m_campaignCompleted, 0, sizeof(m_campaignCompleted));
        // Retail 0x48a406..0x48a41f copies 4+2+1 bytes: only the seven
        // built-in campaigns are promoted from the legacy eight-slot array.
        memcpy(m_campaignCompleted, saved.m_campaignCompleted, 7);

        m_mapScores.resize(saved.m_numScenarios);
        for (i = 0; i < saved.m_numScenarios; ++i) {
            CampaignScenarioInfo& scenario = m_mapScores[i];
            // Retail +0x191 preserves this field assignment order.
            scenario.m_days = saved.m_scenarioDays[m_currentCampaign][i];
            scenario.m_index = g_legacyCampaignScenarioIndices[m_currentCampaign][i];
            scenario.m_score = saved.m_scenarioScores[m_currentCampaign][i];
            scenario.m_completed =
                saved.m_scenarioCompleted[m_currentCampaign][i];
        }

        m_carryOverHeroes.resize(2);
        m_carryoverArtifact.resize(2);

        for (pool = 0; pool < 2; ++pool) {
            std::vector<hero>& heroPool = m_carryOverHeroes[pool];
            heroPool.resize(saved.m_carryOverHeroCounts[pool]);

            for (inner = 0;
                 inner < saved.m_carryOverHeroCounts[pool]; ++inner) {
                const LegacyCampaignHero& oldHero =
                    saved.m_carryOverHeroes[pool][inner];
                hero& newHero = heroPool[inner];

                convertLegacyCampaignHero(newHero, oldHero);
            }
        }
        return;
    }

    m_isCheater = readValue<unsigned char>(infile) != 0;
    if (saveVersion >= 26) {
        m_secretActive = readValue<unsigned char>(infile) != 0;
    } else {
        m_secretActive = false;
    }
    m_currentMap = readValue<unsigned char>(infile);
    m_currentCampaign = readValue<unsigned char>(infile);
    if (saveVersion < 36
            && m_currentCampaign == PRE36_CAMPAIGN_REMAP_SOURCE)
        m_currentCampaign = PRE36_CAMPAIGN_REMAP_TARGET;
    m_numMapRegions = static_cast<signed char>(readValue<unsigned char>(infile));
    m_crossoverArrayIndex = readValue<unsigned char>(infile);
    m_briefingChoice = static_cast<signed char>(readValue<unsigned char>(infile));

    m_campaignFilename = readLengthPrefixedString(infile);
    if (saveVersion >= 36) {
        infile->read(m_campaignCompleted, sizeof(m_campaignCompleted));
    } else {
        infile->read(m_campaignCompleted, 14);
        std::fill(m_campaignCompleted + 14,
                  m_campaignCompleted + sizeof(m_campaignCompleted), 0);
    }

    int count = readValue<unsigned char>(infile);
    m_mapScores.resize(count);
    for (i = 0; i < count; ++i) {
        CampaignScenarioInfo& scenario = m_mapScores[i];
        scenario.read(infile);
    }

    count = readValue<unsigned char>(infile);
    m_carryOverHeroes.resize(count);
    m_carryoverArtifact.resize(count);

    for (pool = 0; pool < count; ++pool) {
        std::vector<hero>& heroPool = m_carryOverHeroes[pool];
        int heroCount = readValue<unsigned char>(infile);
        heroPool.resize(heroCount);
        for (inner = 0; inner < heroCount; ++inner)
            heroPool[inner].load(infile, saveVersion);

        std::vector<type_artifact>& artifactPool = m_carryoverArtifact[pool];
        int artifactCount =
            static_cast<unsigned short>(readValue<short>(infile));
        artifactPool.resize(artifactCount);
        for (inner = 0; inner < artifactCount;
             ++inner) {
            artifactPool[inner].m_artifactId =
                TArtifact(readValue<short>(infile));
            artifactPool[inner].m_extra = readValue<short>(infile);
        }
    }

    readAssignedCampaignHeroes(infile, *this);
}

// The fixed pre-v28 record uses the old 0x462 hero layout. Retained 0x48ae30
// calls the obscuring-object base ctor and armyGroup ctor, zeros bitset<48>,
// then constructs 18 equipped and 64 backpack artifacts. That is exactly
// declaration-order base/member construction; there is no custom body.
// SCampaign::Load passes its address to the 16-element constructor iterator.
VA_COMPGEN(0x0048AE30, 0x5D, CLASS_CTOR, LegacyCampaignHero)

// Complete campaign serialization; no SCampaign::Save counterpart appears
// in the Dreamcast roster. Retail writes seven leading bytes, the campaign
// filename length/data, 21 completion flags, then counted score, carry-over,
// and assigned-carryover runs. Counts are re-read across their back edges.
// Hero counts are bytes; artifact counts/fields and assigned values use
// two bytes. The outer hero/artifact vectors share a 16-byte element stride.

// Retail keeps each score record and each inner pool vector in EDI across
// writes; repeated indexing of the outer vectors loses those lifetimes.
// Each serialized scalar has a separate lifetime, allowing byte/int/short
// buffers to reuse the dead outfile parameter home. The first byte still
// uses [ebp-1] while outfile is live there. Both artifact fields share one
// short buffer; retail's word loads prove narrowing before the writes.

// 2026-09-07: score reference alone 88.0368%, pool references alone 85.4136%,
// both 99.6062% (from 78.8074% MAX). Per-write scalar scopes with short word
// buffers reach 99.9518%; sharing the artifact word reaches 99.9632%; one
// outer counter across all three runs reaches 99.9858%. The canonical
// by-value writeValue<short> boundary for the two artifact fields restores
// retail's shared [ebp-0x10] parameter home and 0x10-byte frame. All 880 bytes,
// 38 CFG blocks, 16 branches, and 24 calls now agree.
// Controls: narrowing into int gives 97.1048% (movsx); also sharing the inner
// counters, scoping the hero loop or entire hero phase, hoisting the word
// to pool/function scope, and unsigned-short buffers are byte-flat at
// 99.9858%. Extra braces around the three runs were also byte-flat. The
// earlier shared-counter probe without the recovered references was
// 78.7965%; that result did not exclude the source reconstruction above.
VA(0x0048ae90, 0x370)  // link-order successor of LegacyCampaignHero's ctor; SCampaign::Load's mirror
void SCampaign::save(TAbstractFile* outfile)
{
    unsigned int index;

    {
        char charBuffer = m_isCheater;
        outfile->write(&charBuffer, sizeof(charBuffer));
    }
    {
        char flag = m_secretActive;
        outfile->write(&flag, sizeof(flag));
    }
    {
        char flag = m_currentMap;
        outfile->write(&flag, sizeof(flag));
    }
    {
        char flag = m_currentCampaign;
        outfile->write(&flag, sizeof(flag));
    }
    {
        char flag = m_numMapRegions;
        outfile->write(&flag, sizeof(flag));
    }
    {
        char flag = m_crossoverArrayIndex;
        outfile->write(&flag, sizeof(flag));
    }
    {
        char flag = m_briefingChoice;
        outfile->write(&flag, sizeof(flag));
    }

    {
        int intBuffer = m_campaignFilename.length();
        outfile->write(&intBuffer, sizeof(intBuffer));
    }
    outfile->write(m_campaignFilename.c_str(), m_campaignFilename.length());
    outfile->write(m_campaignCompleted, sizeof(m_campaignCompleted));

    {
        char flag = m_mapScores.size();
        outfile->write(&flag, sizeof(flag));
    }
    {
        for (index = 0; index < m_mapScores.size();
             ++index) {
            m_mapScores[index].write(outfile);
        }
    }

    {
        char flag = m_carryOverHeroes.size();
        outfile->write(&flag, sizeof(flag));
    }
    {
        for (index = 0; index < m_carryOverHeroes.size(); ++index) {
            std::vector<hero>& heroPool = m_carryOverHeroes[index];
            {
                char flag = heroPool.size();
                outfile->write(&flag, sizeof(flag));
            }

            for (unsigned int whichHero = 0;
                 whichHero < heroPool.size(); ++whichHero)
                heroPool[whichHero].save(outfile);

            std::vector<type_artifact>& artifactPool = m_carryoverArtifact[index];
            {
                int intBuffer = artifactPool.size();
                outfile->write(&intBuffer, 2);
            }
            for (unsigned int whichArtifact = 0;
                 whichArtifact < artifactPool.size(); ++whichArtifact) {
                writeValue<short>(
                    outfile, artifactPool[whichArtifact].m_artifactId);
                writeValue<short>(outfile, artifactPool[whichArtifact].m_extra);
            }
        }
    }

    {
        char flag = m_assignedCarryover.size();
        outfile->write(&flag, sizeof(flag));
    }
    {
        for (index = 0; index < m_assignedCarryover.size();
             ++index) {
            {
                short word = static_cast<short>(m_assignedCarryover[index]);
                outfile->write(&word, sizeof(word));
            }
        }
    }
}

// SCampaign::Load's retained Dinkumware helper cluster.  The element and
// nested-element types are independently fixed by the +0x5c/+0x3c/+0x4c
// member layouts and by the caller's 0x14/0x492/0x10/8-byte strides.
VA_COMPGEN(0x004013D0, 0x28, VECTOR_CONSTRUCTOR_ITERATOR, LegacyCampaignHero)
VA_COMPGEN(0x00404140, 0x03, VECTOR_DESTROY, type_artifact)
VA_COMPGEN(0x0048B440, 0x21, VECTOR_SIZE, CampaignScenarioInfo)
VA_COMPGEN(0x0048B470, 0x23, VECTOR_SIZE, hero)
VA_COMPGEN(0x004AF4E0, 0x13, VECTOR_SIZE, hero_vector)
VA_COMPGEN(0x0048C270, 0x285, VECTOR_INSERT, hero_vector)
VA_COMPGEN(0x0048C7A0, 0x285, VECTOR_INSERT, type_artifact_vector)
// ...and the SINGLE-element arm of the same two overload groups, 387 bytes
// each against the count arms' 645. Which element type each belongs to is
// read off its own named callees: 0x48c0e0 calls _Construct<vector<hero>> at
// 0x45fce0 and _Destroy<vector<vector<hero>>> at 0x48c5b0, 0x48c610 calls the
// type_artifact twins at 0x45fdc0 and 0x48caa0. Each group is now two claims
// against the object's two COMDATs, zipping by RVA against COFF order with
// the 387-byte arm first on both sides.
VA_COMPGEN(0x0048c0e0, 0x183, VECTOR_INSERT, hero_vector)
VA_COMPGEN(0x0048c610, 0x183, VECTOR_INSERT, type_artifact_vector)
VA_COMPGEN(0x0048CA30, 0x6A, VECTOR_ERASE, type_artifact_vector)
VA_COMPGEN(0x0048CAE0, 0x2E4, VECTOR_INSERT, CampaignScenarioInfo)
VA_COMPGEN(0x0048CDD0, 0x44, VECTOR_ERASE, CampaignScenarioInfo)
VA_COMPGEN(0x0048D060, 0x331, VECTOR_INSERT, hero)
VA_COMPGEN(0x0048D3A0, 0x6D, VECTOR_ERASE, hero)
VA_COMPGEN(0x0054D330, 0x246, VECTOR_INSERT, type_artifact)

VA(0x0048b200, 0x80)  // dc 0x7d374
int SCampaign::getScore() const
{
    int totalScore = 0;
    int numScores = 0;

    for (unsigned int i = 0; i < m_mapScores.size(); ++i) {
        if (m_mapScores[i].m_completed && m_mapScores[i].m_score >= 0) {
            totalScore += m_mapScores[i].m_score;
            ++numScores;
        }
    }

    if (!numScores)
        return 0;
    return ((totalScore + numScores / 2) / numScores) * 5;
}

VA(0x0048b280, 0x57)  // dc 0x7d3c0
int SCampaign::getTotalTime() const
{
    int totalTime = 0;

    for (unsigned int i = 0; i < m_mapScores.size(); ++i) {
        if (m_mapScores[i].m_completed)
            totalTime += m_mapScores[i].m_days;
    }

    return totalTime;
}

VA(0x0048b2e0, 0x8C)
void SCampaign::applyBriefingChoice(int option)
{
    m_briefingChoice = option;
    m_assignedCarryover.clear();
    if (m_currentCampaign == ALIGNMENT_CHOICE_CAMPAIGN_A
        && m_currentMap == ALIGNMENT_CHOICE_MAP)
        g_game->m_setup.m_alignment[2] = TOWN_INFERNO;
    if (m_currentCampaign == ALIGNMENT_CHOICE_CAMPAIGN_B
        && m_currentMap == ALIGNMENT_CHOICE_MAP) {
        switch (option) {
        case BRIEFING_CHOICE_FIRST:
            g_game->m_setup.m_alignment[2] = TOWN_INFERNO;
            break;
        case BRIEFING_CHOICE_SECOND:
            g_game->m_setup.m_alignment[2] = TOWN_DUNGEON;
            break;
        }
    }
}

VA_COMPGEN(0x0048C500, 0xA3, VECTOR_ERASE, hero_vector)

// COMDAT pairing: std::_Sort<hero, CrossoverHeroStronger>, agreement 0.972.
// The map hero placeholders' own sort, instantiated by
// ScenarioStruct::PlaceCrossoverHeroes. Both bodies are byte-identical to
// this compile's instruction stream.
VA_COMPGEN(0x0048eec0, 0x3ED, STD_SORT_0, HeroPlaceholderData_HeroPlaceholderStronger)
VA_COMPGEN(0x0048f630, 0x1AE, STD_SORT, HeroPlaceholderData_HeroPlaceholderStronger)

VA_COMPGEN(0x0048f7e0, 0x159, STD_SORT, hero_crossoverherostronger)

// COMDAT pairing: std::_Sort_0<hero, CrossoverHeroStronger>, agreement 0.985.
VA_COMPGEN(0x0048f2b0, 0x333, STD_SORT_0, hero_crossoverherostronger)

VA_COMPGEN(0x0048fa40, 0x1D7, STD_MEDIAN, hero_crossoverherostronger)
VA_COMPGEN(0x0048f940, 0xF4, STD_UNGUARDED_INSERT, hero_crossoverherostronger)
VA_COMPGEN(0x0048fc20, 0x195, STD_UNGUARDED_PARTITION, hero_crossoverherostronger)

// COMDAT pairing: bitset<145>::_Xran and bitset<8>::_Xran. Five byte-identical
// `_Xran` bodies survive in the image, so the discriminator is the BOUND
// COMPARE in each caller: 0x8d9a0's four callers all guard with `cmp <reg>,
// 0x91` (145) - among them game's claimed bitset<145>::set (0x4cfa60) and
// ::test (0x4cfad0) - and 0x8da70's eight with `cmp <reg>, 0x8`, among them
// game's claimed bitset<8>::set (0x4d4cc0) and ::test (0x4cfef0).
// 0x48edf0, which lane 16 claimed here as bitset145 on similarity alone,
// compares against 0x81 (129) at all three of its callers and is claimed in
// game.cpp. Recovered campaign readers now emit 129-bit members here too.
VA_COMPGEN(0x0048d9a0, 0xCB, BITSET_XRAN, Bitset145)
VA_COMPGEN(0x0048da70, 0xCB, BITSET_XRAN, Bitset8)

VA_COMPGEN(0x0048d440, 0x3E, VECTOR_UCOPY, hero)
VA_COMPGEN(0x0048d8d0, 0x38, VECTOR_UCOPY, type_artifact_vector)

// COMDAT pairing: hero::copy_backward, mnemonic agreement 0.918.
VA_COMPGEN(0x0048e880, 0x3B, STD_COPY_BACKWARD, hero)

// The adjacent hero fill walks the same 0x492-byte records forward and
// invokes hero::operator= once per element.  Its sole non-loop relocation
// and all 42 bytes identify the specialization independently of link order.
VA_COMPGEN(0x0048e850, 0x2A, STD_FILL, hero)

// COMDAT pairing: hero::_Ufill, mnemonic agreement 0.913.
VA_COMPGEN(0x0048d970, 0x2C, VECTOR_UFILL, hero)

// CatchableType's copyFunction selects this overload, not the string ctor.
VA_COMPGEN(0x00404700, 0x157, IMPLICIT_COPY_CTOR, out_of_range)

// COMDAT pairing: vector<vector<hero>>::_Destroy - reached from game and from
// two sites in this unit's own segment.
VA_COMPGEN(0x0048c5b0, 0x53, VECTOR_DESTROY, hero_vector)

// COMDAT pairing: vector<hero>::operator=. The caller set is the whole
// argument and it is unambiguous - SCampaign::PruneCrossoverHeroes, the
// already-claimed vector<vector<hero>>::insert and ::erase (which assign
// elements of exactly this type), and game's SCampaign::operator=. Mnemonic
// agreement is 1.000 against a 704 B object and `ret 4` matches the one
// reference argument.
VA_COMPGEN(0x0045ff30, 0x2C0, VECTOR_COPY_ASSIGN, hero)

VA_COMPGEN(0x00460850, 0x4B1, IMPLICIT_COPY_CTOR, hero)

VA_COMPGEN(0x004603a0, 0x355, STD_CONSTRUCT, hero)

// --- basic_filebuf<char> and the <fstream> COMDAT block --------------------

VA_COMPGEN(0x0048b4e0, 0x1E1, FILEBUF_OVERFLOW, char)
VA_COMPGEN(0x0048b6d0, 0x1B0, FILEBUF_PBACKFAIL, char)
VA_COMPGEN(0x0048b880, 0x30, FILEBUF_UNDERFLOW, char)
VA_COMPGEN(0x0048b8b0, 0x1E9, FILEBUF_UFLOW, char)

VA_COMPGEN(0x0048baa0, 0x86, FILEBUF_SEEKOFF, char)
VA_COMPGEN(0x0048bb30, 0x188, FILEBUF_SEEKPOS, char)

VA_COMPGEN(0x0048bcc0, 0x32, FILEBUF_SETBUF, char)
VA_COMPGEN(0x0048bd00, 0x1B, FILEBUF_SYNC, char)

// Slot 0 of the same vtable, which is what makes this one of the three
// indistinguishable 33-byte ??_G bodies in the span that CAN be named.
VA_COMPGEN(0x0048bd20, 0x21, SCALAR_DELETING_DTOR, basic_filebuf)

// COMDAT pairing: basic_streambuf<char>::_Init(), agreement 1.000 - the
// nullary base initializer, distinct from basic_stringbuf's three-argument
// _Init already claimed in resourcemanager.
VA_COMPGEN(0x0048bea0, 0x58, STREAMBUF_INIT, char)

// COMDAT pairing: vector<hero>::capacity and vector<hero>::_Destroy,
// agreements 0.933 and 1.000. Both are reached from the campaign's
// carry-over hero vector.
VA_COMPGEN(0x0048ce20, 0x23, VECTOR_CAPACITY, hero)
VA_COMPGEN(0x0048d410, 0x26, VECTOR_DESTROY, hero)

VA_COMPGEN(0x0048ce50, 0x210, VECTOR_INSERT, hero)

// COMDAT pairing: basic_filebuf<char>::_Init(FILE*, _Initfl), agreement
// 0.973 - the census had already named it `exe_filebuf_open` off the same
// vtable's construction path.
VA_COMPGEN(0x0048d4b0, 0xE2, FILEBUF_INIT, char)

VA_COMPGEN(0x0048dc10, 0x35, VECTOR_UCOPY, type_artifact)

// COMDAT pairing: locale::locale(const locale&), agreement 1.000.
// COMDAT pairing: locale's DEFAULT constructor, the other half of the ctor
// overload group whose second member is claimed below. Its body is
// locale::_Init() into `_Ptr` followed by the two nested _Lockit scopes that
// guard the shared _Locimp's saturating reference count - the copy
// constructor has neither call. COFF order in this object is (default,
// const&) and RVA order is 0x488e60, 0x48d800, so the group zips the same
// way in both directions.
VA_COMPGEN(0x00488e60, 0x4B, CLASS_CTOR, locale)

// ScenarioStruct's deleting wrapper at 0x488eb0 expands in this TU.
// The same native wrapper remains in campaignbrief, where its enrollment
// lives; its ordinary destructor at 0x485fe0 remains owned by this file.

VA_COMPGEN(0x0048d800, 0x19, CLASS_CTOR, locale)

VA_COMPGEN(0x0048d860, 0x38, VECTOR_UCOPY, hero_vector)
VA_COMPGEN(0x0048d8a0, 0x29, VECTOR_UFILL, hero_vector)
VA_COMPGEN(0x0048d910, 0x29, VECTOR_UFILL, type_artifact_vector)
VA_COMPGEN(0x0048dcc0, 0x285, STD_FILL, hero_vector)
VA_COMPGEN(0x0048df50, 0x2C9, STD_COPY_BACKWARD, hero_vector)
VA_COMPGEN(0x0048e4f0, 0x1A0, STD_FILL, type_artifact_vector)
VA_COMPGEN(0x0048e690, 0x1B1, STD_COPY_BACKWARD, type_artifact_vector)

VA_COMPGEN(0x0048eb60, 0x67, CLASS_CTOR, codecvt)
VA_COMPGEN(0x0048ec10, 0x18, CODECVT_DO_LENGTH, char)

// The two starting-options records' own vector helpers, all four proved by
// the call sites in their Read bodies. The crossover choice is a two-byte
// element and the hero choice an eight-byte one, which is why the hero
// option's _Ucopy folds onto vector<type_artifact>'s (same width) and only
// its _Ufill survives as its own address.
VA_COMPGEN(0x0048dba0, 0x31, VECTOR_UCOPY, TCampaignCrossoverChoice)
VA_COMPGEN(0x0048dbe0, 0x28, VECTOR_UFILL, TCampaignCrossoverChoice)
VA_COMPGEN(0x0048dc50, 0x2C, VECTOR_UFILL, TCampaignHeroChoice)
VA_COMPGEN(0x0048e9e0, 0xB, STD_CONSTRUCT, TCampaignCrossoverChoice)

// ScenarioStruct::read's prerequisite append retains single-element insert
// in retail: ret 8, returned iterator and byte-sized copies distinguish it
// from count insert (ret 12). The old 34.74% compared the wrong overload;
// the single-element claim remains unpaired while VC6 expands that wrapper.
// A 72-candidate batch compares push_back/single/count-one insertion, three
// actual Boolean lifetimes, scoped/function-scope size buffers, and four
// bitset constructor/proxy forms. Direct single insertion still does not
// retain the parent; count-one insertion expands it into the reader and
// naturally emits all three children below. Their 36/36/9-byte bodies match
// retail exactly with no other TU score loss (reader 51.56930%). Restoring
// push_back and direct bitset proxy assignment gives the best reader,
// 84.92324%, and inlines these unchanged library bodies; their MAX stays 100%.
VA_COMPGEN(0x0048bf00, 0x1AD, VECTOR_INSERT_SINGLE, unsigned_char)
VA_COMPGEN(0x0048db40, 0x24, VECTOR_UCOPY, unsigned_char)
// rmg_terrain emits a byte-identical _Ufill, but that body already represents
// its distinct retained 0x5b8060. Do not steal that enrollment or manufacture
// another copy to hide this consumer's remaining emission debt.
VA_COMPGEN(0x0048db70, 0x24, VECTOR_UFILL, unsigned_char)
VA_COMPGEN(0x0048e9d0, 0x09, STD_CONSTRUCT, unsigned_char)

// --- the <fstream> facet block, claimed 2026-09-06 -------------------------

// The destructor and the conversion setup, both named by their mnemonic
// agreement against the object's own free COMDATs (0.885 and 0.887, and no
// second candidate above 0.45 in either case):
VA_COMPGEN(0x0048bd50, 0x14F, IMPLICIT_DTOR, basic_filebuf)
VA_COMPGEN(0x0048d5a0, 0x22E, FILEBUF_INITCVT, char)

VA_COMPGEN(0x0048d820, 0x3B, STREAMBUF_GETLOC, char)

// std::copy over hero*, the forward twin of the copy_backward already
// claimed at 0x48e880: `while (first != last) *dest++ = *first++` with
// hero::operator= called and both pointers stepping by 0x492. The const and
// non-const source overloads compile to the same bytes and /OPT:ICF folded
// them, so one claim names the row and the other spelling is its alias.
VA_COMPGEN(0x0048dc80, 0x3B, STD_COPY, hero)

VA_COMPGEN(0x0048ece0, 0x37, BITSET_TEST, Bitset129)

VA_COMPGEN(0x0048ed20, 0x25, BITSET_TIDY, Bitset129)

VA_COMPGEN(0x0054c6f0, 0x39, VECTOR_ERASE, type_artifact)

VA_COMPGEN(0x0054ded0, 0x63, BITSET_SET, Bitset129)

// The facet's installation and teardown. _Addfac copies the locale, adds
// the codecvt to its facet vector and hands the locale back; _Tidyfac's
// static pair is the exit hook - _Save parks the facet in 0x696940 under a
// `_Lockit` and registers _Tidy with atexit (`push 0x48ed50 / call
// _atexit` at 0x48ecae is the link between the two rows).
VA_COMPGEN(0x0048e8c0, 0x102, LOCALE_ADDFAC_CODECVT, char)
VA_COMPGEN(0x0048ec60, 0x7B, TIDYFAC_CODECVT_SAVE, char)
VA_COMPGEN(0x0048ed50, 0x92, TIDYFAC_CODECVT_TIDY, char)

VA_COMPGEN(0x0048ebd0, 0x3, CODECVT_BASE_DO_ALWAYS_NOCONV, char)
VA_COMPGEN(0x0048ebe0, 0x6, CODECVT_BASE_DO_ENCODING, char)
VA_COMPGEN(0x0048ebf0, 0x1C, CODECVT_DO_IN, char)

// ...and the facet's scalar deleting destructor, slot 0 of its vftable.
VA_COMPGEN(0x0048ec30, 0x21, SCALAR_DELETING_DTOR, codecvt)
