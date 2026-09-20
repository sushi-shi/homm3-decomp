# Focused platform-exclusion review

Review baseline: `0e3723c3` (PR #62). The focused lists contain 507 DC
and 720 Windows entries. This is a triage of their reasons and selected
source/binary evidence, not a fresh disassembly audit of all 1,227 entries.
No additional missing retail function or new byte match is established here.

## Confirmed: campaign constructor argument history was reversed

The `TCampaignWindow::TCampaignWindow` reasons in
[`dc_only.tsv`](../config/dc_only.tsv) and
[`win_only.tsv`](../config/win_only.tsv) contradicted each other. The former
claimed a new leading reset flag and a preserved campaign selector.

DC `0x5b570`, `campaignwindow.cpp:86`, has one explicit `int newCampaign`
argument. It is saved from R5 into R9 at `0x5b57e` and tested at `0x5b5c0`
(source line 91), guarding `SCampaign::clear` at source line 93. Its role is
the reset flag. Retail `0x45ea40` reads the first argument as a byte at
`0x45ea7a`, tests it at `0x45ea8c`, and reads the second argument as a dword
at `0x45ebb5` for campaign-set selection. The normal return uses `ret 8`.

Both reasons now describe the preserved reset role and added second selector.
The Windows reason's test address was also corrected from `0x45ea88`.
The current C++ already implements these roles; no code change is needed.
The original Windows spelling and bool-versus-byte type remain unresolved.

## Open: seer-list helpers may have been excluded too broadly

Relevant entries: DC `TSeerHut::SaveSeerList` / `LoadSeerList`, and Windows
`NewfullMap::loadSeerList`. See
[`mapcell.cpp`](../src/mapcell.cpp) and [`seerhut.cpp`](../src/seerhut.cpp).

DC proves static list helpers, not just repeated serialization statements:

| Helper | DC address | Owning source layout |
| --- | --- | --- |
| `SaveSeerList` | `0x12d7e8` | `seerhut.cpp:481..499`, nine recorded rows |
| `LoadSeerList` | `0x12d854` | `seerhut.cpp:503` boundary |

Save has a `short short_buffer`, a two-byte count write, a vector loop and
per-record result checks. Retail `NewfullMap::save` (`0x4fdf40`) still has
the count write and seer loop, selecting the map's `+0x60` vector; it does
not check each record's save result. The current source writes two bytes
from an `int` count and spells the loop directly in `NewfullMap::save`.
Load now has a reconstructed map member helper with a short wire count.

The exclusion reasons correctly reject the old implicit-global-pool
interface. They do **not** prove that the operation lost its helper boundary,
that it moved out of `TSeerHut`, or that it ceased being static. A static
helper receiving the map explicitly can express the changed pool ownership.
Likewise, retail expansion cannot by itself settle the original owner.

Follow-up: compare a canonical static `TSeerHut` helper with an explicit map
argument against the current member/flattened forms, preserving Complete's
versioned records, quest registration and actual error handling. Check the
DC-proven short count declaration against retail too. Inspect load and save
together and measure VC6 caller/helper collateral. This is a recovery lead,
not a demonstrated missing standalone address or authorization to transplant
the old global implementation unchanged.

## Confirmed evidence gap: raw primary-skill copies do not prove new accessors

Windows entries `hero::copyPrimarySkills` in
[`events.cpp`](../src/events.cpp) and `hero::setPrimarySkills` in
[`remote.cpp`](../src/remote.cpp) admit provisional ordinary helpers. Their
reasons relied on raw packet bytes and inheritance from a previous branch;
neither establishes a source member boundary.

The receive operation already exists in DC `CLevelPickWaitDlg::OnHeroLevelUpdate`
at `0x11e894`. Source line 2578 copies 28 secondary-skill bytes; line 2579
copies four primary-skill bytes from packet offset 52; line 2581 restores
the secondary-skill count. At `0x11e8c8` the copy length is explicitly four.
Thus avoiding the clamped accessor is shared behavior, not evidence of a
Windows-only operation. The `setPrimarySkills` reason now records this
positive counterpart and separates it from the inferred interface.

Follow-up: establish the accessor boundaries from retail callers and compiler
experiments, or retain their provisional status. The observed DC memcpy
does not rule out an expanded accessor; absence from the procedure search
does not prove absence from source. No replacement C++ is adopted here.

## Lower priority: the focused DC list still contains generated noise

For example, `type_record_shroud` construction/destruction and the
`type_record_move_hero` / `type_record_teleport` destructors explicitly say
the current implicit C++ definitions already provide them. These remain in
the focused list despite the generated-list split. They are candidates for
moving intact to `dc_only_generated.tsv`, not missing game logic.

Palette attachment was also checked: `addPal16` and `addPal24` already have
canonical bodies beside `ResourceManager::getSprite` in
[`resourcemanager.cpp`](../src/resourcemanager.cpp), with paired moved-owner
entries. Their old `csprite.cpp` exclusions do not hide missing helpers.

## Reproduction

```sh
homm3 dreamcast show dc:0x5b570
homm3 dreamcast asm dc:0x5b570 --blocks
homm3 sema disasm 0x0045ea40
homm3 dreamcast show dc:0x12d7e8
homm3 dreamcast show dc:0x12d854
homm3 sema disasm 0x004fdf40
homm3 dreamcast show dc:0x11e894
homm3 dreamcast asm dc:0x11e894 --blocks
```

`show` includes source-line layout. Retail disassembly uses delinked object
offsets; convert with the function's displayed start offset and retail VA.
Generated disassembly stays in ignored `build/`; this review changes only
documentation and exclusion reasons, not membership or recovered C++.
