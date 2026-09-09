# homm3-decomp

Binary-matching decompilation of **Heroes of Might and Magic III Complete**
(`HEROES3.EXE`, New World Computing, 2000). The goal is to recover the C++ structure and
behavior and, where retail evidence permits, reproduce the original code, data, and
relocations with the **MSVC 6.0 SP3** toolchain. Retail executable bytes and RVAs are
authoritative. [objdiff](https://github.com/encounter/objdiff) is a useful comparison and
navigation surface, not proof of correctness.

This repository does **not** contain either game's executable or resources. To match
the game, supply your own legally obtained retail `HEROES3.EXE` and Dreamcast `H3.EXE`.

<!-- match-score:start -->

**Executable matched: 96.38%** — fuzzy-weighted bytes over all 1,998,942 unfiltered bytes.

**Match score** — 4,073 / 4,764 functions exact (85.5%) across the full engine (4764 in linked units).

**Function exact MAX** — 4,094 / 4,764 current implementations (85.9%) have reached 100%.

| Module       | Units |     Functions exact |  Function exact MAX |   Fuzzy | Fuzzy Max |
| :----------- | ----: | ------------------: | ------------------: | ------: | --------: |
| `game`       |   138 | 4004 / 4695 (85.3%) | 4025 / 4695 (85.7%) |  96.34% |    96.56% |
| `zlib-1.1.3` |    14 |    69 / 69 (100.0%) |    69 / 69 (100.0%) | 100.00% |   100.00% |

_Excluded from the % above — generated/library code, not independent reconstruction targets:_

| Category              | Functions | Code (B) | Why excluded                                                       |
| :-------------------- | --------: | -------: | :----------------------------------------------------------------- |
| `EH unwind funclets`  |     5,125 |   53,151 | compiler EH unwind funclets; match with their parent function      |
| `CRT/C++ runtime`     |       913 |  110,536 | CRT/C++ runtime, named not matched (config/retail-runtime-map.tsv) |
| `init/cleanup thunks` |     1,119 |   94,433 | .CRT$XCU dynamic-initializer bodies (compiler-generated)           |
| `import thunks`       |        27 |      162 | FF 25 jumps through the IAT                                        |

<!-- match-score:end -->

The score ledger always keeps `CUR <= MAX <= HIST`. CUR is the latest full
build; MAX is the best score observed for the function's current source hash;
HIST is its all-time peak across source revisions. Tooling prioritizes MAX.
Unrelated CUR dips keep MAX and are silent. A function's own hash change resets
MAX to its new CUR; a lower MAX is reported, but is not a build failure.
`HIST > MAX` identifies historical peaks worth investigating.

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

From the repository root, supply both executable paths:

```sh
nix develop .#build
HOMM3_EXE=/absolute/path/to/HEROES3.EXE \
HOMM3_DREAMCAST_EXE=/absolute/path/to/H3.EXE \
  homm3 init

homm3 build           # compile, delink, compare, checkpoint, run gates
homm3 link            # optional layout study; the EXE is not runnable
```

You can also pass `--exe PATH` and `--dreamcast-exe PATH` to `homm3 init`;
these override the environment variables.

Initialization verifies both files' size and SHA-256, copies them into ignored
`build/orig/HEROES3.EXE` and `build/orig/dreamcast/H3.EXE`, and reads the Dreamcast
executable's embedded NB11 debug symbols. The Dreamcast input is pinned to
**8,425,752 bytes**, SHA-256
`cdbc7e75bd7d057171fa12b728aaaee01c1db133fff350b034950dd21dd07736`.

`init` also configures the build, downloads and verifies the pinned VC6 SP3 toolchain
if missing, initializes Wine, and smoke-compiles through the normal compiler wrapper.
Toolchain downloads use authenticated `gh`.

Subsequent commands use the staged executables and recheck their bytes; the original
paths need not remain configured. Re-running `homm3 init` verifies and reuses an existing
setup. `homm3 clean` removes all of `build/`, including the staged copies; supply both
paths again when initializing after a clean.

Clangd works with the existing Neovim/CoC setup; hover and SDK definition lookup
have been verified. `compile_commands.json` refreshes automatically on shell entry,
configure, and build.

Generate a browsable Dreamcast source tree from the embedded debug symbols:

```sh
homm3 dreamcast structure                         # all modules
homm3 dreamcast structure --module cursor --asm --output /tmp/dc-cursor
```

The default output is `evidence/dreamcast/structure/README.md`, with annotated
C++ stubs and JSON for each compiland, plus a type catalogue. It includes decoded
signatures, scoped locals, recorded scope nesting, source-line spans and gaps,
inline evidence, and inferred SH4 control flow. These are Dreamcast reference
facts; the generated files are not build inputs. See
[the structure exporter documentation](docs/dc-line-tables.md#generated-source-structure)
for the format and its evidence limits.

## License

Project-authored reconstruction source and tooling are dedicated to the public
domain under [CC0 1.0](LICENSE), to the extent the contributors can do so.
Files carrying separate copyright or license notices — notably everything under
`vendor/` — retain those terms. No binary game assets are stored in this
repository. Thanks to [NH3API](https://github.com/void2012/NH3API)
for labelling the executable.
