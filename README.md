# homm3-decomp

> **Work in progress.** Most functions of `HEROES3.EXE` match; the remaining
> functions and data are being reconstructed.

C++ reconstruction of **Heroes of Might and Magic III Complete**
(`HEROES3.EXE`, New World Computing, 2000), built with the original Visual C++
6.0 SP3 toolchain under Wine. The Dreamcast and Classic Mac ports are references
for source structure. Retail bytes are authoritative. Supply your own
executables.

## Match status

<!-- match-score:start -->

**Windows `HEROES3.EXE`: 98.61% matched (MAX)** — 4,474 / 4,785 functions exact (93.5%), weighted by size over 1,999,585 bytes of code.

| Score | Functions exact | Weighted | Meaning                                        |
| :---- | --------------: | -------: | :--------------------------------------------- |
| CUR   |           4,466 |   98.59% | last measured score                            |
| MAX   |           4,474 |   98.61% | best result for each function's current source |
| HIST  |           4,522 |   98.94% | all-time peak across source revisions          |

MAX by module:

| Module       | Units | Functions exact MAX | Fuzzy MAX |
| :----------- | ----: | ------------------: | --------: |
| `game`       |   123 | 3767 / 3997 (94.2%) |    98.88% |
| `rmg`        |     3 |   307 / 369 (83.2%) |    94.94% |
| `network`    |     4 |   275 / 281 (97.9%) |    99.40% |
| `zlib-1.1.3` |    14 |    69 / 69 (100.0%) |   100.00% |
| `codec`      |     4 |     42 / 43 (97.7%) |    99.92% |
| `victor`     |     4 |     14 / 26 (53.8%) |    86.18% |

Excluded from the scores (generated or library code):

| Category              | Functions | Code (B) | Why excluded                                                       |
| :-------------------- | --------: | -------: | :----------------------------------------------------------------- |
| `EH unwind funclets`  |     5,125 |   53,151 | compiler EH unwind funclets; match with their parent function      |
| `CRT/C++ runtime`     |       912 |  110,461 | CRT/C++ runtime, named not matched (config/retail/runtime-map.tsv) |
| `init/cleanup thunks` |     1,173 |   95,322 | compiler-generated CRT initializer/cleanup bodies                  |
| `import thunks`       |        27 |      162 | FF 25 jumps through the IAT                                        |

<!-- match-score:end -->

<!-- mac-match-score:start -->

**Mac reference `Heroes_III_raw.pef`: 58.50% matched** — 588 / 1,510 paired functions exact, over 443,528 compared bytes (last `homm3 mac build` checkpoint).

<!-- mac-match-score:end -->

Scores satisfy CUR ≤ MAX ≤ HIST. Editing a function resets its MAX to its new
CUR; HIST above MAX marks a lost peak worth recovering.

## Branches

```text
decomp-complete-4.0 (you are here) ----> decomp-loki-1.0
```

- [`decomp-complete-4.0`](https://github.com/sushi-shi/homm3-decomp/tree/decomp-complete-4.0#branches) — Complete 4.0 `HEROES3.EXE` (Sep 2000), VC6 SP3
- [`decomp-loki-1.0`](https://github.com/sushi-shi/homm3-decomp/tree/decomp-loki-1.0#branches) — Loki Linux 1.0 map editor `h3maped`, GCC 2.95.2

## Pinned executables

The Windows executable is the matching target. The Dreamcast and Mac builds
are references; they are not matched as games. Hashes are also pinned in
[config/project.toml](config/project.toml).

| Role | File | Size (B) | SHA-256 |
| :--- | :--- | -------: | :------ |
| Target: English Complete 4.0 (engine 3.2), MSVC 6.0, Sep 2000 | `HEROES3.EXE` | 2,732,032 | `057c9d88e7206f6669a4615de2c6e02ab6c4e2d570a9e2badf07fe0bd6247274` |
| Reference: Dreamcast port, SH-4 with CodeView symbols, Aug 2000 | `H3.EXE` | 8,425,752 | `cdbc7e75bd7d057171fa12b728aaaee01c1db133fff350b034950dd21dd07736` |
| Reference: Classic Mac OS port, PowerPC CodeWarrior, Dec 2000 | `Heroes_III_raw.pef` | 3,418,835 | `650be8880cfda81ffa7704ce3bcdb9c5a6528f67afdf77c0c0c63e8259250d86` |

## Quickstart

With Nix flakes enabled, the three executables and the pinned CodeWarrior
tools, run from the repository root. `homm3 init` verifies the inputs and both
compilers and sets up Wine; `homm3 build` compiles and compares every Windows
module and runs the checks. It does not yet produce a playable game.

```sh
nix develop .#build
gh auth login        # needed for the toolchain download
HOMM3_EXE=/absolute/path/to/HEROES3.EXE \
HOMM3_DREAMCAST_EXE=/absolute/path/to/H3.EXE \
HOMM3_MAC_EXE=/absolute/path/to/Heroes_III_raw.pef \
HOMM3_MAC_TOOLCHAIN=/absolute/path/to/CodeWarrior/tools \
  homm3 init
homm3 build
```

Retail inputs, the toolchain, Wine state and generated reports stay in ignored
`build/`. The shell also provides clangd and keeps `compile_commands.json` up
to date for editors.

## Improve a function

Pick a function whose MAX is below 100%, read the retail assembly and the
Dreamcast evidence, edit the C++, and rebuild its unit. The example uses
`0x00524dd0` from `src/philai.cpp`:

```sh
homm3 status functions
homm3 sema disasm 0x00524dd0
homm3 dreamcast show 0x00524dd0
homm3 dreamcast asm 0x00524dd0 --blocks
homm3 build --fast philai
homm3 sema diff 0x00524dd0 --summary
homm3 status update --write-readme   # bank the scores and regenerate this README
```

`homm3 mac show`, `mac diff` and `mac calls` show a function's Mac counterpart;
`homm3 mac build philai` scores the unit's Mac pairs. `homm3 status check`
reports score changes and `homm3 status merge-baseline` resolves a conflicted
score ledger.

## Data matching

`homm3 compare` compares existing objects without rebuilding.
`homm3 build --data` adds byte accounting: unclaimed bytes, overlapping ranges
and initializer differences.

## Documentation

- [Documentation index](docs/README.md) and
  [reconstruction debt](docs/todos/reconstruction_debt.md)
- [Data matching](docs/tooling/data-matching.md) and the
  [Mac tooling guide](docs/tooling/mac-matching-roadmap.md)
- Contributor rules and the full evidence pass are in [AGENTS.md](AGENTS.md)

## License

Project-authored source and tooling use [CC0 1.0](LICENSE). Files with their
own notices, notably everything under `vendor/`, keep their terms; retail
inputs, compiler binaries and game assets are excluded.

## Thanks

[NH3API](https://github.com/void2012/NH3API) documented game structures and
their layouts, and supplied naming references beyond the Dreamcast symbols.
