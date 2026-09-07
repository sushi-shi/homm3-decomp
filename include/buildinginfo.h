#ifndef HOMM3_BUILDINGINFO_H
#define HOMM3_BUILDINGINFO_H

// The building-description tables read by townmgr.cpp's GetBuildingInfo,
// one per building-id band, mirroring castle.h's gBuildingNames* name
// tables (identical type*11 / type*14 strides) with a per-town-type
// blacksmith column and the Rampart custom-building columns. The owning
// data TU is unlocated; these are consumer-side declarations (the
// gBuildingNames* precedent - reloc names on unclaimed data are cosmetic).
// Kept in this narrow header so only townmgr's include closure sees them.
// Before normalization: gBuildingDescCommon.
extern const char* g_buildingDescCommon[];     // 0x6a7e24, buildingId < 15
// Before normalization: gBuildingDescDwelling.
extern const char* g_buildingDescDwelling[];   // 0x6a7834, buildingId == 15, [type*11]
// Before normalization: gBuildingDescBlacksmith.
extern const char* g_buildingDescBlacksmith[]; // 0x6a7e70, buildingId == 16, [type]
// Before normalization: gBuildingDescTown.
extern const char* g_buildingDescTown[];       // 0x6a77c8, 17 <= buildingId < 30, [type*11 + id]
// Before normalization: gBuildingDescUpgrade.
extern const char* g_buildingDescUpgrade[];    // 0x6a694c, buildingId >= 30, [id + type*14]
// Before normalization: gRampartCustomText.
extern const char* g_rampartCustomText[];      // 0x6a5e64, [town::field_38]
// Before normalization: gRampartExtraDesc.
extern const char* g_rampartExtraDesc;         // 0x6a7838

// The .bss scratch GetBuildingInfo assembles its result into and returns.
// Before normalization: gInfoText.
extern char g_infoText[];                       // 0x6aa820

#endif  // HOMM3_BUILDINGINFO_H
