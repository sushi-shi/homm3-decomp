# homm3-decomp

> **All functions match.** Every function of the Loki `h3maped` image matches;
> data matching is in progress.

C++ reconstruction of the **Heroes of Might and Magic III map editor** in Loki
Software's Linux port (`h3maped` 1.0, Restoration of Erathia), built with GCC
2.95.2 and SGI STL 3.2. All functions match; data is in progress. The editor
shares its engine source with the Windows game on `decomp-complete-4.0`.
Retail bytes are authoritative. Supply your own executable and toolchain
packages.

## Match status

<!-- loki-match-score:start -->

**Loki Linux `h3maped` 1.0 (map editor, GCC 2.95.2): 7,432 / 7,432 functions exact (100.00%) &middot; 100.00% fuzzy &middot; 561,138 / 565,944 data bytes (99.15%) (MAX).**

A separate image with its own scores; `homm3 loki build --bank` banks `config/loki/match_baseline.tsv`, and this block renders from it.

| Phase | Objects | Functions exact MAX | Fuzzy MAX | Data bytes MAX |
| :---- | ------: | ------------------: | --------: | -------------: |
| engine (shared with the game) | 29 | 1,001 / 1,001 (100.0%) | 100.00% | 101,608 / 102,144 (99.48%) |
| editor | 74 | 6,431 / 6,431 (100.0%) | 100.00% | 459,530 / 463,800 (99.08%) |

_CUR / MAX / HIST: 7,432 / 7,432 / 7,432 exact &middot; 100.00% / 100.00% / 100.00% fuzzy, weighted by size &middot; 561,138 / 561,138 / 561,138 data bytes. Project functions only: the 3,120 `.text` functions and 4,312 kept linkonce bodies of the 103 GCC 2.95.2 project objects (103 built units); GTK+/glib/libglade/libxml/zlib and libstdc++ are excluded. Data bytes are the objects' `.rodata`, `.data`, `.bss`, `.gcc_except_table`, `.ctors`/`.dtors` slices of the image, their kept linkonce data (vtables) and COMMON type_info nodes, relocations resolved; jump tables count with their functions._

<!-- loki-match-score:end -->

### HEROES3.EXE

The Windows game still builds from this tree and keeps its own scores.

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

## Branches

```text
decomp-complete-4.0 ----> decomp-loki-1.0 (you are here)
```

- [`decomp-complete-4.0`](https://github.com/sushi-shi/homm3-decomp/tree/decomp-complete-4.0#branches) — Complete 4.0 `HEROES3.EXE` (Sep 2000), VC6 SP3
- [`decomp-loki-1.0`](https://github.com/sushi-shi/homm3-decomp/tree/decomp-loki-1.0#branches) — Loki Linux 1.0 map editor `h3maped`, GCC 2.95.2

## Pinned executables

`h3maped` is this branch's target; the game tooling still uses `HEROES3.EXE`
and its Dreamcast and Mac references. Supply your own
copies; sizes and SHA-256 hashes are pinned in
[config/project.toml](config/project.toml).

- `h3maped`: Loki Linux map editor 1.0 (ELF i386, GCC 2.95.2)
- `HEROES3.EXE`: English Complete 4.0, engine 3.2 (MSVC 6.0, Sep 2000)
- `H3.EXE`: Dreamcast port, SH-4 with CodeView symbols (Aug 2000)
- `Heroes_III_raw.pef`: Classic Mac OS port, PowerPC CodeWarrior (Dec 2000)

## Quickstart

With Nix flakes enabled, run from the repository root. `homm3 loki init`
stages the image and the pinned GCC 2.95.2 toolchain, whose packages and
hashes are listed in [config/loki/toolchain.toml](config/loki/toolchain.toml);
`homm3 loki build` compiles, delinks and compares every unit. The 2000-era
binaries run unmodified, without Wine.

```sh
nix develop .#build
homm3 loki init --exe /path/to/h3maped --debs DIR --sgi-stl DIR \
  --binutils DIR --gcc DIR --gtk DIR
homm3 loki census --check
homm3 loki build -v
```

Retail inputs, the toolchain and generated reports stay in ignored `build/`.

## Improve a function

```sh
homm3 loki disasm _getC__13TGzInflateBuf   # retail, references named
homm3 loki diff Error __11TDebugBreak      # one function, base | retail
homm3 loki build Error
homm3 loki build --bank                    # bank scores and refresh this README
```

Engine units compile the game's own `src/` files, so one source serves both
programs; `homm3 build` still checks the Windows game.

## Data matching

`homm3 loki build` also compares each object's data with its slice of the
image: `.rodata`, `.data`, `.bss`, exception tables, constructors and
vtables. `homm3 loki emitorder` checks function emission order against the
image's.

## Documentation

- [Loki h3maped image](docs/loki/README.md): toolchain, flags, census, ledger
  and source rules
- [Documentation index](docs/README.md); contributor rules are in
  [AGENTS.md](AGENTS.md)

## License

Project-authored source and tooling use [CC0 1.0](LICENSE). Files with their
own notices, notably everything under `vendor/`, keep their terms; retail
inputs, compiler binaries and game assets are excluded.

## Thanks

[NH3API](https://github.com/void2012/NH3API) documented game structures and
their layouts, and supplied naming references beyond the Dreamcast symbols.
