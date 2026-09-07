# Remaining field recovery

The starting population is the 332 declarations left by the naming pass in
[name-normalization.md](name-normalization.md). The objective includes every
one: recover missing member semantics and structure, identify genuine padding,
and distinguish original numbered names from synthetic offset labels. Unknown
storage remains unresolved until supported by reference and retail evidence.

The first eighty-six evidence passes handle 312 of the starting declarations:

- Five `sample` slots now belong to the recovered `MemorySampleStructure`
  aggregate. Dreamcast proves the `memSample` ownership and four original
  members; NH3API supplies the expanded PC layout. Retail construction,
  playback, and size reporting corroborate its offsets and roles. The buffer
  and byte count occupy the two PC-only words. The object stays 0x34 bytes.
- Five high-score record slots have retail-derived names for the player's
  name, land, score, days, and difficulty. Their original field spellings
  are not recovered. Two gaps are identified as four-byte alignment padding
  around the integer fields and the trailing cheat byte; the record remains
  0x64 bytes.
- Three numbered text-entry members were false positives in the placeholder
  census: Dreamcast explicitly calls them `CHSInputDlg::field1` and
  `CMPInputDlg::field1/field2`. Their original names are retained and their
  provenance is documented beside the declarations.
- Six map-header slots now distinguish a player roster's hero ID/name pairs
  from the hero-keyed customization map's portrait/name/player-mask records.
  NH3API supplies `heroes`, `name`, `portrait`, and `players`; retail consumers
  establish which first-word role applies. The direct member layout and
  four-byte portrait storage remain as proven in the existing reconstruction.
- Six slots cover mouse `CurrentX`/`CurrentY` (Dreamcast names, relocated in
  the PC layout), recruitment accept/maximum buttons and quantity slider,
  and the bolt's minimum Manhattan distance during its final approach.
  The last four names come from retail behavior.
- Twelve swap-dialog slots now identify its chat transcript/edit, left/right
  arrows, receive button, four army-transfer selection indices, previous and
  owned network handlers, and the exit-request latch. Constructor resource
  names, stack-transfer accesses, handler installation/restoration, and the
  trade-completion/player-drop message arms establish these roles.

- Five slots cover the chat editors' activation bytes and trailing alignment,
  plus the game's black-market vector. Dreamcast supplies `activated` for
  CGameChatEdit and `BlackMarkets` for game; the combat counterpart has the
  same retail activation behavior, and NH3API confirms the PC market offset.
- Four overview slots now identify the item category/count record and the
  window's flaggable-item and count-label vectors. Construction, icon display,
  scrolling, and rollover descriptions establish the roles; original member
  spellings remain unavailable.
- Seven mine/sign placeholders now use the original `is_abandoned`,
  `mapX/mapY/mapZ`, and `hasText` names, normalized to the agreed convention,
  and identify two genuine mine alignment bytes. The three coordinate union
  aliases were removed in favor of the already-present semantic members.
  Dreamcast supplies the names, NH3API confirms the expanded PC mine layout,
  and retail loaders/consumers corroborate their roles. Sign's existing text
  member also adopts the original `signText` name; that additional rename is
  outside the 332-entry count.

- Sixteen slots recover the quest's `limit` (NH3API plus retail expiration
  checks) and fifteen alignment gaps in quest, creature/artifact/hero traits,
  boat and obscuring-object records, garrison/generator records, and the
  hero-placeholder/random-dwelling records. Evidence distinguishes the real
  byte/short fields from NH3API's wider facade declarations. The four-byte
  unknown at THeroTraits +0x3c remains unresolved; it is not alignment.
- One popup placeholder exposed a shifted reference mapping: Dreamcast's
  `exitId`, `exitCodeX`, and `exitCommand` belong at retail +0x50/+0x54/+0x58.
  Both versions of ExitDialog prove that correspondence. The unsupported
  union with the saved popup-state byte was removed; that byte remains at
  +0x5c with natural class alignment to 0x60.
- One music-table placeholder was a duplicate view of a known track pointer.
  Retail cell 0x66c218 points to 0x66c090, the same table whose loader fills
  `SCampaignMusicCue::track`. Both consumers now use that canonical record.
- Two swap-manager bytes identify the human-player trade handshake and its
  giving/receiving phase. Constructor predicates, GiveMeStuff handling, arrow
  direction, and modification guards establish these role-derived names.

- Three opaque slots now have recovered structure: the preferences gap
  contains Dreamcast's two 13-byte DOS driver names and Redbook flag; the
  DirectDraw prefix contains its FOURCC and RGB bit-count words; and the
  shifted quick-view row was removed in favor of the canonical help table's
  right-click member. Retail corroborates the preferences boundaries, calls
  GetPixelFormat on the SDK layout, and fills both help-text columns. This
  also removes the incorrect second use of the object-name array's symbol.
- Three map-object gaps are now documented as genuine alignment: CObject's
  byte before TypeID and its trailing byte, plus the upper sixteen bits of
  NewmapCell's road allocation unit. Dreamcast and NH3API agree on the gaps;
  the retail-proven bitfield representation remains intact.

- Four TownExtra placeholders disappear with the duplicate partial view.
  The loader's complete record now owns the original class name and is
  shared by town initialization: real armyGroup, custom-name flag/string,
  full town-type word, grouping byte, and two spell bitsets. The two true
  alignment gaps remain implicit. The pointer-union bridge is removed.
  This also types the formerly byte-array object reference as a dword.
- Nine gaps in victory/loss conditions, border and resource-display widgets,
  normal-dialog data, and the resource cache tree are confirmed as alignment
  by the PC reference layouts, Dreamcast members, and VC6 XTREE declarations.
  The victory-condition +3 comment now correctly identifies the artifact
  dword, correcting an earlier copy of the mine-alignment explanation.

- Eleven widget/chat/network gaps are confirmed as alignment: the slider's
  last-focus boundary, editor and handler tails, the window manager's fizzle
  pointer boundary, the adventure window and split dialog, and four chat
  record/manager boundaries. Complete's extra chat-sample handle explains
  the changed offsets; no additional semantic bytes are hidden in the gaps.
- Four creature-offering fields identify the source/offering selection
  frames, creature-count text, and experience text. The grid builder proves
  iconWidget pointers for both frames; the updater proves the count and
  sacrifice-experience formulas. These names describe retail behavior;
  original member spellings are unavailable.
- Four boat-event fields now identify the replay and undo occupancy flags
  and occupying hero IDs. Constructor snapshots, replay, undo, and the
  versioned serializer establish the pairs. The previous coordinate labels
  were wrong; the loader's temporary also now reflects the hero-ID role.

- Eight dialog slots now identify Dreamcast's topTown, thisHero, lastActive,
  and currentCampVideo; retail-derived resourceDisplay, selectedRecruit,
  and recipientCount; and the campaign availability array's alignment gap.
  The unused campaign pointer-sized span remains unresolved: its possible
  Dreamcast correspondence has no reconstructed retail consumers.
- Nine town-manager placeholders now use Dreamcast's panorama, command,
  garrisonStrip, heroStrip, currIndex, srcStrip/srcIndex, and destStrip/
  destIndex. Retail background construction, selection, and army transfer
  consumers corroborate the names. The already named selectedStrip also
  becomes the original currStrip, without changing the placeholder count.

- Nine selection-window entries now identify original textIndex, the
  random-map selection flag and option array, the description widget,
  and five alignment gaps. Retail consumers prove the semantic roles;
  Dreamcast byte declarations corroborate the alignment boundaries.
  The sortDirection-adjacent gap remains unresolved because the reference
  declares a wider field there.
- Five adventure-manager gaps align advCommand, flagFrame, cursorType,
  showMode, and bottomViewType after the reference-proven byte fields.
- The game's Grail warning latch now uses original bGrailAsked normalized
  to grailAsked, and its following gap is vector alignment. Complete's
  extra spellDisabled array explains the shift from Dreamcast +0x4a to
  retail +0x90; the hall click handler proves the shown-once behavior.

- Three combat-manager placeholders now identify original OnNativeTerrain
  and SideSurrendered, and the retail-derived hasAngelicAlliance flag.
  This pass also corrects three earlier reference-name assignments:
  +0x132a8 is SummonedElemental, +0x132b0 is SideRetreated, and +0x132b4
  is gbThisNetHasControl. Summoning, retreat/surrender actions and local
  command-control predicates prove these distinctions. These corrections
  do not subtract extra entries from the starting placeholder population.

- Three combat rendering placeholders now form the original
  cmbtHeroFlagFrame[2], bHeroEffect[2], and bFlagEffect[2] arrays.
  Retail initialization and side-indexed drawing/animation accesses prove
  the boundaries. The frame accesses now index an actual array instead
  of performing pointer arithmetic across separate scalar fields.

- Ten combat-manager gaps are identified as alignment before cells,
  saved-screen storage, the enchanter counters, selector index, spell
  sprite, background-name pointer, extent control, turn number, archer
  records, and original hero stats. Retail boundaries and Dreamcast byte
  declarations distinguish these gaps from the two larger opaque blocks,
  which remain unresolved.

- The combat tail buffer is split into original any_action_taken and a
  retail-only 187-byte obstacleAttackVisited grid. ResetRound and action
  dispatch prove the flag; the obstacle workers prove the per-hex grid.
  The preceding byte is corrected to original auto_retreat_on, corroborated
  by the AI retreat confirmation path. No layout or current scores change.

- Ten anonymous army padding spans disappear with the duplicate overlay
  on spellInfluence. All 26 scalar aliases now index the original array;
  the header preserves their index mapping. This makes the army copy
  constructor (0x437a00) exact, while doCombat (0x4ad470) falls to 98.1758%.
  Disposable island search recovers the prior doCombat score with
  declarations before TU includes; the later palette consolidation
  recovers the prior score in production.

- Five game-state gaps align campaign state, the post-Grail dword, setup,
  signs, and rumours. Retail field boundaries and Dreamcast scalar/array
  widths distinguish the gaps from adjacent semantic data.
- Five AI puzzle-tile placeholders are unused bitfield tails and byte
  alignment. Dreamcast's complete 16-byte record and retail constructor
  masks agree on the occupied bits and the 0/4/8/12-byte field groups.

- Six color-table placeholders disappear with the duplicate SUnnamed6aacb0
  view. Retail initialization at 0x4ee430 stores GetPalette's TPalette16
  result in the pointer; the old prefix and gaps therefore belong to
  resource and the palette array. All color readers now index the existing
  TPalette16 directly. This naturally recovers doCombat to its starting
  98.5379%, with the army copy constructor remaining exact.

- Six network-message/request gaps align integer payloads after byte
  flags, or the header-info base object before its derived version string.
  Retail constructors, fixed message sizes and the request-vector stride
  prove the boundaries; Complete added flags absent from older DC messages.

- Five gaps align the diff header, wall-trait pointers and record extent,
  obstacle damage, and the obstacle vector's first pointer. Reference
  widths and retail serialized/table strides corroborate these gaps.
- TFontSpec::pad was already an original Dreamcast member name at +7,
  aligning numpal at +8. Its normalized m_pad spelling is retained and
  its provenance recorded; it is handled without a gratuitous rename.

- Ten dialog gaps align integer/pointer fields or the garrison-window
  base extent: high scores, castle, garrison, combat creature panel,
  sacrifice, main menu, army view, puzzle, combat options and system
  options. Byte field widths and the next retail offsets prove these
  boundaries; larger untouched dialog blocks remain unresolved.

- Two hexcell gaps align attributes after seven geometry shorts and the
  corpse count after three byte fields. Dreamcast and retail offsets agree.
  The large tail remains unresolved: Dreamcast's obstacleLimitData and
  cloudLimitData suggest a split, but reconstructed retail readers do not
  yet establish it.

- Three AI-record gaps align the combat-value estimate, embedded estimate,
  and melee-enemy array. The same pass restores twelve original semantic
  names, including rounds_left (used against spell duration), estimate,
  enemy_caster, and the friendly/enemy value pairs. Those corrections
  update declarations and uses but do not subtract extra placeholders.

- Three campaign gaps align prologue, hero-status integers, and campaign
  music after byte/boolean fields. The retail loaders distinguish the
  serialized values from these in-memory alignment gaps.

- LODHeader::reserved is an original 80-byte reference field at +12,
  corroborated by the retail constructor and 0x5c-byte header extent.
  Its spelling is preserved. This pass changes evidence comments only;
  the preceding full-build results still apply to identical C++ tokens.
  The external CImmProject fields remain unresolved because the bundled
  newer SDK layout differs from the 16-byte retail object.

- Three final-byte gaps align CDPlay, CNetPlayerHandlerPlayer, and the
  creature-bank level table. Reference byte widths, retail constructor
  stores and record strides establish the tail boundaries.
  IFC20.DLL was not present at the documented runtime path or elsewhere
  under the HoMM3 workspace; its private fields remain unresolved.

- Four map-view placeholders disappear with AdvMapCellObjectsView and
  AdvFullMapObjectsView. Renderer accesses now use canonical NewmapCell,
  its TObjectCell, and NewfullMap vectors at the same retail offsets.
  This also removes the separate renderer-only object-cell type.

- Five RMG gaps align terrain tile/rule tails, object-property entrance
  coordinates, the map-object extent, and the map-items pointer. The
  surrounding byte fields and retail record boundaries establish padding;
  the remaining unknown semantic bytes are not renamed by this pass.

- The selection-window field at +0x189c is the town-heading widget ID.
  Construction, column placement, town-choice controls, and advanced-pane
  show/hide consumers establish the role-derived townHeadingId name.

- Two video descriptor bytes now identify fadeInSecondTrack and
  noFrameSkip. Both frame pumps prove the transition fade; Bink's
  BINKNOSKIP constant proves the open-flag role. The parallel Smacker
  flag's misleading local audio-only label is corrected accordingly.

- One Bink handle gap is replaced by eight unsigned 32-bit members from
  Dreamcast's older BINK type: FrameRate, FrameRateDiv, ReadError, OpenFlags,
  BinkType, Size, FrameSize, and SndSize. The sequence fills +0x10..+0x2f
  between retail-confirmed FrameNum and FrameRects. The newer bundled SDK
  corroborates names/types but has additional earlier members; its offsets
  are not copied. This pass leaves current matching scores unchanged.

- Two Smacker handle gaps are replaced by the older Dreamcast SmackTag
  prefix: frame timing/type, seven-track size/type arrays, decoder table
  sizes, palette state and the original 772-byte palette, plus FrameSize
  and SndSize. Retail anchors the surrounding Frames, FrameNum and LastRect
  fields at the same offsets. This pass leaves current scores unchanged.

- Two gaps belonged to the unused TLargeObstacleInfo duplicate of
  SElevationOverlay at retail 0x0063bec0. Dreamcast PlaceLargeObstacle names
  sElevationOverlay; retail placement and drawing share the 68-byte rows.
  Removing the duplicate leaves x/y and FileName in their canonical owner.
  That owner now includes the PC second terrain mask and the proven 25-entry
  blockedSquares array (the prior 26 included alignment bytes). Current
  scores remain unchanged; the existing placement extern aliases remain.

- Two combat-manager gaps now contain the original iconWidgetWL and
  textWidgetWL arrays (25 typed pointers each), iTtlCombatDirections and
  iBackgroundFrame. Dreamcast proves names and types; NH3API confirms the PC
  offsets, bounded by retail-confirmed window/direction/cursor/death fields.
  Names follow the agreed scope and casing rules. Current scores are unchanged.

- Six random-map request fields now identify human/computer player and
  team counts, waterContent and monsterStrength. Retail bridge 0x54bf60
  passes the slots to generator constructor 0x537b10, whose stores tie them
  to the generator's established fields and consumers. The former local
  names humans/teams/monsters/water were misleading and are corrected too;
  monster strength is converted by clamp(request+3,1,5). Original request
  spellings are unknown, so these semantic names follow the generator.

- One generator gap is nextObjectId, a four-byte counter initialized to
  one by 0x537b10. Four retail creation paths take/increment it and store
  the taken value in the new object's derived data. This is a behavioral
  name; no original spelling is recovered. Current scores remain unchanged.

- One hexcell tail gap now contains the original obstacleLimitData and
  cloudLimitData SLimitData records. Dreamcast and NH3API agree on +0x50
  and +0x60; retail confirms the preceding +0x4d byte and 0x70 stride.
  The two preceding alignment bytes are implicit. The RMG mapping sentinel
  at +0xee0 is documented but remains unresolved pending its full extent.

- One game field recovers the original viewFrame name. Dreamcast InitVars
  at 0xe3a04 (kb.cpp:3771) zeros game +0x32010; retail's expanded InitVars
  zeros +0x4e678 between the same neighboring resets and setup filename
  copy. CodeView names the member viewFrame and types it int. The rename
  preserves current matching scores.

- Two generator gaps are parts of the owning templateName string at
  +0x10c0 and templates vector at +0x10d0. Retail assigns the chosen
  TRmgTemplate name with basic_string::assign, destroys it with _Tidy,
  and destroys the vector's objects with TRmgTemplate::~TRmgTemplate.
  The map-header writer now calls c_str() instead of reading a partial
  buffer-pointer view. Current matching scores are unchanged.

- One connection-record gap contains four integer player-count limits:
  minimum/maximum human players and minimum/maximum total players. Retail
  reads spreadsheet columns 81..84, compares the ranges against generator
  counts, and inserts the 28-byte record only when both ranges admit it.
  The byte before the integers is implicit alignment. Names follow the
  equivalent template-zone limits; current matching scores are unchanged.

- Partial recovery of TRmgZone's +0x3d gap replaces 944 opaque bytes
  with objectCountByType[232] and a vector of signed-short zoneDistances.
  Retail construction, placement/removal counters, per-zone limits, and
  shortest-path relaxation prove those roles and types. Seven bytes at
  +0x3d..+0x43 remain unresolved, so this original declaration stays in
  the remaining count. writeMapHeader improves from 95.6987% to 95.7066%;
  other current scores are unchanged.

- Partial recovery corrects unknownPointers: retail loads rand_trn.txt
  into 76-byte values, not pointers. The container is now randomTerrainEntries,
  a vector of records holding a row index, ten integers, and two integer
  vectors. Loader and destructor independently establish offsets/strides.
  The column and pairwise-value meanings remain unresolved, so the original
  item stays in the remaining count despite its corrected outer name/type.

- One adventure-manager placeholder recovers original animFrame at +0xfc.
  The existing +0x100 member was mislabeled animFrame; Dreamcast and NH3API
  both identify it as animCtr. Both identities and all uses are corrected
  together, retaining retail-proven integer widths and animation accesses.
  Current matching scores are unchanged.

- One combat-manager byte recovers CastleAttackDone from the same
  InitNonVisualVars reset in Dreamcast and retail. This also exposes six
  shifted reference-name assignments: the PC highlighter pair, lastCellIndex,
  lastMoveToIndex, lastCommand and combatCommand now match their actual
  consumers and the Dreamcast names. Retail widths are preserved; the
  castle-attack latch is a byte, not NH3API's misassigned combatCommand int.

- Two type_monster_data fields are implicit eight-byte alignment: the
  gap before the doubles and the tail after total_value. Dreamcast and
  NH3API agree on the member layout and 72-byte record size. Ten surrounding
  names are restored to their originals, including value/total_value:
  retail computes these from baseFightValue, so the former hit-point names
  were misleading. Current matching scores are unchanged.

- One selection-window gap is confirmed as alignment after sortDirection.
  Retail construction, sorting and toggles all use one byte at +0x36c,
  unlike Dreamcast's int. The apparent dword access is to a text-resource
  string table, not the window. The three bytes align currentIndex at
  +0x370; current matching scores are unchanged.

- Nine more declarations are resolved by integrating decomp-complete-4.0 at
  `60ceaa60`. The rand_trn.txt loader (0x536560) and placement scorer
  (0x536bc0) establish the terrain scores and adjacent/blocked rule vectors;
  the partial TRmgRandomTerrainEntry is replaced by TRmgObjectPlacementRule.
  Prototype references now identify the preferred terrain, placement rule,
  outline vector, 8-by-6 overlap priorities and initialization flag. The
  five object bytes track covering, behind, adjacency, overlap and blocking
  during placement scoring. Terrain repair identifies the rule byte that
  permits separated neighbours. These names describe retail roles; no
  original Dreamcast RMG spelling is claimed.
  The stronger upstream point/vector and map-view interfaces are preserved,
  while the earlier object-count and zone-distance evidence remains intact.

- Two declarations resolve the path-cell stopping flags and recruitment's
  addIndex. Dreamcast PushPoint copies can_stop into last_can_stop; retail
  performs the same operation one bit higher, proving that the PC-only bit
  precedes both. That extra bit marks a starting adventure-map trigger, so
  all three names and their uses are corrected together. Recruitment's old
  byte buffer straddled alignment and the addIndex integer at +0xa0;
  Dreamcast and NH3API agree, with retail anchors at +0x9c and +0xa4.
  Three previously named recruitment buffers also regain their proven
  types: CurrentSpriteFrame is four ints, availSource is int*, and errorWin
  is heroWindow*. The full checkpoint passes with every current score
  unchanged from the decomp integration checkpoint below.

- Two declarations recover seer-hut completion provenance and the lobby's
  common game version. Dreamcast's raw TSeerHut save orders CompletedByPlayer
  immediately after QuestCompleted; retail's pre-version-28 reader copies
  that second byte into +0x12. The original name is restored. The selection
  window's +0x1898 integer caches the highest common version derived by
  intersecting seated players' feature sets. Its helper was incorrectly
  named GetPlayerCount after a different class's Dreamcast method; it is
  now getCommonGameVersion, with the old bootstrap spelling documented.
  One full build validates both changes: all 4,406 identities and every
  current score agree with the preceding checkpoint (3,753 exact).

- Nine declarations complete the legacy campaign layouts. Packing the
  Dreamcast hero member sequence after its proven 0x18-byte base reproduces
  all retail conversion anchors and the 0x462 stride: target/patrol/movement
  state, fourteen visit-flag words, spell/morale state (including the old
  identifyLevel), and the six-word AI valuation tail replace five gaps.
  Dreamcast SCampaign similarly identifies the two leading flags, eighth
  completion slot, 8-by-32 packed map-trait records plus artifact requirement,
  and assigned-carryover array. The traits pack from 36 to 27 bytes and each
  artifact requirement from eight to five, exactly filling the old 0x1b05
  gap before carryover heroes at +0x207f. The former 101-byte filename buffer
  also separates into the original 61-byte filename and two choosability
  arrays. Retail copies only seven completion flags, which remains explicit
  despite recovering the saved array's eight-slot extent. Layout checks and
  one full VC6 build pass; all current scores remain unchanged.

- Two boundary-record regions now expose the original site's point,
  previous-edge link and position-computed byte. The paired constructor
  0x5fcef0 writes point/zone/twin and initializes both ring links to self;
  splice 0x5fcf60 maintains the backward link while swapping next pointers.
  buildVertices 0x5fdb40 reads the source points, skips already-computed
  entries, and writes the resulting boundary point and flag on three
  incident edges. The byte at +0x18 leaves three natural alignment bytes
  before the existing point at +0x1c; the allocation remains 0x24 bytes.
  These names are retail-derived. One full build passes and all current
  matching scores remain unchanged.

- Two generator gaps now expose the next seer-hut prototype index, next
  available key-tent color, active-zone total and nine alignment counts,
  144 used quest-artifact bytes and the low-pool latch. Retail 0x54b834
  cycles the prototype index; 0x540f68 advances the color; 0x549bae counts
  active zones by alignment. Quest selection 0x54b490 excludes used IDs,
  latches the warning below 20 eligible artifacts, and marks the selected
  ID at 0x54b813. The byte latch leaves three alignment bytes before the
  existing water-content integer. Names are retail-derived, with original
  spellings unknown. One full build passes; writeMapHeader changes from
  93.2864% to 93.2786%, while all other 4,405 current scores are unchanged.
  Historical MAX remains preserved; this current dip still needs recovery.

- The swap manager's +0x58 word is now `m_armySelectionPending`.
  Constructor 0x5ae530 and reset 0x5ae5c6 set it to -1. The monster-click
  handler tests it at 0x5af2fc, clears it when selecting the source stack
  at 0x5af3af, and accepts the destination only in the zero-state branch
  at 0x5af40b. The selector likewise requires zero before drawing the
  source-stack border. The name is retail-derived; the original spelling
  remains unknown. Its integer type and -1/0 representation are preserved.
  One full VC6 build passes with all current matching scores unchanged.

- Three placeholders disappear with the obsolete `SUnnamed69d808` /
  `CUnnamed69d808_f0` views. Retail global 0x69d808 and accessors 0x553770 /
  0x5537a0 already belong to canonical CDPlayHeroes. Its complete base and
  members replace the opaque prefix, and Dreamcast's m_pNetMsgHandler
  names the +0xf0 pointer. CNetMsgHandler's original m_inPopup names the
  byte at +4; the former ordinal virtual slots are CheckHandleNet and
  GetAbortPopupMsg. CompleteDraw and DoQuickView now use these canonical
  classes and interfaces. One full VC6 build passes; onKeyPress and
  onKillFocus both recover 100% from 99.8868% and 99.8710%, respectively,
  with every other current score unchanged and no compiler noise added.
  There are now 3,755 exact linked functions.

- AILDigitalDriver's opaque prefix is replaced by the pristine Miles
  5.0e SDK DIG_DRIVER type. The bundled SDK matches the runtime version;
  its primary-buffer member lppdsb is at +0xa8, exactly where retail Open
  reads it before calling SetVolume. A layout check also fixes wformat
  at +0x8c. Using the SDK directly preserves all original member names,
  packing and synchronization qualifiers at this external boundary,
  including the rest of the driver beyond the former partial view.
  Vendor files remain unchanged. One full VC6 build passes; onKillFocus
  returns to 99.8710% after the shared-header change, with all other
  current scores unchanged (onKeyPress remains exact). Historical MAX
  remains preserved; there are now 3,754 exact linked functions.

- The game's leading four-byte slot is restored as `m_newGameWin`.
  Dreamcast game::newGameWin is a heroWindow pointer at +0 (type 0x101d
  points to the complete heroWindow class 0x101c), followed by the
  70-byte spellAllocInfo array at +4. Retail preserves that array anchor
  and the preceding four-byte slot. This is source/layout evidence;
  no retail access to the pointer itself has been located. The adjacent
  arrays also regain Dreamcast's spellAllocInfo and NH3API's
  spellDisabledInfo names throughout their declarations and uses.
  One full VC6 checkpoint passes with every current score unchanged.

- Three window gaps regain their Dreamcast member declarations from
  source/layout evidence. TCampaignWindow's lastActive/currentCampVideo
  anchors shift by eight bytes in retail; its following 12-byte gap fits
  saveVideoFile (void*), CheckMark (Bitmap816*) and RolloverWidget
  (const widget*) before the new campaign-availability array. In the
  selection window, the proven randomHero/noHero pointer anchors shift
  by 0x10 and retain the intervening noDice bitmap slot. The same shift
  between saveGameEdit and pNewPlayerUpdateMan restores the mode byte
  and three pointer-alignment bytes. No direct retail accesses to these
  restored members have been located; the owning comments distinguish
  that limit from their proven reference names and surrounding anchors.
  One full VC6 checkpoint passes with all current scores unchanged.

- TRmgMovementCost's upper halfword is now m_zonePathCost. Retail flood
  0x53f1a0 zeros it at the seed, limits neighbours to the supplied zone,
  and relaxes their costs by 2/3 per step. Ground-connection selection
  reads the same high word at 0x5412e9 to rank empty crossing candidates.
  This name is derived from retail behavior; the 16-bit layout and the
  independent low-halfword movement cost remain intact. One full VC6
  checkpoint passes with all current matching scores unchanged.

- Two packed-tile placeholders resolve through the terrain/road/river
  adapters. The terrain frame occupies bits 6..13; river type and frame
  occupy 14..17 and 18..25 in the ground word. The road frame uses all
  eight low bits of the data word, absorbing the former unknown07 bit.
  Matching getters sign-extend these fields. The former decorationType
  is now roadType, independently fixed by the road adapter. Six flip
  bits at data-word positions 15..20 also acquire terrain/river/road X/Y
  names; bit 21 remains unresolved, so the original unknown15 declaration
  is still counted. Setter/getter masks and shifts are documented beside
  the declarations. One full VC6 checkpoint passes; writeMapHeader
  recovers from 93.2786% to 93.2864%, with every other score unchanged.

- The remaining bit of former TRmgGroundTileData::unknown15 is coastal.
  Retail cell serialization 0x532972 maps data-word bit 21 to bit 6 of
  the seventh H3M cell byte. The map-layer reader at 0x4fe220 consumes
  that flag to create ANCHOR_POINT cells on non-water terrain. Together
  with the six previously recovered flip bits, this resolves the whole
  original seven-bit declaration. One full VC6 checkpoint passes with
  every current score unchanged.

- TRmgGroundTileData bit 23 is now m_placementOutline. Helper 0x535ee0
  traces a closed perimeter into its point vector; accepted placement
  0x5468e8 calls that helper then sets bit 23 on the resulting points.
  Quest placement paths at 0x54b6df/0x54bad5 mark the same outline.
  The checker at 0x546ed5 requires the bit at a candidate contact cell.
  The name is retail-derived, with original spelling unknown. One full
  VC6 checkpoint passes with all current matching scores unchanged.

That leaves **20 starting declarations requiring further work**. This is an
accounting of the original population, not a count of names matching a regex:
the recovered aggregate adds a real ownership declaration, and original
numbered field names remain intentionally present in the source.

Validation before the decomp merge: the full VC6/Wine build passed all sixty
batches. At that checkpoint, these scores differed from the 332-entry start:

| Function | Starting | Current |
| --- | ---: | ---: |
| CEnterNameEdit::onKillFocus | 100% | 99.8710% |
| CEnterNameEdit::onKeyPress | 100% | 99.8868% |
| GameSelectionHeadersStruct constructor | 79.2582% | 73.1483% |
| game::processOnMapTowns | 80.3907% | 94.3642% |
| army copy constructor | 66.4292% | 100% |
| advManager::drawAdvObj | 87.9441% | 87.7661% |
| advManager::drawAdvObjShadow | 85.2335% | 85.1872% |
| type_random_map_generator::writeMapHeader | 95.6987% | 95.7066% |

At that pre-merge checkpoint, every other current score agreed, no identities
were missing, and 3,729 functions were exact. The temporary army::doAttack recovery in batch seven returned
to its starting 98.8868% during the popup correction. These measurements check
current reproduction separately from MAX/history retention.

The onKillFocus change exchanges two saved-local stack slots and the order of two
independent register reloads inside the expanded `onEnter` call. The CFG and
call sequence agree. A disposable Gruntz forest search (baseline plus 16
variants, seed 20260906) found two islands; nine variants reproduced the
relocation-masked retail function bytes. No compiler-state noise is retained,
and the current source still has the reported 99.8710% residual. The evidence
is recorded beside `CEnterNameEdit::onKillFocus`.

The selection-header constructor changed during the popup correction, in a
string-helper expansion inside NewSMapHeader construction. Using natural
popup tail alignment yields the same score. A second disposable Gruntz search
(baseline plus 16 forests before the TU includes, seed 20260906) finds three
byte islands scoring 73.1483%, 79.2582%, and 79.3242%. The higher variants
recover/exceed the prior current score, but no probe noise is retained. The
TownExtra consolidation subsequently recovers 79.3242% in the production
source without probe noise; its constructor comment records the recovery.

The same TownExtra consolidation produces the sacrifice-window dip above.
A disposable Gruntz forest search before createArtifactWidgets (baseline plus
16 variants, seed 20260906) finds two islands. All 16 forests recover the prior
99.9983%; the baseline stays at 99.5918%. No probe noise is retained. The
function comment records the EBX reload/loop-boundary residual.

The owning headers contain the field-specific evidence. The temporary audit
and retail disassembly are under `/tmp/homm3-remaining-audit`; neither that
directory nor this progress note replaces source-owned symbol annotations.

The doCombat dip from spell-array consolidation was probed with two disposable
Gruntz forest placements (baseline plus 16 variants each, seed 20260906).
Before the function, every variant stays at 98.1758%. Before includes, two
islands score 98.1758% and 98.5379%; the latter recovers the preceding current
score. No probe noise is retained. The later palette consolidation recovers
98.5379% in production.
The source comment records the result; temporary details are in
`/tmp/homm3-remaining-audit/events-include-islands/results-scored.json`.

The canonical map-view consolidation changes the current rows shown above.
It recovers createArtifactWidgets to 99.9983%, returns customcampaign 0x88880
to 53.9715%, and leaves doCombat at 98.5379%. New renderer/onKeyPress losses
and the renewed selection-header constructor dip require recovery work;
previous island-search results do not prove current reproduction.

Renderer recovery probe: baseline plus 16 Gruntz forests at each of two
placements (before includes and before drawAdvObj), seed 20260906. All 34
trials retain drawAdvObj 87.7661% and drawAdvObjShadow 85.1872%; neither
placement recovers the prior scores. No noise is retained. These results
rule out those sampled compiler-state variations, not all possible recovery.

## Decomp integration checkpoint

The naming branch fast-forwards to decomp-complete-4.0 `60ceaa60`; the naming
and field-recovery changes remain uncommitted on top in the same worktree.
A retained pre-merge stash protects the complete previous state. Conflict
resolution keeps the newly recovered RMG interfaces and helper boundaries,
normalizes their project-owned fields/functions, and removes the duplicate
terrain-rule view. No compiler-state noise or extra inline pins are added.

One full VC6/Wine build after integration passes, including the cleanliness
and source gates. Arithmetic-operator label tests also pass. The current
result is 3,753 exact linked functions and 91.26% executable fuzzy coverage.
All 4,406 upstream retail identities are present, and every newly recovered
upstream function keeps its current score. Comparison with the incoming
branch has nine current-score differences:

| Function | Incoming decomp | Integrated current |
| --- | ---: | ---: |
| advManager::drawAdvObj | 87.9441% | 87.7661% |
| advManager::drawAdvObjShadow | 85.2335% | 85.1872% |
| army copy constructor | 66.4292% | 100% |
| army::doAttack | 98.9251% | 98.8868% |
| game::processOnMapTowns | 80.3907% | 94.3642% |
| type_random_map_generator::writeMapHeader | 94.1017% | 93.2864% |
| GameSelectionHeadersStruct constructor | 79.2582% | 73.1483% |
| CEnterNameEdit::onKeyPress | 100% | 99.8868% |
| CEnterNameEdit::onKillFocus | 100% | 99.8710% |

MAX/history takes the per-retail-address maximum of both inputs. Current
scores above are freshly measured; preserving those historical numbers is
not a claim that their old source/object artifacts were archived. Incoming
decomp removes the TU-local string constructor specializations: that source
change and its measured writeMapHeader residual are retained. Natural
compiler-state recovery and the remaining 20 starting placeholders remain
open work.

The post-integration batch 68 checkpoint recovers both CEnterNameEdit rows
above to 100% by removing the obsolete network class views. These are now
reproduced current peaks, not merely preserved historical scores. The
writeMapHeader current score remains 93.2786% after batch 66; its recovery,
the renderer, selection-header constructor, and army residuals remain open.

Batch 69's SDK-header integration returns onKillFocus to 99.8710%, so its
current recovery is open again. Batch 68 proves a prior current exact
checkpoint; it does not imply the new compiler state still reproduces it.

## Progress-object extent audit

The remaining TRandomMapProgress::pad_28 declaration needs correction before
it can be resolved. Retail constructs the object at ebp-0x5c (0x5862ef),
then reads the independent monsterStrength local at ebp-0x30 (0x586306).
It later writes a min-expression temporary to that same local (0x586437)
before repainting and destroying the still-live progress object. Thus the
current 0x30-byte model overlaps a live local and cannot represent the retail
extent. The known fields fill 0x28 bytes, and this overlap bounds the object
at 0x2c bytes or smaller. The intervening word at ebp-0x34 was used for a
thread ID before construction; that alone does not distinguish a 0x28-byte
object from one with a final member at +0x28. The placeholder remains counted
until that distinction is established. This was a read-only binary audit;
no source layout change or build was made.

Batch 76 removes the proven extraneous +0x2c..+0x2f bytes from the progress
view. The remaining word at +0x28 stays explicitly unresolved; 0x2c is now
checked as the provisional view extent, not asserted as proven retail sizeof.
One full VC6 checkpoint passes with all 4,406 current scores unchanged.
The count remains 299 handled and 33 unresolved because this is only a
partial correction of the original tail declaration.

Batch 77 moves the full Miles SDK include from soundmgr.h to soundmgr.cpp,
its sole field-accessing consumer. The header forwards the real _DIG_DRIVER
type and keeps the same pointer alias, preserving the complete SDK definition
at its owner. One full VC6 checkpoint passes and onKillFocus recovers from
99.8710% to 100%. Every other current score is unchanged, including exact
onKeyPress. There are again 3,755 exact linked functions. No compiler-state
noise is retained. The placeholder count remains 299 handled / 33 unresolved.

The generator player-map recheck confirms nine -1 initializations at +0xee0,
writes into entries 1..8, and zero-based reads from +0xee4. It does not prove
the entire span through +0xf23 belongs to the named array, so its unresolved
leading-slot/extent issue is retained rather than resolved by a rename.

The batch-77 selection peak now has an artifact snapshot at
`/tmp/homm3-remaining-audit/peak77/selection-exact-checkpoint.tar.gz`, with
SHA-256 and base commit in its adjacent manifest. It contains the current
source/configuration plus raw and normalized selection comparison objects
and the report. Archive entries were read back and compared with the saved
objects. This preserves concrete artifacts for the two current exact edit
callbacks; it does not retroactively supply artifacts for older MAX values.

A further SBolt audit over 0x5a5260..0x5a60f0 found that all six textual
+0x1c references are EBP-relative arguments, not accesses to SBolt::field_1c.
That word remains unresolved. The shipped IFC20.DLL is still needed for the
six incompatible-version SDK layout slots; its installation path has been
requested. These read-only audits required no build.


## IFC 2.0.3 runtime: pass 78

The installed Steam copy supplies IFC20.dll version 2.0.3, SHA-256
`e8c2afa0e2a19cd21d03685fd6f18a025160c598ae93a9770a124f2a389e3846`.
Its exported constructor at DLL RVA 0x6ba0 matches the game's four zeroed
project words. GetDevice (0x6bb0), get_next/set_next (0x6be0/0x6bd0),
append_effect_to_list (0xca80), and Close (0xc0a0) establish the actual old
layout: project handle +0, compound-effect list +4, device +8, next project
+12. LoadProjectObjectPointer stores the IFR handle at 0xc559; Close passes
it to the exported IFRReleaseProject. The newer SDK's corresponding names
m_hProj, m_pCreatedEffects, m_pDevice, and m_pNext become m_proj,
m_createdEffects, m_device, and m_next, with typed pointers for the latter
three. The handle remains void* as specified by HIFRPROJECT = LPVOID.

One full checkpoint passed with no score changes: 3,755 exact functions.
303 of the starting declarations are handled; 29 remain. CImmMouse and
CImmEnclosure still require independent runtime layout recovery.


## IFC mouse and device base: pass 79

The same verified IFC 2.0.3 DLL resolves the entire CImmMouse opaque region.
CImmDevice is 0x24 bytes (vector deleting destructor stride at DLL RVA
0x141a), containing its vptr, four-byte CEffectList cache, BOOL initialized,
DWORD deviceType, GUID device, and BOOL guidValid. Cache destruction at
0x3f80 addresses +4; AddEffect 0x7300 and list destruction 0x72e0 establish
the eight-byte effect/next nodes and single head pointer. GetDeviceType
0x6be0 proves +0xc. Enumeration 0x42d0 copies the GUID at +0x10 and sets
+0x20; Initialize consumes both, setting initialized +8 at 0xba6d.
Mouse GetAPI/GetDevice 0x4470/0x4480 return the interface pointers at
+0x24/+0x28. Constructor 0xb7b0 and reset 0xbb60 corroborate ownership.
The resulting 0x2c layout matches the game's allocation. The newer SDK's
product-type field is absent from this old runtime; it is not copied.
Original SDK member names remain in source comments, normalized in code.

One full checkpoint passed; all 4,406 current scores are unchanged and
3,755 remain exact. 304 starting declarations are handled; 28 remain.
The enclosure constructor evidence now establishes its base ends at 0xa4,
followed by a 0x38-byte enclosure descriptor and a word at +0xdc. Its base
and descriptor need further field-by-field analysis before replacing its
remaining opaque region.


## IFC enclosure and effect base: pass 80

The installed IFC 2.0.3 runtime resolves the last Immersion opaque region.
The 0xa4-byte CImmEffect base contains the SDK cache state, suite flag,
short priority (with two alignment bytes), three timing values, owning
device pointer, 0x48-byte FEELIT_EFFECT, two axes, two directions, effect
GUID, playback flag, device type, two API interfaces, axis count,
no-download flags, and iteration count. Constructor RVA 0x48d0 and reset
0x6b20 establish storage and internal descriptor pointers. Exported getters
at 0x1460/0x1470/0x1480/0x14c0 fix effect/device/GUID/priority offsets;
Start 0x5206/0x5215 and Stop 0x5269 identify playback timing/state;
initialize 0x59ee..0x5a46 establishes device and option fields, and 0x5c7a
records the loaded timestamp. SDK names are retained beside declarations.

CImmEnclosure constructor 0x7a60 appends a 0x38-byte FEELIT_ENCLOSURE and
BOOL useMousePosAtStart. set_parameters 0x81b0 fills the descriptor and
links it into FEELIT_EFFECT; Start 0x8101 tests the final flag before
GetCursorPos/SetCenter. The pristine FeelitAPI.h supplies the boundary
structs directly. Newer SDK-only envelope/name/inside-effect/timer members
are not imported into the old class layout.

One full retail checkpoint passed with no score changes (3,755 exact).
An isolated Clang header check confirms device/mouse/effect/enclosure sizes
0x24/0x2c/0xa4/0xe0; record layouts agree with the DLL offsets. The full TU
Clang check encounters generated legacy fstream errors, while VC6 passes.
305 starting declarations are handled; 27 remain. All six starting
Immersion placeholders are now resolved.


## Terrain-music byte: pass 81

The incoming receiveSaveGame reconstruction provides a consumer beyond the
sound-manager initializer. Retail 0x4cbdb7 sign-extends the byte at +0x80,
saves it in EBP-0x60, and 0x4cc6f6..0x4cc716 passes it to
switchAmbientMusic unless it is -1, restoring music after the transfer.
This corroborates NH3API's currentTerrainMusic name. The field becomes
m_currentTerrainMusic, retaining the proven signed-char width; NH3API's
int32 width is not adopted. Declaration, initializer, and game consumer
are updated together. The following three bytes remain unresolved: this
read proves neither their contents nor that they are alignment padding.

One full checkpoint passed with every current score unchanged (3,755 exact).
306 starting declarations are handled; 26 remain.


## Dialog and hero-traits re-audit after pass 81

The latest source still has no semantic consumers for the hall, swap,
blacksmith, or campaign-brief unknown slots. The THeroScreenWindow comment
formerly described field64 as a widget left by the base constructor; this
was unsupported. Its retail store follows a vector-end-minus-one read
before the first push, while the reconstructed CAdvPopup, CHeroWindowEx,
and heroWindow constructors do not populate the vector. The comment now
records that limit. Likewise, the THallWindow comment no longer claims an
external consumer: current setupCastle accesses no extra hall slot.

The current NH3API THeroTraits reference describes THREE byte flags at
+0x38/+0x39/+0x3a (allowedInRoE, allowedInABSoD, isCampaignHero), followed
by five unnamed bytes. It does not supply a fifth named flag or a semantic
name for +0x3c. The in-tree DC-derived attributes dword should be checked
against retail flag consumers before adopting the PC byte interpretation.
This is the next layout question; no speculative split was applied.

This audit changes evidence comments only. No build was run and the count
remains 306 handled / 26 unresolved.


## Hero-trait static-data check after pass 81

The pinned PE's 156 records at 0x679dd0, stride 0x5c, contain these eight-byte
patterns at +0x38 through +0x3f: 127 rows have 01 01 00 00 00 00 00 00;
17 have 00 01 00 00 00 00 00 00; 11 have 00 01 01 00 00 00 00 00;
one has 01 00 00 00 00 00 00 00. This supports NH3API's three-boolean
interpretation, but all five subsequent zero bytes remain semantically
unidentified. Zero initialization alone does not prove padding.

A full text disassembly was generated and reference windows for the trait
pointer 0x67dce8 and storage base were inspected for +0x38..+0x3f consumers.
No discriminating consumer was established. The initializer only fills
names and stack ranges. This search is not a whole-program alias proof:
adjusted pointers or unrecognized references could still reach those bytes.
The DC attributes field is therefore retained pending stronger evidence.
No source-layout edit or build was made; 26 starting declarations remain.


## Campaign caption ownership: pass 82

SCampaignCaption was a duplicate view into the existing campaign help table.
Its base 0x6a5f88 is g_campaignWindowHelp[1], eight bytes after the canonical
0x6a5f80 claim in text.cpp. The retail loader 0x5b9ad1..0x5b9af7 writes
24 pairs from spreadsheet columns 0/1, establishing rollover and right-click
text at +0/+4. Dreamcast THelpText supplies Rollover/RightClick names;
the existing canonical type spells those members m_text/m_rclick.
The old pad04 is therefore a text pointer, not padding.

The chooser now uses g_campaignWindowHelp[campaign + 1].m_text, preserving
retail 0x45ef6d's indexed address. The duplicate structure and caption-array
declaration were removed. Looking only for the exact interior base address
had missed the loader's use of 0x6a5f84 plus indexed stores; the earlier
claim that the block had only one consumer was too narrow.

One full checkpoint passed with every current score unchanged (3,755 exact).
307 starting declarations are handled; 25 remain.


## Video-descriptor address audit after pass 82

The campaign-caption overlap prompted an address-range search for the video
descriptors, including interior row addresses rather than just the first
record. The first 30 rows at 0x6839c0 (stride 20) have plausible video/audio
stem pointers and four flag bytes; all eight bytes at +0xc..+0x13 are zero.
The text disassembly's direct address references in that 600-byte span use
only offsets +0, +4, +8, +9, +0xa, and +0xb of the first row with scaled
indices. No tail consumer was established. This is not an exhaustive alias
proof and does not establish padding or the whole table's extent.

The complete Dreamcast field inventory supplies no 20-byte record with a
one-byte member at +8 that identifies this layout. No equivalent overlapping
canonical table was found. The opaque tail stays unresolved; no build or
source-layout edit was made. The remaining count is 25.


## Player-map and strip cross-check after pass 82

A fresh scan of the generator span still finds the nine-word initialization
at 0x5499fb and one-based writes at 0x549a75/0x549ab8, with readers based
at +0xee4. It supplies no direct accesses establishing the separate region
+0xf04..+0xf23. Replacing the leading gap with a 17-element array would
therefore add an unsupported extent claim; the current uncertainty remains.
The newer decomp tip 9d21e551 changes tooling only, with no source/header
field evidence relative to the incorporated 60ceaa60 tip.

The available HoMM2 reconstruction's strip layout is not a matching source
layout for HoMM3. It models a window pointer plus 24 unused bytes before
x/y, then borders and five-creature icon/cache arrays. HoMM3 has seven army
slots, changed members and offsets, and a widget-message-based drawing
implementation. Those HoMM2 field names cannot be transplanted into the
HoMM3 gaps without retail evidence. No such evidence was established in
this cross-check. No source-layout changes or builds; 25 remain unresolved.


## Game alignment gaps: pass 83

The recovered AI::resource_value double array gives AI eight-byte alignment.
Its containing playerData retains that alignment and its retail-proven 0x168
stride. In turn, game's playerData array requires eight-byte alignment:
NewfullMap ends at +0x20acc, so the next member naturally begins at +0x20ad0.
The final monsterIdentifiers vector ends at +0x4e7cc, and game tail padding
rounds the object size to 0x4e7d0, matching the retail allocation.

Removed the explicit pad20acc and pad4e7cc byte arrays. An isolated header
layout comparison before/after shows every remaining member offset and
sizeof(game) unchanged; header syntax passes. These gaps are now supplied
by compiler alignment rather than represented as unknown member storage.

One full checkpoint passed. The only score change is army::doAttack,
98.8868% to 98.9251%, recovering its incoming-branch score. All other
current scores are unchanged, with 3,755 exact. The historical 99.9962%
MAX for doAttack remains a separate unreproduced peak.
309 starting declarations are handled; 23 remain.


## Sound-manager alignment: pass 84

Pass 81 established currentTerrainMusic as a signed byte, using retail
0x4cbdb7 rather than NH3API's contradictory int32 width. The following
playSounds member is a four-byte integer at +0x84 (including the dword
store at 0x4cbdc7). The three intervening bytes exactly match its required
alignment in the naturally aligned soundManager; no consumer of the
explicit field81 array exists in the reconstruction. Removed that array
and let the compiler supply the alignment gap.

This supersedes pass 81's decision to retain the gap pending a layout
check. The isolated before/after layout check confirms every surviving
member offset, four-byte alignment, and sizeof(soundManager)=0xd8 are
unchanged. One full retail checkpoint passed with every score unchanged
(3,755 exact). 310 starting declarations are handled; 22 remain.

The RMG zone's seven-byte gap differs: only three bytes are required to
align the next dword, leaving a separate unexplained word at +0x40. Its
whole placeholder cannot be resolved by this alignment argument.


## Selection-header human flags: pass 85

Dreamcast GetHeader line 3797, dc 0x138b76, copies 32 bytes from the
symbol-named g_wasHuman into GameSelectionHeadersStruct +0x55c. The literal
at dc 0x138b9c establishes that offset. Its constructor line 78, dc
0x14752c, zeroes the same 32-byte band (literal dc 0x147550 = 0x55c).
This establishes the older header's eight-integer human-status array.

The corresponding eight-integer band in Complete, proven by independent
copy loops at +0x4d0 and the message read/write routines, is named m_wasHuman.
This cross-version correspondence is an inference, not an original member
spelling recovered from a complete type record. The source comment states
that limit and that no Complete producer has been located. Complete's live
restore path copies SavedGameHeader::humanPlayer to g_wasHuman instead.
The proven retail type, count, offsets and serialization order are retained.

One full checkpoint passed with all current scores unchanged (3,755 exact).
311 starting declarations are handled; 21 remain.


## H3API generated-zone reference: pass 86

Pinned RoseKavalier/H3API at commit
92255ab18da784a5842ecc2b8bc0ce00e19a0c56. Its
[H3RmgZoneGenerator](https://github.com/RoseKavalier/H3API/blob/92255ab18da784a5842ecc2b8bc0ce00e19a0c56/include/h3api/H3RMG/H3RmgZoneGenerator.hpp)
identifies the +0x08 slot as INT32 townType2. The +0/+4/+0xc fields,
coordinates, object-count array at +0x44, vectors at +0x3e4/+0x3f4/+0x404,
and 0x414 size establish correspondence to TRmgZone. The opaque four-byte
slot becomes int m_townType2. This is a third-party reference-backed name
and type, not a recovered original spelling or proven retail runtime role;
no semantic consumer was located. The source comment retains that limit.

The same reference leaves the template +0x20 gap and zone +0x3c region
unknown. Its ground-tile tail bits are unnamed too, and its narrower
frame/type fields disagree with retail sign-extension evidence already
recovered here; those older interpretations are not adopted. Seven reference
headers were saved outside the repository for comparison; vendor is unchanged.

One full checkpoint passed with all current scores unchanged (3,755 exact).
312 starting declarations are handled; 20 remain.


## Additional H3API dialog coverage after pass 86

Checked the pinned H3SelectScenarioDialog, H3TownDlg, H3AdventureMgrDlg,
H3SwapManager, and H3HeroInfo headers, then the consolidated H3API.hpp.
H3SelectScenarioDialog still labels +0x358 as four unknown bytes. Its
portrait model separates hpsrandPcx at +0x354, whereas our retail consumers
index that same slot as heroPix[163]; this difference does not identify
+0x358. H3HeroInfo explicitly retains an unknown word at +0x3c, agreeing
that its zero value alone does not establish semantics.

H3SwapManagerDlg is only forward-declared in the consolidated header.
The town/adventure dialog definitions are different classes from the hall,
blacksmith and hero-screen windows; their field names cannot be transferred
on an offset match alone. No definition was found there for the remaining
resource-transfer, campaign-brief, bolt or random-map-progress fields.
These references supply no further recovery. No source-layout changes or
builds; 312 starting declarations handled, 20 unresolved.


## Outstanding evidence barrier after pass 86

Three consecutive follow-up audits produced no supported field recovery:
additional public H3API coverage, recipient/inheritance checks, and inventory
revalidation. All 20 remaining declarations still exist in the current
headers. The newer decomp tip adds no source/header changes. No compiler or
analysis process remains pending; the latest full checkpoint is pass 86.

Further names or types require missing class definitions or a discriminating
consumer not present in the evidence recovered so far. The unresolved groups
are:

- THeroTraits +0x3c, video descriptor +0xc, and the two strip gaps: static
  or allocated storage without established semantics; zero bytes and older
  nonmatching layouts do not prove padding.
- Hero-screen +0x64, hall +0x60, blacksmith +0x68, combat placement +0x38,
  resource-transfer +0x80, campaign brief +0x68, swap +0x60, and selection
  +0x358: missing member definitions; existing stores, allocation extents,
  or neighboring members do not distinguish a semantic role.
- SBolt +0x1c and TRandomMapProgress +0x28: no established bolt-member
  consumer, and a remaining uncertainty in the progress object's extent.
- RMG template +0x20, zone +0x3d, generator +0xee0, and three trailing
  bitfields: unresolved aggregate boundaries or bits whose semantics cannot
  be distinguished from unused storage by the currently recovered accesses.

No new names, padding claims, or builds were made for this final revalidation.
The objective remains incomplete: 312 handled and 20 unresolved. The goal
is blocked on this evidence gap, not complete. Known cross-version/reference
inferences among the named fields remain explicitly qualified above and in
owning declarations; matching scores do not independently prove those names.
