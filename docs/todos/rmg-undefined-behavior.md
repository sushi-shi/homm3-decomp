# RMG undefined behavior and retail quirks

Remaining uninitialized reads and reproduced retail faults in the random map
generator, to revisit when behavior may diverge from retail. Evidence,
reproduction requests and the conditional-contract table are in
[RMG undefined behavior](../reference/rmg-undefined-behavior.md). That list is
not an exhaustive UB audit.

The default build reproduces retail. Defining `HOMM3_RMG_HOTFIX` (described at
the top of `include/rmg.h`) selects the defined alternative given for each item
below; those maps differ from retail.

1. **Boundary test slot town flags.** `buildZoneBoundaries` (`0x53e050`,
   constructor call `0x53e149`; `src/rmg.cpp`, `testSlot`). Retail reads nine
   uninitialized stack bytes. It always found heap-call residue there and so
   drew one `rand()`. The source sets every flag to `true` to keep that draw.
   The residue model is sampled over 3,000 requests, not proven.
   HOTFIX: allows no towns, so the probe zone makes no draw.

2. **Connection-path fallback seed.** `buildZoneConnectionPaths` (`0x5405d0`,
   fallback `0x54077d`; `src/rmg.cpp`, `seed`). A zone without an eligible cell
   reuses the last assigned seed, as retail does. Before any assignment, retail
   reads stack residue. The source starts at `RMG_NO_POSITION` and skips that
   zone. This case was never observed, so its frequency is unmeasured.
   HOTFIX: tracks `foundSeed` per zone and skips any zone whose own scan found
   no cell, so no seed is borrowed or read uninitialized.

3. **Initial key-tent colour.** `type_random_map_generator::m_nextKeyTentColor`
   (`+0xf5c`, read at `0x540d6d`). It is an uninitialized integer in the
   stack-allocated generator, preserved in source. The oracle controls it with
   `stackWord`. HOTFIX: the constructor starts it at colour 0.

4. **Added water zones' town flags.** `buildZoneBoundaries` heap
   `TRmgTemplateZone` (`0x53e45c` constructor call). Retail never writes them;
   reused heap bytes decide whether `TRmgZone`'s constructor draws. The source
   now allows every town (`memset` true), which reproduces retail's one draw
   whenever any residue byte is nonzero; the drawn alignment has no
   consequential reader. It differs only when all nine bytes are zero (oracle
   `heapByte` 0, where retail skips the draw). The zone's `m_creatureTownType`
   is never set but is read by treasure `getValue` before the terrain filter;
   its effect on output is untraced. HOTFIX: allows no towns (no draw), clears
   the template zone's unwritten `m_neutralTownsMatchZone`,
   `m_useNativeTerrain` and `m_guardsMatchZone`, and sets the zone's
   `m_creatureTownType` to `eTownNeutral`.

5. **Reproduced retail faults.** River drawing follows an invalid predecessor
   into `TRmgRiverMapAdapter::getLineType` (`0x53284f`). The monolith guard
   reads `m_zones[-1]`, faulting in `createGuard` (`0x540b34`). Both are
   preserved by default. HOTFIX: `createRiver` and `createRiverToObject` draw
   only when the tested cell's cost is below `RMG_UNREACHED_COST` (the search
   reached it, so its predecessor chain is valid); `placeGuard` skips cells
   off the map or with zone `RMG_NO_ZONE`.

6. **Other unchecked operations.** Each is preserved by default.
   - `createGuard` (`0x540b20`): counted creatures without prototypes let the
     selection reach `creature == -1`. HOTFIX: no guard (returns null).
   - `createRoads` (`0x548290`): an empty target list underflows
     `size() - 1`. HOTFIX: the loop runs while `first + 1 < size()`.
   - `markRiverCoastTarget` (`0x548a40`): the coast scan admits `x == width`,
     aliasing the next row or reading past the map. HOTFIX: rejects it.
   - `removeObject` (`0x54bc50`): failed finds are tested against null rather
     than `end()`. HOTFIX: compares with `end()`.
   - `writeMapHeader` (`0x549cb0`): `char description[500]` with unbounded
     `sprintf`/`strcat`; eight humans with town choices already overflow.
     HOTFIX: a 1024-byte buffer and the template name cut to 255 characters.
   - `getSerializedMapVersion`: an unknown version falls off the end.
     HOTFIX: writes format 28 (Shadow of Death).
   - `loadTemplates` and `readRmgTemplateZones`: short rows read the maximum
     size or zone index column past the row. HOTFIX: such rows are skipped.

   The remaining conditional contracts in the reference table (malformed
   assets or templates, out-of-range request values, arithmetic overflow) and
   the behavioural quirks listed there are unchanged under HOTFIX.
