// Colors.h - the editor's terrain and player colours (Loki Colors.cpp).
#ifndef HOMM3_EDITOR_COLORS_H
#define HOMM3_EDITOR_COLORS_H

// Per ground tileset: palette entries 8 and 9 of its sprite.
extern const unsigned short (* const akTerrainColors)[2];
// Per player (and the neutral entry): game.pal entries 63..71.
extern const unsigned short* const akPlayerColor;

void initColors();

#endif  /* HOMM3_EDITOR_COLORS_H */
