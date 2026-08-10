# The C2 register allocator - preference ranking, assignment order, why-reg v2

Phase 4 of the vc6 area, deliberately a MINIMUM SLICE: reverse-engineer
enough of C2's register-assignment ORDERINGS and TIE-BREAKS to predict the
behavior catalog's **B1** class (the whole-body ESI/EDI/EBX role swap - the
modal plateau wall), plus the corners the same mechanism explains for free
(B8 zero-CSE register choice, B14 EAX-first naming lever, B15's eager char
homing). Spill heuristics, coalescing and the full allocator are explicitly
NOT modelled. Subject: the pinned **C2.DLL 12.00.8447** (hash-gated via
`_toolchain.PINNED`, image base `0x10700000`; all addresses below are RVAs).
Every byte and probe cited was measured 2026-08-10 on the pinned binaries
under wine.

## TL;DR

* C2 numbers the eight GPRs **machine encoding + 1**: 1=EAX 2=ECX 3=EDX
  4=EBX 5=ESP 6=EBP 7=ESI 8=EDI. The numbers are the symbol HANDLES of
  eight pre-created register symbols in the back-end symbol hash.
* There is **one allocation preference order**, a 0-terminated dword table
  `{1,2,3,7,8,4,6}` = **EAX, ECX, EDX, ESI, EDI, EBX, EBP** (const in
  `.rdata:0xa09f0`, runtime copy in `.databe:0xadff4`), consumed
  **first-fit** by regasg.c. There is no separate per-class order - the
  classes fall out of exclusions (call-crossing values lose EAX/ECX/EDX,
  byte-sized values lose ESI/EDI/EBP, ESP never, EBP only when frameless).
* Pseudos are assigned **in creation/processing order**: the first
  call-crossing value takes ESI, the second EDI, the third EBX, the fourth
  is frame-homed. Swapping two values' creation order swaps their
  ESI/EDI bindings - byte-proven with probes, and the mechanism behind B1.
* For named locals, creation order is the front end's **symbol-handle
  order** - directly visible in the IL `sy` stream
  (docs/vc6/il-format.md); the B14 naming lever works by minting the
  handle earlier.
* On the plateau B1 rows the two compiles (retail's and ours) define the
  SAME values at the SAME schedule slots and only the ESI/EDI picks are
  permuted: the same allocator received the same pseudos in a different
  processing order from a different front-end state. When the transposed
  pair is a parameter/`this` against an expression value, no statement-
  local spelling reaches it (the alias is copy-propagated - measured);
  the residual is the **C1 handle-state class**, and `why-reg --model`
  now says so after ONE compile instead of a 20-800 compile sweep.

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

Consequences the corpus already knew as separate facts:

* scratch values: EAX first, then ECX, EDX (B14's "first-preference EAX",
  B10's eax->ecx chain);
* call-crossing values: ESI, then EDI, then EBX - the observed B1
  register population everywhere in the corpus;
* a hoisted zero (B8) enters the same walk: EAX in a leaf, the first
  free callee-saved under calls;
* a byte-sized call-crossing value's candidate set shrinks to {EBX} -
  measured: VC6 frame-homes it at its definition and lets the widened
  reload take the normal walk. This is the mechanism BEHIND B15's "VC6
  homes char locals far more eagerly than ints".

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

So the B1 swap is NOT a different ranking and NOT a different schedule:
the same first-fit walk was fed the same pseudos in a different
processing order. The processing order is upstream of regasg.c - it is
front-end symbol/handle state (the C1 mechanism made register-visible;
cf. the il-format.md killer result: an unused struct shifts every later
handle by +9). Consistent with that: naming levers (B14) flip it when
the competing pair are expression values a name can hoist, and no
statement spelling flips it when one side of the pair is a parameter or
`this` (the alias is copy-propagated - measured on get_attack_change;
window.cpp:391's 792-compile sweep is the class's historical baseline).
WHERE exactly C2/C1XX turn handle values into processing order (the
p2symtab hash, `[handle & 0x3ff]` buckets) is the next phase's RE
target, not claimed here.

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

Honest accuracy statement: the model predicts the pinned compiler's
callee-saved assignment from creation order in 5/5 standalone probes,
and predicts RETAIL's binding from the definition tables in 2/2 real B1
cases; it does NOT predict our own compiler's plateau-side processing
order from source (that order is front-end handle state), and it cannot
flip a parameter/`this` pairing by spelling - it now proves that in one
compile instead of a sweep. Encoder-level tie-breaks (B17 length
feedback, B18 SIB operand order) are not allocator decisions and are
out of scope.

## 7. Files

| path | role |
|---|---|
| `scripts/homm3/vc6/_regmodel.py` | the pure model (ranking, exclusions, first-fit, order solver) |
| `scripts/homm3/vc6/reg_model.py` | v1 guided sweep (`run_why`, unchanged default) + v2 model path (`run_model`, `--model`) |
| `scripts/homm3/vc6/ghidra_scripts/regasg_probe.py` | read-only dump/refs/bytes queries over the persisted C2 project |
| `build/re/vc6/raw/regasg/` | RE working data (gitignored) |

Follow-ups for a supervised session: land the assignment-order probes as
`b*` oracle cases + catalog rows (B1 gains its first standalone probe);
wire a `--model` flag into `homm3 vc6 why-reg`'s argparse; chase the
processing-order source in p2symtab.c (the C1 mechanism's last mile).
