// Reconstructed Victor PCX assembly kernels (Catenary Systems).
// The original source and object boundaries are unknown.
// /O2 /Og- preserves the RLE kernel's EBX save/restore; the C++ wrappers in
// victor.cpp use ordinary /O2. See docs/vc6/victor-library.md.
#include "va.h"

#include "victor.h"

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
