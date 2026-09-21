# Measuring tooling performance

## Reproduction

Run inside the pinned Nix build environment, using initialized disposable
worktrees. Run `homm3 build` in each before measuring non-build commands. Never copy `.ninja_deps` or `.ninja_log` between roots. The runner
requires two distinct Git worktrees and preserves cold-worktree artifacts for
inspection. Merge/cold cases require committed, clean tracked files.

```sh
PYTHONPATH=scripts python3 -m homm3.core.perf \
  --baseline /tmp/homm3-perf-baseline --candidate /tmp/homm3-perf-candidate \
  --scenario full --scenario fast --scenario audit-winmgr \
  --repeat 7 --profile --output "$PWD/build/performance"
```

Repeat with `source-edit-fast`, `source-edit-full`, `header-edit`, `merge-source`,
`merge-header`, or `cold` to cover workflow invalidation. Cold runs always use
three independent worktrees per revision, without warm-up. `source-cold` removes
only remote's disposable debug object, dependency file and stamp before each
invocation to measure the first `/Z7` compile; `source` measures its reuse.


## Measurement protocol

`python -m homm3.core.perf` alternates two prepared, disposable worktrees.
Ordinary timings invoke the actual CLI in fresh Python processes; profiling is
a separate pass. Seven paired repetitions follow an unmeasured warm-up for
warm commands. The runner saves individual elapsed times, medians, quartiles,
paired percentage reductions, exit statuses, CPU time and `wait4` peak RSS.
Peak RSS describes the command tree's largest process, not the sum of concurrent
process memory. Machine, Python/tool versions, worker defaults and revisions
are recorded alongside the raw results. Commands run sequentially.

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
