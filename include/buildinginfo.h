#ifndef HOMM3_BUILDINGINFO_H
#define HOMM3_BUILDINGINFO_H

// The building-description tables read by townmgr.cpp's GetBuildingInfo,
// one per building-id band, mirroring castle.h's gBuildingNames* name
// tables (identical type*11 / type*14 strides) with a per-town-type
// blacksmith column and the Rampart custom-building columns. The owning
// data TU is unlocated; these are consumer-side declarations (the
// gBuildingNames* precedent - reloc names on unclaimed data are cosmetic).
// Kept in this narrow header so only townmgr's include closure sees them.
extern const char* gBuildingDescCommon[];     // 0x6a7e24, buildingId < 15
extern const char* gBuildingDescDwelling[];   // 0x6a7834, buildingId == 15, [type*11]
extern const char* gBuildingDescBlacksmith[]; // 0x6a7e70, buildingId == 16, [type]
extern const char* gBuildingDescTown[];       // 0x6a77c8, 17 <= buildingId < 30, [type*11 + id]
extern const char* gBuildingDescUpgrade[];    // 0x6a694c, buildingId >= 30, [id + type*14]
extern const char* gRampartCustomText[];      // 0x6a5e64, [town::field_38]
extern const char* gRampartExtraDesc;         // 0x6a7838

// The .bss scratch GetBuildingInfo assembles its result into and returns.
extern char gInfoText[];                       // 0x6aa820

#endif  // HOMM3_BUILDINGINFO_H
