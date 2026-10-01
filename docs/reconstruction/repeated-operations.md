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
The two COM queries, HRESULT transitions, size-plus-one allocation and local
`CDPlayMsg` destruction remain in each caller. These helper names and ordinary
TU-local/source placement are project inferences, without native address claims
or invented inline qualifiers.

The nearby data/session/address readers require a separate review: they differ
in accepted first-query results, zero-size handling and when output sizes become
visible. Similar allocation tails alone do not justify sharing their protocols.

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

## Validation provenance

Per the user's instruction, this continuation and the PR split ran no builds,
tests, validation checks or matching-score investigations. Earlier compiler
observations recorded with the accessor inventory do not validate these changes.
The extraction checkpoint `aa71fa920` preserved the published C++ implementation
at `582687638`. The subsequent shared-operation continuations above have not
been compiled or measured.
