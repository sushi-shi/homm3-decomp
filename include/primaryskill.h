// primaryskill.h - shared primary-skill domain.
#ifndef HOMM3_PRIMARYSKILL_H
#define HOMM3_PRIMARYSKILL_H

// Dreamcast LF_ENUM TPrimarySkill (enums.csv). The scholar's packed
// primary lane and Random(0, 3) in retail RandomizeEvents corroborate
// the four stat ordinals. Original enumerator spellings are retained.
enum TPrimarySkill {
    ePriSkillAttack = 0,
    ePriSkillDefense = 1,
    ePriSkillPower = 2,
    ePriSkillKnowledge = 3,
    kNumPrimarySkills = 4,
    kMaxPrimarySkillLevel = 99
};

// Scalar bridge from retail's signed packed field / Random result to
// the recovered four-byte enum ABI, including negative sentinel values.
inline TPrimarySkill primarySkillFromInt(int value)
{
    union {
        int m_integer;
        TPrimarySkill m_skill;
    } converted;
    converted.m_integer = value;
    return converted.m_skill;
}

#endif /* HOMM3_PRIMARYSKILL_H */
