// HeroDefs.cpp of the Loki port (Loki object 16): the RoE hero, hero-class
// and secondary-skill traits tables and their spreadsheet loaders.
#include "herodefs.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "resourcemanager.h"
#include "textresource.h"

// Sex, race, class, starting skills, spell and stacks and the portraits are
// compiled in; hotraits.txt supplies the names and the stack ranges.
static THeroTraits aHeroTraitsImp[kNumHeroBios] = {
    { eSexMale, eRaceHuman, eClassKnight, eSecSkillLeadership, eMasteryBasic, eSecSkillArchery, eMasteryBasic, false, eSpellNone, eCreaturePikeman, eCreatureLightCrossbowman, eCreatureGriffin, "HPS000Kn.PCX", "HPL000Kn.PCX" },
    { eSexFemale, eRaceHuman, eClassKnight, eSecSkillLeadership, eMasteryBasic, eSecSkillArchery, eMasteryBasic, false, eSpellNone, eCreatureLightCrossbowman, eCreatureLightCrossbowman, eCreatureLightCrossbowman, "HPS001Kn.PCX", "HPL001Kn.PCX" },
    { eSexMale, eRaceHuman, eClassKnight, eSecSkillLeadership, eMasteryBasic, eSecSkillDefense, eMasteryBasic, false, eSpellNone, eCreaturePikeman, eCreatureGriffin, eCreatureGriffin, "HPS002Kn.PCX", "HPL002Kn.PCX" },
    { eSexFemale, eRaceHuman, eClassKnight, eSecSkillLeadership, eMasteryBasic, eSecSkillNavigation, eMasteryBasic, false, eSpellNone, eCreaturePikeman, eCreatureLightCrossbowman, eCreatureGriffin, "HPS003Kn.PCX", "HPL003Kn.PCX" },
    { eSexMale, eRaceHuman, eClassKnight, eSecSkillLeadership, eMasteryBasic, eSecSkillEstates, eMasteryBasic, false, eSpellNone, eCreaturePikeman, eCreatureLightCrossbowman, eCreatureGriffin, "HPS004Kn.PCX", "HPL004Kn.PCX" },
    { eSexFemale, eRaceHuman, eClassKnight, eSecSkillLeadership, eMasteryBasic, eSecSkillOffense, eMasteryBasic, false, eSpellNone, eCreaturePikeman, eCreatureLightCrossbowman, eCreatureGriffin, "HPS005Kn.PCX", "HPL005Kn.PCX" },
    { eSexMale, eRaceHuman, eClassKnight, eSecSkillLeadership, eMasteryBasic, eSecSkillBattlefieldBallistics, eMasteryBasic, false, eSpellNone, eCreaturePikeman, eCreatureBallista, eCreatureGriffin, "HPS006Kn.PCX", "HPL006Kn.PCX" },
    { eSexFemale, eRaceHuman, eClassKnight, eSecSkillLeadership, eMasteryBasic, eSecSkillBattleTactics, eMasteryBasic, false, eSpellNone, eCreaturePikeman, eCreatureLightCrossbowman, eCreatureGriffin, "HPS007Kn.PCX", "HPL007Kn.PCX" },
    { eSexMale, eRaceHuman, eClassCleric, eSecSkillWisdom, eMasteryBasic, eSecSkillFirstAid, eMasteryBasic, true, eSpellToughSkin, eCreaturePikeman, eCreatureFirstAidTent, eCreatureGriffin, "HPS008Cl.PCX", "HPL008Cl.PCX" },
    { eSexFemale, eRaceHuman, eClassCleric, eSecSkillWisdom, eMasteryBasic, eSecSkillDiplomacy, eMasteryBasic, true, eSpellBless, eCreaturePikeman, eCreatureLightCrossbowman, eCreatureGriffin, "HPS009Cl.PCX", "HPL009Cl.PCX" },
    { eSexMale, eRaceHuman, eClassCleric, eSecSkillWisdom, eMasteryBasic, eSecSkillEstates, eMasteryBasic, true, eSpellWeakness, eCreaturePikeman, eCreatureLightCrossbowman, eCreatureGriffin, "HPS010Cl.PCX", "HPL010Cl.PCX" },
    { eSexFemale, eRaceHuman, eClassCleric, eSecSkillWisdom, eMasteryAdvanced, eSecSkillNone, eMasteryNone, true, eSpellFrostRing, eCreaturePikeman, eCreatureLightCrossbowman, eCreatureGriffin, "HPS011Cl.PCX", "HPL011Cl.PCX" },
    { eSexMale, eRaceHuman, eClassCleric, eSecSkillWisdom, eMasteryBasic, eSecSkillMysticism, eMasteryBasic, true, eSpellCurse, eCreaturePikeman, eCreatureLightCrossbowman, eCreatureGriffin, "HPS012Cl.PCX", "HPL012Cl.PCX" },
    { eSexFemale, eRaceHuman, eClassCleric, eSecSkillWisdom, eMasteryBasic, eSecSkillEagleEye, eMasteryBasic, true, eSpellDispel, eCreaturePikeman, eCreatureLightCrossbowman, eCreatureGriffin, "HPS013Cl.PCX", "HPL013Cl.PCX" },
    { eSexMale, eRaceHuman, eClassCleric, eSecSkillWisdom, eMasteryBasic, eSecSkillLearning, eMasteryBasic, true, eSpellPrayer, eCreaturePikeman, eCreatureLightCrossbowman, eCreatureGriffin, "HPS014Cl.PCX", "HPL014Cl.PCX" },
    { eSexFemale, eRaceHuman, eClassCleric, eSecSkillWisdom, eMasteryBasic, eSecSkillIntelligence, eMasteryBasic, true, eSpellCure, eCreaturePikeman, eCreatureLightCrossbowman, eCreatureGriffin, "HPS015Cl.PCX", "HPL015Cl.PCX" },
    { eSexFemale, eRaceHuman, eClassRanger, eSecSkillLeadership, eMasteryBasic, eSecSkillDefense, eMasteryBasic, false, eSpellNone, eCreatureCentaur, eCreatureDwarf, eCreatureWoodElf, "HPS016Rn.PCX", "HPL016Rn.PCX" },
    { eSexMale, eRaceDwarf, eClassRanger, eSecSkillMagicResistance, eMasteryBasic, eSecSkillLuck, eMasteryBasic, false, eSpellNone, eCreatureDwarf, eCreatureDwarf, eCreatureDwarf, "HPS017Rn.PCX", "HPL017Rn.PCX" },
    { eSexMale, eRaceElf, eClassRanger, eSecSkillArchery, eMasteryAdvanced, eSecSkillNone, eMasteryNone, false, eSpellNone, eCreatureCentaur, eCreatureDwarf, eCreatureWoodElf, "HPS018Rn.PCX", "HPL018Rn.PCX" },
    { eSexMale, eRaceHuman, eClassRanger, eSecSkillLeadership, eMasteryBasic, eSecSkillDiplomacy, eMasteryBasic, false, eSpellNone, eCreatureCentaur, eCreatureDwarf, eCreatureWoodElf, "HPS019Rn.PCX", "HPL019Rn.PCX" },
    { eSexMale, eRaceDwarf, eClassRanger, eSecSkillMagicResistance, eMasteryAdvanced, eSecSkillNone, eMasteryNone, false, eSpellNone, eCreatureCentaur, eCreatureDwarf, eCreatureWoodElf, "HPS020Rn.PCX", "HPL020Rn.PCX" },
    { eSexMale, eRaceElf, eClassRanger, eSecSkillArchery, eMasteryBasic, eSecSkillOffense, eMasteryBasic, false, eSpellNone, eCreatureCentaur, eCreatureWoodElf, eCreatureWoodElf, "HPS021Rn.PCX", "HPL021Rn.PCX" },
    { eSexMale, eRaceDwarf, eClassRanger, eSecSkillMagicResistance, eMasteryBasic, eSecSkillPathfinding, eMasteryBasic, false, eSpellNone, eCreatureCentaur, eCreatureDwarf, eCreatureWoodElf, "HPS022Rn.PCX", "HPL022Rn.PCX" },
    { eSexFemale, eRaceElf, eClassRanger, eSecSkillArchery, eMasteryBasic, eSecSkillLogistics, eMasteryBasic, false, eSpellNone, eCreatureCentaur, eCreatureDwarf, eCreatureWoodElf, "HPS023Rn.PCX", "HPL023Rn.PCX" },
    { eSexMale, eRaceHuman, eClassDruid, eSecSkillWisdom, eMasteryBasic, eSecSkillMagicScholar, eMasteryBasic, true, eSpellSlayer, eCreatureCentaur, eCreatureDwarf, eCreatureWoodElf, "HPS024Dr.PCX", "HPL024Dr.PCX" },
    { eSexMale, eRaceDwarf, eClassDruid, eSecSkillWisdom, eMasteryAdvanced, eSecSkillSiegeBallistics, eMasteryBasic, true, eSpellCure, eCreatureCentaur, eCreatureDwarf, eCreatureWoodElf, "HPS025Dr.PCX", "HPL025Dr.PCX" },
    { eSexMale, eRaceElf, eClassDruid, eSecSkillWisdom, eMasteryBasic, eSecSkillIntelligence, eMasteryBasic, true, eSpellCurse, eCreatureCentaur, eCreatureDwarf, eCreatureWoodElf, "HPS026Dr.PCX", "HPL026Dr.PCX" },
    { eSexFemale, eRaceHuman, eClassDruid, eSecSkillWisdom, eMasteryBasic, eSecSkillFirstAid, eMasteryBasic, true, eSpellSummonBoat, eCreatureCentaur, eCreatureFirstAidTent, eCreatureWoodElf, "HPS027Dr.PCX", "HPL027Dr.PCX" },
    { eSexMale, eRaceDwarf, eClassDruid, eSecSkillWisdom, eMasteryBasic, eSecSkillEagleEye, eMasteryBasic, true, eSpellMagicBolt, eCreatureCentaur, eCreatureDwarf, eCreatureWoodElf, "HPS028Dr.PCX", "HPL028Dr.PCX" },
    { eSexFemale, eRaceElf, eClassDruid, eSecSkillWisdom, eMasteryBasic, eSecSkillLuck, eMasteryBasic, true, eSpellFortune, eCreatureCentaur, eCreatureDwarf, eCreatureWoodElf, "HPS029Dr.PCX", "HPL029Dr.PCX" },
    { eSexMale, eRaceHuman, eClassDruid, eSecSkillWisdom, eMasteryBasic, eSecSkillSorcery, eMasteryBasic, true, eSpellIceRay, eCreatureCentaur, eCreatureDwarf, eCreatureWoodElf, "HPS030Dr.PCX", "HPL030Dr.PCX" },
    { eSexMale, eRaceElf, eClassDruid, eSecSkillWisdom, eMasteryBasic, eSecSkillScouting, eMasteryBasic, true, eSpellProtectionFromAir, eCreatureCentaur, eCreatureDwarf, eCreatureWoodElf, "HPS031Dr.PCX", "HPL031Dr.PCX" },
    { eSexMale, eRaceHuman, eClassAlchemist, eSecSkillMysticism, eMasteryBasic, eSecSkillScouting, eMasteryBasic, true, eSpellShield, eCreatureStoneGargoyle, eCreatureStoneGargoyle, eCreatureStoneGargoyle, "HPS032Al.PCX", "HPL032Al.PCX" },
    { eSexMale, eRaceGenie, eClassAlchemist, eSecSkillMagicScholar, eMasteryAdvanced, eSecSkillNone, eMasteryNone, true, eSpellMagicBolt, eCreatureApprenticeGremlin, eCreatureStoneGargoyle, eCreatureStoneGolem, "HPS033Al.PCX", "HPL033Al.PCX" },
    { eSexFemale, eRaceHuman, eClassAlchemist, eSecSkillMysticism, eMasteryBasic, eSecSkillSorcery, eMasteryBasic, true, eSpellTailWind, eCreatureApprenticeGremlin, eCreatureStoneGolem, eCreatureStoneGolem, "HPS034Al.PCX", "HPL034Al.PCX" },
    { eSexFemale, eRaceGenie, eClassAlchemist, eSecSkillMagicScholar, eMasteryBasic, eSecSkillDefense, eMasteryBasic, true, eSpellShield, eCreatureApprenticeGremlin, eCreatureStoneGargoyle, eCreatureStoneGolem, "HPS035Al.PCX", "HPL035Al.PCX" },
    { eSexMale, eRaceHuman, eClassAlchemist, eSecSkillMysticism, eMasteryBasic, eSecSkillBattleTactics, eMasteryBasic, true, eSpellMagicBolt, eCreatureApprenticeGremlin, eCreatureBallista, eCreatureStoneGolem, "HPS036Al.PCX", "HPL036Al.PCX" },
    { eSexMale, eRaceGenie, eClassAlchemist, eSecSkillMagicScholar, eMasteryBasic, eSecSkillMagicResistance, eMasteryBasic, true, eSpellTailWind, eCreatureApprenticeGremlin, eCreatureStoneGargoyle, eCreatureStoneGolem, "HPS037Al.PCX", "HPL037Al.PCX" },
    { eSexFemale, eRaceHuman, eClassAlchemist, eSecSkillMysticism, eMasteryBasic, eSecSkillOffense, eMasteryBasic, true, eSpellMagicBolt, eCreatureApprenticeGremlin, eCreatureStoneGargoyle, eCreatureStoneGolem, "HPS038Al.PCX", "HPL038Al.PCX" },
    { eSexFemale, eRaceGenie, eClassAlchemist, eSecSkillMagicScholar, eMasteryBasic, eSecSkillIntelligence, eMasteryBasic, true, eSpellMagicBolt, eCreatureApprenticeGremlin, eCreatureStoneGargoyle, eCreatureStoneGolem, "HPS039Al.PCX", "HPL039Al.PCX" },
    { eSexMale, eRaceHuman, eClassWizard, eSecSkillWisdom, eMasteryAdvanced, eSecSkillNone, eMasteryNone, true, eSpellHypnotize, eCreatureApprenticeGremlin, eCreatureStoneGargoyle, eCreatureStoneGolem, "HPS040Wz.PCX", "HPL040Wz.PCX" },
    { eSexMale, eRaceGenie, eClassWizard, eSecSkillWisdom, eMasteryBasic, eSecSkillMysticism, eMasteryBasic, true, eSpellToughSkin, eCreatureApprenticeGremlin, eCreatureStoneGargoyle, eCreatureStoneGolem, "HPS041Wz.PCX", "HPL041Wz.PCX" },
    { eSexFemale, eRaceHuman, eClassWizard, eSecSkillWisdom, eMasteryBasic, eSecSkillEagleEye, eMasteryBasic, true, eSpellDispel, eCreatureApprenticeGremlin, eCreatureStoneGargoyle, eCreatureStoneGolem, "HPS042Wz.PCX", "HPL042Wz.PCX" },
    { eSexFemale, eRaceGenie, eClassWizard, eSecSkillWisdom, eMasteryBasic, eSecSkillIntelligence, eMasteryBasic, true, eSpellFortune, eCreatureApprenticeGremlin, eCreatureStoneGargoyle, eCreatureStoneGolem, "HPS043Wz.PCX", "HPL043Wz.PCX" },
    { eSexMale, eRaceHuman, eClassWizard, eSecSkillWisdom, eMasteryBasic, eSecSkillSiegeBallistics, eMasteryBasic, true, eSpellShield, eCreatureApprenticeGremlin, eCreatureStoneGargoyle, eCreatureStoneGolem, "HPS044Wz.PCX", "HPL044Wz.PCX" },
    { eSexMale, eRaceGenie, eClassWizard, eSecSkillWisdom, eMasteryBasic, eSecSkillSorcery, eMasteryBasic, true, eSpellChainLightning, eCreatureApprenticeGremlin, eCreatureStoneGargoyle, eCreatureStoneGolem, "HPS045Wz.PCX", "HPL045Wz.PCX" },
    { eSexFemale, eRaceHuman, eClassWizard, eSecSkillWisdom, eMasteryBasic, eSecSkillDiplomacy, eMasteryBasic, true, eSpellTailWind, eCreatureApprenticeGremlin, eCreatureStoneGargoyle, eCreatureStoneGolem, "HPS046Wz.PCX", "HPL046Wz.PCX" },
    { eSexFemale, eRaceGenie, eClassWizard, eSecSkillWisdom, eMasteryBasic, eSecSkillMagicScholar, eMasteryBasic, true, eSpellCurse, eCreatureApprenticeGremlin, eCreatureStoneGargoyle, eCreatureStoneGolem, "HPS047Wz.PCX", "HPL047Wz.PCX" },
    { eSexFemale, eRaceHuman, eClassPagan, eSecSkillScouting, eMasteryAdvanced, eSecSkillNone, eMasteryNone, false, eSpellNone, eCreatureImp, eCreatureHellHound, eCreatureHellHound, "HPS048Hr.PCX", "HPL048Hr.PCX" },
    { eSexMale, eRaceEfreet, eClassPagan, eSecSkillMagicScholar, eMasteryBasic, eSecSkillWisdom, eMasteryBasic, false, eSpellNone, eCreatureImp, eCreatureGog, eCreatureHellHound, "HPS049Hr.PCX", "HPL049Hr.PCX" },
    { eSexFemale, eRaceDemon, eClassPagan, eSecSkillDefense, eMasteryAdvanced, eSecSkillNone, eMasteryNone, false, eSpellNone, eCreatureImp, eCreatureGog, eCreatureHellHound, "HPS050Hr.PCX", "HPL050Hr.PCX" },
    { eSexMale, eRaceHuman, eClassPagan, eSecSkillBattleTactics, eMasteryBasic, eSecSkillMagicResistance, eMasteryBasic, false, eSpellNone, eCreatureImp, eCreatureImp, eCreatureImp, "HPS051Hr.PCX", "HPL051Hr.PCX" },
    { eSexFemale, eRaceEfreet, eClassPagan, eSecSkillMagicScholar, eMasteryBasic, eSecSkillOffense, eMasteryBasic, false, eSpellNone, eCreatureImp, eCreatureGog, eCreatureHellHound, "HPS052Hr.PCX", "HPL052Hr.PCX" },
    { eSexMale, eRaceDemon, eClassPagan, eSecSkillArchery, eMasteryBasic, eSecSkillScouting, eMasteryBasic, false, eSpellNone, eCreatureGog, eCreatureGog, eCreatureGog, "HPS053Hr.PCX", "HPL053Hr.PCX" },
    { eSexFemale, eRaceHuman, eClassPagan, eSecSkillLogistics, eMasteryBasic, eSecSkillBattlefieldBallistics, eMasteryBasic, false, eSpellNone, eCreatureImp, eCreatureBallista, eCreatureHellHound, "HPS054Hr.PCX", "HPL054Hr.PCX" },
    { eSexFemale, eRaceDemon, eClassPagan, eSecSkillOffense, eMasteryAdvanced, eSecSkillNone, eMasteryNone, false, eSpellNone, eCreatureImp, eCreatureGog, eCreatureHellHound, "HPS055Hr.PCX", "HPL055Hr.PCX" },
    { eSexMale, eRaceHuman, eClassHeretic, eSecSkillWisdom, eMasteryBasic, eSecSkillIntelligence, eMasteryBasic, true, eSpellViewEarth, eCreatureImp, eCreatureGog, eCreatureHellHound, "HPS056Dm.PCX", "HPL056Dm.PCX" },
    { eSexMale, eRaceEfreet, eClassHeretic, eSecSkillWisdom, eMasteryBasic, eSecSkillMagicScholar, eMasteryBasic, true, eSpellFireblast, eCreatureImp, eCreatureGog, eCreatureHellHound, "HPS057Dm.PCX", "HPL057Dm.PCX" },
    { eSexMale, eRaceDemon, eClassHeretic, eSecSkillWisdom, eMasteryBasic, eSecSkillMysticism, eMasteryBasic, true, eSpellProtectionFromAir, eCreatureImp, eCreatureGog, eCreatureHellHound, "HPS058Dm.PCX", "HPL058Dm.PCX" },
    { eSexFemale, eRaceHuman, eClassHeretic, eSecSkillWisdom, eMasteryBasic, eSecSkillSiegeBallistics, eMasteryBasic, true, eSpellWeakness, eCreatureImp, eCreatureGog, eCreatureHellHound, "HPS059Dm.PCX", "HPL059Dm.PCX" },
    { eSexFemale, eRaceDemon, eClassHeretic, eSecSkillWisdom, eMasteryBasic, eSecSkillLearning, eMasteryBasic, true, eSpellTailWind, eCreatureImp, eCreatureGog, eCreatureHellHound, "HPS060Dm.PCX", "HPL060Dm.PCX" },
    { eSexFemale, eRaceDemon, eClassHeretic, eSecSkillWisdom, eMasteryBasic, eSecSkillEagleEye, eMasteryBasic, true, eSpellBloodLust, eCreatureImp, eCreatureGog, eCreatureHellHound, "HPS061Dm.PCX", "HPL061Dm.PCX" },
    { eSexMale, eRaceEfreet, eClassHeretic, eSecSkillWisdom, eMasteryBasic, eSecSkillSorcery, eMasteryBasic, true, eSpellToughSkin, eCreatureImp, eCreatureGog, eCreatureHellHound, "HPS062Dm.PCX", "HPL062Dm.PCX" },
    { eSexMale, eRaceHuman, eClassHeretic, eSecSkillWisdom, eMasteryBasic, eSecSkillLeadership, eMasteryBasic, true, eSpellSpontaneousCombustion, eCreatureImp, eCreatureGog, eCreatureHellHound, "HPS063Dm.PCX", "HPL063Dm.PCX" },
    { eSexMale, eRaceHuman, eClassDeathKnight, eSecSkillNecromancy, eMasteryBasic, eSecSkillMagicResistance, eMasteryBasic, true, eSpellTailWind, eCreatureZombie, eCreatureZombie, eCreatureZombie, "HPS064Dk.PCX", "HPL064Dk.PCX" },
    { eSexMale, eRaceVampire, eClassDeathKnight, eSecSkillNecromancy, eMasteryBasic, eSecSkillBattlefieldBallistics, eMasteryBasic, true, eSpellToughSkin, eCreatureSkeleton, eCreatureZombie, eCreatureWight, "HPS065Dk.PCX", "HPL065Dk.PCX" },
    { eSexMale, eRaceLich, eClassDeathKnight, eSecSkillNecromancy, eMasteryBasic, eSecSkillLearning, eMasteryBasic, true, eSpellMuckAndMire, eCreatureSkeleton, eCreatureZombie, eCreatureWight, "HPS066Dk.PCX", "HPL066Dk.PCX" },
    { eSexFemale, eRaceHuman, eClassDeathKnight, eSecSkillNecromancy, eMasteryBasic, eSecSkillBattleTactics, eMasteryBasic, true, eSpellMagicBolt, eCreatureSkeleton, eCreatureWight, eCreatureWight, "HPS067Dk.PCX", "HPL067Dk.PCX" },
    { eSexFemale, eRaceVampire, eClassDeathKnight, eSecSkillNecromancy, eMasteryBasic, eSecSkillOffense, eMasteryBasic, true, eSpellMagicBolt, eCreatureSkeleton, eCreatureZombie, eCreatureWight, "HPS068Dk.PCX", "HPL068Dk.PCX" },
    { eSexFemale, eRaceLich, eClassDeathKnight, eSecSkillNecromancy, eMasteryAdvanced, eSecSkillNone, eMasteryBasic, true, eSpellMagicBolt, eCreatureSkeleton, eCreatureZombie, eCreatureWight, "HPS069Dk.PCX", "HPL069Dk.PCX" },
    { eSexMale, eRaceHuman, eClassDeathKnight, eSecSkillNecromancy, eMasteryBasic, eSecSkillOffense, eMasteryBasic, true, eSpellMagicBolt, eCreatureSkeleton, eCreatureZombie, eCreatureWight, "HPS070Dk.PCX", "HPL070Dk.PCX" },
    { eSexMale, eRaceVampire, eClassDeathKnight, eSecSkillNecromancy, eMasteryBasic, eSecSkillDefense, eMasteryBasic, true, eSpellShield, eCreatureSkeleton, eCreatureSkeleton, eCreatureSkeleton, "HPS071Dk.PCX", "HPL071Dk.PCX" },
    { eSexFemale, eRaceHuman, eClassNecromancer, eSecSkillNecromancy, eMasteryBasic, eSecSkillMagicScholar, eMasteryBasic, true, eSpellDeathRipple, eCreatureSkeleton, eCreatureZombie, eCreatureWight, "HPS072Nc.PCX", "HPL072Nc.PCX" },
    { eSexFemale, eRaceVampire, eClassNecromancer, eSecSkillNecromancy, eMasteryBasic, eSecSkillWisdom, eMasteryBasic, true, eSpellMeteorShower, eCreatureSkeleton, eCreatureZombie, eCreatureWight, "HPS073Nc.PCX", "HPL073Nc.PCX" },
    { eSexMale, eRaceLich, eClassNecromancer, eSecSkillNecromancy, eMasteryBasic, eSecSkillSorcery, eMasteryBasic, true, eSpellMuckAndMire, eCreatureSkeleton, eCreatureZombie, eCreatureWight, "HPS074Nc.PCX", "HPL074Nc.PCX" },
    { eSexMale, eRaceHuman, eClassNecromancer, eSecSkillNecromancy, eMasteryBasic, eSecSkillEagleEye, eMasteryBasic, true, eSpellShield, eCreatureSkeleton, eCreatureZombie, eCreatureWight, "HPS075Nc.PCX", "HPL075Nc.PCX" },
    { eSexMale, eRaceVampire, eClassNecromancer, eSecSkillNecromancy, eMasteryBasic, eSecSkillMysticism, eMasteryBasic, true, eSpellAnimateDead, eCreatureSkeleton, eCreatureZombie, eCreatureWight, "HPS076Nc.PCX", "HPL076Nc.PCX" },
    { eSexFemale, eRaceLich, eClassNecromancer, eSecSkillNecromancy, eMasteryBasic, eSecSkillLearning, eMasteryBasic, true, eSpellToughSkin, eCreatureSkeleton, eCreatureZombie, eCreatureWight, "HPS077Nc.PCX", "HPL077Nc.PCX" },
    { eSexFemale, eRaceHuman, eClassNecromancer, eSecSkillNecromancy, eMasteryAdvanced, eSecSkillNone, eMasteryNone, true, eSpellCurse, eCreatureSkeleton, eCreatureZombie, eCreatureWight, "HPS078Nc.PCX", "HPL078Nc.PCX" },
    { eSexMale, eRaceLich, eClassNecromancer, eSecSkillNecromancy, eMasteryBasic, eSecSkillIntelligence, eMasteryBasic, true, eSpellProtectionFromAir, eCreatureSkeleton, eCreatureZombie, eCreatureWight, "HPS079Nc.PCX", "HPL079Nc.PCX" },
    { eSexFemale, eRaceHuman, eClassOverlord, eSecSkillLeadership, eMasteryBasic, eSecSkillScouting, eMasteryBasic, false, eSpellNone, eCreatureHarpy, eCreatureHarpy, eCreatureHarpy, "HPS080Ov.PCX", "HPL080Ov.PCX" },
    { eSexMale, eRaceTroglodyte, eClassOverlord, eSecSkillOffense, eMasteryBasic, eSecSkillBattlefieldBallistics, eMasteryBasic, false, eSpellNone, eCreatureTroglodyte, eCreatureBallista, eCreatureBeholder, "HPS081Ov.PCX", "HPL081Ov.PCX" },
    { eSexMale, eRaceMinotaur, eClassOverlord, eSecSkillBattleTactics, eMasteryBasic, eSecSkillOffense, eMasteryBasic, false, eSpellNone, eCreatureTroglodyte, eCreatureHarpy, eCreatureBeholder, "HPS082Ov.PCX", "HPL082Ov.PCX" },
    { eSexMale, eRaceHuman, eClassOverlord, eSecSkillLeadership, eMasteryBasic, eSecSkillMagicResistance, eMasteryBasic, false, eSpellNone, eCreatureTroglodyte, eCreatureBeholder, eCreatureBeholder, "HPS083Ov.PCX", "HPL083Ov.PCX" },
    { eSexMale, eRaceTroglodyte, eClassOverlord, eSecSkillOffense, eMasteryAdvanced, eSecSkillNone, eMasteryNone, false, eSpellNone, eCreatureTroglodyte, eCreatureHarpy, eCreatureBeholder, "HPS084Ov.PCX", "HPL084Ov.PCX" },
    { eSexMale, eRaceMinotaur, eClassOverlord, eSecSkillBattleTactics, eMasteryBasic, eSecSkillLogistics, eMasteryBasic, false, eSpellNone, eCreatureTroglodyte, eCreatureHarpy, eCreatureBeholder, "HPS085Ov.PCX", "HPL085Ov.PCX" },
    { eSexFemale, eRaceHuman, eClassOverlord, eSecSkillLeadership, eMasteryBasic, eSecSkillMagicScholar, eMasteryBasic, false, eSpellNone, eCreatureTroglodyte, eCreatureHarpy, eCreatureBeholder, "HPS086Ov.PCX", "HPL086Ov.PCX" },
    { eSexMale, eRaceTroglodyte, eClassOverlord, eSecSkillOffense, eMasteryBasic, eSecSkillBattleTactics, eMasteryBasic, false, eSpellNone, eCreatureTroglodyte, eCreatureTroglodyte, eCreatureTroglodyte, "HPS087Ov.PCX", "HPL087Ov.PCX" },
    { eSexMale, eRaceHuman, eClassWarlock, eSecSkillWisdom, eMasteryBasic, eSecSkillMagicScholar, eMasteryBasic, true, eSpellResurrection, eCreatureTroglodyte, eCreatureHarpy, eCreatureBeholder, "HPS088Wl.PCX", "HPL088Wl.PCX" },
    { eSexMale, eRaceTroglodyte, eClassWarlock, eSecSkillWisdom, eMasteryBasic, eSecSkillMysticism, eMasteryBasic, true, eSpellShield, eCreatureTroglodyte, eCreatureHarpy, eCreatureBeholder, "HPS089Wl.PCX", "HPL089Wl.PCX" },
    { eSexMale, eRaceMinotaur, eClassWarlock, eSecSkillWisdom, eMasteryBasic, eSecSkillSorcery, eMasteryBasic, true, eSpellBloodLust, eCreatureTroglodyte, eCreatureHarpy, eCreatureBeholder, "HPS090Wl.PCX", "HPL090Wl.PCX" },
    { eSexMale, eRaceHuman, eClassWarlock, eSecSkillWisdom, eMasteryAdvanced, eSecSkillNone, eMasteryNone, true, eSpellResurrection, eCreatureTroglodyte, eCreatureHarpy, eCreatureBeholder, "HPS091Wl.PCX", "HPL091Wl.PCX" },
    { eSexMale, eRaceTroglodyte, eClassWarlock, eSecSkillWisdom, eMasteryBasic, eSecSkillEagleEye, eMasteryBasic, true, eSpellMuckAndMire, eCreatureTroglodyte, eCreatureHarpy, eCreatureBeholder, "HPS092Wl.PCX", "HPL092Wl.PCX" },
    { eSexMale, eRaceMinotaur, eClassWarlock, eSecSkillWisdom, eMasteryBasic, eSecSkillScouting, eMasteryAdvanced, true, eSpellMeteorShower, eCreatureTroglodyte, eCreatureHarpy, eCreatureBeholder, "HPS093Wl.PCX", "HPL093Wl.PCX" },
    { eSexFemale, eRaceHuman, eClassWarlock, eSecSkillWisdom, eMasteryBasic, eSecSkillIntelligence, eMasteryBasic, true, eSpellProtectionFromAir, eCreatureTroglodyte, eCreatureHarpy, eCreatureBeholder, "HPS094Wl.PCX", "HPL094Wl.PCX" },
    { eSexMale, eRaceMinotaur, eClassWarlock, eSecSkillWisdom, eMasteryBasic, eSecSkillLearning, eMasteryBasic, true, eSpellToughSkin, eCreatureTroglodyte, eCreatureHarpy, eCreatureBeholder, "HPS095Wl.PCX", "HPL095Wl.PCX" },
    { eSexMale, eRaceGenie, eClassBarbarian, eSecSkillOffense, eMasteryBasic, eSecSkillSiegeBallistics, eMasteryBasic, false, eSpellNone, eCreatureGoblin, eCreatureGoblinWolfRider, eCreatureOrc, "HPS096Br.PCX", "HPL096Br.PCX" },
    { eSexMale, eRaceGoblin, eClassBarbarian, eSecSkillOffense, eMasteryBasic, eSecSkillBattlefieldBallistics, eMasteryBasic, false, eSpellNone, eCreatureGoblin, eCreatureBallista, eCreatureOrc, "HPS097Br.PCX", "HPL097Br.PCX" },
    { eSexMale, eRaceOgre, eClassBarbarian, eSecSkillOffense, eMasteryBasic, eSecSkillArchery, eMasteryBasic, false, eSpellNone, eCreatureGoblin, eCreatureOrc, eCreatureOrc, "HPS098Br.PCX", "HPL098Br.PCX" },
    { eSexFemale, eRaceHuman, eClassBarbarian, eSecSkillOffense, eMasteryBasic, eSecSkillScouting, eMasteryBasic, false, eSpellNone, eCreatureGoblin, eCreatureGoblinWolfRider, eCreatureOrc, "HPS099Br.PCX", "HPL099Br.PCX" },
    { eSexFemale, eRaceGoblin, eClassBarbarian, eSecSkillOffense, eMasteryBasic, eSecSkillPathfinding, eMasteryBasic, false, eSpellNone, eCreatureGoblin, eCreatureGoblin, eCreatureGoblin, "HPS100Br.PCX", "HPL100Br.PCX" },
    { eSexFemale, eRaceOgre, eClassBarbarian, eSecSkillOffense, eMasteryBasic, eSecSkillMagicResistance, eMasteryBasic, false, eSpellNone, eCreatureGoblin, eCreatureGoblinWolfRider, eCreatureOrc, "HPS101Br.PCX", "HPL101Br.PCX" },
    { eSexMale, eRaceHuman, eClassBarbarian, eSecSkillOffense, eMasteryAdvanced, eSecSkillNone, eMasteryNone, false, eSpellNone, eCreatureGoblin, eCreatureGoblinWolfRider, eCreatureOrc, "HPS102Br.PCX", "HPL102Br.PCX" },
    { eSexMale, eRaceGoblin, eClassBarbarian, eSecSkillOffense, eMasteryBasic, eSecSkillBattleTactics, eMasteryBasic, false, eSpellNone, eCreatureGoblinWolfRider, eCreatureGoblinWolfRider, eCreatureGoblinWolfRider, "HPS103Br.PCX", "HPL103Br.PCX" },
    { eSexFemale, eRaceHuman, eClassBattleMage, eSecSkillWisdom, eMasteryBasic, eSecSkillSorcery, eMasteryBasic, true, eSpellBloodLust, eCreatureGoblin, eCreatureGoblinWolfRider, eCreatureOrc, "HPS104Bm.PCX", "HPL104Bm.PCX" },
    { eSexMale, eRaceOgre, eClassBattleMage, eSecSkillWisdom, eMasteryBasic, eSecSkillLeadership, eMasteryBasic, true, eSpellMagicBolt, eCreatureGoblin, eCreatureGoblinWolfRider, eCreatureOrc, "HPS105Bm.PCX", "HPL105Bm.PCX" },
    { eSexMale, eRaceOgre, eClassBattleMage, eSecSkillWisdom, eMasteryBasic, eSecSkillLogistics, eMasteryBasic, true, eSpellToughSkin, eCreatureGoblin, eCreatureGoblinWolfRider, eCreatureOrc, "HPS106Bm.PCX", "HPL106Bm.PCX" },
    { eSexMale, eRaceHuman, eClassBattleMage, eSecSkillWisdom, eMasteryBasic, eSecSkillBattleTactics, eMasteryBasic, true, eSpellTailWind, eCreatureGoblin, eCreatureGoblinWolfRider, eCreatureOrc, "HPS107Bm.PCX", "HPL107Bm.PCX" },
    { eSexMale, eRaceGoblin, eClassBattleMage, eSecSkillWisdom, eMasteryBasic, eSecSkillBattlefieldBallistics, eMasteryBasic, true, eSpellPrecision, eCreatureGoblin, eCreatureGoblinWolfRider, eCreatureOrc, "HPS108Bm.PCX", "HPL108Bm.PCX" },
    { eSexFemale, eRaceOgre, eClassBattleMage, eSecSkillWisdom, eMasteryBasic, eSecSkillOffense, eMasteryBasic, true, eSpellMuckAndMire, eCreatureGoblin, eCreatureGoblinWolfRider, eCreatureOrc, "HPS109Bm.PCX", "HPL109Bm.PCX" },
    { eSexFemale, eRaceHuman, eClassBattleMage, eSecSkillWisdom, eMasteryBasic, eSecSkillEagleEye, eMasteryBasic, true, eSpellProtectionFromAir, eCreatureGoblin, eCreatureGoblinWolfRider, eCreatureOrc, "HPS110Bm.PCX", "HPL110Bm.PCX" },
    { eSexMale, eRaceOgre, eClassBattleMage, eSecSkillWisdom, eMasteryBasic, eSecSkillMagicResistance, eMasteryBasic, true, eSpellBloodLust, eCreatureGoblin, eCreatureGoblinWolfRider, eCreatureOrc, "HPS111Bm.PCX", "HPL111Bm.PCX" },
    { eSexMale, eRaceHuman, eClassBeastmaster, eSecSkillDefense, eMasteryBasic, eSecSkillMagicResistance, eMasteryBasic, false, eSpellNone, eCreatureGnoll, eCreatureBasilisk, eCreatureSerpentFly, "HPS112Bs.PCX", "HPL112Bs.PCX" },
    { eSexMale, eRaceGnoll, eClassBeastmaster, eSecSkillDefense, eMasteryBasic, eSecSkillLeadership, eMasteryBasic, false, eSpellNone, eCreatureGnoll, eCreatureGnoll, eCreatureGnoll, "HPS113Bs.PCX", "HPL113Bs.PCX" },
    { eSexMale, eRaceLizardman, eClassBeastmaster, eSecSkillDefense, eMasteryBasic, eSecSkillArchery, eMasteryBasic, false, eSpellNone, eCreaturePrimitiveLizardman, eCreaturePrimitiveLizardman, eCreaturePrimitiveLizardman, "HPS114Bs.PCX", "HPL114Bs.PCX" },
    { eSexMale, eRaceHuman, eClassBeastmaster, eSecSkillDefense, eMasteryAdvanced, eSecSkillNone, eMasteryNone, false, eSpellNone, eCreatureGnoll, eCreaturePrimitiveLizardman, eCreatureSerpentFly, "HPS115Bs.PCX", "HPL115Bs.PCX" },
    { eSexMale, eRaceGnoll, eClassBeastmaster, eSecSkillDefense, eMasteryBasic, eSecSkillOffense, eMasteryBasic, false, eSpellNone, eCreatureGnoll, eCreaturePrimitiveLizardman, eCreatureSerpentFly, "HPS116Bs.PCX", "HPL116Bs.PCX" },
    { eSexMale, eRaceLizardman, eClassBeastmaster, eSecSkillDefense, eMasteryBasic, eSecSkillPathfinding, eMasteryBasic, false, eSpellNone, eCreatureGnoll, eCreatureSerpentFly, eCreatureSerpentFly, "HPS117Bs.PCX", "HPL117Bs.PCX" },
    { eSexMale, eRaceHuman, eClassBeastmaster, eSecSkillDefense, eMasteryBasic, eSecSkillBattlefieldBallistics, eMasteryBasic, false, eSpellNone, eCreatureGnoll, eCreatureBallista, eCreatureSerpentFly, "HPS118Bs.PCX", "HPL118Bs.PCX" },
    { eSexMale, eRaceLizardman, eClassBeastmaster, eSecSkillDefense, eMasteryBasic, eSecSkillScouting, eMasteryBasic, false, eSpellNone, eCreatureGnoll, eCreaturePrimitiveLizardman, eCreatureSerpentFly, "HPS119Bs.PCX", "HPL119Bs.PCX" },
    { eSexFemale, eRaceHuman, eClassWitch, eSecSkillWisdom, eMasteryAdvanced, eSecSkillNone, eMasteryBasic, true, eSpellWeakness, eCreatureGnoll, eCreaturePrimitiveLizardman, eCreatureSerpentFly, "HPS120Wh.PCX", "HPL120Wh.PCX" },
    { eSexFemale, eRaceGnoll, eClassWitch, eSecSkillWisdom, eMasteryBasic, eSecSkillMysticism, eMasteryBasic, true, eSpellMagicBolt, eCreatureGnoll, eCreaturePrimitiveLizardman, eCreatureSerpentFly, "HPS121Wh.PCX", "HPL121Wh.PCX" },
    { eSexFemale, eRaceLizardman, eClassWitch, eSecSkillWisdom, eMasteryBasic, eSecSkillNavigation, eMasteryBasic, true, eSpellMuckAndMire, eCreatureGnoll, eCreaturePrimitiveLizardman, eCreatureSerpentFly, "HPS122Wh.PCX", "HPL122Wh.PCX" },
    { eSexFemale, eRaceHuman, eClassWitch, eSecSkillWisdom, eMasteryBasic, eSecSkillFirstAid, eMasteryBasic, true, eSpellProtectionFromFire, eCreatureGnoll, eCreatureFirstAidTent, eCreatureSerpentFly, "HPS123Wh.PCX", "HPL123Wh.PCX" },
    { eSexFemale, eRaceGnoll, eClassWitch, eSecSkillWisdom, eMasteryBasic, eSecSkillLearning, eMasteryBasic, true, eSpellToughSkin, eCreatureGnoll, eCreaturePrimitiveLizardman, eCreatureSerpentFly, "HPS124Wh.PCX", "HPL124Wh.PCX" },
    { eSexFemale, eRaceLizardman, eClassWitch, eSecSkillWisdom, eMasteryBasic, eSecSkillSorcery, eMasteryBasic, true, eSpellShield, eCreatureGnoll, eCreaturePrimitiveLizardman, eCreatureSerpentFly, "HPS125Wh.PCX", "HPL125Wh.PCX" },
    { eSexFemale, eRaceHuman, eClassWitch, eSecSkillWisdom, eMasteryBasic, eSecSkillIntelligence, eMasteryBasic, true, eSpellDispel, eCreatureGnoll, eCreaturePrimitiveLizardman, eCreatureSerpentFly, "HPS126Wh.PCX", "HPL126Wh.PCX" },
    { eSexFemale, eRaceGnoll, eClassWitch, eSecSkillWisdom, eMasteryBasic, eSecSkillEagleEye, eMasteryBasic, true, eSpellToughSkin, eCreatureGnoll, eCreaturePrimitiveLizardman, eCreatureSerpentFly, "HPS127Wh.PCX", "HPL127Wh.PCX" },
    { eSexFemale, eRaceHuman, eClassKnight, eSecSkillLeadership, eMasteryBasic, eSecSkillOffense, eMasteryBasic, false, eSpellNone, eCreaturePikeman, eCreatureLightCrossbowman, eCreatureGriffin, "HPS128Qc.PCX", "HPL128Qc.PCX" },
    { eSexFemale, eRaceHuman, eClassKnight, eSecSkillLeadership, eMasteryBasic, eSecSkillOffense, eMasteryBasic, false, eSpellNone, eCreaturePikeman, eCreatureLightCrossbowman, eCreatureGriffin, "HPS129Mk.PCX", "HPL129Mk.PCX" },
};

// hctraits.txt supplies everything but the town.
static THeroClassTraits aHeroClassTraitsImp[kNumHeroClasses] = {
    { eTownCastle },  // eClassKnight
    { eTownCastle },  // eClassCleric
    { eTownRampart },  // eClassRanger
    { eTownRampart },  // eClassDruid
    { eTownTower },  // eClassAlchemist
    { eTownTower },  // eClassWizard
    { eTownInferno },  // eClassPagan
    { eTownInferno },  // eClassHeretic
    { eTownNecropolis },  // eClassDeathKnight
    { eTownNecropolis },  // eClassNecromancer
    { eTownDungeon },  // eClassOverlord
    { eTownDungeon },  // eClassWarlock
    { eTownStronghold },  // eClassBarbarian
    { eTownStronghold },  // eClassBattleMage
    { eTownFortress },  // eClassBeastmaster
    { eTownFortress },  // eClassWitch
};

static TSSkillTraits aSSkillTraitsImp[kNumSecSkills];

const THeroTraits (&akHeroTraits)[kNumHeroBios] = aHeroTraitsImp;
const THeroClassTraits (&akHeroClassTraits)[kNumHeroClasses] = aHeroClassTraitsImp;
const TSSkillTraits (&akSSkillTraits)[kNumSecSkills] = aSSkillTraitsImp;

static void InitializeHeroTraits(int id, const vector<char*>& resource);
static void InitializeHeroClassTraits(int id, const vector<char*>& resource);
static void InitializeSSkillTraits(int id, const vector<char*>& resource);

bool InitializeHeroTraitsTable()
{
    TSpreadsheetResource* resource = ResourceManager::GetSpreadsheet("hotraits.txt");
    if (!resource)
        return false;
    if (resource->GetNumberOfRows() < kNumHeroes + 2) {
        ResourceManager::Dispose(resource);
        return false;
    }

    int i;
    int row = 0;
    int id = 0;
    row += 2;
    for (i = 0; i < kNumHeroes; i++) {
        InitializeHeroTraits(id, resource->GetRow(row));
        id++;
        row++;
    }
    ResourceManager::Dispose(resource);
    return true;
}

bool InitializeHeroClassTraitsTable()
{
    TSpreadsheetResource* resource = ResourceManager::GetSpreadsheet("hctraits.txt");
    if (!resource)
        return false;
    if (resource->GetNumberOfRows() < kNumHeroClasses + 2) {
        ResourceManager::Dispose(resource);
        return false;
    }

    int i;
    int row = 0;
    int id = 0;
    row += 2;
    for (i = 0; i < kNumHeroClasses; i++) {
        InitializeHeroClassTraits(id, resource->GetRow(row));
        id++;
        row++;
    }
    ResourceManager::Dispose(resource);
    return true;
}

bool InitializeSSkillTraitsTable()
{
    TSpreadsheetResource* resource = ResourceManager::GetSpreadsheet("sstraits.txt");
    if (!resource)
        return false;
    if (resource->GetNumberOfRows() < kNumSecSkills + 2) {
        ResourceManager::Dispose(resource);
        return false;
    }

    int i;
    int row = 0;
    int id = 0;
    row += 2;
    for (i = 0; i < kNumSecSkills; i++) {
        InitializeSSkillTraits(id, resource->GetRow(row));
        id++;
        row++;
    }
    ResourceManager::Dispose(resource);
    return true;
}

namespace {

// Owns one loaded string; the loaders keep the pointers in the traits rows.
class TAutoStrPtr {
public:
    TAutoStrPtr() : str(0) {}
    ~TAutoStrPtr() { delete[] str; }
    void set(char* newStr) { str = newStr; }
    char* get() const { return str; }

private:
    char* str;
};

}

static void InitializeHeroTraits(int id, const vector<char*>& resource)
{
#line 408
    assert(id >= 0 && id < kNumHeroes);
    assert(resource.size() >= 10);
    THeroTraits& traits = aHeroTraitsImp[id];

    static TAutoStrPtr aNameAutoStrs[kNumHeroes];
    aNameAutoStrs[id].set(new char[strlen(resource[0]) + 1]);
    strcpy(aNameAutoStrs[id].get(), resource[0]);
    traits.m_name = aNameAutoStrs[id].get();

    traits.m_1stStackLow = atoi(resource[1]);
    traits.m_1stStackHigh = atoi(resource[2]);
    traits.m_2ndStackLow = atoi(resource[4]);
    traits.m_2ndStackHigh = atoi(resource[5]);
    traits.m_3rdStackLow = atoi(resource[7]);
    traits.m_3rdStackHigh = atoi(resource[8]);
}

static void InitializeHeroClassTraits(int id, const vector<char*>& resource)
{
#line 440
    assert(id >= 0 && id < kNumHeroClasses);
    assert(resource.size() >= 50);
    THeroClassTraits& traits = aHeroClassTraitsImp[id];

    static TAutoStrPtr aNameAutoStrs[kNumHeroClasses];
    aNameAutoStrs[id].set(new char[strlen(resource[0]) + 1]);
    strcpy(aNameAutoStrs[id].get(), resource[0]);
    traits.m_name = aNameAutoStrs[id].get();

    traits.m_aggression = atof(resource[1]);

    int i;
    for (i = 0; i < 4; i++)
        traits.m_initialPrimarySkill[i] = atoi(resource[i + 2]);
    for (i = 0; i < 4; i++)
        traits.m_gainPrimarySkillChance[i] = atoi(resource[i + 6]);
    for (i = 0; i < 4; i++)
        traits.m_gainPrimarySkillChance10P[i] = atoi(resource[i + 10]);
    for (i = 0; i < kNumSecSkills; i++)
        traits.m_gainSecondarySkillChance[i] = atoi(resource[i + 14]);
    for (i = 0; i < kNumTownTypes; i++)
        traits.m_foundInTownType[i] = atoi(resource[i + 42]);
}

static void InitializeSSkillTraits(int id, const vector<char*>& resource)
{
#line 488
    assert(id >= 0 && id < kNumSecSkills);
    assert(resource.size() >= 4);
    TSSkillTraits& traits = aSSkillTraitsImp[id];

    static TAutoStrPtr aNameAutoStrs[kNumSecSkills];
    aNameAutoStrs[id].set(new char[strlen(resource[0]) + 1]);
    strcpy(aNameAutoStrs[id].get(), resource[0]);
    traits.m_name = aNameAutoStrs[id].get();

    static TAutoStrPtr aDescAutoStrs[kNumSecSkills][3];
    for (int level = 0; level < 3; level++) {
        aDescAutoStrs[id][level].set(new char[strlen(resource[level + 1]) + 1]);
        strcpy(aDescAutoStrs[id][level].get(), resource[level + 1]);
        traits.m_levelNames[level] = aDescAutoStrs[id][level].get();
    }
}
