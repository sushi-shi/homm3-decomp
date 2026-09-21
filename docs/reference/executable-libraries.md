# Executable libraries and external reference provenance

The original discovery reports have been retired. Reviewed retail identities
live in `config/retail/runtime-map.tsv`, `config/retail/zlib-map.tsv` and source
annotations. The findings below describe the pinned executable; historical
attribution counts are not current matching coverage.

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

## Which executable NH3API describes (and how we used it anyway)

NH3API's embedded addresses **do not fit our pinned image**, and the record in
`AGENTS.md` ("873 of 874 land on x86 entry patterns") overstates it. Measured
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
The retired HD mapping pass took each NH3API-addressed function in the HD
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
