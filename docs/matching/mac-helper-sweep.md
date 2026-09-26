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

Run `homm3 mac helper-queue --all-functions` for the broad sweep. It writes
`build/mac/helper-queue-all.json`, `helper-queue-all-functions.tsv`, and
`helper-queue-all-calls.tsv`. The function file includes exact and unfinished
Windows targets and marks whether each has a reviewed Mac span. The call file
lists every direct Mac branch in those spans. Without `--all-functions`, the
command retains its smaller unfinished-function matching queue.
`review_missing_helper_call` means a reviewed
source helper is absent from the caller's authored body; `identify_target`
means the destination has no reviewed identity. Other named targets get
`review_other_named_call`, since constructors and destructors can be implicit.
These are leads for human review, not proof of an inline qualifier or even a
game helper. A Mac inlined operation may have no retained call and therefore
needs a body-shape review too. A textual `source_call_present` check also needs
inspection when overloads or macros are involved. Reviewed Mac
runtime destinations are marked `runtime_call` and remain in the full report
without crowding the displayed helper leads.

The command display groups repeated call sites by destination and orders them
by distinct Windows callers. `--limit` counts destinations; the TSV
keeps every individual call site. The display skips user-deferred modules unless
`--include-deferred` is supplied; the JSON and TSV retain them. Use
`--include-named` to show named Mac callees absent textually from the immediate
caller. These often come from an authored nested helper, default argument, or
implicit constructor; inspect that path before editing the caller.

Use `--owner worker=game,townmgr` when owner labels help coordinate parallel
work. Unassigned rows remain visible. The `rmg`, `zlib-1.1.3`, `codec`, and
`victor` modules are deferred by user direction.

For each caller, inspect Mac disassembly and direct targets; use Dreamcast
source facts when they are already available. Restore one canonical helper body
and source call in its plausible owning TU/header. The name may be ours when
Mac shows the operation but does not preserve a source symbol. A field load,
array index, or sequence of stores in Mac can be an expanded helper. Do not
remove an authored helper call solely because the PEF has no retained call,
including in Windows functions already at 100%. Check Mac body order
separately from cross-TU VC6 expansion when deciding header placement.
Record unresolved destinations in the queue instead of silently counting them
as absent helpers. Batch the helper edits without per-function score tuning or
per-helper builds; compilation and byte comparisons follow the broad
restoration pass.
