# Anonymous-namespace names

VC6 names an anonymous namespace `?%<path><number>@`. The name appears in
decorated symbols and in RTTI type descriptors, e.g.
`.?AVt_initialize_failure@t_initializer@?%C:\Dev\Heroes 3 Exp 2\Game\ForceFeedback.cpp210603558@@`.
The number is neither a checksum of the source text nor of its timestamp.

## Mechanism

Measured with the pinned VC6 SP3 `CL.EXE` under Wine and libfaketime (frozen
clock, `FAKETIME_DONT_FAKE_MONOTONIC=1`):

- `<path>` is the source path exactly as spelled on the `CL` command line. A
  relative argument gives a relative path.
- The number depends only on that spelling and on the compiler's wall clock,
  at one-second resolution. File contents and modification time do not change it.
  The same path and the same frozen second always give the same number.
- The front end seeds the CRT generator and prints two draws:

  ```
  seed   = time(NULL) ^ (crc32(path) ^ 0xffffffff)   # zlib CRC-32, no final xor
  srand(seed); r1 = rand(); r2 = rand()              # MSVC LCG 214013/2531011
  number = "%d%d" % (r2, r1)
  ```

  This was verified on five controlled compiles, two paths and three frozen
  seconds. Bit 31 of the seed does not affect the draws.

## Recovering retail compile times

The decimal concatenation is ambiguous only at its split point. For each split,
`r1` and `r2` determine the generator state up to bit 31. Inverting the LCG gives
the seed, and XOR with the path term gives `time(NULL)`. Only one candidate
falls in a plausible build window.

| Image | Unit | Compiled (UTC) |
| --- | --- | --- |
| `HEROES3.EXE` | `Game\ForceFeedback.cpp` | 2000-09-01 01:26:05 |
| `h3maped.exe`, `h3ccmped.exe` | `Libraries\ResourceManager\ResourceManager.cpp` | 2000-09-01 01:35:26 (one object, linked into both) |
| `h3maped.exe` | 24 `Editor\*.cpp` units | 2000-09-01 01:35:47 – 01:41:53 |
| `h3ccmped.exe` | 6 `CampaignEditor\*.cpp` units | 2000-09-01 01:43:22 – 01:44:09 |

Older editor builds decode the same way. SoD 3.0 shows several incremental build
sessions between 1999-09 and 2000-02-29. Buka's SP5 rebuild compiled every
unit on 2003-02-27.

## Consequences

- A byte-identical link must compile each anonymous-namespace unit from its
  retail path spelling (for example `C:\Dev\Heroes 3 Exp 2\Editor\MapDoc.cpp`),
  with the clock frozen at the recovered second. Comparison-time normalization
  (`config/retail/anon-ns-paths.tsv`) remains the right tool for scoring.
- The compile seconds give the order of the original batch build, and their
  gaps bound how much was compiled in between. In the map editor they form two
  alphabetical runs (`BlackBox…`–`ToolkitBase`, then `ObjectType`–`VictoryCondition`).
  The linked object order, measured from vtable addresses in `.rdata`, is a
  single case-insensitive alphabetical sequence.
