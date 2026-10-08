// T16bppBitmap.h - the editor's 16-bit pixel format template (Loki h3maped).
//
// RTTI proves the shape: __tf for T16bppBitmapBase<unsigned char> registers
// a single public base TBitmap<unsigned short, unsigned char> (__rtti_si).
// The format accessors are static inline members, kept out of line in the
// Loki linkonce block: RGB 5:5:5 (bits 5/5/5, shifts 10/5/0, masks
// 0x7c00/0x3e0/0x1f). TBitmap's own members are not recovered yet; the file
// name follows the class and is not proven.
#ifndef HOMM3_EDITOR_T16BPPBITMAP_H
#define HOMM3_EDITOR_T16BPPBITMAP_H

template <class TPixel, class TPaletteIndex>
class TBitmap {
};

template <class TPaletteIndex>
class T16bppBitmapBase : public TBitmap<unsigned short, TPaletteIndex> {
public:
    static int redBits() { return 5; }
    static int redShift() { return 10; }
    static int greenBits() { return 5; }
    static int greenShift() { return 5; }
    static int blueBits() { return 5; }
    static int blueShift() { return 0; }
    static unsigned short redMask() { return 0x7c00; }
    static unsigned short greenMask() { return 0x3e0; }
    static unsigned short blueMask() { return 0x1f; }
};

#endif  /* HOMM3_EDITOR_T16BPPBITMAP_H */
