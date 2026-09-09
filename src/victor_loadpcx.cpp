// Reconstructed Victor PCX loader. The original library object name is
// unknown. Retail retains all helper calls, so this provisional caller
// unit uses canonical declarations without exposing their implementations.
#include <va.h>
#include <stdlib.h>
#include <string.h>
#include "victor.h"

// Retail RGB triples and per-decoding-mode scratch-row multipliers.
DATA(0x0068d2a0) const unsigned char g_victorPcxDefaultPalette[48] = {
    0,0,0, 0,0,168, 0,168,0, 0,168,168,
    168,0,0, 168,0,168, 168,84,0, 168,168,168,
    84,84,84, 84,84,252, 84,252,84, 84,252,252,
    252,84,84, 252,84,252, 252,252,84, 252,252,252
};
DATA(0x0068d2d0) const unsigned char g_victorPcxScratchRows[5] = {1,1,1,2,2};

// Dreamcast confirms the public fname/desimg API but contains only a stub.
// Retail proves five decode modes, bounded input refills, paired RGB plane
// accumulation and the palette fallback below. Short input ends decoding
// without introducing a new error; the remaining palette work still runs.
// Residual 78.45%: all 15 real calls agree; the switch-table self-reference
// moves with the body. Retail's branchless refill clamp, validation exits and
// full-width shared nibble value recover the CFG and make the 21-instruction
// nibble loop exact. Bounded status, declaration, address and switch-exit
// families leave a register wall: retail carries consumed in ESI and reloads
// image around the decode loop, while VC6 carries image in ESI and spills
// consumed. Explicit shared switch exits and six classifier forms are flat.
// Hoisting and reordering all prelude, decode-state and allocation-success
// declarations across a complete 64-state old-C family emits one object.
VA(0x00603e00, 0x494)  // anchor-caller PCX importers + RLE/plane/palette helper sequence
int __stdcall loadpcx(const char* filename, imgdes* image)
{
    int status = victorValidateBitmap(image);
    if (status)
        return status;
    PcxData data;
    status = pcxinfo(filename, &data);
    if (status)
        return status;
    OFSTRUCT fileInfo;
    HFILE file = OpenFile(filename, &fileInfo, OF_SHARE_DENY_WRITE);
    if (file < 0)
        return -4;
    unsigned int height = image->m_endy - image->m_sty + 1;
    if (height > data.m_length)
        height = data.m_length;
    unsigned int width = image->m_endx - image->m_stx + 1;
    if (width > data.m_width)
        width = data.m_width;
    int mode = victorPcxInvalidMode;
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
        unsigned int refillAt = 0;
        int buffered = 0;
        unsigned int rowsRemaining = height;
        int planesRemaining = victorPcxRgbPlanes;
        int encodedRowBytes = data.m_bytesPerLine * data.m_nplanes;
        int consumed = 0;
        unsigned char* input = static_cast<unsigned char*>(malloc(victorPcxInputCapacity
            + g_victorPcxScratchRows[mode - 1] * encodedRowBytes));
        if (!input) {
            status = -14;
        } else {
            unsigned char* decoded = input + victorPcxInputCapacity;
            unsigned char* planeStart = decoded + encodedRowBytes;
            unsigned char* plane = planeStart;
            int decodeBytes = mode == victorPcxRgbMode ? data.m_bytesPerLine : encodedRowBytes;
            unsigned int offset = (image->m_bmh->biHeight - image->m_sty - 1)
                * image->m_buffwidth + (image->m_bmh->biBitCount * image->m_stx >> 3);
            unsigned int copyBytes = (image->m_bmh->biBitCount >> 3) * width;
            unsigned char* destination = image->m_ibuff + offset;
            while (rowsRemaining) {
                if (static_cast<unsigned int>(consumed) >= refillAt) {
                    int remaining = buffered - consumed >= 0 ? buffered - consumed : 0;
                    memcpy(input, input + consumed, remaining);
                    buffered = _lread(file, input + remaining,
                                      victorPcxInputCapacity - remaining) + remaining;
                    if (!buffered)
                        break;
                    refillAt = buffered;
                    if (buffered - 2 * encodedRowBytes > 0)
                        refillAt = buffered - 2 * encodedRowBytes;
                    consumed = 0;
                }
                consumed += victorDecodeRleBytes(decoded, input + consumed, decodeBytes);
                switch (mode) {
                case victorPcxNibbleMode: {
                    int pixels = width;
                    unsigned char* packed = decoded + (pixels + 1) / 2 - 1;
                    do {
                        int value;
                        if (!(pixels & 1)) {
                            value = *packed & 15;
                        } else {
                            value = *packed >> 4;
                            --packed;
                        }
                        decoded[pixels - 1] = value;
                    } while (--pixels);
                    goto copyRow;
                }
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
                    victorInterleaveRgbPlanes(decoded, planeStart, width);
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
                const unsigned char* color = g_victorPcxDefaultPalette;
                for (int i = 0; i < image->m_colors; ++i) {
                    image->m_palette[i].rgbRed = color[0];
                    image->m_palette[i].rgbGreen = color[1];
                    image->m_palette[i].rgbBlue = color[2];
                    color += 3;
                }
            } else {
                victorInitializePalette(image);
            }
        }
        if (image->m_bitmap)
            victorUploadPalette(image);
    }
    return status;
}
