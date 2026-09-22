#include "va.h"

#include <string.h>
#include <limits>

#include "cspriteframe.h"

#include "palette.h"
#include "pcx.h"

// Static mask storage, set by SetPixelFormat before sprite drawing.
DATA(0x006968a4) TBlendMask CSpriteFrame::s_div2mask;
DATA(0x006968aa) unsigned short CSpriteFrame::s_div4mask;


// The retail destructor calls the common nothrow deallocator directly;
// this declaration keeps /GX from manufacturing an unwind frame.
__declspec(nothrow) void __cdecl operator delete(void* p);

// DC names these file-static constants kGeneralRLEOpaqueRunCode (const
// unsigned char) and kGeneralRLEMaxRunLength (const unsigned int), and retains
// numeric_limits<unsigned char>::max calls at cspriteframe.cpp:46/47
// (dc 0x745b0/0x745d8). Retail's CRT initializers
// 0x47c260/0x47c270 store 255/256 before any frame decoder copies the literal
// marker into its local static. Leaving the marker zero misreads transparent
// runs as literal pixels and corrupts interface sprites at startup.
DATA(0x006968a6) static const unsigned char g_generalRleOpaqueRunCode =
    (std::numeric_limits<unsigned char>::max)();
VA_COMPGEN(0x0047c260, 0x08, STATIC_CTOR, g_generalRleOpaqueRunCode)
DATA(0x006968b0) static const unsigned int g_generalRleMaxRunLength =
    (std::numeric_limits<unsigned char>::max)() + 1;
VA_COMPGEN(0x0047c270, 0x0b, STATIC_CTOR, g_generalRleMaxRunLength)

// Original: CSpriteFrame::CSpriteFrame; cspriteframe.cpp:67, dc 0x74600.
CSpriteFrame::CSpriteFrame()
    : resource(0, RESOURCE_TYPE_NONE),
      m_dataSize(0), m_imageSize(0), m_encodingMethod(eEncodeRaw),
      m_width(0), m_height(0), m_croppedWidth(0), m_croppedHeight(0),
      m_croppedX(0), m_croppedY(0), m_pitch(0), m_map(0)
{
}

VA(0x0047c2b0, 0xa7)
CSpriteFrame::CSpriteFrame(const char* name, int w, int h,
                           unsigned char* data, int csize,
                           TEncodingMethod encoding)
    : resource(name, RESOURCE_TYPE_SPRITE),
      m_imageSize(w * h), m_encodingMethod(encoding), m_width(w), m_height(h),
      m_croppedWidth(w), m_croppedHeight(h), m_croppedX(0), m_croppedY(0), m_pitch(w)
{
    m_dataSize = csize ? csize : m_imageSize;
    m_map = new unsigned char[m_dataSize];
    if (m_map)
        memcpy(m_map, data, m_dataSize);
}

VA(0x0047c360, 0xc9)
CSpriteFrame::CSpriteFrame(const char* name, int w, int h,
                           unsigned char* data, int csize,
                           TEncodingMethod encoding,
                           int cw, int ch, int cx, int cy)
    : resource(name, RESOURCE_TYPE_SPRITE),
      m_imageSize(cw * ch), m_encodingMethod(encoding), m_width(w), m_height(h),
      m_croppedWidth(cw), m_croppedHeight(ch), m_croppedX(cx), m_croppedY(cy),
      m_pitch(cw)
{
    m_dataSize = csize ? csize : m_imageSize;
    m_map = new unsigned char[m_dataSize];
    if (m_map)
        memcpy(m_map, data, m_dataSize);
}

// Original: CSpriteFrame::CSpriteFrame; cspriteframe.cpp:188, dc 0x747cc.
CSpriteFrame::CSpriteFrame(const char* name, unsigned char cropped)
    : resource(name, RESOURCE_TYPE_SPRITE),
      m_dataSize(0), m_imageSize(0), m_encodingMethod(eEncodeRaw),
      m_width(0), m_height(0), m_croppedWidth(0), m_croppedHeight(0),
      m_croppedX(0), m_croppedY(0), m_pitch(0), m_map(0)
{
    if (!cropped)
        importPCXFile(name);
    else
        importCroppedPCXFile(name);
}

#if 0  // @carcass

// E:\gamedcs\cspriteframe.cpp:202
// RETAIL_LOCATED(0x0047c430, 0x22)  // anchor-bracket, dc 0x7483c
void CSpriteFrame::~CSpriteFrame()
{
    // @stub
}

// E:\gamedcs\cspriteframe.cpp:245 - promoted to a live claim below.
#endif  // @carcass

VA_COMPGEN(0x0047c280, 0x21, SCALAR_DELETING_DTOR, CSpriteFrame)

VA(0x0047c430, 0x22)  // vtable 0x63d6bc + resource::~resource
CSpriteFrame::~CSpriteFrame()
{
    if (m_map)
        delete[] m_map;
}

// Original: CSpriteFrame::clear; cspriteframe.cpp:217, dc 0x748ac.
// Complete removes the DC DirectDraw surface tail. As in its retained
// destructor, the remaining map is always an owned byte allocation.
void CSpriteFrame::clear()
{
    m_width = 0;
    m_height = 0;
    m_croppedWidth = 0;
    m_croppedHeight = 0;
    m_croppedX = 0;
    m_croppedY = 0;
    m_pitch = 0;
    m_dataSize = 0;
    m_imageSize = 0;
    m_encodingMethod = eEncodeRaw;
    if (m_map) {
        delete[] m_map;
        m_map = 0;
    }
}

VA(0x0047c460, 0xF7)  // dc 0x74918
void CSpriteFrame::setPixelFormat(unsigned rmask, unsigned gmask,
                                  unsigned bmask)
{
    int i;
    int rBits = 0;
    int gBits = 0;
    int bits = 0;
    for (i = 0; i < 16; i++)
        if (rmask & (1 << i))
            rBits++;
    for (i = 0; i < 16; i++)
        if (gmask & (1 << i))
            gBits++;
    for (i = 0; i < 16; i++)
        if (bmask & (1 << i))
            bits++;

    s_div2mask.m_word = ((((1 << rBits) - 1) / 2) << (gBits + bits))
        | ((((1 << gBits) - 1) / 2) << bits)
        | (((1 << bits) - 1) / 2);
    s_div4mask = ((((1 << rBits) - 1) / 4) << (gBits + bits))
        | ((((1 << gBits) - 1) / 4) << bits)
        | (((1 << bits) - 1) / 4);
}

// Original: CSpriteFrame::importPCXFile; cspriteframe.cpp:283, dc 0x74a18.
// The ordinary file importer shares Complete's retained Victor PCX ABI with
// Bitmap816::importPCXFile. DC's DirectDraw surface descriptor is absent from
// the Complete frame layout; its owned byte buffer remains.
int CSpriteFrame::importPCXFile(const char* filename)
{
    PcxData pdat;
    imgdes pcxfile;
    int error = pcxinfo(filename, &pdat);
    if (error)
        return 1;

    m_width = pdat.m_width;
    m_height = pdat.m_length;
    m_pitch = pdat.m_width;
    m_dataSize = m_imageSize = m_width * m_height;
    m_croppedX = 0;
    m_croppedY = 0;
    m_croppedWidth = m_width;
    m_croppedHeight = m_height;
    m_map = new unsigned char[m_dataSize];
    if (!m_map)
        return 2;

    allocimage(&pcxfile, pdat.m_width, pdat.m_length,
               pdat.m_bpPixel * pdat.m_nplanes);
    loadpcx(filename, &pcxfile);
    flipimage(&pcxfile, &pcxfile);
    for (int y = 0; y < m_height; ++y)
        memcpy(m_map + y * m_width,
               pcxfile.m_ibuff + y * pcxfile.m_buffwidth, m_width);
    freeimage(&pcxfile);
    return 0;
}

// Original: CSpriteFrame::importCroppedPCXFile; cspriteframe.cpp:368, dc 0x74af4.
int CSpriteFrame::importCroppedPCXFile(const char* filename)
{
    PcxData pdat;
    imgdes pcxfile;
    int error = pcxinfo(filename, &pdat);
    if (error)
        return 1;

    m_width = m_croppedWidth = pdat.m_width;
    m_height = m_croppedHeight = pdat.m_length;
    m_pitch = pdat.m_width;
    m_dataSize = m_imageSize = m_width * m_height;
    allocimage(&pcxfile, pdat.m_width, pdat.m_length,
               pdat.m_bpPixel * pdat.m_nplanes);
    loadpcx(filename, &pcxfile);
    flipimage(&pcxfile, &pcxfile);

    int x;
    int y;
    int leftoff = -1;
    int rightoff = -1;
    int topoff = -1;
    int bottomoff = -1;
    for (x = 0; x < m_width; ++x) {
        for (y = 0; y < m_height; ++y) {
            if (pcxfile.m_ibuff[y * pcxfile.m_buffwidth + x]) {
                leftoff = x;
                break;
            }
        }
        if (leftoff >= 0)
            break;
    }
    for (x = 0; x < m_width; ++x) {
        for (y = 0; y < m_height; ++y) {
            if (pcxfile.m_ibuff[y * pcxfile.m_buffwidth + m_width - x - 1]) {
                rightoff = x;
                break;
            }
        }
        if (rightoff >= 0)
            break;
    }
    for (y = 0; y < m_height; ++y) {
        for (x = 0; x < m_width; ++x) {
            if (pcxfile.m_ibuff[y * pcxfile.m_buffwidth + x]) {
                topoff = y;
                break;
            }
        }
        if (topoff >= 0)
            break;
    }
    for (y = 0; y < m_height; ++y) {
        for (x = 0; x < m_width; ++x) {
            if (pcxfile.m_ibuff[(m_height - y - 1) * pcxfile.m_buffwidth + x]) {
                bottomoff = y;
                break;
            }
        }
        if (bottomoff >= 0)
            break;
    }
    if (leftoff >= 0) {
        m_croppedX += leftoff;
        m_croppedWidth -= leftoff;
    }
    if (rightoff >= 0)
        m_croppedWidth -= rightoff;
    if (topoff >= 0) {
        m_croppedY += topoff;
        m_croppedHeight -= topoff;
    }
    if (bottomoff >= 0)
        m_croppedHeight -= bottomoff;
    m_dataSize = m_croppedWidth * m_croppedHeight;
    m_map = new unsigned char[m_dataSize];
    if (!m_map)
        return 2;

    unsigned char* dest = m_map;
    unsigned char* source = pcxfile.m_ibuff + topoff * pcxfile.m_buffwidth + leftoff;
    for (y = 0; y < m_croppedHeight; ++y) {
        memcpy(dest, source, m_croppedWidth);
        dest += m_croppedWidth;
        source += pcxfile.m_buffwidth;
    }
    // DC retains the original PCX width as Pitch even after packing the crop.
    m_pitch = pdat.m_width;
    freeimage(&pcxfile);
    return 0;
}

// Original: CSpriteFrame::GetPixel; cspriteframe.cpp:542, dc 0x74d60.
// Row directories use the same dword/word formats as the retained renderers.
unsigned char CSpriteFrame::getPixel(int x, int y) const
{
    x -= m_croppedX;
    if (x < 0 || x >= m_croppedWidth)
        return 0;
    y -= m_croppedY;
    if (y < 0 || y >= m_croppedHeight)
        return 0;

    unsigned char pixel;
    switch (m_encodingMethod) {
    case eEncodeRaw:
        pixel = m_map[y * m_pitch + x];
        break;
    case eEncodeGeneralRLE: {
        const unsigned int* lineOffsets =
            static_cast<const unsigned int*>(static_cast<const void*>(m_map));
        const unsigned char* source = m_map + lineOffsets[y];
        unsigned int position = 0;
        while (1) {
            unsigned char code = *source++;
            unsigned int run = *source++ + 1;
            if (position + run > static_cast<unsigned int>(x)) {
                if (code == g_generalRleOpaqueRunCode)
                    pixel = source[x - position];
                else
                    pixel = code;
                break;
            }
            position += run;
            if (code == g_generalRleOpaqueRunCode)
                source += run;
        }
        break;
    }
    case eEncodeTilesetRLE: {
        const unsigned short* lineOffsets =
            static_cast<const unsigned short*>(static_cast<const void*>(m_map));
        const unsigned char* source = m_map + lineOffsets[y];
        unsigned int position = 0;
        while (1) {
            unsigned char control = *source++;
            unsigned char code = control >> 5;
            unsigned int run = (control & 31) + 1;
            if (position + run > static_cast<unsigned int>(x)) {
                if (code == ePackedRleLiteral)
                    pixel = source[x - position];
                else
                    pixel = code;
                break;
            }
            position += run;
            if (code == ePackedRleLiteral)
                source += run;
        }
        break;
    }
    case eEncodeAdvObjRLE: {
        unsigned int cellsPerRow = static_cast<unsigned int>(m_croppedWidth) >> 5;
        const unsigned short* cellOffsets =
            static_cast<const unsigned short*>(static_cast<const void*>(m_map));
        const unsigned char* source = m_map + cellOffsets[y * cellsPerRow + (x >> 5)];
        unsigned int position = x - (x & 31);
        while (1) {
            unsigned char control = *source++;
            unsigned char code = control >> 5;
            unsigned int run = (control & 31) + 1;
            if (position + run > static_cast<unsigned int>(x)) {
                pixel = code;
                if (code == ePackedRleLiteral)
                    pixel = source[x - position];
                break;
            }
            position += run;
            if (code == ePackedRleLiteral)
                source += run;
        }
        break;
    }
    }
    return pixel;
}

// Original: CSpriteFrame::Crop; cspriteframe.cpp:645, dc 0x74ecc.
// This editing operation accepts the raw map and retains the full-image stride.
int CSpriteFrame::crop()
{
    int x;
    int y;
    int leftoff = -1;
    int rightoff = -1;
    int topoff = -1;
    int bottomoff = -1;
    for (x = 0; x < m_width; ++x) {
        for (y = 0; y < m_height; ++y) {
            if (m_map[y * m_width + x]) {
                leftoff = x;
                break;
            }
        }
        if (leftoff >= 0)
            break;
    }
    for (x = 0; x < m_width; ++x) {
        for (y = 0; y < m_height; ++y) {
            if (m_map[y * m_width + m_width - x - 1]) {
                rightoff = x;
                break;
            }
        }
        if (rightoff >= 0)
            break;
    }
    for (y = 0; y < m_height; ++y) {
        for (x = 0; x < m_width; ++x) {
            if (m_map[y * m_width + x]) {
                topoff = y;
                break;
            }
        }
        if (topoff >= 0)
            break;
    }
    for (y = 0; y < m_height; ++y) {
        for (x = 0; x < m_width; ++x) {
            if (m_map[(m_height - y - 1) * m_width + x]) {
                bottomoff = y;
                break;
            }
        }
        if (bottomoff >= 0)
            break;
    }
    if (leftoff >= 0) {
        m_croppedX += leftoff;
        m_croppedWidth -= leftoff;
    }
    if (rightoff >= 0)
        m_croppedWidth -= rightoff;
    if (topoff >= 0) {
        m_croppedY += topoff;
        m_croppedHeight -= topoff;
    }
    if (bottomoff >= 0)
        m_croppedHeight -= bottomoff;
    m_dataSize = m_croppedWidth * m_croppedHeight;
    unsigned char* newMap = new unsigned char[m_dataSize];
    if (!newMap)
        return 2;
    unsigned char* dest = newMap;
    unsigned char* source = m_map + topoff * m_width + leftoff;
    for (y = 0; y < m_croppedHeight; ++y) {
        memcpy(dest, source, m_croppedWidth);
        dest += m_croppedWidth;
        source += m_width;
    }
    m_pitch = m_width;
    delete[] m_map;
    m_map = newMap;
    return 0;
}

// Original: CSpriteFrame::Encode; cspriteframe.cpp:768, dc 0x75094.
void CSpriteFrame::encode(TEncodingMethod method)
{
    switch (method) {
    case eEncodeGeneralRLE: encodeGeneral(); break;
    case eEncodeTilesetRLE: encodeTileset(); break;
    case eEncodeAdvObjRLE: encodeAdvObj(); break;
    }
}

// Original: CSpriteFrame::EncodeGeneral; cspriteframe.cpp:791, dc 0x750d8.
void CSpriteFrame::encodeGeneral()
{
    static const unsigned char opaqueRunCode = g_generalRleOpaqueRunCode;
    // DC's cspriteframe.cpp:47 initializer is max<unsigned char>() + 1.
    static const unsigned int maxRunLength = 256;
    unsigned int newDataSize = m_croppedHeight * sizeof(unsigned int);
    unsigned int linesToGo = m_croppedHeight;
    unsigned char* source = m_map;
    do {
        unsigned char code = source[0] < 10 ? source[0] : opaqueRunCode;
        newDataSize += 2;
        unsigned int run = 0;
        int x = 1;
        bool opaque = code == opaqueRunCode;
        while (1) {
            ++run;
            if (opaque)
                ++newDataSize;
            if (x >= m_croppedWidth)
                break;
            unsigned char nextCode = source[x] < 10 ? source[x] : opaqueRunCode;
            if (nextCode != code || run == maxRunLength) {
                newDataSize += 2;
                code = nextCode;
                run = 0;
                opaque = code == opaqueRunCode;
            }
            ++x;
        }
        source += m_croppedWidth;
    } while (--linesToGo);

    unsigned char* newMap = new unsigned char[newDataSize];
    unsigned int* lineOffset = static_cast<unsigned int*>(static_cast<void*>(newMap));
    linesToGo = m_croppedHeight;
    unsigned int offset = m_croppedHeight * sizeof(unsigned int);
    source = m_map;
    do {
        *lineOffset++ = offset;
        unsigned char code = source[0] < 10 ? source[0] : opaqueRunCode;
        newMap[offset++] = code;
        unsigned char* runLength = newMap + offset++;
        unsigned int run = 0;
        int x = 1;
        while (1) {
            ++run;
            if (code == opaqueRunCode)
                newMap[offset++] = source[x - 1];
            if (x >= m_croppedWidth)
                break;
            unsigned char nextCode = source[x] < 10 ? source[x] : opaqueRunCode;
            if (nextCode != code || run == maxRunLength) {
                *runLength = static_cast<unsigned char>(run - 1);
                code = nextCode;
                newMap[offset++] = code;
                runLength = newMap + offset++;
                run = 0;
            }
            ++x;
        }
        *runLength = static_cast<unsigned char>(run - 1);
        source += m_croppedWidth;
    } while (--linesToGo);
    delete[] m_map;
    m_map = newMap;
    m_dataSize = newDataSize;
    m_encodingMethod = eEncodeGeneralRLE;
}

// Original: CSpriteFrame::EncodeTileset; cspriteframe.cpp:894, dc 0x7528c.
void CSpriteFrame::encodeTileset()
{
    bool hasControlPixels = false;
    for (int i = 0; i < m_dataSize; ++i) {
        if (m_map[i] < 5) {
            hasControlPixels = true;
            break;
        }
    }
    if (hasControlPixels) {
        unsigned int linesToGo = m_croppedHeight;
        unsigned int newDataSize = m_croppedHeight * sizeof(unsigned short);
        unsigned char* source = m_map;
        do {
            unsigned char code = source[0] < 5 ? source[0] : ePackedRleLiteral;
            ++newDataSize;
            unsigned int run = 0;
            int x = 1;
            bool opaque = code == ePackedRleLiteral;
            while (1) {
                ++run;
                if (opaque)
                    ++newDataSize;
                if (x >= m_croppedWidth)
                    break;
                unsigned char nextCode = source[x] < 5 ? source[x] : ePackedRleLiteral;
                if (nextCode != code || run == ePackedRleMaxRunLength) {
                    ++newDataSize;
                    code = nextCode;
                    run = 0;
                    opaque = code == ePackedRleLiteral;
                }
                ++x;
            }
            source += m_croppedWidth;
        } while (--linesToGo);

        unsigned char* newMap = new unsigned char[newDataSize];
        unsigned short* lineOffset = static_cast<unsigned short*>(static_cast<void*>(newMap));
        linesToGo = m_croppedHeight;
        unsigned short offset = static_cast<unsigned short>(m_croppedHeight * sizeof(unsigned short));
        source = m_map;
        do {
            *lineOffset++ = offset;
            unsigned char code = source[0] < 5 ? source[0] : ePackedRleLiteral;
            bool opaque = code == ePackedRleLiteral;
            unsigned char* control = newMap + offset;
            *control = static_cast<unsigned char>(code << 5);
            ++offset;
            unsigned int run = 0;
            int x = 1;
            while (1) {
                ++run;
                if (opaque)
                    newMap[offset++] = source[x - 1];
                if (x >= m_croppedWidth)
                    break;
                unsigned char nextCode = source[x] < 5 ? source[x] : ePackedRleLiteral;
                if (nextCode != code || run == ePackedRleMaxRunLength) {
                    *control |= static_cast<unsigned char>(run - 1);
                    code = nextCode;
                    control = newMap + offset;
                    *control = static_cast<unsigned char>(code << 5);
                    ++offset;
                    run = 0;
                    opaque = code == ePackedRleLiteral;
                }
                ++x;
            }
            *control |= static_cast<unsigned char>(run - 1);
            source += m_croppedWidth;
        } while (--linesToGo);
        delete[] m_map;
        m_map = newMap;
        m_dataSize = newDataSize;
        m_encodingMethod = eEncodeTilesetRLE;
    }
}

// Original: CSpriteFrame::EncodeAdvObj; cspriteframe.cpp:1012, dc 0x75440.
void CSpriteFrame::encodeAdvObj()
{
    int oldWidth = m_croppedWidth;
    int right = m_croppedX + oldWidth;
    int newCroppedX = m_croppedX - (m_croppedX & 31);
    int newRight = right + 31 - ((right + 31) & 31);
    unsigned int newCroppedWidth = newRight - newCroppedX;
    unsigned int addLeft = m_croppedX - newCroppedX;
    unsigned int addRight = newRight - right;
    unsigned int cellsPerLine = newCroppedWidth >> 5;
    unsigned int newDataSize = 2 * cellsPerLine * m_croppedHeight;
    unsigned int linesToGo = m_croppedHeight;
    unsigned char* source = m_map;
    do {
        unsigned char* sourceCell = source;
        int pixelsDone = 0;
        unsigned int cellsToGo = cellsPerLine;
        unsigned char code;
        if (addLeft) {
            code = source[0] < 6 ? source[0] : ePackedRleLiteral;
            if (code != 0)
                ++newDataSize;
            ++newDataSize;
            unsigned int x = 1;
            while (1) {
                ++pixelsDone;
                if (code == ePackedRleLiteral)
                    ++newDataSize;
                if (x >= 32 - addLeft || pixelsDone >= oldWidth)
                    break;
                unsigned char nextCode = source[x] < 6 ? source[x] : ePackedRleLiteral;
                if (nextCode != code) {
                    ++newDataSize;
                    code = nextCode;
                }
                ++x;
            }
            sourceCell += x;
            --cellsToGo;
        }
        while (cellsToGo) {
            code = sourceCell[0] < 6 ? sourceCell[0] : ePackedRleLiteral;
            ++newDataSize;
            unsigned int x = 1;
            while (1) {
                ++pixelsDone;
                if (code == ePackedRleLiteral)
                    ++newDataSize;
                if (x >= 32 || pixelsDone >= oldWidth)
                    break;
                unsigned char nextCode = sourceCell[x] < 6 ? sourceCell[x] : ePackedRleLiteral;
                if (nextCode != code) {
                    ++newDataSize;
                    code = nextCode;
                }
                ++x;
            }
            sourceCell += x;
            --cellsToGo;
        }
        if (addRight && code != 0)
            ++newDataSize;
        source += oldWidth;
    } while (--linesToGo);

    unsigned char* newMap = new unsigned char[newDataSize];
    unsigned short* cellOffset = static_cast<unsigned short*>(static_cast<void*>(newMap));
    unsigned short offset = static_cast<unsigned short>(2 * cellsPerLine * m_croppedHeight);
    source = m_map;
    linesToGo = m_croppedHeight;
    do {
        unsigned char* sourceCell = source;
        int pixelsDone = 0;
        unsigned int cellsToGo = cellsPerLine;
        unsigned char code;
        unsigned char* control;
        unsigned int run;
        if (addLeft) {
            *cellOffset++ = offset;
            code = source[0] < 6 ? source[0] : ePackedRleLiteral;
            run = 0;
            if (code != 0) {
                newMap[offset] = 0;
                newMap[offset++] |= static_cast<unsigned char>(addLeft - 1);
            } else {
                run += addLeft;
            }
            control = newMap + offset;
            *control = static_cast<unsigned char>(code << 5);
            ++offset;
            unsigned int x = 1;
            while (1) {
                ++pixelsDone;
                ++run;
                if (code == ePackedRleLiteral)
                    newMap[offset++] = source[x - 1];
                if (x >= 32 - addLeft || pixelsDone >= oldWidth)
                    break;
                unsigned char nextCode = source[x] < 6 ? source[x] : ePackedRleLiteral;
                if (nextCode != code) {
                    *control |= static_cast<unsigned char>(run - 1);
                    code = nextCode;
                    control = newMap + offset;
                    *control = static_cast<unsigned char>(code << 5);
                    ++offset;
                    run = 0;
                }
                ++x;
            }
            sourceCell += x;
            if (--cellsToGo)
                *control |= static_cast<unsigned char>(run - 1);
        }
        while (cellsToGo) {
            *cellOffset++ = offset;
            code = sourceCell[0] < 6 ? sourceCell[0] : ePackedRleLiteral;
            control = newMap + offset;
            *control = static_cast<unsigned char>(code << 5);
            ++offset;
            run = 0;
            unsigned int x = 1;
            while (1) {
                ++pixelsDone;
                ++run;
                if (code == ePackedRleLiteral)
                    newMap[offset++] = sourceCell[x - 1];
                if (x >= 32 || pixelsDone >= oldWidth)
                    break;
                unsigned char nextCode = sourceCell[x] < 6 ? sourceCell[x] : ePackedRleLiteral;
                if (nextCode != code) {
                    *control |= static_cast<unsigned char>(run - 1);
                    code = nextCode;
                    control = newMap + offset;
                    *control = static_cast<unsigned char>(code << 5);
                    ++offset;
                    run = 0;
                }
                ++x;
            }
            sourceCell += x;
            if (!--cellsToGo)
                break;
            *control |= static_cast<unsigned char>(run - 1);
        }
        if (addRight) {
            if (code != 0) {
                *control |= static_cast<unsigned char>(run - 1);
                control = newMap + offset;
                *control = 0;
                ++offset;
                run = addRight;
            } else {
                run += addRight;
            }
        }
        *control |= static_cast<unsigned char>(run - 1);
        source += oldWidth;
    } while (--linesToGo);
    // Unlike the other two encoders, DC 1256 replaces the map without deleting it.
    m_map = newMap;
    m_dataSize = newDataSize;
    m_encodingMethod = eEncodeAdvObjRLE;
    m_croppedX = newCroppedX;
    m_croppedWidth = newCroppedWidth;
}

VA(0x0047c560, 0x07)  // vtable slot 2: fixed object extent + owned bytes
unsigned int CSpriteFrame::getSize() const
{
    return sizeof(*this) + m_dataSize;
}

inline void CSpriteFrame::clip(int& sx, int& sy, int& sw, int& sh,
                               int& dx, int& dy, int dw, int dh,
                               unsigned char hflip,
                               unsigned char vflip) const
{
    int deltaX;

    if (hflip)
        sx = m_width - sx - sw;
    if (vflip)
        sy = m_height - sy - sh;

    if (dx < 0) {
        if (!hflip)
            sx -= dx;
        sw += dx;
        dx = 0;
    }
    if (dy < 0) {
        if (!vflip)
            sy -= dy;
        sh += dy;
        dy = 0;
    }
    if (sw + dx > dw) {
        if (hflip)
            sx += sw + dx - dw;
        sw = dw - dx;
    }
    if (sh + dy > dh) {
        if (vflip)
            sy += sh + dy - dh;
        sh = dh - dy;
    }

    if (sx < m_croppedX) {
        deltaX = m_croppedX - sx;
        if (!hflip)
            dx += deltaX;
        sw -= deltaX;
        sx = m_croppedX;
    }
    if (sy < m_croppedY) {
        deltaX = m_croppedY - sy;
        if (!vflip)
            dy += deltaX;
        sh -= deltaX;
        sy = m_croppedY;
    }
    deltaX = m_croppedWidth + m_croppedX;
    if (sw + sx > deltaX) {
        if (hflip)
            dx += sw + sx - deltaX;
        sw = deltaX - sx;
    }
    deltaX = m_croppedHeight + m_croppedY;
    if (sh + sy > deltaX) {
        if (vflip)
            dy += sh + sy - deltaX;
        sh = deltaX - sy;
    }

    sx -= m_croppedX;
    sy -= m_croppedY;
}

// E:\gamedcs\cspriteframe.cpp:1357
// General-RLE lines begin with a dword offset table.  Each line then contains
// (code,count-minus-one) runs; code 255 denotes a literal byte string, while
// other codes denote a repeated palette index.  Raw/tile and adventure-object
// encodings dispatch to their specialized renderers before this path.
// Dreamcast names the table pointer `aLineOffset` and records each row loop as
// one enclosing lifetime. Advancing lineDst directly by dpitch preserves that
// source model and makes VC6 place its initialization before the loop guard,
// matching both retail halves exactly; rebuilding it from a base plus an
// integer row offset deferred that store and measured 98.62%.

VA(0x0047c570, 0x465)  // unique PC/DC renderer identity; retail byte verdict
void CSpriteFrame::draw(int sx, int sy, int sw, int sh,
                        unsigned short* dst, int dx, int dy, int dw, int dh,
                        int dpitch, TPalette16& pal, unsigned char hflip,
                        unsigned char tblit) const
{
    if (m_encodingMethod == eEncodeTilesetRLE || m_encodingMethod == eEncodeRaw) {
        drawTile(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                 pal, hflip, 0);
        return;
    }
    else if (m_encodingMethod == eEncodeAdvObjRLE) {
        drawAdvObjImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                       pal, hflip, 0);
        return;
    }

    const unsigned int* lineOffset;
    static const unsigned char opaqueRunCode = g_generalRleOpaqueRunCode;
    clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, 0);

    if (sw > 0 && sh > 0) {
        lineOffset =
            static_cast<const unsigned int*>(static_cast<const void*>(m_map));
        if (!hflip) {
            unsigned short* lineDst =
                static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + dy * dpitch + dx * 2));
            for (int y = sy; y < sy + sh; ++y) {
                unsigned short* out = lineDst;
                unsigned int skipped = 0;
                const unsigned char* src = m_map + lineOffset[y];
                unsigned char code = *src++;
                unsigned int run = *src++ + 1;

                while (skipped + run <= static_cast<unsigned int>(sx)) {
                    skipped += run;
                    if (code == opaqueRunCode)
                        src += run;
                    code = *src++;
                    run = *src++ + 1;
                }

                run += skipped - sx;
                if (code == opaqueRunCode)
                    src += sx - skipped;

                unsigned int remaining = sw;
                do {
                    if (run > remaining)
                        run = remaining;
                    if (code == opaqueRunCode) {
                        unsigned int count = run;
                        do {
                            *out++ = pal.m_data[*src++];
                        } while (--count);
                    } else if (tblit) {
                        out += run;
                    } else {
                        unsigned short color = pal.m_data[code];
                        unsigned int count = run;
                        do {
                            *out++ = color;
                        } while (--count);
                    }
                    remaining -= run;
                    if (!remaining)
                        break;
                    code = *src++;
                    run = *src++ + 1;
                } while (remaining);

                lineDst = static_cast<unsigned short*>(static_cast<void*>(
                    static_cast<unsigned char*>(static_cast<void*>(lineDst))
                    + dpitch));
            }
        } else {
            unsigned short* lineDst =
                static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + dy * dpitch + (dx + sw) * 2));
            for (int y = sy; y < sy + sh; ++y) {
                unsigned short* out = lineDst;
                unsigned int skipped = 0;
                const unsigned char* src = m_map + lineOffset[y];
                unsigned char code = *src++;
                unsigned int run = *src++ + 1;

                while (skipped + run <= static_cast<unsigned int>(sx)) {
                    skipped += run;
                    if (code == opaqueRunCode)
                        src += run;
                    code = *src++;
                    run = *src++ + 1;
                }

                run += skipped - sx;
                if (code == opaqueRunCode)
                    src += sx - skipped;

                unsigned int remaining = sw;
                do {
                    if (run > remaining)
                        run = remaining;
                    if (code == opaqueRunCode) {
                        unsigned int count = run;
                        do {
                            *--out = pal.m_data[*src++];
                        } while (--count);
                    } else if (tblit) {
                        out -= run;
                    } else {
                        unsigned short color = pal.m_data[code];
                        unsigned int count = run;
                        do {
                            *--out = color;
                        } while (--count);
                    }
                    remaining -= run;
                    if (!remaining)
                        break;
                    code = *src++;
                    run = *src++ + 1;
                } while (remaining);

                lineDst = static_cast<unsigned short*>(static_cast<void*>(
                    static_cast<unsigned char*>(static_cast<void*>(lineDst))
                    + dpitch));
            }
        }
    }
}

// E:\gamedcs\cspriteframe.cpp:1878
// General-RLE creature drawing extends Draw's literal runs with optional
// half-alpha blending.  Encoded control runs darken the destination by one
// half or one quarter-plus-half, or install the caller's outline color.

// Residual (95.90%): the dispatch, clipping, decoder, and every forward shade
// block now agree instruction-for-instruction.
// MEASURED AND REJECTED 2026-09-05: dropping the `unsigned int color = out[-1]`
// widening from the two reverse half-shade arms - the one line that took
// DrawTileShadow 97.90 -> 99.93 and DrawAdvObjShadowImpl 98.58 -> 99.94 - costs
// 0.57 HERE (95.9000 -> 95.3300, three blocks size-only). The renderers really
// do spell the same blend two different ways; do not carry the lever across. Retail deliberately widens one
// ushort blend-mask access to a dword AND whose low half alone is stored; the
// explicit dword view below recovers that C1 value-range decision and aligns
// both CFGs at 128 blocks. The cast-free `TBlendMask` view records Complete's
// word write/dword read while retaining CodeView's static-member name and
// ownership; CodeView also proves the function-scope `TOffset`/`TDstPixel`
// identities (`unsigned int`/`unsigned short`) and that `aLineOffset` precedes
// the const `kOpaqueRunCode`. Using `TOffset` for the row table is byte-flat.
// The remaining delta is the
// Clip/setup register permutation and reverse half-shade/default-tail
// scheduling. After this new allocator lever, moving the line table into the
// positive guard scores 95.83%, block-scoping the row destination 95.54%, and
// all one-sided/symmetric reverse ushort or direct-expression variants score
// lower. `why-reg --model --il-order` still finds identical first definitions
// (EDI=sw, ESI=sx, EBX=sh), bounding the residual past the minimum source-order
// slice.
// Row-boundary residual (94.7302%): visited pointers use integral
// byte displacements; only the integer advances after the final row. The
// earlier guarded/break form scored 91.9365%. DC decoder/helper scopes
// and pixel arithmetic remain intact, including reverse/raw source walks.
VA(0x0047c9e0, 0x6BC)  // unique PC/DC renderer identity; retail byte verdict
void CSpriteFrame::drawCreatureImpl(int sx, int sy, int sw, int sh,
                                    unsigned short* dst, int dx, int dy,
                                    int dw, int dh, int dpitch,
                                    TPalette16& pal, unsigned char hflip,
                                    unsigned short outcolor,
                                    unsigned char alpha) const
{
    typedef unsigned int TOffset;
    typedef unsigned short TDstPixel;

    if (!alpha) {
        if (m_encodingMethod == eEncodeTilesetRLE ||
            m_encodingMethod == eEncodeRaw) {
            drawTile(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                     pal, hflip, 0);
            return;
        }
        if (m_encodingMethod == eEncodeAdvObjRLE) {
            drawAdvObjImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                           pal, hflip, outcolor);
            return;
        }
    }

    const TOffset* lineOffset;
    static const unsigned char opaqueRunCode = g_generalRleOpaqueRunCode;
    clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, 0);

    lineOffset =
        static_cast<const TOffset*>(static_cast<const void*>(m_map));
    if (sw > 0 && sh > 0) {
        if (!hflip) {
            dst = static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + dy * dpitch + dx * 2));

                unsigned char* rowBase = static_cast<unsigned char*>(static_cast<void*>(dst));
                int rowOffset = 0;
            for (int y = sy; y < sy + sh; ++y) {
                    dst = static_cast<unsigned short*>(static_cast<void*>(rowBase + rowOffset));
                unsigned short* out = dst;
                unsigned int skipped = 0;
                const unsigned char* src = m_map + lineOffset[y];
                unsigned char code = *src++;
                unsigned int run = *src++ + 1;

                while (skipped + run <= static_cast<unsigned int>(sx)) {
                    skipped += run;
                    if (code == opaqueRunCode)
                        src += run;
                    code = *src++;
                    run = *src++ + 1;
                }

                run += skipped - sx;
                if (code == opaqueRunCode)
                    src += sx - skipped;

                unsigned int remaining = sw;
                do {
                    if (run > remaining)
                        run = remaining;
                    if (code == opaqueRunCode) {
                        unsigned int count = run;
                        if (!alpha) {
                            do {
                                *out++ = pal.m_data[*src++];
                            } while (--count);
                        } else {
                            do {
                                *out = (s_div2mask.m_dword
                                        & (pal.m_data[*src++] >> 1))
                                     + (s_div2mask.m_word & (*out >> 1));
                                ++out;
                            } while (--count);
                        }
                    } else if (!outcolor) {
                        switch (code) {
                        case eRleControlShadow75:
                        case eRleControlOutline7: {
                            unsigned int count = run;
                            do {
                                *out = ((*out >> 2) & s_div4mask)
                                     + ((*out >> 1) & s_div2mask.m_word);
                                ++out;
                            } while (--count);
                            break;
                        }
                        case eRleControlShadow50:
                        case eRleControlOutline6: {
                            unsigned int count = run;
                            do {
                                unsigned int color = *out;
                                *out = (color >> 1) & s_div2mask.m_word;
                                ++out;
                            } while (--count);
                            break;
                        }
                        default:
                            out += run;
                            break;
                        }
                    } else {
                        switch (code) {
                        case eRleControlShadow75: {
                            unsigned int count = run;
                            do {
                                *out = ((*out >> 2) & s_div4mask)
                                     + ((*out >> 1) & s_div2mask.m_word);
                                ++out;
                            } while (--count);
                            break;
                        }
                        case eRleControlShadow50: {
                            unsigned int count = run;
                            do {
                                unsigned int color = *out;
                                *out = (color >> 1) & s_div2mask.m_word;
                                ++out;
                            } while (--count);
                            break;
                        }
                        case eRleControlOutline5:
                        case eRleControlOutline6:
                        case eRleControlOutline7: {
                            unsigned int count = run;
                            do {
                                *out++ = outcolor;
                            } while (--count);
                            break;
                        }
                        default:
                            out += run;
                            break;
                        }
                    }
                    remaining -= run;
                    if (!remaining)
                        break;
                    code = *src++;
                    run = *src++ + 1;
                } while (remaining);

                    rowOffset += dpitch;
                }
        } else {
            dst = static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + dy * dpitch + (dx + sw) * 2));

                unsigned char* rowBase = static_cast<unsigned char*>(static_cast<void*>(dst));
                int rowOffset = 0;
            for (int y = sy; y < sy + sh; ++y) {
                    dst = static_cast<unsigned short*>(static_cast<void*>(rowBase + rowOffset));
                unsigned short* out = dst;
                unsigned int skipped = 0;
                const unsigned char* src = m_map + lineOffset[y];
                unsigned char code = *src++;
                unsigned int run = *src++ + 1;

                while (skipped + run <= static_cast<unsigned int>(sx)) {
                    skipped += run;
                    if (code == opaqueRunCode)
                        src += run;
                    code = *src++;
                    run = *src++ + 1;
                }

                run += skipped - sx;
                if (code == opaqueRunCode)
                    src += sx - skipped;

                unsigned int remaining = sw;
                do {
                    if (run > remaining)
                        run = remaining;
                    if (code == opaqueRunCode) {
                        unsigned int count = run;
                        if (!alpha) {
                            do {
                                *--out = pal.m_data[*src++];
                            } while (--count);
                        } else {
                            do {
                                --out;
                                *out = (s_div2mask.m_dword
                                        & (pal.m_data[*src++] >> 1))
                                     + (s_div2mask.m_dword & (*out >> 1));
                            } while (--count);
                        }
                    } else if (!outcolor) {
                        switch (code) {
                        case eRleControlShadow75:
                        case eRleControlOutline7: {
                            unsigned int count = run;
                            do {
                                --out;
                                *out = ((*out >> 2) & s_div4mask)
                                     + ((*out >> 1) & s_div2mask.m_dword);
                            } while (--count);
                            break;
                        }
                        case eRleControlShadow50:
                        case eRleControlOutline6: {
                            unsigned int count = run;
                            do {
                                unsigned int color = out[-1];
                                --out;
                                *out = (color >> 1) & s_div2mask.m_dword;
                            } while (--count);
                            break;
                        }
                        default:
                            out -= run;
                            break;
                        }
                    } else {
                        switch (code) {
                        case eRleControlShadow75: {
                            unsigned int count = run;
                            do {
                                --out;
                                *out = ((*out >> 2) & s_div4mask)
                                     + ((*out >> 1) & s_div2mask.m_dword);
                            } while (--count);
                            break;
                        }
                        case eRleControlShadow50: {
                            unsigned int count = run;
                            do {
                                unsigned int color = out[-1];
                                --out;
                                *out = (color >> 1) & s_div2mask.m_dword;
                            } while (--count);
                            break;
                        }
                        case eRleControlOutline5:
                        case eRleControlOutline6:
                        case eRleControlOutline7: {
                            unsigned int count = run;
                            do {
                                *--out = outcolor;
                            } while (--count);
                            break;
                        }
                        default:
                            out -= run;
                            break;
                        }
                    }
                    remaining -= run;
                    if (!remaining)
                        break;
                    code = *src++;
                    run = *src++ + 1;
                } while (remaining);

                    rowOffset += dpitch;
                }
        }
    }
}

// E:\gamedcs\cspriteframe.cpp:2234.  Adventure-object rows are divided into
// 32-pixel cells.  Each cell has a word offset into map and contains packed
// (three-bit control, five-bit count-minus-one) runs.  Control seven carries
// literal palette indexes; control five optionally draws the caller's flag
// colour, and the remaining controls are transparent in this renderer.
// Dreamcast records only palette, cellsPerLine and aCellOffset and gives each
// row loop one enclosing lifetime. Advancing lineDst directly by dpitch keeps
// that source model and matches both retail direction arms exactly; rebuilding
// it from rowBase plus an integer rowOffset measured 96.6247%.

VA(0x0047d0a0, 0x44B) // retail packed-cell decoder + DC source identity
void CSpriteFrame::drawAdvObjImpl(int sx, int sy, int sw, int sh,
                                  unsigned short* dst, int dx, int dy, int dw,
                                  int dh, int dpitch, TPalette16& pal,
                                  unsigned char hflip,
                                  unsigned short flagcolor) const
{
    const unsigned short* palette;
    unsigned int cellsPerLine;
    const unsigned short* cellOffset;

    if (m_encodingMethod == eEncodeGeneralRLE) {
        // Retail passes sw in the first source-coordinate slot at 0x47d0e6.
        drawCreature(sw, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip,
                     flagcolor);
        return;
    }
    if (m_encodingMethod == eEncodeTilesetRLE || m_encodingMethod == eEncodeRaw) {
        drawTile(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, 0);
        return;
    }

    clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, 0);
    if (sw > 0) {
        if (sh > 0) {

            cellsPerLine = static_cast<unsigned int>(m_croppedWidth) >> 5;
            cellOffset = static_cast<const unsigned short*>(static_cast<const void*>(m_map));
            palette = pal.m_data;

            if (!hflip) {
                unsigned short* lineDst =
                    static_cast<unsigned short*>(static_cast<void*>(
                    static_cast<unsigned char*>(static_cast<void*>(dst)) +
                    dy * dpitch + dx * 2));

                for (int y = sy; y < sy + sh; ++y) {
                    unsigned short* out = lineDst;
                    unsigned int skipped = static_cast<unsigned int>(sx) & ~31U;
                    const unsigned char* src =
                        m_map + cellOffset[y * cellsPerLine +
                                          (static_cast<unsigned int>(sx) >> 5)];
                    unsigned char packet = *src;
                    unsigned char code = packet >> 5;
                    unsigned int run = (packet & 31) + 1;
                    ++src;

                    while (skipped + run <= static_cast<unsigned int>(sx)) {
                        skipped += run;
                        if (code == eRleControlOutline7)
                            src += run;
                        packet = *src;
                        code = packet >> 5;
                        run = (packet & 31) + 1;
                        ++src;
                    }

                    run += skipped - sx;
                    if (code == eRleControlOutline7)
                        src += sx - skipped;

                    unsigned int remaining = sw;
                    do {
                        if (run > remaining)
                            run = remaining;
                        if (code == eRleControlOutline7) {
                            unsigned int count = run;
                            do {
                                *out++ = palette[*src++];
                            } while (--count);
                        } else if (code == eRleControlOutline5 && flagcolor) {
                            unsigned int count = run;
                            do {
                                *out++ = flagcolor;
                            } while (--count);
                        } else {
                            out += run;
                        }
                        remaining -= run;
                        if (!remaining)
                            break;
                        packet = *src;
                        code = packet >> 5;
                        run = (packet & 31) + 1;
                        ++src;
                    } while (remaining);

                    lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                            static_cast<unsigned char*>(
                                static_cast<void*>(lineDst)) +
                            dpitch));
                }
            } else {
                unsigned short* lineDst =
                    static_cast<unsigned short*>(static_cast<void*>(
                    static_cast<unsigned char*>(static_cast<void*>(dst)) +
                    dy * dpitch + (dx + sw) * 2));

                for (int y = sy; y < sy + sh; ++y) {
                    unsigned short* out = lineDst;
                    unsigned int skipped = static_cast<unsigned int>(sx) & ~31U;
                    const unsigned char* src =
                        m_map + cellOffset[y * cellsPerLine +
                                          (static_cast<unsigned int>(sx) >> 5)];
                    unsigned char packet = *src;
                    unsigned char code = packet >> 5;
                    unsigned int run = (packet & 31) + 1;
                    ++src;

                    while (skipped + run <= static_cast<unsigned int>(sx)) {
                        skipped += run;
                        if (code == eRleControlOutline7)
                            src += run;
                        packet = *src;
                        code = packet >> 5;
                        run = (packet & 31) + 1;
                        ++src;
                    }

                    run += skipped - sx;
                    if (code == eRleControlOutline7)
                        src += sx - skipped;

                    unsigned int remaining = sw;
                    do {
                        if (run > remaining)
                            run = remaining;
                        if (code == eRleControlOutline7) {
                            unsigned int count = run;
                            do {
                                *--out = palette[*src++];
                            } while (--count);
                        } else if (code == eRleControlOutline5 && flagcolor) {
                            unsigned int count = run;
                            do {
                                *--out = flagcolor;
                            } while (--count);
                        } else {
                            out -= run;
                        }
                        remaining -= run;
                        if (!remaining)
                            break;
                        packet = *src;
                        code = packet >> 5;
                        run = (packet & 31) + 1;
                        ++src;
                    } while (remaining);

                    lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                            static_cast<unsigned char*>(
                                static_cast<void*>(lineDst)) +
                            dpitch));
                }
            }
        }
    }
}

// E:\gamedcs\cspriteframe.cpp:3949
// The alpha adventure-object pass, and DrawAdvObjImpl's packed-cell decoder
// verbatim: same `sx & ~31` cell entry, same word cell table, same packet
// walk, same three run arms. Only the writes differ - the literal run and the
// flag-color run are each blended half-and-half against the destination
// instead of stored - and the encoding dispatch is gone, because this entry is
// only ever reached from DrawSpellEffect's own eEncodeAdvObjRLE arm, which has
// already made that decision.

// The identity comes from that caller, which is EXACT: 0x47efca reaches this
// body with the flag color 0 in slot 12 and hflip in slot 13, and
// DrawAdvObjWithFlagAlpha is the only member of this class with that argument
// order - DrawAdvObjImpl takes hflip in slot 12 and its flag color last.

// Residual (98.00%): 83 of 83 blocks, 42 of 42 branches and both returns
// agree, and the call multiset is empty on both sides. The two size-only
// blocks are one register pair transposed across the row-loop setup - retail
// forms `sy + sh` with `lea ebx,[edx+ecx]` and keeps the cell-entry mask in
// ECX where this compile uses the two the other way round - which is B-family
// homing on values that arrive as parameters, the same class DrawTileShadow
// left behind two rows up.
// DC 0x76384 records palette and aCellOffset as read-only pointers; lines
// 2462/2463 load the map table and bind pal+0x1c before either row loop.
// Row-boundary residual (94.5249%): visited pointers use integral
// byte displacements; only the integer advances after the final row. The
// earlier guarded/break form scored 90.3017%. DC decoder/helper scopes
// and pixel arithmetic remain intact, including reverse/raw source walks.
VA(0x0047d4f0, 0x43C)  // anchor-caller (DrawSpellEffect 0x47efca) + DC source identity
void CSpriteFrame::drawAdvObjWithFlagAlpha(int sx, int sy, int sw, int sh,
                                           unsigned short* dst, int dx, int dy,
                                           int dw, int dh, int dpitch,
                                           TPalette16& pal,
                                           unsigned short flagcolor,
                                           unsigned char hflip) const
{
    const unsigned short* palette;
    unsigned int cellsPerLine;
    const unsigned short* cellOffset;

    clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, 0);
    if (sw > 0) {
        if (sh > 0) {

            cellsPerLine = static_cast<unsigned int>(m_croppedWidth) >> 5;
            cellOffset = static_cast<const unsigned short*>(static_cast<const void*>(m_map));

            palette = pal.m_data;

            if (!hflip) {
                unsigned short* lineDst =
                    static_cast<unsigned short*>(static_cast<void*>(
                    static_cast<unsigned char*>(static_cast<void*>(dst)) +
                    dy * dpitch + dx * 2));

                unsigned char* rowBase = static_cast<unsigned char*>(static_cast<void*>(lineDst));
                int rowOffset = 0;
                for (int y = sy; y < sy + sh; ++y) {
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(rowBase + rowOffset));
                    unsigned short* out = lineDst;
                    unsigned int skipped = static_cast<unsigned int>(sx) & ~31U;
                    const unsigned char* src =
                        m_map + cellOffset[y * cellsPerLine +
                                          (static_cast<unsigned int>(sx) >> 5)];
                    unsigned char packet = *src;
                    unsigned char code = packet >> 5;
                    unsigned int run = (packet & 31) + 1;
                    ++src;

                    while (skipped + run <= static_cast<unsigned int>(sx)) {
                        skipped += run;
                        if (code == eRleControlOutline7)
                            src += run;
                        packet = *src;
                        code = packet >> 5;
                        run = (packet & 31) + 1;
                        ++src;
                    }

                    run += skipped - sx;
                    if (code == eRleControlOutline7)
                        src += sx - skipped;

                    unsigned int remaining = sw;
                    do {
                        if (run > remaining)
                            run = remaining;
                        if (code == eRleControlOutline7) {
                            unsigned int count = run;
                            do {
                                *out = (s_div2mask.m_dword
                                        & (palette[*src++] >> 1))
                                     + (s_div2mask.m_dword & (*out >> 1));
                                ++out;
                            } while (--count);
                        } else if (code == eRleControlOutline5 && flagcolor) {
                            unsigned int count = run;
                            do {
                                *out = (s_div2mask.m_dword & (*out >> 1))
                                     + (s_div2mask.m_dword & (flagcolor >> 1));
                                ++out;
                            } while (--count);
                        } else {
                            out += run;
                        }
                        remaining -= run;
                        if (!remaining)
                            break;
                        packet = *src;
                        code = packet >> 5;
                        run = (packet & 31) + 1;
                        ++src;
                    } while (remaining);

                    rowOffset += dpitch;
                }
            } else {
                unsigned short* lineDst =
                    static_cast<unsigned short*>(static_cast<void*>(
                    static_cast<unsigned char*>(static_cast<void*>(dst)) +
                    dy * dpitch + (dx + sw) * 2));

                unsigned char* rowBase = static_cast<unsigned char*>(static_cast<void*>(lineDst));
                int rowOffset = 0;
                for (int y = sy; y < sy + sh; ++y) {
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(rowBase + rowOffset));
                    unsigned short* out = lineDst;
                    unsigned int skipped = static_cast<unsigned int>(sx) & ~31U;
                    const unsigned char* src =
                        m_map + cellOffset[y * cellsPerLine +
                                          (static_cast<unsigned int>(sx) >> 5)];
                    unsigned char packet = *src;
                    unsigned char code = packet >> 5;
                    unsigned int run = (packet & 31) + 1;
                    ++src;

                    while (skipped + run <= static_cast<unsigned int>(sx)) {
                        skipped += run;
                        if (code == eRleControlOutline7)
                            src += run;
                        packet = *src;
                        code = packet >> 5;
                        run = (packet & 31) + 1;
                        ++src;
                    }

                    run += skipped - sx;
                    if (code == eRleControlOutline7)
                        src += sx - skipped;

                    unsigned int remaining = sw;
                    do {
                        if (run > remaining)
                            run = remaining;
                        if (code == eRleControlOutline7) {
                            unsigned int count = run;
                            do {
                                --out;
                                *out = (s_div2mask.m_dword
                                        & (palette[*src++] >> 1))
                                     + (s_div2mask.m_dword & (*out >> 1));
                            } while (--count);
                        } else if (code == eRleControlOutline5 && flagcolor) {
                            unsigned int count = run;
                            do {
                                --out;
                                *out = (s_div2mask.m_dword & (*out >> 1))
                                     + (s_div2mask.m_dword & (flagcolor >> 1));
                            } while (--count);
                        } else {
                            out -= run;
                        }
                        remaining -= run;
                        if (!remaining)
                            break;
                        packet = *src;
                        code = packet >> 5;
                        run = (packet & 31) + 1;
                        ++src;
                    } while (remaining);

                    rowOffset += dpitch;
                }
            }
        }
    }
}

// E:\gamedcs\cspriteframe.cpp:2645
// The adventure-object shadow pass. DrawAdvObjImpl's packed-cell decoder with
// the palette gone - nothing here reads `pal`, and the literal run that
// renderer draws is SKIPPED, because an object's opaque pixels are not what
// casts its shadow. What is drawn instead are the two control runs, darkening
// what is already on the destination by a quarter or a half.

// Its two callers name it: CSprite::DrawAdvObjShadow (0x47be60) and
// CSprite::DrawHeroShadow (0x47c0d0) both reach this address through
// `s[..]->f[framenum]->DrawAdvObjShadowImpl`, and nothing else calls it.

// The dispatch is a COMPARE CHAIN, not the jump table DrawTileShadow gets,
// because only codes 1 and 4 have arms and they are not adjacent - retail
// tests `dec eax / je` then `sub eax,3 / je` with both bodies sunk past the
// default. The arms emerge in reverse of the ascending dispatch order (the
// half blend first at 0x47db45, the quarter-plus-half second at 0x47db5f),
// which is what a compare-chain switch does regardless of source order.

// Residual (99.94%): 86 of 86 blocks EXACT, 42 of 42 branches, both returns,
// empty call multiset. All that is left is the three-quarter blend's two
// sites, the same B-family transposition DrawTileShadow carries below. The
// half blend DID have a source cause: the widening `unsigned int color =
// out[-1]` spelling cost nine flow-kind blocks and a whole missing block, and
// dropping it took this row 98.5765 -> 99.9400 on one line.
// Row-boundary residual (94.3325%): visited pointers use integral
// byte displacements; only the integer advances after the final row. The
// earlier guarded/break form scored 79.4119%. DC decoder/helper scopes
// and pixel arithmetic remain intact, including reverse/raw source walks.
VA(0x0047d930, 0x40F)  // anchor-callee (CSprite::DrawAdvObjShadow/DrawHeroShadow) + DC source identity
void CSpriteFrame::drawAdvObjShadowImpl(int sx, int sy, int sw, int sh,
                                        unsigned short* dst, int dx, int dy,
                                        int dw, int dh, int dpitch,
                                        TPalette16& pal,
                                        unsigned char hflip) const
{
    unsigned int cellsPerLine;
    const unsigned short* cellOffset;

    clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, 0);
    if (sw > 0) {
        if (sh > 0) {

            cellsPerLine = static_cast<unsigned int>(m_croppedWidth) >> 5;
            cellOffset = static_cast<const unsigned short*>(static_cast<const void*>(m_map));

            if (!hflip) {
                unsigned short* lineDst =
                    static_cast<unsigned short*>(static_cast<void*>(
                    static_cast<unsigned char*>(static_cast<void*>(dst)) +
                    dy * dpitch + dx * 2));

                unsigned char* rowBase = static_cast<unsigned char*>(static_cast<void*>(lineDst));
                int rowOffset = 0;
                for (int y = sy; y < sy + sh; ++y) {
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(rowBase + rowOffset));
                    unsigned short* out = lineDst;
                    unsigned int skipped = static_cast<unsigned int>(sx) & ~31U;
                    const unsigned char* src =
                        m_map + cellOffset[y * cellsPerLine +
                                          (static_cast<unsigned int>(sx) >> 5)];
                    unsigned char packet = *src;
                    unsigned char code = packet >> 5;
                    unsigned int run = (packet & 31) + 1;
                    ++src;

                    while (skipped + run <= static_cast<unsigned int>(sx)) {
                        skipped += run;
                        if (code == eRleControlOutline7)
                            src += run;
                        packet = *src;
                        code = packet >> 5;
                        run = (packet & 31) + 1;
                        ++src;
                    }

                    run += skipped - sx;
                    if (code == eRleControlOutline7)
                        src += sx - skipped;

                    unsigned int remaining = sw;
                    do {
                        if (run > remaining)
                            run = remaining;
                        if (code == eRleControlOutline7) {
                            out += run;
                            src += run;
                        } else {
                            switch (code) {
                            case eRleControlShadow75: {
                                unsigned int count = run;
                                do {
                                    *out = ((*out >> 2) & s_div4mask)
                                         + ((*out >> 1) & s_div2mask.m_dword);
                                    ++out;
                                } while (--count);
                                break;
                            }
                            case eRleControlShadow50: {
                                unsigned int count = run;
                                do {
                                    unsigned int color = *out;
                                    *out = (color >> 1) & s_div2mask.m_dword;
                                    ++out;
                                } while (--count);
                                break;
                            }
                            default:
                                out += run;
                                break;
                            }
                        }
                        remaining -= run;
                        if (!remaining)
                            break;
                        packet = *src;
                        code = packet >> 5;
                        run = (packet & 31) + 1;
                        ++src;
                    } while (remaining);

                    rowOffset += dpitch;
                }
            } else {
                unsigned short* lineDst =
                    static_cast<unsigned short*>(static_cast<void*>(
                    static_cast<unsigned char*>(static_cast<void*>(dst)) +
                    dy * dpitch + (dx + sw) * 2));

                unsigned char* rowBase = static_cast<unsigned char*>(static_cast<void*>(lineDst));
                int rowOffset = 0;
                for (int y = sy; y < sy + sh; ++y) {
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(rowBase + rowOffset));
                    unsigned short* out = lineDst;
                    unsigned int skipped = static_cast<unsigned int>(sx) & ~31U;
                    const unsigned char* src =
                        m_map + cellOffset[y * cellsPerLine +
                                          (static_cast<unsigned int>(sx) >> 5)];
                    unsigned char packet = *src;
                    unsigned char code = packet >> 5;
                    unsigned int run = (packet & 31) + 1;
                    ++src;

                    while (skipped + run <= static_cast<unsigned int>(sx)) {
                        skipped += run;
                        if (code == eRleControlOutline7)
                            src += run;
                        packet = *src;
                        code = packet >> 5;
                        run = (packet & 31) + 1;
                        ++src;
                    }

                    run += skipped - sx;
                    if (code == eRleControlOutline7)
                        src += sx - skipped;

                    unsigned int remaining = sw;
                    do {
                        if (run > remaining)
                            run = remaining;
                        if (code == eRleControlOutline7) {
                            out -= run;
                            src += run;
                        } else {
                            switch (code) {
                            case eRleControlShadow75: {
                                unsigned int count = run;
                                do {
                                    --out;
                                    *out = ((*out >> 2) & s_div4mask)
                                         + ((*out >> 1) & s_div2mask.m_dword);
                                } while (--count);
                                break;
                            }
                            case eRleControlShadow50: {
                                unsigned int count = run;
                                do {
                                    --out;
                                    *out = (*out >> 1) & s_div2mask.m_dword;
                                } while (--count);
                                break;
                            }
                            default:
                                out -= run;
                                break;
                            }
                        }
                        remaining -= run;
                        if (!remaining)
                            break;
                        packet = *src;
                        code = packet >> 5;
                        run = (packet & 31) + 1;
                        ++src;
                    } while (remaining);

                    rowOffset += dpitch;
                }
            }
        }
    }
}

// E:\gamedcs\cspriteframe.cpp:2856.  Raw tiles are palette-indexed rows;
// encoded tiles use a word row-offset table and the same packed packet byte as
// adventure cells.  Only code seven carries pixels in this renderer.

// Residual (83.2115%): four DC/retail direction arms, the eight-statement Duff
// loop, raw do/while rows, and indexed encoded for-rows recover all behavior.
// Split packet load/increment and block-scoped row destinations further match
// retail's packet schedule and dead-vflip parameter-home reuse.  The recovered
// `kOpaqueRunCode` and raw-row declaration order are byte-flat positive facts;
// the surviving delta is a C1 register permutation replicated in four arms.
// Retail and Dreamcast also agree on the surprising general-RLE delegation
// `Draw(sw, sy, sw, ...)`; spelling that positive fact alone scores 81.4249%
// because C1 then homes `this` in EDI across the whole body.  Swapping the two
// leading declarations and replacing the local constant with the existing
// code-7 enumerator are byte-flat paired probes.  The proven call spelling is
// retained despite that expected checkpoint dip while the surrounding source
// shape needed to restore retail's EDX home remains under reconstruction.
// Row-boundary residual (79.3933%): visited pointers use integral
// byte displacements; only the integer advances after the final row. The
// earlier guarded/break form scored 77.5881%. DC decoder/helper scopes
// and pixel arithmetic remain intact, including reverse/raw source walks.
VA(0x0047dd40, 0xAD8) // retail raw/tileset decoder + DC source identity
void CSpriteFrame::drawTile(int sx, int sy, int sw, int sh, unsigned short* dst,
                            int dx, int dy, int dw, int dh, int dpitch,
                            TPalette16& pal, unsigned char hflip,
                            unsigned char vflip) const
{
    const unsigned short* lineOffset;
    static const unsigned char opaqueRunCode = 7;

    if (m_encodingMethod == eEncodeGeneralRLE) {
        draw(sw, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, 1);
        return;
    }
    if (m_encodingMethod == eEncodeAdvObjRLE) {
        drawAdvObjImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip,
                       0);
        return;
    }

    clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, vflip);
    if (sw > 0) {
        if (sh > 0) {
            lineOffset = static_cast<const unsigned short*>(
                static_cast<const void*>(m_map));
            if (!vflip) {
                if (!hflip) {
                    unsigned short* lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(dst)) +
                        dy * dpitch + dx * 2));

                    if (m_encodingMethod == eEncodeRaw) {
                        const unsigned char* line = m_map + sy * m_pitch + sx;
                            unsigned char* rowBase = static_cast<unsigned char*>(static_cast<void*>(lineDst));
                            int rowOffset = 0;
                            const unsigned char* sourceRowBase = line;
                            int sourceRowOffset = 0;
                        do {
                                lineDst = static_cast<unsigned short*>(static_cast<void*>(rowBase + rowOffset));
                                line = sourceRowBase + sourceRowOffset;
                            int remaining = sw;
                            unsigned short* out = lineDst;
                            const unsigned char* src = line;
                            switch (remaining & 7) {
                            case eRawRowUnroll8:
                                do {
                                    *out++ = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll7:
                                    *out++ = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll6:
                                    *out++ = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll5:
                                    *out++ = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll4:
                                    *out++ = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll3:
                                    *out++ = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll2:
                                    *out++ = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll1:
                                    *out++ = pal.m_data[*src++];
                                    --remaining;
                                } while (remaining > 0);
                            }

                                sourceRowOffset += m_pitch;
                                rowOffset += dpitch;
                            } while (--sh > 0);
                    } else {
                            unsigned char* rowBase = static_cast<unsigned char*>(static_cast<void*>(lineDst));
                            int rowOffset = 0;
                        for (int y = sy; y < sy + sh; ++y) {
                                lineDst = static_cast<unsigned short*>(static_cast<void*>(rowBase + rowOffset));
                            unsigned short* out = lineDst;
                            const unsigned char* src = m_map + lineOffset[y];
                            unsigned int skipped = 0;
                            unsigned char packet = *src;
                            unsigned char code = packet >> 5;
                            unsigned int run = (packet & 31) + 1;
                            ++src;

                            while (skipped + run <=
                                   static_cast<unsigned int>(sx)) {
                                skipped += run;
                                if (code == opaqueRunCode)
                                    src += run;
                                packet = *src;
                                code = packet >> 5;
                                run = (packet & 31) + 1;
                                ++src;
                            }
                            run += skipped - sx;
                            if (code == opaqueRunCode)
                                src += sx - skipped;

                            unsigned int remaining = sw;
                            do {
                                if (run > remaining)
                                    run = remaining;
                                if (code == opaqueRunCode) {
                                    unsigned int count = run;
                                    do {
                                        *out++ = pal.m_data[*src++];
                                    } while (--count);
                                } else {
                                    out += run;
                                }
                                remaining -= run;
                                if (!remaining)
                                    break;
                                packet = *src;
                                code = packet >> 5;
                                run = (packet & 31) + 1;
                                ++src;
                            } while (remaining);

                                rowOffset += dpitch;
                            }
                    }
                } else {
                    unsigned short* lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(dst)) +
                        dy * dpitch + (dx + sw) * 2));

                    if (m_encodingMethod == eEncodeRaw) {
                        const unsigned char* line = m_map + sy * m_pitch + sx;
                            unsigned char* rowBase = static_cast<unsigned char*>(static_cast<void*>(lineDst));
                            int rowOffset = 0;
                            const unsigned char* sourceRowBase = line;
                            int sourceRowOffset = 0;
                        do {
                                lineDst = static_cast<unsigned short*>(static_cast<void*>(rowBase + rowOffset));
                                line = sourceRowBase + sourceRowOffset;
                            int remaining = sw;
                            unsigned short* out = lineDst;
                            const unsigned char* src = line;
                            switch (remaining & 7) {
                            case eRawRowUnroll8:
                                do {
                                    *--out = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll7:
                                    *--out = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll6:
                                    *--out = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll5:
                                    *--out = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll4:
                                    *--out = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll3:
                                    *--out = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll2:
                                    *--out = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll1:
                                    *--out = pal.m_data[*src++];
                                    --remaining;
                                } while (remaining > 0);
                            }

                                sourceRowOffset += m_pitch;
                                rowOffset += dpitch;
                            } while (--sh > 0);
                    } else {
                            unsigned char* rowBase = static_cast<unsigned char*>(static_cast<void*>(lineDst));
                            int rowOffset = 0;
                        for (int y = sy; y < sy + sh; ++y) {
                                lineDst = static_cast<unsigned short*>(static_cast<void*>(rowBase + rowOffset));
                            unsigned short* out = lineDst;
                            const unsigned char* src = m_map + lineOffset[y];
                            unsigned int skipped = 0;
                            unsigned char packet = *src;
                            unsigned char code = packet >> 5;
                            unsigned int run = (packet & 31) + 1;
                            ++src;

                            while (skipped + run <=
                                   static_cast<unsigned int>(sx)) {
                                skipped += run;
                                if (code == opaqueRunCode)
                                    src += run;
                                packet = *src;
                                code = packet >> 5;
                                run = (packet & 31) + 1;
                                ++src;
                            }
                            run += skipped - sx;
                            if (code == opaqueRunCode)
                                src += sx - skipped;

                            unsigned int remaining = sw;
                            do {
                                if (run > remaining)
                                    run = remaining;
                                if (code == opaqueRunCode) {
                                    unsigned int count = run;
                                    do {
                                        *--out = pal.m_data[*src++];
                                    } while (--count);
                                } else {
                                    out -= run;
                                }
                                remaining -= run;
                                if (!remaining)
                                    break;
                                packet = *src;
                                code = packet >> 5;
                                run = (packet & 31) + 1;
                                ++src;
                            } while (remaining);

                                rowOffset += dpitch;
                            }
                    }
                }
            } else {
                if (!hflip) {
                    unsigned short* lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(dst)) +
                        (dy + sh - 1) * dpitch + dx * 2));

                    if (m_encodingMethod == eEncodeRaw) {
                        const unsigned char* line = m_map + sy * m_pitch + sx;
                            unsigned char* rowBase = static_cast<unsigned char*>(static_cast<void*>(lineDst));
                            int rowOffset = 0;
                            const unsigned char* sourceRowBase = line;
                            int sourceRowOffset = 0;
                        do {
                                lineDst = static_cast<unsigned short*>(static_cast<void*>(rowBase - rowOffset));
                                line = sourceRowBase + sourceRowOffset;
                            int remaining = sw;
                            unsigned short* out = lineDst;
                            const unsigned char* src = line;
                            switch (remaining & 7) {
                            case eRawRowUnroll8:
                                do {
                                    *out++ = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll7:
                                    *out++ = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll6:
                                    *out++ = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll5:
                                    *out++ = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll4:
                                    *out++ = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll3:
                                    *out++ = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll2:
                                    *out++ = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll1:
                                    *out++ = pal.m_data[*src++];
                                    --remaining;
                                } while (remaining > 0);
                            }

                                sourceRowOffset += m_pitch;
                                rowOffset += dpitch;
                            } while (--sh > 0);
                    } else {
                            unsigned char* rowBase = static_cast<unsigned char*>(static_cast<void*>(lineDst));
                            int rowOffset = 0;
                        for (int y = sy; y < sy + sh; ++y) {
                                lineDst = static_cast<unsigned short*>(static_cast<void*>(rowBase - rowOffset));
                            unsigned short* out = lineDst;
                            const unsigned char* src = m_map + lineOffset[y];
                            unsigned int skipped = 0;
                            unsigned char packet = *src;
                            unsigned char code = packet >> 5;
                            unsigned int run = (packet & 31) + 1;
                            ++src;

                            while (skipped + run <=
                                   static_cast<unsigned int>(sx)) {
                                skipped += run;
                                if (code == opaqueRunCode)
                                    src += run;
                                packet = *src;
                                code = packet >> 5;
                                run = (packet & 31) + 1;
                                ++src;
                            }
                            run += skipped - sx;
                            if (code == opaqueRunCode)
                                src += sx - skipped;

                            unsigned int remaining = sw;
                            do {
                                if (run > remaining)
                                    run = remaining;
                                if (code == opaqueRunCode) {
                                    unsigned int count = run;
                                    do {
                                        *out++ = pal.m_data[*src++];
                                    } while (--count);
                                } else {
                                    out += run;
                                }
                                remaining -= run;
                                if (!remaining)
                                    break;
                                packet = *src;
                                code = packet >> 5;
                                run = (packet & 31) + 1;
                                ++src;
                            } while (remaining);

                                rowOffset += dpitch;
                            }
                    }
                } else {
                    unsigned short* lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(dst)) +
                        (dy + sh - 1) * dpitch + (dx + sw) * 2));

                    if (m_encodingMethod == eEncodeRaw) {
                        const unsigned char* line = m_map + sy * m_pitch + sx;
                            unsigned char* rowBase = static_cast<unsigned char*>(static_cast<void*>(lineDst));
                            int rowOffset = 0;
                            const unsigned char* sourceRowBase = line;
                            int sourceRowOffset = 0;
                        do {
                                lineDst = static_cast<unsigned short*>(static_cast<void*>(rowBase - rowOffset));
                                line = sourceRowBase + sourceRowOffset;
                            int remaining = sw;
                            unsigned short* out = lineDst;
                            const unsigned char* src = line;
                            switch (remaining & 7) {
                            case eRawRowUnroll8:
                                do {
                                    *--out = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll7:
                                    *--out = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll6:
                                    *--out = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll5:
                                    *--out = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll4:
                                    *--out = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll3:
                                    *--out = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll2:
                                    *--out = pal.m_data[*src++];
                                    --remaining;
                                case eRawRowUnroll1:
                                    *--out = pal.m_data[*src++];
                                    --remaining;
                                } while (remaining > 0);
                            }

                                sourceRowOffset += m_pitch;
                                rowOffset += dpitch;
                            } while (--sh > 0);
                    } else {
                            unsigned char* rowBase = static_cast<unsigned char*>(static_cast<void*>(lineDst));
                            int rowOffset = 0;
                        for (int y = sy; y < sy + sh; ++y) {
                                lineDst = static_cast<unsigned short*>(static_cast<void*>(rowBase - rowOffset));
                            unsigned short* out = lineDst;
                            const unsigned char* src = m_map + lineOffset[y];
                            unsigned int skipped = 0;
                            unsigned char packet = *src;
                            unsigned char code = packet >> 5;
                            unsigned int run = (packet & 31) + 1;
                            ++src;

                            while (skipped + run <=
                                   static_cast<unsigned int>(sx)) {
                                skipped += run;
                                if (code == opaqueRunCode)
                                    src += run;
                                packet = *src;
                                code = packet >> 5;
                                run = (packet & 31) + 1;
                                ++src;
                            }
                            run += skipped - sx;
                            if (code == opaqueRunCode)
                                src += sx - skipped;

                            unsigned int remaining = sw;
                            do {
                                if (run > remaining)
                                    run = remaining;
                                if (code == opaqueRunCode) {
                                    unsigned int count = run;
                                    do {
                                        *--out = pal.m_data[*src++];
                                    } while (--count);
                                } else {
                                    out -= run;
                                }
                                remaining -= run;
                                if (!remaining)
                                    break;
                                packet = *src;
                                code = packet >> 5;
                                run = (packet & 31) + 1;
                                ++src;
                            } while (remaining);

                                rowOffset += dpitch;
                            }
                    }
                }
            }
        }
    }
}

// E:\gamedcs\cspriteframe.cpp:3365
// The tileset shadow pass: same packed-cell walk as DrawTile above, where the
// palette write becomes a blend of what is already on the destination.
// Nothing here reads `pal` - the parameter survives because CSprite's two
// wrappers (0x47bfa0, 0x47bff0) pass it - and nothing reads the raw encoding
// either, which is why the body opens by returning on eEncodeRaw instead of
// carrying DrawTile's unrolled raw arm.

// Only two blends exist and the jump table at 0x47ef20 says which codes reach
// them: 1 and 2 take three quarters of the destination
// (`(p>>2)&div4mask + (p>>1)&div2mask`), 3 and 4 take a half, code 7's opaque
// run is skipped over rather than drawn, and every other code just advances.
// The mask widths are read straight from the bytes - div4mask is ANDed as a
// word and div2mask as a dword in all four arms.

// The single integral row cursor preserves the original codegen without
// forming an out-of-bounds pointer after the last row. The former
// row-base/offset spelling fell to 92.0317%; this restores 99.9308% with all
// 156 blocks exact, all 72 branches and all 4 returns.
//
// The remaining difference is the three-quarter blend's four sites. Retail
// computes the div4mask term into the COPY register (`mov bx,ax / shr bx,2 /
// and bx,word[div4mask]`) while VC6 computes the div2mask term there,
// emitting the same `add ebx,eax` and store. Source order is not the lever:
// writing the div2mask term first and naming either term in a local are
// byte-flat because VC6 canonicalises `+`. The half blend written
// `unsigned int color = out[-1]; --out; *out = (color >> 1) & mask;`
// emits a widening `xor eax,eax / mov ax,` pair retail does not have.
// `--out; *out = (*out >> 1) & mask;` reads the same location AFTER the
// decrement, which is what retail spells: 97.8963 -> 99.9300 here.
// DC decoder/helper scopes and pixel arithmetic remain intact, including
// reverse/raw source walks.
VA(0x0047e820, 0x740)  // anchor-callee (CSprite::DrawTileShadow/DrawShroudTile) + DC source identity
void CSpriteFrame::drawTileShadow(int sx, int sy, int sw, int sh,
                                  unsigned short* dst, int dx, int dy, int dw,
                                  int dh, int dpitch, TPalette16& pal,
                                  unsigned char hflip,
                                  unsigned char vflip) const
{
    union TLineAddress {
        unsigned short* pointer;
        unsigned long address;
    };

    static const unsigned char opaqueRunCode = 7;

    if (m_encodingMethod == eEncodeRaw)
        return;

    clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, vflip);
    if (sw > 0) {
        if (sh > 0) {
            const unsigned short* lineOffset =
                static_cast<const unsigned short*>(
                    static_cast<const void*>(m_map));
            if (!vflip) {
                if (!hflip) {
                    unsigned short* lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(dst)) +
                        dy * dpitch + dx * 2));

                    TLineAddress line;
                    line.pointer = lineDst;
                    for (int y = sy; y < sy + sh; ++y) {
                        lineDst = line.pointer;
                        unsigned short* out = lineDst;
                        const unsigned char* src = m_map + lineOffset[y];
                        unsigned int skipped = 0;
                        unsigned char packet = *src;
                        unsigned char code = packet >> 5;
                        unsigned int run = (packet & 31) + 1;
                        ++src;

                        while (skipped + run <=
                               static_cast<unsigned int>(sx)) {
                            skipped += run;
                            if (code == opaqueRunCode)
                                src += run;
                            packet = *src;
                            code = packet >> 5;
                            run = (packet & 31) + 1;
                            ++src;
                        }
                        run += skipped - sx;
                        if (code == opaqueRunCode)
                            src += sx - skipped;

                        unsigned int remaining = sw;
                        do {
                            if (run > remaining)
                                run = remaining;
                            if (code == opaqueRunCode) {
                                out += run;
                                src += run;
                            } else {
                                switch (code) {
                                case eRleControlShadow75:
                                case eRleControlShadow2: {
                                    unsigned int count = run;
                                    do {
                                        *out = ((*out >> 2) & s_div4mask)
                                             + ((*out >> 1) & s_div2mask.m_dword);
                                        ++out;
                                    } while (--count);
                                    break;
                                }
                                case eRleControlShadow3:
                                case eRleControlShadow50: {
                                    unsigned int count = run;
                                    do {
                                        unsigned int color = *out;
                                        *out = (color >> 1) & s_div2mask.m_dword;
                                        ++out;
                                    } while (--count);
                                    break;
                                }
                                default:
                                    out += run;
                                    break;
                                }
                            }
                            remaining -= run;
                            if (!remaining)
                                break;
                            packet = *src;
                            code = packet >> 5;
                            run = (packet & 31) + 1;
                            ++src;
                        } while (remaining);

                        line.address += dpitch;
                    }
                } else {
                    unsigned short* lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(dst)) +
                        dy * dpitch + (dx + sw) * 2));

                    TLineAddress line;
                    line.pointer = lineDst;
                    for (int y = sy; y < sy + sh; ++y) {
                        lineDst = line.pointer;
                        unsigned short* out = lineDst;
                        const unsigned char* src = m_map + lineOffset[y];
                        unsigned int skipped = 0;
                        unsigned char packet = *src;
                        unsigned char code = packet >> 5;
                        unsigned int run = (packet & 31) + 1;
                        ++src;

                        while (skipped + run <=
                               static_cast<unsigned int>(sx)) {
                            skipped += run;
                            if (code == opaqueRunCode)
                                src += run;
                            packet = *src;
                            code = packet >> 5;
                            run = (packet & 31) + 1;
                            ++src;
                        }
                        run += skipped - sx;
                        if (code == opaqueRunCode)
                            src += sx - skipped;

                        unsigned int remaining = sw;
                        do {
                            if (run > remaining)
                                run = remaining;
                            if (code == opaqueRunCode) {
                                out -= run;
                                src += run;
                            } else {
                                switch (code) {
                                case eRleControlShadow75:
                                case eRleControlShadow2: {
                                    unsigned int count = run;
                                    do {
                                        --out;
                                        *out = ((*out >> 2) & s_div4mask)
                                             + ((*out >> 1) & s_div2mask.m_dword);
                                    } while (--count);
                                    break;
                                }
                                case eRleControlShadow3:
                                case eRleControlShadow50: {
                                    unsigned int count = run;
                                    do {
                                        --out;
                                        *out = (*out >> 1) & s_div2mask.m_dword;
                                    } while (--count);
                                    break;
                                }
                                default:
                                    out -= run;
                                    break;
                                }
                            }
                            remaining -= run;
                            if (!remaining)
                                break;
                            packet = *src;
                            code = packet >> 5;
                            run = (packet & 31) + 1;
                            ++src;
                        } while (remaining);

                        line.address += dpitch;
                    }
                }
            } else {
                if (!hflip) {
                    unsigned short* lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(dst)) +
                        (dy + sh - 1) * dpitch + dx * 2));

                    TLineAddress line;
                    line.pointer = lineDst;
                    for (int y = sy; y < sy + sh; ++y) {
                        lineDst = line.pointer;
                        unsigned short* out = lineDst;
                        const unsigned char* src = m_map + lineOffset[y];
                        unsigned int skipped = 0;
                        unsigned char packet = *src;
                        unsigned char code = packet >> 5;
                        unsigned int run = (packet & 31) + 1;
                        ++src;

                        while (skipped + run <=
                               static_cast<unsigned int>(sx)) {
                            skipped += run;
                            if (code == opaqueRunCode)
                                src += run;
                            packet = *src;
                            code = packet >> 5;
                            run = (packet & 31) + 1;
                            ++src;
                        }
                        run += skipped - sx;
                        if (code == opaqueRunCode)
                            src += sx - skipped;

                        unsigned int remaining = sw;
                        do {
                            if (run > remaining)
                                run = remaining;
                            if (code == opaqueRunCode) {
                                out += run;
                                src += run;
                            } else {
                                switch (code) {
                                case eRleControlShadow75:
                                case eRleControlShadow2: {
                                    unsigned int count = run;
                                    do {
                                        *out = ((*out >> 2) & s_div4mask)
                                             + ((*out >> 1) & s_div2mask.m_dword);
                                        ++out;
                                    } while (--count);
                                    break;
                                }
                                case eRleControlShadow3:
                                case eRleControlShadow50: {
                                    unsigned int count = run;
                                    do {
                                        unsigned int color = *out;
                                        *out = (color >> 1) & s_div2mask.m_dword;
                                        ++out;
                                    } while (--count);
                                    break;
                                }
                                default:
                                    out += run;
                                    break;
                                }
                            }
                            remaining -= run;
                            if (!remaining)
                                break;
                            packet = *src;
                            code = packet >> 5;
                            run = (packet & 31) + 1;
                            ++src;
                        } while (remaining);

                        line.address -= dpitch;
                    }
                } else {
                    unsigned short* lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(dst)) +
                        (dy + sh - 1) * dpitch + (dx + sw) * 2));

                    TLineAddress line;
                    line.pointer = lineDst;
                    for (int y = sy; y < sy + sh; ++y) {
                        lineDst = line.pointer;
                        unsigned short* out = lineDst;
                        const unsigned char* src = m_map + lineOffset[y];
                        unsigned int skipped = 0;
                        unsigned char packet = *src;
                        unsigned char code = packet >> 5;
                        unsigned int run = (packet & 31) + 1;
                        ++src;

                        while (skipped + run <=
                               static_cast<unsigned int>(sx)) {
                            skipped += run;
                            if (code == opaqueRunCode)
                                src += run;
                            packet = *src;
                            code = packet >> 5;
                            run = (packet & 31) + 1;
                            ++src;
                        }
                        run += skipped - sx;
                        if (code == opaqueRunCode)
                            src += sx - skipped;

                        unsigned int remaining = sw;
                        do {
                            if (run > remaining)
                                run = remaining;
                            if (code == opaqueRunCode) {
                                out -= run;
                                src += run;
                            } else {
                                switch (code) {
                                case eRleControlShadow75:
                                case eRleControlShadow2: {
                                    unsigned int count = run;
                                    do {
                                        --out;
                                        *out = ((*out >> 2) & s_div4mask)
                                             + ((*out >> 1) & s_div2mask.m_dword);
                                    } while (--count);
                                    break;
                                }
                                case eRleControlShadow3:
                                case eRleControlShadow50: {
                                    unsigned int count = run;
                                    do {
                                        --out;
                                        *out = (*out >> 1) & s_div2mask.m_dword;
                                    } while (--count);
                                    break;
                                }
                                default:
                                    out -= run;
                                    break;
                                }
                            }
                            remaining -= run;
                            if (!remaining)
                                break;
                            packet = *src;
                            code = packet >> 5;
                            run = (packet & 31) + 1;
                            ++src;
                        } while (remaining);

                        line.address -= dpitch;
                    }
                }
            }
        }
    }
}

// E:\gamedcs\cspriteframe.cpp:3776
// The general-RLE spell-effect pass. Without alpha it is Draw's transparent
// blit verbatim (`tblit` 1), so the whole body below is the alpha case: literal
// runs are blended half-and-half against what is already on the destination,
// and every encoded control run is skipped rather than filled - which is the
// one place it departs from Draw, whose non-literal arm installs pal.data[code].

// The two delegations are proven by arity and argument order. 0x47f39c hands
// the tileset/raw encodings to DrawTile with a trailing 0, exactly as Draw
// does; 0x47efca hands the adventure-object encoding to 0x47d4f0 with the flag
// color 0 in slot 12 and hflip in slot 13, which is DrawAdvObjWithFlagAlpha's
// signature and no other in this class - DrawAdvObjImpl takes hflip in slot 12
// and its flag color last.
// Row-boundary residual (98.0214%): visited pointers use integral
// byte displacements; only the integer advances after the final row. The
// earlier guarded/break form scored 94.0235%. DC decoder/helper scopes
// and pixel arithmetic remain intact, including reverse/raw source walks.
VA(0x0047ef60, 0x47C)  // anchor-callee (CSprite::DrawSpellEffect) + DC source identity
void CSpriteFrame::drawSpellEffect(int sx, int sy, int sw, int sh,
                                   unsigned short* dst, int dx, int dy, int dw,
                                   int dh, int dpitch, TPalette16& pal,
                                   unsigned char hflip,
                                   unsigned char alpha) const
{
    if (!alpha) {
        draw(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, 1);
        return;
    }

    if (m_encodingMethod == eEncodeTilesetRLE || m_encodingMethod == eEncodeRaw) {
        drawTile(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                 pal, hflip, 0);
        return;
    }
    else if (m_encodingMethod == eEncodeAdvObjRLE) {
        drawHeroAlpha(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip);
        return;
    }

    const unsigned int* lineOffset;
    // DC 0x77664 local palette, bound after the line table at line 3811.
    const unsigned short* palette;
    static const unsigned char opaqueRunCode = g_generalRleOpaqueRunCode;
    clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, 0);

    if (sw > 0 && sh > 0) {
        lineOffset =
            static_cast<const unsigned int*>(static_cast<const void*>(m_map));
        palette = pal.m_data;
        if (!hflip) {
            unsigned short* lineDst =
                static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + dy * dpitch + dx * 2));

            unsigned char* rowBase = static_cast<unsigned char*>(static_cast<void*>(lineDst));
            int rowOffset = 0;
            for (int y = sy; y < sy + sh; ++y) {
                lineDst = static_cast<unsigned short*>(static_cast<void*>(rowBase + rowOffset));
                unsigned short* out = lineDst;
                unsigned int skipped = 0;
                const unsigned char* src = m_map + lineOffset[y];
                unsigned char code = *src++;
                unsigned int run = *src++ + 1;

                while (skipped + run <= static_cast<unsigned int>(sx)) {
                    skipped += run;
                    if (code == opaqueRunCode)
                        src += run;
                    code = *src++;
                    run = *src++ + 1;
                }

                run += skipped - sx;
                if (code == opaqueRunCode)
                    src += sx - skipped;

                unsigned int remaining = sw;
                do {
                    if (run > remaining)
                        run = remaining;
                    if (code == opaqueRunCode) {
                        unsigned int count = run;
                        do {
                            *out = (s_div2mask.m_dword
                                    & (palette[*src++] >> 1))
                                 + (s_div2mask.m_dword & (*out >> 1));
                            ++out;
                        } while (--count);
                    } else {
                        out += run;
                    }
                    remaining -= run;
                    if (!remaining)
                        break;
                    code = *src++;
                    run = *src++ + 1;
                } while (remaining);

                rowOffset += dpitch;
            }
        } else {
            unsigned short* lineDst =
                static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + dy * dpitch + (dx + sw) * 2));

            unsigned char* rowBase = static_cast<unsigned char*>(static_cast<void*>(lineDst));
            int rowOffset = 0;
            for (int y = sy; y < sy + sh; ++y) {
                lineDst = static_cast<unsigned short*>(static_cast<void*>(rowBase + rowOffset));
                unsigned short* out = lineDst;
                unsigned int skipped = 0;
                const unsigned char* src = m_map + lineOffset[y];
                unsigned char code = *src++;
                unsigned int run = *src++ + 1;

                while (skipped + run <= static_cast<unsigned int>(sx)) {
                    skipped += run;
                    if (code == opaqueRunCode)
                        src += run;
                    code = *src++;
                    run = *src++ + 1;
                }

                run += skipped - sx;
                if (code == opaqueRunCode)
                    src += sx - skipped;

                unsigned int remaining = sw;
                do {
                    if (run > remaining)
                        run = remaining;
                    if (code == opaqueRunCode) {
                        unsigned int count = run;
                        do {
                            --out;
                            *out = (s_div2mask.m_dword
                                    & (palette[*src++] >> 1))
                                 + (s_div2mask.m_dword & (*out >> 1));
                        } while (--count);
                    } else {
                        out -= run;
                    }
                    remaining -= run;
                    if (!remaining)
                        break;
                    code = *src++;
                    run = *src++ + 1;
                } while (remaining);

                rowOffset += dpitch;
            }
        }
    }
}

// Original: CSpriteFrame::ClipScaled50; cspriteframe.cpp:3949, dc 0x7799c.
void CSpriteFrame::clipScaled50(int& sx, int& sy, int& sw, int& sh,
                                    int& dx, int& dy, int dw, int dh,
                                    unsigned char hflip, unsigned char vflip) const
{
    int scaledWidth = (m_width + 1) >> 1;
    int scaledHeight = (m_height + 1) >> 1;
    if (hflip)
        sx = scaledWidth - (sx + sw);
    if (vflip)
        sy = scaledHeight - (sy + sh);
    if (dx < 0) {
        if (!hflip)
            sx -= dx;
        sw += dx;
        dx = 0;
    }
    if (dy < 0) {
        if (!vflip)
            sy -= dy;
        sh += dy;
        dy = 0;
    }
    if (sw + dx > dw) {
        if (hflip)
            sx += sw + dx - dw;
        sw = dw - dx;
    }
    if (sh + dy > dh) {
        if (vflip)
            sy += sh + dy - dh;
        sh = dh - dy;
    }
    int scaledCroppedX = (m_croppedX + 1) >> 1;
    int scaledCroppedY = (m_croppedY + 1) >> 1;
    int scaledCroppedWidth = ((m_croppedX + m_croppedWidth + 1) >> 1) - scaledCroppedX;
    int scaledCroppedHeight = ((m_croppedY + m_croppedHeight + 1) >> 1) - scaledCroppedY;
    if (sx < scaledCroppedX) {
        int delta = scaledCroppedX - sx;
        if (!hflip)
            dx += delta;
        sw -= delta;
        sx = scaledCroppedX;
    }
    if (sy < scaledCroppedY) {
        int delta = scaledCroppedY - sy;
        if (!vflip)
            dy += delta;
        sh -= delta;
        sy = scaledCroppedY;
    }
    int endX = scaledCroppedX + scaledCroppedWidth;
    if (sx + sw > endX) {
        if (hflip)
            dx += sx + sw - endX;
        sw = endX - sx;
    }
    int endY = scaledCroppedY + scaledCroppedHeight;
    if (sy + sh > endY) {
        if (vflip)
            dy += sy + sh - endY;
        sh = endY - sy;
    }
    sx <<= 1;
    sy <<= 1;
    sw <<= 1;
    sh <<= 1;
    sx -= m_croppedX;
    sy -= m_croppedY;
}

// Original: CSpriteFrame::DrawAdvObjWithFlagScaled50; cspriteframe.cpp:4054, dc 0x77b6c.
void CSpriteFrame::drawAdvObjWithFlagScaled50(int sx, int sy, int sw, int sh,
    unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
    TPalette16& pal, unsigned short flagcolor) const
{
    clipScaled50(sx, sy, sw, sh, dx, dy, dw, dh, 0, 0);
    if (sw <= 0 || sh <= 0)
        return;
    unsigned int cellsPerLine = m_croppedWidth >> 5;
    const unsigned short* const cellOffset = static_cast<const unsigned short*>(static_cast<const void*>(m_map));
    const unsigned short* const palette = pal.m_data;
    unsigned char* lineDst = static_cast<unsigned char*>(static_cast<void*>(dst)) + dy * dpitch + dx * 2;
    for (int y = sy; y < sy + sh; y += 2) {
        unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
        const unsigned char* source = m_map + cellOffset[y * cellsPerLine + (sx >> 5)];
        unsigned int x = sx & ~31;
        unsigned char code;
        unsigned int run;
        while (1) {
            unsigned char control = *source++;
            code = control >> 5;
            run = (control & 31) + 1;
            if (x + run > static_cast<unsigned int>(sx)) {
                run -= sx - x;
                if (code == ePackedRleLiteral)
                    source += sx - x;
                break;
            }
            x += run;
            if (code == ePackedRleLiteral)
                source += run;
        }
        unsigned int skip = 0;
        unsigned int remaining = sw;
        do {
            if (run > remaining)
                run = remaining;
            if (code == ePackedRleLiteral) {
                unsigned int count = run;
                if (skip) {
                    ++source;
                    --count;
                }
                while (count > 1) {
                    *out = palette[*source];
                    source += 2;
                    ++out;
                    count -= 2;
                }
                skip = count != 0;
                if (skip)
                    *out++ = palette[*source++];
            } else if (code == eRleControlOutline5 && flagcolor) {
                unsigned int count = run;
                if (skip)
                    --count;
                while (count > 1) {
                    *out++ = flagcolor;
                    count -= 2;
                }
                skip = count != 0;
                if (skip)
                    *out++ = flagcolor;
            } else {
                unsigned int count = run;
                if (skip)
                    --count;
                out += (count + 1) >> 1;
                skip = count & 1;
            }
            remaining -= run;
            if (!remaining)
                break;
            unsigned char control = *source++;
            code = control >> 5;
            run = (control & 31) + 1;
        } while (remaining);
        lineDst += dpitch;
    }
}

// Original: CSpriteFrame::DrawAdvObjShadowScaled50; cspriteframe.cpp:4187, dc 0x77d58.
void CSpriteFrame::drawAdvObjShadowScaled50(int sx, int sy, int sw, int sh,
    unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
    TPalette16& pal) const
{
    clipScaled50(sx, sy, sw, sh, dx, dy, dw, dh, 0, 0);
    if (sw <= 0 || sh <= 0)
        return;
    unsigned int cellsPerLine = m_croppedWidth >> 5;
    const unsigned short* const cellOffset = static_cast<const unsigned short*>(static_cast<const void*>(m_map));
    unsigned char* lineDst = static_cast<unsigned char*>(static_cast<void*>(dst)) + dy * dpitch + dx * 2;
    for (int y = sy; y < sy + sh; y += 2) {
        unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
        const unsigned char* source = m_map + cellOffset[y * cellsPerLine + (sx >> 5)];
        unsigned int x = sx & ~31;
        unsigned char code;
        unsigned int run;
        while (1) {
            unsigned char control = *source++;
            code = control >> 5;
            run = (control & 31) + 1;
            if (x + run > static_cast<unsigned int>(sx)) {
                run -= sx - x;
                if (code == ePackedRleLiteral)
                    source += sx - x;
                break;
            }
            x += run;
            if (code == ePackedRleLiteral)
                source += run;
        }
        unsigned int skip = 0;
        unsigned int remaining = sw;
        do {
            if (run > remaining)
                run = remaining;
            if (code == ePackedRleLiteral) {
                unsigned int count = run;
                if (skip)
                    --count;
                out += (count + 1) >> 1;
                skip = count & 1;
                source += run;
            } else {
                switch (code) {
                case eRleControlShadow75: {
                    unsigned int count = run;
                    if (skip)
                        --count;
                    while (count > 1) {
                        *out = ((*out >> 1) & s_div2mask.m_word) + ((*out >> 2) & s_div4mask);
                        ++out;
                        count -= 2;
                    }
                    skip = count != 0;
                    if (skip) {
                        *out = ((*out >> 1) & s_div2mask.m_word) + ((*out >> 2) & s_div4mask);
                        ++out;
                    }
                    break;
                }
                case eRleControlShadow50: {
                    unsigned int count = run;
                    if (skip)
                        --count;
                    while (count > 1) {
                        *out = (*out >> 1) & s_div2mask.m_word;
                        ++out;
                        count -= 2;
                    }
                    skip = count != 0;
                    if (skip) {
                        *out = (*out >> 1) & s_div2mask.m_word;
                        ++out;
                    }
                    break;
                }
                default: {
                    unsigned int count = run;
                    if (skip)
                        --count;
                    out += (count + 1) >> 1;
                    skip = count & 1;
                    break;
                }
                }
            }
            remaining -= run;
            if (!remaining)
                break;
            unsigned char control = *source++;
            code = control >> 5;
            run = (control & 31) + 1;
        } while (remaining);
        lineDst += dpitch;
    }
}

// Original: CSpriteFrame::DrawTileScaled50; cspriteframe.cpp:4330, dc 0x77f98.
void CSpriteFrame::drawTileScaled50(int sx, int sy, int sw, int sh,
    unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
    TPalette16& pal, unsigned char hflip, unsigned char vflip) const
{
    clipScaled50(sx, sy, sw, sh, dx, dy, dw, dh, hflip, vflip);
    if (sw <= 0 || sh <= 0)
        return;
    const unsigned short* const lineOffset = static_cast<const unsigned short*>(static_cast<const void*>(m_map));
    const unsigned short* const palette = pal.m_data;
    if (!vflip) {
        if (!hflip) {
            unsigned char* lineDst = static_cast<unsigned char*>(static_cast<void*>(dst))
                + (dy) * dpitch + (dx) * 2;
            if (m_encodingMethod == eEncodeRaw) {
                const unsigned char* line = m_map + sy * m_pitch + sx;
                do {
                    int remaining = sw;
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    const unsigned char* source = line;
                    do {
                        *out++ = palette[*source];
                        source += 2;
                        remaining -= 2;
                    } while (remaining > 0);
                    line += m_pitch * 2;
                    sh -= 2;
                    lineDst += dpitch;
                } while (sh > 0);
            } else {
                for (int y = sy; y < sy + sh; y += 2) {
                    const unsigned char* source = m_map + lineOffset[y];
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    unsigned int x = 0;
                    unsigned char code;
                    unsigned int run;
                    while (1) {
                        unsigned char control = *source++;
                        code = control >> 5;
                        run = (control & 31) + 1;
                        if (x + run > static_cast<unsigned int>(sx)) {
                            run -= sx - x;
                            if (code == ePackedRleLiteral)
                                source += sx - x;
                            break;
                        }
                        x += run;
                        if (code == ePackedRleLiteral)
                            source += run;
                    }
                    unsigned int skip = 0;
                    unsigned int remaining = sw;
                    do {
                        if (run > remaining)
                            run = remaining;
                        if (code == ePackedRleLiteral) {
                            unsigned int count = run;
                            if (skip) {
                                ++source;
                                --count;
                            }
                            while (count > 1) {
                                *out++ = palette[*source];
                                source += 2;
                                count -= 2;
                            }
                            skip = count != 0;
                            if (skip)
                                *out++ = palette[*source++];
                        } else {
                            unsigned int count = run;
                            if (skip)
                                --count;
                            out += (count + 1) >> 1;
                            skip = count & 1;
                        }
                        remaining -= run;
                        if (!remaining)
                            break;
                        unsigned char control = *source++;
                        code = control >> 5;
                        run = (control & 31) + 1;
                    } while (remaining);
                    lineDst += dpitch;
                }
            }
        } else {
            unsigned char* lineDst = static_cast<unsigned char*>(static_cast<void*>(dst))
                + (dy) * dpitch + (dx + (sw >> 1)) * 2;
            if (m_encodingMethod == eEncodeRaw) {
                const unsigned char* line = m_map + sy * m_pitch + sx;
                do {
                    int remaining = sw;
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    const unsigned char* source = line;
                    do {
                        *--out = palette[*source];
                        source += 2;
                        remaining -= 2;
                    } while (remaining > 0);
                    line += m_pitch * 2;
                    sh -= 2;
                    lineDst += dpitch;
                } while (sh > 0);
            } else {
                for (int y = sy; y < sy + sh; y += 2) {
                    const unsigned char* source = m_map + lineOffset[y];
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    unsigned int x = 0;
                    unsigned char code;
                    unsigned int run;
                    while (1) {
                        unsigned char control = *source++;
                        code = control >> 5;
                        run = (control & 31) + 1;
                        if (x + run > static_cast<unsigned int>(sx)) {
                            run -= sx - x;
                            if (code == ePackedRleLiteral)
                                source += sx - x;
                            break;
                        }
                        x += run;
                        if (code == ePackedRleLiteral)
                            source += run;
                    }
                    unsigned int skip = 0;
                    unsigned int remaining = sw;
                    do {
                        if (run > remaining)
                            run = remaining;
                        if (code == ePackedRleLiteral) {
                            unsigned int count = run;
                            if (skip) {
                                ++source;
                                --count;
                            }
                            while (count > 1) {
                                *--out = palette[*source];
                                source += 2;
                                count -= 2;
                            }
                            skip = count != 0;
                            if (skip)
                                *--out = palette[*source++];
                        } else {
                            unsigned int count = run;
                            if (skip)
                                --count;
                            out -= (count + 1) >> 1;
                            skip = count & 1;
                        }
                        remaining -= run;
                        if (!remaining)
                            break;
                        unsigned char control = *source++;
                        code = control >> 5;
                        run = (control & 31) + 1;
                    } while (remaining);
                    lineDst += dpitch;
                }
            }
        }
    } else {
        if (!hflip) {
            unsigned char* lineDst = static_cast<unsigned char*>(static_cast<void*>(dst))
                + (dy + (sh >> 1) - 1) * dpitch + (dx) * 2;
            if (m_encodingMethod == eEncodeRaw) {
                const unsigned char* line = m_map + sy * m_pitch + sx;
                do {
                    int remaining = sw;
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    const unsigned char* source = line;
                    do {
                        *out++ = palette[*source];
                        source += 2;
                        remaining -= 2;
                    } while (remaining > 0);
                    line += m_pitch * 2;
                    sh -= 2;
                    lineDst -= dpitch;
                } while (sh > 0);
            } else {
                for (int y = sy; y < sy + sh; y += 2) {
                    const unsigned char* source = m_map + lineOffset[y];
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    unsigned int x = 0;
                    unsigned char code;
                    unsigned int run;
                    while (1) {
                        unsigned char control = *source++;
                        code = control >> 5;
                        run = (control & 31) + 1;
                        if (x + run > static_cast<unsigned int>(sx)) {
                            run -= sx - x;
                            if (code == ePackedRleLiteral)
                                source += sx - x;
                            break;
                        }
                        x += run;
                        if (code == ePackedRleLiteral)
                            source += run;
                    }
                    unsigned int skip = 0;
                    unsigned int remaining = sw;
                    do {
                        if (run > remaining)
                            run = remaining;
                        if (code == ePackedRleLiteral) {
                            unsigned int count = run;
                            if (skip) {
                                ++source;
                                --count;
                            }
                            while (count > 1) {
                                *out++ = palette[*source];
                                source += 2;
                                count -= 2;
                            }
                            skip = count != 0;
                            if (skip)
                                *out++ = palette[*source++];
                        } else {
                            unsigned int count = run;
                            if (skip)
                                --count;
                            out += (count + 1) >> 1;
                            skip = count & 1;
                        }
                        remaining -= run;
                        if (!remaining)
                            break;
                        unsigned char control = *source++;
                        code = control >> 5;
                        run = (control & 31) + 1;
                    } while (remaining);
                    lineDst -= dpitch;
                }
            }
        } else {
            unsigned char* lineDst = static_cast<unsigned char*>(static_cast<void*>(dst))
                + (dy + (sh >> 1) - 1) * dpitch + (dx + (sw >> 1)) * 2;
            if (m_encodingMethod == eEncodeRaw) {
                const unsigned char* line = m_map + sy * m_pitch + sx;
                do {
                    int remaining = sw;
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    const unsigned char* source = line;
                    do {
                        *--out = palette[*source];
                        source += 2;
                        remaining -= 2;
                    } while (remaining > 0);
                    line += m_pitch * 2;
                    sh -= 2;
                    lineDst -= dpitch;
                } while (sh > 0);
            } else {
                for (int y = sy; y < sy + sh; y += 2) {
                    const unsigned char* source = m_map + lineOffset[y];
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    unsigned int x = 0;
                    unsigned char code;
                    unsigned int run;
                    while (1) {
                        unsigned char control = *source++;
                        code = control >> 5;
                        run = (control & 31) + 1;
                        if (x + run > static_cast<unsigned int>(sx)) {
                            run -= sx - x;
                            if (code == ePackedRleLiteral)
                                source += sx - x;
                            break;
                        }
                        x += run;
                        if (code == ePackedRleLiteral)
                            source += run;
                    }
                    unsigned int skip = 0;
                    unsigned int remaining = sw;
                    do {
                        if (run > remaining)
                            run = remaining;
                        if (code == ePackedRleLiteral) {
                            unsigned int count = run;
                            if (skip) {
                                ++source;
                                --count;
                            }
                            while (count > 1) {
                                *--out = palette[*source];
                                source += 2;
                                count -= 2;
                            }
                            skip = count != 0;
                            if (skip)
                                *--out = palette[*source++];
                        } else {
                            unsigned int count = run;
                            if (skip)
                                --count;
                            out -= (count + 1) >> 1;
                            skip = count & 1;
                        }
                        remaining -= run;
                        if (!remaining)
                            break;
                        unsigned char control = *source++;
                        code = control >> 5;
                        run = (control & 31) + 1;
                    } while (remaining);
                    lineDst -= dpitch;
                }
            }
        }
    }
}

// Original: CSpriteFrame::ClipScaled25; cspriteframe.cpp:4802, dc 0x785a0.
void CSpriteFrame::clipScaled25(int& sx, int& sy, int& sw, int& sh,
                                    int& dx, int& dy, int dw, int dh,
                                    unsigned char hflip, unsigned char vflip) const
{
    int scaledWidth = (m_width + 3) >> 2;
    int scaledHeight = (m_height + 3) >> 2;
    if (hflip)
        sx = scaledWidth - (sx + sw);
    if (vflip)
        sy = scaledHeight - (sy + sh);
    if (dx < 0) {
        if (!hflip)
            sx -= dx;
        sw += dx;
        dx = 0;
    }
    if (dy < 0) {
        if (!vflip)
            sy -= dy;
        sh += dy;
        dy = 0;
    }
    if (sw + dx > dw) {
        if (hflip)
            sx += sw + dx - dw;
        sw = dw - dx;
    }
    if (sh + dy > dh) {
        if (vflip)
            sy += sh + dy - dh;
        sh = dh - dy;
    }
    int scaledCroppedX = (m_croppedX + 3) >> 2;
    int scaledCroppedY = (m_croppedY + 3) >> 2;
    int scaledCroppedWidth = ((m_croppedX + m_croppedWidth + 3) >> 2) - scaledCroppedX;
    int scaledCroppedHeight = ((m_croppedY + m_croppedHeight + 3) >> 2) - scaledCroppedY;
    if (sx < scaledCroppedX) {
        int delta = scaledCroppedX - sx;
        if (!hflip)
            dx += delta;
        sw -= delta;
        sx = scaledCroppedX;
    }
    if (sy < scaledCroppedY) {
        int delta = scaledCroppedY - sy;
        if (!vflip)
            dy += delta;
        sh -= delta;
        sy = scaledCroppedY;
    }
    int endX = scaledCroppedX + scaledCroppedWidth;
    if (sx + sw > endX) {
        if (hflip)
            dx += sx + sw - endX;
        sw = endX - sx;
    }
    int endY = scaledCroppedY + scaledCroppedHeight;
    if (sy + sh > endY) {
        if (vflip)
            dy += sy + sh - endY;
        sh = endY - sy;
    }
    sx <<= 2;
    sy <<= 2;
    sw <<= 2;
    sh <<= 2;
    sx -= m_croppedX;
    sy -= m_croppedY;
}

// Original: CSpriteFrame::DrawAdvObjWithFlagScaled25; cspriteframe.cpp:4907, dc 0x78774.
void CSpriteFrame::drawAdvObjWithFlagScaled25(int sx, int sy, int sw, int sh,
    unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
    TPalette16& pal, unsigned short flagcolor) const
{
    clipScaled25(sx, sy, sw, sh, dx, dy, dw, dh, 0, 0);
    if (sw <= 0 || sh <= 0)
        return;
    unsigned int cellsPerLine = m_croppedWidth >> 5;
    const unsigned short* const cellOffset = static_cast<const unsigned short*>(static_cast<const void*>(m_map));
    const unsigned short* const palette = pal.m_data;
    unsigned char* lineDst = static_cast<unsigned char*>(static_cast<void*>(dst)) + dy * dpitch + dx * 2;
    for (int y = sy; y < sy + sh; y += 4) {
        unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
        const unsigned char* source = m_map + cellOffset[y * cellsPerLine + (sx >> 5)];
        unsigned int x = sx & ~31;
        unsigned char code;
        unsigned int run;
        while (1) {
            unsigned char control = *source++;
            code = control >> 5;
            run = (control & 31) + 1;
            if (x + run > static_cast<unsigned int>(sx)) {
                run -= sx - x;
                if (code == ePackedRleLiteral)
                    source += sx - x;
                break;
            }
            x += run;
            if (code == ePackedRleLiteral)
                source += run;
        }
        unsigned int skip = 0;
        unsigned int remaining = sw;
        do {
            if (run > remaining)
                run = remaining;
            if (code == ePackedRleLiteral) {
                if (run > skip) {
                    unsigned int count = run - skip;
                    source += skip;
                    while (count > 3) {
                        *out = palette[*source];
                        source += 4;
                        ++out;
                        count -= 4;
                    }
                    if (count) {
                        *out = palette[*source];
                        source += count;
                        ++out;
                        skip = 4 - count;
                    } else {
                        skip = 0;
                    }
                } else {
                    source += run;
                    skip -= run;
                }
            } else if (run > skip) {
                if (code == eRleControlOutline5 && flagcolor) {
                    unsigned int count = run - skip;
                    while (count > 3) {
                        *out++ = flagcolor;
                        // DC 5012 subtracts two even in this quarter-size path.
                        count -= 2;
                    }
                    if (count) {
                        *out++ = flagcolor;
                        skip = 4 - count;
                    } else {
                        skip = 0;
                    }
                } else {
                    unsigned int count = run - skip;
                    out += (count + 3) >> 2;
                    skip = count & 3;
                    skip = skip ? 4 - skip : 0;
                }
            } else {
                skip -= run;
            }
            remaining -= run;
            if (!remaining)
                break;
            unsigned char control = *source++;
            code = control >> 5;
            run = (control & 31) + 1;
        } while (remaining);
        lineDst += dpitch;
    }
}

// Original: CSpriteFrame::DrawAdvObjShadowScaled25; cspriteframe.cpp:5054, dc 0x7897c.
void CSpriteFrame::drawAdvObjShadowScaled25(int sx, int sy, int sw, int sh,
    unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
    TPalette16& pal) const
{
    clipScaled25(sx, sy, sw, sh, dx, dy, dw, dh, 0, 0);
    if (sw <= 0 || sh <= 0)
        return;
    unsigned int cellsPerLine = m_croppedWidth >> 5;
    const unsigned short* const cellOffset = static_cast<const unsigned short*>(static_cast<const void*>(m_map));
    unsigned char* lineDst = static_cast<unsigned char*>(static_cast<void*>(dst)) + dy * dpitch + dx * 2;
    for (int y = sy; y < sy + sh; y += 4) {
        unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
        const unsigned char* source = m_map + cellOffset[y * cellsPerLine + (sx >> 5)];
        unsigned int x = sx & ~31;
        unsigned char code;
        unsigned int run;
        while (1) {
            unsigned char control = *source++;
            code = control >> 5;
            run = (control & 31) + 1;
            if (x + run > static_cast<unsigned int>(sx)) {
                run -= sx - x;
                if (code == ePackedRleLiteral)
                    source += sx - x;
                break;
            }
            x += run;
            if (code == ePackedRleLiteral)
                source += run;
        }
        unsigned int skip = 0;
        unsigned int remaining = sw;
        do {
            if (run > remaining)
                run = remaining;
            if (code == ePackedRleLiteral) {
                if (run > skip) {
                    unsigned int count = run - skip;
                    out += (count + 3) >> 2;
                    skip = count & 3;
                    skip = skip ? 4 - skip : 0;
                } else {
                    skip -= run;
                }
                source += run;
            } else if (run > skip) {
                switch (code) {
                case eRleControlShadow75: {
                    unsigned int count = run - skip;
                    while (count > 3) {
                        *out = ((*out >> 1) & s_div2mask.m_word) + ((*out >> 2) & s_div4mask);
                        ++out;
                        count -= 4;
                    }
                    if (count) {
                        *out = ((*out >> 1) & s_div2mask.m_word) + ((*out >> 2) & s_div4mask);
                        // DC's partial quarter-size shadow run does not advance out.
                        skip = 4 - count;
                    } else {
                        skip = 0;
                    }
                    break;
                }
                case eRleControlShadow50: {
                    unsigned int count = run - skip;
                    while (count > 3) {
                        *out = (*out >> 1) & s_div2mask.m_word;
                        ++out;
                        count -= 4;
                    }
                    if (count) {
                        *out = (*out >> 1) & s_div2mask.m_word;
                        // DC's partial quarter-size shadow run does not advance out.
                        skip = 4 - count;
                    } else {
                        skip = 0;
                    }
                    break;
                }
                default: {
                    unsigned int count = run - skip;
                    out += (count + 3) >> 2;
                    skip = count & 3;
                    skip = skip ? 4 - skip : 0;
                    break;
                }
                }
            } else {
                skip -= run;
            }
            remaining -= run;
            if (!remaining)
                break;
            unsigned char control = *source++;
            code = control >> 5;
            run = (control & 31) + 1;
        } while (remaining);
        lineDst += dpitch;
    }
}

// Original: CSpriteFrame::DrawTileScaled25; cspriteframe.cpp:5201, dc 0x78be0.
void CSpriteFrame::drawTileScaled25(int sx, int sy, int sw, int sh,
    unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
    TPalette16& pal, unsigned char hflip, unsigned char vflip) const
{
    clipScaled25(sx, sy, sw, sh, dx, dy, dw, dh, hflip, vflip);
    if (sw <= 0 || sh <= 0)
        return;
    const unsigned short* const lineOffset = static_cast<const unsigned short*>(static_cast<const void*>(m_map));
    const unsigned short* const palette = pal.m_data;
    if (!vflip) {
        if (!hflip) {
            unsigned char* lineDst = static_cast<unsigned char*>(static_cast<void*>(dst))
                + (dy) * dpitch + (dx) * 2;
            if (m_encodingMethod == eEncodeRaw) {
                const unsigned char* line = m_map + sy * m_pitch + sx;
                do {
                    int remaining = sw;
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    const unsigned char* source = line;
                    do {
                        *out++ = palette[*source];
                        source += 4;
                        remaining -= 4;
                    } while (remaining > 0);
                    line += m_pitch * 4;
                    sh -= 4;
                    lineDst += dpitch;
                } while (sh > 0);
            } else {
                for (int y = sy; y < sy + sh; y += 4) {
                    const unsigned char* source = m_map + lineOffset[y];
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    unsigned int x = 0;
                    unsigned char code;
                    unsigned int run;
                    while (1) {
                        unsigned char control = *source++;
                        code = control >> 5;
                        run = (control & 31) + 1;
                        if (x + run > static_cast<unsigned int>(sx)) {
                            run -= sx - x;
                            if (code == ePackedRleLiteral)
                                source += sx - x;
                            break;
                        }
                        x += run;
                        if (code == ePackedRleLiteral)
                            source += run;
                    }
                    unsigned int skip = 0;
                    unsigned int remaining = sw;
                    do {
                        if (run > remaining)
                            run = remaining;
                        if (code == ePackedRleLiteral) {
                            if (run > skip) {
                                unsigned int count = run - skip;
                                source += skip;
                                while (count > 3) {
                                    *out++ = palette[*source];
                                    source += 4;
                                    count -= 4;
                                }
                                if (count) {
                                    *out++ = palette[*source];
                                    source += count;
                                    skip = 4 - count;
                                } else {
                                    skip = 0;
                                }
                            } else {
                                source += run;
                                skip -= run;
                            }
                        } else if (run > skip) {
                            unsigned int count = run - skip;
                            out += (count + 3) >> 2;
                            skip = count & 3;
                            skip = skip ? 4 - skip : 0;
                        } else {
                            skip -= run;
                        }
                        remaining -= run;
                        if (!remaining)
                            break;
                        unsigned char control = *source++;
                        code = control >> 5;
                        run = (control & 31) + 1;
                    } while (remaining);
                    lineDst += dpitch;
                }
            }
        } else {
            unsigned char* lineDst = static_cast<unsigned char*>(static_cast<void*>(dst))
                + (dy) * dpitch + (dx + (sw >> 2)) * 2;
            if (m_encodingMethod == eEncodeRaw) {
                const unsigned char* line = m_map + sy * m_pitch + sx;
                do {
                    int remaining = sw;
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    const unsigned char* source = line;
                    do {
                        *--out = palette[*source];
                        source += 4;
                        remaining -= 4;
                    } while (remaining > 0);
                    line += m_pitch * 4;
                    sh -= 4;
                    lineDst += dpitch;
                } while (sh > 0);
            } else {
                for (int y = sy; y < sy + sh; y += 4) {
                    const unsigned char* source = m_map + lineOffset[y];
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    unsigned int x = 0;
                    unsigned char code;
                    unsigned int run;
                    while (1) {
                        unsigned char control = *source++;
                        code = control >> 5;
                        run = (control & 31) + 1;
                        if (x + run > static_cast<unsigned int>(sx)) {
                            run -= sx - x;
                            if (code == ePackedRleLiteral)
                                source += sx - x;
                            break;
                        }
                        x += run;
                        if (code == ePackedRleLiteral)
                            source += run;
                    }
                    unsigned int skip = 0;
                    unsigned int remaining = sw;
                    do {
                        if (run > remaining)
                            run = remaining;
                        if (code == ePackedRleLiteral) {
                            if (run > skip) {
                                unsigned int count = run - skip;
                                source += skip;
                                while (count > 3) {
                                    *--out = palette[*source];
                                    source += 4;
                                    count -= 4;
                                }
                                if (count) {
                                    *--out = palette[*source];
                                    source += count;
                                    skip = 4 - count;
                                } else {
                                    skip = 0;
                                }
                            } else {
                                source += run;
                                skip -= run;
                            }
                        } else if (run > skip) {
                            unsigned int count = run - skip;
                            out -= (count + 3) >> 2;
                            skip = count & 3;
                            skip = skip ? 4 - skip : 0;
                        } else {
                            skip -= run;
                        }
                        remaining -= run;
                        if (!remaining)
                            break;
                        unsigned char control = *source++;
                        code = control >> 5;
                        run = (control & 31) + 1;
                    } while (remaining);
                    lineDst += dpitch;
                }
            }
        }
    } else {
        if (!hflip) {
            unsigned char* lineDst = static_cast<unsigned char*>(static_cast<void*>(dst))
                + (dy + (sh >> 2) - 1) * dpitch + (dx) * 2;
            if (m_encodingMethod == eEncodeRaw) {
                const unsigned char* line = m_map + sy * m_pitch + sx;
                do {
                    int remaining = sw;
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    const unsigned char* source = line;
                    do {
                        *out++ = palette[*source];
                        source += 4;
                        remaining -= 4;
                    } while (remaining > 0);
                    line += m_pitch * 4;
                    sh -= 4;
                    lineDst -= dpitch;
                } while (sh > 0);
            } else {
                for (int y = sy; y < sy + sh; y += 4) {
                    const unsigned char* source = m_map + lineOffset[y];
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    unsigned int x = 0;
                    unsigned char code;
                    unsigned int run;
                    while (1) {
                        unsigned char control = *source++;
                        code = control >> 5;
                        run = (control & 31) + 1;
                        if (x + run > static_cast<unsigned int>(sx)) {
                            run -= sx - x;
                            if (code == ePackedRleLiteral)
                                source += sx - x;
                            break;
                        }
                        x += run;
                        if (code == ePackedRleLiteral)
                            source += run;
                    }
                    unsigned int skip = 0;
                    unsigned int remaining = sw;
                    do {
                        if (run > remaining)
                            run = remaining;
                        if (code == ePackedRleLiteral) {
                            if (run > skip) {
                                unsigned int count = run - skip;
                                source += skip;
                                while (count > 3) {
                                    *out++ = palette[*source];
                                    source += 4;
                                    count -= 4;
                                }
                                if (count) {
                                    *out++ = palette[*source];
                                    source += count;
                                    skip = 4 - count;
                                } else {
                                    skip = 0;
                                }
                            } else {
                                source += run;
                                skip -= run;
                            }
                        } else if (run > skip) {
                            unsigned int count = run - skip;
                            out += (count + 3) >> 2;
                            skip = count & 3;
                            skip = skip ? 4 - skip : 0;
                        } else {
                            skip -= run;
                        }
                        remaining -= run;
                        if (!remaining)
                            break;
                        unsigned char control = *source++;
                        code = control >> 5;
                        run = (control & 31) + 1;
                    } while (remaining);
                    lineDst -= dpitch;
                }
            }
        } else {
            unsigned char* lineDst = static_cast<unsigned char*>(static_cast<void*>(dst))
                + (dy + (sh >> 2) - 1) * dpitch + (dx + (sw >> 2)) * 2;
            if (m_encodingMethod == eEncodeRaw) {
                const unsigned char* line = m_map + sy * m_pitch + sx;
                do {
                    int remaining = sw;
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    const unsigned char* source = line;
                    do {
                        *--out = palette[*source];
                        source += 4;
                        remaining -= 4;
                    } while (remaining > 0);
                    line += m_pitch * 4;
                    sh -= 4;
                    lineDst -= dpitch;
                } while (sh > 0);
            } else {
                for (int y = sy; y < sy + sh; y += 4) {
                    const unsigned char* source = m_map + lineOffset[y];
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    unsigned int x = 0;
                    unsigned char code;
                    unsigned int run;
                    while (1) {
                        unsigned char control = *source++;
                        code = control >> 5;
                        run = (control & 31) + 1;
                        if (x + run > static_cast<unsigned int>(sx)) {
                            run -= sx - x;
                            if (code == ePackedRleLiteral)
                                source += sx - x;
                            break;
                        }
                        x += run;
                        if (code == ePackedRleLiteral)
                            source += run;
                    }
                    unsigned int skip = 0;
                    unsigned int remaining = sw;
                    do {
                        if (run > remaining)
                            run = remaining;
                        if (code == ePackedRleLiteral) {
                            if (run > skip) {
                                unsigned int count = run - skip;
                                source += skip;
                                while (count > 3) {
                                    *--out = palette[*source];
                                    source += 4;
                                    count -= 4;
                                }
                                if (count) {
                                    *--out = palette[*source];
                                    source += count;
                                    skip = 4 - count;
                                } else {
                                    skip = 0;
                                }
                            } else {
                                source += run;
                                skip -= run;
                            }
                        } else if (run > skip) {
                            unsigned int count = run - skip;
                            out -= (count + 3) >> 2;
                            skip = count & 3;
                            skip = skip ? 4 - skip : 0;
                        } else {
                            skip -= run;
                        }
                        remaining -= run;
                        if (!remaining)
                            break;
                        unsigned char control = *source++;
                        code = control >> 5;
                        run = (control & 31) + 1;
                    } while (remaining);
                    lineDst -= dpitch;
                }
            }
        }
    }
}
