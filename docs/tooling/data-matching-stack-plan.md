# Plan: candidate/retail data coverage and cross-function contracts

## Objective and starting point

Build a sequence of PRs stacked above #81 (`827ae6e4`), itself above #73.
Pursue all remaining accounting vectors and maximize **verified candidate/retail
byte coverage**, then evaluate the general checks against the failures repaired
in PR #78. Accounting, emitted storage, exact initialized bytes, pointer identity,
initialization and access correctness are different measurements.

The user's recurring failure classes are mandatory acceptance requirements:

- Table extent: consumer bounds must fit the whole backing table, not merely
  the subset initialized by one parser (156 playable heroes versus 163 portraits).
- Shared storage: writers and readers must refer to one actual storage identity,
  even when inconsistent C++ array dimensions produce different linker symbols.
- Initialization: required initializers, registrations and pointer bindings must
  exist and run in the relevant order before consumers.
- Units: byte strides and element/pixel strides must not be confused. Correct
  buffer size and initial bytes alone cannot establish correct address arithmetic.

The user is independently continuing interactive rendering checks across map
sizes. Preserve that work and use its findings as evidence; do not replace it
with per-function mocks or equate a successful launch with complete matching.

Verified starting measurements: 470,624 data-section bytes; 205,235 bytes under
usable DATA declarations; 38,630 additional vendor-accounted bytes; 226,759
unaccounted bytes. Of the last group, 84,612 are already structurally identified
on retail (72,572 EH metadata, 12,020 vtables, 20 import terminator), 80,783 are
provisional strings, and 61,364 remain unknown. These buckets describe accounting,
not candidate matching. Vendor candidates overlap them.

Current implementation gaps inspected on the starting branch:

- `build/data_manifest.py` emits vtable rows and empty data-section/compiler-data
  binding tables, despite 1,535 DATA_COMPGEN/guard annotation sites.
- `build/normalize_objs.py::_drop_data_sections` erases non-code sections from
  disposable comparison copies. The ordinary scoreboard remains code-only.
- `analysis/data_declarations.py` reports source extents, but cannot prove VC6
  emission, referent identity or consumer access bounds. Twenty-six TUs skip bodies.
- Existing `analysis/access_facts.py` checks member visibility, not memory-access
  widths or units; it must not be mistaken for an existing stride checker.

## Shared evidence and artifacts

All stages consume the pinned retail image, hand-admitted inventories, source
annotations, manifest compiler profiles, and fresh raw VC6 objects. Keep source
names authoritative and generated facts under `build/`; do not add a parallel
hand-maintained symbol registry. Freshness includes source/header/profile/tool
content and consumed object hashes. Unknown, stale, ambiguous and unsupported
evidence cannot become a successful empty report.

Use one reusable model joining retail spans, candidate section/symbol/offset,
source identity, declared/emitted extent and alignment, relocation target plus
addend, initializer ownership and consumer accesses. Separate addresses from
identities: relocating an object legitimately changes its address, not its owner.
Do not resolve a pointer solely from the same field whose correctness is tested.
Retain alternative placements and contradiction provenance.

Keep `coverage.tsv` exhaustive and enriched. Preserve `data-declarations.tsv`,
`data-gaps.tsv`, `data-issues.tsv`, vendor evidence and unaccounted spans. Add
generated tables as their producers land:

- `compiler-data.tsv`: structural families, ownership, extents and proof.
- `candidate-data.tsv` and `data-bindings.tsv`: emitted contributions/objects and
  source/retail bindings, with rejected and ambiguous associations.
- `data-matches.tsv` and `data-relocations.tsv`: fixed-byte verdicts, pointer
  owner/addend verdicts, compared/enrolled/excluded ranges and explicit reasons.
- `data-initialization.tsv`: initializer slots, definitions, writes, registrations
  and supported ordering dependencies; dynamic initialization is not static zero-fill.
- `data-accesses.tsv`: access widths, address expressions, proven bounds and units,
  producer/consumer relationships and unproved cases.
- `data-contract-issues.tsv`: cross-function contradictions and missing proof.

Summaries retain fixed retail denominators and distinct byte unions for identified,
bound, enrolled, fixed-byte matched, pointer-verified and still-unmatched storage.
Never add independent file/image totals, count alias/COMDAT copies twice, count
BSS as initialized-byte success, or silently migrate historical code scores.

## Stacked PR sequence

### 1. Compiler/linker structures

Implemented on `codex/compiler-data-accounting`: the measured structural layer
accounts for another 84,612 gap bytes and leaves 142,147 unaccounted. Candidate
compared/matched bytes remain zero. See [evidence and validation](compiler-data-coverage.md).

Add `sema/compiler_data.py`; extend the accounting overlay/export in
`sema/image_coverage.py`. Reuse validated EH records, vtables and bounded PE
structures. Attach records to owning handlers/functions/classes where supported;
retain unknown ownership. Classify the final import descriptor structurally.
Provide a compiler/system accounting channel separate from both DATA declarations
and vendor libraries. Candidate match status remains unverified until later stages.

Acceptance: byte unions and every original denominator remain unchanged;
structural attribution cannot increment candidate matched bytes. Malformed
records, overlap, missing owners and partial evidence remain visible.

### 2. Source-to-COFF bindings, including DATA_COMPGEN

Implemented on `codex/candidate-data-bindings`: 2,565 sites bind to fresh emitted
storage, projecting onto 194,314 distinct retail bytes. All eleven annotated
guards have one-byte candidate ownership proof. Ninety-six binding-site failures
remain explicit. See [candidate binding evidence](candidate-data-bindings.md).
Canonicalizer bindings now carry real candidate topology. Target topology and
ordinary data enrollment move together in stage 3, so this stage cannot manufacture
complete retail sections from partial bindings or claim byte equality.

Add `analysis/candidate_data.py` and a reusable binding layer using the existing
COFF parser, source annotation scanner and declaration evidence. Extend
`build/data_manifest.py` to emit actual enrolled storage topology and compiler-data
bindings rather than empty tables. Reuse `canonicalize_data_symbols.py` contracts.

Cover ordinary globals, externs, reference/pointer cells, local statics, literal
pooling/suffix sharing, guards, COMMON, initialized aggregates and BSS. Correlate
all declarations/definitions and detect contradictory dimensions or multiple
emitted owners for one claimed retail identity. Improve partial declaration
extraction and fix supported declaration defects without hiding failed parsing.

Acceptance: removed/shortened/moved storage, duplicate definitions, inconsistent
declarations, pooled copies and changed compiler-private names produce correct
bindings or explicit failures. No candidate extent alone proves a retail boundary.

### 3. Strict data comparison and ordinary build enrollment

Implemented on `codex/strict-data-matching`: strict raw-COFF/retail comparison,
automatic freshness repair, exhaustive TSV verdicts and actual objdiff data
comparison in a companion project. Explicit packed allocation projections retain
known extents without inventing retail gaps or section layout; the function
project keeps its existing score history. See [policy, files and measured
coverage](strict-data-matching.md). Initialization and consumers remain below.

Add `sema/data_match.py` over those bindings. Compare every enrolled fixed byte
and resolve relocation identity/addends independently. Extend build configuration,
normalization/freshness, CLI and checkpoint reporting so the ordinary build runs
data comparison with an explicit policy and separate data results. Keep existing
code-history semantics intact; do not simply stop stripping data and count whatever
objdiff happens to open. Only deliberately enrolled target topology is comparable.

Acceptance: altered initializers, wrong referents/addends, missing symbols, dropped
units and changed extents fail their relevant gates. Every enrolled byte has a
verdict. Unenrolled bytes remain in the fixed denominator and explicit backlog.
Pointer masks cannot turn an incorrect relationship into an exact match.

### 4. Initialization, initializer arrays and RTTI

Implemented on `codex/data-initialization`: bounded CRT tables, candidate
registrations, conservative write/callback evidence, within-object order checks,
per-storage initialization diagnostics and validated RTTI graphs. The ordinary
checkpoint runs these checks and retains unsupported effects explicitly. See
[evidence flow and measurements](data-initialization.md). Consumer analysis and
remaining candidate/vendor closure continue in the following stages.

Add `analysis/data_initialization.py`; extend compiler-data parsing and the
shared binding/relocation graph. Prove CRT initializer-array bounds and slots
against retail, bind emitted initializer entries to functions, and compare
registrations and writes to their actual storage owners. Parse bounded RTTI
records and pointer relationships with explicit structural validation.

Acceptance: missing initializer bodies/slots, skipped writes, wrong destination
globals, incorrect callback registrations and evidenced ordering inversions are
diagnosed generically. All-zero BSS cannot pass as completed initialization.
Unsupported dynamic effects remain unproved rather than guessed through names.

### 5. Consumer extents and byte/element units

Implemented on `codex/data-consumer-contracts`: raw byte-address expressions,
branch/counting bounds, read-only call summaries, typed array shapes and shared
reader/writer storage checks. Full builds and coverage export the same TSVs;
unsupported effects remain explicit and the optional exact gate rejects the
backlog. See [evidence, measurements and limits](data-consumer-contracts.md).
The refreshed PR #78 audit below must still test the repaired cases and extend
reusable rules where needed.

Add `analysis/data_accesses.py` for retail and candidate accesses, building on
existing disassembly/CFG tools and compiler-bound source layout where useful.
Compare access widths and address expressions (base + index*scale + displacement),
recover supported loop/index bounds, and propagate evidenced producer/consumer
contracts across call arguments, returned values and shared fields.

Use typed pointer element widths, instruction scaling and evidenced API/field
semantics to distinguish byte counts from element counts. Names such as `pitch`
alone are not proof. General affine/bounded cases should work independently of
hero/town/minimap names, addresses, dimensions and map sizes; non-affine or
insufficiently evidenced cases must remain explicit.

Acceptance: a consumer iterating more rows than emitted storage is caught; a
writer and reader using different bases or row dimensions is caught; multiplying
an already-byte stride by pixel size is caught even when buffer size and initial
bytes agree. Test generic defect patterns, plus actual retail/candidate expressions.

### 6. Remaining vendor candidates, declarations and gap recovery

Vendor enrollment is implemented on `codex/vendor-data-matching`, stacked on
the consumer-contract stage. [Vendor strict matching](vendor-data-matching.md)
adds code-only address proofs, explicit COFF coalescing rules and ordinary
objdiff comparison, gaining 25,646 enrolled bytes and 18,892 matched initialized
bytes. [Code-derived allocations](code-data-matching.md), on
`codex/code-data-bindings` above the vendor stage, retain all static-storage
declarations and place individual game allocations and complete compiler pools
through checked code. This gains another 12,960 enrolled bytes and 10,158 matched
initialized bytes. Remaining compiler metadata, declaration reconciliation and
the final gap review are still in progress; this does not close stage 6.

[EH metadata matching](compiler-eh-data-matching.md), on `codex/compiler-eh-data`
above that allocation stage, extends the same model to typed private compiler
contributions, associative COMDAT ownership and checked local code labels. It
adds 886 typed records while excluding inter-record spacing. Remaining ordinary
source extents, linker selection and the final gap review still need attention.
The verified union gain is 33,004 enrolled bytes and 34,004 initialized matches;
remaining unknown code-bound extents fall from 492 to 65. The next declaration
pass must reconcile compiler ABI spellings without using names alone as proof.

Apply the shared model to vendor candidates, unresolved external pointers,
alternative archive members, pooled data and compiler-created storage. Resolve
supported ambiguities with independent anchors; never lower proof thresholds
just to improve totals. Use the resulting gap/access reports to recover additional
arrays, strings, constants, globals and proven alignment regions. Reconcile
unknown/conflicting declarations and recover skipped local-static sites.

Acceptance: account and match additional real bytes, recording before/after
unions and remaining reasons. Re-run negative controls for pooling, relocation
ambiguity, stale objects and overlapping extents. Zero bytes alone never prove padding.

### 7. PR #78 generalization audit

After the preceding capabilities land, fetch the then-current #78 and compare
its base/head with the same tooling and policies. The inspected snapshot is base
`0dbc798f`, head `cd529268`; refresh these identities before the final audit.
Use isolated worktrees and fresh outputs. Do not tune rules around its addresses,
symbol spellings, hero counts, town dimensions or rendering sizes.

Metadata refreshed on 2026-09-23 now reports head `6e854110`, with the same
`0dbc798f` base. This is a navigation update, not a completed before/after audit;
refresh again when that audit starts.

Produce `docs/tooling/pr78-data-contract-audit.md` with a case-by-case matrix:
generic rule, before evidence, after evidence, byte/relationship coverage, and
remaining uncertainty. Cases include hero-table extent, town shared storage,
archive-selection tables, live sound/global bindings, startup initialization,
sprite decoder constants, minimap stride, CD-result overwrite, optional Host
guard and the missing out-of-line constructor. Incorporate later rendering fixes
from the user's branch without overwriting their work.

Data layout, identity and initialization cases must be exercised by the generic
data checks. Branch guards and behavioral choices also require ordinary code/CFG
comparison; do not claim a data-byte matcher alone verifies them. If a case escapes
the general checks, extend the reusable rule and its generic negative controls,
or report it as incomplete and continue. Retain measured code-match regressions
and partial verdicts rather than assuming the fixed branch is entirely exact.

## Checkpoints and completion

Each implementation PR targets the preceding branch, documents its measured
coverage gain, and runs relevant tooling tests plus an independent exported-TSV
audit. Source changes require the repository's retail/DC evidence workflow and
VC6 verification. Build/enrollment integration requires a final full `homm3 build`
with all relevant gates; preserve original raw objects and score histories.

The goal remains active through all stages. Completion requires the actual stacked
PRs, ordinary data comparison enabled with explicit enrollment, verified gains
across the listed vectors, the four cross-function checks, and a current PR #78
before/after audit. Remaining bytes may still be unidentified, but their exact
counts and unsupported cases must stay visible. A structural classification,
green narrow test, or successful launch cannot substitute for this completion audit.
