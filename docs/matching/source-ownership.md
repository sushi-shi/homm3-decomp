# Function source ownership

The ownership gate scans active C++ definitions and checks their retail claims.
Clang supplies declaration identities, physical locations, signatures and
mangled names. VC6 SP3 under Wine and the pinned Windows executable decide
code-generation accuracy. A retained COMDAT's link location does not establish
where its canonical source body belongs.

```sh
homm3 source-ownership
homm3 source-ownership --json
homm3 source-ownership --fresh
```

Full builds run this gate after compilation and claim extraction. The editor
worktree no longer reads Dreamcast binaries, source exclusions or a debug-port
procedure census. Historical evidence remains in source comments; passing this
gate does not independently prove historical source ownership, complete source
coverage, or an editor identity for a shared game function.

## Active definitions and coverage

Collection covers admitted project translation units, their active project
headers, and standalone project headers outside that include closure. Every
C/C++ source file must be admitted in `config/units.toml`; files outside it fail
coverage, including empty files. Collection and parse errors are fatal, never
empty successful inventories.

The scanner uses the unit's compiler profile, undefines `__clang__` and sets
`_MSC_VER=1200` so definitions guarded for VC6 remain visible. The private
`HOMM3_SOURCE_OWNERSHIP` define keeps VA metadata available in `va.h`.
Inactive code and editor-tooling alternatives are excluded. Clang skips body
semantics; the scanner checks the balanced body following each declaration.

An explicit `@stub` placeholder comment inside an active definition is fatal
(`ACTIVE-STUB`), including unannotated helpers and header bodies. String literals
containing that marker and real empty constructors are not placeholders.
Unreconstructed reference bodies can remain inactive under `#if 0`.

## Canonical claims

Source annotations own names. Put a written function's `VA` on its canonical
body, including predicate `operator()` bodies and header inlines. A copied
specialization, inactive .cpp stub or generated enrollment cannot replace that
annotation. `CLAIM_OWNER` checks known header bindings against both extracted
claims and the banked ledger, so removing the annotation cannot erase its own
evidence. `CLAIM_IDENTITY` checks every annotated active definition's exact
mangled identity against extracted claims at that address.

Header annotations are projected onto VC6-emitted symbols. The AST supplies the
mangled name even when LLVM IR omits inline annotation metadata. A banked RVA
prefers its existing comparison carrier if it still emits the symbol. If it
stops emitting and no unique replacement exists, an active annotated body keeps
its banked carrier and identity; comparison records the missing output without
losing its MAX/history. An inactive or absent body cannot use that fallback.
New claims require a unique emitter or an unambiguous retail neighbour among
actual emitters. No separate header-symbol ledger is maintained.

The same fallback applies to an active source-local inline only when its own TU
and RVA already have a baseline entry. It cannot admit a new claim or excuse an
ordinary function missing from the compiled object. Anonymous namespace members
retain their private scope: Clang/VC6 filename-spelling reconciliation requires
the same owning module, entire signature and a unique candidate.

For retained members of a class template, attach concrete selectors to the
canonical generic definition:

```cpp
template<size_t N>
// VA instance: bitset_iterator<144>::operator*
VA(0x0048eb40, 0x14)
// VA instance: bitset_iterator<145>::operator*
VA(0x004d4ca0, 0x14)
typename std::bitset<N>::reference bitset_iterator<N>::operator*() const
{
    return (*m_bits)[m_position];
}
```

The scanner resolves selectors through an unsaved AST probe. Each selected
member must point back to that physical generic definition; a sibling member or
copied specialization cannot steal the claim. Each VA needs one unambiguous
selector. The probe never enters the compiled source.

Retained library and implicit bodies use `VA_COMPGEN` with the canonical type
and operation identity. Handwritten container copies remain project definitions;
restore the supported library type and operations instead of relabeling a copy.
For example, `std::vector<combatManager::TObstacle>` claims name `TObstacle`,
while `MAP_FIND`/`MAP_INSERT` identify public map operations and
`TREE_FIND`/`TREE_LOWER_BOUND` identify underlying tree methods.

Unpaired library or implicit enrollments are reported separately as code-emission
debt. A body may stop emitting after callers change; do not force artificial
instantiation to satisfy the ownership check. The retail enrollment and history
remain intact. Written bodies still need their direct annotations and matching
identities. Anonymous initialization thunks deliberately retain synthetic names.

## Reviewed source-local classes

The cleanliness gate admits only `(file, class)` pairs reviewed in
`config/cleanliness/local_classes.tsv`. The initial table preserves the local
classes established before debug-port tooling was retired. A name is exempt
only when each physical definition is reviewed and each file contains exactly
one definition of it. Duplicates, headers and unreviewed .cpp copies remain
violations. Direct member enums are covered by the local-class exception;
method-local and file-scope enums are not.

A source scan does not refresh objects or extracted claims. After moving or
renaming definitions, run the full build so the gate sees current compiler
output. A clean scan alone does not certify compilation, byte matching or
complete source recovery.
