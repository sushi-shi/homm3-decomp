# Function source ownership

Dreamcast CodeView's procedure source file and line identify the original
location of a written function definition. Compiler-generated special members
are distinguished by the type record's `CV_fldattr_t.compgenx` bit, whose
[format is defined by Microsoft](https://github.com/microsoft/microsoft-pdb/blob/master/include/cvinfo.h).
Their procedure lines are not treated as written source bodies. An explicit
Windows definition replacing such a member remains a fatal `GENERATED` case
until implicit generation is restored or the exact Windows-specific definition
is reviewed in `win_only.tsv`. The CodeView compiland identifies a place
where that definition was emitted. For header and inline functions, neither
Dreamcast object offsets nor retained Windows RVAs replace source line order.

The source-ownership audit uses libclang only to enumerate active definitions,
physical source locations, qualified names and signatures. It includes methods
inside classes and inline helpers without VA annotations. VC6 SP3 under Wine
and the pinned Complete executable remain the authorities for x86 code.

Hand-written copies of standard-library containers are project definitions
and receive the same ownership checks. Restore the proven library type and
its real operations instead of exempting the copied methods. Retained library
bodies use `VA_COMPGEN` with the canonical template element identity; the
label join must not disguise a project class as an STL specialization. For
example, the obstacle container is `std::vector<combatManager::TObstacle>`;
its `_Ucopy`, `_Ufill` and `size` claims name `TObstacle`.
The resource cache likewise uses its native `std::map`: `MAP_FIND` and
`MAP_INSERT` name the public map operations, while `TREE_FIND` and
`TREE_LOWER_BOUND` identify the separate underlying tree methods. Named
class keys, such as `TCacheMapKey`, remain distinct from string keys.

Written predicate `operator()` bodies own `VA` annotations directly, like
other written functions. The label scanner rejects the retired `FUNCTOR_CALL`
enrollment and binds direct claims by the operator's qualified owner. This
does not change the separate `VA_COMPGEN` claims for retained library sort
specializations that call or expand those predicates.

Run the audit with:

```
homm3 source-ownership
homm3 source-ownership --json
```

`config/dc_only.tsv` excludes exact CodeView identities (source file, function,
source line) with a documented platform/pressing reason. An unreconstructed
function is not automatically DC-only. `config/win_only.tsv` admits exact
Windows definition identities (physical file, qualified function, signature)
without a Dreamcast counterpart. It cannot waive a misplaced DC function.
Unused entries and duplicate or incomplete entries are errors.

Ordinary definitions retain retail RVA progression in their owning source
modules, enforced by the VA gate. They do not advance the separate CodeView
source-order stream: the older DC build can order retained functions differently
(for example, townObject's constructor and CTownNetMsgHandler::HandleGiftMsg).
Header bodies, explicit inlines and helpers without a retail claim follow
CodeView line order. The VA gate obtains inline definitions from the AST, so a source-local
class body also follows source order independently of its COMDAT address.
Header definitions retain their CodeView owner and source order; a
VA annotation belongs on the canonical header body, even when its retained
out-of-line copy was emitted by another translation unit. An unannotated
inline has the same ownership obligation.

The audit runs as a fatal gate in full builds. Its preprocessor undefines
`__clang__` and sets `_MSC_VER=1200` so project definitions guarded for VC6
remain visible; the private
`HOMM3_SOURCE_OWNERSHIP` analysis define keeps VA metadata available in `va.h`.
Editor-only alternatives are excluded, and ordinary inactive carcasses stay
inactive. A compiler guard cannot exempt a written constructor, destructor or
template specialization from ownership review.
Clang skips body semantics so
its handling of VC6 local-variable lookup cannot change the source inventory;
it supplies active declaration locations, and the scanner checks the following
balanced body. Parse errors remain fatal. Tests cover constructors with member
initializers, unannotated templates, inactive stubs and header annotations.

Header annotations are projected onto VC6-emitted symbols. The AST supplies the
annotated mangled name even when LLVM IR omits inline annotation metadata. An
existing banked RVA prefers its comparison carrier if that object still
emits the symbol. If it stops emitting and no unique replacement can be chosen,
the active annotated body still supplies
the name and the banked carrier is retained: comparison records missing output
without losing the retail claim or its MAX/history. An absent or inactive body
cannot use this fallback. New claims require a unique emitter or an unambiguous retail
neighbour among actual emitters. The header owns the name and definition; the comparison carrier does
not establish historical source ownership. Header bodies also participate in
per-function source hashing. The gate also checks every annotated active definition's exact mangled identity
against extracted claims, including source-local inline bodies whose annotations
LLVM IR omits. For a header body, a VA left on an inactive .cpp stub or a
compiler-emission enrollment cannot substitute for annotating that body.
No separate header-symbol ledger is maintained.
The same fallback applies to an active source-local inline body only when its
own TU and RVA already have a baseline entry. It cannot admit new claims or
excuse an ordinary function missing from the compiled object. Anonymous
namespace members retain their private scope: the join accepts VC6's embedded
first-declaration filename (source or owner header) in place of Clang's
namespace hash only when the owning module
and entire member signature agree and the candidate is unique.
For a retained member of a class template, attach a concrete selector to the
canonical generic definition instead of copying a specialization into its
comparison TU:

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

The scanner resolves the selector in an unsaved libclang name probe; it never
writes or compiles that probe. The selected member must resolve to the exact
physical token of the annotated generic declaration. A different overload,
sibling member, or explicit specialization cannot borrow the annotation.
Each selector immediately precedes its own VA annotation. Several such pairs
may share the same generic body: ownership counts it once, while projection,
identity checking and carrier selection handle every concrete claim. Duplicate
addresses/selectors and unpaired comments are fatal. Current selectors support unambiguous ordinary
members, `operator*`, `operator()` and destructors of class templates; unresolved
or malformed selectors are fatal. The header body owns source hashing and
CodeView placement, while the selected concrete name identifies the comparison
instance. This uses the existing header-carrier rules and no generated-function
enrollment for a handwritten body.
Renamed definitions retain their identity in an attached source comment of the
form `// Original: Class::Name; Owner.h:123, dc 0x1234.`. The gate reads this
explicit procedure bridge and still checks its owning file and source order.
Incidental mentions in other comments do not establish an identity.
Two physical definitions cannot bind the same unambiguous written CodeView
function. This is a fatal `DUPLICATE`, including when a same-arity adapter
overload borrows another helper's identity. Repeated DC emissions count as one
source identity; distinct formal overloads retain separate identities.
The documented naming pass changes ordinary operation case and underscores.
The audit compares those spellings while retaining containing-type identity,
formal signatures, owner checks, duplicate checks and source-order checks.
Colliding normalized operations still require an unambiguous source identity.
Variadic signatures retain the ellipsis separately from the fixed parameter
count. CodeView's trailing `T_NOTYPE` argument marker and Clang's function
variadic flag must agree; an ellipsis inside a callback parameter does not
make its enclosing function variadic.
The inventory also reads named methods from full CodeView class field lists,
including overload lists. A method without a procedure record remains a
declaration-only identity: the field list proves its signature and generated
status, but supplies no body file or line. Such a written counterpart is a
fatal `UNLOCATED` finding until its body ownership/order is recovered. A missing
procedure alone cannot justify a Windows-only exemption or borrowing another
overload's procedure line.

Positive foreign-header line rows inside a caller can recover an inline body
even when it has no standalone procedure. Attach a reviewed binding immediately
above that definition, with an evidence comment explaining the instructions:

```cpp
// DC DrawCursorAlpha expands the Bitmap16Bit overload here, loading its
// map/width/height/pitch before calling the raw DrawHeroAlpha member.
// @dc-inline-origin: 0x17e5 0x7a1e8
void drawHeroAlpha(/* the declared bitmap-overload parameters */) const { /* ... */ }
```

The first value is the exact CodeView field-list function type; the second is
the inline source-row address. The gate requires that type to name the same
unlocated, non-generated member, and the address to have one foreign-header
source row strictly inside a known caller from that module. It recovers the
file and line from the pinned symbols, then applies the usual signature, owner,
order and duplicate-body checks. Different statement addresses cannot authorize
two bodies for one declaration. This annotation records a semantic review;
the line table alone does not name the inlinee. It cannot override an existing
procedure origin or turn an ordinary out-of-line definition into an inline.

A reviewed Windows return-interface change is also distinct from an unlocated
implementation of the same declaration. An exact `win_only.tsv` row can admit
that change only when both parsed return types are known and different, and
the DC counterpart has no recovered body. For example, Complete's
`checkSetMouseDirection` returns a pointer-change byte consumed by its caller,
whereas DC only declares a `void` method. The same or an unknown return type
still rejects the exemption; an emitted or recovered DC body still needs its
proper source owner.
For example, the `TAutoArrayPtr<char>` and `TResourcePtr<TTextResource>` copy
constructors have distinct ordinary field-list declarations, while only their
pointer constructors have procedure records. Their unused, guessed transfer
bodies have been removed; the proven copy declarations remain. TCheatCode's
unused default constructor and GetCode likewise remain declaration-only until
body evidence exists. This does not waive the UNLOCATED check for an active
implementation or authorize dropping a definition required by a caller.
Field-list declaration sequence
does not establish definition order. Template type-family comparison preserves
the containing type, pointers/references and qualifiers when selecting these
identities; repeated procedure emissions still do not permit duplicate bodies.
Unpaired library or implicit `VA_COMPGEN` enrollments are reported separately
as code-emission debt. Their bodies can stop emitting when callers change;
ownership does not require artificial instantiation or a particular inlining
decision. The retail enrollment and its comparison history remain intact.
This does not excuse a written body: its direct annotation, ownership and
mangled identity are still checked. Anonymous initialization thunks retain
their deliberate synthetic names.

The older cleanliness metric admits a unique source-local class when
its CodeView methods originate in that .cpp and have no competing header origin.
For Complete-only classes, the exact member identities in `win_only.tsv` supply
the reviewed source owner instead. This cannot override a CodeView owner and
does not admit duplicate definitions or a class in another file. There is no
second class-exemption ledger.
Duplicate or unproven local views remain violations. This permits restoring
original local classes without raising a blanket cleanliness allowance.
Direct classes in a global anonymous namespace are private to their source
module. CodeView can therefore prove separate copies of the same class name
in several .cpp files. Each copy must have a matching source-module origin,
the anonymous scope, and a single physical definition in that file; a header,
unproven module, different namespace or duplicate remains a violation.
Member enums stay inside those proven local classes. This exception applies
only at direct class scope; method-local enums, unproven nested classes and
file-scope enums remain subject to the cleanliness gate.

Collection covers admitted game translation units, their active project headers,
and standalone project headers outside that include closure. Source files outside
the manifest may hold empty DC_ONLY reference carcasses; implementation code
there fails coverage. Formal CodeView argument types and member constness resolve
overloads independently of optimized debug parameter records. Const conversion
operators retain their member qualifier even though Clang reports them as
conversion-function cursors rather than ordinary method cursors.
An unmatched formal argument count or member constness is a fatal `SIGNATURE`
case, rather than borrowing another overload's source location. A genuine
platform signature change or new Windows-only overload needs its exact filter
entry with retail evidence. A source annotation may identify the related
CodeView procedure, but it cannot waive the arity or constness check. Matching
renamed definitions still use those annotations to recover ownership.

The ownership inventory has no accepted backlog. Every active definition must
resolve to its canonical CodeView source identity or an exact, reviewed Windows
filter entry. Platform filters include retained Windows APIs and documented
Complete additions; they are not substitutes for recovering a missing helper
body or repairing its class, signature or source file.

A source scan does not refresh compiled objects or extracted claim fragments.
After moving or renaming definitions, the full build regenerates those inputs
before the gate checks projected header claims and emitted symbol identities.
A clean source inventory by itself does not certify that compilation or the
subsequent claim-identity checks passed.
