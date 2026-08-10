# The C2 atlas — navigating the VC6 back end by translation unit

A regenerable, headless-Ghidra map of the pinned C2.DLL (12.00.8447, sha256
`a0cc45f8…`, image base `0x10700000`): which `.text` region belongs to which
compiler source file, and where the optimizer's global state lives. This is
the substrate the inliner-model and regalloc phases consume — **not** a
decompilation. Numbers below are from the 2026-08-09 run; regenerate with:

```sh
homm3 vc6 atlas            # status of the two evidence tables
homm3 vc6 atlas --regen    # rebuild evidence/vc6/c2-{tu-map,globals}.tsv
python3 -m homm3.vc6.atlas --regen --reimport   # + rebuild the Ghidra project
```

The subject is hash-gated through `_toolchain.resolve("C2.DLL")` before any
import — a wrong pressing hard-fails. The Ghidra project lives under
gitignored `build/re/vc6/` (scaffolding, optional); the two TSVs are the
deliverables. Standing: ANALYSIS OUTPUT, not retail evidence — regenerate,
never hand-edit.

## 1. Method

### 1.1 The ICE-string anchors

C2.DLL retains 48 compiler-source-path strings
`E:\8447\vc98\p2\src\{Common,P2,P2\x86}\*.c` in `.rdata` (rva
`0xa59f8`–`0xa9a04`), one per translation unit, referenced from that TU's own
internal-compiler-error sites. Every site has the same byte shape
(`__fastcall` helper; the error formatter uses `"%s(%ld) : "`):

```
ba <line#-imm32>      mov  edx, LINE
b9 <string-va-imm32>  mov  ecx, offset "E:\8447\vc98\p2\src\...\file.c"
e8/e9 <rel32>         call/jmp ICE-helper        ; e9 = noreturn tail-jump
```

A site referencing `inline.c`'s path is code compiled from inline.c — the
string literal is lexed in the TU itself — so its **physical address** pins
that region of `.text` to inline.obj. Two independent site-discovery
channels are unioned: Ghidra's analysis references to the string address
(`ghidra-ref`) and a raw little-endian imm32 scan of the `.text` file bytes
via the gated PE reader (`imm-scan`, no Ghidra involvement).

### 1.2 Interval attribution, not getFunctionContaining

Sites map to functions by **link-order interval**: the function whose
`[entry, next_entry)` span contains the site. Ghidra's own
`getFunctionContaining` is unusable for this: the `e8`-to-noreturn-helper
chains make its auto-analysis merge distant blocks into non-contiguous
bodies (measured: 812/2474 functions fragmented; the 21 p2symtab.c sites,
all physically in `0x833b3`–`0x84060`, came back "owned" by 13 functions
scattered across the whole image, e.g. fn `0x1117` whose body is
`0x1117–0x1153` ∪ `0x83b2b–0x83b39`). The raw table keeps Ghidra's opinion
per row as a diagnostic (`n_frag`; 379 sites diverged).

### 1.3 Anchor hygiene

The compiler sources carry `#line`-style generated code: two coff.c ICE
blocks sit physically inside other TUs' regions. Before propagation:

* a function whose sites name several TUs resolves by site majority
  (`0x7d5ae` main.c-over-emit.c, `0x7e2cf` p2pragma.c-over-tuple.c);
* a majority tie drops the function (`0x86e6a` fg.c/sdsu.c, `0x931b6`
  factor.c/switch.c — one carve interval holding both TUs' single sites);
* an isolated anchor contradicting both anchored neighbours is demoted as
  an out-of-place outlier (`0x6ba6c` and `0x83c5e`, both coff.c blocks
  inside MDmisc.c / p2symtab.c regions).

Six functions were dropped/demoted in total; every one is printed by the
regen run.

### 1.4 Bracket propagation

VC6-era linking keeps one .obj's `.text` contribution contiguous (the
per-TU site clusters are tight and strictly ordered — measured, §2), so
between two consecutive anchored functions of the same TU every function
belongs to that TU (`confidence=bracket`, `evidence=locality`). Across a TU
boundary the gap is ambiguous and spelled `prevTU|nextTU`; before the first
anchor / after the last it is `?|firstTU` / `lastTU|?`.

### 1.5 Corroboration (the gate-shaped check)

Every anchor row is re-proven **from raw bytes with no Ghidra
involvement**: the TU string's VA must occur as an imm32 inside the
function's `[entry, next_entry)` span, via `_toolchain.Binary` only.
**115/115 anchors corroborate.** The checker ships a negative control — it
must reject a deliberately wrong (function, TU) pairing or the run dies —
proving the check can fail. A wrong-hash C2.DLL dies earlier, in the
`_toolchain.resolve` gate.

### 1.6 Globals census

Every referenced address in the two back-end state sections — `.bssbe`
(rva `0x99000`, `0x66d4` bytes, zero-init) and `.databe` (rva `0xac000`,
`0x2470` bytes, initialized) — with per-site direction and width from the
instruction's pcode: `pcode-exact` (a ram varnode covers the target —
direct access, exact width), `pcode-approx` (indexed/indirect access —
direction from the analyzer's read/write bits, width from the LOAD/STORE
data size, i.e. the *element* width for arrays), `reftype` (address-taken;
width 0). Reader/writer functions are interval-attributed like the
anchors; `writer_tus` joins writers through the TU map.

## 2. The partition

2474 `.text` functions: **115 anchor** rows, **2359 bracket** rows (of the
brackets, 340 are ambiguous `a|b` gap rows and 1876 are `?`-edged). All 48
TUs are string-located; 46 have surviving function anchors after hygiene
(factor.c and switch.c share one carve interval — their sites at `0x93375`
and `0x93572` still pin them between stack.c and optimize.c).

The 48 TUs occupy the **upper part of `.text` only** (`~0x6a9fc`–`0x97361`),
strictly in link order with tight per-TU clusters. The front region
(`0x1000`–`~0x6a9fc`, ~1850 functions) carries no source-path string at
all — string-less TUs (codegen/MD tables, runtime) — and stays edge-labeled
`?|MDmisc.c`: honest, unpartitioned by this channel.

Per-TU extents over anchor+bracket rows (function entry to entry+size):

| tu | lo | hi | funcs | anchors |
|---|---|---|---|---|
| MDmisc.c | 0x6a9fc | 0x6bd1a | 21 | 7 |
| code.c | 0x6bfa3 | 0x6c62a | 12 | 4 |
| lower.c | 0x6c876 | 0x6d99e | 23 | 4 |
| lowerflt.c | 0x6e889 | 0x70112 | 9 | 5 |
| cgintrin.c | 0x70329 | 0x720f2 | 12 | 3 |
| ehgen.c | 0x724c8 | 0x72d16 | 4 | 2 |
| inasm.c | 0x72e66 | 0x74092 | 3 | 2 |
| mdlist.c | 0x74566 | 0x75598 | 12 | 3 |
| addr.c | 0x76ba8 | 0x76c1a | 2 | 2 |
| fppeeps.c | 0x772af | 0x77991 | 2 | 2 |
| schedmd.c | 0x7a7de | 0x7b34c | 6 | 3 |
| finlower.c | 0x7cf67 | 0x7d016 | 1 | 1 |
| dll.c | 0x7d32b | 0x7d404 | 1 | 1 |
| main.c | 0x7d5ae | 0x7d5c6 | 1 | 1 |
| emit.c | 0x7d9c2 | 0x7ddb9 | 4 | 4 |
| ide.c | 0x7e27b | 0x7e2aa | 1 | 1 |
| p2pragma.c | 0x7e2cf | 0x7e379 | 1 | 1 |
| tuple.c | 0x7e4a9 | 0x7e5b0 | 4 | 4 |
| getattr.c | 0x7f613 | 0x7f776 | 2 | 2 |
| misc.c | 0x7f7d2 | 0x7fa08 | 4 | 4 |
| hash.c | 0x800ed | 0x80102 | 1 | 1 |
| coff.c | 0x801ee | 0x80e14 | 3 | 3 |
| coffemit.c | 0x80ee1 | 0x82f0f | 24 | 7 |
| p2symtab.c | 0x8339f | 0x8406a | 8 | 4 |
| reader.c | 0x84d9b | 0x864ce | 17 | 5 |
| fg.c | 0x86c8a | 0x86cea | 1 | 1 |
| sdsu.c | 0x87418 | 0x875fb | 2 | 2 |
| except.c | 0x87e09 | 0x87e59 | 1 | 1 |
| ehexcept.c | 0x882fa | 0x8915e | 14 | 4 |
| regasg.c | 0x8b906 | 0x8bcda | 1 | 1 |
| list.c | 0x8cf95 | 0x8dd1a | 8 | 4 |
| dbcheck.c | 0x8e079 | 0x8e174 | 1 | 1 |
| color.c | 0x8e474 | 0x8ed08 | 8 | 3 |
| globdf.c | 0x8f5e3 | 0x8f7ce | 1 | 1 |
| globopt.c | 0x90643 | 0x9079f | 1 | 1 |
| globlopt.c | 0x90ce2 | 0x91a14 | 14 | 4 |
| dag.c | 0x91bbf | 0x91bdd | 2 | 2 |
| lg.c | 0x91d26 | 0x91d32 | 1 | 1 |
| sizeopt.c | 0x91d68 | 0x91dc4 | 1 | 1 |
| optimize.c | 0x923b7 | 0x92499 | 3 | 2 |
| stack.c | 0x93580 | 0x93c23 | 15 | 4 |
| inline.c | 0x94521 | 0x94573 | 1 | 1 |
| dlp.c | 0x95e9f | 0x95efb | 1 | 1 |
| ioin.c | 0x968d8 | 0x96be6 | 2 | 2 |
| error.c | 0x96c58 | 0x96c99 | 1 | 1 |
| getflags.c | 0x971d6 | 0x97214 | 1 | 1 |

These extents cover the anchored/bracketed rows only; each TU's true region
extends into the neighbouring `a|b` gap rows (see the TSV).

### 2.1 The key optimizer modules — anchor-confidence functions

| tu | anchored functions (rva, size) |
|---|---|
| inline.c | `0x94521` (82) |
| color.c | `0x8e474` (488), `0x8e877` (140), `0x8e9de` (810) |
| regasg.c | `0x8b906` (980) |
| reader.c | `0x84d9b` (291), `0x8502d` (1355), `0x8563c` (23), `0x85bd6` (642), `0x864bc` (18) |
| fg.c | `0x86c8a` (96) |
| lg.c | `0x91d26` (12) |

The single-function counts (inline.c, regasg.c, lg.c) reflect *anchored*
functions only — the module's remaining functions are the bracket/gap rows
around them. inline.c's gap-neighbourhood, for instance, spans
`stack.c|inline.c … inline.c … inline.c|dlp.c` ≈ `0x93c23`–`0x95e9f`.

## 3. The globals hunting ground

918 referenced addresses (496 in `.bssbe`, 422 in `.databe`) from 7151
reference sites. `writer_tus` grammar: `;`-joined TU labels of the writing
functions, where a label is either a plain TU (`color.c`), an ambiguous
gap bracket (`stack.c|inline.c`), or an edge label (`?|MDmisc.c` = the
string-less front region).

Top writers (candidate global compiler state):

| rva | section | size | readers | writers | note |
|---|---|---|---|---|---|
| 0xac360 | .databe | 2 | 28 | 97 | written by ~every TU — global error/pass state |
| 0xac354 | .databe | 4 | 3 | 95 | same shape — its sibling word |
| 0xae02c | .databe | 4 | 24 | 31 | list.c/mdlist.c/regasg.c — list/allocator state |
| 0x9f1a8 | .bssbe | 4 | 1 | 17 | optimize.c/stack.c neighbourhood |
| 0x9d670 | .bssbe | 4 | 23 | 16 | ehexcept.c/regasg.c neighbourhood |
| 0xae1d8 | .databe | 4 | 13 | 16 | globdf.c/globopt.c neighbourhood |
| 0x9903c | .bssbe | 4 | 12 | 10 | lowerflt.c |
| 0x99028 | .bssbe | 4 | 8 | 9 | code.c |
| 0xae1b8 | .databe | 4 | 13 | 8 | front region |
| 0x9d728 | .bssbe | 4 | 6 | 8 | list.c/mdlist.c/regasg.c |

Module-local candidates for the phase-3/4 targets:

* **inline.c (the /Ob2 budget hunt)**: the `.databe` cluster
  `0xac094`, `0xac098`, `0xac0a0`, `0xac0d4` (all written from the
  `stack.c|inline.c` gap) plus `0xae24c` and `.bssbe` `0x9f258` (written
  from `inline.c|dlp.c`). These are the dwords an inline-budget counter
  would live among.
* **color.c / regasg.c (allocator state)**: `0x9d864` (dbcheck.c|color.c),
  `0xac194`/`0xac198`, `0xae0ac` (color.c); `0x9d670`, `0xac384`/`0xac388`
  (regasg.c neighbourhood), `0x9d728`.

## 4. Determinism and re-verification

Every table is stably sorted before writing (functions and globals by rva,
anchor rows by (func, tu, string), TU sets serialized sorted), so a regen
against the same analyzed Ghidra project is byte-identical by construction;
the only header field that can differ across days is the provenance date.
The full end-to-end re-verify — two `--regen` runs from a fresh
`--reimport` diffed byte-for-byte — is a follow-up and has not been run
yet. Corroboration (§1.5) did run: 115/115, negative control passed.

Harness note for future maintainers: `GhidraProject` holds one open
transaction per program *by design*; persist with `GhidraProject.save()`,
never `DomainFile.save()` (the latter fails with "active transaction" —
that transaction is GhidraProject's own, not a leak).

## 5. Reference addresses

| what | binary | address (rva) |
|---|---|---|
| `_InvokeCompilerPass@12` export | C2.DLL | `0x68fd0` |
| `_AbortCompilerPass@4` export | C2.DLL | `0x7d4c1` |
| `.text` | C2.DLL | `0x1000`–`0x981fa` |
| `.bssbe` back-end zero-init state | C2.DLL | `0x99000`–`0x9f6d4` |
| `.rdata` (holds the 48 TU path strings) | C2.DLL | `0xa0000` (strings `0xa59f8`–`0xa9a04`) |
| `.databe` back-end init state | C2.DLL | `0xac000`–`0xae470` |
| string-less front region (no TU anchors) | C2.DLL | `0x1000`–`~0x6a9fc` |
| 48-TU anchored zone, link-ordered | C2.DLL | `~0x6a9fc`–`0x97361` |
| inline.c anchored function | C2.DLL | `0x94521` |
| color.c anchored functions | C2.DLL | `0x8e474`, `0x8e877`, `0x8e9de` |
| regasg.c anchored function | C2.DLL | `0x8b906` |
| reader.c anchored functions | C2.DLL | `0x84d9b`, `0x8502d`, `0x85bd6` |
| global error/pass state pair | C2.DLL | `0xac354`, `0xac360` |
| inline-budget hunting cluster | C2.DLL | `0xac094`–`0xac0d4`, `0xae24c`, `0x9f258` |

Generated by `scripts/homm3/vc6/atlas.py` +
`scripts/homm3/vc6/ghidra_scripts/{import_c2,tu_partition,globals_map}.py`;
tables in `evidence/vc6/c2-tu-map.tsv` and `evidence/vc6/c2-globals.tsv`.
