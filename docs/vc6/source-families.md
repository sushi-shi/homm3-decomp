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

The Voronoi return family separates returned-value lifetimes in `buildVertices`
from construction in its five canonical arithmetic operators. Its follow-up
accepts a completed vertex checkpoint, checks the parent source snapshot and
reproduced definitions, and carries all ten retained parents. `--control`
reduces the family to four caller/scale-operator controls for causal isolation:

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-voronoi-vertices-family.py build/rmg-voronoi-vertices.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-voronoi-return-family.py build/source-families/VERTEX_CONTEXT/checkpoint.json build/rmg-voronoi-return.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-voronoi-return-family.py build/source-families/VERTEX_CONTEXT/checkpoint.json build/rmg-voronoi-control.json --control
```

Run the vertex search before producing the follow-up. A changed authored parent
is rejected, not silently rebased to the checkpoint. The circumcenter oracle
checks integer division order and shared half-edge flags with four negative
controls; all three return constructions use the actual point/vector types.

The ray reload and recurrence generators target the two independently scheduled
loads at `traceBranchEnd`'s neighbour-loop exit. They preserve the canonical
coordinate lookup and stepping algorithm while varying loop, query-receiver,
error-update and previous-point lifetimes. Their reduced fixture checks 17,150
cases per form against a closed-form lattice ray, including zero directions,
two levels, seven obstacle patterns, and four deliberately wrong controls:

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-ray-reload-family.py build/rmg-ray-reload.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-ray-recurrence-family.py build/rmg-ray-recurrence.json
PYTHONPATH=scripts python -m unittest homm3.vc6.test_rmg_voronoi_vertices
PYTHONPATH=scripts python scripts/experiments/test_rmg_ray_reload.py
```

The noise-midpoint family checks coordinate ownership, region-copy lifetime,
and center-sample capture without changing quadrant order or vector APIs.
Its native oracle derives expected samples from a 3x3 lattice, checks all
3,136 origin/extent combinations and existing-vector-prefix preservation,
and rejects four wrong midpoint/sample/degeneracy/variation controls:

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-noise-midpoint-family.py build/rmg-noise-midpoint.json
PYTHONPATH=scripts python scripts/experiments/test_rmg_noise_midpoint.py
```

`build/source-families/<context>/` retains the input, snapshot, candidate trees,
objects, complete score vectors, failures, generation summaries and checkpoint.
Re-running with the same inputs resumes; a new source, profile, toolchain,
target, normalization implementation or seed creates another context. An
exhausted family calls for a new evidence-based family, not endless resampling.

The canonical map-accessor family varies row/index lifetime, multiplication
and row-addition operand order, and pointer-result binding. Unlike a local
caller edit, this header experiment scores all seven consuming units, including
the single-selection dialogs and tiles. The scalar inline declaration and
ordinary position-overload delegation remain intact:

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-map-accessor-family.py build/rmg-map-accessor.json
PYTHONPATH=scripts python scripts/experiments/test_rmg_map_accessor.py
```

The independent pointer-offset oracle checks 882 in-bounds coordinates per
form across square/non-square dimensions and both levels. Four deliberately
wrong height, width, level and X controls must fail. Its reduced map-item
fixture tests indexing semantics, not retail object layout.

The completed 60-state family produces eleven distinct objects and ten
reproduced retained candidates. None changes `commitTreasureGroup`'s four
operand bytes. Seventeen other functions move, all in `rmg.cpp`; six have
partial-score gains, but no new exact function is found. The existing
`repairWaterZoneBorders` exact match falls to 95.9863% in several states.
The other six units are unchanged. No source alternative was adopted:
the hypothesized row/result lifetimes do not explain the target expansion.

The connection-owner family targets `canConnect`'s ECX/EBX exchange after
the signed distance conversion. Six slot/value/reference bindings, five sum
lifetimes and two minimum forms make 60 states. Its follow-up retains the
ten distinct reproduced parents (substituting unchanged source for one
object-identical neutral parent) and adds 50 const-value/copied-temporary forms:

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-connect-owners-family.py build/rmg-connect-owners.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-connect-owners-family.py build/rmg-connect-snapshots.json --parents-from build/source-families/OWNER_CONTEXT/checkpoint.json
PYTHONPATH=scripts python scripts/experiments/test_rmg_connect_owners.py
```

Run the first manifest before generating its follow-up. Source/header snapshots,
input options, reproduced bodies, and the neutral parent's code identity are
checked before reuse. Each batch scores all of `rmg`, producing ten and fourteen
distinct objects. All sibling scores remain unchanged. A reference to the other
slot's size reaches 97.7922% from 96.4286%, but splits retail's single field load
into an address addition plus load and exchanges the sum's LEA operands. Copied
scalar temporaries do not resolve this, so the game source remains unchanged.
The native oracle checks 122,018 size/distance/level combinations per form,
plus same-object calls, using integer square root independently of candidate
`sqrt`; wrong level, strictness, minimum and distance controls must fail.

The painter-area family follows the earlier direct-dimension matrix with
the existing ordinary `getWidth`/`getHeight` boundaries used by terrain
transitions. Five dimension bindings, four accessor/member combinations and
three product lifetimes produce 60 states and 24 distinct objects. Ten
retained candidates reproduce; no tracked peak improves and none is adopted.
All 29 constructor CFG blocks already agree, but retail's width reload and
height-load schedule are still unresolved:

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-painter-area-family.py build/rmg-painter-area.json
PYTHONPATH=scripts python scripts/experiments/test_rmg_painter_area.py
```

The native fixture uses the actual accessor bodies and checks dimension,
query-count, area and initialized-cell semantics for 49 dimension pairs per
form. Four negative controls change height, area, strength or query count.
It deliberately does not inspect the packed cell's initially indeterminate bits.

The later `generate-rmg-map-base-family.py` names the scalar lookup's base
pointer independently of its row/index calculation. Value/reference bindings,
offset lifetimes and scalar/point dimension captures produce 59 distinct
source states and 47 code identities, with ten reproduced finalists. All
seven consumers are scored. Underground decoration spans 96.8807–99.7982%
without improving its remaining initial base-load schedule. Nine other RMG
functions have partial gains, but no new exact function appears; several
alternatives disturb the exact island or water-border callers. No accessor
change is adopted. The combined native accessor oracle checks 118 old/new
forms against 882 coordinates each and rejects five wrong dimension, level,
coordinate and mutated-base controls. It verifies owner fields remain intact.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-map-base-family.py build/rmg-map-base.json
PYTHONPATH=scripts python scripts/experiments/test_rmg_map_accessor.py
```

The Voronoi construction family preserves the four perimeter factories and
diagonal while varying corner ownership, edge declaration lifetimes and
diagonal endpoint bindings. Its 60 states produce 24 distinct objects and
ten reproduced finalists; the constructor moves from 80.5217% to 80.7057%,
but the fifth factory still expands instead of matching retail's retained call.

The connector follow-up carries those ten parents into 48 combinations of a
provisional ordinary shared connection member, its endpoint/result bindings,
definition placement and fan-loop lifetime. It never marks the helper inline
or pastes the canonical factory/splice bodies. All seven header consumers are
scored. The 59-state batch produces 35 objects, bringing the constructor to
90.6722% and site insertion to 47.6667%. The fifth factory is now retained, but
the constructor calls both connector splices where retail expands the first;
the fan expands both where retail retains them. That remaining site-specific
disagreement is not captured by aggregate call counts.

The result-lifetime follow-up carries ten reproduced connector parents into
pointer assignment, result-reference, predecessor and twin-result bindings.
All 121 states compile into 31 distinct objects without improving those peaks.
The separate 72-state factory family produces 24 distinct objects: a returned
pointer bound by const reference raises the original constructor to 82.6120%,
while the factory itself stays at 90.9600%. The combined family tests all
100 pairs of factory/connector parents plus their independent controls.
All 121 combined states compile into 110 distinct code identities; ten
finalists reproduce. Neither peak improves, and the best combined candidates
retain the original factory body. These are experimental frontiers, not
adopted matching source.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-voronoi-construction-family.py build/rmg-voronoi-construction.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-voronoi-connector-family.py build/source-families/CONSTRUCTION_CONTEXT/checkpoint.json build/rmg-voronoi-connector.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-voronoi-connector-family.py build/source-families/CONNECTOR_CONTEXT/checkpoint.json build/rmg-voronoi-connector-lifetimes.json --lifetimes
PYTHONPATH=scripts python scripts/experiments/generate-rmg-edge-factory-family.py build/rmg-edge-factory.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-voronoi-combined-family.py build/source-families/FACTORY_CONTEXT/checkpoint.json build/source-families/LIFETIME_CONTEXT/checkpoint.json build/rmg-voronoi-combined.json
PYTHONPATH=scripts python scripts/experiments/test_rmg_voronoi_construction.py
PYTHONPATH=scripts python scripts/experiments/test_rmg_edge_factory.py
HOMM3_VORONOI_MANIFEST=build/rmg-voronoi-combined.json PYTHONPATH=scripts python scripts/experiments/test_rmg_voronoi_connector.py
```

Run each parent search before generating its descendant. Parent source/header
snapshots, input options and reproduced source trees must agree. The native
checks use the actual point/edge/diagram classes, factories and splice bodies:
an explicit ten-edge initial graph, 100 aliased/non-aliased connection pairs,
and full site insertion with eight cyclic input orders and duplicate sites.
Ring links, ownership, planarity and the triangulation edge count are checked
independently; incorrect endpoints, zones and splice operations are rejected.
The factory oracle also checks 40 consecutive paired allocations per form,
including vector growth and preservation of every prior owned pointer.

The independent edge-removal family crosses four structures for each of its
two unsigned searches, shared/separate indices and direct/named erase
iterators. All 64 states compile into eighteen distinct objects, with ten
reproduced finalists and every tracked score unchanged. In particular,
`removeEdge` stays at 76.1539%: retail retains the second splice inside
`detach`, while the candidate expands both. The existing eighteen blocks and
ten branches already agree; public erase and search-loop lifetimes do not
explain that helper decision. No source form is adopted.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-edge-removal-family.py build/rmg-edge-removal.json
PYTHONPATH=scripts python scripts/experiments/test_rmg_edge_removal.py
```

The native removal oracle checks 82 cases per form, removing either half of
every pair from the initial subdivision and from appended isolated pairs.
An independent graph bypass and filtered ownership list check both rings,
surviving pointer order and deletion order. Three omitted/partial-detach
controls must fail without relying on invalid erase or double deletion.

The edge-side family tests an ordinary shared point/edge predicate in lookup
and site legalization. The published
[Graphics Gems IV implementation](https://github.com/erich666/GraphicsGems/blob/master/gemsiv/delaunay/quadedge.C)
suggested that helper boundary; it does not prove HoMM3 names or declarations.
Retail `locate` (0x5fd6b0) already agreed on control flow but spilled an
endpoint in its first orientation, growing its frame from eight to sixteen
bytes. The family preserves the canonical integer orientation helper and
crosses cyclic argument order, endpoint bindings, query value/reference
ownership and integer/byte predicate results. No inline directive is added.

All 61 states compile into 31 distinct code identities, with six exact lookup
forms and ten reproduced finalists. The selected by-value query and named
twin pointer restore all 215 unmasked bytes, with no relocations. Its three
lookup calls and the legalization call use the same ordinary file-local
predicate. `addSite` moves from 45.3869% to 45.4167%; every other tracked
score is unchanged. The separate 47.0387% insertion peak does not preserve
exact lookup. The adopted pair passes the full VC6 build, and the verified
/Z7 source comparison agrees on every lookup block.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-voronoi-right-of-family.py build/rmg-voronoi-right-of.json
PYTHONPATH=scripts python scripts/experiments/test_rmg_voronoi_right_of.py
HOMM3_VORONOI_MANIFEST=build/rmg-voronoi-right-of.json PYTHONPATH=scripts python scripts/experiments/test_rmg_voronoi_connector.py
```

The determinant oracle uses actual point/edge declarations and constructors.
An independent 64-bit shoelace determinant checks 117,649 coordinate triples
per predicate, including collinear/duplicate endpoints and input aliases;
three wrong sign/strictness/endpoint controls must fail. The graph oracle
also exercises every manifest form through complete insertion with eight
cyclic input orders and duplicate-site rejection. After adoption, the
generator recognizes the selected child as its unchanged control and retains
the former direct-orientation body as a negative codegen control. Old search
snapshots remain tied to their original source; they are not silently rebased.

The segment family tests the separate four-product line equation visible in
`addSite` at +0x8c:+0xc1. This differs from the canonical orientation's
two-product translated-vector expression. Five coefficient lifetimes, four
query/endpoint ownership signatures and three distance-guard forms produce
61 states including the unchanged control. All compile into 41 distinct code
identities, with ten reproduced finalists. The 47.6339% leader still retains
the newly proposed line predicate where retail expands its calculation, so
the score improvement alone does not validate that boundary.

The `--line-object` follow-up separates ordinary line construction and
membership methods. Its 61 states produce 25 distinct objects and ten
reproduced finalists, reaching 47.8601%. Every changed state retains both
new method calls; neither occurs in retail. Neither family is adopted, and
the canonical 45.4167% source remains unchanged. These are bounded failures
of the tested boundary models, not evidence for an inline directive.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-voronoi-segment-family.py build/rmg-voronoi-segment.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-voronoi-segment-family.py build/rmg-voronoi-line-object.json --line-object
PYTHONPATH=scripts python scripts/experiments/test_rmg_voronoi_segment.py
HOMM3_SEGMENT_LINE_OBJECT=1 PYTHONPATH=scripts python scripts/experiments/test_rmg_voronoi_segment.py
HOMM3_VORONOI_MANIFEST=build/rmg-voronoi-line-object.json PYTHONPATH=scripts python scripts/experiments/test_rmg_voronoi_connector.py
```

Both forms pass the full insertion oracle and an independent segment oracle:
117,649 coordinate triples per form, with collinearity plus projection bounds
instead of the candidate's two distance bounds. Degenerate segments, endpoint
aliases and preserved inputs are included. Five wrong endpoint, bound and
line-equation controls must fail. Arithmetic stays inside the proven
non-overflowing map-coordinate domain; no floating-point normalization from
the external reference is imported into the integer game model.

The circle-expression family preserves all four by-value points and the
canonical orientation calls, comparing named area values with embedded calls,
equivalent determinant groupings, square-operand order, signed widening on
either product operand and predicate-result lifetimes. Its sixty states
produce twenty distinct code identities and ten reproduced finalists. Scores
span 33.2202%–45.4167%, without improving the unchanged caller or recovering
the missing orientation calls. Only `addSite` varies in all three of these
segment/line-object/circle batches; exact lookup and every sibling score hold.
No game implementation or MAX is changed by these experiments.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-voronoi-circle-family.py build/rmg-voronoi-circle.json
PYTHONPATH=scripts python scripts/experiments/test_rmg_voronoi_circle.py
HOMM3_VORONOI_MANIFEST=build/rmg-voronoi-circle.json PYTHONPATH=scripts python scripts/experiments/test_rmg_voronoi_connector.py
```

An independent four-by-four determinant oracle enumerates all 24 permutations
instead of reusing the candidate's triangle decomposition. Every form passes
8,609 cases: all small-grid point quadruples, degenerate and cocircular cases,
and deterministic wider map-coordinate samples that exercise 64-bit products.
Four wrong sign, strictness, magnitude and omitted-area controls must fail.
The full insertion oracle also passes for every circle manifest form. These
semantic checks do not establish the missing compiler/source boundaries.

The prototype-loader family targets `loadObjectPrototypes` (0x536200).
Its initial 99.5878% body agrees with retail on all 34 blocks and call
positions, but exchanges the scan index and derived byte-offset homes.
Sixty index/version/record-lifetime forms produce six distinct code identities
and six reproduced finalists. Sharing the scan counter with the outer
sorting loop reaches 99.6622% and restores every differing stack displacement.
The 37-state counter-signedness follow-up preserves all six parent objects
without improving the peak. The unsigned comparison alone does not prove the
counter's type: `vector::size()` causes unsigned comparison for either form.

The shared-counter source and read-only record reference first change only
this loader's score. A third, 36-state receiver/type/scope family resolves
the remaining nine register-operand bytes: all six direct-subscript forms
reach 100%, with five distinct code identities and five reproduced finalists.
The minimal direct-read form is also compiled independently; its complete
score vector, code identity and source hashes reproduce. All 428 normalized
bytes equal retail, with the six relocation sites/types aligned. The named
view still distinguishes pooled data labels and the pointer-vector insertion
specialization; these are not differing instructions or call positions.
Allocation-time table reloads, reference ownership, prototype-only sorting
and the later rule/progress calls remain unchanged. The adopted source passes
the full VC6 build and changes only the loader's tracked score, to 100%.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-prototype-loader-family.py build/rmg-prototype-loader.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-prototype-loader-family.py build/rmg-prototype-loader-signed.json --parents-from build/source-families/LOADER_CONTEXT/checkpoint.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-prototype-loader-family.py build/rmg-prototype-loader-receivers.json --receivers
HOMM3_PROTOTYPE_LOADER_MANIFEST=build/rmg-prototype-loader-signed.json PYTHONPATH=scripts python scripts/experiments/test_rmg_prototype_loader.py
```

Parent reuse checks source/header snapshots, rendered source, repeated scores
and code identities. After adoption, the generator recognizes the selected
child as its unchanged control; historical checkpoints must not silently
rebase. The exact child is outside the original 60 forms, so the initial
generator now returns 61 forms; the receiver family still has 36. The native
oracle checks 180 scenarios per form: five map versions,
six subtype patterns, three remapping tables and two progress outcomes.
It imports the actual category enum, consumed record field declarations,
properties declaration and constructor, with reduced host-only table/base
layouts. A class-specific host allocation hook records reference ownership;
sorting must move prototype pointers, not their owning records. An opaque
rule reader replaces the progress pointer before its final query.
Six wrong version/subtype boundaries, remapping, ownership and progress
controls must fail. The contract assumes a nonempty creature bucket, as the
retail unsigned `size()-1` loop does; empty-bucket behavior is not claimed.
These host checks are semantic evidence, not x86 layout evidence.
After adopting the exact form, all 16 experiment tests pass, including all
61 initial loader forms; the rebased 36-state receiver oracle passes as well.
The verified source-labelled diff reports every aligned block identical.
The three RMG units now have 247 currently exact and 253 MAX-exact tracked
functions out of 363; this loader result is not whole-TU closure.

The neighboring prototype selector (`0x546040`) does not share that solution.
`generate-rmg-prototype-receiver-family.py` crosses six property/prototype
receiver forms, five category lifetimes and two scan-counter types. All
60 states compile into 16 distinct code identities, spanning 74.1282–99.6581%.
Its `--returns-from CHECKPOINT` frontier retains the ten reproduced parents
and the unchanged source, then tests named selected pointers/references,
indices, random values and candidate counts. This second 60-state batch
produces 47 code identities spanning 76.4615–99.6581%. Ten finalists reproduce
in each batch; neither moves any other tracked function. No selector source
alternative is adopted. All 17 blocks and branch destinations already agree;
the ESI/EDI terrain/range exchange remains unresolved.

The return-frontier generator verifies full source/header snapshots, all
60 scored records and each retained parent's repeated choices, score vector,
code identity and rendered-source hashes. It never reuses scores after an
authored change. `test_rmg_prototype_polish` exercises the prior 120 forms,
all 60 receiver forms and every selected optional frontier form, covering
ordered candidates, empty ranges, invalid checked-mask indices, exceptional
cleanup paths and exactly one random draw on successful selection. Five
wrong subtype, water, mask, random and candidate-order controls must fail.
The fixture supplies public STL semantics and reduced owner records, not
retail layout or original class declarations; VC6 remains the byte verdict.
Both tests pass with the receiver family and with the return frontier.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-prototype-receiver-family.py build/rmg-prototype-receivers.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-prototype-receiver-family.py build/rmg-prototype-returns.json --returns-from build/source-families/RECEIVER_CONTEXT/checkpoint.json
HOMM3_PROTOTYPE_SELECTOR_MANIFEST=build/rmg-prototype-returns.json PYTHONPATH=scripts python -m unittest homm3.vc6.test_rmg_prototype_polish
```

The placement-rule reader (`0x536560`) has one surplus count-one push at its
rule append. Fresh retail comparison aligns all 62 blocks' branch destinations
and all stack homes; B11 alone has an extra instruction. No Dreamcast procedure
was found. `generate-rmg-rule-reader-family.py` first crosses five append
receiver forms, four terrain-array bindings and three tail-fill forms. The
60-state population produces 31 code identities spanning 92.6314–99.7862%,
with ten reproduced finalists. The `--direct` follow-up tests twelve direct
single-insert variants without a named end iterator plus the unchanged control:
13 scored states, eight code identities, eight reproduced finalists, and a
93.5010–99.7862% range. The minimal direct-insert body scores 98.0754% and
still retains count-insert, so changing the public source call alone does not
recover the retail boundary.

The `--constructors` family independently tests the three scalar vectors'
default, zero-count and zero-count/value constructors, crossed with push-back
versus single insertion for the rule. Its 54 states produce thirty code
identities spanning 87.2322–99.7862%, with ten reproduced finalists. None of
the three finite families raises the reader or another function. Some first
population siblings lower `writeMapHeader` from 77.9030% to 77.8952%; neither
later family changes any sibling score. No game-body alternative is adopted.
The library templates and implicit rule special members remain canonical.
An audit of all 127 emitted call sequences distinguishes the boundary from
the score: twelve constructor states retain the correct single-insert overload,
at 90.4929–92.0754%. These have exactly two zero-count vectors and keep
`push_back` for the rule. The highest form defaults `objectTypes` and constructs
`terrains`/`subtypes` with zero counts. It introduces two `operator new(0)`
and `_Ufill` calls absent from retail and grows the frame by four bytes.
Those concrete contradictions reject the constructors, not merely their lower
scores. The other 115 trials still call count-insert. The inlining decision
is reachable, but these sources do not reconstruct the whole retail function.

`test_rmg_rule_reader.py` compiles all 124 distinct source bodies across these
families. An independent row-value reference checks all ten terrain scores,
both full score matrices, object-type remapping, the lowest recommended terrain
and last matching subtype rule. Its 432 scenarios per body include zero to
seven rows, empty/space/end-of-file terminators, unrelated prototype groups,
three mapping tables, six mask patterns and duplicate rules with precedence.
Nine wrong terminator, index, terrain-tail, adjacent/blocked matrix, remapping,
subtype, selected-rule and resource-disposal controls must fail. The fixture
imports the rule declaration and consumed object fields, terrain constants and
trait-count enum; spreadsheet/base/property owners are reduced host models,
not x86 layout evidence. The input contract is a fresh generator with valid
rule category/terrain indices and complete numeric rows. The reader's old VC6
for-initializer scope is adapted only in the host test, never in matching input.
The oracle establishes parsed values and binding behavior, not VC6 allocation
traces or allocation-failure behavior; the retail call audit is required too.
All seventeen experiment tests pass at this checkpoint.

The later `--empty-initializers` frontier crosses default versus copy-initialized
empty temporaries for the three scalar vectors with shared versus temporary
allocators. A proposed parenthesized direct-temporary form failed VC6's
opposite-corner smoke check, so that 54-state population was never run.
The repaired finite family scores all sixteen states: eight code identities,
eight reproduced finalists, 99.0937–99.7862%, and no sibling movement.
All sixteen still call count-insert. Native acceptance of the rejected C++
spelling was not treated as evidence that VC6 could compile it.

`--rows` tests five leading-character bindings, four first-pass row receivers
and three row-counter lifetimes. The character is compared only with space
and zero; row storage stays spreadsheet-owned. The 60 states produce sixteen
code identities spanning 85.3585–99.7862%, with ten reproduced finalists and
no sibling movement. Every emitted reader still calls count-insert. Neither
new family changes the game body. Together the five completed families have
203 scored states containing 198 distinct source bodies; their full native
value/binding oracle passes, including all nine broken controls. The failed
smoke population is excluded from those counts.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-rule-reader-family.py build/rmg-rule-reader.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-rule-reader-family.py build/rmg-rule-reader-direct.json --direct
PYTHONPATH=scripts python scripts/experiments/generate-rmg-rule-reader-family.py build/rmg-rule-reader-constructors.json --constructors
PYTHONPATH=scripts python scripts/experiments/generate-rmg-rule-reader-family.py build/rmg-rule-reader-empty.json --empty-initializers
PYTHONPATH=scripts python scripts/experiments/generate-rmg-rule-reader-family.py build/rmg-rule-reader-rows.json --rows
PYTHONPATH=scripts python scripts/experiments/test_rmg_rule_reader.py
```

`generate-rmg-zone-fit-family.py` reopens `canPlaceZone` (`0x53ad60`) from
its remaining B12 arithmetic mismatch. All fifteen blocks' control-flow
edges, eleven branches, both math calls and stack homes agree; retail adds
one register-to-register copy before the y subtraction. Earlier all-int
subtraction/square and declaration families had not recovered it.

The first 60 states cross five returned-position lifetimes, four signed
int/long difference-type pairs and three distance-result forms. They produce
six code identities spanning 91.4953–98.0374%, with six reproduced finalists.
`--displacements` preserves the complete three-coordinate getter copy and
then uses an existing `TPoint` or `TRmgVector` value for the two differences.
Constructor/default-assignment, subtraction order and scalar/in-place square
forms give 44 distinct alternatives plus the original control: 45 scored
states, six code identities, six reproduced finalists, 81.9065–98.0374%.
Neither family changes another tracked score or improves the predicate.
No shared type, constructor, getter, math helper or game body is changed.

`test_rmg_zone_fit.py` compiles all 104 distinct source bodies with copy
elision disabled. It imports the actual coordinate classes, consumed slot
and zone fields, getter, template-kind and town enums. Its reduced owner
models do not assert retail layout. An independent binary integer-root
reference checks admission, floor-distance thresholds, exact ordered sqrt
inputs and preservation of zones, slots and the owner vector. Each body
runs 23,040 bounded scenarios including empty/self/same-id/different-level
neighbors, negative coordinates and IDs, zero radii, player/nonplayer kinds,
underground town restrictions and levels -1 through 2. All arithmetic stays
within signed 32-bit range: host `long` width is not treated as VC6 ABI proof.
Seven wrong underground/alignment/level/identity/square/threshold controls
must fail. The first radius set did not distinguish 70% from 80% spacing;
a sum-of-radii-nine, distance-seven case fixes that oracle coverage gap.
The expanded oracle passes. The extra retail move remains unresolved.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-zone-fit-family.py build/rmg-zone-fit.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-zone-fit-family.py build/rmg-zone-fit-displacements.json --displacements
PYTHONPATH=scripts python scripts/experiments/test_rmg_zone_fit.py
```

### Map-size return ownership

`generate-rmg-size-return-family.py` couples twelve base getter forms with
five adapter returns. The base forms cross six construction/return lifetimes
with direct dimensions versus const-reference-bound unsigned temporaries.
Both adapter ICF owners change atomically, preserving the single virtual query
and the canonical grid constructors. Neither Complete-only getter has a mapped
Dreamcast counterpart. Earlier isolated return spellings are controls, not
newly discovered alternatives.

All sixty states compile into 36 code identities, with ten reproduced finalists.
The base getter remains 97.5556%; the adapter spans 84.6471–85.1765%, without
exceeding its unchanged source. Only the header writer moves collaterally,
77.8952–77.9030%. No game-body change is adopted. Retail's hidden-result load
and interleaved return-copy scheduling remain unexplained by these lifetimes.

The native C++98 oracle imports the actual point/grid classes and disables
copy elision. Every paired form passes 49 dimension pairs, including signed
extremes converted to unsigned, direct base calls and both adapters. A scripted
virtual map mutates dimensions after capture, checking returned-value ownership
and exactly one query. Wrong component order, corrupted returned data and a
double query must fail. Reduced owner classes test semantics, not retail ABI.
The final full VC6 build passes with no ledger raises or source-hash resets;
all 26 experiment regressions pass, including this new return oracle.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-size-return-family.py build/rmg-size-returns.json
PYTHONPATH=scripts python scripts/experiments/test_rmg_size_returns.py
```

### Junction preparation coordinate ownership (0x5446a0)

`generate-rmg-junction-prepare-family.py` starts from three coordinate
constructor calls absent from retail and a 0x48 frame versus 0x34. No
Dreamcast counterpart is mapped. Four first-coordinate forms, five
predecessor/final-flood constructions and three scan-coordinate lifetimes
produce sixty states and 57 code identities, spanning 65.0140–77.5209%.

Three 60-state frontiers retain ten reproduced parents each, then test
value/reference bindings, entrance construction and predecessor/cost snapshots.
They produce 34, 46 and 53 code identities without exceeding 77.5209%.
Across all four batches there are 240 scored states, 207 distinct sources
and 157 code identities. Complete snapshots, source hashes, rendered trees,
choices and repeated score/code identities are checked before parent reuse.

The selected first-stage child default-constructs and fills the first entrance,
walk predecessor and final flood coordinate; the captured position also carries
the reset scan. All three surplus constructor calls disappear. The canonical
constructor, reset helper, flood and connection interfaces remain unchanged.
The child's full score vector, code identity, choices and source hashes
reproduce before adoption, and no sibling score moves. Some other variants
lower the unchanged-source header writer to 77.8952% from 77.9030%.

The adopted source passes the full VC6 build at 77.5209%. All three named
calls and relocations agree: flood, connect, flood. Its 0x24 frame, 27 versus
26 blocks and register allocation remain unresolved; the smaller frame is
not evidence of closure. Source-labelled comparison verifies the first
remaining mismatch at the entry frame. After adoption, the generator recognizes
only this reviewed child as unchanged and reconstructs the historical controls;
old parent checkpoints must reject the changed source instead of rebasing.
That rejection is checked at manifest admission after adoption. All 27
experiment regressions pass at this checkpoint.

The native oracle compiles actual coordinate and packed-cell types, the reset
helper, getter and scalar lookup. Flat-cell enumeration independently checks
reset coverage, while scripted floods provide acyclic paths to known endpoints.
Opaque connections mutate the source level and append an entrance to expose
incorrect re-reads or cached list lengths. All 207 distinct bodies pass 864
scenarios each, including empty rectangles, both levels, non-square maps,
negative zone IDs, object/connection filters, zero/30000/30001 costs and full
packed-field/guard preservation. Eleven deliberately broken controls fail.
Reduced owners and scripted helper algorithms establish the caller contract,
not retail ABI or correctness of the complete flood implementation.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-junction-prepare-family.py build/rmg-junction-prepare.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-junction-prepare-family.py build/rmg-junction-bindings.json --bindings-from build/source-families/JUNCTION_CONTEXT/checkpoint.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-junction-prepare-family.py build/rmg-junction-entrances.json --entrances-from build/source-families/BINDING_CONTEXT/checkpoint.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-junction-prepare-family.py build/rmg-junction-predecessors.json --predecessors-from build/source-families/ENTRANCE_CONTEXT/checkpoint.json
HOMM3_JUNCTION_MANIFEST=build/rmg-junction-predecessors.json PYTHONPATH=scripts python scripts/experiments/test_rmg_junction_prepare.py
```

The oracle also accepts colon-separated manifest paths to cover multiple
historical populations together, always including the currently authored body.

`generate-rmg-junction-scan-family.py` then tests four scan owners, five loop
structures and three reset-snapshot declaration scopes. The 60 states produce
sixteen objects without improving 77.5209%. Its `--receivers-from` frontier
retains ten reproduced parents and tests map references/pointers introduced
at the seed or entry, plus bounds-copy assignment. These sixty states produce
thirty code identities and reach 77.7209%. Both batches reproduce ten finalists.
The leader still adds to ESI in place where retail forms a separate LEA
receiver; its frame and extra scan block remain unchanged. That tiny score
gain does not establish the proposed receiver lifetime, so neither a scan nor
receiver alternative is adopted. The canonical score remains 77.5209%.

Both populations pass the same native caller oracle with eleven negative
controls, including the mutable entrance list and captured level. They are
separate source contexts from the four pre-adoption populations above.
The complete six-manifest union contains 314 distinct function bodies; all
pass the 864-scenario oracle and eleven negative controls. The final full
VC6 build passes with no further ledger changes, preserving 77.5209%.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-junction-scan-family.py build/rmg-junction-scans.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-junction-scan-family.py build/rmg-junction-receivers.json --receivers-from build/source-families/SCAN_CONTEXT/checkpoint.json
HOMM3_JUNCTION_MANIFEST=build/rmg-junction-receivers.json PYTHONPATH=scripts python scripts/experiments/test_rmg_junction_prepare.py
```

### Object-distance flood lifetimes (0x5402a0)

`generate-rmg-add-object-family.py` tests public seed/pop APIs and byte-valued
direction parity. No Dreamcast counterpart is mapped. Retail retains both
single-element seed insertions and the position erase; the cost erase expands
only to its copy/destroy helpers. The canonical position-first sorted helper
and ordinary base registration remain unchanged throughout the search.

Three sixty-state populations test queue operations, then coordinate ownership,
then trigger/seed lifetimes. They produce 36, 50 and 50 code identities, with
ten reproduced finalists per batch. Their ranges are 62.3600–75.7527%,
66.7564–75.7527% and 64.3309–75.7527%; every sibling score is unchanged.
Both frontiers verify full source/header snapshots, rendered parents, choices,
source hashes and repeated score/code identities before admitting parents.

The minimal 75.7527% leader uses position `pop_back` and byte parity, but its
frame remains 0x54 rather than 0x48. It calls copy/destroy instead of retail's
whole position erase, retains count-insert instead of single-insert at seeds,
and retains single-insert instead of count-insert at sorted insertions. Its
position-vector destructor also remains called instead of direct delete.
The coordinate and seed frontiers do not exceed this peak. No game-body
alternative is adopted; the canonical score stays 69.5855%. These finite
failures do not establish that the remaining helper decisions are unreachable.

The independent native oracle imports actual value/packed types, constructors,
compound translation, map lookups, base registration and the sorted helper.
A monotone fixed-point reference derives distances without a queue or binary
search. All 159 distinct bodies pass 3,888 scenarios each: non-square maps,
both levels, displaced triggers, signed zone IDs, zero-score barriers, global
and zone counters, preserved object-vector prefixes and complete packed/guard
cell preservation. Opaque base registration can replace object properties,
checking that the derived code reads them afterward. Ten wrong controls fail.
Reduced owners establish this behavioral contract, not retail ABI or allocation
failure behavior. Old checkpoints remain tied to their original snapshots.
The final full VC6 build passes with no ledger changes, and all 28 experiment
regressions pass. This checkpoint does not close the object-distance flood.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-add-object-family.py build/rmg-add-object.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-add-object-family.py build/rmg-add-object-coordinates.json --coordinates-from build/source-families/QUEUE_CONTEXT/checkpoint.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-add-object-family.py build/rmg-add-object-seeds.json --seeds-from build/source-families/COORDINATE_CONTEXT/checkpoint.json
HOMM3_ADD_OBJECT_MANIFEST=build/rmg-add-object-coordinates.json:build/rmg-add-object-seeds.json PYTHONPATH=scripts python scripts/experiments/test_rmg_add_object.py
```

### Key-tent guard ownership and cleanup (0x54b8c0)

`generate-rmg-key-tent-family.py` crosses five origin bindings, four outline
bindings and three fill/add result lifetimes. No Dreamcast counterpart is
mapped. Retail retains the outline's two-coordinate map query and the failure
reset; the initial 67.7917% body expands both. Its frame is 0x7c rather than
0x78 and its guard spills instead of remaining in EDI. The old source note's
map-destructor diagnosis is stale: both calls already agree in this baseline.

Three sixty-state caller populations test these bindings, then real map/vector
receivers and scan counters, then result temporaries and branch ownership.
Each produces 48 code identities and ten reproduced finalists. Their ranges
are 63.6875–73.9722%, 67.7917–73.9722% and 64.7639–73.9722%, with no sibling
score movement. The result-local leader restores the frame and guard register,
but also expands the success map destructor, which retail calls. A lower
73.7014% coordinate/item binding has the same contradiction. Neither restores
failure reset or the outline lookup, so no caller alternative is adopted.

`generate-rmg-key-color-helper-family.py` tests a provisional ordinary color
setter/first-free scan or scan-only member across all four repeated retail
sites: two in this caller, one in `placeBorderObject`, one in `removeObject`.
This is an inferred boundary, not a recovered Dreamcast name or inline
declaration. Sixty paired caller/helper states compile across all seven header
consumers into sixty code identities, with ten reproduced finalists. The best
actual helper form is 73.3681%; higher scores belong to parent controls without
the helper. That form still expands reset. Its retained two-coordinate lookup
occurs inside reset's expansion, not at the retail outline site. Some variants
lower exact border placement to 98.3726%, removal to 83.8622%, and the unchanged
header writer to 77.8952%. No helper/header edit is adopted. These results
bound the tested hypothesis, not all possible shared-helper reconstructions.

Parent admission verifies complete source/header snapshots, rendered trees,
source hashes, choices and repeated score/code identities. The shared-helper
family changes every tested call site and its one declaration atomically;
it introduces no inline controls or duplicate implementations.

The native phase-machine oracle uses actual coordinate/packed declarations,
map lookup bodies, object constructor/destructor and placement-mark reset,
plus the treasure-group declaration and its real inline constructor. Reduced
owners and scripted group helpers model caller contracts, not retail ABI.
All 207 distinct caller/helper combinations pass 17,496 scenarios each.
Checks cover absent and duplicate matching prototypes, both levels, byte
results 0/1/255, zero/signed fill results, color reservation and rollback,
outline marking, reference counts, exact ordered ownership/release/delete
events, and object-list growth from a virtual callback. Thirteen deliberately
wrong selection, flag, value, origin, marking and cleanup controls fail.
The helper additionally passes direct first-free checks over lengths 1..8,
every bit mask, every valid color and flag values 0/1/255. Successful allocation
is assumed. Host allocation tracking distinguishes reused addresses; fixture
instrumentation never enters a matching compiler snapshot.

The closing full build passed all gates without MAX changes, followed by
all 29 then-existing experiment tests (261.569 seconds).

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-key-tent-family.py build/rmg-key-tent.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-key-tent-family.py build/rmg-key-tent-receivers.json --receivers-from build/source-families/KEY_TENT_CONTEXT/checkpoint.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-key-tent-family.py build/rmg-key-tent-results.json --results-from build/source-families/RECEIVER_CONTEXT/checkpoint.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-key-color-helper-family.py build/source-families/RECEIVER_CONTEXT/checkpoint.json build/rmg-key-color-helper.json
HOMM3_KEY_TENT_MANIFEST=build/rmg-key-tent-receivers.json:build/rmg-key-tent-results.json:build/rmg-key-color-helper.json PYTHONPATH=scripts python scripts/experiments/test_rmg_key_tent.py
```

### Object-removal coordinate ownership (0x54bc50)

`generate-rmg-object-removal-family.py` starts from the retail-only 88.4667%
body. Its sixty input-position/index/map-coordinate states produce 48 code
identities and ten reproduced finalists. Unsigned `TRmgGridPoint` indices
restore the outer subtraction and inner mask-offset calculation, reaching
95.6356%, but retain the wrong frame and coordinate homes. A finite 41-state
refinement tests a full `TRmgMapPosition` destination with its level supplied
before or inside the loop, including the canonical position lookup overload.
It produces 31 code identities and ten reproduced finalists at up to 95.68%.
These leaders restore the 0x2c frame and retail coordinate homes; the footprint
loop then agrees instruction-for-instruction, with the earlier counter and
color-register differences shifting its start by seven bytes. Neither batch
changes sibling scores. This is retail-supported coordinate ownership, not
an unused padding local. No Dreamcast counterpart is mapped.

Two further sixty-state batches retain the level frontier while testing
counter spelling/type caching and zone result/item/owner lifetimes. They
produce 30 and 50 code identities respectively, with ten reproduced finalists
each. The counter forms are neutral per parent. Naming the zone owner reaches
95.7022%, but only changes color-register choices; the counter sequence still
disagrees. The simpler 95.6800% position-overload form is adopted. Its full
disassembly and relocations equal the inspected scalar-overload leader's.
The retained residual is the entrance arithmetic/global count scheduling,
zone load/decrement/store versus retail's memory decrement, and color
registers. Old snapshots reject the changed source at parent admission.
The full build banks 95.6800% with all gates passing. The post-build comparison
has 45/45 blocks and 31/31 branches; 43 blocks are exact and two differ in
size. Its strict relocation warning is the folded empty vector `_Destroy`:
the emitted pointer-vector body and retail 0x404140's artifact-vector label
both resolve to `ret 8`. Both retained bitset `_Xran` calls agree by name.
All 30 experiment regression tests pass after adoption, including the current
body and the complete 188-form removal union.

The native oracle imports actual coordinates, mask accessors and packed fields,
using reduced map/object/zone owners. Ninety distinct forms from these two
batches pass 9,216 scenarios each, checking clipped footprints, two levels,
passable/trigger masks, first-occurrence erasure, duplicate borrowed pointers,
global/zone counters, released colors, scores and empty-cell road flags.
The expanded union of all four batches contains 188 forms, each passing
18,432 scenarios after adding signed zone boundaries -128 and 126/127.
Twelve wrong controls fail. Nonempty searched lists contain the object;
retail's raw null-iterator tests are preserved, and `erase(end)` is outside
the contract. Host fixtures check surrounding cells and unrelated counters.
Full parent admission also handles exhausted populations below sixty: every
finite manifest choice must have a distinct scored record, in addition to the
source/header snapshot and repeated-object checks.

### Connection-cost worklist ownership

`generate-rmg-connection-queue-family.py` audits `floodConnectionCosts`
(`0x531460`) against all eight sorted insertions in the five other
position-worklist callers. No Dreamcast counterpart is available. Retail
inserts cost first at +0x3c6 and position second at +0x3de; vector receiver
addresses, element strides and argument addresses prove that order without
depending on folded callee labels. The other eight sites are position-first.
This contradicts this flood's attribution to the canonical position-first
helper, not the helper's supported calls elsewhere.

The first 49-state family tests a proposed ordinary shared index search,
caller-owned insertion, public vector operations, coordinate copies and cost
lifetimes. Three frontiers retain its ten reproduced parents and vary vector
ownership, pointer input/result ownership and explicit interval bounds.
They score 64, 64 and 46 states respectively. Every state in these four
families has a distinct code identity; the best flood score is 86.5439%.
The proposed shared search nevertheless retains new calls in other floods
where retail expands the loop. Those concrete call-stream contradictions,
not the collateral score dips, prevent adopting that abstraction.

The `--local` family withdraws only the contradicted caller attribution.
It preserves the canonical position-first helper and every supported call,
reconstructing this flood's different cost-first routine locally. Its 49
states produce 49 code identities and a reproduced 83.6813% winner from
62.4753%. A further 81-state `--lookups-from` frontier tests all combinations
of the three existing coordinate-lookup overloads across ten reproduced
parents: 41 code identities, no higher peak. Across all six completed
families there are 353 scored states, 311 distinct source states and 271
code identities after repeated controls/parents are removed.

The adopted source keeps the original lookup overloads, uses a copied
coordinate with the canonical compound translation, and pops both vectors
through their public API. Its 0x58 frame and cost-first count-insert calls
agree with retail. Seed insertions and popped-element erasures still expand
too deeply, and the unused queued-cost read is eliminated. Reusing that
variable restores a candidate read but scores 82.2308%; the remaining
lifetimes and call decisions are not solved. The five other queue callers
retain their previous scores. The unchanged-source header writer moves
77.9030% to 77.8952%, retaining its prior MAX/HIST.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-connection-queue-family.py build/rmg-connection-queue.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-connection-queue-family.py build/rmg-connection-local.json --local
PYTHONPATH=scripts python scripts/experiments/generate-rmg-connection-queue-family.py build/rmg-connection-lookups.json --lookups-from build/source-families/LOCAL_CONTEXT/checkpoint.json
HOMM3_CONNECTION_QUEUE_MANIFEST=build/rmg-connection-local.json PYTHONPATH=scripts python scripts/experiments/test_rmg_connection_queue.py
```

The other first-population frontiers use `--parents-from`, `--pointers-from`
and `--ranges-from`. Complete snapshots, rendered source, repeated scores,
code identities and hashes must agree before parent reuse. After adoption,
the generator recognizes the local child as its unchanged control; historical
parent snapshots must reject the changed implementation rather than rebase.

The independent oracle uses linear minimum selection with arrival-order ties,
not the candidate's binary search. Each positive body exercises 3,456 map
scenarios and 32 forty-element insertion sequences, checking cross-zone
costs, entrance/gate restrictions, levels, predecessors and all unaffected
packed fields. Interval-input forms additionally check every subrange of
small sorted arrays. Nine deliberately wrong cost, comparison, trait,
direction, level, gate, predecessor, zone and tie controls must fail.
Actual coordinate classes, packed declarations, constructor, translation,
predicates, setter and map accessors are imported. Owner/object layouts are
reduced host fixtures, not x86 ABI claims. The pointer-input fixture unwraps
libstdc++'s iterator at the host boundary because Dinkumware uses a raw pointer;
matching sources are unchanged. Successful allocation and bounded positive
map dimensions are the contract, not allocator-failure behavior.

### Water-distance cost branches (0x53f1a0)

`generate-rmg-water-queue-family.py` tests 48 combinations of signed/unsigned
cost locals, explicit cost-addition branches, coordinate construction and
public pop APIs. No Dreamcast counterpart is mapped. Retail branches before
adding the cardinal/diagonal step, whereas the initial source selects the
step before one addition. The canonical position-first insertion helper and
all its other callers remain unchanged.

All 48 states compile into fourteen distinct objects, with ten reproduced
finalists. Explicit branches plus `pop_back` reach 83.8661% from 65.6485%;
branches alone give 74.8201% and pops alone 76.3849%. The selected source
preserves both unsigned declarations and scalar coordinate construction.
Its frame is retail's 0x50, but seed insertions, popped erasures, sorted
position insertion and final cleanup still have different expansion decisions.
The only sibling movement is an unchanged-source `writeMapHeader` recovery
from 77.8952% to its existing 77.9030% MAX.

The `--frontier` follow-up crosses five direction bindings, three seed APIs
and four independent pop/erase pairs. Its sixty states produce 34 distinct
objects and ten reproduced finalists. A by-value `TPoint` direction copy
restores retail's paired component loads and reaches 85.7699%, with every
sibling score unchanged. Reference/pointer bindings and reversed additions
do not recover the paired load. The adopted child keeps single-element seed
inserts; replacing both with `push_back` is byte-identical. The remaining
vector expansion decisions and current-position register order stay open.

The native oracle imports the actual coordinate classes, translation, lookup,
direction table, packed field declarations and canonical insertion helper.
Linear minimum selection with arrival-order ties checks 2,560 maps per form,
including two levels, non-square dimensions, negative zone IDs, zero-cost
barriers, both endpoint seeds and unrelated packed-bit preservation. Six
wrong step/level/zone/direction/seed controls must fail. Reduced owner layouts
are not x86 ABI evidence; successful allocation and bounded costs are assumed.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-water-queue-family.py build/rmg-water-queue.json
PYTHONPATH=scripts python scripts/experiments/test_rmg_water_queue.py
PYTHONPATH=scripts python scripts/experiments/generate-rmg-water-queue-family.py build/rmg-water-queue-frontier.json --frontier
HOMM3_WATER_QUEUE_MANIFEST=build/rmg-water-queue-frontier.json PYTHONPATH=scripts python scripts/experiments/test_rmg_water_queue.py
```

After adoption the generator recognizes the selected source as its unchanged
control and retains all original alternatives; old search contexts stay tied
to their original snapshots. Neither the finite search nor the partial score
constitutes closure of this function or the RMG units.
The complete native first/frontier population covers 106 distinct source
bodies and passes the same six negative controls. The intermediate adopted
branch/pop version also passes all 21 surrounding experiment tests.
The final direction-copy child passes the full VC6 build and that complete
native population again. Its verified source comparison still begins at
the seed's surplus count-one push; CUR/MAX/HIST are 85.7699%, not 100%.

The subsequent `--lifetimes` family crosses five seed-value/iterator bindings,
four independent named-pop iterators and three neighbor-cost scopes. All
sixty states compile into sixteen code identities, with ten reproduced elites.
Naming both seed insertion ends reaches 88.7238% and removes the final
cost-vector `_Destroy`, leaving retail's direct `operator delete`. It does
not recover the seed-insert or popped-erase boundaries.

Four `--iterator-controls` isolate the seed bindings: the unchanged source
is 85.7699%; either end iterator alone, or both together, reaches 88.7238%.
The four states produce three whole-object identities and three reproduced
elites. The selected child names only the cost insertion's iterator and keeps
all existing scopes; every sibling score is unchanged. The iterator names
are provisional source hypotheses, not recovered original tokens. Their
actual insert uses and the cleanup control distinguish them from unused
compiler-state padding. Flattening the seed expressions restores the wrong
cleanup. The canonical queue helper, signatures and vector types stay intact.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-water-queue-family.py build/rmg-water-queue-lifetimes.json --lifetimes
PYTHONPATH=scripts python scripts/experiments/generate-rmg-water-queue-family.py build/rmg-water-queue-iterator-controls.json --iterator-controls
HOMM3_WATER_QUEUE_MANIFEST=build/rmg-water-queue-lifetimes.json PYTHONPATH=scripts python scripts/experiments/test_rmg_water_queue.py
```

The native oracle passes all sixty lifetime forms plus the initial controls
(108 distinct bodies), and all four isolated iterator controls plus initial
controls (52 bodies), with six broken controls rejected in each run. Those
counts overlap; they are not additive. After adoption the generators keep
the recognized child as unchanged and reconstruct prior families explicitly,
without silently reusing a pre-adoption source-family checkpoint.
The one-iterator child passes the full VC6 build and the four-control native
population again; its CUR/MAX/HIST are 88.7238%.

### Island mask painting lifetimes (0x53efa0)

`generate-rmg-island-paint-family.py` crosses four dimension representations,
five scan-coordinate lifetimes and three allocation-product forms. The
Complete-only body has no mapped Dreamcast counterpart. Retail's height
local, mask pointer in the old argument slot and paired paint-coordinate
homes motivate the alternatives; the existing borrowed-map constructor and
brush/map destruction boundary stay canonical.

All sixty states compile into eighteen code identities, with ten reproduced
finalists. Width-before-height declarations reproduce the retail subtraction,
allocation multiply and mask-pointer home, improving 93.5790% to 95.3977%.
One `TPoint` shared by the painting and tagging scans reaches 95.7953% with
no sibling score movement. All ten calls and branch destinations remain
aligned. The frame is still 0x28 rather than 0x30, and the inlined borrowed
view and scan homes are unresolved; the point hypothesis is not a frame fix.
Only those two source changes are adopted, leaving allocation's original
multiplication order and every helper declaration intact.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-island-paint-family.py build/rmg-island-paint.json
PYTHONPATH=scripts python scripts/experiments/test_rmg_island_paint.py
```

The native fixture imports actual point/bounds types, packed cell fields,
map lookup and borrowed constructor. Mask generation, terrain painting and
progress are opaque scripted boundaries. Flat-cell enumeration independently
checks 1,152 positive-extent rectangles per body, covering both levels,
non-square maps, clipped origins, terrain/connection flags, nullable progress,
ordered rectangle calls and destruction-before-tagging. The opaque brush's
completion update makes that lifetime boundary observable. Owner layouts are
reduced host fixtures, not x86 ABI evidence; allocation failure and the full
terrain/noise implementations are outside this caller contract.

Six wrong level, strength, terrain, mask predicate, water filter and progress
controls must fail. The first fixture accidentally initialized the target
flags to their final values, masking the broken water filter. Initializing
gate=1 and border=0 fixes that coverage hole; all sixty positives and all six
negative controls then pass. After adoption the generator checks that the
authored child belongs to the original family before rebuilding its unchanged
control. Old checkpoints are not silently rebased.
The adopted island and seed-iterator changes pass the full VC6 build and
all 22 surrounding experiment tests. Fresh source-labelled comparison gives
the island nineteen matching block shapes and ten matching calls, but its
first frame mismatch remains. Neither function is byte-exact yet.

The subsequent `--levels` family combines five level-coordinate forms,
four view-dimension bindings and three coordinate declaration lifetimes.
All sixty states compile into eighteen code identities and ten reproduced
finalists. A `TRmgMapPosition` carrying the tagging level restores retail's
0x30 frame, map at -0x3c, brush at -0x18 and scan x/y at -0x24/-0x20.
The source uses the level in the final scalar lookup; it is not unused
padding added to enlarge the frame. The minimal reproduced child assigns
the level after brush/map destruction, just before tagging. All sibling
scores remain unchanged; island painting moves to 95.8947%.

The `--origins` follow-up tests the original scalar plane query against
reusing that coordinate or naming a separate origin, with real dimension
reference/value bindings and independent origin initialization order.
All fifty states compile into sixteen code identities with ten reproduced
finalists, but none raises the score or resolves the borrowed constructor's
load/store order. No origin/binding alternative is adopted. The canonical
buffer-first constructor and scalar lookup remain unchanged.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-island-paint-family.py build/rmg-island-levels.json --levels
PYTHONPATH=scripts python scripts/experiments/generate-rmg-island-paint-family.py build/rmg-island-origins.json --origins
HOMM3_ISLAND_PAINT_MANIFEST=build/rmg-island-levels.json PYTHONPATH=scripts python scripts/experiments/test_rmg_island_paint.py
HOMM3_ISLAND_PAINT_MANIFEST=build/rmg-island-origins.json PYTHONPATH=scripts python scripts/experiments/test_rmg_island_paint.py
```

The native level population covers 119 distinct original/new bodies; the
origin population covers 110. Both pass 1,152 scenarios per body. A seventh
negative control clears the adopted coordinate's level without changing the
view level, so the final tagging plane is checked independently too. The
borrowed-view fixture additionally imports the existing position lookup;
matching sources and helper declarations receive no test instrumentation.
The adopted level coordinate passes the full VC6 build. Its first remaining
source divergence is the borrowed-map construction, not the frame prologue.
The final checkpoint passes all 22 surrounding experiment tests; every
island block after construction is identical in the verified source diff.

### Water-zone preparation register lifetimes (0x53f470)

`generate-rmg-water-prepare-family.py` tests five bounds/position construction
forms, three zone-index lifetimes and four perimeter-cell bindings. No
Dreamcast counterpart is mapped. The first 60-state batch produces twelve
code identities, with ten reproduced finalists, raising 93.4037% to 94.0882%.
The minimal candidate defaults then assigns the existing position and names
the complete perimeter cell pointer. Its reset body uses retail's register
sequence, and its perimeter now holds the zone index once per row. The
entry-copy scheduling and minimum-X back-edge reload still disagree.

Two 60-state frontiers carry ten reproduced parents into perimeter scopes,
while/do/goto guards and the existing ordinary level-position getter with
returned-value/reference lifetimes. They produce 39 and 48 code identities
without exceeding 94.0882%. Across all three batches there are 180 scored
states, 158 distinct sources and 78 code identities. Every batch reproduces
ten finalists; no sibling score moves. The minimal first-stage candidate also
reproduces independently, including its complete score vector, code identity,
source hashes and choices, before adoption. No loop or getter rewrite is adopted.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-water-prepare-family.py build/rmg-water-prepare.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-water-prepare-family.py build/rmg-water-prepare-loops.json --loops-from build/source-families/PREPARE_CONTEXT/checkpoint.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-water-prepare-family.py build/rmg-water-prepare-getters.json --getters-from build/source-families/LOOP_CONTEXT/checkpoint.json
HOMM3_WATER_PREPARE_MANIFEST=build/rmg-water-prepare-getters.json PYTHONPATH=scripts python scripts/experiments/test_rmg_water_prepare.py
```

Run each parent search before generating its frontier. Complete source/header
snapshots, rendered parents, repeated scores, code identities and source hashes
must agree. Adoption invalidates historical parent anchors; the initial
generator recognizes only its reviewed child and keeps it as unchanged control.

The native oracle imports actual coordinate/bounds/packed types, constructor,
map accessors and getter. A flat-cell reference checks 1,728 scenarios per body:
water/nonwater zones, non-square maps, both levels, signed zone IDs, empty and
clipped rectangles, ordered floods and islands, random draws and packed-field
preservation. Stateful opaque helpers mutate the source zone to test that its
entry snapshots survive. Ten wrong terrain/reset/flag/zone/threshold/selection/
radius/level/alias controls must fail. Initial, loop and getter populations pass
with 60, 109 and 115 distinct bodies respectively; these overlapping counts
are not additive. Reduced owners and scripted helper effects test the caller
contract, not x86 layout, allocation failure or the helper algorithms themselves.
The adopted source passes the full VC6 build and all 23 experiment regressions.
Its final 264-row `rmg` score vector equals the independently reproduced
candidate's vector; CUR/MAX/HIST for preparation are 94.0882%. The fresh
source-linked diff confirms all 25 instructions in reset B4 agree, and the
ordered eight-call sequence agrees under named-relocation comparison. The
remaining 56-versus-54-block layout and entry scheduling are still open.

The `--selection` family next tests five selected-coordinate bindings, four
distance-cell bindings and three radius forms. Its 60 states produce forty
code identities, with ten reproduced finalists, raising the peak to 95.3102%.
The minimal source names only the selected cell pointer: all 36 instructions
in retail's selection/radius blocks now agree, including complete address
formation before the distance read. Every other candidate block is unchanged.
Its complete score vector, code identity, source hashes and choices reproduce
in an additional isolated compile before adoption. All eight named calls still
agree, and no sibling score moves in this population.

Two further 60-state frontiers carry ten reproduced parents into candidate-scan
cell bindings/public insertion APIs and inset-upper-limit expressions/lifetimes.
They produce fifty and 21 code identities without improving 95.3102%. Some
competitors lower the unchanged-source header writer from 77.9030% to 77.8952%;
the selected minimal pointer body preserves it. ADD-negative expressions and
named limit values do not recover retail's two `ADD -4` instructions. No scan,
insertion, radius or upper-limit spelling is adopted. These three populations
contain 180 scored states, 159 distinct sources and ninety code identities.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-water-prepare-family.py build/rmg-water-prepare-selection.json --selection
PYTHONPATH=scripts python scripts/experiments/generate-rmg-water-prepare-family.py build/rmg-water-prepare-scan.json --scan-from build/source-families/SELECTION_CONTEXT/checkpoint.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-water-prepare-family.py build/rmg-water-prepare-upper-bounds.json --bounds-from build/source-families/SCAN_CONTEXT/checkpoint.json
HOMM3_WATER_PREPARE_MANIFEST=build/rmg-water-prepare-selection.json PYTHONPATH=scripts python scripts/experiments/test_rmg_water_prepare.py
```

Each population passes the independent caller oracle with 119 original/new
bodies. Two added upper-limit off-by-one controls bring the negative total to
twelve; the upper-limit population passes those as well. The adopted pointer
is explicitly retained in subsequent native tests and as the fresh selection
family's unchanged control. Pre-adoption parent snapshots remain invalid after
the edit; their old scores are not imported under the new implementation.
The selected-cell child passes the full VC6 build and all 23 experiment
regressions. Its 264-row `rmg` score vector reproduces the isolated candidate;
CUR/MAX/HIST are 95.3102%. The final selection/radius span has 103 identical
normalized bytes, with both random-call names and all 36 instructions aligned.
The rebased 119-body selection oracle passes all twelve negative controls.

### Obstacle clearance rectangles (0x53f880)

`generate-rmg-obstacle-clearance-family.py` crosses four lower-corner and
row/dimension lifetime forms independently across the three clipped scans.
No Dreamcast counterpart is mapped. The exact neighboring water-border pass
supports real `TPoint` corners, but does not establish this caller's lifetimes.
Retail carries outer x+2 while the initial candidate carries x-1; its final
row-bound reload also sits on both incoming paths rather than one.

All 64 states compile into 35 code identities, with ten reproduced finalists.
Scores span 86.8607–86.9809%, without exceeding the unchanged source. Four
states lower the unchanged-source header writer from 77.9030% to 77.8952%;
every other sibling holds. No game-body alternative is adopted.

The native oracle imports actual point/bounds and packed-cell declarations,
the coordinate constructor, both map lookup overloads and the ordinary
connection lookup. A flat-cell Chebyshev-distance reference checks 5,376
scenarios per form, covering empty and non-square dimensions, levels 0/1/2,
negative zone IDs, water, guarded/unguarded/missing directed connections,
occupied and connection-decorated cells, full packed-bit preservation,
guard cells and nullable progress. All 64 forms pass and twelve broken
filter, policy, bound and progress controls fail. Reduced owners are host
semantic fixtures, not retail ABI evidence. The reference may coalesce the
redundant center writes; matching source retains all three retail scan phases.

The `--upper-corners` family independently tests direct expressions, an
upper point, paired upper/lower points and a rectangle aggregate in each
phase. All 64 states compile into 57 code identities with ten reproduced
finalists, spanning 80.8033–91.7350%. Naming the upper point only in the last
scan restores the x+2 induction and raises the score to 91.7350%. Its one
progress call and complete named-relocation stream agree with retail. The
remaining block-layout and register differences are not resolved merely by
restoring that induction; this is a source hypothesis, not a closure claim.
Both full families contain 127 distinct bodies and pass the native oracle.

The 60-state `--corners-from` frontier preserves all ten reproduced upper
parents and combines their earlier clamps with lower-corner/row snapshots
and final dimension bindings. Source/header snapshots, rendered trees,
complete score vectors, code identities and hashes are checked before reuse.
It produces fifty code identities spanning 86.9809–91.7350%, with ten
reproduced finalists and no new peak. Its 49 new bodies join the previous
127 in the native test: all 176 pass, and all twelve broken controls fail.
Only the minimal last-scan upper point is adopted; every sibling score holds
in that candidate. The generator recognizes this child as the unchanged
control after adoption, while historical parent checkpoints must reject the
changed snapshot rather than silently reuse old scores.
The adopted point passes the full VC6 build and all 24 experiment tests.
The verified source comparison confirms x+2 but still has 84 versus 82
blocks; call and relocation streams agree. CUR/MAX/HIST are 91.7350%.
The three RMG units remain at 247 currently exact and 253 MAX-exact tracked
functions out of 363, so this partial recovery is not whole-TU closure.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-obstacle-clearance-family.py build/rmg-clearance.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-obstacle-clearance-family.py build/rmg-clearance-upper.json --upper-corners
PYTHONPATH=scripts python scripts/experiments/generate-rmg-obstacle-clearance-family.py build/rmg-clearance-corners.json --corners-from build/source-families/UPPER_CONTEXT/checkpoint.json
HOMM3_CLEARANCE_MANIFEST=build/rmg-clearance-corners.json PYTHONPATH=scripts python scripts/experiments/test_rmg_obstacle_clearance.py
PYTHONPATH=scripts python scripts/experiments/test_rmg_obstacle_clearance.py
```

### Underground decoration lifetimes (0x5439e0)

`generate-rmg-underground-family.py` tests five scan-coordinate lifetimes,
four dimension snapshots and three zone/bounds read orders. No Dreamcast
counterpart is mapped. All 36 control-flow blocks and ten calls already agree;
retail reuses the later coordinate's stack area for the first scan row, while
the initial source reuses the zone-index home. Borrowed-map construction and
the second scan's bounds-copy registers also differ.

All sixty states compile into sixty code identities with ten reproduced
finalists, spanning 95.6284–99.7431%. Copying the bounds before reading terrain
alone reaches 99.7294%, resolving every second-scan register difference.
The 99.7431% candidate also reuses a full coordinate for the initial plane
query, first scan and later zone-position query, and captures view dimensions
in a `TPoint`. This resolves the first row's stack home. Only the initial
borrowed-map construction differs in the masked block comparison; all ten
named calls remain correct, and the selected candidate changes no sibling.

The `--origins-from` frontier retains all ten reproduced parents and tests
map references, full-coordinate origin queries, value/reference plane-pointer
bindings, and dimension reads before the pointer lookup. Sixty states produce
59 code identities spanning 92.2752–99.7431%, with ten reproduced finalists
and no higher peak. Some alternatives also move the unchanged-source header
writer; no origin alternative is adopted. Parent snapshots, full repeated
score vectors, code identities and rendered-source hashes must agree before
reuse, so this frontier cannot silently rebase onto an edited implementation.

The native oracle imports actual value/packed types, both map lookups, the
borrowed constructor, the zone-position getter and byte-valued tile queries.
Flat-cell enumeration checks 4,480 scenarios per form, including empty and
non-square dimensions, zero to four zones, level filtering, terrain/occupancy
flags, full packed-field preservation and both nullable progress updates.
Opaque brush calls trace paints and terrain changes; progress can replace
or clear its pointer before the second update. Brush and borrowed-map cleanup
must occur after that update, in order. The 60 initial forms and 109 combined
initial/origin forms pass, and eleven broken controls fail. Reduced host
owners test this caller contract, not the retail ABI or terrain-brush algorithm.

The shared-constructor family tests all six orders of its items/width/height
assignments across the ten reproduced callers, plus the original control:
61 states and 61 code identities, with ten reproduced finalists. All seven
header consumers are compiled. Assigning items before width/height closes
`createWaterZoneIsland` at 100% and preserves exact water-border repair.
The combined underground caller reaches 99.7982%. Only three scores change
in the selected candidate: the two improvements and `paintZoneTerrain`
94.0933% to 92.2015%. That coordinator's entire ordered 18-call sequence is
unchanged; its previously known cleanup over-expansion remains unresolved.

Four independently reproduced controls isolate the effects:

| Change | Underground | Island painting | Terrain coordinator |
| --- | ---: | ---: | ---: |
| Neither | 96.0963% | 95.8947% | 94.0933% |
| Caller only | 99.7431% | 95.8947% | 94.0933% |
| Constructor only | 96.1514% | 100% | 92.2015% |
| Both | 99.7982% | 100% | 92.2015% |

Water-border repair stays 100% in every control. No alternate declaration,
inline pin or pasted helper body is introduced. The native coordinator oracle
passes all 110 original/constructor pairs and the 62 original/control pairs,
including all eleven negative controls. Those counts overlap. The exact
island candidate has no instruction differences, with all nineteen blocks,
fourteen branches and ten named calls aligned; the remaining named data
differences are three unclaimed retail labels. Its existing native island
oracle also tests the actual shared constructor after adoption.

The selected caller and constructor pass the full VC6 build. Island painting
has CUR/MAX/HIST at 100%; underground decoration has 99.7982%. The unchanged
terrain coordinator retains its 94.0933% MAX/HIST. The verified underground
source comparison now differs only in initial construction, not the scans.
RMG has 248 currently exact and 254 MAX-exact tracked functions out of 363.
The generator recognizes the adopted caller as its unchanged control;
historical parent snapshots remain invalid after the source/header changes.
All 25 experiment regressions pass at this checkpoint, including the island
and underground oracles against the adopted shared constructor. The expanded
118-form scalar-lookup oracle also passes separately, rejecting five wrong
dimension, level, coordinate and mutated-base controls.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-underground-family.py build/rmg-underground.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-underground-family.py build/rmg-underground-origins.json --origins-from build/source-families/UNDERGROUND_CONTEXT/checkpoint.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-underground-family.py build/rmg-underground-constructor.json --constructor-from build/source-families/UNDERGROUND_CONTEXT/checkpoint.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-underground-family.py build/rmg-underground-controls.json --constructor-from build/source-families/UNDERGROUND_CONTEXT/checkpoint.json --constructor-controls
HOMM3_UNDERGROUND_MANIFEST=build/rmg-underground-origins.json PYTHONPATH=scripts python scripts/experiments/test_rmg_underground.py
```

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

`generate-rmg-group-place-homes-family.py` revisits the six remaining stack
displacements after the later RMG changes. Its sixty forms cross five
entrance declaration lifetimes, four neighbor-point lifetimes and three
guard-scan declaration lifetimes. Getter, snapshot and query order remain
unchanged. All sixty states compile into six whole-TU code identities, and
all six distinct finalists reproduce. Scores span 99.9152–99.9850%; every
sibling score is unchanged. Moving the declarations alone does not resolve
the entrance/neighbor homes, and no alternative is adopted.

The `--phases-from` frontier retains all six reproduced parents. It tests
references to the real working coordinate or returned getter value, plus
shorter object/guard and entrance-check scopes. Invalid combinations that
would let a shared reference outlive its owner are excluded. After removing
identical source forms, the finite family contains 57 states; all are scored,
producing fifteen code identities and ten reproduced finalists. The range is
again 99.9152–99.9850%. Some variants lower the unchanged header writer from
77.9030% to 77.8952%; no other sibling moves. Neither family is adopted.
Across both valid batches there are 117 trials, 110 distinct source bodies
and fifteen whole-TU code identities.

The first phase-search run correctly refused its final checkpoint because
the test harness changed during the run. Its observations are not banked.
A fresh full build and frozen harness establish the completed replacement
context. Parent admission checks the manifest, source/header snapshots,
rendered source hashes and repeated score/code identities. The native harness
now accepts finite populations smaller than sixty and multiple manifest paths;
it does not truncate, pad or silently reuse old compiler scores.

All six placement tests pass, including the 170-body union of the original
sixty semantic controls and the two new populations. The existing flat-cell
oracle checks 2,646 scenarios per body and rejects fourteen wrong controls.
Its actual getters, value types and map interfaces remain canonical. These
bounded negative results do not prove that the six-byte residual is unreachable.
The closing full VC6 build passes all gates with no ledger changes.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-group-place-homes-family.py build/rmg-group-place-homes.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-group-place-homes-family.py build/rmg-group-place-phases.json --phases-from build/source-families/HOMES_CONTEXT/checkpoint.json
HOMM3_GROUP_PLACE_MANIFEST=build/rmg-group-place-homes.json:build/rmg-group-place-phases.json PYTHONPATH=scripts python -m unittest homm3.vc6.test_rmg_group_place
```

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

The later centroid `--entry-bindings` family tests five slot/index bindings,
three returned-position lifetimes and four accumulator constructions. Its
sixty states produce 36 code identities, spanning 65.7391–95.5000%, with ten
reproduced finalists. `--zeroing-from` retains those ten complete parents and
tests earlier independent X/Y initialization, leaving the level capture after
the actual position copy. Its sixty states also produce 36 code identities,
spanning 74.9565–95.5000%, with ten reproduced finalists. Together these two
batches contain 110 distinct sources and 62 code identities. Neither improves
the centroid; some alternatives lower only the unchanged-source header writer
from 77.9030% to 77.8952%. No game-body alternative is adopted.

The entry block still differs at the slot-index/returned-coordinate schedule
and separate zeroing registers. The other ten blocks and the empty named-call
and relocation streams agree. Parent admission checks the entire source/header
snapshot, manifest, rendered source, source hashes, and repeated score vectors
and code identities. The native flat-cell oracle covers both new populations,
the authored body, and the prior controls at 3,000 scenarios per form. It now
also checks preservation of the borrowed slot's index; an eighth negative
control mutates that field and must fail. The `long` local is tested only over
the signed-int input domain; host width does not establish the retail ABI.
The final full VC6 build passes with no checkpoint changes. All nine focused
centroid, spatial-frontier, coastal and river-target regressions pass, including
both new populations and all eight centroid negative controls.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-recenter-family.py build/rmg-recenter-entry-bindings.json --entry-bindings
PYTHONPATH=scripts python scripts/experiments/generate-rmg-recenter-family.py build/rmg-recenter-zeroing.json --zeroing-from build/source-families/ENTRY_CONTEXT/checkpoint.json
HOMM3_RECENTER_MANIFEST=build/rmg-recenter-zeroing.json PYTHONPATH=scripts python -m unittest homm3.vc6.test_rmg_recenter
```

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

### Map-writer coordinate and result lifetimes

`writeMap` (0x54abf0) has no mapped Dreamcast counterpart. Its original
95.5071% source already agrees on all 41 CFG blocks and twelve call sites,
but has a 0xc frame instead of 0x14, different terrain-counter homes, and
a final comparison transferred from CL into AL. Both prototype loops also
use vector-owner-biased addressing instead of retail's first-pointer bias.

`generate-rmg-map-writer-family.py` crosses five scalar/coordinate lifetimes,
three vector receiver bindings and four scoped result forms. Sixty scored
sources produce twelve code identities spanning 90.8626–99.4123%, with ten
reproduced finalists. Its `--buffers-from` frontier retains those parents and
tests five real function-scoped/shared wire-buffer arrangements. Sixty states
produce fifty identities spanning 90.8436–99.4123%, again with ten reproduced
finalists. The union contains 110 source bodies and 52 code identities.
Every sibling score is unchanged. Parent admission verifies the manifest,
complete source/header snapshot, rendered source, hashes and repeated scores
and code identity; historical checkpoints cannot silently rebase.

The adopted first-batch parent declares the actual `TRmgMapPosition` at entry
and uses its components as the three terrain-loop counters. A scoped byte
result gives the final reserved buffer retail's dead parameter home and
produces direct `SETE AL`. All stack homes now agree, not just frame size.
Both prototype-loop receivers still use +0x34 with compensated field loads,
where retail uses +0x38 and saves one instruction byte per loop. This is the
remaining 99.4123% residual; the buffer frontier does not resolve it. No
helper boundary, container implementation, raw layout or inline pin changes.
These lifetimes are retail-supported hypotheses, not original source names.

The final full VC6 checkpoint passes every gate and raises only `writeMap`
to 99.4123%. The verified source comparison agrees on the complete terrain
walk, all scalar-buffer writes, placed-object passes and final return; its
first differing instruction is the first prototype-vector receiver. RMG
remains at 250 currently exact and 256 MAX-exact tracked functions out of 363.

The subsequent `generate-rmg-map-writer-scans-family.py` crosses five shared
counter lifetimes, four public subscript/iterator forms and three counter
type combinations. All sixty states score exactly 99.4123%, producing nine
whole-TU code identities and nine reproduced finalists. Its `--owners-from`
frontier carries all nine parents and tests const receivers, array pointer/
reference bindings and property pointer/object references. The corrected
sixty-state family produces forty code identities and ten reproduced
finalists, spanning 95.5687–99.4123%. No sibling score changes and neither
family is adopted. This bounds these source hypotheses, not every possible
explanation of the remaining address bias.

An initial owner manifest contained sixteen malformed candidates: const
receiver forms retained mutable iterators, and an overbroad property-member
replacement touched the placed-object path. Those failed compiles are not
counted as successful exploration. The generator now fixes the iterator type
and restricts the replacement; a dedicated test checks all 360 scan/owner
combinations for those two defects. The corrected family passes native
validation before the complete VC6 batch.

The expanded native oracle appends a prototype from a serialization callback,
checking the live vector bound and rejecting a ninth, cached-size control.
All 220 distinct bodies across the four valid populations pass 69,120
scenarios each, including the adopted source. Callback growth occurs during
the last bucket's traversal, not just before entering the loop. The nine
negative controls must all fail.

The closing full VC6 checkpoint passes all gates with no ledger changes;
both map-writer tests pass, including the complete 220-body union.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-map-writer-scans-family.py build/rmg-map-writer-scans.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-map-writer-scans-family.py build/rmg-map-writer-owners-fixed.json --owners-from build/source-families/SCAN_CONTEXT/checkpoint.json
HOMM3_MAP_WRITER_MANIFEST=build/rmg-map-writer.json:build/rmg-map-writer-buffers.json:build/rmg-map-writer-scans.json:build/rmg-map-writer-owners-fixed.json PYTHONPATH=scripts python scripts/experiments/test_rmg_map_writer.py
```

`test_rmg_map_writer.py` imports the actual coordinate declaration and abstract
stream interface, with reduced map/prototype/object owners and opaque helper
callbacks. An independent event sequence uses a flattened prototype inventory
and stable object partition. All 110 bodies pass 34,320 scenarios each,
including empty/non-square maps, zero/one/two levels, signed reference-count
boundaries, reserved prototype slots, duplicate object types, both trait
passes, optional progress, five write results and exceptions at every event.
The header callback can change the width before cell serialization. Index
assignments are checked even at interrupted exits; eight wrong order, count,
reference-sign, cell-walk, progress and return controls are rejected. This
tests successful allocation and synchronous buffer consumption, not x86 owner
layout or arbitrary callback mutations. After adoption the original sixty
forms remain explicit semantic controls without importing old compiler scores.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-map-writer-family.py build/rmg-map-writer.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-map-writer-family.py build/rmg-map-writer-buffers.json --buffers-from build/source-families/MAP_WRITER_CONTEXT/checkpoint.json
HOMM3_MAP_WRITER_MANIFEST=build/rmg-map-writer.json:build/rmg-map-writer-buffers.json PYTHONPATH=scripts python scripts/experiments/test_rmg_map_writer.py
```

### Prototype wire-buffer reconstruction

`writeRmgObjectPrototype` (0x54ae30) has no mapped Dreamcast counterpart.
Its original 94.9532% body agrees with retail on all branches and seventeen
calls, but uses a 0x2c rather than 0x30 frame. Each partially initialized
six-byte mask emits an unaligned dword store between two byte stores, where
retail writes one dword and one word. The scalar homes and final reserved
buffer's zero-register scheduling also differ.

`generate-rmg-prototype-writer-family.py` first crosses five initialization
forms, four scalar-buffer lifetimes and three terrain-buffer lifetimes.
Its sixty states produce forty code identities spanning 87.0504–97.4317%.
The `--loops-from` frontier keeps ten reproduced parents and tests real shared
X/Y/bit loop variables. Its sixty states produce 44 code identities spanning
94.9532–97.5000%. The `--reserved-from` frontier tests six representations or
initializations of the final sixteen reserved bytes. Its sixty states produce
38 code identities spanning 92.5252–100%, including twelve exact sources.
Each batch reproduces ten retained results. Across all three batches there
are 158 distinct sources and 99 code identities; every sibling score holds.

The adopted reproduced child keeps the original integer reserved array and
uses `memset` for it and the two six-byte masks. One shared terrain buffer
and shared X/Y scan variables restore every stack home. Both image-name
queries, bit-position helper, bitset operations and eleven virtual writes
remain canonical. No inlining directive, alternate helper declaration or raw
bitset representation is introduced. These lifetimes are retail-supported
source hypotheses, not recovered original names.

The full VC6 build raises only this serializer to 100%, with all gates passing.
All 41 blocks, nineteen branches, seventeen named calls and instruction rows
agree in the verified source comparison. The remaining strict data-label
distinction is `basic_string::_Nullstr` versus the pooled empty string at
0x63a608; both data bytes are verified zero. This instruction match does not
claim whole-program data linking or RMG TU closure. RMG now has 250 currently
exact and 256 MAX-exact tracked functions out of 363.

`test_rmg_prototype_writer.py` imports the actual consumed field declarations,
object-type enum, bit-position helper and abstract stream interface. The
image-name registry is an opaque scripted boundary; the prototype owner is a
reduced host fixture, not an x86 layout model. An independent little-endian
integer encoder checks every complete wire record and the ordered query/write
trace, rather than reproducing the candidate's nested mask loops.

All 158 historical/current bodies pass 73,728 scenarios each, including all
1,024 terrain masks, every single footprint bit, empty/embedded-NUL names,
signed narrowing boundaries, raw underlay bytes, zero/error write returns,
exceptions at each write, and field/name mutations between writes. Twelve
wrong length, query, mask, coordinate, field, narrowing, reserved-data and
missing-write controls are rejected. The caller ignores write results but
propagates exceptions. Successful host allocation and synchronous buffer
consumption are the tested contract, not allocator-failure behavior.

The post-adoption experiment regression run passes all 33 tests in 318.399
seconds, including the full 158-form historical/current writer union.

After adoption, the generator recognizes this exact child as its unchanged
control and reconstructs the historical alternatives explicitly. Old compiler
checkpoints must still pass full source/header, manifest, rendered-body,
repeated-score and code-identity checks; they cannot silently rebase. Native
tests may import the historical bodies without importing any old scores.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-prototype-writer-family.py build/rmg-prototype-writer.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-prototype-writer-family.py build/rmg-prototype-writer-loops.json --loops-from build/source-families/WRITER_CONTEXT/checkpoint.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-prototype-writer-family.py build/rmg-prototype-writer-reserved.json --reserved-from build/source-families/LOOP_CONTEXT/checkpoint.json
HOMM3_PROTOTYPE_WRITER_MANIFEST=build/rmg-prototype-writer.json:build/rmg-prototype-writer-loops.json:build/rmg-prototype-writer-reserved.json PYTHONPATH=scripts python scripts/experiments/test_rmg_prototype_writer.py
```

### Request worker seat setters

Complete-only `TRandomMapRequest::generateToFile` at 0x54bf60 constructs the
0x14e0-byte generator, repairs too-small player counts, copies eight seat
options, generates, optionally writes, and destroys the generator. There is
no mapped Dreamcast counterpart. Retail retains one in EBX across the clamp,
count repair and human-flag stores; the original direct stores did not.

`generate-rmg-request-worker-family.py` scores 60 clamp/repair/seat-binding
states (20 object identities, 67.1359–85.5534%). Its `--flags-from` frontier
scores 60 actual flag-value lifetimes (18 identities, 79.6699–85.5534%).
`generate-rmg-request-byte-family.py` carries those reproduced parents into
60 input/output byte-type states (10 identities, 81.8155–85.5534%), checking
all nine request-header consumers. The worker's `--counts-from` tests 60
count bindings (50 identities, 80.6602–85.5534%). None improves the peak.

`generate-rmg-request-setter-family.py` tests ordinary human, town and combined
seat methods, with paired owning declarations, implementations and calls.
Its 52 distinct source states produce 51 objects spanning 81.8058–99.6116%.
Human setters recover EBX and the complete frame, but leave the town-copy
registers rotated. `--refine` preserves each reproduced parent's *whole*
declaration/body/caller patch and checks town setters, index types, and town
bindings. Its 60 states produce 58 objects spanning 85.5534–100.0000%, with
20 exact worker results and ten independently reproduced retained elites.
All other scores hold across the seven generator-header consumers.

The adopted simple one-argument human setter plus two-argument town setter
uses independent lower/upper clamps and the original separate count stores.
Both ordinary methods naturally expand; no explicit inline, pins, extra
declarations, or dummy operations are used. These source boundaries and names
remain provisional retail-codegen hypotheses, not recovered debug facts.
The full authored VC6 build independently reproduces 100.0000%, raising only
this checkpoint with all gates passing. All 15 blocks, seven branches, two
returns, and instruction rows agree. The strict named-call view retains the
CRT `__chkstk` versus retail `__alloca_probe` label distinction at +0x1d;
the five game calls agree by name and offset. Generated EH labels also differ.
This is an exact instruction/report result, not proof of whole-program linking.

`test_rmg_request_worker.py` imports the actual request declaration, constructor
and result enum. Its opaque generator fixture captures all constructor inputs,
scripts post-construction request mutations, and observes generation, writes,
exceptions and destruction. The six-family union has 285 positive forms, each
passing 126,720 non-overflowing scenarios; 17 incorrect clamp, repair, seat,
argument, result, helper and call-boundary controls are rejected. This is caller
behavior evidence, not validation of the generator algorithm or x86 layout.
The adopted caller and actual setter declarations/bodies are explicitly added
after adoption; all 286 forms and the same 17 negative controls pass.
The full 31-test experimental RMG suite also passes after adoption.
Initial-family generation recognizes that caller as its
unchanged control; historical parent snapshots still require exact provenance.

### Generation coordinator buffers and player mapping (0x549930)

This Complete-only coordinator has no Dreamcast counterpart. The initial
source-labelled retail comparison exposed a semantic error independently of
the buffer search: progress uses `6900 / m_zones.size()` (retail +0x331 loads
`0x1af4`), not 7000. Correcting that constant passes the full build and moves
96.8971% to 96.9003%. The preliminary 7000-based experiment is not a positive
behavioral control; its constant is retained only as a rejected test mutant.

`generate-rmg-coordinator-family.py` combines five human-buffer forms, four
all-player-buffer forms and three nine-entry mapping fills. The corrected
60-state batch produces 60 distinct whole-TU objects and ten reproduced
elites, led by 99.9228%. Both eight-byte `memset`s and an ordinary nine-int
fill loop reproduce retail's four aligned zero stores and destination-first
`rep stosd` setup. The leading sibling vector is unchanged. Some losing
forms also move `writeMapHeader` from 77.9030% to 77.8952%; its retained
MAX/HIST remain distinct from these observations.

The `--parent` family checks snapshot, manifest and repeated score/object
identity before combining ten finalists with signed/unsigned selected-index
locals and either order of the player-count addition. Its entire 41-state
population yields eleven distinct objects and ten reproduced elites, with no
further gain. On the adopted body, all 69 CFG blocks have retail sizes.
The only source-labelled deltas are B6's selected-index EDI versus EBX and
B28/B33's commuted human/computer count loads. The progress immediate and
remaining instruction rows agree. No helper, interface or inline directive
changes are involved.

`test_rmg_coordinator.py` extracts the actual function and domain enums into
reduced native owners. An independent stable partition and slot filter checks
the player mapping. Ordered opaque callbacks exercise live template reloads,
boundary-level growth, separate town passes, zone-list growth during primary
town and treasure placement, water-junction filtering, post-mine active counts,
post-callback progress denominators and final decoration order. The final
union contains 101 source forms (including the adopted body and historical
manifest forms with different comments), each tested on 18,432 scenarios and
their empty-template exits. Seven deliberately wrong progress, map extent,
slot reuse, template caching, town-pass, water-filter and road-pass controls
are rejected. This is bounded behavior evidence, not x86 layout validation.
The full adopted VC6 checkpoint passes, banking CUR/MAX/HIST at 99.9228%.
The coordinator remains non-exact; exhausting these families is not closure.

### Final connection-path terrain extraction check (0x5405d0)

The fresh source-labelled comparison has 41 blocks, 29 branches and three
matching named calls. Only the signed six-bit terrain extraction and a
loop-exit load order differ. `generate-rmg-connection-terrain-family.py`
exhausts seven forms: unchanged, masked signed-field read, explicit unsigned
conversion, separate masking, const unsigned, signed and const signed locals.
Seven scored sources yield three distinct objects, all reproduced. The
unchanged body remains 99.3582%; every masked form gives 81.5933% and also
moves `writeMapHeader` from 77.9030% to 77.8952%. The canonical signed field
is independently required by other callers. No game-code change is adopted;
the local extraction mismatch remains open, not a claimed semantic failure
or a resolved compiler limitation.

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
Those names now own the corresponding `m_` members. The cleanup preserves
every raw byte and relocation destination in both consumers (77 ai_combat
sections / 462 destinations). A follow-up public-symbol check corrects the
initial return-type reading: chooseMelee's `0x5a2b` debug record uses primitive
`0x20` for byte storage, but its decorated public contains `IBA_N` and proves
`bool`. The byte-return ABI and byte-consuming callers do not distinguish
these source types. The declaration and definition therefore retain `bool`.

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

### Tactical mass/summon boundaries and exact spell dispatch

DC `get_damage_value` (`0x3d96c`), `get_group_damage_value` (`0x3dabc`) and
`get_mass_damage_effect` (`0x3db2c`) are const members. `consider_mass_damage`
(`0x3de90`) and `consider_summon` (`0x41e5c`) additionally take writable choice
references. The damage/group/effect/mass interface correction preserves every
raw byte, function location and relocation destination in all five consumers:
ai_tactical (145 sections / 951 destinations), ai (101/542), drawing (77/651),
ai_combat (77/462) and ai_player (420/1963), under the two evidenced retained
function renames. Both retained helpers remain exact.

The dispatcher at `0x43bb20` expands its mass/group/summon helpers but retains
the mass-effect call. Its inherited model imposed that split with force-inline
and a depth pin, and pasted the summon helper at its caller. DC lines 3129/3167
prove the two helper calls; summon line 3098 calls `get_mastery_value`, and line
3107 writes cast-now after the kills-only split. The required retail review
finds a 39-block, 19-branch, ten-return caller at 98.1927%, with all seventeen
named calls already aligned and only register/scheduling deltas remaining.

`generate-tactical-mass-boundary-family.py`, context `07302be162953796ffdc`,
exhausts **24 states / 16 objects**, with ten reproduced elites and all **347
scores across five TUs** checked. It crosses ordinary mass/group definitions,
the existing effect fence/deletion, and pasted-table/pasted-accessor/canonical
ordinary-summon forms. The fully canonical, unfenced corner `[1,1,1,2]`,
`6442401e5b4a12b3a56f8e26`, preserves every score. Removing the pin while
retaining the pasted mastery-table expression instead lowers considerSpell
to **80.5073%**. Restoring the accessor, even before lifting the summon body,
recovers the exact required inline split. A real source call in one arm can
therefore repair an earlier arm's inline decision without caller padding.

The boundary-only production object independently reproduces the chosen
**148 sections / 966 relocation destinations**. The dedicated raw verifier
also proves that all **145 prior sections, 102 emitted functions and 951
relocation destinations** are unchanged. Only the three ordinary helpers gain
unreferenced executable COMDATs (padded sizes 144, 528 and 160 bytes); no
relocation points at them. These natural compiler-emitted copies have no
claimed retained retail address. This is not whole-object identity with the
old object, nor permission to ignore untracked code changes.

`generate-tactical-dispatch-lifetime-family.py`, context `8704e5c4ec9d3952d373`,
then exhausts **16 states / six objects**, all six representative objects
reproduced. DC row 973 forms the target address before row 975's damage call;
the family tests a direct address, named pointer/reference and named damage
result, crossed with four ordered mass-result lifetimes. The pointer and
reference targets both make **considerSpell 100%**; direct addresses and named
damage results stay at 98.1927%, regardless of the mass-result alternative.
Only the caller's score changes among all 92 tactical rows.

The minimal pointer-target form `[1,0]`, `12c6d47d7d9f0b9f0f0e6cb4`, is
independently recompiled and adopted. The accumulator also recovers its DC name
`value`. Production preserves all 148 raw sections / 966 relocation destinations
of that repeat, while the other four consumers preserve their complete objects.
The final retail CFG, instructions and named call stream are checked again;
the former group-walk and later Dispel register differences are resolved.
The 100% verdict uses the existing matching metric: the ten remaining data-
name differences are four spell-table alias references and six unclaimed
global references. All sixteen actual calls and the jump-table dispatch
relocation agree by named target/addend; no data-name normalization is changed.

`test-tactical-mass-boundaries.py` extracts the four actual helper bodies. At
native `-O0` and `-O2`, **3025 effect cases, 6144 group/mass cases and 1024 summon
cases** check ratios, clamps, reverse order, both hero/side arguments, mastery,
guards, the post-split cast-now write, and unaffected choice fields. Nine
deliberate faults are rejected. Mocked battle services and bounded non-overflow
integers make this a semantic check, not a VC6 layout or floating-codegen oracle.

All ai_tactical inline overrides are gone, as are its forced mass declaration
and pasted summon implementation. The global census is **217 overrides**
(212 depth-zero / five auto-inline-off) across 29 TUs, and **63 unions**.
This finishes the current dispatch matching pass, not the remaining interface,
min/max or whole-TU source-reconstruction debt.

### AI turn-close purchase and warning boundaries

`type_AI_player::endTurn` (0x428dd0) stood at 89.5263% behind a string-append
fence. DC ai_player.cpp:439 positively calls the ordinary `purchase_buildings`
member, whose body at 0x31094 owns `prohibited_creatures` and calls
`fill_prohibited_array` followed by the `purchase_building` loop. Complete
expands this boundary with its 145-entry flag table. The previous declaration
was an unused byte-returning, pointer-argument interface; the actual member
is void/no-argument, and its decorated `...@@IAAXXZ` public proves protected
access. No external caller uses the superseded declaration.

DC line 493 calls `format_string`, then `string::operator+=`, then destroys the
temporary. Its warning-loop backedge at 0x2e992 increments and sign-extends a
short. Retail independently passes the formatter's return pointer straight to
the retained `append(string, pos, count)` at 0x41b250 and exits on an unsigned
positive length check. The old source used an explicit append and named copy,
pasted the purchase helper, and expressed the warning walk as two pointers.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-ai-endturn-boundary-family.py build/ai-endturn-boundary.json
PYTHONPATH=scripts python -m homm3.vc6.source_families build/ai-endturn-boundary.json --width 60 --keep 10 --jobs 6
PYTHONPATH=scripts python scripts/experiments/test-ai-endturn-boundaries.py
```

Run the generator against the pre-adoption source (`3be2cd52`); its anchors
intentionally reject changed source. The complete manifest and snapshots are
preserved under context **`6d71af72491811a3bd28`**. All **60/60 states** compile,
producing **60 distinct objects** and **ten reproduced elites**. All **437
tracked scores across five current header consumers** are checked; only
endTurn changes in the entire family. The reproduced opposite corner and
winner **`[14,1,1]` / `eb3fa06f257171a880de80b2`** reaches **100%** with the
ordinary purchase member, temporary `+=`, short index and positive length.

The deletion-only control falls to **61.2669%**. Against the exact candidate,
pasting the purchase loop back falls to **77.0226%**, using an int warning
index to **90.6917%**, using a named format copy to **98.5038%**, and changing
the length guard to a truth test to **99.7744%**. These controls explain why
an isolated fence deletion was previously loss-only. No helper is marked
inline and no dummy operation or replacement suppression is introduced.

The final source restores the DC local names `purchaser`, `checker`, and `msg`.
Its entire raw object reproduces the winning recompile: **421 sections and
1967 relocation destinations**, permitting only the independently proved
public-to-protected helper-symbol correction. Against the old object, the
other **246 emitted function bodies** retain their raw bytes, no function
disappears, and one unreferenced 96-byte padded ordinary helper is added.
The caller's cleanup offsets and STL COMDAT order change, so this is not a
claim of whole-object identity against the old source. The four other
consumers do preserve their complete raw objects: advmgr **308/4091**, ai
**101/542**, ai_combat **77/462**, and philai **229/1440** sections/relocations.

Retail verification agrees on all **54 CFG blocks, 32 branches, two returns,
and seventeen named call targets/addends**, with no differing instruction
row. The existing 100% metric still ignores thirteen data-name differences:
five `g_game`/`gpGame` name aliases, six unclaimed data/vtable references, one
source-claimed resource-name table and one generated empty-string label.
No data-name normalization or retail target is changed to obtain this result.

The native actual-body oracle tests **8192 cases per form at -O0 and -O2**,
crossing all seven-resource sign masks with 64 player/town/alliance/purchase
states. It checks reserve clamps, strategy and purchase order, the mutable
145-entry flag array across repeated calls, Marketplace lookup, AI-before-
human gifts, formatting arguments, and warning order/content. Five deliberately
wrong controls are rejected. All sixty source forms pass before adoption;
the default test subsequently checks the actual adopted bodies, and `--source`
can check any reproduced candidate. Mocked services and bounded host integers
do not claim retail ABI, EH or inlining verification.

Full delinking/build passes at **4083/4764 exact**, **96.43% linked** and
**96.42% whole-image** (rounded), with one checkpoint raised, no MAX reset and
no lost banked RVA. The census is **216 overrides** (211 depth-zero / five
auto-inline-off) and **63 unions**. This removes one override and restores
one exact caller; it does not close ai_player or exhaust the remaining debt.

### Grail destination and shared map-extra boundaries

DC `check_holy_grail` (`ai_player.obj:0x32e30`) takes the destination vector by
reference, names its record `point`, and calls `game::get_cell` at line 3181.
`find_all_destinations` (`0x33038`) also takes that vector by reference and
calls `game::GetNumMapLevels`. Restoring these interfaces/accessors preserves
the initial **96.3651%** and removes the Grail map-lookup depth pin. The
intermediate object calls `NewfullMap::zCell`, whose complete 49-byte body is
identical to retail's folded `cell` target; this is not proof of a new inline
decision. The final fully recovered caller below calls `cell` by name.

The provisional `aiGetArtifactPlayerValue` was not a retail-only function.
DC `AI_get_value_of_artifact` at `0x37514` proves its `const type_artifact&,
long` player overload, including the empty-artifact guard, floor of ten,
hero walk and non-exact equip valuation. One canonical declaration now owns
the interface, including seerhut's formerly inconsistent int/int declaration.
The full build migrates the existing 0x433aa0 claim under its real overload
name without changing any score. Keep its ordinary body and distinct caller
decisions; no false `inline` declaration is introduced.

The initial boundary family (`070f8f532c43a477f273`) emits all 36 states, but
only its 18 pointer-signature states have comparable caller scores. The other
18 change the mangled function name while the runner still requests the old
name, producing false zeroes. They are not codegen losses or valid ranking
inputs. The reference migration was instead verified by full labels/delink/
build. The generator now defaults to those 18 name-preserving states;
`--signature-controls` retains the original diagnostic interface choices.
Run this generator against the pre-adoption `182b7a26` source.

`generate-ai-grail-lifetime-family.py`, context `23ae2167ec23afbe3de3`, exhausts
**24 states / 11 objects**, with ten reproduced elites. It tests actual artifact
construction and player/friendly-distance lifetimes after the interface repair.
Removing the artifact pin alone gives **90.7841%**; the original typed temporary
without the later caller-boundary recovery gives **61.8460%**. These are local
controls, not grounds to reject the positively evidenced typed constructor.

DC `AdvMgr.h:1254`, `advmgr.obj:0x1f084`, proves the inline, int-returning
`GetMapExtra(type_point)` overload forwarding x/y/z to the unsigned-short
scalar accessor. It follows the class's `get_map_center` at line 1245. Move
that body from advmgr.cpp to its original header position after the class,
remove findpath.cpp's falsely static duplicate, and restore the two point
calls at destination lines 3300 and 3359. The scalar declaration agrees with
kb.h and the retained exact 0x4f79b0 body.

`generate-ai-grail-map-extra-family.py`, context `3512473cbfdffe1a9642`, tests
**24 states / 22 objects** and all **2340 tracked scores across 34 header
consumers**, with **ten reproduced elites**. Its unchanged-source and
opposite-corner repeat controls pass.
The fully canonical corner **`[5,3]` / `ecae36eaf4a6f8ae1a792ae7`** reaches
**96.9365%** with both point calls, the typed artifact temporary and neither
Grail pin. Restoring only one point call leaves the unpinned caller at
91.0238% or 91.3555%; keeping a named point copy at the first site with the
typed artifact temporary also gives 91.0238%. Both original boundaries and
the temporary lifetime matter together. No dummy operation, copied helper,
replacement suppression or scoring change is used.

The chosen caller agrees with retail on **81 CFG blocks**, including every
block's flow and instruction count, and has **29 calls with no one-sided
site**. The formerly missing `vector::size()` call at +0x3fd returns naturally.
The map, patrol and artifact calls are retained without intervention: base
+0x696/+0x6ba/+0x741 correspond to retail +0x694/+0x6b8/+0x73f. Eighteen call
targets agree by name; the other eleven use five folded-template identities.
Their complete bodies are independently byte-identical to their retail
targets: `size` (19 B), `_Ucopy` (73 B), `_Ufill` (64 B), `_Destroy` (3 B),
and `insert` (778 B). Only insert has relocations, and both allocation/delete
targets also agree. No new alias normalization is used. The remaining
instruction differences are register allocation and bitfield scheduling,
not the former Grail inline mismatch.

The header-only control independently raises `CEnterNameEdit::onKillFocus`
from 99.8710% to 100%; no other tracked score changes outside the destination
caller. Raw inspection is stronger and slightly different: **31 complete
consumer objects** preserve all section bytes, function locations and
relocation destinations. ai_player changes only the destination function's
body, but also moves the unchanged `zCell` COMDAT past intervening sections;
that section-index translation accounts for all other bytes, function
locations and relocation destinations. singleselectionwindow changes only
the focus handler. kb additionally
changes four bytes in `oldmain`, despite its unchanged 78.4499% score: the
EDI/EBX spill homes at +0x129a/+0x129d exchange `[ebp-0x20]` and `[ebp-0x10]`,
with corresponding reload order at +0x1372/+0x1375. No intervening instruction
accesses either home. All **3063 kb relocation destinations** and all function
locations remain identical. Do not describe all score-flat consumers as raw
byte-identical.

`test-ai-grail-boundaries.py` exercises actual helper source at native `-O0`
and `-O2`: **20,480 cases per form** check eligibility, ordered calls, artifact
fields/owner, coordinate flattening, friendly-cost ties, movement floors,
victory value and existing-vector-prefix preservation. Five deliberately
wrong controls must fail. All 24 lifetime forms and the reproduced selected
corner pass. A separate **2401-case** point-overload oracle covers signed
10/10/4-bit coordinates and unsigned-short-to-int result widening. The
fixture mocks services and does not certify VC6 ABI, EH or inlining. Default
execution checks current source; `--source` selects a snapshot, and
`--all-forms` requires the pre-adoption lifetime-family anchors.

All **34 production objects** strictly reproduce the selected independent
repeat, including section bytes, function locations and relocation
destinations. Full retail delinking/build passes at **4084/4764 exact**,
**96.43% linked** and **96.42% whole-image** (rounded). One checkpoint rises;
no source edit lowers MAX and no banked RVA is lost. The census is **214
inline overrides** (209 depth-zero / five auto-inline-off), across **28 TUs**,
and **63 unions**. Neither Grail pin remains, and ai_player has no inline
override left. This is not a claim that its source or remaining functions
are complete.

### Scenario deselection: ordinary body ownership, not an inline fence

`generate-scenario-deselect-family.py` crosses the base handler's current
header definition / recovered ordinary definition with the scenario caller's
existing fence / natural call. The source fact is **window.cpp:1122/1123,
dc 0x197f48**, not the existence of a folded retail body. Dreamcast's public
`?OnWidgetDeselect@CHeroWindowEx@@MAAHHAA_N@Z` also proves protected virtual
access and `bool&`; five override publics carry the same reference type.
Complete's custom-campaign override shares the independently proven slot 12.
Restore that entire interface, including WindowHandler's local flag and
reference call, before taking the search checkpoint. This avoids scoring
newly mangled methods against stale names.

The interface-only full build migrates six claimed symbols with every score
unchanged. Across all 152 raw objects, **9949 executable sections** retain
their bytes and function locations; relocation names change only by the
seven explicit callback signatures and existing anonymous-namespace nonces.
The herodefs zero-filled BSS's anonymous-static permutation changes its
padding/extent (1156 to 1152 bytes), not executable instructions.

Context `ea8854cffaf04c674e5d` exhausts **four successful source states** and
produces **three distinct whole-consumer code identities**. It scores all
**90 configured window.h consumers / 3964 rows**, determined from the fresh
Ninja dependency closure. All three retained identities reproduce their
scores and code independently; the watched authored source stays unchanged
throughout the run. The scenario callback at 0x5698a0 gives:

| Base handler definition | Existing fence | Natural call |
|---|---:|---:|
| Current implicit-inline header body | 100% | 58.25% |
| Ordinary window.cpp body, in original source order | 100% | 100% |

The ordinary body's two caller forms produce identical objects. The chosen
natural call preserves all 30 retail instruction bytes and the sole call at
+0x15 (relocation +0x16). Its callee emits `33 c0 c2 08 00`, independently
identical to the five bytes at retail 0x559140. That address already belongs
to `ResourceManager::t_stdio_file_adapter::write`; retain the existing single
claim, with a documented `DC_ONLY` definition for the folded base handler.
The same body need not remain implicitly inline merely because ICF selected
another owner. Its source position is after WindowHandler and before
GetRolloverWidget.

Shared-header collateral is small but real: quest-creature `generate` in
rmg goes **99.7349% to 100%**, and `CEnterNameEdit::onKillFocus` goes **100%
to 99.871%**. Both already have MAX/HIST 100%; their own source hashes are
unchanged. Every other scored row is unchanged. The RMG change swaps two
independent loads at +0xca/+0xcd; the focus handler exchanges EDI/ESI spill
homes and their corresponding reload order. kb's score-flat `oldmain` also
changes four bytes, exchanging EDI/EBX spill homes at +0x129a/+0x129d and
reload order at +0x1372/+0x1375, without an intervening access to those homes.

The raw-object review accounts for **13,116 retained sections and 94,778
relocations**. Apart from those twelve instruction bytes, section bytes,
function offsets and named relocation destinations agree after the observed
section-index translation. Twenty consuming TUs stop emitting the base's
header COMDAT and associated `.debug$F`; window.cpp retains the same body at
its proper source position. Compiler-generated EH/branch labels are compared
by their actual section/offset destinations, including the four renumbered
`hero_rollover`, `town_rollover`, `generic_help` and `ignore` labels. No new
score normalization or second symbol ledger is involved.

`test-scenario-deselect.py` extracts the actual base, scenario callback and
WindowHandler bodies. At native `-O0` and `-O2`, **262,160 direct callback
checks** cover both initial flag values, all signed-16-bit IDs and four
outlying IDs; **864 dispatcher cases** check message priority, callback
arguments, flag initialization and end-dialog message mutation. Five wrong
controls must fail: wrong base return, wrong acceptance ID, dropped flag,
skipped callback, and wrong end-dialog return. The current interface and
selected frozen body both pass. This reduced fixture tests semantics, not
VC6 object layout or inlining; `--source` selects a frozen source tree.

The full adopted build passes every gate at **4084/4764 exact**, **96.43%
linked** and **96.42% whole-image** (rounded). All 4764 RVA rows survive;
the only two current-score changes are the collateral described above,
and no MAX/HIST value falls. All **90 production objects** reproduce the
selected independent recompile's executable bytes, function locations and
**94,779 relocation destinations** across **13,118 sections**. The only
raw non-executable layout difference is herodefs's zero-filled anonymous-
static BSS permutation/padding (1148 versus 1156 bytes). Fresh source and
structure diagnostics confirm all three scenario blocks and statements
agree; the named-call difference is precisely the independently byte-proven
ICF twin, not an absent call or a replacement helper. The census is **213
overrides across 27 TUs** (208 depth-zero, five auto-inline-off), with the
**63 unions** unchanged.

Integration with the independently landed ownership cleanup (`4b75e8ce`)
is separately full-built at **4086/4764 exact**, **96.44% linked** and
**96.43% whole-image**. Its two newly exact functions remain exact; all
incoming MAX/HIST peaks and RVA rows survive. Compared with that incoming
branch, only the same RMG/focus-handler current scores above move. Callback,
owner-payload, horde-row and DrawBolt native/negative controls pass on the
combined source. The original four-state experiment remains evidence for
its frozen pre-integration inputs, not a claim to have searched this new
header context.

## Lobby map-header receiver and dispatcher overrides

The dispatcher at `0x5887a0` formerly constructed a bare `NewSMapHeader`
under a depth-zero fence but never read the incoming message. DC source line
6529 calls the ordinary `OnNewMapHeaderInfo` helper, with its definition at
line 6968 (`0x140d50`). That older helper already calls `SetupOrigData` and
returns true; Complete's arm adds the serialized receiver and its lifetime.
Retail `+0x310..+0x365` calls the base constructor, constructs the header at
offset `+0x18`, installs vtable `0x641d30`, calls the receive bridge at
`0x512e00`, calls `SetupOrigData`, and destroys the header. The previously
modeled `CNewMapHeaderInfoMsg` owns exactly this base and member.

First restore the dispatcher's original public `QAA_N...AA_N` signature as
`bool handleNetMsg(CNetMsg*, bool&)`, including its caller's local and all
cancel assignments. The whole interface edit preserves every section byte,
function location and relocation destination across its four consuming TUs;
only the independently evidenced symbol rename is admitted. The full
checkpoint has no score movement before searching.

```sh
PYTHONPATH=scripts python scripts/experiments/generate-lobby-map-header-family.py build/lobby-map-header-family.json
PYTHONPATH=scripts python -m homm3.vc6.source_families build/lobby-map-header-family.json --width 60 --keep 4 --jobs 4 --generations 1
```

The two axes cross the incomplete/pinned arm with the ordinary complete
receiver, and the existing `HeaderRequested` auto-inline override with its
removal. All **four states emit distinct code and reproduce**; all **416
tracked functions across four header consumers** are scored. The receiver
with the adjacent override retained raises `HandleNetMsg` **89.7408% to
90.0449%**. Removing that auto-inline override scores **85.9113%** with the
old arm and **86.1394%** with the recovered arm. Both complete-receiver
states restore `CEnterNameEdit::onKillFocus` **99.871% to 100%**. No other
tracked score moves. The retained ordinary helper has no false `inline`
keyword, copied caller body or replacement override.

The recovered arm's first **62 raw bytes** (`+0x310..+0x34e`) match retail,
including its named constructor, receive and setup calls. The vtable and
game-global relocation spellings differ only by their existing source-owned
identities. Cleanup still expands two string `_Tidy` calls and
`~CMapHeaderData`, whereas retail calls `~NewSMapHeader`. Across the four
units, **811 existing executable sections are byte-identical**; the changed
ones are the dispatcher and its EH cleanup plus four spill/reload operand
bytes each in `onKillFocus` and the score-flat `kb::oldmain`. The latter
changes swap stack homes while preserving the corresponding reloads. The
ordinary helper adds its own body and EH cleanup, not a duplicate retail
claim. All four adopted production objects reproduce the selected candidate
across **1,448 sections and 15,089 relocation destinations**.

After adopting that model and running a fresh full build, the follow-up
verifies exact parent source/header identity and carries both reproduced
complete-receiver parents. It tests each of the remaining 14 dispatcher
depth-zero regions separately and all together, crossed with the adjacent
auto-inline override:

```sh
PYTHONPATH=scripts python scripts/experiments/generate-lobby-dispatch-pins-family.py build/source-families/PARENT_CONTEXT/checkpoint.json build/lobby-dispatch-pins-family.json
PYTHONPATH=scripts python -m homm3.vc6.source_families build/lobby-dispatch-pins-family.json --width 60 --keep 10 --jobs 4 --generations 1
```

All **32 states produce distinct objects**, with **ten reproduced elites**.
All **223 TU score rows** are checked; only the dispatcher moves. No deletion
preserves 90.0449%: single-region deletions span **79.9839% to 88.9839%**;
all depth regions removed scores **41.8479%**, or **38.2972%** with the
auto-inline override also removed. No follow-up deletion is adopted. This
does not exhaust combinations of arbitrary depth regions or other recovered
helper models. Contexts `e95ac2ee39002ff3643c` and `5d5ccc52816c48107f9d`
identify the frozen four-state and follow-up inputs. Their generators are
historical pre-adoption controls and deliberately reject changed anchors.

`test-lobby-map-header.py` extracts the actual helper, receive bridge and
virtual-reader bodies. Its reduced fixture checks **1,920 cases** spanning
sender identities, short-message lengths, read results and exceptional exit
paths, preserving construction/read/setup/destruction order and the input
message. Six controls must fail: skipped receive, skipped setup, reversed
receive/setup order, wrongly rejecting a read failure, wrong header version,
and wrong helper result. Both the frozen candidate and adopted source pass
at `-O0` and `-O2`. This is not a retail ABI or complete wire-format test.

The full checkpoint passes at **4087/4764 exact**, **96.44% linked** and
**96.43% whole-image**, with every RVA and MAX/HIST peak retained. The fresh
census is **212 overrides** (207 depth-zero, five auto-inline-off), across
27 TUs, with **63 unions** unchanged. Remaining destructor/inliner and
flattened-helper differences are open reconstruction work, not TU closure.

The search never writes authored source, CUR, MAX or HIST. Different function
implementations must not be banked under an old source hash. Review a retained
candidate, apply the actual C++ change, then run `homm3 build` to regenerate the
normal checkpoint and README. This preserves `CUR <= MAX <= HIST` and keeps
historical peaks separate from the current implementation's MAX. Completing a
family or getting every currently tracked row exact does not prove whole-TU
source completeness.
