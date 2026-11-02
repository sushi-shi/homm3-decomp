# Video API naming and declaration ownership

This review applies the HoMM1/HoMM2 cleanup approach to the shared video API:
give understood parameters meaningful names, and declare shared functions in
their owning header. The starting point is `eb0115861`. The precedents reviewed
were HoMM1 `1dd0fbb4` (give slot-spelled locals names supported by every use)
and HoMM2 `ef1b504f0` (declare each unit's free functions in its owner header).

## Reviewed changes

| Location | Cleanup | Evidence |
| --- | --- | --- |
| `videoOpen`, retail `0x00597570` | Replace the placeholder parameters `a6`, `a7`, and `a8` with `loop`, `autoDraw`, and `advance`. | The wrapper forwards them unchanged to `openSmackerVideo` (`0x00598af0`). That body stores them in the loop field, screen-update flag, and frame-advance field respectively. Its Bink branch forwards the first two to the parallel loop and screen-update fields. |
| `openSmackerVideo` declarations | Give `smackmgr.h` the shared declaration; remove the local copies in `smackmgr.cpp` and `remote.cpp`. | `smackmgr.cpp` defines the function. Both translation units already include `smackmgr.h`; `CGameTransferSmack::start` (`0x00557480`) is an independent consumer of the opener. |
| `SmackManager::nextSmackerFrame` | Move the forward declaration to the namespace's declaration group in `smackmgr.h`. | The helper belongs to the Smacker playback API beside draw/restart/close; its definition remains in `smackmgr.cpp`. |
| `SmackManager::closeSmacker` | Remove the duplicate translation-unit declaration. | `smackmgr.h` already declares the same namespace function. |
| `loadAnimHeaders` and `loadSoundHeaders` | Declare both loaders beside their teardown functions in `smackmgr.h`; remove the local declarations in `kb.cpp`. | `smackmgr.cpp` defines both archive loaders, while `kb.cpp`'s startup sequence consumes them through an already-included `smackmgr.h`. |
| `g_archiveDriveLetter` ownership comment | Correct the claim that its definition lives in the original owning TU. | The definition is currently in `smackmgr.cpp`; the referenced misc.obj installation scan remains unreconstructed. The DATA address and definition stay unchanged, with ownership explicitly provisional. |
| `playSmackerCore` | Rename `vw`, `vh`, and `pos` to `viewWidth`, `viewHeight`, and `videoPosition`. | The first pair starts with the requested view extent, falls back to the decoded video extent for negative requests, and controls centering and final redraw. The point captures the resulting video origin for the redraw. These are project-derived local names in the recovered helper. |

The parameter names describe roles proven by the Windows implementation; they
are not a claim that the original developers used those spellings. These
changes preserve parameter types, order, function linkage, helper calls, and
playback behavior. No symbol is renamed.

`homm3 dreamcast show 0x00597570` identifies the four-byte WinCE `VideoOpen`
stub at `dc:0x0014ac3c`. It records no parameter names or locals, so it cannot
supply original spellings for these arguments. Its primitive-byte display
does not override the existing Boolean interface, whose decorated-public
evidence is documented in the owning source.

## Validation

The source and build outputs were prepared on the NVMe SSD. The focused
`homm3 build --fast smackmgr remote kb -j2` check passed with **no MAX change**
in any of the three units. `homm3 status functions` confirms both
`videoOpen` (`0x00597570`) and `videoPlay` (`0x005972d0`, which expands the
renamed modal-playback helper) remain CUR/MAX/HIST **100/100/100** with fresh
comparisons. The declaration consumers compile under the pinned VC6 profiles.

`homm3 mac build smackmgr` passed for the available comparison:
`videoRestart` remains **100%**. The other 23 emitted pairs have unresolved
references and remain explicitly unavailable; this check does not certify
those bodies. No Mac data/descriptor inventory was added to force a score.

The banked report refreshes `videoOpen`'s source hash without a score loss.
The unchanged-source `game::transmitSaveGame` CUR moved from 97.4264% to
97.4042%; its MAX and HIST both remain 97.4264%. This is recorded TU-state
variation, with no corrective source change.

The measured Windows report is banked with
`homm3 status update --write-readme` before the lane handoff. The focused logs
remain in ignored `build/naming-fast.log`, `build/naming-mac.log`, and
`build/naming-status-update.log`. No mock behavior tests were used.

## Related work and remaining scope

The earlier `cleanup/naming-pass` branch has seven commits after this starting
point, ending at `e039de8eb`. They recover combat and hexcell member spellings,
owner-byte and corpse-lookup names, and remove duplicate global declarations.
Those changes are a separate integration input and are not repeated here.
Its uncommitted `include/hero.h` and `src/hero.cpp` changes were left untouched.

This is a bounded video-API review, not a claim that every remaining name or
declaration in the repository has been reviewed. The sound/video archive
records and the provisional descriptor-owner comments retain their existing
evidence and are outside this change.

### Retained decisions

- `openSmackerTrack` and `getDriveArchivePath` remain ordinary functions with
  their existing linkage. Their currently reconstructed calls are local to
  the TU, but that alone does not prove original `static` linkage; the drive
  scan described beside `getDriveArchivePath` is still unreconstructed.
- The `SoundHeaderStruct` and `VideoHeaderStruct` fields keep their current
  layout and types. Their archive records are shared with ResourceManager
  and Bink; a native-member-spelling pass should include those consumers.
- Video handles, counters, and header arrays retain the numbered archive
  correspondence documented at their definitions. This review does not
  invent a name for the unresolved descriptor owner's original TU.
- `videoSoundOnOff(int on)` keeps its unused formal: the Windows signature
  carries it, and renaming or removing it would not explain the codec
  service-tail behavior documented at the body.
- `videoDrawRects` retains the SDK bounds record, its coordinate references,
  and its ordered union calculations. Its source already records native
  platform differences and tested declaration-scope alternatives; this
  naming review does not replace those calculations with a generic union.
- The `load*Headers` `nread` variables, geometry arguments (`x`, `y`, `w`,
  `h`), RGB mask names, and ordinary loop index `i` are already understandable
  in their local domains and are not placeholder slots.
