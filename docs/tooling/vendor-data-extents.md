# Vendor source extents and compiled evidence copies

The strict data checkpoint now reads declarations from every admitted translation
unit, including vendor C sources and their consumed headers. This extends
[declaration ABI binding](data-declaration-abi.md): a vendor contribution can have
an independently checked source size and array shape instead of only a physical
COFF span. Vendor files remain unchanged.

## Source identity and extents

`analysis/data_declarations.py` retains the manifest's selected C/C++ language,
storage class and enclosing function name. C tentative definitions participate as
typed allocation requests; they do not prove which strong or COMMON definition
the linker selects. Extern declarations remain declarations. The new declaration
schema invalidates older cached evidence.

Clang gives C local statics the same underscore spelling as ordinary globals.
`analysis/data_symbols.py` prevents that spelling from joining a global and
recognizes VC6's scoped C-local spelling only with a proven owning TU, C language,
static storage and matching enclosing C ABI function. Candidate lookup retains
all compatible source and emitted owners. Repeated same-name block statics stay
ambiguous; byte values never decide which declaration owns them.

All 14 admitted zlib TUs parse without body fallback. This proves 27 formerly
unknown code-bound source extents, totaling 8,779 logical bytes. Their physical
spans contain nine additional padding bytes, which do not become array elements
or valid source access bounds. The remaining three unknown code-bound extents
come from skipped game function bodies.

## Independent copies of one compilation

The normal manifest build and vendor evidence build compile some of the same
TUs independently. `analysis/data_emissions.py` recognizes those copies through:

- The same admitted manifest TU and source path.
- Identical fresh compiler inputs, including flags, source/header trees and
  toolchain; both raw object hashes must agree with their provenance stamps.
- Equal COFF structure: headers, section sizes/flags, symbols and auxiliary
  records, linker directives and relocation edges.

Initializer and code payload equality is not an ownership test. Each copy is
still compared against retail, and the worse verdict survives wherever copies
overlap. A bad initializer or pointer addend cannot disappear behind a good copy.
Archive members have no admitted TU identity and cannot use this rule; Microsoft
library alternatives retain their independent selection requirements.

A typed source projection and its complete physical contribution may overlap
when they represent the same proved emission. Different source extents still
conflict. Conflicting retail placements withhold the otherwise bound copy.
An independently placed copy may anchor a pointer to the same allocation in the
other copy, with a checked owner-relative addend. The pointer field being tested
never supplies its own destination address.

## Files and measurements

`data-emission-copies.tsv` exports the compiler inputs/layout hashes, raw object
hashes, provenance paths, proof status and allocation pairs. Enrollment and
binding TSVs retain `emission_identity`; relocation anchors identify the actual
independent binding and the allocation borrowing its address proof. Structured
fields use JSON in literal TSV cells. Full builds, coverage exports and the
optional exactness gate consume these proofs.

The corpus contains 13 proved compilation copies and 70 paired allocations.
Code-bound unknown source extents fall from 30 to three; storage declaration
observations rise from 22,614 to 23,029. Static projections rise from 4,648 to
4,675, with 4,112 exact.

There is **no increase in distinct matched bytes**: the vendor comparison already
covered these contributions. The complete 470,624-byte data partition retains
289,421 enrolled bytes, 157,895 initialized matches, 109,404 zero-fill agreements
and 181,203 unenrolled bytes. Every previous byte verdict is preserved. Actual
objdiff compares 4,134 projections across 233 units (217,745 packed bytes) and
explicitly withholds 541. Packed totals include the separate compilation copies;
they are not distinct retail coverage. Zero-fill
agreement remains separate from proof of runtime initialization.

Initialization evidence remains 24 paired registrations and 326 proved constant
effect bytes. Consumer analysis resolves 139,894 of 292,036 address expressions,
up by 13; observed storage relationship differences fall from 65 to 61. All
3,929 storage-role rows remain present. These changes add relationship evidence,
not static-byte credit.

The 169 targeted contract tests and 151 sema tests pass. The full VC6 build,
actual objdiff comparison and existing gates pass with unchanged function scores.
Independent audits reconstruct the copy proofs, source extents, code references,
raw comparison images, EH records, vendor bindings, local labels, initialization
effects and consumer operands. The exhaustive export accounts for all 2,732,032
file bytes and 2,842,624 image bytes; its consumer exports agree exactly with the
audited build. Unexplained data ownership remains 114,335 bytes. The exactness
command exits nonzero on the
remaining backlog, as expected.

Contract tests cover C tentative definitions, local/global spelling collisions,
calling conventions, ambiguous block statics, changed provenance, differing COFF
structure, archive exclusion, conflicting placements, padding, bad copy bytes,
and independently anchored versus unresolved/wrong pointer addends. A disposable
pinned VC6 C fixture checks the supported spellings against emitted raw objects.

The remaining vendor selection/gap review and refreshed PR #78 audit are still
open in the [stack plan](data-matching-stack-plan.md).
