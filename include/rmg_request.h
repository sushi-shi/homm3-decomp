// Shared Complete-only random-map request and result contract.
#ifndef HOMM3_RMG_REQUEST_H
#define HOMM3_RMG_REQUEST_H

#include "va.h"
class AbstractFile;

// The generator's result code (retail 0x54c090's return, dispatched through
// GenerateRandomMap's four-entry jump table). Zero is success; each failure
// rung selects one general-text row. Ordinal names - no symbol survives.
enum ERandomMapResult {
    RANDOM_MAP_OK = 0,
    RANDOM_MAP_FAILED_1 = 1,
    RANDOM_MAP_FAILED_2 = 2,
    RANDOM_MAP_FAILED_3 = 3
};

// The Complete-only random-map request record GenerateRandomMap fills and
// hands to the generator entry at 0x54c090. Every offset below is fixed by
// the constructor at 0x54bf00 (`ret 0xc`, so three stack arguments) and by
// the window's own field stores; the constructor's own defaults are
// field_34 = 2, field_38 = 2, field_3C = 0, field_40 = 8, field_44 = 3,
// field_48 = 0 and mapVersion = 2. Named where the caller contract proves a
// role, ORDINAL otherwise - no symbol survives for this type. It is shared
// through this header for the selection window and the RMG implementation.
// Before normalization (type): TRandomMapRequest.
class RandomMapRequest {
public:
    // Set to 1 for every seat the lobby has a live player record for; the
    // constructor zeroes both dwords.
    unsigned char m_isHumanSeat[8];   // +0x00
    // The eight seats' chosen town, -1 for random (constructor fill).
    int m_townType[8];                // +0x08
    int m_width;                      // +0x28
    int m_height;                     // +0x2c
    int m_levels;                     // +0x30
    // Retail 0x54bf60 passes these six slots to generator ctor 0x537b10:
    // +34/+38/+3c/+40 become +f48/+f4c/+f50/+f54 (player/team counts);
    // +44 becomes +10b8 (waterContent), and clamp(+48+3,1,5) becomes
    // +10bc (monsterStrength). Generator consumers corroborate these roles.
    // Semantic names follow the generator; original request names unknown.
    int m_humanPlayerCount;                   // +0x34
    int m_humanTeamCount;                   // +0x38
    int m_computerPlayerCount;                   // +0x3c
    int m_computerTeamCount;                   // +0x40
    int m_waterContent;                   // +0x44
    int m_monsterStrength;                   // +0x48
    // 0/1/2 - the map-format class the running game context implies (the
    // same ordinals EGameVersion carries).
    int m_mapVersion;                 // +0x4c

    RandomMapRequest(int width, int height, int levels);
    int generate(const char* fileName, void* progress);
    int generateToFile(AbstractFile* outfile, void* progress);
};
SIZE(RandomMapRequest, 0x50);

#endif
