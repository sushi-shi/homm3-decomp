# The `vc6` area — a white-box model of the VC6 SP3 compiler

Binary-matching decomp plateaus on two wall classes that no source spelling
reaches: **inliner divergence** (the `/Ob2` budget is positional/sequential;
depth-2 stops go both directions vs our CL) and **register-allocation
tie-breaks** (whole-body ESI/EDI role swaps, memory-homing, spill-to-dead-
parameter-slot, bidirectional constant-CSE). This area stops grinding those by
*modelling the compiler itself*: reverse-engineering the pinned CL / C1XX / C2
binaries and turning their inliner and allocator into predictions, with solvers
that plug into the match loop.

Shape copied verbatim from the sibling homm2 `/Od` stack-slot model
(`~/Projects/homm2/homm2-decomp/docs/od-stack-layout.md`): **model doc /
predictor / real-compiler oracle / census gate**. The predictor is pure and
compiler-free; the oracle compiles known source with the real toolchain and
reads the answer back; the census gate re-checks every matched function so the
model cannot rot.

## Provenance hygiene (hard rules)

- **Binary-only RE.** Never consult leaked MSVC source trees. The admissible
  evidence is bytes and strings *inside our own pinned binaries* (they carry 48
  back-end source-file paths and the option tables in the clear) plus the
  infinite ground truth of compiling known source and reading the result.
- **The pinned pressing is the only subject.** `_toolchain.PINNED` hard-gates
  CL/C1/C1XX/C2/LINK by sha256+size; a wrong-hash binary aborts. The model is
  only valid against those exact bytes (SP3: C1/C1XX 12.00.8472,
  C2/LINK 12.00.8447). The one RTM addition (C2 12.00.8168, Track R) is
  admitted separately, hash-pinned, and §5-logged.
- **The Wine VC6 build stays the sole verdict.** A model prediction is a
  hypothesis; a compile settles it. Solvers propose edits — the human applies
  them after checking the evidence.

## Layout

| Path | Role |
|---|---|
| `scripts/homm3/vc6/` | the area package (`homm3 vc6 <verb>`) |
| `scripts/homm3/vc6/_toolchain.py` | hash-gated PE reader over the compiler binaries |
| `scripts/homm3/vc6/disasm.py` | labeled C2 assembly and code references; inferred roles read from the owning evidence prose |
| `scripts/homm3/vc6/register_trace.py` | `trace-registers UNIT --fn NAME`: verified temporary-binding snapshots with function and compiler-site labels; limited to two documented stores |
| `scripts/homm3/vc6/argv.py` | CL spec-table decoder → per-pass argv model |
| `scripts/homm3/vc6/passes.py` | run C1XX / C2 as separate steps (IL persistence) |
| `scripts/homm3/vc6/oracle.py` | real-compiler ground-truth runners |
| `scripts/homm3/vc6/{inline_model,reg_model,il}.py` | the predictors + solvers |
| `homm3 vc6 il-locals UNIT --fn NAME` | candidate local handles from GL-recorded SY body offsets, using the canonical source/profile; named-symbol overlay, not optimizer register order |
| `scripts/homm3/vc6/{diagnose,report,queue}.py` | one-function routing, plateau report, and recoverable-byte wall census |
| `scripts/homm3/vc6/tu_state_sweep.py` | resumable random-include search once per TU with `MAX < HIST` rows, recording all function scores |
| `scripts/homm3/vc6/hypotheses.py` | [reviewed Cartesian source batches](source-hypotheses.md), adapted from King's Field, compiled in parallel with the unit's VC6 profile |
| `scripts/homm3/vc6/source_families.py` | generated C++ source-family axes, 50–60 candidates per generation, top-N refinement and all-TU collateral scoring; see [source-families.md](source-families.md) |
| `scripts/homm3/vc6/_source.py` | the solvers' source-body locator (demangle + definition grammar + `#if 0` masking) |
| `scripts/homm3/vc6/_eh.py` | the EH cleanup transcript (`[ebp-4]` state stores) — object lifetimes, the one signal the three solvers do not read |
| `scripts/homm3/vc6/census.py` | the gates (each with a negative control) |
| `scripts/homm3/vc6/test_locator.py` | the `locator` gate's cases (`homm3 vc6 check --locator`) |
| `scripts/homm3/vc6/test_disasm.py` | compiler label provenance, selector, byte identity and range controls (also in the `locator` gate) |
| `scripts/homm3/vc6/test_report_resolution.py` | negative controls for shared public-symbol routing and unclaimed flat names |
| `scripts/homm3/vc6/test_queue.py` | negative controls for MAX-first ordering and banked-exact dip exclusion |
| `scripts/homm3/vc6/shim/` | the C2-slot pass-through/instrumentation DLL |
| `scripts/homm3/vc6/ghidra_scripts/` | in-Ghidra headless scripts (no `__init__`) |
| `scripts/homm3/vc6/probes/` | one probe TU per catalogued behaviour |
| `docs/vc6/behavior-catalog.md` | the model's spec: ~80 byte-verified behaviours |
| `docs/vc6/driver-passes.md` | the CL spec-table mini-language + argv model |
| `docs/vc6/{inliner,regalloc,il-format,c2-atlas}.md` | one model doc per subsystem |
| `docs/vc6/eh-cleanup.md` | the EH cleanup-count rule + the tree-wide transcript divergences |
| `docs/vc6/debug-lines.md` | classic COFF source-line encoding and verified `/Z7` controls |
| `evidence/vc6/*.tsv` | generated tables (regenerate, never hand-edit) |
| `build/re/vc6/` | the Ghidra project (gitignored scratch) |

## Residual-routing contract

Before diagnosing a missing emitted body, compare the claim label with the
COFF symbol. The terrain table reconstruction exposed a missing `operator+=`
join key: `TRmgGridPoint` still emitted its exact 33-byte `??Y` body, but the
generic source label no longer paired it. The bounded arithmetic-operator
scanner now joins `+=` by owner and operation; its equal-size, reversed-order
test prevents a positional match from hiding this error. This is a label
binding issue, not evidence for changing the compiler's inline decisions.

`homm3 vc6 queue` ranks existing compiled functions by ascending banked MAX for
their current source implementation, with retail size breaking ties. Current
scores do not change the order while the source hash is unchanged. A source
edit resets MAX while HIST preserves the old peak, so `HIST > MAX` exposes
known recoverable headroom instead of hiding it from the queue. The census
records recoverable bytes as remaining work. `--diagnose`
adds solver routing; `--admission` explicitly lists functions without compiled
bodies. Both modes write generated evidence.

`homm3 vc6 queue --smallest` is the bounded quick-match campaign view. It
combines admitted residuals and unadmitted retail targets by RVA, sorts them by
retail byte size, and omits RVAs recorded in `config/simple-match-parked.tsv`.
It uses the current implementation's MAX for exactness and keeps HIST visible
as lost-peak evidence. Its generated output is
`evidence/smallest-match-queue.tsv`.

`homm3 vc6 state-sweep --trials 30 --jobs 8 --bank` groups every numeric
`MAX < HIST` row by TU. Each trial adds one shuffled set of five to ten project
headers absent from that TU's transitive project-header closure, then scores
every compiled function in the TU from that single object. Per-trial records
capture the selected order and every function score, while the summary lists
every score movement, including drops. Results are cached by source, retail
target, header contents, compiler/profile, normalization code and inputs,
generator version, and seed. A higher observation
is compiled a second time before it can raise MAX; the clean CUR is never
replaced. Authored source and function hashes must remain unchanged for the
whole run. See [tu-state-sweep.md](tu-state-sweep.md) for the audit contract.
Manual follow-up for rows that remain below HIST is recorded in
[manual-hist-recovery.md](manual-hist-recovery.md).

Before invoking disassembly, the router requires a unique emitted function
shared by the compiled and delinked objects. File-static functions qualify;
undefined symbols and local assembly labels do not. A missing comparison body
remains an inspectable diagnosis failure.

The shared-symbol preflight is asymmetric in the useful direction: exact
mangled identities and uniquely resolved source basenames may enter the
register/control-flow/inliner solvers, while ambiguous substrings and symbols
present on only one comparison side may not. `homm3 vc6 check --locator`
includes the measured flat-label defect and the one-side-only case as negative
controls, so the census cannot silently regress into treating missing source as
a compiler wall.

Tree erasure also requires the exact overload identity before compiler-state
diagnosis. `TREE_ERASE` claims private `_Erase(node)`; `TREE_ERASE_KEY` claims
public `erase(const key_type&)`, which returns `size_type`. The iterator and
range overloads retain `TREE_ERASE_ITERATOR` and `TREE_ERASE_RANGE`. The key
overload's VC6 signature tail starts `QAEIAB`, distinct from the iterator
return ABI. RMG terrain's 89-byte `erase(key)` at `0x5b7f60` is exact when
paired with that naturally emitted symbol; the old private-helper probe scored
36.62%. `test_tree_member_keys.py` checks the signatures, both kind registries,
and actual claim joins with deliberately equal-sized overloads.

The RMG branch queue at `0x543e20` proves `std::list<TPoint>` through its
coordinate arithmetic and 16-byte linked nodes. `LIST_DTOR`,
`LIST_INSERT_SINGLE`, `LIST_ERASE_ITERATOR`, `LIST_ERASE_RANGE`, and
`LIST_BUYNODE` name its ordinary Dinkumware members. These are direct COFF
symbols, so both the claim parser and the canonicalizer register them as such.
The two erase keys use their iterator argument suffixes, independently of
object size and emission order. `test_list_member_keys.py` checks shuffled,
equal-sized claim joins, a second element class, and unrelated container and
insert overloads. A missing range-erase COMDAT remains an inlining question;
it must not be paired with the retained iterator overload.

`TREE_CONST_END` identifies the const `_Tree::end()` overload by its const
member signature and `const_iterator` return type. Its fifteen-byte body
copies `_Head` through a hidden result pointer. `getDisplayFace` reaches
100% through a const map reference: all eighteen const lifetime/guard forms
retain retail's two calls within `find()`, while the fifty-four mutable
forms plateau. This is an overload-selection boundary. Keep the source
lookup const and let the vendor implementation make its own inline choices.
The same correction makes `makeHeroFilter` exact when its const reference
stays inside the hero loop; hoisting it outside changes register homes and
scores 90.13809%. Its mutable controls remain below 89%.
`test_tree_member_keys.py` checks distinct owners, equal-sized joins, and
negative controls for mutable end, begin, and unrelated containers.

`STD_CONSTRUCT` also recognizes VC6's scalar placement-construction overloads.
The byte helper at `0x48e9d0` is `?_Construct@std@@YIXPAEABE@Z`; its destination
pointer and const-reference source must encode the same builtin type. Keys
retain signedness and width instead of grouping scalar overloads under `std`.
`test_scalar_construct_keys.py` checks shuffled, equal-sized claim joins and
rejects conversion, qualifier, namespace, calling-convention and arity mismatches.

`homm3 sema diff --calls` and `--relocs` distinguish source-claimed retail
labels from unclaimed, generated and local labels using the regenerated
symbol inventory's provenance. A carcass `VA` already owns its retail name
even while its compiled declaration uses a different mangled symbol. The
report keeps that name difference visible and recommends checking the
declaration/relocation identity, rather than asking for the same claim again.
This annotation does not equate overloads, change reference pairing, or hide
addends. The summary and JSON views carry the same categories.

The resource cache now uses its actual `std::map<TCacheMapKey, resource*>`.
Its global lifetime naturally emits teardown, while the ordinary key comparator
and shared `getFromCache`/`addToCache` functions emit the locked tree searches
and insertion helpers. `_Lbound` at `0x55ebd0`, insert at `0x55dbc0`, `_Insert`
at `0x55e7e0`, and iterator `_Dec` at `0x55ec30` are exact. Handwritten STL
facades and an unused destructor-emission wrapper had obscured that ownership.

`remapGraphics` and `saturateGraphics` show why source accessor boundaries
must survive even when their bodies are only field reads. A for loop restores
the retained iterator increment, but flattened `getName`/`getResType` calls
still expand tree::begin and score 97.7907/97.9075%. Restoring the Dreamcast
header accessors makes both callers exact. Six iterator-construction forms
confirm the distinction; prefix/postfix choice alone does not fix it. The
shared palette getter also makes `loadFontData` exact where its manually
expanded cache path had stalled at 97.6539%.

`MAP_FIND` and named-key `MAP_INSERT` keep the public map layer separate from
the underlying tree. Their return signatures distinguish mutable find and
single-value insert from const find and hinted/range insert. A typed
`PAIR_CTOR` key (`cstr_resource_pair`) identifies the two-reference
`pair<const char*, resource*>` constructor independently of iterator/bool
result constructors. `test_map_member_keys.py` covers owner changes, equal-size
joins, and overload/type negative controls.

An empty destructor's final base-vftable store does not uniquely identify
its source class. At `0x487e00`, the implicit `TStreamBufFile` destructor,
both resource-adapter destructors, and `TAbstractFile` constructor-cleanup
copies have identical bytes **and relocations**. Retail shares that body:
the two resource-adapter deleting destructors call it and seventeen EH
funclets jump to it. `TStreamBufFile`'s retail vtable also uses the LOD
adapter's deleting-destructor copy at `0x55a7d0`. The natural representative
in customcampaign is `TStreamBufFile`; a separately defined ordinary base
destructor had obscured the fold. Restoring the canonical header-inline
base body matches both parked derived destructors and recovers six message/
resource consumers to 100%, while the retained folded body remains exact.
Eight `vc6 hypotheses` states checked absent/inline/ordinary body visibility
in gzfile, netmsg and customcampaign. Inspect emitted copies across the
consuming TUs before treating a missing inline symbol in one TU as evidence
against the declaration.

`TQuickHeroWindow` demonstrates why caller shrinkage must preserve recovered
source ownership. Dreamcast places its disguise scans in constructor scopes;
an unclaimed helper introduced solely to lower the constructor's inline cost
made VC6 retain `basic_ostream` instead of the nested `basic_ios::init`.
Restoring those scopes naturally emits the 71-byte initializer at `0x52f440`
with all three retail calls and relocations exact. An eight-state hypothesis
batch also established that ordinary quantity-widget `push_back` raises the
constructor to 94.1662%, and removing the old mana inline-depth pin is flat.
Its remaining mana-string cleanup expansion is documented beside the caller.
The `BASIC_IOS_INIT` claim accepts the complete protected char-stream,
two-argument signature; overload, stream-type, traits, access and qualifier
negative controls prevent it from claiming an enclosing constructor or a
different initializer.

## Status

Phase 0 (driver ground truth + probe rig) is in progress. Reusable compiler
findings live in this directory; function-specific failed probes are recorded
beside the affected source function.
