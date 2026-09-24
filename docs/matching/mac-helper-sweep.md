# Mac retained-helper sweep

The current pass restores source helper boundaries and calls across unfinished
Windows game functions. A lower byte score does not reject a call supported by
Mac, Dreamcast, and VC6 evidence. Byte polishing follows the broad pass.

Run `homm3 mac helper-queue` after a Windows checkpoint. It writes
`build/mac/helper-queue.json`, `helper-queue-functions.tsv`, and
`helper-queue-calls.tsv`. The function file includes every unfinished Windows
target and marks whether it has a reviewed Mac span. The call file lists every
direct Mac branch in those spans. `review_missing_helper_call` means a reviewed
source helper is absent from the caller's authored body; `identify_target`
means the destination has no reviewed identity. Other named targets get
`review_other_named_call`, since constructors and destructors can be implicit.
These are leads for human review, not proof of an
inline qualifier or even a game helper. A textual `source_call_present` check
also needs inspection when overloads or macros are involved. Reviewed Mac
runtime destinations are marked `runtime_call` and remain in the full report
without crowding the displayed helper leads.

Current work is split by caller TU:

| Owner | Units |
| --- | --- |
| recruit | game, townmgr, recruit, tradpost, creature_bank; then objecttype, puzzlewindow, font, drawing, overview, resourcemanager, questlogwindow |
| customcampaign | advmgr, kb, event_record, customcampaign, campaignbrief, mousemgr, advspells, window, button, strip; then singleselectionwindow, viewwrld, viewarmywindow, multiplayerwindow, adventuremapwindow, bottomviewsubwindow |
| ai_player | hero, mapcell, seerhut, philai, ai_player, search, findpath, armygrp, ai |
| primary | all other non-deferred game units |

Use `--owner recruit=game,townmgr,recruit,tradpost,creature_bank` and repeat
`--owner` for the other workers when owner labels are useful in the generated
files. Unassigned rows remain visible. The `rmg`, `zlib-1.1.3`, `codec`, and
`victor` modules are deferred by user direction.

For each caller, inspect Mac disassembly and direct targets, Dreamcast source
calls and lines where available, and the Windows expansion. Restore one
canonical helper body and source call in its plausible owning TU/header. Check
Mac body order separately from cross-TU VC6 expansion when deciding header
placement. Record unresolved destinations in the queue instead of silently
counting them as absent helpers. Run a focused build to ensure the recovered
source compiles; the full build and tests wait for integration.
