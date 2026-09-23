# Data declarations outside unsupported function bodies

A Clang error in one function previously made declaration extraction skip every
body in its TU. That hid unrelated local arrays and DATA annotations even when
VC6 emitted their storage and independently matched code located it. This stage
extends [vendor source extents](vendor-data-extents.md) by retaining declarations
outside explicitly excluded bodies. Authored source and VC6 input stay unchanged.

## Admission and exclusions

`analysis/data_body_recovery.py` can isolate diagnostics inside ordinary,
source-spelled compound bodies in the main source file. It replaces only the
interior bytes of those bodies with spaces in an unsaved analysis buffer,
preserving braces, byte offsets and line endings. It records each body span,
function identity and original hash, plus hashes of both source buffers.

The rule rejects errors outside a supported body, diagnostic ranges crossing its
boundary, fatal errors, function try blocks, templates, deduced return types and
explicit constant-evaluation function declarations. Macro-spelled bodies,
preprocessing activity, directives, line splices and unsupported encodings also
prevent isolation. No replacement statements, fabricated initializers or source
annotations are introduced.

The analysis buffer must parse without errors. All retained static-storage
declarations must agree with the original parse in source position, source
identity, symbol, type, storage/linkage, definition status, enclosing scope,
extent, array shape and element widths. Every observation participates in this
comparison; duplicate observations cannot silently overwrite one another.
A disagreement rejects the recovery and keeps the existing skip-all-bodies
fallback.

`analysis/data_declarations.py` preserves the original diagnostics and marks both
selective exclusion and the full fallback as incomplete source evidence. Locals
inside an excluded body are unavailable. The optional exactness gate still fails
on skipped bodies, even if every enrolled allocation happens to match. The
source-cache schema and implementation fingerprint invalidate older evidence.

## Reports and measurements

`data-parse-regions.tsv` records every TU's parse mode, original diagnostics,
recovery status/reason, excluded regions and hashes. Nested proof fields are JSON
inside literal TSV cells. Full builds and coverage export the same records;
`data-source-issues.tsv` retains the source-evidence gaps. Source-derived extents
still require their ordinary raw-COFF identity and independent retail address
proofs before strict comparison.

The corpus retains full parsing for 126 TUs, isolates 19 bodies in eight TUs,
and keeps the full fallback for 18 TUs. Storage declaration observations rise
from 23,029 to 23,045. All seven previously unbound DATA annotation sites become
bound. The 26 TUs with incomplete body evidence remain visible.

The three remaining code-bound unknown extents now have source sizes. Two
16-byte arrays gain valid code-derived bindings. The third is eight bytes, but
its code contains relocations at offset nine, used in loop-limit comparisons;
its code-derived binding remains withheld by the current address-range rule.
Its independent DATA annotation still permits the ordinary static comparison. A known extent is not automatically a valid
binding, and an address-forming comparison is not itself an out-of-bounds load.
A following general range-proof pass must distinguish those uses without
expanding the array or accepting arbitrary outside references.

| Distinct retail data bytes | PR #91 | Body recovery |
|---|---:|---:|
| Total | 470,624 | 470,624 |
| Enrolled | 289,421 | 289,581 |
| Matched initialized | 157,895 | 157,951 |
| Zero-fill agreement | 109,404 | 109,508 |
| Unenrolled | 181,203 | 181,043 |

Seven additional projections enroll 160 bytes: 56 initialized matches and 104
zero-fill agreements. No prior byte credit is lost. Zero agreement does not
prove dynamic initialization. The remaining static mismatch, unresolved-pointer,
missing-relocation and conflicting-binding byte totals are unchanged. There are
4,119 exact projections out of 4,682. Actual objdiff compares 4,141 projections
across 233 units, totaling 217,905 packed bytes, and withholds 541.

Initialization evidence remains 24 paired registrations and 326 proved constant
effect bytes. Consumer analysis resolves 139,922 of 292,036 expressions, up by
28, and checks 32,690 bounded ranges. The 61 observed storage relationship
differences remain; relationship evidence does not add static-byte credit.

The 180 targeted contract tests and 152 sema tests pass. The full VC6 build,
actual objdiff and existing gates pass with unchanged function scores. Independent
audits replay the eight source exclusions, raw comparisons, source/vendor address
proofs, compiler records, initialization effects and consumer operands. The
exactness command exits nonzero on the visible backlog, with no unavailable
analysis. The exhaustive export verifies all 2,732,032 file bytes and 2,842,624
image bytes, with identical parse-region and consumer evidence across build and
coverage reports. Unexplained data ownership falls from 114,335 to 114,175 bytes.

Controls cover retained local dimensions and source coordinates, excluded local
storage, multiple failing bodies and nested scopes, header/signature/global
errors, preprocessing effects, deduced/template/constant-evaluation functions,
encoding failures, errors after reparsing and changed retained declarations.
TSV round-trip and exactness controls keep skipped-body evidence visible.

The [remaining gap review and PR #78 audit](data-matching-stack-plan.md) remain
open; this stage recovers source facts and does not repair game behavior.
