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

// Provisional name for the PCX run decoder called at 0x604071. Retail keeps
// its state in AX/CX, rewrites a partially consumed run marker, and returns
// the source-byte count through a local stack slot. VC6 supplies that local
// and the frame around the assembly block. The body preserves the retail
// <= 0xc0 literal test and signed 16-bit repeat counter.
// Residual (94.8781%): this block compiles to 81 bytes; retail additionally saves and
// restores EBX although the decoded kernel never uses it. Do not add an
// unused EBX operation or a naked function to manufacture those two bytes.
VA(0x00604510, 0x53)  // anchor-caller loadpcx + RLE marker semantics; external Victor library
int __cdecl victorDecodeRleBytes(unsigned char* destination,
                                 unsigned char* source, int count)
{
    int consumed;
    __asm {
        mov esi, source
        mov edi, destination
    nextRun:
        cmp count, 0
        jle finished
        mov ecx, 1
        mov al, [esi]
        inc esi
        cmp al, 0c0h
        jbe repeatByte
        and ax, 3fh
        mov cx, ax
        mov al, [esi]
        inc esi
    repeatByte:
        dec cx
        jl nextRun
        dec count
        jl retainRun
        mov [edi], al
        inc edi
        jmp repeatByte
    retainRun:
        or cx, cx
        jl finished
        dec esi
        dec esi
        add cl, 0c1h
        mov [esi], cl
    finished:
        sub esi, source
        mov consumed, esi
    }
    return consumed;
}

// Provisional name for loadpcx's four one-bit planes -> palette-index
// kernel. Retail packs the bit position and mask into CL/CH and the four
// input bytes into DL/DH/BH/BL. This is recovered assembly source, with
// symbolic C++ parameters; VC6 supplies the EBP frame and callee saves.
// The ordinary inline-assembly block reproduces all 99 bytes, including
// the signed byte decrement and the rotating mask. No naked body or emitted
// byte directives are used.
VA(0x00604570, 0x63)  // anchor-caller loadpcx + planar bit semantics; external Victor library
void __cdecl victorUnpackFourPlanes(unsigned char* destination,
                                    const unsigned char* source,
                                    int planeStride, int pixels)
{
    __asm {
        mov edi, destination
        mov esi, source
        mov cx, 80ffh
    nextPixel:
        dec cl
        jge extractPixel
        mov cl, 7
        mov ebx, esi
        mov dl, [ebx]
        add ebx, planeStride
        mov dh, [ebx]
        add ebx, planeStride
        mov ah, [ebx]
        add ebx, planeStride
        mov bl, [ebx]
        mov bh, ah
        inc esi
    extractPixel:
        mov ah, dl
        and ah, ch
        shr ah, cl
        mov al, dh
        and al, ch
        shr al, cl
        shl al, 1
        or ah, al
        mov al, bh
        and al, ch
        shr al, cl
        shl al, 2
        or ah, al
        mov al, bl
        and al, ch
        shr al, cl
        shl al, 3
        or al, ah
        mov [edi], al
        dec pixels
        jle finished
        ror ch, 1
        inc edi
        jmp nextPixel
    finished:
    }
}

// Provisional name for loadpcx's planar RGB -> interleaved BGR kernel.
// The low 16-bit stride is the signed loop counter. EBP is explicitly saved
// within the assembly block and reused for twice the plane stride; VC6
// independently adds the surrounding frame and EBX/ESI/EDI saves. That
// two-level EBP lifetime reproduces retail's unusual double save exactly.
// A high-level short-counter C++ loop is a 58-byte negative control.
VA(0x006045e0, 0x34)  // anchor-caller loadpcx + RGB plane layout; external Victor library
void __cdecl victorInterleaveRgbPlanes(unsigned char* destination,
                                       const unsigned char* source, int stride)
{
    __asm {
        push ebp
        mov edi, destination
        mov esi, source
        mov ebx, stride
        mov ecx, ebx
        mov ebp, ebx
        shl ebp, 1
    nextPixel:
        dec cx
        jl finished
        mov al, [esi + ebp]
        mov ah, [esi + ebx]
        mov [edi], ax
        mov al, [esi]
        mov [edi + 2], al
        inc esi
        add edi, 3
        jmp nextPixel
    finished:
        pop ebp
    }
}
