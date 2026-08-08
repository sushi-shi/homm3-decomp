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
