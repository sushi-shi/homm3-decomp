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
   when they explain a call. Record that actual source path. For real platform
   differences, record the Mac operation and corresponding Windows operation;
   keep an unidentified counterpart explicitly unresolved.

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
open until all applicable xrefs are implemented, already represented, or have a
specific evidenced difference. Unknown names or uncertain header/source
placement do not block recovery. Use the helper-placement skill when placement
needs investigation; keep one ordinary definition and revise its location later
if stronger evidence appears.

For callers owned by another worker, send the helper identity, Mac target/call
sites, Windows counterpart and required source operation to that worker and the
integrator. Track that handoff until it is integrated. Do not stop at the first
caller or silently exclude callers outside your assigned TU.

## Tools and queue

- `homm3 mac show <Windows-VA>` and `homm3 mac disasm <Windows-VA>` identify
  and inspect the Mac caller.
- `homm3 mac xrefs mac:0:0xOFFSET` finds callers of a recovered helper.
- `homm3 mac helper-queue --all-functions --include-named` supplies leads;
  its textual presence flags and old unit-wide review flags do not prove closure.
- `rg` locates Windows source callers and equivalent expansions. Inspect the
  actual source body and nested definitions for each count mismatch.
- `homm3 mac calls` compares Mac retail with compiled Mac candidates; it does
  not replace the Mac-assembly-versus-Windows-source comparison in this skill.

Record each caller/call-site disposition with concrete evidence and maintain
unresolved identities and cross-worker follow-ups in the queue. Distinguish new
helper bodies, restored helper uses, and address mappings in progress reports.
Honor explicitly deferred modules and report them separately.

Batch source edits. Do not run per-helper builds, score searches, or a Dreamcast
dossier pass. Use a focused compiler probe only for a concrete unresolved
semantic question; batch validation follows the restoration pass. Preserve
supported helper calls through score drops. Adding ordinary helper declarations,
bodies and uses is already authorized for this sweep.
