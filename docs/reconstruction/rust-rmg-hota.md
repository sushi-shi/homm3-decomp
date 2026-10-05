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

The isolated integration branch is `codex/rust-hota-rmg`. The first implementation
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

This is input/model support, not completed HotA generation. Layout explicitly
rejects HotA rules until its algorithms are integrated. Requests and generation
catalogs still have Complete domains; extended resources, algorithms, output and
HD/mirror execution remain outstanding. In particular, parsing a mirror pack
does not establish mirror generation, and the reference sample is not exhaustive
Complete compatibility coverage.

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
