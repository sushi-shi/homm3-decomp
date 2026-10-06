// sskilltraits.h - canonical secondary-skill trait table ABI.
#ifndef HOMM3_SSKILLTRAITS_H
#define HOMM3_SSKILLTRAITS_H

#include "va.h"

#include "secondaryskill.h"

// The 28 secondary-skill rows loaded by herodefs.obj. Rollover text
// independently proves the 16-byte stride and name at +0; retail's loader
// fills the three mastery strings at +4/+8/+c.
struct TSSkillTraits {
    const char* m_name;
    const char* m_levelNames[3];
};
SIZE(TSSkillTraits, 0x10);

extern TSSkillTraits g_sSkillTraitsStorage[kNumSecSkills];
extern const TSSkillTraits (&akSSkillTraits)[kNumSecSkills];

#endif  /* HOMM3_SSKILLTRAITS_H */
