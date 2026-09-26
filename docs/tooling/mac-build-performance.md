# Mac build reuse

Mac call inspection and byte comparison share a `BuildSession` for each command.

- Load and validate pair/reference identities once; use the same symbol map
  and labels for inspection, linking and comparison.
- Capture ordinary headers once for each distinct include-directory set.
- Generate each candidate compilation group once. Validate its disk cache,
  compile when necessary, and parse its CodeWarrior listing once.
- Select each paired function from those parsed hunks. Call inspection and
  byte comparison reuse the same selected result.
- Cache failed compilation attempts within the command. Keep source/profile
  provenance when available so the queue reports a current compilation failure.
- Hash inputs before and after the batch. Source, header, SDK, configuration or
  tool changes prevent publication. Recheck object/listing/staged-header bytes
  too. No session cache survives the command; the persistent object cache keeps
  its existing content and profile validation.

A successful full checkpoint reuses the checked session for its per-row
freshness gate and verifies the input snapshot again before banking scores.
Standalone compile and comparison commands retain their freshness checks.

Retail xrefs use `discovery.Index`: one instruction pass gathers direct and
indirect branch sites and TOC uses. Reverse indexes serve subsequent caller,
loader-pointer and TOC queries. The helper audit consumes that index, including
indirect sites; it does not compile candidates or rescan instructions per helper.

## Measurement and checks

On the helper-sweep worktree, the original serial run produced 31 observations
in about 18 minutes before interruption. The first session-based run processed
all 73 admitted pairs in 145.412 seconds, reusing one reference context and
12 compiled units. All 31 overlapping score/error/call observations were
identical. The final warm run took **54.558 seconds** for all 73 pairs; all 33 byte results
and 40 unavailable observations were unchanged from the first complete run.
Different scopes and cache warmth mean this is not a controlled speedup ratio.

The pinned PEF xref index took **1.258 seconds** to construct. Ten thousand
lookups of `mac:0:0xc7f1c` then took **0.027 seconds** total without rescanning
the binary. Timings are local wall-clock observations, not performance gates.

The complete run reported 33 byte comparisons (six exact) and 40 unavailable
comparisons. Those missing prerequisites remain failures and prevent a complete
Mac checkpoint; reuse does not waive or normalize them away.

**39 focused tests passed**, covering one preparation/listing parse per group, shared references,
failed-attempt provenance, input additions/removals and same-size edits,
artifact tampering, publication refusal, and reverse-xref queries without
rescanning the instruction collections. An existing reference-signature test
still fails on a `public:` prefix; the same failure was reproduced with the
pre-change reference loader.
