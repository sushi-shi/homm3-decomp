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
0x001000..0x204823  unattributed  5,837 fns  2,065,874 B   (presumably game code - see caveats)
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

## Coverage arithmetic

734 of 11,943 functions carry a direct per-function attribution (6.1% of
spanned bytes; library bands total 136,751 B). The remaining 2,116,762 B in
10,731 functions is **unattributed**: no channel places it in any known
archive. The head band plus the bulk of the funclet zone is *presumably* the
game's own code (NWC engine), but that is a residual inference, not evidence
— nothing here proves the head band contains no further unidentified static
library, and the band map deliberately says `unattributed`, never `game`.

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
  memberships (`vtable_rva#slot`, repeatable). **1,843 functions carry at
  least one relation**: 565 named, 734 library-proven, 1,080 are vtable-slot
  targets. The 988 vtable-slot targets that are still unnamed and non-library
  are the naming frontier (game virtuals NH3API addresses only mid-function,
  i.e. for a different pressing, or not at all).

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
| `nh3api` | 121 | NH3API wrapper name for that **entry** | external-candidate |
| `vtable-slot` | 311 | slot of a class-labeled vtable → `border__vslot05` | external-candidate |
| `eh-funclet` | 5,125 | FuncInfo unwind-action target (retail EH metadata) | structural |
| `caller` | 3,436 | named by dominant caller + callee ordinal (keeps related code lexically adjacent) | structural |
| `init-ctor` | 1,132 | `.CRT$XCU` slot, numbered in link order | structural |
| `band` | 698 | band prefix + address (fallback that cannot fail) | structural |
| `string` | 483 | an owned literal names the subject (`game_advopts_pcx_51d0`) | structural |
| `import-wrapper` | 193 | thunk to / lone caller of one imported API (`calls_timeGetTime_5e30`) | structural |

Totals by confidence: **443 retail-proven** (85,693 B), **433
external-candidate** (90,180 B), **11,067 structural** (2,077,640 B). Only the
first class asserts an original identity; structural names are stable labels
that carry their rva, so they are unique and unchanged across reruns.

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
