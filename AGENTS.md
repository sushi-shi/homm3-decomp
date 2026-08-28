# HoMM3 Decomp Project Guide

Binary-matching decompilation of Heroes of Might and Magic III Complete. The goal is C++
that reproduces the retail MSVC 6.0 object code.

## Ground truth

- The retail executable is authoritative for code, data, and addresses:
  **English GOG Complete 4.0 (engine 3.2)**, `HEROES3.EXE`, 2,732,032 bytes, SHA-256
  `057c9d88e7206f6669a4615de2c6e02ab6c4e2d570a9e2badf07fe0bd6247274`, fixed base
  `0x00400000`, no `.reloc` directory, no CodeView stream.
- Evidence sources ranked below retail bytes:
  - **Dreamcast CodeView dump** (`../homm3-symbols/HoMM3-Dreamcast-Dump`): proves names,
    types, and layouts for the *Dreamcast* build. Cross-architecture — an x86 identity
    still requires retail-byte proof. The dump's own executable is at
    `../orig/dreamcast/H3.EXE` (WinCE SH4 PE, 8,425,752 B, sha256 `cdbc7e75…`, from two
    byte-identical GD-ROM rips; the embedded NB11 CodeView stream is what the dump
    prints), referenced via `$HOMM3_DC_EXE` — its own xref graph lives in
    `evidence/dc-xref-graph.tsv`.
  - **NH3API** (`../homm3-symbols/NH3API`): external and **contradicted on addresses**.
    Of its 874 embedded wrapper addresses, only **120 land on a carved function entry**;
    **751 fall inside a function body** — many mid-instruction (e.g. `0x5d70` is the last
    byte of the `cmp ecx,8` at `0x5d6e`) — and 3 are unowned (measured 2026-08-04 against
    `config/retail-functions.tsv`). They describe a different address space (HD Mod), so
    an NH3API address is NOT evidence for a boundary on this image. Its *names* and
    signatures remain useful once an address is independently proven.
    Correction: an earlier note here claimed "873 land on x86 entry patterns". That
    misread attempt-1's scorer — its `exec` column (873) only means the address lies in
    an executable section; its prologue-like column was 100. Do not resurrect the claim.
  - **decomp-attempt-1** (`../decomp-attempt-1`): the abandoned first attempt and a
    secondary evidence source. Its admitted inventories are now hand-owned here; its
    remaining artifacts may be consulted as hypotheses but never outrank retail bytes.
- Local retail copies live outside every repo at `../orig/` (safekeeping;
  hash-verify before use). The repo never contains game bytes; `build/` is gitignored.

## Primary matching instrument: Dreamcast source shape

**`homm3 dreamcast` is the campaign's most important reconstruction tool.** For every
non-exact game function with a Dreamcast counterpart, run the Dreamcast evidence pass
before speculative C++ rewrites. It recovers source facts that retail VC6 `/O2 /Ob2`
erased and that x86 disassembly alone cannot recover:

- original helper boundaries and names;
- signatures, parameter types, and a lower-bound local-variable inventory;
- lexical scopes and likely local lifetimes;
- statement order and grouping, with per-statement calls and branch counts;
- accessor calls that retail inlined into anonymous loads or tests;
- constructor, destructor, and RAII boundaries folded out of retail; and
- separation between shared RoE-era code, WinCE-specific code, and later Complete edits.

The mandatory first-pass workflow is:

```sh
homm3 dreamcast show 0x00524dd0
homm3 dreamcast asm 0x00524dd0 --blocks
homm3 sema diff 0x00524dd0 --source
```

`show` is the compact function dossier. `asm --blocks` labels SH4 assembly with
CodeView `bp` line/breakpoint boundaries, lexical `scope` boundaries, and inferred
basic blocks (`B0`, `B1`, ...). It is the detailed view for aligning original statement
groups to the retail and candidate control-flow graphs. Selectors may also be exact or
unambiguous names, `module.obj:0xOFF`, or `dc:0xOFF`; use `homm3 dreamcast find NAME`
to locate a function and `homm3 dreamcast stats` to audit corpus coverage.

The campaign loop is **Dreamcast dossier -> retail source-labelled diff -> C++
hypothesis -> VC6 retail ratchet**. Start with signatures, locals, scopes, helper calls,
and statement groups named by the dossier. Do not start with blind source permutation
when this evidence is available. If a hypothesis changes a claim or decorated signature,
run `homm3 build`, `homm3 delink`, then `homm3 build` again. Keep a source change only
when retail improves or matches, and record useful negative classifications such as
register allocation, scheduling, relocation naming, or version skew so the next matcher
does not repeat an exhausted sweep.

This remains a cross-architecture, older-revision source oracle, **not a second byte
target**. A missing Dreamcast statement or call never proves retail lacks it, and
Dreamcast source shape must be rejected when Complete's x86 bytes disagree. Retail is
always the verdict. See `docs/dc-line-tables.md` for interpretation and
`docs/dreamcast-proof-40.md` for the first bounded proof: four exact cohort closures,
one exact spillover, and one signature-driven near-closure from 40 non-exact functions.

Any future static verifier must therefore be **asymmetric**. It may flag our source for
violating a positive DC fact—an incompatible shared parameter type, a missing recovered
helper/RAII boundary, reversed shared-statement order, or an impossible scope/lifetime.
It must not require equal instruction, basic-block, branch, call, statement, local, or
scope counts; equal addresses; or the absence of retail-only code. Report findings as
`agree`, `retail-only`, `dc-only`, or `unknown`, and make only explicitly admitted
positive invariants fatal. Architecture/compiler codegen and revision skew are expected,
not exceptions to be explained away.

## Toolchain

- Compiler: VC6 SP3 `CL.EXE` under Wine; linker: VC6 `LINK.EXE` 6.00.8447 (the
  generation that built retail). `homm3 init` fetches the toolchain tarball from the
  pinned `toolchain-vc6-sp3` GitHub release via `gh` and verifies its SHA-256.
- The Wine prefix is **stateless**: INCLUDE passes per invocation, `cl.exe` is invoked by
  absolute path, libraries pass at link time. No registry PATH/INCLUDE/LIB writes exist
  to go stale (deliberate divergence from the Gruntz template).
- No MSDIS stub is needed for VC6 linking (unlike Gruntz's VC5): LINK 8447 statically
  imports only mspdb60/msvcrt/kernel32.
- zlib TUs compile with `/O2 /ML /Gr /TC /D_WINDOWS`. Game-TU profiles arrive with the
  first game unit.
- The Wine VC6 build is the sole verdict on a match; clang/clangd is editor tooling only.

## Build

```sh
nix develop .#build       # from the repo root
homm3 init                # one-time: toolchain + wine prefix + smoke compile
homm3 build               # configure + ninja (all manifest units)
homm3 link                # opt-in candidate link: /FORCE /NODEFAULTLIB /MAP,
                          #   .map layout study + unresolved-externals punch list
homm3 clean               # whole-build/ nuke; init restores everything
```

`ninja` alone works for rapid iteration once configured. `homm3 build` never invokes the
delinker (that step is not yet ported).

## Source-labelled matching

Use the candidate's VC6 debug lines to localize a mismatch to the C++ statement that
lowered into it:

```sh
homm3 sema disasm 0x00554400 --base --source --verbose
homm3 sema diff 0x00554400 --source
```

`--source` compiles a separate cached `/Z7` object under `build/debug/`; it never adds
debug flags to the matching object. The tooling verifies that the selected function's
code bytes are identical before applying the line offsets. In the diff view, assembly is
aligned first and source headings are attached afterwards, so comments cannot change the
verdict. Use the first `!!` statement to choose where to inspect or respell the source.

These labels describe the **candidate build only**. They are navigation evidence about
which candidate statement produced an instruction span, not evidence for retail source
semantics, and they never outrank the retail bytes or objdiff score.

## Tooling layout

One importable package (`scripts/homm3/`), one CLI (`homm3`), grouped by role:
`core/` (shared primitives — `cc_wrap`, `common`, `image`), `build/` (ninja-graph
actors and the delink loop), `match/` (status/ratchet + the function-universe
classifier), `init/` (toolchain/prefix setup). Retired tools live in
`scripts/archive/` (the carve bootstrap pipeline is there — do not resurrect).
Later phases add `ghidra/`, `sema/`, `audit/` per the port plan. Add a tool to its
role package, not to a new top-level file.

## Repository model (contracts; enforcement lands with each phase)

- **Source is the authority for names** once game TUs land: annotation macros in source,
  a generated label map re-derived every build, authority-checked against symbol tables.
  No hand-maintained symbol ledger. (Annotation contract = open decision P0.2.)
- **`config/` vs `evidence/`**: `config/` holds only hand-admitted retail inventories
  (functions, relocs, vtables — MANUALLY MANAGED after admission) and build manifests;
  `evidence/` holds GENERATED analysis deliverables (DNA bands, name maps, joins —
  regenerate, never hand-edit).
- **Vendored sources stay pristine**: no macros in `vendor/`; zlib's rva→symbol map is
  the one reviewed table `config/retail-zlib-map.tsv`.
- **Gates must be able to fail**: every future fatal gate ships with a negative control
  proving it still detects its defect.
- The delinker is vostok, pinned in the flake at the upstream stacked-queue head; it
  recovers exact code relocations for stripped PEs directly from a synthesized PDB —
  exactly what this `.reloc`-less target needs.

## References

- `docs/gruntz-script-port.md` — the port plan, module inventory, decision log, and
  open decision points. Read it before proposing any new area.
- `~/Projects/gruntz` — the architecture template (pipeline, gates, conventions).
- `~/Projects/homm2/homm2-decomp` (branch `decomp-pol-2.0`) — the mature sibling
  project; its README/CLAUDE/AGENTS are the style reference for this repo.
- `../decomp-attempt-1/docs/` — target provenance (`target.md`), coverage and layout
  evidence from the first attempt.
- `AGENTS.md` — durable agent policy for this repo.
