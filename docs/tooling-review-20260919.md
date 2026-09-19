# Tooling review, 2026-09-19

Reviewed the tree squash-merged by PR #21 (`e57986b0`), command telemetry across
worktrees, and project-specific agent transcripts. No Rust rewrite is needed
for the first improvements: correctness and redundant compiler work dominate.

## Changes in this patch

- **Reject stale score inputs.** `status` now checks normalized-object provenance
  before invoking objdiff. All status views also ask Ninja whether the source,
  headers or compiler commands need rebuilding; `update` verifies provenance
  again before writing. Previously, a raw-object edit could leave status at 100%
  while a fresh comparison scored 99.96032%, and `update` could bank that old
  100% under the edited source's new hash. Read-only views refuse unbuilt sources
  too: agreeing raw/normalized objects can both predate a merge. Ninja's pending
  work is identified through a controlled `NINJA_STATUS` marker, not its human
  no-work message. CUR/MAX/HIST evolution is unchanged.
- **Finish commands when the output reader closes.** Piping output to `head`
  previously caused forwarding-thread exceptions and Python exit 120. The
  logger and forwarding threads now discard output after a broken pipe while
  draining the child and retaining its actual exit status. This lets builds
  finish their gates even when a reader stops displaying their output.
- **Check required inputs before a full build.** Missing or invalid retail and
  Dreamcast executables fail before compilation, delinking or ledger changes,
  with the existing initialization instructions. Fast builds keep their
  existing input requirements.
- **Cache ownership scans per translation unit.** A source-only edit invalidates
  that unit and any units including it. Header/configuration changes still
  invalidate all units conservatively. Compiler include enumeration tracks
  macro-only headers and included `.cpp` files. Tool implementation, library
  identity, manifest, vendored headers and mirrored VC6 headers participate in
  invalidation. Entries with parse errors are retried; damaged entries are
  rebuilt; writes are atomic. `--fresh` still bypasses the cache.
- **Reject unknown status commands immediately.** A mistyped subcommand no
  longer generates an objdiff report before printing usage.
- **Remove repeated source and object parsing.** A full build derives current
  and legacy fingerprints in one scan and shares them between check/update.
  Symbol demangling has a bounded string-keyed memo; object authority parsing
  has a bounded byte-keyed memo. Every authority lookup still reads the object,
  and callers receive private containers. Neither memo persists across commands.
- **Repair current RMG experiment generators.** Transition-strength variants
  now use the current coordinate accessors and canonical `getFrame` wrapper.
  Both strength and cache-fill helper-order families retain terrain/frame
  wrappers together, preserving each existing body and annotation. They keep
  their guards against unexpected source changes.

## Evidence from usage

Telemetry was deduplicated by event ID and restricted to root CLI events,
excluding marked test invocations and help requests. The September 14–19
snapshot contained 5,004 commands. Historical unmarked experiments may remain;
an error exit is not necessarily a tooling bug.

| Command | Calls | Median | Total recorded duration |
| --- | ---: | ---: | ---: |
| `dreamcast audit` (mixed selector scopes) | 345 | 1.60 s | 5.38 h |
| Full build | 98 | 145.38 s | 2.79 h |
| `sema diff` | 4,049 | 0.47 s | 40.4 min |
| Fast build | 155 | 1.88 s | 5.17 min |

The audit distribution mixes individual functions, modules and whole-corpus
audits; its high tail must not be presented as single-function latency.
Of the recent audit errors, 217 were generic command failures, mostly coverage
gaps. Exit 2 explicitly includes incomplete evidence, so these are not 217
software crashes. Recent sema logs included 61 exception-classified failures;
broken-pipe failures were a recurring example, independently reproduced here.

Project transcripts repeatedly showed `homm3 build ... | tail`, filtered sema
output, and guessed commands such as `sema asm`, `vc6 inline-trace`, and
`status show`. Agents expect concise views, recognizable command names and a
trustworthy command result after filtering. Shell pipelines still require
`pipefail` if the caller needs the tool's exit code rather than the last filter's.

## Further work, in priority order

1. **Batch source-fact parsing.** `analysis/source_facts.py` starts Clang once
   per audited function. A whole-TU/class AST index can amortize compiler work.
   Preserve exact mangled-identity/file checks, incomplete-parse diagnostics,
   source-call ordering and all existing negative controls. Measure AST size
   before replacing filtered dumps with an unrestricted whole-TU JSON dump.
2. **Repair stale experiment generators when needed for matching.** Before this patch, full unittest
   discovery ran 918 tests: 12 failures, 46 errors and one skip. Failures were
   confined to `homm3.vc6`; examples include generators expecting old RMG source
   spellings, duplicate-field regex matches, missing native-fixture `VA` macros,
   and native compilation failures. The strength/cache-fill generators are now
   repaired; their six checks, including native behavioral oracles, pass.
   Other old VC6 failures remain. Update generators against the current proven
   interfaces; do not restore obsolete game interfaces just to pass fixtures.
3. **Make incomplete audits distinct in telemetry.** Preserve the documented
   CLI exit statuses, but distinguish coverage gaps from exceptions in structured
   logs. Otherwise apparent failure rates exaggerate tool instability.
4. **Expose missing helper emissions in a separate review view.** A generated
   helper may lose its emitted body while its RVA and banked MAX/HIST survive.
   The existing ledger intentionally preserves those peaks and ignores unrelated
   CUR movement. PR #22 demonstrated why reviewers also need an explicit emission
   comparison; this should not silently change the MAX policy or make score dips
   fatal.
5. **Reduce repeated history and delink work.** Status and the banked-row gate
   independently walk baseline history. Full builds rebuild retail comparison
   artifacts even when inputs are unchanged. Cache only with complete identities
   for history, retail bytes, claims, manifests and tool implementations.

Rust remains an option for a measured CPU hotspot. It cannot remove repeated
Clang invocations or improve the invalidation algorithm by itself. A profiled
warm ownership audit spent about 2.3 seconds reading Dreamcast evidence and
1.2 seconds comparing ownership; those are much smaller than a cold corpus
scan. Optimize the workflow first, then reprofile.

## Validation

Negative controls cover stale raw objects, missing stamps/targets, unbuilt
sources, header and command changes, early-closing real pipes with successful
and failing children, missing executable inputs, cache corruption, parse errors,
forced scans, and real Clang include dependencies. Full-project build and timing
results:

- 599 tests across all non-VC6 Python tooling packages and six checks for the
  repaired VC6 experiment generators pass. Other pre-existing VC6 fixture
  failures above are outside that passing set.
- Full pinned build passes all gates: 4,083 / 4,765 functions exact, 95.96%
  fuzzy; no score or baseline changes; 4,936 ownership definitions, no violations.
- A real `bitmap16.cpp` comment-edit benchmark preserved every definition and
  rescanned only that TU. The temporary comment was removed and original source
  bytes and timestamps restored. Serialized definitions also match the complete
  pre-change ownership cache, not just the count.

| Ownership collection | Elapsed | Units/headers reparsed |
| --- | ---: | ---: |
| Cold cache | 119.60 s | 151 |
| One C++ source edit | 0.62 s | 1 |
| Restore that source | 0.66 s | 1 |
| Unchanged warm cache | 0.58 s | 0 |

These are ownership-collection timings on this machine, not whole-build speedups.
The old global source key required a full scan after any source edit. Shared
header or parser changes still require that scan with the conservative new key.
The whole-project controls caught nested CRT includes and relative Miles/IFC
vendor includes; both now participate in cache identity and dedicated tests.

Actual Git merge controls now populate the cache, merge source or header changes,
and compare the next cached collection against a forced fresh scan. They retain
the original file size and timestamp to prove invalidation uses contents.
Header-only merges invalidate both fixture consumers despite unchanged `.cpp`
files. A copied-cache control proves another worktree requires fresh scans.
The cache includes the absolute worktree root, so these timings do not promise
reuse of another worktree's ownership cache.

## Profile-guided improvements: frequent worktree creation and merges

A full warm-build cProfile and a cold RMG ownership scan identified work that
does not require a persistent cache shared between worktrees:

- The build called `source_hashes` four times (current/legacy for check/update),
  accounting for 13.83 seconds under the profiler. It now computes both
  fingerprints from one source scan and passes them to both consumers.
- The build called `_base_authority_scan` 484 times for 152 units (14.86 profiled
  seconds). It now reuses each object's parsed authority within that invocation,
  keyed by the actual object bytes rather than its filename or timestamps.
- `_demangle_key` handled 157,527 calls (12.64 profiled seconds, overlapping the
  authority scans). Its result depends only on the symbol string. A separate
  experiment over 33,448 real symbol inputs reduced 0.652 seconds to 0.235 seconds
  with a bounded in-process memo, producing identical results. That memo is
  now implemented; it does not cache filesystem state.
- A cold RMG scan took 1.68 profiled seconds: 0.34 seconds in repeated path
  relativization and 0.21 seconds splitting strings. Reuse path and line indexes
  within the parse; that also helps the first build in a new worktree.

These profiled times include instrumentation overhead and overlap; they cannot
be added into a promised speedup. A direct comparison of all 152 object
authorities and every current/legacy source fingerprint found identical outputs.
An unprofiled pair of fingerprint scans took 4.85 seconds before, versus 3.09
seconds for the combined scan; full builds additionally avoid repeating that
pair. One authority pass took 0.84 seconds before; the optimized first pass took
0.42 seconds and the repeated pass 0.012 seconds. An object mutation preserving
size and timestamps is observed immediately, and mutating returned authority
containers cannot poison future lookups.

An unprofiled warm full-build run fell from **41.26 seconds to 31.73 seconds**
(23% less elapsed time) on this machine. Both runs used the same compiled
sources and passed every gate with all 4,765 scores unchanged. The post-change
ownership refresh was completed before timing, so this is a warm-build
comparison, not a cold-worktree or compile-time claim.

Follow-up measurements and implemented optimizations are recorded in
[Tooling performance, 2026-09-19](tooling-performance-20260919.md).

## Repeatable Python checks

Inside `nix develop .#build`, run `python -m homm3.core.lint` for Ruff's default
rules and Pyright diagnostics. Both tools are supplied by the pinned Nix shell.
The command checks the eight production modules listed in `core/lint.py`,
including itself; Ruff also checks the three listed regression-test modules.
This is an explicit initial scope, not a repository-wide clean verdict. Extend
the lists as other tooling modules are reviewed.

Pyright receives the active interpreter's dependency paths through a temporary
configuration, so Nix's libclang package is resolved without committing a
machine-specific store path. Either check failing makes the command fail, and
Ruff findings do not prevent Pyright from running. No diagnostic suppressions
are applied.
