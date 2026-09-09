# Reconstructing the external Victor library

The former PCX exclusion conflated missing vendor source with inability to
reconstruct its identified functions. `imgdes`, the public PCX API and the
Win32 ownership calls establish Victor Image Processing Library ownership.
The reconstructed C++ functions live in `src/victor.cpp`, and the assembly
kernels in `src/victor_pcx_kernels.cpp`, with explicitly provisional grouping
filenames and separate profiles. Original library source and object filenames
remain unknown; the vendor directory is unchanged.

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

The adjacent RLE decoder distinguishes the assembly compilation profile:
ordinary `/O2` removes its unused EBX save and restore, producing 81 bytes.
Disabling global optimization with `/O2 /Og-` reproduces all 83 bytes without
changing any source operation. Both neighboring assembly kernels remain
exact, giving 234 directly compared bytes with no relocations. `/Od` also
matches all three bodies, so these results do not determine the other original
optimization switches. `/O2 /Os` and `/O1 /Oi` instead shorten the RLE body to
79 bytes and change the ordinary allocation wrapper from 37 to 33 bytes.

The three consecutive kernels now share `src/victor_pcx_kernels.cpp` and the
profile with global optimization disabled. This is a provisional semantic
grouping, not a claim that the original library's object boundaries or source
filename have been recovered. The ordinary C++ wrappers retain their `/O2`
profile: applying `/Og-` to those functions breaks both exact wrappers and
lengthens the release/dimension helpers. `freeimage` and the dimension helper
retain their documented register-save/comparison scheduling residuals.

The shared PCX descriptor now imports the real Windows `RGBQUAD`,
`BITMAPINFOHEADER` and `HBITMAP` types. Its layout remains eleven dwords.
`NOMINMAX` keeps those SDK imports compatible with Bitmap24Bit's existing
`numeric_limits::max()` calls. Both PCX importers and every previously
banked function remain unchanged in the full-build comparison.

The image validator's remaining delta is the established tail-merge class:
retail keeps four return sequences while SP3 keeps two. A complete 64-state
source family crossed equivalent constant expressions for its region and
stride failures; all 64 sources emitted one object. Both the VC6 RTM back-end
and full RTM front/back-end controls emit that same SP3 object, so the return
topology is not a compiler-generation effect.

The palette initializer's first broad source family crossed entry guards,
color-count construction, and loop-local order over 120 candidates. A named
bitmap-header pointer is the sole improving source fact: it preserves retail's
otherwise redundant depth reload and raises the body from 88.8478% to 94.6087%.
All guard and loop-local alternatives collapse to the same object once that
snapshot exists; the remaining delta is the helper expansion's EBX/EBP save
placement. Four further register-hint variants also emit one unchanged object.

The upload helper retains its natural stack-resident status and failure-only
`-14` assignment at 77.1403%. A 97-state family crossed declaration position,
initialization and failure flow; its 77 retained states emit only two objects,
and no alternative improves the nested form. A nine-state storage diagnostic
likewise finds no natural register recovery: a volatile preinitialized status
reaches only 78.0702% and simultaneously drops the inlined initializer from
94.6087% to 82.8261%. Applying `/Og-` to the Victor profile breaks several
neighboring rows and is therefore not evidence for changing this unit.

For `pcxinfo`, a 60-state family crossed extent construction, metadata store
order, and the final depth-normalization CFG. Four named PCX extrema restore
retail's load schedule and raise the body from 83.1429% to 91.8452%. A complete
72-state follow-up exhausts all 24 metadata orders against three normalization
forms. Storing planes, stride, palette type, then depth raises the body to
98.5714%; the other eleven Victor rows are unchanged. The combined condition
remains stronger than the two independent `if` statements (96.6667% under the
winning metadata order). A further 192-state family exhausts all coordinate-
declaration orders and eight shared-store CFG forms; six ordinary/goto forms
compile to the same retained object and none improves it. Eight nested-label/
switch controls produce two objects and no gain.

Retail nevertheless preserves its four-bit fallback comparison after the
monochrome/planes arm. An eight-state lvalue family shows why: projecting the
already stored integer `m_bpPixel` back to its source byte for that fallback
removes the candidate's redundant jump and raises the body to 99.0476%. The CFG
is now exact in all ten blocks, six branches and two returns. The residual is
only the output-byte memory compare versus retail's retained DL and the EBX/EDI
save order. Eight unprojected header/output combinations, ten depth snapshots,
twelve entry-declaration orders and ten named-predicate/value forms exercise 40
sources and yield twenty family-level distinct-object results without improving
this peak. `/Oa`, `/Ow`, `/Oi-`, `/Ob1`, `/Ot` and `/Op` controls are flat for
`pcxinfo` or damage sibling rows.

The palette reader has the same 21 CFG blocks, twelve branches, nine calls and
ten relocations as retail. Its broad declaration-scope family emits one object,
and putting the status call before the color-count initialization is worse.
Retail instead computes the real `buffer + 1` source pointer before the palette
`memset`; moving that declaration across the intrinsic raises the body from
85.2844% to 92.5688%. It simultaneously recovers retail's 0xAC frame, spilled
file handle, EBX color count and ESI filename. Index and countdown loop forms
are byte-identical. The remaining delta is EDI save shrink-wrapping and the
source/destination register pair around the copy loop. Moving the source
initialization earlier is worse, and five named/indexed destination cursors
fall to 82.5688%. A ten-form source/file-scope retry emits one object. A final
73-state old-C family hoists buffer, file, source and index in all 24 orders and
crosses nested, shared-goto and early-return flows: nested/goto forms reproduce
92.5688%, while early returns fall to 78.2569%. This bounds the residual without
inventing an extra operation to force EDI live at entry.

The bit-range extractor remains at 74.24%. Retail has seven CFG blocks and
three conditional branches like the reconstruction, but keeps two return
sites, saves EBX/ESI/EDI before the empty-count branch, and updates the first
remaining-count step with `lea eax,[eax+edx-8]`; the reconstruction merges its
tail and hoists `offset - 8` into EBP. The earlier count and empty-exit probes
tested those facts separately. An exhaustive 60-state follow-up crossed six
real empty-tail/cursor lifetimes with ten equivalent count-update spellings.
All states compiled, produced four distinct code/relocation results, and none
exceeded 74.24%; every arithmetic spelling collapsed within its control-flow
family. The two-return families score 51.14%, 55.32%, or 65.50%, so none is
adopted. This bounds that interaction but does not identify the missing source
or compilation-context fact.

`flipimage` now lives in the provisional `victor_flip.cpp` grouping. Retail
calls the bitmap validator twice, the dimension helper once, and the two
bit-range helpers twice each. Making all those bodies visible in the current
`victor.cpp` `/Ob2` context expands them and produces a 0% comparison. Keeping
their canonical declarations in the flip caller's unit restores all nine
named calls (including allocation and free). Status-scope reconstruction first
reaches 77.5040%; named source/destination depth snapshots then reproduce the
four-instruction retail comparison, and a real destination-stride snapshot
improves allocation setup, reaching 77.8629%. Thirty-two pointer register-hint
states, six declaration-lifetime states, eight source-row intermediates and
eight declaration orders are flat or worse. Six destination-stride forms emit
two objects and favor the retained snapshot; six row-distance forms emit three
objects and are worse. This is a body-visibility hypothesis, not proof of an
original library object name or boundary. The palette initializer and its
ordinary upload helper remain together because retail expands that helper.

A later extractor register-allocation family crossed 64 real cursor, count and
shift lifetimes after the structural family above. All 64 sources emitted the
same 74.24% object. The insertion helper's corresponding loop family attempted
50 states; 45 compiled into two distinct objects and none exceeded 94.4074%.
VC6 RTM and SP3 emit identical retained objects for both helpers. `/Oa` and
`/Ow` are worse, while `/Oi-`, `/Ox`, `/Ob1`, `/Ob0`, `/Oy` and `/Ol-` are
flat. These controls bound the residual register schedules without changing
the canonical helpers or manufacturing alternate declarations.

The reconstructed `loadpcx` body now has the same 73 CFG blocks, 37 branches,
two returns and 15 real calls as retail. A 60-state refill family identifies
the branchless nonnegative remainder clamp (`sets`/`dec`/`and`), raising the
body from 69.0159% to 74.0928%. Rechecking four validation structures in that
new context favors the two early exits and reaches 76.2573%; four failure-tail
gotos do not improve it. Forty-eight decode-state declaration orders then reach
76.4138%. Six nibble-arm forms recover retail's low-arm fallthrough, and a
seven-state result-lifetime follow-up identifies one full-width shared value.
That makes the complete 21-instruction nibble loop byte-identical and raises
the function to 78.4456%.

The remaining loader delta is dominated by a whole-function allocation wall:
retail carries the image/destination chain in EBX and the consumed-byte count
in ESI, while SP3 carries the image in ESI and spills the count. Six mode
classifiers, six status declarations, four parameter-storage forms, all 16
loop-state `register` combinations, eight consumed declaration scopes, ten
destination/address lifetimes, four shared switch exits and four palette-loop
orders each emit one unchanged object. Explicit gotos therefore do not recover
retail's shared row-advance block in this profile. These finite negative
controls leave the natural implementation intact rather than adding a dummy
operation solely to rotate registers.
