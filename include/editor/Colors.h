// Colors.h - the editor's terrain and player colours (Loki Colors.cpp).
#ifndef HOMM3_EDITOR_COLORS_H
#define HOMM3_EDITOR_COLORS_H

// Per ground tileset: palette entries 8 and 9 of its sprite, the mini-map
// colours of open ground and of the terrain obstacles on it.
struct TTerrainColors {
    unsigned short m_color;
    unsigned short m_obstacleColor;
};
extern const TTerrainColors* akTerrainColors;
// Per player (and the neutral entry): game.pal entries 63..71.
extern const unsigned short* akPlayerColor;

void initColors();

#endif  /* HOMM3_EDITOR_COLORS_H */
