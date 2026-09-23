#ifndef HOMM3_NEWGAME_H
#define HOMM3_NEWGAME_H

#include "town.h"

// ShowScenInfo consumes the campaign dialog's 111 reply and translates it
// into the shared main-loop quit command stored at 0x6976d8. The storage is
// claimed by advmgr.cpp; this is the owning new-game consumer declaration.
extern int g_gameCommand;

enum ENewGameDialogCommand {
    NEWGAME_CAMPAIGN_BRIEF_EXIT = 111,
    NEWGAME_COMMAND_QUIT = 107
};

long getAlignmentCount(int legalAlignments);
TTownType pickAlignment(int legalAlignments,
                         unsigned char getFirstAvail);

// Definitions belong to newgame.cpp. Complete widens the alignment mask
// for Conflux; the advanced-options click handler uses the nine-town loops.
TTownType pickPrevAlignment(int legalAlignments, TTownType type);

TTownType pickNextAlignment(int legalAlignments, TTownType type);

// The seven resource names (retail 0x6a5e64, DATA-claimed by seerhut.cpp);
// GetVictoryConditionText's resource arm formats one. Consumer-side plain
// extern, the advmgr.h / ai_player.h pattern.
extern const char* g_resourceNames[8];
// The nine map-region names (retail 0x6a5c48, DATA-claimed by seerhut.cpp;
// game.h exposes them only to its own view). The defeat-monster arm of
// GetVictoryConditionText indexes them by the map-third direction.

#endif  /* HOMM3_NEWGAME_H */
