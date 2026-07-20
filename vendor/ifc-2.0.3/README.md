# Immersion Foundation Classes 2.0.3

`orig/` preserves the complete 26-header IFC 2.2.11 SDK header set unmodified.
It comes from the Jedi Academy source repository at commit
`d71d53e8ecc1edd300c7a9dd22b8fbc39c095423`.

`include/` starts as an exact copy of all 26 headers. `IFC.patch` changes only
the three interfaces whose decorated exports differ in the shipped IFC 2.0.3:

- `CImmMouse::Initialize(void *, void *, unsigned long)`;
- `CImmEnclosure::SetRect(tagRECT const *)`; and
- `CImmEnclosure::Start(unsigned long)`.

The other 23 interfaces imported by the game remain unchanged from IFC 2.2.11.

Source: <https://github.com/grayj/Jedi-Academy/tree/d71d53e8ecc1edd300c7a9dd22b8fbc39c095423/code/ff/IFC>

- Original `IFC.h` SHA-256: `75bf753546a006f1e5ec74e3f05d7609197f712a566b8089305f54d4a4b4f3a2`
- Active `IFC.h` SHA-256: `75bf753546a006f1e5ec74e3f05d7609197f712a566b8089305f54d4a4b4f3a2`
- Runtime: `build/vendor/runtime/IFC20.DLL`, version 2.0.3
- Imported declarations: 26/26 exact decorated matches
- Original IFC20 import library: unresolved; IFC22.lib is not substituted
