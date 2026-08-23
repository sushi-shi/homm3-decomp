// pcx.h - retail PCX helper ABI, corroborated by the Dreamcast type stream.
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
