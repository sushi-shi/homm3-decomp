# homm3-decomp

Binary-matching decompilation of **Heroes of Might and Magic III Complete**
(New World Computing, 2000). The goal is C++ source that compiles with the
original MSVC 6.0 to the same machine code as the retail `HEROES3.EXE`, and
that reads like the original developers wrote it.

This repository does **not** contain the game's executables or resources.
Supply your own copies of the three [pinned executables](#pinned-executables)
and the pinned CodeWarrior tools.

<!-- match-score:start -->

**Windows `HEROES3.EXE`: 98.62% matched (MAX)** — 4,472 / 4,785 functions exact (93.5%), weighted by size over 1,999,585 bytes of code.

| Score | Functions exact | Weighted | Meaning                                        |
| :---- | --------------: | -------: | :--------------------------------------------- |
| CUR   |           4,460 |   98.57% | last measured score                            |
| MAX   |           4,472 |   98.62% | best result for each function's current source |
| HIST  |           4,517 |   98.93% | all-time peak across source revisions          |

MAX by module:

| Module       | Units | Functions exact MAX | Fuzzy MAX |
| :----------- | ----: | ------------------: | --------: |
| `game`       |   123 | 3771 / 3997 (94.3%) |    98.90% |
| `rmg`        |     3 |   307 / 369 (83.2%) |    95.06% |
| `network`    |     4 |   275 / 281 (97.9%) |    99.40% |
| `zlib-1.1.3` |    14 |    69 / 69 (100.0%) |   100.00% |
| `codec`      |     4 |     36 / 43 (83.7%) |    98.61% |
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

<!-- loki-match-score:start -->

**Loki Linux `h3maped` 1.0 (map editor, GCC 2.95.2): 7,432 / 7,432 functions exact (100.00%) &middot; 100.00% fuzzy (MAX).**

A separate image with its own scores; `homm3 loki build --bank` banks `config/loki/match_baseline.tsv`, and this block renders from it.

| Phase | Objects | Functions exact MAX | Fuzzy MAX |
| :---- | ------: | ------------------: | --------: |
| engine (shared with the game) | 29 | 1,001 / 1,001 (100.0%) | 100.00% |
| editor | 74 | 6,431 / 6,431 (100.0%) | 100.00% |

_CUR / MAX / HIST: 7,432 / 7,432 / 7,432 exact &middot; 100.00% / 100.00% / 100.00% fuzzy, weighted by size. Project functions only: the 3,120 `.text` functions and 4,312 kept linkonce bodies of the 103 GCC 2.95.2 project objects (103 built units); GTK+/glib/libglade/libxml/zlib and libstdc++ are excluded._

<!-- loki-match-score:end -->

Scores always satisfy CUR ≤ MAX ≤ HIST. Editing a function resets its MAX to
its new CUR; other CUR dips leave MAX alone. HIST above MAX marks a lost peak
worth recovering. `homm3 status check` reports score changes, and
`homm3 status merge-baseline` resolves a conflicted score ledger.

See the [documentation](docs/README.md) and the
[reconstruction debt checklist](docs/todos/reconstruction_debt.md).

## Pinned executables

The Windows executable is the matching target. The Dreamcast and Mac builds
are references used to recover source structure; they are not matched as games.

```
role        matching target
file        HEROES3.EXE
size        2,732,032 bytes
sha256      057c9d88e7206f6669a4615de2c6e02ab6c4e2d570a9e2badf07fe0bd6247274
format      PE32, x86, MSVC 6.0
built       2000-09-08
source      English GOG Heroes III Complete 4.0 (engine 3.2)
```

```
role        reference: debug symbols and source line tables
file        H3.EXE
size        8,425,752 bytes
sha256      cdbc7e75bd7d057171fa12b728aaaee01c1db133fff350b034950dd21dd07736
format      PE32, SH-4 (Windows CE), CodeView debug information
built       2000-08-11
source      Dreamcast port
```

```
role        reference: lightly optimized helper and call structure
file        Heroes_III_raw.pef
size        3,418,835 bytes
sha256      650be8880cfda81ffa7704ce3bcdb9c5a6528f67afdf77c0c0c63e8259250d86
format      PEF, PowerPC, CodeWarrior
built       2000-12-08
source      Classic Mac OS port
```

## Quickstart

You need Nix with flakes enabled, the three executables above, and the pinned
CodeWarrior tools. From the repository root:

```sh
nix develop .#build
gh auth login        # needed for the toolchain download
HOMM3_EXE=/absolute/path/to/HEROES3.EXE \
HOMM3_DREAMCAST_EXE=/absolute/path/to/H3.EXE \
HOMM3_MAC_EXE=/absolute/path/to/Heroes_III_raw.pef \
HOMM3_MAC_TOOLCHAIN=/absolute/path/to/CodeWarrior/tools \
  homm3 init

homm3 build          # compile and compare every Windows module, run the checks
```

`homm3 init` verifies the executables and both compilers and sets up Wine.
The build compiles and compares the reconstructed code; it does not yet produce
a playable game.

### IDE setup

Launch your editor from the development shell and open the repository root
with clangd enabled. The shell provides clangd and keeps
`compile_commands.json` up to date. Run `homm3 init` first so the compiler
headers are available for code navigation.

## Improve a function

The example uses `0x00524dd0` from `src/philai.cpp`; substitute your target.

1. Pick a function whose MAX is below 100%:

   ```sh
   homm3 status functions
   ```

2. Read the evidence before editing: the retail assembly, then the Dreamcast
   dossier and its source-line blocks.

   ```sh
   homm3 sema disasm 0x00524dd0
   homm3 dreamcast show 0x00524dd0
   homm3 dreamcast asm 0x00524dd0 --blocks
   ```

3. Edit the C++, rebuild its unit, and inspect what still differs:

   ```sh
   homm3 build --fast philai
   homm3 sema diff 0x00524dd0 --summary
   ```

4. Repeat until the function matches, then bank the scores, regenerate this
   README, and commit:

   ```sh
   homm3 status update --write-readme
   ```

If the function has a Mac counterpart, `homm3 mac show 0x00524dd0`,
`homm3 mac diff 0x00524dd0` and `homm3 mac calls 0x00524dd0` show its
CodeWarrior body, byte differences and call targets. `homm3 mac build philai`
scores the unit's Mac pairs; run it occasionally, not on every edit. See the
[Mac tooling guide](docs/tooling/mac-matching-roadmap.md).

The [matching guide](AGENTS.md) covers the full evidence pass and the
reconstruction rules.

## Data matching

`homm3 compare` compares existing objects without rebuilding.
`homm3 build --data` adds byte accounting: unclaimed bytes, overlapping ranges
and initializer differences. See [data matching](docs/tooling/data-matching.md).

## License

Project-authored reconstruction source and tooling are dedicated to the public
domain under [CC0 1.0](LICENSE), to the extent the contributors can do so.
Files carrying separate copyright or license notices — notably everything under
`vendor/` — retain those terms. No binary game assets are stored in this
repository.

## Thanks

Thanks to [NH3API](https://github.com/void2012/NH3API) for documenting game
structures and their layouts, and for naming references that supplement
the Dreamcast debug symbols.
