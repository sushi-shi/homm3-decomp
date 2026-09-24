# homm3-decomp

Binary-matching decompilation of **Heroes of Might and Magic III Complete**
(`HEROES3.EXE`, New World Computing, 2000). The goal is to recover the C++ structure and
behavior.

This repository does **not** contain the game's executables or resources. To match
the game, supply your own copies of retail `HEROES3.EXE`, Dreamcast `H3.EXE`,
and the Classic Mac PowerPC PEF, plus the pinned CodeWarrior tools.

<!-- match-score:start -->

**Executable MAX: 97.40%** — weighted by function size across 1,999,142 bytes of code included in matching.

**Function exact MAX** — 4,321 / 4,768 current implementations (90.6%) have reached 100%.

**CUR diagnostics** — 4,278 / 4,768 functions exact (89.7%) in this build (4768 in linked units). Compiler-context dips with held MAX do not reduce matching progress.

| Module       | Units | Functions exact CUR |  Function exact MAX | Fuzzy CUR | Fuzzy MAX |
| :----------- | ----: | ------------------: | ------------------: | --------: | --------: |
| `game`       |   123 | 3610 / 3991 (90.5%) | 3642 / 3991 (91.3%) |    97.41% |    97.64% |
| `rmg`        |     3 |   291 / 368 (79.1%) |   301 / 368 (81.8%) |    93.19% |    94.37% |
| `network`    |     4 |   268 / 280 (95.7%) |   269 / 280 (96.1%) |    97.75% |    98.11% |
| `zlib-1.1.3` |    14 |    69 / 69 (100.0%) |    69 / 69 (100.0%) |   100.00% |   100.00% |
| `codec`      |     4 |     35 / 43 (81.4%) |     35 / 43 (81.4%) |    94.70% |    94.70% |
| `victor`     |     4 |      5 / 17 (29.4%) |      5 / 17 (29.4%) |    85.40% |    85.40% |

_Excluded from the % above — generated/library code, not independent reconstruction targets:_

| Category              | Functions | Code (B) | Why excluded                                                       |
| :-------------------- | --------: | -------: | :----------------------------------------------------------------- |
| `EH unwind funclets`  |     5,125 |   53,151 | compiler EH unwind funclets; match with their parent function      |
| `CRT/C++ runtime`     |       913 |  110,536 | CRT/C++ runtime, named not matched (config/retail/runtime-map.tsv) |
| `init/cleanup thunks` |     1,119 |   94,433 | .CRT$XCU dynamic-initializer bodies (compiler-generated)           |
| `import thunks`       |        27 |      162 | FF 25 jumps through the IAT                                        |

<!-- match-score:end -->

<!-- mac-match-score:start -->

**Lightly optimized Classic Mac PowerPC reference (last full checkpoint):** 13 / 66 admitted functions exact; 52.58% of 59,304 compared bytes match. The admitted-pair count is coverage, not the whole Mac game.

<!-- mac-match-score:end -->

The score ledger always keeps `CUR <= MAX <= HIST`. CUR is the latest full
build; MAX is the best score observed for the function's current source hash;
HIST is its all-time peak across source revisions. Tooling prioritizes MAX.
Unrelated CUR dips keep MAX and are silent. A function's own hash change resets
MAX to its new CUR; a lower MAX is reported, but is not a build failure.
`HIST > MAX` identifies historical peaks worth investigating.

See the [documentation](docs/README.md) and [reconstruction debt checklist](docs/todos/reconstruction_debt.md).

## Pinned target

The canonical image is the **English GOG Heroes III Complete 4.0 (engine 3.2)** executable:

```
file        HEROES3.EXE
size        2,732,032 bytes
sha256      057c9d88e7206f6669a4615de2c6e02ab6c4e2d570a9e2badf07fe0bd6247274
timestamp   8 September 2000, built by MSVC 6.0
```

The Dreamcast debug-symbol reference is pinned separately:

```
file        H3.EXE
size        8,425,752 bytes
sha256      cdbc7e75bd7d057171fa12b728aaaee01c1db133fff350b034950dd21dd07736
```

The **Classic Mac OS PowerPC PEF** is a lightly optimized reference build
used to recover source structure for Windows matching:

```
file        Heroes_III_raw.pef
size        3,418,835 bytes
sha256      650be8880cfda81ffa7704ce3bcdb9c5a6528f67afdf77c0c0c63e8259250d86
format      Joy!peffpwpc (CodeWarrior PowerPC)
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

homm3 build          # build all modules, compare Windows and Mac bytes, and run checks
```

`homm3 init` verifies all executables and both compilers, and sets up Wine.
The build compiles and compares reconstructed code; it does not yet produce
a playable game.

### IDE setup

Enable your editor's clangd integration and open the repository root.
The development shell provides clangd and generates `compile_commands.json`;
launch your editor from that shell so it can find the tools. The compilation
database refreshes on shell entry and during builds. Run `homm3 init` first
to set up the compiler headers needed for code navigation.

### Improve a function

1. Run `homm3 status functions` and choose a function whose MAX is below 100%.
2. Inspect its retail assembly and Dreamcast source evidence before editing.
   For example, `0x00524dd0` belongs to `src/philai.cpp`:

   ```sh
   homm3 sema disasm 0x00524dd0
   homm3 dreamcast show 0x00524dd0
   homm3 dreamcast asm 0x00524dd0 --blocks
   ```

3. Edit the C++, rebuild its module, and inspect the remaining differences:

   ```sh
   homm3 build --fast philai
   homm3 sema diff 0x00524dd0 --summary
   ```

   Where a Mac counterpart has been admitted, the same build compiles it with
   CodeWarrior. `homm3 mac labels` lists the Mac section offsets paired with
   existing source names. Use `homm3 mac show <Windows-VA>` and
   `homm3 mac diff <Windows-VA>` to inspect that exact target.

   `homm3 mac calls <Windows-VA>` compares retail and candidate call counts
   and ordered targets. Omit the selector for all admitted pairs. Every build
   also writes `build/mac/calls.tsv` and `build/mac/calls.json`.
   `homm3 mac queue` generates the full action list in
   `build/mac/queue.tsv` and `build/mac/queue.json`, including missing pairings,
   stale observations and deferred modules. Missing evidence is shown as
   unavailable. See the [tooling rollout](docs/tooling/mac-matching-roadmap.md).
   That guide also covers `homm3 mac compile` for emitted symbol discovery,
   `homm3 mac pair` for reviewed admission, and `homm3 mac campaign` for
   separate worker packets.

   The [implementation report](docs/tooling/mac-matching-report.md) explains
   the two-target pipeline, validation, current coverage and remaining tooling.

4. Repeat, then run `homm3 build` for the full checks before submitting changes.

Replace the example address and module with your target. The
[matching guide](AGENTS.md) covers the full evidence pass and reconstruction rules.

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
