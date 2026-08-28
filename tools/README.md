# Heroes III resource oracle

This workspace is an independent, safe Rust view of the retail resource
formats. It follows the split used by the local HoMM2 and Gruntz projects:
small `no_std` codec crates at the bottom, and a `std` command-line tool for
files, zlib, reporting, and PNG output at the edge.

| Crate | Runtime boundary | Purpose |
| --- | --- | --- |
| `homm3-lod` | `no_std`, no allocation, no dependencies | Validate a LOD directory and borrow stored/zlib member bytes |
| `homm3-archive` | `no_std`, no allocation, no dependencies | Validate SND/VID indices and gzip envelopes |
| `homm3-def` | `no_std`, no allocation, no dependencies | Parse DEF groups/frames, iterate all four packet encodings, and safely blit every ordinary draw encoding |
| `homm3-resource` | `no_std`, no allocation, no dependencies | Parse PCX/PAL/FNT/MSK/TXT payloads and IFF/XMIDI envelopes |
| `homm3-map` | `no_std`, no allocation, no dependencies | Parse complete H3M/TUT streams and H3C campaign descriptors/maps |
| `homm3-save` | `no_std`, no allocation, no dependencies | Parse and exhaustively consume every GM/TGM/CGM revision accepted by retail Complete |
| `homm3-oracle` | `std` | Read files, inflate zlib/gzip, census corpora, inspect runs, and emit inspection PNGs |

The libraries contain no `unsafe` code. Callers own all input, decode, palette,
and destination buffers. The decoder returns structured errors for every bad
extent, offset, dimension, packet, rectangle, and output size it recognizes.
This keeps it useful as an oracle: it does not call the game's C++ parser or
share its pointer arithmetic.

## Build and test

From the repository root:

```sh
cd tools
cargo test --workspace --all-targets --offline
cargo test -p homm3-def --features cxx-parity --test cxx_parity --offline
cargo clippy --workspace --all-targets --features homm3-def/cxx-parity --offline -- -D warnings
cargo fmt --all -- --check
```

The ordinary tests include generated examples of stored and compressed LOD
members, compact and cropped DEF frames, all four frame encodings, clipping,
mirroring, malformed-input sweeps, and destination canaries. The opt-in
`cxx-parity` feature compiles the actual in-tree `CSpriteFrame::Draw`,
`DrawTile`, and `DrawAdvObjImpl` through a small test-only adapter. It compares
512 generated draws for each encoding (2,048 total), including clipped,
mirrored, transparent-fill, and multi-cell adventure cases. The feature does
not change any Rust library or its `no_std` contract.

An additional test differentials every installed DEF frame when
`HOMM3_DEF_CORPUS` is a platform path-list of LODs:

```sh
HOMM3_DEF_DIALECT=known-interleaved \
  HOMM3_DEF_CORPUS=/path/H3sprite.lod:/path/H3ab_spr.lod \
  cargo test -p homm3-def --features cxx-parity \
  --test cxx_corpus --offline -- --nocapture
```

The current four-LOD Steam corpus is clean for all 39,939 frames. The original
US disc corpora are independently clean for every frame in RoE 1.0 (29,393),
RoE 1.1 (29,393), the Armageddon's Blade supplement (5,599), and Shadow of
Death (34,324). This is candidate parity, while the retail x86 functions remain
the independent authority for reconstructed C++ semantics and packet grammar.
The dialect variable is explicit: omit it or set it to `retail` for the pinned
loader; `known-interleaved` enables only the named `SGTWMTA/B` manifest. The
old value `steam` remains an accepted compatibility alias, but the RoE 1.0 CD
proves the layout belongs to the initial release rather than Steam.

## Corpus commands

No retail data is checked in. Point the CLI at legally obtained archives:

```sh
cargo run -p homm3-oracle --offline -- \
  --lod /path/to/H3sprite.lod census

# Select only the two admitted interleaved-header members.
cargo run -p homm3-oracle --offline -- \
  --lod /path/to/H3sprite.lod --def-dialect known-interleaved census

cargo run -p homm3-oracle --offline -- \
  --lod /path/to/H3bitmap.lod resources

cargo run -p homm3-oracle --offline -- \
  --snd /path/to/Heroes3.snd --vid /path/to/VIDEO.VID containers

cargo run -p homm3-oracle --offline -- \
  --map /path/to/Maps maps

cargo run -p homm3-oracle --offline -- \
  --save /path/to/Games saves

cargo run -p homm3-oracle --offline -- \
  --lod /path/to/H3sprite.lod list --extension .def

cargo run -p homm3-oracle --offline -- \
  --lod /path/to/H3sprite.lod tokens AH00_E.DEF --group 0 --frame 0

cargo run -p homm3-oracle --offline -- \
  --lod /path/to/H3sprite.lod dump AH00_E.DEF /tmp/ah00-e.png \
  --group 0 --frame 0
```

Repeat archive/path options to census the base and expansion sets together.
`census` validates every `.DEF`; `resources` validates every recognized LOD
payload, including both the direct compact RoE 1.0/1.1 H3C header generation
and the later gzipped full header, plus every complete embedded map;
`containers` checks SND/VID tables and member extents; and `maps` validates
gzip plus every header, world, terrain, object, quest, and timed-event byte for
each H3M/TUT below a file or directory. `saves` validates gzip and every byte
of `.GM1`…`.GM8`, `.TGM`, and `.CGM` streams for the retail-accepted revisions
16–18 and 25–42. Revisions 1–15, 19–24, and values above 42 fail at the same
version boundary as the retail loader.
Every command exits nonzero with archive/member context on a failure. `tokens`
preserves the important difference between encoded fills and literal palette
indices.

`dump` is a storage inspection image, not a claim about every retail draw
effect. It places the decoded crop in full-frame coordinates and maps every
stored index through the DEF palette; pixels outside the crop are transparent.
Retail gives some encoded control runs context-specific shadow, outline, or
flag behavior, which belongs in renderer-specific parity work.

The default DEF dialect always follows the pinned retail loader.
`known-interleaved` is an explicit filename manifest for `SGTWMTA.DEF` and
`SGTWMTB.DEF`; it is not a content heuristic and never changes the
interpretation of any other member.

## Evidence boundary

Retail `HEROES3.EXE` remains authoritative. Binary-proven container and
renderer facts, Dreamcast source-shape facts, and current unknowns are recorded
in [the format note](../docs/lod-def-formats.md). A passing Rust census is a
strong consistency check, not permission to override conflicting retail bytes.
The exhaustive inventory and per-family closure status live in
[the resource-format matrix](../docs/resource-format-matrix.md).
