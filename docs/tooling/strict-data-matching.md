# Strict static data comparison

Full `homm3 build` now compares source-enrolled raw VC6 data with pinned retail
and runs **actual objdiff data comparisons**. Open
`build/objdiff/data/objdiff.json` in objdiff; its `report.json` contains the data
section scores. The existing function project and CUR/MAX/HIST ledger keep their
code-only meaning.

```sh
homm3 build
homm3 sema data-match --output build/data-match
homm3 sema data-match --require-exact
homm3 build --require-data-exact
```

The ordinary checkpoint reports differences while the backlog is being recovered.
Missing/stale analysis or an unsuccessful objdiff invocation fails the build.
`--require-data-exact` additionally fails on non-exact enrolled allocations,
unresolved source bindings and source evidence gaps. It currently **fails**, as
expected. This switch requires exact enrolled data, not enrollment of the entire
retail denominator. Unenrolled bytes always remain visible. Fast function builds
skip the data checkpoint and reject `--require-data-exact`.

The following stack stage also adds [initialization checks](data-initialization.md)
to full builds and this exactness gate. The measurements below describe the
static-data stage; initialization effects remain a separate evidence layer.

## Files and interactions

1. `build/data_checkpoint.py` checks compiler provenance before Ninja. Missing or
   content-stale objects are scheduled even when timestamps agree, including
   stale units outside a requested full-build TU. Only their stamps are removed;
   raw objects remain available until compilation replaces them.
2. `analysis/data_declarations.py` retains annotated function entries alongside
   data definitions. `analysis/candidate_data.py` binds fresh emitted storage to
   source declarations and compiler pools. Repeated source uses share one
   projection; conflicting identities poison their overlapping retail bytes.
3. `sema/data_match.py` reads those raw objects and retail directly. Every fixed
   byte is compared. Each DIR32 field needs an independently bound referent and
   an in-range addend. An admitted function proves its entry only: nonzero code
   offsets remain unresolved. Exact complete narrow-string COMDAT copies can
   share a separately bound copy's identity after payload and selection checks.
   Unknown, unsupported, overlapping, partial and missing relocations cannot
   earn pointer-match credit. The field being checked never supplies its own
   destination identity.
4. `build/data_objdiff.py` constructs a companion comparison project from those
   projections. Candidate fixed bytes remain raw. Independently resolved pointer
   values are written at their original allocation-relative offsets; the retail
   side comes from the pinned image. Both objects contain no relocations, so
   objdiff cannot mask a wrong target or addend. Wrong-but-resolved pointers and
   incorrect initializers remain visible differences. Allocations with unknown
   pointer identity, unsupported/missing relocations or conflicting bindings are
   withheld with explicit reasons, and remain non-exact in the strict report.
5. `sema/image_coverage.py` overlays the strict verdicts onto the exhaustive file
   and image partitions. Matched initialized bytes can account for previously
   unresolved gaps; agreement with virtual zero fill cannot establish ownership.

Objdiff objects deliberately **pack logical allocations without padding or retail
gaps**, with one symbol per projection. They are comparison views, not linkable
replacement objects or recovered retail section topology. Zero storage is
materialized to make incorrect initial values visible. Allocation names retain
the retail RVA and candidate symbol. The existing vostok data manifest remains
vtables-only: imposing candidate section spacing on incomplete retail evidence
would manufacture layout proof. The earlier plan's target-topology work is thus
implemented as explicit paired projections rather than fabricated delinker
sections. Per-allocation source sizes still do not prove retail table boundaries.

Data views, their configuration and input/output hashes have provenance stamps.
Before reporting, the build rechecks the source/include/compiler trees as well
as raw objects, manifests, implementations and pinned retail. A separate project
keeps data statistics out of the established function history. Objdiff's aggregate
code percentage is meaningless for this data-only project; inspect its **data
section scores**. Its packed byte total includes repeated candidate copies and
withholds entire unresolved allocations. Use the TSV partition for distinct retail
coverage and byte-exact counts.

## Generated accounting

`build/data-match/` and `homm3 sema coverage --output DIRECTORY` export:

| File | Meaning |
|---|---|
| `data-enrollment.tsv` | Every source-sized retail projection and candidate storage location |
| `data-matches.tsv` | Allocation verdicts, byte counts, hashes and extent limitations |
| `data-relocations.tsv` | Raw target/addend, independent anchors, expected/retail values and verdict |
| `data-byte-verdicts.tsv` | Exhaustive partition of all retail data bytes, including unenrolled intervals |
| `data-enrollment-issues.tsv` | Source binding sites withheld before comparison |
| `data-source-issues.tsv` | Missing or contradictory source evidence |
| `data-match-summary.json` | Denominator, status totals, policy and exact input hashes |

`build/objdiff/data/enrollment.tsv` separately records every projection as compared
or withheld from objdiff. Neither omitted allocations nor a good folded copy can
hide a bad copy in the exhaustive strict verdicts: overlapping copies receive
the worst applicable status. All tabular output uses literal TSV; structured
fields use JSON.

## Measured checkpoint

152 fresh VC6 objects against English GOG Complete 4.0:

| Distinct retail bytes | Count |
|---|---:|
| Full data denominator | 470,624 |
| Enrolled | 194,314 |
| Fixed bytes matched | 75,877 |
| Pointer bytes matched | 1,668 |
| Zero-fill agreement, initialization unverified | 101,207 |
| Fixed differences | 61 |
| Zero-fill differences | 1,051 |
| Missing relocation evidence | 1,174 |
| Unresolved pointer identity | 11,148 |
| Conflicting bindings | 2,128 |
| Unenrolled | 276,310 |

1,847 of 1,963 allocations are statically exact. Objdiff compares 1,867
allocations across 106 units (139,213 packed bytes) and withholds 96 allocations.
Its section scores expose differences in eleven units. Binding failures before
enrollment are a different set of 96 source sites, recorded separately.

The accounting overlay resolves **12,519 additional gap bytes**; unexplained
ownership falls from 142,147 to **129,628** bytes. Ownership accounting includes
the earlier vendor and compiler-structure layers and is not a byte-match score.

Validation includes actual objdiff controls for changed initializers, wrong
pointer targets/addends and exact packed sizes; unresolved pointers, conflicting
bindings and missing relocations are explicitly withheld. Strict checker controls
cover partial/overlapping relocations, interior/one-past addresses, compiler pool
identity, folded-copy disagreement and exhaustive partition preservation.
The targeted 49 tests and 147 sema tests pass. The full VC6 checkpoint and existing
gates pass without function-history changes. An independent exported-TSV audit
reconstructs all comparison images from raw COFF and the PE section table and
verifies the complete 470,624-byte partition. The strict exactness command exits
nonzero on the present backlog.

Registration and supported startup effects now have a separate
[initialization report](data-initialization.md). Shared writer/reader accesses,
independent retail table extents and byte-versus-pixel stride arithmetic remain
following stages of the [stack plan](data-matching-stack-plan.md). Zero agreement
and static byte equality do not establish those relationships.

Full builds also run [consumer contracts](data-consumer-contracts.md): typed table
shapes, supported access bounds, read-only call relationships and shared storage.
These reports do not add static-byte credit. `--require-data-exact` includes their
explicit differences and missing proof.

The [vendor matching stage](vendor-data-matching.md) adds independently code-bound
archive/zlib storage to this same checkpoint and objdiff project. Its measurements
extend the source-only static-data checkpoint above.

The [code-derived allocation stage](code-data-matching.md) adds unannotated game
storage and complete compiler pools, retains reader declarations, and keeps
source bounds distinct from physical COFF spans.

[EH metadata matching](compiler-eh-data-matching.md) adds typed exception records
and checked local cleanup labels. Metadata addresses come from code roots and
private COFF offsets, never from the retail pointer fields being compared.

[Vendor source extents](vendor-data-extents.md) adds typed bounds from admitted
vendor TUs and `data-emission-copies.tsv` for independently compiled evidence
copies. Shared emission identity preserves both comparisons and does not merge
Microsoft archive alternatives or turn physical padding into source bounds.

[Selective body recovery](data-body-recovery.md) retains unaffected local data
when unrelated Clang body errors can be isolated. `data-parse-regions.tsv` records
the excluded spans and original diagnostics; skipped bodies remain incomplete
source evidence for the exactness gate.

[Address operand roles](data-reference-roles.md) admit independently anchored
comparison addresses outside named source extents without expanding those
extents. `code-data-reference-roles.tsv` preserves the operand proofs. Newly
located distinct writer/reader allocations remain conflicts even when both
contain zeros; comparison evidence never relaxes initializer-pointer bounds.
