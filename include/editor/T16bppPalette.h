// T16bppPalette.h - a 256-entry palette in the editor's 16-bit pixel format
// (T16bppPalette.cpp; Loki h3maped object 6). Declared so far only as far
// as the map windows need it. The Windows rgbToEntry is out of line
// (h3maped 0x40657e: the 5:5:5 entry of a three-byte colour), where Loki's
// is an inline 5:6:5 conversion.
#ifndef HOMM3_EDITOR_T16BPPPALETTE_H
#define HOMM3_EDITOR_T16BPPPALETTE_H

// Three bytes per entry: T16bppPalette(const TRGB*) steps its source by 3.
struct TRGB {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

class T16bppPalette {
public:
    static unsigned short rgbToEntry(const TRGB& rgb);
};

#endif  /* HOMM3_EDITOR_T16BPPPALETTE_H */
