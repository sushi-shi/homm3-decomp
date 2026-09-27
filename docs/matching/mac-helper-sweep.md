# Mac retained-helper sweep

The current pass restores source helper boundaries and calls across all Windows
game functions, including byte-exact callers. A lower byte score does not reject
a helper supported by the available source and compiler evidence. Byte polishing
follows the broad pass.

Use clear project names when Mac shows an operation but not its original C++
spelling. Restore the helper and its call sites promptly, including when Mac
inlines the operation too; an original symbol or immediate score gain is not
a prerequisite. Keep one canonical body and refine names or placement later if
new evidence warrants it.

During subsequent byte recovery, keep these helper calls. Do not replace them
with direct fields, array indexing, or pasted statements to recover a score.
An outer helper is permitted when it contains the recovered helper call; review
that complete source path. Tune natural argument evaluation, local lifetimes,
body visibility and source order around the preserved calls.

Use `homm3 mac calls <unit>` and `homm3 mac disasm <Windows-VA>` on claimed
callers. Direct Mac branches resolve to source claims and runtime labels; an
unnamed destination needs an identity review. These are leads for human
review, not proof of an inline qualifier or even a game helper. A Mac inlined
operation may have no retained call and therefore needs a body-shape review
too. The `rmg`, `zlib-1.1.3`, `codec`, and `victor` modules are deferred by
user direction.

For each caller, inspect Mac disassembly and direct targets; use Dreamcast
source facts when they are already available. Restore one canonical helper body
and source call in its plausible owning TU/header. The name may be ours when
Mac shows the operation but does not preserve a source symbol. A field load,
array index, or sequence of stores in Mac can be an expanded helper. Do not
remove an authored helper call solely because the PEF has no retained call,
including in Windows functions already at 100%. Check Mac body order
separately from cross-TU VC6 expansion when deciding header placement.
Record unresolved destinations instead of silently counting them as absent
helpers. Batch the helper edits without per-function score tuning or
per-helper builds; compilation and byte comparisons follow the broad
restoration pass.
