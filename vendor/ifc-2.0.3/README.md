# Immersion Foundation Classes 2.0.3

`orig/` preserves the complete 26-header IFC 2.2.11 SDK header set unmodified.
It comes from the Jedi Academy source repository at commit
`d71d53e8ecc1edd300c7a9dd22b8fbc39c095423`.

`include/` is the active IFC 2.0.3 boundary. `IFC.patch` replaces the newer
umbrella `IFC.h` with the exact class surface used by Heroes III, reconstructed
from IFC20.DLL 2.0.3 layouts, its decorated exports, and the game's two client
vtables. The patch also records the three callable interfaces whose decorated
exports differ from IFC 2.2.11:

- `CImmMouse::Initialize(void *, void *, unsigned long)`;
- `CImmEnclosure::SetRect(tagRECT const *)`; and
- `CImmEnclosure::Start(unsigned long)`.

The active `IFC.h` covers all nine vendor types the game names, including the
older `CImmDevice`, `CImmEffect`, `CImmEnclosure`, and `CImmProject` layouts.
The other 25 active files remain copies of the preserved set except for the two
companion headers containing those three signature corrections.

Source: <https://github.com/grayj/Jedi-Academy/tree/d71d53e8ecc1edd300c7a9dd22b8fbc39c095423/code/ff/IFC>

- Original `IFC.h` SHA-256: `75bf753546a006f1e5ec74e3f05d7609197f712a566b8089305f54d4a4b4f3a2`
- Active `IFC.h` SHA-256: `169cc3688f6bfc36ecd43576affdc8ee0801708b67c386b23af2ed36523f6497`
- Runtime: `build/vendor/runtime/IFC20.DLL`, version 2.0.3
- Imported declarations: 26/26 exact decorated matches
- Original IFC20 import library: unresolved; IFC22.lib is not substituted
