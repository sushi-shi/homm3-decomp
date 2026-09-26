# Helper sweep checkpoint — 2026-09-26

## State

All five workers finished their current batches and stopped. Their submitted
source commits are integrated through `56059c0b2`. The broad helper sweep is
paused, with outstanding work preserved. No game rebuild or score polishing
was performed during this sweep. The next change adds generated reference
reports; it does not resume the workers.

Comparison base: `7b303bedf`, the start recorded in the sweep assignment.
Integration branch: `codex/helper-sweep-20260926`.

## What changed

- Restored canonical helper calls throughout serializers, world/hero accessors,
  UI and combat paths. The final world report records **501 restored uses of
  existing helpers** and four new game helper bodies. The AI report records
  **93 restored call sites**. Together these two lanes alone account for **594
  restored uses**; this is a conservative subtotal, not the entire sweep total.
- Campaign, UI, systems and root also restored helpers and uses, corrected
  overloads and receiver/argument expressions, and added source-owned Mac
  identities. Their incremental counters are not a reliable deduplicated total,
  so they are not added to the subtotal above.
- Removed score-only inline controls where the recovered source model supported
  ordinary helper calls. Helpers remain in source despite unmeasured score
  changes.
- Included source-only helpers without their own Windows VA in the inventory.
  An address mapping or newly inventoried helper is not necessarily newly
  recovered C++.
- Preserved unresolved byte-order, native-memory, dispatch and semantic cases.
  In particular, the CodeWarrior probe did not justify treating `memset` /
  `memcpy` as automatically equivalent to retained `bzero` / `BlockMoveData`.

## Inventory and review progress

These first two columns use the old inventory snapshots so their definitions
remain visible. The expanded scope prevents interpreting raw discrepancy-count
changes as regressions.

| Measure | Start | Latest inventory |
| --- | ---: | ---: |
| Windows functions in scope | 4,768 | 4,768 |
| Additional source-only helper caller entries | Not included | 714 |
| Paired Mac callers exposed by inventory | 3,355 | 4,106 |
| Direct Mac sites exposed by inventory | 42,135 | 44,185 |

Final collected worker/root evidence covers **41,282 active call-site rows**:

| Disposition | Sites |
| --- | ---: |
| Correspondence reported | 33,052 |
| Explicitly unresolved | 814 |
| No individual site evidence | 7,416 |

Thus **8,230 rows still lack a reported correspondence**, before auditing older
claims against the stricter rule. “Correspondence reported” is a worker finding,
not verified completion: legacy runtime classifications and nested-path claims
can require further investigation. This is a call-site inventory, not a count
of unfinished functions. Unpaired functions and entirely Mac-inlined helpers
also need coverage. Deferred modules retain their deferred status.

## Tooling delivered

`homm3 mac helper-audit` now builds the reference inventory from the pinned PEF
and the ordinary Windows source AST. It generates both caller and callee queues,
exact overload/qualifier identities, source expressions and locations, repeated
site counts, source paths, raw branch references, and loader/TOC xrefs.

```sh
homm3 mac helper-audit
homm3 mac helper-audit mac:0:0xc7f1c --json
homm3 mac helper-audit --unit fly
```

Full output: `build/mac/helper-audit.json`, `helper-audit-calls.tsv` and
`helper-audit-source.tsv`. Partial unit runs have separate output names.
Source/header/profile changes invalidate the AST caches. The temporary
`mac-helper-sweep` skill now uses these generated queues instead of manually
maintained caller lists. See [the tooling guide](../tooling/mac-helper-graph-plan.md).

The corpus run selects all **138 admitted game source units**:

| Generated observation | Count |
| --- | ---: |
| Source call sites | 45,344 |
| Mac sites in source-owned callers | 44,189 |
| Deferred sites within that Mac queue | 2,698 |
| Direct source/Mac counts agreeing | 17,061 |
| Nested source paths requiring inline evidence | 1,156 |
| Mac sites with no corresponding source call found | 546 |
| Mac sites in repeated-target count discrepancies | 448 |
| Mac sites lacking a resolved source callee identity | 21,752 |
| Units with Clang errors | 27 |

These are generated graph observations, not the same disposition categories as
the historical review table. Missing joins,
Clang diagnostics, virtual targets, differing TU views and implicit cleanup
remain explicit. It does not fabricate source/library identities from matching
names. A nested path requires Mac-inline evidence; an equal count does not
verify receiver, arguments or control flow. This is usable generated tooling
with documented coverage gaps, not proof that every operation is recovered.

Validation: **14 focused tooling tests passed**. Source-graph and joined-report tests cover overloads,
constructors/operators, repeated sites, reverse references, tail/indirect
branches, cycles, differing TU views, partial outputs and cache invalidation.
All **4,110 source Mac claims** also passed ownership/span validation.
No VC6 or CodeWarrior game build is needed to generate these reports.

## Matching: last measured checkpoint

| Metric | Start | Latest stored checkpoint |
| --- | ---: | ---: |
| Windows weighted MAX | 97.36% | 97.36% |
| Exact MAX functions | 4,307 / 4,768 | 4,307 / 4,768 |
| Weighted CUR | 97.09% | 97.09% |
| Exact CUR functions | 4,268 | 4,268 |
| Weighted HIST | 98.21% | 98.21% |
| Exact HIST functions | 4,377 | 4,377 |

Neither match baseline changed during the sweep. **These unchanged stored
numbers do not measure the edited source.** Whether current matching rose or
fell is unknown until the next game build. There are 461 functions below MAX
100% in that stored checkpoint; the helper-recovery queue includes exact
functions too and has a different denominator.

## Resume

Keep the workers stopped until requested. The generated report is the starting
inventory for the next pass. First address missing joins and parsing/lifetime
coverage where they hide callers, then restore the remaining supported helper
uses across all xrefs. Preserve canonical helpers through later score recovery.
The only accepted structural mismatch is an evidenced Mac-inline expansion;
conflicting semantics remain open for review.

Local evidence is retained under `build/helper-sweep/` in the integration
worktree and each of the five worker worktrees. `current-review-progress.json`
and `current-call-evidence.json` hold the final collected dispositions; worker
handoffs and `results.json` preserve the individual evidence. Those generated
and experimental artifacts remain ignored; recovered source and reusable
tooling are tracked.
