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

## Relocation-verified contributions

`config/retail/runtime-contributions.tsv` now places every linked runtime
section individually, and `homm3 verify library-code` re-proves each one on
every build (see [data matching](../tooling/data-matching.md)). The variant is
fixed by the bytes of those same sections, compared member by member:

| candidate archive | rows byte-equal (same member and symbol) |
|---|---|
| SP3 `lib/LIBCMT.LIB` | 407 / 407 |
| RTM `lib-rtm/LIBCMT.LIB` | 405 (`strftime.obj` `__Strftime`, `tzset.obj` `_cvtdate` differ) |
| single-threaded `LIBC.LIB` | 263 (90 differ in size, 54 absent) |
| debug `LIBCMTD.LIB` | 20 |
| `LIBCPMT.LIB` (SP3 = RTM for these rows) | 486 / 486 named rows |
| single-threaded `LIBCP.LIB` | 366 (113 differ, 152 absent) |

Retail therefore links the multithreaded release SP3 `LIBCMT.LIB` and
`LIBCPMT.LIB` (member paths `build\intel\mt_obj\...`). Identical copies in
several members are attributed by link-order continuity or by the static
symbol retail code references; ICF-folded COMDATs keep one row per folded
symbol that retail references. `basic_string<char>::_Tidy` (0x4040f0) is the
game-emitted COMDAT, not LIBCPMT's, and is claimed in `advmgr.cpp`.

Library data is placed the same way (`kind` data/bss/common rows): 1,044
contributions of LIBCMT, LIBCPMT and compiled zlib data, reached from verified
code relocations, data-to-data relocations, exact `.CRT$` pointer patterns and
member link-order windows. Every non-COMDAT section of each linked member is
placed. The spans are contiguous after the game (and zlib) contributions of
each output group: `.rdata` 0x244450-0x246c4c, `.rdata$r` 0x246c50-0x247d9c,
`.xdata$x` 0x25a110-0x25bd70, `.data` 0x28d77c-0x2911dc, `.bss`
0x2ab160-0x2aba7c, with the COMMON zone (library and game COMMONs
interleaved) up to 0x2ace60. Library COMDATs that a game object also emits
resolve to the game copy when they lie outside those spans (literals, float
constants, RTTI type descriptors, catchable types, `npos`, `_Psave` statics).
The data-only members pulled in are `crt0init`, `ctype`, `nlsdata1-3`,
`cmiscdat`, `days`, `timeset`, `constpow`, `_newmode`, `txtmode` and
`ncommode`; `iomanip.obj` is not linked (no `.CRT$XCU` entry).

The DirectPlay identifiers at `.rdata` 0x243d58-0x243e38 are Platform SDK
data, not game definitions: `GUID_NULL` is `UUID.LIB`'s `cguid_i_guid0.obj`
section, and the thirteen `DPAID_*`, `DPSPGUID_*`, `CLSID_DirectPlay*` and
`IID_IDirectPlay*` values are COMDATs of the pinned `lib/DXGUID.LIB`'s single
`dxguid.obj`, placed in that member's section order. `IID_IDirectPlay4A` and
`IID_IDirectPlayLobby3A` (DirectPlay 6) exist only in that library, not in
`lib-rtm/DXGUID.LIB`.

## Library identity from the Rich header

The Rich header lists the `@comp.id` of every object LINK read. Decoded, with
the pinned members' own `@comp.id` alongside:

| Rich entry (product/build, count) | Producer | Linked members that carry it |
|---|---|---|
| `Utc12_CPP` 8447, 145 | VC6 SP3 C++ | game C++ objects; the candidate link reads 136 (victor's VC5 objects carry no `@comp.id`) |
| `Utc12_C` 8168, 178 | VC6 RTM C compiler | LIBCMT/LIBCPMT C members and the 14 RTM-compiled zlib objects; the candidate link reads 175 |
| `Utc12_CPP` 8168, 26 | VC6 RTM C++ | 13 LIBCMT + 13 LIBCPMT C++ members: exact |
| `Masm613` 7299, 41 | MASM 6.13 | 41 LIBCMT assembler members: exact |
| `AliasObj60` 7291, 12 | OLDNAMES aliases | 12 `OLDNAMES.LIB` weak-alias members |
| `Linker512` 8034, 19 | import descriptors | 9 system DLLs x 2 + the null descriptor |
| `Linker512` 9049, 3 | import descriptors | DirectX 7 `DDRAW.LIB` |
| `Linker600` 8168, 4 | LIB 6.00 RTM | 4 objects that no pinned library contains (vendor import libraries are the likely source) |
| `Cvtres500` 1735, 1 | resource converter | the `.res`: the pinned CVTRES 5.00.1736 stamps `@comp.id` 6/1735 (VS6 RTM's is 5.00.1720) |

The SP3 service pack rebuilt only three LIBCMT members (`strftime`, `tzset`,
`undname`), and kept build 8168 for them. The Rich header therefore cannot
separate SP3 from RTM. The bytes can: `strftime` and `tzset` match only the SP3
`lib/LIBCMT.LIB`. LIBCPMT is identical in RTM and SP3. The libraries are:
`lib/LIBCMT.LIB` (SP3), `lib/LIBCPMT.LIB`, `lib/OLDNAMES.LIB`, `lib/UUID.LIB`,
`lib/DXGUID.LIB` (DirectX 7), and the system import libraries.

Two counts pointed to differences in the build inputs, not in code bytes;
the build now follows both:

- **zlib was compiled by the RTM C compiler.** The 14 zlib objects compiled by
  `orig/vc6-rtm` (C1/C2 12.00.8168) are section-for-section identical to the
  SP3-compiled objects, including relocations, and carry `@comp.id` 10/8168.
  Retail has no 10/8447 entry. The zlib units name `compiler = "msvc6-rtm"`
  (`homm3.init.vc6_rtm`) and are archived into `zlib.lib`.
- **The game used 12 old POSIX names.** Retail pulled 12 OLDNAMES aliases:
  `access chdir close getcwd open read strcmpi stricmp strnicmp strrev strupr
  write`, exactly the underscore names the game objects reference that have
  aliases. The source spells them without the underscore, and normalization
  binds an old name to its CRT function as LINK does.

What the candidate's Rich header still lacks, at the current link: nine
`Utc12_CPP` 8447 objects (game translation units the reconstruction does not
yet have: units whose retail code interleaves, such as the victor, rmg and
singleselection families, mark where one source file stands for several
original ones), three `Utc12_C` 8168 objects, and the vendor import
libraries' shapes (`Linker600` 8168 x4, the unmarked objects and the 270
short imports).

## Per-function runtime verdicts

`config/retail/runtime-functions.tsv` maps each of the 912 runtime-band census
functions to its library, member and covering COFF sections. All 912 are
`exact`; `homm3 verify generated-code` re-derives the table from the verified
contributions. 292 of them are Dinkumware template instantiations
(`wlocale` 154, `xlocale` 80, `wiostrea` 42, `locale` 7, `strstrea` 5,
`iostream` 4). Each is the copy explicitly instantiated in the LIBCPMT member,
and no game object emits it. LINK keeps the first definition, and game objects
come first, so any instantiation a game unit emitted would sit in that unit's
part of `.text`.

STL instantiations compiled into game units are different. They belong to the
game units and are counted in the game score. The library verification found
47 references from library sections that resolve to game-emitted COMDATs:

- code such as `basic_string<char>::~basic_string` (adventuremapwindow),
  `basic_streambuf<char>::sync`/`pbackfail` and the `num_put`/`num_get`
  deleting destructors (bottomviewsubwindow, objecttype), the
  `codecvt<char>` members (customcampaign), and the `out_of_range`/
  `runtime_error` members (advmgr, artifact);
- data such as RTTI and throw records, `npos`, `_Nullstr`, the literals the
  facets use and floating-point constants.

`homm3 verify library-code` lists them under `game_comdats`.

### The Complete editors' library gaps

The editors link the SP3 MFC overlay (`NAFXCW`) as well, so their library band
holds many identical bodies the census cannot tell apart by bytes: deleting
destructors, `GetRuntimeClass`/`GetMessageMap` stubs, one-byte `ret` folds.
`homm3 --image <editor> verify library-closure` proposes a reviewed row for
each census body no source claims between library neighbours, and `--write`
appends them to the image's `runtime-contributions.tsv`. A candidate section
must verify exactly under `homm3 verify library-code` together with every
reviewed row. Before that, names already in the image decide between identical
bodies: the runtime map, each slot of a library vtable at the address the
image's RTTI gives it, the pointers of the data a body references, and one
address per public symbol. Unplaced sections of the same member that a body
reaches (its EH handler stub, a local callee) are placed with it, as `alias`
rows where another owner already holds the bytes (an empty destructor folded
into a source-claimed `ret`). Rows that stay ambiguous, identical code of
several members, list the adjacent member's section and count the others in
their evidence; the rows are proved again as written, and a substitute that
fails in place reverts to the admitted section. A relocation-free body proves
no address by its bytes (any member's `33 c0 c3` matches), so it is admitted
only where the image names one of its symbols there, or where its member is
the library code beside the gap or beside the body.

The first pass closed 375 of the map editor's 378 gaps (7,231 B, 412 rows)
and 326 of the campaign editor's 333 (5,443 B, 365 rows). Reviewed section
starts also anchor census bodies (`_$E` initializer pairs and strchr.obj's
unlabelled first block had been merged into the body before them).

A source-claimed body that is also a reviewed `alias` row (an inline
constructor folded into `CString`'s bytes) bounds a gap as its library member
does. A reached public COMDAT already named at its target (a game unit's copy
of `logic_error`'s copy constructor) needs no row of its own. A candidate whose
member-local code target does not match the bytes at the address it reaches is
refuted at once: `wiostrea.obj`'s nine `_Winit` registrars before the two
`locale::id` ones, whose empty `$E` destructors fold into the shared `ret`.
These closed the map editor's last six gaps (181 B: `CArchiveException`'s
destructors, `length_error::_Doraise` and its copy constructor, `CTempDC`'s
deleting destructor, two registrars).

The census splits a library function at its catch continuations and jump
tables, so a gap can be the interior of a body the runtime map names but no
reviewed row places (`WinMainCRTStartup`, `CCmdTarget::OnEvent`). Such a gap
extends over that body, and the parent's section is offered from its start.
An external the body reaches that the runtime map names at its target, with a
single library definition, is placed with it. These closed three of the
campaign editor's last four gaps (1,474 B, 6 rows). The remaining
`CObject::GetRuntimeClass` at 0x46b4d6 is now proved against objcore.obj:
function descent recovers the class-pointer operand missed by linear
disassembly. The map editor's eight-byte virtual-call thunks at 0x4cd7ca
and 0x4cd7d2 are named independently by CDialog's native IDOK/IDCANCEL
message-map entries and match dlgcore.obj's retained thunk sections.

The function census also ignores pointer-looking words inside string
literals. Eight campaign entries were NOP padding reached only by the final
characters of import names, such as `ngA` in GetClassLongA. These are not
missing source functions. Catch continuation ownership keeps InitInstance's
aligned internal branch inside its parent when those false seeds disappear.
Pure atexit-registration helpers called from a CRT initializer inherit that
initializer's category, just as its tail-jumped registrar does; an arbitrary
constructor call still does not establish compiler-generated ownership.

A ninth campaign padding entry came from `ENC\0` in the CRT's language
table. Unlike longer strings, this three-letter code is not covered by the
literal heuristic. The reviewed 520-byte `getqloc.obj` `___rg_language`
section at RVA 0xc4f60 proves which words are fixed payload and which carry
COFF relocations. Only the former are excluded from function seeds and
relocation candidates; all fixed bytes must agree before applying the rule.

Template names also need the vtable identity check. Its former `\w+` match
silently skipped names such as `basic_streambuf<wchar_t>`, allowing the
campaign destructor at RVA 0x4e595 to be labeled as the narrow specialization.
Its stored vtable at 0x4916b0 proves the wide specialization. The narrow
77-byte destructor at 0x415930 is separately emitted by the recovered game
source. An admitted library alias does not take ownership away from an
already placed source COMDAT: such alias rows declare `ownership=source` in
`runtime-contributions.tsv`. The census reads that reviewed declaration, not
the generated placements, which are themselves derived from the census.
Regression tests cover both distinctions.

Conversely, a complete reviewed section chooses the retained identity when
an expanded base destructor has the same bytes. Map-editor 0x4f59f6 matches
both `logic_error` and `length_error` in string.obj, including their shared
base-vtable store. The reviewed section 45 and the `length_error` deleting
destructor establish the latter owner; the census must not replace it with
the former name, which the image already retains as a source COMDAT.

## Link readiness

`homm3 link` links the game on the retail link line with the pinned LINK
6.00.8447, using no `/FORCE`, with no unresolved or duplicate symbols
(`homm3.build.link`):

- **Flags**, read from the retail headers: LINK 6.00; `/SUBSYSTEM:WINDOWS`
  (4.0); `/BASE:0x400000`; default stack and heap; no `.reloc` (EXE default
  `/FIXED`); file alignment `0x1000`; no `/DEBUG`; `/OPT:REF` with COMDAT
  folding (the ICF-folded runtime COMDATs); no `/ENTRY`, so LINK uses the
  CRT's `WinMainCRTStartup`.
- **Default libraries.** The game objects are `/MT` (LIBCMT) and the zlib and
  victor objects `/ML`; `/NODEFAULTLIB:LIBC` leaves LIBCMT, LIBCPMT and
  OLDNAMES from the objects' directives.
- **Object order** (`homm3.build.link_order`): each unit's key is the lowest
  retail RVA of a function only its object defines (a NODUPLICATES COMDAT
  with a unique, unfolded retail address). Retail's order is the original
  source names' alphabetical order; `config/retail/link-order.tsv` places
  units without retail-placed code.
- **Libraries.** victor and zlib units (`library = ...` in
  `config/units.toml`) are archived and linked as libraries: retail places
  their members after every game object, in LINK's pull order. The library
  line is read from the import-descriptor order (VERSION, WINMM, mss32,
  smackw32, DDRAW, WSOCK32, then the VC6 AppWizard line from KERNEL32 to
  ole32, then binkw32, IFC20). LIBCMT's `delete.obj` at `0x20ab30` is
  byte-identical to LIBCPMT's `delop.obj`; the candidate pulls the latter,
  which puts all of LIBCPMT before LIBCMT as retail has it.

- **Resources.** `src/heroes3.rc` compiles with the pinned RC to payloads
  equal to retail's (`homm3.build.resources`); LINK converts the `.res` with
  the pinned CVTRES.
- **Clock.** LINK runs under libfaketime at retail's TimeDateStamp
  `0x39b83835`.

`homm3 verify link-diff` compares the candidate with retail and `homm3
build` gates on `config/link_diff.tsv` (a down-only ceiling per region).
The target is retail with the post-link edits of
`config/retail/post-link-edits.tsv` reverted to LINK's bytes. While
non-exact functions change size, every later address moves, so the code and
data regions compare each contribution at its own retail address, with
relocated fields compared through the placement of their targets. Only the
regions that measure the link's own inputs fail the build (`headers`, `rich`,
`imports`, `rsrc`); the code and data regions are banked and reported, and
each becomes a gate when it reaches 0. The plain file comparison is printed
as information.

`code-unplaced` is gated row by row: `config/link_unplaced.tsv` lists every
contribution without a retail identity (a unit's `_$E<n>` initializers share
one row) with its bytes and a reason, and a new, grown or unexplained row
fails the build. An ICF-folded copy takes the identity of its fold-mates:
which identical copy LINK keeps moves with unrelated inputs (the
`vector<HeroPlaceholderData>::insert` group was kept in game.obj, mapcell.obj
or sacrifice_window.obj at commits sampled on 2026-10-08..10), so the region rose
by 2,768 bytes at c64017ba7 without any placement changing.

Open items, in the order they block a byte-identical link:

- **Code sizes.** 125 functions still compile to a different size, so every
  later address differs; their own bytes are the matching backlog.
- **Object partition.** `code-order` breaks where retail places a header
  COMDAT in another object than ours (our earlier object emits a body the
  original did not) and where one of our units holds functions of several
  original objects. The RMG Voronoi run (0x5fceb0..), the river and road
  placement operations with their pattern tables (0x55ed70.., 0x55f2f0..)
  and the line walker (0x4f9f00.., with rmg_support's pattern table) are
  their own units since 2026-10-09. Two more interleavings: seerhuttext's 0x56c120/0x56c960
  around seerhut's `initializeSeerHutText` (0x56c3e0; Dreamcast places it in
  seerhut.cpp, so the two units are probably one object), and
  singleselectionwindow's 0x576e00/0x576e80 inside singleselectionpopups' run.
  The victor units follow retail's member pull order except the PCX kernels,
  which retail references before the lock cleanups and the bit helpers
  ([victor-library.md](../vc6/victor-library.md)).
- **zlib pull order.** `gzinflatebuf.cpp` is the Loki editor's `GzBuf.cpp`,
  which also defines `TGzDeflateBuf`. The game links that class
  unreferenced, so `/OPT:REF` drops its code while its references pull
  `deflate.obj` and `crc32.obj` ahead of `inflate.obj`, as retail does.
  Retail still pulls `compress.obj` before `uncompr.obj`: some object before
  lodfile referenced `compress2`.
- **No-CD code edits.** `setupCDDrive`'s source returns 7 in six bytes;
  retail keeps the original body behind the patched entry
  (`0x50c1c6..0x50c599`), so the original function is the source a
  byte-identical link needs, with the stated entry edit on top. The same
  holds for the CD edits in `earlySetup` and `readPrefsFromRegistry`, both
  non-exact today; `mciSendStringA` (WINMM) is referenced only from that body.
- **Vendor import libraries** (`homm3.build.import_libraries`). mss32,
  binkw32 and smackw32 are the `/IMPLIB` of a stub DLL whose `__stdcall`
  exports decorate to the retail names; IFC20's C++ exports are a `LIB
  /DEF`; filler exports put every name at its retail hint, and each library
  is checked against retail. LINK 6.00 RTM writes them
  (`config/retail/import-libraries.tsv`). Retail counts only two libraries'
  descriptor pairs as `Linker600` 8168; the producer of the other two (no
  `@comp.id`) is unknown, so mss32 and smackw32 are provisional.
- **IAT order.** LINK sorts each DLL's thunks with its C runtime's `qsort`.
  The game link runs LINK against the VC6 SP3 MSVCRT.DLL from the pinned SP3
  media (`[toolchain.linker_runtime]`); the remaining order differences
  follow the imports and reference order the reconstruction still lacks.
- **Rich header.** Retail counts 69 objects without `@comp.id`; the
  VC5-compiled Victor library members are most of them
  ([victor-library.md](../vc6/victor-library.md)). Nine `Utc12_CPP` 8447 and
  three `Utc12_C` 8168 objects are still missing; their code is either
  absent from retail (removed by `/OPT:REF`) or folded into other units.

## Import inventory

| dll | imports | note |
|---|---|---|
| `KeRNeL32.dll` | 105 | observed mixed-case spelling; producer and edit history unresolved |
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
