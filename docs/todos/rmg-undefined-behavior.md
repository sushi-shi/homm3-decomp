# RMG undefined behavior and retail quirks

Remaining uninitialized reads and reproduced retail faults in the random map
generator, to revisit when behavior may diverge from retail. Evidence,
reproduction requests and the conditional-contract table are in
[RMG undefined behavior](../reference/rmg-undefined-behavior.md). That list is
not an exhaustive UB audit.

1. **Boundary test slot town flags.** `buildZoneBoundaries` (`0x53e050`,
   constructor call `0x53e149`; `src/rmg.cpp`, `testSlot`). Retail reads nine
   uninitialized stack bytes. It always found heap-call residue there and so
   drew one `rand()`. The source sets every flag to `true` to keep that draw.
   Possible real fix (`TODO` beside it): allow no towns, so no draw. This
   changes generated maps. The residue model is sampled over 3,000 requests,
   not proven.

2. **Connection-path fallback seed.** `buildZoneConnectionPaths` (`0x5405d0`,
   fallback `0x54077d`; `src/rmg.cpp`, `seed`). A zone without an eligible cell
   reuses the last assigned seed, as retail does. Before any assignment, retail
   reads stack residue. The source starts at `RMG_NO_POSITION` and skips that
   zone. This case was never observed, so its frequency is unmeasured. Possible
   real fix (`TODO`): skip any zone whose own scan found no cell.

3. **Initial key-tent colour.** `type_random_map_generator::m_nextKeyTentColor`
   (`+0xf5c`, read at `0x540d6d`). It is an uninitialized integer in the
   stack-allocated generator, preserved in source. The oracle controls it with
   `stackWord`. Possible real fix: initialize it in the constructor. This
   changes key-tent and guard colours.

4. **Added water zones' town flags.** `buildZoneBoundaries` heap
   `TRmgTemplateZone` (`0x53e45c` constructor call). Retail never writes them;
   reused heap bytes decide whether `TRmgZone`'s constructor draws. The source
   now allows every town (`memset` true), which reproduces retail's one draw
   whenever any residue byte is nonzero; the drawn alignment has no
   consequential reader. It differs only when all nine bytes are zero (oracle
   `heapByte` 0, where retail skips the draw). Possible real fix (`TODO` beside
   it): allow no towns. The zone's `m_creatureTownType` is never set but is
   read by treasure `getValue` before the terrain filter; its effect on output
   is untraced.

5. **Reproduced retail faults.** River drawing follows an invalid predecessor
   into `TRmgRiverMapAdapter::getLineType` (`0x53284f`). The monolith guard
   reads `m_zones[-1]`, faulting in `createGuard` (`0x540b34`). Both are
   preserved. Possible real fixes: test search success before drawing, and
   check the zone index before indexing. Further conditional hazards are in
   the reference's contract table.
