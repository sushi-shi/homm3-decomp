// primaryskill.h - shared primary-skill domain.
#ifndef HOMM3_PRIMARYSKILL_H
#define HOMM3_PRIMARYSKILL_H

#include "domains.h"

// Dreamcast LF_ENUM TPrimarySkill (enums.csv). The scholar's packed
// primary lane and Random(0, 3) in retail RandomizeEvents corroborate
// the four stat ordinals. Original enumerator spellings are retained.
H3_ENUM_BEGIN(TPrimarySkill)
    ePriSkillAttack = 0,
    ePriSkillDefense = 1,
    ePriSkillPower = 2,
    ePriSkillKnowledge = 3,
    kNumPrimarySkills = 4,
    kMaxPrimarySkillLevel = 99
H3_ENUM_END(TPrimarySkill)

#endif /* HOMM3_PRIMARYSKILL_H */
