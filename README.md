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

**Windows `HEROES3.EXE`: 98.62% matched (MAX)** — 4,475 / 4,785 functions exact (93.5%), weighted by size over 1,999,585 bytes of code.

| Score | Functions exact | Weighted | Meaning                                        |
| :---- | --------------: | -------: | :--------------------------------------------- |
| CUR   |           4,465 |   98.60% | last measured score                            |
| MAX   |           4,475 |   98.62% | best result for each function's current source |
| HIST  |           4,522 |   98.94% | all-time peak across source revisions          |

MAX by module:

| Module       | Units | Functions exact MAX | Fuzzy MAX |
| :----------- | ----: | ------------------: | --------: |
| `game`       |   123 | 3768 / 3997 (94.3%) |    98.90% |
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

<!-- h3maped-match-score:start -->

**GOG Complete map editor `h3maped.exe`: 7.79% matched (MAX)** — 182 / 10,376 functions exact (1.8%), weighted by size over 940,876 bytes of code.

| Score | Functions exact | Weighted | Meaning                                        |
| :---- | --------------: | -------: | :--------------------------------------------- |
| CUR   |             182 |    7.79% | last measured score                            |
| MAX   |             182 |    7.79% | best result for each function's current source |
| HIST  |             182 |    7.79% | all-time peak across source revisions          |

MAX by module:

| Module        | Units | Functions exact MAX | Fuzzy MAX |
| :------------ | ----: | ------------------: | --------: |
| `rmg`         |     3 |   113 / 253 (44.7%) |    92.54% |
| `game`        |     5 |     48 / 96 (50.0%) |    91.94% |
| `zlib-1.1.3`  |     9 |     15 / 21 (71.4%) |    99.96% |
| `codec`       |     2 |       2 / 7 (28.6%) |    99.82% |
| `victor`      |     2 |       4 / 5 (80.0%) |    94.68% |
| `(unmatched)` |     — |    0 / 9,994 (0.0%) |      0.0% |

Excluded from the scores (generated or library code):

| Category              | Functions | Code (B) | Why excluded                                                       |
| :-------------------- | --------: | -------: | :----------------------------------------------------------------- |
| `EH unwind funclets`  |     4,533 |   55,776 | compiler EH unwind funclets; match with their parent function      |
| `CRT/C++ runtime`     |     1,693 |  209,899 | CRT/C++ runtime, named not matched (config/retail/runtime-map.tsv) |
| `init/cleanup thunks` |     2,356 |   23,808 | compiler-generated CRT initializer/cleanup bodies                  |
| `import thunks`       |        13 |       78 | FF 25 jumps through the IAT                                        |

**Data:** 0 / 16,670 referenced data objects claimed (placements and the image's own `DATA()` claims).

<!-- h3maped-match-score:end -->

Scores satisfy CUR ≤ MAX ≤ HIST. Editing a function resets its MAX to its new
CUR; HIST above MAX marks a lost peak worth recovering.

## Branches

```text
decomp-complete-4.0 (you are here) ----> decomp-loki-1.0
```

- [`decomp-complete-4.0`](https://github.com/sushi-shi/homm3-decomp/tree/decomp-complete-4.0#branches) — Complete 4.0 `HEROES3.EXE` (Sep 2000), VC6 SP3
- [`decomp-loki-1.0`](https://github.com/sushi-shi/homm3-decomp/tree/decomp-loki-1.0#branches) — Loki Linux 1.0 map editor `h3maped`, GCC 2.95.2

## Pinned executables

`HEROES3.EXE` is the target; the Dreamcast and Mac builds are evidence. Supply your own
copies; sizes and SHA-256 hashes are pinned in
[config/project.toml](config/project.toml).

- `HEROES3.EXE`: English Complete 4.0, engine 3.2 (MSVC 6.0, Sep 2000)
- `H3.EXE`: Dreamcast port, SH-4 with CodeView symbols (Aug 2000)
- `Heroes_III_raw.pef`: Classic Mac OS port, PowerPC CodeWarrior (Dec 2000)

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
