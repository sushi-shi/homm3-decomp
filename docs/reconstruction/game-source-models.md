# Game helper source-model evidence

These are source-recovery findings and rejected alternatives, not current scores
or runnable search instructions. Some referenced experiments have been retired;
Git history preserves their source. Start new work with the
[source-family runner](../vc6/source-families.md) and current retail comparisons.

### Ordinary movement and native serialization boundaries

The union/pragma cleanup uses the same driver outside RMG. Its historical
`generate-shipyard-boundary-family.py`, `generate-shipyard-scope-family.py` and
`generate-movehero-helper-family.py` populations restore four ordinary static
helpers, preserving their actual calls and early-exit scopes while deleting
seven existing fences. The native `generate-game-vector-helper-family.py` and
`generate-game-vector-return-family.py` populations jointly recover typed
load/save templates, delete a pointer union and six more fences, and retain
both exact writer bodies. The
[audit](union-pragma-audit.md#ordinary-shipyard-and-movement-helpers) records
the frozen contexts, source/candidate counts, rejected higher-scoring false
declarations, and caller/whole-object controls. These generators require their
pre-adoption source snapshot; stale source anchors must not be relaxed merely
to rerun historical numbers.

`PYTHONPATH=scripts python -m unittest homm3.vc6.test_game_vector_io` extracts
the adopted serializer templates into the native stream-contract fixture.
It covers resize/zero-fill, short I/O, payload strides and narrow count
boundaries with six rejected negative controls. The associated narrow
source-owned vector-instance label join is tested by
`homm3.retail_labels.test_vector_helper_signatures`; it must reject an equal-size
ICF twin when the requested native element/signature is missing or ambiguous.

### University record ownership and header collateral

`generate-university-insertion-family.py` exhausts 36 meaningful API, receiver,
record and fence choices (24 object identities) in context
`9d5e8e882a44a0242bf7`. `generate-university-initializer-family.py` then compares
the generic aggregate plus explicit Conflux initializer with the old default
constructor: 13 successful states, 13 objects, context `99ba0ea5c2a6db673a5a`.
Both families reproduce ten retained candidates. Their exact anchors require
the pre-adoption `662ecc31` snapshot. The selected pointer/count-insert candidate
`75bbad764fd068a3fcc87499` removes the pointer union without altering the
randomizer's bytes or the Conflux initializer/caller bytes, and improves Load.
The [audit](union-pragma-audit.md#generic-university-records-and-conflux-initialization)
separates proven generic-record ownership from the provisional initializer name
and original helper kind. Scores alone do not establish those source facts.

`generate-university-header-control.py` is a post-adoption two-state negative
control: restore only the old constructor declaration in unrelated consumers.
It isolates the two small whole-build score movements in army and
singleselectionwindow: context `ea72eb19220d16fbe772` scores both states,
produces two distinct objects, and reproduces both. Restoring only the old
header declaration recovers exactly 99.9424% and 100%; every other tracked row
in those two TUs is unchanged. The initial seven-unit context `4b5443e33ddcd7da20ab`
was rejected at opposite-corner reproduction: executable section bytes and
scores reproduce, but five table/data units vary anonymous header identities
or symbol placement. It is not counted as a successful search. The driver
rejected those variations in that context. Header path/nonce identity is now
handled as described above; symbol-placement or code differences still fail
reproduction. That historical failed context remains excluded from the totals.

The shared native stream fixture imports the real record and initializer,
also checking untouched generic default-initialization bytes, the four Conflux
schools and the returned record address. Three new semantic negative controls
reject automatic defaults, a wrong school and a wrong return pointer. They
complement, rather than substitute for, the five-TU raw COFF controls and full
retail build.

### Native marketplace artifact ownership

`generate-market-artifact-owner-family.py` is a finite two-state ownership
control against `2359d5a4`: context `d7a243b77c8e39872831`, two scored states,
two object identities, both retained candidates reproduced. The selected
`372591115abd7d96fb098eeb` changes the entry, header, dispatcher, state and
every artifact reader/writer together. DC `DoBlackMarket` (0x1886d4) proves
the `TArtifact*` parameter; the recovered game and black-market fields already
have that element type. Retail's entry stores the pointer unchanged.

The pre-delink old entry label scores zero for the corrected signature. This
is not treated as body identity: `compare-coff-layout.py`, with the explicit
old/new function-name pair, independently proves identical section layout and
bytes, function positions, and relocation sites/kinds/destinations. It passes
for tradpost (150 sections / 4059 relocations), events (304 / 3873), philai
(229 / 1442), ai_player (420 / 1963), and townmgr (402 / 6828). The last three
need no rename. Selected-candidate to production comparisons pass without any
rename. Normal source-owned delinking migrates the entry at 100%; no current
score changes, MAX resets, or historical peaks lost. Full gates pass at
4063/4752 exact, 96.39% linked and 96.13% whole-image.

`homm3.vc6.test_market_artifact_owner` imports the actual entry, dispatcher arm,
enum, record, array member and global declarations into a native-only semantic
fixture. It checks identity and in-place mutation of all seven slots in each
owner, hero forwarding, modal state and AI dispatch. Eight negative controls
reject wrong pointers, records, state, branch choice and missing modal calls.
The host fixture does not claim x86 ABI or codegen proof.

### Marketplace ratio accessor and setup boundaries

`generate-market-ratio-boundary-family.py` exhausts the twelve supported
accessor, resource-value lifetime and selection-call combinations against the
native artifact-pointer checkpoint. Context `ddb391c2a56bfe8804a0` scores all
twelve states, produces six object identities, and reproduces all six elites.
DC rows 2235/2237 prove the two artifact getters; rows 2961/2990 reach the
same `SetupNewTrade` as the arrow arms. Restoring those source calls is not
conditional on a higher fuzzy score.

The selected `c55c83a118bd50486be2a0a8` keeps one ordinary `setupNewTrade`,
uses it at all four sites, and removes tradpost's final depth fence. Of 67
raw emitted functions, only `windowHandler` changes bytes; all other tracked
scores stay fixed, including the retained ratio helper at 100%. Production
reproduces the selected object's entire 150 sections and 4061 relocation
destinations. The handler falls from 90.6792% to 81.9322%, with the old peak
retained in HIST. The direct unfenced/flattened control gives 80.5105%; the
explicit right-value local and getter flattening do not recover the lost call.

The difference is measured at the named site, not inferred from a call census:
retail function +0x509 calls `computeTradeRatios` in resource selection, while
the natural candidate expands it and shares a selection tail. A byte-identity
gated C2 trace has caller cost 872 / initial budget 1744; its first two setup
copies admit the cost-153 ratio helper against nested budgets 189 and 188.
This is remaining caller-budget/source-state debt, not evidence against the
canonical helper boundary. No manufactured assertion replaces the fence.

The native market test also imports the actual setup body and checks selected
artifact/resource arguments, output-pointer identities, and the order of the
ratio call and amount reset. Five rejected semantic controls cover wrong
arguments, outputs, amount, missing call and premature reset. The full retail
build passes at 4063/4752 exact, 96.39% linked and 96.12% whole-image, with
one observed MAX reset and no banked RVA lost. The new census is 227 inline
overrides in 33 TUs, and 65 unions. The retained local peak is a recovery lead,
not grounds to reintroduce the retired compiler intervention.

`generate-count-markets-boundary-family.py` separately exhausts four
declaration/building-query choices in context `d39faa6bb2f147e6f44f`, producing
two symbol identities and reproducing both elites. DC row 621 calls
`GetTown` followed by `HasBuilding(14, true)`; all three retail entry points
expand this active-market test. Selected `5a8fa95a228c773b30041a02` restores
that query and uses an ordinary static helper at its original source position.
The baseline and selected raw COFF layouts, bytes and all 4061 relocation
destinations are identical; all three entries remain exact. The explicit
inline keyword was unnecessary, not evidence of an original annotation.

### Full-width object-type input ownership

The object-type input experiment started at `ba9d07f2` and exhausts
eight owner/query alternatives in context `f19ddfdcac44d6eb1e45`: eight scored
states, three emitted identities, all three elites reproduced. It preserves
the existing integer buffer's filename-length and extra-field uses, crossing
the old union and three native enum lifetimes with a buffer/record diagnostic
query. DC rows 3610..3614 positively use `int_buffer`; the enum local is a
Complete I/O-boundary hypothesis, not a purported recovered DC declaration.
Row 3619 does positively query the committed record for the diagnostic.

Selected `3edff3daea1caee731a5ae58` reads into an enum local adjacent to its
guard, then assigns the record, and uses that committed field in the diagnostic.
Both unscoped lifetimes produce the same 99.9633% function. Merely changing
the union control's diagnostic is byte-neutral and stays exact. The scoped
native value gives 99.9163%. All other mapcell scores stay fixed.

The `--phase-scopes` follow-up tests actual independent field-value lifetimes,
not unused compiler mass: filename length, native object type and extra field,
each optionally scoped through its commit. Context `e8265cfda7e5bc50aa1e`
exhausts nine states (including the unchanged union parent), nine object
identities and nine reproduced elites. The best native phase variant is
99.9673%, but requires splitting the positively evidenced generic integer
buffer and adding scopes. That tiny improvement does not justify choosing
the more speculative source model over the simpler native parent.

The adopted object reproduces the selected candidate's complete sections.
Against the old object, all 419 section layouts, 318 function-symbol positions
and 2077 relocation sites/kinds/destinations agree. Only `readObjectType`'s
1216-byte body changes: 18 stack-slot displacement bytes, with the same
instructions otherwise. All other 259 emitted function bodies are unchanged.
This is an explicit raw comparison, not a relaxation of retail normalization
or the strict byte-neutral comparison tool.

`homm3.vc6.test_object_type_read_owner` compiles the actual read/commit block,
enum and abstract-file interface at native `-O0` and `-O2`. Six valid enum
values and read reports 0..4 verify width, return value, one read call,
neighboring sentinels and commit timing. Five negative controls reject direct
field reads, a two-byte width, rejection of a full read, a wrong value and a
missing assignment. No malformed-input or x86 ABI claim is made by this host
fixture. The neighboring 16-bit readers cannot use the four-byte enum's
`sizeof` without changing their actual wire contract.

The full retail build passes at 4062/4752 exact, 96.39% linked fuzzy and
96.12% whole-image. Only this function resets MAX (100% to 99.9633%); HIST
retains 100%. The census becomes 227 inline overrides and 64 unions.

### Ordinary volume conversion and selected-setting lifetimes

`generate-volume-boundary-family.py` starts from `a7a83c6e` and exhausts
twelve combinations in context `c682cc6559d87a47dbef`: twelve scored states,
five object identities and five reproduced elites. Three actual setting
bindings (value snapshot, const reference, direct reads) cross equivalent
short-circuit/nested bounds and the old/removed auto-inline fence. The source
keeps both duplicated scale arms and the shared lower/upper clamp; Complete's
0..127 return must not acquire Dreamcast's final 0..100 platform conversion.

Selected `7bb1334725faaa2026c1258b` uses branch-local const references and
removes the fence. Strict `compare-coff-layout.py` passes for all 110 raw
sections, 699 relocation destinations and function positions against the old
`c441b9aa0f03520c7206d8aa` control. Production passes the same comparison.
Every scored soundmgr function remains unchanged, including all four exact
callers. These references are source-lifetime hypotheses, not declarations
proved by DC's empty optimized-local inventory.

The unfenced value control `338a47b3bd2bded6fbade5ae` retains the same helper
body but removes the named calls at setMusicVolume +0x1f, modifySample +0x9a,
memorySample +0x15a and processStopAndPlayMP3 +0x25. The four bodies grow,
and their scores drop to 0%, 57.3125%, 81.0864%, and 83.8794%. Direct reads
also expand the helper, with either guard form. Nested value/reference forms
keep all four calls, but only the short-circuit reference form preserves the
original whole object without changing the guards. This is a natural binding
decision, not a new pragma, helper copy or release-elided dummy operation.

`homm3.vc6.test_volume_boundary` imports the actual body and enum, checking
15 music settings, 15 effects settings, four selector values and 513 bounded
volume values at native `-O0` and `-O2`. The 461,700 cases per optimization
verify selection, scale, minimum-one/maximum-127 limits, disabled settings
and unchanged setting storage. Five
negative controls change the selected setting, range, divisor, minimum or
maximum and are all rejected. The host test does not assert Miles behavior,
VC6 inlining or overflowing multiplication semantics.

### Joint recheck of byte-neutral Load fences

The complete post-integration deletion audit at `d7f7be28` finds two
individually byte-neutral `game::load` fences. Before adopting them,
`generate-redundant-load-fence-family.py` exhausts the four combinations in
context `c37c4b8bdc6f290cfc35`: four scored states, one emitted object identity,
one reproduced elite. This separate joint control prevents assuming that two
individually inert directives are also inert together.

The combined `87c339c85bd6535c153b4761` removes only the creature-bank
`loadObjectVector` fence and the normal-return fence. Against the unchanged
`3c3d0c896d77feef57c45b36`, strict raw comparison passes all 822 sections,
5287 relocation destinations and function positions. Every game score stays
fixed. Source statements, helper interfaces, return scopes and the remaining
`isLocalHuman` directive are unchanged. The source generator records the
pre-adoption anchors; the complete audit snapshots remain separate controls.

### Compiler-generated text-dialog teardown

`generate-text-dialog-dtor-family.py` uses positive Dreamcast member attributes:
the CTextDialog field list (class `0x2c52`, fields `0x2c53`) gives its destructor
`0x107`, including `compgenx`, as for CWaitForReadyPlayersDlg. Explicit
CAnimatedDlg and TDialogBox destructor controls give `0x007`. This is not an
absence-of-lines or absence-of-locals argument. The canonical correction removes
the false explicit empty declaration/body and keeps the retained retail body
at `0x490770` as an `IMPLICIT_DTOR` claim; the deleting wrapper stays claimed.

Context `921c64eda6cd9b36c940` exhausts four states across all twelve actual
header consumers: four combined emitted-object identities and four reproduced
elites, each scoring 1129 functions. Implicit-with-existing-fence candidate
`62c9f3297fad1750e2c2db22` preserves every score. CAnimatedDlg's instruction
bytes stay fixed, while its base-cleanup relocation now correctly names
TDialogBox instead of CTextDialog. The readiness caller and the separately
emitted wait-dialog destructor keep their named calls and body bytes.

Removing the animated-destructor fence makes the implicit wait-dialog
destructor exact (86.3333% to 100%), but drops waitForReadyToPlayMsg from
90.6522% to 75.0683% and removes the retained CNetMsgHandler::copy body.
The correction therefore does not itself justify deleting that override.
`generate-animated-dtor-lifetime-family.py` follows the corrected parent with
six real nullable-sprite bindings crossed with that fence. Context
`5ebe0ea56c6aed44ee2a` exhausts all twelve states, five emitted-object identities
and five reproduced elites. Every unfenced state still loses the copy body;
the guarded object reference raises the readiness caller to 83.0186%, while
the remaining forms stay at 75.0683%. None is adopted. The ordinary destructor,
virtual Complete sprite disposal and implicit base cleanup remain canonical;
no dummy work, alternate declaration or explicit derived-destructor router is
introduced. These finite results bound those lifetime alternatives, not all
possible caller reconstruction.

### Creature-bank and resource-cost boundaries

`generate-bank-value-lifetime-family.py` starts from the shared nested-size
fence in valueOfBank. Context `66bc080d5cad95205a44` exhausts sixty bank-receiver,
artifact-size/receiver and combat-value lifetimes crossed with fence removal:
sixty scored states, eight emitted-object identities, eight reproduced elites.
Every unfenced option makes the retained bank body exact (92.2043% to 100%),
but leaves aiValueOfEvent at 97.4610% instead of 98.0336%. No spelling resolves
that remaining nested decision, and all other philai scores stay fixed.

The follow-up discovers an actual missing source boundary: DC `0x10f2f8`,
philai.cpp:1331, calls the player-pointer resource-cost overload from the
player-ID overload. DC bank `0x110808` in turn calls the ID overload with the
hero owner. The resource-cost experiment restored that ordinary
forwarding call and the bank's canonical call together, crossing both with
fence removal. Context `a44951e66f11cc57a5c4` exhausts eight states, eight object
identities and eight reproduced elites. The canonical pair with the fence
(`d42fab3c0207a4cdb4be3f14`) preserves the complete control object's 229 sections,
1442 relocation destinations and function locations. The previous duplicated
loop was not evidence that retail lacked the forwarding boundary.

The adopted unfenced pair `82675560b82de2a6702b0056` retains the ordinary
overloads, the single pointer-based loop, the inline bank helper and all three
source calls. Production exactly reproduces its 229 sections, 1440 relocation
destinations and function positions. Against the original control, all 127
emitted functions remain present; only valueOfBank and its dispatcher change
raw body bytes, with the other 125 byte-identical. The retained bank matches
all twelve retail blocks and all four named calls. Its remaining named data
differences are the existing game/current-player aliases and unclaimed creature
traits label, not new helper-call discrepancies.

Ordered review narrows the dispatcher residual to its **first** bank arm:
retail calls vector::size at +0x66d, while the candidate expands it. The second
arm keeps its call (candidate +0x81e), and the final arm still calls valueOfBank
(+0x11ff). The second arm's six blocks all match in the separately shifted
local ranges; both early arms keep the correct pointer-based resource-cost
calls. Thus the 0.5726-point caller reduction is one missing natural nested
call, not loss of both arms or replacement of a canonical helper. Full build
keeps its MAX/HIST at 98.0336% and restores the bank's MAX to 100%.

`homm3.vc6.test_bank_resource_boundary` imports the three actual bodies. Its
bounded 11,520-case native oracle at `-O0` and `-O2` checks empty/failed-combat
exits, owner versus active-player selection, per-resource accumulation, signed
reward guards, artifact counts and unchanged bank inputs. Fixture-only entry
counters check the recovered call route and ordering. Seven negative controls
are rejected, including a numerically equivalent bypass of the owner wrapper.
The fixture does not claim retail ABI/layout, arbitrary floating-point edge
behavior or VC6 inlining.

### Mouse-thread lifetimes and inherited task teardown

`generate-stop-mouse-lifetime-family.py` tests mutable/const references to
the actual global thread and event storage, plus guarded-block/early-return
exits, crossed with the existing fence. References retain every global reload
across Windows calls; they are not snapshots of potentially changed handles.
DC's older helper retains only the final pointer restore, so its PC line gap
is not used as assertion evidence. Context `34cdd1cea3463120d055` exhausts all
36 states: six emitted-object identities and six reproduced elites. Every
unfenced option raises generateRandomMap from 92.6386% to 97.9759% but drops
setupScenarioOptions from 100% to 90.1470%. None is adopted. The caller's
proven request/progress/path scope and the shared ordinary helper stay intact.

The score-flat task-destructor deletion needs a separate untracked-body review.
The completed experiment at context `0b5874006d53a33b2e6b` reproduced
both source states and both emitted-object identities. All 223 tracked scores
agree. Its one-off whole-object check proved that all 396 emitted functions
remain present, with 395 bodies and their relocation destinations unchanged.
After excluding only `.debug` metadata, all 737 remaining sections preserve
order and attributes; all bytes/relocations outside the generated Proc body
stay fixed. That body changes from a five-byte jump to Task into the same
38-byte teardown as Task, with identical instructions and delete relocation.
Padding changes its section from 16 to 48 bytes. The retained Task body
continues to match every retail instruction and named call at `0x583ef0`.

The initial ICF hypothesis is **refuted**, not reported as a repair. Three
genuine hash-verified VC6 LINK controls use the unchanged/unfenced real objects
with `/OPT:ICF`, plus unfenced `/OPT:NOICF`. Each keeps Task and Proc at
distinct addresses: the ordinary Task body is non-COMDAT. These partial
diagnostic images have 45 unresolved symbols and are not executed. The normal
source model is preserved; no false inline, explicit derived destructor or
unproven implicit Task declaration is introduced to force a fold. The dead
scalar-wrapper claim and final ownership/link-layout questions remain separate
debt. Removing the override therefore buys a cleaner source at unchanged
tracked scores, with an explicitly bounded 32-byte untracked code increase.

Production exactly reproduces selected `f0e98f39dea432f23d145379`: all 824 raw
sections, 7211 relocation destinations and function locations agree. The
body verifier also passes directly against production and rejects unchanged,
reversed and unrelated mouse-helper candidates as three negative controls.
The final full build preserves 4074/4764 exact and 96.38% linked/whole-image,
with no score change, MAX reset, migration or lost banked RVA.

### Sacrifice-slot helper boundaries

`generate-sacrifice-slot-boundary-family.py` checks two positive DC facts from
`update_slot` (`0x125b3c`): line 842 obtains the artifact snapshot from
`hero::get_artifact`, and lines 847/855 both call the ordinary
`update_artifact_widget` (`0x125a4c`, line 800). Retail `0x562840` expands both
widget-helper calls, with different nested visibility decisions. Keep the real
helper before its caller and its retained claim at `0x5639e0`; no false inline
declaration, assertion, source-order trick or replacement override is needed.

Context `f394ad344803747dcb3d` exhausts six successfully scored states and
reproduces all six search-identity objects: direct-field/accessor snapshot
crossed with pasted-fenced, pasted-unfenced and canonical-unfenced first update.
All 55 scores are unchanged for the canonical call. The pasted-unfenced
negative controls lower only updateSlot from 100% to 99.1368%, whether or not
the artifact accessor is restored. Production adopts both positive boundaries,
candidate `b20261b99b0d0a9b31069e1f`, against unchanged
`646bd196a8d8203b4e71d54c`.

The six distinct runner identities do not imply six different instruction
streams: private label counters are deliberately not generalized away by that
metric. The stricter same-layout proof, `compare-coff-layout.py`, independently
shows all **200 raw sections**, **1678 relocation destinations**, and function
identities/positions unchanged between the untouched control and the selected
candidate, and again against production. This includes untracked bodies and
data, not just the 55 scored rows. The pasted-unfenced control fails that proof.

Retail review confirms all twelve CFG blocks and the ordered fourteen named
calls. The first expansion calls `setVisible` at `+0x6d`; the other visibility
sites expand it into `sendMessage`, including the second artifact expansion
at `+0x119`. The three remaining display-name relocation differences are
already claimed artifact/slot data, not new helper-boundary mismatches.
One depth-zero override is retired at unchanged object code and matching score.
The full delink/build passes all gates at **4074/4764 exact and 96.38%
linked/whole-image**. Only updateSlot's source hash changes in the matching
ledger; no score, MAX or HIST changes. The cleanliness bound drops to 216
depth-zero overrides.

### Creature-bank table owners and ordinary level reader

`generate-bank-table-owner-family.py` replaces two incorrectly file-scoped
`const int` tables and the reward enum adapter with the source-owned tables.
NB11's `guard_types` (type `0x5601`) and `reward_types` (`0x5602`) are mutable
`TCreatureType[11][5]` and `[11]` statics belonging to loader procedure
`0x7112c`. This is positive storage/type evidence, not a line-gap inference.
The hash-verified retail image puts their 55 and 11 dwords in writable `.data`
at `0x6702a0` and `0x67037c`. Preserve zero-initialized padding after guard
sentinels rather than inventing extra creature entries.

Six consumed creature names come from DC's actual enum: four added members
and two moved atomically from AI's separate enum into canonical TCreatureType.
The initial unscored opposite corner exposed the two duplicate names; that
manifest was repaired before continuing. Corrected context
`7db5f64e100e77ef33ce` reproduces both states across all **95 header consumers**
and **4117 scored functions**. Control `ed35dc233ed5555e0bfbe8a9` and native
candidate `24e6bf9987f4beff9cd1a45f` change only three tracked RMG scores:
quest-creature generation 100% → 99.7349%, loadTemplates 80.8461% → 80.8308%,
and writeMapHeader 77.8952% → 77.9030%. Their bodies' source hashes are
unchanged, so MAX/HIST retain all prior peaks. All three bank scores stay flat.

The stricter raw-object comparison passes **90 of 95 entire objects**.
Creature-bank moves the tables from `.rdata` into `.data`; all fourteen
emitted function bodies remain byte-identical. Three TUs (creaturetype,
spelldefs, herodefs) retain identical function bytes/named references but
shuffle anonymous-namespace BSS; **same-source reproduction also shuffles
those BSS layouts**, so do not attribute that variation to the enum edit.
RMG preserves all 441 function positions and section extents, with actual
body-byte changes confined to the same three scored functions. No object
rewriter or new scoring normalization is involved.

`verify-bank-table-owners.py` independently checks NB11 procedure ownership,
exact enum dimensions, emitted local-static mangled owners, writable section
placement and **all 66 dwords**. It passes on the selected object and production,
rejects the old file-static control, and rejects six non-mutating byte/order/
extent negative controls. Before the reader recovery, candidate/production
also match all 29 raw sections and 101 relocation destinations exactly.

DC's ordinary static `initialize_creature_bank_level` (`0x70fe0`, source
line 32) takes `type_creature_bank_level&` and `const vector<char*>&`; the
loader calls it at line 136 after installing the guard/reward types. Restore
that real body before its caller, not a false inline declaration or pasted
reader. `generate-bank-level-boundary-family.py` crosses four meaningful
column-cursor lifetimes plus the pasted control with unsigned/signed guard
indices. Retail's guard-copy `jl` positively supports the signed index.
Context `c601a370d5246188c67a` exhausts and reproduces all ten states/objects.
Selected `f9671c7898dc10b79eeb7727` uses the DC cursor from column two and
advances past each guard count before its zero test: **89.4550% → 97.5355%**.
Both other tracked bank functions remain 100%. The signed-index-only
negative control gives 88.8910%, demonstrating why an isolated score dip
does not reject a source fact. Production reproduces all 28 raw sections,
100 relocation destinations and function positions of the chosen object.

All fifteen named calls now agree with retail, including the recovered
string `_Eos` expansion, and all twelve branches agree. The remaining two
CFG size differences are the guard-copy inductions: VC6 forms a destination
minus source bias where retail keeps two cursors and a counter. A string
byte-store address also exchanges commutative operands. This is a measured
residual, not bank-TU closure or a request for a new suppression pragma.

`homm3.vc6.test_bank_level_boundary` imports both actual bodies, with only a
fixture entry counter added. At `-O0` and `-O2`, eleven numeric patterns across
four resource conditions check null/short input disposal, the retail threshold
of thirteen rows, all 44 level records, exact row and parsed-cell order, guard
sentinels, seven resources, artifacts and the reward count's signed-byte zero
test. Seven negative controls are rejected. The mock is a bounded behavioral
oracle, not proof of retail ABI or compiler inlining.

Full delinking/build passes at **4073/4764 exact, 96.39% linked fuzzy and
96.38% whole-image**. One bank checkpoint rises; no MAX resets or banked
RVA losses occur. The audit now has **63 unions** (37 source, 26 header),
including 39 remaining reconstruction adapters, and **221 inline overrides**.

### Skill-quest proposal lifetimes and six-fence boundary

The Complete-only skill-quest proposal at `0x56dad0` has no established
Dreamcast counterpart. Its current score is **83.2252%**, not the old 75.4324%
quoted beside its source. Retail CFG, named calls and byte-verified candidate
statements were inspected before these families. The input is a signed byte;
the current `const int&` binds a converted temporary, not the field itself.

`generate-skill-proposal-input-family.py`, context `32c06caa67f5495b6533`,
exhausts **36 states / 24 distinct objects**, with ten reproduced elites.
It crosses three input bindings, three output bindings, independent declaration
order and the first dialog loop's index signedness. None improves the caller
or any sibling. A native signed-byte reference reaches the same 83.2252%;
the signed dialog index required by retail's `jl` measures 82.9730%.
Output pointer cursors lose further. The byte-reference control has identical
raw section bytes but different generated EH function-label numbering, so
this is not claimed as a strict whole-object identity result.

`generate-skill-proposal-fence-family.py`, context `d65a9ca695d30ffc1d3c`,
exhausts all **64 subsets / 64 objects** of the six inherited fences, including
joint removals; ten elites are reproduced per generation. Every nonempty
deletion lowers only the proposal caller. The least-cost individual deletion
is the custom-string insert fence, **83.2252% → 82.2342%**; removing both
insert fences gives 78.7928%. All 118 other scored rows stay fixed.

`generate-skill-proposal-string-family.py` verifies that parent's snapshot
and reproduced control, then crosses owned versus lifetime-extended const
references for both unmodified returned strings with twelve paired/individual
fence masks. Context `c46dad8e3df70bd51249` exhausts **48 states / 36 objects**
and reproduces ten elites. These real string-lifetime alternatives change no
score; every deletion has the same cost as its parent. They do not recover a
removable fence. Of six differing displayed call names, four are byte-identical
shared vector COMDATs; only two are real string-destructor versus `_Tidy`
boundaries. An aggregate call count is not an additional six-site diagnosis.

These bounds do not prove that the six fences are original source or that
the whole function is unrecoverable. They rule out deletion alone, these
input/output bindings and these returned-string lifetimes in the measured
source context, without inventing helper bodies or changing normalization.

### AI combat container ownership and canonical melee

The inherited `type_monster_vector` subclass was a code-generation shim,
not the Dreamcast member type. DC's `creatures` is an actual
`std::vector<type_monster_data>`. Its `size()` is separate from
`type_AI_combat_data::get_total`: the latter's complete DC body at `0x2c6ac`
loads the `total_combat_value` member. Retail `0x427750` is the 33-byte vendor
vector-size body, not that game accessor; `0x4276c0` is the 135-byte native
vector copy constructor. Both claims now belong to their compiler-generated
STL owners. The class field is named `m_totalCombatValue`, and ordinary count
loops call the native vector's `size()`. Public begin/end also replace access
to vendor-private pointer members. Neither the vendored STL nor normalization
is modified.

Before that correction, `generate-general-melee-boundary-family.py` exhausted
**12 states / six objects**, all six reproduced, in context
`7e95efb5c8032fc9c974`. It crossed canonical versus pasted/pinned melee with
separate/joint zero guards and shared/branch-scoped ratios. Replacing only the
paste with the DC-proven ordinary call lowered chooseMelee from 90.9329% to
77.6098%; removing only its fence scored zero despite retaining the emitted
caller. Those results did not refute the source call at DC line 1346.

With native container ownership, the canonical call raises **chooseMelee,
doGeneralMelee and the two-side getEnchantmentValue to 100%**. One override
is retired. All 42 chooseMelee CFG blocks agree, and the retained general-melee
body also matches. Two displayed chooseMelee call-name differences are proved
shared bodies: its 38-byte vector destructor equals retail's widget-vector
destructor at `0x46a650`, and its three-byte `_Destroy` equals the artifact
instantiation at `0x404140`. They are not wrong helper calls.

The owner change has real collateral: initializeCreatures falls from 91.9548%
to 82.6704%, and the previously exact `_Unguarded_partition` body at `0x427c30`
is no longer emitted because it expands into the initializer. Its compiler-
generated claim and historical peak remain; a passing banked-RVA gate does
not mean that body is still present. `_Sort`, vector-size and vector-copy
remain exact. This must not be hidden as claim removal or a normalization fix.

`generate-ai-creature-initialization-family.py` exhausts/reproduces **nine
states / nine objects**, context `6de4e0a94f21173ed59e`: the force modifier's
real early initialization and three public sort-argument lifetimes. DC line
222 and retail's entry argument copy support the declaration initializer;
it recovers initializeCreatures to **83.4275%**. Direct calls, a vector
reference and named iterators score identically.
`generate-ai-classifier-boundary-family.py` verifies all nine parents and extends them with the
ordinary/inline, const/mutable and original/hoisted `getCatagory` interface.
Context `2027c66d848baf9eadfa` exhausts **72 states / 27 objects** and reproduces
ten elites per generation. Those declaration choices do not change scores;
the adopted ordinary const helper follows DC `0x2a52c` in its real source
position after wall-archery adjustment. All 72 emitted objects were checked:
none restores the missing partition body. The chosen non-elite was independently
recompiled and reproduced before adoption.

The selected initialization/classifier implementation and production agree
in all 73 raw ai_combat sections / 432 relocation destinations, and all 420
ai_player sections / 1963 destinations, including function identities and
positions. Both header consumers and all 176 scored rows are checked.
The full checkpoint has **4075/4764 exact, 96.39% linked and 96.38% whole-image**.
Only the initializer's source edit resets MAX below its old peak; HIST retains
91.9548%. The missing partition keeps its unchanged-source MAX/HIST at 100%.

`test-ai-melee-owner-boundary.py` extracts the actual accessor and ordinary
melee body. At native `-O0` and `-O2`, twenty independent vector-count/total
cases and 169 melee pairs cover zeroes, ties and float rounding around 2^24.
Mocked kill/damage/final-value calls check operands and ordering. Five deliberate
faults are rejected. This is a bounded behavioral check, not a VC6 ABI oracle.

The final interface cleanup follows raw NB11 types rather than inherited
comments: class `0x5a07` / field list `0x5a4c` names `current_hero`,
`current_army`, `can_cast_spells`, `wall_archery_penalty` and `wall_speed_limit`.
Those names now own the corresponding `m_` members. The cleanup preserves
every raw byte and relocation destination in both consumers (77 ai_combat
sections / 462 destinations). A follow-up public-symbol check corrects the
initial return-type reading: chooseMelee's `0x5a2b` debug record uses primitive
`0x20` for byte storage, but its decorated public contains `IBA_N` and proves
`bool`. The byte-return ABI and byte-consuming callers do not distinguish
these source types. The declaration and definition therefore retain `bool`.

Conversely, the same class's constructor method list `0x5a0f` gives its copy
constructor attributes `0x003`, not the `0x103` compiler-generated attributes
on assignment/destruction. Preserve the explicit memberwise copy constructor;
its native vector member has a separate retained STL owner. A sole line row
at a caller's closing source line is not enough to declare it implicit.

### AI mass damage, Familiar predicate and value-wrapper boundaries

DC `cast_mass_damage_spell` (`0x2ac58`, lines 747..760) is one body with two
source calls, not caller-specific copies. Crucially, line 758 overwrites the
running damage value with `take_damage`'s returned capped value; line 759
subtracts that value from total combat value. Both retail `castSpell` expansions
at `0x425bd0` corroborate this assignment (ESI/EDI from EAX). The old cloned,
fenced loops carried the uncapped sum to the next creature, which changes
behavior when accumulated damage exceeds a stack's total. Separate vector
subscripts across the opaque spell-damage call and zero initialization before
mastery lookup are also corroborated by both builds.

`generate-ai-mass-damage-boundary-family.py` crosses these three facts with
cloned/canonical, inherited-inline/ordinary and fenced/unfenced boundaries.
Context `ba7a1cfd8351a6b35d25` exhausts **48 states / 48 objects**, with ten
reproduced elites and complete 176-row, two-TU vectors. A high score from the
uncapped-dataflow control is not an eligible reconstruction. The corrected
ordinary single helper scores 88.5664% with the first inherited fence, and
79.8398% with both old fences removed.

DC `cast_spell` line 1047 calls the ordinary const `has_creature` predicate
(`0x2ab3c`, source line 694, before mass valuation). Retail's Familiar scan
corroborates its type/positive-count tests and single mana update after success.
`generate-ai-familiar-boundary-family.py` verifies the 48-parent manifest,
snapshots and reproduced elites, then crosses that canonical boundary with
all parents. Context `dbe7501806f1d9581f9e` exhausts **96 states / 96 objects**,
with ten reproduced elites per generation. Restoring the predicate raises the
correct unpinned mass variant to **86.7910%**, while the corrected one-pin
variant stays 88.5664%. It changes no other score. The adopted all-correct,
unpinned corner is `f236fbd6dcc860f3c9c8c5da`; production matches its 75 raw
ai_combat sections / 443 relocation destinations and the complete ai_player
object. Both old mass fences and the false helper clone are removed.

The remaining castSpell difference is a real nested-boundary mismatch, not
the old allocator-only diagnosis. Its first mass expansion expands
`getSpellDamage` where retail calls it and retains `takeDamage` where retail
expands it. The resulting extra branches/frame slot explain the source diff's
first prologue mismatch. Retail keeps both calls in the second mass expansion.
The current CFG has 100 blocks / 60 conditionals against 92 / 55 in retail;
both have four returns. No additional pragma or duplicate body is introduced.
The previous 93.0273% score used the incorrect dataflow and survives in HIST.

DC also proves const receivers for both enchantment-valuation overloads and
the two-side mass-valuation wrapper. Correcting all declarations/definitions
together is strictly byte-neutral under the one emitted function rename:
75 ai_combat sections / 443 relocation destinations, plus all of ai_player.
The full build migrates the existing enchantment claim at its same RVA.

`generate-ai-value-helper-family.py` restores a different canonical boundary:
the by-value `min`/`max` wrappers already owned by `homm3_minmax.h`. DC
`includes.h:97,114` and actual AI call relocations name those wrappers; their
callee selectors return const references to the wrappers' still-live arguments.
The local templates incorrectly returned references to their own by-value
parameters. Retail's two operand homes do not prove that invalid declaration.
Context `5468aea93d306459a4f1` exhausts/reproduces **eight states / eight objects**,
also checking ordinary versus inherited-inline mass/enchantment valuation
definitions. The ordinary definitions are score-flat. Canonical min/max
preserve every score except getResurrectionValue's 100% → 92.9310% dip.
Production reproduces the all-canonical corner's 77 sections / 462 relocation
destinations and the unchanged ai_player object.

`generate-ai-resurrection-value-family.py` addresses that precise residual:
retail loads the selected scalar before multiplying, while a single return
expression multiplies through a retained address. DC lines 99..102 support
separate scale/divide, cap and result stages. Context `5de5d8835a813ce9959c`
exhausts **six states / three objects**, all three reproduced. A fresh capped-
value local (`a25b0998f922c6853ac8d8cb`) restores **100%**; reusing the earlier
spell-value local, including separate DC-order stages, remains 92.9310%.
All other 175 scores remain fixed. Keep the real value-returning wrappers.
Production reproduces the selected 77-section / 462-relocation object; retail
review confirms nine CFG blocks, four named calls and zero instruction deltas.

`test-ai-mass-damage-boundary.py` extracts both the actual canonical mass loop
and `takeDamage`; at native `-O0` and `-O2`, 328 stack-array cases check reverse
order, the capped loop carry, hero/damage arguments, stack counts and total
combat value. Six deliberate faults, including the inherited uncapped carry,
are rejected. `test-ai-resurrection-value.py` checks 3456 bounded input cases,
call order and nonmutation with five negative controls. The spell/hero services
are mocked; these are behavioral tests, not VC6 ABI or inlining proofs.

The final full build has **4075/4764 exact and 96.38% linked/whole-image**.
All three ai_combat inline overrides are gone; the global census is **218**.
The independently measured signed skill-dialog index/native-byte binding is
also adopted at 82.9730%, with its 83.2252% HIST preserved. All source-backed
name/signature changes are regenerated by the normal labels/delink pipeline.

### Tactical mass/summon boundaries and exact spell dispatch

DC `get_damage_value` (`0x3d96c`), `get_group_damage_value` (`0x3dabc`) and
`get_mass_damage_effect` (`0x3db2c`) are const members. `consider_mass_damage`
(`0x3de90`) and `consider_summon` (`0x41e5c`) additionally take writable choice
references. The damage/group/effect/mass interface correction preserves every
raw byte, function location and relocation destination in all five consumers:
ai_tactical (145 sections / 951 destinations), ai (101/542), drawing (77/651),
ai_combat (77/462) and ai_player (420/1963), under the two evidenced retained
function renames. Both retained helpers remain exact.

The dispatcher at `0x43bb20` expands its mass/group/summon helpers but retains
the mass-effect call. Its inherited model imposed that split with force-inline
and a depth pin, and pasted the summon helper at its caller. DC lines 3129/3167
prove the two helper calls; summon line 3098 calls `get_mastery_value`, and line
3107 writes cast-now after the kills-only split. The required retail review
finds a 39-block, 19-branch, ten-return caller at 98.1927%, with all seventeen
named calls already aligned and only register/scheduling deltas remaining.

`generate-tactical-mass-boundary-family.py`, context `07302be162953796ffdc`,
exhausts **24 states / 16 objects**, with ten reproduced elites and all **347
scores across five TUs** checked. It crosses ordinary mass/group definitions,
the existing effect fence/deletion, and pasted-table/pasted-accessor/canonical
ordinary-summon forms. The fully canonical, unfenced corner `[1,1,1,2]`,
`6442401e5b4a12b3a56f8e26`, preserves every score. Removing the pin while
retaining the pasted mastery-table expression instead lowers considerSpell
to **80.5073%**. Restoring the accessor, even before lifting the summon body,
recovers the exact required inline split. A real source call in one arm can
therefore repair an earlier arm's inline decision without caller padding.

The boundary-only production object independently reproduces the chosen
**148 sections / 966 relocation destinations**. The dedicated raw verifier
also proves that all **145 prior sections, 102 emitted functions and 951
relocation destinations** are unchanged. Only the three ordinary helpers gain
unreferenced executable COMDATs (padded sizes 144, 528 and 160 bytes); no
relocation points at them. These natural compiler-emitted copies have no
claimed retained retail address. This is not whole-object identity with the
old object, nor permission to ignore untracked code changes.

`generate-tactical-dispatch-lifetime-family.py`, context `8704e5c4ec9d3952d373`,
then exhausts **16 states / six objects**, all six representative objects
reproduced. DC row 973 forms the target address before row 975's damage call;
the family tests a direct address, named pointer/reference and named damage
result, crossed with four ordered mass-result lifetimes. The pointer and
reference targets both make **considerSpell 100%**; direct addresses and named
damage results stay at 98.1927%, regardless of the mass-result alternative.
Only the caller's score changes among all 92 tactical rows.

The minimal pointer-target form `[1,0]`, `12c6d47d7d9f0b9f0f0e6cb4`, is
independently recompiled and adopted. The accumulator also recovers its DC name
`value`. Production preserves all 148 raw sections / 966 relocation destinations
of that repeat, while the other four consumers preserve their complete objects.
The final retail CFG, instructions and named call stream are checked again;
the former group-walk and later Dispel register differences are resolved.
The 100% verdict uses the existing matching metric: the ten remaining data-
name differences are four spell-table alias references and six unclaimed
global references. All sixteen actual calls and the jump-table dispatch
relocation agree by named target/addend; no data-name normalization is changed.

`test-tactical-mass-boundaries.py` extracts the four actual helper bodies. At
native `-O0` and `-O2`, **3025 effect cases, 6144 group/mass cases and 1024 summon
cases** check ratios, clamps, reverse order, both hero/side arguments, mastery,
guards, the post-split cast-now write, and unaffected choice fields. Nine
deliberate faults are rejected. Mocked battle services and bounded non-overflow
integers make this a semantic check, not a VC6 layout or floating-codegen oracle.

All ai_tactical inline overrides are gone, as are its forced mass declaration
and pasted summon implementation. The global census is **217 overrides**
(212 depth-zero / five auto-inline-off) across 29 TUs, and **63 unions**.
This finishes the current dispatch matching pass, not the remaining interface,
min/max or whole-TU source-reconstruction debt.

### AI turn-close purchase and warning boundaries

`type_AI_player::endTurn` (0x428dd0) stood at 89.5263% behind a string-append
fence. DC ai_player.cpp:439 positively calls the ordinary `purchase_buildings`
member, whose body at 0x31094 owns `prohibited_creatures` and calls
`fill_prohibited_array` followed by the `purchase_building` loop. Complete
expands this boundary with its 145-entry flag table. The previous declaration
was an unused byte-returning, pointer-argument interface; the actual member
is void/no-argument, and its decorated `...@@IAAXXZ` public proves protected
access. No external caller uses the superseded declaration.

DC line 493 calls `format_string`, then `string::operator+=`, then destroys the
temporary. Its warning-loop backedge at 0x2e992 increments and sign-extends a
short. Retail independently passes the formatter's return pointer straight to
the retained `append(string, pos, count)` at 0x41b250 and exits on an unsigned
positive length check. The old source used an explicit append and named copy,
pasted the purchase helper, and expressed the warning walk as two pointers.



Run the generator against the pre-adoption source (`3be2cd52`); its anchors
intentionally reject changed source. The complete manifest and snapshots are
preserved under context **`6d71af72491811a3bd28`**. All **60/60 states** compile,
producing **60 distinct objects** and **ten reproduced elites**. All **437
tracked scores across five current header consumers** are checked; only
endTurn changes in the entire family. The reproduced opposite corner and
winner **`[14,1,1]` / `eb3fa06f257171a880de80b2`** reaches **100%** with the
ordinary purchase member, temporary `+=`, short index and positive length.

The deletion-only control falls to **61.2669%**. Against the exact candidate,
pasting the purchase loop back falls to **77.0226%**, using an int warning
index to **90.6917%**, using a named format copy to **98.5038%**, and changing
the length guard to a truth test to **99.7744%**. These controls explain why
an isolated fence deletion was previously loss-only. No helper is marked
inline and no dummy operation or replacement suppression is introduced.

The final source restores the DC local names `purchaser`, `checker`, and `msg`.
Its entire raw object reproduces the winning recompile: **421 sections and
1967 relocation destinations**, permitting only the independently proved
public-to-protected helper-symbol correction. Against the old object, the
other **246 emitted function bodies** retain their raw bytes, no function
disappears, and one unreferenced 96-byte padded ordinary helper is added.
The caller's cleanup offsets and STL COMDAT order change, so this is not a
claim of whole-object identity against the old source. The four other
consumers do preserve their complete raw objects: advmgr **308/4091**, ai
**101/542**, ai_combat **77/462**, and philai **229/1440** sections/relocations.

Retail verification agrees on all **54 CFG blocks, 32 branches, two returns,
and seventeen named call targets/addends**, with no differing instruction
row. The existing 100% metric still ignores thirteen data-name differences:
five `g_game`/`gpGame` name aliases, six unclaimed data/vtable references, one
source-claimed resource-name table and one generated empty-string label.
No data-name normalization or retail target is changed to obtain this result.

The native actual-body oracle tests **8192 cases per form at -O0 and -O2**,
crossing all seven-resource sign masks with 64 player/town/alliance/purchase
states. It checks reserve clamps, strategy and purchase order, the mutable
145-entry flag array across repeated calls, Marketplace lookup, AI-before-
human gifts, formatting arguments, and warning order/content. Five deliberately
wrong controls are rejected. All sixty source forms pass before adoption;
the default test subsequently checks the actual adopted bodies, and `--source`
can check any reproduced candidate. Mocked services and bounded host integers
do not claim retail ABI, EH or inlining verification.

Full delinking/build passes at **4083/4764 exact**, **96.43% linked** and
**96.42% whole-image** (rounded), with one checkpoint raised, no MAX reset and
no lost banked RVA. The census is **216 overrides** (211 depth-zero / five
auto-inline-off) and **63 unions**. This removes one override and restores
one exact caller; it does not close ai_player or exhaust the remaining debt.

### Grail destination and shared map-extra boundaries

DC `check_holy_grail` (`ai_player.obj:0x32e30`) takes the destination vector by
reference, names its record `point`, and calls `game::get_cell` at line 3181.
`find_all_destinations` (`0x33038`) also takes that vector by reference and
calls `game::GetNumMapLevels`. Restoring these interfaces/accessors preserves
the initial **96.3651%** and removes the Grail map-lookup depth pin. The
intermediate object calls `NewfullMap::zCell`, whose complete 49-byte body is
identical to retail's folded `cell` target; this is not proof of a new inline
decision. The final fully recovered caller below calls `cell` by name.

The provisional `aiGetArtifactPlayerValue` was not a retail-only function.
DC `AI_get_value_of_artifact` at `0x37514` proves its `const type_artifact&,
long` player overload, including the empty-artifact guard, floor of ten,
hero walk and non-exact equip valuation. One canonical declaration now owns
the interface, including seerhut's formerly inconsistent int/int declaration.
The full build migrates the existing 0x433aa0 claim under its real overload
name without changing any score. Keep its ordinary body and distinct caller
decisions; no false `inline` declaration is introduced.

The initial boundary family (`070f8f532c43a477f273`) emits all 36 states, but
only its 18 pointer-signature states have comparable caller scores. The other
18 change the mangled function name while the runner still requests the old
name, producing false zeroes. They are not codegen losses or valid ranking
inputs. The reference migration was instead verified by full labels/delink/
build. The generator now defaults to those 18 name-preserving states;
`--signature-controls` retains the original diagnostic interface choices.
Run this generator against the pre-adoption `182b7a26` source.

`generate-ai-grail-lifetime-family.py`, context `23ae2167ec23afbe3de3`, exhausts
**24 states / 11 objects**, with ten reproduced elites. It tests actual artifact
construction and player/friendly-distance lifetimes after the interface repair.
Removing the artifact pin alone gives **90.7841%**; the original typed temporary
without the later caller-boundary recovery gives **61.8460%**. These are local
controls, not grounds to reject the positively evidenced typed constructor.

DC `AdvMgr.h:1254`, `advmgr.obj:0x1f084`, proves the inline, int-returning
`GetMapExtra(type_point)` overload forwarding x/y/z to the unsigned-short
scalar accessor. It follows the class's `get_map_center` at line 1245. Move
that body from advmgr.cpp to its original header position after the class,
remove findpath.cpp's falsely static duplicate, and restore the two point
calls at destination lines 3300 and 3359. The scalar declaration agrees with
kb.h and the retained exact 0x4f79b0 body.

`generate-ai-grail-map-extra-family.py`, context `3512473cbfdffe1a9642`, tests
**24 states / 22 objects** and all **2340 tracked scores across 34 header
consumers**, with **ten reproduced elites**. Its unchanged-source and
opposite-corner repeat controls pass.
The fully canonical corner **`[5,3]` / `ecae36eaf4a6f8ae1a792ae7`** reaches
**96.9365%** with both point calls, the typed artifact temporary and neither
Grail pin. Restoring only one point call leaves the unpinned caller at
91.0238% or 91.3555%; keeping a named point copy at the first site with the
typed artifact temporary also gives 91.0238%. Both original boundaries and
the temporary lifetime matter together. No dummy operation, copied helper,
replacement suppression or scoring change is used.

The chosen caller agrees with retail on **81 CFG blocks**, including every
block's flow and instruction count, and has **29 calls with no one-sided
site**. The formerly missing `vector::size()` call at +0x3fd returns naturally.
The map, patrol and artifact calls are retained without intervention: base
+0x696/+0x6ba/+0x741 correspond to retail +0x694/+0x6b8/+0x73f. Eighteen call
targets agree by name; the other eleven use five folded-template identities.
Their complete bodies are independently byte-identical to their retail
targets: `size` (19 B), `_Ucopy` (73 B), `_Ufill` (64 B), `_Destroy` (3 B),
and `insert` (778 B). Only insert has relocations, and both allocation/delete
targets also agree. No new alias normalization is used. The remaining
instruction differences are register allocation and bitfield scheduling,
not the former Grail inline mismatch.

The header-only control independently raises `CEnterNameEdit::onKillFocus`
from 99.8710% to 100%; no other tracked score changes outside the destination
caller. Raw inspection is stronger and slightly different: **31 complete
consumer objects** preserve all section bytes, function locations and
relocation destinations. ai_player changes only the destination function's
body, but also moves the unchanged `zCell` COMDAT past intervening sections;
that section-index translation accounts for all other bytes, function
locations and relocation destinations. singleselectionwindow changes only
the focus handler. kb additionally
changes four bytes in `oldmain`, despite its unchanged 78.4499% score: the
EDI/EBX spill homes at +0x129a/+0x129d exchange `[ebp-0x20]` and `[ebp-0x10]`,
with corresponding reload order at +0x1372/+0x1375. No intervening instruction
accesses either home. All **3063 kb relocation destinations** and all function
locations remain identical. Do not describe all score-flat consumers as raw
byte-identical.

`test-ai-grail-boundaries.py` exercises actual helper source at native `-O0`
and `-O2`: **20,480 cases per form** check eligibility, ordered calls, artifact
fields/owner, coordinate flattening, friendly-cost ties, movement floors,
victory value and existing-vector-prefix preservation. Five deliberately
wrong controls must fail. All 24 lifetime forms and the reproduced selected
corner pass. A separate **2401-case** point-overload oracle covers signed
10/10/4-bit coordinates and unsigned-short-to-int result widening. The
fixture mocks services and does not certify VC6 ABI, EH or inlining. Default
execution checks current source; `--source` selects a snapshot, and
`--all-forms` requires the pre-adoption lifetime-family anchors.

All **34 production objects** strictly reproduce the selected independent
repeat, including section bytes, function locations and relocation
destinations. Full retail delinking/build passes at **4084/4764 exact**,
**96.43% linked** and **96.42% whole-image** (rounded). One checkpoint rises;
no source edit lowers MAX and no banked RVA is lost. The census is **214
inline overrides** (209 depth-zero / five auto-inline-off), across **28 TUs**,
and **63 unions**. Neither Grail pin remains, and ai_player has no inline
override left. This is not a claim that its source or remaining functions
are complete.

### Scenario deselection: ordinary body ownership, not an inline fence

`generate-scenario-deselect-family.py` crosses the base handler's current
header definition / recovered ordinary definition with the scenario caller's
existing fence / natural call. The source fact is **window.cpp:1122/1123,
dc 0x197f48**, not the existence of a folded retail body. Dreamcast's public
`?OnWidgetDeselect@CHeroWindowEx@@MAAHHAA_N@Z` also proves protected virtual
access and `bool&`; five override publics carry the same reference type.
Complete's custom-campaign override shares the independently proven slot 12.
Restore that entire interface, including WindowHandler's local flag and
reference call, before taking the search checkpoint. This avoids scoring
newly mangled methods against stale names.

The interface-only full build migrates six claimed symbols with every score
unchanged. Across all 152 raw objects, **9949 executable sections** retain
their bytes and function locations; relocation names change only by the
seven explicit callback signatures and existing anonymous-namespace nonces.
The herodefs zero-filled BSS's anonymous-static permutation changes its
padding/extent (1156 to 1152 bytes), not executable instructions.

Context `ea8854cffaf04c674e5d` exhausts **four successful source states** and
produces **three distinct whole-consumer code identities**. It scores all
**90 configured window.h consumers / 3964 rows**, determined from the fresh
Ninja dependency closure. All three retained identities reproduce their
scores and code independently; the watched authored source stays unchanged
throughout the run. The scenario callback at 0x5698a0 gives:

| Base handler definition | Existing fence | Natural call |
|---|---:|---:|
| Current implicit-inline header body | 100% | 58.25% |
| Ordinary window.cpp body, in original source order | 100% | 100% |

The ordinary body's two caller forms produce identical objects. The chosen
natural call preserves all 30 retail instruction bytes and the sole call at
+0x15 (relocation +0x16). Its callee emits `33 c0 c2 08 00`, independently
identical to the five bytes at retail 0x559140. That address already belongs
to `ResourceManager::t_stdio_file_adapter::write`; retain the existing single
claim, with an unannotated definition for the folded base handler.
The same body need not remain implicitly inline merely because ICF selected
another owner. Its source position is after WindowHandler and before
GetRolloverWidget.

Shared-header collateral is small but real: quest-creature `generate` in
rmg goes **99.7349% to 100%**, and `CEnterNameEdit::onKillFocus` goes **100%
to 99.871%**. Both already have MAX/HIST 100%; their own source hashes are
unchanged. Every other scored row is unchanged. The RMG change swaps two
independent loads at +0xca/+0xcd; the focus handler exchanges EDI/ESI spill
homes and their corresponding reload order. kb's score-flat `oldmain` also
changes four bytes, exchanging EDI/EBX spill homes at +0x129a/+0x129d and
reload order at +0x1372/+0x1375, without an intervening access to those homes.

The raw-object review accounts for **13,116 retained sections and 94,778
relocations**. Apart from those twelve instruction bytes, section bytes,
function offsets and named relocation destinations agree after the observed
section-index translation. Twenty consuming TUs stop emitting the base's
header COMDAT and associated `.debug$F`; window.cpp retains the same body at
its proper source position. Compiler-generated EH/branch labels are compared
by their actual section/offset destinations, including the four renumbered
`hero_rollover`, `town_rollover`, `generic_help` and `ignore` labels. No new
score normalization or second symbol ledger is involved.

`test-scenario-deselect.py` extracts the actual base, scenario callback and
WindowHandler bodies. At native `-O0` and `-O2`, **262,160 direct callback
checks** cover both initial flag values, all signed-16-bit IDs and four
outlying IDs; **864 dispatcher cases** check message priority, callback
arguments, flag initialization and end-dialog message mutation. Five wrong
controls must fail: wrong base return, wrong acceptance ID, dropped flag,
skipped callback, and wrong end-dialog return. The current interface and
selected frozen body both pass. This reduced fixture tests semantics, not
VC6 object layout or inlining; `--source` selects a frozen source tree.

The full adopted build passes every gate at **4084/4764 exact**, **96.43%
linked** and **96.42% whole-image** (rounded). All 4764 RVA rows survive;
the only two current-score changes are the collateral described above,
and no MAX/HIST value falls. All **90 production objects** reproduce the
selected independent recompile's executable bytes, function locations and
**94,779 relocation destinations** across **13,118 sections**. The only
raw non-executable layout difference is herodefs's zero-filled anonymous-
static BSS permutation/padding (1148 versus 1156 bytes). Fresh source and
structure diagnostics confirm all three scenario blocks and statements
agree; the named-call difference is precisely the independently byte-proven
ICF twin, not an absent call or a replacement helper. The census is **213
overrides across 27 TUs** (208 depth-zero, five auto-inline-off), with the
**63 unions** unchanged.

Integration with the independently landed ownership cleanup (`4b75e8ce`)
is separately full-built at **4086/4764 exact**, **96.44% linked** and
**96.43% whole-image**. Its two newly exact functions remain exact; all
incoming MAX/HIST peaks and RVA rows survive. Compared with that incoming
branch, only the same RMG/focus-handler current scores above move. Callback,
owner-payload, horde-row and DrawBolt native/negative controls pass on the
combined source. The original four-state experiment remains evidence for
its frozen pre-integration inputs, not a claim to have searched this new
header context.

## Lobby map-header receiver and dispatcher overrides

The dispatcher at `0x5887a0` formerly constructed a bare `NewSMapHeader`
under a depth-zero fence but never read the incoming message. DC source line
6529 calls the ordinary `OnNewMapHeaderInfo` helper, with its definition at
line 6968 (`0x140d50`). That older helper already calls `SetupOrigData` and
returns true; Complete's arm adds the serialized receiver and its lifetime.
Retail `+0x310..+0x365` calls the base constructor, constructs the header at
offset `+0x18`, installs vtable `0x641d30`, calls the receive bridge at
`0x512e00`, calls `SetupOrigData`, and destroys the header. The previously
modeled `CNewMapHeaderInfoMsg` owns exactly this base and member.

First restore the dispatcher's original public `QAA_N...AA_N` signature as
`bool handleNetMsg(CNetMsg*, bool&)`, including its caller's local and all
cancel assignments. The whole interface edit preserves every section byte,
function location and relocation destination across its four consuming TUs;
only the independently evidenced symbol rename is admitted. The full
checkpoint has no score movement before searching.



The two axes cross the incomplete/pinned arm with the ordinary complete
receiver, and the existing `HeaderRequested` auto-inline override with its
removal. All **four states emit distinct code and reproduce**; all **416
tracked functions across four header consumers** are scored. The receiver
with the adjacent override retained raises `HandleNetMsg` **89.7408% to
90.0449%**. Removing that auto-inline override scores **85.9113%** with the
old arm and **86.1394%** with the recovered arm. Both complete-receiver
states restore `CEnterNameEdit::onKillFocus` **99.871% to 100%**. No other
tracked score moves. The retained ordinary helper has no false `inline`
keyword, copied caller body or replacement override.

The recovered arm's first **62 raw bytes** (`+0x310..+0x34e`) match retail,
including its named constructor, receive and setup calls. The vtable and
game-global relocation spellings differ only by their existing source-owned
identities. Cleanup still expands two string `_Tidy` calls and
`~CMapHeaderData`, whereas retail calls `~NewSMapHeader`. Across the four
units, **811 existing executable sections are byte-identical**; the changed
ones are the dispatcher and its EH cleanup plus four spill/reload operand
bytes each in `onKillFocus` and the score-flat `kb::oldmain`. The latter
changes swap stack homes while preserving the corresponding reloads. The
ordinary helper adds its own body and EH cleanup, not a duplicate retail
claim. All four adopted production objects reproduce the selected candidate
across **1,448 sections and 15,089 relocation destinations**.

After adopting that model and running a fresh full build, the follow-up
verifies exact parent source/header identity and carries both reproduced
complete-receiver parents. It tests each of the remaining 14 dispatcher
depth-zero regions separately and all together, crossed with the adjacent
auto-inline override:



All **32 states produce distinct objects**, with **ten reproduced elites**.
All **223 TU score rows** are checked; only the dispatcher moves. No deletion
preserves 90.0449%: single-region deletions span **79.9839% to 88.9839%**;
all depth regions removed scores **41.8479%**, or **38.2972%** with the
auto-inline override also removed. No follow-up deletion is adopted. This
does not exhaust combinations of arbitrary depth regions or other recovered
helper models. Contexts `e95ac2ee39002ff3643c` and `5d5ccc52816c48107f9d`
identify the frozen four-state and follow-up inputs. Their generators are
historical pre-adoption controls and deliberately reject changed anchors.

`test-lobby-map-header.py` extracts the actual helper, receive bridge and
virtual-reader bodies. Its reduced fixture checks **1,920 cases** spanning
sender identities, short-message lengths, read results and exceptional exit
paths, preserving construction/read/setup/destruction order and the input
message. Six controls must fail: skipped receive, skipped setup, reversed
receive/setup order, wrongly rejecting a read failure, wrong header version,
and wrong helper result. Both the frozen candidate and adopted source pass
at `-O0` and `-O2`. This is not a retail ABI or complete wire-format test.

The full checkpoint passes at **4087/4764 exact**, **96.44% linked** and
**96.43% whole-image**, with every RVA and MAX/HIST peak retained. The fresh
census is **212 overrides** (207 depth-zero, five auto-inline-off), across
27 TUs, with **63 unions** unchanged. Remaining destructor/inliner and
flattened-helper differences are open reconstruction work, not TU closure.

## Lobby player helpers and the nested GetPlayer wall

The bounded wall below is superseded by [the exact dispatcher recovery](#lobby-dispatcher-exact-recovery).
Its measurements remain valid for the frozen, partially flattened source.

`generate-lobby-player-helpers-family.py` crosses three binary decisions:
restore both ordinary `getThisPlayer` calls, restore ordinary bool
`onPlayerDroppedMsg`, and remove the existing `HeaderRequested` auto-inline
override. The first two recover positive DC facts at caller lines 6488/6511,
with definitions at 7323/6937. Complete's dropped-player expansion adds version
recomputation and uses no-argument `update`; its older DC `message junk` local
is not copied into the newer implementation.



All **eight states emit distinct objects and reproduce**, scoring **416 rows
across four header consumers**. Dispatcher percentages with the adjacent
auto-inline override retained/removed are: old flattened control
**90.0449/86.1394**, transfer helper only **87.0841/83.4366**, drop helper only
**87.6740/83.4136**, and both recovered helpers **85.9516/82.2074**. The two
drop-only states also move `onKeyPress` from 99.8868% to 100%; that gain does
not justify retaining the flattened transfer arm. The adopted both-helper
state changes no other tracked score and removes three depth-zero regions.

The header consumers `advmgr` and `scenarioinfo` are wholly byte/relocation
identical to control. `kb::oldmain` has only four changed bytes: EDI and EBX
swap their -0x10/-0x20 spill homes and corresponding reloads. Its score is
unchanged. All four production objects strictly reproduce the selected
candidate's 1,449 sections, function locations and 15,092 relocation targets.

The extended `generate-lobby-dispatch-pins-family.py` accepts the completed
player-helper parent and verifies its exact authored source/header identity.
It retains both fully recovered parent corners and tests each of the eleven
remaining depth regions, plus all together, crossed with auto-inline removal.
All **26 states emit distinct objects**, and **ten elites reproduce**. All 223
TU rows are scored; only the dispatcher changes. Singles span **80.7489% to
84.8906%**; all-depth removal gives **60.9862%**, or **58.8618%** with auto-inline
removal too. No follow-up deletion is adopted. Frozen contexts are
`3a8df6d3328901340da9` and `1cb1c3134c84c99aa949`; the generators deliberately
reject incompatible future source anchors.

The passive trace reproduces **390,608 COFF bytes** outside the timestamp and
all **2,720 dispatcher bytes**. Caller cost is 1791 and initial budget 3582.
The two `getThisPlayer` expansions and the drop helper each expose `getPlayer`
at depth two: its cost 75 fits budgets **115, 114 and 118**. Retail keeps
these three calls at **+0x141, +0x160 and +0x22a**. The remaining drop callees
already stay out of line naturally: costs 61/138/122 exceed remaining budget
43. The map-header cleanup now retains `~NewSMapHeader` (cost 68, budget 57),
and its entire **80-byte** arm matches the retail instruction sequence.

The first unresolved boundary is therefore a nested-inlining/compiler-state
wall under the recovered source, not evidence against the helpers. DC lookup
rows 1157/1159/1160/1163 confirm its current loop, `i` local, found-pointer
return and null return. No supported missing invariant/lifetime was found;
adding source mass or a new suppression pin would not be recovery. Later
flattened handler boundaries and allocation choices remain open. This bounded
pass does not prove no future source model can improve the function.

`test-lobby-player-helpers.py` extracts the actual two helper definitions and
transfer arm. Its **146 cases** check local/network mode, null lookup, two
fresh query results, rejection, successful/failed transfer, cancel preservation
and dropped-player update order. Six controls fail: caching the first query,
omitting the version store, recomputing before deletion, skipping manager
notification, skipping cancel, and returning the wrong helper result. Both
frozen and adopted bodies pass at `-O0` and `-O2`; this is behavior testing,
not an ABI or inlining oracle.

The search never writes authored source, CUR, MAX or HIST. Different function
implementations must not be banked under an old source hash. Review a retained
candidate, apply the actual C++ change, then run `homm3 build` to regenerate the
normal checkpoint and README. This preserves `CUR <= MAX <= HIST` and keeps
historical peaks separate from the current implementation's MAX. Completing a
family or getting every currently tracked row exact does not prove whole-TU
source completeness.

## Lobby dispatcher exact recovery

`TSingleSelectionWindow::handleNetMsg` (`0x5887a0`, 2541 retail bytes) reaches
**100% from 85.9516%** using the recovered ordinary helper interfaces and
natural VC6 inlining. All eleven remaining dispatcher depth-zero regions and
the adjacent `HeaderRequested` auto-inline override are removed. This is a
function match, not closure of `singleselectionwindow`.

The `generate-netmsg-*-family.py` experiments exhaust 235 meaningful source
states, with successful unchanged/opposite-corner controls and reproduced
elites. Object counts below are within each context, not distinct across the
whole campaign. Header families score all four consumers (416 rows); local
families score the entire owning TU.

| Family | Frozen context | States | Objects | Reproduced elites |
| --- | --- | ---: | ---: | ---: |
| handlers | `b4ce1465164a776c371e` | 64 | 64 | 10 |
| counts | `199f00d289c8576af6df` | 64 | 64 | 10 |
| host | `32622357a6e52e423acc` | 4 | 4 | 4 |
| transfer-owner | `db87d2dd3025b5b79268` | 8 | 8 | 8 |
| access | `897bef01d8c7a53db259` | 64 | 8 | 8 |
| lifetimes | `8f4fee6ebe53302522fc` | 27 | 1 | 1 |
| delete-owner | `27626b887f8aeb6daeaa` | 4 | 2 | 2 |

Positive DC evidence restores the scroll, header-end, confirmation/request,
click and hero-handler calls, their bool interfaces and guarded lifetimes.
The initial handlers manifest accidentally changed the disabled ReceiveChat
carcass rather than its active definition; its score therefore does not
demonstrate the active chat early-return form. The follow-up corrects that
anchor and tests the active definition. Canonical text-resource `operator[]`
calls and protected manager query methods also survive, even where byte-flat.

The Complete-only header request is modelled as a manager operation, matching
the existing NewPlayer ownership pattern: the manager chooses a free slot,
constructs its specialization, publishes the pointer and starts that job.
`requestMapHeaders` is explicitly a provisional name/ownership inference;
the protected DC `GetFirstAvailable` signature corroborates manager ownership.
The chosen form retains the retail specialization constructor and all three
nested `GetPlayer` calls without pins, at 96.4597%.

Variadic `CChatManager::AddChat` and `PlayerDropMsg` retain their DC member
interfaces; stack-passed `this` does not imply a free function. The isolated
[VC6 ABI control](../vc6/variadic-members.md) verifies this. Restoring AddChat's
canonical `GetNextFreeMsgNbr` call also takes its own body from 96.6326% to
100%. Both host-query interfaces and the DirectPlay boolean storage match the
DC mangling and retail's byte-return body. Reply-construction, query-local and
sort-boolean alternatives in the 27-state lifetime family emit one identical
object; none is adopted merely to change the compiler budget.

The used selected-record binding in DeletePlayer is a **Complete-era lifetime
hypothesis**, not a recovered DC local. Direct indexing, pointer, reference
and const-pointer forms all reproduce its standalone body. The latter three
produce one identical caller object, retaining DeletePlayer and raising the
dispatcher to 99.9885%; direct indexing leaves 96.4597%. No dummy operation,
false inline declaration or invented assertion is involved.

The last 0.0115% is a semantic jump-table error: every one of the 85 instruction
blocks already agrees. Decode the byte index at function `+0x998` and the
dword table at `+0x914`: subtype 1045 (`RS_LAUNCHING_GAME`) selects entry
`+0x974`, destination `+0x795`, title 534; subtype 1082
(`RS_GAME_TRANSMIT_PENDING`) selects entry `+0x988`, destination `+0x6d4`,
title 731. The old source swapped these labels. Correcting just the labels,
preserving both bodies and their physical order, closes the match.

Native extracted-source tests pass at both `-O0` and `-O2`:

- `test-netmsg-transfer-owner.py`: 288 cases across the eight frozen forms,
  36 in the adopted source, five rejected negative controls.
- `test-netmsg-delete-owner.py`: 192 cases across four forms, 48 in the
  adopted source, five rejected controls.
- `test-netmsg-wait-arms.py`: 32 sender/cancel/lifetime cases, four rejected
  controls including the old swapped labels.
- `test-chat-member-interfaces.py`: 12,288 ring/format/flag/sound cases,
  six rejected controls.
- Existing player-helper and map-header tests: 146 and 1,920 cases,
  six rejected controls each.

These are reduced behavioral fixtures, not x86 ABI or complete protocol tests.
The full VC6 build passes every gate at **4091/4764 exact**, **96.45% linked**,
**96.44% whole-image**, with 193 depth-zero regions and four auto-inline-off
regions remaining project-wide. Current collateral is recorded, not hidden:
`handleLowLevelMsg` falls 100% to 82.7580% after the member-interface recovery;
its call stream agrees but frame/cleanup/deque lowering differs. The unchanged
`CEnterNameEdit::onKeyPress` recovers 99.8868% to 100%, while `onKillFocus`
moves 100% to 99.8710%. MAX/HIST retain both collateral 100% peaks.

## Palette channel lifetimes from DC line layout

The palette constructor families show why a compiler-sized arithmetic residual
can originate in the lifetime of a result farther down the expression. DC's
ordinary `Convert24to16` helper has three separately attributed channel
calculations at lines 224–226, each truncated to a word, then OR/store at 228 and
destination advance at 229. Named `unsigned short` channel values reproduce the
byte-wide shift-count hoists in retail callers `0x5226d0` and `0x522770`, taking
94.8548% / 94.9365% to 100%. Casting each channel inside the combined expression
leaves those older scores. The all-int helper signature and ordinary helper
boundary remain intact; no parameter narrowing or inlining directive is needed.
That 72-state family produced 20 distinct objects.

The mask constructor `0x522810` similarly separates channel lines 101–103 from
OR/store at 108, with four intervening unrecorded lines whose contents are
unknown. This supports a channel-lifetime hypothesis without inventing source
text. A 73-state scale/operand/pointer-order family produced six objects and
reached 99.2676% from 98.9155%, but retained the wrong channel evaluation order.
A subsequent 61-state lifetime family produced eight objects, all reproduced:
indexed RGB reads, named `unsigned int` channel values and an early destination
pointer close at 100%. Both ordinary and const channel locals work. Advancing
the source pointer or narrowing the channel locals to words does not close.
All 25 tracked palette functions are exact together with the selected forms.

The matching source also restores the DC `const TPalette24&` constructor
interfaces and its native `unsigned char Palette[768]` member. Update every
caller when correcting such interfaces: the pointer-taking `TPalette24` copy
constructor otherwise permits an old pointer argument to create an unintended
temporary before reference binding. Full-build caller scores confirm unchanged
current results after the calls are corrected.

### Recovering video callers with canonical header helpers

The video-caller experiment compared the sound guard, resume guard
and pause-draining loop while retaining the ordinary `videoSoundOnOff`,
`videoResume` and `videoClose` calls. The shared `serviceSounds` body stays
inline in its proven `soundmgr.h` owner. The PC retail bodies prove the
four-handle sound predicate, zero-count protection, decrement-before-resume
transition and sound/Smacker/Bink close order; Dreamcast's port stubs provide
only declarations and source order for these video functions.

Against PR #3's `03a62eeb`, context `b675e964de23c1bd96c6` exhausts 48 source
candidates, produces 33 distinct emitted objects and reproduces ten retained
candidates. The adopted `1ca50e84d3aa84df22bc70b1` combines the sound guards
into one service call and the resume early-outs into one short-circuit guard.
Its production object reproduces the normalized candidate's complete code
and named relocation identity. The stricter raw COFF comparison also agrees
on all 125 sections, 936 relocation destinations and function locations.
All 30 tracked rows are compared:

| Function | Before CUR | Recovered CUR |
| --- | ---: | ---: |
| `videoClose` | 7.6923% | 38.1538% |
| `showVideo` | 39.1274% | 67.8147% |

Every other tracked score holds, including the two edited helpers. Neither
caller body changes. The sound-guard-only control recovers `videoClose` but
leaves `showVideo` unchanged. The resume-guard-only control gives `showVideo`
73.5019% but leaves `videoClose` at 7.6923%; the combined candidate recovers
more fuzzy-weighted retail bytes across the two functions. No new exact
function is claimed. The historical 100% peaks remain recovery leads.

The named sequence still shows two expanded `serviceSounds` operations in
`videoClose` where retail calls them; the final `closeBinkVideo` tail call
and the four indirect video-library calls agree. The current source-labelled
`showVideo` comparison first diverges at its first `videoClose` site. These
are remaining inlining/context differences, not grounds to move the sound
helper back into a `.cpp`, flatten the ordinary video helpers, or add a pin.
The finite guard/loop family bounds this PR's recovery; it does not close
`smackmgr` or recover every peak lost through header ownership.

`homm3.vc6.test_video_recovery` imports the actual three helper bodies and all
48 generated combinations. An independent transition/effect oracle covers
all 16 handle-presence masks, pause depths 0–5, both initial pause states and
all three entry points. Five negative controls reject missing sound service,
a wrong Bink pause argument, a missing pause-state clear, a missing Smacker
close and reversed close order. The host fixture validates these state and
call-order contracts; VC6 remains the byte verdict. The generator accepts
the reviewed adopted body as its unchanged control and retains the original
alternatives, rather than silently applying stale anchors.

The final full build, including fresh retail delinking, passes all gates and
raises executable matching from 94.81% to 94.83%. Exactly two CUR rows rise;
none fall. The 4,784 canonical definitions retain zero ownership violations,
and the existing 200 inline-depth pins are unchanged. Both edited helper
bodies remain at CUR 0%; their new source hashes reset MAX from 100% to 0%,
while HIST remains 100%. All 4,752 ledger rows and every historical peak
survive; the unchanged caller bodies retain their own MAX 100% peaks.

### Save recovery bounds after header ownership

`generate-save-local-recovery-family.py` uses the `game::Save` Dreamcast
local roster and retail narrow writes to test byte-buffer reuse/scope,
unsigned-word staging, the signed map-extra size, loop-index declaration
lifetime and the native-bool vector writer result. Context
`0db47607a4972b3ef94c` exhausts 48 states, emits six distinct objects and
reproduces ten candidates. `game::save` ranges from 59.2838% to 59.6005%
against 59.5944% unchanged; every other game row holds. The tiny gain does
not recover the `SavedGameHeader::reset`/`save` calls or the retail frame,
so none of these alternatives is adopted.

`generate-save-header-placement-family.py` separately compares the existing
class location, moving it beside the late inline definitions, and embedding
those same bodies inside the class there. Field order, signatures, body
operations and `game.h` ownership stay fixed. Compiler dependency records
select all 64 affected TUs. Moving the class alone is score-neutral; embedding
the methods does not improve `game::save` and slightly lowers two unrelated
rows. The existing class structure is retained. Context
`183778ec3eb1bc6b45bb` scores all three states, produces two distinct objects
and reproduces both retained candidates against a fixed snapshot. A prior
run rejected its final checkpoint after concurrent source edits and is not
counted as a completed search.

### Spell obstacle appends, filter widgets and sound service recovery

These searches start from PR #4's `ecca3a3d` full-build checkpoint and retain
canonical source ownership. The spell and initial filter generators require
that source checkpoint; replay them in a checkout of it with these experiment
scripts available. Their exact anchors deliberately reject a changed body.
The filter refinement also checks the parent snapshot and reproduced objects.
Sound variants accept either reviewed body as the unchanged control.

| Family | Scored source states | Distinct emitted objects | Reproduced retained candidates |
| --- | ---: | ---: | ---: |
| Four obstacle append boundaries | 16 | 16 | 10 |
| Filter allocation/loop lifetimes | 54 | 36 | 10 |
| Filter pointer conversion/refinement | 61 | 61 | 10 per generation |
| Sound service locals and guards, 51 dependent TUs | 30 | 14 | 10 |

`generate-spell-obstacle-append-family.py` tests the four source-proven
`push_back` sites in `combatManager::castSpell`. DC spells.cpp lines
849/925/962/996 supply positive call evidence. Three SH4 sites load the named
callee before the line block containing the indirect `jsr`: inspect
0x14feaa, 0x1500be and 0x1502c0 as well as Force Field's direct attribution
at 0x150212. The dossier's local call attribution alone misses those names.
Retail keeps count-insert bodies at +0x64e/+0x86c/+0x9a2/+0xae4; the native
vector's `push_back` supplies this nested call boundary. All four restored
calls reproduce 93.9814%, up from 72.8210% and above HIST 93.3687%, in context
`034c7ecf3f258f5bbaef`, candidate `eb22de398dcc8adf2e3cd0bf`. The complete
obstacle locals, append-before-slot order, and original-record arguments to
`placeObstacle` are unchanged. No container replacement or new helper is used.

`generate-filter-widget-recovery-family.py` exhausts 54 allocation-result,
button-binding, public append API and loop-scope choices in context
`3916b52e69a9a5b42b8c`. `generate-filter-widget-refinement-family.py` then
adds real widget-pointer conversions and a vector reference to ten reproduced
parents, deduplicating to 61 choices in `88fa5370dc9a8dba6348`. Candidate
`b6b839d0becb8fff58c7d6ce` reaches 92.9883% from 84.8995%. It retains all
constructors, the direct highlight field store and the proven disabled-frame
helper. The retired highlight setter is not restored. The only sibling
movement is `CEnterNameEdit::onKeyPress` 100% -> 99.8868%, one matching byte;
its source is unchanged, so MAX and HIST remain 100%. The best alternative
with no sibling movement reaches 92.7301%; the selected source recovers more
retail bytes overall. These are retail-only filter controls, so no Dreamcast
local roster is claimed for the inferred pointer lifetimes.

The independent native filter oracle checks all 56 widget identities,
constructor arguments, defaults, display fields, order and array identity,
with both fresh and reserved vectors and three preexisting-widget counts.
All initial and refined candidates passed before adoption. The permanent
`test_filter_widget_recovery` imports the current builder and canonical
`setDisabledFrame` body; five negative controls reject wrong highlight,
disabled frame, defaults, missing append and widget ID. This validates UI
construction behavior; it is not an x86 ABI model.

The sound-service experiment took all 51 header-dependent TUs
from compiler dependency records. Context `bf4bbe51a427f361ce69`, reproduced
candidate `7173bdb9b7e2604dc1808b27`, captures the stream after `AIL_serve` and
nests the three real state guards. `serviceSounds` stays inline in SoundMgr.h
and its retained body stays exact. DC 0xe6ef4 is a WinCE stub proving source
ownership; the nonempty PC behavior comes from retail 0x59a7d0. `showVideo`
rises from 67.8147% to 94.1120%, with every other tracked score holding across
the 51 units. The unchanged combined guard is the negative byte control.
The native test imports the current body and all 30 variants. It checks lock
receiver identity separately from the global manager, stream changes during
Miles service, state guards and call order; six negative controls fail.

The generators preserve real helper boundaries and operations. No inline
pins, release VERIFYs, synthetic caller weight, or duplicated helper bodies
are introduced. Remaining mismatches require further evidence; these finite
families do not establish TU closure or exhaustion of the wider recovery queue.

The adopted production objects match the reproduced candidates in section
bytes, relocation destinations and function locations: spells 170 sections /
1,979 relocations, singleselectionwindow 836 / 7,378, smackmgr 125 / 939.
The final full build and fresh delink raise executable matching from 94.83%
to 94.98%, with 3,937/4,751 current exact functions. Exactly three CUR rows
rise and only the keyboard-handler byte falls. All 4,752 ledger rows and all
historical peaks survive. Ownership remains 4,784 definitions with zero
violations, and the existing 200 inline-depth pins are unchanged.

Post-adoption source/call inspection confirms the four obstacle count-insert
calls. `castSpell` still has a 0x80 frame against retail's 0x94 and different
shared spell-effect tails. The filter's first remaining difference is the
vector-base load before its first allocation and the append argument's stack
slot (-0x24 versus -0x20). `showVideo` now retains both early sound-service
calls, as retail does; its later expanded `videoSoundOnOff` still leaves a
`serviceSounds` call where retail calls that ordinary helper. Source-labelled
comparison first differs at the Smacker-handle guard. These specific residuals
remain recovery leads; equal call totals are not evidence of equal boundaries.

### Restoring map readers and the enemy-marking implementation

The next recovery starts from PR #4's `30ecbab7` full-build checkpoint.
`NewfullMap::readObject` had four Dreamcast-proven ordinary readers flattened
into its switch while their named definitions remained inactive stubs.
Restore `readBoatData`, `readHolyGrailData`, `readShrineData` and
`readShipyardData` in mapcell.cpp at their original source positions, with
ordinary declarations using the PC `TAbstractFile` stream. DC 0xed984,
0xedd14, 0xedde8 and 0xefe28 prove the signatures, locals, reads and status
returns; DC readObject calls them at source lines 3350/3379/3388/3406.
Retail's corresponding arms prove their inline expansions, field layout and
character conversions. Complete defers the old shipyard terrain scan to
`loadShipyards`. Grail/shrine retain their final short-read checks in the
helper; the caller discards status, so those final comparisons disappear
naturally in the retail expansion.

`generate-map-reader-helper-family.py` records the initial two-state byte
control in context `a70571d26b1e09d9f9f3`, comparing all 76 header-dependent
TUs. Both objects reproduce; candidate `6835a1c0264f821a3d5faaff` raises the
reader from 56.6382% to 60.0594%. Restoring the helpers also exposes an older
placement error: the existing inline `CObject::getTrigger` (DC line 1119)
sat before line 1095's reader. The final source moves it between
`getObjectTypePtr` and `findTrigger`; its body and declaration are preserved.
That mandatory source-order correction is score-neutral, and the next full
checkpoint passes the ownership gate. Replay the initial generator at
`30ecbab7` with the experiment script available; the control itself predates
this separate order correction.

The host reader oracle imports both the recovered helper source and the
actual four caller arms. It covers all 256 byte values and every short-read
boundary, including partial shipyard state, boat arguments, read sizes/order,
and discarded caller status. Five negative controls reject wrong boat owner,
grail radius, shrine value, boat coordinate and status. This is an effect
oracle, not a replacement for VC6 layout or byte validation.

`generate-quest-guard-append-family.py` then tests 36 source choices at the
corrected full checkpoint (`376ad64f07b280fd969d`): DC's function-scope
read-count local and five separate read/result-test statements, plus actual
quest-vector iterator/reference lifetimes and public append spelling.
Twenty distinct objects and ten retained candidates reproduce. Candidate
`159e2077aa36cf6abc2d1005` keeps the count local and uses `push_back` in the
QUEST_GUARD arm, reaching 60.9729% without any sibling movement. The count
alone is byte-flat; preserve its positive source evidence. Named vector
references and inline data-end expressions add no gain. Both insertion
workers still expand where retail keeps two-argument calls, and the seer
constructor still expands: this is partial recovery, not a closed boundary.
The native quest-arm oracle imports the actual constructor and reader and
checks append-before-index behavior, record copy, existing vector contents,
null quests and optional data registration across empty/reserved vectors.
Every explored arm passes; five wrong controls fail. The generator accepts
an adopted member only after an exact round trip through its finite family.

The active `searchArray::markEnemy` body was a more direct failure: an
integrated carcass stub replaced the existing implementation. The earlier
source still contains the real body after its inactive reference stub, and
DC 0xa0a44 independently proves get_hex, the nested minimum-cost guard,
valid-move flag and unsigned-short cost store. Retail contains the matching
expansions in both callers. Restoring that one ordinary body in place gives:

| Function | Before CUR | Recovered CUR |
| --- | ---: | ---: |
| `findCombatPath` | 50.9545% | 92.2920% |
| `markTeleport` | 71.6135% | 100% |
| Retained `getHex` | 0% | 100% |

`generate-mark-enemy-restoration-family.py` exhausts the two-state control
in `8a71980a4ee16edd6844`; both objects reproduce. The adopted implementation
is candidate `10a61ce51f0271cdda248e93`. The native oracle imports its actual
body and covers existing flags, cheaper/equal/dearer costs, negative and
32-bit boundary costs, unsigned-short narrowing, neighboring-cell preservation
and accessor ordering. The empty-body control and four incorrect controls
fail. Retail comparison verifies the four retained getHex calls inside
findCombatPath's mark expansions; teleport marking and the retained accessor
are exact again. The previous combat-path peak is fully recovered.

An audit of all 4,784 active definitions at the original checkpoint found
only this explicit active stub. The source-ownership gate now rejects
`ACTIVE-STUB` even when ownership and signature are correct. It enumerates
active definitions through the AST, then checks placeholder comments while
ignoring string/character literals. Tests cover inactive stubs, unannotated
source/header bodies, actual empty constructors and literal marker text. The
new gate also rejects the real pre-fix markEnemy snapshot as a negative control.

### Crossover lifetime recovery bound

`generate-crossover-lifetime-family.py` tests the current retail-only
`SCampaign::pruneCrossoverHeroes` body, preserving the repeated inflated-size
read after the virtual pool query, signed scenario count, canonical max,
artifact accessors, sort and vector assignment. Thirty-six combinations of
real hero references/pointers, scenario-vector access, sort endpoints and
front/begin access produce 24 distinct objects and ten reproduced retained
candidates (`98fcd19748a92c53c1fc`). None improves the original 20.6088%.
The caller stays unchanged. Its former five extracted phase wrappers have
no independent source evidence and are not restored to regain their score.

The combined map/path checkpoint passes the full build, fresh retail delink
and all gates: 95.04% executable matching (from 94.98%), 3,940/4,751 current
exact functions, 4,788 canonical definitions and zero ownership violations.
All 71 relevant ownership and native-oracle tests pass. The final mapcell
object matches its reproduced refinement object across 434 sections and
2,094 relocation destinations; findpath matches its restoration object across
54 sections and 238 relocation destinations, with function locations checked.

Five CUR rows improve. Two unchanged-source rows move with the four required
map-header declarations: `army::doAttack` 99.9424% -> 99.9040%, and
`advManager::doCombat` 98.5379% -> 98.1758%. Their MAX and HIST remain held.
`CEnterNameEdit::onKillFocus` returns from 99.8710% to exact. All 4,752 ledger
rows and all historical peaks survive; no unchanged-source MAX is lowered.
The existing 200 inline-depth pins are unchanged. These results preserve
source structure through measured header collateral and leave the wider
map/campaign recovery queue open.

### Campaign read buffers and map-save result lifetimes

The follow-up starts at `028e1b09` with a clean, full-build checkpoint.
`SCampaign::load` is Complete-only: the full Dreamcast roster has no matching
procedure. Retail's six unsigned counts/identifiers use dword loads followed
by masks after one- or two-byte reads. That is not proof of an int source
buffer. `generate-campaign-load-buffer-family.py` exhausts 64 independent
width combinations in `906a2aed32d636cfd8eb`: two distinct emitted identities,
both reproduced, and **no score changes in any tracked function**. Keep the
narrow buffers; replay this historical-width control at `028e1b09`.

The useful alternative is lexical lifetime. Retail gives the modern scenario
fields distinct scratch homes, and the previously removed synthetic scalar
helpers left isolated read/assignment blocks. The generator
`generate-campaign-load-lifetime-family.py` tests named locals in their real
enclosing prefix/loop scopes for four groups, plus separate days/score scopes.
All 32 states compile, nine objects reproduce in `20be28ec9c6caccae8c7`.
Candidate `3937da971249f1010a4be6b1` (`scopes-11010`) raises the loader from
50.8734% to **52.7064%**, with every sibling unchanged. It names the leading,
scenario and artifact buffers; count buffers keep their existing scopes and
days/score remain in the loop scope. It preserves the complete legacy arm.

The emitted modern artifact-pool and hero-pool shrinking paths now retain
additional vector size calls. The first two clear/erase workers, implicit
legacy array iterator, legacy string assignment and most nested size calls
still disagree with retail. The result is partial recovery; the historical
79.2531% remains a lead. No constructor body, scalar helper or pragma is added.
The native oracle imports the actual modern arm and every generated arm,
checks versions 28/35/36 for all 256 leading byte values, empty and populated
vectors, exact read-size order, campaign remapping, signed artifact/assigned
words and completion-flag widening. Five incorrect controls fail. It does
not claim host layout or legacy-construction coverage.

`NewfullMap::Save` has stronger source evidence: dc:0xecdf8 names function-scope
`int count` and assigns each ordinary helper result before its negative test.
The Complete seer/quest loops still belong in the caller. DC's static
`TSeerHut::SaveSeerList` (0x12d7e8) uses the global list and checks per-record
save results; retail uses the current map and ignores those results. This
semantic contradiction rules out restoring that older helper interface.

`generate-map-save-lifetime-family.py` exhausts 24 combinations of result
assignment, list-count sharing, unsigned loop-index scope and final failure
check in `a58ef16d0c060ff7bba4`. Sixteen objects are distinct; ten retained
candidates reproduce. The adopted `d929d6f793d422be39225cf3` restores the DC
result local and spells the final negative-result check explicitly, reaching
**35.5206% from 32.1267%**, with every sibling unchanged. Count assignment
alone is neutral; the explicit tail supplies the gain. Sharing the two
Complete list-count buffers is worse, and a shared loop index adds no gain.
The early list size queries and both event-list helper expansions remain
unresolved. The native oracle imports the actual driver and all 24 variants,
checks each helper failure, both layers, short/negative write results, ignored
quest-write and seer-save results, serialized counts, and list growth during
saving. Four incorrect controls fail. Both adopted-state generators require
an exact round trip through their finite family before admitting the body.

The combined full checkpoint passes all 149 units, fresh retail delinking and
every gate. Exactly the two intended CUR rows improve; all 4,752 ledger rows,
all historical peaks and every unchanged-source MAX survive. Executable
matching remains 95.04% at the displayed precision, with 3,940/4,751 exact
functions, 4,788 canonical definitions and the same 200 existing pins.
All 73 relevant tests pass. Production customcampaign and mapcell objects
match their reproduced candidates in executable section bytes, relocation
destinations and function-symbol locations: respectively 391/312 sections,
1,930/1,862 relocations and 344/274 function locations. The broader recovery
queue remains open.

### Bink ownership and the misplaced obstacle-insert claim

The Bink Dreamcast bodies are port stubs, but their declarations remain
positive source evidence. Raw publics establish namespace ownership: Bink
functions use `YA` and globals use `3`, versus class-static `SA` and `2`.
Restoring the namespace preserves all seven function bodies and their
relocation targets across two reproduced states and four consuming TUs.
Fresh delinking preserves all seven 100% scores and every current-source MAX.
OpenBink and VideoOpen raw publics encode their final flags as `_N`
(bool), and both video-state queries return bool. Restore that coupled
interface, including the Windows ShowVideo worker's byte-valued flags,
and the three Bink Boolean globals. Raw GetBinkFilePtr also proves char*;
its descriptor pointer fields follow that callee without a const-removing
cast. Four Boolean states and two filename states reproduce across all
72 consumers. Each complete correction preserves all 378 function sections
and relocation graphs checked in the five direct consumers. ShowVideo's
new source identity resets MAX to its unchanged 60.4981% CUR, retaining
HIST 100%; no emitted instruction match is lost.
Two full ownership checkpoints preserve every current score. Five function
names migrate by retail RVA, preserving their historical peaks. Own-source
hash changes reset the frame pump's and VideoClose's current-source MAX to
CUR; their historical 92.9245% and 100% peaks remain available.

All seven admitted Bink functions now match retail. Restoring the canonical
playback aggregate and operation order closes playBink; the reviewed Windows
sound-service visibility restores getBinkFilePtr's retained calls. Publishing
the readiness predicate in the actual `g_needsUpdate` byte closes nextBinkFrame's
idle-return and shared-epilogue layout (see behavior-catalog D5). The pump
search generator and disposable behavioral fixtures are retired; Git retains
the earlier experiments. The unlocated three-argument DC `setPixelFormat`
interface remains visible in the source carcass, not counted as a retail match.

A separate emission audit found that 0x46aeb0 was incorrectly claimed as
`objecttype`'s `vector<TImageInfo>::insert(ptr, count, const&)`. Its actual
retail callers are `combatManager::placeObstacle` (0x466010) and `castSpell`
(0x59fe30), operating on the manager's TObstacle vector. The function also
precedes cmbtmgr's native `_Ucopy`/`_Ufill` cluster. Both cmbtmgr and spells
already emit the native TObstacle specialization: all **740 retail bytes**
agree outside two relocations, whose operator new/delete destinations also
agree. Twelve following alignment bytes are outside the admitted extent.
The identical 24-byte stride had made TImageInfo a misleading proxy.

Move the existing claim to cmbtmgr's `VECTOR_INSERT_COUNT, TObstacle` without
changing C++ operations, adding an instantiation, or inventing a source call.
Fresh delinking restores the native body to **100%**, preserves the old peak
by RVA, and lowers generated emission debt from 39 to 38. The full adopted
checkpoint reaches **95.08% executable matching**, 3,941/4,751 exact functions,
4,788 canonical definitions and zero ownership violations; the existing 200
pins are unchanged. This fixes a type/owner error rather than forcing the
image-cache overload to emit. The Bink inline-boundary residual is resolved by the reviewed Windows
sound-service definition described below.

Final validation passes all 75 ownership and native-oracle tests. The fresh
retail comparison for 0x46aeb0 has 55 exact blocks, matching branches, calls
and relocation destinations, and equal masked assembly. The RVA-based ledger
audit retains all 4,752 rows and every HIST peak: this batch has one CUR gain
and no CUR declines. Restoring Bink member names changes the own-source hashes
of `nextBinkFrame` and `videoClose`, resetting their MAX values to their
unchanged current scores (0% and 38.1538%); their 92.9245% and 100% HIST peaks
remain available. Every unchanged-source MAX is preserved.


## Sound definition placement and shared native library bodies

The former in-class/out-of-class header placement family was byte-flat for
tracked functions and is retired. Raw DC records establish an empty CE body
at SoundMgr.h:140, but no Windows lexical inline or definition location.
The full retail import census identifies two complete service expansions,
both in soundmgr, and a retained 81-byte body; all external consumers call it.
An actual VC6 PCH control leaves every Bink function unchanged.

The reviewed Windows model uses one ordinary definition in soundmgr.cpp.
This is an explicit platform-placement inference, not recovered Windows text.
The CE stub and Windows body have separate exact catalog entries, preserving
the known CE origin. Three visibility states across all 51 consumers produced
three reproduced objects; the ordinary definition matches the retained body
and both expanded callers without emission tricks or inline pins. See the
owning source comment for the retail anchors and limits of this inference.

The next emission audit finds retained canonical library bodies in other
real consumers. A source enrollment in a TU that has ceased emitting a
COMDAT does not mean the shared library definition is absent everywhere.
Move only the enrollments, retaining the existing native definitions and
calls. Retail callers and raw bytes establish these identities independently
of the matching score:

| Retained body | Emitting consumer | Retail evidence |
| --- | --- | --- |
| String assign from pointer/count, 0x404150 | advmgr | Hero assignment and getArmyHelpText call it, as does SendChat. All 161 bytes agree outside four matching named calls. |
| Vector size, 0x517750 | objecttype / TImageInfo | Image-cache insertion and setupAndLoadObstacles share the 24-byte element implementation. All 33 bytes agree with no relocations. |
| Single string-vector insertion, 0x4af350 | seerhut | Five quest dialogs, creatureBankEvent and text scrolling call the same 387-byte retained body. |
| String-vector destroy/copy, 0x4af500 / 0x4af800 | seerhut | The retained insertion calls these exact 77/56-byte specializations. |
| String fill/copy_backward, 0x4af870 / 0x4af9d0 | seerhut | The insertion's shift/fill arms call these exact 340/357-byte loops. |
| Mutable/const int copy, 0x5093c0 / 0x54df40 | rmg | The addObject costs worklist and BlackBoxData/RMG helper paths use the two overloads; both emitted bodies agree with all 37 bytes. |

For the 1,217-byte string-vector chain, every non-relocation byte agrees.
Allocation/deallocation and the retained _Construct/_Ufill targets match.
The size call at 0x4af4e0 names a folded vector<vector<hero> > representative;
its native string-vector size matches every byte. The remaining runtime/data
references are the invalid-position throw, overlap-safe memmove, npos at
0x63a60c and the empty string at 0x63a608. Inspect the actual destinations,
including the backward-copy arm and the zero/-1 data, before accepting their
differing generated labels. Do not add a type proxy, explicit instantiation
or a game call to force these COMDATs back into their former consumer.

The full checkpoint restores all nine rows from 0% to **100%**, reaching
**95.15% executable matching** and **3,950/4,751 exact functions**. Generated
emission debt falls from 38 to **29**. All 4,752 ledger rows survive, with
nine CUR gains, no CUR declines, no MAX resets and every historical peak
preserved. Ownership remains 4,788 canonical definitions with zero
violations, and the existing 200 inline-depth pins are unchanged. All 76
ownership and native-oracle tests pass, including the sound-service callback
and receiver checks.

Seven edited TUs retain their cached native code/relocation identities.
The cached RMG raw identity differs from its fresh compile, but recompiling
the frozen pre-edit source and headers reproduces the current object exactly:
the enrollment edits introduce no code change. RMG's pre-existing score rows
also remain unchanged. This control is necessary before attributing a cached
object difference to a comment/enrollment edit.


## Remaining bitset and scenario emission audit

The same native-consumer audit identifies four more retained bodies without
changing their canonical source definitions or adding callers:

| Retained body | Emitting consumer | Proof |
| --- | --- | --- |
| bitset<4>::test, 0x4cf960 | singleselectionwindow | Retail readMapPlayerSlot and setNewPlayerSlot share the 52-byte body and its _Xran call. |
| bitset<4>::_Xran, 0x4d1850 | singleselectionwindow | The feature-test and advanced-options paths call it; all 203 instruction bytes agree outside relocations. |
| bitset<70>::_Tidy, 0x4cfa10 | hero | Retail markArtifactSpells, loadMap and readTownData use the three-word fill/six-bit trim; all 37 bytes agree without relocations. |
| ScenarioStruct deleting destructor, 0x488eb0 | campaignbrief | Its scenario delete loop emits the 33-byte wrapper shared by retail CampaignHeaderStruct::load and selectCampaign; both calls agree. |

The _Xran proof includes exception metadata. Both implementations install
a two-state unwind map, destroy the temporary string at EBP-36 and the
exception at EBP-64, and use the same invalid-bitset-position message,
out_of_range throw information, vtable and five ordinary calls. The two
native `__except_list` relocations resolve to the retail FS:[0] offsets.
Retail's string cleanup at 0x4fca60 is folded with TreasureData's destructor;
the native string destructor has identical blocks, instructions and its
operator-delete target. The three unreachable pops after throwing remain
inside the admitted 203-byte extent. Relocating the enrollment preserves
this metadata ownership and the canonical <bitset> definition.

The byte-vector fill at 0x48db70 also has a byte-identical native copy in
rmg_terrain. That copy already represents the separate retained 0x5b8060,
so it cannot recover another row by taking over that enrollment. Keep both
retail identities and leave the campaign copy's emission debt visible.
Likewise, no other TU currently emits the matching TSeerHut/university resize
or map-hero/university copy specializations; matching a same-stride unrelated
type would not establish their source identity.

The full build restores all four rows to **100%**, reaching **95.17% executable
matching** and **3,954/4,751 exact functions**, with **25** unpaired generated
enrollments. Every one of the five edited TUs retains identical native code,
function locations and named relocations. All 76 ownership/native-oracle tests
pass. The RVA audit has four CUR gains, no declines or MAX resets, and retains
all 4,752 ledger rows plus every historical peak. Ownership remains 4,788
canonical definitions with zero violations; the 200 existing pins are unchanged.


## Aggregate argument materialization in Voronoi

A source-equivalent local can change VC6's aggregate stack allocation even
when the named call sequence is unchanged. In `buildVertices` (0x5fdb40),
const-reference dot operands move the original 87.7394% caller to 93.7676%.
Materializing only the opposite site, with circumcenter parameters ordered
third/origin/second, removes an eight-byte temporary and reaches 97.5070%.
The frame is now retail's 0x78 bytes. All other scored functions retain their
scores, including the five exact arithmetic operators.

The 64 ownership states emit 61 distinct objects; the 48 site-evaluation
states emit 24. Reproduced controls and native integer/ring fixtures support
the change. This remains a partial match: input coordinate scheduling and
one output-coordinate reload still differ. Identical helper calls, frame size
and most arithmetic instructions are insufficient to claim byte exactness.
Keep operator-body construction and caller materialization as separate axes:
changing an already-exact retained operator can affect its inline expansion
and regress siblings without explaining the caller's residual.
