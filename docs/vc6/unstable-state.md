# C2/C1XX state that unrelated edits disturb

A function's bytes can change when nothing it contains, and nothing it
calls, has changed. The ledger shows this as CUR below MAX. This page lists
the compiler state that such unrelated edits move, and the tools that
read it, set it under IL replay, and compile a unit 1-to-M: once, into
every assembly each of its functions can take.

"Unrelated" means the edit touches neither the function nor its callees.
Examples are a declaration added to a header, another function inserted,
moved or removed, or include content that changes.

## The state variables

### 1. Phase flag (C2 `.bssbe` 0x9f120)

**Unrelated edit → C2 state → effect.** The functions compiled before this
one leave a value in the phase flag, and the function reads it before its
own driver resets it.

* **Set by** the per-function driver `0x13615` (one caller, `0x683c7`):
  * 1 in phase 1 (`0x136f6`);
  * 0 in phase 2 (`0x13916`);
  * back to 1 in phase 3 (`0x1398e`), only when the function record's
    `+0x0c` field is zero.
* **Read by** `0x5739` at `0x5b11` (reached from `0x5e5c`, `0x6dd8`,
  `0x6e0b`, `0x5210b`, `0x8ff7a`) during the next function's early
  processing, before that function's own driver runs. Nothing resets it in
  between.
* **Initial value** is 0, so the first function C2 compiles in a unit
  receives 0.
* **Moved by** any edit that changes which function C2 compiles just before
  this one: inserting, removing or reordering functions. A static function
  that is never referenced is not emitted and changes nothing.
  * Every tested kind of emitted earlier function leaves 1: plain, loop,
    EH local, try/catch, switch, float, `__asm`, setjmp, virtual, inline
    user, 40-statement.
  * 0 reaches a function when it is compiled first, or when it follows a
    function whose record has `+0x0c` set.
* **Range**: {0, 1}.
* **Effect, from ground truth**: `combatManager::showCreatureSpellError`
  (drawing). Its current tree compiles it first in the unit, so it receives
  0 and is CUR (99.46%). The MAX commit compiled another function first, so
  it received 1 and was 100%.

### 2. Declaration offset: the symbol-handle numbering (C1XX → C2)

**Unrelated edit → C1XX handle counter → C2 handle values → effect.** Each
declaration takes handles from C1XX's counter, which [handle-order.md](handle-order.md)
models (a `typedef` takes 1, a one-member struct 9, a function definition
1 + its parameters). Every symbol created after an edited point is
renumbered by the difference.

* **Read under replay**: C2 decodes each handle in the IL reader `0x1c92e`
  (a u16, or 31 bits when bit 15 is set).
  * 15 call sites carry handles.
  * `0x1cedf`, `0x1d193` and `0x1d253` carry other numbering and never
    shift.
  * The sites were measured by decoding real captures with and without
    leading declarations, in initialize and singleselectionwindow.
* **Moved by** any declaration added to or removed from a header the unit
  includes, or from the unit before the function. Reordering definitions
  also moves it.
* **Effect**: periodic in the offset, with period 64.
  * `initializeGameData` takes 6 assemblies over offsets 0..63, and the
    same 6 recur for every offset up to 260.
  * The offset changes whether C2 hoists `&g_hierarchyMask[i]` into a
    register or folds it into each access.
  * Real offsets placed before the definition and at the top of the unit
    give the same 6 classes.
  * A 16×16 grid of both placements together adds no class.
* **Range**: offset mod 64.

**Where C2 depends on it.** This has not been located beyond its input.
Ruled out:

* the per-function back-end hash at `.bssbe 0x9d88c`, whose keys are
  per-function temporary numbers (1..n), identical across offsets;
* heap displacement before C2 starts;
* single transplants of other C2 globals.

Observed:

* Under a top offset, the decode order changes at `0x67d47`/`0x67f7a`.
* The first execution divergence in `initializeGameData` is inside the
  bucket walk `0x2df43`: `0x2fda4` performs six extra set tests
  (`0x355e`).

The replay knob reproduces the real effect exactly (below), so the state
is fully accessible even with the mechanism unnamed.

### Not unrelated-edit state

* **Cost records** (`sym+0x6d`) of callees and of the function itself
  move only when a callee's or a referenced declaration's source changes.
  That is a related change. `homm3 vc6 variants` still sweeps them
  separately ([context-variants.md](context-variants.md)).
* **Other C2 globals.** The `.bssbe`/`.data`/`.databe` dwords that differ
  between contexts were each transplanted into the other context's replay
  on 21 ground-truth cases. None changed a function except `0x9f120`.
  These crashed C2 when transplanted alone and were not resolved:
  * object-writer counters `0x998f4`/`0x998fc`, written in `0x1dcd2` and
    read across `coffemit` at `0x81d00..0x83322`;
  * counters `0x9bc4c` (`0x53f4`) and `0xac6c4` (`0x1184`, `0x695dc`,
    `0x7e2cf`);
  * `0xae094` (`0x2a996`);
  * `0x9f508`/`0x9f55c`, which have no direct references.

  Natural variation of all of them is exercised by the fuzz edits below.

## Tools

```sh
homm3 vc6 state <unit|VA>                  # read: emission order, phase received, handle base
homm3 vc6 compile-m <unit> [--function VA] [--against OBJ]
homm3 vc6 fuzz-verify <unit> [--function VA] --edits N [--reuse]
```

**Set (replay).** The trace shim takes two environment variables and C2
decides everything itself:

* `HOMM3_VC6_PHASE=0|1` leaves that value at every driver return
  (`0x683cc`) and initially.
* `HOMM3_VC6_HANDLE_SHIFT=1:k` adds k to every decoded handle. The `gl`
  high-water (offset 7) is raised by k in the replayed streams.
  `unstable_state.State` and `Unit.replay` wrap both.

**Read.** `state` replays the captured context with
`HOMM3_VC6_STATE_LOG` and lists, per function in emission order:

* the phase it received (what the previous driver left; 0 for the first);
* its handle base (`sym+0x28`) modulo 64.

**compile-m.** The unit's front end runs once, and its IL is replayed:

* once with no state set, which must reproduce the plain replay for every
  function (inertness);
* once per (phase, offset) for phase ∈ {0,1} and offset 0..63, 128
  replays.

`build/vc6/unstable-state/<unit>/compile-m.json` lists, for every function,
its M distinct assemblies. Each variant carries its relocation-masked bytes
in hex, its SHA-1 prefix and the states that produce it.

**fuzz-verify.** Applies N seeded random unrelated edits to scratch copies
under `build/`. Each edit is one of:

* 1..4 declarations or definitions inserted at the top or before an
  annotated definition;
* a swap of two adjacent definitions;
* a declaration inserted into a shadow copy of a directly included header.

Each copy is compiled with the plain compiler, with no shim. The harness
asserts that every original function's bytes are in its compile-m set.
Escapes are logged with the edit; coverage counts which predicted variants
the edits actually hit.

## Validation

**Replay knob against real recompiles.** Real compiles with k leading
`typedef`s were compared with handle-shift replays of the k=0 capture, for
every function of the unit:

* initialize, army, philai and advmgr at k = 2, 13, 27, 48: 487 functions,
  0 mismatches;
* `initializeGameData` at all 64 offsets, at both placements: exact.

**Ground truth.** Of the 21 CUR≠MAX functions whose own source is
unchanged:

* Only drawing's has neither its own IL nor any inlined callee's IL changed
  semantically. compile-m of the current tree contains its MAX bytes, at
  phase=1.
* `initializeGameData`'s MAX is also in the set, at decl-offset 27 with
  phase 0.
* The other 19 changed through dependency source and are correctly absent:
  a `bool` normalisation, a member constant, or a callee body.

**Fuzz** (first pass, insert-only edits, 100 edits per unit, 16 units):
185,000 function checks, 0 escapes. FUZZ2

## Limits

* compile-m sweeps one global offset. Combinations of different offsets at
  different points (a header edit plus an edit before the function) were
  probed only on `initializeGameData`'s 16×16 grid.
* The phase model assumes both values are reachable for every function.
  Reaching 0 needs the function compiled first, or after a function with
  record `+0x0c` set; which source constructs set `+0x0c` is not known.
* The decoder's non-handle sites were measured on two units. A unit that
  decodes another kind of number at a handle site would mis-shift it. The
  whole-unit simulation check above is the guard.
