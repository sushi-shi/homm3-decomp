// Reconstructed Victor vertical-flip caller. Original library object name
// is unknown; this filename is a provisional semantic grouping.
// Retail retains both validation calls, the dimension call and all four
// bit-range calls. Compiling with their bodies in victor.cpp under /Ob2
// expands all seven boundaries and scores 0%. The palette initializer needs
// /Ob2 with its upload helper visible, so that proven pair stays together.
// This unit keeps canonical shared declarations and ordinary helper calls.
#include <va.h>
#include <string.h>
#include <stdlib.h>
#include "victor.h"

// Public Victor vertical region flip. Retail reads paired rows through a
// temporary so source and destination may be the same image. Monochrome
// regions use two bit-range temporaries to preserve neighboring pixels.
// Dreamcast has only the public API stub; all Windows semantics are retail.
// Residual 77.86%: all nine named calls agree, named depth snapshots recover
// the retail compare exactly, and a destination-stride snapshot improves the
// allocation setup. Stack-local allocation, address arithmetic and status/
// return scheduling still differ. JSON-batched status, pointer, row-distance,
// declaration-order and register-hint families leave the nested scopes best;
// early validation and allocation returns score 76.21%.
// Boundary repair: a one-row region can otherwise step its top pointer
// before separate DIB storage. Stop after the last paired copy, in both
// depth branches. Three-form family: break 73.8105%, remaining-pair guard
// 69.2661%, unchecked 77.8629% (retained in HIST).
// The three-state relative-row family reproduces all objects: integral byte
// offsets score 61.1976%, multiplied row indices 48.6694%; neither improves.
VA(0x00603b20, 0x2d2)  // anchor-caller PCX importers + paired row/bit helper calls
int __stdcall flipimage(imgdes* source, imgdes* destination)
{
    int status = victorValidateBitmap(source);
    if (!status) {
        status = victorValidateBitmap(destination);
        if (!status) {
            unsigned short sourceDepth = source->m_bmh->biBitCount;
            unsigned short destinationDepth = destination->m_bmh->biBitCount;
            if (sourceDepth != destinationDepth)
                return victorUnsupportedBitDepth;
            unsigned int width, height;
            victorMinimumDimensions(source, destination, &height, &width);
            unsigned int rowBytes, allocationBytes;
            if (source->m_bmh->biBitCount == victorMonochrome) {
                rowBytes = (width + 7) >> 3;
                allocationBytes = rowBytes * 2 + 8;
            } else {
                rowBytes = (source->m_bmh->biBitCount >> 3) * width;
                allocationBytes = rowBytes;
            }
            unsigned char* temporary = static_cast<unsigned char*>(calloc(allocationBytes, 1));
            if (!temporary) {
                status = -14;
            } else {
                unsigned int rows = (height + 1) >> 1;
                unsigned short depth = source->m_bmh->biBitCount;
                int sourceBitOffset = depth * source->m_stx;
                unsigned int sourceOffset = (source->m_bmh->biHeight - source->m_sty - 1)
                * source->m_buffwidth + sourceBitOffset / 8;
                unsigned char* sourceTop = source->m_ibuff + sourceOffset;
                unsigned char* sourceBottom = source->m_ibuff + sourceOffset
                - (height - 1) * source->m_buffwidth;
                unsigned int destinationStride = destination->m_buffwidth;
                int destinationBitOffset = destination->m_bmh->biBitCount * destination->m_stx;
                unsigned int destinationOffset = (destination->m_bmh->biHeight - destination->m_sty - 1)
                * destinationStride + destinationBitOffset / 8;
                unsigned char* destinationTop = destination->m_ibuff + destinationOffset;
                unsigned char* destinationBottom = destination->m_ibuff + destinationOffset
                - (height - 1) * destinationStride;
                if (depth >= victorIndexedColor) {
                    while (rows--) {
                        memcpy(temporary, sourceTop, rowBytes);
                        memcpy(destinationTop, sourceBottom, rowBytes);
                        memcpy(destinationBottom, temporary, rowBytes);
                        if (!rows)
                            break;
                        sourceTop -= source->m_buffwidth;
                        destinationTop -= destination->m_buffwidth;
                        sourceBottom += source->m_buffwidth;
                        destinationBottom += destination->m_buffwidth;
                    }
                } else {
                    unsigned char* secondTemporary = temporary + rowBytes + 4;
                    while (rows--) {
                        victorExtractBits(temporary, sourceBottom, source->m_stx, width);
                        victorExtractBits(secondTemporary, sourceTop, source->m_stx, width);
                        victorInsertBits(destinationTop, temporary, destination->m_stx, width);
                        victorInsertBits(destinationBottom, secondTemporary, destination->m_stx, width);
                        if (!rows)
                            break;
                        sourceTop -= source->m_buffwidth;
                        destinationTop -= destination->m_buffwidth;
                        sourceBottom += source->m_buffwidth;
                        destinationBottom += destination->m_buffwidth;
                    }
                }
                free(temporary);
            }
        }
    }
    return status;
}
