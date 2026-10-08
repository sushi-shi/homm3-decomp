# Import table and executable anomalies: provenance unresolved

The retail import tables are completely bounded by the PE import format, but
part of them cannot be reproduced from any pinned VC6 LINK input. The
evidence suggests editing after linking, but the responsible tool, release and
relationship between the anomalies below have not been established. Until
this is explained, `homm3 verify data-coverage` reports those bytes in a
separate `import-structure` category rather than as verified linker output.

## Current accounting

All addresses are RVAs in the pinned `HEROES3.EXE`.

- **Extent.** The IAT is at `0x23a000..0x23a480`, and the rest of the import
  data is at `0x25bd70..0x25d91b`: 14 descriptors plus the null descriptor,
  lookup tables and IATs with terminators, hint/name records, DLL names and
  even-alignment pads.
- **`linker-import`, 2,952 bytes.** The pinned import libraries reproduce these
  exactly: DLL spelling, hint and name, or the library's ordinal binding. The
  IAT equals the lookup table because the image is not bound, and every slot is
  referenced by a reviewed relocation.
- **`import-structure`, 5,283 bytes.** The PE format bounds these bytes, but no
  pinned input reproduces them. `data_coverage.json["linker_structures"]` lists
  the reason for each one:
  - mss32, binkw32, smackw32 and IFC20 have no pinned import libraries.
  - All 105 KERNEL32 and 56 USER32 imports carry hint 0, while the pinned import
    libraries give real hints. The other DLLs' hints match their libraries.
  - The KERNEL32 DLL name is spelled `KeRNeL32.dll`; the pinned library spells
    it `KERNEL32.dll`.
  - All 14 descriptors carry TimeDateStamp `0xad2b0000`, rather than the zero
    emitted by the pinned ordinary unbound link.
- **Still `missing`, 14 bytes at `0x25d91b`.** A zero byte followed by
  `GetSysteminfo\0` (the string starts at `0x25d91c`)
  has no hint word, and no lookup-table entry or code word refers to it. It
  has no established producer.

As supporting evidence, the bink, smack and IFC hints agree with the export
tables of Steam's runtime DLLs, but mss32's do not. Those DLLs are not pinned,
so this does not verify anything.

## Separate observations at the end of `.text`

The end of `.text`, which sits directly before the IAT at `0x23a000`, has
two unexplained extents. Neither has been reproduced from pinned link inputs;
both stay `missing`.

- **24 bytes of `strlen` at `0x2395fa..0x239612`.** The code is
  `mov ecx,[esp+4]`, a byte-scanning loop, then `sub eax,ecx; dec eax; ret`.
  It starts right after the last LIBCPMT `xlocale.obj` `.text$x` stub, which
  ends at `0x2395fa`. That address is not 4-aligned, unlike the identified
  surrounding contributions. This does not rule out an unknown contribution
  with a different alignment. The body ends exactly at `.text` VirtualSize, `0x238612`
  (`0x1000 + 0x238612 = 0x239612`). LINK sets VirtualSize to the end of the
  last contribution; the observed end is 24 bytes beyond the last identified
  library contribution. Whether the header was subsequently enlarged is
  unproven. The bytes occur nowhere else in the image and in no
  pinned library member, game object or zlib object. No relocation, absolute
  word or `call`/`jmp` refers to them.
- **3 bytes `05 43 5f` at `0x239ffd..0x23a000`.** These are the last bytes of
  `.text`'s FileAlignment tail (VirtualSize `0x239612` up to raw end
  `0x23a000`). LINK zero-fills that tail, and every other byte of it is zero.
  They end exactly where the IAT begins.

Proximity does not establish a common producer. Neither extent is attributed
to the import anomalies or the six-byte `setupCDDrive` entry replacement at
VA `0x0050c1c0` (`mov eax,7; ret`). That replacement leaves the old CD-detection
body behind, but its author, date and relationship to these observations are
also unresolved. No accounting credit follows from a common-patch hypothesis.

## Local comparison, 2026-09-30

Two additional local files were inspected read-only. Their installation paths
identify where they were found, not a verified distribution history:

| Local file | Size | SHA-256 |
|---|---:|---|
| Steam `Heroes Of Might And Magic III/Heroes3.exe` | 2,936,832 | `4bb8784e75aaf0239e85fe767333835d277c9e0de32348fab6f8516f42d66a0e` |
| Steam `Heroes of Might and Magic III - Horn of the Abyss/_HD3_Data/Heroes3.exe` | 2,826,240 | `60f0df04927e17df8aa4baeb387f719d8e06e88c8fca54d310ec4a390fc03224` |

Both have the mixed-case KERNEL32 name, 105 zero KERNEL32 hints, 56 zero USER32
hints, all 14 descriptor timestamps `0xad2b0000`, and the orphan spelling
`GetSysteminfo`. The first has an added `.bind` section and different on-disk
`.text` bytes; it does not provide a plain code comparison. The second has the
same 24-byte `strlen` at its different `.text` virtual end and raw tail
`05 43 77`, instead of the pinned image's `05 43 5f`.

This establishes recurrence across these local files, not which release or
tool introduced it. Neither is a known unmodified predecessor. A controlled
before/after comparison remains necessary to attribute the changes.

## European Collector's Edition comparison, 2026-09-30

The European Collector's Edition discs provide a closely related executable,
likely from a 2004 Ubisoft rerelease; Disc 1's ISO volume date is 2002-06-14.
Neither is a verified release date.

Disc 1 image archive: 419,103,047 bytes, SHA-1
`7046e7a75fc47f64ea8876af2dc83b580460136e`.
Its MODE1/2352 BIN has SHA-256
`954e247d1d1d51abf76ffb6aabc8c8a65ae55efe142e2dd7b375c9d777e6e1c8`.
After extracting the ISO payload, `unshield` extracts
`Program Files/HEROES3.EXE` from `_setup/data1.cab` without running the installer.
The executable is 2,732,032 bytes, version 4.0.0.0, SHA-256
`0da1c7772567f2ad4bf2314d07e8b617c40893c80601b39607cafdf025eb6247`.

It shares the pinned target's PE timestamp `0x39b83835`, entry RVA `0x21a2b4`,
section starts and file length. **467 byte positions differ, in 241 contiguous
spans.** The different byte positions divide into 38 header, 70 `.text`,
319 `.rdata`, 3 `.data`, and 37 resource bytes. These counts describe the whole
file comparison; they are not byte-accounting categories or a causal patch log.

| Observation | Collector's Edition | Pinned target |
|---|---|---|
| CD-detection entry, VA `0x50c1c0` | `55 8b ec 81 ec 3c` (original prologue) | `b8 07 00 00 00 c3` (return 7) |
| KERNEL32 spelling and KERNEL32/USER32 hints | uppercase name; nonzero hints | mixed-case name; zero hints |
| All 14 import descriptor timestamps | zero | `0xad2b0000` |
| `.text` VirtualSize | `0x2385fa` | `0x238612` |
| RVA `0x2395fa..0x239612` | zeros, beyond VirtualSize | 24-byte `strlen` |
| RVA `0x239ffd..0x23a000` | zeros | `05 43 5f` |
| RVA `0x25d91c` | zeros | `GetSysteminfo` |
| Default sample rate / channels | 22050 / 1 | 44100 / 2 |
| VA `0x41e223`, `0x41e35f`, `0x465d63` | `fdivr` memory operand | `fmul` with the same operand |

The pinned header also contains the 18-byte SafeDisc signature at file offset
`0xfd4`, followed by version DWORDs 1, 50, 20 at `0xff4`; the corresponding
Collector's Edition bytes are zero. This matches the signature and fallback
version layout implemented by
[BurnOutSharp's detector](https://github.com/claunia/BurnOutSharp/blob/master/BurnOutSharp/ProtectionFind.cs)
(`GetSafeDiscVersion`), yielding **SafeDisc 1.50.020**. It establishes retained
SafeDisc metadata, not an active protection check or the identity of an
unpacker. The other changed code includes early setup and additional edits
inside the retained CD-detection body; the entry replacement alone does not
describe all differences.
The three arithmetic changes are in `combatManager::chooseBallistaTarget`
(first two) and `combatManager::keepAttack` (third), respectively. They are
gameplay-code differences, not import-format changes. The pinned target remains
authoritative for their reconstruction.

This comparison establishes that the unusual extents are absent in this
closely related disc executable. It does **not** establish that GOG modified
this particular file, that all differences were introduced together, or which
tool produced them. No byte-accounting classifications have changed.
Scratch metadata, extraction outputs and the complete difference list are
under ignored `build/provenance/archive/`.

## Earlier protected Shadow of Death comparison

The USA Shadow of Death disc contains version 3.0, with separate `Heroes3.exe`,
`HEROES3.ICD` and `dplayerx.dll` in the InstallShield cabinets. Its BIN is
781,571,952 bytes, SHA-1 `fd7b273a95d23faf894e57512174c0e3b8ec27ff`; its ISO
volume date is 2000-03-11. The cabinets were
extracted without executing the installer, launcher or protection components.

`HEROES3.ICD` is 2,736,173 bytes, SHA-256
`9788c36f37bc8fd40918768f203a053132caec14611145c68a87f88ff77a9d24`.
It reports game version 3.0.0.0 and has the SafeDisc signature at `0xfd4`, with
version **1.41.000**. The launcher and `dplayerx.dll` independently contain the
same protection version signature.

Crucially, this protected image **already contains `GetSysteminfo`**, at file
offset/RVA `0x25e94c`, immediately after the import-name region. Thus this
spelling is not unique to our GOG target or to the later local Steam files.
Its import descriptors have zero timestamps and uppercase `KERNEL32.dll`;
the KERNEL32 and USER32 lookup arrays begin with zero in this protected image,
so an ordinary PE walker cannot recover their full imports. The `.text` bytes
are protected and cannot serve as a direct source-code or `strlen` comparison.
Its raw `.text` tail is zero.

This separates at least one historical observation from the unresolved
mixed-case-name, timestamp and tail-byte changes. The evidence supports a
SafeDisc-related ancestry, but does not identify a particular unpacker or
prove that all the remaining anomalies share its producer.

The Shadow of Death 3.2 patch was also examined (4,126,480 bytes; SHA-1
`0c51ef77937931313ac4c1d8fbee834dba02cabd`). It is an
RTPatch binary delta, not an independently extracted 3.2 executable, and has
not been applied. Disc 2 of the Collector's Edition was checked too: it has
`00000001.TMP`, but no additional game executable. An Apple HFS install disc
is excluded from this Windows comparison.

## To decide

1. Identify what produced the import anomalies. Check whether one known tool or
   release process explains all of the signatures above together: hint zeroing
   limited to KERNEL32/USER32, the mixed-case DLL name, the non-zero descriptor
   timestamps, and the orphaned `GetSysteminfo` name. Candidates include import
   rebuilders, protection/unpacking tools and the Complete/GOG re-release
   tooling. Compare other retail builds (Complete versions and patches, and
   non-GOG releases) where available.
2. Decide whether reproducing that edit from pinned evidence may move
   `import-structure` into a verified category, and whether `GetSysteminfo`
   becomes edit residue or remains unaccounted. The zero-unaccounted-bytes goal
   for the data campaign treats this as an open policy question.
3. Check whether the same process explains the `.text` differences: the
   unreferenced `strlen`, larger VirtualSize and three non-zero bytes at the
   end of the FileAlignment tail. The disc comparison establishes the byte
   differences, but not their order or common producer.
4. If the missing import libraries (mss32, binkw32, smackw32, IFC20) can be
   pinned, verify their hint and name records the same way as the other DLLs.
