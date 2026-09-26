# Mac second-target tooling

The [full-TU build and executable inventory plan](mac-full-tu-build-and-inventory-plan.md)
builds CodeWarrior objects from the canonical source TUs and scores Mac pairs
from them. Ordinary-header compilation status is recorded in the
[native-header status](mac-native-headers.md).

## What is scored

A pair is a source `MAC_ADDRESS` claim with a Windows VA whose definition the
unit's full-TU object (`build/mac/obj/<unit>.o`) emits. `homm3.mac.emitted`
joins the claim to that hunk by qualified name, telling overloads apart by
their mangled parameters, including when only one same-named symbol exists.
Unsupported parameter types and platform-specific signature differences stay
unscored until the join can verify their identity. `homm3 mac build` (run by `homm3 build`) links the
hunk at the claimed address and compares it with the pinned PEF; the Mac
CUR/MAX/HIST ledger `config/mac/match_baseline.tsv` is keyed by the VA, and a
VA not scored in a checkpoint keeps its previous row. MAX follows the
definition's own token fingerprint, as in the Windows ledger.

Call targets resolve from the same join: every symbol a full-TU object emits
or references names the claim of the one definition it mangles, plus the
`runtime-map.tsv`, `runtime-aliases.tsv`, `zlib-map.tsv` and loader-proven
import glue labels. A pair whose body still references an unresolved call or
TOC datum is reported as unavailable, never masked into a score. A claim whose
unit does not compile, whose body is not emitted, or whose overload is
ambiguous is reported as unscored in `build/mac/report.json`.

Reports compare retail and candidate **call sites and ordered targets**,
alongside bytes and scores. Counts are static instructions, not runtime
execution counts or proof of inlining. Indirect calls and unknown targets
remain explicit.

Mac is a helper target for Windows source recovery, not a runnable-port
requirement. Diagnosed CodeWarrior source errors leave that TU unavailable;
other TUs can still provide evidence. Failed compilation removes its old
object, listing and symbol index. Toolchain, Ninja, timeout and disassembly
failures stop comparison and checkpointing rather than reuse stale outputs.
Direct `ninja mac-objects` still reports source compilation failures as errors.

`show`, `disasm` and `xrefs` select source claims without requiring an emitted
candidate. `diff`, `shape` and `calls` refresh the owning TU before resolving
its emitted body. This keeps the pinned Mac evidence available while a TU is
unbuildable; unsupported joins never become approximate byte verdicts.

```sh
homm3 mac build --fast <unit>          # score one unit's pairs (full build: all, with checkpoint)
homm3 mac show <Windows-VA>            # claimed span and source, even without a candidate
homm3 mac diff <Windows-VA>            # linked candidate vs retail, instruction by instruction
homm3 mac shape <Windows-VA>           # relocation-masked comparison; works before references resolve
homm3 mac calls [<Windows-VA>|<unit>]  # retail/candidate call sequences
homm3 mac labels                       # every scored pair
homm3 mac census                       # unclassified code scan
homm3 mac find --string "heroWindow"
homm3 mac xrefs mac:1:0x44bd4          # loader pointers and code uses of a data object
homm3 mac disasm mac:0:0xf6290 --size 0x40
```

Search windows and unclassified branch-pattern scans are discovery leads.
They can cross functions or contain inline data, and do not admit boundaries.

## Adding a pair

1. Identify the Mac body through strings, constants, field accesses, calls and
   data references, and verify both boundaries. `homm3 mac inventory` admits
   proven single-function spans to `config/mac/functions.tsv`.
2. Write the claim beside the Windows VA:
   `VA(0x004d8720, 0x568) MAC_ADDRESS(0x0f3fe4, 0x568)`. Run `homm3 mac parity`.
3. Run `homm3 build --fast <unit>`, then `homm3 mac diff <Windows-VA>`. If the
   pair is unavailable, `build/mac/report.json` names the unresolved callee or
   TOC datum; add the reviewed runtime label or data binding below.

TU/source order narrows the search, but retained helpers, template instances and
cleanup bodies can intervene; do not zip the two function lists by position.

## Constraints and limits

Source `VA(...)`, `MAC_ADDRESS(...)` and `DATA(...)` annotations own
identities and names; functions are addressed only in source. Data bindings
keep their reviewed inventories below; compiler-generated constants use
verified payloads, not unstable temporary symbol numbers.

`config/mac/data/<TU>.toml` uses the shared data inventory schema. A
`declaration_only = true` row identifies external storage through its source
`DATA` annotation and reviewed Mac loader/TOC evidence. Its digest verifies the
address anchor, not a candidate initializer or the complete object's extent.
Definitions, externs and reference-to-array declarations can own this identity.
TU-static definitions can also bind external storage in the comparison group.
For a class-static member, place DATA on its canonical declaration and supply
the emitted `mac_symbol` in its declaration-only row. Its qualified source
name is derived from the enclosing class; the Mac class view supplies the
declaration, without injecting an out-of-class definition or initializer.
An emitted candidate initializer cannot silently use this address-only path.
A CodeWarrior vtable can use a reviewed `config/mac/vtables/<TU>.toml`
binding. The binder checks the canonical and Mac virtual declarations, full
retail table and RTTI hashes, the loader pointer to the table, and every
transition-vector and code-body link. A full TU that defines the table itself
must emit the reviewed slot layout (RTTI, then one relocated word per slot);
its initializer is never scored against retail bytes.
When a global is actually defined in the compiled source TU and the Mac code
uses its direct TOC address, set `same_tu_definition = true` instead of
`declaration_only`. The source must contain one ordinary uninitialized DATA
definition such as `DATA(...) std::string g_resourcePath;`. The reviewed Mac
storage bytes and emitted candidate storage still have to agree. This option does not create a second source definition or claim a
runtime initializer match.
Use `units = ["<TU>"]` on literals when their equal payloads occur in distinct
linked pools; ambiguity remains an error. Identical worker data entries
coalesce and conflicting entries fail validation.
When several anonymous literals have the same payload inside one function,
add `owner_va`, `candidate_offset` and `target_offset` to each `[[constants]]`
row. The offsets are relative to the emitted MWOB hunk and the claimed Mac
function respectively. The resolver requires one anonymous reference at the
candidate site. A direct reference must load the declared TOC address; an
indirect reference must load through the exact retail TOC cell whose loader
pointer reaches the declared payload. Both forms check the target instruction
against the pinned PEF. An edit that moves an emitted use
must update its reviewed candidate offset before the comparison can run.
If one emitted literal is referenced more than once, list every use site;
unreviewed uses fail the comparison.
Reviewed anonymous literals may reside in a PEF code section. They still
require an exact payload/hash and a loader-proven TOC pointer; a code-section
location does not authorize arbitrary named storage or direct cross-section
TOC addressing.
Named const objects and arrays may also occupy code-section pools. Constness
is derived from the supported canonical declaration, never a manifest flag;
pointer-to-const and reference declarations do not take this route.

Library-owned data named by the pinned CodeWarrior headers uses a separate
`[[data]]` row in `config/mac/runtime.toml`, with its emitted symbol, reviewed
Mac section/offset/size/hash and evidence. The Mac view declares the same
external symbol; the resolver requires its candidate IL/TC reference and the
retail loader pointer. This binds a runtime address without claiming a game
source DATA owner or comparing a fabricated initializer. For example, MSL's
inline `toupper` reads the reviewed 256-byte `__upper_map` table.

Library call targets use `runtime-map.tsv` labels; game callees resolve from
their source claims.

A function-local `static const` DATA initializer uses `owner_va` to identify
its enclosing authored function. The emitted `name$counter` is resolved by its
source name and exact reviewed payload, without pinning the compiler's counter
or injecting another global definition.

Reviewed `[[jump_tables]]` records bind compiler-generated dispatch tables by
owning function, complete span/hash and retail TOC reader. All words must
relocate to instructions within the owner. Candidate labels are rebased after
reload removal, and code plus table bytes determine the score and exact verdict.
See the [report's verdict definition](mac-matching-report.md#what-an-exact-result-means)
for the report fields and current supported form.

CodeWarrior's `TB` exception tables are not compared. Function verdicts cover
resolved code and reviewed switch-table bytes; they do not claim matching
exception metadata or external initializers.

Dreamcast remains a source-fact reference for names, declarations and line
evidence. Mac is the second exact byte target and the primary additional view
of retained helper calls. Neither allows overriding contrary retail evidence.

See [the target design](../matching/mac-second-target-plan.md) for pinned inputs,
compiler calibration and exact-comparison constraints.


## Source addresses and executable inventory

Mac function addresses live in source beside their Windows claim, as
`VA(0x004d8720, 0x568) MAC_ADDRESS(0x0f3fe4, 0x568)`, or on their own line
directly above a definition with no Windows VA; `MAC_COMPGEN_ADDRESS` sits
beside a `VA_COMPGEN`. Offsets are relative to PEF code section 0; the macros
claim functions only. The executable-wide tables mirror `config/retail`:
`config/mac/functions.tsv` (every verified span), `runtime-map.tsv`,
`runtime-aliases.tsv`, `glue-map.tsv` and `zlib-map.tsv` (library labels with
provenance), `code-regions.tsv` (reviewed non-function regions),
and `dispositions.tsv`. `config/mac/units.toml` holds the CodeWarrior flags
and whole-unit dispositions.

```sh
homm3 mac parity             # validate claims and tables; index every source function
homm3 mac inventory          # every code-section byte in one region; boundary census
homm3 mac inventory --admit-proven   # admit proven single-function spans as unowned rows
ninja -k 0 mac-objects       # full-TU CodeWarrior objects; `ninja mac:<unit>` for one
homm3 mac objects            # per-TU compile state and first real error
homm3 mac emitted            # emitted symbols vs source; claim bodies; identity leads
homm3 mac emitted --admit-library    # label rows filled exactly by MSL/zlib hunks
homm3 mac dashboard          # the separate accounting numbers side by side
```

`parity` binds each claim through the analysis arm's annotations: a paired
claim to its VA, a standalone claim to the definition carrying the same `mac:`
attribute. `inventory` admits a span only when it starts at a transition-vector,
direct-call or reviewed vtable-slot entry, ends in a return or unconditional
branch, and every word is reachable from the entry and in-span jump-table
labels; its gate rejects a row that contains another function's entry. Object
leads are relocation-masked byte matches of full-TU hunks that occur once in
all code; they are identity evidence for library labels and leads for game
claims, never automatic `MAC_ADDRESS` edits.
