// Reconstructed Victor Image Processing Library code (Catenary Systems).
// This grouping filename is provisional: the original library source/object
// names are unavailable. Retail library ownership follows imgdes, the public
// PCX APIs and the image-allocation/Win32 import band described in pcx.h.
#include <va.h>
#include <string.h>
#include <stdlib.h>
#include "victor.h"

// The allocation mode occupies zero-initialized storage in retail .data's
// virtual tail; the worker receives its current value as argument five.
DATA(0x006abaa4) unsigned int g_victorUseDibSection;
DATA(0x006abaa8) VictorCreateDibSection g_victorCreateDibSection;
DATA(0x006abaac) VictorSetDibColorTable g_victorSetDibColorTable;
// Retail masks preserve bits preceding/following an inclusive bit range.
DATA(0x00644120) const unsigned char g_victorLeadingBits[8] =
    { 0, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe };
DATA(0x00644128) const unsigned char g_victorTrailingBits[8] =
    { 0x7f, 0x3f, 0x1f, 0x0f, 7, 3, 1, 0 };

// Public API name corroborated by DC_precompiledheaders.cpp:544's PCX stub;
// that four-byte Dreamcast stub supplies no Windows implementation evidence.
// Retail independently proves stdcall/ret 16, four stack arguments forwarded
// to the cdecl worker at 0x6035c0, and the extra global mode argument.
// All 37 bytes agree after resolving the two known relocation targets.
// /Oy- is a negative control: it adds an EBP frame and changes stack operands.
VA(0x00603590, 0x25)  // anchor-caller Bitmap24Bit/Bitmap816::importPCXFile; external Victor library
int __stdcall allocimage(imgdes* image, int width, int height, int bitsPerPixel)
{
    return victorAllocateImage(image, width, height, bitsPerPixel,
                               g_victorUseDibSection);
}

// Retail-only allocation worker. One global-memory block owns the header
// and palette, plus pixels unless a DIB section supplies the pixel buffer.
// Dimensions are checked for zero only; the signed stride arithmetic and
// failure cleanup below follow the pinned executable's actual contract.
// Residual (88.04%): all 24 blocks have the same topology and all calls
// agree, but VC6 lowers the depth switch with subtraction, saves the color
// count instead of its byte size, and schedules return values/restores
// differently. The equivalent chained negative depth guard scores 78.26%;
// signed versus unsigned paletteBytes is byte-flat.
// An 18-state JSON/Python batch crossed switch/positive/negative depth
// guards, signed/unsigned/depth-derived palette sizes and header/descriptor
// count reloads. None improved 88.04%; the descriptor reload reached 86.46%.
VA(0x006035c0, 0x1d6)  // anchor-caller allocimage + imgdes / bitmap header / Win32 allocation
int __cdecl victorAllocateImage(imgdes* image, int width, int height,
                               int bitsPerPixel, unsigned int useDibSection)
{
    memset(image, 0, sizeof(*image));
    if (bitsPerPixel == victorFourBitColor)
        bitsPerPixel = victorIndexedColor;
    switch (bitsPerPixel) {
    case victorMonochrome:
    case victorIndexedColor:
    case victorTrueColor:
        break;
    default:
        return victorUnsupportedBitDepth;
    }
    if (!width || !height)
        return -1;
    image->m_colors = bitsPerPixel == victorTrueColor ? 0 : 1 << bitsPerPixel;
    int stride = ((bitsPerPixel * width + 31) >> 3) & ~3;
    unsigned int paletteBytes = image->m_colors * sizeof(RGBQUAD);
    int imageBytes = stride * height;
    unsigned int bytes = sizeof(BITMAPINFOHEADER) + paletteBytes;
    if (!useDibSection)
        bytes += imageBytes;
    BITMAPINFOHEADER* header = static_cast<BITMAPINFOHEADER*>(
        GlobalLock(GlobalAlloc(GMEM_MOVEABLE, bytes)));
    if (!header)
        return -14;
    image->m_bmh = header;
    memset(header, 0, sizeof(*header));
    image->m_bmh->biSize = sizeof(BITMAPINFOHEADER);
    image->m_bmh->biWidth = width;
    image->m_bmh->biHeight = height;
    image->m_bmh->biBitCount = bitsPerPixel;
    image->m_bmh->biPlanes = 1;
    image->m_bmh->biCompression = BI_RGB;
    image->m_bmh->biSizeImage = imageBytes;
    image->m_bmh->biClrImportant = image->m_colors;
    image->m_bmh->biClrUsed = image->m_bmh->biClrImportant;
    image->m_palette = static_cast<RGBQUAD*>(static_cast<void*>(header + 1));
    victorInitializePalette(image);
    if (!useDibSection) {
        image->m_ibuff = static_cast<unsigned char*>(static_cast<void*>(header + 1))
            + paletteBytes;
    } else {
        HWND desktop = GetDesktopWindow();
        HDC dc = GetDC(desktop);
        image->m_bitmap = g_victorCreateDibSection(dc,
            static_cast<const BITMAPINFO*>(static_cast<const void*>(header)),
            DIB_RGB_COLORS, static_cast<void**>(static_cast<void*>(&image->m_ibuff)),
            NULL, 0);
        ReleaseDC(desktop, dc);
        if (!image->m_bitmap || !image->m_ibuff) {
            if (image->m_bitmap)
                DeleteObject(image->m_bitmap);
            GlobalUnlock(GlobalHandle(header));
            GlobalFree(GlobalHandle(header));
            return -14;
        }
    }
    image->m_buffwidth = stride;
    image->m_endx = image->m_bmh->biWidth - 1;
    image->m_endy = image->m_bmh->biHeight - 1;
    return 0;
}

// The public release API validates the readable header range, releases
// global-memory and optional DIB-section ownership, then clears imgdes.
// This ordinary C++ reconstruction retains the two GlobalHandle calls.
// Residual (90.0976%): VC6 saves/restores ESI only inside the header-release branch;
// retail saves it at entry. An early read-error return, C compilation and
// /G5 or /G6 tuning leave that difference. No guard or dummy use is added
// merely to extend the import-pointer register's lifetime.
// /O2 /Os and /O1 /Oi give 105 different bytes; /O2 /Og- gives 155.
// /G3, /G4 and an explicit HGLOBAL local for both release calls are flat.
// An explicit read-error goto to the final return also preserves 90.0976%;
// spelling a shared exit does not move the conditional ESI save/restore.
// Compiling this body alone with the same headers and flags is byte-identical.
// Dreamcast's PCX stub independently confirms the public void result;
// the incidental EAX value does not justify changing that interface.
VA(0x006037a0, 0x6e)  // anchor-caller PCX importers + Win32 ownership calls; external Victor library
void __stdcall freeimage(imgdes* image)
{
    unsigned int bytes;
    if (image->m_bitmap)
        bytes = image->m_colors * sizeof(RGBQUAD) + sizeof(BITMAPINFOHEADER);
    else
        bytes = image->m_bmh->biSizeImage;
    if (!IsBadReadPtr(image->m_bmh, bytes)) {
        if (image->m_bmh) {
            GlobalUnlock(GlobalHandle(image->m_bmh));
            GlobalFree(GlobalHandle(image->m_bmh));
        }
        if (image->m_bitmap)
            DeleteObject(image->m_bitmap);
        memset(image, 0, sizeof(*image));
    }
}

// Retail loadpcx calls this after filling an indexed palette. The optional
// DIB receives the same RGBQUAD rows through the recovered callback ABI.
// The DC stubs contain no implementation of this Windows-only operation.
// Residual (77.1404%): initializing failure status before the success
// branch restores shared ReleaseDC cleanup, but VC6 retains status in EBX
// where retail uses a stack slot. Four JSON status-scope probes favor this
// form; the original failure assignment in the else arm scores 70.3860%.
// The inlined palette-initializer caller remains unchanged at 88.8478%.
// Eight resource-declaration/success-status lifetimes and six status-type /
// nested-guard controls do not improve the retained 77.1404% body.
VA(0x00603810, 0x9a)  // anchor-caller loadpcx + GDI selection/cleanup + imgdes layout
int __stdcall victorUploadPalette(imgdes* image)
{
    int status = 0;
    if (image->m_bitmap && image->m_colors) {
        HWND desktop = GetDesktopWindow();
        HDC desktopDc = GetDC(desktop);
        HDC memoryDc = CreateCompatibleDC(desktopDc);
        status = -14;
        if (memoryDc) {
            status = 0;
            HGDIOBJ previous = SelectObject(memoryDc, image->m_bitmap);
            g_victorSetDibColorTable(memoryDc, 0, image->m_colors, image->m_palette);
            SelectObject(memoryDc, previous);
            DeleteDC(memoryDc);
        }
        ReleaseDC(desktop, desktopDc);
    }
    return status;
}

// Retail-only Victor validator: IsBadReadPtr on the pixel buffer, unsigned
// inclusive-region normalization, signed byte-stride calculation and
// compression/depth status precedence. No Dreamcast implementation exists.
// Residual (79.95%): retail retains four return sequences while VC6 merges
// them into two; all checks, swaps and the import call otherwise agree.
// Explicit region-error / finished labels are byte-flat and do not restore
// those exits. Reversing the swap stores lowers the score to 79.68%.
// A complete 64-state source family crossed eight equivalent expressions for
// the shared region error with eight for the stride error; every candidate
// compiled to the same object. VC6 RTM and RTM-front-end controls are also
// byte-identical to SP3, excluding the known compiler-generation hypothesis.
VA(0x006038b0, 0xea)  // anchor-caller victorValidateBitmap + imgdes offsets / IsBadReadPtr
int __stdcall victorValidateImage(imgdes* image)
{
    int status = -42;
    if (!IsBadReadPtr(image->m_ibuff, 1)) {
        status = 0;
        BITMAPINFOHEADER* header = image->m_bmh;
        unsigned int maxWidth = header->biBitCount == victorMonochrome ? 65535 : 32768;
        if (!image->m_ibuff || !header)
            return -1;
        if (image->m_stx > image->m_endx) {
            unsigned int value = image->m_endx;
            image->m_endx = image->m_stx;
            image->m_stx = value;
        }
        if (image->m_sty > image->m_endy) {
            unsigned int value = image->m_endy;
            image->m_endy = image->m_sty;
            image->m_sty = value;
        }
        if (image->m_endx >= static_cast<unsigned int>(header->biWidth)
            || image->m_endx >= maxWidth
            || image->m_endy >= static_cast<unsigned int>(header->biHeight)
            || image->m_endy >= 32768)
            return -1;
        if (static_cast<unsigned int>(header->biBitCount * header->biWidth / 8)
            > image->m_buffwidth)
            return -1;
        if (header->biBitCount != victorIndexedColor
            && header->biBitCount != victorTrueColor)
            status = victorUnsupportedBitDepth;
        if (header->biCompression)
            return -12;
    }
    return status;
}

// Provisional internal name. The three callers at 0x603b2c, 0x603b42 and
// 0x603e12 all belong to Victor. Preserve the validator's other statuses;
// only its unsupported-depth result (-26) is cleared for a one-bit image.
// The 16-bit read at BITMAPINFOHEADER+0x0e proves biBitCount, and ret 4
// proves the stack-call ABI. All 32 bytes agree after the callee relocation.
VA(0x006039a0, 0x20)  // anchor-caller + bitmap-header semantics; external Victor library
int __stdcall victorValidateBitmap(imgdes* image)
{
    int status = victorValidateImage(image);
    if (status == victorUnsupportedBitDepth && image->m_bmh->biBitCount == 1)
        status = 0;
    return status;
}
// Both the allocation worker and loadpcx use this grayscale initialization.
// Retail writes red, green, blue, reserved in that order and expands the
// palette-upload helper, discarding its status but preserving GDI cleanup.
// /Ob2 restores that ordinary-helper expansion (21.84 -> 69.72%); the
// reserved-byte post-increment raises it to 88.85%. A 120-state guard/count/
// loop family then identifies the bitmap-header pointer snapshot below and
// raises this body to 94.61%. A real per-iteration index snapshot then fixes
// retail's EBX/EBP entry saves and restore order, reaching 95.16%. Its remaining
// loop delta is a separate scaled old-index temporary plus EBP/EDI/EDX allocation
// where retail keeps colors/index/shade in EDI/EDX/ECX. Sixty-six type/update,
// thirteen zero-order, eleven coalescing, sixty register-hint, and twelve real
// member-binding combinations do not exceed it. The for-clause increment is
// worse (69.72%).
VA(0x006039c0, 0xfc)  // anchor-callers alloc/loadpcx + RGBQUAD stores / GDI cleanup
void __stdcall victorInitializePalette(imgdes* image)
{
    int step = 255;
    if (image->m_palette && image->m_bmh->biBitCount != victorTrueColor) {
        BITMAPINFOHEADER* header = image->m_bmh;
        int colors = header->biClrUsed;
        if (!colors && header->biBitCount != victorTrueColor)
            colors = 1 << header->biBitCount;
        image->m_colors = colors;
        if (colors > 2) {
            image->m_imgtype = 1;
            step = 256 / colors;
        }
        int shade = 0;
        for (int i = 0; i < image->m_colors;) {
            int colorIndex = i++;
            image->m_palette[colorIndex].rgbRed = shade;
            image->m_palette[colorIndex].rgbGreen = shade;
            image->m_palette[colorIndex].rgbBlue = shade;
            image->m_palette[colorIndex].rgbReserved = 0;
            shade += step;
        }
        victorUploadPalette(image);
    }
}

// Provisional helper name. flipimage calls this at 0x603b7e with output
// height then width. The unsigned inclusive extents and ordered stores are
// byte-proven. Its returned EAX is overwritten by the caller, so the source
// models a void result rather than inventing a value to reserve EAX.
// Residual (85.8333%): VC6 loads the last height comparison into EAX and hoists the
// register restores; retail compares memory before one shared epilogue.
// Braced conditionals and C compilation are flat; /G6 changes the schedule
// further. Preserve the direct field calculations and conditional stores.
// /O2 /Os and /O1 /Oi give 65 different bytes; /O2 /Og- gives 122.
// /G3, /G4 and named first-image extent locals are byte-flat.
// Inverting the final height test into an early return or goto exit retains
// 85.8333%; assigning a conditional minimum instead lowers it to 80.2778%.
// These exit spellings do not recover retail's memory compare and epilogue.
// Compiling this body alone with the same headers and flags is byte-identical.
VA(0x00603ac0, 0x4d)  // anchor-caller flipimage + unsigned extent semantics; external Victor library
void __cdecl victorMinimumDimensions(imgdes* first, imgdes* second,
                                      unsigned int* height, unsigned int* width)
{
    unsigned int secondWidth = second->m_endx - second->m_stx + 1;
    unsigned int secondHeight = second->m_endy - second->m_sty + 1;
    *width = first->m_endx - first->m_stx + 1;
    *height = first->m_endy - first->m_sty + 1;
    if (*width > secondWidth)
        *width = secondWidth;
    if (*height > secondHeight)
        *height = secondHeight;
}

// Public Victor PCX metadata reader, corroborated by the Dreamcast API
// stub; the Windows body is retail-only. Open failure leaves output alone;
// successful open clears it even when the header signature is rejected.
// Retail deliberately ignores the short-read result and always closes an
// opened file. Preserve that behavior and the planar four-bit normalization.
// Residual 99.05%: a 60-state extent/store/normalization family identifies
// the four named coordinate snapshots below, raising the body from 83.14% by
// restoring retail's load schedule. Exhausting all 24 metadata orders against
// three normalization forms then identifies planes/stride/palette/depth order.
// Reading the stored output depth for the fallback restores retail's exact
// ten-block CFG. Only its byte reload and the EBX/EDI save order remain.
// Snapshot, predicate and entry-declaration follow-ups are flat or worse.
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
        unsigned int minX = header.m_minX;
        unsigned int maxX = header.m_maxX;
        unsigned int minY = header.m_minY;
        unsigned int maxY = header.m_maxY;
        data->m_width = maxX - minX + 1;
        data->m_length = maxY - minY + 1;
        data->m_nplanes = header.m_planes;
        data->m_bytesPerLine = header.m_bytesPerLine;
        data->m_palInt = header.m_paletteType;
        data->m_bpPixel = header.m_bitsPerPixel;
        data->m_vbitcount = data->m_bpPixel * data->m_nplanes;
        if ((header.m_bitsPerPixel == victorMonochrome
             && header.m_planes == victorPcxEgaPlanes)
            || static_cast<unsigned char>(data->m_bpPixel) == victorFourBitColor)
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
// Residual 92.57%: declaring the real source pointer before memset matches
// retail's LEA-before-REP schedule and moves colors/file/source into the
// retail EBX/spill/ESI homes. A 60-state declaration-scope family is byte-flat;
// status-before-colors forms are worse. Index and countdown loops are identical.
// Re-hoisting all four POD locals across 73 old-C nested/goto/return forms emits
// only the retained object or a worse early-return object. Named destination
// cursors and earlier source-pointer initialization are also measured worse.
// The nested success scope preserves the shared exit; separate early returns
// score 74.23%. A separate exit label crosses initialized locals in VC6.
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
                    goto copyPalette;
                colors = victorPcxHeaderColors;
            } else if (colors > victorPcxHeaderColors) {
                colors = victorPcxHeaderColors;
            }
            _llseek(file, 16, 0);
            _lread(file, buffer + 1, colors * 3);
        copyPalette:
            unsigned char* source = buffer + 1;
            memset(palette, 0, colors * sizeof(RGBQUAD));
            for (int index = 0; index < colors; ++index) {
                palette->rgbRed = source[0];
                palette->rgbGreen = source[1];
                palette->rgbBlue = source[2];
                source += 3;
                ++palette;
            }
            _lclose(file);
        }
        free(buffer);
    }
    return colors;
}

// Retail-only bit-range insertion, called by flipimage. The first and last
// destination bytes retain bits outside the inclusive range. Signed count
// division and the two-stage loop follow retail, including zero counts.
// Residual 94.41%: local shift and destination cursors retain saved bits
// in AL and recover retail's register allocation. Staging the shifted byte
// before updating count raises 90.70% to 94.41%; retail still uses one LEA
// where VC6 emits LEA/add. JSON batches measured shift types, cursor
// lifetimes and eight count/store schedules. /Og-, /Os and /O1 are worse.
// A further nine-state batch of reassociated/unsigned count arithmetic and
// separate source/destination byte reads does not improve 94.41%.
// Rechecking ten shift-type/store combinations after the cursor fix still
// favors signed int plus staged byte. /Ol- and /G5 controls are byte-flat;
// /G6 lowers insertion to 75.24% and extraction to 66.24%.
VA(0x00604720, 0x84)  // anchor-caller flipimage + paired bit-mask tables
void __stdcall victorInsertBits(unsigned char* destination,
                                const unsigned char* source, int offset, int count)
{
    unsigned char* target = destination;
    int shift = offset & 7;
    unsigned char mask = g_victorTrailingBits[(shift + count - 1) & 7];
    unsigned char saved = target[(shift + count - 1) / 8] & mask;
    *target &= g_victorLeadingBits[shift];
    while (count > 0) {
        unsigned char bits = *source >> shift;
        count += shift - 8;
        *target |= bits;
        if (count <= 0)
            break;
        *++target = *source << (8 - shift);
        count -= shift;
        ++source;
    }
    *target = (*target & ~mask) | saved;
}

// Retail-only bit-range extraction, paired with victorInsertBits by
// flipimage. It left-aligns the source range and clears the trailing bits.
// Residual 74.24%: the same offset-8 invariant consumes an extra register;
// retail saves EBX/EDI before the empty-count branch and uses no EBP.
// Nine JSON-batched loop/count spellings did not improve the match. A later
// exhaustive 60-state cross of six empty-tail/cursor structures and ten
// equivalent count updates emitted four objects and likewise retained 74.24%.
// Sixteen cursor/shift lifetime combinations favor a local destination
// cursor (74.24% versus 70.70%); count/source locals do not improve it.
// Seven byte-store/count-update schedules are flat. Explicit empty-count
// returns and do/for loop forms also fail to improve the retained version.
VA(0x006047b0, 0x73)  // anchor-caller flipimage + trailing bit-mask table
void __stdcall victorExtractBits(unsigned char* destination,
                                 const unsigned char* source, int offset, int count)
{
    unsigned char* target = destination;
    offset &= 7;
    unsigned char mask = g_victorTrailingBits[(count - 1) & 7];
    while (count > 0) {
        *target = *source << offset;
        count += offset - 8;
        if (count <= 0)
            break;
        *target |= *++source >> (8 - offset);
        count -= offset;
        if (count <= 0)
            break;
        ++target;
    }
    *target &= ~mask;
}
