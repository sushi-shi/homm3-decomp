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

## Standing orders for matching agents

- **We chase MAX, not cur.** `config/match_baseline.tsv` banks each function's best
  score together with the source hash it was measured at; that banked MAX is the
  judged outcome. A function whose own source did not change and whose current score
  fell (a header correction, a removed view scaffold, a delink generation, new claims
  elsewhere) is observational collateral: note it in the commit message and continue.
- **No agent recovers what is already banked by hash.** Rows sitting below their banked
  MAX are not targets - not for polish lanes, not for view-removal lanes, not for
  integrators. Do not keep a view guard, gate a declaration, or respell a caller to lift
  an unrelated row back to its banked value.
- The ratchet still fails the build for a function whose OWN source changed and regressed
  below its MAX; that, and gate errors, are what must be fixed.

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

## Tooling layout

One importable package (`scripts/homm3/`), one CLI (`homm3`), grouped by role:
`core/` (shared primitives — `cc_wrap`, `common`, `image`, `tsv`), `build/`
(ninja-graph actors and the delink loop), `retail_labels/` (every label as a
typed Claim record: parse-only censuses/providers/IAT channels plus the
source-macro extraction that caches per-TU fragments under `build/gen/claims/`;
CLI `homm3 labels`), `match/` (status/ratchet + the function-universe
classifier), `init/` (toolchain/prefix setup). Two top-level spine modules
mirror the gruntz template: `manifest.py` (the thin `units.toml` reader) and
`model.py` (the ONE label join — the only place labeling policy lives — writing
`build/gen/symbol_names.csv` + `compgen_claims.tsv`; CLI `homm3 model`). Retired
tools live in `scripts/archive/` (the carve bootstrap pipeline is there — do not
resurrect). Later phases add `ghidra/`, `audit/` per the port plan. Add a tool
to its role package, not to a new top-level file.

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

- Function-specific failed probes belong beside the affected source function;
  reusable compiler-wide findings belong under `docs/vc6/`. Do not create a
  second chronological decision log.
- `~/Projects/gruntz` — the architecture template (pipeline, gates, conventions).
- `~/Projects/homm2/homm2-decomp` (branch `decomp-pol-2.0`) — the mature sibling
  project; its README/CLAUDE/AGENTS are the style reference for this repo.
- `../decomp-attempt-1/docs/` — target provenance (`target.md`), coverage and layout
  evidence from the first attempt.
- `AGENTS.md` — durable agent policy for this repo.
