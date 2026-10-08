// Reconstructed Victor PCX metadata readers. The original library object name
// is unknown. Retail's victor members follow the game objects in LINK's pull
// order: allocation/validation (victor.cpp), flipimage, loadpcx, then this
// pcxinfo pair, so the pair is its own library member.
#include "va.h"

#include <stdlib.h>
#include <string.h>

#include "victor.h"

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
                ++palette;
                source += 3;
            }
            _lclose(file);
        }
        free(buffer);
    }
    return colors;
}
