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

- [x] Foundation implementation: crates, source-derived definitions/data, typed
  requests, owned RNG, numeric helpers, and retail/C++ comparison support.
- [x] Assets implementation: native resource loading, trait initialization, templates,
  prototypes and placement rules. Preserve resource precedence, original row
  indices and mode-specific filtering.
- [x] Generation implementation: geometry/terrain, towns/connections/mines,
  treasures/decorations, and roads/rivers. Diagnose with stage and RNG
  checkpoints rather than only final output differences.
- [x] Output implementation: all version-specific serialization and CLI
  generation/replay.
- [ ] Verification and delivery: establish successful hotfix baselines, debug
  end-to-end behavior, complete corpus comparisons and allocation measurements,
  finish review, and make the implementation PR ready for merge.

Implementation checkmarks record authored code, not runtime parity. These are
intermediate review points, not alternative definitions of done. Completion
requires the full generator in both modes and the verification below.

### Remaining work, in execution order

The complete implementation path is now present: parsed assets -> native
orchestration -> all generation stages -> streamed RoE/AB/SoD serialization ->
gzip output and generation/replay CLI. End-to-end debugging is underway;
the evidence below covers initial cases, not the complete corpus or in-game play.

The implementation is published as [draft PR #134](https://github.com/sushi-shi/homm3-decomp/pull/134).
The remaining work is **verification and debugging**: complete the 100,000-case campaign,
resolve its divergences, and run final checks/review before marking the PR ready.

Current working policy (user update): end-to-end debugging is now authorized.
Start with small maps and independent parsing, diagnose divergences against the
native captures, then expand to the comparison and allocation campaigns below.

The output writer borrows map storage and uses one retained prototype-slot
vector. It includes every live object record, fixed reserved prototypes,
decoration-first ordering, native payload narrowing, player/team quirks and
team-assignment draws. Each write starts from the final generation RNG and
returns its own output checkpoint; repeated writes are deterministic without
mutating the generated result. Header descriptions use a bounded stack buffer.

CLI commands and options are documented in
[`tools/homm3-rmg-cli/README.md`](../../tools/homm3-rmg-cli/README.md). Replay
records preserve raw requests and all modeled compatibility inputs before
resource loading. They require the same separately supplied installation assets;
resource fingerprints are not implemented. Map output is published via a
temporary file only after compression completes. Replay reports are published
through a separate fresh inode to avoid truncating an aliased existing map.

Source-only behavior and design reviewers checked output and CLI integration.
Their report-path alias and CRLF replay findings were fixed. Shared description
names, map format values, quest/reward domains and witch-hut masks come from
canonical C++ fragments/headers rather than second Rust definitions.

Initial implementation-only validation: both RMG crates and the CLI binary compiled. Exactly one
isolated replay unit test passed, covering raw-input preservation, both modes,
retail profiles, LF/CRLF reports, and rejection of unknown versions/trailing
input. No full suite, end-to-end map run, native comparison or corpus campaign
was run for this implementation batch. Both reviewers finished with no further
actionable findings after the IO fixes.

The subsequent [hotfix coverage audit](rust-rmg-hotfix-coverage.md) accounts for
every explicit C++ hotfix conditional without finding missing Rust behavior.
An allocation source pass confirmed retained generation/obstacle/group/output
buffers and borrowed payload iteration; it does not replace the deferred
allocation measurements. The lint cleanup removes an infallible helper’s
`Result`, an unused receiver and duplicate wire dispatch, and documents
intentional narrowing and native traversal order. RMG library and CLI Clippy
compilation passes with warnings denied (`--lib --bins`); no tests or map runs
were executed during this audit/cleanup pass.

A final source review restored the river-side bit, noise-corner and terrain-gap
ASCII diagrams and replaced a duplicated terrain direction mapping with the
canonical generated table. Its follow-up found no further documentation/data
issues. Allocation review found no unnecessary heap allocation in successful
warmed generation/output when retained capacities suffice. The declared Rust 1.82 compiler successfully checked the RMG library,
data crate and CLI, plus the existing oracle affected by the shared Clap
dependency. Clap is constrained to 4.5 and the committed lockfile retains
`clap_lex` 1.0.0; dependency updates must repeat that compiler check.

### Initial end-to-end debugging results

Four hotfix baseline maps cover all map sizes, both level counts and all three
formats; every output byte and final RNG state matches the C++ reference. A
further 32 requests from the saved campaign produced 31 exact map/RNG matches
and one matching missing-player-town rejection (native return code 3, RNG
1783991852). The larger campaign is a separate remaining check.

The first retail runs exposed accidental reuse of hotfix prototype admission
during obstacle placement. Scoring and river centers now require only bounded
image dimensions. The admission result is named `hotfix_admission`, so an empty
bottom row is not mistaken for a universal placement fault. Two C++ retail
baselines then matched exactly. The other two stopped at the documented river
coast read beyond the final cell. Retail allocates exactly N cells after its
array cookie; Rust preserves valid flat aliases but reports that out-of-array
read rather than inventing an adjacent tile.

Independent H3M parsing exposed an oracle bug: native RMG omits the editor's
124-byte trailer. The parser now accepts exact EOF after events, while rejecting
partial or nonzero trailers. Native and Rust maps both pass. Six asset-backed
cases also passed repeated generation, repeated writes and A/B/A workspace reuse.
The same integration test verifies three-byte writes, failures in the header and
trailer, unchanged generation RNG, and successful reuse after output failures.
The corrected treasure geometry comparison matches all 453,023 checkpoint lines
across eight retail/hotfix captures.

Nonperturbing captures of retail constructors at 0x53e149 and 0x53e45c resolve
the water-zone draw policy. Zero-filled allocated templates have nine zero town
flags, select neutral alignment and consume no draw. Fills 91 and 255 have nine
nonzero flags and consume one draw each. The stack probe has a nonempty live mask
and consumes one draw even with initial stack fills zero and 0xffffffff. Traced
and untraced maps/RNG agree. Rust now derives water-zone flags from the heap fill,
with a checked optional `TownMask` override. Replay v2 records it; v1 explicitly
retains the previous all-town policy. Historical C++ stage fixtures also name
that override. The medium and XL retail baselines now match the pinned executable
byte-for-byte and in RNG (1772710812 and 3530451699). The small and large baselines
still encounter the classified out-of-array river read. Capture evidence is in
`build/boundary-live-review/`.

The expanded hotfix campaign passed its first 1,000 requests: 987 exact maps
and 13 matching missing-player-town rejections, including final RNG and all
80 repaired request bytes. Eighteen fresh-process repeats agreed. The full
100,000-request campaign continues with bounded workers; this checkpoint is
not a completed campaign result.

Saved retail corpus replays explicitly supply the harness's signed `stackWord`
as the initial key-tent cursor. A nonperturbing capture at 0x54bfeb confirms
generator+0xf5c (`ebp-0x590`) retains that word for both positive and negative
values. Traced and plain map bytes and result states match the archived pinned
executable outputs. This is a controlled-harness input, not a library default.
Capture evidence is in `build/cursor-live-review/`.

Retail sample `sample2-000232` selects shipped template `2SM2i(2)`, whose
unassigned human zones write outside the player-slot arrays. At 0x5499e0 retail
writes `humanSlots[-1]`; at 0x5499e5 it writes `allSlots[-1]`, which aliases
`humanSlots[7]` and creates a phantom seat. Native success in this case depends
on those writes. Rust preserves the selection draw and reports the specific
`UnassignedPlayerZone` fault; hotfix excludes the candidate. The regression
checks fault identity and draw timing. Corpus accounting keeps such native
undefined behavior separate from exact output matches.

Allocation profiling loads assets once, prepares each request separately, and
reuses workspaces across small hotfix, XL hotfix and medium retail cases. Both
warmed repeats of generation and output performed zero allocations/reallocations
in these samples; this is measured behavior, not a zero-allocation requirement.
Cold asset loading peaked at 1,269,248 requested live bytes. First generation and
output peaked at 1,395,524 / 1,430,540 bytes for small hotfix,
7,846,212 / 8,226,372 for XL hotfix, and 8,344,516 / 8,354,188 for medium retail
after retaining the XL capacities. These are total requested heap bytes in the
profiling process, excluding allocator overhead, stack, RSS and transient realloc
internals. Disposable evidence is under `build/rmg-debug-runner/` and
`build/rust-debug/`.

The catalog, lazy values and payload factories remain bound by `TreasuresReady`
and `TreasureGeneration`; do not rebuild their catalogs or reset reservations
while constructing or completing a group.

Implemented treasure behavior retains these source-backed constraints:

- Share an ordered recipe include between native constructor expansion and the
  data exporter. Include dynamic expansion markers in place; also share constructor
  default weights/values and `g_rmgCreatureValueByLevel` rather than duplicating them.
- Expand into retained contiguous definition storage. Keep prepared prototype and
  creature catalogs borrowed so values use the traits that built reward counts.
  Creature reward counts are signed, computed eagerly without RNG by native
  construction; division by zero/overflow faults must not wait for offering.
- Preserve descending eligible creature IDs, descending tent-family ordinals with
  five adjacent values, descending version-limited dwellings, and ascending seer
  family ordinals with descending creatures followed by experience then gold.
  Dwellings remain defined without artwork. Tent/seer ordinals are definition
  subtypes, not necessarily the corresponding prototype row's subtype. The seven
  resource recipes put wood then ore before mercury; artifact recipes are four
  fixed random classes, not an artifact-catalog expansion.
- Keep offer evaluation lazy: first-in-group/terrain-dependent/map/zone-limit
  filters precede value, prototype RNG and geometry/compactness. Roulette selection
  is followed by a second value call and then generation. Quest-creature adjustment
  applies `(2*value-4000)/3` even to the base no-offer value (-1); retain signed
  truncation. Prison exhaustion and scroll availability remain generation-time.
- Constructor-time tent initialization must not clear colors after border guards
  reserve them. Separate immutable definitions from map-owned pools/cursors and
  reservation lifetimes. Treasure payloads and temporary groups must share arena
  ownership, including completion/replacement callbacks and failed-fit cleanup.

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

Connection pathfinding now has typed initial/unreached/seed/arrived movement
state, including zero-cost arrivals with predecessors. Ordered flooding keeps
prior costs, descending direction order, entrance restrictions and the source's
comparison-before-zero-relaxation rule. Path opening retains its starting zone,
excludes zero-cost endpoints and uses ordinary registration for base border guards
without consuming a serialized payload ID. Deferred water-border painting keeps
paired cell/terrain requests, duplicates and plane order; each consecutive terrain
run finishes before the next, using existing brush scratch without a grid copy.

Both stages agree with native captures for all eight maps: all movement costs,
predecessors, terrain/frame/reflection, connection metadata, placement cells and
RNG state (778,496 checkpoint lines across sixteen stage captures). Core tests
now total 69; strict Clippy passes. Both reviewers rechecked their fixes and report
no remaining actionable issues. A regression verifies that even a wholly clipped
raw insertion binds arena ownership when the registered list is empty; a foreign
arena fails before changing borders or RNG.

The behavior reviewer verified one C++/retail discrepancy against the pinned
executable: `buildZoneConnectionPaths` reads uninitialized seed locals at
`0x540780`–`0x540789` if no earlier zone supplied a seed. The Rust port reports
`SeedReplayRequired` after resetting costs, rather than silently copying the
reconstructed C++ sentinel skip. Hotfix skips seedless zones; retail still reuses
an earlier seed. Explicit seed-coordinate replay remains part of the final
compatibility integration. The ordinary stage comparisons above cover defined
source behavior; protected guard creation will also be exercised as crossings
are integrated.

The shared object arena now owns geometry and a typed base/town/monster payload
in one record. Town placement no longer keeps a separate payload vector or a
cached entrance; registered town views borrow arena payloads, and entrance
coordinates are derived from current geometry and the owning prototype. Removal
keeps the record for shared-map ownership. Guard construction connects the
verified selector to shared serialized IDs, monster payloads and registration;
failed selection consumes no ID, and occupied placement draws nothing. Town,
guard and path entry points share arena/catalog admission before mutation.

Eight native guard captures agree through 1,256 creation probes (546 constructed
payloads), 91 registered guards, all 129,600 placement cells/counters/distances
and final RNG states (131,277 checkpoint lines). The sixteen connection-path and
water-border captures still agree after the ownership refactor. All 69 core
tests and strict Clippy pass; both reviewers report no remaining actionable
issues after the shared town-context admission fix.

A named `water_guards_match_alignment` retail replay flag now models the otherwise
unwritten flag on generated water-zone templates. The behavior review verified
that the pinned executable leaves offset `+0x94` untouched while clearing the
allowed-monster bytes beginning at `+0x95` (`0x53e300`–`0x53e45c`). The default
false matches the captured zero-fill baseline; true selects the current resolved
alignment instead of the empty allowed-faction set. This is independent of the
general heap fill. Broader replay-profile corpus coverage remains pending.

Never-placed objects can now release their arena slots after failed placement.
An intrusive free list reuses storage, while checked allocation generations keep
stale handles invalid after slot reuse. Placed and removed objects retain their
records because another map may reference them. Generic zone placement now
scans the source footprint-inset bounds in row order, preserves singleton draws,
and registers only a selected fitting candidate. Its reusable candidate/outline
scratch is separate from connection candidates, so nested tent placement cannot
overwrite an in-progress crossing search.

Eight native zone-placement captures agree on 1,256 attempts (741 placements and
515 failures), object order/coordinates, all cells/counts and RNG states (131,770
checkpoint lines). Existing guard captures and 270,048 fit-query pairs also pass
with slot reuse and map-local zone lookup. Regression checks reject stale handles
after exact-slot reuse and reject discarding placed or removed objects. All 69
core tests and strict Clippy pass. Both reviewers report no remaining findings;
the design review caught and resolved public fit APIs accepting a foreign map's
copied zone metadata. They now take a zone index and resolve it in the active map.

Key-tent and border-guard placement now uses the generic zone-placement helper,
first-subtype catalog lookup, one/three-guard row ADT and shared source constants.
`BorderGuardPlacement` distinguishes failure, actual placement and the native
missing-guard-art bug (reported color zero without placing or reserving objects).
Protected 3x3 neighborhoods mark only empty cells, then clear the stored path
predecessor even for zero-cost arrivals. Cell colors truncate at the native
four-bit field boundary; reservation keeps the full subtype index.

`KeyTentCursor` preserves the raw signed lookup value, including the exhausted
family length. Availability admission happens only after tent/guard registration,
so an unusual matching subtype can retain those placements before a typed index
fault. `RetailProfile::initial_key_tent_color` is optional named replay input;
absence faults at first use unless a rescan already wrote the cursor. The pinned
retail constructor (`0x537b10`–`0x537db8`) leaves generator offset `+0xf5c`
unwritten, and `placeBorderGuard` reads it at `0x540d6d`. Hotfix starts at the
source `KEY_LIGHT_BLUE` value, now obtained through bindgen.

Eight native border-guard captures agree on 314 attempts, 64 successful tent
placements and 192 registered objects, all 129,600 cells and their border/path
states, counts, cursor values and RNG (259,879 checkpoint lines). The 11,862
registration checkpoints still agree. Additional synthetic-catalog regressions
cover absent/negative/huge replay values, missing art, empty families, exhausted
cursor lookup, late reservation faults, duplicate first-match selection,
full-color reservation and repeated failed placements reusing their arena slot.
All 69 core tests and strict Clippy pass; both behavior and design reviews are
clean. Full crossing/gate/shipyard/portal integration remains next.

Ground crossings now preserve the frozen border scan, live direction reads,
minimum-cost ties, fixed crossing count, singleton draws, ordered removal,
source/destination path and entrance order, shared guard suppression and first
reverse-edge completion. `ConnectingZones` owns checked directed-edge handles;
its iterator permits mutation without collecting temporary IDs. This remains a
per-edge operation: the native dispatcher must try ground, shipyard and gate for
each edge in order. A whole-map ground pass would change generation behavior.

Guard scaling uses canonical shared initializer fragments and bindgen-derived
limits, with separately rounded threshold terms and checked native arithmetic.
A private bounded `GuardStrength` prevents invalid table indices. Native scaling
comparisons cover 192 cases; overflow tests retain faults even where widening
would produce a representable final answer.

Sixteen native ground captures cover both modes, ordinary and forced template
border guards, across 454 attempts and 104 successful directed attempts. They
agree on RNG, entrances, graph flags, objects, counts, cells, paths and final
tent reservation vectors (520,134 checkpoint lines). Forced cases create 116
base objects and 27 monsters; ordinary cases create 53 monsters. Workspace reuse
also checks empty entrance state and rejects previous-stage connection handles
before consuming RNG. All 70 core tests and strict Clippy pass. Both reviewers
are clean; their final coverage suggestion added direct reservation snapshots.

Shipyard attempts now use a distinct ownable payload without claiming serialized
object IDs. Road targets belong to the map and are shared by towns and shipyards.
A reused LIFO stack floods open water, marks adjoining land without expanding it,
and retains visit state across a zone's edge attempts. Clearing resets the entire
source level. Shoreline fitting uses the first clear-water probe, while flooding
uses the first water probe regardless of clearance. Both retain the native
column-only bounds checks and flat addressing. The water-offset table and its
ASCII diagram are shared with C++ through a canonical include fragment.

Sixteen native per-edge ground-then-shipyard captures agree on 454 attempts,
264 successes, 60 shipyards, RNG, road targets, visit flags, entrances, graph
state, objects, counts, membership, paths, borders and tent reservations
(520,284 checkpoint lines). Eight ordinary cases contain 71 monsters; eight
forced-border cases contain 145 base objects and 38 monsters. Original native
whole-map outputs remain byte-identical after the isolated probes. All 70 prior
core tests and the new empty-family RNG-fault regression pass, as does strict
Clippy. Both reviewers are clean after moving general helpers to shared modules
and eliminating per-line temporary vectors from the snapshot comparator. The
eight ordinary ground-only captures still match after the shared-state changes.

Subterranean gates now choose the highest combined object-distance sites that
fit on both levels, retaining scan-order ties and singleton draws. Bounds
intersection returns a nonempty rectangle or absence. Candidate selection
precedes the first gate allocation, as retail does. Both base objects and both
entrance records precede guard-value evaluation; approach openings follow it.
Both border attempts run, either success suppresses both monster guards, and
side marking uses empty membership plus native flat addressing. No gate road
targets or serialized object IDs are introduced.

`ConnectingZones::connect_direct_zones` now executes the complete first pass in
native zone/adjacency order and consumes its token into `DirectConnections`.
Each eligible edge tries ground, then shipyard, then gate, skipping a water
destination only after the shipyard attempt. Connection identity and shared
entrance storage now live together in the common stage module. Bidirectional
completion writes the current edge before faulting on a missing first reverse.

Sixteen native first-pass captures agree on final RNG, objects, road targets,
visits, entrances, graph state, counts, membership, movement, borders and tent
reservations (520,175 checkpoint lines). Ordinary cases place 86 gate objects;
forced-border cases exercise gate-side markings and shared guard suppression.
The test invokes the production consuming dispatcher. Native probe isolation
also preserves all sixteen complete C++ map outputs byte-for-byte. All 71 core
tests and strict Clippy pass; both reviewers are clean after keeping the shared
entrance vector private behind its owning module.

Portals and the complete second dispatcher pass are implemented. A private
prototype ADT admits either a two-way prototype or both one-way entrance/exit
prototypes before placement. `PortalProtection` keeps shared guard suppression
across endpoints with borrowed catalogs. Failed unplaced endpoints release their
arena slots; successful endpoints append ordered portal IDs before the zone's
anchor entrance. One-way exits follow both protected entrances. No serialized
IDs or road targets are claimed for portals.

Pass two skips zones without incomplete edges, then clears visits and refloods
existing shipyards in active-object order. Shipyards precede portal fallback.
Both directed records complete even when no portal fits; the shared prototype
cursor advances afterward without RNG. `DirectConnections` is consumed into
`ConnectionsPlaced`, preserving the two passes' ownership and checkpoint order.
Portal protection rebuilds all paths in place, without refreezing border sites,
and marks all five source offsets including occupied cells. Shared source data
retains the ASCII diagram. A canonical checked position-offset helper replaces
three copies across ground, shipyard and portal code.

Thirty-two native full-connection captures cover retail/hotfix, ordinary/forced
border guards, and two-way/one-way resources (1,040,600 checkpoint lines). They
agree on RNG, portal-list order, objects, road targets, visits, entrances, graph
state, counts, cell membership, movement/predecessors, borders, tent reservations
and rebuilt zone-path costs, destinations and directions. Two-way cases place
32 endpoints; one-way cases place 64 entrances/exits. The one-way fixture retypes
only two-way art to unused boat art in both implementations before filtering,
preserving source row IDs and avoiding production test hooks. The 16 unchanged
resource cases leave whole native map output byte-identical after isolated
probes. A missing-exit regression checks deferred pair admission. All 72 core
tests and strict Clippy pass; both reviewers are clean after adding final flood
metadata to snapshots and sharing checked coordinate offsets.

Dry-zone junction preparation consumes `ConnectionsPlaced` into
`JunctionsPrepared`. It resets movement only on owned non-water cells, preserving
connection metadata and protected borders, then follows entrance predecessors
and carves randomized routes in source order. The boundary painter and junction
carver share one midpoint-displacement helper, retaining the signed rounding,
full/half displacement policy and retail zero-roughness draw-before-fault.
Entrance sequences are borrowed; the subdivision stack and flood queue reuse
workspace capacity. Source ASCII documentation remains in place.

Sixteen native junction captures cover both modes with ordinary and forced-border
guard fixtures (520,284 checkpoint lines; 56 dry junction zones). Snapshots compare
RNG, reservations, movement/predecessors, cross-zone flood metadata and all earlier
connection state. Ordinary fixtures visibly carve routes in three maps rather
than merely exercising empty dispatch. All 72 core tests and strict Clippy pass;
both independent reviews are clean. Isolated native probes leave all 32 candidate
and repeat whole-map outputs unchanged against the preceding connection probes.

Mines and nearby resources are implemented. A seven-value `Resource` enum uses
bindgen discriminants; fixed mine counts precede the shared density scheduler.
Starting placement carries its primary town entrance in a private ADT. Mine art
selection borrows the catalog, recording selected and last-scanned prototypes
separately. Site ranking preserves the source's sequential distance, obstacle
and spacing updates. Candidates reuse placement storage; outline scoring uses a
bounded stack buffer. Guard values combine admitted zone/map strength through the
shared scaler. Mine and resource payloads claim no serialized ID; failed unplaced
mines return their arena slots. The resource payload records native defaults.

Both modes preserve the source's last-scanned mine trigger/width behavior and
coin-before-fit resource placement (at most three piles). Canonical mine constants
now live in the C++ settings header and reach Rust through bindgen. Shared trigger
geometry and object-fit helpers replace repeated bodies. A review found that
unselected retail art may have a usable signed width but unusable height: an
`ImageDimensions` ADT now retains those extents without requiring footprint
admission, and the strip reads width alone. Regression cases cover invalid height,
negative width and normal dimensions.

Sixteen native mine captures cover both modes with ordinary and forced-border
fixtures: 521,358 checkpoint lines, 372 mines and 502 resource piles. RNG, object
order/payloads, membership, counts, reservations and all retained movement/connection
metadata agree. All 32 candidate/repeat whole-map outputs remain unchanged by the
isolated probes. All 73 core tests and strict Clippy pass; both reviewers report no
remaining issues after the width-only correction.

Post-mine treasure preparation is implemented as `TreasurePaths`, with a fixed
private `TownZoneCounts` array and the shared path rebuild. Tallying uses current
zone alignment and primary-town presence, not additional-town counts or creature
faction. Pinned retail instructions 0x549bed–0x549c05 prove that alignment -1
indexes the adjacent total field at +0xf60, then reloads and increments it again.
Safe Rust explicitly counts this known alias twice without accessing outside an
array. No replay input or speculative faction repair is needed. Reward adjustment
retains native signed product-before-division and checked overflow.

Sixteen native preparation captures compare 521,392 lines, including faction
buckets, total, RNG and all retained map/object/connection state. The ordinary
path rebuild changes cells in every fixture; forced-border cases create 18 more
guard objects across four maps and consume corresponding art RNG. All 32 native
candidate/repeat map outputs remain unchanged by isolated probes. The 74 core
tests and strict Clippy pass; both reviewers are clear. A focused regression
checks the neutral alias, signed division and overflow before division.

The pre-generation treasure definition catalog is implemented. Canonical recipe
rows, creature/tent reward tables and seer reward rows are shared `.inc` inputs;
C++ keeps its constructors and four named expansion helpers. Constructor defaults
come through bindgen, and cartographer/creature-bank subtype declarations have
one lightweight header owner. The build-host exporter supplies the ordered raw
recipe ADT; Rust admits private definition records and eagerly computes signed
creature counts. Creature/tent/dwelling/seer expansion order matches the source,
including version limits, missing artwork and family ordinals rather than stored
prototype subtypes. Catalog preparation consumes no RNG or reservation state.

`TreasureWorkspace` retains one contiguous vector between preparations and lends
immutable definitions together with the exact creature/prototype catalogs. It
allocates no individual definition objects. The no-template generation path must
skip preparation, matching the native constructor. Later value and payload
queries must remain lazy; this catalog does not prefilter zone offers or spells.

Sixteen native constructor captures compare 12,632 complete records (kind,
subtype, fixed/dynamic value, density and derived reward arguments). All three
formats and both modes agree: 704 definitions for RoE, 818 for either expansion.
The 16 candidate/repeat whole maps are unchanged against the preceding checkpoint.
Focused regressions cover signed rounding boundaries, zero AI faults despite
absent art, recovery after failed preparation, versioned blocks, distinct family
ordinals and reused storage. All 78 core tests and strict Clippy pass. Both
reviewers found no remaining catalog issues.

The extended ASCII audit adds mask/world coordinates, trigger and overlap visit
order, approach cells, clipped neighborhood traversal, path width and native
flat-index alias diagrams. Existing portal/shipyard/reflection diagrams remain.
Rustdoc with warnings denied and formatting pass for these comment-only changes.

Lazy values now run through `TreasuresReady::value(DefinitionId, ZoneId)`. The
consumed stage owns its exact catalog and map, preventing replacement by a catalog
with different traits. Definition IDs carry one preparation owner and become
invalid when reusable storage is prepared again. No value query allocates or
consumes RNG. The dwelling-to-creature table is shared with `src/game.cpp`, and
catalog admission turns each dwelling mapping into a `CreatureId`.

Native signed policies remain explicit: creature and dwelling faction gates skip
arithmetic; dwelling growth/AI multiplication, town adjustment and the final half
bonus stay ordered. Seer cursor/pool gates precede creature valuation, and an
eligible creature quest transforms the base -1 result to -1334. Tent offers compare
only the retained raw cursor; only that branch requests missing retail replay
input. The phase owns the native initial seer cursor, sticky low-pool flag and
fixed used-artifact array; earlier stages cannot modify quest state. Completion
will mutate these same fields rather than resetting them.

Sixteen ordinary/forced-border fixtures compare 247,048 per-zone definition
values and 768,456 total checkpoint lines, retaining RNG, movement, registrations,
tent availability and existing object state. Every native capture repeats exactly;
all 32 candidate/repeat maps remain unchanged by the value probes. All 82 core
tests and strict Clippy pass. Both reviewers report no remaining issues. Focused
regressions cover foreign/stale definition IDs, quest gates and -1334, lazy tent
replay faults, and skipped dwelling overflow for a mismatching faction.

Payload factories now return non-Copy `PendingTreasure` ownership tokens.
`SelectedTreasure` keeps a definition and art prototype together; art selection
resolves the admitted map zone and consumes its original draw. Generation binds
one arena, spell catalog, artifact catalog and hero pool. Foreign contexts are
rejected before factory effects. Payload variants preserve artifact/resource,
Pandora, prison, scroll, tent and seer writer policies; pending quest artifacts
own their seer child and retain their definition ID for later callbacks.

Generic arena discard refuses prison/quest/seer records. The generation's explicit
unplaced discard releases a prison's hero or deletes the quest child, then recycles
the parent without rewinding RNG or IDs. Failed quest construction reclaims its
child. Pandora spell payloads store a bound definition ID and expand the original
criteria against the retained spell catalog, without a spell-list allocation.
The hero spell count has one lightweight C++ header owner consumed by bindgen;
`hero::NUM_SPELLS` aliases it.

Eight native factory fixtures cover all 17 instantiated policies in each mode/
format combination: 6,138 generated objects and 3,438 pending seer children, across
290,427 checkpoint lines. Captures compare chosen art, full reward payloads,
RNG/serialized IDs, hero flags and cleanup. Native prototype reference counts
return to their pre-probe values after each deletion. Every capture repeats and
all 16 candidate/repeat maps remain unchanged. Rust additionally holds prisons
until exhaustion, checks the empty attempt consumes neither RNG nor ID, then
releases them without rewinding either. Scroll regression cases cover empty,
singleton and creature-ability-only eligibility. All 83 core tests pass; reviewers
found no remaining factory issues; strict Clippy passes. Group disposal is now
implemented; completion callbacks remain to be implemented.

Temporary groups now use reusable 16x16 cell storage, retained membership,
object, outline and candidate buffers. The scratch surface borrows common fit
and footprint operations through a constant dirt/unassigned view. Shared cardinal
outline helpers preserve the prototype and group walks' distinct starting rules.
Group candidates retain duplicates, native direction order and flat row aliases.
Arena ownership distinguishes unplaced, group-owned and published records; public
world insertion rejects copied group-owned IDs. Explicit group disposal checks
all ownership/children first, clears memberships and releases records in order.

Selection preserves definition gates, lazy values, per-definition art draws,
compact thresholds and weighted choice. Map/zone limits now have canonical shared
include files. Assembly keeps nested generation/fit retries, target draw rules,
reservation cleanup, absent-guard success and the last-treasure-trigger guard bug.
World-site queries keep all highest-distance ties and the final singleton draw.
The guard fan and world-origin ASCII diagrams remain beside their algorithms.
The source-review admission-order finding (unknown zone drawing before rejection)
was fixed before any reset or target draw.

Before the implementation-first instruction, 87 core tests and strict Clippy
passed for the initial geometry/ownership delta. The subsequent native geometry
comparison reached a retail case-0 line-count mismatch (Rust 27,504; native
27,684). Source inspection identified a snapshot-format defect: native
`groupPrintObject` emits `unit` for payload-free objects, while the Rust geometry
snapshot reused the factory formatter that intentionally emits no such marker.
The saved native case has exactly 180 `unit` records; removing them gives 27,504
lines, exactly the recorded Rust count. Geometry snapshots now emit these markers
without changing factory snapshots or production generation. Failed comparisons
now report the first differing line (including early EOF) and retain Rust output
beside the capture. The earlier Rust text was not saved, so this does **not**
establish value-by-value geometry parity; rerunning the comparison is deferred.
Native geometry captures
exist under ignored `build/rmg-oracle-group-geometry`; disposable probe details
are in `build/group-README.md`. Full assembly probes are prepared but have not
been run. New assembly/site/replacement code has compile checks only. Do not
rerun comparisons until the deferred debugging phase.

World commit now publishes all group roots in order, transfers floor flags,
and runs quest/tent completion. Quest zones retain native distance/jitter order;
artifact selection, child seer placement, tent guards and replacement lifetimes
are implemented. Removed retail parents remain arena records where native
prototype references or memberships survive. Zero-total groups retain roots,
pending seers and reservations in both modes, without leaking Rust allocations.
Zone treasure bands reuse one group, retaining the ordinary/compact retry order.

Underground decoration shares one full-plane brush across zone repainting;
terrain switches finish its repair work at the native points. Coastal flags are
sticky and mark dry non-rock neighbours of water. Obstacle scoring and expansion
use retained candidate, affected-object and pending-position vectors, with lazy
catalog-bound overlap caches. Canonical decoration families now come from one
shared include. Source ASCII diagrams remain beside overlap scoring and anchors.
Both modes preserve the native overwrite of blocked-cell marks. Obstacle inserts
use generator registration: the native base implementation calls virtual
addObject, so decorations also update counters and entrance distances.

Two source-only reviews covered completion and decoration. Findings fixed:
zero-total group lifetime, virtual decoration registration, foreign catalog
admission, checked overlap coordinates, and the moved enums' include guard.
The source canBlockFloor predicate is shared with group floor transfer.
These additions have compile checks only; no tests or comparison captures were
run under the implementation-first policy. Behavioral parity remains unproven.

Road/river tile state and painting are implemented without per-point allocation.
The shared line walker preserves destination-to-previous traversal, corner-cell
painting, conditional endpoint writes and frame-retention RNG behavior. Replacing
a line refreshes the surrounding cells in native side order, including the right
border's penultimate-row quirk. River frame writes alone update near-river and
nearby-target flags; setting only the line type has its narrower native effects.

Road search uses the retained descending worklist, portal lists and gate twins;
newly painted roads trigger source-ordered cost-map rebuilds. Empty retail road
targets fault after the road-type draw, while hotfix returns without roads.
River generation marks object/coast/border targets, preserves E/S/W/N edge draw
order and separate last-inspected/next-position state, and places deltas from
shared canonical art indices and offsets. Multiple initial river seeds append
and pop newest-first; relaxed equal-cost entries remain oldest-first.

The line painter preserves allocated native flat aliases without normalizing raw
coordinates or changing the painter plane. Its neighbor mask tests edge equality,
its walker/clear rectangle retain unsigned arithmetic, and river flag patches
clip the raw coordinate. Out-of-allocation access and absent predecessors remain
typed faults at the corresponding use. Hotfix coast/search guards remain distinct.
Two source-only reviews covered roads and rivers; the identified early painter
bounds rejection was corrected. Compile checks pass; no tests or native runs
were performed for this implementation batch. End-to-end parity is still pending.

The native orchestration API is implemented in `generation.rs`. `Assets` borrows
parsed immutable resources; `Assets::prepare(request, seed)` owns request-dependent
prototype/template preparation and its post-constructor RNG. The returned
`PreparedGeneration` stays alive while `GenerationWorkspace::generate` returns a
borrowed `GeneratedMap`. Every generation stage now runs through rivers without
Wine, file IO, terrain copying or global RNG state. Resource loading and output
remain caller work. The generated result retains arena, catalogs, reward context
and exact RNG needed by serialization; it prevents workspace reuse while borrowed.

Constructor order includes prototype filtering before water resolution/templates,
optional eager treasure definitions, then the shared hotfix required-prototype gate
before selection (also checked with no templates). The pipeline retains original
hotfix starting-town rejection and all later source stage order. Failure reports
own the effective/original request, seed, current stage and exact RNG; stage
checkpoints clear on entry and are present only on successful completion.

Outer workspaces retain treasure offers and nested group buffers. They are lent
to treasure generation and returned before either success or error propagates.
Before another run, old root/nested group handles and offers clear before arena
reset. Other stage buffers retain their existing reuse policies. Both source
reviews are clear after correcting the readiness-gate and checkpoint timing.
At that implementation checkpoint, only compilation had been checked. Current
runtime evidence is recorded under initial end-to-end debugging results above.

Output reference accounting must include every live arena object, not just the
active world list and retired records: replacement factories do not run completion,
so an active replacement quest artifact can still own an unplaced seer child.
That child's native prototype refcount participates in the serialized prototype
list. Count each object record once, regardless of footprint membership count.

Factory and completion constraints, confirmed against native source and retail:

- Extend the existing `ObjectPayload` enum, keeping geometry and payload in one
  arena record. Fixed artifact/resource/scholar/shrine/witch-hut policies remain
  distinct. Pandora rewards use an ADT for experience/gold/creatures/spells, with
  fixed zero defaults elsewhere and no per-definition heap objects. For spell
  rewards, retain admitted selection criteria and bind the exact immutable spell
  catalog through the treasure/output stage. Counting, writing and snapshots can
  iterate the same ordered filtered view; avoid a per-object spell vector or a
  70-entry array inside every object record. Do not accept a replacement spell
  catalog at serialization. A shared spell pool is only necessary if completed
  maps must later detach from their source assets.
- Prison selection reserves a hero before allocation. Retail 0x5348dc selects,
  0x5348f4 allocates, and 0x534902–0b consumes the object ID after successful
  allocation. An allocation fault keeps the hero reservation but does not consume
  an ID. Failed fits/groups explicitly release the hero before deleting its
  object, in group order. Never rewind RNG or serialized IDs on discard; ordinary
  object destruction must not implicitly release a reservation.
- Spell-box generation emits descending levels and ascending spell IDs 0..69
  without RNG. Scroll selection counts eligible spells, then draws even when
  the count is zero; retail rand at 0x534f06 precedes the faulting idiv at
  0x534f0c. A singleton still draws. Eligibility remains generation-time work.
- Quest generation first creates a pending seer hut, selects random-artifact art
  on dirt, creates the artifact wrapper owning that hut, then writes its reward.
  Neither artifact nor seer ordinal is reserved yet. Retain the stable definition
  ID for later replacement valuation. Missing art faults after its selection
  point; dropping/discarding the wrapper must reclaim the pending hut.
- Quest completion scans all artifacts ascending, sets the sticky low-pool flag
  from the pre-selection count (<20), and returns without RNG if none remain.
  It selects one artifact and swaps prototype properties before attempting hut
  placement, without re-registering geometry or counts. Only successful placement
  claims the artifact and advances the seer cursor. Failure reevaluates the
  definition at replacement time; count-zero leaves the original artifact active.
- Tent creation only stores its value. Completion computes value*3 before /2,
  finds border-guard art and allocates the guard before reserving its color.
  Failed placement discards the group before releasing the color and replacing
  the tent. Retail's removed-object leaks need explicit observable-reference
  handling; Rust must not leak allocations or delete an object still active.
- A transient group owns its object identities, including pending hut children.
  Scratch positioning means `discard_unplaced` is deliberately insufficient:
  add an internal ordered disposal path after clearing scratch memberships.
  `reset` only clears non-owning group state; `discard` releases reservations
  then deletes. Nested completion needs reusable scratch separate from its outer
  group. Pair definition/prototype candidates in one retained vector.

Fresh whole-map C++ runs also succeeded and repeated exactly for those eight
cases. Before any Rust generation, C++ retail mode already differs from the
pinned executable for seed 100 / 108x108 / RoE / islands and seed 17 / 144x144 /
two levels / SoD / random water (zero stack/heap replay fills). Keep these
disagreements visible during porting; they are not evidence of Rust parity.

The port initially followed the merged C++ policy of one unused retail town draw
for both the boundary probe and every added water zone. Subsequent nonperturbing
captures established the conditional water-zone draw and the observed nonempty
probe masks; the runtime policy and replay migration are documented above.
Water-zone creature residue remains represented as faction, neutral, or no
matching faction, rather than an unchecked index.

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
