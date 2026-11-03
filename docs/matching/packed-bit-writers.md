# Packed-bit writer ownership and caller audit

Follow-up to PR #195, 2026-11-02. The initial correction restored only player,
hero and six RMG header calls. This audit covers the authored game and both
editor source trees, including already exact functions. It does not claim that
unreconstructed native code has no further consumers. PR #195 remains merged.

## Proper location and evidence limits

Keep the game writer's single canonical body in `include/packed_bits.h`.
Windows and Mac expand the same zero-buffer, checked-bit, LSB-pack and counted
write operation in independently compiled `game.cpp`, `hero.cpp` and `rmg.cpp`.
A shared template definition explains that visibility; a body confined to
`rmg.cpp` with declarations elsewhere does not. The existing packed reader
header is the appropriate project owner. **The original game helper name and
original filename are unknown.** `writePackedBits` and `packed_bits.h` are
inferred project spellings, not native source-file discoveries.

The six retained Windows map-editor writer bodies identify the complete
operation and its full-width integer `write(...) == sizeof(buffer)` result:

| Bits | VA | Body size |
| --- | --- | --- |
| 156 | `0x004aed89` | `0x70` |
| 128 | `0x004aee32` | `0x6f` |
| 144 | `0x004aeea1` | `0x71` |
| 129 | `0x004aef5e` | `0x70` |
| 70 | `0x004aefce` | `0x69` |
| 28 | `0x004af037` | `0x65` |

Their direct callers all belong to the editor's RMG header writer. Intervening
transform/copy template instances make this consistent with RMG instantiation
order, but do not locate the definition in a `.cpp` rather than a header.
Generated Windows symbol names are authored correspondence, not original names.

No independently identified retained Mac packed-writer body supplies native
body-order evidence. Mac `0xe9e7c` is a checked `bitset<156>::test`, not a game
writer; its xrefs at `0xdb6e8` and `0x24f348` identify bit access only. Newly
compiled writer hunk order is not evidence of native placement.

Dreamcast's older `hero::save` has a six-byte local and statements attributed
to `hero.cpp:1191..1199`, ending in `gzwrite`. It does not prove a Complete
`TAbstractFile` template declaration, original header or inline keyword.

## Separate editor interface

Loki's native dynamic symbols prove `void writeBitset(TRawOStream&,
const bitset<N>&)` at `0x0837b380` (48 bits) and `0x0837b580` (9 bits).
These bodies own clearing, packing and array output; the stream throws on a
short write. The game writer instead returns integer complete-write success.
Both families coexist in the Windows map editor, so this is a real interface
distinction, not a platform-dependent name for one function.

The editor templates belong in their existing image-local
`include/editor/RawStream.h` and `include/campaign_editor/RawStream.h` owners.
Their stream implementations differ; keep those native call paths. Loki has
`RawStream.h` strings, but its stripped ELF has no DWARF/STABS source records
assigning this template to a filename. Those strings cannot prove the game's
original header. Existing qualifiers are preserved unless a separate probe
supports changing them. See [editor evidence and probes](../cleanup/editors.md).

## Direct const writer consumers

All twelve source call sites use the complete success-returning game writer;
these callers discard its result. It still owns the retained return comparison.

| Caller | Sites and wire extent | Native operation |
| --- | --- | --- |
| `playerData::save` | One 12-bit / 2-byte mask | Mac `0xcd1f0..0xcd25c` |
| `hero::save` | One 48-bit / 6-byte mask | Mac `0xf3d84..0xf3e08` |
| RMG `writeMapHeader` | Six sites: 156/128 hero, 144/129 artifact, 70 spell, 28 skill bits | Mac owner `0x24e7d4`; retained Windows bodies above |
| `game::save` | One site repeated for each hero: 8 bits / 1 byte | Mac `0xd32f0..0xd3368` |
| `NewSMapHeader::save` | One site repeated for custom heroes: 8 bits / 1 byte | Mac `0xdbe4c..0xdbeb4` |
| `writeRmgObjectPrototype` | Two 10-bit / 2-byte terrain masks | Mac `0x24fb44..0x24fc44` |

The last four sites were missing from the initial review and are restored.
The recommended-terrain call retains `getRecommendedTerrainMask()`. The
custom-hero availability loop is in `NewSMapHeader::save` (`0x004c4f10`), not
`SavedGameHeader::save`. Native unsigned index arithmetic and a one-byte array
support the writer call despite the previous scalar spelling's higher score.

Four additional editor operations now call their own `writeBitset`: WitchHut
skills, town timed-event building mask, and town obligatory/disabled spells.
The building call preserves the Loki-retained `getBuildMask()` accessor.

## Distinct raw-count and padded operations

Four `NewfullMap::saveObjectType` planes are 48-bit / six-byte masks. Mac
`0x126a2c..0x126c88` and DC `mapcell.obj:0xf22cc` identify draw, passable,
shadow and trigger operations. Each write checks **unsigned count < 6**,
which differs from equality-as-success for negative returns and overcounts.
DC records a shared `outBuf[6]`, signed loop index, mutable subscript, proxy
conversion and destruction at lines 3684/3696/3708/3720. Mac retains the
mask/index proxy setup and signed division/remainder. A const-success call
would lose positive source facts and change the result contract.

`town::save` writes a **70-byte** spell buffer, although only nine bytes carry
its 70 bits. Mac passes `0x46` to clearing and writing at
`0x1b2718`/`0x1b278c`; DC independently records `outBuf[70]` and mutable proxy
access. The remaining 61 bytes are zero. A normal `writePackedBits<70>` would
silently shorten the stream. This family needs caller-owned extent, mutable
access and raw count rather than the existing success interface.

All five calls now use a separate `writePackedBitsCount` template in the same
shared header. It takes the caller's buffer, byte extent and native bit-bound
expression, clears the whole buffer, packs through mutable subscripts with a
signed index, and returns the raw result of one write. The caller still owns
the unsigned short-write check. The deduced bound type preserves mapcell's
`sizeof(packed) * 8` versus town's signed `hero::NUM_SPELLS`; no per-N policy
or constness-dependent return overload is introduced. Its name and exact
interface remain inferred. [The counted-writer evidence](../cleanup/packed-counted-writer.md)
records native storage lifetimes and the rejected array-reference probe.

## Other serialization leads

| Lead | Disposition and concrete distinction |
| --- | --- |
| RMG passable/trigger footprints | Already represented by `getBCellPassable/getBCellTrigger -> _getBitPos -> bitset` inside the native reversed 6-by-8 traversal (Mac `0x24f9f4..0x24fb40`). Direct mask output would erase the recovered helper path. |
| Editor town built/disabled planes | Coupled conversion from building-state objects; disabled takes priority. Native Loki retains both getters and both writes. No intermediate bitsets are evidenced. |
| Editor event/timed-event players and abandoned-mine resources | Loki explicitly calls scalar unsigned-char stream output, unlike the canonical array writer. Keep the scalar contract. |
| Campaign scenario prerequisites | Runtime-sized vector, conditional packing when the filename is nonempty, canonical `writeFromIter` output already present. No fixed-N writer input or separately identifiable dynamic packing helper. |
| Editor object-type terrain | Existing `writeBitset` calls serialize an explicit nine-bit conversion; game RMG uses ten bits. Equal two-byte extent does not make their values interchangeable. |
| Packed readers | Existing `readPackedBits`/`decodePackedBits` cover reader paths; checked and padded reads have distinct contracts. They are not missing writer calls. |

The search covered division/modulo eight, shifts by three, low-three-bit masks,
shift/OR assignments, `bitset`, packed/mask names and existing helper calls
through `src/` and `include/`. Scalar flag updates, pixel/channel packing,
Victor bit ranges, compression and gameplay masks do not perform this writer
operation. Decoder/accessor leads outside this writer audit remain separately
identified rather than counted as recovered writer consumers.

## Inline and compiler evidence

The ordinary template retained writer calls in candidate hero/RMG Mac objects
where native callers expand the operation. A full-TU ordinary/explicit-inline
probe under reviewed profiles (including hero `-O1`) removes those calls with
`inline`, leaving the native-style checked `bitset::test` and virtual stream
write at each packed block. Const subscript preserves the checked test path.
This supports explicit inline as the current reconstruction; it is **not** a
surviving original declaration or a proof of unique source spelling.

The complete helper uses a counted zero-initialization loop. The `fill_n`
variant retains an unwanted Windows call in `game::save` and introduces an
anonymous zero TOC load in the RMG Mac expansion, losing that pair's available
comparison. The owned loop removes both without a new literal manifest.
Direct `test` versus const subscript gave header-save Windows 89.01 versus
88.34%; the retained const-subscript path is preserved in the canonical model.
All six retained editor RMG writer instances keep their MAX after these probes.

Named operations matter: equal aggregate call totals do not prove matching
inline boundaries. Many writer callers are unscorable due to unresolved Mac
references. The earlier passing whole-image Mac gate therefore did not verify
their writer expansion. Call listings and native disassembly supply that check.

VC6 expands the game writer but retains some nested operations differently.
Byte-identity-gated traces show `game::save` retaining a `fill_n` call in the
first helper variant and differing nested string/vector boundaries. The header
writer has pre-existing string inlining differences. RMG's new terrain calls
expand with the same named call sequence as retail; its remaining mismatch is
inside the expanded body. These are source-model/compiler residuals, not
permission to paste the helper back into callers.

The map-editor RMG trace separately passes byte identity. It retains two
`writePackedBits<10>` calls where retail expands the outer writer and retains
the nested `bitset<10>::_Xran` calls. Both have nine total calls, demonstrating
why aggregate equality would miss this mismatch.

The editor's native const-subscript and temporary explicit-inline probes are
byte-flat. Its recovered calls currently encounter VC6 post-expansion vetoes:
WitchHut and town event 38/12; town spells 31/12. The unproven editor inline
qualifier was removed. No dummy code, inline-depth pins or standalone behavior
mocks were introduced. Source-owned losses and unchanged-source CUR drift are
reported separately; prior HIST remains available.

## Measured caller costs

Targeted VC6 comparisons after the counted-clear recovery:

| Image / edited caller | Previous MAX | New comparison |
| --- | ---: | ---: |
| Game `game::save` | 92.81% | 92.22% |
| Game `NewSMapHeader::save` | 95.45% | 88.34% |
| Game `writeRmgObjectPrototype` | 100.00% | 91.26% |
| Game `NewfullMap::saveObjectType` | 100.00% | 94.09% |
| Game `town::save` | 100.00% | 100.00% |
| Map editor `writeRmgObjectPrototype` | 99.65% | 69.15% |
| Map editor `TWitchHut::write` | 100.00% | 14.16% |
| Map editor `TTown::TTimedEvent::write` | 100.00% | 40.16% |
| Map editor `TTown::write` | 98.84% | 68.59% |

The three game source costs are observed after restoring four calls. Player,
hero, campaign reader and existing RMG helper consumers showed no additional
projected MAX loss in the targeted comparisons. The editor costs expose retained
helper calls or changed expanded layout; they are not silently hidden by older
HIST values. The RMG Mac pair remains available at 20.2331% versus the preceding
20.5128%. Its source annotation records this one specific canonical-helper
recovery against the preceding source hash and CUR; it does not waive later
regressions. The edited editor TUs have no scored Mac pairs.

The mutable family preserves town's Windows exactness. Mapcell's remaining
two nested `test`/`_Xran` differences are confirmed by a byte-identical trace.
Its Mac comparison was already unavailable due to an unresolved scalar writer;
named calls and proxy setup remain inspectable. Town's Mac score changes from
64.6164% to 64.5503%, one matching byte fewer with the same linked extent and
named calls. Its own narrow abstraction annotation records that recovery.
Neither annotation grants a general score exception or claims exactness.

The integrated fresh game and map-editor reports bank four source-owned MAX
resets each, retaining HIST. Town banks its new source hash at 100%. Unchanged
caller CUR movement includes game `writeMapHeader` 97.0829 -> 95.3396 and twelve
GameMap placement/stream template rows near 99.92--99.95%; their MAX remains
unchanged. No caller padding or declarations were added to chase those values.
Publication uses `homm3 land --mac`, including the required game/both-editor
build and placement cycle and full Mac preservation check.
