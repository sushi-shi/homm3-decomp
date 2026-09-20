# Bink 0.5a

`orig/bink.h` and `orig/Rad.h` preserve the available Bink 1.0a header set
unmodified. They come from the Daikatana source repository at commit
`5c7980f393aadd8abd84f0ffb417e42991e3dfa9`.

`include/` starts as an exact copy. `bink.h.patch` adapts it to the shipped
0.5a ABI: the version/date and the complete `BINK`, `BINKIO`, `BINKSND`, and
`BINKSUMMARY` layouts. The layouts and names come from the Dreamcast build's
CodeView records; the Windows executable and DLL independently confirm every
field and public SDK call consumed by the game.

Source: <https://github.com/DeathEngine2/daikatana/blob/5c7980f393aadd8abd84f0ffb417e42991e3dfa9/4-6-2000%201.0%20Gold/user/bink.h>

Unchanged declarations and layouts remain the later 1.0a SDK baseline. APIs
outside the game's IAT must still be checked against the shipped DLL before use.

- Original header SHA-256: `6380df39ec6484ccad17caf0e8a3b2565671f2f0c1bcbff31ba86493b0775262`
- Active header SHA-256: `e213694f8790b96f3e0b697891b2eb2a85ddef6bcda89c01cddb4c47eb286e7c`
- Runtime: `build/vendor/runtime/BINKW32.DLL`, version 0.5a
- Imported declarations: 13/13 exact decorated matches
- Original import library: unresolved
