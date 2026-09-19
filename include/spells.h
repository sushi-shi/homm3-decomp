// spells.h - spells.cpp (compiland spells.obj)
#ifndef HOMM3_SPELLS_H
#define HOMM3_SPELLS_H

#include "armygrp.h"

TCreatureType getElementalType(SpellID spell);

// DrawBolt's Chain Lightning arm is the one bolt colour shaded
// PROCEDURALLY instead of from a span table: it steps red and green
// down 255 -> 240 -> 224 -> 216 -> 200 -> 192 with depth into the drawn
// span while blue stays saturated, and retail spells all six steps out
// one at a time rather than through a table (each arm re-expands
// RGBto16 in full; only the last channel term is tail-merged). Depth 0
// is the span's outer rim, and anything deeper than four is the floor.
enum EBoltSpanDepth {
    BOLT_SPAN_DEPTH_0 = 0,
    BOLT_SPAN_DEPTH_1 = 1,
    BOLT_SPAN_DEPTH_2 = 2,
    BOLT_SPAN_DEPTH_3 = 3,
    BOLT_SPAN_DEPTH_4 = 4
};

// --- globals ---
extern unsigned char g_boltGreenSpanColors[5][3];
extern unsigned char g_boltWhiteSpanColors[5][3];
extern unsigned char g_boltSpectrumColors[15][3];

// --- combatManager ---
// Complete expands this helper into CastSpell's failure path while Dreamcast
// also emits the standalone inline body.

#endif  /* HOMM3_SPELLS_H */
