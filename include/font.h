// font.h - prototypes of font.cpp (compiland font.obj)
#ifndef HOMM3_FONT_H
#define HOMM3_FONT_H

#include <string>
#include <vector>
#include "resource.h"
#include "palette.h"

class Bitmap16Bit;

// resource base + the dtor-proven tail: an embedded TPalette16 at
// 0x103c (destroyed by the implicit member dtor) and the glyph data
// pointer at 0x1258 (deleted when set). The 0x1020 span before the
// palette is the unmodeled font spec table (DC size 5212).
// Vtable 0x63e5f4.
// Before normalization (type): font.
class Font : public Resource {
public:
    // DC font::TFontSpec (LF_INTERFACE 0x2378, LF_FIELDLIST 0x2377,
    // size 4128 = 0x1020) - the WHOLE header blob at font+0x1c, not the
    // glyph record. Retail's constructor 0x4b5070 copies it wholesale
    // (`mov ecx,0x408; rep movsd` into this+0x1c) straight out of its
    // by-reference parameter, which is what makes it a member initializer
    // rather than a body assignment: it runs before the Palette member's
    // constructor. Every name and offset below is the Dreamcast field
    // list verbatim; retail corroborates height@+5, baseyoffset@+6,
    // abc@+0x20 and Offset@+0xc20 by use.
// Before normalization (type): font::TFontSpec.
    struct FontSpec {
        // DC font::TFontSpec::myABC (LF_FIELDLIST 0x2372, size 12): the
        // Win32 ABC widths - `int abcA` (left side bearing, SIGNED: it is
        // the `field_0 < 0` leading-bearing test in DrawStringExecute),
        // `unsigned abcB` (the inked width DrawCharacter loops over), and
        // `int abcC` (right side bearing). The names are left as field_N
        // because other lanes' sources already spell them that way; the
        // identity is recorded rather than renamed.
// Before normalization (type): font::FontSpec::myABC.
        struct MyABC {
            int m_abcA;
            unsigned int m_abcB;
            int m_abcC;
        };

        unsigned char m_first;
        unsigned char m_last;
        unsigned char m_depth;
        char m_xspace;
        char m_yspace;
        // Glyph row count (DrawCharacter's outer loop bound).
        unsigned char m_height;
        // The signed vertical bearing DrawStringExecute adds to `y`
        // before clipping - retail reads it with
        // `movsx eax, byte ptr [esi+0x22]`.
        char m_baseyoffset;
        // Aligns numpal at +8 after seven byte fields.
        char m_pad;
        unsigned long m_numpal;
        unsigned short* m_pal[5];
        MyABC m_abc[256];
        // Per-character offsets into `data` (DrawCharacter indexes this
        // at font+0xc3c).
        unsigned long m_offset[256];
    };
    SIZE(FontSpec, 0x1020);
    // Glyph pixel encoding (byte-derived from DrawCharacter 0x4b51a0):
    // 0 draws nothing, SOLID takes the color slot, every other value
    // takes the shadow slot (palette.data[32]). Name is a bootstrap
    // invention.
// Before normalization (type): font::EGlyphPixel.
    enum GlyphPixel {
        GLYPH_PIXEL_SOLID = 0xff
    };
    // Dreamcast font::EJustify, verbatim. Retail corroborates the
    // whole roster: DrawBoundedString strips bit 4 for vertical
    // centering, bit 8 for bottom alignment, then switches the
    // remainder on 0/1/2.
// Before normalization (type): font::EJustify.
    enum Justify {
        LEFT_JUSTIFIED = 0,
        TOP_JUSTIFIED = 0,
        CENTER_JUSTIFIED = 1,
        RIGHT_JUSTIFIED = 2,
        VERT_CENTER_JUSTIFIED = 4,
        BOTTOM_JUSTIFIED = 8
    };
    // Dreamcast font::TColor, verbatim. Retail corroborates
    // CUSTOM_COLOR: DrawBoundedString's cursor path tests it with
    // `test ah,1` and clears it with `and ah,-2`.
// Before normalization (type): font::TColor.
    enum Color {
        LowestColor = 1,
        PRIMARY = 1,
        PRIMARY_HIGHLIGHT = 2,
        PRIMARY_DIM = 3,
        WHITE = 4,
        WHITE_HIGHLIGHT = 5,
        WHITE_DIM = 6,
        HEADING = 7,
        HEADING_HIGHLIGHT = 8,
        HEADING_DIM = 9,
        WHITE_PLAYER = 10,
        WHITE_PLAYER_HIGHLIGHT = 11,
        WHITE_PLAYER_DIM = 12,
        CHAT = 13,
        HighestColor = 14,
        CHAT_HIGHLIGHT = 14,
        CHAT_DIM = 15,
        CUSTOM_COLOR = 256
    };
    // DC LF_MEMBER `fs`, offset 28.
    FontSpec m_fs;

private:
    // DC LF_MEMBER `Palette`, offset 4156 = 0x103c, held BY VALUE - the
    // retail constructor 0x4b5070 runs TPalette16's default constructor
    // on this+0x103c as a member initializer (unwind state 1, funclet
    // 0x62b4d8 destroys exactly this subobject).
    Palette16 m_palette;
    // DC LF_MEMBER `Data`.
    void* m_data;

public:
    // The glyph payload's byte count, byte-proven by GetSize below: the
    // whole class is 0x1260 and the only member past `data` is the dword
    // at 0x125c that the size query adds to it. DC has no such member -
    // its port left the resource size query on a different slot shape.
    int m_dataSize;
    Font(const char* name, const FontSpec& fontspec, int dsize,
         unsigned char* d);  // retail 0x4b5070
    virtual ~Font();
    virtual unsigned int getSize() const;
    void setPalette(const Palette16& newPalette);
    void drawCharacter(int c, Bitmap16Bit* bmp, int x, int y, int color) const;
    void drawBoundedString(const char* str, Bitmap16Bit* bitmap, int x, int y, int boxWidth, int boxHeight, Font::Color colorScheme, unsigned justification, int cursorPos);
    int lineLength(const char* str, int boxWidth) const;
    int lineWidth(const char* text) const;
    int longestLineWidth(const char* str) const;
    int longestWrappedLineWidth(const char* str, int boxWidth) const;
    int longestWordLength(const char* str) const;
    int getCharacterWidth(unsigned char currChar) const;
    // Original DrawCursor, font.cpp:123; ordinary member, expanded in retail.
    void drawCursor(Bitmap16Bit* bitmap, int x, int y, int color,
                    int clipX, int clipY, int clipWidth, int clipHeight,
                    bool highlighted);
    long getStringWidth(const char* arg) const;

private:
    void drawStringExecute(const char* text, int count, Bitmap16Bit* bitmap, int x, int y, Font::Color colorScheme, int clipX, int clipY, int clipWidth, int clipHeight, int cursorPos);

public:
    // Retail 0x4b5b90, font.obj's tail. The `fs.abc[' ']` triple it reads
    // at this+0x1bc/0x1c0/0x1c4 is what types the receiver as a font and
    // the second parameter as a pixel box width; NH3API corroborates the
    // name and the three-parameter shape only.
    void fillLinesVector(const char* str, int boxWidth,
                         std::vector<std::string>& result);
private:
    // Original GetColor, font.cpp:56; ordinary member.
    int getColor(Font::Color colorScheme, bool highlighted);
};

// Retail .bss 0x698a08, a loaded `font*` that four bodies read (0x4514b1
// and 0x4514bf in bottomviewsubwindow, plus 0x473064 / 0x4ee120 /
// 0x4ee47c). TBottomViewResourceMessage measures its quantity string
// through this pointer's LineWidth and sizes the widget with its
// `height` byte, then renders that same string with 'smalfont.fnt' - so
// the object is the small font, but NO roster attests a name for it
// (the Dreamcast dump carries only `medFont`), which is why this keeps
// the house ordinal placeholder. Owner TU unlocated - extern only, no
// DATA claim (the gpWindowManager / gTownSizeNames pattern).
extern Font* g_unnamed698a08;

// --- font ---
// CODEVIEW(E:\gamedcs\font.cpp:33, dc 0xa1ba8) void font::font();
// CODEVIEW(E:\gamedcs\font.cpp:56, dc 0xa1ce4) int font::GetColor(font::TColor color_scheme, unsigned char highlighted);
// CODEVIEW(E:\gamedcs\font.cpp:123, dc 0xa1e30) void font::DrawCursor(Bitmap16Bit* bitmap, int x, int y, int color, int clipX, int clipY, int clipWidth, int clipHeight, unsigned char highlighted);
// CODEVIEW(E:\gamedcs\font.cpp:246, dc 0xa209c) void font::DrawString(const char* text, Bitmap16Bit* bitmap, int x, int y, font::TColor color);
// CODEVIEW(E:\gamedcs\font.cpp:254, dc 0xa2108) void font::DrawBoundedString(const char* str, Bitmap16Bit* bitmap, int x, int y, int boxWidth, int boxHeight, font::TColor color_scheme, unsigned justification, int cursorPos);
// CODEVIEW(E:\gamedcs\font.cpp:35, dc 0xa27c4) void* font::`scalar deleting destructor'(unsigned __flags);

#endif  /* HOMM3_FONT_H */
