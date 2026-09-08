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
// Provisional semantic name for the validator's -26 status.
enum { victorUnsupportedBitDepth = -26 };
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
