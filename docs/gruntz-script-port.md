# Gruntz script inventory and the homm3-decomp port plan

Status: **active implementation; the original attempt-1 port review is complete.**
Source of truth for the inventory: `~/Projects/gruntz/scripts/` at commit
`0456d62b4`. The phase plan below is retained as implementation history and
remaining-work inventory.

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

Each step was scoped as roughly one reviewable commit. Order is
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

## 5. Decision log

- **2026-08-30 — `CheckForGrailBuildingWin` restores two named helpers and
  its complete Dreamcast point-local shape.** Raw NB11 for `dc:0x190124`
  records procedure locals `any_town_loc` then `grail_town_loc`, a nested
  `this_town_loc`, and breakpoint rows that construct the wildcard before
  the configured target. The xref graph proves `game::OnSameTeam`, two
  const-reference `type_point::operator==` expressions, and
  `town::HasBuilding`. The former Windows source had renamed all three
  points, replaced OnSameTeam with a local mask helper, flattened HasBuilding
  into `active & bitNumber[...]`, and used the temporary pointer equality
  overload.

  Restoring the two named helpers raises the 515-byte function from
  **99.2692% to 99.2821%**; VC6 `/Ob2` expands both to retail's code and the
  two frozen missing-call rows retire. Restoring the original names,
  any-before-grail construction order, and canonical reference equality is
  byte-flat. All 24 Windows blocks and branch targets remain aligned. The
  residual is one extra move in the first packed equality plus one exchange
  of the two procedure-local stack homes. Fatal rules and negative controls
  preserve the local identities/order, both equality expressions, helper
  arguments, and the active-mask `HasBuilding(..., 1)` boundary.

- **2026-08-30 — `consider_single_enchantment` banks its sole raw-NB11
  local at the honest 99.5673% plateau.** The Dreamcast dossier
  (`dc:0x40bb8`) records exactly one optimized local lower bound:
  `value_func`, the pointer-to-member returned by
  `get_enchantment_function`. The Windows source now preserves that name and
  invokes the recovered member-function boundary directly. A fatal rule and
  negative control reject renaming or flattening it.

  All 40 Windows blocks already align. The residual is two byte-width
  materializations (`mov al,1`/`xor al,al` versus retail's EAX forms) and a
  three-instruction register-homing difference around `should_attack_now`
  and the Haste override. Bounded tests reject `bool act_now` at
  **98.6058%** and separate field assignments in the two decision arms at
  **95.4327%**; the previously tested `long` form is **97.4808%**. SH4 emits
  separate stores, but the absence of a recorded `act_now` is not proof that
  no optimized-out local existed, so the cross-compiler store shape is not
  promoted to a fatal invariant. The current CFG-coherent 99.5673% spelling
  remains the measured Windows peak without weakening the positive local
  fact.

- **2026-08-30 — the Dreamcast helper boundary closes
  `army::can_cast_spell` from 99.7059% to 100%.** The mandatory dossier
  (`dc:0x4beec`) places `hexcell::get_army` and
  `army::get_valid_caliph_spells` in the Master Genie arm's two adjacent
  breakpoint rows, and its SH4 lowering returns the helper's positive test
  directly. The former Windows reconstruction hand-expanded the helper's
  10..69 spell loop. That reached a tempting local maximum but left the true
  path as `mov al,1` where retail uses `mov eax,1`. Restoring the named
  helper and spelling the arm as `target && get_valid_caliph_spells(target)
  > 0` lets VC6 `/Ob2` inline the same loop while reproducing the dword
  result exactly. The same source helper is now restored in the already-
  exact `cast_caliph_spell` (`dc:0x4c3ac`) with no byte change. Its active
  definition retains the recovered 10..69 loop and
  `is_valid_caliph_spell` boundary; it emits no standalone retail slot.

  Both frozen missing-call rows retire down-only. Function-specific fatal
  rules preserve the Genie return expression, the helper body, and the
  count/zero-guard/Random/selection-loop order in `cast_caliph_spell`;
  negative controls prove that flattening the helper, changing its roster
  range, or moving the call past the zero guard turns the gate red. This is
  the concrete anti-local-minimum control: the 99.7059% peak is historical
  evidence, not permission to delete a positive Dreamcast source fact.

- **2026-08-30 — `combatManager::SetNextArmy` removes three source-false
  surrogates without sacrificing its 99.6498% Windows peak.** The Dreamcast
  dossier (`dc:0x5f934`) and raw NB11 record `result` as the sole surviving
  local in the mana-drain message scope. The CodeView xrefs prove two
  `army::get_controlling_side` calls, two `army::GetName` calls, and the
  final named `combatManager::GetControl` call after `lastMovedArmy = 0`.
  Retail corroborates the two expanded Army.h bodies and the command target
  at `0x004782d0`.

  The former 99.6498% source instead used a file-local controlling-side
  clone, direct `CreatureName` calls, the renamed local `message`, and the
  ordinal `Unnamed4782d0`. Restoring the original header-inline view and all
  named boundaries is byte-flat: the candidate remains 1,270 bytes with all
  74 Windows blocks aligned. It retires the three frozen missing-call rows
  for `get_controlling_side`, `GetName`, and `GetControl`. Four fatal
  asymmetric rules plus four negative controls now reject either helper
  flattening, renaming `result`, or restoring the ordinal tail. The remaining
  mismatch is still only the already-bounded string-temporary slot/RVO and
  late register-allocation class; no Dreamcast fact was traded for that
  percentage.

- **2026-08-30 — `combatManager::LoadArmies` restores all five surviving
  NB11 locals without moving its 99.3528% score.** The Dreamcast procedure
  (`dc:0x5e09c`) records procedure-scope `int side`; then const
  `unsigned char grouped` and const `int layout` in the outer side scope;
  and `int hex` followed by `army& thisArmy` in each occupied-slot scope.
  The SH4 line/xref stream corroborates that `thisArmy` receives the
  `army::Init` and `army::LoadResources` calls.

  Windows previously used loop-scoped `side`, mutable
  `tight_formation`/`last`, and repeated direct `armies[side][placed]`
  receivers. Restoring the recovered names, const qualifiers, lifetimes,
  declaration order, and reference receiver is byte-flat: all 36 candidate
  and retail blocks still align, and the sole residual remains retail's
  `cmp eax,esi` versus candidate `test eax,eax` zero-test selection. Three
  fatal asymmetric rules and three negative controls now reject loop-scoping
  `side`, dropping the const recorded locals, or flattening `thisArmy`.

- **2026-08-30 — `game::PerWeek` restores the inlined `town::IsCastle`
  source boundary and makes the complete neutral-town tail exact.** The
  mandatory dossier for `PerWeek` (`dc:0xb41e0`) ends with the older
  `GiveTroopsToNeutralTowns` helper. Its own dossier (`dc:0xaa6f8`) proves
  that the source tests `town::IsCastle`, then applies the 80-percent
  fortified roll or the 40-percent open roll. `town::IsCastle`
  (`dc:0xbcc40`) in turn has one breakpoint statement containing three
  ordered `HasBuilding` calls. The former Windows source had flattened that
  whole positive helper chain into direct 64-bit mask expressions. Restoring
  the helper and letting VC6 `/Ob2` expand it raises `PerWeek` from
  **99.7274% to 99.8370%** and makes all 26 instructions and three branch
  targets in the fort/citadel/castle decision exact. The companion retail
  check also fixes the later Summoning Portal test to `HasBuilding(..., 0)`:
  target reads `built` at +0x150/+0x154, not `active` at +0x158/+0x15c.

  Raw NB11 separately proves ten procedure-scope locals in this order:
  `obscuring_hero`, `iAlign`, `alternate_bonus`, `bonus_amount`, `x`, `y`,
  `i`, `z`, `bonus_creature`, and `map_cell`. It places `iCount`/
  `iIncrease`, `luck_bonus`, and `currHero` in their respective monster,
  fountain, and hero-loop scopes. Restoring that exact procedure/nested
  distinction is byte-flat. Fatal rules and negative controls now reject the
  previous local reordering, hoisted nested locals, direct-mask de-inlining,
  a flattened `IsCastle` definition, and the wrong Summoning Portal mask.
  Candidate and retail have the same 1,976-byte extent, all 128 blocks, and
  every branch, operation, and relocation aligned. The only residual is the
  opening creature-week scan's ESI/EDI role permutation between `this` and
  `i`; why-reg measures 61 unpaired register-visible slots, finds identical
  definition slots/order, and caps its legal creation-order edit byte-flat as
  C1 front-end state.

- **2026-08-30 — `AI_enter_town` ratchets its shared Dreamcast orchestration
  at the honest 99.5109% final-load plateau.** The mandatory dossier at
  `philai.obj+0x10e3f8` has 37 breakpoint rows and 32 lexical scopes. Raw NB11
  records only one non-optimized local, `artifact`, inside the spellbook-
  purchase scope. The reconstruction preserves the nested Grail guard and
  helper sequence, the artifact construction / `GiveArtifact` / gold-debit
  scope, `upgrade_creatures` before `buy_special_building`, both difficulty-
  scoped artifact swaps before the siege purchases, and
  `DemobilizeCurrHero` before the garrison and visiting-hero effects. Four
  fatal asymmetric source rules and four independent negative controls now
  reject flattening, hoisting, reordering, or splitting those facts.

  Complete independently adds the town-type building switch and Conflux
  university visit, and moves Dreamcast's separate `buy_artifacts` step into
  the retail-exact `buy_special_building` receiver; those remain bounded
  retail-only additions. Candidate and retail are both 1,548 bytes, all 74
  blocks align, and their 43 branch targets and two returns agree. Only the
  final visiting-hero address schedule differs: retail loads `gpGame` before
  the first two index operations, while this compile loads it afterward.
  Named receiver/result locals, reuse of `garrison_hero`, and a procedure-
  scope result are byte-flat; naming the id falls to **97.01087%**. The
  synchronized allocator model's three proposals and the guided sweep's nine
  mutations find no improvement, so no source distortion is retained.

- **2026-08-30 — the boat-pool load/save pair restores its symmetric
  Dreamcast serialization locals; the exact save path remains exact.** Raw
  NB11 records `unsigned short ushort_buffer`, `int count`, `int x`,
  `unsigned char uchar_buffer`, and `char char_buffer` at procedure scope in
  that order in both `game::LoadBoatPool` and `game::SaveBoatPool`. Breakpoint
  rows and SH4 operands further identify eight result-bearing I/O statements:
  two byte-unsigned buffers, five char buffers, and one unsigned-short buffer.
  The former load source instead read bytes through `int count`, used an
  unsigned `i`, and nested invented value/hero buffers; the former save source
  had the analogous exact-but-source-false simplification.

  Restoring the proved roster, typed buffers, shared `count` result, and field
  order is byte-flat in both functions. `SaveBoatPool` remains **100.0000%**
  across all 429 bytes. `LoadBoatPool` remains **99.6447%**, with all 25 blocks
  and branches/instructions aligned except seven scale-one SIB encoder choices
  (`[edi+index+field]` versus `[index+edi+field]`). Earlier pointer-addition
  and `why-reg` controls leave the same post-allocation address-fold tie. Fatal
  rules and negative controls now reject the old buffer reuse even though it
  had the same percentage.
- **2026-08-30 — `NewSMapHeader::loadVictoryCondition` restores two
  optimized-out Dreamcast assignments and banks the honest 99.8359% stack-
  coloring plateau.** The mandatory dossier at game.obj+0xaeb64 contains 119
  breakpoint rows, 65 lexical scopes, and 26 `gzread` calls. Because the
  compact scope tree does not attach locals to a lexical owner, the raw NB11
  records were inspected: `int_buffer`, `count`, and `char_buffer` are the
  only `S_REGREL32` locals before the first `S_BLOCK32`, in that procedure-
  scope order; no nested local record occurs before the next procedure. The
  two leading reads now retain DC's `count =` assignments. They optimize out
  byte-flat in Complete, whose retail CFG directly contradicts the older
  build's two immediate short-read returns. A fatal asymmetric source rule
  and negative controls reject either reordering the three locals or erasing
  those assignments; the claim now carries the missing `dc 0xaeb64` bridge.

  Retail and candidate remain the same 996-byte extent with an exact 34-block
  CFG and identical operations. The only differences are stack homes in five
  blocks: the common flag byte, the resource byte, the upgraded-town dword,
  and the final castle byte. Reusing DC's common buffers measured 99.15641%;
  sharing only its char buffer measured 99.23333%; shared Complete case
  temporaries measured 99.78718%; and hoisting the Complete temporaries to
  function scope measured 99.75641%. All changed otherwise-exact homes or the
  frame, while the recovered dead assignments remain at the best
  **99.8359%**. Complete's saved-version hero remap and selective read checks
  remain retail-only revision facts rather than being forced to the older DC
  counts.
- **2026-08-30 — `soundManager::MemorySample` restores and ratchets its raw
  NB11 statement shape at the honest 99.7840% allocator plateau.** The
  mandatory Dreamcast pass (`dc:0x14b528`) records the `sPtr` parameter, no
  post-argument S_REGREL32 locals, 24 breakpoint rows, 27 lexical scopes and
  25 SH4 blocks. Raw line 817 emits the wrapped slot's current/next update as
  one statement, and line 821 groups channel selection with the named
  `StopSample` helper. The source and public declaration now use `sPtr`, the
  wrap is the recovered chained assignment, and `StopSample` consumes the
  selected channel directly. The raw line table jumps from boundary line 759
  to body line 769, but the gap classifier marks that boundary unavailable
  because it is coherent with the preceding procedure's closing row; it is
  not admitted as an assert clue. Fatal asymmetric source rules and negative
  controls reject splitting the chain, expanding StopSample, duplicating
  ConvertVolume, or reordering the shared stop / volume / start / handle-
  return sequence.

  Complete retail independently rejects two older/platform-specific shapes.
  Dreamcast's separate early-return scopes score **66.07407%** under the PC
  compiler; preserving separate guards with a shared failure label reaches
  only **93.88889%**, while retail's seven PC checks have one shared tail.
  Dreamcast's byte-taking SetVolume has one post-selection statement, whereas
  Complete's int-taking Miles adapter needs two call arms to emit retail's
  per-arm argument pushes; a shared int result and a ternary both score
  **98.51234%**. These are measured DC-only/platform spellings, not excuses to
  remove compatible facts.

  Candidate and retail remain 475 bytes with 30 blocks, 21 branches and two
  returns in agreement. The sole residual is ten register-visible slots in
  the PC-only inlined stream-service tail: retail chooses EAX/EDX/ECX where
  this C1 state chooses EDX/ECX/EDX. Earlier and function-scoped handle
  declarations, direct tail expansion, short-lived manager locals, nested MP3
  scopes, named stream and split section-pointer lifetimes are byte-flat at
  **99.78395%**; the synchronized allocator model finds no source-addressable
  improvement. No register-forcing distortion is retained.

- **2026-08-30 — `combatManager::AreaEffect` restores its Dreamcast local
  roster and banks the honest 99.8565% register-allocation plateau.** The
  mandatory dossier (`dc:0x153b60`) proves the function-scope
  `casting_hero`, `multiple_targets`, `targets`, and `damage` locals in that
  order, plus the shared SpellEffect / vector construction /
  mark_area_effect / per-target damage / victim-selection / final-effect
  statement sequence. The reconstruction now carries those names and that
  relative declaration order literally. Complete's independently
  retail-proven Random/SpellCastWorkChance guard is retained as retail-only
  revision shape. Fatal asymmetric source rules and negative controls reject
  reordered or renamed locals, reordered setup calls, bypassing
  `casting_hero`, erasing the failed-target clear, or flattening the recovered
  `multiple_targets` state.

  Candidate and retail remain the same 585-byte extent with 18 basic blocks,
  eight branches, and one return. Every block except the failed-roll clear is
  instruction-exact; that block differs only by an EAX/ECX/EDX rotation over
  five otherwise identical instructions. `homm3 vc6 why-reg` measured ten
  guided mutations without improvement. Combined named side/slot indices,
  separated and moved optimized-out deaths/victim declarations, and a const
  loop target supplement the earlier single-index and if/else probes; all are
  byte-flat at **99.85646%**. No source-distorting arithmetic spelling is kept
  for the remaining C1 allocator tie.

- **2026-08-30 — `value_of_enemy_town` escapes a source-false 99.9561%
  local-lifetime minimum without losing a byte.** The compact dossier's local
  names were insufficient by themselves; the raw NB11 records show
  `creature_cost`, `include_growth`, and `creature` as procedure-scope
  `S_REGREL32` entries before every lexical block, in that record order. The
  previous high-score source incorrectly declared the cost array and creature
  enum inside the positive-population branch. Restoring the three proved
  lifetimes and their order is byte-flat at **99.9561%**. A fatal source rule
  plus nested-lifetime and reordered negative controls now prevent regression.

  Candidate and retail still agree on all 35 blocks, 18 branches, and every
  instruction except the encoding order of the final commutative LEA:
  `[ebx+ecx]` versus `[ecx+ebx]`. The existing return-operand, declaration,
  and accumulate-then-return controls do not steer that SIB choice. The
  positive Dreamcast lifetime fact is retained while that compiler tie remains
  an honest one-instruction residual.

- **2026-08-30 — `town::GiveSpells` ratchets its Dreamcast-proven nested
  guards at the honest 99.9216% schedule plateau.** Dreamcast breakpoint
  rows 992/994/999 and their lexical scopes record the current-hero,
  spellbook, and Mage Guild tests as three nested statement groups. Retail's
  37 blocks and 23 branches independently agree with that lowering. A fatal
  source rule and negative control now reject combining the guards into one
  percentage-seeking `&&` expression.

  The only remaining instruction delta is the order of two independent
  reloads at the ordinary Mage Guild loop latch: retail reloads `level`
  before `this`, while the candidate does the reverse. The prior bounded
  656-shape loop search and the clean lifetime, condition, and const-member
  controls either retain this order or score worse. No positive source fact
  is removed to chase the transposition.

- **2026-08-30 — `game::SetupPuzzlePieces` banks Dreamcast's complete
  seven-local declaration roster without sacrificing its score.** CodeView
  proves, in order, `long piece`, the two percentage floats, `int i`,
  `long j`, `int iExtraPieces`, and `int iPiecesRemoved`. The previous source
  had grouped the loop indexes and integer counters ahead of the floats;
  restoring the recovered order is byte-flat at **98.9637%**. A fatal source
  rule and negative control now prevent that compatible positive fact from
  being traded away during later codegen experiments.

  The explicit-code residual remains one `/Op` scheduling transposition:
  retail loads the numerator between the two int-to-float rounding trips,
  while all three honest cast placements emit both trips before the load.
  The apparent bitset discrepancy is not a source mismatch: candidate
  `puzzlePiecesRemoved+4` and target `bss_2976ec+0` address the same second
  dword. It is the known target data-topology split caused by a size-less
  aggregate claim, so the source is not distorted to imitate it. Resolving
  aggregate symbol extents remains the separately recorded P0.2 annotation-
  contract decision.

- **2026-08-30 — Dreamcast-first reconstruction closes
  `town::initialize_hordes` exactly after deliberately leaving its
  source-false local maximum.** The former **99.7778%** spelling ordered the
  upgraded horde writes `creature, bonus, dwelling`, because that happened to
  leave only an AX/DX register tie. Dreamcast breakpoint rows 958/959/960
  positively prove `creature, dwelling, bonus`; restoring that order first
  dipped to **96.8444%**, with the high-water score preserved only as history.
  A const pointer to the base entry's bonus, declared after the dwelling
  statement, then prevents C2 from moving the bonus load across the creature
  store. The result is **100.0000% across all 118 bytes**, with all 11 blocks
  and all 5 branches exact. A fatal source-order rule and negative control now
  reject the old percentage-seeking reversal.

  Controls bound the spelling: a post-dwelling short value local remains at
  96.8444%, while a named reference to the upgraded entry falls to 85.9778%.
  This is the campaign's concrete demonstration that a Dreamcast-proven
  source correction may need to pass through a lower local score before the
  retail-exact lowering becomes reachable.

- **2026-08-30 — `game::randomize_university` now preserves the compatible
  Dreamcast local facts across its remaining C1 register wall.** CodeView
  types `choice` and `i` as `long` and `skill` as `TSecondarySkill`, in that
  order. Restoring the enum type and recovered order is byte-flat at the
  existing **99.7464%** peak, with all 24 blocks, all 11 branches, and all 377
  retail instructions still aligned; a fatal source rule and negative
  controls now prevent a percentage-neutral retreat to `int skill` or a
  reordered shared roster. Complete's disabled-secondary-skill handling is
  independently retail-proved and remains retail-only.

  The older DC `type_university university` local does not transfer to the
  Complete source: under the retail-proved Complete class model it emits a
  call to `type_university::type_university`, while retail has no such call,
  and the control falls to 97.8623%. The raw four-int record is therefore
  retained. The sole explicit-code residual remains an EBX/EDI role swap
  between `availableCount` and `choice`; `why-reg` finds identical definition
  slots and no successful source-local creation-order mutation, classifying it
  as C1 front-end state rather than permission to remove a positive DC fact.

- **2026-08-30 — `advManager::DoEventPrison` banks its Dreamcast source
  shape at the honest 99.9540% C1-schedule plateau.** Dreamcast CodeView
  proves the original `heroID` (`THeroID`, underlying `T_INT4`),
  `OldColorCycling`, and `OldAnimCtrPaused` locals, the color/animation
  save-disable-restore order, and the rescued-hero statement sequence. The
  Windows model does not yet expose `THeroID`, so `heroID` retains compatible
  four-byte `int` storage rather than inventing a local enum view. A fatal
  asymmetric source contract now preserves those positive facts while
  allowing Complete's retail-proven `heroPoolMap` statement between the
  shared Dreamcast statements; negative controls reject a flattened hero id,
  swapped save locals, and reordered coordinate stores.

  Retail and candidate have the same 886-byte body, 15 blocks, 261
  instructions, seven branches, and three returns. The only code delta is the
  order of two independent reloads after the expanded `bitset<8>::_Xran`
  guard: retail reads the pointer home `[ebp-0xc]` before the position home
  `[ebp-0x8]`, while VC6 SP3 reads them in use order. The model-guided
  register pass found no binding divergence and the guided pass measured 41
  mutations without improvement. Natural `at()`/`set()` spellings,
  pointer/reference/proxy locals, a signed owner local, and both paired local
  declaration orders are byte-flat or worse. No semantic distortion is
  retained to chase that final transposed instruction pair.

- **2026-08-30 — `ResourceManager::GetPalette24` restores Dreamcast's named
  palette locals and banks its source shape at the honest 99.9273% plateau.**
  The mandatory Dreamcast dossier (`dc:0x121ec8`) proves function-scope
  `char header[24]` and `TRGBA rgba[256]` locals, in that order, followed by
  header and rgba reads, direct `TPalette24` construction, and optional
  `AdjustHSV`. Retail Complete independently proves two platform-expanded
  copies of that read/construct/adjust sequence: an ordinary-file adapter and
  an archive/fallback adapter. The reconstruction now uses the attested `rgba`
  name instead of the byte-flat `paletteData` alias, and fatal source rules
  with negative controls reject a rename, reversed reads, or replacing the
  direct constructor boundary with an invented helper.

  Candidate and retail remain the same 721 bytes in extent, with 220 body
  instructions, 24 blocks, 14 branches, three returns, and paired call/data
  relocations. The sole residual is an eight-byte VC6 stack-coloring choice:
  candidate frame `0x444`, retail `0x43c`; retail overlaps the dead path-string
  slot with eight bytes of the later stdio adapter. The existing 120-form
  declaration/result/file-lifetime search and eight helper spellings are now
  supplemented by branch-local results, result/file reordering, shared and
  reordered adapter-interface declarations, a named fopen result, direct
  adapter calls, a named path local, and an explicit ordinary/archive `else`.
  The honest variants are byte-flat or worse (lowest 93.1045%); `why-reg`'s
  nine guided probes also do not move toward retail. This is recorded as
  TU/C1 allocator state, not as authority to discard the positive Dreamcast
  facts for a percentage.

- **2026-08-29 — `playerData::save` restores and ratchets Dreamcast's named
  serialization locals instead of preserving a byte-flat flattened form.**
  The CodeView roster and SH4 statement stream distinguish the write-result
  `count` from the signed loop index `x`: all twenty original `gzwrite`
  statements assign `count`, while the heroes, town-id and resource loops
  reuse `x`. Complete's `TAbstractFile::Write` wrapper lowers those assignments
  away, so the coherent reconstruction remains **99.9557%** with the same 49
  blocks, 385 instructions and branches as retail. A fatal source rule plus
  negative controls now reject direct `Write` comparisons, a reordered local
  roster, or reuse of `count` as the loop index even though each can be
  byte-flat.

  The remaining mismatch is confined to one Complete-only stack-home cycle:
  candidate `x/flags/bits` use `-0xc/-0x8/-0x4`, while retail uses
  `-0x8/-0xc/-0x6`. Moving the two-byte buffer to function scope and changing
  the bitset pointer to a const reference are byte-flat; direct member access
  falls to 98.2109%. The synchronized build -> delink -> build checkpoint stays
  **2,554/3,100 linked functions exact, 93.10% fuzzy, and 61.21% filtered
  executable coverage**, with every fatal gate green.

- **2026-08-28 — Rust DEF parity now uses the real reconstructed candidate
  for every encoding, with retail packet grammar admitted independently.**
  The former test-only `DrawTile`/`DrawAdvObjImpl` stubs are gone. Live claims
  at `0x47dd40` and `0x47d0a0` reconstruct raw rows, word-indexed packed rows,
  32-pixel adventure cells, horizontal/vertical tile branches, literal code 7,
  optional adventure flag code 5, and transparent packed controls. The
  mandatory Dreamcast dossiers (`dc:0x76988`, `dc:0x76060`) supplied the helper,
  local, scope, and repeated-statement shape; retail x86 supplied the actual
  tables, tags, effects, dispatch, and verdict. Restoring the four duplicated
  direction arms raised `DrawTile` from 36.848793% to 52.23191%; the
  eight-statement Duff loop proved by both line tables and retail jump tables
  raised it to 73.22727%; and the DC-attested raw do/while rows plus indexed
  encoded for-rows raised it to 79.04174%. Split packet load/increment and
  block-scoped row destinations now raise it to **83.2115%**, with all 81
  branches and 10 returns agreeing. The recovered `kOpaqueRunCode` and raw-row
  declaration order are byte-flat positive facts; the remaining four-arm
  delta is explicitly a non-exact C1 register permutation.

  `DrawAdvObjImpl` is now **byte-exact** (1,099 bytes): unsigned cell-line
  arithmetic, split packet load/increment, and a block-scoped row destination
  recover retail's logical shift, packet schedule, and dead-hflip parameter
  home. The adjacent general-RLE `CSpriteFrame::Draw` is also **byte-exact**
  (1,125 bytes): declaring its DC-attested dword table at function scope,
  assigning it only inside the positive render guard, and using block-scoped
  row destinations close all 88 blocks. These closures supersede the earlier
  92.91832% and 94.35% C1-wall classifications.

  The specialized general-RLE creature path advances **93.77% -> 95.90%**.
  CodeView restores the public `CSpriteFrame::div2mask`/`div4mask` ownership,
  older ushort view, const `kOpaqueRunCode`, and local-before-static symbol
  order. Complete then exposes the missing C1 lever: `div2mask` is written as
  a word but selected operations read a dword whose low word alone is stored,
  while `div4mask` remains word-wide. A cast-free union storage view at the
  first alpha loop makes every forward blend/shade block instruction-exact
  and aligns both CFGs at 128 blocks. The
  residual is confined to Clip/setup register homes and reverse-loop tail
  scheduling; renewed line-table, row-destination, direct-expression, and all
  four reverse-width combinations are measured regressions.

  The raw Dreamcast CodeView records further recover the renderer's original
  function-scope aliases: `TOffset` is `unsigned int` and `TDstPixel` is
  `unsigned short` (`DrawTile` uses an `unsigned short` `TOffset`). Typing the
  creature row table through `TOffset` is retail-byte-flat. Pairing the
  guarded row-table assignment with one or both `TDstPixel` reverse-shade
  temporaries measures 94.72%/95.69%, below the retained 95.90% high water;
  those score-lowering forms remain rejected.

  `CSpriteFrame::SetPixelFormat` is now **byte-exact** (247 bytes, 13/13
  blocks). The previous source named `rMax`, `gMax`, and `bMax`, forcing their
  lifetimes to overlap all three channel-bit counts; VC6 consequently used a
  12-byte frame and issued the three shifts before their consumers. Repeating
  the maxima directly in the half- and quarter-mask expressions lets C1 recover
  retail's cross-statement CSE, sequential EAX scratch, and 8-byte frame. The
  source-labelled diff is fully identical.

  A fresh `DrawTile` audit also corrected the scope of its remaining wall.
  Retail and Dreamcast independently prove the surprising general-RLE fallback
  call `Draw(sw, sy, sw, ...)`, not the natural `Draw(sx, sy, sw, ...)`.
  Spelling that fact fixes the call site but lowers the whole-function score
  from 83.2115% to 81.4249% because SP3 then carries `this` in EDI through all
  four direction arms instead of retail's EDX. Swapping the two leading
  declarations and replacing the local code-7 constant with the existing enum
  are byte-flat in combination. The score-lowering spelling is retained as a
  positive shared-source and retail semantic fact; its 83.2115% historical
  checkpoint remains recorded while the surrounding source shape needed to
  recover retail's EDX home is reconstructed coherently.

  `combatManager::DrawWallAt` is now **byte-exact** (796 bytes, 54/54
  blocks), superseding its 99.95% selector-layout plateau. Dreamcast lines
  1459..1470 recover a positive three-cover guard containing the direct
  main/lower/upper `if`/`else if`/`else` selection. The former synthetic
  inline helper retained retail's second lower-tower comparison but made VC6
  place the upper selector before the main selector. Restoring the recovered
  lexical shape retains that comparison and emits retail's lower, main, upper
  physical order; every normalized instruction and branch edge now agrees.

  The independent `no_std` Rust blitter fixed one real semantic defect found
  by this comparison: packed encoding controls are always transparent in the
  ordinary renderer; the `tblit` option applies only to general-RLE fills. The
  opt-in gate compiles `src/cspriteframe.cpp` itself and passes 512 generated
  cases per encoding (2,048 total), including multi-cell adventure rectangles.
  A second environment-selected test parses and inflates legally installed
  LODs, then differentials one deterministic clipped full-frame draw for every
  member frame. All **39,939/39,939** Steam frames agree: 665 raw, 26,849
  general RLE, 187 tileset RLE, and 12,238 adventure RLE. Game bytes remain
  external, and the C++ differential remains corroboration rather than a
  substitute for retail-byte proof. The supplied original-media corpus adds
  independently clean gates for RoE 1.0 (29,393), RoE 1.1 (29,393), the AB
  supplement (5,599), and SoD (34,324) frames.

- **2026-08-28 — all retail-accepted saved games get a separate
  allocation-free parser.**
  `homm3-save` consumes the full inflated `H3SVG`/`H3SVC` stream for versions
  16–18 and 25–42, exactly the revisions `SavedGameHeader::Load` accepts:
  saved header and setup, campaign carry-over records, map cells and attached
  objects, object templates and placements, black boxes, quests, both event
  lists, game object pools, all player/town/hero records, map-extra and point
  pools, universities, creature banks, and all eleven recorded-action types.
  The implementation follows the retail serializers and their reconstructed
  mirrors, with Dreamcast dossiers used only for positive helper/source-shape
  facts. Retail-specific quirks stay literal: version 16's coordinate-form
  loss-hero payload; the fixed 0x66a9-byte campaign snapshot below version 28;
  narrow pre-25 creature ids; the 128/156 hero split; versioned equipment,
  town-name, quest, timed-event, and recorded-boat fields; narrowed scalars;
  the 70-byte town bitset dump; and dword hero ids in replay records. The
  library is `no_std`, dependency-free, allocation-free, and forbids unsafe
  code; gzip and recursive GM/TGM/CGM discovery live in `homm3-oracle saves`.
  Generated full streams for all 21 revisions, non-empty probes for every
  historical width family, every truncation of the current stream, and a
  hostile deterministic corpus are gates. Revisions 1–15, 19–24, and above
  42 are rejected at retail's version boundary. The Steam installation still
  supplies no real saves, so no revision is called corpus-closed.

- **2026-08-28 — complete H3M bodies join the allocation-free resource
  oracle, with retail's class remap table admitted explicitly.**
  `homm3-map::MapBody` now consumes every placed-object payload, quest and
  Seer reward variant, nested town event, global timed event, and the installed
  maps' 124-byte zero editor trailer. The retail initializer at `0x41b500`
  first gives all 232 adventure-trait rows identity dispatch values, then
  applies the 46 key/value overrides stored at `0x63a6e4..0x63a854`: raw
  terrain ids 165..205 map onto their runtime classes, while 219/220/221/223/
  230 map to 33/53/99/21/46. This is why the Complete-only Garrison II and
  Abandoned Mine records must use the reconstructed garrison and mine readers;
  treating the initialization loop alone as proof of identity is rejected.
  The independent EOF gate is clean for 160 standalone maps and all 113 H3C
  embedded maps. The fixed zero trailer is corpus-format evidence—the retail
  `NewfullMap::Read` path returns after the timed-event list and does not
  consume it—not a promoted retail reader claim.

- **2026-08-28 — the independent resource oracle follows the sibling
  `no_std`-core/`std`-edge architecture and admits formats by explicit retail
  dialect.** Allocation-free, dependency-free Rust crates now own LOD,
  SND/VID, gzip envelopes, DEF, engine-owned LOD payloads, IFF/XMIDI, H3C
  descriptors, and the three `NewSMapHeader::Read` generations; filesystem,
  inflation, reporting, and PNG work stay in the CLI. External WAV/MP3/SMK/
  BIK/IFR codecs close only at the byte-exact handoff boundary that retail
  owns. The ledger in `docs/resource-format-matrix.md` distinguishes retail
  proof, candidate parity, Dreamcast-only hypotheses, corpus support,
  and open world/save semantics. Corpus success cannot promote a guessed
  dialect: twelve `SGTWMTA/B` DEF frames use a separately named
  interleaved-compact-header manifest, while the default retail dialect
  continues to reject them as a negative control. The original US RoE 1.0 CD
  proves this layout belongs to the initial release rather than Steam; RoE 1.1
  and SoD repeat it. Every DEF frame passes the reconstructed C++ draw oracle:
  29,393 in each RoE pressing, 5,599 in the AB supplement, and 34,324 in SoD.
  All 251 standalone maps and the installed/loose SND/VID sets also pass. The
  same pass adds the original RoE H3C generation: a direct version-1 header
  with compact `(name, gzip size, prerequisite mask)` region records followed
  by map members, versus the later gzipped version-4/5/6 full header. Both
  forms remain allocation-free in `homm3-map`; envelope inflation stays in the
  `std` oracle. Across the four discs, all 42 campaign observations, 175 region
  descriptors, and 159 embedded maps validate to EOF. The selector is now
  `known-interleaved` (`steam` remains an input alias). No game bytes enter the
  repository.

- **2026-08-28 — `TAdventureOptionsWindow::WindowHandler` closes exactly after
  restoring the Dreamcast exit state hidden by its 99.9367% local maximum.**
  The old source duplicated the end-dialog stores and returned directly from
  the mouse arm. Dreamcast instead initializes one exit carrier before
  dispatch, sets it for selected options, and consumes it in the shared
  lines-237-242 tail. Restoring that state first preserves the direct
  line-211 `findWidget(msg->mouseX, msg->mouseY)` statement through a measured
  99.8734% dip, then makes VC6 select retail's EAX/ECX argument staging and all
  508 bytes match. The same pass restores CodeView's const-qualified
  `findWidget`/`findWidgetPtr` pair and the distinct
  `gAdventureOptionsHelp` identity; both helper bodies remain exact after the
  required delink refresh.

  The one missing Dreamcast call is now a proof-carrying Complete transfer,
  not a free skew label. Dreamcast calls `ShowScenInfo` in the handler only for
  campaigns and in `advManager::DoAdventureOptions` only outside campaigns;
  retail forwards the selected id from the handler and its exact outer switch
  calls `ShowScenInfo` unconditionally. The fatal gate suppresses the old-call
  defect only while that forwarding shape, receiver call, and receiver's exact
  retail score all hold. Negative controls remove each proof leg separately.
  Additional rules reject erased exit state, early/duplicated returns, split
  coordinate locals, the wrong help table, and non-const helper declarations.
  Exact count rises **2534 -> 2535/3097**.

- **2026-08-28 — Dreamcast source shape breaks the
  `TSplitWindow::WindowHandler` local maximum and closes all 735 bytes.** The
  old 99.9170% candidate duplicated the end-dialog source tail and deleted the
  two state variables solely to induce retail's cross-jump. Restoring the
  CodeView-proven entry state and one shared tail deliberately fell to
  70.4149%; restoring the single shared `splitSlider->SetState` at line 291
  raised it to 94.9378%. The final five-instruction residual was not retail
  skew: Dreamcast lines 313-318 prove a positive changed-hover scope followed
  by that mouse arm's own return. That source shape makes VC6 assign the two
  semantically identical return tails exactly as retail does. The result is
  34/34 aligned blocks and a 100.0% source-labelled diff. The recovered
  `UpdateSplitArmy` and `SetRolloverText` helper boundaries also remain explicit
  in source and inline exactly. Fatal source-shape rules now reject removed
  close/update state, duplicated slider or end-dialog source, an inverted
  hover early-return, and missing shared consumers. This is the campaign's
  concrete anti-local-minimum control: a score dip cannot waive Dreamcast
  facts, and an isolated failed spelling cannot be classified as version skew.
  Exact count rises **2533 -> 2534/3097**.

- **2026-08-28 — paired relocation proof closes three false plateaus without
  changing source shape.** `AI_value_of_combat`,
  `type_town_threat_checker::mark_towns`, and `value_of_hall` were already
  instruction-identical but remained at 99.95% because stripped-image
  recovery represented honest literals as DIR32 rows or named an interior
  field instead of the candidate aggregate plus addend. The paired normalizer
  now removes a target relocation only when the candidate has no relocation
  of any type at that function-relative site and its literal equals the
  target symbol+addend VA. Aggregate/field rewrites require one unambiguous
  equal-addend retail-data anchor and equal resolved addresses. It compacts
  relocation tables in place, reparses COFF, and checks every unrelated
  section, symbol and relocation. Five negative controls cover wrong literals,
  candidate relocations, missing anchors, and different field addresses. The
  corpus admits 6 false literals and 243 field splits; exact count rises
  **2530 -> 2533/3097**. At that checkpoint the deliberately preserved
  Dreamcast-aligned `TSplitWindow::WindowHandler` valley remained open; the
  later source-shape closure above completes it.

- **2026-08-28 — Dreamcast-aligned `TSplitWindow::TSplitWindow` is byte-exact,
  and its former 99.9912% ceiling is removed in tooling.** The Dreamcast
  CodeView dossier proves the `TCreatureType` parameter/member, thirteen
  ordinary `Widgets.push_back` statements, the canonical `slider` constructor,
  and a null text argument in the status-bar `textWidget`. Restoring those
  facts first crossed a deliberate 79.13% valley because VC6 recursively
  expanded the final vector insertions. A Complete-only elemental-background
  helper restored all 57 body blocks without reintroducing the old
  `AppendSplitWidget`, `insert`, or fabricated `TSplitSliderView` forms. The
  final source correction from `""` to the Dreamcast- and retail-proven null
  argument removes the last instruction-width difference.

  The remaining EH prologue residual was a COFF representation difference:
  VC6 names the handler thunk directly, while Vostok names the last cleanup
  funclet with its size as addend. The paired normalizer now checks the exact
  EH prologue, associative `.text$x`, final handler thunk, equal retail
  addend, and unchanged resolved target before applying the semantic retail
  unwind-owner name. It admitted 551 equivalent corpus relocations and left
  18 differing cleanup topologies untouched; four hermetic controls prevent
  over-normalization. A fatal Dreamcast source-shape contract now rejects the
  adapter/insert local maximum, non-thirteen push counts, the empty-string
  argument, the invented slider class, and the erased enum domain. The report
  reaches **2530/3097 functions exact**.

- **2026-08-28 — the complete TCP session-search path is reconstructed at
  91.32%.** Retail `TMultiPlayerWindow::OnSearch` at `0x511660` opens the
  address dialog, disables OK until the address is valid, replaces the current
  DirectPlay connection with TCP, performs one five-second session enumeration,
  joins the first result, and reports the four proven failure cases. Dreamcast's
  less-optimized 660-byte body supplies unusually strong source-shape evidence:
  one `DisableOK` call, four `NormalDialog` calls, five `CMPInputDlg`
  destructor calls (the five logical returns), and three `CHourGlass`
  destructor calls (all post-enumeration returns). Its line and block records
  additionally prove that `sErr[256]` belongs to the join-failure block and
  that `GetLastError` runs before dialog 456; the SH4 call order independently
  confirms the latter. Complete expands the ordinary two-argument dialog
  constructor at this site; marking that real constructor forced-inline while
  pinning the exact `OnHost` call site preserves both existing 100% functions.
  The DC header helpers `DisableOK`, `GetText`, and `TTextResource::operator[]`
  are now represented directly, while Complete's mangled `QAEEXZ` name retains
  its authoritative `unsigned char` return despite DC's `_N` (`bool`) variant.

  Pinning the nested network calls and three late destructor boundaries yields
  all 13 retail branch tests. Hoisting the saved DirectPlay error before its
  dialog raises the 1,638-byte body from **90.94% to 91.32%**. The residual is a
  coupled C1 front-end-state wall: `why-reg` sees the same definition slots and
  order, but the candidate binds ESI=`this`, EDI=`-1` where retail binds
  EDI=`this`, ESI=`-1`. When the failure block reuses ESI for the error, the
  candidate spills `this`, grows its frame from retail's `0x188` to `0x18c`,
  and merges two late false cleanup tails (three returns versus four). The
  DC-shaped negative join condition reproduces the retail branch sequence but
  regresses to 89.81%; caller locals and a used constructor sentinel parameter
  are copy-propagated byte-flat, confirming this is not a statement-level
  naming lever. The nested positive `if/else` retains the higher score while
  preserving the proven `sErr` scope. The
  synchronized report reaches **2526/3094 functions exact**, **92.93% fuzzy**,
  and **60.80% executable coverage**.

- **2026-08-28 — the multiplayer TCP initialization pair is reconstructed;
  its Winsock helper is byte-exact.** Retail `GetIPAddress` at `0x5112e0`
  retains the Complete-only Winsock path behind Dreamcast's four-byte stub:
  it creates and binds a nonblocking UDP socket, resolves the local hostname,
  copies the first IPv4 address, and closes the socket. All 257 VC6 bytes and
  11 control-flow blocks are exact. Retail `TMultiPlayerWindow::OnTCP` at
  `0x5113f0` initializes DirectPlay, activates the host/join/search controls,
  publishes that address, clears and re-enumerates the session array, updates
  the slider and redraws. Its 611-byte reconstruction has the same 20 logical
  block bodies, stack offsets, call and relocation sites, and per-block
  instructions; the measured residual is physical block order only: VC6
  places the connection-failure dialog at the tail, while retail interposes
  that cold return between the `textWidget` constructor and its null-allocation
  continuation. The synchronized report reaches **2526/3093 functions
  exact**, **92.93% fuzzy**, and **60.73% executable coverage**.

- **2026-08-27 — the multiplayer hot-seat dispatcher is byte-exact.**
  Retail `TMultiPlayerWindow::OnHotSeat` at `0x511d40` opens a stack-local
  `CHotSeatDlg`, returns false on the cancel result, or selects
  `MP_HOTSEAT` and returns true. The two result paths each carry the exact
  compiler-generated dialog/widget teardown and `heroWindow` destruction
  sequence; all 209 VC6 bytes and three control-flow blocks are exact. The
  synchronized report reaches **2525/3091 functions exact**, **92.94%
  fuzzy**, and **60.69% executable coverage**.

- **2026-08-27 — the CHeroSessions destructor pair is byte-exact.**
  Retail's scalar deleting destructor at `0x50ede0` calls the implicit
  `CHeroSessions` destructor at `0x558350`, then conditionally frees the
  object. The implicit body installs the `CAutoArray<CDPlaySession>` vtable,
  deletes every session through virtual `Get`, frees the backing array, and
  zeros its three bookkeeping fields. That call edge, vtable relocation, and
  exact candidate topology prove both compiler-generated identities; all 117
  VC6 bytes and eight control-flow blocks are exact. The synchronized report
  reaches **2524/3090 functions exact**, **92.94% fuzzy**, and **60.68%
  executable coverage**.

- **2026-08-27 — the multiplayer player-name key override is byte-exact.**
  Retail `CMultiPlayerWindowEdit::OnKeyPress` at `0x50ed60` suppresses Enter,
  delegates every other key to `textEntryWidget`, and redraws the multiplayer
  window only when the base editor consumes the key. Dreamcast supplies the
  identity and three-call graph; retail vtable `0x640054` corrects the class
  declaration from slot-2 `Main` to slot-15 `OnKeyPress`. All 67 VC6 bytes
  and five control-flow blocks are exact. The synchronized report reaches
  **2522/3088 functions exact**, **92.94% fuzzy**, and **60.68% executable
  coverage**.

- **2026-08-27 — the multiplayer edit key dispatcher is byte-exact.**
  Retail `CMPEdit::OnKeyPress` at `0x5107d0` rejects keys while the edit lacks
  focus, sends Shift+Tab and keypad Up through virtual slot 20, sends plain
  Tab, Enter and keypad Down through slot 19, and calls the text-entry base
  handler for every other key. The Win32 `HIWORD(GetKeyState(VK_SHIFT))`
  spelling reproduces retail's distinctive sign-extend/logical-shift test;
  all 115 VC6 bytes and 11 control-flow blocks are exact. The synchronized
  report reaches **2521/3087 functions exact**, **92.94% fuzzy**, and
  **60.67% executable coverage**.

- **2026-08-27 — the multiplayer edit-navigation trio is byte-exact.**
  Retail `CMPEdit::OnNextEdit`, `OnPrevEdit`, and `SetFocus` at
  `0x510850`, `0x510870`, and `0x510890` follow the active adjacent edit via
  the owning window or forward focus state to `textEntryWidget`. Dreamcast
  CodeView supplies the virtual identities and proves that the navigation
  pair extends CMPEdit's vtable at slots 19/20; retail fixes the shifted PC
  fields, active-bit gates and short widget-id loads. All 70 VC6 bytes are
  exact. The synchronized report reaches **2520/3086 functions exact**,
  **92.93% fuzzy**, and **60.67% executable coverage**.

- **2026-08-27 — the multiplayer input-dialog OK updater is byte-exact.**
  Retail `CMPInputDlg::UpdateOK` at `0x510980` checks whether the first edit is
  active, explicitly disables or enables widget 505 from the edit string's
  emptiness, and redraws the full dialog. Dreamcast CodeView supplies the
  member identity and field roles; the retail bytes fix the active bit,
  duplicated `GetWidget` calls and empty-first branch polarity. The resulting
  93-byte, eight-block VC6 body is exact. The synchronized report reaches
  **2517/3083 functions exact**, **92.93% fuzzy**, and **60.66% executable
  coverage**.

- **2026-08-27 — the complete multiplayer host dispatcher is byte-exact.**
  Retail `TMultiPlayerWindow::OnHost` at `0x50fda0` dispatches modem hosting,
  delegates the serial path to `OnDirectHost`, and opens the generic
  session-name/password dialog for every other protocol. The decisive source
  boundary is the retained `OnModemHost` helper: VC6 inlines it one level into
  `OnHost` but leaves its nested member `InitRemote` call out of line, exactly
  matching Complete's emission. Dreamcast CodeView supplies both member
  signatures, the `CMPInputDlg sessDlg` local and the dialog/callee xref set;
  retail fixes the three dialog-text cells, empty-password null conversion,
  21-block CFG and all seven destructor-bearing returns. The resulting
  695-byte VC6 body is exact. The synchronized report reaches **2516/3082
  functions exact**, **92.93% fuzzy**, and **60.66% executable coverage**.

- **2026-08-27 — the direct-serial multiplayer host path is byte-exact.**
  Retail `TMultiPlayerWindow::OnDirectHost` at `0x50fc50` expands the complete
  `MP_SERIAL` initialization, arms the two host-state dwords, balances the
  cursor around session creation, and treats DirectPlay's user-cancel result
  separately from reportable failures before restoring the dialog result and
  network state. Dreamcast CodeView supplies the member signature and its
  distinctive `InitRemote`/`HostSession`/`GetLastError`/dialog xref set;
  retail fixes the three general-text indices and complete 12-block CFG. This
  cross-check also proves that Complete absorbed `OnModemHost` into `OnHost`
  and emitted `OnDirectHost` earlier than the older Dreamcast compiland. The
  resulting 335-byte VC6 body is exact. The synchronized report reaches
  **2515/3081 functions exact**, **92.93% fuzzy**, and **60.62% executable
  coverage**.

- **2026-08-27 — the multiplayer session-join path is byte-exact.** Retail
  `TMultiPlayerWindow::JoinSession` at `0x50fa10` joins through the selected
  session's instance GUID, creates the local DirectPlay player with the
  current game-version dword, and publishes the resulting DPID, name, and
  version through `gsThisNetPlayerInfo`. The stored DirectPlay error is the
  success gate; only a clean result clears the session timer. Dreamcast
  CodeView supplies the member signature and emission-order bracket, while
  retail independently fixes the session layout, virtual calls, global
  writes, and complete two-branch/three-return CFG. The resulting 159-byte
  VC6 body is exact. The synchronized report reaches **2514/3080 functions
  exact**, **92.93% fuzzy**, and **60.61% executable coverage**.

- **2026-08-27 — the multiplayer session-host path is byte-exact.** Retail
  `TMultiPlayerWindow::HostSession` at `0x50fab0` formats the advertised name
  as `"%s%c%s"` with the `0xfa` separator and local player name, derives the
  DirectPlay migrate/keep-alive/protocol flags, hosts an eight-player session,
  and creates the local player with the current game-version dword. Success
  copies the player name and version into `gsThisNetPlayerInfo`; TCP alone
  queries the local IP address into the window's 80-byte buffer. Dreamcast
  CodeView supplies the member signature and 256-byte `sFullName` local, while
  retail fixes the PC object fields, five-branch/three-return CFG and every
  virtual call. The resulting 262-byte VC6 body is exact. The synchronized
  report reaches **2513/3079 functions exact**, **92.93% fuzzy**, and **60.60%
  executable coverage**.

- **2026-08-27 — the multiplayer DirectPlay initializer is byte-exact.**
  Retail `TMultiPlayerWindow::InitRemote` at `0x50fbc0` first records the
  selected network protocol, passes the player-name widget's string to the
  global `InitRemote`, and opens the protocol-specific connection through
  `InitConnection`. Only success reaches `CDPlay::GetCaps`: the timeout at
  `DPCAPS+0x24` becomes the session refresh interval plus 100 ms, except TCP
  uses the fixed 1,000 ms interval. Dreamcast CodeView supplies the member
  signature and 0x28-byte `dpCaps` local; retail independently fixes the PC
  object offsets, calls and complete four-branch/three-return CFG. The
  resulting 134-byte VC6 body is exact. The synchronized report reaches
  **2512/3079 functions exact**, **92.91% fuzzy**, and **60.59% executable
  coverage**.

- **2026-08-27 — the multiplayer session-refresh handler is byte-exact.**
  Retail `TMultiPlayerWindow::WindowHandler` at `0x50f940` polls sound, gates
  session enumeration on the elapsed refresh timeout, tears down the old
  session array, refreshes DirectPlay, updates the slider and window, then
  delegates to `CHeroWindowEx::WindowHandler`. The 197-byte body inlines the
  array teardown exactly. A named copy of the pre-call session timestamp is
  the final source fact: VC6 keeps it in EDI across `GameTime::Get` and then
  reuses that saved register for `pSessions`; direct member reads reload the
  timestamp and produce a 200-byte body. The synchronized report reaches
  **2511/3079 functions exact**, **92.90% fuzzy**, and **60.58% executable
  coverage**.

- **2026-08-27 — the Complete multiplayer deselect dispatcher is reconstructed
  at a bounded 98.1199%.** Retail `0x50f4e0` is the 1,112-byte
  `TMultiPlayerWindow::OnWidgetDeselect` switch over the main connection
  choices, host/join/search flows, twelve session rows, slider/rollover no-ops,
  and cancel. The recovered source reproduces the complete CFG, instruction
  count, block offsets, jump tables, DirectPlay caps/session enumeration, and
  all shared cleanup/return tails. Five late blocks retain only VC6 register-
  scheduling differences: three exit-flag stores, the return-to-menu virtual
  draw, and the session-index/GetCount pair; remaining unpaired helper/data
  relocation names are cosmetic.

  The DirectPlay calls use the canonical `CDPlayHeroes` vtable instead of a
  TU-local shell. `DPCAPS` now has one shared, retail-proven 0x28-byte owner
  declaration, while the multiplayer header's already-proven
  `CAutoArray<CDPlaySession>` implementation is selected narrowly to avoid its
  duplicate dxplay definition in this TU. The synchronized build reports
  **2510/3079 functions exact**, **92.88% fuzzy**, and **60.57% executable
  coverage**, with the ratchet, single-view, and cleanliness gates clean.

- **2026-08-26 — `NewfullMap::NewfullMapFn_00505F20` closes at 100%.** The
  settled candidate already agreed with retail in all 19 blocks and 10
  branches; its 343-byte body differed only in four frame-slot encoding bytes.
  The source
  distinction is the non-const `bitset<10>::operator[]` proxy: spelling the
  terrain-mask query as `.test(terrain)` lets VC6 compact the frame from 0xc
  to 0x8, while `mask_34[terrain]` preserves retail's otherwise-dead proxy
  slot and produces the exact object.  The older reference/push_back spelling
  is rejected in the current include/delink generation because it over-inlines
  to 21 branches versus retail's 10; a generated 316-candidate declaration,
  lifetime, and identifier search was flat at 99.9771%.  The required
  build/delink/build cycle leaves the ratchet and all repository gates clean.

- **2026-08-26 — candidate assembly gains verified VC6 `/Z7` source
  statements.** `homm3 sema disasm <addr> --base --source [--verbose]`
  interleaves the candidate's real source lines with the ordinary matching
  object's assembly, and `homm3 sema diff <addr> --source` aligns base/target
  instructions first, then groups the result beneath candidate statement
  headings. Source text is display-only: it cannot affect comparison or rc.

  VC6 stores these lines as classic six-byte COFF `IMAGE_LINENUMBER` records,
  not modern C13 records, so the in-tree reader resolves each function anchor
  through its `.bf` auxiliary symbol and preserves repeated offsets. The
  on-demand debug object is separately cached under `build/debug/`, keyed by
  flags plus source/header contents; sema refuses its offsets unless the
  selected function's logical code bytes equal the pristine base object's.
  Live SP3 controls prove `GetRemoteData` at 0x554400 stays 15 bytes and exact
  with `/Z7`, labels its return at `remote.cpp:1250`, and returns rc 0 in the
  source diff; the known 86.98% `CDPlayHeroes::SendIt` returns rc 1 and names
  its first divergent candidate statement. These are candidate-navigation
  records only and make no claim about retail source.
- **2026-08-26 — seven game bitset `_Xran` boundaries are corrected and nine
  generated rows close exactly.** The admitted carve stopped each `_Xran` at
  the noreturn `__CxxThrowException` call after 200 bytes, but retail visibly
  retains `pop edi; pop esi; pop ebx` immediately afterward and before the
  next function's alignment. VC6's corresponding named publics are 203 bytes
  and include those same three bytes. `config/retail-functions.tsv` therefore
  corrects `0x4d1850`, `0x4d1c80`, `0x4d2090`, `0x4d2610`, `0x4d26e0`,
  `0x4d4eb0`, and `0x4d5000` from 200 to 203 bytes; every corrected row then
  matches byte-exact.

  The public callers' bounds independently fix widths 4, 70, 156, 5, 28, 12,
  and 128. `VA_COMPGEN` gains a narrow direct-symbol `BITSET_XRAN` contract
  with a width-decoding selftest. A late-anchor call also emits the adjacent
  exact `bitset<128>::set` and `test` rows. Finally, retail `0x4cf750` is
  corrected from `vector<TRumour>::size` to `vector<boat>::size`: both callers
  are boat-pool code and the body divides by `sizeof(boat) == 40`, not the
  20-byte rumour stride. The synchronized build reaches **2249/2681 linked
  exact**, **96.85% linked fuzzy**, and **48.87% executable coverage** over
  the corrected **1,996,539-byte** function universe with every gate clean.

- **2026-08-26 — four more public game STL helpers close through the late
  emission anchor.** Retail's immediate bounds and element stride identify
  52-byte `bitset<4>::test` at `0x4cf960`, 19-byte
  `vector<type_university>::capacity` at `0x4cfa40`, 99-byte
  `bitset<156>::set` at `0x4d0070`, and 52-byte `bitset<28>::test` at
  `0x4d17d0`. Each is a named public already supported by a typed
  `VA_COMPGEN` contract; extending the non-runtime game STL anchor makes VC6
  emit all four byte-exact without disturbing an existing row.

  The synchronized build reaches **2239/2672 linked exact**, **96.84% linked
  fuzzy**, and **48.79% executable coverage** with every gate clean. Private
  bitset `_Tidy` residuals remain excluded from this technique because public
  calls cannot emit them and whole-template instantiation is too broad.

- **2026-08-26 — the retained game `std::logic_error` constructor is exact.**
  Retail `0x4c3090` is Dinkumware's 354-byte string-taking
  `std::logic_error` constructor. Its public name fixes the class and overload,
  and byte-identical copies already live in hero.obj, mapcell.obj, and
  victory.obj. Extending game.cpp's trailing, non-runtime STL emission anchor
  produces the same VC6 public after all real game bodies, without changing an
  existing comparison row.

  `VA_COMPGEN` gains the narrow direct-symbol `CLASS_CTOR` contract; the join
  resolves the overload group by the uniquely claimed `0x162` extent, and its
  demangled-key selftest pins the long Dinkumware string signature. The
  synchronized build reaches **2235/2668 linked exact**, **96.84% linked
  fuzzy**, and **48.78% executable coverage** with every gate clean.

- **2026-08-26 — two missing game bitset COMDATs are restored with a late
  emission anchor.** Retail's game callers prove the exact 103-byte
  `bitset<70>::reference::operator=` at `0x4cefa0` and 99-byte
  `bitset<144>::set` at `0x4cf9a0`; the same VC6 members were already emitted
  byte-exact in hero.obj. The still-unreconstructed game bodies do not exhaust
  this tree's /Ob2 budget at the same sites, so game.obj lacked both publics.

  A trailing `#pragma inline_depth(0)` anchor, following hero.cpp's established
  pattern, emits only those public template members after every real game body
  has compiled. It creates no target-enumerated report row and leaves every
  existing match unchanged. The private `bitset<70>::_Tidy` at `0x4cfa10`
  remains unclaimed: a public call cannot emit it, while whole-template
  instantiation is deliberately rejected as broad and collision-prone. The
  synchronized build reaches **2234/2667 linked exact**, **96.84% linked
  fuzzy**, and **48.76% executable coverage** with every gate clean.

- **2026-08-26 — two owner-local generated destructors close exactly.** The
  nine-element random-town-name pool passes `TPickRandomTownName`'s destructor
  to VC6's teardown iterator; its 38-byte game.obj public is byte-identical to
  retail `0x4caa40`, and the Dreamcast roster independently supplies the class
  name. Retail `0x4c4df0` is the exact 62-byte destructor for the value pair in
  `map<int, type_map_hero_info>`: the mangled public fixes
  `pair<const int, type_map_hero_info>`, while the body releases the mapped
  type's string member.

  `VA_COMPGEN` gains a narrow direct-symbol `PAIR_CONST_INT_DTOR` contract so
  the template owner is bound from VC6's public rather than approximated by a
  handwritten declarator; its join has an embedded negative control. The
  synchronized build reaches **2232/2665 linked exact**, **96.84% linked
  fuzzy**, and **48.75% executable coverage** with every gate clean.

- **2026-08-26 — the quest-monster identifier append and reverse lookup are
  reconstructed byte-exact.** Retail `readMonsterData` supplies an integer
  stream identifier and packed `type_point` to `game+0x4e7bc`; the natural
  local-pair plus `vector::push_back` spelling reproduces all 464 bytes at
  `0x4ced40`, including VC6's capacity growth, copy, insertion, cleanup, and
  unwind shape. This also upgrades the final game member's eight-byte element
  layout from a destructor-width hypothesis to executable proof.

  The sole semantic consumer at `0x56ef20` resolves the same identifier for a
  defeat-monster quest. Expressing its scan as `for (unsigned i = size();
  i-- != 0;)` reproduces VC6's retail register schedule and all 104 bytes at
  `0x4cef10`, including the packed all-minus-one miss sentinel. The synchronized
  build reaches **2230/2663 linked exact**, **96.84% linked fuzzy**, and
  **48.74% executable coverage** with every gate clean.

- **2026-08-26 — the `HeroExtra` teardown and two more retained bitset
  COMDATs are exact.** The 74-byte body at retail `0x4ce520` releases and
  clears the string triple at `HeroExtra+0x30c`; its address is passed beside
  the already exact constructor to the 156-element construction helper and
  reused by `game::~game` and its unwind funclets. The owner-specific
  `IMPLICIT_DTOR` join binds VC6's byte-identical generated destructor without
  inventing a source definition.

  The 18-byte body at `0x4cef80` is the width-independent
  `bitset<N>::operator[]` that returns its two-word reference proxy.
  `game::GetRandomMonster` supplies `bitset<145>` callers, while the matching
  game.obj public supplies the exact COFF identity; the direct-symbol contract
  therefore gains `BITSET_SUBSCRIPT` plus a negative-control join case. The
  five-word nibble-count loop at `0x4cf010` independently identifies and
  exactly matches `bitset<145>::count`. The synchronized build reaches
  **2228/2661 linked exact**, **96.84% linked fuzzy**, and **48.72%
  executable coverage** with every gate clean.

- **2026-08-26 — the retained hero-setup map `_Tree::_Min` helper is admitted
  exact.** Retail's `CMapHeaderData::Save` expands iterator increment but
  keeps the protected static `_Min` call. Its sole argument, the 0x2c map
  node's right child, and the loop over node +0 left links identify the helper;
  VC6's pinned `XTREE` body reproduces all 50 bytes including `_Lockit`.
  `VA_COMPGEN` gains the narrow direct-symbol `TREE_MIN` contract, keyed by
  the mapped value type, with a mangled-name negative control. An emission-only
  derived map shim takes the protected member's address without changing the
  authoritative `std::map<int, type_map_hero_info>` member or a runtime caller.

- **2026-08-26 — the retained hero-setup red-black insertion cluster is exact.**
  The 277-byte public `_Tree::insert`, its 761-byte protected `_Insert`, and
  the 179-byte `const_iterator::_Dec` reproduce retail byte for byte. Their
  search, duplicate-key, node-allocation, rotation, and predecessor paths are
  the pinned VC6 `XTREE` algorithms; the mangled symbols independently bind
  the value to `type_map_hero_info`. Retail keeps `_Lockit` calls and nested EH
  cleanup in this cluster, so game.cpp exposes the header's external-lock
  (`_MT`) view while retaining the `/ML` runtime profile. Statement-local
  inline depth preserves the public search body and its inlined `_Max` path.

  A broad explicit `pair` instantiation was rejected because VC6 cached its
  constructor policy and regressed the unrelated insert-result pair. The
  narrow `pair<const int, type_map_hero_info>` specialization instead preserves
  Dinkumware's layout and constructors while keeping the previously exact
  62-byte destructor out of line. `VA_COMPGEN` adds owner-keyed `TREE_INSERT`,
  `TREE_NODE_INSERT`, and `TREE_CONST_ITERATOR_DEC` contracts with mangled-name
  negative controls. The gated build reaches **2266/2700 linked exact**,
  **96.79% linked fuzzy**, and **49.14% executable coverage** with no banked
  loss.

- **2026-08-26 — the map hero-setup value constructor is reconstructed
  byte-exact.** The sole retail caller in `NewSMapHeader::Read` passes an
  identity dword, an owned 16-byte Dinkumware string, and a four-byte
  `bitset<8>`; the callee writes them at `+0`, `+4`, and `+0x14`, destroys the
  by-value string, and returns with `ret 0x18`. Those independent ABI and
  member-offset facts identify the 304-byte body at `0x4c4cc0` as
  `type_map_hero_info(int, string, bitset<8>)`.

  A direct three-member game.obj view is codegen-significant: it lets VC6
  expand the string copy into the constructor and reproduces every instruction,
  branch, cleanup state, and relocation. Expressing the record as an inherited
  identity instead leaves `string::assign` out of line. The direct view is
  therefore scoped behind `HOMM3_GAME_NEW_MAP_DECLS`; other TUs retain the
  layout-equivalent inherited declaration and their already-banked compiler
  state. The gated build reaches **2267/2701 linked exact**, **96.79% linked
  fuzzy**, and **49.16% executable coverage** with no banked loss.

- **2026-08-26 — the split game transfer catch/continuation is folded back
  into its parent.** The carve treated `0xcb1ec` (23 bytes) and `0xcb206`
  (2,782 bytes) as functions following the 540-byte body at `0xcafd0`.
  Retail's `HandlerType` record at `.rdata` `0x64db70` instead points to
  `0xcb1ec` as that body's catch handler. The normal path jumps over it to
  `0xcb206`; the resumed code uses the parent's saved EBP frame, reaches both
  shared epilogues, and owns the trailing switch tables through `0xcbce4`.
  The hand-owned inventory now records one 3,348-byte EH-bearing function,
  removing two false independent targets without discarding any retail code.

- **2026-08-26 — `game::SaveGame`'s typed failure path is restored and its
  false catch entry is retired.** The old 615-byte extent stopped at the normal
  return and treated `0xbf107` as an independent 122-byte function. Retail's
  HandlerType record instead points there as the `TGzFile::TOpenFailure` catch;
  it reads the parent's EBP-relative filename, formats general-text row 10,
  shows `NormalDialog`, and returns the shared zero-result continuation at
  `0xbf181`. The inventory now owns the complete 758-byte extent through
  `0xbf196` and no longer counts the handler as a function.

  Dreamcast independently names the `saveGameTimer` and `compression` locals.
  Restoring the latter's default-plus-override spelling and the stream's nested
  lifetime raises `SaveGame` from 93.8148% to **99.9057%**. All 27 CFG blocks
  and 244 instruction sequences align; the residual is an eight-byte VC6 stack
  coloring difference. The gated build remains **2281/2721 linked exact** at
  **96.75% linked fuzzy** and reaches **49.86% executable coverage**, with no
  banked loss or new cleanliness violation.

- **2026-08-26 — nine retained Dinkumware bitset members are admitted exact,
  and the direct-symbol annotation contract now covers the bitset family.**
  The contiguous game COMDAT run independently exposes the encoded widths in
  each mangled public and the matching retail bounds/masks: reference
  assignment for `bitset<5>` and `bitset<28>`, the `bitset<28>` constructor,
  `count`, `flip`, and `any`, `bitset<145>::set/test`, and
  `bitset<8>::test`. All nine target extents equal the corresponding VC6
  COMDAT extents and compare byte-exact.

  `VA_COMPGEN` gains direct-symbol `BITSET_*` kinds rather than inventing
  source bodies. The source join now decodes VC6's compact decimal and
  hexadecimal non-type template arguments, keeps the existing
  `bitset<10>::_Tidy` mapping, and carries negative-control cases for every
  admitted member kind. The two pre-existing hero claims for
  `bitset<144>::any` and `bitset<70>::set` migrate to the typed contract with
  their exact bytes unchanged. The synchronized build reaches **2225/2658
  linked exact**, **96.84% linked fuzzy**, and **48.71% executable
  coverage**; all ratchet, banked-row, claim, single-view, and cleanliness
  gates pass.

- **2026-08-25 — the skeleton-transformer creature grid reproduces all 655
  retail bytes.** Its sole retail caller is the 0x5654f0 transformer-window
  constructor, while the Dreamcast roster identifies the row at 0x566760 as
  `type_skeleton_window::create_creature_icons` and supplies its full
  signature. Complete independently proves the 83-by-98 grid, small-font
  count widgets, case-distinct `twcrport.def` / `TwCrPort.def` portraits,
  group/slot pair, hidden selection state and all three widget insertions.
  The two expanded `type_transformer_slot` constructors also byte-prove their
  base arguments and the original `slot`-before-`group` assignment order;
  there is no retained constructor body to claim.

  The decisive source correction was `item_number + count`, matching the
  earlier sacrifice grid, rather than directly mutating `item_number`. That
  gives VC6 retail's scalar and pointer induction variables. This partial TU
  remains one source-level inline candidate short of retail's `/Ob2` divisor:
  a measured empty tail-call scaffold emits no instructions and keeps only
  the third nested vector insertion out of line, raising the natural body
  from 32.79% to 99.97%; swapping the constructor's two member assignments
  then makes all 19 blocks and 655 bytes exact. The scaffold is explicitly
  marked for removal when the missing original expression is recovered. The
  synchronized checkpoint reaches **1969/2387 linked exact**, **1900/2318
  game exact**, **96.61% game fuzzy** and **44.46% executable coverage**; the
  full ratchet, banked-row, claim, single-view and zero-debt cleanliness gates
  pass.

- **2026-08-25 — the sacrifice creature-icon grid reproduces all 1,063
  retail bytes.** The helper is the sole call target from
  `create_creature_widgets`; its position in the Dreamcast roster and
  CodeView signature identify it as
  `type_sacrifice_window::create_creature_icons` at 0x561f70. Complete
  independently proves the 83-by-98 grid stride, small-font count labels,
  HELP.TXT row 13, case-distinct `twcrport.def` / `TwCrPort.def` portraits,
  selection frame and all six widget-vector insertions for each cell.

  Both `type_army_slot_widget` constructions are expanded at this site, so
  their base `iconWidget` arguments and the derived slot/left-pane stores are
  byte-proven even though no retained constructor body exists to claim. The
  natural nested-loop reconstruction reached 99.81% with all 34 semantic
  blocks exact. VC6's remaining register-homing difference was resolved by
  restoring the declaration order `count`, `text_x`, `text_y`: the long-lived
  count occupies EAX while the front end's reverse pseudo walk assigns the
  two coordinate initializers to retail's EDX/ECX pair. The synchronized
  checkpoint reaches **1968/2386 linked exact**, **1899/2317 game exact**,
  **96.60% game fuzzy** and **44.43% executable coverage**; the full ratchet,
  banked-row, claim, single-view and zero-debt cleanliness gates pass.

- **2026-08-25 — the sacrifice-window constructor reproduces all 1,396
  retail bytes.** Its direct calls to the next two large retail rows, in the
  Dreamcast roster's `create_artifact_widgets` / `create_creature_widgets`
  order, pin the constructor at 0x55fdd0. Complete then independently proves
  the 150-entry widget reserve, seven terminal widgets, callbacks and help
  row, the hero-class town gates, all eight member-vector initializers and
  the final AddWidget walk. Dreamcast's field records further correct all
  nine action-button members from generic `widget*` to `type_func_button*`.

  Natural reconstruction initially reached 92.11%; moving `current_hero`
  from the initializer list to the body reproduced retail's member-init
  order and raised it to 96.75%. The last structural mismatch was not an
  optimizer guess: Dreamcast CodeView names the constructor's two locals as
  `widget_id` and `widget* new_widget`. Restoring the latter for the OK
  button produces retail's extra stack home and null-path lowering, making
  all 38 semantic blocks exact. The adjacent 33-byte deleting destructor and
  five-byte destructor tail remain deliberately unclaimed because five
  trivial slot-widget classes share those ICF-folded bodies and retail does
  not prove an owning class.

  The full cleanliness gate also exposed the old pointer-parameter model for
  `type_func_button` callbacks: Dreamcast types the stored handler as
  `int(message&)`, while the header had `int(message*)` and forced two new
  casts here. Correcting the shared handler type removes those casts with no
  byte change and keeps the button constructor/Main plus all affected callers
  exact. The synchronized checkpoint reaches **1967/2385 linked exact**,
  **1898/2316 game exact**, **96.60% game fuzzy** and **44.38% executable
  coverage**; the full build ratchet and zero-debt cleanliness floors pass.

- **2026-08-25 — the sacrifice offering/scroll/batch chain adds 2,194 exact
  retail bytes; the 863-byte all-artifacts callback is behavior-complete at
  86.78%.** The offering widget's sole call target and the adjacent
  Dreamcast roster row identify `offering_click` at 0x563a80. Its natural
  nested source exactly reproduces the complete 16-byte offering exchange,
  retained source/value fields, right-click artifact view and both expanded
  pickup/drop paths across all 26 semantic blocks and 795 bytes.

  The next two constructor callback pointers identify the symmetric
  `scroll_backpack_left` / `scroll_backpack_right` rows at 0x563da0 and
  0x563f00. Retail independently fixes HELP.TXT rows 18/19, the corresponding
  hero rotation methods and each expanded backpack refresh; both 338-byte
  callbacks match immediately. The following callbacks at 0x564060 and
  0x564340 are `empty_backpack(message&)` and `all_artifacts(message&)`, not
  the intervening Dreamcast-only helper rows. Complete expands the attested
  `add_artifact` and `empty_backpack()` helpers: they fill the first free
  offering, scale and add its experience, remove up to 64 backpack records,
  and in the all-artifacts case first walk the sixteen admissible equipped
  slots. The 723-byte empty-backpack callback is exact.

  `all_artifacts` agrees through its first 37 semantic blocks. The remaining
  inliner wall is bounded: this compile expands `empty_backpack` but retains
  its nested `update_backpack` call, while retail expands both. A plain
  `inline` hint is byte-flat; force-inlining either helper raises this site to
  about 91.4% but regresses the exact empty-backpack callback to 98.49%, and
  force-inlining `update_backpack` also regresses `backpack_click`. The
  source-authentic call graph is therefore banked without a global steering
  attribute. The synchronized checkpoint reaches **1966/2384 linked exact**,
  **1897/2315 game exact**, **96.59% game fuzzy** and **44.31% executable
  coverage**. All 51 unit tests, five freshness controls, 23 VC6 oracle
  probes, link-order checks and fatal gates pass; the regenerated queue has
  417 residual functions / 29.5 KiB recoverable and only the two known
  inlined-away `create_included_mask` diagnostics. No external implementation
  body was used.

- **2026-08-25 — the sacrifice artifact helper and equipped-slot handler are
  exact; the backpack handler is behavior-complete at 86.13%.** The retained
  171-byte row at 0x55fc30 is the public
  `type_artifact_offering::set`: backpack_click calls it with the offering
  record, source slot and hero, while the adjacent Dreamcast roster supplies
  its identity and signature. The doll/backpack widget call targets and the
  next two Dreamcast rows then identify `artifact_click` at 0x5632a0 and
  `backpack_click` at 0x5636c0. Retail proves pickup/drop behavior, spellbook
  and catapult handling, legal-slot checks, backpack errors, pointer changes,
  experience accounting and every refresh edge.

  The decisive source fact comes from Dreamcast line 126: the offering helper
  caches the artifact class before lines 128-131 copy the four record fields.
  Restoring that local makes the retained helper exact and simultaneously
  aligns both copies expanded into the 1,047-byte equipped-slot handler. The
  sacrifice-specific war-machine warning is general-text row 483; correcting
  that last immediate makes `artifact_click` exact across all 36 blocks, 18
  branches and four returns. CodeView also corrects
  `hero::GetExperienceBonusFactor` to its original const member signature,
  retaining its exact retail body and migrating the claim at the same RVA.

  `backpack_click` has its first 25 semantic blocks exact. The remaining SP3
  inliner wall is bounded: this compile expands `update_all_slots` at both
  helper sites, while retail expands the pickup site and calls it at the final
  put-down site; retail also cross-jumps the first redraw into the common
  redraw where this compiler duplicates an epilogue. `predict-inline` reports
  exactly one over-inline and one under-inline. The DC nested if/else-if source
  shape is retained without a per-site steering pragma. The synchronized
  checkpoint reaches **1962/2379 linked exact**, **1893/2310 game exact**,
  **96.60% game fuzzy** and **44.16% executable coverage**. All 51 unit tests,
  five freshness controls, VC6 negative controls, link-order checks and fatal
  gates pass; the regenerated queue contains 417 residual functions / 29.5
  KiB recoverable and only the two known inlined-away `create_included_mask`
  diagnostics. No external implementation body was used.

- **2026-08-25 — the 617-byte sacrifice action is behavior-complete at a
  96.62% control-flow plateau.** The constructor callback store at 0x56019e
  and adjacent Dreamcast roster row identify
  `type_sacrifice_window::sacrifice` at 0x5646a0. Retail proves the artifact
  and creature reset paths, troop dismissal, slider/button reset and common
  experience award. The Dreamcast line table additionally proves that the
  artifact path calls the retained `put_down_artifact(false)` helper; its
  update-experience gate, offering refresh, pointer reset, slot refresh and
  redraw are reconstructed at their original helper boundary.

  Naming the original `armyGroup*` and loop-index locals restores retail's
  frame and register homes, taking the first transcription from 83.84% to
  96.62%. All eleven branch mnemonics and all three returns agree; one
  zero-slot branch landing exposes the remaining cross-jump. This SP3
  compile merges the helper's
  redraw with the creature-path redraw, while retail keeps the two identical
  six-instruction sites. Direct duplication, a shared-call spelling, an
  explicit join and forced inlining all preserve or worsen that wall, so the
  source-authentic closest spelling is banked. The synchronized checkpoint
  remains **1960/2376 linked exact** and **1891/2307 game exact**, with
  **96.60% game fuzzy** and **44.06% executable coverage**. No external
  implementation body was used.

- **2026-08-25 — the creature-mode and exit callbacks add 725 exact retail
  bytes.** Constructor callback stores at 0x560c82 and 0x560201, together
  with the adjacent Dreamcast private-static roster rows, identify
  `type_sacrifice_window::sacrifice_creatures` at 0x564910 and
  `type_sacrifice_window::exit_click` at 0x564a80. Retail proves their shared
  right-click help arm and the left-click `clear` expansion. The first then
  restores creature mode and redraws the dialog; the second writes the
  standard end-dialog message fields directly and forwards the message,
  rather than calling the virtual `ExitDialog` override.

  Retaining the natural `message&` callback ABI and spelling both paths in
  their source order reproduces all 48 blocks, 30 branches, six returns and
  every normalized instruction across the two functions. The synchronized
  checkpoint reaches **1960/2375 linked exact**, **1891/2306 game exact**,
  **96.60% game fuzzy** and **44.03% executable coverage**. No external
  implementation body was used.

- **2026-08-25 — `type_sacrifice_window::creature_click` reproduces all 916
  retail bytes.** The sole army-slot-widget caller forwards the retail
  `(long, unsigned char, unsigned char)` argument family, and the adjacent
  Dreamcast roster row plus its xrefs identify 0x564fe0. Retail proves both
  paths in full: a new left-click selection refreshes the old and new
  32-byte offering records, creature-name row, maximum sacrifice count,
  slider and max button; a repeated or right click opens the creature detail
  window, subtracting the offered amount for the left pane and using the
  quick-view pump only for a right click.

  Dreamcast block scopes and retail's EH-state numbering settle the otherwise
  non-obvious source order: the quick-view arm is lexical first even though
  VC6 lays the selection arm first. Removing cached creature-type/record
  values then restores retail's deliberate re-reads. Finally, repeating
  `maximum > 0` at the two enable calls lets VC6 retain one byte result in
  `BL`; a named temporary adds two instructions. The result matches all 47
  blocks, 26 branches, three returns and every normalized instruction. The
  synchronized checkpoint reaches **1958/2373 linked exact**, **1889/2304
  game exact**, **96.60% game fuzzy** and **44.00% executable coverage**. No
  external implementation body was used.

- **2026-08-25 — the sacrifice-artifacts callback reproduces all 356 retail
  bytes.** The constructor's callback pointer at 0x561bfe and the Dreamcast
  private-static signature identify 0x564e70. Retail then proves both
  source-inlined helpers in full: `clear` walks the 16-byte offering vector,
  calls `return_artifact` for each nonempty record and for the held artifact,
  clears their ids and resets `total_experience`; `return_artifact` tries the
  original equipped slot, any legal equipped slot, the backpack and one final
  arbitrary equipped-slot fallback in that order. The callback restores
  artifact mode and redraws the window after cleanup.

  The natural helper spelling and callback match all 24 blocks, 15 branches,
  three returns and every normalized instruction on their first measured
  compile. The synchronized checkpoint reaches **1957/2372 linked exact**,
  **1888/2303 game exact**, **96.59% game fuzzy** and **43.95% executable
  coverage**. No external implementation body was used.

- **2026-08-25 — the sacrifice creature controls add 599 exact retail bytes
  and recover one missing function boundary.** Retail 0x564c00 compares and
  rewrites one 32-byte offering record, adjusts `total_experience` by the
  old/new x87-rounded sacrifice values, refreshes that army slot and mirrors
  the amount into the selected record when necessary. The direct
  `set_creature_sacrifice` spelling reproduces all four blocks and 230 bytes
  on its first compile.

  The constructor's two callback pointers and Dreamcast's private-static
  signatures identify `all_creatures` at 0x564cf0 and `max_creatures` at
  0x564de0. The latter address had been absent from the bootstrap carve, but
  the retail pointer at 0x561962, its standalone entry, final `ret` at
  0x564e67 and eight alignment NOPs before the already carved 0x564e70 row
  prove the 136-byte boundary, now admitted to `retail-functions.tsv`.
  `get_max_amount` preserves one troop only when every other group is already
  offered to its limit; its early return and the all-slots callback's
  post-decrement loop are the two source shapes VC6 needs for the exact
  seventeen-block 233-byte body. The max callback expands the Dreamcast-
  attested `creature_slider_change` helper and matches all six blocks.

  The synchronized checkpoint reaches **1956/2371 linked exact**,
  **1887/2302 game exact**, **96.59% game fuzzy** and **43.93% executable
  coverage**. No external implementation body was used.

- **2026-08-25 — the sacrifice creature-offering refresh is instruction-
  exact across all 930 retail bytes.** Dreamcast supplies the retained method,
  its `type_creature_offering` parameter and the inline edges to
  `sacrifice_value` and the shared `GetArmyName` header helper. Retail
  independently proves the six widget fields, terminal group/amount pair,
  hero-army reads and the AI-value calculation `(AI_value / 40) * 5`.
  Reversing the floating multiply operands is source-significant under VC6:
  putting `GetExperienceBonusFactor` on the right evaluates it first and
  keeps its result live on the x87 stack while the integer sacrifice product
  is formed, exactly as retail does. The constant plural call to
  `GetArmyName` then folds its singular arm but retains the two creature-id
  bounds and shared empty-text fallback.

  All forty-two CFG blocks, every block size, all twenty symbolic branches
  and every ordinary instruction agree. The strict score is **99.9744%**;
  its flat diff contains only the established stripped-target relocation-name
  class (`basic_string` `_Nullstr`/`npos`/`assign`, three honest globals and
  synthetic EH records), with no executable source delta. The synchronized
  checkpoint remains **1953/2368 linked exact** and **1884/2299 game exact**,
  while game fuzzy stays **96.59%** and executable coverage rises to
  **43.91%**. No external implementation body was used.

- **2026-08-25 — both sacrifice mode switches add 911 exact retail bytes.**
  The 590-byte `set_artifact_mode` and 321-byte `set_creature_mode` are fixed
  by `DoModal`'s two arms and the Dreamcast roster; retail independently
  proves their complementary `artifact_widgets` / `creature_widgets`
  hide/show passes and every control pointer at +0x84..+0xb8. The artifact
  path clears the held offering, expands the exact slot/offering/backpack
  refresh helpers, configures the backpack and mode buttons, then refreshes
  experience. Its direct spelling matches all twenty-nine blocks.

  Dreamcast's seven-element 224-byte array fixes each creature-offering
  record at 32 bytes. Retail's retained updater proves six widget pointers
  followed by `group` and `amount` at +0x18/+0x1c; the creature path seeds
  those seven records, clears the current record, updates the slider/button
  state and matches all eleven blocks. The DC header-inline `widget::hide`
  and `show` bodies are exposed only to this owning TU because unconditional
  header definitions perturb VC6's global inline budget even when unused.
  The synchronized checkpoint reaches **1953/2367 linked exact**,
  **1884/2298 game exact**, **96.59% game fuzzy** and **43.86% executable
  coverage**. No external implementation body was used.

- **2026-08-25 — the sacrifice experience/destructor checkpoint adds 726
  exact retail bytes.** Dreamcast supplies the adjacent names, signatures
  and source order; retail independently identifies `update_experience` at
  0x562500 through its packed hero +0x51/+0x55 reads and sole
  `hero::GetExperience` call. The direct source computes experience to the
  next level, formats that value and `total_experience`, updates the two
  adjacent text widgets and enables `sacrifice_button` iff the total is
  positive. All twenty blocks of the 346-byte body match without a compiler
  control.

  Vtable 0x641620 slot zero fixes the 33-byte deleting wrapper at 0x560350
  and its 347-byte destructor callee at 0x5623a0. The destructor's reverse
  storage loads at +0x230/+0x220/+0x110/+0x100/+0xf0/+0xe0 prove the six
  still-opaque vector positions; the remaining two vector destructors and
  final `CAdvPopup` call close the complete eight-member teardown. A plain
  `delete_widgets()` body then reproduces the destructor and generated
  wrapper exactly. `convert_with_commas` remains in Dreamcast source order,
  while an annotated redeclaration records its later retained-COMDAT address
  in retail. The synchronized checkpoint reaches **1951/2365 linked exact**,
  **1882/2296 game exact**, **96.58% game fuzzy** and **43.82% executable
  coverage**. No external implementation body was used.

- **2026-08-25 — the sacrifice equipped-slot refresh pair adds 409 exact
  retail bytes.** The 51-byte `update_all_slots` is fixed by its sole callee
  and two artifact-mode callers. Complete selects eighteen or nineteen
  equipped positions with the established `gpGame->f_1f698 >= 2` Shadow of
  Death gate, then forwards each index to the adjacent one-slot updater; the
  direct loop was exact on its first spelling.

  The 358-byte `update_slot` is independently called from twelve retail
  sites, and Dreamcast supplies its name, ABI, helper graph and one local
  `type_artifact`. Retail proves the packed hero artifact read at +0x12d and
  the sacrifice window's next two vector `_First` loads at +0xf0/+0x100,
  admitting `slot_back_widgets` and `slot_widgets` at +0xec/+0xfc. A held
  artifact legal in the slot moves the equipped icon to the back layer and
  paints drop frame 0x90 in front; otherwise the front layer shows the
  equipped artifact, with an empty slot taking its name from
  `akArtifactSlotTraits`. The last fallback is after the held-artifact
  branch, not inside its `else`: that retail edge lets VC6 share one
  `set_help_text` tail and closes all twelve blocks. One explicit
  `inline_depth(0)` preserves the nested `set_visible` edge that retail kept
  after expanding `update_artifact_widget`; the reduced live TU otherwise
  has spare inline budget that the original 211-function compiland did not.
  The synchronized checkpoint reaches **1948/2362 linked exact**,
  **1879/2293 game exact**, **96.58% game fuzzy** and **43.78% executable
  coverage**. No external implementation body was used.

- **2026-08-25 — the sacrifice artifact-offering chain adds 810 exact
  retail bytes.** Dreamcast supplies the names, signatures and order of the
  static `convert_with_commas` / `update_offering` pair; retail independently
  fixes their x86 identities and behavior. `convert_with_commas` formats a
  signed decimal and walks the Dinkumware string backward with a
  post-decrement index, inserting a comma every third digit. Its 29-block
  flow matched immediately; declaring the long-lived digit counter before
  the string gives VC6 retail's EDI allocation and makes all 469 bytes exact.

  `update_offering` expands the already-proven artifact-widget updater,
  formats the offered value through that helper, and toggles the paired icon
  and text widgets. Retail HELP.TXT loader 0x5b9b52 independently proves that
  0x6a6638 is a twenty-row `THelpText` table: it writes stride-8 pairs through
  0x6a66d7, while the updater reads rows 14 and 15. Admitting the array rather
  than inventing two standalone pointers makes the natural 292-byte body
  exact on its first comparison. Finally, the 49-byte
  `type_sacrifice_window::update_artifact_offering` wrapper directly proves
  the three adjacent VC6 vector `_First` loads at object offsets
  +0xc0/+0xd0/+0xe0; the DC member roster and the already-proven eight-byte
  base delta fix the corresponding vector objects at +0xbc/+0xcc/+0xdc.
  The synchronized checkpoint reaches **1946/2360 linked exact**,
  **1877/2291 game exact**, **96.58% game fuzzy** and **43.76% executable
  coverage**. No external implementation body was used.

- **2026-08-25 — the two town special-building market entries add 628
  exact retail bytes.** `townManager::Main` independently fixes both
  identities and Complete-era ABIs. Its building-17 arm admits only town
  types 2, 5 and 8 (Tower, Dungeon and Conflux) before calling 0x5e9d20,
  which is the DC-roster `DoArtifactMerchants`; the retail body retains the
  no-argument form and reloads `gpTownManager->townToView` after the inlined
  market count. Removing a source local that crossed those paths eliminated
  an EBP frame and made all 313 bytes exact.

  The neighboring call loads `townToView` into ECX and selects 0x5e9ea0
  only for town type 6 (Stronghold), proving a new
  `DoFreelancersGuild(town*)` overload beside the already matched
  adventure-object `hero*` form. The 315-byte body was exact on its first
  natural spelling. Both no-hero arms inline DC's attested
  `TTextResource::operator[]`, use retail general-text row 274, and name the
  building through the 10x11 `gSpecialBuildingNames` table populated by
  retail `InitializeSpecialBuildingText` from `bldgspec.txt`. The synchronized
  checkpoint reaches **1943/2357 linked exact**, **1874/2288 game exact**,
  **96.58% game fuzzy** and **43.72% executable coverage**. No external
  implementation body was used.

- **2026-08-25 — artifact-slot rendering and two market entry points add
  367 exact retail bytes.** The 91-byte free helper at 0x5639e0 has the
  Dreamcast `update_artifact_widget(iconWidget*, type_artifact)` ABI and
  xrefs, while retail independently proves the eight-byte by-value record,
  empty-slot visibility/help reset, icon-frame selection and
  `akArtifactTraits` name lookup. The direct source form is byte-exact.

  In tradpost.obj, retail's event-dispatch case for object 213 (Freelancer's
  Guild) passes the active hero in ECX to 0x5e9e60. That proves Complete
  changed Dreamcast's no-argument entry to a one-hero fastcall; its five
  market-state stores and tail into `DoMarket` reproduce all 56 bytes on the
  first spelling. The adjacent 220-byte `DoMarketplace` retains Dreamcast's
  no-argument identity and expands the DC `CountMarkets` helper: it counts
  the acting player's active Marketplace bits, caps efficiency at ten,
  selects the displayed town's visiting hero, and opens the resource-trade
  pane. Using `built` first gave an otherwise instruction-identical
  99.9688%; retail's +0x158 read proves `active`, which closes the body.
  The synchronized checkpoint reaches **1941/2355 linked exact**,
  **1872/2286 game exact**, **96.57% game fuzzy** and **43.69% executable
  coverage**. No external implementation body was used.

- **2026-08-25 — `type_sacrifice_window::ExitDialog` reproduces all 128
  retail bytes.** Vtable 0x641620 slot 14 fixes the body at 0x565430 as the
  sacrifice dialog's protected exit handler, and the Dreamcast roster
  independently supplies its name and signature. Dreamcast class/member
  records prove that `type_artifact_offering` is a 16-byte `type_artifact`
  derivative with `source` and `value` at +8/+12, and place the dialog's
  `holding_artifact`, `sacrificing_artifacts` and
  `can_sacrifice_artifacts` consecutively. Widening the proven eight-byte
  CAdvPopup base difference maps them to retail +0x64/+0x74/+0x75; retaining
  the otherwise-unused +0x74 byte also preserves the already-exact `DoModal`.
  Retail returns a held artifact to its original slot when that slot is below
  19, then tries an arbitrary equipped slot, the backpack, and a final
  arbitrary-slot fallback before clearing the artifact id and closing the
  dialog. A named pointer to the held record gives VC6 retail's ESI/EDI
  allocation, while nesting both equipped-slot attempts under the source
  bound reproduces the last branch edge. The synchronized checkpoint reaches
  **1938/2352 linked exact**, **1869/2283 game exact**, **96.57% game fuzzy**
  and **43.67% executable coverage**. No external implementation body was
  used.

- **2026-08-25 — the adventure hero locator is exact and the first
  stream-adapter `Write` COMDAT is admitted.** `UpdateHeroLocator`'s sole
  residual was retail's branch over `WIDGET_SET_STATUS`/`WIDGET_CLEAR_STATUS`;
  the direct ternary let VC6 fold the adjacent values into arithmetic. The
  branch is the source expansion of Dreamcast Widget.h:263
  `widget::set_visible(unsigned char)`. Exposing that header inline only to
  adventuremapwindow.obj reproduces all 574 retail bytes and retires the
  98.5808% plateau without a control-flow carrier.

  The tractable-span census also exposed the five-byte body at 0x559140.
  Both retail resource-file-adapter vtables point their slot-2 `Write`
  methods at `xor eax,eax; ret 8`; the compiled resourcemanager.obj contains
  both matching inline publics, and the linker keeps the first stdio-adapter
  COMDAT while folding the LOD adapter and CHeroWindowEx default handler onto
  it. A claim-only carcass declarator names the already-emitted header body
  without moving code. The synchronized checkpoint reaches **1937/2351
  linked exact**, **1868/2282 game exact**, **96.57% game fuzzy** and
  **43.66% executable coverage**. No external implementation body was used.

- **2026-08-24 — the 1,491-byte adventure cheat dispatcher is fully
  reconstructed at a bounded 53.5182% compiler plateau.** The retail entry at
  0x402450, its sole retail caller, and Dreamcast `adventuremapwindow.cpp:63`
  establish `CheckAdvCheatCode`, its `std::string&` ABI and local
  `TCheatCode`. Retail fixes all sixteen encoded comparisons and their order:
  the two seven-slot army fills, three war-machine grants, experience award,
  luck/movement/morale flags, puzzle view, reveal/hide-map loops, seven-resource
  award, both end-game outcomes, spellbook/all-spells grant, build-everything
  toggle and the paired palette transforms. It also fixes the shared accepted
  text (general-text row 261) and both cheat latches. The opening 41 compiled
  blocks agree almost instruction-for-instruction; the inliner oracle reports
  equal total out-of-line call counts and isolates the plateau to two VC6
  decisions: our compiland expands two nested `basic_string::_Tidy` calls that
  retail keeps, and merges one of the phisher-price branch's identical redraw
  sites. Direct assignment, counted assignment, named-source/wrapper surfaces,
  `inline_depth(1/2/4)` and explicit branch returns were measured and byte-flat,
  so the natural source is retained without exposing Dinkumware internals. The
  synchronized checkpoint reaches **1935/2350 linked exact**, **1866/2281 game
  exact**, **96.57% game fuzzy** and **43.66% executable coverage**. All 51
  unit tests, five freshness controls, VC6 negative controls, link-order gates
  and fatal gates pass; the regenerated queue contains 415 residual functions
  / 29.4 KiB recoverable and only the two known inlined-away
  `create_included_mask` diagnostics. No external implementation body was
  used.

- **2026-08-24 — the shared cheat-code encoder is exact and the combat cheat
  dispatcher reaches a bounded 90.2966%.** Dreamcast Game.h proves the
  200-byte `TCheatCode`, its 200-byte `code` array, two substitution-alphabet
  pointers and the constructor/compare/encode surface. Retail independently
  places the shared encoder at 0x402a30 through calls from both the adventure
  and combat cheat dispatchers. Preserving the lowered `min(strlen,199)` as
  two locals plus a selected address reproduces all eight blocks and all 161
  bytes. `CCombatChatEdit::SendChat` is the sole retail caller of
  `CheckCombatCheatCode` at 0x472010; its three encoded literals, current-side
  hero lookup, Blue Pill/Red Pill battle termination, spellbook/all-spells
  grant, general-text row 261 and game/campaign cheat latches agree with the
  DC line/xref map and retail bytes. Its complete 28-block CFG is identical.
  The inliner oracle isolates the remaining delta to one call: our compiland
  expands `_Tidy(false)` inside the final string assignment while retail calls
  it, cascading into opposite EBX/ESI homing. Direct operator, assign,
  counted-assign and one-level wrapper surfaces are byte-flat, so the natural
  source is retained without exposing Dinkumware internals. The synchronized
  checkpoint reaches **1935/2349 linked exact**, **1866/2280 game exact**,
  **96.64% game fuzzy** and **43.62% executable coverage**. No external
  implementation body was used.

- **2026-08-24 — the combat chat editor adds five exact rows and one bounded
  nested-inline plateau.** Vtable 0x63d4bc fixes the four ordinary methods at
  0x472600..0x472850; Dreamcast `combatwindow.cpp:143-207` independently
  supplies their names, signatures and statement order. `OnKeyPress`
  reproduces all 165 bytes by treating Tab as the inactive editor's activation
  key and otherwise delegating to `CChatEdit`; `OnEscape` and `UpdateScreen`
  reproduce all 94 and 61 bytes. The retail five-byte
  `CCombatChatEdit::~CCombatChatEdit` is specifically VC6's implicit derived
  destructor: spelling an empty body inserts a derived-vtable store, while the
  established `IMPLICIT_DTOR` claim preserves the exact tail jump into
  `CChatEdit::~CChatEdit`. `TCombatWindow`'s adjacent 33-byte scalar-deleting
  destructor is exact under the existing compiler-generated claim. `SendChat`
  at 0x4726b0 reaches **93.3486%** with all 16 CFG blocks and every instruction
  after string construction identical. Its sole real code delta is VC6
  inlining `basic_string::_Eos` at the end of the c-string constructor where
  retail calls it; `inline_depth(1)` is byte-flat and `inline_depth(0)` also
  suppresses the required outer constructor inline, regressing to 70.82%, so
  the measured compiler-state residual is retained without a source hack. The
  synchronized checkpoint reaches **1934/2347 linked exact**, **1865/2278 game
  exact**, **96.65% game fuzzy** and **43.59% executable coverage**. No
  external implementation body was used.

- **2026-08-24 — `TCombatWindow::TCombatWindow` reproduces all 1,066 retail
  bytes.** `combatManager::Open` is the sole retail caller and supplies the
  placement byte; Dreamcast `combatwindow.cpp:221` independently supplies the
  identity, signature and constructor call graph. Retail fixes the 0x8c-byte
  window, three initial widgets, the 0x74-byte combat chat editor, placement
  versus control bar, two hero panels and four 0x70-byte creature panels,
  including every coordinate and registration order. The natural source was
  immediately 99.6685% with identical 50-block flow. The only semantic delta
  was construction order inside the private chat editor: making its +0x70
  byte a member initializer moves that store ahead of the derived vptr store,
  exactly as retail does. The synchronized checkpoint reaches **1929/2341
  linked exact**, **1860/2272 game exact**, **96.65% game fuzzy** and
  **43.56% executable coverage**. All 51 unit tests, five freshness controls,
  link-order checks and fatal gates pass; the all-unit queue remains 412
  residual functions / 28.6 KiB recoverable and emits only the two known
  inlined-away `initialize.obj` `create_included_mask` diagnostics. No
  external implementation body was used.

- **2026-08-24 — the retail-only mine help-text helper at 0x40d670 is
  reconstructed to 97.3418%.** Its two MINE callers prove the five-parameter
  `/Gr` ABI and compact/full modes; retail independently proves the 64-byte
  mine record, ordinary/abandoned description table, owner and same-team
  resource annotations, and guard-army formatter. Using the returned string
  as a direct temporary moved the body from 91.0717% to 96.3924%, and retail's
  symmetric player/owner `OnSameTeam` argument order reaches 97.3418%. Both
  sides have 17 blocks, 10 branches and two returns, with every instruction
  from the team comparison onward exact. The remaining prelude swaps EAX/EDI
  for the owner and player. The guided nine-mutation register sweep found no
  gain, and the allocator model sees identical first definitions, classifying
  it as late register homing rather than missing semantics. No external
  implementation body was used.

- **2026-08-24 — `TCombatWindow::combat_message` adds 862 exact retail
  bytes and closes the attributed combat-window in-span backlog.** The
  Dreamcast statement map preserves the four live-combat guards, transient
  rollover arm, kept-message construction, newline split, two-line display
  cap, and final `show_messages` range. Retail fixes the combat-manager
  guard offsets, `gUnnamed698a08` small-font measurement, and the +0x54
  `vector<string*>` already proven by scrolling and destruction. The direct
  source form reproduces VC6's inlined string/vector helpers; the one
  control-flow trap was the final null-control-bar check. A positive `if`
  merged its cleanup tail and measured 91.6053%, while the DC-consistent
  early return restores retail's fourth epilogue. All 26 branches and all
  862 bytes agree. No external implementation body was used.

- **2026-08-24 — `TCombatWindow::ProcessRightSelect` adds 292 exact
  retail bytes.** The Dreamcast line map preserves the call to the static
  `convertID2HelpID`, its explicit negative-ID rejection, text sizing, and
  centered type-4 dialog. Retail folds the helper into its only caller and
  uses the same eleven-row `THelpText` table at 0x6a6968 that the combat
  sub-window constructors partition: ids 0x7d1..0x7da map to rows 0..8
  (the two log arrows share row 5), while placement ids 0x8fc and 0x7802
  map to rows 9 and 10. Omitting the source helper's explicit `id < 0`
  arm measured 97.7528% because VC6 folded negatives into the switch
  default; restoring it reproduces all five retail branches, the ten-entry
  jump table, and all 292 bytes. No external implementation body was used.

- **2026-08-24 — `TCombatWindow::~TCombatWindow` adds 334 exact retail
  bytes.** The constructor's direct store to retail .bss 0x695000 and four
  chat-callback reads prove the compiland-local `gpCombatWindow` pointer;
  the destructor clears that same slot. Dreamcast lines 293-313 preserve
  the teardown phases and ownership: delete the polymorphic control bar,
  walk and delete the inherited `Widgets`, walk and delete every owned
  message string, then clear the callback pointer. Retail extends the DC
  tail from two panels to the byte-proven two `TCombatHeroSubWindow` and
  four `TCombatCreatureSubWindow` fields at +0x74..+0x88. The resulting
  VC6 body reproduces all 334 bytes. The semantic viewer differs only in
  synthesized EH, vtable, and data-relocation label spellings; the
  authoritative normalized comparison reports 100.0%. No external
  implementation body was used.

- **2026-08-24 — three combat-window display functions add 613 exact retail
  bytes.** `show_messages` reproduces all 371 bytes from Dreamcast lines
  431-450 and retail's +0x54 message vector: it selects up to two strings,
  joins them with a newline, updates the control bar's visible range and
  rollover, and stamps the +0x6c clock. Its 27 blocks and fourteen branches
  agree. `EndPlacementPhase` reproduces 153 bytes by deleting the placement
  bar, constructing the normal `TCombatControlSubWindow`, dispatching a full
  vslot-5 redraw, and posting 800x600; all six blocks and two branches agree.
  Finally, vtable 0x63d528 slot 5 proves the 89-byte body immediately after
  `DrawChatText` is `TCombatWindow::DrawWindow`, not the earlier DC roster's
  standalone `DrawChatEdit`: retail inlines that helper after the base draw,
  drawing and posting the focused chat editor. All five blocks and three
  branches agree. The semantic viewer names the two EH-handler relocations
  differently from the synthesized target, but the authoritative normalized
  VC6 comparison reports 100.0% for both EH-bearing bodies. No external
  implementation body was used.

- **2026-08-24 — combat-window hover and message scrolling add 167 exact
  retail bytes.** Vtable 0x63d528 slot 4 fixes 0x472b80 as
  `TCombatWindow::handle_widget_hover`; Dreamcast source-line records preserve
  its inlined help-text accessor, the active-message guard with its two widget
  ID exemptions, and the null/non-null `set_rollover` arms. The x86 body
  independently resolves the former +0x4c pad as the chat editor by reading
  its already-proven +0x6d `bHasFocus` byte, and resolves +0x70 as the common
  polymorphic combat-control/placement subwindow. All twelve blocks, nine
  branches, and 103 bytes agree. The adjacent 64-byte
  `TCombatWindow::scroll_rollover` is fixed by Dreamcast lines 456-469 and its
  sole message-handler caller. Restoring the real four-word
  `vector<string*>` at +0x54 makes VC6 emit retail's null-aware inline `size`,
  followed by the exact size-minus-two and zero clamps and `show_messages`
  call; all ten blocks and four branches agree. No external implementation
  body was used.

- **2026-08-24 — `TCombatWindow::ClearCombatMessages` reproduces all 54
  retail bytes.** Dreamcast `combatwindow.cpp:417` places the method between
  `handle_widget_hover` and `show_messages`; retail independently proves the
  identity through its sole combat-manager caller, the +0x64 message-count
  latch, the +0x6c clock, and its call to the three-argument
  `combat_message`. Splitting the previously opaque class tail exposes those
  two fields without changing the proven 0x8c-byte layout. The existing
  header-inline `GameTime::ElapsedSince` is also byte-significant: it evaluates
  the timestamp into EDI before calling `GameTime::Get`, reproducing the
  retail schedule that a hand-written subtraction cannot. All four blocks,
  both branches, and 54 bytes agree. No external implementation body was
  used.

- **2026-08-24 — `HandlePlayerWon` and `HandlePlayerLost` reproduce all 94
  and 75 retail bytes.** Their dispatcher slots, payload offsets, helper
  calls and Dreamcast `remote.cpp:2419/2441` rows establish both identities
  and ABIs. Each passes the embedded victory or loss condition to the
  corresponding display helper and updates the all-players-defeated latch
  from its two output locals; the win handler additionally copies the live
  victory record and raises the game-over latch. CodeView fixes the locals as
  `bGameLost` followed by `bGameWon`, while retail fixes their initialization
  schedule in the opposite order. Keeping the declaration order but spelling
  separate `bGameWon = 0; bGameLost = 0;` assignments reproduces all five
  blocks and every instruction in both functions. The synchronized
  checkpoint reaches **1919/2330 linked exact**, **1850/2261 game exact**,
  **96.63% game fuzzy** and **43.36% executable coverage**. No external
  implementation body was used.

- **2026-08-24 — `combatManager::DrawBackground` reproduces all 427 retail
  bytes, and `DrawWallAt` advances to a bounded 99.95%.** Drawing order, the
  zero-argument ABI and Dreamcast `drawing.cpp:919` establish the background
  row at 0x493cf0. It loads the selected battlefield, composites Complete's
  optional elevation and town-moat layers, caches the battlefield viewport,
  rebuilds the grid, then posts the finished 800x556 bitmap to the window
  manager. The reconstructed CFG and schedule were already complete, but
  caching only the elevation index made VC6 retain `(table offset, bitmap)`
  in `(EDI, EBX)` instead of retail's `(row pointer, bitmap)` in `(EBX,
  EDI)`. Reusing the selected row for `FileName`, x and y gives the optimizer
  the original value lifetime and reproduces every instruction; the const
  `IsQuickCombat` overload independently fixes the first call relocation.
  The same pass reshapes `DrawWallAt`'s inline archer choice as a conditional
  expression, changing its otherwise redundant final lower-tower test to
  retail's `jne` and raising 99.73% to 99.95%. Its 54 blocks and symbolic
  branch sequence now agree; only the byte-distinct but source-equivalent
  ordering of the main-keep and upper-tower pointer blocks remains. The
  synchronized checkpoint reaches **1917/2330 linked exact**, **1848/2261
  game exact**, **96.63% game fuzzy** and **43.36% executable coverage**. No
  external implementation body was used.

- **2026-08-24 — `combatManager::DrawMoatOverlay` reproduces all 471 retail
  bytes.** Its two `DrawOccupant` callers, one-argument ABI and Dreamcast
  `drawing.cpp:2019` row establish the identity. The method intersects the
  selected hex's six-pixel lower strip with the town-specific moat image and
  combat viewport, participates in dirty-extent capture and limited drawing,
  then blits the surviving rectangle. The arithmetic, temporary layout and
  final `Bitmap816::Draw` call were already instruction-identical, but the
  nested reconstruction had one shared failure epilogue and scored only
  24.94%. Retail's five-return CFG proves four flat early-failure guards;
  spelling those directly reproduces all 45 blocks and every byte on the
  first compile. The synchronized checkpoint reaches **1916/2330 linked
  exact**, **1847/2261 game exact**, **96.63% game fuzzy** and **43.36%
  executable coverage**. No external implementation body was used.

- **2026-08-24 — the hex-target `combatManager::SpellEffect` overload
  reproduces all 573 retail bytes.** The adjacent drawing-order row at
  0x496a10, its four-argument `ret 0x10`, two retail callers and Dreamcast
  `drawing.cpp:2593` entry establish the identity. Dreamcast additionally
  supplies the by-value `TSpellEffectTraits` local, frame local, statement
  boundaries and `CSprite::GetWidth`/`GetHeight` inline sources; retail fixes
  Complete's 83-effect bound and Immersion cue. Placement modes 0, 1 and 4
  anchor the sprite above, centrally on, or at the corner of a combat hex.
  Each frame redraws the clean battlefield, blits the effect directly to the
  screen, refreshes the combat viewport, and optionally leaves the last frame
  posted. The decisive source detail is the switch's deliberate lack of a
  fallback assignment: inventing one scored 90.64%; removing it restored
  retail's case layout, and the two DC-attested size accessors closed the last
  register-scheduling delta from 96.12% to exact. All 25 CFG blocks and every
  normalized instruction agree. The synchronized checkpoint reaches
  **1915/2330 linked exact**, **1846/2261 game exact**, **96.59% game fuzzy**
  and **43.34% executable coverage**. No external implementation body was
  used.

- **2026-08-24 — the army-target `combatManager::SpellEffect` overload
  reproduces all 453 retail bytes.** Drawing order and the four-argument
  `ret 0x10` row place it at 0x496840; Dreamcast `drawing.cpp:2524` supplies
  the name, signature, optimized `frame` local and statement skeleton.
  Retail independently fixes Complete's 83-effect bound and adds the
  Immersion force-feedback cue absent from Dreamcast. The method rejects
  quick combat and invalid or resource-less effects, caches the overlay,
  optionally advances the stack's wince sequence in lockstep with it, then
  finishes the remaining overlay frames and clears the stack's draw flag.
  The natural source shape reproduces all 44 CFG blocks and every normalized
  instruction on its first compile, including VC6's reuse of the wince frame
  counter for the remaining effect pass. The synchronized checkpoint reaches
  **1914/2329 linked exact**, **1845/2260 game exact**, **96.59% game fuzzy**
  and **43.31% executable coverage**. No external implementation body was
  used.

- **2026-08-24 — `combatManager::CycleCombatScreen` reproduces all 1,898
  retail bytes.** The 0x4960d0 drawing-order anchor, its zero-argument ABI and
  Dreamcast `drawing.cpp:2257` row establish the identity; the DC line table
  additionally supplies all six optimized locals and the header-inline edges
  to `army::Is`, `IsIncapacitated`, `is_in_area_highlight`, `GameTime` and
  `CSprite::GetNumFrames`. Retail fixes Complete's tower-aware
  `MarkCreatureEffect` expansion, both local-human combat-hero triggers, the
  40-stack fidget walk, palette cycles and 100 ms frame pacer. Two source
  details were decisive: the invalid-player cleanup is the structured `else`
  of each positive local-human arm, and DC lines 2309/2311 put
  `CyclingCreatures = 0` before the explicit 40-byte `memset`; VC6 schedules
  that source order into retail's otherwise surprising zeroing preheader.
  The result reproduces all 111 CFG blocks and every normalized instruction.
  The synchronized checkpoint reaches **1913/2328 linked exact**,
  **1844/2259 game exact**, **96.58% game fuzzy** and **43.29% executable
  coverage**. All 51 unit tests, five freshness controls, link-order checks
  and fatal gates pass; the all-unit VC6 queue emits only the two known
  inlined-away `initialize.obj` `create_included_mask` diagnostics. No
  external implementation body was used.

- **2026-08-24 — `combatManager::ComputeMaxExtent` reproduces all 862 retail
  bytes.** The global anchor at 0x495bf0, its zero-argument signature, the
  `DrawFrame`/`CycleCombatScreen` callers and the Dreamcast
  `drawing.cpp:2093` row establish the identity. Dreamcast preserves four
  source walks and the exact callee family: two 20-stack effect rows, the two
  hero/flag pairs, the placed-obstacle vector and three siege archers, followed
  by `SLimitData::Clip`. Retail independently fixes Complete's 1,352-byte army
  stride, 24-byte obstacle rows, 36-byte archer rows, all screen-coordinate
  constants and the four-byte hero/flag effect band. The decisive source
  shapes were to re-subscript the army instead of caching a reference, retain
  the obstacle's explicit 42-pixel `yOffset`, and bind each archer through a
  block-scoped reference; together they reproduce all 43 CFG blocks and every
  normalized instruction. The synchronized checkpoint reaches **1912/2327
  linked exact**, **1843/2258 game exact**, **96.58% game fuzzy** and **43.20%
  executable coverage**. All 51 unit tests, five freshness controls, link-order
  checks and fatal gates pass; the all-unit VC6 queue emits only the two known
  inlined-away `initialize.obj` `create_included_mask` diagnostics. No external
  implementation body was used.

- **2026-08-24 — `combatManager::UpdateMouseGrid` and its static cleanup
  reproduce 1,258 retail bytes exactly.** Drawing order, the four retail
  callers and Dreamcast `drawing.cpp:982` identify 0x493ea0; Complete's
  `ret 0xc` and call-site pushes prove the added trailing byte is a forced-
  refresh flag. The body restores the old shaded cells from 45-pixel atlas
  lanes, saves clean backgrounds for the new set, darkens those cells, unions
  and clips both dirty regions, posts the framed rectangle, then replaces its
  function-local `old_hexes` vector. Retail independently proves the 19-byte
  lane-use table, DC's `background_offset` member at `hexcell+0x4d`, the
  `iLastMouseGridIndex` datum and the adjacent atexit cleanup at 0x494370.
  The decisive source boundary was DC's inline edge to
  `UpdateCombatArea(SLimitData)`: a direct `UpdateScreen` call emitted the
  same visible work but let VC6 over-inline `vector<long>::_Destroy` out of
  `clear()`. Restoring the adapter, with the const-reference form selected by
  Complete's bytes, preserves retail's empty out-of-line `_Destroy` and makes
  all 66 CFG blocks / 1,226 body bytes exact; its 32-byte static destructor is
  independently exact and is now admitted to the canonical function
  inventory. The synchronized checkpoint reaches **1906/2319 linked exact**,
  **1837/2250 game exact**, **96.60% game fuzzy** and **42.99% executable
  coverage**. All 51 unit tests, five freshness controls and every fatal gate
  pass. The all-unit queue remains 413 residual functions / 28.6 KiB
  recoverable; the ordinary tractable tier falls to **212 functions / 174.5
  KiB**, and `drawing` has one unclaimed row / 2,005 bytes left. No external
  implementation body was used.

- **2026-08-24 — `combatManager::UpdateGrid` reproduces all 960 retail
  bytes.** The retail row at 0x493930 is fixed by the drawing order map, its
  `ret 8`, the unique `SetupGridForArmy` edge and the same six-callee family
  as Dreamcast `drawing.cpp:755`. It optionally seeds the acting stack's
  requested hexes, restores the changed rectangle from the clean battlefield,
  darkens the new selection, draws the visible grid overlay and copies the new
  187-byte state to the posted row. Retail independently proves the two state
  rows, 112-byte cell stride, `GridAreaLimits` rectangle and private posted-
  grid latch. The decisive matching boundary was the DC inline chain
  `hexcell::limits` -> `SLimitData::Include` -> `Clip` -> `Width`/`Height`:
  spelling the equivalent comparisons by hand preserved all 84 CFG blocks but
  plateaued at 94.34%; restoring those natural helpers made VC6 emit every
  instruction and byte exactly. The synchronized checkpoint reaches
  **1904/2317 linked exact**, **1835/2248 game exact**, **96.60% game fuzzy**
  and **42.93% executable coverage**. All 51 unit tests, five freshness
  controls and every fatal gate pass. The all-unit queue remains 413 residual
  functions / 28.6 KiB recoverable; the ordinary tractable tier falls to
  **213 functions / 175.7 KiB**, with two remaining `drawing` rows / 3,231
  bytes. No external implementation body was used.

- **2026-08-24 — `combatManager::DrawWallAt` reconstructs all siege-wall
  behavior and reaches a bounded 99.20% plateau.** Retail 0x494c20 has the
  two-argument `ret 8`, three `DrawWall` calls and one `DrawArcher` call of
  the Dreamcast method, independently correcting the stale order-only map.
  Its eighteen defending-town rows select the standing wall image, clip
  ordinary sections against the current battlefield hex, and layer the keep
  or tower archer beneath its foreground cover. The DC `hexcell` reference
  and explicit archer x/y locals reproduce retail's frame, register pressure,
  both facing paths and every call argument. The normalized flat-asm diff has
  one real residual: retail retains a repeated `cmp eax,0x10; jne` in the
  three-way archer selector, for 33 branches against the reconstruction's 32;
  VC6 folds it from every reviewed structured equivalent. All other reported
  deltas are cosmetic names for the still-unclaimed `DrawWall` and creature-
  traits relocations. The synchronized checkpoint reaches **1903/2316 linked
  exact**, **1834/2247 game exact**, **96.60% game fuzzy** and **42.88%
  executable coverage**. All 51 unit tests, five freshness controls and every
  fatal gate pass. The all-unit queue contains 413 residual functions / 28.6
  KiB recoverable (including this plateau); the ordinary tractable tier falls
  to **214 functions / 176.6 KiB**, with three remaining `drawing` rows /
  4,191 bytes. No external implementation body was used.

- **2026-08-24 — `combatManager::DrawOccupant` corrects a stale drawing
  map and reproduces all 327 retail bytes.** The old order-only hypothesis
  assigned 0x494c20 to `DrawObstacleAt` and 0x494f40 to `DrawWallAt`.
  Retail independently rejects both assignments: 0x494c20 has `ret 8` and
  the three `DrawWall` plus `DrawArcher` calls of the two-argument
  `DrawWallAt`, while 0x494f40 has `ret 0xc`, is called by `DrawFrame`, and
  preserves the complete Dreamcast `DrawOccupant` callee set. The latter
  validates its index, filters the resident army by draw priority, invisibility
  and facing, draws it once, layers the inner or optional outer moat under both
  cells of a wide creature, then redraws the army above the moat. Its row
  calculation and wide-creature front offset inline exactly as retail does;
  all 21 CFG blocks and every normalized byte agree on the first post-delink
  comparison. The source map now leaves `DrawObstacleAt` DC-only and places
  `DrawWallAt` at 0x494c20. The synchronized checkpoint reaches **1903/2315
  linked exact**, **1834/2246 game exact**, **96.59% game fuzzy** and
  **42.84% executable coverage**. All 51 unit tests, five freshness controls
  and every fatal gate pass. The all-unit queue remains 412 residual functions
  / 28.6 KiB recoverable; the ordinary tractable tier falls to **215 functions
  / 177.4 KiB**, including four remaining `drawing` rows / 4,987 bytes. No
  external implementation body was used.

- **2026-08-24 — `combatManager::DrawSpellEffect` and
  `DrawSpriteObject` reproduce all 324 and 322 retail bytes.** The adjacent
  rows at 0x4953b0 and 0x495500 preserve the Dreamcast caller edges and end in
  their uniquely named `CSprite::DrawSpellEffect` and `CSprite::Draw` calls,
  independently proving both identities. Each constructs an inclusive sprite
  rectangle, clips it to the 0x694f18 combat viewport, optionally accumulates
  it into the manager's dirty extent, rejects disjoint or suppressed draws,
  and uses the clipped bottom edge as the source height. The first forwards
  flip and alpha bytes to the spell-effect blitter; the second enables the
  ordinary sprite blitter's transparency flag. A direct field spelling
  reached 99.91% for DrawSpellEffect with one interchangeable SIB encoding.
  Restoring the DC-attested four-argument `SLimitData` constructor changed
  only that source expression's compiler lineage and reproduced retail's
  final byte; the same constructor spelling then made DrawSpriteObject exact
  on its first compile. All 26 CFG blocks agree in each body. Their asm reports
  contain only the cosmetic four-lane names for the source-owned combat
  viewport aggregate, and the normalized exact-byte verdicts are clean. The
  synchronized build/delink/build checkpoint reaches **1902/2314 linked
  exact**, **1833/2245 game exact**, **96.59% game fuzzy** and **42.82%
  executable coverage**. All 51 unit tests and every fatal gate pass. The
  all-unit queue remains 412 residual functions / 28.6 KiB recoverable; the
  ordinary tractable tier falls to **216 functions / 177.7 KiB**. No external
  implementation body was used.

- **2026-08-24 — Complete's ordinary creature and combat-hero drawing
  wrappers reproduce all 253 and 251 retail bytes.** Retail 0x4951b0 has
  `army::DrawToBuffer` as its sole caller and preserves the Dreamcast edge to
  `combatManager::DrawCreature`; its nine-argument stack cleanup and final
  `CSprite::DrawCreature` call independently prove Complete's full signature,
  including the unused id and forwarded output colour. The next retail row at
  0x4952b0 preserves all four Dreamcast `DrawFrame -> DrawCombatHero` edges
  and the seven-argument cleanup. Complete omits the intervening Dreamcast
  `DrawCreatureAlpha` body and implements the hero wrapper through
  `CSprite::DrawCreature` with output colour zero. Both methods share the
  already-proven temporary `SLimitData`, extent-update and drawbridge-bounds
  clip skeleton, then draw the sprite's full width and height to the screen
  bitmap. VC6 reproduces every instruction in both bodies; their only asm
  report is the cosmetic label for still-unclaimed `ComputeExtent`, and the
  normalized byte verdict is exact. The synchronized build/delink/build
  checkpoint reaches **1900/2312 linked exact**, **1831/2243 game exact**,
  **96.59% game fuzzy** and **42.79% executable coverage**. All 51 unit tests
  and every fatal gate pass. The all-unit queue remains 412 residual
  functions / 28.6 KiB recoverable, while the ordinary tractable tier falls
  to **218 functions / 178.3 KiB**. Retail bytes prove both x86 identities and
  behavior; Dreamcast CodeView supplies names, base signatures and caller
  edges, with no external implementation body used.

- **2026-08-24 — Complete's extended `combatManager::DrawArcher` reproduces
  all 276 retail bytes.** The row at 0x495090 forwards the DC method's seven
  arguments to `ComputeExtent`, but retail's `ret 0x20` and its sole caller
  prove an eighth byte-sized colour selector. That caller is the siege-wall
  archer pass at 0x494c20, preserving the Dreamcast
  `DrawWallAt -> DrawArcher` edge and resolving the earlier ambiguity with
  neighbouring `DrawCreatureAlpha`. The method creates a temporary 16-byte
  `SLimitData` when needed, updates and clips the dirty extent against the
  manager's four drawing bounds, then draws the fixed 232-pixel archer strip
  to the screen bitmap with system-palette row 0 or 96. An explicit
  zero-initialize/conditional-assign spelling gives VC6 retail's byte test
  and two-way branch; all 15 CFG blocks then agree. Two reviewed site-specific
  aliases preserve the source names of `gpWindowManager` and
  `gSystemPalette`; the still-unclaimed `ComputeExtent` callee retains a
  cosmetic relocation-label delta, which the normalized exact-byte verdict
  correctly ignores. The synchronized build/delink/build checkpoint reaches
  **1898/2310 linked exact**, **1829/2241 game exact**, **96.59% game fuzzy**
  and **42.77% executable coverage**. All 51 unit tests and every ratchet,
  banked-row, claim, single-view and cleanliness gate pass. The all-unit
  queue remains 412 residual functions / 28.6 KiB recoverable; the ordinary
  tractable tier falls to **220 functions / 178.8 KiB**. No external
  implementation body was used: retail bytes prove the behavior and x86
  verdict; Dreamcast CodeView supplies the name, base signature, layout and
  cross-architecture caller edge.

- **2026-08-24 — `combatManager::SetupGridForArmy` claims and reproduces
  all 341 retail bytes.** The Dreamcast roster, source order and unique call
  graph place the method at 0x4937d0; retail independently confirms the one-
  argument arity and all seven named calls. Complete adds two gates around the
  DC statement skeleton: arrow-tower stacks are excluded, and either the
  persistent combat-grid preference or the creature-placement latch must be
  set. The method clears the 187-byte result row, seeds combat reachability,
  then marks the acting stack and its wide tail, reachable enemy occupants,
  and reachable empty cells (the latter with value 3). The final codegen lever
  was the DC Army.h shape: `get_owning_side` and `get_controlling_side` must be
  distinct inline accessors in the comparison. Raw field spelling let VC6 fold
  the occupant load into memory; the two TU-local inline views reproduce
  retail's `EAX`/`ECX` materialization exactly. All **26 CFG blocks and every
  instruction agree**. After the synchronized build/delink/build cycle, the
  linked checkpoint is **1897/2308 exact** and **1828/2239 game functions
  exact**, at **96.68% fuzzy** and **42.73% executable coverage**. All fatal
  gates are clean. The full census remains **411 residual functions / 28.6
  KiB recoverable**, while the newly claimed body reduces the ordinary
  tractable tier to **222 functions / 179.5 KiB** and raises matched code to
  833.0 KiB. No external implementation body was used; Dreamcast supplied the
  identity and source shape, while retail bytes fixed Complete's behavior and
  the x86 verdict.

- **2026-08-24 — the creature-quest serializer recovers its historical exact
  peak with two typed inline scalar writers.** The 311-byte
  `type_creature_quest::Save` residual had only one instruction wrong: retail
  loads the 16-bit creature id through `AX` but spills the whole `EAX` value
  into the shared scratch home before writing two bytes. A direct short/int
  union emitted a word spill; assigning its int member widened the source
  load, while the previously found pointer-alias spelling was correctly
  rejected by the zero-debt gates. The source-faithful shape is two separately
  inlined helpers, one taking `short` and one taking `int`: VC6 gives each
  formal the same four-byte argument home, producing retail's partial-register
  first store and the count overwrite without casts or aliasing. All 14 CFG
  blocks and every instruction are now exact. After the synchronized
  build/delink/build cycle, the linked checkpoint is **1896/2307 exact** and
  **1827/2238 game functions exact**, at **96.68% fuzzy** and **42.71%
  executable coverage**. All fatal gates are clean. The refreshed all-unit
  census falls to **411 residual functions / 28.6 KiB recoverable**, and only
  two rows remain below any historical peak; the ordinary tractable tier stays
  **223 functions / 179.8 KiB**.

- **2026-08-24 — `AddScoreToHighScore` is promoted from 99.9974% to an exact
  normalized match by correcting the score-file creation mode.** The earlier
  reconstruction passed `_S_IREAD | _S_IWRITE`, which VC6 emits as `0x180`;
  retail's `_open` call instead pushes `0x80`, the VC6 value of `_S_IWRITE`
  alone. The corrected source makes the last substantive semantic block exact:
  all 39 CFG blocks, their sizes, branches and ordinary instructions now agree.
  The verbose semantic view still prints the known synthetic EH-unwind addend
  in the prologue (`0` versus `0xb`), but the relocation-aware normalized
  verifier correctly classifies all 1,228 bytes as exact. After a fresh
  build/delink/build cycle, the linked checkpoint is **1895/2307 exact**
  overall and **1826/2238 game functions exact**, at **96.68% fuzzy** and
  **42.71% executable coverage**. All fatal gates are clean. The refreshed
  all-unit census contains **412 residual functions / 28.6 KiB recoverable**;
  the ordinary tractable tier remains **223 functions / 179.8 KiB**.

- **2026-08-24 — `LobbyLaunchConnect` closes remote.obj's last ordinary
  in-span function with all 996 retail bytes exact.** Dreamcast supplies the
  free-function boundary, the `CHourGlass` local, `gMapName` as a 260-byte
  array and the DirectPlayLobby method/structure names. Retail independently
  fixes the PC identity through eleven connection log strings and the
  `HandleMPlayerLaunch`, `GetConnectionSettings`, `SetConnectionSettings`
  and `Connect` edges. It also proves Complete's widened scenario-header
  call: copy the 251-byte setup filename to `gMapName`, then pass setup path,
  filename and a literal zero to the three-argument PC `NewSMapHeader::Get`.

  The MPlayer arm reuses the admitted bootstrap and expands the same
  `RemoteCleanup` body on failure. The lobby arm retrieves the 0x28-byte
  `DPLCONNECTION`, marks a hosting session migrate-host/keep-alive, logs its
  maximum/current player counts, installs the settings and keeps the
  connection allocation alive through `Connect`. The hourglass spans that
  connect plus local-player creation, including both error cleanups. Success
  copies DirectPlay's short player name into the persisted 21-byte network
  name, preserves retail's one-past terminator store, expands `InitRemote`,
  publishes the TCP/player-count state and creates the local DirectPlay
  player with the four-byte game version. Restoring those three state stores
  raises the initial 98.55% reconstruction to **100.00% across all 26 CFG
  blocks** at 0x555ef0. The synchronized inventory moves from **1893/2306 to
  1894/2307 exact functions**; remote's only remaining in-span gap is the
  deliberately unclaimed implicit wait-dialog destructor.

- **2026-08-24 — DirectPlay compression and member-side player-drop handling
  add 681 exact bytes; `SendIt` is admitted at a bounded 86.98% wall.** The
  Dreamcast remote.obj roster fixes the protected `CompressMsg`, public
  `SendIt`/`HandlePlayerDrop` and protected `QueueMsg` boundaries and their
  source order. Retail independently maps them through the zlib edge, the two
  DirectPlay error log strings, the player-drop log, virtual slots 8/32/35,
  and the existing transmit callers. `CompressMsg` copies the 0x14-byte wire
  header, reserves 20% plus twelve bytes, uses zlib level 6 and discards both
  failed and non-shrinking output. A named original-size local recovers
  retail's ESI lifetime; suppressing automatic inlining only across this
  member preserves the four already-exact transmit wrappers. The result is
  exact across all **185 bytes and five CFG blocks** at 0x5532b0.

  `HandlePlayerDrop` logs the DirectPlay DPID, constructs the 0x18-byte
  `CPlayerDropMsg`, clones it and appends the clone to the message deque.
  Retail proves that the DPID occupies both the base sender cell at +4 and
  the payload at +0x14. Preserving the roster's later `QueueMsg` source
  boundary lets `/Ob2` make the same context-dependent decision: the member
  handler expands the deque internals, whereas `SendIt` stops at the
  Dinkumware `push_back` helper. The handler matches all **496 bytes and 16
  CFG blocks** at 0x553580.

  `SendIt` reconstructs the six-attempt guaranteed/unreliable send path,
  HRESULT 0x88770096/0x80070057 handling, error logging, 200 ms delay,
  general-text row 82 retry dialog, shutdown path and invalid-player destroy
  plus queued-drop edge. Its invalid-player tail is exact block-for-block,
  but C2 rotates the source-honest `for` loop to a bottom test and normalizes
  the `Send` result through CL; retail keeps the retry comparison at the loop
  header and AL live through the HRESULT tests. `while`, explicit header
  breaks and call-site `inline_depth(1/2)` are structurally flat; caching the
  HRESULT worsens 86.98% to 85.00%. The retained form is the highest
  source-authoritative result rather than a control-flow carrier. The
  synchronized inventory moves from **1891/2303 to 1893/2306 exact functions
  at 96.68% fuzzy**.

- **2026-08-24 — player-drop recovery adds a 417-byte exact handler and
  admits its adjacent reload path with the retail CFG intact.** Dreamcast's
  remote.obj roster supplies the `HandlePlayerDrop`, `GetPlayerPos`,
  `OnPlayerDropUpdateMsg` and `CHourGlass` source boundaries. Retail proves
  the PC mapping independently through the drop/recovery log strings, the
  eight 0x168-byte player records and the callers in both wait-dialog
  dispatchers. `HandlePlayerDrop` searches the DPID, emits general-text row
  470 with the dropped player's name, refreshes the DirectPlay roster and,
  when the acting player vanished, hands control to the prior human. The host
  either reloads locally or transmits `CPlayerDropUpdateMsg` through the
  existing free wrapper; using that wrapper, rather than restating its
  `SendIt` pipeline, is the decisive source boundary. The result matches all
  **417 bytes and 20 CFG blocks** at 0x556430.

  The adjacent 0x5565e0 reload path reproduces all eight CFG blocks and both
  branches at 95.30%. It shows general-text row 659, tries the shared SC then
  RC recovery files, explicitly stops the hourglass before refreshing the
  player list, clears the dropped player's net record, rebuilds both current-
  player masks and resumes through `NextPlayer` or `StartLocalPlayerTurn`.
  Retail's two `StopMouseThread` calls prove that `CHourGlass::Stop` leaves its
  one-byte thread selector armed; all four wrapper methods are consequently
  `/Ob2`-only source boundaries with no standalone PC body. The bounded
  residual is compiler generation: this compile releases ESI after the DPID
  lookup, spills the first shift byte and calls the empty `CTextDialog`
  destructor layer, while retail reuses ESI and calls `TDialogBox` directly.
  Local/global assignment forms and explicit game-pointer lifetimes are
  byte-flat. Making the empty destructor visible does change the cleanup, but
  adds a vptr store and regresses the already-exact `CAnimatedDlg` destructor
  to 96.30%, so that TU-wide trade is rejected. The synchronized inventory
  moves from **1890/2301 to 1891/2303 exact functions at 96.68% fuzzy**.

- **2026-08-24 — `UpdateCurrentPlayers` adds 334 exact remote bytes; two
  adjacent player identities are admitted as bounded residuals.**
  Dreamcast supplies the public `UpdateCurrentPlayers` and `IsValidHuman`
  source boundaries plus the complete `CDPlayPlayer` layout. Retail proves
  that layout independently through the DPID load at +0x100, then fixes the
  whole PC body: enumerate DirectPlay players into a local `CAutoArray`, walk
  the eight 0x168-byte `playerData` records, retain DPIDs found in the live
  list, otherwise clear the three net fields and restore general-text row
  469, and publish the surviving player count at 0x699274. Direct
  `gpGame->players[i]` subscripts are byte-selected: VC6 strength-reduces them
  into retail's EBX stride while a named `playerData&` wrongly caches a
  pointer and grows the frame. The decisive lifetime boundary is the explicit
  final `playerArray.Destroy(1)`; it makes VC6 eliminate the immediately
  redundant implicit normal-exit destructor while preserving exception
  cleanup. The result is exact across all **334 bytes and 14 CFG blocks**.

  The same lane admits two source-honest residuals rather than grinding
  register allocation. `GetNextHumanPlayer` at 0x4f4ba0 is 95.12% with all
  twelve CFG blocks aligned and only its initialization schedule different.
  `HandlePlayerDead` at 0x556780 is 98.34% with all twenty blocks aligned and
  nineteen exact; retail proves its local-player cleanup, both dialogs and
  active-human handoff, leaving only the final player-pointer register order.
  The adjacent implicit `CWaitForReadyPlayersDlg` destructor remains
  deliberately unclaimed: enabling the TU-wide base-destructor expansion that
  improves it regresses the already-banked readiness-wrapper cleanup boundary.
  No external implementation body was used. The synchronized inventory moves
  from **1889/2298 to 1890/2301 exact functions at 96.68% fuzzy**.

- **2026-08-24 — the remote-combat constructor and its deleting wrapper add
  349 exact bytes.** Retail 0x556f20 expands `CAnimatedDlg`, constructs the
  PC-only complex combat-init payload and pause handler, installs vtable
  0x640f78, then clears only `m_playerPos` and the +0xbd0 received latch.
  Dreamcast supplies all seventeen `CCombatInitMsg` payload names; retail
  independently proves their shifted scalar prefix, both 0x38-byte army
  groups, the town at +0xb0 and the two 0x492-byte heroes at +0x218/+0x6aa.
  The town's eight-byte alignment rounds the message from its +0xb3c member
  end to 0xb40 and the containing dialog to 0xbd8, which `DoCombat`'s stack
  bands corroborate. That natural class spelling reproduces all 316
  constructor bytes and five CFG blocks on its first synchronized compile.

  The constructor also makes VC6 emit the vtable's 33-byte scalar deleting
  destructor at 0x557060, which is exact as a `VA_COMPGEN` claim. Its call
  target, the 253-byte ordinary destructor at 0x4aea00, remains deliberately
  unclaimed: it is an implicit destructor selected from events.obj by the
  still-unreconstructed `DoCombat`; an explicit empty body reaches 98.75%
  but adds the one derived-vptr store retail omits, so that provenance-false
  transcription was rejected. The synchronized inventory advances from
  **1887/2296 to 1889/2298 exact functions at 96.68% fuzzy**.

- **2026-08-24 — the remote-combat wait dispatcher reproduces all 489 retail
  bytes exactly.** Vtable 0x640f78 slot 3 and the adjacent Dreamcast order
  bracket identify 0x5570f0 as
  `CWaitForRemoteBattleDlg::handle_message`; retail's byte table independently
  fixes the five live subtypes and their chat, combat-init, player-drop, host
  handoff and session-loss arms. The source-shaped `CMessageKill` owns every
  dequeued packet except `RS_COMBAT_INIT`, which the PC-only complex-message
  bridge at 0x512e00 deserializes into the dialog before setting the received
  byte and closing the modal. The drop helper resolves and handles every DPID,
  then closes only when the dropped seat equals `m_playerPos`.

  Retail's complex-message constructor proves a vptr plus 0x14-byte `CNetMsg`
  base at +4, while the `advManager::DoNetCombat` stack frame proves the
  derived `CCombatInitMsg` is 0xb40 bytes. Together with the wait-dialog
  constructor, these facts place the by-value message at +0x80, its pause
  handler at +0xbc0 and the received flag at +0xbd0; Dreamcast contributes
  only the class/member identities and helper name. The first synchronized
  spelling matches all 20 CFG blocks and advances the inventory from
  **1886/2295 to 1887/2296 exact functions at 96.68% fuzzy**.

- **2026-08-24 — the level-pick network dispatcher is exact across all 757
  retail bytes.** Vtable 0x640f40 slot 3 and the Dreamcast order slot fix
  0x556c20 as `CLevelPickWaitDlg::handle_message`; retail's 73-entry byte
  dispatch table independently fixes the five live subtypes: chat, hero-level
  update, player drop, host handoff and session loss. The body follows the
  same source-shaped `CMessageKill` lifetime as the already matched ready-player
  dispatcher, which accounts for all three cleanup expansions around its two
  early returns.

  DC supplies the two private helper names and the four
  `CHeroLevelUpdateMsg` payload names; retail proves their PC layout by copying
  28 secondary-skill bytes to hero +0xc9, four primary-stat bytes to +0x476,
  and the skill count to +0x101. The player-drop arm compares the DirectPlay
  owner with `m_fromWho`, marks `m_playerDropped`, and reaches the same
  `HandlePlayerDrop` boundary as the combat-drop path. `/Ob2` naturally
  expands both helpers, `CAnimatedDlg::handle_message`, `ReceiveChat`, and the
  previously reconstructed `HandleNewHost`/`GetPriorPlayer` pair while keeping
  the expected network transmit out of line. The first synchronized source
  spelling reproduces all 27 CFG blocks and advances the inventory from
  **1885/2294 to 1886/2295 exact functions at 96.67% fuzzy**.

- **2026-08-24 — three adjacent remote.obj rows add 824 exact retail bytes,
  including the pure base network handler's legal out-of-line body.** The
  sole `RS_NORMAL_WIN` dispatcher edge and Dreamcast's matching name/order
  slot fix 0x5569f0 as `HandleNormalWinMsg`. Retail proves its one-dword
  player payload, local-team comparison, general-text rows 660/661, game-over
  state and the same-team completion latch. The first natural transcription
  is exact at 180 bytes. Dreamcast's `pct`, `cText[256]` and `sPct[256]`
  locals then identify 0x5574b0 as `CGameTransferSmack::SetPercentage`;
  retail supplies the 20-frame scale, rows 99/100, `"\n%0.0f%%"` suffix,
  160x160 text/update rectangle and current-Smacker wrapper calls. Its first
  source spelling is exact across all 301 bytes.

  Retail vtable 0x640f14 still contains `_purecall` in slot 3, but direct
  base-qualified calls from the adventure dispatcher land on 0x557920;
  C++ permits that pure virtual to have a definition. Dreamcast independently
  names it `CNetMsgHandler::HandleNetMsg` and records its calls to
  `HandleNewHost` and `DestroyMsg`. Restoring those source boundaries explains
  the PC body: `/Ob2` expands `HandleNewHost` and its `GetPriorPlayer` helper,
  transfers host control with a 0x18-byte `CPlayerDropUpdateMsg`, reports
  general-text row 471, handles session loss with row 329, and retains the
  guarded message destroy. A call-site `inline_depth(0)` preserves retail's
  out-of-line `CDPlayHeroes::TransmitRemoteData` call inside the otherwise
  expanded helper. The result is exact across 19 blocks, 10 branches and all
  343 bytes. This batch advances the synchronized inventory from 1882/2291
  to 1885/2294 exact functions at 96.67% fuzzy.

- **2026-08-24 — the final two ordinary Seer Hut rows are admitted as the
  creature proposal/progress dialogs.** Vtable 0x6418b4 slots 4 and 5, the
  parallel count/type vectors and picture class 0x15 fix 0x570880 and
  0x570b80 as `type_creature_quest::DoProposalDialog(hero*)` and
  `DoProgressDialog()`. The proposal filters to stacks whose whole-army total
  is below the quest count, while progress shows every requested stack. Both
  format localized `count name` rows and pack each picture qualifier as the
  zero-extended 16-bit count over the 16-bit creature id. The generated
  proposal path formats the missing list through the progress template; the
  progress path uses the proposal template plus its optional deadline suffix,
  or the dated custom proposal getter.

  The proposal scores 97.4706% with all 33 CFG blocks and branch targets
  exact. Its only size difference is a one-instruction quest-text address
  schedule; a named source reference is byte-flat. The progress body initially
  reproduced its first 24 blocks but expanded the final string refcount path
  (90.8971%). Giving the two vectors their source-shaped inner scope and
  applying `inline_depth(0)` only to the outer text destructor restores all 25
  retail blocks and reaches 99.5679%. One instruction remains: this compiler
  calls `basic_string::~basic_string`, while retail inlines that wrapper and
  calls `_Tidy(1)`. Default depth and depth 1 regress, so no manual destructor
  or raw-storage spelling is admitted.

- **2026-08-24 — the artifact proposal dialog is admitted with its exact
  retail CFG and stack layout.** Vtable 0x641878 slot 4, the Complete-era
  artifact-traits stride and extended-dialog picture class 8 fix 0x56f8a0 as
  `type_artifact_quest::DoProposalDialog(hero*)`. The body builds parallel
  vectors of the requested artifacts the visitor still lacks and their
  localized names. An empty custom progress string formats that joined list
  through the quest table's progress column; otherwise the custom text passes
  through directly. Both arms then show only the missing artifact pictures.

  Keeping each two-dword dialog record outside its loop recovers retail's
  exact 0x68 frame and every vector, string, temporary and record slot. Retail
  outlines `missingArtifacts.size()` only in the generated-text arm; a
  statement-scoped `inline_depth(0)` restores all 31 CFG blocks and every
  branch target without disturbing the inline element access and insertion.
  The retained body scores 88.7407%. Its five size-only residual blocks are a
  bounded VC6/Dinkumware generation class: one quest-text register schedule,
  two dialog-vector destructor inline choices, the custom arm's equivalent
  vector-insert overload, and the final trivial artifact-vector destroy.
  Extending the depth limit through the dialog scope regresses to 33 blocks
  and 86.4222%; text-pointer scope changes are byte-flat, so no fabricated
  storage or cleanup flow is admitted.

- **2026-08-24 — the last resource-quest dialog is admitted with its full
  retail CFG and an isolated register-homing residual.** Vtable 0x6418f0
  slot 4 fixes 0x571840 as
  `type_resource_quest::DoProposalDialog(hero*)`. The body uses the visitor's
  signed owner byte and retail's 360-byte `playerData` stride to compare all
  seven treasury balances with the quest price. Only deficits become
  localized `"%d %s"` requirement fragments and `type_dialog_resource`
  pictures. An empty custom progress string selects the quest table's progress
  column, formats it with the joined deficit list, and otherwise copies the
  custom string before the common extended dialog.

  Natural source reproduced all 25 blocks and 12 branch targets. Moving the
  reusable dialog record from the loop arm to function scope recovered the
  exact 0x6c frame and all three late string-temporary placements (86.9301 ->
  87.1048%). The bounded remainder is one allocation choice: this VC6 promotes
  the loop index in EBX and homes `this` at -0x10, while retail retains `this`
  in EBX and reuses the dead hero-argument slot for the index. Equivalent
  for/while forms, declaration-order changes and an explicit `this` alias are
  byte-flat; turning the named text-format local into an expression temporary
  changes the CFG and scores 80.2795%. The semantic body is banked without a
  fabricated volatile or recycled-argument spelling.

- **2026-08-24 — the retail-only monster-quest default-text initializer is
  semantically closed at 98.3426%, and its packed position is corrected to
  canonical `type_point`.** Vtable slot 14 and the contiguous monster-quest
  pool fix 0x56ef20 as `type_monster_quest::SetDefaultText`. The body resolves
  the editor monster reference through the 0x4cef10 reverse search over
  `game::monsterIdentifiers`, rejects its all-minus-one result, reads the
  creature id from the referenced `NewmapCell`, selects its plural name, and
  chooses one of the nine compass strings at 0x6a5c48 by map thirds. It then
  adds the retail `" underground"` suffix for the lower layer and fills the
  three empty localized text columns with monster name and direction.

  Retail's packed-field reads prove that both the quest member and slot-10
  argument use the shared signed 10/10/4 `type_point`; NH3API corroborates the
  type only after that byte proof. Replacing the provisional unsigned duplicate
  makes the adjacent 69-byte `NotifyMonsterDefeated` exact (72.6087 -> 100%).
  The initializer also recovers a mixed `NewfullMap::cell` header view: the
  by-value point wrapper is inline, while the three-scalar overload is called
  out of line.

  The retained initializer has retail's exact 77-block, 41-branch CFG. A
  traits-length plus depth-bounded two-argument `std::string::assign` restores
  the one compass site whose inline budget differs from its eight siblings.
  Only three compiler-generation blocks remain: the EH-handler relocation
  addend, opening point/cell register scheduling, and one final inlined `_Eos`
  where retail calls it. Direct `assign`, bounded `operator=`, explicit
  length-aware calls, declaration-order changes, and a named map pointer were
  measured; all either regressed the CFG/score or were byte-flat, so the
  semantic closure is banked without fabricated storage or control flow.

- **2026-08-24 — the two Complete-era quest event dispatchers are decoded;
  `TQuestGuard::DoEvent` is exact and the larger Seer event exposes an honest
  EH/STL-view residual.** Retail's caller, four-argument `ret 0x10`, five-byte
  guard layout and quest vtable calls fix 0x572b60 as
  `TQuestGuard::DoEvent(hero*, bool, NewmapCell*, type_point)`. Its deadline,
  visit-mask, proposal/progress, completion-confirmation, payment and
  `EraseAndFizzle` paths reproduce all 510 bytes. The last instruction lever
  was a direct signed-short day comparison, which lets VC6 keep the modulo-day
  arithmetic in BX instead of copying it through AX.

  The adjacent 0x573670 body retains Dreamcast's public
  `TSeerHut::DoSeerEvent(hero*, bool)` identity but is retail-only in shape:
  it selects all ten reward pictures, values the reward for AI visitors,
  applies payment/reward on acceptance, and formats the randomized empty-hut
  text. Its semantic body is complete, but the current string/STL declaration
  view produces a 0x34-byte frame and 87 blocks / three cleanup exits against
  retail's 0x24 bytes and 59 / two. The code-only iteration plateaued at
  30.8511% after the direct deadline, explicit expired-bool and register-alias
  hypotheses; the admitted 0x400-byte row includes both exact switch tables
  and therefore closes at 39.0772% in the synchronized report.

  The primary-skill slot-4 dialog at 0x56dad0 was decoded in the same pass. It
  computes the four still-missing skills, chooses the custom or synthesized
  progress line, appends the deadline suffix, and emits the four packed skill
  pictures. Source-local inline-depth control restores retail's out-of-line
  vector calls (46.5225 -> 56.5135%), but the present STL view still duplicates
  two cleanup exits; that residual is banked without raw-storage scaffolding.

- **2026-08-24 — six quest text/dialog identities were admitted; four text
  builders are exact and two progress dialogs retain an honest VC6 lifetime
  residual.** Vtable slots and retail operands identify 0x56dfa0 as
  `type_skill_quest::skill_requirement_text`, 0x56f5f0 and 0x5704e0 as the
  artifact/creature slot-6 requirement builders, and 0x572670 as the
  belong-to-player slot-7 description. The primary-skill body walks the
  canonical four-name table at 0x6a5390 and formats `%s %i`; the two container
  bodies feed artifact names or singular/plural `count name` fragments to the
  already exact shared list formatter. The player description lower-cases
  `gPlayerColorNames[required_owner]` with the same `std::transform` expansion
  already exact in that class's default-text body. Natural source made three
  rows exact on the first pass; spelling the DC-attested inline `GetArmyName`
  helper rather than precomputing locals closed the sole creature
  register-schedule residual.

  Retail vtable slot 5 and extended-dialog picture rows independently fix
  0x56dd60 and 0x56fbc0 as the skill/artifact progress dialogs. Both retain
  the returned progress string through construction of an eight-byte picture
  vector: skill pictures are classes `0x1f + i` with packed requirement
  values, while artifact pictures use class 8 and the artifact id. Their
  semantics and twelve-block CFGs agree, but this VC6 SP3 compile reloads the
  lifetime-extended string slot where retail consumes the returned EAX
  directly, cascading into 75.8427% and 75.2353% register-schedule residuals.
  Named-string, bare-pointer, named-string-plus-pointer and const-reference
  shapes were measured; the const reference is retained because it models
  the shipped lifetime without false raw-storage or allocator scaffolding.

- **2026-08-24 — the Seer Hut save/log/appraisal trio is reconstructed
  exactly, closing three adjacent retail-only rows.** Retail's sole
  `NewfullMap::Save` call fixes 0x573fd0 as the one-argument serializer for
  a 0x13-byte `TSeerHut`; the body independently proves quest type slot 8,
  quest save slot 13, the 12-byte reward record, and the shipped tail order
  `field_12`, `visitedPlayers`, `NameIndex`. The exact HD body supplies the
  `TSeerHut::save` name only after those retail facts, while Dreamcast
  preserves the older method lineage. The natural block-scoped byte locals
  reproduce all 145 bytes exactly.

  The quest log's SeerHutList arm is the sole caller of 0x574070 and proves
  a nullary thiscall with a hidden `std::string` result. Its unique HD
  structural twin supplies `getSeerLogText`; retail then fixes the three
  format inputs as the LOG-column template, the quest requirement, and the
  indexed seer name. A direct `quest_text(LOG)` accessor plateaued at 89.45%
  because it folded the table addressing. Spelling the source operation as
  `quest_texts()[QUEST_TEXT_LOG]` restores retail's two-stage address
  arithmetic and makes the 312-byte body exact.

  The independently mapped HD row at 0x573970 identifies retail 0x5735a0 as
  `TSeerHut::getValue(hero*)`; retail's sole AI caller and 195-byte body fix
  the ABI and semantics. An unvisited player receives at least 20 value via
  VC6's by-value, reference-returning `_cpp_max`. A visited hut is worthless
  when its quest is absent, past its +0x3c deadline, or unsatisfied;
  otherwise its value is reward appraisal minus the quest's player-specific
  AI cost. The first direct-guard reconstruction reached 48.91%. Nesting the
  visited path around one shared zero exit, materializing the `expired`
  boolean, and narrowing the absolute-day formula to `short` reproduces the
  retail register allocation, `setl/test` predicate, and 16-bit date
  arithmetic exactly. The synchronized build -> delink -> build closes at
  **1,876/2,277 exact, 96.78% fuzzy, and 42.04% filtered executable
  coverage**; all ratchet, claim, banked-row, single-view and cleanliness
  gates pass.

- **2026-08-24 — `TSeerReward::giveReward` is reconstructed exactly across
  all ten reward arms, its jump table, and its Complete-only AI edges.** The
  sole retail caller at 0x573919 passes `TSeerHut + 5`, independently proving
  the 12-byte reward receiver and the `(hero*, bool)` ABI; the unique fixed-
  byte HD join supplies the later `TSeerReward::giveReward` ownership only
  after that retail proof. Dreamcast's seerhut.cpp lineage supplies the
  statement order and its one named local, `type_artifact artifact`, while
  retail fixes every payload width and callee. The exact body preserves the
  shipped negative-mana assignment, signed morale/luck bytes, the secondary-
  skill eight-slot cap and upgrade delta, the 64-artifact backpack limit,
  spellbook/wisdom/known-spell gates, and the human-dialog versus AI-join
  creature split. Retail also proves that the creature count is widened from
  the packed signed 16-bit payload before all three calls.

  Three source shapes were decisive under VC6: mana selects its amount in the
  branches instead of preinitializing it; a newly granted secondary skill
  exits that switch arm before the mastery is retested; and spell/creature
  payloads stay as direct member reads. Dreamcast Hero.h:209 independently
  names the one-argument `type_artifact(TArtifact)` constructor, while this
  retail body proves its `extra = -1` then artifact-id store order. The final
  0x290-byte claim is 100.00%, with all 14 conditional branches and 13 exits
  agreeing. The synchronized build -> delink -> build advances the inventory
  to **1,873/2,274 exact at 96.77% fuzzy and 42.01% filtered executable
  coverage**; all ratchet, claim, banked-row, single-view and cleanliness
  gates pass.

- **2026-08-24/29 — the 528-byte `TSeerReward::getValue` dispatcher and its
  three retail-only philai dependencies are reconstructed exactly.**
  Retail 0x527cf0/0x527d80 prove that Complete changed Dreamcast's file-local
  `MoraleIncreaseValue(const hero*, int)` and `LuckIncreaseValue` helpers to
  thiscalls: the hero is in ECX, the award is the sole stack argument, and
  both return with `ret 4`. Dreamcast line records supply the statement
  shapes and named `double value_added`; retail independently fixes the
  undead early-out, morale/luck curves, primary-stat and army-value scaling,
  and the luck-cap rewrite. Both bodies are byte-exact (137 and 144 bytes).
  The 96-byte `hero::SoD_get_seer_skill_value` at 0x524630 is also exact. An
  independently identical HD 5.3 body supplies its name, while retail proves
  the receiver, two dword arguments, learned-skill/capacity gates and the
  typed `wants_skill` / `get_skill_value` calls. A representation-only local
  union keeps `TSecondarySkill` out of hero.h's wide include closure.

  With those calls named, retail 0x573a70 is reconstructed through all ten
  reward arms and both jump tables. It preserves the shipped Luck-arm bug:
  `LuckIncreaseValue` is called and its result discarded before falling into
  Resource appraisal. Retail also proves the Complete-only hero tail weights
  `value_of_power` and `value_of_knowledge`; the surrounding five-dword band
  is split with the matching Dreamcast names after a retail writer scan
  confirms every dword boundary. The resulting 528-byte body originally had
  the exact 18-block CFG and every executable instruction except three stack
  operands. The last source fact was visible in retail's relocation all
  along: it calls
  `AI_get_artifact_player_value(const type_artifact&, long)`, not the local
  pointer-shaped alias used by the first reconstruction. Passing the
  constructed artifact as the actual const-reference temporary preserves it
  at `[ebp-8]`; VC6 then keeps the resource-conversion double separately at
  `[ebp-0x10]`, reproducing retail's `sub esp,0x10` and closing all 528 bytes.
  The neighboring `type_artifact_quest::GetAIValue` caller adopts the same
  real helper boundary and remains exact. The synchronized build -> delink
  -> build checkpoint is **2,553/3,100 linked functions exact (82.4%),
  93.10% fuzzy, and 61.21% filtered executable coverage**; all ratchet,
  claim, banked-row, single-view, source-shape and cleanliness gates pass.

- **2026-08-24 — retail's 188-byte `TSeerReward::GetRewardExtra` switch is
  admitted and exact; generated tiny-row lookalikes remain unclaimed.**
  The sole retail call at 0x5737f2 passes `TSeerHut + 5`, exactly the address
  of the independently proven 12-byte reward record, while the three
  contiguous helpers at 0x573a70/0x573c80/0x573f10 all dispatch on its
  0..10 type word and read only its eight-byte payload. That x86 evidence
  proves the retail receiver and address. Dreamcast supplies the semantic
  `GetRewardExtra(const hero*)` identity at seerhut.cpp:373 and the eleven
  `TSeerRewardType` names; NH3API was consulted only for the already-proven
  retail record/helper spelling, never for an address. The body reproduces
  all ten arms: experience-factor scaling, direct mana/resource/artifact/
  spell values, signed morale/luck/primary bonuses, the secondary-skill
  icon formula, and the packed creature/count result. The first direct-cast
  model reached 87.43%; representing the narrow payloads as full-dword
  signed bitfields recovered retail's `mov` + shift sign extension and
  raised it to 91.86%; retaining the creature id's direct 16-bit load made
  all 11 blocks, 8 returns, 188 bytes and the jump table exact. The adjacent
  528-byte `getValue` helper is located but deliberately not claimed in this
  batch: two of its calls lead to retail-thiscall philai helpers whose DC
  `static` signatures differ, so those identities must be promoted first.
  The same audit rejected eight tempting tiny rows as independent game
  claims: cmbtmgr's set/vector/TArcher construction and destruction helpers
  (0x462890/0x4628b0/0x462930/0x466260), hero's rethrow funclet 0x4e29dc,
  mapcell's static-destruction shim 0x504290, and resourcemanager's tree
  destructor/catch tail 0x559440/0x55b8b9. Retail bytes and ownership remain
  authoritative; no generated/library body was laundered into source. The
  post-delink checkpoint is 1,869/2,269 linked functions exact, 96.77% fuzzy
  and 41.93% of the filtered executable matched; the tractable in-span tier
  falls to 257 functions / 195.9 KB.

- **2026-08-24 — command's 1,461-byte `GetControl` state machine is admitted
  with every retail UI/AI transition reconstructed and a bounded VC6 block-
  placement residual.**
  The exhaustive command.obj order bracket fixes retail 0x4782d0 between
  `CheckGetAIMove` and `ResetMouse`; Dreamcast independently publishes
  `combatManager::GetControl`, its void-thiscall ABI, source line 3131 and the
  same direct-call family. Retail then fixes the Complete behavior: clear the
  selected-hex and sentinel fields, flush pending input outside automation,
  restore the combat cursor for an active non-quick manager, inline
  `CheckChangeSelector`, and derive the remote-human control latch from both
  player ids and the network/local-human tests.

  The reconstructed control-bar half disables every button for quick/computer
  actions, applies the AI-turn highlight, or broadcasts the local player's
  palette and updates the retreat, surrender, spellbook and three command
  buttons. The retail bytes correct two source-level details that a superficial
  reading misses: the town gate tests the **Stronghold faction byte at town+4**
  and its active special-building bit, not the owner byte; and placement mode
  clears only 0x8fc/0x7802 before skipping the ordinary 0x7d6..0x7da updates.
  The tail reproduces the inlined `ResetMouse`, clears the pending order and
  calls ai.obj's `DoSpellAI`.

  The best natural VC6 spelling reaches **87.77473%**. Candidate and retail
  both contain 79 blocks, 52 branches and two returns; the entry through the
  network/control-window gates and the shared ResetMouse/DoSpellAI tail agree.
  The principal residual is block placement: retail emits the three-block
  automated-control arm before the long local-human UI arm, while this SP3 C1
  stream permutes it behind that arm and changes a few scratch bindings.
  Positive/negative structured predicates, a nested form and explicit local
  labels are byte-identical; duplicating the final tail regresses to 69.56%,
  and named player/hero locals regress to 79.98%/86.46%. The placement nesting
  is the last semantic correction (86.21% -> 87.77%), so the compiler-layout
  residual is documented instead of disguised with another rewrite.

  `GetControl` and `DoSpellAI` use command-only one-for-one substitutions for
  command-unused spells/AI declarations, preserving the measured member-
  handle population; `GetCommand` remains at its historic **92.57143%** canary.
  After build -> fresh delink -> full build, all **2,268** rows are ratchet-
  clean, with **1,868 exact at 96.77% fuzzy**, 2,199 source claims and
  **41.92% executable coverage** (817.2 KiB matched). The regenerated queue is
  400 non-exact functions / 27.3 KiB recoverable. The ordinary unclaimed in-
  span tier falls to **258 functions / 196.1 KiB** (command: 2 / 8,609 B), and
  the no-unit-claim remainder falls to **1104.7 KiB**. Banked-row, source-claim,
  single-view and cleanliness gates are clean.

- **2026-08-24 — command's move/attack transaction and three tail rows are
  byte-exact, and `ResetRound` is admitted at a fully bounded VC6
  handle-state residual.**
  The exhaustive Dreamcast command.obj roster/order join fixes 0x478900 as
  `combatManager::process_move_then_attack`; retail independently fixes its
  656-byte extent, every Complete field offset and all 25 branch edges. The
  reconstructed transaction preserves the old hex and facing, resets cycling
  and the Champion charge distance, moves before attacking when needed,
  clears Complete's 187-cell movement row on the fallback path, applies
  obstacle attacks, and implements the Harpy/Hag return flight with the three
  retail disable guards. It then restores facing, resolves victory, or closes
  the turn through morale and cycle-timer processing. The first scored VC6
  build reproduces all 656 bytes exactly.

  Retail 0x475ed0 is likewise fixed as `combatManager::ResetRound` by the
  unique whole-body map, command order and the DC statement table. Its live
  body covers Complete's placement-phase/network handoff, highlighter reset,
  both 20-stack side walks, expiring obstacle removal/spell effects and the
  round message. It reaches **97.48035% across 815 bytes** with identical
  229-instruction streams, all 21 branches and both returns exact. The only
  differences occupy 26 register-visible slots in the placement-message and
  inlined-highlighter regions; the callee-saved bindings agree, and `why-reg`
  v2 classifies the remainder as C1 front-end scratch-pseudo order. Named,
  value and pointer remote-position locals, the reversed endpoint spelling,
  and a manual highlighter expansion were measured and were identical or
  worse. The DC line row explicitly requires `TurnOffHighlighter(1)`, so the
  evidence-backed source is retained and the compiler-only residual is
  documented rather than obscured with further semantic rewrites.

  The order bracket after exact `AddArmy` then promotes three formerly
  boundary-attributed rows: private const `get_tower_string` at 0x47a2d0,
  `ViewCastleBallista` at 0x47a380 and `HandleCombatPlayerDrop` at 0x47a500.
  Their DC publics fix the names, constness and ABIs; retail independently
  proves the wall-strength/trait lookup, Citadel/Castle tower composition,
  modal arguments, player-id dispatch and the fully inlined two-side cycle
  timer reset. All **907 bytes** are exact. The only source-version
  correction is decisive: Complete omits the DC statement table's trailing
  `FullUpdate` after the castle dialog and after both player-drop dialogs;
  deleting the first such call alone closes `ViewCastleBallista` from
  98.19444% to 100%. The new signatures initially crossed command.obj's
  measured C1XX handle threshold; command-only substitutions for unused
  drawing/text/remote declarations restore `GetCommand` without hiding any
  dependency or accepting a regression.

  The same order audit now admits the intervening **997-byte** row at
  0x477ee0 as `combatManager::CheckGetAIMove`. Retail fixes both neighbours
  (`TurnOffHighlighter` / `GetControl`) and every field/call edge; the direct
  0x41e570 call occupies Dreamcast's exact `AICheckRetreat` statement slot.
  The reconstructed body handles the human retreat confirmation, computes
  the active-stack combat value, lets `/Ob2` expand the already exact
  `get_surrender_cost`, enforces the town/hero/gold/value surrender gates and
  records retreat/surrender orders 4/5. It reaches **94.43355%** with an exact
  53-block CFG, all 35 branches and all five returns; 49 blocks also have the
  exact instruction count. The residual is confined to the two twenty-stack
  value loops: retail homes one extra four-byte scratch and uses a different
  EDI/ECX/EDX binding. A named side is inert, two explicit current-hero
  lifetimes regress to 87.19%/80.53%, and spelling Dreamcast's `IsActive`
  helper directly regresses to 84.86% by consuming an inline-budget slot and
  leaving an extra string `_Tidy` call. The named army-row base is therefore
  retained as the best natural source, with no missing semantic block.

  The final 17-byte command-tail row at 0x47a670 is deliberately **not**
  claimed: the compiled relocation identifies it as Dinkumware
  `std::_Tree<int,...>::begin()`, a pooled header COMDAT. Thus command's real
  game-code tail ends at 0x47a664; the later Dreamcast-only
  `CheckAutoScrolling` / `ShiftXY` rows have no separate retail slot here and
  are not forced onto library bytes.

  After a fresh PDB/delink and full gated rebuild, the checkpoint is
  **1868/2267 exact at 96.78% fuzzy**, 2198 source claims and **41.86%
  executable coverage** (816.0 KiB matched). `GetCommand` retains its historic
  92.57143% canary score; ratchet, banked-row, source-claim, single-view and
  cleanliness gates are clean. The regenerated queue contains 399 non-exact
  functions / 27.1 KiB recoverable, while the ordinary unclaimed in-span tier
  falls from 262 functions / 199.9 KiB to **259 functions / 197.5 KiB**,
  accounting for the three newly admitted in-span command rows. The three
  boundary-attributed tail rows and newly compiled `CheckGetAIMove` together
  reduce the no-unit-claim remainder from 1108.0 KiB to **1106.2 KiB**.

- **2026-08-24 — seven more command.obj combat-control functions are
  reconstructed and all 1,965 bytes reproduce retail exactly.** The
  exhaustive Dreamcast command.obj roster/order join fixes 0x476200,
  0x4762f0, 0x477ac0, 0x477b60, 0x477c20, 0x477e10 and 0x478b90 as
  `combatManager::auto_resolve_combat`, `CheckWin`, `CheckChangeSelector`,
  `TurnOffSelector`, `CheckChangeHighlighter`, `TurnOffHighlighter` and
  `process_first_aid`; retail independently fixes every boundary, Complete
  field offset, branch and call edge. The selector pair tracks the acting
  stack at +0x132c8, normalises its wait animation and marks the old stack for
  redraw. The highlighter pair closes the adjacent active-byte/grid-index
  state machine, including the inlined hex/occupancy and fidget-sequence
  tests. All four bodies are byte-exact.

  `auto_resolve_combat` copies both `armyGroup`s, runs `AI_auto_combat`, then
  writes simulated counts back through the retail-proven `originalIndex` at
  army +0x5c and raises the zero-count marker. `CheckWin` adds Complete's
  delayed quick-combat resolver, determines a sole winner or draw, propagates
  both combat flags, calls `DoVictory` and terminates the message loop. The
  first-aid worker resolves the target cell, rolls the hero's First Aid
  factor, clamps against `topCreatureDamage`, marks the tent as spent and
  performs the Regeneration sample/message/effect path outside quick combat.
  Its last stack-slot residual closed only when the random maximum is the
  short-lived first local and the old/final damage result occupies the
  longer-lived parameter home; that is the source shape retail's VC6 stream
  proves.

  The full gate initially caught the known command include-set wall:
  `GetCommand` moved 92.5714 -> 92.5357 after all eight command-view method
  declarations (six new rows plus the already landed grid/reset pair) were
  visible beside eight unrelated setup/turn declarations; the seventh new
  body, `TurnOffHighlighter`, reused its existing declaration. A negative
  control proved the member-count cause. The final command-only view hides
  exactly the eight methods command.cpp never references, substitutes the
  no-argument `army::GetName` for its unused static overload in the same slot,
  and keeps `AI_auto_combat`'s one-use declaration local. `GetCommand` is
  restored to its historic 92.5714 without accepting a regression, while
  every promoted body remains exact.

  After a fresh PDB/delink and full gated build, the checkpoint is
  **1864/2261 exact at 96.78% fuzzy**, 2192 source claims and **41.70%
  executable coverage** (812.7 KiB matched). Ratchet, banked-row,
  source-claim, single-view and cleanliness gates are clean. The regenerated
  queue remains 397 non-exact functions / 27.1 KiB recoverable; the ordinary
  unclaimed in-span tier falls to **262 functions / 199.9 KiB**, and
  command.obj itself falls from 13 functions / 14,503 bytes to **6 functions
  / 12,538 bytes**, exactly accounting for the seven promoted rows.

- **2026-08-24 — command's combat-cycle/reset cluster adds four exact
  functions, and the Dreamcast statement table corrects the timer formula.**
  The retail bodies at 0x478890, 0x479de0, 0x479f30 and 0x479fc0 are now
  admitted as `combatManager::ResetMouse`, `ResetCyclingCreatures`,
  `ResetCycleTimers` and `SetCombatGrid`. The identities come from the
  Dreamcast decorated publics and command.obj order, but every placement,
  field offset, call edge and final verdict comes independently from the
  retail image. All four reproduce their retail bytes exactly on the first
  source spelling.

  `ResetMouse` closes the 110-byte cursor reset that `Open` calls and
  `RightClick` open-codes in three return arms. `ResetCyclingCreatures`
  closes the two live-stack walks: the first detects non-dead stacks in
  `cs_fidget` and expands Complete's tower-aware `MarkCreatureEffect`; the
  second returns every non-dead stack to `cs_wait`, resets its frame index
  and stamps `iLastFidgetTime`. `ResetCycleTimers` proves the two retail
  cycle fields at army +0xfc/+0x154 as the DC-named `iLastFidgetTime` and
  embedded `SMonFrameInfo::iFidgetFrequency`. More importantly, the SH4
  statement disassembly refutes the old provisional note: the call is
  **`Random(50, iFidgetFrequency)`**, not `Random(1, 50)`, making the exact
  assignment
  `now + 2*Random(50, iFidgetFrequency) - iFidgetFrequency`. The two manager
  stores at retail +0x53fc/+0x5400 are the hero last-fidget clock row; DC
  gives the semantic name while retail alone fixes its shifted x86 offset.

  `SetCombatGrid` then validates the surrounding inline graph. Its three
  globals are the already byte-proven registry-backed preference members
  `showCombatGrid`, `showCombatMouseHex` and `combatShadeLevel`. Retail
  expands both `get_current_army()` and the newly reconstructed
  `ResetMouse()` under `/Ob2`, exactly as the direct DC source statements
  predict, while retaining `UpdateMouseGrid`, `SetupGridForArmy`,
  `DrawFrame` and `WritePrefs` out of line. Command-only class/army views
  expose the necessary fields and declarations without perturbing any
  previously exact body.

  After fresh delinks and the full gated build, the checkpoint is
  **1857/2254 exact at 96.77% fuzzy**, 2185 source claims and **41.60%
  executable coverage**. All ratchet, banked-row, source-claim, single-view
  and cleanliness gates are clean. The regenerated queue remains 397
  non-exact functions / 27.1 KiB recoverable; the ordinary unclaimed
  in-span tier falls from 273 functions / 202.7 KiB to **269 functions /
  201.8 KiB**, exactly accounting for the four promoted command bodies.

- **2026-08-24 — the resource-archive initializer and remote wait/win-loss
  cluster are reconstructed; five new rows are exact and two are
  instruction-exact.** Retail's 0x559150 dynamic initializer proves eight
  consecutive 0x190-byte `TResourceLODSlot` objects, each consisting of an
  archive-name pointer and a `LODFile` at +4. The resulting 26-byte
  constructor at 0x5591e0 and all eight Complete archive strings now match
  exactly. Dreamcast then supplies the names and signatures for
  `HandlePlayerWon` and `HandlePlayerLost`, while retail fixes the common
  payload offset (+0x18), the 0x4c/0x24 condition records, the fastcall
  helper arguments and every global store. Both candidate instruction
  streams and control flow are byte-identical to retail (94 and 75 bytes);
  their strict scores remain **99.9394% / 99.9091%** only because the
  stripped target still calls the two display helpers by working labels and
  names the three honest globals generically. This is the established
  relocation-name-only residual class, not a source-code mismatch.

  The adjacent dialog sweep adds exact `CLevelPickWaitDlg::WaitForLevels`
  (114 bytes) and `CWaitForRemoteBattleDlg::Wait` (92 bytes). Retail proves
  their shared creature-table/animated-dialog pattern and their deliberate
  differences: the level-pick path seeds and retries Devil/Arch Devil, then
  uses text row 472/sequence 0; the remote-combat path makes one roll and uses
  row 473/sequence 12. The PC remote-battle constructor also proves that its
  object carries a large platform-specific combat-init payload at +0x80 and
  a pause handler at +0xbc0, so only the retail-proven +0x78 prefix is
  modelled until those bodies land; Dreamcast's compact pointer-based tail is
  not transferred to x86. Finally, the now-supported `IMPLICIT_DTOR` claim
  binds the existing five-byte VC6 publics for `CSaveScreen` (0x557340) and
  `CAdvMgrNetMsgHandler` (0x57d160), both exact without inventing source
  destructor definitions. The nearby 1/3-byte text-widget slots, the shared
  popup null virtual, sacrifice-widget folds, cmbtmgr member-subobject
  callbacks and EH funclets remain deliberately excluded rather than falsely
  owned. After fresh delinks and full gated builds, the checkpoint is
  **1853/2250 exact at 96.77% fuzzy**, 2181 source claims and **41.55%
  executable coverage**. Ratchet, banked-row, source-claim, single-view and
  cleanliness gates are clean. The refreshed queue contains 397 non-exact
  functions / 27.1 KiB recoverable and 273 ordinary unclaimed in-span
  functions / 202.7 KiB in the tractable tier; only the three previously
  bounded sub-0.05-point historical peaks remain.

- **2026-08-24 — the two lost town reward peaks are recovered with the real
  vector operation, one honestly exact.** `homm3 vc6 queue` exposed
  `show_building_rewards` at 92.6954% below a historic 100% and
  `show_creature_rewards` at 93.2294% below 99.0596%. Those historic peaks
  had been produced by sixty/thirty self-assignments and were correctly
  withdrawn under the no-numerator-carriers rule. The missing real construct
  was already named by Dreamcast's xref census: both helpers call
  `std::vector<type_dialog_resource>::clear`, while the reconstruction had
  manually spelled `erase(begin(), end())`. The two forms emit the same tail
  copy loop, but only `clear()` gives VC6 retail's earlier inline graph: the
  `format_string` return temporary's destructor expands instead of calling
  `_Tidy(1)`. `show_building_rewards` therefore closes all **474 bytes at
  100.0000%**, and `show_creature_rewards` recovers its carrier-free
  **99.0596%** peak. The creature body now also restores Dreamcast's one
  header-inline `GetArmyName` call in place of its hand-expanded lookup; it is
  byte-flat, but is the better source claim. Its 218 instructions, 12-call
  inline multiset and all 20 branch targets agree with retail. The sole
  remaining delta is one adjacent scheduling transpose around
  `format_string("%d ", count)`: retail forms the hidden return-slot address
  before pushing the count, VC6 does the reverse. Swapping the count/creature
  declarations is flat, the allocation model finds no binding divergence,
  and a scoped named temporary regresses to 97.3624%, so the attested direct
  expression is retained. No synthetic mass remains. After a fresh delink and
  full gated build, the checkpoint is **1848/2243 exact at 96.77% fuzzy** and
  **41.53% executable coverage** (809.6 KiB fuzzy-matched), with 2174 source
  claims. The refreshed queue contains 395 non-exact functions / 27.1 KiB
  recoverable and no lost town peaks; only three sub-0.05-point historic peaks
  remain tree-wide.

- **2026-08-24 — `town::ApplySpecialBuildingEffect` and its retained
  `GetPrimarySkill` header COMDAT close exactly.** The retail bracket between
  exact `SetSummoningGenerator` (0x5bd750) and `town::town` (0x5bde80), plus
  the Dreamcast public at dc 0x165ea0 and its xref census, uniquely fixes the
  1,361-byte body at 0x5bd8e0. Retail proves seven independent town-type arms:
  Dungeon's Mana Vortex doubles maximum mana; Castle's Stables grants the
  shared movement bonus once; Tower and Inferno increment Knowledge and Spell
  Power; Dungeon's Battle Scholar Academy grants a Learning-scaled 1,000
  experience; Stronghold and Fortress increment Attack and Defense. The three
  building masks are the existing `bitNumber[EXTRA_0_ID]`,
  `bitNumber[EXTRA_2_ID]` and `bitNumber[SPECIAL_BUILDING_ID]`, and the five
  once-per-town rewards use the DC-attested `TownSpecialGrantedMask`. The first
  structured transcription was instruction-exact through Mana Vortex and
  Stables but scored 97.4786% overall because VC6 expanded the first bitset
  `operator[]` and left its nested `test` out of line. A nonzero
  `inline_depth` probe was byte-flat; spelling that first predicate as the
  equivalent `.test(id)` exposes the same bounds-check/test body one level
  shallower and closes all 1,361 bytes. The other four predicates retain the
  Dreamcast's proxy spelling and naturally reproduce retail's later inline
  decisions, so this one spelling is recorded as a retail-codegen divergence,
  not promoted to a cross-architecture source fact. Expanding the
  DC-attested `GetMaxMana` header inline also makes town.obj emit the adjacent
  `hero::GetPrimarySkill(int) const` COMDAT at 0x5bde40; its 49 bytes match
  retail exactly and are enrolled with the established claim-only header-body
  pattern. After a fresh PDB/delink and full build, the checkpoint is
  **1847/2243 exact at 96.76% fuzzy**, 2174 source claims and **41.53%
  executable coverage** (809.5 KiB fuzzy-matched). Ratchet, banked-row,
  source-claim, single-view and cleanliness gates are clean. The refreshed
  queue contains 396 non-exact functions / 27.1 KiB recoverable and 279
  ordinary unclaimed in-span functions / 203.1 KiB in the tractable tier.
  No external implementation body was used: retail bytes, relocations and CFG
  prove the x86 behavior; Dreamcast CodeView supplies only names, signatures
  and cross-architecture call-graph corroboration.

- **2026-08-24 — the town-threat search pair is reconstructed and the
  summoning-portal generator closes exactly.** Retail's adjacent bodies at
  0x4280e0/0x4282b0 are uniquely identified as
  `type_town_threat_checker::check_towns` and `mark_towns`: the former is
  called by both turn paths and calls the latter, while the Dreamcast xref
  graph independently supplies `GetHero`, `GetMobility`, `SeedPosition`,
  `GetTown`, `get_cell` and `can_take_town`. `check_towns` clears every town
  mark, searches each opposing hero with 800 bonus mobility, clears that
  hero's bounty and scans the acting player's towns; its first admitted
  spelling is byte-exact over 369 bytes. `mark_towns` reproduces all 343
  bytes, 16 blocks and every instruction, but then reported **99.9554%** because
  stripped-image relocation recovery classifies the honest integer
  5,000,000 as a DIR32 code reference: it numerically equals VA 0x4c4b40.
  The integer spelling therefore remains honest. **Superseded 2026-08-28:**
  paired resolved-address normalization proves that representation and now
  reports the function exact. The adjacent
  town bracket then fixes 0x5bd750 as `town::SetSummoningGenerator`; the DC
  locals name its generator copy and candidate vector, while retail proves
  the owner filter, two uniform random choices and growth-rate assignment.
  Changing the candidate index from unsigned to one address-taken `int`
  raised the first transcription from 78.2652% to 94.3106%; reusing it across
  all three loops and restoring the empty-vector early return closes all 392
  bytes exactly. The checkpoint is **1845/2241 exact at 96.75% fuzzy**, 2172
  source claims and **41.46% executable coverage**. All ratchet, banked-row,
  source-claim, single-view and cleanliness gates are clean. No external
  implementation body was used; retail bytes/relocations/CFGs prove the x86
  behavior and Dreamcast CodeView is limited to names, signatures, locals and
  cross-architecture call-graph corroboration. The refreshed queue contains
  396 non-exact functions / 27.1 KiB recoverable and 281 ordinary unclaimed
  in-span functions / 204.5 KiB in the tractable tier.

- **2026-08-24 — `adventuremapwindow`'s five remaining ordinary in-span
  functions are claimed and all 2,486 bytes reproduce retail exactly.** The
  exhaustive Dreamcast-roster/order-map join fixes the identities and retail
  carves of `convertID2HelpID`, `ProcessRightSelect`, `ProcessHover`,
  `UpdateQuestLogButton` and `SetSleepImage`; the regenerated in-span census
  now has no unclaimed `adventuremapwindow` row. `convertID2HelpID` implements
  the sparse 27-row widget-help map, including Complete's ids 1001..1015.
  `ProcessRightSelect` dispatches the two hero rows and town row to their quick
  views, then sizes and centres generic help. `ProcessHover` reconstructs the
  cached hover gate, mouse reset, town/hero rollover formatters, generic-help
  lookup and id-200 redraw. Its first structured transcription reached
  97.13% with every instruction and all 19 symbolic branches aligned but a
  duplicate epilogue; Dreamcast CodeView's lexical blocks proved that the
  focus early return and hover-change body are siblings, which makes VC6 emit
  retail's single exit and closes the function. `UpdateQuestLogButton` scans
  both quest-object lists and visited-player masks, while `SetSleepImage`
  reproduces the icon, palette, draw, status and hotkey broadcasts. The work
  also admits the retail hover cache at 0x65f228, sleep icon/hotkey data, the
  interleaved eight-byte quick-help row, and the narrow declarations required
  by the five callers. After a fresh delink, the checkpoint is **1843/2238
  exact at 96.75% fuzzy**, 2169 source claims and **41.40% executable
  coverage** (807.0 KiB matched); `adventuremapwindow` itself is 23/24 exact
  at 99.8494%, with only the already banked 98.5808% `UpdateHeroLocator`
  residual. All ratchet, banked-row, source-claim, single-view, cleanliness
  and delink-freshness gates are clean. The refreshed wall census remains 395
  residual functions / 27.1 KiB recoverable, and the tractable unclaimed tier
  is 283 functions / 205.2 KiB. No external implementation body was used:
  retail bytes, relocations and CFGs drove the reconstruction, with Dreamcast
  evidence limited to names, types and lexical source shape.

- **2026-08-24 — hiscore's last two ordinary in-span functions are claimed;
  the 1,228-byte score insertion is instruction-exact and the 1,286-byte
  window constructor is semantically closed.** `AddScoreToHighScore` now
  models the two eleven-row 0x64-byte tables, the campaign/cheat placement
  rules, the descending whole-record shift, the fully inlined
  `CHSInputDlg` construction and teardown, and the 0x898-byte file rewrite.
  Its 39 CFG blocks, every block size and all 17 symbolic branch targets
  agree with retail. The strict report remained **99.9974%**; at this
  checkpoint that residual was mistakenly attributed solely to the
  EH-prologue's synthetic unwind-table relocation addend. A later byte audit
  also found the `_open` creation-mode mismatch and the correction is recorded
  in the newer entry above.
  `THighScoreWindow` constructs the four controls, both eleven-icon score
  families, the widget stream and the two captured backgrounds. Its 46-block
  CFG and branch sequence agree, with 42 exact-sized blocks and four
  one-byte size residuals; **90.7049%** is bounded to the global
  EBX/ESI/EDI allocation phase. An early `i = 0` lifetime was byte-flat, and
  reversing the two widget-ID additions fell to 87.55%, so neither probe was
  retained. Dreamcast CodeView independently proves `Creatures[2][11]`,
  `CreatureFrames[2][11]`, `bIsStandard`, `iCreatureFrame`, `lLastServe` and
  `hiScoreBack[2]`; retail shifts the arrays by its eight-byte-wider base,
  then drops DC's intervening `m_h_index` dword, exactly explaining the
  observed +0xfc..+0x10c tail. The refreshed delink/build checkpoint is
  **1838/2233 exact at 96.74% fuzzy**, 2164 source claims and **41.28%
  executable coverage** (804.6 KiB matched). The score ratchet, banked-row,
  claim, single-view and cleanliness gates are clean. The regenerated wall
  census contains 395 residual functions / 27.1 KiB recoverable; the two new
  claims remove `hiscore` from the tractable unclaimed inventory, reducing
  that tier to 204 functions / 188.2 KiB. No externally sourced
  implementation body was used: retail bytes, relocations and xrefs drove
  the reconstruction, with Dreamcast evidence limited to names and layouts.

- **2026-08-24 — four retail-only combat-manager helpers are promoted and
  reproduce all 964 bytes exactly.** The nearby 0x462890/0x4628b0/0x462930
  member-subobject constructors and 0x466260 operator-delete tail remain the
  excluded COMDAT class; the queue now reports exactly those four rows (253
  bytes) as the only unclaimed functions inside `cmbtmgr`'s span. The ordinary
  bodies have no counterpart in the Dreamcast `cmbtmgr.obj` roster, so the
  three unattested methods retain address-ordinal names while the already
  documented Phoenix sweep keeps its bootstrap `CheckRebirth` name.
  `Unnamed465f20` derives the siege-tower strength, preserves retail's
  diagnostic `numArchers * 6 / 2` arithmetic, selects a defender and emits the
  SHOOT order; it was exact on the first compile. `Unnamed4693a0` disables the
  highlighter and kills every non-tower stack on the requested side.
  `CheckRebirth` spends Phoenix rebirth charges, applies one resurrection per
  full group of five plus the independent remainder rolls, plays the
  Resurrection sample outside quick combat, and restores the successful
  stack. `Unnamed469e50` is the Complete-only moat worker: it stops/resumes the
  walk sample, applies the faction's moat damage, prints the nine-row
  faction-specific message, drives the effect, then checks rebirth. The latter
  three closed in one retail-grounded source-shape probe: explicit advancing
  `army*` induction, a zero-tested remainder loop, and the moat worker's one
  shared false epilogue. The refreshed build/delink/build checkpoint reaches
  **1838/2231 exact at 96.74% fuzzy**, 2162 source claims and **41.16%
  executable coverage**. All ratchet, banked-row, source-claim, single-view,
  cleanliness and delink-freshness gates are clean. The wall census remains
  393 residual functions (27.0 KiB recoverable); the tractable unclaimed tier
  falls to 206 functions / 190.6 KiB, and matched executable code rises to
  802.3 KiB. No externally sourced implementation body was used: retail bytes,
  xrefs and call conventions drove the reconstructions, with Dreamcast
  evidence used only to prove that the four rows are absent from its roster.

- **2026-08-24 — the retail MP3 stop/swap worker cluster is reconstructed,
  adding three exact functions and one bounded `soundManager::Open`.** The
  missing 16-byte boundary at 0x59a830 is admitted from five `_beginthread`
  address-takes, its retail tail into `_endthread`, and the Dreamcast
  `ProcessMP3Stop(void*) -> ThreadStopMP3` relationship. `ProcessMP3Stop`
  (16 bytes), `soundManager::ThreadStopMP3` (533 bytes), and
  `ProcessStopAndPlayMP3` (955 bytes) now reproduce retail byte-for-byte,
  including the fifty-entry resume-position cache, stream pause/reopen
  sequence, nested critical sections, and ten-step volume fades.
  `soundManager::Open` is independently fixed by vtable slot zero, the
  Dreamcast decorated signature, its unique Miles startup/import surface,
  and the `Device: ` literal. Its complete 703-byte semantic body models the
  PCM fallback loop, emulated-driver retry, Bink/Smack setup, primary-buffer
  mute, and sample-handle allocation. After four source-shape probes it is
  bounded at **84.53%** with the same 17-branch, one-return inventory; the
  residual is two retry-edge targets/block placement plus global EBX/ESI
  allocation. The refreshed build/delink/build checkpoint reaches
  **1834/2227 exact at
  96.73984% fuzzy**, 2158 source claims and **41.11% executable coverage**.
  Every ratchet, banked-row, claim, single-view, and cleanliness gate is
  clean. The wall census contains 393 residual functions (27.0 KiB
  recoverable), while 212 tractable unclaimed functions span 193.2 KiB; the
  four promoted RVAs no longer appear in the unclaimed inventory. No
  externally sourced implementation body was used: retail bytes and xrefs
  drove the reconstruction, with Dreamcast evidence used only for identity,
  names, and types.

- **2026-08-24 — NextArmy's two retail-only turn-scan helpers are admitted;
  the move-order predicate is exact and the Azure-Dragon fear check is
  semantically closed.** Neither 0x464d40 nor 0x464f50 appears in the
  Dreamcast `cmbtmgr.obj` roster, so both deliberately retain address-ordinal
  names. Their retail bodies and sole caller nevertheless settle the types
  and behavior. `Unnamed464f50(best, candidate)` compares turn-state bit 24,
  arrow-tower and catapult priority, pass-dependent speed, side, then slot;
  preserving the full inline `Is(24)` words through its first early return
  reproduces all 291 bytes. `Unnamed464d40(selected)` rejects immune and
  Azure stacks, resolves hypnosis-adjusted controller side, counts opposing
  Azure Dragons, makes the ten-percent roll, consumes the turn, and performs
  the FEAR.WAV/message/effect sequence outside quick combat. Its 525-byte
  candidate is bounded at **91.10%** after four source-shape probes: all 19
  branches and five returns agree, while retail and this VC6 invocation
  choose different registers and loop-strength reductions. The refreshed
  build/delink/build checkpoint reaches **1831/2223 exact at 96.74% fuzzy**,
  2154 source claims and **41.00% executable coverage**; the score ratchet,
  banked-row, claim, single-view, and cleanliness gates are clean. The wall
  census now contains 392 residual functions and 26.9 KB recoverable; no
  externally sourced implementation body was used.

- **2026-08-24 — the tractable-gap sweep adds ten exact rows and one bounded
  sound-worker residual.** `hiscore.obj` is now 14/14 exact: retail's
  0x898-byte manager span resolves to 22 0x64-byte records, letting
  `ResetHighScores`, `Open`, `ViewHiScore`, the 16-argument
  `CHighScoreEdit` constructor, and the signed-short threshold walker
  `GetMonType` reproduce byte-for-byte. `soundManager::GetSampleInfo` is
  exact from its Miles volume/status imports and remote callers;
  `WaitEndSampleThread` is semantically closed at 92.0%, with all 13 branches
  aligned and only EAX/ESI allocation differences remaining across the two
  inlined guards after four source-shape probes. The combat sweep adds exact
  `combatManager::mark_tower_army` from 27 cross-TU callers, plus exact
  `TCombatControlSubWindow::set_rollover` and both disable passes from the
  three four-slot vtables; the shared `ret 4`/`ret 8` folds and Dreamcast
  decorated publics settle the two otherwise-empty slot signatures. The
  build/delink/build checkpoint reaches **1830/2221 exact at 96.75% fuzzy**,
  2152 source claims and **40.97% executable coverage**. The score ratchet,
  banked-row, claim, single-view, and cleanliness gates are all clean; no
  externally sourced implementation body was used.

- **2026-08-24 — `CAdvMgrNetMsgHandler`'s deleting destructor is admitted
  exact from its now-live class model.** The 33-byte body at 0x4077b0 is slot
  0 of the class's unique five-slot vtable at 0x63a684, and the current
  `advmgr.obj` independently emits the corresponding `??_G` public. Candidate
  and retail have identical wrapper bytes and relocation topology: call the
  implicit five-byte destructor, conditionally delete, return `this`. The
  target still gives that implicit callee a generic working label, a cosmetic
  relocation-name delta that the exact-byte verdict correctly ignores. The
  earlier note correctly rejected attributing this row to `advManager`, but
  its instruction to wait for the network-handler model had become stale.
  `VA_COMPGEN` now
  pairs the right compiler emission without falsely claiming the implicit
  ordinary destructor. The build/delink/build cycle adds a second exact row
  this session, reaching **1820/2210 exact**, 2141 source claims, and 40.89%
  executable coverage; all gates remain clean.

- **2026-08-24 — the monster-quest defeat notification is admitted at its
  bounded 72.61% compiler-generation wall.** The unclaimed-span census also
  exposed the 69-byte `type_monster_quest` slot-10 body at 0x56ed40. Retail's
  four field comparisons and final `defeated_by = player` store establish the
  semantic body and the `TQuestPosition, int` argument shape already inferred
  from the shared base slot. The candidate has the same state guard, field
  comparisons, store, and calling convention. Its remaining structural delta
  is stable across four source families: this VC6 invocation combines the
  adjacent y/z bitfields into one masked test and three branches, while retail
  preserves separate masks and four branches. The claim adds one reviewed row
  without moving the exact count: **1819/2209 exact at 96.74% fuzzy**, 2140
  source claims, and 40.89% executable coverage; all gates remain clean.

- **2026-08-24 — the templated ostringstream vbase-destructor closure is
  admitted exact, and the claim join now handles MSVC `??_D?$...` owners.**
  The tractable unclaimed-span census exposed the 20-byte body at 0x5594f0
  between `ResourceManager::AddToCache` and the common resource reporter.
  Its only caller family is the TU's two missing-resource ostringstream
  paths; the emitted `resourcemanager.obj` public independently identifies
  it as `std::basic_ostringstream<char,...>::\`vbase destructor'`. Candidate
  and retail both adjust `this` by 0x50, call the same complete destructor
  and `basic_ios` destructor, then return, matching all bytes. The existing
  claim-only vbase-destructor channel already handled non-template owners
  such as `ostrstream`, but `_demangle_key` retained the leading `?$` for a
  template class and therefore could not join an otherwise identical carcass
  declarator. Stripping that compiler template marker down to the stable
  owner name, with a negative-control selftest, fixes the general `??_D?$`
  case without adding a new annotation kind. The build/delink/build cycle
  adds one exact row and advances the linked inventory from 1818/2207 to
  **1819/2208 exact at 96.75% fuzzy**; all gates remain clean.

- **2026-08-23 — `CSpriteFrame::DrawCreatureImpl` was semantically closed at a
  bounded 93.77% C1 scheduling wall; superseded by the 95.90% 2026-08-28
  ratchet above.** With alpha
  zero, retail dispatches raw/tile encodings to `DrawTile` and encoding 3 to
  `DrawAdvObjImpl`; the general path otherwise clips and decodes the same
  dword-offset scanlines as `Draw`. Literal runs either install palette pixels
  or half-blend them with the destination. Encoded runs darken existing pixels
  by one half or by one quarter plus one half, skip unsupported codes, or
  install the requested outline color for codes 5--7. Both forward and
  horizontally reversed paths are reconstructed. Moving each run counter into
  its switch case raised the renderer from 81.42% to 92.47%; promoted color
  temporaries in the four half-shade cases give the 93.77% wall. Block-scoping
  the line-offset table recovers the displaced setup-block sizes but scores
  93.58%; direct half-shade expressions restore reverse default-tail merging
  but score 92.47%; explicit 32-bit alpha temporaries score 88.55%; and
  mask-first expressions are byte-flat. `why-reg --model --il-order` finds the
  same first callee-saved definitions on both sides (EDI=sw, ESI=sx, EBX=sh),
  classifying the residual as later C1 handle state rather than a
  source-nameable ordering edit. The admitted inventory is now 1818/2207 exact.

- **2026-08-23 — `CSpriteFrame::Draw` was semantically closed at a bounded
  94.35% C1 scheduling wall; superseded by the exact 2026-08-28 closure.** Retail selects
  the tile renderer for raw/tile encodings and the adventure-object renderer
  for encoding 3, then clips and decodes encoding-1 scanlines from a dword
  offset table. Each run is `(code,count-minus-one)`; the TU-installed 255
  marker selects literal palette indices, other codes are repeated colors,
  and transparent blit skips repeated runs. Forward and horizontally reversed
  output paths are byte-semantically symmetric. The reconstruction has the
  exact 44-branch/four-return sequence and all 88 retail CFG blocks. A single
  reused clipping scratch recovers retail's clip schedule, `for` scanlines
  plus `do/while` runs recover its control flow, and declaring the output
  cursor before decoder state recovers the EDI/homed-skipped allocation.
  Dreamcast CodeView independently proves the sole named local is `const
  unsigned int* aLineOffset`; placing it before the guarded literal-run static
  gives the best score. Entry-, post-static-, post-Clip-, block-scope and
  shared-map variants were flat or worse before the later row-destination
  lifetime changed C1 state. `why-reg --model --il-order` found
  identical first callee-saved definitions (EDI=sw, EBX=sx, ESI=dx), bounding
  the five remaining size-only blocks to later C1 handle state rather than a
  source-nameable creation-order edit at that plateau. The later exact closure
  invalidates this historical residual classification.

- **2026-08-23 — fourteen adjacent `CSprite` draw adapters are exact.** The
  retail bodies select a `CSpriteFrame` through the sequence/frame arrays and
  forward the crop, destination and palette arguments to the specialized
  frame renderer. `DrawCreature` calls `DrawCreatureImpl` with alpha zero;
  `DrawAdvObj` and `DrawHero` call `DrawAdvObjImpl` with flag color zero; and
  `DrawHeroAlpha` calls `DrawAdvObjWithFlagAlpha` with flag color zero. The
  pointer adapter supplies the complete frame dimensions, while the interface
  adapter supplies transparent-blit one. The shroud adapter makes paired tile
  and tile-shadow calls on one selected frame; declaring its palette alias
  before the frame alias reproduces retail's ESI/EDI assignment exactly. The
  other tile, shadow, hero, spell-effect and flag-color paths are direct
  forwarders. All fourteen close without walls, advancing the inventory from
  1804/2191 to 1818/2205 exact functions at 96.75% fuzzy.

- **2026-08-23 — ten adjacent CSprite/CSpriteFrame ownership and construction
  rows are exact.** `ResourceManager::GetSprite` proves the retail calls and
  overloads for the 280-byte `CSprite` constructor, `AllocateSeq`, the
  frame-pointer `AddFrame`, and both importing `CSpriteFrame` constructors.
  The `CSprite` constructor reproduces the complete inlined sixteen-entry
  resource-type switch: creatures allocate 22 sequences, heroes 18, combat
  heroes 5, the single-sequence sprite families 1, and the remaining explicit
  domain 0. Keeping all zero-valued 65/72/74..79 arms is significant—without
  them C1 shortens or compresses the jump table. Member-initializer spelling
  places the derived vptr store after the six initial field stores and closes
  the last constructor bytes. `AllocateSeq` and `AddFrame` then reduce exactly
  to `CSequence` construction and forwarding. The compact and cropped frame
  constructors are exact on their first scored forms: both create a type-64
  resource, use `csize ? csize : cropped-area` for `DataSize`, allocate/copy
  the encoded payload, and install the byte-proven dimensions, crop rectangle,
  pitch and encoding fields. The same pass closes `CSprite`'s generated scalar
  deleting destructor, ordinary destructor, `SetPalette(const unsigned
  short*)`, and `ResetPalette`. Destruction releases every live sequence,
  both sequence arrays and both virtual palette resources before the resource
  base. Reset reconstructs 24- and 16-bit stack palettes and installs a heap
  copy. Dreamcast's decoration for the header helper is `AAVTPalette16`, a
  reference—not the stale pointer rendering in the generated game tree;
  restoring `SetPalette(TPalette16&)` and passing the constructor temporary
  retains its returned `this` in EDI exactly as retail does. The batch advances
  the inventory from 1794/2181 to 1804/2191 exact functions, at 96.75% fuzzy.

- **2026-08-22 — the retail ResourceManager archive/cache/sound/sprite tranche
  is reconstructed: thirty-one exact rows and eleven bounded walls.** Cross-build
  names are admitted only after the PC bytes prove the ABI and data layout.
  The archive pool consists of eight 0x190-byte interleaved slots: an archive
  pathname at 0x69d870 plus the 0x18c-byte `LODFile` subobject at +4. Four
  active-context rows at 0x69e538 each contain three
  `(count,index-list)` pairs. That model makes the sprite and bitmap
  `PointTo*Resource` walkers exact at five blocks / 131 bytes each. The
  17-byte shared reader is exact as well: it receives its `LODFile *` in ECX,
  preserves the destination in EDX, pushes the byte count, and calls
  `LODFile::read`. `GetBackdrop` is exact across all three blocks, including
  its `Bitmap816::Draw`, virtual `Dispose`, resource-type 16 and common
  missing-resource reporter call. `GetBitmapResourceSize` has the exact
  three-block flow and 30 of 31 instructions; retail alone retains a dead
  context-address `lea`. Pointer, reference and inline-accessor spellings
  remove it, a by-value spelling emits a real 24-byte copy, and both why-reg
  paths reject a binding explanation, bounding the 96.77% residual to C1
  dead-address materialization. `resource::Dispose` is exact at all 11 blocks
  / 161 bytes: it decrements a live reference, constructs the 13-byte cache
  key, follows Dinkumware's inlined lower-bound/find path, erases the hit, and
  invokes virtual deletion with flag 1. The out-of-line 32-byte cache-key
  constructor, `Close`, `SetPath`, and `SetPixelFormat` are also exact.
  Complete's 753-byte `Open` extends Dreamcast's two-`bool` signature with an
  `int*` error output proved by its sole caller and outer catch handler. It
  walks the active sprite and bitmap archive lists, opens each interleaved
  slot's pathname, throws the retail 0/1 error classes, and retains the shipped
  oddity of reserving an eight-index rollback vector without appending to it.
  Its seven-state/two-try EH map, 36-block count, sixteen branches and three
  returns agree. The 74.6250% residual is one positional STL boundary: retail
  expands `reserve`'s `_Ucopy` but calls `pop_back`'s `_Destroy`; SP3 makes the
  opposite choices, shifting one frame slot and downstream registers. A
  zero-through-eight inert-site ladder, the branch/register solvers and an RTM
  C2 A/B exhausted the known levers; RTM is byte-identical to SP3.
  CodeView's `char[261]` type supplies SetPath's otherwise-hidden four bytes
  of frame padding; SetPixelFormat matches all 22 blocks, including the three
  destructive mask scans; and Close matches its iterator deletion, range
  erase, and eight-file clear loop. Four newly claimed public cache getters
  (`GetBitmap16`, `GetPalette`, `GetFont`, and `GetText`) reproduce the same
  six-block / 138-byte family as `GetSpreadsheet`. Restoring Dinkumware's real
  `std::pair<const char*,resource*>` conversion in `AddToCache` closes that
  helper and all five getters exactly; this was the source model behind their
  previously shared fourteen-slot insertion schedule. Keeping the
  archive bodies and inline find surface late in the TU avoids perturbing
  earlier C1-sensitive functions. `RemapGraphics` and `SaturateGraphics` add
  two more exact rows (485 and 545 bytes): their 81-byte switch tables prove
  the dispatched retail resource types, and their scoped Dinkumware
  `auto_ptr` temporaries reproduce both exception states and virtual cleanup.
  Dreamcast's `TPalette24* CSprite::p24` field is independently confirmed by
  retail's palette-header offset, while the attested `Bitmap16Bit*` Draw
  forwarding overload supplies the otherwise-hidden C1 inlining boundary
  needed for the exact blit register schedule. The 576-byte `LoadText` and
  `LoadSpreadsheet` twins are exact as well, along with their stdio/LOD Read
  adapters and both implicit scalar deleting destructors. Retail's LOD Read
  convention returns the requested size when `LODFile::read` reports zero;
  its extra `not` fixes that ABI. The loaders' last cleanup-order detail comes
  from the header-inline empty `TAbstractFile` virtual destructor: C1 erases
  its dead stack-vptr reset, while the generated deleting destructors retain
  the real base-destructor call. An inner buffer scope then places `delete[]`
  before `fclose` on the ordinary-file path, matching all 21 blocks. The two
  font-loader rows are now admitted as bounded walls. The 361-byte stream
  helper is at 86.7419% with the exact header/payload reads, font construction,
  palette cache/refcount path, `SetPalette`, and virtual `Dispose` semantics.
  Retail proves a pointer-only unwind guard here, distinct from Dreamcast's
  independently attested 8-byte `TResourcePtr`; the provisional
  `TScopedResourcePtr` records that PC-only shape without corrupting the
  canonical cross-build type. why-reg confirms that every first register
  definition agrees and bounds the residual to the later 15-vs-17-block cache
  CFG/homing choice. The 553-byte file/archive wrapper reaches 95.6292%; its
  two archive searches and fallback/error behavior agree, while the remaining
  dominant difference is the established VC6 midpoint where retail inlines a
  string temporary's destructor but leaves `_Tidy(true)` out of line. No
  `inline_depth` value expresses that boundary; depth zero, retained here,
  is the best measured form. `GetPalette24` adds the same bounded class at
  94.7045%: Dreamcast and the HD masked identity fix its public name, while
  retail proves the 24-byte header, 256-`TRGBA` payload, ordinary/archive
  adapters, `default.pal` fallback, resource-type 96 reporter calls, and the
  global saturation transform. Candidate and target each contain 220 body
  instructions; the residual is the same string teardown boundary followed
  by an EBX/EDI C1 allocation permutation. why-reg's only legal declaration-
  order proposal worsens the distance, and branch-local read buffers double
  the frame rather than recovering retail's slot coloring. `LoadPalette` is
  bounded at 95.6920% with the same complete file/archive and fallback
  behavior, plus the proven six-global pixel-format conversion tuple. An
  explicit file/archive `else` lets C1 reuse one `TPalette24` slot and recovers
  retail's exact 0x758-byte frame; an inner ordinary-path scope also places
  its destructor before `fclose`. The remaining 22-vs-24-block split is the
  shared string `_Tidy` midpoint and an ESI/EDI front-end-state permutation;
  why-reg's only legal declaration reorder worsens its distance 67 -> 75.
  The adjacent 904-byte `LoadBitmap16` body is exact across all 39 blocks:
  file and archive sources converge through a temporary `Bitmap24Bit`, its
  12-byte `{DataSize,Width,Height}` header and both ownership guards reproduce
  retail cleanup, and the resulting 24-to-16 draw agrees instruction for
  instruction. `inline_depth(1)` reaches the string `_Tidy` midpoint here;
  restoring Dreamcast's header-inline `GetWidth`/`GetHeight` accessors then
  fixes the final argument-register order. The 1,055-byte `GetBitmap816` is
  now reconstructed end to end: cache lookup, ordinary-file constructor,
  active-LOD and `default.pcx` fallback walks, 12-byte header, owned pixel
  data, 24-to-16 palette conversion, saturation, both cache insertion forms,
  cleanup and reference accounting. Separating its ordinary-file return from
  the archive tail raises it to 94.4307%; the residual 32-vs-34-block split
  is the same positional string boundary (retail inlines `c_str` but calls
  `_Tidy(true)`) and its resulting 20-byte frame/slot-coloring displacement.
  why-branch found every archive-loop rewrite flat or worse, why-reg found no
  viable creation-order edit, and RTM C2 emits the SP3 object exactly. Its
  newly claimed 44-byte public map-insert wrapper is independently bounded at
  99.7222%: every byte but the returned success-flag load width agrees, and
  volatile/transport spellings only add instructions. The public `GetSample`
  getter then closes exactly through the adjacent 0x55c3c0 loader, and
  retail's two byte-identical 86-byte cache-find layers are exact after
  keeping both out of C1 auto-inlining. Finally, `CSprite::Dispose` is exact
  across all 25 blocks / 280 bytes: it releases every live sequence frame and
  then reproduces the inlined base cache removal. Its retained 23-byte
  lower-bound facade is exact as well; expressing the aggregate-return ABI as
  its explicit result pointer preserves the two live caller iterator slots,
  and copying the selected iterator value recovers retail's final register
  flow. The Complete-only sound path then adds exact `GetSoundFile` and
  `LoadSample` bodies. The former is exact across 41 blocks / 655 bytes and
  proves the three active sound-header descriptors, 48-byte header records,
  extension-free case-insensitive lookup, scalar `auto_ptr` transfer, and
  positioned Win32 reads. The latter is exact across 25 blocks / 854 bytes:
  it probes a temporary resource-path string, reads an ordinary file as one
  `size`-byte item with catch/rethrow cleanup, falls back through the sound
  archives and `default.wav`, emits both diagnostics, and constructs the
  sample with retail's 0/127/1 tuple. The adjacent virtual
  `sample::GetSize` is exact as well. The newly claimed common missing-resource
  reporter is semantically closed but bounded at 77.39%: its twelve named
  resource cases, `0x` hexadecimal default, seven-part message and MessageBox
  call agree; retail expands four late string assignments while this C1
  expands one. Direct `operator=`/`assign`, explicit length, force-inline and
  depth probes do not express that boundary, and why-reg identifies the
  remaining register order as non-source-nameable C1 handle state. Complete
  also adds its sprite-family twin at 0x5599e0. That 1,096-byte
  reporter has the same fastcall ABI and message construction, plus all eleven
  named sprite cases and the hexadecimal default. It is bounded at 70.5441%:
  why-reg sees 298 later string-expression slots processed in a different
  order despite identical first register definitions, and depth/declaration
  probes either stay flat or perturb already-exact later rows. Finally,
  `GetSprite` is reconstructed end to end at 88.86%. Its cache lookup, two
  active-archive walks, 0x310-byte DEF header, sequence/name/offset copies,
  compact and cropped frame paths, frame cache insertion, palette conversion
  and final sprite insertion all agree semantically; its 45 branches and three
  returns are exact. The surviving 73-vs-72-block split is one inner-loop ESI
  reload followed by an ESI/EDI/EBX allocation permutation. A prechecked
  `do/while` recovers retail's frame-index/name-offset initialization order;
  register, volatile, declaration and loop-form probes cannot recover the
  later C1 handle state, and why-reg finds no first-definition permutation to
  move. This work advances the inventory from 1762/2140 to 1794/2181 exact
  functions, at 96.74% fuzzy.

- **2026-08-22 — the PC quest factory and its implicit destructor family are
  exact: eight newly claimed rows, no explicit destructor definitions.**
  Retail 0x573240 is the nine-arm `create_quest` factory: the subtract-one
  jump table, allocation sizes, vtables and payload stores independently map
  h3m quest types 1..9 onto the nine derived classes. Its common 95-byte
  callee at 0x56cb80 is `type_quest::type_quest(unsigned char)`; three empty
  strings precede the body assignments, which select the text table, choose
  `rand()%3`, and set the last base dword to -1. The frameless factory proves
  the TU's nothrow `operator delete` contract, while `auto_inline(off)` on the
  base constructor reproduces retail's nine calls. The nested monster-field
  assignment fixes the last scheduling slot and makes all 23 factory blocks,
  including the jump table, exact. Crucially, those constructor calls make
  VC6 emit the derived virtual destructor symbols naturally. Claim-only
  annotations then bind the ICF-shared simple-leaf body/wrapper and the
  distinct artifact- and creature-vector wrapper/body pairs: all six are
  byte-exact without declaring or defining a derived destructor. The batch
  advances the compiled inventory from 1754/2132 to 1762/2140 exact, at
  96.83% fuzzy.

- **2026-08-30 — Dreamcast local order and two Complete team-index encodings
  are restored in `fill_prohibited_array`; one SIB byte remains.** The
  mandatory dossier for Dreamcast ai_player.obj+0x2f694 records
  `human_strength`, `income[7]`, `short i`, and `resources[7]` in that order,
  followed by the initial income/dwelling-cost phase and the separate
  `human_strength = 0` statement. The source now preserves those facts, and a
  fatal source-shape contract plus negative controls rejects either reordered
  locals or moving the zero into the declaration. Spelling both Complete-only
  team lookups directly with `gNetLocalGamePos` changes VC6's commutative SIB
  selection to retail without changing the 1,017-byte function, its 76 blocks,
  311 instructions, 39 branches, or single return. This raises the function
  from 99.903534% to 99.96784%. The sole residual is the same base/index choice
  in `gpGame->playerDisabled[player_index]`; a named load, named `game*`, and
  reversed subscript are byte-flat, while why-reg classifies the wall as
  aligned scheduling with a post-first-definition binding difference. The
  Dreamcast `is_human_ally` calls and 119-creature bound remain explicitly
  older-revision facts: Complete's x86 body proves team-aware checks and a
  145-creature pass.

- **2026-08-22 — the last unclaimed ai_player row is closed at a 99.90%
  encoding wall.** Retail 0x429d50 is `fill_prohibited_array`: end_turn's
  unique caller supplies its 145-byte result, the body starts from seven turns
  of `turnProductionResource`, subtracts dwelling growth costs, and Complete's
  two Easy-only phases cap computer-only teams below the strongest projected
  human dwelling value. Dreamcast CodeView supplies the exact top locals
  (`human_strength`, `income[7]`, `short i`, `resources[7]`) and the adjacent
  `sum_player_dwellings` helper; VC6 inlines that helper twice in retail. The
  reconstruction has the exact 76-block CFG and differs only at three
  commutative SIB base/index encodings. A reversed-subscript probe was
  byte-flat, bounding the residual to post-RA address encoding rather than
  game behavior. This claims the final 1,017-byte in-span ai_player row: the
  inventory is 1754/2132 exact, with this function retained as the sixth
  bounded wall in the cluster. The temporary
  `HOMM3_AI_PLAYER_CREATURE_LEVEL_VIEW` used during reconstruction is now
  retired as well: `TOWN_DWELLING_COUNT - 1` names the same zero-based top
  tier, keeps the three-instruction encoding residual unchanged, and leaves
  town.obj at its prior score without widening `TCreatureTypeTraits`.

- **2026-08-22 — the ai_player garrison/purchaser cluster is identified end
  to end: 9 exact bodies and 5 bounded compiler walls.** The retail vtable
  resolves 0x428580 as `type_garrison_purchaser::mark_town`; its artifact
  query and army call expose Complete's purchaser extension as two distinct
  bytes (`allow_trade` plus `has_angelic_alliance`). Retail then proves the
  swapper's ten-byte alignment census, with only the surrounding ABI gaps
  left implicit, and the Complete elemental/Angelic-Alliance grouping rules.
  Exact bodies are garrison `mark_town`, `type_creature_source`'s constructor,
  the swapper constructor, `get_alignments`, `add_creatures`, the town
  purchaser constructor, `set(town*)`, and `AI_consolidate_army`. The ninth is
  the already-emitted 38-byte implicit purchaser destructor: an
  `IMPLICIT_DTOR` claim binds VC6's existing named COFF public without adding
  any destructor declaration or definition to C++. The five
  semantically closed residuals are `choose_weakest_army` 88.34% (41/42
  blocks, 135/140 instructions), `value_of_adding_army` 89.09% (67/68,
  287/292), `do_best_purchase` 97.27% (217/219 instructions), `do_purchase`
  97.53% (exact 90-instruction multiset/CFG), and `get_purchase_value` 97.33%
  (all 13 blocks and all 86 instructions). Their in-tree notes record the
  exhausted `why-reg`, volatile, declaration, expression, call-form and
  direct-loop probes; the remaining differences are VC6 register allocation,
  stack coloring or post-RA scheduling rather than missing game behavior.
  The batch advances the ratchetable inventory from 1745/2117 to 1754/2131
  exact (82.3%, 96.82% fuzzy).

- **2026-08-22 — implicit special members stay implicit; claim-only compiler
  output requires an already-emitted VC6 symbol.** Explicit derived
  destructors used as emission probes were removed: although they can force
  VC6 to produce similar bodies, they change the source model and therefore
  are not admissible matches. `VA_COMPGEN` instead gained
  `DEFAULT_CTOR_CLOSURE` for the 24-byte
  `std::vector<CObjectType>::`default constructor closure' already emitted
  by `NewfullMap`'s member construction in `mapcell.obj`; the claim pairs it
  at 100% without a C++ definition. Like `SCALAR_DELETING_DTOR`, this is a
  named COFF symbol joined directly against the base object, so both kinds
  bypass the anonymous compiler-function canonicalizer. The same rule admits
  the contiguous retail COMDAT run containing four already-emitted vector
  destructors (`CObjectType`, `TreasureData`, `MonsterData`, `BlackBoxData`)
  and `vector<CObjectType>::size` through `VECTOR_DTOR` / `VECTOR_SIZE`; all
  five pair at 100% without source definitions. The same inventory sweep
  then admitted another eleven exact named COMDATs: `TSeerHut::resize`, two
  vector sizes, two event-vector destructors, five vector insert/erase
  members, and `bitset<10>::_Tidy`. The source join selftest carries the real
  mangled-name regression checks, and `homm3 delink` plus the fast build
  validate the end-to-end path. A following seven-helper batch added six
  more exact `_Destroy` / `_Ucopy` / `_Ufill` rows. The seventh,
  `vector<TTownEvent>::erase`, is a documented 53.87% nested-inliner wall:
  retail expands the implicit base assignment where this VC6 context calls
  it, and an inline-depth probe was byte-flat. COFF section order then proved
  the owners of two otherwise folded 16-byte-record COMDATs
  (`HeroPlaceholderData::erase`, `RandomDwellingData::insert`); those and the
  stride-unique `generator::erase` add three more exact rows. The next six
  claim-only rows are exact as well: `generator::insert`, `std::copy<int>`,
  `std::copy<type_university>`, and three `std::_Construct` instantiations
  (`MonsterData`, `TSeerHut`, `TQuestGuard`). The 37-byte `copy<int>` body is
  also the `/OPT:ICF` destination used by `copy<pathCell **>` call sites;
  `mapcell.obj`'s COFF section order identifies `copy<int>` as the primary
  owner, immediately before the event/university copy instantiations. The
  full batch left the ratcheted inventory at 1731/2085 exact functions. A
  second six-row identity sweep then exposed why layout-only padding must
  also remain implicit: explicit pad members made VC6 copy bytes retail
  skips. Removing the byte-proven alignment/tail members from `TTimedEvent`,
  `TTownEvent`, `TreasureData`, `BlackBoxData`, and `type_creature_bank`
  preserved every offset and class size while closing five helpers:
  `copy<type_creature_bank>` and `_Construct` for `TreasureData`,
  `BlackBoxData`, `TTimedEvent`, and `TTownEvent`. `copy<TTimedEvent>` remains
  a documented 96.50% CSE wall with an identical 36-block CFG: retail hoists
  and reuses `string::npos`, while this instantiation folds its first use to
  -1 and reloads it later. That batch reached 1736/2091 exact. A claim-only
  `IMPLICIT_COPY_ASSIGN` kind then admitted five named `??4` symbols without
  defining any operator in C++. Making `TScenarioTown`'s four alignment gaps
  implicit closed `_Construct<TScenarioTown>` and raised its assignment from
  56.57% to 96.71%. Its assignment and those for `MonsterData`,
  `BlackBoxData`, `TTimedEvent`, and `TTownEvent` retain identical CFGs but
  the same `string::npos` hoist/constant-fold choice; their five residuals
  are documented together at the claims. A final three-body sweep added the
  already-emitted `vector<TArtifact>::operator=`,
  `vector<SecondarySkillData>::capacity`, and
  `std::copy<SecondarySkillData>` symbols. Retail bytes corrected two initial
  hypotheses here: the pointer span in the 19-byte member reaches the
  end-of-storage field, proving `capacity` rather than `size`, while the
  43-byte copy loop receives its first two arguments in ECX/EDX and pops only
  the result argument, proving the global fastcall `std::copy` rather than
  the member `_Ucopy`. All three are exact after correcting their identities;
  the inventory reached 1740/2100 exact. The following vector-clear sweep
  admitted five more genuine emitted bodies. Retail element strides and the
  adjacent `NewfullMap::Load` member accesses prove `CObjectType::erase`,
  `TreasureData::{insert,erase}`, `MonsterData::erase`, and
  `BlackBoxData::erase`. Removing `CObjectType`'s named alignment bytes raised
  its erase helper from 49.09% to 91.19%; retail also proved that the old
  three-byte tail actually contains an aligned 16-bit field at +0x42, with
  only +0x41 implicit. The four erase residuals now have identical CFGs and
  only the established `string::npos` fold/hoist register fallout.
  `TreasureData::insert` remains 84.00% (49 vs 48 blocks); `why-branch`
  classifies its extra return as VC6 exit-merge/jump-threading topology in an
  already-generated pristine-vendor specialization. All five are documented
  compiler walls, leaving the inventory at 1740/2105 exact. The next five
  call-site/stride-proven vector bodies produced one exact match,
  `vector<TTownEvent>::insert`. `MonsterData::insert`, timed-event
  insert/erase, and `TScenarioTown::erase` retain the same CFGs as retail
  (52, 50, 10, and 9 blocks respectively); their residuals are confined to
  the same npos CSE and consequent register/stack-slot allocation. They are
  documented at their claims, taking the inventory to 1741/2110 exact. The
  same doctrine then admitted the 62-byte one-string
  `TreasureData::~TreasureData` body through a direct-symbol
  `IMPLICIT_DTOR` claim. The C++ destructor remains implicit;
  `mapcell.obj` already emits its real `??1TreasureData` public first among
  the byte-identical destructors folded to the retail address. The joined
  body is exact, raising the inventory to 1742/2111. The adjacent 88-byte
  `??_ENewmapCell` body received the same treatment through a direct-symbol
  `VECTOR_DELETING_DTOR` claim: retail and base agree on the flags, array
  cookie, vector-destructor iterator, and both conditional delete paths.
  It too is exact without defining a special member, for 1743/2112. The
  first remaining retail-only mapcell game body was then decoded from its
  `NewfullMap::Load` call site: 0x4fd950 reads a 16-bit quest-guard count,
  resizes the five-byte `TQuestGuard` vector, loads each row, and appends each
  non-null quest to `mapObjectData`. A masked `int` plus a guarded do/while is
  the best measured source spelling at 90.4916%; all 19 branch decisions
  agree, with the residual confined to register/stack scheduling through the
  two inlined vector operations. The candidate must carry scoped
  `auto_inline(off)`: otherwise /Ob2 expands the entire 616-byte callee into
  `NewfullMap::Load`, dropping that established caller from 92.201836% to
  64.12% (a call-site `inline_depth(0)` recovers only 84.27%). The row is a
  documented compiler wall, so enrolling it changes the inventory to
  1743/2113 exact. The next retail-only body, 0x4fe6c0, is the per-cell
  save-version compatibility pass called by `loadMapLayer`. Its lookup table
  proves seven object arms: ARTIFACT, DEAD_GUY, MONSTER, PYRAMID,
  TREASURE_CHEST, WAGON and WARRIOR_TOMB. Dreamcast CodeView supplies the
  legacy named bitfields, while retail proves their current widened/shifted
  destinations. Keeping a distinct legacy dword reproduces all masked field
  transfers at 96.26% with the same 14-block flow; three arms are
  instruction-exact, and the remaining four differ only in EAX/EDX/ESI role
  selection. The resulting one-byte code-size difference moves the otherwise
  identical switch tables across the next four-byte alignment boundary.
  Direct aliasing (69.53%), a shared pre-switch snapshot (92.55%), reversing
  the monster index conversion (94.52%), case-local storage, `register`, and
  declaration-order probes were rejected or byte-flat. The gated conversion
  views live in `mapcell.h`, not as `.cpp`-local shadow types. Enrolling this
  second compiler wall takes the inventory to 1743/2114 exact. The following
  569-byte retail-only body is the post-load shipyard pass. Its Read-tail call
  and NewfullMap member offsets independently locate it; a uniquely
  byte-identical HD twin supplies the name `NewfullMap::LoadShipyards` only
  after that retail proof. Retail `.data` supplies the twelve signed adjacent
  `(dx,dy)` pairs. Nesting the three-cell coordinate propagation inside the
  successful direction arm lets VC6 eliminate the explicit found test and
  count stack slot, reproducing all 27 blocks and every instruction exactly.
  The offset table is exact too. This raises the inventory to 1744/2115. The
  other large retail-only row has a uniquely byte-identical HD twin named
  `NewfullMap::SoD_transformRandomDwellings`. Retail proves the 16-byte input
  record, its signed faction/owner/level fields, the reverse 136-byte town
  lookup, both generator tables, and the width-bound passable/trigger-mask
  scan. Postfix-decrement conditions on both reverse searches plus reuse of
  the horizontal offset as FindTrigger's x output reproduce all 667 bytes
  and 36 blocks. With that last large mapcell row exact, the inventory is
  1745/2116. The final claimable row in mapcell's retail span is the
  421-byte nullary helper at 0x5042c0, called immediately after object-type
  deserialization by both readMapObjects and loadMapObjects. It first clears
  the resolved 16-bit index in every record of the 232 per-class vectors,
  then searches the appropriate vector backwards for each main objectTypes
  record by `(extra, ImageName)` and writes the main-vector index. Raw NB11
  closes the Dreamcast-attribution question: the mapcell procedure roster
  has only `$E482..$E485` between `loadObjectType` and `readMapObjects`, and
  both DC callers proceed directly from deserialization to sprite rebuilding.
  `readMapObjects` records function locals `int_buffer`, `numObjects`,
  `count`, `x`, and `i`, plus block-local `v`; `loadMapObjects` records only
  `int_buffer`, `count`, and `x`. None owns this lookup's locals or scopes.
  The helper is therefore a Complete-only boundary proved by the two retail
  calls, not a missing Dreamcast symbol. Fatal source rules on both callers
  preserve the explicit call and its retail-proved placement while making no
  claim about a DC-local inventory for the helper itself. The
  reconstructed body reaches 99.88535% with the same 32-block CFG and the
  same 157-instruction multiset. Its sole code delta is a post-RA transpose
  of one ESI reload across the adjacent EAX/EDI reloads; `why-reg` confirms
  no binding divergence, and the tested expression, condition, index-scope,
  signedness and declaration-order spellings either leave the schedule
  unchanged or worsen it. A 2026-08-30 bounded recheck made a named
  `CObjectType&` and split guard byte-flat at 99.88535%; moving `extra` after
  `typeIndex` retained 421 bytes but fell to 95.93630%. The other asm-report
  row is a symbolic-only
  relocation: mapcell's Dinkumware `_Nullstr` and the target's pooled
  source-owned empty literal both resolve to 0x63a608, whose physical
  DATA_COMPGEN owner is another compiland. This is a documented compiler /
  pooled-COMDAT wall, enrolling the row at 1745/2117; mapcell's sole
  remaining unclaimed in-span row is the deliberately excluded 42-byte
  static-destruction cinit shim at 0x504290.

- **2026-08-20 — clang-IR extraction LANDED (lane/ir-extraction); the
  2026-08-19 declination was wrong on both of its stated grounds.** That
  entry declined the port because IR "would change names" and because
  `verify_va_claims` "already owns" the completeness sweep. Measured, both
  are false. Names: the IR channel binds 1396 function claims across 116
  TUs and **every one is character-identical to the name the declarator
  join already produced** — `build/gen/symbol_names.csv` comes out with 0
  name changes, 0 rows added or removed, and identical unit/size/kind.
  Completeness: `verify_va_claims` checks a VA site's uniqueness, census
  reconciliation, universe class and file order, but never asks whether the
  site reaches a fragment; the ported sweep finds **119 sites that reach no
  fragment at all** (see the two findings below).

  What the port buys, both defects reproduced first in a scratch tree:
  * *Positional theft.* The declarator scan names a claim, then a lossy
    demangle key (case-folded, `::`→`_`, template arguments dropped) matches
    it against the base obj, falling back to EXACT-content-size pairing when
    the group counts differ. A claim on `Widget::Value(int)` whose retail
    size coincides with a HEADER-inline `Widget::Value()` bound to the
    inline — `?Value@Widget@@QBEHXZ` in the label map, channel `src-VA+base`,
    exit 0, no gate. 72 authority keys tree-wide carry more than one mangled
    member and reach those paths. IR pairs the annotation with the symbol in
    the compiler, so there is no key and no size heuristic to fool.
  * *Stale artifact.* Change a claimed function's parameter type and do not
    rebuild: `homm3 labels` reports "0 fragment(s) changed" and the label map
    keeps answering with the old overload's mangling, a symbol the current
    source can no longer produce. Now cl's obj only CONFIRMS the name clang
    paired: a string the obj does not define is refused, reported, and the
    claim keeps its raw declarator label rather than being handed to the
    weaker key match against that same disputed object.

  Cost, honestly: **no `core.msvc_names` equivalent was needed.** With `/Gr`
  passed (without it 171 free functions mangle `@@YA` where cl writes
  `@@YI`), clang's Microsoft mangler agreed with cl on 1396/1396 confirmable
  claims, so the port VERIFIES against cl's spelling instead of deriving it
  — and a future divergence surfaces as a loud unconfirmed claim, never as a
  silently wrong name. The real cost is `build/gen/msvc-include`: a
  generated lowercase symlink mirror of the 992 VC6 headers (they ship
  UPPERCASE, so clang cannot open `<bitset>` on a case-sensitive
  filesystem), with 7 of them carrying a conformance patch for a VC6-ism cl
  accepts and clang rejects. Each patch is argued mangling-neutral in
  `homm3.core.clang.PATCHES`, and the corpus-wide confirmation rate is that
  argument's negative control, re-run on every extraction.

  Residual, deliberately left on the declarator join and now COUNTED rather
  than assumed sound: 166 rows — 98 `??_G` scalar-deleting-dtor claims (keyed
  by owner class, no declarator involved), 9 hand-reviewed `#if 0
  @carcass` claim-only stubs whose real body lives in a header or the STL
  (mousemgr's `TCSLock`, textresource's `vector::erase`, hero's bitset/vector
  COMDATs — each already carrying a written argument in the source), and
  army.cpp's, the one manifest TU clang cannot parse (`_cpp_max` overload
  resolution at army.cpp:1731). 116 of 117 manifest units parse clean.

  Numbers: `homm3 build` exit 0 at 1421/1809 exact (78.6%), 71.90% fuzzy,
  ratchet clean; `homm3 delink` exit 0 through model → synth PDB → vostok.
  Extraction costs 7.3 s wall for the whole tree (116 clang probes, thread
  pooled), and only on the `homm3 delink` path — `homm3 build` never runs it.

- **2026-08-20 — FINDING (header-VA member resolved 2026-08-22): 119 macro sites reach no fragment.**
  Surfaced by the ported completeness sweep, three classes.
  (1) **100 `DATA()` sites in `include/`** — extraction reads `src/*.c*`
  only, so a header claim never becomes a row; they sit on `extern`
  declarations, which `include/va.h` already forbids ("never on a header
  extern"). (2) **1 `VA()` site in a header**: `include/game.h:722` claims
  `SaveAbstractString` at 0x4bbb60, and that address carries the dense
  working label `game_b9270_sub00_bbb60` in the label map instead. On
  2026-08-22 the writer was reconstructed exactly in `src/game.cpp`, its
  source claim enrolled the row, and the stale header floor entry was removed. The IR
  channel *finds* this one (it is the single claim clang names that cl's obj
  does not define, because the header only declares it), which is how it was
  confirmed. (3) **18 `DATA_COMPGEN`/`DATA_COMPGEN_GUARD` sites in `src/`**
  that clang-format wrapped across lines — `scan_file` matches those macros
  per LINE, so e.g. `src/advmgr.cpp:919` and `src/herodefs.cpp:78,133` are
  dropped silently. gruntz already has the fix (balanced-paren, quote-aware
  scanning; its docstring names this exact hazard). Not fixed here because
  fixing it ADDS label-map rows, which is a separate change with its own
  justification burden; this port holds at 0 label changes.

- **2026-08-19 — the labeling monolith restructured into gruntz's modern
  `retail_labels/` + `model.py` shape (lane/gruntz-port), byte-identical.**
  Gruntz has evolved past the layout this plan describes (its 1411-line
  `labels.py` became `retail_labels/` - typed `Claim` records, parse-only
  censuses/providers, a per-TU fragment cache under `build/gen/claims/`,
  extraction public as `gruntz labels` - plus the one-join top-level
  `model.py`). Ported here as a pure restructure of `build/labels.py`:
  `homm3.retail_labels` (censuses / providers / iat / fragments / source),
  `homm3.model`, `homm3.manifest`, `homm3.core.tsv`; CLI verbs
  `homm3 labels` and `homm3 model`; `homm3 delink` chains labels -> model.
  Every mechanism moved unchanged (lexical VA scan, base-obj authority
  join, put-order policy, all fatal gates); fragments keep the RAW
  declarator name beside the joined spelling so the model replays the
  monolith's scan-order dedup exactly. Proven by running old and new into
  scratch: `symbol_names.csv` and `compgen_claims.tsv` byte-identical
  modulo the generator header lines, and row-identical to the live
  last-delink outputs; `homm3 build` green at 1421/1809 throughout.
  Deliberately NOT ported: clang-IR extraction (would change names - the
  P0.2 interim declarator+base-obj binding stays; the IR channel now has a
  clean seam inside `retail_labels/source.py`), `bindings.tsv`/alias
  records (`symbol_names.csv` IS the binding file here; duplicate rva
  stays fatal), gruntz's derived-extent censuses (our functions census
  admits explicit sizes), and its completeness sweep (the
  `verify_va_claims` build gate already owns that job).
  **SUPERSEDED 2026-08-20 (see the head of this log): both declination
  grounds were measured false - the IR channel changes 0 names, and
  `verify_va_claims` never checks fragment coverage, which is how 119
  uncovered macro sites went unnoticed. Both are now ported.** One quirk fixed
  in passing, behavior-equivalent by analysis: extraction-time join keys
  come from raw names rather than post-dedup suffix-stripped ones.

- **2026-08-15 — `army::get_controller` / `get_owner` CLOSED, and the merged
  reading was right.** `0x442690` is `get_controller`, `0x4426d0` is
  `get_owner`, `combatSide` (+0xf4) holds the OWNING side. The decisive proof
  is not semantic and not the order-map: the DC dump names the field itself
  through two separate procs — `army::get_owning_side` (Army.h:795, dc
  `0x27d3c`, 8 B) is a bare load of the side field, `army::get_controlling_side`
  (Army.h:800, dc `0x27d44`, 0x30 B) tests the hypnotize word and returns
  `1 - get_owning_side()`. Retail's pair is the same isomorphism one level up
  (`heroes[flip]` vs `heroes[raw]`), and a full `call rel32` scan reproduces
  every DC caller multiplicity (19 sites on `0x442690`, 2 on `0x4426d0`;
  `compute_fire_shield_damage` 2:0, the `TViewArmyWindow` ctor 2:1,
  `is_computer_action` 1:0, `ProcessCombatMsg` 0:1). The old third argument —
  `is_enemy`'s asymmetric compare — is WITHDRAWN as a proof: it is symmetric
  and answers a hypnotized `arg` wrongly under either reading. No row moved.

- **2026-08-15 — `army::can_shoot` re-banked at 92.0000 with
  `get_AI_target_time` still exact; the merge's "claimed OR expanded" trade is
  closed.** VC6 emits an `inline` function's out-of-line copy exactly when the
  TU holds a use it DECLINES to expand — measured four ways (no keyword:
  can_shoot 92.0000 / caller 27.8667 / `get_berserk_targets` **0.0000**, i.e.
  without the keyword the body is not an inline candidate anywhere, which is
  C1XX's body-save cliff and not the `/Ob2` budget; keyword on the definition
  or on the header declarator: caller 100.0000 and no copy; keyword + one
  rejected site: **both**). Retail's rejected site is
  `spell_is_valid_on_target` (`0x447a80`, two calls at `0x447c0e`/`0x447cb9`),
  the only body in retail's `army.obj` that calls `0x4428f0` out of line and
  the reason its object carries the copy at all. Until that body is
  reconstructed the site is supplied by a `#pragma inline_depth(0)` SCAFFOLD
  around an un-carcassed, deliberately unclaimed `get_total_combat_value`;
  retiring `0x447a80` retires the pragma and makes `0x442e60` claimable.

- **2026-08-15 — new fatal gate `homm3.match.banked_rows`: a banked function
  may not leave the ledger** (pipeline extension). The ratchet only compares
  rows that are IN `config/match_baseline.tsv`, so a row that leaves the file
  is invisible — which is how `?can_shoot@army@@QBEEPBV1@@Z` (83.2927) was
  lost at the lane/army-6 integration: army-6 forked before townmgr-14 banked
  it, the merge took army-6's generated baseline, and every gate stayed green.
  The gate walks the tracked revisions of the baseline (read-only `git log -p`;
  the status writer remains the sole TSV writer), collects every retail RVA
  that ever carried a positive banked score, and fails when one is no longer
  represented by ANY row. **Identity is the RVA, never the label** — fifteen
  const-qualification/label-promotion renames landed in that one merge and a
  label-keyed check would have drowned in them. Deliberate withdrawals are
  admitted in `config/match-banked-waivers.tsv` (va-claims-backlog shape).
  Negative controls: an embedded `selftest()` runs on every invocation
  (removed row detected, same-RVA rename NOT reported, never-matched and
  RVA-less rows ignored, waiver silences exactly one RVA), four hermetic cases
  in `homm3.match.test_status`, and the live control — deleting the can_shoot
  row from the real baseline reports `LOST 0x0428f0 … (banked 83.2927%)` and
  exits 1. Replayed against the merge commit `0f768de` it reports exactly that
  one row and zero false positives across the whole history.

- **2026-08-13 — `combatManager::place_shooter` raised 80.57% → 82.49%.**
  The DC roster fixes its function-scope locals as `best_hex`,
  `best_open_hexes`, `new_hex`; retail initializes the first two before
  `SeedCombatPosition`, not after it as the prior reconstruction did. The
  three wait outcomes now share one tail, reducing the compiled shape from
  four returns to retail's two. Both sides have 21 conditional branches;
  the only branch residual is the equivalent final polarity. The remaining
  mismatch is a cyclic allocation family: retail homes `this` in EBX and
  later recycles EBX as the open-neighbour counter, while this VC6 spelling
  chooses ESI and spills the counter. Positive-gate nesting (82.21%) and an
  explicit final normal arm (81.29%) were measured and rejected.

- **2026-08-13 — `combatManager::choose_cyclops_action` is byte-exact
  (441/441) on its first paired build.** The DC record supplies the one
  automatic local (`count`) and function-static const `walls[4]`; retail
  proves that array at 0x63abd0 as wall rows `{1,2,4,5}`. The body counts
  surviving targets, calls the independently identified `find_AI_targets`,
  sums only armies without an existing AI target, applies retail's double
  precision 1.2× value threshold, and uses `Random(1,count)` to choose among
  the weakest positive-strength walls before emitting combat order 9. The
  static datum is now source-owned with `DATA`, not represented through a
  shadow view. Engine total: **981/1369 exact (71.7%)**, **53.14% fuzzy**.

- **2026-08-13 — `combatManager::choose_shooter_target` is byte-exact
  (544/544).** Retail xrefs and the DC local roster recover the complete
  enemy-stack scan, including the otherwise unused `std::vector<army*>`
  whose EH frame is present in retail. The function prices Magog/Lich splash
  attacks over both occupied hexes, rejects dead/simulated-zero targets, and
  prefers able stacks before comparing attack value. Its 38-block CFG and
  27-branch sequence agree exactly. Three VC6 source-order decisions close
  the remaining allocation gap: declare both side indices before the zeroed
  flag/pointer, write the two disability preferences as independent tests,
  and spell the acceptance assignments best-army/output-value/best-hex even
  though VC6 schedules the stores best-hex/best-army/output-value. Removing
  `army::IsIncapacitated`'s no-inline pin made the new body structurally
  right but regressed exact `find_move_order` to 84.39%; expanding the same
  three-field predicate at the two retail-inline sites preserves both exact
  functions and the one retail out-of-line call. Engine total: **980/1369
  exact (71.6%)**, **53.06% fuzzy** before the final synchronized ratchet.

- **2026-08-13 — the fake town-name string view is deleted without a
  codegen regression.** `town_rollover_name_storage_view` duplicated the
  Dinkumware `std::string` already proven at `town+0xc4` merely to expose
  its nullable backing pointer at +4. `advManager::SetRolloverText` now
  holds the returned town as `const town*` and reads `cName.begin()`; the
  const overload is the canonical direct pointer load and preserves the
  function's 89.8242% retail score exactly. The non-const overload was
  rejected because its copy-on-write `_Freeze` path lowered the function to
  88.9698%, and `c_str()` was rejected at 89.5832% because it substitutes a
  static empty string for null. The remaining `HOMM3_*_VIEW` switches are
  measured include-closure/layout personalities, not duplicate overlay
  structs; removing them changes VC6 code generation in sensitive TUs.

- **2026-08-13 — the fake 0x6a5d5c general-text view is deleted.**
  `InitializeGeneralText` already proved that this global is the canonical
  `TTextResource*` returned for `genrltxt.txt`; the surrogate
  `SUnnamed6a5d5c::entry` at +0x20 was merely duplicating the real
  `TTextResource::Text` vector's `_First` pointer. All source references
  now use `gpGeneralText` with a named `EGeneralTextIndex` value, and both
  surrogate structures and their conditional layout personalities are gone.
  This correction made seven functions exact: `executive::CallManager`,
  `playerData::GetName`, `game::GetPlayerName`, `hero::get_backpack_error`,
  and the three `TViewArmyWindow` hitpoint/speed widget builders. It also
  raised `MainMenuHandler` from 89.65% to 90.07% without regressing
  `recruitUnit::Update`. Coverage is **979 / 1369 (71.5%)**, 52.95% fuzzy;
  the single-view and all cleanliness counters remain zero.

- **2026-08-13 — `THeroScreenWindow::UpdateHeroLocator` is byte-exact
  (203/203), with no fake UI view.** Retail and Dreamcast jointly identify
  `THeroTraits::smallPortraitName` at +0x30; the live `hero::portrait` member
  selects that canonical row. The screen's +0x60 tail is now the real
  `topHero` scroll origin, while 0x698a84 is named
  `gHeroScreenHeroPosition` because `HeroView` stores
  `GetLocalPlayer()->FindHero(gpCurrentHero->id)` there immediately before
  setup. The function uses canonical `playerData`, `game`, `hero`, and
  `THeroTraits` layouts throughout. The first compile reached 99.86%; fixing
  the retail `BroadcastMessage(codeX, codeY, id, extra)` argument order made
  all 203 bytes exact and raised synchronized coverage to **972 / 1369
  (71.0%)**, 52.83% fuzzy.

- **2026-08-13 — `advManager::DisableButtons` and `EnableButtons` are
  byte-exact (376/376).** While the global adventure manager is active, the
  two fully unrolled twins clear or set `WIDGET_ACTIVE` on the canonical
  navigation ids 3–14, 30 and 31. Checking `gpAdvManager->status` rather than
  the receiver's status is the source-significant retail detail. The same
  DC/base evidence admits `gpAdvManager` as a reviewed relocation owner. The
  project reaches **961/1369 exact (70.2%)**.

- **2026-08-13 — `hero::Fly` is promoted from a 0% carcass to a complete
  94.2468% reconstruction (248 retail bytes).** Retail sets `flightLevel`,
  obtains the current magic terrain from the packed hero coordinate, asks
  `get_spell_level` for spell id 6, charges that row's per-mastery mana cost
  with a floor of one, and expands the already-exact `UseSpell` mana/UI tail.
  A call-site `inline_depth(0)` preserves retail's out-of-line spell-level
  call; the residual is confined to the packed point-equality register
  schedule. The DC-attested operator and local/direct variants compile
  identically, so no conditional API or fake view is retained. Aggregate
  fuzzy coverage rises to **52.03%** while exact coverage remains
  **959/1369 (70.1%)**.

- **2026-08-13 — `combatManager::LearnSpellFromEagleEye` is byte-exact
  (136/136), and its masked-diff semantic bug is fixed.** Unmasked retail
  bytes read hero +0xd0, canonical secondary-skill slot 7 (Wisdom); the old
  source incorrectly used +0xd4, slot 11 (Eagle Eye). The reviewed relocation
  alias identifies the `akSpellTraits` data cell, while the exact 163-byte
  Dinkumware `set<int>::const_iterator::_Inc` COMDAT is admitted to the runtime
  name map from the base object's authoritative public. The project reaches
  **959/1369 exact (70.1%)**.

- **2026-08-13 — `hero::GetMobilityFrame` and `hero::GetManaFrame` are
  byte-exact (129/129).** Their former 90.4%/88.9% register residuals came
  from direct-return source spelling. Giving each conditional ladder one
  shared result local lets VC6 retain the signed-division quotient in EDX,
  choose ECX for the mana threshold tail, and still duplicate every retail
  return. The project reaches **958/1369 exact (70.0%)** with no view or
  register-forcing construct.

- **2026-08-13 — `army::FlyTo` and `army::TeleportTo` are byte-exact
  (236/236).** The masked assembly diff had hidden the decisive branch-target
  mismatch: retail always plays the stand animation after movement and gates
  only the corrective turn on `restore_facing`. Expressing that CFG also makes
  VC6 select retail's EBX/EDI allocation, closing both former 94% residuals
  without a register-forcing view. The project is now **956/1369 exact
  (69.8%)**.

- **2026-08-13 — `hero::GetHighestSchool` is byte-exact (115/115).** The
  surviving CodeView symbol supplies the missing `const` qualifier. Retail
  reads the low byte of the `TSpellSchool` parameter for its four mask tests
  but reloads the complete enum for the fallback result; a volatile byte
  access preserves those distinct widths without introducing a fake enum or
  layout view. The project is now **954/1369 exact (69.7%)**, with all
  fake-view and cast cleanliness counters still zero.

- **2026-08-13 — `TAdventureMapWindow::UpdateSpellButton` is byte-exact
  (76/76), and a false inline-wrapper mapping is retired.** The function
  enables casting only for a nonnegative-owner hero while the current player
  is local human. An absent/non-local hero broadcasts the dim state, while a
  present negative-owner hero returns without broadcasting; that asymmetric
  nested guard is retail's. The normal path broadcasts `WIDGET_CLEAR_STATUS`
  or `WIDGET_SET_STATUS` for the CodeView-named `CAST_SPELL_ID`, carrying the
  canonical DIMMED and UPDATE flags. The complete 0..38
  `TAdventureMapWindow::EWidgetIDs` roster is now canonical source data, so no
  magic widget view was introduced. The randomly sampled 0x404200 row was
  also disproven as `button::set_hotkey`: its `ret 0xc` and full body identify
  the three-argument Dinkumware `vector<int>::insert`; the one-argument
  header wrapper is inlined and has no standalone retail body. This match also
  activated the first reviewed relocation alias (`gpCurrentPlayer` at the
  sole 0x403bfb site), exposing and fixing two dormant pipeline omissions:
  `homm3 delink` now passes the alias manifest to vostok, and `labels` seeds
  each reviewed alias owner into the synthetic PDB before delinking.

- **2026-08-13 — the last duplicated hero obscuring view is removed, and
  `type_obscuring_object::load` / `save` are byte-exact (428/428).**
  Dreamcast CodeView proves that both `hero` and `boat` derive from the
  0x18-byte `type_obscuring_object`; the prior hero model redundantly repeated
  that prefix and eleven call sites needed `void*` bridge casts to recover the
  base. The canonical base now owns the retail-proven packed +0x07
  `obscured_location`, the DC-named `valid`, `was_trigger`, and `extra_info`
  members, and both real inheritance edges. Direct inherited calls reproduce
  every previously exact function, so the duplicate prefix and all eleven
  bridges are deleted. The two 214-byte serializers read/write the eight
  fields in retail order. Their CodeView-attested `bool(void*)` ABI and a named
  final bool local are source-significant: direct comparison return emits
  `sbb/inc`, while the local emits retail's bare `setae al`.

- **2026-08-13 — `advManager::DoAdventureOptions` is byte-exact
  (248/248).** The reconstruction uses the canonical
  `TAdventureOptionsWindow`, `advManager`, and `game` interfaces throughout;
  no local or fake layout view was needed. Dreamcast names both dispatched
  game members (`play_recorded_events` and `ShowScenInfo`) and supplies the
  option IDs (`VIEW_WORLD_ID`, `VIEW_PUZZLE_ID`, `VIEW_SCENARIO_ID`, `DIG_ID`,
  and `REPLAY_ID`). Retail proves the modal window's lifetime ends before the
  dispatch switch. VC6 then reproduces the retail block order when the source
  cases are ordered view-world, puzzle, replay, scenario, dig, even though the
  jump-table IDs remain 1, 2, 5, 3, 4. The one narrow declaration personality
  around `ViewWorld` prevents the canonical `TSkillMastery` enum from leaking
  into TUs which own the incompatible `ai_tactical.h` typedef.

- **2026-08-13 — the adjacent hero terrain lookups are byte-exact (429/429).**
  Both pack the hero's three coordinate shorts into the canonical five-byte
  `type_point`, compare it with the all-minus-one invalid point, and index the
  canonical world-map cell array with the proven 38-byte stride. The
  Dreamcast-named `hero::get_special_terrain` (215 bytes) returns the
  no-magic-terrain sentinel when unplaced and otherwise delegates to
  `NewmapCell::get_magic_terrain_type`; the retail-only preceding member (214
  bytes, still ordinal-named) returns `NOTHING` or delegates to
  `NewmapCell::get_special_terrain`. Direct array access is source-significant:
  routing through the out-of-line `cell` accessor has the same semantics but
  does not reproduce retail's inlined address math.

- **2026-08-13 — `TTimedEvent::Save` is byte-exact (183/183), and the
  canonical timed-event layout is corrected.** Retail serializes the message,
  seven resource quantities, player mask, distinct ApplyToHuman and
  ApplyToComputer bytes, first-day word, and interval word. The prior model
  omitted ApplyToHuman and consequently misnamed/misaligned its tail. Retail
  Save/Load prove the two byte fields and offsets; NH3API contributes the
  ApplyToHuman name only after those offsets are independently established.
  The ignored one-byte human-flag write uses a byte temporary, reproducing
  VC6's reuse of the dead parameter home without any access-only view.

- **2026-08-13 — `advManager::UpdBottomView` is byte-exact (320/320).**
  The dispatcher preserves override 8 as the disabled state, expires timed
  overrides with the retail signed tick-delta test, and maps override values
  1/2/3/4/6/7 to NewTurn/Kingdom/Hero/Town/Resource/Message. The retail jump
  table proves value 5 exits without changing the view. With no override, a
  local human receives the selected hero, town, or NewTurn view unless the
  complete-map draw mode is active; all other paths select EnemyTurn. A
  changed view is drawn only when requested, with the update byte normalized
  to boolean. The original shared update label is required for VC6 to retain
  retail's distinct switch/default call blocks; this is genuine control-flow
  structure, not an artificial layout shim.

- **2026-08-13 — the bottom-view message producers are reconstructed:
  `BVMessage` 92.68% and `BVResMsg` 93.33%.** Both assign the canonical
  +0x3a8 Dinkumware string, set their override type and deadline, and force
  `gpAdvManager->UpdBottomView(1,1,1)`; the resource form additionally writes
  the proven type/quantity pair, uses a 5000-ms deadline, and updates the
  resource display. Their CFGs and all non-string instructions agree. Their
  common residual is one VC6 library-inline choice on assigning an empty
  shared string: retail calls `_Tidy(0)`, while this invocation clears the
  representation words directly. The bodies also prove retail 0x4040f0,
  0x404bd0 and 0x60ab3b as `basic_string::_Tidy`, `_Copy`, and `std::_Xlen`,
  replacing flat runtime/helper labels globally.

- **2026-08-13 — three `advManager` bottom-view updaters reconstructed exact:
  `UpdBottomViewNewTurn` (176/176), `UpdBottomViewResMsg` (175/175), and
  `UpdBottomViewMessage` (161/161).**
  A non-forced refresh of an already-active NewTurn view
  only calls `animate_bottom_view(0)` and reports no replacement. Every other
  path clears the old view, installs type 1, allocates the retail-proven
  0x48-byte `TBottomViewNewTurn`, installs it through the canonical
  `set_bottom_view` API, and refreshes the resource display. The allocation
  fixes the real derived extent without exposing its unread 0x14-byte tail.
  The resource-message sibling proves the manager's canonical +0x3a0 type,
  +0x3a4 quantity and +0x3a8 `std::string` state, passes all three to a
  storage-free 0x34-byte `TBottomViewResourceMessage`, and does not refresh
  the resource display. The ordinary-message twin passes that same canonical
  string to a 0x34-byte `TBottomViewMessage`. Its producer `BVMessage` is now
  behavior-complete at 92.68%: the only code delta is retail calling
  `basic_string::_Tidy(0)` on an empty shared string where this VC6 invocation
  clears the representation inline; three normal assignment overloads are
  byte-identical and the iterator-range form is worse. Dreamcast supplies the
  class/constructor identities. Synchronized coverage reaches 945/1368 exact,
  51.61% fuzzy and 13.27% executable, with all cleanliness gates still zero.

- **2026-08-13 — `NewmapCell::is_diggable` reconstructed from a 0% carcass
  to 96.16%.** Retail rejects Water, Rock and impassable ground, resolves a
  hero or boat to the object hidden underneath it, accepts NOTHING and the
  HOLY_GRAIL marker, and scans every layered object on ANCHOR_POINT cells to
  reject a `TERRAIN_HOLE` overlay. Canonical `NewmapCell`, `NewfullMap`,
  `CObject` and `CObjectType` members express the whole operation; no local
  fake view or cast was introduced. The resulting 14-branch/two-return CFG is
  exact. Only VC6's propagation of NOTHING through the two inlined obscurer
  arms remains: three natural assignment/helper spellings compile identically,
  while retail materializes zero and rejoins the shared type tests. The body
  also independently admits `eTerrainRock = 9` alongside the already-proven
  Water enumerator.

- **2026-08-13 — retail 0x60ab30 identified as scalar `operator delete`.**
  Its complete 11-byte body is `push argument; call _free; pop ecx; ret`, so
  the runtime map now carries the VC6 `??3@YAXPAX@Z` identity instead of a
  caller-derived flat label. This removes a false relocation-name residual
  from `Bitmap816::~Bitmap816` and any other reconstructed destructor using
  the same runtime thunk; the destructor's remaining 99.97% delta is EH
  metadata naming, not executable instructions.

- **2026-08-13 — the two live `Bitmap24Bit` constructors closed their
  smallest-module residuals.** The data constructor rises from 92.70% to
  99.51% after restoring retail's zero-size rule: allocate and copy `w*h*3`
  bytes when no compressed size is supplied. Its five-block graph is now
  exact; only VC6's ordering of three independent member/vptr stores remains,
  after sweeping the meaningful field orders. The pathname constructor is
  exact (192/192 bytes): Dreamcast CodeView type 0x2894 proves its local is
  `char[261]`, and retail independently requires the corresponding four-byte
  larger aligned frame. Coverage is 942/1368 exact and 51.44% fuzzy.

- **2026-08-13 — `THeroScreenWindow` destructor and scalar-deleting wrapper
  reconstructed exact (162/162 and 33/33 bytes).** Retail proves the class
  derives directly from `CAdvPopup`: the destructor installs vtable 0x63eae8,
  restores a live dragged artifact to `gpCurrentHero`, resets the mouse and
  selected army slot, deletes the canonical `Widgets` vector entries, and
  invokes the base destructor. The adjacent dragged-artifact dwords are one
  real `type_artifact`, initialized together to -1; 0x697738 is the hero-screen
  army-slot index used throughout the same UI paths, disproving NH3API's
  shifted `puzzlePiecesRemoved` label. Making the inheritance and overrides
  real also migrates the already-exact `ExitDialog` to its correct virtual
  mangling. No replacement view or cast is involved. Synchronized coverage
  is 941/1368 exact, 51.43% fuzzy, and 13.22% executable matched; all
  single-view and cleanliness counters remain zero.

- **2026-08-13 — `advManager::LoadRemote` reconstructed exact (217/217
  bytes).** Retail loads the generated remote-control save path from the
  canonical preferences record, calls `game::LoadGame`, optionally writes
  `orig.dat`, and preserves the four calendar selectors that LoadGame clears.
  The Dreamcast local roster supplies the selector names and the one-byte
  `CHourGlass` type; retail independently proves its constructor, explicit
  `Stop`, and second scope-exit stop. A first complete spelling reached
  96.07%; restoring the explicit stop raised it to 99.67%. The remaining
  four stores then disproved NH3API's shifted label mapping: direct
  compiler-to-retail instruction correspondence fixes month/month-extra at
  0x697750/0x6983fc and week/week-extra at 0x697748/0x698834. Correcting that
  mapping and retail's week-first restore order closes the function exactly.
  Coverage is 939/1367 exact, 51.40% fuzzy, and 13.21% executable matched.

- **2026-08-13 — `town::can_build` raised from 81.19% to 89.60%.** Its
  ten-branch/four-return graph and all building rules were already exact;
  the gain comes from giving the faction byte the stack home used by retail,
  which VC6 reproduces when the narrow local is volatile. An explicit
  promoted integer erased the home and returned to 81.19%, while making the
  short building parameter volatile over-constrained every access and fell
  to 67.61%; both probes were discarded. The remaining delta is bounded to
  the bit-mask index and short-parameter register family.

- **2026-08-13 — five `TViewArmyWindow` stat-widget helpers admitted.**
  `create_attack_widget` and `create_defense_widget` are exact (654/654 bytes
  each); `create_hitpoints_widget` is 99.9585% across 663 bytes with an exact
  17-branch/two-return graph, `create_hitpoints_left_widget` is 99.9565%
  across 637 bytes with 16 exact branches, and `create_speed_widget` is
  99.9611% across 702 bytes with 19 exact branches. The previously unclaimed
  helper band between the window
  handler and the known action-widget creators follows the Dreamcast source
  roster. Retail independently identifies this member through its two
  `textWidget` allocations at (154,48), the attack label/value IDs 205/206,
  the shared primary-skill table's attack entry, and the `%d` / `%d(%d)`
  formatting paths; the defense twin changes only its row and IDs. Hitpoints
  uses the same recipe with the central text record's +0x614 label; remaining
  health and speed prove the corresponding +0x324 and +0x308 fields. Speed's
  zero-first `_cpp_max` calls reproduce retail's by-value/reference clamps.
  All three residuals are the equivalent EDX/ECX schedule used to follow the
  two-pointer label chain; a named entry-pointer probe on hitpoints regressed
  to 98.09% and was discarded. The canonical widget vector and real `TViewArmyWindow` layout
  reproduce these bodies without a view or cast. The shared
  primary-skill table declaration moved from a cpp-local extern into
  `game.h`, keeping the cleanliness gate at zero. Synchronized coverage is
  938/1367 exact, 51.36% fuzzy, and 13.20% executable matched.

- **2026-08-13 — `NewmapCell::get_special_terrain` reconstructed from 0%
  to 79.09% (278 retail bytes).** The body now reproduces the hero-obscured
  and direct-garrison special cases, walks the canonical cell-object vector
  backwards through the retail CObject/CObjectType pools, and accepts exactly
  CURSED_GROUND, MAGIC_PLAINS and the eight SoD magic-terrain IDs in retail
  comparison order. The cell flag word now has a canonical whole-word overlay
  under mapcell's full-layout configuration; no cast or fake view was added,
  and all pre-existing exact consumers remain exact. A switch probe scored
  74.61% and was discarded. The residual is VC6 tail-merging the two special
  cases (three returns versus retail's four) and the resulting register homes.

- **2026-08-13 — `TSystemOptionsWindow::WindowHandler` raised from 86.31%
  to 91.92% by restoring the retail help-switch source order.** Placing the
  six main-menu/dialog IDs before the contiguous system-option range makes
  VC6 keep the discrete mapping beside its callers instead of outlining it
  after the right-click return. A single-join/range-first probe reproduced
  the old 86.31% layout and was discarded. The remaining difference is the
  retail hot/cold placement of that same inlined mapping; all later option
  cases retain the same semantics and six-return shape.

- **2026-08-13 — `sacrifice_window.obj` entered with two exact callback
  widgets and no replacement fake view.** Retail vtables 0x641578 and
  0x6415b0 prove the slot-13 identities at 0x55fd10/0x55fd40; both bodies
  read the canonical iconWidget parent (+4) and derived index (+0x48), then
  call the independently carved `type_sacrifice_window::backpack_click`
  (0x5636c0) or `offering_click` (0x563a80). The real window derives from
  CAdvPopup and is 0x23c bytes: constructor 0x55fdd0 proves current_hero at
  +0x60 and the final VC6 vector triplet through +0x23c, exactly matching
  the DC roster after the known base/vector-width deltas. Both 38-byte
  handlers are exact; the build moves 934/1360 to 936/1362 exact and the
  cleanliness gate remains at zero local/fake views and casts.

- **2026-08-13 — `seerhut.obj` entered with an exact retail-only SoD
  constructor, and the last unnecessary fake vector views were removed.**
  The quest/seer bracket between `search.obj` and
  `singleselectionpopups.obj`, two `NewfullMap` construction callers, the
  0x13-byte retail vector stride, and an independently verified cross-build
  body identify 0x573580 as `TSeerHut::TSeerHut`. Retail proves the canonical
  packed model: a 5-byte `TQuestGuard` base (`type_quest*` plus visited-player
  mask), a 12-byte reward, name index at +0x11, and state byte at +0x12. A
  no-store protected base-construction path lets the reward type initialize
  before the base assignments, reproducing all 19 retail bytes exactly.
  `TSeerHutVectorView` and `TQuestGuardVectorView` are gone; `advmgr` now
  indexes the real Dinkumware vectors. The separately proven 0x6c-byte
  `type_creature_bank` record moved to a lightweight shared header, allowing
  `game::creatureBanks` to become `std::vector<type_creature_bank>` and
  retiring `type_creature_bank_vector_view` as well. The canonical container
  expansion deliberately moves `SetRolloverText` 89.71→89.69 and
  `get_creature_bank_help_text` 87.28→87.13; both historical peaks remain in
  the ledger, no exact function regressed, and the single-view and cleanliness
  gates remain at zero. Coverage is 934/1360 exact across 130 units.

- **2026-08-13 — `searchArray::BuildPath` rose from 69.73% to 86.33%.**
  Dreamcast CodeView proves that `type_point`'s constructor and equality pair
  are header-inline and that both comparisons are const-reference methods.
  Reconstructing the two source and destination locals through the constructor,
  using the equality operations at all three decision sites, and placing the
  loop-state initialization before the initial result clear restores the
  retail point-packing order and most of VC6's register lifetime. The helpers
  remain TU-local because adding their declarations to shared `struct.h`
  measurably regresses the exact definition-count-sensitive
  `initialize_game_data`; this is source-shape isolation, not a data view.
  A negated-equality implementation of `operator!=` scored 64.78%, and a
  result-vector reference scored 12.40%, so both were rejected. The residual
  is bounded to VC6's inline-budget split for the pointer-vector erase/insert
  helpers: retail keeps six calls where this build expands their bodies. The
  synchronized aggregate remains 933/1359 exact across 129 units, while fuzzy
  coverage reached 50.97%; all gates are clean.

- **2026-08-13 — `singleselectionwindow.obj` entered the build with
  `CNetPlayerHandler::DeletePlayer` exact (77/77 bytes).** Dreamcast supplies
  the source identity and signature; the retail body independently proves
  the eight-entry human-player array, its 0x7c-byte element stride, and the
  `dpid`, hero, town, and player-position stores. The separately identified
  retail player constructor closes the canonical Windows record: unlike the
  28-byte Dreamcast base, retail's `CNetPlayerInfo` has a version dword at
  +0x1c and the derived fields begin at +0x20. The complete handler is two
  eight-record arrays followed by four dwords, not a function-local view.
  The original `GetNetPos` and `Clear` helpers inline naturally under /Ob2
  to reproduce the retail loop and stores. The remaining 264-function
  carcass stays fenced. Aggregate coverage reached 933/1359 exact across
  129 units, 50.95% fuzzy, and 13.01% executable matched with every gate
  clean.

- **2026-08-13 — `TSystemOptionsWindow::WindowHandler` is behavior-complete
  and reaches 86.31102% across its 1566-byte retail body.** Retail proves the
  two-case widget dispatch, all sparse main-menu commands, the 42-entry option
  range, preference writes, radio-button updates, unavailable-audio dialogs,
  sound-manager volume adjustment, six toggles, command confirmation and the
  hover sample. Re-reading the canonical preference globals after each write
  reproduces retail's selector and volume blocks exactly in structure and
  instruction count; spelling `codeX` as its natural two-case switch also
  restores retail's `sub`/`dec` dispatch and EBX-held consume result. The
  remaining plateau is bounded to VC6 layout of the inlined
  `convertID2HelpID` decision tree and scheduling of the unused local
  `message` initialization in inlined `UpdateSystemOptions`; explicit-goto,
  single-return/early-return, `inline`/`__forceinline`, and shared-label probes
  were byte-inert or regressive. The reconstruction uses the complete
  canonical `TSystemOptionsWindow`, central-text and preference records—no
  access-only view. The synchronized build -> delink -> build result remains
  932/1358 exact, rises to 50.93% fuzzy and 13.00% executable across 128 units;
  ratchet, VA-claim, single-view and cleanliness gates are clean.

- **2026-08-13 — `multiplayerwindow.obj` entered the build with the five-
  function `CHotSeatDlg` tail exact (370/370 bytes).** Vtable `0x6401d8`
  independently maps the destructor, `OnWidgetDeselect`, rollover getter and
  scalar-deleting wrapper; the direct call at `0x51243c` identifies the
  adjacent `OnOK`. DC supplies the canonical class/member names and proves
  `edit` is `textWidget*[8]`, while retail corroborates its repacked `+0x50`
  start, four-byte stride, and the `std::string` payload at each widget's
  `+0x30`. DC's nested `char[8][21]` record gives `CHotSeatMan` its complete
  0xac-byte layout; retail independently proves the eight-player bound,
  21-byte name stride, copy, and post-copy count increment when its constructor
  and `AddPlayer` inline into `OnOK`. The full dialog tail is the canonical
  class (including `THelpText[20]`), not an access-only view. The synchronized
  build -> delink -> build result is 932/1358 exact, 50.67% fuzzy and 12.93%
  executable across 128 units; ratchet, VA-claim, single-view and cleanliness
  gates are clean.

- **2026-08-13 — `advspells.obj` entered the build with
  `advManager::CheckCastSpell` exact (402/402 bytes).** The retail carve fixes
  the real entry at `0x41c2f0`; the attempt-1/HD address `0x41c470` is inside
  that body and was not reused as boundary evidence. The reconstruction uses
  the canonical `TSpellbookWindow`, `hero`, `game`, terrain, spell-trait and
  central-text records—no compatibility view. Retail widens the spellbook's
  magic-terrain constructor parameter to `int`, matching the already-proven
  retail `OnMagicPlains` dword. The final VC6 source-shape lever was the
  Dreamcast-attested inline `game::GetCurrHeroId` (`Game.h:992`, dc `0x2f18`):
  spelling the initial guard through that real accessor produced retail's
  EAX load and preserved `this` in ECX for the immediately following
  `MobilizeCurrHero` call. The synchronized build → delink → build result is
  927/1353 exact, 50.63% fuzzy, 12.92% executable across 127 units; ratchet,
  VA-claim, single-view and cleanliness gates are clean.

- **2026-08-12 — `newgame.obj` entered the build with both alignment
  helpers exact (107/107 bytes), advancing aggregate coverage to 926/1352
  exact across 126 units.** Source order and retail behavior place the
  27-byte nine-bit counter at 0x5132b0 and the following 80-byte picker at
  0x5132d0, immediately before `InitNewGame`; the independently named
  scenario-info/text bodies corroborate the later TU order. Retail widens
  Dreamcast's `unsigned char legal_alignments` to `int`, preserving bit 8 for
  Conflux and consuming full ECX with no byte mask. The picker counts eligible
  towns, optionally chooses a one-based random ordinal, and returns the
  corresponding town id, defaulting to Castle. The remaining eight source
  bodies stay fenced until independently mapped.

- **2026-08-12 — `advManager::MobilizeCurrHero` is exact (157/157
  bytes), advancing aggregate coverage to 924/1350 exact.** The retail body
  selects the waiting or current `playerData`, rejects ordinary mobilization
  while a dialog is active, and reuses the current hero when present. With no
  current hero it deliberately probes both `NextHero` and `NextTown`, prefers
  the hero, and otherwise installs the town context while forwarding the
  original movement/waiting/draw flags. Dreamcast supplies the signature and
  `player`/`hID`/`tID` identities; retail fixes every field, call, branch, and
  argument order.

- **2026-08-12 — `viewwrld.obj` entered the build with its bounded
  destructor slice exact (167/167 bytes), advancing aggregate coverage to
  923/1350 exact across 125 units.** The independently identifying
  `VWorld.pcx` constructor anchors the TU at 0x5fa600; its following
  33-byte scalar deleting destructor and 134-byte destructor land at
  0x5fbd30/60. Retail proves the owned globals as the 64x64
  `Bitmap16Bit` scratch buffer and `VWsymbol.def` sprite, then destroys
  inherited widgets virtually before the canonical `CAdvPopup` teardown.
  Dreamcast supplies the real `TViewWorldWindow` identity and four-member
  tail; retail's wider popup fixes the resulting class at 0x70 bytes. The
  remaining 24 source bodies stay fenced until independently mapped.

- **2026-08-12 — the three selectable bottom-view updaters are exact
  (474/474 bytes), advancing aggregate coverage to 921/1348 exact.**
  `UpdBottomViewKingdom`, `UpdBottomViewHero`, and `UpdBottomViewTown`
  share retail's clear/type/install control flow; the kingdom arm alone
  refreshes the resource display. Retail allocations independently prove
  the three installed subclasses are 0x34 bytes, while Dreamcast supplies
  their real identities. They therefore extend the canonical bottom-view
  hierarchy rather than introducing access-only views.

- **2026-08-12 — `CAdvPopup::CAdvPopup` is exact (154/154 bytes),
  advancing aggregate coverage to 918/1348 exact.** The natural base
  initializer, four member stores, and popup-state handoff reproduce retail
  byte-for-byte. Retail independently proves `CNetMsgHandler::m_inPopup` at
  +4 and `CDPlayHeroes::m_pNetMsgHandler` at +0xf0; Dreamcast supplies the
  real class/member identities and the two inline accessor names. The shared
  networking surface now models the canonical `CDPlayLobby`/`CDPlayHeroes`
  hierarchy rather than introducing an access-only view.

- **2026-08-12 — retail-only `ImmMouseWindowMoved` is exact (154/154
  bytes).** `ClientToScreen` produces the new client origin, the helper keeps
  the previous origin at 0x696d70/74, and every tracked Immersion enclosure
  receives its offset rectangle. The out-of-line iterator increment at
  0x4b7330 proves a Dinkumware red-black tree; node payload +0x0c/+0x10 proves
  `map<CImmEnclosure*, RECT>`, and the `_Head` load at 0x696d64 fixes the map
  object base at 0x696d60. Naming remains explicitly role-derived because
  this PC-only block has no Dreamcast source row. A focused canonical header
  models the real SDK/container surface without adding an access-only view.

- **2026-08-12 — `NewfullMap::saveTimedEventList` is exact (165/165
  bytes).** The retail vector begins at map +0x80, stores 0x34-byte events,
  writes its dword count, and calls `TTimedEvent::Save` for each element with
  an immediate short-write/failure exit. Dreamcast supplies the complete
  `TTimedEvent` member roster; retail's vector divisor independently proves
  the VC6-widened layout. The canonical type and vector member are shared by
  the existing map-object surface without introducing another TU-only view,
  and the shared-header build preserved every prior ratchet score.

- **2026-08-12 — `NewfullMap::saveMonsterData` is exact (107/107
  bytes).** Dreamcast supplies the `Message`, seven-dword `ResQty`, and
  `Artifact` member order; retail independently proves the VC6-widened
  offsets +0x10 and +0x2c and the complete 0x30-byte stride. The natural
  serializer ignores `saveString`'s result, rejects any short resource
  write, and returns the final one-byte artifact write as retail's branchless
  0/-1 result. `MonsterData` now has one canonical focused header rather than
  another source-boundary view.

- **2026-08-12 — `combatManager::get_surrender_cost` is exact (178/178
  bytes), advancing aggregate coverage to 914/1348 exact.** The retail loop
  walks twenty stacks on `currentSide`, rejects empty and bit-22 temporary
  stacks, subtracts troops resurrected during this battle, and prices the
  remainder with the gold column (`cost[6]`, traits offset +0x38). It then
  halves the integer base cost and multiplies it by the current hero's
  `GetSurrenderCostFactor`. Dreamcast supplies the
  `numTroopsBattleResurrected` identity at the independently proven retail
  +0x54 offset; retail bytes select every condition, the resource column,
  loop spelling, and conversion order. All gates remain clean.

- **2026-08-12 — a mixed touched/new batch added nine exact retail
  bodies across six units.** `victorylossconditions` gained the 80-byte time
  limit check, the 83-byte defeat-hero check, and the 99-byte player-
  applicability helper; the last exposes a PC/DC return-width divergence
  because retail returns full EAX where Dreamcast records a byte. `mapcell`
  gained the 141-byte four-field `NewfullMap::saveObject`. New narrow units
  admitted all three `dxplay` GUID/host leaves, `swapManager::IsLeftHero`,
  and the trading-post market-value getter. Each identity was bounded by
  retail callers/data/vtables and only then corroborated with Dreamcast;
  no external implementation was used.

- **2026-08-12 — the latest fake-view audit removed four more dead aliases
  without moving the retail ratchet.** `victorylossconditions.obj` no longer
  enables the map-object header personality merely to inline `GetTeam`, and
  its byte read of the current player now uses the canonical
  `gNetLocalGamePos` datum rather than a second DATA name/type. The same
  `GetTeam` declaration was retired from the shared `game` class after its
  sole remaining live consumer in `mapcell.obj` proved byte-identical with a
  source-local inline helper. The puzzle-only `NewfullMap` projection was
  deleted in favor of the already-proven canonical map-object projection,
  and game.obj's point-constructor personality was replaced by an ordinary
  TU-visible inline definition. VC6 retained 913/1348 exact rows and all
  ratchet, VA, single-view, and cleanliness gates. Fresh probes also
  reconfirmed that the town-name overlay, Seer Hut/Quest Guard vector
  overlays, and adventure renderer object overlays are still required: their
  canonical replacements regress `SetRolloverText`, `DrawAdvObj`, or
  `DrawAdvObjShadow`, so they were restored rather than mislabeled dead.

- **2026-08-12 — `combatwindow.obj` entered with
  `TCombatWindow::Close` exact (42/42 bytes).** The alphabetical retail gap
  between `combatresultswindow` and `command`, Dreamcast compiland order, and
  the retail vtable at 0x63d528 identify the body at 0x4728d0 as slot 2.
  It deletes the polymorphic combat-control subwindow at +0x70, clears that
  member, and delegates to `heroWindow::Close`. The constructor caller in
  `combatManager::Open` independently allocates 0x8c bytes, proving the
  complete derived extent while the unconsumed fields stay opaque. The new
  unit is deliberately narrow: the other combat-window methods and local
  Dinkumware COMDAT family remain fenced until their bodies are reconstructed.
  Aggregate coverage reached 880/1314 exact across 114 units.

- **2026-08-12 — `herodefs.obj` entered with all three retail table
  loaders byte-exact (1,312 bytes total).** `InitializeHeroTraitsTable`
  parses 156 `hotraits.txt` rows and proves that the reference cell at
  0x67dce8 points to the 0x679dd0 backing table: the loader fills each
  0x5c-byte row's default-name pointer at +0x40 and the three
  low/high starting-stack pairs at +0x44..+0x58. This corrected the old
  array-base interpretation and made the 374-byte body exact.
  `InitializeHeroClassTraitsTable` proves the eighteen 0x40-byte rows at
  0x67d868, including class name, aggression, three four-byte primary-skill
  groups, 28 secondary-skill chances, and nine town-availability bytes;
  the 482-byte body is exact, and `GetNewHeroId` remained exact after its
  overlapping placeholder became `foundInTownType[alignment]`.
  `InitializeSSkillTraitsTable` proves the 28 0x10-byte rows at 0x698cf0,
  one name plus three mastery strings, and is exact at 456 bytes. Moving
  the second owned-string array to its actual declaration point reproduced
  retail's two-bit static guard order. The source-private string owner stays
  in `herodefs.h` for the cleanliness contract, while the shared skill ABI
  now lives canonically in `sskilltraits.h` rather than behind the
  adventure quick-info switch. The remaining seven link-order names in the
  cinit tail stay withdrawn as documented: they are terrain-header static
  initializers, not herodefs helpers. Aggregate coverage reached 879/1313
  exact across 113 units, 49.73% fuzzy, with all gates clean.

- **2026-08-12 — `search.obj` entered with
  `searchArray::BuildPath` reconstructed at 69.73% (642 retail bytes).**
  Retail proves the hero target dwords at unaligned +0x35/+0x39/+0x3d,
  backtracking through `pathCell::last_point`, the non-increasing adjusted
  cost guard, visited and cycle rejection, the optional cost-limited result
  insertion, and flight-plane selection from the inverse of bit 11. The
  compiled and retail bodies have the same semantic loop, packed-point
  comparisons, calls, and two returns. The remaining structural delta is
  VC6's nested Dinkumware inlining: retail has 15 branches and shares local
  range-erase helpers, while this TU population expands six copy/destroy
  loop branches (21 total). TU-wide inline-depth, `vector::clear`, merged
  cleanup labels, and split visited-state probes all regressed sharply and
  were reverted. The current source is the best natural spelling; no helper
  body or fake vector view was introduced.

- **2026-08-12 — `bitmap816.obj` entered the build and advanced to five
  exact functions.** The 33-byte deleting destructor is fixed by retail
  vtable 0x63ba14. Its slot-2 resource-size helper is exact at nine bytes
  after naming DC's `DataSize`/`ImageSize` dwords: it adds `DataSize` to the
  fixed 0x56c-byte object extent. Both palette setters are exact as well:
  the first copies the complete 0x200-byte `TPalette16` payload and the
  second delegates to the exact `TPalette24::operator=`. Finally,
  `Bitmap816::ResetPalette` is exact (64/64 bytes), retiring its false raw
  24-bit-palette view. Retail proves a complete embedded `TPalette16`
  resource at +0x34 (whose 256-word color payload begins at +0x50) and a
  complete embedded 0x31c-byte `TPalette24` resource at +0x250:
  ResetPalette passes the latter to the `TPalette16` conversion constructor,
  copies the temporary's 0x200-byte color payload into the former, and
  destroys the temporary. The destructor independently calls the +0x250 and
  +0x34 palette destructors and raises the natural `delete[] map` body to
  99.97%; its only residual is VC6's internal EH-state ordinal (compiled 2,
  retail 1), while calls, branches, offsets, and unwind transitions agree.
  The virtual size helper adds the resulting 0x56c fixed object extent to
  the owned bitmap bytes. The
  canonical palette-painter overload now accepts `TPalette24*`; raw
  `paletteHiColor*` consumers remain a distinct proven overload. After the
  later neighboring admissions, aggregate coverage reached 901/1340 exact,
  50.19% fuzzy and 12.78% executable matched, with all gates clean.

- **2026-08-12 — the remaining dead fake-view surface was removed.** The
  resource-display retail six-argument `border` constructor and
  eleven-argument `textWidget` constructor are ordinary overloads, the three
  text pointers and player selector are canonical declarations, and both
  consuming TUs compile without a private header personality. The proven event
  record pointer-vector at game +0x4e7ac is likewise the canonical tail member.
  The puzzle
  window header no longer hides its complete admitted classes behind a
  TU-only switch; game.obj uses a narrow declaration for the sole puzzle
  function it calls. The puzzle TU still needs the genuinely required
  `NewfullMap` object-pool projection, but its dedicated puzzle-only branch
  was later removed in favor of the canonical map-object projection. A probe
  making `type_artifact`'s setup-only default
  constructor global likewise breaks the aggregate initialization used by
  hero.obj, so the game setup personality remains necessary. The other
  remaining projections retain their previously measured retail-score
  regressions. The provisional `PuzzleWindowMissingWidget` alias was also
  deleted once its body address and error path proved it was simply the
  canonical `MemError`. A full 114-unit VC6 build stayed at 880/1314 exact and
  49.73% fuzzy, with ratchet, VA, single-view, and cleanliness gates clean.

- **2026-08-12 — `bitmap24.obj` entered the build with four exact functions
  and one measured constructor residual.** The retail constructors prove a `resource`-derived
  0x30-byte object with `DataSize` at +0x1c, dimensions at +0x24/+0x28 and
  the owned pixel pointer at +0x2c. Vtable 0x63b9f4 slot 2 returns
  `DataSize + 0x30`, exactly matching the resource-size virtual already
  established for `Bitmap16Bit` and `Bitmap816`. The 55-byte draw wrapper is
  exact with the natural forwarding call through the destination bitmap's
  map, width, height and pitch fields. The 34-byte destructor is
  the natural conditional `delete[] data` followed by the base destructor;
  its frameless retail form independently proves the same TU-visible nothrow
  deallocator contract used by bitmap16.obj. The canonical generated scalar
  deleting wrapper is exact as well. The 192-byte path constructor zeroes the
  complete tail through member initializers, concatenates `path + name` in a
  260-byte buffer, and calls the PCX importer; all instructions agree and its
  99.94% residual is only stripped-target callee/EH relocation naming. The
  170-byte data constructor is 92.70%
  with exact allocation/copy semantics and CFG; its residual is field/vptr
  store scheduling before the allocation. Direct parameter reuse, stored
  `DataSize` reloads, member-based image-size calculation, an explicit
  image-size local, and three assignment orders bounded the current maximum.
  The importer, inner blitter and HSV body stay fenced. After mainmenu's
  parallel admission, aggregate coverage stands at 887/1325 exact across 116
  units and 49.60% fuzzy, with all gates clean.

- **2026-08-12 — `mainmenu.obj` entered the build with its modal and
  destructor trio exact; its constructor now has exact retail flow.**
  `TMainMenu::DoModal` is the 44-byte twin of the
  other front-end dialogs: start `"MainMenu"` music with `(0, 1)`, then pass
  `this`, the address-taken `MainMenuHandler`, and fade flag zero to
  `heroWindowManager::DoDialog`. The destructor restores vtable 0x63ff50,
  clears the source-private active-menu pointer at 0x699660 before walking
  and deleting the inherited widget vector, then destroys `heroWindow`; the
  natural loop becomes all 117 retail bytes once that observable global
  clear is kept in retail order. Its generated scalar deleting wrapper is
  exact as well. The 901-byte constructor reconstructs the 0x54-byte class,
  five packed button rectangles and all five button recipes, registration
  loop, network-host button suppression, and time snapshot. Its 35-block CFG
  is exact and it scores 97.01%; the sole structural residual is the same
  VC6 `vector<widget*>::reserve` nested-inline choice already bounded in the
  game-type window. `CDPlay::IsHost` is called through its CodeView-proven
  slot 36 (+0x90), without an ad-hoc object view. `MainMenuHandler` is now
  reconstructed end to end: its one-shot 5 MiB free-space warning, localized
  missing-CD notice, right-click help dispatch, quit confirmation, hover
  highlighting, dirty-video redraw, and ten-second multiplayer timeout all
  agree with the retail branch stream (37 branches and three returns). It
  scores 89.65%; the remaining structural residual is confined to VC6 giving
  the two mutually exclusive temporary strings separate stack slots instead
  of retail's reused slot. The called 66-byte
  `get_available_disk_space` wrapper is byte-exact. The five-row
  `VideomodeChoice` surface remains retail-dropped. Aggregate coverage is now
  888/1326 exact across 116 units, 49.98% fuzzy, and 12.67% executable
  matched, with all gates clean.

- **2026-08-12 — `campaignbrief.obj` entered the build from its 399-byte
  destructor anchor.** Retail proves a 0x68-byte `TCampaignBrief`: after the
  0x4c-byte `heroWindow` base come the z buffer, saved music volume, a
  Dinkumware vector at +0x54, and the owning campaign-header pointer at
  +0x64. The vector's reverse cleanup advances by 0x4d4 and destroys a
  `NewSMapHeader` at element +0, fixing the element layout without a private
  view. The reconstructed destructor restores the saved `game` through its
  register-passed assignment operator, restores ambient music, disposes the
  saved game and campaign header, frees the z buffer, virtually deletes every
  inherited widget, then lets VC6 emit the scenario-vector and base cleanup.
  It has the exact 399-byte extent and scores 95.76%. The naturally generated
  `NewSMapHeader` destructor scores 91.51%; this TU requires `/MT`, independently
  visible in the retail `std::_Lockit` pair around the shared map sentinel
  that `/ML` omits. Its remaining differences are instruction scheduling and
  EH-state numbering, not missing behavior. Aggregate coverage is 50.04%
  fuzzy before the generated scalar deleting wrapper was admitted exact.
  Aggregate coverage is now 889/1329 exact, 50.05% fuzzy, and 12.71%
  executable matched; all gates remain clean.

- **2026-08-12 — `customcampaign.obj` entered the build with both located
  scoring functions exact.** The retail loops prove that `SCampaign` owns a
  Dinkumware vector at +0x5c whose 0x14-byte entries carry an active byte,
  elapsed days at +4, and score at +8. `get_total_time` sums the days of
  active entries. `get_score` ignores inactive and negative-score entries,
  computes the rounded average of the rest, and multiplies it by five. Using
  the original repeated `mapScores[i]` expressions reproduces VC6's separate
  element-address and active-byte loads exactly; a reference-local probe was
  three bytes shorter and was rejected. The 128- and 87-byte retail rows are
  exact. Aggregate coverage is now 891/1331 exact across 118 units, 50.07%
  fuzzy, and 12.72% executable matched, with all gates clean.

- **2026-08-12 — `spellbookwindow.obj` entered the build with four exact
  functions.** Retail's vtable at 0x641dcc independently fixes the generated
  deleting destructor, `Open`, and `Close` at 0x0059c8c0/0x0059c970/
  0x0059c990; constructor order and the shared active-window store fix the
  117-byte destructor between them. Its natural widget-vector teardown is
  exact. The class is canonical rather than an overlay: DC gives every
  member name and the 0x58-byte base layout, while retail proves its 0x60-byte
  `CAdvPopup`, twelve-entry (not DC's six-entry) `SpellMap`, widget range
  pointers at +0xac/+0xb0/+0xb4, five widget pointers through +0xc8, and a
  total size of 0xcc. `Open`'s `heroWindow::Open(...) ? 3 : 0` and `Close`'s
  direct base delegation reproduce all 27 and 16 bytes. The synchronized
  build/delink/build cycle raised aggregate coverage to 897/1336 exact and
  50.18% fuzzy, with all ratchet, VA, single-view, and cleanliness gates
  clean.

- **2026-08-12 — `hillfortwindow.obj` entered the build with three exact
  functions.** Retail's nine-slot vtable at 0x63eb68 and direct
  `heroWindow` constructor/destructor calls prove the canonical base without
  inventing a derived-tail view. The vtable fixes the 33-byte deleting
  wrapper; the 117-byte destructor clears the constructor-proven active
  window at 0x699194, deletes the canonical widget vector, and falls through
  to the base destructor exactly. `DoModal` is exact at 31 bytes: it calls
  the independently located `Recalculate(0)` and passes the handler address
  proven by its 0x4e8850 immediate to `heroWindowManager::DoDialog`. Six
  located but unreconstructed neighbors remain non-claiming location notes,
  so they do not create artificial 0% denominator rows. The synchronized
  cycle reached 904/1343 exact, 50.21% fuzzy, and 12.79% executable matched,
  with every gate clean.

- **2026-08-12 — `overview.obj` entered the build with
  `game::SetupNewOverviewType` exact (826/826 bytes).** Dreamcast supplies
  the method identity, locals, and original overview-global names; retail
  independently fixes the two player-count fields, slider protocol, two
  widget messages, three title-widget cells, mode-dependent title counts,
  and both six-element geometry tables. The reconstruction uses the
  canonical `playerData`, `slider`, `message`, `textWidget`, and
  `heroWindow` interfaces—no TU-local views. HoMM2 lineage supplied the
  source vocabulary, while every changed HoMM3 behavior was selected from
  the retail body. Moving the unsigned-short geometry declarations back to
  their retail lifetime point and preserving the original message-field
  assignment order closed all 29 CFG blocks exactly. Aggregate coverage is
  now 892/1332 exact across 119 units, 50.15% fuzzy, and 12.76% executable
  matched, with all gates clean.

- **2026-08-12 — `hero::SetSS` closed from 3.8% to exact (63/63
  bytes).** Its natural source was already correct, but VC6 over-inlined the
  exact `GiveSS` body where all four retail in-TU callers use an out-of-line
  call. A call-site-only `inline_depth(0)` pin reproduces that retail
  decision while leaving `GiveSS`, its other callers, and unrelated TU
  inlining unchanged. Aggregate coverage is now 893/1332 exact, 50.16%
  fuzzy, and 12.76% executable matched; all gates remain clean.

- **2026-08-12 — `victorylossconditions.obj` entered the build with
  `VictoryConditionStruct::IsTownCaptureTarget` exact (52/52 bytes).** Retail
  proves condition type 6, signed town ids, and the three Complete-era dword
  coordinates at +0x18/+0x1c/+0x20; Dreamcast independently supplies the
  class, method, parameter, and `bool` return identity. The exact source
  rejects non-town-capture conditions, calls `game::GetTownId`, and uses an
  explicit false/true return pair so VC6 emits the retail `sete al` directly.
  The other sixteen located rows remain fenced. Aggregate coverage reached
  875/1307 exact across 110 units, 49.55% fuzzy, with all gates clean.

- **2026-08-12 — twelve obsolete fake-view switches and one dead declaration
  switch were removed.** The cache ABI, artifact-owned tables, puzzle and
  quick-creature text fields, hero disguise/class-name fields, hire helpers,
  boat/landing helpers, and cell-adjustment methods now use their canonical
  declarations without TU-only preprocessor aliases; the never-consumed
  adventure-options switch and the now-empty combat-manager declaration
  switch were deleted outright. A full 109-unit VC6 build retained all
  874/1290 exact rows, 50.18% fuzzy coverage, and every ratchet, VA,
  single-view, and cleanliness gate. The remaining explicit compatibility
  layouts are not dead: probes replacing the town-name overlay and the
  creature-bank vector overlay with canonical `std::string`/`std::vector`
  access regressed `SetRolloverText` (89.71% to 89.50%) and
  `get_creature_bank_help_text` (87.28% to 87.13%) and were reverted; the two
  adventure object-renderer overlays and Seer Hut/Quest Guard vector access
  retain the previously documented VC6 regressions. The contemporary claim
  that `TSplitSliderView` was a genuine class is superseded by the 2026-08-28
  Dreamcast/retail proof: the source and retail call canonical `slider` and no
  Dreamcast class record exists. The later herodefs proof also made the
  custom-name fields and methods plus the 144-bit combination-artifact
  component set canonical everywhere;
  all 37 dependent units and the full ratchet stayed neutral.

- **2026-08-12 — `creature_bank.obj` entered the build with
  `type_creature_bank_traits::type_creature_bank_traits` exact (94/94
  bytes).** The constructor initializes the 16-byte Dinkumware name string
  and then four level records, calling `armyGroup::armyGroup` at +0x10 with
  a 0x60 stride. Those retail bytes replace the old anonymous 0x180 tail
  with four canonical `type_creature_bank_level` records and close the
  already observed 0x190 trait stride. The remaining Dreamcast roster is
  fenced because its command-to-creaturetype retail bracket is not uniquely
  attributable. Aggregate coverage reached 874/1290 exact across 109 units,
  50.18% fuzzy, and 12.44% executable matched with every gate clean.

- **2026-08-12 — the retail `CNetMsg` initialization order made
  `game::ClaimGenerator` and `game::CreateBoat` exact.** The shared inline
  constructor assigns the message subtype before the -1 sentinel, then the
  size and zero fields; the previous member-initializer spelling forced a
  different VC6 store schedule. Reconstructing the constructor body in that
  retail-proven order raised ClaimGenerator from 99.9706% and CreateBoat
  from 99.9726% to 100%, while also raising the large `game::Load` body from
  50.3078% to 50.4577%. A declaration-order probe for ClaimGarrison regressed
  and was reverted. Aggregate coverage reached 873/1289 exact across 108
  units, 50.17% fuzzy, with every gate clean.

- **2026-08-12 — the false `NewfullMap::Close` view at 0x4fd460 was
  removed.** Retail takes a destructor-flags argument, tests the array and
  delete bits, reads the cookie at `[this-4]`, invokes
  `NewmapCell::~NewmapCell` through the vector destructor iterator, and
  conditionally frees the allocation. It is a compiler-generated
  `NewmapCell` deleting destructor, not Dreamcast's no-argument
  `NewfullMap::Close`; the latter is inlined into retail's NewfullMap
  destructor and Init. Demoting the false claim removed one artificial 0%
  row without claiming bytes. Coverage is 871/1289 exact across 108 units,
  50.17% fuzzy, with every gate clean.

- **2026-08-12 — `NewfullMap::saveTreasureData` is exact (77/77
  bytes), and the opaque `TreasureData` view was retired.** Dreamcast names
  the message, custom-guardians flag, and guardian army; retail fixes them at
  +0x00, +0x10, and +0x14 and closes the independently proven 0x4c record
  stride. The body serializes the message, writes the flag through the
  virtual file interface with an unsigned short-write check, reloads that
  flag after the potentially aliasing call, and conditionally saves the
  army. `BlackBoxData` now retains its DC-proven `TreasureData` base rather
  than duplicating the base as anonymous payload. Aggregate coverage reached
  871/1290 exact across 108 units, 50.16% fuzzy, and 12.43% executable
  matched with every gate clean.

- **2026-08-12 — `searchArray::PushCombatPoint` was restored from its
  fenced carcass and raised from 0% to 75.7383%.** The retail 860-byte body
  proves the 187-hex bounds, 500-entry queue cap, visited-cell cost
  rejection, descending-cost binary search, partial `pathCell`
  initialization, vector insertion, and final grid-cell copy. Dreamcast
  CodeView independently supplies the five parameter roles and local
  identities. The remaining delta is source-shape/register allocation in
  VC6's large inlined `std::vector<pathCell>::insert`; the reconstructed
  control flow and all semantic operations are present. Aggregate coverage
  remains 870/1290 exact across 108 units, while fuzzy coverage rose from
  50.01% to 50.14% and executable matched rose from 12.40% to 12.43%, with
  every gate clean.

- **2026-08-12 — two non-emitting generated-destructor claims were
  removed from the comparison universe.** The old 0x4077b0 claim assigned
  a fourth virtual slot to `advManager`, contradicting the admitted
  three-slot retail table; that address actually begins the following
  `CAdvMgrNetMsgHandler` table. The correctly located
  `THeroScreenWindow` deleting destructor at 0x4e1520 was also demoted to
  evidence-only until its real destructor and canonical base layout are
  admitted, because an isolated `VA_COMPGEN` annotation cannot emit code.
  This removes two artificial 0% rows without claiming any retail bytes;
  coverage is 870/1290 exact across 108 units, with all gates clean.

- **2026-08-12 — `singleselectionpopups.obj` entered the build with
  `CSpriteWidget::Main` exact (16/16 bytes).** The retail constructor at
  0x5754f0 installs vtable 0x641a00; slot 2 uniquely locates this body at
  0x575a10, where it directly forwards its message pointer to
  `widget::Main`. The same constructor's 0x38-byte allocation and stores at
  +0x30/+0x34 establish the canonical sprite-widget tail without a local
  view type. The other 37 Dreamcast-derived stubs remain fenced. Aggregate
  coverage reached 870/1292 exact across 108 VC6 units, 50.01% fuzzy, and
  12.40% executable matched with every gate clean.

- **2026-08-12 — `event_record.obj` entered the build with
  `game::replay_available` exact (65/65 bytes).** Retail proves the
  Dinkumware pointer vector at game +0x4e7ac (first/last at +0x4e7b0 and
  +0x4e7b4), the signed player byte at record +4, and the comparison against
  `gNetLocalGamePos`. The function returns true on the first queued record
  belonging to another player and false for an empty or entirely local
  queue. Its remaining 212-function generated carcass is fenced, while the
  two old unreconstructed visibility VAs were demoted to evidence-only
  locations. Aggregate coverage reached 869/1291 exact across 107 VC6 units,
  50.00% fuzzy, and 12.40% executable matched with every gate clean.

- **2026-08-12 — `NewfullMap::loadObject` is exact (164/164 bytes).**
  The function reads the object's x, y, and z bytes individually, followed
  by its two-byte object-type index, and returns -1 immediately whenever
  virtual `TAbstractFile::Read` supplies fewer bytes than requested. Retail
  proves the four read widths, field offsets, short-read polarity, and local
  reuse; Dreamcast supplies the member identity and parameter roles. The
  old opaque `void*` carcass signature was promoted to the canonical stream
  interface, and the source-owned VA migrated cleanly to its resulting
  decorated identity. Aggregate coverage reached 868/1290 exact across 106
  VC6 units, 50.00% fuzzy, and 12.39% executable matched with every gate
  clean.

- **2026-08-12 — `cursor.obj` entered the build with
  `advManager::TurnTo` exact (26/26 bytes).** Repeated direct-call edges
  from the admitted adventure-manager bodies locate the retail cursor run:
  `StopCursor` at 0x47f7d0, the drawing and movement functions in roster
  order through `ValidMove`, and `SendMapChange` at 0x482390. The smallest
  member in that run clears `cursorTurning` at +0x204 and stores the new
  `cursorDirection` at +0x1f4. Dreamcast supplies those consecutive member
  names at offsets twelve bytes later; retail's already-proven cursor-array
  extent and the two stores establish the retail shift. The remaining
  28-function carcass stays fenced. Aggregate coverage reached 867/1290
  exact across 106 VC6 units, 49.96% fuzzy, and 12.39% executable matched
  with every gate clean.

- **2026-08-12 — `GetMonsterCost` is exact (49/49 bytes).** The original
  source shape is the direct seven-element indexed copy from the creature
  record's cost row. VC6 strength-reduces the repeated
  `29 * monId + 8 + resource` subscript into retail's two-LEA multiply,
  keeps that product in ECX, and lowers the indexed loop to paired pointer
  increments with a seven-item countdown. The previously hand-lowered
  pointer loop was behaviorally identical but selected EAX for the product
  and plateaued at 86%. Aggregate coverage reached 866/1289 exact across
  105 VC6 units, 49.96% fuzzy, and 12.38% executable matched with every
  gate clean.

- **2026-08-12 — `army::get_fire_shield_strength` is exact (37/37
  bytes).** Retail returns the stack's `float` at +0x4a0 while the
  independently established round counter at +0x20c is non-zero. With no
  active spell, only creature type 0x35 (the already-proven Efreet Sultan
  domain value) receives the shared 0.2f innate multiplier; every other
  stack receives the image-wide zero-float constant. Both anonymous pool
  entries are source-owned through `DATA_COMPGEN`, and the canonical army
  layout now exposes the previously padded strength field. Aggregate
  coverage reached 865/1289 exact across 105 VC6 units, 49.96% fuzzy, and
  12.38% executable matched with every gate clean.

- **2026-08-12 — `bottomviewsubwindow.obj` entered the build with the
  `type_bottom_view_window` destructor pair exact.** Vtable 0x63bb04
  proves the scalar deleting destructor at 0x450d20 (33/33 bytes), which
  calls the 120-byte destructor at 0x450d50. The latter walks the inherited
  widget vector, removes each live widget from the parent, deletes it
  virtually, and then invokes the exact `TSubWindow` destructor. The empty
  `animate` slot is the image-wide one-byte COMDAT at 0x5bc690 and is not
  double-claimed. The 2,260-byte constructor remains fenced. Aggregate
  coverage reached 864/1288 exact across 105 VC6 units, 49.96% fuzzy, and
  12.38% executable matched with every gate clean.

- **2026-08-12 — `TAdventureMapWindow::animate_bottom_view` is exact
  (36/36 bytes).** The body calls the bottom view's virtual `animate`
  slot when running in the foreground, or in the background only when
  the gate byte is set. Dreamcast names that member
  `animate_in_background` at +0x64; the independently established
  eight-byte retail base shift puts it at the exact +0x6c byte read by
  retail. This grows the existing canonical adventure-window layout rather
  than adding a view. Aggregate coverage reached 862/1286 exact, 49.94%
  fuzzy, and 12.37% executable matched with every gate clean.

- **2026-08-12 — `kb.obj` entered the build with `GetMapExtra`
  (37/37 bytes) and `GetMapExtraPtr` (36/36 bytes) exact.** Both bodies
  linearize coordinates as `((z * gMapHeight + y) * gMapWidth + x)` into
  the 16-bit map-extra buffer whose retail pointer cell is at 0x6989f8;
  one loads the value and the other returns its address. The 121-function
  carcass is fenced and twelve stale claims were demoted, leaving only the
  two reconstructed accessors. Aggregate coverage reached 861/1285 exact
  across 104 VC6 units, 49.94% fuzzy, and 12.37% executable matched with
  every gate clean.

- **2026-08-12 — `events.obj` entered the build with both 35-byte pool
  accessors exact.** `get_treasure_data` extracts a 12-bit index from
  `NewmapCell::extraInfo`, reads the vector's first pointer at
  `NewfullMap+0x34`, and indexes 0x4c-byte records. `get_black_box`
  extracts a 10-bit index, reads the first pointer at +0x54, and indexes
  0xe4-byte records. The record payloads remain opaque; only the extents
  proved by retail address arithmetic were admitted. The 275-function
  carcass is fenced and 24 stale claims were demoted, leaving only these
  two real claims. Aggregate coverage reached 859/1283 exact across 103
  VC6 units, 49.93% fuzzy, and 12.37% executable matched with every gate
  clean.

- **2026-08-12 — `army::CancelSpellType` is exact (59/59 bytes).** The
  independently anchored retail body switches on the cancellation moment:
  the after-attack arm cancels spell 59, while the after-damage arm cancels
  62, 70, and 74 in that order. A named move/attack/damage domain replaces
  the raw switch labels; its zero move value is also corroborated by the
  exact `FlyTo` consumer, and the mature HoMM2 sibling supplies the same
  semantic spellings. Aggregate coverage reached 857/1281 exact, 49.92%
  fuzzy, and 12.37% executable matched with every gate clean.

- **2026-08-12 — `cspriteframe.obj` entered the build with all three
  vtable functions exact.** Retail's two importing constructors prove the
  `resource` base followed by `DataSize` at +0x1c through `map` at +0x44,
  and the vtable/destructor prove that the retail object ends there at
  0x48 bytes (unlike Dreamcast's DirectDraw-tailed form). The scalar
  deleting destructor is 33/33 bytes, the destructor is 34/34, and the
  resource-size slot is 7/7. The unreconstructed 34-function carcass is
  fenced and its two old claims were demoted. Aggregate coverage reached
  856/1280 exact across 102 VC6 units, 49.92% fuzzy, and 12.36%
  executable matched with every gate clean.

- **2026-08-12 — `palette.obj` advanced with `TPalette24`'s scalar
  deleting destructor (33/33 bytes) and assignment operator (33/33 bytes)
  exact.** The assignment leaves the resource base intact and copies only
  the independently proven 0x300-byte RGB payload with VC6's `rep movsd`.
  The label generator now recognizes only MSVC `??4` assignment operators,
  allowing the source-owned claim to adopt the compiled decorated name;
  other special operators remain deliberately unjoined, and positive and
  negative controls cover that boundary. Aggregate coverage reached
  853/1277 exact, 49.91% fuzzy, and 12.36% executable matched with every
  gate clean.

- **2026-08-12 — `philai.obj` entered the build with
  `hero::ValueOfSpell` exact (86/86 bytes).** Retail's callers and body use
  a thiscall-shaped ABI (hero in ECX, spell on the stack), while Dreamcast
  exposes the same source logic as a file-local two-argument helper. The
  body rejects spells above Wisdom+2, spells already in the hero's book,
  and heroes without the spellbook artifact before forwarding to the AI
  spell evaluator. A compound success condition reproduces retail's shared
  zero epilogue. The remaining 134-function carcass is fenced and nine
  stale `VA` annotations were demoted. Aggregate coverage reached 851/1275
  exact across 101 VC6 units, 49.90% fuzzy, and 12.36% executable matched
  with every gate clean.

- **2026-08-12 — `adventuremapwindow.obj` entered the build with
  `ClearBottomView` (31/31 bytes) and `UpdateResourceDisplay` (30/30
  bytes) exact.** Retail proves the owned bottom-view pointer at +0x98
  through its virtual deletion and clear, and the resource-display pointer
  at +0x5c through its two-argument update call. Both fields extend the
  existing canonical `TAdventureMapWindow` model; no duplicate view was
  introduced. The remaining 226-function carcass is fenced and 23 stale
  `VA` annotations were demoted to non-claims. Aggregate coverage reached
  850/1274 exact across 100 VC6 units, 49.89% fuzzy, and 12.35% executable
  matched with every gate clean.

- **2026-08-12 — `army.obj` entered the build with `army::move_to`
  exact (20/20 bytes).** The old link-order join mislabeled retail
  0x445d10 as `choose_wall_target`; Dreamcast's decorated public symbol,
  its call edge to `simple_move`, and retail's two-argument forwarding
  thunk instead prove `move_to`. The unreconstructed 182-function carcass
  is now fenced, and 71 stale `VA` annotations were demoted to non-claims
  rather than exposing fake owners. Aggregate coverage reached 848/1272
  exact across 99 VC6 units, 49.89% fuzzy, and 12.35% executable matched
  with every gate clean.

- **2026-08-12 — `TPalette24::TPalette24()` is exact (22/22 bytes).**
  Retail passes a null name and zero resource type to the independently
  exact `resource` constructor, then installs vtable 0x640374. The natural
  derived initializer reproduces that complete sequence with no guessed
  state. Aggregate coverage reached 847/1271 exact, 49.89% fuzzy, and
  12.35% executable matched with every gate clean.

- **2026-08-12 — `palette.obj` advanced with `TPalette24`'s destructor
  (11/11 bytes) and resource-size slot (6/6 bytes) exact.** Retail
  constructors copy exactly 0x300 RGB bytes at +0x1c, while vtable 0x640374
  identifies the empty derived destructor and a slot returning the resulting
  0x31c-byte extent. Those independent facts admit the real resource-derived
  class layout and correct the generated inventory's mistaken `TPalette16`
  slot label. Aggregate coverage reached 846/1270 exact, 49.88% fuzzy, and
  12.35% executable matched with every gate clean.

- **2026-08-12 — `bitmap16.obj` advanced with the `Bitmap16Bit`
  resource-size vtable slot exact (7/7 bytes).** Retail slot 2 loads
  `DataSize` from the proven +0x1c field and adds the class's independently
  proven 0x38-byte extent. Expressing that contract directly reproduces the
  complete body and strengthens the shared resource-size interpretation.
  Aggregate coverage reached 844/1268 exact, 49.88% fuzzy, and 12.35%
  executable matched with every gate clean.

- **2026-08-12 — `csprite.obj` advanced with the `CSprite` resource-size
  vtable slot exact (6/6 bytes).** The generated name inventory misassigned
  retail 0x47bd50 to a `CSpriteFrame` destructor, but the address is slot 2
  of the independently proven `CSprite` vtable and its entire body returns
  the literal 0x38-byte class extent. The corrected shared resource-size
  contract therefore identifies and reproduces it without trusting the
  contradicted external name. Aggregate coverage reached 843/1267 exact,
  49.88% fuzzy, and 12.34% executable matched with every gate clean.

- **2026-08-12 — `palette.obj` advanced with the `TPalette16` resource-size
  vtable slot exact (6/6 bytes).** Retail's 0x522b40 row is not the
  Dreamcast `AdjustValue` body assigned there by the old bracket join: it is
  slot 2 of vtable 0x640368 and returns the class's proven 0x21c-byte extent.
  The shared resource slot is now correctly typed as a const unsigned-size
  query across the modeled derived classes; all 44 affected VC6 units kept
  their prior maxima. Aggregate coverage reached 842/1266 exact, 49.88%
  fuzzy, and 12.34% executable matched with every gate clean.

- **2026-08-12 — `palette.obj` entered the build with
  `TPalette16::~TPalette16` exact (11/11 bytes).** This was the smallest
  retail-tied body not already present in the comparison graph. The empty
  derived destructor naturally restores the `TPalette16` vtable and
  tail-jumps to `resource::~resource`, exactly matching retail. Fifteen older
  annotations on unreconstructed palette bodies were withdrawn during
  admission. Aggregate coverage reached 841/1265 exact, 49.88% fuzzy, and
  12.34% executable matched with every gate clean.

- **2026-08-12 — `wingraph.obj` advanced with `InitGraphics` exact
  (5/5 bytes).** WinMain's post-CreateWindow callee is a pure tail wrapper
  into the adjacent DirectDraw-creating body at 0x6014f0. Retail emits no
  argument setup, proving the zero-argument x86 variant even though the
  Dreamcast port's same-named initializer accepts mode and reinit arguments.
  The natural C++ call compiles to retail's five-byte tail jump. Aggregate
  coverage reached 840/1264 exact, 49.88% fuzzy, and 12.34% executable
  matched with every gate clean.

- **2026-08-12 — `text.obj` entered the build with
  `InitializeGeneralText` exact (25/25 bytes).** The stale first-pass join
  classified the Dreamcast text roster as retail-dropped, but retail's
  0x5b90f0 body directly references `genrltxt.txt`, calls the independently
  mapped `ResourceManager::GetText`, stores the result at 0x6a5d5c, and
  returns its non-null status. That literal, call edge, and source order
  prove the identity without relying on the contradicted IDA address label.
  Aggregate coverage reached 839/1263 exact, 49.88% fuzzy, and 12.34%
  executable matched with every gate clean.

- **2026-08-12 — `csprite.obj` entered the build with `CSprite::GetPalette`
  exact (14/14 bytes) and `CSprite::ColorCycle` exact (27/27 bytes).** The
  retail accessor loads the owned `TPalette16` at +0x20 and returns its
  raw 16-bit palette at +0x1c, while the successor delegates its three
  arguments to `TPalette16::Cycle`. The palette resource now exposes its
  same-storage raw-record member through a union, preserving its proven
  layout without a cast or an access-only view. Four older annotations on
  unreconstructed sprite bodies were withdrawn during admission. Aggregate
  coverage reached 838/1262 exact, 49.88% fuzzy, and 12.34% executable
  matched with every gate clean.

- **2026-08-12 — `wingraph.obj` entered the build with
  `GetDesktopWidth` and `GetDesktopHeight` exact (6/6 bytes each).** The two
  source-order functions are the direct retail loads from the desktop-width
  and desktop-height globals at 0x68c874 and 0x68c878. Four older annotations
  on unreconstructed graphics stubs were withdrawn during admission, leaving
  the live unit limited to retail-proven bodies. Aggregate coverage reached
  836/1260 exact, 49.87% fuzzy, and 12.34% executable matched with every gate
  clean.

- **2026-08-12 — `dimensiondoorwindow.obj` entered the build with
  `TSkuttleBoatWindow::ExitDialog` exact (44/44 bytes).** Retail's caller
  allocates the 0x64-byte window and its constructor accesses the sole
  derived `RolloverWidget` at +0x60, agreeing with the Dreamcast field
  roster after accounting for retail's wider `CAdvPopup`. The handler
  rewrites the incoming event as widget code 10, clears the window
  manager's dialog result, and forwards dispatch. Aggregate coverage
  reached 834/1258 exact, 49.87% fuzzy, and 12.34% executable matched with
  every gate clean.

- **2026-08-12 — `drawing.obj` entered the build with
  `combatManager::ResetLimitCreature` exact (99/99 bytes).** The body clears
  the forty per-stack effect bytes, the paired hero/flag latches, and three
  archer latches, then restores the manager's 16-byte drawing extent from
  the retail aggregate at 0x6aace8. The effect tail begins beyond the
  currently materialized `combatManager` prefix; its five proven offsets
  are named directly instead of introducing an access-only class. An
  attempted shared-header tail expansion changed VC6's type population and
  regressed the previously exact `initialize_game_data`; it was rejected,
  and the established class contract plus its 100% match were restored.
  Twelve older annotations on unreconstructed drawing stubs were withdrawn
  during admission. Aggregate coverage reached 833/1257 exact, 49.87%
  fuzzy, and 12.34% executable matched with every gate clean.

- **2026-08-12 — `towngatewindow.obj` entered the build with the
  `TTownGateWindow` destructor pair exact (33/33 generated wrapper and
  143/143 body).** The Dreamcast field roster starts the derived tail at its
  0x58-byte `CAdvPopup`; retail's eight-byte-wider base and four-byte-wider
  VC6 vector place `Towns`, `topTown`, `selectedTown`, and
  `adventure_spell` at +0x60/+0x70/+0x74/+0x78. `advManager::TownGate`'s
  stack object independently proves the resulting 0x7c total. The destructor
  deletes each owned widget, releases the town-index vector, and delegates to
  the popup base exactly as the natural C++ ownership loop specifies.
  Aggregate coverage reached 832/1256 exact, 49.86% fuzzy, and 12.33%
  executable matched with every gate clean.

- **2026-08-12 — `lodfile.obj` entered the build with
  `LODFile::getItemIndex` exact (69/69 bytes).** Retail constructor, open,
  clear, and read accesses account for the canonical 0x18c-byte object,
  including the 0x20-byte `LODEntry` rows and the VC6 vector at +0x17c;
  seven older annotations on unreconstructed stubs were withdrawn during
  admission. The lookup rejects a closed archive, binary-searches the live
  entry count through `Find`, and returns the indexed row only on a
  nonnegative match. A nested success return was needed to reproduce
  retail's single shared null-return epilogue. Aggregate coverage reached
  830/1254 exact, 49.84% fuzzy, and 12.32% executable matched with every
  gate clean.

- **2026-08-12 — `questlogwindow.obj` entered the build with
  `TQuestLogWindow::WindowHandler` exact (29/29 bytes).** Source-order
  mapping and the two retail call edges identify the handler: it first
  delegates to `CAdvPopup::WindowHandler`, then invokes the public
  `TrueFalseDialogHandler` thunk only when the base leaves the message
  unhandled. `DoQuestLog` independently proves the canonical class size by
  allocating 0x74 bytes, while the constructor initializes the byte at
  +0x60 and the VC6 `vector<int>` at +0x64..+0x73; the header therefore
  carries the real layout rather than an access-only view. Aggregate
  coverage reached 829/1253 exact, 49.83% fuzzy, and 12.32% executable
  matched with every gate clean.

- **2026-08-12 — `bitmap16.obj` entered the build with the
  `Bitmap16Bit` destructor pair exact (33/33 generated wrapper and 41/41
  body).** Retail's 0x38-byte allocation/layout and the destructor body agree:
  `map` is +0x30 and is freed only when non-null and not `referenced` at
  +0x34, followed by the `resource` base destructor. The initial natural body
  gained an /GX unwind frame because VC6's default deallocator declaration
  may throw; the same retail-proven nothrow redeclaration established by
  `sample.obj` removes it and reproduces the frameless retail body exactly.
  Four older `VA` annotations on still-stubbed drawing methods were withdrawn
  and retained only as documented retail locations. Aggregate coverage
  reached 828/1252 exact, 49.83% fuzzy, and 12.32% executable matched with
  every gate clean.

- **2026-08-12 — `townmgr.obj` entered the build with `DoMapTavern`
  exact (48/48 bytes).** The event-dispatch caller, the direct `DoTavern`
  edge, and the paired town-manager path settle both identity and role: map
  taverns set the shared mode flag, run the common chooser, then ask its
  selected hero to hire for `gNetLocalGamePos` at the event point. The two
  source-private globals at 0x6aaa48/0x6aa628 are now claimed with
  role-derived provisional names. Admitting this TU also exposed an old
  `VA` annotation on the still-stubbed 821-byte `GetBuildingInfo`; that
  unearned claim was withdrawn and retained only as a documented retail
  location. The clean post-delink result is 826/1250 exact, 49.82% fuzzy,
  and 12.31% executable matched with all gates clean.

- **2026-08-12 — `soundManager::Close` closed exact at 0x00599a90
  (241/241 bytes).** The slot-1 virtual shutdown path disables future sound,
  closes video, gives asynchronous sample waiters up to twenty 50-ms drain
  intervals, serializes Miles shutdown under the two sound sections, closes
  the MP3 stream, ends every sample handle, and clears the manager status.
  Retail xrefs to 0x6a3254 prove it as the live waiter counter: the wait
  thread increments/decrements it and `Close` polls it. The first natural
  compound `while` reconstruction was semantically complete but scheduled
  the two tests differently; the source-shaped bounded `for` with an early
  worker-count break reproduces retail's loop and made the function exact.
  Aggregate coverage reached 825/1249 exact, 49.82% fuzzy, and 12.31%
  executable matched with all gates clean.

- **2026-08-12 — `binkmanager.obj` entered the build with
  `BinkManager::RestartBink` exact (77/77 bytes).** Retail caller roles and
  the body correct attempt-1's stale assignment of 0x0044da50: this is the
  restart operation, not draw-current-frame. It seeks the active Bink handle
  to frame one, decodes that frame, and copies it to the configured buffer
  using the current pitch, height, and surface flags. The canonical static
  `BinkManager` declarations and `BINK` tag now reflect the DC decorated
  identities while preserving the existing retail-tested alias surface used
  by `smackmgr`. Aggregate coverage reached 824/1248 exact and 49.79% fuzzy;
  all ratchet, VA, single-view, and cleanliness gates remain clean.

- **2026-08-12 — `combatcontrolsubwindow.obj` entered the build with
  `TCombatHeroSubWindow::Show` and `UnShow` exact (116/116 and 58/58).**
  Two retail allocation sites prove the complete 0x5c-byte object: the
  canonical 0x34-byte `TSubWindow` base, nine widget pointers through +0x54,
  and `shown` at +0x58. The paired bodies independently corroborate that
  layout while toggling `WIDGET_ACTIVE | WIDGET_DRAWN` across the inherited
  widget vector, saving/restoring the background, and updating the exposed
  rectangle. Natural Dinkumware iterator loops reproduce retail's raw-pointer
  walk exactly. One semantic diff reports only delinker symbol-name aliases
  for `gpWindowManager`/`UpdateScreen`; instruction and relocation bytes are
  exact. Aggregate coverage reached 823/1247 exact, 49.78% fuzzy, and 12.30%
  executable matched with all gates clean.

- **2026-08-12 — `CanBuy` closed exact at 0x00461130 (92/92 bytes).**
  Source-order mapping places the DC identity immediately after
  `GetBuildingName`; the retail body independently proves the boundary and
  implementation. It asks the town for the seven-column building-cost row,
  selects `gpGame->players[gNetLocalGamePos]`, and returns false on the first
  resource deficit. The natural array-and-loop spelling reproduces the VC6
  frame, register allocation, and both return epilogues without adjustment.
  The build→delink→build cycle raised aggregate coverage to 821/1245 exact,
  49.77% fuzzy, and 12.29% executable matched with all gates clean.

- **2026-08-12 — `csequence.obj` closed 2/2 exact as the smallest
  actionable unadmitted TU.** Retail's 68-byte int constructor proves the
  complete 12-byte DC layout unchanged: `numFrames` +0, `allocatedFrames`
  +4, and `CSpriteFrame** f` +8. Natural `new CSpriteFrame*[num]` plus the
  zeroing loop reproduces every constructor byte. The direct
  `CSprite::AddFrame(sequence, frame)` caller settles the 38-byte overload as
  `AddFrame(CSpriteFrame*)`, overruling an older HD-derived `const char*`
  name assignment; its successful store must be the fallthrough and its
  full-array zero return the tail block to reproduce VC6 exactly. The
  default constructor, destructor, and richer overloads have no distinct
  rows in the retail bracket; the adjacent 15-byte deleting destructor is
  compiler-generated in `csprite` on the DC roster. Aggregate coverage
  reached 820/1244 exact and 49.76% fuzzy with all gates clean.

- **2026-08-12 — the combat Teleport reachability pair closed exact:
  `searchArray::mark_teleport` 664/664 and
  `combatManager::is_valid_teleport` 195/195.** The first function rebuilds
  all 187 path cells, excludes the two invisible columns, combines
  `army::CanFit` with the spell-side predicate, and preserves the retail
  enemy-marking walk. Dreamcast's call graph supplied the identities and
  original inline roster; retail independently proved every boundary,
  field, branch, and call target. The decisive VC6 spelling was the
  DC-attested inline `mark_enemy(hex,cost)` with separate `hexcell*` and
  `pathCell*` locals: that changed the initial 71.33% reconstruction to the
  exact retail register allocation. The source deliberately preserves a
  retail quirk: wide enemies invoke `mark_enemy` twice on the same anchor
  hex rather than marking a second hex. The located spell predicate rejects
  the current cell or a failed fit, gets Teleport (literal 0x3f) mastery from
  the acting hero, and enforces mastery 2 for crossing a moat and mastery 3
  for bypassing line of sight. The spell stays literal rather than adding an
  enumerator to the measured type-population-sensitive `armygrp.h`. The
  required build→delink→build cycle raised aggregate coverage to
  818/1242 exact, 49.75% fuzzy, and 12.28% executable matched, with every
  ratchet and cleanliness gate clean.

- **2026-08-12 — `university_window.obj` entered the build with
  `type_university_window::ExitDialog` exact (45/45 bytes).** Link-order
  bracketing between `u2dvers` and `victorylossconditions`, the retail
  15-slot vtable's slot 14, and Dreamcast's decorated public identity agree
  on 0x005f1180. The body converts the message to `MESSAGE_WIDGET`, clears
  `heroWindowManager::dialogReturn`, assigns codeY/codeX 10 in VC6's chained
  store order, and forwards dispatch. The same mapping campaign independently
  corroborated `type_university_window : CAdvPopup`: retail's proven 0x60
  base shifts DC's rollover pointer and four-skill array from +0x68/+0x6c to
  the observed +0x70/+0x74, while the two widened Dinkumware vectors explain
  the later +0xdc/+0xec data pointers. No tail-layout view was fabricated;
  unresolved methods remain fenced until promoted. The required
  build→delink→build cycle raised aggregate coverage to 816/1241 exact
  at 49.59% fuzzy, with the ratchet, VA, single-view, and cleanliness gates
  clean.

- **2026-08-11 — `combatManager::combatManager` was promoted from a 0%
  carcass to all 295 retail bytes exact.** The constructor independently
  proves `combatManager : baseManager`, the 187-entry `hexcell` array at
  +0x1c4, two 16-byte Dinkumware set records beginning at +0x545c, the 42
  `army` records at +0x54cc, the empty vector representation at +0x13d58,
  and three non-trivial 36-byte archer records at +0x13d78. The nested
  archer destructor's two reverse-order `CSprite::Dispose` calls corroborate
  the sprite fields at +4/+8. VC6 schedules the four scalar assignments in
  retail order only when they are constructor-body statements rather than
  member initializers. Correcting the Eagle Eye record base simultaneously
  advanced the unchanged `LearnSpellFromEagleEye` body from 89.37% to
  99.98%; its instruction bytes now agree and only two flat delinker
  relocation names remain. Aggregate coverage reached 815/1240 exact and
  49.59% fuzzy before the full ratchet build.

- **2026-08-11 — `TGameTypeWindow`'s constructor was promoted from a 0% carcass
  to a complete 93.73% source reconstruction.** Retail proves the full-screen
  `heroWindow`, nine-widget reserve, mode-indexed `newgame.pcx`/`loadgame.pcx`
  banner, restricted-menu gate, five button recipes, and the final
  `AddWidget`/`MemError` walk. The five coordinate rows are one canonical
  packed-four-short type in `gametypewindow.h`, backed by the 40 bytes at
  0x63e6b0. Retail's `bitmapBorder16` constructor ends in `ret 0x1c`, proving
  that PC takes seven arguments and dropped Dreamcast's trailing `focusable`
  byte. The unit is now four exact functions plus the constructor residual;
  its only structural delta is the known VC6 nested-inline choice inside
  `vector<widget*>::reserve` (range-destroy versus `size`). Direct/reference
  row forms, both include orders, a reserve adapter, and a 1..8 type-population
  sweep bounded that plateau. Aggregate fuzzy coverage rose 49.35% -> 49.52%.

- **2026-08-11 — eight redundant partial-layout views and one TU-only chat
  declaration were retired without weakening the match ratchet.** Canonical
  fields now serve `TAdventureMapWindow::chatEdit`, `hero::targetIsCritical`,
  the packed ground/road/river flags, `mine`, the university vector, and the
  complete `CChatManager`; the unnamed 16-byte adventure-traits row is kept
  honestly raw instead of receiving a fabricated class. The earlier
  integration note saying the input and AI views were required is therefore
  superseded: with the current combined type population both canonical fields
  retain every established maximum. Seven compatibility layouts remain
  deliberately: the two large object renderers, `SetRolloverText`, and the
  creature-bank helper each regress under their removal. The contemporary
  `TSplitSliderView` claim is superseded by the 2026-08-28 canonical `slider`
  closure. The final
  full build remains at 813/1240 exact, 48.61% fuzzy, with all 1,240 ratchets
  clean and every cleanliness counter at zero.

- **2026-08-11 — `TPickANumber` closed 2/2 exact after recovering its
  retail VC6 container layout.** The four apparent fields at +8..+14 are
  Dinkumware `vector<unsigned char>`: its empty allocator byte at +8 and
  `_First`/`_Last`/`_End` pointers at +0xc/+0x10/+0x14. This explains the
  constructor's formerly anomalous copy from `[ebp+0xb]` as VC6 copying the
  empty default allocator temporary, and `Pick`'s otherwise redundant reload
  from +0xc as the inlined `vector::operator[]`. The natural member
  initializer `marks(count, 1)` reproduces the complete 85-byte constructor;
  the existing selection loop then reproduces all 82 bytes of `Pick`.
  Dreamcast CodeView's `vector<bool>` helpers describe its STLport build, not
  the retail PC representation. Both existing consumers,
  `combatManager::PlaceLargeObstacle` and `PlaceAllObstacles`, remain exact,
  including inlined vector destruction. No external implementation body was
  used.

- **2026-08-11 — the temporary attempt-1 porting gate was removed.** It applied
  only while material from `decomp-attempt-1` was being ported, and that port
  is complete. Matching work needs no per-function or per-admission approval.
  External sources remain secondary evidence: names, layouts, and source
  shapes still require independent retail corroboration, and the retail
  executable remains authoritative.

- **2026-08-11 — `mapcell.obj` entered the build with seven byte-exact
  functions and one measured residual.** Exact bodies are
  `ExtraInfoUnion::SetCellVisited`, `NewmapCell::get_map_extraInfo`,
  `cell_is_trigger`, `HasTriggerableEvent`, the packed cell constructor and
  destructor, and `CObject::get_object_type_ptr`. Retail proves the
  Dinkumware vector at cell +0x0e: its allocator occupies four bytes and its
  first/last/end pointers land at +0x12/+0x16/+0x1a; VC6 consequently emits
  both lifetime functions exactly. `SetCellVisited` also proves the eight-bit
  visited-team lane at bits 5..12 and the DC-attested inline `game::GetTeam`
  shape. `get_map_object` is semantically complete at 93.48485%; only the
  boat-index EAX/EDX versus ECX schedule remains after signed/unsigned local
  and shared-tail probes. The other 60 located rows remain compiled-out
  carcasses, so the TU is admitted but not closed. The same random new-module
  pass classified `ds_engine.obj` as Dreamcast-only: all 108 bodies implement
  the WinCE DirectSound backend, while retail PC uses the Miles/AIL path and
  has no `ds_engine` slot in the exhausted post-drawing bracket.

- **2026-08-11 — `puzzlewindow.obj` entered the build with five exact retail
  bodies and two measured residuals.** The retail window layout is now proven
  at 0x12c bytes (0x60-byte `CAdvPopup`, resource display at +0x64, 48 bitmap
  pointers at +0x68, puzzle selector at +0x128). The scalar destructor,
  destructor, `WindowHandler`, `UpdatePuzzle`, and `Bitmap816::mark_puzzle`
  are byte-exact. The 904-byte window constructor is 98.60596% with an exact
  branch graph after restoring the single-call `get_puzzle_bitmap` helper;
  its remaining deltas are VC6 instruction ordering around the heading text
  and `button::set_hotkey`. The packed 16-byte AI tile constructor is
  98.84259%, with only the 10-bit object-type assignment's EAX/EDX choice
  remaining. `AI_attempt_puzzle_guess` is located at 0x52c9b0 but remains a
  carcass, so the TU is not closed.

- **2026-08-11 — `dialogbox.obj` CLOSED 12/12 exact (functions-only).** The
  fresh retail span is 0x48fdc0..0x490b67: both `TDialogBox` constructors, its
  destructor pair and 2,099-byte tiled `Setup`; then `CTextDialog`'s destructor
  pair, one-argument constructor, three virtuals, and nonvirtual `ExitDialog`.
  Vtables 0x63db40/0x63db68 prove both class identities and the virtual order;
  CodeView supplies the reference signature of `ExitDialog`, the protected
  text-widget member, and the `Setup`/`UpdateText`/`CalcDimensions` names.
  Retail independently proves the 0x54/0x58 layouts, 64-pixel framing,
  256-pixel background tiles, sequential IDs from 200, lower-case
  `dialgbox.def`/`diboxbck.pcx` assets, and the complete ownership/registration
  loops. The large setup is exact with retail's reservation formula: full
  256-pixel tile count plus the complete 64-pixel grid-cell count. The
  three-argument `CTextDialog` constructor has no retail body or reference;
  the image's only code relocation to its vtable is the admitted one-argument
  constructor, so that Dreamcast row is retail-dropped rather than forced.
  The left flank at 0x48fc20 belongs to the preceding multi-unit bracket; the
  right flank begins the independently classified `diff.obj` cinit family at
  0x490b70. Every carved function inside the refreshed span is claimed and
  byte-exact.

- **2026-08-11 — `scenarioinfo` admitted four retail functions, all exact.**
  The scenario-selection caller at 0x513740 reserves a 0xb4-byte stack object,
  calls the constructor at 0x567290, runs it modally, and directly calls the
  destructor at 0x569800. The constructor and destructor both store vtable
  0x641710, whose 15-slot `CAdvPopup` topology independently places
  `ProcessRightSelect` in slot 11 and `OnWidgetDeselect` in slot 12; the class
  identity is now recorded in `retail-vtables.tsv`. The 152-byte destructor
  proves four leading `CSprite*` fields at +0x60..+0x6c, interleaved disposal
  of the eight-element `Panels`/+0x70 and `Flags`/+0x90 arrays, and
  `heroSpecificAbility` at +0xb0. Those offsets are the DC field list shifted
  by retail's independently proven eight-byte larger popup base. Its ordinary
  body and 33-byte scalar-deleting wrapper are byte-exact. The 30-byte
  `OnWidgetDeselect` override sets the exit flag only for widget 188 and then
  delegates to `CHeroWindowEx`; it is exact. The 242-byte
  `UpdateAllyEnemyFlags` body is exact after restoring retail's eight-player
  loop, widget ranges 112..119/120..127, `setup.playerPos` gate, inlined
  `OnSameTeam`, and per-arm message order. `SetDifficultyHiLite` has no
  standalone retail row: its difficulty-to-frame switch is visibly inlined at
  0x56921f inside the constructor. This is not TU closure: the 8,457-byte
  constructor, 800-byte `ProcessRightSelect`, and the retail-only/local popup
  block at 0x5693a0..0x5697c4 remain.

- **2026-08-11 — `TAdventureOptionsWindow` construction was reconstructed to
  89.5711%.** The 1,194-byte retail body proves the 289x387 popup at (255,106),
  an eight-slot widget reservation, the palette-colored `AdvOpts.pcx`
  background, five action buttons, accept button, and rollover text widget.
  All coordinates, IDs, sprite names, frames, styles, hotkeys, the complete
  widget-registration loop, and the local-human/current-hero enable gates are
  represented. Its unique caller and vtable store prove class vtable 0x63a610,
  now named in `retail-vtables.tsv`. The source-private name of the no-argument
  game helper at 0x49da70 did not survive; it remains an ordinal declaration,
  while retail proves its byte return feeds the End Turn button's virtual
  `enable` call. The only structural delta is the established
  `vector<widget*>` reserve wall: retail calls shared `_Ucopy` and empty-range
  destroy helpers, whereas this SP3 invocation expands the copy/cleanup path,
  adding two branches and changing downstream scheduling. The same template
  instantiation's depth, adapter, local-order, and type-population probes were
  already exhausted in the quick-info and split-window lanes, so the residual
  is recorded rather than re-ground. The destructor pair remains exact.
  **Superseded 2026-08-28:** `WindowHandler`'s 99.9367% register symptom came
  from erased Dreamcast exit state and a duplicated source tail; restoring the
  state closes all 508 bytes.

- **2026-08-11 — `executive::CallManager` advanced 68.3663% ->
  98.1387%, and its three contiguous catch-handler splits were retired.**
  The bootstrap ended the parent at 0x4b0dc2, but the following handlers at
  0x4b0dc2/0x4b0dd7/0x4b0e25 all use CallManager's saved EBP frame: they
  remove the temporary manager, perform the reduced saved-manager resume,
  and restore `currentManager`, respectively, before rethrowing. Three nested
  source `try`/`catch (...)` scopes reproduce the exact state 0/1/2
  transitions and VC6 emits one 0x1d0-byte section ending at `MainLoop`'s
  independent 0x4b0e40 entry. The manual function inventory therefore merges
  the former 338+21+78+18 split rows into the single 464-byte parent, following
  the already admitted `InitImmMouse` catch-boundary precedent. The HoMM2 twin
  supplied only the saved/remove/add/main/remove/add/restore statement order;
  retail independently proves all HoMM3 adventure-manager suspend/resume
  behavior, exception cleanup, calls, fields, arguments, and branches. The
  best spelling uses `saved` for the first comparison but removes
  `currentManager` directly, matching retail's initial EAX lifetime. All seven
  main-body branches, the return, EH states, catch edges, raw section extent,
  and padding agree. Five load-chain/argument register choices remain after
  direct-current, scoped-window, and restore-error-expression probes; the
  source records that measured compiler plateau. No HoMM2 address, type,
  constant, or HoMM3-specific implementation statement was adopted.

- **2026-08-11 — `spelldefs.obj` CLOSED 6/6 exact (functions-only).**
  Retail and Dreamcast source order place the unit at 0x59e060..0x59e50f:
  `SpellTargetsASingleArmy`, `InitializeSpellTraitsTable`, the source-private
  `InitializeSpellTraits`, and three lazy-static array cleanup thunks. Direct
  `atexit` relocations prove the formerly uncarved thunk entries at
  0x59e4b0/0x59e4d0/0x59e4f0; all three call the shared element destructor
  through VC6's vector-destruction iterator and are now admitted to the manual
  function inventory. The following eleven rows at 0x59e510..0x59e8ff are the
  excluded guard/`atexit` cinit family, and `spells.obj` begins independently
  at 0x59e900. The four Dreamcast `TAutoStrPtr` methods have no TU-local retail
  rows: construction/destruction use ICF-shared representatives and `set`/`get`
  are fully inlined. Retail proves the target-flag tests, the 92-row
  `sptraits.txt` traversal, the 81-by-136-byte backing table, all numeric
  columns, and the three interleaved lazy string arrays. The resulting six
  emitted bodies, including the 863-byte loader, reproduce retail exactly.
  Dreamcast CodeView supplies names, signatures, and source order only; no
  external implementation body was used.

- **2026-08-11 — `iconWidget::Main` admitted at 95.27076% (756 bytes),
  completing the retail message-handler semantics.** Vtable 0x63ec48 slot 2,
  the Dreamcast roster position, and the surrounding independently named
  iconWidget rows identify 0x4ea810. Retail proves the sleep/active/disabled
  gates; signed-short hit testing; left/right select and deselect messages;
  the virtual `handle_click` calls; and the six live widget commands for
  sprite, frame, sequence, color, palette, and player recoloring. The palette
  arms independently corroborate `ResourceManager::GetPalette`,
  `CSprite::SetPalette`/`GetPalette`, both player-color overloads, and the
  dropped source helpers `SetIconSequence`, `SetPalette`, and
  `SetPlayerPaletteColors`, whose behavior survives fully inlined here. The
  reconstructed CFG has the exact 28 branches. After command-arm ordering,
  shared-exit, helper-boundary, lifetime, register-hint, and label-placement
  probes, the residual is one C2-duplicated zero epilogue plus the inlined
  sequence parameter's scheduling/register choice; `why-reg` confirms the
  first ESI/EDI/EBX definitions already agree and finds no first-definition
  source handle. No external implementation body was used.

- **2026-08-11 — `heroWindow::CenterWindow` smallest-lane re-audit keeps
  the 92.8767% compiler-state plateau.** Dreamcast CodeView's exact local
  names `startX`, `startY`, `startW`, and `startH` replace the provisional
  `old*` spellings without changing one byte. The current v2 allocator model
  confirms the retail/base CFGs and all nine branches agree, but their three
  call-crossing pseudos reach VC6 in different front-end orders. Its best
  generated edit aliases `centerX` into a fifth local, reducing the
  register-visible distance from 79 to 61 slots without matching; that local
  is absent from CodeView's exhaustive four-local roster and is rejected.
  Adjacent-declaration candidates leave 77/81 slots. The evidence-compatible
  source and ratcheted maximum therefore stay unchanged; no external body was
  used.

- **2026-08-11 — `get_elemental_type` admitted byte-exact (60 bytes) and
  `spells.obj` entered the build.** Retail's bounded jump table accepts the
  contiguous summon-spell IDs 0x42..0x45 and returns Fire, Earth, Water and
  Air elemental IDs 0x72, 0x71, 0x73 and 0x70 respectively, with -1 for every
  other spell. The canonical four-case switch reproduces the bounds check,
  dispatch order, five returns, padding and jump-table data exactly. Retail
  proves all values and control flow; Dreamcast CodeView supplies the spell,
  creature and function names only. The remaining large Dreamcast roster is
  fenced as carcasses, so this is not a TU-closure claim. No external
  implementation body was used.

- **2026-08-11 — `resourcemanager.obj` entered the build with two retail-
  proven cache functions and its cache owner.** The 64-byte `AddToCache` at
  0x5594b0 is identified by its 12-byte resource-name key copy, map insertion,
  and reference-count increment, which appear verbatim inlined at the tail of
  the independently located 138-byte `GetSpreadsheet` at 0x55c0a0. The
  latter is also anchored by the `cranim.txt` parser caller, its cache lookup,
  and the spreadsheet loader at 0x55be60. Their accesses prove the cache map
  object at 0x69e528, its sentinel pointer at +4, node key at +0xc, resource
  pointer at +0x1c, and the 13-byte key layout. `AddToCache` reaches 86.6296%
  and `GetSpreadsheet` 93.6111%; both have exact CFGs and are capped at the
  same fourteen caller-saved scheduling slots by the VC6 v2 model. Named
  pointer, POD, insertion-temporary, nested-pair and genuine Dinkumware-map
  variants were measured and rejected. A smallest-lane re-audit after the
  later include/layout work also swept zero through eight additional
  user-defined types; every count was byte-identical at both established
  scores, so the known include-population lever does not reach this TU. The
  remaining Dreamcast roster stays fenced as carcasses. Dreamcast supplies
  names/signatures only; no external implementation body was used.

- **2026-08-11 — `advManager::RedrawAdvScreen` admitted byte-exact
  (326 bytes).** Retail gets the local player ordinal, loads `AdvMap.pcx`,
  applies that player's 16-bit palette, draws the bitmap over the full screen,
  disposes it, and clears the manager field at +0x38c. It then refreshes
  buttons, hero/town locators, quest log, bottom view, resources and locator
  highlighting; draws the adventure window over ids -65535..65535; performs a
  complete draw and radar update from the packed +0xe4 origin; and, when
  `bUpdate` is set, updates the 800x600 screen. `bForceSaveBorder` is unused in
  retail. All five blocks, three branches, the return, calls, arguments,
  packed-point extraction, instructions, and registers match. Retail proves
  every operation and layout; Dreamcast CodeView contributes only the
  signature and the local names `player_id` and `bmp`. No external
  implementation body was used.

- **2026-08-11 — `PopupPlayerTurnInfo` admitted byte-exact (369 bytes),
  and retail `NormalDialogTimeOut` located at 0x4f6530.** The popup restores
  and foregrounds the application window, stops MP3 playback, selects the
  default cursor, clears a non-local player's bottom-view override, arms the
  sound-manager popup state, and plays `SysMsg.wav`. Unless the retail sound
  latch suppresses it, it formats the current player's name into a 256-byte
  buffer, shows a 15-second dialog, and repeats the restore/foreground/sample
  sequence every 500 ms while the dialog returns 9999 and the turn duration
  has not expired, finally releasing the current `SAMPLE2`. All twelve blocks,
  six branches, the single return, instructions, and register choices match.
  The called 52-byte predecessor of `NormalDialog` is byte-identical in shape
  to Dreamcast `NormalDialogTimeOut`: it merely moves `timeOut` into
  `NormalDialog`'s timeout slot while forwarding the other eleven arguments,
  so its former `DC_ONLY` classification is retired. Retail proves every
  global offset, call, literal, dialog argument, and control-flow edge;
  Dreamcast CodeView supplies the two function names, signatures, `sample2`,
  and `cText` local names only. No external implementation body was used.

- **2026-08-11 — `advManager::UpdBottomViewEnemyTurn` admitted byte-exact
  (135 bytes).** Retail compares the current bottom-view discriminator at
  `advManager +0x394` with ordinal 5, returns false without using
  `force_update` when it is already active, and otherwise clears the existing
  adventure-window view, stores ordinal 5, allocates 0x74 bytes, constructs a
  `TBottomViewEnemyTurn` from `advWindow`, installs it, and returns true.
  Dreamcast CodeView supplies the function/parameter names, the local
  `bChanged`, the constructor signature, and the zero-offset TSubWindow
  inheritance; retail independently proves the 0x34 base boundary, 0x74
  allocation, call targets, field offset, and two-slot base vtable. The header
  therefore admits only byte-bounded narrow bottom-view layouts, leaving all
  unobserved derived fields padded and unnamed. No external implementation
  body was used.

- **2026-08-11 — retail-only `InitImmMouse` admitted byte-exact, including
  its catch funclets; the bootstrap carve and unwind owner were corrected.**
  WinMain's sole post-window call proves the two-register `/Gr` signature.
  Retail then tests the guard byte at 0x696d58, constructs the static
  Immersion wrapper at 0x696d78 from `(hInst, hwnd)`, registers its destructor,
  and returns true. The contiguous 0x4b68f4 dispatch and 0x4b68fa catch body
  return false when construction throws, proving the source-level
  `try`/`catch (...)` and extending the real function from the bootstrap's
  100-byte split to 125 bytes. Accordingly `config/retail-functions.tsv`
  removes the false 0x4b68f4 function, records the actual 58-byte static
  destructor at 0x4b6910, and `config/retail-funclets.tsv` assigns the
  0x62b5f0 unwind record to Init rather than the preceding 0x4b6730 body.
  VC6 reproduces all 125 bytes and both funclets exactly. The wrapper name
  remains explicitly provisional; no Immersion implementation body or
  external source was imported.

- **2026-08-11 — `combatresultswindow.obj` located and admitted at 6/7
  exact.** The unique `CPResult.pcx` reference and Dreamcast source order
  bind the retail unit to 0x4702d0..0x471c13: the 5997-byte constructor,
  scalar deleting destructor, destructor, `Open`, `Close`, `DoModal`, and
  `CombatResultsWindowHandler`. The constructor's direct heroWindow base and
  vptr store prove a storage-free 0x4c-byte derived class and identify the
  nine-slot vtable at 0x63d46c; its EH-heavy body remains an explicit
  carcass. All six remaining entries are byte-exact. The destructor clears
  the source-private active-window cell at 0x694fbc and deletes the widget
  vector; `Open` forces the base update flag off, redraws the video frame,
  then conditionally refreshes the full 800x600 screen; `Close` stops video
  before the base close; and `DoModal` installs the handler. The handler
  polls sound, accepts the split-result widget command or the shared absolute
  dialog deadline, writes the window manager's result slot, synthesizes
  `WIDGET_END_DIALOG`, clears the deadline, and returns the proved dispatch
  verdict. Its exact VC6 shape requires the source-level label inside the
  success conditional: the timeout arm jumps directly to it while the normal
  arm tests the byte exit flag. Dreamcast CodeView contributes names,
  signatures, the 200..221 widget-id roster, and source order only; retail
  independently proves every admitted address, call, constant, field access,
  and branch. No external implementation body was used.

- **2026-08-11 — `bitmap8.obj` is retail-dropped.** The exhaustive
  bitmap-family order map assigns 0x44ed20..0x44f787 to all nine Bitmap24Bit
  entries, ending in its 1528-byte `AdjustHSV`; the only code before
  Bitmap816's deleting destructor at 0x44f7d0 is the excluded cinit pair at
  0x44f790/0x44f7b0 (both guard-byte/`atexit` records). Retail's adjacent
  vtable interval likewise runs directly from Bitmap24Bit at 0x63b9f4 to
  Bitmap816 at 0x63ba14, with no Bitmap8Bit table, and the image has no
  Bitmap8Bit function/address-take evidence. All nine Dreamcast rows therefore
  remain `DC_ONLY`; the surviving `RType_bitmap8` resource-type spelling does
  not imply that the Dreamcast wrapper class was linked. No neighboring
  bitmap body was misclaimed and no external implementation body was used.

- **2026-08-11 — `advManager::ProcessWaitingHover` advanced to 80.9400%,
  and iconWidget's two random-sequence tables now use their CodeView-attested
  aggregate type.** Retail proves that TOWN hover gates only on local-human
  ownership and selects cursor 3 from `currTownId`, while HERO hover also
  requires `owner == gNetLocalGamePos` and selects cursor 2 from
  `currHeroId`; the action ordering is likewise byte-visible. With named
  `rx`/`ry`, `town`, and `hero` locals, all 27 branches and four returns now
  agree symbolically. The residual is a 16-byte-versus-8-byte local frame and
  packed-point register coloring; sharing the two scoped rx/ry pairs regressed
  to 80.27% and was rejected. Dreamcast type records independently prove both
  icon tables are local const arrays of anonymous
  `{creature_seqid sequence_id; int chance;}` records. Restoring that type and
  the attested enum names leaves the 72.0056%/86.4111% bytes unchanged,
  confirming the siege residual is still VC6 loop-invariant hoisting, not
  missing const qualification. No external implementation body was used.

- **2026-08-11 — `slider.obj` located and admitted at 14/16 exact, with
  all sixteen claimed bodies accounted for.** Retail's contiguous block is
  0x596020..0x597184: scalar deleting dtor, initialize, the ten-argument
  constructor, dtor, SetState, five large input/draw bodies, SetKnob,
  UpdateResolution, SetResolution, both focus hooks and enable. The
  constructor's vptr store admits `slider` vtable 0x641d50 (17 slots); its
  final retail-only slot points at the ICF/shared empty `widget::Close` body.
  Complete reorders the Dreamcast fields into a byte-proven 0x68-byte layout:
  resource pointers at +0x30/+0x34, state/knob/count geometry through +0x54,
  input flags at +0x5c/+0x5d, focus at +0x60 and the callback at +0x64.
  Retail bytes independently fix the six resource literals and both resource
  manager callees; Dreamcast CodeView supplies only names, signatures and the
  BROWN/BLUE enum spellings. `KeyAccel`, `Select`, `Deselect`, and `Draw` now
  join the scalar dtor, initialize, ctor, dtor, SetState, UpdateResolution,
  SetResolution, both focus hooks and enable as byte-exact. Their retail
  shapes additionally prove the private left/right modifier latch at
  0x69fdd4 and the canonical 16-bit-bitmap `CSprite::DrawInterface` inline
  wrapper; the latter was added to the live header and recompiles all users
  without exact-match regressions. `Deselect` requires the source-level
  successful-decrement edge to the shared redraw tail, while `Select` fixes
  the horizontal knob/page comparison polarity. `Main` is semantically
  complete at 95.190125% with the retail 60-branch/11-return CFG: retail C2
  shares the KP3/KP2 forward `KeyAccel` suffix, whereas this invocation shares
  the equivalent KP9/KP8 backward suffix after case-order, exit, grouping and
  lifetime probes. SetKnob remains semantically complete at 88.44% with an
  identical CFG and exact 64-instruction tail; `why-reg --model --il-order`
  reduces its sole ESI/EDI head transposition to C1 front-end handle state.
  The default ctor has no standalone retail entry, while
  GetRealWidth/Height/zBufferDraw are ICF/header COMDAT representatives owned
  elsewhere. No external implementation body was copied or used.

- **2026-08-11 — `artifact.obj` located and admitted at 2/3 exact, with
  cinit ownership bounded but not fabricated.** Retail link order places the
  unit between `armygrp.obj` and `basemgr.obj`: its two bitset cinit families
  occupy 0x44c700..0x44cd4f, the sole surviving source body is the 1512-byte
  `InitializeArtifactTraitsTable` at 0x44cd50, its two static-array cleanup
  thunks are 0x44d340/0x44d360, and the Dinkumware bitset COMDAT tail ends at
  0x44d50f before `basemgr` begins. Both cleanup thunks reproduce exactly.
  The main body is reconstructed from retail at 75.01%: it loads 146
  `artraits.txt` rows into the proved 144-by-32-byte traits table, derives 15
  allowable-slot classes from 19 columns, applies the three disabled and nine
  spell-giving IDs, links twelve combination artifacts, then packs 19
  `artslots.txt` names and their mask types. This proves the complete traits
  layout (name, cost, slot mask, class, description, combo/target indices,
  disabled and gives-spells flags), the 19-by-8 slot-traits table, both public
  reference cells, and the two owned string buffers. Dreamcast CodeView is
  used only for source names and the AB-era member prefix; retail fixes the
  Complete counts, added fields, strings, control flow and addresses. The
  15-mask and 12-combination storage addresses are declared for relocation
  authority, but their excluded cinit source initializers remain unadmitted;
  no external implementation body or contradictory external address was
  imported. The residual is VC6 nested-inlining/EH tail shape after direct
  `set`/`operator[]`, helper, lifetime and loop-induction probes, so the TU is
  not declared closed.

- **2026-08-11 — `diff.obj` located and admitted at 1/4 exact, with its
  complete retail source-body surface accounted.** The ten terrain-mask cinit
  bodies at 0x490b70..0x490f5f follow `dialogbox.obj`'s final virtual leaf and
  prove the TU's `terrain.h` include; four ordinary bodies occupy
  0x490f60..0x4912ff, and the next ten-mask cinit family begins
  `dimensiondoorwindow.obj` at 0x491300. Body semantics and the 16-byte maker /
  12-byte record layouts identify `CDiffFile::Apply` (197 B), the byte-exact
  `CDiffMaker` constructor (32 B), `FindNextSame` (237 B), and `MakeDiff`
  (447 B). Dreamcast CodeView corrects the latter two interfaces to `int&` and
  `unsigned long&`, and its line/locals evidence corroborates the indexed
  same-byte loop, reference-selecting maximum, three scoped record locals,
  and direct terminal return; retail fixes all bounds, constants, operand
  order, allocation size, and record serialization. The other four DC source
  rows are exhausted rather than forced: `GetData`, `CountSameBytes`, and the
  record constructor are visibly inlined into these bodies, while the empty
  private file constructor is unreferenced and dropped. The three residuals
  retain exact extents and retail CFGs at 68.9643%, 83.2813%, and 81.5174%; all
  remaining differences are measured VC6 register/block placement plateaus
  after source-shape, signature, include-closure, declaration-order, register
  hint, and inert-type probes. No external implementation body was used, and
  the TU is not declared CLOSED while those three byte residuals remain.

- **2026-08-11 — `command.obj` admitted at 2/11 exact for its first
  bounded leaf slice.** Existing anchor/call-order evidence bounds the
  admitted command span at 0x474bf0..0x47a2cc; nine previously located
  large command bodies remain explicit zero-match carcasses. The two exact
  leaves are `combatManager::is_outside_placement_boundry` (0x4763f0,
  77 B) and `valid_wall_target` (0x476440, 80 B). The former independently
  exposes `placementBoundaryDepth` at manager+0x13d70 and computes signed
  modulo-17 side limits `2*n+1` / `2*n+15`. The latter gates tower targets
  on Castle/Citadel tiers and indexes the already byte-bounded
  `gWallTargets[].wall_id` into manager+0x13f60. Admitting this partial TU
  deliberately exposes the nine known residuals in the measured
  denominator; no broad command handler or combat routine is guessed.

- **2026-08-11 — `viewarmywindow.obj` located and admitted; 7 claims now
  include four exact rows and three reconstructed action helpers.**
  `crstkpu.pcx` and the constructor's vtable store place
  the TU at 0x5f3360 with vtable 0x643c14; the following `viewwrld.obj`
  begins at the independently identifying `vworld.pcx` constructor
  0x5fa600. The first slice closes TViewArmyWindow's scalar deleting
  destructor, 213-byte destructor, `convertID2HelpID`, and `DoModal`.
  Retail allocation/destruction proves a 0xb8-byte CAdvPopup derivative:
  the two VC6 strings have data pointers at +0x70/+0x84, and the destructor
  deletes the Widgets entries without erasing the vector before the string
  and base teardowns. The ID mapper's exact 300-byte form requires an
  explicit negative guard and one source arm per widget ID—even where arms
  return the same help index—so VC6 emits retail's direct 21-entry jump
  table instead of its smaller two-level byte dispatch. The 92-byte modal
  body exactly resets `glTimers[0]`, disables upgrade/dismiss for a non-local
  player, and invokes the base modal pump. The two constructor call sets,
  no-argument member ABI, unique literals, widget IDs, coordinates, hotkeys,
  and allocation callees additionally map `create_ok_widget` to 0x5f6870,
  `create_upgrade_widget` to 0x5f6ae0, and `create_dismiss_widget` to
  0x5f6d50. Their complete bodies allocate the backing bitmap border and
  action button, add the proved hotkeys, and append both widgets.
  `create_ok_widget` reaches 98.659386% with exact 14-branch/2-return control
  flow; `why-reg` caps its sole EBX/EDI permutation as non-source-nameable
  front-end handle state. Upgrade and dismiss are structurally identical
  twins at 88.11441%: retail retains the Dinkumware pointer-destruction call
  and inlines `vector::size` in the second `push_back`, while this VC6
  invocation makes the opposite `/Ob2` choices (14 branches/2 returns versus
  15/3). Canonical `push_back`, explicit `insert(end(),1,value)`, and explicit
  derived-to-base conversion forms were measured; the latter is inert and
  `insert` regresses both to 78.31356%. Large constructors, remaining helper
  builders, and the handler remain carcasses; Dreamcast `QuickView` has no
  independently identified retail row and is not forced onto `DoModal`. No
  external implementation body was used.

- **2026-08-11 — `hiscore.obj` located and admitted; first 8 claims are
  8/8 exact.** The manager's `hiscore.dat` read and vtable stores bound its
  retail block inside the hillfortwindow..iconwdgt gap: `ResetHighScores`
  begins at 0x4e8fb0, the manager constructor/destructor/Open/ViewHiScore
  run through 0x4e91cf, and `AddScoreToHighScore` begins at 0x4e91d0; the
  window/edit tail ends before iconWidget's deleting destructor at
  0x4ea6f0. Retail-byte vtable use admits 0x63eb8c as highScoreManager,
  0x63eb98 as THighScoreWindow, 0x63ebbc as CHSInputDlg, and 0x63ebf4 as
  CHighScoreEdit. The exact first slice is the manager ctor/dtor,
  CHSInputDlg dtor/deleting-dtor plus OnWidgetDeselect and
  GetRolloverWidget, and THighScoreWindow dtor/deleting-dtor. Its layouts
  retain only byte-bounded spans: Open proves the manager's 0x898-byte
  table at +0x38 and selector at +0x8d0; the shifted DC field list plus
  retail loads prove CHSInputDlg's +0x50/+0x54/+0x58 pointers and
  THighScoreWindow's two backgrounds at +0x108/+0x10c. The latter dtor
  deliberately spells its widget deletion loop locally: retail does not
  call `heroWindow::delete_widgets` and does not erase the vector there.
  Large constructors, updates, handlers, score-table serialization, and
  retail-inlined/dropped DC rows remain carcasses rather than importing
  unreviewed bodies.

- **2026-08-11 — `textresource.obj` CLOSED 9/9 exact
  (functions-only).** Retail's two parameterized parser constructors are
  0x5bbba0 (551 B, CR/LF text rows) and 0x5bbe70 (742 B, CR/LF rows plus
  tab-delimited cells). Their stores independently identify vtables 0x642d98
  as `TTextResource` and 0x642da4 as `TSpreadsheetResource`, correcting the
  contradictory external enrichment; the hand vtable census now records the
  retail-byte identities. Both constructors, destructors, scalar deleting
  destructors, and vtable-slot-2 `GetSize` bodies reproduce byte-for-byte.
  The spreadsheet resize path also emits the exact 51-byte VC6 Dinkumware
  `vector<TStringVector*>::erase(first,last)` at 0x5bc1f0. Dreamcast's two
  default constructors have no cross-compiland callers and no retail entries,
  so `/OPT:REF` dropped them. Its remaining header-origin bodies are STLport
  template machinery, not transferable to retail's Dinkumware library; only
  the independently called/compiled erase semantic analog is admitted. Fresh
  link-order evidence leaves only the recognized 32-byte guard/atexit cinit
  rows at the flanks before the next TU's 0x5bc250 deleting destructor.

- **2026-08-11 — `campaignwindow.obj` located and admitted from retail
  bytes.** The sole `campbkx2.pcx` constructor at 0x45ea40 installs vtable
  0x63bca4 and is followed by the deleting destructor, destructor, 44-byte
  `DoModal`, and address-taken handler through 0x45f55b. Complete changes the
  constructor ABI from Dreamcast's one campaign argument to `(unsigned char
  newGame, int newCampaign)`; `oldmain` pushes both slots and the retail body
  consumes `[ebp+8]` as a byte and `[ebp+0xc]` as the campaign index. The
  constructor and handler remain carcasses. `DoModal` is reconstructed from
  its two byte-proven calls. The destructor is byte-exact: a signed six-row
  campaign-index loop restores each live 12-dword Bink-state snapshot, calls
  `CloseBinkVideo`, saves the cleared snapshot, then performs the common
  widget/global teardown. Together with the generated deleting destructor,
  that closes three of the five claimed functions exactly. Dreamcast
  `HideText` has no standalone retail entry: both the
  constructor and handler inline its widget-id 101..107 loop. The adjacent
  0x45e7c0 campaign-building helper is retail-only and remains unnamed rather
  than being forced onto a Dreamcast row. External name-map addresses that
  fall inside these independently bounded bodies are rejected.

- **2026-08-11 — `ResSw.obj` is retail-dropped.** Dreamcast's compiland is a
  self-contained global `ResolutionSwitch`: its ctor calls `S_to_600`,
  `Switch` calls only `S_to_600`/`S_to_800`, its deleting destructor calls its
  destructor, and no cross-compiland caller exists. If linked on x86, its
  global object would still force the four initializer/atexit contributions;
  the authoritative retail link-order inventory contains neither those nor a
  `ResSw` span. The external name-map's supposed `SCREEN_WIDTH` and
  `SCREEN_HEIGHT` addresses are independently rejected because they fall in
  the middle of an error-format string and have no relocation users. All six
  source rows remain `DC_ONLY`; no retail claims are invented.

- **2026-08-11 — `quicktownwindow.obj` advanced to 5/7 claimed functions
  exact; its 1661-byte town constructor is reconstructed at 96.30877%.**
  The two unique literals `townqvbk.pcx` and `cprsmall.def`, the
  vtable 0x6406f4, and the ordered destructor family bound the source unit at
  0x530120..0x530d3c: town constructor, deleting destructor, garrison
  constructor, destructor, army-display routine, `center`, and the shared
  `QuickWindowWait` representative. The deleting destructor, both destructor
  entries, garrison constructor, `center`, and wait wrapper are byte-exact;
  correcting the garrison title color from PRIMARY (1) to retail WHITE (4)
  closed its last byte delta without making a false ownership claim for the
  still-unowned text cell at 0x6a7a70. The army-display routine reaches 82.95%
  with all ten branches and the retail seven-slot/`ostrstream` behavior
  aligned. Its
  56-byte position table is singly referenced at 0x5309df and admitted at
  0x6823b8. Dreamcast proves `limit(int,int,int)` delegates through the
  const-reference `t_limit` template; that exact layering makes retail
  `center` match. It also corrects `armyGroup::GetNumArmies` to `const` via
  decorated symbol `QBA`, with its 0x44acc0 retail body remaining exact. The
  former town-constructor carcass is now behavior-complete: it reserves the
  CodeView `NWIDGETS=25`, builds the palette-adjusted background, portrait and
  name, derives hall/castle frames from the 64-bit building masks, displays
  Resource Silo icons and daily gold for `ViewAll`, then delegates the army
  row and registers every widget. All 36 branches and its one return agree.
  The residual is compiler scheduling: retail retains vector::reserve's empty
  range destroy and string `_Tidy(false)` helpers and spills an indexed
  resource scan, while this SP3 compile elides/inlines those helpers and walks
  the same seven incomes by pointer. `why-reg v2` finds identical first
  register definitions. A direct call-site name accessor regressed to 95.50%
  (a named `c_str()` local to 93.71%); an enum induction variable did not
  change the resource loop; `inline_depth(1)` was byte-identical. The retail
  name read also retires the placeholder vector model at town +0xc4: it is a
  Dinkumware `std::string cName`, whose `begin()` exposes the +0xc8 text
  pointer without cast debt while preserving the exact `town::town` body.
  The constructor's called `town::GetPortraitFrame(bool)
  const` is independently admitted at 0x5bd700 (71 bytes) and byte-exact; its
  active-Fort, faction, town-state and small-icon arithmetic also proves the
  helper identity and signature. No external implementation body was used.

- **2026-08-11 — `systemoptionswindow.obj` located and admitted from retail
  bytes; `TSystemOptionsWindow::DoModal` is now byte-exact (142 bytes).** The
  sole `sysopbck.pcx` reference locates the 6268-byte constructor at
  0x5b1790, followed by the deleting destructor, destructor, `DoModal`, and
  1566-byte handler through 0x5b375d. `DoModal` clears the CodeView-named
  `bPrefsChanged`, runs the base modal loop, and only writes preferences when
  that byte is set. In a network game, a changed Quick Combat preference is
  copied to the local player's +0xe4 `quickCombat`, wrapped in the 24-byte
  CodeView-named `CCombatTypeMsg` (subtype `RS_COMBAT_TYPE` = 1009), and sent
  guaranteed to destination 127. Retail independently proves all six message
  stores, the exact-address Quick Combat global, send arguments, branches and
  calls; CodeView supplies the member, class, subtype and callee names. The
  constructor and handler remain carcasses; both destructor entries and
  `DoModal` are byte-exact.
  Retail inlines Dreamcast's `convertID2HelpID` switch into the handler and
  `UpdateSystemOptions` at its call sites: there is no standalone entry before
  the unrelated next-compiland constructor at 0x5b3780. The dtor and DoModal
  prove a 0x68 derived layout: the 0x60 `CAdvPopup` base plus
  `bPrefsChanged` at +0x60 and `quickCombatSave` at +0x64. External name-map
  addresses are deliberately rejected: they fall inside these independently
  bounded retail bodies. No external implementation body was used.

- **2026-08-11 — `creaturetype.obj` admitted with its first two retail rows
  byte-exact.** Link order between `command.obj`/`csprite.obj` and the
  `crtraits.txt` initializer, corroborated by the Dreamcast source roster,
  places `IsSiegeWeapon` at 0x47b180 and `UpgradedCreatureType` at 0x47b1a0.
  The 22-byte siege predicate is exactly the inclusive Catapult-through-Ammo
  Cart range. The 113-byte upgrade lookup rejects creatures without a town,
  verifies whether the input is the base or upgraded member of its dwelling
  pair, rejects an already-upgraded input, and returns the base creature's
  upgraded dwelling slot. Repeating the authoritative trait-table expression
  in the final lookup is source-semantic and gives VC6 retail's pointer and
  index lifetimes; both bodies then match byte-for-byte. The previously used
  address-derived declaration in `cmbtmgr.cpp` is retired in favor of the
  admitted source name, and the pending raised-creature member is typed as
  `TCreatureType` without changing its proven four-byte layout. The tempting
  body at 0x529710 is explicitly rejected for this identity: retail proves it
  is a `game`-object wrapper around the 0x47b1a0 leaf, with an additional
  ruleset guard. The remaining source rows stay carcasses; no external
  implementation body was used.

- **2026-08-11 — `TSplitWindow::WindowHandler` was measured at 99.9170%.** Its
  34 blocks and 17 branches agreed and the visible delta looked like an EAX/EDX
  scratch swap around `splitSlider->SetState`. A `why-reg` v2 run found no
  source-nameable B14 edit. **Superseded 2026-08-28:** the candidate itself had
  deleted Dreamcast-proven state and duplicated a source tail. Preserving those
  facts through a 70.4149% valley, then restoring the line-291 shared slider
  call and lines-313-318 positive hover scope, closes the function exactly.

- **2026-08-11 — `global.obj`'s two Dreamcast source rows are retail-dropped,
  but its retail-only family remains open.** Both rows are STLport bitset
  constructors; the x86 build uses the shipped VC6 Dinkumware library and has
  no corresponding entries after the ten terrain initializer blocks. The
  next retail functions are the Complete-only `TGzInflateBuf` family beginning
  at 0x4d6050, absent from the Dreamcast roster. The two source rows therefore
  remain `DC_ONLY`; no guessed source identity is assigned to that distinct
  retail-only work.

- **2026-08-11 — `quickherowindow.obj` advanced to 2/3 exact, with its
  2248-byte constructor reconstructed at 86.84469%.** The sole
  `heroqvbk.pcx` reference and ordered destructor block locate the unit at
  0x52ead0..0x52f43a. Retail and Dreamcast together prove the complete recipe:
  four primary-stat positions at 0x640688, seven army positions at 0x682378,
  portrait/name/mana/morale/luck widgets, the `disguiseLevel` dword at hero
  +0x10e, strongest-stack substitution for disguise levels 0..2, and the
  strongest creature of the owner's alignment for level 3. The preserved
  retail slot-ordinal comparison is a source-level wart, not a guessed fix.
  Both destructors remain byte-exact. The constructor's residual is VC6
  optimizer state around vector reserve, temporary-string cleanup, and the
  two disguise scans; structural stream spellings raised it from 75.28% to
  86.84%. Dreamcast's `QuickWindowWait` is identical to the quick-creature/
  quick-town wrappers and retail ICF-folds all three onto 0x530d30, owned by
  the later quicktownwindow span; no duplicate claim is made here.

- **2026-08-11 — `fly.obj` located at 0x4b46c0..0x4b5011; both movement
  wrappers reconstructed.** The retail `army::simple_move` body calls
  0x4b46c0 for flight and then 0x4b49c0, while its teleport arm calls
  0x4b4e90. Those entries head a five-function sequence whose sizes and
  mutual calls agree in order with Dreamcast `ValidFlight`, `FlyTo`, `Fly`,
  `TeleportTo`, and `Teleport`. The two private Dreamcast
  `find_flyer_attack_cell` overloads have no separate retail entries: their
  only DC caller is `ValidFlight`, whose 761-byte retail body contains the
  work inline. This supersedes only the old statement that the real block was
  unlocated; the withdrawn 0x4b4420 claim remains withdrawn because that
  address is still the fourth terrain initializer. `FlyTo` and `TeleportTo`
  are admitted from retail control flow at 94.00% each; all instructions and
  branches align, with only a symmetric destination/old-facing EBX/EDI
  allocation swap plus pre-existing unresolved relocation-label authority.
  `why-reg` classifies the swap as non-source-nameable C2 handle state. The
  `Teleport` is additionally byte-exact (258/258) from the retail body:
  facing and wide-stack destination adjustment, quick-combat animation gates,
  grid replacement, and spell cancellation are all byte-derived.
  `ValidFlight` and `Fly` remain located carcasses.

- **2026-08-11 — `genericresource.obj` is retail-dropped.** Dreamcast's
  `TGenericResource` constructor is called only by
  `ResourceManager::GetResource`; neither that getter nor a generic-resource
  vtable survives in retail. The complete post-`gametypewindow` carve consists
  of excluded cinit rows through cinit0429 and then `global.obj`'s
  `TGzInflateBuf` family, while the independently recovered
  `resource::resource` caller graph contains no generic wrapper. Constructor,
  destructor, and deleting-destructor rows therefore remain `DC_ONLY` with no
  invented retail claims.

- **2026-08-11 — `TQuickCreatureWindow` constructor reconstructed to
  88.1211%.** Retail bytes prove the complete widget recipe: a 256x256
  dialog, three reserved widget slots, `TwCrPort.def` portrait, singular/
  plural or army-size count text, and the four disposition texts at central
  text-record offsets +0x3d0..+0x3dc. All arguments, coordinates, IDs,
  literals, data fields, and branches outside the reserve implementation are
  represented. The remaining dominant delta is VC6 optimizer state: retail
  calls vector `_Ucopy` and the empty range destroy from its inlined reserve,
  while this compile expands the copy loop and consequently chooses different
  EBX/EDI roles. Depth limits, a reserve adapter, and named-local variants did
  not improve it. A later smallest-lane re-audit extended the optimizer-state
  check to every unused-type population from zero through eight; all nine
  builds were byte-identical at 88.1211%, ruling out that proven lever here.
  The prior two exact destructors and ICF ownership decision remain unchanged.

- **2026-08-11 — `textButton::Draw` closed byte-exact.** Retail keeps the
  widget-status snapshot in CX while EAX carries the shared parent-window
  pointer. Naming both source values reproduces that allocation, removes the
  prior extra register copy, and matches all 130 bytes. `button` advances to
  14/16 exact; its established constructor and `button::Main` residuals remain
  documented separately.

- **2026-08-11 — `playvideo.obj`'s sole source row is Dreamcast-port-only.**
  SH4 bytes prove that `playVideoDll` loads `playsfddll.dll`, resolves
  `PlayVideo`, invokes it, and releases the module. The retail image contains
  neither literal and its authoritative import table has `LoadLibraryA` and
  `GetProcAddress` but no `FreeLibrary`, excluding the wrapper's semantics from
  the x86 build. The row therefore remains `DC_ONLY`; no retail address or
  synthetic match claim is admitted.

- **2026-08-11 — `adventureoptionswindow` admitted from retail bytes.** The
  `advopts.pcx` literal and `advManager::DoAdventureOptions` stack construction
  locate the 1194-byte constructor at 0x4051d0; it remains a carcass. The
  derived destructor and scalar deleting destructor are byte-exact. The
  508-byte handler reaches 99.94% after reproducing retail's early shared
  consume-return block and separate EBX-restoring hover epilogue. Its only
  remaining instruction delta is the register used to carry `mouseX` into
  `findWidget` (candidate ESI, retail ECX); direct arguments, declaration-order
  variants, and a symbol-order perturbation did not alter that allocator choice.
  Dreamcast supplied the derived member name and source signature, while the
  retail vtable, allocation size, globals, calls, and bytes proved the x86
  layout and implementation. The adjacent 0x405680 slot-3 forwarder is shared
  by 42 vtables and is deliberately left with its header-inline ownership.

  **Superseded 2026-08-28:** the mouseX register was not an allocator wall.
  Dreamcast's shared exit-state tail, direct hover statement, const helper
  signatures, and distinct options-help table close the handler at 100.0%.

- **2026-08-11 — `campaignmap` CLOSED 2/2 exact (functions-only), and
  volatile compiler-function normalization completed.** The sole source
  function was independently located at 0x45dee0 from the retail
  `camptext.txt` literal, its sole `InitMainClasses` caller, the 21-record
  campaign walk, and the Dreamcast roster position. Its full 784-byte body
  now matches exactly. Retail bytes prove a 0x10 `TCampaignMapTraits` stride
  and a Complete-only 0x6c `TRegionTraits` stride: name/X/Y followed by three
  eight-player image-name arrays; the Dreamcast 0x18 layout supplies member
  names but is not reused as an x86 layout. The adjacent 22-byte `$E3`
  static destructor is also exact. To pair it without admitting its volatile
  ordinal, `labels` now emits `build/gen/compgen_claims.tsv`, normalization
  treats that manifest as a stamped input, and the semantic binder recognizes
  an /Ob2-inlined static destructor only when the `$E<n>` relocation graph
  contains both its function-local owner and operator delete. Stamp schema 5
  invalidates pre-contract normalized objects. The nine remaining Dreamcast
  roster rows are header/template emissions with no distinct retail bodies.

- **2026-08-11 — `gametypewindow`, `quickinfowindow`, `levelupwindow`, and
  `u2dvers` advanced from retail bytes.** `gametypewindow` has four exact
  functions (deleting destructor, destructor, `DoModal`, and the 544-byte
  handler); its constructor remains a located carcass. The handler's exact
  jump table required the source-level five-entry help-index selection, and
  its modifier-key branch proved a shared update tail. `quickinfowindow` has
  both destructors exact and its five-argument constructor located; the
  13-byte `QuickWindowWait` body is ICF-owned by the later quick-view unit and
  is deliberately not double-claimed. `levelupwindow` has both destructors
  exact; its handler is 73.18% with the first 138 bytes exact and a documented
  stale-CL cross-jump/tail-duplication residual, while the constructor remains
  a carcass. The older `u2dvers BLOCKED` inventory entry is superseded: VC6's
  shipped Dinkumware string surface removed the supposed STLport blocker and
  all three functions are now byte-exact.

- **2026-08-11 — `font::~font` closed exactly with the retail palette
  ownership model.** The retail object stores `TPalette16` inline; replacing
  the provisional pointer with raw aligned storage plus the explicit
  qualified palette destructor reproduces VC6's EH-state transitions and
  raises the destructor from 96.63% to 100%. This is a layout/codegen
  correction from retail bytes, not an imported class body. The existing
  `DrawBoundedString` (98.79%), `DrawCharacter` (78.26%), and `LineLength`
  (92.68%) plateau notes remain in force.

- **2026-08-11 — `subwindow` and `resourcedisplay` admitted from retail
  bytes.** `TSubWindow`'s complete 0x34-byte retail layout is now live: its
  apparent byte at +0x14 is VC6's vector allocator subobject, not a gameplay
  field. The two constructors bracketed immediately before the old unit span
  and the canonical deleting destructor between them were promoted on body
  evidence; `RemoveWidget` remains Dreamcast-only because no retail row exists
  in its bracket. All nine retail functions match exactly. The proven base
  unlocks `TResourceDisplay`'s 0x78-byte layout, its seven resource text/border
  pairs, background and status widgets, the 0..6 resource-order table at
  0x641008, and the three central-text-record fields at +0xfc/+0x100/+0x104.
  Its deleting destructor, destructor, `Update`, and `Clear` are byte-exact.
  The 673-byte constructor reaches 99.39%: a real `isSmall(is_small)` member
  initializer reproduces retail's member-before-derived-vptr store order and
  raises the prior 99.30% result. Control flow, calls, constants, stack homes
  and instruction lengths agree; the remaining 52 register-visible slots are
  one whole-body EBX/EDI transposition (`this` versus `textX`). The VC6 v2
  solver classifies that pair as a C1 symbol-handle-state cap with no
  source-nameable mutation. A later smallest-lane sweep of zero through eight
  additional user-defined types was byte-flat for the constructor and kept
  all four exact neighbors exact, ruling out the known type-population lever.
  Retail bytes and Dreamcast signatures/layout names were the only
  implementation evidence.

- **2026-08-11 — two `font` register plateaus advanced under the VC6
  solver.** `font::DrawBoundedString` rises 96.8123% -> 98.7864% by
  making the bottom-justification `total` volatile; this aligns the
  scratch-register family through the remaining body, leaving only the
  three forced stack-memory instructions and a cosmetic flat callee name.
  `font::LineLength` rises 91.7423% -> 92.6804% by making its backtrack
  boundary `lineStart` volatile. Both functions now have exact branch
  sequences. Fresh guided solver passes found no independent local mutation
  that reduces the remaining register-visible distances (9 and 17 slots).
  Retail bytes were the only implementation evidence used.

- **2026-08-11 — `army::ValidPath` admitted byte-exact (168 bytes).**
  Retail's path query preserves the requested destination separately, widens
  `FindCombatPath`'s byte return to an integer, and stores that result into the
  otherwise-dead `destIndex` parameter slot before branching. A volatile view
  of that slot makes VC6 retain the exact `and eax,0xff; mov [ebp+8],eax; jne`
  sequence while EDI carries the original destination into `pathTarget`.
  This closes the previously documented three-instruction residual and raises
  `path` to 7/8 exact. Retail bytes and the existing Dreamcast signature were
  the only implementation evidence used.

- **2026-08-11 — `type_AI_spellcaster::consider_chain_lightning` admitted
  byte-exact (209 bytes).** The retail body walks every live stack on
  `1 - side`, rejects creature-id bit 21, asks `SpellCastWorks` about spell
  19, and keeps the highest `get_chain_lightning_value` result together with
  that stack's grid index and the choice-valid byte. The natural short-circuit
  loop reproduced retail's frame, register allocation, calls, branches, and
  relocations on the first compiled reconstruction. This advances
  `ai_tactical` to 54/86 exact. Retail bytes established the body; Dreamcast
  CodeView supplied the function and callee signatures.

- **2026-08-11 — `combatManager::ViewArmy` admitted byte-exact (292
  bytes).** The retail body derives the popup origin from the selected
  combat cell, clamps the 298x325 window inside 800x600, constructs a
  `TViewArmyWindow`, dispatches quick-view versus modal behavior, handles the
  modal command result, and virtually deletes the popup. The natural source
  reproduces all instructions, ten branches, the allocation failure arm, and
  the one-state VC6 EH frame. Its direct constructor edge plus the unique
  `CrStkPU.pcx` literal also locates the first retail `TViewArmyWindow`
  constructor at 0x5f3360 (1973 bytes); that body remains unreconstructed.
  Retail bytes established the implementation and Dreamcast CodeView supplied
  the two function signatures.

- **2026-08-10 — the `vc6` compiler-model area landed.** A new
  role package `scripts/homm3/vc6/` reverse-engineers the pinned toolchain
  itself to model the codegen decisions matching plateaus on, and ships solvers
  that diagnose a residual instead of blind spelling sweeps. Binary-only RE (no
  leaked MSVC sources); the RE subjects are hash-gated in `vc6/_toolchain.py`.
  Delivered: the CL option-spec decoder (`argv`, `evidence/vc6/cl-option-spec.tsv`,
  proving `/Ob2` front-end-only and `/d2` C2-verbatim); a byte-inert C2 pass-through
  shim (`_InvokeCompilerPass(argc,argv,fLastTU)` ABI) with an inertness gate + negative
  control; the behavior catalog (`docs/vc6/behavior-catalog.md`) + 23 real-compiler
  probes + `oracle`; the **/Ob2 inliner fully RE'd and validated 9/9**
  (`docs/vc6/inliner.md`: budget = clamp(2·caller_cb, 1000, 35000), sequential,
  nested budget ÷ sites-remaining) with the STL under-inline verdict (budget
  starvation, "finish the caller"); the **register allocator's preference order**
  (`docs/vc6/regalloc.md`: first-fit EAX ECX EDX ESI EDI EBX EBP in pseudo-creation
  order); the **IL tap** settling the include-set (C1) wall as C1XX symbol-handle
  renumbering (`docs/vc6/il-format.md`); the **handle-order model**
  (`docs/vc6/handle-order.md`, 32/32 + 4/4 blind) splitting register/include-set
  residuals into source-movable vs C1-capped; the optimization-scope writeup
  (`docs/vc6/optimization-scope.md`); the Ghidra C2 atlas
  (`atlas`, `evidence/vc6/c2-{tu-map,globals}.tsv`, 48/48 TUs anchored 115/115
  byte-corroborated, project under gitignored `build/re/vc6/`); the solvers
  `why-reg` (v1 sweep + v2 model), `why-branch`, `predict-inline` (+`--gap`),
  `diagnose` (inline→control-flow→register routing), and the `check` census
  (behavioral gate with negative control + informational consistency). The
  `wall-identifier` skill teaches the loop. **Track R admission:** RTM
  `C2.DLL` 12.00.8168 (sha256 `45187b0b6288240f73272a7c61e6329c50048a76e57db3bb87b6f0229e09e27d`,
  737,329 B, sourced from archive.org, staged OUTSIDE the repo at
  `../orig/vc6-rtm/`) is admitted as a hash-pinned A/B-only input; the `ab`
  harness proved 0/18 walls are C2-generation artifacts, retiring the
  stale-generation hypothesis. Generated tables live in `evidence/vc6/`
  (regenerable); no game or toolchain bytes entered the repository.

- **2026-08-09 — wall-identifier cross-project field note approved.** The
  Gruntz `TmDeflectStep` plateau was used as a read-only portability test of
  the wall taxonomy and register model documented under `docs/vc6/`. The
  HOMM3 command itself was not run against Gruntz because its unit manifest,
  profiles and object paths are project-specific; the classification was
  reproduced with Gruntz's own objdump/sema and Cartesian-variant tools. The
  exercise showed that call-multiset classification must precede a register
  hypothesis, and that a clean rebuild must precede the classification: a
  batch trial's stale raw object first suggested an inline-boundary mismatch,
  while the rebuilt candidate proved every helper-call count exact and routed
  the residue back to register/scheduler analysis. No source or implementation
  material was copied between projects.

- **2026-08-09 — `SavedGameHeader::SavedGameHeader` reconstructed to
  96.4326% (593 bytes).** Retail proves the flattened construction order for
  the map-header vector, victory and loss records, eight 0x44-byte player
  slots, 0x10-byte map, two strings, 156-bit availability set, and setup
  options. It also proves the condition defaults, player-slot defaults, setup
  loop and defaults, `H3SVG` identifier, and version 42. The canonical layouts
  now include the 0x24-byte loss record, player-slot vector at +0x34, and the
  destructor-compatible ordinal map records; opaque fields remain ordinal.
  All four blocks, the sole branch, and the return agree. The residual is a
  VC6 inline-boundary choice: retail calls the public two-argument Dinkumware
  map constructor, while this compile inlines that wrapper and calls its
  three-argument tree constructor, shifting two bytes and the associated EH
  state schedule. Equivalent constructor spellings and scoped inline pragmas
  did not change that choice. Dreamcast CodeView supplied original aggregate
  and field names only; retail bytes supplied every x86 offset, default, and
  call target admitted here. The approved read-only `decomp-attempt-1` survey
  found only stubs for this unit, and no material from it was admitted.

- **2026-08-09 — `hero::can_summon_boat` reconstructed to 85.7031%
  (350 bytes).** Retail requires spell-zero availability, computes Summon
  Boat mastery first without terrain and then at the hero cell's magic
  terrain, clamps its mastery-indexed mana cost to one, and rejects heroes
  who cannot pay. A reachable unoccupied boat succeeds immediately; otherwise
  Advanced mastery is required and the global boat pool must have a free id.
  The all-minus-one packed point bypasses the cell lookup. This admits spell
  id zero as `SPELL_SUMMON_BOAT` and the cell magic-terrain member used by the
  already reconstructed spell-school reducer. All sixteen blocks, eight
  branches, and five returns agree; the residual is register allocation around
  the packed-point comparison. No external implementation body was used.

- **2026-08-09 — `hero::can_land` admitted byte-exact (231 bytes).**
  Retail packs the hero's current point and directly indexes the canonical
  38-byte world-map cell array. The cell's water terrain must agree with the
  hero's boat flag and cell flag 0x40 must permit passage. A trigger cell is
  additionally rejected when byte zero of its sixteen-byte adventure-object
  traits row is set; retail proves that 0x660428 stores the row-table pointer,
  not the table itself. All eight blocks, four branches, and four returns are
  exact. No external implementation body was used.

- **2026-08-09 — `hero::IsMobile` admitted byte-exact (191 bytes).**
  Retail packs the hero's current point, resolves its map cell, and compares
  remaining movement at +0x4d with `MinimumTerrainCost`. Pathfinding comes
  from signed secondary-skill slot zero and the final cost flag is exactly the
  presence of a Nomad in the hero's army. While aboard a boat, flight and
  water-walking are both forced to -1; otherwise their +0x112/+0x116 mastery
  values pass through. This names `MinimumTerrainCost`'s already-proven final
  argument and matches the shared retail call tail exactly. No external
  implementation body was used.

- **2026-08-09 — `hero::IsInIdentifyRange` reconstructed to 98.6813%
  (284 bytes).** Retail proves that the inlined +0x129/Rogue mastery getter
  selects the Visions mastery-bonus row, which is multiplied by clamped spell
  power and floored at three cells. A candidate must share the hero's packed
  map level and lie strictly inside the squared Euclidean radius. This also
  admits spell id 2 as `SPELL_VISIONS`; the 0x144 traits displacement proves
  the value independently of the Dreamcast spelling. All thirteen blocks and
  seven branches agree. The residual is one independent load moving across
  the adjacent packed-point read. No external implementation body was used.

- **2026-08-09 — `game::GetRandomSpell` admitted byte-exact (398 bytes).**
  Retail proves that the nominal Dreamcast integer argument is physically a
  five-level bitset passed by value: all three scans inline its bounds-checked
  test, and the recursive retry pushes the same one-word value. The member
  counts the 70 ordinary spells whose level is selected, whose school mask is
  nonzero, and whose game +0x04 draw byte is clear; it chooses one uniformly,
  marks it drawn, and returns it. When exhausted, it clears reusable entries
  not blocked by the parallel +0x4a scenario mask and retries, otherwise
  returning -1. No external implementation body was used.

- **2026-08-09 — `game::calculate_production` reconstructed to
  88.1659% (1,395 bytes).** Retail proves the eight-player production
  reset, mine and town income, silo and faction-building effects, the six
  resource artifacts and Cornucopia, resource-specialist heroes, Crystal
  Dragon income, difficulty-based computer bonuses, and handicap scaling.
  The source reproduces 40 of 42 retail branch edges and raises linked fuzzy
  coverage from 52.52% to 52.85%. Additional declarations remain visible only
  to `game.obj`, preserving all unrelated exact rows. Dreamcast CodeView
  supplied names, types, and artifact identities only; no external
  implementation body was used.

- **2026-08-09 — `game::Save` opens at 27.9354% with its retail prefix
  through all eight player records.** Retail constructs and resets a
  0x5a4-byte save header, calls the now byte-exact
  `SavedGameHeader::Save`, then writes the versioned state bands, rumours,
  event records, world map, four object pools,
  generators, obelisk state, and player roster. The remainder of the
  2,725-byte caller is explicitly left unreconstructed. Its four additional
  declarations are visible only while compiling `game.obj`, preserving every
  unrelated exact row despite VC6's member-population sensitivity. Linked
  fuzzy coverage reaches 52.52% and executable coverage rises from 9.95% to
  9.99%; the status writer migrates the legacy label by retail RVA. Retail call
  targets, write widths, and receiver offsets supplied the implementation
  evidence; no external implementation body was used.

- **2026-08-09 — `SavedGameHeader::Save` replaces a false view and is
  admitted byte-exact (378 bytes).** The previous `game::SaveBlackMarkets`
  claim rested only on cross-build source order. Retail instead proves that
  every access belongs to the 0x5a4-byte save header: it writes the `H3SVG`
  id, version fields, a 32-byte compatibility band, the embedded map header
  and setup options, an optional campaign, the fixed-width filename, and the
  trailing difficulty and player-state bands. The saver narrows the canonical
  four-byte difficulty field to its serialized short; the adjacent retail
  constructor and loader independently prove that in-memory width. Disjoint
  scalar-local scopes are codegen-significant because VC6 reuses the dead
  parameter home for retail's exact stack schedule. The compatibility buffer
  remains deliberately uninitialized, as in retail, and only its short write
  is checked. Dreamcast CodeView corroborates the `SavedGameHeader::Save`,
  `NewSMapHeader::Save`, and `SGameSetupOptions::save` identities; retail
  layout and call targets prove this x86 body, with the campaign callee named
  from its embedded receiver. `decomp-attempt-1` was checked read-only and
  contains only stubs for both competing names, so no external implementation
  body was used.

- **2026-08-09 — `town::initialize_spells` reconstructed to 86.7424%
  (810 bytes).** Retail proves the setup argument and its two 70-bit spell
  masks, the game-wide disabled-spell band, five weighted faction spell rows,
  fixed-spell precedence, and the active Mage Guild count calculation including
  Tower's Library bonus. The retail body also absorbs `set_spells_available`.
  A depth-zero adapter preserves VC6's out-of-line two-argument bitset setter
  while leaving its predicate tests inline; the remaining mismatch is code
  shape in the spell-selection control flow. Dreamcast CodeView supplied names
  and types only; no external implementation body was used.

- **2026-08-09 — `town::SwapHeroes` admitted byte-exact (428 bytes).**
  Retail proves the complete exchange: resolve both resident heroes, swap the
  town ids, remove the former visitor from the acting player's signed-count
  hero roster, restore its obscured cell, broadcast the 0x426 hide message,
  clear the current-hero view latches when locally owned, and place the former
  garrison hero on the town tile. The 24-byte hide-message layout and its store
  order are local to town.obj; the unnamed 0x49c720 game member remains an
  ordinal declaration. `std::swap` is required for retail's reference-based
  exchange schedule. Dreamcast CodeView supplied names and types only; no
  external implementation body was used.

- **2026-08-09 — `town::GiveSpells` reconstructed to 95.0327% (467
  bytes).** Retail proves the five six-spell Mage Guild rows at town +0x44,
  their signed counts at +0xbc, and the existing 70-bit spell veto at +0xd4.
  The member visits a forced hero once or the visiting and garrison heroes,
  requires a spellbook and active Mage Guild, grants ordinary rows through
  Wisdom + 2, and gives an Aurora-Borealis Conflux every eligible non-Titan
  spell. All 37 retail blocks and both granting paths are present; the
  residual is VC6 folding retail's dead positive-count preheader. No external
  implementation material was used.

- **2026-08-09 — `game::Load` reaches 50.3078% with the complete roster
  band.** Retail reads a byte town count, resizes the canonical 360-byte town
  vector through a default temporary, loads every town, then loads 128 heroes
  before save version 25 and all 156 thereafter. The adjacent compatibility
  state is required as one unit: pre-version-31 saves consume an eight-byte
  legacy record, hero availability restores 128 or 156 bytes with 0x40 for
  the older 28-entry tail, and newer saves rebuild 156 eight-player
  eligibility bitsets from one byte each. Those eight legacy bytes plus the
  four-byte bitset temporary close the previously observed 12-byte frame gap.
  Retail also keeps the already byte-exact `generator` constructor out of
  line for its 92-byte resize temporary; a pragma scoped to that definition
  restores the exact 0x7a8-byte frame and saved-header slots without changing
  the constructor's 100% match. The function rises from 48.8668% to 50.3078%,
  all 986 cur/max/history ratchets remain clean, and no external implementation
  body was used.

- **2026-08-09 — `game::GetNewHeroId` reconstructed to 98.9815%.** Retail
  callers and the callee's `ret 0x10` prove the Complete-build extension to
  four arguments: the first selects a player setup slot and eligibility bit,
  while the fourth requests a preferred hero class. The body counts the 156
  unused, player-eligible heroes by class, applies the signed per-alignment
  class weights, suppresses empty and excluded classes, enforces the Complete
  build's Conflux-class gate, optionally restricts selection to the player's
  alignment, then performs the two one-based random selections. This admits
  the canonical eighteen-value `THeroClass` domain and the +0x00 town type /
  +0x33 ten-byte selection-weight slice of `THeroClassTraits`. Dreamcast
  CodeView corroborates the original class ladder and the `hero_class`,
  `total_count`, `choice`, `counts`, `hero_id`, `weights`, and `aligned_count`
  locals; retail alone proves the two added Conflux classes, 156-hero extent,
  fourth argument, and all control flow. Every branch agrees, and all
  instruction differences are confined to one known
  symmetric-register scheduling choice at the `gpGame->f_1f698 >= 2` gate:
  retail uses eax/ecx where this SP3 compile uses ecx/eax, making the compiled
  body one byte longer; equivalent condition nesting, operand order, named
  pointer/value/flag locals, and the attested long/enum local types do not
  move it. `decomp-attempt-1` was checked read-only and contains only a stub;
  no external implementation body was used.

- **2026-08-09 — `game::Load` extends from 47.6526% to 48.8668%.** Save
  version 41 introduces one unchecked byte read immediately after the path
  search array closes; retail sign-extends that byte into the global at
  0x69950c and stores -1 for older saves. The address was already independently
  attested by `advManager`, so its declaration moves to the canonical owner
  header rather than creating another translation-unit-local extern. The
  town/hero roster candidate was rejected because either partial form lowered
  the function score; it remains out until the adjacent retail stack schedule
  can be reconstructed as one additive unit. All 986 cur/max/history ratchets
  remain clean, and no external implementation body was used.

- **2026-08-09 — `game::Load` reaches 47.6526% with the retail save-header
  frame.** The 0x5a4-byte `SavedGameHeader` is fixed by retail stack offsets,
  constructor order, copy widths, and member destinations: it contains the
  0x304-byte `NewSMapHeader`, 0x1cc-byte `SGameSetupOptions`, 0x7c-byte
  `SCampaign`, Dinkumware filename string, and trailing player-state bands.
  Those same three aggregates now occupy their canonical `game` offsets
  (+0x1f86c, +0x1f6a0, and +0x1f458 respectively), replacing flat padding
  and aliases for every translation unit. Loading and restoring this header
  raises the function from 26.8395% to 47.6526%. The emitted
  `SavedGameHeader` constructor is independently admitted at 27.1011%, and
  the cur/max/history ratchet remains clean across all 986 linked functions.
  Candidate cross-build data supplied names only; retail instructions prove
  every admitted size and offset, and no external implementation body was
  used.

- **2026-08-09 — `game::GetStartingHeroId` admitted byte-exact.** Retail
  proves the complete algorithm: map each of the nine town types to its two
  hero classes, collect unused heroes whose per-player eligibility bit is set,
  fall back to all eligible unused heroes when the class-filtered set is empty,
  and select the one-based `Random(1, count)` result. The third parameter is
  unused in this retail body. The +0x4dfb4 band is consequently canonicalized
  from an integer placeholder to 156 `std::bitset<8>` records; retail's two
  range-check calls and direct bit tests match VC6 exactly. Dreamcast CodeView
  supplied the function/local names and corroborated the 156-entry candidate
  array. `decomp-attempt-1` was checked read-only and contains only a stub, so
  no external implementation body was used.

- **2026-08-09 — `game::Load` extends from 26.3228% to 26.8395%, and its
  state is canonical.** Retail's call sequence proves eight consecutive
  `playerData::load(infile, saveVersion)` calls after the obelisk pool; the
  loop is additive on its own. Every byte-proven load field now belongs to the single canonical
  `game` layout compiled by all translation units, while the existing member
  offsets and match totals remain stable. No external implementation body was
  used, and the generated match baseline is updated only by the full build.

- **2026-08-09 — `game::CreateBoat` reconstructed to 99.9726%.** Retail
  proves the inlined 64-entry boat-pool allocation, the local-only 0x421
  map-change message and show-boat record, and every initialized boat field.
  Dreamcast CodeView supplies the `CMCBuildBoat` and inline
  `boat::obscure_cell` identities; the latter is also required for retail's
  byte-to-long zero extension. VC6's retail store schedule requires the
  source order `type`, `x`, `y`, `z`. All instructions and branches agree;
  the remaining score delta consists only of four target-side working-label
  spellings for already-addressed relocations. The temporary shipyard
  declaration gate was removed completely rather than retained as
  architecture. `decomp-attempt-1` was checked read-only and
  contained no body or additional evidence worth admitting; no external
  implementation material was used. Making those declarations unconditional
  changes VC6's transitive type population: the already-documented unstable
  `initialize_game_data` row remeasures from 100% to 94.0741%, and
  `recruitUnit::Update` from 90.8376% to 90.8325%; both historical peaks remain
  recorded rather than being presented as semantic regressions.

- **2026-08-26 — the new-map string reader and header-normalization helper
  are byte-exact.** Retail 0x4c6010 uses the map format's signed dword string
  length (rather than the saved-game helper's short), accepts only positive
  lengths below 0xffff, and otherwise erases the destination string. The
  211-byte helper at 0x4c4e30 maps the header's available-hero bits into the
  live availability row, applies explicit player masks, and reserves the
  artifact named by an artifact-victory condition. Its comparison/copy shape
  independently proves that `type_map_hero_info` ends in a `bitset<8>`: a
  default bitset followed by `set()` becomes retail's direct 0xff local, and
  the mapped value copies as one dword. Both functions match every retail
  instruction and relocation; no external implementation body was used.

- **2026-08-09 — `game::Load` extends from 23.1861% to 26.3228%.** Retail
  fixes the prefix order after header restoration: clear recorded events,
  publish the loaded square map extent, close the path search array, restore
  the versioned 0x90/0x81 scenario bands and the 28-byte post-version-29
  band, then load rumours, the signed-count 28-byte event vector, the world
  map and the sign/mine/generator/garrison/boat pools before the 48 obelisk
  bytes. Generator count is a signed short; each 92-byte record must report
  success. A function-scope Dinkumware string is independently visible in
  the target prologue and cleanup: adding it turns every source short-read
  into retail's shared failure exit and makes the prefix additive. The
  save-header and player/town/hero roster bands remain explicit follow-up
  work. No external implementation body was used, and rejected roster and
  map-extra experiments were not retained.

- **2026-08-09 — `game::ExperienceValueOfStack` admitted byte-exact.**
  Retail's +0x4c creature-traits load proves that the seven stack products use
  `hitPoints`, not the +0x40 AI valuation field; a non-null hero contributes
  the remaining fixed 500 experience. This agrees with every instruction and
  branch in the 88-byte body and with the game's battle-experience rule. No
  external implementation material was used.

- **2026-08-09 — the first `game::Load` slice reaches 23.1861% of the
  3,778-byte routine.** Retail proves the consecutive scenario-state reads at
  game +0x1f4d4, +0x1f634, +0x1f63e..+0x1f69c and the 32/8/6/3-byte global
  information bands at +0x4e344..+0x4e374. It also proves nineteen
  Dinkumware `vector<type_point>` loads: two version-dependent arrays at
  +0x4e67c/+0x4e6fc use three entries before save version 32 and eight after
  it, followed by three standalone vectors at +0x4e77c/+0x4e78c/+0x4e79c.
  A short two-byte count skips that vector, while a present count resizes it
  and reads four bytes per point without checking the data read. The
  canonical `game` layout exposes these byte-proven offsets to every
  compiland. The save-header/map/pool/roster prefix remains explicit
  follow-up work, with a temporary leading version read preserving its gates.
  No external implementation body was used; the full build remains the sole
  baseline writer.

- **2026-08-09 — `game::get_new_boat_id` admitted byte-exact.** The
  214-byte member scans the retail-proven 40-byte boat pool for the first
  clear `allocated` byte at +0x18, returns that reusable index, or appends a
  default boat while the pool contains fewer than 64 entries and returns the
  new tail index. An unsigned loop index reproduces retail's inlined
  Dinkumware `vector::size()` null guard and unsigned comparison; the signed
  spelling scored 83.98% and introduced a non-retail empty-vector arm. Every
  instruction and relocation agrees. Dreamcast CodeView supplies the method
  and local names only. The abandoned first attempt contains the already
  admitted identity/boundary mapping but no implementation body used here;
  no external implementation material was used.

- **2026-08-09 — the object-pool serialization and claim views are integrated
  without losing their admitted matches.** The mine tail exposes serialization
  ordinals and packed-coordinate names as aliases of the same three retail
  bytes, while the garrison loader uses the newer +0x3d..+0x3f coordinate names
  and retains +0x3c as its serialized flag. The now-byte-exact free `loadString`
  owns 0x4bb990; a scoped `auto_inline(off)` on that definition preserves the
  out-of-line calls required for byte-exact `LoadSignPool` and `LoadRumours`.
  A clean delink rebuild reports 650/985 exact functions, 50.90% linked fuzzy
  coverage and no ratchet, claim, single-view or cleanliness regressions. No
  external implementation material was used.

- **2026-08-09 — `combatManager::move_toward` admitted byte-exact.** The
  772-byte path walker already matched all 65 control-flow blocks; its last
  mismatch was the second inlined minimum assigning two equivalent stack
  homes in reverse. A source-private variant takes the enemy value by value,
  the current best by reference, and explicitly copies that second operand.
  VC6 consequently places the enemy in the fresh -0x1c local and recycles
  `best_danger`'s +0xc argument slot for the copy, reproducing the final eight
  retail instructions without changing behavior. No external implementation
  was used.

- **2026-08-09 — `town::get_growth_rate` admitted byte-exact; `town`
  gains its 29th exact row.** The 614-byte caller gates base and upgraded
  dwelling slots, adds creature and fortification growth, applies Legion and
  tier artifacts for owned towns, takes the first active matching horde,
  folds in the generator adjustment, and grants the Grail's signed half-growth.
  A distinct three-arm castle contribution (full, recomputed half after the
  out-of-line `HasBuilding`, or explicit zero) recovers retail's shared average,
  12-byte local frame, dwelling spill, EBX horde index, and all 28 control-flow
  blocks. Every instruction and relocation agrees. No external implementation
  was used.

- **2026-08-09 — the retail-only town artifact-growth helper is reconstructed
  to 99.8523%.** The 600-byte member resolves the town's garrison and visiting
  heroes (including the packed-coordinate map-cell fallback), then applies the
  tier-two through tier-six growth artifacts with bonuses 5, 4, 3, 2, and 1.
  All 36 control-flow blocks and every instruction agree; the sole residue is
  an independent map-size load scheduled one instruction before retail's
  packed-coordinate load. Source declaration order recovered retail's EBX game
  cache, argument-slot hero spill, EDI accumulator, and exact switch layout.
  No external implementation was used.

- **2026-08-09 — `game::LoadSignPool` reconstructed to 87.3457%
  (435 bytes).** Retail proves the 20-byte `Sign` record and the pool base at
  game +0x4e378: a one-byte text gate followed by an STL string at +4. The
  loader reads a signed-byte count, resizes the Dinkumware vector, calls the
  now-exact free `loadString` helper for each record, reads and normalizes the
  saved text flag, and shares retail's -1 short-read exit. The new pool base
  also corrects the adjacent game layout: the +0x4e364 guard-visit band is
  eight bytes, followed by the independently attested cartographer state,
  rather than the provisional 36-byte guard band. The loop body, error flow,
  offsets and record stride agree with retail; the residual is VC6 emitting
  the temporary `Sign` string cleanup through `_Tidy` where retail inlines
  the same refcount/deallocation path. A fully inlined `loadString` attempt
  scored 41.3951%; pinning only that helper out of line and precomputing the
  indexed record produced the retained score. Dreamcast supplies the
  `Sign`/`hasText`/`signText` names only; no external implementation body was
  used, and the full build remains the sole baseline writer.
- **2026-08-09 — the retail `loadString` save-stream helper is byte-exact
  (463 bytes).** The retail call convention disproves the Dreamcast member
  shape: the abstract file arrives in ECX and the destination STL string in
  EDX, so the admitted function is a free `__fastcall` helper. It reads a
  signed 16-bit length, rejects short reads, allocates and zero-fills a
  length-plus-one buffer for positive strings, assigns that buffer into the
  destination and frees it, while nonpositive lengths erase the destination.
  The direct retail reconstruction reproduces every instruction and supplies
  the string reader called by `LoadSignPool`, `LoadRumours`, and map-event
  loaders. No external implementation body was used. The full build added the
  generated baseline row, raised the project from 626 to 627 exact functions,
  and moved executable fuzzy coverage from 9.13% to 9.15%; the baseline was
  never edited by hand.

- **2026-08-09 — `game::ClaimShipyard` reconstructed to 77.4860%
  (543 bytes).** Retail resolves the packed map point, temporarily restores a
  hero obscuring the cell, and reads the signed owner from the low byte of the
  four-byte `ShipyardInfo`. On an ownership change it finds and erases the
  point from the old player's Dinkumware `vector<type_point>`, reveals and
  inserts it for a nonnegative new owner, updates the packed owner, sends the
  28-byte subtype-0x420 map-change message, then re-obscures the hero. The
  complete `ShipyardInfo` bitfield layout, `this_hero`/`cell`/`i`/
  `current_player` locals and message identity come from Dreamcast CodeView;
  retail independently fixes every map offset, vector operation, helper call,
  gate and argument. Game-only header views expose those proven types and the
  existing obscuring-object methods without changing other compilands. The
  remaining flow residual is two null/size guards retained by retail's
  inlined vector erase but folded by the pinned compiler, followed by register
  binding and message-store scheduling differences. The measured unnamed
  old-owner spelling reduced masked register distance but lowered objdiff to
  73.5140%, so it was withdrawn in favor of the 77.4860% candidate. No
  external implementation body was used. Linked fuzzy coverage rises from
  48.13% to 48.24% and executable fuzzy coverage from 9.13% to 9.15%, with all
  626 exact functions retained.

- **2026-08-09 — `advManager::QuickInfo` object dispatch is structurally
  complete at 55.8089%.** The last two explicit retail arms are restored.
  Hero cells resolve the packed id through the inline `game::GetHero`, then
  format the stored name and admitted hero-class description. Empty and
  anchor cells construct a temporary string from either the indexed terrain
  name or `get_special_terrain` object name, append the newly exposed +0x52c
  text only when retail's `is_diggable` predicate succeeds, and copy the
  result before destruction. A scoped zero-depth pin retains retail's
  out-of-line string-construction boundary; measured one-level and fully
  inlined variants scored lower and were withdrawn. These paths raise the
  function from 54.0594% and linked fuzzy coverage from 47.98% to 48.02%,
  preserving all 626 exact functions. Retail fixes the dispatch membership,
  tables, offsets, calls and string lifetime; Dreamcast supplies semantic
  names only. No external implementation body was used, and the baseline
  remains full-build-owned.

- **2026-08-09 — `game::ClaimGenerator` reconstructed to 99.9706%
  (420 bytes).** Retail indexes the 92-byte generator pool, sends the
  28-byte subtype-0x41e claim message, removes the old owner's matching town
  growth bonuses, writes the new signed owner and calls the already-exact
  bonus updater. Nonnegative owners reveal the generator's packed map point
  at radius three, after which the flagged-generator victory condition is
  checked unconditionally. Dreamcast CodeView supplies the message and
  `generator::set_owner` identities; retail proves the message layout, pool
  arithmetic, elemental exception, creature-town alignment lookup, player
  town walk, visibility arguments and victory tail. The retail build has no
  standalone `set_owner` slot, so the recovered helper remains inline and
  ClaimGenerator reuses it. Applying that helper to the already-exact
  `generator::Initialize` changed VC6's caller context and dropped the latter
  to 63.8323%, so that experiment was withdrawn and its proven longhand source
  shape retained. Candidate and retail contain the same 136-instruction
  multiset and branch topology; the only four unpaired slots are the -1 head
  field and subtype stores transposed by the open B16 scheduler. No external
  implementation body was used. Linked fuzzy coverage rises from 47.93% to
   48.09% and executable fuzzy coverage from 9.10% to 9.12%, with all 626 exact
   functions retained.

- **2026-08-09 — `advManager::QuickInfo` status details reach 54.0594%.**
  Pyramid, wagon, warrior-tomb, water-wheel and windmill cases now append the
  retail double-newline separator and select the admitted visited/unvisited
  text from the exact current-player visibility tests and packed weekly-state
  fields. Fountain of Fortune first appends its known-object label, then sums
  retail's four independent hero visit bits before selecting the same status
  suffix. Hill fort and university append their global-info labels through
  the retail single-newline formatter only when the selected cell is known to
  the local player. These eight retail-proven paths raise the function from
  43.7530% and linked fuzzy coverage from 47.67% to 47.93%, retaining all 626
  exact functions. Dreamcast supplies semantic field identities only; retail
  fixes every gate, mask, string and append order. No external implementation
  body was used, and the baseline is updated only by the full build.

- **2026-08-09 — `game::ClaimGarrison` reconstructed to 84.4118%
  (201 bytes).** Retail indexes the 64-byte garrison pool, packs the three
  coordinate bytes at +0x3d..+0x3f, sends a 28-byte claim-garrison map-change
  message, writes the new signed owner and reveals a radius-three area for
  every nonnegative owner. Retail fixes message subtype 0x41f, size 0x1c,
  payload offsets +0x14/+0x18 and the `SendMapChange` target; Dreamcast
  CodeView supplies the `CNetMsg`/`CMapChange`/`CMCClaimGarrison` hierarchy,
  member identities and helper name. The shared 20-byte message head was
  lifted unchanged from the already-admitted gift-message view rather than
  duplicated, following the HoMM2/Gruntz layout-reuse and canonical-helper
  rules. The candidate and retail have the same 68-instruction multiset and
  three-block flow; the remaining ten unpaired slots are five constructor
  stores transposed by VC6's open B16 post-register-allocation scheduler.
  Initializer/body splits and declaration orderings were measured and
  withdrawn when they worsened that residual. `decomp-attempt-1` contains
  only generated stubs and CodeView inventories for these rows, while NH3API
  independently corroborates the layout and subtype but has no constructor
  body; neither supplied implementation material. Linked fuzzy coverage rises
   from 47.67% to 47.71% and executable fuzzy coverage from 9.04% to 9.05%,
   with all 626 exact functions retained.

- **2026-08-09 — `advManager::QuickInfo` extends to 43.7530%.** The
  lighthouse path reads its signed owner from the indexed mine record and
  appends the owner color through the shared quick-info suffix. Mine dispatch
  calls the independently delimited five-parameter retail helper with the
  local player id, newline separator and full-list flag. Mystical gardens and
  obelisks select their visited text from the packed cell flags and admitted
  player/game bitsets. Seer huts and quest guards index their map-owned
  vectors and copy the returned temporary string, preserving the retail
  destructor paths. These six retail-proven cases raise the function from
  37.2317% and linked fuzzy coverage from 47.50% to 47.67%, with all 626 exact
  functions retained. Dreamcast supplies surviving method/type identities;
  retail fixes all indices, gates, arguments and lifetimes. No external body
  was used, and the baseline remains full-build-owned.

- **2026-08-09 — `advManager::QuickInfo` visited-state dispatch reaches
  37.2317%.** The retail case blocks share a 512-byte formatting buffer and
  the `"\n\n%s"` visited/unvisited suffix. Arena, border tent, buoy and
  clover restore that frame and common tail first; twenty-two further cases
  then select the same two admitted text fields from their retail-proven hero
  flags, per-object bitsets or local-player visit flags. The family covers
  dead guys, defense towers, faerie rings, fountains of youth, gardens,
  idols, lean-tos, libraries, magic sites, mercenary camps, mermaids, oases,
  schools, rally flags, sirens, stables, temples, training grounds and
  watering holes. Each mask, field offset, trigger/current-hero gate and
  shared formatter is independently visible in retail; the equivalent
  rollover consumers corroborate only the already-proven domains. This raises
  the function from 15.9777% and linked fuzzy coverage from 46.96% to 47.50%
  without reducing the 626 exact functions. Dreamcast supplies names only;
  no external implementation body was used, and the baseline remains
  full-build-owned.

- **2026-08-09 — `advManager::QuickInfo` extends to 15.9777%.** Retail's
  border-guard arm combines the indexed color and object names through the
  already-attested rollover format. The two creature-generator arms resolve
  the generator through the cell's packed id, select the class-specific name,
  and include its owner only when the signed owner byte is nonnegative, using
  the retail `"%s\n\n%s"` literal. The resource arm indexes the admitted
  resource-name table directly. After trigger-adjuster cleanup, the shared
  tail conditionally appends the selected packed map coordinates through the
  retail format and a bounded local buffer. These retail-proven paths raise
  the function from 12.3003% and linked fuzzy coverage from 46.87% to 46.96%
  while preserving all 626 exact functions. Dreamcast supplies semantic names
  only; no external implementation body was used, and the match baseline is
  updated solely by the full build.

- **2026-08-09 — the first `advManager::QuickInfo` slice reaches 12.3003%.**
  The 9,632-byte retail function now validates and packs the selected map
  point, resolves its cell and trigger through the byte-exact
  `type_cell_adjuster`, distinguishes invalid and shrouded text, and restores
  the default object-name path. Ten retail-confirmed dispatch arms reuse the
  admitted creature-bank, shrine, tree and witch-hut text builders with the
  exact player, separator and full-list arguments. The shared tail measures
  the resulting quick view, clamps it to the retail 600-by-552 bounds, and
  opens the normal dialog. This promotes the placeholder to its authoritative
  mangled symbol and raises linked fuzzy coverage from 46.37% before the
  slice to 46.68%, without reducing the 623 exact functions. Retail bytes
  prove the branches, map indexing, case identities, constants and calls;
  Dreamcast CodeView supplies only function/local names. No external
  implementation body was used, and `match_baseline.tsv` remains generated
  solely by the full build.

- **2026-08-09 — `game::ClaimMine` reconstructed byte-exact
  (203 bytes).** Retail indexes the 64-byte mine pool, constructs a packed map
  point from the three tail coordinate bytes, records only normal actions,
  writes the new signed owner byte, and reveals a radius-three area for every
  nonnegative owner. Non-initialization actions additionally test the embedded
  flagged-mine victory condition and force an end-game check on success. The
  four-value action enum, `record_claim_mine`, `SetVisibility`, victory member
  and `CheckEndGame` identities come from Dreamcast CodeView/xrefs and are each
  independently fixed by retail call targets and argument shapes. Retail also
  proves the mine tail layout and every gate, constant and packed-coordinate
  operation. A game-only header view exposes the already-exact `type_point`
  constructor body so VC6 reproduces retail's inline bitfield writes without
  perturbing its out-of-line owner in advmgr.obj. This applies the
   HoMM2/Gruntz minimal-view and canonical-helper-boundary rules. No external
   implementation body or `decomp-attempt-1` material was used.

- **2026-08-09 — `playerData::Init` reconstructed byte-exact
  (277 bytes).** Retail clears the active hero and town state, empties the
  shipyard vector without releasing its allocation, restores the packed Grail
  guess's three bitfields to -1, resets visit flags, recruits, personality,
  resource-production AI state and network flags, fills all eight hero ids and
  seventy-two town ids with -1, enables placement help, and copies the central
  default computer-player name into the twenty-one-byte buffer. Bit-preserving
  two-byte copies express the odd-aligned point updates without casts and fold
  to retail's two word ORs; the fixed eight-id loop selects retail's
  `lea/count/value/rep stosd` schedule. Retail proves every offset, width,
  fill extent, vector operation, default-name chain and duplicate final flag
  stores. Dreamcast supplies the member identity and its single local only.
   This applies the HoMM2/Gruntz layout-reuse and source-shape rules. No external
   implementation body or `decomp-attempt-1` material was used.

- **2026-08-09 — `advManager::ProcessHover` extends to 74.7787%.**
  Retail reads the acting hero id directly from the current player for the
  map-level gate, preserving the redundant `game::GetHero` sentinel check,
  then constructs a temporary packed `type_point` from the hero's three
  coordinate shorts and inlines the attested point-equality expression. The
  no-current-hero shipyard arm branches to the shared normal-cursor exit on
  team rejection before taking the allied-shipyard exit. Reconstructing these
  source shapes raises the 2,328-byte function from 71.4102% while preserving
  its exact retail extent and all 621 exact functions. Retail instructions
  prove the bitfield masks, branch polarity, lookup repetition and field
  offsets; Dreamcast CodeView supplies only the surviving
  `type_point::operator==` identity. No external implementation body was used,
  and `match_baseline.tsv` remains generated solely by the full build.

- **2026-08-09 — `playerData::NumOfGivenArtifact` reconstructed
  byte-exact (220 bytes).** Retail first walks the player's signed hero count
  and eight-id roster, resolves each id through the inline `game::GetHero`, and
  counts matching ids across all nineteen equipped artifact records. It then
  walks the signed town roster, resolves each town through inline `GetTown`,
  skips negative garrison-hero ids, and repeats the same nineteen-slot scan for
  each garrison hero. Retail proves both loops, both inline accessor null arms,
  the 1,170-byte hero and 360-byte town indexing, the unaligned equipped row at
  +0x12d, its eight-byte stride and nineteen-slot extent. Dreamcast supplies
  the member identity and parameter type only. The implementation reuses the
  admitted layouts and canonical accessors directly, applying the
  HoMM2/Gruntz minimal-view and helper-boundary rules. No external
  implementation body or `decomp-attempt-1` material was used.

- **2026-08-09 — `generator::Initialize` reconstructed byte-exact
  (503 bytes).** Retail clears the four creature and population slots,
  initializes the guard army, selects either one creature from the class-1
  generator table or four from the class-4 table, copies them backward, marks
  the town and owner unassigned, and grows the new roster. If the requested
  owner differs afterward, the inlined transition removes the old player's
  matching-town bonuses and adds the new player's, retaining the Complete-only
  elemental gate on both halves. The two table declarations now carry their
  Dreamcast/retail-symbol `TCreatureType` domains; the additional owning-header
  edge is byte-inert in the existing adventure-manager consumer. Branch-local
  generator-type temporaries reproduce retail's separate signed-byte loads,
  and a call-site `inline_depth(0)` pin preserves the observed out-of-line Grow
  boundary while leaving the inline `GetTown` accessors intact. Retail proves
  all control flow, table addresses, bounds, offsets, sentinels and called
  behavior; Dreamcast supplies the member, local and table identities. This
  applies the HoMM2/Gruntz source-shape, lifetime and helper-boundary rules. No
  external implementation body or `decomp-attempt-1` material was used.

- **2026-08-09 — `generator::load` reconstructed byte-exact
  (315 bytes).** Retail checks exact read sizes for the owner, generator class
  and type, population row, three map coordinates and town id. Between those
  fields it reads four one-byte creature ids into an integer, masks each to
  the serialized byte domain, restores 0xff as `CREATURE_NONE`, and rejects
  only an embedded guard load result of -1. The final town-id comparison is
  retained in an explicit byte local, mirroring the exact save-side result
  shape. Separating the raw read slot from its masked value is
  codegen-significant: it gives VC6 retail's escaped local and argument-home
  loop-counter lifetimes without a spurious masked-value writeback. A small
  bit-preserving union bridge keeps the live `TCreatureType` row honest while
  preserving the zero-cast cleanliness floor. Dreamcast CodeView supplies the
  member identity and typed stream signature; every field, size, sentinel and
  branch is retail-byte-proven. This applies the HoMM2/Gruntz source-shape and
  local-lifetime method. No external implementation body or
  `decomp-attempt-1` material was used.

- **2026-08-09 — `advManager::ProcessHover` extends to 71.4102%.**
  Retail keeps the screen-to-cell quotients live in EDI/EBX and passes those
  `rx`/`ry` map-cell coordinates to `SetRolloverText`; the earlier candidate
  incorrectly passed the original pixel coordinates. Correcting the call both
  restores the behavior and recovers the quotient register lifetimes across a
  large part of the 2,328-byte hover function. The argument identity and
  register flow are retail-byte-proven; no external implementation body was
  used.

- **2026-08-09 — `generator::update_bonus` reconstructed byte-exact
  (184 bytes).** Retail ignores unowned generators and, while the existing
  expansion/version gate is clear, the four base elementals. It takes the
  town alignment from the first generated creature's 116-byte static-traits
  row, rejects the -1 alignment, then walks the owning player's signed town
  roster. Each town whose faction byte matches receives a +1 generator
  bonus for that creature through the already-exact town member. The inline
  `game::GetTown` -1 arm and 360-byte town-vector indexing reproduce retail
  directly. Restoring the generator's four-slot creature row from plain
  integers to its attested `TCreatureType` domain is layout-neutral and
  leaves the exact constructor, save, and growth bodies unchanged.
  Dreamcast CodeView supplies the member identity and creature-row domain;
  all gates, offsets, constants, indexing, and the called behavior are
  retail-byte-proven. No external implementation body was used.
  Whole-linked fuzzy coverage rises from 46.07% to 46.12% and exact linked
  functions from 620 to 621.

- **2026-08-09 — `generator::save` reconstructed byte-exact
  (177 bytes).** Retail serializes the owner byte, generator class and type,
  the low byte of each of four creature ids, the complete eight-byte
  population row, three map-coordinate bytes, the embedded guard army, and
  finally the town id. The retail vtable slot fixes the formerly opaque
  stream parameter as `TAbstractFile*`; every direct write uses its proven
  slot-2 `Write` method, and the guard army uses its already-exact save
  member. Keeping the final `Write(...) == sizeof(town_id)` result in an
  explicit byte local is codegen-significant: VC6 then emits retail's
  `cmp/sete` instead of a four-instruction integer normalization. Dreamcast
  CodeView supplies the member identity and parameter/return widths; the
  serialized offsets, sizes, order and stream operations are all
  retail-byte-proven. No external implementation body was used. Against the
  re-anchored floor, whole-linked fuzzy coverage rises from 46.03% to
  46.07%, executable fuzzy coverage from 8.73% to 8.74%, and exact linked
  functions from 619 to 620.

- **2026-08-09 — creature-bank rollover help extends to 87.2833%.**
  Retail checks cell knowledge before materializing the bank-state dword, then
  keeps the known-cell work in its own branch. Its full-list and compact-list
  result strings occupy sibling scopes and therefore share one `-0x14` VC6
  string home. Reconstructing those lifetimes reduces the frame from 0x24 to
  retail's 0x14 bytes and restores the shared cleanup topology without changing
  either output path. The control flow and lifetime evidence come entirely from
  retail disassembly; no external implementation body was used.

- **2026-08-09 — `advManager::SetRolloverText` extends to 89.7053%.**
  Retail's repeated visited-object handlers materialize the byte returned by
  `game::GetInfoFlag`, widen it through a shared integer temporary, and reuse
  that temporary for the hero-visit predicate before selecting the visited or
  unvisited string. Reconstructing that source shape restores the retail
  byte-store/zero-extend sequence across the shared Buoy/Clover-style handlers
  and the matching spill used by the remaining visit-text arms. The function's
  candidate body grows toward the 8,860-byte retail extent while preserving
  the already exact 216-byte dispatch map. The behavior is derived entirely
  from retail disassembly; no external implementation body was used.

- **2026-08-09 — `hero::HeroFn_004DBE80` reconstructed byte-exact
  (164 bytes).** The retail-only ordinal name remains provisional. Retail
  copies `gCombinationArtifacts[combination].components`, the five-dword
  `std::bitset<144>` at row offset +4, then walks all nineteen equipped
  artifact slots. Every non-empty artifact id clears its bit in the local
  mask, including Dinkumware's native out-of-range check, and the function
  returns the negation of `bitset::any()`—true only when no required
  component remains missing. The member declaration stays confined to
  hero.obj's existing narrow view. The Dreamcast build predates this
  Shadow of Death combination-artifact family and has no corresponding
  row; the identity therefore stays ordinal while all behavior, layout and
  constants are retail-byte-proven. No external implementation body was
  used. Whole-linked fuzzy coverage rises from 45.85% to 45.89% and exact
  linked functions from 618 to 619.

- **2026-08-09 — `hero::HeroFn_004D8FB0` reconstructed byte-exact
  (160 bytes).** The retail-only ordinal name remains provisional. Retail
  first honors the custom-name flag at +0x3d9, returning the unaligned
  pointer at +0x3de or the empty literal at 0x63a608. Otherwise campaign
  mode selects a runtime override except in scenario 20 when the current
  hero's portrait is 156; the ordinary path returns the live +0x23 name,
  substituting the shared 0x6a66d8 table entry only while that name still
  equals `akHeroTraits[id].defaultName` at trait offset +0x40. The newly
  exposed hero fields and member declaration are confined to hero.obj's
  existing narrow view. Dreamcast corroborates `portrait` at +0x34 and the
  static trait name role (at a different cross-build offset); all x86
  offsets, branches, constants and data accesses are retail-byte-proven.
  No external implementation body was used. Against the re-anchored floor,
  whole-linked fuzzy coverage rises from 45.81% to 45.85%, executable fuzzy
  coverage from 8.69% to 8.70%, and exact linked functions from 617 to 618.

- **2026-08-09 — `advManager::SetRolloverText` dispatch extends to
  84.1879%.** Retail's 216-byte object-to-handler map and 59-entry target
  table admit the remaining control-flow structure: `BORDER_GATE` (id 212)
  shares Border Guard's color/name handler; only Nothing, Anchor Point,
  Event and Holy Grail pass through special-terrain detection; the generic
  fallback performs a signed 0..231 name lookup with the shared empty text;
  and the bank/quest, Dead Guy/Defense Tower, Hill Fort/Hero, Lean-To/
  Library/Lighthouse and Wagon/War School/Warrior Tomb blocks follow their
  retail physical order. Wagon and Warrior Tomb retain distinct handlers.
  The resulting candidate reproduces the complete compressed dispatch map
  byte-for-byte. Names come only from admitted enum/structure evidence and
  no external implementation body was used. Whole-linked fuzzy coverage
  rises from 45.48% to 45.68% with all 614 exact linked functions retained.

- **2026-08-09 — `advManager::SetRolloverText` extends to 75.5187%.**
  Retail admits the Quest Guard and Seer string-producing arms. The two
  switch blocks read the first pointers of consecutive VC6 vectors at
  `NewfullMap+0x64/+0x74`, index five-byte Quest Guard and nineteen-byte
  Seer records, call the retail-only string-returning rows at 0x573040 and
  0x5741b0, copy their results to the rollover buffer and run the exact
  Dinkumware temporary cleanup. Quest Guard receives the dword at 0x69778c;
  Seer receives the local player. The pool names are corroborated by the
  admitted structure evidence, but both callees retain ordinal names and
  their bodies remain unclaimed. No external implementation body was used.
  The paired temporaries restore retail's 0x234-byte frame. Whole-linked
  fuzzy coverage rises from 45.32% to 45.39% with all 611 exact linked
  functions retained.

- **2026-08-09 — `hero::GiveResource` reconstructed byte-exact
  (178 bytes).** Retail accepts resource ids 0..6 with the inclusive
  `cmp 6 / jg` form, adjusts the owning player's proven seven-dword resource
  row and clamps negative results to zero. It refreshes the resource display
  only for `gpCurrentPlayer` while the adventure manager is active, then
  retains the trailing `game::IsHuman(owner)` call whose result is discarded.
  The single newly consumed window declaration is exposed only in hero.obj's
  narrow view. Dreamcast CodeView supplies the identity and parameter names;
  the player indexing, clamp, manager predicate, display call and trailing
  call are all retail-byte-proven. No external implementation body was used.
  Whole-linked fuzzy coverage rises from 45.49% to 45.53%, exact linked
  functions from 616 to 617, and executable fuzzy coverage from 8.63% to
  8.64%.

- **2026-08-09 — `hero::find_summonable_boat` reconstructed byte-exact
  (170 bytes).** Retail first asks the already-exact `game::GetHeroBoat` for
  the hero's existing unoccupied vessel, then scans the proven 40-byte boat
  vector for allocated, unoccupied boats owned by the local player or by no
  player. It minimizes Manhattan distance from the hero and deliberately
  accepts equal-distance later entries. Natural x-first source evaluation
  produces retail's y-first instruction schedule under VC6. The new member
  declaration is confined to hero.obj's existing narrow view: exposing it
  broadly moved an unrelated recruit function below its ratchet, and that
  experiment was withdrawn rather than accepted. Dreamcast corroborates the
  identity and boat member names only; no external implementation body was
  used. Whole-linked fuzzy coverage rises from 45.44% to 45.49%, exact
  linked functions from 615 to 616, and executable fuzzy coverage from
  8.62% to 8.63%.

- **2026-08-09 — `type_artifact::get_rollover_text` reconstructed
  byte-exact (134 bytes).** Retail branches directly on artifact id -1 and
  0, copying two runtime-loaded text pointers at 0x6a8040/0x6a804c; every
  other id indexes the 32-byte `akArtifactTraits` row and formats its name
  with the pointer at 0x6a8050. Natural `strcpy` calls reproduce both VC6
  `repne scasb`/`rep movs` expansions and `sprintf` reproduces the third
  branch exactly. The three text-cell spellings remain provisional because
  no public names them. Dreamcast CodeView supplies the member identity and
  signature only; no external implementation body was used. Whole-linked
  fuzzy coverage rises from 45.41% to 45.44%, exact linked functions from
  614 to 615, and executable fuzzy coverage from 8.61% to 8.62%.

- **2026-08-09 — `advManager::UpdateScreen` reconstructed byte-exact
  (125 bytes).** Retail fixes the update rectangle at (0,8), 608x544,
  regardless of the two retained formal flags, then samples `GameTime`,
  tests KB timer slot 0 as a signed due time and advances `animFrame` unless
  the byte at +0x104 is paused. The target's signed `jg` and local-slot order
  prove `_cpp_max(elapsedTime, 180)`: the timer advances by at least 180 ms,
  not by a capped amount, before one Windows message is pumped. Dreamcast
  CodeView supplies the signature, local names and `animCtrPaused` spelling;
  the x86 operations, constants, offsets and relocations are retail-proven.
  No external implementation body was used. Whole-linked fuzzy coverage
  rises from 45.37% to 45.41%, exact linked functions from 613 to 614, and
  executable fuzzy coverage from 8.60% to 8.61%.

- **2026-08-09 — `advManager::get_map_center` reconstructed byte-exact
  (111 bytes).** Retail reads the packed origin at +0xe4, adds the two
  viewport half-extents at +0xec/+0xf0, preserves the origin's four-bit z,
  and returns one packed `type_point`. The same three field expressions are
  independently present in the admitted hover paths, while Dreamcast
  CodeView supplies only the surviving inline member's const signature.
  Expressing the result as direct `type_point` construction, rather than
  assigning three fields after default construction, makes VC6 combine y
  and z before their single word store and reproduces all retail bytes. No
  external implementation body was used. Whole-linked fuzzy coverage rises
  from 45.34% to 45.37% and exact linked functions from 612 to 613.

- **2026-08-09 — `HeroExtra::HeroExtra` reconstructed byte-exact
  (104 bytes).** Retail's `game::game` hands this constructor to the vector
  iterator for 156 elements with a 0x334 stride. The body independently
  proves nineteen 8-byte equipped-artifact records at +0x68, sixty-four
  backpack records at +0x100, a Dinkumware string at +0x308 and a
  `std::bitset<70>` at +0x320; `HeroFn_004D8B30` closes the untouched tail
  offsets and total size. A game-TU-only view gives `type_artifact` its
  two-`-1` default constructor, allowing VC6's implicit member construction
  to reproduce both loops, the string triple and the bitset clear exactly.
  Dreamcast CodeView corroborates the class and member identities only; no
  external implementation body was used. Whole-linked fuzzy coverage rises
  from 45.32% to 45.34% and exact linked functions from 611 to 612.

- **2026-08-09 — `advManager::SetRolloverText` extends to 72.3630%.**
  Retail admits Mine's call into the 595-byte helper at 0x40d670: the two
  call sites and `ret 0xc` prove a five-parameter /Gr surface receiving the
  output buffer, cell, local player, separator and full-list flag. Because no
  surviving name identifies that retail-only helper, the declaration remains
  the ordinal `AdvmgrFn_0040D670` and its body is not admitted. The retail
  literals at 0x660330/0x66034c also correct the shared separator and visited
  format to `" "`/`" %s"`. No external implementation body was used.
  Whole-linked fuzzy coverage rises from 45.27% to 45.28% with all 610 exact
  linked functions retained.

- **2026-08-09 — `advManager::SetRolloverText` extends to 72.1363%.**
  Retail admits both creature-generator ownership arms, Lighthouse ownership
  and Town naming. The switch blocks prove the 92-byte generator and 64-byte
  mine pool indexing, signed subtype/owner loads, the two generator-name
  tables, the eight-entry ownership-color table and the `%s - %s`/` - %s`
  formats. Town uses the already proven `game::GetTown` inline path, reads
  the displayed-name pointer at town +0xc8 with the shared empty fallback,
  and pairs it with the nine-entry town-type name table. Dreamcast is used
  only to corroborate the generator-table semantics; no external
  implementation body was used. Whole-linked fuzzy coverage rises from
  45.14% to 45.18% with all 606 exact linked functions retained.

- **2026-08-09 — `advManager::SetRolloverText` extends to 70.4286%.**
  Retail admits the Hero and Obelisk arms and completes Border Tent's
  player-visit annotation. The function computes the local-player bit once,
  reads Border Tent bytes at game +0x4e364 and signed Obelisk bytes at
  +0x4e3e9, and formats a map hero's thirteen-byte +0x23 name with the
  central +0x40 format and the retail-only class-text helper. These offsets,
  the switch targets, call relocations and visited/unvisited tails are all
  retail-byte-proven; no external implementation body was used. Whole-linked
  fuzzy coverage rises from 44.87% to 45.10% with all 601 exact linked
  functions retained.

- **2026-08-09 — `advManager::SetRolloverText` extends to 60.3162%.**
  Retail admits Border Guard/Tent color-name formatting, Hill Fort's global
  information annotation, Pyramid's current-hero knowledge state, and the
  Water Wheel/Windmill known-and-depleted states. The retail switch entries,
  direct name-table relocations, player-knowledge tests, extra-info masks and
  common visited/unvisited tails prove the behavior; no external
  implementation body was used. Whole-linked fuzzy coverage rises from
  44.76% to 44.85% with all 599 exact linked functions retained.

- **2026-08-09 — `advManager::SetRolloverText` extends to 56.3176%.**
  Retail admits the Dead Guy, Fountain of Fortune, Lean-To, Magic Spring,
  Monster, Mystical Garden and Resource arms. The four player-visit fields
  were already byte-modeled at playerData +0xb8..+0xc4; this caller now
  corroborates their per-object masks and the Magic Spring/Garden depletion
  bits. Monster's trigger gate, bounded creature-id lookup, army-size name
  call and `%s %s` format, and Resource's direct name-table lookup are all
  relocation- and branch-proven. No external implementation body was used.
  Whole-linked fuzzy coverage rises from 44.63% to 44.76% with all 599 exact
  linked functions retained.

- **2026-08-09 — `advManager::SetRolloverText` extends to 51.0853%.**
  Retail's repeated global-info and hero-visit blocks admit the Magic/Power/
  War School, Magic Well, Mercenary Camp, Mermaid, Oasis, Rally Flag, Siren,
  Stables, Temple, Training Grounds, University, Wagon, Warrior Tomb and
  Watering Hole arms, plus the chat-editor focus early exit. Direct retail
  loads at hero +0x57/+0x63/+0x67/+0x77/+0x7b and +0x105 prove the newly
  consumed visit fields and masks; the object switch targets, branches,
  relocations and measured objdiff delta prove the control flow. No external
  implementation body was used. Whole-linked fuzzy coverage rises from
  43.39% to 43.90% with all 592 exact linked functions retained.

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

- **2026-08-09 — `advManager::ProcessWaitingHover` reconstructed, then
  advanced on 2026-08-11 to 80.9400%.** Retail first bounds the pointer against the adventure map,
  converts its pixels to cached tile offsets and a packed map point, and
  admits rollover detail only when the point is valid, visible to the local
  player and on the current hero's map level. It then resolves the cell,
  updates rollover text, and selects the local-human-town or owned-hero cursor
  command; outside the map it preserves active scroll cursors only inside the
  16-pixel screen-edge zone before forwarding hover to the adventure window.
  The compiled and retail bodies have the same 975-byte target span, 27
  conditional branches and four returns, with every branch mnemonic and
  symbolic target agreeing. VC6 register/stack scheduling around the inlined
  packed-point helpers remains. Retail proves the complete control flow, screen/scroll bounds,
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

- **2026-08-09 — `THeroScreenWindow::ExitDialog` reconstructed byte-exact
  (44 bytes).** The vtable-only handler sets the shared window manager's
  dialog return to 0x7802, emits a widget message with both control codes
  equal to ten, and returns the forward-dispatch verdict. Existing
  `winmgr.h`/`message.h` domains name every value; retail instructions and
  the vtable prove the behavior and identity, while Dreamcast corroborates
  only the method signature. One source-order detail is byte-selected:
  assigning `codeY` before `codeX` reproduces retail's +8 then +4 stores;
  the intuitive opposite order emitted those stores in the opposite order.
  This applies the HoMM2/Gruntz named-domain and source-order method. No
  external implementation body or `decomp-attempt-1` material was used.

- **2026-08-09 — `hero::HeroFn_004D8F70` reconstructed byte-exact
  (62 bytes).** The retail-only getter normally indexes one of eighteen
  64-byte hero-class records and returns its name pointer at +4. For hero 27
  in campaign mode and scenario 15 it instead returns the central text
  record's pointer at +0xb80. Retail instructions and relocations prove both
  branches, the byte-width campaign flag, the full-dword scenario field and
  every offset; the public identity remains provisional because no surviving
  Dreamcast name covers this Complete-only body. The hero-only include view
  exposes just the scenario and text fields needed here, following the
  HoMM2/Gruntz minimal-view rule without widening the shared game or text
  layouts in unrelated translation units. No external implementation body or
  `decomp-attempt-1` material was used.

- **2026-08-09 — `hero::UseSpell` reconstructed byte-exact (89 bytes).**
  The method subtracts its cost from the hero's signed-short mana and clamps
  the result to zero. When the adventure manager is active and the acting
  player is local, it refreshes all hero locators with the retail arguments
  `(-1, 1, 1)`. Retail instructions and relocations prove the mana width,
  manager/player globals, active-state test, caller target and arguments;
  Dreamcast supplies the public method name and `int cost` signature only.
  The matching source uses the TU's by-value min/max idiom plus a named
  integer result: that lifetime keeps retail's 32-bit selected-temporary load
  before the final short truncation, while direct assignment made VC6 narrow
  the load to sixteen bits. This applies the HoMM2/Gruntz helper-boundary and
  lifetime method. No external implementation body or `decomp-attempt-1`
  material was used.

- **2026-08-09 — `hero::hire` reconstructed byte-exact (100 bytes).**
  The method finds this hero id in the player's two tavern offers, subtracts
  the shared hero price from gold, places the hero at the supplied packed
  point and passes the consumed offer index to the game's town-hire closeout.
  Retail instructions and relocations prove the player/recruit/resource
  fields, every stride, both calls and the source-significant unbounded offer
  scan; Dreamcast supplies the public method name and parameter types. The
  already exact `town::hire` sibling provided the canonical in-tree helper
  boundary, while retail selected this shorter variant's exact operations.
  The shared cost and closeout declarations remain visible only in hero.obj
  and town.obj: exposing the cost declaration tree-wide measurably regressed
  `initialize_game_data` and was rejected. This applies the HoMM2/Gruntz
  sibling-pattern and minimal-view rules. No external implementation body or
  `decomp-attempt-1` material was used.

- **2026-08-09 — `hero::HeroScreenUpdate` reconstructed byte-exact
  (101 bytes), settling its retail arity.** Contrary to the no-argument
  Dreamcast revision, retail consumes two ints: a primary-stat index and a
  quick-view flag. It inlines the established signed-byte primary-skill
  clamp, combines the result with the quantity marker, selects normal versus
  quick dialog type and calls `NormalDialog` with the indexed primary-stat
  name and resource frame. Retail proves both arguments, the four-name
  pointer row at 0x6a7540, every dialog ordinal and the complete call shape;
  Dreamcast contributes only the source-order identity and semantic name
  family. Rewriting the byte-equivalent skill floor from `skill > 1` to
  `skill >= 2` selects retail's `cmp 2 / setge` and preserves every existing
  caller, applying the HoMM2/Gruntz source-shape rule without a duplicate
  helper. No external implementation body or `decomp-attempt-1` material was
  used.

- **2026-08-09 — retail-only `hero::HeroFn_004DC070` reconstructed
  byte-exact (135 bytes).** The provisional member removes the combination
  artifact from one equipped slot, uses that artifact's traits field at +0x14
  to select a 24-byte combination row, walks all 144 component bits and
  re-equips every set artifact id with extra value -1 into its first legal
  slot. Retail proves the complete behavior, both table cells and strides,
  the five-dword bitset, the two helper calls and all bounds; no Dreamcast
  row exists for this Shadow of Death addition, so the public identity stays
  ordinal. Only hero.obj sees the typed component bitset, preserving the
  established layouts elsewhere. A named const reference to that bitset is
  source-significant: it makes VC6 hoist the row address in EBX across calls,
  whereas the direct member expression recomputed it inside the loop. This
  applies the HoMM2/Gruntz minimal-view and lifetime rules. No external
  implementation body or `decomp-attempt-1` material was used.

- **2026-08-09 — `THeroScreenWindow::update_all_slots` reconstructed
  byte-exact (23 bytes).** Retail retains `this` in EDI and walks a long
  ESI index from zero through all nineteen equipped positions, calling the
  570-byte per-slot updater once per iteration. Dreamcast supplies both
  method names and their caller edge, but its older artifact-slot enum has
  only eighteen positions; the retail body and the independently proven
  nineteen-record hero layout therefore control the bound. The narrow
  window declaration follows the existing `hero::remove_artifact(long)`
  precedent and carries only ordinal first/count constants, avoiding a
  fabricated transfer of the smaller DC enum or any unproved window layout.
  This is the HoMM2/Gruntz minimal-view and named-bound method. No external
  implementation body or `decomp-attempt-1` material was used.

- **2026-08-09 — all three retail `ExtraInfoUnion` accessors reconstructed
  byte-exact (13, 32 and 28 bytes).** `get_black_box` forwards the const
  union pointer to the central adventure manager. `get_creature_bank` and
  `get_university` extract bits 13..24 as an unsigned twelve-bit index and
  address the central game's vector first pointers at +0x4e3dc/+0x4e3cc
  with retail's 108-byte and 16-byte element strides. Dreamcast supplies
  the union-arm names and correct const/reference signatures; retail proves
  every field, shift, mask, pool base and stride. Only the two indexed arms
  needed here were admitted under the existing advmgr-only view—the other
  eighteen DC alternatives remain unmodelled, and `NewmapCell` keeps its
  established raw-dword view in every other TU. This applies the
  HoMM2/Gruntz narrow-view rule without perturbing the wider include graph.
  No external implementation body or `decomp-attempt-1` material was used.

- **2026-08-09 — `ComputeUALoc` reconstructed byte-exact (44 bytes).**
  The function indexes the central game's eight 360-byte player records by
  its fastcall argument and asks that player to update the same player's
  Grail guess. VC6 inlines the newly exact `guess_grail_location` leaf,
  producing retail's local hidden-return buffer and odd-offset four-byte
  store without any duplicated source. Retail proves the global, stride,
  index reuse and all emitted instructions; Dreamcast independently records
  the direct `ComputeUALoc` -> `playerData::guess_grail_location` edge. This
  is the HoMM2/Gruntz canonical-helper-boundary rule applied directly. No
  external implementation body or `decomp-attempt-1` material was used.

- **2026-08-09 — `playerData::guess_grail_location` reconstructed
  byte-exact (27 bytes), and its callee identity corrected from retail.**
  The wrapper passes the player id in EDX and a hidden four-byte return
  buffer in ECX, then copies that packed point to `playerData+0x39`.
  Its only retail call lands at 0x52c9b0; that 1,371-byte body begins with
  `SetupPuzzlePieces`, performs the puzzle-map search and returns a packed
  `type_point`, while the old IDA name at 0x52ce90 is demonstrably 0x4e0
  bytes inside the same function. Dreamcast independently supplies the
  `AI_attempt_puzzle_guess(long)` public signature and the unique
  `guess_grail_location` caller edge, so 0x52c9b0 is now admitted under
  that name. The cast-free source keeps the still-raw, odd-aligned member
  model: a named local receives the UDT return and intrinsic `memcpy`
  writes its four bytes. Applying the HoMM2/Gruntz lifetime rule makes VC6
  reuse the dead incoming argument slot for that local and emits every
  retail instruction exactly. No external implementation body or
  `decomp-attempt-1` material was used.

- **2026-08-09 — `combatManager::DamageWall` reconstructed byte-exact
  (320 bytes, including its eight-way jump table).** Positive damage is
  subtracted from the target row's indexed strength and clamped to zero. A
  destroyed ordinary segment clears bit 1 on its blocked combat cell; target
  3 clears the drawbridge state; targets 0/6/7 clear paired special-wall
  dwords and set creatureId bit 21 on an indexed defender stack. The tail
  writes the clamped strength and a 0/1 standing-state dword. Retail directly
  proves `type_wall_target::wall_id` at +8, the ID values
  {5,6,8,9,10,12,13,14}, the three defender indexes at +0x13d98/+0x13dbc/
  +0x13de0, paired three-dword rows at +0x13f9c/+0x13fe4, and the fifteen-
  dword standing row at +0x13fa8; these slices close the old padding exactly.
  The first reconstruction had all 21 blocks flow-identical and 15 exact.
  Applying the HoMM2/Gruntz named-lifetime rule closed the remaining six:
  splitting `strength`'s declaration from its assignment reserves ESI for it
  and leaves the table offset in EDI, while introducing `wall_id` only after
  the switch keeps EAX live across both final stores. No external code or
  `decomp-attempt-1` material was used.

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
  keeps honest integer/aggregate spellings. **Superseded 2026-08-28:** paired
  literal and aggregate/field normalization proves both representations and
  the function is now exact. `decomp-attempt-1` was surveyed
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
  tightly scoped inline-budget method used for mousemgr (the historical
  TSplitWindow adapter comparison was superseded on 2026-08-28);
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
  local inline-budget technique was historically compared with TSplitWindow's
  adapter, which was superseded by its natural-source closure on 2026-08-28;
  here it applies to a constructor lifetime rather than an STL
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

  **Superseded 2026-08-28:** this was a local maximum. Dreamcast proves
  thirteen ordinary `push_back` statements and the canonical `slider`; the
  adapter and final `insert` are removed, and the constructor is exact.

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

  **Superseded 2026-08-28:** duplicating the stores was a source-false local
  maximum. Dreamcast's state, shared slider statement, and positive mouse arm
  with its own return reproduce the same retail topology and close at 100.0%.

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
  admitted; its inventories remained candidate metadata at that point.

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
  still requires retail-byte corroboration before admission.

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
  contract change and is **open for a design decision**.
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
  tree-wide move needing a dedicated change; the member is carried
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
  entered `include/armygrp.h` with **NH3API lineage**. Every VALUE is
  retail-proven — the
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
  needs independent retail corroboration; currently the placeholder
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
  external-candidate — promotion only after retail corroboration. One
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
  row becomes evidence only when retail corroboration promotes it.

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
  column (empty until an identity is byte-proven and admitted)
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
  deliberate claim review; NEW violations fatal; `--write-baseline`
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
  carrying the target identity, the evidence provenance policy, the evidence
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
