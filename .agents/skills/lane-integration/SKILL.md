---
name: lane-integration
description: Integrate parallel HoMM3 matching lanes while preserving source-owned MAX/HIST score evidence and validating the merged build. Use for rebasing, reviewing, or merging worker branches and stacked matching PRs.
---

# Integrate a matching lane

Read `AGENTS.md` and the active branch's worktree status first. Keep each lane's
build directory separate. Preserve source-supported helper and layout changes
through incidental CUR dips; inspect the actual authored source and target bytes
when a lane reports a loss.

Use the lane's targeted build results for routine integration. For a specifically
requested baseline investigation, this command compares its compiled report
with the parent branch's committed ledger:

```sh
homm3 status check --baseline-ref <parent-ref>
```

RESET means the function's own source changed while CUR held against that
snapshot; the new implementation has not reproduced an older MAX. CHANGED-CUR
means the edited function's emitted score moved and needs a byte or source
review. Unchanged-source CUR dips retain MAX. Score events are observational;
evidence and source gates decide whether to keep a reconstruction.

When `config/match_baseline.tsv` conflicts during rebase, finish source
conflicts and use `homm3 status merge-baseline` to merge Git's three stages
per row. It carries earned MAX and HIST without treating ordinary CUR snapshot
drift as an earned change. Inspect rows where both sides changed the same
function, especially differing source hashes or RVAs, before staging. The
command refuses conflicting retail RVA bindings. Do not take either whole
ledger side.

After resolving source conflicts, use `homm3 build --fast <TU>` for the affected
function's TU when its compiled result needs refreshing. For renamed compared
symbols, compile their owning TUs and use `homm3 delink --unit <TU>` (repeatable)
to refresh target labels without the global label self-test/completeness gate.
Then run the affected fast comparison. Regenerate README with
`homm3 status update --write-readme` to bank the measured scores, then commit and push under the user's existing
authorization. Do not run routine full builds, tests or standalone validation
checks. README's table is MAX and its footer reports CUR/MAX/HIST. Workers return
their commits for coordinated integration and publishing.
