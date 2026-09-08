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
