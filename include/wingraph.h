#ifndef HOMM3_WINGRAPH_H
#define HOMM3_WINGRAPH_H

#include "va.h"

#if defined(HOMM3_TARGET_MAC)
#include "platform.h"  // ddraw.h needs the Win32 SDK scope platform.h opens
#else
#include <ddraw.h>
#endif

// E:\gamedcs\WinGraph.h:55.  Dreamcast keeps this header helper out of
// line, and its xref graph proves calls from mousemgr, spells, and wingraph.
// Retail VC6 /Ob2 expands every observed Windows use instead.  Keep the
// pixel-format declaration with the helper so each consumer sees the same public
// source boundary rather than a TU-local facsimile.  Retail DrawBolt fixes
// their order as R/G/B: its green ramp selects only 0x68c864, while Chain
// Lightning keeps 0x68c868 saturated as its other two components fade.
// DC wingraph.cpp:235 names PixelFormat as _DDPIXELFORMAT; line 239 reads
// its RGB mask members. Retail 0x6014f0 passes this complete 32-byte SDK
// object at 0x68c850 to GetPixelFormat, then reads masks at +0x10/+0x14/+0x18.
DATA(0x0068c850) extern DDPIXELFORMAT g_pixelFormat;

// Original InitWin; shared view of the locked DirectDraw back surface.
class Bitmap16Bit;
extern Bitmap16Bit g_initWin;

inline unsigned rgBto16(int r, int g, int b)
{
    unsigned color;
    color = (r * g_pixelFormat.dwRBitMask / 255) & g_pixelFormat.dwRBitMask;
    color |= (g * g_pixelFormat.dwGBitMask / 255) & g_pixelFormat.dwGBitMask;
    color |= (b * g_pixelFormat.dwBBitMask / 255) & g_pixelFormat.dwBBitMask;

    return color;
}

// Live prototypes (claimed wingraph.cpp bodies; called from kbwin's
// AppCommand fullscreen arm, AppExit, WM_PAINT and WinMain).
unsigned char setFullScreenStatus(int fullScreenOn);    // 0x6019a0
void cleanUpWinGraphics();                               // 0x601890
int appPaint(void* hwnd, void* hdc);                     // 0x601820
void initGraphics();
void ddInitGraphics();
void ddCleanUpWinGraphics();                             // 0x6018a0
unsigned char ddSetFullScreenStatus(int newStatus);     // 0x601a00
void ddsd(int ddErr, char* file, int line);           // 0x6006e0
int getDesktopWidth();                                   // 0x6014c0
int getDesktopHeight();                                  // 0x6014d0
// GetDesktopInfo's verdict: the engine's 16-bit surfaces need a desktop
// already at 16bpp, so the GetDeviceCaps(BITSPIXEL) reading is compared
// against this depth and the result is the function's return value.
enum { DESKTOP_REQUIRED_BITS_PER_PIXEL = 16 };
unsigned char getDesktopInfo();
long ddRestoreSurfaces();                                // 0x6013a0

void ddCleanUpWinGraphics();                             // 0x6018a0
unsigned char ddSetFullScreenStatus(int newStatus);     // 0x601a00

// The windowed frame DDSetFullScreenStatus restores, and the 565 green
// mask it tests the rebuilt surface against. Both are written as raw
// immediates by retail (0x10ca0000 and 0x7e0); the style decomposes
// exactly, and the mask is the value bitmap16.h's Remap note already
// names from the other side.
// (this header is included where <windows.h> is not, so the style is
// spelled as its own value: WS_VISIBLE 0x10000000 | WS_CAPTION 0x00c00000
// | WS_SYSMENU 0x00080000 | WS_MINIMIZEBOX 0x00020000.)
enum {
    WINDOWED_WINDOW_STYLE = 0x10ca0000,
    GREEN_MASK_565 = 0x7e0
};

// The blitter AppPaint hands its damaged rectangle to; the wingraph roster's
// RobAppBlit slot (fastcall, one tagRECT* in ecx).
struct tagRECT;
void robAppBlit(tagRECT* combRect);                     // 0x5ffe70

// The adventure-map complete-redraw latch, read by winmgr's fizzle pair
// (heroWindowManager::SaveFizzleSourceX / ::FizzleForwardX) before they touch
// the screen surface. DECLARATION ONLY - advmgr.h owns the DATA claim on
// .bss 0x6989c0 and a second claim on the same RVA is a fatal duplicate at
// delink time. Declared here rather than in winmgr.h because this header has
// eight includers against winmgr.h's seventy, and a declarator's cost is
// paid by every TU in the closure.
extern int g_completeDrawEnabled;                         // .bss 0x6989c0

// Fullscreen-toggle inhibitor read by SetFullScreenStatus's leading test.
// That test is the ONLY reference to .bss 0x6989d4 anywhere in the image (a
// whole-image scan for the absolute operand returns exactly one hit), so no
// writer attests its producer. The descriptive name follows the mode-change
// guard; wingraph.cpp owns the storage.
extern int g_fullScreenChangesDisabled;             // .bss 0x6989d4

struct IDirectDrawSurface;
struct tagRECT;
IDirectDrawSurface* ddCreateSurface(unsigned long width,
                                   unsigned long height,
                                   int primary);         // 0x6005f0
void ddBlit(IDirectDrawSurface* dstSurface, const tagRECT& dstRect,
            IDirectDrawSurface* srcSurface, const tagRECT& srcRect,
            unsigned long flags);

// The DirectDraw surface pair (Blt target and game draw surface).
// Owner attribution: the DD lifecycle (DDCreatePrimary/DDCreateSurface
// per the roster above) lives in this TU, so the pair is declared
// here; the .bss addresses stay extern-only until the owning TU's
// claims land. The forward declaration keeps <ddraw.h> out of
// includers that never touch DirectDraw.
extern IDirectDrawSurface* g_ddsPrimary;  // .bss 0x6aacbc
extern IDirectDrawSurface* g_ddsBack;     // .bss 0x6aacc0

#endif  /* HOMM3_WINGRAPH_H */
