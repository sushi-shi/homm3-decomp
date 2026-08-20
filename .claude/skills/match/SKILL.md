---
name: match
description: "Byte-match HoMM3 functions/TUs against retail HEROES3.EXE - the whole in-tree loop: locate via the DC-roster order-map, claim, reconstruct C++ for VC6 SP3 (/O2 /Ob2 /Oy- /Op /ML /Gr /GX), iterate with homm3 sema diff, ratchet with homm3 build, document residual classes, and judge TU closure. Use when matching a function, reconstructing/mapping/closing a TU, chasing a plateau, or promoting located addresses to claims. Sibling doctrine adapted from homm2/gruntz matcher agents."
---

# match — reconstruct byte-matching HoMM3 TUs (VC6 SP3 /O2)

You write C++ that, compiled with VC6 SP3 `CL.EXE` under wine (per-TU flags in
`config/units.toml`; default game profile `/O2 /Ob2 /Oy- /Op /ML /Gr /GX /D_WINDOWS` - /Op is engine-wide, byte-proven by the AI TUs),
produces code byte-identical to retail `HEROES3.EXE`, verified by objdiff through
`homm3 build`. External sources remain hypotheses until retail-byte evidence proves
them; substantive outcomes are recorded in the port plan's §5 decision log.

## The governing ledger: per-function MAX fuzzy (ratchet)

`config/match_baseline.tsv` records each function's best-observed fuzzy; a drop
below MAX fails the build. **MAX is the only ledger** (gruntz doctrine): current-%
dips from correct changes are acceptable — the user tracks MAX, not simultaneous
exactness. A deliberate lower is a hand edit of the baseline row with a dated
comment (precedent: GetArmyMorale). Promoting a carcass fn renames its row —
DELETE the superseded flat-name row in the same change or the gate reports it
MISSING forever.

**A MAX IS ONLY COMPARABLE WITHIN A DELINK GENERATION** (proven 2026-08-20).
New claims in ANY TU rename symbols in the synth PDB, vostok delinks every
unit against that PDB, so the TARGET half of the comparison moves for units
you never touched. `recruit::Update` fell 90.8376 -> 88.2360 at an
integration with recruit.cpp and all its headers byte-identical; reverting
only mapcell.cpp's two new VA claims and re-delinking returned it to exactly
90.83756, and restoring them returned it to 88.23604. So:
- a cross-unit "regression" after a merge that added claims MAY be this
  rather than a code change — check whether the unit's source and headers
  actually moved before hunting a spelling. But do not assume it: of the
  three observed instances, only one was the delink generation. The other
  two were `recruitUnit::Update` again, both caused by real ungated `game.h`
  additions (a `type_point` member that made a class non-POD, and a nested
  enum), both diagnosed and restored to 90.8376 by gating them. Rule out the
  source FIRST — it is cheap, and it is the more common cause;
- the newer, more-claimed reference is the more ACCURATE one, so the lower
  number is generally the honest one. Accept it with the cause recorded, do
  not chase it;
- conversely, do not bank a max measured against a stale delink. Re-run
  build -> delink -> build before trusting a number you are about to record.

## The loop

1. **Locate** (if unclaimed). The DC CodeView roster is the spine:
   `awk -F',' '$5=="<tu>.obj"' evidence/dreamcast/functions.csv` — the `file`
   column separates real TU rows from header/template attributions. Order-map DC
   line order onto carve rows (`config/retail-functions.tsv`) inside the TU's
   link-order bracket (`evidence/link-order/{units,gaps,functions}.tsv` — STALE
   between regens; recompute with `python -m homm3.analysis.link_order`).
   Corroborate EVERY pairing with body evidence: strings, imports
   (`__imp__X@N` = N arg bytes), claimed-callee names, address-takes (hardest
   proof), vtable stores, size plausibility (SH4→x86 0.3-2.5x). Exhaustive
   order-maps over a segment count as proof (kb.cpp PollSound..oldmain
   precedent). cinit-pattern rows (guard byte 0x6abaa0 / atexit / ~95B
   ten-iteration initializers) and STL COMDAT tails are excluded class - never
   claim them. DC size columns NEVER transfer to claims; sizes come from the
   carve only. **`DC_ONLY(off, cb)` = "evidenced only in the Dreamcast build"
   (va.h:31), NOT "no retail body exists".** Many DC_ONLY rows have real retail
   bodies merely not located yet; promoting one to `VA` on body evidence is
   ORDINARY locate work needing no special approval. The reverse also holds: a
   claim found sitting on an excluded class is WITHDRAWN back to DC_ONLY.
   Arity (`ret N` vs DC parameter count) is the highest-yield screen there is -
   eight consecutive lanes found misattributed claims, most of them this way.
**CLAIMING IS BULK WORK; RECONSTRUCTING IS NOT** (measured 2026-08-20, and
this CORRECTS an entry written earlier the same day that said never to
bulk-promote a roster).

A `VA()` claim may sit INSIDE the `#if 0 // @carcass` block with a `// @stub`
body. The va-claims gate is a line regex, so a carcass claim is still
order-, size- and class-checked without ever being compiled. 54 such claims
already existed across 13 files (ai_tactical 14, advmgr 7, town 7,
victorylossconditions 7) before anyone wrote this down. So once an order-map
is verified, promote the whole roster in one pass - verify every RVA against
`config/retail-functions.tsv` offline first - and let the gate check it.
spells went from 2 claims to 40 in a single commit that way.

The compile clash only bites when you pull a row OUT of the carcass to
reconstruct it, and then it is per-function work: the carcass signature and
the hand-modelled header declaration frequently DISAGREE, and VC6 reports the
clash as a cascade of C2065 `undeclared identifier` on the PARAMETER NAMES -
which reads like a missing include, not a signature mismatch, and the first
error can land 300 lines from the row that caused it.
`combatManager::ComputeSpellDamage`'s carcass row says
`int (SpellID, int, int, const hero*, const hero*, const army*, unsigned
char)` while `cmbtmgr.h:1409` declares `long (SpellID, long, long, ...)`.
Reconcile per function, preferring the header - it is modelled from bytes.

2. **Claim.** `VA(0x004xxxxx, 0xSIZE)  // <evidence-tag>, dc 0x<off>` above the
   declarator; absolute VAs; sizes carve-exact; strictly increasing per file
   (the ORDER gate); keep `// @stub` bodies for located-not-reconstructed.
   Compiler-generated scalar deleting dtors:
   `VA_COMPGEN(0xADDR, 0xSIZE, SCALAR_DELETING_DTOR, <class>)` — the base obj
   already emits `??_G`, the claim alone pairs it. Data: `DATA(0xADDR)` on
   definitions in the owning TU (or nearest consumer with an ownership note);
   string literals/float-pool via `DATA_COMPGEN(0xADDR, name, "value")` — read
   exact bytes from the hash-verified image, never guess. Unclaimed data externs
   still pair (reloc NAMES don't gate the verdict — DoDialog precedent); claims
   are hygiene + future data-phase truth.
3. **Reconstruct.** Decode with `homm3 sema disasm 0x<va>` (`--verbose` bytes+
   relocs, `--blocks` CFG, `--base` your side). Conventions: /Gr = free
   functions fastcall (ecx, edx, stack); members thiscall; WINAPI stdcall.
   Model real types in the owner header (`include/<tu>.h`), pads sliced only
   where bytes prove a field; SIZE() asserts are clang-arm only — VC6 does NOT
   check them, so re-verify stride arithmetic by hand (the 187*0x70 incident).
4. **Build + score.** `homm3 build --fast` for the inner loop. After ANY new
   claim or DATA lands, run `homm3 delink` ONCE (the target side must relearn
   names), then `--fast` again. Scores: filter `build/objdiff/report.json` by
   unit (report addresses are obj-local — count identity, not RVAs).
5. **Iterate.** `homm3 sema diff 0x<va> --asm` (masked; reloc-name-only rows on
   data are cosmetic), `--branches` for the structural signal the masking hides.
   For real deltas compare UNMASKED (`disasm --base` vs `disasm`) — masking
   hides immediates (the IDC_ARROW and 0x54cc bugs were invisible masked).
   Structural versions first (homm2 doctrine): pick the retail-compatible CFG
   family before micro-spelling.
6. **Finish.** Full `homm3 build` exit 0 "ratchet clean" — read the exit code
   UNPIPED (`homm3 build; echo $?`); `build 2>&1 | tail; echo $?` reports the
   pipe's exit and stderr buffering displaces fatal-gate lines to look early. Residual left? Write a
   house-style comment: `// Residual (NN.N%): <delta> - tried and rejected:
   <spellings>.` Never record scores-as-claims in §5 without the ratchet
   agreeing.

## The proven levers (all byte-verified in this tree — try in this order)

- **Adjacent early-out guards**: retail merges `if (a<0) return E; if (a>=N)
  return E;` into one inline block. Our CL: split ifs = closest (duplicated
  inline); `||`/goto spellings get re-threaded into a SUNK shared block —
  strictly worse. Known residual class when retail's merge is unreachable
  (path.obj) — see "compiler-generation" below.
- **goto loops**: a top-tested loop with ONE call site and no import hoist =
  retail source used `goto`; while/for(;;)+break get rotated (duplicated
  condition) AND win the import an LICM hoist
  (kbwin::Process1WindowsMessage). VC6 does not rotate or LICM goto flow.
- **DO NOT CACHE WHAT RETAIL RELOADS — this is worth more than it sounds**
  (mapcell GenerateHeightMap, 2026-08-20: caching a `CObjectType*` across the
  loop measured **68.17 against 96.75 for re-subscripting every iteration**,
  a 29-point penalty, and hoisting the whole subscript above the `memset`
  went further backwards to 64.89). Retail frequently keeps only a byte
  OFFSET live and reloads the container's `_First` each pass. The instinct to
  hoist an invariant load is usually wrong here: read what retail keeps live
  across the back edge and spell that, even when it looks redundant. **The
  loop BOUND obeys the same rule**: `game::GetNumThievesGuilds` caps at 91.89
  with `numTowns` hoisted into a local and reaches EXACT with
  `i < players[iWhichPlayer].numTowns` left in the condition, re-read every
  iteration. Related smaller instances: `get_trigger_cell` loses 5 points assigning `z` before
  `y`; `calc_cell_extra` needs `is_trigger = 1` AFTER `type_value`, not
  before; `PlaceObject` needs `(col & 0xf) | (row << 4)` and not the operands
  reversed — with `|`, operand ORDER decides which half accumulates (96.80
  vs 100.0).
- **A sibling function is the cleanest control for "source or codegen?"**
  readBlackBox's three lists each need an explicit `if (count == 0) clear();
  else { resize(count); <loop> }` — plain `resize(count)` scores 54.65 and
  leaves the CFG one branch short, the guarded form 93.01 with branches
  agreeing 55/55. What PROVED it is source rather than a codegen quirk:
  retail's own `loadBlackBox` reaches the same resize with NO zero test, and
  our plain `resize(count)` reproduces that shape exactly. When two retail
  functions differ, the difference is in their sources.
- **...AND SOMETIMES IT IS NOT A SWITCH AT ALL.** If retail's compares appear
  in an order that is NOT ascending by case value, the source wrote an
  IF-CHAIN, not a `switch` — VC6 sorts a switch's tests ascending and will
  relocate whole arms to do it. advManager::ProcessMapSelect went **77.76 ->
  90.90** on that change alone: retail compares HERO(34), TOWN(98),
  SHIPYARD(87) in that order, which no `switch` can produce. Read the compare
  ORDER before assuming the construct.
- **SCOPED LOCALS SHRINK THE FRAME** (SetHeroContext 90.56 -> 99.27). Retail
  writing three values through ONE stack slot does not mean one variable — it
  can be three separate BLOCK-SCOPED locals whose lifetimes do not overlap.
  A single function-scope local keeps the slot live across an inlined region,
  so that region's own values need fresh slots and the spill cascades. When
  the frame is bigger than retail's, look for values whose scope should end
  early. **The inverse is also real, so measure both directions**: REMOVING
  the brace scope around a shared block's locals took ProcessKeyPress 71.36
  -> 75.65 through register allocation alone, with block layout unchanged.
- **SWITCH ARM ORDER IS SOURCE ORDER — BUT ONLY FOR A JUMP-TABLE SWITCH.**
  Byte-proven 2026-08-20 on advManager::ProcessSelect (84.02 -> 100.0 on the
  edit alone): its four arms were each already byte-correct and it still
  plateaued, because retail lays them out hero/town/map/radar, the reverse of
  widget-id order. Reordering the cases and changing nothing inside them
  closed it. **QUALIFIED the same day, with a control**: for a SPARSE switch
  that compiles to a compare chain rather than a jump table, source order is
  NOT a lever — a control over three exact compare-chain switches
  (`army::AttackWall`, `bitmapBorder::Main`, `kbwin::AppCommand`) shows arm
  bodies emerge in REVERSE of the ascending dispatch order regardless of what
  the source wrote. So: check first whether retail dispatches through a jump
  table (reorder is a lever) or a compare chain (it is not, and the layout
  tells you nothing about source order).
- **A SEPARATE ZERO TEST BEFORE THE SWITCH.** advManager::Main went **64.64
  -> 93.23** and its frame 0x18 -> retail's 0x14 on one restructure: written
  as a single switch over {0, 1, 4, 0x200} VC6 pivots the list, whereas
  retail is `if (msg.id != MESSAGE_NONE) { switch } else { idle }` — a plain
  ascending three-case chain behind an explicit zero test. When a dispatch
  includes a "none" value, try lifting it out of the switch.
- **ZERO-INIT MUST ENUMERATE EVERY MEMBER** (97.42 -> 100.0). For a struct
  local, `= {0}` compiles to one store plus `rep stosd`, and `memset` to a
  bare `rep stosd`. Only the fully written-out `= {0,0,0,0,0,0,0,0}` gives
  retail's individual per-member stores — and only that form keeps the zero
  in a REGISTER, which is what lets it unify with a later `push` feeding
  other call sites. Count retail's stores and match the initializer's arity.
- **A pointer relational compare is UNSIGNED and can never produce `jl`**
  (byte-proven 2026-08-20, closed three hero.obj functions). VC6
  strength-reduces a *signed* `for (int i = 0; i < N; ++i)` walk into pointer
  form while KEEPING the original compare's signedness, so retail ending a
  loop on `cmp ebx,<End> / jl` was written as an INT INDEX, not a pointer
  walk — a `p < end` spelling emits `jb` and cannot match. Change BOTH loops
  of a nested pair in the same edit: converting only the inner one measured
  WORSE (equip_artifact 85.25 -> 75.08) because VC6 re-derived the outer
  bound as an unrelated offset while the outer walk stayed pointer-form.
- **The /Ob2 budget cuts BOTH ways - a local win can cost exact functions
  elsewhere.** Factoring duplicated arms into a shared helper made
  soundmgr's ConvertVolume score better IN ISOLATION but shrank it under
  VC6's inline budget, so it got inlined into MemorySample and ModifySample
  where retail CALLS it - knocking ModifySample off exact (100% -> 57.3%).
  Writing both arms longhand, as retail does, pushed it back over budget and
  restored both callers. `--fast` does not surface this; only the per-function
  scores of the CALLERS do. After any size-changing edit to a callee, check
  its callers' scores, not just its own. The prettier spelling is often wrong.
- **OVER-INLINE: pin the callee retail CALLS. The single highest-yield
  repeated lever found so far** (three functions, two lanes, 2026-08-20).
  When `predict-inline` reports `<callee> base x0 vs retail x1` — we expand
  it, retail calls it — put `#pragma auto_inline(off)` around that callee (or
  a statement-scoped `#pragma inline_depth(0)` at the call). Measured:
  `auto_inline(off)` on three advManager members our CL inlined
  (UpdateScreen, CompleteDraw(uchar), CheckDimNextHeroBut) paid **+7.90 on
  DoAdvCommand and +13.24 on ShowRoute**; `inline_depth(0)` on a 191-byte
  callee took cmbtmgr's place_obstacle **49.09 -> 67.19**. In all three the
  standing residual had blamed registers, and the register story was
  downstream — ROUTE THE INLINER FIRST on any low-scoring function.
  Definition order is NOT the lever: VC6 inlines functions defined LATER in
  the TU, so moving a definition does not control expansion (measured, and it
  refutes a note previously banked in advmgr.cpp).
  **The same wall has a second fix, and on big bodies it is the better one:
  SHRINK THE CALLER.** Our pre-inline caller being larger than retail's is
  what pushes the budget past retail's decisions, so splitting the caller into
  helpers restores them without pinning anything. The hero lane closed four
  plateaus this way in one round — mark_spells 84.29 -> 92.71,
  HeroFn_004E6120 93.41 -> 99.97, WindowHandler 56.54 -> 71.10,
  HeroFn_004D8B30 49.03 -> 55.74 — and for WindowHandler the tree's own
  carcass already named the three helpers to split out
  (`handle_artifact_click`, `handle_backpack_click`, `show_skills`). Check
  the carcass for helper names before inventing your own.
  **BUT THE PIN CAN RANK BACKWARDS, and the mechanism is worth knowing:
  `auto_inline(off)` / `inline_depth(0)` de-inlines EVERYTHING in the
  statements it covers, not just the callee you diagnosed.** Measured on
  cmbtmgr's SetupAndLoadObstacles: `PlaceObstacle base x0 vs retail x1` is
  the identical diagnosis that paid +18.10 on place_obstacle, and the
  identical pragma LOST 5.89 there (64.81 -> 58.92) because it also
  de-inlined the `obstacles.size()` in the same statement; hoisting `size()`
  out first recovers part and is still worse (61.59). **THE MECHANISM IS NOW
  KNOWN AND IT IS A FEATURE: `#pragma inline_depth(0)` IS STATEMENT-GRANULAR
  IN VC6, not function-granular** (game lane, 2026-08-20). So you can place
  it per call site, and that is how the two biggest serializers in the tree
  were broken open: retail keeps every `vector::resize` OUT of line while
  inlining insert/erase INTO it, and we did the exact inverse — pinning the
  eight `resize` sites took `game::Load` **50.46 -> 71.71**, and the same
  lever twice on `game::Save` (`bitset::test`, whose inlined `_Xran` throw
  path drags a `std::out_of_range` onto the frame, plus seven `save_vector`
  sites) took it **42.23 -> 73.80**. Pin the SITE, not the function.
- **/Ob2 single-call-site inlining**: statics AND extern functions with one
  call site inline REGARDLESS OF SIZE (AppInit→WinMain ~300B; AppCommand→
  AppWndProc is our uncracked over-inline residual). Unconditional out-of-line
  emission holds for EXTERN linkage ONLY — inlined single-call STATICS vanish
  (initialize.obj: two creators have no retail bodies). Same-TU helpers:
  write the call, let VC6 inline; inline-cost is calibrated by the callee's
  locals (initialize's cim precedent).
- **Signedness CSE**: -1 stores share one register (`or ecx,-1`, byte stores
  from cl) ONLY when the byte fields are signed/plain char; unsigned char
  splits a separately-materialized 0xff (hexcell ctor — the codegen itself is
  signedness evidence).
- **Bool materialization**: `unsigned char f = expr != 0;` → `setne`+byte;
  comparing normalized ints → `neg/sbb/neg`. Retail mixing both in one compare
  = one side was a byte local (AppWndProc fMinimized/fActive). **The RETURN
  case is the same rule and closes functions** (game's `save_vector` 96.60 ->
  100.0): `return a >= b;` emits `sbb`/`inc`, while assigning to an
  `unsigned char` local first and returning that emits retail's `setae al`.
  If retail sets a byte, the source landed the result in a byte.
- **Import call forms**: `call [__imp__X]` needs a dllimport declaration
  (windows.h family); `call _X@N` thunk form needs a plain declaration. The
  form is PER-TU (timeGetTime: kbwin IAT vs button/misc/mousemgr thunk) — a
  plain redeclaration downgrades dllimport and dllimport-after-plain loses;
  declare file-locally in the TUs that need the thunk form.
- **Ternaries** on small ints → `sete/sbb` idioms; `(dir+3)%6` signed → cdq/idiv.
- **Dead args / stray calls are real**: retail keeps discarded GetSpeed()
  calls, unused params, dead sprintf args — transcribe faithfully.
- **Chained assignment order**: `a = b = 0` stores b first (window rect
  left/top ordering).
- **Statement order from homm2**: when a buka twin exists
  (evidence/homm2-overlap/), adopt its statement ORDER as the starting shape —
  proven closer on the whole basewin family. Preserve its provenance in the
  §5 record.

## Known residual classes (document, don't grind past 3-4 real hypotheses)

- **Merged-return blocks / stale-CL-generation** (path.obj, kbwin AppWndProc):
  retail's tail-merge behavior differs from our SP3 CL in both directions;
  suspected earlier-generation objects in the retail link. OPEN — needs a
  compiler-generation probe. Keep the closest spelling.
- **Register-homing family** (smackmgr VideoPlay/DrawRects, widget
  send_message/enable): retail memory-homes a value our CL promotes (or vice
  versa). Order sweeps plateau; document.
- **EH-bearing functions** — **NOT a wall; this entry was wrong** (corrected
  2026-08-20). It used to read "fs:[0] frames can't close until the synth-PDB
  EH scope lands (P2.2); claim + `// EH-bearing`, body only if cheap", and
  lanes were skipping bodies on that basis. Counter-evidence, byte-proven:
  `THeroScreenWindow::THeroScreenWindow` (0x4de710, 11,346 B) is EH-bearing
  AND exact, and the whole EH-bearing mapcell reader family
  (readArtifactData, readGarrisonData, TTimedEvent::Read, readTimedEventList,
  loadTimedEventList) is exact or near it. Treat an fs:[0] frame as ordinary
  work. Local string/temporary cleanup DOES duplicate per early return in
  retail while our CL sometimes shares one epilogue — that is the
  merged-return class above, not an EH limitation.
- **Include-set sensitivity** (initialize_game_data precedent, byte-proven
  2026-08-08): a function's exactness can depend on the COUNT OF
  USER-DEFINED TYPE DEFINITIONS visible in its TU, with no semantic change
  anywhere. One unused `struct probe_t { int a; };` in a header took
  initialize_game_data 100.0 -> 96.09; a 0..8-struct sweep gave
  100.0/96.09/96.09/26.18/97.04/94.07/100.0/100.0/96.09 - NON-MONOTONIC, so
  it is VC6 optimizer state (type/symbol-table population perturbing CSE and
  addressing-mode choice), not a modelling error. The delta is real codegen:
  retail addressed a row directly (`mov [8*eax + tbl+0x160], ebx`) where our
  CL hoisted the base into a register. Blank lines, comments and typedefs do
  NOT move it. **CORRECTED 2026-08-20 — the trigger is the DECLARATOR COUNT
  C1XX numbers member handles from, and `extern int` DOES move it.** The
  cmbtmgr lane bisected `command.obj`'s GetCommand one edit at a time
  (92.5714 -> 92.5357, three times, restored by gating each): retyping a pad
  IN PLACE is free — three such renames landed un-gated at no cost — while
  adding a declarator, an enumerator, *or a single file-scope `extern int`*
  each costs 0.036. So the old "a 200-extern probe cleared the hypothesis"
  reasoning does not generalize past the header it was run on; count
  declarators, not type definitions, and gate anything you add. NO local
  spelling change can fix this; the match returns when the TU's include
  closure matches retail's (breaking one include edge restored it). When a
  header change moves an unrelated TU's exact function, suspect this before
  hunting a spelling, and re-measure after any include-graph edit.
- **STLport surface** — CLOSED 2026-08-07: retail links Dinkumware (VC6's
  shipped STL), NOT STLport. `#include <bitset>`/`<vector>` from the pinned
  toolchain is all that is needed where real call sites exist. Bodies parked
  on the old premise are reachable; re-read any "STLport-blocked" note.

## TU closure (functions-only, per the standing data-scope decision)

A TU may be declared CLOSED in §5 (zlib/hexcell precedent) only with: every
carved target fn in the freshly-recomputed span claimed AND exact; flanking
gaps attributed (cinit/excluded classes identified, neighbors' rosters
corroborate); the DC roster exhausted — each row located, proven inlined-away
(/Ob2 emission rules), proven retail-dropped (no slot fits), or proven
DC-port-only. Absent methods are documented, never forced. The claims-only
scoreboard is NOT closure evidence (the border illusion).

## Running in a worktree lane

`HOMM3_DIR` is baked into the nix devshell pointing at the MAIN repo, and the
`homm3` CLI resolves every path from it - **cwd is irrelevant**. In a worktree
lane, prefix every invocation with `export HOMM3_DIR=<your worktree>` or you
will silently build, delink and ratchet the main repo (and race whoever owns
it). Re-establish your baseline after fixing it; numbers measured before are
void. Pipeline order is **build -> delink -> build**: delinking against stale
objects makes the target side fall back to flat carve names, after which the
ratchet reports every promoted function MISSING and invents 0.0000 flat rows.

## House rules

- The wine VC6 build is the only verdict; clang diagnostics are editor noise.
- One reviewable unit ≈ one commit; never commit unasked.
- Claim-source-only files (not in units.toml) hold free-standing @stubs; in
  COMPILED units stubs must sit inside `#if 0 // @carcass`.
- Evidence tags on claims: anchor-global / anchor-bracket / anchor-callee /
  anchor-import / anchor-vtable / linkorder / dc-bracket forced /
  corroborates — say which, plus `dc 0x<off>` (or `retail-only`).
- Name lineage is explicit: DC > homm2(buka) > NH3API/IDA, provisional names
  marked; NH3API addresses are NEVER location evidence (wrong address space).
- Pipeline extensions (labels joins, new claim kinds) are contract changes:
  smallest possible diff + §5 entry in the same change.
