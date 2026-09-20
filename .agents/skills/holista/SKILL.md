---
name: holista
description: Pair a primary matcher with Holista to reconstruct plausible original C++ from debug evidence, retail assembly, temporary lifetimes and shared helpers. Use for difficult matches or explicitly authorized sustained module-wide or whole-tree reconstruction.
---

# Holista

Recover a coherent source model: what C++ would the original developers
plausibly have written? Treat the current reconstruction, including its helper
boundaries and interfaces, as a hypothesis. Follow the active repository's
`AGENTS.md` and [matching workflow](../match/SKILL.md) for evidence and measurement.

## Two-worker collaboration

The primary owns integration, builds, retail comparisons, the ledger and
adoption. Start or reuse one worker named `holista` to interpret the source and
write candidate C++. Give it the target identity, worktree, relevant classes and
callers, raw retail/debug evidence and remaining differences. Separate facts
from earlier interpretations; failed probes need not rule out a combined model.
When the user asks for a clean review, supply raw artifacts and scope without
seeding the worker with the previous review's conclusions.

Initially Holista writes proposals in scratch space. Delegate edits only with
explicit file ownership or a separate preparation worktree; do not race source,
headers, builds or generated ledgers. Keep this a two-worker pattern unless the
user asks for more. Holista sends a useful lead early; the primary returns
measured instruction, call, lifetime and CFG differences so both can refine it.

For an explicitly authorized module-wide or whole-tree campaign, Holista writes
source batches rather than stopping at advice. Track owned files/functions, base
commit, evidence, prepared changes and next steps in an ignored work area. Label
unbuilt work as prepared, not measured. Hand batches to the primary for integration
and continue independent work. Preserve coverage of unvisited and evidence-limited
units; a finished batch is not completion of the authorized campaign. Checkpoint
at context boundaries and coordinate shared-header ownership.

## Reconstruct whole operations

Read entry-to-exit behavior and relevant callers/callees before chasing a local
instruction difference. Establish objects, ownership, phases and invariants
using the project's contemporary idioms, recovered declarations and sibling
implementations. Consider natural construction, statement order, traversal,
early exits, cleanup and definition visibility together.

Repeated field operations, predicates, navigation, aggregate copies and cleanup
can reveal expanded accessors, constructors, operators or nested helpers.
For each proposed helper, explain its purpose, receiver, parameters, return type,
ownership and call sites. Repetition alone is not proof. Keep one canonical body
and real calls; distinguish proven source `inline`, in-class definitions and
ordinary auto-inlining.

Short lifetimes and reused stack slots may arise from an inlined helper's return
temporary. Do not translate them directly into artificial caller blocks, even
if those blocks currently score 100%. Inspect value-return wrappers, output-
reference interfaces, constructors and operators across their consumers. Preserve
aliasing and returned-reference behavior; a plausible wrapper does not authorize
changing a proven virtual ABI. Explicit scopes need a source purpose such as
RAII cleanup or a loop body.

Use debug signatures, locals, scopes, line layout, declaration order and source-
file switches with their assembly. Optimized debug information is incomplete:
blank gaps do not prove assertions, and missing emitted calls do not prove absent
source helpers. For HoMM3 use the DC evidence pass in AGENTS.md alongside retail
`sema diff --summary`, `--structure` and `--source`. Candidate `/Z7` labels describe
the candidate; do not demand matching SH4/x86 counts or manufacture source layout.
State where counterparts or evidence are missing.

## Propose, measure, refine

Return a small set of coherent C++ candidates, the evidence each explains,
uncertainties and a concrete prediction that distinguishes each from the current
model. Combine interface, helper and caller changes when they express one
explanation. Revisit earlier probes when their surrounding model changes.
Temporary score or CFG drops are acceptable investigation results; contradictions
of proven behavior, ABI, layout or source facts require revision. Avoid dummy
operations, unsupported release checks and optimizer-only declarations or scopes.

The primary measures candidates with the existing JSON search and VC6/retail
comparison, retaining reproduction checks and inspecting affected consumers.
Judge progress and candidate adoption by MAX, using projected MAX for searches.
Compiler-context changes can lower CUR while MAX holds; these are not lost
matches or grounds for rejection. Keep CUR for codegen diagnosis and reproduction,
and omit held-MAX CUR dips from progress reports. Report collateral only when
MAX falls or a concrete correctness/build failure requires action.
Do not generate a per-function mock test suite as part of the handoff. A temporary
behavioral check is useful only for a specific unresolved semantic question.
Keep candidate files and diagnostics under ignored `build/`; commit supported
source and concise findings. Run relevant tooling tests when changing tools.

Finalize through the required full build and evidence gates. Keep unresolved
work explicit rather than promising that unrelated edits will repair it.
Plausibility guides reconstruction; retail bytes decide exactness, and exactness
does not prove the original source spelling.
