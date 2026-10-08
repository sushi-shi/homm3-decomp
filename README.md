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

**Windows `HEROES3.EXE`: 98.71% matched (MAX)** — 4,497 / 4,785 functions exact (94.0%), weighted by size over 1,999,585 bytes of code.

| Score | Functions exact | Weighted | Meaning                                        |
| :---- | --------------: | -------: | :--------------------------------------------- |
| CUR   |           4,487 |   98.69% | last measured score                            |
| MAX   |           4,497 |   98.71% | best result for each function's current source |
| HIST  |           4,535 |   98.98% | all-time peak across source revisions          |

MAX by module:

| Module       | Units | Functions exact MAX | Fuzzy MAX |
| :----------- | ----: | ------------------: | --------: |
| `game`       |   124 | 3778 / 3998 (94.5%) |    98.95% |
| `rmg`        |     3 |   309 / 369 (83.7%) |    95.13% |
| `network`    |     4 |   274 / 280 (97.9%) |    99.40% |
| `zlib-1.1.3` |    14 |    69 / 69 (100.0%) |   100.00% |
| `codec`      |     4 |    43 / 43 (100.0%) |   100.00% |
| `victor`     |     5 |     24 / 26 (92.3%) |    98.05% |

Library and compiler-generated code (outside the scores; each function verified against what produced it):

| Category              | Functions | Verified | Code (B) | Status                                   | How verified                                                                                            |
| :-------------------- | --------: | -------: | -------: | :--------------------------------------- | :------------------------------------------------------------------------------------------------------ |
| `CRT/C++ runtime`     |       912 |      912 |  110,461 | library, verified                        | bytes and relocations equal pinned VC6 SP3 LIBCMT/LIBCPMT members (config/retail/runtime-functions.tsv) |
| `EH unwind funclets`  |     5,125 |    4,558 |   53,151 | compiler-generated, verified with parent | parent's `.text$x` COMDAT: bytes and every relocation target (library parents: their library section)   |
| `init/cleanup thunks` |     1,173 |    1,166 |   95,322 | compiler-generated, verified             | source-emitted CRT bodies, bytes and named relocations                                                  |
| `import thunks`       |        27 |       26 |      162 | linker-generated, verified               | `FF 25` through a named IAT slot (and the pinned import library where one exists)                       |

<!-- match-score:end -->

<!-- mac-match-score:start -->

**Mac reference `Heroes_III_raw.pef`: 58.50% matched** — 588 / 1,510 paired functions exact, over 443,528 compared bytes (last `homm3 mac build` checkpoint).

<!-- mac-match-score:end -->

<!-- h3maped-match-score:start -->

**GOG Complete map editor `h3maped.exe`: 15.46% matched (MAX)** — 378 / 8,255 functions exact (4.6%), weighted by size over 910,134 bytes of code.

| Score | Functions exact | Weighted | Meaning                                        |
| :---- | --------------: | -------: | :--------------------------------------------- |
| CUR   |             373 |   15.44% | last measured score                            |
| MAX   |             378 |   15.46% | best result for each function's current source |
| HIST  |             378 |   15.46% | all-time peak across source revisions          |

MAX by module:

| Module        | Units | Functions exact MAX | Fuzzy MAX |
| :------------ | ----: | ------------------: | --------: |
| `rmg`         |     3 |   174 / 287 (60.6%) |    92.19% |
| `game`        |    14 |   131 / 239 (54.8%) |    85.56% |
| `zlib-1.1.3`  |    12 |     43 / 56 (76.8%) |    99.97% |
| `codec`       |     3 |     18 / 35 (51.4%) |    93.16% |
| `victor`      |     4 |     12 / 16 (75.0%) |    97.68% |
| `(unmatched)` |     — |    0 / 7,622 (0.0%) |      0.0% |

Library and compiler-generated code (outside the scores; each function verified against what produced it):

| Category              | Functions | Verified | Code (B) | Status                                   | How verified                                                                                          |
| :-------------------- | --------: | -------: | -------: | :--------------------------------------- | :---------------------------------------------------------------------------------------------------- |
| `CRT/C++ runtime`     |     1,858 |        — |  220,999 | excluded                                 | CRT/C++ runtime, named not matched (config/retail/runtime-map.tsv)                                    |
| `EH unwind funclets`  |     4,533 |        0 |   55,766 | compiler-generated, verified with parent | parent's `.text$x` COMDAT: bytes and every relocation target (library parents: their library section) |
| `init/cleanup thunks` |     2,356 |        2 |   23,808 | compiler-generated, verified             | source-emitted CRT bodies, bytes and named relocations                                                |
| `import thunks`       |        13 |        0 |       78 | linker-generated, verified               | `FF 25` through a named IAT slot (and the pinned import library where one exists)                     |

**Data:** 142 / 16,670 referenced data objects claimed (placements and the image's own `DATA()` claims).

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
- `h3maped.exe`: English Complete 4.0 map editor (MSVC 6.0 SP3, MFC 4.2, Sep 2000)
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
