---
name: helper-placement
description: Decide whether a recovered HoMM3 helper body belongs in an ordinary header or source file, and whether it was explicitly inline, using Mac body order and Windows cross-TU expansion; use Mac xrefs to identify callers. Use when restoring or moving shared helper definitions.
---

# Place a recovered helper

Read `AGENTS.md`. This skill answers **where one canonical helper body was visible to its callers**. During the broad Mac helper sweep, use the available evidence to place and call helpers without running the normal byte-matching pass for every caller. Use the `match` skill when byte matching resumes. Keep the two placement routes separate, then weigh them together.

## Route 1: Mac body order

Identify the retained Mac body and its bounds with `homm3 mac show` / `disasm`. Inspect its immediate predecessors and successors in the PEF, and compare that order with known functions from a source TU. A helper placed among that TU's functions in plausible source order supports a source-file body. Linker placement is a lead, not a source-owner record; record the boundaries and competing order explanations.

## Route 2: VC6 cross-TU expansion, with Mac xrefs for identity

Run `homm3 mac xrefs mac:0:0xOFFSET` to identify the helper's callers and pair them with known Windows functions. In the observed CodeWarrior build, the helper can remain a call whether its body is in a header or a source file. Mac xrefs therefore identify the helper and its uses; they supply **no placement evidence by themselves**. Find the Windows retained body, if any, and inspect caller xrefs with `homm3 sema diff <VA> --calls` and `homm3 vc6 predict-inline <VA>`. Establish each caller's TU from its source and retail claim. If VC6 expands the identified helper body into a caller from another TU, the definition had to be visible there; this strongly supports a header body. Expansion only inside the helper's own TU is compatible with a source-file body. A retained call across TUs proves only that a declaration was visible.

## Decide and verify

Use positive Dreamcast declarations, source-file attribution, and source-call order when available. Unknown original names do not block recovery; choose a clear project name. Distinguish an explicitly `inline` helper from an ordinary helper that a compiler auto-inlined. A retained Mac call does not rule out an `inline` declaration; an absent Mac call does not prove one. The absence of a retained Windows body also does not prove `inline`: VC6 can emit an ordinary helper as a COMDAT that the linker discards after expanding its callers. Preserve a Dreamcast-proven `inline` declaration. Do not add `inline` solely to remove a Mac call or improve a score.

Inspect the Mac callee and callers before treating a retained call as a game helper; library, runtime, glue, and generated functions need separate handling. When the Mac call target's body identifies a game operation, find equivalent direct access or pasted logic in corresponding Windows source callers and replace it with a call to the canonical helper. Add that helper when absent. A recognizable helper expansion in a Mac caller is also positive evidence. Apply this to already exact Windows functions as well. Do not require a Mac symbol name, Dreamcast confirmation, an exact Mac byte comparison, or an immediate Windows score gain. If the original name is unknown, choose a clear project name and keep the helper; rename it later if evidence improves. If an existing nested source helper already makes the call, record it as represented rather than adding a duplicate. Classify a lead as unresolved only when the Mac target's operation or its corresponding Windows operation cannot yet be identified, and record that specific gap. An uncertain name or placement alone must not turn a supported helper into an unresolved lead.

Keep a member declaration in its ordinary game header; give a free or file-static helper only the declaration its callers need. Put the **single body** in a header when cross-TU visibility or direct source evidence supports it; otherwise use the best-supported owning source file and source order. Uncertain placement is a lead for later review, not a reason to defer a clear helper. During the broad sweep, restore supported bodies and calls in batches. Compile the same body for Mac and Windows and inspect call streams when the batch reaches validation or a concrete ambiguity needs resolving. Keep source-supported structure through an incidental score dip while testing its callers.

Mac's stripped PEF has no source-owner records, and indirect calls or unresolved relocations leave xrefs incomplete. Record those gaps instead of inferring that a helper is absent. Do not duplicate bodies, add dummy calls, or use compiler-specific inline pragmas to force a decision.
