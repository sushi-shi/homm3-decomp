// sskilltraits.h - the secondary-skill traits record.
#ifndef HOMM3_SSKILLTRAITS_H
#define HOMM3_SSKILLTRAITS_H

#include "secondaryskill.h"

// The 28 secondary-skill rows HeroDefs.cpp fills from sstraits.txt: the
// skill name and one description per mastery level (16 bytes).
struct TSSkillTraits {
    const char* m_name;
    const char* m_levelNames[3];
};

extern const TSSkillTraits (&akSSkillTraits)[kNumSecSkills];

#endif  /* HOMM3_SSKILLTRAITS_H */
