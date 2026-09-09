// Reconstructed Victor PCX assembly kernels (Catenary Systems).
// This semantic grouping and filename are provisional: the original source
// and object boundaries are unknown. These three consecutive retail kernels
// share explicit inline-assembly bodies and compiler-generated outer frames.
// /O2 /Og- reproduces all 234 bytes; /Od is also byte-exact. Ordinary /O2
// removes RLE's unused EBX save/restore. The C++ library wrappers instead need
// ordinary /O2 and remain in victor.cpp. See docs/vc6/victor-library.md.
#include <va.h>
#include "victor.h"

// Provisional name for the PCX run decoder called at 0x604071. Retail keeps
// its state in AX/CX, rewrites a partially consumed run marker, and returns
// the source-byte count through a local stack slot. VC6 supplies that local
// and the frame around the assembly block. The body preserves the retail
// <= 0xc0 literal test and signed 16-bit repeat counter.
// All 83 bytes agree with global optimization disabled for this assembly
// kernel group. Ordinary /O2 removes the unused EBX save/restore (81 bytes);
// /O2 /Og- and /Od both retain the retail frame without an extra source use.
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
