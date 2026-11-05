# homm3-editor

Binary-matching decompilation of the **Heroes of Might and Magic III Complete**
Windows map editor (`h3maped.exe`) and campaign editor (`h3ccmped.exe`), reusing
the game's recovered C++ source.

This repository contains no executables or game resources. Supply your own
legally obtained copies of the pinned Windows binaries. Dreamcast and Mac
inputs are not part of this worktree; historical evidence remains in source comments.

## Pinned targets

Both editors come from GOG's `setup_homm3_complete_2.0.0.16.exe` release. They
are MSVC 6.0 PE32 images at base `0x00400000`, built on 5 September 2000.

| Target | Executable | Size | SHA-256 |
| --- | --- | ---: | --- |
| Map editor | `h3maped.exe` | 2,134,016 bytes | `4480fba145c9f885942cc668d4bce430fe39c0fa482d1a6e58f96318ab857a37` |
| Campaign editor | `h3ccmped.exe` | 1,212,416 bytes | `93e98214c1d21d320281f3ef6b8db5bb2ab5a1056b84bf0cf34815d01f7533c1` |

The Steam copies have identical code and resource sections, but different
whole-file hashes because of certificate metadata. Input verification requires
the GOG files above. See [editor provenance and matching](docs/matching/editors.md)
for the comparison and compiler evidence.

## Quickstart

Use Nix with flakes enabled. The current initializer also requires the pinned
`HEROES3.EXE` for the inherited shared-source checkpoint, described below.

```sh
nix develop .#build
export HOMM3_DIR="$PWD"
gh auth login        # needed for the toolchain download
homm3 init --exe /path/to/HEROES3.EXE \
  --map-editor-exe /path/to/h3maped.exe \
  --campaign-editor-exe /path/to/h3ccmped.exe

homm3 editor h3maped gzinflatebuf
homm3 editor h3ccmped gzinflatebuf
```

`homm3 init` verifies supplied executables and sets up VC6 and Wine. The editor
commands freshly compile the shared TU with the provisional `/O1` profile and
write isolated objects and JSON reports under `build/editor/<target>/<profile>/`.
Use `--json` to print a report or `--profile NAME` to select another profile
from `config/units.toml`. This does not yet build runnable editors.

### Current editor coverage

| Target | Reviewed function spans | Reviewed CRT identities | Shared-source pilot |
| --- | ---: | ---: | --- |
| `h3maped` | 4 | 3 | `TGzInflateBuf::getByte`: all 84 bytes exact at `0x0044960f` |
| `h3ccmped` | 3 | 3 | No exact `getByte` counterpart found |

The CRT entries are included in the function-span counts. Editor inventories
live separately in [`config/retail/h3maped/`](config/retail/h3maped/) and
[`config/retail/h3ccmped/`](config/retail/h3ccmped/). These are small reviewed seeds,
not complete censuses, so no editor completion percentage is reported.

The discovery command compares complete unrelocated bytes against reviewed
spans. Functions with relocations and hits without reviewed boundaries need
further analysis. A broader [direct fingerprint pass](docs/matching/editors.md#direct-coff-fingerprints)
compiled all 152 source units with `/O1` and found shared-source, MFC, CRT and
zlib candidates without using Ghidra boundaries. Zlib strongly favors its own
`/O2` profile. These findings have not been added to the seed TSVs.

A [direct comparison between the editors](docs/matching/editors.md#common-code-across-the-editors)
pairs 1,309 function candidates, including 127 unnamed pairs. Their code ranges
and reference evidence remain separate from reviewed inventory coverage.

The [reviewed engine map](docs/matching/editor-engine.md) records 157 source-owned
map-editor identities, extending the original 108 anchors through named calls,
vtables and function order. It also identifies both artifact-mask helpers in the
campaign editor. The address tables and evidence are preserved in documentation;
full generated analysis stays under ignored `build/`. All TSVs remain unchanged.

### Source layout

Keep the existing game/shared files at the roots of `src/` and `include/`.
Editor-specific code uses matching subdirectories:

```text
src/
  artifact.cpp          # Existing game/shared implementation
  rmg.cpp
  editor/               # Map-editor implementation
  campaign_editor/      # Campaign-editor implementation

include/
  artifact.h            # Existing game/shared declarations
  rmg.h
  editor/               # Map-editor declarations
  campaign_editor/      # Campaign-editor declarations
```

Shared code keeps one canonical definition in its existing file. Each editor
will select the required shared TUs with its own compiler settings and outputs.
The editor directories are placeholders for reconstruction; new C/C++ files
must be admitted to the build and ownership tooling when added. Original source
paths remain provenance evidence, without requiring a directory-for-directory
copy of the original tree.

### IDE setup

Enable your editor's clangd integration and open the repository root. The Nix
shell provides clangd and generates `compile_commands.json`; launch your editor
from that shell. The compilation database refreshes on shell entry and during
builds. Run `homm3 init` first to install the compiler headers.

### Continue matching

Choose the editor explicitly with `homm3 editor h3maped <TU>` or
`homm3 editor h3ccmped <TU>`. Read the recorded source evidence and the selected
editor's assembly before changing shared C++. Verify each editor address and
helper identity independently; a game's `VA` annotation is not an editor claim.

The [matching guide](AGENTS.md) covers source recovery and validation. The
[editor notes](docs/matching/editors.md) describe the pilot's limits and next
steps; the [documentation index](docs/README.md) covers the inherited tooling.

## Inherited game checkpoint

`homm3 build`, `homm3 status` and `homm3 sema` currently target the original
**English GOG Complete 4.0 (engine 3.2)** game. They provide a shared-source
checkpoint; their addresses and scores do not describe either editor.

```
file        HEROES3.EXE
size        2,732,032 bytes
sha256      057c9d88e7206f6669a4615de2c6e02ab6c4e2d570a9e2badf07fe0bd6247274
timestamp   8 September 2000, built by MSVC 6.0
```

Run `homm3 build` to verify that checkpoint after shared-source changes.

<details>
<summary>Inherited game matching scores</summary>

<!-- match-score:start -->

**Executable MAX: 97.10%** — weighted by function size across 1,999,142 bytes of code included in matching.

**Function exact MAX** — 4,266 / 4,768 current implementations (89.5%) have reached 100%.

**CUR diagnostics** — 4,199 / 4,768 functions exact (88.1%) in this build (4767 in linked units). Compiler-context dips with held MAX do not reduce matching progress.

| Module        | Units | Functions exact CUR |  Function exact MAX | Fuzzy CUR | Fuzzy MAX |
| :------------ | ----: | ------------------: | ------------------: | --------: | --------: |
| `game`        |   123 | 3533 / 3990 (88.5%) | 3588 / 3990 (89.9%) |    97.01% |    97.35% |
| `rmg`         |     3 |   291 / 368 (79.1%) |   301 / 368 (81.8%) |    93.19% |    94.37% |
| `network`     |     4 |   266 / 280 (95.0%) |   268 / 280 (95.7%) |    97.68% |    98.08% |
| `zlib-1.1.3`  |    14 |    69 / 69 (100.0%) |    69 / 69 (100.0%) |   100.00% |   100.00% |
| `codec`       |     4 |     35 / 43 (81.4%) |     35 / 43 (81.4%) |    94.70% |    94.70% |
| `victor`      |     4 |      5 / 17 (29.4%) |      5 / 17 (29.4%) |    85.40% |    85.40% |
| `(unmatched)` |     — |        0 / 1 (0.0%) |        0 / 1 (0.0%) |      0.0% |      0.0% |

_Excluded from the % above — generated/library code, not independent reconstruction targets:_

| Category              | Functions | Code (B) | Why excluded                                                       |
| :-------------------- | --------: | -------: | :----------------------------------------------------------------- |
| `EH unwind funclets`  |     5,125 |   53,151 | compiler EH unwind funclets; match with their parent function      |
| `CRT/C++ runtime`     |       913 |  110,536 | CRT/C++ runtime, named not matched (config/retail/runtime-map.tsv) |
| `init/cleanup thunks` |     1,119 |   94,433 | .CRT$XCU dynamic-initializer bodies (compiler-generated)           |
| `import thunks`       |        27 |      162 | FF 25 jumps through the IAT                                        |

<!-- match-score:end -->

The game ledger keeps `CUR <= MAX <= HIST`: CUR is the latest build, MAX is the
best score for the current function source, and HIST is its all-time peak.
These values are separate from editor discovery results.

</details>

## License

Project-authored reconstruction source and tooling are dedicated to the public
domain under [CC0 1.0](LICENSE), to the extent the contributors can do so.
Files carrying separate copyright or license notices — notably everything under
`vendor/` — retain those terms. No binary game assets are stored in this
repository.

## Thanks

Thanks to [NH3API](https://github.com/void2012/NH3API) for documenting game
structures and their layouts, and for naming references that supplement
historical source evidence.
