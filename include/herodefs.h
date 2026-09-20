#ifndef HOMM3_HERODEFS_H
#define HOMM3_HERODEFS_H

#include "sskilltraits.h"

unsigned char initializeHeroTraitsTable();
// DC publics end in `_N`, the MSVC mangling for bool.
bool initializeHeroClassTraitsTable();
bool initializeSSkillTraitsTable();

#endif  /* HOMM3_HERODEFS_H */
