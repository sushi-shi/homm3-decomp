# RMG prototype writer: signed planes and native const access

`writeRmgObjectPrototype` now uses the counted packed-plane helper for its two
ten-bit terrain masks, retaining one caller-owned two-byte buffer and the
native signed loop domain. The six unsigned-index success-writer instances
used by `writeMapHeader` remain unchanged. The earlier writer audit's statement
that these terrain masks belong to the unsigned const family was too broad:
constness and index signedness are separate source facts.

This work depends on the four-argument `writePackedBitsCount<WireBytes>`
recovery in `f7ea8b18a`. Its `Bits&` parameter preserves const or mutable bitset
access; its signed index and raw int write result do not depend on constness.
RMG passes the native signed ten-bit bound, uses const mask views, and ignores
the returned byte count, as before. Mapcell/town retain their own bounds,
mutable proxies and checked count results. The shared implementation remains
in `packed_bits.h`; no per-size specialization, pasted caller loop, duplicate
writer body, pragma or artificial compiler-budget statement was introduced.

## Positive native evidence

- Mac `0x24fb44..0x24fc44` uses one buffer at stack offsets `0x58/0x59` for
  both terrain masks. `srawi` plus `addze` at `0x24fb70/0x24fb78` and
  `0x24fb80/0x24fb88` performs signed division/remainder; `cmpwi` at
  `0x24fba4` bounds the first loop at ten. The second loop repeats this at
  `0x24fbf0..0x24fc28`. Both call the const ten-bit test at `0x12b128`.
- Loki's original `getTerrainMask__C11TObjectType` at `0x08363aac` returns
  `this + 0x14`; its existing recommended-mask counterpart at `0x08363ac0`
  returns `this + 0x18`. The restored const getter preserves that spelling
  and operation. Its class-inline body exposes the same reference path to
  the serializer. Mutable field consumers and member visibility are unchanged.
- The Complete-only serializer has no mapped Dreamcast counterpart. No
  original name is claimed for the counted writer or its header, and a
  code-generation match does not uniquely recover the original source.

The serializer retains `getBCellPassable`, `getBCellTrigger`, `_getBitPos`,
`getRecommendedTerrainMask`, all field queries and serialization order. The
shared buffer is live across both complete clear/pack/write calls. Declaring
that same buffer at function entry or immediately before its first use gives
identical candidate objects in the controlled game comparison; no synthetic
lifetime scope is needed.

## Measured results

All results use the pinned image-specific VC6 profiles in an isolated SSD
checkout, starting at `043806c7b`. They compare the complete RMG TU and preserve
unchanged-source and opposite-corner reproduction controls. These are PR
branch results; no default-branch publication occurred.

| Compared function/image | Starting MAX / fresh baseline | Candidate |
| --- | ---: | ---: |
| `writeRmgObjectPrototype`, game `0x0054ae30` | 91.2554% | 98.8489% |
| Same source, map editor `0x004ad3eb` | 69.1513% | 71.6303% |
| Same source, available Mac pair `0:0x24f980` | 20.23305085% | 20.88607595% |

No projected Windows MAX losses occur across either selected RMG TU. All six
retained editor `writePackedBits` instances (156, 128, 144, 129, 70 and 28 bits)
measure **CUR 100% before and after**, independently of their hashless template
MAX rows. Their complete raw COMDAT code sections are byte-identical between
baseline and accepted candidate, with sizes 112, 111, 113, 112, 105 and 101
bytes. The candidate-only unsigned ten-bit instantiation disappears because
RMG now uses the supported signed counted family; it had no retained retail
writer claim.

Mac retains all 67 available RMG comparisons (13 exact), with the other 66
scores unchanged. The existing 115 emitted-unavailable pairs and sixteen
claims without a full-TU body remain explicit coverage gaps. The improved
prototype pair also exceeds its pre-follow-up 20.5128% observation, so its
obsolete `MAC_ABSTRACTION_FROM` annotation was removed. No new waiver or
literal/TOC manifest is needed.

## Remaining differences and bounded probes

The game candidate's instruction comparison agrees with retail except for
zeroing each terrain buffer: the helper's two-byte `memset` becomes one word
store, while retail has two immediate byte stores. Trailing padding also
shifts. A genuine counted-byte-clear alternative leaves both Windows target
scores unchanged; its separate mapcell/town check reduces town's Mac score
from 64.5503% to 49.3590% and removes the native bulk-clear boundary. It is
rejected. The common `memset` operation remains intact; no per-N clear policy
was invented to manufacture the two-byte sequence.

The editor's byte-identity-gated inline trace reproduces the compiled object:
caller cost 469, initial budget 1000. Both counted-writer sites cost 104 and
pass their ordinary budget checks; their const subscript/test descendants
are considered, but the final caller retains the outer writer. Retail instead
expands it and retains nested `bitset<10>::_Xran`. Both versions have nine
calls, so aggregate call counts conceal this mismatch. The trace identifies
a nested inline frontier, not a missing packed operation. Four ordinary
scalar `writeValue` call variants (length, four scalar fields, both, neither)
are target-score-flat at 71.6303%; they were not adopted as a budget device.
The remaining loss against historical 100% game / 99.6471% editor is recorded,
not hidden by those retained historical peaks.

## Reproduction artifacts and integration

Ignored artifacts under the naming worker's `build/rmg-writer-recovery/`
contain manifests, logs, the raw six-writer hash comparison and adopted
Windows/Mac reports. Controlled source-family contexts are:

- `f38369fe4824d4f45daf`: three game states, two distinct objects, two
  reproduced elites (buffer lifetime placement).
- `e681194328ce5974bdb9`: three game states and three objects (clear forms).
- `76c7ff4b489105283575`: the same three editor states and three objects;
  measured CUR results and raw helper sections establish six-writer preservation.
- `c1775089bb8a1d798a96`: four editor scalar-helper states, four objects,
  two reproduced elites; target score unchanged.

Each family retains its baseline/reproduction controls. A fresh adopted
`homm3 build --fast rmg`, `homm3 --image h3maped build --fast rmg`, and
`homm3 mac build rmg` reproduce the results above. The coordinator owns the
combined source-aware ledger refresh and PR integration; this worker does
not run a routine whole-tree build merely to bank one targeted result.

A separate source lead remains in the editor-only `TObjectType` stream
operator (`src/objecttype.cpp`): its explicit nine-bit conversions still read
the terrain fields directly before `to_ulong()`. Loki computes the same
`this+0x14/+0x18` addresses before converting to nine bits, without a retained
getter call at those sites. Reusing both const getters is a supported next
source hypothesis; it must preserve that nine-bit conversion and receive its
own editor comparison. This RMG change does not alter that operator or claim
its getter substitutions were tested.
