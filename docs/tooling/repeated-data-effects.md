# Bounded repeated startup writes

Initialization analysis now follows `MOVS` and `STOS` in byte, word and dword
forms, including bounded `REP` operations. The same implementation processes
retail instructions and raw candidate COFF. It contains no game symbol names,
retail addresses or expected initializer values.

A copy can recover values from a known stack aggregate. Destination pointers
and pointer-valued aggregate members must still resolve through the existing
independent source/code bindings. Reading an unmodeled global does not borrow
its retail contents or assume its static bytes survive until startup. Unknown
source values, conflicting destinations and unsupported instructions remain
incomplete effects.

The analysis records each observed non-stack read/write in the existing
`data-initialization.tsv` structured cells, with its instruction site, iteration,
count, element width, direction, source and destination. Exact initializer
comparison still requires a complete supported straight-line path, known writes
and no calls. Pairing by unique shared storage remains a lead; the complete final
write maps, including values and destinations, decide the effect verdict.

## Bounds and assumptions

- `ECX` must be known for `REP`; zero performs no accesses. Each instruction is
  limited to 4,096 transferred bytes. Unknown or larger counts produce an
  explicit issue and unknown write, invalidate stack facts, and cannot earn
  exact-effect credit.
- Function entry uses the Windows x86 C convention that the direction flag is
  clear. Microsoft documents that convention for its
  [runtime routines](https://learn.microsoft.com/en-us/cpp/c-runtime-library/direction-flag)
  and requires inline assembly to
  [restore changes to the flag](https://learn.microsoft.com/en-us/cpp/assembler/inline/using-and-preserving-registers-in-inline-assembly).
  This is an ABI premise, not an observation of the process's initial flags.
  `CLD` and `STD` update the direction; conflicting joins, unmodeled calls and
  unsupported flag writes lose the fact. The analyzer also accepts an unknown
  entry direction for callers without that premise.
- Default flat DS/ES and 32-bit addressing are required. Segment overrides,
  address-size overrides and unsupported repeat prefixes remain explicit gaps.
- Transfers execute sequentially, preserving overlap behavior. Known integer
  stack stores can supply smaller loads or be reassembled; symbolic pointers
  survive only exact-width copies. Partial overwrites conservatively discard
  overlapping larger stack facts rather than invent untouched bytes.

## Validation

Generic controls cover forward/reverse stores, byte/word tails, zero and unknown
counts, the transfer limit, conflicting direction joins, segment/address-size
overrides, overlapping copies, unknown source/destination/relocation identities,
stack invalidation, symbolic values and register postconditions. Initializer
comparison controls change counts, values and destinations independently and
require differences. These fixtures use unrelated constants and addresses.

The [PR #78 case audit](pr78-data-contract-audit.md) records the application to
the archive-context initializer. Static zero-fill agreement remains separate
from these startup writes.

Both full builds pass and retain their static byte verdicts and consumer exports.
The base now diagnoses the archive owner's missing startup writes; the repaired
head proves all 96 bytes exactly, raising its proved effect coverage from 374 to
470 bytes. Independent replay of raw retail and candidate instructions verifies
every exact pair: 14 pairs/326 bytes on the base, 19 pairs/470 bytes on the head.
Every previously exact pair remains exact. Another newly paired initializer
remains unproved and receives no byte credit.
