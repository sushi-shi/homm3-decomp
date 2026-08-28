# homm3-decomp

Binary-matching decompilation of **Heroes of Might and Magic III Complete**
(`HEROES3.EXE`, New World Computing, 2000). The goal is to recover the C++ structure and
behavior and, where retail evidence permits, reproduce the original code, data, and
relocations with the **MSVC 6.0 SP3** toolchain. Retail executable bytes and RVAs are
authoritative. [objdiff](https://github.com/encounter/objdiff) is a useful comparison and
navigation surface, not proof of correctness.

This repository does **not** contain the original game's executable or resources. Supply a
legally obtained `HEROES3.EXE` locally to initialize the matching workspace.

<!-- match-score:start -->

**Executable matched: 60.86%** — fuzzy-weighted bytes over all 1,997,013 unfiltered bytes.

**Match score** — 2,535 / 4,765 functions exact (53.2%) across the full engine (3097 in linked units).

| Module        | Units |     Functions exact |   Fuzzy | Fuzzy Max |
| :------------ | ----: | ------------------: | ------: | --------: |
| `game`        |   117 | 2466 / 3028 (81.4%) |  92.80% |    92.80% |
| `zlib-1.1.3`  |    14 |    69 / 69 (100.0%) | 100.00% |   100.00% |
| `(unmatched)` |     — |    0 / 1,668 (0.0%) |    0.0% |      0.0% |

_Excluded from the % above — generated/library code, not independent reconstruction targets:_

| Category              | Functions | Code (B) | Why excluded                                                       |
| :-------------------- | --------: | -------: | :----------------------------------------------------------------- |
| `EH unwind funclets`  |     5,125 |   53,151 | compiler EH unwind funclets; match with their parent function      |
| `CRT/C++ runtime`     |       915 |  110,788 | CRT/C++ runtime, named not matched (config/retail-runtime-map.tsv) |
| `init/cleanup thunks` |     1,119 |   94,433 | .CRT$XCU dynamic-initializer bodies (compiler-generated)           |
| `import thunks`       |        27 |      162 | FF 25 jumps through the IAT                                        |

<!-- match-score:end -->

## Pinned target

The canonical image is the **English GOG Heroes III Complete 4.0 (engine 3.2)** executable:

```
file        HEROES3.EXE
size        2,732,032 bytes
sha256      057c9d88e7206f6669a4615de2c6e02ab6c4e2d570a9e2badf07fe0bd6247274
base        0x00400000 (fixed; no base-relocation directory)
entry       VA 0x0061A2B4
.text       RVA 0x001000, 0x238612 bytes
.rdata      RVA 0x23a000 (IAT + import descriptors live here; no .idata section)
timestamp   8 September 2000, built by MSVC 6.0
```

## Quickstart

Provide the retail exe via `HOMM3_EXE=/path/to/HEROES3.EXE` — it is
sha256-verified against the pinned pressing and copied into `build/orig/`
(a wrong file is refused, never silently used).

```sh
nix develop .#build   # VC6 SP3 under wine + the tools
homm3 init            # ONE-TIME: toolchain tarball (pinned release, SHA-256-verified),
                      # wine prefix, smoke compile through the real cc_wrap path
homm3 build           # configure + ninja: compile every manifest unit
homm3 link            # OPT-IN candidate link (layout study; the EXE is not runnable)
homm3 clean           # nuke build/ entirely; `homm3 init` restores it
```

## Rust resource oracle

`tools/` contains an independent resource parser and rendering oracle following
the local HoMM2 and Gruntz split: allocation-free, dependency-free `no_std`
libraries for format logic, plus a `std` CLI for files, compression, corpus
reports, and PNG inspection. Current coverage includes LOD/SND/VID archives,
DEF sprites, engine-owned LOD payloads, IFF/XMIDI envelopes, concatenated H3C
campaigns, all three retail H3M/TUT header generations, and the map stream
through every terrain, object-template, placed-object, quest, and timed-event
record. Every GM/TGM/CGM save revision accepted by retail Complete (16–18 and
25–42) is also covered by a separate allocation-free parser; the local Steam
install has no save corpus, so real-file validation remains open. It includes
deterministic malformed-input coverage and an opt-in differential test against
the actual reconstructed C++ renderers for every DEF encoding. The generated
gate covers 2,048 draws, and the local four-LOD Steam gate agrees on all 39,939
frames. Independent gates over the original US discs agree on every frame in
RoE 1.0 (29,393), RoE 1.1 (29,393), the Armageddon's Blade supplement (5,599),
and Shadow of Death (34,324), and validate all 251 standalone maps. They date
the named interleaved DEF layout to RoE 1.0 and add support for its direct,
compact version-1 H3C campaign headers. Retail resources are supplied locally
and are never added to the repository.

See [tools/README.md](tools/README.md) for commands and
[the resource-format matrix](docs/resource-format-matrix.md) for the evidence
boundary and remaining semantic work.

## License

Project-authored reconstruction source and tooling are dedicated to the public
domain under [CC0 1.0](LICENSE), to the extent the contributors can do so.
Files carrying separate copyright or license notices — notably everything under
`vendor/` — retain those terms. No binary game assets are stored in this
repository. Thanks to [NH3API](https://github.com/void2012/NH3API)
for labelling the executable.
