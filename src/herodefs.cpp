#include "va.h"

#include <stdlib.h>
#include <string.h>

#include "herodefs.h"

#include "hero.h"
#include "resourcemanager.h"
#include "textresource.h"

// Retail initial data: parsers fill names and numeric traits later; hero
// portraits, starting skills/stacks and availability already exist at startup.
DATA(0x00679dd0)
THeroTraits g_heroTraitsStorage[163] = {
    { 0, 7, THeroClass(0), eSecSkillLeadership, eMasteryBasic, eSecSkillArchery, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS000Kn.PCX", "HPL000Kn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(0), eSecSkillLeadership, eMasteryBasic, eSecSkillArchery, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_ARCHER, CREATURE_ARCHER, CREATURE_ARCHER, "HPS001Kn.PCX", "HPL001Kn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(0), eSecSkillLeadership, eMasteryBasic, eSecSkillDefense, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIKEMAN, CREATURE_GRIFFIN, CREATURE_GRIFFIN, "HPS002Kn.PCX", "HPL002Kn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(0), eSecSkillLeadership, eMasteryBasic, eSecSkillNavigation, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS003Kn.PCX", "HPL003Kn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(0), eSecSkillLeadership, eMasteryBasic, eSecSkillEstates, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS004Kn.PCX", "HPL004Kn.PCX", { 1 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(0), eSecSkillLeadership, eMasteryBasic, eSecSkillOffense, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS005Kn.PCX", "HPL005Kn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(0), eSecSkillLeadership, eMasteryBasic, eSecSkillBattlefieldBallistics, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIKEMAN, CREATURE_BALLISTA, CREATURE_GRIFFIN, "HPS006Kn.PCX", "HPL006Kn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(0), eSecSkillLeadership, eMasteryBasic, eSecSkillBattleTactics, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS007Kn.PCX", "HPL007Kn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(1), eSecSkillWisdom, eMasteryBasic, eSecSkillFirstAid, eMasteryBasic, 1, { 0, 0, 0 }, 46, CREATURE_PIKEMAN, CREATURE_FIRST_AID_TENT, CREATURE_GRIFFIN, "HPS008Cl.PCX", "HPL008Cl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(1), eSecSkillWisdom, eMasteryBasic, eSecSkillDiplomacy, eMasteryBasic, 1, { 0, 0, 0 }, 41, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS009Cl.PCX", "HPL009Cl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(1), eSecSkillWisdom, eMasteryBasic, eSecSkillEstates, eMasteryBasic, 1, { 0, 0, 0 }, 45, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS010Cl.PCX", "HPL010Cl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(1), eSecSkillWisdom, eMasteryAdvanced, eSecSkillNone, eMasteryNone, 1, { 0, 0, 0 }, 20, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS011Cl.PCX", "HPL011Cl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(1), eSecSkillWisdom, eMasteryBasic, eSecSkillMysticism, eMasteryBasic, 1, { 0, 0, 0 }, 42, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS012Cl.PCX", "HPL012Cl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(1), eSecSkillWisdom, eMasteryBasic, eSecSkillEagleEye, eMasteryBasic, 1, { 0, 0, 0 }, 35, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS013Cl.PCX", "HPL013Cl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(1), eSecSkillWisdom, eMasteryBasic, eSecSkillLearning, eMasteryBasic, 1, { 0, 0, 0 }, 48, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS014Cl.PCX", "HPL014Cl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(1), eSecSkillWisdom, eMasteryBasic, eSecSkillIntelligence, eMasteryBasic, 1, { 0, 0, 0 }, 37, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS015Cl.PCX", "HPL015Cl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(2), eSecSkillLeadership, eMasteryBasic, eSecSkillDefense, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_CENTAUR, CREATURE_DWARF, CREATURE_WOOD_ELF, "HPS016Rn.PCX", "HPL016Rn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, THeroClass(2), eSecSkillMagicResistance, eMasteryBasic, eSecSkillLuck, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_DWARF, CREATURE_DWARF, CREATURE_DWARF, "HPS017Rn.PCX", "HPL017Rn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 3, THeroClass(2), eSecSkillArchery, eMasteryAdvanced, eSecSkillNone, eMasteryNone, 0, { 0, 0, 0 }, -1, CREATURE_CENTAUR, CREATURE_DWARF, CREATURE_WOOD_ELF, "HPS018Rn.PCX", "HPL018Rn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(2), eSecSkillLeadership, eMasteryBasic, eSecSkillDiplomacy, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_CENTAUR, CREATURE_DWARF, CREATURE_WOOD_ELF, "HPS019Rn.PCX", "HPL019Rn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, THeroClass(2), eSecSkillMagicResistance, eMasteryAdvanced, eSecSkillNone, eMasteryNone, 0, { 0, 0, 0 }, -1, CREATURE_CENTAUR, CREATURE_DWARF, CREATURE_WOOD_ELF, "HPS020Rn.PCX", "HPL020Rn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 3, THeroClass(2), eSecSkillArchery, eMasteryBasic, eSecSkillOffense, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_CENTAUR, CREATURE_WOOD_ELF, CREATURE_WOOD_ELF, "HPS021Rn.PCX", "HPL021Rn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, THeroClass(2), eSecSkillMagicResistance, eMasteryBasic, eSecSkillPathfinding, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_CENTAUR, CREATURE_DWARF, CREATURE_WOOD_ELF, "HPS022Rn.PCX", "HPL022Rn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 3, THeroClass(2), eSecSkillArchery, eMasteryBasic, eSecSkillLogistics, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_CENTAUR, CREATURE_DWARF, CREATURE_WOOD_ELF, "HPS023Rn.PCX", "HPL023Rn.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(3), eSecSkillWisdom, eMasteryBasic, eSecSkillMagicScholar, eMasteryBasic, 1, { 0, 0, 0 }, 55, CREATURE_CENTAUR, CREATURE_DWARF, CREATURE_WOOD_ELF, "HPS024Dr.PCX", "HPL024Dr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, THeroClass(3), eSecSkillWisdom, eMasteryAdvanced, eSecSkillSiegeBallistics, eMasteryBasic, 1, { 0, 0, 0 }, 37, CREATURE_CENTAUR, CREATURE_DWARF, CREATURE_WOOD_ELF, "HPS025Dr.PCX", "HPL025Dr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 3, THeroClass(3), eSecSkillWisdom, eMasteryBasic, eSecSkillIntelligence, eMasteryBasic, 1, { 0, 0, 0 }, 42, CREATURE_CENTAUR, CREATURE_DWARF, CREATURE_WOOD_ELF, "HPS026Dr.PCX", "HPL026Dr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(3), eSecSkillWisdom, eMasteryBasic, eSecSkillFirstAid, eMasteryBasic, 1, { 0, 0, 0 }, 0, CREATURE_CENTAUR, CREATURE_FIRST_AID_TENT, CREATURE_WOOD_ELF, "HPS027Dr.PCX", "HPL027Dr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, THeroClass(3), eSecSkillWisdom, eMasteryBasic, eSecSkillEagleEye, eMasteryBasic, 1, { 0, 0, 0 }, 15, CREATURE_CENTAUR, CREATURE_DWARF, CREATURE_WOOD_ELF, "HPS028Dr.PCX", "HPL028Dr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 3, THeroClass(3), eSecSkillWisdom, eMasteryBasic, eSecSkillLuck, eMasteryBasic, 1, { 0, 0, 0 }, 51, CREATURE_CENTAUR, CREATURE_DWARF, CREATURE_WOOD_ELF, "HPS029Dr.PCX", "HPL029Dr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(3), eSecSkillWisdom, eMasteryBasic, eSecSkillSorcery, eMasteryBasic, 1, { 0, 0, 0 }, 16, CREATURE_CENTAUR, CREATURE_DWARF, CREATURE_WOOD_ELF, "HPS030Dr.PCX", "HPL030Dr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 3, THeroClass(3), eSecSkillWisdom, eMasteryBasic, eSecSkillScouting, eMasteryBasic, 1, { 0, 0, 0 }, 30, CREATURE_CENTAUR, CREATURE_DWARF, CREATURE_WOOD_ELF, "HPS031Dr.PCX", "HPL031Dr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(4), eSecSkillMysticism, eMasteryBasic, eSecSkillScouting, eMasteryBasic, 1, { 0, 0, 0 }, 27, CREATURE_STONE_GARGOYLE, CREATURE_STONE_GARGOYLE, CREATURE_STONE_GARGOYLE, "HPS032Al.PCX", "HPL032Al.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 4, THeroClass(4), eSecSkillMagicScholar, eMasteryAdvanced, eSecSkillNone, eMasteryNone, 1, { 0, 0, 0 }, 15, CREATURE_GREMLIN, CREATURE_STONE_GARGOYLE, CREATURE_STONE_GOLEM, "HPS033Al.PCX", "HPL033Al.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(4), eSecSkillMysticism, eMasteryBasic, eSecSkillSorcery, eMasteryBasic, 1, { 0, 0, 0 }, 53, CREATURE_GREMLIN, CREATURE_STONE_GOLEM, CREATURE_STONE_GOLEM, "HPS034Al.PCX", "HPL034Al.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 4, THeroClass(4), eSecSkillMagicScholar, eMasteryBasic, eSecSkillDefense, eMasteryBasic, 1, { 0, 0, 0 }, 27, CREATURE_GREMLIN, CREATURE_STONE_GARGOYLE, CREATURE_STONE_GOLEM, "HPS035Al.PCX", "HPL035Al.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(4), eSecSkillMysticism, eMasteryBasic, eSecSkillBattleTactics, eMasteryBasic, 1, { 0, 0, 0 }, 15, CREATURE_GREMLIN, CREATURE_BALLISTA, CREATURE_STONE_GOLEM, "HPS036Al.PCX", "HPL036Al.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 4, THeroClass(4), eSecSkillMagicScholar, eMasteryBasic, eSecSkillMagicResistance, eMasteryBasic, 1, { 0, 0, 0 }, 53, CREATURE_GREMLIN, CREATURE_STONE_GARGOYLE, CREATURE_STONE_GOLEM, "HPS037Al.PCX", "HPL037Al.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(4), eSecSkillMysticism, eMasteryBasic, eSecSkillOffense, eMasteryBasic, 1, { 0, 0, 0 }, 15, CREATURE_GREMLIN, CREATURE_STONE_GARGOYLE, CREATURE_STONE_GOLEM, "HPS038Al.PCX", "HPL038Al.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 4, THeroClass(4), eSecSkillMagicScholar, eMasteryBasic, eSecSkillIntelligence, eMasteryBasic, 1, { 0, 0, 0 }, 15, CREATURE_GREMLIN, CREATURE_STONE_GARGOYLE, CREATURE_STONE_GOLEM, "HPS039Al.PCX", "HPL039Al.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(5), eSecSkillWisdom, eMasteryAdvanced, eSecSkillNone, eMasteryNone, 1, { 0, 0, 0 }, 60, CREATURE_GREMLIN, CREATURE_STONE_GARGOYLE, CREATURE_STONE_GOLEM, "HPS040Wz.PCX", "HPL040Wz.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 4, THeroClass(5), eSecSkillWisdom, eMasteryBasic, eSecSkillMysticism, eMasteryBasic, 1, { 0, 0, 0 }, 46, CREATURE_GREMLIN, CREATURE_STONE_GARGOYLE, CREATURE_STONE_GOLEM, "HPS041Wz.PCX", "HPL041Wz.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(5), eSecSkillWisdom, eMasteryBasic, eSecSkillEagleEye, eMasteryBasic, 1, { 0, 0, 0 }, 35, CREATURE_GREMLIN, CREATURE_STONE_GARGOYLE, CREATURE_STONE_GOLEM, "HPS042Wz.PCX", "HPL042Wz.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 4, THeroClass(5), eSecSkillWisdom, eMasteryBasic, eSecSkillIntelligence, eMasteryBasic, 1, { 0, 0, 0 }, 51, CREATURE_GREMLIN, CREATURE_STONE_GARGOYLE, CREATURE_STONE_GOLEM, "HPS043Wz.PCX", "HPL043Wz.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(5), eSecSkillWisdom, eMasteryBasic, eSecSkillSiegeBallistics, eMasteryBasic, 1, { 0, 0, 0 }, 27, CREATURE_GREMLIN, CREATURE_STONE_GARGOYLE, CREATURE_STONE_GOLEM, "HPS044Wz.PCX", "HPL044Wz.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 4, THeroClass(5), eSecSkillWisdom, eMasteryBasic, eSecSkillSorcery, eMasteryBasic, 1, { 0, 0, 0 }, 19, CREATURE_GREMLIN, CREATURE_STONE_GARGOYLE, CREATURE_STONE_GOLEM, "HPS045Wz.PCX", "HPL045Wz.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(5), eSecSkillWisdom, eMasteryBasic, eSecSkillDiplomacy, eMasteryBasic, 1, { 0, 0, 0 }, 53, CREATURE_GREMLIN, CREATURE_STONE_GARGOYLE, CREATURE_STONE_GOLEM, "HPS046Wz.PCX", "HPL046Wz.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 4, THeroClass(5), eSecSkillWisdom, eMasteryBasic, eSecSkillMagicScholar, eMasteryBasic, 1, { 0, 0, 0 }, 42, CREATURE_GREMLIN, CREATURE_STONE_GARGOYLE, CREATURE_STONE_GOLEM, "HPS047Wz.PCX", "HPL047Wz.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(6), eSecSkillScouting, eMasteryAdvanced, eSecSkillNone, eMasteryNone, 0, { 0, 0, 0 }, -1, CREATURE_IMP, CREATURE_HELL_HOUND, CREATURE_HELL_HOUND, "HPS048Hr.PCX", "HPL048Hr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 2, THeroClass(6), eSecSkillMagicScholar, eMasteryBasic, eSecSkillWisdom, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_IMP, CREATURE_GOG, CREATURE_HELL_HOUND, "HPS049Hr.PCX", "HPL049Hr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, THeroClass(6), eSecSkillDefense, eMasteryAdvanced, eSecSkillNone, eMasteryNone, 0, { 0, 0, 0 }, -1, CREATURE_IMP, CREATURE_GOG, CREATURE_HELL_HOUND, "HPS050Hr.PCX", "HPL050Hr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(6), eSecSkillBattleTactics, eMasteryBasic, eSecSkillMagicResistance, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_IMP, CREATURE_IMP, CREATURE_IMP, "HPS051Hr.PCX", "HPL051Hr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 2, THeroClass(6), eSecSkillMagicScholar, eMasteryBasic, eSecSkillOffense, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_IMP, CREATURE_GOG, CREATURE_HELL_HOUND, "HPS052Hr.PCX", "HPL052Hr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, THeroClass(6), eSecSkillArchery, eMasteryBasic, eSecSkillScouting, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_GOG, CREATURE_GOG, CREATURE_GOG, "HPS053Hr.PCX", "HPL053Hr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(6), eSecSkillLogistics, eMasteryBasic, eSecSkillBattlefieldBallistics, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_IMP, CREATURE_BALLISTA, CREATURE_HELL_HOUND, "HPS054Hr.PCX", "HPL054Hr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, THeroClass(6), eSecSkillOffense, eMasteryAdvanced, eSecSkillNone, eMasteryNone, 0, { 0, 0, 0 }, -1, CREATURE_IMP, CREATURE_GOG, CREATURE_HELL_HOUND, "HPS055Hr.PCX", "HPL055Hr.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(7), eSecSkillWisdom, eMasteryBasic, eSecSkillIntelligence, eMasteryBasic, 1, { 0, 0, 0 }, 3, CREATURE_IMP, CREATURE_GOG, CREATURE_HELL_HOUND, "HPS056Dm.PCX", "HPL056Dm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 2, THeroClass(7), eSecSkillWisdom, eMasteryBasic, eSecSkillMagicScholar, eMasteryBasic, 1, { 0, 0, 0 }, 22, CREATURE_IMP, CREATURE_GOG, CREATURE_HELL_HOUND, "HPS057Dm.PCX", "HPL057Dm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, THeroClass(7), eSecSkillWisdom, eMasteryBasic, eSecSkillMysticism, eMasteryBasic, 1, { 0, 0, 0 }, 30, CREATURE_IMP, CREATURE_GOG, CREATURE_HELL_HOUND, "HPS058Dm.PCX", "HPL058Dm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(7), eSecSkillWisdom, eMasteryBasic, eSecSkillSiegeBallistics, eMasteryBasic, 1, { 0, 0, 0 }, 45, CREATURE_IMP, CREATURE_GOG, CREATURE_HELL_HOUND, "HPS059Dm.PCX", "HPL059Dm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, THeroClass(7), eSecSkillWisdom, eMasteryBasic, eSecSkillLearning, eMasteryBasic, 1, { 0, 0, 0 }, 53, CREATURE_IMP, CREATURE_GOG, CREATURE_HELL_HOUND, "HPS060Dm.PCX", "HPL060Dm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 0, THeroClass(7), eSecSkillWisdom, eMasteryBasic, eSecSkillEagleEye, eMasteryBasic, 1, { 0, 0, 0 }, 43, CREATURE_IMP, CREATURE_GOG, CREATURE_HELL_HOUND, "HPS061Dm.PCX", "HPL061Dm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 2, THeroClass(7), eSecSkillWisdom, eMasteryBasic, eSecSkillSorcery, eMasteryBasic, 1, { 0, 0, 0 }, 46, CREATURE_IMP, CREATURE_GOG, CREATURE_HELL_HOUND, "HPS062Dm.PCX", "HPL062Dm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(7), eSecSkillWisdom, eMasteryBasic, eSecSkillLeadership, eMasteryBasic, 1, { 0, 0, 0 }, 21, CREATURE_IMP, CREATURE_GOG, CREATURE_HELL_HOUND, "HPS063Dm.PCX", "HPL063Dm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(8), eSecSkillNecromancy, eMasteryBasic, eSecSkillMagicResistance, eMasteryBasic, 1, { 0, 0, 0 }, 53, CREATURE_WALKING_DEAD, CREATURE_WALKING_DEAD, CREATURE_WALKING_DEAD, "HPS064Dk.PCX", "HPL064Dk.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 13, THeroClass(8), eSecSkillNecromancy, eMasteryBasic, eSecSkillBattlefieldBallistics, eMasteryBasic, 1, { 0, 0, 0 }, 46, CREATURE_SKELETON, CREATURE_WALKING_DEAD, CREATURE_WIGHT, "HPS065Dk.PCX", "HPL065Dk.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 8, THeroClass(8), eSecSkillNecromancy, eMasteryBasic, eSecSkillLearning, eMasteryBasic, 1, { 0, 0, 0 }, 54, CREATURE_SKELETON, CREATURE_WALKING_DEAD, CREATURE_WIGHT, "HPS066Dk.PCX", "HPL066Dk.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(8), eSecSkillNecromancy, eMasteryBasic, eSecSkillBattleTactics, eMasteryBasic, 1, { 0, 0, 0 }, 15, CREATURE_SKELETON, CREATURE_WIGHT, CREATURE_WIGHT, "HPS067Dk.PCX", "HPL067Dk.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 13, THeroClass(8), eSecSkillNecromancy, eMasteryBasic, eSecSkillOffense, eMasteryBasic, 1, { 0, 0, 0 }, 15, CREATURE_SKELETON, CREATURE_WALKING_DEAD, CREATURE_WIGHT, "HPS068Dk.PCX", "HPL068Dk.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 8, THeroClass(8), eSecSkillNecromancy, eMasteryAdvanced, eSecSkillNone, eMasteryBasic, 1, { 0, 0, 0 }, 15, CREATURE_SKELETON, CREATURE_WALKING_DEAD, CREATURE_WIGHT, "HPS069Dk.PCX", "HPL069Dk.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(8), eSecSkillNecromancy, eMasteryBasic, eSecSkillOffense, eMasteryBasic, 1, { 0, 0, 0 }, 15, CREATURE_SKELETON, CREATURE_WALKING_DEAD, CREATURE_WIGHT, "HPS070Dk.PCX", "HPL070Dk.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 13, THeroClass(8), eSecSkillNecromancy, eMasteryBasic, eSecSkillDefense, eMasteryBasic, 1, { 0, 0, 0 }, 27, CREATURE_SKELETON, CREATURE_SKELETON, CREATURE_SKELETON, "HPS071Dk.PCX", "HPL071Dk.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(9), eSecSkillNecromancy, eMasteryBasic, eSecSkillMagicScholar, eMasteryBasic, 1, { 0, 0, 0 }, 24, CREATURE_SKELETON, CREATURE_WALKING_DEAD, CREATURE_WIGHT, "HPS072Nc.PCX", "HPL072Nc.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 13, THeroClass(9), eSecSkillNecromancy, eMasteryBasic, eSecSkillWisdom, eMasteryBasic, 1, { 0, 0, 0 }, 23, CREATURE_SKELETON, CREATURE_WALKING_DEAD, CREATURE_WIGHT, "HPS073Nc.PCX", "HPL073Nc.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 8, THeroClass(9), eSecSkillNecromancy, eMasteryBasic, eSecSkillSorcery, eMasteryBasic, 1, { 0, 0, 0 }, 54, CREATURE_SKELETON, CREATURE_WALKING_DEAD, CREATURE_WIGHT, "HPS074Nc.PCX", "HPL074Nc.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(9), eSecSkillNecromancy, eMasteryBasic, eSecSkillEagleEye, eMasteryBasic, 1, { 0, 0, 0 }, 27, CREATURE_SKELETON, CREATURE_WALKING_DEAD, CREATURE_WIGHT, "HPS075Nc.PCX", "HPL075Nc.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 13, THeroClass(9), eSecSkillNecromancy, eMasteryBasic, eSecSkillMysticism, eMasteryBasic, 1, { 0, 0, 0 }, 39, CREATURE_SKELETON, CREATURE_WALKING_DEAD, CREATURE_WIGHT, "HPS076Nc.PCX", "HPL076Nc.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 8, THeroClass(9), eSecSkillNecromancy, eMasteryBasic, eSecSkillLearning, eMasteryBasic, 1, { 0, 0, 0 }, 46, CREATURE_SKELETON, CREATURE_WALKING_DEAD, CREATURE_WIGHT, "HPS077Nc.PCX", "HPL077Nc.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(9), eSecSkillNecromancy, eMasteryAdvanced, eSecSkillNone, eMasteryNone, 1, { 0, 0, 0 }, 42, CREATURE_SKELETON, CREATURE_WALKING_DEAD, CREATURE_WIGHT, "HPS078Nc.PCX", "HPL078Nc.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 8, THeroClass(9), eSecSkillNecromancy, eMasteryBasic, eSecSkillIntelligence, eMasteryBasic, 1, { 0, 0, 0 }, 30, CREATURE_SKELETON, CREATURE_WALKING_DEAD, CREATURE_WIGHT, "HPS079Nc.PCX", "HPL079Nc.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(10), eSecSkillLeadership, eMasteryBasic, eSecSkillScouting, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_HARPY, CREATURE_HARPY, CREATURE_HARPY, "HPS080Ov.PCX", "HPL080Ov.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 12, THeroClass(10), eSecSkillOffense, eMasteryBasic, eSecSkillBattlefieldBallistics, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_TROGLODYTE, CREATURE_BALLISTA, CREATURE_BEHOLDER, "HPS081Ov.PCX", "HPL081Ov.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 10, THeroClass(10), eSecSkillBattleTactics, eMasteryBasic, eSecSkillOffense, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_TROGLODYTE, CREATURE_HARPY, CREATURE_BEHOLDER, "HPS082Ov.PCX", "HPL082Ov.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(10), eSecSkillLeadership, eMasteryBasic, eSecSkillMagicResistance, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_TROGLODYTE, CREATURE_BEHOLDER, CREATURE_BEHOLDER, "HPS083Ov.PCX", "HPL083Ov.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 12, THeroClass(10), eSecSkillOffense, eMasteryAdvanced, eSecSkillNone, eMasteryNone, 0, { 0, 0, 0 }, -1, CREATURE_TROGLODYTE, CREATURE_HARPY, CREATURE_BEHOLDER, "HPS084Ov.PCX", "HPL084Ov.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 10, THeroClass(10), eSecSkillBattleTactics, eMasteryBasic, eSecSkillLogistics, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_TROGLODYTE, CREATURE_HARPY, CREATURE_BEHOLDER, "HPS085Ov.PCX", "HPL085Ov.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(10), eSecSkillLeadership, eMasteryBasic, eSecSkillMagicScholar, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_TROGLODYTE, CREATURE_HARPY, CREATURE_BEHOLDER, "HPS086Ov.PCX", "HPL086Ov.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 12, THeroClass(10), eSecSkillOffense, eMasteryBasic, eSecSkillBattleTactics, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_TROGLODYTE, CREATURE_TROGLODYTE, CREATURE_TROGLODYTE, "HPS087Ov.PCX", "HPL087Ov.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(11), eSecSkillWisdom, eMasteryBasic, eSecSkillMagicScholar, eMasteryBasic, 1, { 0, 0, 0 }, 38, CREATURE_TROGLODYTE, CREATURE_HARPY, CREATURE_BEHOLDER, "HPS088Wl.PCX", "HPL088Wl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 12, THeroClass(11), eSecSkillWisdom, eMasteryBasic, eSecSkillMysticism, eMasteryBasic, 1, { 0, 0, 0 }, 27, CREATURE_TROGLODYTE, CREATURE_HARPY, CREATURE_BEHOLDER, "HPS089Wl.PCX", "HPL089Wl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 10, THeroClass(11), eSecSkillWisdom, eMasteryBasic, eSecSkillSorcery, eMasteryBasic, 1, { 0, 0, 0 }, 43, CREATURE_TROGLODYTE, CREATURE_HARPY, CREATURE_BEHOLDER, "HPS090Wl.PCX", "HPL090Wl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(11), eSecSkillWisdom, eMasteryAdvanced, eSecSkillNone, eMasteryNone, 1, { 0, 0, 0 }, 38, CREATURE_TROGLODYTE, CREATURE_HARPY, CREATURE_BEHOLDER, "HPS091Wl.PCX", "HPL091Wl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 12, THeroClass(11), eSecSkillWisdom, eMasteryBasic, eSecSkillEagleEye, eMasteryBasic, 1, { 0, 0, 0 }, 54, CREATURE_TROGLODYTE, CREATURE_HARPY, CREATURE_BEHOLDER, "HPS092Wl.PCX", "HPL092Wl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 10, THeroClass(11), eSecSkillWisdom, eMasteryBasic, eSecSkillScouting, eMasteryAdvanced, 1, { 0, 0, 0 }, 23, CREATURE_TROGLODYTE, CREATURE_HARPY, CREATURE_BEHOLDER, "HPS093Wl.PCX", "HPL093Wl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(11), eSecSkillWisdom, eMasteryBasic, eSecSkillIntelligence, eMasteryBasic, 1, { 0, 0, 0 }, 30, CREATURE_TROGLODYTE, CREATURE_HARPY, CREATURE_BEHOLDER, "HPS094Wl.PCX", "HPL094Wl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 10, THeroClass(11), eSecSkillWisdom, eMasteryBasic, eSecSkillLearning, eMasteryBasic, 1, { 0, 0, 0 }, 46, CREATURE_TROGLODYTE, CREATURE_HARPY, CREATURE_BEHOLDER, "HPS095Wl.PCX", "HPL095Wl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 4, THeroClass(12), eSecSkillOffense, eMasteryBasic, eSecSkillSiegeBallistics, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_GOBLIN, CREATURE_WOLF_RIDER, CREATURE_ORC, "HPS096Br.PCX", "HPL096Br.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 6, THeroClass(12), eSecSkillOffense, eMasteryBasic, eSecSkillBattlefieldBallistics, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_GOBLIN, CREATURE_BALLISTA, CREATURE_ORC, "HPS097Br.PCX", "HPL097Br.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 11, THeroClass(12), eSecSkillOffense, eMasteryBasic, eSecSkillArchery, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_GOBLIN, CREATURE_ORC, CREATURE_ORC, "HPS098Br.PCX", "HPL098Br.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(12), eSecSkillOffense, eMasteryBasic, eSecSkillScouting, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_GOBLIN, CREATURE_WOLF_RIDER, CREATURE_ORC, "HPS099Br.PCX", "HPL099Br.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 6, THeroClass(12), eSecSkillOffense, eMasteryBasic, eSecSkillPathfinding, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_GOBLIN, CREATURE_GOBLIN, CREATURE_GOBLIN, "HPS100Br.PCX", "HPL100Br.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 11, THeroClass(12), eSecSkillOffense, eMasteryBasic, eSecSkillMagicResistance, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_GOBLIN, CREATURE_WOLF_RIDER, CREATURE_ORC, "HPS101Br.PCX", "HPL101Br.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(12), eSecSkillOffense, eMasteryAdvanced, eSecSkillNone, eMasteryNone, 0, { 0, 0, 0 }, -1, CREATURE_GOBLIN, CREATURE_WOLF_RIDER, CREATURE_ORC, "HPS102Br.PCX", "HPL102Br.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 6, THeroClass(12), eSecSkillOffense, eMasteryBasic, eSecSkillBattleTactics, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_WOLF_RIDER, CREATURE_WOLF_RIDER, CREATURE_WOLF_RIDER, "HPS103Br.PCX", "HPL103Br.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(13), eSecSkillWisdom, eMasteryBasic, eSecSkillSorcery, eMasteryBasic, 1, { 0, 0, 0 }, 43, CREATURE_GOBLIN, CREATURE_WOLF_RIDER, CREATURE_ORC, "HPS104Bm.PCX", "HPL104Bm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 11, THeroClass(13), eSecSkillWisdom, eMasteryBasic, eSecSkillLeadership, eMasteryBasic, 1, { 0, 0, 0 }, 15, CREATURE_GOBLIN, CREATURE_WOLF_RIDER, CREATURE_ORC, "HPS105Bm.PCX", "HPL105Bm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 11, THeroClass(13), eSecSkillWisdom, eMasteryBasic, eSecSkillLogistics, eMasteryBasic, 1, { 0, 0, 0 }, 46, CREATURE_GOBLIN, CREATURE_WOLF_RIDER, CREATURE_ORC, "HPS106Bm.PCX", "HPL106Bm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(13), eSecSkillWisdom, eMasteryBasic, eSecSkillBattleTactics, eMasteryBasic, 1, { 0, 0, 0 }, 53, CREATURE_GOBLIN, CREATURE_WOLF_RIDER, CREATURE_ORC, "HPS107Bm.PCX", "HPL107Bm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 6, THeroClass(13), eSecSkillWisdom, eMasteryBasic, eSecSkillBattlefieldBallistics, eMasteryBasic, 1, { 0, 0, 0 }, 44, CREATURE_GOBLIN, CREATURE_WOLF_RIDER, CREATURE_ORC, "HPS108Bm.PCX", "HPL108Bm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 11, THeroClass(13), eSecSkillWisdom, eMasteryBasic, eSecSkillOffense, eMasteryBasic, 1, { 0, 0, 0 }, 54, CREATURE_GOBLIN, CREATURE_WOLF_RIDER, CREATURE_ORC, "HPS109Bm.PCX", "HPL109Bm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(13), eSecSkillWisdom, eMasteryBasic, eSecSkillEagleEye, eMasteryBasic, 1, { 0, 0, 0 }, 30, CREATURE_GOBLIN, CREATURE_WOLF_RIDER, CREATURE_ORC, "HPS110Bm.PCX", "HPL110Bm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 11, THeroClass(13), eSecSkillWisdom, eMasteryBasic, eSecSkillMagicResistance, eMasteryBasic, 1, { 0, 0, 0 }, 43, CREATURE_GOBLIN, CREATURE_WOLF_RIDER, CREATURE_ORC, "HPS111Bm.PCX", "HPL111Bm.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(14), eSecSkillDefense, eMasteryBasic, eSecSkillMagicResistance, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_GNOLL, CREATURE_BASILISK, CREATURE_SERPENT_FLY, "HPS112Bs.PCX", "HPL112Bs.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 5, THeroClass(14), eSecSkillDefense, eMasteryBasic, eSecSkillLeadership, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_GNOLL, CREATURE_GNOLL, CREATURE_GNOLL, "HPS113Bs.PCX", "HPL113Bs.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 9, THeroClass(14), eSecSkillDefense, eMasteryBasic, eSecSkillArchery, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_LIZARDMAN, CREATURE_LIZARDMAN, CREATURE_LIZARDMAN, "HPS114Bs.PCX", "HPL114Bs.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(14), eSecSkillDefense, eMasteryAdvanced, eSecSkillNone, eMasteryNone, 0, { 0, 0, 0 }, -1, CREATURE_GNOLL, CREATURE_LIZARDMAN, CREATURE_SERPENT_FLY, "HPS115Bs.PCX", "HPL115Bs.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 5, THeroClass(14), eSecSkillDefense, eMasteryBasic, eSecSkillOffense, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_GNOLL, CREATURE_LIZARDMAN, CREATURE_SERPENT_FLY, "HPS116Bs.PCX", "HPL116Bs.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 9, THeroClass(14), eSecSkillDefense, eMasteryBasic, eSecSkillPathfinding, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_GNOLL, CREATURE_SERPENT_FLY, CREATURE_SERPENT_FLY, "HPS117Bs.PCX", "HPL117Bs.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(14), eSecSkillDefense, eMasteryBasic, eSecSkillBattlefieldBallistics, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_GNOLL, CREATURE_BALLISTA, CREATURE_SERPENT_FLY, "HPS118Bs.PCX", "HPL118Bs.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 9, THeroClass(14), eSecSkillDefense, eMasteryBasic, eSecSkillScouting, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_GNOLL, CREATURE_LIZARDMAN, CREATURE_SERPENT_FLY, "HPS119Bs.PCX", "HPL119Bs.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(15), eSecSkillWisdom, eMasteryAdvanced, eSecSkillNone, eMasteryBasic, 1, { 0, 0, 0 }, 45, CREATURE_GNOLL, CREATURE_LIZARDMAN, CREATURE_SERPENT_FLY, "HPS120Wh.PCX", "HPL120Wh.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 5, THeroClass(15), eSecSkillWisdom, eMasteryBasic, eSecSkillMysticism, eMasteryBasic, 1, { 0, 0, 0 }, 15, CREATURE_GNOLL, CREATURE_LIZARDMAN, CREATURE_SERPENT_FLY, "HPS121Wh.PCX", "HPL121Wh.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 9, THeroClass(15), eSecSkillWisdom, eMasteryBasic, eSecSkillNavigation, eMasteryBasic, 1, { 0, 0, 0 }, 54, CREATURE_GNOLL, CREATURE_LIZARDMAN, CREATURE_SERPENT_FLY, "HPS122Wh.PCX", "HPL122Wh.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(15), eSecSkillWisdom, eMasteryBasic, eSecSkillFirstAid, eMasteryBasic, 1, { 0, 0, 0 }, 31, CREATURE_GNOLL, CREATURE_FIRST_AID_TENT, CREATURE_SERPENT_FLY, "HPS123Wh.PCX", "HPL123Wh.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 5, THeroClass(15), eSecSkillWisdom, eMasteryBasic, eSecSkillLearning, eMasteryBasic, 1, { 0, 0, 0 }, 46, CREATURE_GNOLL, CREATURE_LIZARDMAN, CREATURE_SERPENT_FLY, "HPS124Wh.PCX", "HPL124Wh.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 9, THeroClass(15), eSecSkillWisdom, eMasteryBasic, eSecSkillSorcery, eMasteryBasic, 1, { 0, 0, 0 }, 27, CREATURE_GNOLL, CREATURE_LIZARDMAN, CREATURE_SERPENT_FLY, "HPS125Wh.PCX", "HPL125Wh.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(15), eSecSkillWisdom, eMasteryBasic, eSecSkillIntelligence, eMasteryBasic, 1, { 0, 0, 0 }, 35, CREATURE_GNOLL, CREATURE_LIZARDMAN, CREATURE_SERPENT_FLY, "HPS126Wh.PCX", "HPL126Wh.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 5, THeroClass(15), eSecSkillWisdom, eMasteryBasic, eSecSkillEagleEye, eMasteryBasic, 1, { 0, 0, 0 }, 46, CREATURE_GNOLL, CREATURE_LIZARDMAN, CREATURE_SERPENT_FLY, "HPS127Wh.PCX", "HPL127Wh.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(16), eSecSkillOffense, eMasteryBasic, eSecSkillBattlefieldBallistics, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIXIE, CREATURE_AIR_ELEMENTAL, CREATURE_AIR_ELEMENTAL, "HPS000Pl.PCX", "HPL000pl.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(16), eSecSkillBattleTactics, eMasteryBasic, eSecSkillEstates, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIXIE, CREATURE_AIR_ELEMENTAL, CREATURE_WATER_ELEMENTAL, "HPS001pl.PCX", "HPL001pl.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(16), eSecSkillOffense, eMasteryBasic, eSecSkillBattlefieldBallistics, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIXIE, CREATURE_AIR_ELEMENTAL, CREATURE_WATER_ELEMENTAL, "HPS002pl.PCX", "HPL002pl.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(16), eSecSkillBattleTactics, eMasteryAdvanced, eSecSkillNone, eMasteryNone, 0, { 0, 0, 0 }, -1, CREATURE_PIXIE, CREATURE_WATER_ELEMENTAL, CREATURE_WATER_ELEMENTAL, "HPS003pl.PCX", "HPL003pl.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(16), eSecSkillOffense, eMasteryBasic, eSecSkillLogistics, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIXIE, CREATURE_AIR_ELEMENTAL, CREATURE_AIR_ELEMENTAL, "HPS004pl.PCX", "HPL004pl.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(16), eSecSkillBattleTactics, eMasteryBasic, eSecSkillEstates, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIXIE, CREATURE_AIR_ELEMENTAL, CREATURE_WATER_ELEMENTAL, "HPS005pl.PCX", "HPL005pl.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(16), eSecSkillOffense, eMasteryAdvanced, eSecSkillNone, eMasteryNone, 0, { 0, 0, 0 }, -1, CREATURE_PIXIE, CREATURE_AIR_ELEMENTAL, CREATURE_WATER_ELEMENTAL, "HPS006pl.PCX", "HPL006pl.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(16), eSecSkillBattleTactics, eMasteryBasic, eSecSkillLearning, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIXIE, CREATURE_WATER_ELEMENTAL, CREATURE_WATER_ELEMENTAL, "HPS007pl.PCX", "HPL007pl.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(17), eSecSkillWisdom, eMasteryBasic, eSecSkillSchoolOfFireMagic, eMasteryBasic, 1, { 0, 0, 0 }, 13, CREATURE_PIXIE, CREATURE_AIR_ELEMENTAL, CREATURE_WATER_ELEMENTAL, "HPS000el.PCX", "HPL000el.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(17), eSecSkillWisdom, eMasteryBasic, eSecSkillSchoolOfAirMagic, eMasteryBasic, 1, { 0, 0, 0 }, 53, CREATURE_PIXIE, CREATURE_AIR_ELEMENTAL, CREATURE_WATER_ELEMENTAL, "HPS001el.PCX", "HPL001el.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(17), eSecSkillWisdom, eMasteryBasic, eSecSkillSchoolOfWaterMagic, eMasteryBasic, 1, { 0, 0, 0 }, 15, CREATURE_PIXIE, CREATURE_AIR_ELEMENTAL, CREATURE_WATER_ELEMENTAL, "HPS002el.PCX", "HPL002el.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(17), eSecSkillWisdom, eMasteryBasic, eSecSkillSchoolOfEarthMagic, eMasteryBasic, 1, { 0, 0, 0 }, 46, CREATURE_PIXIE, CREATURE_AIR_ELEMENTAL, CREATURE_WATER_ELEMENTAL, "HPS003el.PCX", "HPL003el.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(17), eSecSkillWisdom, eMasteryBasic, eSecSkillSchoolOfFireMagic, eMasteryBasic, 1, { 0, 0, 0 }, 43, CREATURE_PIXIE, CREATURE_AIR_ELEMENTAL, CREATURE_WATER_ELEMENTAL, "HPS004el.PCX", "HPL004el.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(17), eSecSkillWisdom, eMasteryBasic, eSecSkillSchoolOfAirMagic, eMasteryBasic, 1, { 0, 0, 0 }, 47, CREATURE_PIXIE, CREATURE_AIR_ELEMENTAL, CREATURE_WATER_ELEMENTAL, "HPS005el.PCX", "HPL005el.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(17), eSecSkillWisdom, eMasteryBasic, eSecSkillSchoolOfWaterMagic, eMasteryBasic, 1, { 0, 0, 0 }, 35, CREATURE_PIXIE, CREATURE_AIR_ELEMENTAL, CREATURE_WATER_ELEMENTAL, "HPS006el.PCX", "HPL006el.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(17), eSecSkillWisdom, eMasteryBasic, eSecSkillSchoolOfEarthMagic, eMasteryBasic, 1, { 0, 0, 0 }, 54, CREATURE_PIXIE, CREATURE_AIR_ELEMENTAL, CREATURE_WATER_ELEMENTAL, "HPS007el.PCX", "HPL007el.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(0), eSecSkillLeadership, eMasteryAdvanced, eSecSkillNone, eMasteryNone, 0, { 0, 0, 0 }, -1, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS130Kn.PCX", "HPL130Kn.PCX", { 256 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(15), eSecSkillWisdom, eMasteryBasic, eSecSkillSchoolOfFireMagic, eMasteryExpert, 1, { 0, 0, 0 }, 22, CREATURE_GNOLL, CREATURE_LIZARDMAN, CREATURE_SERPENT_FLY, "HPS000Sh.PCX", "HPL000Sh.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(0), eSecSkillLeadership, eMasteryBasic, eSecSkillOffense, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS128Qc.PCX", "HPL128Qc.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(5), eSecSkillWisdom, eMasteryAdvanced, eSecSkillNone, eMasteryNone, 1, { 0, 0, 0 }, 53, CREATURE_ENCHANTER, CREATURE_ENCHANTER, CREATURE_ENCHANTER, "HPS003Sh.PCX", "HPL003Sh.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 3, THeroClass(2), eSecSkillLeadership, eMasteryBasic, eSecSkillArchery, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_SHARPSHOOTER, CREATURE_SHARPSHOOTER, CREATURE_SHARPSHOOTER, "HPS004Sh.PCX", "HPL004Sh.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(12), eSecSkillOffense, eMasteryAdvanced, eSecSkillNone, eMasteryNone, 0, { 0, 0, 0 }, -1, CREATURE_GOBLIN, CREATURE_WOLF_RIDER, CREATURE_ORC, "HPS005Sh.PCX", "HPL005Sh.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(8), eSecSkillNecromancy, eMasteryAdvanced, eSecSkillNone, eMasteryNone, 1, { 0, 0, 0 }, 54, CREATURE_SKELETON, CREATURE_WALKING_DEAD, CREATURE_WIGHT, "HPS006Sh.PCX", "HPL006Sh.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(10), eSecSkillBattleTactics, eMasteryBasic, eSecSkillEstates, eMasteryBasic, 1, { 0, 0, 0 }, 15, CREATURE_TROGLODYTE, CREATURE_HARPY, CREATURE_BEHOLDER, "HPS007Sh.PCX", "HPL007Sh.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(0), eSecSkillLeadership, eMasteryBasic, eSecSkillDefense, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS009Sh.PCX", "HPL009Sh.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 7, THeroClass(10), eSecSkillBattleTactics, eMasteryBasic, eSecSkillEstates, eMasteryBasic, 1, { 0, 0, 0 }, 15, CREATURE_TROGLODYTE, CREATURE_HARPY, CREATURE_BEHOLDER, "HPS008Sh.PCX", "HPL008Sh.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(12), eSecSkillOffense, eMasteryBasic, eSecSkillBattleTactics, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_GOBLIN, CREATURE_WOLF_RIDER, CREATURE_ORC, "HPS001Sh.PCX", "HPL001Sh.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(6), eSecSkillLeadership, eMasteryBasic, eSecSkillBattleTactics, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_IMP, CREATURE_HELL_HOUND, CREATURE_HELL_HOUND, "HPS131Dm.PCX", "HPL131Dm.PCX", { 65792 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    // Seven additional portrait records follow the 156 playable heroes.
    { 1, 7, THeroClass(0), eSecSkillLeadership, eMasteryBasic, eSecSkillOffense, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS129Mk.PCX", "HPL129Mk.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(0), eSecSkillLeadership, eMasteryBasic, eSecSkillOffense, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS002Sh.PCX", "HPL002Sh.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(0), eSecSkillLeadership, eMasteryBasic, eSecSkillOffense, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS132Wl.PCX", "HPL132Wl.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(0), eSecSkillLeadership, eMasteryBasic, eSecSkillOffense, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS133Nc.PCX", "HPL133Nc.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(0), eSecSkillLeadership, eMasteryBasic, eSecSkillOffense, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS134Nc.PCX", "HPL134Nc.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(0), eSecSkillLeadership, eMasteryBasic, eSecSkillOffense, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS135Wi.PCX", "HPL135Wi.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 7, THeroClass(0), eSecSkillLeadership, eMasteryBasic, eSecSkillOffense, eMasteryBasic, 0, { 0, 0, 0 }, -1, CREATURE_PIKEMAN, CREATURE_ARCHER, CREATURE_GRIFFIN, "HPS136Wi.PCX", "HPL136Wi.PCX", { 257 }, { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0 },
};
DATA(0x0067d868)
THeroClassTraits g_heroClassTraits[kNumHeroClasses] = {
    { 0, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 0, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 1, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 1, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 2, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 2, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 3, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 3, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 4, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 4, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 5, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 5, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 6, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 6, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 7, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 7, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 8, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
    { 8, 0, 0.0f, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0 } },
};

DATA(0x00698cf0) TSSkillTraits g_sSkillTraitsStorage[kNumSecSkills];
DATA(0x0067dce8) const THeroTraits (&akHeroTraits)[163] = g_heroTraitsStorage;
DATA(0x0067dcec) const THeroClassTraits (&akHeroClassTraits)[18] = g_heroClassTraits;
DATA(0x0067dcf0) const TSSkillTraits (&akSSkillTraits)[kNumSecSkills] = g_sSkillTraitsStorage;

namespace {

// CodeView field pStr; each loader owns its own private string class.
class TAutoStrPtr {
public:

    // E:\gamedcs\herodefs.cpp:391
    DC_ADDRESS(0x0d60d4, 0x8)
    TAutoStrPtr() : m_string(0) {}

    // E:\gamedcs\herodefs.cpp:394
    DC_ADDRESS(0x0d60dc, 0x18)
    ~TAutoStrPtr() { delete[] m_string; }

    // E:\gamedcs\herodefs.cpp:396
    DC_ADDRESS(0x0d60f4, 0x4)
    void set(char* value) { m_string = value; }

    // E:\gamedcs\herodefs.cpp:398
    DC_ADDRESS(0x0d60f8, 0x4)
    char* get() const { return m_string; }

private:
    char* m_string;
};

}

static void InitializeHeroTraits(int id, const TSpreadsheetResource::TStringVector& values);
static void InitializeHeroClassTraits(int id, const TSpreadsheetResource::TStringVector& values);
static void InitializeSSkillTraits(int id, const TSpreadsheetResource::TStringVector& values);

VA(0x004e67a0, 0x176)
DC_ADDRESS(0x0d5a40, 0x72)
MAC_ADDRESS(0x1077f8, 0xd4)
unsigned char InitializeHeroTraitsTable()
{
    TSpreadsheetResource* resource = ResourceManager::GetSpreadsheet(
        DATA_COMPGEN(0x0067f154, heroTraitsSpreadsheetName,
                     "hotraits.txt"));
    if (!resource)
        return 0;

    if (resource->GetNumberOfRows() < 158) {
        ResourceManager::Dispose(resource);
        return 0;
    }

    int id = 0;
    int row = 2;
    for (; id < 156; ++id, ++row) {
        InitializeHeroTraits(id, resource->GetRow(row));
    }

    ResourceManager::Dispose(resource);
    return 1;
}

VA(0x004e6920, 0x1E2)
DC_ADDRESS(0x0d5ab4, 0x72)
MAC_ADDRESS(0x1078cc, 0xd4)
bool InitializeHeroClassTraitsTable()
{
    TSpreadsheetResource* resource = ResourceManager::GetSpreadsheet(
        DATA_COMPGEN(0x0067f164, heroClassTraitsSpreadsheetName,
                     "hctraits.txt"));
    if (!resource)
        return 0;

    if (resource->GetNumberOfRows() < 20) {
        ResourceManager::Dispose(resource);
        return 0;
    }

    int id = 0;
    int row = 2;
    for (; id < kNumHeroClasses; ++id, ++row) {
        InitializeHeroClassTraits(id, resource->GetRow(row));
    }

    ResourceManager::Dispose(resource);
    return 1;
}

VA(0x004e6b10, 0x1C8)
DC_ADDRESS(0x0d5b28, 0x98)
MAC_ADDRESS(0x1079a0, 0xd4)
bool InitializeSSkillTraitsTable()
{
    TSpreadsheetResource* resource = ResourceManager::GetSpreadsheet(
        DATA_COMPGEN(0x0067f174, secondarySkillTraitsSpreadsheetName,
                     "sstraits.txt"));
    if (!resource)
        return 0;

    if (resource->GetNumberOfRows() < 30) {
        ResourceManager::Dispose(resource);
        return 0;
    }

    int id = 0;
    int row = 2;
    for (; id < kNumSecSkills; ++id, ++row) {
        InitializeSSkillTraits(id, resource->GetRow(row));
    }

    ResourceManager::Dispose(resource);
    return 1;
}

// The DC table loaders call these ordinary static functions. Complete's
// table bodies retain the same row parsing and private string ownership,
// including the expanded static initialization and destruction families.
// Original: InitializeHeroTraits; herodefs.cpp:409
DC_ADDRESS(0x0d5bc0, 0x10c)
MAC_ADDRESS(0x107a74, 0x134)
static void InitializeHeroTraits(int id, const TSpreadsheetResource::TStringVector& values)
{
    THeroTraits& traits = g_heroTraitsStorage[id];

    DATA_COMPGEN_GUARD(0x00698b99, heroStringsGuard, heroStrings)
    DATA(0x00698eb0)
    static TAutoStrPtr heroStrings[156];

    heroStrings[id].set(new char[strlen(values[0]) + 1]);
    strcpy(heroStrings[id].get(), values[0]);

    traits.m_defaultName = heroStrings[id].get();
    traits.m_1stStackLow = atoi(values[1]);
    traits.m_1stStackHigh = atoi(values[2]);
    traits.m_2ndStackLow = atoi(values[4]);
    traits.m_2ndStackHigh = atoi(values[5]);
    traits.m_3rdStackLow = atoi(values[7]);
    traits.m_3rdStackHigh = atoi(values[8]);
}

// Original: InitializeHeroClassTraits; herodefs.cpp:441
DC_ADDRESS(0x0d5d28, 0x1a6)
MAC_ADDRESS(0x107c38, 0x1d8)
static void InitializeHeroClassTraits(int id, const TSpreadsheetResource::TStringVector& values)
{
    THeroClassTraits& traits = g_heroClassTraits[id];

    DATA_COMPGEN_GUARD(0x00698b9a, heroClassStringsGuard,
                      heroClassStrings)
    DATA(0x00699120)
    static TAutoStrPtr heroClassStrings[kNumHeroClasses];

    heroClassStrings[id].set(new char[strlen(values[0]) + 1]);
    strcpy(heroClassStrings[id].get(), values[0]);
    traits.m_name = heroClassStrings[id].get();
    traits.m_aggression = static_cast<float>(atof(values[1]));

    int column;
    for (column = 0; column < kNumPrimarySkills; ++column)
        traits.m_initialPrimarySkill[column] =
            static_cast<signed char>(atoi(values[column + 2]));
    for (column = 0; column < kNumPrimarySkills; ++column)
        traits.m_gainPrimarySkillChance[column] =
            static_cast<signed char>(atoi(values[column + 6]));
    for (column = 0; column < kNumPrimarySkills; ++column)
        traits.m_gainPrimarySkillChance10P[column] =
            static_cast<signed char>(atoi(values[column + 10]));
    for (column = 0; column < kNumSecSkills; ++column)
        traits.m_gainSecondarySkillChance[column] =
            static_cast<signed char>(atoi(values[column + 14]));
    for (column = 0; column < TOWN_TYPE_COUNT; ++column)
        traits.m_foundInTownType[column] =
            static_cast<signed char>(atoi(values[column + 42]));
}

// Original: InitializeSSkillTraits; herodefs.cpp:489
DC_ADDRESS(0x0d5ee8, 0x194)
MAC_ADDRESS(0x107e40, 0x16c)
static void InitializeSSkillTraits(int id, const TSpreadsheetResource::TStringVector& values)
{
    TSSkillTraits& traits = g_sSkillTraitsStorage[id];

    DATA_COMPGEN_GUARD(0x00698b98, secondarySkillStringsGuard,
                      secondarySkillNames)
    DATA(0x00698b28)
    static TAutoStrPtr secondarySkillNames[kNumSecSkills];

    secondarySkillNames[id].set(new char[strlen(values[0]) + 1]);
    strcpy(secondarySkillNames[id].get(), values[0]);
    traits.m_name = secondarySkillNames[id].get();

    DATA(0x00698b9c)
    static TAutoStrPtr secondarySkillLevelNames[kNumSecSkills][3];

    int level;
    for (level = 0; level < 3; ++level) {
        secondarySkillLevelNames[id][level].set(
            new char[strlen(values[level + 1]) + 1]);
        strcpy(secondarySkillLevelNames[id][level].get(),
               values[level + 1]);
        traits.m_levelNames[level] =
            secondarySkillLevelNames[id][level].get();
    }
}
