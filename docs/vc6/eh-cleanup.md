# The EH cleanup transcript — a wall class the three solvers cannot see

*Opened 2026-08-14. Tool: `homm3.vc6._eh`, surfaced by `homm3 vc6 diagnose`
as the `eh signal` line and routed ahead of `predict-inline`.*

## What the signal is

A `/GX` function that owns anything destructible carries a **state variable**
at `[ebp-4]`. `mov [ebp-4], N` says "a throw from the region that follows
enters unwind-map entry N". The ordered list of those immediates is therefore
a **transcript of the function's object lifetimes**, and it sits in `.text` on
both sides — no compile, no model, no inference.

That makes it orthogonal to everything else in this area:

| solver | reads |
|---|---|
| `predict-inline` | the out-of-line CALL multiset |
| `why-branch` | the CFG |
| `why-reg` | register bindings |
| **the EH transcript** | **object lifetimes / which regions can throw** |

None of the first three notices that retail's body opens one **more** cleanup
region than ours — i.e. that retail constructs a temporary we never wrote, or
that a call we constant-fold is a call retail could throw from.

## The rule, byte-measured

**VC6 emits a cleanup chain only for a region that can THROW.** Measured on
`Bitmap816::~Bitmap816` (`src/bitmap816.cpp`), reading `maxState` straight out
of the object's `.xdata$x`:

| TU / class input | `maxState` |
|---|---|
| body `if (map) delete[] map;`, `~TPalette24() throw()` | 3 |
| body emptied | 1 |
| `<new>` included, `~TPalette24() throw()` | 1 |
| `<new>` included, `~TPalette24()` **without** `throw()` | **2 = retail** |

Two independent inputs, each a fact about retail's TU:

1. **Can the body throw?** VC6's `<new>` declares
   `void __cdecl operator delete(void*) _THROW0();`. The implicit declaration
   does not, and **`NEW.H` does not either** — it declares only the placement
   forms. With a throwing `operator delete`, the whole-body region needs the
   full `{p24, p16, base}` chain; with the nothrow one it needs none.
2. **Can a member's destructor throw?** A `throw()` on `~TPalette24` erases the
   region that its own call would otherwise need, collapsing the map to one
   entry.

## Retail publishes the answer — the funclets are data

Do not infer the map; read it. Our side: `.xdata$x` holds `maxState` at +4 and
points at the unwind map, whose entries are `(toState, action)` pairs; the
actions are the `$L…` funclets in `.text$x`, each an
`mov ecx,[ebp-0x10]; add ecx,<off>; jmp ~T` naming the subobject it cleans.
Retail's side: the funclet group sits under `<Class>_<fn>_unwindNN` labels in
`build/gen/symbol_names.csv`, and the second push of the EH prologue is
`<group>+<n>`, the handler thunk just past the last funclet.

Worked example — `Bitmap816`:

```
0x6285a0  8 B   mov ecx,[ebp-0x10]; jmp ~resource        \
0x6285a8  0xb B add ecx,0x34;      jmp ~TPalette16        > CONSTRUCTOR: 3
0x6285b3  0xe B add ecx,0x250;     jmp ~TPalette24       /
0x628600  8 B   mov ecx,[ebp-0x10]; jmp ~resource        \  DESTRUCTOR: 2
0x628608  0xb B add ecx,0x34;      jmp ~TPalette16       /  (no p24 funclet)
```

The destructor's missing 14-byte funclet **is** the `mov [ebp-4],1` vs our
`,2`. One immediate; the cause is two lines of TU input.

## Reading a divergence

`diagnose` reports two kinds.

- **COUNT** — different lengths. Retail-longer means a statement (or a
  throwing call) we never wrote; retail-shorter means an extra lifetime of
  ours, or a callee retail's TU proved nothrow (the `<new>` case above).
- **ORDER** — same multiset, permuted. The unwind-map entries were allocated
  in a different order: a declaration-order or statement-order fact.

The signal is best-effort and deliberately conservative — **three** guards,
and the third one is the one that matters most:

1. no `fs:[0]` prologue ⇒ suppressed (an FP constant halved into the frame
   reads as `mov [ebp-4],0x3ff00000`);
2. any value outside `[-1, 0x1000]` ⇒ suppressed (the slot is a plain local);
3. **the state store's source need not be an immediate.** When VC6 CSEs the
   constant 0 into a register the store becomes `mov [ebp-4],ebx`. Those are
   counted as region boundaries with an OPAQUE value, and when the two sides
   have the same number of stores and either side has an opaque one, the row
   is suppressed rather than judged.

Guards 1 and 2 removed 10 of 25 raw candidates. Guard 3 removed a further 6 —
including a divergence this document's first draft got **wrong** (below).

## Tree-wide, 2026-08-14 (9 rows)

| % | size | kind | row |
|---|---|---|---|
| 97.1049 | 961 | COUNT | `viewarmywindow:??0TViewArmyWindow@@QAE@HHHE@Z` |
| 95.7583 | 399 | ORDER | `campaignbrief:??1TCampaignBrief@@UAE@XZ` |
| 94.3114 | 2260 | COUNT | `bottomviewsubwindow:??0TBottomViewTown` |
| 90.9329 | 1021 | COUNT | `ai_combat:?choose_melee@type_AI_combat_data` |
| 86.8447 | 2248 | COUNT | `quickherowindow:??0TQuickHeroWindow` |
| 74.7874 | 968 | COUNT | `armygrp:?get_luck_description` |
| 67.5649 | 2140 | COUNT | `armygrp:?get_morale_description` |
| 50.4577 | 3778 | COUNT | `game:?Load@game` |
| 27.9354 | 2725 | COUNT | `game:?Save@game` |

Every remaining COUNT row is retail-longer by exactly the pattern the rule
predicts. Three worth naming:

- **`TViewArmyWindow(int,int,int,E)`** — base `[reg,2,3,2,4,2,5,2,6,2]`,
  retail `[0,1,2,3,2,4,2,5,2,6,2]`: retail opens ONE region we do not, and it
  is the second `std::string` MEMBER's construction. Retail CALLS
  `basic_string::basic_string(const allocator&)` at both `+0x6c` and `+0x80`;
  our compile calls the first and **inlines the second**. That is what
  `budget ÷ sites-remaining` predicts, since the quotient GROWS along the site
  list, so the later of two identical sites is the one that gets expanded.
- **`TBottomViewTown`** — retail's transcript carries a state `15` between our
  `14` and `16`: one extra region, i.e. one construction we do not make. That
  is the same deficit the site-count work measured as "+3", located exactly.
- **`TQuickHeroWindow`** — retail has an extra `14` reset (`…,15,14,16,14,0`
  against our `…,15,16,14,0`): a throwing statement between two regions that
  our compile expanded away.

## Correction, same day — `[N,0,N+1,0,…]` is NOT a cleanup-count divergence

The first draft of this document named a second recurring shape:
`[N,0,N+1,0,…]` on retail against `[N,N+1,…]` on ours, on
`type_combat_sub_window` and (sides reversed) `TCombatOptionsWindow`, and read
it as "the side that emits resets is the side whose intervening statements can
throw". **That reading was an artefact of guard 3 not existing yet.** Both
sides emit exactly the same number of regions; the side that "looked" to be
missing its resets was simply spelling them `mov [ebp-4],ebx` off a CSE'd
zero. With guard 3 both rows drop out of the table entirely, and neither has
any EH divergence at all. The lesson generalises: **an EH state store sourced
from a register is still an EH state store**, and any transcript comparison
that only greps immediates will manufacture COUNT divergences on precisely the
functions where the constant-zero CSE fired.

## Two negative results — do not re-measure these

1. **The body's site count does not reach a member-initialiser expansion.**
   On `TViewArmyWindow(int,int,int,unsigned char)` the row is **97.1049 at
   +0, +1, +2, +3, +5 and +8** probe sites appended to the body, while the
   *sibling* overload in the same TU moves 88.7021 → 87.0343 → 83.1490 —
   so the probes reach the compiler and simply do not reach this decision.
   The implicit member-construction prologue is accounted separately from the
   body's site list.
2. **The constant-zero CSE is downstream of the inline, not a wall of its
   own.** `TViewArmyWindow(int,int,int,E)` shows it directly: inlining the
   second string constructor adds three more zero stores
   (`[esi+0x84]/[esi+0x88]/[esi+0x8c]`), and *that* is what tips VC6 into
   parking 0 in EBX (`xor ebx,ebx`, then `push ebx` / `mov [ebp-4],ebx` /
   `mov [esi+0x94],bl`) where retail writes the immediate every time. This
   one survives the guard-3 correction, because it is read off the CALL
   structure and the store count, not off the state values. It does NOT
   transfer to `type_combat_sub_window` on this evidence — that row's
   transcripts agree once opaque stores are counted, so its recorded
   constant-zero verdict stands unchanged.
