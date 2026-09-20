# Miles Sound System 5.0e

`orig/Mss.h` preserves the complete public 5.0e header unmodified. It comes
from the Jagged Alliance 2.5 repository at commit
`9503575b46c8587f73cdf40eeb2844f03b120d54`.

`include/Mss.h` starts as a byte-for-byte copy. `Mss.h.patch` names the sample
record `class ds_memsample`, the game's retail-proven C++ ABI tag, directly in
the active header. Layouts, calling conventions, parameters, and return types
remain those of the 5.0e public header.

Source: <https://github.com/gondur/jagged-alliance-2.5/blob/9503575b46c8587f73cdf40eeb2844f03b120d54/Build/Standard%20Gaming%20Platform/Mss.h>

- Original header SHA-256: `ba5f4b3c7c08dffa61e61e5a6e0317c7a0b3ac00017e0a385da4b901efd41510`
- Active header SHA-256: `df363948615f3edf3a237756ffc2a78efa3ad81a442bdd6f7f38aa50266ff1f2`
- Runtime: `build/vendor/runtime/MSS32.DLL`, version 5.0e
- Imported declarations: 31/31 exact decorated matches
- Original import library: unresolved; the retail IAT hints differ from the
  shipped DLL EAT hints
