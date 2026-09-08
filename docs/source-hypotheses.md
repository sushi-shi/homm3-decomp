# Source-hypothesis batches

`homm3 hypotheses` (also available as `homm3 vc6 hypotheses`) adapts King's Field's `kf hypotheses` manifest and
Cartesian source renderer to the configured HoMM3 VC6 profiles and normalized
objdiff scoring. Each option describes a reviewed, exact source substitution.
Independent axes combine; five binary axes produce 32 candidates.

```json
{
  "schema": 1,
  "unit": "bitmap16",
  "function": "?reference@Bitmap16Bit@@QAEXHHHPAG@Z",
  "axes": [{
    "name": "extent_product",
    "find": "    m_imageSize = w * h * 2;",
    "options": [
      {"name": "baseline"},
      {"name": "pixel_size", "replace": "    m_imageSize = w * h * sizeof(unsigned short);"}
    ]
  }]
}
```

```sh
homm3 hypotheses /tmp/reference.json -j 8 --keep-top 10
```

Functions accept exact scored symbols or unique symbol substrings within the
unit. Each `find` must occur exactly once. An option may have atomic
`extra_edits`, each containing `find` and `replace`; axes must not overlap.
Duplicate sources compile once, and `--limit` bounds the Cartesian product
before rendering (default 256).

The runner compiles a canonical baseline, then candidates in parallel, directly
through the TU's normal compiler wrapper. Each compile has its own source and
object directory. All scored functions in that TU are recorded to expose
collateral. Ranked results, input fingerprints, top sources and their objects
live under `build/hypotheses/`, or the new directory specified by `--output`.
`results.json` contains the canonical `baseline`, ranked `results`, source/target
hashes and the shared-input fingerprint. Raw and normalized objects are retained
for the top candidates. Source, ledger, headers, compiler and scoring inputs must stay unchanged during
a batch. Exit 0 means at least one target scores exactly 100%; 1 means no exact
candidate. Malformed manifests and invalidated observations are errors.

Run the required retail/Dreamcast evidence pass before designing hypotheses.
Scores do not authorize changing proven source facts. The runner never applies
or banks a winner; inspect the source and assembly, apply supported changes,
then run the normal focused build and full checkpoint gates. Vendored sources
remain pristine. Compiler-state padding and artificial ODR uses are not source
hypotheses.

The RMG river generators produce 60 reviewed alternatives per matrix from the
current source. Unlike transient include variation, they change the function's actual
C++ and reject a baseline outside their reviewed family:

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-river-overlay-hypotheses.py \
  build/rmg-river-overlay-hypotheses.json
homm3 hypotheses build/rmg-river-overlay-hypotheses.json -j 6 --keep-top 10
PYTHONPATH=scripts python scripts/experiments/generate-rmg-river-tile-hypotheses.py \
  build/rmg-river-tile-hypotheses.json
homm3 hypotheses build/rmg-river-tile-hypotheses.json -j 6 --keep-top 10
PYTHONPATH=scripts python scripts/experiments/generate-rmg-river-tile-hypotheses.py \
  build/rmg-river-tile-refine-hypotheses.json --refine
homm3 hypotheses build/rmg-river-tile-refine-hypotheses.json -j 6 --keep-top 10
```

The overlay family varies real map/cell addressing and nonzero-predicate
lifetimes. Retail writes river presence at tile-data bit 29; bit 30 remains a
separate routing target. A byte-sized predicate recovers all 87 bytes at
`0x532730`, with no other RMG score changed. Testing only the truncated four-bit
river kind would be wrong for nonzero inputs such as 16 or 256.

The full tile family tests sprite snapshots, signed bounds storage and predicate
types. It preserves both clipped, y-major neighbourhood passes: mark the 3x3
area impassable, then clear routing targets on river-free cells in the 5x5 area.
Both passes depend on the full input kind, not the packed kind. Host-only tests
check every generated alternative against raw retail masks and independent
distance tests, including tiny maps, edges, non-boolean flip bytes and truncation.
Those tests validate behavior; the pinned VC6 build alone judges byte matching.
The `--refine` matrix keeps scalar sprite snapshots and varies the first loop's
cell/flag receiver around four bounds-lifetime parents. If the current source
belongs only to the other phase, its unchanged control is added without dropping
any of the 60 reviewed alternatives. Rebase checks recognize both phases.
Scalar snapshots and bounds records raised the writer from 79.11% to 98.48%;
binding the first loop's whole cell then recovered all 517 retail bytes at
`0x532520`, with every other RMG score unchanged. Addressing the nested flag
directly is the negative control: VC6 forms a field pointer rather than keeping
the cell base as retail does. No added helper or inlining pin is involved.

Already-tracked, non-exact RMG functions have focused polishing matrices too:

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-prototype-polish-hypotheses.py \
  build/rmg-prototype-polish-hypotheses.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-zone-placement-polish-hypotheses.py \
  build/rmg-zone-placement-polish-hypotheses.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-painter-size-polish-hypotheses.py \
  build/rmg-painter-size-polish-hypotheses.json
PYTHONPATH=scripts python scripts/experiments/generate-rmg-strength-polish-hypotheses.py \
  build/rmg-strength-polish-hypotheses.json
```

Pass each output to `homm3 hypotheses ... -j 6 --keep-top 10`. Each generator
emits 60 distinct **sources**, not a claim of 60 distinct compiler outputs.
The prototype-selector matrix crosses counter initialization, range binding and
terrain-index lifetimes while preserving filtering, checked mask access and one
ordered random selection. The zone-placement matrix tests declaration/copy
lifetimes while preserving signed arithmetic, `sqrt` and truncation. The painter
constructor matrix retains the assigned virtual query and vector APIs, varying
dimension snapshots and area sources. These three finite matrices did not raise
their 99.6581%, 98.0374% and 97.9205% baselines; do not resample them unchanged.
The selector's `--branches` follow-up adds a separate 60-case matrix of conditional,
split-insertion and boolean-result filters, receiver bindings and public insertion
APIs. It also leaves the original 99.6581% implementation best. The generator
preserves an unchanged control when rebasing between the two reviewed phases.
After the terrain adapter's signed-integer domain was recovered, the selector
generator and its oracle were rebased to integer kinds too; no candidate restores
the earlier enum inference. The recorded sweeps above predate that domain correction.

The transition-strength matrix instead varies real point ownership and query
lifetimes around its mismatched cache-call boundaries. It retains west/north/
east/south order, two separate cache queries when the terrain matches, and
logical halving. Its stateful native oracle changes cache replies between
queries and checks the entire ordered query/frame stream, not just the result.
Its helper-order follow-up crosses ten retained source parents with all six
orders of the existing packed-cell accessor, terrain wrapper and paired size
accessors. Each helper body and its evidence annotation move together, remaining
single and unchanged. This tests natural nested-body visibility, not a pin:

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-strength-polish-hypotheses.py \
  build/rmg-strength-order-hypotheses.json \
  --helper-order-from build/hypotheses/STRENGTH_BATCH/results.json
homm3 hypotheses build/rmg-strength-order-hypotheses.json -j 6 --keep-top 10
```

Both strength matrices retain 70.1029% as their best. All six polishing matrices
compiled successfully (360 source candidates), but no target or collateral score
rose. Their native oracles pass; no experimental source was adopted just because
it appeared among retained parents. Function-specific residuals stay beside the
functions, and reruns need a new evidence-based hypothesis family.

The simple object serializers have a joint 60-case buffer-lifetime matrix:

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-object-writer-polish-hypotheses.py \
  build/rmg-object-writer-polish.json
homm3 hypotheses build/rmg-object-writer-polish.json -j 6 --keep-top 10
```

It changes the artifact, resource, scholar and shrine writers atomically,
crossing buffer scopes, byte signedness, initialization and dword reuse. Every
candidate preserves the canonical base call, each virtual write and its width.
The native oracle compares the entire ordered wire record, including truncation,
reserved bytes and unchanged object fields. Eighteen candidates close all four
writers; separate scopes with ordinary `char`/`int` buffers are sufficient.
These scopes recover retail's dead argument homes and eight-byte frames.
The prior function-scoped forms are negative controls at 99.8033%, 99.4933%,
99.4146% and 99.4933%. No other RMG score changes in the selected candidate.
Inspect all four entries in each result's `scores`: ranking by the primary
artifact writer alone also retains partial winners that do not close its siblings.

The rectangle painter has a separate 60-case bounds/predicate/tile matrix:

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-rectangle-polish-hypotheses.py \
  build/rmg-rectangle-polish.json
homm3 hypotheses build/rmg-rectangle-polish.json -j 6 --keep-top 10
```

It keeps one mutable grid point, unsigned end-exclusive traversal, the existing
terrain predicate and canonical tile/frame helpers. Its native oracle checks
stateful terrain queries, operation order, selected frame arguments, zero flips,
empty rectangles and wrapping bounds. The source family targets the retained
`initializePackedCell` call where retail expands that nested cache fill; changing
the helper's identity or inserting a pin is not part of the matrix.
The first matrix lifts the painter from 69.9804% to 72.8497% with named
endpoints and `isPaintTerrain`, without collateral. Its top ten parents can
be crossed with six for/while/guarded-do loop forms before changing the source:

```sh
PYTHONPATH=scripts python scripts/experiments/generate-rmg-rectangle-polish-hypotheses.py \
  build/rmg-rectangle-loops.json \
  --loop-parents-from build/hypotheses/RECTANGLE_BATCH/results.json
homm3 hypotheses build/rmg-rectangle-loops.json -j 6 --keep-top 10
```

The parent source hash must still match; every parent is recompiled in the new
batch, alongside its separate canonical baseline. All 60 loop combinations
retain 72.8497%, so they do not resolve the nested cache-fill boundary. The
retained source uses ordinary `for` loops; no failed loop form is adopted.
