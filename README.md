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

**Match score** — 64/68 functions exact (94.1%), 99.96% fuzzy across 14 unit(s).

| Module       | Units | Functions exact |  Fuzzy | Fuzzy Max |
| :----------- | ----: | --------------: | -----: | --------: |
| `zlib-1.1.3` |    14 | 64 / 68 (94.1%) | 99.96% |    99.96% |

_Function universe: the linked units only; the full-engine denominator and excluded-category tables arrive with the universe classifier._

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

```sh
nix develop .#build   # VC6 SP3 under wine + the tools
homm3 init            # ONE-TIME: toolchain tarball (pinned release, SHA-256-verified),
                      # wine prefix, smoke compile through the real cc_wrap path
homm3 build           # configure + ninja: compile every manifest unit
homm3 link            # OPT-IN candidate link (layout study; the EXE is not runnable)
homm3 clean           # nuke build/ entirely; `homm3 init` restores it
```
