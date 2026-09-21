# Plan: every retail byte, actionable ownership

## Requirements and acceptance evidence

1. Work in `/tmp/homm3-retail-byte-accounting` on `codex/retail-byte-accounting`.
   Preserve other worktrees and their uncommitted work.
2. Account for the complete pinned retail file, not merely data sections:
   DOS/PE headers, all section raw bytes, file gaps/overlay, executable bytes,
   resources, imports, initialized data, and raw alignment tails.
3. Separately account for every RVA in the image: mapped headers/sections,
   zero-filled storage and alignment/unmapped gaps. File and image domains have
   separate totals so shared backing bytes are not double-counted as progress.
4. Preserve every supported symbol/claim alias and shared storage. Distinguish
   compatible shared claims and structural containment from conflicting extents.
   Never promote candidate sizes, labels, string guesses or zero runs to proof.
5. Reuse admitted function/vtable/relocation inventories and source claims;
   identify bounded PE structures and validated compiler records where possible.
   Keep heuristic leads and missing evidence visible.
6. Produce standalone, actionable TSV rows with coordinates, region/storage,
   category/confidence, all owners and source/evidence, reference counts, byte
   preview and a concrete next action. Export detailed claim/reference tables
   and a machine-readable summary. A row must be usable without chasing numeric
   claim IDs into a separate JSON file.
7. Verify exhaustive, nonoverlapping partitions in both coordinate domains,
   exact pinned totals, aliases, contradictory overlaps, boundary conditions,
   malformed extents, stale/missing optional evidence and unknown regions.
   Add defect-injection tests; unknown remains an explicit backlog rather than
   false success at semantic reconstruction.
8. Generate the retail TSV artifacts, inspect representative rows across all
   families, record measured coverage and known limits, and commit the tooling,
   tests and documentation in the dedicated worktree.

## Implementation sequence

- Audit existing coverage and the older data census. Keep useful data/reference
  extraction but replace its data-only denominator and opaque TSV claim IDs.
- Implement a validated PE layout with exhaustive file and image regions.
- Assemble independently evidenced extents plus optional navigation/provisional
  layers; partition them with shared ownership preserved.
- Export actionable TSVs and summary with fixed denominators and evidence hashes.
- Run focused and integration tests against the verified retail executable;
  audit every acceptance requirement above before declaring completion.

The prior turn was progress: it added data-region accounting and exposed linking
and enrollment defects. It did not meet this full-image objective. The older
`tooling/data-matching` worktree contains a separate referenced-address census;
its source-derived extents are provisional and it must remain untouched.

## Completion audit

- **Worktree isolation:** implementation and reports live in
  `/tmp/homm3-retail-byte-accounting`, branch `codex/retail-byte-accounting`.
  Other checkouts and their prior uncommitted work were preserved.
- **Complete file/image scope:** `retail_layout.py` partitions the full PE file
  and `[0, SizeOfImage)`. An independent verifier read the exported TSV, marked
  each coordinate in a byte bitmap, rejected duplicate marks and asserted every
  mark was present: **2,732,032 file bytes and 2,842,624 image RVAs verified**.
  It independently checked raw/RVA projections and every exported byte preview.
- **Ownership/sharing:** alias and overlap regression tests pass. The retail
  export retains 1,498 bytes with multiple recorded symbol spellings. Reviewed
  partial sharing, identical extents and contained EH handlers are distinguished
  from incompatible overlaps. `labels.tsv` preserves spelling provenance;
  anonymous-namespace spelling aliases do not imply distinct source objects.
- **Evidence reuse:** admitted functions/vtables, bounded PE imports/resources,
  validated EH records, source-label snapshots and all 57,088 reference evidence
  sites are represented. Unknown and provisional extents remain explicit.
- **Actionable artifacts:** `coverage.tsv`, prioritized `backlog.tsv`,
  `claims.tsv`, `labels.tsv`, `references.tsv` and `summary.json` were generated
  under `build/retail-accounting`. All TSVs were independently parsed using the
  documented literal-tab dialect. Coordinates, owners, source paths, evidence,
  previews and next actions are available in the main TSV without a JSON join.
- **Tests/gates:** all 108 sema tests pass, including malformed PE layouts,
  resource cycles/escapes, invalid EH targets/maps, absent optional labels,
  alias preservation, contradictory extents, zero tails, leading/trailing gaps,
  TSV escaping and denominator defect injections. Ruff and diff whitespace
  checks pass. `--require-complete` correctly returns 1 while semantic unknowns
  remain; ordinary coverage, JSON summary and legacy data-only modes succeed.
- **Artifact integrity:** the independent audit verifies all 57,088 reference
  operand values against retail and every input/implementation content hash.
  Representative rows cover headers, every PE section, raw tails, zero-fill,
  image gaps, recognized/provisional objects and unknown regions. Its disposable
  script and `verification.json` remain alongside the reports under `build/`.

This completes tooling and exhaustive accounting, not semantic identification
of all objects. The remaining 248,141 unknown file bytes (358,733 image RVAs)
and 84,398 provisional bytes are the explicit, actionable reconstruction
backlog. File and image counts must never be added together.
