#ifndef HOMM3_HERODEFS_H
#define HOMM3_HERODEFS_H

#include "sskilltraits.h"

unsigned char InitializeHeroTraitsTable();
// DC publics end in `_N`, the MSVC mangling for bool.
bool InitializeHeroClassTraitsTable();
bool InitializeSSkillTraitsTable();

#endif  /* HOMM3_HERODEFS_H */
