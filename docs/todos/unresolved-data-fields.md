# Unresolved data fields

Status: 20 unresolved declarations from the original 332-field inventory;
312 handled after evidence pass 86. This is a recovery backlog, not a claim
that all retained integer types or opaque extents are original source types.
The detailed evidence and completed recoveries are in
[remaining-field-recovery.md](../remaining-field-recovery.md).

No supported semantic name was established for these 20 from the inspected
Dreamcast and NH3API records. Some owners or neighboring members have names;
that does not identify these fields. Additional H3API checks also left them
unresolved. Original names take priority when found: retain their spelling
in the owning evidence comment, drop Hungarian type prefixes, normalize to
lowerCamelCase and apply `m_`, `s_`, or `g_` according to scope.

Offsets below are hexadecimal byte offsets unless explicitly marked as bits.
The declarations describe the current model, including its uncertainties.

| # | Owner and field | Current storage | Evidence gap and next useful check |
| :-- | :-- | :-- | :-- |
| 1 | [THeroTraits::m_pad3c](../../include/hero.h) | +0x3c, char[4] | Retail table rows have zero bytes here. NH3API establishes only three neighboring flags; H3API also leaves +0x3c unknown. Find a discriminating consumer or matching type definition; zeros do not prove padding. |
| 2 | [THeroScreenWindow::m_field64](../../include/hero.h) | +0x64, widget* | Constructor 0x4de7e6 reads vector end-1 before the first widget push. Base constructors do not populate that vector. Trace the stored pointer's origin and uses; a background/last-base-widget interpretation is unsupported. |
| 3 | [SVideoDescriptor::m_padC](../../include/smackmgr.h) | +0x0c, char[8] | Thirty inspected rows have zero tails; checked direct references do not read past +0x0b. Follow aliases or recover the descriptor definition before claiming unused storage. |
| 4 | [SBolt::m_field1c](../../include/cmbtmgr.h) | +0x1c, int | No distinguishing member consumer or reference definition established. Trace bolt allocation, initialization and drawing accesses. |
| 5 | [strip::m_pad00](../../include/strip.h) | +0x00, char[0x1c] | Five recovered retail bodies leave this span untouched. HoMM2's different strip layout cannot supply HoMM3 names. Examine external owners and obtain a matching definition. |
| 6 | [strip::m_pad30](../../include/strip.h) | +0x30, char[0x34] | Same coverage limit as the leading gap; surrounding selection fields and seven army slots do not establish this span's members. Trace external accesses. |
| 7 | [THallWindow::m_field60](../../include/townmgr.h) | +0x60, int | Allocation proves extent, not type or role. Constructor/destructor and reconstructed setupCastle provide no distinguishing access. Find an actual consumer or type record. |
| 8 | [TBlacksmithWindow::m_field68](../../include/townmgr.h) | +0x68, int | Constructor clears it, but zero initialization does not distinguish its role. Locate another access or the original member definition. |
| 9 | [TCombatPlacementSubWindow::m_pad038](../../include/combatcontrolsubwindow.h) | +0x38, char[4] | Allocation at 0x4721d0 is 0x3c versus the base's 0x38. No write identifies the extra word; hidden inheritance metadata is unproven. Recover the derived definition or a distinguishing access. |
| 10 | [TGiveResourceWindow::m_field80](../../include/tradpost.h) | +0x80, int | Follows seven recipient colors. At most seven other players are eligible, so this does not prove an eighth array element. Recover aggregate extent or a separate consumer. |
| 11 | [TCampaignBrief::m_field68](../../include/campaignbrief.h) | +0x68, int | Neighboring campaign and selected-scenario fields are identified, but inspected NH3API/DC evidence does not identify this slot. Seek a matching member record or runtime use. |
| 12 | [TSwapWindow::m_field60](../../include/swapmgr.h) | +0x60, int | Complete allocation proves a tail word; its role is unknown. H3API only forward-declares the corresponding dialog. Find a complete definition or meaningful access. |
| 13 | [TSingleSelectionWindow::m_pad358](../../include/singleselectionwindow.h) | +0x358, char[4] | H3API also calls this unknown. Its different portrait interpretation at +0x354 does not identify +0x358; retail indexes the preceding slot as heroPix[163]. Trace the remaining word separately. |
| 14 | [TRandomMapProgress::m_pad28](../../include/singleselectionpopups.h) | +0x28, char[4] | Known members fill 0x28 bytes; 0x2c is only an extent bound. Old 0x30 size overlapped a separate live local (0x5862ef, 0x586306, 0x586437). Establish whether retail sizeof is 0x28 or 0x2c before removing or naming the word. |
| 15 | [TRmgTemplate::m_opaque0020](../../include/rmg.h) | +0x20, char[0x10] | Gap between zones and size bounds; H3API also leaves it unknown. Recover its construction/destruction or member accesses to establish aggregate boundaries and types. |
| 16 | [TRmgGroundTile::m_unknown30](../../include/rmg.h) | bits 30–31, unsigned : 2 | No explicit semantic use established; H3API also leaves tail bits unnamed. Inspect masks and whole-word transformations before declaring padding or naming flags. |
| 17 | [TRmgConnectionDecoration::m_unknown05](../../include/rmg.h) | bits 5–31, unsigned : 27 | Only present/direction bits are identified. Trace producers and consumers of the remaining bits; lack of explicit accesses is not proof of padding. |
| 18 | [TRmgZone::m_opaque003d](../../include/rmg.h) | +0x3d, char[7] | Three bytes can align the next dword, but +0x40 remains unexplained. H3API does not resolve the region. Identify that word before replacing the entire gap with compiler padding. |
| 19 | [type_random_map_generator::m_opaque0ee0](../../include/rmg.h) | +0xee0, char[4] | 0x5499fb initializes nine ints; 0x549a75/0x549ab8 write entries 1–8, while readers use +0xee4. Leading sentinel role is visible, but extent through +0xf23 is unresolved. Prove the mapping's boundaries; inventing a 17-element array would hide this uncertainty. |
| 20 | [TRmgPackedTerrainCell::m_unknown14](../../include/rmg_terrain.h) | bits 14–15, unsigned short : 2 | Neighboring frame/flip fields are recovered; no distinguishing use of the final bits is established. Trace packed-word masks and serialization before naming or removing them. |

## Completion criteria

Resolve each entry with a matching original definition or retail evidence for
its semantic role, type and extent. Qualify cross-version/reference inferences
explicitly. Update declarations, definitions and uses together; preserve ABI
and vendor boundaries. Remove opaque storage as alignment only when layout
and access evidence support it. Record the evidence beside the owning source
and remove the corresponding backlog entry once verified.

Batch actual source changes for retail validation. Documentation alone needs
no build. The latest full checkpoint (pass 86) passed with 3,755 exact linked
functions, 4,406 matching rows and 91.26% executable fuzzy coverage. Historical
MAX values remain distinct from current reproducible scores.
