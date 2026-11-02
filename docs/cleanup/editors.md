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
