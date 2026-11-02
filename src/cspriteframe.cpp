#include "va.h"

#include <assert.h>
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
VA_COMPGEN_AT(h3maped, 0x004dec20, 0x8, STATIC_CTOR, kGeneralRLEOpaqueRunCode)
VA_COMPGEN_AT(h3ccmped, 0x0044b260, 0x8, STATIC_CTOR, kGeneralRLEOpaqueRunCode)

// Original: kGeneralRLEMaxRunLength, file-static const unsigned int.
// DC cspriteframe.cpp:47 calls the same max helper, then adds one;
// retail CRT 0x47c270 installs 256 in this otherwise unreferenced storage.
DATA(0x006968b0) static const unsigned int kGeneralRLEMaxRunLength =
    std::numeric_limits<unsigned char>::max() + 1;
VA_COMPGEN_AT(h3maped, 0x004dec40, 0xb, STATIC_CTOR, kGeneralRLEMaxRunLength)
VA_COMPGEN_AT(h3ccmped, 0x0044b280, 0xb, STATIC_CTOR, kGeneralRLEMaxRunLength)

// Original: kCellWidth (Loki h3maped's CSpriteFrame.cpp asserts); the pixel
// width of an adventure-object cell. Its unsigned type also recovers the
// logical cell-division shifts retained by the editor renderers.
static const unsigned int kCellWidth = 32;

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
    assert(csize >= 0);
    assert(encoding == eEncodeRaw || csize > 0);
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
    assert(csize >= 0);
    assert(encoding == eEncodeRaw || csize > 0);
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
        unsigned char code = *source++;
        unsigned int run = *source++ + 1;
        while (position + run <= static_cast<unsigned int>(x)) {
            position += run;
            if (code == kGeneralRLEOpaqueRunCode)
                source += run;
            code = *source++;
            run = *source++ + 1;
        }
        if (code == kGeneralRLEOpaqueRunCode)
            pixel = source[x - position];
        else
            pixel = code;
        break;
    }
    case eEncodeTilesetRLE: {
        const unsigned short* lineOffsets =
            static_cast<const unsigned short*>(static_cast<const void*>(map));
        const unsigned char* source = map + lineOffsets[y];
        unsigned int position = 0;
        unsigned char control = *source;
        unsigned char code = control >> 5;
        unsigned int run = (control & 31) + 1;
        ++source;
        while (position + run <= static_cast<unsigned int>(x)) {
            position += run;
            if (code == ePackedRleLiteral)
                source += run;
            control = *source;
            code = control >> 5;
            run = (control & 31) + 1;
            ++source;
        }
        if (code == ePackedRleLiteral)
            pixel = source[x - position];
        else
            pixel = code;
        break;
    }
    case eEncodeAdvObjRLE: {
        assert(CroppedWidth % kCellWidth == 0);
        unsigned int cellsPerRow = static_cast<unsigned int>(CroppedWidth) >> 5;
        const unsigned short* cellOffsets =
            static_cast<const unsigned short*>(static_cast<const void*>(map));
        const unsigned char* source = map + cellOffsets[y * cellsPerRow + (static_cast<unsigned int>(x) >> 5)];
        unsigned int position = x - (x & 31);
        unsigned char control = *source;
        unsigned char code = control >> 5;
        unsigned int run = (control & 31) + 1;
        ++source;
        while (position + run <= static_cast<unsigned int>(x)) {
            position += run;
            if (code == ePackedRleLiteral)
                source += run;
            control = *source;
            code = control >> 5;
            run = (control & 31) + 1;
            ++source;
        }
        if (code == ePackedRleLiteral)
            pixel = source[x - position];
        else
            pixel = code;
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
    assert(EncodingMethod == eEncodeRaw);
    assert(method > eEncodeRaw && method < kNumEncodingMethods);
    assert(CroppedWidth > 0 && CroppedHeight > 0);
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
    unsigned int curOffset = CroppedHeight * sizeof(unsigned int);
    source = map;
    do {
        *lineOffset++ = curOffset;
        unsigned char code = source[0] < 10 ? source[0] : kOpaqueRunCode;
        newMap[curOffset++] = code;
        unsigned char* runLength = newMap + curOffset++;
        unsigned int run = 0;
        int x = 1;
        while (1) {
            ++run;
            if (code == kOpaqueRunCode)
                newMap[curOffset++] = source[x - 1];
            if (x >= CroppedWidth)
                break;
            unsigned char nextCode = source[x] < 10 ? source[x] : kOpaqueRunCode;
            if (nextCode != code || run == kMaxRunLength) {
                *runLength = static_cast<unsigned char>(run - 1);
                code = nextCode;
                newMap[curOffset++] = code;
                runLength = newMap + curOffset++;
                run = 0;
            }
            ++x;
        }
        *runLength = static_cast<unsigned char>(run - 1);
        source += CroppedWidth;
    } while (--linesToGo);
    assert(curOffset == newDataSize);
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
        unsigned short curOffset = static_cast<unsigned short>(CroppedHeight * sizeof(unsigned short));
        source = map;
        do {
            *lineOffset++ = curOffset;
            unsigned char code = source[0] < 5 ? source[0] : ePackedRleLiteral;
            bool opaque = code == ePackedRleLiteral;
            unsigned char* control = newMap + curOffset;
            *control = static_cast<unsigned char>(code << 5);
            ++curOffset;
            unsigned int run = 0;
            int x = 1;
            while (1) {
                ++run;
                if (opaque)
                    newMap[curOffset++] = source[x - 1];
                if (x >= CroppedWidth)
                    break;
                unsigned char nextCode = source[x] < 5 ? source[x] : ePackedRleLiteral;
                if (nextCode != code || run == ePackedRleMaxRunLength) {
                    *control |= static_cast<unsigned char>(run - 1);
                    code = nextCode;
                    control = newMap + curOffset;
                    *control = static_cast<unsigned char>(code << 5);
                    ++curOffset;
                    run = 0;
                    opaque = code == ePackedRleLiteral;
                }
                ++x;
            }
            *control |= static_cast<unsigned char>(run - 1);
            source += CroppedWidth;
        } while (--linesToGo);
        assert(curOffset == newDataSize);
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
    unsigned short curOffset = static_cast<unsigned short>(2 * cellsPerLine * CroppedHeight);
    source = map;
    linesToGo = CroppedHeight;
    do {
        unsigned char* sourceCell = source;
        int srcLineX = 0;
        unsigned int cellsToGo = cellsPerLine;
        unsigned char code;
        unsigned char* control;
        unsigned int run;
        if (addLeft) {
            *cellOffset++ = curOffset;
            code = source[0] < 6 ? source[0] : ePackedRleLiteral;
            run = 0;
            if (code != 0) {
                newMap[curOffset] = 0;
                newMap[curOffset++] |= static_cast<unsigned char>(addLeft - 1);
            } else {
                run += addLeft;
            }
            control = newMap + curOffset;
            *control = static_cast<unsigned char>(code << 5);
            ++curOffset;
            unsigned int x = 1;
            while (1) {
                ++srcLineX;
                ++run;
                if (code == ePackedRleLiteral)
                    newMap[curOffset++] = source[x - 1];
                if (x >= 32 - addLeft || srcLineX >= oldWidth)
                    break;
                unsigned char nextCode = source[x] < 6 ? source[x] : ePackedRleLiteral;
                if (nextCode != code) {
                    *control |= static_cast<unsigned char>(run - 1);
                    code = nextCode;
                    control = newMap + curOffset;
                    *control = static_cast<unsigned char>(code << 5);
                    ++curOffset;
                    run = 0;
                }
                ++x;
            }
            sourceCell += x;
            if (--cellsToGo)
                *control |= static_cast<unsigned char>(run - 1);
        }
        while (cellsToGo) {
            *cellOffset++ = curOffset;
            code = sourceCell[0] < 6 ? sourceCell[0] : ePackedRleLiteral;
            control = newMap + curOffset;
            *control = static_cast<unsigned char>(code << 5);
            ++curOffset;
            run = 0;
            unsigned int x = 1;
            while (1) {
                ++srcLineX;
                ++run;
                if (code == ePackedRleLiteral)
                    newMap[curOffset++] = sourceCell[x - 1];
                if (x >= 32 || srcLineX >= oldWidth)
                    break;
                unsigned char nextCode = sourceCell[x] < 6 ? sourceCell[x] : ePackedRleLiteral;
                if (nextCode != code) {
                    *control |= static_cast<unsigned char>(run - 1);
                    code = nextCode;
                    control = newMap + curOffset;
                    *control = static_cast<unsigned char>(code << 5);
                    ++curOffset;
                    run = 0;
                }
                ++x;
            }
            sourceCell += x;
            if (!--cellsToGo)
                break;
            *control |= static_cast<unsigned char>(run - 1);
        }
        assert(srcLineX == CroppedWidth);
        if (addRight) {
            if (code != 0) {
                *control |= static_cast<unsigned char>(run - 1);
                control = newMap + curOffset;
                *control = 0;
                ++curOffset;
                run = addRight;
            } else {
                run += addRight;
            }
        }
        *control |= static_cast<unsigned char>(run - 1);
        source += oldWidth;
    } while (--linesToGo);
    assert(curOffset == newDataSize);
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
// Dreamcast names the table pointer `aLineOffset` (const) and records each
// row loop as one enclosing lifetime: lineDst advances directly by dpitch.
// Loki's packet-reading loop keeps code/count decoding inside the skip loop
// and exits when the requested source column lies in the current run.
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

    assert(EncodingMethod == eEncodeGeneralRLE);
    // Retail construction guard 0x6968b7; the copied run code at 0x69689c.
    DATA_COMPGEN_GUARD(0x006968b7, drawOpaqueRunCodeGuard, kOpaqueRunCode)
    DATA(0x0069689c)
    static const unsigned char kOpaqueRunCode = kGeneralRLEOpaqueRunCode;
    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, 0);

    if (sw <= 0 || sh <= 0)
        return;

    const unsigned int* const aLineOffset =
        static_cast<const unsigned int*>(static_cast<const void*>(map));
    const unsigned short* const palette = pal.Palette;
    if (!hflip) {
        unsigned short* lineDst =
            static_cast<unsigned short*>(static_cast<void*>(
            static_cast<unsigned char*>(static_cast<void*>(dst))
            + dy * dpitch + dx * 2));
        for (int y = sy; y < sy + sh; ++y) {
            unsigned short* out = lineDst;
            unsigned char* src = map + aLineOffset[y];
            unsigned char code;
            unsigned int runLength;
            unsigned int skipped = 0;
            for (;;) {
                code = *src++;
                runLength = *src++ + 1;
                assert(runLength > 0);
                if (skipped + runLength > static_cast<unsigned int>(sx)) {
                    runLength -= sx - skipped;
                    if (code == kOpaqueRunCode)
                        src += sx - skipped;
                    skipped = sx;
                    break;
                }
                skipped += runLength;
                if (code == kOpaqueRunCode)
                    src += runLength;
            }

            unsigned int remaining = sw;
            for (;;) {
                assert(runLength > 0);
                if (runLength > remaining)
                    runLength = remaining;
                if (code == kOpaqueRunCode) {
                    unsigned int count = runLength;
                    do {
                        *out++ = palette[*src++];
                    } while (--count);
                } else if (tblit) {
                    out += runLength;
                } else {
                    unsigned short color = palette[code];
                    unsigned int count = runLength;
                    do {
                        *out++ = color;
                    } while (--count);
                }
                remaining -= runLength;
                if (!remaining)
                    break;
                code = *src++;
                runLength = *src++ + 1;
            }

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
            unsigned char* src = map + aLineOffset[y];
            unsigned char code;
            unsigned int runLength;
            unsigned int skipped = 0;
            for (;;) {
                code = *src++;
                runLength = *src++ + 1;
                assert(runLength > 0);
                if (skipped + runLength > static_cast<unsigned int>(sx)) {
                    runLength -= sx - skipped;
                    if (code == kOpaqueRunCode)
                        src += sx - skipped;
                    skipped = sx;
                    break;
                }
                skipped += runLength;
                if (code == kOpaqueRunCode)
                    src += runLength;
            }

            unsigned int remaining = sw;
            for (;;) {
                assert(runLength > 0);
                if (runLength > remaining)
                    runLength = remaining;
                if (code == kOpaqueRunCode) {
                    unsigned int count = runLength;
                    do {
                        *--out = palette[*src++];
                    } while (--count);
                } else if (tblit) {
                    out -= runLength;
                } else {
                    unsigned short color = palette[code];
                    unsigned int count = runLength;
                    do {
                        *--out = color;
                    } while (--count);
                }
                remaining -= runLength;
                if (!remaining)
                    break;
                code = *src++;
                runLength = *src++ + 1;
            }

            lineDst = static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(lineDst))
                + dpitch));
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

    assert(EncodingMethod == eEncodeGeneralRLE);
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
        palette = pal.Palette;
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
                unsigned int runLength = *src++ + 1;
                assert(runLength > 0);

                while (skipped + runLength <= static_cast<unsigned int>(sx)) {
                    skipped += runLength;
                    if (code == kOpaqueRunCode)
                        src += runLength;
                    code = *src++;
                    runLength = *src++ + 1;
                }

                runLength += skipped - sx;
                if (code == kOpaqueRunCode)
                    src += sx - skipped;

                unsigned int remaining = sw;
                do {
                    assert(runLength > 0);
                    if (runLength > remaining)
                        runLength = remaining;
                    if (code == kOpaqueRunCode) {
                        unsigned int count = runLength;
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
                            unsigned int count = runLength;
                            do {
                                *out = ((*out >> 1) & div2mask)
                                     + ((*out >> 2) & div4mask);
                                ++out;
                            } while (--count);
                            break;
                        }
                        case eRleControlShadow50:
                        case eRleControlOutline6: {
                            unsigned int count = runLength;
                            do {
                                *out = (*out >> 1) & div2mask;
                                ++out;
                            } while (--count);
                            break;
                        }
                        default:
                            out += runLength;
                            break;
                        }
                    } else {
                        switch (code) {
                        case eRleControlShadow75: {
                            unsigned int count = runLength;
                            do {
                                *out = ((*out >> 1) & div2mask)
                                     + ((*out >> 2) & div4mask);
                                ++out;
                            } while (--count);
                            break;
                        }
                        case eRleControlShadow50: {
                            unsigned int count = runLength;
                            do {
                                *out = (*out >> 1) & div2mask;
                                ++out;
                            } while (--count);
                            break;
                        }
                        case eRleControlOutline5:
                        case eRleControlOutline6:
                        case eRleControlOutline7: {
                            unsigned int count = runLength;
                            do {
                                *out++ = outcolor;
                            } while (--count);
                            break;
                        }
                        default:
                            out += runLength;
                            break;
                        }
                    }
                    remaining -= runLength;
                    if (!remaining)
                        break;
                    code = *src++;
                    runLength = *src++ + 1;
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
                unsigned int runLength = *src++ + 1;
                assert(runLength > 0);

                while (skipped + runLength <= static_cast<unsigned int>(sx)) {
                    skipped += runLength;
                    if (code == kOpaqueRunCode)
                        src += runLength;
                    code = *src++;
                    runLength = *src++ + 1;
                }

                runLength += skipped - sx;
                if (code == kOpaqueRunCode)
                    src += sx - skipped;

                unsigned int remaining = sw;
                do {
                    assert(runLength > 0);
                    if (runLength > remaining)
                        runLength = remaining;
                    if (code == kOpaqueRunCode) {
                        unsigned int count = runLength;
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
                            unsigned int count = runLength;
                            do {
                                --out;
                                *out = ((*out >> 1) & div2mask)
                                     + ((*out >> 2) & div4mask);
                            } while (--count);
                            break;
                        }
                        case eRleControlShadow50:
                        case eRleControlOutline6: {
                            unsigned int count = runLength;
                            do {
                                --out;
                                *out = (*out >> 1) & div2mask;
                            } while (--count);
                            break;
                        }
                        default:
                            out -= runLength;
                            break;
                        }
                    } else {
                        switch (code) {
                        case eRleControlShadow75: {
                            unsigned int count = runLength;
                            do {
                                --out;
                                *out = ((*out >> 1) & div2mask)
                                     + ((*out >> 2) & div4mask);
                            } while (--count);
                            break;
                        }
                        case eRleControlShadow50: {
                            unsigned int count = runLength;
                            do {
                                --out;
                                *out = (*out >> 1) & div2mask;
                            } while (--count);
                            break;
                        }
                        case eRleControlOutline5:
                        case eRleControlOutline6:
                        case eRleControlOutline7: {
                            unsigned int count = runLength;
                            do {
                                *--out = outcolor;
                            } while (--count);
                            break;
                        }
                        default:
                            out -= runLength;
                            break;
                        }
                    }
                    remaining -= runLength;
                    if (!remaining)
                        break;
                    code = *src++;
                    runLength = *src++ + 1;
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
// row loop one enclosing lifetime: lineDst advances directly by dpitch in
// both retail direction arms (not rowBase plus an integer rowOffset).
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

    assert(EncodingMethod == eEncodeAdvObjRLE);
    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, 0);
    if (sw > 0) {
        if (sh > 0) {

            assert(CroppedWidth % kCellWidth == 0);
            cellsPerLine = static_cast<unsigned int>(CroppedWidth) >> 5;
            cellOffset = static_cast<const unsigned short*>(static_cast<const void*>(map));
            palette = pal.Palette;

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
                    unsigned int runLength = (packet & 31) + 1;
                    ++src;

                    while (skipped + runLength <= static_cast<unsigned int>(sx)) {
                        skipped += runLength;
                        if (code == eRleControlOutline7)
                            src += runLength;
                        packet = *src;
                        code = packet >> 5;
                        runLength = (packet & 31) + 1;
                        ++src;
                    }

                    runLength += skipped - sx;
                    if (code == eRleControlOutline7)
                        src += sx - skipped;

                    unsigned int remaining = sw;
                    do {
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == eRleControlOutline7) {
                            unsigned int count = runLength;
                            do {
                                *out++ = palette[*src++];
                            } while (--count);
                        } else if (code == eRleControlOutline5 && flagcolor) {
                            unsigned int count = runLength;
                            do {
                                *out++ = flagcolor;
                            } while (--count);
                        } else {
                            out += runLength;
                        }
                        remaining -= runLength;
                        if (!remaining)
                            break;
                        packet = *src;
                        code = packet >> 5;
                        runLength = (packet & 31) + 1;
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
                    unsigned int runLength = (packet & 31) + 1;
                    ++src;

                    while (skipped + runLength <= static_cast<unsigned int>(sx)) {
                        skipped += runLength;
                        if (code == eRleControlOutline7)
                            src += runLength;
                        packet = *src;
                        code = packet >> 5;
                        runLength = (packet & 31) + 1;
                        ++src;
                    }

                    runLength += skipped - sx;
                    if (code == eRleControlOutline7)
                        src += sx - skipped;

                    unsigned int remaining = sw;
                    do {
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == eRleControlOutline7) {
                            unsigned int count = runLength;
                            do {
                                *--out = palette[*src++];
                            } while (--count);
                        } else if (code == eRleControlOutline5 && flagcolor) {
                            unsigned int count = runLength;
                            do {
                                *--out = flagcolor;
                            } while (--count);
                        } else {
                            out -= runLength;
                        }
                        remaining -= runLength;
                        if (!remaining)
                            break;
                        packet = *src;
                        code = packet >> 5;
                        runLength = (packet & 31) + 1;
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

// The identity comes from its caller, which passes the flag color 0 in slot
// 12 and hflip in slot 13; DrawAdvObjImpl takes hflip in slot 12 and its
// flag color last.

// Each row's cell source is computed before the skipped-pixel base
// `sx & ~31`, and the flag blend evaluates the flag term before the
// destination term, as in Loki's h3maped. DC records palette and
// aCellOffset as read-only pointers; lines 2462/2463 load the map table and
// bind pal+0x1c before either row loop. A single row cursor.
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

    assert(EncodingMethod == eEncodeAdvObjRLE);
    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, 0);
    if (sw > 0) {
        if (sh > 0) {

            assert(CroppedWidth % kCellWidth == 0);
            cellsPerLine = static_cast<unsigned int>(CroppedWidth) >> 5;
            cellOffset = static_cast<const unsigned short*>(static_cast<const void*>(map));

            palette = pal.Palette;

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
                    unsigned int runLength = (packet & 31) + 1;
                    ++src;

                    while (skipped + runLength <= static_cast<unsigned int>(sx)) {
                        skipped += runLength;
                        if (code == eRleControlOutline7)
                            src += runLength;
                        packet = *src;
                        code = packet >> 5;
                        runLength = (packet & 31) + 1;
                        ++src;
                    }

                    runLength += skipped - sx;
                    if (code == eRleControlOutline7)
                        src += sx - skipped;

                    unsigned int remaining = sw;
                    do {
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == eRleControlOutline7) {
                            unsigned int count = runLength;
                            do {
                                *out = (div2mask
                                        & (palette[*src++] >> 1))
                                     + (div2mask & (*out >> 1));
                                ++out;
                            } while (--count);
                        } else if (code == eRleControlOutline5 && flagcolor) {
                            unsigned int count = runLength;
                            do {
                                *out = (div2mask & (flagcolor >> 1))
                                     + (div2mask & (*out >> 1));
                                ++out;
                            } while (--count);
                        } else {
                            out += runLength;
                        }
                        remaining -= runLength;
                        if (!remaining)
                            break;
                        packet = *src;
                        code = packet >> 5;
                        runLength = (packet & 31) + 1;
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
                    unsigned int runLength = (packet & 31) + 1;
                    ++src;

                    while (skipped + runLength <= static_cast<unsigned int>(sx)) {
                        skipped += runLength;
                        if (code == eRleControlOutline7)
                            src += runLength;
                        packet = *src;
                        code = packet >> 5;
                        runLength = (packet & 31) + 1;
                        ++src;
                    }

                    runLength += skipped - sx;
                    if (code == eRleControlOutline7)
                        src += sx - skipped;

                    unsigned int remaining = sw;
                    do {
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == eRleControlOutline7) {
                            unsigned int count = runLength;
                            do {
                                --out;
                                *out = (div2mask
                                        & (palette[*src++] >> 1))
                                     + (div2mask & (*out >> 1));
                            } while (--count);
                        } else if (code == eRleControlOutline5 && flagcolor) {
                            unsigned int count = runLength;
                            do {
                                --out;
                                *out = (div2mask & (flagcolor >> 1))
                                     + (div2mask & (*out >> 1));
                            } while (--count);
                        } else {
                            out -= runLength;
                        }
                        remaining -= runLength;
                        if (!remaining)
                            break;
                        packet = *src;
                        code = packet >> 5;
                        runLength = (packet & 31) + 1;
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

    assert(EncodingMethod == eEncodeAdvObjRLE);
    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, 0);
    if (sw > 0) {
        if (sh > 0) {

            assert(CroppedWidth % kCellWidth == 0);
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
                    unsigned int runLength = (packet & 31) + 1;
                    ++src;

                    while (skipped + runLength <= static_cast<unsigned int>(sx)) {
                        skipped += runLength;
                        if (code == eRleControlOutline7)
                            src += runLength;
                        packet = *src;
                        code = packet >> 5;
                        runLength = (packet & 31) + 1;
                        ++src;
                    }

                    runLength += skipped - sx;
                    if (code == eRleControlOutline7)
                        src += sx - skipped;

                    unsigned int remaining = sw;
                    do {
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == eRleControlOutline7) {
                            out += runLength;
                            src += runLength;
                        } else {
                            switch (code) {
                            case eRleControlShadow75: {
                                unsigned int count = runLength;
                                do {
                                    *out = ((*out >> 1) & div2mask)
                                         + ((*out >> 2) & div4mask);
                                    ++out;
                                } while (--count);
                                break;
                            }
                            case eRleControlShadow50: {
                                unsigned int count = runLength;
                                do {
                                    unsigned int color = *out;
                                    *out = (color >> 1) & div2mask;
                                    ++out;
                                } while (--count);
                                break;
                            }
                            default:
                                out += runLength;
                                break;
                            }
                        }
                        remaining -= runLength;
                        if (!remaining)
                            break;
                        packet = *src;
                        code = packet >> 5;
                        runLength = (packet & 31) + 1;
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
                    unsigned int runLength = (packet & 31) + 1;
                    ++src;

                    while (skipped + runLength <= static_cast<unsigned int>(sx)) {
                        skipped += runLength;
                        if (code == eRleControlOutline7)
                            src += runLength;
                        packet = *src;
                        code = packet >> 5;
                        runLength = (packet & 31) + 1;
                        ++src;
                    }

                    runLength += skipped - sx;
                    if (code == eRleControlOutline7)
                        src += sx - skipped;

                    unsigned int remaining = sw;
                    do {
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == eRleControlOutline7) {
                            out -= runLength;
                            src += runLength;
                        } else {
                            switch (code) {
                            case eRleControlShadow75: {
                                unsigned int count = runLength;
                                do {
                                    --out;
                                    *out = ((*out >> 1) & div2mask)
                                         + ((*out >> 2) & div4mask);
                                } while (--count);
                                break;
                            }
                            case eRleControlShadow50: {
                                unsigned int count = runLength;
                                do {
                                    --out;
                                    *out = (*out >> 1) & div2mask;
                                } while (--count);
                                break;
                            }
                            default:
                                out -= runLength;
                                break;
                            }
                        }
                        remaining -= runLength;
                        if (!remaining)
                            break;
                        packet = *src;
                        code = packet >> 5;
                        runLength = (packet & 31) + 1;
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

// Four DC direction arms, the eight-statement Duff loop, raw do/while rows
// and indexed encoded for-rows. Retail and Dreamcast agree on the surprising
// general-RLE delegation `Draw(sw, sy, sw, ...)`. `pal.Palette` is loaded
// once, right after the line-table pointer and before the flip dispatch (as
// in Loki). DC attributes both raw-row advances to one line (2938); the
// destination row advances before the source row. DC's
// hFlipped/vFlipped/aLineOffset spellings are kept.
VA(0x0047dd40, 0xAD8)
DC_ADDRESS(0x076988, 0x762)
MAC_ADDRESS(0x08cf24, 0x834) // retail raw/tileset decoder + DC source identity
void CSpriteFrame::DrawTile(int sx, int sy, int sw, int sh, unsigned short* dst,
                            int dx, int dy, int dw, int dh, int dpitch,
                            TPalette16& pal, bool hFlipped,
                            bool vFlipped) const
{
    static const unsigned char kOpaqueRunCode = 7;

    if (EncodingMethod == eEncodeGeneralRLE) {
        Draw(sw, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hFlipped, 1);
        return;
    }
    if (EncodingMethod == eEncodeAdvObjRLE) {
        DrawAdvObjImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hFlipped,
                       0);
        return;
    }

    assert(EncodingMethod == eEncodeTilesetRLE || EncodingMethod == eEncodeRaw);
    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hFlipped, vFlipped);
    if (sw > 0) {
        if (sh > 0) {
            const unsigned short* const aLineOffset = static_cast<const unsigned short*>(
                static_cast<const void*>(map));
            const unsigned short* const palette = pal.Palette;
            if (!vFlipped) {
                if (!hFlipped) {
                    unsigned short* lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(dst)) +
                        dy * dpitch + dx * 2));

                    if (EncodingMethod == eEncodeRaw) {
                        unsigned char* line = map + sy * Pitch + sx;
                        do {
                            unsigned short* out = lineDst;
                            unsigned char* src = line;
                            int remaining = sw;
                            switch (remaining & 7) {
                                do {
                                case eRawRowUnroll8:
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
                            const unsigned char* src = map + aLineOffset[y];
                            unsigned int skipped = 0;
                            unsigned char packet = *src;
                            unsigned char code = packet >> 5;
                            unsigned int runLength = (packet & 31) + 1;
                            ++src;

                            while (skipped + runLength <=
                                   static_cast<unsigned int>(sx)) {
                                skipped += runLength;
                                if (code == kOpaqueRunCode)
                                    src += runLength;
                                packet = *src;
                                code = packet >> 5;
                                runLength = (packet & 31) + 1;
                                ++src;
                            }
                            runLength += skipped - sx;
                            if (code == kOpaqueRunCode)
                                src += sx - skipped;

                            unsigned int remaining = sw;
                            do {
                                assert(runLength > 0);
                                if (runLength > remaining)
                                    runLength = remaining;
                                if (code == kOpaqueRunCode) {
                                    unsigned int count = runLength;
                                    do {
                                        *out++ = palette[*src++];
                                    } while (--count);
                                } else {
                                    out += runLength;
                                }
                                remaining -= runLength;
                                if (!remaining)
                                    break;
                                packet = *src;
                                code = packet >> 5;
                                runLength = (packet & 31) + 1;
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
                        unsigned char* line = map + sy * Pitch + sx;
                        do {
                            unsigned short* out = lineDst;
                            unsigned char* src = line;
                            int remaining = sw;
                            switch (remaining & 7) {
                                do {
                                case eRawRowUnroll8:
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
                            const unsigned char* src = map + aLineOffset[y];
                            unsigned int skipped = 0;
                            unsigned char packet = *src;
                            unsigned char code = packet >> 5;
                            unsigned int runLength = (packet & 31) + 1;
                            ++src;

                            while (skipped + runLength <=
                                   static_cast<unsigned int>(sx)) {
                                skipped += runLength;
                                if (code == kOpaqueRunCode)
                                    src += runLength;
                                packet = *src;
                                code = packet >> 5;
                                runLength = (packet & 31) + 1;
                                ++src;
                            }
                            runLength += skipped - sx;
                            if (code == kOpaqueRunCode)
                                src += sx - skipped;

                            unsigned int remaining = sw;
                            do {
                                assert(runLength > 0);
                                if (runLength > remaining)
                                    runLength = remaining;
                                if (code == kOpaqueRunCode) {
                                    unsigned int count = runLength;
                                    do {
                                        *--out = palette[*src++];
                                    } while (--count);
                                } else {
                                    out -= runLength;
                                }
                                remaining -= runLength;
                                if (!remaining)
                                    break;
                                packet = *src;
                                code = packet >> 5;
                                runLength = (packet & 31) + 1;
                                ++src;
                            } while (remaining);
                            lineDst = static_cast<unsigned short*>(static_cast<void*>(
                                static_cast<unsigned char*>(static_cast<void*>(lineDst)) + dpitch));
                        }
                    }
                }
            } else {
                if (!hFlipped) {
                    unsigned short* lineDst =
                        static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(dst)) +
                        (dy + sh - 1) * dpitch + dx * 2));

                    if (EncodingMethod == eEncodeRaw) {
                        unsigned char* line = map + sy * Pitch + sx;
                        do {
                            unsigned short* out = lineDst;
                            unsigned char* src = line;
                            int remaining = sw;
                            switch (remaining & 7) {
                                do {
                                case eRawRowUnroll8:
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
                            const unsigned char* src = map + aLineOffset[y];
                            unsigned int skipped = 0;
                            unsigned char packet = *src;
                            unsigned char code = packet >> 5;
                            unsigned int runLength = (packet & 31) + 1;
                            ++src;

                            while (skipped + runLength <=
                                   static_cast<unsigned int>(sx)) {
                                skipped += runLength;
                                if (code == kOpaqueRunCode)
                                    src += runLength;
                                packet = *src;
                                code = packet >> 5;
                                runLength = (packet & 31) + 1;
                                ++src;
                            }
                            runLength += skipped - sx;
                            if (code == kOpaqueRunCode)
                                src += sx - skipped;

                            unsigned int remaining = sw;
                            do {
                                assert(runLength > 0);
                                if (runLength > remaining)
                                    runLength = remaining;
                                if (code == kOpaqueRunCode) {
                                    unsigned int count = runLength;
                                    do {
                                        *out++ = palette[*src++];
                                    } while (--count);
                                } else {
                                    out += runLength;
                                }
                                remaining -= runLength;
                                if (!remaining)
                                    break;
                                packet = *src;
                                code = packet >> 5;
                                runLength = (packet & 31) + 1;
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
                        unsigned char* line = map + sy * Pitch + sx;
                        do {
                            unsigned short* out = lineDst;
                            unsigned char* src = line;
                            int remaining = sw;
                            switch (remaining & 7) {
                                do {
                                case eRawRowUnroll8:
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
                            const unsigned char* src = map + aLineOffset[y];
                            unsigned int skipped = 0;
                            unsigned char packet = *src;
                            unsigned char code = packet >> 5;
                            unsigned int runLength = (packet & 31) + 1;
                            ++src;

                            while (skipped + runLength <=
                                   static_cast<unsigned int>(sx)) {
                                skipped += runLength;
                                if (code == kOpaqueRunCode)
                                    src += runLength;
                                packet = *src;
                                code = packet >> 5;
                                runLength = (packet & 31) + 1;
                                ++src;
                            }
                            runLength += skipped - sx;
                            if (code == kOpaqueRunCode)
                                src += sx - skipped;

                            unsigned int remaining = sw;
                            do {
                                assert(runLength > 0);
                                if (runLength > remaining)
                                    runLength = remaining;
                                if (code == kOpaqueRunCode) {
                                    unsigned int count = runLength;
                                    do {
                                        *--out = palette[*src++];
                                    } while (--count);
                                } else {
                                    out -= runLength;
                                }
                                remaining -= runLength;
                                if (!remaining)
                                    break;
                                packet = *src;
                                code = packet >> 5;
                                runLength = (packet & 31) + 1;
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

// Only two blends exist: codes 1 and 2 take three quarters of the
// destination (`(p>>2)&div4mask + (p>>1)&div2mask`), 3 and 4 take a half,
// code 7's opaque run is skipped over rather than drawn, and every other
// code just advances.

// A single row cursor. The three-quarter blend evaluates its div2mask term
// first, as Loki's h3maped does; div4mask's placement (and so its AND
// width) depends on EncodeGeneral's dynamically initialized kMaxRunLength in
// the .bss. The half blend is written `--out; *out = (*out >> 1) & mask;`,
// reading the same location after the decrement. DC decoder/helper scopes
// and pixel arithmetic remain intact, including reverse/raw source walks.
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

    assert(EncodingMethod == eEncodeTilesetRLE);
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
                        unsigned int runLength = (packet & 31) + 1;
                        ++src;

                        while (skipped + runLength <=
                               static_cast<unsigned int>(sx)) {
                            skipped += runLength;
                            if (code == kOpaqueRunCode)
                                src += runLength;
                            packet = *src;
                            code = packet >> 5;
                            runLength = (packet & 31) + 1;
                            ++src;
                        }
                        runLength += skipped - sx;
                        if (code == kOpaqueRunCode)
                            src += sx - skipped;

                        unsigned int remaining = sw;
                        do {
                            assert(runLength > 0);
                            if (runLength > remaining)
                                runLength = remaining;
                            if (code == kOpaqueRunCode) {
                                out += runLength;
                                src += runLength;
                            } else {
                                switch (code) {
                                case eRleControlShadow75:
                                case eRleControlShadow2: {
                                    unsigned int count = runLength;
                                    do {
                                        *out = ((*out >> 1) & div2mask)
                                             + ((*out >> 2) & div4mask);
                                        ++out;
                                    } while (--count);
                                    break;
                                }
                                case eRleControlShadow3:
                                case eRleControlShadow50: {
                                    unsigned int count = runLength;
                                    do {
                                        unsigned int color = *out;
                                        *out = (color >> 1) & div2mask;
                                        ++out;
                                    } while (--count);
                                    break;
                                }
                                default:
                                    out += runLength;
                                    break;
                                }
                            }
                            remaining -= runLength;
                            if (!remaining)
                                break;
                            packet = *src;
                            code = packet >> 5;
                            runLength = (packet & 31) + 1;
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
                        unsigned int runLength = (packet & 31) + 1;
                        ++src;

                        while (skipped + runLength <=
                               static_cast<unsigned int>(sx)) {
                            skipped += runLength;
                            if (code == kOpaqueRunCode)
                                src += runLength;
                            packet = *src;
                            code = packet >> 5;
                            runLength = (packet & 31) + 1;
                            ++src;
                        }
                        runLength += skipped - sx;
                        if (code == kOpaqueRunCode)
                            src += sx - skipped;

                        unsigned int remaining = sw;
                        do {
                            assert(runLength > 0);
                            if (runLength > remaining)
                                runLength = remaining;
                            if (code == kOpaqueRunCode) {
                                out -= runLength;
                                src += runLength;
                            } else {
                                switch (code) {
                                case eRleControlShadow75:
                                case eRleControlShadow2: {
                                    unsigned int count = runLength;
                                    do {
                                        --out;
                                        *out = ((*out >> 1) & div2mask)
                                             + ((*out >> 2) & div4mask);
                                    } while (--count);
                                    break;
                                }
                                case eRleControlShadow3:
                                case eRleControlShadow50: {
                                    unsigned int count = runLength;
                                    do {
                                        --out;
                                        *out = (*out >> 1) & div2mask;
                                    } while (--count);
                                    break;
                                }
                                default:
                                    out -= runLength;
                                    break;
                                }
                            }
                            remaining -= runLength;
                            if (!remaining)
                                break;
                            packet = *src;
                            code = packet >> 5;
                            runLength = (packet & 31) + 1;
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
                        unsigned int runLength = (packet & 31) + 1;
                        ++src;

                        while (skipped + runLength <=
                               static_cast<unsigned int>(sx)) {
                            skipped += runLength;
                            if (code == kOpaqueRunCode)
                                src += runLength;
                            packet = *src;
                            code = packet >> 5;
                            runLength = (packet & 31) + 1;
                            ++src;
                        }
                        runLength += skipped - sx;
                        if (code == kOpaqueRunCode)
                            src += sx - skipped;

                        unsigned int remaining = sw;
                        do {
                            assert(runLength > 0);
                            if (runLength > remaining)
                                runLength = remaining;
                            if (code == kOpaqueRunCode) {
                                out += runLength;
                                src += runLength;
                            } else {
                                switch (code) {
                                case eRleControlShadow75:
                                case eRleControlShadow2: {
                                    unsigned int count = runLength;
                                    do {
                                        *out = ((*out >> 1) & div2mask)
                                             + ((*out >> 2) & div4mask);
                                        ++out;
                                    } while (--count);
                                    break;
                                }
                                case eRleControlShadow3:
                                case eRleControlShadow50: {
                                    unsigned int count = runLength;
                                    do {
                                        unsigned int color = *out;
                                        *out = (color >> 1) & div2mask;
                                        ++out;
                                    } while (--count);
                                    break;
                                }
                                default:
                                    out += runLength;
                                    break;
                                }
                            }
                            remaining -= runLength;
                            if (!remaining)
                                break;
                            packet = *src;
                            code = packet >> 5;
                            runLength = (packet & 31) + 1;
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
                        unsigned int runLength = (packet & 31) + 1;
                        ++src;

                        while (skipped + runLength <=
                               static_cast<unsigned int>(sx)) {
                            skipped += runLength;
                            if (code == kOpaqueRunCode)
                                src += runLength;
                            packet = *src;
                            code = packet >> 5;
                            runLength = (packet & 31) + 1;
                            ++src;
                        }
                        runLength += skipped - sx;
                        if (code == kOpaqueRunCode)
                            src += sx - skipped;

                        unsigned int remaining = sw;
                        do {
                            assert(runLength > 0);
                            if (runLength > remaining)
                                runLength = remaining;
                            if (code == kOpaqueRunCode) {
                                out -= runLength;
                                src += runLength;
                            } else {
                                switch (code) {
                                case eRleControlShadow75:
                                case eRleControlShadow2: {
                                    unsigned int count = runLength;
                                    do {
                                        --out;
                                        *out = ((*out >> 1) & div2mask)
                                             + ((*out >> 2) & div4mask);
                                    } while (--count);
                                    break;
                                }
                                case eRleControlShadow3:
                                case eRleControlShadow50: {
                                    unsigned int count = runLength;
                                    do {
                                        --out;
                                        *out = (*out >> 1) & div2mask;
                                    } while (--count);
                                    break;
                                }
                                default:
                                    out -= runLength;
                                    break;
                                }
                            }
                            remaining -= runLength;
                            if (!remaining)
                                break;
                            packet = *src;
                            code = packet >> 5;
                            runLength = (packet & 31) + 1;
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

// Both delegations follow Draw: the tileset/raw encodings go to DrawTile
// with a trailing 0, the adventure-object encoding to
// DrawAdvObjWithFlagAlpha. The delegated member is spelled `Draw`, as in DC
// (name-keyed TU state, docs/vc6/behavior-catalog.md C12).
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

    assert(EncodingMethod == eEncodeGeneralRLE);
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
        palette = pal.Palette;
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
                unsigned int runLength = *src++ + 1;
                assert(runLength > 0);

                while (skipped + runLength <= static_cast<unsigned int>(sx)) {
                    skipped += runLength;
                    if (code == kOpaqueRunCode)
                        src += runLength;
                    code = *src++;
                    runLength = *src++ + 1;
                }

                runLength += skipped - sx;
                if (code == kOpaqueRunCode)
                    src += sx - skipped;

                unsigned int remaining = sw;
                do {
                    assert(runLength > 0);
                    if (runLength > remaining)
                        runLength = remaining;
                    if (code == kOpaqueRunCode) {
                        unsigned int count = runLength;
                        do {
                            *out = (div2mask
                                    & (palette[*src++] >> 1))
                                 + (div2mask & (*out >> 1));
                            ++out;
                        } while (--count);
                    } else {
                        out += runLength;
                    }
                    remaining -= runLength;
                    if (!remaining)
                        break;
                    code = *src++;
                    runLength = *src++ + 1;
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
                unsigned int runLength = *src++ + 1;
                assert(runLength > 0);

                while (skipped + runLength <= static_cast<unsigned int>(sx)) {
                    skipped += runLength;
                    if (code == kOpaqueRunCode)
                        src += runLength;
                    code = *src++;
                    runLength = *src++ + 1;
                }

                runLength += skipped - sx;
                if (code == kOpaqueRunCode)
                    src += sx - skipped;

                unsigned int remaining = sw;
                do {
                    assert(runLength > 0);
                    if (runLength > remaining)
                        runLength = remaining;
                    if (code == kOpaqueRunCode) {
                        unsigned int count = runLength;
                        do {
                            --out;
                            *out = (div2mask
                                    & (palette[*src++] >> 1))
                                 + (div2mask & (*out >> 1));
                        } while (--count);
                    } else {
                        out -= runLength;
                    }
                    remaining -= runLength;
                    if (!remaining)
                        break;
                    code = *src++;
                    runLength = *src++ + 1;
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

// Loki's row/packet model with DC's typed pixel-row cursor, unsigned cell
// division, separate packet reads and a byte skip flag; the tail tests are
// positive (`count > 0`) where Loki writes `count != 0`.
// Original: CSpriteFrame::DrawAdvObjWithFlagScaled50; cspriteframe.cpp:4054
DC_ADDRESS(0x077b6c, 0x1ea)
void CSpriteFrame::DrawAdvObjWithFlagScaled50(int sx, int sy, int sw, int sh,
                                              unsigned short* dst, int dx, int dy,
                                              int dw, int dh, int dpitch,
                                              TPalette16& pal,
                                              unsigned short flagcolor) const
{
    assert(EncodingMethod == eEncodeAdvObjRLE);
    ClipScaled50(sx, sy, sw, sh, dx, dy, dw, dh, false, false);
    if (sw <= 0 || sh <= 0)
        return;

    assert(CroppedWidth % kCellWidth == 0);
    unsigned int cellsPerLine = CroppedWidth / kCellWidth;
    const unsigned short* const aCellOffset =
        static_cast<const unsigned short*>(static_cast<const void*>(map));
    const unsigned short* const palette = pal.Palette;
    unsigned short* lineDst = static_cast<unsigned short*>(static_cast<void*>(
        static_cast<unsigned char*>(static_cast<void*>(dst))
        + dy * dpitch + dx * sizeof(unsigned short)));
    for (int y = sy; y < sy + sh; y += 2) {
        unsigned short* out = lineDst;
        unsigned char* src = map + aCellOffset[y * cellsPerLine + sx / kCellWidth];
        unsigned char code;
        unsigned int runLength;
        unsigned int skipped = sx & ~(kCellWidth - 1);
        for (;;) {
            code = *src >> 5;
            runLength = (*src & 31) + 1;
            src++;
            if (skipped + runLength > sx) {
                runLength -= sx - skipped;
                if (code == ePackedRleLiteral)
                    src += sx - skipped;
                skipped = sx;
                break;
            }
            skipped += runLength;
            if (code == ePackedRleLiteral)
                src += runLength;
        }

        bool skip = false;
        unsigned int remaining = sw;
        for (;;) {
            assert(runLength > 0);
            if (runLength > remaining)
                runLength = remaining;
            if (code == ePackedRleLiteral) {
                unsigned int count = runLength;
                if (skip) {
                    src++;
                    count--;
                }
                while (count > 1) {
                    *out++ = palette[*src];
                    src += 2;
                    count -= 2;
                }
                skip = count > 0;
                if (skip)
                    *out++ = palette[*src++];
            }
            else {
                switch (code) {
                case eRleControlOutline5:
                    if (flagcolor != 0) {
                        unsigned int count = runLength;
                        if (skip)
                            count--;
                        while (count > 1) {
                            *out++ = flagcolor;
                            count -= 2;
                        }
                        skip = count > 0;
                        if (skip)
                            *out++ = flagcolor;
                        break;
                    }
                default: {
                    unsigned int count = runLength;
                    if (skip)
                        count--;
                    out += (count + 1) / 2;
                    skip = count & 1;
                    break;
                }
                }
            }
            remaining -= runLength;
            if (remaining == 0)
                break;
            code = *src >> 5;
            runLength = (*src & 31) + 1;
            src++;
        }
        lineDst = static_cast<unsigned short*>(static_cast<void*>(
            static_cast<unsigned char*>(static_cast<void*>(lineDst))
            + dpitch));
    }
}

// The same Loki row/packet model with the positive tail tests the bytes
// require. Residual (99.96%): two div4mask AND widths; native editor
// storage is four-aligned, while the current local-static allocation places
// the ushort at 2 mod 4.
// Preserve the proven mask type and encoder statics (the game layout agrees).
// Original: CSpriteFrame::DrawAdvObjShadowScaled50; cspriteframe.cpp:4187
DC_ADDRESS(0x077d58, 0x240)
void CSpriteFrame::DrawAdvObjShadowScaled50(int sx, int sy, int sw, int sh,
                                            unsigned short* dst, int dx, int dy,
                                            int dw, int dh, int dpitch,
                                            TPalette16& pal) const
{
    assert(EncodingMethod == eEncodeAdvObjRLE);
    ClipScaled50(sx, sy, sw, sh, dx, dy, dw, dh, false, false);
    if (sw <= 0 || sh <= 0)
        return;

    assert(CroppedWidth % kCellWidth == 0);
    unsigned int cellsPerLine = CroppedWidth / kCellWidth;
    const unsigned short* const aCellOffset =
        static_cast<const unsigned short*>(static_cast<const void*>(map));
    unsigned short* lineDst = static_cast<unsigned short*>(static_cast<void*>(
        static_cast<unsigned char*>(static_cast<void*>(dst))
        + dy * dpitch + dx * sizeof(unsigned short)));
    for (int y = sy; y < sy + sh; y += 2) {
        unsigned short* out = lineDst;
        unsigned char* src = map + aCellOffset[y * cellsPerLine + sx / kCellWidth];
        unsigned char code;
        unsigned int runLength;
        unsigned int skipped = sx & ~(kCellWidth - 1);
        for (;;) {
            code = *src >> 5;
            runLength = (*src & 31) + 1;
            src++;
            if (skipped + runLength > sx) {
                runLength -= sx - skipped;
                if (code == ePackedRleLiteral)
                    src += sx - skipped;
                skipped = sx;
                break;
            }
            skipped += runLength;
            if (code == ePackedRleLiteral)
                src += runLength;
        }

        bool skip = false;
        unsigned int remaining = sw;
        for (;;) {
            assert(runLength > 0);
            if (runLength > remaining)
                runLength = remaining;
            if (code == ePackedRleLiteral) {
                unsigned int count = runLength;
                if (skip)
                    count--;
                out += (count + 1) / 2;
                src += runLength;
                skip = count & 1;
            }
            else {
                switch (code) {
                case eRleControlShadow75: {
                    unsigned int count = runLength;
                    if (skip)
                        count--;
                    while (count > 1) {
                        *out = ((*out >> 1) & div2mask)
                                               + ((*out >> 2) & div4mask);
                        out++;
                        count -= 2;
                    }
                    skip = count > 0;
                    if (skip) {
                        *out = ((*out >> 1) & div2mask)
                                               + ((*out >> 2) & div4mask);
                        out++;
                    }
                    break;
                }
                case eRleControlShadow50: {
                    unsigned int count = runLength;
                    if (skip)
                        count--;
                    while (count > 1) {
                        *out = (*out >> 1) & div2mask;
                        out++;
                        count -= 2;
                    }
                    skip = count > 0;
                    if (skip) {
                        *out = (*out >> 1) & div2mask;
                        out++;
                    }
                    break;
                }
                default: {
                    unsigned int count = runLength;
                    if (skip)
                        count--;
                    out += (count + 1) / 2;
                    skip = count & 1;
                    break;
                }
                }
            }
            remaining -= runLength;
            if (remaining == 0)
                break;
            code = *src >> 5;
            runLength = (*src & 31) + 1;
            src++;
        }
        lineDst = static_cast<unsigned short*>(static_cast<void*>(
            static_cast<unsigned char*>(static_cast<void*>(lineDst))
            + dpitch));
    }
}

// Loki's row/RLE cursors and four flip branches, with byte skip flags and
// positive tail tests (Loki writes `count != 0`).
// Original: CSpriteFrame::DrawTileScaled50; cspriteframe.cpp:4330
DC_ADDRESS(0x077f98, 0x606)
void CSpriteFrame::DrawTileScaled50(int sx, int sy, int sw, int sh,
                                    unsigned short* dst, int dx, int dy,
                                    int dw, int dh, int dpitch,
                                    TPalette16& pal, bool hFlipped, bool vFlipped) const
{
    assert(EncodingMethod == eEncodeTilesetRLE || EncodingMethod == eEncodeRaw);
    ClipScaled50(sx, sy, sw, sh, dx, dy, dw, dh, hFlipped, vFlipped);
    if (sw <= 0 || sh <= 0)
        return;

    const unsigned short* const aLineOffset =
        static_cast<const unsigned short*>(static_cast<const void*>(map));
    const unsigned short* const palette = pal.Palette;
    if (!vFlipped) {
        if (!hFlipped) {
            unsigned short* lineDst = static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + dy * dpitch + dx * sizeof(unsigned short)));
            if (EncodingMethod == eEncodeRaw) {
                unsigned char* line = map + sy * Pitch + sx;
                do {
                    unsigned short* out = lineDst;
                    unsigned char* src = line;
                    int remaining = sw;
                    do {
                        *out++ = palette[*src];
                        src += 2;
                        remaining -= 2;
                    } while (remaining > 0);
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst))
                        + dpitch));
                    line += Pitch * 2;
                    sh -= 2;
                } while (sh > 0);
            }
            else {
                for (int y = sy; y < sy + sh; y += 2) {
                    unsigned short* out = lineDst;
                    unsigned char* src = map + aLineOffset[y];
                    unsigned char code;
                    unsigned int runLength;
                    unsigned int skipped = 0;
                    for (;;) {
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                        if (skipped + runLength > sx) {
                            runLength -= sx - skipped;
                            if (code == ePackedRleLiteral)
                                src += sx - skipped;
                            skipped = sx;
                            break;
                        }
                        skipped += runLength;
                        if (code == ePackedRleLiteral)
                            src += runLength;
                    }

                    bool skip = false;
                    unsigned int remaining = sw;
                    for (;;) {
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == ePackedRleLiteral) {
                            unsigned int count = runLength;
                            if (skip) {
                                src++;
                                count--;
                            }
                            while (count > 1) {
                                *out++ = palette[*src];
                                src += 2;
                                count -= 2;
                            }
                            skip = count > 0;
                            if (skip)
                                *out++ = palette[*src++];
                        }
                        else {
                            unsigned int count = runLength;
                            if (skip)
                                count--;
                            out += (count + 1) / 2;
                            skip = count & 1;
                        }
                        remaining -= runLength;
                        if (remaining == 0)
                            break;
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                    }
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst))
                        + dpitch));
                }
            }
        }
        else {
            unsigned short* lineDst = static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + dy * dpitch + dx * sizeof(unsigned short) + (sw >> 1) * sizeof(unsigned short)));
            if (EncodingMethod == eEncodeRaw) {
                unsigned char* line = map + sy * Pitch + sx;
                do {
                    unsigned short* out = lineDst;
                    unsigned char* src = line;
                    int remaining = sw;
                    do {
                        *--out = palette[*src];
                        src += 2;
                        remaining -= 2;
                    } while (remaining > 0);
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst))
                        + dpitch));
                    line += Pitch * 2;
                    sh -= 2;
                } while (sh > 0);
            }
            else {
                for (int y = sy; y < sy + sh; y += 2) {
                    unsigned short* out = lineDst;
                    unsigned char* src = map + aLineOffset[y];
                    unsigned char code;
                    unsigned int runLength;
                    unsigned int skipped = 0;
                    for (;;) {
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                        if (skipped + runLength > sx) {
                            runLength -= sx - skipped;
                            if (code == ePackedRleLiteral)
                                src += sx - skipped;
                            skipped = sx;
                            break;
                        }
                        skipped += runLength;
                        if (code == ePackedRleLiteral)
                            src += runLength;
                    }

                    bool skip = false;
                    unsigned int remaining = sw;
                    for (;;) {
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == ePackedRleLiteral) {
                            unsigned int count = runLength;
                            if (skip) {
                                src++;
                                count--;
                            }
                            while (count > 1) {
                                *--out = palette[*src];
                                src += 2;
                                count -= 2;
                            }
                            skip = count > 0;
                            if (skip)
                                *--out = palette[*src++];
                        }
                        else {
                            unsigned int count = runLength;
                            if (skip)
                                count--;
                            out -= (count + 1) / 2;
                            skip = count & 1;
                        }
                        remaining -= runLength;
                        if (remaining == 0)
                            break;
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                    }
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst))
                        + dpitch));
                }
            }
        }
    }
    else {
        if (!hFlipped) {
            unsigned short* lineDst = static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + (dy + (sh >> 1) - 1) * dpitch + dx * sizeof(unsigned short)));
            if (EncodingMethod == eEncodeRaw) {
                unsigned char* line = map + sy * Pitch + sx;
                do {
                    unsigned short* out = lineDst;
                    unsigned char* src = line;
                    int remaining = sw;
                    do {
                        *out++ = palette[*src];
                        src += 2;
                        remaining -= 2;
                    } while (remaining > 0);
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst))
                        - dpitch));
                    line += Pitch * 2;
                    sh -= 2;
                } while (sh > 0);
            }
            else {
                for (int y = sy; y < sy + sh; y += 2) {
                    unsigned short* out = lineDst;
                    unsigned char* src = map + aLineOffset[y];
                    unsigned char code;
                    unsigned int runLength;
                    unsigned int skipped = 0;
                    for (;;) {
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                        if (skipped + runLength > sx) {
                            runLength -= sx - skipped;
                            if (code == ePackedRleLiteral)
                                src += sx - skipped;
                            skipped = sx;
                            break;
                        }
                        skipped += runLength;
                        if (code == ePackedRleLiteral)
                            src += runLength;
                    }

                    bool skip = false;
                    unsigned int remaining = sw;
                    for (;;) {
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == ePackedRleLiteral) {
                            unsigned int count = runLength;
                            if (skip) {
                                src++;
                                count--;
                            }
                            while (count > 1) {
                                *out++ = palette[*src];
                                src += 2;
                                count -= 2;
                            }
                            skip = count > 0;
                            if (skip)
                                *out++ = palette[*src++];
                        }
                        else {
                            unsigned int count = runLength;
                            if (skip)
                                count--;
                            out += (count + 1) / 2;
                            skip = count & 1;
                        }
                        remaining -= runLength;
                        if (remaining == 0)
                            break;
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                    }
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst))
                        - dpitch));
                }
            }
        }
        else {
            unsigned short* lineDst = static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + (dy + (sh >> 1) - 1) * dpitch + dx * sizeof(unsigned short) + (sw >> 1) * sizeof(unsigned short)));
            if (EncodingMethod == eEncodeRaw) {
                unsigned char* line = map + sy * Pitch + sx;
                do {
                    unsigned short* out = lineDst;
                    unsigned char* src = line;
                    int remaining = sw;
                    do {
                        *--out = palette[*src];
                        src += 2;
                        remaining -= 2;
                    } while (remaining > 0);
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst))
                        - dpitch));
                    line += Pitch * 2;
                    sh -= 2;
                } while (sh > 0);
            }
            else {
                for (int y = sy; y < sy + sh; y += 2) {
                    unsigned short* out = lineDst;
                    unsigned char* src = map + aLineOffset[y];
                    unsigned char code;
                    unsigned int runLength;
                    unsigned int skipped = 0;
                    for (;;) {
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                        if (skipped + runLength > sx) {
                            runLength -= sx - skipped;
                            if (code == ePackedRleLiteral)
                                src += sx - skipped;
                            skipped = sx;
                            break;
                        }
                        skipped += runLength;
                        if (code == ePackedRleLiteral)
                            src += runLength;
                    }

                    bool skip = false;
                    unsigned int remaining = sw;
                    for (;;) {
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == ePackedRleLiteral) {
                            unsigned int count = runLength;
                            if (skip) {
                                src++;
                                count--;
                            }
                            while (count > 1) {
                                *--out = palette[*src];
                                src += 2;
                                count -= 2;
                            }
                            skip = count > 0;
                            if (skip)
                                *--out = palette[*src++];
                        }
                        else {
                            unsigned int count = runLength;
                            if (skip)
                                count--;
                            out -= (count + 1) / 2;
                            skip = count & 1;
                        }
                        remaining -= runLength;
                        if (remaining == 0)
                            break;
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                    }
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst))
                        - dpitch));
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

// Loki CSpriteFrame.cpp's quarter-size renderer supplies the packet/switch
// model; the tests are the positive `> 0` forms where Loki writes
// `count != 0` and `(count & 3) ? ...`. DC 5118..5119 record the remainder
// and its four-complement select on consecutive lines; Loki's single
// conditional statement is kept.
// Original: CSpriteFrame::DrawAdvObjWithFlagScaled25; cspriteframe.cpp:4907
DC_ADDRESS(0x078774, 0x208)
void CSpriteFrame::DrawAdvObjWithFlagScaled25(int sx, int sy, int sw, int sh,
                                              unsigned short* dst, int dx, int dy,
                                              int dw, int dh, int dpitch,
                                              TPalette16& pal,
                                              unsigned short flagcolor) const
{
    assert(EncodingMethod == eEncodeAdvObjRLE);
    ClipScaled25(sx, sy, sw, sh, dx, dy, dw, dh, false, false);
    if (sw <= 0 || sh <= 0)
        return;

    assert(CroppedWidth % kCellWidth == 0);
    unsigned int cellsPerLine = CroppedWidth / kCellWidth;
    const unsigned short* const aCellOffset =
        static_cast<const unsigned short*>(static_cast<const void*>(map));
    const unsigned short* const palette = pal.Palette;
    unsigned short* lineDst = static_cast<unsigned short*>(static_cast<void*>(
        static_cast<unsigned char*>(static_cast<void*>(dst))
        + dy * dpitch + dx * sizeof(unsigned short)));
    for (int y = sy; y < sy + sh; y += 4) {
        unsigned short* out = lineDst;
        unsigned char* src = map + aCellOffset[y * cellsPerLine + sx / kCellWidth];
        unsigned char code;
        unsigned int runLength;
        unsigned int skipped = sx & ~(kCellWidth - 1);
        for (;;) {
            code = *src >> 5;
            runLength = (*src & 31) + 1;
            src++;
            if (skipped + runLength > sx) {
                runLength -= sx - skipped;
                if (code == ePackedRleLiteral)
                    src += sx - skipped;
                skipped = sx;
                break;
            }
            skipped += runLength;
            if (code == ePackedRleLiteral)
                src += runLength;
        }

        unsigned int skip = 0;
        unsigned int remaining = sw;
        for (;;) {
            assert(runLength > 0);
            if (runLength > remaining)
                runLength = remaining;
            if (code == ePackedRleLiteral) {
                if (skip < runLength) {
                    unsigned int count = runLength - skip;
                    src += skip;
                    while (count > 3) {
                        *out++ = palette[*src];
                        src += 4;
                        count -= 4;
                    }
                    if (count > 0) {
                        *out++ = palette[*src];
                        src += count;
                        skip = 4 - count;
                    }
                    else {
                        skip = 0;
                    }
                }
                else {
                    src += runLength;
                    skip -= runLength;
                }
            }
            else if (skip < runLength) {
                switch (code) {
                case eRleControlOutline5:
                    if (flagcolor != 0) {
                        unsigned int count = runLength - skip;
                        while (count > 3) {
                            *out++ = flagcolor;
                            // DC 5012 and both retail ports retain this decrement.
                            count -= 2;
                        }
                        if (count > 0) {
                            *out++ = flagcolor;
                            skip = 4 - count;
                        }
                        else {
                            skip = 0;
                        }
                        break;
                    }
                default: {
                    unsigned int count = runLength - skip;
                    out += (count + 3) / 4;
                    skip = (count & 3) > 0 ? 4 - (count & 3) : 0;
                    break;
                }
                }
            }
            else {
                skip -= runLength;
            }
            remaining -= runLength;
            if (remaining == 0)
                break;
            code = *src >> 5;
            runLength = (*src & 31) + 1;
            src++;
        }
        lineDst = static_cast<unsigned short*>(static_cast<void*>(
            static_cast<unsigned char*>(static_cast<void*>(lineDst))
            + dpitch));
    }
}

// Loki packet traversal with the positive count/remainder tests the bytes
// require. Residual (99.96%): as in the half-size shadow pass, only two
// div4mask AND widths differ because existing local-static storage is
// ordered differently.
// Original: CSpriteFrame::DrawAdvObjShadowScaled25; cspriteframe.cpp:5054
DC_ADDRESS(0x07897c, 0x262)
void CSpriteFrame::DrawAdvObjShadowScaled25(int sx, int sy, int sw, int sh,
                                            unsigned short* dst, int dx, int dy,
                                            int dw, int dh, int dpitch,
                                            TPalette16& pal) const
{
    assert(EncodingMethod == eEncodeAdvObjRLE);
    ClipScaled25(sx, sy, sw, sh, dx, dy, dw, dh, false, false);
    if (sw <= 0 || sh <= 0)
        return;

    assert(CroppedWidth % kCellWidth == 0);
    unsigned int cellsPerLine = CroppedWidth / kCellWidth;
    const unsigned short* const aCellOffset =
        static_cast<const unsigned short*>(static_cast<const void*>(map));
    unsigned short* lineDst = static_cast<unsigned short*>(static_cast<void*>(
        static_cast<unsigned char*>(static_cast<void*>(dst))
        + dy * dpitch + dx * sizeof(unsigned short)));
    for (int y = sy; y < sy + sh; y += 4) {
        unsigned short* out = lineDst;
        unsigned char* src = map + aCellOffset[y * cellsPerLine + sx / kCellWidth];
        unsigned char code;
        unsigned int runLength;
        unsigned int skipped = sx & ~(kCellWidth - 1);
        for (;;) {
            code = *src >> 5;
            runLength = (*src & 31) + 1;
            src++;
            if (skipped + runLength > sx) {
                runLength -= sx - skipped;
                if (code == ePackedRleLiteral)
                    src += sx - skipped;
                skipped = sx;
                break;
            }
            skipped += runLength;
            if (code == ePackedRleLiteral)
                src += runLength;
        }

        unsigned int skip = 0;
        unsigned int remaining = sw;
        for (;;) {
            assert(runLength > 0);
            if (runLength > remaining)
                runLength = remaining;
            if (code == ePackedRleLiteral) {
                if (skip < runLength) {
                    unsigned int count = runLength - skip;
                    out += (count + 3) / 4;
                    skip = (count & 3) > 0 ? 4 - (count & 3) : 0;
                }
                else {
                    skip -= runLength;
                }
                src += runLength;
            }
            else if (skip < runLength) {
                switch (code) {
                case eRleControlShadow75: {
                    unsigned int count = runLength - skip;
                    while (count > 3) {
                        *out = ((*out >> 1) & div2mask)
                                               + ((*out >> 2) & div4mask);
                        out++;
                        count -= 4;
                    }
                    if (count > 0) {
                        *out = ((*out >> 1) & div2mask)
                                               + ((*out >> 2) & div4mask);
                        // DC's partial quarter-size shadow run does not advance out.
                        skip = 4 - count;
                    }
                    else {
                        skip = 0;
                    }
                    break;
                }
                case eRleControlShadow50: {
                    unsigned int count = runLength - skip;
                    while (count > 3) {
                        *out = (*out >> 1) & div2mask;
                        out++;
                        count -= 4;
                    }
                    if (count > 0) {
                        *out = (*out >> 1) & div2mask;
                        // DC's partial quarter-size shadow run does not advance out.
                        skip = 4 - count;
                    }
                    else {
                        skip = 0;
                    }
                    break;
                }
                default: {
                    unsigned int count = runLength - skip;
                    out += (count + 3) / 4;
                    skip = (count & 3) > 0 ? 4 - (count & 3) : 0;
                    break;
                }
                }
            }
            else {
                skip -= runLength;
            }
            remaining -= runLength;
            if (remaining == 0)
                break;
            code = *src >> 5;
            runLength = (*src & 31) + 1;
            src++;
        }
        lineDst = static_cast<unsigned short*>(static_cast<void*>(
            static_cast<unsigned char*>(static_cast<void*>(lineDst))
            + dpitch));
    }
}

// Loki's row/packet model with positive count/remainder tests (Loki writes
// `!= 0`).
// Original: CSpriteFrame::DrawTileScaled25; cspriteframe.cpp:5201
DC_ADDRESS(0x078be0, 0x67e)
void CSpriteFrame::DrawTileScaled25(int sx, int sy, int sw, int sh,
                                    unsigned short* dst, int dx, int dy,
                                    int dw, int dh, int dpitch,
                                    TPalette16& pal, bool hFlipped, bool vFlipped) const
{
    assert(EncodingMethod == eEncodeTilesetRLE || EncodingMethod == eEncodeRaw);
    ClipScaled25(sx, sy, sw, sh, dx, dy, dw, dh, hFlipped, vFlipped);
    if (sw <= 0 || sh <= 0)
        return;

    const unsigned short* const aLineOffset =
        static_cast<const unsigned short*>(static_cast<const void*>(map));
    const unsigned short* const palette = pal.Palette;
    if (!vFlipped) {
        if (!hFlipped) {
            unsigned short* lineDst = static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + dy * dpitch + dx * sizeof(unsigned short)));
            if (EncodingMethod == eEncodeRaw) {
                unsigned char* line = map + sy * Pitch + sx;
                do {
                    unsigned short* out = lineDst;
                    unsigned char* src = line;
                    int remaining = sw;
                    do {
                        *out++ = palette[*src];
                        src += 4;
                        remaining -= 4;
                    } while (remaining > 0);
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst))
                        + dpitch));
                    line += Pitch * 4;
                    sh -= 4;
                } while (sh > 0);
            }
            else {
                for (int y = sy; y < sy + sh; y += 4) {
                    unsigned short* out = lineDst;
                    unsigned char* src = map + aLineOffset[y];
                    unsigned char code;
                    unsigned int runLength;
                    unsigned int skipped = 0;
                    for (;;) {
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                        if (skipped + runLength > sx) {
                            runLength -= sx - skipped;
                            if (code == ePackedRleLiteral)
                                src += sx - skipped;
                            skipped = sx;
                            break;
                        }
                        skipped += runLength;
                        if (code == ePackedRleLiteral)
                            src += runLength;
                    }

                    unsigned int skip = 0;
                    unsigned int remaining = sw;
                    for (;;) {
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == ePackedRleLiteral) {
                            if (skip < runLength) {
                                unsigned int count = runLength - skip;
                                src += skip;
                                while (count > 3) {
                                    *out++ = palette[*src];
                                    src += 4;
                                    count -= 4;
                                }
                                if (count > 0) {
                                    *out++ = palette[*src];
                                    src += count;
                                    skip = 4 - count;
                                }
                                else {
                                    skip = 0;
                                }
                            }
                            else {
                                src += runLength;
                                skip -= runLength;
                            }
                        }
                        else if (skip < runLength) {
                            unsigned int count = runLength - skip;
                            out += (count + 3) / 4;
                            skip = (count & 3) > 0 ? 4 - (count & 3) : 0;
                        }
                        else {
                            skip -= runLength;
                        }
                        remaining -= runLength;
                        if (remaining == 0)
                            break;
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                    }
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst))
                        + dpitch));
                }
            }
        }
        else {
            unsigned short* lineDst = static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + dy * dpitch + dx * sizeof(unsigned short) + (sw >> 2) * sizeof(unsigned short)));
            if (EncodingMethod == eEncodeRaw) {
                unsigned char* line = map + sy * Pitch + sx;
                do {
                    unsigned short* out = lineDst;
                    unsigned char* src = line;
                    int remaining = sw;
                    do {
                        *--out = palette[*src];
                        src += 4;
                        remaining -= 4;
                    } while (remaining > 0);
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst))
                        + dpitch));
                    line += Pitch * 4;
                    sh -= 4;
                } while (sh > 0);
            }
            else {
                for (int y = sy; y < sy + sh; y += 4) {
                    unsigned short* out = lineDst;
                    unsigned char* src = map + aLineOffset[y];
                    unsigned char code;
                    unsigned int runLength;
                    unsigned int skipped = 0;
                    for (;;) {
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                        if (skipped + runLength > sx) {
                            runLength -= sx - skipped;
                            if (code == ePackedRleLiteral)
                                src += sx - skipped;
                            skipped = sx;
                            break;
                        }
                        skipped += runLength;
                        if (code == ePackedRleLiteral)
                            src += runLength;
                    }

                    unsigned int skip = 0;
                    unsigned int remaining = sw;
                    for (;;) {
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == ePackedRleLiteral) {
                            if (skip < runLength) {
                                unsigned int count = runLength - skip;
                                src += skip;
                                while (count > 3) {
                                    *--out = palette[*src];
                                    src += 4;
                                    count -= 4;
                                }
                                if (count > 0) {
                                    *--out = palette[*src];
                                    src += count;
                                    skip = 4 - count;
                                }
                                else {
                                    skip = 0;
                                }
                            }
                            else {
                                src += runLength;
                                skip -= runLength;
                            }
                        }
                        else if (skip < runLength) {
                            unsigned int count = runLength - skip;
                            out -= (count + 3) / 4;
                            skip = (count & 3) > 0 ? 4 - (count & 3) : 0;
                        }
                        else {
                            skip -= runLength;
                        }
                        remaining -= runLength;
                        if (remaining == 0)
                            break;
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                    }
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst))
                        + dpitch));
                }
            }
        }
    }
    else {
        if (!hFlipped) {
            unsigned short* lineDst = static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + (dy + (sh >> 2) - 1) * dpitch + dx * sizeof(unsigned short)));
            if (EncodingMethod == eEncodeRaw) {
                unsigned char* line = map + sy * Pitch + sx;
                do {
                    unsigned short* out = lineDst;
                    unsigned char* src = line;
                    int remaining = sw;
                    do {
                        *out++ = palette[*src];
                        src += 4;
                        remaining -= 4;
                    } while (remaining > 0);
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst))
                        - dpitch));
                    line += Pitch * 4;
                    sh -= 4;
                } while (sh > 0);
            }
            else {
                for (int y = sy; y < sy + sh; y += 4) {
                    unsigned short* out = lineDst;
                    unsigned char* src = map + aLineOffset[y];
                    unsigned char code;
                    unsigned int runLength;
                    unsigned int skipped = 0;
                    for (;;) {
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                        if (skipped + runLength > sx) {
                            runLength -= sx - skipped;
                            if (code == ePackedRleLiteral)
                                src += sx - skipped;
                            skipped = sx;
                            break;
                        }
                        skipped += runLength;
                        if (code == ePackedRleLiteral)
                            src += runLength;
                    }

                    unsigned int skip = 0;
                    unsigned int remaining = sw;
                    for (;;) {
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == ePackedRleLiteral) {
                            if (skip < runLength) {
                                unsigned int count = runLength - skip;
                                src += skip;
                                while (count > 3) {
                                    *out++ = palette[*src];
                                    src += 4;
                                    count -= 4;
                                }
                                if (count > 0) {
                                    *out++ = palette[*src];
                                    src += count;
                                    skip = 4 - count;
                                }
                                else {
                                    skip = 0;
                                }
                            }
                            else {
                                src += runLength;
                                skip -= runLength;
                            }
                        }
                        else if (skip < runLength) {
                            unsigned int count = runLength - skip;
                            out += (count + 3) / 4;
                            skip = (count & 3) > 0 ? 4 - (count & 3) : 0;
                        }
                        else {
                            skip -= runLength;
                        }
                        remaining -= runLength;
                        if (remaining == 0)
                            break;
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                    }
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst))
                        - dpitch));
                }
            }
        }
        else {
            unsigned short* lineDst = static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + (dy + (sh >> 2) - 1) * dpitch + dx * sizeof(unsigned short) + (sw >> 2) * sizeof(unsigned short)));
            if (EncodingMethod == eEncodeRaw) {
                unsigned char* line = map + sy * Pitch + sx;
                do {
                    unsigned short* out = lineDst;
                    unsigned char* src = line;
                    int remaining = sw;
                    do {
                        *--out = palette[*src];
                        src += 4;
                        remaining -= 4;
                    } while (remaining > 0);
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst))
                        - dpitch));
                    line += Pitch * 4;
                    sh -= 4;
                } while (sh > 0);
            }
            else {
                for (int y = sy; y < sy + sh; y += 4) {
                    unsigned short* out = lineDst;
                    unsigned char* src = map + aLineOffset[y];
                    unsigned char code;
                    unsigned int runLength;
                    unsigned int skipped = 0;
                    for (;;) {
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                        if (skipped + runLength > sx) {
                            runLength -= sx - skipped;
                            if (code == ePackedRleLiteral)
                                src += sx - skipped;
                            skipped = sx;
                            break;
                        }
                        skipped += runLength;
                        if (code == ePackedRleLiteral)
                            src += runLength;
                    }

                    unsigned int skip = 0;
                    unsigned int remaining = sw;
                    for (;;) {
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == ePackedRleLiteral) {
                            if (skip < runLength) {
                                unsigned int count = runLength - skip;
                                src += skip;
                                while (count > 3) {
                                    *--out = palette[*src];
                                    src += 4;
                                    count -= 4;
                                }
                                if (count > 0) {
                                    *--out = palette[*src];
                                    src += count;
                                    skip = 4 - count;
                                }
                                else {
                                    skip = 0;
                                }
                            }
                            else {
                                src += runLength;
                                skip -= runLength;
                            }
                        }
                        else if (skip < runLength) {
                            unsigned int count = runLength - skip;
                            out -= (count + 3) / 4;
                            skip = (count & 3) > 0 ? 4 - (count & 3) : 0;
                        }
                        else {
                            skip -= runLength;
                        }
                        remaining -= runLength;
                        if (remaining == 0)
                            break;
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                    }
                    lineDst = static_cast<unsigned short*>(static_cast<void*>(
                        static_cast<unsigned char*>(static_cast<void*>(lineDst))
                        - dpitch));
                }
            }
        }
    }
}
