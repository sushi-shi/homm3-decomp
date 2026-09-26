# HoMM3 matching guide

Recover C++ that reproduces Heroes III Complete's retail MSVC 6.0 object code.

## Evidence

- Retail bytes are authoritative: **English GOG Complete 4.0 (engine 3.2)**,
  `HEROES3.EXE`, fixed base `0x00400000`. The exact size and SHA-256 are in
  [README.md](README.md#pinned-target).
- Dreamcast's embedded debug symbols prove source facts for an older,
  cross-architecture build; x86 identities require retail proof.
- The pinned Classic Mac PowerPC PEF is a second exact byte target for
  hand-admitted shared functions. Mac addresses are PEF section-relative
  offsets paired with existing Windows VA claims; unpaired functions have no
  Mac verdict. See [the Mac target plan](docs/matching/mac-second-target-plan.md).

Windows is the game being rebuilt. Use Mac solely as evidence for recovering
the Windows source, particularly helper boundaries, source calls and function
structure hidden by VC6 optimization. Restore evidenced helpers in ordinary
Windows game headers/source and keep their call sites; a retained Mac call does
not require VC6 to retain that call. Exact bytes on both architectures strengthen
the reconstruction without proving a unique C++ spelling. A runnable Mac port
is not a project requirement. Use native library headers for Mac comparisons;
do not add duplicate game declarations or extracted header-body fragments.

For the broad helper sweep, cover byte-exact Windows functions too. Use Mac's
retained calls and simpler body shapes to restore helper calls and canonical
bodies throughout the source. Inspect the Mac callee and its callers to
distinguish game helpers from library, runtime, glue, or generated code. A
retained Mac call to an identifiable game helper, or a recognizable helper
body expanded in a Mac caller, is enough to restore that operation as a helper
call in corresponding Windows source callers. If the helper already exists,
replace equivalent direct field access or pasted logic with its call; if it
does not, add one canonical body and its calls. Mac's stripped executable need
not supply the helper's name: infer its operation from its body and callers,
then choose a clear project name. Do not remove or defer a supported helper
because its original spelling, exact Mac byte match, placement, or immediate
Windows score gain is unknown. Keep the best-supported ordinary header or
source placement and revise it when stronger evidence appears. A Windows
function that is already byte-exact still needs its supported helper calls.
Track each lead through implemented, already represented by a nested helper,
or a specific reason why the Mac target or corresponding Windows operation
cannot yet be identified. An uncertain name or placement is not such a reason.

The verdict is VC6 SP3 under Wine; clang/clangd is editor tooling only. Use the
per-TU compiler profiles in `config/units.toml`.

Use `homm3 build --fast <TU>` (for example, `homm3 build --fast cursor`) for the
inner loop. Normally supply the active TU so shared-header edits rebuild only
that TU during iteration. It reports the selected TU's per-function projected
MAX movements without banking them; unchanged-source CUR dips stay silent.
Admitted Mac counterparts in that TU are compiled
with CodeWarrior and compared in the same loop. Run `homm3 build` for the final
checkpoint: it rebuilds affected TUs, refreshes retail targets through delinking,
checks every admitted Mac pair, and runs the gates.

## Byte-matching evidence: DC source layout as well as statements

When byte matching a non-exact function, **inspect its source-line layout before
speculative rewrites.** The broad Mac helper sweep above uses Mac calls and body
shape first; it does not require a Dreamcast dossier for each restored helper.
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

When byte matching a non-exact game function with a Dreamcast counterpart, run
this evidence pass **before speculative C++ rewrites**. It is not a prerequisite
for the broad Mac helper sweep:

```sh
homm3 dreamcast show 0x00524dd0
homm3 dreamcast lines 0x00524dd0
homm3 dreamcast asm 0x00524dd0 --blocks
homm3 dreamcast inline-clues 0x00524dd0
homm3 dreamcast audit 0x00524dd0
homm3 sema diff 0x00524dd0 --summary
homm3 sema diff 0x00524dd0 --structure
homm3 sema diff 0x00524dd0 --source
```

For byte matching an admitted Mac counterpart, also run
`homm3 mac show <Windows-VA>`, `homm3 mac disasm <Windows-VA>` and
`homm3 mac diff <Windows-VA>` before speculative rewrites. A helper-sweep edit
only needs enough Mac body/call evidence to identify the operation and its
Windows callers; batch compilation and byte comparisons can follow the sweep.
The Mac byte comparison is a separate exact verdict;
its stripped PEF does not supply Dreamcast's names or line tables. Match the
same authored C++ body against both targets, and document evidenced platform
differences at the owning source.

`homm3 mac calls <Windows-VA>` compares call sites and ordered targets.
Indirect calls through the reviewed Mac glue remain explicitly unknown;
equal aggregate counts do not prove a matching helper/inlining decision.
`homm3 mac queue --unit <TU>` names missing pairing, compilation and reference
prerequisites. Follow the [Mac tooling guide](docs/tooling/mac-matching-roadmap.md)
to create a shared unit profile, inspect emitted symbols and admit a reviewed
span. Compile admitted bodies and extra source helpers in original source order;
do not duplicate game bodies in Mac declaration headers. Each worker owns its
unit profile and pair file; coordinate changes to shared runtime/data manifests.

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
See [docs/matching/dc-line-tables.md](docs/matching/dc-line-tables.md) for interpretation.

`dreamcast audit` checks positive typed declarations and named source-call
anchors against the authored Clang AST. It reports cv/ref layers, array/base
types, member qualifiers, helper calls and unambiguous source-order leads;
this is not an SH4-versus-C++ structure comparison. Review each finding with
retail evidence, restore supported facts, then rerun the audit and VC6 build.
Keep justified platform differences in owning source comments. Coverage gaps
are not zero differences: missing names, unsupported types and Clang errors
remain explicit. See [docs/matching/source-facts.md](docs/matching/source-facts.md).

Reject Dreamcast shape only when retail semantics, ABI, layout, or CFG contradict
it. A lower similarity score is insufficient. Preserve proven classes, interfaces,
helpers, and scopes through temporary score dips, including header/TU collateral;
measure that collateral and keep prior peaks in max/history. Score dips are
observational, not build failures. `homm3 status check` reports source-edit
MAX drops as RESET when CUR held against the banked snapshot, CHANGED-CUR
when it moved, or MISSING when the body vanished. Compare a rebased lane with
main using `homm3 status check --baseline-ref REF`; the full build banks its
local ledger automatically, so the explicit ref remains useful afterward.
`homm3 status merge-baseline` merges a conflicted ledger three ways during
rebase; review its result before staging. The invariant is CUR <= MAX <= HIST:
MAX is monotone for an unchanged function hash, a proven edit resets MAX to
CUR, and HIST retains the
all-time peak. Tooling prioritizes MAX; HIST is a lead for recovering lost peaks.

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

Use the [helper-placement skill](.agents/skills/helper-placement/SKILL.md) when
Mac body order or VC6 cross-TU expansion may locate a recovered helper body.
Mac xrefs identify callers but do not decide header versus source placement.
During the broad sweep, an unidentified original name or uncertain placement
does not veto a clear helper body and call. Put it in the best-supported
ordinary header or source file, then revise placement if later cross-TU or
source-order evidence warrants it.

During byte recovery after the Mac helper sweep, keep the recovered helper
calls. Do not replace them with direct fields, array indexing, or pasted
statements for a higher score. An outer helper may replace a call when its
implementation contains that recovered helper call; preserve the complete path.

Preserve one canonical helper and its source calls. Preserve proven types and
inline qualifiers; choose a clear name where the original is unknown. Match
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
Support retained VERIFYs and temporary inline-depth diagnoses with named caller,
callee and retail/Dreamcast evidence. Compare the removal or flattening variant
under VC6 to establish its effect; this does not require a mock behavior suite.

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
and uses together. When an original spelling is known, retain it in the owning
source's evidence comment so reference lookup remains possible. Do not invent
one or delay helper recovery when no spelling survives. Preserve required
external ABI spellings at their boundaries.

Source annotations own names; build regenerates labels. Do not maintain a second
symbol ledger. `config/` contains hand-admitted retail inventories and manifests.
Keep `vendor/` pristine; zlib's address-to-symbol mapping belongs in
`config/retail/zlib-map.tsv`.
