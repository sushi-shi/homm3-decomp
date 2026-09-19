# Tooling performance, 2026-09-19

Baseline: PR #23 at `a191f308`. Warm/source-merge series: `2df00698`; path-resolution follow-up and
header/cold series: `c2d085cf`. The only intervening production change reuses
resolved paths within each validation operation.
This work leaves the matching source, compiler flags, evidence gates and
CUR/MAX/HIST policy unchanged. No shared cross-worktree cache was added.

## Measurements

Seconds are uninstrumented medians. Reduction is the median of paired
percentage reductions; it need not equal the ratio of the two medians.
A negative reduction is a slowdown. Profile calls are collected separately.

| Workload | Pairs | Baseline (s) | Candidate (s) | Paired reduction |
|---|---:|---:|---:|---:|
| Warm full build | 7 | 32.276 | 22.721 | 29.9% |
| Unchanged fast build | 7 | 0.780 | 0.790 | -0.4% |
| Status | 7 | 0.343 | 0.388 | -12.0% |
| Summary diff | 7 | 0.463 | 0.463 | -0.1% |
| Cached source labels | 7 | 0.447 | 0.440 | -0.6% |
| First source labels | 7 | 1.366 | 1.329 | 2.3% |
| Single-function audit | 7 | 1.421 | 1.396 | 1.4% |
| bitmap16 audit | 7 | 1.552 | 1.173 | 24.8% |
| winmgr audit | 7 | 5.352 | 1.395 | 73.6% |
| Source edit, fast build | 7 | 1.342 | 1.368 | -2.6% |
| Source edit, full build | 7 | 32.006 | 22.166 | 29.9% |
| Build after source merge | 7 | 33.247 | 23.390 | 29.7% |
| Header edit, full build | 3 | 146.467 | 93.375 | 36.2% |
| Build after header merge | 3 | 145.691 | 105.028 | 28.8% |
| Fresh-worktree build | 3 | 159.316 | 108.214 | 34.1% |

| Workload | Python calls, before → after | C calls, before → after |
|---|---:|---:|
| Warm full build | 22,642,532 → 15,296,453 | 45,361,272 → 27,354,469 |
| Unchanged fast build | 830,944 → 1,311,051 | 1,297,704 → 1,357,221 |
| Status | 167,140 → 337,137 | 253,563 → 339,925 |
| bitmap16 audit | 2,936,083 → 2,215,531 | 3,577,590 → 2,840,695 |
| winmgr audit | 3,188,143 → 2,306,861 | 3,854,121 → 2,943,532 |
| Header edit, full build | 156,262,943 → 81,653,254 | 140,720,775 → 77,062,214 |
| Build after header merge | 156,263,426 → 81,654,255 | 140,721,473 → 77,063,867 |
| Fresh-worktree build | 157,993,687 → 89,112,259 | 144,281,757 → 94,479,780 |


Warm full builds avoid 2,385 normalization COFF-object parses. Ownership
collection drops from four invocations to three; shared Dreamcast origin and
Git-history reads each drop from two to one. Both header cases still scan all
151 ownership units, preserving conservative header invalidation. Clang launches
fall from 9 to 1 for bitmap16 audits and 17 to 1 for winmgr audits.

Both cold profiles capture 154 Python processes, including all 152 compiler
wrappers. Cold-worktree preparation takes about 0.16s per root, recorded
separately from build time. Individual cold runs span 157–182s baseline
and 104–120s candidate; these are local measurements, not universal timings.

The last driver-only commit rejects missing profiles and unsupported Python
versions instead of reporting zero calls. It does not change measured production
code or the instrumentation used in these completed captures.

## Costs and remaining work


Unchanged fast builds, individual diffs and single-function audits show little
wall-time movement. Status adds about 45 ms (paired median 12%) because it now
checks normalized output integrity. Fast/status Python-call totals increase;
these are not presented as call-count wins.

Warm full-build peak process RSS is about 246 MiB on both revisions. Header-edit
runs use 538 → 678 MiB, fresh-worktree runs use 532 → 637 MiB, and winmgr
audits use 153 → 161 MiB. Ownership indexes
live only for their scan, and audit AST reuse is bounded to eight groups per
invocation. These memory costs matter when several matching agents run at once.

The full build still launches 149 Clang frontend jobs. Source masking/tokenizing
also remains visible in the profiles. This change removes measured redundant
work without adding a shared cross-worktree cache or changing language/runtime.

## Measurement protocol

`python -m homm3.core.perf` alternates two prepared, disposable worktrees.
Ordinary timings invoke the actual CLI in fresh Python processes; profiling is
a separate pass. Seven paired repetitions follow an unmeasured warm-up for
warm commands. The runner saves individual elapsed times, medians, quartiles,
paired percentage reductions, exit statuses, CPU time and `wait4` peak RSS.
Peak RSS describes the command tree's largest process, not the sum of concurrent
process memory. Machine, Python/tool versions, worker defaults and revisions
are recorded alongside the raw results. Commands run sequentially. This host is
an AMD Ryzen 9 5900X (24 logical CPUs), using Python 3.13.13, Clang 21.1.8,
Wine 11.0 and Ninja 1.13.2. No CPU affinity or governor controls were imposed;
use the recorded distributions, not small differences in single timings.

The opt-in startup hook uses CPython 3.13's process-wide cProfile monitoring,
which includes Python worker threads. It also propagates into Python children,
including Ninja's compiler wrappers. A control proves worker and child calls
are counted exactly once. Interpreter/profiler setup and native child internals
are outside that profile; Python calls and C-function calls are reported
separately. Counts use cProfile records (Python records versus the `~` C-call
records) and include the small runtime subprocess/thread tracking wrappers.
Inclusive profile times overlap across threads and processes and
must not be summed into elapsed-time claims. External launches and their wait
durations are recorded separately.

Source/header workloads append comments outside function bodies. Repetitions
use distinct comments, identical between each baseline/candidate pair, so the
previous iteration cannot accidentally supply a warm ownership-cache entry.
The original source is restored in a finally block. Merge workloads use actual
Git fast-forward merges on detached benchmark HEADs; they never advance the
source branch. Header edit/merge workloads use three paired repetitions. Cold
runs create three new detached worktrees per revision and provision only
immutable game/toolchain inputs, with no copied Ninja dependencies, objects,
labels, debug objects, header mirror or ownership cache. OS page caches and
the already provisioned Wine prefix are warm. Worktree creation and synthetic
merge preparation are recorded separately from CLI elapsed time.

## Changes

- Preserve normalized comparisons across full delinking. Reuse requires hashes
  of all recorded inputs, the normalizer implementation, output object and
  symbol sidecar. Removed manifest units are pruned from the disposable cache.
  Old stamp schemas regenerate automatically and stamp writes are atomic.
- Replace the old process-global size/mtime digest memo with an explicit context
  owned by one normalization/validation operation. Writers invalidate changed
  outputs. A subsequent command or operation starts fresh, including when a
  merge/replacement preserves size and timestamp.
- Index source lines and Clang filenames once per ownership scan; reuse attached
  comment prefixes and pass the collected ownership snapshot to header
  projection. Header/config changes still invalidate all ownership scans;
  source-only changes retain the existing per-TU dependency checks. The build
  also shares its Git-history read and Dreamcast origin data between consumers. Location-only cleanliness evidence still excludes
  declaration-only records.
- Batch source-fact audits by translation unit and qualified owner. Filtered
  Clang dumps retain exact mangled-name/file selection, parse diagnostics,
  findings, coverage gaps and exit statuses. Single-function queries retain
  their narrow filter. The in-process AST cache is bounded to eight groups.
- Decode NB11 fields directly from their input buffer using bounded cached
  struct layouts, avoiding a byte slice for each field read.

## Validation

The 604-test non-VC6 tooling suite and 11 standalone freshness controls pass.
Forced normalization from the identical real raw inputs reproduces all 304
objects and 304 symbol sidecars byte-for-byte. Repairing either damaged side
of bitmap16, winmgr and RMG also reproduces the same paired results.

The full pinned build passes all gates with 4,083 / 4,765 functions exact and
95.96% fuzzy. Entire objdiff reports agree on all 4,765 function rows, and
the match ledger is byte-identical, including all eight timed/profiled fresh
worktrees. All 9,684 Dreamcast procedure records and 25,325 type records
compare equal to the baseline parser. Single-function, bitmap16 and winmgr audit
JSON compare equal after replacing only the worktree prefix in paths.

Ownership inventories agree on all 4,936 definitions and every ownership/type
field. Sixteen Clang anonymous-namespace manglings have the expected per-path
nonce difference between worktrees; their source identity and remaining fields
agree. Full-build matching uses the reviewed VC6 namespace normalization.

Controls cover same-size/same-timestamp input replacement, damaged normalized
outputs, missing/invalid provenance, removed units, exact overload selection,
incomplete Clang parses and profile coverage. Existing actual-Git source/header
merge and copied-cache controls remain in the passing tooling suite. Older VC6
experiment-fixture failures documented in the preceding review are outside this
change; no broad skips were added.

## Reproduction

Run inside the pinned Nix build environment, using initialized disposable
worktrees. Run `homm3 build` in each before measuring non-build commands. Never copy `.ninja_deps` or `.ninja_log` between roots. The runner
requires two distinct Git worktrees and preserves cold-worktree artifacts for
inspection. Merge/cold cases require committed, clean tracked files.

```sh
PYTHONPATH=scripts python3 -m homm3.core.perf \
  --baseline /tmp/homm3-perf-baseline --candidate /tmp/homm3-ff-normalize \
  --scenario full --scenario fast --scenario audit-winmgr \
  --repeat 7 --profile --output /tmp/homm3-perf-reproduction
```

Repeat with `source-edit-fast`, `source-edit-full`, `header-edit`, `merge-source`,
`merge-header`, or `cold` to cover workflow invalidation. Cold runs always use
three independent worktrees per revision, without warm-up. `source-cold` removes
only remote's disposable debug object, dependency file and stamp before each
invocation to measure the first `/Z7` compile; `source` measures its reuse.

## Local measurement artifacts

Individual samples, metadata, logs, raw `.prof` files and per-function profile
summaries remain locally under `/tmp/homm3-perf-final/`,
`/tmp/homm3-perf-path-fix/` and `/tmp/homm3-perf-workflows/`. Each directory's
`results.json` records timing samples and profile totals. Fresh-worktree build
artifacts are retained too. These temporary artifacts are not checked into Git;
the benchmark runner above reproduces them.
