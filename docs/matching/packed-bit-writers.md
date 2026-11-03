# Packed-bit writer placement and review

PR #216 proposes this work for review. The earlier follow-up at `043806c7b`
was reverted in `92c6636e` and published through `52c662b39f`; that publication
is not approval to merge this proposal. The baseline is the restored source
from `f6657771a`. The review corrects both missing helper calls and several
boundaries that the initial follow-up inferred too broadly.

## Proper location and strength of evidence

Keep the game serialization templates in the shared `include/packed_bits.h`.
The operation appears in separately compiled game, hero, mapcell, town and RMG
code. Its template body must be visible to the corresponding instantiations
and expansions. A body confined to `rmg.cpp` with declarations elsewhere would
not explain that recovered model. The existing packed-reader header is a
coherent project owner. **The original game helper name and original filename
remain unknown**; the chosen names are not discoveries of native source files.

Six retained Windows map-editor RMG bodies identify the const writer's complete
operation: owned zeroed buffer, checked bit reads, LSB-first packing, one virtual
write, and full-width integer complete-write success (`count == buffer size`).

| Bits | VA | Body size |
| --- | --- | --- |
| 156 | `0x004aed89` | `0x70` |
| 128 | `0x004aee32` | `0x6f` |
| 144 | `0x004aeea1` | `0x71` |
| 129 | `0x004aef5e` | `0x70` |
| 70 | `0x004aefce` | `0x69` |
| 28 | `0x004af037` | `0x65` |

All six belong to the editor's RMG header caller. Their interspersed template
support bodies do not prove the definition's source filename. No independently
identified retained Mac packed-writer body supplies native body-order placement
evidence. Mac `0xe9e7c` is a library `bitset<156>::test`, not a packed writer.
Its xrefs identify bit access, not a game-helper owner. Generated Windows names
and candidate Mac hunk order likewise cannot prove original placement.

Dreamcast's older hero save records a local six-byte buffer and statements in
`hero.cpp:1191..1199`, ending in `gzwrite`; it does not prove the Complete
`TAbstractFile` template declaration or an original inline keyword.

## Separate editor interface

Loki's native symbols prove the free `void writeBitset(TRawOStream&,
const bitset<N>&)` template at `0x0837b380` (48 bits) and `0x0837b580` (9 bits).
Its array output throws on a short write. This differs from both game result
contracts below. Both families coexist in the Windows editor; do not rename or
combine their public interfaces merely because the bit order agrees.

Keep the editor templates in their existing image-local
`include/editor/RawStream.h` and `include/campaign_editor/RawStream.h` owners.
The stream implementations differ. Loki has `RawStream.h` strings but no
DWARF/STABS attribution assigning these writer bodies to a file; those strings
cannot establish the game's original header. Preserve proven helper spelling
and existing image-specific qualifiers without claiming new declaration proof.

## Const writer returning success

Ten game source sites use `writePackedBits`; these callers discard its result,
but the retained writer still owns its complete-write comparison.

| Caller | Sites and wire extent | Native operation |
| --- | --- | --- |
| `playerData::save` | One 12-bit / 2-byte mask | Mac `0xcd1f0..0xcd25c` |
| `hero::save` | One 48-bit / 6-byte mask | Mac `0xf3d84..0xf3e08` |
| RMG `writeMapHeader` | Six versioned hero/artifact/spell/skill sites | Mac owner `0x24e7d4`; six retained Windows bodies above |
| `game::save` | One repeated 8-bit / 1-byte hero-player site | Mac `0xd32f0..0xd3368` |
| `NewSMapHeader::save` | One repeated 8-bit / 1-byte custom-hero site | Mac `0xdbe4c..0xdbeb4` |

The final two were missing from the initial #195 review. The custom-hero site
belongs to `NewSMapHeader::save`, not `SavedGameHeader::save`.

An ordinary/explicit-inline full-TU probe under reviewed Mac profiles, including
hero `-O1`, supports the current explicit-inline reconstruction: the complete
operation expands and leaves the native-style checked test and virtual write.
This is a tested source hypothesis, not a surviving original declaration.
The counted clear avoids the first variant's retained Windows `fill_n` and
anonymous Mac zero-TOC load. All six retained RMG writer bodies remain byte
identical and measure 100%; held MAX alone is insufficient for template rows
without source hashes. Named calls matter more than equal aggregate totals.

## Raw-count family and buffer ownership

Seven sites use `writePackedBitsCount<sizeof(buffer)>`. The helper takes the
stream, a reference preserving the mask's actual constness, its native bit
bound, and caller-owned storage. The byte extent is an explicit template
argument. It clears the full extent, packs with a signed index, and returns the
raw count of one write. Constness never selects a different return contract.

- Four `NewfullMap::saveObjectType` masks each write six bytes. DC
  `mapcell.obj:0xf22cc` records one shared `outBuf[6]`, named `int count`, signed
  index and mutable proxy calls. Mac `0x126a2c..0x126c88` corroborates the four
  operations. The caller stores the result in `count` and uses
  `count < sizeof(packed)`. The usual C++ conversion already makes that
  comparison unsigned; explicit casts are unnecessary. Negative returns and
  overcounts keep their original conversion behavior.
- `town::save` writes **70 bytes for a 70-bit mask**, including 61 trailing
  zero bytes. Mac `0x1b2718`/`0x1b278c` and DC `outBuf[70]` prove that extent.
  Preserve the caller buffer, mutable access and signed spell bound. A normal
  nine-byte bitset writer would corrupt the format.
- RMG's two terrain planes use one shared two-byte buffer, signed division and
  remainder, and a signed comparison with ten. This **corrects the initial
  follow-up's unsigned-writer classification**. The const native terrain
  getters preserve direct checked access rather than mutable proxies.

The deduced bound type preserves unsigned `sizeof(packed) * 8` for mapcell
versus signed spell/terrain bounds. The name and exact API remain inferred.
VC6 rejected reference-to-array deduction and the dependent bitset partial
argument form; the accepted generic-reference form retains the observed
storage and access paths. No per-N code-generation policy is introduced.
See [counted-writer evidence](../cleanup/packed-counted-writer.md) and
[RMG recovery](rmg-packed-writer-recovery.md).

Loki proves `getTerrainMask` at `0x8363aac` and the recommended-mask getter at
`0x8363ac0`. The editor object-type writer's corresponding `this+0x14/+0x18`
operations also use those const views, retaining `to_ulong()` and its explicit
**nine-bit** conversion. Game RMG continues writing ten bits.

## Editor boundary correction

The initial follow-up restored four editor whole-writer calls. The deeper
review rejects three of those groupings on positive native evidence:

- Town timed-event output has `fill_n → getBuildMask → pack →
  getGeneratorBonuses → array write → bonus write` at Loki `0x8186cb4`.
  Its natural chained stream expression explains that order. A sequential
  complete writer followed by bonus output does not represent this source
  grouping. Restore the native caller operation and both getters.
- Town spell output calls the actual `fill_n` loop before packing; the named
  retained writer bodies call `memset`. The Complete writer also shares its
  spell buffer. Restore that supported caller model rather than forcing the
  inferred complete-writer boundary onto it.
- WitchHut has no comparable contrary boundary evidence: its Windows operation
  closely follows the retained writer28 body. Keep its canonical call and
  document the unresolved VC6 veto. Dead argument-slot reuse alone does not
  prove an in-caller buffer.

Thus the proposal adds **ten writer calls**, not the initial follow-up's
thirteen: nine game calls and WitchHut. Supported editor writer users in
GameMap, Hero, Generator, objecttype and Campaign remain represented.
[Editor evidence and searches](../cleanup/editors.md) record the boundary
correction, two native getters, count lifetime and failed whole-helper probes.

## Other reviewed leads

| Lead | Disposition |
| --- | --- |
| RMG passable/trigger footprints | Preserve the native reversed 6-by-8 traversal and nested `getBCellPassable/getBCellTrigger -> _getBitPos -> bitset` path, Mac `0x24f9f4..0x24fb40`. |
| Editor town built/disabled planes | Coupled conversion from building-state objects with disabled taking priority; preserve both native getters and writes. No intermediate bitsets are evidenced. |
| Editor event/timed-event players and abandoned-mine resources | Loki explicitly calls scalar unsigned-char output, unlike the canonical array writer. |
| Campaign prerequisites | Runtime vector, conditional packing for nonempty filenames, canonical `writeFromIter` already present. No identifiable fixed-N writer operation. |
| Readers | Existing `readPackedBits`/`decodePackedBits` cover reader paths; checked/padded reads have distinct contracts. |

The source sweep covered division/modulo eight, shifts by three, bitwise packing,
bitset/mask names and existing helpers through `src/` and `include/`. It does
not establish completeness of unreconstructed native code. Pixel/channel
packing, compression, Victor ranges and gameplay flags are different operations.

## Matching results and limits

The matching recovery uses controlled source families, native disassembly and
byte-identity-gated compiler traces. No dummy caller mass, inline pins, per-N
policies or behavior mocks were added. The header saver keeps a retail-supported
length reload even though a cached-length variant scores higher; see
[save-result lifetime evidence](game-save-recovery.md).

| Caller | Restored default MAX | Revised measurement |
| --- | ---: | ---: |
| Game `game::save` | 92.81% | 92.22% |
| Game `NewSMapHeader::save` | 95.45% | 97.90% |
| Game object masks | 100.00% | 94.09% |
| Game town save | 100.00% | 100.00% |
| Game RMG prototype writer | 100.00% | 98.85% |
| Editor RMG prototype writer | 99.65% | 71.63% |
| Editor WitchHut | 100.00% | 14.16% |
| Editor town event | 100.00% | 99.93% |
| Editor town writer | 98.84% | 99.98% |
| Editor object-type writer | 99.88% | 99.88% |

The remaining WitchHut and editor RMG losses are significant and unresolved.
Mapcell retains two nested `test` calls where retail expands them and calls
`_Xran`; game save has a cleanup boundary mismatch. RMG game differs in the
short-buffer clear. Town's editor residuals are masked-equal, not justification
for artificial declarations. These costs must remain visible in review.

RMG Mac improves from the pre-follow-up 20.5128% to 20.8861%; its old abstraction
exception is removed. Game town remains 64.5503% versus pre-follow-up 64.6164%
(one matching byte less, same linked extent/named calls), covered by its specific
canonical-helper annotation. Several edited game targets remain unscorable
because of existing unresolved references; named call/shape evidence does not
make those pairs exactly verified. The edited editor TUs have no Mac pairs.

The integrated game and map-editor fast comparisons reproduce these results
and bank them against the restored baseline. All six retained RMG writer
instances preserve their actual 100% comparisons. The two editor terrain
getter calls preserve the object-type writer's 99.8809% result and nine-bit
conversion. Historical peaks remain in HIST; unchanged-function CUR drift is
not counted as an additional source regression.

The seven affected game/header-consumer Mac units retain all 351 previously
available comparisons. Of those, 349 scores are unchanged, RMG improves and
town has the documented one-byte loss. Three previously unavailable pairs
also compare exactly; they are not claimed as source changes from this work.
The selected report has 354 scored pairs and 356 emitted unavailable pairs.
This is targeted Mac verification, not a new full-image preservation-gate
claim. The requested default-branch revert separately passed its full game,
both-editor and Mac landing gates. This proposal remains unmerged.
