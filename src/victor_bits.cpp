// Reconstructed Victor bit-range helpers (Catenary Systems). The original
// library object name is unknown; this filename is a provisional semantic
// grouping. Retail links the two helpers after every other Victor function,
// and they reproduce retail register allocation only in a unit of their own
// (with the full victor.cpp before them VC5 swaps the insertion's final
// mask registers). flipimage and loadpcx call them out of line.
#include "va.h"

#include "victor.h"

// Retail masks preserve bits preceding/following an inclusive bit range.
DATA(0x00644120) const unsigned char g_victorLeadingBits[8] =
    { 0, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe };
DATA(0x00644128) const unsigned char g_victorTrailingBits[8] =
    { 0x7f, 0x3f, 0x1f, 0x0f, 7, 3, 1, 0 };

// Retail-only bit-range insertion, called by flipimage. The first and last
// destination bytes retain bits outside the inclusive range. Signed count
// division and the two-stage loop follow retail, including zero counts.
// It advances its destination and source parameters directly, like the
// paired extractor.
VA(0x00604720, 0x84)  // anchor-caller flipimage + paired bit-mask tables
void __stdcall victorInsertBits(unsigned char* destination,
                                const unsigned char* source, int offset, int count)
{
    int shift = offset & 7;
    unsigned char mask = g_victorTrailingBits[(shift + count - 1) & 7];
    unsigned char saved = destination[(shift + count - 1) / 8] & mask;
    *destination &= g_victorLeadingBits[shift];
    while (count > 0) {
        unsigned char bits = *source >> shift;
        count += shift - 8;
        *destination |= bits;
        if (count <= 0)
            break;
        *++destination = *source << (8 - shift);
        count -= shift;
        ++source;
    }
    *destination = (*destination & ~mask) | saved;
}

// Retail-only bit-range extraction, paired with victorInsertBits by
// flipimage. It left-aligns the source range and clears the trailing bits.
// Advancing the destination parameter itself (no cursor copy) reproduces
// retail's separate empty-count tail, which still addresses the parameter.
VA(0x006047b0, 0x73)  // anchor-caller flipimage + trailing bit-mask table
void __stdcall victorExtractBits(unsigned char* destination,
                                 const unsigned char* source, int offset, int count)
{
    offset &= 7;
    unsigned char mask = g_victorTrailingBits[(count - 1) & 7];
    while (count > 0) {
        *destination = *source << offset;
        count += offset - 8;
        if (count <= 0)
            break;
        *destination |= *++source >> (8 - offset);
        count -= offset;
        if (count <= 0)
            break;
        ++destination;
    }
    *destination &= ~mask;
}
