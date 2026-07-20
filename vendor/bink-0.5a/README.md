# Bink 0.5a

`orig/bink.h` and `orig/Rad.h` preserve the available Bink 1.0a header set
unmodified. They come from the Daikatana source repository at commit
`5c7980f393aadd8abd84f0ffb417e42991e3dfa9`.

`include/` starts as an exact copy. `bink.h.patch` changes only `BINKVERSION`
to the shipped 0.5a and `BINKDATE` to the DLL's 20 January 1999 PE build date.
All 13 calls imported by the game exist in this public header with the exact
stdcall argument sizes found in the executable and DLL.

Source: <https://github.com/DeathEngine2/daikatana/blob/5c7980f393aadd8abd84f0ffb417e42991e3dfa9/4-6-2000%201.0%20Gold/user/bink.h>

The retained layouts and declarations are still the later 1.0a SDK baseline;
the version patch does not claim they are all proved for 0.5a. APIs outside the
game's IAT must be checked against the shipped DLL before use.

- Original header SHA-256: `6380df39ec6484ccad17caf0e8a3b2565671f2f0c1bcbff31ba86493b0775262`
- Active header SHA-256: `4c1ef6e966335729af858646f78983a9da5ff8e189706be71f7ff01202121dcc`
- Runtime: `build/vendor/runtime/BINKW32.DLL`, version 0.5a
- Imported declarations: 13/13 exact decorated matches
- Original import library: unresolved
