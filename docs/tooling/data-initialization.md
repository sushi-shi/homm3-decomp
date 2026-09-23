# Initialization and RTTI evidence

Full `homm3 build` now checks CRT registration tables, follows initializer writes
and direct calls, compares independently paired initializers, and accounts for
bounded VC6 RTTI/exception records. These results accompany the strict static
data comparison introduced in [#84](https://github.com/sushi-shi/homm3-decomp/pull/84).

```sh
homm3 build
homm3 sema data-initialization --output build/data-initialization
homm3 sema data-initialization --require-exact
homm3 sema coverage --output build/initialization-accounting
```

The ordinary checkpoint reports the existing initialization backlog.
`homm3 build --require-data-exact` now also requires initialization evidence to
be exact. Missing foundational CRT evidence fails the ordinary checkpoint;
unsupported effects and unpaired registrations remain explicit. The exactness
gate currently fails. Neither an all-zero BSS allocation nor static byte equality
can satisfy the initialization checks.

## Evidence flow

`sema/data_match.py::prepare` supplies the same fresh raw objects, source-bound
storage, entry anchors and pinned image to all data analyses. The initialization
pass does not read normalized objdiff objects or use function similarity scores.

`analysis/data_initialization.py` locates the named `__cinit` implementation in
pinned `LIBCMT.LIB` and compares it against its admitted retail entry. Its actual
relocations identify the two boundary pairs passed to `__initterm`. Each called
walker is independently checked against the pinned helper's complete bytes.
Aligned, bounded, sentinel-terminated slots are then read from those ranges.
Slots pointing outside code invalidate the table. A code target lacking an
admitted extent remains a registered but unbounded body.
The CRT's additional indirect startup hook is recorded separately as unverified;
proving its two table walks does not prove every startup effect.

This follows the CRT's documented compiler-generated pointer registration and
subsection ordering, but the measured addresses and sizes come from the pinned
VC6/retail bytes, not current compiler assumptions. See Microsoft's
[CRT initialization documentation](https://learn.microsoft.com/en-us/cpp/c-runtime-library/crt-initialization).

Candidate `.CRT$XI*` and `.CRT$XC*` slots must have an aligned, zero-addend DIR32
relocation to an emitted function body. Malformed slots and missing bodies have
separate statuses. Every emitted CRT slot is retained, including unsupported
families; section position cannot establish a retail identity.

`analysis/data_effects.py` propagates known integer values and stack locations
through reachable x86 instructions. At a control-flow join, only equal facts
survive. Calls invalidate volatile registers and unknown stack effects. Partial
register writes, overwritten arguments, unresolved relocations, indirect writes
and unsupported instructions cannot manufacture a known value. Push arguments
are tracked even when an earlier call's stack cleanup remains unknown.

`analysis/data_functions.py` resolves candidate code operands from raw COFF and
independent storage bindings. It follows bounded direct-call closures and records
reads, writes, call arguments and `_atexit` registrations. A registration records
its callback; it does not execute it. Unmapped callbacks, runtime effects, indirect
calls, tail edges and closure limits remain gaps.

Initializers pair by an admitted entry anchor or uniquely shared, independently
bound storage. The write comparison happens **after** pairing, so omitted fields,
wrong destinations and incorrect constants remain detectable. Ambiguous pairing
is retained as debt. Exact final byte effects require a fully supported
straight-line body with known writes and no calls. Complex loops and constructors
remain unproved; the current analysis does not substitute caller arguments into
callee bodies or claim general symbolic execution.

Order checks use section suffixes and slot order inside one object. They report
both the inversion and any observed read/write dependency bytes. A reordered pair
with no known dependency is not automatically a behavioral bug. Cross-object
execution order still requires a fresh linked candidate image.

## Bounded RTTI

`analysis/compiler_rtti.py` independently locates the `type_info` vtable from
its admitted destructor and the corresponding pinned CRT object. Its layout is
also documented by the pinned `TYPEINFO` header: vptr, cached name pointer, then
the decorated name. Exception and class record layouts follow the x86 Microsoft
ABI; LLVM's [Microsoft ABI implementation](https://github.com/llvm/llvm-project/blob/main/clang/lib/CodeGen/MicrosoftCXXABI.cpp)
provides an additional field-layout reference.

Roots come from reachable calls to the admitted `__CxxThrowException@8` with a
dataflow-proved type argument, validated EH catch maps, and candidate locator
pointers immediately before admitted vtables. Each entire root graph must
validate before any of its new records receive credit. Checked records include
TypeDescriptor, ThrowInfo, CatchableType/array, CompleteObjectLocator,
ClassHierarchyDescriptor and base-class descriptor/array. Counts, flags, bounds,
pointer storage, terminated names and the independently proved type_info vptr
are checked. Newer descriptor extensions are not guessed. Unconfirmed prefixes,
unsupported records and unknown throw arguments remain explicit.

RTTI and initializer tables add **retail structural accounting**, not candidate
byte-match credit. Logical descriptor sizes exclude unproved alignment padding.

## Generated TSVs

The full build writes these to `build/data-match/`. The standalone command and
exhaustive coverage command write the same tables to their output directory.

| File | Contents |
|---|---|
| `data-initializer-tables.tsv` | Proven CRT bounds, walker and call sites |
| `data-initializer-slots.tsv` | Every retail slot, sentinel and unbounded target |
| `candidate-initializer-slots.tsv` | Raw candidate registration and body status |
| `data-initialization.tsv` | Per-root writes, reads, calls, callbacks and effect gaps |
| `data-initialization-matches.tsv` | Pairing evidence, missing/extra/changed writes, registrations |
| `data-initialization-storage.tsv` | Each enrolled allocation's observed startup writers |
| `data-initialization-issues.tsv` | Missing pairing/body evidence and order differences |
| `data-rtti.tsv` | Validated records, exact extents, fields and pointer edges |
| `data-rtti-roots.tsv` | Runtime-call, catch-map and vtable-prefix provenance |
| `data-rtti-runtime.tsv` | Pinned archive and admitted destructor anchoring type_info |
| `data-rtti-issues.tsv` | Unsupported/rejected records and unresolved arguments |

Structured fields use JSON inside literal TSV. The summary JSON carries input
hashes, the pinned retail hash and explicit policy. `coverage.tsv` remains an
exhaustive partition; the new structure boundaries participate in the same
compiler overlay and conflict handling as the earlier EH/vtable layer.

## Measured checkpoint

| Measurement | Result |
|---|---:|
| CRT table bytes, including four boundary sentinels | 4,612 |
| Registered retail C++ initializers | 1,145 |
| Registered retail C initializers | 4 |
| C initializer bodies lacking admitted extents | 3 |
| Candidate registered initializers | 349 |
| Independently paired initializers | 19 |
| Pairs with proved identical constant effects | 12 |
| Distinct bytes of those effects | 230 |
| Bound allocations with retail startup writes not observed in candidate | 6 |
| Within-object order differences | 6, without observed dependency bytes |
| Validated RTTI/exception records | 324 / 6,867 bytes |
| Rejected locator candidates / unresolved throw arguments | 1 / 29 |
| Newly explained ownership gaps | **11,225 bytes** |
| Remaining unexplained ownership | **118,403 bytes** |

The static comparison still reports 77,545 matched initialized bytes and 101,207
bytes of zero-fill agreement. Dynamic effect bytes are a separate measurement.
Of the 19 paired initializers, seven still have unproved effects. Another 330
candidate registrations and 1,127 bounded retail registrations remain unpaired.
These counts are evidence coverage, not a claim that every unpaired initializer
is missing behavior.

The per-storage report exposes the context-selector startup write through the
same rule as other globals; neither its name nor address appears in the analysis
implementation. Similarly, the direction table's 64 bytes of constant startup
stores are proved equal without a table-specific rule. Independent table extents,
shared consumers and stride units remain the next [stack stage](data-matching-stack-plan.md).

## Validation

The full 152-unit VC6 checkpoint and 192 tooling tests pass. An independent audit
rechecks the exported input hashes, raw CRT registrations, RTTI graph extents,
and exhaustive file/image partitions. It also replays every exact initializer
pair from raw instructions with a separate constant/stack evaluator, reproducing
the 230 matched effect bytes. The strict initialization gate exits nonzero for
the recorded backlog, as intended.
