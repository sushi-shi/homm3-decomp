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

- **2026-08-06 — comparison scope narrowed to FUNCTIONS ONLY.**
  User-directed: data comparison returns later as its own phase. The
  normalize step now truncates every non-code section in the
  disposable comparison copies (both sides) and re-homes their symbols
  as undefined externs; the raw compiler/delinker objects keep their
  data sections untouched, so re-admitting data is flipping one call
  in `normalize_objs`. STAMP_SCHEMA 3.

- **2026-08-06 — homm2 Gold 2.1/Buka joins the template shelf.**
  User-directed: the sibling's `decomp-gold-2.1-buka` branch (VC6 SP5,
  1727/1727 exact) is chronologically and compiler-wise closer to
  retail HoMM3 than `decomp-pol-2.0` (MSVC 4.2); prefer its statement
  shapes when both branches carry a template, and consult both.

- **2026-08-06 — text-pad trim added to the reviewed target
  normalization.** The compiled base carries each function in a /Gy
  COMDAT padded to its section alignment with 0x90 fill, while the
  delinked target packs functions at their claimed retail sizes - the
  same matched function compared at two lengths and the fill scored as
  a difference (widget::Main, byte-identical through its jump tables,
  capped at 98.58% over eight trailing nops). The canonicalizer now
  shrinks executable sections past their trailing 0x90 run on BOTH
  comparison copies, guarded: never into a relocation span, at most 15
  bytes (one alignment), and `_assert_only_canonical_changes` verifies
  the removed bytes were pure fill. STAMP_SCHEMA bumped to 2.

- **2026-08-06 — vtable NAME census moved from source `VTBL()` macros to
  `config/retail-vtables.tsv`.** User-directed. The macros (annotation
  contract v2) are retired from `include/va.h` and the two claim sites
  (basemgr.cpp, button.cpp); the tsv gains a hand-admitted `class`
  column (empty until an identity is byte-proven in supervised review)
  and is MANUAL from here on - never regenerated. `homm3.build.labels`
  now derives `??_7<class>@@6B@` label-map rows from that column
  (provenance `vtable-name`), keeping the analysis-grade
  `evidence/retail-vtable-symbols.csv` enrichment only for unnamed
  rows. First admitted names: baseManager (0x23b9bc, store inside the
  matched ctor 0x4d530), button/textButton/type_func_button
  (0x23bb54/0x23bb88/0x23bbbc, stores inside their claimed ctor/dtor
  bodies), CSprite (0x23d6b0, both stores inside the csprite.obj ctor
  region, slot 0 = the adjacent scalar deleting destructor).

- **2026-08-04 — retail LINK ORDER carved from the src/ claims; it is
  exactly ALPHABETICAL, and 977 unclaimed functions gained module
  ownership.** User-directed. `homm3.analysis.link_order` brackets each
  TU's retail span from its `VA()` claims and sorts them.
  Header-origin claims are EXCLUDED per the user's rule: an inline
  defined in a header is a COMDAT every using TU emits, the linker
  folds/drops copies, and the Dreamcast build shows one landing *inside*
  button.obj's run (`Button.h:78 button::SetText` between two
  button.cpp functions) - placement is a property of that link, not of
  source order, so counting it would stretch a span across its
  neighbour (10 such residents today, reported separately). Results:
  63 units spanned from 742 own-cpp anchors covering 739,953 B (32.8%
  of carved function bytes); **zero span overlaps** (a self-check -
  a misattributed claim would swallow a neighbour's anchor); **link
  order alphabetical with zero inversions** across all 63 (vendor
  groups differ - zlib is in library order). Because the order is
  alphabetical, every gap's owner set is exactly the unspanned
  compilands sorting between its neighbours (e.g. the 42,900 B between
  cmbtmgr and command belongs to the combat*window family), which
  `gaps.tsv` now states per gap. Evidence in evidence/link-order/
  (units/order/gaps/attribution + README); regenerate as src/ grows.

- **2026-08-04 — armygrp wave 1: the module's whole small/medium
  surface matched - 14 functions implemented, 13 EXACT (user scoped
  out the big UI/morale family).** New exact matches: HasCreatures,
  default ctor, Initialize, HasAllUndead, CanJoin, get_AI_value, Add,
  Swap (retail's `short` temp quirk reproduced), save/load (both use
  the -1/0 convention through a virtual stream interface - slot 1
  Read, slot 2 Write - modeled as TAbstractFile), GetArmySizeName
  (proven STATIC by its /Gr register args; nine 12-byte-stride BSS
  string tables at 0x6a5bb8 modeled as one 9x3 array, thresholds
  5/10/20/50/100/250/500/1000). Layouts admitted:
  TCreatureTypeTraits (stride 116 from retail index math, attributes
  @0x10 with CTA_UNDEAD=0x40000, AI_value @0x40; NH3API field roster
  lands exactly), reached via the akCreatureTypeTraits reference
  global [0x6747b0]. Profile gained /GX (funclet-proven; zero
  regressions). labels step 1b extended: ctor/dtor keys (??0/??1 ->
  class_class) and ordered-group overload adoption (claims by rva vs
  base publics by COFF section number - definition order both sides);
  claim keys strip the step-1 rva-dedup suffix. Two ratchet
  rename-artifact cleanups hand-edited (19 stale declarator rows).
  RESIDUAL: armyGroup(TCreatureType,int) at 40.5% - semantics match
  line for line but retail's loop keeps short-indexed addressing and
  memory-resident `amount` where our compile strength-reduces;
  resisted /GX and three source shapes (guard-return, hoisted short
  i); open codegen puzzle, candidate explanations: VC6 service-pack
  optimizer delta or a source spelling not yet found. Scoreboard:
  83/102 exact (81.4%) in linked units.

- **2026-08-04 — FIRST GAME-CODE MATCHES (P5.2 milestone): Random,
  armyGroup::GetNumArmies, armyGroup::IsMember - all three EXACT
  (100.0) — and the game compiler profile discovered: /O2 /Oy- /ML
  /Gr.** User-directed ("match a few functions implemented in homm2";
  in-tree, no scratchpads). Bodies adapted from homm2's exact-matched
  source (the overlap report's h2-source-template lane working as
  designed - Random is token-identical logic across the games); the
  armyGroup layout admitted to include/armygrp.h from Dreamcast
  CodeView (size 56, armies@0, numTroops@28) corroborated by retail
  codegen (7-slot loops, CREATURE_NONE=-1), with SIZE(armyGroup, 56)
  per the va.h contract. Evidence for the flags: /Gr from fastcall
  free functions (Random's args in ecx/edx); /Oy- pinned by a
  surgical sema-diff - IsMember's body was instruction-identical but
  retail keeps an EBP frame exactly where stack access exists while
  frameless GetNumArmies proves the frame is conditional (so /O2
  /Oy-, not /O1//Od). Two enabling mechanisms landed: (1) the
  `#if 0 // @carcass` bootstrap pattern - unimplemented carcass stubs
  stay lexically present (labels + the va-claims gate scan text) but
  outside compilation, so partial TUs build; (2) labels.py step 1b,
  base-obj name AUTHORITY - a compiled unit's public symbols carry
  the true MSVC spellings, adopted for uniquely-joined src claims
  (provenance src-VA+base) so delinked targets pair against base objs
  by identical names (the interim binding until the clang-IR labels
  path, P0.2; without it every game row read 0.00 unpaired).
  Scoreboard: 72/102 exact in linked units; misc + armygrp admitted
  to units.toml as bootstrap-partial TUs; ratchet grew 33 rows; all
  gates green.

- **2026-08-04 — the NWC gzio deviation admitted as a vendor patch;
  zlib CLOSED at 69/69, 100.0% exact, 100.00% fuzzy.** User-approved
  mechanism following the existing vendor convention (bink/smacker/ifc
  `<file>.patch` beside the snapshot): `vendor/zlib-1.1.3/gzio.c.patch`
  carries the single proven retail deviation (`uInt len` -> `int len`
  in check_header - retail's jl at va 0x6064b5; no official zlib
  1.0.4-1.2.1 ever spelled it signed, and the retail bytes bound NWC's
  edit to exactly this one of the tree's 226 uInt sites, since every
  other zlib function matches pristine source at 100%). Mechanism
  SIMPLIFIED on user direction to match the bink/smacker/ifc
  convention exactly: the deviation is applied IN PLACE - the vendored
  tree holds the RETAIL source state - and `gzio.c.patch` sits beside
  it as the documented delta from the official release (whose tarball
  sha stays recorded in vendor/README.md). The interim staging
  machinery (patch_src actor, configure patchsrc edge, units.toml
  patch key) was removed the same session as over-piping for a
  one-token fact; the build compiles vendor/ directly and the ratchet
  holding 69/69 across the switch proves the objects identical. One
  ratchet artifact hand-edited per doctrine: the renamed
  tr_static_init's old synthetic baseline row removed. Full build
  green: ratchet clean, all gates hold, README refreshed.

- **2026-08-04 — objdiff pinned 3.7.1 -> 3.7.3 (user-approved); the
  inflate phantom byte root-caused to an upstream ORDERING bug, fixed
  by upstream #360.** The trailing `.byte 0x90` row was not a nop-trim
  off-by-one: in 3.7.1, `infer_symbol_sizes` ran inside `map_symbols`
  BEFORE `map_relocations`, so `infer_function_size`'s inline-reloc
  skip iterated an EMPTY relocation list and the decoder read the
  jump table's raw addend bytes - inflate's last entry `e9 03 00 00`
  (case offset 0x3e9; low byte = the jmp-rel32 opcode) fabricated a
  phantom 5-byte jmp that swallowed the first pad nop (zero-addend
  tables decoded as harmless add-pairs, correct by luck - which hid
  the bug from the first synthetic repros; the e9-addend minimal COFF
  repro isolated it, archived in the session scratchpad). v3.7.3
  moves the call after `map_relocations` ("must be done after
  map_relocations is called") - shipped under the ARM-labeled
  changelog line of #360, actually arch-generic. Verified end to end:
  minimal repro 6->5, real pair both sides 1068, inflate fuzzy
  100.0 on UNMODIFIED comparison objects. flake: objdiff-src
  v3.7.3, objdiffVersion 3.7.3, new binary + cargo hashes; no
  planned PR needed (already fixed upstream), and the
  cbProcSize-trim normalization idea is retired as unnecessary.
  Scoreboard: inflate 5/5, overall 68/69 exact (98.6%), 99.99%
  fuzzy - the sole remaining zlib deficit is check_header's one
  genuine source byte (jb/jl), gated on the vendor-patch decision.

- **2026-08-04 — the 0.04% analyzed; deflate_fast boundary corrected
  1→812 (user-approved config edit); root cause identified.** The
  zlib deficit decomposed into 4 functions/9.3 weighted bytes:
  (1) `deflate_fast` carved as 1 byte at 0x205220 — proven the real
  812-byte body by full-byte comparison against the compiled base
  (785/812 identical; all 27 diffs inside reloc operands) + three
  configuration_table slots holding its VA + flush 4-nop fit against
  longest_match. Corrected in retail-functions + zlib map; re-delink →
  EXACT on first pairing: deflate 14/14, overall 66/69 (95.7%), fuzzy
  99.98%. Root cause: carve `extents.synthesize` computes
  size = max(range end) − entry; entries seeded from reloc-target
  evidence (the fn-ptr table) with NO disassembled body ranges
  degenerate to size 1. 35 such rows exist: 5 genuine 1-byte `ret`
  functions, ~30 tiny CRT/EH fragments + 4 small holes in runtime
  territory (excluded classes — harmless), deflate_fast the only one
  in matchable territory. Its table-only reachability (no direct
  caller) is why traversal missed it while deflate_slow/stored (same
  table) happened to carve fully. (2) the 1-byte `ret` at 0x206e40 is
  provably `@tr_static_init@0` (called by _tr_init at the same +3
  offset as our base reloc; empty body under STDC static-trees;
  rename APPROVED and applied: trees 20/20, overall 67/69 at 97.1%.
  Post-hoc lesson: base trees.obj emission order and retail trees
  address order agree position-for-position (modulo /OPT:REF-removed
  _tr_tally), so an incremental-RVA positional join would have named
  the content-free 1-byte body that byte fingerprinting structurally
  cannot - position and content are complementary channels. Also
  removed the phantom 0xf004e row (two alignment nops mis-seeded as a
  target function; inventory 11,944 -> 11,943) and audited all size-1
  rows: 5 genuine ret-stubs, ~30 mis-sized tiny CRT/EH fragments in
  excluded runtime territory, correction batch optional).
  (3) check_header 99.50%: retail source
  variant `int len` vs stock 1.1.3 `uInt len` (jl vs jb) — reaching
  100% needs a vendor-deviation decision — PROVEN fixable by an
  out-of-tree wine-cl experiment: the one-token variant flips the
  branch byte 0x72->0x7c (jl) and matches retail 310/310 modulo
  relocs (10-byte nop tail = normal padding, present in stock too).
  (4) inflate 99.74% — TWO earlier claims corrected in sequence:
  not a source variant (stock 1.1.3 inflate.c is byte-EXACT with
  retail modulo relocations, 0 non-reloc diffs across all 0x42c
  bytes, all 14 jump-table case targets identical fn-relative), and
  NOT the table representation either - objdiff-cli's own row diff
  penalizes exactly SIX rows and zero of them are table rows:
  5x DIFF_ARG_MISMATCH = the five `mov [esi+0x18], <errmsg>` sites
  (inflate's z->msg strings "unknown compression method"/"invalid
  window size"/"incorrect header check"/"incorrect data check"/"need
  dictionary") where the compiled side names the literal ??_C@... and
  the delinked side only has data_<rva> - zlib's DATA statics were
  never named (the map is functions-only); 1x DIFF_INSERT = a
  trailing alignment nop at 0x42c, candidate-only because COFF
  symbols carry no size and objdiff extends to the section end while
  the target symbol is exactly 1068 B (retail's identical 4-nop pad
  sits outside its span). FINAL decomposition (user-driven, proven by
  experiment): the report's byte-FUZZY - what the ratchet consumes -
  charges ONLY the trailing pad: trimming the 4 nops from a scratch
  copy of the candidate (arg mismatches untouched) yields fuzzy
  100.0 exactly. The five ??_C@-vs-data_<rva> reloc-name rows are
  display-level only under our scoring, which is sound because
  vostok recovers reloc sites+targets exactly from the synth PDB -
  names are presentation. So: inflate to 100% = trailing-pad
  handling in normalize (tooling, tiny); string-literal naming
  demoted to a navigation/readability nicety; check_header = the
  same pad handling + the one genuinely-source jb/jl byte, gated on
  the vendor-patch decision.

- **2026-08-04 — HoMM2↔HoMM3 symbol overlap measured; report in
  docs/homm2-symbol-overlap.md.** User-directed investigation of the
  sibling homm2-decomp's CodeView-authoritative symbols against our
  DC/NH3API corpora. Verdict: direct source-level lineage — 53/95 TU
  stems recur, 43 exact class-name matches (basewin + game core), 611
  normalized function-name pairs (461 backed by homm2's exact-matched
  source), 57 rare-string anchors (50 naming still-unnamed HoMM3 fns;
  flagship: the `%sattk.82M` family pins our creature-resource loaders
  to homm2's `army::LoadResources`). Calibration on 20 name pairs
  (sizes 0.26x–2.10x across MSVC 4.2→6.0) rules OUT a byte-identity
  lane — the boost is names-via-anchors + source templates, both
  external-candidate grade, admission paths in the report. Negative
  controls held except the documented generic-name collision
  (homm2 bzip `compress`/`uncompress` vs our zlib). Tables in
  evidence/homm2-overlap/ (removable); the one-shot script retired to
  scripts/archive/homm2_overlap.py on landing, per plan.

- **2026-08-04 — single_view ported (user-picked): every global gets
  ONE view.** gruntz audit/single_view landed as
  `homm3.match.single_view` (our fatal-gate area, not audit/ - noted
  divergence): a global declared under two (type, linkage) signatures
  is a split view - only one spelling can match retail's symbol, the
  other is a fake alias a candidate link cannot resolve. Frozen-backlog
  shape shared with verify_va_claims (config/single-view-baseline.tsv;
  new splits fatal; never in `--fast`). The tree declares zero externs
  today, so it lands as pure arrival-prevention with an empty backlog.
  Port fixed a real adaptation bug its own selftest caught on landing:
  the shared string-stripper ate the `"C"` in `extern "C"`, erasing the
  linkage distinction - now sentinel-protected before stripping.
  Drilled live: a planted int/short split across include/ + src/ dies
  naming both views and files. Known blind spot inherited from gruntz
  (documented): template types with pointers/multiple args don't fit
  DECL_RE. The 11-claim VA backlog stays frozen (drain deferred); the
  strings probe already confirmed the real `Initialize*TraitsTable`
  loaders are correctly claimed at 0x4e67a0/0x4e6920/0x4e6b10 via
  their hotraits/hctraits/sstraits.txt literals.

- **2026-08-04 — the VA-claims gate + three more board rows (the
  user-picked ratchet batch); first real catch: 11 mis-landed
  linkorder claims.** New FATAL gate `homm3.match.verify_va_claims`
  in the normal build tail (never `--fast` - gates belong to the
  orchestrator's loop, the matcher's inner loop stays lean): UNIQUE
  (one VA = one claim tree-wide), RECONCILED (every claim is a carved
  entry, claimed size == admitted size), CLASSIFIED (VA() must land on
  target code; VA_COMPGEN may also claim init-thunks), IN ORDER
  (VA() strictly increasing per file - VA only, DC_ONLY is being
  removed as functions gain retail claims and is not order-checked).
  Selftest (9 synthetic defects + clean control) runs every
  invocation; live drills: size typo, duplicate, order swap each
  fatal. **First run finding: all 750 claims are unique, sized
  exactly, and in perfect link order, but 11 CLASS violations are
  real** - linkorder-grade DC names (InitializeHeroTraits et al.)
  bracketed onto addresses that are byte-provably compiler-generated
  initializer thunks (guard byte + _atexit registration; e.g.
  0x4e6d60). The transfer slid across interleaved $E thunks. Frozen
  as a known backlog in `config/va-claims-baseline.tsv` (the gruntz
  single_view shape: backlog reported as standing debt and drained in
  supervised claim review; NEW violations fatal; `--write-baseline`
  re-freezes only after review). Board grew three ratchet rows, all
  floors blessed at 0: `.cpp-local enums`, `casts to enum types`
  (static_cast into any tree-declared enum, found via a per-run enum
  registry - an enum-to-enum cast usually means two mis-modeled
  domains; floor raises only via explicit `--update`), and `unnamed
  domain compares` (`== 0`/`== 1` exempt). 61 selftest samples across
  8 metrics; all three new rows drilled fatal-and-unblessed live.

- **2026-08-04 — sema first-wave adversarial review: fixes + one
  inventory admission (branch `sema-review-fixes`, user-directed).**
  Review confirmed the ported engine byte-faithful to homm2 and the
  decision-log claims live (zcalloc's two registrations exact;
  check_header jb→jl = SIGNEDNESS rc=1). Fixes: rc wording — the
  default skeleton compares flow+size only, so "rc=1 when the sides
  differ" overstated it (docstrings + cli help now say "when the
  requested view differs"); `--asm`/`--verbose` documented as noisy on
  matched functions (the delinked target names data relocs
  synthetically — `data_<rva>`, or neighbor+addend folded into the
  immediate — so reloc spellings diverge with zero retail-byte
  difference; observed rc=1 on objdiff-100% `@deflateInit2_@32`);
  `image_text` clamps a span to its section's raw extent;
  `objdump()`'s missing-normalized hint said `homm3 delink`, now
  `homm3 build`; `classify(image=)` stops the second load+hash of the
  exe per `rva` run; `_src_locs` now matches VTBL/VTBL2 (address is
  the LAST macro argument — the old first-argument pattern could never
  hit) and anchors the macro name; the rva dossier prints the OWNER's
  src claim for inside-body addresses and says "+N more" when >4
  vtables hold an entry; `strings 0x<literal>` answers the reverse
  question instead of printing `?`; `xref --raw` without `--flat` dies
  instead of being silently ignored; the sema log records the argv the
  call actually parsed; `normalize_objs` skip now uses
  `freshness_problems` itself (content identity, not mtimes — kills
  the stale-stamp wedge where build said fresh and sema said stale),
  with the canonicalizer-code-identity trade-off documented
  (STAMP_SCHEMA is the invalidation lever); the freshness gate gained
  its CLAUDE.md-mandated negative controls
  (`homm3.build.test_normalized_freshness`, 5 controls, standalone
  runnable). **Inventory admission (needs the user's merge review):
  `zcfree` at rva 0x206c90, 8 bytes** — `config/retail-functions.tsv`
  had a hole between 0x206c70 (zcalloc) and 0x206ca0 (adler32); the
  bytes there are `52 e8 4a 29 01 00 59 c3` (`push edx; call free;
  pop ecx; ret`, fastcall @zcfree@8), and retail deflateInit2_ /
  inflateInit2_ store this VA as the default `zfree` (site 0x2048b3
  holds 0x606c90) — the delinker had been spelling it
  `@zcalloc@12+0x20`, the direct cause of the false `--asm` diffs.
  Rows added to retail-functions.tsv and retail-zlib-map.tsv
  (`0x206c90 8 @zcfree@8 zutil`). NOT fixed (deliberate):
  `parse_ins`'s byte-column/mnemonic ambiguity and the skeleton
  census wording stay verbatim-homm2 (port fidelity, no observed
  failure); the cli REMAINDER `--help` stub matches every other
  subcommand.

- **2026-08-04 — cleanliness area started: C-style casts BANNED at 0
  (the first board metric).** User directive "ratchet that we don't use
  C-casts at all"; both siblings surveyed first. gruntz precedent
  adopted: `cleanliness/board.py` scoreboard over comment/string-stripped
  src/ + include/, committed floors in `config/cleanliness-baseline.tsv`,
  build rolls floors DOWN-only (min(count, floor) — a regression is never
  blessed), `board --update` the one deliberate bless, FATAL gate in the
  `homm3 build` tail when a ratcheted metric rises (gruntz's own
  cast-metric-policy bans C-casts the same way; homm2 has no board — its
  discipline is hard header-drift gates + the constants audit). Detected
  shapes: builtin/numeric casts, pointer casts, casts applied to `this`;
  the bare value-cast `(Foo)x` is a documented regex gap left to review
  (same gap gruntz accepts). The gate self-tests on EVERY invocation
  (8 embedded positives must be detected, 13 negatives must stay clean —
  including the backtick-destructor apostrophe line), drilled live:
  planted `(int*)`/`(void)` casts → build gate rc 1 naming file:line,
  floor stayed 0, revert green. The tree starts clean: 0 casts across
  66,580 lines. Future metrics land as board rows, not new packages.
  Same day, user-directed, four more RATCHET rows (ratchets, not
  policy bans - floors drain, no new arrivals): `reinterpret_casts`
  (named-cast debt), `cpp extern decls` (violation message teaches the
  fix: declare once in the owner header, consumers #include it),
  `.cpp-local views` (a type's one true shape belongs in include/),
  `magic case labels` (declare the domain enum). All five floors bless
  at the tree's current 0; per-metric selftests (43 samples) run on
  every invocation; extern + line-start `case 7:` drills fired with
  their fix notes and left the floors un-blessed.
  = xref / diff / disasm / rva / strings.** gruntz's architecture (one
  process over a lazy `Context`, rc 0/1/2 with `die()`=2, the
  `build/homm3_sema.log` usage feed `[date][time][rc]: cmd` ported
  verbatim as a compatibility contract) + homm2's diff engine (one
  llvm-objdump over the normalized objects, block-index CFG comparison,
  symbolic `--branches`, freshness-guarded inputs — `normalize_objs`
  now writes the provenance stamps the guard verifies). **Flag polarity
  redesigned per user directive, diverging from BOTH siblings:** `diff`
  is its own subcommand whose default is the block-SKELETON diff
  (`--verbose` = block bodies, `--asm` = the old flat masked diff,
  `--branches` = the flip/topology comparison); lite is the default
  everywhere with `--verbose` opting into columns (killing the sibling
  wart where `--diff --lite` was a silent no-op); `xref` defaults to
  the caller TREE (depth 4, `--flat` opt-out). homm3 adaptations
  stronger than either sibling: attribution over the COMPLETE 11,943-fn
  universe; data refs from the admitted dir32 reloc sites (operand
  read from the image - no blind byte scans, zcalloc's two fn-ptr
  registrations found exactly); universe classification as the tree
  frontier (`[runtime - frontier]`); capstone image path renders ANY
  retail function with `<symbol>` annotations (the case homm2 cannot
  do). Verified live: `--branches` classified check_header's real
  jb→jl divergence SIGNEDNESS rc=1. SymbolDb stays in `sema/context.py`
  until a second consumer justifies `core/symbols.py`. Deferred:
  `--switch`, `--dot`, batch `sema -`, map/class/vtable, clangd, --rich.

- **2026-08-04 — the build loop integrated (P3.1 partial): `homm3 build`,
  `--fast`, the ratchet, `delink`, `status`, README score block.** One
  module per command, cli.py is pure dispatch (`homm3.build.build`,
  `homm3.build.delink`, `homm3.match.status`). `homm3 build` = configure →
  ninja → normalize → objdiff report → overall line → baseline raise
  (monotone, 4-decimal quantized) → **ratchet check, FATAL on a drop**
  (deliberate divergence: both siblings keep the per-function baseline
  observational; here a regression fails the build, drilled poisoned-row →
  exit 1) → README `<!-- match-score -->` block (gruntz shape, full-engine
  denominator via `homm3.match.universe` — the single authoritative
  classifier, evidence-free: retail EH walk for the 5,125 funclets, the
  runtime/zlib config maps, the carve init-array for 1,119 init thunks,
  FF 25 scan for 27 import thunks; total reconciles to the carve's
  11,943 fns / 2,253,513 B exactly; `(unmatched)` row + excluded-category
  table included) → warning-only
  stale-delink probe (homm2's drift-census idea; build never RE-delinks —
  a fresh tree with no targets bootstraps the first delink, since the rule
  protects an existing comparison target, and from nothing there is
  nothing to protect).
  `--fast` stops after the %% line and says so. Baseline
  `config/match_baseline.tsv` (`unit fn max_fuzzy`; the src_hash epoch
  column arrives with the clang fingerprint path). No report caching yet
  (sub-second at 14 units). Ghidra init deliberately untouched.

- **2026-08-04 — zlib map completed from base-obj identity; evidence/
  declared scaffolding.** Three user decisions, one measurement: (1) the
  zlib map carries its own `unit` column (rva, size, name, unit) — file
  locations live in the map, not joined from evidence at run time; (2)
  `evidence/` WILL BE REMOVED — the runtime pipeline may not depend on it.
  `labels.py` now derives VA-row names from the source declarator itself
  and treats evidence as optional enrichment (drilled: labels runs with
  evidence/ absent); (3) a one-off carve script
  (`scripts/homm3/carve/zlib_names.py`, not a pipeline actor) recovered
  real names for
  the map's working labels by masked per-function identity against OUR OWN
  compiled base objects (pass 1 unique-in-extent, pass 2 hdmap-style
  brackets, monotonicity gate): **32 of 33 labels became real symbols
  (`@longest_match@8`, `@build_tree@8`, …), all 35 previously proven names
  independently corroborated**, one 1-byte sliver unmatched. Result:
  **objdiff overall fuzzy 55.8% → 99.96%, 64/68 functions byte-exact, 10
  of 14 zlib units at 100%**. Also: canonical content digests shortened to
  6 hex chars (readability; deterministic lengthening on collision).

- **2026-08-04 — the delink loop wired (P2.3); first REAL objdiff numbers.**
  Correction to the entry below: the smoke delink proved vostok consumes
  the synth PDB, but objdiff still compared every unit against dummy.obj.
  Now: zlib functions carry per-MEMBER units (DNA attribution,
  link-order fill), `homm3.build.delink` runs
  labels → synth_pdb → data_manifest → vostok → copies the units.toml
  scope into build/objdiff/target/ → canonicalizes both sides →
  re-emits objdiff.json against the NORMALIZED copies. Measured on the
  spot: **34/68 zlib functions match retail exactly; overall fuzzy 55.8%**
  (9 units at 100% — adler32, compress, crc32, infblock, infcodes,
  inffast, infutil, uncompr, zutil; inflate 99.8%). The gap is
  concentrated where the zlib map still holds working labels (33
  functions) that cannot name-pair against base symbols — completing
  that map (P2.1 authority path) is the direct next lever. Also per user
  decision: content-derived canonical names truncate their digests to
  6 hex chars (deterministic lengthening only on in-object collision).

- **2026-08-04 — synth PDB delivered (P2.2); the delink lessons implemented
  in full.** Annotation contract v3: homm2-decomp's `va.h` vocabulary
  adopted verbatim (`VA`/`VA_COMPGEN`/`DATA`/`DATA_COMPGEN(_GUARD)`/`VTBL`/
  `VTBL2`/`OVERRIDE`/`SIZE`, absolute VAs in source, plus our `DC_ONLY`) —
  superseding the same-day gruntz `RVA()` choice; `src/` regenerated under
  it (750 `VA`, 7,179 `DC_ONLY`), `src/rva.h` retired for `include/va.h`.
  New build actors: `homm3.build.labels` (symbol inventory: 25,923 rows —
  11,943 functions total-covered + 13,980 data symbols incl. dense
  const/data/bss naming of every absolute-reloc target, vtables, IAT
  slots), `homm3.build.synth_pdb` (yaml2pdb + DBI stream-0x14 patch →
  `build/pdb/HEROES3.pdb`, 100 modules), `homm3.build.data_manifest`
  (vostok tsvs at the PINNED 1393e24 schemas — 8-column data manifest, no
  `--contribution-manifest` at this pin — plus the canonicalizer's
  DATA_COMPGEN bindings table and hand-owned
  `config/delink-reloc-aliases.tsv`), and the transform-before-compare
  machinery ported from homm2 (`canonicalize_data_symbols` with `__h3cg$`
  semantic prefix + `normalize_objs`), landed ahead of its first objdiff
  user by design. **First smoke delink of HEROES3.EXE succeeded**:
  `--reloc-manifest config/retail-relocs.tsv` + the synth PDB → 101
  per-TU COFF objects (`advmgr.c.obj`, …, `_msvc_internal/*`) in 0.1 s,
  exact-PDB-operand code-reloc recovery active, no `.rdata`
  all-constants-must-be-named panic. The delink LOOP (ninja wiring,
  objdiff pairing, P2.3) remains deliberately unstarted.

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
