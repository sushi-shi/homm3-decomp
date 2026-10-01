# RMG cleanup review

This review covers all function bodies, declarations and fixed tables in
`src/rmg.cpp`, `src/rmg_support.cpp`, `src/rmg_terrain.cpp`, `include/rmg.h`,
`include/rmg_terrain.h` and `include/rmg_request.h`. The initial reference
revision was `22315fafa`; the published cleanup is isolated on the helper
recovery branch at `45032fe50`, excluding unrelated local recovery commits.
After the initial review, seven parallel reviewers continued through four
more rounds, rotating source regions and independently auditing safety.
The fifth round found no remaining substantive cleanup under the preservation
constraint. Integration covered shared interfaces, source ownership, matching
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

| Area | Review outcome |
| --- | --- |
| Map storage, adapters and packed fields | Reviewed ownership, view lifetime, flattening and partial initialization. Named template-zone records, half-edges, owned-object lists, and the river painter/adapter and corrected the inverted line-painter blockage query. Scalar accessors now distinguish terrain/frame from line type/underlying terrain. Kept layouts, field widths, virtual ordering and snapshot boundaries. |
| Template and object-prototype loading | Named town count/density slots and mine arrays from their consumers. Replaced understood object/version/resource literals with existing constants. Recorded malformed-row and missing-family assumptions without adding validation. |
| Generated objects and serialization | Named creature reward count, reservation release and request failure causes; clarified writer parameters and format constants. Retained serialization widths, write order, default payloads and ignored-write-result behavior. |
| Factory selection | Shared the spell-scroll eligibility predicate across counting and selection. Preserved ascending scans, RNG draws, modulo selection and trait dependencies. |
| Zone placement, bounds, noise and connections | Named scanline seeds, major/minor line steps, displacement quadrants, clearance flags and connection choices. Reviewed the existing canonical lookup, position, flood and boundary helpers. |
| Towns, mines and treasures | Shared first-minimum weighted-category selection across three schedulers. Named weighted counters, increments and last-scanned mine prototype. Retained strict tie ordering and finished-category short circuit. |
| Roads, rivers and quests | Identified worklist algorithms, reviewed predecessor traversal and stable rankings, retained random draw order and known failed-search behavior. |
| Terrain and line painting | Shared neighbour-mask queries, frame-range draws, boundary counting, neighbour classification and cyclic gap construction. Reused the existing base-tile operation. Corrected transition/frame and special-frame probability names. |
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

## Algorithm names and preservation constraints

| Source operation | Algorithm and details that affect output |
| --- | --- |
| `TRmgVoronoi::addSite`, `locate`, `buildVertices` | Incremental Delaunay triangulation, walking point location and integer circumcenters for the Voronoi dual. Local edge-flip order and zero-determinant decisions matter. The [provenance study](../reference/rmg-voronoi-provenance.md) distinguishes algorithm identification from original-author attribution. |
| Ordered map-cost worklists | Descending `lower_bound` insertion with removal from the back implements minimum-cost relaxation. Preserve tie placement and the different zone, water and road cost functions. |
| Zone graph distances | Dijkstra-style ordered relaxation or stack-based label correction, according to the actual worklist; do not call every flood a breadth-first search. |
| `createRiver` | Randomized best-first relaxation. Costs draw randomness during visits, so treating this as a fixed-weight graph and reordering the worklist changes output. |
| Straight boundaries, branch rays and line walker | Bresenham-style integer error accumulation. The line walker also paints the cell before a minor-axis step to maintain four-connected lines, and walks destination-to-start. |
| Irregular boundaries, branching paths and noise regions | Random midpoint displacement. Preserve integer truncation, endpoint order, random draw order and the distinct LIFO subdivision/FIFO branch worklists. |
| `fillZoneArea` and mask floods | Scanline flood fill or depth-first worklist traversal. Guarded backward pointer movement stays within the flattened map array. |
| Town/mine/treasure density placement | Stride scheduling by minimum weighted count; increments are the integer density product divided by each density. First-on-tie selection is stable. |
| Weighted factory selection | Roulette-wheel/cumulative-weight selection using retail modulo draws. No unbiased-RNG substitution. |
| Prototype ordering and quest ranking | Exchange sort and stable insertion ranking respectively; preserve their comparison and equal-key behavior. |
| Object outline tracing | Cardinal wall following around a footprint. Direction-update and stopping rules are significant. |
| Terrain transition selection | Priority-ordered pattern classification under four reflections. Pattern-family priority precedes reflection priority. |
| Terrain gap repair | Cyclic run-length encoding followed by greedy first-minimum weighted-gap filling. Cardinal cells weigh two and diagonal cells one. |

## Control flow and indexing decisions

All three terrain gotos were removed using direct returns from a shared gap
helper and structured early exits from the separated-neighbour predicate.
Two `canFitObject` failure gotos remain: they converge on the shared placement-failure
return, and the owning comment records the existing byte-matching evidence
for that first-failure join. The remaining generator/support bodies have no
gotos. The review does not replace a short, evidenced join with duplicated
cleanup or an artificial state machine.

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

The final pass rejected further generic factories, tiny one-use predicates and
mechanical merging of different snapshot, traversal or placement policies.
`isPassableLand` retains its established name and exact road-passable/non-rock
predicate; it admits water, whose policy is checked separately by callers.
The no-progress result is a review stopping point, not proof that every defect
or possible improvement has been discovered.

## Validation

The final executable-source revision is `7d422bcc8` (2026-10-02); subsequent
changes only record review evidence and the intentional Mac abstraction
checkpoint. VC6 SP3 compiled
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

Cleanup preserves the sampled generated output; it does not preserve every
former instruction sequence. The following bodies show representative Windows score changes after the
shared-helper and structured-flow changes (previous banked CUR versus final):

| Body | Previous checkpoint | Final comparison |
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
| `rmgTerrainPainter::getTransitionStrength` | 100.00% | 73.81% |
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
