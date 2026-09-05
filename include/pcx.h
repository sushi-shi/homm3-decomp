// pcx.h - retail PCX helper ABI, corroborated by the Dreamcast type stream.
//
// LIBRARY IDENTIFIED (2026-09-05): the five entry points below are the
// **Victor Image Processing Library** (Catenary Systems), linked statically.
// `imgdes` with exactly these eleven fields under exactly these names -
// ibuff / stx / sty / endx / endy / buffwidth / palette / colors / imgtype /
// bmh / hBitmap - is Victor's own image descriptor, and allocimage /
// freeimage / flipimage / loadpcx / savepcx / pcxinfo is its documented API.
//
// It occupies the WHOLE unmapped tail of the game band, 0x603590..0x604823,
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
// frameless cdecl throughout, a different compiler profile from the game.
//
// NOT ADMISSIBLE AS A UNIT: Victor is commercial closed source, it is not in
// vendor/, and none of it is trivially reconstructible. The band stays
// unmapped; only the ABI above is ours.
#ifndef HOMM3_PCX_H
#define HOMM3_PCX_H

struct PcxData {
    int PCXvers;
    unsigned int width;
    unsigned int length;
    int BPPixel;
    int Nplanes;
    int BytesPerLine;
    int PalInt;
    int vbitcount;
};

struct RGBQUAD {
    unsigned char rgbBlue;
    unsigned char rgbGreen;
    unsigned char rgbRed;
    unsigned char rgbReserved;
};

struct imgdes {
    unsigned char* ibuff;
    unsigned int stx;
    unsigned int sty;
    unsigned int endx;
    unsigned int endy;
    unsigned int buffwidth;
    RGBQUAD* palette;
    int colors;
    int imgtype;
    void* bmh;
    void* hBitmap;
};

int __stdcall pcxinfo(const char* filename, PcxData* data);
int __stdcall allocimage(imgdes* image, int width, int height,
                         int bits_per_pixel);
int __stdcall loadpcx(const char* filename, imgdes* image);
int __stdcall flipimage(imgdes* source, imgdes* destination);
void __stdcall freeimage(imgdes* image);

#endif  // HOMM3_PCX_H
