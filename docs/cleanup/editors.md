# Editor cleanup

This pass follows HoMM1/HoMM2's constant-domain and source-readability cleanup:
reuse an existing domain only when the operation has that meaning, preserve
native names and ABI, and compare the affected Windows translation units.
The starting revision is `eb0115861`.

## Tile-neighbour directions

`include/TilePoint.h` owns `ETileDirection`: eight adjacent cells, clockwise
from north. Its evidence comment records the game and editor offset tables
and the boundary-mask builder. `TWorldDirection` in
`include/editor/WorldDirections.h` describes map regions and includes the
centre; it remains a separate domain despite its overlapping values.

The editor's `GameMap.cpp` beach-border and coastline checks and
`MapValidation.cpp` reachability check still spell these same adjacent-cell
indices and array extents as numbers. Reuse of `TILE_DIR_*` and
`TILE_DIR_COUNT` replaces 17 bare numeric occurrences. The five-direction
approach subset
retains the literal `5`: it counts the first five entries in `akDirOrder`,
not the enum value for southwest. The table remains `const int[]`; this
pass does not infer a new original local type.

## Nested search exits

Two source `goto`s remain in the editor trees at the starting revision:

| Function | Retail VA | Search exit |
| --- | --- | --- |
| `TGameMap::_TImpl::setVersion` | `0x00421319` | Stop searching both map layers after migrating the first matching hero. |
| `TMapValidationFunc::_checkForUnreachableObjects` | `0x0047dac4` | Stop the direction, column and row searches after finding one reachable trigger. |

Both functions start at MAX 100%. The probes preserve first-match behavior,
iteration order, object lifetimes and the post-search work, but neither has
native evidence establishing its new flag/test structure:

- A `heroMoved` flag propagated through nested breaks changes the layer-loop
  branch polarity. Testing that flag in the outer `for` condition instead
  scores 89.675% in the fresh targeted build.
- Propagating the existing `bReachable` flag through direction/column/row
  breaks changes 44 retail blocks to 48 and scores 89.9647% in the focused
  comparison.

Both probes were reverted. The two shared exits remain; concise failed-probe
notes sit beside their owning functions. This records investigated cleanup
rather than claiming that absence of a `goto` proves better original source.

## Retained behavior and follow-up

- `computeAdjacentDirs` continues receiving map width for both dimensions in
  the Complete validator. Its existing comment documents the Loki difference;
  enum cleanup is not a change to that retail behavior.
- Hero IDs `4` and `144` in version migration are retained. Naming them needs
  an evidence-backed shared hero-ID owner; a same-valued unrelated enum does
  not establish reuse.
- `kArtifactVialOfDragonBlood = 127` remains editor-owned for now because
  the shared artifact enum at the starting revision lacks that member. The
  serialized `bitset<127>` and version cutoffs are not automatically the same
  semantic constant.
- The campaign-editor source has no `goto` at this revision. This pass does
  not claim that all its constants, declarations or names have been reviewed.

## Validation

The worktree, outputs, VC6 SP3/RTM compiler copies and staged MFC files are on
NVMe SSD. Existing receipts were checked rather than overridden. Most editor
cache receipts failed because the unit manifest differed; a fresh editor
object pass and retail delink therefore initialized valid comparison state.

The final targeted command is:

```sh
homm3 --image h3maped build --fast GameMap MapValidation -j2
```

The final build passes and reports no source MAX change in either unit:
`GameMap` retains 815/839 exact functions and `MapValidation` 29/29. The
three enum-edited functions and the restored `setVersion` retain MAX 100%.
The temporary probe failures are not retained source MAX losses. Scores and
README are refreshed with `homm3 --image h3maped status update --write-readme`.

No campaign-editor source was changed. The accepted source work uses the
already evidenced tile enum; it adds no new helper, signature, ABI change,
compiler pragma or duplicate enum domain.

## Packed-bit writer recovery

The later packed-writer sweep restores four complete fixed-size bitset output
operations: `TWitchHut::write`'s skill mask, `TTown::TTimedEvent::write`'s
build mask, and `TTown::write`'s obligatory and disabled spell masks. Each now
calls the existing `writeBitset` in `include/editor/RawStream.h`.

Loki's native dynamic symbols retain `writeBitset<48>` at `0x0837b380` and
`writeBitset<9>` at `0x0837b580`: clear the entire byte array, test bits through
const subscript, pack low-bit first, then call the array stream operator.
That operator throws `TRawOStream::TWriteFailure` on a short write. The helper
now uses the native const subscript; VC6's library forwards that subscript to
`test`, preserving the checked retail operation. No stream/error contract or
explicit inline qualifier changes.

Native Loki `TTown::TTimedEvent` output at `0x08186cb4` retains `getBuildMask`
at `0x08186cf3` before its recognizable packing operation; that accessor is
also restored. Loki town spell output `0x0818aee5..0x0818af72` expands the same
nine-byte operation. The older Loki executable has no WitchHut writer symbol;
its Windows operation is identified by the existing retained `writeBitset<28>`
at `0x004342e7`, whose checked packing and array-write path is also expanded
in retail `TWitchHut::write` at `0x0048f8a1`.

The following packed-looking operations remain distinct:

- Town built/disabled output derives two planes from building-state objects,
  with disabled taking priority. Loki `0x0818adb8..0x0818aeaa` retains that
  coupled conversion and the two member predicates; it has no bitset input.
- Event/player, timed-event/player and abandoned-mine/resource masks are
  scalar bytes. Loki calls scalar unsigned-char output at `0x0813b9be`,
  `0x08185fdb` and `0x081727ae`, respectively, rather than array output.
- Campaign prerequisites have a runtime-sized byte vector and conditional
  packing for nonempty map filenames; output already uses `writeFromIter`.
- Campaign's existing fixed-size crossover masks already call `writeBitset`.
  Its separate RawStream copy retains its `_read`/`_write` layers and original
  qualifiers; no shared encoder or cross-image stream unification is added.
- Game `TAbstractFile` output returns integer complete-write success. It is
  a separate API from the editor's void helper with throwing output, despite
  the shared low-bit-first encoding algorithm.

The targeted `Town ObjectSpecializations GameMap` build passes. Restoring
calls currently exposes an inlining mismatch: `TWitchHut::write` moves from
MAX100 to14.16%, town timed-event output100 to40.16%, and town output98.84
to68.59%. Existing GameMap helper instances report no MAX change. A fresh
byte-identity-gated trace shows the WitchHut helper passes its cb97/budget1000
test but is rejected by a post-expansion veto38/12; its array output operator
is separately vetoed18/12. Const subscript and a temporary explicit-inline
probe both preserve those scores. The unproven inline qualifier was removed.
This corrects the earlier inliner document's unsupported inference that a
compiler veto proves the original caller pasted a loop. Keep the canonical
helper call while investigating the compiler state, and retain historical
peaks in HIST rather than describing these changes as byte improvements.

Neither edited editor TU has Mac pairs. Mac scoring and call-stream checks
for these functions are unavailable, not passed. No game Mac claim is reused
for an editor function. These changes were compiled only in the isolated SSD
worktree; no compiler pragmas, source padding or mock behavior tests were used.

The town trace also passes the byte-identity gate (198/198 roots reproduced).
Its timed-event bitset call is vetoed38/12 after cb97 passes budget1000;
`getBuildMask` expands. Both spell writers pass remaining budgets705/608,
then veto31/12. Their nested subscript expands, but its nested `test` has only
8/6 budget left. Thus even identical retained helper code can have different
nested expansion opportunities at different callers. The three-TU checks
include the retained GameMap instances, not only the newly restored calls.

Operation identity and the native helper body support this recovery; they do
not uniquely prove that each original source caller spelled a helper call.
The current compile mismatch remains explicit, with no claim that the
subscript spelling or header placement alone solves retail inlining.

Banking was attempted after the final targeted build. `status update` correctly
refused because the shared RawStream edit makes76 other unit receipts stale.
The worker did not bypass freshness or edit scores manually. Integration's
fresh map-editor comparison refreshes those receipts and banks the combined
scores, retaining HIST. The source-only worker commit leaves the prior ledger
untouched; the coordinator's follow-up owns that score update.
