# Command telemetry

Every `homm3` CLI invocation appends a copyable command to
`build/homm3_usage.log` and a structured completion record to
`build/homm3_usage.jsonl`. This includes build/init failures and CLI parsing
errors. The existing sema, vc6 and Dreamcast logs remain available, including
when those modules are invoked directly with `python -m`.

The JSONL schema is `homm3.usage.v2`. Each record contains:

- `event_id`: a UUID for this invocation. Copied logs retain the same ID;
  deduplicate copies by ID, not by command text or timestamps.
- `parent_event_id`: the enclosing logged invocation, when present. The outer
  CLI passes this correlation to its child process. CLI and module records
  describe different scopes of the same operation: use `scope: "cli"` for
  CLI usage totals; include root module events separately for direct usage.
- `worktree`: absolute repository root; `git_dir`: Git's absolute per-worktree
  administrative directory; `cwd`, `pid`, and `revision`. Git metadata can be
  null outside an initialized checkout. These identify where an event ran
  even after its log is copied elsewhere.
- `started_at`, `time` (completion), and `duration_seconds`. Timestamps are UTC
  with an explicit offset. New legacy text log timestamps are also UTC;
  historical text logs may use the process's local time zone.
- `is_test` and `test_marker`: explicitly identify test/probe invocations.
- `rc`, `outcome`, `error_category`, and bounded diagnostic text in `error`.

Set a descriptive test marker for deliberate negative controls:

```sh
HOMM3_USAGE_TEST=invalid-selector-control homm3 sema diff 0xzz --json
```

The marker is inherited by child processes. Ordinary failed matching attempts
are not automatically marked as tests. Existing unmarked historical records
cannot be reliably separated after the fact.

For sema/vc6/Dreamcast, rc=1 is an answered difference, not a tooling error.
Build/init and other pipeline failures may use rc=1. `outcome` records that
command-family distinction. Exceptions and process signals are errors.

Error categories are best-effort classifications of diagnostic text:
`environment.permission`, `environment.wine`, `compile.cpp`,
`candidate.unavailable`, `candidate.object_missing`, `candidate.symbol_missing`,
`candidate.stale`, `build.target`, `cli.arguments`, `tool.exception`,
`interrupted`, `process.signal`, and fallback `command.failed`. In particular,
Ninja's generic failure alone does not establish a C++ compilation error.
Environment evidence takes precedence over a wrapping traceback. Retain and
inspect the diagnostic text when the cause is ambiguous.

The CLI streams child stdout and stderr to their original destinations and
observes both, because VC6 emits compiler errors on stdout. Routine successful
output is not stored; failed calls retain at most 16 KiB from each stream.
Python exceptions retain their traceback. Logging is best-effort and must not
change a command's return code if writing telemetry fails.

These are completion records: a SIGKILL or machine failure can leave no event.
The revision is sampled at completion and is not a fingerprint of dirty source.
Direct invocations of lower-level build modules or Ninja are covered only when
run beneath the CLI. The build directory remains disposable; preserve logs
outside it when cleaning the worktree. Changes here do not retrofit older
running worktrees until they receive the updated tooling.
