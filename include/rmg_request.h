// Random-map request and result shared by the lobby and the generator.
#ifndef HOMM3_RMG_REQUEST_H
#define HOMM3_RMG_REQUEST_H

#include "va.h"
#include "homm3_bool.h"
#include "homm3_int.h"
#include "town_type.h"
class TAbstractFile;
class TProgressSink;

// Generation result; the lobby shows a general-text message for each failure.
enum ERandomMapResult {
    RANDOM_MAP_OK = 0,
    RANDOM_MAP_OPEN_FAILED = 1,
    RANDOM_MAP_WRITE_FAILED = 2,
    RANDOM_MAP_GENERATION_FAILED = 3
};

// Player colours (red, blue, tan, green, orange, purple, teal, pink).
enum ERmgPlayerLimits {
    RMG_PLAYER_COUNT = 8
};

// Requested water content; the generator resolves RANDOM to one of the
// other three.
enum ERmgWaterContent {
    RMG_WATER_NONE = 0,
    RMG_WATER_NORMAL = 1,
    RMG_WATER_ISLANDS = 2,
    RMG_WATER_RANDOM = 3
};

enum ERmgMapVersion {
    RMG_MAP_RESTORATION_OF_ERATHIA = 0,
    RMG_MAP_ARMAGEDDONS_BLADE = 1,
    RMG_MAP_SHADOW_OF_DEATH = 2
};

// Random-map settings chosen in the lobby and passed to the generator.
class TRandomMapRequest {
public:
    // Lobby human seats are 1; computer seats remain zero.
    b8 m_isHumanSeat[RMG_PLAYER_COUNT];  // +0x00
    // -1 selects a random town.
    TTownType m_townChoices[RMG_PLAYER_COUNT]; // +0x08
    // Map size in tiles; the lobby passes its map dimension for both.
    s32 m_width;                         // +0x28
    s32 m_height;                        // +0x2c
    // Map levels; the second is underground.
    s32 m_levels;                        // +0x30
    // A team count of 0 gives each player its own team.
    s32 m_humanPlayerCount;              // +0x34
    s32 m_humanTeamCount;                // +0x38
    s32 m_computerPlayerCount;           // +0x3c
    s32 m_computerTeamCount;             // +0x40
    // RMG_WATER_RANDOM by default.
    ERmgWaterContent m_waterContent;     // +0x44
    // 0 is normal. The generator receives it plus RMG_ZONE_MONSTERS_AVERAGE
    // (3), clamped to [1, 5].
    s32 m_monsterStrength;               // +0x48
    // Map format 0/1/2, using the EGameVersion ordinals.
    ERmgMapVersion m_mapVersion;         // +0x4c

    TRandomMapRequest(s32 width, s32 height, s32 levels);
    // The optional progress sink is borrowed. generateToFile changes both
    // player counts to one when their sum is below two, even on later failure.
    ERandomMapResult generate(const char* fileName, TProgressSink* progress);
    ERandomMapResult generateToFile(TAbstractFile* outputFile, TProgressSink* progress);
#if defined(HOMM3_RMG_HOTFIX)
    // Settings the lobby can produce; the generator relies on them.
    bool isSupported() const;
#endif
};
SIZE(TRandomMapRequest, 0x50);

#endif
