---
name: match
description: Reconstruct and byte-match HoMM3 functions or translation units against retail using Dreamcast evidence, VC6, and JSON source-family searches. Use for locating, claiming, matching, or closing a TU.
---

# HoMM3 matching

Recover plausible C++ that reproduces retail bytes. Read the active repository's
`AGENTS.md` for evidence, source ownership, naming and admissibility rules.
VC6 under Wine decides codegen; clang/clangd supports source analysis and editing.
Use the pinned per-TU profiles in `config/units.toml`, not hand-picked flags.

## Worktree and baseline

Set `HOMM3_DIR` to the active worktree and `PYTHONPATH` to its `scripts/` directory
inside the build shell. The shell can inherit paths to the main checkout;
changing cwd alone does not select the worktree. Keep build outputs separate
between workers and do not edit inputs another worker is compiling.

Establish a full `homm3 build` checkpoint before searching and after integration.
It refreshes targets, scores and gates. After source, header, profile, claim or
merge changes, establish a fresh search context; do not reuse stale snapshots or
scores. Inspect a failing command's actual exit status and cause before continuing.

## Evidence and reconstruction

1. **Locate and claim when needed.** Use the DC roster and source order to form
   hypotheses, then corroborate identity with retail bodies, strings, imports,
   named callees, address-takes or vtables. NH3API addresses are not location
   evidence. Read sizes from `config/retail-functions.tsv`, never from DC.
   `DC_ONLY` means a retail counterpart is unproven, not necessarily absent.
   Follow the annotation contracts in `include/va.h`; keep unreconstructed
   bodies inside the carcass of compiled TUs. Reconcile declarations and ABI
   before activating a body. Use source annotations rather than a second ledger.
2. **Read the evidence pass in AGENTS.md.** For a DC counterpart, inspect its
   signatures, locals, scopes, source lines, file switches and helper calls
   alongside retail assembly. DC is an optimized build on another architecture:
   missing calls or line gaps do not prove missing source. Do not force SH4/x86
   counts or candidate line layout to agree. For retail-only code, state that
   limitation and use retail and sibling evidence.
3. **Explain the mismatch.** Start with `homm3 sema diff <selector> --summary`,
   then inspect `--structure`, `--calls` and `--source` as relevant. Candidate
   `/Z7` labels identify the statement producing a difference, not retail source.
   Compare unmasked `sema disasm <selector>` and `--base` when immediates or
   relocation masking obscure the difference. For large functions, narrow the
   comparison with `--base-range` and `--target-range`.
4. **Recover the source model.** Preserve proven types, layout, ABI, declaration
   ownership and one canonical definition of each helper. Match retained helper
   bodies and caller expansions separately. Returned objects and temporary
   lifetimes can expose nested helpers; do not manufacture caller braces,
   false `inline`, alternate declarations, dummy operations or pragma pins to
   force a score. Use `homm3 vc6 predict-inline <selector>` for inlining questions
   and the Holista skill for difficult reconstruction plateaus.

Unreconstructed callees and EH frames are not automatic blockers to matching a
caller. Distinguish missing evidence from a demonstrated compiler limitation.
Consult `docs/vc6/README.md` for focused compiler diagnostics; historical findings
are hypotheses to recheck in the current source and compiler context.

## Search meaningful alternatives

Use the existing JSON source-family driver for iterative alternatives. Read
`docs/vc6/source-families.md` for the schema and `--help` for command options:

```sh
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  build/choices.json --width 60 --keep 10 --jobs 6 --generations 3
```

- Set `units` explicitly, including affected callers and shared-header consumers.
- Encode independent choices as named axes/options and coupled edits with
  `extra_edits`. Keep the first option unchanged and anchors unambiguous and
  non-overlapping. Validate with `--validate-only` before compiling.
- Preserve the driver's unchanged-source and opposite-corner reproduction
  checks. Repair failed controls before trusting a population.
- Aim for 50–60 successfully scored states where the evidence supports that
  many; retain diverse reproduced parents. Do not invent variants to fill a
  quota. An exhausted manifest needs a new evidence-backed hypothesis, not a
  rerun of the same spellings. Report successful source candidates separately
  from distinct emitted objects.
- Keep manifests, temporary Python authoring helpers, snapshots and checkpoints
  under ignored `build/`. Reuse existing helpers when useful; do not commit a
  generator, fixture or experiment diary for every function or follow-up.

Manual `homm3 build --fast <TU>` and `homm3 sema diff` are useful for bootstrapping
and focused diagnostics. Adopt supported source deliberately: the search does
not adopt candidates or update the score ledger.

## Validation and completion

The normal loop is evidence → C++ → VC6 → retail comparison. Do not write or run
per-function mock behavior suites as a routine matching step. Use a small,
temporary behavioral check only to resolve a concrete uncertainty that the
available evidence leaves open; it cannot prove a retail match. Run relevant
tooling regression tests when changing tooling. Preserve build gates and search
reproduction checks; those protect the measurement itself.

Use the generated ledger's `CUR <= MAX <= HIST` semantics. MAX describes the
current implementation's best score; HIST is a recovery lead for older source.
Do not edit or inflate scores manually. Preserve recoverable candidate evidence
under ignored build output or Git history. A source-supported combined model
may temporarily lower scores: investigate its concrete predictions rather than
rejecting it solely on a percentage. Revise models that contradict proven
behavior, ABI, layout or source facts.

Finish an adopted change with full `homm3 build` and the relevant evidence review.
Report the target's result and remaining differences. Mention collateral only
when MAX falls or a concrete correctness/build failure requires action; omit
unchanged exact counts and unrelated CUR dips while MAX holds. Keep concise
residuals beside the owning function and reusable findings in `docs/vc6/`.

A TU is closed only when the fresh retail span is accounted for and all its
function targets match, flanking gaps are attributed, and each DC roster entry
is located or evidenced as inlined, retail-dropped or port-only. A claims-only
scoreboard or an exhausted search batch does not establish closure.
