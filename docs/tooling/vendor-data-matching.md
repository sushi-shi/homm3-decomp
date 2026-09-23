# Vendor storage in strict data comparison

Full `homm3 build` now enrolls independently located data from the pinned
`LIBCMT.LIB`, `LIBCPMT.LIB`, and fresh zlib objects in the ordinary strict data
checkpoint and its companion objdiff project. This extends the earlier vendor
ownership accounting: attribution alone did not prove candidate pointer bytes.

## Evidence and files

`analysis/vendor_bindings.py` starts from admitted runtime and zlib code symbols.
It compares complete COFF code contributions against executable retail storage,
allowing only the existing trailing code-NOP rule. Checked code-to-code edges
extend the rooted proof; code-to-data relocations place data contributions using
their actual signed addends and symbol offsets. Data initializers never establish
addresses. Changing an initializer therefore preserves enrollment and produces
a strict byte difference. Data-only pointer chains remain unresolved until their
destinations have independent anchors.

The shared COFF topology lives in `analysis/vendor_data.py`. Its earlier
attribution pass still follows data edges, but that pass supplies no strict
pointer identities. The two measurements remain separate.

`sema/data_match.py` merges vendor bindings with fresh source bindings, compares
the original bytes, and partitions the complete retail data denominator.
`build/data_objdiff.py` uses the same explicit unit-to-object selection map.
Private zlib provenance files can share basenames with normal build objects;
they cannot replace the selected candidate object. Archive hashes, extracted
member hashes, private compiler stamps, source/header/compiler trees and binding
implementation hashes remain checked. A normal full build refreshes stale private
zlib objects; read-only analysis reports unavailable inputs explicitly.

Generated files accompany the existing strict reports in `build/data-match/`
and `homm3 sema coverage --output DIRECTORY`:

| TSV | Evidence |
|---|---|
| `vendor-data-bindings.tsv` | Candidate allocations, retail addresses, code paths and withheld alternatives |
| `vendor-code-anchors.tsv` | Checked or rejected code contributions and rooted paths |
| `vendor-binding-objects.tsv` | Every archive/member or private object needed to reconstruct those paths |
| `vendor-binding-issues.tsv` | Missing, stale or malformed inputs |

The exhaustive `data-byte-verdicts.tsv` remains the distinct-byte denominator;
packed objdiff totals include repeated emitted copies. Code evidence is exported
even for ancestors that do not themselves contribute enrolled data.

## Linking and boundaries

COMMON copies and complete COMDAT allocations with supported selection rules
can share an explicit linker identity. Same bytes or zero fill alone do not
establish identity. Every admitted copy is compared; a matching copy cannot hide
a differing copy. Incompatible identities and overlapping extents remain
conflicts. Ordinary duplicate definitions in alternative archive members are
withheld unless independently proved; a reference does not select the most
convenient candidate layout. Conflicting code placements remain visible.
COMMON requests with possible strong definitions in the archives or source
objects remain unproved until linker selection establishes their initializer.
Eight such requests cover 16 distinct CRT bytes; treating them as zero-filled
objects would have manufactured differences against initialized retail storage.

COFF spans establish emitted storage lengths, not original source array bounds.
Consumer reports retain those physical spans separately and do not award
`within-source-extent` proof without a source extent. Zero-fill agreement remains
separate from initialization effects. This work does not change game C++ or the
function-score ledger.

## Measured coverage

Against the same pinned retail and 152-unit source snapshot as PR #86:

| Distinct retail data bytes | Before | With vendor enrollment |
|---|---:|---:|
| Denominator | 470,624 | 470,624 |
| Enrolled | 194,314 | 219,960 |
| Matched initialized bytes | 77,545 | 96,437 |
| Zero-fill agreement | 101,207 | 104,257 |
| Unenrolled | 276,310 | 250,664 |
| Missing relocation evidence | 1,174 | 1,750 |
| Unresolved pointer identity | 11,148 | 14,168 |
| Conflicting bindings | 2,128 | 2,236 |
| Unexplained ownership bytes | 118,403 | 118,213 |

The union gains 25,646 enrolled bytes and 18,892 matched initialized bytes.
No previously enrolled or initialized-matched bytes are lost.
These are strict comparison gains; most vendor ownership was already attributed
by the earlier stack stage. New missing-relocation and unresolved-pointer rows
expose previously unenrolled obligations, rather than masking them.

The pass retains 734 bound allocations from 106 members/objects, with ten
ambiguous-definition bindings, ten conflicting-placement bindings, and eight
COMMON bindings with unproved initializer selection withheld.
There are 768 rooted code contributions and 189 unproved code proposals.
Across source and vendor data, 2,356 of 2,697 projections are statically exact.
Fixed differences remain 61 bytes; zero-fill differences are 1,051 bytes.

Validation: 84 targeted tooling tests and 149 sema tests pass. Negative controls
cover changed initializers, circular pointer evidence, wrong independently
resolved pointers, competing archive layouts, code outside executable storage,
COMDAT copy disagreement, arbitrary equal bytes, COMMON size conflicts, object
basename collisions and stale provenance. The full 152-unit VC6 build and all existing gates pass with no function-ledger
changes. Independent TSV audits reconstruct the code paths, verify all 470,624
data bytes, and rebuild the objdiff images from raw COFF: 2,376 allocations
compared across 199 units, 321 withheld, and 158,724 packed bytes. The consumer
audit checks 291,996 observations, including 282,248 raw operands and 4,260
paired expression comparisons. The strict exactness CLI correctly exits nonzero
on the remaining backlog.

Further game/compiler-pool recovery and the refreshed PR #78 audit remain in the
[stack plan](data-matching-stack-plan.md). A preliminary reuse probe finds many
game data sections whose references imply inconsistent whole-section placements;
those layouts need allocation-level evidence, not a relaxed placement rule.
