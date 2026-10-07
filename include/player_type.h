// player_type.h - shared player-slot domain.
#ifndef HOMM3_PLAYER_TYPE_H
#define HOMM3_PLAYER_TYPE_H

// Red through pink: the length of game::players and playerDead, of the map
// header's per-player tables and of every per-player loop. No Dreamcast
// enum carries the count (NH3API EPlayerColor spells it MAX_PLAYERS, the
// name CHotSeatMan and CNetPlayerHandler use for their own members).
enum { NUM_PLAYERS = 8 };

#endif  // HOMM3_PLAYER_TYPE_H
