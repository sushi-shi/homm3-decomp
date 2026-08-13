# HoMM2 ↔ HoMM3 symbol overlap

**Verdict: HoMM3 is a direct source-level descendant of HoMM2's engine.**
More than half of homm2's compilands recur by name in HoMM3, the NWC
`basewin.lib` UI framework and the game-core class roster carry over
nearly wholesale, and 593 distinct function names are shared across the
two games — since the 2026-08-06 buka repoint, **every one backed by an
exact-matched VC6 template**. The reuse is source-level with heavy
evolution, NOT byte-level: cross-compiler/cross-era code identity is
infeasible (measured below), but homm2's matched C++ is a ready
translation-template library for homm3 matching, and rare-string anchors
name 50 of our still-unnamed functions.

Measured 2026-08-04 by `homm3.analysis.homm2_overlap`; revived
2026-08-06 from `scripts/archive/` as the live dual-branch generator
(decision log §5). Inputs: our pinned HEROES3.EXE (sha256 `057c9d88…`)
+ symbol db + Dreamcast/NH3API corpora; BOTH homm2 trees read-only —
PoL 2.0 (`$HOMM2_DECOMP`, HEROES2W.EXE sha256 `bc8f362d…`, MSVC 4.2
`/Gr`, CodeView-NB09 authoritative names) and Gold 2.1/Buka
(`$HOMM2_BUKA`, HMM2PL.exe sha256 `bc7e9c93…`, VC6 SP5, 1,727/1,727
exact). The functions lane is buka-preferred: when both branches carry
a name the row is buka's (`h2_branch=both`, pol's score kept in
`h2_fuzzy_pol`); the other lanes stay pol-driven as first measured.

## Lane results

| Lane | Result |
| :-- | :-- |
| TU stems | **53 of homm2's 95 units** recur as HoMM3 DC compilands (advmgr, army, armygrp, findpath, fly, game, hero, recruit, soundmgr, strip, swapmgr, townmgr, window, …) |
| Classes | **43 exact class-name matches**: `advManager`, `army`, `armyGroup`, `baseManager`, `border`, `button`, `combatManager`, `executive`, `font`, `game`, `hero`, `heroWindow`, `heroWindowManager`, `hexcell`, `mouseManager`, `resourceManager`, `soundManager`, `town`, `widget`, … (the whole basewin family + the game core) |
| Function names | **593 distinct normalized-name pairs** (dual-branch: 591 `both` + 2 `buka`-only; the earlier 611 counted overload duplicates); **all 593 at homm2 fuzzy = 100.0** via buka; **173 with a known HoMM3 retail RVA** (via the DC name map); `h2_arity` carried from buka's signature table |
| String anchors | **57 rare-literal anchors** (literal referenced by ≤3 fns on each side); 50 land on `working-label` (unnamed) HoMM3 functions |
| Boost list | **220 rows** in `boost.csv`: 49 `string-anchor` + 171 `h2-source-template` (buka's 100.0 scores widen the template tier) |
| Renamed twins | **12 `twin-strong` + 19 `twin-candidate`** proposals from `homm3.analysis.h2_twins` over the 918-function unpaired residue (section below) |

## The two boosts

**1. Names for unnamed functions (`string-anchor`, 50 rows).** A shared
rare literal pins a fn↔fn pair with no name needed on our side. Flagship
example: HoMM3 `game_smove_82m_3d9f0` / `game_skill_82m_166c20`
(working labels) share the creature-animation resource format strings
`%sattk.82M` / `%smove.82M` / `%skill.82M` / … with homm2's
`army::LoadResources` (`SOURCE/ARMY`, 100% exact) — those HoMM3
functions are the army/creature resource loaders, and the `.82M`
extension surviving into HoMM3 is lineage evidence by itself.

**2. Source templates for matching (`h2-source-template`, 148 rows).**
HoMM3 functions we already NAME (DC evidence) whose homm2 twin is
exact-matched — homm2's C++ is the starting body for our
reconstruction. Examples: `combatManager::ResetLimitCreature`,
`combatManager::UpdateGrid`, `combatManager::ViewArmy`,
`Process1WindowsMessage` — each with homm2 fuzzy 100.0. Whole units at
homm2-100% with many HoMM3 counterparts: `BASE/WINDOW`,
`SOURCE/SWAPMGR`, `SOURCE/STRIP`, `BASE/soundmgr`, `SOURCE/RECRUIT`,
`SOURCE/ARMYGRP`.

## Calibration: why there is no byte-identity lane

20 name-paired functions compared across MSVC 4.2 → 6.0 and three years
of engine evolution: size ratios span **0.26x–2.10x**, call counts
drift freely. One striking outlier — `combatManager::GetCommand` is
1306 vs 1322 bytes with 11 vs 11 direct calls — but it is the
exception. Conclusion: do NOT build a masked-byte transfer lane against
homm2 (unlike the HD-Mod pressing, which is the same build); the value
is names via anchors and source via templates.

## Known false-positive mode

The name lane's negative control caught exactly one collision class:
homm2's `BASE/Bzip` exports the generic names `compress`/`uncompress`,
which pair with our **zlib** `compress.obj`/`uncompr.obj` — different
libraries, same generic C names. Units/anchors lanes are clean on all
negative controls (Bzip, Modem, Netbios). Treat short generic free-names
as untrusted without a second signal.

## Campaign status (2026-08-06)

After the buka repoint the exact-template universe is **593 pairs**
(was 466: buka's 1,727/1,727 exact turns every joined name into a
template). **173 are located** via `evidence/retail-dc-name-map.csv`;
live `VA()` claims in `src/` locate more - the generator only knows the
first source, so its own `h3_retail_rva` column undercounts; measure
against both. The `dc_bracket` homm2-traveled set (exact templates,
name-map-unlocated) grew 322 → 420.

The bulk-join shortcut does NOT exist: joining through
`evidence/retail-game-tree.csv` adds exactly zero. What does work is
**link-order bracketing** (`homm3.analysis.dc_bracket`, decision log
2026-08-06): inside a TU the DC and retail sequences are the same
sequence, so proven addresses cut it into gaps, and an equal-count gap
forces the mapping. That yielded 37 locations, 21 promoted to claims
here. The gaps bracketing cannot decide go to the **call-graph lane**
(`homm3.analysis.dc_callgraph`): R(F) must be among the retail callees
of R(F's proven DC callers), which yields 24 more unique locations
under an injective-monotone soundness check. Both lanes proved
themselves end to end - `inputManager::Close`/`::Main` and
`heroWindowManager::RemoveWindow` were located by them and then
matched.

## Renamed-twins lane (2026-08-06, `homm3.analysis.h2_twins`)

The exact-name join is blind to every function HoMM3 renamed. The twins
lane scores the **918-function unpaired residue** (354 with their class
present in the DC corpus) against same-class DC candidates only, with
three auditable terms — `S = 0.6·name + 0.2·arity + 0.2·callee`:
CamelCase token-set Jaccard over the case-preserved method names, the
this-inclusive DC `params` delta (a soft term, never a veto — HoMM3
extends signatures), and shared-callee Jaccard restricted to the
593-name cross-game vocabulary (homm2 callees from an E8 scan of the
sibling images, DC callees from `evidence/dc-xref-graph.tsv`).

Yield: **12 `twin-strong`, 19 `twin-candidate`**, 887 `refused` with a
machine-readable reason (451 free-function by construction — that rule
alone kills the bzip/zlib collision — 113 no-shared-class, 227
weak-name, 73 no-candidates, 20 collision, 3 tie). Flagship strong
rows: `army::DoAttack → army::do_attack` (HoMM3's snake_case renames
surface as a family), `advManager::SystemOptions → DoSystemOptions`,
`combatManager::GetNextArmy → NextArmy`, `advManager::ProcessMapChange
→ ProcessMapChangeNew`, `townManager::DoTavern → DoTownTavern`.

Calibration (leave-one-out over the exact-name pairs, pools ≥2):
**top-1 487/499 (97.6%)**, median winning margin 0.40 against the 0.15
`twin-strong` bar. The honest false-positive estimate comes from a
rename-stress pass (delete each single token in turn, take the worst
rank): top-1 survives in 274/401, and only **5/401 (1.2%)** of the
stress winners would still grade `twin-strong` — `twin-candidate` is
noisier by design. Controls asserted on every run (the module refuses
to write on failure): the three flagship twins must grade `twin-*`,
bzip `compress`/`uncompress` must stay `refused free-function`,
`army::DispelGood` must never propose `army::Is` (the
substring-nonsense case), and `combatManager::SpellMessage` must not
steal `ShowSpellMessage` from its exact-name pair (homm2 carries both;
the consumed-row exclusion holds).

Output `evidence/homm2-overlap/twins.csv` — one row per unpaired homm2
function, so the file IS the residue the old generator silently
dropped. ANALYSIS OUTPUT, external-candidate grade.

## Admission path (nothing admitted by this report)

Evidence grade for everything here: **external-candidate** (homm2 is
another game's build; no HoMM3 retail-byte proof). Recommended use:

1. `string-anchor` rows → propose as *naming evidence* for the labeled
   functions (each needs a comparison of both bodies; the anchor
   literal is the argument).
2. `h2-source-template` rows → when the owning TU comes up for
   reconstruction (P5.2+), start the body from homm2's matched source
   and adapt; record the template's origin in the TU.
3. The 43 shared classes → corroborating input for the class-modeling
   phase (a third oracle beside Dreamcast layouts and NH3API types —
   homm2's layouts are VC 4.2-proven and will differ, but member
   *rosters* and method sets transfer).
4. `twin-strong` / `twin-candidate` rows → a rename proposal is BOTH a
   naming candidate and a template pointer; each needs an evidence-backed
   look at the homm2 body against the DC/retail evidence before its
   template is used, exactly like a `h2-source-template` row.
