// CSpriteFrame.cpp of the Loki port (Loki object 45). Function order, assert
// text and line numbers follow the Loki h3maped image; the `#line`
// directives reproduce the line numbers its asserts record.
#include <assert.h>
#include <limits>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cspriteframe.h"

#include "bitmap16.h"
#include "palette.h"

unsigned short CSpriteFrame::div2mask;
unsigned short CSpriteFrame::div4mask;

// Dreamcast names both file statics. Their initializers call
// numeric_limits<unsigned char>::max(), so the unit's static initializer
// stores them and the encoders copy them into function-local statics.
static const unsigned char kGeneralRLEOpaqueRunCode =
    std::numeric_limits<unsigned char>::max();
static const unsigned int kGeneralRLEMaxRunLength =
    std::numeric_limits<unsigned char>::max() + 1;

// Adventure-object frames are encoded in 32-pixel cells ("CroppedWidth %
// kCellWidth == 0"); the unsigned constant makes the cell arithmetic unsigned.
static const unsigned int kCellWidth = 32;


CSpriteFrame::CSpriteFrame()
    : resource(0, RESOURCE_TYPE_NONE),
      DataSize(0), ImageSize(0), EncodingMethod(eEncodeRaw),
      Width(0), Height(0), CroppedWidth(0), CroppedHeight(0),
      CroppedX(0), CroppedY(0), Pitch(0), map(0)
{
}

CSpriteFrame::CSpriteFrame(const char* name, int w, int h,
                           unsigned char* data, int csize,
                           TEncodingMethod encoding)
    : resource(name, RESOURCE_TYPE_SPRITE),
      ImageSize(w * h), EncodingMethod(encoding), Width(w), Height(h),
      CroppedWidth(w), CroppedHeight(h), CroppedX(0), CroppedY(0), Pitch(w)
{
#line 90
    assert(csize >= 0);
    assert(encoding == eEncodeRaw || csize > 0);
    DataSize = csize ? csize : ImageSize;
    map = new unsigned char[DataSize];
    if (map)
        memcpy(map, data, DataSize);
}

CSpriteFrame::CSpriteFrame(const char* name, int w, int h,
                           unsigned char* data, int csize,
                           TEncodingMethod encoding,
                           int cw, int ch, int cx, int cy)
    : resource(name, RESOURCE_TYPE_SPRITE),
      ImageSize(cw * ch), EncodingMethod(encoding), Width(w), Height(h),
      CroppedWidth(cw), CroppedHeight(ch), CroppedX(cx), CroppedY(cy),
      Pitch(cw)
{
#line 108
    assert(csize >= 0);
    assert(encoding == eEncodeRaw || csize > 0);
    DataSize = csize ? csize : ImageSize;
    map = new unsigned char[DataSize];
    if (map)
        memcpy(map, data, DataSize);
}

CSpriteFrame::CSpriteFrame(const char* name, bool cropped)
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

CSpriteFrame::~CSpriteFrame()
{
    if (map)
        delete[] map;
}

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

void CSpriteFrame::SetPixelFormat(unsigned int rmask, unsigned int gmask,
                                  unsigned int bmask)
{
    int rBits = 0;
    int gBits = 0;
    int bBits = 0;
    int i;
    for (i = 0; i < 16; i++)
        if (rmask & (1 << i))
            rBits++;
    for (i = 0; i < 16; i++)
        if (gmask & (1 << i))
            gBits++;
    for (i = 0; i < 16; i++)
        if (bmask & (1 << i))
            bBits++;

    div2mask = ((((1 << rBits) - 1) / 2) << (gBits + bBits))
        | ((((1 << gBits) - 1) / 2) << bBits)
        | (((1 << bBits) - 1) / 2);
    div4mask = ((((1 << rBits) - 1) / 4) << (gBits + bBits))
        | ((((1 << gBits) - 1) / 4) << bBits)
        | (((1 << bBits) - 1) / 4);
}

// The port has no PCX reader: both importers report and quit (the plain one
// with the cropped importer's message).
int CSpriteFrame::importPCXFile(const char* filename)
{
    printf("CSprintFrame::importCroppedPCXFile ... Write ME!");
    exit(0);
    return 0;
}

int CSpriteFrame::importCroppedPCXFile(const char* filename)
{
    printf("CSprintFrame::importCroppedPCXFile ... Write ME!");
    exit(0);
    return 0;
}

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
        const unsigned int* const lineOffsets = (const unsigned int*)map;
        unsigned char* source = map + lineOffsets[y];
        unsigned int position = 0;
        for (;;) {
            unsigned char code = *source++;
            unsigned int run = *source++ + 1;
            if (position + run > x) {
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
        const unsigned short* const lineOffsets = (const unsigned short*)map;
        unsigned char* source = map + lineOffsets[y];
        unsigned int position = 0;
        for (;;) {
            unsigned char code = *source >> 5;
            unsigned int run = (*source & 31) + 1;
            source++;
            if (position + run > x) {
                if (code == 7)
                    pixel = source[x - position];
                else
                    pixel = code;
                break;
            }
            position += run;
            if (code == 7)
                source += run;
        }
        break;
    }
    case eEncodeAdvObjRLE: {
        const unsigned short* const cellOffsets = (const unsigned short*)map;
#line 493
        assert(CroppedWidth % kCellWidth == 0);
        unsigned int cellsPerRow = CroppedWidth / kCellWidth;
        unsigned char* source = map + cellOffsets[y * cellsPerRow + x / kCellWidth];
        unsigned int position = x - x % kCellWidth;
        for (;;) {
            unsigned char code = *source >> 5;
            unsigned int run = (*source & 31) + 1;
            source++;
            if (position + run > x) {
                if (code == 7)
                    pixel = source[x - position];
                else
                    pixel = code;
                break;
            }
            position += run;
            if (code == 7)
                source += run;
        }
        break;
    }
    }
    return pixel;
}

int CSpriteFrame::Crop()
{
    unsigned char* source = map;
    unsigned char* dest;
    int leftoff = -1;
    int rightoff = -1;
    int topoff = -1;
    int bottomoff = -1;
    // i walks the scan direction, j across it.
    int i;
    for (i = 0; i < Width; i++) {
        for (int j = 0; j < Height; j++) {
            if (source[Width * j + i]) {
                leftoff = i;
                break;
            }
        }
        if (leftoff >= 0)
            break;
    }
    for (i = 0; i < Width; i++) {
        for (int j = 0; j < Height; j++) {
            if (source[Width * j + Width - i - 1]) {
                rightoff = i;
                break;
            }
        }
        if (rightoff >= 0)
            break;
    }
    for (i = 0; i < Height; i++) {
        for (int j = 0; j < Width; j++) {
            if (source[Width * i + j]) {
                topoff = i;
                break;
            }
        }
        if (topoff >= 0)
            break;
    }
    for (i = 0; i < Height; i++) {
        for (int j = 0; j < Width; j++) {
            if (source[(Height - i - 1) * Width + j]) {
                bottomoff = i;
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
    dest = newMap;
    source = map + topoff * Width + leftoff;
    for (int y = 0; y < CroppedHeight; y++) {
        memcpy(dest, source, CroppedWidth);
        dest += CroppedWidth;
        source += Width;
    }
    Pitch = Width;
    delete[] map;
    map = newMap;
    return 0;
}

void CSpriteFrame::Encode(TEncodingMethod method)
{
#line 650
    assert(EncodingMethod == eEncodeRaw);
    assert(method > eEncodeRaw && method < kNumEncodingMethods);

    assert(CroppedWidth > 0 && CroppedHeight > 0);
    switch (method) {
    case eEncodeGeneralRLE:
        EncodeGeneral();
        break;
    case eEncodeTilesetRLE:
        EncodeTileset();
        break;
    case eEncodeAdvObjRLE:
        EncodeAdvObj();
        break;
    }
}

void CSpriteFrame::EncodeGeneral()
{
    static const unsigned char kRawCode = kGeneralRLEOpaqueRunCode;
    static const unsigned int kMaxRunLength = kGeneralRLEMaxRunLength;

    unsigned int newDataSize = CroppedHeight * sizeof(unsigned int);
    unsigned char* source = map;
    unsigned int linesToGo = CroppedHeight;
    do {
        newDataSize += 2;
        unsigned char code = source[0] < 10 ? source[0] : kRawCode;
        unsigned int run = 0;
        int x = 1;
        for (;;) {
            run++;
            if (code == kRawCode)
                newDataSize++;
            if (x >= CroppedWidth)
                break;
            unsigned char nextCode = source[x] < 10 ? source[x] : kRawCode;
            if (nextCode != code || run == kMaxRunLength) {
                newDataSize += 2;
                code = nextCode;
                run = 0;
            }
            x++;
        }
        source += CroppedWidth;
    } while (--linesToGo);

    unsigned char* newMap = new unsigned char[newDataSize];
    unsigned int* lineOffset = (unsigned int*)newMap;
    unsigned int curOffset = CroppedHeight * sizeof(unsigned int);
    source = map;
    linesToGo = CroppedHeight;
    do {
        *lineOffset++ = curOffset;
        unsigned char code = source[0] < 10 ? source[0] : kRawCode;
        newMap[curOffset++] = code;
        unsigned char* runLength = &newMap[curOffset++];
        unsigned int run = 0;
        int x = 1;
        for (;;) {
            run++;
            if (code == kRawCode)
                newMap[curOffset++] = source[x - 1];
            if (x >= CroppedWidth)
                break;
            unsigned char nextCode = source[x] < 10 ? source[x] : kRawCode;
            if (nextCode != code || run == kMaxRunLength) {
                *runLength = run - 1;
                code = nextCode;
                newMap[curOffset++] = code;
                runLength = &newMap[curOffset++];
                run = 0;
            }
            x++;
        }
        *runLength = run - 1;
        source += CroppedWidth;
    } while (--linesToGo);
#line 765
    assert(curOffset == newDataSize);
    delete[] map;
    map = newMap;
    DataSize = newDataSize;
    EncodingMethod = eEncodeGeneralRLE;
}

void CSpriteFrame::EncodeTileset()
{
    bool hasControlPixels = false;
    for (int i = 0; i < DataSize; i++) {
        if (map[i] < 5) {
            hasControlPixels = true;
            break;
        }
    }
    if (hasControlPixels) {
        register const int kRawCode = 7;
        register const unsigned int kMaxRunLength = 32;

        unsigned int newDataSize = CroppedHeight * sizeof(unsigned short);
        unsigned char* source = map;
        unsigned int linesToGo = CroppedHeight;
        do {
            newDataSize++;
            unsigned char code = source[0] < 5 ? source[0] : kRawCode;
            unsigned int run = 0;
            int x = 1;
            for (;;) {
                run++;
                if (code == kRawCode)
                    newDataSize++;
                if (x >= CroppedWidth)
                    break;
                unsigned char nextCode = source[x] < 5 ? source[x] : kRawCode;
                if (nextCode != code || run == kMaxRunLength) {
                    newDataSize++;
                    code = nextCode;
                    run = 0;
                }
                x++;
            }
            source += CroppedWidth;
        } while (--linesToGo);

        unsigned char* newMap = new unsigned char[newDataSize];
        unsigned short* lineOffset = (unsigned short*)newMap;
        unsigned short curOffset = CroppedHeight * sizeof(unsigned short);
        source = map;
        linesToGo = CroppedHeight;
        do {
            *lineOffset++ = curOffset;
            unsigned char code = source[0] < 5 ? source[0] : kRawCode;
            newMap[curOffset] = code << 5;
            unsigned char* control = &newMap[curOffset++];
            unsigned int run = 0;
            int x = 1;
            for (;;) {
                run++;
                if (code == kRawCode)
                    newMap[curOffset++] = source[x - 1];
                if (x >= CroppedWidth)
                    break;
                unsigned char nextCode = source[x] < 5 ? source[x] : kRawCode;
                if (nextCode != code || run == kMaxRunLength) {
                    *control |= run - 1;
                    code = nextCode;
                    newMap[curOffset] = code << 5;
                    control = &newMap[curOffset++];
                    run = 0;
                }
                x++;
            }
            *control |= run - 1;
            source += CroppedWidth;
        } while (--linesToGo);
#line 882
        assert(curOffset == newDataSize);
        delete[] map;
        map = newMap;
        DataSize = newDataSize;
        EncodingMethod = eEncodeTilesetRLE;
    }
}

void CSpriteFrame::EncodeAdvObj()
{
    register const int kRawCode = 7;
    register const unsigned int kMaxRunLength = 32;

    int right = CroppedX + CroppedWidth;
    int newCroppedX = CroppedX - CroppedX % kCellWidth;
    int newRight = right + 31 - (right + 31) % kCellWidth;
    unsigned int newCroppedWidth = newRight - newCroppedX;
    unsigned int addLeft = CroppedX - newCroppedX;
    unsigned int addRight = newRight - right;
    unsigned int cellsPerLine = newCroppedWidth / kCellWidth;
    unsigned int newDataSize = CroppedHeight * cellsPerLine * sizeof(unsigned short);
    unsigned char* source = map;
    unsigned int linesToGo = CroppedHeight;
    do {
        unsigned char code;
        unsigned char* sourceCell = source;
        unsigned int cellsToGo = cellsPerLine;
        int srcLineX = 0;
        if (addLeft) {
            code = sourceCell[0] < 6 ? sourceCell[0] : kRawCode;
            if (code)
                newDataSize++;
            newDataSize++;
            unsigned int x = 1;
            srcLineX++;
            for (;;) {
                if (code == kRawCode)
                    newDataSize++;
                if (x >= kMaxRunLength - addLeft || srcLineX >= CroppedWidth)
                    break;
                unsigned char nextCode = sourceCell[x] < 6 ? sourceCell[x] : kRawCode;
                if (nextCode != code) {
                    newDataSize++;
                    code = nextCode;
                }
                x++;
                srcLineX++;
            }
            sourceCell += x;
            cellsToGo--;
        }
        while (cellsToGo) {
            code = sourceCell[0] < 6 ? sourceCell[0] : kRawCode;
            newDataSize++;
            unsigned int x = 1;
            srcLineX++;
            for (;;) {
                if (code == kRawCode)
                    newDataSize++;
                if (x >= kMaxRunLength || srcLineX >= CroppedWidth)
                    break;
                unsigned char nextCode = sourceCell[x] < 6 ? sourceCell[x] : kRawCode;
                if (nextCode != code) {
                    newDataSize++;
                    code = nextCode;
                }
                x++;
                srcLineX++;
            }
            sourceCell += x;
            cellsToGo--;
        }
        if (addRight && code)
            newDataSize++;
        source += CroppedWidth;
    } while (--linesToGo);

    unsigned char* newMap = new unsigned char[newDataSize];
    unsigned short* cellOffset = (unsigned short*)newMap;
    unsigned short curOffset = CroppedHeight * cellsPerLine * sizeof(unsigned short);
    source = map;
    linesToGo = CroppedHeight;
    do {
        unsigned char code;
        unsigned int run;
        unsigned char* control;
        unsigned char* sourceCell = source;
        unsigned int cellsToGo = cellsPerLine;
        int srcLineX = 0;
        if (addLeft) {
            *cellOffset++ = curOffset;
            code = sourceCell[0] < 6 ? sourceCell[0] : kRawCode;
            run = 0;
            if (code) {
                newMap[curOffset] = 0;
                newMap[curOffset++] |= addLeft - 1;
            } else {
                run += addLeft;
            }
            newMap[curOffset] = code << 5;
            control = &newMap[curOffset++];
            unsigned int x = 1;
            srcLineX++;
            for (;;) {
                run++;
                if (code == kRawCode)
                    newMap[curOffset++] = sourceCell[x - 1];
                if (x >= kMaxRunLength - addLeft || srcLineX >= CroppedWidth)
                    break;
                unsigned char nextCode = sourceCell[x] < 6 ? sourceCell[x] : kRawCode;
                if (nextCode != code) {
                    *control |= run - 1;
                    code = nextCode;
                    newMap[curOffset] = code << 5;
                    control = &newMap[curOffset++];
                    run = 0;
                }
                x++;
                srcLineX++;
            }
            sourceCell += x;
            if (--cellsToGo)
                *control |= run - 1;
        }
        if (cellsToGo) {
            for (;;) {
                *cellOffset++ = curOffset;
                code = sourceCell[0] < 6 ? sourceCell[0] : kRawCode;
                newMap[curOffset] = code << 5;
                control = &newMap[curOffset++];
                run = 0;
                unsigned int x = 1;
                srcLineX++;
                for (;;) {
                    run++;
                    if (code == kRawCode)
                        newMap[curOffset++] = sourceCell[x - 1];
                    if (x >= kMaxRunLength || srcLineX >= CroppedWidth)
                        break;
                    unsigned char nextCode = sourceCell[x] < 6 ? sourceCell[x] : kRawCode;
                    if (nextCode != code) {
                        *control |= run - 1;
                        code = nextCode;
                        newMap[curOffset] = code << 5;
                        control = &newMap[curOffset++];
                        run = 0;
                    }
                    x++;
                    srcLineX++;
                }
                sourceCell += x;
                if (!--cellsToGo)
                    break;
                *control |= run - 1;
            }
        }
#line 1114
        assert(srcLineX == CroppedWidth);
        if (addRight) {
            if (code) {
                *control |= run - 1;
                newMap[curOffset] = 0;
                control = &newMap[curOffset++];
                run = addRight;
            } else {
                run += addRight;
            }
        }
        *control |= run - 1;
        source += CroppedWidth;
    } while (--linesToGo);
#line 1135
    assert(curOffset == newDataSize);
    map = newMap;
    DataSize = newDataSize;
    EncodingMethod = eEncodeAdvObjRLE;
    CroppedX = newCroppedX;
    CroppedWidth = newCroppedWidth;
}

// Defined inline here, so the unit emits it after the header's inline
// members (Loki 0x81a4c6c, last before _GLOBAL_.I).
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
    if (dx + sw > dw) {
        if (hflip)
            sx += dx + sw - dw;
        sw = dw - dx;
    }
    if (dy + sh > dh) {
        if (vflip)
            sy += dy + sh - dh;
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
    if (sx + sw > deltaX) {
        if (hflip)
            dx += sx + sw - deltaX;
        sw = deltaX - sx;
    }
    int limitY = CroppedY + CroppedHeight;
    if (sy + sh > limitY) {
        if (vflip)
            dy += sy + sh - limitY;
        sh = limitY - sy;
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
void CSpriteFrame::Draw(int sx, int sy, int sw, int sh,
                        unsigned short* dst, int dx, int dy, int dw, int dh,
                        int dpitch, TPalette16& pal, bool hflip,
                        bool tblit) const
{
    if (EncodingMethod == eEncodeTilesetRLE || EncodingMethod == eEncodeRaw) {
        DrawTile(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, false);
        return;
    }
    else if (EncodingMethod == eEncodeAdvObjRLE) {
        DrawAdvObjImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, 0);
        return;
    }

#line 1252
    assert(EncodingMethod == eEncodeGeneralRLE);
    static const unsigned char kRawCode = kGeneralRLEOpaqueRunCode;
    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, false);
    if (sw <= 0 || sh <= 0)
        return;

    const unsigned int* const lineOffsets = (const unsigned int*)map;
    const unsigned short* const palette = pal.m_data;
    if (!hflip) {
        unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
            + dy * dpitch + dx * sizeof(unsigned short));
        for (int y = sy; y < sy + sh; y++) {
            unsigned short* out = lineDst;
            unsigned char* src = map + lineOffsets[y];
            unsigned char code;
            unsigned int runLength;
            unsigned int skipped = 0;
            for (;;) {
                code = *src++;
                runLength = *src++ + 1;
#line 1289
                assert(runLength > 0);
                if (skipped + runLength > sx) {
                    runLength -= sx - skipped;
                    if (code == kRawCode)
                        src += sx - skipped;
                    skipped = sx;
                    break;
                }
                skipped += runLength;
                if (code == kRawCode)
                    src += runLength;
            }

            unsigned int remaining = sw;
            for (;;) {
#line 1307
                assert(runLength > 0);
                if (runLength > remaining)
                    runLength = remaining;
                if (code == kRawCode) {
                    unsigned int count = runLength;
                    do {
                        *out++ = convert555to565(palette[*src++]);
                    } while (--count);
                }
                else if (tblit) {
                    out += runLength;
                }
                else {
                    unsigned short color = convert555to565(palette[code]);
                    unsigned int count = runLength;
                    do {
                        *out++ = color;
                    } while (--count);
                }
                remaining -= runLength;
                if (remaining == 0)
                    break;
                code = *src++;
                runLength = *src++ + 1;
            }
            lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
        }
    }
    else {
        unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
            + dy * dpitch + dx * sizeof(unsigned short) + sw * sizeof(unsigned short));
        for (int y = sy; y < sy + sh; y++) {
            unsigned short* out = lineDst;
            unsigned char* src = map + lineOffsets[y];
            unsigned char code;
            unsigned int runLength;
            unsigned int skipped = 0;
            for (;;) {
                code = *src++;
                runLength = *src++ + 1;
#line 1367
                assert(runLength > 0);
                if (skipped + runLength > sx) {
                    runLength -= sx - skipped;
                    if (code == kRawCode)
                        src += sx - skipped;
                    skipped = sx;
                    break;
                }
                skipped += runLength;
                if (code == kRawCode)
                    src += runLength;
            }

            unsigned int remaining = sw;
            for (;;) {
#line 1385
                assert(runLength > 0);
                if (runLength > remaining)
                    runLength = remaining;
                if (code == kRawCode) {
                    unsigned int count = runLength;
                    do {
                        *--out = convert555to565(palette[*src++]);
                    } while (--count);
                }
                else if (tblit) {
                    out -= runLength;
                }
                else {
                    unsigned short color = convert555to565(palette[code]);
                    unsigned int count = runLength;
                    do {
                        *--out = color;
                    } while (--count);
                }
                remaining -= runLength;
                if (remaining == 0)
                    break;
                code = *src++;
                runLength = *src++ + 1;
            }
            lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
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
// Loki probes: a DrawTile-style `palette` local is byte-flat here, and
// assigning the line table inside the positive extent guard (where retail
// and Loki load map) scores 93.52%; RoE's body also differs in its tail.
// The native per-row destination lifetime also appears in the exact adjacent
// adventure renderer. Advancing that cursor directly restores 95.9000%;
// splitting its address into a base and offset leaves 94.7302%.
void CSpriteFrame::DrawCreatureImpl(int sx, int sy, int sw, int sh,
                                    unsigned short* dst, int dx, int dy,
                                    int dw, int dh, int dpitch,
                                    TPalette16& pal, bool hflip,
                                    unsigned short outcolor,
                                    bool alpha) const
{
    if (!alpha) {
        if (EncodingMethod == eEncodeTilesetRLE || EncodingMethod == eEncodeRaw) {
            DrawTile(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, false);
            return;
        }
        else if (EncodingMethod == eEncodeAdvObjRLE) {
            DrawAdvObjImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, outcolor);
            return;
        }
    }

#line 1768
    assert(EncodingMethod == eEncodeGeneralRLE);
    static const unsigned char kRawCode = kGeneralRLEOpaqueRunCode;
    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, false);
    if (sw <= 0 || sh <= 0)
        return;

    const unsigned int* const lineOffsets = (const unsigned int*)map;
    const unsigned short* const palette = pal.m_data;
    if (!hflip) {
        unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
            + dy * dpitch + dx * sizeof(unsigned short));
        for (int y = sy; y < sy + sh; y++) {
            unsigned short* out = lineDst;
            unsigned char* src = map + lineOffsets[y];
            unsigned char code;
            unsigned int runLength;
            unsigned int skipped = 0;
            for (;;) {
                code = *src++;
                runLength = *src++ + 1;
#line 1805
                assert(runLength > 0);
                if (skipped + runLength > sx) {
                    runLength -= sx - skipped;
                    if (code == kRawCode)
                        src += sx - skipped;
                    skipped = sx;
                    break;
                }
                skipped += runLength;
                if (code == kRawCode)
                    src += runLength;
            }

            unsigned int remaining = sw;
            for (;;) {
#line 1823
                assert(runLength > 0);
                if (runLength > remaining)
                    runLength = remaining;
                if (code == kRawCode) {
                    unsigned int count = runLength;
                    if (!alpha) {
                        do {
                            *out++ = convert555to565(palette[*src++]);
                        } while (--count);
                    }
                    else {
                        do {
                            *out = convert555to565(((palette[*src++] >> 1) & div2mask)
                                                   + ((*out >> 1) & div2mask));
                            out++;
                        } while (--count);
                    }
                }
                else if (outcolor == 0) {
                    switch (code) {
                    case 1:
                    case 7: {
                        unsigned int count = runLength;
                        do {
                            *out = convert555to565(((*out >> 1) & div2mask)
                                                   + ((*out >> 2) & div4mask));
                            out++;
                        } while (--count);
                        break;
                    }
                    case 4:
                    case 6: {
                        unsigned int count = runLength;
                        do {
                            *out = convert555to565((*out >> 1) & div2mask);
                            out++;
                        } while (--count);
                        break;
                    }
                    default:
                        out += runLength;
                        break;
                    }
                }
                else {
                    switch (code) {
                    case 1: {
                        unsigned int count = runLength;
                        do {
                            *out = convert555to565(((*out >> 1) & div2mask)
                                                   + ((*out >> 2) & div4mask));
                            out++;
                        } while (--count);
                        break;
                    }
                    case 4: {
                        unsigned int count = runLength;
                        do {
                            *out = convert555to565((*out >> 1) & div2mask);
                            out++;
                        } while (--count);
                        break;
                    }
                    case 5:
                    case 6:
                    case 7: {
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
                if (remaining == 0)
                    break;
                code = *src++;
                runLength = *src++ + 1;
            }
            lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
        }
    }
    else {
        unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
            + dy * dpitch + dx * sizeof(unsigned short) + sw * sizeof(unsigned short));
        for (int y = sy; y < sy + sh; y++) {
            unsigned short* out = lineDst;
            unsigned char* src = map + lineOffsets[y];
            unsigned char code;
            unsigned int runLength;
            unsigned int skipped = 0;
            for (;;) {
                code = *src++;
                runLength = *src++ + 1;
#line 1964
                assert(runLength > 0);
                if (skipped + runLength > sx) {
                    runLength -= sx - skipped;
                    if (code == kRawCode)
                        src += sx - skipped;
                    skipped = sx;
                    break;
                }
                skipped += runLength;
                if (code == kRawCode)
                    src += runLength;
            }

            unsigned int remaining = sw;
            for (;;) {
#line 1982
                assert(runLength > 0);
                if (runLength > remaining)
                    runLength = remaining;
                if (code == kRawCode) {
                    unsigned int count = runLength;
                    if (!alpha) {
                        do {
                            *--out = convert555to565(palette[*src++]);
                        } while (--count);
                    }
                    else {
                        do {
                            --out;
                            *out = (div2mask & (convert555to565(palette[*src++]) >> 1))
                                + ((*out >> 1) & div2mask);
                        } while (--count);
                    }
                }
                else if (outcolor == 0) {
                    switch (code) {
                    case 1:
                    case 7: {
                        unsigned int count = runLength;
                        do {
                            --out;
                            *out = convert555to565(((*out >> 1) & div2mask)
                                                   + ((*out >> 2) & div4mask));
                        } while (--count);
                        break;
                    }
                    case 4:
                    case 6: {
                        unsigned int count = runLength;
                        do {
                            --out;
                            *out = convert555to565((*out >> 1) & div2mask);
                        } while (--count);
                        break;
                    }
                    default:
                        out -= runLength;
                        break;
                    }
                }
                else {
                    switch (code) {
                    case 1: {
                        unsigned int count = runLength;
                        do {
                            --out;
                            *out = convert555to565(((*out >> 1) & div2mask)
                                                   + ((*out >> 2) & div4mask));
                        } while (--count);
                        break;
                    }
                    case 4: {
                        unsigned int count = runLength;
                        do {
                            --out;
                            *out = convert555to565((*out >> 1) & div2mask);
                        } while (--count);
                        break;
                    }
                    case 5:
                    case 6:
                    case 7: {
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
                if (remaining == 0)
                    break;
                code = *src++;
                runLength = *src++ + 1;
            }
            lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
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
void CSpriteFrame::DrawAdvObjImpl(int sx, int sy, int sw, int sh,
                                  unsigned short* dst, int dx, int dy, int dw,
                                  int dh, int dpitch, TPalette16& pal,
                                  bool hflip,
                                  unsigned short flagcolor) const
{
    if (EncodingMethod == eEncodeGeneralRLE) {
        // The port passes sw as the source x, as the Windows build does.
        DrawCreature(sw, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, flagcolor);
        return;
    }
    else if (EncodingMethod == eEncodeTilesetRLE || EncodingMethod == eEncodeRaw) {
        DrawTile(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, false);
        return;
    }

#line 2121
    assert(EncodingMethod == eEncodeAdvObjRLE);
    register const unsigned char kRawCode = 7;
    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, false);
    if (sw <= 0 || sh <= 0)
        return;

#line 2135
    assert(CroppedWidth % kCellWidth == 0);
    unsigned int cellsPerLine = CroppedWidth / kCellWidth;
    const unsigned short* const cellOffsets = (const unsigned short*)map;
    const unsigned short* const palette = pal.m_data;
    if (!hflip) {
        unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
            + dy * dpitch + dx * sizeof(unsigned short));
        for (int y = sy; y < sy + sh; y++) {
            unsigned short* out = lineDst;
            unsigned char* src = map + cellOffsets[y * cellsPerLine + sx / kCellWidth];
            unsigned char code;
            unsigned int runLength;
            unsigned int skipped = sx & ~(kCellWidth - 1);
            for (;;) {
                code = *src >> 5;
                runLength = (*src & 31) + 1;
                src++;
                if (skipped + runLength > sx) {
                    runLength -= sx - skipped;
                    if (code == kRawCode)
                        src += sx - skipped;
                    skipped = sx;
                    break;
                }
                skipped += runLength;
                if (code == kRawCode)
                    src += runLength;
            }

            unsigned int remaining = sw;
            for (;;) {
#line 2179
                assert(runLength > 0);
                if (runLength > remaining)
                    runLength = remaining;
                if (code == kRawCode) {
                    unsigned int count = runLength;
                    do {
                        *out++ = convert555to565(palette[*src++]);
                    } while (--count);
                }
                else {
                    switch (code) {
                    case 5:
                        if (flagcolor != 0) {
                            unsigned int count = runLength;
                            do {
                                *out++ = convert555to565(flagcolor);
                            } while (--count);
                            break;
                        }
                    default:
                        out += runLength;
                        break;
                    }
                }
                remaining -= runLength;
                if (remaining == 0)
                    break;
                code = *src >> 5;
                runLength = (*src & 31) + 1;
                src++;
            }
            lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
        }
    }
    else {
        unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
            + dy * dpitch + dx * sizeof(unsigned short) + sw * sizeof(unsigned short));
        for (int y = sy; y < sy + sh; y++) {
            unsigned short* out = lineDst;
            unsigned char* src = map + cellOffsets[y * cellsPerLine + sx / kCellWidth];
            unsigned char code;
            unsigned int runLength;
            unsigned int skipped = sx & ~(kCellWidth - 1);
            for (;;) {
                code = *src >> 5;
                runLength = (*src & 31) + 1;
                src++;
                if (skipped + runLength > sx) {
                    runLength -= sx - skipped;
                    if (code == kRawCode)
                        src += sx - skipped;
                    skipped = sx;
                    break;
                }
                skipped += runLength;
                if (code == kRawCode)
                    src += runLength;
            }

            unsigned int remaining = sw;
            for (;;) {
#line 2265
                assert(runLength > 0);
                if (runLength > remaining)
                    runLength = remaining;
                if (code == kRawCode) {
                    unsigned int count = runLength;
                    do {
                        *--out = convert555to565(palette[*src++]);
                    } while (--count);
                }
                else {
                    switch (code) {
                    case 5:
                        if (flagcolor != 0) {
                            unsigned int count = runLength;
                            do {
                                *--out = convert555to565(flagcolor);
                            } while (--count);
                            break;
                        }
                    default:
                        out -= runLength;
                        break;
                    }
                }
                remaining -= runLength;
                if (remaining == 0)
                    break;
                code = *src >> 5;
                runLength = (*src & 31) + 1;
                src++;
            }
            lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
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
// DC records palette and aCellOffset as read-only pointers; lines
// 2462/2463 load the map table and bind pal+0x1c before either row loop.
// A single native row cursor restores 98.0000%; a fixed base plus row offset
// leaves 94.5249%. The decoder and palette helpers remain unchanged.
void CSpriteFrame::DrawAdvObjWithFlagAlpha(int sx, int sy, int sw, int sh,
                                           unsigned short* dst, int dx, int dy,
                                           int dw, int dh, int dpitch,
                                           TPalette16& pal,
                                           unsigned short flagcolor,
                                           bool hflip) const
{
#line 2318
    assert(EncodingMethod == eEncodeAdvObjRLE);
    register const unsigned char kRawCode = 7;
    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, false);
    if (sw <= 0 || sh <= 0)
        return;

#line 2332
    assert(CroppedWidth % kCellWidth == 0);
    unsigned int cellsPerLine = CroppedWidth / kCellWidth;
    const unsigned short* const cellOffsets = (const unsigned short*)map;
    const unsigned short* const palette = pal.m_data;
    if (!hflip) {
        unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
            + dy * dpitch + dx * sizeof(unsigned short));
        for (int y = sy; y < sy + sh; y++) {
            unsigned short* out = lineDst;
            unsigned char* src = map + cellOffsets[y * cellsPerLine + sx / kCellWidth];
            unsigned char code;
            unsigned int runLength;
            unsigned int skipped = sx & ~(kCellWidth - 1);
            for (;;) {
                code = *src >> 5;
                runLength = (*src & 31) + 1;
                src++;
                if (skipped + runLength > sx) {
                    runLength -= sx - skipped;
                    if (code == kRawCode)
                        src += sx - skipped;
                    skipped = sx;
                    break;
                }
                skipped += runLength;
                if (code == kRawCode)
                    src += runLength;
            }

            unsigned int remaining = sw;
            for (;;) {
#line 2376
                assert(runLength > 0);
                if (runLength > remaining)
                    runLength = remaining;
                if (code == kRawCode) {
                    unsigned int count = runLength;
                    do {
                        *out = convert555to565(((palette[*src++] >> 1) & div2mask)
                                               + ((*out >> 1) & div2mask));
                        out++;
                    } while (--count);
                }
                else {
                    switch (code) {
                    case 5:
                        if (flagcolor != 0) {
                            unsigned int count = runLength;
                            do {
                                *out = convert555to565(((flagcolor >> 1) & div2mask)
                                                       + ((*out >> 1) & div2mask));
                                out++;
                            } while (--count);
                            break;
                        }
                    default:
                        out += runLength;
                        break;
                    }
                }
                remaining -= runLength;
                if (remaining == 0)
                    break;
                code = *src >> 5;
                runLength = (*src & 31) + 1;
                src++;
            }
            lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
        }
    }
    else {
        unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
            + dy * dpitch + dx * sizeof(unsigned short) + sw * sizeof(unsigned short));
        for (int y = sy; y < sy + sh; y++) {
            unsigned short* out = lineDst;
            unsigned char* src = map + cellOffsets[y * cellsPerLine + sx / kCellWidth];
            unsigned char code;
            unsigned int runLength;
            unsigned int skipped = sx & ~(kCellWidth - 1);
            for (;;) {
                code = *src >> 5;
                runLength = (*src & 31) + 1;
                src++;
                if (skipped + runLength > sx) {
                    runLength -= sx - skipped;
                    if (code == kRawCode)
                        src += sx - skipped;
                    skipped = sx;
                    break;
                }
                skipped += runLength;
                if (code == kRawCode)
                    src += runLength;
            }

            unsigned int remaining = sw;
            for (;;) {
#line 2464
                assert(runLength > 0);
                if (runLength > remaining)
                    runLength = remaining;
                if (code == kRawCode) {
                    unsigned int count = runLength;
                    do {
                        --out;
                        *out = convert555to565(((palette[*src++] >> 1) & div2mask)
                                               + ((*out >> 1) & div2mask));
                    } while (--count);
                }
                else {
                    switch (code) {
                    case 5:
                        if (flagcolor != 0) {
                            unsigned int count = runLength;
                            do {
                                --out;
                                *out = convert555to565(((flagcolor >> 1) & div2mask)
                                                       + ((*out >> 1) & div2mask));
                            } while (--count);
                            break;
                        }
                    default:
                        out -= runLength;
                        break;
                    }
                }
                remaining -= runLength;
                if (remaining == 0)
                    break;
                code = *src >> 5;
                runLength = (*src & 31) + 1;
                src++;
            }
            lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
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
// The native row cursor restores 99.9404%; an extra base/offset pair changes
// the surrounding lifetimes and leaves 94.3325%.
void CSpriteFrame::DrawAdvObjShadowImpl(int sx, int sy, int sw, int sh,
                                        unsigned short* dst, int dx, int dy,
                                        int dw, int dh, int dpitch,
                                        TPalette16& pal, bool hflip) const
{
#line 2519
    assert(EncodingMethod == eEncodeAdvObjRLE);
    register const unsigned char kRawCode = 7;
    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, false);
    if (sw <= 0 || sh <= 0)
        return;

#line 2533
    assert(CroppedWidth % kCellWidth == 0);
    unsigned int cellsPerLine = CroppedWidth / kCellWidth;
    const unsigned short* const cellOffsets = (const unsigned short*)map;
    const unsigned short* const palette = pal.m_data;
    if (!hflip) {
        unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
            + dy * dpitch + dx * sizeof(unsigned short));
        for (int y = sy; y < sy + sh; y++) {
            unsigned short* out = lineDst;
            unsigned char* src = map + cellOffsets[y * cellsPerLine + sx / kCellWidth];
            unsigned char code;
            unsigned int runLength;
            unsigned int skipped = sx & ~(kCellWidth - 1);
            for (;;) {
                code = *src >> 5;
                runLength = (*src & 31) + 1;
                src++;
                if (skipped + runLength > sx) {
                    runLength -= sx - skipped;
                    if (code == kRawCode)
                        src += sx - skipped;
                    skipped = sx;
                    break;
                }
                skipped += runLength;
                if (code == kRawCode)
                    src += runLength;
            }

            unsigned int remaining = sw;
            for (;;) {
#line 2577
                assert(runLength > 0);
                if (runLength > remaining)
                    runLength = remaining;
                if (code == kRawCode) {
                    out += runLength;
                    src += runLength;
                }
                else {
                    switch (code) {
                    case 1: {
                        unsigned int count = runLength;
                        do {
                            *out = convert555to565(((*out >> 1) & div2mask)
                                                   + ((*out >> 2) & div4mask));
                            out++;
                        } while (--count);
                        break;
                    }
                    case 4: {
                        unsigned int count = runLength;
                        do {
                            *out = convert555to565((*out >> 1) & div2mask);
                            out++;
                        } while (--count);
                        break;
                    }
                    default:
                        out += runLength;
                        break;
                    }
                }
                remaining -= runLength;
                if (remaining == 0)
                    break;
                code = *src >> 5;
                runLength = (*src & 31) + 1;
                src++;
            }
            lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
        }
    }
    else {
        unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
            + dy * dpitch + dx * sizeof(unsigned short) + sw * sizeof(unsigned short));
        for (int y = sy; y < sy + sh; y++) {
            unsigned short* out = lineDst;
            unsigned char* src = map + cellOffsets[y * cellsPerLine + sx / kCellWidth];
            unsigned char code;
            unsigned int runLength;
            unsigned int skipped = sx & ~(kCellWidth - 1);
            for (;;) {
                code = *src >> 5;
                runLength = (*src & 31) + 1;
                src++;
                if (skipped + runLength > sx) {
                    runLength -= sx - skipped;
                    if (code == kRawCode)
                        src += sx - skipped;
                    skipped = sx;
                    break;
                }
                skipped += runLength;
                if (code == kRawCode)
                    src += runLength;
            }

            unsigned int remaining = sw;
            for (;;) {
#line 2670
                assert(runLength > 0);
                if (runLength > remaining)
                    runLength = remaining;
                if (code == kRawCode) {
                    out -= runLength;
                    src += runLength;
                }
                else {
                    switch (code) {
                    case 1: {
                        unsigned int count = runLength;
                        do {
                            --out;
                            *out = convert555to565(((*out >> 1) & div2mask)
                                                   + ((*out >> 2) & div4mask));
                        } while (--count);
                        break;
                    }
                    case 4: {
                        unsigned int count = runLength;
                        do {
                            --out;
                            *out = convert555to565((*out >> 1) & div2mask);
                        } while (--count);
                        break;
                    }
                    default:
                        out -= runLength;
                        break;
                    }
                }
                remaining -= runLength;
                if (remaining == 0)
                    break;
                code = *src >> 5;
                runLength = (*src & 31) + 1;
                src++;
            }
            lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
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
void CSpriteFrame::DrawTile(unsigned short* dst, int dx, int dy, int dpitch,
                            TPalette16& pal, bool hflip, bool vflip) const
{
    printf("CSpriteFrame::DrawTile(ushort *, int, int, int, TPalette16&, bool, bool) ... possibly hosed.\n");
    DrawTile(0, 0, Width, Height, dst, dx, dy, Width, Height, dpitch, pal, hflip, vflip);
}

void CSpriteFrame::DrawTile(int sx, int sy, int sw, int sh, unsigned short* dst,
                            int dx, int dy, int dw, int dh, int dpitch,
                            TPalette16& pal, bool hflip, bool vflip) const
{
    if (EncodingMethod == eEncodeGeneralRLE) {
        // As in the Windows build, sw is passed as the source x.
        Draw(sw, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, true);
        return;
    }
    else if (EncodingMethod == eEncodeAdvObjRLE) {
        DrawAdvObjImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, 0);
        return;
    }

#line 2748
    assert(EncodingMethod == eEncodeTilesetRLE || EncodingMethod == eEncodeRaw);
    register const unsigned char kRawCode = 7;
    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, vflip);
    if (sw <= 0 || sh <= 0)
        return;

    const unsigned short* const lineOffsets = (const unsigned short*)map;
    const unsigned short* const palette = pal.m_data;
    if (!vflip) {
        if (!hflip) {
                unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
                    + dy * dpitch + dx * sizeof(unsigned short));
                if (EncodingMethod == eEncodeRaw) {
                    unsigned char* line = map + sy * Pitch + sx;
                    do {
                        unsigned short* out = lineDst;
                        unsigned char* src = line;
                        int remaining = sw;
                            switch (remaining & 7)
                                do {
                            case 0:
                                    *out++ = convert555to565(palette[*src++]);
                                    --remaining;
                            case 7:
                                    *out++ = convert555to565(palette[*src++]);
                                    --remaining;
                            case 6:
                                    *out++ = convert555to565(palette[*src++]);
                                    --remaining;
                            case 5:
                                    *out++ = convert555to565(palette[*src++]);
                                    --remaining;
                            case 4:
                                    *out++ = convert555to565(palette[*src++]);
                                    --remaining;
                            case 3:
                                    *out++ = convert555to565(palette[*src++]);
                                    --remaining;
                            case 2:
                                    *out++ = convert555to565(palette[*src++]);
                                    --remaining;
                            case 1:
                                    *out++ = convert555to565(palette[*src++]);
                                    --remaining;
                                } while (remaining > 0);
                        lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
                        line += Pitch;
                    } while (--sh > 0);
                }
                else {
                    for (int y = sy; y < sy + sh; y++) {
                        unsigned short* out = lineDst;
                        unsigned char* src = map + lineOffsets[y];
                        unsigned char code;
                        unsigned int runLength;
                        unsigned int skipped = 0;
                        for (;;) {
                            code = *src >> 5;
                            runLength = (*src & 31) + 1;
                            src++;
                            if (skipped + runLength > sx) {
                                runLength -= sx - skipped;
                                if (code == kRawCode)
                                    src += sx - skipped;
                                skipped = sx;
                                break;
                            }
                            skipped += runLength;
                            if (code == kRawCode)
                                src += runLength;
                        }

                        unsigned int remaining = sw;
                        for (;;) {
#line 2855
                            assert(runLength > 0);
                            if (runLength > remaining)
                                runLength = remaining;
                            if (code == kRawCode) {
                                unsigned int count = runLength;
                                do {
                                    *out++ = convert555to565(palette[*src++]);
                                } while (--count);
                            }
                            else {
                                out += runLength;
                            }
                            remaining -= runLength;
                            if (remaining == 0)
                                break;
                            code = *src >> 5;
                            runLength = (*src & 31) + 1;
                            src++;
                        }
                        lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
                    }
                }
        }
        else {
                unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
                    + dy * dpitch + dx * sizeof(unsigned short) + sw * sizeof(unsigned short));
                if (EncodingMethod == eEncodeRaw) {
                    unsigned char* line = map + sy * Pitch + sx;
                    do {
                        unsigned short* out = lineDst;
                        unsigned char* src = line;
                        int remaining = sw;
                            switch (remaining & 7)
                                do {
                            case 0:
                                    *--out = convert555to565(palette[*src++]);
                                    --remaining;
                            case 7:
                                    *--out = convert555to565(palette[*src++]);
                                    --remaining;
                            case 6:
                                    *--out = convert555to565(palette[*src++]);
                                    --remaining;
                            case 5:
                                    *--out = convert555to565(palette[*src++]);
                                    --remaining;
                            case 4:
                                    *--out = convert555to565(palette[*src++]);
                                    --remaining;
                            case 3:
                                    *--out = convert555to565(palette[*src++]);
                                    --remaining;
                            case 2:
                                    *--out = convert555to565(palette[*src++]);
                                    --remaining;
                            case 1:
                                    *--out = convert555to565(palette[*src++]);
                                    --remaining;
                                } while (remaining > 0);
                        lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
                        line += Pitch;
                    } while (--sh > 0);
                }
                else {
                    for (int y = sy; y < sy + sh; y++) {
                        unsigned short* out = lineDst;
                        unsigned char* src = map + lineOffsets[y];
                        unsigned char code;
                        unsigned int runLength;
                        unsigned int skipped = 0;
                        for (;;) {
                            code = *src >> 5;
                            runLength = (*src & 31) + 1;
                            src++;
                            if (skipped + runLength > sx) {
                                runLength -= sx - skipped;
                                if (code == kRawCode)
                                    src += sx - skipped;
                                skipped = sx;
                                break;
                            }
                            skipped += runLength;
                            if (code == kRawCode)
                                src += runLength;
                        }

                        unsigned int remaining = sw;
                        for (;;) {
#line 2972
                            assert(runLength > 0);
                            if (runLength > remaining)
                                runLength = remaining;
                            if (code == kRawCode) {
                                unsigned int count = runLength;
                                do {
                                    *--out = convert555to565(palette[*src++]);
                                } while (--count);
                            }
                            else {
                                out -= runLength;
                            }
                            remaining -= runLength;
                            if (remaining == 0)
                                break;
                            code = *src >> 5;
                            runLength = (*src & 31) + 1;
                            src++;
                        }
                        lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
                    }
                }
        }
    }
    else {
        if (!hflip) {
                unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
                    + (dy + sh - 1) * dpitch + dx * sizeof(unsigned short));
                if (EncodingMethod == eEncodeRaw) {
                    unsigned char* line = map + sy * Pitch + sx;
                    do {
                        unsigned short* out = lineDst;
                        unsigned char* src = line;
                        int remaining = sw;
                            switch (remaining & 7)
                                do {
                            case 0:
                                    *out++ = convert555to565(palette[*src++]);
                                    --remaining;
                            case 7:
                                    *out++ = convert555to565(palette[*src++]);
                                    --remaining;
                            case 6:
                                    *out++ = convert555to565(palette[*src++]);
                                    --remaining;
                            case 5:
                                    *out++ = convert555to565(palette[*src++]);
                                    --remaining;
                            case 4:
                                    *out++ = convert555to565(palette[*src++]);
                                    --remaining;
                            case 3:
                                    *out++ = convert555to565(palette[*src++]);
                                    --remaining;
                            case 2:
                                    *out++ = convert555to565(palette[*src++]);
                                    --remaining;
                            case 1:
                                    *out++ = convert555to565(palette[*src++]);
                                    --remaining;
                                } while (remaining > 0);
                        lineDst = (unsigned short*)((unsigned char*)lineDst - dpitch);
                        line += Pitch;
                    } while (--sh > 0);
                }
                else {
                    for (int y = sy; y < sy + sh; y++) {
                        unsigned short* out = lineDst;
                        unsigned char* src = map + lineOffsets[y];
                        unsigned char code;
                        unsigned int runLength;
                        unsigned int skipped = 0;
                        for (;;) {
                            code = *src >> 5;
                            runLength = (*src & 31) + 1;
                            src++;
                            if (skipped + runLength > sx) {
                                runLength -= sx - skipped;
                                if (code == kRawCode)
                                    src += sx - skipped;
                                skipped = sx;
                                break;
                            }
                            skipped += runLength;
                            if (code == kRawCode)
                                src += runLength;
                        }

                        unsigned int remaining = sw;
                        for (;;) {
#line 3092
                            assert(runLength > 0);
                            if (runLength > remaining)
                                runLength = remaining;
                            if (code == kRawCode) {
                                unsigned int count = runLength;
                                do {
                                    *out++ = convert555to565(palette[*src++]);
                                } while (--count);
                            }
                            else {
                                out += runLength;
                            }
                            remaining -= runLength;
                            if (remaining == 0)
                                break;
                            code = *src >> 5;
                            runLength = (*src & 31) + 1;
                            src++;
                        }
                        lineDst = (unsigned short*)((unsigned char*)lineDst - dpitch);
                    }
                }
        }
        else {
                unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
                    + (dy + sh - 1) * dpitch + dx * sizeof(unsigned short) + sw * sizeof(unsigned short));
                if (EncodingMethod == eEncodeRaw) {
                    unsigned char* line = map + sy * Pitch + sx;
                    do {
                        unsigned short* out = lineDst;
                        unsigned char* src = line;
                        int remaining = sw;
                            switch (remaining & 7)
                                do {
                            case 0:
                                    *--out = convert555to565(palette[*src++]);
                                    --remaining;
                            case 7:
                                    *--out = convert555to565(palette[*src++]);
                                    --remaining;
                            case 6:
                                    *--out = convert555to565(palette[*src++]);
                                    --remaining;
                            case 5:
                                    *--out = convert555to565(palette[*src++]);
                                    --remaining;
                            case 4:
                                    *--out = convert555to565(palette[*src++]);
                                    --remaining;
                            case 3:
                                    *--out = convert555to565(palette[*src++]);
                                    --remaining;
                            case 2:
                                    *--out = convert555to565(palette[*src++]);
                                    --remaining;
                            case 1:
                                    *--out = convert555to565(palette[*src++]);
                                    --remaining;
                                } while (remaining > 0);
                        lineDst = (unsigned short*)((unsigned char*)lineDst - dpitch);
                        line += Pitch;
                    } while (--sh > 0);
                }
                else {
                    for (int y = sy; y < sy + sh; y++) {
                        unsigned short* out = lineDst;
                        unsigned char* src = map + lineOffsets[y];
                        unsigned char code;
                        unsigned int runLength;
                        unsigned int skipped = 0;
                        for (;;) {
                            code = *src >> 5;
                            runLength = (*src & 31) + 1;
                            src++;
                            if (skipped + runLength > sx) {
                                runLength -= sx - skipped;
                                if (code == kRawCode)
                                    src += sx - skipped;
                                skipped = sx;
                                break;
                            }
                            skipped += runLength;
                            if (code == kRawCode)
                                src += runLength;
                        }

                        unsigned int remaining = sw;
                        for (;;) {
#line 3209
                            assert(runLength > 0);
                            if (runLength > remaining)
                                runLength = remaining;
                            if (code == kRawCode) {
                                unsigned int count = runLength;
                                do {
                                    *--out = convert555to565(palette[*src++]);
                                } while (--count);
                            }
                            else {
                                out -= runLength;
                            }
                            remaining -= runLength;
                            if (remaining == 0)
                                break;
                            code = *src >> 5;
                            runLength = (*src & 31) + 1;
                            src++;
                        }
                        lineDst = (unsigned short*)((unsigned char*)lineDst - dpitch);
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
void CSpriteFrame::DrawTileShadow(int sx, int sy, int sw, int sh,
                                  unsigned short* dst, int dx, int dy, int dw,
                                  int dh, int dpitch, TPalette16& pal,
                                  bool hflip, bool vflip) const
{
    if (EncodingMethod == eEncodeRaw)
        return;

#line 3247
    assert(EncodingMethod == eEncodeTilesetRLE);
    register const unsigned char kRawCode = 7;
    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, vflip);
    if (sw <= 0 || sh <= 0)
        return;

    const unsigned short* const lineOffsets = (const unsigned short*)map;
    const unsigned short* const palette = pal.m_data;
    if (!vflip) {
        if (!hflip) {
            unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
                + dy * dpitch + dx * sizeof(unsigned short));
            for (int y = sy; y < sy + sh; y++) {
                unsigned short* out = lineDst;
                unsigned char* src = map + lineOffsets[y];
                unsigned char code;
                unsigned int runLength;
                unsigned int skipped = 0;
                for (;;) {
                    code = *src >> 5;
                    runLength = (*src & 31) + 1;
                    src++;
                    if (skipped + runLength > sx) {
                        runLength -= sx - skipped;
                        if (code == kRawCode)
                            src += sx - skipped;
                        skipped = sx;
                        break;
                    }
                    skipped += runLength;
                    if (code == kRawCode)
                        src += runLength;
                }

                unsigned int remaining = sw;
                for (;;) {
#line 3304
                    assert(runLength > 0);
                    if (runLength > remaining)
                        runLength = remaining;
                    if (code == kRawCode) {
                        out += runLength;
                        src += runLength;
                    }
                    else {
                        switch (code) {
                        case 1:
                        case 2: {
                            unsigned int count = runLength;
                            do {
                                *out = convert555to565(((*out >> 1) & div2mask)
                                                       + ((*out >> 2) & div4mask));
                                    out++;
                            } while (--count);
                            break;
                        }
                        case 3:
                        case 4: {
                            unsigned int count = runLength;
                            do {
                                *out = convert555to565((*out >> 1) & div2mask);
                                    out++;
                            } while (--count);
                            break;
                        }
                        default:
                            out += runLength;
                            break;
                        }
                    }
                    remaining -= runLength;
                    if (remaining == 0)
                        break;
                    code = *src >> 5;
                    runLength = (*src & 31) + 1;
                    src++;
                }
                lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
            }
        }
        else {
            unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
                + dy * dpitch + dx * sizeof(unsigned short) + sw * sizeof(unsigned short));
            for (int y = sy; y < sy + sh; y++) {
                unsigned short* out = lineDst;
                unsigned char* src = map + lineOffsets[y];
                unsigned char code;
                unsigned int runLength;
                unsigned int skipped = 0;
                for (;;) {
                    code = *src >> 5;
                    runLength = (*src & 31) + 1;
                    src++;
                    if (skipped + runLength > sx) {
                        runLength -= sx - skipped;
                        if (code == kRawCode)
                            src += sx - skipped;
                        skipped = sx;
                        break;
                    }
                    skipped += runLength;
                    if (code == kRawCode)
                        src += runLength;
                }

                unsigned int remaining = sw;
                for (;;) {
#line 3399
                    assert(runLength > 0);
                    if (runLength > remaining)
                        runLength = remaining;
                    if (code == kRawCode) {
                        out -= runLength;
                        src += runLength;
                    }
                    else {
                        switch (code) {
                        case 1:
                        case 2: {
                            unsigned int count = runLength;
                            do {
                                --out;
                                    *out = convert555to565(((*out >> 1) & div2mask)
                                                       + ((*out >> 2) & div4mask));
                            } while (--count);
                            break;
                        }
                        case 3:
                        case 4: {
                            unsigned int count = runLength;
                            do {
                                --out;
                                    *out = convert555to565((*out >> 1) & div2mask);
                            } while (--count);
                            break;
                        }
                        default:
                            out -= runLength;
                            break;
                        }
                    }
                    remaining -= runLength;
                    if (remaining == 0)
                        break;
                    code = *src >> 5;
                    runLength = (*src & 31) + 1;
                    src++;
                }
                lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
            }
        }
    }
    else {
        if (!hflip) {
            unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
                + (dy + sh - 1) * dpitch + dx * sizeof(unsigned short));
            for (int y = sy; y < sy + sh; y++) {
                unsigned short* out = lineDst;
                unsigned char* src = map + lineOffsets[y];
                unsigned char code;
                unsigned int runLength;
                unsigned int skipped = 0;
                for (;;) {
                    code = *src >> 5;
                    runLength = (*src & 31) + 1;
                    src++;
                    if (skipped + runLength > sx) {
                        runLength -= sx - skipped;
                        if (code == kRawCode)
                            src += sx - skipped;
                        skipped = sx;
                        break;
                    }
                    skipped += runLength;
                    if (code == kRawCode)
                        src += runLength;
                }

                unsigned int remaining = sw;
                for (;;) {
#line 3497
                    assert(runLength > 0);
                    if (runLength > remaining)
                        runLength = remaining;
                    if (code == kRawCode) {
                        out += runLength;
                        src += runLength;
                    }
                    else {
                        switch (code) {
                        case 1:
                        case 2: {
                            unsigned int count = runLength;
                            do {
                                *out = convert555to565(((*out >> 1) & div2mask)
                                                       + ((*out >> 2) & div4mask));
                                    out++;
                            } while (--count);
                            break;
                        }
                        case 3:
                        case 4: {
                            unsigned int count = runLength;
                            do {
                                *out = convert555to565((*out >> 1) & div2mask);
                                    out++;
                            } while (--count);
                            break;
                        }
                        default:
                            out += runLength;
                            break;
                        }
                    }
                    remaining -= runLength;
                    if (remaining == 0)
                        break;
                    code = *src >> 5;
                    runLength = (*src & 31) + 1;
                    src++;
                }
                lineDst = (unsigned short*)((unsigned char*)lineDst - dpitch);
            }
        }
        else {
            unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
                + (dy + sh - 1) * dpitch + dx * sizeof(unsigned short) + sw * sizeof(unsigned short));
            for (int y = sy; y < sy + sh; y++) {
                unsigned short* out = lineDst;
                unsigned char* src = map + lineOffsets[y];
                unsigned char code;
                unsigned int runLength;
                unsigned int skipped = 0;
                for (;;) {
                    code = *src >> 5;
                    runLength = (*src & 31) + 1;
                    src++;
                    if (skipped + runLength > sx) {
                        runLength -= sx - skipped;
                        if (code == kRawCode)
                            src += sx - skipped;
                        skipped = sx;
                        break;
                    }
                    skipped += runLength;
                    if (code == kRawCode)
                        src += runLength;
                }

                unsigned int remaining = sw;
                for (;;) {
#line 3592
                    assert(runLength > 0);
                    if (runLength > remaining)
                        runLength = remaining;
                    if (code == kRawCode) {
                        out -= runLength;
                        src += runLength;
                    }
                    else {
                        switch (code) {
                        case 1:
                        case 2: {
                            unsigned int count = runLength;
                            do {
                                --out;
                                    *out = convert555to565(((*out >> 1) & div2mask)
                                                       + ((*out >> 2) & div4mask));
                            } while (--count);
                            break;
                        }
                        case 3:
                        case 4: {
                            unsigned int count = runLength;
                            do {
                                --out;
                                    *out = convert555to565((*out >> 1) & div2mask);
                            } while (--count);
                            break;
                        }
                        default:
                            out -= runLength;
                            break;
                        }
                    }
                    remaining -= runLength;
                    if (remaining == 0)
                        break;
                    code = *src >> 5;
                    runLength = (*src & 31) + 1;
                    src++;
                }
                lineDst = (unsigned short*)((unsigned char*)lineDst - dpitch);
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
// name-keyed TU state (docs/vc6/behavior-catalog.md C12): spelling Draw as DC's
// `Draw` (or drawz/drawA) restores every instruction, `Draw`/`drawRle` do not;
// true/false call arguments, uchar Clip flags and dropping the Draw call
// leave it unchanged. The project's normalized names are kept.
void CSpriteFrame::DrawSpellEffect(int sx, int sy, int sw, int sh,
                                   unsigned short* dst, int dx, int dy, int dw,
                                   int dh, int dpitch, TPalette16& pal,
                                   bool hflip, bool alpha) const
{
    if (!alpha) {
        Draw(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, true);
        return;
    }
    if (EncodingMethod == eEncodeTilesetRLE || EncodingMethod == eEncodeRaw) {
        DrawTile(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, false);
        return;
    }
    else if (EncodingMethod == eEncodeAdvObjRLE) {
        DrawHeroAlpha(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip);
        return;
    }

#line 3674
    assert(EncodingMethod == eEncodeGeneralRLE);
    static const unsigned char kRawCode = kGeneralRLEOpaqueRunCode;
    Clip(sx, sy, sw, sh, dx, dy, dw, dh, hflip, false);
    if (sw <= 0 || sh <= 0)
        return;

    const unsigned int* const lineOffsets = (const unsigned int*)map;
    const unsigned short* const palette = pal.m_data;
    if (!hflip) {
        unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
            + dy * dpitch + dx * sizeof(unsigned short));
        for (int y = sy; y < sy + sh; y++) {
            unsigned short* out = lineDst;
            unsigned char* src = map + lineOffsets[y];
            unsigned char code;
            unsigned int runLength;
            unsigned int skipped = 0;
            for (;;) {
                code = *src++;
                runLength = *src++ + 1;
#line 3711
                assert(runLength > 0);
                if (skipped + runLength > sx) {
                    runLength -= sx - skipped;
                    if (code == kRawCode)
                        src += sx - skipped;
                    skipped = sx;
                    break;
                }
                skipped += runLength;
                if (code == kRawCode)
                    src += runLength;
            }

            unsigned int remaining = sw;
            for (;;) {
#line 3729
                assert(runLength > 0);
                if (runLength > remaining)
                    runLength = remaining;
                if (code == kRawCode) {
                    unsigned int count = runLength;
                    do {
                        *out = (div2mask & convert555to565(palette[*src++] >> 1))
                            + ((*out >> 1) & div2mask);
                        out++;
                    } while (--count);
                }
                else {
                    out += runLength;
                }
                remaining -= runLength;
                if (remaining == 0)
                    break;
                code = *src++;
                runLength = *src++ + 1;
            }
            lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
        }
    }
    else {
        unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
            + dy * dpitch + dx * sizeof(unsigned short) + sw * sizeof(unsigned short));
        for (int y = sy; y < sy + sh; y++) {
            unsigned short* out = lineDst;
            unsigned char* src = map + lineOffsets[y];
            unsigned char code;
            unsigned int runLength;
            unsigned int skipped = 0;
            for (;;) {
                code = *src++;
                runLength = *src++ + 1;
#line 3777
                assert(runLength > 0);
                if (skipped + runLength > sx) {
                    runLength -= sx - skipped;
                    if (code == kRawCode)
                        src += sx - skipped;
                    skipped = sx;
                    break;
                }
                skipped += runLength;
                if (code == kRawCode)
                    src += runLength;
            }

            unsigned int remaining = sw;
            for (;;) {
#line 3795
                assert(runLength > 0);
                if (runLength > remaining)
                    runLength = remaining;
                if (code == kRawCode) {
                    unsigned int count = runLength;
                    do {
                        --out;
                        *out = (div2mask & convert555to565(palette[*src++] >> 1))
                            + ((*out >> 1) & div2mask);
                    } while (--count);
                }
                else {
                    out -= runLength;
                }
                remaining -= runLength;
                if (remaining == 0)
                    break;
                code = *src++;
                runLength = *src++ + 1;
            }
            lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
        }
    }
}

// Original: CSpriteFrame::ClipScaled50; cspriteframe.cpp:3949
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
    if (dx + sw > dw) {
        if (hflip)
            sx += dx + sw - dw;
        sw = dw - dx;
    }
    if (dy + sh > dh) {
        if (vflip)
            sy += dy + sh - dh;
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
void CSpriteFrame::DrawAdvObjWithFlagScaled50(int sx, int sy, int sw, int sh,
                                              unsigned short* dst, int dx, int dy,
                                              int dw, int dh, int dpitch,
                                              TPalette16& pal,
                                              unsigned short flagcolor) const
{
#line 3933
    assert(EncodingMethod == eEncodeAdvObjRLE);
    register const unsigned char kRawCode = 7;
    ClipScaled50(sx, sy, sw, sh, dx, dy, dw, dh, false, false);
    if (sw <= 0 || sh <= 0)
        return;

#line 3947
    assert(CroppedWidth % kCellWidth == 0);
    unsigned int cellsPerLine = CroppedWidth / kCellWidth;
    const unsigned short* const cellOffsets = (const unsigned short*)map;
    const unsigned short* const palette = pal.m_data;
    unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
        + dy * dpitch + dx * sizeof(unsigned short));
    for (int y = sy; y < sy + sh; y += 2) {
        unsigned short* out = lineDst;
        unsigned char* src = map + cellOffsets[y * cellsPerLine + sx / kCellWidth];
        unsigned char code;
        unsigned int runLength;
        unsigned int skipped = sx & ~(kCellWidth - 1);
        for (;;) {
            code = *src >> 5;
            runLength = (*src & 31) + 1;
            src++;
            if (skipped + runLength > sx) {
                runLength -= sx - skipped;
                if (code == kRawCode)
                    src += sx - skipped;
                skipped = sx;
                break;
            }
            skipped += runLength;
            if (code == kRawCode)
                src += runLength;
        }

        bool skip = false;
        unsigned int remaining = sw;
        for (;;) {
#line 3990
            assert(runLength > 0);
            if (runLength > remaining)
                runLength = remaining;
            if (code == kRawCode) {
                unsigned int count = runLength;
                if (skip) {
                    src++;
                    count--;
                }
                while (count > 1) {
                    *out++ = convert555to565(palette[*src]);
                    src += 2;
                    count -= 2;
                }
                skip = count != 0;
                if (skip)
                    *out++ = convert555to565(palette[*src++]);
            }
            else {
                switch (code) {
                case 5:
                    if (flagcolor != 0) {
                        unsigned int count = runLength;
                        if (skip)
                            count--;
                        while (count > 1) {
                            *out++ = flagcolor;
                            count -= 2;
                        }
                        skip = count != 0;
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
        lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
    }
}

// Original: CSpriteFrame::DrawAdvObjShadowScaled50; cspriteframe.cpp:4187
void CSpriteFrame::DrawAdvObjShadowScaled50(int sx, int sy, int sw, int sh,
                                            unsigned short* dst, int dx, int dy,
                                            int dw, int dh, int dpitch,
                                            TPalette16& pal) const
{
#line 4066
    assert(EncodingMethod == eEncodeAdvObjRLE);
    register const unsigned char kRawCode = 7;
    ClipScaled50(sx, sy, sw, sh, dx, dy, dw, dh, false, false);
    if (sw <= 0 || sh <= 0)
        return;

#line 4080
    assert(CroppedWidth % kCellWidth == 0);
    unsigned int cellsPerLine = CroppedWidth / kCellWidth;
    const unsigned short* const cellOffsets = (const unsigned short*)map;
    const unsigned short* const palette = pal.m_data;
    unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
        + dy * dpitch + dx * sizeof(unsigned short));
    for (int y = sy; y < sy + sh; y += 2) {
        unsigned short* out = lineDst;
        unsigned char* src = map + cellOffsets[y * cellsPerLine + sx / kCellWidth];
        unsigned char code;
        unsigned int runLength;
        unsigned int skipped = sx & ~(kCellWidth - 1);
        for (;;) {
            code = *src >> 5;
            runLength = (*src & 31) + 1;
            src++;
            if (skipped + runLength > sx) {
                runLength -= sx - skipped;
                if (code == kRawCode)
                    src += sx - skipped;
                skipped = sx;
                break;
            }
            skipped += runLength;
            if (code == kRawCode)
                src += runLength;
        }

        bool skip = false;
        unsigned int remaining = sw;
        for (;;) {
#line 4121
            assert(runLength > 0);
            if (runLength > remaining)
                runLength = remaining;
            if (code == kRawCode) {
                unsigned int count = runLength;
                if (skip)
                    count--;
                out += (count + 1) / 2;
                src += runLength;
                skip = count & 1;
            }
            else {
                switch (code) {
                case 1: {
                    unsigned int count = runLength;
                    if (skip)
                        count--;
                    while (count > 1) {
                        *out = convert555to565(((*out >> 1) & div2mask)
                                               + ((*out >> 2) & div4mask));
                        out++;
                        count -= 2;
                    }
                    skip = count != 0;
                    if (skip) {
                        *out = convert555to565(((*out >> 1) & div2mask)
                                               + ((*out >> 2) & div4mask));
                        out++;
                    }
                    break;
                }
                case 4: {
                    unsigned int count = runLength;
                    if (skip)
                        count--;
                    while (count > 1) {
                        *out = convert555to565((*out >> 1) & div2mask);
                        out++;
                        count -= 2;
                    }
                    skip = count != 0;
                    if (skip) {
                        *out = convert555to565((*out >> 1) & div2mask);
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
        lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
    }
}

// Original: CSpriteFrame::DrawTileScaled50; cspriteframe.cpp:4330
void CSpriteFrame::DrawTileScaled50(int sx, int sy, int sw, int sh,
                                    unsigned short* dst, int dx, int dy,
                                    int dw, int dh, int dpitch,
                                    TPalette16& pal, bool hflip, bool vflip) const
{
#line 4209
    assert(EncodingMethod == eEncodeTilesetRLE || EncodingMethod == eEncodeRaw);
    register const unsigned char kRawCode = 7;
    ClipScaled50(sx, sy, sw, sh, dx, dy, dw, dh, hflip, vflip);
    if (sw <= 0 || sh <= 0)
        return;

    const unsigned short* const lineOffsets = (const unsigned short*)map;
    const unsigned short* const palette = pal.m_data;
    if (!vflip) {
        if (!hflip) {
            unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
                + dy * dpitch + dx * sizeof(unsigned short));
            if (EncodingMethod == eEncodeRaw) {
                unsigned char* line = map + sy * Pitch + sx;
                do {
                    unsigned short* out = lineDst;
                    unsigned char* src = line;
                    int remaining = sw;
                    do {
                        *out++ = convert555to565(palette[*src]);
                        src += 2;
                        remaining -= 2;
                    } while (remaining > 0);
                    lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
                    line += Pitch * 2;
                    sh -= 2;
                } while (sh > 0);
            }
            else {
                for (int y = sy; y < sy + sh; y += 2) {
                    unsigned short* out = lineDst;
                    unsigned char* src = map + lineOffsets[y];
                    unsigned char code;
                    unsigned int runLength;
                    unsigned int skipped = 0;
                    for (;;) {
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                        if (skipped + runLength > sx) {
                            runLength -= sx - skipped;
                            if (code == kRawCode)
                                src += sx - skipped;
                            skipped = sx;
                            break;
                        }
                        skipped += runLength;
                        if (code == kRawCode)
                            src += runLength;
                    }

                    bool skip = false;
                    unsigned int remaining = sw;
                    for (;;) {
#line 4293
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == kRawCode) {
                            unsigned int count = runLength;
                            if (skip) {
                                src++;
                                count--;
                            }
                            while (count > 1) {
                                *out++ = convert555to565(palette[*src]);
                                src += 2;
                                count -= 2;
                            }
                            skip = count != 0;
                            if (skip)
                                *out++ = convert555to565(palette[*src++]);
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
                    lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
                }
            }
        }
        else {
            unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
                + dy * dpitch + dx * sizeof(unsigned short) + (sw >> 1) * sizeof(unsigned short));
            if (EncodingMethod == eEncodeRaw) {
                unsigned char* line = map + sy * Pitch + sx;
                do {
                    unsigned short* out = lineDst;
                    unsigned char* src = line;
                    int remaining = sw;
                    do {
                        *--out = convert555to565(palette[*src]);
                        src += 2;
                        remaining -= 2;
                    } while (remaining > 0);
                    lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
                    line += Pitch * 2;
                    sh -= 2;
                } while (sh > 0);
            }
            else {
                for (int y = sy; y < sy + sh; y += 2) {
                    unsigned short* out = lineDst;
                    unsigned char* src = map + lineOffsets[y];
                    unsigned char code;
                    unsigned int runLength;
                    unsigned int skipped = 0;
                    for (;;) {
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                        if (skipped + runLength > sx) {
                            runLength -= sx - skipped;
                            if (code == kRawCode)
                                src += sx - skipped;
                            skipped = sx;
                            break;
                        }
                        skipped += runLength;
                        if (code == kRawCode)
                            src += runLength;
                    }

                    bool skip = false;
                    unsigned int remaining = sw;
                    for (;;) {
#line 4404
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == kRawCode) {
                            unsigned int count = runLength;
                            if (skip) {
                                src++;
                                count--;
                            }
                            while (count > 1) {
                                *--out = convert555to565(palette[*src]);
                                src += 2;
                                count -= 2;
                            }
                            skip = count != 0;
                            if (skip)
                                *--out = convert555to565(palette[*src++]);
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
                    lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
                }
            }
        }
    }
    else {
        if (!hflip) {
            unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
                + (dy + (sh >> 1) - 1) * dpitch + dx * sizeof(unsigned short));
            if (EncodingMethod == eEncodeRaw) {
                unsigned char* line = map + sy * Pitch + sx;
                do {
                    unsigned short* out = lineDst;
                    unsigned char* src = line;
                    int remaining = sw;
                    do {
                        *out++ = convert555to565(palette[*src]);
                        src += 2;
                        remaining -= 2;
                    } while (remaining > 0);
                    lineDst = (unsigned short*)((unsigned char*)lineDst - dpitch);
                    line += Pitch * 2;
                    sh -= 2;
                } while (sh > 0);
            }
            else {
                for (int y = sy; y < sy + sh; y += 2) {
                    unsigned short* out = lineDst;
                    unsigned char* src = map + lineOffsets[y];
                    unsigned char code;
                    unsigned int runLength;
                    unsigned int skipped = 0;
                    for (;;) {
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                        if (skipped + runLength > sx) {
                            runLength -= sx - skipped;
                            if (code == kRawCode)
                                src += sx - skipped;
                            skipped = sx;
                            break;
                        }
                        skipped += runLength;
                        if (code == kRawCode)
                            src += runLength;
                    }

                    bool skip = false;
                    unsigned int remaining = sw;
                    for (;;) {
#line 4518
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == kRawCode) {
                            unsigned int count = runLength;
                            if (skip) {
                                src++;
                                count--;
                            }
                            while (count > 1) {
                                *out++ = convert555to565(palette[*src]);
                                src += 2;
                                count -= 2;
                            }
                            skip = count != 0;
                            if (skip)
                                *out++ = convert555to565(palette[*src++]);
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
                    lineDst = (unsigned short*)((unsigned char*)lineDst - dpitch);
                }
            }
        }
        else {
            unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
                + (dy + (sh >> 1) - 1) * dpitch + dx * sizeof(unsigned short) + (sw >> 1) * sizeof(unsigned short));
            if (EncodingMethod == eEncodeRaw) {
                unsigned char* line = map + sy * Pitch + sx;
                do {
                    unsigned short* out = lineDst;
                    unsigned char* src = line;
                    int remaining = sw;
                    do {
                        *--out = convert555to565(palette[*src]);
                        src += 2;
                        remaining -= 2;
                    } while (remaining > 0);
                    lineDst = (unsigned short*)((unsigned char*)lineDst - dpitch);
                    line += Pitch * 2;
                    sh -= 2;
                } while (sh > 0);
            }
            else {
                for (int y = sy; y < sy + sh; y += 2) {
                    unsigned short* out = lineDst;
                    unsigned char* src = map + lineOffsets[y];
                    unsigned char code;
                    unsigned int runLength;
                    unsigned int skipped = 0;
                    for (;;) {
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                        if (skipped + runLength > sx) {
                            runLength -= sx - skipped;
                            if (code == kRawCode)
                                src += sx - skipped;
                            skipped = sx;
                            break;
                        }
                        skipped += runLength;
                        if (code == kRawCode)
                            src += runLength;
                    }

                    bool skip = false;
                    unsigned int remaining = sw;
                    for (;;) {
#line 4629
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == kRawCode) {
                            unsigned int count = runLength;
                            if (skip) {
                                src++;
                                count--;
                            }
                            while (count > 1) {
                                *--out = convert555to565(palette[*src]);
                                src += 2;
                                count -= 2;
                            }
                            skip = count != 0;
                            if (skip)
                                *--out = convert555to565(palette[*src++]);
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
                    lineDst = (unsigned short*)((unsigned char*)lineDst - dpitch);
                }
            }
        }
    }
}

// Original: CSpriteFrame::ClipScaled25; cspriteframe.cpp:4802
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
    if (dx + sw > dw) {
        if (hflip)
            sx += dx + sw - dw;
        sw = dw - dx;
    }
    if (dy + sh > dh) {
        if (vflip)
            sy += dy + sh - dh;
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
void CSpriteFrame::DrawAdvObjWithFlagScaled25(int sx, int sy, int sw, int sh,
                                              unsigned short* dst, int dx, int dy,
                                              int dw, int dh, int dpitch,
                                              TPalette16& pal,
                                              unsigned short flagcolor) const
{
#line 4786
    assert(EncodingMethod == eEncodeAdvObjRLE);
    register const unsigned char kRawCode = 7;
    ClipScaled25(sx, sy, sw, sh, dx, dy, dw, dh, false, false);
    if (sw <= 0 || sh <= 0)
        return;

#line 4800
    assert(CroppedWidth % kCellWidth == 0);
    unsigned int cellsPerLine = CroppedWidth / kCellWidth;
    const unsigned short* const cellOffsets = (const unsigned short*)map;
    const unsigned short* const palette = pal.m_data;
    unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
        + dy * dpitch + dx * sizeof(unsigned short));
    for (int y = sy; y < sy + sh; y += 4) {
        unsigned short* out = lineDst;
        unsigned char* src = map + cellOffsets[y * cellsPerLine + sx / kCellWidth];
        unsigned char code;
        unsigned int runLength;
        unsigned int skipped = sx & ~(kCellWidth - 1);
        for (;;) {
            code = *src >> 5;
            runLength = (*src & 31) + 1;
            src++;
            if (skipped + runLength > sx) {
                runLength -= sx - skipped;
                if (code == kRawCode)
                    src += sx - skipped;
                skipped = sx;
                break;
            }
            skipped += runLength;
            if (code == kRawCode)
                src += runLength;
        }

        unsigned int skip = 0;
        unsigned int remaining = sw;
        for (;;) {
#line 4843
            assert(runLength > 0);
            if (runLength > remaining)
                runLength = remaining;
            if (code == kRawCode) {
                if (skip < runLength) {
                    unsigned int count = runLength - skip;
                    src += skip;
                    while (count > 3) {
                        *out++ = convert555to565(palette[*src]);
                        src += 4;
                        count -= 4;
                    }
                    if (count != 0) {
                        *out++ = convert555to565(palette[*src]);
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
                case 5:
                    if (flagcolor != 0) {
                        unsigned int count = runLength - skip;
                        while (count > 3) {
                            *out++ = flagcolor;
                            count -= 2;
                        }
                        if (count != 0) {
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
                    skip = (count & 3) ? 4 - (count & 3) : 0;
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
        lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
    }
}

// Original: CSpriteFrame::DrawAdvObjShadowScaled25; cspriteframe.cpp:5054
void CSpriteFrame::DrawAdvObjShadowScaled25(int sx, int sy, int sw, int sh,
                                            unsigned short* dst, int dx, int dy,
                                            int dw, int dh, int dpitch,
                                            TPalette16& pal) const
{
#line 4933
    assert(EncodingMethod == eEncodeAdvObjRLE);
    register const unsigned char kRawCode = 7;
    ClipScaled25(sx, sy, sw, sh, dx, dy, dw, dh, false, false);
    if (sw <= 0 || sh <= 0)
        return;

#line 4947
    assert(CroppedWidth % kCellWidth == 0);
    unsigned int cellsPerLine = CroppedWidth / kCellWidth;
    const unsigned short* const cellOffsets = (const unsigned short*)map;
    const unsigned short* const palette = pal.m_data;
    unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
        + dy * dpitch + dx * sizeof(unsigned short));
    for (int y = sy; y < sy + sh; y += 4) {
        unsigned short* out = lineDst;
        unsigned char* src = map + cellOffsets[y * cellsPerLine + sx / kCellWidth];
        unsigned char code;
        unsigned int runLength;
        unsigned int skipped = sx & ~(kCellWidth - 1);
        for (;;) {
            code = *src >> 5;
            runLength = (*src & 31) + 1;
            src++;
            if (skipped + runLength > sx) {
                runLength -= sx - skipped;
                if (code == kRawCode)
                    src += sx - skipped;
                skipped = sx;
                break;
            }
            skipped += runLength;
            if (code == kRawCode)
                src += runLength;
        }

        unsigned int skip = 0;
        unsigned int remaining = sw;
        for (;;) {
#line 4988
            assert(runLength > 0);
            if (runLength > remaining)
                runLength = remaining;
            if (code == kRawCode) {
                if (skip < runLength) {
                    unsigned int count = runLength - skip;
                    out += (count + 3) / 4;
                    skip = (count & 3) ? 4 - (count & 3) : 0;
                }
                else {
                    skip -= runLength;
                }
                src += runLength;
            }
            else if (skip < runLength) {
                switch (code) {
                case 1: {
                    unsigned int count = runLength - skip;
                    while (count > 3) {
                        *out = convert555to565(((*out >> 1) & div2mask)
                                               + ((*out >> 2) & div4mask));
                        out++;
                        count -= 4;
                    }
                    if (count != 0) {
                        *out = convert555to565(((*out >> 1) & div2mask)
                                               + ((*out >> 2) & div4mask));
                        skip = 4 - count;
                    }
                    else {
                        skip = 0;
                    }
                    break;
                }
                case 4: {
                    unsigned int count = runLength - skip;
                    while (count > 3) {
                        *out = convert555to565((*out >> 1) & div2mask);
                        out++;
                        count -= 4;
                    }
                    if (count != 0) {
                        *out = convert555to565((*out >> 1) & div2mask);
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
                    skip = (count & 3) ? 4 - (count & 3) : 0;
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
        lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
    }
}

// Original: CSpriteFrame::DrawTileScaled25; cspriteframe.cpp:5201
void CSpriteFrame::DrawTileScaled25(int sx, int sy, int sw, int sh,
                                    unsigned short* dst, int dx, int dy,
                                    int dw, int dh, int dpitch,
                                    TPalette16& pal, bool hflip, bool vflip) const
{
#line 5080
    assert(EncodingMethod == eEncodeTilesetRLE || EncodingMethod == eEncodeRaw);
    register const unsigned char kRawCode = 7;
    ClipScaled25(sx, sy, sw, sh, dx, dy, dw, dh, hflip, vflip);
    if (sw <= 0 || sh <= 0)
        return;

    const unsigned short* const lineOffsets = (const unsigned short*)map;
    const unsigned short* const palette = pal.m_data;
    if (!vflip) {
        if (!hflip) {
            unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
                + dy * dpitch + dx * sizeof(unsigned short));
            if (EncodingMethod == eEncodeRaw) {
                unsigned char* line = map + sy * Pitch + sx;
                do {
                    unsigned short* out = lineDst;
                    unsigned char* src = line;
                    int remaining = sw;
                    do {
                        *out++ = convert555to565(palette[*src]);
                        src += 4;
                        remaining -= 4;
                    } while (remaining > 0);
                    lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
                    line += Pitch * 4;
                    sh -= 4;
                } while (sh > 0);
            }
            else {
                for (int y = sy; y < sy + sh; y += 4) {
                    unsigned short* out = lineDst;
                    unsigned char* src = map + lineOffsets[y];
                    unsigned char code;
                    unsigned int runLength;
                    unsigned int skipped = 0;
                    for (;;) {
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                        if (skipped + runLength > sx) {
                            runLength -= sx - skipped;
                            if (code == kRawCode)
                                src += sx - skipped;
                            skipped = sx;
                            break;
                        }
                        skipped += runLength;
                        if (code == kRawCode)
                            src += runLength;
                    }

                    unsigned int skip = 0;
                    unsigned int remaining = sw;
                    for (;;) {
#line 5164
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == kRawCode) {
                            if (skip < runLength) {
                                unsigned int count = runLength - skip;
                                src += skip;
                                while (count > 3) {
                                    *out++ = convert555to565(palette[*src]);
                                    src += 4;
                                    count -= 4;
                                }
                                if (count != 0) {
                                    *out++ = convert555to565(palette[*src]);
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
                            skip = (count & 3) ? 4 - (count & 3) : 0;
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
                    lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
                }
            }
        }
        else {
            unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
                + dy * dpitch + dx * sizeof(unsigned short) + (sw >> 2) * sizeof(unsigned short));
            if (EncodingMethod == eEncodeRaw) {
                unsigned char* line = map + sy * Pitch + sx;
                do {
                    unsigned short* out = lineDst;
                    unsigned char* src = line;
                    int remaining = sw;
                    do {
                        *--out = convert555to565(palette[*src]);
                        src += 4;
                        remaining -= 4;
                    } while (remaining > 0);
                    lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
                    line += Pitch * 4;
                    sh -= 4;
                } while (sh > 0);
            }
            else {
                for (int y = sy; y < sy + sh; y += 4) {
                    unsigned short* out = lineDst;
                    unsigned char* src = map + lineOffsets[y];
                    unsigned char code;
                    unsigned int runLength;
                    unsigned int skipped = 0;
                    for (;;) {
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                        if (skipped + runLength > sx) {
                            runLength -= sx - skipped;
                            if (code == kRawCode)
                                src += sx - skipped;
                            skipped = sx;
                            break;
                        }
                        skipped += runLength;
                        if (code == kRawCode)
                            src += runLength;
                    }

                    unsigned int skip = 0;
                    unsigned int remaining = sw;
                    for (;;) {
#line 5287
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == kRawCode) {
                            if (skip < runLength) {
                                unsigned int count = runLength - skip;
                                src += skip;
                                while (count > 3) {
                                    *--out = convert555to565(palette[*src]);
                                    src += 4;
                                    count -= 4;
                                }
                                if (count != 0) {
                                    *--out = convert555to565(palette[*src]);
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
                            skip = (count & 3) ? 4 - (count & 3) : 0;
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
                    lineDst = (unsigned short*)((unsigned char*)lineDst + dpitch);
                }
            }
        }
    }
    else {
        if (!hflip) {
            unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
                + (dy + (sh >> 2) - 1) * dpitch + dx * sizeof(unsigned short));
            if (EncodingMethod == eEncodeRaw) {
                unsigned char* line = map + sy * Pitch + sx;
                do {
                    unsigned short* out = lineDst;
                    unsigned char* src = line;
                    int remaining = sw;
                    do {
                        *out++ = convert555to565(palette[*src]);
                        src += 4;
                        remaining -= 4;
                    } while (remaining > 0);
                    lineDst = (unsigned short*)((unsigned char*)lineDst - dpitch);
                    line += Pitch * 4;
                    sh -= 4;
                } while (sh > 0);
            }
            else {
                for (int y = sy; y < sy + sh; y += 4) {
                    unsigned short* out = lineDst;
                    unsigned char* src = map + lineOffsets[y];
                    unsigned char code;
                    unsigned int runLength;
                    unsigned int skipped = 0;
                    for (;;) {
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                        if (skipped + runLength > sx) {
                            runLength -= sx - skipped;
                            if (code == kRawCode)
                                src += sx - skipped;
                            skipped = sx;
                            break;
                        }
                        skipped += runLength;
                        if (code == kRawCode)
                            src += runLength;
                    }

                    unsigned int skip = 0;
                    unsigned int remaining = sw;
                    for (;;) {
#line 5413
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == kRawCode) {
                            if (skip < runLength) {
                                unsigned int count = runLength - skip;
                                src += skip;
                                while (count > 3) {
                                    *out++ = convert555to565(palette[*src]);
                                    src += 4;
                                    count -= 4;
                                }
                                if (count != 0) {
                                    *out++ = convert555to565(palette[*src]);
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
                            skip = (count & 3) ? 4 - (count & 3) : 0;
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
                    lineDst = (unsigned short*)((unsigned char*)lineDst - dpitch);
                }
            }
        }
        else {
            unsigned short* lineDst = (unsigned short*)((unsigned char*)dst
                + (dy + (sh >> 2) - 1) * dpitch + dx * sizeof(unsigned short) + (sw >> 2) * sizeof(unsigned short));
            if (EncodingMethod == eEncodeRaw) {
                unsigned char* line = map + sy * Pitch + sx;
                do {
                    unsigned short* out = lineDst;
                    unsigned char* src = line;
                    int remaining = sw;
                    do {
                        *--out = convert555to565(palette[*src]);
                        src += 4;
                        remaining -= 4;
                    } while (remaining > 0);
                    lineDst = (unsigned short*)((unsigned char*)lineDst - dpitch);
                    line += Pitch * 4;
                    sh -= 4;
                } while (sh > 0);
            }
            else {
                for (int y = sy; y < sy + sh; y += 4) {
                    unsigned short* out = lineDst;
                    unsigned char* src = map + lineOffsets[y];
                    unsigned char code;
                    unsigned int runLength;
                    unsigned int skipped = 0;
                    for (;;) {
                        code = *src >> 5;
                        runLength = (*src & 31) + 1;
                        src++;
                        if (skipped + runLength > sx) {
                            runLength -= sx - skipped;
                            if (code == kRawCode)
                                src += sx - skipped;
                            skipped = sx;
                            break;
                        }
                        skipped += runLength;
                        if (code == kRawCode)
                            src += runLength;
                    }

                    unsigned int skip = 0;
                    unsigned int remaining = sw;
                    for (;;) {
#line 5536
                        assert(runLength > 0);
                        if (runLength > remaining)
                            runLength = remaining;
                        if (code == kRawCode) {
                            if (skip < runLength) {
                                unsigned int count = runLength - skip;
                                src += skip;
                                while (count > 3) {
                                    *--out = convert555to565(palette[*src]);
                                    src += 4;
                                    count -= 4;
                                }
                                if (count != 0) {
                                    *--out = convert555to565(palette[*src]);
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
                            skip = (count & 3) ? 4 - (count & 3) : 0;
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
                    lineDst = (unsigned short*)((unsigned char*)lineDst - dpitch);
                }
            }
        }
    }
}
