# HoMM3 matching guide

Recover C++ that reproduces Heroes III Complete's retail MSVC 6.0 object code.

## Evidence

- Retail bytes are authoritative: **English GOG Complete 4.0 (engine 3.2)**,
  `HEROES3.EXE`, fixed base `0x00400000`. The exact size and SHA-256 are in
  [README.md](README.md#pinned-target).
- Dreamcast's embedded debug symbols prove source facts for an older,
  cross-architecture build; x86 identities require retail proof.

The verdict is VC6 SP3 under Wine; clang/clangd is editor tooling only. Use the
per-TU compiler profiles in `config/units.toml`.

Use `homm3 build --fast <TU>` (for example, `homm3 build --fast cursor`) for the
inner loop. Normally supply the active TU so shared-header edits rebuild only
that TU during iteration. Run `homm3 build` for the final checkpoint: it rebuilds
affected TUs, refreshes retail targets through delinking, and runs the gates.

## Required DC evidence: source layout as well as statements

**Inspect the function's source-line layout before speculative rewrites.**
Recorded line positions, observed span lengths, internal gaps and their lengths,
repeated attributions, and source-file switches all help reveal its approximate
source shape. Read these alongside signatures, locals/lifetimes, scopes, helper
calls, and statement order. Even apparently empty space can inform an educated
guess about the original source.

`homm3 dreamcast lines <selector>` exposes that layout; `lines --module <TU>`
and `lines --all` extend it across the corpus. `show` accepts repeated selectors
and modules, and `show`/`structure` include the same line-layout evidence.

**Observations support hypotheses; they do not recover missing text.** A gap
can contain empty lines, comments, declarations, braces, or optimized/release-
elided operations. Observed spans are not total function line counts: inline
attributions can extend them, boundaries can be borrowed, and trailing lines
are unknown. Do not pad C++ with blank lines or invent assertions to satisfy a
count. Do not compare this DC shape with MSVC `/Z7` source structure or require
equal line counts. Test each meaningful hypothesis against retail VC6 output.

## Matching loop

For every non-exact game function with a Dreamcast counterpart, run this evidence
pass **before speculative C++ rewrites**:

```sh
homm3 dreamcast show 0x00524dd0
homm3 dreamcast lines 0x00524dd0
homm3 dreamcast asm 0x00524dd0 --blocks
homm3 dreamcast inline-clues 0x00524dd0
homm3 sema diff 0x00524dd0 --summary
homm3 sema diff 0x00524dd0 --structure
homm3 sema diff 0x00524dd0 --source
```

Use `homm3 dreamcast find NAME` to locate counterparts. Selectors also accept
unambiguous names, `module.obj:0xOFF`, and `dc:0xOFF`. `show` gives the dossier;
`asm --blocks` exposes source-line groups, lexical scopes, and basic blocks.

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

Dreamcast supplies positive source evidence, not a second byte target. Missing
statements or empty `inline-clues` do not prove absence in retail; an emitted
standalone helper does not disprove an inline copy. Do not compare SH4 shape with
candidate `/Z7` shape, build regex rosters/automated source-structure comparators,
or demand equal instruction, block, call, statement, local, or scope counts.
See [docs/dc-line-tables.md](docs/dc-line-tables.md) for interpretation.

Reject Dreamcast shape only when retail semantics, ABI, layout, or CFG contradict
it. A lower similarity score is insufficient. Preserve proven classes, interfaces,
helpers, and scopes through temporary score dips, including header/TU collateral;
measure that collateral and keep prior peaks in max/history. Score dips are
observational, not build failures. `homm3 status check` attributes a regression
only when a function's own source hash changed and its new MAX fell below the
preceding MAX. The invariant is CUR <= MAX <= HIST: MAX is monotone for an
unchanged function hash, a proven edit resets MAX to CUR, and HIST retains the
all-time peak. Tooling prioritizes MAX; HIST is a lead for recovering lost peaks.

## Helper boundaries and inlining

Preserve one canonical helper, its proven declaration, and source calls. Match
its retained retail body and each caller's call/expansion decision separately:

- A Dreamcast-proven `inline` stays `inline` even where retail calls it out of line.
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

A Dreamcast line gap alone does not prove an ASSERT/VERIFY/TRACE. Retain
`HOMM3_RELEASE_VERIFY(expression)` only for a meaningful recovered invariant
supported by line-table and codegen evidence. No self-assignments, unreachable
branches, dummy calls, or repeated expressions solely to change the inline budget.
Every retained VERIFY or temporary inline-depth experiment needs a source comment
naming caller, callee, and retail/Dreamcast evidence, plus a negative control
showing that flattening or de-inlining fails.

Record function-specific failed probes beside the function and reusable compiler
findings under [docs/vc6/](docs/vc6/README.md), without a separate chronological log.

## Matching data ownership

Preserve original semantic names where evidence exists, preferring Dreamcast
source names and using NH3API as a fallback. Drop Hungarian type prefixes and
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
symbol ledger. `config/` contains hand-admitted retail inventories and manifests;
`evidence/` contains generated analysis, which must be regenerated, not hand-edited.
Keep `vendor/` pristine; zlib's address-to-symbol mapping belongs in
`config/retail-zlib-map.tsv`.
