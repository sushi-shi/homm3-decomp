# RMG undefined behavior and uninitialized state

The pinned English GOG Complete retail generator is not a pure function of its
seed and settings. The following uninitialized reads are present in retail and
preserved in the recovered implementation, except the boundary test slot, whose
observed effect is modelled explicitly below. Initializing the others in game
source would change retail behavior; the [execution oracle](../oracles/rmg.md)
instead records and controls storage contents at its boundary.

RMG has no Dreamcast counterpart. The evidence below is retail x86 code and
execution, not inferred original source text or a sanitizer result. These are
known issues, not an exhaustive UB audit.

The default build keeps all of this retail behavior. The opt-in compile-time
switch `HOMM3_RMG_HOTFIX` (described in `include/rmg.h`) instead selects
defined, non-crashing alternatives at each site: unreached river targets are
not drawn, guards are skipped on unzoned cells or when no creature is
selectable, uninitialized town flags and the first key-tent colour get fixed
values, and short template rows, empty road lists and long map descriptions
are bounded. It also validates the request, templates, placement rules and
object prototypes where they enter the generator (see
[Conditional contracts](#conditional-contracts-and-unresolved-hazards)).
Hotfix maps differ from retail and are not oracle targets; each site is an
`#if defined(HOMM3_RMG_HOTFIX)` block in the source.

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

- `buildZoneBoundaries` (`0x53e050`) allocates a `TRmgTemplateZone` for each added
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
index, kind, size and connection vector, and passes it to the same zone
constructor at `0x53e149`. Retail never writes the slot's nine town flags
(`[ebp-0x123]..[ebp-0x11b]`, `testSlot+0x41`). If any is nonzero, the
constructor draws one `rand()` at `0x532a5b`. The chosen town is unused:
`canPlaceZone` reads the alignment only for human and computer slots, and the
test slot is a junction. Its only observable effect is that one draw.

**Residue.** The oracle captured the slot at the constructor call through a
call-site thunk that writes no stack storage (see the `RMG_TRACE_SITE` hook in
`scripts/homm3/rmg/driver.cpp`). With the harness entry stack at `0x22f518`,
retail's slot is at `0x22de54`, so the flags are bytes `0x22de95..0x22de9d`.
`buildZoneBoundaries` writes none of them, and its callees run below its frame.
Both levels therefore see the bytes left before `generate` called it: the last
writes at that depth by `initializeZones`' zone positioning, where vector
reallocation under `positionZone` reaches `operator delete` → `free`. Across
3,417 captures from 3,000 requests (2,000 sampled requests and million-v1
requests 0..999, retail run with heap fill), the flags held:

| Flag bytes 3..6 (one aligned stack dword) | Captures | Writer |
| --- | ---: | --- |
| `0x0061960a` | 2,082 | `free`'s return address from `_unlock(9)` (small-block path) |
| `0x0061b305` | 457 | `_unlock`'s return address from `LeaveCriticalSection` |
| `0x00619626` | 278 | `free`'s return address from `HeapFree` (large-block path) |
| `0x600`, `0xc00`, `0x1800` | 599 | Wine `RtlFreeHeap` internals below `HeapFree` |
| `0x30001752` | 1 | the harness's allocation-fill wrapper (an oracle frame) |

Flag bytes 0..2 hold a saved Wine frame pointer (`0x0022de..`) or zero, and
bytes 7..8 a lock number, critical-section or heap address. That dword was
nonzero in every capture, so retail always drew. The exact bytes depend on
which heap call last reached that depth, on CRT and host DLL frames, and on
oracle frames; they are not a function of generation state alone. On Windows,
the `HeapFree` path would leave different words.

The candidate's slot was 4 bytes higher (`0x22de58`; `buildZoneBoundaries` is not
byte exact), where it read other heap-call residue, also always nonzero. A
frame change exposed the hazard: splitting `initializeZones`' square origin into
new locals (`0feba6777`) left all nine bytes zero for million-v1 `m0000076` and
`m0000135`. The candidate skipped the draw and its maps diverged.

**Source model.** `buildZoneBoundaries` now allows all nine test-slot towns
explicitly, preserving retail's one draw without reading the stack. This costs
VC6 match: the function moves from 72.15% to 70.46% because retail emits no
store. With that initialization and the origin-split perturbation re-applied,
the same 3,000 requests matched retail: 2,993 equal maps, plus 7 river-drawing
faults at corresponding instructions on both sides. That set includes
`m0000076` and `m0000135`. Without the perturbation, the same 3,000 requests
also matched. With `getLandType()` returning `TTerrainType` (which changes later
generator frames), million-v1 requests 0..999 matched. This is sampled
agreement. A state whose last heap call leaves that dword zero would make
retail skip the draw; none was observed. The real fix (no towns, so no draw)
is the `HOMM3_RMG_HOTFIX` branch. It changes generated maps.

**Added water zones.** Their heap `TRmgTemplateZone` flags remain uninitialized
(oracle `heapByte`). The resulting `TRmgZone::m_alignment` has no consequential
reader under the reviewed paths. `canPlaceZone` tests it
only for human and computer kinds. `placePrimaryTown` and
`placeAdditionalTowns` see zero town counts and densities. The town-zone
counts in `generate` and the player alignments in `writeMapHeader` require a primary town or a player.
`createGuard` is reached only through `placeGuard` or
`assembleTreasureGroup`. Water zones have monster strength none, so
`getZoneGuardValue` returns 0; their connections are unguarded. These zones
also never set `m_creatureTownType`, because `chooseCreatureTownType` runs only in
`initializeZones`, before they exist. `TRmgZone`'s constructor leaves
it as allocation contents. `createTreasureObject` still reads it through
`getValue` for creature rewards, dwellings and quest creatures. This happens for every candidate definition, before the water-terrain
prototype filter. Whether a water prototype makes that read affect output was
not traced.

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
fault is the unchecked tile-array read at `TRmgRiverMapAdapter::getLineType`, `0x53284f`.
The captured query is `(0xffffffff, 0xffffff35)` (signed `(-1,-203)`) on a 108×108
map. A snapshot at the accepted-target branch (`0x549315`) captured target
`(55,0,0)` with cost `32000` and the invalid predecessor. Two further sampled
crashes (`sample-02899` and `sample-02953`) reproduced this same failure, with
unreached targets `(79,0,0)` and `(38,0,0)` respectively. Their source-to-target
searches had no valid predecessor chain, but drawing proceeded anyway.

The captured retail call chain is `createRiver` → `TRmgLineWalker::drawTo` →
`paintPoint` → `refreshRmgLinePoint` → `TRmgRiverLinePainter::getLineType` →
`TRmgRiverMapAdapter::getLineType`. Candidate faults at the corresponding instruction
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
paired retail/candidate faults in `TRmgRiverMapAdapter::getLineType` during river drawing
and 23 paired faults in `createGuard` after the negative zone lookup. Every fault
repeated at the same corresponding instruction, and the campaign found no
candidate-only crash or additional crash signature.

## Handling findings

Keep retail bugs separate from candidate reconstruction errors. The oracle reports
nonrepeatability and crashes explicitly; neither counts as a passing map comparison.
Preserve the full request, seed, memory-fill inputs, binary/asset provenance and
raw outputs when reproducing a finding. Record a new UB cause here only after its
retail read and missing initialization or invalid access have been established.

## Review classification (2026-10-01)

The findings below separate instruction-established defects, C++
reconstruction risks and conditional input contracts; none is repaired. An
unchecked operation is not evidence that a shipped template reaches it, and no
new crash reproduction was run for this review.

### Object removal: failed search is tested against null

`type_random_map_generator::removeObject` (`0x54bc50`) searches both
`m_objects` and each occupied tile's `m_objects`. Retail tests the returned
pointer with `test ecx,ecx` at `0x54bc95` and `test edx,edx` at `0x54be6d`, then
enters the expanded erase. It does not compare either result with the vector's
end pointer. The source's `if (found)` and `if (entry)` preserve those tests.

If the object is absent from a vector with nonnull storage, the result is its
end iterator and the erase is invalid. In the first copy loop, the source begins
at `end + 1` and advances until equality with `end`, so the failed lookup can
read/write outside the allocation rather than merely remove the wrong item.
An empty vector retaining capacity also has nonnull `end()`. The normal caller
contract is that the object belongs to the generator and every affected tile.
The missing check is instruction-established; no valid-request violation of
that membership contract was reproduced in this review. Changing the predicates
to `!= end()` would fix retail behavior and is intentionally deferred.

### Map description: fixed buffer with unbounded appends

`writeMapHeader` (`0x549cb0`) formats and appends into `char description[500]`.
Retail forms the destination at `[ebp-0x324]`, calls `sprintf` at
`0x549df6`, then uses unbounded NUL scans and copy loops for the version and
player descriptions. Neither the template name nor the accumulated description
length is checked. This is an instruction-established unbounded write, not just
a possible format-string mismatch in the reconstruction.

Length arithmetic already gives an overflowing example: template name `X`,
seed `1`, width `36`, levels `1`, human count `8`, computer count `0`, water
`None`, internal monster strength `3` (request strength `0`), second-expansion
format, all eight human flags,
and a fixed town choice for each seat produce 525 text bytes plus the NUL.
This is a calculation from the retained literals, not an executed map request.
Long template names can also overflow the initial `sprintf` independently of
player annotations. No truncation or larger buffer is introduced by cleanup.

The description has a separate preserved correctness defect: its town string
is indexed by the player number, not `m_townChoices[player]`. Retail
`0x549fba` loads `g_rmgTownNames` with that player offset. This changes only the
reported town choice; correcting it would change generated output.

### River coast test: right boundary admits x equal to width

`markRiverCoastTarget` (`0x548a40`) rejects `x > width`, while its Y check rejects
`y >= height`. Retail's first X comparison uses `jg` at `+0x74` and its Y
comparison uses `jge` at `+0x87`; the later strips repeat the asymmetric check.
The flattened lookup therefore admits `x == width`. Except on the final row of
the final level, this aliases the next row/level's first cell; at the end of the
whole allocation it is an out-of-bounds read. The source retains this exact
boundary. The review establishes the defective check, not that a shipped
request necessarily reaches the allocation-end case.

`createRiverToObject` (`0x548500`) also tests its last inspected tile after the
queue empties and follows predecessors if that tile has the `m_hasRiver` flag
(a different flag from `createRiver`'s `m_riverTarget`). Its
source has the same missing-success-state risk as `createRiver`. The crash
requests above establish `createRiver` specifically; they do not establish a
second reproduced crash class in `createRiverToObject`.

### Guard selection: counted eligibility can exceed selectable prototypes

`createGuard` (`0x540b20`) counts eligible creature traits without requiring a
nonnegative `prototypeIndices[creature]`. Selection subsequently decrements the
random rank only for nonnegative prototype indices. If any counted creature
lacks an object prototype, a sufficiently large random rank exhausts the scan
at `creature == -1`. Retail falls through from that scan to the indexed load at
`+0x164` without a success check, followed by the object-prototype and creature
trait reads. This is a conditional invalid-index path confirmed in the native
body; no incomplete-prototype asset set was executed during the review.

The RoE exclusion has a distinct compatibility quirk: it clears entries
144..118, evaluates eligibility for 116..0, then selects across 144..0. Slot 117
retains its loaded prototype without the eligibility test.
`RMG_ROE_CREATURE_TYPE_COUNT` (118) is the exclusion boundary; evaluation
starts two below it because retail decrements the exclusion loop's exit index
(117) once more. Do not align these boundaries or add prototype presence to
eligibility while doing source cleanup.

### Factory destruction: C++ lifetime risk with direct retail deallocation

`m_objectGenerators` owns derived factory instances through
`type_treasure_def*`, whose base has virtual factory methods but no virtual
destructor. Deleting these derived objects through that base is undefined by
C++ lifetime rules. Retail's generator destructor directly calls operator
`delete` for each factory at `0x537df0 + 0xd0`, without virtual destructor
dispatch. Current factory subclasses contain only scalar additions, so there
is no evidenced lost resource cleanup or observed retail failure here.

This is a standards-level risk in the recovered declaration/operation, not
proof of the unavailable original C++ spelling. Adding a virtual destructor
would change the vtable layout. Any later portability repair must retain the
direct-deallocation contract or deliberately establish a new ownership model.

### Failed quest/key-tent replacement: removed wrapper is never destroyed

`removeObject` (`0x54bc50`) removes the object from the generator and tile
pointer vectors, but never destroys or deallocates it. This operation alone is
not a leak: a caller could retain ownership. The two actual replacement callers,
however, abandon the removed wrapper:

- `rmgKeyTentObject::completePlacement` (`0x5338e0`) removes `this` at `+0x4a`,
  attempts to generate a replacement, and returns false without deleting the
  original key tent.
- `placeQuestArtifact` (`0x54b490`) removes the artifact wrapper at `+0x298`
  when `placeQuestGroup` fails. Its remaining cleanup destroys the temporary
  group's containers and map, not the wrapper. On return,
  `rmgQuestArtifactObject::completePlacement` (`0x533a50`) deletes the seer hut through
  the member at `+0x20` and clears that member, but does not delete itself.

Retail `commitTreasureGroup` calls vtable slot `+8` at `0x546c55` and advances
the loop immediately, ignoring the returned byte. Its group vector contains
nonowning pointers; clearing or destroying that vector does not delete them.
The generator destructor (`0x5363b0`) only deletes objects still in `m_objects`.
Consequently the removed wrapper allocation survives generation. Its missing
base destructor also leaves the prototype's `m_refCount` elevated, so a prototype
used only by a removed object can still enter the serialized prototype table.
Deleting the wrapper as a cleanup would therefore affect both allocation
history and potentially map bytes.

These are native control-flow and ownership findings, not newly executed leak
reproductions. They apply when the specified replacement path is taken; an early
`placeQuestArtifact` failure due to an empty artifact pool does not remove the
wrapper and is not this leak. Preserve the existing lifetime behavior.

## Conditional contracts and unresolved hazards

Source review leads, not reproduced retail bugs unless evidence is given
above. The second column is what the operation needs; the third says what
guarantees it under `HOMM3_RMG_HOTFIX`. That build validates external input
once where it enters the generator, so later code can rely on it:

- **Request** (`generateToFile`, before the player-count repair): square
  36/72/108/144 maps, 1–2 levels, 0–8 humans and computers with at most eight
  in total, team counts 0–8, water 0–3, monster strength −2..2 (the range the
  clamp leaves unchanged), map version 0–2, and towns −1 or a type the
  version knows. Otherwise `RANDOM_MAP_GENERATION_FAILED`.
- **Template** (`loadTemplates`, after the retail filters): a template is not
  offered unless it has zones, every zone has a positive size and a player
  number 0–8 (1–8 for human/computer zones), its town, mine and treasure
  density products and weighted counts fit in half the `s32` range, and its
  distinct player numbers seat the humans and then all players.
- **Placement rule** (`readObjectPlacementRules`): a row lacking any score
  column, with a null field, or with an object type or terrain outside the
  binding tables binds nothing; scores stay indexed by table row.
- **Prototype** (`loadObjectPrototypes`): a prototype is skipped unless its
  footprint fits 8×6 with a footprint cell in the bottom row, and a monster's
  subtype is a creature type.
- **Families** (`generateToFile`, after construction; otherwise
  `RANDOM_MAP_GENERATION_FAILED`): monsters, `RANDOM_MONSTER`,
  `TERRAIN_HOLE`, shipyards, gates on two-level maps, one town per town type,
  monoliths (with exits for every one-way entrance), a dirt-selectable border
  guard for each tent colour, and, when seer huts exist, a dirt-selectable
  random artifact and a prototype for every quest-eligible artifact.

Internally produced values (Voronoi topology, fixed pattern tables, treasure
group padding, coordinates passed to `getMapItem`) take no input checks; the
entry validation bounds what they are built from.

| Operation | Prerequisite | Under `HOMM3_RMG_HOTFIX` |
| --- | --- | --- |
| `TRandomMapRequest::generateToFile`, map construction and lookup | Supported dimensions and levels, bounded player counts and enum values. Retail only clamps monster strength (after an overflowable `+ 3`) and repairs totals below two. | Request check. Team counts are clamped by `writeMapHeader`, so the lobby's out-of-range values stay accepted. |
| `getSerializedMapVersion` | Version 0–2; another value falls off the nonvoid helper. | Request check. |
| `getMapItem`, terrain cache, line and terrain painting | Coordinates in the map; `paintTransitions` needs at least two rows and columns. | Request check (dimensions ≥ 36); coordinates are internal. |
| `loadTemplates` | The short-row guard compares with the maximum-size column itself, so a row ending at the minimum size reads past it. | Existing short-row fix; template check. |
| `readRmgTemplateZones` | A row ending before the zone index reads it. | Existing short-row fix. |
| `readObjectPlacementRules` | Rows have every column and nonnull fields; type and terrain index local tables unchecked. | Placement-rule check. |
| `loadObjectPrototypes` | Required families exist; an empty monster list wraps `size() - 1`. Trait alias rows map to prototype-table indices. | Family check; the sort is skipped for an empty list, which then fails. Alias rows come from the built-in trait table. |
| `buildOutline` / `hasConnectedOutline` | A bottom-row footprint cell, so the outline is nonempty before `index % outline.size()`. | Prototype check. |
| `buildOverlapPriorities` / `scoreObjectPlacement` | Footprint fits 8×6; blocked predecessor cells are drawn; nonnull rule. | Prototype check for 8×6. Blocked but undrawn cells are not rejected (shipped `objects.txt` not verified); the only caller skips rule-less prototypes. |
| `positionZone` / `paintZoneTerrain` | A candidate survives filtering; zones exist. | Template check ensures zones. An empty candidate set depends on the layout and is **not covered**. |
| `initializeZones` | Nonzero normalization span. | Template check: positive sizes make each footprint, so the span, at least three cells. |
| `insetIslandZone` | Nonempty boundary before `m_boundary[0]`. | Internal (Voronoi boundary). |
| `buildZoneConnectionPaths` | A zone without a free same-zone tile reuses the last assigned seed; before any assignment retail reads stack residue. Retail stores `seed` only at `0x540731..0x54073f`; a 3,000-request probe found reused earlier-zone seeds in 53 requests and no read before the first assignment. The default source starts `seed` at `RMG_NO_POSITION` and skips such a zone, matching retail on those requests. | Existing fix: no other zone's seed is borrowed. |
| `connectZones` and connection graph consumers | Destinations exist, reverse connections exist, monolith families are nonempty. Appended extra-zone connections leave unread player limits uninitialized. | Retail already stores connections only between existing zones, both ways (connections to player-count-filtered zones are dropped, as shipped templates expect); family check. |
| `createTreasureObject` | A footprint cell to divide by; a positive representable density total. | Prototype check; definition densities are built-in. |
| `placeAdditionalTowns`, `placeExtraMines`, `placeZoneTreasures` | The enabled densities' product, their total and weighted counts stay representable (seven mine densities of 100 overflow). Disabled categories are not read. | Template check. Nonpositive densities still disable a category. |
| Creature/scroll factories | Creature levels fit the value table, nonzero AI value, valid indices, a selectable spell per scroll level. | **Not covered**: the traits are game data loaded outside the generator; the built-in roster satisfies them. |
| `connectJunctionEntrance`, `drawIrregularZoneBoundary`, `drawIslandBoundary` | A positive displacement limit before `rand() % limit`. | Template check (positive sizes). Roughness is the scaled size, which can still round to 0 (1 on island coasts), so `splitRmgBoundarySegment` raises a zero limit to one: one draw, no displacement. |
| `TRmgTreasureGroup` placement/guard routines | Objects keep the temporary map's padding; `addGuard` reuses the last object's prototype. | Internal. |
| `canPlaceTreasureGroup` | Callers establish the lower bounds. | Internal. |
| `createRoads` | At least one road target. | Existing fix. |
| `generate` player mapping | Slots cover the players, player indices 0–7, at most eight players, valid town-zone alignments. | Request check (≤ 8 players, valid towns) and template check (player numbers, distinct seats). |
| `assignRmgTeams` | Team and player counts fit the arrays; a positive number of nonempty teams. | `writeMapHeader` recounts players (≤ 8) and clamps teams to 1..players before calling. |
| `placeQuestArtifact` | Every eligible artifact has a prototype; nonempty seer family for the modulus. | Family check. Seer quests exist only for seer prototypes, so the modulus runs only when the family is nonempty. |
| `writeMap` | `RANDOM_MONSTER` and `TERRAIN_HOLE` entry zero. Only the final write result is checked. | Family check. Unlatched short writes are an output issue, not input. |
| `TRmgLinePatternTable` / terrain pattern tables and selectors | Fixed tables are well formed. | Internal fixed tables. |
| Voronoi geometry / vector operators | 32-bit differences, squares and cross products stay representable; triangles noncollinear. | Internal; sites are scaled map positions. The unscaled layout (`canPlaceZone`, `canConnect`, radial positions) has no upper bound on template sizes: **not covered**. |
| `TRmgVoronoi::removeEdge` | Both half-edges belong to the owner. | Internal topology. |
| `TRmgVoronoi::locate` | Sites inside the triangulation; valid rings. See [Voronoi provenance](rmg-voronoi-provenance.md). | Internal topology. |

Other preserved behavior defects, separate from undefined behavior:

- `TRmgZone::chooseCreatureTownType` uses the disjunction
  `choice != eTownNeutral || expanded || choice != TOWN_CONFLUX`; the two different
  inequality terms make it true for every choice. It therefore counts all four
  entries, including `eTownNeutral` and the zero padding (a Castle candidate),
  rather than stopping at `eTownNeutral` or excluding Conflux. Preserve the
  expression and its RNG behavior.
- On RoE maps `readRmgTemplateZones` clears `m_allowedMonsters[TOWN_CONFLUX]`.
  `createGuard` indexes that array by town type + 1 (neutral at 0), so this
  slot is Fortress; Conflux guards stay allowed. Preserve the index.
- `filterZonePositions` drops occupied-level candidates only when the last
  unused-level candidate's index is greater than zero, so a lone unused-level
  candidate at index zero leaves every candidate in place. Changing to a
  nonnegative test changes selection and subsequent RNG.
- `placeBorderGuard` returns zero if it cannot find the required guard
  prototype, although zero is also a successful color and other failure paths
  return minus one. Preserve the return value pending a deliberate bug-fix pass.
- `tryPlaceMine` uses the last scanned prototype's trigger and width for the
  entrance approach and resource area after selecting a random candidate. Retail stores it at
  `0x5459f5`/`0x545a5d` and reloads it at `0x545b7e`/`0x545ca9`. Substituting the
  selected candidate's prototype changes the existing behavior.
- `scoreObjectPlacement` overwrites each blocked footprint cell's mark with
  ADJACENT (retail store at `0x536d34`), discarding its OVERLAP and BLOCKED
  bits. Objects under an obstacle's blocked cells therefore score only as
  neighbours: rand_trn.txt's blocked scores, the draw-order conflict test and
  the rejection of rule-less objects never apply to them. Preserve the store.
- `createSubterraneanGate` shares one guard value between both entrances, and
  both guards are placed after both border guards: a border guard on either
  side leaves neither entrance guarded. Separate per-entrance values would change that.

## Reviewed invariants and coverage

The review covered `src/rmg.cpp`, `src/rmg_support.cpp`,
`src/rmg_terrain.cpp`, `include/rmg.h`, `include/rmg_terrain.h` and
`include/rmg_request.h`. It is a source audit with concrete leads, not a claim
of exhaustive dynamic coverage or proof that arbitrary custom assets are safe.

Several suspicious-looking expressions are supported by local invariants:

- `scoreObjectPlacement` reads `[column - 1][row - 1]` only under the overlap
  flag written at `[column + 1][row + 1]` for valid draw cells. The padded
  10×8 scratch array matches the 8×6 footprint. Its arithmetic is not an
  unconditional negative-index bug.
- `buildOverlapPriorities` checks `x > 0` before the preceding-column read,
  and increments/checks Y before accessing `y - 1`.
- `fillZoneArea` steps to `item - 1` only when X is positive, and to
  `item - width` only when Y is positive. These are offsets within the same
  contiguous map allocation for an in-bounds seed, not accesses before the
  array. Noise-mask row/column transposition is consistent between its
  producer and consumer; changing just one side would change the map.
- Terrain gap repair reaches its north/south or west/east reads only after the
  corresponding gap predicate proves an interior coordinate. Its gap array
  holds four runs, the maximum number of disjoint gaps in an eight-cell ring.
  The separated-neighbor predicate proves that the initial matching-bit search
  and subsequent gap selection have an entry; an arbitrary all-zero mask is
  not passed directly into that block.
- Terrain reflection and diagonal indices derive from selector-produced 0/1
  flips. Bit writers shift by `index % 8`, `bit % 8`, or cardinal direction
  0..3; these shifts have valid counts.
- `getSize(TRmgGridPoint())` uses the VC6 non-const-reference extension. The
  returned reference is copied within the same full expression, before the
  temporary dies; it is not stored as a dangling reference.
- `memcpy` of the object-traits name-row representation avoids a typed pointer
  aliasing read. Replacing it with a reinterpreted integer pointer would
  introduce a separate aliasing/alignment assumption.
- Map clear and packed-cache construction intentionally initialize selected
  bitfields only. Map clear preserves `m_borderConnection.m_guardColor`, unnamed
  bitfield remainders and `m_connectionVisited`, and sets only predecessor X to the invalid sentinel;
  later cost resets initialize full predecessors. Copying these aggregates
  includes indeterminate fields at initial construction, a C++ portability
  concern, but the review did not prove that every preserved field is consumed
  before initialization. Do not convert this into whole-object zeroing.
- Terrain painter finalization occurs during destruction while its adapter
  still exists: local map views precede adapters/painters, and reverse local
  destruction preserves their dependency order. Painter set iterations copy
  points before erasing/inserting, avoiding use of invalidated iterators.
