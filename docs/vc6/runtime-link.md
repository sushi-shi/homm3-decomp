# Linking the reconstructed game

After `homm3 build`, run:

```sh
homm3 link --out build/exe/HEROES3.linked.EXE
```

The default link includes all 152 objects and retains their COMDATs. It uses
`WinMainCRTStartup`, LIBCMT/LIBCPMT from the pinned VC6 SP3 toolchain, Windows
import libraries and generated vendor import libraries. It rejects unresolved
symbols, duplicate-symbol diagnostics, `/FORCE` overrides and linker timeouts.
No opt-in flag is needed for these checks or the game libraries.

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
The ledger separates CUR (the latest compile) from MAX (the best score for the
current function source hash) and HIST (the peak across source revisions).
Changing a function's source hash resets MAX to CUR, even if its current score
is unchanged. That explains all seven formerly MAX-exact entries in this PR:

| Function | Parent CUR | PR CUR | Parent MAX → PR MAX |
| --- | ---: | ---: | ---: |
| `advManager::main` | 99.7919% | 99.7919% | 100% → 99.7919% |
| `TCampaignBrief::~TCampaignBrief` | 62.7417% | 62.7417% | 100% → 62.7417% |
| `combatManager::drawFrame` | 98.0274% | 98.0274% | 100% → 98.0274% |
| `TRmgTreasureGroup::canFitObject` | 87.4868% | 87.4868% | 100% → 87.4868% |
| `type_belong_to_player_quest::setDefaultText` | 87.4421% | 87.4421% | 100% → 87.4421% |
| `type_resource_quest::setDefaultText` | 91.2333% | 91.2333% | 100% → 91.2333% |
| `TSingleSelectionWindow::~TSingleSelectionWindow` | 86.6265% | 86.6265% | 100% → 86.6265% |

The comparison is commit `66275fba` against its parent `e4b68c84`. These seven
current scores did not worsen; their source hashes changed when declarations,
names and data access were corrected. HIST retains the previous exact peaks.
Across all 20 MAX decreases, 18 have unchanged CUR, one has lower CUR
(`armyGroup::getMoraleDescription`) and one has slightly higher CUR
(`type_monster_quest::setDefaultText`).

Two existing functions gained MAX 100% (`army::doAttack` and
`swapManager::setRolloverText`). The two new inventory entries are retail's
campaign difficulty arrow callbacks at 0x457f70 and 0x457fc0, both 73 bytes and
both exact. They replace the unresolved shared callback declaration, which
incorrectly identified the copy-assignment body at 0x457cb0 as a button handler.
Thus exact MAX is 4,244 − 7 + 2 + 2 = 4,241, and the function count grows from
4,766 to 4,768. No gameplay functions were invented to increase the count.

CUR is a separate result: ten existing functions lost current exact status,
three existing functions gained it, and the two new callbacks are exact, giving
4,182 − 10 + 3 + 2 = 4,177. The ten losses occur in quest description/log builders
after the typed quest-text accessor changes. Their previous MAX peaks remain
where their own source hashes are unchanged. The implementation remains
partially matched; the successful link does not establish byte identity.

## Shared storage and names

The naming audit reduced `g_unnamed*` / `g_unk*` globals from 83 to one. It
also removed address suffixes from named globals. Original names come from
positive Dreamcast references where available, including `gConfig`, `InitWin`,
`gbRemoteOn`, `currentLoop` and `waitingLoop`; other names describe proven
retail roles. The sole remaining placeholder, `g_unnamed6968e8`, is a
retail-only byte cleared by `animateMove`. No reader or original name has been
established, so its role remains unknown.

Four former screen-geometry globals were fields of `InitWin`, and fourteen
other definitions duplicated fields of `gConfig`. The consumers now access
the canonical objects. This matters at runtime: DirectDraw surface updates and
preference changes must be visible to the window manager, sound and UI code.
The trading dialogs retain their own separate window coordinates.

`widget::onSleepChange` describes the retail-only virtual slot 12; its empty
out-of-line definition and button's qualified base call remain intact. Removing
the redundant `emptyText` local keeps `CHeroWindowEx::processHover` exact, as do
the corrected `heroWindowManager::open` and renamed button override.

Relative to the linked implementation above, this cleanup changes exact MAX
from 4,241 to 4,237. Three losses reset unchanged lower CUR scores after a
source rename (`processDeSelect`, `processSelect`, `updateMouseGrid`). The
system-options constructor has a new CUR/MAX decrease from 100% to 99.3013%
after accessing the shared preferences object. `screenScroll` and `earlySetup`
also reset already-inexact MAX to unchanged CUR. HIST retains every prior peak.
Exact CUR is 4,176, and executable MAX is 96.87%.

## Execution status

The linker completed with zero unresolved symbols and zero duplicate
warnings. The PE entry point was verified to be `WinMainCRTStartup`.

The first isolated Wine launch lacked BINKW32.DLL, MSS32.DLL, SMACKW32.DLL
and IFC20.dll. Both local Steam installations provide identical 32-bit copies
with all 81 required exports. A disposable copy of the standard installation's
libraries and game data lets the reconstructed executable load them successfully.
The repository does not distribute those files.

That launch exposed a missing CRT initializer, not a missing DLL: the global
network-player record read through the zero-initialized `g_videoGameState`.
Retail also starts its cell at 0x69923c as zero, but initializer 0x4eccf0 binds
it to the initialized dword 3 at 0x67f554. The CRT table lists this initializer
at 0x65e7d4, before the player-record initializer 0x552290 at 0x65ebf0.

`gamecontext.cpp` now owns that storage and binding as `g_installedGameContext`
and `g_gameContext`. A mutable reference to the initialized dword reproduces
the 11-byte initializer; a literal or const scalar makes VC6 introduce an
extra temporary/value store. The reference preserves the former mutable
pointee contract. Existing consumers retain their instruction bytes. Both
startup initializers are enrolled explicitly and match retail, including
independently resolved relocation targets/addends; the backing dword also
matches. The real link places the binding before the player-record initializer.

This illustrates why initial data bytes and function scores alone are
insufficient: the missing binding cell was correctly zero-filled, and the
consumer's instructions were correct, while the required initializer was
unenrolled and absent. Data validation must cover backing values, reference
identity, generated startup code and its execution order together.

The retry exposed two more missing initializers in the resource manager:

- `ResourceManager::open` dereferenced the zero archive-index pointer in
  `g_resourceArchiveContexts[3].m_sprites`. Retail 0x559320 builds four 24-byte
  contexts from twelve read-only archive-index arrays. Reconstructing the
  descriptor constructor and explicit array temporaries reproduces all 209
  initializer bytes, including 16 resolved relocations. The twelve lists
  occupy 100 bytes at 0x641028..0x64108b and independently match retail.
- With archive selection working, `ResourceManager::getSoundFile` dereferenced
  a zero count pointer in `g_soundHeaderDescriptors`. Retail 0x5592b0 binds three
  descriptors to the live sound-header, count and file-handle globals. The
  reconstructed descriptor constructor and array initialization reproduce all
  97 bytes, including 18 resolved relocations. They refer to the existing
  globals rather than copies, so LoadSoundHeaders updates reach the readers.

The next `/i0` launch reached the main menu, but general-RLE sprites were
corrupt: buttons and dialog borders read a zero literal-run marker. Retail
0x47c260 initializes that marker at 0x6968a6 to 255; 0x47c270 initializes its
companion maximum run length at 0x6968b0 to 256. Dreamcast identifies the
file-static constants as `kGeneralRLEOpaqueRunCode` (`const unsigned char`)
and `kGeneralRLEMaxRunLength` (`const unsigned int`). Its initializer line
rows at cspriteframe.cpp:46/47 (dc 0x745b0/0x745d8) call the unsigned-char
numeric limit, with an added one for the maximum length. Restoring those
expressions reproduces both retail initializer bodies (8 and 11 bytes),
including their destination relocations. The CRT entries at 0x65e41c/0x65e420
run them before the decoders copy the marker into function-local statics.
A fresh launch verifies correctly rendered menu buttons and dialog borders.

All four fixes pass the full build and link gates without MAX regressions.
The six newly enrolled entries are existing retail CRT initializers: the
context binding, its network-player consumer, the two resource tables and
the two sprite constants. They all score 100%; their address operands were
also checked independently of objdiff's relaxed relocation scoring. No
gameplay function was added. The sprite fix changes no existing function
scores, so the README matching totals remain unchanged.

The Complete intro plays, confirmed by advancing captured frames, and the
main menu now renders correctly. A spurious CD-version warning remains:
`earlySetup` conditionally enters its legacy video-archive scan, whereas
the pinned retail body unconditionally jumps over it. That control-flow
mismatch is separate from the recovered data initializers. After dismissing
the warning, the user reached the menu and reported an “Out of memory”
termination when selecting multiplayer. Its cause has not yet been traced;
map, battle and gameplay execution remain unverified.
