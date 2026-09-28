# Data matching

HoMM3 uses Gruntz's model, data manifests and verification commands. The port
adapts the pinned PE, VC6 names and compiler profiles, and HoMM3's source
annotations. It replaces the separate byte-accounting approach proposed in #73.

## Workflow

```sh
homm3 build                         # compile, delink, compare, checkpoint, README
homm3 compare                       # compare existing objects; no compilation or checkpoint
homm3 compare --reference previous-report.json
homm3 verify data-coverage --all-bytes
homm3 verify data-access --build
homm3 verify data-relocs
homm3 verify data-tu-order
homm3 verify data-coverage --tsv
homm3 verify library-data-refs
```

The normal build refreshes the complete byte report and its README summary.
The verification commands expose findings for review; `--gate`, where offered,
returns nonzero for unresolved findings. Existing findings are not silently
accepted or turned into exclusions.

`DATA` declarations supply source identities and typed extents under the TU's
VC6 ABI. The reviewed function and vtable censuses supply their own boundaries.
A relocation target without a known extent remains a zero-sized address anchor;
it does not own every byte up to the next address. Parse failures, extern-only
claims, ambiguous identities and overlaps remain explicit coverage gaps.

`homm3 model` writes `build/gen/bindings.tsv` and `violations.tsv`. The existing
`symbol_names.csv` is a compatibility export used by the synthetic PDB and
Windows navigation tools. It is not another hand-maintained symbol ledger.

## Relocations and comparison

Vostok consumes `config/retail/relocs.tsv` through its native
`--reloc-manifest` support. The reviewed alias table retains explicit owner and
addend evidence. No pointer-shaped integer scan replaces those inputs.

The Gruntz data and section manifests are generated under `build/gen/`.
Candidate COFF contributes emission and section topology. Retail contributes
the bytes. A target section ordinal is not a candidate object section number.
String identity must resolve unambiguously; folded copies retain their owning
objects. A storage disagreement withholds section placement instead of changing
retail storage to agree with the candidate.

Normalization writes disposable comparison objects and content-based provenance
stamps. It keeps data sections, normalizes compiler-private data names and
materializes COFF COMMON allocations for comparison. Raw compiler objects remain
unchanged. Initializer verification reads those raw objects, so normalization of
alignment or symbol names cannot hide a missing initializer byte.

The CLI and GUI use the same patched objdiff core. `functionRelocDiffs = all`
checks callee/data identity as well as values; absolute relocation addends also
participate. Changing to this scoring policy resets implementation MAX from the
new comparison and preserves earlier peaks in HIST. This is a measurement-policy
change, not a source regression.

## Complete byte accounting

`homm3 verify data-coverage --all-bytes` produces:

| Generated file under `build/gen/` | Contents |
| --- | --- |
| `data_coverage_file.tsv` | Complete partition of executable file offsets |
| `data_coverage_image.tsv` | Complete partition of the loaded image's RVA space |
| `data_initializer_comparison.tsv` | Raw initializer comparison per enrolled owner |
| `data_coverage.json` | Both partitions, initializer verdicts and model violations |
| `data_extraction_issues.tsv` | Declarations whose typed extraction is unavailable |

Every byte occurs once in each applicable partition. Gaps before the first claim,
between claims and after the last claim are included. Nested overlaps are reported;
identical folded copies count once. Section containers have lower priority than
individual definitions. Provisional gap allocations do not count as recovered data.

Ownership and matching are separate. An owned range may contain incorrect bytes.
Initializer verdicts are `exact`, `mismatch`, `unresolved` or `unavailable`.
Pointer words require a known target plus the correct addend, and the relocation
site set must agree. A missing referent does not become an exact comparison by
masking its word. Counts are per enrolled owner, including folded COMDAT copies;
partition totals count physical bytes once.

Verified library/vendor ranges are separate from game data. Runtime labels without
byte proof are `library-unverified`; they are not exempted as verified library
coverage. Unknown zero bytes remain unknown: zeros alone do not prove padding.
PE headers and independently identified resource/relocation sections are structural.

The access report complements byte accounting with instruction-width, stride and
shortfall checks. Its register tracking is local to a basic block; pointer escapes
and accesses through unknown runtime pointers remain blind spots. A report of no
findings is not proof that every datum is fully understood.

## Provenance

The model and verification implementation was ported from local Gruntz revision
`fead802f2`. Vostok remains pinned at `1393e24b4804cb357fdac147c68013f0aa5a9d95`;
its patch adds Gruntz manifest schemas and COMDAT topology, preserving native
relocation TSV handling. VC6-specific function/EH adapters and content-freshness
checks remain in the shared comparison pipeline.

## First integrated checkpoint

The initial full build passed all existing Windows/source gates and the Mac
preservation gate (581/1,503 scored pairs exact, unchanged). No game source or
helper bodies changed.

| Report | Result |
| --- | ---: |
| Unclaimed executable file bytes | 338,658 |
| Conflicting file bytes | 4 |
| Unclaimed loaded-image bytes | 353,136 |
| Conflicting loaded-image bytes | 328 |
| Exact initializer comparisons | 5,389 |
| Mismatching initializer comparisons | 57 |
| Unresolved initializer comparisons | 88 |
| Unavailable initializer comparisons | 1,979 |
| Model violations needing review | 18 |

These findings form the data worklist; they do not all establish a source defect.
The strict checkpoint reports Windows MAX 97.10%, with 2,664/4,769 exact functions.
The preceding README reported 97.59% MAX and 4,395 exact under the earlier policy.
MAX was reseeded for this policy change; HIST retains the previous peaks.
