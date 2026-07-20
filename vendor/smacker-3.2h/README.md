# Smacker 3.2h

`orig/SMACK.H` and its included `orig/RAD.H` preserve the 3.2f public header
set unmodified. They come from the Vengeance Reloaded source repository at
commit
`368f7260440eb02efe083ad6dff6dcd350b30189`.

`include/` starts as an exact copy. `SMACK.H.patch` changes only the version
string from 3.2f to the shipped 3.2h; the DLL's embedded version string confirms
3.2h. The 11 calls imported by the game retain the public header declarations
and exact decorated argument sizes.

Source: <https://github.com/VengeanceReloaded/vr_source/blob/368f7260440eb02efe083ad6dff6dcd350b30189/Standard%20Gaming%20Platform/SMACK.H>

The complete 944-byte `SmackTag` topology independently exists in Dreamcast
CodeView and agrees field-for-field with the nearby 3.2f header. It remains
cross-architecture layout evidence until official Windows member accesses
prove every offset.

- Original header SHA-256: `a60df6bc2420c03fc482b1a2fe215b38e587bd47a0f43c75f70196b9495a4034`
- Active header SHA-256: `f9fbe068fbea7224c4abf6e32a591d94f6bd9a45c88d5d9c78a7f369e134869d`
- Runtime: `build/vendor/runtime/SMACKW32.DLL`, version 3.2h
- Imported declarations: 11/11 exact decorated matches
- Original import library: unresolved
