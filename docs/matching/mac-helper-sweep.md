# Mac retained-helper sweep

The current pass restores source helper boundaries and calls across all Windows
game functions, including byte-exact callers. A lower byte score does not reject
a helper supported by the available source and compiler evidence. Byte polishing
follows the broad pass.

Use clear project names when Mac shows an operation but not its original C++
spelling. Restore the helper and its call sites promptly, including when Mac
inlines the operation too; an original symbol or immediate score gain is not
a prerequisite. Keep one canonical body and refine names or placement later if
new evidence warrants it.

During subsequent byte recovery, keep these helper calls. Do not replace them
with direct fields, array indexing, or pasted statements to recover a score.
An outer helper is permitted when it contains the recovered helper call; review
that complete source path. Tune natural argument evaluation, local lifetimes,
body visibility and source order around the preserved calls.

Use `homm3 mac calls <unit>` and `homm3 mac disasm <Windows-VA>` on claimed
callers. Direct Mac branches resolve to source claims and runtime labels; an
unnamed destination needs an identity review. These are leads for human
review, not proof of an inline qualifier or even a game helper. A Mac inlined
operation may have no retained call and therefore needs a body-shape review
too. The current whole-corpus request supersedes the earlier module deferrals:
RMG is included, and library/runtime/generated destinations must be identified
as such rather than silently dropped from the call inventory. Keep vendor code
pristine; identifying a library destination does not require recreating it as
a game helper.

For each caller, inspect Mac disassembly and direct targets; use Dreamcast
source facts when they are already available. Restore one canonical helper body
and source call in its plausible owning TU/header. The name may be ours when
Mac shows the operation but does not preserve a source symbol. A field load,
array index, or sequence of stores in Mac can be an expanded helper. Do not
remove an authored helper call solely because the PEF has no retained call,
including in Windows functions already at 100%. Check Mac body order
separately from cross-TU VC6 expansion when deciding header placement.
Record unresolved destinations instead of silently counting them as absent
helpers. Batch the helper edits without per-function score tuning or
per-helper builds; compilation and byte comparisons follow the broad
restoration pass.

## Whole-corpus review

The working inventory covers 139 game translation units, 5,549 canonical source
definitions, 9,684 DC procedure records and 6,921 known Mac function spans.
Native call destinations without a recorded extent are also retained as leads.
These are coverage totals, **not a claim that every semantic review is closed**.
The inventory includes exact Windows functions and source functions without a
DC/Mac address. Source annotations remain the function-name authority.

The generated working files are under `build/all-function-calls/`:
`function-inventory.tsv` contains every canonical source function;
`native-source-edges.json` records individual native calls and source identities;
`review-with-paths.json` retains possible nested and implicit lifetime paths.
Clang diagnostics, unresolved indirect calls, unpaired targets, and ambiguous
template instances are coverage gaps, not evidence of absent source calls.
In particular, a reachable nested helper is a review candidate: an unrelated
call elsewhere in the caller does not discharge the native operation.

### Recovered structure

* RMG now uses canonical helpers for line/terrain neighbour masks, frame
  selection, first-object lookup, guard valuation, treasure replacement and
  disposal, occupied-cell counting, per-zone distance initialization, bounds
  accumulation, map-cell object removal and active-zone counting. Native
  offsets and caller evidence accompany the owning bodies.
* Mac's reward constructors derive treasure value internally. Experience boxes
  take one reward argument and compute `experience * 12 / 10`; quest rewards
  compute `2 * (reward - 2000) / 3`. The roster no longer supplies independently
  precomputed values. Prison constructor argument order and the intermediate
  dwelling constructor's four-argument interface are restored too.
* Multiplayer IPX setup calls the shared member `initRemote`; TCP setup calls
  `refreshSessions`. Both use the ordinary existing member definitions.
* Resource disposal, cache hooks, text access, bitmap access, timing, network
  setup, widget visibility and town-name operations again use their existing
  helpers where native evidence supports those calls. This includes exact
  callers; a zero byte-score change is not a reason to leave pasted logic.
* `TSeerData` inherits the five-byte `TQuestGuard` prefix. Mac's seer constructor
  calls that base constructor and then performs its additional field writes;
  the packed Windows layout is preserved.
* `CDPlayGroup` inherits `CDPlayPlayer` and delegates construction to it.
  DC class records 0x3d83/0x5447 prove the public base at offset zero, and
  SH4 0x8be7a calls its constructor. The former duplicate name/DPID fields
  hid that relationship. The 0x104-byte Windows layout and VC6 scores hold.
* Bitmap and sprite drawing again use their bitmap-taking overloads in window
  backgrounds, fades, borders, buttons, pointer loading, radar/puzzle drawing,
  combat animation, campaign subtitles, credits, RMG progress and text scrolling.
  This restores 37 calls whose map/width/height/pitch forwarding had been pasted
  into callers. DC retains the wrappers; Mac retains the raw calls with those
  same forwarding expansions. The DirectDraw locked-surface path still supplies
  its actual surface pointer, dimensions and pitch.
* Adventure redraw, screen update and combat victory again call the five
  DC-proven mouse enable/disable hooks. Complete's release helpers only return
  the counter; discarded results disappear under VC6, so these calls are
  byte-neutral.
* Seer log, rollover and quick-info text builders call `TSeerHut::getName`
  instead of repeating the name-vector lookup. DC retains that call in the
  older callers; Mac expands it inside the Complete text builders. This
  restores the complete nested path without changing Windows MAX scores.
* Combat mouse highlighting calls `validHex` and `inInvisibleColumn` again.
  DC retains both calls; Mac expands both, including the nested bounds check.
  `army::getMirrorEffect` calls `getSpellTime` and `cppMax` instead of spelling
  out their field access and reference selection. Complete moved the older
  spell-duration check into that helper. All four calls preserve Windows MAX.
* Army spell-state reads now use `getSpellTime` in 25 ordinary methods and
  three shared predicates, including damage, luck/morale, animation and spell
  expiration. This restores 44 duration reads and one `getSpellLevel` read;
  mutation stays in the owning lifecycle methods. Mac retains the indexed
  reads at army+0x198+4*spell and expands the same accessors inside controlling
  side, incapacitation and retaliation. The header records representative
  native sites. Recognizable expansions need review even when no native call
  survives.
* Eight network-message constructors default-construct their `type_point`
  member before assigning payload fields, as the DC calls show. Explicit
  point copy initialization had removed that lifetime. Restore the original
  construction followed by the observed assignment order.
* Mac's river/road painter destructors identify the existing line-painter base
  destructors. Their source definitions now carry the corresponding Mac claims;
  the C++ base destruction paths already represented those calls.

### Call-extraction correction

The DC pass exposed stale-register call labels in the SH4 analysis tooling.
It now invalidates literal targets after register writes and after a call's
delay slot, preserves saved registers, and follows register copies. A load
through a global pointer no longer labels the subsequent indirect call as a
call to that global. The assembly renderer uses the same analysis.

Regenerating all 9,684 DC procedure records changed 1,014 unsupported target
claims to unresolved indirect calls; no named game-procedure target changed.
Most were import-slot addresses, which are also data rather than direct code
targets. Keep these unresolved instead of counting them as reviewed callees.
Pointer-slot provenance is reported separately from a direct target, including
in rendered assembly. The working Mac join also preserves the runtime map's
`indirect_tvector` classification: 2,750 calls through `__ptr_glue` have unknown
dynamic destinations. The stub's runtime identity does not discharge the game
operation it invokes. These are explicit review gaps. The
Dreamcast/structure/source-facts tests pass (60 tests, eight skipped).

### Identity corrections

DC `game::get_alignment(int)` selects a **player's** setup alignment and handles
a negative player index. The separately retained Windows creature-alignment
lookup is now named `getCreatureAlignment`; it no longer claims that DC body.
The setup array and network message pointer use the symbol-proven `TTownType`.

DC `ExtraInfoUnion::getCustomIndex` extracts the 12-bit resource/treasure index.
Its annotation had been attached to an 8-bit monster accessor. Treasure lookup
now calls the correct helper, and the distinct monster accessor retains its
8-bit semantics. Town-manager swap/garrison identities and the two freelancers'
guild overloads likewise carry their corrected DC pairings.

### Validation limits

VC6 checks distinguish semantic restoration from byte-score movement. Some
restorations improve matching, some are byte-neutral, and some alter inlining.
The RMG roster's recovered constructor interfaces currently lower its Windows
score from 97.22% to 96.25%; the previous peak remains in HIST. Keep the source
interfaces and investigate natural compiler context during byte recovery.

The shared `hasBuilding` to `getBuildingMask` path changes PPC allocation in
20 scored callers. Their annotations document the one-checkpoint abstraction
exception required by the Mac preservation gate. This does not waive later
regressions or turn an unresolved call identity into a verified pairing.

The bitmap-overload batch makes `viewPuzzle` exact and improves
`fizzleForwardX` to 99.94%. It lowers `creditsWait` from 100% to 98.28% and
`shootBallisticMissile` from 94.19% to 91.24%; HIST retains their previous
peaks. Three Mac callers (`updateProgressBar`, `saveFizzleSourceX`, and
`fadeToBlack`) carry reviewed abstraction exceptions for the same forwarding
operations. Their score changes do not justify pasting the wrappers back into
the callers.

The full checkpoint passes source ownership (5,549 definitions, no violations),
source inventory, layout/order and cleanliness gates, and links with zero
unresolved externals. Windows records 4,448/4,814 exact functions and 98.21%
weighted MAX. The Mac gate scores 1,527 pairs with no unreviewed regression;
1,741 emitted pairs remain unavailable because of unresolved references, and
83 Windows-VA claims have no full-TU Mac body. These comparison gaps are not
call-review completion. This checkpoint includes the bitmap wrappers,
DirectPlay base constructor, mouse hooks, seer-name and combat accessors,
spell-duration accessors and the eight network point lifetimes;
598 scored Mac pairs are exact.

The spell-accessor batch keeps the supported calls through Windows score dips:
`getUnitCombatValue` moves from 100% to 93.42% and `resetRound` from 100% to
97.40%. Their previous peaks remain in HIST. Ten Mac callers carry the narrowly
scoped abstraction checkpoint for nested `isIncapacitated -> getSpellTime`
reads, with each native expansion site recorded on its `MAC_ADDRESS` line.

The constructor review verifies default member/base initialization separately
from explicit initializer lists. In particular, a member assignment inside a
constructor does not replace its earlier construction, while explicit point
copy initialization must not be counted as a default-constructor call. The
working review records 183 existing native constructor sites under that rule,
separately from the eight restored point-construction sites.

Manual inspection also verifies 64 terrain-cache calls through `getTerrain`
or `getFrame`, `getPackedCell`, and `initializePackedCell`. The operation at
each site is already represented; the working notes distinguish terrain
queries, frame queries and the two whole-tile reads. Those reviewed paths do
not discharge unrelated calls in the same caller.

The creature-name pass restores eleven `getArmyName` calls: seven army-command
messages, the recruitment title, both monster-quest text methods and the
campaign creature bonus. Dreamcast retains the seven town calls and the
recruitment call; Mac expands the same range guard and singular/plural lookup
in all eleven operations. The full checkpoint preserves Windows scores and
improves one Mac pair to exact. An unrelated path from `selectArmy` to
`getArmyName` did not represent the town messages; nested reachability alone
must not close a call-site review.

A further 57 inspected lifetime/copy sites and 33 movement/terrain-predicate
sites are already represented. The notes identify the particular member,
base, local or predicate at each address. For example, campaign previews
construct their own setup member, selection messages construct header members
before assigning them, and the water-walking artifact query belongs to
`canWalkOnWater`, not an arbitrary reachable `isFlying` call. These are
call-site dispositions, not claims that the enclosing functions have completed
the whole-source review.

### Identified Mac call leads

The current index has no remaining undisposed Mac `missing_source_call_review`
or `nested_path_review` sites with an identified authored target. This closes
that lead list only: direct source-call presence is not a complete body review,
and indirect calls, unpaired functions and Dreamcast leads remain open.

The per-site review rejects unrelated graph paths. The lobby's retained
315-by-128 bitmap constructor belongs to `CChatWidget::CChatSave`, not the
flag-back `CSaveScreen`; its 61-character editor belongs to `CSaveGameEdit`,
not the player-name editor. The scenario-row widget constructs its own base,
not an icon widget. Coordinate copies at the RMG proxy entries are already
owned by proxy construction, rather than by the later neighbour query.
The stripped shared copy body does not distinguish copy from conversion by
itself. Mac-only ping diagnostics already have explicit source branches.

Mac's pre-host transport cleanup/error dialogs differ from the Windows
DirectPlay flow. Its synchronous music stop corresponds to Windows' queued
`processStopAndPlayMP3 -> threadStopMP3` operation, not the optional
`stopAllSamples` path. The owning source comments record both cases.

### Full-screen forwarding and arithmetic helpers

The next batch restores 27 no-argument `updateScreen` calls in 25 functions.
DC retains the facade; Complete expands its 800-by-600 rectangle forwarding
across translation units. One shared header body preserves those calls and
records the newer placement separately from DC's older `.cpp` ownership.
The native witnesses and reviewed placement are beside that definition.
Two Mac callers carry one-checkpoint abstraction annotations; Windows
`setHeroContext` moves from 97.03% to 93.87%, with its prior peak held in HIST.
Other redraw operations in the same functions still require individual review.

Seven arithmetic operations now call the canonical `min`/`max` wrappers,
and `getPuzzleOrigin` retains its evidenced default point construction before
coordinate assignment. In the long-index combat-distance overload, Mac calls
`labs` for the two displacements; recovering their `long` type raises the Mac
comparison from 55.70% to 76.92% without changing Windows MAX. DC records no
local types there, so this type evidence comes from Mac.

The older DC save-time campaign collection is a reviewed version difference:
Complete collects and prunes crossover pools before saving, through
`completeCurrentMap` and `pruneCrossoverHeroes`. Its human-player list scan
and bounded pool selection replace DC's local-player scan, temporary map-cell
restoration and fixed-pool limit. Those calls must not be cleared by unrelated
reachability through `SavedGameHeader::reset` or normal hero serialization.

The following lifetime pass restores the artifact temporary before
`getFullValue`'s slot loop, the symbol-named `boatCell` initializer before
`createBoat`'s remote branch, and the palette copy temporary in
`Bitmap816::import`. The first two initial values disappear in Complete's
optimized code; their DC constructors/declaration and source-call positions
remain positive evidence. The import overload has no proven retained Complete
counterpart, so its restored lifetime is explicitly attributed to DC while
using the existing Complete palette interfaces. Focused checks preserve
Windows MAX in all three affected units.

DC's extra `GameText[727]` draws during town open/setup/close and combat open
are absent from the corresponding Complete transitions. Their specific native
sites are recorded separately from retained text calls elsewhere in those
functions. The town-close Mac indirect calls at `0x1bf454`, `0x1bf488` and
`0x1bf4f0` dispatch the deleting destructors for the town window, resource
display and network handler; the three source `delete` expressions own them.

### Combat viewport calls

Six further calls restore `scrollTo`/`updateCombatArea` in `doBolt` and
`playAnimation`, and the coordinate update facade in the creature subwindow
and the no-argument combat update. Complete folds scrolling to false and
expands the rectangle forwarding. The existing inclusive bounds and Complete's
animation extent order stay intact. Focused comparisons preserve Windows MAX
in all five checked units.

This review reopened an overly broad console-only disposition for `doBolt`:
the older scrolling implementation differs, but the source call still belongs
in the caller. Other differences have narrower evidence: Complete's hero
subwindow refreshes its own rectangle, generic subwindows omit DC's separate
combat branch, and spell-target/drop-dialog transitions omit the older full
combat refresh. Native positions are recorded beside those operations.

The wandering-monster diplomacy bonus also calls the canonical `min(int,int)`
again. DC retains that call; Mac expands its argument copies and selection.

The coordinate facade changes two Mac comparisons: creature-subwindow show
100% to 98.53% (a larger stack frame) and the no-argument combat update 100%
to 83.06% (width/height instruction scheduling). Their address annotations
record the supported abstraction against the preceding hashes and scores.
Windows scores hold; the canonical calls remain for later byte recovery.

Sprite disposal now retains `ResourceManager::dispose(image)` inside the
Complete virtual override. DC's older free disposal function calls that
facade for each frame, and Mac expands the same virtual resource operation.
The artifact/campaign resource exits already have their specific `TResourcePtr`
owners, including failure returns and the campaign buffer-transfer temporary;
those reviewed operations require no duplicate disposal calls.

The player-slot review also fixes a wrong DC pairing. The symbol-proven
`SetNewPlayerSlot(unsigned long)` at DC `0x13b178` is the existing seat helper
at Mac `0x17c8f0`, formerly named `assignPlayerToOpenHumanSlot`. It now owns
that DC annotation, its original semantic name, and its DC/Mac position
between `doModal` and `setHumanSlot`. The distinct Complete function at
Windows `0x58e700` receives `CNetPlayerInfo*`, registers the player and
reconciles version compatibility; its project name is now
`addPlayerAndUpdateVersion`, with a Complete-only inventory entry.
The seat helper's `getPlayer` call was already present under the wrong identity.

### Popup and adventure cleanup review

The random-map progress window now calls the canonical full-screen update
and sprite-disposal facades. Mac expands both operations; the original
caller null guard remains around sprite disposal. The focused Windows
comparison preserves all scores in `singleselectionpopups`.

The five older popup builders each add a separate `iGPCrDiv.def` button.
Their complete Mac bodies retain the authored text/image/icon widgets but
omit that extra button. The initial virtual calls are the existing dialog
`setup` calls, and all six flagged caption/format lookups already have their
corresponding `getText` calls. The native sites are recorded with the family.

Adventure teardown's twenty resource dispatches and two deleting destructors
are represented by its source disposal and delete expressions. The two
sample loops and the options-window sample reload use Complete resource
disposal with their native guards; DC's sample-specific backend is older.
The extra DC adventure-menu key arm and hero skill-page key controls are
absent from the corresponding Complete dispatchers. In `doCombat`, the
dialog's embedded combat-init payload and implicit destructor replace DC's
separate message pointer and `DestroyMsg`; the native stack address and
destruction sequence are recorded beside the caller.

### Sprite-cache and selection lifetime review

Eighteen retained sprite methods were inspected as complete Mac bodies
(`0x8a604..0x8b03c`). Their palette/frame operations are represented; none
has DC's conditional `SpriteDataReload`. The expanded `setPalette(TPalette16&)`
in `resetPalette` (`0x8a6b8..0x8a6f8`) likewise deletes/copies the palette
without reloading. Other unpaired sprite methods still need caller evidence.

DC's complete `delSprFromCache` (`0x1226d4+0x1d6`) disables file mapping,
walks cached DEF resources, deletes palettes and frames, and clears `Sp_loaded`.
The complete selection destructors on Windows (`0x583b40+0x37d`) and Mac
(`0x17b504+0x3a8`) contain no eviction phase: after owned-resource and widget
cleanup, their save-header branch joins member/base destruction directly.
Complete's existing empty helper and guarded call therefore remain. The
twelve Mac virtual resource-disposal sites and two deleting-destructor sites
in that caller correspond to its existing source disposal/delete expressions.

The full checkpoint after the two progress-dialog changes passed: both
Windows functions remain exact, no source edit lowered Windows MAX, and
the Mac preservation and source ownership/inventory gates passed.

### Remaining full-screen facades and combat dialog redraws

Fourteen further callers now use the existing no-argument `updateScreen`.
Each Mac site loads the window manager and forwards `(0,0,800,600)`;
the source retains the same guard and surrounding operations.

| Source operation | Mac call sites |
| --- | --- |
| Adventure hero/town locator refresh | `0x2d30`, `0x2f38` |
| World-view surface/underground callbacks | `0x208f18`, `0x208ffc` |
| Campaign map-text playback | `0x9798c` |
| Video completion and framed-video startup | `0x25dbe0`, `0x10fa44` |
| Overview dynamic setup | `0x135d3c` |
| Overview hero/visiting-hero/garrison-hero dialogs | `0x1373cc`, `0x137974`, `0x137a18` |
| Overview heroes/towns tabs | `0x13aeb0`, `0x13aee8` |
| Town hall before its modal loop | `0x1cf700` |

Campaign playback's separate strip refresh (`0x97ba4`) passes
`(96,510,608,82)` and remains a rectangular update.

Twelve DC `FullUpdate` sites have no corresponding redraw in Complete's
dialog paths. Mac proceeds directly from the dialog or spell operation to
response handling, cleanup or return: `0x8349c`, `0x834b8`, `0x8350c`,
`0x835bc`, `0x84e84`, `0x85d38`, `0x864a4`, `0x866dc`, `0x877b4`,
`0x88578`, `0x80a0c`, and `0x6e774`. These cover the combat dispatcher,
spell command, surrender/AI decisions, refusal, tower/help dialogs and
placement help. The four dispatcher `InitMouse(0)` additions are likewise
absent at those exits; the existing `resetMouse` calls remain. This finding
does not classify other `FullUpdate` or cursor-control sites.


The full checkpoint passed without a Windows MAX loss or an unreviewed Mac
regression. `TOverviewWindow::windowHandler` improved from 95.9492% to
97.2042% Windows MAX; source ownership and inventory gates remained clean.

### Resource readers and accessor expansions

`NewfullMap::readObjectType` already preserves all four mask-resource reads.
DC `ReadFromSpriteResource` (`0x12249c`) forwards to `LODFile::read` using
the global sprite resource file. Complete's Mac body (`0x126478 + 0x528`)
uses the file returned by `pointToSpriteResource`, including its default-mask
fallback, and calls the explicit-file reader at `0x126594`, `0x1265ac`,
`0x1265c4` and `0x1266c0`. These read width, height, drawing mask and shadow
mask (1, 1, 6 and 6 bytes). The source already calls the canonical
`readFromBitmapResource` for each operation. Its eleven virtual input reads
also correspond to existing direct or nested `readLittleEndianValue` /
`readValue` calls, with the native buffers, counts and failure checks.

The six older `GetBitmap16` leads likewise have corresponding source paths:
`remapGraphics` and `saturateGraphics` use the uncached `loadBitmap16`, while
the bitmap-border constructor and `setImage` use `getBitmap16`. DC's two
main-menu loading phases become one guarded load in Complete (Mac
`0x110120`; the preceding `0x10fd68..0x110138` startup path was inspected).
DC's boolean argument bypasses both cache lookup and insertion; Complete's
cached wrapper (`0x153204`) explicitly calls lookup, loader and insertion.

Existing sprite accessor calls also account for the expanded operations in
Mac `computeExtent` (`0xa81c0`), icon-widget real width/height (`0x10c858`,
`0x10c864`), icon-frame selection (`0x10cf60`) and radar palette colors
(`0x13d44..0x13d64`, `0x13e0c..0x13e24`). The missing DC edges in these
specific operations are obsolete `SpriteDataReload` guards. This does not
classify the remaining sprite drawing and palette-getter leads.


### Campaign launch and text-entry call review

The full Mac `campaignBriefHandler` (`0x679f0 + 0x838`) preserves the
campaign launch through `CampaignHeaderStruct::startScenario` (`0x97e20`)
and `ScenarioStruct::startScenario` (`0x967f8`). Their existing source calls
own `setupFirstPlayer` and the streamed `newMap` operation. DC's direct
`doPreLoadCustomization` call (`0x5a8fc`) moved into `newMap`: Mac `0xd5d10`
calls it under a non-null campaign-context guard before `processOnMapHeroes`.
The scenario caller passes that context at `0x96908`. The handler's
`applyBriefingChoice` is a separate operation, not a replacement for
pre-load customization.

The older special hero-preview branch (heroes 117/123, `un44.def`,
`CHeroDlg`, bitmap/sprite disposal) is absent from Complete's right-click
path. Mac `0x67c20..0x67c80` obtains the widget help text and opens the
normal dialog, as the source does. Both prologue paths, reset-game arguments,
five Mac virtual calls and four DC virtual calls are also represented.
DC's two description `textButton` constructions use GeneralText 39/497;
Complete uses plain `textWidget` labels with the same indices at Mac
`0x6652c` / `0x66818`. These are widget-interface changes, not missing
button calls.

DC `textEntryWidget::main` proves the local `message setFocus` at
`textntry.cpp:448`, inside `!m_hasFocus` and before the parent-window guard.
The canonical constructor only initializes the unused object's fields.
That source declaration and scope are restored; the focused VC6 build
preserves Windows MAX 100%. Mac `0x1b0180..0x1b019c` and Windows
`0x5bb287..0x5bb29f` are compatible with eliminating its unused stores.
This recovers a source lifetime, not a missing runtime action.

The remaining text-entry indirect calls were checked by receiver, vtable
slot, arguments and guard: resource disposal/background deletion, focus
redraw, key filtering, post-edit redraw, text assignment, focus gain/loss,
key dispatch and background capture. All already have canonical source
calls. Mac's complete constructor, destructor, focus/key/main/draw bodies,
display-string setup, text setter, focus callbacks and background methods
were read alongside their source; the saved-background flag, constructor
and bitmap forwarding are already represented by `CTextEntrySave`.

The full checkpoint also preserved text-entry Windows MAX 100%; its Mac
comparison rose from 30.1980% to 30.3218%. Linking, source ownership,
source inventory and data coverage passed without an unreviewed regression.

### Save-dialog operation placement

Complete validates the filename inside the selection dialog, then writes the
save after it closes. Mac `saveValid` (`0x16dc84 + 0x18c`) performs validation;
the outer `saveGame` (`0x187b4 + 0x288`) constructs, runs and destroys the
dialog at `0x18888/0x18894/0x188a0`, then calls the game writer at `0x1894c`
for a nonempty filename. Both paths already exist in source. The older DC
writer calls at `0x13d174` and the save-mode constructor's `0x1352b6` therefore
do not justify adding a second write inside Complete's dialog. Its constructor
backs up headers in memory; that is distinct from the final disk write.

DC's pre-launch `stopAllSamples` (`0x13d69c`) is absent from Complete's
corresponding Mac dispatch (`0x17e808..0x17e814`) and complete `onBeginGame`
body (`0x183698 + 0x51c`). The inspected constructor tail also preserves
three existing virtual operations: disabling widget 186 for a remote non-host,
enabling save-edit auto draw, and disabling widgets 107 through 111 in
save/load mode (`0x175498`, `0x175518`, `0x175584`).

### Review coverage correction

The working call inventory had treated every named entry in
`config/mac/runtime-map.tsv` as library/runtime code. That was too broad:
descriptive labels with unknown ownership, `mac_port` and `mac_platform`
ownership do not establish that the corresponding Windows source operation
has been reconciled. The inventory now retains those destinations as review
leads: 1,580 call sites across 195 targets were reopened. They include actual
runtime operations as well as platform adapters and potential shared helpers;
each still needs operation and caller evidence. This correction is coverage
work, not a claim that those calls are all missing from source.


### Reopened music and rectangle helpers

The full Mac equality body (`0x26b018 + 0x30`) compares bytes through NUL
and returns a boolean. `startMP3` calls it at `0x21946c` and `0x2194b4`;
Windows expands equivalent equality operations at `0x59ad12` and
`0x59ad4b`. Those Windows source comparisons and the two threaded resume-position
searches now call `stringsEqual`,
whose canonical body expresses equality through `strcmp`. Its name is
inferred. The body is visible before the confirmed Windows caller in
`soundmgr.cpp`; the Mac utility-region placement and other Mac callers do
not establish cross-TU Windows expansion or an original header. The source
owns its `MAC_ADDRESS`, replacing the old unknown-runtime label.

The native playback bodies differ in storage and scheduling: Mac compares
against the current record and its three-record cache; Windows compares
against queued and current stream names. This restores the common equality
operation without claiming identical state machines. Mac's preceding ASCII
lowercase copy (`0x219454`) is absent from the complete Windows `startMP3`
body (`0x59acb0 + 0x355`), which compares the original filename. The focused
VC6 build retains Windows MAX 100% for `startMP3`.

Re-reading every branch of `0x20f2dc + 0x74` disproved its prior intersection
label: it stores **minimum** left/top and **maximum** right/bottom. Its Mac
mouse-update caller (`0x2174c4`) passes the new rectangle, saved rectangle
and work output. This is the existing Windows `UnionRect` source operation,
not a missing intersection or a demonstrated platform semantic difference.
The descriptive label and its evidence have been corrected.

Mac `serviceSounds` (`0x219268 + 0x20`) forwards to the three-record music
service (`0x2181a0 + 0x18c`), whose tick-guarded loop changes volume and
stops or pauses records. The entire Windows counterpart (`0x59a7d0 + 0x51`)
instead locks, calls `AIL_serve`, conditionally services one stream, sleeps
and unlocks, exactly as the existing source does. Its separate threaded
fade is already represented in `threadStopMP3`; inserting the Mac record
loop in Windows `serviceSounds` would change the platform implementation.

The remaining music filename copies already use `strcpy`; Mac's copy
primitive (`0x26ae40 + 0x1c`) returns the written terminator, but `startMP3`
does not consume that return at `0x21959c`. Mac's stack file adapter is
constructed once and destroyed at all seven exits. Its path operation
(`0x277bc4`) calls `FSMakeFSSpec`, optional `FSpCreate`, and
`ResolveAliasFile`; it does not itself open the stream. Windows' existing
playback thread formats the path at `0x59a98e` and calls `AIL_open_stream`
at `0x59a9f4`. The adapter lifetime therefore does not require an extra
Windows object; the deferred file/stream operation is already represented.

### Combat startup and victory review

The full Mac victory body (`0x853cc + 0x6e4`) and exact Windows source
preserve Complete's later darken/update after `freeArmies`. DC's earlier
GameText lookup and medium-font draw (`0x6e214`, `0x6e23c`) are absent.
After the local-winner results modal/destructor (`0x859bc`, `0x859cc`),
Mac tests the hero and directly awards experience and rewards. It does not
perform DC's intervening fade, backbuffer recreation and screen fill
(`0x6e74a`, `0x6e766`, `0x6e782`). The earlier darken is a distinct operation;
a nested text or fade call elsewhere is not evidence for those old sites.

Combat startup likewise goes from the battle sample/fade to its three
canonical bitmap allocations (`0x6e2f0`, `0x6e314`, `0x6e338`), without
DC's extra global backbuffer creation at `0x5d686`. Two reopened Mac `bzero`
calls (`0x6e368`, `0x6e374`) already correspond to the source's two `memset`
calls: receiver offsets `+0x4c`/`+0x107` and count `0xbb` match
`m_lastDrawGridShade`/`m_curDrawGridShade` and `COMBAT_GRID_CELLS` exactly.

The seven indirect `startMP3` calls are also accounted for. Native array
construction at `0x2198f8..0x21990c` builds three music records; their
constructor (`0x2199b8`) calls the stream constructor (`0x27dd70`), which
installs the loader-proven vtable at data-section `0x6050c`. Its slots identify
open (`+0xc`, `0x27de30`), close (`+0x10`, `0x27df70`), play (`+0x18`,
`0x27dff0`), resume (`+0x24`, `0x27e1e8`) and volume (`+0x28`, `0x27e240`).
The complete callees use Classic Mac file/sound APIs. The corresponding
Windows operations already live in `resumeStream` and its playback thread,
including close/open, loop count, saved-position restoration, volume and
start. Mac's three-record cache and tick-driven fade are distinct from the
Windows single-stream/thread sequence; equal call counts were not used as
proof of correspondence.

The full checkpoint passed: no Windows MAX loss, no unreviewed Mac
regression, no unresolved link symbols, and clean source ownership,
source inventory and data coverage gates. All four restored equality calls
preserve their callers' previous Windows scores.

### Saved-header cleanup on serialization exits

All 304 calls to the four campaign-vector cleanup wrappers in Mac
`game::load` and `game::save` were checked individually for receiver and
deleting flag. Each function has 38 calls per member. The `SavedGameHeader`
local is at stack `+0x168` in load and `+0xe8` in save; its constructor
(`0xcf490`) constructs `SCampaign` at header `+0x4a8`. Campaign construction
(`0x98064`) establishes the following members:

| Campaign member | Mac offset | Cleanup chain |
| --- | --- | --- |
| `m_carryOverHeroes` | `+0x30` | `0x98320` → `0x6b7fc` → `0x6b9d8` |
| `m_carryoverArtifact` | `+0x3c` | `0x98264` → `0x6b860` → `0x6b964` |
| `m_mapScores` | `+0x48` | `0x981a8` → `0x6b8c4` → `0x6b928` |
| `m_assignedCarryover` | `+0x54` | `0x67278` → `0x68da4` → `0x68f40` |

Every inspected caller passes the identified member address and flag zero.
The complete destructor chains destroy nested hero/artifact vectors or
clear POD map-score/hero-ID elements, then free vector storage. The source
already owns the same four `std::vector` members through local
`SavedGameHeader saved`; implicit C++ destruction supplies those operations
on every return. No explicit destructor call or new game helper is needed.
This accounts for these cleanup sites, not the complete load/save review.

### Host queries and remote cleanup

Seven Mac host-query sites already have source calls to `CDPlay::isHost`:
lobby startup (`0x11025c`), main-menu widget hiding (`0x11cb60`), selection
host forwarding (`0x182650`), the ready-dialog completion (`0x21370c`),
player-drop recovery (`0x213fbc`) and both ready-dialog message guards
(`0x215e70`, `0x216078`). The ready-dialog completion query is inside the
existing nested `CWaitForReadyPlayersDlg::wait`. The Mac helper (`0x211154`)
reads a platform-global host byte; Windows' canonical method reads
`m_isHost`. The native guards, arguments and corresponding source calls
were checked individually.

The eighth query, in `remoteCleanup` (`0x212aac`), guards a Mac-only
GameRanger host-closed notification. Its complete helper builds a
`GRgr/HCls` Apple Event; the preceding notification builds `GRgr/GEnd`
with `GTim`, `Scor` and `AScr` parameters. Both send and dispose their Apple
Event descriptors. Complete Windows `remoteCleanup` (`0x5544b0 + 0xaa`)
contains neither notification nor that host guard. Its existing virtual
player destruction, session close and DirectPlay deletion also account for
DC's three indirect calls at `0x11ce9c`, `0x11cea8`, `0x11ceb8`, by receiver,
vtable slot (`+0x20`, `+0x18`, `+0`) and deleting flag.

Mac additionally destroys its separate global network transport at
`0x212afc`. Its destructor calls `0x27afb4`, which shuts down the connection,
destroys service state, frees its buffer, removes the receiver from a global
registry and updates the service reference count. Windows owns its transport
through the DirectPlay interface: `closeSession` calls `Close`, and the
canonical `CDPlay` destructor calls `Release`. Those platform ownership
paths are already present; no second Windows transport object is implied.

### Load/save stream operations and older header interfaces

The remaining 56 Mac indirect calls in `game::load` and `game::save` were
checked by receiver, vtable slot, buffer, byte count and surrounding guards.
They are the existing abstract-file reads (`+0xc`) and writes (`+0x10`):
versioned artifact/skill arrays, hero availability and membership masks,
twelve staged scalars, six fixed arrays, the reserved word and map-extra
plane. The four byte/word staging locals remain distinct. Both native
functions request 32 unique-system-ID bytes but accept eight; the source
preserves that unusual bound. No additional retained game helper was found
at these virtual stream sites.

DC's `GetSaveGameHeaders` call at `0xa83e4` has an ownership replacement:
Complete loads a local `SavedGameHeader`, checks it, then calls the existing
`applySavedGameHeader`. Its `LoadTownPool` call at `0xa8542` is also already
present; Complete adds the saved version passed to each town reader.
These are reviewed interface changes, not absent source operations.

Three formerly direct DC operations now belong to the saved snapshot.
Mac `SavedGameHeader::save` calls map-header save at `0xcfad4` and setup
save at `0xcfaf0`, preserving their failure checks. `reset` calls
`playerData::isHuman` for eight players at `0xcf9ec`; `save` writes that
32-byte flag array at `0xcfbec`. Those native paths verify the existing
nested source calls corresponding to DC `0xa8d14`, `0xa8d30`, `0xa92a2`.

DC's three `clear_carryover_pool` calls (`0xa8dea`, `0xa8dfa`, `0xa8e12`)
reset one or both fixed hero pools. The complete callee fills raw hero and
assignment storage with `-1`, then clears the pool count. Its artifact
requirement reset at `0xa9074` stores an artifact sentinel and guard byte
in another retired fixed record. Complete instead collects and prunes
constructed hero/artifact vectors before `saveGame`, through the existing
`completeCurrentMap`/`pruneCrossoverHeroes` path. Mac's save prefix proceeds
through snapshot construction, reset and serialization without those raw
resets. Neither an extra vector reset nor the removed requirement object is
supported in the Windows save body.

The full `SGameSetupOptions` save/load bodies and `SavedGameHeader` save/load
bodies also preserve their remaining 51 virtual stream and cleanup sites.
Setup serialization deliberately transfers the color array twice; both native
addresses and the source agree. Its eight hero IDs use the existing scalar
writer/versioned `loadHeroId` reader. Snapshot scalar reads/writes already
use the canonical helpers, and the two conditional deleting-destructor sites
in snapshot load are owned by its `auto_ptr` for internally opened input.

Mac's three filename-copy calls in snapshot load are a platform path detail.
The complete `0x26ae40` leaf copies through NUL and returns its terminator.
At `0xcfc80` the return is unused while preserving the original filename;
`0xcfc8c`/`0xcfc9c` chain the loader-resolved `:games:` prefix and filename
before constructing the compressed stream. Windows `0x4bc7ab..0x4bc834`
uses the existing `openedName` assignment, `_chdir("games")`, stream
construction and `_chdir("..")` sequence instead.

Finally, the 24 vector calls in snapshot reset/application are members of
implicit campaign assignment, checked in both directions. The data accessors
return the vector's pointer slot at `+8`; the four complete assignment bodies
copy/grow/shrink hero pools (`0x6b0b8`), artifact pools (`0x6b354`), scenario
scores (`0x6b5f0`) and assigned hero IDs (`0x682fc`). Caller member offsets
and element strides identify each operation. The existing `m_campaign`
assignments own these library calls; adding game wrappers would duplicate
the represented operation.

### Save-game transfer file and network operations

The full Mac transmit/receive bodies and their remaining 71 queued call
sites were reviewed against the corresponding Windows operations. The
three zero fills belong to packet creation, `playerDone[8]` and the
`blockReceived` allocation; packet creation already owns its fill through
`CGameTransmitMainMsg::createMsg`.

Mac's file adapter separates path resolution/creation from opening its data
fork. The complete callees and loader imports identify `FSMakeFSSpec`,
`FSpCreate`, `ResolveAliasFile`, `FSpOpenDF`, `FSRead`, `FSWrite`, `GetEOF`
and `FSpDelete`. Original/current/diff-file buffers, counts and failure
branches match the existing `File` calls. Final transfer input/output uses
CRT descriptors in Windows (`0x4cb288..0x4cb2d5`,
`0x4cc664..0x4cc6b9`); the extra Mac adapter destructors run after the
corresponding handle close. They do not require another Windows owner.
The receiver's separate Mac path/open failure checks remain a documented
platform difference from Windows' single `File::open` guard.

Seven Mac idle-service calls walk the application callback list at `+0x1828`;
three clock-refresh calls use `Microseconds` and update the cached Mac clock.
The Windows transfer loop entries (`0x4cb448`, `0x4cb563`, `0x4cbf13`)
retain `pollSound`/`checkDoMain` directly, as the source does. Its post-receive
file/UI sequence has no corresponding Mac callback-list service.

The three Mac no-op player-destruction calls and DC vtable-slot `+0x20`
calls correspond to existing `g_dPlay->destroyPlayer` calls, proved at
Windows `0x4cb970`, `0x4cbaae`, `0x4cc38f`. The subsequent drop handling
and notification keep their distinct source operations. Seven DC text
lookups use indices 99, 82, 100, 15, 329, 432 and 471 for the same messages
and guards as the current `getText` calls; no accessor substitution is needed.

### Resource-cache backend boundaries

The complete Mac cache insert/remove/find bodies at `0x151ff4`, `0x15208c`
and `0x1520e8` operate on 16,384 eight-byte hash/resource rows. Find compares
a case-sensitive name hash, then calls `stringsEqual`; insertion finds an
empty row, while removal searches by resource pointer. Windows instead
retains the current case-insensitive `TCacheMap` operations: insertion at
`0x5594df`, lower-bound/comparison in `0x55e330`, and iterator erasure at
`0x55d17f`. The three source callers already preserve `insert`, `find` and
`erase`, plus reference-count changes.

Disposal also has a native behavioral distinction: Mac ignores the removal
result before deleting a zero-reference resource; Windows deletes only after
a successful tree lookup. The source preserves the Windows guard and virtual
`delete this` call (`0x55d18a`; Mac's deleting slot is called at `0x154820`).
The Mac array and its hash/equality helpers must not be inserted into this
different Windows cache implementation.

### Map and saved-game victory/loss condition streams

The six complete Mac condition serializers (`0xd9320` through `0xda6b4`)
account for another 103 queued virtual stream calls. Their receivers, transfer
widths, field destinations and short-transfer guards agree with the existing
source operations, apart from the Windows-specific check below. Two- and
four-byte values use little-endian conversion on Mac; Windows reads or writes
its native little-endian representation. Existing scalar helpers remain in
the saved-game loaders and loss-condition writer.

The map-format defeat-hero condition reads coordinates. Saved games instead
use an ID, through the already recovered `loadHeroId` call at Mac `0xda0a8`.
Loss-condition loading reads coordinates only for save version `0x10` and
otherwise retains `loadHeroIdShort` at `0xda64c`. The source preserves these
version gates, the distinct byte/short ID formats and both helper calls.
The map victory reader also retains `validateVictoryLossConditions` at
`0xd9884`.

Mac's map loss-condition reader ignores the two-byte time-limit read result
at `0xda3ac`. Windows explicitly checks that result at `0x4c3cac` and returns
`-1` on a short read; the source correctly preserves that guard. The saved-game
loss reader and writer check the time-limit transfer in Mac too. This batch
found no additional missing game helper or source call; resolving the virtual
stream sites does not establish whole-corpus body-review completion.
