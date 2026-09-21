# VC6 compiler reference

These models come from the pinned compiler binaries and experiments compiled
with them. Predictions are hypotheses; the retail comparison using VC6 SP3 under
Wine remains the verdict. Do not use leaked compiler source as evidence.

| Topic | Reference |
| --- | --- |
| Compiler binary map and labels | [C2 atlas](c2-atlas.md) |
| Runtime libraries and strict linking | [Runtime link](runtime-link.md) |
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

Recovered source models and cleanup audits live in
[reconstruction](../README.md#reconstruction); outstanding work lives in
[todos](../README.md#outstanding-work).
