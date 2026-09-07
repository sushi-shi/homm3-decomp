// Narrow cross-TU globals owned by multiplayerwindow.obj.
#ifndef HOMM3_MULTIPLAYERWINDOW_GLOBALS_H
#define HOMM3_MULTIPLAYERWINDOW_GLOBALS_H

// multiplayerwindow.obj owns the persisted multiplayer save name at
// 0x69880a; game::TransmitSaveGame is its cross-TU reader.
// Before normalization: gLoadedGameName.
extern char g_loadedGameName[13];

#endif /* HOMM3_MULTIPLAYERWINDOW_GLOBALS_H */
