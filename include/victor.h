#ifndef HOMM3_VICTOR_H
#define HOMM3_VICTOR_H

#include "pcx.h"

// Internal Victor library names are provisional descriptions of the pinned
// retail behavior. Public entry-point spellings remain in pcx.h.
// 0x6035c0: cdecl worker; the wrapper passes the image, dimensions, pixel
// depth, then the process-wide DIB-section mode. It returns an error code.
int __cdecl victorAllocateImage(imgdes* image, int width, int height,
                               int bitsPerPixel, unsigned int useDibSection);
// 0x6038b0: stdcall validator, ret 4. It checks the descriptor/buffer and
// bitmap header, normalizes region endpoints, and returns a signed status.
int __stdcall victorValidateImage(imgdes* image);
int __stdcall victorUploadPalette(imgdes* image);
void __stdcall victorInitializePalette(imgdes* image);
// Provisional identity from the HDC/start/count/RGBQUAD call ABI at
// 0x603871 and 0x603a93: dynamically supplied DIB color-table setter.
typedef UINT (WINAPI *VictorSetDibColorTable)(HDC, UINT, UINT, const RGBQUAD*);
extern VictorSetDibColorTable g_victorSetDibColorTable;
// Provisional semantic name for the validator's -26 status.
enum { victorUnsupportedBitDepth = -26 };
enum VictorPixelDepth {
    victorMonochrome = 1,
    victorIndexedColor = 8,
    victorTrueColor = 24
};
int __stdcall victorValidateBitmap(imgdes* image);
void __cdecl victorMinimumDimensions(imgdes* first, imgdes* second,
                                      unsigned int* height, unsigned int* width);
int __cdecl victorDecodeRleBytes(unsigned char* destination,
                                 unsigned char* source, int count);
void __cdecl victorUnpackFourPlanes(unsigned char* destination,
                                    const unsigned char* source,
                                    int planeStride, int pixels);
void __cdecl victorInterleaveRgbPlanes(unsigned char* destination,
                                       const unsigned char* source, int stride);

// Provisional role name. The allocation worker uses the fifth argument to
// select a DIB section versus a single global-memory allocation.
extern unsigned int g_victorUseDibSection;

#endif
