# The C2 register allocator - preference ranking, assignment order, why-reg v2

Phase 4 of the vc6 area, deliberately a MINIMUM SLICE: reverse-engineer
enough of C2's register-assignment ORDERINGS and TIE-BREAKS to predict the
behavior catalog's **B1** class (the whole-body ESI/EDI/EBX role swap - the
modal plateau wall), plus the corners the same mechanism explains for free
(B8 zero-CSE register choice, B14 EAX-first naming lever, B15's eager char
homing). Spill heuristics, coalescing and the full allocator are explicitly
NOT modelled. Subject: the pinned **C2.DLL 12.00.8447** (hash-gated via
`_toolchain.PINNED`, image base `0x10700000`; all addresses below are RVAs).
The initial measurements are from 2026-08-10; later findings are dated below.
All use the pinned binaries under Wine.

## TL;DR

* C2 numbers the eight GPRs **machine encoding + 1**: 1=EAX 2=ECX 3=EDX
  4=EBX 5=ESP 6=EBP 7=ESI 8=EDI. The numbers are the symbol HANDLES of
  eight pre-created register symbols in the back-end symbol hash.
* There is **one fallback preference order**, a 0-terminated dword table
  `{1,2,3,7,8,4,6}` = **EAX, ECX, EDX, ESI, EDI, EBX, EBP** (const in
  `.rdata:0xa09f0`, runtime copy in `.databe:0xadff4`), consumed
  **first-fit** by regasg.c. The selector can first honor a preferred register
  or rotate through EAX/ECX/EDX (section 3a). Call-crossing values lose those
  three registers; byte-sized values lose ESI/EDI/EBP, ESP is excluded, and
  EBP is available only when frameless.
* Small standalone probes follow **creation order**: three call-crossing
  values take ESI, EDI and EBX; swapping their creation order swaps bindings.
  This is the minimum model, not a complete allocator description. The
  real-TU trace in section 3b also shows priority ordering, interference
  constraints and register costs deciding callee-saved assignments.
* The IL `sy` stream exposes named locals' front-end handle order, but this
  is not a general optimizer allocation order. The first-assignment controls
  in section 3 and shipyard's passive observations below bound that
  interpretation. Moving a bare declaration can change the front-end handle
  while leaving every register decision unchanged.
* The earlier B1 cases show register permutations with unchanged schedules.
  The minimum model labels some of these **C1 handle-state** cases when
  aliases are copy-propagated. That label is a hypothesis: unchanged
  schedules do not exclude changes to interference, costs or assignment
  priority, and do not uniquely establish a handle-order cause.

## 1. Method

1. **Navigation by the atlas.** `evidence/vc6/c2-tu-map.tsv` places
   regasg.c at `0x8941f..0x8cf95` (ICE anchor `0x8b906`) and color.c at
   `0x8e1f7..0x8f5e3` (anchors `0x8e474/0x8e877/0x8e9de`).
   `ghidra_scripts/regasg_probe.py` (read-only over the persisted
   `build/re/vc6/` project) dumped decompilation + disasm of both
   neighbourhoods and the register-set library they call into
   (`build/re/vc6/raw/regasg/`).
2. **Structure recovery.** The dumps expose a bitset-over-registers
   library (sorted 32-bit chunk lists; the iterator returns members in
   ascending numeric order), per-register state arrays indexed 1..8, and
   an allocatable-register test `reg && reg < 9 && reg != 5`.
3. **Table discovery.** A raw imm32 scan of the pinned bytes (same
   channel as the atlas corroboration - no Ghidra) located the dword
   sequence `1,2,3,7,8,4,6,0` twice: a const copy with a begin/end
   pointer pair, and a `.databe` runtime copy read exactly where the
   regasg.c decompilation walks a preference list.
4. **Identity proofs, two independent channels.**
   - The listing name table `.rdata:0xa9194` = `["eax","ecx","edx","ebx",
     "esp","ebp","esi","edi"]` is indexed `[0xa9190 + reg*4]` (byte-
     verified `8b 04 95 90 91 7a 10` = `mov eax,[edx*4+0x107a9190]` at
     `0x74b23` and five sibling sites, mdlist.c) - so internal number 1
     prints "eax", 8 prints "edi".
   - regasg.c `0x8c1dc` removes exactly registers {7, 8, 6} from a
     pseudo's candidate set when its width field is 1 byte - and
     {ESI, EDI, EBP} are precisely the three x86 GPRs with no 8-bit
     subregister.
5. **Behavioral oracle.** Standalone probes against the pinned SP3 CL
   (game profile `/O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS`)
   measured the assignment order, its overflow, the byte-width exclusion
   and the creation-order flip (section 4).
6. **Real-case validation.** The model was run against retail via the
   delinked target objects on two catalog B1 rows and one deliberately
   reopened B14 row (section 6).

## 2. Address ledger (C2.DLL 12.00.8447, RVAs)

<!-- c2-role: site 0x8be6c rebindRegisterPreferenceLoop -->
<!-- c2-role: site 0x8c1dc chooseCandidateRegister -->
<!-- c2-role: function 0x3356b bindTemporaryRegister -->
<!-- c2-role: site 0x3356e storeTemporaryBinding -->
<!-- c2-role: site 0x323fe storeIndexedTemporaryBinding -->
<!-- c2-role: global 0xac380 currentFunctionBody -->
<!-- c2-role: global 0xa09f0 initialRegisterPreferenceOrder -->
<!-- c2-role: global 0xadff4 registerPreferenceOrder -->
<!-- c2-role: global 0x9d6ec registerBindings -->
<!-- c2-role: global 0x9d6c8 registerConflictSets -->
<!-- c2-role: global 0xac730 registerDescriptors -->
<!-- c2-role: global 0xa9194 registerEncodingNames -->

These inferred role labels are supported by the reads and probes below.
The `site` labels name observation points, not recovered function starts;
their physical intervals may be fragments of larger compiler functions.

| what | where | evidence |
|---|---|---|
| preference table, const `{1,2,3,7,8,4,6,0}` | `.rdata 0xa09f0..0xa0a10` | imm scan; begin/end pointer pair at `0xa0a14/0xa0a18` |
| preference table, runtime copy | `.databe 0xadff4..0xae010` (0-terminated) | read as `DAT_107adff4` + walker `&DAT_107adff8` |
| the first-fit walk (re-bind) | regasg.c `0x8be6c` (reads table at `0x8be77/0x8be92`) | decomp: walks the table until `binding[reg]==0 && !conflict(reg)`, then re-binds |
| the first-fit pick (candidate set) | regasg.c `0x8c1dc` (reads table at `0x8c1de/0x8c1e3`) | decomp: candidate set = all table entries; byte-width removes {7,8,6}; pick = first table entry in the surviving set |
| per-register binding array `binding[reg]` | `.bssbe 0x9d6ec` (dword[9], index 1..8) | `(&DAT_1079d6ec)[reg]` = owning node, 0 = free |
| per-register conflict sets | `.bssbe 0x9d6c8` (ptr[9]) | `DAT_1079d6c8 + reg*4` |
| machine-register descriptor array | `.databe 0xac730`, stride 0x54 | flags byte at +5 (`0xac735 + reg*0x54`), dword at +0x34 |
| register-name table (listings) | `.rdata 0xa9194`, indexed `[0xa9190 + reg*4]` | sites `0x74b23/0x74fe9/0x7505e/0x752ec/0x75315/0x75394/0x753c1/0x7542e` (mdlist.c) |
| back-end symbol hash (registers = handles 1..8) | buckets `.bssbe 0x9d88c` (1024 entries; key at sym+0x1c, chain +0x2c) | lookup fn `0x232ec` (`[handle & 0x3ff]`) |
| allocatable test `reg && reg<9 && reg!=5` | regasg.c neighbourhood `0x8bde7` (Ghidra body fragmented) | ESP(5) never allocatable |
| node class table / per-class register sets | `.rdata 0xa09c4` (`[u16>>12]`), `.bssbe 0x99034` (`[class*4]`) | color.c reads both |
| color.c anchors: pressure walk / spill driver / live-range splitter | `0x8e474` / `0x8e877` / `0x8e9de` | `0x8e877` counts {7,8,6} membership separately; `0x8e9de` executes the split when pressure exceeds the limit |
| regasg.c ICE anchor (coalesce/rewrite walk) | `0x8b906` | uses `0x9d6ec`/`0x9d6c8`; byte-width `{7,8,6}` conflict add |

`homm3 vc6 atlas` row `0xadfe4` (the census's `ehexcept.c|regasg.c` writer
cluster) sits 16 bytes before the runtime preference table - the "list/
regasg allocator state" neighbourhood the atlas flagged is exactly this
block.

### Passive temporary-binding observations

```sh
homm3 vc6 trace-registers rmg --fn createShipyardConnection
```

This captures the manifest source/profile once, then replays identical C1
streams through unmodified and instrumented C2. The whole objects must agree
outside the four timestamp bytes before `bindings.txt` is published under
`build/vc6/shim/gate/register-trace/<unit>/`. Both replays use the same output
path because `/Z7` includes that path in its object metadata. A changed byte,
failed compile, or missing observation rejects the report. The hook-free shim
is restored in `finally`, including failure paths. The normal matching object
is never replaced.

The two guarded stores are `0x3356e` (`binding[ESI] = EDX`) and `0x323fe`
(`binding[EAX] = EDX`). The first belongs to the complete routine at `0x3356b`:
it stores the binding, marks the register descriptor used through `0x3359b`,
and returns that descriptor. The second is an independently observed store;
its label does not claim the surrounding routine's complete purpose. Each
snapshot records the selected register and the binding table **before** the
store. `currentFunctionBody` supplies the containing function's actual mangled
name. Value category at `+4` and handle at `+0x1c` are captured together, avoiding
stale identities from reused heap addresses.

These are compiler temporary identities, **not recovered C++ local names**.
Category 3 names an expression temporary. This does not observe all coloring,
spills, releases, or register occupancy; an empty binding-table slot does not
prove that the corresponding machine register is available for any value.
In particular, the two older first-fit sites at `0x8be6c` and `0x8c20d` did
not fire for the shipyard function, while the two stores above produced 151
observations. That result limits the explanatory reach of the simple model
below; it does not explain the late `guardValue`/`entranceX` allocation.

A scratch `/Z7` control also produced 151 events, with whole-object equality
under instrumentation and the same 1472-byte candidate function section as
the release compile. Its generated temporaries still lacked source-local
names. This control therefore does not authorize labeling those handles as
`guardValue` or `entranceX`. The candidate section includes alignment padding;
the authoritative retail function remains 1456 bytes.

## 3. The model

Implemented pure and compiler-free in `scripts/homm3/vc6/_regmodel.py`:

```
PREFERENCE = (eax, ecx, edx, esi, edi, ebx, ebp)      # the 0xadff4 table
for pseudo in creation order:
    take the first PREFERENCE register that is
        not already bound,
        not EBP unless the function is frameless (/Oy),
        not EAX/ECX/EDX if the pseudo crosses a call,
        not ESI/EDI/EBP if the pseudo is byte-sized;
    none left -> frame-homed
```

Consequences within this minimum slice:

* scratch values follow this order only on the fallback path; the preferred-
  register and rotating paths below can override it;
* call-crossing values: ESI, then EDI, then EBX - the observed B1
  register population everywhere in the corpus;
* a hoisted zero (B8) enters the same walk: EAX in a leaf, the first
  free callee-saved under calls;
* a byte-sized call-crossing value's candidate set shrinks to {EBX} -
  measured: VC6 frame-homes it at its definition and lets the widened
  reload take the normal walk. This is the mechanism BEHIND B15's "VC6
  homes char locals far more eagerly than ints".

### 3a. Volatile-register rotation is active in a real TU (2026-09-07)

The selector at C2 RVA `0x33273` has three paths before spilling:

1. Honor the pseudo's preferred register (`pseudo+0x2c`) when it is free and
   absent from that register's conflict set. This does not advance the cursor.
2. When the dword at `0xac0b0` is nonzero, start at the cursor stored at
   `0x9d710` and cycle through the table's EAX/ECX/EDX prefix. Skip occupied or
   conflicting registers; after success, advance to the following register,
   wrapping to EAX. If none qualifies, continue to the fallback.
3. Scan the full preference table from its beginning.

The binder at `0x3356b` receives the selected register in ECX and the pseudo
in EDX, then writes `binding[register]`. Entry hooks at these two RVAs expose
the cursor, preferred register, pseudo ID (`pseudo+0x1c`), and binding without
changing compiler decisions. Preserve GPRs, EFLAGS, x87 state and last-error
state, and verify the resulting object against an uninstrumented replay of
the same captured IL.

`TObjectType::setImageName` at retail `0x514610`, compiled with its configured
`/O2 /Ob2 /Oy- /Op /MT /Gr /GX` profile, produced 36 requests and 36 bindings
through this selector. Rotation was enabled. Its first four requests had no
occupied registers:

| Request | Cursor before | Selection | Cursor after | Path |
|---|---|---|---|---|
| 1 | EAX | EAX | ECX | rotation |
| 2 | ECX | ECX | EDX | rotation |
| 3 | EDX | EDX | EAX | rotation |
| 4 | EAX | EAX | EAX | preferred register |

The complete 156,637-byte object was identical outside its four timestamp
bytes. A separate `/Z7` replay also passed full-object identity (421,028
bytes) and reproduced the normal function's bytes. The latter exposes
candidate statement origins for the allocation requests. The controls are in
`build/least-matched/object-image-20260907/register-rotation-trace/`.

The lookup's reference-return and value-return controls make the effect
observable in this caller. With a reference return, request 7 allocates EDX
for the mapped-value load at node offset `+0x1c`, advancing the cursor to EAX.
The value-return control lacks that request: it has 35 requests and reaches
the following allocation with the cursor still at EDX. Its full 156,709-byte
object also passed replay identity. The score falls from 96.6403 to 87.8696;
the changed scratch allocation follows from a missing request, rather than
requiring a different register preference table.

Consequently, another allocation or a change between preferred-register and
rotating selection can change subsequent scratch-register choices. An empty
binding array does not imply EAX will be selected: both the cursor and the
conflict sets matter. `_regmodel.assign` remains a model of the documented
call-crossing slice; it does not model this cursor or the complete selector.
No modified-compiler result is a matching checkpoint.

The local selector does not account for every emitted register. A second
byte-verified trace at `0x323c8` records operands as the binding-update walk
receives each instruction. In the same function it visited 251 instructions,
with the same 36 selector requests and 55 writes of already assigned registers
at `0x323fe`. The registry's `_Last` load at function offset `+0xd0` already
has EAX assigned to a local temporary (backend ID `0x105`, symbol kind 4).
It does not pass through the scratch selector. The cache's `_Last` load at
`+0x140` does: request 10 selects EDX for a kind-3 pseudo. The two similar
loads therefore require different allocation evidence; cursor arithmetic
alone cannot explain both.

The instruction trace reproduced its complete 421,030-byte `/Z7` object
outside the timestamp and the normal function's 800 bytes including padding
(SHA-256 `bf79c365b8fa6c15b2139eca7ade8fef3b8ab10fd1daff5d8c6eb8903c005314`).
Controls are under
`build/least-matched/object-image-20260907/instruction-binding-trace/`.
At this checkpoint, naming the append position, using `push_back`, assigning
the iterator in the insert argument, and naming the caller's index reference
are byte-identical. These spellings do not change the observed ownership.

Hook boundaries must preserve branch-entry addresses. The binding store at
`0x323fe` is seven bytes; `0x32405` is a branch target. A nine-byte trampoline
that also steals `mov esi,[esi]` at that target makes the traversal loop
without advancing on its skip path. The seven-byte hook leaves that target
intact and passes the object-identity check.

### 3b. Global assignment uses register costs (2026-09-07)

Before the local walk, `0x245c3` assigns a register to each selected live-range
group. Its candidate set is at `group+0x20`; the chosen descriptor is stored
at `group+0x10` (`0x2475e`). It clears the nine-dword cost array at `0x9d868`,
adds competing groups' copy costs and singleton-candidate penalties, then
subtracts the current group's copy preferences. It picks the eligible
register with the lowest signed cost; ties follow the constant preference
table. This is distinct from the local selector's rotating cursor.

A hook at `0x24754` records that final decision before the candidate set is
freed. `0x32467` later rewrites group-valued operands using the chosen
descriptor; calls from `0x32526` to `0x1f828` expose that connection. The
`setImageName` trace records 23 global assignments, visited in descending
order of the priority field at `group+0x0c`, rather than source-definition
order. Replaying minimum-cost selection over the recorded sets and costs
reproduces all 23 choices. Examples:

| Value | Eligible registers | Relevant costs | Choice |
|---|---|---|---|
| loop index `cell` | EBX, ESI, EDI | EBX=2000; ESI=EDI=0 | ESI |
| `maskFile` | EBX, EDI | EBX=1000; EDI=0 | EDI |
| parameter `name` | EBX, ESI, EDI | ESI=1800; EBX=EDI=0 | EDI |
| registry end temporary | EAX, ECX, EDX, ESI | all zero | EAX |

The already rejected loop-local counter control (92.0079%) provides a
second trace. Its index is still assigned ESI first. The later initialization
shortens its live range: `maskFile` now permits ESI as well as EBX and EDI,
with unchanged costs, and chooses ESI. The name parameter also chooses ESI;
`this` then has only EBX/EDI available and chooses EDI. Moving an initializer
therefore changes interference and costs without changing which of these
values is processed first. Declaration/handle order alone is insufficient.
All 22 choices in this control pass the same minimum-cost replay.

Both instrumented `/Z7` objects reproduce their uninstrumented replays outside
timestamps (421,023 and 421,144 bytes respectively), and each function agrees
with its normal-profile compile. Artifacts and the 45-choice replay are under
`build/least-matched/object-image-20260907/global-color-trace/`. The current
function remains at 96.6403%; none of these diagnostic hooks changes a
compiler decision or constitutes a matching checkpoint.

#### Static storage class can prevent propagation (2026-09-07)

The two end-pointer loads above diverge during propagation, before register
assignment. At the 96.6403 checkpoint, both `vector::end()` expansions create
kind-4 result temporaries through `0x19d6e`. The global optimization driver
`0x13615` removes the cache temporary's executable uses but retains the
registry temporary. Its operand substitution routine `0x70c7` explains why:
`0x7365` tests the replacement symbol's kind, and `0x74d8` rejects a kind-7
replacement unless the original symbol is also kind 7. A kind-4 local cannot
be replaced by that kind-7 storage operand. Kind-8 storage passes this check.
This is a specific restriction, not a general failure to propagate locals.

In the active TU, an external inline accessor gives its local static kind-7
storage; an ordinary accessor gives kind 8. A `static inline` accessor also
retains kind 8. The earlier registry accessor was external inline, while the
cache accessor was ordinary. Reversing those declarations changes which
end-pointer result survives to global assignment. Returning the mapped index
by value then restores the subsequent scratch allocation. With the record
selected before the string copy, these ordinary C++ changes raise
`setImageName` from 96.6403 to 99.2095. Its 791-byte body retains all 27 calls
and EH states `0,-1,1,-1`; only the counter's zeroing instruction remains
before the reads instead of at the loop entry. Guard-byte separation proves
the shared accessor boundary; it did not prove the old `inline` declaration.

Observe each temporary's lifetime, not just its first allocated handle.
The registry's final ID `0x105` reused the same 84-byte slot six times during
inlining. Allocation hooks at `0x1e2b` and `0x1e47`, the result hook at
`0x19d6e`, and the phase driver observations establish the final use. A freed
record's old preference/definition fields are not live allocation evidence.

All hooks are passive. The substitution trace reproduces its complete
421,023-byte object outside the COFF timestamp. The final storage-class
trace reproduces its 421,000-byte object and confirms kind 8 for the registry
field and kind 7 for the cache field. Its normal-profile function, including
padding, hashes to
`7ef1c6dff4104f771e25d4074a6d94c6ec030c232f2ac0013428e6075ddc74ba`.
Artifacts are under
`build/least-matched/object-image-20260907/{local-allocation-trace,temporary-phase-trace,substitution-trace,storage-class-trace}/`;
source controls and the banked object are alongside them. No modified
compiler decision supplies the matching score.

The corrected storage classes do not remove the counter-lifetime effect.
The corresponding loop-local control scores 92.5020 and reproduces its
421,121-byte `/Z7` object outside the timestamp. Both traces process `cell`
first (priority 206, ESI), followed by the byte index and bit mask (160 each),
then `maskFile` (84). Only the early initializer excludes ESI from the file's
candidate set. With the late initializer, its ESI/EDI costs both remain zero;
the fixed tie-break chooses ESI. That changes the subsequent assignments:
`oldCount` moves from EBX to EDI and `this` from ESI to EBX. The earlier
reference-returning lookup's late-counter trace chose different downstream
registers, so its exact assignments should not be carried across the accessor
correction. Reusing a spent source index and spelling the loader joins with
explicit jumps reproduce the current late-counter function byte for byte.
Its 800 bytes including padding hash to
`6455c63fd9654a29164f437b046f79695dbbfe16fac31218d43edc97f897499d`;
the paired traces are in `storage-class-trace/{debug,late-counter-debug}/`.

### 3c. Source creation order

**"Creation order" means the FIRST ASSIGNMENT, not the declaration**
(measured three ways, 2026-09-06, polish 30). A bare `long i;` moved to the
top of a block is byte-inert; moving `long i = 0;` can change allocation.
The source probes below establish that effect; section 3b shows why it does
not by itself prove that the allocator's processing order changed.
That single fact settles three things at once and is the cheapest lever in
this file to try:

* `AICheckRetreat`'s town census - `long i = 0;` declared beside `count`, so
  the index pseudo is born ahead of `numTowns`, hands EBX to the index and
  spills the bound with a reload at the back edge, which is retail's
  allocation: **95.3960 -> 95.7676**. The control (`long i;` hoisted, `i = 0`
  left in the `for` head) is byte-flat at 95.3960, and it is exactly what an
  older note had recorded as "initialising the index before the numTowns
  guard - byte-flat", i.e. the note had measured the declaration;
* the same order decides the SIB base/index slot for a two-local sum (6b);
* it runs the other way too - `should_attack_now` wants its index born FIRST
  and loses 1.10 when it is born after `current`/`count`.

**And naming the ELEMENT of a subscript that feeds a call re-homes the
pair.** `type_monster_data& monster = monsters[i];` at the head of a loop
body gives the strength-reduced 72-byte offset the callee-saved register and
spills the index - retail's allocation - worth **+5.62** on
`type_AI_combat_data::get_enchantment_value` (across four inlined copies) and
+1.55 on `cast_spell`. The pointer spelling is byte-identical, so what
matters is that the ADDRESS is named, not its type. It is per-site: the same
naming over a subscript that feeds only field compares (`choose_melee`'s
opening scan, `cast_spell`'s familiar scan) is byte-flat.

## 4. Measured probe base (2026-08-10, pinned SP3 CL, game profile)

Scratch TUs (extern `source`/`sink` calls keep values live across calls):

| probe | result |
|---|---|
| `a,b,c` created in order, all live across calls | a=ESI b=EDI c=EBX |
| four such values | ESI, EDI, EBX, 4th frame-homed (`_d$[ebp]`) |
| param `x` live across calls + one local | x=ESI (first), local=EDI |
| `a` then `b` created | a=ESI b=EDI |
| `b` then `a` created (same body otherwise) | **b=ESI a=EDI - the flip** |
| `unsigned char` value across a call | frame-homed at def; widened reload ESI |

IL cross-check (`homm3.vc6.il` capture of the two-order probe TU): the
`sy` stream's symbol handles follow lexical creation order - in the
swapped variant `b@0xef < a@0xf0` - and the handle order predicts the
ESI/EDI outcome in all five probes. `_regmodel.assign` reproduces every
row above (asserted in the validation run).

## 5. Where B1 actually lives - the plateau finding

On the real plateau rows the schedule is identical and so is the
DEFINITION ORDER; only the picks are permuted:

* `combatManager::get_attack_change` (ai.cpp, 96.4%): both sides define
  `enemy` at slot 5/6, `this` next, the first call result at slot 24.
  Retail picks **ESI** for the first definition (the model's first-fit
  prediction); ours picks **EDI** - our allocator processed the
  call-result pseudo first. `this` takes EBX on BOTH sides.
* `type_AI_combat_parameters::get_simple_attack_effect` (ai_tactical,
  99.1%): retail is the pure first-fit prediction 3/3 (`this`=ESI,
  `start_our`=EDI, `start_enemy`=EBX); ours transposes exactly the first
  pair.

The original model attributed these swaps to processing order and C1
symbol/handle state (see il-format.md's measured handle shift). Naming
probes can flip expression values, while aliases of parameters or `this`
were copy-propagated in these cases. Those observations remain useful,
but they did not trace the global allocator. Section 3b now demonstrates
another mechanism: equal-priority visitation can produce different
registers through changed candidate sets and costs. These B1 cases need
that evidence before a handle-order cause can be treated as established.

## 6. why-reg v2 - the model path

`python3 -m homm3.vc6.reg_model <src> --fn F (--against UNIT:FN |
--against-src FILE) [--tries N] [--il-order] [--sweep]` - or any caller
that sets `args.model` before `reg_model.run_why` (the `homm3 vc6
why-reg` subcommand keeps the v1 guided sweep as its default).

Algorithm: diagnose via `_align` as v1 does; read both sides'
callee-saved first-DEFINITION tables; attribute values from the base
`/FAs` listing (frame-operand names, `this`, fastcall entry registers);
derive each side's processing order from the bindings through the
preference ranking; name the transposed pair; then compile ONLY the
model-prescribed edit (parameter alias / declaration reorder for the
callee class, the B14 flag-naming for the scratch class), up to
`--tries` (default 1). When the pair is not source-nameable it reports
the C1-class cap with zero or one compile spent.

Validation (2026-08-10, against the delinked retail objects):

| case | distance | v2 outcome | compiles |
|---|---|---|---|
| get_attack_change (B1 open) | 50 | correct pair named (`enemy` must move first); alias edit compiled, copy-propagated; capped C1-class | 1 (v1 sweep: 20, no winner) |
| get_simple_attack_effect (B1 open) | 32 | `this` correctly identified as unmovable; ranked fallback found a real -1 (else-arm store order) | 1 |
| get_disease_value, reopened (flag un-named) | 10 | model-ranked B14 edit = the historical lever -> **0, register-visible EXACT** | 1 |
| hermetic probe pair (decl order swapped) | 17 | correctly declines (schedules differ - not the B1 slice); v1 sweep run as control finds the swap -> 0 | 0 |

### 6a. B18 (SIB operand order) - a measured tree census, 2026-09-06

The B18 encoder tie-break above is declared out of scope, and this is what
it costs. A first-divergence sweep over every sub-100 row found FIVE rows
whose entire remaining residual is one `[base+index]` pair with the two
registers exchanged - same mnemonic, same operands, same displacement, one
SIB byte:

| row | site | ours | retail |
|---|---|---|---|
| `singleselectionwindow ?SetCurrentMap` (99.9874) | `gpGame->setup.handicap[i]` | `[eax+ecx+0x1f6a8]` | `[ecx+eax+0x1f6a8]` |
| `swapmgr ?SetRolloverText` (99.9787) | `gPrimarySkillNames` row | `[edx+eax+0xc9]` | `[eax+edx+0xc9]` |
| `ai_player ?fill_prohibited_array` (99.9678) | `gpGame->playerDisabled[player_index]` | `[ecx+edi+0x1f636]` | `[edi+ecx+0x1f636]` |
| `seerhuttext ?LoadSeerHutTextColumn` (99.9621) | inlined `basic_string::_Eos` terminator | `[eax+ecx]` | `[ecx+eax]` |
| `philai ?value_of_enemy_town` (99.9561) | `return combat_value + town_value;` | `lea [ebx+ecx]` | `lea [ecx+ebx]` |

Two facts were banked here on the day the table was taken. First, it is NOT
the source operand order: swapping the addends of `value_of_enemy_town`'s
`return` is byte-flat to the digit (VC6 canonicalises `+` before the encoder
sees it), so no `a+b` -> `b+a` edit reaches it. That still holds. Second, the
row read "none of them is reachable" - **and that is now refuted**; see 6b.

### 6b. B18 is compiler STATE, not an encoder tie-break - 2026-09-06 (polish 30)

An empirical rule search was run over the exact corpus: every scale-1
two-register SIB memory operand in the 100.0000 rows of
`build/objdiff/normalized/base` (those bytes ARE retail's), **2,123 sites in
833 functions**, each labelled with what the emitted stream says about its two
registers (defining instruction and its class, last mention, live-in-ness,
displacement, `lea` vs `mov`). No local rule fits:

| candidate rule | accuracy on the 2,123-site corpus |
|---|---|
| base = lower x86 register number | 50.1% |
| base = most recently *mentioned* register | 61.4% |
| base = later-defined (block-local defs; 32% of sites have both live-in) | 43.7% overall, 65.5% of the 566 both-defined sites |
| def-class ranking, best possible (per-class-pair majority ORACLE) | 65.1% |

So the choice is not a function of the operands' local properties. Three
sub-rules ARE clean, though, and they are worth knowing:

* a `shl`/`imul`/`lea`-computed operand ALWAYS takes the base slot against a
  freshly loaded global pointer (103/103) or against an `[ebp-N]` local load
  (17/17); an `inc`/`add`-computed one always beats a global load (13/13);
* a register still live from function entry (`this`, or a parameter register
  never rewritten) is the INDEX at 58 of 68 sites (85%);
* both orders occur for the same shape in one function: the exact row
  `cmbtmgr::CombatIsOver` emits `[ecx+edi+0x132b2]` and `[edi+ecx+0x132b0]`
  in two adjacent, structurally identical statements - in retail AND in our
  compile, identically.

That last one is the key: the order is per-SITE STATE, and state is what
source moves. Two levers are now measured, each with a minimal probe pair
compiled by the pinned SP3 CL at the game profile (`build/p30/sibprobe*.cpp`):

1. **int + int: base = the local whose FIRST ASSIGNMENT is later in source
   order.** Minimal pair: `int x=f(a); int y=h(b); v(); return x+y;` emits
   `lea eax,[esi+edi]`; moving a `int y = 0;` above `int x=f(a)` emits
   `lea eax,[edi+esi]` - one SIB byte, every other byte identical, and the
   `= 0` store itself is dead-code-eliminated. A bare `int y;` declaration
   does NOT do it (the pseudo is born at the first assignment, not the
   declaration), and the LAST assignment does not either.
   **Applied: `philai value_of_enemy_town` 99.9561 -> 100.0000** by hoisting
   `long town_value = 0;` above the `combat_value` initialiser.
2. **pointer + index: the FIRST addressing mode built over a given (pointer,
   index) register pair in a function encodes base=pointer; a SECOND one over
   the same pair encodes base=index.** Probe `z1`/`z2` mirror CombatIsOver:
   swapping the two statements swaps which array gets which encoding.

3. **pointer + index: the birth position of an UNRELATED nearby local moves
   it.** Two more levers, both measured 2026-09-06 (polish 32), both closing a
   row whose SIB transposition was its ONLY divergence:
   * **A named default-constructed local passed to a defaulted STL parameter
     must be the DEFAULT ARGUMENT.** `game::LoadBoatPool` writes seven
     `boats[x].field =` stores; with `boat defaultBoat; boats.resize(n,
     defaultBoat);` ahead of the loop all seven encode base=offset against
     retail's base=pointer (99.6447), and `boats.resize(n)` - Dinkumware's
     `resize(size_type, _Ty _X = _Ty())` - makes all seven agree
     (**100.0000**), with no other byte moving. Hoisting the same declaration
     to the top of the frame instead costs 5.96 (93.6853), so it is the
     temporary's BIRTH POSITION, not its existence. A hand-rolled probe of the
     identical loop (`build/p32/sib1.cpp`) and the exact twin `SaveBoatPool`,
     neither of which has such a local, both already emit retail's order.
   * **A block-scoped loop index against a reused function-scope one.**
     `initialize_ballistics_table`'s inlined sea-row `GetRow` addressed
     `[edi+edx]` against retail's `[edx+edi]` (99.9485, the row's only byte).
     Giving that loop its own `for (int row = 2; ...)` instead of reusing the
     function-scope `int i` flips it: **100.0000**. Hoisting the loop's
     destination pointer above `int i` costs 5.50 (94.4485); moving
     `++sea_movement` out of the for-increment is byte-flat.

   Both say the same thing: the pair's encoding is C1 handle/creation state,
   and the cheapest source knobs on that state are a local's SCOPE and the
   birth position of a temporary, neither of which touches the addressing
   expression itself.

4. **The scope lever RUNS BOTH WAYS: several `for (int i = ...)` loops
   sharing ONE function-scope index** (2026-09-06, polish 37). The polish-32
   entry above gives a loop its OWN block-scoped index to turn base=pointer
   into base=index; the mirror move turns base=index back into base=pointer.
   `TSingleSelectionWindow::SetCurrentMap` read
   `gpGame->setup.handicap[i]` as `[i + gpGame]` against retail's
   `[gpGame + i]`, and it is the function's ONLY two-register SIB site, so
   the "second occurrence of the pair" lever (2) cannot reach it. Declaring
   one `int i;` at the top of the frame and writing the body's three
   `for (int i = 0; ...)` loops as `for (i = 0; ...)` makes `i` born early
   enough to become the INDEX and the freshly loaded global the BASE:
   **99.9874 -> 100.0000**, every other byte identical, no other row in the
   unit moved. Read together with (3), the rule is that the operand born
   LATER takes the BASE slot, and a loop counter's birth position is moved
   by hoisting or sinking its declaration's INITIALISER (a bare declaration
   is still inert, per section 3).

   `type_random_map_generator::placeBorderObject` (0x540d60) supplies another
   measured reuse case. Its two prototype searches and guard-placement loop
   share one index. Giving the third loop a separate `guardIndex` changes
   only the byte-vector SIB bytes at 0x540f6a and 0x540f9e; naming the vector
   bases or using `begin()[index]` is neutral. Reusing the first index closes
   all 598 raw bytes, including five resolved relocations. Reusing the
   prototype/water index in the shipyard caller is neutral, so this remains
   a site-specific lifetime effect rather than a universal loop spelling.

   Bounds measured the same lane, all byte-flat: on
   `ai_player::fill_prohibited_array` (base `[gpGame + player_index]` vs
   retail `[player_index + gpGame]`, and the FIRST occurrence of that
   register pair 0x45 bytes earlier already AGREES on base=index in both)
   neither hoisting `int player_index;` to function scope nor swapping it
   with `players_left` moves the byte - that row's second occurrence of an
   already-used pair is the flip our C2 makes and retail does not.
   `game::save@playerData`'s frame-slot variant of the same question - x at
   -0x8/-0xc against retail's -0xc/-0x8 - is inert under ALL 48
   permutations of its six local declarations, which re-confirms section
   3's "creation order means the first ASSIGNMENT, not the declaration" for
   frame-slot colouring as well as for register binding.

Byte-flat for this class, all measured this lane: source addend order;
`*(p+i)`, `&p[i]`, `i[p]`; naming the pointer, the index or the whole address
in a local; declaring that local before or after the counter; a local copy of
`this`; `unsigned`/`short`/`char` index; do-while vs for vs goto loop form; a
dead duplicate read of the same member.

The first occurrence of an entry-live pointer plus an unscaled index was a
measured frontier for those particular source models, not a proof that no
source reconstruction can match. `hero::getPrimarySkillTotal` and
`CDiffFile::apply` are now exact after recovering source facts. In `apply`,
Dreamcast line 63 obtains the payload pointer immediately after allocation,
before the three offset initializers. Caching that pointer recovers all three
SIB operands at 100%; repeated `getData()` calls score 99.6429%. Restoring the
ordinary accessor to its original `.cpp` position is independently byte-neutral.
Older cached-pointer controls were flat in their then-current source state.

Two further controls make the same limit concrete. Restoring the canonical
`TSpreadsheetResource::getSpreadsheet(row, column)` calls in
`initializeHighScoreDefaults` closes four first-loop SIB transpositions
(99.3846% to 100%). Removing the extra `bankGuardTypes` pointer view from
`initializeCreatureBankTraits` restores the two-cursor copy and the string
terminator's SIB order (97.5355% to 100%). Each result was reproduced with a
negative control and checked across the owning TU. These are recovered
interfaces and lifetimes, not arbitrary handle-population changes.

`ai_player::fillProhibitedArray` and `seerhuttext::loadSeerHutTextColumn`
retain their documented residuals. Do not extend the outcomes of exhausted
spelling families to untested canonical helpers or newly recovered lifetimes.

Honest accuracy statement: the model predicts the pinned compiler's
callee-saved assignment from creation order in 5/5 standalone probes,
and predicts RETAIL's binding from the definition tables in 2/2 real B1
cases; it does NOT predict our own compiler's plateau-side processing
order from source (that order is front-end handle state), and it cannot
flip a parameter/`this` pairing by spelling - it now proves that in one
compile instead of a sweep. Encoder-level tie-breaks (B17 length
feedback, B18 SIB operand order) are not allocator decisions and are
out of scope.

### 6d. The one-line forwarder is a DEPTH level (2026-09-06, polish 30)

`army::GetName()` is `return GetArmyName(creatureType, numTroops);`, so a
statement written through it reaches the trait lookup one /Ob2 level deeper
than a direct `GetArmyName(a->creatureType, a->numTroops)` call - and that one
level is the whole difference between the leaf being CALLED and being expanded
with its range guard, its 116-byte stride and its +0x14/+0x18 name pair
inline. In `drawing.obj`: `show_creature_spell_error` **82.4044 -> 92.3889**
on one pair of sites, `CombatMessage` **90.2999 -> 92.7545** on nine.

Three bounds, all measured:

* **all-or-nothing per body** - converting two of CombatMessage's nine sites
  scores 86.42, BELOW the untouched baseline;
* **coupled across a caller edge** - converting only `show_creature_spell_error`
  costs `CombatMessage` 3.22 and gives it an EH frame retail has not got, so
  the caller has to be converted in the same change;
* **per body, not global** - the identical rewrite at `ModifySpellDamage`'s
  four name sites costs 16.2 (88.49 -> 72.25), because retail CALLS the lookup
  there. The screen that tells them apart is a census of
  `?GetArmyName@@YIPBDHH@Z` call sites, base object against delinked target;
  after the drawing fix no other sub-100 row in the tree disagrees.

Keeping the forwarder and passing a CONSTANT count is not a substitute
(81.66): the constant then has to be materialised for a call that stays.

### 6c. REFUTED: the `_Ufill` / `_Destroy` surplus is a delink NAMING artifact
(2026-09-06, polish 32)

The lead below is wrong, and the defect is in its instrument. It counted
`?<member>@?$vector@` call-site NAMES in the delinked target - but the target
does not spell an ICF-folded vector leaf that way. `vector<widget*>::_Destroy`
and `vector<type_artifact>::_Destroy` are both `ret` for a POD element, so
/OPT:ICF folds them onto ONE body and the delinker labels that body with
whichever symbol it picked: `__h3cg$customcampaign$vector_destroy$type_artifact`
for `_Destroy`, `game_1510_sub07_8d940` for `_Ufill`, and the
`vector<army*>` / `vector<int>` instantiations for `size` / `push_back` /
`begin` / `end`. A member-name census attributes NONE of those to the member,
so retail's calls vanish from its column and every row reads as a surplus.

Resolved per row on the three largest carriers the lead named:

| row | `_Destroy` | `_Ufill` | `size` | `_Ucopy` |
|---|---|---|---|---|
| `TSingleSelectionWindow` ctor (11,619 B, 95.71) | 4 = 4 | 6 = 6 | 13 = 13 | - |
| `type_garrison_base_window` (7,456 B, 93.88) | 7 vs **8** | 14 vs **16** | 26 vs **33** | 28 vs **32** |
| `TSystemOptionsWindow` ctor (6,268 B, 96.34) | 4 vs **5** | **8** vs 7 | **13** vs 7 | **16** vs 15 |

The largest carrier is EXACTLY EQUAL on every leaf (its `push_back` 6=6,
`begin` 1=1 and `end` 2=2 too) - the lead was empty there. On the garrison
ctor the SIGN IS INVERTED: retail calls MORE of every leaf, i.e. we
over-expand and the direction is caller-shrink, not the ladder. Only
`TSystemOptionsWindow` keeps a one-sided surplus and it is on `size`, not on
`_Ufill`/`_Destroy`.

**Use a NAME-INDEPENDENT instrument instead** (`build/p32/calltotal.py`):
count `call` INSTRUCTIONS per function on both sides. Over the tree that
gives 133 at-MAX sub-100 rows whose total differs at all, 80 of them with
retail calling MORE (we over-expand -> shrink the caller) and 53 the other
way; every row not in that list has an identical call census whatever the
relocation names say, so its residual is spelling, registers or scheduling
and no inliner knob applies. The same trap sinks any per-callee census:
`GetLuck`'s `_cpp_clamp base x1 vs retail x0` is
`THeroScreenWindow_scalar_deleting_destructor` on the other side, and
`SetupAndLoadObstacles`'s `TObstacleVector::Destroy` is the same
`__h3cg$...vector_destroy$type_artifact` fold. Check the TOTAL first.

### 6c-old. Superseded lead: the tree-wide `_Ufill` / `_Destroy` surplus

An element-agnostic reloc census over every sub-100 row (base object against
the delinked body, counting `?<member>@?$vector@` call sites across ALL
instantiations, so /OPT:ICF's cross-element folding cannot skew it) finds one
shape repeated far more than any other:

* **`_Destroy`: ours > retail in 71 rows** (the reverse in 6);
* **`_Ufill`: ours > retail in 32 rows** (never the reverse on a large row).

We CALL both leaves inside the `vector::insert` expansions we keep; retail
EXPANDS them - and `_Destroy` over a POD pointer element collapses to nothing
at all, so every one of those calls is pure surplus. The largest carriers are
`TSingleSelectionWindow::TSingleSelectionWindow` (11,619 B, 95.71, 6+4),
`type_garrison_base_window` (7,456 B, 93.88, 14+7) and
`TSystemOptionsWindow` (6,268 B, 96.34, 8+4).

The depth ladder's shallower spelling is NOT the lever: rewriting all 49
`Widgets.push_back(x)` in the garrison ctor as
`Widgets.insert(Widgets.end(), x)` costs **10.3 points** (93.8825 -> 83.54,
40 target-only calls). Note that retail also calls `size` and `_Ucopy` MORE
often than we do at the same sites (33/32 against 26/28), so this is not a
single budget knob in either direction - it is a different split of which
leaves the kept expansions inline. Left open with the census recorded.

### 6e. A dead stack home can expose a missing member read or accessor

The `CNetMsgHandler` destructor family was classified as an inaccessible
register-homing residual: retail writes an old handler pointer to `[ebp-4]`
and never reads it, while the candidate removes that store and its entire
frame. The source-level cause is now measured in the real `remote` TU:

1. `CDPlayHeroes::SetNetMsgHandler` saves `pOld`, installs the new pointer,
   then tests **the installed member** and calls `m_pNetMsgHandler->Copy(pOld)`.
   Dreamcast's `remote.cpp:788-789` reloads that member after the store.
   Reusing the incoming parameter for the test and receiver produces an exact
   standalone setter, but changes its inline copies. Restoring the member
   reads closes the base destructor **41.875% -> 100%** and its scalar deleting
   wrapper **78.125% -> 100%**; the setter remains exact. The pause-handler
   destructor improves **91.1111% -> 99.7778%**.
2. `Copy` calls `IsInPopup()` before the virtual abort-message getter;
   Dreamcast retains that call at `remote.h:633`. Flattening it to the field
   leaves the standalone Copy exact but uses EDX for the later virtual call
   inside the pause destructor. Restoring the accessor changes those two
   instructions to retail's EAX and closes the destructor to **100%**.
   Restoring Copy's header definition preserves these matches.

These controls distinguish source relationships that standalone helper bytes,
identical caller CFGs, and identical surviving call lists cannot distinguish.
The restored member reads reproduce the dead home naturally; no volatile,
release-VERIFY carrier, or inline pragma is needed. The result establishes the
source lever, not which internal C2 pass preserves the store. Before treating
an allocation residual as unreachable, inspect the helper's member-versus-
parameter reads and the accessor calls that were flattened during reconstruction.

### 6f. An inline helper's return expression can remove a caller spill

The third boat sprite in four hero-part renderers spilled the `GetNumFrames`
divisor into a recycled parameter home. Their CFGs and surviving call lists
already agreed with retail, and the allocator model reported no binding
divergence. Naming the sprite, divisor, coordinates, or remainder did not
recover retail's ECX divide.

Dreamcast's `CSprite.h:293` calls `IsValidSeq` at `0x1f1fc` and joins the frame
count and zero values before returning. The reconstruction had flattened the
predicate and used an `if` with two returns. Restoring the predicate call
alone leaves the spill; expressing the result as
`return IsValidSeq(seq) ? s[seq]->numFrames : 0;` removes it in all four
callers without changing their bodies:

| Caller | Retail VA | MAX before | MAX after |
|---|---|---:|---:|
| `DrawHeroPart` | `0x40fe30` | 98.1667% | 100% |
| `DrawHeroPartShadow` | `0x4102c0` | 98.1840% | 100% |
| `VWDrawHeroPart` | `0x5f7500` | 98.2385% | 100% |
| `VWDrawHeroPartShadow` | `0x5f7900` | 98.2385% | 100% |

Measured 2026-09-06 in the real `advmgr` and `viewwrld` TUs, followed by a full
build and MAX/history audit. This establishes a source-expression lever;
the exact internal scheduling or coalescing pass remains unproven. An
unchanged call list does not establish that the inlined helper's expression
shape is correct.

### 6g. Recover a coordinate local's meaning before its stack home

`DrawCursorShadow` (`0x47fb40`) reached 100% from 72.5446% after restoring
`refY` as a destination screen coordinate: `CellY * 32 + 232`, with
`CellY * 32` retained as the separate clipping argument. Dreamcast's
cursor.cpp:207 stores the offset-inclusive local; its UI offsets are
170/168, while Complete's x86 establishes 256/232. `refX` precedes `refY`
in the recorded statement sequence. Restoring screen meaning alone reaches
97.13%; restoring their order closes the last frame/home differences.

The former raw-Y local (`refY = CellY * 32`, adding 232 separately at both
calls) put the locals in opposite homes despite an equal-sized frame and
matching control flow. Reversing those raw-Y declarations did not help;
that control tested the wrong meaning. The original early guards and
`GetCurrHero` call are independently byte-flat source corrections.

The same correction closes `DrawCursor` (`0x47f860`) from 80.3240% to
100%, eliminating the apparent CellX/refY register-homing conflict across
its five sprite draws. Its shadow and alpha siblings remain exact. These
results establish a source-expression and local-lifetime lever, without
claiming which internal C2 pass causes the changed homes.

### 6h. A resources-pointer alias can displace an unrelated hero local

`type_AI_player::buy_creatures` (`0x42ba60`) kept a `long*` alias for the
player's resources alongside a separate temporary funds array. Dreamcast
records the local `long funds[7]`. Restoring that array and spelling the
persistent supply as `player->resources` raises the caller from 76.84% to
97.95%, after restoring its two early exits and `GetTeam` call. The garrison
hero moves from a stack spill into retail's EBX; the guarded GetHero path,
TownAlreadyBuiltOn byte test, and human-ally loop also recover their retail
registers and control flow without editing those helpers.

Dreamcast records `amount` at function scope. Moving it out of the inner
conditional recovers retail's 0x94 frame and the purchaser/amount homes,
reaching 98.18% in the archived three-argument-set control. Recovering the
actual two-argument `set` interface leaves 97.77%; its first vector insertion
still expands one level too far. The old inline_depth(1) pin around
`AI_consolidate_army` is now byte-flat and has been removed: standalone
`do_swap` remains exact and its nested copy retains the retail call.

### 6i. A named endpoint pointer changes address formation

`type_random_map_generator::DrawStraightZoneBoundary` (0x53c220) reaches
all 362 raw retail bytes with a named `TRmgMapItem* lastItem` followed by
`lastItem->zoneState.zone = zoneIndex`. Flattening those statements into
`map.GetMapItem(...)->zoneState.zone = zoneIndex` leaves the loop and step
setup intact, but changes the endpoint's address calculation: the candidate
loads the map base later and folds the field displacement into the pointer;
retail forms the item pointer first and accesses its `+0x20` field.

This is a local lifetime/address-formation control, with no extra operation,
helper declaration, or inline directive. Check an evidenced intermediate
pointer before attributing a final SIB/address mismatch to global compiler
state. The negative control and step-initialization controls are recorded
beside the function.

### 6j. A temporary point's scope controls arithmetic register roles

`DrawIrregularZoneBoundary` (0x53bff0) matched 98.0198% with a named
`delta = to - from` that lived through displacement calculation. Keeping
the subtraction and perpendicular construction together in a nested block,
with `perpendicular` declared outside it, ended delta's lifetime before
`Length()` and restored retail's register roles (99.90099%). Exchanging
the commutative midpoint addends to `from.x + to.x` and `from.y + to.y`
then settled the remaining SIB encodings: all 555 raw bytes match.

The scalar negative control, assigning perpendicular's two components
independently, collapsed a retained intermediate and scored 90.3416%.
This is a measured lifetime hypothesis; without a DC counterpart it is
not a recovered original lexical scope. A byte match also does not settle
whether the midpoint calculation was itself a helper expansion.

### 6g. A bounds aggregate preserves the retail stack frame

`RepairWaterZoneBorders` (0x53fcb0) stores its four rectangular bounds in
consecutive homes at EBP-0x50 through EBP-0x44. The minimum-y home is dead,
but remains part of that four-integer area. Two `TPoint` corners and four
independent scalars produce the same clamp instructions, yet VC6 folds their
storage and allocates a 0x7c frame instead of retail's 0x84.

Using the existing `TRmgZoneBounds` aggregate restores that area. Keeping
one terrain local across searching and painting then reproduces every
observed local home, including both vectors, the two coordinates, the brush,
and the level map. Separate terrain locals score slightly higher (97.0449%
versus 96.9219%) but shift the vector and current-pointer homes by four bytes.
The candidate with the retail frame is retained; MAX/history bank the peak.

This is a type/lifetime finding from raw stack operands, not proof of an
original class name. Scalar declaration hoisting, scoping the nearby point
to one cell, and changing the found flag to native bool were neutral controls.
No padding local, lifetime-extending dummy operation, or inline directive
is involved. The remaining instruction differences are documented
beside the function.

Two `TPoint` members inside one bounds object also shrink this frame to
0x7c, as do the tested by-value corner setters. A named clamped upper point
instead grows it to 0x88. The four integer fields and direct stores preserve
the observed homes; enclosing two constructed points in a record is not
equivalent for VC6's local allocation.

### 6h. Constructor argument order preserves dimension values

The plane view in `RepairWaterZoneBorders` originally used the candidate
signature `(type_random_map& source, int level)`. It stored width too early
and differed from retail in both dimension multiplication and the subsequent
painting loop's registers. Recovering a buffer-first signature
`(TRmgMapItem* items, int width, int height)` and calling it with
`map.GetMapItem(0, 0, position.z), map.mapWidth, map.mapHeight` preserves
the dimension values before the pointer calculation.

This raises 97.0918% to 98.3965%. After placing the candidate fragment at its
retail address and resolving five relocations, all 232 bytes from 0x540124
to 0x54020c agree, including the painting loop's raw branch displacements.
The function's 0x84 frame and existing search bytes remain unchanged.

Dimensions-first scalar arguments reload dimensions. A map/level pair,
a separate plane local, and an all-member initializer list do not reproduce
the same loads, multiplication operand, or vptr/store order. The constructor
keeps body assignments and the caller keeps its canonical map accessor.
This is a retail-driven model of an expanded constructor, with no DC RMG
counterpart; the other view caller, `CreateRiver`, remains partial.

The constructor's field-store order is independently observable too. Once the
island painter's real three-coordinate lifetime is restored, changing only
the shared body from width/height/items to items/width/height closes that
caller from 95.8947% to 100%, while water-border repair stays exact. All six
orders were measured across ten reproduced underground callers and all seven
header consumers. Four independent caller/helper controls isolate the island
gain to the constructor, not the unrelated underground rewrite. The full
build confirms the gain; a lower unchanged-source terrain-coordinator CUR is
recorded with its previous MAX/HIST intact and no changed call sequence.
The signature, inline declaration, assignment-in-body model and scalar field
ownership remain canonical; no caller-specific constructor is introduced.

### 6i. A caller improvement does not prove a helper declaration

The former member-subtraction model made
`TPoint(position.x, position.y) - TPoint(radius, radius)` restore the second
scan's map-index operand order in `RepairWaterZoneBorders` (97.04883% to
97.0918% before the buffer-first constructor recovery). Direct component
construction lost that order; changing only the member's argument to a value
changed the outer induction from retail's x+2 to x-1. An unused addition
member was neutral. Those were useful controls, but the const-reference
member declaration was still a hypothesis.

The retained Voronoi arithmetic below supersedes that declaration. Direct
lower-corner construction initially retained the retail x+2 induction at
98.3535%, below the old model's 98.3965% MAX. Naming the original row and
keeping it through the upper clamps then restored the map-index operand
order at 98.3965%, with the free operations and two-value ABI intact.
The temporary dip was not evidence for restoring the disproved interface.

Shared lower/radius values, by contrast, alter outer-loop registers. Merely
reusing a lower variable or assigning it after default construction is
neutral. Reusing one clamped point for both corners still grows the frame
to 0x88. These controls distinguish value lifetime from variable scope.

### 6j. Retained calls distinguish point translation from vector arithmetic

`TRmgVoronoi::BuildVertices` at 0x5fdb40 forms a circumcenter from three
site positions. Its retained helper sequence provides stronger interface
evidence than the already exact, fully expanded arithmetic in the clipping
and irregular-boundary callers:

| Retail body | Operation model | ABI | Raw bytes |
|---|---|---|---:|
| 0x5fdcb0 | vector + vector | member, eight-byte right value, hidden result | 30/30 |
| 0x5fdcd0 | vector * scalar | member, integer scale, hidden result | 29/29 |
| 0x5fdcf0 | vector / scalar | member, signed integer division, hidden result | 37/37 |
| 0x5fdd20 | point + vector | free, two eight-byte values, result in ECX | 32/32 |
| 0x5fdd40 | point - point | free, two eight-byte values, result in ECX | 32/32 |

The subtraction results feed displacement arithmetic; the last free addition
translates the origin by the resulting vector. Distinguishing the position
and displacement types accounts for both the member and free addition
interfaces without competing overloads on a single type. `TPoint` and
`TRmgVector` are role names, not recovered Dreamcast declarations.

All 160 bytes agree without relocations. The length helper at 0x5fceb0
belongs to the displacement type and remains exact. Ordinary definitions
remain visible in the arithmetic TU, without inline controls. The clipping
body imported from master's c447c5f3 remains 614/614 bytes exact, as do the
irregular and straight boundary bodies. Thus exact expanded callers alone
did not settle the former member/free or point/vector models.

### 6k. Named dimension values settle the clamp allocation

`RepairWaterZoneBorders` reached 100% by naming `height = map.mapHeight`
immediately before `min(row + upper, height)` and `width = map.mapWidth`
immediately before `min(position.x + upper, width)` in all three bounds
groups. The original row remains a separate value through each group.
These single-use locals preserve the retail dimension loads and change VC6's
allocation of the surrounding clamp operands. The second group's two zeros
are materialized independently, and the last maximum-X comparison uses the
retail EAX/ECX roles and store order. All 90 CFG blocks agree; the function
retains its 0x84 frame and all observed local homes.
Resolving the 18 relocations reproduces all 1,516 raw function bytes. The
registration handler and five-state unwind table also agree after placement.

Flattening the dimension locals restores both residual regions at 98.3965%.
Updating the row in place instead changes already matching upper-Y loads
(97.4473%); naming column before row loses the recovered map-index operand
order (98.3535%). Using map-item references is byte-neutral. No helper
declaration, inline control, or unused operation is needed. This is a measured
source/value-lifetime model for a retail-only function, not proof of the
original local names or lexical scope.

### 6l. A retained comparison distinguishes free and member interfaces

The grid-set lookup in `PaintPoint` calls 0x5b8ca0 with the two point addresses
in ECX and EDX, without stack arguments. The 32-byte callee compares unsigned
y, then x, and returns with plain `ret`. That boundary contradicts the former
`TRmgGridPoint` member comparison, which passes its right reference on the
stack. An ordinary free `bool operator<(const TRmgGridPoint&, const
TRmgGridPoint&)` reproduces all 32 raw bytes without relocations.

The source-label scanner now joins this bounded `operator<` spelling to its
VC6 `??M` public, alongside the arithmetic operators. Equal-size join tests
keep the operators distinct and reject `<=`, `<<`, and template-owner forms;
the 130-test label suite passes. The source declaration remains the name owner.

### 6m. Copy initialization, named return, and the base tile's lifetime

The grid translation in `rmgTerrainPainter::paintPoint` (0x5b4b20) exposed
three distinct source-form effects with the comparison-return terrain predicate.
First, construct its working point through a copied coordinate value:

```cpp
TRmgGridPoint result = TRmgGridPoint(x, y);
result += offset;
return result;
```

Direct initialization of `result` removes retail's original-x store at
0x5b4e3f. Copy initialization restores the EBP-0x14 store and lifts the caller
from 98.2893% to 98.5805%. Returning `result += offset` still uses the wrong
direction register and schedules the two additions differently. Applying the
compound operation separately and returning the named result reproduces the
entire translation sequence at 0x5b4e38..0x5b4e5a (99.5389%). Direct initialization
of the caller's nearby point instead of copy initialization is neutral.

The remaining frame excess is a different lifetime. A scope around the named
frame selection, base-tile construction, and `setTile` call ends the tile's
lifetime before the worklist updates. This reduces the frame from 0x58 to
retail's 0x50 without disturbing the original-x store (99.5570%). The earlier
tile-scope probe with flat field assignments was neutral; that observation did
not transfer to the reconstructed constructor and point-copy state.

The current 48-block CFG has matching branch destinations. Two commuted cache
multiplications and the distance-wrapper expansion remain. Coordinate-constructor
body assignments, multiplication operand reversal, and adjacent repair guards
are neutral; rewriting repair selection as nested conditional values changes
the exact destructors' byte-result tests and was reverted. The grid constructor
and free comparator remain raw-exact at 24 and 32 bytes, and the shared return
change also improves `repairTerrainPoint` from 81.8432% to 82.8347%.

The later guard-return predicate changes this return-form result. It restores
the retail distance-wrapper call but leaves the named point return at 99.0163%.
Returning `result += offset` instead restores the complete translation and
backedge, reaching 99.9204%. Its ordinary compiler output has retail's 1,483-byte
length, 0x50-byte frame, and all 61 resolved relocations. Only eight bytes in
the two commuted cache products remain. Reversing the field additions instead
retains an unwanted helper and gives 97.4864%. A source-form observation must
be rechecked after a later inline decision changes; the earlier named-return
result was not a permanent requirement of the point class.

### 6n. Observe multiplication operands before allocation

<!-- c2-role: function 0x2003e constrainNativeOperands -->
<!-- c2-role: global 0xa5a88 nativeInstructionNames -->

At the 99.9204% `paintPoint` checkpoint, the remaining eight raw bytes exchange
the memory operands of two `mov ebx, ...` / `imul ebx, ...` pairs. A passive
scratch shim now observes the pinned C2 machine-operand constraint routine at
RVA `0x2003e`. Its instruction-name table at `0xa5a88` identifies opcode `0xc1`
as `_imul2`. The trace sees three such operations for this caller. At the two
residual sites, the first source has kind byte 1 and a field-symbol record
with offset 4; the second has kind byte 6. At the already-exact entry product,
both sources have kind byte 6. These are compiler representations after
optimization, not recovered retail declarations or a proven explanation of
the commutation.

The scratch trace hooks whole instructions, preserves registers, flags and
last-error state, and passes complete object identity against unmodified VC6
outside the COFF timestamp. Its runner restores the normal shim in `finally`.
Working artifacts live in `build/rmg-multiply-trace/`; the copied, hash-gated
Ghidra project lives in `build/re/vc6/`. Neither instrumented compiler output
nor edited compiler state is used for matching.

Source controls distinguish this observation from a fix: a pointer cache
parameter, a named cache-cell reference, and signed multiplication intermediates
leave the same eight bytes. A const-reference binding for the translated point
changes the distance-wrapper expansion and gives 98.5986%. Restoring the
434-byte retail rectangle caller before `paintPoint` also preserves its eight
remaining differences. The operand-class observation therefore does not justify
changing the canonical point interface or adding an inlining control.

The next passive trace locates the actual ordering decision. The optimizer
driver starts at `0x6819b`. A hook at its phase checkpoint, C2 RVA `0x9ab4`,
snapshots the generic multiplication
nodes between optimization passes. Immediately before the second
`FUN_1070f349(body, 2)` call, both residual products have width first and the
local row second. Hardware write watchpoints on their source-list heads catch
the reversal at `0xd008` (reported EIP `0xd00b`). This is the source-list sort in
`FUN_1070cee1`, using `FUN_1070e725` and comparator `FUN_1070e12f`.

<!-- c2-role: function 0x9ab4 phaseCheckpoint -->
<!-- c2-role: function 0x6819b optimizeFunction -->
<!-- c2-role: function 0xf349 optimizeExpressions -->
<!-- c2-role: function 0xcee1 rankAndSortOperands -->
<!-- c2-role: site 0xd008 storeSortedOperands -->
<!-- c2-role: function 0xe725 sortOperandList -->
<!-- c2-role: function 0xe12f compareOperandRanks -->

The comparator orders the packed unsigned value at operand offset `+0xc`.
`FUN_1070d19e` combines an expression-demand value in the top byte, a recursive
cost component in the next byte, and the low 16 bits from `FUN_1070d25d`.
The earlier phase snapshot had width at `0x01020060` and row at `0x00016660`,
but those are **not** the two ranks compared by the sort. A later passive
watch on the width rank catches instruction `0xcf2a` (reported EIP `0xcf2d`)
recomputing it to `0x00010007` before the source-list reversal. At the actual
`0xd008` write, both residual products have row `0x00016660` first and width
`0x00010007` second. `0xe12f` returns positive when the left rank is smaller;
the list sort puts the larger rank first. The cost components therefore tie,
and the low hash bits decide this ordering. The earlier conclusion that
changing those low bits could not matter was based on a stale phase rank.

The rank/definition trace and the rank-plus-list watch both preserve complete
object identity outside the COFF timestamp; their runner restores the normal
shim. Working artifacts are `build/rmg-multiply-rank-origin/`. The row's field
symbol has generated handle `0x333` at this checkpoint, while its original
eight-byte local has a different handle. Runtime pointers remain only guarded
selectors for these observations, not stable compiler identities.

The hash cases are directly readable in the pinned table at `0xdf00` /
`0xdee0` and the labeled `hashOperand` listing. Kinds 1–3 enter `0xd305`;
the row's category-4 field symbol uses the handle at symbol `+0x1c`, folds
its two 16-bit halves, and rotates that result left by five bits. Handle
`0x333` therefore gives `0x6660`. Kinds 5–6 enter `0xd2c9`: they combine the
folded displacement at operand `+0x24`, opcode minus `0x145`, and the base
operand hash shifted left by eight, retaining the low 16 bits.

Here the width base is a kind-2 operand with a category-3 symbol, handle
`0x41c`, and no definition link at the observation point. The category-3
case at `0xd365` hashes that handle shifted left by six. With width opcode
`0x14c` and zero displacement in this IR representation, its final hash is
`(7 + (0x41c << 14)) & 0xffff`, namely `7`. The corresponding values for
base handles ending in 1, 2 or 3 modulo four would be `0x4007`, `0x8007` or
`0xc007`. These formulas describe C2 symbol identities, not source names or
machine register numbers. They do not authorize manufacturing declarations
or changing compiler state to force a match. The remaining source question
is how the natural expression/temporary creation order produces those handles.

<!-- c2-role: function 0xd19e computeOperandRank -->
<!-- c2-role: function 0xd25d hashOperand -->
<!-- c2-role: site 0xcf2a storeRecomputedOperandRank -->

The width base's origin is now observed as well. A passive hook at `0x6ac8`,
after the call to `0x673f` and before `0x235b` constructs a kind-2 operand,
records the returned symbol and the original expression operands. At this
checkpoint, handle `0x41c` represents generic addition (`0x16d`) of the
original `this` symbol (handle 1) and constant `0xc`. It is the shared address
of `m_width`, first encountered in the opening `setTile` expansion, rather
than the symbol for `this` itself. Five observed requests reuse that symbol,
including the addresses feeding both residual products.

The immediate creation sequence includes address `point + 4` (`0x419`),
its loaded value (`0x41b`), address `this + 0xc` (`0x41c`), its loaded value
(`0x41e`), and their product (`0x41f`). These are observed generated handles,
not original source locals. The call at `0x11593` supplies the expression
opcode, type and operand list to `0x6ab4`; the latter obtains a symbol via
`0x673f` and creates its value operand via `0x235b`. The physical Ghidra entry
at `0x1158b` is only a fragment, so it is not assigned a whole-function role.

<!-- c2-role: function 0x6ab4 makeExpressionValueOperand -->
<!-- c2-role: function 0x673f getExpressionValueSymbol -->
<!-- c2-role: function 0x235b makeSymbolValueOperand -->
<!-- c2-role: site 0x6ac8 expressionValueSymbolReady -->

This trace checks the active function name, the returned symbol's readable
record and the observed handle range before recording it. It preserves the
entire object outside its timestamp and restores the normal shim in `finally`.
Artifacts are in `build/rmg-width-address-origin/`. Ordinary source controls
using a pointer cache alias, reversing the multiplication, or putting x first
in `setTile` leave all eight caller differences. Naming width changes the
entry product and distance-wrapper expansion, but leaves both residual
products row-first. Thus merely rewriting the setter expression has not
recovered the required natural symbol order.

The handle allocation explains another limit on that search. `0x64bb`
requests allocation kind `0xf` from `0x1da6`, then sets the returned record's
category byte to 3. The pinned dispatch tables at `0x226c` / `0x2254` send
kind `0xf` to `0x1eb4`, which consumes successive 84-byte records from the
arena at `0x9bc74`. This is a distinct path from the kind-3 free list at
`0x9bc60`; that free list must not be used to explain expression allocation.

`0x53f4` allocates a 12-byte header followed by 32 records. It preassigns their
handles at record offset `+0x1c`, incrementing `0x9bc4c` by 32 per arena. The
initialization at `0x1bcc6` zeroes that counter and creates four other arenas
before clearing the expression-arena pointer. Therefore allocations into a
different arena can change an expression handle's upper bits without changing
its low five bits. The slot within its own arena is the relevant local order;
total declarations or allocations across all pools are not an equivalent count.

A passive hook at `0x64c5` checks the active caller, readable arena bounds and
84-byte slot alignment before recording allocation. It confirms that the
current width address is slot 28 of the arena beginning at handle `0x400`:
handle `0x41c`. The named-width setter control instead creates that address at
slot 25, handle `0x419`. Its low-bit hash contribution changes from `7` to
`0x4007`, but the ordinary output still has both residual products row-first.
Both traces reproduce their respective complete ordinary-compiler objects
outside COFF timestamps, and the normal shim is restored. The artifacts are
`build/rmg-expression-arena-trace/` and `build/rmg-expression-arena-run.log`.

<!-- c2-role: function 0x64bb makeExpressionSymbol -->
<!-- c2-role: function 0x1da6 allocateSymbolRecord -->
<!-- c2-role: function 0x53f4 allocateSymbolArena -->
<!-- c2-role: function 0x1bcc6 initializeSymbolArenas -->
<!-- c2-role: global 0x9bc74 expressionSymbolArena -->
<!-- c2-role: global 0x9bc4c nextSymbolArenaHandle -->
<!-- c2-role: site 0x64c5 expressionSymbolAllocated -->

The distinction develops before that sort. After inlining, the two row reads
are indirect operands (kind 6). A later passive snapshot at `FUN_1070f1f0`
entry separates the first `FUN_1070f349` call from that follow-up: the row
operands are already kind 2 when the follow-up begins. The later pass
`FUN_107261bf` changes those to kind 1. Their
width operands remain indirect. These observations describe candidate C2
state, not the retail compiler's input or recovered point declarations.

<!-- c2-role: function 0x261bf promoteLocalOperands -->

A separate opcode write watchpoint finds the generic multiplication-to-native
rewrite at `0x2873b` (reported EIP `0x2873e`) in `FUN_10728610`. Its variable
32-bit multiplication arm selects `_imul2` without reversing the source list.
The later operand-constraint routine therefore receives the already ordered
operands. These phase and write-watchpoint traces each pass whole-object
identity against an unmodified compile outside the COFF timestamp. The Wine
watchpoints use a vectored single-step handler and two four-byte write slots;
the handler preserves last-error state and clears handled debug status. The
source-list watch disables each slot after native lowering replaces its list.
Scratch runners restore the normal compiler shim in `finally`.

<!-- c2-role: function 0x28610 lowerNativeOpcode -->
<!-- c2-role: site 0x2873b selectNativeMultiply -->

A **modified-compiler counterfactual**, kept outside matching, swaps only those
two source lists after expression optimization and before native lowering.
That produces all 1483 retail bytes, with all 61 named relocations independently
resolved. Compared with its unmodified control, the only section-data changes
are the eight `paintPoint` bytes; the corresponding COFF section-definition
checksum also changes. This proves that the two operand decisions suffice to
explain the residual. It does **not** establish a source reconstruction, and
the ordinary compiler's result remains 99.9204%, with eight raw differences.

Working artifacts are `build/rmg-multiply-phases/`,
`build/rmg-multiply-commute-watch/`, `build/rmg-multiply-watch/`, and the explicitly
separate `build/rmg-multiply-counterfactual/`. The first opcode-watch run selected
the entry variable product and a constant scale product; the later source-list
watch selects both residual products. Runtime addresses and ordinal positions
are observations of this checkpoint, not stable compiler interfaces.

The first residual's field fold is now traced directly. Its row operand comes
from a clone of an address of an eight-byte local aggregate. The clone routine
at `0x156f` allocates through `0x12f7`, using the kind-indexed size table at
`0xa0184`. A guarded watch
on that live clone catches `FUN_1070c6fd` replacing the aggregate symbol with
an offset-4 field symbol at instruction `0xc832` (reported EIP `0xc835`).
`FUN_10741ed0` then changes the operand's type at `0x41fb5` and calls
`FUN_1070fd9d`, which changes its kind from 3 to 2 at `0xfdaa` (reported EIP
`0xfdad`). Thus the observed chain is address-plus-field-offset folding,
dereference folding, then the second expression pass's operand sort. The
clone and field-watch runs both preserve whole-object identity. The artifacts
are `build/rmg-multiply-row-promotion/` and `build/rmg-multiply-row-watch/`.

<!-- c2-role: function 0x156f cloneOperand -->
<!-- c2-role: function 0x12f7 allocateOperand -->
<!-- c2-role: global 0xa0184 operandSizes -->
<!-- c2-role: function 0xc6fd foldAddressOffset -->
<!-- c2-role: site 0xc832 storeFieldSymbol -->
<!-- c2-role: function 0x41ed0 propagateDereference -->
<!-- c2-role: site 0x41fb5 setDereferencedType -->
<!-- c2-role: function 0xfd9d dereferenceOperand -->
<!-- c2-role: site 0xfdaa makeLocalValue -->

The address fold calls `0x37fc` to find or create the offset field symbol.
It is gated by the dword at `0xac128`. A cold block at `0x837d5`, reached
from C2's input-header decoder near `0x69616`, writes that flag after testing
bit 3 of a header byte. Its source option meaning is still unknown; the
label describes only the observed folding guard.

<!-- c2-role: function 0x37fc findOrCreateFieldSymbol -->
<!-- c2-role: global 0xac128 addressFoldDisabled -->
<!-- c2-role: site 0x837d5 disableAddressFold -->

Runtime pointers alone are insufficient to join these observations. The
instrumented DLL's layout can shift C2 allocations while the emitted object
remains identical, and allocations are also reused later in compilation.
An initial watch selected by a prior run's pointer failed its guard. The
successful watch locates the clone using the observed local symbol handle
and clone ordinal, then checks its live opcode, type and owner before arming.
Those selectors are still checkpoint-specific. Traces must establish the
allocation's identity and lifetime before attributing a later write to it.

An ordinary-compiler source control at this same checkpoint also rules out
splitting the cache calculation into assignments: initializing `index` from
width or row, applying `*=`, then adding x produces the same 1483-byte caller
and the same eight differences. The unmodified scratch control and both
variants resolve all 61 relocations in the raw verifier. These candidates
remain outside the matching objects; see `build/rmg-width-assignment-control/`.

The residual is now closed with ordinary VC6. `paintPoint` passes
`rmgTerrainTile(m_paintTerrain, frame)` directly to `setTile`, and names the
direction-table element as `const TPoint& offset` before `point + offset`.
It retains the ordinary guard-return terrain predicate and the compound
grid-addition return. All 1483 retail bytes match, including 61 independently
resolved named relocations. No compiler-state mutation or inlining pragma is
part of this source.

The source fix has two measured effects. A passive hook at `0x64c5` now also
records the pending expression operands from `0x99620`. In the earlier
named-tile source, handles `0x40c` through `0x411` comprise three pairs of
opcode `0x165` / `0x14c` expressions. Their aggregate field records describe
regions `[4,12)`, `[8,12)` and `[9,12)` of the tile. These are observed compiler
regions, not a recovered name or complete meaning for opcode `0x165`. The
direct temporary removes those six entries from the prefix. The same width
address therefore receives handle `0x416` instead of `0x41c`. After the second
expression pass, both products have width rank `0x00018007` first and row
rank `0x00016660` second. The row still has handle `0x333`.

The temporary also lowers the caller's observed `cb` from 920 to 914. With
the guard predicate, that changes a nested tree-find expansion and gives the
inner three-argument `_Distance` wrapper a budget of 58 against its cost of
41. A comparison-return predicate restores the tree-find expansion but still
gives that wrapper budget 45, so it expands into the four-argument overload.
The named direction reference raises the caller to `cb=919`; with the guard
predicate the wrapper budget is 38 and it remains a call, as in retail. The
original `cb=920` and recovered `cb=919` sources share that decision. Exact
source recovery does not require reproducing the earlier candidate's total
cost or any aggregate compiler-node count.

The old source, direct temporary with either predicate, and final exact source
each pass whole-object identity between passive and ordinary compiles outside
the COFF timestamp. All runners restore the normal shim in `finally`.
Artifacts are `build/rmg-expression-prefix-trace/`,
`build/rmg-direct-tile-budget-trace/` and `build/rmg-direct-tile-exact-trace/`.
The prefix also supplies negative controls: reversing the constructor's first
two field stores leaves the same allocation prefix while changing emitted
entry bytes; an owned two-byte flip value changes the middle expressions but
still consumes six slots and leaves width at `0x41c`. Thus neither source
store order nor field grouping alone predicts the required operand rank.

<!-- c2-role: global 0x99620 expressionArguments -->

## 7. Files

| path | role |
|---|---|
| `scripts/homm3/vc6/_regmodel.py` | the pure model (ranking, exclusions, first-fit, order solver) |
| `scripts/homm3/vc6/reg_model.py` | v1 guided sweep (`run_why`, unchanged default) + v2 model path (`run_model`, `--model`) |
| `scripts/homm3/vc6/ghidra_scripts/regasg_probe.py` | read-only dump/refs/bytes queries over the persisted C2 project |
| `build/re/vc6/raw/regasg/` | RE working data (gitignored) |

Follow-ups: land the assignment-order probes as
`b*` oracle cases + catalog rows (B1 gains its first standalone probe);
wire a `--model` flag into `homm3 vc6 why-reg`'s argparse; chase the
processing-order source in p2symtab.c (the C1 mechanism's last mile).


### Value-returning point addition and its argument lifetime

`ClipRmgBoundaryPoint` (0x53cac0, 614 bytes) reproduces retail's integer
clipping arithmetic with a preserved original point and a separate working
point. `clipped += delta * distance / divisor` gives 64.11% and a 0x24-byte
frame. Recovering `TPoint::operator+` and assigning the returned value gives
98.04%, the retail 0x1c-byte frame, and all 40 control-flow blocks. Keeping
`+=` with that same new header is byte-flat at 64.11%; an unused declaration
is not responsible for the improvement.

Putting each distance expression directly inside the product reaches
99.11%. The final maximum-Y multiply then identifies the addition argument:
`operator+(TPoint)` reaches 100%, while `operator+(const TPoint&)` leaves the
remaining register/scheduling difference. The subtraction operator's
argument convention is independently flat, so it retains its existing
const-reference declaration. Scalar operand reversal, named numerator or
bound values, and a member-wise scaling result are also flat at 99.11%.

The earlier argument-slot reading was insufficient: mutating the first
input and saving a copy reached 80.07%, while reusing the second input was
63.97%. Retail reuses dead parameter storage without requiring the source
to mutate that parameter. Recover the arithmetic operators and their
argument lifetimes before replacing a separate working value with an input.


### Separate object lifetimes can eliminate a by-value parameter copy

In `CreateRiver` (0x548df0), reusing the initial reset position for the
worklist position retains an extra predecessor snapshot and a 0xc8-byte
frame (71.86%). Giving the reset pass its own block and declaring a separate
worklist position later removes that snapshot and recovers retail's
0xbc-byte frame (71.47%). The queue insertion's distinct next-position copy
remains. Merely renaming the reset position without ending its scope leaves
the larger frame (71.31%).

The reset position's constructor is out of line and receives its address;
the later worklist position is populated by an inline copy. These controls
establish a useful lifetime hypothesis, not a recovered Dreamcast scope or
a proven internal optimizer mechanism. Changing the predecessor helper to
const-reference also removes the snapshot, but the neighboring road caller
independently preserves a by-value copy. Keep that shared interface and
investigate caller lifetimes before inferring argument conventions from
one optimized expansion. The higher score remains banked in MAX/history.

### Name the integer lookup result before its consumers

`highScoreWindowHandler` (0x4ea1d0) reached 100% after restoring its source
helper boundaries, animation locals, and final dialog-exit flag. Dreamcast
hiscore.cpp:1165-1166 binds the frame value and creature-pointer slot before
the increment at 1168; explicit references preserve that evidence and prevent
the animation update/draw tail merging with the category tail. Restoring the
switch breaks feeding the final exit flag left 97.90124%, with all 47 CFG
blocks structurally exact.

`homm3 vc6 why-reg --model` isolated caller-saved register differences in the
reset arm's expanded `getMonType` lookup. Binding its integer result to
`monsterType` before the object lookup produces retail's EAX result and reaches
100%. Binding the resulting `CObjectType*` instead is byte-identical to
97.90124%; the diagnostic's adjacent exit-store swap is also byte-neutral.
This is a concrete value/lifetime reconstruction, with no artificial calls,
declarations, or inline controls. The exact 1120-byte padded caller has SHA-256
`8406701804be9029cf749a7e0781812f2ad8063406a6c5594624730d54c1781c`.


### Check retained references before blaming register-order state

`SCampaign::save` (0x48ae90) had matching control flow and call order at
78.8074%, but retail kept each score record and each inner pool vector in EDI
across writes. Naming those references raised the score to 99.6062%. The
score reference alone reached 88.0368%; the pool references alone reached
85.4136%. The register-model diagnosis of permuted callee-saved roles did
not establish that source changes could not recover them.

Per-write scalar lifetimes let VC6 reuse the dead stream-parameter home for
byte, int, and short values. Combined with retail-proven short serialization
buffers and a shared outer counter, these reached 99.9858%. A short cast
assigned to an int buffer emitted `movsx`; an actual short local emitted
retail's `mov ax` and widened stack store. Five non-relocation bytes remain:
the frame size and four offsets of the artifact buffer. Additional loop/phase
scopes, word declaration hoists, and unsigned-short spelling were byte-flat.
The function's own comment records the controls; frame storage reuse is
still unresolved, so this is not a byte-exact result.

### Materialize call-argument values before diagnosing register rotation

`combatManager::markMoat` (0x421590) reached 100% by naming the moat damage
before each `getLossCombatValue` call. Retail loads the defending town before
`killsOnly`; the local lets VC6 reproduce that scheduling and assign
estimate/attacks/hex to ESI/EDI/EBX. The inline table expression scored 79.9375%
with the same CFG. Caching only the town pointer reached 91.625%; changing the
index types or sharing the hex local was byte-flat. All thirteen retail CFG
blocks match after the damage local, and the exact padded 240-byte function
has SHA-256 `0db9e77505e353b05f2a08f719afd06ca5da16de4600205c4e0218c5323b6f55`.

`type_AI_spellcaster::getProtectionValue` (0x4396e0) provides a second
control. Splitting mana-cost calculation from the comparison with current
mana reaches 94.5232%; materializing the complete base damage before
`modifySpellDamage` reaches 89.5106%. Together they reach 100%, restoring
retail's ESI spell-offset lifetime and its reloads of the army parameter.
Dreamcast ai_tactical.cpp:2035-2037 separates the mana-cost call and mana
read; the retail damage calculation precedes its argument loads. Restoring
`army::is`, `SpellIsAvailable`, the const signature, and the ordinary
`get_duration` helper is byte-flat before these two statement changes.
The exact padded 704-byte function has SHA-256
`9c8c35382ab8b583231581f37408868f3409ae471d6f8b03eadcdc18ddff6fcf`.

### Inspect canonical return construction when the caller's late schedule differs

`connectJunctionEntrance` (0x5443a0) had all 40 CFG blocks aligned at
99.5699%. Its neighbour lookup loaded map height and multiplied by the level
register; retail copied the level register and multiplied by memory. The
lookup itself was not the source change needed. Constructing the ordinary
`TRmgVector::operator*` result with a default local and x/y member assignments
recovers the caller's schedule, while the retained 29-byte operator at
0x5fdcd0 remains exact. Its parameter and return ABI, definition visibility,
and every source call are unchanged.

A Voronoi source-family search found the return form; four reproduced controls
then isolated its effect from the unrelated `buildVertices` local lifetimes:

| Caller locals | Scale operator result | Vertices | Junction entrance |
| --- | --- | ---: | ---: |
| Original | Constructed temporary | 31.6268% | 99.5699% |
| Named radius / bound side references | Constructed temporary | 37.8873% | 99.5699% |
| Original | Named, assigned x/y | 31.6268% | 100% |
| Named radius / bound side references | Named, assigned x/y | 37.8873% | 100% |

Only the scale operator change is retained. The full build raises exactly one
score and leaves every other tracked function unchanged. The junction's three
vector-insertion relocation spellings remain the existing eight-byte-element
ICF aliases; their positions and overload arities do not change. This is not
evidence that arbitrary helper edits improve scheduling: it is a controlled
example where identical standalone code did not imply identical expansions.

### Float conversion parameter storage

`Bitmap16Bit::colorize` (0x44e940) reaches 100% from 98.8301% when its
ordinary file-static `ftol` updates its by-value `double d` and reads that
parameter's low word, matching DC 0x50a9c lines 62–63 and the existing bitmap24
source. The invented result union caused eighteen separate inlined scratch
slots (0xcc frame); parameter ownership restores retail's 0x48 frame and
load/store order. Sixteen source states produce eight reproduced objects;
removing `__forceinline`, restoring plain pixel pointers, and moving the
zero-area check to an early return independently leave scores unchanged.

### Field-bound references, scoped copies, and loop-variable reuse (addSite)

`TRmgVoronoi::addSite` (0x5fd790) closed from 97.6518% to 100% with every
call decision already matching; the last three residuals were frame and
register facts, each isolated by a single-axis control from a complete
64-state source family (2 x 4 x 4 x 2, all scored):

- **A reference bound to a member field loads it once; a by-value copy
  through an accessor is re-read from the object.** The segment predicate's
  line origin as `const TPoint& origin = edge->m_sitePosition;` loads
  `org.x` into ecx once and spills the derived `dx`/`dy` to
  [ebp-8]/[ebp-0xc] for the four products, as retail. `TPoint origin =
  edge->getSitePosition();` re-reads `[ebx]` for each use and keeps `dx` in
  a register; with the other three spellings exact it scores 90.3482%.
- **Two by-value copies in one scope allocate in a different order than
  two copies that die in turn.** The coincidence test as two scoped blocks,
  each copying one endpoint and comparing, reads eax/ecx then ecx/edx as
  retail; declaring both copies before one `||` condition keeps the same
  0x30 frame and swaps those registers over 14 rows (98.5536%). A single
  `||` over the accessor calls, or two bare `if`s, scores 93.7411%.
- **Assigning a call result back to the loop variable before deriving
  from it homes it differently from a named temporary.** `base =
  connectEdges(edge, base->getTwin()); edge = base->getPrevious();` homes
  `base` in [ebp-8] and the connected edge in the dead `zone` argument
  slot; a named `next` local in any assignment order shares `base`'s slot
  (90.6-90.9%).
- **Reading a site through `getTwin()->getSitePosition()` rather than
  `getOppositeSitePosition()`** in both suspect tests keeps `point.m_y` in
  EDI and `edge` in EBX; the composite accessor swaps them (93.7411%).

All four are byte-neutral in the helpers they touch and change no call
decision, so `predict-inline --trace` cannot see them; a family that holds
the inline state fixed and varies only lifetimes and access spellings is
the tool. None of these is a recovered Dreamcast scope: RMG has no
symbols, and the spellings are period-style guesses that VC6 confirms.
