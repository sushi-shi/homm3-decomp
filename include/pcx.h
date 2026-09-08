// pcx.h - retail PCX helper ABI, corroborated by the Dreamcast type stream.
//
// LIBRARY IDENTIFIED (2026-09-05): the five entry points below are the
// **Victor Image Processing Library** (Catenary Systems), linked statically.
// `imgdes` with exactly these eleven fields under exactly these names -
// ibuff / stx / sty / endx / endy / buffwidth / palette / colors / imgtype /
// bmh / hBitmap - is Victor's own image descriptor, and allocimage /
// freeimage / flipimage / loadpcx / savepcx / pcxinfo is its documented API.
//
// It occupies the tail at 0x603590..0x604823,
// about 4.7 KB in 22 functions, ending where zlib's own band begins at
// 0x604830. The two callers on the game side are Bitmap24Bit::importPCXFile
// (0x44eef0) and Bitmap816::importPCXFile (0x44fa40); each calls five of the
// band's rows and their call ORDER order-maps the exports one for one:
//
//   0x6042a0  295 B   pcxinfo(const char*, PcxData*)
//   0x603590   37 B   allocimage(imgdes*, int, int, int)   -> worker 0x6035c0
//   0x603e00 1172 B   loadpcx(const char*, imgdes*)
//   0x603b20  722 B   flipimage(imgdes*, imgdes*)
//   0x6037a0  110 B   freeimage(imgdes*)
//
// Corroboration inside the band: 0x6035c0 zeroes 11 dwords (`mov ecx,0xb /
// rep stosd`) - sizeof(imgdes) - validates bits-per-pixel against {1, 8, 24}
// with 4 folded onto 8, returns -26 for anything else, and writes
// `colors = 1 << bpp` at +0x1c (zero for 24bpp); 0x6037a0 releases `bmh` at
// +0x24 through GlobalHandle/GlobalUnlock/GlobalFree and `hBitmap` at +0x28
// through DeleteObject, then re-zeroes the same eleven dwords. The band's
// import set is the rest of the fingerprint: OpenFile/_lread/_llseek/_lclose,
// GlobalAlloc/Lock/Unlock/Free/Handle, CreateCompatibleDC/SelectObject/
// DeleteDC/DeleteObject, GetDesktopWindow/GetDC/ReleaseDC and IsBadReadPtr -
// public wrappers use stdcall (ret N), while the allocation worker and
// dimension helper use cdecl. The compiled wrappers omit EBP frames.
//
// Recovered external-library code is grouped in src/victor.cpp and
// src/victor_pcx_kernels.cpp; original source/object filenames are unavailable.
// Separate VC6 profiles reproduce the allocation/validation wrappers and
// all three assembly kernels, including the RLE decoder's register saves.
// The remaining library band is only partly reconstructed; vendor/ is pristine.
#ifndef HOMM3_PCX_H
#define HOMM3_PCX_H

// Use the SDK declarations for the external RGBQUAD, BITMAPINFOHEADER and
// HBITMAP ABI instead of maintaining a second Windows structure definition.
// Bitmap24Bit also uses numeric_limits::max(); suppress the SDK function
// macros while importing these ABI types.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

struct PcxData {
    // Before normalization: PCXvers.
    int m_pcXvers;
    // Before normalization: width.
    unsigned int m_width;
    // Before normalization: length.
    unsigned int m_length;
    // Before normalization: BPPixel.
    int m_bpPixel;
    // Before normalization: Nplanes.
    int m_nplanes;
    // Before normalization: BytesPerLine.
    int m_bytesPerLine;
    // Before normalization: PalInt.
    int m_palInt;
    // Before normalization: vbitcount.
    int m_vbitcount;
};

struct imgdes {
    // Before normalization: ibuff.
    unsigned char* m_ibuff;
    // Before normalization: stx.
    unsigned int m_stx;
    // Before normalization: sty.
    unsigned int m_sty;
    // Before normalization: endx.
    unsigned int m_endx;
    // Before normalization: endy.
    unsigned int m_endy;
    // Before normalization: buffwidth.
    unsigned int m_buffwidth;
    // Before normalization: palette.
    RGBQUAD* m_palette;
    // Before normalization: colors.
    int m_colors;
    // Before normalization: imgtype.
    int m_imgtype;
    // Before normalization: bmh.
    BITMAPINFOHEADER* m_bmh;
    // Before normalization: hBitmap.
    HBITMAP m_bitmap;
};

int __stdcall pcxinfo(const char* filename, PcxData* data);
int __stdcall allocimage(imgdes* image, int width, int height,
                         // Before normalization (locals): bits_per_pixel.
                         int bitsPerPixel);
int __stdcall loadpcx(const char* filename, imgdes* image);
int __stdcall flipimage(imgdes* source, imgdes* destination);
void __stdcall freeimage(imgdes* image);

#endif  // HOMM3_PCX_H
