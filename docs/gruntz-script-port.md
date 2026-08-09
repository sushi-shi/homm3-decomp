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

- **2026-08-09 — the first `advManager::SetRolloverText` slice is
  reconstructed at 29.4501%.** Retail's 216-byte object dispatch table fixes
  every routed adventure-object id and its handler entry; the reconstructed
  slice admits the common player/current-hero and trigger-cell setup, the
  generic special-terrain/object-name fallback, Arena visitation, eight
  global/hero visitation arms, all five creature-bank variants, the three
  shrines, Tree of Knowledge, Witch Hut, and the common rollover draw. The
  remaining specialized handlers intentionally retain the retail fallback
  behavior for now rather than acquiring guessed details. The retail
  disassembly, relocations, four format literals, switch bytes, and measured
  objdiff delta are the correctness evidence; Dreamcast CodeView contributes
  only the function signature and already-admitted enum/member names. The
  full build raises whole-linked fuzzy coverage from 42.699493% to 43.39%
  without reducing the 592 exact functions.

- **2026-08-09 — `advManager::ProcessSearch` reconstructed at 91.3120%.**
  The 1,453-byte retail Grail-digging command now reproduces all 32 branches:
  full-movement and backpack gates; AI puzzle-guess invalidation; water and
  diggability rejection; dig-hole insertion; Holy Grail discovery, dialogs,
  samples and artifact award; all-player puzzle-location recomputation; and
  the final movement, route, locator and button updates. Retail instructions
  and relocations prove every condition, constant, text-record offset, game
  field and callee. Dreamcast CodeView contributes only the signature and the
  attested local roles (`curr`, `sample2`, `i`, `tempCell`, `artifact`); no
  external implementation body was used. The residual is instruction/register
  selection plus known synthetic-symbol relocation naming. The shared
  `initialize_game_data` tripwire remains exact, and `match_baseline.tsv`
  remains generated only by the full build.

- **2026-08-09 — `advManager::ProcessHover` reconstructed at 69.3789%.**
  The 2,328-byte retail body now delegates non-local turns to the waiting
  hover path; caches and visibility-checks the map tile; resolves rollover,
  owned hero/town and allied shipyard cursors; clears stale paths when terrain
  and boat state reject the destination; seeds and indexes the retail
  30-byte path grid; converts movement cost to a capped four-day cursor; and
  dispatches the anchor, boat, garrison, hero, monster and town cursor cases.
  The outside-map arm preserves scroll pointers only in the 16-pixel edge
  zone. Retail proves every predicate, cursor/command ordinal, path formula,
  object case and field offset. Dreamcast CodeView supplies only names and
  layouts: `DebugViewAll` +0x3d, `cursorType` +0x1f0, hero `maxMobility`
  (adapted as `maxMovePoints`) +0x49, the garrison army at +4, and the inline
  path helpers. The remaining delta is VC6 scheduling around the packed hero
  location, Dinkumware result-vector erase and shared exit blocks. No external
  implementation body was used, and `match_baseline.tsv` remains generated
  solely by the full build.

- **2026-08-09 — `advManager::ProcessWaitingHover` reconstructed at
  74.8967%.** Retail first bounds the pointer against the adventure map,
  converts its pixels to cached tile offsets and a packed map point, and
  admits rollover detail only when the point is valid, visible to the local
  player and on the current hero's map level. It then resolves the cell,
  updates rollover text, and selects the owned-town or local-human-hero cursor
  command; outside the map it preserves active scroll cursors only inside the
  16-pixel screen-edge zone before forwarding hover to the adventure window.
  The compiled and retail bodies have the same 975-byte target span, 27
  conditional branches and four returns. Two case-layout branch polarities
  and VC6 register/stack scheduling around the inlined packed-point helpers
  remain. Retail proves the complete control flow, screen/scroll bounds,
  pointer frames, object/owner gates and the `advManager` fields at
  +0x40/+0xe8/+0xec/+0xf0; Dreamcast CodeView supplies the function, local,
  field and helper names only. No external implementation body was used, and
  the generated baseline remains build-owned.

- **2026-08-09 — `set_witch_hut_help_text` reconstructed as a `QuickInfo`
  prerequisite at 98.0056%.** Retail copies the fixed Witch Hut name, gates
  details on the trigger bit and the seven-bit no-skill sentinel, checks the
  cell-knowledge bits, extracts the signed secondary-skill id, formats the
  16-byte trait row's name, and reports when the current hero already knows
  that skill. The unknown-cell path separately checks `WitchHutInfo` and
  appends its fixed known-skill text. All ten branches and the return agree.
  The residual is register allocation around the local-player value and the
  shared `PlayerKnowsCell` dword mask where retail narrows both operands to
  bytes. Retail proves every mask, the 50-byte local buffer, trait stride/name
  field, hero skill byte band, text-record offsets and fixed pointers;
  Dreamcast CodeView supplies the function, `TSecondarySkill`, `TSSkillTraits`
  and info-flag names only. No external implementation body was used, and the
  generated baseline remains build-owned.

- **2026-08-09 — `SetTreeHelpText` reconstructed as a `QuickInfo`
  prerequisite at 99.7414%.** Retail copies the fixed Tree of Knowledge name,
  gates detail on the trigger bit, checks the `TreeOfKnowledgeInfo` visit bit
  and cell-knowledge bits, selects one of three signed price states, and then
  reports whether the current hero's +0x6b visit bit is set for the cell's
  low-five-bit id. All nine branches and the return agree. The residual is
  register allocation around the cell/info-level lifetimes and a dword-width
  `PlayerKnowsCell` test where retail uses the low bytes. This also proves that
  text-record +0x584/+0x588 are generic visited/unvisited labels rather than
  creature-bank-specific strings. Retail proves both pointer tables, the two
  fixed text pointers, every bit extraction and the hero field; Dreamcast
  CodeView supplies the function/helper names and the `WiseTreePrices` domain
  only. No external implementation body was used, and the generated baseline
  remains build-owned.

- **2026-08-09 — `SetShrineHelpText` reconstructed as the next `QuickInfo`
  prerequisite at 99.3641%.** Retail copies the cell object's display name,
  gates detail on the trigger bit, bounds and tests the local player's global
  shrine-info bit, separately tests the cell-knowledge bits, extracts the
  signed shrine spell id, formats the spell name, and appends the current
  hero's spell-availability annotation. All nine branches and both returns
  agree. The remaining two instruction differences are a byte-width versus
  dword-width bit test and retail's redundant reload of the saved destination
  pointer on the unknown-cell path. Retail proves the 32-byte flag band at
  `game+0x4e344`, spell display name at traits +0x10, hero availability byte at
  +0x430, both text-record offsets, and the two referenced pointer tables;
  Dreamcast CodeView supplies `GlobalInfoFlags`, the inline helper names and
  the function signature only. No external implementation body was used, and
  the generated baseline remains build-owned.

- **2026-08-09 — two army/creature-bank text builders admitted as
  `QuickInfo` prerequisites.** The retail-only 893-byte army describer at
  0x40abe0 consolidates repeated stacks, emits either a compact-prefix full
  list or an approximate size/name pair, and guards variable creature names;
  its operation set and caller role support the provisional
  `get_army_help_text` name even though no standalone Dreamcast copy survives.
  It enters comparison at 54.9494%. The 636-byte, DC-named
  `get_creature_bank_help_text` copies the 400-byte trait record's Dinkumware
  string name, checks the short-width player-visit bits, distinguishes unknown,
  emptied and guarded banks, and chooses full versus parenthesized approximate
  army text. Restoring the inline visit helper and recomputing the 108-byte
  bank record only in the selected branch raises it from its uncompiled 0% row
  to 85.3542%. Retail instructions prove the 400/108-byte strides, the game
  vector at +0x4e3d8, every bit mask and all text-record offsets; Dreamcast
  CodeView supplies surviving names and the bank/helper types only. The still
  ambiguous 0x40d670 sibling remains unclaimed. No external implementation
  body was used, and the generated baseline remains build-owned.

- **2026-08-09 — army morale/luck descriptions restore bounded creature-name
  selection.** Retail checks variable creature ids against the inclusive
  0..150 range before reading the plural-name field and otherwise supplies the
  shared empty rollover text. Capturing that repeated inline idiom raises the
  2,140-byte `armyGroup::get_morale_description` body from 63.9572% to
  67.5649% and the 968-byte `get_luck_description` body from 60.3234% to
  74.7874%. The fixed Angel/Archangel and Halfling paths remain direct after
  VC6 constant propagation, while enemy Dragon and Devil selections retain
  the retail guards. The behavior and the 150 bound come directly from retail
  instructions and relocations; the shared string's address is already
  established in-tree. No external implementation body was used, and the
  generated baseline remains build-owned.

- **2026-08-09 — the 2,446-byte `advManager::DrawAdvObj` body rises
  85.9125% -> 86.3772% by restoring packed-offset widths.** Retail's initial
  object-vector gate materializes the element count before testing it, which
  selects the explicit `size() > 0` source form. More significantly, its
  packed object-cell nibble arithmetic stays in CL/AL before sign extension;
  declaring the extracted x/y offsets as signed bytes recreates that width
  and instruction tree. All 72 symbolic branches and seven returns still
  agree. An explicit manager alias, base/tile declaration-order probes,
  function-lifetime object locals, an ObjCell reference/register hint, and an
  algebraically reordered bit expression were byte-inert or regressive and
  were removed. Applying the byte widths to `DrawAdvObjShadow` regressed that
  twin 83.2068% -> 78.7790%, so it too was restored. Retail bytes selected the
  retained forms; no external implementation was used, and the generated
  baseline remains build-owned.

- **2026-08-09 — `combatManager::SetupAdjacencyArray` reconstructed
  byte-exact (324 bytes, including its jump table).** Retail walks all 187
  combat indexes and six directions, derives row and column independently,
  rejects the two off-field margin columns, computes each neighbour from row
  parity, and admits it only when both its index and resulting column are on
  the playable grid. The six direction names remain honest ordinal
  placeholders; all values and formulas are direct retail-byte proof. The
  natural nested loops, `ValidHex` inline boundary and switch reproduce every
  instruction on the first build. One apparently odd source detail is also
  byte-selected: the switch destination is declared before the loops without
  an initializer. Although directions 0..5 assign it on every reachable
  iteration, VC6 retains a default switch edge and loads the prior value at
  entry exactly as retail does. This is the HoMM2/Gruntz source-lifetime and
  helper-boundary method applied without external code; `decomp-attempt-1`
  was not used.

- **2026-08-09 — `combatManager::RaiseDoor` reconstructed from unscored to
  86.9340% (375 bytes).** Retail exits unless a defending town exists, the
  bridge is down, cells 96 and 95 have neither an army nor the +0x1c blocker,
  and (for a Fortress) cell 94 passes the same pair of tests. Quick combat
  raises the state directly to 3; otherwise the shared sound/bounds path
  draws states 1/2/3 and waits for `drawbrg.82m`. All nine entry guards, the
  town-type branch, exact IsQuickCombat expansion, aggregate destination
  formation, loop and calls are byte-aligned with retail. The residual is
  LowerDoor's same VC6 inliner-layout family (local fallback before the
  animation instead of after, splitting retail's shared animation/guard
  epilogue) plus the three known interior DATA-relocation addends. Reusing
  the exact helper and aggregate is the HoMM2/Gruntz canonical-form choice;
  no external implementation or `decomp-attempt-1` material was used. One
  pipeline detail was proven along the way: the two methods' identical
  `drawbrg.82m` literals need one `DATA_COMPGEN` claim and one plain use in
  the same TU, letting VC6 pool both references into the single retail
  compiler-generated string without a duplicate-RVA label claim.

- **2026-08-09 — combat quick mode and drawbridge lowering reconstructed.**
  `combatManager::IsQuickCombat` is byte-exact (113 bytes): the special-mode
  byte vetoes quick combat; a network battle with both per-side latches set
  requires both indexed players' `quickCombat` dwords; every other case uses
  the registry-backed `Quick Combat` preference at 0x69877c. The local DC
  public name (`?IsQuickCombat@combatManager@@QBA_NXZ`) corrects the carcass
  prototype to `const bool`, and the retail body corrects the stale
  `army_WaitSample` working identity at 0x46a4a0. Writing the two player tests
  as one `&&` is the decisive HoMM2/Gruntz canonical-source lever: it shares
  retail's early false return and removes both the duplicate epilogue and the
  second-result `setne`.
  `combatManager::LowerDoor` is reconstructed from unscored to **83.3951%**
  (275 bytes). Its inline query selects either an immediate state 1 or the
  full path: play `drawbrg.82m`, copy the 16-byte drawing-bounds aggregate,
  draw states 3/2/1 with the retail six `DrawFrame` arguments, reopen the two
  pathfinding gate cells, and wait for the sample. The aggregate assignment
  reproduces retail's destination-pointer formation, sound-return lifetimes,
  loop and calls instruction-for-instruction; three interior source-data
  relocation addends remain subject to the already-recorded DATA-size
  contract limitation. The other residual is VC6 block placement: even the
  exact helper source puts the local-preference fallback before the animation
  when inlined here, while retail puts it after. Natural if/else, inverted
  if/else, early-return, explicit-label and sequential-return forms were
  measured; 83.3951% is the best semantics-preserving form. This also proves
  `combatManager+0x54a8` as the two player ids and +0x13d38 as the copied
  16-byte bounds. `decomp-attempt-1` was surveyed read-only and does contain
  old bodies for both names, but they were not opened, copied or used; all
  admitted implementation and identity evidence came from current retail
  bytes and the local Dreamcast public-name inventory.

- **2026-08-09 — `combatManager::LoadWallTraitsTable` reconstructed
  byte-exact (143 bytes).** The loader opens `walls.txt`, rejects a missing
  or sub-179-row sheet (disposing only the undersized resource), then walks
  nine town groups of eighteen wall rows with two skipped rows before each
  group. Each row supplies the wall record's name and parsed hit points.
  Retail proves the row schedule, 9×18×36-byte destination extent, and both
  written offsets; Dreamcast CodeView supplies only the `TWallTraits` names
  and corroborating 36-byte layout. Reusing the already retail-proven
  `TSpreadsheetResource::GetNumberOfRows`/`GetRow` source boundary from the
  animation-table loader lets VC6 reproduce every instruction on the first
  build. This directly applies the HoMM2/Gruntz rule to reuse canonical
  library/source forms. No external implementation or `decomp-attempt-1`
  material was used.

- **2026-08-09 — `combatManager::GetBackgroundName` reconstructed
  byte-exact (197 bytes).** Retail's sixteen-block cascade first selects a
  town-siege background, then a non-default magic-terrain background, then
  the boat/deck/beach special cases, and finally one of a 9-by-3 ordinary
  terrain/tree-density table. The three .rdata pointer tables and three
  direct filenames were decoded from the executable; the method always
  performs the two trailing animation-state resets. Naming the
  `MoreTreesNear` result before the combat-terrain snapshot preserves
  retail's EAX/ECX lifetimes and closes the sole first-pass residual. This
  applies the HoMM2/Gruntz named-lifetime and multidimensional-layout rules;
  no external implementation or `decomp-attempt-1` material was used.

- **2026-08-09 — `combatManager::UpdateArmyGroup` reconstructed byte-exact
  (218 bytes).** Retail first clears all seven slots in the selected
  persistent army group, then walks the side's live combat stacks. Positive
  survivors pass four `creatureId` bit gates and a conditional fifth rule,
  require an original slot in 0..6, and write their current creature type and
  troop count back to that slot. The offsets, bit numbers, conditional gate
  and loop bounds all come from retail. Reusing the already retail-proven
  header-inline `army::Is(bit)` is the crucial HoMM2/Gruntz canonical-source
  boundary: it anchors VC6's induction pointer at `creatureId` and closes the
  first-pass 97.4085% residual. Dreamcast corroborates the method identity,
  `origPos` field name and inline call; no external implementation or
  `decomp-attempt-1` material was used.

- **2026-08-09 — `combatManager::GenerateMap` reconstructed from unscored to
  58.8421% (220 bytes).** Retail initializes the complete 11-by-17 combat
  grid: eight signed-short screen-coordinate fields, five empty-stack
  sentinels and three cleared state fields per 112-byte cell. The natural
  nested loop has the same five-block CFG, bounds and store semantics. The
  residual is whole-loop allocation and strength reduction: retail spills
  the row plus three derived coordinates and retains -1 in EBX, while this
  compile retains/strength-reduces the row and emits immediate sentinels.
  Cached versus repeated `GetCell`, function/block row lifetimes, a shared
  sentinel and the nested `GetHexIndex` header boundary were measured; the
  best canonical form is retained under the HoMM2/Gruntz bounded-hypothesis
  rule. Dreamcast contributes only the method/helper identities and local
  name; no external implementation or `decomp-attempt-1` material was used.

- **2026-08-09 — `combatManager::PlaceLargeObstacle` reconstructed to
  99.9888% (264 bytes).** Retail's second stack parameter is a special-
  terrain mask absent from the older Dreamcast signature. A
  `TPickANumber(0,33)` loop rejects catalogue rows until either unsigned
  mask matches, routes exhaustion directly through the inline destructor,
  then walks up to 25 signed-short cell indexes (terminated by -1), marks
  each cell with bit 2, records the selected id and returns the marked count.
  The natural early-return picker loop reproduces all ten blocks and every
  instruction. The score sliver is stripped-target EH/interior-data
  relocation representation and one unused post-store LEA addend, not code
  behavior. Retail proves the 68-byte row and all used offsets; Dreamcast
  supplies the method and picker identities only. No external implementation
  or `decomp-attempt-1` material was used.

- **2026-08-09 — `combatManager::RaiseSkeletons` reconstructed to 93.2203%.**
  The function first merges the pending raised stack unchanged; if the
  destination group has neither a matching nor an empty slot, it promotes
  the creature through the same 113-byte family lookup called by retail's
  `UpgradedCreatureType` wrapper, converts the count as `(2*n+2)/3`, and
  retries. The four original elementals map to `CREATURE_NONE` under the
  older ruleset exactly as they do in that wrapper. A second failure, or a
  non-positive initial count, clears the pending count. The retail call graph,
  field offsets, comparisons and signed divide establish the implementation;
  Dreamcast supplies only the method identity. The helper retains its
  address-derived working label until its own source row is reconstructed.
  All twelve blocks, all eight branches and every instruction outside the
  two success conversions agree. The residual is retail's repeated
  `test/setne/test` byte materialization versus this compile's direct integer
  test; separate bool/byte locals, explicit comparisons/casts, double
  negation, a tiny inline adapter and a register hint were byte-inert and
  removed under the HoMM2/Gruntz bounded-hypothesis rule.
  `decomp-attempt-1` was checked read-only and contains only the
  already-admitted prototype, so none of its material was used.

- **2026-08-09 — `combatManager::LearnSpellFromEagleEye` reconstructed
  from unscored to 68.1633%.** Retail walks a per-side 16-byte Dinkumware
  `set<SpellID>` at +0x5460, tests the side's hero for a spellbook, applies
  the Wisdom level gate, and grants every surviving spell. This deliberately
  differs from quick combat's first-success exit: the full combat path has
  already chance-filtered the set before this award pass. Restoring the STL
  container and iterator boundary reproduces the full CFG and call sequence.
  The residual is a whole-loop register rotation: retail spills the set
  address and retains `{side,this,spell}` in `{EBX,EDI,ESI}`, while this
  compile retains the set and spills `this`. Named-manager, repeated-container
  and volatile-pointer lifetime probes were inert or regressive and removed.
  Retail bytes alone select the retained form; no external implementation or
  `decomp-attempt-1` material was used.

- **2026-08-09 — `combatManager::CombatSystemOptions` reconstructed to
  99.8919%, with every instruction matching.** Retail proves a 0x50-byte
  stack object whose lifetime is constructor -> `DoModal` -> destructor,
  followed between the modal call and destruction by a +0x53b8 clear,
  `UpdateGrid(0, 1)`, and the six-argument `DrawFrame` refresh. Dreamcast
  CodeView supplies only the `TCombatOptionsWindow` identity and corroborates
  those three dialog calls; their retail rows at 0x46e3b0, 0x46f700 and
  0x46f780 are admitted on the independently matching x86 call graph and
  compatible sizes. Modelling the local as one opaque class object lets VC6
  regenerate the exact EH frame and cleanup. The remaining score sliver is
  stripped-target relocation naming/EH metadata, not code. This follows the
  HoMM2/Gruntz lifetime-boundary method; no external implementation was used,
  and `decomp-attempt-1` supplied nothing.

- **2026-08-09 — `combatManager::PlaceAllObstacles` reconstructed
  byte-exact (189 bytes).** The function samples obstacle ids 0..90 without
  replacement, accepting catalogue rows whose terrain or special-terrain
  mask matches the current battlefield, and stops when the picker is
  exhausted. Retail proves the two unsigned-short mask offsets, both
  negative-id exits, the inner redraw loop, and the call sequence. Dreamcast
  CodeView contributes only the `TObstacleInfo` name/20-byte extent and the
  existence of `TPickANumber`'s destructor. Making that destructor an
  inline-only `delete[] marks` cleanup reproduces retail's EH frame and final
  operator-delete call without adding a standalone x86 row. This applies the
  HoMM2/Gruntz guidance to restore the natural local-object lifetime and loop
  boundary instead of transcribing generated cleanup. A fresh delink pairs
  the formerly unscored row and raises the linked floor 592 -> 593 exact;
  existing picker scores are unchanged. No external implementation was
  used, and the earlier read-only `decomp-attempt-1` survey supplied nothing.

- **2026-08-09 — `type_AI_combat_data::do_aftermath` raised from 79.7256%
  to 97.3171% by restoring Eagle Eye's first-success exit and pointer
  lifetime.** Retail leaves the spell scan immediately after `AddSpell`, so
  the battle can teach at most one spell; adding the missing `break` is a
  semantic correction and collapses the reconstruction's three full-width
  induction values to retail's single signed-short DX walker. Naming the
  victorious hero inside the defeated-hero guard then keeps that pointer in
  EDI for the whole scan, matching every loop instruction. These changes
  apply the HoMM2/Gruntz guidance that control-flow exits and local lifetimes
  are optimizer inputs, while retail bytes alone prove the behavior and
  select the retained source. The residual is two harmless instruction-order
  differences around the volatile surrender byte. Register hints, alternate
  nesting and surrender-arm aliases were inert and removed. The earlier
  read-only `decomp-attempt-1` survey supplied no implementation or admitted
  metadata.

- **2026-08-09 — `type_AI_combat_data::cast_spell` reconstructed from
  unscored to 85.4199%.** The 1,427-byte retail body now selects among every
  available combat spell, enforces Cursed Ground and Recanter's Cloak, pays
  mana (including the Familiar's one-fifth channel), evaluates direct,
  opening-round, mass, enchantment and resurrection families, and applies
  the winning cast. Dreamcast CodeView contributes only the local names,
  reference signature and three helper boundaries. Retail independently
  proves all gates, values, loop directions and effects. Restoring the
  inline-only resurrection, mass-value and mass-cast helpers is the decisive
  HoMM2/Gruntz source-shape step; a scoped inline-depth boundary keeps the
  nested damage call out of line while allowing the surrounding helper to
  disappear. Making spell power's lifetime explicit raises the first
  transcription from 67.5762% to 85.4199%. The residual is predominantly an
  ESI/EDI coloring swap in the selection loop and the two expanded mass
  loops. A named defender reference regressed 85.4199% -> 85.3984% and was
  removed. Adding Recanter/Familiar to the widely included roster enums made
  `initialize_game_data` and `recruitUnit::Update` regress through VC6's
  type environment, so those enumerators were withdrawn and the same
  retail-proven ids remain TU-local typed constants. No external
  implementation was used; `decomp-attempt-1` supplied nothing.

- **2026-08-09 — `type_AI_combat_data::initialize_creatures` reconstructed
  from unscored to 91.0075%; `ai_combat` has no unscored retail body left.**
  The 1,646-byte function now computes the Tactics edge, square-root
  attack/defense force modifier, archery and speed bonuses, builds one
  simulated 72-byte stack record for each occupied army slot, applies the
  shooter/melee/wall model, accumulates total combat hit points, and sorts
  the vector by per-creature simulated hit value. Retail proves the full
  algorithm, including the two `sqrt` calls and all four trait bits.
  Dreamcast CodeView supplies only local names and the inline-only
  `get_catagory`/comparison helper boundaries. Restoring those boundaries,
  `vector::push_back` and `std::sort` lets VC6 regenerate the entire
  870-byte template-algorithm tail instead of transcribing it. The same
  retail body completes the DC-attested `hero::GetPrimarySkill` rule:
  attack/defense floor at zero, power/knowledge at one; that correction also
  raises `cast_spell` 85.4199% -> 85.4316%. The remaining delta is chiefly
  the function-wide EBX/EDI choice for `this` plus stripped-target relocation
  names. A named `this` alias and explicit attribute lifetime regressed
  91.0075% -> 83.29% and were removed. This applies the HoMM2/Gruntz rule
  that STL/helper boundaries and local lifetimes are codegen inputs. No
  external implementation was used; `decomp-attempt-1` supplied nothing.

- **2026-08-09 — `type_AI_combat_data::simulate_combat` is byte-exact.**
  The former direct transcription over-expanded nested accessors and damage
  routines, scoring 46.7778%. Restoring the original source boundaries—one
  inline-only spell-order helper plus ranged, speed-limited melee and general
  melee helpers—makes VC6 expand each outer helper while retaining retail's
  deeper calls. The four choice arms are now explicit in their true
  semantics: both choose melee, attacker only, defender only (reverse helper
  orientation), or neither. That structure first raises the 548-byte body to
  57.0139%. The remaining excess came from `inflict_damage` repeating
  `total_hit_points = 0` immediately before `kill()`, which owns the same
  store. Removing the redundant assignment keeps the 141-byte out-of-line
  function exact and makes every nested kill/survive arm byte-identical,
  closing `simulate_combat`. This directly applies the HoMM2/Gruntz rule that
  helper boundaries are part of optimized codegen, not cosmetic factoring.
  Retail bytes established the helper expansions and every branch
  orientation; `decomp-attempt-1` supplied nothing.

- **2026-08-09 — `type_AI_combat_data::cast_chain_lightning` is
  byte-exact.** The 325-byte body already had the correct three-bounce
  algorithm but remained at 79.6984% under a whole-function ESI/EDI swap.
  A named defender reference makes its lifetime explicit: VC6 assigns the
  defender to ESI and `this` to EDI exactly as retail does, raising the body
  to 95.3254% and matching the prologue/call setup. Assigning the inlined
  `take_damage` return back to the existing damage value then reproduces
  retail's kill/survive join and closes every remaining byte. Reordering the
  target/mask declarations regressed to 69.78%; hoisting the loop-counter
  declaration was byte-inert, so both probes were reverted. This is the
  HoMM2/Gruntz lifetime-first discipline applied to register ownership and
  an inlined return-value lifetime. Retail bytes alone selected the retained
  form; `decomp-attempt-1` supplied nothing.

- **2026-08-09 — `type_AI_combat_data::choose_melee` reconstructed from
  retail to 90.9329%.** The formerly unscored 1,021-byte body now rejects
  armies without a living melee stack, evaluates each possible switch from
  ranged to melee combat, copies and simulates both sides with spell-order
  priority, resolves the remaining general melee, and selects whether the
  current speed band is optimal. Its retail decorated name corrects the
  carcass to a const `bool` method taking a const reference. The five speed
  bands and helper names/signatures use Dreamcast CodeView as naming/type
  evidence; every helper statement was independently reconstructed from its
  repeated retail expansion. Restoring `cast_spells`, ranged-combat and
  speed-limited melee helpers is the decisive HoMM2/Gruntz-style source
  boundary: VC6 expands each helper but leaves its nested calls out of line,
  raising the first direct transcription from 15.4543% to 77.6098%. Writing
  the retail-expanded general-melee region locally under `inline_depth(0)`
  then reaches 90.9329% and the correct effective body length. The residual
  is local-object stack coloring and call identity: retail calls the already
  exact `type_monster_vector` wrapper, while this caller expands it into the
  underlying vector constructor/destructor. Declaration reversal and scoped
  inline-depth probes regressed or were inert; VC6 SP3 does not support the
  tested `__declspec(noinline)`, and all probes were removed. Concurrent
  commit `0dc5cfe` was preserved. `decomp-attempt-1` remained read-only and
  supplied nothing.

- **2026-08-09 — `AI_value_of_combat` reconstructed from retail to
  99.9592%.** The formerly unscored 792-byte overload now reproduces all
  23 branches and both returns: it simulates copies of both armies, rejects
  a defeated attacker at -1,000,000,000, prices experience and troop loss,
  adds the defender's attack value, and applies the two 5,000,000 objective
  bonuses. The decisive codegen fact was the already-admitted, DC-attested
  inline `hero::get_aggression`: a direct +0x47a float load stalled at
  93.5061%, while restoring the accessor recreated retail's overlapping
  float-return temporary and reached 99.9592%. Retail now proves +0x47a as
  a float and the combat-value consumer, though the field itself remains
  ordinal. The residual is not an algorithmic mismatch: stripped-image
  relocation recovery treats integer 5,000,000 as a DIR32 reference because
  it numerically equals VA 0x4c4b40, and splits the 0x6604d0 double array at
  its +4 interior read (the open DATA-size decision-point family). Source
  keeps honest integer/aggregate spellings. `decomp-attempt-1` was surveyed
  read-only as requested and supplied no code or layout to this change.
- **2026-08-09 — `type_cell_adjuster`'s cleanup and trigger resolver are
  admitted byte-exact as `QuickInfo` scaffolding.** Dreamcast CodeView fixes
  the helper at 12 bytes and names its three pointer members; retail then
  proves every member role through the calls and offsets in the 74-byte
  destructor and 303-byte `get_trigger_cell`. Both reconstructed bodies are
  instruction-identical to retail. The resolver's decisive source shapes are
  direct indexing of `game::heroes` after its explicit `-1` gate (avoiding a
  redundant `GetHero` null arm) and a shared equal-type return tail. This adds
  two exact `advmgr` rows, 588 -> 590 exact overall in the full comparison,
  and supplies the helper used by the still-unmatched 9,632-byte
  `advManager::QuickInfo`. Retail bytes and Dreamcast names/layouts supplied
  the evidence; no external implementation was used, and the generated
  baseline remains build-owned.

- **2026-08-09 — the final boat-frame source lifetime improves both hero
  render twins.** Inlining the final `animFrame % GetNumFrames(...)`
  expression into the draw call restores retail's right-to-left argument
  schedule: facing and the screen bitmap are captured before the frame
  division. `advManager::DrawHeroPart` rises 98.1571% -> 98.1667% and
  `DrawHeroPartShadow` rises 98.1745% -> 98.1840%, with all 16/18 branches
  and both exits still agreeing. A named flip and bitmap lifetime was
  measured separately and regressed both routines to roughly 95.1%, so it
  was reverted. The remaining shared block difference is VC6 spilling the
  frame-count divisor where retail keeps it in ECX. Retail bytes selected
  the retained source shape; no external implementation was used, and the
  generated baseline remains build-owned.

- **2026-08-09 — the primary AI-combat constructor is fully reconstructed
  at 99.9969%.** Its Dinkumware vector construction, hero/army ownership,
  wall penalty, mana and spellbook/orb gates, special-ground selector,
  pre-SoD Cursed Ground rule, enemy orb gate, and creature initialization
  call now reproduce every retail instruction, branch, and selector byte.
  The single-byte residue is semantically dead: VC6 sources the empty
  allocator byte from `[ebp+0xf]` rather than retail's `[ebp+0xb]`;
  spelling the allocator default explicitly does not move it and was
  reverted. The first attempt grew `EMagicTerrain` in `armygrp.h`, which
  exposed this tree's documented include-closure sensitivity by moving
  `initialize_game_data` from exact to 96.0880%. Reusing the already
  isolated, byte-proven 1/6/7/8/9 domain in `magicterrain.h` restores that
  exact row and keeps the ratchet clean. Retail bytes, the pinned VC6
  `<vector>` header, and existing admitted map-cell domains supplied the
  evidence; no external implementation was used.

- **2026-08-09 — AI-combat aftermath reconstructed; its experience helper
  is instruction-identical.** `game::ExperienceValueOfStack` is promoted
  from its Dreamcast-only row to the 88-byte retail slot at 0x4ca3b0:
  retail and the pinned build agree on every instruction and branch, with
  the 99.9706% display residue solely the target's synthetic name for the
  creature-traits relocation. `type_AI_combat_data::do_aftermath` is fully
  transcribed at 79.7256% with all 24 branches and its one return agreeing:
  mana restoration, surrender-aware experience, the retail-observed
  artifact-transfer direction, town claim, army adjustment, necromancy,
  the 70-spell Eagle Eye pass, and battle-temporary cleanup are all present.
  Its Dreamcast pointer parameter corrects the earlier invented reference
  signature without moving the exact `AI_quick_combat` caller. A
  memory-resident surrender byte raises the body from 75.4146%; the residual
  is an EDI/EBX whole-region allocation swap plus retail's signed-short spell
  induction versus this CL's fused pointer/index walk. Inline accessors, a
  bottom-tested loop, local order, and the separate inlined helper boundary
  were byte-inert or regressed and were reverted where they added no truth.
  Retail bytes and existing Dreamcast names/signatures supplied the evidence;
  no external implementation was used.

- **2026-08-09 — the AI-combat vector copy constructor is identified and
  byte-exact; an inherited class-copy claim is withdrawn.** The 135-byte
  retail body at 0x4276c0 copies only an allocator byte and the vector's
  pointer head, computes the element count from +0x04/+0x08, allocates
  `count * 72`, and copies each 72-byte `type_monster_data`; it never reads
  or writes the surrounding `type_AI_combat_data` fields. The pinned VC6
  `<vector>` copy constructor has exactly that source and compiles to all
  135 retail bytes when inlined into a source-private derived view. The
  model now places the real 16-byte Dinkumware vector at class offset zero
  (allocator byte, alignment, `_First/_Last/_End`) without moving any later
  field. A direct `operator[]` adapter preserves the prior /Ob2 nesting
  budget: without it, `cast_chain_lightning` and `get_enchantment_value`
  rise, but `do_general_melee` regresses; with it every existing score is
  unchanged and the new constructor alone is exact. The displaced
  Dreamcast `type_AI_combat_data` copy constructor returns to `DC_ONLY`;
  no retail slot is claimed for it. Retail bytes plus the pinned toolchain
  header establish the correction; no external implementation was used.

- **2026-08-09 — `type_AI_player::end_turn` rises 89.0940% -> 89.5263%
  by spelling retail's Marketplace scan exit.** Retail tests the town count
  once, then its inactive-Marketplace path increments the index, exits on
  `index >= count`, and takes an unconditional backedge. Replacing the
  source `for`/`continue` with that equivalent explicit scan fixes the first
  of two remaining branch-flow differences; 31/32 branch flows now agree.
  Writing the final warning test as unsigned `length() > 0` fixes the last
  jbe locally but lowers the total score to 88.7143% through allocation
  churn, so that probe was reverted. Retail control flow and objdiff selected
  the retained form; no external implementation was used.

- **2026-08-09 — `type_AI_player::make_gift` rises 71.8739% -> 79.8885%
  by restoring assignment in the multi-resource request arm.** Retail's
  single-resource arm calls the three-argument string append overload, but
  the multi-resource arm contains the inlined self-assignment, shared-buffer,
  and growth paths of `basic_string::assign`. Changing only that arm from
  `+=` to `=` recovers seven blocks and four branches. Direct three-argument
  assign, a named temporary, and depth-one/broader inline probes regressed or
  were byte-inert and were reverted. The remaining three missing branches
  belong to retail's inlined single-request temporary cleanup, which cannot
  yet be separated from the deliberately out-of-line append without
  disturbing the surrounding allocation. Retail bytes alone selected the
  semantic correction; no external implementation was used.
- **2026-08-09 — the default `town` constructor admitted byte-exact;
  `town` gains its 28th exact row.** Retail closes the former +0x32..+0xdf
  pad: ordinal state at +0x33/+0x34/+0x38/+0x3c, a 16-byte Dinkumware vector
  at +0xc4, and `std::bitset<70>` at +0xd4 immediately before the +0xe0
  garrison. Their automatic construction, the garrison constructor call,
  Village Hall seed, hero sentinels, and redundant seven-slot empty-army fill
  reproduce all 212 bytes including EH setup. Adding the standard container
  declarations to the canonical header preserved every dependent unit's
  exact rows. No external implementation was used.

- **2026-08-09 — `town::hire` admitted byte-exact from retail; `town`
  gains its 27th exact row.** The 201-byte body scans the player's two tavern
  offers for the hero id, charges `gHeroGoldCost`, refetches the canonical
  hero record, and places it on the town's packed cell before teaching town
  spells and retiring the consumed offer. Retail's uninitialised
  `type_point` read-modify-writes, two-iteration cursor loop, player and hero
  strides, and all three call relocations agree. The two unimplemented callees
  receive TU-scoped declarations only; all pre-existing exact rows remain
  intact. No external implementation was used.

- **2026-08-09 — `town::View` admitted byte-exact from retail; `town`
  gains its 26th exact row.** The 192-byte body sets the two view-state
  flags, hands `this` through the manager's byte-proven +0x38 town pointer
  to `executive::CallManager`, then restores the selected visiting hero as
  the adventure context when its signed owner matches the acting player.
  The dead `bAlreadyFaded` parameter, both duplicated cleanup returns, six
  data relocations, and two call relocations all agree. The manager prefix
  and the two large member calls are header-owned but visible only while
  compiling town.obj, preserving every existing exact row. No external
  implementation was used.

- **2026-08-09 — `town::destroy_extra_capitol` reconstructed from retail to
  96.4595%.** The 371-byte body checks the owner's other town ids, downgrades
  a duplicate Capitol to City Hall in `built`, clears the Capitol from
  `active`, and converts the corresponding adventure-map cell. The scan now
  reproduces retail's byte comparison, inlined `GetTown`, shared exit, and
  32-bit cursor-plus-negative-base induction exactly. A TU-scoped declaration
  pair preserves `initialize_game_data` and every other existing exact row;
  broad class declarations were measured and rejected after dropping that
  initializer to 96.09%. The remaining mismatch is a scratch-register cycle
  across the City Hall/active masks. No external implementation was used.

- **2026-08-09 — `town::get_build_cost` rises 77.4561% -> 81.4912% by
  restoring retail's cursor and down-counter loop.** The seven resource
  columns now advance through the cost row while a separate tail count is
  decremented; VC6 consequently reuses the dead `building` parameter slot
  for that count exactly as retail does. An indexed loop, an up-counter,
  extending the resource lifetime across `memset`, and moving `count` after
  the cost selection all scored lower and were reverted. The Dreamcast
  `EGameResource*` output signature was tested with a fresh label/delink pass
  and produced the same body while requiring a source-only enum cast, so it
  was also reverted to preserve the zero-cast floor. The remaining mismatch
  is a register cycle; control flow and the recovered countdown agree. No
  external implementation was used.

- **2026-08-09 — `town::update_shipyard` admitted byte-exact from retail;
  `town` gains its 25th exact row.** The 421-byte body first gates on the
  active Dock bit, packs the town's dock coordinates into a `type_point`, and
  indexes the retail map record directly. A trigger holding either a boat or
  a hero synthesises `DOCK_WITH_BOAT_ID`; when that occupant disappears, the
  pseudo-building is removed and `active` is rebuilt from the surviving
  `built` bits plus `town::included_buildings`. Testing `built`, not the
  evolving `active` mask, in that 44-entry rebuild is both the semantic fix
  and the final byte-matching lever: it raised the first complete
  reconstruction from 92.3000% to exact. The function identity and signature
  are corroborated by the Dreamcast CodeView row, but the implementation was
  reconstructed from retail disassembly and relocations; no external source
  implementation was used.

- **2026-08-09 — the adventure-rendering worktree was approved for
  integration into `master`; narrow views preserve both lanes' optimizer
  states.** The initial semantic merge placed `CAdvPopup`, `chatEdit`, and
  `targetIsCritical` into broad headers and compiled cleanly, but retail
  comparison exposed the include-set effect: `advmgr` fell from 22 exact
  functions to 4. `CAdvPopup` now lives in the split-dialog-only
  `advmgr_popup.h`; KeyboardMessageHandler and AI start-turn use narrow
  +0x58/+0x43 views instead of increasing the member populations of
  `TAdventureMapWindow` and `hero`. The combined report reaches 582 exact
  functions while retaining byte-exact `DrawRoad`, `start_turn`, and
  `KeyboardMessageHandler`. The baseline is regenerated only by the build.

- **2026-08-09 — `type_AI_player::make_gift` rises 64.6655% -> 71.8739%
  by restoring two retail string-append call edges.** The local-human gift
  and single-resource request call the three-argument `std::string::append`
  overload in retail, while the multi-resource request expands it. Directly
  expressing that overload and applying `inline_depth(0)` only to those two
  statements reproduces the asymmetry. Extending the override to both request
  branches, using the one-argument operator call, or naming the temporaries
  all regressed the surrounding allocation and were reverted. No external
  implementation was used; the retail disassembly and measured objdiff delta
  selected the retained form.

- **2026-08-09 — `type_AI_player::end_turn` rises 56.8835% -> 89.0940%
  by isolating retail's out-of-line string append and matching its warning
  scan.** Retail calls the three-argument `std::string::append` overload but
  inlines destruction of the formatted temporary. Naming that temporary and
  applying `inline_depth(0)` only to the append statement reproduces the
  split. Expressing the seven resource iterations as two advancing pointers
  plus a tail count then matches retail's loop direction and address
  progression. The remaining cursor/register permutation did not respond to
  declaration order, register hints, shared alliance indices, or extra
  scopes, so those byte-inert probes were reverted. This extends the same
  tightly scoped inline-budget method used for TSplitWindow and mousemgr;
  retail bytes remained the authority and no external implementation was
  used.

- **2026-08-09 — mousemgr gains two exact rows: `CheckUpdate` and the
  out-of-line `TCSLock` constructor.** Retail inlines CheckUpdate's outer
  lock constructor but calls the same constructor for the nested
  out-of-bounds lock, while inlining both destructors. A block-scoped VC6
  `inline_depth(0)` around only the inner declaration reproduces that
  call-site asymmetry: CheckUpdate rises 96.8521% -> exact and VC6 emits the
  byte-exact 25-byte `??0TCSLock` COMDAT. The existing reviewed carcass claim
  at 0x50d890 then binds to that public symbol on the fresh label/delink pass
  and also becomes exact; no annotation-contract extension is needed. This
  is the same local inline-budget technique that closed TSplitWindow's final
  vector insertion, applied to a constructor lifetime rather than an STL
  call. Retail bytes remained the verdict; `decomp-attempt-1` supplied
  nothing.

- **2026-08-09 — `executive::CallManager`'s missing exception-safety
  structure is byte-proven but not admitted while funclet attribution makes
  it a scoreboard regression.** The three isolated retail entries directly
  after the 0x4b0c70 body are catch funclets: 0x4b0dc2 removes the temporary
  manager and rethrows; 0x4b0dd7 performs the reduced exception-path resume
  of the saved manager and rethrows; 0x4b0e25 restores `currentManager` and
  rethrows. Three nested `try`/`catch (...)` scopes reproduce retail's EH
  prologue, state 0/1/2 transitions, 11 shared body blocks, and the funclet
  boundaries. VC6 places the generated catch blocks in CallManager's base
  COFF section, however, while the delinked retail object exposes them as
  three separately carved functions. Objdiff consequently appends five
  base-only blocks to CallManager and lowers its row from 68.3663% to 58.29%.
  The source probe was reverted and the ratchet was not lowered. This is a
  comparison-boundary problem to revisit when generated EH funclets can be
  paired explicitly, not evidence that the retail scopes are absent. The
  discovery came solely from retail bytes and the project's own carved
  entries; `decomp-attempt-1` supplied nothing.

- **2026-08-09 — `armyGroup::get_luck_description` rises 59.1587% ->
  60.3234%.** Retail's cursed-ground arm default-constructs a string, assigns
  the static text and routes it toward the return cleanup. A branch-local named
  result reproduces that lifetime and removes three generated blocks. Direct
  string initialization fell to 54.7365%, while a function-wide result with a
  single source return still emitted two machine returns and fell to 57.6257%;
  both probes were reverted. The remaining residual is primarily the hidden
  return-object/EH lowering shared by the two description routines.

- **2026-08-09 — `TSplitWindow::TSplitWindow` rises 77.0387% ->
  98.3954%.** Retail snapshots `creature` only across the elemental/background
  selector, then reloads the member for the sprite; that lifetime recovers its
  ESI/EDI allocation. The destination text entry's navigation argument is the
  byte-proven source id 4, not its own id 5. Finally, a depth-zero inline
  adapter evaluates the vector end iterator in the constructor but retains the
  final `vector::insert` call, avoiding the 16-block recursive expansion that
  `push_back` otherwise triggers in this compile. All 57 blocks and 25 branches
  agree. Direct insert, named-value, constructor-wide inline-depth and reserve
  adapter probes regressed or were inert and were reverted.

- **2026-08-09 — `soundManager::MemorySample` rises 98.7963% ->
  99.7840%.** The guarded explicit cursor repeated the empty-range test.
  Writing the slot search as a natural indexed `while` lets VC6 introduce
  retail's `[ebp-8]` cursor as a strength-reduction variable after the one
  range guard. All 30 blocks and 21 branches now agree; the only residual is
  a scratch-register rotation in the inlined stream-service tail. Named
  stream and split section-pointer lifetime probes were byte-inert and
  reverted.

- **2026-08-09 — inputmgr reaches 10/10 exact.** Retail clears all 64
  buffered messages between the base-manager constructor and the derived
  vptr store. That ordering proves nontrivial member construction, not an
  ordinary loop in `inputManager`'s body. A layout-neutral, message-derived
  buffer element with an inline zeroing constructor reproduces the member
  construction loop and vptr schedule exactly. Keeping the type scoped to
  inputmgr avoids the regressions caused by the rejected global `message`
  default-constructor probe; all eight dependent units and the 980-function
  ratchet remain green.

- **2026-08-09 — `armyGroup::GetArmyLuck` exact.** Retail's mode-5 branch
  lands on the Halfling check, proving that the minimum-one rule follows the
  Clover-only block instead of belonging to it. Within that block, a scoped
  creature snapshot is reused by the elemental exclusions and town lookup;
  VC6 consequently reloads the index into EDI and shrink-wraps EBX around
  the game-state test exactly as retail does. Together with the previously
  recovered explicit town-selector labels, all instructions, branches and
  in-text selector data now match. This is the control-flow-and-lifetime-first
  discipline taken from the HoMM2/Gruntz matching guidance; no external
  implementation material was used.

- **2026-08-09 — `armyGroup::GetArmyMorale` rises 80.5625% ->
  96.5625%.** Retail's two magic-terrain switches cover all nine town
  values and VC6 lowers each through a 9-byte selector table. Omitting the
  three no-op towns produced direct six-entry jump tables; empty cases were
  folded into default before lowering. Explicit Stronghold/Fortress/Conflux
  cases routed to named no-op exits preserve the full domain and reproduce
  both retail tables. All 38 block flows agree; only an ESI/EDI whole-body
  allocation mirror and retail's shrink-wrapped morale save remain.

- **2026-08-09 — `TSplitWindow::WindowHandler` rises 97.4066% ->
  99.9170%.** Retail keeps the accept arm adjacent to the shared end-dialog
  return tail and emits close/cancel out of line with a backward jump. A
  single tail after the inner switch forced the opposite join; repeating the
  same two message stores and return in both semantic arms lets VC6's
  cross-jumper recover retail's exact 34-block layout. Only one local
  EAX/EDX carrier swap at the destination slider call remains; named value
  and pointer probes were inert and reverted.

- **2026-08-09 — soundmgr reaches 20/21 exact; `MemorySample` rises
  92.0309% -> 98.7963%.** `ConvertVolume` becomes exact by recovering the
  retail CFG: each setting arm owns its range check, divide and minimum-one
  clamp, while the negative and 127 clamps are shared after the join. That
  semantic lifetime selects retail's EDX result carrier and its cross-jumped
  exits. A scoped `auto_inline(off)` is required because VC6 otherwise
  expands the newly smaller exact body into three callers that retail leaves
  as calls. In `MemorySample`, an explicit guarded `sampleHandles` cursor
  recovers retail's two stack homes; loading the chosen handle before the
  independent driver-state write preserves EDI through the playback tail;
  and two volume-call arms reproduce retail's push-eax/push-zero cross-jump.
  Retail bytes established each CFG; HoMM2's exact counterparts independently
  corroborate the shared-clamp and if/else-call source shapes. Remaining:
  one duplicate loop-entry compare and two scratch-register choices in an
  inlined helper. `decomp-attempt-1` remained read-only and supplied nothing.

- **2026-08-09 — `soundManager::soundManager` exact via construction-phase
  evidence.** The former 99.0312% body differed only in whether
  `MP3Playing = 0` appeared before or after the compiler-generated derived
  vptr store. Moving that byte initialization from the constructor body to
  the member-initializer list makes VC6 schedule it before the vptr, matching
  retail exactly while leaving both `rep stosd` regions and all other scalar
  stores unchanged. Body-statement reordering had been inert because it did
  not change the C++ construction phase. This applies the HoMM2/Gruntz rule
  to test source-semantic lifetime/phase distinctions before treating a
  one-instruction residual as register noise. No external implementation or
  `decomp-attempt-1` material was used.

- **2026-08-09 — `armyGroup::Merge` raised from 58.0726% to 79.5363%
  by restoring its two local working objects.** Dreamcast CodeView metadata
  names the 56-byte stack objects `ag1` and `ag2`; retail bytes independently
  confirm their four inlined constructor fill loops and remain the authority
  for layout and control flow. Expressing both as ordinary `armyGroup` locals
  and using a natural four-field indexed copy reproduces retail's fused
  pointer-difference copy shape. Reversing the two source-walker declarations
  gives the best observed VC6 allocation. Spelling the duplicate scan as an
  empty-body search followed by `if (b < 7)` restores retail's post-search
  continuation and supplies the final 1.2290-point gain. The residual is
  bounded to a four-byte frame difference, coalesced type/troop walkers,
  different stack coloring for the processed count, and one extra retail
  top-of-loop progress test. Volatile-pointer, alternate-index and
  explicit-goto probes regressed and were reverted. This follows the
  HoMM2/Gruntz matching discipline: use debug data
  as naming/type evidence, then accept or reject source shapes solely through
  fresh retail comparisons. `decomp-attempt-1` remained read-only and supplied
  no implementation or admitted metadata.

- **2026-08-09 — `armyGroup::get_morale_description` raised from
  60.1036% to 63.9572% by restoring the complete terrain selectors.** The
  two retail switch tails each contain a nine-byte selector
  `{0,0,0,1,1,1,2,2,2}` over all town types. Keeping the three neutral
  cases explicit while omitting a `default` prevents VC6 from folding them
  into an abbreviated six-value dispatch, matching the source-shape rule
  already retail-proven in `GetArmyMorale`. Named-result and explicit
  return-temporary probes regressed or were inert and were reverted. Retail
  bytes remain the authority; no external implementation was consulted.
  The generated baseline remains exclusively build-owned.

- **2026-08-09 — `type_AI_combat_data::simulate_combat` raised from
  46.0556% to 46.7778% by restoring its asymmetric speed calls.** Retail
  inlines the attacker's `get_fastest_speed` but calls the defender's copy.
  Naming the two values and suppressing inlining only on the defender
  expression restores that call edge; naming alone is byte-inert. Attempts
  to reproduce the attacker's shallow nested expansion or wrap the damage
  path regressed sharply and were reverted. The remaining excess branches
  are the known nested `get_total` and `kill` inline-budget divergence.
  Retail bytes alone selected the retained shape, and the generated baseline
  remains exclusively build-owned.

- **2026-08-09 — the split-army path completed and its message producer is
  exact; both retail combat-stat descriptions reconstructed.**
  `SplitSliderCallback` and `armyGroup::SplitArmy` now match exactly, raising
  the engine scoreboard to 557/980 exact functions. `SplitArmy` required the
  natural member-by-member `message` initialization order evidenced by retail
  stores—qualifier through window first, followed by id/codeX/codeY; aggregate
  initialization is semantically equivalent but schedules different VC6
  stores. The complete `TSplitWindow` constructor is present at 77.0387%, and
  its handler remains at the best-measured 97.4066% with 29/34 blocks exact.
  Reconstruction-only dialog and description declarations stay behind the
  armygrp source boundary because exposing them globally perturbs VC6 `/Ob2`;
  that boundary restores `initialize_game_data` to exact. Retail `ret 24h` and
  `ret 20h`, trait indexing and string uses correct the stale Dreamcast
  description prototypes. `get_morale_description` (60.1036%) now covers
  cursed/no-morale exits, hero, terrain, alignment, undead, creature, town,
  artifact and residual modifiers; `get_luck_description` (59.1587%) covers
  cursed ground, Hourglass, hero, Clover Field, enemy Devils, Fountain,
  Halfling and residual modifiers. Their remaining deltas are dominated by
  Dinkumware string-temporary/EH layout. The HoMM2/Gruntz matching guidance
  was applied directly: recover natural control-flow and library source forms,
  keep reconstruction types local, and validate each source-shape experiment
  against fresh retail comparisons. `decomp-attempt-1` was surveyed read-only;
  its bodies are mostly stubs and supplied no implementation or admitted
  metadata.

- **2026-08-09 — the TSplitWindow destructor pair reconstructed exactly;
  `SplitSliderCallback` raised from unimplemented to 96.8246%.** Retail's
  `TSplitWindow` is a 0x80-byte `CAdvPopup` derivative: the callback proves
  the count fields at +0x6c/+0x70/+0x74 and signed transfer minimum at
  +0x78, while the constructor proves the creature at +0x7c. The
  Dreamcast `CAdvPopup` field list, adjusted by retail's independently
  proven 8-byte `heroWindow` vector delta, agrees with the retail ctor and
  dtor at +0x50..+0x5c and fixes the base at 0x60 bytes. With that layout,
  a normal virtual destructor containing the widget-pointer walk emits the
  retail 107-byte body exactly, and the established `VA_COMPGEN` scalar
  deleting-destructor form emits its 33-byte wrapper exactly. The slider
  callback keeps the CodeView member helper inline-only, as retail does;
  its branchless 195-byte body and all calls now agree, with only VC6's
  equivalent temporary-register rotation remaining. Making
  `CHeroWindowEx`'s byte-proven concrete slots non-pure caused no score
  regression in the 25 dependent units rebuilt. A fresh delink admitted
  only these source-derived names. `decomp-attempt-1` remained read-only:
  its implementations are predominantly stubs, so nothing was copied or
  admitted; its inventories remain candidate metadata pending P4.2 and
  explicit approval.

- **2026-08-09 — `get_spell_work_chance` raised from 53.1589% to
  88.5071% by restoring retail dataflow and switch-body order.** Both
  switches keep their byte-proven selector maps, but their action bodies
  are written in retail layout order. A named creature-traits cursor
  recovers the reused record pointer, while the later bit-10 gate must read
  `akSpellTraits[spell].field_c` directly: this keeps retail's
  `spell * sizeof(SSpellTraits)` offset live in ESI. The nested gate then
  tests creature attributes bit `0x400`; the former `spellRec->flags_10`
  test was semantically wrong. Animate Dead likewise checks undead status
  only—retail has no damage-bound test there. The two Orb of Vulnerability
  checks guard the resistance/immunity block, after which retail shares the
  spell-record `field_0` check and chance floor. Explicit shared tails now
  model the pendant checks and the 0.8/0.6 resistance calculation. The
  remaining mismatch is confined to VC6 layout choices: immediate pushes
  versus an enregistered shared artifact id, four over-merged zero exits,
  the resistance-tail fall-through direction, and prologue instruction
  scheduling. The Gruntz/HoMM2 discipline of matching semantic control-flow
  regions before local instruction tuning directly drove this recovery.
  The same control-flow-first pass raised `GetArmyLuck` from 84.8947% to
  90.2526%: a dense town 0–8 switch with explicit no-bonus and `+2` labels
  reproduces retail's two-destination byte selector, and repeated
  `armies[index]` accesses remove the incorrect creature cache. All 22 code
  blocks and both dispatch-table offsets now agree; only the index/register
  allocation and equivalent table target identities remain.

- **2026-08-09 — the shared armygrp clamp recovered: `GetLuck` exact,
  `GetMorale` 98.5654%, `GetArmyMorale` 80.5625%, and `GetArmyLuck`
  84.8947%.** Retail's common tail is not a nested min/max or a direct
  ternary. It is one by-value, reference-returning three-operand selector;
  the byte-proven argument order is `(low, value, high)`. That order alone
  produces retail's low/high/value stack homes without forcing the running
  rating into memory early. It closes `GetLuck` (91.1735% to exact), raises
  `GetMorale` from 88.4346% to 98.5654%, `GetArmyMorale` from 72.8920% to
  80.5625%, and `GetArmyLuck` from 75.6316% to 84.8947%. In `GetMorale` it
  also shifts `/Ob2`'s budget so all four bitset operator[] assignments now
  match; all 57 block flows agree, leaving only EBX shrink-wrapping and an
  equivalent atexit-thunk relocation representation. An explicit nine-town
  value-switch probe in `GetArmyMorale` fell to 76.5% and was reverted.
  `Merge` also moved 54.4022% to 58.0726% by expressing copy-in as one
  byte-offset loop and walking the source troop array directly, which
  prevents four incorrect rep-movsd runs and recovers another piece of
  retail's source-pointer induction;
  its remaining frame/pointer-induction reconstruction stays active.
  `decomp-attempt-1` remained read-only and supplied nothing to these edits.

- **2026-08-09 — `armyGroup::merge_armies` exact; luck routines raised by
  restoring retail expression and switch structure; `modify_spell_damage`
  exact.** `merge_armies` rose
  from 84.7652% to exact after three semantic corrections: stop the weakest
  scan at the first empty slot, express both fit searches as direct jumps to
  their success continuations, and use the two CodeView-attested
  `std::swap` calls with source as the first argument. The swap spelling is
  load-bearing: hand-written exchanges preserve values but change the
  entire frame/register allocation; reversing the references changes the
  final load/store order. `GetLuck` rose from 85.4592% to 91.1735% by
  combining the two Hourglass artifact gates into one short-circuit
  expression and shared return. `GetArmyLuck` rose from 30.3579% to
  75.6316% by spelling its nine town cases as a value-producing switch;
  retail uses a compressed two-destination selector table while this VC6
  spelling retains a full jump table, so that codegen residual remains
  bounded. This directly applies the HoMM2/Gruntz guidance: first match
  branch topology, fallthrough and standard-library source forms, then
  assess register-only residuals. `modify_spell_damage` rose from 55.6350%
  to exact by putting the elemental arms before the golems in retail order,
  mutating the damage parameter and breaking to the shared return. The
  jump table also corrected a semantic error: Earth/Magma, not Water/Ice,
  take double Meteor Shower damage; Water/Ice take double damage from the
  three fire spells. `decomp-attempt-1` remained read-only and supplied no
  implementation or admitted metadata.

- **2026-08-09 — `armyGroup(TCreatureType,int)` exact and
  `hero::get_primary_skill_total` raised to 99.5833% by restoring retail
  loop structure.** The army constructor's prior outer guard was
  semantically wrong: once entered, it populated all seven slots even if
  the remaining amount became zero. Retail tests the short index first and
  the remaining amount second; preserving that short-circuit order lets
  VC6 sink the index bound to the back-edge, retain the amount test at the
  loop head, and reproduce all 90 bytes exactly. For the hero total, an
  explicit index plus a separate four-step countdown reproduces retail's
  ESI/EDI induction pair and every instruction; only the equivalent
  scale-1 SIB choice for `this + index` remains different. These fixes
  apply the HoMM2/Gruntz structure-first rule directly: recover the retail
  branch and induction family before attempting register-level spellings.
  `decomp-attempt-1` was kept read-only and contributed no source to either
  reconstruction.

- **2026-08-09 — six `type_AI_player` turn-economy routines reconstructed;
  two are exact; demand reaches 86.3767%, reserve 96.4566%, gifting 64.6655%,
  and end-turn is structurally restored.** `start_turn` (329 bytes) is
  exact after restoring the retail-proven hero critical-target byte, the
  production/Grail calls, and the two 8-byte threat-checker strategy
  objects; spelling the base constructor assignment in its body reproduces
  retail's vptr-before-member store. `get_total_value` (343 bytes) is exact
  with the player AI production slice at +0x108 and retail's trade-supply,
  trade-feasibility, weighted-cost, and ratio flow. `calculate_reserve`
  reconstructs the full 640-byte algorithm and exact CFG: collect populated
  dwellings as 12-byte value records, narrow the AI-value product through
  short, sort ascending, price the strongest two, and keep each resource's
  maximum across towns. Its residual is confined to one VC6 allocation
  cycle in the dwelling scan; the rest of the body agrees. `calculate_demand`
  restores stock plus two production turns, maximum legal-building costs,
  the full 145-record creature valuation/sort and strongest-three pricing,
  Marketplace efficiency, seven resource-value doubles, and retail's
  six-resource running total divided by five. Its residual is concentrated
  in VC6's sort final-insertion allocation. `make_gift` restores the surplus
  caps and thresholds, AI-recipient shortage transfer, human gift/request
  resource vectors, the exact 0x432/0x433 wire payloads and network gate,
  all three localized messages, and the turn-duration-dependent 15000 ms
  request timeout. Directly indexing the recipient, rather than retaining a
  convenience pointer, recovered retail's 0x7c frame and `ebx` player-id
  lifetime; the asymmetric gold compare/indexed store was likewise
  preserved from the bytes. Its remaining delta is chiefly VC6 choosing to
  inline temporary-string destruction where retail calls `_Tidy`, plus two
  transfer-loop register permutations. This applies the HoMM2/Gruntz
  structure-first rule and records failed type/lifetime/codegen probes
  instead of guessing through the plateau. `end_turn` now implements
  production/reserve accounting, both town strategies, the prohibited
  purchase loop, hero hiring, demand refresh, the first-Marketplace gift
  passes (AI allies before human allies), and negative-resource warning.
  Its current 56.8835% retains a TU optimizer dependency: even with both
  neighbouring routines restored, VC6 expands the 230-byte `string::append`
  body that retail calls out of line. `decomp-attempt-1` was checked
  read-only and contains only stubs/prototypes for these routines; no
  implementation, layout, or name was imported from it.

- **2026-08-09 — `button::Main` improved from 84.6566% to 88.1009%;
  remaining string delta is an inline-boundary choice.** Retail emits the
  palette-success arm first, then falls through to the sprite-reload path;
  expressing that case positively recovered the complete block order.
  Restoring the natural header spelling `Text = new_text` also aligned the
  strlen and raw `movsd`/`movsb` copy tail. The remaining text region is
  source-equivalent VC6 `/Ob2` state: retail calls the 0x404a90
  `std::string::_Grow(size, true)` COMDAT here, while this compile expands
  it; the same header body makes the inverse boundary choice in the
  `textButton` constructor. Inline-depth 1 through 4 and scoped
  `auto_inline` probes were inert and reverted. The rest is the already
  bounded ESI/EDI whole-body allocation mirror. HoMM2/Gruntz's
  structure-first method led to the success-first branch correction;
  `decomp-attempt-1` was checked read-only and contains only Button
  prototypes/stubs, so nothing was imported from it.

- **2026-08-09 — `mouseManager::Update` retail reconstruction reached
  94.1675%; its inlined helper boundaries are restored.** The 0x770-byte
  body now has retail's complete 72-block CFG: re-entrancy and busy gates,
  cursor sampling, interleaved hotspot lookup, clipping, overlap union,
  DirectDraw save/draw/restore paths, and cleanup. Applying the
  homm2/Gruntz structure-first method identified the decisive source shape:
  the retail stack retains argument homes and private `RECT`s from the
  known `SaveAndDraw` and `RestoreUnderlying` members even though their
  standalone retail copies were eliminated. Restoring those helpers raised
  the body from 83.1422% to 92.1801%; preserving separate screen/client
  rectangles in the non-overlap path and the shared helper rectangle
  initialization reached 94.1675%, with all 72 branch flows and 68 block
  sizes exact. The bounded remainder is VC6 parameter-homing/zero-CSE state,
  an eight-byte frame-allocation delta, and the Y-coordinate relocation form
  (`POINT` base + 4 locally versus a retail relocation rooted at the +4
  address). Ordinary inline, volatile parameters, coordinate temporaries,
  declaration-order changes, and alternate rectangle spellings did not
  improve the best. `decomp-attempt-1` was checked read-only: these mouse
  bodies are stubs there, so it supplied no admitted implementation; its
  potentially useful content remains limited to inventory metadata that
  still requires separate supervised approval.

- **2026-08-09 — `mouseManager::LoadFrame` reconstructed from retail at
  99.3104%; attempt-1 remained inventory-only.** The 0x16b-byte body proves
  a 0x64-byte `DDBLTFX`, the older 0x6c-byte `DDSURFACEDESC` passed through
  the DirectDraw4 ABI, a referenced `Bitmap16Bit`, and the eight-argument
  `CSprite::DrawPointer` call. Using `DDSURFACEDESC2` first produced a
  structurally useful 96.8793%; correcting the stack type reached 97.00%,
  and preserving the sprite pointer in a named local made the entire
  draw/unlock block exact and reached 99.3104%. All three CFG blocks agree;
  B1/B2 are instruction-exact. B0 is bounded to the inlined color-mask
  accumulator's register assignment plus the normalized EH-handler addend;
  an explicit accumulator-local spelling fell to 88.72%. The corresponding
  `decomp-attempt-1` body is a stub, so no source or data identity was
  imported from it.

- **2026-08-09 — `misc` preference lifecycle reconstructed; defaults exact,
  two retail/codegen residuals bounded.** Retail calls and registry value
  names pin `CheckConfigFile` at 0x50b260, `SetGameDefaults` at 0x50b4d0,
  and `ReadPrefsFromRegistry` at 0x50b7b0, as well as the 212-byte prefs
  layout, path buffers, and three missing registry-name pointer objects.
  Applying the homm2/Gruntz structural-first method exposed the original
  inline-helper boundaries: movement speeds belong to system defaults,
  combat speed precedes the auto-combat flags, and blackout is top-level.
  That made all three `SetGameDefaults` blocks instruction-exact and raised
  the engine 544 -> 545 exact. `CheckConfigFile` is 98.5030% with 23/24
  blocks exact; retail alone retains one redundant second AND that VC6
  removes from every equivalent C spelling tried. `ReadPrefsFromRegistry`
  is 95.9319%: all live code surrounding its CD-path tail agrees, while the
  shipped image contains a four-byte-copy hot patch, jump, and 17 NOPs over
  the unreachable remainder of an older inline `strcpy`; canonical strcpy
  was retained instead of encoding patched bytes as source. The two
  6-byte desktop-size getters at 0x6014c0/0x6014d0 are direct retail loads.
  `decomp-attempt-1` was surveyed read-only and supplied no admitted source.

- **2026-08-09 — `executive::MainLoop` exact after correcting command
  semantics.** The earlier reconstruction incorrectly merged executive
  commands 1 and 4 into one `dialogReturn = msg.extra; done = 1` arm.
  Retail's unmasked `dec`/`dec`/`sub 2` dispatch proves three distinct
  cases: TERMINATE_LOOP sets only `done`, REMOVE_MANAGER removes the current
  manager, and RETURN_RESULT copies `msg.extra` before setting `done`.
  Splitting those arms also restored retail's stack frame, dispatch homing,
  register allocation, and branch polarity with no codegen lever. Result:
  88.1290% -> 100.0000%, engine-wide 543 -> 544 exact.

- **2026-08-09 — both Windows input handlers exact; canonical field chain
  admitted.** `KeyboardMessageHandler`'s 0x1ab-byte body now models the
  input ring entry, key-state qualifiers, `% 64` head/tail advance,
  `extendFlag`, and F1/F4 command tail. The previously unresolved field chain
  is represented by the real types: Dreamcast places
  `TAdventureMapWindow::chatEdit` at +0x50, retail's independently proven
  `heroWindow` base is eight bytes wider, and retail directly confirms the
  resulting +0x58 pointer followed by the byte-proven
  `textEntryWidget::bHasFocus` at +0x6d. A bounded VC6 sweep applied the
  homm2/Gruntz structural-first doctrine: the six clears are one chained
  assignment, and `HIWORD(lParam) & 0xff` preserves retail's full-width
  load/shift/mask. The same chained clear then moved the otherwise
  byte-identical `MouseMessageHandler` from 99.9645% to 100.0000%.
  Result: keyboard 94.9323% initial reconstruction -> 100.0000%,
  engine-wide 541 -> 543 exact. `decomp-attempt-1` was surveyed read-only;
  it corroborates the address, size, and +0x58 layout but contains only a
  stub body, so no source was admitted from it.

- **2026-08-09 — text-pad normalization corrected for linked-target
  alignment.** `inputManager::AsciiConvert` was byte/reloc-identical over
  its claimed 0x1c6 bytes but remained at 98.80% because the delinked target
  necessarily kept two NOPs before the next 4-byte-aligned function while
  the base COMDAT normalizer removed all ten of its trailing NOPs. The
  normalization driver now pairs base and target: it restores only a target
  NOP suffix of at most 15 bytes, only when removing that suffix makes the
  two logical function lengths equal, and records both raw objects in the
  provenance stamp. Full-engine sweep: 540 -> 541 exact, no function score
  decreased; `AsciiConvert` 98.8024% -> 100.0000%. This also disproves the
  earlier theory that its base-only COFF labels capped the match.

- **2026-08-09 — `advManager::DrawRiver` admitted byte-exact;
  `advmgr` 22 → 23 exact.** The 471-byte body reproduces its map bounds,
  packed-point fallback cell, river-presence gate, viewport clipping,
  river set/frame selection, word-width flip flags at bits 2 and 3, and
  the raw `CSprite::DrawTile` target. Unlike the adjacent road pass, retail
  proves no half-tile vertical origin or bottom-row crop. All instructions
  compare exactly. Retail bytes and relocations are the correctness
  evidence; Dreamcast CodeView supplied only surviving names and signatures.
  No external body was consulted or ported, and the generated baseline
  remains exclusively build-owned.

- **2026-08-09 — `advManager::DrawRoad` admitted byte-exact;
  `advmgr` 21 → 22 exact.** The 492-byte body reproduces its map bounds,
  packed-point fallback cell, road-presence gate, half-tile vertical origin,
  viewport clipping, bottom-row crop, road set/frame selection, word-width
  flip flags, and raw `CSprite::DrawTile` target. All 13 symbolic branches,
  the single return, and every instruction compare exactly. Retail bytes and
  relocations are the correctness evidence; Dreamcast CodeView supplied only
  surviving names and signatures. No external body was consulted or ported,
  and the generated baseline remains exclusively build-owned.

- **2026-08-09 — the 584-byte `advManager::DrawShroud` body advanced
  from 81.41% to 91.25%.** Retail instructions and relocations prove the
  clipped draw rectangle, visibility gate, full-map star tiles, cloud lookup, horizontal
  flip encoding, alternating edge frames, and the raw shroud-tile sprite
  target. Retail also proves that a zero cloud lookup draws the star fallback
  rather than skipping the cell. Routing both full-draw mode and zero lookup
  to one star tail after the cloud path removes the two polarity/layout
  defects: all 21 symbolic branches and both returns now agree. VC6 register
  and local scheduling prevent an exact claim. Dreamcast CodeView supplied
  only the surviving names and signatures; no external body was consulted or ported.
  The generated baseline remains exclusively build-owned.

- **2026-08-09 — the 715-byte `advManager::DrawGround` pass refined
  from 86.63% to 91.16%.** Retail proves the packed-point fallback cell,
  clipped tile rectangle, ten-entry ground tileset, ground frame and two flip
  bits, four corners, four edge-pattern families, and the repeating border
  fallback.
  Passing the packed point by value through the narrow inline cell lookup
  reproduces retail's 12-byte frame, argument-slot reuse, and point-temporary
  lifetime; the reconstruction still has the same 30 symbolic branches and
  both return paths agree with retail. The remaining invalid-cell call shape
  and local/register scheduling prevent an exact claim. Dreamcast CodeView
  supplied only the surviving tileset, field and method names; no external
  body was consulted or ported. The generated baseline remains exclusively
  build-owned.

- **2026-08-09 — both `advManager::CompleteDraw` overloads admitted
  byte-exact; `advmgr` 19 → 21 exact.** The 1,149-byte orchestrator
  reproduces its entry/message gates, forced full-map origin, nine 20-by-18
  tile passes, view-world and route gating, cursor state, shroud and gem
  passes, 100-frame FPS timing ring, chat refresh, sound polling, and bottom
  view update. Its 40 symbolic branches and every instruction compare
  exactly. The 67-byte packed-origin forwarding overload is exact as well.
  Retail bytes and relocations prove all control flow, constants, storage
  widths and call targets; Dreamcast CodeView supplied only surviving member,
  local and method names. No external body was consulted or ported, and the
  generated baseline remains exclusively build-owned.

- **2026-08-09 — the 1,154-byte `advManager::DrawUnderlay` body advanced
  from 78.31% to 80.76%.** Retail instructions and relocations prove the
  clipped cell lookup, underlay-only object filter, object/type/sprite pool traversal,
  eight-case flagged-object selector, checked trigger-cell lookup, animation
  frame, player output color, and normal/flagged sprite paths. The
  reconstruction's 22 symbolic branch targets agree with retail. A later
  structural pass corrected the byte-packed cell-object view: its 16-byte
  vector begins at `NewmapCell+0x0e`, placing `_M_start`/`_M_finish` at
  retail's `+0x12/+0x16` and ending exactly at the proven `type` field at
  `+0x1e`. Retaining both packed-point assignment temporaries and using the
  already proven inline cell helper then restored retail's shared cell-call
  boundary; narrowing the full-map view's lifetime to one object iteration
  removed an extra stack slot. Recomputing the object address at its actual
  type, trigger and animation uses removes the same non-retail long-lived
  pointer found in the two larger object passes. All 22 symbolic branches
  and the return still agree. The remaining difference is instruction,
  register and local-slot scheduling, so no exact claim is made. Dreamcast
  CodeView
  supplied surviving names and signatures only; no external body was
  consulted or ported. The generated baseline remains exclusively build-owned.

- **2026-08-09 — the 2,446-byte `advManager::DrawAdvObj` body refined
  from 80.22% to 85.91%.** Retail proves the seven-layer object-cell
  traversal, normal 48-bit draw mask, 48-entry view-world terrain selector,
  82-entry flagged object selector, trigger-cell ownership lookup, player
  output color,
  transient-object override, animated sprite path, interleaved hero/boat
  parts, and cursor rows. Retail's unsigned seven-layer back edge corrects
  the last signed branch. Narrowing the full-map view to each object
  iteration and recomputing the object address at its three actual uses
  removes a non-retail long-lived pointer; the non-hero layers now continue
  directly to the outer back edge instead of manufacturing an empty part
  range. All 72 symbolic branches and seven returns consequently agree with
  retail. Remaining differences are VC6 register and local-slot scheduling,
  so no exact claim is made. Dreamcast CodeView supplied surviving names and
  signatures only; no external body was consulted or ported. The generated
  baseline remains exclusively build-owned.

- **2026-08-09 — the 1,508-byte `advManager::DrawAdvObjShadow` body
  refined from 75.36% to 83.21%.** Retail instructions and relocations prove
  the clipped map-cell object-vector walk, object/type/sprite pool offsets,
  48-bit shadow mask, animation selection, transient-object override, cursor
  shadows, and the final hero/boat shadow overlays. The view-world switch is
  reconstructed from its retail 48-byte selector table: terrain ids
  114..161 draw except `TERRAIN_HOLE` and the river/road group. The packed
  point now has retail's assignment lifetime, while a validity-first cached
  map lookup reproduces the load before the fallback split. Narrowing the
  full-map view to one object iteration and recomputing object addresses
  removes the same non-retail long-lived pointer found in `DrawAdvObj`;
  spelling the final overlay loop as `part <= 5` fixes its back edge. All 42
  symbolic branches and the return agree with retail. Remaining differences
  are VC6 local and register scheduling, so no exact claim is made. Dreamcast
  CodeView supplied only surviving local/type names and signatures; no external body was
  consulted or ported. The generated baseline remains exclusively build-owned.

- **2026-08-09 — `advManager::DrawBoatPartShadow` admitted byte-exact;
  `advmgr` 18 → 19 exact.** The 591-byte shadow twin preserves every
  matched boat lookup, packed-cell, froth, frame, crop and flip operation
  from `DrawBoatPart` while relocating both final calls to the distinct raw
  `CSprite::DrawHeroShadow` body. Retail blocks and relocations are the
  correctness evidence; CodeView supplied only the signature/name, no
  external body was consulted, and the generated baseline is build-owned.

- **2026-08-09 — `advManager::DrawBoatPart` admitted byte-exact;
  `advmgr` 17 → 18 exact.** The 591-byte retail body proves the direct
  40-byte boat-pool index, its packed position, the asymmetric inlined
  valid-cell lookup, optional froth layer, boat layer, signed-facing flip,
  and Bitmap-to-raw-map sprite wrapper. The reached boat position words at
  +0/+2/+4 are now explicit. CodeView supplied the signature and names only;
  no external body was consulted or ported. The generated baseline remains
  owned by `homm3 build` and was not edited manually.

- **2026-08-09 — the 1,172-byte `advManager::DrawHeroPartShadow`
  body opened to 98.1745%.** Retail proves that the shadow pass mirrors
  the five hero/boat layers already reconstructed, calls the distinct raw
  `CSprite::DrawHeroShadow` target, and adds an owner-range guard before the
  boat path. The matching header-inline Bitmap wrapper and every semantic
  branch are reconstructed; thirty-five of thirty-six control-flow blocks
  compare exactly. The remaining block is the same final boat-frame VC6
  scheduling residual as `DrawHeroPart`, so no byte-exact claim is made.
  CodeView supplied signatures and names only; no external body was
  consulted or ported, and the generated baseline is build-owned.

- **2026-08-09 — the 1,156-byte `advManager::DrawHeroPart` body opened
  to 98.1571%.** Retail instructions and relocations prove the hero/boat
  split, packed-point validation and map-cell lookup, the optional boat
  froth layer, the two boat layers, the owner flag/cursor layers, and all
  reached layout offsets. The five draws use the retail CSprite header
  wrapper shape: its Bitmap argument expands inline to the raw-map overload.
  CodeView contributed signatures and names only; no external body was
  consulted or ported. Thirty-three of the function's thirty-four control-
  flow blocks now compare exactly. The sole residual is VC6 scheduling in
  the final boat-sprite draw, so this entry deliberately does not claim a
  byte-exact admission. The generated baseline remains owned by `homm3
  build` and was not edited manually.

- **2026-08-09 — `GetFlaggedObjectOwner` admitted exact; `advmgr`
  16 → 17 exact.** The 270-byte retail body proves and reproduces hero
  obscured-object unwrapping plus the generator, garrison, mine/lighthouse,
  town and shipyard ownership paths, including its six-target switch and
  82-byte selector tables. The reached layout slices are now explicit:
  hero obscured type/index at +0x0c/+0x14, and a 0x40-byte garrison record
  with signed owner at +0. All pre-existing exact rows survive those shared
  header changes. No external body was consulted or ported. The generated
  score moves 5.89% → 5.91%; the baseline migration came only from
  `homm3 build`.

- **2026-08-09 — adventure flag-render scan admitted exact;
  `advmgr` 14 → 16 exact.** `advManager::ScanForHeroOrBoat` (359 B) matches
  its six-slot result walk, bounds checks, packed-point construction,
  inlined `GetCell(type_point)`, trigger/type/id predicates, and writes
  byte-for-byte. The retail xref set corroborates the surviving public name
  `gbInViewWorld`; CodeView contributes only the function signature and the
  16-byte `TDrawParts` layout. `hasFlag` (122 B) is also exact, including both
  compiler tables; its eight named cases were decoded from retail's selector
  bytes over object ids 17..98. No external body was consulted or ported.
  The supported build-generated status migration raises executable coverage
  5.87% → 5.89%; `config/match_baseline.tsv` was not edited manually.

- **2026-08-09 — `advManager::GetSoundId` admitted at 89.50% from retail
  control flow and compiler tables.** The 1,508-byte function reconstructs
  the ground-overlay fast path, trigger-object dispatch, creature-bank,
  mine, garrison, creature-generator and terrain-special cases. Retail's
  two selector arrays and four jump tables prove every admitted ordinal
  mapping; the surviving public name at 0x63d570 proves the generator-one
  lookup, while a narrow three-byte mine view avoids widening `game.h`.
  Sound and creature values deliberately retain ordinal spellings until
  semantic names have separately admissible evidence. The remaining delta
  is VC6 block placement and shared-return folding: the object selector is
  byte-identical, but several equivalent return blocks and compiler tables
  are emitted in a different order. No external body was consulted or
  ported. Generated status records the decorated function at 89.50%;
  `config/match_baseline.tsv` was not edited manually.

- **2026-08-09 — `advManager::GetCloudLookup` admitted byte-exact from
  retail evidence; `advmgr` 13 → 14 exact.** The 613-byte body reproduces
  the boundary-seeded eight-neighbour visibility mask, all sixteen guarded
  `GetMapExtra` paths, and the final `giCloudType` byte lookup exactly. Retail
  relocations prove the table at 0x65f694 and the one-byte visibility mask at
  0x69ccbc; the latter keeps a provisional role-based name because no public
  retail spelling survives. No external function body was consulted or
  ported. The generated status update raises the full executable score from
  5.84% to 5.87% and migrates the function's flat label to its decorated VC6
  symbol without any manual baseline edit.

- **2026-08-09 — `advmgr` 11 → 13 exact; the first 500-byte body
  opened to 82.01%; two required member declarations restore
  `initialize_game_data` to 100%.** `MapExtraPosAndAdjacentsSet`
  (151 B) and `ForceNewHover` (73 B) are byte-exact from retail control
  flow and relocations. `FindAdjacentMonster` (507 B) is structurally
  reconstructed at 82.0057%: retail proves the reference-returning
  max/min bounds, NewfullMap indexing, trigger/object-trait test,
  monster/water/excluded-point predicates, and packed result writes;
  the remaining delta is local/register allocation, not yet claimed
  exact. Its direct load proves 0x660428 is a pointer to 16-byte object
  traits (not an inline array). Declaring the two actually-called
  `NewmapCell` members (`cell_is_trigger`, `get_map_object`) changes the
  shared include population and independently returns
  `initialize_game_data` 94.0741 → 100.0000; both declarations are
  required by retail REL32 calls, so the gain is retained rather than
  tuned for score. No unadmitted `kb.cpp` body was changed or added to
  the build, and no Dreamcast body was ported.

- **2026-08-09 — `advmgr` 8 → 11 exact and a withdrawn `findpath`
  body restored; four retail-derived bodies exact, one jump-table body
  code-identical at 99.34%.** `advManager::GetCell(type_point)` (108 B),
  the five-argument `UpdateRadar` forwarding overload (39 B),
  `DrawRolloverText` (100 B), and `type_point::is_valid` (59 B) match
  byte-for-byte. The last body corrects the 2026-08-07 claim that it had
  no retail slot: 0x4b1330 is a 59-byte four-bound predicate immediately
  before `searchArray::searchArray`, and `GetCell` independently relocates
  to it. `OverrideBottomView` reproduces all 120 bytes and the six-entry
  switch table's resolved targets; objdiff reads 99.3415% because base
  VC6 names case labels locally while the delinker represents them as
  enclosing-function-plus-addend relocations, the already documented
  jump-table comparison residual. No Dreamcast body was ported: the
  implementations were reconstructed from retail instructions and
  relocation identities; the DC roster supplied signatures only.

- **2026-08-09 — Gruntz's generated per-function `cur` / `max` / `hist`
  score model approved for adaptation (user: “take idea from gruntz for cur,
  max, hist for matching functions”); manual baseline editing prohibited.**
  HoMM3 keeps the deliberately small part needed by its current pipeline:
  `cur` is refreshed from the current objdiff report, `max` is the enforced
  regression ratchet, and `hist` is the all-time peak that survives an explicit
  `homm3 status update --accept-regressions`. Stable retail RVA, rather than a
  working label, carries all three values across flat-to-decorated promotions.
  The status writer owns `config/match_baseline.tsv`; it marks the file
  generated, migrates the legacy three-column form, and automatically retires
  missing legacy 0% rows because they contain no historical achievement.
  Gruntz's broader fingerprint/tries machinery is not ported by this decision.

- **2026-08-08 — `ai` 6 → 11, `findpath` 9 → 11; the include-set class
  is MEMBER population, not just type count; and two change-sets that
  each reached 100 alone COMPOSE TO 94.07.** Engine-wide 534 →
  **540/977 exact (55.3%)**, **5.77%** matched. Ten bodies written,
  seven exact. One deliberate ratchet lower, described below.
  **THE INCLUDE-SET CLASS IS BETTER CHARACTERISED THAN IT HAS EVER
  BEEN.** Five clean-build measurements of `initialize_game_data` in
  one lane (three earlier readings taken against a *broken* build were
  found void and discarded — a good catch in itself):
  branch point 96.0880 · + a `mapcell.h` bitfield slice and a method
  declaration 96.0880 · + `CREATURE_NOMAD` in `armygrp.h` **alone**
  **26.1806** · + a 10-enumerator SoD block 97.0370 · + an
  **8**-enumerator block **100.0000**. Two conclusions: enumerator
  **position is irrelevant** (tail vs middle both 97.0370) — it is the
  **count**; and **the sensitivity is to MEMBER population, not only to
  the count of type definitions** — splitting `flags_00_11 : 12` into
  three named bitfields, no new type and no semantic change, reads
  26.1806. That bitfield is therefore deliberately NOT sliced, and
  `findpath` tests bit 6 as `flags_00_11 & 0x40`, which compiles to
  retail's `test byte ptr [cell+0xc], 0x40` unchanged.
  **DELIBERATE RATCHET LOWER, 100.0000 → 94.0741.** The hero lane and
  this lane each drove that row to 100 **alone, by different routes**;
  merged, the tree reads 94.0741. That composition is the strongest
  evidence yet that the row is not a property of any source text near
  `initialize.cpp` but a codegen artifact of its closure's type and
  member population — **and one that does not add.** Recovering 100
  would mean adding or removing names for score alone; every name in
  both change-sets is evidence-tight (the eight SoD ids are exactly
  what `NewmapCell::get_special_terrain` can answer with; the two that
  merely duplicate `CURSED_GROUND`/`MAGIC_PLAINS` were excluded on that
  ground, and `CREATURE_NOMAD` is pinned by `GetTerrainCost`). Carrying
  an unevidenced lever to hold a number is worse than an honest lower —
  the same judgement this row already carries from when it read 96.09.
  Hand-edited with a dated rationale per standing doctrine.
  **Lever bounded, not just added: a CROSS-TU accessor does not
  inline.** `berserk_attack` emitted a real
  `call ?getCellData@searchArray@@…` (defined in `findpath.cpp`) where
  retail open-codes the two-arm body; substituting
  `cellData == 0 ? 0 : &cellData[i]` took it **89.8 → 100** and carried
  the whole function's ESI/EDI colouring with it. This **bounds** the
  "call the inline accessor instead of open-coding it" lever landed
  the same day — it holds same-TU only.
  Other levers, each byte-reasoned: **declaration order colours the
  allocator** (declaring the `std::vector` before `long total = 0` lets
  VC6 keep one zeroed register for the three vector words, the EH state
  slot, the `_First == 0` compare and the running total — 94.4 → 100 on
  the swap alone); **post-decrement down-count**
  `for (unsigned i = size(); i-- != 0; )` reproduces retail's
  `test`/`je` entry and pre-decrement back edge where the signed form
  gets `cmp/jl` + `jns`; **float compare operand order IS the
  evaluation order** (`(float)points_left >= (float)full * K` converts
  `points_left` first, matching retail's `fild`/`fstp` pairing);
  **invert a tiny accessor's guard so the table lookup is the
  fallthrough**; **two separate `continue` guards, not one `||`**; and
  **a second extern symbol at the one-past-the-end address**
  (`gCastleWallGateTargetsEnd` at 0x63abf4) to give a pointer loop's
  bound an addend-0 reloc — **the sanctioned way around the DATA-split
  cap**, as against aliasing, which was rejected the day before.
  Claims corrected: **`army+0x10`/`+0x14` are the AI's CHOSEN TARGET
  (side, slot), not the stack's own identity** — `berserk_attack`
  copies the *target's* `combatSide` (+0xf4) and `bitIndex` (+0xf8)
  into them and clears both to −1 when it walks instead of striking;
  and `CalcTerrainCost`'s ninth parameter, previously called
  unnameable, is `get_creature_total(CREATURE_NOMAD) > 0`, whose only
  effect is erasing Sand's movement penalty.
  **A retail out-of-bounds read transcribed faithfully rather than
  corrected:** `should_stay_in_castle` uses each `TWallTargetId` from
  `gCastleWallGateTargets` (`{6,8,9,10,12}`) both as a `wallStrength`
  index (correct, id-indexed) *and* as a `gWallTargets` subscript
  (wrong — that table is position-indexed with eight rows, as
  `GetTargetWallIndex` 0x465970 proves).
  `DoCompAI` capped at 98.4697 by exactly one instruction (retail keeps
  an `and eax,0xff` our CL folds); **six spellings all landed on the
  same 98.4697**, and the two that differ are worse (87.8, 81.4).
  `place_shooter` 80.5659 is structurally settled and residual on
  register homing.

- **2026-08-08 — hero's spell-school family OPENED (49 → 58 exact); the
  blocker was never the enum; `initialize_game_data` back to 100 from a
  cause nobody could isolate.** Engine-wide 524 → **534/977 exact
  (54.7%)**, **5.65%** matched; all gates green.
  **The C2371 that blocked this family for two lanes was misdiagnosed.**
  It is not `TSpellSchool` at all — `hero.cpp` can never include
  `ai_tactical.h` under any spelling, because that header's
  `typedef int TSkillMastery` collides with `herospec.h`'s enum of the
  same name. The fix is the domain-header split, not a re-spelling:
  `TSpellSchool` now lives in **`include/spellschool.h`**, included from
  both sides — one definition, no duplication, the
  `artifact.h`/`herospec.h`/`prefs.h` precedent.
  Family: `get_spell_level` 100, `GetSpellSchoolLevel` 100,
  `GetManaCost` 100, `GetHighestSchool` 98.375.
  **SIGNATURE CORRECTED — retail over the Dreamcast, on two of the
  four.** The DC prototypes say `unsigned char is_on_magic_plains`.
  Retail reads that parameter with a 32-bit `mov` and switches it over
  **1..9** into a school mask (1→all, 6→water, 7→fire, 8→earth,
  9→air) — exactly the domain `NewmapCell::get_magic_terrain_type`
  (0x4fcf40) returns and `hero::Fly` pushes. The DC build kept only the
  magic-plains case and narrowed it to a bool; **retail's five-way form
  is the later revision.** New `include/magicterrain.h` carries
  `TMagicTerrain`, deliberately not `mapcell.h` (closure).
  New levers, each with its byte-level reason: **sunk fallback via `&&`
  + `else`** (`GetHighestSchool` 77.1 → 98.4 — hoisting the assignment
  keeps the mask dword in a register; the `&&`-with-`else` form sinks it
  into the air arm's else path, retail's `jmp` + `mov eax,[ebp+8]`);
  **an ascending `for` beats a hand-written downcount** (`GetExperience`
  98.6 → 100 — VC6's own induction-variable rewrite emits
  `add edi,-0xd`/`dec edi`/`jne`; writing that rewrite by hand emits
  `sub edi,0xd` and costs the last byte); **declaration order fixes
  register assignment** (declaring `total` first puts the parameter in
  EDI and the sum in ESI the way retail allocates; the other order
  swaps both); **switch bodies come out in SOURCE order**, so the ten
  arms had to be written in retail's emission order, not case-label
  order; **a plain `if` beats the ternary when retail hoists the
  constant** (`GetNecromancyCreature` 83.1 → 100 — `x >= 1 ? A : B`
  lowers branchlessly via `setl`/`dec`/`and 2`/`add`).
  **A DELIBERATE BACK-OUT worth +1 exact.** `hero::GetHeroSpellBonus`
  reaches 99.23% and is nevertheless left a `@stub` with its full
  reconstruction in the comment: `hero.obj` has exactly ONE call site
  for it today, so `/Ob2`'s single-call-site rule inlines it there and
  takes `modify_spell_damage` **100 → 32.6**, where retail plainly
  `call`s it. Retail's `hero.cpp` must have a second call site among the
  ~70 bodies still unreconstructed. Writing a correct body can cost more
  than leaving it unwritten, and the rule is now recorded.
  **`initialize_game_data` 96.0880 → 100.0000 and THE CAUSE WAS NOT
  ISOLATED.** Four closure headers changed together, and the one
  candidate A/B-measured alone (`armygrp.h` gaining the `spellschool.h`
  include) came back **neutral**. An earlier draft of the baseline note
  credited that include; it was wrong and is corrected in place, in both
  the header and the baseline. Counter-measurement the same session,
  same header, opposite sign: three `ESpellId` **enumerators** moved the
  row 96.09 → 90.16 and were withdrawn. **The row is now pinned at 100,
  which makes it a constraint on every other lane** — any change to
  `game.h`/`hero.h`/`mapcell.h`/`struct.h`/`town.h`/`armygrp.h`/
  `artifact.h`/`army.h` must re-measure it before landing.
  **GATE GAP, newly evidenced:** the hero lane mis-sliced the embedded
  `TCreatureTypeTraits` at `army+0x74` by four bytes and silently lost
  **56 functions** across `ai_tactical`/`ai_combat`/`cmbtmgr` for a
  build. `SIZE()` is a **clang-only** static assert; VC6 said nothing.
  A layout error inside a shared header is therefore invisible to the
  build that decides matches. Wants a VC6-visible size check.
  Layout landed: `SSpellTraits` +0x1c `school` (a dword, replacing a
  mis-modelled `byte_1c`) and +0x20 `int mana_cost[4]`; `army` +0x78
  `monInfoLevel`.

- **2026-08-08 — `kbwin` CLOSED (27 TUs now at 100%); a SYMMETRIC-
  REGISTER family identified across four TUs; a delinker artifact that
  will cap `DATA()`-relative reads.** Engine-wide 523 → **524/977 exact
  (53.6%)**, **5.53%** matched; all gates green. Only +1 exact, and the
  lane is worth more than that number: five stuck partials went from
  *two symptoms each* to *one root cause each*.
  **`kbwin::AppWndProc` 74.76 → 100 (916 B)**, three independent facts:
  (1) **`AppCommand` must not be inlined** — our `/Ob2` expanded its
  single in-TU call site, adding exactly its 5 branches (36 vs retail's
  31) and tail-merging the per-case epilogues. `#pragma
  auto_inline(off)` scoped to that one definition: 74.8 → 92.1. Retail's
  `AppCommand` has THREE call sites in the image (0x4ec253, 0x4ec275,
  0x4f7f59), two in another TU — so the retail compile saw the same
  one-call-site TU we do and still emitted the `call`. The source-level
  reason is unidentified and the pragma stands in for it, flagged
  in-source as load-bearing-but-unexplained (the `ai.cpp` codegen-pin
  precedent). (2) **The `DefWindowProcA` fallthrough is written TWICE**
  — an explicit `default:` arm *plus* the post-switch return; retail
  emits two copies, 15 rets to our 14, and the duplicate stopped VC6
  hoisting `window` into edi at entry. 92.1 → 99.16. (3)
  `WM_ACTIVATEAPP` is one if/else with a single trailing return, and
  the deactivate arm's `bAppDeactivated = 1;` sits **outside** its
  guard — retail's `jne` lands *on* that store (+0x480) rather than
  past it, while the activate arm's `= 0` stays inside. The asymmetry
  is retail's.
  **Wrong callee corrected:** `button::Main` calls `GameTime::Get()`,
  not `timeGetTime()`. The delinked target reads
  `call ?Get@GameTime@@SIKXZ`; `GameTime::Get` (0x4f82e0) is a 6-byte
  `jmp` through the same import, so the two are runtime-equal but
  different in the object. `button::Select`'s `timeGetTime` at +0x188
  is genuinely the thunk form and stays.
  **NEW RESIDUAL FAMILY — the symmetric-register swap, six functions
  across four TUs.** `window::CenterWindow` (ebx/edi), `button::Main`
  (retail esi=msg / edi=parentWindow), `misc::TPickANumber` ctor
  (retail esi=this / edi=span), and the allocator running out one
  register earlier in both `iconwdgt` partials. **In every case retail
  hands the lower register to the value used FIRST and we hand it to
  the second**, everything else instruction-identical. No source handle
  in any of them; this belongs with the merged-return / stale-CL
  question, not with per-function spelling.
  Root causes replacing symptom lists: `strip::DrawOwner` 93.37 is ONE
  allocation decision (we pick EAX for the `akHeroTraits` base where
  retail picks EDX; EAX still holds `frame`, so the load sinks below
  the index chain and the cross-jumper fires — the pos==0 twin escapes
  only because its extra `codeY = 122` store keeps EAX live);
  `iconwdgt::NextRandomSiegeEngineFrame` 86.41 is **loop-invariant
  hoisting** (our CL lifts the odds-table init into the preheader, so
  the literal 2 is needed once and takes volatile EAX; retail needs it
  every iteration, hoists it to callee-saved EDI and homes `this` at
  [ebp-4] — hence the 0x24-vs-0x20 frame and every table
  displacement); `soundmgr::ConvertVolume` 67.5 is one defect, not two
  (retail's clamp tails are one instruction longer, crossing VC6's
  cross-jump threshold so both clamps are shared — 4 exits to our 6).
  Two measurements that kill spellings outright: a
  `const THeroTraits* traits = akHeroTraits;` base local is **folded
  away entirely** (byte-identical, not a distinct spelling), and a
  `goto retry` bottom-tested loop is byte-identical to the `while`
  (VC6 recognises the same natural loop and still hoists).
  **PIPELINE CAP FOUND, deliberately not worked around:** `src-DATA`
  labels carry no size (`scripts/homm3/build/labels.py:266-270` emits
  `"size": ""`), so vostok synthesises a separate `bss_29959c` for
  `rcAppWindow.top` at 0x69959c and our `rcAppWindow`+4 reloc can never
  equal the target's addend-0. **This will cap any function that reads
  a struct member at a non-zero offset from a `DATA()`-claimed
  aggregate.** An alias probe confirmed the mechanism and was
  REVERTED — modelling a delinker artifact would make our object differ
  from the true retail object. Giving `DATA()` an optional size is a
  contract change and is **open for a supervised decision**.
  `initialize_game_data` untouched; the 96.0880 pin and its dated
  rationale intact.

- **2026-08-08 — `game` 6 → 35 exact; the BOOL-RETURN rule; the DC local
  list promoted to a spelling oracle; `type_point`'s retail alignment
  found.** Engine-wide 491 → **523/977 exact (53.5%)**, 5.39% →
  **5.52%** matched; all gates green. Thirty `game` stubs written, 29
  exact first-or-second try; `hero` 46 → 49. The `sema xref` body walk
  cleared eleven brackets.
  **New codegen rule — a `bool` return materialises in AL only when the
  returned value is a bool LITERAL or LOCAL.** `return x != 0;` emits
  `xor eax,eax` + `setne`; `return x ? true : false;` and
  `if (c) return true; return false;` emit retail's `mov al,1` /
  `xor al,al` / bare `setne al`. Six functions, +6 exact — and the DC
  mangling (`_N`, `QBA`) predicted every one of them in advance.
  **`evidence/dreamcast/variables.csv` is a SPELLING ORACLE, and it
  cuts both ways.** `GetNumObelisks`' DC local list is `i, numFound` —
  no `mask`. Deleting the `int mask` local and inlining `1 << player`
  took it **73.8 → 100** and reproduced retail's allocation exactly.
  This is the **inverse** of the named-local lever landed the same day;
  the DC local list is what decides which direction to go. Two levers
  that contradict each other are only usable because the roster
  arbitrates.
  **The goto-loop form** — `if (i >= N) return X; loop: …; i++;
  if (i >= N) return X; goto loop;` — reproduces retail's
  `jge END; jmp L` back edge *and* makes VC6 fold the redundant `>= N`
  out of an inlined clamp. `IsLastHuman` 70.7 → 100, `HasCapitol`
  92.9 → 100, `GetLocalPlayer` 76.3 → 100 via `goto found`.
  Also: **call the inline accessor instead of open-coding it**
  (`gpGame->GetTown(i)` for `&gpGame->towns[i]` took
  `get_obscured_town` 88.9 → 100 — the −1 arm then tail-merges with the
  outer `return 0`); member-initializer lists to place stores BEFORE an
  embedded member's ctor; and **probe TUs** (`cc_wrap … /FAs`, ~5 s per
  ten-spelling sweep) as the cheap way to settle a spelling before
  touching the real TU.
  **The "unexplained uninitialised stack read" in `town::town()` is
  explained and CLOSED.** `playerData::playerData` opens with the same
  `mov cl,[ebp-1]` + store: it is Dinkumware's **empty-allocator copy**
  for an embedded `std::vector` — `vector()` copies a temporary
  `allocator<T>()` into the container's +0. Both ctors are exact with a
  plain `std::vector` member and no special spelling.
  **Retail's `type_point` is 4 bytes, ONE-byte aligned; the Dreamcast's
  is two-byte aligned.** `playerData::puzzle_guess` sits at the odd
  +0x39 between `extraPuzzlePieces` (+0x38) and `iDeathCountDown`
  (+0x3d), and `playerData::Init` confirms the bit layout from the
  other side (`or word [+0x39],0x3ff` / `or word [+0x3b],0x3fff`).
  `struct.h`'s short-bitfield spelling reproduces the DC alignment, not
  retail's. **`struct.h` was deliberately NOT changed** — that is a
  tree-wide move needing a supervised decision; the member is carried
  as raw bytes with the finding documented in place.
  **CLAIM CORRECTED: 0x4ba130 was claimed as `playerData::SetName`; the
  body is `AssignNetInfo`** (dword dpid at +0, `strncpy` 20 from +4,
  then `isHuman`). Arity does not separate the two — both p=2, both
  `ret 4` — only the body does. `SetName` and `GetNetInfo` have no
  retail row in the span. Recorded as a dated comment above the
  affected baseline rows, with the 32 flat/mangled deletions it belongs
  to.
  **Measured and therefore admitted: `#include <vector>` and
  `#include "town.h"` in `game.h` cost exactly ZERO** across all ten
  including TUs, both directions — which unblocked honest
  `std::vector<town|mine|generator|boat|type_point>` pool models
  instead of pointer-triple approximations. `initialize_game_data`
  measured 96.08796, exactly its pin, across six header changes.
  DC-roster transfer is now a three-way result, not a rule: it repacks
  **unshifted** for `playerData`, `boat` and `generator` (DC 92 IS
  retail's generator stride; mapX/Y/Z at 84/85/86 exact) but **not**
  for `mine` (DC 12 vs retail 64), whose tail stays a pad.
  New headers admitted: `include/netplayer.h` (`CNetPlayerInfo`;
  DC-declared in `struct.h` but placed apart from `initialize.cpp`'s
  tripwire closure, the `artifact.h`/`prefs.h`/`herospec.h` precedent)
  and `include/netgame.h` (`enum eNetGameType` + `iMPNetProtocol` at
  0x6989f0 — `== 3` is `MP_HOTSEAT`, which is what both local-player
  accessors branch on).
  Residual: `playerData::GetName` 95.85 and `game::GetPlayerName` 99.53
  are four instructions of argument setup (retail runs the default-name
  chain eax→ecx and slots the `lea` between loads two; ours starts in
  ecx and spends edx) — the register-tie-break family.

- **2026-08-08 — HALF THE CARVE IS EXACT (491/977, 50.3%); the naming
  lever generalises into FOUR source-level shapes; `TSpellSchool`
  proven codegen-neutral.** Engine-wide 480 → **491 exact (50.3%)**,
  5.35% → **5.39%** executable matched; ratchet clean, eight
  cleanliness floors at 0, va-claims clean, single-view 0 splits.
  **The lever that closed the SpellCastWorkChance family is one case of
  four.** All four are byte-proven, and all four are invisible out of
  line — they only move bytes once the callee is INLINED, which is why
  they were never found by looking at the callee's own diff:
  1. **Overwrite the variable; do not make a new one.**
     `type_monster_data::take_damage` returns its own PARAMETER,
     reassigned in an if/else, not a `dealt` local with two returns.
     One edit: `inflict_melee_damage` 97.0, `cast_area_effect` 97.4 and
     `cast_damage_spell` 94.2 **all to 100**, `cast_chain_lightning`
     71.9 → 79.7. Same shape closed `get_traitor_value` 90.5 → 100.
  2. **Three-operand selector.** `get_total`'s null test is a `?:` on
     the return expression, not a split `if`. 83.6 → 100, cascading
     through its ~12 inlined copies: `get_area_value` 86.6 → 100,
     `cast_area_effect` +11, `do_general_melee` 79.6 → 94.8.
  3. **A cached member local is not free — it crowds out `this`.**
     `long odds = params.odds;` cost `get_speed_value` its whole match
     (82.4 → 100 by deleting it and re-reading the member four times).
     In `get_defense_boost_value` 96.2 → 100 **both** cached locals had
     to go; dropping only one changed nothing.
  4. **The literal lever** (bind an inline argument to a named local)
     closed the newly written `mark_firewalls`, 86.4 → 100.
  Also: a DUP-EXIT goto plus a tail rewrite took
  `get_ranged_attack_value` 75.5 → 100; a `short` loop index (forcing
  retail's separate `dec edi` down-counter) closed `adjust_army`; and
  hoisting the armies-row base to a named local took
  `get_hypnotize_value` **18.2 → 96.3**, correcting an older in-tree
  note whose "zero register at the top" diagnosis was a consequence of
  the missing hoist, not the cause.
  **`TSpellSchool` promoted from `typedef int` to the real DC enum**
  (`include/ai_tactical.h`, names and values verbatim from
  `evidence/dreamcast/enums.csv`; it is a BITMASK, and
  `kNumSpellSchools` sharing `eSchoolWater`'s 4 is the dump's own
  doing). The typedef could not be made visible from `hero.h` without
  C2371 and was blocking five hero bodies. **A/B-measured: ZERO of the
  977 scored functions in all 52 units moved by a byte** — this is DC
  naming evidence over an int-sized domain, claiming no retail byte.
  Six `get_protection_value` call sites lost bare `1/2/4/8/15`
  literals in the same change.
  `mark_firewalls` (0x4214f0) written exact; it slices
  `TObstacle::spell_damage` at +0xc. Two facts **transcribed as found
  rather than corrected**: retail passes `estimate->lowest_attack`
  TWICE to `get_loss_combat_value` (`lowest_defense` is never read at
  0x4218b8), and the spell id stays a bare `0xd` because the roster
  lives in another lane's `armygrp.h` — it wants
  `SPELL_FIRE_WALL = 0xd`, flagged rather than reached across for.
  Capped with in-tree residual notes: `get_hex_attack_value` 84.4 and
  `CalculateGainedExperience` 75.0 are the same **first-callee-saved-
  push tie-break** (retail pushes EDI first and parks `this` there; we
  push EBX); `get_attack_change` 96.4 is a symmetric-parameter
  esi↔edi swap; `do_general_melee` 94.8 is **/Ob2 inliner depth**
  (retail expands `get_total` inside both inlined `kill()`s, our budget
  runs out after the first); `can_take_town` 98.8 rejects naming
  `gpGame` in all five positions tried (83.5–93.0).

- **2026-08-08 — hero 39 → 46 exact; the DC roster REPACKED reproduces
  retail's layout, and two "rules" are demoted to per-function.**
  Engine-wide 466 → 480 exact, 5.25% → 5.35% matched. Seven of eight
  written bodies were exact on the FIRST try, including a 497-byte
  `ApplyBattleWinTemps`.
  **The modelling result outranks the score: the Dreamcast `hero`
  roster, repacked with NO alignment, reproduces retail's layout
  exactly.** It PREDICTED `facing`@0x47 and `ArenaFlags`@0x73 before
  retail bytes confirmed them and agrees at eleven independently proven
  points, which makes hero's whole 0x23..0x12c band cheap to model.
  Used strictly as a hypothesis generator — only bytes-proven fields
  were committed.
  **Two rejections worth as much as the wins.** Respelling `GiveSS`
  around a `signed char* pLevel` local to fix `SetSS` looks right —
  retail's own `lea edx,[esi+ecx+0xc9]` invites it — but it costs
  `GiveSS` 100 → 73.4: **the address CSE is the optimiser's, not the
  source's.** And `is_in_patrol_radius` INVERTED the guard rule (retail
  genuinely has the sunk shared block; split early-outs scored 70.5) —
  **the guard rule is per-function, not directional.**
  Ratchet trap handled: two baseline rows differ by one suffix —
  `hero_remove_artifact_e2dd0` was superseded by a promotion while
  plain `hero_remove_artifact` (0x4e2bd0) is a live stub. Only the
  former was deleted.
  **New pipeline hazard recorded for every lane: delinking while any TU
  fails to compile POISONS the target side**, presenting as a spurious
  77-function drop. Confirm a clean `homm3 build --fast` before
  `homm3 delink`.

- **2026-08-08 — the LOCATE phase: 132 carve rows claimed across two
  lanes; `sema xref` established as the primary locate instrument; the
  border illusion measured and removed.** Engine-wide 405 → 414 exact;
  the baseline denominator went **740 → 877 rows** as unclaimed carve
  rows were brought in, and per-unit fuzzy fell correspondingly — that
  drop is the honest denominator arriving, not a regression. The
  whole-engine figure is **414 / 4,749 (8.7%)**.
  **The binding constraint is no longer spelling, it is unclaimed carve
  rows.** `ai` had read 5/7 exact while 26 of the 33 carved functions in
  its span were simply unclaimed — 2,316 of 14,873 bytes, ~16%. Per-unit
  percentages had been measuring claims against claims.
  Claimed this phase: findpath 10 → 0 unclaimed, ai 27 → 3,
  ai_tactical 39 → 2, hero 41 → 2, town 13 → 0, mousemgr 1 → 0,
  misc 8 → 4, game 111 → 106.
  **`homm3 sema xref` is the strongest locate tool in the box** and
  should lead every locate lane. Real rel32 edges settled `town`
  outright — `give_event_reward → show_building_rewards/
  show_creature_rewards`, and `town::initialize → initialize_buildings
  → check_shipyard_square` — and also `hero::CheckLevel →
  get_skill_award`, `ApplyBattleWinTemps`'s two callers, and misc's
  `WritePrefs` jmp edge. **All of these are static-helper-after-caller**
  (occurrences 5–8 of that pattern): retail defines a static helper
  AFTER its caller where the DC source has it before, so rank alone
  mis-maps them and arity plus the call graph settles them.
  **THE "FORCED" BRACKET IS A TRAP — new cautionary rule.** hero's
  `0x4db350..0x4e2340` presented 24 DC rows against 24 carve rows, a
  perfect count match that turned out to be **two compensating errors**:
  `0x4dbd90` is a 30-byte `get_last_backpack_index`, not the 526-byte
  `handle_artifact_click`. **Count equality is not proof.** Every
  pairing must come from bodies, arity or rel32 edges.
  **A clean `/Ob2` witness:** `0x4c6f80` contains `StartAITheme` inlined
  AND `0x4c6f40` still exists out-of-line — single-call-site inlining
  with unconditional extern emission, exactly as the skill documents.
  All three music functions compiled byte-exact, which retroactively
  proves the identification.
  **Ratchet subtlety worth recording:** the `mousemgr TCSLock_TCSLock`
  row went MISSING and was NOT the constructor despite its name — it
  was the delinker's fallback label for the DESTRUCTOR at 0x50cd80,
  which the carve had mislabelled. Claiming the dtor renamed the row to
  `??1TCSLock@@QAE@XZ` at 100%, so it was a genuine rename; the real
  constructor at `_10d890` correctly survives at 0.0000. A flat row
  going MISSING is not automatically a rename — check which function it
  actually named.
  Also: `getCellData` and `Clear` (findpath), `searchArray::Init`/
  `Close`/`lower_door`, and nine hero/game/mousemgr functions driven to
  exact. `Clear` needed two register-colouring levers — the memset
  destination must be an ASSIGNED LOCAL rather than an accessor call
  (the accessor's two-return form makes VC6 load the member twice), and
  the fly-plane flag must be `unsigned char` (a plain int comparison
  makes VC6 clear a scratch register where retail reuses one).
  **OPEN, spans two lanes:** `hero::SetSS`/`TakeSS`/`GiveSS` are
  byte-decoded but unwritten — they need `hero+0xc9` modelled as
  `skillLevel[28]`, and `ai_combat.cpp` reads `hero::ballisticsLevel`
  out of that same band. One rename plus one reference change yields
  three near-certain exacts, but it crosses lane ownership.
  **`game`'s remaining 106 needs a different instrument:** 946 DC rows
  against 176 carve rows in the span means most DC rows were inlined
  away, so rank alignment carries no information. The `sema xref`
  caller-graph approach is the way in, one bracket at a time.

- **2026-08-08 — `widget` CLOSED 12/12 at 100%; store order proven
  PER-FUNCTION; a third pipeline cap pinned by elimination.**
  Engine-wide 333 → 337 exact, 4.25% → **4.27%** executable matched.
  Also `misc` 70.10 → 89.58%, `strip` 95.28 → 97.97%, `textntry`
  97.86 → 98.78%.
  **`widget` CLOSES against the skill's bar, not the scoreboard:** all
  12 carve rows in the rechecked span 0x5fe340..0x5fe9c3 claimed AND
  exact; both flanks are the excluded cinit class (guard byte 0x6abaa0
  + atexit, 95-byte ten-iteration funclets); the 13-row DC roster is
  exhausted, `widget::Close` having already been attributed as the
  ICF-folded `/Gy` COMDAT at 0x5bc690. Fourth TU closed (after zlib,
  hexcell, sample, winmgr).
  **Two of the twelve were FOUND, not fixed** — a pattern worth
  hunting elsewhere: `config/retail-functions.tsv` carried unclaimed
  rows at 0x5fe3b0 and 0x5fe410 *inside* the span, both parked as
  `DC_ONLY` carcasses on the false premise that no retail body existed.
  Body evidence promoted them (`??_G` stores the widget vtable, clears
  `last_hover_widget`, ends on the `flags & 1 → operator delete` tail;
  the default ctor is 29 B thiscall storing exactly five fields), and
  **both came out byte-exact on the first compile** — which is itself
  the confirmation. A `DC_ONLY` carcass sitting on a real carve row is
  free exactness.
  **NEW DOCTRINE — message-field store order is PER-FUNCTION, not a
  house style.** `send_message` 79.43 → 100 and `enable` 89.56 → 100
  came from an exhaustive **720-permutation sweep** of the store order
  against retail bytes, run out-of-tree so the build was untouched.
  Five orders are exact for send_message, four for enable; the shared
  invariant is **the zero run must precede the `codeY` store**. The
  house order used by strip's already-exact `DrawMonster` family is NOT
  among them (23.6%). The sweep technique generalises to any TU that
  builds `message` objects.
  **THIRD PIPELINE CAP, pinned by elimination rather than assertion:
  post-RA scheduling with an IDENTICAL BYTE MULTISET.**
  `initialize::create_included_mask` (93.87) differs only as
  `mov ecx,0x56 / mov esi,edx / lea edi,[edx+8]` vs our `lea` hoisted
  two slots. Three hypotheses were ruled out by measurement: NOT
  reloc-name cost (`add_to_included_mask` carries the *same*
  `bitNumber+4`→`data_26cd9c` and `kCommonIncludeList`→`data_23fbc4`
  rows and scores 100.00); NOT include-set sensitivity (a 0..8
  dummy-struct sweep leaves it at 93.8667 at *every* count, while
  `initialize_game_data` swings 100/100/100/94.07/94.07/100/100/100/100
  across the same sweep); NOT scheduler target (`/G5` byte-identical to
  the default `/GB`; `/G6` also 93.8667).
  **An inherited residual note was WRONG and is corrected:** it claimed
  every spelling that fixes create_included_mask costs
  `initialize_game_data` its exact match. No spelling fixes it — 16
  were tried.
  **Discipline worth repeating:** `misc`'s ctor `flag` byte
  (`mov dl,[ebp+0xb]` in retail, the root of the whole remaining
  register cascade) was probed with a byte-alias spelling purely as a
  diagnostic. It scored **worse** (72.13 vs 72.18), so it was rejected
  rather than asserted, and no `reinterpret_cast` was introduced — the
  clean `>>24` stand-in stays.
  **Not attempted, with reasons:** `exec::CallManager` (68.37) needs an
  unidentified RAII type — retail carries a real `fs:[0]` frame with an
  unwind table and a `mov byte ptr [ebp-X]` state store we do not
  model; that is research, not spelling. `exec::MainLoop` (88.13)
  plateaus on retail memory-homing `dispatch` while keeping constant 1
  in `edi` (do-while and `for(;;)` both drop it to 65.1).

- **2026-08-08 — media lane: soundmgr 98.05%; the RELOC-NAME theory
  REFUTED; two pipeline-level caps pinned precisely.** Engine-wide
  330 → 333 exact, 4.18% → **4.25%** executable matched. `soundmgr`
  16/21 → **18/21 (67.36% → 98.05%)**, `inputmgr` 5/10 → 6/10.
  **THE ORCHESTRATOR'S RELOC-NAME HYPOTHESIS WAS WRONG, refuted two
  independent ways — record this so it is not re-derived.** The claim
  was that objdiff scores relocation targets by symbol name, so
  unclaimed data globals cap their referencing functions, and that
  adding `DATA(...)` claims would close the gap.
  (a) **objdiff does not score reloc names at all.** `StopMP3` is
  exactly **100.0000%** while carrying five mismatched data reloc names
  (`?gMP3Stream@@3PAXA` vs `bss_29fe78`, `?bShutDownDone@@3EA` vs
  `data_299608`, `?service_sounds@@YAXPAX@Z` vs
  `soundManager_service_sounds`) — verified independently by the
  orchestrator. smackmgr shows ten more such rows on already-exact
  functions.
  (b) **A `DATA` claim could not have produced a matching name anyway:**
  `labels.py` names src-DATA rows `data_{rva:x}`, never the mangled C++
  spelling. A `DATA(0x006994e0)` probe moved the target row
  `bss_2994e0` → `data_2994e0` and `MouseMessageHandler` stayed at
  exactly 99.9645%.
  **The skill's "reloc-name-only rows on data are cosmetic" line is
  therefore CORRECT as written.** The wrong conclusion came from reading
  the first 30 lines of a masked diff and inferring causation from the
  only rows visible — the third time in one day that truncated or
  masked output produced a wrong answer, after the initialize_game_data
  misread and the `type_obstacle_shape` offset that sat at 99.99%.
  **Separately confirmed and worth a gate: `labels.py:193` globs
  `SRC_DIR` only (`*.c*`), so the five `DATA(0x...)` claims in
  `include/armygrp.h` and `include/game.h` are INERT** — they bind
  nothing. A claim that can never take effect should be a hard error,
  not silence.
  **Two caps pinned precisely, both PIPELINE-level, neither
  source-addressable:**
  1. **Symbol-table resync (AsciiConvert, 98.80%).** An address-keyed
     compare shows base and target **byte-identical AND reloc-identical
     over the whole [0, 0x1c6) span**. The gap is in the SYMBOL TABLE:
     the base COMDAT still carries two storage-class-6 `$L` LABEL
     symbols at +0x144 and +0x190, and because the 0x36-byte index
     table has no relocations those labels are the only disassembler
     resync points — so the same 54 data bytes decode into different
     rows. Fix is a canonicalizer change (drop label-class symbols once
     their relocs are rewritten). This also supersedes the earlier
     "jump-table label spelling" framing: the mechanism is leftover
     label SYMBOLS, not the addend encoding.
  2. `MouseMessageHandler` 99.9645% is the same family, trailing row.
  **Exact this lane:** `inputManager::Open` 42.37% → exact (the claim
  was UNDER-RECONSTRUCTED, not mis-spelled — retail also calls
  `MakeScanCodeTable()` as a real call despite one call site, sets
  id/priority/status and `strcpy`s "inputManager"; `priority = -1` and
  the inline `strlen` counter share one `or ecx,-1`, signedness CSE);
  `SetMusicVolume` 0 → exact first try (all five parked "missing views"
  resolved — 0x6993d0 = gpCombatManager, 0x699268 = gpAdvManager,
  0x66fedc = `"combat%02d"`, 0x678330 = the terrain→music table, and
  the `vol==0` arm's thread entry is the SAME one `StopMP3` uses, so
  that arm is `StopMP3()` inlined rather than a distinct body);
  `StartMP3` 0 → exact.
  **New lever (StartMP3's last 9%):** spell a name-match early-out as a
  **negated outer `if` with the `Leave` textually last**, not
  `if (==0) { Leave; return; }` and not a `goto` — both of those make
  our CL duplicate the epilogue in place, while the negated form lets
  VC6 sink the `Leave` into the single shared epilogue AND flips a
  tail-merge into retail's direction. Two residuals, one placement
  decision.
  **New named residual class — vptr-store scheduling.**
  `inputManager`'s ctor (86.57%) matches instruction for instruction,
  but retail sinks the compiler's own vptr store past the whole
  64-entry loop where our CL pins it after the base ctor call. Hoisting
  `keyboardFilter = 1` above the loop scored 78.2% and the vptr store
  STILL led, so no statement order can sink it. `soundManager`'s ctor
  (99.03%) shows the identical signature one slot wide — systematic,
  not local. Cross-linked in both files.
  **`KeyboardMessageHandler`'s blocker narrowed from three items to
  one:** 0x699280 and 0x699268 are `gpWindowManager` and `gpAdvManager`
  (both modelled), but `advWindow`'s static type is NOT `heroWindow` —
  that class is 0x4c bytes, so +0x58 is past its end. It is the derived
  adventure-map window, and `TAdventureMapWindow` has no layout
  anywhere (adventuremapwindow.h is a comment-only carcass). Full
  decode recorded in-source; once that class lands it is transcription.

- **2026-08-08 — `ShotIsThroughWall` exact; six NH3API-lineage
  enumerator names ADMITTED (user: "commit everything").** Engine-wide
  329 → 330 exact, 4.17% → **4.18%** executable matched; `cmbtmgr`
  15/35 → 16/35.
  `combatManager::ShotIsThroughWall` (0x00467510, 0xEA) landed at
  **100.0000% on the first spelling**, verified UNMASKED — every
  instruction byte identical, the only deltas being obj-local addresses,
  target-side flat reloc names and trailing alignment `nop`s. The
  split-if guards tail-merged into retail's single shared `xor al,al`
  epilogue on their own; no `||`-sinking or `goto` variant was needed.
  **NAME ADMISSION (the substance of this entry).** Six enumerators
  entered `include/armygrp.h` with **NH3API lineage**, which the
  supervised-review rule governs. Every VALUE is retail-proven — the
  four creature ids read off the compare chain at 0x46753d
  (`0x22, 0x23, 0x88, 0x89, 0x95` in that order), the two artifact ids
  are the pushed immediates: `CREATURE_MAGE 0x22`, `CREATURE_ARCH_MAGE
  0x23`, `CREATURE_ENCHANTER 0x88`, `CREATURE_SHARPSHOOTER 0x89`,
  `ARTIFACT_GOLDEN_BOW 0x5b`, `ARTIFACT_BOW_OF_THE_SHARPSHOOTER 0x89`.
  The NAMES are corroborated two ways: semantically these are exactly
  HoMM3's no-wall-penalty shooters and the two obstacle-penalty
  artifacts, and positionally they are bracketed by ids this enum
  already byte-proves — `CREATURE_STONE_GOLEM/IRON_GOLEM` 0x20/0x21 sit
  immediately below the Mage pair in Tower dwelling order, and
  `AZURE_DRAGON` 0x84 / `CRYSTAL_DRAGON` 0x85 / `HALFLING` 0x8a leave
  0x88/0x89 exactly where Enchanter and Sharpshooter fall in the neutral
  block. Lineage is flagged in-comment per house rules.
  **Third DC-roster correction in this family, and the second of its
  kind:** the roster types this `int`, but retail's early exit is
  `xor al, al`, not `xor eax, eax` — a BYTE return. The control is
  `is_adjacent` in the same TU, declared `unsigned char` and exact,
  emitting precisely that. This follows `hero::IsWieldingArtifact`
  (212 of its 224 call sites follow with `test al, al`), so DC `int`
  returns in this family should be treated as suspect by default.
  Also: the first parameter is `const army*`, not the DC roster's
  `int group`. And a near-miss worth recording — **the two inlined wall
  tests are `InCastle`, not `LeftOfMoat`**: both magic-divide blocks
  relocate against `const_23bd00` = `gCastleWallColumns`, while
  `gMoatColumns` at 0x63bce8 is never touched. The two helpers are
  adjacent 0x22-byte twins and the wrong one scores nowhere.
  No regressions: adding `#include "hero.h"` to cmbtmgr.cpp did not
  move any of the 15 previously-exact functions in that TU
  (`RemoveObstacle` held at 87.2872) and the enum additions moved
  nothing tree-wide — a clean negative result for the include-set
  sensitivity class identified earlier today.

- **2026-08-08 — cmbtmgr 2/35 → 15/35; the combatManager model built
  entirely from retail bytes, with Ghidra deliberately unused.**
  Engine-wide 316 → 329 exact, 4.07% → **4.17%** executable matched;
  `cmbtmgr` 0.54% → 14.27% fuzzy, unwritten stubs 33 → 19.
  Thirteen exact: GetTargetWallIndex, LeftOfMoat, is_adjacent,
  enemy_is_adjacent, CombatIsOver, HexIsBlocked, should_lower_door,
  IsInMoat, RemoveArmyFromGrid, PlaceArmyInGrid, IsWinner,
  PlaceObstacle, get_distance. RemoveObstacle plateaus at 87.29% on a
  register-allocation decision (retail keeps `obstacles_begin` in EAX
  across three guards and touches neither EBX nor EDI until after the
  bound check).
  **The lane never opened `evidence/ghidra-structs/` — correctly.** The
  0x1000 cutoff leaves that table silent about a 0x140ec object, so
  every field below is its own byte proof: `drawbridgeState` @0x53a4
  (LowerDoor steps 3→2→1 one DrawFrame apart, RaiseDoor gates on ==1),
  `field_53a8`, `field_53c8` (RaiseDoor null-checks it then
  `cmp byte [eax+4], 7` = TOWN_FORTRESS), `field_132b0[2]`/
  `field_132b2[2]` indexed by SIDE as bytes with 0x132b2 tested first in
  both consumers, `field_132f4`, `obstacles_begin`/`_end` @0x13d5c/
  0x13d60 (`end-begin` divided by 24), nested `TObstacle` 0x18 whose
  `sprite` is virtual-called at **vtable slot 1** with no args and then
  nulled (= `CSprite::Dispose`), `type_obstacle_shape`,
  `type_wall_target` + `gWallTargets[8]`, `gMoatColumns`,
  `gOuterMoatColumns`, and enums `EDrawbridgeState`/`ECombatGateHex`.
  **A masked diff would have hidden a modelling error here:**
  `type_obstacle_shape`'s `offsets` member was first placed at +7 and
  sat at 99.99% until the UNMASKED disasm exposed it; `pad_07` fixed it
  to exact. Same lesson as the initialize_game_data misread.
  **New lever — `std::max`'s shape needs a const-reference-in,
  const-reference-out helper** to reproduce retail's home-both / `lea` /
  `lea` / deref select; every inline-ternary and by-value spelling
  plateaus at 93.3%. VC6's `<algorithm>` does NOT export `max` into
  `std`, so the template is written out locally. Also: **`goto` vs
  `break` cuts both ways** — in CombatIsOver a `goto` out of the inner
  loop killed VC6's induction-variable elimination (96.15 → 46.62) and a
  single-call-site file static gave retail's CFG and the pointer IV
  together, while in IsWinner the same `goto` was harmless and removed a
  duplicate bound test.
  **Inherited claims corrected (twelfth consecutive lane).**
  `include/army.h`'s note that the IsWinner walk tests `army+0x84`
  against −1 is **FALSE**: both consumers, now byte-exact, test
  **+0x34 `creatureType`** and read +0x84 only as a BITFIELD (one dword
  load feeding `shr 6/21/22`) — so +0x84 is a flags word, not an id.
  `army+0xf8` is also the grid slot (PlaceArmyInGrid narrows it to a
  byte and stores it as the cell's `armySlot`, exactly as it stores
  +0xf4 as `armySide`); left renamed-pending since the ai_tactical call
  sites belong to another lane. **Four DC-roster *methods* are free
  `__fastcall` in retail** (GetTargetWallIndex, LeftOfMoat,
  get_distance), joining the existing InCastle precedent — all three are
  exact under that signature. `enemy_is_adjacent`'s DC `const army*`
  first parameter must drop const, because retail calls the non-const
  `army::is_enemy` on it.
  **OPEN — `ShotIsThroughWall` (0x00467510, 234 B) is fully decoded and
  blocked only on six enumerators** in `armygrp.h`, which a concurrent
  lane owned at the time: `CREATURE_MAGE` 0x22, `CREATURE_ARCH_MAGE`
  0x23, `CREATURE_ENCHANTER` 0x88, `CREATURE_SHARPSHOOTER` 0x89,
  `ARTIFACT_GOLDEN_BOW` 0x5b, `ARTIFACT_BOW_OF_THE_SHARPSHOOTER` 0x89.
  Values are retail-proven; the names are NH3API lineage and the
  semantics corroborate exactly (these are HoMM3's no-wall-penalty
  shooters and the two obstacle-penalty artifacts). Its first parameter
  is an `army*`, NOT the DC roster's `int group` — the body reads
  +0x288/+0xf4/+0x34. Needs `#include "hero.h"` in cmbtmgr.cpp.
  **Process:** the lane's first pre-delink build invented 28 flat
  0.0000 baseline rows, which it reverted before proceeding — a second
  independent demonstration of why the order is build → delink → build.

- **2026-08-08 — recruit reconstructed across two lanes; the JUMP-TABLE
  LABEL CAP identified as a pipeline-level residual class.** Engine-wide
  312 → 316 exact, 3.97% → **4.07%** executable matched (crossing 4%);
  `recruit` 1/3 exact 6.34% → 5/10 exact **93.69%** fuzzy.
  Lane 1 landed six previously-unlocated stubs — both `TRecruitWindow`
  and `TRecruitQuickWindow` destructors plus their scalar deleting dtors
  (all four exact on the FIRST compile) and the two `recruitUnit` ctors
  at 97.98% / 96.67%. Lane 2 cleared both remaining blockers:
  `recruitUnit::Update` (1,428 B) 0 → **90.84%** and
  `siege_artifact_to_creature` 0 → **99.47%**.
  **NEW RESIDUAL CLASS — jump-table label spelling, NOT fixable from
  source.** `siege_artifact_to_creature`'s *code* is byte-identical; the
  only delta is that VC6 emits one local label per case
  (`$L47262…$L47265`) with a ZERO addend, while the delinker can only
  name the enclosing function and carries the case offsets
  (0x0f/0x15/0x1b/0x21) as addends. **This caps every jump-table-bearing
  function just short of 100%** — `recruitUnit::Update` carries the same
  table. Wants a pipeline decision (teach the delinker to emit per-case
  local labels), not more spelling attempts.
  **The `min()` block was proved, not guessed:** `std::min` does not
  exist in this toolchain — VC6/Dinkumware spells it `std::_cpp_min`
  (macro `_MIN`) in `<xutility>`. Retail's shape (two memory slots,
  address-select, deref) needed
  `long maxBuy = maxAvail; numberToBuy = std::_MIN<long>(numberToBuy, maxBuy);`
  — the explicit `<long>` is what creates the `int→long` conversion
  temporary at `[ebp-4]`, and VC6 parks `maxBuy` in the DEAD `slot`
  parameter's home at `[ebp+0xc]`, exactly as retail does. **The
  template-deduction failure (`'_Ty' is ambiguous`) was itself the
  evidence** that the two operands differ in type.
  **`recruitUnit`'s layout was byte-CONFIRMED, not adopted** from the
  Ghidra lead: every offset the TU uses is proven by a named body (base
  `baseManager` at 0x38 via all three ctors calling 0x44d530 then
  storing vptr 0x640c70; 0x48 type … 0xb8 numberToBuy; size 0xbc), and
  five DC-named fields with no local proof (0x38, 0x80, 0x94, 0xa0,
  0xa8) were left as PADDING rather than fabricated. The lane also
  contradicted the lead's own labels: 0x151560, which the HD map calls
  plain `recruitUnit::recruitUnit`, is specifically the **town** ctor
  (`ret 0xc`, calls `town::get_army`); 0x151350/0x151460 are the DC 10-
  and 9-parameter ctors (`ret 0x28` / `ret 0x24`). This is the intended
  standing for `evidence/ghidra-structs/` — leads, never claims.
  **`playerData::resources[7]` at +0x9c** is byte-proven from two
  UNRELATED TUs: `recruitUnit::Update` (0x550274) and
  `TResourceDisplay::Update` (0x558f45), which prints
  `[player + 4*id + 0x9c]` for the seven ids in the table at
  0x641008..0x641024. Gold is index 6.
  **Two inherited claims found wrong (tenth and eleventh consecutive
  lanes).** `TResourceDisplay::Update` transcribed the DC three-arg
  prototype but retail is `ret 8` — it reads the gate byte at `[ebp+8]`
  and forwards the dword at `[ebp+0xc]`; the DC's `inMap` has no retail
  home. FIXED. And `bVideoPaused` (0x69954c) is contradicted: its 275
  image-wide references cluster on remote.obj's `CChatEdit`,
  `type_AI_player::make_gift`, `combatManager::is_computer_action`,
  `SaveGame` and the advManager turn machinery — it reads as a
  network-game flag, and `Update`'s gate is a multiplayer test.
  **Deliberately NOT renamed** — a 275-reference global belongs to its
  owning lane; the call-site evidence is recorded instead.
  Two retail switches genuinely DISAGREE and are transcribed as the
  bytes read, with the asymmetry flagged in-source:
  `siege_artifact_to_creature` maps artifact 5→0x93 and 6→0x94, while
  the `SiegeMonsterToSiegeArtifact` table inlined into `Update` pairs
  0x93→6 and 0x94→5.
  **Process hazard, fourth occurrence this session:** merging a lane
  resurrects superseded flat-name baseline rows, because git keeps
  master's side of lines the lane correctly deleted under the rename
  rule. The gate catches them as MISSING every time, and each was
  verified to have a mangled successor before removal — but this is a
  structural consequence of blessing by hand-edit, and is the case for
  porting gruntz's `status update --accept-regressions`.
  Also recorded: VC6 accepts an undeclared member-function DEFINITION
  far enough to resolve early members, then silently loses member scope
  for the rest of the body and reports C2065/C2100 at plausible-looking
  later lines. `recruitUnit::Update` was simply missing from the class
  declaration; the error cascade pointed everywhere but there.

- **2026-08-08 — the hero/game include CYCLE BROKEN; `sizeof(hero)` landed
  with a third proof; INCLUDE-SET SENSITIVITY identified as a new residual
  class.** Engine-wide 309 → 312 exact, 3.95% → 3.97% executable matched;
  `town` 11/25 → 13/25.
  **The `initialize_game_data` mystery is SOLVED, and the orchestrator's
  reading of it was WRONG.** The unmasked comparison shows the bytes
  genuinely DIFFER — it was never a symbolization artifact. Inline copy 0 of
  `create_requirement_masks` is byte-identical; copies 1 and 2 are not:
  retail addresses the row directly (`mov [8*eax + gHierarchyMask+0x160],
  ebx`, 7 bytes, twice) where our CL hoists the base into a register first.
  Copy 0 survives only because its row offset is 0. **The trigger is the
  COUNT OF USER-DEFINED TYPE DEFINITIONS visible in the TU** — one unused
  `struct probe_t { int a; };` drops it to 96.0880; a 0..8-struct sweep gives
  100.0 / 96.09 / 96.09 / 26.18 / 97.04 / 94.07 / 100.0 / 100.0 / 96.09,
  **non-monotonic**, i.e. VC6 optimizer state, not a modelling error. Blank
  lines, comments, typedefs, `extern int` and bare forward declarations move
  nothing — **which is exactly why the orchestrator's 200-extern probe
  cleared the wrong hypothesis** and why the "reloc-name-only, therefore
  cosmetic" reading of the masked diff was mistaken. The masked view hid
  this, precisely as the skill warns. **It then recovered to 100.0000 with
  no change to initialize.cpp at all**: breaking the include edge below
  removed game.h/mapcell.h's types from town.h's transitive closure and put
  the TU back on the lucky point. The baseline row is therefore a RAISE, not
  a lowering; the full analysis is retained in `config/match_baseline.tsv`
  because the same sensitivity will move this function again. Recorded as a
  named residual class in the match skill.
  **CYCLE BROKEN** (`include/armygrp.h`): `#include "game.h"` →
  `#include "mapcell.h"` + `#include "struct.h"`. armygrp.h never needed
  game.h itself — `game`/`gpGame` moved to their owner's header the previous
  day, and all that remained was forwarding two small value-type headers.
  `include/game.h` now includes `hero.h`, so `game` can hold the complete
  hero type. Three TUs that had been relying on the transitive include now
  say so themselves (armygrp.cpp, town.cpp, ai_tactical.cpp).
  **`sizeof(hero) == 1170 (0x492)` landed with a THIRD independent proof**
  found while bounding the array: the hero sweep at 0x4be841 is
  `lea edi,[gpGame+0x21620]` … `add edi,0x492 / inc eax / cmp eax,0x9c /
  jb`, so ONE loop pins both the element size and the bound — `hero
  heroes[156]` at `game+0x21620`, byte-proven at both ends
  (`0x21620 + 156*1170 == 0x4ded8`, clearing the 156-dword band at +0x4dfb4
  that 0x4bf2a2 fills with `mov ecx,0x9c` / `rep stosd`, a second witness
  for 156). hero's tail is modelled as `stats[4]` at +0x476 plus a 0x18 pad
  to 0x492 — the Ghidra tail fields are cited as LEADS only, since the
  extent is what is proven, not the split. `game::GetHero` landed as an
  INLINE member of game.h, which is what the DC row says (`E:\gamedcs\
  Game.h:972`) and what retail behaves like: `town::HasGarrison` reaches it
  after its own `garrisonHeroId < 0` gate and STILL emits the redundant
  `cmp edx,-1`, proving the test lives inside the accessor.
  Exact: `town::HasGarrison` (the whole body nests inside
  `if (visitingHeroId < 0) {…}` with a single trailing `return 1`; the flat
  two-statement form makes VC6 normalise with `neg/sbb/neg` AND clone the
  HasCreatures call into both arms — 33.8%) and `town::remove_garrison_hero`
  (an explicit `int player = owner;` right after the first GetHero is what
  makes retail save both esi and edi). `GiveSpells`, `View`, `SwapHeroes`
  are decoded but blocked on surface this lane declined to invent — the
  mage-guild tables and the `std::bitset<70>` at `town+0xd4` (which ends
  exactly at the garrison at +0xe0), `townManager`/`gpTownManager`/
  `gpExecutive` plus four unnamed globals, and `playerData`'s hero list.
  **`type_AI_combat_data +0x20` is `long tactics_advantage`**, verified not
  assumed: `initialize_creatures` seeds it 0, stores `movsx` of `my_hero
  +0xdc` (secondary-skill slot 19 = Tactics), subtracts the enemy hero's
  slot 19, and clamps back to 0 on `jns` — that `jns` is what makes the
  signed `long` byte-proven. DC offset 28 maps to retail 0x20 via this
  class's constant +4 shift after `monsters` (VC6's 16-byte vector vs the
  DC's 12-byte STLport one).
  **OPEN:** `game+0x21610` is NOT padding — the town walk at 0x4be80a does
  `lea ecx,[gpGame+0x21610]` and calls a count-returning method with the
  `towns` pointer at +0x21614 as its data member, so that slot is a small
  container head, not `char pad_21610[4]`.

- **2026-08-07 — armygrp: GetMorale unblocked by the Dinkumware answer;
  the whole TSplitWindow bracket found MISATTRIBUTED.** `armygrp`
  16/34 → 17/34 exact, 26.13% → 31.79% fuzzy; engine-wide 283 → 284
  exact, 3.68% → 3.72% executable matched.
  **`armyGroup::GetMorale` (666 B, previously an unwritten stub whose
  carcass note blamed the STLport question) landed at 87.996%.** The
  four helpers are Dinkumware `<bitset>` members — `_Tidy(unsigned
  long)` 0x44c6e0, `set(size_t,bool)` 0x44c680, `operator[]` 0x4cef80,
  `reference::operator=(bool)` 0x44c610 — with `test()` inlined and
  `_Xran` at 0x434ad0. **Instantiation cost in practice: just
  `#include <bitset>`** — no vendoring, no anchor, no pragma, no
  explicit instantiation. (hero.cpp needed the `inline_depth(0)`
  scaffold only because those COMDATs had no natural call site; where
  real call sites exist, the include suffices.) The guard byte +
  `atexit` pattern proves a FUNCTION-LOCAL static, not file scope — the
  atexit thunk at 0x44a4c0 is literally one byte, `c3`. Semantics
  recovered: hero morale, the alignment census with a grouped collapse
  over {Castle,Rampart,Tower,Stronghold,Fortress}, `morale += 2 -
  numAlignments`, undead −1, Angel/Archangel +1, enemy Bone/Ghost
  Dragon −1, Tavern +1, Castle's Brotherhood of the Sword +2, clamp to
  [-3,3].
  **The TSplitWindow bracket was systematically misattributed — the
  carcass generator's 1:1 order pairing slips by one.** Five
  corrections, each byte-proven, recorded at `src/armygrp.cpp:33-138`:
  0x4496c0 was `UpdateSplitArmy` → is `SplitSliderCallback` (bare `ret`,
  ecx used as an INTEGER, window read from file-scope 0x693878: a `/Gr`
  free function, not a member); 0x449790 was `SplitSliderCallback` →
  is the 3-arg `TSplitWindow` ctor (call site: `new(0x80)` then thiscall
  with (0xb1, 0x14, armies[i])); 0x449df0 was that ctor → is the
  scalar deleting dtor (**`ret 4`**, call-dtor / `test byte[ebp+8],1` /
  `operator delete` / return this — a 3-arg ctor would be `ret 0xc`);
  0x44a180 was `SetRolloverText` → is `WindowHandler` (`ret 4` with a
  `message*`); 0x44a460 was `WindowHandler` → is armygrp.cpp's
  static-set accessor (no args, no `this`, returns `&bitset<9>` at
  0x693884). Size ratios corroborate every pairing at 1.05–1.18× once
  inlining is accounted, against 4.06× for the old SplitSliderCallback
  pairing. `UpdateSplitArmy` and `SetRolloverText` have **no retail
  slot** — inlined at their single call sites then dropped by
  `/OPT:REF` (the HasSomeUndead pattern in this same file) — and moved
  back to `DC_ONLY`. **Ninth consecutive lane to find inherited claims
  wrong.**
  Bonus: 0x44a460 extracted as an extern-linkage helper
  (`ArmyGrpFn_0044A460`, unattested ordinal placeholder — no DC row)
  took GetMorale 79.4 → 88.0 as a side effect, because retail emits
  BOTH the out-of-line body and the inline expansion, and the inline
  expansion is what leaves the bitset members as calls.
  **Rejected and recorded so a later lane is not poisoned:** the
  `_cpp_min`/`_cpp_max` const-reference clamp *is* what retail's three
  homed temps + address-selection means, but it scores strictly worse
  in all four consumers (GetMorale 79.4→75.7, GetLuck 84.8→79.6,
  GetArmyMorale 72.9→45.7) because our compile reuses the stat's memory
  home as the by-ref temp and reloads it. The ternary was kept **with
  an explicit in-source warning that it is NOT evidence about retail's
  source** — the blocker is upstream register homing, not the idiom.
  **OPEN:** `gTavernMask`/`gBrotherhoodOfTheSwordMask` sit in
  `armygrp.h` only because another lane owned `town.h` this session;
  they are `bitNumber[5]` and `bitNumber[22]` and belong beside
  `gFountainOfFortuneMask`.

- **2026-08-07 — hero +0x430 RE-MODELLED; the Dinkumware surface proven
  BY BYTES and 5 STL COMDATs matched; P2.3 closed in practice.**
  Engine-wide 278 → 283 exact, 3.65% → 3.68% executable matched; `hero`
  3/89 (0.58%) → 8/94 (2.79%).
  **The hero re-model landed, and the names are DC-ATTESTED, not
  invented.** `evidence/dreamcast/members.csv` carries
  `hero,969,in_spellbook`, `hero,1039,available_spells`,
  `hero,1109,stats` — the same 70/70 spacing as retail's
  0x3ea / 0x430 / 0x476, and the DC `SpellID` enum ends `kNumSpells,70`.
  So `include/hero.h` now models `in_spellbook[70]` at +0x3ea and
  `available_spells[70]` at +0x430 (`NUM_SPELLS = 70`); `stats[4]` at
  +0x476 is documented but not modelled (no compiled consumer yet).
  **The +0x43e byte is `available_spells[14]` = `eSpellEarthquake`, and
  that is a SEMANTIC confirmation, not merely an index**: a hero who can
  bring the wall down is modelled as taking no wall archery penalty.
  `SPELL_EARTHQUAKE = 0xe` added to `ESpellId`; `ai_combat.cpp` reads
  `my_hero->available_spells[SPELL_EARTHQUAKE]`.
  **`check_wall_archery_penalty` scored 70.65% before AND after** —
  same displacement, same byte width, identical codegen. The correct
  model cost nothing, which is the ideal outcome for a semantic fix.
  **P2.3 is now proven by BYTES, not just by strings.** Five COMDATs
  matched at 100.0000: `bitset<144>::any()` (27 B), `vector<int>::begin`
  and `::end` (4 B each), `vector<int>::push_back(const int&)` (434 B),
  `bitset<70>::set(size_t,bool)` (96 B). **Inherited-identity
  correction: 0x4e6500 is `push_back`, NOT `insert`** — `insert(iterator,
  const T&)` is a 2-param member (`ret 8`); retail is `ret 4` with `_P`
  loaded from `[this+8]` (`_Last` = `end()`) at entry, i.e.
  `push_back(_X) { insert(end(), _X); }` with the 2-arg insert inlined.
  **Arity was the tell for the seventh consecutive lane.** Four further
  vector helpers reproduce with ZERO differing bytes but are NOT claimed
  — link order brackets them ambiguously: `_Destroy` 0x404140,
  `_Ucopy` 0x574ce0, `_Ufill` 0x48d940, `_Construct` 0x404dc0, plus
  `bitset<70>::_Xran` 0x4d1c80.
  **INSTANTIATION CONTRACT (ratify):** no vendoring — `#include <bitset>`
  / `<vector>` straight from the pinned VC6 toolchain (precedent already
  in tree: `src/monframeinfo.cpp` includes `<vector>`). A claim alone
  emits nothing, because an inline member only gets an out-of-line
  COMDAT when a call site declines to inline it and `/Ob2` inlines
  everything; **`#pragma inline_depth(0)` around one scaffold function
  `h3_stl_comdat_anchor`** is the smallest construct reproducing
  retail's emission decision, and it deletes itself when hero.cpp's real
  bodies land. Rejected and documented in-file: member-pointer
  address-taking (adds `.CRT$XCU` dynamic initializers),
  `template class std::vector<int>;` (~70 unrelated COMDATs),
  `#define private public` (wrong mangling). **Measured**: extra
  base-side symbols are INERT — objdiff enumerates the TARGET object's
  functions, so the anchor and the ~25 COMDATs it drags in add no report
  rows and no ratchet rows.
  **PIPELINE CHANGE (ratify):** `scripts/homm3/build/labels.py` +24
  lines — `_demangle_key` could not join a class-template member
  (`?push_back@?$vector@…` keyed to garbage) and the declarator scan
  dropped everything before `<int>`; both sides now normalize template
  argument lists away, symmetrically. **Verified zero impact**:
  `build/gen/symbol_names.csv` byte-identical before/after on the
  pre-change tree.
  **`type_AI_combat_data`'s vector head now has BYTE PROOF** (recorded,
  not acted on — it moves the head of a class several exact bodies
  depend on): the copy ctor 0x4276c0 opens `mov al,[esi] / mov [edi],al`
  — a ONE-BYTE copy at +0 before any pointer — then `size()` from
  `[esi+4]/[esi+8]`, `if (_N<0) _N=0`, `operator new(_N*72)`. That byte
  is Dinkumware's empty `allocator` subobject, which STLport's vector
  does not have. So the vector starts at +0x00 (16 bytes), NOT 12 bytes
  at +0x04, and the current `field_00` IS the subobject.
  **ORCHESTRATOR'S "STLport-blocked" LIST WAS MOSTLY WRONG — corrected
  in-file so no future lane re-parks on it.** Genuinely unblocked:
  `armyGroup::GetMorale` 0x44ae60 (needs a file-scope `std::bitset<9>`
  holding {1,2,7,6} — exactly the proven surface; the rest of the body
  is already transcribed) and the `_cpp_min`/`_cpp_max` file-local
  copies in ai_combat/ai_tactical (real `<xutility>` takes both params
  as `const _Ty&`, the local copies take them by value). NEVER actually
  STL-blocked: `armyGroup::Merge` (retail's fused four-array
  pointer-difference copy loop), `ai_combat::choose_melee`,
  `ai_tactical::get_berserk_value`/`get_area_effect_value` (all
  EH-bearing, P2.2), `ai_player::get_total_value` (a bare stub).
  `ai_combat.cpp:1102`'s DC rows are the DREAMCAST's STLport
  instantiations and are DC_ONLY by construction — retail links
  Dinkumware, so those symbols have no retail counterpart at all.
  **OPEN:** `bitset<48>::_Tidy` 0x4e66c0 is reproduced byte-exact (all
  40 B) but `_Tidy` is private — the only emitting constructs are
  `#define private public` (yields `QAE` where retail is `AAE`, a
  structurally wrong spelling) or `template class std::bitset<48>;`
  (drags ~70 COMDATs AND emits a second `bitset<48>::set` colliding with
  the bitset<70> claim's join key). Left unclaimed. And 0x4e6750, the
  3-arg `/Gr` clamp `(*b<*a) ? a : ((*c<*b) ? c : b)` whose only caller
  is `hero::GetLuck` (luck clamps to [-3,3]), stays unclaimed: it is NOT
  `_Median` (both Dinkumware's and STLport's take three VALUES and spend
  three comparisons; this takes three pointers and spends two) and no
  evidence names it.

- **2026-08-07 — hero.obj tail audit: 7 claims WITHDRAWN as STL COMDATs;
  P2.3 ANSWERED (retail is Dinkumware, NOT STLport); the hero
  `0x430+spell` contradiction RESOLVED.** Engine-wide 277 → 278 exact,
  3.59% → 3.65% executable matched; inputmgr 31.5% → 80.15%, 2/10 →
  5/10.
  **P2.3 — THE STLPORT VENDORING QUESTION IS ANSWERED, and the answer
  unblocks rather than blocks.** The DC dump's hero.obj tail is STLport
  (`..\stlport\stl_bitset.h`); **retail's is Dinkumware — VC6's own
  shipped headers.** Retail carries `"invalid bitset<N> position"`
  (0x0065f450) and `"invalid string position"` and **no STLport string
  anywhere**. Verified independently by the orchestrator against the
  hash-checked image: `invalid bitset` ×2, `invalid string position`
  ×1, and ZERO hits for `stlport`/`STLport`/`_STL`/`__stl`. Consequence:
  every body parked on "STLport-blocked" is reproducible with the
  toolchain already in the tree, and the seven COMDATs below are real
  unclaimed surface (the `0x4e6500` `vector::insert` body alone serves
  **276 call sites**).
  **The inherited tail map had walked seven DC header/compgen rows onto
  seven retail COMDATs that are a different set entirely.** All
  WITHDRAWN, each identified by body: 0x4e64c0 `bitset<144>::any()`
  (`mov eax,4` = `_Nw`, Dinkumware's descending loop over `_A[5]`; N
  pinned by `bitset<144>::set` at 0x4cf9a0's `cmp edi,0x90`); 0x4e64e0
  `vector::begin()` and 0x4e64f0 `vector::end()` (bare `ret` vs the DC
  rows' `ret 4`); 0x4e6500 `vector::insert` (growth
  `_N = size() + (size()<_M ? _M : size())`, `_Allocate` verbatim,
  per-element callee 0x404dc0 = `_Construct`'s placement-new null
  check) — claimed as `SCampaign::GetExpCap`, a 32 B `ret 0` getter;
  0x4e66c0 `bitset<48>::_Tidy` (`_Trim` masks `_A[1] &= 0xFFFF` ⇒
  `_N%32==16`); 0x4e66f0 `bitset<70>::set(size_t,bool)`
  (`cmp edi,0x46`, returns `this`, `ret 8`; 70 = the spell count);
  0x4e6750 a 3-arg `/Gr` clamp helper `(*b<*a) ? a : ((*c<*b) ? c : b)`
  with no delete, no vcall, no vtable load — left deliberately UNNAMED.
  Also: 0x4e6120 CORRECTED (slot right, arity wrong — DC 1-param `ret 0`
  vs retail `ret 8`; a 2-arg member adding artifact attack/defense/HP to
  a stat block, caller 0x43d7b2 in army.obj) → ordinal placeholder
  `hero::HeroFn_004E6120`; 0x4e5b80 CONFIRMED with signature corrected
  to `ret 4`; 0x4e5ce0 and 0x4e5dd0 PROMOTED from `DC_ONLY` to
  `hero::can_land()` and `hero::WalkOnWater(int)` (the latter proven by
  `Fly(int)` storing to +0x112 and `IsMobile` loading +0x116 and +0x112
  together as the movement-override pair); one NEW retail-only claim at
  0x4e5de0. **18 further tail claims body-verified CONFIRMED.**
  Coverage: every claim from 0x4e4990 to end-of-TU is body-audited, and
  all 89 claims in the file were screened by a new `ret N` vs
  DC-param-count validator (recursive-descent, so jump tables do not
  desync it) plus a size-ratio sweep. Unaudited: 0x4d7470..0x4db350
  screened but not body-verified; the 42 unclaimed functions in
  0x4db3d6..0x4e2340 untouched.
  **THE `hero[0x430 + spell]` CONTRADICTION IS RESOLVED — and the tree's
  current model is the wrong side of it.** `hero::AddSpell(int)` at
  0x4d9330 is 26 bytes and writes TWO per-spell byte arrays, stride 1,
  bases exactly **0x46 = 70 apart**: `[eax+ecx+0x3ea] = 1` and
  `[eax+ecx+0x430] = 1`. Corroboration: `can_summon_boat` guards on
  `byte [this+0x430]` then handles spell 0 (Summon Boat); a whole-image
  scan of byte displacements in 0x3ea..0x47f finds scattered 1–2-ref
  named-spell reads (+0x3ea, +0x3f4, +0x404, +0x420, +0x430,
  +0x436..+0x439, +0x43e, +0x453, +0x455, +0x461) and then **jumps to
  20–28 references at +0x476..+0x479**, the four primary skills
  (byte-proven by `get_primary_skill_total` and 0x4e6120) — and
  `0x430 + 70 = 0x476` exactly. +0x436..+0x439 = spells 6,7,8,9 =
  Fly / Water Walk / Dimension Door / Town Portal, all read from one
  adventure-map TU. **So `noWallPenalty` at +0x43e is the WRONG reading:
  it is element 14 of the second spell array, and
  `check_wall_archery_penalty` (0x42482b) really reads
  `hero->spellFlags2[14]`.** Per the orchestrator's standing
  instruction the matcher did NOT re-model unilaterally — `include/hero.h:66`
  and `src/ai_combat.cpp:915` still carry the old reading. **The
  re-model is queued as its own change.**
  **inputmgr:** `MakeScanCodeTable` exact (the last 0.57% was `jl` vs
  `jb` — the loop counter is **unsigned**; retail also re-states all 89
  entries after the `index << 8` pre-fill, including 21 redundant with
  it); `MouseMessageHandler` 99.9645% with only reloc-NAME-only rows
  left (the labels layer names DATA rows `data_<rva>` project-wide).
  **New lever:** declaring `int quals = 0;` INSIDE the `if` block rather
  than at function scope took it 86.5% → 99.96% — at function scope VC6
  homes it to `[ebp-4]` and that one spill cascades through the whole
  tail. Case ORDER is also load-bearing: MOUSEMOVE, LBUTTONDBLCLK,
  LBUTTONDOWN, RBUTTONDOWN, RBUTTONDBLCLK, LBUTTONUP, RBUTTONUP puts
  the two `ReleaseCapture()` arms adjacent so VC6 tail-merges them as
  retail does. `KeyboardMessageHandler` is fully decoded and the decode
  written into its carcass, but finishing it would mean inventing
  structure for two unmodeled globals — deliberately left a stub.
  **OPEN:** adopting `hero::GetRoguePower` for 0x4e5de0 (address now
  independently proven, but the NAME is HD-crossbuild/NH3API lineage and
  needs supervised admission; currently the placeholder
  `hero::HeroFn_004E5DE0`); and six head-region arity divergences in
  hero.cpp (`HeroMessageUpdate`, `HeroScreenUpdate`, `UpdateArmies`,
  `ViewStat`, `ViewArtifact` — the hero-SCREEN block the Dreamcast
  redesigned — plus `hero::load`), flagged in-file, not acted on.

- **2026-08-07 — the va-claims backlog DRAINED (9/9 were bugs, not
  debt); winmgr CLOSED; the try/catch lever proven twice more.**
  Engine-wide 273 → 277 exact, 3.45% → 3.59% executable matched.
  **Part 1 — every one of the nine remaining CLASS rows in
  `config/va-claims-baseline.tsv` was a real claim misattribution.
  Zero were legitimately-filed debt.** The backlog is now ONE row (the
  kbwin `GameTime::Get` entry, correctly filed: a real function whose
  whole body is `jmp [__imp__timeGetTime]`, which the universe
  classifier can only read as an import thunk). Two shapes, both the
  excluded class the skill names verbatim: 0x4e6d60 and 0x4e6780 are
  32 B guard-byte/atexit prologues (`mov cl,[0x6abaa0]; mov al,1; test
  al,cl; jne+8; or cl,al; …; call _atexit`); 0x4b4420 and herodefs'
  six are members of enumerated ten-block runs — the identical
  `bitset<10>` loop differing only in the trailing
  `[bss] = (bits & MASK) << N`, with MASK/N stepping 0x3ff/0, 0x1ff/1,
  0xff/2, 0x7f/3, 0x3f/4, 0x1f/5, 0xf/6, 7/7, 3/8, 1/9 — ten per TU,
  the `terrain.h:70..79` `$E4xx` statics, corroborated by inputmgr.obj's
  DC roster printing the run by name. Size plausibility independently
  condemns six (ratios 0.12, 0.23, 0.235, 3.96, 4.0, 23.75, 23.75 —
  all outside the 0.3–2.5× band). Where the displaced functions went:
  the three `Initialize*Traits` are DC `static` with one call site each,
  so /Ob2 ate them (DC caller+callee 382/536/556 vs retail Table sizes
  374/482/456 = 0.98/0.90/0.82, one tight cluster, where each Table
  alone would be 3.0–4.2×); the four `TAutoStrPtr` methods are
  8/24/4/4-byte anonymous-namespace accessors, inlined away.
  `army::ValidFlight` (DC 228 B) has no located slot, so fly.obj is now
  unattributed — deliberately NOT re-anchored on guesswork.
  **Part 2 — `winmgr` is CLOSED, 6/6 at 100.00%.** `FadeScreen`
  99.87 → 100% (retail's `je` lands ON `isWaitingForFadeIn = 0`, which
  is outside the guard); `DoDialog` exact on the FIRST build at the
  merged 640 B, four try levels read straight off the funclets, with
  the homm2 buka twin supplying statement order and the DC-confirmed
  names `gbInDialog`/`gbSendMouseMoveMessages`; `DoDialogDraw` exact at
  666 B once the handler's verdict was spelled as a **two-case
  `switch`**, not an if/else chain (the chain falls into the
  `WIDGET_END_DIALOG` arm; retail sinks it past the redraw arm behind a
  forward `je`). **Second and third carve boundary corrections** to
  `config/retail-functions.tsv`, same self-validating standard:
  0x202520 533 → 640 and 0x2027a0 559 → 666, deleting eight split
  funclet rows (11,940 → 11,932). Extra corroboration beyond the
  byte-exact compiles: 533+25+31+19+32 = 640 lands exactly on
  DoDialogDraw's entry with no padding. Value 0x20 in the widget domain
  has no DC name and was added as `WIDGET_RETURN_32`, an explicit
  **ordinal placeholder flagged as unattested** rather than an invented
  semantic name.
  **Part 3:** `font::DrawBoundedString` 0 → 96.33% (unit 53.3 → 90.4%),
  `inputManager::ForceMouseMove` 0 → 100% first build,
  `inputManager::AsciiConvert` 0 → 95.13% (unit 15.2 → 31.5%).
  **New lever: naming the scanned character costs 10 points** —
  `char c = str[pos]` gets a stack home and a reload at every use;
  writing `str[pos]` at each use keeps it in `al` like retail (86.9 →
  96.3 from that change alone). Layout correction proven here:
  `inputManager::scanCodeTable` is **128 shorts, not 256 bytes**
  (`[ecx + 2*codeX + 0x848]`, `movsx word` for the F-key band).
  **OPEN — hero.cpp's tail order-map looks unsound beyond the withdrawn
  row** and wants an audit lane: 0x4e6500 (434 B) is claimed as
  `SCampaign::GetExpCap` (DC 32 B, ratio 13.6×); 0x4e66f0 (96 B) is
  claimed as `type_artifact::'default constructor closure'` (DC 24 B)
  but its body is `bitset<70>::set(pos,bool)` returning `this` with
  `ret 8`; 0x4e6750 is claimed as a scalar deleting dtor but contains
  no delete and no vcall. All three left alone.

- **2026-08-07 — small-TU sweep: sample CLOSED, the `_noeh` profile
  RETIRED, and the va-claims backlog found to be MASKING real
  misattributions.** Engine-wide 268 → 273 exact, 3.41% → 3.45%
  executable matched. Exact this lane: `misc::WritePrefs` (a real tail
  `jmp`, not an excluded class), `sample::sample`, `winmgr::DoQuickView`
  (unit 46.9% → 99.98%), `findpath::get_travel_time`,
  `mousemgr::Reset`. **sample is CLOSED 3/3, 100.00% fuzzy.**
  **Governance finding — the known-backlog was hiding bugs, not
  recording debt.** `exec` 0x4b0660 and `findpath` 0x4b1090 were
  claimed as `executive::executive` and `type_point::is_valid`. They are
  **byte-identical 95-byte bodies** differing only in the `.bss` slot
  stored; that body occurs **900 times image-wide, exactly ten per TU**,
  and the DC roster names the run: ten `$E4xx` *static* rows from
  `E:\gamedcs\terrain.h:70..79` — per-TU file-scope dynamic
  initialisers, i.e. the skill's cinit excluded class. Both claims are
  WITHDRAWN. Critically, `verify_va_claims` had been flagging both
  correctly as "init-thunk code" and both had been parked in
  `config/va-claims-baseline.tsv` as accepted standing debt. They were
  not debt. **Parking a gate violation in a baseline masked a genuine
  misattribution for weeks — and eight more entries of the same class
  remain** (fly.cpp 0x4b4420, hero.cpp 0x4e6780, herodefs.cpp ×6), all
  sitting inside enumerated cinit runs. Draining them is queued.
  **Consequence: the "exec is on the STLport wall" note was WRONG** —
  the `std::bitset<10>` body was this cinit block, never
  `executive::executive`, whose real DC size is 14 bytes (6.8× smaller
  than claimed, outside the SH4→x86 band) and which `DoDialog` inlines.
  exec has NO unwritten claimed surface; its residual is CallManager
  68.4% / MainLoop 88.1%.
  **PROFILE DECISION — `game_o2_ml_gr_windows_noeh` is RETIRED, closing
  the P2.2 profile re-decision.** The "first no-EH TU" inference was
  wrong: sample.obj's frameless `~sample` does not exclude `/GX`, it
  only proves `operator delete` was visible as **nothrow** — the
  ai_combat lever, applied to a base-subobject unwind instead of a
  scope-exit one. `sample::sample` (0x566da0) carries a full `fs:[0]`
  frame no non-`/GX` compile can emit. With `/GX` + the nothrow
  redeclaration all three sample functions are byte-exact. **The engine
  now has ONE C++ profile.**
  **New lever:** `try { … } catch (...) { <cleanup>; throw; }` is
  retail's RAII idiom. The carve leaves 25/31/19/22-byte blocks after
  DoDialog/DoDialogDraw/DoQuickView, every one ending in
  `_CxxThrowException(0,0)` — a *rethrow*, which no destructor funclet
  emits. Three nested try/catch levels made DoQuickView exact on the
  second compile. **78 carve rows image-wide end in that rethrow tail**;
  this unlocks DoDialog (4 levels) and DoDialogDraw (4 levels).
  Corollary: **EH-bearing ≠ blocked** — two `/GX` functions with `fs:[0]`
  frames went exact here; the unwind data lives in dropped sections, so
  only the `[ebp-4]` state stores must be reproduced.
  **CARVE BOUNDARY CORRECTION admitted to `config/retail-functions.tsv`**
  (hand-admitted inventory, so recorded explicitly): 0x202a40 is **392 B**
  (0x602a40..0x602bc8), not 314 — the three EH funclets the carve split
  off are local labels inside one symbol. Proof is self-validating: the
  VC6 compile of the reconstructed body emits the same three blocks
  inside one symbol byte-for-byte, and the function is exact at the
  merged size. 11,943 → 11,940 rows. The same correction is pending for
  the DoDialog/DoDialogDraw funclets (left alone — minimal diff).
  **Other surface:** `pathCell` modelled, stride byte-proven at 30 with
  `#pragma pack(1)` (default packing rounds `sizeof` to 32); `cellData`
  void* → pathCell*; `searchArray::result` deliberately NOT touched.
  `SPointerSprite` retired as a duplicate bootstrap view of `CSprite`
  (s@0x1c, numSequences@0x28, validSeqMask@0x2c match, and SetPointer
  calls vslot 1 = CSprite::Dispose then refills via
  ResourceManager::GetSprite). Provisional offset-anchored names added
  per the `gUnnamed6a5d5c` precedent: `SUnnamed69d808` /
  `CUnnamed69d808_f0` / `get_field_f0` (winmgr), `gMouseSetPointerBusy`
  (0x69ca21), `gPointerSetSprites` (0x67ff38, DATA + DATA_COMPGEN),
  `RESOURCE_TYPE_SFX = 32`. Residuals documented in-source:
  mousemgr::SetPointer 82.9% (constant-CSE family; the `sub/neg/sbb/and`
  mask proved the polarity is INVERTED — retail forces frame 0 for the
  animated SPELL set), mousemgr TCSLock 0% / CheckUpdate 96.85% (/Ob2
  over-inline asymmetry, unfixable by definition placement).
  **Not attempted, with reasons:** recruit::Update (recruitUnit has no
  class layout at all), findpath SeedCombatPosition/FindCombatPath (need
  the full pathCell grid plus ~12 unclaimed callees),
  font::DrawBoundedString and inputmgr ×5 (real targets, out of budget).

- **2026-08-07 — ai_player.obj ADMITTED (user: "approve"); a swapped
  claim pair corrected; a duplicate `class game` merged.** The AI phase
  closes: `src/ai_player.cpp` (503 functions in link order, 26
  `$`-thunks omitted) joins `config/units.toml` under
  `game_o2_ml_gr_windows`, 492 un-reconstructed functions fenced in
  `#if 0 // @carcass`. Engine-wide 263 → 268 exact, 48,667 → 49,290
  matched bytes, 3.36% → 3.41% executable matched; new `ai_player` unit
  5/11 exact, 16.55% fuzzy. Denominator 682 → 693, **+11 exactly the
  claim count — the cheap-admission rule from the ai.obj entry held.**
  (The engine-wide *fuzzy* figure dips 32.69% → 32.24% purely from the
  larger denominator; matched bytes only rose, and a re-run of
  `homm3 status` against the pre-change snapshot shows every other unit
  byte-identical, so the shared-header surgery below is codegen-neutral
  everywhere else.) **Claim correction:** 0x004297c0 and 0x00429910 were
  SWAPPED — the order-map had placed them in DC source order
  (find_magus_hut_value :664 before start_turn :695) but retail emits
  them reversed. Proofs: 0x4297c0 takes a `this` whose +0 is a short,
  walks the player record at `gpGame+0x20ad0`, and CALLS 0x429910;
  0x429910 touches no `this`, takes `ecx`=long / `dl`=bool, sweeps map
  cells for object type 0x1b; and the unambiguous
  `reset_magus_hut_value` also calls 0x429910. Size ratios corroborate
  (1.06 / 1.15 corrected vs 0.86 / 1.42 swapped), and
  find_magus_hut_value then came out byte-exact in the 405-byte slot.
  **Tree defect fixed:** `include/armygrp.h` carried a SECOND,
  conflicting `class game` (pad to 0x1f698 + `AI_in_control`) alongside
  `include/game.h`'s (pad to 0x1fb70 + `worldMap`); they had never met
  until this TU needed both. Merged into game.h at proven offsets, with
  the `DATA(0x006994e8) extern game* gpGame;` claim. **Gate gap worth
  acting on: the single-view gate scans only `.cpp` files, so a
  header-vs-header duplicate class slipped past it for weeks.**
  **New byte-proven lever:** widening a byte through a `short` temporary
  before assigning into a `short:N` bitfield — assigning the
  `unsigned char` directly lets VC6 narrow the insert to 8-bit ops
  (`mov dl`/`and dl,0xf`/`and ch,0xc0`) and split the two field stores,
  while the short temporary reproduces retail's `movzx ax, byte` loads
  and the single `and edx,0xffffc000` clearing both fields at once
  (worth 12 points on can_take_town, which stalls at 98.75% on one
  scheduling slot; an aggregate initialiser reached 89.3% but is
  STRUCTURALLY WRONG — retail read-modify-writes both storage units —
  and was deliberately not recorded, since ratcheting it would have
  poisoned the baseline against the correct spelling).
  **New surface, PENDING USER RATIFICATION:** `include/struct.h` is a
  new file holding only `type_point` (`short x:10; short y:10;
  short z:4;`), DC-attributed and layout-proven by three independent
  readers — no existing compiland header owned it. And
  `TAdventureObjectType` is **163 enumerators transcribed wholesale
  from the DC enum** into `mapcell.h` with only ONE value retail-proven
  (0x1b = EYE_OF_MAGI, what a Hut of Magi reveals); names are
  unprefixed as the DC spells them (`HERO`, `EVENT`, `RESOURCE`,
  `MONSTER`…) and now reach every TU including game.h via armygrp.h.
  Zero collisions in the tree today, but a future TU pulling in
  `windows.h` is a live risk — **prefixing is offered and unresolved.**
  `NewmapCell` moved to mapcell.h, `#pragma pack(1)` size 38 (a 38-byte
  record cannot be 4-aligned, and only packing puts the 4-byte type
  field at the odd-dword 0x1e retail reads); `is_trigger` bit 12 is
  provisional. findpath.h `danger_zones` void* → long*; town.h
  `pad_05[3]` → DC-named mapX/mapY/mapZ. **Blocked, decoded, not
  invented:** get_total_value is STLport-blocked (P2.3, local
  `std::vector<long>`); start_turn/end_turn/make_gift/calculate_demand
  need a real `playerData` model (retail's differs from the DC's:
  numHeroes@+1, currHeroId@+4 int, int heroes[]@+8, numTowns@+0x3e,
  currTownId@+0x3f, char towns[]@+0x40), the hero array embedded at
  `gpGame+0x21620` stride 1170 and the town array POINTER at
  `gpGame+0x21614` stride 360 (360 matches our current `town` model
  exactly — independent confirmation of it), and **two net-message
  classes whose names would have to be fabricated** — 8-byte
  `{vptr; long team}` objects with vtables 0x63b670/0x63b67c referenced
  ONLY by start_turn and end_turn, no corroborating call site, no DC
  public. The matcher stopped rather than invent; that is the correct
  call and the precedent stands.

- **2026-08-07 — ai.obj ADMITTED to the build (user: "approve"); a
  misattributed claim corrected; the ratchet writer's comment-eating
  bug fixed.** `src/ai.cpp` (219 functions in link order, 20 `$`-thunks
  omitted) joins `config/units.toml` under `game_o2_ml_gr_windows`; its
  212 un-reconstructed functions sit inside 7 `#if 0 // @carcass`
  fences, lexically intact with `DC_ONLY`/`VA` annotations and
  `E:\gamedcs\` provenance. Engine-wide 259 → 263 exact, 3.32% → 3.36%
  executable matched; new `ai` unit 4/7 exact, 41.13% fuzzy.
  **Scoreboard doctrine learned — the orchestrator's prediction was
  WRONG:** admitting a carcass TU was expected to add ~219 functions to
  the "linked units" denominator and depress the headline %. It added
  **7**. The delinker only materialises *claimed* addresses into a
  unit's target object, so a carcass TU costs the scoreboard nothing
  for its unclaimed rows — 675 → 682, and the percentage ROSE.
  Admitting the remaining carcass TUs is therefore cheap, not costly.
  **Claim correction (the substantive find):** `0x0041f380` was claimed
  as `get_attack_value`, a four-argument `/Gr` free function — which
  must emit `ret 8`, but the slot is a 39-byte body reading only ECX
  and ending in a bare `ret`, at a 0.10 DC size ratio. It is
  `army::IsIncapacitated` (DC `Army.h:840`): its one caller is
  `find_move_order` at 0x41f1ee in a can-this-stack-act gate chain; the
  identical `disabled_290 || disabled_2b0 || disabled_2c0` test appears
  inlined at 23 further sites across ai/ai_tactical/army (a header
  inline whose duplicate COMDATs `/OPT:ICF` folded); and the caller
  tests AL while the callee materialises full EAX — an `unsigned char`
  return over an int `||` chain. Claim moved, `get_attack_value`
  returned to `DC_ONLY(0x248b4, 0x180)`. Also: `hero::IsWieldingArtifact`
  returns a BYTE, not `int` — 212 of its 224 retail call sites follow
  the call with `test al, al`; correcting it took `can_cast_spells` to
  exact and *raised* three armygrp rows (GetArmyMorale, GetLuck,
  get_spell_work_chance). **Tooling:** the ratchet writer destroyed
  every inline `#` comment on each add/raise rewrite (invisible on
  "0 added, 0 raised" runs, which is why it hid) — fixed with a
  negative control; see the separate commit. **OPEN:** ai.obj's min
  helper compares its FIRST parameter against its second, the opposite
  orientation to `_cpp_min`/`std::min` (`_Y < _X`); shipped file-local
  as `min_ref` rather than a second differently-bodied `_cpp_min`,
  since the DC roster attests a hand-written `double min(double,
  double)` in NWC's own `includes.h:117`. **Whether to unify these is a
  user decision.** Left carcassed with decoded notes: `find_move_order`
  (blocked on P2.2 *and* P2.3 at once — an fs:[0] frame around a
  `std::vector<army*>` fed to `std::sort`), `move_toward` (needs a
  `pathCell` model — 30-byte stride, direction in bits 12..15, six-bit
  blocked mask in bits 26..31 — and proves `searchArray::result`'s
  element type is `pathCell*`, not the admitted `std::vector<int>`
  placeholder; pinning that is a findpath.h contract change touching
  every consumer, deferred while P2.3 is open).

- **2026-08-07 — ai_tactical stub campaign; the spellcaster family shape,
  and a WRONG "EH-blocked" carcass note corrected.** Same orchestrated
  single-matcher lane. TU 17/50 → 21/50 exact, 23.72% → 37.17% fuzzy;
  engine-wide 255 → 259 exact, 3.21% → 3.32% executable matched. Four
  driven to exact (get_dispel_value, get_backlash_value,
  get_antimagic_value, get_defense_skill_value), five more landed as
  scoring reconstructions (get_attack_skill_value 99.85,
  get_cure_value 95.49, get_damage_value 93.21, should_attack_now
  85.58, get_hypnotize_value 18.23 — semantics complete, codegen not).
  **The family shape:** every `type_AI_spellcaster::get_<spell>_value`
  answers "what would this stack be worth if the spell had landed" by
  making a REAL `army` copy on the stack (`sub esp,0x548`), mutating
  the copy, and re-pricing it through the ordinary combat-value leaves.
  **The `~army()` in that pair is what forces the `/GX` frame** — so
  the nine carcass notes reading `// EH-bearing (P2.2): blocked until
  the synth-PDB EH scope lands` were WRONG; nothing was blocked.
  Declaring `army(const army&)` / `~army()` in `include/army.h`
  (declared, never defined — both stay unresolved externals) is the
  entire unlock. Corollaries for the next matcher: `value += f(...)`
  vs `return value + f(...)` decides whether retail's accumulate
  register survives (66.7 → 88.7 → exact on get_antimagic_value); a
  dead copy is real (get_backlash_value builds `test_army`, never
  reads it, destroys it — retail keeps it); `_cpp_min`/`_cpp_max`
  homes `_Y` FIRST (right-to-left arg evaluation), which fixes
  argument order; `unsigned char f = (unsigned)x >> N;` then
  `if (f & 1)` is required for retail's `shr`/`test cl` (the direct
  `((unsigned)x >> N) & 1` folds to `test dword, imm`); a byte local
  passed as an `unsigned char` parameter is pushed as a full dword
  with GARBAGE upper bytes, legal because the callee reads only the
  low byte. **Negative result worth keeping:** the nothrow `operator
  delete` lever from ai_combat does NOT apply here — ai_tactical has
  no scope-exit `delete`, its frames come from `~army()`, and retail
  DOES emit the `mov [ebp-4], 0/-1` updates around those.
  **Resolves open question (c) of the ai_combat entry:** the
  `mov al, byte [ebp+0xb]` → `[esi]` in the 6-arg type_AI_combat_data
  ctor is VC6 copy-constructing an EMPTY `std::allocator` (one byte
  read from uninitialised stack), immediately followed by
  `[esi+4]/[esi+8]/[esi+0xc] = 0` — the STLport vector head layout
  (`_M_start`/`_M_finish`/`_M_end_of_storage`). The identical shape
  appears in ai_tactical's get_area_effect_value (0x437040) where the
  local is provably a `std::vector<army*>`. So type_AI_combat_data's
  first member is a vector and there was never dishonest C++ to find —
  it is the P2.3 STLport decision, which now also gates
  get_berserk_value, get_area_effect_value and the two consider_*
  functions. **STILL OPEN:** (a) the hero layout contradiction now
  also blocks get_protection_value (700 B, the largest tractable
  stub), a direct consumer of `hero[0x430 + spell]`; and
  `akHypnotizeTurns[4] = {1,1,2,3}` at 0x660858 is declared extern
  only — the owning TU is unproven, so no DATA claim was made.
  New surface (byte-derived, provisional names flagged in-source):
  army copy-ctor/dtor decls, attackSkill +0xc8, defenseSkill +0xcc,
  field_c4, field_190, get_estimated_damage (0x443e30, unnamed in
  DC/HD/IDA); combatManager actingSide +0x132b8, actingSlot +0x132bc,
  ModifySpellDamage / SpellCastWorkChance (DC names, addresses from
  the ai_tactical call sites); hexcell field_4a; searchArray
  SeedCombatPosition; type_AI_spellcaster field_14 and
  worst_enemies[20] at +0x2d0.

- **2026-08-07 — ai_combat stub campaign; the nothrow `operator delete`
  lever.** Orchestrated single-matcher lane (user: "become an
  orchestrator and manage one matcher", AI TUs first then smallest-TUs
  upward; "approve"). Scope was chosen by splitting the TU's 24
  non-exact functions into 14 claimed-but-`@stub` bodies at 0.00% and
  10 already-documented residuals — the matcher was pointed at the
  stubs smallest-first and explicitly told NOT to grind the residual
  family (skill rule: stop after 3–4 real hypotheses). Six promoted to
  byte-exact (create_skeletons, both cast_enchantment overloads,
  get_damage_spell_value, AI_quick_combat, AI_auto_combat);
  get_enchantment_value 0 → 89.46%, homing residual documented
  in-source. TU 13/37 → 19/37 exact, 32.45% → 53.37% fuzzy;
  engine-wide 249 → 255 exact, 3.05% → 3.21% executable matched.
  **New proven lever, expected to generalize to every EH-bearing TU:**
  under `/GX` VC6 keeps the EH state variable live across every call a
  scope-exit dtor makes; retail emits none because its `operator
  delete` was visible as nothrow and VC6's `<new>` declares it WITHOUT
  an exception specification — a single `__declspec(nothrow) void
  __cdecl operator delete(void*);` in the TU took two entry points from
  96.5% / 97.8% to exact. Reconstruction facts: create_skeletons' edx
  arg is an armyGroup walked slot-by-slot (DC's `short amount` is
  wrong); cast_enchantment's per-creature local is `__int64`, proven by
  retail spilling the `__alldiv` HIGH dword to a dead slot; the 180B
  cast_enchantment overload sat at 0.00% only because the labels
  overload-group dedup needs both claims compiled. Process note: the
  matcher's baseline diff had dropped the dated hand-lowering comment
  on get_spell_work_chance — restored, and a build confirmed inline
  comments DO survive the tool's rewrite, so the skill's dated-comment
  convention is sound. **OPEN, escalated not guessed:** (a) hero layout
  contradiction — inlined do_eagle_eye walks `hero[0x430 + spell]` for
  spells 0..0x45, overlapping the byte-proven `noWallPenalty` at
  +0x43e; blocks do_aftermath. (b) choose_melee's copy shape implies
  `type_AI_combat_data` has a BASE CLASS holding +0x10..+0x33
  (adjacent to the STLport P2.3 call). (c) the 6-arg ctor at 0x423ee0
  stores byte 3 of the `new_hero` pointer into field_00 — read is real
  against three call sites, no honest C++ spelling found.
  **[(c) SUPERSEDED the same day — see the ai_tactical entry below:
  it is not a pointer byte at all but an empty `std::allocator`
  copy-construct, i.e. the STLport vector head. (a) remains open.]**

- **2026-08-07 — cleanliness debt cleared to zero (user: "Fix everything
  now"); two masked fatal gates fixed.** Discovery: `homm3 build` had
  been exiting 1 since the kbwin session on two FATAL verify_va_claims
  violations, masked by exit-code checks that piped through `tail`
  (reading the pipe's exit) while stderr buffering displaced the gate
  lines — process rule adopted: read build verdicts UNPIPED
  (`homm3 build; echo $?`), now in the match skill. Fixes: kbwin's
  `GameTime::Get` (0x4f82e0, a real kbwin.cpp function whose whole body
  is `jmp [__imp__timeGetTime]`) recorded as CLASS backlog in
  `config/va-claims-baseline.tsv`; window.cpp claim order corrected
  (delete_widgets before SleepAllWidgets). Cleanliness campaign (offered
  "bless as debt" vs "fix now"; user chose fix now, floors stay 0): all
  ~230 sites cleared — C-style casts 77→0, magic case labels 74→0,
  unnamed domain compares 46→0, cpp extern decls 26→0, .cpp-local views
  9→0 — codegen-neutral (ratchet: 0 added, 0 raised; kbwin bit-identical
  11/12, winfile 14/14). New surface: domain enums across armygrp
  (TCreatureType growth, ESpellId, EMagicTerrain, EArtifactId), town,
  message, inputmgr, winmgr, csprite, mousemgr (DC-verbatim EPointerSet),
  army/smackmgr/font/strip (provisional, so marked);
  `include/winmm_thunks.h` holds the one plain `timeGetTime` decl with
  the per-TU import-form doctrine (included only by button/misc/mousemgr,
  after their windows.h includes; kbwin keeps the dllimport IAT form).
  First fully-green gated build since the debt accrued: README
  match-score block refreshed (1.80% → 3.05% executable matched,
  249/4,760 exact, game 36 units 180/606).

- **2026-08-07 — ai_tactical + ai_combat lanes merged; /Op proven and
  sweep-tested.** Two parallel Opus matcher lanes (worktree pool,
  serial integration). ai_combat: 13 exact of 22 reconstructed;
  ai_tactical: 17 exact of 24 live. BOTH lanes independently byte-
  proved **/Op** for their TU (fdiv kept against the pool constant vs
  reciprocal fmul; int->double rounded through memory; /Op also
  disables FP intrinsics - get_defense_boost_value calls CRT sqrt,
  not fsqrt) -> new `game_o2_ml_gr_windows_op` profile. **Engine-wide
  /Op sweep result**: flipping all 35 game TUs to /Op leaves every
  one of the 249 exact functions exact (sole movement: unmatched
  get_spell_work_chance 53.21 -> 53.09, tuned under non-/Op). /Op is
  compatible with everything matched so far; APPROVED engine-wide the
  same day - folded into game_o2_ml_gr_windows(_noeh), the _op
  profile removed (no config proliferation), get_spell_work_chance's
  non-/Op-tuned max hand-lowered 53.21 -> 53.09.
  Merge unions worth noting: SSpellTraits carries TWO per-mastery
  rows (mastery_bonus@0x34 flat add, mastery_values@0x68 value row -
  each byte-proven by its own lane's consumers); type_enchant_data is
  0x14 (ctor-proven) with ai_combat's consumer names grafted onto
  type_spell_choice (target/+0x14, value/+0x1c). New army combat-AI
  field slices and the AI leaf declarations landed in army.h;
  advManager::HeroLoses, army::get_average_damage,
  army::get_second_grid_index claimed by callee evidence. Residuals:
  the register-homing family dominates; simulate_combat adds an
  inliner-depth divergence (retail /Ob2 stops at nesting depth 2) to
  the open compiler-generation question. Identity correction:
  0x4253e0 = get_mass_damage_value (has_creature has no retail body).

- **2026-08-07 — mapcell/mainmenu/lodfile mapped (user-directed).**
  Order-mapping of the DC rosters onto the carve, every pairing
  body-corroborated, claims promoted as located @stubs:
  - **mapcell**: 54 new claims (14 -> 68) covering the whole map
    read/save/load family; the mapcell/misc boundary moved to 0x10ade0
    (misc's terrain-$E head), mapcell's own $E head at 0xfbba0; 10
    retail-only span rows recorded unclaimable from the DC spine
    (get_magic_terrain_type etc.); the seer path proven rewritten onto
    the quest-guard machinery; the inlining-vanished DC rows listed in
    the TU header.
  - **mainmenu**: the whole TMainMenu class promoted (ctor/dtors/
    DoModal/MainMenuHandler - the handler by DoModal's address-take,
    hard proof); VideomodeChoice proven a DC-port-only class (no
    retail slot exists); the second scalar-deleting-dtor claim.
  - **lodfile**: clear + the ctor promoted (8 of 17 DC rows located);
    TU head corrected to 0xfa590; nine externs proven /OPT:REF-
    stripped or inlined (sort/compare/exist/...); retail uses
    fopen-family I/O, not CreateFile.
  All three gap regions are now fully attributed (levelupwindow tail,
  $E runs classified per owner, STL COMDAT tails excluded).

- **2026-08-07 — small-TU campaign: ten TUs processed one by one
  (user-directed goal).** Four more CLOSED (functions-only, zlib/hexcell
  precedent), four reconstructed wall-to-wall with documented residual
  classes, two located-and-blocked. Repo 186 -> 217/~585 exact across
  the run. Per TU:
  - **basemgr CLOSED 1/1**: the ctor was already exact; closure evidence
    is the 1-function DC roster + cinit-thunk flanks.
  - **resource CLOSED 3/3**: the missing resident was the compiler-
    generated `??_Gresource` scalar deleting dtor. CONTRACT EXTENSION
    approved with this entry: `VA_COMPGEN` gains kind
    `SCALAR_DELETING_DTOR` (owner = class), `labels._demangle_key`
    joins `??_G<Cls>` publics, and the adoption loop includes such
    claims - the base obj already emits the body, so the claim alone
    paired it at 100 (sample's ??_G likewise).
  - **monframeinfo CLOSED 2/2, both exact on first compile**: the
    CRANIM.TXT parsers, located by the string anchor in the
    misc->mousemgr bracket. SMonFrameInfo stride 0x54 byte-derived;
    the 150-element table + const-reference alias DATA-claimed;
    ResourceManager::GetSpreadsheet promoted (DC public at dc
    0x122164); `_atoi` named in the runtime map. Second labels
    extension: the base-authority join now also reads DEFINED STATIC
    function symbols (first file-static claim).
  - **winfile CLOSED 14/14 exact**: File's surviving methods; the File
    vtable 0x643d20 slot-order corroborates (retail-vtables.tsv row
    named `File`). CFindFile proven ABSENT (the only FindFirstFile
    call sites are the CRT _find trio); five File methods absent, two
    survive only inlined - reconstructed as inline definitions, obj
    emits exactly retail's five... [see the TU header for the closed-
    file eax-passthrough idiom].
  - **path reconstructed 8/8 located (1 exact)**: the whole combat
    direction system (FindPath/ValidPath/GetAttackMask/ValidAttack/
    GetAdjacentCellIndex(NoArmy)/get_adjacent_hex/OppositeDirection);
    GetBestDirection proven dropped from retail. Byte-derived combat
    layout landed in cmbtmgr.h/army.h/hexcell.h: cells[187] stride
    0x70 at +0x1c4, adjacentCells[187][6] at +0x13468, the placement
    latch +0x13d68; army side/slot/facing/combatSide/berserk/
    hypnotize/bound fields; army::is_enemy located (0x442880).
    RESIDUAL CLASS (documented in the TU header): retail merges
    adjacent bounds-return blocks into one inline block; our SP3 CL
    either duplicates or sinks them across every structured and goto
    spelling - with kbwin's AppWndProc inline divergence this points
    at stale objects from an earlier CL generation in the retail link;
    needs a compiler-generation probe (OPEN).
  - **smackmgr reconstructed 14/14 (10 exact)**: the Smacker/Bink
    wrapper layer, .bss video cluster DATA-claimed, DirectDraw surface
    v1 vtable offsets byte-verified; four residuals in one register-
    homing family (documented per function).
  - **initialize reconstructed (3 exact + 1 at 93.9)**: the building-
    mask builders. /Ob2 LORE CORRECTION recorded: unconditional
    out-of-line emission holds for EXTERN linkage only - single-call
    inlined STATICS are eliminated (create_included_masks /
    create_building_masks have no retail bodies; initialize_game_data
    rides a forward declaration to keep DC source order under the
    ORDER gate). town::included_buildings[9][44] + the mask tables
    DATA-claimed; bitNumber/gTownEligibleBuildMask/gHierarchyMask
    named by DC publics (NH3API's giMonthTypeExtra=0x697798
    contradicted by bytes).
  - **strip reconstructed 5/5 surviving (4 exact)**: ~strip absent,
    DrawNumber/DrawSelector survive only inlined (same emission rule);
    DrawOwner 84.6 residual (cross-jump merge family). THeroTraits
    stride 92 byte-proven; akHeroTraits extern is NH3API/IDA name
    lineage - flagged.
  - **u2dvers located 3/3, BLOCKED**: the version.dll import thunks
    anchor the whole TU (ctor/dtor/GetVersionInfo at 0x5eeda0/df0/e00);
    GetVersionInfo carries an EH frame + inlined std::string (P2.2 +
    STLport OPEN).
  - **sample located 3/3, BLOCKED**: ctor found at 0x566da0 storing
    ??_7sample - WITH an EH frame, contradicting the noeh-profile
    inference (a frameless dtor does NOT exclude /GX); profile
    re-decision deferred to P2.2. ??_G claimed and exact; dtor exact.
  - Swap-outs recorded: herodefs (std::vector params + anonymous-
    namespace TU-hash manglings - poor ROI until STLport),
    quickinfowindow/global/genericresource (location inside large
    shared brackets not yet cheap).

- **2026-08-06 — hexcell CLOSED 3/3 exact (functions-only): the first
  complete game TU.** Every carved target function in hexcell.obj's
  proven link-order span (0x4e7150..0x4e71fd) is claimed and
  byte-exact: the ctor, get_army, get_dead_army. Extent evidence:
  fresh `homm3.analysis.link_order` run - 0 unclaimed in span, the
  herodefs flank is a 1-byte gap with 0 functions, the right gap's 52
  functions are hillfortwindow/hiscore candidates outside hexcell's
  alphabetical slot, and the DC compiland roster (3 functions) is
  exhausted. The ctor closed via a signedness discovery: retail feeds
  all four -1 byte stores from the SAME register as the dword -1
  (`or ecx,-1` / cl), which an `unsigned char` field declaration
  splits into a separately materialized 0xff constant - the codegen
  itself byte-proves field_1a/field_4d are signed (plain) char.
  "Complete" here is the functions-only claim of the standing
  data-scope decision; hexcell.obj's data/vtable surface is empty.

- **2026-08-06 — kbwin reconstructed wall-to-wall: 12/12 span
  residents claimed, 11 exact; AppWndProc residual documented.** The
  session that set out to close two near-misses instead recovered the
  whole Windows shell:
  - `SetNoDialogMenus` 85.9 -> 100: the "unexplained" register moves
    were fastcall argument setup for a TAIL CALL into 0x4f8220 -
    which the DC line-779 slot, the WinCE 4-byte stub, and the homm2
    template identify as **SetMenus**; reconstructed at 100 (recursive
    menu walker, homm2 buka kbwin.cpp:498 statement order, minus the
    trailing UpdateDfltMenu call retail dropped). Its data landed
    with it: `gsMenuEnableStatus` (0x67f930, stride 8 - homm2's
    pack(1) struct naturally aligned, values read from the image) and
    `gbInSetupDialog` (0x6989d0).
  - `Process1WindowsMessage` 81.4 -> 100: the peek loop is a GOTO
    loop (top-tested, no rotation, PeekMessageA called through
    memory - a recognized loop would rotate AND win the import an
    LICM hoist); the iconic pump is a bottom-tested do-while whose
    exits re-enter the peek loop.
  - New bodies at 100: `AppExit` (homm2 shape; tail jmp into
    CleanUpMenus), `GameTime::Get`/`DelayTil`/`Delay` (the wait loops
    inline Process1WindowsMessage whole - /Ob2 nesting - and Delay is
    just `DelayTil(Get() + interval)`), and **WinMain** (0x1CF at
    0x4f7a30, identified by the CRT entry's call; static AppInit
    inlined by /Ob2's single-call-site expansion, single-instance
    event guard, IDC_ARROW, szAppName/szTitle arrays at
    0x67f820/0x67f82c). The timeGetTime import forms split by TU:
    kbwin is IAT-indirect (dllimport via mmsystem.h), so kb.h's plain
    thunk-form declaration moved to button.cpp (mousemgr/misc
    precedent); button::Select still exact proves the thunk form.
  - `AppWndProc` (0x394) reconstructed at 74.8 with every case body
    instruction-exact and 15/15 rets; the one residual is our /Ob2
    single-call-site INLINE of AppCommand into the WM_COMMAND arm
    (which then tail-merges the per-case return epilogues). Retail
    keeps the call; the suppressing lever is unidentified - the
    in-source note lists the rejected hypotheses (explicit default
    arm, pointer-cast call, /Ob2-less profiles which break the
    byte-proven GameTime inlines). kbwin is NOT declared closed until
    this lands.
  - Evidence-backed callee/data claims recorded with their owners:
    kb.cpp `InitMainClasses` 0x4ed650 / `oldmain` 0x4ee3e0 (exhaustive
    order-map of the six carve rows after PollSound onto the DC
    roster) / `GameUnsaved` 0x4f4310 / `CleanUpMenus` 0x4f4b50;
    wingraph.cpp `CleanUpWinGraphics` 0x601890 / `InitGraphics`
    0x6014e0 / `AppPaint` 0x601820; misc.cpp `WritePrefs` 0x50c1b0
    (5-byte jmp into the registry writer); mousemgr.cpp
    `mouseManager::Reset` 0x50cc80; inputmgr.cpp
    `KeyboardMessageHandler` 0x4ec0e0 / `MouseMessageHandler`
    0x4ec290 (homm2-named AppWndProc callees). Provisional
    (retail-only, no DC lines, names pending): game.cpp's Immersion
    force-feedback pair `InitImmMouse` 0x4b6890 /
    `ImmMouseWindowMoved` 0x4b6950 (identities from the
    __imp_??0CImmMouse / CIFCErrors imports in the singleton ctor at
    0x4b6260) and soundmgr.cpp `ResumeSamples` 0x599b90 /
    `PauseSamples` 0x599c40 (WM_ACTIVATEAPP suspend pair).
  - Tooling: `labels._demangle_key` now joins C-mangled
    stdcall/fastcall publics (`_name@N`) so `_WinMain@16` adopts its
    true spelling; `heroWindowManager::dialogReturn` surfaced at
    +0x38 (WM_CLOSE tests 0x7805). Classifier note: GameTime::Get's
    entire body is `jmp [__imp__timeGetTime]`, so the universe
    classifier flags the claim as import-thunk-shaped; it is a real
    kbwin.cpp line-823 function (DC roster) and the gate accepts it.
  - Naming caveat recorded: 0x6987b8 "bWindowedMode" is semantically
    a fullscreen flag (nonzero selects WS_POPUP|WS_EX_TOPMOST and
    suppresses the windowed x/y save); rename deferred.

- **2026-08-06 — ratchet hygiene: stale-row deletion is part of
  carcass promotion.** 44 flat-name 0.0000 rows whose functions had
  been renamed by carcass->real promotion were hand-deleted from
  `config/match_baseline.tsv` (they can never re-appear in the report
  and each held no score information); the promotion workflow now
  implies deleting the superseded row. One hand-lower recorded:
  GetArmyMorale 72.8920 -> 72.8352, the cost of the corrected
  six-param re-transcription (recover with the STLport work).

- **2026-08-06 — recovery pass, batch 2 (goal-directed session,
  continued).** Eleven byte-exact recoveries: the full border unit
  (bitmapBorder16 dtor + Draw2 + bitmapBorder dtor - unit now 6/6),
  heroWindowManager::BroadcastMessage, kbwin's AppAbout and
  AppCommand, textEntryWidget::SetFocus, the bitmapBackedTextWidget
  dtor (empty derived dtor over inlined ~textWidget), and hexcell's
  get_army/get_dead_army plus combatManager::ResetHitByCreature.
  Ratcheted raises: SetNoDialogMenus 85.9 (retail drops the homm2
  KBChangeMenu tail; a three-delta register/jmp residual is noted
  in-source), textEntryWidget::GetCharPressed 95.6 (scan-byte
  extraction residual). Identity correction: the linkorder claim
  0x4f8140 "UpdateDfltMenu" is byte-proven to be **AppAbout** (4-arg
  stdcall dialog proc; DC kept only a 4-byte stub). Callee-claim
  promotions for label truth in unadmitted skeletons (same class as
  the earlier bracket promotions): kb.cpp
  HandleAppSpecificMenuCommands = 0x4f4350 and NormalDialog =
  0x4f6570 (shape + call-graph + homm2 lineage), wingraph.cpp
  SetFullScreenStatus = 0x6019a0; these made AppCommand's three
  REL32s name-exact. **Model correction:** the cmbtmgr
  TCombatSide/TStack view sat 0x34 past the real army record and its
  field offsets were 0x50 short - objdiff's immediate masking had
  hidden this as ResetHitByCreature's "99.9 residual". Rebased to
  `army armies[2][21]` at 0x54cc (stride 0x548, army head model in
  army.h, gpCombatManager at 0x6993d0); ResetHitByCreature went
  exact under the corrected offsets, byte-proving the fix. New
  codegen lore: VC6 pair-swaps adjacent load/store assignment pairs
  in site-built message stores (BroadcastMessage: zeros first, then
  natural field order compiles to retail's swapped-pair schedule).
  Late additions: get_upgrade_cost exact (retail is FOUR-arg - both
  creature rows in ecx/edx - against the DC three-arg prototype; the
  toCost-first declaration order produces retail's delta-walk loop)
  and executive::DoDialog exact first-try (aggregate-zeroed local
  executive, save/restore of the manager chain, ShutDown = 0x4f3690
  and executive::MainLoop = 0x4b0e40 promoted as callee claims; the
  unnamed 1046-ref central global 0x6a5d5c modeled provisionally in
  exec.cpp). CallManager was then fully implemented at 68.4 (the
  advManager suspend/resume dance; advmgr.h now derives baseManager -
  its +0x34 mode writes are baseManager::status - with advWindow,
  field_38c, gpAdvManager at 0x699268), and the retail-only
  heroWindow::SleepAllWidgets nest counter went byte-exact at
  0x5ff5b0. CallManager's sole residual is retail's EH frame with
  states 0/1/2 - homm2's version is EH-free and no local object is
  visible; the generating construct is an open research item.
  Scoreboard 160->174/523 exact (33.3%), fuzzy 25.39%->26.45%,
  baseline 565 rows.

- **2026-08-06 — OPEN (user decision needed): vendor era-correct
  STLport?** Now FOUR functions are blocked on it (the executive ctor
  - a stack std::bitset<10> - and GetArmyMorale's callee GetMorale
  joined during the recovery pass). Two transcribed functions were
  the original blockers: the
  `searchArray` ctor (its three inline std::vector member-inits) and
  `armyGroup::GetMorale` (a static `std::bitset<9>` with four
  armygrp-owned helper instantiations plus the invalid-position
  thrower at 0x34ad0). The Dreamcast build compiled against
  `..\stlport\` and the retail codegen matches STLport shapes, not
  VC6's native Dinkumware. Matching these functions requires the
  STLport headers of that era under `vendor/` (pristine, per the
  vendored-sources contract). Until approved, both functions stay at
  their transcribed-and-documented state; no further armygrp
  morale-family work is gated (GetMorale is the last member).

- **2026-08-06 — recovery pass over both workstreams: 30 locations
  promoted, two vtables named (one against NH3API), first
  workstream-driven EXACT.** User-directed ("go through both
  workstreams and recover functions"). (a) Every remaining `forced` /
  `callgraph-unique` row landed as a `VA()` claim: 13 bracket + 17
  callgraph across army, drawing, palette, ai_combat, ai_tactical,
  mapcell; all 833 claims verify unique / size-agreed with
  `config/retail-functions.tsv` / in link order. The two lanes'
  location queues are now fully consumed - what remains of them is
  `ambiguous`/`narrowed` rows needing new signals. (b) Vtable NAME
  census: `mouseManager = 0x240028` and `heroWindow = 0x243cc4`,
  byte-proven by their matched ctors' `mov [this], <vtable VA>` stores
  (0x50cb50 at 99.98, 0x5fe9f0 at 99.62). The mouseManager admission
  CONTRADICTS the carve enrichment, which had attributed 0x240038 to
  mouseManager from NH3API classes - its slots point outside the
  mousemgr body range while 0x240028's point inside it; one more entry
  for the NH3API-is-wrong-on-addresses file. `labels.py` now drops a
  candidate-enrichment class attribution wherever the hand census
  admits the same class (admitted outranks candidate; the duplicate-
  name gate is what surfaced the conflict). (c) First recovery:
  `armyGroup::GetNativeTerrain` (0x44c590) reconstructed and **EXACT
  (100.0)** - armygrp 16/34; then two template-guided near-miss closes:
  `heroWindow::AddWidget` (99.14 -> 100.0; the buka template's
  statement order - neighbor-link assignment before the NULL
  assignment - is what VC6's store pairing wants) and
  `heroWindowManager::RemoveWindow` (96.89 -> 100.0; the tail is
  `lastActive = activeWindow ? activeWindow : tailWindow;`, a shared
  store both branches feed - the old two-branch form also LEFT
  lastActive STALE when a window stayed active, so the match fixed a
  real semantic miss) - then `executive::AddManager` (90.08 -> 100.0)
  and `executive::RemoveManager` (86.95 -> 100.0), both by adopting the
  buka template's exact statement structure (combined `!status && Open`
  condition, neighbor-link-first stores, member-not-local in the tail
  arm, hoisted `prev` local with shared null-stores and early return).
  Overall 146/520, baseline ratcheted. Negative lesson recorded: the
  `searchArray` ctor (96.3) is NOT a statement-shuffle case - retail
  initializes its three STLport vectors inline (member-init codegen
  with a shared uninitialized char temp), so it needs init-list
  modeling; a store-reorder attempt scored WORSE and was reverted.
  Follow-up in the same pass: `soundManager::StopSample` 97.46 ->
  99.96 by declaring the Miles imports as dllimports with their real
  leading-underscore export spellings (`_AIL_*` behind `AIL_*` alias
  macros - retail calls through `__imp___AIL_end_sample@4`, so the
  direct-call form was a source-shape error); and the IAT label
  placeholders gained their import-lib PROOF - `labels.py` now reads
  the VC6 toolchain import libraries' archive symbol tables and names
  182 of the 274 slots with their decorated `__imp__X@N` spellings
  (provenance `iat-implib`; ambiguous or lib-absent names stay
  `iat-undecorated`). Measured: reloc-name agreement does not move
  objdiff scores, so the decoration is modeling hygiene, not ratchet
  fuel - the remaining sub-100 slivers on StopSample (99.96) and the
  mousemgr ctor (99.98) are byte-level residue still to be identified.
  A third residual class identified while probing the button family:
  EH-heavy functions (textButton::Draw and kin) differ in the
  EXCEPTION RECORD shape - our objects emit `__except_list` relocs
  and `$L…` state labels where the delinked side carries synthesized
  `…_unwind…` records - plus template-COMDAT callee naming
  (`std::vector<int>::size` vs the carve label at its retail address).
  Both are SYSTEMIC (synth-PDB EH records, P2.2's data/EH companion
  scope; template COMDAT claims) - source edits cannot close them, so
  the queue should deprioritize EH-bearing near-misses until that
  machinery lands.
  Then `heroWindow::WidgetSetStatus` and `WidgetClearStatus` (95.97
  each -> 100.0, overall 148/520): retail builds the message AT THE
  SITE with the zero fields assigned before the MESSAGE_WIDGET/command
  constants, not through the BroadcastMessage(int,int,int,int) forward
  - the block flow was already identical, only the two constant stores
  sat early. Site-built message with constants last is now a known
  house pattern alongside neighbor-link-first stores. The mousemgr
  stub campaign then landed three first-compile EXACTs (151/520):
  `HidePointer` (TCSLock RAII around `++field_68 == 1 &&
  !IsIconic(hwndApp) -> Update(1)` - the fs:[0] frame is the guard
  dtor's unwind scaffolding, TCSLock now defined inline in mousemgr.h),
  `ShowPointer` (symmetric: force resets the count, `--field_68 == 0`
  refreshes field_6c/70 through the GetCursorPos/ScreenToClient
  sequence inside an explicit nested critical section, then the same
  IsIconic gate; VC6 CSEs the EnterCriticalSection import pointer into
  ebx across the guard and the explicit call), and `ShowSystemCursor`
  (`show_it ? ShowCursor(1),HidePointer() : ShowPointer(0),
  ShowCursor(0)`). Negative result recorded: the heroWindow ctor's
  last 0.38 is a single `[this+4] = -1` store-slot slide, but moving
  `priority = -1` after the link zeros rewired the whole allocation
  (99.62 -> 40.2, reverted) - the `or edx,-1` CSE materialization
  point is order-load-bearing, so that residue needs a hypothesis
  about the -1 SHARING (priority and focusId both store edx), not
  statement shuffles. CheckUpdate's decode notes: TCSLock guard +
  one-time-init bit flags gating two timer deadlines (+0x21/+0x64 ms)
  before the IsIconic gate - needs the flag/deadline globals named
  before the body is honest. Later in the pass: `KBChangeMenu` 84.78
  -> 100.0 (152/520) - retail's fall-through arm is the windowed-off
  SetMenu(newMenu) path, the windowed block sits last (condition
  inverted, `if (!bWindowedMode) { if (newMenu) ... } else ...`).
  Third vtable admitted: soundManager = 0x23fe54, byte-proven by its
  92.7-matched ctor's `mov [this], 0x63fe54`. Three more negative
  results recorded at their sites: GetMonsterCost's struct-field
  spelling (69.0 vs the hand-lowered 82.7 admissible max - retail
  splits base and byte-offset across registers, only the forbidden
  cast reproduces it; note now in recruit.h), Process1WindowsMessage's
  two-loop split (66.8 vs 81.4 - the island-track for(;;) form stays),
  and the soundManager ctor's first-two-statement swap (92.56 vs
  92.72 - scheduler-window class, byte store vs vptr store pairing).
  Two facts the bytes taught: the faction->terrain table at 0x643698
  has a hidden `-1` row at 0x643694 (gated elementals index it at -1,
  the same bias pattern as GetAlignments' +1 census), admitted as
  `akNativeTerrains` in armygrp.h; and the merge loop's source shape
  is `if (native != NONE) { mismatch -> return NONE } else adopt`
  (the flipped form scores 90 with identical flow). Also learned and
  worth writing down: objdiff does NOT penalize data-reloc NAME
  mismatches (GetAlignments is 100.0 while its `akCreatureTypeTraits`
  reloc reads `data_2747b0` on the target side), so near-miss scores
  are real codegen deltas, not labeling debt; and the iteration loop
  is `ninja` -> `homm3 delink` (labels + normalize refresh) ->
  `homm3 status` whenever a claim or label input changes.

- **2026-08-06 — `homm2_overlap` revived as the live dual-branch
  generator; the RENAMED-TWINS lane (`homm3.analysis.h2_twins`) opens
  the unpaired residue.** User-approved plan (functions only; data and
  enum/constant comparison stay deferred per the FUNCTIONS-ONLY
  decision below). (a) The one-shot retired to `scripts/archive/` had
  gained recurring users, so it moved back to
  `scripts/homm3/analysis/homm2_overlap.py`; `$HOMM2_BUKA` (default
  `~/Projects/homm2/homm2-buka`) joins `$HOMM2_DECOMP`, and the
  functions lane joins BOTH branches buka-preferred per the
  template-shelf decision — schema keeps the first nine columns and
  appends `h2_branch,h2_arity,h2_fuzzy_pol`. Result: 593 distinct-name
  pairs (the old 611 counted overload duplicates), every one now an
  exact VC6 template (was 466); `boost.csv` template tier 148 → 171;
  the `dc_bracket` traveled set 322 → 420; both downstream maps
  regenerated byte-identical (counts 37/1350 and 24/22/1222 unchanged).
  `dc_bracket`'s traveled filter hardened from a string compare against
  `"100.0"` to a float compare in the same change. (b) The twins lane
  scores the 918 unpaired homm2 functions against SAME-CLASS DC
  candidates only: `S = 0.6·name-token-Jaccard (case-preserved
  CamelCase) + 0.2·arity (params delta, this-inclusive, soft - HoMM3
  extends signatures) + 0.2·callee-set Jaccard (homm2 E8 scan vs
  dc-xref-graph, restricted to the shared vocabulary)`. Refusal over
  cleverness: free functions refused by construction (kills the
  bzip/zlib collision), one row per unpaired function (the residue the
  old join silently dropped), and an injectivity demotion when two
  methods claim one DC row (same law as dc_callgraph). Calibrated on
  the exact-name pairs: leave-one-out top-1 487/499 (97.6%), median
  margin 0.40 vs the 0.15 bar; rename-stress (single-token deletions,
  worst rank) leaves only 5/401 (1.2%) wrong winners at `twin-strong`.
  Four positive/negative control families are asserted on every run
  and the module refuses to write evidence when one fails. Yield: 12
  `twin-strong` + 19 `twin-candidate` (flagships: `army::DoAttack →
  army::do_attack`, `SystemOptions → DoSystemOptions`, `GetNextArmy →
  NextArmy`, `ProcessMapChange → ProcessMapChangeNew`). Output
  `evidence/homm2-overlap/twins.csv`, ANALYSIS OUTPUT,
  external-candidate — promotion only in supervised review. One
  planned control was corrected during implementation: armyGroup::
  GetMorale → GetArmyMorale is NOT a rename (the DC corpus carries
  BOTH names; the exact join owns GetMorale), which is itself the
  lesson that a twin proposal must never outrank an exact pair — the
  consumed-row exclusion enforces exactly that.

- **2026-08-06 — the CALL-GRAPH lane resolves gaps link-order
  bracketing cannot (`homm3.analysis.dc_callgraph`).** Bracketing only
  decides a gap whose DC and retail runs are the same length; most gaps
  fail that because retail inlined or dropped DC functions. The second,
  independent argument: if DC function F is called by DC function G and
  R(G) is proven, then R(F) is among R(G)'s retail callees. Intersect
  that set with the gap's slots and with F's order window, and the
  choice usually collapses. **The lane refuses to emit an unsound
  proposal**: a location map is a strictly increasing INJECTION, so any
  candidate claimed by two DC functions, or landing out of order, is
  demoted (12 of the first 36 were - three advmgr rows had all proposed
  the same address, which is what surfaced the check). Final yield: 24
  `callgraph-unique`, 22 `callgraph-narrowed`. Verified independently
  before promotion - the proposed `heroWindowManager::RemoveWindow`
  (0x6024a0) takes a heroWindow*, calls its slot-2 Close(1), and
  unlinks it from head/tail at +0x50/+0x54, which is the function; once
  claimed it **matched 96.9% on the first compile** and proved the
  manager's window-list layout. 7 rows promoted to claims in admitted
  TUs. Output `evidence/dc-callgraph-map.tsv`, ANALYSIS OUTPUT.

- **2026-08-06 — DC-only functions are located by LINK-ORDER BRACKETING
  (`homm3.analysis.dc_bracket`).** The 322 homm2-traveled functions
  attested only in the Dreamcast build had no retail address, which
  blocked every one of them. Within a compiland the linker emits
  functions in source order and the DC CodeView proc offsets are in
  that same order, so proven retail addresses cut a TU into gaps; when
  a gap holds equally many DC-only functions and unclaimed retail
  functions, the order-preserving map is the ONLY one - a proof, not a
  guess. Yield: **37 forced locations across 10 TUs** (advmgr, army,
  armygrp, cmbtmgr, drawing, hero, inputmgr, palette, soundmgr) out of
  1,387 examined; the other 1,350 are honestly reported `ambiguous`
  because retail inlined or OPT:REF-dropped DC functions inside the
  gap (button is the negative control: 10 DC vs 3 retail, correctly
  refused). Proven end to end: the inputmgr trio (Open/Close/Main) was
  located by the tool, verified by body (Open clears the 0x200-dword
  buffer and latches keyboardFilter), promoted to `VA()` claims, and
  **Close and Main now match exact**. Lesson recorded in the tool: the
  claim size must come from `config/retail-functions.tsv`, never from
  the DC size - the DC build is SH4, and a DC-sized claim truncates the
  body mid-instruction (Main first scored 70% that way).
  Output is `evidence/dc-bracket-map.tsv`, ANALYSIS OUTPUT: a `forced`
  row becomes evidence only when a supervised review promotes it.

- **2026-08-06 — VC6 shows NO nonlocal register-allocation islands;
  the homm2 4.2 permute port is cancelled.** User-directed probe. The
  sibling's `docs/msvc42-optimized-nonlocal-islands.md` records that
  under MSVC 4.2 a distant source change moves allocation and
  scheduling elsewhere in the function, which is why homm2 built
  `homm2.permute.tu_state_noise` island censuses. Measured here on
  `widget::send_message` / `enable` / `set_help_text` / `Main`: adding
  an unrelated static helper to the TU, and reordering two unrelated
  functions, changed **nothing** - all four scores byte-identical
  across both probes (79.4348 / 87.1220 / 100 / 100 each time). VC6
  allocation is a LOCAL function of the function's own statement
  shape. Consequence: no `scripts/homm3/permute/` TU-state actor is
  needed; residual recovery stays local-source-shape work, and the
  skeleton/branch/reloc views are the instruments. Also fixed in this
  change: `homm3.sema._asm.summary` crashed (IndexError) on a blank
  line inside a block, which had hidden that `button::Main` differs
  STRUCTURALLY (110 vs 95 blocks), not by allocation.

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
  The constructor residual recorded in this session was resolved on
  2026-08-09: the missing fact was the source's `i < 7 && amount > 0`
  short-circuit order, which is both semantically significant and now
  byte-exact. Scoreboard at the time of this original wave:
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
6. **P0.2 sub-question, raised 2026-08-08** — should `DATA()` carry an
   optional SIZE? `labels.py:266-270` emits `"size": ""` for every `src-DATA`
   label, so vostok splits any aggregate that is read at a non-zero offset
   into a second synthetic symbol (`rcAppWindow.top` at 0x69959c became
   `bss_29959c`), and our `base+4` reloc can then never equal the target's
   `addend 0`. This caps every function reading a member of a `DATA()`-claimed
   aggregate. The cap was confirmed by an alias probe that was deliberately
   reverted — modelling a delinker artifact in source would make our object
   diverge from the real retail object. Adding a size to the annotation
   contract is the honest fix and needs sign-off.
