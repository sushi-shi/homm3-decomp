# Executing RMG against retail

`homm3 rmg compare` builds and runs the recovered whole-map generator beside
the pinned English GOG Complete executable. It compares uncompressed map bytes,
generation return codes, the post-call request, final retail RNG state, and x87
control words before and after generation. Every case runs twice on each side,
in four fresh processes. Nothing is masked out of the comparison.

Inside the VC6/Wine build shell, with `homm3 init` already completed:

```sh
export HOMM3_GAME_DIR='/path/to/installed/game'
homm3 rmg compare --seed 1
```

This runs a default 36×36 request twice through retail and twice through the
candidate. Decimal and `0x` hexadecimal seeds are accepted. Use `--stack-word`
and `--heap-byte` to select the controlled memory inputs for a direct seed run.
For custom requests or batches, pass a JSON case file:

```sh
mkdir -p build/rmg-cases
cat > build/rmg-cases/small.json <<'JSON'
[
  {"name": "small", "seed": 1},
  {"name": "underground", "seed": 123456, "levels": 2, "waterContent": 1}
]
JSON
homm3 rmg compare --cases build/rmg-cases/small.json
```

The installation selected by `--game-dir` or `HOMM3_GAME_DIR` supplies `Data/`
and `BINKW32.DLL`, `MSS32.DLL`, `SMACKW32.DLL`, and `IFC20.dll`. Its executable
is not used. The harness verifies the pinned
`build/orig/HEROES3.EXE` and patches a disposable copy under `build/`.
`--out` selects a new output directory; an existing directory is rejected.
`--timeout` is a per-process limit in seconds, default 300. The harness neither
launches the game UI nor changes the installation.

Each case requires a unique `name` and a 32-bit unsigned `seed`. `stackWord`
optionally specifies a 32-bit unsigned initial stack word (default zero).
The common entry thunk pushes this word across 64 KiB of future stack storage,
then restores the stack pointer and calls the selected implementation from
the same call site. Descending pushes respect Windows stack guard pages.

This is an input, not an output normalization: retail leaves the generator's
initial key-tent color at `+0xf5c` uninitialized. Different caller frames changed
key-tent/guard colors. The recovered game code preserves this retail behavior.

`heapByte` controls the initial byte of every C++ allocation during generation
(0..255, default zero). The harness redirects operator new's allocator call at
`0x616ed8` through a fill wrapper, retaining the real CRT allocator at `0x61a417`
and its allocation/failure policy. It installs this hook after resource and trait
initialization. Retail's added water-zone slots leave `m_allowedTowns` untouched
before `TRmgZone` reads them at `0x532a47`; reused heap contents can therefore
add a random draw and change the entire map. This was observed in retail-vs-retail
runs. Vary the fill to exercise both zero and nonzero uninitialized flags.
Set `heapByte` to JSON `null` to retain native allocation contents for diagnostics;
such runs can legitimately report nonrepeatability.

These controls supply storage contents before game code uses them; no generated
bytes or RNG draws are normalized. They do not guarantee every uninitialized read
is controlled: intervening calls can overwrite initial stack contents. The repeat
checks remain necessary. Seed alone does not describe the complete execution state.
See [known RMG undefined behavior](rmg-undefined-behavior.md) for the retail
addresses, observed effects and reproduction inputs for these uninitialized reads.

Other optional fields
correspond to the 80-byte `TRandomMapRequest` in `include/rmg_request.h`:

| Field | Default | Accepted values |
|---|---:|---|
| `width`, `height` | 36 | Equal; 36, 72, 108, 144 |
| `levels` | 1 | 1 or 2 |
| `humanPlayerCount` | 2 | 1..8 |
| `humanTeamCount` | 2 | 0..8 |
| `computerPlayerCount` | 0 | 0..7; total players 2..8 |
| `computerTeamCount` | 8 | 0..8 |
| `waterContent` | 3 | 0..3 (3 requests random) |
| `monsterStrength` | 0 | -2..2 |
| `mapVersion` | 2 | 0..2 |
| `isHumanSeat` | Eight zeroes | Eight 0/1 bytes |
| `townType` | Eight -1s | Eight integers, -1..8 |

Defaults reproduce the request constructor, including its team-count defaults.
The generator can reject a validly encoded combination or find no compatible
template; its return code is part of the result, not a harness crash.

The command returns 0 for identical, repeatable results; 1 for differences or
nonrepeatability; 2 for build, input, timeout, crash, or output-protocol failures.
`report.json` distinguishes retail repeatability, candidate repeatability, and
cross-implementation agreement. Each process directory retains `job.bin`,
`map.raw`, `result.bin`, stage logging, Wine logging and `run.json`.
Crashes include the exception code, instruction address and initialization stage.
Differences include hashes, lengths, first differing byte and nearby bytes.
The driver disables process error boxes and the launcher disables Wine's debugger,
so crash cases remain unattended batch results rather than interactive popups.
`cases.json` records every input, including the stack and allocation fill.
`provenance.json` records the input image, all asset and vendor-library hashes,
Wine version, compiler object hashes, compiler profile manifest and driver hash.
The unmodified objects, disposable link inputs, linker map and explicit external
bindings are retained beside it. Keep cases and outputs in ignored `build/`.

## Sampled campaign result

A completed campaign tested 100,000 unique seeds and requests generated from
master seed `0x524d4732`. It varied map size, level count, map version, water and
monster settings, player and town arrangements, the full 32-bit `stackWord`, and
all 256 `heapByte` values. Each ordinary case ran retail and candidate once;
periodic cases and every failure were repeated in fresh processes.

- 99,840 cases generated byte-identical maps and matched return values, final RNG
  state, post-call requests, and x87 state.
- 160 cases crashed deterministically in both implementations at corresponding
  instructions: 137 in the documented river path and 23 in the documented
  negative-zone monolith guard path.
- There were no output mismatches, candidate-only failures, retail-only failures,
  or repeatability disagreements.

The ignored campaign artifacts and full per-case inputs are under
`build/rmg-oracle/sampled-100000/`. These results establish agreement for the
sampled execution states; they do not turn the behavioral oracle into byte-match
evidence.

## Execution boundary

The original PE loader maps retail at `0x00400000`; the original entry point
runs its CRT and static initialization. Only WinMain's admitted `0x1cf` bytes
at `0x4f7a30` are replaced, with a bootstrap that loads `rmg-driver.dll` and calls
`run`. The driver uses VC6, `/NOENTRY /NODEFAULTLIB`, and the already initialized
retail CRT. It is fixed at `0x30000000`, away from the vendor DLL bases. VC6 LINK's
REL32 handling of ABS externals requires subtracting this base from their
relocation addends in disposable COFF copies; absolute data references are left
alone. Original compiler objects are never rewritten.

Candidate mode links fresh `rmg.obj`, `rmg_support.obj`, and `rmg_terrain.obj`
using their normal Ninja dependency graph and per-TU profiles. A strict link
rejects unresolved symbols. The external-service allowlist cannot substitute
retail RMG functions or data: candidate vtables, tables, initialization and
generator bodies all come from the recovered objects. Runtime, resource loading,
object prototype loading, trait tables and `buildTileNeighbourMask` remain retail
services. Game data bindings come from owning source `DATA` annotations; named
function bindings come from the generated inventory. The few unnamed CRT bridges
are documented in `scripts/homm3/rmg/bindings.py`.

Before generation, both modes run the same resource and trait initialization,
including adventure-object traits (`0x41b500`). `_time` at `0x6198e0` is patched
to return the case seed and write it through a non-null output pointer before
constructing a generator. Retail `srand`/`rand` remain intact, with final state
read from `__getptd()+0x14`. A null progress interface keeps execution headless.
`TRandomMapRequest::generateToFile` writes through a real `TAbstractFile` subclass
into `map.raw`; compression and file-container metadata are outside this boundary.

Every process first compares the independently entered candidate and retail
request constructors as a small ABI/link control. Tooling tests exercise output
mutation detection, COFF relocation handling, common-symbol resolution, rejection
of retail RMG fallbacks, bootstrap extent, input validation, and crash/timeout
classification:

```sh
python3 -m unittest homm3.rmg.test_rmg homm3.core.test_cc_wrap
```

This is a behavioral oracle for the stated boundary, not proof of a byte match or
of whole-game link completeness. Agreement covers the selected assets, requests
and seeds. Continue using `homm3 build` for byte comparison and repository gates.
