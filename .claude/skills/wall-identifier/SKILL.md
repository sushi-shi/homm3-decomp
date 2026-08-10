---
name: wall-identifier
description: Identify and route a HoMM3 matching WALL using the vc6 compiler-model tooling. When a reconstructed function plateaus below 100% and no obvious spelling closes it, run `homm3 vc6 diagnose <unit:fn>` to classify the residual (inliner / control-flow / register / masked-cosmetic / include-set) and get the exact solver command + fix lever. Use when a function is stuck, when triaging which plateaus are worth effort, when asked "why won't this match", "what kind of wall is this", "which solver", or when reasoning about register swaps, inline divergence, control-flow shape, or include-set sensitivity. Complements the `match` skill (which reconstructs); this one DIAGNOSES the wall.
---

# wall-identifier — classify a matching wall with the vc6 tooling

The pinned VC6 compiler is a deterministic function: `bytes = f(preprocessed TU,
flags)`. Retail came out of that same `f` from real source, so **every function
is reducible in principle** — the job is finding which *input* differs. The `vc6`
area reverse-engineered `f`'s inliner and register allocator (from `C2.DLL`) and
its front-end IL (from `C1XX.DLL`), so a plateau's residual can be *named* — which
compiler decision diverged, and which source/TU input controls it — instead of
guessed at with a blind spelling sweep.

**Mental model that decides everything below:** the unit of reproduction is the
**whole TU**, not the function. Some residuals are controlled by *TU-global /
front-end state* (symbol-handle order, inline budget) that a local body edit
CANNOT reach — but matching the TU context can. The tools tell you which case
you're in.

## Start here: `homm3 vc6 diagnose <unit:fn>`

One command from a red `sema diff` to a routed answer. It reads the same
base-vs-delinked-target pair the ratchet scores (no recompile), classifies the
residual, routes in doctrine order **inline → control-flow → register**, and
prints the exact solver command(s) to run.

```
homm3 vc6 diagnose 'ai_combat:?do_general_melee@type_AI_combat_data@@QAEXAAV1@@Z'
```

`<unit:fn>` is `unit:mangled-name` (the mangled name from the objdiff report /
`homm3 status`). For a whole-tree triage instead of one function:
`homm3 vc6 report` → `evidence/vc6/plateau-diagnosis.md` (every plateau bucketed
by wall class + the knob to try).

## The wall taxonomy (what `diagnose` reports, and the lever for each)

| class | signal | tool | lever — and is it LOCALLY editable? |
|---|---|---|---|
| **inliner** | out-of-line CALL multiset diverges (a callee inlined on one side only; A8/A9/A12) | `predict-inline` | **budget starvation** — usually NOT a local edit; *finish the caller's body* |
| **control-flow** | CFG/branch shape diverges — block count, ret count, loop rotation, merged-return (D-family) | `why-branch` | loop form / merged-return placement / case order / ternary / bool spelling — often local |
| **register-homing** | schedule aligned, register bindings permuted (ESI/EDI swap), memory-homing, spill (B-family) | `why-reg --model` | pseudo-creation order — local ONLY when the swapped value is a named local; else C1 handle-state |
| **masked-equal** | reg-dist 0 AND flow-dist 0 but objdiff < 100 | — | displacement/reloc only (C9/D21) — **not a real wall**, don't grind |
| **include-set** | a header/type edit moved an *unrelated* function's score | `il-diff` | front-end symbol-handle renumbering — match retail's include closure (TU-global) |

### inliner walls — the dominant class (~65% of plateaus)
`homm3 vc6 predict-inline <src> --fn <caller> --against <unit:caller>` lists which
callees retail inlines but we call (UNDER) or vice-versa (OVER). The `/Ob2` rule
is RE'd and validated (`docs/vc6/inliner.md`): **budget = clamp(2×caller_cb, 1000,
35000)**, spent sequentially per call site (cost = the callee's front-end size
estimate; ≤40 is free), nested expansions get `budget ÷ sites-remaining`. The huge
`basic_string::_Tidy` / `vector` / `new` / `delete` under-inlines are **budget
starvation**: our leaner reconstruction sits at the 1000 floor while retail's
fuller body earns a bigger budget and inlines everything. **FIX: finish the
caller's body — the budget follows statement mass (byte-inert statements count).
Do NOT chase `_Tidy`/`vector` spellings, `#pragma auto_inline`, or headers.** On
low-% functions this class largely self-resolves as reconstruction completes; it
is the pure remaining wall only on high-% rows.

### control-flow walls
`homm3 vc6 why-branch <src> --fn F --against <unit:F>` — guided oracle search over
the D-knobs (goto ↔ while ↔ `for(;;)+break`, split-if vs `||`/`&&` vs
merged-return block placement, switch case-emission order, ternary-vs-if, bool
materialization). `homm3 sema diff <addr> --branches` is the raw structural signal.

### register walls — use the model (v2)
`homm3 vc6 why-reg <src> --fn F --against <unit:F> --model`. The allocator is RE'd
(`docs/vc6/regalloc.md`): one preference order **EAX ECX EDX ESI EDI EBX EBP**,
first-fit per pseudo in **creation order**; call-crossing values get ESI/EDI/EBX
in order (the B1 swap), byte-sized values lose ESI/EDI/EBP (B15 char homing). v2
predicts the exact assignment, names the transposed value, and proposes the ONE
creation-order edit — then confirms in ONE compile. Crucially it also **proves
when the residual is unreachable**: if the value that must move first is
`this`/a parameter/a call result, no local spelling reaches it — it's front-end
**handle-state (C1)**, and the answer is to converge the TU, not edit the body.
`--sweep` falls back to the v1 blind mutation search.

### include-set walls (the C1 flagship)
When adding an unused struct/type or a header edit non-monotonically moves an
*unrelated* function, it is C1XX front-end symbol-handle renumbering delivered
through the IL (proven: `docs/vc6/il-format.md`). Confirm with
`homm3 vc6 il-diff <A.cpp> <B.cpp>` — `IL DIFFERS` ⇒ front-end (match the include
closure); `IL IDENTICAL` ⇒ look at C2 state. There is no local body knob.

## Doctrine (do not violate)

- **The tools PROPOSE, never apply.** Every solver prints a diff for supervised
  application. Nothing lands without the supervised-review rule (CLAUDE.md) and a
  §5 decision-log entry in the same change. Never auto-edit source.
- **In-unit fidelity is built in** — solvers diagnose off the real
  `build/objdiff/base/<unit>.obj` and compile mutants with the unit's exact
  `units.toml` flags, so their distances match the ratchet. Trust them; do not
  re-add `/I<src>` or a hand-rolled profile.
- **The VC6 compile is the sole verdict.** A model prediction is a hypothesis a
  compile settles; why-reg v2 always confirms with one real compile.
- **Don't grind a bounded wall.** `masked-equal`, encoder tie-breaks (SIB
  base/index, B18), and proven handle-state residuals are not source-local — the
  tool saying so in one compile is the *answer*, not a failure. Record it and move
  on (`match` skill: document, don't grind past 3–4 hypotheses).
- **rc convention:** 0 = agrees / exact, 1 = diverges / improved-not-exact,
  2 = error. Every invocation logs to `build/homm3_vc6.log`.

## Supporting verbs

- `homm3 vc6 argv --flags "<cl flags>"` — exact per-pass (C1XX/C2) argv for a
  command line (proves `/Ob2` is front-end-only, `/d2` reaches C2 verbatim).
- `homm3 vc6 oracle all --all` — the behavior-catalog probe corpus (23 probes);
  the model's regression gate.
- `homm3 vc6 check` — the census: behavioral gate (with a negative control) +
  distance-vs-objdiff consistency (informational).
- `homm3 vc6 ab run --fn <mangled>` — RTM-vs-SP3 C2 A/B (Track R). Only for a
  function whose sole residual is a `jb`/`jl`-style generation twin — Track R
  proved this is a tiny tail; the C2 generation explains none of the current
  walls.
- `homm3 vc6 atlas --regen` — regenerate the C2 TU/globals map (rarely needed).

## Reference (read the doc for the class you're in)

`docs/vc6/behavior-catalog.md` (the ~90 byte-verified behaviors, A/B/C/D IDs —
the shared vocabulary) · `inliner.md` (the `/Ob2` rule + address ledger) ·
`regalloc.md` (the preference table + first-fit order) · `il-format.md` (IL
capture + the include-set verdict) · `driver-passes.md` (the CL spec table) ·
`c2-atlas.md` (C2 module RVAs) · `rtm-generation.md` (Track R). The full picture:
`~/.claude/plans/good-now-onto-the-lexical-allen.md`.
