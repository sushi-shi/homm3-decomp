# Address comparisons and data identity

A named storage address used as a comparison immediate can locate that storage
even when its addend lies outside the source extent. Rejecting every such
reference hid valid placement evidence and, in this corpus, separate allocations
used by loaders and readers. This stage extends [declaration recovery](data-body-recovery.md)
with operand evidence; it does not change C++ or vendor sources.

## Admission

`analysis/code_reference_roles.py` reads the raw i386 COFF code contribution
already located by the code-binding analysis. It follows instructions from typed
function entries through supported branch targets and fallthrough. Symbolic
branches use the same-section symbol and relocation addend. Returns, traps and
unresolved or indirect unconditional jumps do not expose following bytes as
instructions. Overlapping decodes or malformed relocation fields reject proof.

The additional placement rule accepts only a DIR32 relocation occupying the
complete immediate field of `cmp register32, immediate32`. The relocation must
name an already identified allocation; a section-symbol reference into padding
does not identify its logical owner. The proof records the function entry,
instruction path, raw bytes, operand, relocation and owner-relative offset.

`code_data_bindings.py` preserves the independently established logical size,
source identity and placement checks. One accepted comparison cannot hide an
unsupported outside load or conflicting placement. Data initializer pointers
still require their original in-range referents. Comparison evidence proves
neither a memory access nor a valid loop bound; consumer bounds remain separate.

Full builds and coverage export `code-data-reference-roles.tsv`, with JSON proof
cells and a corresponding nested proof on each affected binding. The helper is
included in input hashes so old comparison reports cannot remain fresh after a
rule change. The source-address disagreement counter also stops counting
unannotated objects accidentally inserted by a lookup into its source index.

## Observed storage conflicts

The corpus has 121 outside references: 49 proved comparison immediates, 54 memory
displacements and 18 without supported instruction paths. Of 41 bindings whose
outside references are comparisons, 35 become bound; six retain independent
placement or source-address conflicts. Code-bound allocations rise from 3,315
to 3,350. Bindings rejected for outside references fall from 80 to 39.

Seven newly placed arrays in `text.cpp` overlap separately defined reader
storage. These are distinct source entities and raw allocations, with no admitted
linker or compiler-copy identity. Equal zero bytes cannot merge them.

| Loader storage | Reader storage | Loader bytes |
|---|---|---:|
| `g_giveResourceWindowHelp` | `tradpost.cpp`: `g_giveHelpText` | 40 |
| `g_sellCreatureWindowHelp` | `tradpost.cpp`: `g_sellCreaHelpText` | 40 |
| `g_universityWindowHelp2` | `university_window.cpp`: `g_universityWindowHelp`, `g_universitySkillHelpFormat` | 32 |
| `g_spellbookHelp` | `spellbookwindow.cpp`: `g_spellbookHelpText` | 88 |
| `g_buyArtifactWindowHelp` | `tradpost.cpp`: `g_buyArtHelpText` | 40 |
| `g_sellArtifactWindowHelp` | `tradpost.cpp`: `g_sellArtHelpText` | 40 |
| `g_resourceWindowHelp` | `tradpost.cpp`: `g_marketHelpText` | 48 |

The loader writes these arrays in its text-loading loops; the reader TUs access
their separate arrays for help text. The university declarations also disagree
in shape. These findings require source/retail investigation before repair;
this report does not claim a runtime reproduction or prescribe a replacement
layout.

| Distinct retail data bytes | PR #92 | Reference roles |
|---|---:|---:|
| Total | 470,624 | 470,624 |
| Enrolled | 289,581 | 289,593 |
| Matched initialized | 157,951 | 157,951 |
| Zero-fill agreement | 109,508 | 109,192 |
| Binding conflicts | 2,180 | 2,508 |
| Unenrolled | 181,043 | 181,031 |

The seven projections expose 328 conflicting bytes: 316 previously credited as
zero-fill agreement and 12 previously unenrolled. No initialized-match credit
changes. The lower zero-fill total is a corrected identity verdict, not a source
or compiler regression. Static exact projections fall from 4,119/4,682 to
4,111/4,689. Missing relocations, unresolved pointers and differing-byte totals
remain unchanged.

Actual objdiff compares 4,133 projections across 233 units, totaling 217,589
packed bytes, and withholds 556. The packed total falls by 316 bytes because
previously admitted reader projections now have conflicting storage identities.
Initializer evidence stays at 24 paired registrations and 326 constant-effect
bytes. Consumer expressions known fall from 139,922 to 139,863 out of 292,036;
59 previously bounded observations lose their unique storage mapping. The
reported relationship differences fall from 61 to 60 because one affected
consumer becomes unproved, not because a source defect was repaired.

The 210 targeted contract tests and 152 sema tests pass. Controls cover genuine
instruction paths, branches over inline bytes, symbolic branch targets, malformed
and overlapping relocations, memory comparisons, outside loads, section padding,
competing placements, unknown/conflicting extents and outside pointer initializers.
A separate-writer/reader control keeps equal zero-filled allocations conflicting.
The full VC6 build, actual objdiff and repository gates pass with all 4,781
function-ledger rows unchanged. Independent audits reconstruct raw COFF and
retail comparisons, source/vendor address proofs, each accepted comparison path,
compiler/copy identities, initialization effects and consumer operands. They
also verify the exact 316-byte credit loss and 12-byte enrollment gain against
the preceding checkpoint, with no other byte-verdict changes.

The exhaustive coverage audit verifies all 2,732,032 file bytes and 2,842,624
image bytes. Its consumer, parse-region and reference-role exports are identical
to the audited build checkpoint. Unexplained data ownership remains 114,175
bytes. The strict exactness command exits nonzero on the visible backlog, with
no unavailable analysis and the same static report as the full build.

The [remaining gap review and current PR #78 audit](data-matching-stack-plan.md)
remain open. The generic comparison rule contains no game names, retail addresses,
table counts or rendering dimensions.
