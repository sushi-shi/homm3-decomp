#include "va.h"

#include <new>
#include <limits>
#include <string.h>

#include "cspriteframe.h"

#include "palette.h"
#include "pcx.h"

// Static mask storage, set by SetPixelFormat before sprite drawing.
DATA(0x006968a4) unsigned short CSpriteFrame::div2mask;
DATA(0x006968aa) unsigned short CSpriteFrame::div4mask;


// The TU initializer at 0x47c260 installs the general-RLE literal-run code.
// Draw copies it into a function-local static on first use, accounting for
// the guard and one-byte local-static storage seen in retail.
// DC kGeneralRLEOpaqueRunCode is a file-static const unsigned char; the TU
// retains std::_Integer_limits<unsigned char,0,255,-1>::max at 0x79418.
DATA(0x006968a6) static const unsigned char kGeneralRLEOpaqueRunCode =
    std::numeric_limits<unsigned char>::max();

// Original: kGeneralRLEMaxRunLength, file-static const unsigned int.
// DC cspriteframe.cpp:47 calls the same max helper, then adds one;
// retail CRT 0x47c270 installs 256 in this otherwise unreferenced storage.
DATA(0x006968b0) static const unsigned int kGeneralRLEMaxRunLength =
    std::numeric_limits<unsigned char>::max() + 1;

// Original: CSpriteFrame::CSpriteFrame; cspriteframe.cpp:67
DC_ADDRESS(0x074600, 0x64)
CSpriteFrame::CSpriteFrame()
    : resource(0, RESOURCE_TYPE_NONE),
      DataSize(0), ImageSize(0), EncodingMethod(eEncodeRaw),
      Width(0), Height(0), CroppedWidth(0), CroppedHeight(0),
      CroppedX(0), CroppedY(0), Pitch(0), map(0)
{
}

VA(0x0047c2b0, 0xa7)
DC_ADDRESS(0x074664, 0xaa)
MAC_ADDRESS(0x08b24c, 0xc8)
CSpriteFrame::CSpriteFrame(const char* name, int w, int h,
                           unsigned char* data, int csize,
                           TEncodingMethod encoding)
    : resource(name, RESOURCE_TYPE_SPRITE),
      ImageSize(w * h), EncodingMethod(encoding), Width(w), Height(h),
      CroppedWidth(w), CroppedHeight(h), CroppedX(0), CroppedY(0), Pitch(w)
{
    DataSize = csize ? csize : ImageSize;
    map = new unsigned char[DataSize];
    if (map)
        memcpy(map, data, DataSize);
}

VA(0x0047c360, 0xc9)
DC_ADDRESS(0x074710, 0xba)
MAC_ADDRESS(0x08b314, 0xd4)
CSpriteFrame::CSpriteFrame(const char* name, int w, int h,
                           unsigned char* data, int csize,
                           TEncodingMethod encoding,
                           int cw, int ch, int cx, int cy)
    : resource(name, RESOURCE_TYPE_SPRITE),
      ImageSize(cw * ch), EncodingMethod(encoding), Width(w), Height(h),
      CroppedWidth(cw), CroppedHeight(ch), CroppedX(cx), CroppedY(cy),
      Pitch(cw)
{
    DataSize = csize ? csize : ImageSize;
    map = new unsigned char[DataSize];
    if (map)
        memcpy(map, data, DataSize);
}

// Original: CSpriteFrame::CSpriteFrame; cspriteframe.cpp:188
DC_ADDRESS(0x0747cc, 0x6e)
CSpriteFrame::CSpriteFrame(const char* name, unsigned char cropped)
    : resource(name, RESOURCE_TYPE_SPRITE),
      DataSize(0), ImageSize(0), EncodingMethod(eEncodeRaw),
      Width(0), Height(0), CroppedWidth(0), CroppedHeight(0),
      CroppedX(0), CroppedY(0), Pitch(0), map(0)
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

VA(0x0047c430, 0x22)
DC_ADDRESS(0x07483c, 0x6e)
MAC_ADDRESS(0x08b3e8, 0x70)  // vtable 0x63d6bc + resource::~resource
CSpriteFrame::~CSpriteFrame()
{
    if (map)
        delete[] map;
}

// Original: CSpriteFrame::clear; cspriteframe.cpp:217
// Complete removes the DC DirectDraw surface tail. As in its retained
// destructor, the remaining map is always an owned byte allocation.
DC_ADDRESS(0x0748ac, 0x6a)
void CSpriteFrame::clear()
{
    Width = 0;
    Height = 0;
    CroppedWidth = 0;
    CroppedHeight = 0;
    CroppedX = 0;
    CroppedY = 0;
    Pitch = 0;
    DataSize = 0;
    ImageSize = 0;
    EncodingMethod = eEncodeRaw;
    if (map) {
        delete[] map;
        map = 0;
    }
}

VA(0x0047c460, 0xF7)
DC_ADDRESS(0x074918, 0xfe)
MAC_ADDRESS(0x08b458, 0x2ac)
void CSpriteFrame::SetPixelFormat(unsigned rmask, unsigned gmask,
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

    div2mask = ((((1 << rBits) - 1) / 2) << (gBits + bits))
        | ((((1 << gBits) - 1) / 2) << bits)
        | (((1 << bits) - 1) / 2);
    div4mask = ((((1 << rBits) - 1) / 4) << (gBits + bits))
        | ((((1 << gBits) - 1) / 4) << bits)
        | (((1 << bits) - 1) / 4);
}

// Original: CSpriteFrame::importPCXFile; cspriteframe.cpp:283
// The ordinary file importer shares Complete's retained Victor PCX ABI with
// Bitmap816::importPCXFile. DC's DirectDraw surface descriptor is absent from
// the Complete frame layout; its owned byte buffer remains.
DC_ADDRESS(0x074a18, 0xdc)
int CSpriteFrame::importPCXFile(const char* filename)
{
    PcxData pdat;
    imgdes pcxfile;
    int error = pcxinfo(filename, &pdat);
    if (error)
        return 1;

    Width = pdat.m_width;
    Height = pdat.m_length;
    Pitch = pdat.m_width;
    DataSize = ImageSize = Width * Height;
    CroppedX = 0;
    CroppedY = 0;
    CroppedWidth = Width;
    CroppedHeight = Height;
    map = new unsigned char[DataSize];
    if (!map)
        return 2;

    allocimage(&pcxfile, pdat.m_width, pdat.m_length,
               pdat.m_bpPixel * pdat.m_nplanes);
    loadpcx(filename, &pcxfile);
    flipimage(&pcxfile, &pcxfile);
    for (int y = 0; y < Height; ++y)
        memcpy(map + y * Width,
               pcxfile.m_ibuff + y * pcxfile.m_buffwidth, Width);
    freeimage(&pcxfile);
    return 0;
}

// Original: CSpriteFrame::importCroppedPCXFile; cspriteframe.cpp:368
DC_ADDRESS(0x074af4, 0x26a)
int CSpriteFrame::importCroppedPCXFile(const char* filename)
{
    PcxData pdat;
    imgdes pcxfile;
    int error = pcxinfo(filename, &pdat);
    if (error)
        return 1;

    Width = CroppedWidth = pdat.m_width;
    Height = CroppedHeight = pdat.m_length;
    Pitch = pdat.m_width;
    DataSize = ImageSize = Width * Height;
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
    for (x = 0; x < Width; ++x) {
        for (y = 0; y < Height; ++y) {
            if (pcxfile.m_ibuff[y * pcxfile.m_buffwidth + x]) {
                leftoff = x;
                break;
            }
        }
        if (leftoff >= 0)
            break;
    }
    for (x = 0; x < Width; ++x) {
        for (y = 0; y < Height; ++y) {
            if (pcxfile.m_ibuff[y * pcxfile.m_buffwidth + Width - x - 1]) {
                rightoff = x;
                break;
            }
        }
        if (rightoff >= 0)
            break;
    }
    for (y = 0; y < Height; ++y) {
        for (x = 0; x < Width; ++x) {
            if (pcxfile.m_ibuff[y * pcxfile.m_buffwidth + x]) {
                topoff = y;
                break;
            }
        }
        if (topoff >= 0)
            break;
    }
    for (y = 0; y < Height; ++y) {
        for (x = 0; x < Width; ++x) {
            if (pcxfile.m_ibuff[(Height - y - 1) * pcxfile.m_buffwidth + x]) {
                bottomoff = y;
                break;
            }
        }
        if (bottomoff >= 0)
            break;
    }
    if (leftoff >= 0) {
        CroppedX += leftoff;
        CroppedWidth -= leftoff;
    }
    if (rightoff >= 0)
        CroppedWidth -= rightoff;
    if (topoff >= 0) {
        CroppedY += topoff;
        CroppedHeight -= topoff;
    }
    if (bottomoff >= 0)
        CroppedHeight -= bottomoff;
    DataSize = CroppedWidth * CroppedHeight;
    map = new unsigned char[DataSize];
    if (!map)
        return 2;

    unsigned char* dest = map;
    unsigned char* source = pcxfile.m_ibuff + topoff * pcxfile.m_buffwidth + leftoff;
    for (y = 0; y < CroppedHeight; ++y) {
        memcpy(dest, source, CroppedWidth);
        dest += CroppedWidth;
        source += pcxfile.m_buffwidth;
    }
    // DC retains the original PCX width as Pitch even after packing the crop.
    Pitch = pdat.m_width;
    freeimage(&pcxfile);
    return 0;
}

// Original: CSpriteFrame::GetPixel; cspriteframe.cpp:542
// Row directories use the same dword/word formats as the retained renderers.
DC_ADDRESS(0x074d60, 0x16a)
unsigned char CSpriteFrame::GetPixel(int x, int y) const
{
    x -= CroppedX;
    if (x < 0 || x >= CroppedWidth)
        return 0;
    y -= CroppedY;
    if (y < 0 || y >= CroppedHeight)
        return 0;

    unsigned char pixel;
    switch (EncodingMethod) {
    case eEncodeRaw:
        pixel = map[y * Pitch + x];
        break;
    case eEncodeGeneralRLE: {
        const unsigned int* lineOffsets =
            static_cast<const unsigned int*>(static_cast<const void*>(map));
        const unsigned char* source = map + lineOffsets[y];
        unsigned int position = 0;
        while (1) {
            unsigned char code = *source++;
            unsigned int run = *source++ + 1;
            if (position + run > static_cast<unsigned int>(x)) {
                if (code == kGeneralRLEOpaqueRunCode)
                    pixel = source[x - position];
                else
                    pixel = code;
                break;
            }
            position += run;
            if (code == kGeneralRLEOpaqueRunCode)
                source += run;
        }
        break;
    }
    case eEncodeTilesetRLE: {
        const unsigned short* lineOffsets =
            static_cast<const unsigned short*>(static_cast<const void*>(map));
        const unsigned char* source = map + lineOffsets[y];
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
        unsigned int cellsPerRow = static_cast<unsigned int>(CroppedWidth) >> 5;
        const unsigned short* cellOffsets =
            static_cast<const unsigned short*>(static_cast<const void*>(map));
        const unsigned char* source = map + cellOffsets[y * cellsPerRow + (x >> 5)];
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

// Original: CSpriteFrame::Crop; cspriteframe.cpp:645
// This editing operation accepts the raw map and retains the full-image stride.
DC_ADDRESS(0x074ecc, 0x1c6)
int CSpriteFrame::Crop()
{
    int x;
    int y;
    int leftoff = -1;
    int rightoff = -1;
    int topoff = -1;
    int bottomoff = -1;
    for (x = 0; x < Width; ++x) {
        for (y = 0; y < Height; ++y) {
            if (map[y * Width + x]) {
                leftoff = x;
                break;
            }
        }
        if (leftoff >= 0)
            break;
    }
    for (x = 0; x < Width; ++x) {
        for (y = 0; y < Height; ++y) {
            if (map[y * Width + Width - x - 1]) {
                rightoff = x;
                break;
            }
        }
        if (rightoff >= 0)
            break;
    }
    for (y = 0; y < Height; ++y) {
        for (x = 0; x < Width; ++x) {
            if (map[y * Width + x]) {
                topoff = y;
                break;
            }
        }
        if (topoff >= 0)
            break;
    }
    for (y = 0; y < Height; ++y) {
        for (x = 0; x < Width; ++x) {
            if (map[(Height - y - 1) * Width + x]) {
                bottomoff = y;
                break;
            }
        }
        if (bottomoff >= 0)
            break;
    }
    if (leftoff >= 0) {
        CroppedX += leftoff;
        CroppedWidth -= leftoff;
    }
    if (rightoff >= 0)
        CroppedWidth -= rightoff;
    if (topoff >= 0) {
        CroppedY += topoff;
        CroppedHeight -= topoff;
    }
    if (bottomoff >= 0)
        CroppedHeight -= bottomoff;
    DataSize = CroppedWidth * CroppedHeight;
    unsigned char* newMap = new unsigned char[DataSize];
    if (!newMap)
        return 2;
    unsigned char* dest = newMap;
    unsigned char* source = map + topoff * Width + leftoff;
    for (y = 0; y < CroppedHeight; ++y) {
        memcpy(dest, source, CroppedWidth);
        dest += CroppedWidth;
        source += Width;
    }
    Pitch = Width;
    delete[] map;
    map = newMap;
    return 0;
}

// Original: CSpriteFrame::Encode; cspriteframe.cpp:768
DC_ADDRESS(0x075094, 0x44)
void CSpriteFrame::Encode(TEncodingMethod method)
{
    switch (method) {
    case eEncodeGeneralRLE: EncodeGeneral(); break;
    case eEncodeTilesetRLE: EncodeTileset(); break;
    case eEncodeAdvObjRLE: EncodeAdvObj(); break;
    }
}

// Original: CSpriteFrame::EncodeGeneral; cspriteframe.cpp:791
DC_ADDRESS(0x0750d8, 0x1b2)
void CSpriteFrame::EncodeGeneral()
{
    // Both copies are dynamically initialized from the file statics. DC
    // CodeView places EncodeGeneral's kOpaqueRunCode and kMaxRunLength in
    // its .bss segment beside Draw's, and Loki's h3maped guards and copies
    // both at entry. Retail does not link this function, but its statics stay
    // in the compiland's .bss: they fill retail's unreferenced bytes around
    // the blend masks and so place div4mask at 2 mod 4, as retail has it.
    static const unsigned char kOpaqueRunCode = kGeneralRLEOpaqueRunCode;
    static const unsigned int kMaxRunLength = kGeneralRLEMaxRunLength;
    unsigned int newDataSize = CroppedHeight * sizeof(unsigned int);
    unsigned int linesToGo = CroppedHeight;
    unsigned char* source = map;
    do {
        unsigned char code = source[0] < 10 ? source[0] : kOpaqueRunCode;
        newDataSize += 2;
        unsigned int run = 0;
        int x = 1;
        bool opaque = code == kOpaqueRunCode;
        while (1) {
            ++run;
            if (opaque)
                ++newDataSize;
            if (x >= CroppedWidth)
                break;
            unsigned char nextCode = source[x] < 10 ? source[x] : kOpaqueRunCode;
            if (nextCode != code || run == kMaxRunLength) {
                newDataSize += 2;
                code = nextCode;
                run = 0;
                opaque = code == kOpaqueRunCode;
            }
            ++x;
        }
        source += CroppedWidth;
    } while (--linesToGo);

    unsigned char* newMap = new unsigned char[newDataSize];
    unsigned int* lineOffset = static_cast<unsigned int*>(static_cast<void*>(newMap));
    linesToGo = CroppedHeight;
    unsigned int offset = CroppedHeight * sizeof(unsigned int);
    source = map;
    do {
        *lineOffset++ = offset;
        unsigned char code = source[0] < 10 ? source[0] : kOpaqueRunCode;
        newMap[offset++] = code;
        unsigned char* runLength = newMap + offset++;
        unsigned int run = 0;
        int x = 1;
        while (1) {
            ++run;
            if (code == kOpaqueRunCode)
                newMap[offset++] = source[x - 1];
            if (x >= CroppedWidth)
                break;
            unsigned char nextCode = source[x] < 10 ? source[x] : kOpaqueRunCode;
            if (nextCode != code || run == kMaxRunLength) {
                *runLength = static_cast<unsigned char>(run - 1);
                code = nextCode;
                newMap[offset++] = code;
                runLength = newMap + offset++;
                run = 0;
            }
            ++x;
        }
        *runLength = static_cast<unsigned char>(run - 1);
        source += CroppedWidth;
    } while (--linesToGo);
    delete[] map;
    map = newMap;
    DataSize = newDataSize;
    EncodingMethod = eEncodeGeneralRLE;
}

// Original: CSpriteFrame::EncodeTileset; cspriteframe.cpp:894
DC_ADDRESS(0x07528c, 0x1b2)
void CSpriteFrame::EncodeTileset()
{
    bool hasControlPixels = false;
    for (int i = 0; i < DataSize; ++i) {
        if (map[i] < 5) {
            hasControlPixels = true;
            break;
        }
    }
    if (hasControlPixels) {
        unsigned int linesToGo = CroppedHeight;
        unsigned int newDataSize = CroppedHeight * sizeof(unsigned short);
        unsigned char* source = map;
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
                if (x >= CroppedWidth)
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
            source += CroppedWidth;
        } while (--linesToGo);

        unsigned char* newMap = new unsigned char[newDataSize];
        unsigned short* lineOffset = static_cast<unsigned short*>(static_cast<void*>(newMap));
        linesToGo = CroppedHeight;
        unsigned short offset = static_cast<unsigned short>(CroppedHeight * sizeof(unsigned short));
        source = map;
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
                if (x >= CroppedWidth)
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
            source += CroppedWidth;
        } while (--linesToGo);
        delete[] map;
        map = newMap;
        DataSize = newDataSize;
        EncodingMethod = eEncodeTilesetRLE;
    }
}

// Original: CSpriteFrame::EncodeAdvObj; cspriteframe.cpp:1012
DC_ADDRESS(0x075440, 0x3d0)
void CSpriteFrame::EncodeAdvObj()
{
    int oldWidth = CroppedWidth;
    int right = CroppedX + oldWidth;
    int newCroppedX = CroppedX - (CroppedX & 31);
    int newRight = right + 31 - ((right + 31) & 31);
    unsigned int newCroppedWidth = newRight - newCroppedX;
    unsigned int addLeft = CroppedX - newCroppedX;
    unsigned int addRight = newRight - right;
    unsigned int cellsPerLine = newCroppedWidth >> 5;
    unsigned int newDataSize = 2 * cellsPerLine * CroppedHeight;
    unsigned int linesToGo = CroppedHeight;
    unsigned char* source = map;
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
    unsigned short offset = static_cast<unsigned short>(2 * cellsPerLine * CroppedHeight);
    source = map;
    linesToGo = CroppedHeight;
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
    map = newMap;
    DataSize = newDataSize;
    EncodingMethod = eEncodeAdvObjRLE;
    CroppedX = newCroppedX;
    CroppedWidth = newCroppedWidth;
}

VA(0x0047c560, 0x07)
MAC_ADDRESS(0x08b704, 0xc)  // vtable slot 2: fixed object extent + owned bytes
unsigned int CSpriteFrame::getSize() const
{
    return sizeof(*this) + DataSize;
}

// Loki's GCC 2.95 copies (which keep association order) flip with
// `width - (x + w)` and form the cropped limits as origin plus extent;
// VC6 emits the same bytes for either spelling.
DC_ADDRESS(0x079294, 0x184)
inline void CSpriteFrame::Clip(int& sx, int& sy, int& sw, int& sh,
                               int& dx, int& dy, int dw, int dh,
                               bool hflip,
                               bool vflip) const
{
    int deltaX;

    if (hflip)
        sx = Width - (sx + sw);
    if (vflip)
        sy = Height - (sy + sh);

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

    if (sx < CroppedX) {
        deltaX = CroppedX - sx;
        if (!hflip)
            dx += deltaX;
        sw -= deltaX;
        sx = CroppedX;
    }
    if (sy < CroppedY) {
        deltaX = CroppedY - sy;
        if (!vflip)
            dy += deltaX;
        sh -= deltaX;
        sy = CroppedY;
    }
    deltaX = CroppedX + CroppedWidth;
    if (sw + sx > deltaX) {
        if (hflip)
            dx += sw + sx - deltaX;
        sw = deltaX - sx;
    }
    deltaX = CroppedY + CroppedHeight;
    if (sh + sy > deltaX) {
        if (vflip)
            dy += sh + sy - deltaX;
        sh = deltaX - sy;
    }

    sx -= CroppedX;
    sy -= CroppedY;
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
VA(0x0047c570, 0x465)
DC_ADDRESS(0x075810, 0x310)
MAC_ADDRESS(0x08b710, 0x430)  // unique PC/DC renderer identity; retail byte verdict
void CSpriteFrame::Draw(int sx, int sy, int sw, int sh,
                        unsigned short* dst, int dx, int dy, int dw, int dh,
                        int dpitch, TPalette16& pal, bool hflip,
                        bool tblit) const
{
    if (EncodingMethod == eEncodeTilesetRLE || EncodingMethod == eEncodeRaw) {
        DrawTile(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                 pal, hflip, 0);
        return;
    }
    else if (EncodingMethod == eEncodeAdvObjRLE) {
        DrawAdvObjImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                       pal, hflip, 0);
        return;
    }

    const unsigned int* lineOffset;
    // Retail construction guard 0x6968b7; the copied run code at 0x69689c.
    DATA_COMPGEN_GUARD(0x006968b7, drawOpaqueRunCodeGuard, kOpaqueRunCode)
    DATA(0x0069689c)
    static const unsigned char kOpaqueRunCode = kGeneralRLEOpaqueRunCode;
    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, 0);

    if (sw > 0 && sh > 0) {
        lineOffset =
            static_cast<const unsigned int*>(static_cast<const void*>(map));
        if (!hflip) {
            unsigned short* lineDst =
                static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + dy * dpitch + dx * 2));
            for (int y = sy; y < sy + sh; ++y) {
                unsigned short* out = lineDst;
                unsigned int skipped = 0;
                const unsigned char* src = map + lineOffset[y];
                unsigned char code = *src++;
                unsigned int run = *src++ + 1;

                while (skipped + run <= static_cast<unsigned int>(sx)) {
                    skipped += run;
                    if (code == kOpaqueRunCode)
                        src += run;
                    code = *src++;
                    run = *src++ + 1;
                }

                run += skipped - sx;
                if (code == kOpaqueRunCode)
                    src += sx - skipped;

                unsigned int remaining = sw;
                do {
                    if (run > remaining)
                        run = remaining;
                    if (code == kOpaqueRunCode) {
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
                const unsigned char* src = map + lineOffset[y];
                unsigned char code = *src++;
                unsigned int run = *src++ + 1;

                while (skipped + run <= static_cast<unsigned int>(sx)) {
                    skipped += run;
                    if (code == kOpaqueRunCode)
                        src += run;
                    code = *src++;
                    run = *src++ + 1;
                }

                run += skipped - sx;
                if (code == kOpaqueRunCode)
                    src += sx - skipped;

                unsigned int remaining = sw;
                do {
                    if (run > remaining)
                        run = remaining;
                    if (code == kOpaqueRunCode) {
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

// Exact. Loki's h3maped
// (GCC -O0) supplies the shape that closed the rest together: m_map and the
// palette load inside the positive-extent guard, a row cursor `lineDst`
// separate from the `dst` parameter, the source row read before `skipped`,
// and the reverse half shade reading through the decremented cursor without
// a widening local. Measured singly in older TU states each of these lost
// (93.52%, 94.73%, 95.33%); together they take 95.85 -> 99.95. CodeView also
// proves the function-scope `TOffset`/`TDstPixel` identities and that
// `aLineOffset` precedes the const `kOpaqueRunCode`. The last four bytes,
// the three-quarter blends' word AND of div4mask, came from EncodeGeneral's
// missing .bss static (99.95 -> 100).
VA(0x0047c9e0, 0x6BC)
DC_ADDRESS(0x075b20, 0x540)
MAC_ADDRESS(0x08bb40, 0x6f4)  // unique PC/DC renderer identity; retail byte verdict
void CSpriteFrame::DrawCreatureImpl(int sx, int sy, int sw, int sh,
                                    unsigned short* dst, int dx, int dy,
                                    int dw, int dh, int dpitch,
                                    TPalette16& pal, bool hflip,
                                    unsigned short outcolor,
                                    bool alpha) const
{
    typedef unsigned int TOffset;
    typedef unsigned short TDstPixel;

    if (!alpha) {
        if (EncodingMethod == eEncodeTilesetRLE ||
            EncodingMethod == eEncodeRaw) {
            DrawTile(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                     pal, hflip, 0);
            return;
        }
        if (EncodingMethod == eEncodeAdvObjRLE) {
            DrawAdvObjImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                           pal, hflip, outcolor);
            return;
        }
    }

    const TOffset* lineOffset;
    const unsigned short* palette;
    // Retail construction guard 0x6968b4; the copied run code at 0x6968b5.
    DATA_COMPGEN_GUARD(0x006968b4, creatureOpaqueRunCodeGuard, kOpaqueRunCode)
    DATA(0x006968b5)
    static const unsigned char kOpaqueRunCode = kGeneralRLEOpaqueRunCode;
    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, 0);

    if (sw > 0 && sh > 0) {
        lineOffset =
            static_cast<const TOffset*>(static_cast<const void*>(map));
        palette = pal.m_data;
        if (!hflip) {
            unsigned short* lineDst =
                static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + dy * dpitch + dx * 2));

            for (int y = sy; y < sy + sh; ++y) {
                unsigned short* out = lineDst;
                const unsigned char* src = map + lineOffset[y];
                unsigned int skipped = 0;
                unsigned char code = *src++;
                unsigned int run = *src++ + 1;

                while (skipped + run <= static_cast<unsigned int>(sx)) {
                    skipped += run;
                    if (code == kOpaqueRunCode)
                        src += run;
                    code = *src++;
                    run = *src++ + 1;
                }

                run += skipped - sx;
                if (code == kOpaqueRunCode)
                    src += sx - skipped;

                unsigned int remaining = sw;
                do {
                    if (run > remaining)
                        run = remaining;
                    if (code == kOpaqueRunCode) {
                        unsigned int count = run;
                        if (!alpha) {
                            do {
                                *out++ = palette[*src++];
                            } while (--count);
                        } else {
                            do {
                                *out = (div2mask
                                        & (palette[*src++] >> 1))
                                     + (div2mask & (*out >> 1));
                                ++out;
                            } while (--count);
                        }
                    } else if (!outcolor) {
                        switch (code) {
                        case eRleControlShadow75:
                        case eRleControlOutline7: {
                            unsigned int count = run;
                            do {
                                *out = ((*out >> 1) & div2mask)
                                     + ((*out >> 2) & div4mask);
                                ++out;
                            } while (--count);
                            break;
                        }
                        case eRleControlShadow50:
                        case eRleControlOutline6: {
                            unsigned int count = run;
                            do {
                                *out = (*out >> 1) & div2mask;
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
                                *out = ((*out >> 1) & div2mask)
                                     + ((*out >> 2) & div4mask);
                                ++out;
                            } while (--count);
                            break;
                        }
                        case eRleControlShadow50: {
                            unsigned int count = run;
                            do {
                                *out = (*out >> 1) & div2mask;
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

                lineDst = static_cast<unsigned short*>(static_cast<void*>(
                    static_cast<unsigned char*>(static_cast<void*>(lineDst)) + dpitch));
            }
        } else {
            unsigned short* lineDst =
                static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + dy * dpitch + (dx + sw) * 2));

            for (int y = sy; y < sy + sh; ++y) {
                unsigned short* out = lineDst;
                const unsigned char* src = map + lineOffset[y];
                unsigned int skipped = 0;
                unsigned char code = *src++;
                unsigned int run = *src++ + 1;

                while (skipped + run <= static_cast<unsigned int>(sx)) {
                    skipped += run;
                    if (code == kOpaqueRunCode)
                        src += run;
                    code = *src++;
                    run = *src++ + 1;
                }

                run += skipped - sx;
                if (code == kOpaqueRunCode)
                    src += sx - skipped;

                unsigned int remaining = sw;
                do {
                    if (run > remaining)
                        run = remaining;
                    if (code == kOpaqueRunCode) {
                        unsigned int count = run;
                        if (!alpha) {
                            do {
                                *--out = palette[*src++];
                            } while (--count);
                        } else {
                            do {
                                --out;
                                *out = (div2mask
                                        & (palette[*src++] >> 1))
                                     + (div2mask & (*out >> 1));
                            } while (--count);
                        }
                    } else if (!outcolor) {
                        switch (code) {
                        case eRleControlShadow75:
                        case eRleControlOutline7: {
                            unsigned int count = run;
                            do {
                                --out;
                                *out = ((*out >> 1) & div2mask)
                                     + ((*out >> 2) & div4mask);
                            } while (--count);
                            break;
                        }
                        case eRleControlShadow50:
                        case eRleControlOutline6: {
                            unsigned int count = run;
                            do {
                                --out;
                                *out = (*out >> 1) & div2mask;
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
                                *out = ((*out >> 1) & div2mask)
                                     + ((*out >> 2) & div4mask);
                            } while (--count);
                            break;
                        }
                        case eRleControlShadow50: {
                            unsigned int count = run;
                            do {
                                --out;
                                *out = (*out >> 1) & div2mask;
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

                lineDst = static_cast<unsigned short*>(static_cast<void*>(
                    static_cast<unsigned char*>(static_cast<void*>(lineDst)) + dpitch));
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
VA(0x0047d0a0, 0x44B)
DC_ADDRESS(0x076060, 0x324)
MAC_ADDRESS(0x08c234, 0x460) // retail packed-cell decoder + DC source identity
void CSpriteFrame::DrawAdvObjImpl(int sx, int sy, int sw, int sh,
                                  unsigned short* dst, int dx, int dy, int dw,
                                  int dh, int dpitch, TPalette16& pal,
                                  bool hflip,
                                  unsigned short flagcolor) const
{
    const unsigned short* palette;
    unsigned int cellsPerLine;
    const unsigned short* cellOffset;

    if (EncodingMethod == eEncodeGeneralRLE) {
        // Retail passes sw in the first source-coordinate slot at 0x47d0e6.
        DrawCreature(sw, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip,
                     flagcolor);
        return;
    }
    if (EncodingMethod == eEncodeTilesetRLE || EncodingMethod == eEncodeRaw) {
        DrawTile(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, 0);
        return;
    }

    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, 0);
    if (sw > 0) {
        if (sh > 0) {

            cellsPerLine = static_cast<unsigned int>(CroppedWidth) >> 5;
            cellOffset = static_cast<const unsigned short*>(static_cast<const void*>(map));
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
                        map + cellOffset[y * cellsPerLine +
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
                        map + cellOffset[y * cellsPerLine +
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

// Loki's h3maped (GCC -O0) computes each row's cell source before the
// skipped-pixel base `sx & ~31`; that statement order closed the row-setup
// register transposition (98.00 -> 100). Its flag blend also evaluates the
// flag term before the destination term.
// DC records palette and aCellOffset as read-only pointers; lines
// 2462/2463 load the map table and bind pal+0x1c before either row loop.
// A single native row cursor restores 98.0000%; a fixed base plus row offset
// leaves 94.5249%. The decoder and palette helpers remain unchanged.
VA(0x0047d4f0, 0x43C)
DC_ADDRESS(0x076384, 0x302)
MAC_ADDRESS(0x08c694, 0x450)  // anchor-caller (DrawSpellEffect 0x47efca) + DC source identity
void CSpriteFrame::DrawAdvObjWithFlagAlpha(int sx, int sy, int sw, int sh,
                                           unsigned short* dst, int dx, int dy,
                                           int dw, int dh, int dpitch,
                                           TPalette16& pal,
                                           unsigned short flagcolor,
                                           bool hflip) const
{
    const unsigned short* palette;
    unsigned int cellsPerLine;
    const unsigned short* cellOffset;

    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, 0);
    if (sw > 0) {
        if (sh > 0) {

            cellsPerLine = static_cast<unsigned int>(CroppedWidth) >> 5;
            cellOffset = static_cast<const unsigned short*>(static_cast<const void*>(map));

            palette = pal.m_data;

            if (!hflip) {
                unsigned short* lineDst =
                    static_cast<unsigned short*>(static_cast<void*>(
                    static_cast<unsigned char*>(static_cast<void*>(dst)) +
                    dy * dpitch + dx * 2));

                for (int y = sy; y < sy + sh; ++y) {
                    unsigned short* out = lineDst;
                    const unsigned char* src =
                        map + cellOffset[y * cellsPerLine +
                                          (static_cast<unsigned int>(sx) >> 5)];
                    unsigned int skipped = static_cast<unsigned int>(sx) & ~31U;
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
                                *out = (div2mask
                                        & (palette[*src++] >> 1))
                                     + (div2mask & (*out >> 1));
                                ++out;
                            } while (--count);
                        } else if (code == eRleControlOutline5 && flagcolor) {
                            unsigned int count = run;
                            do {
                                *out = (div2mask & (flagcolor >> 1))
                                     + (div2mask & (*out >> 1));
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

                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst)) + dpitch));
                }
            } else {
                unsigned short* lineDst =
                    static_cast<unsigned short*>(static_cast<void*>(
                    static_cast<unsigned char*>(static_cast<void*>(dst)) +
                    dy * dpitch + (dx + sw) * 2));

                for (int y = sy; y < sy + sh; ++y) {
                    unsigned short* out = lineDst;
                    const unsigned char* src =
                        map + cellOffset[y * cellsPerLine +
                                          (static_cast<unsigned int>(sx) >> 5)];
                    unsigned int skipped = static_cast<unsigned int>(sx) & ~31U;
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
                                *out = (div2mask
                                        & (palette[*src++] >> 1))
                                     + (div2mask & (*out >> 1));
                            } while (--count);
                        } else if (code == eRleControlOutline5 && flagcolor) {
                            unsigned int count = run;
                            do {
                                --out;
                                *out = (div2mask & (flagcolor >> 1))
                                     + (div2mask & (*out >> 1));
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

                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst)) + dpitch));
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

// Loki's h3maped (GCC -O0) reads the row's cell source before its
// skipped-pixel base and evaluates the three-quarter blend's div2mask term
// first; the latter took 99.92 -> 99.98. The blend's word AND of div4mask
// then came from EncodeGeneral's missing .bss static (-> 100), as
// DrawTileShadow below records. The half blend DID have a
// source cause: the widening `unsigned int color =
// out[-1]` spelling cost nine flow-kind blocks and a whole missing block, and
// dropping it took this row 98.5765 -> 99.9400 on one line.
// The native row cursor restores 99.9404%; an extra base/offset pair changes
// the surrounding lifetimes and leaves 94.3325%.
VA(0x0047d930, 0x40F)
DC_ADDRESS(0x076688, 0x2fe)
MAC_ADDRESS(0x08cae4, 0x440)  // anchor-callee (CSprite::DrawAdvObjShadow/DrawHeroShadow) + DC source identity
void CSpriteFrame::DrawAdvObjShadowImpl(int sx, int sy, int sw, int sh,
                                        unsigned short* dst, int dx, int dy,
                                        int dw, int dh, int dpitch,
                                        TPalette16& pal,
                                        bool hflip) const
{
    unsigned int cellsPerLine;
    const unsigned short* cellOffset;

    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, 0);
    if (sw > 0) {
        if (sh > 0) {

            cellsPerLine = static_cast<unsigned int>(CroppedWidth) >> 5;
            cellOffset = static_cast<const unsigned short*>(static_cast<const void*>(map));

            if (!hflip) {
                unsigned short* lineDst =
                    static_cast<unsigned short*>(static_cast<void*>(
                    static_cast<unsigned char*>(static_cast<void*>(dst)) +
                    dy * dpitch + dx * 2));

                for (int y = sy; y < sy + sh; ++y) {
                    unsigned short* out = lineDst;
                    const unsigned char* src =
                        map + cellOffset[y * cellsPerLine +
                                          (static_cast<unsigned int>(sx) >> 5)];
                    unsigned int skipped = static_cast<unsigned int>(sx) & ~31U;
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
                                    *out = ((*out >> 1) & div2mask)
                                         + ((*out >> 2) & div4mask);
                                    ++out;
                                } while (--count);
                                break;
                            }
                            case eRleControlShadow50: {
                                unsigned int count = run;
                                do {
                                    unsigned int color = *out;
                                    *out = (color >> 1) & div2mask;
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

                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst)) + dpitch));
                }
            } else {
                unsigned short* lineDst =
                    static_cast<unsigned short*>(static_cast<void*>(
                    static_cast<unsigned char*>(static_cast<void*>(dst)) +
                    dy * dpitch + (dx + sw) * 2));

                for (int y = sy; y < sy + sh; ++y) {
                    unsigned short* out = lineDst;
                    const unsigned char* src =
                        map + cellOffset[y * cellsPerLine +
                                          (static_cast<unsigned int>(sx) >> 5)];
                    unsigned int skipped = static_cast<unsigned int>(sx) & ~31U;
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
                                    *out = ((*out >> 1) & div2mask)
                                         + ((*out >> 2) & div4mask);
                                } while (--count);
                                break;
                            }
                            case eRleControlShadow50: {
                                unsigned int count = run;
                                do {
                                    --out;
                                    *out = (*out >> 1) & div2mask;
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

                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst)) + dpitch));
                }
            }
        }
    }
}

// E:\gamedcs\cspriteframe.cpp:2856.  Raw tiles are palette-indexed rows;
// encoded tiles use a word row-offset table and the same packed packet byte as
// adventure cells.  Only code seven carries pixels in this renderer.

// Four DC/retail direction arms, the eight-statement Duff loop, raw do/while
// rows and indexed encoded for-rows recover all behavior. Retail and
// Dreamcast agree on the surprising general-RLE delegation
// `Draw(sw, sy, sw, ...)`. Loki's GCC 2.95 build (no auto-inlining or
// cross-branch hoisting) loads `pal.m_data` once, right after the line-table
// pointer and before the flip dispatch, as the sibling decoders' `palette`
// local does; restoring that local took retail from 81.42% to 98.27%. DC
// attributes both raw-row advances to one line (2938); advancing the
// destination row before the source row reproduces retail exactly.
VA(0x0047dd40, 0xAD8)
DC_ADDRESS(0x076988, 0x762)
MAC_ADDRESS(0x08cf24, 0x834) // retail raw/tileset decoder + DC source identity
void CSpriteFrame::DrawTile(int sx, int sy, int sw, int sh, unsigned short* dst,
                            int dx, int dy, int dw, int dh, int dpitch,
                            TPalette16& pal, bool hflip,
                            bool vflip) const
{
    static const unsigned char kOpaqueRunCode = 7;

    if (EncodingMethod == eEncodeGeneralRLE) {
        Draw(sw, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, 1);
        return;
    }
    if (EncodingMethod == eEncodeAdvObjRLE) {
        DrawAdvObjImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip,
                       0);
        return;
    }

    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, vflip);
    if (sw > 0) {
        if (sh > 0) {
            const unsigned short* const lineOffset = static_cast<const unsigned short*>(
                static_cast<const void*>(map));
            const unsigned short* const palette = pal.m_data;
            if (!vflip) {
                if (!hflip) {
                    unsigned short* lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(dst)) +
                        dy * dpitch + dx * 2));

                    if (EncodingMethod == eEncodeRaw) {
                        const unsigned char* line = map + sy * Pitch + sx;
                        do {
                            int remaining = sw;
                            unsigned short* out = lineDst;
                            const unsigned char* src = line;
                            switch (remaining & 7) {
                            case eRawRowUnroll8:
                                do {
                                    *out++ = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll7:
                                    *out++ = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll6:
                                    *out++ = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll5:
                                    *out++ = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll4:
                                    *out++ = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll3:
                                    *out++ = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll2:
                                    *out++ = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll1:
                                    *out++ = palette[*src++];
                                    --remaining;
                                } while (remaining > 0);
                            }
                            lineDst = static_cast<unsigned short*>(static_cast<void*>(
                                static_cast<unsigned char*>(static_cast<void*>(lineDst)) + dpitch));
                            line += Pitch;
                        } while (--sh > 0);
                    } else {
                        for (int y = sy; y < sy + sh; ++y) {
                            unsigned short* out = lineDst;
                            const unsigned char* src = map + lineOffset[y];
                            unsigned int skipped = 0;
                            unsigned char packet = *src;
                            unsigned char code = packet >> 5;
                            unsigned int run = (packet & 31) + 1;
                            ++src;

                            while (skipped + run <=
                                   static_cast<unsigned int>(sx)) {
                                skipped += run;
                                if (code == kOpaqueRunCode)
                                    src += run;
                                packet = *src;
                                code = packet >> 5;
                                run = (packet & 31) + 1;
                                ++src;
                            }
                            run += skipped - sx;
                            if (code == kOpaqueRunCode)
                                src += sx - skipped;

                            unsigned int remaining = sw;
                            do {
                                if (run > remaining)
                                    run = remaining;
                                if (code == kOpaqueRunCode) {
                                    unsigned int count = run;
                                    do {
                                        *out++ = palette[*src++];
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
                            lineDst = static_cast<unsigned short*>(static_cast<void*>(
                                static_cast<unsigned char*>(static_cast<void*>(lineDst)) + dpitch));
                        }
                    }
                } else {
                    unsigned short* lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(dst)) +
                        dy * dpitch + (dx + sw) * 2));

                    if (EncodingMethod == eEncodeRaw) {
                        const unsigned char* line = map + sy * Pitch + sx;
                        do {
                            int remaining = sw;
                            unsigned short* out = lineDst;
                            const unsigned char* src = line;
                            switch (remaining & 7) {
                            case eRawRowUnroll8:
                                do {
                                    *--out = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll7:
                                    *--out = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll6:
                                    *--out = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll5:
                                    *--out = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll4:
                                    *--out = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll3:
                                    *--out = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll2:
                                    *--out = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll1:
                                    *--out = palette[*src++];
                                    --remaining;
                                } while (remaining > 0);
                            }
                            lineDst = static_cast<unsigned short*>(static_cast<void*>(
                                static_cast<unsigned char*>(static_cast<void*>(lineDst)) + dpitch));
                            line += Pitch;
                        } while (--sh > 0);
                    } else {
                        for (int y = sy; y < sy + sh; ++y) {
                            unsigned short* out = lineDst;
                            const unsigned char* src = map + lineOffset[y];
                            unsigned int skipped = 0;
                            unsigned char packet = *src;
                            unsigned char code = packet >> 5;
                            unsigned int run = (packet & 31) + 1;
                            ++src;

                            while (skipped + run <=
                                   static_cast<unsigned int>(sx)) {
                                skipped += run;
                                if (code == kOpaqueRunCode)
                                    src += run;
                                packet = *src;
                                code = packet >> 5;
                                run = (packet & 31) + 1;
                                ++src;
                            }
                            run += skipped - sx;
                            if (code == kOpaqueRunCode)
                                src += sx - skipped;

                            unsigned int remaining = sw;
                            do {
                                if (run > remaining)
                                    run = remaining;
                                if (code == kOpaqueRunCode) {
                                    unsigned int count = run;
                                    do {
                                        *--out = palette[*src++];
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
                            lineDst = static_cast<unsigned short*>(static_cast<void*>(
                                static_cast<unsigned char*>(static_cast<void*>(lineDst)) + dpitch));
                        }
                    }
                }
            } else {
                if (!hflip) {
                    unsigned short* lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(dst)) +
                        (dy + sh - 1) * dpitch + dx * 2));

                    if (EncodingMethod == eEncodeRaw) {
                        const unsigned char* line = map + sy * Pitch + sx;
                        do {
                            int remaining = sw;
                            unsigned short* out = lineDst;
                            const unsigned char* src = line;
                            switch (remaining & 7) {
                            case eRawRowUnroll8:
                                do {
                                    *out++ = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll7:
                                    *out++ = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll6:
                                    *out++ = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll5:
                                    *out++ = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll4:
                                    *out++ = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll3:
                                    *out++ = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll2:
                                    *out++ = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll1:
                                    *out++ = palette[*src++];
                                    --remaining;
                                } while (remaining > 0);
                            }
                            lineDst = static_cast<unsigned short*>(static_cast<void*>(
                                static_cast<unsigned char*>(static_cast<void*>(lineDst)) - dpitch));
                            line += Pitch;
                        } while (--sh > 0);
                    } else {
                        for (int y = sy; y < sy + sh; ++y) {
                            unsigned short* out = lineDst;
                            const unsigned char* src = map + lineOffset[y];
                            unsigned int skipped = 0;
                            unsigned char packet = *src;
                            unsigned char code = packet >> 5;
                            unsigned int run = (packet & 31) + 1;
                            ++src;

                            while (skipped + run <=
                                   static_cast<unsigned int>(sx)) {
                                skipped += run;
                                if (code == kOpaqueRunCode)
                                    src += run;
                                packet = *src;
                                code = packet >> 5;
                                run = (packet & 31) + 1;
                                ++src;
                            }
                            run += skipped - sx;
                            if (code == kOpaqueRunCode)
                                src += sx - skipped;

                            unsigned int remaining = sw;
                            do {
                                if (run > remaining)
                                    run = remaining;
                                if (code == kOpaqueRunCode) {
                                    unsigned int count = run;
                                    do {
                                        *out++ = palette[*src++];
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
                            lineDst = static_cast<unsigned short*>(static_cast<void*>(
                                static_cast<unsigned char*>(static_cast<void*>(lineDst)) - dpitch));
                        }
                    }
                } else {
                    unsigned short* lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(dst)) +
                        (dy + sh - 1) * dpitch + (dx + sw) * 2));

                    if (EncodingMethod == eEncodeRaw) {
                        const unsigned char* line = map + sy * Pitch + sx;
                        do {
                            int remaining = sw;
                            unsigned short* out = lineDst;
                            const unsigned char* src = line;
                            switch (remaining & 7) {
                            case eRawRowUnroll8:
                                do {
                                    *--out = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll7:
                                    *--out = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll6:
                                    *--out = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll5:
                                    *--out = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll4:
                                    *--out = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll3:
                                    *--out = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll2:
                                    *--out = palette[*src++];
                                    --remaining;
                                case eRawRowUnroll1:
                                    *--out = palette[*src++];
                                    --remaining;
                                } while (remaining > 0);
                            }
                            lineDst = static_cast<unsigned short*>(static_cast<void*>(
                                static_cast<unsigned char*>(static_cast<void*>(lineDst)) - dpitch));
                            line += Pitch;
                        } while (--sh > 0);
                    } else {
                        for (int y = sy; y < sy + sh; ++y) {
                            unsigned short* out = lineDst;
                            const unsigned char* src = map + lineOffset[y];
                            unsigned int skipped = 0;
                            unsigned char packet = *src;
                            unsigned char code = packet >> 5;
                            unsigned int run = (packet & 31) + 1;
                            ++src;

                            while (skipped + run <=
                                   static_cast<unsigned int>(sx)) {
                                skipped += run;
                                if (code == kOpaqueRunCode)
                                    src += run;
                                packet = *src;
                                code = packet >> 5;
                                run = (packet & 31) + 1;
                                ++src;
                            }
                            run += skipped - sx;
                            if (code == kOpaqueRunCode)
                                src += sx - skipped;

                            unsigned int remaining = sw;
                            do {
                                if (run > remaining)
                                    run = remaining;
                                if (code == kOpaqueRunCode) {
                                    unsigned int count = run;
                                    do {
                                        *--out = palette[*src++];
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
                            lineDst = static_cast<unsigned short*>(static_cast<void*>(
                                static_cast<unsigned char*>(static_cast<void*>(lineDst)) - dpitch));
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

// The single native row cursor preserves 99.9308%, all 156 block shapes,
// all 72 branches and all four returns. Splitting it into a fixed base and
// row offset leaves 92.0317%; an address union is byte-neutral and unnecessary.
//
// Loki's h3maped (GCC -O0) evaluates the three-quarter blend's div2mask term
// first; that order puts the div4mask term in the copy register as retail
// does (99.90 -> 99.97). Which term VC6 schedules first follows TU symbol
// state, so this was byte-flat in earlier TU states. The term's AND width
// is not this body either: VC6 widens a ushort global's AND to a dword only
// when the object places that global on a 4-byte boundary. Retail's div4mask
// sits at 0x6968aa (2 mod 4, `and bx,word[div4mask]`); ours did too once
// EncodeGeneral's dynamically initialized kMaxRunLength joined the .bss
// (99.97 -> 100).
// The half blend written
// `unsigned int color = out[-1]; --out; *out = (color >> 1) & mask;`
// emits a widening `xor eax,eax / mov ax,` pair retail does not have.
// `--out; *out = (*out >> 1) & mask;` reads the same location AFTER the
// decrement, which is what retail spells: 97.8963 -> 99.9300 here.
// DC decoder/helper scopes and pixel arithmetic remain intact, including
// reverse/raw source walks.
VA(0x0047e820, 0x740)
DC_ADDRESS(0x0770ec, 0x576)
MAC_ADDRESS(0x08d758, 0x7a0)  // anchor-callee (CSprite::DrawTileShadow/DrawShroudTile) + DC source identity
void CSpriteFrame::DrawTileShadow(int sx, int sy, int sw, int sh,
                                  unsigned short* dst, int dx, int dy, int dw,
                                  int dh, int dpitch, TPalette16& pal,
                                  bool hflip,
                                  bool vflip) const
{
    static const unsigned char kOpaqueRunCode = 7;

    if (EncodingMethod == eEncodeRaw)
        return;

    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, vflip);
    if (sw > 0) {
        if (sh > 0) {
            const unsigned short* lineOffset =
                static_cast<const unsigned short*>(
                    static_cast<const void*>(map));
            if (!vflip) {
                if (!hflip) {
                    unsigned short* lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(dst)) +
                        dy * dpitch + dx * 2));

                    for (int y = sy; y < sy + sh; ++y) {
                        unsigned short* out = lineDst;
                        const unsigned char* src = map + lineOffset[y];
                        unsigned int skipped = 0;
                        unsigned char packet = *src;
                        unsigned char code = packet >> 5;
                        unsigned int run = (packet & 31) + 1;
                        ++src;

                        while (skipped + run <=
                               static_cast<unsigned int>(sx)) {
                            skipped += run;
                            if (code == kOpaqueRunCode)
                                src += run;
                            packet = *src;
                            code = packet >> 5;
                            run = (packet & 31) + 1;
                            ++src;
                        }
                        run += skipped - sx;
                        if (code == kOpaqueRunCode)
                            src += sx - skipped;

                        unsigned int remaining = sw;
                        do {
                            if (run > remaining)
                                run = remaining;
                            if (code == kOpaqueRunCode) {
                                out += run;
                                src += run;
                            } else {
                                switch (code) {
                                case eRleControlShadow75:
                                case eRleControlShadow2: {
                                    unsigned int count = run;
                                    do {
                                        *out = ((*out >> 1) & div2mask)
                                             + ((*out >> 2) & div4mask);
                                        ++out;
                                    } while (--count);
                                    break;
                                }
                                case eRleControlShadow3:
                                case eRleControlShadow50: {
                                    unsigned int count = run;
                                    do {
                                        unsigned int color = *out;
                                        *out = (color >> 1) & div2mask;
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

                        lineDst = static_cast<unsigned short*>(static_cast<void*>(
                            static_cast<unsigned char*>(static_cast<void*>(lineDst)) + dpitch));
                    }
                } else {
                    unsigned short* lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(dst)) +
                        dy * dpitch + (dx + sw) * 2));

                    for (int y = sy; y < sy + sh; ++y) {
                        unsigned short* out = lineDst;
                        const unsigned char* src = map + lineOffset[y];
                        unsigned int skipped = 0;
                        unsigned char packet = *src;
                        unsigned char code = packet >> 5;
                        unsigned int run = (packet & 31) + 1;
                        ++src;

                        while (skipped + run <=
                               static_cast<unsigned int>(sx)) {
                            skipped += run;
                            if (code == kOpaqueRunCode)
                                src += run;
                            packet = *src;
                            code = packet >> 5;
                            run = (packet & 31) + 1;
                            ++src;
                        }
                        run += skipped - sx;
                        if (code == kOpaqueRunCode)
                            src += sx - skipped;

                        unsigned int remaining = sw;
                        do {
                            if (run > remaining)
                                run = remaining;
                            if (code == kOpaqueRunCode) {
                                out -= run;
                                src += run;
                            } else {
                                switch (code) {
                                case eRleControlShadow75:
                                case eRleControlShadow2: {
                                    unsigned int count = run;
                                    do {
                                        --out;
                                        *out = ((*out >> 1) & div2mask)
                                             + ((*out >> 2) & div4mask);
                                    } while (--count);
                                    break;
                                }
                                case eRleControlShadow3:
                                case eRleControlShadow50: {
                                    unsigned int count = run;
                                    do {
                                        --out;
                                        *out = (*out >> 1) & div2mask;
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

                        lineDst = static_cast<unsigned short*>(static_cast<void*>(
                            static_cast<unsigned char*>(static_cast<void*>(lineDst)) + dpitch));
                    }
                }
            } else {
                if (!hflip) {
                    unsigned short* lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(dst)) +
                        (dy + sh - 1) * dpitch + dx * 2));

                    for (int y = sy; y < sy + sh; ++y) {
                        unsigned short* out = lineDst;
                        const unsigned char* src = map + lineOffset[y];
                        unsigned int skipped = 0;
                        unsigned char packet = *src;
                        unsigned char code = packet >> 5;
                        unsigned int run = (packet & 31) + 1;
                        ++src;

                        while (skipped + run <=
                               static_cast<unsigned int>(sx)) {
                            skipped += run;
                            if (code == kOpaqueRunCode)
                                src += run;
                            packet = *src;
                            code = packet >> 5;
                            run = (packet & 31) + 1;
                            ++src;
                        }
                        run += skipped - sx;
                        if (code == kOpaqueRunCode)
                            src += sx - skipped;

                        unsigned int remaining = sw;
                        do {
                            if (run > remaining)
                                run = remaining;
                            if (code == kOpaqueRunCode) {
                                out += run;
                                src += run;
                            } else {
                                switch (code) {
                                case eRleControlShadow75:
                                case eRleControlShadow2: {
                                    unsigned int count = run;
                                    do {
                                        *out = ((*out >> 1) & div2mask)
                                             + ((*out >> 2) & div4mask);
                                        ++out;
                                    } while (--count);
                                    break;
                                }
                                case eRleControlShadow3:
                                case eRleControlShadow50: {
                                    unsigned int count = run;
                                    do {
                                        unsigned int color = *out;
                                        *out = (color >> 1) & div2mask;
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

                        lineDst = static_cast<unsigned short*>(static_cast<void*>(
                            static_cast<unsigned char*>(static_cast<void*>(lineDst)) - dpitch));
                    }
                } else {
                    unsigned short* lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(dst)) +
                        (dy + sh - 1) * dpitch + (dx + sw) * 2));

                    for (int y = sy; y < sy + sh; ++y) {
                        unsigned short* out = lineDst;
                        const unsigned char* src = map + lineOffset[y];
                        unsigned int skipped = 0;
                        unsigned char packet = *src;
                        unsigned char code = packet >> 5;
                        unsigned int run = (packet & 31) + 1;
                        ++src;

                        while (skipped + run <=
                               static_cast<unsigned int>(sx)) {
                            skipped += run;
                            if (code == kOpaqueRunCode)
                                src += run;
                            packet = *src;
                            code = packet >> 5;
                            run = (packet & 31) + 1;
                            ++src;
                        }
                        run += skipped - sx;
                        if (code == kOpaqueRunCode)
                            src += sx - skipped;

                        unsigned int remaining = sw;
                        do {
                            if (run > remaining)
                                run = remaining;
                            if (code == kOpaqueRunCode) {
                                out -= run;
                                src += run;
                            } else {
                                switch (code) {
                                case eRleControlShadow75:
                                case eRleControlShadow2: {
                                    unsigned int count = run;
                                    do {
                                        --out;
                                        *out = ((*out >> 1) & div2mask)
                                             + ((*out >> 2) & div4mask);
                                    } while (--count);
                                    break;
                                }
                                case eRleControlShadow3:
                                case eRleControlShadow50: {
                                    unsigned int count = run;
                                    do {
                                        --out;
                                        *out = (*out >> 1) & div2mask;
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

                        lineDst = static_cast<unsigned short*>(static_cast<void*>(
                            static_cast<unsigned char*>(static_cast<void*>(lineDst)) - dpitch));
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
// The native row cursor restores all Windows bytes. Splitting its address
// into a fixed base and row offset leaves 98.0214%; retain one row lifetime.
// 99.3376% since the DC-proven bool flags: the two blend loops use word
// `and dx,word [div2mask]` where retail loads the mask as a dword. This is
// name-keyed TU state (docs/vc6/behavior-catalog.md C12): spelling draw as DC's
// `Draw` (or drawz/drawA) restores every instruction, `draw`/`drawRle` do not;
// true/false call arguments, uchar clip flags and dropping the draw call
// leave it unchanged. The project's normalized names are kept.
VA(0x0047ef60, 0x47C)
DC_ADDRESS(0x077664, 0x338)
MAC_ADDRESS(0x08def8, 0x44c)  // anchor-callee (CSprite::DrawSpellEffect) + DC source identity
void CSpriteFrame::DrawSpellEffect(int sx, int sy, int sw, int sh,
                                   unsigned short* dst, int dx, int dy, int dw,
                                   int dh, int dpitch, TPalette16& pal,
                                   bool hflip,
                                   bool alpha) const
{
    if (!alpha) {
        Draw(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, 1);
        return;
    }

    if (EncodingMethod == eEncodeTilesetRLE || EncodingMethod == eEncodeRaw) {
        DrawTile(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                 pal, hflip, 0);
        return;
    }
    else if (EncodingMethod == eEncodeAdvObjRLE) {
        DrawHeroAlpha(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip);
        return;
    }

    const unsigned int* lineOffset;
    // DC 0x77664 local palette, bound after the line table at line 3811.
    const unsigned short* palette;
    // Retail construction guard 0x6968a7; the copied run code at 0x6968b6.
    DATA_COMPGEN_GUARD(0x006968a7, spellEffectOpaqueRunCodeGuard, kOpaqueRunCode)
    DATA(0x006968b6)
    static const unsigned char kOpaqueRunCode = kGeneralRLEOpaqueRunCode;
    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, 0);

    if (sw > 0 && sh > 0) {
        lineOffset =
            static_cast<const unsigned int*>(static_cast<const void*>(map));
        palette = pal.m_data;
        if (!hflip) {
            unsigned short* lineDst =
                static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + dy * dpitch + dx * 2));

            for (int y = sy; y < sy + sh; ++y) {
                unsigned short* out = lineDst;
                unsigned int skipped = 0;
                const unsigned char* src = map + lineOffset[y];
                unsigned char code = *src++;
                unsigned int run = *src++ + 1;

                while (skipped + run <= static_cast<unsigned int>(sx)) {
                    skipped += run;
                    if (code == kOpaqueRunCode)
                        src += run;
                    code = *src++;
                    run = *src++ + 1;
                }

                run += skipped - sx;
                if (code == kOpaqueRunCode)
                    src += sx - skipped;

                unsigned int remaining = sw;
                do {
                    if (run > remaining)
                        run = remaining;
                    if (code == kOpaqueRunCode) {
                        unsigned int count = run;
                        do {
                            *out = (div2mask
                                    & (palette[*src++] >> 1))
                                 + (div2mask & (*out >> 1));
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

                lineDst = static_cast<unsigned short*>(static_cast<void*>(
                    static_cast<unsigned char*>(static_cast<void*>(lineDst)) + dpitch));
            }
        } else {
            unsigned short* lineDst =
                static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + dy * dpitch + (dx + sw) * 2));

            for (int y = sy; y < sy + sh; ++y) {
                unsigned short* out = lineDst;
                unsigned int skipped = 0;
                const unsigned char* src = map + lineOffset[y];
                unsigned char code = *src++;
                unsigned int run = *src++ + 1;

                while (skipped + run <= static_cast<unsigned int>(sx)) {
                    skipped += run;
                    if (code == kOpaqueRunCode)
                        src += run;
                    code = *src++;
                    run = *src++ + 1;
                }

                run += skipped - sx;
                if (code == kOpaqueRunCode)
                    src += sx - skipped;

                unsigned int remaining = sw;
                do {
                    if (run > remaining)
                        run = remaining;
                    if (code == kOpaqueRunCode) {
                        unsigned int count = run;
                        do {
                            --out;
                            *out = (div2mask
                                    & (palette[*src++] >> 1))
                                 + (div2mask & (*out >> 1));
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

                lineDst = static_cast<unsigned short*>(static_cast<void*>(
                    static_cast<unsigned char*>(static_cast<void*>(lineDst)) + dpitch));
            }
        }
    }
}

// Original: CSpriteFrame::ClipScaled50; cspriteframe.cpp:3949
DC_ADDRESS(0x07799c, 0x1d0)
void CSpriteFrame::ClipScaled50(int& sx, int& sy, int& sw, int& sh,
                                    int& dx, int& dy, int dw, int dh,
                                    bool hflip, bool vflip) const
{
    int scaledWidth = (Width + 1) >> 1;
    int scaledHeight = (Height + 1) >> 1;
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
    int scaledCroppedX = (CroppedX + 1) >> 1;
    int scaledCroppedY = (CroppedY + 1) >> 1;
    int scaledCroppedWidth = ((CroppedX + CroppedWidth + 1) >> 1) - scaledCroppedX;
    int scaledCroppedHeight = ((CroppedY + CroppedHeight + 1) >> 1) - scaledCroppedY;
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
    sx -= CroppedX;
    sy -= CroppedY;
}

// Original: CSpriteFrame::DrawAdvObjWithFlagScaled50; cspriteframe.cpp:4054
DC_ADDRESS(0x077b6c, 0x1ea)
void CSpriteFrame::DrawAdvObjWithFlagScaled50(int sx, int sy, int sw, int sh,
    unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
    TPalette16& pal, unsigned short flagcolor) const
{
    ClipScaled50(sx, sy, sw, sh, dx, dy, dw, dh, 0, 0);
    if (sw <= 0 || sh <= 0)
        return;
    unsigned int cellsPerLine = CroppedWidth >> 5;
    const unsigned short* const cellOffset = static_cast<const unsigned short*>(static_cast<const void*>(map));
    const unsigned short* const palette = pal.m_data;
    unsigned char* lineDst = static_cast<unsigned char*>(static_cast<void*>(dst)) + dy * dpitch + dx * 2;
    for (int y = sy; y < sy + sh; y += 2) {
        unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
        const unsigned char* source = map + cellOffset[y * cellsPerLine + (sx >> 5)];
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

// Original: CSpriteFrame::DrawAdvObjShadowScaled50; cspriteframe.cpp:4187
DC_ADDRESS(0x077d58, 0x240)
void CSpriteFrame::DrawAdvObjShadowScaled50(int sx, int sy, int sw, int sh,
    unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
    TPalette16& pal) const
{
    ClipScaled50(sx, sy, sw, sh, dx, dy, dw, dh, 0, 0);
    if (sw <= 0 || sh <= 0)
        return;
    unsigned int cellsPerLine = CroppedWidth >> 5;
    const unsigned short* const cellOffset = static_cast<const unsigned short*>(static_cast<const void*>(map));
    unsigned char* lineDst = static_cast<unsigned char*>(static_cast<void*>(dst)) + dy * dpitch + dx * 2;
    for (int y = sy; y < sy + sh; y += 2) {
        unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
        const unsigned char* source = map + cellOffset[y * cellsPerLine + (sx >> 5)];
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
                        *out = ((*out >> 1) & div2mask) + ((*out >> 2) & div4mask);
                        ++out;
                        count -= 2;
                    }
                    skip = count != 0;
                    if (skip) {
                        *out = ((*out >> 1) & div2mask) + ((*out >> 2) & div4mask);
                        ++out;
                    }
                    break;
                }
                case eRleControlShadow50: {
                    unsigned int count = run;
                    if (skip)
                        --count;
                    while (count > 1) {
                        *out = (*out >> 1) & div2mask;
                        ++out;
                        count -= 2;
                    }
                    skip = count != 0;
                    if (skip) {
                        *out = (*out >> 1) & div2mask;
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

// Original: CSpriteFrame::DrawTileScaled50; cspriteframe.cpp:4330
DC_ADDRESS(0x077f98, 0x606)
void CSpriteFrame::DrawTileScaled50(int sx, int sy, int sw, int sh,
    unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
    TPalette16& pal, bool hflip, bool vflip) const
{
    ClipScaled50(sx, sy, sw, sh, dx, dy, dw, dh, hflip, vflip);
    if (sw <= 0 || sh <= 0)
        return;
    const unsigned short* const lineOffset = static_cast<const unsigned short*>(static_cast<const void*>(map));
    const unsigned short* const palette = pal.m_data;
    if (!vflip) {
        if (!hflip) {
            unsigned char* lineDst = static_cast<unsigned char*>(static_cast<void*>(dst))
                + (dy) * dpitch + (dx) * 2;
            if (EncodingMethod == eEncodeRaw) {
                const unsigned char* line = map + sy * Pitch + sx;
                do {
                    int remaining = sw;
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    const unsigned char* source = line;
                    do {
                        *out++ = palette[*source];
                        source += 2;
                        remaining -= 2;
                    } while (remaining > 0);
                    line += Pitch * 2;
                    sh -= 2;
                    lineDst += dpitch;
                } while (sh > 0);
            } else {
                for (int y = sy; y < sy + sh; y += 2) {
                    const unsigned char* source = map + lineOffset[y];
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
            if (EncodingMethod == eEncodeRaw) {
                const unsigned char* line = map + sy * Pitch + sx;
                do {
                    int remaining = sw;
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    const unsigned char* source = line;
                    do {
                        *--out = palette[*source];
                        source += 2;
                        remaining -= 2;
                    } while (remaining > 0);
                    line += Pitch * 2;
                    sh -= 2;
                    lineDst += dpitch;
                } while (sh > 0);
            } else {
                for (int y = sy; y < sy + sh; y += 2) {
                    const unsigned char* source = map + lineOffset[y];
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
            if (EncodingMethod == eEncodeRaw) {
                const unsigned char* line = map + sy * Pitch + sx;
                do {
                    int remaining = sw;
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    const unsigned char* source = line;
                    do {
                        *out++ = palette[*source];
                        source += 2;
                        remaining -= 2;
                    } while (remaining > 0);
                    line += Pitch * 2;
                    sh -= 2;
                    lineDst -= dpitch;
                } while (sh > 0);
            } else {
                for (int y = sy; y < sy + sh; y += 2) {
                    const unsigned char* source = map + lineOffset[y];
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
            if (EncodingMethod == eEncodeRaw) {
                const unsigned char* line = map + sy * Pitch + sx;
                do {
                    int remaining = sw;
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    const unsigned char* source = line;
                    do {
                        *--out = palette[*source];
                        source += 2;
                        remaining -= 2;
                    } while (remaining > 0);
                    line += Pitch * 2;
                    sh -= 2;
                    lineDst -= dpitch;
                } while (sh > 0);
            } else {
                for (int y = sy; y < sy + sh; y += 2) {
                    const unsigned char* source = map + lineOffset[y];
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

// Original: CSpriteFrame::ClipScaled25; cspriteframe.cpp:4802
DC_ADDRESS(0x0785a0, 0x1d4)
void CSpriteFrame::ClipScaled25(int& sx, int& sy, int& sw, int& sh,
                                    int& dx, int& dy, int dw, int dh,
                                    bool hflip, bool vflip) const
{
    int scaledWidth = (Width + 3) >> 2;
    int scaledHeight = (Height + 3) >> 2;
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
    int scaledCroppedX = (CroppedX + 3) >> 2;
    int scaledCroppedY = (CroppedY + 3) >> 2;
    int scaledCroppedWidth = ((CroppedX + CroppedWidth + 3) >> 2) - scaledCroppedX;
    int scaledCroppedHeight = ((CroppedY + CroppedHeight + 3) >> 2) - scaledCroppedY;
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
    sx -= CroppedX;
    sy -= CroppedY;
}

// Original: CSpriteFrame::DrawAdvObjWithFlagScaled25; cspriteframe.cpp:4907
DC_ADDRESS(0x078774, 0x208)
void CSpriteFrame::DrawAdvObjWithFlagScaled25(int sx, int sy, int sw, int sh,
    unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
    TPalette16& pal, unsigned short flagcolor) const
{
    ClipScaled25(sx, sy, sw, sh, dx, dy, dw, dh, 0, 0);
    if (sw <= 0 || sh <= 0)
        return;
    unsigned int cellsPerLine = CroppedWidth >> 5;
    const unsigned short* const cellOffset = static_cast<const unsigned short*>(static_cast<const void*>(map));
    const unsigned short* const palette = pal.m_data;
    unsigned char* lineDst = static_cast<unsigned char*>(static_cast<void*>(dst)) + dy * dpitch + dx * 2;
    for (int y = sy; y < sy + sh; y += 4) {
        unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
        const unsigned char* source = map + cellOffset[y * cellsPerLine + (sx >> 5)];
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

// Original: CSpriteFrame::DrawAdvObjShadowScaled25; cspriteframe.cpp:5054
DC_ADDRESS(0x07897c, 0x262)
void CSpriteFrame::DrawAdvObjShadowScaled25(int sx, int sy, int sw, int sh,
    unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
    TPalette16& pal) const
{
    ClipScaled25(sx, sy, sw, sh, dx, dy, dw, dh, 0, 0);
    if (sw <= 0 || sh <= 0)
        return;
    unsigned int cellsPerLine = CroppedWidth >> 5;
    const unsigned short* const cellOffset = static_cast<const unsigned short*>(static_cast<const void*>(map));
    unsigned char* lineDst = static_cast<unsigned char*>(static_cast<void*>(dst)) + dy * dpitch + dx * 2;
    for (int y = sy; y < sy + sh; y += 4) {
        unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
        const unsigned char* source = map + cellOffset[y * cellsPerLine + (sx >> 5)];
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
                        *out = ((*out >> 1) & div2mask) + ((*out >> 2) & div4mask);
                        ++out;
                        count -= 4;
                    }
                    if (count) {
                        *out = ((*out >> 1) & div2mask) + ((*out >> 2) & div4mask);
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
                        *out = (*out >> 1) & div2mask;
                        ++out;
                        count -= 4;
                    }
                    if (count) {
                        *out = (*out >> 1) & div2mask;
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

// Original: CSpriteFrame::DrawTileScaled25; cspriteframe.cpp:5201
DC_ADDRESS(0x078be0, 0x67e)
void CSpriteFrame::DrawTileScaled25(int sx, int sy, int sw, int sh,
    unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
    TPalette16& pal, bool hflip, bool vflip) const
{
    ClipScaled25(sx, sy, sw, sh, dx, dy, dw, dh, hflip, vflip);
    if (sw <= 0 || sh <= 0)
        return;
    const unsigned short* const lineOffset = static_cast<const unsigned short*>(static_cast<const void*>(map));
    const unsigned short* const palette = pal.m_data;
    if (!vflip) {
        if (!hflip) {
            unsigned char* lineDst = static_cast<unsigned char*>(static_cast<void*>(dst))
                + (dy) * dpitch + (dx) * 2;
            if (EncodingMethod == eEncodeRaw) {
                const unsigned char* line = map + sy * Pitch + sx;
                do {
                    int remaining = sw;
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    const unsigned char* source = line;
                    do {
                        *out++ = palette[*source];
                        source += 4;
                        remaining -= 4;
                    } while (remaining > 0);
                    line += Pitch * 4;
                    sh -= 4;
                    lineDst += dpitch;
                } while (sh > 0);
            } else {
                for (int y = sy; y < sy + sh; y += 4) {
                    const unsigned char* source = map + lineOffset[y];
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
            if (EncodingMethod == eEncodeRaw) {
                const unsigned char* line = map + sy * Pitch + sx;
                do {
                    int remaining = sw;
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    const unsigned char* source = line;
                    do {
                        *--out = palette[*source];
                        source += 4;
                        remaining -= 4;
                    } while (remaining > 0);
                    line += Pitch * 4;
                    sh -= 4;
                    lineDst += dpitch;
                } while (sh > 0);
            } else {
                for (int y = sy; y < sy + sh; y += 4) {
                    const unsigned char* source = map + lineOffset[y];
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
            if (EncodingMethod == eEncodeRaw) {
                const unsigned char* line = map + sy * Pitch + sx;
                do {
                    int remaining = sw;
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    const unsigned char* source = line;
                    do {
                        *out++ = palette[*source];
                        source += 4;
                        remaining -= 4;
                    } while (remaining > 0);
                    line += Pitch * 4;
                    sh -= 4;
                    lineDst -= dpitch;
                } while (sh > 0);
            } else {
                for (int y = sy; y < sy + sh; y += 4) {
                    const unsigned char* source = map + lineOffset[y];
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
            if (EncodingMethod == eEncodeRaw) {
                const unsigned char* line = map + sy * Pitch + sx;
                do {
                    int remaining = sw;
                    unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(lineDst));
                    const unsigned char* source = line;
                    do {
                        *--out = palette[*source];
                        source += 4;
                        remaining -= 4;
                    } while (remaining > 0);
                    line += Pitch * 4;
                    sh -= 4;
                    lineDst -= dpitch;
                } while (sh > 0);
            } else {
                for (int y = sy; y < sy + sh; y += 4) {
                    const unsigned char* source = map + lineOffset[y];
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
