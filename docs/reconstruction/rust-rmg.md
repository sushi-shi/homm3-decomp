# Rust random-map generator

The native Rust library and CLI generate RoE, AB and SoD maps in retail or
`HOMM3_RMG_HOTFIX` mode. The behavior reference is `decomp-complete-4.0` at
`c2e0f6ea6d9ba70dc37634b80d3a01727ac5fa02`, with the pinned retail executable
authoritative for retail behavior. Generation runs without Wine or the game
executable. The future engine will use Rust in place of C++ RMG; game/lobby
integration is outside the current scope.

The currently known whole-map output/RNG differences have been resolved. Full
corpus parity and in-game playability have not been established. See
[CLI usage](../../tools/homm3-rmg-cli/README.md) and the
[hotfix coverage table](rust-rmg-hotfix-coverage.md).

## Architecture

| Crate | Responsibility |
| --- | --- |
| `homm3-rmg-data` | Bindgen declarations and generated immutable tables from canonical C++ definitions |
| `homm3-rmg` | Parsed domains, compatibility policy, RNG, generation and H3M serialization |
| `homm3-rmg-cli` | Installation resources, explicit seeds/modes, gzip output and replay reports |
| Existing resource codecs | Borrowed archive and resource parsing |
| `homm3-map` | Independent parsing of generated output |

The public flow is resource bytes -> parsed `Assets` and `Request` ->
`PreparedGeneration` -> `GenerationWorkspace` -> `OutputWorkspace`.
Assets borrow immutable catalogs and resource storage owned by the caller.
Preparation resolves mode/version-dependent domains; generation borrows that
prepared context and mutable workspace. File IO, compression and scheduling
belong to callers.

Parse raw inputs into enums and private newtypes for bounded values, player
slots, object IDs and coordinate domains. Named `ObjectKind` constants are
checked at compile time against bindgen values. Optional values replace sentinels;
stage types express which operations are available. `PlacementMap::cell` admits
coordinates into a borrowed `MapCell` containing state, terrain and zone;
`WorldPosition` remains the signed calculation/probe type. Flat row/plane aliases
use a separate internal resolver. `ObjectArena::resolve` checks an ID once and
borrows its geometry and payload together; `PositionedObject` additionally
carries an assigned anchor, including for retained objects. These local borrows
prevent invalidating the viewed storage. Entrance queries use `PositionedObject`
and return a coordinate directly. Object factories retain resolved geometry
through kind admission and arena insertion. Persistent IDs retain owner/generation
checks because slots can be recycled. Keep catalog-dependent construction tied
to its catalog. Parsing must preserve the native timing of
random choices and request repairs. Failures carry stage/RNG diagnostics and
the effective request when available. Path walkers match `Movement` variants;
zero-cost arrivals retain a predecessor even where walking stops. Numeric costs
remain available for search priorities and compatibility thresholds.

Generation uses an owned RNG and reusable, mostly flat storage. The object
arena holds typed payloads shared by world placement and temporary treasure
groups. Explicit completion/discard operations manage reservations and object
lifetimes; discarding a failed group must not rewind RNG or object IDs. Keep
the catalog that generated a payload bound through serialization. Pandora spell
payloads carry an admitted `SpellReward`, so expanding spells needs no definition
lookup or repeated reward-kind check.

The implementation uses `std`. Retain workspace capacity between maps and
borrow catalogs instead of cloning them. Allocations are allowed when useful;
measure cold asset loading, generation and output separately before optimizing.
`OutputWorkspace` streams to `std::io::Write`. Each write starts from the final
generation RNG, so repeated serialization does not mutate the generated map.

## Shared definitions and documentation

Clang selects existing enums and plain records from the original headers and
translation units into a build-only header. Bindgen imports that header, excluding
C++ methods, containers and ownership. Rust domain types use the generated values;
native declarations do not need to move into special headers for Rust. Unnamed
algorithm literals remain part of the Rust implementation.

Canonical initializers remain in their ordinary C++ translation units.
`homm3-rmg-data/extract.py` uses Clang and the repository's compiler profiles to
select named definitions, constructor defaults, terrain-rule arguments and
ordered treasure recipes. A host exporter evaluates those expressions and emits Rust
scalar values, including exact float bits, without copying native struct layouts
or pointers. This runs at build time even when cross-compiling; generation has
no runtime C++ dependency. Asset-derived traits are parsed from installation
resources. Neither C++ source layout nor helper boundaries should serve the
Rust exporter; adapt selectors when native reconstruction changes them.

Building RMG requires the repository Python/Clang environment and configured
VC6 headers, plus libclang and a host C++ compiler. `HOMM3_PYTHON` can select the
Python executable. Cargo tracks parsed TU/header dependencies and extraction
configuration. Missing or ambiguous definitions, unsupported recipes, and Clang
errors affecting selected definitions fail the build. Errors confined to
unrelated VC6 function bodies do not invalidate clean data definitions. Selected
declarations must agree across TU profiles. Unsupported constructors and
host-dependent default expressions fail extraction rather than changing data.

Keep ASCII diagrams beside canonical tables and algorithm implementations.
Source-specific ordering, lifetime and native-behavior evidence belongs beside
the affected code. Git history retains the implementation progress log.

## Compatibility and limits

Retail mode targets defined native behavior: uncompressed map bytes, RNG draw
order/state, effective requests, traversal order, ties and serialization.
Hotfix mode applies the referenced C++ hotfix policies; the
[coverage table](rust-rmg-hotfix-coverage.md) maps those policies to Rust.
Source coverage alone does not establish executed parity.

`RetailProfile` models observed memory-dependent inputs explicitly:

- Stack-word and allocation-byte fills default to zero.
- Water-zone town flags derive from the allocation fill unless a checked mask
  overrides them. Zero allows no towns and consumes no town-selection draw;
  nonzero allows all nine factions.
- The water-zone alignment-matching flag is independent and defaults to false.
- The initial key-tent cursor is an optional signed value. If absent, reading
  it before initialization returns a typed fault. A particular oracle harness
  may establish its value from stack fill; this is not a general default.

These inputs do not describe arbitrary native memory. Rust never reads
uninitialized storage. Valid flat aliases into another allocated map row or
plane preserve retail behavior; a coast read beyond the final cell returns a
typed fault. Unassigned starting zones that make retail write before its
player-slot arrays also return a typed fault after the selection draw. Keep
such undefined behavior and native crashes separate from successful parity.
Model observable retained-object effects explicitly without leaking memory.

Replay reports preserve the original request and compatibility inputs. Version
2 records the optional water-town mask; version 1 retains its former all-town
policy. Reports neither embed nor fingerprint installation assets, so replay
requires the same resources. Compare uncompressed H3M streams: gzip bytes need
not match the game's compressor. Native RMG omits the editor-only map trailer.

Verification compares bytes, outcomes, repaired requests and RNG against both
retail and current C++. Cover all sizes, level counts and formats, player/team
settings, malformed resources and short/failing writers. Independently parse
maps, repeat cases in fresh processes, and vary workspace reuse order. Complete
the 100,000-case campaign and resolve unexplained differences before claiming
corpus parity. Preserve the workspace's Rust 1.82 minimum compiler version.

## Parallelism roadmap

1. Run independent maps with shared immutable assets and one RNG,
   `GenerationWorkspace` and `OutputWorkspace` per worker. Bound worker count
   to bound retained memory. The current ownership model supports this split.
2. Parallelize RNG-free preprocessing and independent read-only calculations
   where measurement justifies it.
3. Consider intra-map work using immutable snapshots, independent proposals and
   deterministic serial commits. Require identical output across worker counts
   and schedules before enabling it.

Keep the serial implementation as the reference. Per-zone RNG streams,
reordered draws and concurrent mutation of generation state would change the
compatibility contract.
