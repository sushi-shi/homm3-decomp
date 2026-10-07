# The C2 phase flag: why a TU's first function compiles differently

C2.DLL 12.00.8447 (image base `0x10700000`; RVAs below) keeps one dword,
`.bssbe 0x9f120`, that carries over from one function to the next. It decides
whether the global optimizer may fold a conditional branch whose compare
has a known outcome. The global optimizer driver always leaves the flag at 1.
The **first function in a TU that the global optimizer processes** therefore
starts with the zero from back-end initialization. Every later function
starts with 1. A function is affected only when its opening simplification
sweep sees a compare whose outcome is already known. Such compares usually
come from an inline helper called with constant arguments.

Evidence levels: **measured** means observed in pinned-compiler runs with an
instrumented C2 (see [Method](#method)). **Static** means read from the
disassembly. **Hypothesis** is marked as such.

## 1. The driver `0x13615` and its stages

<!-- c2-role: function 0x13615 globalOptimizeDriver -->
<!-- c2-role: global 0x9f120 globalOptPhaseFlag -->
<!-- c2-role: global 0x9f114 globalOptStage -->
<!-- c2-role: global 0x9f0e0 flowGraphDirty -->

`optimizeFunction` (`0x6819b`) calls the driver once, at `0x683c7`. The call
comes after the first `optimizeExpressions` pass. It runs only when the
function's per-function option bit 21 (`0xac054`, unpacked at `0x1bd89`) is
set. `#pragma optimize("g", off)` or `("", off)` clears it (measured).
`cold_owner` places the driver and most of its callees in the
`globdf.c`/`globopt.c` cold region (§7), so this is the global optimizer.
`0x9f114` is a stage counter written by the driver as 0, 1, 2 and 3. The
named stages below are static readings of the control flow. Callee roles
not traced further are left as addresses.

| stage (`0x9f114`) | flag `0x9f120` | what runs (static) |
| --- | --- | --- |
| 0, prologue | **not written**: the leftover | pool setup; `0x129ec` resets per-symbol state; **`0x12a78` simplification sweep over every block** (`0x5e29` then `0x5739` on each expression tuple); `0x4256c` CFG rebuild if `flowGraphDirty`; `0x12aa5`, `0x1268e`; dataflow `0x5479(1)`/`0x50ec`; `0x11703` global propagation (also reaches `0x5739`, through cold `0x8ff63`); `0x5479(5)`/`0x50ec`; loop analysis `0x47a0c`/`0x47a4b`/`0x47eeb` **only if the body has loops**; `0x111c5`, `0x10b45`, `0x5df3` |
| 1 | **=1** at `0x136f6` | per-block pass over the block list `body+8`: `0x6b7e` on each block, which reaches `0x5739` through `0x6c56`/`0x6dfe`; extra work for each block that heads a loop (`block+0x68`): `0x4324b` and the live-set handling `0x11f1`…`0x18ce`; then `0x5724`, `0x8570`, and `0x4256c` when dirty |
| 2 | **=0** at `0x13916` | dataflow `0x5479(5)`; loop work **only if the body has loops**: `0x55a0` with `0x9f10c` = loop list, `0x47a0c`…`0x480c4`; `0x10b45`, `0x877e`, `0x11ec`×3, `0x8c93`; **`0x43d41` over each top-level loop** (recursive over the loop tree; it reaches `0x5739` through `0x449b0`); `0x9819` |
| 3 | **=1** at `0x1398e` | `0x9892`, `0x9792`, `0x877e`, `0x5df3`; **the same per-block pass as stage 1**, now with `0x9f0e8`=1, which enables the small-loop arm, plus `0x432de`; then `0x5724`, `0x8570`, `0x913d`, dataflow `0x5479(1)`/`0x50ec`, `0x55a0`, **`0x25477`** (reaches `0x5739` through cold `0x8ff9f`), `0x25522`, cleanup |

**Is it a re-run? Partly.** Stage 3 runs stage 1's per-block pass again; the
loop bodies at `0x13719…0x138a7` and `0x139ab…0x13ba0` have the same callee
sequence. Stage 3, including write 3, is **unconditional**. The test of
`body+0x0c` at `0x13941` leads to the per-loop walk at `0x13a2f`. That walk
calls `0x43d41` for each loop node and then jumps back to `0x1394c`, so the
two paths join before `0x1398e`.

Measured: all 500 driver runs in drawing, army, advmgr and philai reach all
three writes and leave 1. These include the 166 runs whose loop list was
non-empty. The hypothesis that the third write depends on `+0x0c` is refuted.

The stage-2 zero is deliberate. While the driver walks the loop tree,
branch folding is disabled because it could delete edges and invalidate the
tree. This purpose is a hypothesis, but the stage-2 reads do see 0 (measured).

## 2. Writers, readers and values

* **Writes:** exactly the three driver writes. Ghidra references and a raw
  imm32 scan of `.text` agree (4 sites in total). No other code takes the
  address of `0x9f0e0…0x9f12c`.
* **Read:** exactly one site, `0x5b11` in `0x5739`.
* **Values:** {0, 1}. The registers are fixed at `0x13641` (esi=1),
  `0x13653` (edi=0) and `0x1397d` (ebx=1). Every logged read was 0 or 1.
* **Initial value:** `InvokeCompilerPass` (`0x68fd0`) calls `0x6902f` once per
  TU. The first call saves `.databe`. Later calls restore it and zero
  `.bssbe` from `0x99000` to `0x9f6d0`. The flag therefore restarts at 0 for
  every TU, even when one CL invocation compiles several files. Measured:
  `cl a.cpp drawing.cpp` and `cl drawing.cpp` produce the same `drawing.obj`
  apart from the timestamp.
* **Leftover-dependent reads:** only the stage-0 reads, through `0x12a78`
  and `0x11703`. Reads in stage 1 and stage 3 see 1, and reads in stage 2
  see 0, for every function.
* **Sibling stage flags:** `0x9f0e8`, `0x9f0fc`, `0x9f104` and `0x9f118` also
  survive between functions. Forcing them to their first-function values at
  driver entry changed **0 of 500** functions in four units, so they have no
  effect.

<!-- c2-role: function 0x6902f resetBackendState -->

## 3. `body+0x0c` is the loop list

<!-- c2-role: function 0xa811 findNaturalLoops -->
<!-- c2-role: function 0x4256c rebuildFlowGraph -->

`0xa811` writes `body+0x0c`. Its cold blocks are in `lg.c`, the loop-graph
module, and `0xb20b` and `0x4256c` reach it. It allocates a 0x40-byte
sentinel node with `0xab52`, then clears each block's `+0x68`. It finds back
edges by testing each predecessor in the dominator bitset at
`block+0x60`/`+0x64` (`0x3355`). For each back edge it creates a loop node:
`+0x14` is the header, `+0x18` the latch and `+0x24|=1`. At `0xab46` it stores
the sentinel's first child.

`+0x0c` is therefore non-null exactly when the CFG contains a natural loop
at the time of the most recent rebuild. That can be a `for`, `while` or
`do` loop, or a backward `goto`. It controls only the loop-specific steps
listed in §1. It does not affect the flag left for the next function.

## 4. What the reader decides

<!-- c2-role: function 0x5739 simplifyExpressionTuple -->
<!-- c2-role: site 0x5b11 globalOptPhaseFlagRead -->
<!-- c2-role: function 0x2fc6 findResultConsumer -->
<!-- c2-role: function 0x48761 evaluateConstantCondition -->
<!-- c2-role: function 0x12a78 simplifyAllBlocks -->

`0x5739` simplifies one expression tuple. The flag gates only one arm:

1. The tuple is a compare (opcode `0x17d`; the type word's low 12 bits are
   the condition). Its outcome is known: both operands are constants, the
   two operands are identical, or the operands match one of the other
   known-outcome cases at `0x5a17…0x5a92`.
2. `0x2fc6` finds the next tuple that consumes the compare result.
   * If the consumer has kind byte `0x10`, the result is materialized at
     `0x5bac` without reading the flag.
   * If the consumer has kind byte `0x11` (opcode `0x185`, a conditional
     branch; its byte `+0xa` is the branch condition), the flag is read at
     `0x5b11`.
3. **Flag = 0:** the compare and branch are left unchanged.
4. **Flag = 1:** `0x48761` evaluates the branch condition on the constants.
   * True: the branch becomes opcode `0x186` (an unconditional jump, by
     inference) with condition 0.
   * False: the branch is deleted.

   In both cases the dead CFG edge is removed by `0x425d2`/`0x3e27`. Then
   `0x425ef` marks the unreachable blocks (`block+0x18 |= 0x2000000`) and
   sets `flowGraphDirty`, so the driver calls `rebuildFlowGraph` (`0x4256c`).
   That function rebuilds the flow graph, the dominators and the loops, and
   merges straight-line blocks.

The flag therefore permits CFG edits. Measured reads from five units:
* 616 had both operands constant;
* 20 compared an operand of kind 2 with a constant (the meaning of kind 2
  is a hypothesis: possibly an address compared with a literal);
* all 636 had consumer opcode `0x185`.

`0x13315` folds the same constant compare and branch pattern before the
driver, without reading the flag. It does not catch the
`showCreatureSpellError` compares, whose operands become constant only
after the driver's per-symbol reset and sweep. This is a hypothesis for why
those compares remain.

## 5. `showCreatureSpellError`, mechanistically

The function is at VA `0x4922f0` in the drawing TU. It is compiled first in
the current tree, so it starts with flag 0.

* Its stage-0 sweep reads the flag ten times. Every read is a compare of two
  constants that feeds a conditional branch: `2 == 1`, `48 < 0` and
  `48 > 150` in repeating triples. These are the bounds and count tests of
  `getArmyName(army::ARMY_CREATURE_DEMON /*48*/, 2)` inlined into the
  `case army::ARMY_CREATURE_PIT_LORD` (`sprintf(…,
  getArmyName(DEMON, 2))`) and later arms.
* **Per-read forcing** (flag replaced for chosen read indices only):
  forcing reads 2, 3 and 4 to 1, the three tests of one `getArmyName`
  expansion, is sufficient: the function then matches retail byte for byte.
  None of these alternatives produces retail: any single read; the pairs 2+3,
  3+4 and 4+5; the triples 3–5 and 4–6; the sets {1,2,3,5}, {1,2,4,5} and
  {1,3,4,5}; reads 6–10.
* **Block counts at stage-1 entry** (measured): **153** blocks with flag 0
  and **144** with those three reads forced. With flag 0 the branches survive
  the prologue. They are folded inside stage 1 and the CFG is rebuilt only
  after stage 1. Both variants have 125 blocks at stage 2.
* **Result:** in retail, which corresponds to flag 1, the code loads
  everything before the pushes:
  `mov ecx,[traits]; mov eax,[text]; mov edx,[ecx+0x15d8]; mov ecx,[eax+0x20];
  mov eax,[ebp+8]; push edx; mov edx,[ecx+0xaf8]; push edx; push eax`.
  With flag 0, the plural name `[traits+0x15d8]` is pushed as soon as it is
  loaded, and the instruction stream is one byte shorter.
* **Hypothesis for the final step:** with early folding, the remaining
  `getArmyName` arm is merged into the `sprintf` block before the stage-1
  per-block propagation. That pass then sees the name load and the other
  argument loads in one block. Without early folding, the name is still in
  its own block when that pass runs.

The other lane's `compile-m` sweep agrees. Across 64 declaration offsets
and the callee-order states, this function has exactly two assemblies, and
the phase alone selects between them.

**Minimal reproduction.** `probes/c13_phase_flag_first_function.cpp`
contains two functions with identical source,
`sprintf(b, text->get(K), armyName(48, 2))`, and the oracle shows that only
their compile position changes the push order. The probe passes. Controls:

* an `armyName` without branches gives identical bytes in first and second
  position;
* runtime arguments `armyName(g_id, g_count)` also give identical bytes;
* in both controls the reader is never reached.

## 6. Source-level rule

**A function receives 1 if and only if an earlier function in the same TU
has gone through the global optimizer.** C2 compiles functions in the order
C1XX emits them, which is also the COFF section order and the link order.
The table records the received value of the probe's `target` function,
compiled with the game profile and logged at driver entry.

| what comes first in the TU | `target` receives |
| --- | --- |
| nothing (target is first) | 0 |
| any ordinary out-of-line function defined above it (empty, loop, EH local, try/catch, `setjmp`, `_alloca`, varargs, `__asm`, naked, template caller) | 1 |
| a namespace-scope object with a dynamic initializer, defined or included above it (`_$E…` is compiled at that point: extern-call initializer, out-of-line constructor, inline constructor, `std::bitset<10>(1) << k`) | 1 |
| an out-of-line virtual destructor defined above it (`??1`, then the generated `??_G`) | 1 |
| a function compiled with `#pragma optimize("g", off)` or `("", off)` | 0 (that function does not touch the flag) |
| an inline function used by target, an unreferenced `static` function, or a `static` function defined after target | 0 (none is compiled before target) |
| a dynamic initializer defined *after* target | 0 |

Within a TU, compile order is:
* definitions in source order;
* each dynamic initializer at the point where its object is defined;
* compiler-generated special members right after the definition that
  triggers them;
* inline functions, template instantiations and template static-member
  initializers emitted out of line, deferred to the end of the TU.

**What a needed value means for the original source:**

* **Needed 0:** the function was the first one, or every earlier function
  was compiled with global optimization disabled.
* **Needed 1 for the function the current tree compiles first:** the
  original TU had some globally optimized function earlier in compile order.
  Any such function supplies 1, so the flag does not identify which one.
* **Interpreting retail order:** in an object VC6 builds, the address order
  equals the compile order. A link probe confirmed this for `_$E`
  initializers too. If the retail range of the TU still begins with the
  function, the predecessor's code is not in front of it. Possible
  explanations are a COMDAT discarded in favour of another object's copy, an
  ICF-folded body, or code that the retail layout places elsewhere. These are
  leads, not determinations.

**drawing.cpp history (measured):**

* At `11ad7f73d`, `drawing.cpp` defined `combatManager::s_visibleCombatAreaLimits`
  and `s_combatAreaLimits` above `showCreatureSpellError`. Both have
  `SLimitData` constructors, so their initializers `_$E60` and `_$E63` were
  compiled first, and the function received 1 and matched retail.
* `3f0b03da3` moved those definitions out of the TU. The function became the
  first one compiled and has been at 99.4556% since.
* The ledger rows alternate between 100% and 99.4556% at later commits that
  do not touch `drawing.cpp`. Recompiling `6d1274512`, one of the 100% rows,
  gives the flag-0 bytes, so those rows are not reproducible from their
  trees.
* In retail, the bytes immediately before `0x4922f0` belong to
  dimensiondoorwindow. They are its terrain-mask initializer funclets, whose
  stores go to `0x6969xx`. The retail predecessor that supplies 1 is
  therefore still unidentified.

Do not add dummy functions or initializers to supply the 1. The flag is
evidence of a real predecessor, which must be recovered from other evidence.

## 7. Method

* **Static.** I read the driver and reader with `homm3 vc6 disasm` and
  capstone. `python3 -m homm3.vc6.cold_owner RVA…` assigns a hot function to
  a compiler TU through its cold-block jumps. C2 is block-reordered, so each
  TU's cold blocks and ICE sites stay in link order (see the `c2-atlas.md`
  §2 table). For example, `0x6819b` maps to `main.c`, `0x1994f` to
  `inline.c`, `0xa811` to `lg.c`, and `0x13615`, `0x11703` and `0x5739` to
  `globdf.c|globopt.c`. Contract tests: `test_cold_owner.py`.
* **Dynamic.** I used a scratch copy of the trace shim (`inline_trace.c`;
  not committed here, because that file belongs to the unstable-state lane).
  * **Read hook:** the hook at `0x5b11` replays `mov eax,[0x9f120]` and logs:
    * the read index, the value and the stage;
    * the return address of `0x5739`;
    * the compare tuple and its operands, with their kinds and constants;
    * the consumer and the block number (`block+0x6c`).
  * **Forcing:** `HOMM3_VC6_PHASE_READS=k=v,…` overrides the value used by
    chosen reads.
  * **Checkpoints:** whole-instruction hooks at `0x13615` and at the three
    writes log the block count, the loop list and `flowGraphDirty`.
    `HOMM3_VC6_SIBLING_ZERO`/`_ONE` set the sibling stage flags at driver
    entry for the transplant test.
  * **Inertness:** without these variables the run is unchanged. All 47
    `drawing` functions equal the normal build object. The `compile-m` sweep
    used the same scratch shim.

## 8. Open questions

* The final scheduling link in §5: which stage-1 pass turns the merged
  block into "all loads before the pushes"? The likely candidates are
  `0x6b7e` and the second `optimizeExpressions` operand ranks
  (regalloc.md §6n).
* What sets the body bit `+0x34 & 0x800`? At `0x687d8` it clears `0xac054`,
  so such a function would also skip the driver. None of the probe
  constructs set it.
* The exact source meaning of operand kind 2 in the 20 reads that compare
  a kind-2 operand with a constant.
* What retail compiled before `showCreatureSpellError`.
* Whether the retail terrain-mask tails, placed at the end of each object
  although declared in a header, indicate a deferred initializer form. That
  form would not precede a TU's first function.
