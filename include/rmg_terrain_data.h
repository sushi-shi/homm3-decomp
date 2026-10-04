// Scalar terrain-painting domains and canonical frame records.
#ifndef HOMM3_RMG_TERRAIN_DATA_H
#define HOMM3_RMG_TERRAIN_DATA_H
#include "homm3_bool.h"
#include "homm3_int.h"

// Brush strength scales each terrain's special base-frame chance, which is
// a percentage at full strength. The generator always paints at half.
enum ERmgBrushStrength {
    RMG_FULL_BRUSH_STRENGTH = 8,
    RMG_BRUSH_STRENGTH = 4
};

// Transition identities; the diagrams are beside the classifier in rmg_terrain.cpp.
enum ERmgTerrainShape {
    SHAPE_FILL = 0,
    SHAPE_N_W_BLEND = 2,
    SHAPE_W_BLEND = 3,
    SHAPE_N_BLEND = 4,
    SHAPE_SE_BLEND = 5,
    SHAPE_N_W_DIAG_BLEND = 6,
    SHAPE_SE_DIAG_BLEND = 7,
    SHAPE_N_W_HARD = 8,
    SHAPE_W_HARD = 9,
    SHAPE_N_HARD = 10,
    SHAPE_SE_HARD = 11,
    SHAPE_N_W_DIAG_HARD = 12,
    SHAPE_SE_DIAG_HARD = 13,
    SHAPE_NW_SE_BLEND = 14,
    SHAPE_NW_BLEND_SE_HARD = 15,
    SHAPE_NW_SE_HARD = 16,
    SHAPE_E_BLEND_SW_HARD = 17,
    SHAPE_S_BLEND_NE_HARD = 18,
    SHAPE_E_BLEND_SE_HARD = 19,
    SHAPE_S_BLEND_SE_HARD = 20,
    SHAPE_E_HARD_SW_BLEND = 21,
    SHAPE_S_HARD_NE_BLEND = 22,
    SHAPE_N_W_SE_BLEND = 23,
    SHAPE_N_W_SE_HARD = 24,
    SHAPE_N_W_BLEND_SE_HARD = 25,
    SHAPE_N_W_HARD_SE_BLEND = 26,
    SHAPE_E_S_BLEND_SE_HARD = 27,
    SHAPE_E_S_BLEND_NE_SW_HARD = 28,
    RMG_TERRAIN_SHAPE_COUNT = 29
};

// No edge for equal terrain, a sand centre, or a dirt centre that blends with
// its neighbour; other blending pairs blend, remaining changes are hard edges.
enum ERmgTerrainNeighbourKind {
    RMG_NEIGHBOUR_NO_EDGE = 0,
    RMG_NEIGHBOUR_BLEND_EDGE = 1,
    RMG_NEIGHBOUR_HARD_EDGE = 2
};

struct TRmgTerrainPatternEntry {
    ERmgTerrainShape m_transition;
    b8 m_special;
};

// Fixed transition table entry; carries flips instead of a special-frame flag.
struct TRmgTerrainTransitionEntry {
    ERmgTerrainShape m_transition;
    b8 m_flipX;
    b8 m_flipY;

    bool matches(ERmgTerrainShape transition, b8 flipX, b8 flipY) const;
};

// First plain (shape 0) water frame.
enum ERmgWaterFrames {
    RMG_WATER_BASE_FRAME = 21
};

#endif
