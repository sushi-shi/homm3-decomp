---
name: helper-placement
description: Decide whether a recovered HoMM3 helper body belongs in an ordinary header or source file, and whether it was explicitly inline, using Mac function order and xrefs plus Windows cross-TU expansion evidence. Use when restoring or moving shared helper definitions.
---

# Place a recovered helper

Read `AGENTS.md` and use the `match` skill for the normal Dreamcast, VC6, and Mac evidence pass. This skill answers the narrower question of **where one canonical helper body was visible to its callers**. Keep the two routes separate, then weigh them together.

## Route 1: Mac body order

Identify the retained Mac body and its bounds with `homm3 mac show` / `disasm`. Inspect its immediate predecessors and successors in the PEF, and compare that order with known functions from a source TU. A helper placed among that TU's functions in plausible source order supports a source-file body. Linker placement is a lead, not a source-owner record; record the boundaries and competing order explanations.

## Route 2: VC6 cross-TU expansion, with Mac xrefs for identity

Run `homm3 mac xrefs mac:0:0xOFFSET` to identify the helper's callers and pair them with known Windows functions. In the observed CodeWarrior build, the helper can remain a call whether its body is in a header or a source file. Mac xrefs therefore identify the helper and its uses; they supply **no placement evidence by themselves**. Find the Windows retained body, if any, and inspect caller xrefs with `homm3 sema diff <VA> --calls` and `homm3 vc6 predict-inline <VA>`. Establish each caller's TU from its source and retail claim. If VC6 expands the identified helper body into a caller from another TU, the definition had to be visible there; this strongly supports a header body. Expansion only inside the helper's own TU is compatible with a source-file body. A retained call across TUs proves only that a declaration was visible.

## Decide and verify

Check positive Dreamcast declarations, source-file attribution, and source-call order. Distinguish an explicitly `inline` helper from an ordinary helper that a compiler auto-inlined. A retained Mac call does not rule out an `inline` declaration; an absent Mac call does not prove one. The absence of a retained Windows body also does not prove `inline`: VC6 can emit an ordinary helper as a COMDAT that the linker discards after expanding its callers. Preserve a Dreamcast-proven `inline` declaration. Do not add `inline` solely to remove a Mac call or improve a score.

Keep a member declaration in its ordinary game header; give a free or file-static helper only the declaration its callers need. Put the **single body** in a header only when cross-TU visibility or direct source evidence supports it; otherwise use the owning source file and original source order. Compile the same body for Mac and Windows, then inspect both ordered call streams and the VC6 retail result. Keep source-supported structure through an incidental score dip while testing its callers.

Mac's stripped PEF has no source-owner records, and indirect calls or unresolved relocations leave xrefs incomplete. Record those gaps instead of inferring that a helper is absent. Do not duplicate bodies, add dummy calls, or use compiler-specific inline pragmas to force a decision.
