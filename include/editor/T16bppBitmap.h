// T16bppBitmap.h - the editor's bitmap format templates (Loki h3maped).
//
// RTTI proves the shapes: the type_info functions register
// TBitmapBase<TPixel> without bases (__rtti_user), TBitmap<TPixel,
// TPaletteIndex> with the single public base TBitmapBase<TPixel>, and
// T8bppBitmapBase<unsigned char> / T16bppBitmapBase<unsigned char> with the
// single public bases TBitmap<unsigned char, unsigned char> /
// TBitmap<unsigned short, unsigned char> (__rtti_si). GUIGameObject.o emits
// all six unreferenced, with no vtable anywhere in the image: g++ 2.95 does
// that when a polymorphic class is completed (set_rtti_entry), so the
// hierarchy is polymorphic; the virtual destructor stands for its unknown
// virtual members. The 16-bit format accessors are static inline members,
// kept out of line in the Loki linkonce block: RGB 5:5:5 (bits 5/5/5,
// shifts 10/5/0, masks 0x7c00/0x3e0/0x1f). The other members are not
// recovered; the file name follows the class and is not proven.
#ifndef HOMM3_EDITOR_T16BPPBITMAP_H
#define HOMM3_EDITOR_T16BPPBITMAP_H

// Two one-byte constants follow every includer's .rodata (1, then 2): the
// bytes per pixel of the two formats. Names and type are inferred.
const unsigned char k8bppBytesPerPixel = 1;
const unsigned char k16bppBytesPerPixel = 2;

template <class TPixel>
class TBitmapBase {
public:
    virtual ~TBitmapBase() {}
};

template <class TPixel, class TPaletteIndex>
class TBitmap : public TBitmapBase<TPixel> {
};

// The editor's two formats are explicit specializations: each is
// completed where it is defined (the includers' type names follow the
// 8-bit format with the 16-bit one), and the 16-bit accessors are compiled
// and queued right there, before stdafx.h's CPoint (cppbridge.o writes the
// masks it passes to ResourceManager::SetPixelFormat in that place).
template <class TPaletteIndex>
class T8bppBitmapBase;

template <>
class T8bppBitmapBase<unsigned char> : public TBitmap<unsigned char, unsigned char> {
public:
    // One colour per value of the 8-bit pixel.
    enum { kNumColors = 256 };
};

template <class TPaletteIndex>
class T16bppBitmapBase;

template <>
class T16bppBitmapBase<unsigned char> : public TBitmap<unsigned short, unsigned char> {
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
