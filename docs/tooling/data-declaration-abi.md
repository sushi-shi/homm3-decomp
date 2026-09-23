# Binding source declarations to VC6 data symbols

The strict data checkpoint now reconciles specific Clang/VC6 spelling
differences before comparing candidate storage with retail. Previously, a typed
source definition could remain unbound even when independently matched code
located its emitted allocation. This stage extends the [EH metadata
stage](compiler-eh-data-matching.md) without changing game C++ or raw objects.

## Rules and evidence

`analysis/data_declarations.py` retains the files containing actual anonymous
namespace declarations, the enclosing mangled function of each local static,
and whether a stored reference refers to a const array. Source paths are
canonicalized before ownership checks. The declaration cache schema and content
fingerprint invalidate older evidence.

`analysis/data_symbols.py` admits three narrow ABI adjustments:

- Replace a Clang anonymous-namespace hash and a VC6 path/decimal-suffix component
  only in the owning TU. The VC6 path must end in an observed source/header path,
  including its directory. All named scopes and remaining type encoding survive.
- Reconcile the leading underscore and numeric scope discriminator of a local
  static. Its variable name, complete enclosing function signature and complete
  data type must agree. Single-digit and encoded multi-digit discriminators are
  supported; numbers elsewhere in a symbol are not rewritten.
- Reconcile the trailing `B`/`A` spelling of a positively typed const
  array-reference cell. Array dimensions and referent qualifiers remain encoded.
  This rule does not apply to scalar references or ordinary pointers.

The existing VC6 C-style spelling for namespace-scope internal objects remains
supported in the owning TU. A pinned compile probe confirms that VC6 can omit
namespace and type encoding here, including two separate `_same` symbols for
objects in different namespaces. Every compatible source entity and emitted
owner must therefore participate in ambiguity checks.

Exact spelling never suppresses another compatible local scope. The probe also
shows two distinct local statics receiving the same Clang spelling and different
VC6 scope numbers. Multiple source identities or emitted owners remain failures,
including when only one of the competing owners was emitted. These rules do not
resolve ambiguity using initializer values, nearby retail bytes or guessed
source order. Unsupported anonymous-type back-reference compression remains
unbound; demangled display names are not identity keys.

## Pipeline and TSVs

`candidate_data.named_matches` applies the same rules to annotated DATA and
unannotated code-derived definitions. A leading-name index limits the search;
the complete spelling/provenance check establishes each result. Source sizes,
shapes, owner counts and physical storage bounds still determine admissibility.

`data-bindings.tsv` includes `symbol_matches`, keyed by candidate allocation.
`code-data-bindings.tsv` retains these proofs within each typed definition.
Each proof records the original source symbol, unchanged emitted symbol, TU,
applied rules and anonymous namespace origins. Complex fields use JSON inside
literal TSV cells. The new implementation participates in input hash checks.

These proofs only join source and candidate identities. DATA annotations or
independently checked code references still supply retail addresses. Source
types still supply logical candidate sizes, excluding padding. The existing
strict matcher then compares raw initializers and independently resolved pointer
targets, retaining unresolved fields and every unclaimed retail interval.

## Measured checkpoint

| Distinct retail data bytes | PR #89 | Declaration ABI binding |
|---|---:|---:|
| Denominator | 470,624 | 470,624 |
| Enrolled | 265,924 | 289,421 |
| Matched initialized bytes | 140,599 | 157,895 |
| Zero-fill agreement | 104,395 | 109,404 |
| Unenrolled | 204,700 | 181,203 |
| Unresolved pointer bytes | 15,856 | 17,048 |
| Fixed-byte differences | 72 | 72 |
| Pointer differences | 16 | 16 |
| Zero-fill differences | 1,051 | 1,051 |
| Missing relocation evidence | 1,755 | 1,755 |
| Conflicting bindings | 2,180 | 2,180 |
| Unexplained ownership | 114,335 | 114,335 |

The union gains 23,497 enrolled bytes: 17,244 fixed matches, 52 pointer matches,
5,009 zero-fill agreements and 1,192 unresolved pointer bytes. No previous
enrollment or match credit is lost. Zero-fill agreement does not establish
runtime initialization. Source-extent-unproved code bindings fall from 65 to 30;
4,086 of 4,648 static projections are exact. Compiler EH and vendor findings,
code anchors and source-evidence issue counts remain unchanged.

Actual objdiff compares 4,108 allocations across 229 units, totaling 209,006
packed bytes, and explicitly withholds 540 unresolved allocations. The strict
TSVs retain fixed-byte verdicts inside those withheld allocations. Initialization
pairing rises from 22 to 24 registrations and independently verified constant
effects from 230 to 326 bytes. Consumer analysis resolves 139,881 of 292,036
address expressions across 5,680 entry pairs; 65 storage relationships still
differ. These relationship measurements do not add static-byte credit.

Controls exercise wrong TUs, origin paths, named scopes, enclosing signatures,
array dimensions and qualifiers; unsupported anonymous back-references; exact
names competing with alternate local scopes; and distinct namespace/local
entities sharing a compiler spelling. A disposable pinned VC6 fixture confirms
the supported spellings and both ambiguity patterns against raw COFF output.

The 151 targeted contract tests and 151 sema tests pass. The full VC6 build,
actual objdiff invocation and project gates pass with unchanged function scores.
Independent audits replay spelling proofs, source extents and code references,
raw candidate/retail byte comparisons, actual objdiff views, EH records, vendor
bindings, local labels, constant startup effects and consumer operands. The
exhaustive accounting audit verifies all 2,732,032 file bytes and 2,842,624 image
bytes, including the complete data partition; its consumer exports match the
audited checkpoint exactly. The
strict `data-match --require-exact` command exits nonzero on the remaining
backlog, as expected.

Remaining gaps include skipped function bodies, unsupported compiler spellings,
unresolved pointers, archive-selection ambiguity and unproved source extents.
This stage does not complete the [remaining gap review or PR #78
audit](data-matching-stack-plan.md).
