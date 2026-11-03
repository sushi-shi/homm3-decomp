# Counted packed-plane writer

`writePackedBitsCount<WireBytes>` shares the zero/pack/write operation for
`NewfullMap::saveObjectType` and `town::save`. Its four arguments are the stream,
mask reference, native bit-count expression, and caller-owned buffer. The wire
extent is a template argument, keeping town's 70-byte format explicit without
a fifth runtime argument. `Bits&` preserves the actual mutable or const access
path; the return is always the raw int write count. The separately evidenced
unsigned-index `writePackedBits` success writer remains unchanged.

DC `mapcell.obj:0xf22cc` names one `outBuf[6]`, signed `int i`, signed `int count`
and `unsigned short ushort_buffer`. Its checked gzwrite rows store into count
before each unsigned size comparison. The source restores the shared count
and unsigned-short staging value while retaining the canonical scalar and
packed writers. Lines 3684/3696/3708/3720 show mutable bitset proxy conversions;
Mac 0x126a2c..0x126c88 expands clear-six/test-48/pack/write-six. The four calls
continue to share the same six-byte buffer and the signed loop index.

`count < sizeof(buffer)` uses the usual unsigned arithmetic conversion on both
native 32-bit builds. The removed casts added no behavior: negative int results
still convert to large unsigned values, and overcounts still pass. Neither is
silently changed to equality or signed error checking. The scalar guards use
their actual staging sizes in the same way.

DC `town.obj:0x165988` names `outBuf[70]`; Mac 0x1b2718 clears 70 bytes,
0x1b2778 bounds the loop at 70 bits, and 0x1b278c writes 70 bytes. The final
61 zero bytes are part of the saved-game format. Town retains its signed
`hero::NUM_SPELLS` bound, while mapcell retains unsigned `sizeof(packed) * 8`.
Each mask performs one write. Storage extent and loop bound remain distinct
because those are concrete native facts, not a new serialization feature.

The helper name and grouping are inferred. Neither a named older DC template
nor an original header name is asserted. Header visibility serves both TUs.
CodeWarrior expands the inline helper through the native proxy/test path;
an ordinary template leaves a writer call absent from native. VC6 rejects
deduced array-reference extents and the partially explicit `WireBytes,N,Count`
form; deducing `Bits` directly accepts the four-argument interface.

## Verification and remaining mismatch

Baseline for this recovery is `043806c7b2638333eeed8e3eb445931eadebcb84`.
A bounded source family tested nine states: unchanged, cast removal, mask-only
count staging, all checked count staging, native unsigned-short staging, and
their explicit-wire-extent variants. All nine compiled and scored; seven
whole-object variants were distinct. Unchanged-source and opposite-corner
controls passed, and four elites reproduced. Every state preserved the scores
of both affected units. The rejected template-deduction model failed its
opposite-corner compile and supplied no accepted score evidence.

| Function | Baseline Windows MAX | Final Windows | Mac result |
| --- | --- | --- | --- |
| saveObjectType, 0x00503c40 | 94.0864% | 94.0864% | Existing unresolved writeScalar<char> prevents exact comparison |
| town::save, 0x005bd2f0 | 100% | 100% | 64.550265% unchanged |

Fresh targeted VC6 and CodeWarrior builds confirm the adopted source. All
**66 scored Mac pairs** keep their scores and availability, with 24 exact;
there are still 62 emitted unavailable pairs and one claim without a full-TU
body. No new waiver or linking manifest was added. The prior town abstraction
annotation concerns the earlier helper recovery (64.616402% to 64.550265%),
not a new loss in this change.

Comparing the compiled mapcell function between unchanged, cast-removal-only,
and adopted variants proves the same **656 bytes and named relocations**.
This establishes cast-removal neutrality beyond equal aggregate scores. The
remaining 94.0864% result is not a claimed improvement: the native inlining
diagnosis still identifies shadow/trigger `bitset<48>::test` calls where retail
expands test and retains `_Xran`. The earlier byte-identical C2 trace identifies
the nested inlining boundary; restoring native count staging and simplifying
the interface do not move it. No helper was removed, and no unused native
`tempText[100]`, invented code, cast, or pragma was added to steer the budget.

A shared counted-byte zero loop was also checked against the native RMG
terrain clear lead. It preserves Windows scores but drops town's Mac result
64.550265% -> 49.358974%: the bulk clear call disappears and test<70> moves
from offset 1200 to 1420. It supplies no RMG Windows gain either. The probe is
rejected; the shared helper keeps memset without a size-dependent policy.

Evidence and disposable source-family material remain under
`build/packed-counted-recovery/` and
`build/source-families/b02bfc9924c9225f8b8b/`. No full build or mock behavior
suite was run. The coordinator owns the normal PR and combined score banking;
this worker does not publish to the default branch. The
[combined audit](../matching/packed-bit-writers.md) covers the other writer
families and their separate native contracts.
