# RMG cleanup review

This review covers all function bodies, declarations and fixed tables in
`src/rmg.cpp`, `src/rmg_support.cpp`, `src/rmg_terrain.cpp`, `include/rmg.h`,
`include/rmg_terrain.h` and `include/rmg_request.h`. The reference revision is
`22315fafa`. Six parallel reviewers divided the generator into contiguous
function ranges, reviewed support and terrain separately, and independently
audited safety. The integration pass reviews shared interfaces and cross-file
uses. Generated-output comparison is deferred until the combined cleanup is
finished.

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
Two `canFitObject` failure gotos remain: they converge on the next candidate
position, and the owning comment records the existing byte-matching evidence
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

## Validation

Final compiler and generated-output results are recorded here after all
source changes and integration reviews are complete. Historical campaign
results in the oracle guide describe the pre-cleanup implementation only.
