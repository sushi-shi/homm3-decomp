# Review of the 111 unnamed Mac targets

All 111 distinct unnamed targets from the active helper-sweep snapshot now have
an assembly-supported operation description. These are working descriptions,
not recovered original names or newly established Windows source identities.
The review used the pinned PEF bodies, imported OS names, callers, and current
Windows sources. The last nine were also checked against pinned Windows retail
assembly. No workers, compiler runs or matching searches were used.

| Operation group | Targets |
|---|---:|
| Platform services and adapters | 97 |
| Library operations | 5 |
| Game implementations differing from current Windows source | 7 |
| Shared string-helper candidates | 2 |
| Total | 111 |

All 111 operation labels and their evidence now live in the existing
[runtime-map.tsv](../../config/mac/runtime-map.tsv). Extents live in
[functions.tsv](../../config/mac/functions.tsv). The temporary observations TSV
has been removed. Names are descriptive where original linkage is unknown.

The last nine are identified as follows:

| Mac offset | Operation | Windows finding |
|---|---|---|
| `0x151ff4` | Cache insert | Retail `addToCache` calls tree insert; Mac scans a fixed array. |
| `0x15208c` | Cache remove | Retail `dispose` erases a tree iterator; Mac scans resource pointers. |
| `0x1520e8` | Cache find | Retail uses case-insensitive tree lookup; Mac uses a case-sensitive hash and equality. |
| `0x17cde8` | Pointer bubble sort | Retail `sortMaps` calls six Dinkumware record-sort specializations. |
| `0x2181a0` | Service music fades | Mac updates three stream records; Windows services Miles and fades a single stream separately. |
| `0x21b7dc` | AppleTalk join dialog | Windows has DirectPlay, modem and serial paths. |
| `0x21bca8` | TCP join dialog | Windows search has similar fields but a different transport lifecycle; no one-to-one pairing established. |
| `0x26afe8` | Copy and lowercase ASCII | Windows quests use scalar CRT `tolower`; Windows MP3 checks do not lowercase the name. |
| `0x26b018` | Boolean string equality | Windows MP3 checks expand `strcmp`; a shared equality wrapper remains unproven. |

The first seven use owner `mac_port`: an identified port implementation without
an established shared source identity. The two string utilities keep unknown
ownership. Their operations are clear; their library or project provenance is
not established. The map records concrete Mac call sites and Windows addresses.

`homm3 mac helper-audit` reads the map, includes labels and evidence in function
records, and exports `callee_name` in its calls TSV. Labels never create source
identities or close caller discrepancies. The generated queue retains 17 source-owned call sites to these nine targets
as `source_callee_unavailable`; no game C++ or caller completion state changed
in this pass.

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

All mapped extents validate against the pinned PEF;
the Mac table validator reports no defects. All 50 focused tooling tests pass,
including checks covering the
requirement that a mapped operation cannot invent a source identity or close
an unavailable-source call. Game C++ and matching scores were not changed.

The earlier 111 count meant unnamed binary destinations, not 111 missing game
helpers. This pass identifies their operations. Source recovery and propagation
through every caller remain the next task; no caller-completion state is banked
by these labels.
