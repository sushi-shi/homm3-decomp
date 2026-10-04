# Rust random-map generator

Implementation base: `decomp-complete-4.0`, commit
`c2e0f6ea6d9ba70dc37634b80d3a01727ac5fa02` (2026-10-04).
The C++ generator remains the reference; this work is a behavioral Rust port,
not machine-code matching.

## Deliverable and decisions

Deliver a complete native Rust library and CLI in the `tools` workspace.
Support runtime-selected retail and current `HOMM3_RMG_HOTFIX` behavior in one
build. Game/lobby integration is a later task. Normal generation must not
depend on Wine or the retail executable.

Use `std` normally. Avoid unnecessary allocation by borrowing, retaining
workspace capacity, and using flat storage where appropriate. There is no
`no_std` requirement, allocation-free mandate, or custom allocator project.
Measure allocation count, reallocations, and peak live bytes before adding
complexity. Use `std::io::Write` for streaming output.

## Architecture

- `homm3-rmg-data`: allowlisted bindgen definitions from the existing C++ and
  generated immutable table contents. Do not hand-copy numeric definitions.
- `homm3-rmg`: parsed domain types, assets, compatibility policy, owned RNG,
  reusable generation workspace, geometry, painters, placement and serialization.
- `homm3-rmg-cli`: installation resource loading, explicit mode and seed,
  compressed H3M output, replay and diagnostics.
- Reuse existing Rust archive/resource readers. Keep `homm3-map` independent
  as an output checker.

The public flow is resource bytes -> parsed catalog -> request prepared for
the selected behavior -> generation -> serialization. Catalogs are immutable
and shareable; each generation owns its RNG and mutable workspace. File IO,
clock-derived seeds, compression and thread scheduling belong to callers.

Parse into domain types once. Use enums for town/water choices, zone roles,
object payloads, treasure definitions and meaningful state transitions.
Use private newtypes for bounded values and distinct arena IDs; distinguish
world, map and footprint coordinates. Replace sentinels with explicit absence
or variants. Resolve references and check arithmetic prerequisites when
constructing owning structures. Keep catalog-dependent constructors private.
Parsing must not consume randomness or resolve random choices early.

Return structured input, generation, modeled-retail-fault, reservation and IO
errors. Retain effective requests and final RNG state in run diagnostics,
including failures, while preserving the timing of request repairs.

## Shared source data and documentation

Use bindgen for existing enum values, constants and necessary plain records;
exclude C++ methods, containers and ownership. Rust domain types refer to the
generated constants rather than repeating numeric values.

For tables which bindgen exposes only as external arrays, put canonical
initializers in data-only include fragments used by the C++ owner and a
build-time host exporter. Emit immutable Rust data, including exact float
bits, from those same initializers. Cover RMG patterns/directions/limits and
built-in traits used by generation; parse asset-derived traits natively.

Preserve all ASCII documentation in code. Shared-table diagrams stay beside
canonical data and algorithm diagrams accompany their Rust implementations.
Keep existing C++ documentation. Review each ported subsystem against the
source for diagrams and behavioral caveats.

## Compatibility contract

Inventory each current hotfix branch; source and executed observations take
precedence over stale prose. Require an explicit CLI behavior mode.

Retail mode reproduces defined outputs, including uncompressed bytes, RNG
draw order/state, request repairs, ordering/ties, and serialization. Preserve
defined quirks in both modes unless the current hotfix changes them. Use an
owned retail RNG with explicit wrapping arithmetic. Test numeric conversion,
overflow, square roots and radial calculations against VC6; isolate any
necessary compatibility arithmetic.

Represent observed memory-dependent inputs as an explicit retail replay
profile, with a documented default and full recording in replay reports.
Never read uninitialized Rust memory. Known retail crashes and unmodeled
undefined states may return typed faults; hotfix uses its specified
alternatives. Model observable leaked-reference effects explicitly without
leaking allocations. Keep disagreements between current C++ and retail visible.

## Implementation milestones

- [ ] Foundation: crates, source-derived definitions/data, typed requests,
  owned RNG, numeric checks, and comparison support for retail and C++ hotfix.
  Establish fresh successful hotfix baselines.
- [ ] Assets: native resource loading, trait initialization, templates,
  prototypes and placement rules. Preserve resource precedence, original row
  indices and mode-specific filtering.
- [ ] Generation: geometry/terrain, towns/connections/mines,
  treasures/decorations, and roads/rivers. Diagnose with stage and RNG
  checkpoints rather than only final output differences.
- [ ] Delivery: all version-specific serialization, CLI generation/replay,
  corpus comparisons, allocation measurements and documentation review.

These are intermediate review points, not alternative definitions of done.
Completion requires the full generator in both modes.

### Remaining work, in execution order

Nine substantial work packages remain; the first is partially implemented. They describe
implementation order, not equal amounts of work or a percentage estimate.
The library cannot yet generate a complete map.

1. **Object storage and map mutation.** Complete typed payload ownership.
   Neighbourhood path helpers, stable geometry IDs, ordered cell membership,
   registration/removal, global/zone type counts, entrance-distance propagation
   and individual path/obstacle/border transitions now use reusable storage.
   Tent availability follows the loaded prototype count; retail's initial cursor
   remains an explicit replay requirement until a native rescan initializes it.
2. **Zone connections.** Port connection preparation and pathfinding, junctions,
   gates/portals, border guards and key-tent reservations, including water-zone
   border repair and mode-specific failures.
3. **Mines.** Port fixed and density-driven mine placement and guards. Connect
   the existing guard selector to object creation, ownership and spatial state.
4. **Treasures.** Port treasure definitions/values, object payload ADTs, group
   assembly/scoring/placement, reservations and completion/replacement callbacks.
   Integrate the existing hero, artifact, spell and creature catalogs.
5. **Decoration.** Port underground decoration, coastal marking and general
   obstacle placement, preserving overlap priorities and original iteration.
6. **Roads and rivers.** Port route construction and painting around the existing
   line classifiers and shared pattern tables. Preserve all source diagrams.
7. **Generation API and compatibility integration.** Connect all stages and
   immutable asset ownership to one native entry point with reusable workspace,
   structured failures and RNG diagnostics. Audit every hotfix branch and
   reconcile the boundary/water-zone replay inputs with executable evidence.
8. **Output and CLI.** Implement all three formats' H3M serialization, streamed
   compressed output, native generation/replay commands and actionable errors.
   Normal execution must not require Wine or the retail executable.
9. **Final verification and publication.** Run the complete 100,000-case
    campaign; independently parse maps; verify repeated/run-order behavior;
    profile cold loading, warm generation and output allocations; audit ASCII
    documentation/shared definitions; finish deterministic parallelism guidance;
    run required Rust checks and publish the new PR.

Immediate next implementation: movement/cost propagation for
`buildZoneConnectionPaths`, then water-zone border repair and crossings. Water-zone
islands now consume `ConnectionBorders` into `WaterIslands`, retaining the map,
town payloads and reusable terrain workspace for subsequent stages. Town payloads,
primary entrances, road targets and the shared serialized counter already exist.
Complete shared world/treasure-group payload lifetimes before integrating
transient treasure objects. Reset object and placement workspaces together.
Entrance-distance flooding preserves current-cell costs and pruning against
distances retained after removal.

### Current implementation checkpoint

Implemented the source-derived data boundary, typed requests, owned retail RNG,
template parsing/selection, player assignment, Delaunay/Voronoi geometry, line
patterns, initial zone layout, boundary rasterization, added water zones and
their connection graph, recentering, island terrain coverage and terrain
painting (repair queues, reflected transitions and decorated frame selection). The
implementation uses `std`; template names borrow resource bytes, and layout,
subdivision, raster, polygon and graph workspaces retain their storage.

The installed `rmg.txt` passed 864 request/mode combinations. Retail retained
252 deferred-fault candidate instances; hotfix excluded them. A retail template
with an invalid player slot cannot be removed before selection without changing
seeded output. It stays as an explicit candidate fault until selected.

Eight VC6 stage checkpoints (four sizes in both modes) agree with Rust on the
template, player assignments, zone positions/sizes, terrain/faction choices,
clipped polygons, every boundary cell, connection adjacency order, graph
distances, recentered bounds, island coverage and every painted terrain/frame/flip.
Each random stage also agrees on RNG state. There are 54 focused unit tests across the core/data crates,
three installation-adapter tests and six LOD codec checks, including malformed-input corpora. The
installed-data checks require explicit local assets and are ignored by default.

The boundary-to-coverage transition consumes its stage value, preventing a
second application of random island displacement. Retail modulo-zero faults
retain the preceding random draw; hotfix checks an empty placement list before
drawing and substitutes a singleton displacement range for zero roughness.

Terrain painting consumes coverage and retains a mutable workspace borrow, with
a read-only tile view for callers. Its two ordered worklists are reusable bitsets, preserving the original Y/X set order
without allocating tree nodes. Neighbour checks replace the temporary edge-count
array because terrain does not change during transition selection. Repeated
brush tests verify unchanged tile/worklist storage and drained queues. This is
buffer-reuse evidence; full allocator profiling is still pending. Frame tables
and rule parameters are shared C++ initializers exported at build time, including
one shared land-frame table for all six land rules. Brush strength and valid
terrain frames have private, parsed domains.

Adventure-object identities and all five trait override tables now come from
shared source definitions. The native `rand_trn.txt` parser keeps original row
identities separate from compacted rule IDs, preserves last-match binding, and
stores neighbour matrices in one flat buffer. Short/invalid hotfix rows are
skipped without shifting their score columns; retail reports typed indexing
faults. Every score in all 109 installed placement rules and all 232 initialized
object-trait rows match the C++ loader in both modes.

Native prototype loading parses `objects.txt`, borrows ordinary image names,
interns mask metadata once per image, applies mode/version filtering, preserves
the monster exchange sort and binds placement rules. Family buckets occupy one
vector. Selection counts and visits borrowed candidates without allocating a
temporary list. The installed source has 1,326 rows and 1,305 distinct images.
Every prepared prototype, all four masks, trigger coordinates, terrain masks,
ordering and rule binding match C++ for all three formats in both modes:
retail retains 1,326/1,233/1,072 prototypes (SoD/AB/RoE), and hotfix retains
1,209/1,194/1,033. Unusable retail candidates remain explicit deferred faults;
hotfix filters them. An injected mask provider keeps IO and archive lookup out
of the core; the installed comparison uses Complete's base-before-expansion
sprite precedence.

Creature and spell catalogs now parse their native spreadsheet sections into
fixed arrays without heap allocation or localized-string copies. Shared C++
initializer fragments supply creature factions/tiers and spell flags; school
constants come through bindgen. All 150 creature rows and 81 spell/ability rows
agree with the executable's initialized tables in both modes. Zero AI value and
unused tiers are explicit optional values, and parsed creature IDs also govern
prototype eligibility. Custom signed comparison values remain intact; arithmetic
will be checked where generation uses them. Missing rows/columns and overflowing
integer prefixes are typed faults, including the creature loader's weak
179-row check followed by accesses through row 184. The owning creature/spell
C++ TUs compile with their configured VC6 profiles after sharing data.

Artifact metadata now parses into a fixed array with typed identity, class and
combination-recipe fields. All 144 class/exclusion/combination records agree with
the executable's initialized table. The parser rejects equipment masks for
which the native loader's unchecked slot-class search runs off its table.
Slot masks, column mappings, combination constructor arguments and disabled IDs
are shared with C++; no localized artifact strings or unused prices are copied.

Hero availability comes from shared canonical initializers, not hotraits.txt:
the native spreadsheet loader changes names and starting-stack counts, which
RMG does not use. All 156 playable availability records agree with native data.
The per-map prison pool uses fixed flags and borrowed descending scans; full
seed-one selection sequences through exhaustion and final RNG states match C++
in all three formats and both modes. Empty pools consume no draw and singleton
pools still consume one. Artifact/herodefs owning TUs compile with their VC6
profiles after extraction of the portable definitions.

The installation adapter now lives in `homm3-rmg-cli`. Its disk-backed LOD
reader uses the shared codec's directory-only view, retaining decoded indexes
while leaving archive payloads on disk. Text honors loose Data overrides before
base/expansion bitmaps; masks use base/expansion sprites only. File-name lookup
is ASCII case-insensitive and rejects ambiguous case collisions. Compressed
input, mask output and inflater state are reused across reads. Tests cover
resource precedence, corrupt/wrong-size streams, empty compressed members and
buffer reuse. This is not yet the complete allocator profile.

Loading the installed resources through this adapter reproduces every native
prepared-prototype checkpoint in all three formats and both modes. Prototype
admission consumes the catalog into a value carrying its mode, format and plane
count. Hotfix requires the native family/town/portal/guard/quest relationships;
retail retains deferred faults. The eventual generation entry point must keep
the constructor's random-water draw before this gate's failure, then connect
the asset catalogs to the remaining placement and output stages. The CLI crate
currently contains the IO adapter, not a runnable generator command.

Guard selection now uses a fixed prototype-index array and returns a typed
stack without allocating an object or candidate list. It preserves last-loaded
prototype selection, descending creature order, the missing-prototype counting
bug, RoE's unevaluated creature 117 and both count-variation draws. Across 3,456
C++ captures (three formats, both modes, twelve faction policies, twelve guard
values and four seeds), creature, original prototype row, quantity and final
RNG state agree. Targeted tests cover invalid creature indices, arithmetic and
zero-divisor faults and hotfix's absent-selection result at the correct RNG
position. The generation owner still needs to assign the object ID and place
the returned stack.

Prototype outlines and overlap priorities now run without heap allocation.
One reusable outline workspace holds the bounded cardinal walk; a fixed mask
stores priorities only for cells C++ initializes. Empty bottom rows remain an
empty outline, and invalid dimensions or a nonterminating walk are typed faults.
The direction enum and initializer are shared with C++, including the original
ASCII direction diagram; the outline diagram stays beside the Rust traversal.
All 7,067 dimension-valid prepared prototypes across three formats and both
modes match native outline order and drawn-cell priorities. The bounded walk
also passes all 65,536 four-by-four masks, including disconnected footprints.
Placement must still reject an empty outline before a connected-outline check;
no placement grid or object ownership is implemented by this helper milestone.

The placement stage now consumes painted terrain while borrowing the original
tile and coverage buffers. A reusable flat cell-state buffer supports footprint
blocking, circular outline connectivity and southern entrance-approach queries.
Each query uses typed policies and the existing fixed outline workspace, with
no heap allocation. A nonempty-outline type excludes modulo-by-zero internally;
empty source outlines become explicit faults only when that check is reached.
Across six generated maps, 270,048 footprint/fit pairs agree with source-derived
checkpoints: 263,348 complete-fit results were executed in C++, while 6,700
empty-outline faults were identified and skipped before the native division.
All 256 circular blocking patterns and focused entrance/path/water policies
also pass. These results cover fit queries before town placement; mutation
verification is described below.

Map mutation now owns no per-cell vectors: cell chains preserve insertion order
in one reusable membership arena, and removal recycles links. Geometry IDs carry
owner tags, survive moves and become invalid after arena reset. Insertion clips
footprints, opens triggers and preserves overlapping blocking flags; erasure
removes only the first duplicate, retains the object/anchor and refreshes the
first-object entrance kind. Retail's never-allocated versus emptied-vector
removal distinction is modeled explicitly. Path/obstacle/border transitions use
typed states and a source-derived colour width. Object payload lifetimes remain
pending; generator registration is described below.

Native comparison covers 10,476 snapshots (873 first-family prototypes across
six format/mode maps, twelve phases each), including overlaps, duplicates,
clipping, removal and border protection. The reviewed extreme-coordinate case
also compares the retained anchor after an entirely clipped insertion. A
read-only design review found no regressions; the behavior review's row-clipping
finding is fixed and independently rechecked. Targeted tests cover recycled
link storage, foreign/stale object IDs and mode-specific absent removal.

Generator registration now preserves global insertion order, duplicates, signed
global/zone counts and ordered entrance-distance flooding with a reused queue.
Removal retains native counter timing, triggerless entrance probes, footprint-only
distance clearing and border-guard color release. Never-placed objects preserve
the constructor's clipped-footprint semantics. Native flat entrance indexing
admits aliases inside the allocation while overflow and out-of-allocation access
become typed faults. One shared arena/catalog admission check covers direct
footprint mutation and generator operations, including entirely clipped inserts.

Across all three formats and both modes, 729 registration/removal snapshots
agree with C++ checkpoints, including 5,668,704 distance values plus all global
and zone counters, active object order and tent cursors. The existing 10,476
cell-mutation snapshots still pass. Both reviewers rechecked their fixes and
reported no remaining actionable findings. Regression checks include foreign
catalogs/arenas, never-placed guards, triggerless flat aliases, and typed failure
when a valid extreme alias overflows on its next flood step. All 65 core tests
and strict Clippy pass. These remain source-C++ comparisons, not a claim of
complete executable parity or completed generation.

The four neighbourhood helpers now preserve their distinct source policies:
centre-first 3x3 path opening, dry/passable/non-entrance obstacle-patch release,
unoccupied-cell clearance release at either source radius, and the southern
entrance approach. A row-major iterator clips without allocation. Shared radius
constants come from bindgen; flat centre/approach aliases and checked arithmetic
retain the registration access rules. Expanded native comparison covers 17,460
mutation snapshots (646,020 lines), including occupancy, protected borders,
terrain and clipping through these helpers. Both reviewers found no actionable
issues. All 66 core tests and strict Clippy pass.

Review fixes now bind prototype/rule handles to their owning catalogs. Foreign
and stale handles cannot silently select another catalog's row. Checked process
ownership tags allocate no heap storage and never affect generation order, RNG
or output. Completed layout also carries its exact template, request and water
choice into boundary generation, preventing mismatched plane counts or unrelated
zone slices from reaching terrain painting. Both fixes were independently
re-reviewed. All 61 core unit tests, eight layout-through-terrain checkpoints,
3,456 guard captures and 270,048 placement-fit checkpoints pass; strict Clippy
passes. These checks retain the C++-reference scope described above.

Town placement now covers primary, fixed additional and density-driven towns,
with typed owner/fort payloads, a serialized ID separate from geometry identity,
ordered road targets, primary entrances and the hotfix starting-town requirement.
`TownsPlaced` consumes the preceding stage to prevent repeating town placement.
The borrowed template/request and fixed player assignment now survive layout
through boundary/terrain placement; no unrelated preparation arguments enter the
new stage. Hotfix primary towns resolve neutral zone alignment while subsequent
additional attempts retain their source-cached alignment. Candidate and outline
storage is reused across all attempts. Weighted category state represents only
finished or initialized active entries, with checked source arithmetic.

All eight native town captures agree through 47 payloads and 129,600 map cells,
including global/zone counts, primary entrances, road targets and final RNG state
(130,048 checkpoint lines total). This spans all four sizes, all three formats,
both plane configurations and both behavior modes. All 68 core tests and strict
Clippy pass. Both reviewers report no remaining actionable issues; their shared
category/selection-helper cleanups are included. The native-capture scope remains
the C++ source baseline, and full map generation/output remains unfinished.

Branch carving and border preparation now preserve LIFO subdivision, FIFO
branches, native segment orientation and draw timing, ray stopping rules, the
first directed connection policy and sequential neighbourhood mutations. Paired
segments and reusable buffers avoid parallel vectors and per-branch allocation.
Shared checked geometry and centered random offsets also serve rasterization;
source seed diagrams remain in C++ and Rust. All eight native captures agree on
129,600 cell states, town payloads, counts, road targets and final RNG state
(130,048 checkpoint lines). Town checkpoints still pass after sharing their
renderer. Both reviewers found no actionable issues; all 68 core tests and
strict Clippy pass. Water-zone islands and subsequent connection stages remain.

Water-zone island placement now covers rectangle-wide distance initialization,
ordered chamfer flooding, repeated candidate selection, midpoint-displacement
noise and full-plane brush repairs. The terrain stage retains a mutable borrow
of its existing workspace, so later painting copies no map grid and reuses both
repair queues. Noise dimensions are admitted against the source radius cap;
a fixed 144-byte mask and reusable subdivision/candidate/flood storage replace
per-island allocations. Movement directions are distinct from terrain-pattern
directions and shared by object-distance flooding. Water-spacing metadata keeps
its native zone-zero/forward-direction meaning. Source diagrams remain in code.

All eight native island captures match 129,600 terrain/frame/reflection and
spacing-metadata records, all placement cells/towns/counts and final RNG state
(259,648 checkpoint lines). The test verifies tile storage remains unchanged
through repainting. Both reviewers report no remaining actionable issues; the
metadata naming/documentation correction is included. All 68 core tests and
strict Clippy pass. The six native registration/removal comparisons also pass
after sharing movement-direction helpers (11,862 checkpoint lines).
These comparisons remain against the current C++ source, not a claim of complete
retail-executable parity or finished map generation.

Fresh whole-map C++ runs also succeeded and repeated exactly for those eight
cases. Before any Rust generation, C++ retail mode already differs from the
pinned executable for seed 100 / 108x108 / RoE / islands and seed 17 / 144x144 /
two levels / SoD / random water (zero stack/heap replay fills). Keep these
disagreements visible during porting; they are not evidence of Rust parity.

Top-level asset/generation orchestration, placement, serialization, CLI,
allocation profiling and the full corpus campaign remain to be implemented and
verified. This checkpoint cannot generate a map.

The current boundary port follows the merged C++ policy of one unused retail
town draw for the boundary probe and each added water zone. Those fields were
uninitialized in the executable; uniform stack/heap prefill alone does not
describe all their live values. The explicit replay policy for these particular
fields still needs to be reconciled with captured executable observations. Other
water-zone creature residue is represented as faction, neutral, or no matching
faction, rather than an unchecked index.

## Verification

Compare bytes, outcomes, effective requests and RNG states against retail and
current C++. Cover all four map sizes, both level counts, all three formats,
water/monster settings, player/team/town combinations and replay profiles.
Include documented crashes, hotfix regressions, malformed resources, missing
prototype families, arithmetic limits and failing/short output writers.

Run focused cases while developing and the existing 100,000-case campaign
(or its reproducible equivalent) before completion. Classify undefined or
crashing cases separately. Independently parse generated maps with
`homm3-map`. Repeat cases and vary run order to expose leaked state. Preserve
workspace MSRV and run relevant tests, Clippy and formatting checks.

Profile cold asset loading separately from generation and output. Reuse
candidate/path/visitation buffers and workspace capacity; eliminate repeated
clones, per-item allocations and inner-loop temporary collections where
unnecessary. Repeated warmed cases should reuse their buffers. Allocation is
allowed when it serves a purpose; no arbitrary zero-allocation gate.

## Parallelism roadmap

Preserve serial output for identical inputs.

1. Parallelize independent maps using immutable shared catalogs and separate
   RNG/workspace per worker. Bound worker count to bound memory.
2. Parallelize RNG-free preprocessing and independent read-only calculations.
3. Consider intra-map work through immutable snapshots, independent proposals
   and deterministic serial commits. Verify identical outputs across worker
   counts and schedules before enabling any parallel stage.

Do not introduce per-zone RNG streams, concurrent mutable generation state or
reordered draws. The serial implementation remains the reference.
