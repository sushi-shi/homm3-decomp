// T16bppBitmap.h - the editor's bitmap templates (Loki h3maped; the header
// name is Loki's). RTTI proves TBitmapBase<unsigned short> and
// TBitmap<unsigned short, unsigned long> (h3maped vtables 0x531e54 and
// 0x531e6c, five slots each): the base owns a pixel buffer of
// width x height rows, pitch bytes apart, and leaves allocation, release
// and the row pitch to its derived classes (pure slots 2..4); slot 1 is
// its assignment, which reallocates through them and copies the pixels.
// TBitmap allocates with operator new and pads rows to four bytes. Every
// body is a COMDAT that DIBSection.cpp emits first (0x40d38f..0x40d6b1).
// The member names are not recorded.
#ifndef HOMM3_EDITOR_T16BPPBITMAP_H
#define HOMM3_EDITOR_T16BPPBITMAP_H

#include <string.h>

#include "exceptions.h"
#include "va.h"

template <class TPixel>
class TBitmapBase {
public:
    // copy's flags: mirror the rows top to bottom, the pixels left to right.
    enum {
        kFlipVertical = 1,
        kFlipHorizontal = 2
    };

    TBitmapBase() : _m_width(0), _m_height(0), _m_pitch(0), _m_pPixels(NULL) {}
    // VA instance: TBitmapBase<unsigned short>::~TBitmapBase
    VA(0x0040d38f, 0x7)
    virtual ~TBitmapBase() {}

    // VA instance: TBitmapBase<unsigned short>::operator=
    VA(0x0040d3cf, 0x6e)
    virtual TBitmapBase& operator=(const TBitmapBase& other)
    {
        if (&other == this)
            return *this;
        if (_m_pPixels != NULL) {
            free(_m_pPixels);
            _m_pPixels = NULL;
        }
        _m_width = other._m_width;
        _m_height = other._m_height;
        _m_pitch = computePitch(_m_width);
        if (other._m_pPixels != NULL) {
            _m_pPixels = allocate(_m_width, _m_height);
            copy(0, 0, other, 0, 0, _m_width, _m_height, 0);
        }
        return *this;
    }

    unsigned int getWidth() const { return _m_width; }
    unsigned int getHeight() const { return _m_height; }

    // VA instance: TBitmapBase<unsigned short>::create
    VA(0x0040d43d, 0x49)
    void create(unsigned int width, unsigned int height)
    {
        if (_m_pPixels != NULL) {
            free(_m_pPixels);
            _m_pPixels = NULL;
        }
        _m_width = width;
        _m_height = height;
        if (_m_width > 0 && _m_height > 0) {
            _m_pitch = computePitch(_m_width);
            _m_pPixels = allocate(_m_width, _m_height);
        }
    }

    // VA instance: TBitmapBase<unsigned short>::destroy
    VA(0x0040d486, 0x22)
    void destroy()
    {
        if (_m_pPixels != NULL) {
            free(_m_pPixels);
            _m_pPixels = NULL;
            _m_pitch = 0;
            _m_height = 0;
            _m_width = 0;
        }
    }

    // VA instance: TBitmapBase<unsigned short>::copy
    VA(0x0040d544, 0x16d)
    void copy(int x, int y, const TBitmapBase& src, int srcX, int srcY,
              unsigned int width, unsigned int height, unsigned int flags)
    {
        if (!(flags & kFlipVertical)) {
            unsigned char* pDest = (unsigned char*)_m_pPixels + y * _m_pitch + x * sizeof(TPixel);
            const unsigned char* pSrc = (const unsigned char*)src._m_pPixels + srcY * src._m_pitch
                                        + srcX * sizeof(TPixel);
            if (!(flags & kFlipHorizontal)) {
                while (height-- > 0) {
                    memcpy(pDest, pSrc, width * sizeof(TPixel));
                    pDest += _m_pitch;
                    pSrc += src._m_pitch;
                }
            } else {
                while (height-- > 0) {
                    TPixel* pDestPixel = (TPixel*)pDest;
                    for (unsigned int i = width; i > 0; i--)
                        *pDestPixel++ = ((const TPixel*)pSrc)[i - 1];
                    pDest += _m_pitch;
                    pSrc += src._m_pitch;
                }
            }
        } else {
            unsigned char* pDest = (unsigned char*)_m_pPixels + y * _m_pitch + x * sizeof(TPixel);
            const unsigned char* pSrc = (const unsigned char*)src._m_pPixels
                                        + (srcY + height - 1) * src._m_pitch + srcX * sizeof(TPixel);
            if (!(flags & kFlipHorizontal)) {
                while (height-- > 0) {
                    memcpy(pDest, pSrc, width * sizeof(TPixel));
                    pDest += _m_pitch;
                    pSrc -= src._m_pitch;
                }
            } else {
                while (height-- > 0) {
                    TPixel* pDestPixel = (TPixel*)pDest;
                    for (unsigned int i = width; i > 0; i--)
                        *pDestPixel++ = ((const TPixel*)pSrc)[i - 1];
                    pDest += _m_pitch;
                    pSrc -= src._m_pitch;
                }
            }
        }
    }

    virtual TPixel* allocate(unsigned int width, unsigned int height) = 0;
    virtual void free(TPixel* pPixels) = 0;
    virtual unsigned int computePitch(unsigned int width) const = 0;

protected:
    unsigned int _m_width;
    unsigned int _m_height;
    unsigned int _m_pitch;
    TPixel* _m_pPixels;
};

template <class TPixel, class TPaletteIndex>
class TBitmap : public TBitmapBase<TPixel> {
public:
    TBitmap() {}
    // VA instance: TBitmap<unsigned short, unsigned long>::~TBitmap
    VA(0x0040d4a8, 0x33)
    virtual ~TBitmap() { destroy(); }

    // Rows of whole pixels padded to a multiple of four bytes.
    static unsigned int _alignedPitch(unsigned int width) { return (width * sizeof(TPixel) + 3) & ~3; }

    // VA instance: TBitmap<unsigned short, unsigned long>::allocate
    VA(0x0040d4db, 0x38)
    virtual TPixel* allocate(unsigned int width, unsigned int height)
    {
        TPixel* pPixels = static_cast<TPixel*>(operator new(_alignedPitch(width) * height));
        if (pPixels == NULL)
            throw TAllocationFailure();
        return pPixels;
    }

    // VA instance: TBitmap<unsigned short, unsigned long>::free
    VA(0x0040d52a, 0xd)
    virtual void free(TPixel* pPixels) { operator delete(pPixels); }

    // VA instance: TBitmap<unsigned short, unsigned long>::computePitch
    VA(0x0040d537, 0xd)
    virtual unsigned int computePitch(unsigned int width) const { return _alignedPitch(width); }
};

// The 16-bit RGB 5:5:5 format over TBitmap (Loki's accessors). It adds no
// virtual of its own: h3maped keeps no vtable for it, and its implicit
// destructor (0x40d18f) is TBitmap's body.
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
