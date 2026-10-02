// Shared Complete-only random-map request and result contract.
#ifndef HOMM3_RMG_REQUEST_H
#define HOMM3_RMG_REQUEST_H

#include "va.h"
#include "homm3_bool.h"
class TAbstractFile;
class TProgressSink;

// Retail 0x54c090 returns these codes; GenerateRandomMap maps failures
// through a four-entry jump table to general-text rows.
enum ERandomMapResult {
    RANDOM_MAP_OK = 0,
    RANDOM_MAP_OPEN_FAILED = 1,
    RANDOM_MAP_WRITE_FAILED = 2,
    RANDOM_MAP_GENERATION_FAILED = 3
};

// Shared lobby/generator request. Constructor 0x54bf00 and the lobby stores
// prove the layout; field names describe those roles.
class TRandomMapRequest {
public:
    // Lobby human seats are 1; computer seats remain zero.
    b8 m_isHumanSeat[8];   // +0x00
    // -1 selects a random town.
    int m_townType[8];                // +0x08
    int m_width;                      // +0x28
    int m_height;                     // +0x2c
    int m_levels;                     // +0x30
    // Retail 0x54bf60 forwards these to generator ctor 0x537b10, adding 3
    // to monster strength and clamping it to [1, 5].
    int m_humanPlayerCount;           // +0x34
    int m_humanTeamCount;             // +0x38
    int m_computerPlayerCount;        // +0x3c
    int m_computerTeamCount;          // +0x40
    int m_waterContent;               // +0x44
    int m_monsterStrength;            // +0x48
    // 0/1/2 - the map-format class the running game context implies (the
    // same ordinals EGameVersion carries).
    int m_mapVersion;                 // +0x4c

    TRandomMapRequest(int width, int height, int levels);
    // The optional progress sink is borrowed. generateToFile changes both
    // player counts to one when their sum is below two, even on later failure.
    int generate(const char* fileName, TProgressSink* progress);
    int generateToFile(TAbstractFile* outputFile, TProgressSink* progress);
};
SIZE(TRandomMapRequest, 0x50);

#endif
