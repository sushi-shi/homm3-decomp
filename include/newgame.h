// newgame.h - prototypes of newgame.cpp (compiland newgame.obj)
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

// Dreamcast keeps these two tiny helpers out of line. Complete widens the
// alignment mask for Conflux; VC6 expands both helpers into the advanced-
// options click handler, where the complete nine-town loops are visible.
inline TTownType pickPrevAlignment(int legalAlignments, TTownType type)
{
    do {
        type = static_cast<TTownType>(type - 1) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */;
        if (type < -1)
            type = TOWN_CONFLUX;
        else if (type == -1)
            break;
    } while (!(legalAlignments & (1 << type)));
    return type;
}

inline TTownType pickNextAlignment(int legalAlignments, TTownType type)
{
    do {
        type = static_cast<TTownType>(type + 1) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */;
        if (type > TOWN_CONFLUX)
            type = static_cast<TTownType>(-1) /* HOMM3_ENUM_CAST_REVISION_BOUNDARY */;
    } while (type != -1 && !(legalAlignments & (1 << type)));
    return type;
}

// The seven resource names (retail 0x6a5e64, DATA-claimed by seerhut.cpp);
// GetVictoryConditionText's resource arm formats one. Consumer-side plain
// extern, the advmgr.h / ai_player.h pattern.
extern const char* g_resourceNames[7];
// The nine map-region names (retail 0x6a5c48, DATA-claimed by seerhut.cpp;
// game.h exposes them only to its own view). The defeat-monster arm of
// GetVictoryConditionText indexes them by the map-third direction.
extern const char* g_questMonsterDirections[9];

// --- globals ---
// CODEVIEW(E:\gamedcs\newgame.cpp:355, dc 0x1037f8) TTownType pick_prev_alignment(unsigned char legal_alignments, TTownType type);
// CODEVIEW(E:\gamedcs\newgame.cpp:368, dc 0x10380c) TTownType pick_next_alignment(unsigned char legal_alignments, TTownType type);

// --- game ---
// CODEVIEW(E:\gamedcs\newgame.cpp:337, dc 0x1037f4) void game::SetupNetPlayerNames();
// CODEVIEW(E:\gamedcs\newgame.cpp:826, dc 0x103fb4) int game::GetSideDesc(char* rText, int iStartPos, int iEndPos);

#endif  /* HOMM3_NEWGAME_H */
