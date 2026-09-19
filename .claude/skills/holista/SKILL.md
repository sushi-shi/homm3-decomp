---
name: holista
description: Use a two-worker approach for difficult C++ retail byte matches. Pair the primary matcher with Holista to reconstruct code as the original developers would plausibly have written it, recognizing inline helper patterns and combining source evidence, declaration and statement order, loops, and lifetimes. Use for hard cases and matching plateaus alongside the repository's matching workflow.
---

# Holista

Recover a coherent original-source model, then test it against retail. The
central question is: **What C++ would the original developers naturally have
written to implement this operation?** Treat the current reconstruction as a
hypothesis, including its class interfaces and helper boundaries.

This skill explicitly calls for two workers: the primary matcher and a second
worker named **Holista**. Use the repository's matching instructions and the
`match` skill when available for compiler setup, evidence tools, and verification.
This skill adds a way to reason about hard functions; it does not replace those
requirements or change the byte target.

## Two complementary workers

The primary matcher owns the active source, compiler experiments, retail
comparisons, score ledger, and adoption. It continues useful measurement while
Holista works independently on the source model.

Start one worker with task name `holista`, or reuse the existing Holista worker.
Give it the target identity, active worktree, relevant source/classes/callers,
raw retail and Dreamcast evidence, and the concrete remaining differences.
Distinguish observed facts from previous interpretations. Previous failed probes
help avoid duplicate work but do not establish that a model is impossible.

Holista owns the whole-function interpretation and writes plausible candidate
C++, including relevant helper bodies and interfaces. Initially it reads shared
source and writes proposals in its own scratch area. The primary can delegate a
specific edit or experiment later, with explicit file ownership or an isolated
snapshot. Do not race builds, source edits, or generated ledgers in a shared
worktree. Keep this a two-worker pattern unless the user asks for more workers.

Holista should send a useful lead promptly, then refine it. The primary should
return actual instruction, call, lifetime, and CFG changes from experiments,
including negative results. Reuse that exchange to revise the model.

## Think like the original developers

Read the operation from entry to exit and through the relevant callers and
callees. Establish the algorithm, objects, ownership, invariants, and phases
before optimizing a local mismatch. Use contemporary compiler capabilities,
project idioms, sibling implementations, and recovered declarations; avoid
projecting modern C++ style onto older code.

Write source that expresses those phases naturally. Consider why a developer
would declare a local at function entry, at a phase boundary, or inside a loop;
which declarations would be grouped; and where a value must remain alive.
Consider plausible statement order, loop form, traversal state, early exits,
constructor/destructor boundaries, and placement of helper definitions.
These choices interact with aliasing, temporary reuse, inlining, and lowering.
Do not choose them solely because an isolated spelling scores better.

Actively recognize patterns that could be **expanded functions**:

- Repeated field reads, navigation chains, predicates, and paired field/flag
  writes can indicate accessors or small class methods.
- Coordinate copies, arithmetic sequences, returned aggregates, and cleanup
  paths can indicate constructors, assignment, operators, or lifetime helpers.
- A larger inlined region can be a composition of several small helpers,
  including helpers nested inside other expansions.
- Similar patterns in separate parts of a function or in sibling functions may
  have one shared source abstraction. Test that explanation across its uses.

Describe each suspected helper's semantic purpose, receiver, arguments, return
type, ownership, and place in the source call sequence. Repetition alone does
not prove a helper: compiler lowering and explicitly repeated statements are
alternatives. Distinguish a source-declared `inline` from an ordinary helper
auto-inlined by the compiler. Keep one canonical definition and real source
calls; do not paste helper bodies, add false `inline`, or invent alternate
declarations merely to influence code generation.

## Use Dreamcast as source evidence

Where a counterpart exists, start from its signatures, locals and lifetimes,
scopes, source calls, line layout, and source-file switches. Dreamcast often
exposes boundaries that retail x86 expanded, but Dreamcast itself is optimized
and also inlines. Neither its emitted calls nor their absence provide a complete
source call graph. A retained standalone helper may also have inline copies.

For HoMM3, inspect `dreamcast show`, `lines`, `asm --blocks`, `inline-clues`,
and `audit`, alongside retail `sema diff --summary`, `--structure`, and
`--source`. Use `dreamcast find` and source-order context when locating a
counterpart. Read line positions, spans, gaps, repeated attributions, and file
switches as clues to source composition, not missing text to manufacture.

Do not assume a one-to-one mapping between DC instructions, scopes, line groups,
and original statements. Do not compare SH4 and x86 instruction/block counts or
force candidate line counts to match DC. Candidate `/Z7` labels describe the
candidate, not recovered retail source. Missing counterparts and coverage gaps
remain explicit; when no DC bridge exists, use retail and sibling evidence
without attributing speculative declarations to Dreamcast.

## Test whole models, including temporary regressions

Holista should present a small set of coherent candidate implementations. For
each, explain the observations it accounts for, inferred source structure,
uncertainties or contradictions, and an experiment that distinguishes it from
the current model. Include actual C++ where possible, not just a list of
optimizer knobs. State a concrete prediction across the affected residual
regions or sibling uses, so that a shared abstraction explains more than one
convenient instruction change.

Combine changes when they belong to one source explanation. A recovered helper
interface may require changing several inline expansions, declaration order,
local lifetimes, and a loop together. Evaluate the complete combination as well
as useful constituent controls; an edit failing alone does not refute the
combination. Revisit earlier rejected probes when their surrounding model has
changed.

**Do not be afraid of a temporary score or CFG match drop.** A partial model
can change inlining, scope cleanup, or loop lowering before the remaining
pieces restore retail's structure. Preserve the prior candidate and peaks, inspect
what changed, and follow the supported hypothesis through the relevant combined
experiments. Do not require every intermediate candidate to preserve the current
CFG, call count, frame size, or similarity score.

That freedom is not permission to ignore contradictory evidence. Separate a
temporary compiler-shape mismatch from a conflict with proven behavior, ABI,
layout, or source facts. If a model violates those facts, revise it. A proposed
recovery needs a concrete remaining experiment, not an indefinite promise that
unrelated edits will fix it. Never manufacture dummy calls, self-assignments,
unreachable code, arbitrary declarations, or unsupported release-elided checks
to steer the optimizer.

The primary turns supported models into reviewable source-family experiments
using the repository's workflow. Keep unchanged controls, reproduce promising
candidates, inspect named call/relocation sequences, and measure affected
siblings and shared-header consumers. Use behavioral checks when source
ownership, arithmetic, or traversal changes warrant them.

Finalize only after the complete implementation satisfies the actual retail
target and required build gates. Preserve proven source facts even through score
dips. Document specific failed hypotheses and remaining differences in the
owning source, and reusable compiler findings in the repository's normal place.
Source plausibility guides the search; verified retail bytes decide exactness.
An exact candidate still does not prove that its precise source text or helper
spelling was the original. Keep that distinction explicit.
