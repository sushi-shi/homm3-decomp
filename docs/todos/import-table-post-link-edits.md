# Import table post-link edits

The retail import tables are completely bounded by the PE import format, but
part of them cannot be reproduced from any pinned VC6 LINK input. The
evidence points to an edit of the shipped executable after linking. Until
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
  - All 102 KERNEL32 and 56 USER32 imports carry hint 0, while the pinned import
    libraries give real hints. The other DLLs' hints match their libraries.
  - The KERNEL32 DLL name is spelled `KeRNeL32.dll`; the pinned library spells
    it `KERNEL32.dll`.
  - All 14 descriptors carry TimeDateStamp `0xad2b0000`. LINK writes zero here.
- **Still `missing`, 14 bytes at `0x25d91b`.** The string `GetSysteminfo\0`
  has no hint word, and no lookup-table entry or code word refers to it. It
  is not linker output.

As supporting evidence, the bink, smack and IFC hints agree with the export
tables of Steam's runtime DLLs, but mss32's do not. Those DLLs are not pinned,
so this does not verify anything.

## To decide

1. Identify what edited the import tables. Check whether one known tool or
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
3. If the missing import libraries (mss32, binkw32, smackw32, IFC20) can be
   pinned, verify their hint and name records the same way as the other DLLs.
