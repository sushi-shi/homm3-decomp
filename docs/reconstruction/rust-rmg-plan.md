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

### Current implementation checkpoint

Implemented the source-derived data boundary, typed requests, owned retail RNG,
template parsing/selection, player assignment, Delaunay/Voronoi geometry, line
patterns, initial zone layout, boundary rasterization, added water zones and
their connection graph, recentering and island terrain coverage. The
implementation uses `std`; template names borrow resource bytes, and layout,
subdivision, raster, polygon and graph workspaces retain their storage.

The installed `rmg.txt` passed 864 request/mode combinations. Retail retained
252 deferred-fault candidate instances; hotfix excluded them. A retail template
with an invalid player slot cannot be removed before selection without changing
seeded output. It stays as an explicit candidate fault until selected.

Eight VC6 stage checkpoints (four sizes in both modes) agree with Rust on the
template, player assignments, zone positions/sizes, terrain/faction choices,
clipped polygons, every boundary cell, connection adjacency order, graph
distances, recentered bounds and island coverage. Each random stage also agrees
on RNG state. There are 26 focused unit tests across the two crates. The
installed-data checks require explicit local assets and are ignored by default.

The boundary-to-coverage transition consumes its stage value, preventing a
second application of random island displacement. Retail modulo-zero faults
retain the preceding random draw; hotfix checks an empty placement list before
drawing and substitutes a singleton displacement range for zero roughness.

Fresh whole-map C++ runs also succeeded and repeated exactly for those eight
cases. Before any Rust generation, C++ retail mode already differs from the
pinned executable for seed 100 / 108x108 / RoE / islands and seed 17 / 144x144 /
two levels / SoD / random water (zero stack/heap replay fills). Keep these
disagreements visible during porting; they are not evidence of Rust parity.

Terrain painting, native trait/prototype loading, placement, serialization, CLI,
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
