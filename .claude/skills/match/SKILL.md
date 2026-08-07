---
name: match
description: Byte-match HoMM3 functions/TUs against retail HEROES3.EXE - the whole in-tree loop: locate via the DC-roster order-map, claim, reconstruct C++ for VC6 SP3 (/O2 /Ob2 /Oy- /ML /Gr /GX), iterate with homm3 sema diff, ratchet with homm3 build, document residual classes, and judge TU closure. Use when matching a function, reconstructing/mapping/closing a TU, chasing a plateau, or promoting located addresses to claims. Sibling doctrine adapted from homm2/gruntz matcher agents.
---

# match — reconstruct byte-matching HoMM3 TUs (VC6 SP3 /O2)

You write C++ that, compiled with VC6 SP3 `CL.EXE` under wine (per-TU flags in
`config/units.toml`; default game profile `/O2 /Ob2 /Oy- /Op /ML /Gr /GX /D_WINDOWS` - /Op is engine-wide, byte-proven by the AI TUs),
produces code byte-identical to retail `HEROES3.EXE`, verified by objdiff through
`homm3 build`. Nothing lands without the supervised-review rule (CLAUDE.md);
approved outcomes are recorded in the port plan's §5 decision log in the same
change.

## The governing ledger: per-function MAX fuzzy (ratchet)

`config/match_baseline.tsv` records each function's best-observed fuzzy; a drop
below MAX fails the build. **MAX is the only ledger** (gruntz doctrine): current-%
dips from correct changes are acceptable — the user tracks MAX, not simultaneous
exactness. A deliberate lower is a hand edit of the baseline row with a dated
comment (precedent: GetArmyMorale). Promoting a carcass fn renames its row —
DELETE the superseded flat-name row in the same change or the gate reports it
MISSING forever.

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
   carve only.
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
6. **Finish.** Full `homm3 build` exit 0 "ratchet clean". Residual left? Write a
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
  = one side was a byte local (AppWndProc fMinimized/fActive).
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
  proven closer on the whole basewin family. Per-function supervised adoption,
  §5-recorded.

## Known residual classes (document, don't grind past 3-4 real hypotheses)

- **Merged-return blocks / stale-CL-generation** (path.obj, kbwin AppWndProc):
  retail's tail-merge behavior differs from our SP3 CL in both directions;
  suspected earlier-generation objects in the retail link. OPEN — needs a
  compiler-generation probe. Keep the closest spelling.
- **Register-homing family** (smackmgr VideoPlay/DrawRects, widget
  send_message/enable): retail memory-homes a value our CL promotes (or vice
  versa). Order sweeps plateau; document.
- **EH-bearing functions** (P2.2): fs:[0] frames can't close until the
  synth-PDB EH scope lands. Claim + `// EH-bearing`, body only if cheap.
- **STLport surface** (armygrp/exec/u2dvers/herodefs): OPEN vendoring
  decision; std::string/vector-shaped bodies and anonymous-namespace TU-hash
  manglings wait.

## TU closure (functions-only, per the standing data-scope decision)

A TU may be declared CLOSED in §5 (zlib/hexcell precedent) only with: every
carved target fn in the freshly-recomputed span claimed AND exact; flanking
gaps attributed (cinit/excluded classes identified, neighbors' rosters
corroborate); the DC roster exhausted — each row located, proven inlined-away
(/Ob2 emission rules), proven retail-dropped (no slot fits), or proven
DC-port-only. Absent methods are documented, never forced. The claims-only
scoreboard is NOT closure evidence (the border illusion).

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
