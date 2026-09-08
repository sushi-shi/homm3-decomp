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

The audit runs as a fatal gate in full builds. Clang skips body semantics so
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
The inventory also reads named methods from full CodeView class field lists,
including overload lists. A method without a procedure record remains a
declaration-only identity: the field list proves its signature and generated
status, but supplies no body file or line. Such a written counterpart is a
fatal `UNLOCATED` finding until its body ownership/order is recovered. It cannot
be exempted as Windows-only or borrow another overload's procedure line.
For example, the `TAutoArrayPtr<char>` and `TResourcePtr<TTextResource>` copy
constructors have distinct ordinary field-list declarations, while only their
pointer constructors have procedure records. Field-list declaration sequence
does not establish definition order. Template type-family comparison preserves
the containing type, pointers/references and qualifiers when selecting these
identities; repeated procedure emissions still do not permit duplicate bodies.
Non-anonymous `VA_COMPGEN` enrollments must also bind an emitted VC6 public.
A raw enrollment placeholder is a fatal `CLAIM_COMPGEN`, even when its RVA
remains in the comparison ledger. Anonymous initialization thunks retain their
deliberate synthetic names.

The older cleanliness metric now admits a unique source-local class only when
its CodeView methods originate in that .cpp and have no competing header origin.
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
platform signature change needs an explicit source annotation identifying the
reviewed CodeView procedure; a new Windows-only overload needs its exact filter
entry.

Completion is still outstanding: remaining identity ambiguities and all reported
source ownership/order violations must be resolved. Existing exclusions are exact
reviewed platform shims, Complete-only RMG/quest/campaign classes, and the
Complete popup-state destructor, and the CObjectType conversion/default
constructor pair proved by the retail conversion body and the DC class field
list. They do not exempt an accepted backlog.
