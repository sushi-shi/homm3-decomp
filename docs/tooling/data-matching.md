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
homm3 verify library-code
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

An error confined to one function body withholds that function's local data;
clean functions can still contribute their typed declarations. Header and
signature errors withhold the affected parse. Diagnostics remain in the
extraction report. Anonymous namespace names join only when VC6's source
module and complete symbol type uniquely agree with the declaration.

Pristine zlib definitions use the existing `config/retail/zlib-map.tsv` with
`kind = func` or `data`. Data sizes come from the vendor declarations under
the matching ABI. Initializers and pointer destinations are checked against
retail; data rows do not enter the function denominator.

VC6 local-static guard claims own one byte, as confirmed by the compiler's
unsigned-char guard symbols and retail byte accesses. They do not claim the
adjacent flags or other guards. Mutable source-owned character arrays are
excluded from pooled-literal pairing: identical initial text does not make a
writable buffer and a string literal the same object.

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

Exception type descriptors and catch/throw records are enrolled from the
ordinary VC6 objects too. A descriptor's full encoded type name and reviewed
`type_info` vtable identify it. Catchable-type records, arrays and ThrowInfo
then join through known referents. Every byte, pointer addend and relocation
site must agree. Unknown constructors/destructors and conflicting copies are
withheld; an identical-looking record with unresolved pointers is not a match.
Current compiler content receipts are required, and the emitted `.xdata$x`
COMDAT topology is retained in the comparison objects.

Folded exception constructors/destructors can have several source names at one
retail address. The model admits an alias only when its complete emitted body
and named references match a known retail body, and a complete type-identified
exception record points to it. Source/object receipts are checked again before
publishing the alias. Ambiguous addresses remain unresolved. These aliases
share the existing function entry and do not increase the function denominator.

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

`compiler-generated` ranges belong to an implemented source function's exception
handling. A decoded registration push and retail FuncInfo identify the parent;
each cleanup must also agree with the reviewed funclet census and its individual
size. Bytes between separate cleanup bodies remain unclaimed. Explicit source
body claims take priority, including catch handlers already inside their parent.
This attribution does not assert that the candidate cleanup bytes match.

`source-initializer-exact` ranges compare ordinary source declarations'
dynamic initializers with the retail CRT entries. Their instructions, named
call targets, relocation sites and destination stores must all agree. Typed
source declarations supply the destination extents. The checked CRT table
supplies roots; reviewed function extents exclude alignment padding. A raw
VC6 object is accepted only with a current content receipt for its source,
headers, compiler and flags. Missing or stale evidence leaves bytes unclaimed.

These results appear in `data_coverage.json` under `source_initializers`.
Repeated header copies identify a declaration but do not establish the retail
TU. The compiled witness is recorded separately; no per-copy handwritten
address ledger is needed. Conflicting storage claims remain overlaps in the
image partition and findings in that report. These generated bodies do not
inflate the independent game-function matching denominator.
`source-initializer-padding-exact` separately records the compiler's emitted
alignment bytes when retail repeats them through the next reviewed function
boundary. A zero-filled or NOP-looking gap alone is not sufficient.

`startup_initializers` extends this comparison to source-owned `DATA` objects.
References into their typed extents select possible CRT roots; the entire
emitted body must then match, including named references and field addends.
The report retains mismatching and unresolved candidates as a worklist. A
raw zero-initialized object comparison does not check its later constructor:
for example, a default-constructed artifact table can have correct BSS bytes
while its startup values are wrong.

Unpaired local cleanup functions reached by a candidate initializer must also
compare completely against reviewed retail extents. Their verified bytes are
`source-cleanup-exact`, counted once even when several initializers reference
them. Reviewed atexit callbacks use slot `-` in the existing `init-thunks.tsv`
inventory: they are excluded from game-function totals and from CRT table slots.
Unknown data references remain unresolved. Current source/compiler
receipts are checked before and after the pass; this adds no address ledger,
duplicate game bodies or new game-function score entries.

Private SDK tree statics (`_Nil` and `_Nilrefs`) are identified from ordinary
source instantiations. A current object must emit the complete four-byte BSS
COMDAT, and an independently claimed function must match every instruction,
relocation site and other named reference. Its address operands then identify
the static; all witnesses must agree, and claimed storage cannot be overwritten.
The model regenerates these names and sizes from compiler receipts. This does
not require SDK edits, replacement globals or a separate symbol ledger.

Some header statics are initialized identically by many units without any
authored storage owner: VC6's `std::ctype<unsigned short>::id` and the other
facet ids use a compiler-private one-byte COMMON guard and register an empty
destructor with `_atexit`. `shared_initializers` credits such a CRT root only
when a current raw object emits the complete body: every unrelocated byte,
relocation site, named callee and emitted local cleanup must agree. A private
COMMON guard is bound only when every matching retail copy agrees on one
zero-filled `.bss` address that no model claim owns and no other private
symbol proposes. When byte-identical bodies name different private guards
(`num_get` versus `num_put`), only the unit owning the preceding claimed
source function may decide. Identical copies identify the header definition,
not the retail unit. The bound guards are reported in `shared_initializers`
but not claimed as storage here. A header's file-static object (such as
`<iostream>`'s `_Ios_init`) binds only through the unit whose claimed code
precedes the retail root, and only its address is checked. A call to a name
the model or the reviewed runtime placements already know must reach that
address; compiler-private `$E<n>` ordinals from runtime members are never
used as names. Beyond a placed code section's own symbol, a name is known at
an address only when a byte-verified library section defines it there: a
static data member inside a placed `.bss` section (LIBCPMT `locale0.obj`'s
`ctype<char>::_Cltab`, 0x10 into the section at 0x2ab1b4) is named at its own
offset, never at the section start.

VC6 pads each function COMDAT to its section alignment with NOP bytes inside
the section's raw payload. `source-padding-exact` credits the bytes after a
claimed function's reviewed extent when the function's own current COMDAT is
exactly as long as the retail span to the next reviewed boundary and its
bytes past the retail extent agree. The `source-padding-aligned` generator
proposes a reviewed padding row (see "Reviewed padding") for non-exact
implementations: the retail body starts on the emitted section's
alignment, the next reviewed start is the very next aligned address, the
object demonstrably pads its code COMDATs with 0x90, and every gap byte is
that fill. Anonymous compiler functions are found through normalization's
stamped sidecar. Startup bodies use `source-initializer-padding-exact` in the
same way.

The `linker-padding` generator proposes the gap after a source function's
`.text$x` exception contribution. VC6 emits the cleanup funclets followed by the registration
stub in one associative section, so the retail contribution ends with the
decoded stub. LINK 6.00.8447 aligns the next contribution to 16 bytes and
fills with INT3: linking the current `winmgr`, `winfile` and `window` objects
with `/FORCE` places `0xcc` between every `.text$x` contribution, while the
compiler's own NOP fill stays inside code COMDATs. The gap must end at the
next 16-byte boundary, which must be a reviewed contribution start. Whether
the emitted cleanup bytes also agree is reported per row as `contribution`.

The same INT3 fill is proposed directly before a source contribution: a
claimed function's exclusive COMDAT or a source `.text$x` section (whose
first cleanup outside the parent begins it), when the run starts at a
reviewed extent's end and is shorter than the emitted section alignment.
A function the unit does not emit (a compiler-function claim without a
paired body) may use the unit's single `.text` COMDAT alignment for
`source-padding-aligned`.

`local_cleanups` credits `source-cleanup-exact` code that a claimed function
references without a source name: local-static destructors registered with
`_atexit` and element constructors/destructors handed to the vector
iterators. Sites pair either at identical offsets, when the parent's
emitted body has the retail length and exactly the retail relocation sites,
or as ordered `_atexit` registrations of equal count. The whole emitted body,
its relocation sites and every named or local callee must match. Reviewed
atexit callbacks use slot `-` in `init-thunks.tsv`.

`import-thunk` covers six-byte `jmp dword ptr [IAT slot]` census rows whose
slot is named by the PE import directory and whose operand relocation is
admitted. System DLL imports must come from the pinned VC6 import library;
vendor DLL thunks rest on the import directory and say so in the report.
Rows already owned by source or runtime placements are left to them.

`config/retail/code-extents.tsv` holds reviewed `.text` extents that are
neither a function nor its fill. `patch-residue` is the cut-off original body
behind a binary-patched entry: it must start at a claimed function's reviewed
end, end at the next reviewed start, and receive no admitted relocation.

The zero-filled bytes between an executable section's VirtualSize and its
FileAlignment/SectionAlignment boundary are `structural` when the section
header, the next section's start and the raw size agree. Any non-zero byte in
that tail remains missing. Initialized data-section tails are classified
with the linker structures below; the two passes never share a section.

Ownership and matching are separate. An owned range may contain incorrect bytes.
Initializer verdicts are `exact`, `mismatch`, `unresolved` or `unavailable`.
Pointer words require a known target plus the correct addend, and the relocation
site set must agree. A missing referent does not become an exact comparison by
masking its word. Counts are per enrolled owner, including folded COMDAT copies;
partition totals count physical bytes once.

Compiled vendor ranges are separate from game data; their initializer verdicts
still distinguish exact bytes from unresolved comparisons. Runtime labels without
byte proof are `library-unverified`; they are not exempted as verified library
coverage. Unknown zero bytes remain unknown: zeros alone do not prove padding.
PE headers and independently identified resource/relocation sections are structural.

`library-runtime` bytes are statically linked MSVC runtime code and data
proven by `homm3 verify library-code`. The reviewed
`config/retail/runtime-contributions.tsv` places one COFF section of a pinned
VC6 SP3 `LIBCMT.LIB` or `LIBCPMT.LIB` member (or one import-library thunk, or
one linker-allocated COMMON, or one Platform SDK GUID section of `UUID.LIB` or
`DXGUID.LIB`) per row; compiled pristine zlib data sections use
the same rows, and the zlib map places zlib code. Every run re-checks each
row: unrelocated bytes must equal retail, uninitialized sections and COMMONs
must be zero-filled `.data` at their alignment (a COMMON takes its largest
library or game declaration), and every relocation must resolve to its symbol:
a placed library section, the import slot, an absolute symbol, a game
definition, or a byte-identical game-emitted COMDAT. A COMDAT emitted by both
a game object and a library is attributed by position: a game object's copy
lies in the game part of its output section, a library copy among its
member's neighbours. Library code COMDATs that the linker folded into a game
function (`alias` rows) are verified but do not claim the game's bytes.

Runtime-map and zlib labels, and model data claims of the same name, yield to
verified sections that define them; a differently named claim inside a
verified section remains and shows as an overlap. Credited code fill is the
0xCC run directly before a verified section, shorter than its alignment and
ending on its aligned start. Data fill is zero, which alone proves nothing: it
is credited only when it runs exactly from one verified contribution's end to
the next contribution's aligned start. Rows that fail stay
`library-unverified`, and `data_coverage.json` lists them under
`library_code`. Library data that verified relocations reach but no row
places is listed there too (`homm3 verify library-code --data`).

Linker-produced structures are accounted by `homm3.verify.linker_structures`.
The import tables are bounded by the PE import format: descriptors to the null
descriptor, each lookup table and IAT to its terminator, each hint/name record
and DLL name to its NUL plus LINK's even-address pad. A structure is
`linker-import` when the pinned toolchain import libraries reproduce it (same
DLL spelling, hint and name, or the library's own ordinal binding; IAT equal to
the lookup table; every slot referenced by a reviewed relocation). Bytes the
format bounds but no pinned library reproduces are `import-structure`, with each
reason in `data_coverage.json` `linker_structures`. In the pinned image these
are the vendor DLLs (no pinned import libraries), the hints of every KERNEL32
and USER32 import (0 in retail), the DLL spelling `KeRNeL32.dll`, and the
descriptors' TimeDateStamp 0xad2b0000, none of which LINK writes; the
unreferenced string `GetSysteminfo` after the last record stays missing.

Game `.CRT$XCU` entries are the words `__initterm` walks between the verified
`___xc_a` marker and the library entries. A slot is `source-initializer-exact`
when it points to an exact initializer body; a startup body's unit must emit a
`.CRT$XCU` relocation to the matched symbol, and header copies must repeat one
declaration order block by block. The zero fill between `___xt_z` and `.data` is
proposed as linker padding: LINK aligns the `.data` group that follows the merged `.CRT`
group to its largest section alignment (a one-object VC6 link with LIBCMT shows
it). Zero tails of initialized data sections up to the FileAlignment and
SectionAlignment boundaries are `structural`.

COMMON storage is allocated by LINK at min(32, size rounded up to a power of
two); a one-object VC6 link of `char c1; char big64[64]; char mid12[12];`
shows the 32-byte cap. COMMONs referenced only by game code are `game` rows in
the runtime inventory, checked like library COMMONs; the zero fill before them
is proposed as linker padding.

Sites inside verified library sections follow their COFF relocations in
`config/retail/relocs.tsv`; `reloc-evidence.tsv` names the owning member for
each site added or rejected on that evidence.

The access report complements byte accounting with instruction-width, stride and
shortfall checks. Its register tracking is local to a basic block; pointer escapes
and accesses through unknown runtime pointers remain blind spots. A report of no
findings is not proof that every datum is fully understood.

## Retail records, pairing and padding

`homm3/verify/retail_records.py` adds claims whose extents the retail records
state themselves. Every pointer word inside such a record must be an admitted
relocation site; nothing is sized by the distance to a label.

- `compiler-metadata`: exception frames rooted by a registration stub
  (`mov eax, FuncInfo; jmp ___CxxFrameHandler`) in game code, sized by the
  FuncInfo's own state/try/catch counts; throw records rooted by a game code
  word; type descriptors reached from those records or referenced by game
  code; x87 constants that at least one game function loads with one operand
  width (named with VC6's `__real@` spelling of the value). Records owned by
  runtime-library code belong to `homm3 verify library-code`.
- `structural`: the IAT slots bounded by the import directory.
- `game`: DATA_COMPGEN string literals whose bytes and NUL equal retail at
  the annotated address, and C strings that retail pushes (`push imm32`) or
  stores in an initialized pointer word.
- `alignment-padding` (a generator): a zero gap is proposed as padding only when (a) the following
  claim is a whole contribution whose COFF alignment places it exactly at the
  aligned end of a preceding whole contribution, or (b) two members of one
  candidate ordinary section sit at the candidate's own distance in retail and
  the candidate emitted zeros between them.

The data manifest places one-symbol game COMDAT data (x87 constants, template
statics, virtual-base tables) and ambiguous literals from corroborated code
pairings: a claimed function's DIR32 sites pair with retail's up to the first
offset divergence, each wanted site must sit behind the same instruction bytes
and at least one referent the model already knows must agree. Constructor
immediate stores pair in order as well. Source data pointers place literals
exactly; an ambiguous literal enrolls only when these votes name one address.

`homm3/verify/eh_records.py` compares enrolled FuncInfo/unwind maps with the
candidate `.xdata$x` record reached through the function's associative
`.text$x` stub. Self references must name the same offset; unwind actions must
name the census funclets of that parent in order. Records of functions whose
bodies still differ are reported non-comparable, not mismatching.

Raw initializer comparisons also resolve a pointer through identical code
folding (the object's own definition of the named symbol reproduces retail at
that address) and through identity-preserving spellings that differ only by
the compiling path (a type's anonymous-namespace hash, VC6's `_name` form for
statics in an anonymous namespace, a reference's cv letter, a guard's `$S`
counter symbol in its owner's scope). A VC6 `?%<path><nonce>@` scope listed in
`config/retail/anon-ns-paths.tsv` is compared under its retail spelling, for
vtables and exception records as well. A word naming a vendor DLL's decorated
export resolves to that import's verified thunk.

Vtable identities come from the `class` column of `config/retail/vtables.tsv`.
A row names `??_7<class>@@6B@`, or spells a complete `??_S` symbol: the local
vftable VC6 emits and stores after calling a dllimport class's imported
constructor (IFC's `CImmMouse` and `CImmEnclosure`).

## Verified game bytes

Ownership alone is not proof. `homm3.verify.game_bytes` replaces every
`game` byte of both partitions with a verdict. The invariant is that a
retail data byte is exact only if a compiled candidate definition emits that
byte at that address. Retail-side extents, labels, slot sizes and zero-ness
never cover a byte on their own.

| Category | Meaning |
| --- | --- |
| `game-code-exact` | The claimed function's current compiled bytes equal retail (CUR 100% in the objdiff report under the strict relocation policy). |
| `game-code-unverified` | Any other claimed game function byte. The reason is `mismatch:<CUR>%` or `no-comparison`. |
| `game-data-exact` | The claim's own candidate definition emits this byte at this address with the retail value. A `DATA_COMPGEN` string literal whose source text and NUL equal retail also counts. |
| `game-bss-exact` | Uninitialized storage whose candidate definition has the retail size and is zero in the image, at a compatible address. The address is compatible when it is a multiple of the power of two dividing the member's size, its section offset and its section alignment, or of the declared type's alignment from the layout oracle. Verified game COMMONs also count. |
| `game-data-mismatch` | A byte the candidate emits with a different value, reported byte by byte. A pointer word naming another referent or addend counts. |
| `game-data-unverified` | Every other game-claimed data byte, with a reason. |
| `padding` | Fill named by a reviewed padding row; its content is not compared. |

Every raw initializer comparison now checks its extent against the
candidate definition. The definition runs to the next datum of its section,
or to the section end.
- `beyond-candidate-definition`: the claim is longer than the definition.
  Only the definition's bytes are compared, and the rest are unverified.
- `candidate-larger-than-extent`: the definition is longer than the claim,
  and the remainder is not member padding. Member padding is zero, shorter
  than the section alignment, and never occurs in a single-datum COMDAT.

The retail access audit (`homm3 verify data-access`) adds disputes: a
shortfall, undercount, adjacency or stride finding on a claim makes the
whole claim unverified. Retail touches past a claim's end into padding
re-attribute that padding to the claim as unverified.

Other `game-data-unverified` reasons:
- `unavailable:<reason>` and `unresolved-pointer`: a comparison that could
  not be completed.
- `no-comparison`: no comparison exists for the claim.
- `label-only-extent`: a vtable census extent without a definition.
- `reference-only-extent`: a string sized by a retail reference alone.
- `bss-misaligned` and `bss-alignment-unknown`.
- `size-from-slot`: a byte array whose literal bound only fills its retail
  slot. The zero tail past its content stays unverified until a type or a
  consumer proves the count; `game_verification.size_from_slot` lists these
  claims.

The finish line is zero `missing`, `game-data-unverified` and
`game-data-mismatch` bytes, except post-link
residue recorded in `docs/todos/import-table-post-link-edits.md`. Each such
run is written to `build/gen/data_worklist.{json,tsv}`, sorted by address,
with these fields:
- the retail bytes;
- the owning claim and the claims on both sides, each with its annotation
  file:line, declared type, sizeof and retail extent;
- mismatching bytes as retail versus candidate, with the field path when
  the layout oracle knows the type, and both pointer targets;
- a diagnosis.

List the worklist by TU:

```sh
homm3 verify data-worklist --unit kbwin
homm3 verify data-worklist --category game-data-mismatch
```

## Reviewed padding

Coverage no longer infers padding. A gap counts as `padding` only when a
reviewed row names it: data sections in `config/retail/data-extents.tsv`,
`.text` in `config/retail/code-extents.tsv`, both with category `padding`.
Padding bytes are not compared with any candidate. A gap without a row stays
`missing`. Byte-compared fill (`source-padding-exact`,
`source-initializer-padding-exact`) remains a real comparison and needs no
row. Section alignment tails stay `structural`: the section headers bound
them.

Every row records:
- `section` and the observed `fill`: `00`, `CC`, `90` or `mixed`, and only
  those values;
- the `preceding` and `following` claims as name | file:line | type | sizeof;
- the `alignment` that explains the gap: the generator's link alignment, or
  the smallest power of two that places the following start and exceeds the
  gap;
- the generator `evidence`;
- a `proof` class:
  - `proven-end`: the preceding size is fixed by pinned bytes or a format.
    That covers library and vendor sections, import records, code, EH and
    RTTI metadata, FP constants, guards and COMMONs, and a declared type that
    is not a byte array.
  - `after-byte-array`, `after-literal`, `after-unknown-type` and
    `after-unverified`: a longer retail datum would end in the same fill, so
    the row is a reviewed risk.

Coverage validates every row. A row must lie in its section and in the right
file, carry a proof class and evidence, and its bytes must be the recorded
fill. A row that overlaps any other claim (only section topology and PE
structure may lie beneath it) is an error.

The former inference passes (link alignment, compiler member padding, LINK
fill before contributions and verified library sections, NOP fill of
non-exact bodies) are generators only:

```sh
homm3 verify padding                     # rows per proof class
homm3 verify padding --proof after-byte-array
homm3 verify padding --propose           # build/gen/padding_proposals.tsv
homm3 verify padding --propose --write   # append proposals for review
```

Every full build rewrites the proposal file with the gaps the generators
would still credit; `data_coverage.json["padding"]` holds the proposals and
the reviewed rows per proof class.

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
