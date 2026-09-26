---
name: mac-helper-sweep
description: Temporary HoMM3 helper-recovery sweep comparing Mac assembly calls with Windows source calls, then restoring each recovered helper across all corresponding callers. Use during the broad helper pass, including byte-exact functions.
---

# Recover helpers throughout the Windows source

Temporary skill for the current broad sweep. Windows is the reconstructed game;
Mac supplies lightly optimized source-structure evidence. Recover helpers and
all their uses regardless of matching percentages. Read the active worktree's
AGENTS.md. Retire this task skill when the sweep and its remaining queue are
finished.

## Pass 1: reconcile each caller

1. Inspect a Mac function and find its Windows counterpart using the source
   VA/MAC_ADDRESS claims and reviewed maps. If the counterpart is unpaired,
   identify it from the body, callers and surrounding functions, then record it.
2. Read the Mac assembly and enumerate its call sites and targets. Read the
   Windows function's authored C++ and enumerate its calls. Compare both the
   amount of calls and their identities, including repeated calls to one helper.
   The Windows side is **source code**, not emitted MSVC assembly.
3. Investigate every discrepancy. Find the corresponding operation in the
   Windows source: pasted helper logic, raw member access, an expanded accessor,
   a missing base-class call, or a different helper with similar behavior.
   Restore the canonical helper and replace the corresponding expansion with
   its call. Add a clear helper name and body if none exists.
4. Follow existing nested helpers and ordinary constructor/destructor lifetimes
   when they explain a call. Record the actual source path and its Mac evidence.
   An unexplained call-structure discrepancy remains open.

## Allowed structural differences

Every Mac helper operation must be represented in Windows source. A different
boundary is accepted only with evidence that Mac inlined the source helper.
This includes a source wrapper `f` whose body calls `g` and `h`: if Mac expanded
`f` and retains calls to `g` and `h`, keep canonical `f -> g, h` in source.
Likewise, a helper expanded entirely in Mac may remain a function in our source.
Trace the complete expansion, arguments and operations; an invented wrapper or
coincidentally equal total does not establish correspondence.

Do not close a mismatch merely as optimization, a common tail, an unknown
original name, a score-preserving spelling, or a platform difference. Investigate
and restore the matching source call structure, allowing only the evidenced
Mac-inline cases above. If retail semantics genuinely conflict, preserve the
concrete evidence and leave that item unresolved for an explicit decision;
do not force a semantic change or silently declare the discrepancy handled.
Reopen older dispositions that relied solely on those excluded explanations.

A call through virtual-dispatch glue is still a source operation to identify.
Resolve its receiver, slot and arguments; a runtime-glue label does not close
the item.

Counts locate work; compare the actual operations to resolve it. Equal totals
can hide a wrong target, and one textual occurrence does not account for several
Mac sites. Exclude the function's own declarator from source-call counts: a
method named getValue is not itself a call to a base getValue. Include overloads,
operators, macros and implicit lifetime operations in the source review. Mac
can inline too; inspect recognizable expansions rather than requiring a retained
branch for every helper. Do not manufacture calls to equalize totals.

## Pass 2: propagate every recovered helper

After identifying or restoring a helper, **follow all its Mac xrefs**. Pair every
Mac caller with Windows source and inspect the corresponding operation there.
Restore the helper at every supported site, including callers already at 100%.
Compare the full Mac caller set with the Windows source caller set; resolve
missing uses, extra uses and repeated sites. A helper reached through another
canonical helper is represented through that source path.

Repeat this pass for newly discovered nested helpers. Keep the caller worklist
open until every applicable xref is implemented or already represented through
an evidenced Mac-inline path. Keep conflicting or unidentified operations open
with concrete evidence. Unknown names or uncertain header/source placement do
not block recovery. Use the helper-placement skill when placement
needs investigation; keep one ordinary definition and revise its location later
if stronger evidence appears.

For callers owned by another worker, send the helper identity, Mac target/call
sites, Windows counterpart and required source operation to that worker and the
integrator. Track that handoff until it is integrated. Do not stop at the first
caller or silently exclude callers outside your assigned TU.

## Tools and queue

Generate the caller inventory with `homm3 mac helper-audit`. It writes the full
resolved graph plus `build/mac/helper-audit-calls.tsv` and
`build/mac/helper-audit-source.tsv`. Use these generated queues for both passes.

- `homm3 mac helper-audit <Windows-VA-or-Mac-offset> --json` supplies Mac callers,
  source users, callees, exact declaration identities, expressions, nested paths,
  loader/TOC references, and missing mappings. After a helper edit, regenerate
  this report and follow every caller comparison and reverse source comparison.
- `--unit <TU>` is useful for local iteration, but its output is explicitly
  partial. Run the full graph before claiming all xrefs were propagated.
- `homm3 mac show <Windows-VA>` and `homm3 mac disasm <Windows-VA>` provide the
  assembly needed to decide each generated discrepancy and verify expansions.
- `homm3 mac xrefs mac:0:0xOFFSET` gives raw binary references independently.
- The older `helper-queue` uses textual presence leads. Do not use its counts or
  old unit-wide review flags as resolved source xrefs or completion evidence.
- `homm3 mac calls` compares compiled Mac candidates to retail; it does not
  replace this Mac-assembly-versus-authored-Windows-source audit.

Do not manually enumerate caller sets or maintain a duplicate call inventory.
Record semantic findings and unresolved questions against generated site/target
identities; regenerate the inventory after source changes. A resolved direct
count or reachable nested path is a lead, not automatic closure. Parse errors,
unpaired library identities, differing header views, indirect dispatch and
implicit lifetime gaps remain open. Inspect the graph's diagnostics and gaps;
use source/assembly review for operations the AST cannot expose. See
[the graph guide](../../../docs/tooling/mac-helper-graph-plan.md).

Distinguish new helper bodies, restored uses and address mappings in reports.
Honor deferred modules. The strict Mac-inline-only boundary rule above still
applies, including when a generated report finds equal counts.

Batch source edits. Do not run per-helper builds, score searches, or a Dreamcast
dossier pass. Use a focused compiler probe only for a concrete unresolved
semantic question; batch validation follows the restoration pass. Preserve
supported helper calls through score drops. Adding ordinary helper declarations,
bodies and uses is already authorized for this sweep.
