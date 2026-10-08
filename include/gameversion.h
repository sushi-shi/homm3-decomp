// gameversion.h - the product generation, Restoration of Erathia,
// Armageddon's Blade or Shadow of Death: the saved game's header records it,
// and the map editor's TGameMap keeps the same three values at +8.
#ifndef HOMM3_GAMEVERSION_H
#define HOMM3_GAMEVERSION_H

// Product generation recorded in SavedGameHeader::gameVersion.  The save
// loader derives the same three rungs from the on-disk format version when an
// older header does not carry the field explicitly.
enum EGameVersion {
    GAME_VERSION_ROE = 0,
    GAME_VERSION_AB = 1,
    GAME_VERSION_SOD = 2
};

#endif  // HOMM3_GAMEVERSION_H
