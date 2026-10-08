// 18 functions in link order.
#include "va.h"

#include <string.h>

#include "font.h"

#include "bitmap16.h"

// Original: font::font; font.cpp:33
DC_ADDRESS(0x0a1ba8, 0x5c)
font::font() : resource("", RESOURCE_TYPE_FONT), Data(0)
{
}

VA_COMPGEN(0x004b5040, 0x21, SCALAR_DELETING_DTOR, font)

// The resource type is 0x50; the neighbouring proven values are
// text 2, bitmap24 0x11 and sfx 0x20.
VA(0x004b5070, 0x9B)
DC_ADDRESS(0x0a1c04, 0x90)
MAC_ADDRESS(0x0c8cdc, 0xb8)  // anchor-global
font::font(const char* name, const font::TFontSpec& fontspec, int dsize,
           unsigned char* d)
    : resource(name, RESOURCE_TYPE_FONT), fs(fontspec)
{
    Data = new unsigned char[dsize];
    m_dataSize = dsize;
    if (Data)
        memcpy(Data, d, dsize);
}

// Mac 0:0xc8dc8 uses array delete for the new[] glyph buffer.
VA(0x004b5110, 0x67)
DC_ADDRESS(0x0a1c94, 0x4e)
MAC_ADDRESS(0x0c8d94, 0x7c)
font::~font()
{
    if (Data)
        delete[] Data;
}

// E:\gamedcs\font.cpp:56..76. Original name: GetColor.
// The decorated DC member signature uses TColor and bool (_N). Both
// string renderers call this ordinary member; retail expands the custom
// color test and palette bias. Keep the shared return and nested highlight.
DC_ADDRESS(0x0a1ce4, 0x30)
MAC_ADDRESS(0x0c8e10, 0x3c)
int font::GetColor(font::TColor colorScheme, bool highlighted)
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
VA(0x004b5180, 0x16)
DC_ADDRESS(0x0a1d14, 0x44)
MAC_ADDRESS(0x0c8e4c, 0x24)  // anchor-global
void font::SetPalette(const TPalette16& newPalette)
{
    // DC82 calls the reference copy assignment; Complete 0x4b5180 calls
    // the retained pointer assignment at 0x522910, which copies palette
    // data through that canonical overload. Keep the source's ref formal.
    Palette = &newPalette;
}

VA(0x004b51a0, 0xA9)
DC_ADDRESS(0x0a1d58, 0xd6)
MAC_ADDRESS(0x0c8e70, 0xd0)
void font::DrawCharacter(int c, Bitmap16Bit* bmp, int x, int y, int color) const
{
    if (c < 0)
        return;
    if (c >= 256)
        return;
    int width = fs.abc[c].abcB;
    int height = fs.height;
    unsigned char* src = static_cast<unsigned char*>(Data) + fs.Offset[c];
    unsigned char* dst = static_cast<unsigned char*>(
                             static_cast<void*>(bmp->GetMap(0, 0)))
                         + y * bmp->GetPitch() + 2 * (x + fs.abc[c].abcA);
    for (int row = 0; row < height; row++) {
        unsigned short* out = static_cast<unsigned short*>(static_cast<void*>(dst));
        for (int col = 0; col < width; col++) {
            unsigned char pix = *src++;
            if (pix != 0) {
                if (pix == GLYPH_PIXEL_SOLID)
                    *out = Palette.Palette[color];
                else
                    *out = Palette.Palette[32];
            }
            out++;
        }
        dst += bmp->GetPitch();
    }
}

VA(0x004b5250, 0xC)
MAC_ADDRESS(0x0c8f84, 0xc)
unsigned int font::getSize() const
{
    return m_dataSize + sizeof(font);
}

// E:\gamedcs\font.cpp:123..125. Original name: DrawCursor.
// DC proves the ordinary nine-argument member and the underscore draw;
// clip arguments are unused. Retail expands it at the string-rendering
// call sites. The decorated bool (_N) remains the highlight interface.
DC_ADDRESS(0x0a1e30, 0x2c)
MAC_ADDRESS(0x0c8f40, 0x44)
void font::DrawCursor(Bitmap16Bit* bitmap, int x, int y, int color,
                      int clipX, int clipY, int clipWidth, int clipHeight,
                      bool highlighted)
{
    DrawCharacter('_', bitmap, x, y, color + highlighted);
}

// E:\gamedcs\font.cpp:138
VA(0x004b5260, 0x22E)
DC_ADDRESS(0x0a1e5c, 0x240)
MAC_ADDRESS(0x0c8f90, 0x2a8)
void font::DrawStringExecute(const char* text, int count, Bitmap16Bit* bitmap,
                             int x, int y, font::TColor colorScheme, int clipX,
                             int clipY, int clipWidth, int clipHeight,
                             int cursorPos)
{
    int currPos;
    bool highlighted;
    unsigned char c;

    y += fs.baseyoffset;
    if (*text && fs.abc[static_cast<unsigned char>(*text)].abcA < 0)
        x -= fs.abc[static_cast<unsigned char>(*text)].abcA;

    if (y < clipY)
        return;
    if (y + fs.height > clipY + clipHeight)
        return;

    while (count > 0) {
        if (x + fs.abc[static_cast<unsigned char>(*text)].abcA >= clipX)
            break;
        x += GetCharacterWidth(*text);
        text++;
        count--;
    }

    const int color = GetColor(colorScheme, false);

    if (count == 0 && cursorPos != -1) {
        DrawCursor(bitmap, x, y, color, clipX, clipY,
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
            if (x + GetCharacterWidth(c) > clipX + clipWidth)
                break;
            switch (colorScheme) {
            case PRIMARY:
            case WHITE:
            case HEADING:
            case WHITE_PLAYER:
                DrawCharacter(c, bitmap, x, y, color + highlighted);
                if (cursorPos == currPos)
                    DrawCursor(bitmap, x, y, color, clipX, clipY,
                   clipWidth, clipHeight, false);
                break;
            default:
                DrawCharacter(c, bitmap, x, y, color + highlighted);
                if (cursorPos == currPos)
                    DrawCursor(bitmap, x, y, color, clipX, clipY,
                   clipWidth, clipHeight, false);
                break;
            }
            x += GetCharacterWidth(c);
        }
        text++;
        count--;
        currPos++;
    }

    if (cursorPos == currPos)
        DrawCursor(bitmap, x, y, color, clipX, clipY,
                   clipWidth, clipHeight, highlighted);
}

// Original: font::DrawString; font.cpp:246
DC_ADDRESS(0x0a209c, 0x6a)
void font::DrawString(const char* text, Bitmap16Bit* bitmap,
                      int x, int y, TColor color)
{
    DrawStringExecute(text, strlen(text), bitmap, x, y, color,
                      0, 0, bitmap->GetWidth(), bitmap->GetHeight(), -1);
}

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
// The plain bottom-justification `total` is retained. Overflow-only recovery
// locals preserve the canonical wrapping helpers. Separately initialize
// currY before strlen and pos after it: Windows is exact (2026-09-27).
// Twelve declaration/lifetime spellings reproduce Windows exact; this order
// also improves the unresolved Mac comparison (30.20 -> 30.71%, 980/968 bytes).
// A prior volatile probe raised the banked MAX to 98.7864 by
// aligning the scratch-register family, but retail keeps `total` in EAX;
// the qualifier itself forces three non-retail stack-memory instructions.
// Reusing the later-overwritten iHeight or width local is byte-identical
// to the plain block local, so it supplies no independent lever. Tried
// and rejected: volatile `limit`
// (98.28 but one duplicated loop guard), the first `total` (97.24),
// register/long/initializer/address-taken spellings (96.81), and removing
// or bypassing the loop guard (95-96% allocation cascades).
VA(0x004b5490, 0x308)
DC_ADDRESS(0x0a2108, 0x316)
MAC_ADDRESS(0x0c9238, 0x3c8)  // anchor-global
void font::DrawBoundedString(const char* str, Bitmap16Bit* bitmap, int x,
                             int y, int boxWidth, int boxHeight,
                             font::TColor colorScheme, unsigned justification,
                             int cursorPos)
{
    int currY;
    int pos;
    int limit;
    int lineStart;
    int height;
    int width;

    if (!str)
        return;
    currY = 0;
    limit = strlen(str);
    pos = 0;
    if (limit == 0) {
        if (cursorPos != -1) {
            DrawCursor(bitmap, x, y, GetColor(colorScheme, false),
                       x, y, boxWidth, boxHeight, false);
        }
        return;
    }

    if (justification & VERT_CENTER_JUSTIFIED) {
        int total;

        justification &= ~VERT_CENTER_JUSTIFIED;
        height = fs.height;
        total = LineLength(str, boxWidth) * height;
        if (total < boxHeight)
            currY = (boxHeight - total) / 2;
        else if (boxHeight < height * 2)
            currY = (boxHeight - height) / 2;
    }
    if (justification & BOTTOM_JUSTIFIED) {
        int total;

        justification &= ~BOTTOM_JUSTIFIED;
        total = LineLength(str, boxWidth) * fs.height;
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
        height = fs.height;
        if (currY + height > boxHeight && currY != 0)
            return;
        width = 0;
        lineStart = pos;
        while (str[pos] == '{' || str[pos] == '}')
            ++pos;
        if (str[pos] != 0
            && fs.abc[static_cast<unsigned char>(str[pos])].abcA < 0)
            width = -fs.abc[str[pos]].abcA;
        while (str[pos] != 0 && str[pos] != '\n' && width <= boxWidth) {
            if (str[pos] != '{' && str[pos] != '}')
                width += GetCharacterWidth(str[pos]);
            ++pos;
        }
        k = pos - 1;
        while ((str[k] == '{' || str[k] == '}') && k > lineStart)
            --k;
        if (pos > 0 && fs.abc[static_cast<unsigned char>(str[k])].abcC < 0)
            width -= fs.abc[static_cast<unsigned char>(str[k])].abcC;
        if (width > boxWidth) {
            int origPixelWidth = width;
            int okWidthIndex = 0;
            if (fs.abc[static_cast<unsigned char>(str[k])].abcC < 0)
                width += fs.abc[str[k]].abcC;
            pos = k;
            for (;;) {
                if (str[pos] == ' ')
                    break;
                if (pos < lineStart)
                    break;
                if (str[pos] != '{' && str[pos] != '}') {
                    width -= GetCharacterWidth(str[pos]);
                    if (currY + 2 * height > boxHeight && width < boxWidth)
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
                width -= GetCharacterWidth(' ');
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
        DrawStringExecute(str + lineStart, pos - lineStart, bitmap, x + currX,
                          y + currY, colorScheme, x, y, boxWidth, boxHeight,
                          cursorPos);
        currY += fs.height;
        ++pos;
    }
}

// The ordinary ABC sum order matches both retained retail bodies exactly.
VA(0x004b57a0, 0x25)
DC_ADDRESS(0x0a2420, 0x18)
MAC_ADDRESS(0x0c9600, 0x28)
int font::GetCharacterWidth(unsigned char currChar) const
{
    const TFontSpec::myABC* record = &fs.abc[currChar];
    return record->abcA + record->abcB + record->abcC;
}

VA(0x004b57d0, 0x44)
DC_ADDRESS(0x0a2438, 0x34)
MAC_ADDRESS(0x0c9628, 0x68)
long font::get_string_width(const char* arg) const
{
    long width = 0;
    for (const char* p = arg; *p;)
        width += GetCharacterWidth(*p++);
    return width;
}

VA(0x004b5820, 0xF2)
DC_ADDRESS(0x0a246c, 0xe6)
MAC_ADDRESS(0x0c9690, 0x14c)
int font::LineLength(const char* str, int boxWidth) const
{
    int limit = strlen(str);
    int count = 0;
    int pos = 0;
    while (pos < limit && str[pos] != 0) {
        int width = 0;
        int lineStart = pos;
        while (str[pos] != 0 && str[pos] != '\n' && width <= boxWidth) {
            if (str[pos] != '{' && str[pos] != '}')
                width += GetCharacterWidth(str[pos]);
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
                    width -= GetCharacterWidth(str[pos]);
                    if (candidate == 0 && width < boxWidth)
                        candidate = pos;
                }
            }
            if (pos <= lineStart)
                pos = candidate;
            if (str[pos] == ' ')
                width -= GetCharacterWidth(' ');
        }
        pos++;
        count++;
    }
    return count;
}

VA(0x004b5920, 0x64)
DC_ADDRESS(0x0a2554, 0x74)
MAC_ADDRESS(0x0c97dc, 0xac)
int font::LineWidth(const char* text) const
{
    int len = strlen(text);
    int idx = 0;
    int width = 0;
    while (idx < len && text[idx] != 0) {
        while (text[idx] != 0 && text[idx] != '\n') {
            if (text[idx] != '{' && text[idx] != '}')
                width += GetCharacterWidth(text[idx]);
            idx++;
        }
    }
    return width;
}

// Ordinary len/pos/best declaration order matches both Windows and Mac;
// the native difference was seven uses of the swapped best/pos registers.
VA(0x004b5990, 0x76)
DC_ADDRESS(0x0a25c8, 0x86)
MAC_ADDRESS(0x0c9888, 0xc4)
int font::LongestLineWidth(const char* str) const
{
    int len = strlen(str);
    int pos = 0;
    int best = 0;
    while (pos < len && str[pos] != 0) {
        int lineWidth = 0;
        while (str[pos] != 0 && str[pos] != '\n') {
            if (str[pos] != '{' && str[pos] != '}')
                lineWidth += GetCharacterWidth(str[pos]);
            pos++;
        }
        if (lineWidth > best)
            best = lineWidth;
        pos++;
    }
    return best;
}

VA(0x004b5a10, 0x6F)
DC_ADDRESS(0x0a2650, 0x84)
MAC_ADDRESS(0x0c994c, 0xd4)
int font::longest_word_length(const char* str) const
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
                    wordWidth += GetCharacterWidth(*p);
                p++;
            }
            if (wordWidth > best)
                best = wordWidth;
        } while (*p != 0);
    }
    return best;
}

VA(0x004b5a80, 0x110)
DC_ADDRESS(0x0a26d4, 0xf0)
MAC_ADDRESS(0x0c9a20, 0x158)
int font::LongestWrappedLineWidth(const char* str, int boxWidth) const
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
                width += GetCharacterWidth(str[pos]);
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
                    width -= GetCharacterWidth(str[pos]);
                    if (candidate == 0 && width < boxWidth)
                        candidate = pos;
                }
                pos--;
            }
            if (pos <= lineStart)
                pos = candidate;
            if (str[pos] == ' ')
                width -= GetCharacterWidth(' ');
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
// Mac 0xc9d04..0xc9d08 adds line width before subtracting word width;
// 0xc9d74..0xc9d7c counts pending spaces down. Both source forms restore
// the Windows retail body while retaining every string/vector helper.
// Mac's local register order also fixes the entry declarations below: p,
// wordEnd, wordWidth, lineWidth, spaceCount, spaceWidth. Windows stays exact.
VA(0x004b5b90, 0x3A5)
MAC_ADDRESS(0x0c9b78, 0x270)  // anchor-member (fs.abc[' '] at this+0x1bc), retail-only
void font::fillLinesVector(const char* str, int boxWidth,
                           std::vector<std::string>& result)
{
    const char* p;
    const char* wordEnd;
    int wordWidth;
    std::string line;
    int lineWidth = 0;
    int spaceCount;
    int spaceWidth;
    line = "";
    p = str;
    result.clear();
    while (*p != 0) {
        spaceWidth = 0;
        spaceCount = 0;
        int blankWidth = GetCharacterWidth(' ');
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
        wordWidth = 0;
        wordEnd = p;
        while (*wordEnd != 0 && *wordEnd != ' ' && *wordEnd != '\n') {
            wordWidth += GetCharacterWidth(*wordEnd);
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
                    int charWidth = GetCharacterWidth(*p);
                    if (lineWidth + charWidth > boxWidth)
                        break;
                    line += *p;
                    lineWidth += charWidth;
                    wordWidth -= charWidth;
                    p++;
                }
                result.push_back(line);
                line = "";
                lineWidth = 0;
            }
        }
        while (spaceCount--)
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
