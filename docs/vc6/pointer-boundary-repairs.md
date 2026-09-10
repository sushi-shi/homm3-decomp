# Pointer-boundary backlog implementation

Implemented 2026-09-10 in `codex/address-arithmetic-20260910`. This supersedes
the retained lookup, DirectDraw and image-row exceptions in the earlier
[owner-pointer audit](owner-pointer-audit.md). The follow-up request was to
fix the listed backlog and try to reach 100% in each touched function, using
Dreamcast as positive source evidence. It did not require retaining the earlier
audit's no-score-loss restriction.

All listed actionable cases are repaired. Radar and the full-screen fades
are closed by allocation proofs, without changing their production bodies.
This is not a proof that every pointer in the engine is valid. Union aliasing,
downcast checks, serialized-buffer lifetime/alignment, malformed-stream bounds,
and unsupported image dimensions/layouts remain separate debt.

## Ownership and failure contracts

Both `NewfullMap` reverse searches now test for a miss before indexing.
The ordinary shared `missingMapObjectDefinition` helper throws
`std::out_of_range`; it does not return a null pointer to callers that
immediately dereference the result. Successful searches retain last-definition
precedence and terrain filtering. The object-insertion search throws before
publishing an index, adding a definition, or acquiring a sprite. This is an
intentional new failure path, not a recovered retail statement. It propagates
through the existing exception mechanism; this change does not add a user-facing
recovery dialog or guarantee the application can continue after corrupt data.

`ddInitGraphics` (`0x6014f0`) proves that the mouse surfaces come directly
from `IDirectDraw::CreateSurface`, without `QueryInterface`. Complete uses
surface **v1**, unlike the older Dreamcast declarations. The three surface
globals, `ddBlit`, mouse helpers and their callers now use `IDirectDrawSurface`.
`loadFrame` passes its actual 108-byte `DDSURFACEDESC` directly to `Lock`:
there is no cast to a 124-byte `DDSURFACEDESC2`, and no artificial enlargement
of the local or its allocation. `loadFrame` and `ddBlit` remain 100%.

The coupled API cleanup retains DC's `const RECT&` blit/helper signatures,
ordinary helper definitions in source order, branch-local origins/descriptors,
and the canonical `getPointerPosition` / reference-parameter `mouseCoords`
calls. No helper bodies are pasted into callers to regain inlining.

`aiAttemptPuzzleGuess` owns a flat `unsigned char visible[17 * 19]`, matching
DC's flat `unsigned char[156]` for its older 13×12 puzzle. Passing a pointer to
the first subarray of a 17×19 nested array is no longer needed. The restored
flat-owner change alone is byte-identical. The further matching pass restores
the ordinary `markAIPuzzle` and `createAIPuzzleMap` helpers, including the latter's
reference to the complete two-dimensional tile array; their bodies are no longer
pasted into the caller. Complete retains its 19×17 tile dimensions.

## Row traversal

Row updates now occur only when a subsequent visited row needs them, use
an allocation-row origin that can safely end one-past, or advance an integral
relative byte displacement and form pointers only for visited rows. These
displacements are not integer-converted addresses. Source, destination
and mask cursors are handled independently. No allocation is oversized to
hide an invalid cursor, and no production pointer is converted to an integer.

| Function | Retained repair | Before → after match |
| --- | --- | ---: |
| Bitmap16 `draw` | Zero-column source/destination row bases; horizontal offsets only at actual spans | 83.5794 → 75.0079 |
| Bitmap16 `grab` | Independent relative byte displacements for both rows | 97.7093 → 82.5698 |
| Bitmap16 `fillRect` | Advance before each row after the first | 100 → 82.1964 |
| Bitmap16 `frameRect` | Advance before each row after the first | 100 → 95.2113 |
| Bitmap16 `darken` | Relative byte displacement; visited-row pointer only | 100 → 82.6377 |
| Bitmap16 masked `darken` | Guard image and mask final row updates | 100 → 86.8791 |
| Bitmap16 floating `colorize` | Relative byte displacement; visited-row pointer only | 98.8301 → 96.5654 |
| Bitmap24 raw `draw` | Advance both cursors before each subsequent row | 100 → 85.0312 |
| Bitmap24 `adjustHSV` | Relative byte displacement; visited-row pointer only | 100 → 95.7470 |
| Sprite `draw` | Relative byte displacement; visited-row pointer only | 100 → 98.6225 |
| Sprite `drawCreatureImpl` | Relative byte displacement; visited-row pointer only | 95.9000 → 94.7302 |
| Sprite `drawAdvObjImpl` | Relative byte displacement; visited-row pointer only | 100 → 96.6247 |
| Sprite `drawAdvObjWithFlagAlpha` | Relative byte displacement; visited-row pointer only | 98.0000 → 94.5249 |
| Sprite `drawAdvObjShadowImpl` | Relative byte displacement; visited-row pointer only | 99.9404 → 94.3325 |
| Sprite `drawTile` | Independent relative displacements for forward/reverse and raw-source rows | 81.4249 → 79.3933 |
| Sprite `drawTileShadow` | Relative displacement in either direction | 99.9308 → 92.0317 |
| Sprite `drawSpellEffect` | Relative byte displacement; visited-row pointer only | 100 → 98.0214 |
| Bitmap816 `markPuzzle` | Guard final sample and final grid/source row steps | 100 → 60.6300 |
| `fizzleForwardX` | Guard only the nonzero-origin screen row step | 84.1098 → 84.7843 |
| Victor `flipimage` | Stop after the last paired copy in both depth branches | 77.8629 → 73.8105 |
| Victor `loadpcx` | Guard the last completed bottom-up row step | 78.4456 → 80.2679 |
| Map lookup `0x505ea0` | Throw before indexing a missing definition | 100 → 84.0566 |
| Map lookup `0x505f20` | Same failure boundary, before publication | 100 → 97.7099 |

DC proves `Bitmap816::getMap` uses storage pitch, but `getPitch` returns
**width**. Retail's masked darken reads those same distinct fields. Both
canonical accessors and source calls are restored; changing mask traversal
to storage pitch would alter the proven algorithm. Tests deliberately use
different width and storage pitch.

Sprite changes cover the raw, general-RLE, tile-RLE and adventure-RLE
decoder branches, including both vertical directions. Horizontal predecrement
from a valid span-end pointer stays unchanged. DC's `drawHeroAlpha` wrapper
and the named read-only palette locals in alpha/spell rendering are restored
without changing their original byte scores before adding the guards.

Victor's allocator (`0x6035c0`) supports either a complete
header/palette/pixel block or a separate DIB pixel allocation. A pointer before
the pixel subregion is not automatically before the complete block, but the
separate allocation gives a concrete failing case for the old final decrement.
The repairs work with both owners. Partial RGB planes still continue without
advancing a destination row, and decoder/refill/palette/error behavior is
unchanged. Malformed-stream validation was not added.

## Allocation-only closures

### Radar

`TAdventureMapWindow` (`0x401510`) constructs the sole radar widget at
`(630, 26, 144, 144)` in its `(0, 0, 800, 600)` window. The reference search
finds no reassignment or coordinate mutation of that widget; view-world uses
the same widget. `updateRadar`'s supported square map sizes are 36, 72, 108
and 144.

The row cursor is an `unsigned short*` while its increment uses the byte
pitch: preserve that retail behavior instead of silently correcting its
scale. At pitch 1600 all four modes advance it 288 physical rows in total.
Its maximum formed offset is `(26 + 288) * 800 + 630 = 251830` pixels,
strictly inside the 480000-pixel screen. Maximum store offsets for those
sizes are 247973, 249573, 249573 and 250373. The last unused cursor is therefore
valid for the supported placement. An arbitrary bottom-edge radar placement
would not be safe and is not claimed to be supported.

### Fizzle and full-screen fades

The back surface is created as an 800×600 system-memory offscreen surface
by `ddInitGraphics` → `ddCreateSurface` (`0x6005f0`). Its successful `Lock`
publishes the same map, dimensions and positive pitch to the screen bitmap.
`robAppBlit` refreshes that reference after relocking. `Bitmap16Bit`'s owned
constructor allocates exactly `width * height` 16-bit pixels at pitch
`width * 2`.

`saveFizzleSourceX` clips and allocates the rectangle saved for the matching
fizzle call. That source and the locally grabbed destination both start at
`getMap(0, 0)`: their final pitch steps end exactly one-past their respective
allocations. Only the screen's nonzero `startX` needed a guard. This proof
assumes the existing save/forward pairing, not mismatched arbitrary rectangles.

`fadeToBlack` and `fadeFromBlack` use zero-column, zero-row starts and visit
600 rows in both their 800×600 temporary and the screen. Their final byte
cursor is `base + 600 * pitch`, exactly one-past the corresponding complete
buffer. Pixel visits fit each row. They need no row-boundary production edit;
their existing 88.5116% / 88.1358% matching residuals remain unchanged.

## Matching attempts and residuals

Every non-exact game function changed here was examined with the DC dossier,
block assembly, inline clues and retail semantic summary/structure/source
views before speculative edits. Victor's DC entries are API stubs, not
evidence for Windows decoder structure. The two map searches have no proved
direct counterpart in the older DC build.

Searches use authored `source_families`, with unchanged-source/opposite-corner
controls and independent reproduction. The fill family exhausted ten forms
(nine objects); bitmap batches scored 60 + 60 exploratory states followed
by two four-state recombinations; Bitmap24 exhausted nine states. Sprite
search scored 60 states and reproduced a five-state whole-TU recombination.
Puzzle/fizzle/PCX each exhausted four forms; flip exhausted three. Function
comments retain the failed forms and specific remaining differences. The
generators under `scripts/experiments/generate-*-row-*-family.py` describe
pre-adoption source anchors: a fresh changed source requires a rebased family,
not reuse of stale snapshots or old scores.

The two map checks exhausted shared-throw, local-throw and `vector::at`
alternatives against the unchecked control. The shared ordinary helper was
closest. The safety branches are absent in retail, so preserving them is
more important than retaining the old exact but invalid final step/result.
No tested bounded form reached 100% for those repairs; finite search is not
a proof that a better supported form cannot exist.

Mouse `update` improves **94.1675 → 99.8420%** with the DC helper/scoping
restorations. Restoring `checkUpdate`'s canonical helpers and function-static
timers, and removing its pasted-body inline-depth pin, leaves **96.1361%**
from 100%. VC6 now inlines the nested `TCSLock` constructor where retail
calls it. Keeping the class in its canonical header while placing its ordinary
ctor/dtor definitions before the manager constructor, as attributed by DC,
restores the retained constructor from **0 → 100%** without changing caller
bytes. All four boundary forms were independently reproduced. A further
16-state constructor/hotspot family emits six objects, all reproduced, and
does not improve either caller. `update`'s remaining two Y-field relocation
representations resolve to the same addresses; no fake alias or new comparison
normalization is introduced. No dummy reference, false inline declaration,
or replacement pin is added.

The additional row-offset pass scores 60 Bitmap16 states (60 objects), then
16 retained-parent/recombined states (16 objects); Bitmap24 exhausts 16 states
(nine objects). Sprite search scores 60 states (60 objects), then 13 parent/
recombined states (13 objects). Ten objects are independently reproduced in
each initial 60-state batch; all objects are reproduced in the smaller batches.
The selected four bitmap and eight sprite changes improve on the first safe
checkpoint. Another 60 sprite binding/counter/lifetime states emit one object,
also reproduced, with no improvement. Remaining sprite differences include
initial row stores across the loop guard and register/load scheduling.

The four miscellaneous row families each exhaust and reproduce three states/
objects. Relative offsets and multiplied indices do not improve the retained
puzzle, fizzle, flip or PCX repairs. A fresh 16-state paired map failure-boundary
family reproduces all 16 objects; reverse-index/exit and pre/postdecrement
failure forms all lose. Their specific scores remain beside the functions.

The puzzle helper restoration is deliberately retained despite the caller
falling **97.1621 → 89.3151%**. Six boundary states emit six reproduced objects;
the flattened control wins numerically but violates the positive helper
evidence. The restored `bitset` proxy and `game::getCell` calls expose nested
`bitset::test` and `NewfullMap::cell` expansions where retail retains calls.
The frame is eight bytes larger than retail. The standalone `bitset::test`
body is now un-emitted (CUR 0%, MAX/HIST 100%); its claim is preserved. Twelve
subsequent owner/guard/origin-lifetime states emit four reproduced objects
without an improvement. This remains an open natural-inlining problem, not
100% closure. Two old caller inline-depth pins are removed.

## Verification and collateral

The actual-body native fixtures pass at `-O0` and `-O2`:

- `test-map-lookup-boundaries.py`: reverse precedence, missing/empty classes,
  terrain filters, cached/new definitions, and no publication on failure.
- `test-fill-row-boundaries.py`: clipped/empty/bottom/right edges and padding.
- `test-bitmap-row-boundaries.py`: 76500 cases, including fill, independent source/destination
  strides and mask width/pitch, pixel oracles and one-row HSV comparisons.
- `test-sprite-row-boundaries.py`: 42112 cases, all primary encoding branches,
  flips, clipping, padding, palette/flag/shadow paths and Duff-loop widths.
- `test-misc-row-boundaries.py`: 10314 puzzle/Victor cases, partial puzzle
  blocks, one-row/in-place flips, all five PCX modes, both allocation owners,
  error exits and palette/close behavior.
- `test-window-row-boundaries.py`: 36 fizzle/fade cases with every frame
  checked, clipping, padding, 555/565 masks, restore/release and disabled paths.
- `test-radar-row-domain.py`: actual row/write switches at all four supported
  map sizes; an unsupported bottom placement is rejected as a negative control.
- `test-puzzle-helper-boundaries.py`: 2256 cases covering both restored helper
  bodies, flat visibility ownership, failure guards, proxy tests, origin wrap
  and disposal order, with six rejected negative controls. Services and tile
  construction are stand-ins; this does not test the entire AI guess algorithm.

These diagnostics check cursor bounds **before pointer formation**, because
ordinary sanitizers need not reject an unused out-of-range pointer. Negative
controls reject old final increments/decrements and incorrect pixel/lifecycle
changes. Portable Victor helpers isolate row/control plumbing; they do not
revalidate the assembly kernels or malformed inputs. Native window fixtures
model Win32 word widths, not DirectDraw or VC6 floating-point semantics.
The existing `homm3-def` C++/Rust parity suite also passes all 14 tests.

Full `homm3 build` compiles affected TUs, regenerates/delinks targets, compares
all 152 units and passes the source/claim/cleanliness/status gates. The initial
repair checkpoint moved **4086 → 4073 of 4764 functions exact**;
normalized fuzzy **96.439120% → 96.362190%**, executable display **96.36%**.
The lower scores above are retained honestly, with prior peaks in HIST.
Integrating the then-current default branch brought the checkpoint to 4077
exact; the additional matching pass ends at **4077 / 4764 exact, 96.39% fuzzy**.
Relative to that integrated checkpoint, the only CUR changes are twelve
rendering gains, the restored exact lock ctor, the puzzle caller decrease and
its un-emitted bitset helper. Constructor and bitset label changes are compared
by retail RVA, not mistaken for added/removed targets.

Final integration also includes the independently committed lobby helper work
(`22f6bd23`). The combined full build passes at **4078 / 4764 exact, 96.40%
fuzzy**. In addition to that commit's changes, unchanged-source
`CEnterNameEdit::onKillFocus` returns from 99.8710% to 100% in the combined
TU; there are no other fresh CUR changes. Its five
lobby/chat/ownership native fixtures pass again in the combined tree. The
combined source has 190 inline-depth pins; only three removals belong to this
pointer-boundary work.

Outside the listed repairs and mouse/puzzle helper restorations, the initial
score movement is `CEnterNameEdit::onKeyPress` **99.8868 → 100%**. The mechanical
`mouseCoords` call update does **not** lower `processWaitingHover`'s CUR:
it stays 95.1433%. Its changed source hash resets MAX from the older 96.1467%
peak to 95.1433%, while HIST keeps 96.1467%. This distinction matters when
reading the generated ledger. Canonical `mouseCoords` and `ddBlit` symbol
renames preserve their 100% scores and retail RVAs.

No vendor, compiler-profile or hand-admitted target-inventory changes are
needed. Three inline-depth pins are removed by this work; none is added.
The changes were prepared and verified in a dedicated worktree.
