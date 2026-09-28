#ifndef HOMM3_VICTOR_H
#define HOMM3_VICTOR_H

#include "pcx.h"

// PCX disk header. Field roles and offsets follow the retail reader;
// names are provisional. Natural WORD alignment gives the 128-byte layout.
struct VictorPcxHeader {
    unsigned char m_manufacturer;
    unsigned char m_version;
    unsigned char m_encoding;
    unsigned char m_bitsPerPixel;
    unsigned short m_minX, m_minY, m_maxX, m_maxY;
    unsigned short m_horizontalDpi, m_verticalDpi;
    unsigned char m_palette[48];
    unsigned char m_reserved, m_planes;
    unsigned short m_bytesPerLine, m_paletteType;
    unsigned short m_screenWidth, m_screenHeight;
    unsigned char m_filler[54];
};
enum { victorPcxManufacturer = 10, victorPcxRleEncoding = 1,
       victorPcxEgaPlanes = 4, victorPcxVersionWithPalette = 2,
       victorPcxVersionThree = 5, victorPcxRgbPlanes = 3,
       victorPcxExtendedPaletteMarker = 12, victorPcxExtendedColors = 256,
       victorPcxHeaderColors = 16 };
int __stdcall victorReadPcxPalette(const char* filename, RGBQUAD* palette);
enum VictorPcxDecodeMode {
    victorPcxInvalidMode = -16,
    victorPcxIndexedMode = 1,
    victorPcxMonochromeMode = 2,
    victorPcxFourPlaneMode = 3,
    victorPcxNibbleMode = 4,
    victorPcxRgbMode = 5
};
enum { victorPcxInputCapacity = 65540, victorPcxMonochromeColors = 2,
       victorImageGrayscale = 1, victorPcxSinglePlane = 1 };

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
typedef HBITMAP (WINAPI *VictorCreateDibSection)(HDC, const BITMAPINFO*,
                                               UINT, void**, HANDLE, DWORD);
extern VictorCreateDibSection g_victorCreateDibSection;
// Provisional semantic name for the validator's -26 status.
enum { victorUnsupportedBitDepth = -26 };
enum VictorPixelDepth {
    victorMonochrome = 1,
    victorFourBitColor = 4,
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
void __stdcall victorInsertBits(unsigned char* destination,
                                const unsigned char* source, int offset, int count);
void __stdcall victorExtractBits(unsigned char* destination,
                                 const unsigned char* source, int offset, int count);

// Provisional role name. The allocation worker uses the fifth argument to
// select a DIB section versus a single global-memory allocation.
extern unsigned int g_victorUseDibSection;

// Victor's per-module lock: each of eight unrecovered cleanups at
// 0x604620..0x604700 runs `if (m_initialized) { DeleteCriticalSection(
// &m_section); m_initialized = 0; }` on one record, the flag at +0x18.
// Type and member names are invented.
struct VictorLock {
    CRITICAL_SECTION m_section;
    int m_initialized;
};
SIZE(VictorLock, 0x1c);

// Unrecovered Victor code reached only through g_victorModuleCleanups:
// 0x603b10 is a bare `ret`; victorReleaseLock<N> is the cleanup of the Nth
// VictorLock by address. Names are invented; bodies belong to the .text
// recovery.
void __cdecl victorReleaseNothing();
void __cdecl victorReleaseLock0();
void __cdecl victorReleaseLock1();
void __cdecl victorReleaseLock2();
void __cdecl victorReleaseLock3();
void __cdecl victorReleaseLock4();
void __cdecl victorReleaseLock5();
void __cdecl victorReleaseLock6();
void __cdecl victorReleaseLock7();

// TIFF 6.0 field layouts. The in-memory IFD image keeps the on-disk packing
// (2-byte count, 12-byte entries at 2-byte offsets, next-IFD offset).
#pragma pack(push, 2)
struct VictorTiffEntry {
    unsigned short m_tag;
    unsigned short m_type;
    unsigned long m_count;
    unsigned long m_value;
};
struct VictorTiffIfd {
    unsigned short m_count;
    VictorTiffEntry m_entries[19];
    unsigned long m_nextIfd;
};
#pragma pack(pop)
SIZE(VictorTiffIfd, 0xea);
// A 16-byte tag row; the last dword is zero in every retail row and its
// role is unknown. Type and member names are invented.
struct VictorTiffTagDefault {
    unsigned short m_tag;
    unsigned short m_type;
    unsigned long m_count;
    unsigned long m_value;
    unsigned long m_reserved;
};
SIZE(VictorTiffTagDefault, 0x10);

#endif
