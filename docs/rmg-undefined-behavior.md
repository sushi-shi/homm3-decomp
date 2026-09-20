# RMG undefined behavior and uninitialized state

The pinned English GOG Complete retail generator is not a pure function of its
seed and settings. The following uninitialized reads are present in retail and
preserved in the recovered implementation. Initializing them in game source would
change retail behavior; the [execution oracle](rmg-oracle.md) instead records and
controls storage contents at its boundary.

RMG has no Dreamcast counterpart. The evidence below is retail x86 code and
execution, not inferred original source text or a sanitizer result. These are
known issues, not an exhaustive UB audit.

## Initial key-tent color: uninitialized stack integer

- Field: `type_random_map_generator::m_nextKeyTentColor`, offset `+0xf5c`.
- The retail constructor and object-generator initialization leave it untouched.
  `TRandomMapRequest::generateToFile` constructs the generator on the stack.
- Retail `0x540d6d` uses this integer to select the key-tent subtype.
  After placement, `0x540f68` disables the color and searches for the next one.
- Different caller frames produced different key-tent/guard colors in otherwise
  identical maps. This is an indeterminate integer read in the recovered C++.
- The oracle enters both implementations through the same thunk and fills 64 KiB
  of future stack storage with the recorded `stackWord` before entry.

## Added water-zone town flags: uninitialized heap bytes

- `buildZoneBoundaries` (`0x53e050`) allocates a `TRmgTownSlot` for each added
  surface water zone. It initializes several fields, but not `m_allowedTowns`
  (nine bytes at `+0x41`). The slot has no scalar-initializing constructor.
- Its zone-constructor call at `0x53e45c` reaches `TRmgZone::TRmgZone`
  (`0x5329e0`). The loop at `0x532a47` counts nonzero town flags. If any are set,
  the constructor calls `rand` at `0x532a5b` to select a town.
- Reused allocation contents therefore decide whether a random draw occurs.
  The subsequent water-terrain assignment does not undo that RNG advancement.
- Retail-vs-retail runs produced different maps and final RNG states. Tracing
  localized the first extra draw to this constructor; slot snapshots showed
  old pointer bytes in the town flags. Candidate exhibited both output variants
  too. Supplying the same fresh allocation contents restored agreement.

Two observed requests, with other request fields at oracle defaults:

```json
[
  {"name":"heap-history-108", "seed":1, "mapVersion":1,
   "width":108, "height":108, "levels":2,
   "waterContent":1, "monsterStrength":1, "stackWord":0, "heapByte":null},
  {"name":"heap-history-144", "seed":123456, "mapVersion":1,
   "width":144, "height":144, "levels":2,
   "waterContent":3, "monsterStrength":-2, "stackWord":0, "heapByte":null}
]
```

For the first request, observed final RNG states were `2037487062` and
`898566223`; map lengths were 224787 and 225516 bytes. A fresh native-heap run
need not exhibit both variants. Allocation history and instrumentation affect
the symptom. The oracle's `heapByte` supplies allocation contents without
changing RMG instructions; `null` retains native allocation behavior.

## Temporary boundary slot: uninitialized stack town flags

`buildZoneBoundaries` also creates a stack `testSlot`, initializes only its zone
index, kind and size, and passes it to the same zone constructor at `0x53e149`.
The constructor reads the same uninitialized nine town flags. A retail snapshot
at that constructor's random draw contained stack/address residue in those bytes.
This is a separate source of indeterminate input even when fresh heap allocations
are filled. Initial stack fill does not control storage overwritten by intervening
calls, so repeatability checks remain necessary.

## River drawing: unchecked out-of-range coordinates

After its path-search queue becomes empty, `createRiver` tests the last inspected
tile's river-target flag at `0x5492a7..0x5492b2`. That tile may have been rejected
by the neighbor filters; this test does not establish that the search reached it.
There is no separate successful-search flag or reachable-cost check. A target
whose cost remains the reset value `32000` and whose predecessor remains
`(-1,-1,-1)` can therefore enter drawing.

The sampled request below crashes reproducibly in both implementations. Retail
`createRiver` follows that predecessor and calls the line walker at `0x5496ef` with
destination `(0xffffffff, 0xffffffff)`, the unsigned representation of `(-1,-1)`.
There is no coordinate-validity check in this predecessor loop. The eventual
fault is the unchecked tile-array read at `TRmgMapAdapter::getLand`, `0x53284f`.
The captured query is `(0xffffffff, 0xffffff35)` (signed `(-1,-203)`) on a 108×108
map. A snapshot at the accepted-target branch (`0x549315`) captured target
`(55,0,0)` with cost `32000` and the invalid predecessor. Two further sampled
crashes (`sample-02899` and `sample-02953`) reproduced this same failure, with
unreached targets `(79,0,0)` and `(38,0,0)` respectively. Their source-to-target
searches had no valid predecessor chain, but drawing proceeded anyway.

The captured retail call chain is `createRiver` → `TRmgLineWalker::drawTo` →
`paintPoint` → `refreshRmgLinePoint` → `TRmgLinePainter::getLand` →
`TRmgMapAdapter::getLand`. Candidate faults at the corresponding instruction
with the same coordinates. This is not a candidate-only reconstruction error.

```json
[
  {"name":"river-invalid-coordinate", "seed":2683630114,
   "width":108, "height":108, "levels":1, "mapVersion":0,
   "humanPlayerCount":6, "computerPlayerCount":2,
   "humanTeamCount":3, "computerTeamCount":1,
   "isHumanSeat":[1,0,1,1,1,1,1,0],
   "townType":[-1,-1,2,-1,-1,-1,1,6],
   "waterContent":0, "monsterStrength":-2,
   "stackWord":4294967295, "heapByte":85}
]
```

## Monolith guard placement: negative zone index

`createMonolithConnection` places a guard one tile below a monolith through the
shared `placeGuard` operation. Retail `0x54308a..0x543099` extracts the tile's
signed zone index and immediately indexes `m_zones`, without checking for `-1`.
The request below reaches an unassigned tile at `(53,112,0)`: its packed state
at `+0x20` is `0xffff0003`, whose zone byte is `0xff`. The resulting
`m_zones[-1]` read yields the invalid pointer `0x91` in the observed run.
`createGuard` then faults while dereferencing it at `0x540b34`.

Retail and candidate reproduce the same negative index and invalid pointer.
The out-of-bounds vector access is established independently of the particular
allocator word (`0x91`) that happens to be returned. This issue is distinct from
uninitialized memory: filling fresh allocations does not make index `-1` valid.

```json
[
  {"name":"monolith-unassigned-guard", "seed":2488707456,
   "width":144, "height":144, "levels":1, "mapVersion":2,
   "humanPlayerCount":1, "computerPlayerCount":2,
   "humanTeamCount":0, "computerTeamCount":1,
   "isHumanSeat":[1,0,0,0,0,0,0,0],
   "townType":[-1,3,6,-1,6,-1,-1,-1],
   "waterContent":0, "monsterStrength":0,
   "stackWord":4294967295, "heapByte":128}
]
```

These crash reproductions use the installed Steam data set from the sampling
campaign; the archived run's `provenance.json` records the exact asset hashes.
The two cases are `sample-00090` and `sample-00443` under
`build/rmg-oracle/sampled-10000/`. Diagnostic replays under
`build/rmg-oracle/crash-probe/` retain registers and readable memory at the fault.
The river-search snapshots are under `build/rmg-oracle/river-probe/`.
These ignored artifacts are session evidence; the requests and retail addresses
above are the durable reproduction notes.

The later 100,000-case campaign reproduced only these two crash classes: 137
paired retail/candidate faults in `TRmgMapAdapter::getLand` during river drawing
and 23 paired faults in `createGuard` after the negative zone lookup. Every fault
repeated at the same corresponding instruction, and the campaign found no
candidate-only crash or additional crash signature.

## Handling findings

Keep retail bugs separate from candidate reconstruction errors. The oracle reports
nonrepeatability and crashes explicitly; neither counts as a passing map comparison.
Preserve the full request, seed, memory-fill inputs, binary/asset provenance and
raw outputs when reproducing a finding. Record a new UB cause here only after its
retail read and missing initialization or invalid access have been established.
