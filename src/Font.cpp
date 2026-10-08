// Font.cpp - font (Loki h3maped object 60).
//
// The Loki link order; g++ then emits the inline GetPalette.
#include "va.h"

#include "font.h"

#include <assert.h>
#include <string.h>

#include <stdexcept>
#include "bitmap16.h"

DC_ADDRESS(0x0a1ba8, 0x5c)
font::font() : resource("", RESOURCE_TYPE_FONT), m_data(0)
{
}

VA(0x004b5070, 0x9B)
DC_ADDRESS(0x0a1c04, 0x90)
MAC_ADDRESS(0x0c8cdc, 0xb8)
font::font(const char* name, const font::TFontSpec& fontspec, int dsize,
           unsigned char* d)
    : resource(name, RESOURCE_TYPE_FONT), m_fs(fontspec)
{
    m_data = new unsigned char[dsize];
    if (m_data)
        memcpy(m_data, d, dsize);
}

VA(0x004b5110, 0x67)
DC_ADDRESS(0x0a1c94, 0x4e)
MAC_ADDRESS(0x0c8d94, 0x7c)
font::~font()
{
    if (m_data)
        delete[] m_data;
}

DC_ADDRESS(0x0a1ce4, 0x30)
MAC_ADDRESS(0x0c8e10, 0x3c)
int font::GetColor(font::TColor colorScheme, bool highlighted)
{
    int color;
    if (!(colorScheme & CUSTOM_COLOR)) {
        color = colorScheme + 9;
        if (highlighted && (colorScheme == PRIMARY || colorScheme == WHITE
                            || colorScheme == HEADING))
            color++;
    } else {
        color = colorScheme & ~CUSTOM_COLOR;
    }
    return color;
}

VA(0x004b5180, 0x16)
DC_ADDRESS(0x0a1d14, 0x44)
MAC_ADDRESS(0x0c8e4c, 0x24)
void font::SetPalette(const TPalette16& newPalette)
{
    m_palette = newPalette;
}

VA(0x004b51a0, 0xA9)
DC_ADDRESS(0x0a1d58, 0xd6)
MAC_ADDRESS(0x0c8e70, 0xd0)
void font::DrawCharacter(int c, Bitmap16Bit* bmp, int x, int y, int color) const
{
#line 79
    assert(c>=0 && c<256);
    if (c < 0 || c >= 256)
        return;
    int width = m_fs.m_abc[c].m_abcB;
    int height = m_fs.m_height;
    unsigned char* src = (unsigned char*)m_data + m_fs.m_offset[c];
    unsigned char* dst = (unsigned char*)bmp->GetMap(0, 0)
                         + y * bmp->GetPitch() + 2 * x;
    dst += 2 * m_fs.m_abc[c].m_abcA;
    for (int row = 0; row < height; row++) {
        unsigned short* out = (unsigned short*)dst;
        for (int col = 0; col < width; col++) {
            unsigned char pix = *src++;
            if (pix != 0) {
                if (pix == GLYPH_PIXEL_SOLID)
                    *out = m_palette.m_data[color];
                else
                    *out = m_palette.m_data[kFontShadowColor];
            }
            out++;
        }
        dst += bmp->GetPitch();
    }
}

DC_ADDRESS(0x0a1e30, 0x2c)
MAC_ADDRESS(0x0c8f40, 0x44)
void font::DrawCursor(Bitmap16Bit* bitmap, int x, int y, int color,
                      int clipX, int clipY, int clipWidth, int clipHeight,
                      bool highlighted)
{
    DrawCharacter('_', bitmap, x, y, color + highlighted);
}

VA(0x004b5260, 0x22E)
DC_ADDRESS(0x0a1e5c, 0x240)
MAC_ADDRESS(0x0c8f90, 0x2a8)
void font::DrawStringExecute(const char* text, int count, Bitmap16Bit* bitmap,
                             int x, int y, font::TColor colorScheme, int clipX,
                             int clipY, int clipWidth, int clipHeight,
                             int cursorPos)
{
    y += m_fs.m_baseyoffset;
    if (*text && m_fs.m_abc[*text].m_abcA < 0)
        x -= m_fs.m_abc[*text].m_abcA;
    if (y < clipY)
        return;
    if (y + m_fs.m_height > clipY + clipHeight)
        return;
    while (count > 0 && x + m_fs.m_abc[*text].m_abcA < clipX) {
        x += GetCharacterWidth(*text);
        text++;
        count--;
    }
    const int color = GetColor(colorScheme, false);
    if (count == 0 && cursorPos != -1) {
        DrawCursor(bitmap, x, y, color, clipX, clipY, clipWidth, clipHeight,
                   false);
        return;
    }
    bool highlighted = false;
    int currPos = 0;
    while (count > 0) {
        unsigned char c = *text;
        if (c == '{') {
            highlighted = true;
        } else if (c == '}') {
            highlighted = false;
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
                    DrawCursor(bitmap, x, y, color, clipX, clipY, clipWidth,
                               clipHeight, false);
                break;
            default:
                DrawCharacter(c, bitmap, x, y, color + highlighted);
                if (cursorPos == currPos)
                    DrawCursor(bitmap, x, y, color, clipX, clipY, clipWidth,
                               clipHeight, false);
                break;
            }
            x += GetCharacterWidth(c);
        }
        text++;
        count--;
        currPos++;
    }
    if (cursorPos == currPos)
        DrawCursor(bitmap, x, y, color, clipX, clipY, clipWidth, clipHeight,
                   highlighted);
}

DC_ADDRESS(0x0a209c, 0x6a)
void font::DrawString(const char* text, Bitmap16Bit* bitmap, int x, int y,
                      TColor color)
{
    DrawStringExecute(text, strlen(text), bitmap, x, y, color, 0, 0,
                      bitmap->GetWidth(), bitmap->GetHeight(), -1);
}

VA(0x004b5490, 0x308)
DC_ADDRESS(0x0a2108, 0x316)
MAC_ADDRESS(0x0c9238, 0x3c8)
void font::DrawBoundedString(const char* str, Bitmap16Bit* bitmap, int x,
                             int y, int boxWidth, int boxHeight,
                             font::TColor colorScheme, unsigned justification,
                             int cursorPos)
{
    if (!str)
        return;
    int limit = strlen(str);
    int currY = 0;
    int pos = 0;
    if (limit == 0) {
        if (cursorPos != -1)
            DrawCursor(bitmap, x, y, GetColor(colorScheme, false), x, y,
                       boxWidth, boxHeight, false);
        return;
    }
    if (justification & VERT_CENTER_JUSTIFIED) {
        justification &= ~VERT_CENTER_JUSTIFIED;
        const int total = LineLength(str, boxWidth) * m_fs.m_height;
        if (total < boxHeight)
            currY = (boxHeight - total) / 2;
        else if (boxHeight < m_fs.m_height * 2)
            currY = (boxHeight - m_fs.m_height) / 2;
    }
    if (justification & BOTTOM_JUSTIFIED) {
        justification &= ~BOTTOM_JUSTIFIED;
        const int total = LineLength(str, boxWidth) * m_fs.m_height;
        if (total < boxHeight)
            currY = boxHeight - total;
    }
    while (pos < limit && str[pos] != 0
           && (currY + m_fs.m_height <= boxHeight || currY == 0)) {
        int width = 0;
        int lineStart = pos;
        while (str[pos] == '{' || str[pos] == '}')
            pos++;
        if (str[pos] != 0 && m_fs.m_abc[str[pos]].m_abcA < 0)
            width -= m_fs.m_abc[str[pos]].m_abcA;
        while (str[pos] != 0 && str[pos] != '\n' && width <= boxWidth) {
            if (str[pos] != '{' && str[pos] != '}')
                width += GetCharacterWidth(str[pos]);
            pos++;
        }
        int k = pos - 1;
        while ((str[k] == '{' || str[k] == '}') && k > lineStart)
            k--;
        if (pos > 0 && m_fs.m_abc[str[k]].m_abcC < 0)
            width -= m_fs.m_abc[str[k]].m_abcC;
        if (width > boxWidth) {
            int origPixelWidth = width;
            int okWidthIndex = 0;
            if (m_fs.m_abc[str[k]].m_abcC < 0)
                width += m_fs.m_abc[str[k]].m_abcC;
            pos = k;
            while (str[pos] != ' ' && pos >= lineStart) {
                if (str[pos] != '{' && str[pos] != '}') {
                    width -= GetCharacterWidth(str[pos]);
                    if (currY + 2 * m_fs.m_height > boxHeight && width < boxWidth)
                        break;
                    if (okWidthIndex == 0 && width < boxWidth)
                        okWidthIndex = pos;
                }
                pos--;
            }
            if (pos <= lineStart) {
                pos = okWidthIndex;
                width = origPixelWidth;
            }
            if (str[pos] == ' ')
                width -= GetCharacterWidth(str[pos]);
        }
        int currX = 0;
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
        DrawStringExecute(str + lineStart, pos - lineStart, bitmap, currX + x,
                          currY + y, colorScheme, x, y, boxWidth, boxHeight,
                          cursorPos);
        currY += m_fs.m_height;
        pos++;
    }
}

VA(0x004b57a0, 0x25)
DC_ADDRESS(0x0a2420, 0x18)
MAC_ADDRESS(0x0c9600, 0x28)
int font::GetCharacterWidth(unsigned char currChar) const
{
    const TFontSpec::myABC& abc = m_fs.m_abc[currChar];
    return abc.m_abcA + abc.m_abcB + abc.m_abcC;
}

VA(0x004b57d0, 0x44)
DC_ADDRESS(0x0a2438, 0x34)
MAC_ADDRESS(0x0c9628, 0x68)
long font::get_string_width(const char* arg) const
{
    long width = 0;
    while (*arg)
        width += GetCharacterWidth(*arg++);
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
            int okWidthIndex = 0;
            pos--;
            while (str[pos] != ' ' && pos >= lineStart) {
                if (str[pos] != '{' && str[pos] != '}') {
                    width -= GetCharacterWidth(str[pos]);
                    if (okWidthIndex == 0 && width < boxWidth)
                        okWidthIndex = pos;
                }
                pos--;
            }
            if (pos <= lineStart)
                pos = okWidthIndex;
            if (str[pos] == ' ')
                width -= GetCharacterWidth(str[pos]);
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
    int limit = strlen(text);
    int pos = 0;
    int width = 0;
    const char* str = text;
    while (pos < limit && str[pos] != 0) {
        while (str[pos] != 0 && str[pos] != '\n') {
            if (str[pos] != '{' && str[pos] != '}')
                width += GetCharacterWidth(str[pos]);
            pos++;
        }
    }
    return width;
}

VA(0x004b5990, 0x76)
DC_ADDRESS(0x0a25c8, 0x86)
MAC_ADDRESS(0x0c9888, 0xc4)
int font::LongestLineWidth(const char* str) const
{
    int limit = strlen(str);
    int pos = 0;
    int longest = 0;
    while (pos < limit && str[pos] != 0) {
        int width = 0;
        while (str[pos] != 0 && str[pos] != '\n') {
            if (str[pos] != '{' && str[pos] != '}')
                width += GetCharacterWidth(str[pos]);
            pos++;
        }
        if (width > longest)
            longest = width;
        pos++;
    }
    return longest;
}

VA(0x004b5a10, 0x6F)
DC_ADDRESS(0x0a2650, 0x84)
MAC_ADDRESS(0x0c994c, 0xd4)
int font::longest_word_length(const char* str) const
{
    int longest = 0;
    while (*str != 0) {
        int width = 0;
        while (*str == ' ' || *str == '\n')
            str++;
        while (*str != 0 && *str != ' ' && *str != '\n') {
            if (*str != '{' && *str != '}')
                width += GetCharacterWidth(*str);
            str++;
        }
        if (width > longest)
            longest = width;
    }
    return longest;
}

VA(0x004b5a80, 0x110)
DC_ADDRESS(0x0a26d4, 0xf0)
MAC_ADDRESS(0x0c9a20, 0x158)
int font::LongestWrappedLineWidth(const char* str, int boxWidth) const
{
    int limit = strlen(str);
    int pos = 0;
    int longest = 0;
    while (pos < limit && str[pos] != 0) {
        int width = 0;
        int lineStart = pos;
        while (str[pos] != 0 && str[pos] != '\n' && width <= boxWidth) {
            if (str[pos] != '{' && str[pos] != '}')
                width += GetCharacterWidth(str[pos]);
            pos++;
        }
        if (width > boxWidth) {
            int okWidthIndex = 0;
            pos--;
            while (str[pos] != ' ' && pos >= lineStart) {
                if (str[pos] != '{' && str[pos] != '}') {
                    width -= GetCharacterWidth(str[pos]);
                    if (okWidthIndex == 0 && width < boxWidth)
                        okWidthIndex = pos;
                }
                pos--;
            }
            if (pos <= lineStart)
                pos = okWidthIndex;
            if (str[pos] == ' ')
                width -= GetCharacterWidth(str[pos]);
        }
        if (width > longest)
            longest = width;
        pos++;
    }
    return longest;
}
