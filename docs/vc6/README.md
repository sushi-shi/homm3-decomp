# VC6 compiler reference

These models come from the pinned compiler binaries and experiments compiled
with them. Predictions are hypotheses; the retail comparison using VC6 SP3 under
Wine remains the verdict. Do not use leaked compiler source as evidence.

| Topic | Reference |
| --- | --- |
| Compiler binary map and labels | [C2 atlas](c2-atlas.md) |
| Driver options and compiler passes | [Driver passes](driver-passes.md) |
| Intermediate language | [IL format](il-format.md) |
| Inlining | [Inliner](inliner.md), [optimization scope](optimization-scope.md) |
| Register allocation | [Register allocator](regalloc.md), [handle order](handle-order.md) |
| Control flow and object lifetimes | [Control flow](control-flow.md), [EH cleanup](eh-cleanup.md) |
| Candidate debug information | [Debug lines](debug-lines.md) |
| Variadic member functions | [Variadic members](variadic-members.md) |
| Compiler generation comparison | [RTM generation](rtm-generation.md) |
| Compiler instrumentation | [Shim](shim.md) |
| Known compiler behavior | [Behavior catalog](behavior-catalog.md) |
| Victor library compiler findings | [Victor](victor-library.md) |

For experiments, see [source families](source-families.md),
[source hypotheses](../matching/source-hypotheses.md) and
[TU state sweeps](tu-state-sweep.md). Keep generated results under `build/`.

`homm3 sema diff --asm` masks stack-slot displacements. A 99.98% byte result
can therefore show identical masked assembly and CFG while two locals occupy
the opposite stack slots. In `game::loadMinePool` at 0x004b9340, raw base and
target COFF disassembly showed the signed guard bytes swapped at `[ebp-1]` and
`[ebp-2]`; `--why-bytes` reported the first alignment as a missing instruction.
Inspect the raw normalized objects for this specific residual before changing
control flow or inventing an operation.

Recovered source models and cleanup audits live in
[reconstruction](../README.md#reconstruction); outstanding work lives in
[todos](../README.md#outstanding-work).
