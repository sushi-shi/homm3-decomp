# The executable's DNA — static-library attribution

What `HEROES3.EXE` was linked against and where each library lives in `.text`.
Generated evidence: `config/retail-library-bands.tsv` (band map) and
`config/retail-function-libraries.tsv` (per-function attribution) — both
regenerable via `python3 -m homm3.carve dna`; raw channel outputs under
`build/dna/`. Everything below is against the pinned image
(`057c9d88…`, 2,732,032 B) and the carve inventory (11,943 functions,
2,253,513 spanned bytes).

## Verdict

| library | status | placement | evidence |
|---|---|---|---|
| **LIBCMT, SP3 servicing** | linked | `0x216e8d..0x227234` (60,667 B, 468 fns, 362 attributed) | masked byte identity vs the pinned `lib/LIBCMT.LIB`; 377 contributions byte-identical in RTM+SP3, **2 match SP3 only (`__Strftime`/strftime.obj, tzset.obj), 0 match RTM only** — the CRT is the SP3 one. Entry point 0x21a2b4 lies in this band. Stock Ghidra FID corroborates with 11 CRT-internal names (`__local_unwind2`, `__NLG_Notify1`, FP dispatch helpers) exactly here. |
| **LIBCPMT (std C++)** | linked | `0x20ab3b..0x216e8d` (49,783 B, 444 fns, 258 attributed) + its EH funclets at `0x238568..0x2392e0` | masked identity vs pinned `LIBCPMT.LIB` (locale/facet/basic_string/streams members); 26 tail slots of the `.CRT$XCU` init array target this band (facet static ctors, after all 1,119 game ctors — the link-order echo). |
| **zlib 1.1.3** | linked | `0x204830..0x20ab22` (24,027 B, 68 fns, 66 attributed) | masked identity vs our recompiled `vendor/zlib-1.1.3` objects: **all 14 members matched** (deflate 14 sections, trees 19, gzio 10, …); `deflate/inflate 1.1.3 Copyright` version literals at `.rdata` 0x244451/0x245381. |
| MFC (any form) | **absent, static and dynamic** | — | Three agreeing channels: (1) masked identity — 4,139 NAFXCW code sections swept, 0 unique-member hits, the only 2 landings are 8-byte generic EH stubs also present in other archives (`-island`, no band); MFCS42 0 of 18; (2) bytes — zero `Afx`/`AFX`/`NAFXCW`/`MFC42` sequences anywhere in the file; (3) imports — no `MFC42.DLL`, and the game runs a raw Win32 message pump (`GetMessageA`/`TranslateMessage`/`DispatchMessageA` imported directly, which an MFC app routes through `CWinApp`). attempt-1 hedged here ("25 NAFXCW-family candidates, none unambiguous"); the resolution is that MFC statically embeds CRT/C++-runtime code, so an MFC-built FID collides with LIBCMT/LIBCPMT members on exactly the shared functions — its masked-FID oracle shows 5,174 NAFXCW records, **every one `ambiguous`**. FID said "maybe MFC"; imports+strings+unique-bytes say no MFC; the collision explains the disagreement. |
| LIBCIMT (old iostream) | **not linked** | — | its only 4 landings are byte-identical *shared* members with LIBCPMT (`shared(cxx-libcpmt+iostream-libcimt)` rows); zero LIBCIMT-only hits. (attempt-1's exact FID: zero libcimt records.) |
| MSVCRT.DLL | **not imported** | — | no `MSVCRT`/`MSVCP` import descriptor or byte sequence anywhere: the CRT is fully static, and LIBCMT's own `R6002`/`R6008`/`runtime error` strings are in `.rdata`. |
| Middleware | **all dynamic — zero `.text` bytes** | import table only | see the import inventory below. |

## The band map

```
0x001000..0x204823  game          5,837 fns  2,065,874 B   5,790 positively evidenced - see below
0x204830..0x20ab22  zlib             68 fns     24,027 B
0x20ab30..0x20ab3b  unattributed      1 fn          11 B   (operator delete - generic bytes, ambiguous)
0x20ab3b..0x216e8d  cxx-libcpmt     444 fns     49,783 B
0x216e8d..0x227234  crt-libcmt      468 fns     60,667 B   (entry point 0x21a2b4)
0x227240..0x23855c  unattributed  4,841 fns     50,436 B   EH-funclet zone (share 1.00)
0x238568..0x2392e0  cxx-libcpmt     232 fns      2,274 B   EH-funclet zone (share 1.00)
0x2392e0..0x2395f0  unattributed     52 fns        441 B   EH-funclet zone (share 1.00)
```

Two structural laws fall out of the measurements:

1. **Linkage order is preserved.** Game objects, then zlib, then LIBCPMT, then
   LIBCMT — one contiguous band each, at object/archive-member granularity.
   The `.CRT$XCU` initializer array replays the same order: 1,119 game-ctor
   slots, then 26 LIBCPMT slots, nothing else.
2. **The tail of `.text` is the `.text$x` group.** COFF `$`-suffix sorting
   places every EH unwind-funclet contribution after all `.text$mn` code: the
   last three bands hold **exactly** the 5,125 unwind-action funclets the EH
   audit walks (4,841 + 232 + 52), and the code bands hold zero. The
   "second LIBCPMT cluster" is not a second archive pull — it is LIBCPMT's
   own funclets, sorted into the `$x` group in the same link order.

## Method (independent channels; disagreements reported, not arbitrated)

- **Masked archive identity** (primary): every code section of every member
  of the pinned VC6 archives, relocation dwords masked, remaining fixed bytes
  searched in `.text` (exact; ≥10 fixed bytes, ≥5-byte anchor, >4 image hits
  discarded as generic). The archives are the very linker inputs, so a linked
  member's bytes are in the image by construction.
- **Recompiled zlib identity**: same matcher over our own
  `/O2 /ML /Gr /TC` builds of `vendor/zlib-1.1.3` — weaker in principle
  (recompiled, not pinned input), but 66/68 band functions matched.
- **Stock Ghidra FID** (`vsOlder_x86`, already run during carve S2): 49
  verdicts, all inside the LIBCMT/LIBCPMT bands — agrees with the masked
  channel everywhere both speak; adds 11 CRT asm helpers the masked channel
  could not place (odd boundaries).
- **Strings**: CRT banner block (`R6002…`, "Microsoft Visual C++ Runtime
  Library", referenced from 0x22271b/0x225caa in the CRT band); zlib version
  literals. MFC markers: absent. A note on a tempting false negative:
  `invalid distance too far back` is NOT in the image — and not in zlib
  1.1.3 either (it is a zlib 1.2.x message); 1.1.3's actual string
  `invalid distance code` (`vendor/zlib-1.1.3/inffast.c:127`) IS present,
  as is `invalid literal/length code`.
- **Imports**: 14 descriptors, 274 imported symbols; middleware is dynamic
  and contributes zero bytes to `.text`.

## Import inventory

| dll | imports | note |
|---|---|---|
| `KeRNeL32.dll` | 105 | mixed-case name as linked — an import-library fingerprint worth preserving |
| `USER32.dll` | 56 | includes the raw message pump (`GetMessageA`, `TranslateMessage`, `DispatchMessageA`) |
| `mss32.dll` | 31 | Miles Sound System (vendored SDK: `vendor/miles-5.0e`) |
| `IFC20.dll` | 26 | Immersion force-feedback (CImmMouse/CImmProject/CImmEffect…), all MSVC-mangled C++ |
| `binkw32.dll` | 13 | Bink video (`vendor/bink-0.5a`) |
| `smackw32.dll` | 11 | Smacker video (`vendor/smacker-3.2h`) |
| `WSOCK32.dll` | 10 | imported **by ordinal only** — resolved below |
| `GDI32.dll` 6, `ADVAPI32.dll` 5, `WINMM.dll` 4, `VERSION.dll` 3, `ole32.dll` 2, `SHELL32.dll` 1, `DDRAW.dll` 1 | | DDRAW imports only `DirectDrawCreate`; the rest of DirectDraw goes through COM vtables |

WSOCK32 ordinals, resolved against the pinned toolchain's own
`msvc/lib/WSOCK32.LIB` import members (never guessed):
`2=_bind@12, 3=_closesocket@4, 8=_htonl@4, 9=_htons@4, 11=_inet_ntoa@4,
12=_ioctlsocket@12, 23=_socket@12, 52=_gethostbyname@4, 57=_gethostname@8,
115=_WSAStartup@8`.

The 26 `IFC20.dll` imports double as a validation of the vendored IFC SDK:
their MSVC mangling encodes access and virtualness, and it agrees exactly
with `vendor/ifc-2.0.3/include/ImmMouse.h` — `?reset@CImmMouse@@MAEXXZ` and
`?prepare_device@CImmMouse@@MAEHXZ` are `M` (protected virtual), declared
under `protected:`; `?SwitchToAbsoluteMode@…@@UAEHH@Z` and
`?ChangeScreenResolution@…@@UAEHHKK@Z` are `U` (public virtual), declared
under `public:`. Retail-byte proof that the vendored IFC headers are the
right SDK generation — and the same technique validates any mangled import
surface.

## The game band by positive evidence

The head band is no longer "presumably game": **5,790 of its 5,837 functions
(2,061,859 B, 99.2%) carry a positive game attribution**, from five channels
layered strongest-first (library evidence always wins; every channel is cut
at the first library band, since linkage order makes game contributions a
contiguous prefix of `.text`):

| evidence | fns | bytes | what it is |
|---|---:|---:|---|
| `hd-crossbuild-name` | 837 | 282,757 | NH3API name carried onto our bytes by unique masked identity vs HD Mod's sibling build (crossbuild-verified) |
| `nh3api-name` | 31 | 4,243 | NH3API address that directly hits one of our entries |
| `init-array-ctor` | 1,106 | 93,448 | the `.CRT$XCU` game-ctor prefix, in link order (retail-derived) |
| `xref-to-game-direct` | 1,186 | 816,341 | shares a call edge with a NAMED game function (`E8` rel32 calls are retail bytes; Ghidra call-graph export) |
| `xref-to-game-transitive` | 2,001 | 745,620 | reached from game code through other closure members only |
| `game-vtable-link` | 629 | 119,450 | tied to game code across a retail vtable: the vptr-store site (ctor/dtor) and the slot targets belong to one class |

Closure rules, all enforced in `homm3.carve.dna:game_channel`: it never
enters an EH funclet (calls FROM a funclet fold into the function the
funclet guards, via retail EH parentage — that is how a dtor reached only on
exception paths still ties to its owner, +29 functions); it never propagates
*through* a library-attributed function (game code calling `memcpy` must not
paint memcpy's other callers); and nothing past the first library band can
become game no matter what calls it (the generic `operator delete` sliver at
0x20ab30 is the case that forced the guard — game code calls it, linkage
order vetoes it).

**What remains: 45 functions, 3,962 B** (plus 2 generic 8-byte island
stubs) with *no* positive evidence — 37 of them have zero Ghidra callers,
median size ~35 B: unreferenced helpers, dead code, or carve noise. They are
the entire unknown of the head band.

## Coverage arithmetic

6,524 of 11,943 functions carry a per-function attribution: 5,790 game
(2,061,859 B) + 734 library (library bands total 136,751 B). Outside those,
the EH-funclet zone (5,125 tiny funclets, 53,151 B) is structurally owned by
its parents (each funclet is named after the function it guards — see
`retail-symbols.csv`), leaving the 45-function residue above and the two
slivers as the only genuinely unattributed code. The `game` label is still
positive-evidence-per-function, not proof of the band as a whole — but the
residual-inference caveat that used to live here has been retired by
measurement.

## Candidates and known noise (not claims)

- **Header-template islands**: `std::locale` dtor etc. at 0x53c80/0x53d70 —
  byte-identical to LIBCPMT members but deep inside the head band; these are
  game-TU COMDATs instantiated from headers, kept as `-island` candidates,
  never band founders. Six more 8-byte `-island` funclet stubs likewise.
- **Shared members**: 8 functions whose bytes exist identically in two
  archives (`shared(crt-libcmt+cxx-libcpmt)` operator-delete family, and the
  LIBCPMT/LIBCIMT stream funclets) — real library code, member undecidable.
- **Non-entry matches (33 rows in `build/dna/archive_matches.tsv`)**: archive
  sections matching at addresses our carve does NOT list as entries —
  `_memcmp` (0x21aae0), `__allmul` (0x21e7f0), `87triga` (0x2221b0),
  `___init_time/numeric/monetary`, `__fcloseall`, `_iswalpha`, … These are
  symbol-level identifications of functions sitting in the carve's residue
  gaps (they corroborate `build/carve/gap_candidates.tsv` independently) and
  are the natural next hand-admissions into `config/retail-functions.tsv`.
- **zlib stragglers**: 2 of 68 band functions unmatched at entry granularity
  and `@zcfree@8` matching at non-entry 0x206c90 — boundary or codegen
  discrepancies between our recompile and retail; worth revisiting when zlib
  matching starts in earnest.

## Relating names to functions and vtables

`python3 -m homm3.carve relate` joins the three carve deliverables for the
pinned image only (no Dreamcast/HD address ever enters — those are other
pressings) into two generated CSVs:

- **`config/retail-function-symbols.csv`** — one row per function in
  `retail-functions.tsv` (11,943), carrying: its entry name/signature where we
  have one (`retail-function-names.csv`, **entry rows only**), the library +
  retail-proven symbol where the DNA pass placed it, and its vtable-slot
  memberships (`vtable_rva#slot`, repeatable). After the game channel:
  **6,601 functions carry at least one relation** — 565 named, 6,524
  band-attributed (734 library + 5,790 game), 1,080 are vtable-slot targets.
  The vtable-slot targets that are still unnamed are the naming frontier
  (game virtuals NH3API addresses only mid-function, i.e. for a different
  pressing, or not at all).

- **`config/retail-vtable-symbols.csv`** — one row per vtable slot (363
  vtables, 3,040 slots): the slot's target function, that function's method
  name where known, and the owning **class** where NH3API's
  `NH3API_SPECIALIZE_TYPE_VFTABLE(addr, class)` lands on or inside the vtable.
  **86 vtables are class-labeled**; the class channel is unverified but
  checkable against our retail-derived vtable starts — 54 labels land exactly
  on a start (`class_addr_offset 0`), 31 land 4–16 bytes in (our cut evidence,
  a ctor's vptr store, begins the piece a few slots earlier: recorded as the
  offset, never silently), 8 miss. The slot→function→method topology is fully
  retail-derived; only the class *string* is a candidate. Worked example that
  validates the whole chain: the `exe_strstreambuf` vtable at 0x245670 has
  slots resolving to `?overflow@strstreambuf@std@@MAEHH@Z`,
  `?underflow@…@std@@MAEHXZ` — **retail-byte-proven LIBCPMT symbols** from the
  masked channel, in vtable order, under the class NH3API independently names.

Both CSVs are GENERATED (regenerate, don't hand-edit) and stay candidates
until a supervised admission; the source carcass that consumes them is a
later stage.

## Naming every function

`python3 -m homm3.carve naming` → **`config/retail-symbols.csv`**: one row per
carved function, **11,943 of 11,943 named, all unique, all valid C
identifiers** (asserted, not hoped — the stage fails on any collision, gap, or
malformed name). A name is a working label; the `tier`/`confidence` columns
say what kind of evidence produced it.

| tier | count | what it is | confidence |
|---|---:|---|---|
| `library-symbol` | 433 | masked-archive/zlib byte identity → the real linker symbol, VC6 mangling decoded (`?clear@ios_base@std@@…` → `std_ios_base_clear`) | retail-proven |
| `fid` | 11 | stock Ghidra Function ID (`local_unwind2`, `NLG_Notify1`) | retail-proven |
| `hd-crossbuild` | 846 | **original class/method name** transferred from HD Mod's sibling build by unique masked byte identity (`TAdventureMapWindow_ProcessRightSelect`) | crossbuild-verified |
| `nh3api` | 31 | NH3API address that happens to hit one of our entries directly | external-candidate |
| `dc-linkorder` | 145 | **original name + source FILE:LINE** from the Dreamcast CodeView dump, transferred by link order alone (see below) | linkorder-candidate |
| `vtable-slot` | 303 | slot of a class-labeled vtable → `border__vslot05` | external-candidate |
| `eh-funclet` | 5,125 | unwind funclet named after the **parent function it guards** (`TAdventureMapWindow_TAdventureMapWindow_unwind03`) | structural |
| `caller` | 2,656 | named by dominant caller + callee ordinal | structural |
| `init-ctor` | 1,120 | `.CRT$XCU` slot, numbered in link order | structural |
| `band` | 695 | band prefix + address (fallback that cannot fail) | structural |
| `string` | 427 | an owned literal names the subject (`game_advopts_pcx_51d0`) | structural |
| `import-wrapper` | 151 | thunk to / lone caller of one imported API | structural |

**8,592 of 11,943 (72%)** carry a semantic name — an original symbol, a class
method, or a parent-anchored EH funclet — rather than an address label. By
confidence: **443 retail-proven** (85,669 B), **846 crossbuild-verified**
(283,051 B), **335 external-candidate** (63,120 B), **145 linkorder-candidate**
(54,913 B), **10,174 structural** (1,766,760 B).

The two channels that turned addresses into meaning:

- **EH funclets (5,125 — 43% of all functions).** VC6 loads a FuncInfo inside
  a tiny `__ehhandler$` stub (`mov eax, <FuncInfo>; jmp __CxxFrameHandler`)
  that the linker parks in the `.text$x` tail, outside every carved function.
  So parentage resolves in two hops — FuncInfo ← stub ← the function that
  pushes the stub as its SEH handler — and **every one of the 5,125 funclets
  resolved to a parent, zero orphans**. A funclet now reads
  `<parent>_unwind03`.
- **HD Mod cross-build transfer (782).** See below.

Evidence channels feeding this, beyond the earlier passes: a Ghidra export
(`ghidra/export_xrefs.py` → `build/dna/function_xrefs.tsv`) supplying the call
graph — relative `E8` calls carry no relocation, so the reloc sweep cannot see
them — plus thunk identity, reached imports, and literals sniffed from memory
rather than trusted to be defined data (4,574 functions have known callers,
584 own literals, 469 reach an import, 46 are thunks).

Two traps worth recording, both caught and fixed here: Ghidra's placeholder
`thunk_FUN_…` names are not API names (they were leaking into
`import-wrapper`), and an `-island` byte-identity — a header-template COMDAT
instantiated in a game TU, e.g. `std_locale_dtor` at 0x53d70 — proves the
*bytes* but not the library *attribution*, so it is demoted to
external-candidate rather than claimed as a LIBCPMT contribution.

## Which executable NH3API describes (and how we used it anyway)

NH3API's embedded addresses **do not fit our pinned image**, and the record in
`CLAUDE.md` ("873 of 874 land on x86 entry patterns") overstates it. Measured
here against the carve:

| test | our exe | HD Mod's `Heroes3.exe` |
|---|---:|---:|
| NH3API call-macro addresses that are `E8` call targets | **102 / 920 (11.1%)** | **904 / 920 (98.3%)** |
| land exactly on a carved function entry | 121 / 920 | — |

11.1% is chance level for 16-aligned addresses in this image, and probes land
mid-instruction — e.g. NH3API's `get_black_box` at `0x405D70` is the **last
byte of `cmp ecx, 8`** in our bytes. The offsets to the nearest entry spread
smoothly (±16/32/48…), so there is no constant shift either.

NH3API's README says why: it targets "the Complete edition **with HD Mod by
baratorch**", built on an IDA database of *that* executable. HD Mod's
installer ships its own `app/_HD3_Data/Heroes3.exe` — 2,826,240 B,
`sha256 60f0df04…` — a **sibling build**: identical `.text` virtual size
(0x239000) but bytes diverging from `.text+0x25`, so a different compilation
of the same sources, not our image patched.

That sibling relationship is exactly what makes the names recoverable.
`python3 -m homm3.carve hdmap` takes each NH3API-addressed function in the HD
build, masks what layout changes between builds (in-image absolute operands
and `E8`/`E9`/`0F 8x` rel32 displacements), and searches the remaining
instruction skeleton in **our** `.text`, in two passes:

- **Pass 1 — global unique identity.** A shrinking match window (down to the
  first diverging neighbour) accepted only a globally unique masked hit:
  **846 of 915**.
- **Order gate.** The builds preserve link order — the pass-1 map is 845/846
  monotonic in (HD rva → our rva), median neighbour-gap difference 0 bytes —
  so the one pair that broke monotonicity was a byte-twin false match and was
  demoted.
- **Pass 2 — bracketed.** Each still-unresolved address was retried only
  *between its resolved neighbours*, where a handful of fixed bytes uniquely
  place it: **+41**, each required unique-in-bracket and monotonicity-
  preserving (asserted).

**886 transferred, 884 onto function entries our carve had already found
independently** (29 unresolved: no unique in-bracket match). The two that did
*not* land on a carved entry are findings, not errors: `0x1ff500`
(`heroWindow::HeroWindowHandler`) is a 12-byte function our carve missed but
attempt-1 also has — an independent carve-gap rediscovery; `0x1bbaaf`
(`Bitmap16Bit::~Bitmap16Bit`) is an adjustor-destructor tail folded into our
0x1bba70. Both keep `our_state` so the namer excludes them.

So the name is NH3API's (external, unverified), but the **identification is
our own bytes** — hence the distinct `crossbuild-verified` confidence class,
above external-candidate and below retail-proven. That the transfer and the
carve agree on 884 entries is mutual corroboration of both.

Output: `config/retail-hd-name-map.csv` (`rva, hd_va, name, signature,
match_bytes, fixed_bytes, pass, our_state, evidence`). The HD executable is the
user's own download, referenced via `$HOMM3_HD_EXE` and never copied into the
repository.

## The Dreamcast dump by link order (`carve dcmap`)

The Dreamcast port's CodeView dump proves names, signatures, and source
FILE:LINE for 9,684 procs — of **another pressing** (RoE sources, SH4 code),
so no byte or address evidence can cross. What crosses is **link order**:
of 588 name pairs unique on both sides (our hd-crossbuild/nh3api-named
entries vs Dreamcast procs), **95.1% are monotone** in (our rva → DC
offset). Both projects evidently emitted their objects in the same order —
the third build to replay it, after the retail/HD pair. The 29 off-backbone
pairs are tiny COMDAT-style dtors (`CAdvPopup::~CAdvPopup`) that genuinely
migrate between builds; LIS demotes them.

`python3 -m homm3.carve dcmap` turns that into a transfer channel
(`config/retail-dc-name-map.csv`):

- **Pass 1**: 588 unique-name anchors, LIS-gated → 559.
- **Pass 2**: globally ambiguous names (overloads, repeated dtor names)
  re-anchored where unique *inside* their bracket, monotonicity re-asserted
  (the hdmap idea, keyed on names instead of bytes): **+40** → 599 anchors.
- **Pass 3 — incremental rva**: between two adjacent anchors, if our carve
  holds exactly as many entries as the dump holds procs, the bracket aligns
  1:1 in address order. An **agreement gate** rejects any bracket where an
  already-named entry would align against a different name — 16 brackets
  died there, 243 failed the count outright (Complete's insertions land
  here by design, failing loudly instead of shifting names silently), and
  **51 brackets survived**: **145 NEW names** (`advManager::CompleteDraw`,
  `type_AI_player::calculate_demand`, `SaveGame`, …, each with
  `E:\gamedcs\*.cpp:line` provenance) plus 10 alignments that corroborate
  existing HD names.

The internal consistency is visible in the output: consecutive rvas map to
consecutive line numbers of the same source file. Still, the names ride on
order alone, from a build of older sources — hence the dedicated
`linkorder-candidate` confidence class, below `external-candidate`, and the
`dc-linkorder` naming tier. The mismatched brackets are the channel's
remaining headroom: every new rva-anchored name (NH3API or future retail
proof) splits brackets and can only grow the monotone backbone.

## Caveats

- The masked matcher proves presence, not absence: a library built into the
  image with different compiler switches than its archive would evade it.
  The MFC negative therefore rests on three agreeing channels (0 unique
  byte hits, 0 strings, attempt-1's all-ambiguous oracle), not on one.
- Ambiguity is capped (>4 image hits = generic), so tiny common thunks are
  systematically unattributed — that is why band interiors show attributed
  counts below function counts.
- The funclet zone's per-parent ownership (which funclet belongs to which
  band's FuncInfo) is measured only in aggregate here.
- The game closure's *seeds* are external names (NH3API/HD); the *edges* are
  retail bytes (rel32 calls, vtable slots, EH parentage) as read by Ghidra's
  analysis. A misdecoded call site could add a false edge — which is why the
  closure stays inside the head band and why the confidence class remains
  `external-candidate`, never retail-proven.
