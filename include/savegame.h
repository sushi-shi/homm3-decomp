// savegame.h - the save-slot selector used by the game and keyboard modules.
#ifndef HOMM3_SAVEGAME_H
#define HOMM3_SAVEGAME_H

#include "gzfile.h"

// Retail .bss 0x699274. game::SaveGame formats it into "%s.GM%d" for an
// ordinary (non-campaign, non-tutorial) save, which is what makes the
// familiar .GM1 / .GM2 extensions - so it is a save-slot or player-count
// selector. NAME UNATTESTED, address-ordinal placeholder in the
// gUnnamed69ccc4 style; eighteen .text sites read it and none of them is
// modelled yet, so nothing constrains the role further.
// Before normalization: gUnnamed699274.
extern int g_unnamed699274;

#endif  /* HOMM3_SAVEGAME_H */
