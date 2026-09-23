#ifndef HOMM3_BUILDINGINFO_H
#define HOMM3_BUILDINGINFO_H

// The building-description tables read by townmgr.cpp's GetBuildingInfo,
// one per building-id band, mirroring castle.h's gBuildingNames* name
// tables (identical type*11 / type*14 strides) with a per-town-type
// blacksmith column and the Rampart custom-building columns. The owning
// data TU is unlocated; these are consumer-side declarations (the
// gBuildingNames* precedent - reloc names on unclaimed data are cosmetic).
// Kept in this narrow header so only townmgr's include closure sees them.
     // 0x6a7e24, buildingId < 15
      // 0x6a5e64, [town::field_38]
         // 0x6a7838

// The .bss scratch GetBuildingInfo assembles its result into and returns.
extern char g_infoText[];                       // 0x6aa820

#endif  // HOMM3_BUILDINGINFO_H
