# Consumer address contracts (work in progress)

This is the in-progress consumer stage above [#85](https://github.com/sushi-shi/homm3-decomp/pull/85).
It is not yet integrated into the ordinary build or ready for its stacked PR.
It compares observed machine byte-address expressions, not complete function
behavior, semantic units, or memory safety.

```sh
HOMM3_DIR="$PWD" PYTHONPATH=scripts python -m homm3.analysis.data_accesses \
    --output build/data-accesses
```

## Current evidence flow

`sema/data_match.py::prepare` supplies fresh raw objects, source-bound allocations,
independent code-entry anchors and the pinned retail image. The existing
`analysis/data_functions.py` resolver is shared by startup and consumer analyses;
source function anchors are scoped to their translation unit. A same-spelled
local function in another unit cannot supply identity.

`analysis/access_expressions.py` represents bounded polynomial expressions in
32-bit machine byte addresses. Inputs are incoming stack arguments/registers,
independently bound storage, and initial memory loads. Expressions canonicalize
LEA/add/shift/multiply spelling and register allocation without dropping factors.
Complex expressions beyond the explicit degree/term budget become unknown.

`analysis/data_accesses.py` follows reachable raw x86 instructions, retains only
agreed facts at joins, and records reads/writes, widths, storage identities and
address coefficients. A memory write or unknown call prevents subsequent loads
from being mistaken for initial values; partial argument overwrites survive joins
as unknown storage. Relocation placeholders cannot become literal addresses.
Unknown instruction effects, control flow, repeat counts and referents remain
explicit. An observed expression mismatch is a diagnostic, not proof of different
behavior: ordering, path feasibility and calling contracts remain separate.

`analysis/access_loops.py` currently admits only one linear counted loop body,
a single induction update, adjacent comparison and latch, and a unique linear
prefix. It proves the full counter domain, including signed/unsigned comparison,
step divisibility and arithmetic wrap limits. Other loops remain unbounded.
An enclosing back edge invalidates a first-iteration domain. Loop-local identities support ranges; they do not establish cross-binary loop
correspondence. Independent counter execution tests check the closed-form bounds.

The standalone pass exports literal TSV with JSON structured fields:

- `data-accesses.tsv`: every observed access, width, expression, extent evidence
  and supported loop domain.
- `data-access-matches.tsv`: independently paired functions and the missing/extra
  expression multisets. Agreement never means complete function equivalence.
- `data-contract-issues.tsv`: unresolved entries, instructions, paths and effects.
- `data-access-summary.json`: fixed function denominators, verdict counts, input
  hashes, pinned retail hash and explicit policy.

Input content is checked again after analysis. A concurrent source/object change
cannot receive a report bearing the newer hash for an older computation.

## First audited corpus checkpoint

The pass inspected 11,952 admitted retail functions and 9,193 emitted candidate
bodies, with 3,718 independently anchored candidate/retail pairs. It recorded
278,344 access sites: 121,995 known expressions and 156,349 unproved addresses.
143 pairs have agreeing observed expressions with complete local observation;
61 have no observed data access, 949 have differing expressions and 2,565 remain
unproved. These counts do not add any static data-match credit.

The extent report contains 24,874 accesses within a source projection, 318 with
unproved ranges, and 19 crossing source projections. The 19 crossings occur at
three one-byte globals whose emitted COFF spans are four bytes. Sampled raw VC6
instructions load a dword and mask its value to a byte. These are **not established
buffer overruns**: the current report must retain source projection, emitted
span and actual access footprint as separate evidence. A COFF span can include
alignment bytes and does not by itself enlarge a retail object's logical extent.

13,809 repeated-instruction sites have unproved access counts. A four-byte MOVSD
operand cannot establish a four-byte read when the repeat count may be zero.

Validation so far: 41 targeted tests and 148 sema tests. An independent TSV audit
checks input hashes, every exported expression comparison and all bounded extent
calculations. This checkpoint has not yet run build integration checks because
consumer analysis is still a standalone pass.

## Required work before publishing this stage

- Add path-constrained index bounds and propagate supported call arguments and
  return/field relationships. Preserve unknown aliases and mutating callees.
- Distinguish source projection crossings from actual emitted-span overruns;
  retain widened compiler loads and unknown padding as explicit evidence.
- Correlate shared reader/writer storage and row dimensions across functions,
  including inconsistent declarations and unpaired candidate bodies.
- Establish byte-versus-element units from typed layout and independently proved
  producer/consumer contracts. Names and matching initial bytes are insufficient.
- Integrate exports, summaries and explicit failure policy into the ordinary
  build and exhaustive accounting, then run the full checkpoint and audit.

The next stages remain vendor/declaration/gap recovery and a refreshed generic
base/head audit of PR #78. No rules may use its specific names, addresses, table
sizes or rendering dimensions.
