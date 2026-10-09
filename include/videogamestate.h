// videogamestate.h - the values the code tests on g_videoGameState, the
// installed-game context (gamecontext.h).
#ifndef HOMM3_VIDEOGAMESTATE_H
#define HOMM3_VIDEOGAMESTATE_H

// Byte-derived; the pointee's real domain arrives with its owning TU, and
// the names are provisional role names. Two disjoint pairs are attested on
// this global. {2, 3} force VIDEO_ID_STATE_GATED onto the bink arm
// (VideoPlay / VideoOpen); {1, 3} open the h3ab_ahd expansion archives
// (LoadAnimHeaders / LoadSoundHeaders). Value 3 is a member of both sets,
// so it keeps the name the bink gate gave it. The map editor offers the
// Conflux alignment only under value 3 (h3maped 0x473db5).
enum EVideoGameState {
    VIDEO_GAME_STATE_EXPANSION_ARCHIVES = 0x1,
    VIDEO_GAME_STATE_FORCED_BINK_LOW = 0x2,
    VIDEO_GAME_STATE_FORCED_BINK_HIGH = 0x3
};

#endif  // HOMM3_VIDEOGAMESTATE_H
