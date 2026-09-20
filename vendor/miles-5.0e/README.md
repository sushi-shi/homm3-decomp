# Miles Sound System 5.0e

`orig/Mss.h` preserves the complete public 5.0e header unmodified. It comes
from the Jagged Alliance 2.5 repository at commit
`9503575b46c8587f73cdf40eeb2844f03b120d54`.

`include/Mss.h` starts as a byte-for-byte copy. `Mss.h.patch` maps the 31
public `AIL_*` identifiers imported by the game to the leading-underscore
exports in the shipped DLL. It also supplies an opt-in sample-tag hook for the
game's retail-proven `class ds_memsample` C++ boundary; the default remains the
public 5.0e `struct _SAMPLE` interface. Layouts, calling conventions, parameters,
and return types remain those of the 5.0e public header.

Source: <https://github.com/gondur/jagged-alliance-2.5/blob/9503575b46c8587f73cdf40eeb2844f03b120d54/Build/Standard%20Gaming%20Platform/Mss.h>

- Original header SHA-256: `ba5f4b3c7c08dffa61e61e5a6e0317c7a0b3ac00017e0a385da4b901efd41510`
- Active header SHA-256: `f67f8c57c8420d4f68d4b32d16933310e379069e3913802064380728c143cadd`
- Runtime: `build/vendor/runtime/MSS32.DLL`, version 5.0e
- Imported declarations: 31/31 exact decorated matches
- Original import library: unresolved; the retail IAT hints differ from the
  shipped DLL EAT hints
