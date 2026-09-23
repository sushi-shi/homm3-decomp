# Code-derived data allocations and declaration evidence

Full `homm3 build` now extends strict data matching to unannotated source
storage and compiler pools whose addresses can be proved from matched code.
This stage builds on [vendor matching](vendor-data-matching.md); its results
also feed initialization checks, consumer checks and the companion objdiff
project at `build/objdiff/data/objdiff.json`.

## Evidence flow

`analysis/data_declarations.py` retains typed definitions and every static-storage
declaration, including unannotated extern readers and local statics. Declarations
of the same source entity participate in size and shape checks. An incomplete
array remains unknown; the parser avoids a libclang alignment query that crashes
when its record element is also incomplete. Twenty-six units still require the
explicit body-skipping fallback. That fallback cannot prove unseen local storage.

`analysis/code_data_bindings.py` reuses the independently rooted code graph from
`analysis/vendor_bindings.py`. Fixed code bytes establish code contributions;
their relocations, signed addends and symbol offsets locate individual data
allocations. Data initializer values never establish addresses. Candidate
section spacing does not determine retail spacing: references to two allocations
can place them independently, while a section-symbol reference supplies an
address only for the allocation containing its addend.

The pass joins emitted allocations to typed source definitions using the existing
VC6 symbol rules. All observed sizes and shapes must agree. A wrong code addend
can contradict a valid DATA address: the code proposal remains non-exact while
the independent DATA anchor remains available to diagnose the incorrect access.
Competing code placements are retained; the pass never chooses the placement
whose initializer happens to match.

Extent evidence remains explicit:

| Kind | Admitted extent | Consumer interpretation |
|---|---|---|
| `source` | Consistent typed declarations or existing source binding | Source bound, with raw emitted span checked separately |
| `compiler-type` | A complete, narrowly recognized VC6 scalar type suffix | Scalar storage length; no inferred source array bound |
| `coff-contribution` | Complete named COMDAT span with supported linker identity | Physical storage only; source extent remains unproved |
| `unknown` | No supported extent | Withheld from comparison |

The scalar rule covers integer, floating-point and bool storage, including
compiler byte guards. It does not infer array, pointer or record sizes. Ordinary
symbol-to-symbol spacing cannot turn padding into a source object. An emitted
extent does not independently establish the original retail object boundary.

Function pointer identities now preserve linkage: an internal function can anchor
references inside its own object, but cannot resolve another object's external
reference merely because its spelling agrees.

## COMDAT identity

VC6 vtables emitted with RTTI can use LARGEST selection and place a locator before
the public symbol. Copies without RTTI can use ANY selection. The public tail
can share a linker identity; the anonymous prefix cannot inherit that identity.
The [PE/COFF specification](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format)
defines the selection values, and the
[LLVM linker implementation discussion](https://lists.llvm.org/pipermail/llvm-commits/Week-of-Mon-20190211/627651.html)
documents this ANY/LARGEST compatibility for Microsoft objects.

A synthetic control using the pinned VC6 linker selects the larger contribution
and preserves its prefix in both object orders. The comparison still checks
every enrolled copy and gives overlapping bytes the worst verdict. Equal values
alone do not identify distinct allocations or excuse an incorrect copy.

## Generated files

The ordinary checkpoint and `homm3 sema coverage --output DIRECTORY` export:

| TSV | Contents |
|---|---|
| `code-data-bindings.tsv` | Allocation proposals, sizes, source facts, code references and rejection reasons |
| `code-data-anchors.tsv` | Checked and unproved code contributions and rooted paths |
| `code-data-objects.tsv` | Source object identities and hashes needed to reconstruct paths |
| `code-data-issues.tsv` | References without a supported emitted owner |
| `code-data-declarations.tsv` | All static-storage declarations, emitted owners, bound addresses and proposed addresses |

Complex fields use JSON inside literal TSV cells. The existing
`data-byte-verdicts.tsv` partitions all 470,624 retail data bytes, including gaps.
Packed objdiff totals retain repeated emitted copies and therefore are not the
distinct-byte coverage denominator. Source, compiler, object and implementation
hashes remain part of freshness validation.

## Measured checkpoint

Against the same pinned retail and source snapshot as PR #87:

| Distinct retail data bytes | PR #87 | With code-derived allocations |
|---|---:|---:|
| Denominator | 470,624 | 470,624 |
| Enrolled | 219,960 | 232,920 |
| Matched initialized bytes | 96,437 | 106,595 |
| Zero-fill agreement | 104,257 | 104,395 |
| Unenrolled | 250,664 | 237,704 |
| Fixed-byte differences | 61 | 65 |
| Resolved pointer differences | 0 | 16 |
| Zero-fill differences | 1,051 | 1,051 |
| Missing relocation evidence | 1,750 | 1,754 |
| Unresolved pointer identity | 14,168 | 16,864 |
| Conflicting bindings | 2,236 | 2,180 |
| Unexplained ownership | 118,213 | 114,335 |

The union gains 12,960 enrolled bytes and 10,158 matched initialized bytes, with
no loss of prior enrollment or initialized-byte matches. Newly exposed
differences remain differences. Zero-fill agreement does not prove initialization.

The new pass binds 2,365 allocations: 1,370 with source extents, 108 with compiler
scalar extents and 887 with complete COMDAT spans. It retains 492 unknown extents,
40 conflicting code placements, 79 out-of-extent code references, three source
address conflicts, two source size conflicts and four COMMON requests whose
initializer selection remains unproved. Vendor ambiguity remains explicit too.
There are 4,881 anchored code contributions and 1,142 unproved proposals.

Combined strict comparison finds 3,084 of 3,723 projections statically exact.
Objdiff compares 3,106 projections across 216 units, withholding 617, for 166,089
packed bytes. Initialization pairs rise from 20 to 22 of 1,149 retail slots;
proved constant effects remain 230 bytes. Consumer entry pairs rise from 4,260
to 5,680; 138,663 of 291,996 address expressions are known. These partial checks
do not establish complete startup or access correctness.

Validation includes 110 targeted tooling tests, 150 sema tests, and a full VC6
build with no function-ledger changes. Independent audits reconstruct all new
bound addresses from raw code and 14,376 relocations, check 22,614 declaration
exports, validate the unchanged vendor paths, and reconstruct objdiff images
from raw COFF. Consumer audits verify raw operands, bounded ranges, call targets
and paired expressions. Coverage audits verify the complete 2,732,032-byte file
and 2,842,624-byte mapped image partitions, with checkpoint and coverage exports
identical. The strict exactness CLI exits nonzero on the remaining backlog.
Generic negative controls cover reordered storage,
changed initializers, conflicting reader dimensions, wrong code addends, unknown
extents, local/external pointer identity and anonymous COMDAT prefixes.

Remaining compiler metadata, declaration gaps, unresolved linker selection and
the refreshed PR #78 audit remain in the [stack plan](data-matching-stack-plan.md).
This checkpoint extends stage 6; it does not close that stage or the overall goal.
