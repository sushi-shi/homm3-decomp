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

### Full-width object-type input ownership

`generate-object-type-read-owner-family.py` starts at `ba9d07f2` and exhausts
eight owner/query alternatives in context `f19ddfdcac44d6eb1e45`: eight scored
states, three emitted identities, all three elites reproduced. It preserves
the existing integer buffer's filename-length and extra-field uses, crossing
the old union and three native enum lifetimes with a buffer/record diagnostic
query. DC rows 3610..3614 positively use `int_buffer`; the enum local is a
Complete I/O-boundary hypothesis, not a purported recovered DC declaration.
Row 3619 does positively query the committed record for the diagnostic.

Selected `3edff3daea1caee731a5ae58` reads into an enum local adjacent to its
guard, then assigns the record, and uses that committed field in the diagnostic.
Both unscoped lifetimes produce the same 99.9633% function. Merely changing
the union control's diagnostic is byte-neutral and stays exact. The scoped
native value gives 99.9163%. All other mapcell scores stay fixed.

The `--phase-scopes` follow-up tests actual independent field-value lifetimes,
not unused compiler mass: filename length, native object type and extra field,
each optionally scoped through its commit. Context `e8265cfda7e5bc50aa1e`
exhausts nine states (including the unchanged union parent), nine object
identities and nine reproduced elites. The best native phase variant is
99.9673%, but requires splitting the positively evidenced generic integer
buffer and adding scopes. That tiny improvement does not justify choosing
the more speculative source model over the simpler native parent.

The adopted object reproduces the selected candidate's complete sections.
Against the old object, all 419 section layouts, 318 function-symbol positions
and 2077 relocation sites/kinds/destinations agree. Only `readObjectType`'s
1216-byte body changes: 18 stack-slot displacement bytes, with the same
instructions otherwise. All other 259 emitted function bodies are unchanged.
This is an explicit raw comparison, not a relaxation of retail normalization
or the strict byte-neutral comparison tool.

`homm3.vc6.test_object_type_read_owner` compiles the actual read/commit block,
enum and abstract-file interface at native `-O0` and `-O2`. Six valid enum
values and read reports 0..4 verify width, return value, one read call,
neighboring sentinels and commit timing. Five negative controls reject direct
field reads, a two-byte width, rejection of a full read, a wrong value and a
missing assignment. No malformed-input or x86 ABI claim is made by this host
fixture. The neighboring 16-bit readers cannot use the four-byte enum's
`sizeof` without changing their actual wire contract.

The full retail build passes at 4062/4752 exact, 96.39% linked fuzzy and
96.12% whole-image. Only this function resets MAX (100% to 99.9633%); HIST
retains 100%. The census becomes 227 inline overrides and 64 unions.

### Ordinary volume conversion and selected-setting lifetimes

`generate-volume-boundary-family.py` starts from `a7a83c6e` and exhausts
twelve combinations in context `c682cc6559d87a47dbef`: twelve scored states,
five object identities and five reproduced elites. Three actual setting
bindings (value snapshot, const reference, direct reads) cross equivalent
short-circuit/nested bounds and the old/removed auto-inline fence. The source
keeps both duplicated scale arms and the shared lower/upper clamp; Complete's
0..127 return must not acquire Dreamcast's final 0..100 platform conversion.

Selected `7bb1334725faaa2026c1258b` uses branch-local const references and
removes the fence. Strict `compare-coff-layout.py` passes for all 110 raw
sections, 699 relocation destinations and function positions against the old
`c441b9aa0f03520c7206d8aa` control. Production passes the same comparison.
Every scored soundmgr function remains unchanged, including all four exact
callers. These references are source-lifetime hypotheses, not declarations
proved by DC's empty optimized-local inventory.

The unfenced value control `338a47b3bd2bded6fbade5ae` retains the same helper
body but removes the named calls at setMusicVolume +0x1f, modifySample +0x9a,
memorySample +0x15a and processStopAndPlayMP3 +0x25. The four bodies grow,
and their scores drop to 0%, 57.3125%, 81.0864%, and 83.8794%. Direct reads
also expand the helper, with either guard form. Nested value/reference forms
keep all four calls, but only the short-circuit reference form preserves the
original whole object without changing the guards. This is a natural binding
decision, not a new pragma, helper copy or release-elided dummy operation.

`homm3.vc6.test_volume_boundary` imports the actual body and enum, checking
15 music settings, 15 effects settings, four selector values and 513 bounded
volume values at native `-O0` and `-O2`. The 461,700 cases per optimization
verify selection, scale, minimum-one/maximum-127 limits, disabled settings
and unchanged setting storage. Five
negative controls change the selected setting, range, divisor, minimum or
maximum and are all rejected. The host test does not assert Miles behavior,
VC6 inlining or overflowing multiplication semantics.

### Joint recheck of byte-neutral Load fences

The complete post-integration deletion audit at `d7f7be28` finds two
individually byte-neutral `game::load` fences. Before adopting them,
`generate-redundant-load-fence-family.py` exhausts the four combinations in
context `c37c4b8bdc6f290cfc35`: four scored states, one emitted object identity,
one reproduced elite. This separate joint control prevents assuming that two
individually inert directives are also inert together.

The combined `87c339c85bd6535c153b4761` removes only the creature-bank
`loadObjectVector` fence and the normal-return fence. Against the unchanged
`3c3d0c896d77feef57c45b36`, strict raw comparison passes all 822 sections,
5287 relocation destinations and function positions. Every game score stays
fixed. Source statements, helper interfaces, return scopes and the remaining
`isLocalHuman` directive are unchanged. The source generator records the
pre-adoption anchors; the complete audit snapshots remain separate controls.

### Compiler-generated text-dialog teardown

`generate-text-dialog-dtor-family.py` uses positive Dreamcast member attributes:
the CTextDialog field list (class `0x2c52`, fields `0x2c53`) gives its destructor
`0x107`, including `compgenx`, as for CWaitForReadyPlayersDlg. Explicit
CAnimatedDlg and TDialogBox destructor controls give `0x007`. This is not an
absence-of-lines or absence-of-locals argument. The canonical correction removes
the false explicit empty declaration/body and keeps the retained retail body
at `0x490770` as an `IMPLICIT_DTOR` claim; the deleting wrapper stays claimed.

Context `921c64eda6cd9b36c940` exhausts four states across all twelve actual
header consumers: four combined emitted-object identities and four reproduced
elites, each scoring 1129 functions. Implicit-with-existing-fence candidate
`62c9f3297fad1750e2c2db22` preserves every score. CAnimatedDlg's instruction
bytes stay fixed, while its base-cleanup relocation now correctly names
TDialogBox instead of CTextDialog. The readiness caller and the separately
emitted wait-dialog destructor keep their named calls and body bytes.

Removing the animated-destructor fence makes the implicit wait-dialog
destructor exact (86.3333% to 100%), but drops waitForReadyToPlayMsg from
90.6522% to 75.0683% and removes the retained CNetMsgHandler::copy body.
The correction therefore does not itself justify deleting that override.
`generate-animated-dtor-lifetime-family.py` follows the corrected parent with
six real nullable-sprite bindings crossed with that fence. Context
`5ebe0ea56c6aed44ee2a` exhausts all twelve states, five emitted-object identities
and five reproduced elites. Every unfenced state still loses the copy body;
the guarded object reference raises the readiness caller to 83.0186%, while
the remaining forms stay at 75.0683%. None is adopted. The ordinary destructor,
virtual Complete sprite disposal and implicit base cleanup remain canonical;
no dummy work, alternate declaration or explicit derived-destructor router is
introduced. These finite results bound those lifetime alternatives, not all
possible caller reconstruction.

### Creature-bank and resource-cost boundaries

`generate-bank-value-lifetime-family.py` starts from the shared nested-size
fence in valueOfBank. Context `66bc080d5cad95205a44` exhausts sixty bank-receiver,
artifact-size/receiver and combat-value lifetimes crossed with fence removal:
sixty scored states, eight emitted-object identities, eight reproduced elites.
Every unfenced option makes the retained bank body exact (92.2043% to 100%),
but leaves aiValueOfEvent at 97.4610% instead of 98.0336%. No spelling resolves
that remaining nested decision, and all other philai scores stay fixed.

The follow-up discovers an actual missing source boundary: DC `0x10f2f8`,
philai.cpp:1331, calls the player-pointer resource-cost overload from the
player-ID overload. DC bank `0x110808` in turn calls the ID overload with the
hero owner. `generate-bank-resource-boundary-family.py` restores that ordinary
forwarding call and the bank's canonical call together, crossing both with
fence removal. Context `a44951e66f11cc57a5c4` exhausts eight states, eight object
identities and eight reproduced elites. The canonical pair with the fence
(`d42fab3c0207a4cdb4be3f14`) preserves the complete control object's 229 sections,
1442 relocation destinations and function locations. The previous duplicated
loop was not evidence that retail lacked the forwarding boundary.

The adopted unfenced pair `82675560b82de2a6702b0056` retains the ordinary
overloads, the single pointer-based loop, the inline bank helper and all three
source calls. Production exactly reproduces its 229 sections, 1440 relocation
destinations and function positions. Against the original control, all 127
emitted functions remain present; only valueOfBank and its dispatcher change
raw body bytes, with the other 125 byte-identical. The retained bank matches
all twelve retail blocks and all four named calls. Its remaining named data
differences are the existing game/current-player aliases and unclaimed creature
traits label, not new helper-call discrepancies.

Ordered review narrows the dispatcher residual to its **first** bank arm:
retail calls vector::size at +0x66d, while the candidate expands it. The second
arm keeps its call (candidate +0x81e), and the final arm still calls valueOfBank
(+0x11ff). The second arm's six blocks all match in the separately shifted
local ranges; both early arms keep the correct pointer-based resource-cost
calls. Thus the 0.5726-point caller reduction is one missing natural nested
call, not loss of both arms or replacement of a canonical helper. Full build
keeps its MAX/HIST at 98.0336% and restores the bank's MAX to 100%.

`homm3.vc6.test_bank_resource_boundary` imports the three actual bodies. Its
bounded 11,520-case native oracle at `-O0` and `-O2` checks empty/failed-combat
exits, owner versus active-player selection, per-resource accumulation, signed
reward guards, artifact counts and unchanged bank inputs. Fixture-only entry
counters check the recovered call route and ordering. Seven negative controls
are rejected, including a numerically equivalent bypass of the owner wrapper.
The fixture does not claim retail ABI/layout, arbitrary floating-point edge
behavior or VC6 inlining.

### Mouse-thread lifetimes and inherited task teardown

`generate-stop-mouse-lifetime-family.py` tests mutable/const references to
the actual global thread and event storage, plus guarded-block/early-return
exits, crossed with the existing fence. References retain every global reload
across Windows calls; they are not snapshots of potentially changed handles.
DC's older helper retains only the final pointer restore, so its PC line gap
is not used as assertion evidence. Context `34cdd1cea3463120d055` exhausts all
36 states: six emitted-object identities and six reproduced elites. Every
unfenced option raises generateRandomMap from 92.6386% to 97.9759% but drops
setupScenarioOptions from 100% to 90.1470%. None is adopted. The caller's
proven request/progress/path scope and the shared ordinary helper stay intact.

The score-flat task-destructor deletion needs a separate untracked-body review.
`generate-update-task-fence-family.py`, context `0b5874006d53a33b2e6b`, reproduces
both source states and both emitted-object identities. All 223 tracked scores
agree. `verify-update-task-bodies.py` proves that all 396 emitted functions
remain present, with 395 bodies and their relocation destinations unchanged.
After excluding only `.debug` metadata, all 737 remaining sections preserve
order and attributes; all bytes/relocations outside the generated Proc body
stay fixed. That body changes from a five-byte jump to Task into the same
38-byte teardown as Task, with identical instructions and delete relocation.
Padding changes its section from 16 to 48 bytes. The retained Task body
continues to match every retail instruction and named call at `0x583ef0`.

The initial ICF hypothesis is **refuted**, not reported as a repair. Three
genuine hash-verified VC6 LINK controls use the unchanged/unfenced real objects
with `/OPT:ICF`, plus unfenced `/OPT:NOICF`. Each keeps Task and Proc at
distinct addresses: the ordinary Task body is non-COMDAT. These partial
diagnostic images have 45 unresolved symbols and are not executed. The normal
source model is preserved; no false inline, explicit derived destructor or
unproven implicit Task declaration is introduced to force a fold. The dead
scalar-wrapper claim and final ownership/link-layout questions remain separate
debt. Removing the override therefore buys a cleaner source at unchanged
tracked scores, with an explicitly bounded 32-byte untracked code increase.

Production exactly reproduces selected `f0e98f39dea432f23d145379`: all 824 raw
sections, 7211 relocation destinations and function locations agree. The
body verifier also passes directly against production and rejects unchanged,
reversed and unrelated mouse-helper candidates as three negative controls.
The final full build preserves 4074/4764 exact and 96.38% linked/whole-image,
with no score change, MAX reset, migration or lost banked RVA.

### Sacrifice-slot helper boundaries

`generate-sacrifice-slot-boundary-family.py` checks two positive DC facts from
`update_slot` (`0x125b3c`): line 842 obtains the artifact snapshot from
`hero::get_artifact`, and lines 847/855 both call the ordinary
`update_artifact_widget` (`0x125a4c`, line 800). Retail `0x562840` expands both
widget-helper calls, with different nested visibility decisions. Keep the real
helper before its caller and its retained claim at `0x5639e0`; no false inline
declaration, assertion, source-order trick or replacement override is needed.

Context `f394ad344803747dcb3d` exhausts six successfully scored states and
reproduces all six search-identity objects: direct-field/accessor snapshot
crossed with pasted-fenced, pasted-unfenced and canonical-unfenced first update.
All 55 scores are unchanged for the canonical call. The pasted-unfenced
negative controls lower only updateSlot from 100% to 99.1368%, whether or not
the artifact accessor is restored. Production adopts both positive boundaries,
candidate `b20261b99b0d0a9b31069e1f`, against unchanged
`646bd196a8d8203b4e71d54c`.

The six distinct runner identities do not imply six different instruction
streams: private label counters are deliberately not generalized away by that
metric. The stricter same-layout proof, `compare-coff-layout.py`, independently
shows all **200 raw sections**, **1678 relocation destinations**, and function
identities/positions unchanged between the untouched control and the selected
candidate, and again against production. This includes untracked bodies and
data, not just the 55 scored rows. The pasted-unfenced control fails that proof.

Retail review confirms all twelve CFG blocks and the ordered fourteen named
calls. The first expansion calls `setVisible` at `+0x6d`; the other visibility
sites expand it into `sendMessage`, including the second artifact expansion
at `+0x119`. The three remaining display-name relocation differences are
already claimed artifact/slot data, not new helper-boundary mismatches.
One depth-zero override is retired at unchanged object code and matching score.
The full delink/build passes all gates at **4074/4764 exact and 96.38%
linked/whole-image**. Only updateSlot's source hash changes in the matching
ledger; no score, MAX or HIST changes. The cleanliness bound drops to 216
depth-zero overrides.

### Creature-bank table owners and ordinary level reader

`generate-bank-table-owner-family.py` replaces two incorrectly file-scoped
`const int` tables and the reward enum adapter with the source-owned tables.
NB11's `guard_types` (type `0x5601`) and `reward_types` (`0x5602`) are mutable
`TCreatureType[11][5]` and `[11]` statics belonging to loader procedure
`0x7112c`. This is positive storage/type evidence, not a line-gap inference.
The hash-verified retail image puts their 55 and 11 dwords in writable `.data`
at `0x6702a0` and `0x67037c`. Preserve zero-initialized padding after guard
sentinels rather than inventing extra creature entries.

Six consumed creature names come from DC's actual enum: four added members
and two moved atomically from AI's separate enum into canonical TCreatureType.
The initial unscored opposite corner exposed the two duplicate names; that
manifest was repaired before continuing. Corrected context
`7db5f64e100e77ef33ce` reproduces both states across all **95 header consumers**
and **4117 scored functions**. Control `ed35dc233ed5555e0bfbe8a9` and native
candidate `24e6bf9987f4beff9cd1a45f` change only three tracked RMG scores:
quest-creature generation 100% → 99.7349%, loadTemplates 80.8461% → 80.8308%,
and writeMapHeader 77.8952% → 77.9030%. Their bodies' source hashes are
unchanged, so MAX/HIST retain all prior peaks. All three bank scores stay flat.

The stricter raw-object comparison passes **90 of 95 entire objects**.
Creature-bank moves the tables from `.rdata` into `.data`; all fourteen
emitted function bodies remain byte-identical. Three TUs (creaturetype,
spelldefs, herodefs) retain identical function bytes/named references but
shuffle anonymous-namespace BSS; **same-source reproduction also shuffles
those BSS layouts**, so do not attribute that variation to the enum edit.
RMG preserves all 441 function positions and section extents, with actual
body-byte changes confined to the same three scored functions. No object
rewriter or new scoring normalization is involved.

`verify-bank-table-owners.py` independently checks NB11 procedure ownership,
exact enum dimensions, emitted local-static mangled owners, writable section
placement and **all 66 dwords**. It passes on the selected object and production,
rejects the old file-static control, and rejects six non-mutating byte/order/
extent negative controls. Before the reader recovery, candidate/production
also match all 29 raw sections and 101 relocation destinations exactly.

DC's ordinary static `initialize_creature_bank_level` (`0x70fe0`, source
line 32) takes `type_creature_bank_level&` and `const vector<char*>&`; the
loader calls it at line 136 after installing the guard/reward types. Restore
that real body before its caller, not a false inline declaration or pasted
reader. `generate-bank-level-boundary-family.py` crosses four meaningful
column-cursor lifetimes plus the pasted control with unsigned/signed guard
indices. Retail's guard-copy `jl` positively supports the signed index.
Context `c601a370d5246188c67a` exhausts and reproduces all ten states/objects.
Selected `f9671c7898dc10b79eeb7727` uses the DC cursor from column two and
advances past each guard count before its zero test: **89.4550% → 97.5355%**.
Both other tracked bank functions remain 100%. The signed-index-only
negative control gives 88.8910%, demonstrating why an isolated score dip
does not reject a source fact. Production reproduces all 28 raw sections,
100 relocation destinations and function positions of the chosen object.

All fifteen named calls now agree with retail, including the recovered
string `_Eos` expansion, and all twelve branches agree. The remaining two
CFG size differences are the guard-copy inductions: VC6 forms a destination
minus source bias where retail keeps two cursors and a counter. A string
byte-store address also exchanges commutative operands. This is a measured
residual, not bank-TU closure or a request for a new suppression pragma.

`homm3.vc6.test_bank_level_boundary` imports both actual bodies, with only a
fixture entry counter added. At `-O0` and `-O2`, eleven numeric patterns across
four resource conditions check null/short input disposal, the retail threshold
of thirteen rows, all 44 level records, exact row and parsed-cell order, guard
sentinels, seven resources, artifacts and the reward count's signed-byte zero
test. Seven negative controls are rejected. The mock is a bounded behavioral
oracle, not proof of retail ABI or compiler inlining.

Full delinking/build passes at **4073/4764 exact, 96.39% linked fuzzy and
96.38% whole-image**. One bank checkpoint rises; no MAX resets or banked
RVA losses occur. The audit now has **63 unions** (37 source, 26 header),
including 39 remaining reconstruction adapters, and **221 inline overrides**.

### Skill-quest proposal lifetimes and six-fence boundary

The Complete-only skill-quest proposal at `0x56dad0` has no established
Dreamcast counterpart. Its current score is **83.2252%**, not the old 75.4324%
quoted beside its source. Retail CFG, named calls and byte-verified candidate
statements were inspected before these families. The input is a signed byte;
the current `const int&` binds a converted temporary, not the field itself.

`generate-skill-proposal-input-family.py`, context `32c06caa67f5495b6533`,
exhausts **36 states / 24 distinct objects**, with ten reproduced elites.
It crosses three input bindings, three output bindings, independent declaration
order and the first dialog loop's index signedness. None improves the caller
or any sibling. A native signed-byte reference reaches the same 83.2252%;
the signed dialog index required by retail's `jl` measures 82.9730%.
Output pointer cursors lose further. The byte-reference control has identical
raw section bytes but different generated EH function-label numbering, so
this is not claimed as a strict whole-object identity result.

`generate-skill-proposal-fence-family.py`, context `d65a9ca695d30ffc1d3c`,
exhausts all **64 subsets / 64 objects** of the six inherited fences, including
joint removals; ten elites are reproduced per generation. Every nonempty
deletion lowers only the proposal caller. The least-cost individual deletion
is the custom-string insert fence, **83.2252% → 82.2342%**; removing both
insert fences gives 78.7928%. All 118 other scored rows stay fixed.

`generate-skill-proposal-string-family.py` verifies that parent's snapshot
and reproduced control, then crosses owned versus lifetime-extended const
references for both unmodified returned strings with twelve paired/individual
fence masks. Context `c46dad8e3df70bd51249` exhausts **48 states / 36 objects**
and reproduces ten elites. These real string-lifetime alternatives change no
score; every deletion has the same cost as its parent. They do not recover a
removable fence. Of six differing displayed call names, four are byte-identical
shared vector COMDATs; only two are real string-destructor versus `_Tidy`
boundaries. An aggregate call count is not an additional six-site diagnosis.

These bounds do not prove that the six fences are original source or that
the whole function is unrecoverable. They rule out deletion alone, these
input/output bindings and these returned-string lifetimes in the measured
source context, without inventing helper bodies or changing normalization.

### AI combat container ownership and canonical melee

The inherited `type_monster_vector` subclass was a code-generation shim,
not the Dreamcast member type. DC's `creatures` is an actual
`std::vector<type_monster_data>`. Its `size()` is separate from
`type_AI_combat_data::get_total`: the latter's complete DC body at `0x2c6ac`
loads the `total_combat_value` member. Retail `0x427750` is the 33-byte vendor
vector-size body, not that game accessor; `0x4276c0` is the 135-byte native
vector copy constructor. Both claims now belong to their compiler-generated
STL owners. The class field is named `m_totalCombatValue`, and ordinary count
loops call the native vector's `size()`. Public begin/end also replace access
to vendor-private pointer members. Neither the vendored STL nor normalization
is modified.

Before that correction, `generate-general-melee-boundary-family.py` exhausted
**12 states / six objects**, all six reproduced, in context
`7e95efb5c8032fc9c974`. It crossed canonical versus pasted/pinned melee with
separate/joint zero guards and shared/branch-scoped ratios. Replacing only the
paste with the DC-proven ordinary call lowered chooseMelee from 90.9329% to
77.6098%; removing only its fence scored zero despite retaining the emitted
caller. Those results did not refute the source call at DC line 1346.

With native container ownership, the canonical call raises **chooseMelee,
doGeneralMelee and the two-side getEnchantmentValue to 100%**. One override
is retired. All 42 chooseMelee CFG blocks agree, and the retained general-melee
body also matches. Two displayed chooseMelee call-name differences are proved
shared bodies: its 38-byte vector destructor equals retail's widget-vector
destructor at `0x46a650`, and its three-byte `_Destroy` equals the artifact
instantiation at `0x404140`. They are not wrong helper calls.

The owner change has real collateral: initializeCreatures falls from 91.9548%
to 82.6704%, and the previously exact `_Unguarded_partition` body at `0x427c30`
is no longer emitted because it expands into the initializer. Its compiler-
generated claim and historical peak remain; a passing banked-RVA gate does
not mean that body is still present. `_Sort`, vector-size and vector-copy
remain exact. This must not be hidden as claim removal or a normalization fix.

`generate-ai-creature-initialization-family.py` exhausts/reproduces **nine
states / nine objects**, context `6de4e0a94f21173ed59e`: the force modifier's
real early initialization and three public sort-argument lifetimes. DC line
222 and retail's entry argument copy support the declaration initializer;
it recovers initializeCreatures to **83.4275%**. Direct calls, a vector
reference and named iterators score identically.
`generate-ai-classifier-boundary-family.py` verifies all nine parents and extends them with the
ordinary/inline, const/mutable and original/hoisted `getCatagory` interface.
Context `2027c66d848baf9eadfa` exhausts **72 states / 27 objects** and reproduces
ten elites per generation. Those declaration choices do not change scores;
the adopted ordinary const helper follows DC `0x2a52c` in its real source
position after wall-archery adjustment. All 72 emitted objects were checked:
none restores the missing partition body. The chosen non-elite was independently
recompiled and reproduced before adoption.

The selected initialization/classifier implementation and production agree
in all 73 raw ai_combat sections / 432 relocation destinations, and all 420
ai_player sections / 1963 destinations, including function identities and
positions. Both header consumers and all 176 scored rows are checked.
The full checkpoint has **4075/4764 exact, 96.39% linked and 96.38% whole-image**.
Only the initializer's source edit resets MAX below its old peak; HIST retains
91.9548%. The missing partition keeps its unchanged-source MAX/HIST at 100%.

`test-ai-melee-owner-boundary.py` extracts the actual accessor and ordinary
melee body. At native `-O0` and `-O2`, twenty independent vector-count/total
cases and 169 melee pairs cover zeroes, ties and float rounding around 2^24.
Mocked kill/damage/final-value calls check operands and ordering. Five deliberate
faults are rejected. This is a bounded behavioral check, not a VC6 ABI oracle.

The final interface cleanup follows raw NB11 types rather than inherited
comments: class `0x5a07` / field list `0x5a4c` names `current_hero`,
`current_army`, `can_cast_spells`, `wall_archery_penalty` and `wall_speed_limit`.
Those names now own the corresponding `m_` members. chooseMelee's type
`0x5a2b` returns primitive `0x20`, unsigned char; its byte-return ABI and both
byte-consuming callers agree with retail. This cleanup preserves every raw
byte and relocation destination in both consumers, under that one explicit
function rename (77 ai_combat sections / 462 destinations).

Conversely, the same class's constructor method list `0x5a0f` gives its copy
constructor attributes `0x003`, not the `0x103` compiler-generated attributes
on assignment/destruction. Preserve the explicit memberwise copy constructor;
its native vector member has a separate retained STL owner. A sole line row
at a caller's closing source line is not enough to declare it implicit.

### AI mass damage, Familiar predicate and value-wrapper boundaries

DC `cast_mass_damage_spell` (`0x2ac58`, lines 747..760) is one body with two
source calls, not caller-specific copies. Crucially, line 758 overwrites the
running damage value with `take_damage`'s returned capped value; line 759
subtracts that value from total combat value. Both retail `castSpell` expansions
at `0x425bd0` corroborate this assignment (ESI/EDI from EAX). The old cloned,
fenced loops carried the uncapped sum to the next creature, which changes
behavior when accumulated damage exceeds a stack's total. Separate vector
subscripts across the opaque spell-damage call and zero initialization before
mastery lookup are also corroborated by both builds.

`generate-ai-mass-damage-boundary-family.py` crosses these three facts with
cloned/canonical, inherited-inline/ordinary and fenced/unfenced boundaries.
Context `ba7a1cfd8351a6b35d25` exhausts **48 states / 48 objects**, with ten
reproduced elites and complete 176-row, two-TU vectors. A high score from the
uncapped-dataflow control is not an eligible reconstruction. The corrected
ordinary single helper scores 88.5664% with the first inherited fence, and
79.8398% with both old fences removed.

DC `cast_spell` line 1047 calls the ordinary const `has_creature` predicate
(`0x2ab3c`, source line 694, before mass valuation). Retail's Familiar scan
corroborates its type/positive-count tests and single mana update after success.
`generate-ai-familiar-boundary-family.py` verifies the 48-parent manifest,
snapshots and reproduced elites, then crosses that canonical boundary with
all parents. Context `dbe7501806f1d9581f9e` exhausts **96 states / 96 objects**,
with ten reproduced elites per generation. Restoring the predicate raises the
correct unpinned mass variant to **86.7910%**, while the corrected one-pin
variant stays 88.5664%. It changes no other score. The adopted all-correct,
unpinned corner is `f236fbd6dcc860f3c9c8c5da`; production matches its 75 raw
ai_combat sections / 443 relocation destinations and the complete ai_player
object. Both old mass fences and the false helper clone are removed.

The remaining castSpell difference is a real nested-boundary mismatch, not
the old allocator-only diagnosis. Its first mass expansion expands
`getSpellDamage` where retail calls it and retains `takeDamage` where retail
expands it. The resulting extra branches/frame slot explain the source diff's
first prologue mismatch. Retail keeps both calls in the second mass expansion.
The current CFG has 100 blocks / 60 conditionals against 92 / 55 in retail;
both have four returns. No additional pragma or duplicate body is introduced.
The previous 93.0273% score used the incorrect dataflow and survives in HIST.

DC also proves const receivers for both enchantment-valuation overloads and
the two-side mass-valuation wrapper. Correcting all declarations/definitions
together is strictly byte-neutral under the one emitted function rename:
75 ai_combat sections / 443 relocation destinations, plus all of ai_player.
The full build migrates the existing enchantment claim at its same RVA.

`generate-ai-value-helper-family.py` restores a different canonical boundary:
the by-value `min`/`max` wrappers already owned by `homm3_minmax.h`. DC
`includes.h:97,114` and actual AI call relocations name those wrappers; their
callee selectors return const references to the wrappers' still-live arguments.
The local templates incorrectly returned references to their own by-value
parameters. Retail's two operand homes do not prove that invalid declaration.
Context `5468aea93d306459a4f1` exhausts/reproduces **eight states / eight objects**,
also checking ordinary versus inherited-inline mass/enchantment valuation
definitions. The ordinary definitions are score-flat. Canonical min/max
preserve every score except getResurrectionValue's 100% → 92.9310% dip.
Production reproduces the all-canonical corner's 77 sections / 462 relocation
destinations and the unchanged ai_player object.

`generate-ai-resurrection-value-family.py` addresses that precise residual:
retail loads the selected scalar before multiplying, while a single return
expression multiplies through a retained address. DC lines 99..102 support
separate scale/divide, cap and result stages. Context `5de5d8835a813ce9959c`
exhausts **six states / three objects**, all three reproduced. A fresh capped-
value local (`a25b0998f922c6853ac8d8cb`) restores **100%**; reusing the earlier
spell-value local, including separate DC-order stages, remains 92.9310%.
All other 175 scores remain fixed. Keep the real value-returning wrappers.
Production reproduces the selected 77-section / 462-relocation object; retail
review confirms nine CFG blocks, four named calls and zero instruction deltas.

`test-ai-mass-damage-boundary.py` extracts both the actual canonical mass loop
and `takeDamage`; at native `-O0` and `-O2`, 328 stack-array cases check reverse
order, the capped loop carry, hero/damage arguments, stack counts and total
combat value. Six deliberate faults, including the inherited uncapped carry,
are rejected. `test-ai-resurrection-value.py` checks 3456 bounded input cases,
call order and nonmutation with five negative controls. The spell/hero services
are mocked; these are behavioral tests, not VC6 ABI or inlining proofs.

The final full build has **4075/4764 exact and 96.38% linked/whole-image**.
All three ai_combat inline overrides are gone; the global census is **218**.
The independently measured signed skill-dialog index/native-byte binding is
also adopted at 82.9730%, with its 83.2252% HIST preserved. All source-backed
name/signature changes are regenerated by the normal labels/delink pipeline.

The search never writes authored source, CUR, MAX or HIST. Different function
implementations must not be banked under an old source hash. Review a retained
candidate, apply the actual C++ change, then run `homm3 build` to regenerate the
normal checkpoint and README. This preserves `CUR <= MAX <= HIST` and keeps
historical peaks separate from the current implementation's MAX. Completing a
family or getting every currently tracked row exact does not prove whole-TU
source completeness.
