# Bink 0.5a

`orig/bink.h` and `orig/Rad.h` preserve the available Bink 1.0a header set
unmodified. They come from the Daikatana source repository at commit
`5c7980f393aadd8abd84f0ffb417e42991e3dfa9`.

`include/` starts as an exact copy. `bink.h.patch` adapts it to the shipped
0.5a ABI: the version/date, the 13 imported DLL identifiers, and the complete
`BINK`, `BINKIO`, `BINKSND`, and `BINKSUMMARY` layouts. The layouts and names
come from the Dreamcast build's CodeView records; the Windows executable and
DLL independently confirm every field and import consumed by the game.

Source: <https://github.com/DeathEngine2/daikatana/blob/5c7980f393aadd8abd84f0ffb417e42991e3dfa9/4-6-2000%201.0%20Gold/user/bink.h>

Unchanged declarations and layouts remain the later 1.0a SDK baseline. APIs
outside the game's IAT must still be checked against the shipped DLL before use.

- Original header SHA-256: `6380df39ec6484ccad17caf0e8a3b2565671f2f0c1bcbff31ba86493b0775262`
- Active header SHA-256: `b2da1d43413d2bc153c6cb383f2739ea0bae7c3ec78a95cdd8df8e6ada903244`
- Runtime: `build/vendor/runtime/BINKW32.DLL`, version 0.5a
- Imported declarations: 13/13 exact decorated matches
- Original import library: unresolved
