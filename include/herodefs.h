#ifndef HOMM3_HERODEFS_H
#define HOMM3_HERODEFS_H

#include "creaturetype.h"
#include "armygrp.h"
#include "herospec.h"
#include "secondaryskill.h"
#include "sskilltraits.h"
#include "town_type.h"

// HeroDefs.h of the Loki port (RoE source): the hero, hero-class and
// secondary-skill traits HeroDefs.cpp fills from hotraits.txt, hctraits.txt
// and sstraits.txt. Enumerators and field names are the Dreamcast CodeView
// ones (THeroID 0x1d96, TSex 0x3df1, TRace 0x3df3, THeroClass 0x24e4,
// THeroTraits 0x3f76, THeroClassTraits 0x40e3).

// The 128 heroes of the RoE scenarios, then the two campaign heroes that
// only have biographies and portraits.
enum THeroID {
    eHeroNone = -1,
    eHeroGier = 0,
    eHeroRislav = 1,
    eHeroDesslock = 2,
    eHeroCuthbert = 3,
    eHeroLordHaart = 4,
    eHeroSorsha = 5,
    eHeroSirChristian = 6,
    eHeroTyris = 7,
    eHeroRion = 8,
    eHeroAdela = 9,
    eHeroHester = 10,
    eHeroGaldwyn = 11,
    eHeroIngham = 12,
    eHeroSanya = 13,
    eHeroLoynis = 14,
    eHeroEngle = 15,
    eHeroMephala = 16,
    eHeroUfretin = 17,
    eHeroAlagar = 18,
    eHeroRyland = 19,
    eHeroThorgrim = 20,
    eHeroIvor = 21,
    eHeroClancy = 22,
    eHeroElleshar = 23,
    eHeroCoronius = 24,
    eHeroClova = 25,
    eHeroKyrre = 26,
    eHeroGem = 27,
    eHeroKyriell = 28,
    eHeroMelodia = 29,
    eHeroJenova = 30,
    eHeroAeris = 31,
    eHeroPiquedram = 32,
    eHeroThane = 33,
    eHeroRugard = 34,
    eHeroNeelam = 35,
    eHeroTorosar = 36,
    eHeroFafner = 37,
    eHeroRissa = 38,
    eHeroKhem = 39,
    eHeroAstral = 40,
    eHeroHalon = 41,
    eHeroSerena = 42,
    eHeroDaremyth = 43,
    eHeroTheodorus = 44,
    eHeroSolmyr = 45,
    eHeroCyra = 46,
    eHeroAine = 47,
    eHeroFion = 48,
    eHeroAragorn = 49,
    eHeroMarius = 50,
    eHeroIgnatius = 51,
    eHeroXyron = 52,
    eHeroCalh = 53,
    eHeroPyre = 54,
    eHeroNymus = 55,
    eHeroAyden = 56,
    eHeroRashka = 57,
    eHeroAxsis = 58,
    eHeroOlema = 59,
    eHeroCalid = 60,
    eHeroAsh = 61,
    eHeroZydar = 62,
    eHeroXarfax = 63,
    eHeroStraker = 64,
    eHeroVokial = 65,
    eHeroMoandor = 66,
    eHeroCharna = 67,
    eHeroSkullreaver = 68,
    eHeroIsra = 69,
    eHeroClavius = 70,
    eHeroGalThran = 71,
    eHeroSeptienna = 72,
    eHeroAislinn = 73,
    eHeroSandro = 74,
    eHeroNimbus = 75,
    eHeroThant = 76,
    eHeroXsi = 77,
    eHeroViDomina = 78,
    eHeroNagash = 79,
    eHeroLorelei = 80,
    eHeroArlach = 81,
    eHeroDace = 82,
    eHeroAjit = 83,
    eHeroDamacon = 84,
    eHeroGunnar = 85,
    eHeroSynca = 86,
    eHeroShakti = 87,
    eHeroAlamar = 88,
    eHeroJaegar = 89,
    eHeroMalekith = 90,
    eHeroJeddite = 91,
    eHeroGeon = 92,
    eHeroDeemer = 93,
    eHeroSephinroth = 94,
    eHeroDarkstorn = 95,
    eHeroYog = 96,
    eHeroGurnisson = 97,
    eHeroJabarkas = 98,
    eHeroShiva = 99,
    eHeroGretchin = 100,
    eHeroKrellion = 101,
    eHeroCragHack = 102,
    eHeroTyraxor = 103,
    eHeroGird = 104,
    eHeroVey = 105,
    eHeroDessa = 106,
    eHeroTerek = 107,
    eHeroZubin = 108,
    eHeroGundula = 109,
    eHeroWystan = 110,
    eHeroSaurug = 111,
    eHeroBron = 112,
    eHeroDrakon = 113,
    eHeroOris = 114,
    eHeroTazar = 115,
    eHeroAlkin = 116,
    eHeroKorbac = 117,
    eHeroGerwulf = 118,
    eHeroBroghild = 119,
    eHeroMirlanda = 120,
    eHeroRosic = 121,
    eHeroVoy = 122,
    eHeroVerdish = 123,
    eHeroMerist = 124,
    eHeroStyg = 125,
    eHeroAndra = 126,
    eHeroTiva = 127,
    kNumHeroes = 128,
    eHeroCatherine = 128,
    eHeroGeneralKendal = 129,
    kNumHeroBios = 130
};

enum TSex {
    eSexMale = 0,
    eSexFemale = 1
};

enum TRace {
    eRaceDemon = 0,
    eRaceDwarf = 1,
    eRaceEfreet = 2,
    eRaceElf = 3,
    eRaceGenie = 4,
    eRaceGnoll = 5,
    eRaceGoblin = 6,
    eRaceHuman = 7,
    eRaceLich = 8,
    eRaceLizardman = 9,
    eRaceMinotaur = 10,
    eRaceOgre = 11,
    eRaceTroglodyte = 12,
    eRaceVampire = 13
};

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
    kNumHeroClasses = 16
};

// 88 bytes. The fields up to m_large_portrait_name are compiled in; the
// name and the three starting-stack ranges come from hotraits.txt.
struct THeroTraits {
    TSex m_sex;
    TRace m_race;
    THeroClass m_class;
    TSecondarySkill m_1stSkill;
    TSkillMastery m_1stSkillLevel;
    TSecondarySkill m_2ndSkill;
    TSkillMastery m_2ndSkillLevel;
    bool m_startsWithSpellbook;
    SpellID m_startingSpell;
    TCreatureType m_1stStack;
    TCreatureType m_2ndStack;
    TCreatureType m_3rdStack;
    const char* m_small_portrait_name;
    const char* m_large_portrait_name;
    unsigned int attributes;
    const char* m_name;
    int m_1stStackLow;
    int m_1stStackHigh;
    int m_2ndStackLow;
    int m_2ndStackHigh;
    int m_3rdStackLow;
    int m_3rdStackHigh;
};

// 60 bytes; m_townType is compiled in, the rest comes from hctraits.txt.
struct THeroClassTraits {
    TTownType m_townType;
    const char* m_name;
    float m_aggression;
    char m_initialPrimarySkill[4];
    char m_gainPrimarySkillChance[4];
    char m_gainPrimarySkillChance10P[4];
    char m_gainSecondarySkillChance[kNumSecSkills];
    char m_foundInTownType[kNumTownTypes];
};

extern const THeroTraits (&akHeroTraits)[kNumHeroBios];
extern const THeroClassTraits (&akHeroClassTraits)[kNumHeroClasses];

bool InitializeHeroTraitsTable();
bool InitializeHeroClassTraitsTable();
bool InitializeSSkillTraitsTable();

#endif  /* HOMM3_HERODEFS_H */
