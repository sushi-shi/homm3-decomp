# VC6 candidate source lines

`homm3 sema diff --source` reads the parallel `/Z7` object's classic COFF
`IMAGE_LINENUMBER` records. A stored line of zero anchors a function symbol;
ordinary records add their stored line to the function's `.bf` opening line.
VC6 uses **0x7fff for a zero relative line**, allowing later instructions to
return to that opening line without being interpreted as another anchor.

The pinned SP3 compiler proves the encoding with this control:

```cpp
void zeroRelative(volatile int* dst)
{
    dst[0] = 1;
#line 2
    dst[1] = 2;
#line 8
    dst[2] = 3;
}
```

With `/O2 /Ob2 /Oy- /Op /ML /Gr /GX /Gy /Z7 /FAs`, `.bf` is line 2.
The second store at offset 6 has stored line 0x7fff; the compiler's assembly
listing labels it line 2. Changing only `#line 2` to `#line 3` changes that
record to 1. Both functions have identical 21-byte bodies, SHA-256
`efd96d94536a7ef0515b4b22ae952081e7fd73486c3116fc5e64313b3f21dda0`.

The same encoding occurs naturally at offset 0x8b of mapcell's
`CObjectType(TObjectType*)` candidate: the loop's `xor ebx, ebx` is attributed
to the opening brace. Treating 0x7fff as an ordinary delta produced an
out-of-range source line. The reader decodes this one value explicitly;
other deltas remain intact. Source-range validation and byte identity
between matching and debug functions remain required.

## Retained header functions

The line number belongs to the COFF `.file` record preceding the function
symbol, not necessarily the manifest TU. VC6 emits another `.file` record
when a retained header body enters the symbol table, and switches back for
subsequent TU bodies. Filenames span as many 18-byte auxiliary records as
needed. Resolve this ownership in symbol-table order before sorting functions
by section/offset or assigning duplicate-name ordinals.

The `3464038c` `rmg_terrain` `/Z7` object provides a concrete control: the retained
grid copy constructor (`0x4fa520`) has `.bf` line 581 under `include/rmg.h`,
and the coordinate constructor (`0x5b76b0`) has line 584 under that same header.
Their verified non-debug bodies are 22 and 24 bytes respectively. Reading
those numbers from `src/rmg_terrain.cpp` used to print unrelated statements,
despite the code-byte check passing. `sema --source` now resolves the recorded
filename through Wine's `Z:` mapping and accepts only a current TU dependency
or a pinned compiler header. An unavailable/foreign recorded file is an error,
not a reason to fall back to the TU. Objects without `.file` records retain
the legacy manifest-source fallback.

Wine also resolves `<xtree>` to the pinned file `XTREE`, while the recorded
filename retains the lowercase spelling. For an unavailable compiler-header
path, the reader accepts a unique filename differing only in case inside the
pinned include tree. Ambiguous case matches and unrelated project files remain
errors. The naturally retained `_Tree::end() const` at `0x58eb50` exercises
this path without changing the compiler files or the source-byte check.

Hermetic controls cover multi-record paths, TU/header/TU switches, reversed
section order, identical line numbers with different header/TU text, and
rejection of missing or unrelated files. Source text still never participates
in the assembly comparison or changes its verdict.
