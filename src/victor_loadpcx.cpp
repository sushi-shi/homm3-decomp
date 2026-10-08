// Reconstructed Victor PCX loader. The original library object name is
// unknown. Retail retains all helper calls, so this provisional caller
// unit uses canonical declarations without exposing their implementations.
#include "va.h"

#include <stdlib.h>
#include <string.h>

#include "victor.h"

// Retail RGB triples and per-decoding-mode scratch-row multipliers.
DATA(0x0068d2a0) unsigned char g_victorPcxDefaultPalette[48] = {
    0,0,0, 0,0,168, 0,168,0, 0,168,168,
    168,0,0, 168,0,168, 168,84,0, 168,168,168,
    84,84,84, 84,84,252, 84,252,84, 84,252,252,
    252,84,84, 252,84,252, 252,252,84, 252,252,252
};
DATA(0x0068d2d0) unsigned char g_victorPcxScratchRows[5] = {1,1,1,2,2};

// Dreamcast confirms the public fname/desimg API but contains only a stub.
// Retail proves five decode modes, bounded input refills, paired RGB plane
// accumulation and the palette fallback below. Short input ends decoding
// without introducing a new error; the remaining palette work still runs.
// The nibble arm jumps to the indexed row copy, which the converted RGB
// row falls into; partial RGB planes continue without advancing the row.
// Every handled row steps the destination up one stride, as retail does.
// Victor is C: every local is declared at the top of the function, and
// VC5 allocates registers by that declaration order (the input buffer
// precedes its read index, the refill counts precede both). The refill
// point is the buffered count less two encoded rows, unless that is not
// positive.
VA(0x00603e00, 0x494)  // anchor-caller PCX importers + RLE/plane/palette helper sequence
int __stdcall loadpcx(const char* filename, imgdes* image)
{
    int status;
    PcxData data;
    HFILE file;
    OFSTRUCT fileInfo;
    unsigned int height;
    unsigned int width;
    int mode;
    unsigned int rowsRemaining;
    unsigned int refillAt;
    int buffered;
    int encodedRowBytes;
    int planesRemaining;
    int remaining;
    int limit;
    unsigned char* input;
    int consumed;
    unsigned char* decoded;
    unsigned char* planeStart;
    unsigned char* plane;
    int decodeBytes;
    unsigned int offset;
    unsigned char* destination;
    unsigned int copyBytes;
    int pixels;
    unsigned char* packed;
    int value;
    int i;

    status = victorValidateBitmap(image);
    if (!status) {
        status = pcxinfo(filename, &data);
        if (status)
            return status;
        file = OpenFile(filename, &fileInfo, OF_SHARE_DENY_WRITE);
        if (file < 0)
            return -4;
        height = image->m_endy - image->m_sty + 1;
        if (height > data.m_length)
            height = data.m_length;
        width = image->m_endx - image->m_stx + 1;
        if (width > data.m_width)
            width = data.m_width;
        mode = victorPcxInvalidMode;
        if (data.m_nplanes == victorPcxSinglePlane) {
            if (data.m_bpPixel == victorIndexedColor)
                mode = victorPcxIndexedMode;
            else if (data.m_bpPixel == victorMonochrome)
                mode = victorPcxMonochromeMode;
            else if (data.m_bpPixel == victorFourBitColor)
                mode = victorPcxNibbleMode;
        } else if (data.m_nplanes == victorPcxEgaPlanes && data.m_bpPixel == victorMonochrome) {
            mode = victorPcxFourPlaneMode;
        } else if (data.m_nplanes == victorPcxRgbPlanes && data.m_bpPixel == victorIndexedColor) {
            mode = victorPcxRgbMode;
        }
        status = mode;
        if (mode != victorPcxInvalidMode) {
            _llseek(file, 128, 0);
            rowsRemaining = height;
            consumed = 0;
            refillAt = 0;
            buffered = 0;
            encodedRowBytes = data.m_bytesPerLine * data.m_nplanes;
            planesRemaining = victorPcxRgbPlanes;
            input = static_cast<unsigned char*>(malloc(victorPcxInputCapacity
                + g_victorPcxScratchRows[mode - 1] * encodedRowBytes));
            if (!input) {
                status = -14;
            } else {
                decoded = input + victorPcxInputCapacity;
                planeStart = input + victorPcxInputCapacity + encodedRowBytes;
                plane = planeStart;
                decodeBytes = mode == victorPcxRgbMode ? data.m_bytesPerLine : encodedRowBytes;
                offset = (image->m_bmh->biHeight - image->m_sty - 1)
                    * image->m_buffwidth + (image->m_bmh->biBitCount * image->m_stx >> 3);
                destination = image->m_ibuff + offset;
                copyBytes = (image->m_bmh->biBitCount >> 3) * width;
                while (rowsRemaining) {
                    if (consumed >= refillAt) {
                        remaining = buffered - consumed >= 0 ? buffered - consumed : 0;
                        memcpy(input, input + consumed, remaining);
                        buffered = _lread(file, input + remaining,
                                          victorPcxInputCapacity - remaining) + remaining;
                        if (!buffered)
                            break;
                        limit = buffered - 2 * encodedRowBytes;
                        refillAt = limit <= 0 ? buffered : limit;
                        consumed = 0;
                    }
                    consumed += victorDecodeRleBytes(decoded, input + consumed, decodeBytes);
                    switch (mode) {
                    case victorPcxNibbleMode:
                        pixels = width;
                        packed = decoded + (pixels + 1) / 2 - 1;
                        do {
                            if (!(pixels & 1)) {
                                value = *packed & 15;
                            } else {
                                value = *packed >> 4;
                                --packed;
                            }
                            decoded[pixels - 1] = value;
                        } while (--pixels);
                        goto copyRow;
                    case victorPcxFourPlaneMode:
                        victorUnpackFourPlanes(destination, decoded, data.m_bytesPerLine, width);
                        break;
                    case victorPcxMonochromeMode:
                        victorInsertBits(destination, decoded, image->m_stx, width);
                        break;
                    case victorPcxRgbMode:
                        memcpy(plane, decoded, width);
                        plane += width;
                        if (--planesRemaining)
                            continue;
                        planesRemaining = victorPcxRgbPlanes;
                        plane = planeStart;
                        victorConvertRgbPlanesToBgr(decoded, planeStart, width);
                    case victorPcxIndexedMode:
                    copyRow:
                        memcpy(destination, decoded, copyBytes);
                        break;
                    }
                    destination -= image->m_buffwidth;
                    --rowsRemaining;
                }
                free(input);
                status = 0;
            }
        }
        _lclose(file);
        if (!status) {
            image->m_colors = image->m_palette ? victorReadPcxPalette(filename, image->m_palette) : 0;
            if (image->m_colors == victorPcxMonochromeColors
                && image->m_palette[0].rgbRed == image->m_palette[1].rgbRed
                && image->m_palette[0].rgbGreen == image->m_palette[1].rgbGreen
                && image->m_palette[0].rgbBlue == image->m_palette[1].rgbBlue)
                image->m_colors = 0;
            image->m_imgtype &= ~victorImageGrayscale;
            if (!image->m_colors) {
                if (image->m_palette && (mode == victorPcxFourPlaneMode || mode == victorPcxNibbleMode)) {
                    image->m_colors = victorPcxHeaderColors;
                    for (i = 0; i < image->m_colors; ++i) {
                        image->m_palette[i].rgbRed = g_victorPcxDefaultPalette[i * 3];
                        image->m_palette[i].rgbGreen = g_victorPcxDefaultPalette[i * 3 + 1];
                        image->m_palette[i].rgbBlue = g_victorPcxDefaultPalette[i * 3 + 2];
                    }
                } else {
                    victorInitializeGrayscalePalette(image);
                }
            }
            if (image->m_bitmap)
                victorUploadPalette(image);
        }
    }
    return status;
}
