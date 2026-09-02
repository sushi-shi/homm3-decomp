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

The disposable comparison-object pass now canonicalizes this representational
difference. It rewrites VC6's direct handler-label relocation to retail's
last-funclet owner plus size only after proving the associative COMDAT, exact
EH prologue, final ten-byte handler thunk, unchanged resolved target, and an
equal retail addend. A different cleanup size or topology is deliberately not
normalized. See `homm3.build.test_eh_handler_normalization` for the negative
controls.

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
| ~~97.1049~~ **EXACT** | 961 | COUNT | `viewarmywindow:??0TViewArmyWindow@@QAE@HHHE@Z` |
| 95.7583 | 399 | ORDER | `campaignbrief:??1TCampaignBrief@@UAE@XZ` |
| ~~94.3114~~ 97.3638 | 2260 | COUNT | `bottomviewsubwindow:??0TBottomViewTown` |
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

## The signal converted — four rows worked, three landed (2026-08-14)

| row | before | after | what the transcript named |
|---|---|---|---|
| `armygrp:?get_luck_description` | 74.7874 | **82.5689** | a whole extra lifetime: a branch-local `std::string` where retail returns the literal into the NRV |
| `quickherowindow:??0TQuickHeroWindow` | 86.8447 | **90.7834** | an extra reset per if-arm: `push_back(new …)` in each arm, not a shared pointer pushed once |
| `bottomviewsubwindow:??0TBottomViewTown` | 94.3114 | **95.6295** | a state store that moved: copy-initialize the `std::string`, do not default-construct and assign |
| `bottomviewsubwindow:??0TBottomViewTown` (again, off the line table) | 95.6295 | **97.3638** | not an EH finding: `game::GetCurrTown`, see `docs/dc-line-tables.md` |
| `viewarmywindow:??0TViewArmyWindow@@QAE@HHHE@Z` | 97.1049 | **100.0000** | the missing region is one free inline candidate site away — and that site is the post-Dreamcast version gate (below) |

Two of the three landings were **invisible to every other lens**, and that is
the finding worth keeping:

- On `TQuickHeroWindow` the edit changes the object's call multiset by
  **nothing at all** — VC6 tail-merges the two arms' `push_back` sequences
  back down to a single `vector::insert` — so `predict-inline`, a CALL diff
  and the score itself had no way to point at it. Only the unwind map did.
- On `TBottomViewTown` the divergence was not a COUNT at all but the
  **position** of a store: retail's `mov [ebp-4],5` sits after the whole
  `strlen`/`_Grow`/`rep movs` block, ours sat before it. Read transcripts
  POSITIONALLY — which instructions each store sits between — not just as a
  multiset. `diagnose` reports COUNT/ORDER; the positional read is yours.

Method that worked three times: **compare the two state sequences with their
neighbouring calls attached** (`grep -E ': call |\[ebp - 0x4\]'` over
`homm3 sema disasm` on both sides, then `diff`; the default listing carries
the callee name on the call row). The call names localize the store; the
store localizes the statement.

The Dreamcast xref graph is the natural corroborator for a lifetime claim,
because it names the CALLS a compiland makes at source level:
`awk -F'\t' '$1=="0x<dcoff>"' evidence/dc-xref-graph.tsv`. It confirmed
`TBottomViewTown` reaches `basic_string`'s **constructor** (plus the
`allocator<char>` temporary of the `const _A& = _A()` default argument) and no
`operator=`.

### What the remaining rows now say

- **`ai_combat:?choose_melee`** `[0,1,-1]` vs `[0,1,0,-1]` — the missing reset
  is not a statement: retail CALLS `type_monster_vector`'s copy constructor at
  both local-copy sites and takes one of the two teardowns out of line, while
  our compile expands both one level further (down to `std::vector`'s copy
  ctor and to inline `operator delete`). Inline depth, not lifetimes.
- **`armygrp:?get_morale_description`** — the one extra region is the inlined
  `std::bitset<N>::_Xran()`: retail CALLS the out-of-line
  "invalid bitset<N> position" thrower at `0x434ad0`, we expand the whole
  `basic_string` + `out_of_range` + `_CxxThrowException` sequence at the
  `test()` site and that string temporary is the extra lifetime. Every other
  lifetime in that 2140-byte body already matches.
- **`campaignbrief:??1TCampaignBrief`** (ORDER) — destruction order is
  identical; only the unwind-map INDEX assigned to each `NewSMapHeader`
  sub-object differs (retail `0x2c0→2, 0x2d0→3, 0x20→4, 0xa0→5`, ours
  `0xa0→2, 0x2c0→3, 0x2d0→4, 0x20→5`). Retail numbers the derived class's own
  members before the base's; we interleave. That is a `game.h` layout/ctor
  question and the row's bigger residual is a callee-saved role swap
  (`ebx`↔`edi`) anyway.
- **`game:?Save`** — retail opens **44** more regions than we do and has 55
  conditional branches against our 24. The transcript agrees for its first 13
  stores and then we skip retail's 13/14/15. This is not a wall, it is an
  unfinished body, and the transcript measures how unfinished: 44 object
  lifetimes.
- **`game:?Load`** — 80 stores against retail's 81, i.e. the lifetimes are
  essentially right while the body is over-inlined (97 conditional branches
  against 72). Opposite diagnosis to `Save` on the same pair of functions.

### The recurring shape behind the COUNT rows: we inline one level deeper

Once the lifetime bugs are out, every remaining row in the table is the same
divergence in the same direction — **retail stops one inline level earlier
than we do**: `basic_string::assign` (luck), `_Tidy` (`TQuickHeroWindow`),
`bitset::_Xran` (morale), `type_monster_vector`'s ctor/dtor (`choose_melee`),
`strstreambuf::strstreambuf` vs `basic_ostream`'s ctor (`TBottomViewTown`).
Per `docs/vc6/inliner.md` that is the `budget / sites-remaining` allowance
being too large, and the site-count probe quantifies the deficit per row —
with `Widgets.capacity()`-style FREE candidates (cb ≤ 0x28, no bytes emitted):

| row | extra free sites to flip | result |
|---|---|---|
| `viewarmywindow:??0TViewArmyWindow@@QAE@HHHE@Z` | +1 | 97.1049 → **100.0000, LANDED** |
| `bottomviewsubwindow:??0TBottomViewTown` | +2 | ~~95.6295 → 97.4523~~ **97.3638 → 98.8631** (re-measured after the GetCurrTown landing; still +2, flat at +3) |
| `armygrp:?get_luck_description` | +4 | ~~82.5689 → 90.1916~~ **82.5689 → 95.1557** (re-measured 2026-08-15; still +4, and the probe must be a USER-DEFINED inline — `armygrp_clamp(0,luck,3)` registers, `basic_string::size()`/`capacity()` do not) |

**The first row is landed and EXACT** (2026-08-14); the padding is not what
landed it. `docs/dc-line-tables.md` is the instrument that answered "which
candidate site does retail's source have here that ours does not": the
Dreamcast **line/addr table** attributes every run of DC instructions to a
source line, and the DC build of a compiland is an OLDER REVISION of the same
file — so retail's post-DC edits are exactly the code with no DC line of its
own, and that is where a site retail has and we do not must live. On
`TViewArmyWindow(int,int,int,unsigned char)` it is the elemental/version gate
(`dc 0x19148c` line 284 hands `traits->townType` straight to the portrait
builder — the gate does not exist there), spelled as a call to a free
predicate. The same table separately showed the three `Influence[i] = -1`
stores are a counted `for` loop, which VC6 unrolls back into retail's three
stores and which recovers retail's EDX. 97.1049 → 99.9352 → 100.0000.

**Placement is itself a bound, and on `get_luck_description` it halves the
search** (2026-08-15). The same four probes score 95.1557 in the clover arm
and in the devil block, but **80.5000** in the halfling arm, after the tail,
or split 1/3, 2/2, 3/1 across clover+halfling. The four sites are therefore
at or before the Rampart gate, which EXCLUDES the halfling arm — one of the
two post-Dreamcast blocks the line table had allowed — and leaves the clover
arm as the only place they can live. Two real in-window sites are landed
(`is_base_elemental` in the clover gate, `town::HasBuilding` in the Rampart
gate) and both are byte-flat, which is what a threshold this sharp predicts.

The other two rows are still padding-only — a measurement, not a spelling.
The deficit is a measured per-row integer, and the line table is now the way
to spend it. Note the placement rule from `inliner.md` §5.9 still
applies: a site helps only if it is at or after the divergent site in the
tuple stream, except where the divergent site is at index 0 or in the
member-initializer prologue, where any site in the body raises the divisor.

**How far the table actually gets you, seven rows in (2026-08-14).** The
survey in `docs/dc-line-tables.md` says which of three answers a row gets,
and it is worth knowing before spending a round:

* the divergent site is a statement the Dreamcast build spells differently
  and retail's bytes AGREE with the DC spelling → the table names it
  outright. That is what closed `TViewArmyWindow` and what moved
  `TBottomViewTown`/`TBottomViewHero` (`game::GetCurrTown`/`GetCurrHero`);
* the divergent site sits inside a POST-Dreamcast block → the table bounds
  the search negatively and no further. `TBottomViewTown`'s remaining +2 is
  somewhere in sixteen source lines of quantity text, `get_luck_description`'s
  +4 in its clover and halfling arms;
* the row has NO post-Dreamcast region at all (`TBottomViewKingdom`) → the
  missing site MUST be a differently-spelled statement, and the table
  enumerates the candidates. Refusing one of them (`memset`, whose intrinsic
  expansion is not retail's four stores) is the asymmetry rule working, not
  a dead end.

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
