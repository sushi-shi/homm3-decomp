// font.cpp - E:\gamedcs\font.cpp (compiland font.obj)
// 18 functions in link order.
#include <va.h>
#include <string.h>
#include "font.h"
#include "bitmap16.h"

// The sample.obj lever (src/sample.cpp), needed here for the opposite
// reason: with the palette a real member, a THROWING `delete data` forces
// ~font's ENTRY unwind state up to 1, because the palette would have to be
// unwound out of the body. Retail's ~font enters at state 0 - the resource
// base alone - which is the shape a nothrow-visible operator delete gives,
// since the first throwing point then IS the palette destructor, by which
// time the palette is already being destroyed.
__declspec(nothrow) void __cdecl operator delete(void* p);

#if 0  // @carcass

// E:\gamedcs\font.cpp:33
// The default constructor has NO retail row: font.obj's carved span runs
// 0x4b5020..0x4b5b90 and every row in it is accounted for below, with the
// 32-byte 0x4b5020 an atexit/guard-byte cinit thunk (excluded class).
// Either the retail source dropped it or its single use inlined it away.
DC_ONLY(0xa1ba8, 0x5C)
void font::font()
{
    // @stub
}

#endif  // @carcass

VA_COMPGEN(0x004b5040, 0x21, SCALAR_DELETING_DTOR, font)

// The resource type is 0x50; the neighbouring proven values are
// text 2, bitmap24 0x11 and sfx 0x20.
VA(0x004b5070, 0x9B)  // anchor-global, dc 0xa1c04
font::font(const char* name, const font::FontSpec& fontspec, int dsize,
           unsigned char* d)
    : resource(name, RESOURCE_TYPE_FONT), m_fs(fontspec)
{
    m_data = new unsigned char[dsize];
    m_dataSize = dsize;
    if (m_data)
        memcpy(m_data, d, dsize);
}

VA(0x004b5110, 0x67)  // dc 0xa1c94
font::~font()
{
    if (m_data)
        delete m_data;
}

// E:\gamedcs\font.cpp:56..76. Original name: GetColor.
// The decorated DC member signature uses TColor and bool (_N). Both
// string renderers call this ordinary member; retail expands the custom
// color test and palette bias. Keep the shared return and nested highlight.
int font::getColor(font::Color colorScheme, bool highlighted)
{
    int color;
    if (!(colorScheme & CUSTOM_COLOR)) {
        color = colorScheme + 9;
        if (highlighted && (colorScheme == PRIMARY || colorScheme == WHITE
                            || colorScheme == HEADING)) {
            color++;
        }
    } else {
        color = colorScheme & ~CUSTOM_COLOR;
    }
    return color;
}

// E:\gamedcs\font.cpp:81
VA(0x004b5180, 0x16)  // anchor-global, dc 0xa1d14
void font::setPalette(const Palette16& newPalette)
{
    // DC82 calls the reference copy assignment; Complete 0x4b5180 calls
    // the retained pointer assignment at 0x522910, which copies palette
    // data through that canonical overload. Keep the source's ref formal.
    m_palette = &newPalette;
}

VA(0x004b51a0, 0xA9)  // dc 0xa1d58
void font::drawCharacter(int c, Bitmap16Bit* bmp, int x, int y, int color) const
{
    if (c < 0)
        return;
    if (c >= 256)
        return;
    int width = m_fs.m_abc[c].m_abcB;
    int height = m_fs.m_height;
    unsigned char* src = static_cast<unsigned char*>(m_data) + m_fs.m_offset[c];
    unsigned char* dst = static_cast<unsigned char*>(
                             static_cast<void*>(bmp->getMap(0, 0)))
                         + y * bmp->getPitch() + 2 * (x + m_fs.m_abc[c].m_abcA);
    for (int row = 0; row < height; row++) {
        unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(dst));
        for (int col = 0; col < width; col++) {
            unsigned char pix = *src++;
            if (pix != 0) {
                if (pix == GLYPH_PIXEL_SOLID)
                    *out = m_palette.m_data[color];
                else
                    *out = m_palette.m_data[32];
            }
            out++;
        }
        dst += bmp->getPitch();
    }
}

VA(0x004b5250, 0xC)
unsigned int font::getSize() const
{
    return m_dataSize + sizeof(font);
}

// E:\gamedcs\font.cpp:123..125. Original name: DrawCursor.
// DC proves the ordinary nine-argument member and the underscore draw;
// clip arguments are unused. Retail expands it at the string-rendering
// call sites. The decorated bool (_N) remains the highlight interface.
void font::drawCursor(Bitmap16Bit* bitmap, int x, int y, int color,
                      int clipX, int clipY, int clipWidth, int clipHeight,
                      bool highlighted)
{
    drawCharacter('_', bitmap, x, y, color + highlighted);
}

// E:\gamedcs\font.cpp:138, dc 0xa1e5c
VA(0x004b5260, 0x22E)  // dc 0xa1e5c
void font::drawStringExecute(const char* text, int count, Bitmap16Bit* bitmap,
                             int x, int y, font::Color colorScheme, int clipX,
                             int clipY, int clipWidth, int clipHeight,
                             int cursorPos)
{
    int currPos;
    bool highlighted;
    unsigned char c;

    y += m_fs.m_baseyoffset;
    if (*text && m_fs.m_abc[static_cast<unsigned char>(*text)].m_abcA < 0)
        x -= m_fs.m_abc[static_cast<unsigned char>(*text)].m_abcA;

    if (y < clipY)
        return;
    if (y + m_fs.m_height > clipY + clipHeight)
        return;

    while (count > 0) {
        if (x + m_fs.m_abc[static_cast<unsigned char>(*text)].m_abcA >= clipX)
            break;
        x += getCharacterWidth(*text);
        text++;
        count--;
    }

    const int color = getColor(colorScheme, false);

    if (count == 0 && cursorPos != -1) {
        drawCursor(bitmap, x, y, color, clipX, clipY,
                   clipWidth, clipHeight, false);
        return;
    }

    highlighted = 0;
    currPos = 0;
    while (count > 0) {
        c = *text;
        if (c == '{') {
            highlighted = 1;
        } else if (c == '}') {
            highlighted = 0;
        } else {
            if (x + getCharacterWidth(c) > clipX + clipWidth)
                break;
            switch (colorScheme) {
            case PRIMARY:
            case WHITE:
            case HEADING:
            case WHITE_PLAYER:
                drawCharacter(c, bitmap, x, y, color + highlighted);
                if (cursorPos == currPos)
                    drawCursor(bitmap, x, y, color, clipX, clipY,
                   clipWidth, clipHeight, false);
                break;
            default:
                drawCharacter(c, bitmap, x, y, color + highlighted);
                if (cursorPos == currPos)
                    drawCursor(bitmap, x, y, color, clipX, clipY,
                   clipWidth, clipHeight, false);
                break;
            }
            x += getCharacterWidth(c);
        }
        text++;
        count--;
        currPos++;
    }

    if (cursorPos == currPos)
        drawCursor(bitmap, x, y, color, clipX, clipY,
                   clipWidth, clipHeight, highlighted);
}

#if 0  // @carcass

// E:\gamedcs\font.cpp:246
DC_ONLY(0xa209c, 0x6A)
void font::DrawString(const char* text, Bitmap16Bit* bitmap, int x, int y, font::Color color)
{
    // @stub
}

// E:\gamedcs\font.cpp:254
#endif  // @carcass

// The layout pass: split `str` into lines that fit boxWidth, place the
// block vertically per the justification bits, and hand each line to
// DrawStringExecute. Locals carry the Dreamcast names (limit,
// lineStart, iOrigPixelWidth, okWidthIndex); `{` and `}` are the
// in-string colour markers and are weightless everywhere.
// Signed/unsigned note, byte-forced throughout: every BEARING TEST
// indexes spec[] with the character zero-extended, while the bearing
// VALUE that follows indexes with the plain (signed) char - `and
// eax,0xff` against `movsx`, in three separate places.
// Lever found here: NAMING the scanned character costs 10 points. A
// `char c = str[pos];` local gets a stack home and a reload at every
// use (VC6 homes char locals far more eagerly than ints); writing
// `str[pos]` at each use lets the same load stay in `al` exactly as
// retail does. 86.9% -> 96.3% for that change alone.
// The claim that this was "register allocation only" was WRONG until
// 2026-08-08: `--branches` reported a TOPOLOGY retarget, the
// wrap-backtrack's `pos < lineStart` exit landing one block past
// retail's. Spelling the backtrack as `for (;;) { if (str[pos] == ' ')
// break; if (pos < lineStart) break; ... }` - the same two-break shape
// that closed LongestWrappedLineWidth - routes BOTH exits through the
// shared `if (pos <= lineStart)` instead of letting VC6 jump-thread
// the lineStart exit straight to the assignment (96.33 -> 96.81, and
// the branch sequences now AGREE). `while (str[pos] != ' ' &&
// pos >= lineStart)` scores the same; `pos <= lineStart` as the break
// condition is worse (96.59).
// The source-faithful plain bottom-justification `total` is retained at
// 96.8123. A prior volatile probe raised the banked MAX to 98.7864 by
// aligning the scratch-register family, but retail keeps `total` in EAX;
// the qualifier itself forces three non-retail stack-memory instructions.
// Reusing the later-overwritten iHeight or width local is byte-identical
// to the plain block local, so it supplies no independent lever. Tried
// and rejected: volatile `limit`
// (98.28 but one duplicated loop guard), the first `total` (97.24),
// register/long/initializer/address-taken spellings (96.81), and removing
// or bypassing the loop guard (95-96% allocation cascades).

VA(0x004b5490, 0x308)  // anchor-global, dc 0xa2108
void font::drawBoundedString(const char* str, Bitmap16Bit* bitmap, int x,
                             int y, int boxWidth, int boxHeight,
                             font::Color colorScheme, unsigned justification,
                             int cursorPos)
{
    int pos = 0;
    int limit;
    int currY;
    int lineStart;
    int okWidthIndex;
    int origPixelWidth;
    int height;
    int width;

    if (!str)
        return;
    currY = 0;
    limit = strlen(str);
    if (limit == 0) {
        if (cursorPos != -1) {
            drawCursor(bitmap, x, y, getColor(colorScheme, false),
                       x, y, boxWidth, boxHeight, false);
        }
        return;
    }

    if (justification & VERT_CENTER_JUSTIFIED) {
        int total;

        justification &= ~VERT_CENTER_JUSTIFIED;
        height = m_fs.m_height;
        total = lineLength(str, boxWidth) * height;
        if (total < boxHeight)
            currY = (boxHeight - total) / 2;
        else if (boxHeight < height * 2)
            currY = (boxHeight - height) / 2;
    }
    if (justification & BOTTOM_JUSTIFIED) {
        int total;

        justification &= ~BOTTOM_JUSTIFIED;
        total = lineLength(str, boxWidth) * m_fs.m_height;
        if (total < boxHeight)
            currY = boxHeight - total;
    }
    if (limit <= 0)
        return;

    while (pos < limit) {
        int k;
        int currX;

        if (str[pos] == 0)
            return;
        height = m_fs.m_height;
        if (currY + height > boxHeight && currY != 0)
            return;
        width = 0;
        lineStart = pos;
        while (str[pos] == '{' || str[pos] == '}')
            ++pos;
        if (str[pos] != 0
            && m_fs.m_abc[static_cast<unsigned char>(str[pos])].m_abcA < 0)
            width = -m_fs.m_abc[str[pos]].m_abcA;
        while (str[pos] != 0 && str[pos] != '\n' && width <= boxWidth) {
            if (str[pos] != '{' && str[pos] != '}')
                width += getCharacterWidth(str[pos]);
            ++pos;
        }
        k = pos - 1;
        while ((str[k] == '{' || str[k] == '}') && k > lineStart)
            --k;
        if (pos > 0 && m_fs.m_abc[static_cast<unsigned char>(str[k])].m_abcC < 0)
            width -= m_fs.m_abc[static_cast<unsigned char>(str[k])].m_abcC;
        if (width > boxWidth) {
            origPixelWidth = width;
            okWidthIndex = 0;
            if (m_fs.m_abc[static_cast<unsigned char>(str[k])].m_abcC < 0)
                width += m_fs.m_abc[str[k]].m_abcC;
            pos = k;
            for (;;) {
                if (str[pos] == ' ')
                    break;
                if (pos < lineStart)
                    break;
                if (str[pos] != '{' && str[pos] != '}') {
                    width -= getCharacterWidth(str[pos]);
                    if (height * 2 + currY > boxHeight && width < boxWidth)
                        break;
                    if (okWidthIndex == 0 && width < boxWidth)
                        okWidthIndex = pos;
                }
                --pos;
            }
            if (pos <= lineStart) {
                pos = okWidthIndex;
                width = origPixelWidth;
            }
            if (str[pos] == ' ')
                width -= getCharacterWidth(' ');
        }
        currX = 0;
        switch (justification) {
        case LEFT_JUSTIFIED:
            currX = 0;
            break;
        case CENTER_JUSTIFIED:
            currX = (boxWidth - width) / 2;
            break;
        case RIGHT_JUSTIFIED:
            currX = boxWidth - width;
            break;
        }
        drawStringExecute(str + lineStart, pos - lineStart, bitmap, x + currX,
                          y + currY, colorScheme, x, y, boxWidth, boxHeight,
                          cursorPos);
        currY += m_fs.m_height;
        ++pos;
    }
}

VA(0x004b57a0, 0x25)  // dc 0xa2420
int font::getCharacterWidth(unsigned char currChar) const
{
    const FontSpec::myABC* record = &m_fs.m_abc[currChar];
    return record->m_abcB + record->m_abcC + record->m_abcA;
}

VA(0x004b57d0, 0x44)  // dc 0xa2438
long font::getStringWidth(const char* arg) const
{
    long width = 0;
    for (const char* p = arg; *p;)
        width += getCharacterWidth(*p++);
    return width;
}

VA(0x004b5820, 0xF2)  // dc 0xa246c
int font::lineLength(const char* str, int boxWidth) const
{
    int limit = strlen(str);
    int count = 0;
    int pos = 0;
    while (pos < limit && str[pos] != 0) {
        int width = 0;
        int lineStart = pos;
        while (str[pos] != 0 && str[pos] != '\n' && width <= boxWidth) {
            if (str[pos] != '{' && str[pos] != '}')
                width += getCharacterWidth(str[pos]);
            pos++;
        }
        if (width > boxWidth) {
            int candidate = 0;
            for (;;) {
                pos--;
                if (str[pos] == ' ')
                    break;
                if (pos < lineStart)
                    break;
                if (str[pos] != '{' && str[pos] != '}') {
                    width -= getCharacterWidth(str[pos]);
                    if (candidate == 0 && width < boxWidth)
                        candidate = pos;
                }
            }
            if (pos <= lineStart)
                pos = candidate;
            if (str[pos] == ' ')
                width -= getCharacterWidth(' ');
        }
        pos++;
        count++;
    }
    return count;
}

VA(0x004b5920, 0x64)  // dc 0xa2554
int font::lineWidth(const char* text) const
{
    int len = strlen(text);
    int idx = 0;
    int width = 0;
    while (idx < len && text[idx] != 0) {
        while (text[idx] != 0 && text[idx] != '\n') {
            if (text[idx] != '{' && text[idx] != '}')
                width += getCharacterWidth(text[idx]);
            idx++;
        }
    }
    return width;
}

VA(0x004b5990, 0x76)  // dc 0xa25c8
int font::longestLineWidth(const char* str) const
{
    int len = strlen(str);
    int best = 0;
    int pos = 0;
    while (pos < len && str[pos] != 0) {
        int lineWidth = 0;
        while (str[pos] != 0 && str[pos] != '\n') {
            if (str[pos] != '{' && str[pos] != '}')
                lineWidth += getCharacterWidth(str[pos]);
            pos++;
        }
        if (lineWidth > best)
            best = lineWidth;
        pos++;
    }
    return best;
}

VA(0x004b5a10, 0x6F)  // dc 0xa2650
int font::longestWordLength(const char* str) const
{
    int best = 0;
    const char* p = str;
    if (*p != 0) {
        do {
            int wordWidth = 0;
            while (*p == ' ' || *p == '\n')
                p++;
            while (*p != 0 && *p != ' ' && *p != '\n') {
                if (*p != '{' && *p != '}')
                    wordWidth += getCharacterWidth(*p);
                p++;
            }
            if (wordWidth > best)
                best = wordWidth;
        } while (*p != 0);
    }
    return best;
}

VA(0x004b5a80, 0x110)  // dc 0xa26d4
int font::longestWrappedLineWidth(const char* str, int boxWidth) const
{
    int limit = strlen(str);
    int maxWidth = 0;
    int pos = 0;
    while (pos < limit && str[pos] != 0) {
        int width = 0;
        int lineStart = pos;
        while (str[pos] != 0) {
            if (str[pos] == '\n')
                break;
            if (width > boxWidth)
                break;
            if (str[pos] != '{' && str[pos] != '}')
                width += getCharacterWidth(str[pos]);
            pos++;
        }
        if (width > boxWidth) {
            int candidate = 0;
            pos--;
            for (;;) {
                if (str[pos] == ' ')
                    break;
                if (pos < lineStart)
                    break;
                if (str[pos] != '{' && str[pos] != '}') {
                    width -= getCharacterWidth(str[pos]);
                    if (candidate == 0 && width < boxWidth)
                        candidate = pos;
                }
                pos--;
            }
            if (pos <= lineStart)
                pos = candidate;
            if (str[pos] == ' ')
                width -= getCharacterWidth(' ');
        }
        if (width > maxWidth)
            maxWidth = width;
        pos++;
    }
    return maxWidth;
}

// E:\gamedcs\font.cpp - font.obj's tail, and the only member that
// produces text rather than measuring it: it word-wraps `str` into
// `result` at `boxWidth` pixels. Retail proves the receiver outright -
// the space width is `fs.abc[' ']` read as this+0x1bc/0x1c0/0x1c4 - and
// NH3API corroborates the name and the parameter shape only.

VA(0x004b5b90, 0x3A5)  // anchor-member (fs.abc[' '] at this+0x1bc), retail-only
void font::fillLinesVector(const char* str, int boxWidth,
                           std::vector<std::string>& result)
{
    int lineWidth = 0;
    std::string line;
    line = "";
    const char* p = str;
    result.clear();
    while (*p != 0) {
        int spaceWidth = 0;
        int spaceCount = 0;
        int blankWidth = getCharacterWidth(' ');
        while (*p == ' ' || *p == '\n') {
            if (*p == '\n') {
                result.push_back(line);
                line = "";
                lineWidth = 0;
                spaceCount = 0;
                spaceWidth = 0;
            } else {
                spaceCount++;
                spaceWidth += blankWidth;
            }
            p++;
        }
        int wordWidth = 0;
        const char* wordEnd = p;
        while (*wordEnd != 0 && *wordEnd != ' ' && *wordEnd != '\n') {
            wordWidth += getCharacterWidth(*wordEnd);
            wordEnd++;
        }
        if (spaceWidth + wordWidth + lineWidth > boxWidth) {
            if (lineWidth > 0) {
                result.push_back(line);
                line = "";
                lineWidth = 0;
            }
            spaceCount = 0;
            spaceWidth = 0;
            while (wordWidth > boxWidth) {
                while (*p != 0 && *p != ' ' && *p != '\n') {
                    int charWidth = getCharacterWidth(*p);
                    if (lineWidth + charWidth > boxWidth)
                        break;
                    line += *p;
                    wordWidth -= charWidth;
                    lineWidth += charWidth;
                    p++;
                }
                result.push_back(line);
                line = "";
                lineWidth = 0;
            }
        }
        for (int space = 0; space != spaceCount; space++)
            line += ' ';
        lineWidth += spaceWidth;
        while (p != wordEnd) {
            line += *p;
            p++;
        }
        lineWidth += wordWidth;
    }
    if (lineWidth > 0)
        result.push_back(line);
}

// The vector<string> range erase `result.clear()` reaches, retained as a
// font.obj COMDAT because this is the only TU that clears one.
VA_COMPGEN(0x004B6010, 0x175, VECTOR_ERASE, string)

#if 0  // @carcass

// E:\gamedcs\font.cpp:35
DC_ONLY(0xa27c4, 0x34)
void* font::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

#endif  // @carcass
