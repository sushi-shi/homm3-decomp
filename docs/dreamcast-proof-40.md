# Dreamcast source-shape proof: 40 non-exact retail functions

*Run 2026-08-28 against English GOG Complete 4.0.*

This is the first bounded test of whether `homm3 dreamcast` produces useful
matching evidence rather than merely nicer disassembly. The cohort was locked
before reconstruction: the 40 highest-scoring non-exact retail functions that
had an unambiguous source claim, a real (non-placeholder) Dreamcast body, and
an x86 candidate no larger than 500 bytes.

Every row received both passes:

```sh
homm3 dreamcast show <retail-rva>
homm3 sema diff <retail-rva> --source
```

`show` supplied the Dreamcast signature, parameter/local inventory, lexical
blocks, line-program statement groups, calls and branches. The retail diff
remained the verdict. Source hypotheses were kept only when the VC6 SP3 build
improved or matched retail; byte-flat or worse experiments were reverted.

## Result

The tree moved from **2526/3093** exact functions to **2531/3093**. Four of
the locked 40 became exact, one related function outside the size-limited
cohort became exact through the same constructor correction, and one cohort
function rose from **97.5333% to 99.8667%**. Overall fuzzy coverage remains
92.93%; these are small, high-scoring functions, so the exact-function count
is the more meaningful measure.

| # | retail target | before | outcome and Dreamcast evidence |
|---:|---|---:|---|
| 1 | `philai::AI_set_hero_bonuses` `0x527760` | 99.9870 | No source delta: only PDB/relocation names; branch bytes already agree. |
| 2 | `CSingleSelPopup::handle_message` `0x575430` | 99.9636 | **Exact.** Retail required codeY-before-codeX in both symmetric arms; keeping both arms alike also preserves VC6 tail merging. DC scoped the two statement groups. |
| 3 | `type_AI_player::mark_towns` `0x4282b0` | 99.9554 | Delinker symbol-plus-addend presentation, not a literal/source difference. |
| 4 | `type_AI_player::value_of_hall` `0x42b8b0` | 99.9524 | Same symbol-plus-addend class; raw retail operand is already accounted for. |
| 5 | `philai::buy_siege_engine` `0x525ca0` | 99.9362 | Named temporary/scope probes were byte-flat; remaining stack/register selection. |
| 6 | `army::do_multi_head_attack` `0x440310` | 99.9351 | Semantically identical effective-address/SIB commutation. |
| 7 | `philai::value_of_experience` `0x527710` | 99.9310 | Relocation-name difference (`gHeroGoldCost`), not source-reachable here. |
| 8 | `town::GiveSpells` `0x5be030` | 99.9216 | DC exposes accessor calls retail inlined; current residual is relocation/load scheduling, not a missing accessor boundary. |
| 9 | `type_AI_spellcaster::get_curse_value` `0x43b370` | 99.8731 | **Exact.** DC names three ratio locals: `old_average`, `new_average`, `decrease`. Restoring those lifetimes reproduces retail frame-slot colouring. |
| 10 | `soundManager::MemorySample` `0x59a210` | 99.7840 | Instruction graph agrees; scratch-register homing only. |
| 11 | `town::initialize_hordes` `0x5bdf60` | 99.7778 | AX/DX value homing; prior source-order sweep already exhausted. |
| 12 | `game::randomize_university` `0x4c06f0` | 99.7464 | EBX/EDI availability and register choice, not statement structure. |
| 13 | `CDiffFile::Apply` `0x490f60` | 99.6429 | Commutative SIB index choice. |
| 14 | `OnGameTransmitInitMsg` `0x589b20` | 99.6237 | Executable operations agree; object-name/register presentation remains. |
| 15 | `hero::get_primary_skill_total` `0x4e5960` | 99.5833 | One semantically identical SIB index-order byte. |
| 16 | `TTimedEvent::Read` `0x4fc1a0` | 99.4737 | DC confirms constructor/destructor boundary; remaining `_Tidy(true)` versus destructor relocation pinning. |
| 17 | puzzle `type_AI_puzzle_tile` ctor `0x52c770` | 99.4444 | Retail chooses eager field load versus address-plus-memory xor; DC's older accessor spelling does not close it. |
| 18 | `mouseManager::LoadFrame` `0x50d8b0` | 99.3103 | DC RAII boundary agrees; colour calculation differs only in register scheduling. |
| 19 | `type_AI_spellcaster::get_simple_attack_effect` `0x435b90` | 99.1053 | `this`/first-local register swap. |
| 20 | `type_AI_combat::choose_defense_hex` `0x4205d0` | 99.0992 | Register homing only. |
| 21 | `playerData::add_garrison_hero` `0x4b9fc0` | 98.9923 | DC confirms `CMCHideHero` RAII scope; residual is EH-state write scheduling. |
| 22 | `OnPlayerDropUpdateMsg` `0x5565e0` | 98.7064 | EH/string addends plus register scheduling; no missing source block. |
| 23 | `game::record_hide_hero` `0x49c720` | 98.7027 | **Exact.** DC preserves the helper constructor; retail's inline stores prove `prev_owner` precedes `new_owner`. |
| 24 | `hero::IsInIdentifyRange` `0x4e5e10` | 98.6813 | One independent load-order choice. |
| 25 | `game::CreateTownHeroes` `0x4ca040` | 98.6076 | Register reload order. |
| 26 | `combatManager::DoCompAI` `0x4221f0` | 98.4697 | Target has a redundant zero-extension; source spellings were already exhausted. |
| 27 | `game::match_underground_gates` `0x4c0b60` | 98.3451 | Equivalent spill-slot/register assignment. |
| 28 | `TCampaignBrief::~TCampaignBrief` `0x45afb0` | 98.2083 | DC destructor is line-complete; local-lifetime probe was byte-flat. Residual is EH addend/delete-receiver scheduling. |
| 29 | `combatManager::automate_first_aid_tent` `0x473ea0` | 98.1203 | Address construction register choice. |
| 30 | `hero::GetExperienceIncrement` `0x4da420` | 98.1013 | Inlined-copy register swap. |
| 31 | `TBuyBuildWindow::WindowHandler` `0x5d6810` | 98.0247 | Retail materializes the message id differently; six source spellings already exhausted. |
| 32 | `recruitUnit` armyGroup ctor `0x551350` | 97.9750 | Large initializer's store/register scheduling; statement orders already swept. |
| 33 | `philai::GetTurnAIVars` `0x527960` | 97.8723 | One x87 `fld` timing choice. |
| 34 | `type_school_artifact::get_value` `0x432890` | 97.7949 | **Negative control.** DC uses `std::min` in one arm; importing it regressed retail to 97.7308, so the Complete revision intentionally differs. Reverted. |
| 35 | `philai::wants_skill` `0x524dd0` | 97.7472 | **Exact.** DC's descending statement group plus retail control flow selects `for (i = 28; i-- > 0;)`. |
| 36 | `ComputeTradeRatios` `0x5ece80` | 97.5961 | Inverted-arm scratch-register rotation. |
| 37 | `type_AI_creature_purchaser::do_purchase` `0x42d690` | 97.5333 | **99.8667.** DC proves `do_best_purchase` takes `unsigned char`, removing caller bool normalization. One adjacent reload transposition remains. |
| 38 | `TMageGuildWindow::WindowHandler` `0x5ce370` | 97.3926 | EH addend, push order and one SIB commutation. |
| 39 | `type_AI_creature_purchaser::get_purchase_value` `0x42d780` | 97.3256 | Inlined consolidation-walk register rotation. |
| 40 | `type_AI_combat::do_aftermath` `0x426ee0` | 97.3171 | Two scheduling choices around the volatile surrender flag; DC's older local layout cannot be ported directly. |

The constructor correction in row 23 also made
`game::record_show_hero` (`0x49cb20`) exact, from 98.2970%. It was excluded
from the locked cohort only because its x86 candidate is 550 bytes.

## What this validates

The positive results exercise four independent kinds of recovered metadata:

- a helper boundary and shared constructor statement order
  (`record_hide_hero`, plus the `record_show_hero` spillover);
- original local inventory and lifetimes (`get_curse_value`);
- loop statement/control grouping (`wants_skill`);
- parameter type across a call boundary (`do_purchase` through
  `do_best_purchase`).

`CSingleSelPopup::handle_message` is a retail-led closure: the Dreamcast
statement/scope view narrowed the symmetric arms, while retail bytes—not the
cross-architecture instruction order—proved the equal-value store order.

The non-closures are useful too. Most of this deliberately difficult cohort
was already above 97%; 35 rows reduce to register allocation, instruction
scheduling, relocation naming or version skew after the source-shape pass.
That is a stop signal against unproductive semantic rewrites. Row 34 proves
the essential guardrail: when older Dreamcast source shape conflicts with
retail Complete bytes, retail wins.

The practical policy is therefore to use Dreamcast as a second structural
target, not a second byte target: query signatures, locals, scopes and
per-statement call/branch groups first; formulate a VC6 source hypothesis;
then keep it only if the retail x86 match ratchets.
