# HotA integration into the Rust RMG

## Objective and acceptance

Make the recovered HotA changes first-class features of `homm3-rmg`: one owned
data model and generator, with explicit versioned rules and input/output codecs.
Scope covers the full generator, including water, islands, underground, extended
map sizes, all factions/terrains/objects, template settings and mirror templates.
Jebus Cross is an early verification case, not the support boundary. The proposed
road-distance value discount is separate future work.

Completion requires native Rust generation and serialization, exercised against
the appropriate pinned native reference, with no runtime C++/Wine dependency.
Compare uncompressed map bytes, effective requests, outcomes and RNG checkpoints;
independently parse the emitted maps and check them in the game. Existing retail
and Complete-hotfix behavior must retain its own verified results. Parsing packs,
adding types, successful compilation and a few matching seeds do not establish
completion. No HotA CLI generation option may silently invoke Complete rules.

## Evidence

- Rust starting point: `29ae0418d` (`origin/decomp-complete-4.0`).
- Recovered C++ reference: `hota-rmg` at `3a1421ef5`, including the destination-road
  fix in `commitTreasureGroup` (`a2ec18af4`). Inspect composed `src/rmg.cpp` bodies
  as well as the nine `src/hota_rmg_*.cpp` units and headers.
- HotA.dll: SHA-256
  `1ed72766595ce5be50b0022fb42ed0369ec65ce0c5f2ef69bfcd76d3b6ceacc6` (1.8.1).
- `docs/hota-rmg-triage-templates.md` and `docs/hota-rmg-triage-drm.md` on that
  branch supersede the early runtime status in its overview. They report exact
  C++ results for their sampled packs/settings; they do not prove the Rust port.
- The existing HotA oracle runs without HD/HW_Rulez, admits only 36..144 requests,
  and does not establish repeated-generation independence. It cannot validate
  the entire objective as it stands.
- Mirror behavior, larger sizes and HD configuration must additionally use the
  evidence in sibling `homm3-mod-analysis/docs/rmg/hd.md`, `rmg-configuration.md`
  and `src/hd/rmg/hd.cpp`, checked against the pinned HD/HW binaries. Its documented
  clock-derived mirror monolith state needs an explicit replay input; a seed-only
  claim would be false. Unresolved behavior remains work, not an exclusion.

## Shared architecture

- Model catalogs as immutable, versioned input data. Counts and availability come
  from the admitted catalog; Complete array bounds must not truncate HotA IDs.
  Check identities at their owning catalog boundary. Keep native source tables
  canonical and extract data instead of maintaining hand-copied competing tables.
- Keep zones, connections, placement policies, object limits, quests, terrain,
  roads and output in their existing owning subsystems. Properties such as road
  policy, connection kind, maximum block value and monster disposition belong to
  those types and are consumed directly by shared algorithms.
- Distinguish the generation ruleset from the serialized map format and from
  observed native compatibility inputs. Do not implement HotA by treating it as
  the existing hotfix Boolean.
- Use focused format-specific readers (`rmg.txt`, `.h3t`, `HotA.dat`) and versioned
  writer branches. Normalize into the shared model while retaining source order,
  omitted-versus-present defaults, deferred selection faults and RNG timing.
- Select genuine algorithm and pipeline differences explicitly: early roads,
  additional connection-path pass, hint solving, object generation and post-pass.
  Avoid a second generator, per-tile version tests and speculative plugin traits.
- Keep mutable state per run/workspace; share immutable assets. Replays identify
  rules, template and asset content as well as seed and external native inputs.

## Implementation sequence

1. **Shared domains and catalogs.** Admit expanded terrain/faction/hero/creature/
   artifact domains through versioned catalogs; preserve Complete filtering and
   bounds. Add explicit ruleset/request admission, all size and water settings,
   and distinguish generation profile from output version.
2. **Template and resource inputs.** Read `.h3t` field-count headers, pack/map/zone/
   connection records, migration rules and object scripts into the common model.
   Add HotA archive/data loading with provenance and complete immutable catalogs.
3. **Setup and layout.** Port bans, hero choices, the separate hint-solver RNG and
   its shuffle, town/terrain/faction constraints, zone sizes, sparseness, repulsion,
   boundaries, rock blocks, water zones and islands.
4. **Connections and terrain.** Implement connection types, directed/null links,
   fictive links, forced/forbidden roads, guard rules, towns/mines/airship yards,
   twelve-terrain painting and the source-ordered path/road passes.
5. **Treasures and completion.** Integrate object definitions and limits, default
   and zone overrides, group fit/scoring/road preservation, guards, quest artifact
   eligibility and destinations, key tents, prisons and cleanup/retained effects.
6. **Output, replay and CLI.** Write HotA format 32's versioned header, catalogs,
   bans, heroes and object records; extend the independent reader. Expose native
   generation with explicit pack selection and complete replay identity.
7. **HD-dependent features.** Recover and integrate mirror placement, links and
   writing, extended dimensions and configuration-dependent rules. Extend the
   reference harness rather than treating native refusal as feature support.
8. **Integration verification.** Compare a cross-feature corpus, minimize/fix
   divergences, exercise fresh/reused workspaces and concurrent independent maps,
   test malformed inputs and output failures, check game loading/playability,
   and publish code with the exact achieved coverage and remaining limitations.

Each step includes focused tests and native evidence where available. Later steps
may expose changes needed in earlier types; do not freeze an unsuitable API merely
because a partial implementation compiled.

## Current state

The integration exposes HotA layout, boundaries, coverage and terrain painting
through the shared workspaces. Full-map generation remains gated; placement and
later stages are not yet admitted.
The checkpoints below record their own achieved scope and evidence in order.

The integration branch is `rust-hota-rmg`, begun as the local Codex lane
`codex/rust-hota-rmg`. The first implementation
checkpoint adds explicit generation rules and a shared template model. The `.h3t`
reader handles variable section widths, pack availability, map/zone/connection
settings, legacy migration, source ordering and single-ended mirror links. Masks
are catalog-sized rather than fixed to Complete's factions and terrains. Object
scripts are retained for the later object-rule interpreter.

Verification of this checkpoint:

- 98 library tests and two compile-fail documentation tests pass. Installed-data
  tests remain opt-in; the two new pack tests were also run explicitly.
- All 34 pinned installed packs, including eight mirror packs, parse across the
  tested four sizes, two level settings, four player configurations and three
  resolved water modes: 1,646 admitted candidates and no deferred native faults.
  Jebus Cross's source graph and extended settings have dedicated assertions.
- Library Clippy passes with warnings denied; formatting and diff checks pass.
- A 36-request Complete reference sample includes both levels, all original
  dimensions, all output versions, water/islands/random water. Hotfix has 35
  exact matches and one matching rejection. Retail has 19 exact matches and 17
  already-modeled coast-one-past faults. No unexpected differences occurred.
  Corpus manifest SHA-256:
  `f9141c88440c0dcbfe45d015ea49acb30fb8bdad553f955217e3556e2546da14`.

This first checkpoint established input/model support, not completed HotA
generation. Layout explicitly rejects HotA rules until its algorithms are
integrated. Subsequent checkpoints below expand requests and catalogs; generation
algorithms, output and HD/mirror execution remain outstanding. Parsing a mirror
pack does not establish mirror generation, and the reference sample is not
exhaustive Complete compatibility coverage.

The next checkpoint adds the allocation-free `homm3_resource::hdat` reader and
installed `treasure::ObjectRecipes` loading. The container layout was checked
against the pinned DLL's loader at RVA `0x127030` and string reader at `0x127920`.
The loader's version comparison accepts 2 (the earlier C++ overview's "1 or 2"
comment is not the admission rule used here). Payloads remain opaque borrowed
bytes, including any stale native pointer values; those bytes are never cast to
host structures. Counts, lengths and exact file consumption are checked.

Object list parsing follows RVA `0x126b30`'s delimiters, comments, NUL termination
and CRT numeric prefixes; duplicate names replace earlier lists, string index 7
is clamped, and incomplete trailing six-integer records are ignored. The pinned
file has 433 records, 151 payloads, 42 additional object recipes and 13 optional
default recipes. Installed-data checks verify these and representative land,
water, shrine and optional reward entries. Three synthetic container tests cover
framing (including every truncated prefix of the fixture); three RMG unit tests
cover integer streams and native object-record selection quirks. The RMG library
now passes 101 unit tests, and both libraries pass Clippy with warnings denied.

At that checkpoint the reader did not initialize the expanded creature, artifact,
hero and terrain catalogs or consume the new recipes in generation. Those require the
native record field interpretations and initialization patches, followed by the
shared definition builder and placement changes. The full objective remains open.

The creature catalog now uses shared contiguous storage with checked ID lookup,
and loads all 200 pinned rows from the installation's `crtraits.txt` plus HDAT
`monstNNN` payloads. DLL RVA `0x16a39f` copies each 116-byte native record; only
town, tier, AI value, growth and wandering-count fields enter Rust. Final native
writes at `0x16b5df..0x16b633` adjust tiers 123/125/129 and assign creature 138 to
Factory. The base spreadsheet comes from `HotA_lng.lod` (SHA-256
`ca8f3a15ddde8264a97e712a78906a4555df6a0799e9bc2d3f09452440bdf3c5` for its
uncompressed `crtraits.txt`). Missing expanded rows and invalid field domains
fail admission rather than leaving uninitialized catalog entries.

Shared guard selection now uses that catalog's creature count and requires
existing monster prototypes under HotA rules (RVA `0x1bca30`). It retains the
RoE 27-slot exclusion and unchanged 116 eligibility upper bound, including the
resulting unfiltered expanded candidates. Tests cover duplicate prototype
selection, missing prototypes, no-draw failure, count variation and that RoE
quirk. All 104 library tests and Clippy pass; the installed-data test checks all
rows are admitted and representative Cove/Factory/Bulwark fields.

A further 14-request Complete corpus sample after these catalog/guard changes
has 14 exact hotfix matches, seven exact retail matches and seven pre-existing
typed coast faults, with no unexpected differences. It covers all three map
formats, both levels, original dimensions and no-water/water/island inputs.

The Complete pipeline and definition builder explicitly reject an expanded
catalog until their remaining HotA stages are integrated. Standalone guard
selection is exercised with retained prototype inputs; versioned prototype
admission, monster disposition/output, archive precedence, other catalogs,
and full generation parity remain outstanding. No initialized native-table or
end-to-end HotA parity is claimed from these resource and algorithm tests.

The shared hero catalog now admits all 215 heroes and 24 classes. Complete class
IDs are extracted from the canonical C++ initializer alongside availability;
HDAT `heroNNN` records overlay those fields (DLL RVA `0x16aafa`, 92-byte records),
followed by the final availability writes at `0x16b536..0x16b568`. Expanded IDs
cannot index a Complete catalog, and missing rows, short payloads or invalid
classes fail catalog admission.

`HeroPool` implements constructor water substitutions, pack availability overrides,
per-class counts and the initial `max(enabled - 48 - 10 * players, 0)` prison cap.
Starting-hero claims stay separate from the counts, as in the native request hook.
Prison selection follows the shared ruleset: Complete descends through its legacy
range and draws for singletons; HotA scans all heroes in ascending order, reserves
the last two enabled heroes per class, excludes starting heroes and skips singleton
RNG draws. A rejected prison restores its hero and class count. The cap is exposed
for the forthcoming shared definition builder; selection itself does not apply it.

All 110 library tests, three installed-data checks and library Clippy pass. The
installed HotA default hero pool has eight heroes per class under each water mode.
An eight-request Complete replay after the pool change has eight exact hotfix
matches, five exact retail matches and three pre-existing typed coast faults.
These tests cover catalog loading and standalone hero selection, not a native
initialized-table snapshot or end-to-end HotA generation. The main generation
pipeline still initializes a Complete pool; request, setup, definition limits and
expanded hero output must be connected when those HotA stages are integrated.

Artifact catalogs now own versioned storage and checked identities for all 166
HotA slots. Base combination component masks are extracted from the canonical C++
recipes. The installed reader applies `artNNN` numeric strings (field 8, clamped)
and the last `artsinfo0` metadata record. Native RVA `0x108220`, hooked at HD
`0x44cd01`, initializes slots 144 onward, ignores changes to 144/145, and applies
partial numeric records before the ordinary recipe-to-trait membership pass.
RVA `0x1058e0` extends the 12 base recipes to 16, changes assembled IDs, and ORs
component bits; repeated recipes accumulate components and the final ordinal
owns a component shared by multiple recipes. Out-of-range component bits are
ignored as in native code; unsafe recipe/root indices fail Rust admission.

The installed base `artraits.txt` from `HotA_lng.lod` has SHA-256
`6ba3f838ea45400d0949b53a9cb02b000001eafe1b452c64429d6f879e29ca90`.
Tests check the added combination recipes, new classes, reserved slots, partial
writes, duplicate records and invalid input. The Complete pipeline and its lower
level treasure-generation boundary reject expanded artifact catalogs until the
per-map bans and quest changes are integrated. Merely parsing the catalog does
not implement the HotA combination-sensitive quest filter or output bitmaps.

At this artifact checkpoint, 113 library tests, four installed-data checks and
library Clippy pass. The eight-request Complete sample again has eight exact
hotfix matches, five exact retail matches and three pre-existing typed coast
faults, without unexpected differences. These are resource and compatibility
checks; initialized HotA table snapshots and full generation parity remain open.

The shared `artifact::ArtifactPool` now owns map exclusions, combination bans and
normalized template overrides while borrowing the immutable catalog. Constructor
bans follow water and ruleset; the selected template can override random-class,
non-disabled artifacts. The no-allowed-artifacts fallback resets artifact 8's
*override* and leaves its effective ban unchanged. Both pack and map settings now
use the same native availability scanner. Invalid list input leaves the pool
unchanged.

Quest completion consumes this pool instead of a separate fixed-size used array.
HotA counts all unbanned, enabled treasure-class artifacts toward its low-pool
threshold, then excludes assembled artifacts and components of permitted
combinations from selection. Selection scans upward, draws even for a singleton,
and claims only after successful seer placement. The owning callback preserves
the sticky low-pool flag and supports HotA's six-seer cycle. Complete retains its
own eligibility, prototype-count cycle and header ban rules. Pool diagnostics now
live at `TreasureGeneration::artifact_pool()` rather than on `TreasuresReady`.

Verification: 117 library tests, five installed-data checks and library Clippy
pass, and the CLI connection-test target compiles after the query API change.
The installed checks exhaust the quest pool under all three resolved water modes
and verify that template combination bans change component eligibility. A further
14-request Complete replay has 14 exact hotfix matches, seven exact retail
matches and seven pre-existing typed coast faults. Independent H3M parsing of all
14 hotfix outputs succeeds and finds 112 seer huts, exercising successful quest
completion in the replay sample. No unexpected replay differences occurred.

Full HotA generation is still gated. Its artifact pool must be prepared before
object-rule pruning and transferred through the complete versioned pipeline;
current Complete generation creates its pool at the payload stage because no
earlier Complete operation mutates those exclusions. HotA group placement,
quest destinations, setup/output consumption and full native parity remain work.

Requests now carry explicit generation rules, resolved mirror mode and eight
starting-hero choices outside the original lobby record. Complete's existing
parser and hotfix policy retain their original domains. HotA accepts all twelve
factions and 180/216/252 dimensions in addition to the original four sizes. The
extended dimensions come from the pinned DLL UI hook at RVA `0x181da0`, with
size writes at `0x181eab`, `0x181efc` and `0x181f4d`. Constructor water coercion
follows RVA `0x1b8f30`: values outside 0..2 become none without a random draw,
while the caller's raw water value remains unchanged.

Mirror request preparation follows HW's HD `0x54c494` entry hook and HW_HOTA RVA
`0x19220` constructor splice. It skips the entry minimum-player repair and exposes
separate constructor counts (halved, with at least one human slot) and one
generated plane. The original/post-entry counts remain available for HotA's outer
hero/prison setup. Generation-entry faction compaction follows HW RVA `0x191c0`,
keeps unused slots and reports an out-of-bounds colour-pair read when the native
zero-human/eight-computer case reaches source slot 8. It does not silently clamp
or repair that case. All eight starting-hero choices survive mirror preparation.

Template preparation now defaults to the request's ruleset. HotA's area, zone and
connection admission use constructor-local counts and planes. The explicit
`prepare_for` override remains available for independent template analysis.
Complete pipeline admission checks the request as well as its catalogs, so a
HotA request cannot silently run the unfinished stages under Complete rules.

This remains preparation support: starting heroes still need wiring into the
full HotA setup path, and mirror monolith state, placement, duplication and output
are not implemented by these request transformations. Native constructor captures
and end-to-end HotA comparisons remain required.

Verification of request integration: all 122 library tests and library Clippy pass.
Both installed-pack checks pass; the expanded seven-size, two-level, four-player-
configuration, three-water-mode matrix prepares 2,399 candidates across 34 packs
(eight mirror packs), with no deferred faults. An installed-asset regression
confirms that a HotA request is rejected before RNG draws when given the current
Complete pipeline. The 14-request Complete reference sample again yields 14 exact
hotfix matches, seven exact retail matches and seven pre-existing typed coast
faults, with no unexpected differences. Runner SHA-256:
`237aa756753aa5e2521af295965c63162ad5a811df6f59d79dac9cb39e98799c`.

The layout subsystem now owns the town/terrain constraint engine in
`layout::hints`. Its graph preserves native town-link duplicates, unique terrain
links, the first-town-only rule when merging through a terrain, domain
intersection order, connected-component order and degree/ID search ordering.
The backjumping search retains failed-component assignments and continues into
later components. Contradictions found during graph construction still reach
the seed/search operations; earlier admission contradictions do not. Handles
carry problem ownership, and all mutable graph and random state belongs to the
individual solve.

The engine uses the solver's independent MT19937 stream and the pinned MSVC
forward shuffle, rather than consuming the ordinary CRT stream. Its clock is an
explicit input: component entry and every 500,000 iterations request signed
100-nanosecond observations, with the native strict `elapsed > 10 seconds` test.
Missing observations produce a typed fault; timeout produces the native
diagnostic and preserves subsequent component work. The solution retains the MT
state for the forthcoming late density-town selector.

Verification: 133 library tests and library Clippy pass, including the new constraint/RNG tests.
MT output was checked against `std::mt19937` across two refill boundaries. A
temporary comparison harness compiled the recovered C++ constraint implementation
from `hota-rmg` and compared 8,200 graphs against Rust, including large independent
components, contradictions and unsatisfiable searches. Diagnostic kinds/order,
town/terrain assignments and the next MT word match in every case. A separately
clock-controlled C++ case checks the complete assignment and MT position at the
500,000-step timeout. Input corpus SHA-256:
`68eb8439e3a6a01670c7f7efa20ce18bf7397640219d83e0b597ffa4a5aaeceb`.
Reference source SHA-256:
`3f99e036c5affbf5922002e9c1691e40f6f865c9daeb344e3ff45c0c56f0cb19`.
Temporary harnesses and inputs remain under ignored `build/hota-hint-port/`.

These are comparisons with the recovered C++ implementation, not new execution
captures from HotA.dll. At that checkpoint, template hint token parsing,
condition application, zone-level diagnostics, late town selection and layout
consumption remained to be connected. The full HotA pipeline gate remains in
place; the constraint engine alone does not establish generation parity.

`ZoneSolution` now connects the shared selected-template model to the constraint
engine. It preserves source zone numbers, the native player/neutral town counts,
land-terrain index mapping and fixed player choices. Mirror player assignment
uses the constructor-local counts, and hint setup sees the compacted generation
town choices. Town, terrain and faction tokens use the native ordered patterns
over bytes, including space-only splitting, numeric prefix captures and silently
ignored unknown faction tokens. The cached `regex` dependency supports Rust 1.65,
within this workspace's 1.82 MSRV; no external process runs during hint solving.

The adapter applies neutral-town restrictions before other conditions, creates
faction pseudo-towns in source order, retains native fatal/nonfatal diagnostics
and applies the same-type/native-terrain/forced-neutral zone settings before
search. Late town queries retain the solver RNG and reapply conditions, preserving
fixed and already-resolved towns. They also preserve the empty-domain quirk
(return zero once without recording that value) and lazy seed-zero behavior
after an earlier failed solve. An unresolved different-town constraint produces
a typed bounds fault instead of writing before the native domain array.

Verification: 142 library tests and library Clippy pass. Both installed-pack
checks pass, including all 2,399 candidates across the seven-size matrix, eight
mirror packs and initial/density-town queries, with no diagnostics and no change
to the ordinary CRT stream. A 2,500-case full-zone comparison against the recovered
C++ covers parsing, initial assignments, late queries and the next MT word. Of
these, 1,540 exercise defined solver outcomes, 695 reproduce numeric conversion
exceptions, and 265 stop at the explicitly modeled native bounds fault. No
unexpected differences remain. The temporary C++ harness uses `std::stoi` for
the DLL's throwing conversion, replacing the reconstruction's acknowledged
`strtol` fallback. Thirteen cases differ only in forced-neutral diagnostic order:
Rust follows the DLL's pair sort, confirmed at RVA `0x1fb3be` calling `0x1f8100`,
while the recovered C++ reports those pairs in traversal order. Dedicated tests
cover these distinctions. Input corpus SHA-256:
`1feb182d062d28d1746ca30318afc1069ee5a3f88e31cb6291f48b03438c79d1`.

The Complete reference sample remains unchanged: 14 exact hotfix matches, seven
exact retail matches and seven pre-existing typed coast faults. Runner SHA-256:
`a66c595d62421c53e999b9ade4f408c08f27be29f6f713d073b948ec89b30102`.
This validates shared selection compatibility and the hint adapter, not the full
HotA generator. Layout still rejects HotA while its positioning/terrain changes
and consumption of solved hints are unfinished; placement, output and HD state
remain part of the open objective.

The shared positioning pass now delegates its versioned policies to
`layout::positioning`: HotA's aggregate-size/sparseness estimate, 4/5 bounding
radii, size-squared centroid and source-ordered origin adjustments, weighted
connection preferences, omission of teleport/random links from candidate
seeding, explicit level placement and the final repulsion filter. Circle radii,
overlap checks and scaled zone sizes still use the full template size. Mirror
requests use constructor-local planes for these policies. Complete retains its
original policies and RNG consumption. The first surface candidate still
bypasses `canPlaceZone`, including an explicit underground-placement restriction,
as in the composed native body.

The DLL's connection jump table at RVA `0x1d51c8` confirms that teleport/random
links decrement the count before retail's increment (net zero). Normalization
RVA `0x1be611` retains both accumulated coordinate low DWORDs when wrapped square
weights sum to zero; the recovered C++ approximation instead resets them. Rust
follows the instructions, with a dedicated cancellation case. Arithmetic keeps
the evidenced 32-bit products before widening and the 64-bit accumulators;
unrepresentable floating conversions remain explicit layout faults.

Verification: 149 library tests and library Clippy pass. Seven focused tests
cover the policy differences, source-order clipping, integer overflow widths,
repulsion tolerance and the native first-candidate quirk. A temporary harness
compiled the recovered C++ spacing, origin and repulsion bodies and compared
10,000 generated cases with Rust; all four outputs per case matched. This tests
those calculations, not the whole positioning pass or a new DLL execution.
Input corpus SHA-256:
`988a87db9da612c8d023f7d1d67e0c3da192e6b26e1caf6dea0d9451d7b57c52`.
Reference source SHA-256:
`dc2e9bdcb1a01cd9e53f67a3968bbb654c0d804223ab641f3ab30e357a1e16ef`.
Temporary inputs and harnesses are in ignored `build/hota-layout-port/`.
The Complete sample remains 14 exact hotfix maps, seven exact retail maps and
seven matching pre-existing coast faults. Runner SHA-256:
`6825b8586cab0a978713a117d7081ebd43e5a9aa0bb9e30468269d367d17dd05`.

HotA layout admission remains gated: initial/late town-query state, solved
terrain/faction consumption and the expanded terrain domain must be connected
before returning a usable layout. The new positioning policies are exercised
directly in tests; they do not yet establish HotA generation through the public
layout API. Boundaries, painting, placement, output and HD/mirror execution remain
in the full objective. The next integration step is to connect hints and terrain
selection while preserving a clear admission boundary for unfinished later stages.

The layout stage now consumes solved town, terrain and faction hints through the
same positioning/normalization pass as Complete. `generate_with_hints` takes the
original seed and explicit solver-clock observations; it never reseeds or draws
from the advanced CRT stream during hint solving. The layout token owns its
solver and per-template-zone town query counters. Constructor queries and later
`select_zone_town` queries follow RVA `0x1c2550`'s player/neutral instance order,
including no query/count increment when the slot allows no town. Reusing the
workspace starts fresh per-map query state. Fixed request overrides use compacted
mirror town choices, and mirror layout uses constructor-local planes.

The shared terrain domain now admits Highlands and Wasteland under explicit
version admission. Solver and layout native-terrain mapping share the canonical
Complete table with HotA's four overrides. Terrain choice skips the retail draw
loops, maps solver land IDs past water/rock and preserves underground substitution.
Creature preference applies forced neutrality, then explicit faction hints,
then alignment; unaligned zones use the patched sentinel-bounded terrain table,
a separate 1-in-4 neutral draw and the subsequent ordinary modulo draw (even for
a singleton). Layout's admitted map-version ordinals all satisfy the native
`m_mapVersion >= 0` argument to this scan.

Boundary admission now rejects HotA layouts before mutating workspace or RNG,
rather than applying unfinished Complete algorithms. Highland/Wasteland frame
selection similarly reports unloaded data before indexing Complete's tables.
Their blending/separated-neighbour properties follow the native rule constructor.
Placement score lookup returns an absent score column explicitly; it cannot index
beyond a Complete rule with an expanded terrain ID. These are temporary admission
boundaries, not exclusions from the full objective.

Verification: 151 library tests, four layout integration tests, two documentation
tests and library Clippy pass. Both installed-pack checks pass; all 2,399 candidates
now complete layout, including eight mirror packs and all seven sizes, both
requested plane counts and three water modes. The matrix includes 1,556 Highland
and 1,282 Wasteland zones, no layout failures and no hint diagnostics. Focused
tests cover retained late-query state, workspace reuse, missing-clock admission,
mirror town compaction, native/underground terrain choice, faction precedence,
neutral/singleton draw counts and safe expanded-terrain admission.

Six fresh **HotA.dll execution captures**, using the pinned module and the archived
reference-only route of the native oracle driver, match Rust on all 54 original
template zones: index, scaled XY/plane/radius, alignment, terrain and creature
faction. Compared cases are Jebus Cross 108 seeds 1 and 7 (the latter fixes Factory
and Bulwark), Default Random Map 36 underground/water seed 42 and 72 islands seed
17, 8mm6a 144 underground/water seed 5, and 2sm2c(2) 72 surface seed 10. Fields
come from the native `zones` phase, comparing only the initial template-zone
prefix; extra boundary-created zones are not part of this layout comparison.
The final layout CRT states also match the trace immediately before boundary
random calls. These captures run without HD and do not verify mirror execution
or extended-size native parity. A 2sm2c 72 underground request produced no native
zone phase because its template was not admitted and is not counted as a match.

Artifacts are under ignored `build/hota-layout-native/`, with a minimal Rust stage
probe, job records, phase/trace captures, comparison output and the sandbox replay
script. The reference host uses read-only game inputs, temporary prefix/Data
overlays and disabled networking. DLL SHA-256 is the pinned value above; archived
oracle driver SHA-256:
`f61a8bc97f3c74319ef84d1ed24f1c9218e633d530fcbe9eb5136001702a9c34`.
Comparison report SHA-256:
`82b5998f51057bd1dce1589dfdc0317f09993e6001854806c963ef3cb06951e7`.

The Complete sample remains 14 exact hotfix maps, seven exact retail maps and
seven matching pre-existing coast faults. Runner SHA-256:
`54190a898b06a94df4d36acef4753f3e65875ae445f5e31d04b32649edb45a74`.
No full HotA map has yet been generated or serialized by Rust. Next is boundary
construction (irregular-edge limits, rock blocks, water zones and islands), then
installed terrain-frame data and painting hooks. Town-query state must continue
through those stages to later placement; serialization, HD/mirror completion,
whole-map replay and game loading remain required.


The shared boundary stage now admits HotA layouts. The Delaunay constructor uses
HotA's enclosing square (-800..1100 instead of -200..400), including when workspace
storage is reused across rulesets. The seven patched immediates in the recovered
`rmg_support.cpp` constructor matter even on small maps: they change integer
circumcenter rounding and the starting edge of some rings. The native comparisons
below initially exposed those differences and now agree without polygon rotation
or cell normalization.

Irregular edges carry an owned, edge-local shape through the existing rasterizer:
the shrunken-site quadrilateral, circle radius, nearest perpendicular crossings,
sagitta/roughness limits and narrow/wide sign-selection rules follow RVAs
`0x1ce000`, `0x1cacd0` and `0x1cadc0`. Empty ranges consume no CRT draw. Ordinary
Complete boundaries and island strokes retain their existing midpoint policy.
HotA's unclamped final-cell claim, first-cell preservation, straight-edge final
paint flag, rejected outside-segment clips and asymmetric corner-walk conditions
are part of the shared raster/clip methods.

Radial construction uses the template's rock-block factor (or the underground
four-radius default), distinguishes the probe size from the radial reach, and
creates water zones only on a surface with water. Fresh temporary/water slots
have no allowed towns and consume no constructor town draw; water creature
preference is neutral. Directed connection policies survive into the boundary
map, with fictive links excluded from HotA distance propagation. The map also
owns the layout's hint solution and query counts for later town selection.
Mirror requests use constructor-local planes here; copying and finishing the
other half remain later work.

Verification: 155 library tests, four layout integration tests, two documentation
tests and library Clippy pass. Focused cases cover displacement draw/no-draw
branches, owned endpoints, clipping/corner walks, fictive-distance behavior and
retained query state. The installed-pack matrix completes boundaries for all
2,399 admitted combinations (34 packs, eight mirror packs, all seven sizes,
87,975,072 raster cells, 249 rock-block maps), without hint diagnostics. This is
Rust execution coverage, not native proof for HD sizes or mirrors.

Eight pinned HotA.dll executions agree on all 114 zones, every polygon point in
order, every zone-distance entry, all 142,560 cell ownership/terrain-mark pairs,
and the final boundary CRT state and draw count. Six are the layout captures
above; two fresh runs add Sapphire 144 underground/no-water seed 19 (rock factor
3.1) and Jebus Outcast 108 underground/no-water seed 23 (rock factor 3). The native
`zones` phase is before painting, so this does not establish island inset,
terrain-frame or painted-map parity. These executions remain no-HD, non-mirror,
at most 144 tiles per side.

Temporary stage probe, captures, comparison scripts and reports remain under
ignored `build/hota-layout-native/`. Boundary report SHA-256:
`f3c0923825525d932c0ecf878833f2614e488b14b2a65b817106b92b618c9668`.
It includes the phase hashes, case counts and exact compared RNG endpoints.
The Complete sample still gives 14 exact hotfix maps, seven exact retail maps
and seven matching pre-existing coast faults. Updated replay runner SHA-256:
`081f6920068709583e1cd67d5e9a9269fa0e6464ebee194a00fcecb20e13748c`.

The admission boundary has moved to `BoundaryMap::prepare_terrain`, before RNG
or workspace mutation. Next work is island coverage and saved zone centers,
installed Highlands/Wasteland frame data, painting hooks and the terrain
post-pass. Generated water-zone placement/treasure defaults must be consumed by
their placement stage. Whole-map HotA generation, output, HD/mirror completion,
native whole-map replay, game loading and publication remain required.


Terrain frame data now belongs to the shared immutable `TerrainCatalog`. Complete
uses its canonical source tables; `parse_hota181` reads `terrainNN` HDAT records
and an explicit 992-byte slice of the pinned DLL at RVA `0x25f860`. It validates
all 124 shape/special entries and contiguous frame ranges, requires both new
terrain records, preserves last-record replacement and the native unsigned
probability calculation. Highlands/Wasteland probabilities come from HDAT's
integer entry `min(1, count - 1)`; the installed records both supply 50. The
binary table is not copied into a second hand-maintained Rust inventory.
`TerrainTile` retains validated shape/special metadata, so expanded tiles never
index Complete's arrays. Catalog clones share their immutable installed data.

The existing brush now supplies its actual in-bounds neighbours to versioned
base and transition selection. HotA's RVA `0x1f2af0` excludes used same-terrain
frames, ignores duplicate exclusions, restores the full range when exhausted,
and consumes exactly one biased modulo draw. Existing matching frames still
skip selection; fixed rock artwork retains its ordinary embedded reflections.
Complete's public selection helpers remain wrappers over the Complete catalog.
`TerrainWorkspace::with_catalog` carries the catalog across later brushes and
rejects a map/catalog rules mismatch before changing cells or RNG.

Verification: 158 library tests, four layout integration tests, two documentation
tests, library Clippy and the explicit installed-terrain test pass. The latter
admits every new-terrain frame, checks special frames 102..117, an exact fill
selection/draw count and exclusion of seven of eight edge variants. Malformed
ranges/records, exhausted ranges, duplicate neighbours, old-frame preservation
and unsigned probability behavior have focused synthetic tests. A temporary
host harness compiles the recovered C++ `hotaSelectLeastRepeatedFrame` body and
compares 10,000 fill/transition contexts against Rust; frames, final CRT state
and draw counts all match. This is recovered-source helper verification, not a
new HotA.dll execution or complete painted-map comparison. Artifacts are in
ignored `build/hota-terrain-port/`; corpus SHA-256:
`770706c6839fa11a1a83211eaec0775c558410b219669be3d83477fcd6bb7ed0`;
compiled reference source SHA-256:
`3a8fb49e14569a39136c66607c95d8859c480bf6bc84ab96ce26976e752cf44e`;
extracted pattern bytes SHA-256:
`d8ee75b2367bdc7059c92a4ba6693c388779a92f5c6350f7dc34f130485fbe96`.

Complete replay remains 14 exact hotfix maps, seven exact retail maps and seven
matching pre-existing coast faults. Runner SHA-256:
`ce2baf19a1664f3d44af205e5a60d25b1d1de01ff489a0d547679955c524e38f`.
HotA coverage/painting is still gated: saved centers and island coverage, brush
strength 1, rock-block surface Dirt, ownership/bounds updates during the zone
brush flush, the ordered unowned-water flood, guard caps, border/reachability
flags, discarded unreachable ownership and final bounds/recentering must be
integrated together. Resource IO must supply the pinned pattern slice when the
full HotA assets/CLI path is admitted. The eight native phase captures retained
from boundary verification are the next painted-map comparison target.

The shared coverage and terrain pipeline now admits HotA. It retains saved zone
centers and growing bounds, paints with strength 1, initializes the surface to
Dirt for no-water rock-block maps, and updates zone ownership/bounds for cells
repaired during the zone brush flush. The post-pass follows RVA `0x1ce1c0`: guard
caps, ordered unowned-water floods, land-border classification, reachability from
saved centers, removal of unreachable ownership, then final bounds/recentering.
`PaintedTerrain::regions` retains border and reachability classifications for
placement. Other HotA tile flags belong to their later placement operations.
The admission gate moves to `PlacementWorkspace::begin`, before state mutation;
full generation remains gated at asset preparation.

The fixed rock rule also uses HotA's least-repeated selector for both fills and
transitions (hooks at HD `0x5b3dd6` and `0x5b3e4d`). Embedded rock reflections
still select the frame range and leave the sprite reflection flags clear. A
focused regression test covers neighbour exclusion for rock fills.

All eight retained native DLL terrain-phase captures now match: 114 zone centers
and bounds, 142,560 terrain/frame/reflection records, ownership and paint bits,
and every final CRT state/draw count. This covers the previously listed Jebus,
DRM water/islands, underground, fixed Factory/Bulwark, 8mm6a, 2sm2c, Sapphire and
Outcast cases. The captures do not contain the extension's guard caps or region
flag arrays; those are source-based ports, with ownership providing indirect
coverage of reachability. The reference remains non-HD, non-mirror and at most
144 tiles per side. Comparison report SHA-256:
`32dbcf3c3765103563978d34ecb3fe891becc886f79ce344ad3c79952aabc305`;
probe, phase captures and report remain in ignored `build/hota-layout-native/`.

Verification: 159 library tests, four layout integration tests, two documentation
tests, the installed-terrain test and library Clippy pass. With installed frame
data supplied, the pack corpus now runs through painting: all 2,399 admitted
requests across 34 packs (eight mirror packs), seven sizes, both requested plane
settings, three water settings and four player configurations complete using
reused workspaces. It processes 87,975,072 cells and 249 rock-block maps, with no
hint diagnostics or deferred faults. Every retained owner is reachable and all
zone centers/caps are initialized. This is Rust execution/invariant coverage,
not native equivalence for the expanded-size or mirror cases.

The 14-case Complete sample retains 14 exact hotfix outputs, seven exact retail
outputs and seven matching pre-existing coast faults. Replay runner SHA-256:
`f0559e5264fd153998e2be6078e3c96ae19a5f9868e498beeb7b41b732956f41`.
Next work is shared placement state, expanded terrain scoring and the ordered
HotA connection/town/mine/road operations. Whole-map generation, versioned output,
HD/mirror completion, native whole-map replay, game loading and publication
remain required.

`PlacementRules::parse_for` now reads the versioned `rand_trn.txt` layout in the
shared catalog (recovered `readObjectPlacementRules`, DLL RVA `0x1f23f0`). Terrain
scores use catalog-sized storage: columns 7..15 remain Dirt through Water, Rock
is always -5000, columns 16..17 hold Highlands/Wasteland, and both neighbour
matrices start at column 18 for HotA. Complete retains its existing parser entry
point, columns, last-match rule selection and hotfix row-skipping policy. HotA
reports unsafe short rows/identities instead of applying that Complete hotfix.
Rule handles still own their catalog and source-column identity.

Prototype rule binding now uses the rule catalog's generation version. HotA
scans all twelve recommended bits, treats a first Rock bit as no binding, uses
the existing prototype bucket without Complete's second alias lookup, and picks
the first matching subtype. Prepared catalogs retain the ruleset; registration
rejects a catalog from another ruleset, and full generation rejects HotA
placement catalogs before constructor RNG draws while that pipeline is gated.
This does not yet load HotA's expanded object masks or changed object traits:
`PrototypeSource::parse` still reads Complete's resource format. The expanded
mask binding test supplies the masks directly to isolate binding from resource
loading. The installed HotA core archive cannot be read as the existing plain
LOD directory; its object resources and versioned loader remain required work.

Verification: 161 library tests and library Clippy pass. New tests cover both
extra score columns, the omitted Rock column, matrix offsets, duplicate binding,
HotA short-row/identity failures and the first-Rock-bit stopping rule. A temporary
host harness compiles the recovered C++ loader body with spreadsheet/prototype
adapters, then compares 200 generated tables (4,954 rows) with Rust. All terrain
scores, both neighbour matrices and first-match subtype bindings agree. This is
recovered-source verification, not an installed-data or native DLL comparison.
Ignored artifacts are in `build/hota-placement-port/`; generated corpus SHA-256:
`e13f09c8d98acc7cf6b89ed1afc21424193c714ef68109e024b8eab83fde0a54`;
compiled reference source SHA-256:
`083509b0ef9eaaaaf6dd123c39ab8baa3747fc9a25497fac37297596f9f8e95f`;
comparison report SHA-256:
`22f5aa6a08449938ad097cc318682da80696c17e42b11f47ceb786ed219b3682`.

The installed Complete admission test also passes: both a HotA request and a
HotA placement catalog fail at the asset boundary with the original seed and
zero draws. Complete replay retains 14 exact hotfix maps, seven exact retail
maps and seven matching pre-existing coast faults. Runner SHA-256:
`27cf0dfa7abeeac4482678101227df0cd5e24f7517ffbae2784466db44f5af61`.
Placement state, the frame-aware scorer, installed prototype loading, later
HotA operations, output and full-map/native/game verification remain unfinished.

`HotA.lod` is now readable. Its header's reserved word holds a key; zero and
`0x007e0213` (Complete's `H3sprite.lod`) remain named-format markers. The
`homm3_lod::HashedDirectory` reader decodes the keyed records, whose names are
FNV-1a hashes of the lowercase resource name (DLL lookup/read RVAs `0x170ba0`,
`0x171e40` and `0x170e20`, hash `0x174050`); the named reader rejects such a
header instead of misreading it. `Installation::open_with_archives` places
explicitly ordered archives ahead of Complete's for both text and masks, so
resource precedence is an input rather than a ruleset side effect. Only stored
and zlib members are decoded; other codec tags fail explicitly.
`PrototypeSource::parse_for` reads HotA's twelve-terrain object masks. Prepared
catalogs reject placement rules of another ruleset, and asset preparation
rejects a HotA prototype source while the pipeline is gated. This completes the
Codex lane's uncommitted resource work.

Verification: three synthetic hashed-directory tests (keyed extents, case
folding, NUL-terminated names, named-reader rejection, payload bounds) and an
installed-data test. Through `open_with_archives`, `HotA.lod` supplies
`objects.txt` (1,883 rows and 1,865 distinct masks, none falling back to
`default.msk`; 1,877 rows allow each new terrain) and HotA's `rand_trn.txt`.
Complete's nine-bit reader rejects these rows, and a Complete placement table
cannot prepare them.

After rebasing onto `ddf0d4873`, the Complete replay runner is byte-identical
to the pre-rebase runner (SHA-256
`1533627251731c24831ec09c5474f9c30c3a5ed50a35b95affa895231c742b0f`). The
first 2,000 corpus requests give 1,970 exact hotfix maps and 30 matching
rejections. Retail gives 1,099 exact maps, 880 pre-existing typed coast
faults, 20 pre-existing typed unassigned-player-zone faults and one native
fault. The replay classifier now accepts the shared reader's
`template row` wording for the last of these. No mismatches occurred.

`placement::scoring` holds HotA's frame-aware scoring policies from the object
scorer at RVA `0x1c6090`. A blocked draw cell's transition frame also counts
the terrain of its edge art: blending shapes count Dirt, hard shapes Sand,
mixed shapes both. A terrain in the neutral band `(-5000, -4000]` is not
counted through such a frame, while base frames (and shape 28, the native
switch's default arm) always count their own terrain. A counted score at or
below -4000, or a total below -1000, rejects a multi-cell object and gives a
single-cell object the native forbidden score -2,000,000,000; a counted
positive score must belong to a terrain under a blocked cell. The decoration
filter (RVA `0x1c7ea0`) requires edge terrains above -5000, moves shape 28 to
the mixed family and requires a plain frame's own score above -4000.

The same-prototype spacing scan (RVA `0x1c7c90`) covers seven cells around the
blocked bounds, clipped to the map (RVA `0x1cac70`). It keeps the native gap
asymmetry: west and north gaps count from the first blocked column/row, east
and south gaps from one past the last, and a diagonal cell takes the smaller
axis gap. Strict scoring then adds three per blocked cell less nine; a
same-prototype object at distance `d < 8` costs `(8 - d) * 20`.

These are policy building blocks. The footprint, draw-list and neighbour walks
need HotA's per-cell draw lists and records, so `score_obstacle` and decoration
remain Complete-only behind the placement gate.

Verification: eight focused tests. A temporary host harness compiles the
verbatim recovered helper bodies and the scorer's terrain-gate and finishing
blocks from `hota_rmg_terrain.cpp`, then compares 40,000 generated cases:
20,000 tallies over real Complete frames of all nine non-rock terrains (with
decoration verdicts per cell) and 20,000 spacing/finishing cases. All agree;
two deliberate mutations (shape 28 classification, east gap) are detected.
Recovered source SHA-256:
`58ef1600f2ddf38cc01b5491c4983770932aa2de27448a2948fd30308f7288e2`;
compiled reference SHA-256:
`6820c500dbe198a4cacf184af3062ecb954f1d7ed0714b0eb1c2756be4a65459`;
corpus SHA-256:
`2dc1b7f7dedbfa40f4e71d0c522fff82ad594cbed620bf2de63e8d682fb7e59e`.
Artifacts are in ignored `build/hota-scoring-port/`. This is recovered-source
verification, not a HotA.dll execution capture.
