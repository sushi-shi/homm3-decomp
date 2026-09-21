# Generated C++ source families

## Current workflow

The general runner consumes JSON directly. Put a manifest under ignored `build/`
and name the affected units explicitly, including shared-header consumers:

```json
{
  "schema": 1,
  "units": ["your_unit"],
  "source": "src/your_unit.cpp",
  "axes": [{
    "name": "count_lifetime",
    "find": "    return items.size();",
    "options": [
      {"name": "unchanged"},
      {"name": "named_result", "replace": "    int count = items.size();\n    return count;"}
    ]
  }]
}
```

Replace the illustrative unit, source and statements with evidence-backed edits.
Each anchor must occur once. Axes must not overlap, and their first options must
preserve the source. An option's `extra_edits` couples edits across files; each
edit can override `source` and use `find`/`replace` or `insert_before`/`insert_after`
with `text`. Paths are relative to `HOMM3_DIR` and must stay under `src/` or
`include/`. `load_manifest` in `scripts/homm3/vc6/source_families.py` defines the
schema.

Unknown edit fields are rejected. Use `extra_edits` for coupled changes;
`edits` is not an alias and must not silently leave part of a candidate unchanged.

```sh
PYTHONPATH=scripts python -m homm3.vc6.source_families build/choices.json --validate-only
PYTHONPATH=scripts python -m homm3.vc6.source_families build/choices.json --width 60 --keep 10 --jobs 6
```

Run in the active worktree with `HOMM3_DIR` set there and fresh fast- or
full-build comparison objects. The unchanged-source control compares with the
live report, so a fast-build adoption does not require an intermediate MAX
checkpoint. The ledger still supplies projected MAX/HIST behavior. The driver
also verifies opposite-corner reproduction before searching. Adopt supported
source deliberately and finish with full `homm3 build`. Per-function mock
behavior suites are not prerequisites; use a temporary diagnostic only for a
concrete unresolved semantic question.

Source-model evidence is collected separately for [RMG](../reconstruction/rmg-source-models.md)
and [game helpers](../reconstruction/game-source-models.md). Those records describe
measured historical candidates; use current source and scores for new searches.
