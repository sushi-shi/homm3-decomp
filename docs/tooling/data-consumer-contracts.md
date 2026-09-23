# Consumer address contracts

Full builds now analyze retail and candidate data consumers alongside static bytes
and startup initialization. This stage is stacked above [#85](https://github.com/sushi-shi/homm3-decomp/pull/85).
It compares machine byte-address relationships, including supported index domains,
read-only calls, and shared storage. It does not turn incomplete observations into
whole-function equivalence or a memory-safety verdict.

```sh
homm3 build
homm3 sema data-accesses --output build/data-accesses
homm3 sema data-accesses --require-exact
homm3 sema coverage --output build/consumer-accounting
```

The ordinary checkpoint reports differences and missing proof. Unavailable fresh
inputs fail the checkpoint. `homm3 build --require-data-exact` additionally requires
complete consumer evidence; the existing backlog fails that optional gate.

## Evidence flow

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
Unknown instructions, control flow, repeat counts and referents remain explicit.
Relative branch families such as LOOP retain their back edge even when their
instruction semantics are unsupported; the first iteration cannot stand for all accesses.

`analysis/access_bounds.py` derives intervals from actual signed/unsigned
comparisons, tests, narrowing operations and masks. Joins widen facts; a bypassed
guard or overwritten flags cannot supply bounds. Unsigned predicates are mapped
to signed intervals before taking a conservative hull. Affine comparisons that
would require an unproved no-wrap assumption are not solved as integer inequalities.
Changed guard domains are reported even when the address expression is identical.

`analysis/access_loops.py` admits one linear counted body, a single induction
update, adjacent comparison and latch, and a unique linear prefix. It proves the
full counter domain, including comparison signedness, step divisibility and wrap
limits. An enclosing back edge invalidates a first-iteration domain. Other loops
remain unbounded. Loop-local identities support ranges; they do not establish
cross-binary loop correspondence. Independent counter execution tests check the
closed-form bounds.

`analysis/access_calls.py` instantiates supported read-only callees using actual
stack arguments and register inputs. Return expressions, receiver fields and
accesses carry through nested summaries. A summary requires supported straight-line
code, balanced stack, preserved nonvolatile registers and no caller-stack/global
writes. Mutating, recursive, depth-limited and unresolved callees remain unproved.
Callee loads after a caller mutation cannot reuse initial-memory identities.
Expanded reads retain their call path and do not add static data-match credit.

`analysis/data_consumers.py` collects reader/writer roles per bound allocation and
compares those bases in independently paired functions. Unique equal symbolic
factor sets expose changed byte coefficients without pairing by instruction order.
Ambiguous repeated shapes remain unpaired. Expression, scale, range and storage
relationship differences are diagnostics; conditional or unobserved effects are
not proved absent.

Source DATA extraction additionally records canonical dimensions, byte strides,
element sizes and pointer element sizes. Equal total bytes cannot hide different
row dimensions. Printed template defaults alone are not a shape contradiction.
These are compiler type/layout facts, not semantic units inferred from names such
as `pitch`. A doubled coefficient in a machine byte address is visible even when
the backing buffer and static bytes are unchanged; names and expected map sizes
never enter the rule.

## Generated evidence

The full build writes these to `build/data-match/`; standalone analysis and full
coverage export the same artifacts to their output directory. Structured fields
are JSON inside literal TSV.

| Artifact | Evidence |
|---|---|
| `data-accesses.tsv` | Direct and contextual accesses, widths, expressions, bounds, spans, loop domains and call paths |
| `data-access-matches.tsv` | Entry-paired functions; missing/extra expressions, changed scales and index domains |
| `data-consumer-calls.tsv` | Actual arguments/registers, callee anchors and read-only/unproved status |
| `data-consumer-storage.tsv` | Typed declarations and retail/candidate reader/writer function IDs |
| `data-contract-issues.tsv` | Unknown effects/entries, shape contradictions and storage relationship differences |
| `data-access-summary.json` | Fixed function denominators, verdict counts, input hashes and policy |

Coverage also exports the enriched `data-declarations.tsv`. Its exhaustive byte
partition and the static matching denominators remain unchanged: observing an
access does not identify all padding or prove the accessed bytes match.

Input content is checked again after analysis. A concurrent source/object change
cannot receive a report bearing a newer hash for an older computation.

## Limits and interpretation

The pass visits all admitted retail functions and every fresh emitted candidate
body, including unpaired entries. It has no function-name, address, table-size or
rendering-size exceptions. Function similarity scores do not supply identities.

Address ranges are conservative footprints. A footprint crossing a declared
projection is not automatically a runtime overrun. Source extents, emitted COFF
spans and retail object boundaries are separate evidence: a COFF span may include
alignment bytes. The reports retain widened compiler loads rather than resizing
one-byte declarations to match a four-byte instruction.

Repeated instructions have unproved counts until independently bounded. A
four-byte MOVSD operand does not establish a four-byte access when its repeat
count may be zero. Heap referents, complex loops, aliasing after writes, mutating
call effects and unsupported instructions remain explicit gaps. Read-only
summaries are bounded to 16 nested analyses. Relationships after unknown calls
are not guessed from source field names.

## Validation

The generic raw-code controls cover shortened tables, bypassed guards, equal-byte
arrays with different rows, split writer/reader bases, doubled strides (including
through a field getter), changed widths, signed/unsigned boundaries, arithmetic
wrap, clobbered flags, unknown relocations, stack overwrites, caller cleanup,
callee cleanup, nested calls and recursion. The tooling suites currently contain
114 targeted tests and 148 sema tests. The exported-data audit checks input hashes,
expression comparisons, bounded extent calculations and shared storage roles.

The vendor/declaration/gap recovery stage and refreshed generic base/head audit of
PR #78 follow this stage. A later case escaping these supported rules must extend
a reusable rule and its generic controls, or remain explicitly incomplete.

## Measured checkpoint

| Measurement | Result |
|---|---:|
| Admitted retail functions / fresh emitted candidate bodies | 11,952 / 9,193 |
| Independently paired function copies | 3,718 |
| Direct and contextual access observations | 279,405 |
| Known address expressions / unproved addresses | 128,784 / 150,621 |
| Read-only call summaries instantiated | 1,643 |
| Agreeing observed-expression pairs / differing pairs | 146 / 951 |
| Pairs with no observed data access / unproved pairs | 63 / 2,558 |
| Within-projection bounded footprints / projection crossings | 25,264 / 69 |
| Observed reader/writer storage differences | 31 |
| Conflicting source array shapes | 2 |

The shape check exposes the existing town-name 9×16/9×17 disagreement and a
15/16-entry help-table disagreement without rules for those symbols or sizes.
The wider footprint findings include compiler loads spanning a narrow source
projection; emitted spans and conditional bounds remain visible for review.
No uniquely paired scale or guard-domain differences were established on this
source snapshot. Generic negative controls exercise both rules; the refreshed
PR #78 audit must establish their coverage of the repaired cases separately.

The static comparison remains at 77,545 matched initialized bytes, 101,207 bytes
of zero-fill agreement and 118,403 unexplained ownership bytes. Consumer evidence
adds no static-byte or padding credit. The full 152-unit checkpoint passes with
function MAX/history unchanged. The optional consumer exact gate exits 1 for the
explicit backlog.

The independent raw-data audit verifies 270,840 decoded memory operands, 484
context call targets, 25,333 bounded footprints, all 3,718 expression comparisons
and reader/writer roles for 1,830 storage anchors. Contextual observations carry
call paths and can reuse the same raw instruction; these are not extra code or
data bytes.
