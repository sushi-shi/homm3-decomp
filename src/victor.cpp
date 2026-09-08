// Reconstructed Victor Image Processing Library code (Catenary Systems).
// This grouping filename is provisional: the original library source/object
// names are unavailable. Retail library ownership follows imgdes, the public
// PCX APIs and the image-allocation/Win32 import band described in pcx.h.
#include <va.h>
#include <string.h>
#include "victor.h"

// The allocation mode occupies zero-initialized storage in retail .data's
// virtual tail; the worker receives its current value as argument five.
DATA(0x006abaa4) unsigned int g_victorUseDibSection;

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
