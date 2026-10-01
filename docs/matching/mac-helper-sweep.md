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

The working inventory covers 139 game translation units, 5,550 canonical source
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

### Shared packed-bit writer in saved-map headers

Reviewing the complete `NewSMapHeader::read`, `save`, `load`, `get` and
`readString` Mac bodies exposed a missing call in `save`. Its final player
availability loop was a scalar accumulator in the recovered source. Mac
`0xdbe4c..0xdbeb0` instead clears a one-byte array, tests eight bits, indexes
the byte with `i >> 3`, sets `1 << (i & 7)` and writes the packed byte.
Windows `0x4c544d..0x4c5497` retains the same indexed packing.

This is the eight-bit expansion of the existing random-map
`encodePackedBits`/`writePackedBits` operation (for example, the 70-bit Mac
expansion at `0x24f810..0x24f890`). Both canonical templates now live in
`packed_bits.h`, beside the readers, and `NewSMapHeader::save` calls the
writer. The RMG calls are preserved. Cross-TU expansion
supports a shared header; the original names and filename remain unknown.
The initial VC6 check moves this caller from 95.4451% to 88.34%; that measured
difference does not justify restoring the scalar approximation.

The other 83 queued sites in these five bodies are represented. Stream
receivers, transfer sizes, version guards and short-read/write checks were
reviewed individually. Placeholder clearing is the trivial four-byte vector
clear at `0x68f40`; hero setup clearing uses `0x68f78`, whose recursive
`0x68688` destroys each node's string and frees the node. The three MSL
replacement calls pass the erase arguments to `0x1c910`. The zero fill in
`readString` owns exactly `length + 1` allocated bytes before the payload read.

Mac `get` chains four terminating copies into `:path:filename`, with the
colon and `rb` mode proved by PEF TOC data. Windows constructs the existing
`std::string(path)`, backslash and filename sequence before `TGzFile`.
Finally, DC's rejected-version text lookup at `0xaf6e6` uses index 431,
matching the Mac caller and existing `getText` expression.

The same review found the twelve-bit expansions in `playerData::load`
(`0xccce0..0xccd60`) and `save` (`0xcd1f0..0xcd258`). Both now use the shared
packed-bit helpers. Loading retains its version-37 guard and returned bitset
copy; writing retains the unchecked two-byte transfer. The canonical encoder
uses `fill_n`, preserving Mac's repeated const-zero loads without introducing
the CRT call observed with `memset`. Windows `playerData::load` remains exact;
`playerData::save` improves from 99.9557% to all 874 bytes exact. RMG's source
MAX remains held; its current emitted comparison moves from 96.3419% to
94.1690% after the shared encoder change.

### Remaining player, pool and rumour stream operations

The map-player slot reader and three hero-ID readers account for twenty
additional sites. Native slot reads preserve the RoE/AB branches, the unused
post-AB byte, the little-endian hero count and the unused string local before
vector resize. Its vector clear at `0x68ed0` destroys each eight-byte record's
string and resets the size. The byte hero readers map `0xff` to `-1` before
their map/save-version remaps; the signed-short reader has no byte-sentinel
test. Those distinctions already exist in the canonical source helpers.

The generator pair, ten sign/mine/garrison/boat/obelisk pool serializers and
four string/rumour serializers account for another 83 sites. Each native
transfer was checked for receiver, size, consumer and error guard. The mine
loader's legacy signed type/count arm already initializes/adds guards, while
newer saves retain the army loader. Garrison removable-troop flags retain
their version gate and unchecked scalar transfer. Boat serializers keep the
following boat load/save call and its failure guard. The rumour-state buffer
transfers 256 bytes but deliberately checks only for four in both native
directions, as the recovered source does. The remaining 36 scalar/buffer
sites in player load/save likewise already preserve their transfers and guards.

Refreshing the graph also surfaced twelve indirect source paths that needed
site-level reconciliation. Prayer's target-time call belongs to the
no-argument `getAITargetTime` wrapper, not its separate speed-value calculation.
The button timer belongs to `elapsedSince`, not `select`. Campaign and high-score
message calls belong to the local `show`, `hide` and `setVisible` operations,
with command/mask pairs `5/6`, `6/6`, `5/4` and `6/4`; reaching the same callee
through a later window update did not establish those operations.

The refreshed RMG body scan still reports 201 Clang diagnostics involving
legacy VC6 local lookup and reference rules. The unchanged parent source and
header reproduce the same diagnostics. Ownership parsing reports no gaps,
but these body-analysis gaps remain explicit; this batch does not certify
complete RMG body coverage.

The full checkpoint passed with 1,530 Mac comparisons preserved, no unresolved
link symbols, and no source ownership or inventory violations. Player load
and save are both exact. The header writer's 95.4451% historical peak remains
recorded beside its new 88.3414% current score; RMG's unchanged-source current
score is 94.1690%. Shared-header collateral also moved `giveArtifact` from
95.4049% to 95.3158% and `transmitSaveGame` from 97.4264% to 97.4042%; their
previous MAX values remain recorded.

### Text access and additional fixed stream bands

Thirteen DC text-call sites were checked against their literal indices,
branches and consumers. Player name initialization/comparison use 469;
save-name exclusions use 77 and 109; the seven special-rumour branches use
209–212, 264, 265 and 213. These already retain their `getText` calls.
`setupOrigData` uses 12 for its initial filename copy. DC repeats that copy
through `gpGame` at `0xaa0ac`; the complete Windows `0x4bf1a0+0x183` and Mac
`0xd47e4+0x3fc` bodies instead have only the initial bounded copy and explicit
terminator. The older second reset is a version difference, documented beside
the owning source, rather than a missing Windows call.

Twelve further Mac calls in black-market and creature-bank serialization
already preserve their stream operations. Black markets stage a signed-byte
count, transfer `count * 0x1c` contiguous bytes and test unsigned short counts.
The creature-bank reader checks equality for its `0x38`, `0x1c`, four-byte and
one-byte fixed bands; the writer deliberately ignores all four counts. Both
then call their existing artifact-vector helper. These call-site dispositions
do not certify complete body review of their callers.

### Further packed-mask expansions

Reviewing the bodies beyond their already-accounted stream calls exposed six
more missing shared calls. `game::load` now uses `readPackedBits<8>` for each
hero's player mask: Mac `0xd1210..0xd129c` constructs the local mask, reads one
byte, decodes it and copies the returned temporary before member assignment.
`game::save` uses `writePackedBits` for the corresponding expansion at
`0xd32fc..0xd3368`. The shared reader also delegates its array read to
`readValue`, preserving the existing lower-level source path.

`readMapHeroSetups` now uses `decodePackedBits` for the nine-byte spell mask
at Mac `0xd9234..0xd9290`. This writes the existing member, without adding a
new bitset or returned temporary. The other sixteen stream sites in its full
`0xd8ec0+0x460` body already use their value or little-endian helpers. The
signed sex sentinel, signed artifact IDs, narrowed backpack count, complete
artifact assignments and returned name string were checked individually.

`hero::save` now shares the 48-bit writer expanded at `0xf3d84..0xf3e08`.
Its native zero fill repeatedly reads a constant before each byte store.
`writeRmgObjectPrototype` shares the two ten-bit terrain writers expanded at
`0x24fb44..0x24fc44`. Their retained callees `0x1c82c` and `0x12b128` are
bounded bitset tests, confirmed from their complete bodies. The separate
coordinate-mask loops remain a distinct lead because they traverse reversed
coordinates through the existing object predicates.

Eight additional town/vector stream sites were inspected. Town helpers
transfer an unsigned-byte count, loop over the live vector size, and propagate
the negative element result. Point/university vector writers transfer a
signed-short count and a contiguous four-/sixteen-byte element payload;
creature-bank object vectors retain their element load/save calls and bool
failure checks. Their source operations are already present.

The subsequent town/mapcell pass below restores their mask operations while
retaining town's padded 70-byte save format. This does not close the complete
helper family or the whole-function review.

The initial targeted VC6 check preserved `hero::save` at 100% and raised
`NewSMapHeader::read` from 92.62% to 94.14%. Keeping the shared operations
moves `game::load` from 100% to 93.60%, `game::save` from 92.80% to 86.38%,
`readMapHeroSetups` from 100% to 88.79%, and `writeRmgObjectPrototype` from
100% to 85.27%. These are measured code-generation differences, not reasons
to paste the helper bodies back into their callers.


### Town and map-object mask operations

Seventeen more source calls now share the existing packed-bit helpers: town
load/save, the map town's two spell masks, the map hero's spell mask, four
object-definition mask decoders, four saved-object mask decoders and four
saved-object mask encoders. Mac retains the same bounded bitset operations:
48-bit set/test at `0xe98b8`/`0x1c82c`, and 70-bit set/test at
`0xe9a84`/`0xe99a0`. Their complete native bodies were inspected separately
from their callers.

The object-definition reader keeps its two input sources: draw/shadow masks
come from resources; passable/trigger masks come from the map stream. All four
are six-byte buffers decoded into existing members. The saved-object pair
retains its checked six-byte transfers and caller-owned reused buffer. The
map-town reader keeps the RoE branch that clears nine bytes and skips the
fixed-spell decoder, the unchecked newer fixed-spell read, and the checked
ordinary spell read. Map-hero custom spells retain their unchecked nine-byte
read and existing destination member.

Town uses 70 serialized bytes for 70 bits, not nine bytes. Native
`0x1b271c` clears the entire buffer before the checked write at `0x1b2794`;
61 trailing bytes remain zero. The canonical encoder now deduces the complete
buffer type and clears `sizeof(packed)`. This preserves both padded and compact
callers. VC6 rejects direct array-extent template deduction with C2265/C2783;
deducing `Buffer&`, as the existing value reader does, compiles on both
compilers. A runtime size parameter also compiled but made the native buffer
extent opaque within the emitted encoder.

The same review restored nine `readLittleEndianValue` calls in
`readHeroData`: identifier, name length, both experience arms, secondary-skill
count, troop count, equipped/backpack artifact IDs and backpack count. Native
`lwbrx`/`lhbrx` operations immediately follow the unchecked reads. The returned
value helper preserves that unchecked contract, the signed short values and
the subsequent byte narrowing of the backpack count. Three RMG prototype
writes now use `writeLittleEndianValue` for name length, object type and
subtype; Mac stores each through `stwbrx` before its unchecked four-byte write.

The RMG coordinate-mask loops already call `isPassableCell`/`isTriggerCell`
through `getBitPos`. Native reverses both coordinates and serializes bit
positions zero through 47. Those predicate calls remain; replacing their
complete path with a direct field mask would discard supported helpers.
The missing-object-mask warning likewise already has its source call:
`MessageBoxA` maps through the ordinary platform boundary to the Mac dialog
at `0x20f0b0`, whose complete body constructs resource 1001 and passes the
message and title to the shared display routine.

Site-level review also checked town's remaining 51 non-mask transfers, ten
object load/save scalar transfers, the map-town scalar/flag/event reads, the
hero-map stream bands and the RMG prototype's remaining transfers. Buffer
widths, sign extension, version branches and deliberately ignored counts were
checked individually. These dispositions do not certify complete caller-body
review.

### Current validation debt

The previous full checkpoint fixed an incorrect generated constructor claim:
Windows `0x4c3090` takes a string directly, while `0x4044e0` copies the string
at `logic_error+0xc`. `CLASS_NONCOPY_CTOR` prevents the former address from
binding to the latter when only the copy constructor is emitted. Three game
library enrollments now retain raw generated claims because their bodies no
longer emit. That checkpoint linked without unresolved symbols and passed
source ownership/inventory checks, but failed Mac preservation for the RMG
prototype writer; it was not a green checkpoint.

The current six-TU fast check compiles successfully. The canonical loops now
retain signed division/remainder, as Mac `0x126e08`/`0x126a64`, Windows
`loadObjectType` and the DC signed loop local support. Map-town read is 85.80%,
map-hero read 90.93%, object-definition read 70.88%, saved-object write 94.42%
and saved-object read 65.62%. Town load/save are both 99.87%, the RMG prototype
writer is 90.28%, and `readMapHeroSetups` is 96.20%. These are projected
matching scores, not a full checkpoint. Hero's unchanged-source MAX remains
held, but its emitted comparison moves from 100% to 98.10%; its native encoder
uses unsigned shifts, a caller-shape distinction still to resolve. In particular, Windows `loadObjectType` still expands the shared
decoder, but retains four `bitset<48>::set` calls where retail expands those
bodies and retains `_Xran`; equal aggregate call counts hide that difference.

Three previously available Mac comparisons are now unavailable: the RMG
prototype writer and town load/save retain unpaired helper calls. The other
available comparisons in the targeted report have no score drops. No invented
Mac address, forced inline qualifier or preservation waiver was added. These
availability failures remain explicit comparison gaps. The current workflow
checkpoints the targeted Windows measurements without requiring a full build;
no successful full Mac-preservation checkpoint is claimed. The refreshed source scan finds 5,550 canonical definitions
and no ownership gaps; RMG still has its 201 pre-existing Clang body-analysis
diagnostics, which remain explicit coverage gaps.

The working inventory now has 5,979 open native call-site leads and 2,017
explicit operation-site notes. There are also three source-body mapping gaps
in unpaired template members (`bitset_iterator` default construction and
addition, and `TRmgCoordinatePoint::operator+=`). Neither those gaps nor the
359 functions without native pairing are treated as completed body reviews.


### Black-box and treasure stream calls

The map reader's full Mac body at `0x121878+0x570` decodes its experience,
mana, resource dwords, newer artifact shorts and troop-count shorts as little
endian. Those five operations now use the existing scalar reader. The
artifact read remains unchecked; the other four operations keep their count
guards. Both canonical creature-ID readers also now decode their unchecked
short arm through that helper, matching `lhbrx`/`extsh` at `0x1217f4` and
`0x121860` while keeping their different version tests.

The saved-game reader and writer use native-order reward dwords and troop
counts. Creature IDs are an explicit exception: `saveBlackBox` uses `sthbrx`
at `0x1225ec`, so that unchecked transfer now calls the existing endian
writer. The following checked troop-count write remains native order. The
signed vector counts, signed skill/spell IDs, and unchecked unsigned artifact
byte in `loadBlackBox` remain distinct. Map-file empty lists call `clear`;
saved-game lists always call `resize`, including for zero.

`readTreasureData` now calls the canonical `readMapCreatureId` for the same
expanded version branch at Mac `0x120f9c..0x121004`, followed by the checked
endian troop-count reader. Its forward declaration preserves the ordinary
helper's later source position. Save/load treasure helpers already retain
their string and army calls; army results are deliberately ignored, and the
helpers themselves return only zero or minus one.

This batch restores ten source calls and accounts for 64 native call-site
leads after inspecting the complete callers and the four-byte vector-clear
callee. The targeted mapcell build keeps `saveBlackBox` and `readGarrisonData`
at 100%, raises `loadBlackBox` from 91.4115% to 91.4222%, and measures
`readBlackBox` at 91.1309% and `readTreasureData` at 89.4321%. The latter's Mac
comparison now retains an unpaired `readLittleEndianValue<short>` call; this
additional comparison gap is recorded, with no fabricated target address or
inline qualifier. Windows score losses remain visible in the checkpoint.

### Monster, event and random-dwelling readers

`readMonsterData` now uses the canonical endian readers for its identifier,
quantity, resource and newer artifact fields. Mac `0x123b78`/`0x123d18`
performs the dword conversions, and `0x123bac`/`0x123d94` the short
conversions. Identifier and artifact reads remain unchecked; quantity and
resource reads retain their unsigned count guards. The separate identifier
staging local, signed legacy artifact byte, temporary MonsterData lifetime,
version source and default disposition behavior are preserved.

The two random-dwelling readers at `0x125830` and `0x125964` also decode
castle IDs and conditional faction masks through the existing unchecked
endian readers. The faction-only variant has no multibyte transfer: it derives
its mask from this map's object-type table. All three keep their existing
byte readers, ignored padding, object association and vector insertion.

The full event-reader body at `0x122bbc+0x3b0` already represents its game
calls. The extra destructor targets at `0xbe748` and `0x89c84` clear trivial
four-byte vector elements and free their buffers; six exits destroy the
local BlackBoxData's spell and artifact members with a zero delete-object
flag. These are implicit member lifetimes, not missing explicit calls.

This batch restores eight endian-helper uses and reviews 40 native call-site
leads. The targeted mapcell build succeeds and raises the Windows monster
reader from 98.38% to 99.96%. Its previously available Mac comparison
(42.5703%) is now unavailable because of an unpaired value-returning
`readLittleEndianValue<int>` call. This is an additional comparison loss,
not a successful Mac-preservation checkpoint. No address or inline qualifier
was invented to conceal it. The working review now has 5,939 open native
call-site leads and 2,057 explicit operation-site notes; the source graph is
still the earlier checkpoint and does not certify current complete bodies.

### Layer records and witch-hut mask

The complete Mac layer bodies at `0x11fd20`, `0x1200b4` and `0x1206c0`
already retain the cell lookup, scalar transfers and saved-cell upgrade path
in our source. Their 33 stream sites now have individual dispositions. Map
input has seven checked bytes per cell, with its seventh byte unpacked into
flags and derived terrain state. Saved layers use native-order scalar fields,
including their object vectors; they must not be changed to map-file endian
reads. The per-element vector transfers now call the existing reference
`writeScalar`/`readValue`, preserving their actual element address and live
vector-size loop. Neither transfer is a bulk range operation.

The witch-hut reader's newer-format arm performs an unchecked dword read
followed by `lwbrx` at Mac `0x125814`. It now calls the canonical endian
reader, while the RoE arm keeps its constant mask without consuming input.

This batch restores three helper uses and reviews 34 native call-site leads.
The targeted build succeeds; `saveMapLayer` stays at 100%, and `loadMapLayer`
moves from 100% to 94.8593%, with HIST preserved. The named call comparison
shows all thirteen stream calls and `upgradeCellExtraInfo` still represented;
the candidate now retains vector `copy` and `_Destroy` where retail expands
them. The helper is kept while that compiler difference remains open. No
additional Mac comparison availability loss occurs in this batch. The working
review has 5,905 open native call-site leads and 2,091 explicit operation-site
notes; these counts do not establish complete body coverage.

### Resource quantity and garrison troops

Two more previously exact Windows callers now retain their endian-reader
operations. `readResourceData` checks its dword transfer before Mac
`0x121744` decodes the quantity, then inserts its low nineteen bits without
disturbing the custom-treasure index. `readGarrisonData` checks each short
before `lhbrx` at `0x1254ec` and signed widening into the army count. Its
existing versioned creature-ID helper remains separate, and its two version
sources remain distinct. Both use the existing checked reference reader.

The full artifact, scroll, resource and garrison bodies account for twelve
more stream sites. Four inspected member-destructor sites in
`readBlackBoxData` are already implicit in its local BlackBoxData lifetime;
six more stream sites in the grail, shrine and shipyard readers are already
represented. The source preserves the shrine's signed-byte widening, the
shipyard's post-padding boat-coordinate initialization, and the garrison's
pool insertion before its final padding check.

The targeted build succeeds and keeps `readGarrisonData` at 100%.
`readResourceData` now measures 96.1611%, with its earlier exact peak retained.
Its named call comparison retains all eight retail calls and adds a
`basic_string::_Tidy` call where retail expands it. The recovered quantity
reader remains in source. No further Mac comparison availability loss occurs
in this batch. With these 22 reviewed sites, the working queue has 5,883 open
native call-site leads and 2,113 explicit operation-site notes. The broader
all-function review remains incomplete.

### Timed events, town-event arrays and building masks

Mac retains a single 28-byte resource transfer followed by seven dword
conversions in both `TTimedEvent::read` (`0x11d64c..0x11d6cc`) and `load`
(`0x11dc2c..0x11dca0`). The writer first copies to a temporary through the
reviewed BlockMoveData import, converts all seven elements, then issues one
28-byte write. Windows writes the original member array directly. The new
ordinary `readLittleEndianValues`, `writeLittleEndianValues` and shared
conversion body preserve those operations; a short read leaves the partial
buffer unconverted. These inferred helpers currently live in mapcell because
only same-TU expansions are established. The source uses memcpy for the
writer's equivalent temporary copy, without claiming the original API name.

All six timed-event first-day/interval transfers now use the existing scalar
endian helpers. Map input increments the decoded day; saved input does not.
Both timed-event list readers also decode their count before resizing. The
town-event saved payload and its list count remain native order, distinct
from the timed base payload. Derived town read/save/load deliberately ignore
their base result, as both native builds show.

`TTownEvent::read` now decodes its seven-short generator band with the shared
bulk reader. Its six-byte building-mask copy and the built/disabled masks in
`readTownData` use one `copyLittleEndianMask48` body. Mac zeroes, copies and
reverses both words; Windows copies six bytes and preserves the upper sixteen
bits (the event constructor initializes them separately). Town input also
retains the scalar helpers for its unchecked identifier and checked troop
and event counts; the earlier wrapper score loss no longer leaves those
operations pasted into the caller.

This family restores eighteen caller helper uses and adds four canonical
bodies. Thirty-three new native stream-site dispositions and five updated
town dispositions record widths, byte order, version gates and ignored
results. A targeted mapcell AST refresh finds the new bodies and caller uses
with no diagnostics. Its dependent template calls are still a scanner gap;
their written bodies were inspected directly. The full inventory/native graph
has not been regenerated and must not be described as current complete-body
coverage. The queue has 5,850 open native sites and 2,146 operation notes.

The targeted VC6 and CodeWarrior build succeeds. Timed-event save/load and
both timed-list readers retain Windows 100%. Timed-event map input is 73.50%
(previously 100%), and town input is 81.44% (previously 85.7956%); prior peaks
are retained. The map reader keeps all fourteen retail calls and additionally
retains string `_Tidy` on one failure path. The unchanged list-writer sources
also have observed CUR losses: timed-list save 0%, town-list save 43.0408%,
with their MAX/HIST held at 100%. Both bodies still emit; named call reports
show VC6 now expands `TTimedEvent::save` where retail retains its call. These
are compiler differences to resolve with the canonical source calls kept.
There is no additional Mac comparison availability loss in this batch.

### Remaining mine, monster and scholar scalar operations

Generator ownership uses an exact-one-byte count test at Mac `0x120c94`;
mine ownership uses unsigned at-least-one at `0x12365c`. Both now call the
reference scalar reader while preserving those distinct guards and their
separate locals. The abandoned-mine mask remains a one-byte partial read
into the Windows local, consumed only through its low eight bits; it is not
silently widened to a four-byte transfer. Its population/ordinal selection,
trigger lookup and pool insertion already have source counterparts.

Saved monster records use native-order resource dwords. Their writer's local
resource and artifact transfers now call `writeScalar`, and the list reader's
signed short count uses `readValue`. The saved artifact reader remains
unchecked and unsigned, mapping 255 to `ARTIFACT_NONE`; it must not become a
signed-byte read. Hero placeholders likewise retain unchecked unsigned owner,
hero and conditional power-rating bytes. Sign input retains its insertion
before the trailing padding check and its implicit string cleanup.

The scholar reader now uses the reference scalar helper for both signed
bytes. Its random-candidate vectors, scoped destruction and existing
`getScholarAward` call account for the remaining native operations. The spell
vector destructor at `0xbe748` receives a zero delete-object flag; it is not
an omitted explicit game call. This batch restores seven scalar-helper uses
and records twenty additional native site dispositions.

The targeted build succeeds with no Windows MAX changes in this scalar
batch. Mac `saveMonsterData` loses its previously available 68.9815%
comparison because the emitted body retains an unpaired `writeScalar<int>`
call at `+0x4c`. The helper remains, with no fabricated address or forced
inline qualifier. The working queue now has 5,830 open native call-site leads
and 2,166 operation notes. The latest targeted AST snapshot predates these
seven scalar substitutions; the four new event helper bodies remain in its
separate inventory until the next full source-graph reconciliation.

### Object records and saved pool counts

`saveObject` now calls the scalar writer for its three byte coordinates and
native-order unsigned-short type index, preserving the shared byte local and
separate short local seen at Mac `0x126268`. `saveMapObjects` likewise retains
scalar writes for its two native-order dword counts. The treasure and black-box
list readers now use the reference reader for their checked signed-short
counts. These are eight recovered helper uses; their widths, result guards,
loop bounds and success values are unchanged.

The object map reader already uses the little-endian dword reader before
narrowing its type index, whereas saved objects use a native-order short.
The seer, treasure and black-box list counts are native order; quest guards
are the unchecked unsigned-short little-endian exception. Seer and guard
loads already register nonnull quests, with the guard loop using its cached
count and the seer loop rereading size. Twenty-two new stream-site notes and
one updated treasure note record these distinctions. The working queue has
5,808 open sites and 2,188 explicit operation notes; this remains partial
call-site accounting, not complete function-body coverage.

The targeted build succeeds. Windows `saveObject`, `loadTreasureList` and
`loadBlackBoxList` remain 100%. `saveMapObjects` falls from 100% to 55.4453%,
with HIST retained: named call comparison confirms that VC6 expands the four
scalar transfers from `saveObject`, while retail retains that member call.
Keep the canonical source calls while recovering the compiler decision.
Mac comparison loses the two previously exact writers, `saveObject` and
`saveMapObjects`, because their emitted bodies retain unpaired `writeScalar`
references. The two list-reader comparisons were already unavailable; their
first unresolved reference is now `readValue<short>`. No native helper
addresses or forced inline qualifiers have been invented to mask these gaps.

### Map lifetime and nested call paths

The map destructor now releases sprites through the existing
`ResourceManager::dispose(CSprite*)` wrapper, as the object loaders already
do. Mac `0x11efb0` is its expanded virtual disposal operation; the adjacent
`0x8100` accessor returns vector storage and is library machinery. `close`
already expresses the checked virtual deletion of map-object data, and the
hero/monster notification loops already broadcast through the corresponding
virtual slots. The two zero-fill calls, invalid-object message dialog and
rectangle intersection are represented by the existing platform operations.

DC's outer read/load disposal calls are represented by Complete's nested
object loaders, which retain old sprite references until replacement sprites
have been acquired. The seer-name call is likewise already nested in
`TSeerHut::read`; adding it to the outer wrapper would assign twice.
Two other DC leads are actual revision differences. Its `readMapObjects`
constructs `type_point(-1,-1,-1)` in a hero-reset loop before the count read;
Windows `0x50449b..0x5044b2` and Mac `0x1272b8..0x1272c8` proceed directly
from invalid-placement clearing to that read. DC `0xf106e` rolls default
experience and adds forty; Windows `0x5023d9..0x5023ea` instead conditionally
stores supplied experience and immediately starts reading the portrait.
The existing source comments identify both differences.

The targeted build keeps the destructor at 100%, has no Windows MAX changes
and loses no further Mac comparison availability. Twenty reviewed sites bring
the working queue to 5,788 open native sites and 2,208 operation notes. All
currently queued mapcell sites have dispositions. This does not certify every
mapcell body or erase the outstanding inlining/comparison debt; the queue is
still based on the earlier global native graph.

### Event-record scalar transfers and boat occupant IDs

The event base, hero-move, mine-claim and boat-hide serializers now retain
fourteen more reference scalar-helper uses. Member transfers still use member
storage; the hero-ID and boat-ID readers keep their local staging. Exact-size
checks remain exact-size checks. Loads stop on a failed base read, while
saves ignore the base result; the move/mine writers return only their final
transfer's Boolean result. Packed source/destination points remain whole-record
transfers, with no invented endian conversion.

The boat occupancy tail requires four additional endian-helper uses. Mac
`0xbfef4`/`0xbff20` uses `lhbrx` then signed widening for the two occupant IDs;
`0xc002c`/`0xc0054` narrows and uses `sthbrx` on output. The previous source
left these as native-order helpers with an unresolved-conversion comment.
They now call `readLittleEndianValue<short>` and
`writeLittleEndianValue<short>`. These transfers remain unchecked, and the
old-save version gate/defaults remain intact.

All eight native bodies were inspected, accounting for 32 more stream sites.
The queue now has 5,756 open sites and 2,240 operation notes. Targeted mapcell
and event_record AST snapshots were refreshed after their edits; the global
native graph and its broader coverage gaps remain unchanged.

The targeted event_record build succeeds. Seven edited functions remain
Windows 100%; boat-hide save is 96.7460%, preserving HIST 100%. Its ordered
six virtual writes and all compared references agree with retail. The
unchanged boat-show writer also has CUR 94.2254%, with MAX/HIST 100% retained.
Six Mac comparisons become unavailable because scalar helpers remain as
unpaired references: base load/save (both previously 100%), move load/save
(18.9024%/20.6140%) and mine-claim load/save (20.3390%/19.6809%). Boat-hide
comparisons were already unavailable. The recovered helpers remain while
these compiler and comparison differences stay open.
