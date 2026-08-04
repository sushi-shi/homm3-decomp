# Gruntz script inventory and the homm3-decomp port plan

Status: **plan under review — nothing is ported without explicit approval.**
Source of truth for the inventory: `~/Projects/gruntz/scripts/` at commit
`0456d62b4`. Every port step below is one reviewable unit; each lands only
after it has been read, adapted, and approved in a supervised session.

## 1. The architecture we are adopting

Gruntz keeps **one importable Python package** — `scripts/gruntz/` — run as
`python -m gruntz.<area>.<module>`, fronted by **one CLI** (`gruntz`, defined
in `cli.py`) inside one Nix dev shell. `homm3-decomp` already mirrors the
shell and package naming (`scripts/homm3/`), so the port is area-by-area, not
a rename.

The package is grouped by role, and the grouping is load-bearing:

| Area | Role |
|---|---|
| `core/` | Shared read-once primitives (PE, symbols, manifest, IR). No tool logic. |
| `build/` | Actors invoked by the ninja graph: compile → labels → synth-PDB → delink → normalize. |
| `init/` | One-time environment setup (wine toolchain, clangd compdb). |
| `ghidra/` | PyGhidra driver + non-importable in-Ghidra scripts (`ghidra/scripts/`, no `__init__.py` — the boundary is explicit). |
| `sema/` | Interactive semantic navigation for humans/agents (disasm, xref, rva dossiers). |
| `match/` | Match progress, ratchets, and **fatal build gates** with negative-control selftests. |
| `audit/` | One-shot campaign tools, dispatched as `gruntz audit <tool>`. |
| `cleanliness/` | Live regression metrics (scoreboard vs a committed baseline). |
| `permute/` | Source-permutation hill-climbers for the /O2 codegen wall. |
| `scripts/archive/` | Retired tools + a "do NOT resurrect" README; the metrics they drove to 0 stay live as gates. |

Principles worth porting verbatim:

- **Source is the authority for names.** Matched functions carry an
  `RVA(0x…, size)` macro (from `src/rva.h`, compiled out under MSVC); matched
  globals carry `DATA(0x…)`. `build/labels.py` re-derives the complete
  rva→mangled-name map every build from clang IR annotations + AST, then
  authority-checks each name against the base obj's symbol table (llvm-nm).
  There is no hand-maintained symbol ledger to go stale.
- **Vendored sources stay pristine.** Vendored C TUs (zlib) get no macros;
  their rva→symbol map is one static reviewed CSV
  (`config/zlib_labels.csv`), authority-checked the same way.
- **The pipeline is ninja's dependency graph**, emitted by `configure.py`
  from `config/units.toml`:
  `src → cl(wine) base objs → labels → synth_pdb → vostok delink target objs
  → normalize both sides → objdiff report.json → match summary`.
- **Ghidra is live in the loop**, not a frozen export: struct/enum layouts are
  derived from *compilable source* by clang, applied to the Ghidra DB by a
  PyGhidra driver, and the refreshed `functions.csv`/`symbols.csv` exports
  feed the next delink.
- **Gates must be able to fail.** Fatal gates (unique names, library overlap,
  stub metadata), a MAX-fuzzy% high-water ratchet with a degeneracy guard, and
  `gate_selftest` negative controls that prove each gate still detects the
  defect it was built for.

## 2. Module inventory

Line counts are current Gruntz sizes — a proxy for review effort, not port
effort (some modules shrink or vanish under HoMM3 adaptation).

### 2.1 `core/` — shared primitives (~4.9k LOC)

| Module | LOC | Purpose |
|---|---|---|
| `pe.py` | 205 | Retail EXE parsed once: sections, .text, reloc sites, call index, string table. |
| `symbols.py` | 227 | The rva↔name database: generated labels + Ghidra exports + demangled aliases. |
| `manifest.py` | 46 | `config/units.toml` reader, cached per process. |
| `exe_map.py` | 618 | Queryable whole-.text ownership map (TU / MFC / CRT / zlib / funclet / thunk). |
| `ir.py` | 102 | Clang front-end helpers (TU → LLVM IR under MSVC-compat flags). |
| `cc_wrap.py` | 185 | The `wine cl` wrapper (ninja rule + permuter + sema all share it). |
| `codeview.py` | 216 | Read `/Z7` CodeView locals from COFF objects (no PDB). |
| `clangd_query.py` | 412 | One-shot clangd queries over the compdb (defs, refs, types). |
| `report.py` | 29 | objdiff `report.json` loaded once. |
| `library_labels.py` | 37 | Contract for FID library-label rows (HIGH/MED/AMBIG/LOW). |
| `vtable_scan.py` | 333 | Recover every retail vtable + exact size from three independent signals. |
| `vtable_hierarchy.py` | 1185 | Per-class slot table: new / override / inherited, incl. MI secondaries. |
| `class_meta.py` | 295 | src/+include/ class-definition scanner feeding the cleanliness gates. |
| `data_audit.py` | 319 | Attribute retail .rdata/.data/.bss bytes to `DATA()` symbols + fingerprints. |

### 2.2 `build/` — pipeline actors (~4.4k LOC)

| Module | LOC | Purpose |
|---|---|---|
| `labels.py` | 1411 | Derive rva→mangled-name map from src annotations (IR join + AST join + authority check; vendored-CSV path). |
| `synth_pdb.py` | 871 | Fabricate the PDB vostok consumes, from the label map + Ghidra inventory. |
| `delink.py` | 182 | Run vostok: retail EXE + synth PDB → per-unit target objects. |
| `data_manifest.py` | 503 | Generate vostok's `--data-manifest`/`--data-section-manifest` (reviewed data topology). |
| `coff_oracle.py` | 162 | Recover cl.exe string-pool COMDAT names for delinked data. |
| `normalize_objs.py` | 103 | Batch-normalize base+target objs into disposable comparison copies. |
| `canonicalize_data_symbols.py` | ~200 | The local COFF transform behind normalize (compiler-private name canonicalization). |
| `harvest_locals.py` | 201 | Second `/Z7` compile (byte-identical .text) → per-RVA local names for Ghidra enrichment. |
| `ghidra_metadata_generate.py` | 336 | Derive Ghidra struct/enum definitions from compilable source via clang. |
| `link.py` | 207 | Phase 2: link base objs into a candidate EXE + map. |
| `msdis_stub.py` | 172 | Make VC5 `link.exe` loadable under wine (MSDIS100.DLL stub). |
| `ninja_syntax.py` | 186 | Vendored ninja writer (already in homm3-decomp). |

### 2.3 `init/` and `ghidra/` (~1.6k LOC)

| Module | LOC | Purpose |
|---|---|---|
| `init/toolchain.py` | 177 | Wine prefix + toolchain setup for `wine cl.exe`. |
| `init/clangd.py` | 227 | Generate the clangd compdb (editor-side, additive). |
| `ghidra/ghidra_metadata_apply.py` | 109 | PyGhidra driver: import/analyze + apply + export. |
| `ghidra/scripts/apply.py` | 1038 | The in-Ghidra enrichment script (names, structs, enums). |
| `ghidra/scripts/export.py` | 64 | Dump `functions.csv`/`symbols.csv`. |
| `ghidra/scripts/export_user.py` | 116 | Capture human Ghidra edits → `config/user_annotations.json`. |
| `ghidra/scripts/decomp_export.py` / `decomp_force.py` / `decomp_gaps.py` / `dump_cc.py` | 232 | Decompiler-output exports and gap probes. |

### 2.4 `match/` — progress + gates (~2.4k LOC)

| Module | LOC | Purpose |
|---|---|---|
| `status.py` | 1101 | Match progress and regression detection over objdiff report history. |
| `high_water.py` | 171 | MAX-fuzzy% ratchet (`config/match-max.tsv`) with degeneracy guard. |
| `residual_queue.py` | 124 | Persistent best-first worklist of every function under 100%. |
| `fingerprints.py` | 282 | Per-function source fingerprints so status knows if a dip is a real regression. |
| `verify_unique_names.py` | 111 | FATAL gate: one mangled name = one RVA; one .text stretch = one claim. |
| `verify_library_overlap.py` | 226 | FATAL gate: src claims may not collide with library-label rows. |
| `verify_stubs.py` | 154 | Verify the `// @…`-block metadata on labeled-but-unmatched stubs. |
| `gate_selftest.py` | 503 | Negative controls: prove every gate still fails on its pinned defect. |

### 2.5 `sema/` — navigation (~1.1k LOC)

`disasm` (237), `xref` (261), `strings` (127), `rva` dossiers (61),
`classof` (85), `dump_target` (99), `map` (15), `match` (34), `vtable` (27),
`clangd` LSP passthrough (27), `_common` (84). One `sema` CLI entry point;
thin wrappers over `core/`.

### 2.6 `cleanliness/`, `audit/`, `permute/` (~2.5k + ~8.5k + ~2k LOC)

- `cleanliness/`: the scoreboard (`board.py`, 481) plus vtable/view/class
  gates (`vtable_virtuality`, `vtable_coverage`, `vtable_slot_binding`,
  `vtable_owner`, `vtable_secondary`, `vtable_bans`, `class_sizes`,
  `class_vtables`, `caller_callee`, `declared_only`, `foldable_views`,
  `view_debt`). Printed under every `gruntz build`.
- `audit/`: ~27 one-shot campaign tools (`assert_relocs`, `exe_diff`,
  `link_defects`, `link_order`, `mfc_class`, `tu_layout`, `stale_walls`,
  `rename_member`, `reorder_tu`, `retype_ints`, FID subpackage, …). These are
  campaign-born: most will NOT be ported up front — the *convention* (an
  `audit/` area + name-dispatch) is what we adopt.
- `permute/`: the /O2 hill-climbers (`permute.py`, `permute_sweep`,
  variant generators, TU state metrics). HoMM3 attempt-1 already carried
  early copies of four of these (`batch_source_variants`,
  `generate_ast_variants`, `match_variants`, `tu_state_*`) — the Gruntz
  versions are the ones to take.
- `scripts/archive/`: NOT ported. We adopt only the convention: retired tools
  move to `scripts/archive/` with a README entry; their metrics stay live.

Top-level (outside the package): `configure.py`, ninja glue, and
`create-toolchain-release.{py,nix}` — homm3-decomp already has all three.

## 3. What homm3-decomp already has

- `scripts/homm3/cli.py` — configure/build only.
- `scripts/homm3/build/cc_wrap.py`, `build/ninja_syntax.py`.
- `config/units.toml` (14 zlib TUs), `configure.py`, the ninja graph → 14
  base objects.
- Flake pins vostok-delinker and objdiff 3.7.1; VC6 SP3 toolchain tarball.
- objdiff config exists but every `target_path` is `dummy.obj` — there is no
  retail side yet, so nothing is verified end-to-end.

## 4. Port plan

Each step is one supervised review session and roughly one commit. Order is
driven by one goal: **restore an end-to-end verified match (zlib) as early as
possible**, then widen. Adaptation notes mark where HoMM3 differs from Gruntz
(VC6 SP3 vs VC5 SP3; LINK 8447; 12,012-function denominator; C++-heavy game
code; Dreamcast CodeView as extra evidence with no Gruntz analog).

### Phase 0 — contract before code

- **P0.1** Write the repo's `AGENTS.md`/`docs/build-system.md` equivalent:
  package layout, area roles, the source-as-authority rule, the
  vendored-pristine rule, the archive convention, and the gate philosophy.
  (Gruntz files to read for this: `AGENTS.md`, `docs/build-system.md`,
  `docs/comment-markers.md`, `scripts/archive/README.md`.)
- **P0.2** Decide the annotation contract for HoMM3: adopt `src/rva.h`
  `RVA()/DATA()` macros as-is, or adapt. Decide the vendored-zlib label CSV
  name/location. **Decision point — nothing else in Phase 2 starts before
  this.** *(RESOLVED 2026-08-04 in two steps — macros adopted as-is plus
  `DC_ONLY`; the zlib label CSV is `config/retail-zlib-map.tsv`. See the
  decision log.)*

### Phase 1 — core skeleton (target: `homm3 build` still green)

- **P1.1** Restructure `scripts/homm3/` into areas; move `cc_wrap.py` to
  `core/` (as in Gruntz), keep `ninja_syntax.py` in `build/`. CLI docstring
  becomes the pipeline's living documentation, Gruntz-style.
- **P1.2** Port `core/manifest.py` (trivial) and `core/report.py` (trivial).
- **P1.3** Add `config/target.toml` + retail `HEROES3.EXE` intake (hash-gated
  copy under `build/exe/`), then port `core/pe.py`.
  *Adaptation: PE facts come from our target.toml (attempt-1 has the reviewed
  values); Gruntz's ILT/thunk specifics need re-derivation for LINK 8447.*

### Phase 2 — the delink loop (target: real objdiff % for zlib)

- **P2.1** Port `build/labels.py` — initially ONLY the vendored-CSV path:
  a reviewed `config/zlib_labels.csv` (rva, symbol, unit, size), authority
  checked against the base objs. The macro/IR path ports later with the first
  game TU. *The 28 proven zlib functions from attempt-1 are the seed rows,
  re-reviewed one compiland at a time.*
- **P2.2** Port `build/synth_pdb.py`. *Adaptation: attempt-1's synth-PDB knew
  VC6 EH funclets, folded aliases, and data topology; the Gruntz module is
  the structural template, attempt-1 is the VC6 evidence. Reconcile under
  review rather than copying either.*
- **P2.3** Port `build/delink.py` + wire objdiff `target_path` to the real
  delinked objects, replacing `dummy.obj`. **Milestone: `homm3 build` prints
  a genuine zlib match summary.**
- **P2.4** Port `build/normalize_objs.py` + `canonicalize_data_symbols.py`.
- **P2.5** Port `build/data_manifest.py` + `build/coff_oracle.py` when the
  first data mismatches appear. *Depends on which vostok PRs (11/12/15 line)
  are merged at the flake's pinned rev vs still need local patches.*

### Phase 3 — match bookkeeping and gates (target: regressions impossible to miss)

- **P3.1** Port `match/status.py` + `match/residual_queue.py` +
  `match/high_water.py` (+ `fingerprints.py`).
- **P3.2** Port the fatal gates: `verify_unique_names.py`,
  `verify_library_overlap.py`, `verify_stubs.py`. *Adaptation:
  unique-names' linker theorem must be restated for VC6 (`/OPT:ICF` folding
  exists in HEROES3 — the gate needs the alias model attempt-1 proved).*
- **P3.3** Port `match/gate_selftest.py` and pin one negative control per
  gate as it lands.

### Phase 4 — Ghidra in the loop

- **P4.1** Port `init/toolchain.py` semantics into the existing flake/init
  path where missing; port `init/clangd.py`.
- **P4.2** Port the Ghidra side: `ghidra_metadata_apply.py` driver,
  `scripts/export.py`, `scripts/apply.py`, `export_user.py`, decomp probes;
  extend `homm3 init` to build the Ghidra DB from HEROES3.EXE.
  *Adaptation: attempt-1's 12,012-boundary denominator, the 26 extra COFF
  roots, and boundary overrides must become reviewed inputs to this loop —
  the step where we decide which attempt-1 CSVs survive as evidence vs are
  regenerated.* **Decision point.**
- **P4.3** Port `build/ghidra_metadata_generate.py` + `harvest_locals.py`
  once the first game TU compiles.

### Phase 5 — navigation and first game code

- **P5.1** Port `core/symbols.py`, `core/exe_map.py`, and the `sema/` area
  (disasm, xref, rva, strings, map first; the rest as needed).
- **P5.2** Port the macro/IR half of `labels.py` + `src/rva.h`, and admit the
  first game TU to `config/units.toml`. *Candidate: start from attempt-1's
  proven `inputmgr` unit, re-reviewed.* **Milestone: first game-code match.**
- **P5.3** Port `core/ir.py`, `core/clangd_query.py`, `core/codeview.py` as
  their consumers arrive.

### Phase 6 — scale-out tooling (on demand, campaign-driven)

- **P6.1** Vtable/class tooling: `core/vtable_scan.py`,
  `core/vtable_hierarchy.py`, `core/class_meta.py`, `core/data_audit.py`,
  and the `cleanliness/` gates + board with a fresh baseline.
- **P6.2** `permute/` when the first /O2 plateau appears (supersedes the
  attempt-1 copies).
- **P6.3** `audit/` tools individually, only when a campaign needs one;
  everything else stays in Gruntz as reference. `core/library_labels.py` +
  the FID audit subpackage arrive with the CRT/library-labeling campaign.
- **P6.4** `build/link.py` (+ a VC6 equivalent of `msdis_stub.py` only if
  LINK 8447 needs one under wine) when phase-2 whole-image linking starts.

## 5. Decision log (approved in supervised sessions)

- **2026-08-04 — mapping architecture fixed; P0.2 fully resolved.** The
  synth PDB's function map has four sources of truth: (1) game code — the
  `RVA()` macros in `src/` ARE the map ("acts as a .csv"), re-derived every
  build by the future label-map generator (P5.2), no config table; (2) zlib
  — the only statically-linked code never reconstructed from `src/` —
  `config/retail-zlib-map.tsv` (rva/size/name, 68 functions, 35 proven
  symbols), which decides the zlib-CSV half of P0.2; (3) the CRT/C++
  runtime, named-but-not-matched — `config/retail-runtime-map.tsv`
  (rva/name, 1,144 functions, 408 proven symbols), rows properly mapped as
  matching proceeds; (4) the universe — `config/retail-functions.tsv`
  reused as-is to find still-unmatched functions by set difference. The
  eventual consumer is a gruntz-style README coverage table; functions not
  written directly in code (EH funclets, init/cleanup thunks, runtime
  library, import thunks) are excluded there, derivable from
  `evidence/retail-symbols.csv` tiers. Synth-PDB generation and delinking
  themselves are deliberately NOT started yet.

- **2026-08-04 — function maps admitted (superseded the same day).** An
  earlier cut emitted a combined `config/retail-function-map.tsv`
  (game+zlib); the mapping-architecture decision above removed it — game
  names belong to `src/` annotations, zlib to its own reviewed TSV.

- **2026-08-04 — source carcass admitted; P0.2 macro half resolved.**
  `src/` + `include/` created by `python3 -m homm3.carve carcass` and
  hand-owned from admission on: one .cpp per game compiland (128 TUs; 13
  zlib compilands stay vendored), functions in link order with Dreamcast
  CodeView prototypes, and per-TU headers keeping prototypes as comments
  until retail layouts are proven (attempt-1's vetted layouts remain a
  quarry, P4.2). Annotation contract: gruntz `RVA(0x<rva>, size)`/`DATA()`
  adopted as-is via `src/rva.h`, plus a `DC_ONLY(off, cb)` marker for
  Dreamcast-only procs — a claim about the DC build, never the retail
  image (750 RVA ties, 7,179 DC_ONLY at admission). The vendored-zlib
  label CSV half of P0.2 stays open.

- **2026-08-04 — target pinned; P0.1 landed.** The RE target is attempt-1's
  canonical image: English GOG Complete 4.0 (engine 3.2), 2,732,032 bytes,
  SHA-256 `057c9d88…` (Collector's `0da1c777…` kept as comparison build; both
  local copies preserved outside the repos at `../orig/`). NH3API's mapping was
  verified against it: 873/874 embedded wrapper addresses land on x86 entry
  patterns, identically in both pressings — NH3API describes the unpatched
  Complete exe, not an HD Mod address space. `README.md`, `CLAUDE.md`, and
  `AGENTS.md` written (homm2-decomp `decomp-pol-2.0` as the style template),
  carrying the target identity, the supervised-review rule, the evidence
  tiers, and the layout/gate contracts.

- **2026-07-23 — layout + lifecycle (P1.1 partially done).** Approved and
  implemented: `scripts/homm3/{core,init,build}` areas; `cc_wrap.py` moved to
  `core/`; `configure.py` moved INTO the package as `homm3.build.configure`
  (divergence from Gruntz's root `configure.py`: one package, one CLI; ninja's
  generator rule re-invokes the module); CLI rewritten Gruntz-style with
  `init | configure | build | link | clean`.
- **2026-07-23 — Wine env stays stateless.** Per-invocation INCLUDE +
  absolute-path cl (no registry PATH/INCLUDE/LIB writes); libraries pass per
  invocation at link time. Divergence from Gruntz's `configure_registry()`.
- **2026-07-23 — `homm3 init` owns the toolchain tarball unpack**, the Wine
  prefix boot, and a smoke compile that goes through the REAL
  `homm3.core.cc_wrap` path (improvement over Gruntz's bespoke smoke).
- **2026-07-23 — link phase from day one** (P6.4 pulled forward): VC6
  `link.exe` candidate link with `/FORCE /NODEFAULTLIB /MAP`, opt-in `ninja
  candidate` / `homm3 link`, unresolved-externals punch list. Finding: VC6
  LINK.EXE statically imports only mspdb60/msvcrt/kernel32 (import table
  walked); MSDIS110.DLL is dynamic-only (`/dump /disasm`), so Gruntz's
  `msdis_stub.py` is NOT needed for VC6.
- **2026-07-23 — `homm3 clean` is whitelist-based** (divergence from Gruntz's
  nuke-build/): removes only pipeline-owned entries, keeps the toolchain
  tarball and unrecognized build/ entries (hand-built vostok PR fixtures),
  and lists what it kept.

- **2026-08-04 — bootstrap analysis landed (`scripts/homm3/carve/`).** Function and
  vtable inventories carved from the retail image and admitted to `config/`
  (`retail-functions.tsv` 11,943 rows, sizes INCLUDING jump tables per the /Gy COMDAT
  extent; `retail-vtables.tsv` 363 tables), plus `retail-relocs.tsv` (56,937 recovered
  DIR32 sites, directly consumable as vostok `--reloc-manifest`) and its evidence
  sidecar, the DNA band map, per-function library attribution, and a names export.
  `docs/exe-dna.md` documents the library findings. Established: **no MFC** in the
  image (static or dynamic); **static CRT** (LIBCMT SP3) + LIBCPMT + zlib 1.1.3; all
  other middleware DLL-bound and therefore contributing zero `.text` bytes; the build
  is **non-incremental** (99.82% of direct calls land on entries, no ILT band) and
  `/Gi-`. **NH3API addresses were disproven for this image** — see CLAUDE.md.
  Carve tooling is bootstrap-scoped and retires to `scripts/archive/`.

- **2026-08-04 — vostok pinned at the stack head** (resolves open decision
  point 3): flake re-pinned from the stale `81d34b2` snapshot to `1393e24`
  (`feature/pdb-linker-trampolines`), the open-PR queue head as rebased
  upstream on 2026-08-01/02 = current master (incl. the runnable examples)
  plus 18 stacked commits. This brings the flagless stripped-PE exact
  code-relocation recovery (auto-enabled on `IMAGE_FILE_RELOCS_STRIPPED`
  images) — delinking HEROES3.EXE needs no reloc symbols. Note: the
  pre-rebase stack's top commit ("recover PDB-declared inline text data")
  was NOT rebased into the new queue; HEROES3's `.text`-embedded data may
  want it later — watch upstream.

- **2026-07-23 — repo + toolchain distribution.** Private GitHub repo
  `sushi-shi/homm3-decomp` created and pushed; toolchain tarball published as
  release `toolchain-vc6-sp3`. `homm3 init` now downloads it via `gh` when
  missing and always verifies the pinned SHA-256
  (`9fd1b3b3…ed61bc9`). `homm3 clean` therefore nukes build/ entirely,
  Gruntz-style (the earlier whitelist divergence is retired).

## 6. Vostok upstream status (checked 2026-07-23)

The flake pins `srp-survarium/vostok-delinker@1393e24`
(`feature/pdb-linker-trampolines`, re-pinned 2026-08-04 — see the decision
log): the open-PR queue head as rebased onto current master on 2026-08-01/02.
The triage below describes the state as of 2026-07-23 and is kept for the
PR-number ↔ feature mapping; everything listed as "merged" plus the whole
stacked queue is now in the pin.

**Merged on master (2026-07-20/21)** — the stripped-PE foundation HEROES3
needs: #38 optional `.reloc` + relocation-site rediscovery from the PDB, #40
reviewed reloc manifest, #11 DIR32-for-absolute/REL32-for-branches (the PR-11
fixture), #13, #36, #37, plus "emit reviewed data definitions", "emit
candidate data topology" (the PR-15 fixture's `--data-manifest` line), "share
one symbol per referenced name", and "use PDB function names verbatim".

**Still open** (needed later, in rough dependency order for us):
- #34 exact PDB code relocations for stripped PEs — the core of attempt-1's
  `vostok-exact-pdb-relocs-default`/`vostok-pdb-code-relocs` patches;
- #12 typed fallback data relocations (the x-cdtest PR-12 fixture);
- data-section-manifest family: #19, #20, #21, #22, #26, #27, #41;
- function-alias family (HEROES3's folded COMDATs): #23, #24, #25, #33;
- #29 IAT relocations from PDB; #28, #30, #31, #32, #35; #9.

Merged since the triage: #43 (docs: supported PDB format) and #42 (runnable
examples, 2026-07-26 — squashed to one commit, reviewed, extended with
04-synthetic-pdb: the no-debug-info delink via a symbol inventory +
`llvm-pdbutil yaml2pdb`, validated end-to-end with VC6 SP3 under Wine against
master). Known upstream defect found during that work: a `.rdata`-targeted
base relocation below every named constant panics
(`relocs.rs` `unreachable!("All constants must be named")`) instead of
producing a diagnostic — matters for partial-coverage synthetic PDBs during
bootstrap.

Attempt-1's remaining local patches map onto the open PRs above (aliases →
#23–25/#33; local function symbols → #31; PE layout → #32/#38-line), plus one
objdiff-side patch (`objdiff-data-symbol-details`) that is unrelated to
vostok. Fixture note: `build/vostok-pr-{11,12,15}-example` document in-flight
PRs; `homm3 clean` now deletes them, so run clean only once they are no
longer needed as PR evidence.

## 7. Open decision points (need your call before the affected step)

1. **P0.2** — annotation contract: recommendation on the table is **VA**
   (homm2's `va.h` model: VA in everything human-facing, RVA allowed in
   generated `build/` artifacts, 8-digit zero-padded style) — awaiting
   explicit sign-off. Remaining sub-questions: macro names verbatim from
   homm2? Size mandatory for functions?
2. **P2.2** — synth-PDB reconciliation: Gruntz structure + attempt-1 VC6
   knowledge; which attempt-1 behaviors (EH funclets, alias records, data
   topology companion records) are in scope for the zlib-only milestone?
3. ~~**P2.5 / flake** — vostok re-pin~~ — RESOLVED 2026-08-04: pinned to the
   open-PR queue head `1393e24` (see decision log).
4. **P4.2** — attempt-1 evidence policy: which `config/*.csv` inventories are
   re-admitted as reviewed inputs (function denominator, boundary overrides,
   Dreamcast CodeView exports), and where do candidate-only sources
   (`homm3-symbols`: Dreamcast dump, NH3API) live relative to this repo?
5. **General** — do we keep Gruntz module names verbatim (grep-ability across
   the two projects) or rename where HoMM3 semantics differ?
