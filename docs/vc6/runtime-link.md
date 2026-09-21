# Linking the reconstructed game

After `homm3 build`, run:

```sh
homm3 link --strict --game-libraries --out build/exe/HEROES3.linked.EXE
```

The strict link includes all 152 objects and retains their COMDATs. It uses
`WinMainCRTStartup`, LIBCMT/LIBCPMT from the pinned VC6 SP3 toolchain, Windows
import libraries and generated vendor import libraries. It rejects unresolved
symbols, duplicate-symbol diagnostics, `/FORCE` overrides and linker timeouts.
Plain `homm3 link` remains a partial-image layout diagnostic.

The vendor libraries contain loader import records, not substitute implementations.
`llvm-dlltool` generates them from the pinned executable's named imports. Its short
import records must use `IMPORT_NAME`: `IMPORT_NAME_NOPREFIX` strips significant
leading underscores even when dlltool receives `--no-leading-underscore`. The
linked PE was checked against retail for all 13 Bink, 31 Miles, 11 Smacker and 26
IFC20 import identities. IFC20's error-handling flags are a data import.

## Runtime selection

Retail uses the multithreaded static CRT and C++ runtime; see
[executable libraries](../reference/executable-libraries.md). Four provisional
`/ML` profiles—kb, newgame, quickherowindow and quicktownwindow—emitted a retained
empty `std::_Lockit` destructor that collided with LIBCPMT's real lock release
at retail 0x60b634. Using `/MT` removes that conflicting definition and retains
real lock acquisition/release. Isolated profile probes left all 78 enrolled game
function scores unchanged; changes to emitted standard-library helpers are
expected. Subsequent source and data corrections are separate from this probe.

The zlib objects legitimately use `/ML`. Their four retail references at
0x6063f3, 0x6065e8, 0x60669c and 0x606768 address the global `errno` at 0x6ab15c;
that datum is defined at the project boundary in gzfile.cpp. LIBCMT's `_errno()`
continues to provide its separate thread-local state. Vendor sources stay pristine.

## Source and data validation

Missing storage was reconstructed in its owning source using typed initializers,
real string literals and relocatable references to project objects. Field views,
biased table bases and alternate names now reach their canonical storage, including
object traits, quest text, movement constants and creature animation data. Genuine
release-only empty methods are identified by their retail vtable/caller evidence.
There are no unresolved-symbol fallback bodies or pointers into the retail image.

VC6 object data checks compared 112 restored tables (82,559 bytes), resolving and
checking 3,267 string referents against the pinned image. These disposable checks
and extraction manifests live in ignored `build/`. The ordinary build refreshes
retail targets and runs the source inventory, ownership and address-claim gates.
Source-supported changes can lower individual matching scores; retained HIST
values preserve previous peaks. Relative to the starting checkout, 20 functions
have lower MAX values after the canonical declarations and data references were
restored; the largest is TCampaignBrief's destructor (100% to 62.74%, with its
previous peak retained in HIST). The implementation remains partially matched.

## Execution status

The strict linker completed with zero unresolved symbols and zero duplicate
warnings. The PE entry point was verified to be `WinMainCRTStartup`.

A launch in the isolated Wine prefix exits with loader status `c0000135`: the
runtime DLLs BINKW32.DLL, MSS32.DLL, SMACKW32.DLL and IFC20.dll are unavailable.
The repository does not distribute those DLLs or the game resources. No menu,
map, battle or gameplay execution has been verified. A successful link is not
proof that remaining reconstruction differences are safe at runtime.
