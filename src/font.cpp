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
font::font(const char* name, const font::TFontSpec& fontspec, int dsize,
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
int font::getColor(font::TColor colorScheme, bool highlighted)
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
// Before normalization (locals): new_palette.
VA(0x004b5180, 0x16)  // anchor-global, dc 0xa1d14
void font::setPalette(const TPalette16& newPalette)
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

// E:\gamedcs\font.cpp:138
// EXACT 2026-09-01. Dreamcast's local roster contains color, highlighted,
// and currPos, but no `curX`: the source mutates its existing x parameter.
// Removing the invented curX local makes VC6 keep `this` in ESI and x in
// EDI, exactly closing every residual byte (98.4598 -> 100).
//
// Historical 98.46% diagnosis (retained as negative-control evidence):
// all 37 blocks agreed in size, flow kind and target
// (`homm3 sema diff 0x4b5260` reports 37/37 exact); every immediate,
// displacement, memory operand and jump-table byte agrees. The ONLY
// residual is the unit's known callee-saved role swap: retail parks
// `this` in ESI and `curX` in EDI, ours the mirror image (edi->esi x15,
// esi->edi x13), which also moves the shrink-wrapped `push edi` three
// slots. DrawCharacter above carries the SAME mirror-image swap, so the
// signature is TU-wide, not statement-local. See the residual ledger at
// the end of this comment.
//
// THE PARTITION, and it was the one open question in the decode: the
// jump table at 0x4b547c has two targets, 0x4b53a6 and 0x4b53dc,
// selected by the byte table at 0x4b5484. That table is
// {0,1,1,0,1,1,0,1,1,0} over color_scheme 1..10 - the earlier note read
// it shifted by one and got {1,2,5,8}. Arm 0 is therefore
// color_scheme in {1,4,7,10} = {PRIMARY, WHITE, HEADING, WHITE_PLAYER},
// i.e. THE BASE COLOUR OF EACH TRIPLE, and everything else (plus the
// default) is arm 1.

//     if (color_scheme & CUSTOM_COLOR) return color_scheme & ~CUSTOM_COLOR;
//     color = color_scheme + 9;
//     if (highlighted && (color_scheme == PRIMARY ||
//                         color_scheme == WHITE || color_scheme == HEADING))
//         color++;
//     return color;

VA(0x004b5260, 0x22E)  // dc 0xa1e5c
void font::drawStringExecute(const char* text, int count, Bitmap16Bit* bitmap,
                             // Before normalization (locals): color_scheme.
                             int x, int y, font::TColor colorScheme, int clipX,
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
void font::DrawString(const char* text, Bitmap16Bit* bitmap, int x, int y, font::TColor color)
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

// HISTORICAL NEGATIVE CONTROL: what the volatile does, read off the bytes
// 2026-08-14. The residual
// is exactly three memory instructions and nothing else: retail spends
// `imul eax,ecx / cmp eax,edi / jge / mov ecx,edi / sub ecx,eax /
// mov [ebp-4],ecx` with `total` living only in EAX, where the volatile
// forces `mov [ebp-0x10],eax` and reloads it for both reads. The frame
// is the SAME six slots on both sides (-4,-8,-0xc,-0x10,-0x14,-0x18);
// retail touches -0x10 four times, we touch it seven. What the volatile
// buys is slot ORDER, not the slot count: with a plain `total` the pair
// at [ebp-4] and [ebp-0xc] SWAPS against retail (visible as retail's
// `mov eax,[ebp-0xc] / mov edi,[ebp-4] / lea eax,[edi+2*eax]` becoming
// `mov eax,[ebp-4] / mov edi,[ebp-0xc] / lea eax,[eax+2*edi]`) and an
// EDX/EDI scratch cascade opens through the whole tail - 96.81.
// Rejected 2026-08-14: all sixteen single-variable relocations of the
// eight function-scope declarations (each of pos/limit/currY/lineStart/
// okWidthIndex/iOrigPixelWidth/iHeight/width moved to the front and to
// the back of the block), crossed with plain and volatile `total` -
// every one of the 32 compiles is 96.8123 or 98.7864 to four decimals,
// so VC6's slot assignment here is NOT driven by source declaration
// order. `why-reg --model` on the plain base declines (bindings agree at
// every first definition). The /Ob2 two-axis probe (mass 0..32 x 0..8
// tail candidate sites) is flat in all sixteen cells.
// DC-informed height reads and wrap-local lifetime controls (13 source
// states, three distinct objects) also leave the best at 96.8123. Reading
// height directly only in the centering arm loses 0.0259; direct reads in
// both regions and block-scoped wrapping locals do not improve the match.
VA(0x004b5490, 0x308)  // anchor-global, dc 0xa2108
void font::drawBoundedString(const char* str, Bitmap16Bit* bitmap, int x,
                             int y, int boxWidth, int boxHeight,
                             // Before normalization (locals): color_scheme.
                             font::TColor colorScheme, unsigned justification,
                             int cursorPos)
{
    int pos = 0;
    int limit;
    int currY;
    int lineStart;
    int okWidthIndex;
    // Before normalization (locals): iOrigPixelWidth.
    int origPixelWidth;
    // Before normalization (locals): iHeight.
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
    const TFontSpec::myABC* record = &m_fs.m_abc[currChar];
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

// Three lengths are live at once: `lineWidth` is what the built line
// already costs, `spaceWidth`/`spaceCount` are the run of blanks not yet
// committed to it, and `wordWidth` is the next word measured ahead of
// its copy. A word that cannot fit even an empty line is broken
// character by character in the inner loop.
// Default construction followed by assignment places the live EH state at
// +0x3c, before the empty-literal scan at +0x3f, as retail does. Constructing
// directly from "" instead marks the string live after assign. The nineteen
// named calls agree in both forms; the supported lifetime raises 87.9484%
// to 88.6275% without changing any sibling.
// Residual: 52 candidate CFG blocks versus 49 retail blocks. The first extra
// scan-entry/backedge blocks precede the later space-count zero propagation;
// that later branch is not the sole cause. Retail homes this at -0x20 and
// spaceCount at -0x1c; the candidate homes this at -0x1c and keeps the count
// in EDX during the blank scan. At overflow retail stores zero to spaceCount
// and spaceWidth before branching to the append guard; the candidate elides
// the count store and threads the zero-count path past that guard.
// Exhausted controls: 60 scan/string-construction/declaration-order states
// (42 objects, ten reproduced elites) select only the lifetime change above.
// Another 64 scalar-scope/string-lifetime states yield only the two original
// objects; hoisting spaceWidth, spaceCount, blankWidth, wordWidth or wordEnd
// adds nothing. Earlier chained/swapped zero assignments were byte-flat;
// a '<' append guard fell to 87.81%, and moving blankWidth before the counters
// fell to 86.72%. Preserve these as failed probes, not a compiler-only verdict.
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
