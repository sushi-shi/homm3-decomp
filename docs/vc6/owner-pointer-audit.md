# Member-owner recovery and out-of-object pointer review

Reviewed 2026-09-10 in `codex/address-arithmetic-20260910`, following the
[address-arithmetic review](address-arithmetic-audit.md). The constraint for
this pass is to preserve or improve **every function's matching percentage**.
The comparison checkpoint already includes the restored 100% horde initializer.

The review removes the recoverable ownership mistakes below. It does **not**
claim that all retail pointer operations are defined by modern C++ or that
malformed inputs are safe. Proven retail edge cases and the unresolved
DirectDraw version bridge are retained explicitly. README's separate buffer
bounds, cast, union and lifetime work is not silently declared finished.

## Review coverage

The lexical pass covers all 352 tracked project C/C++ files in `src/` and
`include/`, with comments, literals and literal `#if 0` bodies masked using
`homm3.vc6._source.mask`; macro definitions were inspected separately.
Vendor and generated build files are excluded. Nonliteral preprocessor arms
remain conservative candidates. Searches covered member-address casts,
pointer subtraction, negative indices, first-row decay, one-element tails,
integer address carriers, SDK output pointers and forward/reverse row walkers.

A supplementary typed census examines the 138 project compile-command TUs.
It is navigation evidence, not a proof of absence: Clang cannot fully parse
13 VC6-era TUs (the same limitations listed in the earlier audit). Those files
were also reviewed lexically. Local casts were followed to their declarations,
and suspect pointers to their allocation, caller or API. The generated census
and lexical listings remain disposable files under `build/`, not a symbol ledger.
The final deduplicated navigation inventory contains 420 pointer-arithmetic
sites, 554 increments/decrements, 14 cast subscripts, 520 record-pointer casts
and four one-byte members (three padding fields and `m_onBeach`). Those are
candidates, not defect counts; typed traversal also excludes resolved vendor
paths reached through `src/../vendor` includes.

The King's Field owner-cast review supplied the useful distinction: use an
already-known complete owner, but do not fabricate one by subtracting a member
offset or combining unrelated adjacent globals. HoMM3's SDK output arguments
exposed two instances of the latter problem.

## Adopted recoveries

| Site | Recovered owner / boundary | Match result |
| --- | --- | --- |
| `CDiffFile::getBase`, inline in `makeDiff` (`0x491140`) | The earlier arithmetic pass replaced `m_data - 4` with the known `this` byte view. No further reverse member-owner arithmetic was identified. | 83.9244%, unchanged |
| Immersion initialization (`0x4b6260`), window movement (`0x4b6950`), enclosure (`0x4b6a50`) | Replace two independent `LONG` globals with one SDK `POINT g_immWindowOrigin` at `0x696d70`. `ClientToScreen` receives the complete 8-byte owner; consumers use `x/y`. The API call and both retail field addresses prove the grouping. | All three remain 100% |
| DirectDraw `g_pixelFormat`, `0x68c850` | Replace the 16-byte prefix plus three separate mask globals with one 32-byte SDK `DDPIXELFORMAT`. DC `wingraph.cpp:235/239` names `PixelFormat` and the SDK type. The retail initializer is `{32, DDPF_RGB, 0, ...}`; `GetPixelFormat` and mask consumers use the same owner. | All caller scores preserved; `drawBolt` improves 89.316536% → 94.90726% |
| Mask-driven `TPalette16` constructor (`0x522810`) | Walk `paletteHiColor::m_data[256][3]` with a pointer to three-byte rows, not a byte pointer beyond row zero. DC `palette.cpp:95..109` supplies entry-wise RGB access. | 98.91549%, unchanged |
| Six-field palette constructors (`0x5226d0`, `0x522770`) and `CSprite::resetPalette` (`0x47bc00`) | Give the existing byte-oriented helper/constructor the complete `paletteHiColor` object representation. Keep their signatures, source calls and inlining decisions. | 94.854836%, 94.93651%, 100%, unchanged |
| `CDiffFile::getData`, inline in `apply` (`0x490f60`) | Remove the fabricated one-byte tail. A diff is an allocated byte stream with a four-byte size header and variable records; derive its payload from the allocation/header base plus `sizeof(m_numBytes)`. DC `diff.cpp:58` independently returns `this + 4`; retail writes the same header. | 99.6429%, unchanged |

`CDiffFile` now models just the four-byte header. No allocation was shortened:
`makeDiff` still allocates `max(oldSize, newSize) + 5000` bytes. The existing
serialized-byte-buffer convention is retained, as for network message headers;
this is not a claim to have modernized all placement, alignment or lifetime rules.

No new pragmas, inline gates, compiler options, dummy operations, integerized
pointers or oversized allocations were introduced. The pinned SDK stays pristine.

## Dispositions of remaining families

| Family | Disposition and boundary |
| --- | --- |
| `ExtraInfoUnion` / `ShipyardInfo` casts from `NewmapCell::m_extraInfo` | Four-byte alternate views of the same four-byte payload, not recovery of a larger containing cell. Cast/union cleanup remains separate. |
| Widget, window, resource and network-message downcasts | Base/subtype or explicitly allocated message-buffer views; not subtraction from an interior member. Dynamic-type validation is not certified by this review. |
| Diff/network headers followed by payload | Keep explicit complete-header/allocated-byte-stream boundaries (`this + 1`, `base + sizeof(header)`). A serialized payload is not a hidden adjacent C++ member. |
| Vector iterators, reverse widget hit-testing and obstacle copy-backward | Keep guarded walks within the vector allocation. `findWidgetPtr` tests `it != begin()` before `it[-1]`; its decrement stops at `begin()`. RMG neighbour access is guarded by map coordinates. Empty-vector subtraction follows the existing VC6 library convention. |
| Army, town-population, creature-bank and resource-table walks | Keep walks bounded by the owning declared row/array. Earlier cross-member army reads and cross-row town/UI table walks have already been removed. Quest text selection stays within one 52-string row. |
| Text, packed PCX/RLE and gzip cursors | Keep real byte-stream traversal. PCX's reverse nibble cursor may go before its decoded subregion, but that subregion begins inside the larger input allocation; it is not automatically before the allocation. Input-length validation remains buffer-bounds work. |
| Image-row cursors | Retain the source/retail pitch traversal with the edge cases below documented. A final unused pointer increment is **not** called safe merely because it is never dereferenced. |

### Retained image-row edge cases

`Bitmap16Bit::fillRect` (`0x44e4c0`) is an exact, small control for this class.
At the bottom of an image, with nonzero `x`, its final `dst += pitch` forms
`end + x`. Both the DC row walk (`bitmap16.cpp:679..703`) and retail contain
the unconditional update. The corresponding clipped `draw`, `grab`,
`frameRect`, darkening and colour walks in `bitmap16.cpp`, the blit in
`bitmap24.cpp`, sprite-frame row walkers, radar/puzzle row steps, and window
fizzle/fade paths require the same distinction between accessed pixels and
the final unused cursor. Some full-width, zero-origin paths end exactly
one-past and are valid; others can pass the allocation boundary.

Vertically reversed `CSpriteFrame::drawTile` / `drawTileShadow` paths can form
a before-begin row after drawing the topmost row. Victor `loadpcx` has the same
final bottom-up decrement; `flipimage` has it for a one-row region. These are
retained legacy operations, not fabricated owners and not certified safe.

Five finite source forms were compiled and independently reproduced through
`source_families` (context `ffa7831715037827a92f`):

| `fillRect` row model | Match |
| --- | ---: |
| Retail pointer update (control) | 100% |
| `getMap(x, y + row)` | 53.8929% |
| Byte-offset cursor, materialize the pointer only for the current row | 73.7857% |
| `row * pitch` from the base | 64.2143% |
| Guard the final pointer update | 69.6250% |

All 14 scored sibling functions stayed unchanged. This finite result is not
a proof that no matching bounded source exists. It explains retaining the
proven 100% form under this pass's no-loss constraint. The source comment and
`scripts/experiments/generate-fill-row-family.py` retain the failed controls.

### Retained lookup and API boundaries

`NewfullMap::newfullMapFn00505EA0` (`0x505ea0`) deliberately computes
`&m_objectTypeIndex[objectType][i]` after a reverse search, including `i == -1`
on a miss. This is a real before-begin result, not a valid portable sentinel.
The known `game::convertObject` caller immediately dereferences it to get the
monster definition; its usable contract requires a matching definition.
`newfullMapFn00505F20` similarly uses the reverse-search result unchecked.
Changing the failure result to null alone would change retail behavior without
making the caller safe. No such partial fix or invented assertion was added.

`mouseManager::loadFrame` (`0x50d8b0`) passes an old 108-byte `DDSURFACEDESC`
through a `DDSURFACEDESC2*` cast to a surface-v4 `Lock` slot. Retail explicitly
zeros and publishes size `0x6c` and uses the old field offsets. Unlike the
incorrect global pixel-format prefix, this is a versioned ABI discrepancy:
blindly enlarging it to 124 bytes changes the retail stack and size contract.
It remains unresolved, not asserted to be a safe SDK output buffer.
The summary still reports 100% instruction matching for this function. Its
strict name view separately reports the existing decorated/undecorated
critical-section imports and surface labels, plus the recovered aggregate's
`+0x14/+0x18` mask references versus the old split labels. Those mask expressions
resolve to the same `0x68c864/0x68c868` retail addresses; no comparison policy
was changed to suppress the name differences.

## Verification

The palette row family has four forms, two distinct compiled objects and
independently reproduced results (context `ff34f7a3f61b8e1d5a15`). Typed rows
and the complete-object byte view preserve 98.9155%; indexed byte-row rebinding
falls to 75.2535%. The selected typed-row object was reproduced separately
and agrees with the adopted production object.

`scripts/experiments/test-owner-payloads.py` extracts the actual palette
constructors/helper and diff `apply`, and includes the actual diff accessors.
At `-O0` and `-O2`, it checks 48 palettes in three channel layouts, all 256
entries through all three conversion constructors, the raw RGB copy, unchanged
input storage, and a mixed three-record diff. Three bounded negative controls
(missing final entry, wrong channel, wrong old-data offset) are rejected.
These native checks do not replace the VC6 build or audit malformed streams.

The compiled SDK owners have the expected 8-byte BSS and 32-byte initialized
extents. The pixel-format initializer is exactly eight dwords `{32,64,0,...}`.
All palette executable sections and named relocation destinations are unchanged;
diff and sprite instruction bytes are unchanged, with compiler-private EH-label
renumbering in their object-level identities. Matching uses the normal repository
normalization, with no added exemptions.

The final full `homm3 build` passes after regenerating targets and checking
all 152 units: **4,084 / 4,764 exact**, normalized similarity
**96.429930% → 96.434440%**, executable-wide display **96.43%**.
The complete 4,764-function comparison against
`build/owner-pointer-baseline-report.json` finds only the `drawBolt`
improvement above, with no removed functions or lower scores. All status,
banked-RVA, claim, single-view and cleanliness gates pass. The horde
initializer remains CUR/MAX/HIST 100/100/100.

The added complete-buffer byte views offset the removed SDK casts: the combined
arithmetic/owner work has the same written named-cast count as the original
checkout (3,047, including four macro-body casts omitted by the lexical masker).
That count is not an excuse to put the unsafe first-row views back. No vendor,
compiler-profile or hand-admitted target-inventory changes were needed.

### Follow-up: drawBolt restored to 100%

The subsequent `drawBolt` pass retains the complete `DDPIXELFORMAT` owner and
raises the remaining **94.90726% → 100%**. Dreamcast `spells.cpp:3781/3788`
computes the palette-row address before the RGB conversion and pixel write.
The adopted source binds `unsigned char* rgb` once in each table arm, retaining
the row's source scope. It also restores the DC-proven `InCombatArea` and
`GetMap(0,0)` calls; the retail row stride remains 800 pixels.

`generate-drawbolt-family.py` tested 32 forms: repeated channel subscripts versus
local/const/shared row pointers, the two helper calls, and the two ordinary pixel
index spellings. There were 12 distinct objects and ten reproduced elites.
The selected local-row/helper-call form was independently reproduced as well;
its executable bytes and named relocation identities equal the adopted object.
All 58 sibling function scores are unchanged. Helpers alone do not fix the
repeated-subscript form. No new inlining pins, declaration permutations, dummy
operations, or normalization exemptions were introduced.

Retail comparison reports all 58 blocks exact and no divergent source statement.
The raw name-sensitive view still reports existing spelling differences:
palette/global labels, the former split mask labels versus `g_pixelFormat`
at offsets `+0x10/+0x14/+0x18`, and the unclaimed retail `_sin`/`_cos` labels.
The mask destinations remain `0x68c860/0x68c864/0x68c868`; these are not changed
data dependencies or substituted callees.

`test-drawbolt-rows.py` compares the actual body, record, full screen, and RNG
call count with the saved pre-edit control: 294 cases at both `-O0` and `-O2`,
covering three pixel layouts, all color arms, both span orientations, clipping,
fractional movement, zero-length drawing, and arrival latches. Wrong-channel
and missing-pixel negative controls are rejected. This is bounded native
equivalence evidence, not a replacement for the VC6 verdict.

The final full build passes with **4,085 / 4,764 exact**. Comparing all function
scores with `build/drawbolt-baseline-report.json` finds only this improvement;
no score is lower. `drawBolt` is CUR/MAX/HIST **100/100/100**, and the horde
initializer remains **100/100/100**. The retained out-of-object exceptions
documented above are unaffected by this matching pass.
