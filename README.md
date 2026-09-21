# homm3-decomp

Binary-matching decompilation of **Heroes of Might and Magic III Complete**
(`HEROES3.EXE`, New World Computing, 2000). The goal is to recover the C++ structure and
behavior.

This repository does **not** contain either game's executable or resources. To match
the game, supply your own legally obtained retail `HEROES3.EXE` and Dreamcast `H3.EXE`.

<!-- match-score:start -->

**Executable MAX: 96.97%** — weighted by function size across 1,998,996 bytes of code included in matching.

**Function exact MAX** — 4,238 / 4,766 current implementations (88.9%) have reached 100%.

**CUR diagnostics** — 4,171 / 4,766 functions exact (87.5%) in this build (4766 in linked units). Compiler-context dips with held MAX do not reduce matching progress.

| Module       | Units | Functions exact CUR |  Function exact MAX | Fuzzy CUR | Fuzzy MAX |
| :----------- | ----: | ------------------: | ------------------: | --------: | --------: |
| `game`       |   123 | 3504 / 3989 (87.8%) | 3558 / 3989 (89.2%) |    96.76% |    97.10% |
| `rmg`        |     3 |   292 / 368 (79.3%) |   303 / 368 (82.3%) |    93.54% |    95.03% |
| `network`    |     4 |   266 / 280 (95.0%) |   268 / 280 (95.7%) |    97.68% |    98.18% |
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

## Quickstart

You need Nix with flakes enabled and your own copies of the two executables
listed above. From the repository root:

```sh
nix develop .#build
gh auth login        # needed for the toolchain download
HOMM3_EXE=/absolute/path/to/HEROES3.EXE \
HOMM3_DREAMCAST_EXE=/absolute/path/to/H3.EXE \
  homm3 init

homm3 build          # build all modules, compare with retail, and run checks
```

`homm3 init` verifies the executables and sets up the VC6 toolchain and Wine.
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
