// sskilltraits.h - canonical secondary-skill trait table ABI.
#ifndef HOMM3_SSKILLTRAITS_H
#define HOMM3_SSKILLTRAITS_H

#include <va.h>

// The 28 secondary-skill rows loaded by herodefs.obj. Rollover text
// independently proves the 16-byte stride and name at +0; retail's loader
// fills the three mastery strings at +4/+8/+c.
struct TSSkillTraits {
    // Before normalization: name.
    const char* m_name;
    // Before normalization: levelNames.
    const char* m_levelNames[3];
};
SIZE(TSSkillTraits, 0x10);

// Before normalization: aSSkillTraits.
// Before normalization: akSSkillTraits.
DATA(0x00698cf0) extern TSSkillTraits g_sSkillTraitsStorage[28];
DATA(0x0067dcf0) extern const TSSkillTraits (&g_sSkillTraits)[28];

#endif  /* HOMM3_SSKILLTRAITS_H */
