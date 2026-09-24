# Mac retained-helper sweep

The current pass restores source helper boundaries and calls across all Windows
game functions, including byte-exact callers. A lower byte score does not reject
a call supported by Mac, Dreamcast, and VC6 evidence. Byte polishing follows
the broad pass.

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
These are leads for human review, not proof of an
inline qualifier or even a game helper. A textual `source_call_present` check
also needs inspection when overloads or macros are involved. Reviewed Mac
runtime destinations are marked `runtime_call` and remain in the full report
without crowding the displayed helper leads.

The command display groups repeated call sites by destination and orders them
by distinct Windows callers. `--limit` counts destinations; the TSV
keeps every individual call site. The display skips user-deferred modules unless
`--include-deferred` is supplied; the JSON and TSV retain them.

Use `--owner worker=game,townmgr` when owner labels help coordinate parallel
work. Unassigned rows remain visible. The `rmg`, `zlib-1.1.3`, `codec`, and
`victor` modules are deferred by user direction.

For each caller, inspect Mac disassembly and direct targets, Dreamcast source
calls and lines where available, and the Windows expansion. Restore one
canonical helper body and source call in its plausible owning TU/header. Check
Mac body order separately from cross-TU VC6 expansion when deciding header
placement. Record unresolved destinations in the queue instead of silently
counting them as absent helpers. Run a focused build to ensure the recovered
source compiles; the full build and tests wait for integration.
