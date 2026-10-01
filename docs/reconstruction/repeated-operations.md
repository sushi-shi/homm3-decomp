# Repeated owner operations

This report accompanies the repeated-operation changes extracted from PR #122.
The base accessor review retains the inventory, visibility changes, complete
combat-window boundaries and [general guidelines](accessor-guidelines.md).
This continuation covers shared operations recovered by reading multiple caller
flows, including the differences that prevent similar-looking code from sharing
one operation. It is not evidence that the whole-codebase review is complete.

## Caller operations and repeated patterns

The semantic review follows definitions and callers across network cleanup,
lobby setup, campaign start/restart, solo play, adventure rendering, hero and
town army UI, combat results, terrain/road/river adapters, placement scoring,
quest loading, skill evaluation and widget state changes. The operations below
come from those complete flows. New project names are labelled at their
definitions; they are not claims of independently recovered native names.

| Pattern and callers read | Owner operation | Distinction that must survive |
| --- | --- | --- |
| `updateCurrentPlayers`, player-drop handlers, `playerData::init` | Existing `clearNetInfo()` | Clears name, DirectPlay ID and both control flags. A control-only change must retain identity. |
| `updatePlayerPositions`, campaign/tutorial start, restart, `advManager::main`, solo cancellation, loss/endgame dialogs, `CTurnDurationPause` | `setLocalHuman()`, `setComputer()`, `copyControlFrom()` | The first two change the paired flags; copying preserves both raw bytes. None restores/discards a connection. Keep original guards and re-read the current player after AI/dialog calls. |
| `showComputerScreen` | Locality-only drawing override | The computer must remain non-human while its local view is drawn. Its native-public locality field and the recorded `isLocalHuman()` snapshot remain explicit. |
| Lobby map changes and seat reassignment | `resetTownAndHero()` | Retains the seat. Native `resetAdvancedOptions()` clears town, seat and hero; `clear()` also discards the connection. Town/hero setters remain for genuinely independent choices. |
| Hero-screen, swap-manager and town-manager stack merges; `armyGroup::merge` | `mergeStack()` delegates source clearing to existing `dismiss()` | Transfer the count, retain the destination type, then clear the source. `add()` also selects a slot and clamps a negative count, so it is not equivalent. |
| Combat results, town initialization and random hero armies | Existing `dismiss()` in the slot-clear loops | Keep recorded per-slot source loops. Custom army loading can retain a nonpositive count while clearing only the type, so that partial operation does not use `dismiss()`. |
| AI duplicate-stack consolidation, splitting, whirlpools, market sales and creature-bank setup | Existing `mergeStack()` and `dismiss()` | Consolidation retains the first slot's type and traversal order. Clearing is reused under the existing exact-zero guards; negative-count setup paths remain partial. Trade resource updates and the bank's column order remain in place. |
| Army help and artifact guardian descriptions | `armyGroup::copyConsolidatedFrom()` | Reset a distinct destination, pack occupied source slots in encounter order and combine duplicate types through `add()`. Keep the source intact and preserve each caller's text format. |
| Terrain, road and river adapter reads; line-painter snapshots | Complete `rmgTerrainTile` construction | Construct terrain/frame/flips together. Keep the existing two-argument constructor and implicit copy boundary; do not add a custom copy constructor that changes padding-copy behavior. |
| Line refresh and terrain transitions | `setFrameAndFlips()` | A frame's orientation changes with it; terrain kind survives. |
| Packed-cell initialization and painter `setTile` | `TRmgPackedTerrainCell::setTile()` through existing field helpers | Copy only the four payload fields, preserve reserved bits, and keep validity changes at their original points before/after the payload. |
| Road/river adapter writes and river overlay changes | `TRmgMapItem::setRoad()`, `setRiver()`, `setRiverType()` | Full writes replace frame/flips. River-kind changes also update presence. River-object marking can set presence without changing kind, so `setHasRiver()` remains separate. |
| Path carving, border clearance/repair, guard and entrance placement, treasure-group commit | `TRmgMapItem::openPath()` / `markBorder()` with `closePath()` / `clearBorder()` for partial edits | Each operation honors the connection-decoration guard. Full transitions write both flags in their original order; a clearance ring only closes paths, while widening a route only clears border marks. |
| Border-object areas, subterranean-gate sides, monolith borders and connection opening | `setConnectionDecoration()` / `clearConnectionDecoration()` through the path/border operations | Installing marks the border before publishing direction/presence; replacing a present marker retains path flags. Clearing removes presence/direction before opening. Keep the packed direction width and reserved bits; map-wide `clear()` instead retains the old direction. |
| Outline connectivity, connection/road floods, shipyard/mine placement, decoration and zone preparation | Existing `TRmgMapItem::isPassableLand()` | Reuse the road-passable/non-rock conjunction without absorbing water, entrances, borders or zone tests. Keep the cached-terrain seed query and the water-repair query's intervening border test explicit. |
| Header construction and map/save player-slot readers | `clearPlayerCounts()` / `countPlayerSlot()` and `TPlayerSlotAttributes::clearHeroCustomization()` | Count mandatory human, possible human and active slots together. Readers keep their different increment points and final minimum-one rule. Customization clearing retains the selected hero ID and name-buffer tail. Native-public fields remain public. |
| Artifact and artifact-transport victory checks | `hero::hasCombinationComponents()` through existing `hasArtifact()` | Require all components on one hero, including backpack contents. Retain ascending, count-terminated mask traversal and its nonempty-mask contract. Assembly's equipped-only query and campaign collection across multiple heroes remain separate. |
| Hero and swap equipment displays | `hero::isArtifactSlotReserved()` with private reservation counters | Assign reservations to empty slots from the end of the slot class, skipping occupied slots. UI still owns placeholder frame `0x91`, dragged-artifact highlighting and widget messages. |
| Equipping/removing an artifact and its combination components | `adjustCombinationBonuses()` and `adjustArtifactPrimarySkills()` through existing `adjustPrimarySkill()` | Share inverse component effects and byte reservation updates, sparing the first component in the assembled artifact's class. Keep actual slot writes, recursive spellbook equipment, assembled bonuses and spell refresh at their original caller stages. |
| Adventure hero click and hover handlers | `TAdventureMapWindow::getHeroLocatorSlot()` | Map portrait, movement, mana and highlight widget bands to their shared row slots. Caller dispatch still owns accepted bands; left-click and right-click retain their different roster bounds. |
| Adventure hero/town list refresh and selection highlighting | `scrollLocatorIntoView()`, selected-locator draws and `drawTownLocatorHighlight()` | Share page clamping and selected-row scans using the caller's cached player. Preserve human-player guards, row redraws and screen-update timing. Single-hero refresh retains its different image/visibility order. |
| Adventure elevation and sleep image changes | `showButtonImage()` | Share image, player-palette, draw and update messages. Read the player after the image message; caches and sleep hotkey replacement remain caller-owned. |
| Terminal destructors across the `heroWindow` hierarchy | `deleteWidgetObjects()` nested beneath existing `deleteWidgets()` | Delete owned widgets in forward order without clearing the vector or unlinking the message stream. Full cleanup still clears only after deletion; resources, globals and other child windows retain their caller-specific teardown order. |
| Hill-fort construction | Existing `addWidgetsToMessageStream()` | Register the complete owned vector with default priority and the existing null-entry error path before initial palette setup. Partial and subwindow registration are separate operations. |
| Bottom-view banners and combat subwindow construction | `TSubWindow::addWidgetsToMessageStream()` / `addHiddenWidgetsToMessageStream()` | Ordinary registration skips nulls; popup construction only guards the flag change and still registers every entry. Local-vector append paths and enemy-turn indexed registration retain their own policies. |
| Replaceable combat/bottom-view strips and combat information popup destruction | `TSubWindow::removeAndDeleteWidgets()` / `deleteWidgetObjects()` | Unlink each strip widget immediately before deleting it, using the existing `removeWidget()` helper. Information popups delete without unlinking. Neither clears the vector. |
| Combat hero/creature popup show and hide | `showWithSavedBackground()` / `hideAndRestoreBackground()` | Save before enabling flags and drawing; disable before restoring. Preserve each derived shown guard and its final flag store, and the hero popup's extra update column. |
| Combat popup construction/drawing and spellbook page assembly | `widget::setActiveAndDrawn()` | Change only the paired flags, retaining other status bits and short storage. This operation has no message-dispatch or drawing side effects; native-public status remains public. |
| Spellbook heading slots and trailing empty page slots | `clearSpellSlot()` | Hide level/icon widgets, empty the caption and invalidate the spell ID together. Retain caption status, icon frames, sorting, availability and page-navigation rules. |
| Combat click, hover, inspection, spell selection and next-action transitions | `combatManager::hideInfoSubWindows()` | Preserve the ordered six `unShow()` calls and fresh combat-window member reads across restoration. Callers keep control/placement/quick-combat guards, cursor changes and later redraws. |
| Combat initialization, control handoff and hover-cell changes | `invalidateCommandForCell()` | Set the cell and invalidate the displayed-command cache with `-99` together. Mouse reset and obstacle diagnostics retain their cell-only updates; command calculation, cursor choice and message publication stay at their original stages. |
| Spellbook keyboard and widget navigation | `turnPage()`, `displayNewContext()` and nested `animatePageTurn()` | Preserve previous/next-page and context setters, animation direction and redraw order. Keyboard arrows retain their active-widget guards, unchanged contexts remain inert, and construction restores saved state without these display transitions. |
| Map and save-header condition readers | `VictoryConditionStruct::resetForType()` / `LossConditionStruct::resetForType()` | Initialize the signed-byte type and clear result/player together; retain unrelated payload. Keep stream reads, failure exits and campaign overrides at their original points. Native constructors retain their initializer lists. |
| Random-map setup and unsupported-map selection display | `CMapHeaderData::disableSpecialConditions()` | Disable loss then victory types only. Copied result flags, player identities and payload survive; this is not the reader's initialization operation. |
| Artifact, creature/resource, town/hero/monster and ownership victory checks; town/time loss checks | `recordWin()` / `recordLoss()` | Record player and result together without displaying, transmitting or changing condition type. Player-independent wins only set the flag; hero loss records type/hero/result before the adventure handler supplies its saved owner. Native-public condition fields remain public. |
| Placement overlap scoring and `clearPlacementMarks()` | `markCandidateOverlap()` | Marks accumulate across cells. A new covers/behind relation must not clear the opposite relation already encountered. |
| `TSeerHut::read`, reward interpretation, savegame loading | `TSeerReward::readFromMap()` | Own the tag and selected union payload together. Preserve sequential reads, signed widths, untouched payload bits and unknown tags. The hut consumes its reserved bytes; savegame loading remains a whole-record transfer. |
| Campaign secondary-skill bonus and AI `getSchoolValue` | Mastery-only setter for replacement/simulation; existing `giveSS()` for acquisition | These raw-byte writes do not acquire/remove skill-order entries or normalize stored values. `setSS()` has a zero-level removal path, so it is not a general substitute. Preserve the separate `giveSS()` branch for an absent skill, including its zero-level behavior. |
| All five marketplace panes, their completion handlers and tab switches | Shared initialization, completion and selection-reset helpers in `tradpost.cpp` | Pane entry resets quantity and backpack position. Completion and tab switching retain them and set different denomination states. None of these helpers performs the transaction or sends network/UI messages. |
| Army construction/movement, AI selection, berserk attacks and combat commands | `setAttackTarget()`, `clearAttackTarget()`, `setWallAttackTarget()` | The side/index pair identifies the target, not the moving stack. A catapult stores a wall hex in the index lane. The post-attack side-only invalidation remains partial and retains the index. |
| Combat-stack stats, army-group queries and morale/luck descriptions | Shared terrain morale and clover-field luck rules | Preserve copied combat traits versus game alignment queries, cursed-ground/artifact guards and creature-specific limits. Descriptions subtract the same contribution and retain the terrain-specific text. No `inline` keyword or native identity is inferred from a goto alone. |
| Morale value and description alignment scans | `getMoraleAlignmentCount()` through existing `getAlignments()` | Neutral creatures count in the census but are not faction-grouped. The grouping flag and original bitset queries remain part of the operation. |
| Skeleton-transformer construction, `unselect()` and `creatureClick()` | State-only `clearCreatureSelection()` | The click path hides its border, updates both old/new slots and clears hover before invalidating the indices. Moving `unselect()` earlier would invalidate indices still needed by those updates. |
| Swap-manager construction and `reset()` | State-only `clearArmySelection()` | Construction has no usable window yet. Split-mode reset and widget messages belong only to the existing UI reset. |
| Hero placement, prison rescue, campaign placement, garrisoning, town swaps and dismissal | `playerData::addHero()` / `removeHeroAt()` | Maintain the ordered roster, count and empty tail together. Callers retain capacity/presence guards, current-hero invalidation, map visibility and network order. A missing roster entry does not suppress dismissal's separate current-hero cleanup. |
| Hero initialization, day rollover and boat boarding | `resetAdventureSpells()` through `clearMovementSpells()` and existing `walkOnWater()` | Boarding clears only flight and water walking. The complete reset also invalidates disguise/Visions and zeroes the Dimension Door count. `fly(-1)` would charge mana and is not a reset. Initialization and dismissal also reuse existing `clearTarget()`. |
| Hero movement, spell restrictions, replay, AI search, rendering and combat terrain | `isOnBoat()` / `setOnBoat()` | Normalize the boat bit to a predicate and preserve other flags on mutation. Dreamcast explicitly declares `hero::flags` public, so its visibility stays public. Replay/undo and remote landing only change state; they do not charge movement or clear spells. |
| Map rollover, quick information and native map-help builders | `getMapVisitText()`, `appendMapVisitStatus()` and `appendMapHelpText()` | Share label selection and format-then-append while retaining caller-owned scratch buffers, trigger/hero guards, knowledge queries, predicate calculations and separators. Shrine/witch names and lighthouse ownership use the same formatting operation. |
| Defense Tower, Garden of Revelation, Mercenary Camp and Power School | `doEventPrimarySkillSite()` through hero visit methods and existing `adjustPrimarySkill()` / `setInfoFlag()` | Share the complete visited/reward branch. Dialog comes before skill mutation, team information before visit marking; reread the cell ID at the final store. Native named event handlers remain the entry points. |
| AI valuation, rollover and quick information for those four sites | `hero::visitedPrimarySkillSite()` | Share the skill-to-visit-mask mapping. Preserve raw event/AI IDs, the rollover's five-bit extraction and quick-info's existing `getItemId()` call. Native masks remain public. |
| Hero morale/luck calculations and their descriptions | `playerData::hasGrailTown()` | Scan the cached player's roster, test included Grail buildings before faction and stop at the first match. Each caller retains its negative-owner guard, bonus amount and description formatting. |
| Generator construction and map initialization | `clearCreatureSlots()` | Clear all four creature/count pairs in slot order, retaining separate guard-army, owner and map-position setup. Save loading retains serialized order and sentinel decoding. |
| Generator bonus addition/removal during ownership and town changes | `adjustTownBonuses()` through existing `updateBonus()` / `removeBonus()` | Preserve the negative-owner and unknown-alignment guards, cached player row and each town's creature read. Apply the signed change through existing `town::changeGeneratorBonus()`; owner mutation remains between removal and addition. |
| Hero initialization, campaign carryover, recruitment and dismissal | `resetManaToMaximum()` | Unconditionally store the current maximum with native short narrowing. Preserve army/equipment setup and dismissal reservation guards around the reset. |
| Daily Wizard's Well, Mysticism and mage-guild recovery | `raiseManaTo()` | Compare the full-width threshold against stored mana before narrowing. Keep the daily maximum snapshot before the artifact query and preserve existing surplus mana. |
| Turn handoff, remote dead-player handoff, campaign setup, prison release and movement cheat | `refreshMovement()` | Calculate mobility once, store remaining points and then allowance. Garrison waking and each roster's bounds remain with its caller. |
| Fountain, oasis, rally flag, watering hole and stables rewards | `addMovementBonus()` and `grantStablesMovement()` | Add the reward to allowance then remaining points. Stables shares its visit-bit guard and mutation before the nested bonus, returning whether the reward was granted. Dialogs, map-info flags, morale/luck and creature upgrades retain their original stages. |
| Boarding and landing event handlers | `applyBoatMovementCost()` | Unlimited movement bypasses both the charge and locator refresh. Admiral's Hat preserves the remaining fraction with the original signed multiply/divide order; otherwise movement becomes zero. Boarding still clears movement spells before charging; landing does not. |
| `hero::isMobile()` and the cursor's next-step decision | `getMinimumTerrainCost()` delegates to existing `minimumTerrainCost()` | Boat travel suppresses both movement spell levels. Land travel uses stored levels, pathfinding and the army's Nomad query; the separate step-cost calculator retains its artifact overrides and native-terrain rule. |
| Combat-cell initialization, army placement, grid removal, death and vanishing | `hexcell::setArmy()`, `clearArmy()`, `resetArmy()` | Identity-only clearing retains the double-wide marker; full removal clears it. Turning can update only the marker. Preserve signed-byte narrowing and the owning-side/offset helper calls. |
| Corpse recording for both halves of an army | `hexcell::recordArmyBody()` through existing `hasArmy()` | Check capacity for both cells before recording either. Append side, slot and double-wide marker together, then increment the count. Empty-cell guards remain in the helper; the no-body path stays separate. |
| Game construction and initial scenario setup | `resetHolyGrail()` | Restore the unavailable coordinates, search radius and presence flag together. Successful digging only clears presence and retains the puzzle location. Map/save loading still preserves its sequential reads. |
| Adventure-manager construction and wandering-monster encounters | `clearMovingObject()` | End the drawing override by invalidating its index/sequence; retain the frame and sprite. The encounter keeps its draw, screen update and result handling before the reset. |
| Puzzle object and object-shadow rendering | `isPuzzleMapObject()` | Share the accepted decoration types after each caller's mask/underlay guard. Keep roads, rivers, holes and all unlisted types excluded. |
| Adventure object, shadow, river, road, arrow, arrow-shadow, shroud, underlay and ground drawing; View World icon drawing | `clipAdventureTile()` | Adjust source offsets, extents and clipped destination copies together. Road bottom trimming follows clipping; View World retains the saved unclipped draw origin. |
| Town-building right-click branches, ordinary descriptions and hall/castle icons | Existing `showBuildingInfo()` | Own the description copy through the modal call and retain dialog mode/icon arguments. The shared popup goto is replaced by the helper. The silo's left-click `sprintf` treats the description as a format string and remains distinct. |
| Town changes, unloading and the three hero/garrison move paths | `deleteStrips()` / `rebuildStrips()` through existing `newStrips()` | Preserve hero-delete/null then garrison-delete/null order. Setup still updates locators/music between deletion and creation. `resetStrips()` clears selections and redraws; tavern hiring replaces only the visiting strip. |
| Combat-window close and end of placement | `clearControlSubWindow()` | Delete/null the live child before closing or replacing it. The destructor's terminal deletion remains separate. Widget destruction also reuses existing `clearHoverWidget()` under its original identity guard. |
| Lobby transfer start, completion and player drop | `CNewPlayerUpdateMan::startProc()` / `deleteProc()` | Check for a free slot before allocating; install the new job before virtual `go()`. Delete before publishing an empty slot. Keep the tick/finished checks, player identity guards and terminal destructor loop. |
| Adventure ambience initialization, environment scans and slot replacement | `soundNode::reset()` and `advManager::stopLoopingSound()` | Playback, cached-resource lifetime and slot bookkeeping are separate. Full reset clears identity/priority; an ordinary scan invalidates priority alone, and distance expiry clears identity alone. Stop before resetting/replacing the record; retain the touched-mask OR/XOR behavior. |
| Combat UI orders, siege automation, melee/berserk AI, spell selection, network reception and simulation restore | `setTargetAction()` / `setPendingAction()` with private pending-action storage | Three-field orders retain the secondary spell target. Complete copies/reset own all four slots. Movement computes the double-wide anchor first. AI/berserk `queueShot()` retains extra/secondary payload, unlike the UI shot's explicit `-1` extra. |
| Spell selection and human/AI surrender | `prepareAction()` nested beneath the targeted and complete action setters | Set opcode and payload while retaining both hexes. Only the original two spell families select the creature-spell opcode, and only for `creatureSpell == 1`; other families still queue hero casting. Surrender keeps affordability and confirmation guards. |
| Spell-target callbacks and teleport valuation | Pending-action queries, `selectActionTarget()`, `selectSecondaryActionTarget()`, `cancelPendingAction()` | The two-click spells genuinely select targets separately. Cancellation clears only the opcode; skipping an army instead queues `AI_ORDER_NONE`. Keep picker-local caches, highlighter cleanup and outgoing-message order with each callback. |
| Combat/system option construction and updates; native combat-speed highlighting | `heroWindow::setExclusiveWidgetStatus()` and `showVolumeLevel()` | Clear the complete ID row before selecting one widget; volume additionally sets its frame. Preserve direct widget message dispatch and each caller's DRAWN/DIMMED/DIMMED_NODRAW behavior. Audio adjustment, sound-enable overrides, changed-preference flags and writes remain outside the display helpers. |
| Spell target pickers | `message::setDialogEnd()` | Set widget/end together while retaining codeY and the rest of the input payload. Per-handler target caches, cancellation and highlight clearing retain their separate order; wall targeting does not acquire a new reset. |
| Campaign selection, custom campaign buttons, window exits and marketplace tab switches | `setDialogEnd(result)` and `setDialogEndCodes(result)` | The complete tuple supplies a message result; the codes-only form retains the original event ID. Preserve the View World keyboard path's ID and the marketplace redraw before its close message. Button deselection still shares its ID/result writes before choosing end versus ordinary deselect. |
| Popup, options, puzzle, campaign, lobby, hero/army/spellbook, level-up and sacrifice exits | `heroWindowManager::finishDialog()` | Save the manager result separately from the conventional END_DIALOG message payload. Keep deadline clearing, held-artifact return, network cleanup and other caller work at their original points. Native-public message fields and manager `dialogReturn` remain public. |

For the network example, the whole sequence is the operation:

```cpp
// Name, connection and control all reset through the existing owner method.
g_game->m_players[i].clearNetInfo();
```

Lobby setup and solo mode instead use `setComputer()` or `setLocalHuman()`.
Replacing only `m_isHuman` with `setHuman()` leaves callers responsible for a
multi-field transition, so that single-field setter and its raw-byte getter
were removed. The reward loader's tag/payload setters and unused terrain
mutation wrappers were likewise removed in favor of the operations above.

The review also distinguishes established APIs that already cover their
callers. Game-owned world-map and indexed hero, town, mine, garrison and boat
access use their existing getters. Count, append and clear operations live
with those collections, with loader destruction order preserved. Hero route
setters own the coordinate group; full-width queries preserve sentinels and
`clearTarget()` invalidates X/Y while retaining Z. Palette-copy setters retain
delete-before-copy ownership and the resource copy constructors.

Army operations also depend on widths and thresholds. The skeleton transformer's
swap preserves full-width counts, whereas native `armyGroup::swap()` saves one
count through a `short`; these cannot share that implementation. Its merge arm
uses `add()` before dismissal and retains that helper's negative-count clamp.
AI swapping compares a narrowed source count before choosing dismissal or a
subtraction. Sacrifice clears nonpositive results, ordinary sales clear only
zero, and damage can force dismissal from the casualty rate. Keep these
conditions and conversions rather than inventing a universal removal rule.

Army description copies share a different complete operation: initialize a
distinct destination, visit the source slots in order, skip only the empty-type
sentinel and add each occupied stack through the existing `add()` method. This
packs holes and combines duplicate types without changing the source. Zero and
negative source counts still participate; duplicate additions retain `add()`'s
negative destination-count normalization. Both callers keep their temporary
group lifetime and their different description, separator and prompt rules.
The method name is project-inferred, not a native declaration claim.

In-place AI consolidation preserves holes and sums directly, so it retains
`mergeStack()`. Campaign carryover filters creature types before adding. Live
combat and transformer moves dismiss their source after adding, whereas reward
delivery dismisses only after successful addition and snapshots values before
dialogs. AI extra-creature evaluation dismisses before recomputing alignments
and may restore the slot afterward. Transactional `merge()` uses copies and
commits only after fitting all stacks; `mergeArmies()` instead selects gains
and can exchange full-width stacks. None is a consolidated display copy.

Widget and slider methods illustrate why names alone are insufficient.
`widget::sendMessage` can dispatch drawing/update behavior. A raw status-bit
edit is not automatically a message send. `slider::setResolution` resets
current and old state; `updateResolution` retains/clamps the current state
through `setState`. Recruit callers already use these distinct operations.
Likewise, legacy map-cell conversion records are serialization layouts, not
live `ExtraInfoUnion` objects: rewriting their payload copies with gameplay
setters could reset visited bits or other preserved fields.

Option rows use direct `getWidget()->sendMessage()` calls. Window-wide status
broadcasts are not equivalent when IDs repeat, and raw status writes bypass
message behavior. Their clear/select order is shared, but constructor dimming
suppresses drawing while interactive system-option dimming redraws. The volume
icons only update status/frame during this operation; sound adjustment follows
it. Keep the native combat-speed helper as an outer call with the shared row
operation inside. Other menus clear nullable widgets, only the previous hover,
a conditional subset, or a pair in descending ID order; these are not the same
unconditional ascending-row operation.

Deletion patterns also need their surrounding ownership rules. Overview
rebuilds unregister live widgets and null their slots; final teardown deletes
the widgets before destroying the window and pointer arrays. Its native
destructor deliberately does not use `deleteWidgets()`, which additionally
clears the vector. Terminal `heroWindow` destructors now share
`deleteWidgetObjects()`; the existing full cleanup nests it before clearing.
The helper preserves forward traversal and leaves entries untouched. It is
not repeatable cleanup: the remaining pointers refer to deleted objects until
the vector itself is destroyed or cleared. It adds no unlinking, null stores,
or base-destructor cleanup that could delete derived-owned widgets twice.

The surrounding teardown still matters. Game-type selection clears its global
window pointer after widget deletion, while several other dialogs clear theirs
before it. Campaign previews close Bink state first. Hero-screen teardown returns
the dragged artifact before deleting widgets. Single-selection teardown retains
its later save-header restoration, and combat keeps control-window, message and
status-subwindow deletion in their original order. The helper operates only on
the inherited window vector; parent-linked subwindows, progress-dialog vectors
and normal-dialog local vectors have their own owners and registration lifetimes.

`TSubWindow` now owns its two distinct destruction operations. Replaceable
combat-control and bottom-view strips remove each widget from the parent and
then delete that same widget, before advancing to the next. Combat information
popups only delete their objects. These operations do not move cleanup into
the base destructor, which still owns only its background image. Resource
displays own separate pointer arrays and retain their interleaved destruction.

Registration has three different null policies. Ordinary subwindow banners
skip null entries. Hidden combat popups guard their flag mutation but still
call `addWidget()` unconditionally. The ordinary `heroWindow` helper reports
a null entry with `memError()`. The control bar appends every local-vector entry
before guarded registration; the placement bar guards both append and
registration. Do not collapse these into one permissive bulk-add operation.

Popup display stages share their background and widget-flag work while leaving
the derived shown-state guard and final assignment in place. Showing a hero
popup updates width plus one; showing a creature popup updates width exactly.
Both restoration paths retain the existing width-plus-one update. Bulk combat
dismissal still calls the two hero popups before the four creature popups and
resolves the manager's window member at each step. Individual hover and hero
inspection paths remain individual operations.

The paired active/drawn flag mutation is deliberately separate from widget
messages: `show()` can draw a dimmed widget and consume a pending update, while
the page/popup assembly paths only change state. The spellbook's shared empty
slot transition uses that state-only operation on its two icons, then empties
the caption and assigns `-1` to the spell map. It does not clear caption status,
cached frames, other status bits or page state. Widget status has positive
native-public evidence, so its declaration remains public.

Spellbook navigation also has a shared display transition around the existing
page/context operations. It plays the configured animation before changing
state and redraws afterward. Keyboard arrows require an active arrow; selected
arrow widgets request a turn directly. Switching context keeps the current
school, resets to page zero and updates remembered context through the existing
setter. School switching still updates its tab frame after resetting the page.
Constructor restoration keeps its saved school/page decisions and does not
animate or draw the window through these new operations.

Combat's `-99` command value is an invalidation marker paired with the selected
cell, not a request to clear all mouse state. Full invalidation is shared by
nonvisual initialization, control handoff, leaving the combat area and ordinary
hover-cell changes. `resetMouse()` only clears the cell before forcing a mouse
event; obstacle diagnostics only record the cell before returning. Both partial
updates retain the displayed-command cache. The next ordinary hover computes
the command and publishes its message at the original points. These fields
have positive native-public declarations and remain public.

`TSubWindow::restoreBackground()` restores and updates the
screen before deleting/nulling its saved bitmap; terminal destruction only
deletes it. These paths do not support one unconditional cleanup operation.

Bitmap reset methods already own their distinct contracts. `Bitmap16Bit::clear()`
retains pitch, deletes only an owned map, and clears the reference flag only
when a map exists. `Bitmap816::clear()` resets pitch and always owns its map;
`Bitmap24Bit::clear()` has its own RGB size fields. Their import/reference paths
already call the canonical reset. Similar zero stores do not justify merging
these methods or substituting a full reset for a destructor's buffer release.

Dialog completion shows why the consumer matters. `doDialog()` and
`doDialogDraw()` copy codeY to the manager result only after a widget broadcast;
a callback can end the loop without that copy. Spell pickers therefore retain
codeY, campaign buttons return their chosen ID in codeY, and many other callbacks
save the result on the manager before writing END_DIALOG into the message.
The codes-only operation deliberately retains the incoming message ID, including
View World's keyboard branch. Recruitment uses the executive command protocol
and keeps its existing `exitRecruitUnit()` / `finishRecruitUnit()` helpers.
No shared dialog helper clears a timeout, dispatches the message, closes a window
or assumes ownership of the caller's cleanup.

Random-map path flags describe several operations. Opening a path clears the
border flag and sets the gate flag; marking a border reverses them. Both leave
connection-decorated cells alone. Marker installation first invokes the guarded
border operation, then writes direction and presence, so replacing an existing
marker does not reinitialize its flags. Removal clears presence and direction
before opening the cell; the later call after virtual object placement still
rechecks protection. All of this state belongs to the map item.

Nearby partial writes have different contracts. The clearance ring closes paths
without adding border marks; widening a route clears border marks without opening
all neighboring cells. Treasure-group commit captures the destination flags
before modifying either map and restores them to the source in border-then-gate
order; if both saved flags are set, the gate wins. Keep its independent protection
checks. The full map-item reset uses packed snapshots and preserves the old
connection direction, so it must not call the live marker-removal operation.

The AI choice review also found a meaningful distinction: tactical choices store
combat hexes, while simulated army choices store creature-vector indices and use
the second target as a Dispel fallback. Their producers update score, target and
cast timing at different stages. Native declarations positively expose several
of those fields as public. A repeated `-1` is not enough to impose one complete
choice setter or make the record private. Likewise, puzzle-tile constructors
initialize different visibility/terrain data and the empty constructor leaves
the grail flag untouched; they do not share an unconditional full reset.

Header player counts are also a grouped operation. A human-only slot increments
the minimum, any human-capable slot increments the maximum, and either control
flag contributes to the total. The map reader counts after reading the whole
slot; the save reader counts immediately after reading its control/strategy
fields, before later reads can fail. Keep those different call points and the
post-loop minimum-one rule. Random-map options supply counts directly and do
not use the census. Clearing a fixed hero's customization invalidates its portrait
and terminates its name, retaining the hero ID and the rest of the name buffer.
Both readers and construction share that operation; populated names still use
their existing format-specific readers and copy lifetimes.

Condition results distinguish disabling, initialization and completion. The
selection window's repeated `-1` pair disables two special condition types and
retains everything else. Map/save readers instead initialize each type and its
result independently after the corresponding successful byte read. Campaign
overrides between those reads alter the victory payload without resetting it.
The reader methods preserve that timing and the signed-byte conversion.

Victory and defeat checks share complete player/result updates. The upgraded-town
check used the reverse order for those two adjacent scalar stores; its call now
uses the same player-then-result operation as the other complete wins. There was
no intervening query or callback. Defeating all monsters and surviving a time
limit remain flag-only wins: their display cases do not ask for a winner.
Hero death remains staged because the adventure handler saves ownership before
deallocation and assigns that saved owner after `heroKilled()`. Network messages
copy the complete record and do not reinitialize it. Positive native-public
Type, GameWon/GameLost and playerWinner/playerLoser declarations are preserved.

Artifact component masks support different questions. The shared victory query
asks whether one hero owns every component and includes the backpack. The
existing assembly predicate only sees equipped artifacts and clears a copied
missing-components mask. The campaign's piece collection can span multiple
heroes, while its loss rule asks whether a hero owns any listed piece. Keep
those rules distinct. The victory query retains the original count-terminated
walk; it does not turn an empty combination mask into an automatic win. Artifact
transport also retains its different winner selection for a complete artifact
versus a collection of components.

Equipment reservation is a hero-owned rule: scan the slot class backward,
counting only empty slots, until the reservation count is exhausted or the
requested empty slot is reached. Both equipment displays use that query and
choose the placeholder frame themselves. The placement checker instead tests
class capacity and simulates reservations for a candidate combination; it is
not a substitute for the display query.

Combination equip/remove operations traverse components in ascending order,
apply each component's four primary-skill bonuses through the existing method,
accumulate the spell-refresh requirement and adjust extra slot reservations.
The first component sharing the assembled artifact's slot class consumes no
extra reservation. Signed skill storage and unsigned-byte counters retain
their existing narrowing behavior. Removing an artifact still clears its real
slot between component effects and its own bonuses; equipping still handles
the recursive spellbook before those effects. The shared operation returns the
refresh requirement rather than rebuilding spells midway through mutation.

Adventure widget IDs encode repeated row positions in four separate bands.
The mapping helper only decodes those positions; it does not select a hero or
change the caller's guard. Left-click reads the roster entry before comparing
the row slot against the hero count. Right-click first resolves the widget,
then compares the scrolled index against the count. Hover also accepts movement
and mana widgets. Those differences survive the common mapping. An older
decoded comment says locator overlays keep default hover text, while the active
authored handler routes them to hero rollover. This pass preserves the active
dispatch; it does not claim to resolve that pre-existing evidence discrepancy.

Locator scrolling leaves negative or already-visible selections alone and
otherwise clamps the proposed top to the last full page and then zero. Shared
selection draws use the caller's existing player reference across drawing calls.
Town highlights set the frame before their row redraw. Hero highlights set
visibility, image and then draw; the singular hero refresh retains its separate
image-before-visibility sequence. Elevation and sleep buttons share their four
widget messages, with local-player lookup after image replacement. Each keeps
its cache update before dispatch, and sleep still replaces hotkeys afterward.

Hero resource review distinguishes refresh, reward and partial recalculation.
A complete movement refresh derives both stored values from one `getMobility()`
call. Closing the hero screen recalculates only the allowance; boarding/landing
rescales remaining points or spends them; initialization first zeroes both.
Those operations are not refreshes. Stables grants its bonus only once per
visit-bit lifetime, and both town and map-object callers now use that shared
guard, bit update and paired increment. Other movement rewards retain their
own visit masks and the ordering of morale, luck and global information updates.

Mana reset and raise-only recovery are separate contracts. The daily maximum
is still computed before the Wizard's Well artifact query, while Mysticism
retains its additive calculation and cap. The comparison happens before the
native short store. Magic springs, wells and the town vortex calculate and test
a cap before displaying a dialog, then assign the saved cap afterward; folding
those paths into a later conditional raise would add a second test across the
dialog. Their staged updates remain intact. The black-box reward also keeps
its separate 0..999 adjustment and reward-message calculation. Native declarations
explicitly make `mana`, `maxMobility` and `currMobility` public; the new owner
operations preserve that evidence and do not alter storage layout.

Town-roster queries also need their complete contract. Morale and luck award
one Grail bonus, even if several matching towns exist, and their descriptions
use the same predicate. `buildingsOwned()` is not that predicate: it counts
built-only masks and has different faction and mage-level conditions. Necromancy
instead accumulates bonuses from every qualifying town, so it keeps its scan.
The Grail victory check filters by location and team and credits the winning
town's owner; it cannot substitute the faction-only query.

Generator reset pairs each creature sentinel with its zero population before
moving to the next slot. The constructor and map initializer share that operation,
while the loader retains separate creature decoding and population reads.
Bonus removal and addition retain their native named entry points and delegate
to one inverse operation. The owning-player reference and alignment are resolved
before traversal; each town update still reads the generator's first creature
and calls the existing town helper. Initialization grows guards/population before
assigning ownership; ownership changes remove old bonuses before the owner store.

Visit-flag review found similar bodies with different sequencing: Idol changes
luck/morale before flags, Buoy changes flags and morale before its information
update and dialog, and Faerie Ring changes flags after its dialog but before the
information update and luck. Fountain also chooses its flag from the signed
reward, while post-battle cleanup clears a specific set of temporary flags.
Those flows do not justify a universal combined visit-and-reward transition.
The four permanent single-skill sites do share a complete operation: test the
appropriate hero visit mask, display the visited or reward dialog for humans,
award one skill point, update team information and finally mark the site. Their
native handlers now delegate to that operation. The text/info descriptor is
explicitly project-inferred, and the corresponding hero query also serves AI
valuation and both map-help paths. Each path retains its existing site-ID decoding;
the event rereads the cell after the dialog rather than caching its visit bit.
Native declarations make all four masks public. Loading, saving and initialization
retain their individual field order. Arena, Library, Training Grounds and paid
Magic/War Schools keep their different choices, rewards, payment and marking stages.

Map help shares presentation only after the caller has determined the state.
The visit formatter accepts the full integer predicate, so high flag bits are
not truncated. Rollover and quick-info keep their existing 500-byte temporary
arrays and predicate locals. Their space, one-newline and two-newline formats
remain distinct, including Lean-To's space-only quick-info suffix. Knowledge
labels still precede hero visit labels. Fountain retains the independent
`getInfoFlag()` query and `playerKnowsCell()` guard; spring and garden retain
their visit-bit/depletion conjunctions. Native Pyramid, Wagon, Tomb, Water Wheel,
Windmill, Tree and creature-bank helpers nest the same label selector while
preserving their own visibility, depletion and guard-army decisions.

The formatted append operation also serves lighthouse ownership, shrine spell
names and witch-hut skill names. Each caller retains its scratch-buffer size;
shrine and witch-hut builders also retain their separate separator append and
later hero-knowledge test. Rollover lighthouse
still rereads the mine owner; quick info retains its cached owner byte. These
helpers do not own the cell adjuster's lifetime, rollover drawing, quick-info
position clamping or final dialog.

Puzzle object and shadow rendering share one accepted-type predicate. Its
default rejects every unlisted type, including terrain holes, roads and rivers.
Each caller still checks its own draw/shadow mask and underlay flag first.
Hero/boat suppression remains in `scanForHeroOrBoat()`, and `completeDraw()`
still controls the separate river, road, underlay, route and shroud passes.
Random-map decoration includes six additional numeric types, radar shading
uses a smaller terrain set and nearby-scenery counting distinguishes three
categories; none is the puzzle predicate.

Ten drawing paths share the same rectangle clipping rule against horizontal
coordinates 8..600 and vertical coordinates 0..544, with exclusive upper
bounds. All begin with zero source offsets and 32-by-32 extents. Source offsets
advance as the destination is clipped, and empty-area handling remains with
the caller. Roads add 16 to their initial destination Y and subtract 16 from
height on the final map row after clipping. Adventure layers draw at the clipped
origin plus the eight-pixel screen Y border. View World icons instead retain
their separate, unclipped `drawX`/`drawY` destination and apply clipping only to
the source rectangle. The scaled-buffer helpers retain their 8..552 screen Y
bounds, scale-table sampling and transparent-pixel behavior. The shared clipping
name and ordinary source placement are project inferences.

Map-object placement review also distinguishes resets that share some stores.
Cell recalculation invalidates the selected object, derives blocking from rock,
then considers triggers, flagged objects, other blockers and holes in that
priority order. It can retain the previous subtype/payload and animation bit;
it is not a fresh-cell constructor. Event placement writes a non-trigger event,
one-shot event consumption clears its payload, and hero/boat restoration puts
back a saved type/trigger/payload overlay with town bookkeeping. Object erasure
removes the first matching entry from each covered cell before recalculation,
retaining the world object for recorded undo. Those paths do not support one
generic cell reset or object-removal operation.

Repeated sentinels and control-flow joins are leads, not sufficient proof of a
helper. Read the successful path as well as the reset path, and find what each
sentinel disables. The terrain switches above describe one reusable rule;
their goto/one-iteration-loop spelling need not be duplicated in every caller.
Other joins remain within their existing operation: marketplace modal cleanup
has separately evidenced deletes, `hasSeparatedNeighbours()` already owns its
ring scan, and combat message dispatch has a live `CMessageKill` whose lifetime
must survive any future extraction. Do not flatten those lifetimes to remove
a label.

The original visibility inventory covers 161 backing members (144 private,
17 protected). The complete terrain value interface additionally makes
`rmgTerrainTile::m_terrain` private in place. Native-public exceptions retain
their declarations. The later pending-action review additionally makes the
four `combatManager` action slots private in place and migrates their external
AI/army/spell-callback users. Their original access level is unknown; this is
the project's inferred owner boundary. The random-map connection review also
makes `TRmgMapItem::m_connection` private and moves its external reads and
transitions behind the cell interface. Its original visibility is unknown.
The artifact review similarly makes `hero::m_artifactSlotCounts` private in
place after replacing the two external reservation scans with the owner query;
its original declaration is unresolved. The complete combat-window visibility
review remains in the base accessor PR. Counts describe the reviewed inventory;
they do not prove that every caller's operation was understood, and adding more
methods is not a completion criterion.

## Combat damage display and vanishing creatures

`army::resetDamageDisplay()` now owns the same four-field transition in
`combatManager::powEffect()` and the common completion path of `castSpell()`:
clear the damage latch, restore draw priority 4, clear attack-frame display and
restore the troop-count override to -1. PowEffect still clears its pow overlay
immediately before this operation. CastSpell does not acquire that extra write.
Neither path changes the death latch or current animation frame through this
helper. `initClean()` and `initialize()` keep their staged initialization, and
walk completion retains its draw-priority-only reset.

The same review follows the override from `damage()`, which snapshots the old
count before casualties, to both displays. Battlefield drawing and the creature
popup now call `getDisplayedTroopCount()`. Only -1 falls back to the live count;
other saved values pass through unchanged. Each caller keeps its formatting,
drawing guards and original local/parameter destination. Native army type
`0x1a95`, field list `0x205b`, explicitly makes the four display fields public.
These operations therefore do not change their access declarations.

Four flows—PowEffect, ResetRound, Armageddon and ShowMassSpell—now share
`clearVanishingCreatures()` and `makeCreaturesVanishIfNeeded()`. The first clears
all forty marker bytes before clearing the pending flag. The second preserves
the pending guard around the existing native `makeCreaturesVanish()` operation.
Processing does not clear the markers or flag afterward. Death processing marks
the owner-side slot and sets the pending flag through `markCreatureForVanish()`.
The array remains 2x20 despite the 2x21 army storage.

The caller-specific death walks remain separate: PowEffect tests the all-killed
latch, while the spell paths test affected stacks with exactly zero troops.
Round processing retains its creature-type guard and per-stack reset, which can
itself run a poison pow effect. Death redraws, siege-artifact removal, rebirth and
quick-combat gates retain their original positions. MakeCreaturesVanish still
clears occupancy in quick combat while gating its drawing/fizzle work.

Direct native records identify the marker array as `bCreatureVanish` and the
pending byte as `bSomeCreaturesVanish`, both public in combatManager `0x1ed7`,
field list `0x4429`. The earlier unresolved semantic alias is now documented in
the owning header. The new helper names and ordinary source-file placement are
project inferences; they carry no invented native address or inline claim.

## Recruitment selection and quantity changes

The four creature-card branches in `recruitUnit::main()` now use
`handleCreatureClick()`. Selecting a different card with the left button stores
the selected position and creature, resets the purchase quantity and controls,
then invokes the existing `update(1, slot)`. Clicking the selected card or using
the right button opens the same scoped creature preview. That preview is still
destroyed before Main's common redraw; recruitment retains its fixed position,
whereas the hill-fort and sacrifice/transformer callers center their previews.

`setPurchaseQuantity()` shares the quantity/slider/accept-button transition
between card selection, maximum quantity and a completed purchase. Its nested
`updatePurchaseQuantityControls()` is also used after typed input's existing
lower-then-upper clamp. The window and quantity are reread after the virtual
slider setter before enabling the accept button. The helpers do not calculate
prices, change slider resolution, redraw or finish the dialog.

The slider callback already receives the slider's new state and deliberately
does not write it back. Cancellation clears the quantity and disables acceptance
without touching the slider. Opening initializes quantity and totals separately;
`update()` retains its affordability clamp, remote/view-only button gates and
new-creature-only resolution change. Successful purchase still delivers the
army/artifact first, charges gold and the optional resource, narrows the stock
deduction to short, and only then resets the purchase selection. Capacity
failures and single-creature/town completion retain their existing exits.

Native recruitUnit `0x3f7f`, field list `0x5223`, explicitly records public
`monsterType`, `selectedPosition` and `numberToBuy`; no visibility is inferred
from these new private helper methods. Names and ordinary source placement are
project inferences without native address or explicit inline claims.

The surrounding resource review distinguishes paired/partial recruitment costs,
market exchange/gift ordering and dense seven-resource upgrade costs. The
blacksmith's seven-entry loop subtracts every entry from gold, unlike a normal
row payment; the sparse town-build dialog also preserves its selected resource
indices. Those operations are not changed by the recruitment helper extraction.

## Complete resource-cost payments

Nine quest, building, upgrade and war-machine purchase paths now share
`playerData::payResourceCost()`: town building purchase, AI dwelling purchase,
army-view upgrading, hill-fort slot upgrading, AI town creature upgrading,
AI hill-fort upgrading, war-factory visits, siege-engine buying and the resource
quest's native payment wrapper. The operation
subtracts one complete seven-resource row in ascending order. It does not test
affordability, skip zero entries, clamp negative costs or update any UI.

The ordinary `int*` and `long*` cost interfaces retain the existing callers'
types. Both use one source-local implementation template; no array is copied,
reinterpreted or widened in advance. Each cost entry is read immediately before
its matching resource debit. This preserves the int difference row used by AI
creature upgrades as well as long rows produced by `getUpgradeCost()`.

Ordering remains specific to the caller. `town::buyBuilding()` pays before
building; AI dwelling purchase builds first and then pays through its already
cached player pointer. The hill-fort UI changes the creature type before paying,
while army-view and AI upgrade paths pay first. Siege-engine buying retains its
second affordability check after constructing a missing building. War-factory
valuation and artifact delivery retain their positions around payment.

Native playerData `0x1c50`, field list `0x35de`, explicitly records public
`resources`; that storage remains public. The payment interfaces and shared
implementation template are project inferences, without native helper claims.
Sparse town-dialog payments, the blacksmith's all-entries-from-gold behavior,
recruitment's gold/optional-resource pair, gifts, exchanges and scaled purchase
budgets remain distinct operations. Existing affordability loops and the native
hill-fort `canAfford()` helper retain their own contracts.

## Boat prices and centered creature previews

The boat-price review connects four affordability checks, three successful
purchase paths and two AI cost estimates. `playerData::canAffordBoat()` owns the
1,000-gold/10-wood predicate used by map shipyards, the town ship dialog, AI boat
building and AI shipyard marking. These are pure resource reads; the shared
query uses the gold-first ordering of the UI paths. `payBoatCost()` preserves
gold-before-wood debits and leaves every other resource untouched. `addBoatCost()`
adds wood then gold to an existing int cost row, preserving any dock-building
cost already present. The three operations use one pair of named price values.

Boat creation and payment remain separate. The map event checks dialog success
and creation success before charging; town docks additionally update the dock
building before charging and refreshing resources. AI uses the player pointer
captured before dock/boat construction, while UI callers resolve their player
at the existing payment stage. The local-player getter reads protocol/player
identity and human flags, which neither debit changes. Missing locations,
capacity checks, ownership claims and the different affordability timing remain
with the callers. Map loading, remote replay and Summon Boat still use
`createBoat()` without paying a purchase cost. AI path valuation retains its
missing-shipyard return and dock-plus-boat estimate; marking keeps its full
resource-row test when a dock must also be built.

Hill-fort, sacrifice and skeleton-transformer click handlers now share
`TViewArmyWindow::showCenteredCreature()`. It owns the identical stack lifetime:
construct at the existing 119/32 coordinates with the same OK-button policy,
center, run quick view or modal view, and destroy before returning. Caller
creature/amount/selection guards remain outside it. Recruitment stays at its
fixed position, and game/combat army views keep their heap-owned windows and
post-dialog upgrade/dismiss/spell work. No new data members or virtual slots
are introduced. All new helper names and ordinary source placement are project
inferences, without invented native addresses or inline declarations.

## Quest payment and completion

The resource quest's pointer-walk payment is another complete seven-resource
debit. Its native `takePayment()` wrapper now calls `payResourceCost()` with
the existing int cost row. The owner lookup and unconditional ascending debits
stay at payment time; satisfaction and owner-validity checks remain in their
existing caller/predicate, without a new affordability check inside payment.

`TSeerHut::completeQuest()` shares the payment, reward delivery and quest-pointer
clear used by `doSeerEvent()` and the retained Dreamcast `doCompletionDialog()`
boundary. It is a private, project-inferred operation with provisional ordinary
source placement and no native identity claim. The active event keeps its human
dialog lifetime and AI value gate. Completion neither deletes the quest nor
resets reward data, visit bits or the serialized legacy completion byte.

Quest guards retain their different completion operation: payment, map-object
erasure/fizzle, then pointer clearing. Expired guard visits and empty/expired
seer visits also retain their different visit-mask behavior. Map initialization
and versioned save loading preserve their distinct field updates. Resource
description loops remain separate pending review of local string/vector
lifetimes and the choice between virtual and qualified calls.

## Hover cache transitions

Nine rollover handlers share `heroWindowManager::updateHover()`: hero screen,
hero swap, kingdom overview, army split, recruitment, thieves' guild, mage
guild, ship purchase and building purchase. It compares the dispatched widget
ID and stores a changed ID before the caller updates its text. Each caller
retains its original early return or fallthrough, including swap chat updates
and ship animation. `convertToHover()` remains outside the helper because it
dispatches through windows; the manager is resolved again after that dispatch.

`invalidateHover()` shares the -1 reset across construction, both dialog pumps
and hero-screen widget handling. The two Shift-key paths use `refreshHover()`
to invalidate and then queue a mouse move. Plain invalidation does not enqueue
input; forced moves elsewhere retain their existing cache behavior.

The town screen and fort page share a separate `townManager::updateHover()`
over their ID/qualifier pair. Either change stores both values before rollover
work. The town screen retains its z-buffer ID resolution. The hall still uses
its ID-only cache operation without writing the qualifier. Town construction,
opening and commands share ID invalidation, while retaining respectively -1,
0 and the existing modifier value. Garrison and tavern windows own their own
pair; the blacksmith owns another ID-only cache. They are not redirected to a
manager cache.

Native heroWindowManager `0x1010` / field list `0x1893` explicitly records
public `lastHover`; townManager `0x1ba0` / `0x719f` likewise records public
`lastHover` and `lastQualifier`. Their visibility and layout remain intact.
These helper names and ordinary source placement are project inferences,
without native address or explicit-inline claims.

## Sacrifice display resets and map-object initialization

Creature-mode initialization and completed creature sacrifice share two private
display operations. `updateUnselectedCreatureOffering()` calls the existing
offering updater, then hides the source and offering selection borders in that
order. `clearCurrentCreature()` writes the -1 group sentinel and calls the
existing updater, which zeroes the amount and hides the four current-creature
widgets. Both retain the canonical nested update operation.

Initialization still zeroes each offered amount and assigns its group before
refreshing it. Sacrifice still deducts troops, dismisses depleted stacks and
then clears each offered amount. The helpers leave name visibility, experience,
button ordering and slider changes in their callers. In particular, only
completed sacrifice resets slider resolution/state. Selecting a different
creature hides the old borders in the opposite order and retains that sequence.
The current-creature record has no selection borders, so it does not use the
array-offering border helper. Both new methods are project-inferred ordinary
source definitions; private storage and object layout remain unchanged.

`ExtraInfoUnion::randomizeFountainLuck()` shares the initial-map and weekly
fountain operation: one `random(0, 3)` call, with zero mapped to -1 and other
results kept as drawn. Assignment stays through the signed four-bit luck field,
preserving adjacent packed bits. Initial randomization still clears visited
bits afterward; weekly refresh preserves them and retains the surrounding
hero restore/obscure sequence. It does not apply a hero bonus or mark a visit.
The helper name and ordinary owner-TU placement are project inferences without
a native address or explicit-inline claim.

Five map-initialization cases share private `game::addCreatureBank()`: ordinary
creature banks, derelict ships, sepulchers, shipwrecks and dragon cities. The
operation clears visits, records the current bank-pool size, clears the empty
flag, constructs a local bank, calls the native `initializeCreatureBank()` and
appends a copy. Its local bank is destroyed before returning to the map scan,
including normal exception cleanup. No rollback or new bounds check is added.
The ordinary bank still selects its enum kind from the cell subtype; the four
special objects keep their fixed kinds. This project-inferred operation adds
no fields or virtual slots and retains the existing enum parameter type. The
game header includes its defining creature-bank header directly.

## Event eligibility and creature-bank state

Hero event handling and AI valuation share `hasFountainEffect()`,
`hasIdolEffect()` and `hasTempleEffect()`. These query the same combined flag
sets: fountain curse or any positive tier, either idol benefit, and either
temple morale tier. They do not represent permanent site visits. Individual
effect application, descriptions and post-battle clearing retain their
separate bits. AI movement-cost guards remain after these predicates; event
dialogs, day-dependent rewards and information/visit marking keep their order.

Library events and AI also share `meetsLibraryLevelRequirement()`: hero level
plus twice the Diplomacy rank must reach ten. Each caller still checks its
per-library visit mask first. The event's cached visit mask, four skill awards
and final marking remain unchanged, as does the AI's reward valuation.

`ExtraInfoUnion::creatureBankIsEmpty()` replaces four repeated checks in
ordinary-bank events, dragon-city events, undead-lair events and AI valuation.
`setCreatureBankEmpty()` shares the occupied/empty writes in bank creation and
successful reward completion. It changes only the existing packed empty bit;
it does not erase guards, rewards, pool indices or visit bits. Reward completion
marks empty after every award and before checking hero level. AI still resolves
the bank record before checking empty, and the distinct event prompt/return
rules remain intact.

Native hero `0x1a6e` / field list `0x3f74` proves `flags` public; creature-bank
info `0x2d41` / `0x2d40` proves its packed members public. Storage visibility
and layout remain unchanged. New helper names and ordinary owner-TU placement
are project inferences without native address or explicit-inline claims.

## Permanent hero visit masks

Training grounds, Library, Tree of Knowledge, Magic School and War School now
use hero-owned visit queries across events, AI valuation and map help, with
paired marking methods in their event handlers. The methods accept an existing
`unsigned long` mask. They do not decode cell data, introduce bounds checks,
cache a cell pointer or choose a new time to read an object ID.

Library and Tree of Knowledge keep their original local masks across dialogs
and rewards. Training grounds and both schools still compute their marking
mask from a fresh cell read after the experience/skill award. Gold charging,
information flags, cancellation, level checks and the Library's skill-award
order stay with each caller. Rollover retains its low-five-bit ID extraction;
quick info retains `getItemId()`. The AI paths likewise retain their own ID
reads and prior affordability/value calculations.

Arena's native `visitedArena(cell)` boundary now calls the same mask query used
by its two map-help paths. Its native setter and event/AI callers remain in
place. Map-help consumers use only zero/nonzero status, so the shared predicates
normalize that result to bool before selecting the existing visited text.
Across these six site families, 22 query sites share the owner predicates;
five event writes use the new marking operations. Save/load and initialization
continue to transfer/reset the complete masks in their original order.

Native hero `0x1a6e` / field list `0x3f74` records all six masks public. No
visibility, data layout or virtual slots change. New names and ordinary source
placement are project inferences without invented native addresses or inline
qualifiers; the existing native Arena annotations remain attached to their
original interfaces.

## Player visits and effect flags in map help

Four player-owned visit masks now use shared queries and marking operations:
skeletons, lean-tos, magic springs and mystical gardens. Ten query sites span
AI, rollover and quick info; five event writes retain their original position.
The methods accept caller-computed masks, preserving packed ID reads, existing
locals and the selected player. In particular, skeleton AI still checks the
current player's visits before looking up the hero owner's resource valuation.

Lean-to visits are marked after either the empty dialog or reward/depletion.
Spring and garden visits are recorded before their fullness checks, including
visits that grant nothing. Skeleton marking remains after its reward/empty
branch and rereads the item ID there. The two map-help paths still test spring
and garden fullness only after a positive player-visit result; weekly refills
therefore retain their existing display behavior. Save/load and initialization
continue to handle complete masks in their original order. Native playerData
`0x1c50` / field list `0x35de` proves all four masks public.

Rollover and quick info also share the fountain, idol and temple effect-flag
sums. `getFountainEffectFlags()` preserves the four-term sum supported by
Dreamcast advmgr.cpp line 3491 and the existing retail evidence. The two-term
idol/temple sums remain explicit; temple uses rollover's term order for both
pure, nonvolatile reads. Terms occupy disjoint bits and cannot overflow.
The existing Boolean effect predicates now call these helpers and test for a
nonzero result, preserving the complete helper path through event/AI callers.
Individual effect application and post-battle clearing remain unchanged.
Names and ordinary source placement are project inferences; no native helper
address, explicit-inline qualifier or new storage is introduced.

## DirectPlay interface replacement and names

Base initialization and lobby connection now share `releaseDirectPlay()`: release
an existing interface, then clear its pointer. Lobby initialization calls the
qualified base `CDPlay::init()` before replacing its lobby interface. A failed
base initialization still leaves the existing lobby untouched. The protected,
nonvirtual helper changes no connection flags or virtual slots. Destructors
retain their release-only operation without pointer clearing.

Four group-creation/name-setting paths share the four-field `DPNAME` setup.
Names remain borrowed; each setter retains its missing-long-name fallback.
Player creation remains distinct: it zeroes the record and supplies only the
short name, leaving the long name null. The external record layout is unchanged.

Player and group name queries share their output-copy operation, preserving
short-before-long order, optional outputs, `strncpy` bounds and the zero-byte
write for a null source. No extra terminator or bounds clamp is introduced.
The later name-query continuation below also shares their two-query protocol,
while preserving HRESULT transitions, size-plus-one allocation and local
`CDPlayMsg` destruction. These helper names and ordinary TU-local/source
placements are project inferences, without native address claims or invented
inline qualifiers.

The nearby data/session/address readers differ in accepted first-query results,
zero-size handling and when output sizes become visible. The later data-reader
continuation shares the player/group data protocol separately; similar allocation
tails alone do not justify merging it with names, sessions or addresses.

## Chat queue removal and system-message state

Capacity eviction in `addChat()` and timed expiration in `killOldChat()` now
share `discardOldestChat()`: clear the oldest record's timestamp, advance the
circular index through the existing `getNextMsgNbr()` helper, then decrement
the count. Both callers retain their nonempty/capacity guards. Expiration keeps
its clock reads, changed/killed flags, later-message timeout extensions and
scroll-position update. Insertion keeps its pre-eviction newest-message snapshot
and post-insertion position/changed updates. The helper does not erase text or
system-message tags, and `clearChat()` retains its separate full timestamp sweep.

Ordinary system messages and turn-duration messages share `addSystemChat()`:
set the system flag, call the existing variadic `addChat()`, then clear the flag.
Formatting and sound selection stay in each caller, as does the turn-duration
display guard and unconditional sound tail. The existing interpretation of the
formatted string by `addChat()` is preserved. Player-enter and player-drop
announcements intentionally remain distinct: they keep the flag set through
their sound helper before clearing it. The new private methods introduce no
storage, virtual slots, native address claims or explicit-inline qualifiers;
their names and ordinary source placement are project inferences.

## Resource-quest requirement lists

`type_resource_quest::getRequirementText()` and `setDefaultText()` now share
the positive-resource formatting loop through private `appendRequirements()`.
It visits resource indices zero through six in order, skips nonpositive costs,
formats the full amount and resource name, then appends the string. Both callers
keep their existing vector and scratch string, including their construction and
destruction order. Default text still selects its quest-text row before building
the list, joins into its existing scratch string and fills only empty proposal
and completion text. No new virtual dispatch through `getRequirementText()` is
introduced. The helper name and ordinary source placement are project inferences.

The proposal dialog remains a different operation: it lists requirements the
player cannot cover, and appends corresponding picture records alongside text.
The progress dialog uses positive requirements but builds only pictures. Neither
is substituted with the text-only list helper. Serialization and payment remain
unchanged.

## AI spell initialization and restoration priority

Both `type_spell_choice` constructors now use private `initializeSelection()`
for the same four stores: zero value, invalid primary and secondary targets,
then a cleared cast-now byte. Each constructor still initializes its enchantment
base with its original spell/mastery/power/duration first. The shared operation
does not conflate later tactical hex targets with simulated-combat vector indices
or Dispel fallback handling. Existing public data and field layout remain intact.

`type_AI_combat_parameters::getRestorationPriorityValue()` shares the conditional
value doubling across hero Resurrection, Sacrifice and creature resurrection.
The condition remains awake friendly value greater than awake enemy value,
followed by an estimated remaining-round count at most one. Each caller retains
its original base valuation, target eligibility and comparison with the best
candidate. Sacrifice still rejects a nonpositive net value before the adjustment;
creature resurrection still distinguishes the Pit Lord's demon valuation from
the Archangel's restored-stack valuation.

Resurrection and Sacrifice also share private `shouldRestoreNow()`. It checks
whether the restored stack is current, then the existing win-likely flag, then
calls the existing `isLastAction()` helper only if needed. Timing is still
evaluated after saving the candidate's value and target(s). Teleport's
incapacitation condition and ordinary enchantments' flag-based timing remain
distinct. No target or current-army read is cached across the existing helper
calls. The three new methods use project-inferred names and ordinary source
definitions; no native addresses, explicit-inline qualifiers, fields or virtual
slots are added.

The attack-hex chooser has a different reset boundary: its constructor clears
attack time, but `findAttackHex()` retains it while invalidating only value and
hex. The constructor interleaves those stores with other setup. That review does
not justify replacing either path with a full reset of the result triple.

## AI adventure-shipyard flags

The native `markShipyards()` and `clearShipyards()` boundaries now share their
owned adventure-shipyard traversal through TU-local `setOwnedShipyardBuildFlags()`.
The helper visits the current player list in order, reads each shipyard's packed
boat coordinates, skips the X-coordinate `NO_BOAT` sentinel, then changes only
the boat cell's build flag. It retains the fresh list read for the map level
after the shipyard lookup. The copied point is a value record with no custom
copy constructor or destructor; both callers use the same lookup sequence.

Setup still returns early when the player cannot afford a boat, prices town
docks first, and marks adventure shipyards afterward. Cleanup still visits town
docks first and uses its original two-coordinate validity guard. Neither town
loop is folded into the shared traversal. `moveHero()` still brackets the search
with the native marking/cleanup helpers and danger-zone setup/clearing in the
same order. The new helper's name, Boolean flag parameter and ordinary source
placement are project inferences; existing native annotations remain on their
original functions. No field visibility or storage changes are involved.

## Path results and spell-picker phase state

Adventure-search clearing and combat-path setup now use the existing
`searchArray::clearPath()` for their result-vector clearing. Their other work
stays ordered as before: adventure clearing empties the queue first and visited
points afterward, while combat setup constructs its path-cell temporary, clears
the result, then clears its queue through the existing reference. The two paths
do not share a broader reset or change visited-cell/terrain initialization.

Four spell-picker phases now each have one TU-local cache-reset operation,
shared by confirmation and cancellation: Sacrifice beneficiary, sacrificed stack,
Teleport source and Teleport destination. The two Sacrifice validity fields retain
their distinct byte/int types; both Teleport phases retain separate hover and
selection sentinels. Each caller still selects its pending-action target or
cancels the opcode before clearing its own cache, then ends the dialog. Invalid
hover handling remains a partial update and does not use these complete resets.

Four invalid-target paths share `showInvalidSpellTarget()`: ordinary rollover,
both Sacrifice phases and Teleport source selection. It restores the combat
pointer, obtains the caller-selected general-text prompt, invokes the existing
failure-reason helper and turns off the highlighter. Globals are resolved at
each original stage; the prompt lookup remains after the cursor update. Existing
`getText()` calls are retained, and the three `operator[]` spellings use that
same accessor body directly. Caller validity/sentinel writes remain before this
display operation; area highlighting remains afterward where applicable.

Wall spells retain their distinct mouse-grid handling, and Teleport destination
failure retains its direct message without highlighter cleanup. Ordinary spell
confirmation and cancellation retain their different ordering of cache clearing
and area-highlight cleanup. The new helper names and ordinary source placement
are project inferences; no native addresses, inline qualifiers or storage changes
are introduced.

## Widget initialization and pointer-change exits

The full widget constructor and `initialize()` share the three detached-link
stores through private `initializeLinks()`. Both constructors share the null
rollover/right-click pointers and cleared ownership byte through private
`initializeHelpText()`. Calls remain at their original positions: the full
constructor sets geometry before links and help state after style, while
`initialize()` clears links before geometry and retains help state. The default
constructor still initializes only its original subset. Derived button, border,
icon and text initialization continue through the existing base initializer.
Window insertion/removal retains its neighbor-link updates; help replacement
and destruction retain their different deletion order and ownership handling.

The mouse manager's negative-frame and unchanged-frame exits share private
`finishPointerWithoutRedraw()`: decrement busy state, call the existing `enable()`
helper, then clear the set-pointer guard. The caller's critical-section scope
continues through the helper and unlocks on return. The successful update keeps
its different sequence of loading, enabling and drawing before lowering busy
state. Earlier exits that never acquire the set-pointer guard remain unchanged.
All three new methods have project-inferred names and ordinary source bodies;
field visibility, storage and virtual slots are unchanged.

## Input queue commits and partial message resets

Keyboard, mouse and forced-mouse producers share `commitBufferedEvent()` after
writing the event and querying current modifiers. It advances the tail modulo
64 and advances the head only when the new tail reaches it, retaining the
oldest-event discard policy. Callback callers still resolve `g_inputManager`
after the modifier query, while forced input uses its original receiver.
Capture/release calls, coordinate decoding, busy-flag handling and keyboard menu
shortcuts stay in their original positions. Keyboard input does not acquire the
mouse producer's busy guard.

Opening, active close and flush share private `resetQueueIndices()`, storing
tail zero before head zero. Opening retains its preceding whole-buffer wipe;
flush retains its preceding Windows-message pump; close retains its status guard
and subsequent filter/status changes. `getEvent()` still advances the head,
whereas `peekEvent()` only normalizes it modulo 64. Both keep their sound poll,
message copy and conditional ASCII conversion.

The native-event bridges share `message::clearInputFields()`, clearing their six
input fields in the existing chained-assignment order while retaining extra and
window. Empty/inactive queue results share `setNoInput()`, clearing id, codeY,
codeX and qualifier in that order. Their default-constructed local message stays
in each caller; neither partial reset replaces the complete message constructor.
Forced mouse movement retains its direct writes without acquiring an extra clear.

Dreamcast inputManager `0x240a` / field list `0x240b` declares the buffer, head,
tail and busy flag public. Their visibility and types remain unchanged, as do
the public native message fields. The four new methods use project-inferred
names and ordinary definitions in the input TU, with no added virtual slots,
storage, native addresses or explicit-inline qualifiers.

## Sound playback state and temporary overrides

Seven temporary playback overrides share `soundManager::enablePlayback()`,
which returns the previous raw integer state before enabling playback. Button
feedback, chat audio, both options volume adjustments and save transmission
retain integer snapshots; the two save-reception scopes retain their existing
`char` narrowing. Each caller restores its snapshot explicitly through
`setPlaybackState()` at the original stage. Button sample setup remains inside
the override, and chat stores its returned sample handle before restoration.
Global manager lookups after intervening calls remain fresh.

Nine sample/music paths share `isPlaybackAllowed()`, combining the playback
state with solo mode. Global no-sound suppression, driver availability, volume,
sample-pointer and channel guards remain in their original short-circuit order.
Environment-origin updates continue to query only the raw playback state;
individual sample stop/query operations do not acquire a playback gate. Turn,
video and shutdown transitions retain their explicit enable/disable sequence
rather than acquiring a saved-state restoration.

The PC playback field is private and all external reads/writes use its owning
operations. Its integer storage and position are unchanged. Dreamcast
`soundManager` type `0x215e` / field list `0x231f` does not supply this PC field;
its native-public driver, change-sounds, MP3-playing and critical-section fields
remain public. The four new method names and ordinary sound-TU definitions are
project inferences, with no claimed native addresses or explicit inline
qualifiers. Constructor and opening initialization remain owner-local writes.

## Manager insertion, detachment and resumption

Eight manager-add sites share `executive::addManagerOrShutDown()`: four dialog
stack additions, new-manager entry, both saved-manager restoration paths and
adventure entry from the main menu. It keeps default priority `-1`, calls the
existing insertion operation and looks up the common error text only on failure.
Input/mouse/window startup retains its distinct error messages. Shutdown may
return when already in progress, so neither the helper nor its callers acquire
an unconditional return, throw or no-return annotation. Dialog stack snapshots,
manager receivers and exception boundaries remain in their original callers.

Both removal branches share `baseManager::clearLinks()` after close and neighbor
repair, clearing previous then next. Head/tail repair stays in the executive;
construction retains its next-then-previous member initializers, and dialog
restoration keeps its saved links rather than clearing them. Dreamcast
`baseManager` `0x1866` / field list `0x1867` explicitly declares both links public;
`executive` `0x51da` / field list `0x51e4` also declares its list state public.
Their visibility, storage, and virtual slots are unchanged.

Normal return and exception unwinding share TU-local `resumeAdventureManager()`:
set active status through the existing setter, then wake the adventure window's
widgets. Global accesses remain separate. Success-only redraw, menu, hover,
bottom-view and fade restoration stays in the normal branch; exception handling
retains its rethrow and outer current-manager restoration. All three helper
names and ordinary source placements are project inferences, without native
address or explicit-inline claims.

## DirectPlay compound-address connection creation

TCP/IP, IPX, modem and serial factories share private
`CDPlayLobby::createConnectionFromElements()`. It queries the address size,
requires the existing buffer-too-small result, allocates the temporary packed
address, performs the second COM call, constructs the connection's independent
copy and releases the temporary. Failure after allocation still frees it and
returns null. Both COM calls read the owning lobby pointer separately and retain
their result in the existing error field; zero-size and allocation/constructor
exception behavior are unchanged.

Each factory retains its element array, optional parameter handling and provider
identity. TCP/IP and IPX retain their address-enumeration containers through the
shared operation and destroy them after its return, keeping all borrowed element
buffers alive while packing and copying. Remote connection initialization still
owns and deletes the returned connection on either initialization outcome.
The scalar address-size scratch moves into the common operation; no object
lifetime, field layout or virtual slot changes. The helper name and ordinary
source placement are project inferences.

Player/group data readers, player-address queries and lobby-settings probes
retain their separate protocols: their accepted first results, output-size
updates and allocation ownership differ from compound-address construction.
The player/group data pair now uses its own shared protocol, described below,
rather than being routed through this factory operation.

## DirectPlay borrowed address-element setup

Ten address-record assignment sites now share the GUID/size/borrowed-pointer
operation `initializeDirectPlayAddressElement()`. Two of those sites are nested
in the shared TCP/IP and IPX enumeration-copy loop,
`copyDirectPlayAddressElements()`. The loop retains virtual `getCount()` and
`get()` calls and advances the caller's count after each copied record.

Provider, IP-address, modem, phone and serial records retain their original
optional guards and positions. String lengths still include the terminator;
serial data retains its fixed `0x14` size. Enumeration and container ownership
stay in each factory, through packed-address construction and connection copy.
The helpers borrow data without allocating, freeing or adding a new capacity
policy. Both are project-inferred TU-local operations; SDK record layouts and
native container interfaces are unchanged.

## Town and garrison strip-selection state

Town setup and garrison-dialog setup share `clearStripSelection()`, clearing
destination, source and current pointers before setting their indices to `-2`
in the same order. Town setup retains the preceding loaded-town update;
garrison setup retains its preceding null town pointer. Neither call redraws
or clears divide state.

Eight current-strip/index assignments and five destination-strip/index
assignments share `setCurrentStrip()` and `setDestinationStrip()`. These cover
constructor pairs, portrait targeting, existing `selectArmy()` / `armyCommand()`
boundaries and both pages' right-click paths. Each operation writes the strip
before its index; portrait `-1` and troop indices remain caller-selected.
Command generation, ownership checks, text, split rules and army previews stay
at their original stages. Existing native helper calls are retained.

Opening retains its separated index and pointer initialization. `newStrips()`
retains its four-field `-1` reset; `resetStrips()` still redraws then clears only
source/destination state to `-2`, preserving the current pair. Single-field
merge-loop index changes and the source-selection latch remain distinct.
Dreamcast townManager field list `0x719f` explicitly declares all six selection
fields public; their visibility and types remain unchanged. The three new
methods have project-inferred names and ordinary source definitions without
added storage, virtual slots, native addresses or inline qualifiers.

## Quick-preview placement

Hero, town and creature adventure previews share the coordinate-only clamp
through `heroWindow::centerQuickView()`. The native `TQuickTownWindow::center()`
wrapper delegates to that operation, and town preview now calls the wrapper
instead of repeating its body; garrison preview keeps its existing wrapper call.
Half-width/height arithmetic and the final-pixel bounds remain unchanged.
`centerWindow()` and `moveWindow()` retain their distinct top-left/default-center
semantics and background/redraw behavior.

Three fixed hero previews share `TQuickHeroWindow::quickWindowWaitAt()`, storing
x then y before calling the existing native quick-view wrapper. Both town hero
portraits retain `(0x140, 0x172)`; the hero-screen locator retains `(0x1a4, 0x172)`.
Their block-scoped windows, including the Dreamcast-named `infowin`, remain in
the callers with their original construction/destruction boundaries. Adventure
visibility selection, shadow flags and creature-window heap ownership also
remain in their callers. The town-locator popup keeps its separate placement.

Dreamcast heroWindow `0x101c` / field list `0x1a5f` declares x, y, width and height
public; their visibility, types and layout are unchanged. New method names and
ordinary source placement are project inferences. Existing native centering and
quick-view wrappers remain on the complete call paths, with no new virtual slots,
native-address claims or inline qualifiers.

## Bink pause transitions and campaign snapshots

Eight pause/resume pairs share `BinkManager::setTrackPaused()`, which writes the
raw paused state then calls the vendor operation for the selected track. Campaign
opening, selection and hover retain their primary-track calls. General video
pause/resume retains its nesting counter, first/second-track guards, separate
state updates for each present track, Smacker state and final sound toggle.
The helper adds no guard or pause-count policy.

Campaign preview opening and destruction share `savePlaybackState()`; destruction
and hover share `restorePlaybackState()`. Both copy the entire twelve-word native
aggregate with `memcpy`. Opening still clears only the active primary handle
after saving. Destruction still checks each saved primary handle, restores,
closes through `closeBink()` and saves the resulting state before moving to the
next row. Hover retains text hiding/showing before restore and restart afterward.
The existing six-row destruction bound remains unchanged.

`closeBink()` keeps its separate pause-before-close operations and final reset;
it does not acquire the shared state write before each close. Track-transition
and playback-completion resets also retain their own sequences. New helpers are
ordinary namespace functions with project-inferred names, preserving the native
aggregate, globals and SDK boundary without new storage or inline claims.

## DirectPlay data retrieval and player-slot setup

The native virtual `getGroupData()` and `getPlayerData()` boundaries delegate to
private `getPlayerOrGroupData()`, using the existing player/group discriminator
to select the corresponding COM method at both query stages. It retains the
optional input size, initial null-buffer query, buffer-too-small-only allocation
path and raw `operator new` / failure `operator delete` ownership. An initial
successful query still returns null while publishing the reported size; a
buffer-too-small result clears the stored error before the zero-size early exit.
Other first-call failures and second-call failures return without updating the
caller's size. The second call reads the current COM interface again.

New-game and load-game selection share private `setupPlayerSlots()`. Network
hosts add the local record; clients preserve original-game setup, chat hiding,
announcement transmission, enumeration and the per-player version-buffer guard.
The announcement remains alive through enumeration, and each `auto_ptr<int>`
version remains alive through that player's slot update with scalar deletion.
Hot-seat preserves its per-seat temporary records, local identity writes and
manager deletion/nulling; ordinary local play retains its configured name.

Each mode still sets duration before player initialization and reevaluates host
status afterward. Header discovery, the distinct new/load filenames, map
selection and load-only slider disabling remain in the callers. Save mode still
skips both setup paths. Both new methods are project-inferred ordinary bodies;
existing virtual slots, native interfaces, field visibility and storage remain
unchanged. Address, name, lobby-settings and compound-address readers retain
their distinct allocation/error protocols.

## Shared volume-setting calculation

`convertVolume()` now selects the music/effects setting by reference and applies
one copy of the existing range-and-scale calculation. Only `VOLUME_TYPE_101`
selects music; every other type still selects effects. Settings outside `1..10`
produce zero, valid settings retain `(setting + 1) * volumeValue / 10` and its
minimum-one rule, and the original final `0..127` clamp remains.

All sample and music callers keep using the existing native operation, including
sample playback's separate zero-volume branch. This is a project-inferred
consolidation inside the established owner function, without another helper,
interface change, arithmetic widening or normalization of the stored settings.

## Empty MP3 requests and pooled resource strings

Both empty-request exits in `processStopAndPlayMP3()` share TU-local
`finishEmptyMP3Request()`: release the name-change lock, release the MP3-change
lock, then end the thread. The first exit remains before stopping the existing
stream; the second remains after stopping/closing it and reacquiring the name
lock. Each caller retains its explicit return. No owning C++ temporaries are
live at these exits, no destructor-based cleanup is introduced, and each unlock
still resolves the global manager independently. The successful path and other
thread exits retain their distinct lock state and cleanup.

Six pooled text-copy sites share `copyResourceString()`: campaign and region
names, music tracks, artifact names/descriptions and artifact-slot names. It
scans the source length once, includes the terminator, copies with `memcpy` and
returns the original unsigned byte count. The callers still assign the table
pointer before advancing their local cursor, preserving writable versus const
pointer types and text-line progression. Existing resource getters and the
native two-parameter `initializeArtifactTraits()` boundary remain in place.

Allocation, sizing passes and resource guards stay with the loaders. Campaign
map names retain their reassigned static pool; music/artifact loaders retain
their one-time static allocations. Blank-line/header skips, region counts and
artifact metadata initialization are unchanged. The new names and ordinary
source placements are project inferences, with the resource-copy declaration
shared through the existing text-resource header and one body in its source.
No new native addresses, inline qualifiers or ownership policies are claimed.

## Combat highlighter and obstacle state

`checkChangeHighlighter()` reuses the native `turnOffHighlighter(0)` after its
existing guarded effect marking. Both `markCreatureEffect()` branches were
read: ordinary armies update the effect bitmap and towers update their archer
effect, without changing highlighter state. The call therefore preserves the
flag/index clear while retaining the caller's has-army guard, extent reset and
final redraw. Death's flag-only clear and constructor initialization remain
distinct; neither acquires an index reset or draw.

Hex cells share three obstacle operations: `setObstacle()` ORs the caller's
attributes before recording the index; `clearObstacle()` clears only the supplied
mask before invalidating the index; `resetObstacle()` invalidates the index then
clears all attributes. Each has two call sites. Footprint removal retains the
full obstacle mask, while anchor removal retains only `obstacleOrigin`. Terrain,
boat and siege blocking that does not identify an obstacle stays separate.
Constructor/map initialization retain their different ordering relative to army,
corpse, shading and background initialization.

Three obstacle walks share private `getObstacleFootprintHex()`: placement
eligibility, attachment and removal. It adds the signed offset and applies the
original odd-origin/even-destination row correction through existing `gridY()`
and `rowIsOdd()` calls. Each caller retains its cached origin parity, loop,
column/overlap checks and anchor processing. Removal keeps sprite disposal and
pointer clearing after the cell updates.

Dreamcast hexcell `0x1fb1` / field list `0x4434` explicitly declares attributes
and obstacle index public; visibility, stored types and layout are unchanged.
The four new helpers have project-inferred names and ordinary owner-source
bodies, without new native-address claims, storage, virtual slots or inline
qualifiers.

## Army animation transitions

Movement, wall attacks, highlighting, combat drawing and spells now share
`army::startAnimationSequence()`: select the sequence, then reset its frame
index to zero. The call replaces paired stores across six translation units,
including both guarded wince/death branches in `showMassSpell()`. Existing
sequence guards, frame advancement, terminal death poses, sound calls and
redraws retain their caller-specific behavior.

`updateHighlightAnimation()` shares the complete conditional from area-effect
highlighting and pointer highlighting. It first tests whether fidget is valid
and the stack is not already fidgeting; otherwise a non-waiting stack returns
to wait. In particular, an already-fidgeting stack returns to wait rather than
restarting fidget. The two callers retain their different effect-marking order
and area-effect latch guard.

`finishFidgetAnimation()` shares the wait/frame-zero transition followed by
`GameTime::get()` in cycling reset and normal fidget completion. It nests the
sequence helper. Normal completion keeps its subsequent random timer adjustment;
cycling reset keeps its immobilization guard and separate hero timers.

The remaining separate stores were read in context. Initialization interleaves
other state; `playAnimation()` accepts a caller-selected starting frame and
performs setup between sequence selection and its loop; flying and directional
attacks likewise set the sequence before their frame loops. Armageddon plays
the sample between sequence selection and frame reset. Single-target spell
wincing writes frame zero only if its frame loop runs. Those operations retain
their order and partial-update behavior rather than acquiring an early reset.
`setupAnimation()` captures the background and draw bounds and remains a
distinct operation.

Dreamcast army `0x1a95` / field list `0x205b` explicitly declares
`currFrameType` and `currFrameIndex` public. Their authored types, visibility
and layout are preserved. The three operation names and ordinary bodies in
`army.cpp` are project inferences, without new native-address or inline claims.

## Spell reaction steps, hero animation and DirectPlay name queries

`army::advanceSpellReactionAnimation()` shares the complete per-stack frame
step from Armageddon and mass-spell display. It advances while frames remain,
resets an exhausted wince to wait through `startAnimationSequence()`, and holds
other exhausted sequences, including death. `CSprite::getNumFrames()` and its
sequence-validity helper were read: neither modifies the army, so the existing
cached-sequence form also represents Armageddon's condition. Caller effect
masks, show-wince guards, frame loops and overlay updates retain their order.
The different `powEffect()` completion rules remain separate.

Private `combatManager::startHeroAnimationSequence()` shares four hero
sequence/frame-zero pairs: icon initialization, cast completion, cycling into
a selected sequence, and cycling back to idle. The latter alone updates its
fidget timer. Cast startup still selects its sequence before a conditional
frame loop, without acquiring an unconditional frame-zero write.
`clearPendingHeroReactions()` shares the four pending-yeah/pending-doh clears
in the accepted and rejected local-human reaction branches. Played-this-round
latches, sprite availability and the two branches' different effect/sequence
store order remain with their callers. Round and initial setup retain their
different full-reset order. Both new manager operations are private; no stored
field, virtual slot or layout changes.

Private `CDPlay::getPlayerOrGroupName()` now owns the two name-query flows. Both
native virtual entry points retain their signatures and call it with the
existing player/group discriminator. The helper keeps a local `CDPlayMsg`,
accepts only `DPERR_BUFFERTOOSMALL` from the first query, uses the original
unsigned size-plus-one allocation, rereads the COM interface for the second
query, and copies outputs only after success. The native `CDPlayMsg::allocSize()`
now performs allocation: the newly constructed message has a zero pointer and
size, so its reuse/delete branches add no behavior, including for a wrapped
zero allocation size. Destruction still uses the existing message cleanup on
all exits, after any short/long copy. Name truncation and missing-name handling
remain in `copyDirectPlayName()` without extra termination or clamping.

The four new helper names and ordinary owner-source placements are project
inferences. Native APIs and existing nested helpers remain intact; no new
native-address or explicit-inline claims are made.

## RMG connection state and invalid positions

Five connection-search updates share `TRmgMapItem::setConnectionPathState()`:
zone-path cost, connection direction, then connection eligibility. Existing
cost/direction setters remain nested in the operation, preserving their bitfield
conversions. Cross-zone flooding retains its source-zone value and reversed
direction; water-distance seeding/relaxation retain zero eligibility; water
preparation retains cost 32000 with zero eligibility; the whole-map connection
reset retains cost 32000 with eligibility -1. Movement cost and predecessor
reset remain separate and follow the connection-state update as before.
Cell initialization retains its different 32700 costs, zone/score writes and
partial predecessor sentinel. The helper's name and ordinary cell-owner source
placement are project inferences, without a new native-address or inline claim.

Six full invalid-position initializations now reuse the existing retained
`TRmgMapPosition(-1, -1, -1)` constructor. Five are local predecessors or
unspecified placement positions in connection/junction preparation, the two
treasure-group selection paths, and river creation. Their scopes and per-loop
construction stay in place. The object constructor nests the position value
through its existing `setPosition()` after incrementing the properties reference
and before clearing placement marks. The existing point base, three-coordinate
layout and by-value movement helpers remain intact.

X-only sentinels and the two-coordinate Voronoi position remain distinct;
neither gains a third coordinate or extra initialization. Guard prototype
exclusion was also read: its two -1 writes are individual local-array entries
inside different filters, with the RoE loop's shared counter and skipped slot
117 feeding the eligibility scan. They do not form the connection or position
reset operation and retain their original loop behavior.

## RMG clipped scan rectangles

Nineteen integer-coordinate rectangle constructions now share
`type_random_map::getClippedBounds()`. The operation clamps lower coordinates
against zero and upper coordinates against the owning map's width/height,
returning the existing half-open `TRmgZoneBounds` value. It does not force
inverted or wholly out-of-map intervals into nonempty ranges. Dimension getters
remain nested in the shared body, and the map's dimension fields stay private.
The name and ordinary source placement are project inferences, without a new
native-address or explicit-inline claim.

Callers retain their actual requested rectangles: coast and path neighborhoods,
river painting's 3-by-3 and 5-by-5 areas, object-footprint clearance, water-zone
surroundings, island radii, obstacle scans, border repair, connection clearing,
additional-town eligibility and the row below a mine. Signed conversions from
the adapter's unsigned grid point remain before the arithmetic. Prototype and
map dimension getters were read and return signed integers without mutation;
these calls and local-coordinate reads can feed the shared bounds calculation
without changing caller-visible state. Existing named upper/lower point
constructions and their scopes remain in the obstacle/water-border callers.

Treasure-group overlap clips against the group's own map after translating the
destination map limits into group coordinates. Object insertion still precedes
the bounds calculation; per-cell ownership and flag snapshots remain inside the
unchanged loop. Water preparation's interior margin and subterranean-gate bounds
intersection are separate domains and retain their different lower limits.
The junction entrance still uses its explicit `cppMin<long>`/`cppMax<long>`
path; this pass does not erase that typed-helper path in favor of the new
integer-coordinate interface.

## Main-loop campaign completion and re-entry

Two campaign-completion branches share `showCampaignCompletionDialogs()`:
`showCongrats(0)` followed by conditional pending-high-score display. The latter
also serves the main loop's common completion tail through
`showPendingHighScores()`. That operation tests the pending flag, clears it,
then reads the manager pointer and calls the existing `viewHiScore()`.
`showCongrats()` was read through its score insertion, fade and string cleanup;
its local lifetime still ends before high-score display. The native view helper
still owns the modal window. Explicit high-score menu requests retain their
unconditional display and video pause/resume/restart sequence.

Both campaign re-entry paths share `prepareCampaignScenarioStart()`, which
clears game-over before setting the start-events latch. Their original conditions
and jumps to `runGame` remain in the caller, preserving campaign-header and
briefing-window scope exits. Re-entry still starts with the existing score flag,
color-cycling and network-control setup. Lobby launch, load-game handling and
the lone game-over/campaign clears are distinct transitions and retain their
original writes.

All three helpers are ordinary TU-local project inferences in `kb.cpp`.
No native addresses, inline qualifiers, new stored state or interface changes
are introduced.

## RMG domain queries and river step costs

Eleven generator callers now use the existing `contains(const TPoint&)` query:
island/zone fill, water-distance flooding, object-score propagation, border
cleanup, connection flooding, treasure-group placement, road costs, both river
builders and river-object targets. The shared body and its original branch
callers remain intact. The query preserves X-before-Y short-circuiting and live
map-dimension access; positions bind through their existing point base without
copying or adding a level check. Seed clipping, local construction, map lookups,
terrain vetoes and loop exits retain their places.

Three river-coast scans share private `isRiverCoastPointInRange()`. The query
preserves the documented asymmetry: X may equal map width, while Y must remain
strictly below height. Water, dry and inland loops retain their distinct step
calculations, lookup forms, iteration counts and final target marking. Interior
margins and the shipyard's X-only checks remain distinct. The map and treasure
group's separate owner contexts were read but are not redirected through a
generator instance.

Both river builders share `TRmgMapItem::calculateRiverStepCost()`: add one
`rand() & 31` result and one to the predecessor cost, then add 30 if the current
cell has a road. The draw occurs after terrain eligibility and before the road
query, as before. Cost rejection, predecessor writes, work-list insertion and
the general river builder's additional impassable/direction checks stay in their
callers. The existing road getter remains nested; no random sample is cached
across steps or moved across a rejection condition.

The coast query and cost operation have project-inferred names and ordinary
owner-source bodies, without native-address, explicit-inline, field-layout or
virtual-interface changes.

## RMG failed-group disposal, retries and monolith traversal

Four failed-treasure-group paths now share `TRmgTreasureGroup::discard()`:
failed guard assembly, normal and alternate placement attempts, and failed key
tent placement. It walks the live object vector in order, calls the existing
virtual `unknownOperation()` rollback hook, reloads the slot for deletion, and
then invokes the canonical `reset()`. The base rollback is empty; the hero
override releases its reservation. Object destruction still owns property
reference release and any derived payload cleanup. The helper adds neither a
cached object pointer across the callback nor an early container clear.

The non-owning `reset()` remains unchanged for construction, reuse and successful
ownership transfer. Failed guard assembly deletes the unaccepted guard after
discard; key-tent cleanup retains its separate guard deletion and subsequent
color release. No destructor acquires blanket deletion of group entries.

Private `tryPlaceTreasureRange()` shares the complete normal/alternate retry
loop. Both calls use the same caller-owned group and live range reference,
reading minimum/maximum on every attempt. A successful placement returns before
discard; failed assembly does not acquire an additional cleanup; failed placement
after assembly calls `discard()`. The normal pass still precedes the alternate
pass, each keeps its own `RMG_TREASURE_ATTEMPTS` limit, and only exhaustion of
both passes finishes the selected density band. Group/map/vector lifetimes
remain in `placeZoneTreasures()`.

The one-way entrance/exit and two-way monolith cases in `buildRoadCostMap()`
now select their existing destination vector by reference and share the traversal.
Subtype lookup remains before traversal; size and entries remain live per
iteration. The same-subtype filter, position getter, cost-plus-50 comparison,
by-value predecessor setter and ordered work-list insertion remain together.
Underground gates retain their separate level flip and cost-plus-one operation.
The two new helper names and ordinary source placements are project inferences;
virtual slots, native entry points and stored layouts are unchanged.

## RMG key-tent reservations and quest eligibility

Four color transitions share private `setKeyTentDisabled()`: successful border
placement, key-tent guard reservation, its failure release, and border-guard
removal. The operation writes the byte first, resets the selected color to zero,
and walks the live availability vector until an enabled entry or `size()`.
The all-disabled sentinel is unchanged. Reservation still follows border-object
placement, precedes tentative treasure generation, and is released only after
failed-group disposal. Object removal still releases the color after its object
and zone bookkeeping and before clearing its footprint.

`getNextKeyTentColor()` serves the key-tent value query and border placement.
Initialization retains its bulk resize/clear and definition generation without
calling the transition helper: the existing constructor does not initialize
the cached next color, and this pass does not introduce an earlier scan.
Historical matching comments now distinguish the earlier unadopted experiment
from the shared operation implemented here; no new matching measurement is
claimed.

Three quest definitions share `canGenerateQuest()`, testing the selected
seer-hut prototype before the artifact-pool latch. Rejected definitions still
return -1; creature valuation still invokes its existing value calculation only
after eligibility, while gold/experience return their stored values.
`isQuestArtifactAvailable()` shares the count/selection filter: globally enabled,
not reserved by this generator, and in the required artifact class. Both passes
reevaluate it; the low-pool latch, random selection, replacement failure path and
successful reservation/cursor advance retain their original order.

The two selection cursors, key-tent availability vector, quest-artifact
reservation array and pool latch are private generator state. Their declaration
order and stored types are unchanged. Original RMG access control is unavailable;
this visibility and the four ordinary helper names/placements are project
inferences from their owner operations and callers, not new native source facts.

## RMG prototype lookup and spell-scroll filtering

Four searches share private `findObjectPrototypeIndex()`: border-tent and
border-guard lookup during border placement, guard lookup during key-tent
placement, and the selected quest artifact's prototype lookup. It walks the
existing vector in order, reads its live size and subtype getter, and returns
the first match or the end index. Each caller keeps its index variable and
original next operation: distinct -1/zero failures for missing border
prototypes, zero for a missing key-tent guard, and the existing direct artifact
indexing. Prototype reference-count replacement remains after that artifact
lookup. The terrain-aware randomized `selectObjectPrototype()` and mine
fallback selection remain separate operations.

The spell-scroll definition now shares private `canChooseSpell()` between its
count and selection passes. It retains flag-mask, nonzero-school and matching-
level tests in that order. Both loops still scan spell indices 0 through 69,
with one unchanged random draw between them and the original post-decrement
selection rule. No cached candidate list or new empty-pool behavior is added.
The definition's level is private owner state, preserving its stored type and
position and its constructor assignment.

Both names and ordinary owner-source bodies are project inferences. The new
visibility does not claim original RMG access control; native entry points and
virtual slot order are unchanged.

## RMG quest construction and placement fallback

The three seer-hut reward definitions share the artifact wrapper's ordinary
`createForSeerHut()` factory. Each caller still allocates its hut first; the
factory selects the dirt/artifact/subtype-zero prototype and constructs the
owning wrapper, then the caller assigns its creature, experience or gold reward.
The hut constructor remains the canonical source of all six default fields.
Reward assignment order, wrapper ownership and the absence of new failure
cleanup are preserved. The hut writer still selects experience, creatures,
then resources in its existing precedence order.

Failed key-tent and quest-artifact placement share generator-owned
`replaceObjectWithTreasure()`. It saves the old position, removes the object's
map registration without deleting it, then rereads the position's zone and
requests treasure in the original value-to-three-halves range. A non-null
replacement is added at that position. The key-tent caller retains its cached
generator/value reads; the quest caller evaluates its definition's virtual
value query first and keeps its treasure-group lifetime. Pending hut deletion
or transfer remains in the wrapper's existing writable operation/destructor.

Both helper names and source placement are project inferences from repeated
flows in the same TU. No native address, explicit inline qualifier, layout or
virtual slot is introduced.

## Adventure hover and recruitment setup

The active and waiting adventure hover handlers share `beginMapHover()` for
command invalidation, screen-cell caching and ordered packed map-point writes.
The active handler still skips unchanged screen cells; waiting hover still
refreshes every accepted call. Their off-map paths share
`processOutsideMapHover()`, retaining the short-circuit cursor-frame/scroll-zone
tests before forwarding the original pixel coordinates to the window. The three
rejected-path branches share `clearRejectedHoverPath()`: the existing canonical
path clear runs before a fresh mouse-manager lookup and pointer reset. Each
caller keeps its original return and guards.

Both map-selection handlers share `refreshHoverScreenCoordinates()` after
validating their copied point. The helper reads the current hover/origin fields,
as before. `forceNewHover()` retains its X-only invalidation and local-human
gate; command completion and scrolling retain their distinct cache writes.
Cached native types record public `advCommand`, `last_map_hover`, `lastHoverX`
and `lastHoverY` (Dreamcast `advManager` 0x1a68, field list 0x351c), so their
visibility stays public.

The army-group and hero recruitment constructors share `initializeNonTownSource()`
and `initializeCreatureChoices()`. The former sets the existing -1 source tag,
view-only flag and town-screen flag; the latter initializes the selected creature,
borrowed availability pointer, selected slot and all four choices/count pointers
in the original order. The distinct hero/group owner assignments stay between
these operations. Town recruitment keeps its dwelling and upgraded/base creature
rules. All three constructors share `prepareInitialCost()`, which writes timer
zero to the current time plus 100 before calling the existing `updateCost()`.
It does not initialize quantity, sprite frames or other untouched state.

Three unavailable/remote-player paths share `disablePurchaseButtons()`, preserving
accept-before-maximum ordering and fresh window reads across the virtual calls.
The update path still disables the slider afterward; initial accept-only disabling
and conditional enable predicates remain separate. Dreamcast recruitUnit field
list 0x5223 records the affected source flags, choice fields and owner pointers as
public. Layout, borrowed pointer ownership and virtual interfaces are unchanged.
All eight new private helper names and ordinary source bodies are project
inferences from repeated same-TU operations, not claimed original symbols.

## Remaining window registration copies

Forty-four additional full-vector registration loops now call the existing
`heroWindow::addWidgetsToMessageStream()` body. The pass covers adventure and
combat windows, hero/recruit/swap windows, campaign selection and briefing,
quest/puzzle/spellbook/university/sacrifice windows, main/game-type menus,
single-selection and options windows, dimension-door/scuttle-boat windows,
army splitting and all three army-view constructors, combat results, world/town
views, five trading windows, ten town-screen/building windows, high scores and
`TDialogBox::setup()`.

Every migrated loop registered non-null entries at priority -1 and reported
null entries with `memError()`. The existing helper preserves iteration order,
live end reads and the error path. Qualified base calls reuse that exact body
without adding virtual dispatch to the former pasted loops; the original
protected virtual interface and header definition remain unchanged. Empty-range
while/do-loop spellings in town/game/adventure menus share the same operation.
Named vector references in army view and campaign briefing still refer to
`m_widgets` and remain available for the surrounding appends. Registration stays
after vector construction and before each caller's later text, resource, video,
subwindow or control setup.

Quick-town's two constructors, quick-hero and level-up instead share ordinary
protected `addPresentWidgetsToMessageStream()`. Their loops deliberately skip
null entries without calling `memError()`. This new helper is project-inferred
and defined once in window.cpp. The existing subwindow helpers retain their
separate parent ownership, and local-vector append/registration, indexed
unconditional registration and dynamic widget insertion remain distinct.

## Archive search, trade arrows and puzzle object state

`LODFile::findLinear()` shares the identical short-range scan from both sides of
`find()`. It searches the original half-open range with case-insensitive name
comparison, stores the first matching index or -1, and keeps the existing
midpoint comparison, empty-range handling and recursive long-range branches.
The private match-index storage, stream cursor and decompression-buffer behavior
are unchanged.

`TSwapWindow::showTransferDirection()` shares the paired arrow visibility
operation between giving and receiving modes. Both nullable-arrow guards remain
in `updateArrows()`. The left arrow is processed first, then the current right
arrow pointer; the receive button is enabled or disabled afterward. The side
query still occurs inside the selected mode branch.

Both puzzle-tile constructors share `initializeObjectState(visible)`: object
type zero, the caller's visibility, then both -1 offsets. The default constructor
still leaves the grail flag untouched and initializes unknown terrain; the cell
constructor still reads terrain/river/road/diggability before computing grail
presence and any object offsets. Signed packed bitfields and default/cell
visibility values are preserved. Cached Dreamcast type 0x3fbd / field list
0x3fd9 records these fields as public, so they remain public. All three new
helper names and ordinary source bodies are project inferences.

## Dynamic widget ownership and thieves' guild rows

Nineteen original append/register sites now share public
`heroWindow::addOwnedWidget()`: quick-creature information (three), monster-join
and garrison additions (three), thieves' guild dynamic rows (nine), multiplayer
IP text, text-dialog setup and town building resource text/icons (two). It records
the pointer in the owned vector before passing the vector's last entry to the
existing `addWidget()` at priority -1.
Registration still opens the widget and links it through the native operation;
a rejected open still leaves ownership recorded, and no new null or allocation
failure policy is introduced. Allocations, retained widget handles, title-string
lifetimes, formatting and conditional insertion remain with the callers. Base
registration is unchanged. The operation is public because the town manager
adds resource widgets to its newly allocated purchase dialog; it preserves
caller-side allocation checks, text/icon locals, sprite disposal and dialog
ownership without exposing new stored state.

Combat control and placement bars share their own protected
`TSubWindow::addOwnedWidget()`. It appends even null entries but only registers
non-null widgets through the subwindow's native `addWidget()`, which adjusts
coordinates, expands the ID range and registers with the parent window. The
placement caller retains its outer null guard, so it still skips ownership of
null entries; the control caller still keeps them. Both local vectors retain
their original lifetime and traversal. Combat-family base destruction still
unlinks and deletes each non-null owned widget. This separate owner operation does not
append those widgets to the parent's owned vector.

The guild's four primary-skill cells share `addHeroPrimarySkill()`, preserving
skill order, the existing getter, formatting before construction and immediate
owned registration. Their y positions are the existing eleven-pixel progression
from 0x18c; the four caller-supplied widget-ID bands remain distinct.

The town and hero army passes share `considerStrongestCreature()` for each slot.
It requires a nonempty type and positive count, compares the existing AI value
strictly against the current best, then stores creature/value, copies the whole
army for later preview and saves the slot. The caller retains town-before-hero
traversal, signed indices, seven-slot loops and -1/zero initial best values.
Town candidates still resolve their army through the existing garrison-aware
`getArmy()` for each slot. Its body only selects the town or garrison-hero army;
no state change is moved across the selection. Equal values keep the first
candidate, and no candidate leaves the prior display snapshots untouched.

All four helper names and ordinary source placements are project inferences.
No stored fields, native access declarations or virtual slots change.

## Widget hit boxes, presentation and input transitions

Thirteen hit tests now share ordinary `widget::containsPoint()`: the base widget,
border, icon, text, text-entry, button, slider and hotspot handlers, plus the
window's reverse widget lookup. The half-open bounds retain their comparison order and
signed dimensions. Callers still subtract parent coordinates and narrow to
short where they did before; hotspot handling and window lookup retain int
coordinates, and window lookup retains its subsequent active/dimmed checks. Slider track interiors and drag tolerance have
different bounds and remain separate.

Ten full-widget screen updates share protected `updateScreenRegion()`. Six are
nested through `drawAndUpdate()`, used by button/slider selection and deselection,
text-entry focus and the native chat-edit update wrapper. Virtual drawing still
precedes fresh geometry and parent reads. Slider keyboard updates remain at their
original points around timer polling and state notification. Text editing's
update retains its earlier short coordinate snapshots and is deliberately not
folded into this fresh-coordinate operation.

Four slider notification sites share private `notifyStateChange()`: both
keyboard stages, mouse selection and deselection. A changed state is saved
before virtual `close()`; the callback, current state and parent are read again
afterward. Timer updates, event pumping, message conversion and modifier state
remain ordered in their callers.

Left-arrow and backspace share private `textEntryWidget::moveCursorLeft()`,
including the display-start correction. Both callers retain their nonzero guard,
and backspace still copies the text first. Rejected-character rollback only
changes the cursor and remains a distinct partial operation.

Cached Dreamcast widget type 0x170f / fields 0x182d records public short geometry;
types 0x4804 and 0x61cb agree. Text-entry type 0x1e69 / fields 0x3def records a
public unsigned-short cursor and signed-short display offset. Slider type
0x235b / fields 0x235c records public current state and state count, with protected
old state and callback. These native access distinctions are preserved. The five
new helper names and ordinary source placements are project inferences; stored
layout and virtual slots do not change.

## Widget command messages and slider stepping

Ninety-one three-field command setups now share ordinary
`message::setWidgetCommand(command, widgetId)`. Twelve are widget-side command
production: base send-message, border/icon/text/hotspot deselection, button and
slider selection/right-click, and text-entry keyboard/left/right handling.
Twenty-eight are recruitment broadcasts in `open()`, `update()` and
`quickViewRecruit()`. Fifty-one further setups cover hill-fort (16), town pages
(11), market setup/update (10), town gate (four), overview and castle (two each),
and quest log, hero swap, campaign briefing, scenario difficulty, army splitting
and window text setup (one each). Five market setups originally used literal
0x200 rather than the message-id enumerator. The operation writes message id,
command and widget id in that order. It retains modifier bits, mouse coordinates,
payload and window;
borrowed text assignments and dispatch stay in the callers, and reused messages
remain the same objects. The existing dialog-end methods and partial command
updates remain distinct. The native message fields remain public.

Four mouse-down branches share protected `widget::prepareMouseSelection()`.
Right clicks set the right-button modifier and command; left clicks mark the
widget selected before setting the command. It leaves the input id intact.
Border's virtual click hook remains after this preparation and before message
conversion, while icon's hook remains before preparation. Text and hotspot
handlers retain their own drawn/disabled policies. Icon release still converts
the message before its hook and later right-button test; this unusual existing
ordering has not been normalized.

Six original slider step-and-position pairs share private `stepState(direction)`:
the two keyboard arrows and both arrow ends for horizontal/vertical release.
Unlike native `setState()`, the operation leaves the saved notification state
unchanged and adds no clamp or one-state division fallback. Private
`stepFromArrow(click)` shares the complete release conditional for both axes,
retaining the strict far-arrow comparison, state guards and multiplication/division
order. Callers still choose their axis, clear selection, draw, set message and
modifier state, and notify in their existing order. Page selection, knob dragging,
resolution changes and initialization retain their distinct semantics.

All four new helper names and ordinary source placements are project inferences.
No stored layout, public/protected native data distinctions or virtual slots
change. Partial, reordered and conditionally assembled messages remain separate
operations; this section records the complete setups actually migrated.

## Message construction, market controls and campaign difficulty

Eighteen message declarations now rely on the existing native default constructor
instead of repeating all or part of its zero initialization immediately afterward.
The callers are hill-fort recalculation, base window drawing/text setup, three
single-selection methods, three army-strip drawing methods, widget send-message,
and eight town-page methods. The message objects stay in their original scopes;
no dispatch, callback or other intervening operation moves. Town setup's plain
`int objToLoad` declaration remains after the message declaration. The existing
constructor in message_record.h, paired to retail 0x589190 and Dreamcast 0x2d58,
still initializes all eight Windows fields. Resets after use and ring-buffer
partial input operations are not replaced with construction.

All fifteen native market status methods remain: on/off/disabled for resource
trade, gifts, artifact buying, artifact selling and creature selling. Their five
`setWidgetOn()` bodies share project-inferred `setTradeWidgetOn()`, which invokes
the existing window status helpers to set ACTIVE|DRAWN and then clear
DIMMED_NODRAW. Each dispatch still gets a separate locally constructed message,
and a first dispatch's result does not suppress the second. The five off methods
reuse `widgetClearStatus()` with all three bits; the five disabled methods reuse
`widgetSetStatus()` with DIMMED_NODRAW only. Neither operation changes the
separate WIDGET_DISABLED bit. Original short widget IDs, native method boundaries
and their callers are retained.

Campaign briefing's two native difficulty callbacks share the complete guarded
operation `changeCampaignDifficulty(message, change)`. Only deselection without
the right-button modifier changes the stored signed-char difficulty. The message's
window is captured before the change; button refresh precedes the full-window
draw and consume return. Other events return zero. The callbacks still supply
-1 and +1, and the existing button-visibility policy continues to control the
available directions; no new clamp is introduced. Both new helper names and
ordinary same-TU source placements are project inferences.

## Map-size display and radar refresh operations

Five map-size icon switches share `getMapSizeIconFrame(size)`: campaign scenario
selection, scenario information, both save/current-header branches of single-map
selection, and the size filter. Dimensions 36/72/108/144 select frames 0/1/2/3;
every other value selects frame 4, including the all-sizes filter. Each message
keeps its existing id/target/command assignment order, scope and broadcast. The
scenario-info caller keeps its local frame and direct icon setter. The helper's
ordinary body is in singleselectionwindow.cpp, which owns three callers; the
small map_display.h header declares it without importing the selection class.
The separate filter-button switch has no default selection and remains distinct.

Adventure radar selection and the world-view radar handler share
`getRadarInputScale(mapHeight)`. It preserves the literal 4.0f/2.0f/1.3333f/1.0f
mapping, including the default, and each caller still captures the value once
before its drag loop. Adventure radar drawing's large-map scale is 1.33f, not
1.3333f; rendering and row-stride switches retain their different constants and
sampling behavior. Input coordinates, conversion order, existing asymmetric
bounds and event-pump lifetimes stay in their callers. The ordinary input lookup
body belongs to advmgr.cpp and is declared in map_display.h.

Three origin-change paths share private `advManager::refreshRadarAndMap()`:
initial radar selection, subsequent drag movement, and screen scrolling. It
calls existing `updateRadar(1, 1, 0, 0, 0)`, `completeDraw(0)` and
`updateScreen(0, 0)` in order on the same receiver. Two visibility-message paths
and the reveal menu command share the separate free
`refreshAdventureRadarAndMap()`, preserving their fresh `g_advManager` read at
each of those three steps. Keeping these receiver policies separate avoids
silently caching the active manager across drawing calls. The broader native
`redrawAdvScreen()` paints other controls and calls radar drawing in another
order with different arguments; it is not a replacement for this operation.

All four helper names and ordinary source placements are project inferences
from repeated authored operations, not newly established original interfaces.
No native addresses or inline qualifiers are assigned to them. Existing native
drawing calls remain nested in the shared operations; no storage or virtual
layout changes. These thirteen migrated occurrences are a reviewed batch, not
a claim that the whole-codebase search is complete.

## Optional saved bytes, legacy hero IDs and player-choice cycles

Ten optional-byte conversions share file-static `decodeOptionalByte(value)` in
game.cpp. Only 0xff becomes -1; other values, including 0x80 through 0xfe, retain
their unsigned-byte values. The callers are the two native byte hero-ID readers,
three grail coordinates in each of the scenario/save victory readers, and the
portrait field in each custom-hero setup reader. The scenario grail reader still
uses its existing partially filled int buffer and explicit low-byte mask; the
save reader retains its unsigned-char temporary and individual stream calls.
Coordinates are stored in X/Y/Z order between reads. Existing unchecked reads
remain unchecked; no error handling or input consumption is added.

The custom-hero setup constructor and save writer identify that value as a
portrait, despite the former local name heroId. Those locals now say portrait;
the scenario reader keeps its byte temporary. Portraits receive no hero-roster
remapping. Both readers retain their existing name/mask temporary lifetimes,
record insertion, count loops and version-dependent availability handling.
Required victory/loss coordinates and raw placeholder IDs retain 0xff as before.
The short hero-ID reader is also excluded from byte-sentinel decoding: its 255
is a short value, while an already signed -1 remains -1.

The separate file-static `remapLegacyHeroId()` shares the two ordered legacy-ID
comparisons across `readHeroId()`, `loadHeroId()` and `loadHeroIdShort()`. Their
native boundaries and read types remain intact. The map reader gates on version
14; the save readers gate on versions below 25. The byte readers still return
for the absent value before applying their version gate. All other IDs pass
through, and only 0x80/0x81 map to 0x92/0x9c. The short reader keeps this same
remapping without inheriting the byte sentinel rule.

Private `CNetPlayerHandler::beginPlayerCycle(pos)` shares the ordered current
position, m_unused=-1 and saved-assignment=-1 transition between construction
and selection of a different position. Constructor player-count initialization
still precedes it, with color/name setup afterward. The same-position path still
restores its saved assignment before clearing only that assignment. The separate
external invalidation of m_playerPos also remains a partial mutation. The helper
does not reset players, their town/hero choices, or the player count.

These three names and ordinary source placements are project inferences from
reviewed repeated operations; they add no native annotations or inline claims.
The three legacy remap occurrences overlap two of the ten byte-decoding callers;
the two cycle resets are separate occurrences. Native field layout and visibility
remain unchanged. The no-team loops were also reviewed: both stream readers use
identity team assignments, while random-map setup interleaves them with slot
configuration. The shared reader conditional is now implemented in the next
section; random-map setup retains its interleaved assignments.

## Screen-effect calculations and team payloads

Five capture/fade paths share file-static `clipScreenEffectRect()` in winmgr.cpp:
ordinary and X fizzle-source capture, ordinary and X fizzle-forward, and flash.
The operation trims width for a negative X before clamping X to zero, then does
the same for Y/height, clips the high edges to 800x600, and reports whether both
extents are positive. Callers retain their complete-draw gate before clipping
and their existing early-return or positive-branch shape. Empty rectangles do
not allocate or release a saved bitmap or alter color-cycling state.

`updateScreen()` has a different low-edge policy: it clamps the origin without
subtracting from the extent. `fadeBlit()` additionally shifts source coordinates,
uses live bitmap dimensions, and retains its evidenced width/height asymmetry.
Neither is converted to the fixed screen-effect clipping helper. Fizzle source
replacement remains distinct from `releaseFizzleSource()`: replacement retains
its allocation/draw sequence without introducing an intermediate null store;
manager close still performs its final teardown without reusable-state resets.

Four pixel-loop bodies share file-static `blendScreenPixel()`: ordinary and X
fizzle-forward, and transparent/opaque fade-blit. It extracts blue, green and
red through the existing runtime masks, interpolates each with the signed-int
16.16 difference/product/shift expression, remasks and joins the components,
and narrows the result to unsigned short. Palette lookup remains in fade-blit;
its transparent path still skips source index zero before reading the saved
pixel. Pointer increments, row-pitch queries, buffer lifetimes, timer reads,
sound polling and screen updates remain in each native caller. Four frames
with a 10ms default and eight frames with a 33ms default remain distinct;
no floating-point blend, clamp, widened product or cached global mask is added.

Fade-to-black and fade-from-black also share their packed two-pixel calculation
through file-static `darkenScreenPixelPair()`. Their existing mask snapshots
remain before bitmap capture, and their source locals keep their respective
unsigned-int/unsigned-long types. The shared operation preserves the unsigned
shifts, unsigned-long channel intermediates, remasking and final combination.
Each caller still advances its source before the calculation and destination
after the store, steps rows through the native pitch accessors, and owns its
different shift loop, deadline/break handling and final image restoration.

Protected `CMapHeaderData::readTeamAssignments()` shares the complete team-payload
conditional in the scenario and save readers. Each caller first reads and stores
the team count, then invokes the operation and returns -1 on its failure. A
nonzero count consumes the same signed-byte array with the same short-read test,
including partial writes; zero consumes no payload and sets each of the eight
players to its own team. No new validation of team IDs or count is introduced.
Random-map construction still assigns teams between its per-slot changes rather
than moving those writes across the other initialization operations.

All four names and ordinary source placements are project inferences. The two
map-reader occurrences, five rectangle operations, four blends and two dimming
calculations are a further reviewed batch; original native entry points and
existing helper calls remain represented. No native addresses or inline claims
are attached to these extractions, and field visibility/layout is unchanged.

## Resource field markers and owned text initialization

`isResourceFieldSet()` supplies one ordinary textresource.cpp body for the
non-null spreadsheet-cell predicate: the leading character is neither NUL nor
a literal space. Eleven direct expressions now call it: four spell schools,
artifact slot eligibility, seer-hut names, three random-map connection fields,
placement-rule termination and template-row continuation. Tabs, numeric zero
text and other non-space characters retain their previous meaning. This is not
a whitespace trim or a textual boolean parser. Spell bit order, artifact bitset
proxy assignment, the seer-hut skip and the RMG break/continue boundaries stay
in their callers.

The existing `isRmgTemplateFieldSet()` remains the nullable wrapper and calls
the same predicate after its pointer check, preserving all eleven existing
wrapper call sites (including loops over town, terrain and monster fields).
The connection's second zone field still tests only for an empty string; it
has not acquired the first field's space rule. Row-width gates and lookup order
are unchanged. `getRow()` is the existing nonvirtual vector accessor; template
boundary scanning now passes its first cell once to the shared predicate.

Ten table-string initialization sites share `TAutoStrPtr::copyText()` within
their respective native owner classes: three spell strings, three creature
strings, and four hero/class/secondary-skill strings. Dreamcast declarations
and the existing source put separate private string classes in those three
translation units. Each keeps its own ordinary method body and type identity,
private pointer, native constructor/destructor, and native `set()`/`get()`
methods. The new operation allocates strlen+1, installs the buffer through set,
and copies through get. Trait-pointer publication remains afterward in the
caller. Static-array guards, destruction claims, row parsing and description
loops retain their positions. Like the original sequence, this initialization
does not delete a previous value or add a null-input policy.

Widget help replacement shares two private per-string operations at two sites
each. `releaseHelpText()` deletes a non-null string only under the existing
ownership flag, then detaches it; borrowed strings are merely detached.
`copyHelpText()` copies only a non-null source, installing the allocation before
strcpy. An empty but non-null source still allocates a terminator. The complete
setHelpText operation releases rollover before right-click text, changes its
ownership flag after both releases, and then copies or borrows in that same
order. The destructor keeps its different final-teardown order, one ownership
gate and absence of nulling stores. Initial zeroing remains the existing
initializeHelpText operation.

The predicate and added owner methods are project-inferred names and ordinary
source placements, with no native address or inline claims. Pooled artifact
strings still use copyResourceString into their existing owner; widget text
retains its separate conditional ownership. The three native table-owner types
have not been merged merely because their layouts and small methods agree.

## RMG scalar writers and tutorial option transitions

Eighty-five one-use scalar staging sequences in thirteen RMG object writers now
call the existing `writeValue<T>()` from abstractfile.h. The explicitly selected
staging types are 57 char, 21 int, six short and one unsigned int. This covers
the base object and monster, town, ownable, artifact, resource, black-box, seer
hut, hero, scholar, shrine, spell-scroll and witch-hut payloads. Each helper
still makes one native virtual write with a by-value scalar buffer; no endian
conversion, coalesced write or new result check is introduced. The base write
and existing position accessors remain nested in each relevant caller.

The version gates retain their original order and constants: object IDs and
expanded creature/artifact IDs appear only in their established formats, and
hero custom-experience, biography, sex and spell/primary-skill fields keep their
separate conditions. Seer rewards still prefer experience, then creatures,
then resources. Spell counts still narrow to char before their write, while
the element loop continues to query the live vector size. Reserved scalar
writes remain separate calls. Field roles formerly carried only by staging
variable names are retained as comments beside literal payloads.

Five int buffers deliberately passed to write with sizeof(short) remain explicit
rather than being relabeled as short source variables. Array payloads also stay
local: base/town/ownable/hero reserved regions, black-box resources and the town
spell mask. The town mask still uses the same buffer for its conditional and
unconditional writes; this preserves possible stream-visible mutation between
calls. Bare scopes used only by the removed one-use staging scalar are gone;
branch/loop scopes and the remaining buffer lifetimes are retained. The helper
is an existing inferred interface, not a newly claimed native template.

The tutorial's move-reminder and quick-combat options share private
`TSystemOptionsWindow::disableTutorialOption(id)`: send frame zero, set
DIMMED_NODRAW, then clear ACTIVE. Every step resolves the widget anew and uses
its native sendMessage operation. The two option IDs keep their original order,
and normal-mode preference frames remain outside the operation. This is a
complete three-message transition, distinct from raw status writes, ordinary
enable/disable, and the reused-message multiplayer load/restart handling.
The method name and ordinary source placement are project inferences; no native
annotation, virtual slot, field visibility or layout changes are introduced.

## Popup network polling, grace periods and hero overrides

Four popup consumers share `CNetMsgHandler::pollPopupAbort(msgReceived)`:
recruitment, normal dialogs, town management and the adventure-popup base.
The ordinary method invokes virtual checkHandleNet with inPopup=1 and the
caller's existing byte flag, then invokes virtual getAbortPopupMsg only when
that flag is nonzero. It preserves the same captured handler across both calls,
even if processing changes the globally installed handler. It does not reset
the byte itself: caller initialization remains in place, which also preserves
the behavior of the single-selection override that does not write the flag.
The native poll's return value remains ignored.

Each caller keeps its transport/remote guard and timer check. Recruitment still
sets its shared abort flag before its one exit operation; the other paths keep
their existing exit helpers and returns. Adventure-popup's missing-handler
path is now the explicit left side of the combined condition; its initialized
zero byte previously skipped the abort query through short-circuit evaluation.
The quick-view pump is distinct: it passes a null flag and queries an abort
without the received-message gate, so it does not use this operation.

Three guarded popup-state changes share
`CDPlayHeroes::setHandlerPopupState(value)`: hero-view return, Hut of Magi
processing, and adventure-popup destruction. It gets the current handler once,
checks it and calls native setInPopup. The caller-specific remote, transport and
local-player conditions remain outside; destruction still restores its saved
byte. Popup construction keeps its coupled snapshot-and-set operation on the
same captured handler. Handler replacement/copying and ownership are unchanged.

Normal-dialog timeout and network-abort branches share file-static
`deferNormalDialogExit()`. The first elapsed-time query decides whether the
15-second grace period remains; only that branch makes the second query and
stores the existing 15000-minus-elapsed value. It reports whether exit was
deferred, leaving native exitNormalDialog calls and their random/default answer
selection in the caller. No time query is cached and no deadline arithmetic is
changed; the outer zero-deadline guards retain their original order.

The two complete starting-hero override resets share
`clearStartingHeroOverrides()` in the array's owning game.cpp. The operation
keeps memset(-1) over all eight ints. Both setupOrigData and beginNewGame call
it, including the second reset after progress and map-name work in the launch
path. Per-player override selection, subsequent map loading and the array's
external declaration are unchanged.

These four names and ordinary source placements are project inferences across
eleven occurrences. No native annotation or virtual slot is added, and the
existing accessors/poll/exit helpers remain nested in the source call paths.
The network flag and other caller-owned locals retain their existing scopes;
backing field visibility and layout are unchanged.

## File positioning and lightning repaint operations

Four File virtual methods (seekBegin, seekEnd, seekCur and getPosition) share
private ordinary `seekFrom(distance, origin)`: return zero for a null handle,
otherwise call SetFilePointer with the original signed distance and origin.
The helper is nonvirtual, so getPosition does not acquire dispatch through
seekCur or seek. The two-stage seek remains distinct: it guards once, performs
its absolute step when required, ignores that result and performs the relative
step. getLength retains its existing virtual getPosition, seekEnd and seek calls.

File construction now calls the existing native header init operation. Open
reuses qualified `File::close()` before its existence/read-only handling, so a
derived override cannot intercept the original release-and-null transition.
Close still reports whether a handle was present, ignoring CloseHandle's return;
open still ignores that result. Final destruction only releases its handle and
does not perform the reusable nulling transition. No virtual slots, signatures,
field access or layout change. The new seek helper name and source placement are
project inferences; the reused init and close operations already existed.

Lightning drawing shares file-static `clipBoltCoordinate` across X and Y. The
operation uses the already-converted pixel coordinate for its two independent
bounds checks, updating both the pixel and fractional pen only when clipped.
Both original float-to-long conversions precede either clip, X precedes Y, and
the maxima remain 799 and 555. Unclipped fractional positions remain intact.

The animator shares file-static `includeBoltInUpdateBounds` across its two
four-comparison unions around drawBolt. Both calls read the current bolt fields;
the second remains after pen movement. The caller retains its distinct int
bottom and long left/top/right locals, sentinel initialization, thickness
expansion, screen clipping, timing and repaint call. Neither helper adds native
annotations or changes SBolt storage. Names and ordinary source placement are
project inferences from the repeated source operations.

The reviewed random-map team branches remain distinct: the computer branch's
upper-bound assignment uses humanPlayerCount, while the human branch uses its
own count. LOD buffer cleanup also retains distinct clear and pointAt state
changes, and memory-file reads clamp where writes grow. Similar-looking tests
alone do not establish interchangeable operations.

## Hidden-hero cursor state, campaign scalars and victory gates

Four hide/garrison paths share ordinary `advManager::clearHeroCursor()`, which
clears the cursor-overlay byte followed by the current-hero mobility byte.
The callers are hide-record replay, show-record undo, playerData's garrison
insertion and town hero exchange. They retain their original local-owner and
current-hero guards, roster/owner updates, cell operations and redraws. The
helper performs no calls between its two stores. Hero dismissal remains
separate because its cursor and mobility writes have different owner guards;
demobilizeCurrHero retains its intervening cursor, cell and facing work. Neither
the initialization paths nor single-field updates are broadened into this pair.
The name and ordinary source placement are project inferences; storage and
existing access declarations are unchanged.

CampaignScenarioInfo::write and SCampaign::save now reuse the existing by-value
writeValue operation for eighteen one-use scalar buffers: fourteen chars,
three ints and one short. The five-field score writer retains completion,
days, score, completion-order and index order. Campaign headers retain their
seven separate byte writes, string length before payload, completion array,
score records, carryover pools and assigned-hero IDs. The helper owns each
staged value, preserves its original narrowing and ignores the write result as
the caller did. Loop bounds still query live vector sizes; hero/artifact pool
references retain their scopes, and the existing artifact short writes remain.
The artifact-count int passed with a two-byte length stays explicit because its
source buffer type differs from its written width. No array, record or string
payload is coalesced with a scalar write. Historical matching comments are
explicitly labeled as preceding this broader reuse.

Eleven victory paths share file-static `canCheckCurrentPlayerVictory()`: require
a current player before checking the local slot's disabled flag. Each caller
retains the condition-type check ahead of this query; the defeated-hero path
also retains its loser-null check first. Campaign artifact exceptions,
defeat-all-monsters and time-survival paths keep their distinct gates.

Six paths share private `VictoryConditionStruct::allowsCurrentPlayerVictory()`:
query isHumanAlly first, then the computer-policy flag only if needed. Total
creatures/resources, town capture, flagged generators/mines and artifact
transport use it. The native appliesToPlayer checks the flag before querying
the team and therefore remains independent; artifact collection retains its
explicit getTeam, nonnegative-team guard and isHumanTeam calls. The two new
predicate names and ordinary source placements are project inferences. Native
condition entry points, winner recording, field visibility and layout remain
unchanged.

## Buffered input, mouse rectangles and selection repainting

The native getEvent and peekEvent wrappers share private ordinary
`inputManager::readBufferedEvent(message&, consume)`. Each still constructs its
own return message before polling sound. The common operation polls first,
checks active status and a nonempty queue, copies the current slot, updates the
head, then conditionally converts a key-down to ASCII. Consumption advances by
one modulo 64; peeking retains its original head modulo 64 store. Inactive or
empty queues retain the nested setNoInput operation and do not normalize the
head. Conversion acts on the caller-owned copy, leaving the buffered record
untouched. Both native signatures, return lifetimes and claims remain intact.

Mouse rendering shares three file-static operations in mousemgr.cpp. Both
branches obtain the primary surface description through the same zero, size
and GetSurfaceDesc sequence, retaining their separate caller-owned v1
DDSURFACEDESC objects and ignoring the SDK result as before. Three surface-space
rectangle clips share left/right/top/bottom checks against that description;
comparisons retain the signed-long dimension casts and stores retain the
original DWORD values. The fixed 800x600 initial clipping and translated client
clipping remain separate because their ordering, bounds and arithmetic differ.

Five save/copy/blit rectangle constructions share setLocalMouseRect: zero left
and top, then derive right and bottom from the bounds' extents. The rectangle
objects keep their original scopes, including saveAndDraw's subsequent copy
into sourceRect. Blit surfaces, offsets, flags, IsRectEmpty guards and the
retained saveAndDraw/restoreUnderlying boundaries remain unchanged. The two
update exits reached after incrementing m_busy share private finishUpdate,
which clears the global reentrancy byte before decrementing m_busy. Earlier
exits still clear only that byte, and the caller-owned critical-section lock
outlives every cleanup call.

Twenty-five selection-window draw/update sequences share private ordinary
redrawSelection. It invokes virtual drawWindow with the existing no-immediate-
update flag and full ID range, then the window's native update. Six of those
paths use an outer refreshChatAndSelection operation, retaining displayChat
before the nested repaint: five detail-popup branches and player-drop handling.
The filter-options branch also reuses the existing refreshFilterWidgets rather
than repeating its three operations; that native helper and openRandomMapOptions
now retain the common repaint as a nested call.

Host, popup, redraw and map-receive guards remain caller-owned. Modal object
lifetimes, widget changes, setup broadcasts, file-slider state and full-window
update timing retain their order. Paths that draw with update=1, draw partial
ID ranges or do intervening text work are distinct. New names and ordinary
source placements are project inferences; no native annotations, virtual slots,
field access declarations or storage layout are added or changed.

## Media archive ownership, town popups and overview replacement

Six archive-directory loads share file-static readArchiveHeaders in smackmgr:
three video directories and three sound directories. The operation reads the
four-byte count, allocates count+2 typed entries, publishes the pointer and
reads the directory. The existing record declarations prove 44-byte video and
48-byte sound entries; the size is cast to int before multiplying by the signed
count, preserving the original arithmetic. Handles, counts and published
pointer cells are references, so the second read retains current values after
the first API call and allocation. The caller still owns its DWORD read-count
local across archive loads. Neither read's result is newly checked.

Opening paths, required-archive error dialogs, optional-archive handling and
partial-load state remain in their callers. In particular, a failed optional
video archive becomes null while sound handles retain INVALID_HANDLE_VALUE.
Three video arrays reuse clearArchiveHeaders in their original 3,2,1 order.
Three sound archives reuse closeSoundArchive: close a non-invalid handle, store
INVALID_HANDLE_VALUE, then invoke that same guarded array delete/null operation.
Counts remain unchanged. Video shutdown's separate handle-close protocol is
not folded into the sound operation. These templates and ordinary source
placement are project inferences, with no native template claim.

Town mage-guild and construction-hall paths share private showHallPopup: draw
the current hall window, update the full screen, then run its modal loop. The
member window is read again for the modal call. Their resource-bar replacement
and two completion paths share private clearPopupBank, preserving deletion
before nulling. createPopupBank retains its existing nonnull guard. Mage-guild
completion restores the network handler's ordinary bar before deleting the
popup bar and hall window; construction-hall completion deletes the hall first,
then clears the popup bar, then restores the subscription. Neither helper
absorbs those intentionally different orders, allocation or town rebuilding.

Six overview replacement sites share clearOverviewWidget: skip empty slots,
call the existing removeWidget, delete the typed slot and null it. Five dynamic
tables and the fixed title array retain their original loop order and bounds.
The template binds the table rather than a cached widget or element reference;
removeWidget can dispatch virtual close, so deletion and nulling continue to
reread a dynamic table pointer after that call. The native removeAndDeleteWidget
ID-search method is not a replacement for this operation: its current recovered
body only unlinks matching widgets. Final overview teardown remains distinct,
with its original non-unlinking deletes and array release order.

All added names and source placements are project inferences. Existing native
entry points and nested helpers remain represented. Field visibility, layouts,
resource types and ownership policies are unchanged. No compiler or matching
claim is made for this continuation.

## Hero/boat locations, town initialization and fresh messages

Ten packed-point-to-coordinate copies share ordinary
`type_obscuring_object::setLocation(const type_point&)`. The operation reads X,
Y and Z in order and stores each into the existing short member; it does not
restore or obscure a map cell, change occupancy, update visibility or move a
roster entry. Callers are hero placement and setup initialization, prison
release, teleportation, remote recruitment, move-hero undo, and show-hero/boat
replay and undo. Their existing cell, facing, owner, boat-state, visibility and
redraw operations retain their relative order. The point is referenced without
introducing another packed-point construction or copy.

The cached Dreamcast type record positively declares mapX, mapY and mapZ as
public short members at offsets 0, 2 and 4 (field attributes 3). Those fields
therefore remain public despite the new operation. The setter name and ordinary
hero.cpp placement are project inferences, paired with the existing native
getLocation; it receives no native annotation and adds no virtual slot. Raw
stream reads, partial coordinate changes and unrelated packed path records
remain distinct from this complete hero/boat update.

Town-manager construction and opening share private initializeDialogPointers:
resource bar, dialog resource bar, auxiliary window, global tavern window,
active handler and saved handler are nulled in the original order. They also
share clearTownGraphicSlots, retaining the two whole-array memsets for monster
portraits and town-object pointers. The constructor's panorama write remains
between those operations, while open keeps them adjacent. These are pointer
initializations, not release operations: allocation, cleanup, counts, strips,
command sentinels, hover qualifiers and the other caller-specific fields stay
where they were. Names and ordinary source placement are project inferences.

Three fresh-message sites now rely on the native message constructor for their
unchanged zero fields: hero-stat refresh, overview mode changes and the window
manager's broadcast overload. The hero message also retains constructor-provided
codeY zero before its loop assigns widget IDs. Overview construction stays after
the slider branch, and its two broadcasts continue to reuse the same message.
Generic broadcasts retain the supplied message ID rather than acquiring a
widget-only envelope. Payload writes, loops and dispatch order are unchanged.
No reset is added to an already-used message.

## Packed path coordinates and movement-spell rules

Seven named-coordinate copies share ordinary
`type_point::copyCoordinatesFrom(const type_point&)`: four pathfinding callers
(lith exit, underground-gate exit, Town Portal destination and fresh search
seed) and three AI callers (Grail guess, destination evaluation and a flying
path's revised endpoint). The body retains X/Y/Z field assignments, rather
than replacing them with whole-object assignment that also copies packed
storage outside the named coordinates. The original point and path-cell
lifetimes, predecessor metadata, cost changes, trigger handling and pushPoint
calls stay in their callers. Existing whole-object copies and neighbor-offset
calculations are distinct. The name and findpath.cpp placement alongside
isValid are project inferences; no original spelling or inline qualifier is
claimed.

Path estimation and AI Town Portal execution share
`hero::getTownPortalMovementCost() const`: call the native getSpellLevel and
return 200 for exactly expert, otherwise 300. Estimation still queries it once
before iterating destinations. Execution still teleports, charges mana, queries
the movement cost, subtracts it and clamps movement to zero. The local cost
keeps the skill query ahead of the movement read. Town choice, spell
availability, mastery limits and mana cost remain separate. No extra backing
field or virtual slot is introduced.

Three movement-spell failure paths share file-static stopMovementAtPathStart:
only step zero clears the hero's movement points. checkMoveSpell uses it when
no stopping cell exists or the required movement exceeds the available amount;
those paths still return zero. The Dimension Door cast-limit branch in
attemptTeleport uses the same operation before its existing return-one result.
Later failed steps retain movement, and unconditional empty-path/exhaustion
handling remains separate. The long step type, guards and result meanings are
unchanged. These names and ordinary source placements are project inferences,
with no new native annotations or field-visibility changes.

## Resurrection eligibility, messages and effect overlays

Three corpse searches share private isCorpseFootprintFree: inspect the recorded
half of a double-wide corpse and reject its other hex if occupied or blocked.
The two part tests remain independent, with right before left and occupancy
before obstacle checks. Other part values still need no second-hex test. The
callers retain selected-hex validation, descending corpse order, owning-side
and living/undead rules, and their distinct spell-chance policies. No neighbor
bounds check or global-manager lookup is introduced. The existing getDeadArmy
is not substituted: it adds a negative-side guard and uses g_combatManager,
whereas these searches address their own manager's army array.

Four live-target eligibility tests share army::hasLostTroops: the current troop
count is strictly below the original count. Resurrection, Animate Dead and
Sacrifice retain their other conditions and short-circuit order. Excess-count
clamping, casualty arithmetic and hit-point reconstruction are different
operations and remain in their callers.

Demonic and ordinary resurrection share showResurrectionMessage, retaining
separate singular/plural sprintf branches followed by combatMessage. The raised
count remains long; the native creature-name helper still receives that count.
Quick-combat checks, corpse removal, stack creation, sound waits and animation
remain caller-owned in their original order. No combined format selection or
cached name is introduced.

Armageddon and mass-spell presentation share clearArmySpellOverlays, traversing
both sides and their current army counts to clear only m_showPowEffect.
Armageddon calls it unconditionally at its original cleanup point; mass-spell
presentation keeps it inside its graphical branch. Neither damage flags nor
the effected eligibility matrix are reset here. PowEffect's interleaved
per-stack overlay/damage reset remains distinct.

All four new helper names and ordinary source placements are project inferences.
No native identity, explicit inline qualifier or virtual boundary is invented;
existing fields, layout and native entry points are unchanged.

## Death cleanup, siege archers and obstacle selection

Armageddon and mass-spell presentation share processSpellDeaths: reset the
vanish queue, scan both army sides for affected stacks with exactly zero
remaining troops, process each death, redraw if any qualified, then consume
the vanish queue through its existing guarded helper. The supplied bool[2][20]
row is referenced throughout the scan, without a snapshot or changed extent.
Army counts are reread as before. Armageddon retains its damage announcement
before rebirth; mass-spell presentation retains its immediate rebirth call.
Their overlay reset and quick-combat guards remain outside this operation.

The nested processArmyDeath is also used by PowEffect: call native ProcessDeath
with zero, then query the siege-weapon flag and remove the corresponding hero
artifact when set. The hero is selected from the caller's side only after death
processing; no controlling-side substitution or early hero snapshot is added.
PowEffect retains its all-units-killed latch predicate, whereas spell cleanup
uses the supplied affected row plus troop count. Cheat-driven deaths and clone
recursion remain direct native calls because they do not remove an artifact.

Three siege-archer rows share initializeArcher, retaining creature type, sprite
load/handle assignment, shadow load/handle assignment, coordinates, facing zero,
sequence two and frame zero in that order. The raw-pointer staging record and
sprite-name lookup stay in initializeArchers across all three calls. The initial
whole-array memset, citadel gate and castle-only second/third rows are unchanged;
army-slot assignment and later resource teardown are separate operations.

Normal obstacle setup and placeAllObstacles share getObstacleTerrainMasks:
zero both masks, then select magic terrain when its index is not -1, otherwise
normal terrain. They also share pickObstacleForTerrain, consuming picker results
until a terrain-compatible ordinary obstacle or the original negative result
is reached. Normal-mask matching short-circuits special-mask matching. Both
caller-owned no-repeat pickers retain their lifetime and 0..90 domain. Budget
updates, random draws for large obstacles and placement failure policies stay
in the callers; large overlays use a different table/domain and remain separate.

These five names and ordinary source placements are project inferences. Native
entry points and nested helpers remain present; no layout, virtual slot or
field-visibility change is made. Historical compiler observations in source
comments do not validate these extractions.

## Target-delay discounts, hidden selection and moat rings

Both ballista target passes share file-static discountDelayedTargetValue.
The operation divides the candidate's signed long value by five when it cannot
attack, has no AI target or has target time above five; otherwise it divides by
a fresh native getAITargetTime call. The short-circuit order and repeated time
query remain intact, without caching or a new zero-time guard. Damage estimation,
the Mac/Windows arithmetic difference, loss valuation, kills-only retry gate,
shared candidate locals and last-wins tie handling remain in the two callers.

Garrison entry and town hero exchange share playerData::clearHiddenHeroSelection.
Only a matching selected hero is cleared to -1, and only the local owner's
matching hero clears the adventure cursor through its existing paired reset.
The hero is passed by reference so its owner is read after selection changes.
Roster removal, hide-message dispatch, cell restoration and town resident-slot
updates retain their original caller order. Hero dismissal has different owner
checks for its two cursor fields; replay has different selection policy. Those
paths retain their distinct operations.

The two moat rings share private findMoatHex: scan eleven unsigned-byte cells
in order, accepting a matching hex when the drawbridge is up or it is not that
ring's gate hex. Success conditionally writes the row output; failure leaves
it untouched. The native isInMoat entry point retains the moat-enabled guard,
ordinary-ring precedence, Fortress-only inner ring, and final -1 write after
all applicable searches fail. No coordinate validation or altered gate rule is
introduced.

All three helper names and ordinary source placements are project inferences.
Existing native calls, visibility, member layouts and return interfaces remain
unchanged. Historical compiler observations predate this extraction; no new
compiler or behavior validation is claimed.

## Map masks, timed-event flags and loss-condition teams

Twelve existing-mask conversions share decodeMapBits: both town spell masks,
map hero spells, custom hero setup spells, and four object-cell masks in each
of the map and saved-object readers. It retains signed int division/modulo,
ascending bit order and direct mutable bitset proxies, writing exactly 70 or
48 bits. The original unsigned decodePackedBits has separate native evidence
and remains unchanged. No returned mask temporary or extra stream read is
introduced. Each caller retains its packed-buffer lifetime, checked or unchecked
read, format gates, early failures and resource-file versus map-file source.
The town's other x-controlled loops remain separate.

Four saved-object conversions share file-static encodeObjectCellMask: zero the
same six-byte buffer, then accumulate the 48 bitset entries using signed indices
and direct mutable proxies. Each existing write and short-write guard remains
in the caller. The separate RMG encoder uses an unsigned traversal and const
test() API; those source operations are not substituted here. Object mask order
remains draw, passability, shadow, trigger in all three serializers.

The map and saved timed-event readers share private readHumanApplicability.
They pass their respective >=28 and >=42 version decisions. The helper reads
and normalizes one signed byte when present, otherwise assigns one without
consuming input. It adds no read-status guard and leaves all other record fields,
first-day adjustment, padding and failure handling in their original readers.

Hero-loss and town-loss validation share private game::countHumanTeams. It calls
the native isHumanTeam for team identifiers zero through seven and counts each
successful team once. Hero validation still counts only after finding the hero
at the configured location; town validation still counts after resolving the
town. No cached global count or player-count substitute is introduced, and the
callers retain their different owner checks and condition invalidations.

Names and placements are project inferences. The signed decoder is an ordinary
header template shared by two TUs; the other three bodies stay in their owner
sources, with no invented inline qualifier or native identity. No serialized
width, record layout, virtual interface or field visibility changes.

## Map sprite replacement, object bounds and cell identity

Both map-object loaders share private copySpriteReferences and
reloadObjectSprites. The first retains resize plus indexed raw-reference copy;
the second resizes the destination, loads sprites in object-type order, makes
the two original progress updates, then disposes old sprites through the native
ResourceManager wrapper and clears the temporary vector. The old-reference
vector remains caller-owned through all subsequent object reads and failures.
Map loading still passes its cached numObjects; save loading evaluates the
current type-vector size after the reference copy. The loops keep current
bounds and progress thresholds, including two updates at the same index when
small lists make those thresholds equal. No null skipping, move/swap or new
failure cleanup is introduced. Destructor disposal remains distinct.

stampObject shares getObjectBounds for the new and existing objects. Left/top
subtract the current type's dimensions and add one; right/bottom add one to the
object coordinates. Coordinates and dimensions are read in their original
order. Both caller-owned rectangles, intersection, map-edge clipping and layer
comparison remain in place; no bounds normalization or cached type is added.

Three non-trigger classification branches share NewmapCell::setObjectIdentity:
store the unsigned object index into the existing short field, assign type and
narrowed subtype, then optionally copy extra information. The two blocking
branches share the outer setBlockingObject, which nests that identity operation
before blockMovement. Terrain-hole classification uses only the identity
operation. The trigger branch retains its different order, inserting the trigger
flag between type and subtype writes. Reverse scan order and early returns are
unchanged.

blockMovement also serves rock loading: clear passability, then set the packed
blocked bit. The rock test remains after the original permissive initialization;
cell recalculation's separate rock-only blocked-bit write is a partial update
and remains distinct. Existing packed fields, neighboring bits and record
layouts are unchanged.

All six helper names and ordinary mapcell.cpp placements are project inferences,
without new native identities or inline declarations. Existing native lookup,
resource and bit-position helpers remain on the call paths. No current compiler
or behavioral validation is claimed.

## Validation provenance

Per the user's instruction, this continuation and the PR split ran no builds,
tests, validation checks or matching-score investigations. Earlier compiler
observations recorded with the accessor inventory do not validate these changes.
The extraction checkpoint `aa71fa920` preserved the published C++ implementation
at `582687638`. The subsequent shared-operation continuations above have not
been compiled or measured.
