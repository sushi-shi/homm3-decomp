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

## Packed-bit writer boundaries

`include/editor/RawStream.h` retains the canonical complete `writeBitset`
operation and its native const-subscript path. Loki's dynamic symbols retain
`writeBitset<48>` at `0x0837b380` and `<9>` at `0x0837b580`: clear through
`memset`, pack low-bit first, then call array stream output. The map-editor
Windows instances instead match the current `fill_n` spelling. VC6's const
subscript forwards to checked `test`; no explicit inline qualifier is proven.
The stream operator still throws `TWriteFailure` on a short write.

The earlier blanket restoration of four caller operations to this complete
writer inferred too much from equivalent packed bytes. A native boundary
audit distinguishes the town operations from the retained helper:

- Loki town-event output calls the retained `fill_n<unsigned char*, unsigned
  int, int>` at `0x08186ce4`, then `getBuildMask` at `0x08186cf3`. It calls
  `getGeneratorBonuses` at `0x08186d6c` before the chained array and bonus writes
  at `0x08186d82`/`0x08186d8d`. The complete writer would evaluate its mask
  argument before its own clear and finish array output before the bonus
  getter. Restore the native caller operation, both accessors and the chained
  output. The base cast remains: VC6 rejects `::TTimedEvent::write` with
  C2352 and `this->::TTimedEvent::write` with C2299 inside the same-named
  nested class. No invented alias or encoder abstraction is added.
- Loki town spell output calls that same retained `fill_n` at `0x0818aef9`,
  loops over 70 bits and writes the array at `0x0818af6d`. Its `fill_n` is a
  decrement/store loop, not a wrapper around the retained helper's `memset`.
  Complete adds an obligatory plane and clears the same caller buffer at
  `0x004c2c16` and `0x004c2c69`. These facts support the caller's shared spell
  buffer and separate packing operations. The shared stack address alone
  would not prove a boundary: inlined helper temporaries can reuse storage.

This corrects the prior claim that these town operations were established
expansions of the complete helper. The older Loki build is source evidence,
not the Windows byte target; the revised whole caller is compared under VC6.
Canonical `writeBitset` and its supported GameMap/Hero/Generator/objecttype
uses remain intact.

`TWitchHut::write` remains different. Its Windows body at `0x0048f8a1` has the
same checked bit test, `_Xran` and array-output path as retained
`writeBitset<28>` at `0x004342e7`, with no external operation interleaved.
Its buffer reuses the dead version slot after the guard, which is compatible
with an inlined helper local. No positive ordering or lifetime contradiction
was found, so its helper call remains despite the unresolved 38/12 compiler
veto and current 14.1628% score. A veto alone does not prove original pasted
source. No WitchHut symbol exists in the older Loki editor used here.

### Controlled matching investigation

The finite JSON family checks six full-buffer initialization forms against
three ascending loop forms: existing `fill_n`, typed `fill_n`, aggregate zero,
`memset`, two range-fill forms; and ordinary `if`, early `continue`, `while`.
All 18 source states scored across Town, ObjectSpecializations and GameMap,
producing eight distinct aggregate object sets. Unchanged-source, opposite-
corner and four frontier reproduction controls pass. A premature caller edit
initially tripped the final freshness guard; restoring the watched source and
rerunning the cached family with fresh controls resolved it without a bypass.
Every state left the three
caller scores at 14.1628%, 40.1594%, and 68.5895% respectively. Const-subscript,
`test` and explicit-inline probes had previously also been flat.

Fresh retained-body scores were inspected directly: template rows can retain
historical MAX even when a changed header body emits worse code. Baseline
GameMap has six writer instances at CUR 100 and `<28>` at CUR 99.88095. Direct
`memset` lowers all seven (minimum 77.2449); aggregate zero lowers six
(minimum 87.4792); range-fill forms lower all seven (minimum 69.3878).
Typed `fill_n` and the alternative loop forms preserve the retained bodies
but do not restore caller expansion. None of these header variants is adopted.
Temporary manifests/results remain under ignored `build/`; no compiler
pragmas, false qualifiers or dummy operations were used.

### Adopted caller result

The native town-event model measures CUR 99.927536%, up from the introduced
complete-writer call's 40.1594%. Its register and flow distances are both zero;
`diagnose` classifies the remaining displacement/relocation difference as
masked-equal. The town writer initially returns to 98.82096% after restoring
its caller buffer. A separate controlled three-state family tests the event
count's actual lifetime: direct long conversion 98.821%, named long 98.7205%,
and named `std::vector<TTimedEvent>::size_type` followed by the existing long
conversion 99.9782%. All three states and both retained frontier candidates
reproduce. The size_type form matches the native count-store/stream-load
ordering and is adopted, improving beyond the former 98.84% MAX. This is a
real count value used by serialization, not a dummy declaration. Final
`diagnose` also reports zero register/flow distance and masked-equal residual
for the town writer; neither remaining percentage gap is an inlining wall.

The final targeted command is `homm3 --image h3maped build --fast Town
ObjectSpecializations -j2`. The shared RawStream header is unchanged, as are
its retained GameMap writer bodies measured in the initial three-TU baseline.
The WitchHut helper remains 14.1628%; this pass does not claim to have recovered
its inlining. Prior HIST is preserved. The branch switch leaves unrelated
unit receipts stale (`status update` refuses the incomplete report), so the
worker does not run a duplicate full image cycle
or manually edit scores; the coordinator refreshes and banks the PR's final
combined image. Only source and evidence are included in the worker commit.

### Other encodings and coverage

Town built/disabled output derives two planes from building-state objects,
with disabled taking priority. Loki `0x0818adb8..0x0818aeaa` retains the coupled
conversion and both predicates. Event/player, timed-event/player and
abandoned-mine/resource masks are scalar bytes: Loki calls scalar output at
`0x0813b9be`, `0x08185fdb` and `0x081727ae`, respectively, rather than array
output. Campaign prerequisites have runtime-sized vectors and conditional
packing; output already calls `writeFromIter`. Campaign fixed-size crossover
masks already call their own `writeBitset`, preserving that image's stream
layers and qualifiers. The game's counted `TAbstractFile` writer remains a
separate API from throwing editor streams.

No edited editor function has a Mac pair; Mac scoring and call-stream checks
are unavailable, not passed. Work and compiler outputs remain on the isolated
SSD worktree. This follow-up is for a normal reviewable PR, with no default
branch merge or `homm3 land` operation.
