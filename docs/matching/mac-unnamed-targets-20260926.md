# Review of the 111 unnamed Mac targets

All 111 distinct unnamed targets from the active helper-sweep snapshot now have
an assembly-supported operation description. These are working descriptions,
not recovered original names or newly established Windows source identities.
The review used the pinned PEF bodies, imported OS names, callers, and current
Windows sources; no workers, compiler runs or matching searches were used.

| Operation group | Targets |
|---|---:|
| Platform services and adapters | 97 |
| Library operations | 5 |
| Game implementations differing from current Windows source | 7 |
| Shared string-helper candidates | 2 |
| Total | 111 |

The findings live in [target-observations.tsv](../../config/mac/target-observations.tsv).
Each observation has a verified code extent, SHA-256, category, descriptive
operation and concrete evidence. `homm3 mac helper-audit` attaches them to
function records and call sites; its calls TSV includes `callee_operation` and
`callee_category`. Selecting a Mac address displays the evidence. Names and
source identities still come from ordinary source declarations and address
claims. An observation does not supply a compiler linkage symbol.

## Leads for the next pass

- `0x26afe8`: copies an ASCII string while lowercasing A..Z. Quest descriptions
  and MP3 paths use it. Check whether their Windows lowercase operations should
  share a canonical helper.
- `0x26b018`: bytewise string equality returning boolean, not strcmp ordering.
  Cache lookup and MP3 paths call it. Inspect Windows callers before recovering
  the shared interface.
- `0x151ff4`, `0x15208c`, `0x1520e8`: insert, remove and find in a fixed
  0x4000-entry resource cache. Current Windows source uses `TCacheMap` operations.
- `0x17cde8`: bubble sort of pointers with a predicate and two context arguments.
  Current Windows `sortMaps` uses `std::sort` over records. This is not an MSL
  sort implementation or sufficient reason to replace the Windows algorithm.
- `0x2181a0`: services three Mac music streams and their volume fades.
- `0x21b7dc`, `0x21bca8`: AppleTalk and TCP join dialogs, respectively.

The five library operations are selection-header vector append/destruction,
network-message deque append, `SmackToBuffer`, and `SmackToBufferRect`. The
platform group includes graphics surfaces, display modes, file and directory
adapters, audio channels, event/callback dispatch, network transport and
GameRanger integration. These classifications identify operations; they do not
prove that their corresponding Windows callers are complete or exempt them
from reconciliation.

## Function boundaries

Added three previously missing extents:

- `0x187940 + 0x1cc`: vector append ends with BLR at `0x187b08`, followed by the
  next prologue at `0x187b0c`.
- `0x275b60 + 0x68`: blend-color construction returns at `0x275bc4`; the next
  instruction is a separate empty return body.
- `0x275bcc + 0x10`: RGB555 packing returns at `0x275bd8`, before another empty
  return body.

Corrected the existing `0x297510 + 0x264` span to `0x297458 + 0x31c`:
`0x297510` is a conditional-branch destination inside `SmackToBuffer`. Its
prologue is at `0x297458`, callers branch there, and its return is at `0x297770`,
before the next prologue at `0x297774`. No source claim or runtime label owned
that erroneous interior entry.

## Validation and limits

All observation hashes and spans validate against the pinned PEF; the Mac table
validator reports no defects. Ten focused tooling tests pass, including the
requirement that an observed operation cannot invent a source identity or close
an unavailable-source call. Game C++ and matching scores were not changed.

The earlier 111 count meant unnamed binary destinations, not 111 missing game
helpers. This pass identifies their operations. Source recovery and propagation
through every caller remain the next task; no caller-completion state is banked
by these observations.
