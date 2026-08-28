# VC6 SP3 behavior catalog — the model's spec and test corpus

This document is the specification the `vc6` predictors are built against and
**the model's test corpus**: every behavior below carries a stable ID, its
in-tree evidence pointer, a status, and a `probe:` field. A landed probe is an
**oracle case** — a standalone TU under `scripts/homm3/vc6/probes/` that
reproduces the behavior against the real pinned compiler in ~5 seconds, so the
claim never has to be re-litigated from the match tree. Behaviors a single TU
cannot reproduce say so explicitly and why; several also record the *measured
failed reductions* so nobody re-burns those hypotheses.

Profile under test: **VC6 SP3 `CL.EXE`, `/O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR-
/D_WINDOWS`**, LINK 6.00.8447 with `/OPT:REF` (+ evidence of ICF), Dinkumware
STL. Sibling corpus: **MSVC 4.2 `/Od /MT /Gr /G5 /Ob1 /QIfdiv`** with 39 `/O2`
TUs. Compiled sources: the `Residual (`/`EXACT` blocks across `src/*.cpp`,
`config/units.toml`, the dated block in `config/match_baseline.tsv`,
`.claude/skills/match/SKILL.md`, homm2's `docs/patterns/*` + `docs/msvc42-*`,
and attempt-1's `docs/compiler-toolchain.md`.

## Running the oracle

```sh
nix develop .#build --command python3 -m homm3.vc6 oracle all --all   # everything
nix develop .#build --command python3 -m homm3.vc6 oracle inline --all # a* probes
nix develop .#build --command python3 -m homm3.vc6 oracle a06 --all    # one probe
```

Subsystems: `inline` = `a*`, `reg` = `b*`, `state` = `c*`, `flow` = `d*`,
`all` = everything; any other token selects by probe-stem prefix. rc 0 = all
selected probes PASS; rc 1 = any FAIL/ERROR (gate semantics — the runner's
FAIL path is negative-controlled). Scratch and `.asm` listings land under
`build/vc6/probes/`.

Each probe declares its expectations in its header as machine-readable
directives the runner greps from the source:

```
// CATALOG: A1 A2                    ids reproduced
// FLAGS: /O2 /Ob2 ...               cl flags (runner appends /c /FAs)
// EXPECT-ASM[(scope)]: <regex>      DOTALL search (whole listing / one PROC..ENDP)
// EXPECT-NOT-ASM[(scope)]: <regex>  must not match
// EXPECT-COUNT-ASM(scope): <n> <regex>   exactly n matches
// EXPECT-SAME-ASM: <scopeA> <scopeB>     normalized instruction streams equal
```

Matching runs over the *filtered* listing (comment lines dropped, inline `;`
comments stripped, PROC/ENDP boundary lines reduced to bare markers), so
`[\s\S]{0,N}` windows measure instructions, not comment text. Probe statuses:
PASS / FAIL / UNCHECKED (no directives; compiled for human reading) / ERROR.

Status legend for catalog entries: **explained-lever** (the source-side handle
is known and byte-proven), **open-residual** (observed, not yet predictable).
Probe verification date for every PASS below: 2026-08-09, 23/23 probes, 65
checks, against the pinned SP3 CL under Wine.

---

## 0. Flag-level ground truth (the model's fixed inputs)

| ID | Fact | Evidence | Status | probe |
|---|---|---|---|---|
| 0.1 | `/Ob2` (not `/Ob1`): auto-inlining *with* unconditional emission — `heroWindow::BroadcastMessage` pair: both out-of-line bodies exist in retail AND their loops are inlined into `WidgetSet`/`ClearStatus` in the same TU | `config/units.toml:21-24` | closed | `a01_single_site_extern.cpp` (both halves: expansion + kept extern copy) |
| 0.2 | `/Gr`: retail's free functions are fastcall (`Random` takes ecx/edx) | — | closed | implicit in every probe (`@@YI` fastcall mangling, args in ecx/edx) |
| 0.3 | `/Oy-` and `/O2` (not `/O1`/`/Od`): `IsMember` keeps an EBP frame; frameless `GetNumArmies` proves the frame is *conditional* | — | closed | none (needs retail contrast; the conditional frame is visible across probes: `b21` framed, `a01` frameless) |
| 0.4 | `/Op` engine-wide: `fdiv` against the pool constant, int→double through memory, CRT `sqrt` (FP intrinsics off); all-TU sweep left every exact function exact | `units.toml:25-29`, `src/ai_tactical.cpp:1549` | closed | none dedicated; `d16` corroborates the scope: /Op does NOT disable the *string* intrinsics |
| 0.5 | `/GX`: `sample` ctor `0x566da0` carries a full `fs:[0]` frame; the "first no-EH TU" inference was RETIRED | `units.toml:43-50` | closed | none (image-level; probes compile under /GX) |
| 0.6 | `/GR-` for game TUs: none of 252 recovered vtable starts has a COL pointer; no `__RTDynamicCast` family | attempt-1 `docs/compiler-toolchain.md:125-133` | closed | none (image-level) |
| 0.7 | Dinkumware, **not** STLport (CLOSED 2026-08-07); `<bitset>`/`<vector>` from the pinned toolchain suffice | SKILL.md:158-160 | closed | none (toolchain identity) |
| 0.8 | Non-`/Gy`: `inline` member definitions reproduce absence of out-of-line copies (winfile `Exists`) | `src/strip.cpp:21-23` | closed | none |
| 0.9 | Mixed object generations in the retail link: Rich header shows C++ build **8447 for 145 objects**, **8168 for 26** — independent corroboration for the stale-CL-generation residual class (F1) | attempt-1 `docs/compiler-toolchain.md:6-8` | closed | none (link-level) |

---

## A. Inlining — `/Ob2` budget and emission rules

### A1. Single-call-site inlining is unconditional and size-independent
`AppInit`→`WinMain` ≈300 B inlined; applies to *statics AND externs*.
- evidence: SKILL.md:101-107
- status: explained-lever
- probe: `a01_single_site_extern.cpp` (PASS — loop-bodied extern helper
  expanded at its one site, no `call`)

### A2. Extern linkage ⇒ out-of-line copy still emitted, in addition to the inline expansion
Byte-proven: `armygrp` accessor `0x44a460` (85 B, standalone body AND
expansion inside `GetMorale`, `src/armygrp.cpp:486-497`); `AI_value_of_morale`
(30-B forwarder inlined and still emitted at `0x435830`,
`src/ai_tactical.cpp:1163-1166`); `mouseManager::Reset` inlined into `Open`
with the copy at `0x50cc80` serving the external WM_ACTIVATE caller
(`src/mousemgr.cpp:85-87`).
- status: explained-lever
- probe: `a01_single_site_extern.cpp` (PASS); also visible in
  `a04_auto_inline_off.cpp` (`open_`'s copy kept)

### A3. Static + single call site ⇒ body vanishes entirely
`/Ob2` inlines, and no copy is emitted (compile side; `/OPT:REF` handles the
COMDAT case). Byte-proven absences: `initialize.cpp`
`create_included_masks`/`create_building_masks`; `recruit`
`SiegeMonsterToSiegeArtifact`, `recruitUnit::UpdateCost`,
`TRecruitQuickWindow::TRecruitQuickWindow`; `strip`
`DrawNumber`/`DrawSelector`; `soundmgr` `SamplePlaying`/`ServeSampleStream`;
`findpath` `valid_move_adjacent`×2, `build_combat_path`, `mark_enemy`,
`check_enemy_armies`; `armygrp` `UpdateSplitArmy`, `SetRolloverText`,
`DamageGroup`; `exec` `executive::executive`.
- evidence: `src/armygrp.cpp:156-158`, `src/recruit.cpp:108-118`, `src/findpath.cpp:136-140`
- status: explained-lever
- probe: `a02_static_single_site.cpp` (PASS — no `?helper` symbol anywhere)

### A4. Reproduction instrument: `static` for free functions, `inline` for members (non-`/Gy`)
- evidence: `src/recruit.cpp:197-200`, `src/strip.cpp:97-99`
- status: explained-lever
- probe: `a02_static_single_site.cpp` (the `static` half); member `inline` not probed

### A5. Size arithmetic validates the rule
Retail `FindCombatPath` is 1927 B against the DC body's 1082 + absorbed
callees 212+72+140.
- evidence: `src/findpath.cpp:136-140`
- status: explained (arithmetic over retail bytes)
- probe: none (needs image-level context — the check is against retail sizes)

### A6. Inline cost is calibrated by the callee's SOURCE body/locals — the sharpest datum
`initialize.cpp` `create_included_mask`: the two const locals and the repeated
slot-0 store reproduce retail's /Ob2 cost estimate — `initialize_game_data`
expands exactly four copies (rows 0/1/3/2) and calls the other five; drop any
of them and the inliner expands five or six (measured 2026-08-07). The
duplicate store is optimized away; no byte of the body changes.
- evidence: `src/initialize.cpp:288-295`
- status: explained-lever; quantized. The single best calibration target for a cost model.
- probe: `a06_inline_cost_calibration.cpp` (PASS) — standalone reduction at
  the measured budget edge: baseline 9-site caller = 7 expansions + 2 calls; a
  dead pre-loop store (~4 emitted instructions) collapses it to 1 expansion +
  8 calls — the estimate shift is wildly disproportionate to the byte delta,
  proving source-side accounting. **Refinement the reduction adds:** three
  const locals folded into the loop head shift *nothing* (EXPECT-SAME-ASM
  verifies the emitted bodies are instruction-identical, and the counts hold)
  — in isolation the cost lives in the extra *statement*, not in const-local
  declarations per se.

### A7. The budget cuts both ways — caller-score coupling
`soundmgr::ConvertVolume`: factoring the duplicated arms into a helper
improved it in isolation but shrank it under budget, so it inlined into
`MemorySample`/`ModifySample` — `ModifySample` **100 → 57.3**. Longhand
restored both callers. Current state (`src/soundmgr.cpp:121-128`): the
retail-exact smaller body still falls under OUR budget; a scoped
`#pragma auto_inline(off)` pin is carried — our budget and retail's disagree
at this size point.
- evidence: SKILL.md:92-100
- status: lever explained; **budget calibration vs retail OPEN**
- probe: `a06_inline_cost_calibration.cpp` (PASS — demonstrates the
  size→decision coupling standalone; the retail-vs-ours calibration itself
  needs retail bytes)

### A8. Depth-2 budget stop — `ai_combat::simulate_combat`, 46.8%
Retail inlines `get_fastest_speed` but leaves its inner `get_total()` as a
real call — budget stops at nesting depth 2 — while our SP3 CL inlines
`get_total` too. Mitigated by splitting the speed values; the attacker still
over-inlines.
- evidence: `src/ai_combat.cpp:960-967`
- status: open-residual
- probe: none (the stop is retail's, diverging from our CL; a standalone TU
  only shows our side. Note from `a05`: `inline_depth(1)` cannot pin a
  depth-2 leaf either — the one-pass inliner folds leaves into callers'
  stored bodies bottom-up, measured on an f→g→h chain)

### A9. Budget runs out MID-function — `ai_combat::do_general_melee`, 94.8% (was 79.6)
`kill()` inlines twice; retail expands `get_total` inside BOTH copies; our
CL's budget runs out after the first, and every register downstream of the
surviving call renames. Callee-side probes rejected (no-local ternary,
unsigned-cast-free — identical; doubly-nested ternary 75.5). Caller-side:
hoisting `ratio` 94.79, moving `kill()` 89.30, folding ratio into the
argument 93.42.
- evidence: `src/ai_combat.cpp:911-925`
- status: OPEN — cleanest evidence the budget is positional/sequential within one function
- probe: `a06_inline_cost_calibration.cpp` (PASS — standalone shadow: the 2
  surviving calls are the LAST 2 of 9 sites, i.e. sequential exhaustion; the
  retail-vs-ours exhaustion *point* still needs image context)

### A10. Depth-2 leaf calls survive inside an expansion
`armygrp` `0x44a460`: retail's expansion inside `GetMorale` leaves the bitset
members as depth-2 calls (`call set(0,1)` etc.) while the same source folds
all five to `or al,<bit>` out of line.
- evidence: `src/armygrp.cpp:489-497`
- status: explained observation; direct depth-boundary test case
- probe: none (needs the bitset member context and retail contrast; cf. A8
  note on why depth pinning is not standalone-reproducible)

### A11. Over-inline that is NOT the single-call-site rule — `hero::SetSS`, 3.8%
PROVEN by `#pragma auto_inline(off)` around `GiveSS`: SetSS then 27/27 exact.
Adding a second live call site changed nothing (measured) — retail's GiveSS
simply carried more inline cost than the reconstruction; the residual is
under-reconstruction of the callee. Rejected: `signed char* pLevel` respelling
(costs GiveSS 100.0 → 73.4).
- evidence: `src/hero.cpp:1080-1097`
- status: OPEN; hypothesis "callee reconstruction is cheaper than retail's callee"
- probe: `a04_auto_inline_off.cpp` (PASS — the *instrument* only: the pragma
  restores a real call; the residual itself needs the hero TU)

### A12. The AppCommand anomaly — the counterexample to A1
`kbwin::AppWndProc` 74.8 → 100.0 under the pragma stand-in. Retail's
AppCommand has THREE call sites in the image (0x4ec253, 0x4ec275, 0x4f7f59) —
two in another TU, so retail's compile saw the same one-call-site TU we do
and still did not inline it; the source-level reason is unidentified.
- evidence: `src/kbwin.cpp:109-121`; SKILL.md:103-104
- status: OPEN — highest-value single unexplained inliner decision in the corpus
- probe: `a04_auto_inline_off.cpp` (instrument only, as A11)

### A13. Scoped `inline_depth` as the retail-shape instrument
`mousemgr::CheckUpdate` EXACT (block-scoped depth-0 around only the inner
`TCSLock` declaration; emits the byte-exact 25-B ctor COMDAT at `0x50d890`);
`ai_player::make_gift` (scoped depth-0 reproduces the called
`string::append` instantiation at `0x41b250`).
- evidence: `src/mousemgr.cpp:583-588,620-622`, `src/ai_player.cpp:381-394,457-459`
- status: explained-lever; strong evidence the true budget is lexically/positionally scoped
- probe: `a05_inline_depth_scope.cpp` (PASS — depth(0) pins `call ?mid` in
  one function while a depth(255) sibling in the same TU fully inlines)

The old `TSplitWindow` depth-0 `AppendSplitWidget` entry was a 98.40% local
maximum, not a source fact. It was removed after the Dreamcast dossier proved
thirteen ordinary `push_back` statements; the natural source is exact as of
2026-08-28.

### A14. Emission order of deferred bodies
VC6 defers the out-of-line copies of inline-expanded functions to the end of
the TU (retail: `initialize_game_data` FIRST at 0x4eb730, then the kept
statics in source order); the one-pass inliner needs every body before its
caller, so definitions keep DC source order.
- evidence: `src/initialize.cpp:13-19`
- status: explained
- probe: `a03_deferred_emission.cpp` (PASS — an address-taken static defined
  FIRST in source is inlined at its call site and its kept body is emitted
  after both callers. Note: a plain extern's unconditional copy is NOT
  deferred — `a01`'s helper is emitted in place, so deferral is specifically
  about bodies whose emission was pending the referenced/inlined decision)

### A15. Leaf spelling is a GLOBAL variable — score the callee through the expansion
`ai_combat::get_total` ternary respell: out-of-line 83.6 → 100.0, and the
payoff is in ~12 INLINED copies (`get_area_value` 86.6→100,
`cast_area_effect` 86.0→97.4, `do_general_melee` 79.6→94.8, `adjust_army`
89.8→93.4, no other edit). `type_monster_data::take_damage`: both spellings
byte-identical out of line, but INLINED the parameter-as-return-value form
took `inflict_melee_damage` 97.0, `cast_area_effect` 97.4,
`cast_damage_spell` 94.2 all to 100.0 and `cast_chain_lightning` 71.9→79.7
with no call-site edit. Rejected: early return (97.06 — split exit duplicates
the epilogue); a `dealt` local (99.91).
- evidence: `src/ai_combat.cpp:1364-1383`, `src/ai_combat.cpp:206-224`
- status: explained-lever; the model must score callee spellings through the expansion
- probe: none (the observable is the delta across a dozen retail call sites;
  the *ternary merge mechanism* itself is probed standalone — see D8)

### A16. Retail's inline copies of one body disagree with each other
`game::GetPlayerName` 99.5: retail's two copies disagree on registers; ours
agree — the tell that the difference is allocation order, not shape.
`initialize_game_data`: retail hoists in inline copy 0 (`lea eax,[8*eax]`)
and folds in copies 1 and 2 (`mov [8*eax + gHierarchyMask+0x160], ebx`) — one
loop body, two addressing decisions, no spelling can produce both.
- evidence: `src/game.cpp:1982-1986`; `config/match_baseline.tsv` dated block, `src/initialize.cpp:44-50`
- status: OPEN — proves per-expansion optimizer state
- probe: none (needs retail bytes; our CL's copies agree by construction)

### A17. Cross-jumping between two inline expansions — `army::ValidAttack`, 81.0%
Retail cross-jumps the two inlined `GetAdjacentCellIndex` bodies (WIDE_LOWER's
`cmp <dir>,6` mismatch jumps into WIDE_UPPER's `cmp <dir>,7` block); our CL
emits both copies. NOT reachable from source: different first tests, only the
second half common.
- evidence: `src/path.cpp:151-158`
- status: open-residual
- probe: none (retail-only transformation; not reproducible by our CL on any spelling — that is the residual)

### A18. Miscellaneous inliner facts
- Budget *shifted* by an unrelated helper's parameter shape: `(low,value,high)`
  `armygrp_clamp` form shifts /Ob2's budget so the `set()` + four
  `operator[]` assignments match (`src/armygrp.cpp:1143-1145`).
- Three plain calls budgeted: `hero::GetArcheryFactor` — retail inlines all
  three `IsWieldingArtifact` calls; only the recursion inside survives
  (`src/hero.cpp:1542-1545`).
- Trivial ctor + single site: `DoDialog`'s `executive dialogExec = {0,0,0,0}`
  inline, no out-of-line body (`src/exec.cpp:35`).
- Locate-side: 6 of 11 DC bodies in the `ai_player` threat-checker bracket are
  4–18-B accessors /Ob2 folds away without trace (`src/ai_player.cpp:118-120`).
- `/OPT:ICF` suspected for `recruit::RecruitSliderCallback`; independent ICF
  evidence: `xstod.obj` `_Stof`/`_Stold` fold, but 19 groups of byte-identical
  bodies survive at 56 distinct addresses (attempt-1
  `docs/compiler-toolchain.md:101-108`).
- status: mixed
- probe: none (each needs its TU/link context)

---

## B. Register allocation

The modal residual class (~45 of 79 residual comments). Most B-entries are
divergences *between retail bytes and our CL's output*; a standalone probe can
only pin OUR CL's side, so `probe: none` below usually means "the phenomenon
is a two-compiler diff, not a standalone observable".

### B1. Whole-body ESI/EDI (or EBX) role swap, schedule otherwise identical
| Function | Score | Detail |
|---|---|---|
| `window.cpp:391` | 92.88 | retail pins centerX EBX / centerY EDI, ours reversed; 9/9 branches, 2/2 rets, schedule identical |
| `ai.cpp:387` | 96.4 | ESI/EDI swapped throughout; retail gives ESI to `enemy`, coalesces EDI across {enemy_hits, loop pointer} — "the allocator ranking the loop-heavy class first" |
| `ai_combat::cast_chain_lightning` | 79.7 | pure esi↔edi swap; retail parks `this` in edi at entry, ours esi + shrink-wrapped edi push |
| `ai_tactical:230` | 99.1 | retail: `this` esi, `our_hits` edi; ours swapped (start_enemy ebx in both) |
| `armygrp::GetArmyMorale` | 96.56 | mirror; retail homes `this` in EDI, shrink-wraps ESI |
| `ai_player::calculate_reserve` | 96.46 | retail ESI/EBX/EDI for {finish, dwelling, population}; ours ECX/EDI/ESI |
| `ai_combat::do_aftermath` | 79.73 | retail EDI/EBX for {defender, defeated hero}; ours opposite |
| `smackmgr::VideoPlay` | 87.2 | retail ebx,esi,edi = (x, vw, vh); ours edi,esi,ebx — cascade mechanistic: `x` in EBX forces `pos.x` into EAX and a `[ebp-0xc]` spill, freeing EBX for the loop's zero |
- status: open-residual (the flagship allocator class)
- probe: none (retail-vs-ours divergence)

### B2. `this` memory-homed vs promoted (frame size differs)
`iconwdgt.cpp:165` 72.0 (retail spills `this` to `[ebp-4]`, frame 0x5c vs
0x58, 21/21 flow agree); `iconwdgt.cpp:235` 86.4 (frame 0x24 vs 0x20; retail
burns EDI on the CSE'd literal 2, reloads `this` after each `Random()`);
`ai.cpp:974` 80.6 (retail parks `this` in EBX, spills across the inner loop,
RECYCLES EBX as the neighbour counter, ESI save shrink-wraps into
`SeedCombatPosition`'s inline).
- status: open-residual
- probe: none (retail-vs-ours divergence)

### B3. Counter/accumulator memory-homed vs enregistered
`ai_tactical.cpp:663` 85.6 (retail memory-homes the first loop's counter,
freeing ECX for `creatureId`); `cmbtmgr.cpp:944` 75.0 (retail carries `total`
in EBX and writes through to `[ebp-4]` after each ±500; ours spills `total`
outright); `smackmgr:430` 89.4 (retail homes x/w/h at -4/-8/-0xc, keeps y in
edi; ours registers x in ebx, spills w/h).
- status: open-residual
- probe: none (retail-vs-ours divergence)

### B4. Spill-to-parameter-slot / dead-parameter-slot coloring
`font.cpp:57` 78.3 (retail homes `width` in dead `c` slot [ebp+8]; declaration
order load-bearing, 24-permutation sweep run); `ai_combat::get_enchantment_value`
89.5 (retail spills `i` to the parameter slot, ours the opposite + loop-entry
`jmp`/reload rotation); `ai_tactical::get_blind_value`/`get_curse_value`
95.95/99.60 (retail colours combat value and priced value into the SAME dead
`enemy` slot — what one reused variable produces); `AI_value_of_morale` (the
running fraction lives in dead `caster` [ebp+0x18]); `path::ValidPath` 95.7
(retail stores to the dead `destIndex` slot; our CL dead-codes the same
parameter assignment — six spellings byte-identical, **NOT source-addressable**).
- status: open-residual (partly explained via the reused-variable spelling)
- probe: none (dead-parameter coloring needs the exact parameter list + pressure of those TUs)

### B5. Frame-slot COLOURING of equal-size slots — by live range, not declaration
`ai_tactical::get_attack_skill_value` 99.0 (one scratch slot reused for both
int→double conversions in retail, ours takes a second; three naming variants
swept); `get_defense_skill_value` 99.9 (four 8-B double slots permuted —
hoisting declarations gives IDENTICAL output: **VC6 orders these by live
range, not declaration**); `get_curse_value` 99.9; `ai.cpp:492` 99.4 (eight
instructions in the second inlined `min_ref`; byte multiset identical — a
slot tie-break).
- status: open-residual (tie-break rule unknown)
- probe: none (needs the exact slot population of those frames)

### B6. …but plain integer locals ARE laid out first-declared-lowest
`ai_tactical::get_area_effect_value`: accumulators declared before the vector
and friendly-before-enemy because VC6 lays the frame's plain locals out
first-declared-lowest ([ebp-0x14] then [ebp-0x10]) and emits the zero stores
in that order — swapping the declarations swaps both and costs the match.
- evidence: `src/ai_tactical.cpp:851-856`
- status: explained (hard rule; note the tension with B5 — doubles by live range, ints by declaration)
- probe: none — **two reductions measured 2026-08-09 and rejected**: with two
  loop-carried accumulators + a call, both stay in ESI/EDI (no homes at all);
  with five, VC6 homes an allocator-chosen subset (a1,a3) at -4/-8 — the
  declaration-order signal is erased because a standalone TU lets the
  allocator pick *which* locals to home. The rule is only observable in the
  original frame's pressure; re-verify in-tree via that function.

### B7. Register renaming cascade after a call-vs-inline change
`do_general_melee` (every register downstream of the surviving `get_total`
call renames); `get_area_value` 86.6/MAX 99.0 (retail keeps `defender` in
caller-saved ecx, reloaded after every call, freeing eax as divide-tail
scratch; the 99.0 MAX via begin()/end() costs six other functions the
`_M_start` CSE and is rejected); `cast_chain_lightning` 71.9 → 79.7 purely
from the inlined `take_damage` fix.
- status: open-residual (consequence of A9/B1 decisions)
- probe: none (cascade of a retail-divergent decision)

### B8. Constant/zero CSE into a callee-saved register vs materialize-at-use — bidirectional
Retail hoists, we don't: `mousemgr.cpp:155` 82.9 (retail: `push ebx; xor
ebx,ebx`, five+ reads); `ai_tactical::set_melee_attacks` 87.7 (retail reuses
the `rep stosd` zero in EAX; ours parks a second zero in ESI); `recruit:455`
98.0 (retail edx, ours eax). We hoist, retail doesn't:
`check_wall_archery_penalty` 70.7 (ours parks 0 in ebx for six zero
constants + hoists BOTH wall-mask dwords; retail writes immediates and ands
from memory); `iconwdgt.cpp:165` (ours CSEs 0 into EBX where retail compares
immediate); `findpath.cpp:462` 89.8 (retail's callee-saveds all spoken for →
load+`test`; ours hoists into free EDI → `cmp mem,edi` + rotated preheader).
- evidence: as cited per row
- status: OPEN class — bidirectionality proves it is an allocator-ranking output, not a spelling
- probe: `b08_zero_cse.cpp` (PASS — pins OUR CL's hoist side deterministically:
  one `xor eax,eax` serves byte/word/dword stores and two `cmp mem,reg`; in a
  leaf the zero takes EAX, the callee-saved choice appears under calls. The
  retail side of the class is a divergence and stays unprobeable standalone)

### B9. Signedness CSE for `-1` materialization
`-1` stores share one register (`or ecx,-1`, byte stores from cl) ONLY when
the byte fields are signed/plain char; unsigned char splits a
separately-materialized 0xff — the codegen itself is signedness evidence
(hexcell ctor). Concrete: `cmbtmgr::RemoveArmyFromGrid` — three −1 stores on
offsets 0x19, 0x18, 0x1a in that order, all from one `or bl,-1`.
- evidence: SKILL.md:108-111; `src/cmbtmgr.cpp:690-693`
- status: explained-lever, used as a type-recovery oracle
- probe: `b09_minus1_signedness.cpp` (PASS — signed: one `or eax,-1` shared
  by the dword store and three `al` byte stores, no `255` anywhere; unsigned:
  separate `mov al,255` AND the dword store falls back to an immediate `-1`;
  source store order preserved in both)

### B10. Argument-chain tie-break
`game::GetName` 95.8 (retail eax→ecx chain, `lea eax,[this+0xcc]` slotted
between loads two and three; ours starts ecx, spends edx, lea earlier);
inherited by `GetPlayerName` 99.5 + three eax/edx/ecx swaps.
- status: open-residual — probe: none (retail-vs-ours)

### B11. Divide-result register
`hero::GetMobilityFrame` 90.4 (retail keeps the quotient in EDX for the sign
fix-up; ours moves to EAX and scratches with dead `movePoints` ECX; same
count); `GetManaFrame` 88.9 (same family + `xor ecx,ecx / setge cl / add
ecx,0x18 / mov eax,ecx` tail vs retail settling in EAX).
- status: open-residual — probe: none

### B12. Byte-width parameter re-read vs dword load; recompute vs spill-reload
`hero.cpp:1906` 98.4 (retail loads only the LOW BYTE for four mask tests and
re-reads on the else path); `hero.cpp:441` 96.5; `town.cpp:840` 81.2 (retail
re-reads the short parameter for both `cmp ax` tests). Recompute family:
`recruit:248` 90.8 (retail recomputes 29*monsterType at all three sites,
zero live in edx; ours materialises once, spills, reloads); `recruit:47` 86.0
(retail parks the product in ECX, `mov ecx,7` one slot later; 8 spellings
measured, post-increment-in-subscript 64.0).
- status: open-residual — probe: none

### B13. Rematerialization vs CSE of a global/member — the cached-member-local lever
Retail re-loads where we cache: `findpath.cpp:361` 88.0 (`gpGame` ×3),
`ai_tactical.cpp:386` 84.4, `misc.cpp:904` 91.2. **The lever (closed two
functions):** `get_speed_value` 82.4 → 100.0 by DELETING `long odds =
params.odds;` — the local creates a fourth long-lived pseudo, `this` loses
its callee-saved register, every `params.` reference becomes reload-then-
index. GENERAL SHAPE: a member cached in a local is not free; retail
re-loading a member repeatedly is evidence the source did not cache it.
`get_defense_boost_value` 96.2 → 100.0 by deleting BOTH `odds` and `index`
(dropping only one leaves 96.15). Counter-case (not monotone):
`ai_player.cpp:74` — naming `gpGame` loses from every position tried (89.3 /
85.8 / 85.8 / 93.0 / 83.5).
- evidence: `src/ai_tactical.cpp:1756-1767`, `src/ai_tactical.cpp:1295-1302`
- status: explained-lever with counter-case
- probe: none (the pseudo-crowding needs ≥4 long-lived candidates + member
  traffic of those bodies; a minimal TU leaves spare callee-saveds and the
  lever has nothing to crowd)

### B14. NAMING a value to steer pseudo-creation order → register preference
`ai_tactical` SpellCastWorkChance family — one naming closed eight functions:
`long creature_cast = creature_spell != 0;` bound to a name is created before
`side`'s pseudo and takes VC6's first-preference EAX (`xor eax,eax / setne al
/ push eax` at all eight sites); left inline it falls to DL/CL. Closed:
`get_damage_value` 93.2, `get_disease_value` 93.9, `get_misfortune_value`/
`get_blind_value` 96.6, `get_disruptive_ray_value` 97.7, `get_sorrow_value`
99.5, `get_forgetfulness_value`, `get_curse_value`. Counter-cases where naming
does not reach: `get_cure_value` 95.5 (un-naming identical at 95.4886),
`get_simple_attack_effect` 99.03, `get_attack_skill_value`, `ai_player` point
stores.
- evidence: `src/ai_tactical.cpp:1380-1396`
- status: explained-lever; model rule "pseudo creation order = argument evaluation order unless a name hoists it"
- probe: none — **two reductions measured 2026-08-09 and rejected**: a
  free-function shape (flag + `side` + 3-arg fastcall sink) and a
  member-function shape mirroring `get_damage_value` both compile the named
  and unnamed spellings to IDENTICAL bodies (the small TU's scheduler
  converges them; in the member shape both normalize in EAX). The lever is a
  tie-break that only bites under the original TU's pseudo pressure —
  consistent with the corpus's own counter-cases. Orbit-local negative (E11).

### B15. Type-dependent homing eagerness — the char-local rule
`font.cpp:139-142`: naming the scanned character costs 10 points — `char c =
str[pos];` gets a stack home and a reload at every use (VC6 homes char locals
far more eagerly than ints); writing `str[pos]` at each use keeps the load in
`al`. 86.9 → 96.3 from that change alone; repeated at `font.cpp:317-320`
(load stays in DL + drops the redundant null re-test).
- status: explained-lever
- probe: none — **three reductions measured 2026-08-09 and rejected**: leaf
  compare/index shape, a 3-exit scan loop, and a scan loop with calls all
  keep the named `char` in a byte register (AL/BL) with the two spellings
  compiling identically. The eager homing needs the font TU's register
  pressure; orbit-local negative (E11).

### B16. Post-RA scheduling (identical byte multiset, transposed instructions)
`hero.cpp:1214` 98.0 (`cmp eax,-1` sunk one slot; the equipped loop's
identical copy already matches — scheduler, not source); `initialize.cpp:296`
93.9 (memcpy setup lea hoisted two slots); `town.cpp:901` 81.2 (prologue
order); `ai_player.cpp:74` 98.8 (one slot); `town.cpp:108` 99.8 (bonus copied
through `ax` vs `dx`).
- status: open-residual (scheduler model)
- probe: none (requires retail contrast; multiset-identical by definition)

### B17. Encoding-length feedback into allocation — `strip::DrawOwner`, 93.37%
Exactly 1 byte short of retail: our compile picks EAX for the base (`A1`
form, 5 B) instead of retail's EDX (`8B 15`, 6 B); EAX still holds `frame`
until the index chain's last `sub`, so the load sinks below it, the closing
broadcast schedule matches the pos==0 arm's, and the cross-jumper fires. The
pos==0 twin escapes only because its extra `codeY = 122` store keeps EAX live
across the base load, so EDX gets picked there. 20 spellings + all 24
preamble permutations measured.
- evidence: `src/strip.cpp:121-166`
- status: OPEN, fully characterised — the best single end-to-end model test case in the tree
- probe: none (the phenomenon IS the 1-byte divergence vs retail)

### B18. Minimal encoding tie-breaks
`hero.cpp:2162` 99.5833 (commutative scale-1 SIB base/index swap;
pointer-arithmetic spelling byte-identical); `ai.cpp:1059` 98.5 (one
`and eax,0xff` retail keeps, six spellings all identical at 98.4697);
`ai_tactical:2096` 96.4 (byte-narrowed bit-26 test reproducible only at a
net −5.5; folded form kept as documented divergence).
- status: open-residual — probe: none

### B19. Strength reduction / induction-base choice
`smackmgr:430` (retail bases the rect pointer at `.Top`, ours `.Left`;
indexed `FrameRects[i]` bought 85.1 → 89.4); `set_melee_attacks`
(subscripting 69.70 — only the pointer spelling reproduces retail's
strength-reduced loops); `armygrp::Merge` 79.54 (retail keeps distinct
type/troop walkers, frame 0x8c vs 0x88; ours coalesces).
- status: open-residual — probe: none

### B20. Address reassociation with no source handle — `smackmgr:78`, 88.9%
Retail adds `(Pitch*gBinkY + 2*gBinkX)` first and the map pointer last; our
CL reassociates EVERY spelling into `(Pitch*gBinkY + map) + scaled-2*gBinkX`.
Tried and rejected, all compiling identical: term orders, `&map[..]`,
integer-cast arithmetic, `x+x`.
- status: OPEN, no source handle — probe: none (that is the finding)

### B21. Dinkumware `min`/`max` reference shape as an allocation constraint
`cmbtmgr::MaxOf`: retail's max arm homes both operands to the frame, selects
an ADDRESS with lea/lea and dereferences — what a const-ref-in/const-ref-out
select inlines to and neither a by-value helper nor an inline ternary
produces (both enregister; 93.3%). Same shape in `ai_tactical` (`lea &a / jl
/ lea &b / mov [eax]` = `_cpp_min`) and `ai_player.cpp:37`.
- evidence: `src/cmbtmgr.cpp:771-777`
- status: explained-lever
- probe: `b21_minmax_ref_home.cpp` (PASS — both register parameters homed
  before the compare, each arm materializes the winner's address with `lea`
  and loads through it)

### B22. Aliasing obligations that are semantics, not allocation
`town::Deallocate` (char store aliases the char count → cannot stay in a
register; the entry test still uses the pre-loop hoist);
`cmbtmgr::RemoveArmyFromGrid` (byte store may alias the int behind `a` — only
the second block's register-local index folds its address);
`inputmgr::MouseMessageHandler` (`bufferBusy = 1` kills the `gpInputManager`
CSE; the guard chain above shares one load); `findpath.cpp:689` (may alias
the pointer member → base not kept).
- status: explained (the model needs an invalidation rule)
- probe: `b22_store_invalidation.cpp` (PASS — repeated pointer-derived load
  CSEs to one load when nothing intervenes; ANY intervening store forces the
  reload. Measured addendum from the reduction: a store to an *unrelated
  named int global* also invalidates the char-field CSE — VC6's invalidation
  is address-blind conservative, no type-based refinement)

### B23. `volatile` as a modelling instrument — contested across the two trees
homm3, byte-justified: `do_aftermath` `volatile unsigned char surrendered`
load-bearing 75.41 → 79.73; `ai_player` volatile pointer homes recover two
memory reads + town spill; `mousemgr:256` volatile parameters bounded at
≤94.1675. homm2, rejected doctrine: volatile `pendingSkip` added a block,
changed flow, expanded beyond retail — a structural contradiction
(`docs/msvc42-optimized-nonlocal-islands.md:71-80`).
- status: FLAG for the model — the trees disagree on when a homing artifact licenses `volatile`
- probe: none (doctrinal conflict, not a single reproducible codegen fact)

---

## C. Optimizer state sensitivity

### C1. Include-set / type-table population sensitivity — `initialize_game_data` (THE flagship)
A function's exactness depends on the COUNT of user-defined type definitions
and members visible in its TU, with no semantic change anywhere. Quantitative
record (byte-proven 2026-08-07/08, `config/match_baseline.tsv` ~741–927,
`src/initialize.cpp:20-60`):
- One unused `struct probe_t { int a; };`: 100.0 → 96.09. An unused `enum` the same.
- 0..8-struct sweep: 100.0 / 96.09 / 96.09 / 26.18 / 97.04 / 94.07 / 100.0 /
  100.0 / 96.09 — NON-MONOTONIC.
- Post-terrain.h town.h 0..10 sweep: 100/100/94.07×3/100×4/94.07/94.07.
- Does NOT move it: blank lines, comments, typedefs, `extern int` (200-extern
  probe), bare forward declarations.
- Member population counts too: splitting `flags_00_11 : 12` into three named
  bitfields reads 26.1806 (bitfield deliberately not sliced).
- Enumerator POSITION irrelevant, COUNT matters (tail vs middle both 97.0370);
  `CREATURE_NOMAD` alone 96.09 → 26.18; +10 enumerators 97.04; +8 100.00;
  three `ESpellId` enumerators 96.09 → 90.16. Non-monotonic in both
  directions — MEASURE, DO NOT REASON.
- Effects do not ADD: two change-sets each reaching 100.0000 alone COMPOSED
  read 94.0741.
- Values shown by this one row with no semantic change: 26.18, 90.16, 94.07,
  96.09, 97.04, 100.00.
- The delta is real codegen, not symbolization (unmasked `disasm --base`
  proof): copy 0 of `create_requirement_masks` byte-identical; copies 1/2
  differ (direct `mov [8*eax + gHierarchyMask+0x160], ebx` vs hoisted lea).
- Lever-header lottery (18 candidates, k=0..10): `<stdio.h>` first gives 7 of
  11 hundreds; 17 others 2–4; the honest include list 26.18 at k=0.
- A genuine model fix existed within the class: retail includes terrain.h (DC
  CodeView: funclets `$E439..$E467` at terrain.h:70-79; in 75 of the DC game
  compilands). Modelling it took 94.07 → 100.0000 honestly, ten funclets
  byte-identical to `0x4ebd10..0x4ec0de` + the `ctype<wchar_t>::id` guard.
  But the class did not close — the dummy-struct sweep still moves the row.
- status: OPEN — best-characterised phenomenon in the corpus. SKILL.md:141-156:
  no local spelling can fix this; suspect it when a header change moves an
  unrelated TU's exact function.
- probe: none (needs image-level context BY DEFINITION — the observable is
  exactness against retail as a function of the TU's include closure; a
  standalone TU has no retail reference and no honest closure to perturb)

### C2. Cross-TU coupling of the same state
Keeping the reconstruction-only dialog type source-private preserves
`initialize_game_data`'s exact optimizer population — a type's VISIBILITY,
not its use, is the input.
- evidence: `src/armygrp.cpp:192-193`
- status: explained consequence of C1 — probe: none (same reason)

### C3. Statement-order effects (semantically inert), measured sweeps
| Site | Sweep | Result |
|---|---|---|
| `town.cpp:108` | 4 orders of {creature, bonus, dwelling} | 99.78 / 96.84 / 89.16 / 84.98 / 84.98 |
| `ai_player.cpp:74` | all 36 declaration × assignment orders | 98.8 ceiling; plain byte assignment 86.9 |
| `recruit:455/497` | ctor store orders | `thisHero = _thisHero;` first reproduces retail's order but hoists the MonType2 load/store out of place → 95.8 (local-optimum trap) |
| `strip.cpp:138-151` | 20 spellings + 24 preamble permutations | 93.37 ceiling; 78.11 / 74.87 / 73.55 / 73.40; refutes "two stores give the scheduler slack" |
| `misc.cpp:817` | 5 spellings × 3 slots, then 9 permutations of four stores | 72.18 / 69.10 / 69.10; peak 72.18 |
| `ai_tactical:2467` | chained clear vs below-memsets vs subscripting | 87.73 / 84.86 / 69.70 |
- status: open-residual (the sweeps are the current instrument)
- probe: none (deltas are against retail bytes)

### C4. Chained-assignment order
`a = b = 0` stores b FIRST (window rect left/top); the reverse store order is
VC6's source fingerprint for the chained spelling (`src/inputmgr.cpp:125-127`);
findpath's `water_walking = flying = -1` transcribed from retail's bytes.
- evidence: SKILL.md:123-124
- status: explained-lever
- probe: `c04_chained_assign_order.cpp` (PASS — one `xor eax,eax`, `+4`
  member stored before `+0`)

### C5. Construction phase is observable in the store schedule
`soundManager::soundManager` EXACT: `MP3Playing` in the member-initializer
list emits its byte clear BEFORE the derived vptr store; in the body it
schedules after and plateaus at 99.0312. `inputManager::inputManager` EXACT:
the 64-element clear before the vptr proves nontrivial member construction.
- evidence: `src/soundmgr.cpp:157-160`, `src/inputmgr.cpp:201-205`
- status: explained-lever
- probe: `c05_ctor_init_schedule.cpp` (PASS — init-list clear lands before
  the vptr install; body assignment after)

### C6. Aggregate initializer ≠ member-by-member
`armygrp::SplitArmy` EXACT (member-by-member in store order; the equivalent
aggregate initializer changes the store schedule and does not match);
`ai_player.cpp:78-80` (aggregate scores 89.3 but is structurally wrong —
retail read-modify-writes BOTH storage units, which an aggregate initialiser
never does).
- evidence: `src/armygrp.cpp:289-291`
- status: explained-lever
- probe: none (not yet reduced; a bitfield-adjacent RMW shape is a candidate
  future probe)

### C7. Import-declaration form is per-TU state
`call [__imp__X]` needs a dllimport declaration; `call _X@N` thunk form needs
a plain declaration. The form is PER-TU (`timeGetTime`: kbwin IAT vs
button/misc/mousemgr thunk); a plain redeclaration downgrades dllimport and
dllimport-after-plain loses; declare file-locally where the thunk form is
needed.
- evidence: SKILL.md:115-119; `src/mousemgr.cpp:579-580`
- status: explained-lever
- probe: `c07_import_decl_form.cpp` (PASS — both forms side by side in one
  TU: `call DWORD PTR __imp__timeGetTime@0` vs `call _timeKillEvent@4`)

### C8. Cross-family confirmation from homm2 (MSVC 4.2) — same class, better instrumented
From `docs/patterns/msvc42-tu-declaration-state.md` (+ raw `.tsv`) and
`docs/msvc42-optimized-nonlocal-islands.md`: an unused header-level `typedef
enum` changed 22 raw bytes of `font::GetCharacterWidth` (same size/frame/CFG/
relocs) and 11/3/2 bytes in three LATER functions while four earlier stayed
identical → a cumulative state consumed at particular later lowering
decisions. Unused one-member struct: icon2bc 1368 → 1372 B; typedef enum:
1368/89 → 1390/92, WIDGET `Main` 752 → 756; plain typedef alias neutral;
unused member-function declaration changed /O2 and /Od TUs; supplying the
in-class body changed further functions though never called. Exact
predecessor bytes do not imply neutral predecessor state (six-parameter copy
left 90 ctor bytes exact, changed later `Main` by 473 raw positions).
`__FILE__` path changed a string reloc + two raw bytes elsewhere. Include
presence and transitive surface matter; order only when it changes the parsed
surface. /O2 TU-cumulative register steering (`bitmap::CopyTo` exact alone,
89.59155 combined, restored by removing two redundant aliases). /Od TU-global
commutative parity flipped by bodying SIBLING functions
(`game::GetNumThievesGuilds` 92 → 100, `philAI::SetupRelativeHeroStrengths`
98.9 → 100, `ExperienceValueOfStack` 96 → 99). Non-local prologue effects
(`FlipIconToBitmap`). Negative results are orbit-local.
- status: cross-family evidence for C1's mechanism (different compiler)
- probe: none (MSVC 4.2 is outside this pinned toolchain)

### C9. Measurement hygiene the corpus depends on (model-training caveats)
objdiff fuzzy gives partial credit for a differing displacement (a 97%
function can have every local mis-slotted); masked diffs hide immediates (the
IDC_ARROW and 0x54cc bugs); reloc-name-only rows on data are cosmetic; scores
against a broken build are void; MAX is the only ledger.
- evidence: homm2 `docs/patterns/INDEX.md:12-15`; SKILL.md:69-71
- status: doctrine
- probe: none (not a compiler behavior)

---

## D. Control flow / other

### D1. goto non-rotation and non-LICM (the stated lever)
A top-tested loop with ONE call site and no import hoist = retail source used
`goto`; while/for(;;)+break get rotated (duplicated condition) AND win the
import an LICM hoist (`kbwin::Process1WindowsMessage`). "VC6 does not rotate
or LICM goto flow."
- evidence: SKILL.md:88-91
- status: explained-lever, but REFINED by D2/D11 — the form, not the keyword, decides
- probe: `d02_loop_rotation.cpp` (PASS — the rotation half: `while (g)` gets
  the duplicated guard, two compare sites; see D2 for the goto caveat)

### D2. The loop-form lever is form-specific; the goto transcription is NOT always the unrotated one
`smackmgr::VideoPlay` 77.7 → 87.2, 27/27 branches: the wait loop MUST be
`while (1) { if (gSmackVideo == 0) break; … }`. Every other spelling —
`while (g)`, `for(;;)+if-break`, `do{}while(1)`, the explicit goto
transcription — lets VC6 rotate, duplicating the guard after VideoDrawRects
and jump-threading the VideoNeedsUpdate arms to the epilogue. Retail's loop
is unrotated: five back edges land on the single top test, which is why the
loop-invariant `xor ebx,ebx` sits inside the header.
- evidence: `src/smackmgr.cpp:125-137`
- status: explained-lever that contradicts the naive reading of D1
- probe: `d02_loop_rotation.cpp` (PASS — `while(1)+break` stays top-tested
  with an unconditional `jmp` back edge; `while(g)` rotates) and
  `d03_goto_loop_duplication.cpp` (PASS — the literal `retry:` goto
  transcription gets its whole body TAIL-DUPLICATED/peeled, two copies of the
  stores and the call, while the equivalent `while(1)` form stays single-copy:
  direct standalone proof the goto spelling is not the unrotated one)

### D3. LICM legality forces a duplicated guard, then jump-threading removes ours — `VideoClose`, 95.9%
Retail has THREE test sites, we have two: the entry guard before the hoisted
`mov esi,[__imp__BinkPause]` (the duplicated guard making LICM legal), a real
top test, and the back edge tail-duplicated through it. Every spelling
collapses sites (a) and (b): while+if, if+do-while, while+continue, the
literal goto transcription, and that transcription inside an explicit outer
`if (n != 0)` all produce identical objects — VC6 threads the top test away
regardless, having just proved the value nonzero from `dec eax`. NOT
source-addressable. The `while(1){if(n==0)break;}` form that unrotated
VideoPlay does not help (88.3%).
- evidence: `src/smackmgr.cpp:250-275`
- status: OPEN — cleanest LICM + jump-threading interaction in the corpus
- probe: none (the phenomenon is retail's extra guard our CL provably cannot
  be spelled into; five measured spellings collapse identically)

### D4. Merged-return blocks — SOLVED as a source shape
`path.cpp` head, TU 75.97 → 92.13%, 1/8 → 6/8 exact in one edit:
```c
if (index < 0)  goto off_grid;
if (index >= 187)
off_grid:       return <fail>;
```
emits retail's `test/jl A; cmp/jl B; A: <fail-block>; B:` exactly — guard 1
jumps into the block, guard 2 falls into it, the block sits BETWEEN the
guards' flow instead of duplicated per guard (split ifs, +4 B each) or sunk
to the end (`||`, `&&`+goto, `!(a && b)`, goto-after-block: all four
re-thread to the sunk form, ~11 points worse). Closed `FindPath`,
`GetAdjacentCellIndex`, `GetAdjacentCellIndexNoArmy`, `get_adjacent_hex`;
same trick for `ValidAttack`'s non-return merged block.
- evidence: `src/path.cpp:6-32`; SKILL.md:83-87 (still filed there as a residual class)
- status: explained-lever
- probe: `d04_merged_return_goto.cpp` (PASS — the exact layout: `jl` into
  the fail block, second guard falls in, one `or eax,-1`, two rets)

### D5. DUP-EXIT: our CL duplicates epilogues at split exits where retail merges
`town::check_shipyard_square` (nested ifs with ONE `return 0` = retail's 8
branches / 2 rets; the flat seven-`return 0` chain assembles byte-for-byte in
the body but duplicates the five-instruction epilogue at every gate: 8 rets,
17.0). `town.cpp:840` 81.2 (nested beats flat 64.2 and explicit-tail 62.6);
`soundmgr::StartMP3` (negated outer `if` with the Leave textually last closed
the duplication AND flipped the tail-merge in one step); `button.cpp:125`
67.4 → 88.1 (two exits BREAK to one shared return; own-returns expand the
inlined deselect twice, +8.4); `ai_combat::take_damage` (early return
instead of else = 97.06).
- status: explained-lever
- probe: `d05_dup_exit.cpp` (PASS — flat chain: 4 rets with the final gate
  folded branchless neg/sbb/neg; nested single-`return 0`: 2 rets, one merged
  fail exit)

### D6. The opposite direction: retail duplicates where we merge
`ai_tactical` 75.5 → 95.9 (three guards goto INTO the third one's body; `||`
sinks the value/10 block); `armygrp::TSplitWindow::WindowHandler` EXACT
(the source-false duplicated end-dialog spelling was only a 99.917 local
maximum; Dreamcast's two state values, one shared slider update, and positive
changed-hover scope with its own return make VC6 select retail's duplicated
tail layout); `armygrp::modify_spell_damage` EXACT
(mutate in each arm + break to shared return makes VC6 duplicate precisely
retail's epilogues); `adventureoptionswindow::WindowHandler` EXACT (99.9367
duplicated the source tail and looked like a mouseX register wall; Dreamcast's
explicit exit state plus one shared tail selects retail's register staging);
`town::get_legion_bonus` 81.7 OPEN (retail's FOURTH exit
carries a dead `xor eax,eax` no spelling reproduces — an allocator tie-break,
not statement order; sweeps 73.87 / 81.07 / 73.87 / 81.73, four source
returns re-merged to three).
- status: mixed (`WindowHandler` CLOSED 2026-08-28; get_legion_bonus OPEN)
- probe: none (each shape is a retail-contrast; the duplication our CL
  performs is covered by d05)

### D7. Cross-jumping we perform and retail does not
`strip::DrawOwner` (inline portrait-name read lets VC6 cross-jump the closing
broadcast into the pos==0 arm's, 78.11; a local breaks the merge but sinks
the codeX store, 93.37 — pinned between two defects). Matched counterpart:
`File::Open` (retail cross-jumps the two `CreateFileA` calls into one site,
`src/winfile.cpp:99`).
- status: open-residual
- probe: none (needs the two-arm context + retail contrast)

### D8. Ternary as a CFG/merge instrument — the three-operand selector
`ai_combat::get_total` 83.6 → 100.0: the `?:` on the return expression merges
both arms into one pseudo, homed in edx, copied out with the closing
`mov eax,edx` retail has; on the null path the merged pseudo is already the
zero `_M_start`, so no `xor eax,eax` either — both deltas one cause.
`ai_tactical` war-machine tail: only the merged pseudo puts the /5 quotient
in edx. Counter-case: doubly-nested ternary catastrophic (75.5).
`misc::TPickANumber`: retail clamps with a BRANCH — an if on a copy, not a
ternary (ternary compiles to branchless `setl/dec/and`). SKILL: small-int
ternaries → `sete/sbb`; `(dir+3)%6` signed → `cdq/idiv`.
- evidence: `src/ai_combat.cpp:1364-1383`
- status: explained-lever
- probe: `d08_ternary_selector.cpp` (PASS — ternary: `sub edx,ecx / sar
  edx,2 / mov eax,edx` EDX-homed merge; if-spelling: subtract targets EAX
  directly. The no-`xor` half is context-bound to the inlined `_M_start` and
  not asserted standalone)

### D9. Switch lowering — emission order is source order
`armygrp::GetArmyMorale` 80.56 → 96.56 (named no-op exits keep all nine town
values visible, producing retail's two compressed byte-selector tables; empty
`break` cases fold into default too early); `button::Main` +8.9 (WIDGET
sub-switch in retail's emission order, not ascending); `modify_spell_damage`
EXACT (Air, Fire, Earth, Water, then the Golems); `army::ValidAttack`
(`facing` is a real `switch` — the `sub ecx,0 / dec ecx` chain — which lays
DEFENDER first, no if/else ordering reproduces it: 69.3 / 74.5);
`inputmgr::KeyToASCII` (case bodies in KEYBOARD ROW order = source order);
`armygrp:1217` (VC6 cross-jumps the shared inc/dec bodies);
`SiegeMonsterToSiegeArtifact` inlined = "the jump table at 0x5504d4".
- status: explained-lever
- probe: `d09_switch_source_order.cpp` (PASS — bodies emitted 3333, 1111,
  2222, 4444 in source order under a 4-entry value-mapped jump table)

### D10. Loop induction and index-type forms
`ai_combat` 89.8 → 100.0: the Dismiss loop's index is a SHORT consumed 32-bit
— VC6 carries the trip count separately (`mov edi,7 / dec edi / jne` beside
`inc esi`), and needing edi in the first arm stops the `push edi`
shrink-wrap, which fixes the monsters-row add order — one declaration, three
deltas. Alternatives measured: long `!=` 93.37, long `<` 92.02, int 93.37,
unsigned 93.37, `while(i != N)` 90.79, `do{}while(--left)` 98.65, descending
96.57. `get_area_effect_value`: only `for (long i = targets.size(); i-- > 0;)`
tests the raw count signed AND latches on the pre-decrement value.
`armyGroup::armyGroup` RETAIL-EXACT: `i < 7` first sinks that half to the
back edge. Unsignedness propagates from `size_type` (unsigned divide + `jae`).
`font::LongestWrappedLineWidth`: the null test must be the rotated loop
condition; a second condition duplicates at the bottom (87.5).
- status: explained-lever family
- probe: `d10_short_induction.cpp` (PASS — the core short-index rule:
  separate down-counter + `inc`, no `cmp`; int twin compares the index, no
  `dec`. The shrink-wrap and latch-form corollaries stay in-tree)

### D11. LICM we perform and retail does not
`iconwdgt.cpp:248-256`: our CL hoists the whole loop-invariant table
initialisation into the preheader (hence the literal 2 needed once → volatile
EAX, `this` keeps EDI, 0x24-vs-0x20 frame); rewriting the re-roll as a `goto
retry` bottom-tested loop is byte-identical — VC6 recognises the same natural
loop and still hoists (direct counterexample to "no LICM for goto flow").
`town.cpp:903` (the LICM'd `active` pair).
- evidence: `src/iconwdgt.cpp:248-256`
- status: OPEN
- probe: none — **two reductions measured 2026-08-09 and rejected**: with a
  dead local table DSE deletes the stores outright (constant-folded return);
  with the table live-out (passed to a sink after the loop) our CL keeps the
  invariant stores INSIDE the loop — the hoist needs the iconwdgt loop's
  actual body. Bonus finding while reducing: the goto twin got its body
  peeled, which became probe `d03` (D2)

### D12. Jump threading, elsewhere
`font::DrawBoundedString` family 96.33 → 96.81 (two-break `for(;;)` routes
BOTH exits through the shared `if (pos <= lineStart)` instead of letting VC6
jump-thread the lineStart exit straight to the assignment); also VideoPlay
(rotation causes threading) and VideoClose (D3).
- status: explained-lever (spelling-dependent)
- probe: none (delta is against retail layout)

### D13. Bool materialization
`unsigned char f = expr != 0;` → `setne`+byte; comparing normalized ints →
`neg/sbb/neg`; retail mixing both in one compare = one side was a byte local
(`AppWndProc` fMinimized/fActive). `findpath::Clear` 96.88 → 100.0: the
fly-plane flag as `unsigned char` makes VC6 reuse the register holding fly
and zero-extend after (`setne bl; and ebx,0xff`) instead of clearing scratch
first — and freed the pressure behind the pointer-local's spill, closing both
residuals together. `army::ValidAttack`: positive `if (…) return 1; break;`
per case — the negative form folds the three `return 1` tails into
`setge`/`sete`/neg-sbb-neg, losing exactly three branches.
- evidence: SKILL.md; `src/findpath.cpp` (Clear)
- status: explained-lever
- probe: `d13_bool_materialization.cpp` (PASS — all four idioms: `setne al`
  + byte store for the uchar flag; `xor/test/setne` for an int normalize of
  a register value; `neg/sbb/neg` for `!= 0` of a CALL result (value already
  in EAX); `neg/sbb/inc` for `== 0`)

### D14. Word/byte extraction spellings (four measured forms)
`textntry.cpp:87-97`: `(code & 0xFF00) >> 8` emits the byte-register extract
`xor edx,edx; mov dl,ch` with NO re-widening; byte-narrowed spellings land
the same extract but pay a redundant `and edx,0xff` for the switch's
byte→int promotion (97.5%); `code >> 8` loses the `ch` extract for
`sar ecx,8; and ecx,0xff`; `(code & 0xFFFF) >> 8` for `and/shr`. Also
`inputmgr:24-25`: the Win32 HIWORD spelling keeps retail's full-width
load/shift/mask instead of folding to a byte load.
- status: explained-lever
- probe: `d14_byte_extract_switch.cpp` (PASS — mask-then-shift: `mov al,ch`
  with no `and eax,255`; `(unsigned char)((unsigned short)code >> 8)`: same
  extract + the redundant `and eax,255`. Note the bare `>> 8` distinction
  needed the switch context — with an explicit `& 0xFF` mask both spellings
  converge to the `ch` extract, measured during reduction)

### D15. Local placement forces coalescing decisions
`textntry.cpp:80-85`: the `int code` copy INSIDE the block forces codeX into
a register while leaving `msg` live in edx for the else arm's memory read;
hoisting it above the `if` coalesces msg and codeX into one register and
costs four instructions. `findpath::Clear` 95.85 → 96.88: the destination
must be an assigned pointer local, not the two-return accessor call.
- status: explained-lever
- probe: none (needs the surrounding two-arm consumer shape; not yet reduced)

### D16. Intrinsics
`soundmgr::StartMP3`: both name compares are VC6's INLINE strcmp intrinsic
(two-bytes-per-iteration loop closing `sbb eax,eax; sbb eax,-1`), written as
plain `strcmp` calls. `font.cpp:329` 91.7: retail materialises a fresh
`xor edi,edi` for `count = 0` where ours reuses the strlen intrinsic's zero
in EAX, cascading into a boxWidth reload per iteration. `/Op` disables the FP
intrinsics (`sqrt` → CRT call) — see 0.4.
- status: explained (intrinsic surface); the reuse-vs-fresh zero is B8-adjacent OPEN
- probe: `d16_strcmp_intrinsic.cpp` (PASS — the intrinsic loop with byte
  loads, `add eax,2` stride and the `sbb eax,eax / sbb eax,-1` tail; no
  `call strcmp`. Compiled under the full game profile including /Op,
  corroborating that /Op leaves string intrinsics on)

### D17. STL/library shape as codegen
`get_total()` is VC6's own `_First == 0 ? 0 : _Last - _First` (the null arm
explains the shared `test ecx,ecx`); `monsters.size()` via begin()/end()
fixes two functions and costs six others the `_M_start` CSE (measured twice);
`ai_combat 0x4276c0` is the pinned `<vector>` copy ctor instruction for
instruction; the vector-head byte proof (one byte at +0 = Dinkumware's empty
allocator subobject + three zero dwords → `std::vector<T*>` is 16 bytes);
`armygrp`'s local-static guard bit `0x69385c` + atexit thunk `0x44a4c0` =
VC6's function-local-static idiom; `std::max` written out locally because
VC6's `<algorithm>` does not export `max`.
- status: explained (library identity facts)
- probe: none (needs the Dinkumware headers + retail context; the ternary
  core of `get_total` is probed as D8)

### D18. Dead code retail keeps (transcribe faithfully)
Discarded `GetSpeed()` calls, unused params, dead sprintf args; `ai_tactical:2104`
dead `dec eax`/`inc eax` pair around the loop-entry test; `get_legion_bonus`
dead `xor eax,eax`; `get_total_value` dead 20-B `caster` record;
`set_melee_enemies` loop body loop-INVARIANT in retail (esi never advanced).
- status: transcription doctrine
- probe: none (source-transcription facts, not compiler predictions)

### D19. The uninitialized-slot artifact wall — `misc::TPickANumber`, 72.18%
Retail reads MEMORY (`mov dl,[ebp+0xb]`) while eax already holds `lowBound`;
no well-defined expression can emit that — the byte is the reused upper byte
of the low-param slot, almost certainly an uninitialized bool local shipped
as-is. 5 spellings × 3 slots measured; the literal byte-alias spelling scores
WORSE (72.13) and is rejected on evidence grounds. Flow agrees 4/4.
- status: OPEN — the classic uninit-artifact wall
- probe: none (depends on retail's uninitialized stack garbage; by nature not reproducible)

### D20. EH representation as a residual class (partly a comparison artifact)
`font::~font` 96.6, `button` 88.4, `mousemgr` 99.31, `armygrp` 98.40 (EH
state/addend spellings); `executive::CallManager`: the three try/catch scopes
reproduce the 11-block body closely but VC6 emits the funclets in the base
COFF section — objdiff charges them to the symbol (58.29) vs the EH-free body
kept at 68.37. Corpus fact: 5,125 tiny entries from RVA 0x227240 are VC6
parent-EBP cleanup/dtor funclets (2,621 dtor tail jumps, 2,327 cleanup calls,
80 two-object, 61 vector, 36 guarded).
- evidence: `src/exec.cpp:183-195`; `scripts/homm3/build/normalize_objs.py`;
  `scripts/homm3/build/test_eh_handler_normalization.py`; attempt-1
  `docs/compiler-toolchain.md:116-123`
- status: representation subcase CLOSED 2026-08-28; source/topology cases remain
- probe: the paired normalizer admits only the exact VC6 EH prologue, an
  associative `.text$x`, its final ten-byte handler thunk, and a retail unwind
  owner whose addend equals the measured final-funclet size. It preserves the
  resolved target. The corpus admitted 551 equivalent relocations and left 18
  different cleanup sizes visible; four hermetic controls cover the positive
  form, wrong retail size, malformed thunk, and missing funclet.

### D21. Relocation/decode representation (excluded from the codegen model)
`mousemgr` 94.17 (interleaved POINT +4 reloc vs base+displacement); `recruit`
99.5 (local-label table addends + a boundary decode artifact);
`inputmgr::KeyToASCII` EXACT after the paired-padding correction (two
alignment NOPs); `armygrp::GetMorale` (`ArmyGrpFn_0044A460+0x60` vs `_$E20`
at addend zero — no source statement differs); `hero.cpp:1220` (flat carve
name).
- evidence: `scripts/homm3/build/normalize_objs.py` and
  `scripts/homm3/build/test_equivalent_relocation_normalization.py`
- status: two resolved-address subcases CLOSED 2026-08-28; other decode/name
  classes remain excluded from the codegen model, not waived
- probe: the paired normalizer removes a stripped-target DIR32 only when the
  candidate has no relocation of any type at the same function-relative site
  and its literal equals the generated retail symbol RVA plus addend. It
  rewrites aggregate+addend versus synthesized field-symbol+addend only after
  either a reviewed exact relocation alias proves the candidate aggregate's
  retail base or one unambiguous equal-addend data anchor does, and both forms
  resolve identically. Generated `data_`/`bss_` owner spellings are accepted
  only when their encoded RVA is already in that authority; duplicate COFF
  indices of one name are harmless, while two names at one base fail closed.
  The reviewed `AI_enter_town` sites at retail RVAs `0x12555d` and `0x125698`
  resolve `bitNumber+0x128/+0x160`, removing B18/B33 representation noise.
  Hermetic controls keep wrong literals, candidate relocation sites, missing
  or ambiguous anchors, unknown generated names, and different field addresses
  visible.

### D22. Excluded classes (never claim / never model as code)
cinit-pattern rows (guard byte `0x6abaa0` / atexit / ~95 B ten-iteration
initializers), STL COMDAT tails, compiler-generated scalar-deleting dtors
`??_G` (claimed via `VA_COMPGEN`).
- evidence: SKILL.md:36-45
- status: doctrine — probe: none

---

## E. VC-family cross-checks from homm2 (MSVC 4.2) — separate rule set

Negative/positive controls only; none of these bind the SP3 model directly,
so none carry probes (the pinned toolchain is not MSVC 4.2).

- **E1.** `/Od` frame offsets are a hash of each local's NAME (16-bucket
  per-scope table), NOT declaration order or type — fully reverse-engineered
  (`homm2/core/od_slots.py`, `docs/od-stack-layout.md`). Renaming `pos` → `p`
  changed 28 bytes with size and 13 relocs fixed. *Contrast VC6 /O2: B5
  live-range doubles, B6 declaration-order ints.*
- **E2.** `/Ob1` fingerprint: `jmp $+0` clusters = per-call-site continuation
  jumps of inlined accessors; the accessor's RETURN SHAPE controls the
  addressing mode (row-pointer defers `[x]` with scale; cell-pointer resolves
  early, no scale).
- **E3.** `/G5`: unsigned 16→32 via `AND`, never `MOVZX`; `/QIfdiv` wraps
  every float divide.
- **E4.** Relocation *occurrences* are a codegen output, not a
  source-reference count: `Misc::CycleColors` 95.33 → 99.83, relocs 70/71 →
  71/71, by moving the DEFAULT restore into the DEFAULT arm. Open contrasts:
  `BlitBitmapToScreen`, `FlipIconToBitmapColorTable`, `IconToBitmapYModify`.
- **E5.** Short-local coordinate truncation (`/O2`): a `short` LOCAL
  reproduces `(short)op + op` narrowing; an inline cast forces an int sum.
  `widget::Dim` 38% → 100%.
- **E6.** A signed value cast prevents `/Od` memory-RMW folding:
  `ApplyBattleWinTemps` 69.397% → exact via `static_cast<i32>` in twelve
  clears; a 120-state census isolated the cast as the transition.
- **E7.** Array-decayed-to-pointer: CodeView `PA..` with SIZE > 4 is really
  `T[]`; the array declaration gives direct `mov [g+eax*4],edx`, byte-exact.
- **E8.** Branch-layout polarity: write the fall-through path first; the
  decompiler's inverted polarity was the 22% → 96% fix on resource getters.
- **E9.** `/Od` commutative-operand parity is TU-global; the commuted
  subscript `i[(T*)p]` is the local escape hatch; `OD_STEER` retired as
  persistent steering debt.
- **E10.** Selection-order doctrine: semantics → CFG → topology → relocations
  → size/bytes → fuzzy last; the objectives are independent.
- **E11.** Negative results are orbit-local — two arms compiling identically
  prove only that the tested state erased the distinction. (Applied to THIS
  corpus's probe reductions: B6, B14, B15, D11 record their failed
  standalone reductions under exactly this rule.)

---

## F. Open items the SKILL still lists as residual classes (model targets)

1. **Merged-return / stale-CL-generation** (`path.obj`, `kbwin::AppWndProc`) —
   retail's tail-merge differs from our SP3 CL in BOTH directions; suspected
   earlier-generation objects. Corroborated by the Rich header: 26 C++
   objects at build 8168 vs 145 at 8447 (0.9). OPEN — needs a
   compiler-generation probe (the RTM C2 12.00.8168, Track R, is admitted for
   this). The *source-shape* half is closed as D4 (probed).
2. **Register-homing family** (`smackmgr` VideoPlay/DrawRects, `widget`
   send_message/enable) — retail memory-homes a value our CL promotes (or
   vice versa); order sweeps plateau. → §B2/B3 (unprobeable standalone).
3. **EH-bearing functions** — direct-handler versus last-funclet+size
   representation is closed; genuine state, scope, and funclet-topology
   differences remain source work. → D20.
4. **Include-set sensitivity** — → C1 (image-level by definition).
5. **STLport surface — CLOSED** 2026-08-07 (retail links Dinkumware); bodies
   parked on the old premise are reachable.

Standing doctrine that constrains any model: **document, don't grind past 3-4
real hypotheses**; and after any size-changing edit to a callee, check its
CALLERS' scores, not just its own (SKILL.md:98-100) — the corpus's own
statement that the inliner makes per-function scoring non-compositional
(probed as the a06 expansion-count observable).

---

## Probe corpus index

23 probes, all PASS (2026-08-09, 65 checks). `oracle all --all` is the gate.

| Probe | IDs | Core observable |
|---|---|---|
| `a01_single_site_extern.cpp` | A1 A2 (0.1) | single-site extern inlined, no call; out-of-line copy kept |
| `a02_static_single_site.cpp` | A3 A4 | static single-site body vanishes entirely |
| `a03_deferred_emission.cpp` | A14 | kept body of an inlined static deferred to end of TU |
| `a04_auto_inline_off.cpp` | A11 A12 (instrument) | `auto_inline(off)` restores the call, sibling still inlines |
| `a05_inline_depth_scope.cpp` | A13 | positional `inline_depth(0)` vs (255) in one TU |
| `a06_inline_cost_calibration.cpp` | A6 A7 A9 | source-side cost accounting; sequential budget exhaustion; folded-const negative control (EXPECT-SAME) |
| `b08_zero_cse.cpp` | B8 | one xor'd zero CSE'd across mixed-width stores + memory compares |
| `b09_minus1_signedness.cpp` | B9 | signed chars share `or eax,-1`; unsigned split `mov al,255` |
| `b21_minmax_ref_home.cpp` | B21 | const-ref min homes operands, selects an address, derefs |
| `b22_store_invalidation.cpp` | B22 | any intervening store kills pointer-load CSE; clean CSE without |
| `c04_chained_assign_order.cpp` | C4 | `a = b = 0` stores b first |
| `c05_ctor_init_schedule.cpp` | C5 | init-list clear before vptr; body assignment after |
| `c07_import_decl_form.cpp` | C7 | `__imp__` IAT call vs `_X@N` thunk call in one TU |
| `d02_loop_rotation.cpp` | D1 D2 | `while(g)` rotated; `while(1)+break` top-tested |
| `d03_goto_loop_duplication.cpp` | D2 | literal goto loop transcription gets body-duplicated |
| `d04_merged_return_goto.cpp` | D4 | goto-into-arm merged return, block between the guards |
| `d05_dup_exit.cpp` | D5 | flat return chain 4 rets vs nested 2 rets |
| `d08_ternary_selector.cpp` | D8 | ternary merges to an EDX pseudo + closing `mov eax,edx` |
| `d09_switch_source_order.cpp` | D9 | case bodies in source order under a value-mapped table |
| `d10_short_induction.cpp` | D10 | short index forces a separate down-counted trip count |
| `d13_bool_materialization.cpp` | D13 | setne / xor-test-setne / neg-sbb-neg / neg-sbb-inc |
| `d14_byte_extract_switch.cpp` | D14 | mask-then-shift avoids the re-widening `and eax,255` |
| `d16_strcmp_intrinsic.cpp` | D16 | inline strcmp intrinsic under /Op, `sbb/sbb,-1` tail |

Behaviors judged NOT reducible to a standalone probe fall into three honest
classes: (1) **retail-divergence classes** (B1–B5, B7, B10–B12, B16–B20, A8,
A15–A17, C3, D3, D6, D7, D12, D19) — the phenomenon is a diff against retail
bytes, and a lone TU has no retail side; (2) **TU/image-state classes** (C1,
C2, C8, A5, A10, A18, B13, D15, D17) — the input is the include closure or
the full TU's pressure; (3) **measured failed reductions** (B6, B14, B15,
D11) — reductions were built and compiled 2026-08-09, and the two spellings
converged or the allocator erased the signal; the entries record exactly what
was tried so the negatives stay orbit-local (E11) and nobody re-burns them.
