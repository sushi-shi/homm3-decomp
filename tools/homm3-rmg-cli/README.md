# Native Rust RMG

The implementation connects installation resources, generation and streamed H3M
output. Initial whole-map comparisons and independent parsing pass; broader
corpus debugging is underway. See the implementation plan for measured coverage.

Build from the repository's Rust/Nix development environment:

```sh
cargo build --manifest-path tools/Cargo.toml -p homm3-rmg-cli --locked
```

The committed dependency lockfile supports Rust 1.82. Recheck that compiler
when updating dependencies: newer Clap and lexer releases require Rust 1.85.

Generate a compressed map using a Complete installation's **Data directory**:

```sh
homm3-rmg generate --data /games/HoMM3/Data --output /tmp/example.h3m \
  --mode hotfix --seed 42 --size 72 --levels 2 --format sod \
  --humans 2 --human-seats 0 --water normal
```

Modes are `retail` and `hotfix`; formats are `roe`, `ab`, and `sod`. Side lengths
are 36, 72, 108, and 144. Run `homm3-rmg generate --help` for player, team, faction
and monster options. Seeds are explicit unsigned 32-bit values; zero is valid.

Retail memory-dependent behavior uses explicit inputs: `--stack-word`,
`--heap-byte`, `--water-zone-town-mask`, `--water-guards-match-alignment` and
`--initial-key-tent-color`.
The first two default to zero, the flag defaults to false, and an omitted tent
cursor produces a typed fault if read before initialization. These are the
currently modeled inputs, not a claim to reproduce arbitrary native memory.
Water-zone town flags follow the allocation fill by default: zero allows no
towns and consumes no selection draw; a nonzero byte allows all nine factions.
The optional mask overrides those flags (decimal 0..511, bit k permits faction
k). Retail reads beyond allocated map storage return typed errors; valid aliases
into another allocated row or plane retain native behavior.

Each attempt saves a `.rmg-replay` report beside its requested output (override
with `--report`). It contains the original request and compatibility inputs,
then effective request, stage/RNG checkpoints, output state or the failure.
Replay uses the same assets supplied explicitly through `--data`:

```sh
homm3-rmg replay --data /games/HoMM3/Data --input /tmp/example.rmg-replay \
  --output /tmp/repeated.h3m
```

The replay record does not embed or fingerprint installation assets. Preserve
those files when investigating a case; changing resources can change output.
New reports use replay version 2 and record the optional town mask. Version 1
reports remain readable and preserve their former all-town water-zone policy.
Reports accept LF and CRLF. Both modes run natively without Wine or a retail
executable. Map output is gzip level 6 with a fixed timestamp; exact compressed
bytes need not match the game's compressor. Serialization comparisons use the
uncompressed H3M stream.

Map output is published only after generation, serialization and compression
finish. A failed attempt retains its replay report and removes temporary map
output, preserving an existing destination. IO errors are reported with a
nonzero exit status. The library's `OutputWorkspace` can instead write an
uncompressed stream to any `std::io::Write` destination.

Independent maps can share immutable `generation::Assets`, with one
`GenerationWorkspace` and `OutputWorkspace` per worker. Reuse those workspaces
for batches; keep one RNG per map and preserve serial stage/draw order.

The [implementation plan](../../docs/reconstruction/rust-rmg-plan.md) records
known discrepancies, measured allocations and ongoing comparison campaign.
