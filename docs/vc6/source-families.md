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
anonymous-namespace nonces. These scopes can originate in `.cpp` or `.h`;
normalization retains the defining basename, type and full signature. The
`TAutoStrPtr` header control reproduces with different compiler nonces, while
changing its basename, type or constructor signature remains distinct.
Report successful source candidates and distinct
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

The spatial-frontier generator crosses three independently reconstructed
algorithms: zone-bound output lifetimes (`0x53b1f0`), the initial pending-point
insert in island subdivision (`0x53cd30`), and junction neighbour-query values
(`0x5443a0`). The 6 x 6 x 5 family has enough combinations for three complete
populations, with aggregate/specialist elites and fresh sampling between them:

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-spatial-frontiers.py \
  build/rmg-spatial-frontiers.json
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  build/rmg-spatial-frontiers.json --width 60 --keep 10 --jobs 6 --generations 3
```

All states preserve the bounds' output-reference write order and canonical
long min/max calls, the initial single-element `insert`, and the real map-query
overloads. The native fixture compiles the actual point/position/vector types
and arithmetic helpers, then all 180 method combinations with copy elision
disabled. It checks aliased output references (including input-field aliases),
cell flags, ordered coordinate queries, and random draws. Wrong upper bounds,
queued endpoints and queried levels are required to fail those same checks.
No fixture instrumentation enters a matching compiler snapshot.

All 180 spatial states compiled into 119 distinct code/relocation results;
the final ten parents reproduced. Assigning the returned zone position makes
`getInitialZoneBounds` exact in all 254 unmasked bytes, with the other 319
tracked RMG scores unchanged. The iterator/endpoint and junction-query axes
add no peak: island drawing stays at 99.4615% and the junction at 99.5699%.
Only the bounds assignment is adopted. This separates a proven local
improvement from neutral or regressive alternatives in the same population.

`generate-rmg-treasure-outline-family.py` addresses the cached group perimeter
at `0x535ee0`, separately from the exact prototype-outline builder. Retail's
initial row-major scan has a forward inner-loop exit and a backward jump;
the current C++ emits the opposite branch polarity. Five scan structures,
six point initialization forms and two dimension-read lifetimes make 60
states. A follow-up carries the ten reproduced scan parents into six actual
perimeter-start construction/assignment lifetimes:

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-treasure-outline-family.py \
  build/rmg-treasure-outline.json
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  build/rmg-treasure-outline.json --width 60 --keep 10 --jobs 6
PYTHONPATH=scripts python scripts/experiments/generate-rmg-treasure-outline-family.py \
  build/rmg-treasure-outline-parents.json --parents-from build/source-families/SCAN_CONTEXT/checkpoint.json
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  build/rmg-treasure-outline-parents.json --width 60 --keep 10 --jobs 6
```

The parent loader requires the completed input, matching source/header
snapshot, source hashes and reproduced scores/code identities; old scores
are never imported as observations for the new population. The two batches
produced 38 and 40 distinct object-code results without exceeding 95.1849%,
so neither changes the game body. Native controls compile all 360 scan/start
combinations against independently constructed rectangular perimeters and
ordered map queries. They cover all four blocking reasons, zero dimensions,
and cached outlines. In particular, retail tests `x == width` after the scan:
a positive-width, zero-height map traces four outside points. A seemingly
safer `y == height` return fails the control, as do changed start coordinates,
entrance predicates and levels. Fixture instrumentation never enters VC6.

`generate-rmg-zone-terrain-family.py` supplies another 60-state population for
the newly recovered `TRmgZone::chooseTerrain` at `0x532ab0`. Three template
bindings, four count-loop structures and five rank-selection forms preserve
the native-town preference, eight-terrain eligible set, conditional random
draw and exact `z == 1` underground restriction. Native checks use the actual
field declarations, coordinate type, terrain enum and owned table. An
independent explicit eligible list checks all 256 masks, alignments -1..8,
levels -1..2, zero/nonzero byte flags and six random results; wrong levels,
rank tests, native preference guards and empty-set defaults must fail.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-zone-terrain-family.py \
  build/rmg-zone-terrain.json
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  build/rmg-zone-terrain.json --width 60 --keep 10 --jobs 6
```

The first selector reconstruction matches the 150-byte retail body. The
60-state control produces 18 distinct objects and eight exact forms, with
ten reproduced finalists. Naming the template pointer/reference suppresses
retail's reload after `rand`, while changing the decrement test changes the
old-value register lifetime. Neither the higher-level loop alternatives nor
their combinations improve another RMG function, so the first exact body
is retained. Its new table at `0x6408c8` is read from the pinned image, not
aliased to the different army native-terrain table. The inferred RMG terrain
field and mine-selection local consistently carry integer ordinals, with
named terrain constants and no artificial enum conversion. The canonical
checkpoint also observes `placeZoneTreasures` at 68.8293% (previously 67.0569%)
with unchanged function source; no existing score or banked peak falls.

For finite single-TU source matrices, the newer
[`homm3 hypotheses` runner](../source-hypotheses.md) also records every scored
function in that TU. The river sprite/presence generators use this path, while
the multi-TU family runner remains useful for shared declarations and population
search. Both require semantic review and a canonical checkpoint before adoption.

`generate-rmg-group-fit-family.py` reconstructs and refines Complete's
`TRmgTreasureGroup::canFitObject` at `0x5355e0`. There is no Dreamcast
counterpart. The scalar first body scores 87.8889%; retail's three ordered
surface-neighbor scans, first entrance-object traits, original by-value
placement, and two retained `isPlacementBlocked` calls constrain the search.
Six origin lifetimes, five neighbor constructions and two failure-exit forms
make the first 60 states. Explicit scan scopes account for VC6's pre-standard
for-initializer lifetime in the shared-failure variants.

The next population retains all ten reproduced parents plus the current-source
control and 49 fresh states. These test point-valued direction operands,
returned-value assignment, compound translation, scan scope and the first
placement-failure join. A third population carries its ten reproduced parents
into position/trigger snapshots and prototype-reference binding. Parent input,
source/header snapshots, source hashes, scores and code/relocation identities
must reproduce before the next population is generated; a changed source
requires a new checkpoint, not reused scores.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-group-fit-family.py \
  build/rmg-group-fit.json
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  build/rmg-group-fit.json --width 60 --keep 10 --jobs 6
PYTHONPATH=scripts python scripts/experiments/generate-rmg-group-fit-family.py \
  build/rmg-group-fit-parents.json --parents-from build/source-families/FIT_CONTEXT/checkpoint.json
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  build/rmg-group-fit-parents.json --width 60 --keep 10 --jobs 6
PYTHONPATH=scripts python scripts/experiments/generate-rmg-group-fit-family.py \
  build/rmg-group-fit-entry.json --entry-parents-from build/source-families/PARENT_CONTEXT/checkpoint.json
HOMM3_GROUP_FIT_MANIFEST=build/rmg-group-fit-entry.json PYTHONPATH=scripts \
  python -m unittest homm3.vc6.test_rmg_group_fit
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  build/rmg-group-fit-entry.json --width 60 --keep 10 --jobs 6
```

The independent mask oracle compiles all 198 first/second-stage forms and,
when selected, every exact method from a later-stage manifest. It checks all
256 entrance masks, both guard types, all four neighboring-object trait
combinations, individual incompatible front entrances, nonzero byte flags,
two placement levels, every open-neighbor subset and each blocking reason.
Results, ordered surface-coordinate queries, input preservation and helper
arguments/call count must agree. Deliberately wrong ranges, traits, border
policy, neighbor level, rock checks and closed-neighbor acceptance must fail
the same oracle. The fixture projects actual field declarations and reuses
the actual value types, bitfields, predicates, lookup and point addition; it
makes no host-to-x86 layout claim and adds no instrumentation to VC6 inputs.

The first two batches produce 50 and 39 distinct object-code results, reaching
96.2381% and 98.8042%. The existing point-plus-vector operation restores
retail's 0x18 frame and otherwise-eliminated neighbor-y stores. Taking the
direction as the point operand removes the four-byte table-base bias, and
the first-failure join restores the three early branch destinations. Entry
loads remain a separate source-lifetime question rather than an inline-call
disagreement. The entry population produces 42 distinct objects and ten
reproduced finalists without exceeding 98.8042%, so the second-generation
body is retained. Its only collateral competitor movement is a lower
`writeMapHeader` observation in some variants; the adopted body preserves
that row. The trigger snapshots use `TObjectType::TPoint`, a distinct nested
POD type, not the RMG point class; the native fixture preserves that lexical
ownership too. No conversion constructor is invented for either class.
Across the three batches, the 180 scored states contain 108 unique code and
relocation identities after excluding repeated parents. The canonical build
banks 98.8042%, with all 25 CFG blocks, all branch destinations and both
placement-helper calls aligned; the other 324 tracked RMG scores are unchanged.
Full repository gates and the 54 related regressions plus three group-fit
tests pass. These checks validate this reconstruction, not whole-TU closure.

`generate-rmg-group-commit-family.py` explores Complete's
`type_random_map_generator::commitTreasureGroup` at `0x5469b0`. Retail proves
the group's saved position at +0x54, the object-position transfer, clipped
row-major traversal, pre-write destination flag snapshots and final mutable
virtual queries. No Dreamcast counterpart is currently mapped. The first
60 states cross three scan-coordinate lifetimes, five destination-position
construction forms and four snapshot/source-query orders. All preserve the
canonical accessors and the retained three-coordinate constructor.

The first population scores all 60 states with 60 distinct code/relocation
identities and ten reproduced elites, improving 99.8047% to 99.9141%. Reading
the border snapshot before the gate snapshot restores the outer-loop reload
order without changing the generated bit-query sequence. The raw remaining
delta is four operand bytes at +0x16b/+0x16c/+0x16f/+0x170: loading y and
multiplying width rather than loading width and multiplying y. The body is
692 bytes, with the same constructor relocation at +0x133 and no other raw
differences. All other 324 tracked RMG scores are unchanged in this batch.

The next population retains all ten reproduced parents and an unchanged
control, then rotates six local source-map/coordinate-binding refinements
across parents until there are 60 distinct source states. It verifies the
input manifest, all three TU snapshots, complete header population and bytes,
source hashes and repeated scores/code identities before reusing a parent.
The second population produces 43 distinct objects and ten reproduced elites
but no score above 99.9141%; no other tracked RMG score moves. Across the two
populations, 120 scored states cover 109 distinct sources and 92 distinct
code/relocation identities after repeated parents are removed. The minimal
first-generation snapshot reordering is retained; the additional bindings
are controls, not adopted source. The four-byte multiply difference remains
open rather than being hidden by a helper rewrite or false inline boundary.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-group-commit-family.py \
  build/rmg-group-commit.json
PYTHONPATH=scripts python -m unittest homm3.vc6.test_rmg_group_commit
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  build/rmg-group-commit.json --width 60 --keep 10 --jobs 6
PYTHONPATH=scripts python scripts/experiments/generate-rmg-group-commit-family.py \
  build/rmg-group-commit-parents.json --parents-from build/source-families/COMMIT_CONTEXT/checkpoint.json
HOMM3_GROUP_COMMIT_MANIFEST=build/rmg-group-commit-parents.json PYTHONPATH=scripts \
  python -m unittest homm3.vc6.test_rmg_group_commit
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  build/rmg-group-commit-parents.json --width 60 --keep 10 --jobs 6
```

The independent native oracle checks all 420 permitted initial/refined bodies
and every body in an optional selected manifest. It reuses actual value types,
bitfields, byte predicates, accessors, position constructor and object-position
getter, projecting field declarations without asserting host/x86 layout identity.
An independent flat-cell model enumerates local coordinates and filters the
overlap rather than repeating the candidate's min/max expressions. It checks
all 1,024 source/destination flag pairs, dirt/water/rock combinations, both
levels, negative offsets, empty dimensions and shared map storage. It verifies
object positions, both live vector traversals, exact call order and unmodified
map fields; virtual calls deliberately append objects to expose illegal cached
loop bounds. Wrong clipping, level, connection policy, snapshot copying, water
policy, omitted virtual calls and cached vector length must fail the same
checks. No fixture instrumentation enters the VC6 inputs.
The canonical full build banks 99.9141% under the adopted source hash, with
all 45 CFG blocks, 27 branches and three calls aligned and the other 324 RMG
scores unchanged. The 57 related regression tests and three group-commit
tests pass, as do all repository gates. This completes the current population
checkpoint, not the remaining multiply match or whole-TU coverage.

`generate-rmg-group-place-family.py` covers Complete's
`type_random_map_generator::canPlaceTreasureGroup` at `0x546c70`, whose
1106-byte retail body has no currently mapped Dreamcast counterpart. Its
first 60 states cross five guard-position constructions, four direction
lifetimes and three canonical neighbor-point constructions. Four successive
frontiers each retain ten reproduced parents, an unchanged-source control
and 49 fresh states. They test guard-point snapshots after the opaque object
checks, explicit outline-policy arms, three-coordinate guard scans using
either real lookup overload, and shared working-position lifetimes.

The five batches score all 300 states and reproduce all ten elites each time.
They produce 45, 51, 40, 46 and 31 distinct code/relocation identities per
batch, reaching 97.1072%, 98.7506%, 99.9102%, 99.9825% and 99.9850% from
90.8130%. Removing repeated parents leaves 244 distinct sources and 158
distinct code/relocation identities across the five batches. Point-plus-vector
restores the direction join; a real guard-point snapshot restores constructor
argument load order; the explicit allow arm restores the policy branch
destinations; and a three-coordinate guard scan restores retail's 0x48 frame.
Sharing the initial object and guard position then restores the first Z home.
The ordinary constructor, getter and map-helper interfaces remain intact.

The canonical full build banks 99.9850%, with all 65 CFG blocks, 45 branches
and three calls aligned. The /Z7-verified first differing candidate statement
is the later entrance getter. Six stack-displacement bytes in B19/B24/B25
put the entrance at -0x38..-0x30 instead of -0x2c..-0x24 and the neighboring
point's X at -0x28 instead of -0x38. Sharing all three positions, or only
guard/entrance, does not resolve the residual (99.9676%). Some experimental
variants also move `writeMapHeader`; the adopted source preserves all other
339 tracked RMG scores. The six bytes remain open, and these measurements
are not a whole-TU closure claim.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-group-place-family.py \
  build/rmg-group-place.json
HOMM3_GROUP_PLACE_MANIFEST=build/rmg-group-place.json PYTHONPATH=scripts \
  python -m unittest homm3.vc6.test_rmg_group_place
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  build/rmg-group-place.json --width 60 --keep 10 --jobs 6
PYTHONPATH=scripts python scripts/experiments/generate-rmg-group-place-family.py \
  build/rmg-group-place-parents.json --parents-from build/source-families/PLACE_CONTEXT/checkpoint.json
```

The later stages use `--lifetime-parents-from`, `--frame-parents-from` and
`--shared-parents-from` against the immediately preceding stage's checkpoint.
The loader verifies each prior manifest, all three TU/header snapshots,
source hashes and repeated score/code identities recursively. Historical
checkpoints are not valid after adopting a source change: establish a fresh
control and explicitly review a new frontier. Tests construct the historical
stage fixtures separately and verify that their old anchors reject changed
working source. The native check always exercises the current authored body,
all 60 initial forms and every body in an optional selected manifest; it does
not claim coverage of every unselected grammar combination.

An independent flat-cell oracle checks all 256 entrance masks, both map
levels, water/land zones and both direction ranges, each source/destination
veto, monster/other-object ordering, guard boundaries, nonzero byte flags,
empty/clipped bounds and occupied-cell traversal. It checks exact ordered
lookup and opaque-call arguments, including original cached bounds/zone
identity and live object-vector growth. Actual value types, bitfields,
predicates, canonical constructor/getter/point addition and both map lookup
overloads are reused with projected field declarations; no host/x86 layout
identity is assumed. Fourteen deliberately wrong variants must fail, including
wrong levels, guard traversal order, trait/outline rules and cached vector
length. No oracle instrumentation enters the VC6 source population.

`generate-rmg-group-select-family.py` targets `placeTreasureGroup` at
`0x5470d0`, the 646-byte Complete-only caller that selects and commits a
treasure-group offset. Its first 60 states cross five entry-declaration
orders, four centered-query forms and three public vector-emptying APIs.
Three follow-up populations each keep ten reproduced parents, an unchanged
control and 49 fresh states. They cover group-bounds binding, real iterator
lifetimes, public insertion overloads, centered point/vector operand roles,
equivalent ordered filters and lifetimes of existing coordinate values.

All four batches score 60 states and reproduce ten finalists each. They
produce 60, 41, 47 and 36 distinct code/relocation identities respectively;
across 240 scored states there are 198 distinct sources and 144 code
identities after removing repeated parents. The first two batches reach
86.8529% from 77.1261%, but the `resize(0)` leader retains `size` and `erase`
calls rather than retail's nested `copy` and `_Destroy`. That score is not
evidence that the clearing boundary has matched.

The third population reaches 87.0168% using an actual center-point copy,
compound translation and public single-element insertion. This restores
the retained `std::copy`/`_Destroy` sequence inside `clear()` naturally; all
nine call sites are present. No inline control or library-body substitution
is added. The remaining call-name diagnostic is `_Destroy`: the candidate's
`vector<TRmgMapPosition>` specialization is `c2 08 00` (`ret 8`), identical
to the three-byte retail body at `0x404140` currently named for
`vector<type_artifact>`. This folded empty-destructor label does not justify
changing the candidate vector's element type. The fourth population rechecks declaration lifetimes under this
new inline state but does not exceed 87.0168%, so the minimal third-stage
body is retained. The frame is retail's 0x48, but the vector and coordinate
homes differ, X rather than Y occupies EBX during the scan, and a separate
outer-loop bound-reload block is missing (17 blocks versus 18). These remain
source/register-lifetime questions, not a reason to flatten helpers.
The adopted candidate preserves the other 339 tracked RMG scores; some
experimental competitors also move `writeMapHeader`.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-group-select-family.py \
  build/rmg-group-select.json
HOMM3_GROUP_SELECT_MANIFEST=build/rmg-group-select.json PYTHONPATH=scripts \
  python -m unittest homm3.vc6.test_rmg_group_select
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  build/rmg-group-select.json --width 60 --keep 10 --jobs 6
PYTHONPATH=scripts python scripts/experiments/generate-rmg-group-select-family.py \
  build/rmg-group-select-parents.json --parents-from build/source-families/SELECT_CONTEXT/checkpoint.json
```

Continue from the second and third checkpoints with `--shape-parents-from`
and `--lifetime-parents-from`, respectively. Parent manifests, complete
source/header snapshots, source hashes and repeated score/code identities
are checked recursively. Adoption invalidates the old checkpoint anchors;
scores are not silently reused under a changed implementation.

The independent native oracle enumerates translated footprint inequalities
over a containing coordinate domain rather than copying the candidate's
adjusted-bounds loops. Its 4,696 scenarios include all 256 permission masks
over eight tied candidates, every random residue for those candidates, both
levels, positive and negative group origins, empty and single-offset ranges,
signed zone IDs and nonnegative spacing through 65,536. Controlled fit calls
raise, lower or wrap the current cell score and mutate zone/group metadata;
ordered lookup/call traces must retain the pre-call bounds/zone snapshots and
the post-call score re-reads. In particular, retail does not repeat the
minimum-score admission test after a successful fit call lowers that score.
Twelve wrong variants must fail, covering clearing, tie policy, score updates,
zone identity, post-call retesting, center coordinates, helper arguments,
random draw/selection, commit level and traversal order.

Actual value classes, packed cell fields, field declaration types, getters,
coordinate operations and map lookups are reused. Fixture members/globals use
the project scope prefixes; no host/x86 layout identity is claimed. Negative
spacing is outside this native contract because VC6 and the host compiler
promote the unsigned 16-bit bitfield differently; retail's unsigned comparison
is authoritative, and none of the source alternatives changes either operand
type. Native checks cover the current body, all 60 initial forms and every
body in the selected optional manifest, not every unselected grammar product.
The canonical full build banks 87.0168%, with the /Z7-verified first mismatch
at the entry declaration and the restored nine-call structure. The six
group-placement tests and five selection tests pass after adopting both
bodies; the earlier placement checkpoint also passed all 104 then-existing
RMG regression tests. These checkpoints do not complete either residual or
prove whole-TU coverage.

The treasure-reset family follows the reset helper's retained body at
`0x535040` and its copies in assembly (`0x5466e0`) and zone scheduling
(`0x547360`). Reading those copies exposes an overload error that the
standalone optimized body hides: retail calls `getMapItem(int,int)` with
two zero arguments at `0x547625` and `0x547717`, not the three-coordinate
overload. The one ordinary two-coordinate definition belongs in `rmg.cpp`,
where standalone reset expands it, while its retained 30-byte body remains
exact. Its RVA-backed ledger row migrates from `rmg_support` automatically;
the declaration is unchanged and no duplicate, false `inline`, or pin is added.

Body visibility also changes other consumers. On this checkpoint it raises
roads from 71.7235% to 72.6912% and scheduling from 68.8293% to 69.2629%,
but lowers assembly to 67.7205%, river creation to 79.5586%, and
river-to-object creation to 79.6612%. Their preceding 72.3188%, 85.9575%,
and 89.6141% peaks remain in MAX/HIST; hiding the required body again is
not a reconstruction fix. The subsequent reset family recovers assembly
past its preceding peak without changing the accessor boundary.

Three 60-state populations cross nine public container-emptying pairs with
seven surface-walk forms, then refine ten reproduced parents using actual
vector references/iterators, point origins, dimension snapshots, byte flags
and cursor lifetimes. They produce 51, 49 and 39 distinct code/relocation
results within their respective batches: 180 scored trials, 158 distinct
source states and 115 code identities overall. Every batch reproduces ten
elites. The standalone reset peak is 80.8902%, with the object-vector
`_Destroy` retained but the outline's still expanded, an extra EBP spill,
and swapped dimension-multiply operands. Its 11 blocks and eight branches
agree with retail; those counts do not make it exact.

The peaks for assembly (79.9913%) and scheduling (74.6342%) belong to
different source states, not one completed implementation. The adopted
third-stage combination instead gives reset 80.8902%, assembly 77.0873%
and scheduling 72.3117%. It changes those three tracked scores plus a small
`writeMapHeader` increase (93.2786% to 93.2864%), preserving the other 336
RMG scores relative to the corrected-accessor baseline. Assembly now
retains both entry erasures, but its first map clear is under-inlined and
its later two-coordinate lookups are over-inlined. Scheduling still has
three extra blocks and different reset/cleanup expansion decisions.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-group-reset-family.py \
  build/rmg-group-reset.json
HOMM3_GROUP_RESET_MANIFEST=build/rmg-group-reset.json PYTHONPATH=scripts \
  python -m unittest homm3.vc6.test_rmg_group_reset
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  build/rmg-group-reset.json --width 60 --keep 10 --jobs 6
PYTHONPATH=scripts python scripts/experiments/generate-rmg-group-reset-family.py \
  build/rmg-group-reset-frontier.json --parents-from build/source-families/RESET_CONTEXT/checkpoint.json
```

Use `--lifetime-parents-from` for the second checkpoint. The loader verifies
the preceding manifests recursively, source/header snapshots, source hashes,
and each elite's repeated score vector and code identity. Adoption requires
a new current-source baseline; old scores are not silently reused.

The independent native reset oracle checks 600 scenarios per body: empty,
rectangular and 16-by-16 maps, one to three levels, varied packed bits,
borrowed-pointer populations, and flag bytes 0/1/255. It compiles the actual
packed field declarations, map/cell clears, terrain setter and two-coordinate
accessor. Expected retail masks preserve unrelated bits; the ordered trace
requires both containers empty before map clear, every allocated cell cleared,
flags reset afterward, and only the surface plane repainted. Pointer objects
are not deleted, vector capacities and unrelated group fields remain intact,
and guard cells catch off-by-one writes. Twelve negative controls cover
missing phases/flags, wrong overload/origin, early flag stores, wrong terrain,
wrong plane count and short/long scans. The original 63 forms, two explicit
count-before-lookup dimension controls, the current body and each selected
optional 60-state manifest are exercised, not all unselected grammar products.
The native field layout is not an x86 layout proof; VC6 remains the byte verdict.
The adopted checkpoint passes the full VC6 build and all 113 RMG regression
tests. The relocated accessor also reports agreement in every sema view.
These results preserve the helper boundary and bank partial improvements;
they do not close the remaining reset/caller mismatches or the three TUs.

### Treasure-group fill lifetimes and unsigned centering

Retail `fillTreasureGroup` (0x546520) has no identified Dreamcast counterpart.
Its 97.7697% starting body already has the retail control flow and helper
decisions, but a four-byte frame surplus and different centering load order.
Three 60-state populations cross five centering forms, four receiver/value
bindings and three position lifetimes, then refine ten reproduced parents
through accepted-object scopes, map receivers and unsigned accumulation or
coordinate read-backs. They score 180 trials, 160 distinct source states and
76 distinct code/relocation identities overall; each batch reproduces ten
elites. The batches contain 48, 34 and 21 distinct code identities respectively.

The adopted initial-object scope reaches 97.8371% and removes the frame
surplus, with the other 339 RMG scores unchanged. Its selected pointer and
position end their lifetimes before the accumulation loop; the total survives
that phase. Retail still reads map width before prototype width and prefetches
the insertion end between dimension reads. The third population's staged
unsigned accumulators and coordinate read-backs do not exceed the scoped peak.
Reusing the primary position in later creation calls instead expands vector
insertion: the split-properties control grows from 438 to 821 bytes. None of
these experiments changes canonical helpers, adds inline controls or passes
an integer sum through signed-overflow arithmetic.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-group-fill-family.py \
  build/rmg-group-fill.json
HOMM3_GROUP_FILL_MANIFEST=build/rmg-group-fill.json PYTHONPATH=scripts \
  python -m unittest homm3.vc6.test_rmg_group_fill
PYTHONPATH=scripts python -m homm3.vc6.source_families \
  build/rmg-group-fill.json --width 60 --keep 10 --jobs 3
```

Use `--parents-from FIRST_CHECKPOINT` for the receiver/lifetime frontier and
`--centering-parents-from SECOND_CHECKPOINT` for the centering frontier. The
loader validates preceding manifests recursively, unchanged source/header
snapshots, source hashes and each parent's repeated code identity and score
vector. After adoption, generate a fresh baseline rather than silently reusing
pre-adoption checkpoints.

The independent native oracle uses a phase/budget state machine and scripted
opaque factory, map-placement, fit and virtual lifecycle boundaries. It checks
1,540 scenarios per body, including independent three-attempt budgets,
first-object failures, greedy stopping, pre-existing group entries, virtual
release-before-delete, factory-mutated dimensions and post-helper value loads.
Unsigned centers cover odd, non-square, negative and extreme dimensions;
generation limits cover signed division and remainder boundaries without
overflowing signed value arithmetic. Actual coordinate types, dimension
accessors and placement-limit constants are compiled from the owning source.
The first family, all second-stage grammar bodies, twelve baseline centering
forms, the current body and an optional selected manifest are exercised, not
every unselected third-stage product. Fourteen negative controls must fail.
Native checks establish this contract, not retail bytes or whole-TU closure.
The adopted scope passes the full VC6 build and all 19 focused fill, reset,
selection and placement regression tests. Its verified source diff starts at
the prototype/receiver loads, with the entry frame now agreeing with retail.

### Treasure creation helper boundaries (0x546190)

`generate-rmg-treasure-create-family.py` crosses three public mask forms,
four independent candidate/property clear-or-erase choices, and five real
footprint bindings/lifetimes. The Complete-only retail body calls passability
`bitset::test` and both vector erases; the current source expands these into
range-error and copy/destroy helpers. No Dreamcast counterpart is claimed.

The initial 60 states produce 56 distinct code identities, ranging from
57.4907% to the unchanged 81.6894%. `--parents-from FIRST_CHECKPOINT` retains
ten independently reproduced parents and generates 60 states around the
object-type, scan and footprint scopes, a generator-vector reference, a named
normalized value and public count insert calls. Parent admission checks the
source/header snapshot, input manifest, source hashes, scores and repeated code
identity. This second batch produces 41 distinct objects and no higher score.
Across both batches there are 110 distinct sources and 86 code identities.

The direct count insert family expands both insert bodies into allocation,
copy/fill/destroy paths: a representative grows to 0x66d bytes against retail's
0x385 and scores zero. These are successful compiles with real code divergence,
not missing symbols. Six generator-reference candidates also move
`writeMapHeader` from 93.2864% to 93.2786%; the other collateral scores hold.
Neither family is adopted, so no game implementation or MAX changes.

The independent native selection oracle covers 2,304 scenarios for every
first-stage body and every second-stage grammar body, plus an optional
`HOMM3_TREASURE_CREATE_MANIFEST`. It checks ordered filter/virtual/selector/
placement traces, map and zone limits, inclusive value bounds, compact mask
occupancy, candidate resets, cumulative weighted intervals, null factories,
and the final fresh value call and output store before generation. Coordinate
types, their constructor, dimension accessors and `CObjectType::getBitPos` are
extracted from the owning source. Twelve deliberately broken controls must
fail. The valid domain uses positive densities, 1..8 by 1..6 footprints with
nonzero occupancy, and non-overflowing value arithmetic; invalid bit indices,
host-versus-VC6 exception behavior and opaque-callee mutation are not claimed
as tested. The three admission/behavior/frontier tests pass. The final full
VC6 build passes with no checkpoint raises or source-hash MAX resets, and all
12 focused creation, fill and prototype-selection tests pass.

### River coordinator aggregate capture (0x549870)

`generate-rmg-river-coordinator-family.py` generates 60 states from five
trigger-offset forms, four object/prototype bindings and three position
lifetimes. The initial retail/source pass agrees on all eight blocks, four
branches and named calls, but retail loads both trigger components before
copying the wheel's position. That ordering suggested a real two-component
value capture, not a different helper boundary or extra compiler-only work.

The completed family produces ten distinct code identities, with twelve source
states at 100% and a range of 86.4462%–100%. All twelve exact states copy
`TObjectType::TPoint` before copying `TRmgMapPosition`; the receiver constness
and position lifetime do not change that result. A reference to the trigger,
two scalar captures, and subtraction through an explicitly constructed RMG
`TPoint` do not recover it. The twelve scalar-capture states also perturb
`writeMapHeader` from 93.2864% to 93.2786%. The selected aggregate-copy body
changes only `createRivers` among all 340 RMG scores, from 90.5077% to 100%.
Ten diverse parents reproduce; the simplest exact body is additionally
recompiled independently before adoption, with its full score vector,
code identity and source hashes checked.

The prototype's nested `TPoint` is distinct from the RMG coordinate type.
The VC6 smoke check rejects passing it directly to the RMG compound operator;
the native fixture imports both actual declarations so that this distinction
is tested too. The oracle checks 1,024 scenarios per body against an independent
route trace: ordered preparation, mixed wheel/scenery populations, negative
and non-square offsets, level preservation, two-cell source displacement,
live vector growth and a progress pointer replaced during routing. Opaque
river-to-object calls mutate the original coordinates and trigger to detect
incorrect reloads before the second call. Ten broken controls must fail.

Reusable finding: naming each scalar is not equivalent for VC6 scheduling to
copying its existing aggregate owner. Here the aggregate copy loads the pair
together and restores the generator/x/y register assignment across both opaque
calls. Preserve the actual nested type instead of merging equal-layout point
classes or introducing an alternate constructor declaration.
The adopted copy passes the full VC6 build, a freshly delinked all-views-equal
semantic diff, and all 15 focused coordinator, river-overlay, river-tile and
line-painting tests. Its CUR, MAX and HIST are all 100%.

### River object-target cell binding (0x5497a0)

`generate-rmg-river-object-target-family.py` crosses five accessor/result
forms, four offset-value forms and three receiver bindings: 60 distinct source
states. The original 95.8442% body agrees with retail on all 19 blocks, twelve
branches and its progress call. The only differing region is the final cell
address: retail loads the cell base earlier and keeps the tile-data displacement
in the OR instruction. The working hypothesis is therefore the returned cell's
binding, not another filtering branch or a replacement indexing formula.

The completed family produces eleven distinct code identities. All 24 states
that name a map-item pointer or reference reach 100%; the other 36 stay at
95.8442%. Naming only the tile-data member or using the canonical position
accessor does not suffice. Ten diverse parents reproduce. The selected minimal
pointer form is checked against its repeated code identity, complete score
vector and source hashes before adoption. Its ordinary scalar offset variables
and all helper/type declarations remain unchanged.

Nine candidates also move `writeMapHeader` from 93.2864% to 93.2786%, including
the selected pointer form. That function's own source is unchanged, so its
previous MAX/HIST remain banked. The selected scalar offsets already agree
with retail. Every other RMG score is unchanged in the selected
state, including the newly exact `createRivers`.

The native oracle imports the actual packed tile-data declaration, both point
classes, coordinate constructor, scalar/position accessors and object/resource
constants. It checks 1,536 scenarios per body, comparing every tile-data word
and guard cell against an independent 64-bit coordinate reference. Coverage
includes mountains/lakes/gem mines, non-gem exclusions, trigger versus unsigned
half-size offsets, negative and odd dimensions, both levels, clipping edges,
unchanged unrelated bits and one post-loop progress update. A reduced host map
item is used for these semantic tests; it is not retail layout evidence.
Arithmetic inputs keep the candidate's signed subtractions defined. All twelve
negative controls fail, including a signed-half-size error isolated from the
other axis so that clipping cannot mask it.

Reusable finding: bind the accessor's complete result object when retail keeps
a member displacement on the eventual store. Binding that member alone can
preserve the original premature address folding. This is compiler evidence for
a real local variable, not justification to paste the accessor's arithmetic
into the caller or alter its inline declaration.
The adopted pointer binding passes the full VC6 build and a freshly delinked
all-views-equal diff. All 17 focused object-target, coordinator, river-overlay,
river-tile and line-painting tests pass. Its CUR, MAX and HIST are 100%; the
unchanged-source header writer retains its previous MAX and HIST.

### River target coordinate induction (0x548c70)

`generate-rmg-river-target-family.py` combines five coordinate lifetimes with
four edge-cell bindings and three cursor/map-receiver forms. The initial
80.9379% body has an eight-byte frame surplus and an extra three-coordinate
constructor call. Retail's y/z homes instead fit adjacent fields of one map
position; its edge stores also retain complete-cell bases before accessing
the packed flag. This supports testing coordinate-member induction together
with the previously established complete-cell binding.

The 60 states produce 24 distinct code identities and six exact sources,
ranging from 74.1310% to 100%. Every exact source uses one `TRmgMapPosition`
for the scan and border counters, with an explicit pointer/reference binding
for each edge cell. Using scalar scan counters and separately filling a
coordinate can reach 99.9379%, but not exactness. The canonical default and
three-argument coordinate constructors remain unchanged: the reconstruction
uses the coordinate itself as the loop state, not a pasted constructor body
or an inline suppression directive.

Ten diverse parents reproduce. The selected form reuses the existing item
pointer for all four edge stores; its complete score vector, code identity and
source hashes agree on the repeat build. It lifts `markRiverTargets` to 100%
and restores `writeMapHeader` from 93.2786% to its previous 93.2864% MAX. Every
other RMG score is unchanged, including both newly exact neighboring river
passes. The header writer's older 95.7066% HIST remains a separate lead.

The native check imports the actual coordinate classes and constructor,
ground/auxiliary bitfields, water constant and scalar map accessor. A phase
machine derives the ordered coastal calls; an independent edge predicate
derives the final packed writes. All 60 bodies pass 512 scenarios covering
zero/one/two levels, narrow and non-square positive dimensions, terrain
mutation and shrinking dimensions inside the opaque coast call, exactly four
cardinal directions per admitted cell, progress-pointer replacement, and
unchanged unrelated fields/guard cells. Eleven broken controls must fail.
Host compilation lifts only the legacy VC6 scalar `for` variable declaration
when needed; it does not replace any constructor or game expression. The
reduced map-item fixture tests semantics, not the retail layout or codegen.

Reusable finding: adjacent coordinate homes plus a repeated constructor call
can indicate that the coordinate's fields were the induction variables. Test
the actual coordinate class and its lifetime before treating this as an
inliner refusal or modifying a shared constructor's visibility.
The selected coordinate loop passes the full VC6 build and a freshly delinked
all-views-equal diff: 19 blocks, fifteen branches and both calls agree. All
19 focused river-target, object-target, coordinator, overlay, tile and
line-painting tests pass. The three river-preparation passes now have CUR,
MAX and HIST at 100%; this is not closure of the three RMG translation units.

### Coastal target counter lifetimes

`generate-rmg-river-coast-family.py` covers `markRiverCoastTarget` (0x548a40).
No Dreamcast counterpart was found. The fresh initial summary, structure,
source-labelled comparison and inline prediction agree on both constructor
calls and all 25 control-flow blocks. Retail keeps direction in EBX, recycles
its old argument slot for each counter, and retains a 0x20-byte frame. The
initial source instead keeps its first two counters in registers.

The first 60-state family crosses five counter lifetimes, four coordinate
initializations/lifetimes and three step snapshots. All 60 compile; 17 distinct
objects span 73.0511–79.7557%, with ten reproduced retained candidates. The
79.7557% peak replaces the shared counter with independent water/dry/inland
counters. Lexically isolated counters and three independently named counters
both reach it; changing only the shared declaration's location stays flat.

The `--parents-from CHECKPOINT` mode verifies the completed manifest, source
and header snapshot, all 60 successful records, and the ten retained objects'
repeated scores, choices, identities and source hashes. It then creates a
60-state frontier from those parents: water-cell bindings, const step values,
named direction values, shared item lifetimes and a map reference. This second
batch has 60 successful compiles, 39 distinct objects and ten reproduced
retained candidates; its range is 69.5284–79.7557%. Water-item and direction
bindings reach 79.6875%, but none surpasses the first peak. Neither family
changes canonical arithmetic helpers, constructor visibility or inline policy.

The retained three-counter source changes only this function's score among
the 340 RMG rows. Its dry loop gains the memory-counter instruction sequence,
but not retail's exact home: the frame grows to 0x24, direction still spills,
and the water counter remains in EDI. This is a measured partial recovery,
not a solved register-allocation explanation or proof of original token names.

`test_rmg_river_coast.py` imports the actual coordinate classes, constructors,
arithmetic helpers, direction table, map accessor, entrance query and packed
tile fields. An independent affine ten-sample path checks 1,800 scenarios per
body, including valid coasts, terrain failures at each sample, entrance failures,
two levels, non-square maps and retail's asymmetric `x > width` bound. It
compares the complete padded allocation and unrelated bits. The reduced item
fixture is behavioral evidence, not a retail-layout claim. All 60 first-family
bodies and all six refinements of ten representative parents pass; seven
wrong water/entrance/count/bound/direction/target-bit controls are rejected.
Only the host compiler's legacy `for`-scope compatibility is adjusted.
The adopted version passes the full VC6 build and all 17 focused coastal,
target, object-target, coordinator and line-painting tests. Fresh delinking
confirms 79.7557% with all twenty branches and both calls agreeing; 22 of 25
block sizes now agree, up from 21. CUR, MAX and HIST are all 79.7557%.
The three RMG units remain at 235/340 currently exact tracked functions.

### Zone centroid coordinate lifetimes

`generate-rmg-recenter-family.py` targets `recenterZone` (0x53d0d0). No
Dreamcast counterpart was found. Fresh summary, structure, source-labelled
and inline-prediction views show matching scan/division branches and fully
expanded canonical accessors. The initial 88.9891% version uses a TPoint sum
and has a 0x30-byte frame, versus retail's 0x34. Its position-copy scheduling
and every subsequent aggregate stack home differ.

The first 60-state family crosses five position-copy lifetimes, four
accumulator representations and three independent input declaration orders.
All 60 compile, producing 24 distinct objects spanning 66.4239–95.3043%.
Reading the position before the zone index recovers the EBX copy loads and
the 95.3043% peak, but not the extra frame slot. Ten distinct retained objects
reproduce before the next stage.

`--parents-from` verifies the completed manifest, source/header snapshot,
all 60 successful records and the ten retained results' repeated scores,
choices, object identities and source hashes. Its 60-state counter/centroid
frontier has 40 distinct objects spanning 70.0652–95.5000%. The positive
centroid hypothesis carries the original level in an actual three-coordinate
sum, averages only x/y, and passes the full coordinate through the unchanged
setter. Default construction with field assignments avoids adding a retained
three-argument constructor absent from retail. This restores the 0x34 frame
and all scan/output stack offsets; it is not unused padding or a dummy local.

`--centroid-parents-from` tests full-coordinate copy forms and count placement
from the ten reproduced second-stage parents. Its 60 states produce 36
distinct objects without exceeding 95.5000%. This finite frontier initially
concentrates on the earliest copy alternatives. The complementary
`--centroid-balanced-parents-from` rotates each parent's starting point by
initializer family, giving all three copy forms and all six field-store orders
representation within 60 states. A test asserts that coverage. The balanced
batch produces 44 distinct objects spanning 66.0109–95.5000%, again with ten
reproduced retained results and no new peak. These are four 60-state batches,
not 240 unique bodies: each frontier deliberately retains its parent controls.

The adopted default-then-assigned position and three-coordinate centroid
change only this function's score among the 340 RMG rows. Entry-time load
and zero-initialization scheduling still differs; matching the frame does not
close the function. Canonical position accessors, constructors and shared
helper definitions remain unchanged. After adoption the initial generator
keeps the recognized child as its unchanged-source control plus 59 original
alternatives. Historical parent checkpoints still require their exact
pre-adoption source snapshot and cannot silently rebase onto edited source.

`test_rmg_recenter.py` imports the actual coordinate/bounds/packed-zone types,
position constructor, position getter/setter and scalar map accessor. Its
independent reference filters a flat allocation by rectangle membership,
decodes the signed eight-bit zone id explicitly, and checks integer averages
and preserved levels. Each positive body runs 3,000 small non-overflowing
scenarios: non-square/two-level maps, partial/empty/inverted bounds, negative
zone ids, ids outside the packed field's range, single-cell and larger groups.
The full cell allocation, guard words, bounds and slot pointer remain unchanged.
The reduced map-item fixture is behavioral evidence, not a retail ABI model.
All initial bodies, all seven refinements of ten representative first-stage
parents, and both 60-state scheduling frontiers pass. Seven wrong zone, level,
x/y sum, count, singleton-guard and output-level controls are rejected.
The adopted version passes the full VC6 build and all eight focused centroid,
spatial-frontier, coastal-target and river-target tests. Fresh source-labelled
retail comparison confirms every instruction after the entry block agrees;
all six branches, the return and the no-call/no-relocation streams agree.
CUR, MAX and HIST are 95.5000%; the three units still have 235/340 currently
exact tracked functions, so this partial recovery is not TU closure.

### Island interior fill

Retail `insetIslandZone` ends with a retained call at 0x53d36b to the
previously omitted 0x53cf50 cardinal flood. The recovered helper uses a
coordinate-vector stack, marks eligible neighbors when enqueuing them, and
does not mark an isolated seed merely because it was initially enqueued.
The source call belongs after all boundary drawing. No Dreamcast counterpart
is claimed; names are provisional retail roles.

`generate-rmg-island-fill-family.py` emits 60 JSON states combining coordinate
capture, public vector insertion, and direction bindings. This first population
produces 42 code identities. Its ten reproduced parents feed another 60-state
component-lifetime frontier (`--parents-from`), producing 21 code identities
and a reproduced **100.0000%** winner from a 79.2015% starting body.
The winner uses single-element `insert`, copies the neighbor's x/y, translates
through the unchanged canonical `operator+=`, and copies z afterward. That
operator only reads/writes x/y, so no uninitialized level is consumed. Only
the helper's score changes among the 341 RMG rows in the winning snapshot.

`test_rmg_island_fill.py` imports the actual coordinate operations, packed
flags, direction table, getter and lookup. A fixed-point reference tests
the generated fills independently of the candidate's stack traversal, also
testing the real inset caller with scripted boundary writes. Wrong diagonal,
zone, blocked-cell, level, extent and seed variants are rejected, as are
missing and prematurely executed caller fills. The adopted body is explicitly
tested. After adoption the generator retains the recognized winning refinement
as its unchanged control; old parent snapshots must still match exactly.

The final full VC6 build passes with CUR/MAX/HIST at 100.0000%. All 14 blocks
and all instruction rows agree. The stricter named-relocation view still
reports the folded empty `_Destroy` under `vector<type_artifact>`: both that
retail 0x404140 body and the emitted coordinate-vector body are `ret 8`.
The other two label differences are the claimed direction table at 0x69cdc0
and its end at +0x40. These are not divergent instruction sequences.
The four focused island/centroid tests pass, followed by both island tests
with the explicitly adopted winner included.
Integration with the current main headers and recovered RMG neighbors keeps
the helper at 100.0000%. The full merged build passes all gates, and all 15
focused island, centroid, river and treasure-group integration tests pass.

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

### Ordinary movement and native serialization boundaries

The union/pragma cleanup uses the same driver outside RMG. Its historical
`generate-shipyard-boundary-family.py`, `generate-shipyard-scope-family.py` and
`generate-movehero-helper-family.py` populations restore four ordinary static
helpers, preserving their actual calls and early-exit scopes while deleting
seven existing fences. The native `generate-game-vector-helper-family.py` and
`generate-game-vector-return-family.py` populations jointly recover typed
load/save templates, delete a pointer union and six more fences, and retain
both exact writer bodies. The
[audit](union-pragma-audit.md#ordinary-shipyard-and-movement-helpers) records
the frozen contexts, source/candidate counts, rejected higher-scoring false
declarations, and caller/whole-object controls. These generators require their
pre-adoption source snapshot; stale source anchors must not be relaxed merely
to rerun historical numbers.

`PYTHONPATH=scripts python -m unittest homm3.vc6.test_game_vector_io` extracts
the adopted serializer templates into the native stream-contract fixture.
It covers resize/zero-fill, short I/O, payload strides and narrow count
boundaries with six rejected negative controls. The associated narrow
source-owned vector-instance label join is tested by
`homm3.retail_labels.test_vector_helper_signatures`; it must reject an equal-size
ICF twin when the requested native element/signature is missing or ambiguous.

### University record ownership and header collateral

`generate-university-insertion-family.py` exhausts 36 meaningful API, receiver,
record and fence choices (24 object identities) in context
`9d5e8e882a44a0242bf7`. `generate-university-initializer-family.py` then compares
the generic aggregate plus explicit Conflux initializer with the old default
constructor: 13 successful states, 13 objects, context `99ba0ea5c2a6db673a5a`.
Both families reproduce ten retained candidates. Their exact anchors require
the pre-adoption `662ecc31` snapshot. The selected pointer/count-insert candidate
`75bbad764fd068a3fcc87499` removes the pointer union without altering the
randomizer's bytes or the Conflux initializer/caller bytes, and improves Load.
The [audit](union-pragma-audit.md#generic-university-records-and-conflux-initialization)
separates proven generic-record ownership from the provisional initializer name
and original helper kind. Scores alone do not establish those source facts.

`generate-university-header-control.py` is a post-adoption two-state negative
control: restore only the old constructor declaration in unrelated consumers.
It isolates the two small whole-build score movements in army and
singleselectionwindow: context `ea72eb19220d16fbe772` scores both states,
produces two distinct objects, and reproduces both. Restoring only the old
header declaration recovers exactly 99.9424% and 100%; every other tracked row
in those two TUs is unchanged. The initial seven-unit context `4b5443e33ddcd7da20ab`
was rejected at opposite-corner reproduction: executable section bytes and
scores reproduce, but five table/data units vary anonymous header identities
or symbol placement. It is not counted as a successful search. The driver
rejected those variations in that context. Header path/nonce identity is now
handled as described above; symbol-placement or code differences still fail
reproduction. That historical failed context remains excluded from the totals.

The shared native stream fixture imports the real record and initializer,
also checking untouched generic default-initialization bytes, the four Conflux
schools and the returned record address. Three new semantic negative controls
reject automatic defaults, a wrong school and a wrong return pointer. They
complement, rather than substitute for, the five-TU raw COFF controls and full
retail build.

### Native marketplace artifact ownership

`generate-market-artifact-owner-family.py` is a finite two-state ownership
control against `2359d5a4`: context `d7a243b77c8e39872831`, two scored states,
two object identities, both retained candidates reproduced. The selected
`372591115abd7d96fb098eeb` changes the entry, header, dispatcher, state and
every artifact reader/writer together. DC `DoBlackMarket` (0x1886d4) proves
the `TArtifact*` parameter; the recovered game and black-market fields already
have that element type. Retail's entry stores the pointer unchanged.

The pre-delink old entry label scores zero for the corrected signature. This
is not treated as body identity: `compare-coff-layout.py`, with the explicit
old/new function-name pair, independently proves identical section layout and
bytes, function positions, and relocation sites/kinds/destinations. It passes
for tradpost (150 sections / 4059 relocations), events (304 / 3873), philai
(229 / 1442), ai_player (420 / 1963), and townmgr (402 / 6828). The last three
need no rename. Selected-candidate to production comparisons pass without any
rename. Normal source-owned delinking migrates the entry at 100%; no current
score changes, MAX resets, or historical peaks lost. Full gates pass at
4063/4752 exact, 96.39% linked and 96.13% whole-image.

`homm3.vc6.test_market_artifact_owner` imports the actual entry, dispatcher arm,
enum, record, array member and global declarations into a native-only semantic
fixture. It checks identity and in-place mutation of all seven slots in each
owner, hero forwarding, modal state and AI dispatch. Eight negative controls
reject wrong pointers, records, state, branch choice and missing modal calls.
The host fixture does not claim x86 ABI or codegen proof.

### Marketplace ratio accessor and setup boundaries

`generate-market-ratio-boundary-family.py` exhausts the twelve supported
accessor, resource-value lifetime and selection-call combinations against the
native artifact-pointer checkpoint. Context `ddb391c2a56bfe8804a0` scores all
twelve states, produces six object identities, and reproduces all six elites.
DC rows 2235/2237 prove the two artifact getters; rows 2961/2990 reach the
same `SetupNewTrade` as the arrow arms. Restoring those source calls is not
conditional on a higher fuzzy score.

The selected `c55c83a118bd50486be2a0a8` keeps one ordinary `setupNewTrade`,
uses it at all four sites, and removes tradpost's final depth fence. Of 67
raw emitted functions, only `windowHandler` changes bytes; all other tracked
scores stay fixed, including the retained ratio helper at 100%. Production
reproduces the selected object's entire 150 sections and 4061 relocation
destinations. The handler falls from 90.6792% to 81.9322%, with the old peak
retained in HIST. The direct unfenced/flattened control gives 80.5105%; the
explicit right-value local and getter flattening do not recover the lost call.

The difference is measured at the named site, not inferred from a call census:
retail function +0x509 calls `computeTradeRatios` in resource selection, while
the natural candidate expands it and shares a selection tail. A byte-identity
gated C2 trace has caller cost 872 / initial budget 1744; its first two setup
copies admit the cost-153 ratio helper against nested budgets 189 and 188.
This is remaining caller-budget/source-state debt, not evidence against the
canonical helper boundary. No manufactured assertion replaces the fence.

The native market test also imports the actual setup body and checks selected
artifact/resource arguments, output-pointer identities, and the order of the
ratio call and amount reset. Five rejected semantic controls cover wrong
arguments, outputs, amount, missing call and premature reset. The full retail
build passes at 4063/4752 exact, 96.39% linked and 96.12% whole-image, with
one observed MAX reset and no banked RVA lost. The new census is 227 inline
overrides in 33 TUs, and 65 unions. The retained local peak is a recovery lead,
not grounds to reintroduce the retired compiler intervention.

`generate-count-markets-boundary-family.py` separately exhausts four
declaration/building-query choices in context `d39faa6bb2f147e6f44f`, producing
two symbol identities and reproducing both elites. DC row 621 calls
`GetTown` followed by `HasBuilding(14, true)`; all three retail entry points
expand this active-market test. Selected `5a8fa95a228c773b30041a02` restores
that query and uses an ordinary static helper at its original source position.
The baseline and selected raw COFF layouts, bytes and all 4061 relocation
destinations are identical; all three entries remain exact. The explicit
inline keyword was unnecessary, not evidence of an original annotation.

The search never writes authored source, CUR, MAX or HIST. Different function
implementations must not be banked under an old source hash. Review a retained
candidate, apply the actual C++ change, then run `homm3 build` to regenerate the
normal checkpoint and README. This preserves `CUR <= MAX <= HIST` and keeps
historical peaks separate from the current implementation's MAX. Completing a
family or getting every currently tracked row exact does not prove whole-TU
source completeness.

### Recovering video callers with canonical header helpers

`generate-video-recovery-family.py` compares the sound guard, resume guard
and pause-draining loop while retaining the ordinary `videoSoundOnOff`,
`videoResume` and `videoClose` calls. The shared `serviceSounds` body stays
inline in its proven `soundmgr.h` owner. The PC retail bodies prove the
four-handle sound predicate, zero-count protection, decrement-before-resume
transition and sound/Smacker/Bink close order; Dreamcast's port stubs provide
only declarations and source order for these video functions.

Against PR #3's `03a62eeb`, context `b675e964de23c1bd96c6` exhausts 48 source
candidates, produces 33 distinct emitted objects and reproduces ten retained
candidates. The adopted `1ca50e84d3aa84df22bc70b1` combines the sound guards
into one service call and the resume early-outs into one short-circuit guard.
Its production object reproduces the normalized candidate's complete code
and named relocation identity. The stricter raw COFF comparison also agrees
on all 125 sections, 936 relocation destinations and function locations.
All 30 tracked rows are compared:

| Function | Before CUR | Recovered CUR |
| --- | ---: | ---: |
| `videoClose` | 7.6923% | 38.1538% |
| `showVideo` | 39.1274% | 67.8147% |

Every other tracked score holds, including the two edited helpers. Neither
caller body changes. The sound-guard-only control recovers `videoClose` but
leaves `showVideo` unchanged. The resume-guard-only control gives `showVideo`
73.5019% but leaves `videoClose` at 7.6923%; the combined candidate recovers
more fuzzy-weighted retail bytes across the two functions. No new exact
function is claimed. The historical 100% peaks remain recovery leads.

The named sequence still shows two expanded `serviceSounds` operations in
`videoClose` where retail calls them; the final `closeBinkVideo` tail call
and the four indirect video-library calls agree. The current source-labelled
`showVideo` comparison first diverges at its first `videoClose` site. These
are remaining inlining/context differences, not grounds to move the sound
helper back into a `.cpp`, flatten the ordinary video helpers, or add a pin.
The finite guard/loop family bounds this PR's recovery; it does not close
`smackmgr` or recover every peak lost through header ownership.

`homm3.vc6.test_video_recovery` imports the actual three helper bodies and all
48 generated combinations. An independent transition/effect oracle covers
all 16 handle-presence masks, pause depths 0–5, both initial pause states and
all three entry points. Five negative controls reject missing sound service,
a wrong Bink pause argument, a missing pause-state clear, a missing Smacker
close and reversed close order. The host fixture validates these state and
call-order contracts; VC6 remains the byte verdict. The generator accepts
the reviewed adopted body as its unchanged control and retains the original
alternatives, rather than silently applying stale anchors.

The final full build, including fresh retail delinking, passes all gates and
raises executable matching from 94.81% to 94.83%. Exactly two CUR rows rise;
none fall. The 4,784 canonical definitions retain zero ownership violations,
and the existing 200 inline-depth pins are unchanged. Both edited helper
bodies remain at CUR 0%; their new source hashes reset MAX from 100% to 0%,
while HIST remains 100%. All 4,752 ledger rows and every historical peak
survive; the unchanged caller bodies retain their own MAX 100% peaks.

### Save recovery bounds after header ownership

`generate-save-local-recovery-family.py` uses the `game::Save` Dreamcast
local roster and retail narrow writes to test byte-buffer reuse/scope,
unsigned-word staging, the signed map-extra size, loop-index declaration
lifetime and the native-bool vector writer result. Context
`0db47607a4972b3ef94c` exhausts 48 states, emits six distinct objects and
reproduces ten candidates. `game::save` ranges from 59.2838% to 59.6005%
against 59.5944% unchanged; every other game row holds. The tiny gain does
not recover the `SavedGameHeader::reset`/`save` calls or the retail frame,
so none of these alternatives is adopted.

`generate-save-header-placement-family.py` separately compares the existing
class location, moving it beside the late inline definitions, and embedding
those same bodies inside the class there. Field order, signatures, body
operations and `game.h` ownership stay fixed. Compiler dependency records
select all 64 affected TUs. Moving the class alone is score-neutral; embedding
the methods does not improve `game::save` and slightly lowers two unrelated
rows. The existing class structure is retained. Context
`183778ec3eb1bc6b45bb` scores all three states, produces two distinct objects
and reproduces both retained candidates against a fixed snapshot. A prior
run rejected its final checkpoint after concurrent source edits and is not
counted as a completed search.

### Spell obstacle appends, filter widgets and sound service recovery

These searches start from PR #4's `ecca3a3d` full-build checkpoint and retain
canonical source ownership. The spell and initial filter generators require
that source checkpoint; replay them in a checkout of it with these experiment
scripts available. Their exact anchors deliberately reject a changed body.
The filter refinement also checks the parent snapshot and reproduced objects.
Sound variants accept either reviewed body as the unchanged control.

| Family | Scored source states | Distinct emitted objects | Reproduced retained candidates |
| --- | ---: | ---: | ---: |
| Four obstacle append boundaries | 16 | 16 | 10 |
| Filter allocation/loop lifetimes | 54 | 36 | 10 |
| Filter pointer conversion/refinement | 61 | 61 | 10 per generation |
| Sound service locals and guards, 51 dependent TUs | 30 | 14 | 10 |

`generate-spell-obstacle-append-family.py` tests the four source-proven
`push_back` sites in `combatManager::castSpell`. DC spells.cpp lines
849/925/962/996 supply positive call evidence. Three SH4 sites load the named
callee before the line block containing the indirect `jsr`: inspect
0x14feaa, 0x1500be and 0x1502c0 as well as Force Field's direct attribution
at 0x150212. The dossier's local call attribution alone misses those names.
Retail keeps count-insert bodies at +0x64e/+0x86c/+0x9a2/+0xae4; the native
vector's `push_back` supplies this nested call boundary. All four restored
calls reproduce 93.9814%, up from 72.8210% and above HIST 93.3687%, in context
`034c7ecf3f258f5bbaef`, candidate `eb22de398dcc8adf2e3cd0bf`. The complete
obstacle locals, append-before-slot order, and original-record arguments to
`placeObstacle` are unchanged. No container replacement or new helper is used.

`generate-filter-widget-recovery-family.py` exhausts 54 allocation-result,
button-binding, public append API and loop-scope choices in context
`3916b52e69a9a5b42b8c`. `generate-filter-widget-refinement-family.py` then
adds real widget-pointer conversions and a vector reference to ten reproduced
parents, deduplicating to 61 choices in `88fa5370dc9a8dba6348`. Candidate
`b6b839d0becb8fff58c7d6ce` reaches 92.9883% from 84.8995%. It retains all
constructors, the direct highlight field store and the proven disabled-frame
helper. The retired highlight setter is not restored. The only sibling
movement is `CEnterNameEdit::onKeyPress` 100% -> 99.8868%, one matching byte;
its source is unchanged, so MAX and HIST remain 100%. The best alternative
with no sibling movement reaches 92.7301%; the selected source recovers more
retail bytes overall. These are retail-only filter controls, so no Dreamcast
local roster is claimed for the inferred pointer lifetimes.

The independent native filter oracle checks all 56 widget identities,
constructor arguments, defaults, display fields, order and array identity,
with both fresh and reserved vectors and three preexisting-widget counts.
All initial and refined candidates passed before adoption. The permanent
`test_filter_widget_recovery` imports the current builder and canonical
`setDisabledFrame` body; five negative controls reject wrong highlight,
disabled frame, defaults, missing append and widget ID. This validates UI
construction behavior; it is not an x86 ABI model.

`generate-sound-service-recovery-family.py` takes all 51 header-dependent TUs
from compiler dependency records. Context `bf4bbe51a427f361ce69`, reproduced
candidate `7173bdb9b7e2604dc1808b27`, captures the stream after `AIL_serve` and
nests the three real state guards. `serviceSounds` stays inline in SoundMgr.h
and its retained body stays exact. DC 0xe6ef4 is a WinCE stub proving source
ownership; the nonempty PC behavior comes from retail 0x59a7d0. `showVideo`
rises from 67.8147% to 94.1120%, with every other tracked score holding across
the 51 units. The unchanged combined guard is the negative byte control.
The native test imports the current body and all 30 variants. It checks lock
receiver identity separately from the global manager, stream changes during
Miles service, state guards and call order; six negative controls fail.

The generators preserve real helper boundaries and operations. No inline
pins, release VERIFYs, synthetic caller weight, or duplicated helper bodies
are introduced. Remaining mismatches require further evidence; these finite
families do not establish TU closure or exhaustion of the wider recovery queue.

The adopted production objects match the reproduced candidates in section
bytes, relocation destinations and function locations: spells 170 sections /
1,979 relocations, singleselectionwindow 836 / 7,378, smackmgr 125 / 939.
The final full build and fresh delink raise executable matching from 94.83%
to 94.98%, with 3,937/4,751 current exact functions. Exactly three CUR rows
rise and only the keyboard-handler byte falls. All 4,752 ledger rows and all
historical peaks survive. Ownership remains 4,784 definitions with zero
violations, and the existing 200 inline-depth pins are unchanged.

Post-adoption source/call inspection confirms the four obstacle count-insert
calls. `castSpell` still has a 0x80 frame against retail's 0x94 and different
shared spell-effect tails. The filter's first remaining difference is the
vector-base load before its first allocation and the append argument's stack
slot (-0x24 versus -0x20). `showVideo` now retains both early sound-service
calls, as retail does; its later expanded `videoSoundOnOff` still leaves a
`serviceSounds` call where retail calls that ordinary helper. Source-labelled
comparison first differs at the Smacker-handle guard. These specific residuals
remain recovery leads; equal call totals are not evidence of equal boundaries.

### Restoring map readers and the enemy-marking implementation

The next recovery starts from PR #4's `30ecbab7` full-build checkpoint.
`NewfullMap::readObject` had four Dreamcast-proven ordinary readers flattened
into its switch while their named definitions remained inactive stubs.
Restore `readBoatData`, `readHolyGrailData`, `readShrineData` and
`readShipyardData` in mapcell.cpp at their original source positions, with
ordinary declarations using the PC `TAbstractFile` stream. DC 0xed984,
0xedd14, 0xedde8 and 0xefe28 prove the signatures, locals, reads and status
returns; DC readObject calls them at source lines 3350/3379/3388/3406.
Retail's corresponding arms prove their inline expansions, field layout and
character conversions. Complete defers the old shipyard terrain scan to
`loadShipyards`. Grail/shrine retain their final short-read checks in the
helper; the caller discards status, so those final comparisons disappear
naturally in the retail expansion.

`generate-map-reader-helper-family.py` records the initial two-state byte
control in context `a70571d26b1e09d9f9f3`, comparing all 76 header-dependent
TUs. Both objects reproduce; candidate `6835a1c0264f821a3d5faaff` raises the
reader from 56.6382% to 60.0594%. Restoring the helpers also exposes an older
placement error: the existing inline `CObject::getTrigger` (DC line 1119)
sat before line 1095's reader. The final source moves it between
`getObjectTypePtr` and `findTrigger`; its body and declaration are preserved.
That mandatory source-order correction is score-neutral, and the next full
checkpoint passes the ownership gate. Replay the initial generator at
`30ecbab7` with the experiment script available; the control itself predates
this separate order correction.

The host reader oracle imports both the recovered helper source and the
actual four caller arms. It covers all 256 byte values and every short-read
boundary, including partial shipyard state, boat arguments, read sizes/order,
and discarded caller status. Five negative controls reject wrong boat owner,
grail radius, shrine value, boat coordinate and status. This is an effect
oracle, not a replacement for VC6 layout or byte validation.

`generate-quest-guard-append-family.py` then tests 36 source choices at the
corrected full checkpoint (`376ad64f07b280fd969d`): DC's function-scope
read-count local and five separate read/result-test statements, plus actual
quest-vector iterator/reference lifetimes and public append spelling.
Twenty distinct objects and ten retained candidates reproduce. Candidate
`159e2077aa36cf6abc2d1005` keeps the count local and uses `push_back` in the
QUEST_GUARD arm, reaching 60.9729% without any sibling movement. The count
alone is byte-flat; preserve its positive source evidence. Named vector
references and inline data-end expressions add no gain. Both insertion
workers still expand where retail keeps two-argument calls, and the seer
constructor still expands: this is partial recovery, not a closed boundary.
The native quest-arm oracle imports the actual constructor and reader and
checks append-before-index behavior, record copy, existing vector contents,
null quests and optional data registration across empty/reserved vectors.
Every explored arm passes; five wrong controls fail. The generator accepts
an adopted member only after an exact round trip through its finite family.

The active `searchArray::markEnemy` body was a more direct failure: an
integrated carcass stub replaced the existing implementation. The earlier
source still contains the real body after its inactive reference stub, and
DC 0xa0a44 independently proves get_hex, the nested minimum-cost guard,
valid-move flag and unsigned-short cost store. Retail contains the matching
expansions in both callers. Restoring that one ordinary body in place gives:

| Function | Before CUR | Recovered CUR |
| --- | ---: | ---: |
| `findCombatPath` | 50.9545% | 92.2920% |
| `markTeleport` | 71.6135% | 100% |
| Retained `getHex` | 0% | 100% |

`generate-mark-enemy-restoration-family.py` exhausts the two-state control
in `8a71980a4ee16edd6844`; both objects reproduce. The adopted implementation
is candidate `10a61ce51f0271cdda248e93`. The native oracle imports its actual
body and covers existing flags, cheaper/equal/dearer costs, negative and
32-bit boundary costs, unsigned-short narrowing, neighboring-cell preservation
and accessor ordering. The empty-body control and four incorrect controls
fail. Retail comparison verifies the four retained getHex calls inside
findCombatPath's mark expansions; teleport marking and the retained accessor
are exact again. The previous combat-path peak is fully recovered.

An audit of all 4,784 active definitions at the original checkpoint found
only this explicit active stub. The source-ownership gate now rejects
`ACTIVE-STUB` even when ownership and signature are correct. It enumerates
active definitions through the AST, then checks placeholder comments while
ignoring string/character literals. Tests cover inactive stubs, unannotated
source/header bodies, actual empty constructors and literal marker text. The
new gate also rejects the real pre-fix markEnemy snapshot as a negative control.

### Crossover lifetime recovery bound

`generate-crossover-lifetime-family.py` tests the current retail-only
`SCampaign::pruneCrossoverHeroes` body, preserving the repeated inflated-size
read after the virtual pool query, signed scenario count, canonical max,
artifact accessors, sort and vector assignment. Thirty-six combinations of
real hero references/pointers, scenario-vector access, sort endpoints and
front/begin access produce 24 distinct objects and ten reproduced retained
candidates (`98fcd19748a92c53c1fc`). None improves the original 20.6088%.
The caller stays unchanged. Its former five extracted phase wrappers have
no independent source evidence and are not restored to regain their score.

The combined map/path checkpoint passes the full build, fresh retail delink
and all gates: 95.04% executable matching (from 94.98%), 3,940/4,751 current
exact functions, 4,788 canonical definitions and zero ownership violations.
All 71 relevant ownership and native-oracle tests pass. The final mapcell
object matches its reproduced refinement object across 434 sections and
2,094 relocation destinations; findpath matches its restoration object across
54 sections and 238 relocation destinations, with function locations checked.

Five CUR rows improve. Two unchanged-source rows move with the four required
map-header declarations: `army::doAttack` 99.9424% -> 99.9040%, and
`advManager::doCombat` 98.5379% -> 98.1758%. Their MAX and HIST remain held.
`CEnterNameEdit::onKillFocus` returns from 99.8710% to exact. All 4,752 ledger
rows and all historical peaks survive; no unchanged-source MAX is lowered.
The existing 200 inline-depth pins are unchanged. These results preserve
source structure through measured header collateral and leave the wider
map/campaign recovery queue open.

### Campaign read buffers and map-save result lifetimes

The follow-up starts at `028e1b09` with a clean, full-build checkpoint.
`SCampaign::load` is Complete-only: the full Dreamcast roster has no matching
procedure. Retail's six unsigned counts/identifiers use dword loads followed
by masks after one- or two-byte reads. That is not proof of an int source
buffer. `generate-campaign-load-buffer-family.py` exhausts 64 independent
width combinations in `906a2aed32d636cfd8eb`: two distinct emitted identities,
both reproduced, and **no score changes in any tracked function**. Keep the
narrow buffers; replay this historical-width control at `028e1b09`.

The useful alternative is lexical lifetime. Retail gives the modern scenario
fields distinct scratch homes, and the previously removed synthetic scalar
helpers left isolated read/assignment blocks. The generator
`generate-campaign-load-lifetime-family.py` tests named locals in their real
enclosing prefix/loop scopes for four groups, plus separate days/score scopes.
All 32 states compile, nine objects reproduce in `20be28ec9c6caccae8c7`.
Candidate `3937da971249f1010a4be6b1` (`scopes-11010`) raises the loader from
50.8734% to **52.7064%**, with every sibling unchanged. It names the leading,
scenario and artifact buffers; count buffers keep their existing scopes and
days/score remain in the loop scope. It preserves the complete legacy arm.

The emitted modern artifact-pool and hero-pool shrinking paths now retain
additional vector size calls. The first two clear/erase workers, implicit
legacy array iterator, legacy string assignment and most nested size calls
still disagree with retail. The result is partial recovery; the historical
79.2531% remains a lead. No constructor body, scalar helper or pragma is added.
The native oracle imports the actual modern arm and every generated arm,
checks versions 28/35/36 for all 256 leading byte values, empty and populated
vectors, exact read-size order, campaign remapping, signed artifact/assigned
words and completion-flag widening. Five incorrect controls fail. It does
not claim host layout or legacy-construction coverage.

`NewfullMap::Save` has stronger source evidence: dc:0xecdf8 names function-scope
`int count` and assigns each ordinary helper result before its negative test.
The Complete seer/quest loops still belong in the caller. DC's static
`TSeerHut::SaveSeerList` (0x12d7e8) uses the global list and checks per-record
save results; retail uses the current map and ignores those results. This
semantic contradiction rules out restoring that older helper interface.

`generate-map-save-lifetime-family.py` exhausts 24 combinations of result
assignment, list-count sharing, unsigned loop-index scope and final failure
check in `a58ef16d0c060ff7bba4`. Sixteen objects are distinct; ten retained
candidates reproduce. The adopted `d929d6f793d422be39225cf3` restores the DC
result local and spells the final negative-result check explicitly, reaching
**35.5206% from 32.1267%**, with every sibling unchanged. Count assignment
alone is neutral; the explicit tail supplies the gain. Sharing the two
Complete list-count buffers is worse, and a shared loop index adds no gain.
The early list size queries and both event-list helper expansions remain
unresolved. The native oracle imports the actual driver and all 24 variants,
checks each helper failure, both layers, short/negative write results, ignored
quest-write and seer-save results, serialized counts, and list growth during
saving. Four incorrect controls fail. Both adopted-state generators require
an exact round trip through their finite family before admitting the body.

The combined full checkpoint passes all 149 units, fresh retail delinking and
every gate. Exactly the two intended CUR rows improve; all 4,752 ledger rows,
all historical peaks and every unchanged-source MAX survive. Executable
matching remains 95.04% at the displayed precision, with 3,940/4,751 exact
functions, 4,788 canonical definitions and the same 200 existing pins.
All 73 relevant tests pass. Production customcampaign and mapcell objects
match their reproduced candidates in executable section bytes, relocation
destinations and function-symbol locations: respectively 391/312 sections,
1,930/1,862 relocations and 344/274 function locations. The broader recovery
queue remains open.
