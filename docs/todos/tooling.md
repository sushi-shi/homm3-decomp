# Tooling follow-ups

- [ ] Distinguish incomplete source-fact audits from exceptions in command
  telemetry while preserving CLI exit statuses.
- [ ] Review the build's explicit helper-emission debt separately from matching
  percentages; retain MAX/HIST while investigating absent emitted bodies.
- [ ] Review old experiment fixtures only when an unfinished match needs them.
  Retire completed experiments; do not restore obsolete game interfaces to
  satisfy their tests.
- [ ] Extend the explicit Ruff/Pyright scope in `scripts/homm3/core/lint.py` as
  production modules are reviewed. Run `python -m homm3.core.lint` in the pinned
  Nix environment; its current scope is not a repository-wide clean verdict.

Batch AST parsing, shared history reads and normalization reuse are already
implemented. Profile current workloads before choosing further optimizations;
see [measurement instructions](../tooling/performance.md).
