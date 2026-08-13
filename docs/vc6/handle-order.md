# The front-end symbol-handle order - assignment model, per-declaration cost, and the B1/C1 verdicts

Phase 5 of the vc6 area: model HOW C1XX assigns the numeric symbol handles
that every IL stream bakes in (docs/vc6/il-format.md section 2), so the two
wall classes that share that root - the B1 register-swap residue
(docs/vc6/regalloc.md section 5) and the C1 include-set sensitivity
(il-format.md section 5) - become statements about declarations.  Subjects:
the pinned C1XX.DLL 12.00.8472 (game profile `/c /nologo /O2 /Ob2 /Oy- /Op
/ML /Gr /GX /GR- /D_WINDOWS`) plus C1.DLL for one C-mode cross-check and
C2.DLL 12.00.8447 for the back-end structure; every number below was
measured 2026-08-10 on the pinned binaries under wine via the
`homm3.vc6.il` capture tap.

## TL;DR

* **Assignment is strictly top-to-bottom at parse point, additive, and
  redeclaration-free.**  A declaration costs a fixed number of handles,
  shifts every LATER symbol by exactly that cost, and never renumbers
  anything before it (a struct placed after a function: high-water +9,
  the function's handles +0; two structs: +18; a repeated forward tag or
  typedef: +0).
* **The +9 is explained**: `struct probe0_t { int a; };` = tag(1) +
  C++ class-definition overhead(7) + data member(1).  The same struct in
  C mode costs 2 (tag + member) - the 7 is C++ machinery, consistent
  with injected-class-name(1) + implicit default ctor(1) + copy
  ctor(2) + dtor(1) + operator=(2) under the same measured
  "function = 1 + #params" rule.  The killer experiment's numbers
  reproduce at micro scale and at initialize.cpp scale.
* **The cost table predicts blind**: 32/32 calibration rows exact, then
  4/4 NEW probes predicted before compiling - including two deliberate
  extrapolations (second virtual method, parametric ctor).
* **Within a function**: parameters (source order), then `this` (a real
  per-function `sy` record), then the file symbol (once per TU, at the
  FIRST body), then the block, then locals in textual creation order.
  `this` is not source-spellable; params < `this` < locals is
  parse-FIXED.
* **get_simple_attack_effect verdict: NOT source-movable.**  The
  relative order `this` < `start_our` already matches retail's ESI-first
  binding; our transposition survives 21 handle-shift probes at three
  localities (+1..+12, +64/128/256/512 before the function; +1/+9 at top
  of file; struct/typedef appends to an early header) with ZERO
  instruction changes anywhere in the TU - the C1 numbering lever does
  not reach this function.  The pair is capped as front-end STATE that
  differs from retail's actual TU content, not handle order or count.
* **The include-set advisor works**: on the C1 flagship it measures the
  +9, names `struct probe0_t { int a; };` as the cause, localizes its
  creation point between two named symbols, and reports EXPLAINED.

## 1. Method

The oracle is `homm3.vc6.il.capture` (the `/d1il` tap): compile a probe
TU, read the `gl` high-water (the counter's next-free value) and the
named-record handles back from the `gl`/`sy` scans.  The baseline TU is
the il-format probe `int add(int a, int b) { return a + b; }` (a=0xd5
b=0xd6 add=0xd7 file=0xd8 block=0xd9, high-water 0xda); each battery
variant adds ONE declaration and the cost is the high-water delta.  Every
row is reproducible with

    python3 -m homm3.vc6.il handles <probe.cpp> [--predict]

## 2. The measured cost table

C++ front end, game profile, file scope; cost = high-water delta vs the
baseline, shift = the baseline function's handle delta (parse-point
proof):

| declaration | cost | shift of later symbols |
|---|---|---|
| `struct S { int a; };` | **+9** | +9 |
| `struct S { int a; int b; };` | +10 | +10 |
| `struct S { };` | +8 | +8 |
| `struct S;` (forward) | +1 | +1 |
| `struct S; struct S;` | +1 | +1 (redeclaration free) |
| `class S { int a; };` / `union S { int a; };` | +9 | +9 |
| `enum E { X1 };` / `enum E { X1, X2 };` | +2 / +3 | same |
| `typedef int T;` (repeat free) | +1 | +1 |
| `typedef struct { int a; } T;` | +10 | +10 |
| `int f();` / `int f(int);` / `int f(int, int);` | +1 / +2 / +3 | same |
| `int g;` / `extern int g;` / `static int g;` | +1 | +1 |
| `int sub(int c, int d) { ... }` (definition) | +4 | +5 (see below) |
| `struct S { int a; void m(); };` | +10 | +10 |
| `struct S { int a; void m(int); };` | +11 | +11 |
| `struct S { int a; void m(); void n(); };` | +11 | +11 |
| `struct S { S(); int a; };` | +9 | +9 (explicit default ctor replaces the implicit - net 0) |
| `struct S { S(); ~S(); int a; };` | +13 | +13 (explicit dtor net +4) |
| `struct S { virtual void m(); };` | +12 | +12 (first virtual +3 over a plain method) |
| `struct S { int a; virtual void m(); };` | +13 | +13 |
| `struct S { static int a; };` | +10 | +10 (static member +2) |
| `struct S { int a; }; struct T2 { int a; };` | +18 | +18 (additive) |
| `struct S { struct Inner { int a; }; int b; };` | +18 | +18 (nesting recurses) |
| `struct S { int a; }; struct D : S { };` | +17 | +17 (a base class costs 0) |
| `struct S { int a; }; S s;` | +10 | +10 (using a type costs only the var) |
| `struct S; S* p;` | +2 | +2 |
| `struct S { int a; };` **in C mode** (C1.DLL, `/TC`-class TU) | +2 | - (tag + member; no C++ overhead) |
| declaration AFTER the function | +9 | **+0** (parse-point assignment) |

The function-definition asymmetry (+4 cost, +5 shift) is the file
symbol's laziness: `sub`'s definition mints c, d, sub, block (+4) AND
pulls the once-per-TU file symbol ahead of `add` (baseline order
a b add **file** block; with a preceding definition c d sub **file**
block a b add block) - measured, and modelled in
`_handles.predict_handles`.

Derived rules (all byte-verified above): aggregate = 1 (tag) + 7 (C++
definition overhead) + 1/data member + (1 + #params)/method + 3 once for
the first virtual + 2/static member + net 0 for an explicit default
ctor, net +4 for an explicit dtor; enum = 1 + #enumerators; function
declaration = 1 + #params (parameters consume handles even in a pure
declaration); everything redeclared is free.

## 3. Within a function - the order the B-family cares about

The member-function probe (`struct C { long x; long m(long p); };
long C::m(long p) { long v = x + p; return v; }`) gives, at the
out-of-line definition: `p`@0xe0 < `this`@0xe2 < file@0xe3 < block@0xe4
< `v`@0xe5.  So:

* `this` IS a per-function symbol with its own handle (a named `01 01
  <u16> 00 "this"` record in `sy`), minted after the declared parameters
  and before the body - its relative position is fixed by the parse, not
  by any spelling.
* Locals follow their textual creation point - the mechanism behind the
  B13/B14 naming levers (regalloc.md section 4: minting the handle
  earlier moves the pseudo earlier).
* In-class method declarations mint handles at class-parse time
  (`m`@0xd8 inside C's +tag block); an out-of-line definition re-mints
  its parameters (the class-parse `p` and the definition `p` are
  different handles).

## 4. The model - `scripts/homm3/vc6/_handles.py`

* `predict_decl_cost(text)` / `scan_decls(text)` - the pure cost model
  over a file-scope declaration subset (aggregates with
  members/methods/virtual/static/ctor/dtor, enums, typedefs, function
  declarations, variables, nesting).  Anything unrecognized (templates,
  macro residue) costs **None** and is reported UNPREDICTED - never
  guessed (negative control: `template<class T> struct X { T a; };`
  yields no number).
* `predict_handles(tu_source)` - absolute ranges from base 0xd5 (0x58
  for C), exact for include-free simple TUs, flagged APPROXIMATE beyond
  (function bodies add locals the file-scope walk does not count).
* `capture_handles(src)` / `creation_table(cap)` - the oracle side over
  `il.capture`.
* `handle_delta(srcA, srcB, shadow_a=, shadow_b=)` - capture both,
  report high-water delta, which named handles shifted and by how much,
  the creation point the cause entered at (between the last stable and
  first shifted symbol), and the attribution of the source/shadow diff
  through the cost model with an EXPLAINED / NOT-explained verdict.
* `advise_handle_move(unit, fn, target_value)` - the why-reg v2 hook
  (section 7).

Calibration: 32/32 battery rows exact.  Blind validation (prediction
stated before the compile, then confirmed):

| new probe | predicted | measured |
|---|---|---|
| `struct S { int a; int b; int c; void m(int, int); };` | +14 | +14 |
| `typedef int T; enum E { A, B }; int f(int, int);` | +7 | +7 |
| `struct S { virtual void m(int); virtual void n(); int a; };` | +15 | +15 |
| `class Q { Q(int); ~Q(); virtual void v(); int a, b; };` | +19 | +19 |

Rows 3 and 4 exercise the two extrapolated rules (second virtual at the
plain-method rate; parametric ctor at +#params).

## 5. Validation A - the C1 flagship (initialize.cpp)

`_handles.handle_delta` over the killer pair (initialize.cpp, honest
shadow town.h vs town.h + `struct probe0_t { int a; };`):

    handle high-water 0x2bb8 -> 0x2bc1 (+9)
    58 named handle(s) shifted (+9), first: ??_C@..."invalid bitset..." @0x2a86
    cause enters between ?gHierarchyMask@@3PAY0CM@_JA @0x273b (stable)
      and the first shifted symbol - that is its creation point
      added:   struct probe0_t { int a; };   predicted +9
    model prediction +9 vs measured +9 -> EXPLAINED

Same numbers as il-format.md section 5 (high-water 0x2bb8->0x2bc1, 58
shifted fields, all +9), now ATTRIBUTED and localized by the model: the
advisor answers "which declaration caused the shift and where it
entered" for a header regression.  CLI:

    python3 -m homm3.vc6.il handles srcA.cpp --against srcB.cpp \
        [--shadow town.h=honest.h --shadow town.h=perturbed.h:b]

## 6. Validation B - the get_simple_attack_effect B1 swap

regalloc.md section 5: retail binds `this`=ESI `start_our`=EDI
`start_enemy`=EBX (the pure first-fit prediction from creation order);
our compile transposes the first pair.  why-reg v2 proved no statement
spelling reaches it; the open question was whether a DECLARATION or
include change (the C1 lever) can.

The creation-order fact (section 3) already rules out relative-order
fixes: `this`@h is minted before `start_our`, so retail's ESI-first
binding IS the handle order, and our transposition must come from
handle-VALUE-keyed state, not order.  The remaining lever - shifting the
absolute values - was swept with unused-forward-tag fillers (+1 each)
and header appends, compiled with the unit's exact build profile
(`_unit.compile_text`), reading the binding from the `/FAs` listing
(`this` arrives in ecx under `/Gr`; the detector is the first
`mov esi|edi|ebx, ecx` of the PROC):

| locality | shifts tried | binding |
|---|---|---|
| file scope, immediately before the function | +1..+12, +64, +128, +256, +512 | `this`->EDI, all 16 |
| top of file (after the last `#include`) | +1, +9 | `this`->EDI |
| appended to an early header (shadow ai_tactical.h - the killer mechanism) | struct x1 (+9), x2 (+18), typedef (+1) | `this`->EDI |

Controls that the probes LANDED: C2's internal label/static counters
shift by exactly k (`$L20814` -> `$L20815`/`$L20823`/`$L21326` under
+1/+9/+512), and under +64..+512 the listing's EXTRN/stack-equate
EMISSION ORDER visibly permutes - yet masking counters and line echoes,
the k=1 listing is byte-IDENTICAL to k=0 and no probe changes a single
instruction anywhere in the TU (the obj deltas are numbering artifacts,
37 B at +1).  Contrast initialize.cpp, where the same +9 moves 11,900
obj bytes: shift-sensitivity is TU-/function-local state, not a
universal lever.

**Verdict: ABI-and-parse-fixed, C1-class capped.**  No source change in
our reach - spelling, declaration order, include set, or handle count -
flips this pair; the front-end state that orders the two pseudos
differently from retail is not the NUMBERING but something retail's
actual TU content (its real header set) established.  `advise_handle_move`
returns exactly this with the evidence list, for `this` and for
parameters; for body locals it returns the movable verdict with the
textual-creation-point lever.

## 7. The C2 side - where handle values become state

Read-only queries over the persisted c2-atlas project
(`ghidra_scripts/handle_probe.py hash`, output under
`build/re/vc6/raw/handles/`): the back-end symbol hash regalloc.md
pinned resolves handles as `bucket[handle & 0x3ff]`, key at sym+0x1c,
chain at +0x2c (lookup fn rva 0x232ec, buckets .bssbe 0x9d88c).  The
reference walk adds the machinery around it: an insert (0x21264), an
unlink (0x213c6), the REP STOSD init (0x2abae), and two whole-ARRAY
iteration walks (0x2450d, 0x2df43) - symbol ENUMERATION in C2 is
bucket-ordered, which is exactly the reorderable structure behind the
measured EXTRN/equate emission permutation under large shifts, and the
standing suspect for C1's non-monotonic codegen sensitivity where it
does bite.  Atlas attribution: all these sites sit in the `?|MDmisc.c`
bracket neighbourhood (il-format.md's "p2symtab.c hash" reading was a
hypothesis; p2symtab.c proper is the 0x8339f..0x84d9b block, dumpable
via `handle_probe.py dump` for the next phase).  Walking a bucket-order
enumeration explains the inertness of a UNIFORM shift on co-shifted
symbols (pairwise bucket order is preserved except across the 0x400
wrap) - consistent with all 21 flat probes - while mixed
shifted/unshifted populations (a header change amid later headers, the
initialize.cpp shape) can permute; proving that last step in the
dumped code is open work, not a claim.

## 8. why-reg v2 hook (proposed wiring)

`_handles.advise_handle_move(unit, fn, target_value)` is importable
today and returns `{movable, verdict, levers, evidence}` with zero
compiles.  The intended call site is `reg_model.run_model`, right after
the `want_value == "this"` message - behind try/except so a missing
model never breaks why-reg:

```python
try:
    from homm3.vc6._handles import advise_handle_move
    adv = advise_handle_move(unit, args.fn, want_value or "this")
    print(f"[handles] {adv['verdict']}")
except Exception:
    pass
```

Not landed here (this phase's file set is
`_handles.py`, `handle_probe.py`, this doc, and the `handles` mode in
`il.py`).  why-reg's default behavior is untouched.

## 9. Commands and files

| command | role |
|---|---|
| `python3 -m homm3.vc6.il handles <src> [--predict] [--json]` | capture the creation table; compare the pure predictor against it |
| `python3 -m homm3.vc6.il handles <srcA> --against <srcB> [--shadow NAME=FILE[:b]]` | the handle-delta advisor (which handles shifted, which declaration explains it) |
| `python3 scripts/homm3/vc6/ghidra_scripts/handle_probe.py hash\|dump` | read-only C2-side queries (bucket machinery; p2symtab.c neighbourhood) |

| path | role |
|---|---|
| `scripts/homm3/vc6/_handles.py` | the model: cost table, scanner, predictor, delta advisor, advise_handle_move |
| `scripts/homm3/vc6/il.py` (`handles` subcommand) | the CLI over capture + model |
| `scripts/homm3/vc6/ghidra_scripts/handle_probe.py` | the C2-side probe |
| `build/vc6/il/handles/`, `build/re/vc6/raw/handles/` | captures / RE working data (gitignored) |

Open items: the +4 explicit-dtor anomaly (which four symbols; EH
machinery under /GX is the suspect), the unattributed handle between a
definition's parameters and `this` (0xe1 in the member probe), sy
type-tail semantics (il-format.md section 4 - unchanged), and the
bucket-walk -> codegen path for the TUs where shifts DO bite
(initialize.cpp) - the `handle_probe.py dump` corpus is the entry
point.
