// savegame.h - the save-file plumbing game::SaveGame (0x4beea0) needs.

// NEITHER DECLARATION HAS A PROVEN OWNER COMPILAND, which is why they
// are here rather than in an existing header: TGzFile's three bodies sit
// in the gametypewindow..hero link-order bracket with four candidate
// units and no Dreamcast row, and the save-slot number below has
// eighteen readers spread across the image with none of them modelled.
// This narrow domain header now serves the two reconstructed consumers,
// game.cpp and kb.cpp, without widening game.h's include-set wall. Move a
// declaration to its real owner header once that owner is proven.
#ifndef HOMM3_SAVEGAME_H
#define HOMM3_SAVEGAME_H

#include "gzfile.h"

// Retail .bss 0x699274. game::SaveGame formats it into "%s.GM%d" for an
// ordinary (non-campaign, non-tutorial) save, which is what makes the
// familiar .GM1 / .GM2 extensions - so it is a save-slot or player-count
// selector. NAME UNATTESTED, address-ordinal placeholder in the
// gUnnamed69ccc4 style; eighteen .text sites read it and none of them is
// modelled yet, so nothing constrains the role further.
extern int g_unnamed699274;

#endif  /* HOMM3_SAVEGAME_H */
