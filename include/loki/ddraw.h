/* ddraw.h - the DirectDraw records the shared source names, as the Loki port
 * spells them over SDL surfaces (work/loki-game only). */
#ifndef HOMM3_LOKI_DDRAW_H
#define HOMM3_LOKI_DDRAW_H

#include <windows.h>

struct IDirectDraw;
struct IDirectDrawSurface;
struct IDirectDrawPalette;
typedef IDirectDraw* LPDIRECTDRAW;
typedef IDirectDrawSurface* LPDIRECTDRAWSURFACE;
typedef IDirectDrawPalette* LPDIRECTDRAWPALETTE;

typedef struct _DDPIXELFORMAT {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwFourCC;
    DWORD dwRGBBitCount;
    DWORD dwRBitMask;
    DWORD dwGBitMask;
    DWORD dwBBitMask;
    DWORD dwRGBAlphaBitMask;
} DDPIXELFORMAT, *LPDDPIXELFORMAT;

#define DD_OK 0

#endif
