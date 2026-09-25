# Full-TU Mac build and executable inventory plan

## Goal and rule

Compile every shared game translation unit from its canonical `src/` file with
CodeWarrior, under Ninja, into one Mac object per TU. The Windows and Mac
objects use the same authored function bodies, helpers, globals, and source
order. Platform differences belong in those sources or their canonical
headers, with evidence at the affected declaration or operation. A generated
file containing selected function bodies is not a Mac compilation unit.

Track three independent questions for every function: did the full Mac TU
compile and emit it, where (if anywhere) is its counterpart in the pinned PEF,
and do the candidate bytes match that counterpart? An absent address is a
coverage state, not a zero score. The final inventory must also account for
Mac-only code, libraries, compiler-generated code, glue, and padding.

The present Mac path extracts selected bodies in `mac.source.candidate_source`
and builds them after Ninja has finished the Windows objects. Retire that path
as a source of match verdicts after full-TU controls reproduce its existing
exact pairs. Keep its reports as migration references until then.

## 1. Source address contract and parity index

Add `MAC_ADDRESS(offset, size)` to `include/va.h`. It expands to nothing in
VC6 and CodeWarrior; the analysis arm may annotate it. The values are the start
and extent relative to the pinned PEF's single code section (section 0), never
a Windows VA, a file offset, or a loaded address. Every function body lives in
that section, so the annotation carries no section index; tables that also
hold data spans keep an explicit section column. Put it on the canonical
definition beside `VA(...)` where both targets retain a body. A source helper
with no Windows VA can still carry `MAC_ADDRESS`; a function that exists only
as Mac inline expansions cannot.
Use a distinct `MAC_COMPGEN_ADDRESS` claim for a proved compiler-generated
body rather than attaching an address to a fictional C++ definition.

`MAC_ADDRESS` claims functions only. Globals, constants, string literals and
other data do not receive Mac source annotations in this phase; the reviewed
Mac data inventories (`config/mac/data*.toml`) keep owning those bindings, and
the source scanner rejects a `MAC_ADDRESS` that does not precede a function
definition or declaration.

Extend the existing source-claim extraction so one canonical definition owns
both address annotations and its source name. Generate a parity index keyed
by source identity and Windows VA where one exists. Each entry records Mac
address and size or one explicit disposition: `unlocated`, `inlined_only`,
`mac_only`, `windows_only`, `folded`, or `platform_rewritten`. A disposition
needs evidence; the index must not silently treat missing annotations as
absence in the Mac binary. Overloads and compiler-generated bodies use their
canonical symbol/owner identity, not a name-only join.

Build `homm3 mac parity` to validate unique source ownership, PEF bounds,
alignment, nonoverlapping spans, target-byte hashes, and agreement with every
existing reviewed pair/reference. It reports the entire source inventory,
including unannotated functions, grouped by TU and disposition. Migrate the
existing reviewed Mac pairs into source annotations only after these checks
pass; do not infer new addresses from matching names or source order.

**Gate:** every source-owned game function appears exactly once in parity
output; existing reviewed addresses survive without changed target bytes;
unknown counterparts remain visible. This gate measures accounting, not a
claim that every Mac address has already been found.

**Status (2026-09-25):** implemented. `include/va.h` defines `MAC_ADDRESS`
and `MAC_COMPGEN_ADDRESS`; `scripts/homm3/mac/addresses.py` scans them,
`homm3 mac migrate` copied all reviewed pairs, references, helpers and
compiler-generated rows into source (4011 claims) and wrote
`config/mac/functions.tsv` (4211 spans), `runtime-map.tsv` (200 labels),
`runtime-aliases.tsv` (9 folded names) and an empty `dispositions.tsv`.
`homm3 mac parity` reports 6563 source functions: 4011 located and 2552
unlocated, with no ownership, bounds, overlap, hash or agreement defects.
The reviewed TOML inventories stay as the legacy build input while workers
still add rows; rerun `homm3 mac migrate` after merging them, and retire the
TOML readers with the extracted-body path in step 3 of the rollout. Same-line
annotations are excluded from the Windows ratchet fingerprint, so existing
MAX rows keep their source hashes.

## 2. Full CodeWarrior objects in Ninja

Add a CodeWarrior compile rule to `homm3.build.configure`, keyed by the same
`config/units.toml` unit/source entry as VC6. Use a Mac profile only for flags,
include roots, and evidenced platform settings. It must not list selected
function bodies. Produce `build/mac/obj/<unit>.o` from the actual source file,
and capture its complete project-header dependency closure for incremental
Ninja rebuilds. A changed source or header rebuilds the Mac object. Keep the
Windows and Mac object directories and Wine prefixes separate.

Bring canonical headers and sources through CodeWarrior one full TU at a time.
Resolve target ABI and OS differences in shared declarations and explicit
platform branches; do not duplicate game bodies in `config/mac/include/` or
splice selected definitions into a generated TU. A platform-only TU receives
an explicit manifest disposition. Compile failures are reported per TU with
their real diagnostics, not concealed by leaving that TU out of the graph.

During migration, `ninja mac-objects` builds all declared shared game TUs and
`ninja mac:<unit>` builds one. Once the shared-TU coverage gate passes, include
Mac objects in the normal `all` graph; `homm3 build --fast <TU>` then builds
both objects, and the full build checks all of them. The object extractor
indexes every emitted MWOB function/data hunk in each complete object,
including ordinary and inline helpers that CodeWarrior chose to retain.

**Gate:** every shared game TU has a current full-TU Mac object or a reviewed
platform-only disposition. The report lists authored definitions not emitted,
emitted symbols without source owners, and all compile errors. Existing exact
Mac controls still compare exactly when read from the full-TU objects.

**Status (2026-09-25):** started. `build.ninja` has an `mwcc` rule
(`homm3.mac.cc_wrap`) producing `build/mac/obj/<unit>.o` from every
`config/units.toml` source, with `ninja mac-objects` / `ninja mac:<unit>`
outside `all`. The rule adds only CodeWarrior flags, `-prefix
include/compiler.h` and the project/MSL include roots; it writes the full
diagnostics, the `MWLinkPPC -dis` listing, an index of every emitted code and
data hunk, and a header-closure depfile. `homm3 mac objects` reports per-TU
state: 99 of 152 TUs compile (6968 code hunks) and 53 fail with their first
real error in `build/mac/objects.tsv` (DirectDraw/Windows SDK types,
`__declspec` operator declarations, CRT functions without a declaration in
scope, overload ambiguities). No TU has a platform-only disposition yet
(`config/mac/tu-dispositions.tsv` is read when present). `homm3 mac tu-compare
<unit>` is the migration control: hero currently reproduces 2 of its 16
admitted pairs exactly from the full-TU object; its data-binding model still
assumes the selected-body extern shims, and full-TU std instantiations need
runtime identities.

`homm3 mac emitted` joins every full-TU hunk to authored definitions by
CodeWarrior qualified name: of 6968 emitted code hunks, 2017 belong to the
unit's own source, 354 to project headers, 4249 are MSL library templates,
195 compiler-generated members or static initializers, 95 vendored zlib,
9 another source file and 49 have no source owner. 33 non-template
definitions authored in a compiled unit have no emitted body. Reports are in
`build/gen/mac/{emitted-symbols,not-emitted}.tsv`.
`config/mac/tu-dispositions.tsv` exists but is empty: a unit's missing string
literals do not discriminate (shared units such as font and palette have none
in the PEF either), so no unit is yet evidenced as platform-only.

## 3. Pair the source and emitted object with the PEF

Join the parity index to full-TU emitted symbols and to verified PEF spans.
For each `MAC_ADDRESS`, require an emitted body or an evidenced folding/
inlining exception; verify the code-section extent, hash, call destinations,
and data references. Reuse the current linker/relocation comparison where it
is sound, but take the candidate body from the full-TU object. Never mask an
unresolved reference into an exact verdict.

Populate Mac annotations from positive identity and boundary evidence:
CodeWarrior symbol and ABI, calls, loader pointers, vtables, constants,
strings, neighboring bodies, and target bytes. The stripped PEF does not
provide a trustworthy name-to-address table. Inline expansions are attributed
to callers; only a retained out-of-line helper receives its own Mac address.

**Gate:** all annotated functions have valid target spans and current object
comparisons. Parity output reports every unlocated function and every emitted
helper without a target identity. Exact-match percentage is reported only
over verified spans, alongside source/TU coverage denominators.

## 4. Library and generated-code maps

Add `config/mac/runtime-map.tsv` and separate maps where provenance matters
(for example, CodeWarrior MSL versus a vendored library), analogous to the
Windows runtime and zlib maps. Each row records code-section offset, size,
symbol or stable working label, library/archive owner if known, target hash,
and evidence. Library data such as MSL tables stays in the reviewed data
inventory with its section index. Imports and transition-vector/glue stubs have explicit categories;
they are not scored as authored game functions. Do not put library addresses
in source `MAC_ADDRESS` annotations or alter `vendor/`.

Move the reviewed entries in `config/mac/runtime.toml` into the TSV inventory
with identical spans and hashes before deleting the old reader. Validate
unique starts, nonoverlap, PEF bounds, and exact target bytes. The library
map may retain unresolved names, but not unbounded or guessed spans.

**Gate:** every known library and glue span is represented once, with its
provenance; no library row is counted in the game exact-match denominator.

**Status (2026-09-25):** implemented. `homm3.mac.tables` owns the maps.
`runtime-map.tsv` carries owner (`msl_c`, `msl_cxx`, `cw_runtime`,
`compiler_generated`, or empty when unknown), call kind and evidence; all 209
`runtime.toml` function rows moved with identical spans and call resolution,
and `runtime.toml` keeps only library data. `glue-map.tsv` admits the 362
loader-proven CFM import stubs and `zlib-map.tsv` the vendored zlib bodies.
Owners come from positive evidence only; 96 working labels stay unknown.

## 5. Complete pinned-PEF function and byte inventory

Create `config/mac/functions.tsv` as the hand-admitted, executable-wide Mac
counterpart to `config/retail/functions.tsv`. One row describes each verified
function span by code-section offset, size, category, owner/source identity or
library label, and evidence reference. Include Mac-only and compiler-generated
bodies. The target digest and PEF section bounds are fixed inputs. Generate
readable consolidated views from the source annotations, library TSVs, and
this inventory; do not hand-maintain another copy of function names.

Seed a separate **candidate** census from loader pointers, branch targets,
vtable entries, emitted object symbols, and reviewed spans. Branch targets
are leads, not automatically functions. Admit each boundary only with positive
evidence, including multi-return bodies and code islands. Account for all
code-section bytes as function, owned switch/exception data, linker glue,
padding, or unresolved region. Unresolved regions stay explicit in the
coverage report until classified; they are never silently omitted from a
"complete" denominator.

**Gate:** zero overlapping or out-of-bounds spans; every code-section byte
has one reviewed category; every function row has a source, library, generated,
Mac-only, or unresolved owner; every source address claim resolves to one
function row. Completion requires zero unresolved function/owner rows, while
the intermediate reports show their counts without inventing mappings.

**Status (2026-09-25):** accounting implemented; completion open.
`homm3 mac inventory` places every code-section byte in one region
(`build/gen/mac/code-regions.tsv`): 74.46% source-owned functions, 0.98%
runtime, 0.30% import glue, 0.37% vendored zlib, 15.51% verified functions
without identity (2285 rows), 2.99% the reviewed read-only pool
(`config/mac/code-regions.tsv`) and 5.40% unresolved in 118 gaps.

Entries are transition-vector targets and direct call targets; other loader
pointers into code are jump-table labels. A span is admitted
(`--admit-proven`) only when it starts at an entry, ends in blr or an
unconditional branch at the next entry or verified row, and every word is
reachable from its entry and in-span jump-table labels. Against the 4211
reviewed spans the splitter reproduces 4146 exactly and none wrongly; merged
adjacent reviewed pairs never pass. It admitted 2335 spans. The gate also
rejects rows containing another function's entry: it found 15 over-extended
reviewed spans, 13 of which were corrected with the proof; two runtime rows
whose rethrow landing pads the proof cannot reach remain reported defects.

Full-TU hunks whose relocation-masked bytes occur exactly once in all code,
filling an identity-less row, are identity leads (`object-leads.tsv`,
`identity-leads.tsv`). `homm3 mac emitted --admit-library` labelled 19 MSL
template bodies (plus one folded alias) and 31 zlib bodies this way; game
bodies stay leads for reviewed MAC_ADDRESS claims. The gate is incomplete
until the unresolved gaps, the unowned rows and the two defects are resolved.

## Rollout and retirement

1. Implement the macro and parity reader with tests for duplicate claims,
   wrong code-section extent, missing source owner, and inline-only dispositions.
   Reconcile all existing reviewed Mac pairs/references with that index.
2. Add the Ninja rule and compile a full representative TU, including its
   globals and helpers. Use the current exact hero controls as a byte-level
   migration check, then exercise a TU with exceptions, data, and virtual
   calls. Expand to every shared game TU; do not declare the build complete
   while any is omitted without disposition.
3. Switch Mac comparison to full-TU objects, retain score provenance, and
   remove the extracted-body build path and duplicate declaration-only game
   views after its controls pass.
4. Populate the library TSVs and executable-wide function inventory. Keep
   source correspondence, library coverage, Mac-only code, unresolved spans,
   and byte-match scores as separate dashboard numbers.

The first useful checkpoint is a Ninja build of every shared game TU plus a
parity report covering every authored function, even while many Mac target
addresses are still unresolved. That establishes the build and coverage
foundation before further per-function matching work.
