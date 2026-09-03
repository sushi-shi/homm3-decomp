// secondaryskill.h - shared secondary-skill domain.
#ifndef HOMM3_SECONDARYSKILL_H
#define HOMM3_SECONDARYSKILL_H

// DC LF_ENUM `TSecondarySkill`, transcribed from
// evidence/dreamcast/enums.csv (31 enumerators; the DC spellings, minus
// aliases that name no distinct skill). Retail corroborates the domain in
// hero specialty lookups and type_university's four elemental-school slots.
enum TSecondarySkill {
    eSecSkillNone = -1,
    kNumSecSkillsPerHero = 8,
    eSecSkillPathfinding = 0,
    eSecSkillArchery = 1,
    eSecSkillLogistics = 2,
    eSecSkillScouting = 3,
    eSecSkillDiplomacy = 4,
    eSecSkillNavigation = 5,
    eSecSkillLeadership = 6,
    eSecSkillWisdom = 7,
    eSecSkillMysticism = 8,
    eSecSkillLuck = 9,
    eSecSkillSiegeBallistics = 10,
    eSecSkillEagleEye = 11,
    eSecSkillNecromancy = 12,
    eSecSkillEstates = 13,
    eSecSkillSchoolOfFireMagic = 14,
    eSecSkillSchoolOfAirMagic = 15,
    eSecSkillSchoolOfWaterMagic = 16,
    eSecSkillSchoolOfEarthMagic = 17,
    eSecSkillMagicScholar = 18,
    eSecSkillBattleTactics = 19,
    eSecSkillBattlefieldBallistics = 20,
    eSecSkillLearning = 21,
    eSecSkillOffense = 22,
    eSecSkillDefense = 23,
    eSecSkillIntelligence = 24,
    eSecSkillSorcery = 25,
    eSecSkillMagicResistance = 26,
    eSecSkillFirstAid = 27,
    kNumSecSkills = 28
};

#endif  /* HOMM3_SECONDARYSKILL_H */
