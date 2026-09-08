# Generated C++ source families

This search follows Gruntz's exact-edit JSON families: combine meaningful
construction, assignment, lifetime, helper-call and statement-order alternatives.
It is not `state-sweep`, which adds transient random includes to an unchanged
function body. Source alternatives require semantic review and retail evidence;
a higher similarity score alone does not validate a reconstruction.

Inside the pinned build shell, with `HOMM3_DIR` set to the owning worktree:

```sh
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  scripts/experiments/rmg-grid-source-family.json --width 60 --keep 8 --jobs 6
```

The schema-1 manifest specifies `source`, `units`, and named `axes`. Every axis
has a unique exact `find` span and named `options`; an option omitting `replace`
keeps the original. `extra_edits` can atomically change another source/header,
using `find`/`replace` or `insert_before`/`insert_after` plus `text`. Edits must
not overlap. The first option of every axis is the unchanged-source control.
Anchors deliberately fail after an incompatible source edit; update the family
for the new baseline instead of applying a stale patch.

Each candidate is compiled from an isolated source/header snapshot using every
unit's exact profile. A single TU object scores all of its configured functions,
including exact siblings and missing bodies. An unchanged-source compile must
reproduce the current ledger, and an opposite-corner candidate must compile and
reproduce before the full population starts. Failed compiles are recorded
separately from successful candidates.

`--generations 3` aims for 60 successfully scored new combinations per generation
(or all remaining combinations when the finite family is exhausted), with
crossover/mutation from retained candidates and 25% fresh sampling. Half the
retained slots favor aggregate score/exactness, with the rest covering specialist
gains above the original baseline. Failed candidates are replaced by new
combinations; a high failure count stops for inspection. Distinctness uses
emitted code and named relocations, excluding timestamps, paths and VC6
anonymous-namespace nonces. Report successful source candidates and distinct
code results separately: several real C++ alternatives can compile identically.
This identity is a search/reproduction metric, not an extra score normalization.
The retained candidates must reproduce both their scores and that code identity.

`build/source-families/<context>/` retains the input, snapshot, candidate trees,
objects, complete score vectors, failures, generation summaries and checkpoint.
Re-running with the same inputs resumes; a new source, profile, toolchain,
target, normalization implementation or seed creates another context. An
exhausted family calls for a new evidence-based family, not endless resampling.

The RMG strength-family generator demonstrates a structural follow-up: it
selects top distinct grid parents from a completed grid checkpoint and combines
them with ordinary implementation alternatives for `getTransitionStrength`.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-strength-family.py \
  build/source-families/GRID_CONTEXT/checkpoint.json build/rmg-strength-family.json
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  build/rmg-strength-family.json --width 60 --keep 8 --generations 3
```

Other generators cover the classifier caller/packed-field snapshots, the
diagonal checks/coordinate class, border marking/connection flooding, and
shipyard footprint/side selection, and connection-path/prototype selection.
They read the current authored baseline;
their output lives under `build/`, while the generator itself is reviewable.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-neighbour-clear-family.py \
  build/rmg-neighbour-clear-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-diagonal-family.py \
  build/rmg-diagonal-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-border-flood-family.py \
  build/rmg-border-flood-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-shipyard-family.py \
  build/rmg-shipyard-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-path-family.py \
  build/rmg-path-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-position-family.py \
  build/rmg-position-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-guard-placement-family.py \
  build/rmg-guard-placement-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-outline-family.py \
  build/rmg-outline-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-outline-refine-family.py \
  build/rmg-outline-refine-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-copy-cursor-family.py \
  build/rmg-copy-cursor-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-tile-value-family.py \
  build/rmg-tile-value-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-zone-math-family.py \
  build/rmg-zone-math-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-clear-lifetime-family.py \
  build/rmg-clear-lifetime-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-clear-declaration-family.py \
  build/rmg-clear-declaration-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-line-walk-family.py \
  build/rmg-line-walk-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-line-selector-family.py \
  build/rmg-line-selector-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-line-selector-end-family.py \
  build/rmg-line-selector-end-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-line-refresh-family.py \
  build/rmg-line-refresh-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-line-proxy-copy-family.py \
  build/rmg-line-proxy-copy-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-line-proxy-binding-family.py \
  build/rmg-line-proxy-binding-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-line-entry-family.py \
  build/rmg-line-entry-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-line-neighbour-family.py \
  build/rmg-line-neighbour-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-line-query-family.py \
  build/rmg-line-query-family.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-grid-add-boundary-family.py \
  build/rmg-grid-add-boundary-family.json
```

Use an output as the family-runner argument. The neighbour family keeps
the eight real classifier calls and their order; it varies point/query and
iterator lifetimes plus independent packed-field updates. The diagonal family
keeps signed offset tables, reference-select clamps and early-return predicates,
then varies point reuse/construction and real dimension-accessor calls. A
retained caller and each expanded copy are checked against their own retail
evidence; no variant pastes a canonical helper into its callers or pins inlining.

The border family tests scalar bounds against one rectangle or two corner
values, including independent bound-evaluation order. A rectangle's allocation
can preserve operand homes that scalars reuse, even with identical control flow.
The flood family preserves LIFO/cardinal traversal and varies public STL calls,
point lifetime and byte predicates. It checks the whole TU: a local statement
can change an earlier `copy`/`_Destroy` expansion elsewhere in that caller.
The shipyard family preserves the six-cell footprint and four side offsets,
varying coordinate reuse/construction, terrain snapshots and equivalent
opposite-side selection. Its flood axis compares real public vector APIs for
the one-element seed; it does not introduce unused work or alternate helpers.
The path family distinguishes the undecorated route from a virtual placement
whose flag must be queried again. Bounds/predecessor storage and public
prototype-vector operations remain independent source axes; every candidate
keeps the ordinary selector and map-lookup boundaries.

The three-dimensional position family is separate from the terrain grid family.
It preserves the retained three-int constructor while comparing implicit and
explicit copies/assignments, independent field-store order, returned-value
lifetimes and arithmetic-definition order. These are actual C++ value operations,
not declarations added solely to perturb the compiler. Every arithmetic variant
keeps the canonical compound operator, and every resulting object is scored
against all three TUs' tracked functions before selecting parents.

The guard/placement family targets signed predecrement loops, the map-version
limit's lifetime, ordered random draws, count storage and independent constructor
stores. It preserves the retail RoE exclusion boundary, including its unusual
gap, and calls the canonical base constructor. Serializer variants narrow the
two-byte count without changing the stream writes. Zone-placement alternatives
keep row-major traversal, prototype accessors and public vector operations while
varying footprint evaluation order and selected-position copies.

The outline family retains all four placement helpers and the separate trigger
and passable-mask tests. Its alternatives vary mask-index lifetime, coordinate
construction, cyclic connectivity return structure and trigger-entry storage.
It also carries the guard family forward with mixed member-initializer/body
assignments: declaration order is fixed, and only independent body stores are
permuted around the derived constructor's vptr setup. The already exact monster
serializer is kept unchanged and remains part of every candidate's score vector.

The outline refinement separates the lazy-size/zero setup, direction-value
copy, canonical translation and comparison receiver. It combines those with
independent predicate initialization, iteration-local previous state, descending
map coordinates beside ascending mask coordinates, and entry-time placement
snapshots. These alternatives recovered the full outline and connectivity
bodies; the footprint and placement callers still require further work.
The generators can rebase their reviewed forms after adopting a winner;
round-trip tests preserve the mask arms and their coordinate progression.

The copy/cursor family holds the now-exact outline helpers fixed. It varies
the allocation result and real `std::copy` argument lifetimes, the footprint's
coordinate representation and independent loop increments, and placement's
byte-return/terrain guards. A named allocation result can change the expanded
copy's induction variables without changing the standard helper. Reference
bindings are changed only for the local receiver, never for similarly named
members in an object's pointer chain; round-trip tests cover that distinction.

The tile-value family compares four-field special members, returned-value
lifetimes and forwarded snapshot construction across both painter classes and
the terrain TU. The line-painter getter has an explicit output reference;
the adapter's getter still returns by value. Natural alignment bytes are not
source fields. The `--refine` generator option concentrates on implicit-copy
parents after the broad sweep: a nontrivial copy constructor can change the
adapter return handling throughout terrain callers, even when it makes a
single setter exact. Snapshot alternatives call the existing value constructor
and copy the actual flip flags, without adding work solely to steer inlining.
The jointly exact wrappers use a field-constructed setter snapshot and a named
adapter return followed by canonical assignment; the implicit copy boundary
also restores the packed-cell fill and its previously exact terrain callers.

The zone-math family preserves signed subtraction, integer squares, conversion
to double, `sqrt` and truncation. It varies real arithmetic intermediates and
the branch-local radius minimum, without replacing the distance operation or
flattening position accessors. The prototype-selector axis keeps its complete
filter loop and vector lifetime, varying only the count, random index and
selected-pointer lifetimes after the empty guard.

The clear-lifetime family changes the public pointer-vector clearing API and
the construction/order of the three actual packed-word snapshots. Their
lifetimes may span the erase without introducing dummy locals: this vector
owns pointer values, so clearing it invokes no pointed-to game object code.
Every variant retains all named bitfield updates and the final member-store
sequence. Copies never expose or replace packed words with raw integer masks.
Exhausting this family raised `TRmgMapItem::clear` from 76.95% to 91.03% by
carrying the connection snapshot across the erase. The remaining copies stay
afterward; moving every snapshot early does not reproduce the same lifetime.
The declaration follow-up separates declaring those snapshots from reading
their fields: all member reads stay after erase while real local declarations
may precede it. This tests reusing the copy loop's register for the later
connection value, without keeping the connection value live during that loop.

The line-walk family reconstructs a shared missing body before searching its
actual source alternatives. It preserves the two three-dword axis records,
unsigned major/minor selection and backward visit order, including the extra
minor-axis paint and zero-distance endpoint. It varies axis argument binding,
independent initialization, selection spelling, count-up/count-down loops and
real point lifetimes. A portable C++ test compares every generated axis option
over a grid of endpoint pairs; only the pinned VC6 build judges byte matching.
The broad beam raised this body from 79.13% to 99.45%; `--refine` exhausts the
nearby reference-axis and cached-count combinations. The exact source uses
reference-bound axis arguments, the original major/minor arm order and a cached
count-up bound, which VC6 turns into retail's countdown loop. Direct point
temporaries suffice; named and reference-bound call-local points also matched.

The line-selector family preserves ordered cardinal guards, all four reflected
corner tests and the optional corner/end ranges. Its three output references
are separate locals in the sole retail caller, so independent store order is
a valid source axis. Joined exits end the corner-search locals' real scope
before their common zero-flip label; no jump bypasses a live initialization.
The other axis varies the availability value and indexed/reference/pointer
reflection iteration. VC6 requires the pointer-to-array declaration before the
`for` clause here, although the host compiler accepts it in that clause.
The `--refine` family concentrates on retail's pattern-before-flip stores and
branched end flips. A portable test checks every mask, both optional ranges,
non-boolean mask bytes and the complete focused cross-product. The host test
checks semantics only and never adds harness declarations to a VC6 candidate.
The end-case follow-up tests the neighbour before the complete per-arm stores,
instead of writing the common pattern/X outputs before that decision. This
recovers the two retail Y-flip exits: explicit branches or early returns after
the common stores still collapse to `SETE`. The exact family keeps the indexed
reflection loop and constant flip stores after each cardinal group. Both a
byte and a native-bool corner-availability local reproduce the same code.

The line-refresh family keeps the painter-plus-coordinate proxy and canonical
neighbour translation. It varies real proxy member construction, the returned
proxy lifetime, and value-return versus explicit-output tile handling. The
last choice changes declaration, helper body and caller atomically: retail's
virtual tile query already has an explicit output reference, and its caller
has no following tile copy. A shared grid-translation axis still calls the
canonical compound-add helper, choosing construction and named/compound return
forms rather than pasting additions into callers. A portable full-product test
checks copied-coordinate ownership, virtual query/set forwarding, meaningful
tile fields and unsigned translation wraparound; only VC6 judges codegen.

The proxy-copy follow-up compares implicit copying with six ordinary explicit
memberwise copy constructors, keeping the reviewed value constructor and output
tile query fixed. It combines those with real return/entry initialization and
assigned-grid return forms. Header declarations and ordinary definitions change
together; the parent family selects its value-constructor overload explicitly.
Its portable full-product test disables copy elision and checks independent
coordinate ownership through repeated copies. It does not change the proven
reference argument ABI or introduce a destructor just to affect codegen.
The binding family instead tests the expanded value constructor's coordinate
parameter, whose ABI is not separately pinned: copying that coordinate on entry
and binding a const reference both preserve the proxy's owned value. All twelve
construction options keep `at(const grid&)` unchanged and update the constructor
declaration/body atomically; they never declare two competing overloads.

The line-entry family extends the reconstructed cluster with rectangle clearing
and walker point painting. Both retail bodies construct their initial proxy
in place, with no retained factory call proving that source boundary there.
Their source options therefore compare a returned proxy with a direct call to
the same ordinary constructor. Refresh's two canonical factory calls stay put.
Another option moves each vertical border's initial y into the `for` clause,
after its cached end is computed, matching the retail store lifetime. All
variants preserve the right border's asymmetric bottom bound and the point
walker's complete neighbour snapshot before any neighbour refresh.
The host-only painting oracle checks every axis option and their opposite
corner against all small-grid rectangles, including empty/border rectangles,
plus old/new line types and the virtual gate's low-byte behavior. It compares
the complete ordered query, clear, assign and refresh stream; the harness never
enters a matching VC6 source snapshot.
The exhausted 384-case entry family produced 88 distinct code results. The
selected source changes only the two entry constructions; it raises clearing
from 86.01% to 92.65% and point painting from 70.19% to 79.94%, leaving every
other tracked RMG score unchanged. Clearing's ten calls now agree with retail;
its lower-border coordinate store and the point painter's retained compound-add
boundary remain separate source-lifetime follow-ups.

The line-neighbour family tests those two remaining boundaries directly. It
compares a lower-y snapshot taken before the upper bound but assigned to the
point afterward, plus real named/temporary border proxies. The point-painting
axis varies translated coordinate values/references and construction followed
by the canonical compound-add call; it does not replace that helper with
arithmetic in the caller. The original helper-return chain remains a control.
The second neighbour pass and all entry operations stay unchanged. Round-trip
checks reject an unreviewed first-pass body rather than silently dropping new
statements, and the same portable painting oracle checks every new axis option.
All 300 combinations compiled into 105 distinct code results. Keeping the
original border factory calls and naming only the lower-y snapshots raised
clearing from 92.65% to 94.51%, with all other 162 scores unchanged. The
point-painting alternatives did not improve 79.94%; their low was 72.83%.

The query follow-up keeps the coordinate argument aliased to the proxy's owned
member. Seven ordinary getter forms bind the real receiver or coordinate,
vary their evaluation order, or name the actual result. They combine with six
member-construction forms and four border-proxy lifetimes. The helper remains
canonical, and no variant copies its implementation into a caller. This tests
the remaining virtual-call setup after the lower-bound lifetime is recovered.
Its 168 successful combinations produced 80 distinct code results but no new
peak; the direct-query baseline was retained.

The grid-add boundary family compares the current in-class compound-add body
with one ordinary definition before or after the line-painting cluster. There
is no recovered inline qualifier for this Complete-only helper. The latter
placement follows the retained retail address order. Declaration and definition
move atomically, and the same body remains available for ordinary auto-inlining
in the painting TU. Translation result lifetimes and representative neighbour
constructions remain independent axes. Round-trip checks enforce one definition;
the portable oracle crosses every placement with the real lifetime alternatives.
After rebasing onto the newer adapter checkpoint, all 168 combinations compiled
into 48 distinct code results without raising an existing RMG score. Moving only
the ordinary compound-add definition after the painting cluster is sufficient
to retain its exact 33-byte body at `0x4fa540`. The existing grid copy constructor
also reproduces all 22 bytes at `0x4fa520`. Their painter call sites and unsigned
coordinate ownership establish the two admissions; body size alone does not.

The mixed grid-constructor family extends that boundary with genuine
initializer/body splits, copy-through-assignment, translation lifetimes and
member-definition order. It preserves the unsigned layout, both constructor
signatures and the ordinary compound-add definition. Each stage compiles 60
source states across **all three** RMG TUs:

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-grid-ctors-family.py \
  build/rmg-grid-ctors.json
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  build/rmg-grid-ctors.json --width 60 --keep 10 --jobs 6
PYTHONPATH=scripts python scripts/experiments/generate-rmg-grid-ctors-family.py \
  build/rmg-grid-returns.json --parents-from build/source-families/CTOR_CONTEXT/checkpoint.json
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  build/rmg-grid-returns.json --width 60 --keep 10 --jobs 6
PYTHONPATH=scripts python scripts/experiments/generate-rmg-grid-ctors-family.py \
  build/rmg-grid-orders.json --member-orders-from build/source-families/RETURN_CONTEXT/checkpoint.json
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  build/rmg-grid-orders.json --width 60 --keep 10 --jobs 6
PYTHONPATH=scripts python scripts/experiments/generate-rmg-grid-ctors-family.py \
  build/rmg-grid-assignment.json --assignment-from build/source-families/ORDER_CONTEXT/checkpoint.json
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  build/rmg-grid-assignment.json --width 60 --keep 10 --jobs 6
PYTHONPATH=scripts python scripts/experiments/generate-rmg-grid-ctors-family.py \
  build/rmg-grid-proxy.json --proxy-from build/source-families/ASSIGNMENT_CONTEXT/checkpoint.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-grid-ctors-family.py \
  build/rmg-grid-temporaries.json --temporaries-from build/source-families/PROXY_CONTEXT/checkpoint.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-grid-ctors-family.py \
  build/rmg-grid-visibility.json --visibility-from build/source-families/TEMPORARY_CONTEXT/checkpoint.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-grid-ctors-family.py \
  build/rmg-grid-binding.json --binding-from build/source-families/TEMPORARY_CONTEXT/checkpoint.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-grid-ctors-family.py \
  build/rmg-grid-factory.json --factory-from build/source-families/BINDING_CONTEXT/checkpoint.json
```

Run each generated manifest with the same runner arguments before generating
its descendant. The visibility test is a separate negative-control branch;
binding consumes the pre-visibility parents. Factory returns have portable
semantic coverage but are not part of the completed retail matrix below.

Six copy forms, five coordinate forms and two returns produce six distinct
code/relocation results. Crossing those reproduced constructor parents with
ten returns produces 36 results. Crossing the ten retained parents with six
definition orders produces the same ten code results: source-order variation
alone is neutral here. Each follow-up validates the parent source snapshot
and carries the exactly unchanged source as its control, replacing only an
object-identical equivalent parent when necessary.

At the `3464038c` baseline, the assignment-built copy form raises brush
destruction from 78.6021% to 92.1398% and terrain `paintPoint` from 90.9656%
to 98.3653%, but stops emitting the retained 22-byte copy constructor in
**every** RMG TU. The missing body is not a renamed equivalent. It also lowers
the exact brush `changeTerrain` method to 81.3309%. No such header is adopted
or banked as an observation of the unchanged source. The assignment follow-up
tests the actual copy-assignment boundary: the implicit operation and five
explicit in-class value implementations, all with the same signature and
receiver-reference result. It does not add guards or artificial work.

The completed chain contains 480 successful source states. The distinction
between source alternatives and resulting code matters:

| Family | Source states | Distinct code/relocation results |
| --- | ---: | ---: |
| Mixed construction | 60 | 6 |
| Translation return | 60 | 36 |
| Member-definition order | 60 | 10 |
| Copy assignment | 60 | 40 |
| Proxy construction | 60 | 50 |
| Returned temporaries | 60 | 37 |
| Ordinary translation visibility | 60 | 20 |
| Proxy parameter binding | 60 | 45 |

The latter stages carry each parent's header **and** proxy implementation
together. Value versus const-reference proxy parameters change the ordinary
constructor declaration and definition atomically; the retained factory's
`at(const grid&)` ABI never changes. All ten binding finalists retain the
reference parameter. Moving the translation body to five ordinary TU locations
adds no tracked gain. These controls do not supply evidence for an inline pin.

A coordinate-assigned translation result preserves every currently exact body
and raises line point painting to 83.3411% and terrain repair to 92.3508%, but
lowers terrain point painting and line refresh. Another parent restores the
refresh compound-add call and reaches 70.2846%, while introducing unwanted
arithmetic calls in terrain painting. No candidate closes the mismatching named
helper sequence across callers, so the canonical header remains unchanged.
Further work needs a different source hypothesis, not another sample of these
exhausted matrices. The optional factory-return axis tests real named values
and const-reference-bound temporaries; the return copy completes before the
local's lifetime ends, and no artificial destructor is added.

Portable C++98 checks disable copy elision and compile the actual point class
with its canonical compound-add body. They cover all 300 initial
constructor/return combinations, plus 60 controls for each follow-up:
extreme unsigned coordinates, signed offsets, wrapping, copied value ownership,
aliased coordinate inputs, self-assignment and returned receiver identity.
The proxy checks use actual constructors/factories and virtual queries, including
mutating the input after construction to reject borrowed-coordinate ownership.
These are semantic controls, not a substitute for VC6 or retail-byte verification.

For finite single-TU source matrices, the newer
[`homm3 hypotheses` runner](../source-hypotheses.md) also records every scored
function in that TU. The river sprite/presence generators use this path, while
the multi-TU family runner remains useful for shared declarations and population
search. Both require semantic review and a canonical checkpoint before adoption.

Real global ownership can also expose an ordinary helper boundary. The river
and road pattern globals call the shared pattern-table constructor and register
cleanup callbacks which tail-call its destructor. Their definitions belong on
the consumer side of that support boundary. Placing them beside the destructor
leaves the initializer calls intact but inlines delete into both callbacks;
source ordering alone does not prevent that generated cleanup expansion.
Likewise, both final painters retain the shared line-walker constructor. Making
its small body visible in their TU expands it even when its definition follows
both callers. Keeping the shared walker on the painting-library side preserves
the retained calls without an inline pin or an alternate declaration. The common
six-slot painter prefix is supported by the same walker reading both final
painters' dimensions at +4/+8; that model does not assert an original class name.

The search never writes authored source, CUR, MAX or HIST. Different function
implementations must not be banked under an old source hash. Review a retained
candidate, apply the actual C++ change, then run `homm3 build` to regenerate the
normal checkpoint and README. This preserves `CUR <= MAX <= HIST` and keeps
historical peaks separate from the current implementation's MAX. Completing a
family or getting every currently tracked row exact does not prove whole-TU
source completeness.
