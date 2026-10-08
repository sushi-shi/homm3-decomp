// Reconstructed Victor Image Processing Library code (Catenary Systems).
// This grouping filename is provisional: the original library source/object
// names are unavailable. Retail library ownership follows imgdes, the public
// PCX APIs and the image-allocation/Win32 import band described in pcx.h.
#include "va.h"

#include <stdlib.h>
#include <string.h>

#include "victor.h"

// The allocation mode occupies zero-initialized storage in retail .data's
// virtual tail; the worker receives its current value as argument five.
DATA(0x006abaa4) unsigned int g_victorUseDibSection;
DATA(0x006abaa8) VictorCreateDibSection g_victorCreateDibSection;
DATA(0x006abaac) VictorSetDibColorTable g_victorSetDibColorTable;
// Victor .rdata that no linked code reads: the linked Victor objects keep
// their data while retail drops their unreferenced functions. Every object
// in this band starts 8-byte aligned; the zero bytes before an aligned start
// (four after each 12-entry value list, six after each 162-entry list, two
// after the type sizes) are that alignment and remain unclaimed. Extents come
// from each table's content; all names are invented.
// A complete permutation of the 64 coefficient positions, as byte offsets of
// 32-bit coefficients.
DATA(0x00643e38) extern const unsigned char g_victorJpegCoefficientOffsets[64] = {
    112, 40, 72, 104, 144, 176, 208, 136, 88, 8, 16, 48, 192, 232, 240, 168,
    56, 24, 0, 80, 160, 224, 248, 200, 96, 64, 32, 120, 128, 216, 184, 152,
    148, 180, 212, 140, 116, 44, 76, 108, 196, 236, 244, 172, 92, 12, 20, 52,
    164, 228, 252, 204, 60, 28, 4, 84, 132, 220, 188, 156, 100, 68, 36, 124
};
// JPEG zigzag scan position -> natural (row-major) coefficient index.
DATA(0x00643e78) extern const unsigned char g_victorJpegZigzagOrder[64] = {
    0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};
// ITU T.81 Annex K.1/K.2 quantization tables in zigzag order.
DATA(0x00643eb8) extern const unsigned char g_victorJpegLuminanceQuantizer[64] = {
    16, 11, 12, 14, 12, 10, 16, 14, 13, 14, 18, 17, 16, 19, 24, 40,
    26, 24, 22, 22, 24, 49, 35, 37, 29, 40, 58, 51, 61, 60, 57, 51,
    56, 55, 64, 72, 92, 78, 64, 68, 87, 69, 55, 56, 80, 109, 81, 87,
    95, 98, 103, 104, 103, 62, 77, 113, 121, 112, 100, 120, 92, 101, 103, 99
};
DATA(0x00643ef8) extern const unsigned char g_victorJpegChrominanceQuantizer[64] = {
    17, 18, 18, 24, 21, 24, 47, 26, 26, 47, 99, 66, 56, 66, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99
};
// ITU T.81 Annex K.3 Huffman tables: 16 code-length counts, then the values.
// The format fixes the BITS lists at sixteen counts (code lengths 1..16), so
// the four lists are sized by their complete initializers, trailing zeros
// included.
DATA(0x00643f38) extern const unsigned char g_victorJpegDcLuminanceBits[] = {
    0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0
};
DATA(0x00643f48) extern const unsigned char g_victorJpegDcLuminanceValues[12] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};
DATA(0x00643f58) extern const unsigned char g_victorJpegDcChrominanceBits[] = {
    0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0
};
DATA(0x00643f68) extern const unsigned char g_victorJpegDcChrominanceValues[12] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};
DATA(0x00643f78) extern const unsigned char g_victorJpegAcLuminanceBits[] = {
    0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 125
};
DATA(0x00643f88) extern const unsigned char g_victorJpegAcLuminanceValues[162] = {
    0x01, 0x02, 0x03, 0, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06,
    0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
    0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72,
    0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45,
    0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75,
    0x76, 0x77, 0x78, 0x79, 0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3,
    0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
    0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
    0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
    0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4,
    0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa
};
DATA(0x00644030) extern const unsigned char g_victorJpegAcChrominanceBits[] = {
    0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 119
};
DATA(0x00644040) extern const unsigned char g_victorJpegAcChrominanceValues[162] = {
    0, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41,
    0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
    0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0, 0x15, 0x62, 0x72, 0xd1,
    0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
    0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44,
    0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a,
    0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
    0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
    0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
    0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4,
    0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa
};
// TIFF 5.0 field-type byte sizes for types 0..5 (none, BYTE, ASCII, SHORT,
// LONG, RATIONAL).
DATA(0x006440e8) extern const unsigned char g_victorTiffTypeSizes[6] = {
    0, 1, 1, 2, 4, 8
};
// PageNumber, Predictor and ColorMap tag rows; the IFD template below ends
// with the same three tags.
DATA(0x006440f0) extern const VictorTiffTagDefault g_victorTiffTagDefaults[3] = {
    { 0x129, 3, 2, 0, 0 },
    { 0x13d, 3, 1, 1, 0 },
    { 0x140, 3, 0, 0, 0 }
};
// Three RGB bits (red = 1, green = 2, blue = 4) -> standard VGA color index.
DATA(0x00644130) extern const unsigned char g_victorRgbToVgaIndex[8] = {
    0, 12, 10, 14, 9, 13, 11, 15
};
// 6x6x6 color-cube index -> palette entry, filled from 225 downward.
DATA(0x00644138) extern const unsigned char g_victorColorCubeIndices[216] = {
    225, 224, 223, 222, 221, 220, 219, 218, 217, 216, 215, 214, 213, 212, 211, 210,
    209, 208, 207, 206, 205, 204, 203, 202, 201, 200, 199, 198, 197, 196, 195, 194,
    193, 192, 191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, 180, 179, 178,
    177, 176, 175, 174, 173, 172, 171, 170, 169, 168, 167, 166, 165, 164, 163, 162,
    161, 160, 159, 158, 157, 156, 155, 154, 153, 152, 151, 150, 149, 148, 147, 146,
    145, 144, 143, 142, 141, 140, 139, 138, 137, 136, 135, 134, 133, 132, 131, 130,
    129, 128, 127, 126, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114,
    113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100, 99, 98,
    97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82,
    81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66,
    65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50,
    49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34,
    33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18,
    17, 16, 15, 14, 13, 12, 11, 10
};
// 8x8 ordered-dither thresholds on the 51-step cube scale (0..50).
DATA(0x00644210) extern const unsigned char g_victorOrderedDither[64] = {
    0, 38, 9, 47, 2, 40, 11, 50,
    25, 12, 35, 22, 27, 15, 37, 24,
    6, 44, 3, 41, 8, 47, 5, 43,
    31, 19, 28, 15, 34, 21, 31, 18,
    1, 39, 11, 49, 0, 39, 10, 48,
    27, 14, 36, 23, 26, 13, 35, 23,
    7, 46, 4, 43, 7, 45, 3, 42,
    33, 20, 30, 17, 32, 19, 29, 16
};
// Channel byte -> cube level and remainder (x / 51, x % 51).
DATA(0x00644250) extern const unsigned char g_victorDivide51[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5
};
DATA(0x00644350) extern const unsigned char g_victorModulo51[256] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
    13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
    29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
    45, 46, 47, 48, 49, 50, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
    42, 43, 44, 45, 46, 47, 48, 49, 50, 0, 1, 2, 3, 4, 5, 6,
    7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
    23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
    39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 0, 1, 2, 3,
    4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 0
};

// Victor .data. The cleanup table lists the bare-return cleanup of the
// allocation object, then the eight lock cleanups from the last to the
// first; nothing reads the table (its function pointers are its eight
// admitted relocations plus one). The zero dword after it (0x68d29c) may
// be a terminator or alignment before the next object and stays unclaimed.
DATA(0x0068d278) void (__cdecl* g_victorModuleCleanups[9])() = {
    victorReleaseNothing, victorDestroyLock7, victorDestroyLock6,
    victorDestroyLock5, victorDestroyLock4, victorDestroyLock3,
    victorDestroyLock2, victorDestroyLock1, victorDestroyLock0
};
// JPEG: the JFIF APP0 identifier pointer and the EOI marker bytes.
DATA(0x0068d350) const char* g_victorJfifIdentifier =
    DATA_COMPGEN(0x0068d358, victorJfifText, "JFIF");
DATA(0x0068d354) unsigned char g_victorJpegEndOfImage[2] = { 0xff, 0xd9 };
// ITU T.4 (CCITT Group 3) code words as { code, bit length }: 64 terminating
// codes, 27 make-up codes (64..1728) and the 13 shared extended make-up codes
// (1792..2560) for each color, then the nine T.4 two-dimensional mode codes
// (VR3, VR2, VR1, V0, VL1, VL2, VL3, pass, horizontal).
DATA(0x0068d360) unsigned char g_victorCcittWhiteCodes[104][2] = {
    { 0x35, 8 }, { 0x07, 6 }, { 0x07, 4 }, { 0x08, 4 }, { 0x0b, 4 }, { 0x0c, 4 }, { 0x0e, 4 }, { 0x0f, 4 },
    { 0x13, 5 }, { 0x14, 5 }, { 0x07, 5 }, { 0x08, 5 }, { 0x08, 6 }, { 0x03, 6 }, { 0x34, 6 }, { 0x35, 6 },
    { 0x2a, 6 }, { 0x2b, 6 }, { 0x27, 7 }, { 0x0c, 7 }, { 0x08, 7 }, { 0x17, 7 }, { 0x03, 7 }, { 0x04, 7 },
    { 0x28, 7 }, { 0x2b, 7 }, { 0x13, 7 }, { 0x24, 7 }, { 0x18, 7 }, { 0x02, 8 }, { 0x03, 8 }, { 0x1a, 8 },
    { 0x1b, 8 }, { 0x12, 8 }, { 0x13, 8 }, { 0x14, 8 }, { 0x15, 8 }, { 0x16, 8 }, { 0x17, 8 }, { 0x28, 8 },
    { 0x29, 8 }, { 0x2a, 8 }, { 0x2b, 8 }, { 0x2c, 8 }, { 0x2d, 8 }, { 0x04, 8 }, { 0x05, 8 }, { 0x0a, 8 },
    { 0x0b, 8 }, { 0x52, 8 }, { 0x53, 8 }, { 0x54, 8 }, { 0x55, 8 }, { 0x24, 8 }, { 0x25, 8 }, { 0x58, 8 },
    { 0x59, 8 }, { 0x5a, 8 }, { 0x5b, 8 }, { 0x4a, 8 }, { 0x4b, 8 }, { 0x32, 8 }, { 0x33, 8 }, { 0x34, 8 },
    { 0x1b, 5 }, { 0x12, 5 }, { 0x17, 6 }, { 0x37, 7 }, { 0x36, 8 }, { 0x37, 8 }, { 0x64, 8 }, { 0x65, 8 },
    { 0x68, 8 }, { 0x67, 8 }, { 0xcc, 9 }, { 0xcd, 9 }, { 0xd2, 9 }, { 0xd3, 9 }, { 0xd4, 9 }, { 0xd5, 9 },
    { 0xd6, 9 }, { 0xd7, 9 }, { 0xd8, 9 }, { 0xd9, 9 }, { 0xda, 9 }, { 0xdb, 9 }, { 0x98, 9 }, { 0x99, 9 },
    { 0x9a, 9 }, { 0x18, 6 }, { 0x9b, 9 }, { 0x08, 11 }, { 0x0c, 11 }, { 0x0d, 11 }, { 0x12, 12 }, { 0x13, 12 },
    { 0x14, 12 }, { 0x15, 12 }, { 0x16, 12 }, { 0x17, 12 }, { 0x1c, 12 }, { 0x1d, 12 }, { 0x1e, 12 }, { 0x1f, 12 }
};
DATA(0x0068d430) unsigned char g_victorCcittBlackCodes[104][2] = {
    { 0x37, 10 }, { 0x02, 3 }, { 0x03, 2 }, { 0x02, 2 }, { 0x03, 3 }, { 0x03, 4 }, { 0x02, 4 }, { 0x03, 5 },
    { 0x05, 6 }, { 0x04, 6 }, { 0x04, 7 }, { 0x05, 7 }, { 0x07, 7 }, { 0x04, 8 }, { 0x07, 8 }, { 0x18, 9 },
    { 0x17, 10 }, { 0x18, 10 }, { 0x08, 10 }, { 0x67, 11 }, { 0x68, 11 }, { 0x6c, 11 }, { 0x37, 11 }, { 0x28, 11 },
    { 0x17, 11 }, { 0x18, 11 }, { 0xca, 12 }, { 0xcb, 12 }, { 0xcc, 12 }, { 0xcd, 12 }, { 0x68, 12 }, { 0x69, 12 },
    { 0x6a, 12 }, { 0x6b, 12 }, { 0xd2, 12 }, { 0xd3, 12 }, { 0xd4, 12 }, { 0xd5, 12 }, { 0xd6, 12 }, { 0xd7, 12 },
    { 0x6c, 12 }, { 0x6d, 12 }, { 0xda, 12 }, { 0xdb, 12 }, { 0x54, 12 }, { 0x55, 12 }, { 0x56, 12 }, { 0x57, 12 },
    { 0x64, 12 }, { 0x65, 12 }, { 0x52, 12 }, { 0x53, 12 }, { 0x24, 12 }, { 0x37, 12 }, { 0x38, 12 }, { 0x27, 12 },
    { 0x28, 12 }, { 0x58, 12 }, { 0x59, 12 }, { 0x2b, 12 }, { 0x2c, 12 }, { 0x5a, 12 }, { 0x66, 12 }, { 0x67, 12 },
    { 0x0f, 10 }, { 0xc8, 12 }, { 0xc9, 12 }, { 0x5b, 12 }, { 0x33, 12 }, { 0x34, 12 }, { 0x35, 12 }, { 0x6c, 13 },
    { 0x6d, 13 }, { 0x4a, 13 }, { 0x4b, 13 }, { 0x4c, 13 }, { 0x4d, 13 }, { 0x72, 13 }, { 0x73, 13 }, { 0x74, 13 },
    { 0x75, 13 }, { 0x76, 13 }, { 0x77, 13 }, { 0x52, 13 }, { 0x53, 13 }, { 0x54, 13 }, { 0x55, 13 }, { 0x5a, 13 },
    { 0x5b, 13 }, { 0x64, 13 }, { 0x65, 13 }, { 0x08, 11 }, { 0x0c, 11 }, { 0x0d, 11 }, { 0x12, 12 }, { 0x13, 12 },
    { 0x14, 12 }, { 0x15, 12 }, { 0x16, 12 }, { 0x17, 12 }, { 0x1c, 12 }, { 0x1d, 12 }, { 0x1e, 12 }, { 0x1f, 12 }
};
DATA(0x0068d500) unsigned char g_victorCcittModeCodes[9][2] = {
    { 0x03, 7 }, { 0x03, 6 }, { 0x03, 3 }, { 0x01, 1 }, { 0x02, 3 }, { 0x02, 6 }, { 0x02, 7 }, { 0x01, 4 },
    { 0x01, 3 }
};
// The TIFF 6.0 IFD image a writer fills: 19 entries ending with next-IFD 0.
DATA(0x0068d518) VictorTiffIfd g_victorTiffIfdTemplate = {
    19,
    {
        { 0xfe, 4, 0x1, 0x0 },
        { 0xff, 3, 0x1, 0x1 },
        { 0x100, 3, 0x1, 0xff },
        { 0x101, 3, 0x1, 0xff },
        { 0x102, 3, 0xff, 0xff },
        { 0x103, 3, 0x1, 0xff },
        { 0x106, 3, 0x1, 0xff },
        { 0x10d, 2, 0x10, 0xff },
        { 0x111, 4, 0xff, 0xff },
        { 0x115, 3, 0x1, 0xff },
        { 0x116, 4, 0x1, 0xff },
        { 0x117, 4, 0xff, 0xff },
        { 0x11a, 5, 0x1, 0xff },
        { 0x11b, 5, 0x1, 0xff },
        { 0x11c, 3, 0x1, 0x1 },
        { 0x128, 3, 0x1, 0xff },
        { 0x129, 3, 0x2, 0xff },
        { 0x13d, 3, 0x1, 0x1 },
        { 0x140, 3, 0xff, 0xff }
    },
    0
};
// TIFF 6.0 little-endian file header: "II", 42, first IFD at offset 8. The
// format fixes the header at these eight bytes.
DATA(0x0068d650) unsigned char g_victorTiffHeader[] = { 'I', 'I', 42, 0, 8, 0, 0, 0 };
// TGA 2.0 footer signature and the software name.
DATA(0x0068d740) char g_victorTgaSignature[18] = "TRUEVISION-XFILE.";
DATA(0x0068d754) const char* g_victorSoftwareName =
    DATA_COMPGEN(0x0068d758, victorImageText, "Victor Image");
// Low-bit masks (1 << n) - 1 for n = 0..8.
DATA(0x0068d770) unsigned char g_victorLowBitMasks[9] = {
    0, 1, 3, 7, 15, 31, 63, 127, 255
};

// Victor .bss: the eight module locks, in cleanup order.
DATA(0x006aad38) VictorLock g_victorLock0;
DATA(0x006aad60) VictorLock g_victorLock1;
DATA(0x006aad98) VictorLock g_victorLock2;
DATA(0x006aade0) VictorLock g_victorLock3;
DATA(0x006aafe8) VictorLock g_victorLock4;
DATA(0x006ab0c0) VictorLock g_victorLock5;
DATA(0x006ab100) VictorLock g_victorLock6;
DATA(0x006ab140) VictorLock g_victorLock7;

VA(0x00603590, 0x25)  // anchor-caller Bitmap24Bit/Bitmap816::importPCXFile; external Victor library
int __stdcall allocimage(imgdes* image, int width, int height, int bitsPerPixel)
{
    return victorAllocateImage(image, width, height, bitsPerPixel,
                               g_victorUseDibSection);
}

// Retail-only allocation worker. One global-memory block owns the header
// and palette, plus pixels unless a DIB section supplies the pixel buffer.
// Dimensions are checked for zero only; the signed stride arithmetic and
// failure cleanup below follow the pinned executable's actual contract.
VA(0x006035c0, 0x1d6)  // anchor-caller allocimage + imgdes / bitmap header / Win32 allocation
int __cdecl victorAllocateImage(imgdes* image, int width, int height,
                               int bitsPerPixel, unsigned int useDibSection)
{
    memset(image, 0, sizeof(*image));
    if (bitsPerPixel == victorFourBitColor)
        bitsPerPixel = victorIndexedColor;
    if (bitsPerPixel != victorMonochrome && bitsPerPixel != victorIndexedColor
        && bitsPerPixel != victorTrueColor)
        return victorUnsupportedBitDepth;
    if (!width || !height)
        return -1;
    image->m_colors = bitsPerPixel == victorTrueColor ? 0 : 1 << bitsPerPixel;
    int stride = ((bitsPerPixel * width + 31) >> 3) & ~3;
    unsigned int paletteBytes = image->m_colors * sizeof(RGBQUAD);
    int imageBytes = stride * height;
    unsigned int bytes = sizeof(BITMAPINFOHEADER) + paletteBytes;
    if (!useDibSection)
        bytes += imageBytes;
    BITMAPINFOHEADER* header = static_cast<BITMAPINFOHEADER*>(
        GlobalLock(GlobalAlloc(GMEM_MOVEABLE, bytes)));
    if (!header)
        return -14;
    image->m_bmh = header;
    memset(header, 0, sizeof(*header));
    image->m_bmh->biSize = sizeof(BITMAPINFOHEADER);
    image->m_bmh->biWidth = width;
    image->m_bmh->biHeight = height;
    image->m_bmh->biBitCount = bitsPerPixel;
    image->m_bmh->biPlanes = 1;
    image->m_bmh->biCompression = BI_RGB;
    image->m_bmh->biSizeImage = imageBytes;
    image->m_bmh->biClrImportant = image->m_colors;
    image->m_bmh->biClrUsed = image->m_bmh->biClrImportant;
    image->m_palette = static_cast<RGBQUAD*>(static_cast<void*>(header + 1));
    victorInitializeGrayscalePalette(image);
    if (!useDibSection) {
        image->m_ibuff = static_cast<unsigned char*>(static_cast<void*>(header + 1))
            + paletteBytes;
    } else {
        HWND desktop = GetDesktopWindow();
        HDC dc = GetDC(desktop);
        image->m_bitmap = g_victorCreateDibSection(dc,
            static_cast<const BITMAPINFO*>(static_cast<const void*>(header)),
            DIB_RGB_COLORS, static_cast<void**>(static_cast<void*>(&image->m_ibuff)),
            NULL, 0);
        ReleaseDC(desktop, dc);
        if (!image->m_bitmap || !image->m_ibuff) {
            if (image->m_bitmap)
                DeleteObject(image->m_bitmap);
            GlobalUnlock(GlobalHandle(header));
            GlobalFree(GlobalHandle(header));
            return -14;
        }
    }
    image->m_buffwidth = stride;
    image->m_endx = image->m_bmh->biWidth - 1;
    image->m_endy = image->m_bmh->biHeight - 1;
    return 0;
}

// The public release API validates the readable header range, releases
// global-memory and optional DIB-section ownership, then clears imgdes.
// This ordinary C++ reconstruction retains the two GlobalHandle calls.
// Dreamcast's PCX stub independently confirms the public void result;
// the incidental EAX value does not justify changing that interface.
VA(0x006037a0, 0x6e)  // anchor-caller PCX importers + Win32 ownership calls; external Victor library
void __stdcall freeimage(imgdes* image)
{
    unsigned int bytes;
    if (image->m_bitmap)
        bytes = image->m_colors * sizeof(RGBQUAD) + sizeof(BITMAPINFOHEADER);
    else
        bytes = image->m_bmh->biSizeImage;
    if (!IsBadReadPtr(image->m_bmh, bytes)) {
        if (image->m_bmh) {
            GlobalUnlock(GlobalHandle(image->m_bmh));
            GlobalFree(GlobalHandle(image->m_bmh));
        }
        if (image->m_bitmap)
            DeleteObject(image->m_bitmap);
        memset(image, 0, sizeof(*image));
    }
}

// Retail loadpcx calls this after filling an indexed palette. The optional
// DIB receives the same RGBQUAD rows through the recovered callback ABI.
// The DC stubs contain no implementation of this Windows-only operation.
// Only a failed memory DC sets the status, which lives in a stack slot.
VA(0x00603810, 0x9a)  // anchor-caller loadpcx + GDI selection/cleanup + imgdes layout
int __stdcall victorUploadPalette(imgdes* image)
{
    int status = 0;
    if (image->m_bitmap && image->m_colors) {
        HWND desktop = GetDesktopWindow();
        HDC desktopDc = GetDC(desktop);
        HDC memoryDc = CreateCompatibleDC(desktopDc);
        if (memoryDc) {
            HGDIOBJ previous = SelectObject(memoryDc, image->m_bitmap);
            g_victorSetDibColorTable(memoryDc, 0, image->m_colors, image->m_palette);
            SelectObject(memoryDc, previous);
            DeleteDC(memoryDc);
        } else {
            status = -14;
        }
        ReleaseDC(desktop, desktopDc);
    }
    return status;
}

VA(0x006038b0, 0xea)  // anchor-caller victorValidateBitmap + imgdes offsets / IsBadReadPtr
int __stdcall victorValidateImage(imgdes* image)
{
    int status = -42;
    if (!IsBadReadPtr(image->m_ibuff, 1)) {
        status = 0;
        BITMAPINFOHEADER* header = image->m_bmh;
        unsigned int maxWidth = header->biBitCount == victorMonochrome ? 65535 : 32768;
        if (!image->m_ibuff || !header)
            return -1;
        if (image->m_stx > image->m_endx) {
            unsigned int value = image->m_endx;
            image->m_endx = image->m_stx;
            image->m_stx = value;
        }
        if (image->m_sty > image->m_endy) {
            unsigned int value = image->m_endy;
            image->m_endy = image->m_sty;
            image->m_sty = value;
        }
        if (image->m_endx >= static_cast<unsigned int>(header->biWidth)
            || image->m_endx >= maxWidth
            || image->m_endy >= static_cast<unsigned int>(header->biHeight)
            || image->m_endy >= 32768)
            return -1;
        if (static_cast<unsigned int>(header->biBitCount * header->biWidth / 8)
            > image->m_buffwidth)
            return -1;
        if (header->biBitCount != victorIndexedColor
            && header->biBitCount != victorTrueColor)
            status = victorUnsupportedBitDepth;
        if (header->biCompression)
            return -12;
    }
    return status;
}

VA(0x006039a0, 0x20)  // anchor-caller + bitmap-header semantics; external Victor library
int __stdcall victorValidateBitmap(imgdes* image)
{
    int status = victorValidateImage(image);
    if (status == victorUnsupportedBitDepth && image->m_bmh->biBitCount == 1)
        status = 0;
    return status;
}

// Both the allocation worker and loadpcx use this grayscale initialization.
// Retail writes red, green, blue, reserved in that order and expands the
// palette-upload helper, discarding its status but preserving GDI cleanup.
// /Ob2 expands the upload helper. The bitmap-header snapshot keeps retail's
// depth reload; index and shade advance together in the for clause.
VA(0x006039c0, 0xfc)  // anchor-callers alloc/loadpcx + RGBQUAD stores / GDI cleanup
void __stdcall victorInitializeGrayscalePalette(imgdes* image)
{
    int step = 255;
    if (image->m_palette && image->m_bmh->biBitCount != victorTrueColor) {
        BITMAPINFOHEADER* header = image->m_bmh;
        int colors = header->biClrUsed;
        if (!colors && header->biBitCount != victorTrueColor)
            colors = 1 << header->biBitCount;
        image->m_colors = colors;
        if (colors > 2) {
            image->m_imgtype = 1;
            step = 256 / colors;
        }
        int i, shade;
        for (i = 0, shade = 0; i < image->m_colors; ++i, shade += step) {
            image->m_palette[i].rgbRed = shade;
            image->m_palette[i].rgbGreen = shade;
            image->m_palette[i].rgbBlue = shade;
            image->m_palette[i].rgbReserved = 0;
        }
        victorUploadPalette(image);
    }
}

// Provisional helper name. flipimage calls this at 0x603b7e with output
// height then width. The unsigned inclusive extents and ordered stores are
// byte-proven. Its returned EAX is overwritten by the caller, so the source
// models a void result rather than inventing a value to reserve EAX.
VA(0x00603ac0, 0x4d)  // anchor-caller flipimage + unsigned extent semantics; external Victor library
void __cdecl victorMinimumDimensions(imgdes* first, imgdes* second,
                                      unsigned int* height, unsigned int* width)
{
    unsigned int secondWidth = second->m_endx - second->m_stx + 1;
    unsigned int secondHeight = second->m_endy - second->m_sty + 1;
    *width = first->m_endx - first->m_stx + 1;
    *height = first->m_endy - first->m_sty + 1;
    if (*width > secondWidth)
        *width = secondWidth;
    if (*height > secondHeight)
        *height = secondHeight;
}

// Victor's module cleanups are reached only through g_victorModuleCleanups
// (0x68d278), which nothing in the linked image reads; the lock records have
// no other reader either. The owning Victor objects are unidentified, so the
// bodies sit with the table. The first entry is a module with nothing to
// release: 0x603b10 is a bare `ret`.
VA(0x00603b10, 0x1)  // g_victorModuleCleanups[0]; external Victor library
void __cdecl victorReleaseNothing()
{
}

// Public Victor PCX metadata reader, corroborated by the Dreamcast API
// stub; the Windows body is retail-only. Open failure leaves output alone;
// successful open clears it even when the header signature is rejected.
// Retail deliberately ignores the short-read result and always closes an
// opened file. Preserve that behavior and the planar four-bit normalization.
VA(0x006042a0, 0x127)  // anchor-caller PCX importers + OpenFile/header offsets
int __stdcall pcxinfo(const char* filename, PcxData* data)
{
    int status = 0;
    OFSTRUCT fileInfo;
    VictorPcxHeader header;
    HFILE file = OpenFile(filename, &fileInfo, OF_SHARE_DENY_WRITE);
    if (file < 0)
        return -4;
    memset(data, 0, sizeof(*data));
    _lread(file, &header, sizeof(header));
    if (header.m_manufacturer == victorPcxManufacturer
        && header.m_encoding == victorPcxRleEncoding) {
        data->m_pcXvers = header.m_version;
        data->m_width = header.m_maxX - header.m_minX + 1;
        data->m_length = header.m_maxY - header.m_minY + 1;
        data->m_nplanes = header.m_planes;
        data->m_bytesPerLine = header.m_bytesPerLine;
        data->m_palInt = header.m_paletteType;
        data->m_bpPixel = header.m_bitsPerPixel;
        data->m_vbitcount = data->m_bpPixel * data->m_nplanes;
        if ((header.m_bitsPerPixel == victorMonochrome
             && header.m_planes == victorPcxEgaPlanes)
            || header.m_bitsPerPixel == victorFourBitColor)
            data->m_vbitcount = victorIndexedColor;
    } else {
        status = -16;
    }
    _lclose(file);
    return status;
}

// Retail-only PCX palette reader, called by loadpcx. Version 2/5 palettes
// are accepted except planar RGB. A missing extended palette marker falls
// back to the 16 header entries. OpenFile's strictly-positive test and
// returned color count on open failure are preserved from retail.
// An extended palette jumps straight to the conversion; a missing marker
// sets the header colour count itself, which VC5 shares with the clamp.
VA(0x006043d0, 0x13c)  // anchor-caller loadpcx + PCX palette marker/seek offsets
int __stdcall victorReadPcxPalette(const char* filename, RGBQUAD* palette)
{
    PcxData data;
    OFSTRUCT fileInfo;
    int colors = 0;
    int status = pcxinfo(filename, &data);
    if (status)
        return status;
    if ((data.m_pcXvers == victorPcxVersionThree
         || data.m_pcXvers == victorPcxVersionWithPalette)
        && (data.m_bpPixel != victorIndexedColor || data.m_nplanes != victorPcxRgbPlanes)) {
        colors = 1 << (data.m_bpPixel * data.m_nplanes);
        unsigned char* buffer = static_cast<unsigned char*>(calloc(770, 1));
        if (!buffer)
            return -14;
        HFILE file = OpenFile(filename, &fileInfo, OF_SHARE_DENY_WRITE);
        if (file > 0) {
            if (colors == victorPcxExtendedColors) {
                _llseek(file, -769, 2);
                _lread(file, buffer, 769);
                if (*buffer == victorPcxExtendedPaletteMarker)
                    goto copy;
                colors = victorPcxHeaderColors;
            } else if (colors > victorPcxHeaderColors) {
                colors = victorPcxHeaderColors;
            }
            _llseek(file, 16, 0);
            _lread(file, buffer + 1, colors * 3);
        copy:
            unsigned char* source = buffer + 1;
            memset(palette, 0, colors * sizeof(RGBQUAD));
            for (int index = 0; index < colors; ++index) {
                palette->rgbRed = source[0];
                palette->rgbGreen = source[1];
                palette->rgbBlue = source[2];
                source += 3;
                ++palette;
            }
            _lclose(file);
        }
        free(buffer);
    }
    return colors;
}

// The eight lock cleanups, one per VictorLock in address order: 0x604620 +
// 0x20*n releases g_victorLock<n> (g_victorModuleCleanups[8-n]). Each is 31
// bytes: test the initialized flag at +0x18, DeleteCriticalSection through
// the IAT, clear the flag.
VA(0x00604620, 0x1f)  // g_victorModuleCleanups[8]; external Victor library
void __cdecl victorDestroyLock0()
{
    if (g_victorLock0.m_initialized) {
        DeleteCriticalSection(&g_victorLock0.m_section);
        g_victorLock0.m_initialized = 0;
    }
}

VA(0x00604640, 0x1f)  // g_victorModuleCleanups[7]; external Victor library
void __cdecl victorDestroyLock1()
{
    if (g_victorLock1.m_initialized) {
        DeleteCriticalSection(&g_victorLock1.m_section);
        g_victorLock1.m_initialized = 0;
    }
}

VA(0x00604660, 0x1f)  // g_victorModuleCleanups[6]; external Victor library
void __cdecl victorDestroyLock2()
{
    if (g_victorLock2.m_initialized) {
        DeleteCriticalSection(&g_victorLock2.m_section);
        g_victorLock2.m_initialized = 0;
    }
}

VA(0x00604680, 0x1f)  // g_victorModuleCleanups[5]; external Victor library
void __cdecl victorDestroyLock3()
{
    if (g_victorLock3.m_initialized) {
        DeleteCriticalSection(&g_victorLock3.m_section);
        g_victorLock3.m_initialized = 0;
    }
}

VA(0x006046a0, 0x1f)  // g_victorModuleCleanups[4]; external Victor library
void __cdecl victorDestroyLock4()
{
    if (g_victorLock4.m_initialized) {
        DeleteCriticalSection(&g_victorLock4.m_section);
        g_victorLock4.m_initialized = 0;
    }
}

VA(0x006046c0, 0x1f)  // g_victorModuleCleanups[3]; external Victor library
void __cdecl victorDestroyLock5()
{
    if (g_victorLock5.m_initialized) {
        DeleteCriticalSection(&g_victorLock5.m_section);
        g_victorLock5.m_initialized = 0;
    }
}

VA(0x006046e0, 0x1f)  // g_victorModuleCleanups[2]; external Victor library
void __cdecl victorDestroyLock6()
{
    if (g_victorLock6.m_initialized) {
        DeleteCriticalSection(&g_victorLock6.m_section);
        g_victorLock6.m_initialized = 0;
    }
}

VA(0x00604700, 0x1f)  // g_victorModuleCleanups[1]; external Victor library
void __cdecl victorDestroyLock7()
{
    if (g_victorLock7.m_initialized) {
        DeleteCriticalSection(&g_victorLock7.m_section);
        g_victorLock7.m_initialized = 0;
    }
}
