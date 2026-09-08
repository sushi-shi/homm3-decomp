# Reconstructing the external Victor library

The former PCX exclusion conflated missing vendor source with inability to
reconstruct its identified functions. `imgdes`, the public PCX API and the
Win32 ownership calls establish Victor Image Processing Library ownership.
The reconstructed functions now live in `src/victor.cpp`, with an explicitly
provisional grouping filename and a separate profile. Original library source
and object filenames remain unknown; the vendor directory is unchanged.

The public allocation and validation wrappers use `stdcall`; the allocation
worker and dimension helper use `cdecl`. The 37-byte `allocimage` wrapper
loads the process-wide mode, forwards five stack arguments, cleans the worker's
arguments, and returns with `ret 16`. The 32-byte bitmap validator preserves
all statuses except the unsupported-depth result for a one-bit image.
Resolving the three known relocation targets makes both complete byte ranges
identical to retail. Keeping EBP frames with `/Oy-` changes both wrappers.

The minimal admitted profile is `/O2 /ML /Gd /D_WINDOWS`, with public calling
conventions explicit in the declarations. Default and `/G5` tuning are flat;
`/G6` changes the dimension helper without fixing it. `/GX`, `/Ob2`, their
combination, and a C-front-end control do not fix the outstanding differences.
These functions do not identify the original CRT mode or compiler generation.

Two planar kernels need their assembly structure preserved. The RGB kernel
uses a signed 16-bit counter and temporarily repurposes EBP as twice the
plane stride. An ordinary VC6 `__asm` block with symbolic C++ arguments
reproduces its explicit EBP save inside the compiler-generated EBP frame,
including all three callee saves: all 52 bytes are exact. A high-level C++
short-counter loop emits 58 different bytes. The four-plane unpacker likewise
packs its bit index/mask in CL/CH and source bytes in four byte registers;
the ordinary inline-assembly definition matches all 99 bytes. Neither needs
a naked function, raw byte directives, or hand-written outer prologue.

The adjacent RLE decoder is different: the same approach reproduces its loop
and consumed-byte local, but VC6 omits retail's unused EBX save and restore.
That two-byte discrepancy remains explicit. `freeimage` and the dimension
helper also retain register-save/comparison scheduling residuals. Their
function-specific controls are recorded beside their definitions.

The shared PCX descriptor now imports the real Windows `RGBQUAD`,
`BITMAPINFOHEADER` and `HBITMAP` types. Its layout remains eleven dwords.
`NOMINMAX` keeps those SDK imports compatible with Bitmap24Bit's existing
`numeric_limits::max()` calls. Both PCX importers and every previously
banked function remain unchanged in the full-build comparison.
