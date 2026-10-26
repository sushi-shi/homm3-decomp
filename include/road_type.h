// road_type.h - shared adventure-map road and river layer domains.
#ifndef HOMM3_ROAD_TYPE_H
#define HOMM3_ROAD_TYPE_H

#include "domains.h"

// Dreamcast TRoadType (NB11 0x356f): the road layer of a map cell.
H3_ENUM_BEGIN(TRoadType)
    eRoadNone = 0,
    eRoadDirt = 1,
    eRoadGravel = 2,
    eRoadCobblestone = 3,
    kNumRoadTypes = 4
H3_ENUM_END(TRoadType)

// Dreamcast TRiverType (NB11 0x3571): the river layer of a map cell.
H3_ENUM_BEGIN(TRiverType)
    eRiverNone = 0,
    eRiverClear = 1,
    eRiverIcy = 2,
    eRiverMuddy = 3,
    eRiverLava = 4,
    kNumRiverTypes = 5
H3_ENUM_END(TRiverType)

#endif  // HOMM3_ROAD_TYPE_H
