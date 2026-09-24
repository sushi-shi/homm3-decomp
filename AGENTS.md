# HoMM3 matching guide

Recover C++ that reproduces Heroes III Complete's Windows map and campaign
editors, reusing the game's recovered source. Keep the inherited game checkpoint
separate from editor matching results.

## Evidence

- Retail bytes are authoritative: **English GOG Complete 4.0 (engine 3.2)**,
  `HEROES3.EXE`, fixed base `0x00400000`. The exact size and SHA-256 are in
  [README.md](README.md#pinned-target).
- Editor identities and compiler settings must be established against their own
  pinned executables. Shared source does not imply identical bytes or addresses.
- Historical source facts, including Dreamcast evidence, remain in owning source
  comments. This worktree has no Dreamcast or Mac inputs or analysis commands.
  Those facts guide reconstruction; each Windows identity requires retail proof.

The verdict is VC6 SP3 under Wine; clang/clangd is editor tooling only. Use the
per-TU compiler profiles in `config/units.toml`.

Use `homm3 build --fast <TU>` (for example, `homm3 build --fast cursor`) for the
inner loop. Normally supply the active TU so shared-header edits rebuild only
that TU during iteration. Run `homm3 build` for the final checkpoint: it rebuilds
affected TUs, refreshes retail targets through delinking, and runs the gates.

## Matching loop

Before speculative C++ rewrites, read the existing source evidence and inspect
the selected Windows function, its callers and its helpers:

```sh
homm3 sema disasm 0x00524dd0
homm3 sema diff 0x00524dd0 --summary
homm3 sema diff 0x00524dd0 --structure
homm3 sema diff 0x00524dd0 --source
```

These commands currently use the inherited game target. Do not interpret a game
VA or score as an editor claim. Keep editor inventories, outputs and compiler
profiles separate; verify shared-source identities from each executable's code,
strings, imports, vtables and callers. The initial editor compiler hypothesis is
`/O1`, supported by shared inflate-buffer code; VC6 output must test it.

Form hypotheses from recovered signatures, locals/lifetimes, scopes, statement
order, helper/accessor calls, and constructor/destructor/RAII boundaries. Then
check the candidate against retail with VC6. `--structure` compares candidate
and retail CFGs. `--source` labels **candidate** statements using a separate `/Z7`
object whose function bytes are verified against the matching object; start at
the first `!!` statement. Those labels do not recover retail source semantics.

For large dispatchers, inspect the current arm. Ranges are function-local and
end-exclusive; use separate offsets when candidate and retail have shifted:

```sh
homm3 sema diff 0x0059fe30 --base-range +0xb90:+0xc60 \
  --target-range +0xc00:+0xcdc --structure
```

Historical debug observations support hypotheses; they do not recover missing
text. Missing statements or calls do not prove absence in retail, and an emitted
standalone helper does not disprove an inline copy. Do not invent assertions,
pad source, or demand equal cross-architecture instruction, block or line counts.
Keep justified platform differences and evidence limits beside the source.

Reject recorded source facts only when retail semantics, ABI, layout, or CFG
contradict them. A lower similarity score is insufficient. Preserve proven
classes, interfaces, helpers and scopes through temporary score dips, including
header/TU collateral; measure that collateral and keep prior peaks in max/history.
Score dips are observational, not build failures. `homm3 status check` attributes
a regression only when a function's own source hash changed and its new MAX fell
below the preceding MAX. The invariant is CUR <= MAX <= HIST: MAX is monotone
for an unchanged function hash, a proven edit resets MAX to CUR, and HIST retains
the all-time peak. Tooling prioritizes MAX; HIST is a lead for recovering lost peaks.

## Experiment lifetime

The routine matching loop is evidence, C++, VC6 and retail comparison. Do not
create or run a per-function mock behavior suite as a matching requirement.
Use a temporary behavioral check only for a concrete unresolved semantic
question; agreement with a mock is not proof of a retail match.

Keep regression tests for tooling contracts such as cache freshness, symbol and
relocation pairing, score accounting, source ownership and search rendering.
Run relevant tests when changing that tooling. Build gates and the search
driver's baseline/reproduction controls remain part of normal matching.

Keep one-off search manifests, generators, snapshots and diagnostic fixtures in
ignored `build/`, alongside the JSON source-family results. Prefer JSON axes and
options; use temporary Python when it helps author them. Commit recovered C++,
concise evidence and reusable tooling, not a new script for every search batch.

Per-function behavioral fixtures are disposable even while a match is unfinished.
Retire existing search generators when all their targets reach MAX 100% for the
current implementation. Git history is the archive. Keep a shared experiment
only while an unfinished target needs it: an exact helper can still support a
search for an unfinished caller. HIST 100% alone does not establish completion.
Remove obsolete imports and command references with retired experiments.

## Holista for hard cases

For difficult matching plateaus, use the repository's
[Holista skill](.agents/skills/holista/SKILL.md). Pair the primary matcher, who
owns experiments and adoption, with a second worker named Holista, who proposes
coherent C++ as the original developers might have written it. Look for expanded
helper patterns and natural declaration, statement, loop, and lifetime structure.
Short temporary lifetimes and stack reuse are clues to inlined helpers and
returned objects. Challenge artificial caller blocks even in exact functions;
recover the canonical helper and its natural call sites, including nested
expansions. Braces added solely to reproduce stack reuse are diagnostics, not
recovered source.
Test related changes together: a temporary score or CFG drop can recover when
the complete source model is implemented. Keep those intermediate differences
distinct from contradictions of proven behavior, ABI, layout, or source facts;
the final implementation still requires retail verification.

## Helper boundaries and inlining

Preserve one canonical helper, its proven declaration, and source calls. Match
its retained retail body and each caller's call/expansion decision separately:

- An evidence-proven `inline` stays `inline` even where retail calls it out of line.
- An ordinary helper auto-inlined by retail stays ordinary: expose its real body
  in the original TU and source order. Do not paste its body into the caller or
  add a false `inline` keyword.

Diagnose with `homm3 vc6 predict-inline <selector>` and inspect
the named call sequence, not aggregate call counts. Recover natural compiler state
through declarations, body visibility, source order, local lifetimes, meaningful
release-elided operations, and TU/PCH state. Do not manufacture alternate
declarations or dummy caller code to steer the inliner.

`#pragma inline_depth(0)` and `#pragma auto_inline(off)` are temporary diagnostics
only; inspect affected call sites and remove them before commit. Do not add
`INLINE_GATE`; existing inline-depth pins may only be removed.

A historical debug line gap alone does not prove an ASSERT/VERIFY/TRACE. Retain
`HOMM3_RELEASE_VERIFY(expression)` only for a meaningful recovered invariant
supported by recorded source and codegen evidence. No self-assignments, unreachable
branches, dummy calls, or repeated expressions solely to change the inline budget.
Support retained VERIFYs and temporary inline-depth diagnoses with named caller,
callee and recorded source/retail evidence. Compare the removal or flattening variant
under VC6 to establish its effect; this does not require a mock behavior suite.

Record function-specific failed probes beside the function and reusable compiler
findings under [docs/vc6/](docs/vc6/README.md), without a separate chronological log.

## Matching data ownership

Preserve original semantic names where evidence exists, preferring source names
recorded in owning evidence comments and using NH3API as a fallback. Drop Hungarian type prefixes and
normalize the semantic part of project-owned identifiers to lowerCamelCase.
Use scope prefixes consistently: `m_` for instance data members, `s_` for static
data members, and `g_` for globals (including file-static globals). Locals,
parameters, and ordinary functions use lowerCamelCase without these prefixes.
For instance fields, `bShowTroopCount` becomes `m_showTroopCount`,
`disabled_frame` becomes `m_disabledFrame`, and `Text` becomes `m_text`.
Apply the convention throughout the code, updating declarations, definitions,
and uses together. Retain the
original spelling in the owning source's evidence comment so reference lookup
remains possible. Preserve required external ABI spellings at their boundaries.

Source annotations own names; build regenerates labels. Do not maintain a second
symbol ledger. `config/` contains hand-admitted retail inventories and manifests.
Keep `vendor/` pristine; zlib's address-to-symbol mapping belongs in
`config/retail/zlib-map.tsv`.
