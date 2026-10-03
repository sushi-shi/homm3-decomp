# RMG cleanup review

This review covers all function bodies, declarations and fixed tables in
`src/rmg.cpp`, `src/rmg_support.cpp`, `src/rmg_terrain.cpp`, `include/rmg.h`,
`include/rmg_terrain.h` and `include/rmg_request.h`. The initial reference
revision was `22315fafa`; the published cleanup is isolated on the helper
recovery branch at `45032fe50`, excluding unrelated local recovery commits.
After the initial review, seven parallel reviewers continued through four
more rounds, rotating source regions and independently auditing safety.
The fifth round found no remaining substantive cleanup under its original
scope. A subsequent helper-focused campaign broadened the review to repeated
semantic operations, including composite predicates, bounded geometry,
serialization and coordinated state updates. Integration covered shared interfaces, source ownership, matching
experiment references and the oracle's typed request entry. Generated-output
comparison ran only at the end of each completed cleanup campaign, never
between these additional review rounds.

The compatibility requirement is unchanged generated maps, return values,
request mutations, RNG state and x87 state. Retail bugs remain intentional
compatibility behavior. The [safety inventory](../reference/rmg-undefined-behavior.md)
separates instruction-established defects from conditional source hazards and
records prerequisites for a later bug-fix effort. Neither cleaner C++ nor a
sampled execution comparison proves absence of undefined behavior.

## Coverage and changes

The helper-focused campaign requires each new abstraction to be self-contained,
combine multiple actions or checks, occur in at least two genuine source sites,
and make its callers clearer. A descriptive semantic name supports that decision;
textual similarity alone does not. The final review rounds read complete
algorithms and ask whether high-level operations still contain repeated low-level
math or state manipulation. Workers rotate regions and independently review
preservation risks. Existing evidenced native helpers remain canonical.

The expanded sweep shares transition/flip predicates, directional terrain
patterns, coordinate lookup, clipped neighborhood bounds, guarded clearance
updates, object footprints, reward serialization, descending worklist search,
density initialization and complete treasure-placement retries. Small wrappers
that merely relocate a single action are rejected; the former single-caller
`buildTerrainGaps` cleanup wrapper was folded back into its owning algorithm.
Byte-similarity percentage drops are acceptable for this campaign. Generated
behavior is the acceptance criterion, with the million-case validation deferred
until the review and source changes are finished.

Seven reviewers completed an extraction round followed by nine semantic rounds,
rotating ownership across all six files and independently reviewing preservation.
The final two rounds reconsidered complete operations and previous dispositions,
including arithmetic hidden inside higher-level placement and painting code.
The ninth round found no further worthwhile extraction or missing canonical
helper call at executable-source checkpoint `7d1d07f2f`. This is a review stopping
point, not proof that every possible improvement has been discovered.

The subsequent cleanup removes stale matching percentages, failed-probe history,
placeholder commentary and explanations that merely repeat the code. Native
evidence, address annotations, retail quirks and non-obvious ordering, arithmetic,
aliasing and lifetime constraints remain. A lexical comparison against
`a9e013a71` verified unchanged executable tokens in all six RMG files before
the separate boolean-type cleanup.

All 166 one-off experiment scripts were retired at the user's request. Four
reusable audit/comparison tools moved from `scripts/experiments` to
`scripts/tools`, and documentation references were updated. Each retained
entry point accepts `--help` successfully. The RMG output-validation tooling in
`scripts/homm3/rmg` and the generic source-family runner remain available;
Git history preserves the retired scripts.

Logical byte and integer declarations now use `b8` and `b32`, respectively,
with `true`/`false` for logical constants. These are project aliases for
`unsigned char` and `int`, preserving storage, mangling and noncanonical values;
they do not assert original source spellings or native `bool` contracts.
Retail `completePlacement` implementations return through AL, so that interface stays
byte-sized. The line-painter virtual blocking result remains 32-bit, and its
tile wrapper retains the original low-byte conversion. Existing native `bool`,
bitfield base types, counts, masks, noise, IDs and unresolved option bytes stay
unchanged. Token review limited this edit to aliases, includes and logical
literals; generated-output validation remains pending.

The final source shares complete policies for placement retries, monolith
lifecycle, ordered worklists, player limits, subtype lookup, footprint bounds,
border clearance, outline traversal and terrain repair. Captured trigger values
can use the same translation helper as live prototype fields. Existing native
point and half-edge helpers remain nested in the shared operations. Cleanup-only
helpers reduced to one source call by a larger extraction were folded into that
operation; native-supported helper boundaries were retained. New helper names
and signatures are admitted in `config/source/win_only_modules.tsv` as cleanup
choices, not recovered original symbols.

Remaining explicit code has specific reasons: different worklist mutation order,
live versus cached terrain reads, native long-coordinate selectors, callback
snapshot lifetimes, partial-width integer-backed serialization, differing cache
validity timing and reflection priority. The review rejected wrappers that need
many policy parameters or merely package obvious individual stores. No retail
bug was repaired and no generated-output comparison ran between rounds.

| Area | Review outcome |
| --- | --- |
| Map storage, adapters and packed fields | Reviewed ownership, view lifetime, flattening and partial initialization. Named template-zone records, half-edges, owned-object lists, and the river painter/adapter and corrected the inverted line-painter blockage query. Scalar accessors now distinguish terrain/frame from line type/underlying terrain. Kept layouts, field widths, virtual ordering and snapshot boundaries. |
| Template and object-prototype loading | Named town count/density slots and mine arrays from their consumers. Replaced understood object/version/resource literals with existing constants. Recorded malformed-row and missing-family assumptions without adding validation. |
| Generated objects and serialization | Named creature reward count, reservation release and request failure causes; clarified writer parameters and format constants. Retained serialization widths, write order, default payloads and ignored-write-result behavior. |
| Factory selection | Shared the spell-scroll eligibility predicate across counting and selection. Preserved ascending scans, RNG draws, modulo selection and trait dependencies. |
| Zone placement, bounds, noise and connections | Named scanline seeds, major/minor line steps, displacement quadrants, clearance flags and connection choices. Reviewed the existing canonical lookup, position, flood and boundary helpers. |
| Towns, mines and treasures | Shared first-minimum weighted-category selection across three schedulers. Named weighted counters, increments and last-scanned mine prototype. Retained strict tie ordering and finished-category short circuit. |
| Roads, rivers and quests | Identified worklist algorithms, reviewed predecessor traversal and stable rankings, retained random draw order and known failed-search behavior. |
| Terrain and line painting | Shared neighbour-mask queries, frame-range draws, boundary counting, neighbour classification and axis-gap repair. Reused the existing base-tile operation. Corrected transition/frame and special-frame probability names. |
| Voronoi support | Clarified circumcircle and squared-distance names; documented incremental Delaunay insertion, local flips and walking location. Retained integer arithmetic and half-edge ownership contracts. |
| Safety | Reviewed index guards, pointer offsets, uninitialized fields, arithmetic, shifts, temporary references, destruction and fixed tables. Added concrete native evidence and conditional leads to the safety inventory. |

The subsequent rounds reused the existing scalar writer at 95 more call sites,
shared failed treasure-group cleanup, candidate bounds, noise quadrants,
boundary-ring lookup and border marking, and consolidated the identical
monolith road-cost loops. Terrain helpers now also share guarded border
refreshes, diagonal queue admission and special-frame queries. Pattern IDs
have topology names; count and selection share the quest-artifact predicate.
The request's progress pointer is typed through the game and oracle callers.

Path-clearance bit 27 previously had a misleading subterranean-gate name.
Its name now describes its generation role, and common open/border updates
preserve their connection guard and opposite store orders. Four key-color
rescans share one helper; the first-use uninitialized color remains untouched.
The independent safety pass also established a removed-wrapper leak in failed
quest/key-tent replacement and documented density-product overflow contracts.

The new source-local inline helpers are cleanup abstractions, not claims that
their original spellings or exact declarations survived. Existing ordinary
helpers stay ordinary; no inlining pragmas or duplicated game declarations are
introduced. Painter wrappers with different native entries retain their own
copy/snapshot boundaries rather than being mechanically merged.

A reader-trap round named RMG-internal sentinels (no zone, unset position,
no category, no frame, failed border placement), literal arguments (3x3/5x5
neighbourhoods, border-entrance and path-clearance policies, border guard
counts, neighbour steps, spacing), the H3M tile-flag bits and the unit-bearing
constants: density areas are squared entrance distances (2 per cardinal
step), so 82944 = 4 x 144 x 144, 800 and 1600 are 4 x 200 and 4 x 400 tiles.
Header fields document their ranges and writers. Enumerators replace literals
only where VC6 emits identical bytes; `buildRoadCostMap`'s enum-typed step
cost conditional once needed `s32` casts for that, but now compiles
identically without them and no longer has them.
Function-local static tables keep the repository's unprefixed convention, with
`<table>Guard` labels.

## Algorithm names and preservation constraints

| Source operation | Algorithm and details that affect output |
| --- | --- |
| `TRmgVoronoi::addSite`, `locate`, `buildVertices` | Incremental Delaunay triangulation, walking point location and integer circumcenters for the Voronoi dual. Local edge-flip order and zero-determinant decisions matter. The [provenance study](../reference/rmg-voronoi-provenance.md) distinguishes algorithm identification from original-author attribution. |
| Ordered map-cost worklists | Descending `lower_bound` insertion with removal from the back implements minimum-cost relaxation. Preserve tie placement and the different zone, water and road cost functions. |
| Zone graph distances | Dijkstra-style ordered relaxation or stack-based label correction, according to the actual worklist; do not call every flood a breadth-first search. |
| `createRiver` | Randomized best-first relaxation. Costs draw randomness during visits, so treating this as a fixed-weight graph and reordering the worklist changes output. |
| Straight boundaries, branch rays and line walker | Bresenham-style integer error accumulation. The line walker also paints the corner cell on each minor-axis step to keep lines four-connected, and walks destination-to-start. |
| Irregular boundaries, branching paths and noise regions | Random midpoint displacement. Preserve integer truncation, endpoint order, random draw order and the distinct LIFO subdivision/FIFO branch worklists. |
| `fillZoneArea` and mask floods | Scanline flood fill or depth-first worklist traversal. Guarded backward pointer movement stays within the flattened map array. |
| Town/mine/treasure density placement | Stride scheduling by minimum weighted count; increments are the integer density product divided by each density. First-on-tie selection is stable. |
| Weighted factory selection | Roulette-wheel/cumulative-weight selection using retail modulo draws. No unbiased-RNG substitution. |
| Prototype ordering and quest ranking | Exchange sort and stable insertion ranking respectively; preserve their comparison and equal-key behavior. |
| Object outline tracing | Cardinal wall following around a footprint. Direction-update and stopping rules are significant. |
| Terrain transition selection | Priority-ordered pattern classification under four reflections. Pattern-family priority precedes reflection priority. |
| Terrain gap repair | Cyclic run-length encoding followed by greedy first-minimum weighted-gap filling. Cardinal cells weigh two and diagonal cells one. |

## Control flow and indexing decisions

All three terrain gotos were removed using structured gap loops and early
exits from the separated-neighbour predicate. A later control-flow pass
replaced `canFitObject`'s two jumps into the placement-failure return with
early returns, so the RMG sources contain no gotos. The same pass rewrote
matching-shaped loops (`while (1)` binary searches, a `do`/post-decrement seat
scan, an attempt loop that cleared its result to signal exhaustion, an
empty-bodied reverse-iterator search) and removed scope blocks or hoisted
declarations that only shaped stack reuse. Rewrites in the constructor and
`initializeZones` call trees were kept only where VC6 emits identical code,
because `buildZoneBoundaries` reads their stack residue. A second review
merged nested selection tests, made `createGuard`'s skipped RoE creature
explicit, returned directly from Voronoi point location and declared
boundary-tracing, fill-seed and fit-test locals where they are used.
A third review dropped the river adapter's paired scope blocks and gave
Delaunay legalization its if/else-if/else form; both compile identically.
The deliberate keeps are listed under frame-sensitive code below.

### Frame-sensitive code

Retail `buildZoneBoundaries` reads its `testSlot` town flags uninitialized
(see the [safety inventory](../reference/rmg-undefined-behavior.md)), so those
bytes are whatever earlier calls left on the stack. Source changes that alter
VC6's frames or inlining in the generator constructor tree, `initializeZones`
or `buildZoneBoundaries` change generated maps. Source comments mark the
following keeps only with a pointer here:

- `initializeObjectGenerators`' key-tent scope block: removing it changes
  VC6's `push_back` inlining in that function and the constructor tree's
  stack residue.
- `initializeZones` reuses `minimumY`/`minimumX` as the square's origin.
  Separate origin locals changed its frame and moved towns on maps with water
  or two levels (caught by the output comparison below).
- `assignRmgZoneCell` and `clampRmgBoundaryToMap` stay free functions: as a
  `TRmgMapItem` or `type_random_map` member respectively, VC6 lays out
  `drawIrregularZoneBoundary`, called under `buildZoneBoundaries`, differently.
- A fifth pass removed temporaries that only reordered loads (the three
  adapter `setTile` bodies, reward factories, quest scores, group placement
  bounds, branch-ray steps) and result variables. It kept the staged locals in
  `getConnectionGuardValue`, `createGroundConnection` and `tryPlaceMine`, the
  `getNeighborhoodBounds` and `getLevelPosition` bodies, `TRmgVector::operator*`
  and `TRmgMapItem::clear`'s copies. Rewriting them changes the frames or stack
  writes of code that runs during map construction or before
  `buildZoneConnectionPaths`, whose fallback seed reads stack residue.
- A sixth pass removed the remaining staging that compiles identically
  (`isPlacementBlocked`'s mask point, `recenterZone`'s position, the creature
  reward value's zone counts). For the same reason it kept the
  reference-bound `y` in `getRmgRadialZonePosition`, `positionZone`'s origin
  stores and staged selection, the declared-then-assigned positions in
  `getInitialZoneBounds`, `filterZonePositions`, `fillZoneArea`,
  `placeWaterZoneIslands`, `createSubterraneanGate` and `placeObjectInZone`,
  `type_random_map::addObject`'s mask point and `floodConnectionCosts`'
  neighbour position: each rewrite changed VC6's code for these functions.
- A seventh pass passed the line-walk axis endpoints, the creature reward
  count and the prison object id by value, initialized the prison object and
  map request fields in declaration order, and dropped staged copies in
  `canPlaceTreasureGroup`, `markRiverCoastTarget` and `markRiverTargets`.
  Apart from the request constructor, which only reorders two field stores,
  that code runs after the connection paths are built. It kept
  `rmgTerrainPainter::getPaintTerrain` returning a reference: by value,
  `repairTerrainPoint`, which paints zone terrain earlier, compiled differently.
- An eighth pass initialized the guard, town, Pandora's box and seer hut
  objects in declaration order. Guards and towns are created before the
  connection paths, but `createGuard` and `placeTownAtRandomCandidate` only
  reorder equal-length heap stores; their frames, stack writes and call
  addresses are unchanged.
- A ninth pass constructed the guard-outline position in
  `TRmgTreasureGroup::addGuard`, the group centre in `placeTreasureGroup`
  and the outline start in `buildOutline` directly, and dropped
  `refreshRmgLinePoint`'s staged frame. `buildOutline` runs before the
  connection paths, but only swaps two adjacent zero stores to the same
  slots; its frame, function size and stack contents are unchanged.
- A tenth pass passed treasure-group positions, the chosen group position
  and the quest artifact's replacement value directly, centred group objects
  with a point constructor and let `createRiver` use its target position
  instead of a copy. All of this runs after the connection paths are built;
  only `placeQuestArtifact`'s frame changed, through VC6 inlining
  `TRmgTreasureGroup::addObject` there.
- An eleventh pass dropped the staged roughness in `traceZoneBoundary`,
  built `recenterZone`'s coordinate total with its constructor and dropped
  `canPlaceShipyard`'s staged terrain, all compiling identically, and
  returned `hasConnectedOutline`'s result as one expression, which only
  reorders its epilogue. Later code (`removeObject`, `canPlaceTreasureGroup`,
  `decorateUnderground`, `markRiverTargets`, `writeMap`) now loops over plain
  coordinates or an item count instead of position structs used only as
  counters. It kept `TRmgZone::canConnect`'s staged minimum,
  `TRmgHalfEdge::setPosition`'s copy, `matchesTerrainAt`'s setter calls and
  `openConnectionPath`'s staged predecessor: rewriting them changed the
  `initializeZones` or `buildZoneBoundaries` code, terrain painting before the
  connection paths, or `openConnectionPath`'s frame.
- A twelfth pass replaced the last counter-only loops in post-path code:
  `canPlaceTreasureGroup`'s group-bounds scan uses plain coordinates, and
  `decorateMap` walks the cell array directly when counting border cells and
  opening paths. Only those two frames changed.
- A thirteenth pass let `decorateMapCell` iterate the blocked footprint
  cells instead of a staged bounds struct, chose `placeZoneTreasures`'
  density area once, and dropped staging in the line-pattern table
  constructor and in `isRmgPointRightOfEdge`, the pattern and table rules'
  `selectBaseFrame` result variables and the table rule's staged range; all
  but the first two compile identically or run only at static
  initialization. It kept the staged positions in `isRmgEdgeOrigin`,
  `isRmgEdgeDestination` and `isRmgPointOnSegment`, the half-edge
  constructors' body stores, `getTerrainNeighbourKindAt`'s staged terrain,
  both `selectTransitionFrame` result variables, the setter-driven loops in
  `paintRectangle` and `paintTransitions`, `type_object`'s constructor
  stores, `carveBranchingPaths`' field-wise seed points, the counter
  positions in `traceBranchEnd` and `createWaterZoneIsland` and
  `generateRmgIslandMask`'s field-wise midpoints: each rewrite changed
  Voronoi, terrain-painting or other pre-path code.
- A fourteenth pass gave `buildOverlapPriorities` an ordinary row loop
  instead of a mid-loop break, dropped the line painters' staged `getTile`
  copy and let `createRivers` seed the second river at the entrance offset
  instead of shifting a reused position. All of it runs after the connection
  paths. It kept `makeTerrainFlip` in `selectTerrainTransition`'s body:
  constructing the flips directly reorders that terrain-painting code.
- A fifteenth pass passed the line painters' tile to the adapter instead of
  rebuilding it field by field, seeded `createRiver`'s north and north-east
  cells at offsets from the source instead of shifting the parameter, and
  dropped `TRmgTreasureGroup::tryAddObject`'s staged candidate count. All of
  it runs after the connection paths; the last two frames changed.

`writeRmgReservedBytes` takes its count as a function argument for a separate
reason: VC6 merges function templates whose parameters do not mention every
template argument, so a count-only template parameter wrote the same size for
every call.

Subtraction in an index is not automatically an out-of-bounds access. The
placement scratch array includes a one-cell border, its overlap flag proves
positive indices, and scanline predecessor reads have coordinate guards.
Those invariants are documented next to the code and in the safety inventory.
The actual negative-zone guard lookup and failed predecessor-chain bugs are
preserved and documented separately.

Opaque fields and unobserved bits keep their offset-based names when no read
establishes their role. Renaming them to invented gameplay concepts would make
the reconstruction less accurate. Existing source-attested type spellings and
external ABI names are also retained. Role-derived renames keep former names
in evidence comments where useful for lookup.

The earlier campaign's final pass rejected further generic factories, tiny one-use predicates and
mechanical merging of different snapshot, traversal or placement policies.
`isPassableLand` retains its established name and exact road-passable/non-rock
predicate; it admits water, whose policy is checked separately by callers.
The no-progress result is a review stopping point, not proof that every defect
or possible improvement has been discovered.

## Validation status

The generated-output comparison of the final cleanup (2026-10-03) uses the
standard oracle with deterministic random requests (`million-v1`: all sizes,
levels, formats, water and monster settings, player/team splits, town choices,
seeds, stack words and heap fill bytes). It first caught two cleanup defects,
both fixed before the comparison was restarted:

- `writeRmgReservedBytes<N>` took its size only as a template argument; VC6
  merged every instantiation, so plain object records gained 26 bytes.
- Splitting `initializeZones`' square origin into new locals changed the stack
  residue that `buildZoneBoundaries`' uninitialized `testSlot` town flags read,
  moving towns on maps with water or two levels.

After the fixes, 88,923 single-process cases agreed byte for byte (map, return
code, request, RNG and x87 state) except for retail faults reproduced at the same
documented sites on both sides (river `getLineType+0x1f`, guard
`createGuard+0x14`). The comparison continues with batched driver processes,
which were checked identical to fresh processes on 640 sampled cases per side;
it stops at the first mismatch that is not such an agreed fault. This is
sampled agreement, not a proof over every input.

After the review converged at `7d1d07f2f`, VC6 SP3 compiled `rmg`,
`rmg_support`, `rmg_terrain` and `singleselectionwindow`. The same targeted
`homm3 build --fast` passed again after the boolean cleanup at `48df064ea`,
with exit status 0. The ignored logs are
`build/rmg-review/helper-sweep-build.log` and
`build/rmg-review/cleanup-bool-build.log`. Reported similarity drops were
retained, as intended for this readability sweep; these results do not establish
generated-output identity. The current Mac report has 95 scored pairs, 30 exact,
268 emitted pairs unavailable because of unresolved references, and 40 Windows
claims without a full-TU body. No full-build preservation gate or global ledger
checkpoint was run. The older score table below belongs only to the previous
campaign.

### Earlier source checkpoint

The earlier executable-source revision is `7d422bcc8` (2026-10-02), published
with evidence updates at `370c71dee`. VC6 SP3 compiled
`rmg`, `rmg_support`, `rmg_terrain` and the result-code consumer
`singleselectionwindow`. Renamed symbols were refreshed with unit-scoped
`homm3 delink`; their targeted `homm3 build --fast` completed successfully.

After all source edits, the execution oracle compared 600 cases, each twice
in retail and twice in the candidate: **2,400 fresh processes**. The set contains
480 sampled requests covering the full product of four sizes, two level counts,
three formats, four water settings and five monster settings; 118 existing
acceptance requests; and the two documented crash reproductions. Player counts,
town arrangements, 484 distinct stack fills and 222 heap fills also vary.

- **597 cases:** byte-identical uncompressed maps, return codes, post-call
  requests, final RNG state and x87 state; both implementations repeat exactly.
- **Three cases:** two river faults and one negative-zone guard fault, each
  repeated in both implementations. No candidate-only failure, retail-only
  failure, normal-output mismatch or repeatability disagreement occurred.
- The river fault is retail `0x53284f` and candidate `0x3000337f`: offset `+0x1f`
  in `TRmgRiverMapAdapter::getLineType` on both sides. The guard fault is retail
  `0x540b34` and candidate `0x30011a34`: offset `+0x14` in `createGuard`.
  The linker map establishes the candidate function starts. Crashes remain
  execution errors in the raw report; they are not counted as passing maps.

The ignored artifacts are `build/rmg-review/final-reviewed/`: full `cases.json`,
`report.json`, per-process raw files/logs, frozen link inputs, `driver.map` and
`provenance.json` with asset/tool/object hashes. The disposable bounded-parallel
runner in `build/rmg-review/compare_reviewed.py` uses the standard oracle's
preparation, execution and comparison functions. The campaign returned 2
because its three preserved crash cases are explicitly classified as errors.
This is sampled behavioral agreement, not a proof over every possible input.

### Machine-code comparison remains a separate measure

That checkpoint preserves the sampled generated output; it does not preserve
every former instruction sequence. The following bodies show representative
Windows score changes at that checkpoint (previous banked CUR versus measured):

| Body | Previous checkpoint | Earlier measured comparison |
| --- | ---: | ---: |
| `TRmgTreasureGroup::addGuard` | 86.54% | 83.09% |
| `filterZonePositions` | 98.87% | 85.42% |
| `joinExtraZones` | 76.28% | 69.73% |
| `subdivideRmgNoiseRegion` | 100.00% | 89.04% |
| `createSubterraneanGate` | 85.81% | 80.44% |
| `carveBranchingPaths` | 96.10% | 94.16% |
| `tryPlaceMine` | 95.06% | 93.27% |
| `buildRoadCostMap` | 90.92% | 42.07% |
| `writeMapHeader` | 96.34% | 94.09% |
| `writeRmgObjectPrototype` | 100.00% | 91.43% |
| `placeQuestArtifact` | 76.68% | 73.44% |
| `refreshRmgLinePoint` | 99.75% | 84.81% |
| `TRmgLineWalker::paintPoint` | 100.00% | 78.78% |
| `rmgTerrainPainter::paintPoint` | 100.00% | 85.16% |
| `rmgTerrainPainter::queueOtherTerrainNeighbours` | 100.00% | 75.19% |
| `rmgTerrainPainter::getSpecialFrameStrength` | 100.00% | 73.81% |
| `rmgTerrainPainter::repairTerrainPoint` | 99.18% | 88.44% |
| `rmgTerrainPainter::paintTransitions` | 100.00% | 78.10% |
| `rmgTerrainPainter::hasSeparatedNeighbours` | 100.00% | 99.75% |
| `rmgTerrainPainter::buildNeighbourKinds` | 100.00% | 69.42% |

Other bodies improve: `assembleTreasureGroup` reaches 88.23%,
`commitTreasureGroup` 93.30%, and `tryPlaceAdditionalTown` 83.28%. The weighted
treasure scheduler reaches 69.33% (banked CUR 65.74%, prior MAX 69.76%). Other
reported MAX resets can expose an already lower CUR when a function is renamed; those
are distinct from newly changed machine code. These are targeted comparisons;
this review does not bank a repository-wide scoring checkpoint from unrelated
copied objects.

The targeted CodeWarrior report scores 97 pairs with 31 exact; 266 emitted
pairs remain unresolved and 40 Windows claims have no full-TU Mac body.
Compared with the initial cleanup checkpoint, 15 RMG serializer pairs that
previously scored are now unavailable: CodeWarrior retains calls to the shared
`writeValue` template instantiations, whose Mac targets are unpaired. This is
an explicit loss of comparison coverage, not a zero score or a passing gate.
No speculative Mac target addresses or linking manifests are added to conceal
it. `assembleTreasureGroup` moves from 29.6117% to 27.7523% after adopting the
canonical failed-group cleanup; its `MAC_ADDRESS` line records the one-checkpoint
abstraction reason against the preceding ledger hash and score. The ancillary
selection-window `CChatWidget::draw` pair remains unavailable because
`.getPitch__11Bitmap16BitCFv` cannot be resolved.

The full-build Mac preservation gate has not been passed: the lost serializer
comparisons remain a limitation of this draft PR. The Windows execution oracle
is the generated-output check; it does not establish unchanged instruction
bytes or complete Mac comparison coverage. The targeted results also retain
pre-existing Clang parsing gaps and use the label tool's declared fallback.
