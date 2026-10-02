# Tooling follow-ups

- [ ] Distinguish incomplete source-fact audits from exceptions in command
  telemetry while preserving CLI exit statuses.
- [ ] Review the build's explicit helper-emission debt separately from matching
  percentages; retain MAX/HIST while investigating absent emitted bodies.
- [ ] Review old experiment fixtures only when an unfinished match needs them.
  Retire completed experiments; do not restore obsolete game interfaces to
  satisfy their tests.
- [ ] Resolve the remaining equivalent-address relocation in
  `type_dialog_icon::set` at `0x004f4eb0+0x62`: the candidate's
  `g_townBuildingSpriteNames - 0x58` and retail's `HiScCam.def` label both
  denote RVA `0x27f520`. The latter has a proven `hiscore` unit-copy identity
  but no `symbol_names.csv` entry, so equivalent-data-address normalization
  cannot currently use it. Preserve unit scoping and verify the retail
  operand when extending this comparison; do not rewrite the game expression.
- [ ] Extend the explicit Ruff/Pyright scope in `scripts/homm3/core/lint.py` as
  production modules are reviewed. Run `python -m homm3.core.lint` in the pinned
  Nix environment; its current scope is not a repository-wide clean verdict.

Batch AST parsing, shared history reads and normalization reuse are already
implemented. Profile current workloads before choosing further optimizations;
see [measurement instructions](../tooling/performance.md).
