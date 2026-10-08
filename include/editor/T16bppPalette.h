// T16bppPalette.h - a 256-entry palette in the editor's 16-bit pixel format
// (Loki h3maped object 6; the file name follows the class).
#ifndef HOMM3_EDITOR_T16BPPPALETTE_H
#define HOMM3_EDITOR_T16BPPPALETTE_H

#include "editor/T16bppBitmap.h"

// Three bytes per entry: T16bppPalette(const TRGB*) steps its source by 3.
struct TRGB {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

class T16bppPalette {
public:
    T16bppPalette(const TRGB* rgb);

    // Static: the constructor pushes only the colour.
    static unsigned short rgbToEntry(const TRGB& rgb)
    {
        typedef T16bppBitmapBase<unsigned char> Format;
        return ((rgb.red >> (8 - Format::redBits())) << Format::redShift())
             | ((rgb.green >> (8 - Format::greenBits())) << Format::greenShift())
             | ((rgb.blue >> (8 - Format::blueBits())) << Format::blueShift());
    }

    unsigned short m_entries[256];
};

#endif  /* HOMM3_EDITOR_T16BPPPALETTE_H */
